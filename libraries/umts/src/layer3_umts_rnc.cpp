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
#include <iostream>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"

#include "cellular.h"
#include "cellular_layer3.h"
#include "cellular_umts.h"
#include "layer3_umts.h"
#include "layer3_umts_rnc.h"
#include "layer2_umts_rlc.h"

#define DEBUG_SPECIFIED_NODE_ID 0 // DEFAULT shoud be 0
#define DEBUG_UE_ID        0
#define DEBUG_SINGLE_NODE   (node->nodeId == DEBUG_SPECIFIED_NODE_ID)
#define DEBUG_PARAMETER    0
#define DEBUG_BACKBONE     0
#define DEBUG_RR           0
#define DEBUG_NBAP         0
#define DEBUG_TIMER        0
#define DEBUG_RANAP        0
#define DEBUG_CODE         0
#define DEBUG_HO           0
#define DEBUG_IUR          0
#define DEBUG_MEASUREMENT  0
#define DEBUG_GTP          0
#define DEBUG_RR_RELEASE   0
#define DEBUG_ASSERT       0  // some ERROR_Assert will be triggered
#define DEBUG_HSDPA        0

// /**
// FUNCTION   :: UmtsLayer3RncAtmptRmvActvSet
// LAYER      :: Layer3 RRC
// PURPOSE    :: Attempt to remove a nodeb into the active set
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : The UE RRC data in RNC
// + nodebInfo : UmtsMntNodebInfo*      : The monitored nodeb info
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncAtmptRmvActvSet(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    UmtsMntNodebInfo*   nodebInfo);

// /**
// FUNCTION   :: UmtsLayer3RncAtmptAddActvSet
// LAYER      :: Layer3 RRC
// PURPOSE    :: Attempt to add a nodeb into the active set
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : The UE RRC data in RNC
// + nodebInfo : UmtsMntNodebInfo*      : The monitored nodeb info
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncAtmptAddActvSet(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    UmtsMntNodebInfo*   nodebInfo);

// /**
// FUNCTION   :: UmtsLayer3RncServRabReqQueue
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Check RAB request serving queue to see if there is new
//               RAB request to be served
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + rncL3     : UmtsLayer3Rnc*         : Pointer to the RNC data
// RETURN     :: void
// **/
static
void UmtsLayer3RncServRabReqQueue(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsLayer3Rnc*          rncL3);

// /**
// FUNCTION   :: UmtsLayer3RncEndRabAssgtCheckQueue
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Finish the current RAB assigment procedure,
//               Check RAB request serving queue to see if there is new
//               RAB request to be served
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + rncL3     : UmtsLayer3Rnc*         : Pointer to the RNC data
// RETURN     :: void
// **/
static
void UmtsLayer3RncEndRabAssgtCheckQueue(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsLayer3Rnc*          rncL3);

// /**
// FUNCTION   :: UmtsLayer3RncRmvRabReqUponConnRel
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Remove queued RAB request upon RRC CONN release
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + rncL3    : UmtsLayer3Rnc *         : Pointer to RNC layer3 data
// + ueId     : NodeId                  : UE Id
// + curServReq: BOOL&                  : return whether this UE has 
//                                        an ongoing RAB assignment process
// RETURN     :: void
// **/
static
void UmtsLayer3RncRmvRabReqUponConnRel(
    Node*           node,
    UmtsLayer3Rnc*  rncL3,
    NodeId          ueId,
    BOOL&           curServReq);

// /**
// FUNCTION   :: UmtsLayer3RncUlCallAdmissionControl
// LAYER      :: Layer3 RRC
// PURPOSE    :: Check NodeB resources for CAC (UL)
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to the NodeB data in RNC
// + ulBwAlloc : int                : UL bandwidth allocated
// + ulBwReq   : int                : UL bandwidth requested
// RETURN     :: BOOL               : TRUE if resources can be allocated
// **/
BOOL UmtsLayer3RncUlCallAdmissionControl(
         Node*               node,
         UmtsLayer3Data*     umtsL3,
         UmtsRrcNodebDataInRnc* nodebRrc,
         int   ulBwAlloc,
         int   ulBwReq)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    if (rncL3->ulCacType == UMTS_CAC_TESTING)
    {
        // for testing
        // when UL  BW allocated is already reach a threshold
        // reject new services
        // implement a general function for differnet types of CAC
        if ((nodebRrc->totalUlBwAlloc + ulBwAlloc) >
              UMTS_MAX_UL_CAPACITY_3)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        // TODO: other CAC methods
        return TRUE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncDlCallAdmissionControl
// LAYER      :: Layer3 RRC
// PURPOSE    :: Check NodeB resources for CAC
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to nodeB data in RNC
// + dlBwAlloc : int                : DL bandwidth allocated
// + dlBwReq   : int                : DL bandwidth requested
// + hsdpaRb   : BOOL               : HSDPA radio bearer or not
// RETURN     :: BOOL               : TRUE if resources can be allocated
// **/
BOOL UmtsLayer3RncDlCallAdmissionControl(
         Node*               node,
         UmtsLayer3Data*     umtsL3,
         UmtsRrcNodebDataInRnc* nodebRrc,
         int   dlBwAlloc,
         int   dlBwReq,
         BOOL hsdpaRb)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    if (rncL3->dlCacType == UMTS_CAC_TESTING)
    {
        signed char slotFormatIndex = -1;
        // for testing
        // when DL  BW allocated is already reach a threshold
        // reject new services
        // implement a general function for differnet types of CAC
        if (hsdpaRb &&
            (nodebRrc->hsdpaInfo.totalHsBwReq + dlBwAlloc) * 4 /3 >
              UmtsLayer3GetDlPhChDataBitRate(
                UMTS_SF_16,
                &slotFormatIndex,
                UMTS_PHYSICAL_HSPDSCH) * 5)
        {
            // return FALSE;
            return TRUE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        // TODO: other CAC methods
        return TRUE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncPrintParameter
// LAYER      :: Layer 3
// PURPOSE    :: Print out RNC specific parameters
// PARAMETERS ::
// + node      : Node*           : Pointer to node.
// + rncL3     : UmtsLayer3Rnc * : Pointer to UMTS RNC Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncPrintParameter(Node *node, UmtsLayer3Rnc *rncL3)
{
    printf("Node%d(RNC)'s specific  parameters:\n", node->nodeId);
    //TIME_PrintClockInSecond(rncL3->para.sysBcastInterval, timeStr);
    //printf("    System info broadcast interval = %s\n", timeStr);
    printf("    SHO AS_Th = %f\n", rncL3->para.shoAsTh);
    printf("    SHO AS_Th_Hyst = %f\n", rncL3->para.shoAsThHyst);
    printf("    SHO AS_Rep_Hyst = %f\n", rncL3->para.shoAsRepHyst);

    return;
}

// /**
// FUNCTION   :: UmtsLayer3RncInitParameter
// LAYER      :: Layer 3
// PURPOSE    :: Initialize RNC specific parameters
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + rncL3     : UmtsLayer3Rnc *  : Pointer to UMTS RNC Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncInitParameter(Node *node,
                                const NodeInput *nodeInput,
                                UmtsLayer3Rnc *rncL3)
{
    BOOL wasFound;
    double doubleVal;

    // Read parameters for Soft Handover (SHO)
    // read the threshold (AS_Th) to add a cell into Active Set (AS)
    IO_ReadDouble(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "UMTS-RNC-SHO-AS-THRESHOLD",
                  &wasFound,
                  &doubleVal);

    if (wasFound && doubleVal > 0)
    {
        // do validation check first
        // stored the value
        rncL3->para.shoAsTh = doubleVal;
    }
    else
    {
        if (wasFound)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "UMTS Node%d(RNC): Value of parameter "
                    "UMTS-RNC-SHO-AS-THRESHOLD is invalid. It must be "
                    "a number larger than 0. "
                    "Default value as %.2f is used.\n",
                    node->nodeId,
                    UMTS_RNC_DEF_SHO_AS_THRESHOLD);
            ERROR_ReportWarning(errString);
        }

        // use the default value
        rncL3->para.shoAsTh = UMTS_RNC_DEF_SHO_AS_THRESHOLD;
    }

    // read the hysteresis for the AS threshold (AS_Th_Hyst)
    IO_ReadDouble(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "UMTS-RNC-SHO-AS-THRESHOLD-HYSTERESIS",
                  &wasFound,
                  &doubleVal);

    if (wasFound && doubleVal > 0)
    {
        // do validation check first
        // stored the value
        rncL3->para.shoAsThHyst = doubleVal;
    }
    else
    {
        if (wasFound)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "UMTS Node%d(RNC): Value of parameter "
                    "UMTS-RNC-SHO-AS-THRESHOLD-HYSTERESIS "
                    "is invalid. It must be "
                    "a number larger than 0. "
                    "Default value as %.2f is used.\n",
                    node->nodeId,
                    UMTS_RNC_DEF_SHO_AS_THRESHOLD_HYSTERESIS);
            ERROR_ReportWarning(errString);
        }

        // use the default value
        rncL3->para.shoAsThHyst = 
            UMTS_RNC_DEF_SHO_AS_THRESHOLD_HYSTERESIS;
    }

    // read the replacement hysteresis (AS_Rep_Hyst)
    IO_ReadDouble(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "UMTS-RNC-SHO-AS-REPLACE-HYSTERESIS",
                  &wasFound,
                  &doubleVal);

    if (wasFound && doubleVal > 0)
    {
        // do validation check first
        // stored the value
        rncL3->para.shoAsRepHyst = doubleVal;
    }
    else
    {
        if (wasFound)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "UMTS Node%d(RNC): Value of parameter "
                    "UMTS-RNC-SHO-AS-REPLACE-HYSTERESIS is invalid."
                    "It must be a number larger than 0. "
                    "Default value as %.2f is used.\n",
                    node->nodeId,
                    UMTS_RNC_DEF_SHO_AS_REPLACE_HYSTERESIS);
            ERROR_ReportWarning(errString);
        }

        // use the default value
        rncL3->para.shoAsRepHyst = UMTS_RNC_DEF_SHO_AS_REPLACE_HYSTERESIS;
    }

    return;
}

// /**
// FUNCTION   :: UmtsLayer3RncPrintCodeTree
// LAYER      :: Layer 3
// PURPOSE    :: Print the whole code tree at RNC for one NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + codeTree  : char * : Binary tree maintains alloc condition
// + nodebId   : NodeId : ID of the NodeB that this code tree belongs to
// RETURN     :: void   : NULL
// **/
static
void UmtsLayer3RncPrintCodeTree(Node *node, char *codeTree, NodeId nodebId)
{
    int i;
    printf("UMTS Node%d(RNC)'s current code tree of NodeB %d:\n",
           node->nodeId,
           nodebId);

    for (i = 0; i <= UMTS_RNC_MAX_SPREAD_FACTOR_LEVEL; i ++)
    {
        int startIndex = ((int)pow(2.0, i)) - 1;
        int endIndex = ((int)pow(2.0, i + 1)) - 1;
        int j;

        for (j = startIndex; j < endIndex; j ++)
        {
            printf("%d ", codeTree[j]);
        }
        printf("\n");
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncMarkCodeTree
// LAYER      :: Layer 3
// PURPOSE    :: Mark the code tree accordingly
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to the layer3 data
// + codeTree  : char *           : Binary tree maintains alloc condition
// + spreadFactor  : int          : level of spread factor in tree
// + numCodeAlloced : int          : Number of codes alloced
// + hsdpaRb : BOOL *        : HSDPA RB or not
// RETURN     :: void              : NULL
// **/
static
void UmtsLayer3RncMarkCodeTree(
         Node           *node,
         char           *codeTree,
         int            spreadFactor,
         int            *allocCodeList,
         int            numCodeAlloced,
         BOOL           hsdpaRb)
{
    int i;

    // for each code, we need to mark its children as reserved and mark
    // the branch from it to root as reserved
    // 1) Mark code and its children
    for (i = 0; i < numCodeAlloced; i ++)
    {
        int keyPart = 2 * allocCodeList[i];
        int coefficient = 2;
        int offset = 1;
        int j;
        int k;

        // mark the code itself
        if (!hsdpaRb)
        {
            codeTree[allocCodeList[i]] = UMTS_CODE_RESERVED;
        }
        else
        {
            codeTree[allocCodeList[i]] = UMTS_CODE_RESERVED_HSDPA;
        }

        // mark all childent of this code as reserved
        for (j = spreadFactor + 1; 
            j <= UMTS_RNC_MAX_SPREAD_FACTOR_LEVEL; 
            j ++)
        {
            // there are 2^(j - spreadFactor) children at this level
            for (k = 0; k < coefficient; k ++)
            {
                codeTree[keyPart + offset] = UMTS_CODE_RESERVED;

                offset ++;
            }

            keyPart *= 2;
            coefficient *= 2;
        }

        // 2) Mark codes on path from this code to root as reserved
        // get the immediate parent of allocated code
        int parentCode = allocCodeList[i] / 2;
        if (allocCodeList[i] % 2 == 0)
        {
            parentCode --;
        }

        for (j = spreadFactor - 1; j >= 0; j --)
        {
            if (codeTree[parentCode] != UMTS_CODE_FREE)
            {
                // this is optimization, if a pareent is already marked,
                // no need to proceed as all upper parents must have been
                // makred already.
                break;
            }

            // mark this code as reserved
            codeTree[parentCode] = UMTS_CODE_RESERVED;

            // find parent of this code
            if (parentCode % 2 == 0)
            {
                parentCode = parentCode / 2 - 1;
            }
            else
            {
                parentCode /= 2;
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncAllocCodeWithSpreadFactor
// LAYER      :: Layer 3
// PURPOSE    :: Allocate one or multiple codes with given spread factor.
//               In this function, the allocation is temporarily marked.
//               One need to finalize them later. The temp mark is for
//               possible cancels if allocation cannot be fulfill in whole
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to the layer3 data
// + codeTree  : char *           : Binary tree maintains alloc condition
// + spreadFactor  : int          : level of spread factor in tree
// + numCodeNeeded : int          : Number of codes needed
// + allocCodeList : int *        : To store codes allocated
// + hsdpaRb       : BOOL : HSDPA RB or not
// RETURN     :: int              : Actual number of codes allocated
// **/
static
int UmtsLayer3RncAllocCodeWithSpreadFactor(
        Node           *node,
        UmtsLayer3Data *umtsL3,
        char           *codeTree,
        int            spreadFactor,
        int            numCodeNeeded,
        int            *allocCodeList,
        BOOL           hsdpaRb)
{
    int numCodeAlloced;
    int startIndex;
    int endIndex;
    int i;

    // A code is free when its value is zero

    // search the level of given spread factor
    startIndex = ((int)pow(2.0, spreadFactor)) - 1;
    endIndex = ((int)pow(2.0, spreadFactor + 1)) - 1;
    numCodeAlloced = 0;
    for (i = startIndex; i < endIndex; i ++)
    {
        if (!hsdpaRb && codeTree[i] == UMTS_CODE_FREE)
        {
            // this code is available
            allocCodeList[numCodeAlloced ++] = i;
        }
        else if (hsdpaRb &&  codeTree[i] == UMTS_CODE_RESERVED_HSDPA)
        {
            allocCodeList[numCodeAlloced ++] = i;
        }

        if (numCodeAlloced == numCodeNeeded)
        {
            break;
        }
    }

    if (!hsdpaRb)
    {
        // only non HSDPA needs to be marked
        // HSDPA has benn marked during initialization
        UmtsLayer3RncMarkCodeTree(
            node, codeTree, spreadFactor,
            allocCodeList, numCodeAlloced, hsdpaRb);
    }

    return numCodeAlloced;
}

// /**
// FUNCTION   :: UmtsLayer3RncAllocNodebOvsf
// LAYER      :: Layer 3
// PURPOSE    :: Allocate ovsf code and spreading factor at NodeB side given
//               requested data rate.
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to the UE data in RNC
// + dataRate  : int                    : Requested data bit rate (bps)
// + sfs       : UmtsSpreadFactor*      : For returning of the allocated 
//                                        spread factors
// + codes     : unsigned int *         : For returning of the allocated
//                                        OVSF code indexes
// + numAlloc  : int*                   : Maximum number of codes allowed 
//                                        to be allocated
//                                      : and return actual number of 
//                                        allocated codes
// + hsdpaRb   : BOOL                   : HSDPA Rb or not
// RETURN     :: BOOL                   : TRUE, if new code assigned
// **/
static
BOOL UmtsLayer3RncAllocNodebOvsf(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsRrcNodebDataInRnc*  nodebRrc,
    int                     dataRate,
    UmtsModulationType      modType,
    UmtsCodingType          codingType,
    UmtsSpreadFactor*       sfs,
    unsigned int *          codes,
    int*                    numAlloc,
    BOOL                    hsdpaRb)
{
    signed char slotFormatIndex = -1;
    int ratesOfHsdpa;
    int ratesOfSpreadFactor[10] =  // data rates for each spread factor
            {7680000, // 7680kbps for code w/ spread factor 1
             3840000, // 3840kbps for code w/ spread factor 2
             UmtsLayer3GetDlPhChDataBitRate(UMTS_SF_4, &slotFormatIndex),  
                                    // 1920kKbps for code w/ spread factor 4
             UmtsLayer3GetDlPhChDataBitRate(UMTS_SF_8, &slotFormatIndex),  
                                    // 960kbps for code w/ spread factor 8
             UmtsLayer3GetDlPhChDataBitRate(UMTS_SF_16, &slotFormatIndex),  
                                   // 480kKbps for code w/ spread factor 16
             UmtsLayer3GetDlPhChDataBitRate(UMTS_SF_32, &slotFormatIndex),  
                                   // 240kbps for code w/ spread factor 32
             UmtsLayer3GetDlPhChDataBitRate(UMTS_SF_64, &slotFormatIndex),   
                                   // 120kbps  for code w/ spread factor 64
             UmtsLayer3GetDlPhChDataBitRate(UMTS_SF_128, &slotFormatIndex),   
                                   // 60kbps  for code w/ spread factor 128
             UmtsLayer3GetDlPhChDataBitRate(UMTS_SF_256, &slotFormatIndex),   
                                  // 30kbps  for code w/ spread factor 256
             UmtsLayer3GetDlPhChDataBitRate(UMTS_SF_512, &slotFormatIndex)};   
                                  // 15kbps   for code w/ spread factor 512

     UmtsSpreadFactor spreadFactorOfLevel[10] = // spread factor for rates
                         {UMTS_SF_1, UMTS_SF_2, UMTS_SF_4,
                          UMTS_SF_8, UMTS_SF_16,UMTS_SF_32,
                          UMTS_SF_64, UMTS_SF_128, 
                          UMTS_SF_256, UMTS_SF_512};

    ratesOfHsdpa  = UmtsLayer3GetDlPhChDataBitRate(
                        UMTS_SF_16,
                        &slotFormatIndex,
                        UMTS_PHYSICAL_HSPDSCH);

    int minLevel = UMTS_RNC_MIN_SPREAD_FACTOR_LEVEL;
    int maxLevel = UMTS_RNC_MAX_SPREAD_FACTOR_LEVEL;
    int deltaRate = 500; // if difference within delta, consider as same
    char errMsg[MAX_STRING_LENGTH];

    BOOL alloced = FALSE;
    unsigned int allocCodeList[UMTS_RNC_MAX_NUM_CODE_PER_ALLOC];
    UmtsSpreadFactor codeFactorList[UMTS_RNC_MAX_NUM_CODE_PER_ALLOC];
    int numCodeAlloced = 0;
    int rateNeeded;
    int spreadFactor;
    int i;
    BOOL newCodeAssigned = FALSE;

    if (DEBUG_CODE)
    {
        printf("UMTS Node%d (RNC) tries to alloc code for data "
               "rate %d at NodeB %d\n",
               node->nodeId, dataRate, nodebRrc->nodebId);
        printf("Code Tree before allocation:\n");
        UmtsLayer3RncPrintCodeTree(node,
                                   nodebRrc->codeTree,
                                   nodebRrc->nodebId);
    }

    if (!hsdpaRb)
    {
        // find the max spread factor which can support required rate
        // the coding rate is considered
        if (codingType == UMTS_NO_CODING)
        {
            rateNeeded = dataRate;
        }
        else if (codingType == UMTS_CONV_2)
        {
            rateNeeded = 2 * dataRate;
        }
        else // 1/3 coding rate
        {
            rateNeeded = 3 * dataRate;
        }

        spreadFactor = maxLevel;
        while (spreadFactor > minLevel &&
               rateNeeded - ratesOfSpreadFactor[spreadFactor] > deltaRate)
        {
            spreadFactor --;
        }

        if (rateNeeded - ratesOfSpreadFactor[spreadFactor] > deltaRate)
        {
            // The requested rate is too big, print warning and reduce
            // required rate to available rate
            sprintf(errMsg,
                    "UMTS Node%d (RNC), requested data "
                    "rate as %d is too high!\n",
                    node->nodeId,
                    dataRate);
            ERROR_ReportWarning(errMsg);

            *numAlloc = 0;

            return newCodeAssigned;
        }

        // starting from this lvl, try to find free 
        // codes to meet required rate
        alloced = FALSE;
        while (spreadFactor <= maxLevel && !alloced)
        {
            int numCodeThisStep;
            int numCodeNeeded;
            int codeThisStep[UMTS_RNC_MAX_NUM_CODE_PER_ALLOC];

            // calculate how many code still needed with this level
            // of spread factor for remaining rate needed
            numCodeNeeded = rateNeeded / ratesOfSpreadFactor[spreadFactor];
            if ((rateNeeded % ratesOfSpreadFactor[spreadFactor])>deltaRate)
            {
                numCodeNeeded ++;
            }

            // if number of code needed is more than allocated memory,
            // return error
            if (numCodeNeeded + numCodeAlloced > *numAlloc ||
                numCodeNeeded + numCodeAlloced > 
                UMTS_RNC_MAX_NUM_CODE_PER_ALLOC)
            {
                break;
            }

            // try to allocate such number of code with this spread factor
            numCodeThisStep = UmtsLayer3RncAllocCodeWithSpreadFactor(
                                  node,
                                  umtsL3,
                                  nodebRrc->codeTree,
                                  spreadFactor,
                                  numCodeNeeded,
                                  codeThisStep,
                                  hsdpaRb);

            if (numCodeThisStep == numCodeNeeded)
            {
                // allocation is fulfilled
                alloced = TRUE;
            }
            else
            {
                // calculate how much can be allocated and how much left
                rateNeeded -= (numCodeThisStep
                              * ratesOfSpreadFactor[spreadFactor]);
            }

            // copy allocated codes in this step to permanient one
            for (i = 0; i < numCodeThisStep; i ++)
            {
                codeFactorList[numCodeAlloced] =
                    spreadFactorOfLevel[spreadFactor];
                allocCodeList[numCodeAlloced ++] = codeThisStep[i];
            }

            // increase the level of spread factor, and keep search
            spreadFactor ++;
        }
    }
    else
    {
        // FOR HSDPA
        // check if HSPDSCH and HSSCCH exsits?
        // if not. allocate one SF-16 for HSPDSCH
        // and one SF-128 for HSSCCH
        // TODO: HSDPA admission ocntrol
        int numHsdpaCodeThisStep;
        int numHsdpaCodeNeeded = nodebRrc->hsdpaInfo.maxHsdpschSupport;
        int hsdpaCodeThisStep[UMTS_RNC_MAX_NUM_CODE_PER_ALLOC];

        if (nodebRrc->hsdpaInfo.numHspdschAlloc == 0)
        {
            // alloc HSPDSCH
            // try to allocate such number of code with this spread factor
            numHsdpaCodeThisStep = UmtsLayer3RncAllocCodeWithSpreadFactor(
                                  node,
                                  umtsL3,
                                  nodebRrc->codeTree,
                                  4, // for UMTS_SF_16
                                  numHsdpaCodeNeeded,
                                  hsdpaCodeThisStep,
                                  hsdpaRb);

            if (numHsdpaCodeThisStep == numHsdpaCodeNeeded)
            {
                // allocation is fulfilled
                alloced = TRUE;

                if (!newCodeAssigned)
                {
                    newCodeAssigned = TRUE;
                }
                nodebRrc->hsdpaInfo.numHspdschAlloc =
                    nodebRrc->hsdpaInfo.maxHsdpschSupport;

                // copy allocated codes in this step to permanient one
                for (i = 0; i < numHsdpaCodeThisStep; i ++)
                {
                    codeFactorList[numCodeAlloced] = UMTS_SF_16;

                    // keep a copy
                    nodebRrc->hsdpaInfo.hspdschCodeIndex[numCodeAlloced] =
                        hsdpaCodeThisStep[i];

                    allocCodeList[numCodeAlloced ++] = hsdpaCodeThisStep[i];
                }

                if (DEBUG_HSDPA)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_RRC);
                    printf("\n\tAlloc HS-DPSCH for nodeB %d\n",
                        nodebRrc->cellId);
                }
            }
        }
        else
        {
            // reuse the originally allocated one
            // copy the code to the list
            alloced = TRUE;

            for (i = 0; i < nodebRrc->hsdpaInfo.numHspdschAlloc; i ++)
            {
                codeFactorList[numCodeAlloced] = UMTS_SF_16;
                allocCodeList[numCodeAlloced ++] =
                    nodebRrc->hsdpaInfo.hspdschCodeIndex[i];
            }
            if (DEBUG_HSDPA)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\tno new HS-DPSCH alloc, because \n"
                    "nodeB %d has already been allocated\n",
                    nodebRrc->cellId);
            }
        }
        if (nodebRrc->hsdpaInfo.numHsscchAlloc == 0)
        {
            // alloc HSSCCH
            // assume 1 HSSCH allocated
            numHsdpaCodeNeeded = 1;

            // try to allocate such number of code with this spread factor
            numHsdpaCodeThisStep = UmtsLayer3RncAllocCodeWithSpreadFactor(
                                       node,
                                       umtsL3,
                                       nodebRrc->codeTree,
                                       7, // for UMTS_SF_128,
                                       numHsdpaCodeNeeded,
                                       hsdpaCodeThisStep,
                                       hsdpaRb);

            if (numHsdpaCodeThisStep == numHsdpaCodeNeeded)
            {
                // allocation is fulfilled
                alloced = TRUE;
                nodebRrc->hsdpaInfo.numHsscchAlloc ++;

                // copy allocated codes in this step to permanient one
                for (i = 0; i < numHsdpaCodeThisStep; i ++)
                {
                    codeFactorList[numCodeAlloced] = UMTS_SF_128;
                    // keep a copy
                    nodebRrc->hsdpaInfo.hsscchCodeIndex[numCodeAlloced] =
                        hsdpaCodeThisStep[i];
                    allocCodeList[numCodeAlloced ++] = hsdpaCodeThisStep[i];
                }

                if (DEBUG_HSDPA)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_RRC);
                    printf("\n\tAlloc HS-SSCH for nodeB %d\n",
                        nodebRrc->cellId);
                }
            }
        }
        else
        {
            // reuse the originally allocated one
            // copy the code to the list
            alloced = TRUE;

            for (i = 0;
                i < nodebRrc->hsdpaInfo.numHsscchAlloc;
                i ++)
            {
                codeFactorList[numCodeAlloced] = UMTS_SF_128;
                allocCodeList[numCodeAlloced ++] =
                    nodebRrc->hsdpaInfo.hsscchCodeIndex[i];
            }

            if (DEBUG_HSDPA)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\tno new HS-SCCH alloc, because \n"
                    "nodeB %d has already been allocated\n",
                    nodebRrc->cellId);
            }
        }
    }

    if (alloced)
    {
        // successfully allocated

        // Mark the reservations to allocated;
        for (i = 0; (unsigned int)i <sizeof(nodebRrc->codeTree); i ++)
        {
            if (!hsdpaRb && nodebRrc->codeTree[i] == UMTS_CODE_RESERVED)
            {
                nodebRrc->codeTree[i] = UMTS_CODE_ALLOCATED;

                // since new code has benn allocated
                // update rbsetuppara
                if (!newCodeAssigned)
                {
                    newCodeAssigned = TRUE;
                }
            }
            // HSDPA code will alwasy be in RESERVED state
        }

        *numAlloc = numCodeAlloced;
        for (i = 0; i < numCodeAlloced; i ++)
        {
            codes[i] = allocCodeList[i];
            sfs[i] = codeFactorList[i];
        }
    }
    else
    {
        // not successful, cancel reservations
        for (i = 0; (unsigned int)i < sizeof(nodebRrc->codeTree); i ++)
        {
            if (nodebRrc->codeTree[i] == UMTS_CODE_RESERVED)
            {
                nodebRrc->codeTree[i] = UMTS_CODE_FREE;
            }
        }

        *numAlloc = 0;
    }

    if (DEBUG_CODE)
    {
        printf("UMTS Node%d (RNC) allocated %d codes:",
               node->nodeId, numCodeAlloced);
        for (i = 0; i <numCodeAlloced; i ++)
        {
            printf(" %d", allocCodeList[i]);
        }
        printf("\n");
        printf("Code Tree after allocation:\n");
        UmtsLayer3RncPrintCodeTree(node,
                                   nodebRrc->codeTree,
                                   nodebRrc->nodebId);
    }

    return newCodeAssigned;
}

// /**
// FUNCTION   :: UmtsLayer3RncReleaseNodebOvsf
// LAYER      :: Layer 3
// PURPOSE    :: Release ovsf code at NodeB side at RNC node
// PARAMETERS ::
// + node      : Node *                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *        : Pointer to the layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc * : Pointer to the NodeB data at RNC
// + numCodes  : int                     : # of codes to be released
// + codeList  : unsigned int *          : List of codes to be released
// RETURN     :: void                    : NULL
// **/
static
void UmtsLayer3RncReleaseNodebOvsf(
         Node                   *node,
         UmtsLayer3Data         *umtsL3,
         UmtsRrcNodebDataInRnc  *nodebRrc,
         int                    numCodes,
         unsigned int           *codeList)
{
    int i;

    if (DEBUG_CODE)
    {
        int tmpIndex;
        printf("UMTS Node%d (RNC) tries to free %d codes for NodeB %d:",
               node->nodeId, numCodes, nodebRrc->nodebId);
        for (tmpIndex = 0; tmpIndex < numCodes; tmpIndex ++)
        {
            printf(" %d", codeList[tmpIndex]);
        }
        printf("\n");
        printf("Code Tree before free:\n");
        UmtsLayer3RncPrintCodeTree(node,
                                   nodebRrc->codeTree,
                                   nodebRrc->nodebId);
    }

    // loop to free each code
    for (i = 0; i < numCodes; i ++)
    {
        int keyPart = 2 * codeList[i];
        int coefficient = 2;
        int offset = 1;
        int sfLevel;
        int j;
        int k;

        nodebRrc->codeTree[codeList[i]] = UMTS_CODE_FREE;

        sfLevel = (int) (log((double)(codeList[i] + 1)) / log(2.0));
        // free children code of this code
        for (j = sfLevel + 1; j <= UMTS_RNC_MAX_SPREAD_FACTOR_LEVEL; j ++)
        {
            // there are 2^(j - spreadFactor) children at this level
            for (k = 0; k < coefficient; k ++)
            {
                nodebRrc->codeTree[keyPart + offset] = UMTS_CODE_FREE;

                offset ++;
            }

            keyPart *= 2;
            coefficient *= 2;
        }

        // check if need to remark parent codes
        int parentCode = codeList[i] / 2;
        if (codeList[i] % 2 == 0)
        {
            parentCode --;
        }

        for (j = sfLevel - 1; j >= 0; j --)
        {
            // check if left and right immediate child codes are all free
            if (nodebRrc->codeTree[parentCode * 2 + 1] == UMTS_CODE_FREE &&
                nodebRrc->codeTree[parentCode * 2 + 2] == UMTS_CODE_FREE)
            {
                // this parent code can be freed
                nodebRrc->codeTree[parentCode] = UMTS_CODE_FREE;
            }
            else
            {
                // no need to continue as there is another branch allocated
                break;
            }

            // find parent of this code
            if (parentCode % 2 == 0)
            {
                parentCode = parentCode / 2 - 1;
            }
            else
            {
                parentCode /= 2;
            }
        }
    }

    if (DEBUG_CODE)
    {
        printf("Code Tree after free:\n");
        UmtsLayer3RncPrintCodeTree(node,
                                   nodebRrc->codeTree,
                                   nodebRrc->nodebId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncInitUeRrc
// LAYER      :: Layer 3
// PURPOSE    :: Initialize a UE RRC data for an UE connecting to this RNC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + ueId      : NodeId           : UE ID
// + nodebRrc  : UmtsRrcNodebDataInRnc* : NodeB Data at RNC
// RETURN     :: UmtsRrcUeDataInRnc : The created UE RRC data
// **/
static
UmtsRrcUeDataInRnc* UmtsLayer3RncInitUeRrc(
    Node *node,
    NodeId ueId,
    UmtsRrcNodebDataInRnc* nodebRrc)
{
    BOOL   wasFound;
    char buf[MAX_STRING_LENGTH];

    UmtsRrcUeDataInRnc* rrcData =
        (UmtsRrcUeDataInRnc*) (MEM_malloc(sizeof(UmtsRrcUeDataInRnc)));
    memset(rrcData, 0, sizeof(UmtsRrcUeDataInRnc));
    rrcData->curSf = UMTS_SF_256; // by default, Sf 256 is used

    rrcData->ueId = ueId;
    rrcData->primScCode = ueId;                 //TODO

    rrcData->primCellId = nodebRrc->cellId;
    rrcData->cellSwitchQueue = new std::vector<UmtsCellSwitchQuePkt*>;

    rrcData->state = UMTS_RRC_STATE_IDLE;
    rrcData->subState = UMTS_RRC_SUBSTATE_CAMPED_NORMALLY;
    rrcData->transactions =
        new std::map<UmtsRRMessageType, unsigned char>;

    for (int i = 0; i < UMTS_MAX_RB_ALL_RAB; i++)
    {
        rrcData->rbRabId[i] = UMTS_INVALID_RAB_ID;
    }

    // get HSDPA of the UE
    IO_ReadString(node,
                  rrcData->ueId,
                  MAPPING_GetInterfaceIndexFromInterfaceAddress(
                        node,
                        MAPPING_GetDefaultInterfaceAddressFromNodeId(
                            node,
                            rrcData->ueId)),
                  node->partitionData->nodeInput,
                  "PHY-UMTS-HSDPA-CAPABLE",
                  &wasFound,
                  buf);
    if (wasFound  && strcmp(buf, "YES") == 0)
    {
        rrcData->hsdpaEnabled = TRUE;
    }
    else if (!wasFound  || strcmp(buf, "NO") == 0)
    {
        rrcData->hsdpaEnabled = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Specify PHY-UMTS-HSDPA-CAPABLE as YES|NO! Default value"
            "NO is used.");
        rrcData->hsdpaEnabled = FALSE;
    }

    //rrcData->rlcInfo = new std::list<UmtsRlcRrcEstablishArgs*>;
    rrcData->rbMappingInfo = new std::list<UmtsRbMapping*>;
    rrcData->rbPhChMappingList = new std::list<UmtsRbPhChMapping*>;

    rrcData->ulDcchBitSet = new std::bitset<UMTS_MAX_LOCH>;
    rrcData->ulDtchBitSet = new std::bitset<UMTS_MAX_LOCH>;

    rrcData->pendingDlDpdchInfo = NULL;

    rrcData->mntNodebList = new std::list<UmtsMntNodebInfo*>;

    UmtsMntNodebInfo* newMntNodeb =
                 new UmtsMntNodebInfo(nodebRrc->cellId);
    newMntNodeb->timeStamp = node->getNodeTime();
    newMntNodeb->cellStatus = UMTS_CELL_ACTIVE_SET;
    rrcData->mntNodebList->push_back(newMntNodeb);

    return rrcData;
}

// /**
// FUNCTION   :: UmtsLayer3RncFinalizeUeRrc
// LAYER      :: Layer 3
// PURPOSE    :: Finnalize UE RRC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + rrcData   : UmtsRrcUeDataInRnc : The created UE RRC data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncFinalizeUeRrc(Node *node,
                                UmtsRrcUeDataInRnc* rrcData)
{
    delete rrcData->cellSwitchQueue;
    delete rrcData->transactions;

    for (int i = 0; i<UMTS_MAX_RAB_SETUP; i++)
    {
        if (rrcData->rabInfos[i])
        {
            MEM_free(rrcData->rabInfos[i]);
        }
        if (rrcData->pendingRab[i])
        {
            delete(rrcData->pendingRab[i]);
        }
    }

    for (std::list<UmtsRbPhChMapping*>::iterator it =
                    rrcData->rbPhChMappingList->begin();
         it != rrcData->rbPhChMappingList->end();
         ++it)
    {
        delete *it;
    }
    delete rrcData->rbPhChMappingList;
    std::for_each(rrcData->rbMappingInfo->begin(),
                 rrcData->rbMappingInfo->end(),
                 UmtsFreeStlItem<UmtsRbMapping>());
    delete rrcData->rbMappingInfo;

    if (rrcData->pendingDlDpdchInfo)
        MEM_free(rrcData->pendingDlDpdchInfo);

    delete rrcData->ulDcchBitSet;
    delete rrcData->ulDtchBitSet;

    for (int i=0; i < UMTS_MAX_TRCH; i++)
    {
        if (rrcData->ulDchInfo[i])
        {
            MEM_free(rrcData->ulDchInfo[i]);
        }
    }
    for (int i=0; i<UMTS_MAX_DPDCH_UL; i++)
    {
        if (rrcData->ulDpdchInfo[i])
        {
            MEM_free(rrcData->ulDpdchInfo[i]);
        }
    }

    for (std::list<UmtsMntNodebInfo*>::iterator it =
            rrcData->mntNodebList->begin();
         it != rrcData->mntNodebList->end();
         it++)
    {
        delete (*it);
    }
    delete rrcData->mntNodebList;

    if (rrcData->timerMeasureCheck)
    {
        MESSAGE_CancelSelfMsg(node, rrcData->timerMeasureCheck);
        rrcData->timerMeasureCheck = NULL;
    }

    if (rrcData->timerT3101)
    {
        MESSAGE_CancelSelfMsg(node, rrcData->timerT3101);
        rrcData->timerT3101 = NULL;
    }
    MEM_free(rrcData);
}

// /**
// FUNCTION   :: UmtsLayer3RncFindUeRrc
// LAYER      :: Layer 3
// PURPOSE    :: Find a UE RRC data in RNC layer3 data structure
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + rncL3     : UmtsLayer3Rnc *  : Pointer to UMTS RNC Layer3 data
// + ueId      : NodeId           : UE Id
// RETURN     :: UmtsRrcUeDataInRnc* : The created UE RRC data
// **/
static
UmtsRrcUeDataInRnc* UmtsLayer3RncFindUeRrc(
    Node* node,
    UmtsLayer3Rnc* rncL3,
    NodeId ueId)
{
    std::list<UmtsRrcUeDataInRnc*>::iterator itemPos;
    itemPos = std::find_if(rncL3->ueRrcs->begin(),
                           rncL3->ueRrcs->end(),
                           UmtsFindItemByUeId<UmtsRrcUeDataInRnc>(ueId));
    if (itemPos != rncL3->ueRrcs->end())
    {
        return (*itemPos);
    }
    else
    {
        return NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncFindNodebRrc
// LAYER      :: Layer 3
// PURPOSE    :: Find a NodeB RRC data in RNC layer3 data structure
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + rncL3     : UmtsLayer3Rnc *  : Pointer to UMTS RNC Layer3 data
// + nodebId   : NodeId           : NodeB Id
// RETURN     :: UmtsRrcNodebDataInRnc* : The nodeb RRC data
// **/
static
UmtsRrcNodebDataInRnc* UmtsLayer3RncFindNodebRrc(
    Node* node,
    UmtsLayer3Rnc* rncL3,
    NodeId nodebId)
{
    UmtsRrcNodebDataInRnc* nodebRrc = NULL;
    std::list<UmtsRrcNodebDataInRnc*>::iterator itemPos;
    itemPos = std::find_if(
                rncL3->nodebRrcs->begin(),
                rncL3->nodebRrcs->end(),
                UmtsFindItemByNodebId
                    <UmtsRrcNodebDataInRnc>(nodebId));
    if (itemPos != rncL3->nodebRrcs->end())
    {
        nodebRrc = *itemPos;
    }
    return nodebRrc;
}

#if 0
// /**
// FUNCTION   :: UmtsLayer3RncAddUeRrc
// LAYER      :: Layer 3
// PURPOSE    :: Create a UE RRC data for a newly connected UE
//               add into the UE RRC data list
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + rncL3     : UmtsLayer3Rnc *  : Pointer to UMTS RNC Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc : The ue RRC data
// RETURN     :: BOOL : TRUE if success
// **/
static
BOOL UmtsLayer3RncAddUeRrc(
    Node* node,
    UmtsLayer3Rnc* rncL3,
    UmtsRrcUeDataInRnc* ueRrc)
{
    BOOL retVal = FALSE;
    if (!UmtsLayer3RncFindUeRrc(node, rncL3, ueRrc->ueId))
    {
        rncL3->ueRrcs->push_back(ueRrc);
        retVal = TRUE;
    }
    return retVal;
}
#endif

// /**
// FUNCTION   :: UmtsLayer3RncRemoveUeRrc
// LAYER      :: Layer 3
// PURPOSE    :: Remove a UE RRC data from the UE RRC data list
//               and detroy the data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + ueId      : UInt32           : UE Id
// + rncL3     : UmtsLayer3Rnc *  : Pointer to UMTS RNC Layer3 data
// RETURN     :: BOOL : TRUE if success
// **/
static
BOOL UmtsLayer3RncRemoveUeRrc(
    Node* node,
    UInt32 ueId,
    UmtsLayer3Rnc* rncL3)
{
    BOOL retVal = FALSE;

    std::list<UmtsRrcUeDataInRnc*>::iterator itemPos;
    itemPos = std::find_if(rncL3->ueRrcs->begin(),
                           rncL3->ueRrcs->end(),
                           UmtsFindItemByUeId<UmtsRrcUeDataInRnc>(ueId));
    if (itemPos != rncL3->ueRrcs->end())
    {
        UmtsRrcUeDataInRnc* ueRrcData = *itemPos;
        UmtsLayer3RncFinalizeUeRrc(node, ueRrcData);
        rncL3->ueRrcs->erase(itemPos);

        retVal = TRUE;
    }
    return retVal;
}

// /**
// FUNCTION   :: UmtsLayer3RncPreAllocNodebRrc
// LAYER      :: Layer 3
// PURPOSE    :: preallocate some resource in a NodeB RRC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to the rrcData
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncPreAllocNodebRrc(
         Node* node,
         UmtsRrcNodebDataInRnc* rrcData)
{
    // reserve OVSF code for DL common control channel
    // reserve the code for CPICH
    int codeThisStep[UMTS_RNC_MAX_NUM_CODE_PER_ALLOC];
    int numCodeAlloc;
    int i;
    numCodeAlloc =5;
    for (i = 0; i < numCodeAlloc; i ++)
    {
        // reserve the code for
        // CPICH
        // P-CCPCH
        // S-CCPCH
        // PICH
        // AICH
        codeThisStep[i] = 255 + i;
    }
    // SF 256
    UmtsLayer3RncMarkCodeTree(
        node, rrcData->codeTree, 8,
        codeThisStep, numCodeAlloc, FALSE);


    if (rrcData->hsdpaEnabled)
    {
        // reserved for HS-PDSCH
        // assume only 5 codes supported
        if (rrcData->hsdpaInfo.maxHsdpschSupport == 5)
        {
            numCodeAlloc = 5;

            for (i = 0; i < numCodeAlloc; i ++)
            {
                // for HS-DPSCH
                codeThisStep[i] = 26 + i;
            }

            // SF 16
            UmtsLayer3RncMarkCodeTree(
                node, rrcData->codeTree, 4,
                codeThisStep, numCodeAlloc, TRUE);

            // for HS-SCCH
            numCodeAlloc = 1;
            codeThisStep[0] = 130;
            // SF 128
            UmtsLayer3RncMarkCodeTree(
                node, rrcData->codeTree, 7,
                codeThisStep, numCodeAlloc, TRUE);
        }
        else
        {
            // not supported
            //
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncInitNodebRrc
// LAYER      :: Layer 3
// PURPOSE    :: Initialize a NodeB RRC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodebId   : NodeId           : NodeB ID
// RETURN     :: UmtsRrcNodebDataInRnc : The created Nodeb RRC data
// **/
static
UmtsRrcNodebDataInRnc* UmtsLayer3RncInitNodebRrc(
    Node *node,
    NodeId nodebId,
    UInt32 cellId)
{
    BOOL   wasFound;
    char buf[MAX_STRING_LENGTH];
    UmtsRrcNodebDataInRnc* rrcData =
        (UmtsRrcNodebDataInRnc*) 
        (MEM_malloc(sizeof(UmtsRrcNodebDataInRnc)));
    memset(rrcData, 0, sizeof(UmtsRrcNodebDataInRnc));

    rrcData->nodebId = nodebId;
    rrcData->cellId = cellId;

    // get HSDPA of the nodeB
    IO_ReadString(node,
                  rrcData->nodebId,
                  MAPPING_GetInterfaceIndexFromInterfaceAddress(
                        node,
                        MAPPING_GetDefaultInterfaceAddressFromNodeId(
                            node,
                            rrcData->nodebId)),
                  node->partitionData->nodeInput,
                  "PHY-UMTS-HSDPA-CAPABLE",
                  &wasFound,
                  buf);
    if (wasFound  && strcmp(buf, "YES") == 0)
    {
        rrcData->hsdpaEnabled = TRUE;
    }
    else if (!wasFound || strcmp(buf, "NO") == 0)
    {
        rrcData->hsdpaEnabled = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Specify PHY-UMTS-HSDPA-CAPABLE as YES|NO! Default value"
            "NO is used.");
        rrcData->hsdpaEnabled = FALSE;
    }

    if (rrcData->hsdpaEnabled)
    {
        rrcData->hsdpaInfo.maxHsdpschSupport =
            UMTS_RNC_MAX_HSDSCH_CODE;
    }
    // preallocate resource for common channels
    // and reserve for HSDPA
    UmtsLayer3RncPreAllocNodebRrc(node, rrcData);

    rrcData->rbMappingInfo = new std::list<UmtsRbMapping*>;
    rrcData->rbPhChMappingList = new std::list<UmtsRbPhChMapping*>;

    rrcData->dlDcchBitSet = new std::bitset<UMTS_MAX_LOCH_PER_CELL>;
    rrcData->dlDtchBitSet = new std::bitset<UMTS_MAX_LOCH_PER_CELL>;

    rrcData->dlDchInfo = new std::list<UmtsDlDchInfo*>;
    rrcData->dlDchBitSet = new std::bitset<UMTS_MAX_DL_TRCH_PER_CELL>;

    rrcData->dlDpdchInfo = new std::list<UmtsDlDpdchInfo*>;
    rrcData->dlDpdchBitSet = new std::bitset<UMTS_MAX_DL_DPCH_PER_CELL>;

    // HS-SSCH
    // no transport channle/logical channel maping to it;
    rrcData->dlHsscchInfo = new std::list<UmtsDlHsscchInfo*>;

    return rrcData;
}

// /**
// FUNCTION   :: UmtsLayer3RncFinalizeNodebRrc
// LAYER      :: Layer 3
// PURPOSE    :: Finnalize NodeB RRC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + rrcData   : UmtsRrcNodebDataInRnc : The Nodeb RRC data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncFinalizeNodebRrc(
    Node *node,
    UmtsRrcNodebDataInRnc* rrcData)
{
    for (std::list<UmtsRbPhChMapping*>::iterator it =
                    rrcData->rbPhChMappingList->begin();
         it != rrcData->rbPhChMappingList->end();
         ++it)
    {
        delete *it;
    }
    delete rrcData->rbPhChMappingList;
    std::for_each(rrcData->rbMappingInfo->begin(),
                 rrcData->rbMappingInfo->end(),
                 UmtsFreeStlItem<UmtsRbMapping>());
    delete rrcData->rbMappingInfo;

    delete rrcData->dlDcchBitSet;
    delete rrcData->dlDtchBitSet;

    std::for_each(rrcData->dlDchInfo->begin(),
                  rrcData->dlDchInfo->end(),
                  UmtsFreeStlItem<UmtsDlDchInfo>());
    delete rrcData->dlDchInfo;
    delete rrcData->dlDchBitSet;

    std::for_each(rrcData->dlDpdchInfo->begin(),
                  rrcData->dlDpdchInfo->end(),
                  UmtsFreeStlItem<UmtsDlDpdchInfo>());
    delete rrcData->dlDpdchInfo;
    delete rrcData->dlDpdchBitSet;

    delete rrcData->dlHsscchInfo;

    MEM_free(rrcData);
}

// /**
// FUNCTION   :: UmtsLayer3RncRemoveRab
// LAYER      :: Layer 3
// PURPOSE    :: Delete RAB control data after releasing RAB
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + ueData    : UmtsRrcUeDataInRnc: UE data structure in RNC
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncRemoveRab(
    Node *node,
    UmtsRrcUeDataInRnc* ueRrc,
    char rabId)
{
    MEM_free(ueRrc->rabInfos[(int)rabId]);
    ueRrc->rabInfos[(int)rabId] = NULL;

    if (ueRrc->pendingRab[(int)rabId])
    {
        delete ueRrc->pendingRab[(int)rabId];
        ueRrc->pendingRab[(int)rabId] = NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncPrintStats
// LAYER      :: Layer 3
// PURPOSE    :: Print out RNC specific statistics
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + rncL3     : UmtsLayer3Rnc *  : Pointer to UMTS RNC Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncPrintStats(Node *node, UmtsLayer3Rnc *rncL3)
{
    char buf[MAX_STRING_LENGTH];
    std::list<UmtsRrcNodebDataInRnc*>::iterator nodebPos;

    for (nodebPos = rncL3->nodebRrcs->begin();
         nodebPos != rncL3->nodebRrcs->end();
         ++nodebPos)
    {
        // final update Ul resource
        (*nodebPos)->ulResUtil += (*nodebPos)->totalUlBwAlloc *
                               (node->getNodeTime() -
                               (*nodebPos)->lastUlResUpdateTime) / SECOND;
        sprintf(buf,
                "Average UL radio resource allocated "
                "at NodeB %d (bps) = %d",
                (*nodebPos)->nodebId,
                static_cast<int>((*nodebPos)->ulResUtil / 
                                 (node->getNodeTime() / SECOND)));
        IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

        sprintf(buf,
                "Peak UL radio resource allocated at NodeB %d (bps) = %d",
                (*nodebPos)->nodebId,
                (*nodebPos)->peakUlBwAlloc);
        IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

        // final update Dl resource
        (*nodebPos)->dlResUtil += (*nodebPos)->totalDlBwAlloc *
                               (node->getNodeTime() -
                               (*nodebPos)->lastDlResUpdateTime) / SECOND;
        sprintf(buf,
               "Average DL radio resource allocated at NodeB %d (bps) = %d ",
                (*nodebPos)->nodebId,
                static_cast<int>((*nodebPos)->dlResUtil / 
                                 (node->getNodeTime() / SECOND)));
        IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

        sprintf(buf,
                "Peak DL radio resource allocated at NodeB %d (bps) = %d",
                (*nodebPos)->nodebId,
                (*nodebPos)->peakDlBwAlloc);
        IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);
    }

    // RRC
    sprintf(
        buf,
        "Number of RRC CONN REQUEST messages received = %d",
        rncL3->stat.numRrcConnReqRcvd);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of RRC CONN SETUP messages sent = %d",
        rncL3->stat.numRrcConnSetupSent);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of RRC CONN SETUP COMPLETE messages received = %d",
        rncL3->stat.numRrcConnSetupCompRcvd);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of RRC CONN REJECT messages sent = %d",
        rncL3->stat.numRrcConnSetupRejSent);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
            "Number of data packets received from UE (via NodeB) = %d",
            rncL3->stat.numDataPacketFromUe);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
            "Number of data packets from UE dropped due to no RRC = %d",
            rncL3->stat.numDataPacketFromUeDroppedNoRrc);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
            "Number of data packets from UE dropped due to no RAB = %d",
            rncL3->stat.numDataPacketFromUeDroppedNoRab);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
            "Number of PS domain data packets forwarded to SGSN = %d",
            rncL3->stat.numPsDataPacketToSgsn);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
            "Number of CS domain data packets forwarded to SGSN = %d",
            rncL3->stat.numCsDataPacketToSgsn);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
            "Number of data packets received from SGSN = %d",
            rncL3->stat.numDataPacketFromSgsn);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
            "Number of data packets from SGSN dropped due to no RRC = %d",
            rncL3->stat.numDataPacketFromSgsnDroppedNoRrc);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
            "Number of data packets from SGSN dropped due to no RAB = %d",
            rncL3->stat.numDataPacketFromSgsnDroppedNoRab);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
       "Number of PS domain data packets forwarded to UE (via NodeB) = %d",
            rncL3->stat.numPsDataPacketToUe);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    sprintf(buf,
       "Number of CS domain data packets forwarded to UE (via NodeB) = %d",
            rncL3->stat.numCsDataPacketToUe);
    IO_PrintStat(node, "Layer3", "UMTS RNC", ANY_DEST, -1, buf);

    return;
}

// /**
// FUNCTION   :: UmtsLayer3RncSendNbapMessageToNodeb
// LAYER      :: Layer 3
// PURPOSE    :: Send a NBAP message to a Nodeb
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the NBAP message
// + msgType   : UmtsNbapMessageType: NBAP message type
// + transctId : unsigned char    : message transaction Id
// + nodebId   : NodeId           : The NodeB ID
// + ueId      : NodeId           : The UE ID associated in the message
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncSendNbapMessageToNodeb(Node *node,
                                         UmtsLayer3Data *umtsL3,
                                         Message *msg,
                                         UmtsNbapMessageType msgType,
                                         unsigned char transctId,
                                         NodeId nodebId,
                                         NodeId ueId)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsLayer3NodebInfo *itemPtr = NULL;
    itemPtr = (UmtsLayer3NodebInfo *)
              FindItem(node,
                       rncL3->nodebList,
                       0,
                       (char *) &(nodebId),
                       sizeof(nodebId));
    ERROR_Assert(itemPtr, "UmtsLayer3RncSendNbapMessageToNodeb: "
        "cannot find NodeB info. ");

    UmtsAddNbapHeader(node,
                      msg,
                      transctId,
                      ueId);

    UmtsAddIubBackboneHeader(node,
                             msg,
                             ANY_NODEID,
                             (UInt8)msgType);

    UmtsLayer3SendMsgOverBackbone(node,
                                  msg,
                                  itemPtr->nodeAddr,
                                  itemPtr->interfaceIndex,
                                  IPTOS_PREC_INTERNETCONTROL,
                                  1);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRrcMessageThroughNodeb
// LAYER      :: Layer 3
//
// PURPOSE    :: Send a NBAP message to a Nodeb
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the NBAP message
// + ueId      : NodeId           : The destination UE Id
// + rbId      : UInt8            : The destination RB Id
// + nodebId   : NodeId           : The NodeB ID in which the 
//                                  destination UE resides
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncSendRrcMessageThroughNodeb(Node *node,
                                             UmtsLayer3Data *umtsL3,
                                             Message *msg,
                                             NodeId ueId,
                                             char rbId,
                                             NodeId nodebId)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsLayer3NodebInfo *itemPtr = NULL;
    itemPtr = (UmtsLayer3NodebInfo *)
              FindItem(node,
                       rncL3->nodebList,
                       0,
                       (char *) &(nodebId),
                       sizeof(nodebId));

    ERROR_Assert(itemPtr, "UmtsLayer3RncSendRrcMessageThroughNodeb: "
        "cannot find NodeB info. ");

    UmtsAddIubBackboneHeader(node,
                             msg,
                             ueId,
                             rbId);

    UmtsLayer3SendMsgOverBackbone(node,
                                  msg,
                                  itemPtr->nodeAddr,
                                  itemPtr->interfaceIndex,
                                  IPTOS_PREC_INTERNETCONTROL,
                                  1);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRanapMsgToSgsn
// LAYER      :: Layer 3
//
// PURPOSE    :: Send a RANAP message to the SGSN
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg       : Message*               : Msg containing the NBAP message
// + ueId      : NodeId                 : UE ID
// + msgType   : UmtsRanapMessageType   : RANAP message type
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncSendRanapMsgToSgsn(
    Node *node,
    UmtsLayer3Data *umtsL3,
    Message *msg,
    NodeId nodeId,
    UmtsRanapMessageType msgType)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsAddIuBackboneHeader(node,
                            msg,
                            nodeId,
                            msgType);

    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        rncL3->sgsnInfo.nodeAddr,
        rncL3->sgsnInfo.interfaceIndex,
        IPTOS_PREC_INTERNETCONTROL,
        1);
}

// /**
// FUNCTION   :: UmtsLayer3RncAddIurHdr
// LAYER      :: Layer 3
//
// PURPOSE    :: build a IUR message to another RNC
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg       : Message*               : Msg containing the NBAP message
// + ueId      : NodeId                 : ueId
// + msgType   : UmtsRnsapMessageType   : RANAP message type
// RETURN     :: MESSAGE*
// **/
static
Message* UmtsLayer3RncAddIurHdr(
    Node *node,
    UmtsLayer3Data *umtsL3,
    Message* msg,
    NodeId ueId,
    UmtsRnsapMessageType msgType,
    const char* additionInfo,
    int   additionInfoSize)
{
    std::string headerInfo;
    headerInfo.reserve(sizeof(char)
                        + sizeof(ueId)
                        + sizeof(additionInfoSize));
    headerInfo += (char)msgType;
    headerInfo.append((char*)&ueId, sizeof(ueId));
    if (additionInfo && additionInfoSize > 0)
    {
        headerInfo.append(additionInfo, additionInfoSize);
    }
    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_IUR,
                          headerInfo.data(),
                          (int)headerInfo.size());

    return msg;
}

// /**
// FUNCTION   :: UmtsLayer3RncSendIurMsg
// LAYER      :: Layer 3
//
// PURPOSE    :: Send a IUR message to another RNC
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg       : Message*               : Msg containing the NBAP message
// + destRnc   : NodeId                 : Destination RNC ID
// + msgType   : UmtsRnsapMessageType   : RANAP message type
// RETURN     :: MESSAGE*
// **/
static
void UmtsLayer3RncSendIurMsg(
    Node *node,
    UmtsLayer3Data *umtsL3,
    Message* msg,
    NodeId ueId,
    NodeId destRnc,
    UmtsRnsapMessageType msgType,
    const char* additionInfo = NULL,
    int   additionInfoSize = 0)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tsends an IUR message to RNC: %u \n",
            destRnc);
        UmtsPrintMessage(std::cout, msg);
    }
    Message* iurMsg = UmtsLayer3RncAddIurHdr(
                            node,
                            umtsL3,
                            msg,
                            ueId,
                            msgType,
                            additionInfo,
                            additionInfoSize);

    UmtsLayer3RncInfo* rncInfo = (UmtsLayer3RncInfo*)
                                 FindItem(node,
                                          rncL3->neighRncList,
                                          0,
                                          (char*)&destRnc,
                                          sizeof(destRnc));
    if (rncInfo)
    {
        UmtsLayer3SendMsgOverBackbone(
            node,
            iurMsg,
            rncInfo->nodeAddr,
            rncInfo->interfaceIndex,
            IPTOS_PREC_INTERNETCONTROL,
            1);
    }
    else
    {
        UmtsLayer3RncSendRanapMsgToSgsn(
            node,
            umtsL3,
            iurMsg,
            destRnc,
            UMTS_RANAP_MSG_TYPE_IUR_MESSAGE_FORWARD);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncSendPktToUeOnDch
// LAYER      :: Layer 3
//
// PURPOSE    :: Send a packet (signalling or data) via dedicated channel
//               to the UE
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the NBAP message
// + ueId      : NodeId           : The destination UE Id
// + rbId      : UInt8            : The destination RB Id
// + nodebId   : NodeId           : The NodeB ID in which the 
//                                  destination UE resides
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncSendPktToUeOnDch(
    Node *node,
    UmtsLayer3Data *umtsL3,
    Message *msg,
    NodeId ueId,
    char rbId)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (ueRrc->primCellSwitch)
    {
        ueRrc->cellSwitchQueue->push_back(
                new UmtsCellSwitchQuePkt(msg, rbId));
    }
    else
    {
        UmtsCellCacheInfo* cellCache = rncL3->cellCacheMap->find(
                                            ueRrc->primCellId)->second;

        if (cellCache->rncId == node->nodeId)
        {
            UmtsLayer3RncSendRrcMessageThroughNodeb(
                node,
                umtsL3,
                msg,
                ueId,
                rbId,
                cellCache->nodebId);
        }
        else
        {
            char iurHdrInfo[10];
            int index = 0;
            memcpy(iurHdrInfo, &ueRrc->primCellId, sizeof(UInt32));
            index += sizeof(UInt32);
            iurHdrInfo[index++] = rbId;
            UmtsLayer3RncSendIurMsg(
                node,
                umtsL3,
                msg,
                ueId,
                cellCache->rncId,
                UMTS_RNSAP_MSG_TYPE_DOWNLINK_TRANSFER,
                iurHdrInfo,
                index);
        }
    }
}

#if 0
// /**
// FUNCTION   :: UmtsLayer3RncSendIurSrncUpMsg
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send a IUR message a DRNC to update the SRNC
//               Called when SRNC relocation has been done, and
//               the DRNCs which haven't become into SRNC should
//               be notified
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendIurSrncUpMsg(
    Node* node,
    UmtsLayer3Data *umtsL3,
    NodeId drncId,
    NodeId ueId)
{
    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, sizeof(NodeId), TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg), &node->nodeId, sizeof(NodeId));

    UmtsLayer3RncSendIurMsg(
        node,
        umtsL3,
        msg,
        ueId,
        drncId,
        UMTS_RNSAP_MSG_TYPE_SRNC_UPDATE);
}
#endif

// /**
// FUNCTION   :: UmtsLayer3RncSendIurNodebJoinActvSetRes
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Send JOIN ACTIVE SET RESPONSE from Nodeb back to SRNC
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + success        : BOOL                   : Join active set result
// + ueId           : NodeId                 : The ue id originating the msg
// + cellId         : UInt32                 : Cell Id of the nodeB
// + cellInfo       : const char*            : Cell Info
// + cellInfoSize   : int                    : Size of cell info 
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncSendIurNodebJoinActvSetRes(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    BOOL                        success,
    NodeId                      ueId,
    UInt32                      cellId,
    const char*                 cellInfo = NULL,
    int                         cellInfoSize = 0)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsUeSrncMap::iterator mapIter = rncL3->ueSrncMap->find(ueId);
    if (mapIter == rncL3->ueSrncMap->end())
    {
        return;
    }
    NodeId srncId = mapIter->second->srncId;
    UmtsRnsapMessageType msgType
        = UMTS_RNSAP_MSG_TYPE_JOIN_ACTIVE_SET_RESPONSE;

    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(char) + cellInfoSize,
                        TRACE_UMTS_LAYER3);
    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);

    packet[index++] = (char)success;
    if (cellInfo && cellInfoSize > 0)
    {
        memcpy(&packet[index], cellInfo, cellInfoSize);
    }
    UmtsLayer3RncSendIurMsg(
        node,
        umtsL3,
        msg,
        ueId,
        srncId,
        msgType,
        (char*)&cellId,
        sizeof(cellId));
}

// /**
// FUNCTION   :: UmtsLayer3RncSendIurNodebLeaveActvSetRes
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Send LEAVE ACTIVE SET RESPONSE from Nodeb back to SRNC
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + umtsL3         : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
// + ueId           : NodeId            : The ue id originating the message
// + cellId         : UInt32                 : Cell Id of the nodeB
// + cellInfo       : const char*            : Cell Info
// + cellInfoSize   : int                    : Size of cell info 
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncSendIurNodebLeaveActvSetRes(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    NodeId                      ueId,
    UInt32                      cellId,
    const char*                 cellInfo,
    int                         cellInfoSize)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsUeSrncMap::iterator mapIter = rncL3->ueSrncMap->find(ueId);
    if (mapIter == rncL3->ueSrncMap->end())
    {
        return;
    }
    NodeId srncId = mapIter->second->srncId;
    UmtsRnsapMessageType msgType
        = UMTS_RNSAP_MSG_TYPE_LEAVE_ACTIVE_SET_RESPONSE;

    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        cellInfoSize,
                        TRACE_UMTS_LAYER3);
    char* packet = MESSAGE_ReturnPacket(msg);
    memcpy(packet, cellInfo, cellInfoSize);
    UmtsLayer3RncSendIurMsg(
        node,
        umtsL3,
        msg,
        ueId,
        srncId,
        msgType,
        (char*)&cellId,
        sizeof(cellId));
}

// /**
// FUNCTION   :: UmtsLayer3RncSendIurNodebRbSetupRes
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Send IUR NODEB RB SETUP RESPONSE to SRNC
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + umtsL3         : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
// + success        : BOOL              : setup successully or not
// + ueId           : NodeId            : The ue id originating the message
// + rbSetupPara    : const UmtsNodebRbSetupPara&  : RB Setup parameters
// RETURN     ::    : void
// **/
static
void UmtsLayer3RncSendIurNodebRbSetupRes(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    BOOL                        success,
    NodeId                      ueId,
    const UmtsNodebRbSetupPara& rbSetupPara)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsUeSrncMap::iterator mapIter = rncL3->ueSrncMap->find(ueId);
    if (mapIter == rncL3->ueSrncMap->end())
    {
        return;
    }
    NodeId srncId = mapIter->second->srncId;
    UmtsRnsapMessageType msgType
        = UMTS_RNSAP_MSG_TYPE_RADIO_BEARER_SETUP_RESPONSE;

    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(char) + sizeof(rbSetupPara),
                        TRACE_UMTS_LAYER3);
    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);

    packet[index++] = (char)success;
    memcpy(&packet[index], &rbSetupPara, sizeof(rbSetupPara));
    UmtsLayer3RncSendIurMsg(
        node,
        umtsL3,
        msg,
        ueId,
        srncId,
        msgType);
}

// /**
// FUNCTION   :: UmtsLayer3RncRespToRabAssgt
// LAYER      :: Layer 3
// PURPOSE    :: Send a RAB_ASSIGNENT_RESPOND to 
//               reply RAB_ASSIGNMENT_REQUEST .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + setupSuccess   : BOOL                   : whether RAB setup is success
// + domain         : UmtsLayer3CnDomainId   : domain ID
// + connMap     : UmtsRabConnMap*        : RAB connection map
// + rabId          : char                   : Assigned RAB ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncRespToRabAssgt(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    NodeId          ueId,
    BOOL            setupSuccess,
    UmtsLayer3CnDomainId domain,
    const UmtsRabConnMap* connMap,
    char            rabId = UMTS_INVALID_RAB_ID)
{
    std::string msgBuf;

    if (setupSuccess)
    {
        msgBuf += (char) TLV_RABASSGTRES_SETUP;
        msgBuf += (char) domain;
        msgBuf.append((char*)connMap, sizeof(UmtsRabConnMap));
        msgBuf += rabId;
    }
    else
    {
        msgBuf += (char) TLV_RABASSGTRES_FAIL_SETUP;
        msgBuf += (char) domain;
        msgBuf.append((char*)connMap, sizeof(UmtsRabConnMap));
    }

    Message* ranapMsg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        ranapMsg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(ranapMsg),
           msgBuf.data(),
           msgBuf.size());

    UmtsLayer3RncSendRanapMsgToSgsn(
        node,
        umtsL3,
        ranapMsg,
        ueId,
        UMTS_RANAP_MSG_TYPE_RAB_ASSIGNMENT_RESPONSE);
}

// /**
// FUNCTION   :: UmtsLayer3RncReportIuRelease
// LAYER      :: Layer 3
// PURPOSE    :: Send a IU_RELEASE or IU_RELEASE_COMPlETE to the SGSN
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId           : NodeId                 : NodeId of the UE
// + domain         : UmtsLayer3CnDomainId   : domain ID
// + reply          : BOOL                   : reply or not
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncReportIuRelease(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    NodeId          ueId,
    UmtsLayer3CnDomainId domain,
    BOOL            reply)
{
    UmtsRanapMessageType msgType;
    if (reply)
    {
        msgType = UMTS_RANAP_MSG_TYPE_IU_RELEASE_COMPLETE;
    }
    else
    {
        msgType = UMTS_RANAP_MSG_TYPE_IU_RELEASE_REQUEST;
    }

    Message* ranapMsg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        ranapMsg,
                        sizeof(char),
                        TRACE_UMTS_LAYER3);
    *MESSAGE_ReturnPacket(ranapMsg) = (char)domain;

    UmtsLayer3RncSendRanapMsgToSgsn(
        node,
        umtsL3,
        ranapMsg,
        ueId,
        msgType);
}

// /**
// FUNCTION   :: UmtsLayer3RncChooseRbParaUponQoS
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Configure RB parameters based on QoS and domain information
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : Ue Data at RNC
// + ulRabPara : const UmtsRABServiceInfo* : UL RAB QoS requirement param
// + dlRabPara : const UmtsRABServiceInfo* : DL RAB QoS requirement param
// + domain    : UmtsLayer3CnDomainId   : Domain ID
// + rlcType   : UmtsRlcEntityType&     : RLC type
// + ueRlcConfig : UmtsRlcConfig&       : RLC config for UE
// + nodeBRlcConfig : UmtsRlcConfig&    : RLC cofnig for nodeB
// + ulTransFormat : UmtsTransportFormat& : UL transport format 
// + dlTransFormat : UmtsTransportFormat& : DL transport format
// RETURN     :: BOOL : Qos Support or not
// **/
BOOL UmtsLayer3RncChooseRbParaUponQoS(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsRrcUeDataInRnc* ueRrc,
        const UmtsRABServiceInfo* ulRabPara,
        const UmtsRABServiceInfo* dlRabPara,
        UmtsLayer3CnDomainId domain,
        UmtsRlcEntityType& rlcType,
        UmtsRlcConfig&     ueRlcConfig,
        UmtsRlcConfig&     nodeBRlcConfig,
        unsigned char&     ulLogPriority,
        unsigned char&     dlLogPriority,
        UmtsTransportFormat& ulTransFormat,
        UmtsTransportFormat& dlTransFormat)
{
    ulLogPriority = (unsigned char)ulRabPara->trafficClass;
    dlLogPriority = (unsigned char)dlRabPara->trafficClass;
    BOOL formatSupport = TRUE;
    signed char slotFormatIndex = -1;

    // Use Signal RB tranport format for traffic 
    // with no bandwidth requirement
    if (domain == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        ulTransFormat.codingType = UMTS_CONV_3;
        ulTransFormat.crcSize = UMTS_CRC_16;
        ulTransFormat.modType = UMTS_MOD_QPSK;
        ulTransFormat.transBlkSize = 248;
        ulTransFormat.TTI = 2*UMTS_TRANSPORT_DCH_DEFAULT_TTI /
                                       UMTS_SLOT_DURATION_384;

        ulTransFormat.transBlkSetSize = 248;
        ulTransFormat.trChType = UMTS_TRANSPORT_DCH;
        ulLogPriority = (unsigned char)(UMTS_QOS_CLASS_CONVERSATIONAL);

    }
    else if (ulRabPara->maxBitRate == 0.0)
    {
        if (ulRabPara->trafficClass > UMTS_QOS_CLASS_STREAMING)
        {
            // set non default vlue
            ulTransFormat.codingType = UMTS_CONV_3;
            ulTransFormat.crcSize = UMTS_CRC_16;
            ulTransFormat.modType = UMTS_MOD_QPSK;
            ulTransFormat.transBlkSize = 144;
            ulTransFormat.TTI = UMTS_TRANSPORT_DCH_DEFAULT_TTI /
                                           UMTS_SLOT_DURATION_384;

            ulTransFormat.transBlkSetSize = 144; 

            // for ACK only
            ulTransFormat.trChType = UMTS_TRANSPORT_ACKCH;

            // give acknowledgement high priority
            ulLogPriority = (unsigned char)(ulRabPara->trafficClass);
        }
        else
        {
            memcpy((void*)&ulTransFormat,
                   (void*)&umtsDefaultSigDchTransFormat_3400,
                   sizeof(UmtsTransportFormat));
        }
    }
    else if (!UmtsSelectDefaultTransportFormat(
        UMTS_TRANSPORT_DCH,
        &ulTransFormat,
        ulRabPara))
    {
        if (ulRabPara->trafficClass > UMTS_QOS_CLASS_STREAMING)
        {
            // set non default vlue
            ulTransFormat.codingType = UMTS_CONV_3;
            ulTransFormat.crcSize = UMTS_CRC_16;
            ulTransFormat.modType = UMTS_MOD_QPSK;
            ulTransFormat.transBlkSize = 96;
            ulTransFormat.TTI = UMTS_TRANSPORT_DCH_DEFAULT_TTI /
                                           UMTS_SLOT_DURATION_384;

            ulTransFormat.transBlkSetSize = (UInt64)
                ceil((double)ulRabPara->maxBitRate *
                UMTS_TRANSPORT_DCH_DEFAULT_TTI / SECOND /
                ulTransFormat.transBlkSize) *
                ulTransFormat.transBlkSize;
            ulTransFormat.trChType = UMTS_TRANSPORT_DCH;
        }
        else
        {
            formatSupport = FALSE;
            if (ulRabPara->trafficClass == UMTS_QOS_CLASS_STREAMING)
            {
                ERROR_ReportWarning(
                    " Too high rate or large "
                    "packet size for Streaming Flow\n"
                    " Max Rate         Packet Size (w/ IP header) \n"
                    " 14.4kbps           576 bit \n"
                    " 28.8kbps           576 bit \n"
                    " 57.6kbps           576 bit \n"
                    " 115.2kbps          576 bit \n");
            }
            else if (ulRabPara->trafficClass == UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ERROR_ReportWarning(
                    " Too high rate or large packet "
                    "size for Conversational Flow\n"
                    " Max Rate        Packet Size (w/ IP header) \n"
                    " 28kbps           576 bit \n"
                    " 32kbps           640 bit \n"
                    " 64kbps           640 bit \n"
                    " 128kbps          640 bit \n");
            }
            return formatSupport;
        }
    }

    if (domain == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        dlTransFormat.codingType = UMTS_CONV_3;
        dlTransFormat.crcSize = UMTS_CRC_16;
        dlTransFormat.modType = UMTS_MOD_QPSK;
        dlTransFormat.transBlkSize = 248;
        dlTransFormat.TTI = 2*UMTS_TRANSPORT_DCH_DEFAULT_TTI /
                                       UMTS_SLOT_DURATION_384;

        dlTransFormat.transBlkSetSize = 248;
        dlTransFormat.trChType = UMTS_TRANSPORT_DCH;
        dlLogPriority = (unsigned char)UMTS_QOS_CLASS_CONVERSATIONAL;

    }
    else if (dlRabPara->maxBitRate == 0.0)
    {
        memcpy((void*)&dlTransFormat,
               (void*)&umtsDefaultSigDchTransFormat_3400,
               sizeof(UmtsTransportFormat));
    }
    else if (domain != UMTS_LAYER3_CN_DOMAIN_CS &&
             ueRrc->hsdpaEnabled &&
             ((dlRabPara->maxBitRate * 4 / 3) <=
             (unsigned)(UmtsLayer3GetDlPhChDataBitRate(
                UMTS_SF_16,
                &slotFormatIndex,
                UMTS_PHYSICAL_HSPDSCH) * 5)))
    {
        // assume CS cannot use HSDPA
        // 3/4 turbo code
        // if UE support HSDPA and the service
        // rate is acceptable, using HSDPA
        // assume nodeB support HSDPA t this time
        // if not, it will removed later
        UmtsSelectDefaultTransportFormat(
            UMTS_TRANSPORT_HSDSCH,
            &ulTransFormat,
            NULL);

        UmtsSelectDefaultTransportFormat(
            UMTS_TRANSPORT_HSDSCH,
            &dlTransFormat,
            NULL);

        ulTransFormat.trChType = UMTS_TRANSPORT_HSDSCH;
        dlTransFormat.trChType = UMTS_TRANSPORT_HSDSCH;
    }
    else if (!UmtsSelectDefaultTransportFormat(
                UMTS_TRANSPORT_DCH,
                &dlTransFormat,
                dlRabPara))
    {


        if (dlRabPara->trafficClass > UMTS_QOS_CLASS_STREAMING)
        {
            // set non default vlue
            dlTransFormat.codingType = UMTS_CONV_3;
            dlTransFormat.crcSize = UMTS_CRC_16;
            dlTransFormat.modType = UMTS_MOD_QPSK;
            dlTransFormat.transBlkSize = 144;
            dlTransFormat.TTI = UMTS_TRANSPORT_DCH_DEFAULT_TTI /
                                           UMTS_SLOT_DURATION_384;

            dlTransFormat.transBlkSetSize = (UInt64)
                ceil((double)dlRabPara->maxBitRate *
                UMTS_TRANSPORT_DCH_DEFAULT_TTI / SECOND /
                dlTransFormat.transBlkSize) *
                dlTransFormat.transBlkSize;
            dlTransFormat.trChType = UMTS_TRANSPORT_DCH;

        }
        else
        {
            formatSupport = FALSE;

            if (ulRabPara->trafficClass == UMTS_QOS_CLASS_STREAMING)
            {
                ERROR_ReportWarning(
                    " Too high rate or large packet "
                    "size for Streaming Flow\n"
                    " Max Rate         Packet Size (w/ IP header) \n"
                    " 14.4kbps           576 bit \n"
                    " 28.8kbps           576 bit \n"
                    " 57.6kbps           576 bit \n"
                    " 115.2kbps          576 bit \n");
            }
            else if (ulRabPara->trafficClass == 
                UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ERROR_ReportWarning(
                    " Too high rate or large packet size "
                    "for Conversational Flow\n"
                    " Max Rate        Packet Size (w/ IP header) \n"
                    " 28kbps           576 bit \n"
                    " 32kbps           640 bit \n"
                    " 64kbps           640 bit \n"
                    " 128kbps          640 bit \n");
            }
            return formatSupport;

        }
    }

    if (domain == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        rlcType = UMTS_RLC_ENTITY_TM;
        ueRlcConfig.tmConfig.segIndi = FALSE;
        nodeBRlcConfig.tmConfig.segIndi = FALSE;
    }
    else
    {
         const UmtsRABServiceInfo* rabPara =
             (ulRabPara->maxBitRate >= dlRabPara->maxBitRate) ?
             ulRabPara : dlRabPara;
        if (rabPara->trafficClass < UMTS_QOS_CLASS_INTERACTIVE_3)
        {
            rlcType = UMTS_RLC_ENTITY_UM;
            ueRlcConfig.umConfig.maxUlPduSize = 
                ulTransFormat.transBlkSize / 8;
            nodeBRlcConfig.umConfig.maxUlPduSize = 
                ulTransFormat.transBlkSize / 8;
            if (dlTransFormat.transBlkSize / 8 <= 
                UMTS_RLC_UM_LI_PDUTHRESH)
            {
                ueRlcConfig.umConfig.dlLiLen = 7;
                nodeBRlcConfig.umConfig.dlLiLen = 7;
            }
            else
            {
                ueRlcConfig.umConfig.dlLiLen = 15;
                nodeBRlcConfig.umConfig.dlLiLen = 15;
            }
            ueRlcConfig.umConfig.alterEbit = FALSE;
            nodeBRlcConfig.umConfig.alterEbit = FALSE;
        }
        else
        {
            rlcType = UMTS_RLC_ENTITY_AM;
            UmtsRlcAmConfig& ueConfig = ueRlcConfig.amConfig;
            ueConfig.sndPduSize = ulTransFormat.transBlkSize / 8;
            ueConfig.rcvPduSize = dlTransFormat.transBlkSize / 8;
            ueConfig.sndWnd = 2047; //UMTS_RLC_DEF_WINSIZE;   
            ueConfig.rcvWnd = 2047; //UMTS_RLC_DEF_WINSIZE; 
            ueConfig.maxDat = UMTS_RLC_DEF_MAXDAT;          //TODO
            ueConfig.maxRst = UMTS_RLC_DEF_MAXRST;          //TODO
            ueConfig.rstTimerVal = UMTS_RLC_DEF_RSTTIMER_VAL;   //TODO

            UmtsRlcAmConfig& nodeBConfig = nodeBRlcConfig.amConfig;
            nodeBConfig.sndPduSize = dlTransFormat.transBlkSize / 8;
            nodeBConfig.rcvPduSize = ulTransFormat.transBlkSize / 8;
            nodeBConfig.sndWnd = 2047; //UMTS_RLC_DEF_WINSIZE * 1000;        
            nodeBConfig.rcvWnd = 2047; //UMTS_RLC_DEF_WINSIZE * 1000;        
            nodeBConfig.maxDat = UMTS_RLC_DEF_MAXDAT;           //TODO
            nodeBConfig.maxRst = UMTS_RLC_DEF_MAXRST;           //TODO
            nodeBConfig.rstTimerVal = UMTS_RLC_DEF_RSTTIMER_VAL;    //TODO
        }
    }

    return formatSupport;
}

// /**
// FUNCTION    :: UmtsLayer3RncCheckUeOngoingRabProc
// DESCRIPTION :: Check if there is ongoing RAB est/release process
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc* : UeInfo at RRC
// **/
static
BOOL UmtsLayer3RncCheckUeOngoingRabProc(
         Node* node,
         UmtsLayer3Data *umtsL3,
         UmtsRrcUeDataInRnc* ueRrc)
{
    BOOL retVal = FALSE;
    for (int i = 0; i < UMTS_MAX_RAB_SETUP; i++)
    {
        if (!ueRrc->rabInfos[i])
            continue;
        if (ueRrc->rabInfos[i]->state != UMTS_RAB_STATE_ESTED)
        {
            retVal = TRUE;
            break;
        }
    }
    return retVal;
}

// /**
// FUNCTION   :: UmtsLayer3RncInitCellLookup
// LAYER      :: Layer 3
// PURPOSE    :: Initiate Cell lookup located outside current RNC
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to the layer3 data
// + cellId    : UInt32 : CELL ID of this NodeB to be searched (not nodebId)
// RETURN     :: void   : NULL
// **/
static
void UmtsLayer3RncInitCellLookup(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UInt32  cellId)
{
    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, sizeof(cellId), TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg), &cellId, sizeof(cellId));
    UmtsLayer3RncSendRanapMsgToSgsn(
        node,
        umtsL3,
        msg,
        0,
        UMTS_RANAP_MSG_TYPE_CELL_LOOKUP);
}

//--------------------------------------------------------------------------
// Build and send msg FUNCTIONs
//--------------------------------------------------------------------------
//
// Send RR Msg
//
// /**
// FUNCTION   :: UmtsLayer3RncBuildRrcConnSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Build RRC_CONNECTION_SETUP message with specificaion mode
//               set as "Preconfiguration"
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + transctId : char           : transaction id
// + initUeId  : UInt32         : Initial UE identifier
// + stateIndi : UmtsRrcSubState: RRC state indicator
// + preConfigMode : UmtsRrcConnSetupPreConfigMode : Preconfiguration mode
// + ueScCode  : UInt32         : UE primary scrambling code
// + dlDpdchInfo: const UmtsRcvPhChInfo*  : downlink DPDCH information
// RETURN     :: Message* : Point to the message containing DSC-REQ PDU
// **/
static
Message* UmtsLayer3RncBuildRrcConnSetup(
             Node* node,
             char transctId,
             NodeId initUeId,
             UmtsRrcSubState stateIndi,
             UmtsRrcConnSetupPreConfigMode preConfigMode,
             UInt32 ueScCode,
             const UmtsRcvPhChInfo* dlDpdchInfo)
{
    char buffer[50];
    int index = 0;
    memcpy(&buffer[index], &initUeId, sizeof(initUeId));
    index += sizeof(initUeId);

    buffer[index++] = (char)stateIndi;
    buffer[index++] = (char)(UMTS_RRCCONNSETUP_SPECMODE_PRECONFIG);
    buffer[index++] = (char)(preConfigMode);
    memcpy(&buffer[index], &ueScCode, sizeof(ueScCode));
    index += sizeof(ueScCode);
    memcpy(&buffer[index], dlDpdchInfo, sizeof(UmtsRcvPhChInfo));
    index += sizeof(UmtsRcvPhChInfo);

    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, index, TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg), buffer, index);

    if (DEBUG_RR && 0)
    {
        printf("Node: %u build a RRC_CONNECTION_SETUP message\n ",
                node->nodeId);
#if 0
        printf("packet: \t\t");
        for (int i = 0; i < msg->packetSize; i++)
            printf("%d", (char) msg->packet[i]);
        printf("\n");
#endif
    }
    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        transctId,
        (char)UMTS_RR_MSG_TYPE_RRC_CONNECTION_SETUP);

    return msg;
}

// /**
// FUNCTION   :: UmtsLayer3RncBuildRrcConnRej
// LAYER      :: Layer3 RRC
// PURPOSE    :: Build RRC_CONNECTION_REJECT message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + transctId : char         : transaction id
// + initUeId  : UInt32       : Initial UE identifier
// + waitTime  : unsigned int : wait time
// RETURN     :: Message* : Point to the message containing DSC-REQ PDU
// **/
static
Message* UmtsLayer3RncBuildRrcConnRej(
             Node* node,
             char transctId,
             UInt32 initUeId,
             clocktype waitTime)
{
    unsigned char buffer[20];
    int index = 0;
    memcpy(&buffer[index], &initUeId, sizeof(initUeId));
    index += sizeof(initUeId);

    //buffer[index ++] = (unsigned char)(waitTime);
    memcpy(&buffer[index], &waitTime, sizeof(waitTime));
    index += sizeof(waitTime);
    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, index, TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg), buffer, index);

    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        transctId,
        (unsigned char)UMTS_RR_MSG_TYPE_RRC_CONNECTION_REJECT);

    return msg;
}


// /**
// FUNCTION   :: UmtsLayer3RncSendRrcConnSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RRC CONNECTION SETUP message to UE
// PARAMETERS ::
// + node       : Node*                     : Pointer to node.
// + umtsL3     : UmtsLayer3Data *          : Pointer to UMTS Layer3 data
// + ueId       : NodeId                    : UE ID the message to be sent
// + dlDpdchInfo: const UmtsRcvPhChInfo*  : downlink DPDCH information
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendRrcConnSetup(
    Node* node,
    UmtsLayer3Data *umtsL3,
    NodeId ueId,
    const UmtsRcvPhChInfo* dlDpdchInfo)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    ERROR_Assert(ueRrc, "UmtsLayer3RncSendRrcConnSetup: "
        "cannot not find UE RRC data in the RNC.");

    char transctId = 0;
    std::map<UmtsRRMessageType, unsigned char>::iterator pos =
        ueRrc->transactions->find(UMTS_RR_MSG_TYPE_RRC_CONNECTION_SETUP);
    if (pos == ueRrc->transactions->end())
    {
        (*ueRrc->transactions)[UMTS_RR_MSG_TYPE_RRC_CONNECTION_SETUP]
                = transctId;
    }
    else
    {
        transctId = pos->second++;
    }

    // update stat
    rncL3->stat.numRrcConnSetupSent ++;

    Message* setupMsg = UmtsLayer3RncBuildRrcConnSetup(
                            node,
                            transctId,
                            ueRrc->ueId,
                            UMTS_RRC_SUBSTATE_CELL_DCH,
                            UMTS_RRCCONNSETUP_PRECONFIGMODE_DEFAULT,
                            ueRrc->primScCode,
                            dlDpdchInfo);
    if (DEBUG_RR && 0)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, RNC: %d sent a RRC_CONNECTION_SETUP to UE: %d\n",
            timeStr, node->nodeId, ueRrc->ueId);
        printf("packet: \t\t");
        for (int i = 0; i < setupMsg->packetSize; i++)
            printf("%d", (char) setupMsg->packet[i]);
        printf("\n");
        printf("allocate dl ch code %d sf %d\n",
            dlDpdchInfo->chCode,dlDpdchInfo->sfFactor);
    }

    UmtsCellCacheInfo* cellCache = rncL3->cellCacheMap->find(
                                        ueRrc->primCellId)->second;
    UmtsLayer3RncSendRrcMessageThroughNodeb(
        node,
        umtsL3,
        setupMsg,
        ueRrc->ueId,
        UMTS_CCCH_RB_ID,
        cellCache->nodebId);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRrcConnRej
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RRC_CONNECTION_REJECT message to the UE
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transctId : char                   : transaction id
// + ueId      : NodeId                 : UE identifier
// + nodebId   : NodeId                 : The nodeb Id the msg to be sent to
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendRrcConnRej(
             Node* node,
             UmtsLayer3Data* umtsL3,
             char transctId,
             NodeId ueId,
             NodeId nodebId)
{
    Message* rejMsg = UmtsLayer3RncBuildRrcConnRej(
                        node,
                        transctId,
                        ueId,
                        UMTS_LAYER3_TIMER_RRCCONNWAITTIME_VAL);
    //send RRC_CONNECTION_REJECT through signalling RB0
    UmtsLayer3RncSendRrcMessageThroughNodeb(node,
                                            umtsL3,
                                            rejMsg,
                                            ueId,
                                            UMTS_CCCH_RB_ID,
                                            nodebId);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRrcConnRelease
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RRC_CONNECTION_RELEASE message to the UE
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transctId : char                   : transaction id
// + ueRrc     : UmtsRrcUeDataInRnc*    : UE RRC data at RNC
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendRrcConnRelease(
             Node* node,
             UmtsLayer3Data* umtsL3,
             UmtsRrcUeDataInRnc* ueRrc)
{
    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, sizeof(NodeId), TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg), &(ueRrc->ueId), sizeof(NodeId));

    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_RRC_CONNECTION_RELEASE);

    UmtsLayer3RncSendPktToUeOnDch(
        node,
        umtsL3,
        msg,
        ueRrc->ueId,
        UMTS_SRB1_ID);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRbSetupToUe
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RB SETUP message to UE
// PARAMETERS ::
// + node       : Node*                     : Pointer to node.
// + umtsL3     : UmtsLayer3Data *          : Pointer to UMTS Layer3 data
// + ueRrc      : UmtsRrcUeDataInRnc*       : UE RRC data at RNC
// + pendingRab : UmtsRrcPendingRabInfo*    : Rab info
// + rabId      : char                      : Rab ID
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendRbSetupToUe(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    UmtsRrcPendingRabInfo* pendingRab,
    char rabId)
{
    if (ueRrc->rrcRelReq.requested == TRUE)
    {
        return;
    }

    std::string toUeMsgBuf;
    toUeMsgBuf.reserve(150);
    toUeMsgBuf.append((char*)&pendingRab->uePara,
                      sizeof(pendingRab->uePara));

    toUeMsgBuf += (char)pendingRab->nodebResList->size();

    // update Bw Req
    ueRrc->curBwReq +=
        ueRrc->rabInfos[(int)rabId]->ulRabPara.maxBitRate;

    std::list<UmtsRrcNodebResInfo*>::iterator iter;
    for (iter = pendingRab->nodebResList->begin();
         iter != pendingRab->nodebResList->end();
         ++iter)
    {
        toUeMsgBuf.append((char*) (*iter),
                          sizeof(UmtsRrcNodebResInfo));
    }

    Message* toUeMsg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        toUeMsg,
                        (int)toUeMsgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(toUeMsg),
           toUeMsgBuf.data(),
           toUeMsgBuf.size());
    UmtsLayer3AddHeader(
        node,
        toUeMsg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_RADIO_BEARER_SETUP);

    UmtsLayer3RncSendPktToUeOnDch(
        node,
        umtsL3,
        toUeMsg,
        ueRrc->ueId,
        UMTS_SRB2_ID);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRbReleaseToUe
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RB RELEASE message to UE
// PARAMETERS ::
// + node       : Node*                     : Pointer to node.
// + umtsL3     : UmtsLayer3Data *          : Pointer to UMTS Layer3 data
// + ueRrc      : UmtsRrcUeDataInRnc*       : UE RRC data at RNC
// + rabId      : char                      : RAB Id
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendRbReleaseToUe(
    Node* node,
    UmtsLayer3Data* umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    char rabId)
{
    UmtsRrcPendingRabInfo* pendingRab = ueRrc->pendingRab[(int)rabId];
    UmtsSpreadFactor sf = UMTS_SF_4;

    // update Bw Req
    ueRrc->curBwReq -= ueRrc->rabInfos[(int)rabId]->ulRabPara.maxBitRate;

    //  assume rate 1/3 is  used
    if (UmtsGetBestUlSpreadFactorForRate(
            ueRrc->curBwReq * 3,
            &sf))
    {
        pendingRab->uePara.sfFactor = sf;
        ueRrc->curSf = sf;
        pendingRab->uePara.numUlDPdch = (unsigned char)
                ceil((double)(ueRrc->curBwReq) * 3 / 960000); 
                                 // SF4 960000bps per channel
    }
    else
    {
        // TODO: admission control
        pendingRab->uePara.sfFactor = sf;
        pendingRab->uePara.numUlDPdch = (unsigned char)
                ceil((double)(ueRrc->curBwReq) * 3 / 960000); 
                                 // SF4 960000bps per channel
        if (pendingRab->uePara.numUlDPdch > 6)
        {
            pendingRab->uePara.numUlDPdch = 6;
        }
    }

    // to see if HS-DPCCH need to be released

    std::string toUeMsgBuf;
    toUeMsgBuf.reserve(150);
    toUeMsgBuf.append((char*)&(pendingRab->uePara),
                      sizeof(pendingRab->uePara));

    toUeMsgBuf += (char)pendingRab->nodebResList->size();


    std::list<UmtsRrcNodebResInfo*>::iterator iter;
    for (iter = pendingRab->nodebResList->begin();
         iter != pendingRab->nodebResList->end();
         ++iter)
    {
        toUeMsgBuf.append((char*) (*iter),
                          sizeof(UmtsRrcNodebResInfo));
    }

    Message* toUeMsg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        toUeMsg,
                        (int)toUeMsgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(toUeMsg),
           toUeMsgBuf.data(),
           toUeMsgBuf.size());
    UmtsLayer3AddHeader(
        node,
        toUeMsg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_RADIO_BEARER_RELEASE);

    UmtsLayer3RncSendPktToUeOnDch(
        node,
        umtsL3,
        toUeMsg,
        ueRrc->ueId,
        UMTS_SRB2_ID);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendActvSetUp
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send ACTIVE SET UPDATE message to UE
// PARAMETERS ::
// + node       : Node*                     : Pointer to node.
// + umtsL3     : UmtsLayer3Data *          : Pointer to UMTS Layer3 data
// + ueId       : NodeId                    : UE ID the message to be sent
// + addCell    : BOOL                      : add cell or not
// + cellInfo   : const char*               : cell info
// + cellIfnoSize : int                     : size of the cell info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncSendActvSetUp(
    Node* node,
    UmtsLayer3Data *umtsL3,
    NodeId ueId,
    BOOL   addCell,
    const char*  cellInfo,
    int    cellInfoSize)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    ERROR_Assert(ueRrc, "cannot find the UE RRC data in the RNC.");

    int index = 0;
    int msgSize = sizeof(char) + cellInfoSize;
    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, msgSize, TRACE_UMTS_LAYER3);
    char* packet = MESSAGE_ReturnPacket(msg);
    packet[index++] = (char)addCell;
    memcpy(&packet[index], cellInfo, cellInfoSize);

    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        0,
        (unsigned char)UMTS_RR_MSG_TYPE_ACTIVE_SET_UPDATE);

    if (DEBUG_RR && DEBUG_UE_ID == ueId )
    {
        printf("Rnc: %u sends a ACTIVE SET UPDATE message\n ",
                node->nodeId);
        UmtsPrintMessage(std::cout, msg);
    }

    UmtsLayer3RncSendPktToUeOnDch(
        node,
        umtsL3,
        msg,
        ueId,
        UMTS_SRB2_ID);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendSignalConnRel
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send SIGNAL CONNECTION RELEASE message to UE
// PARAMETERS ::
// + node       : Node*                     : Pointer to node.
// + umtsL3     : UmtsLayer3Data *          : Pointer to UMTS Layer3 data
// + ueRrc      : UmtsRrcUeDataInRnc*       : UE RRC data at RNC
// + domain     : UmtsLayer3CnDomainId      : Domain Id
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendSignalConnRel(
    Node* node,
    UmtsLayer3Data* umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    UmtsLayer3CnDomainId domain)
{
    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, sizeof(char), TRACE_UMTS_LAYER3);

    (*MESSAGE_ReturnPacket(msg)) = (char)domain;

    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_SIGNALLING_CONNECTION_RELEASE);

    UmtsLayer3RncSendPktToUeOnDch(
        node,
        umtsL3,
        msg,
        ueRrc->ueId,
        UMTS_SRB1_ID);
}


// /**
// FUNCTION   :: UmtsLayer3RncSendRrcSetupToNodeb
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RRC CONNECTION SETUP message to NodeB
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to UE RRC data in RNC
// + ueId      : NodeId                 : The UE ID associated with the
//                                        dedicated resources setup by
//                                        this message
// + srbConfig : const UmtsNodebSrbConfigPara*: Const pointer to the 
//                                       NodeB configuration parameters
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendRrcSetupToNodeb(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsRrcNodebDataInRnc*  nodebRrc,
    NodeId                  ueId,
    const UmtsNodebSrbConfigPara* srbConfig)
{
    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(UmtsNodebSrbConfigPara),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           srbConfig,
           sizeof(UmtsNodebSrbConfigPara));

    UmtsLayer3RncSendNbapMessageToNodeb(
        node,
        umtsL3,
        msg,
        UMTS_NBAP_MSG_TYPE_RRC_SETUP_REQUEST,
        0,
        nodebRrc->nodebId,
        ueId);

    if (DEBUG_NBAP && 0)
    {
        std::cout << "Rnc: " << node->nodeId
                  << " sent an RRC_SETUP_REQUEST message to NodeB: "
                  << nodebRrc->nodebId << " with transaction ID: "
                  << "0" << " and UE ID: " << ueId << std::endl
                  << "Allocated channels for SRB 1-3: " << std::endl
                  << "Downlink DCCH ID:  "
                  << "SRB1: " << (int)srbConfig->dlDcchId[0] << "\t"
                  << "SRB2: " << (int)srbConfig->dlDcchId[1] <<  "\t"
                  << "SRB3: " << (int)srbConfig->dlDcchId[2] <<  std::endl
                  << "Downlink DCH ID:   "
                  << "SRB1-3: " << (int)srbConfig->dlDchId << "\t"
                  << "Downlink DPDCH: "
                  << "ID: " << (int)srbConfig->dlDpdchId << "\t"
                  << "SFactor: " << srbConfig->sfFactor <<  "\t"
                  << "OVSF code: " << srbConfig->chCode <<  std::endl;
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRbSetupToNodeb
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RB setup to the Nodeb
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : NodeB data in RNC
// + nodebPara : const UmtsNodebRbSetupPara& : Nodeb RB setup parameters
// RETURN     :: void
// **/
static
void UmtsLayer3RncSendRbSetupToNodeb(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    const UmtsNodebRbSetupPara& nodebPara)
{
    NodeId ueId = nodebPara.ueId;
    std::string toNodebMsgBuf;
    toNodebMsgBuf.reserve(sizeof(nodebPara));
    toNodebMsgBuf.append((char*)&nodebPara, sizeof(nodebPara));

    Message* toNodebMsg = MESSAGE_Alloc(
                              node, NETWORK_LAYER, 
                              MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        toNodebMsg,
                        (int)toNodebMsgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(toNodebMsg),
           toNodebMsgBuf.data(),
           toNodebMsgBuf.size());

    UmtsLayer3RncSendNbapMessageToNodeb(
        node,
        umtsL3,
        toNodebMsg,
        UMTS_NBAP_MSG_TYPE_RADIO_BEARER_SETUP_REQUEST,
        0,
        nodebRrc->nodebId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRbReleaseToNodeb
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RB setup to the Nodeb
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to the NodeB RRC data
// + nodebPara : const UmtsNodebRbSetupPara& : Nodeb RB setup parameters
// RETURN     :: void
// **/
static
void UmtsLayer3RncSendRbReleaseToNodeb(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    const UmtsNodebRbSetupPara& nodebPara)
{
    NodeId ueId = nodebPara.ueId;
    std::string toNodebMsgBuf;
    toNodebMsgBuf.reserve(sizeof(nodebPara));
    toNodebMsgBuf.append((char*)&nodebPara, sizeof(nodebPara));

    Message* toNodebMsg = 
        MESSAGE_Alloc(node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        toNodebMsg,
                        (int)toNodebMsgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(toNodebMsg),
           toNodebMsgBuf.data(),
           toNodebMsgBuf.size());

    UmtsLayer3RncSendNbapMessageToNodeb(
        node,
        umtsL3,
        toNodebMsg,
        UMTS_NBAP_MSG_TYPE_RADIO_BEARER_RELEASE_REQUEST,
        0,
        nodebRrc->nodebId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3RncReleaseNodebConfigForSrbRelease
// LAYER      :: Layer3 RRC
// PURPOSE    :: Release NodeB resources for releasing SRB1-3 for a UE
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to the NodeB RRC data
// + ueId      : NodeId             : Node Id Of UE
// + nodebPara : const UmtsNodebRbSetupPara& : Nodeb RB setup parameters
// RETURN     :: void
// **/
static
void UmtsLayer3RncReleaseNodebConfigForSrbRelease(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsRrcNodebDataInRnc*  nodebRrc,
    NodeId                  ueId,
    UmtsNodebSrbConfigPara& nodebSrbConfig)
{
    std::list<UmtsRbMapping*>::iterator pos;
    for (int i = 0; i < 3; i++)
    {
        pos = std::find_if(nodebRrc->rbMappingInfo->begin(),
                           nodebRrc->rbMappingInfo->end(),
                           UmtsFindItemByUeIdAndRbId
                                <UmtsRbMapping>(ueId, (char)(i+1)));
        ERROR_Assert(pos != nodebRrc->rbMappingInfo->end(),
            "UmtsLayer3RncReleaseNodebConfigForSrbRelease: "
            "cannot find stored signal RB mapping information.");

        nodebRrc->dlDcchBitSet->reset((*pos)->logChId);
        nodebRrc->dlDchBitSet->reset((*pos)->trChId);
        nodebSrbConfig.dlDcchId[i] = (*pos)->logChId;
        if (i == 0)
        {
            nodebSrbConfig.dlDchId = (*pos)->trChId;
        }
        MEM_free(*pos);
        nodebRrc->rbMappingInfo->erase(pos);
    }

    std::list<UmtsDlDchInfo*>::iterator dlDchPos;
    dlDchPos = std::find_if(
                nodebRrc->dlDchInfo->begin(),
                nodebRrc->dlDchInfo->end(),
                UmtsFindItemByChId<UmtsDlDchInfo>(
                                nodebSrbConfig.dlDchId));
    ERROR_Assert(dlDchPos != nodebRrc->dlDchInfo->end(),
        "cannot find stored signal downlink DCH info.");
    for (int i = 0; i < 3; i++)
        UmtsClearBit<UInt32>((*dlDchPos)->rbBitSet, i+1);
    MEM_free((*dlDchPos));
    nodebRrc->dlDchInfo->erase(dlDchPos);

    std::list<UmtsRbPhChMapping*>::iterator phMapPos;
    phMapPos = std::find_if(nodebRrc->rbPhChMappingList->begin(),
                            nodebRrc->rbPhChMappingList->end(),
                            UmtsFindItemByUeIdAndRbId
                                <UmtsRbPhChMapping>
                                (ueId, UMTS_SRB1_ID));
    nodebSrbConfig.dlDpdchId =
        (unsigned char)((*phMapPos)->phChList->front()->chId);
    delete *phMapPos;
    nodebRrc->rbPhChMappingList->erase(phMapPos);

    std::list<UmtsDlDpdchInfo*>* dlDpdchInfo = nodebRrc->dlDpdchInfo;
    std::list<UmtsDlDpdchInfo*>::iterator dlDpdchPos;
    dlDpdchPos = std::find_if(
                dlDpdchInfo->begin(),
                dlDpdchInfo->end(),
                UmtsFindItemByChId<UmtsDlDpdchInfo>
                                  (nodebSrbConfig.dlDpdchId));
    ERROR_Assert(dlDpdchPos != dlDpdchInfo->end(),
        "cannot find stored signal downlink DCH info.");
    nodebSrbConfig.sfFactor = (*dlDpdchPos)->sfFactor;
    nodebSrbConfig.chCode = (*dlDpdchPos)->dlChCode;
    for (int i = 0; i < 3; i++)
        UmtsClearBit<UInt32>((*dlDpdchPos)->rbBitSet, i+1);
    MEM_free((*dlDpdchPos));
    dlDpdchInfo->erase(dlDpdchPos);

    // update UL BW allocation
    nodebRrc->ulResUtil += nodebRrc->totalUlBwAlloc *
                           (node->getNodeTime() -
                           nodebRrc->lastUlResUpdateTime) / SECOND;
    nodebRrc->totalUlBwAlloc -= 3400;
    nodebRrc->lastUlResUpdateTime = node->getNodeTime();


    // update Dl BW Allocation
    nodebRrc->dlResUtil += nodebRrc->totalDlBwAlloc *
                           (node->getNodeTime() -
                           nodebRrc->lastDlResUpdateTime) / SECOND;
    nodebRrc->totalDlBwAlloc -= 3400;
    nodebRrc->lastDlResUpdateTime = node->getNodeTime();
}

// /**
// FUNCTION   :: UmtsLayer3RncReleaseUeConfigForSrbRelease
// LAYER      :: Layer3 RRC
// PURPOSE    :: Release NodeB resources for releasing SRB1-3 for a UE
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc* : UE RRC data
// RETURN     :: void
// **/
static
void UmtsLayer3RncReleaseUeConfigForSrbRelease(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcUeDataInRnc* ueRrc)
{
    unsigned char ulDchId = 0;
    std::list<UmtsRbMapping*>::iterator pos;
    for (int i = 0; i < 3; i++)
    {
        pos = std::find_if(ueRrc->rbMappingInfo->begin(),
                           ueRrc->rbMappingInfo->end(),
                           UmtsFindItemByRbId<UmtsRbMapping>((char)(i+1)));
        ERROR_Assert(pos != ueRrc->rbMappingInfo->end(),
            "UmtsLayer3RncReleaseUeConfigForSrbRelease: "
            "cannot find stored signal RB mapping information.");
        ueRrc->ulDcchBitSet->reset((*pos)->logChId);
        if (i == 0)
        {
            ulDchId = (*pos)->trChId;
        }
        MEM_free(*pos);
        ueRrc->rbMappingInfo->erase(pos);
    }

    UmtsUlDchInfo* ulDch = ueRrc->ulDchInfo[ulDchId];
    for (int i = 0; i < 3; i++)
        UmtsClearBit<UInt32>(ulDch->rbBitSet, i+1);
#if 0
    ERROR_Assert(ulDch->rbBitSet == 0,
        "incorrect signal RB configuration.");
#endif // 0
    MEM_free(ulDch);
    ueRrc->ulDchInfo[ulDchId] = NULL ;

    std::list<UmtsRbPhChMapping*>::iterator phMapPos;
    phMapPos = std::find_if(ueRrc->rbPhChMappingList->begin(),
                            ueRrc->rbPhChMappingList->end(),
                            UmtsFindItemByRbId
                                <UmtsRbPhChMapping>(UMTS_SRB1_ID));
    unsigned char ulDpdchId =
        (unsigned char)((*phMapPos)->phChList->front()->chId);
    delete *phMapPos;
    ueRrc->rbPhChMappingList->erase(phMapPos);

    UmtsUlDpdchInfo* ulDpdchInfo = ueRrc->ulDpdchInfo[ulDpdchId];
    for (int i = 0; i < 3; i++)
        UmtsClearBit<UInt32>(ulDpdchInfo->rbBitSet, i+1);
    MEM_free(ulDpdchInfo);
    ueRrc->ulDpdchInfo[ulDpdchId] = NULL ;
}

// /**
// FUNCTION   :: UmtsLayer3RncCheckNodebForSrbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Check NodeB resources for setting SRB1-3 for a UE
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to the NodeB RRC data
// + nodebPara : const UmtsNodebRbSetupPara& : Nodeb RB setup parameters
// RETURN     :: BOOL               : TRUE if resources can be allocated
// **/
static
BOOL UmtsLayer3RncCheckNodebForSrbSetup(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    UmtsNodebSrbConfigPara& nodebSrbConfig)
{
    int dlDcchId[3];
    int dlDchId;
    int dlDpdchId;
    for (int i = 0; i < 3; i++)
    {
        dlDcchId[i] = UmtsGetPosForNextZeroBit(
                                *(nodebRrc->dlDcchBitSet),
                                (i == 0) ? 0 : (dlDcchId[i-1] + 1));
        if (dlDcchId[i] == -1 )
        {
            return FALSE;
        }
    }
    dlDchId = UmtsGetPosForNextZeroBit(
                        *(nodebRrc->dlDchBitSet), 0);
    dlDpdchId = UmtsGetPosForNextZeroBit(
                        *(nodebRrc->dlDpdchBitSet), 0);
    if (dlDchId == -1 || dlDpdchId == -1)
    {
        return FALSE;
    }

    //Allocate channelization code
    int dataRate = 3400;        //3.4kbps signalling
    int numOvsf = 1;
    UmtsSpreadFactor sf;
    UmtsModulationType modType;
    UmtsCodingType codingType;
    unsigned int ovsfCode;

    // Uplink admission control
    if (!UmtsLayer3RncUlCallAdmissionControl(
        node, umtsL3, nodebRrc,
        dataRate, dataRate))
    {
        return FALSE;
    }

    // Find the right modulation and coding type according to the
    // error rate requirement andother QoS paras
    // By Default use QPSK and TURBO 1/3
    modType = UMTS_MOD_QPSK;
    codingType = UMTS_CONV_3;//UMTS_TURBO_3;

    UmtsLayer3RncAllocNodebOvsf(
        node,
        umtsL3,
        nodebRrc,
        dataRate,
        modType,
        codingType,
        &sf,
        &ovsfCode,
        &numOvsf,
        FALSE);

    if (!numOvsf)
    {
        return FALSE;
    }
    for (int i = 0; i < 3; i++)
    {
        nodebSrbConfig.dlDcchId[i] = (unsigned char)(dlDcchId[i]);
    }
    nodebSrbConfig.dlDchId = (unsigned char)dlDchId;
    nodebSrbConfig.dlDpdchId = (unsigned char)dlDpdchId;
    nodebSrbConfig.sfFactor = sf;
    nodebSrbConfig.chCode = ovsfCode;

    return TRUE;
}

// /**
// FUNCTION   :: UmtsLayer3RncStoreNodebConfigForSrbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Check NodeB resources for setting SRB1-3 for a UE
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to the NodeB RRC data
// + ueId      : NodeId             : Node Id Of UE
// + nodebPara : const UmtsNodebRbSetupPara& : Nodeb RB setup parameters
// RETURN     :: void
// **/
static
void UmtsLayer3RncStoreNodebConfigForSrbSetup(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    NodeId              ueId,
    const UmtsNodebSrbConfigPara& nodebSrbConfig)
{
    //Store the NodeB downlink DPDCH information
    UmtsDlDpdchInfo* dlDpdch = (UmtsDlDpdchInfo*)
                               MEM_malloc(sizeof(UmtsDlDpdchInfo));
    memset(dlDpdch, 0, sizeof(UmtsDlDpdchInfo));
    dlDpdch->chId = nodebSrbConfig.dlDpdchId;
    dlDpdch->sfFactor = nodebSrbConfig.sfFactor;
    dlDpdch->dlChCode = nodebSrbConfig.chCode;
    ERROR_Assert(nodebRrc->dlDpdchInfo->end() ==
        std::find_if(nodebRrc->dlDpdchInfo->begin(),
                     nodebRrc->dlDpdchInfo->end(),
                     UmtsFindItemByChId<UmtsDlDpdchInfo>
                        (dlDpdch->chId)),
        "UmtsLayer3RncStoreNodebConfigForSrbSetup: "
        "Downlink DPDCH information already stored.");
    nodebRrc->dlDpdchInfo->push_back(dlDpdch);
    nodebRrc->dlDpdchBitSet->set(nodebSrbConfig.dlDpdchId);

    //store the NodeB downlink DCH information
    UmtsDlDchInfo* dlDch = (UmtsDlDchInfo*)
                           MEM_malloc(sizeof(UmtsDlDchInfo));
    memset(dlDch, 0, sizeof(UmtsDlDchInfo));
    dlDch->chId = nodebSrbConfig.dlDchId;
    ERROR_Assert(nodebRrc->dlDchInfo->end() ==
        std::find_if(
            nodebRrc->dlDchInfo->begin(),
            nodebRrc->dlDchInfo->end(),
            UmtsFindItemByChId<UmtsDlDchInfo>(dlDch->chId)),
        "UmtsLayer3RncStoreNodebConfigForSrbSetup: "
        "NodeB downlink DCH information already stored.");
    nodebRrc->dlDchInfo->push_back(dlDch);
    nodebRrc->dlDchBitSet->set(nodebSrbConfig.dlDchId);

    for (int i = 1; i <= 3; i++)
    {
        UmtsSetBit<UInt32>(dlDpdch->rbBitSet, i);
        UmtsSetBit<UInt32>(dlDch->rbBitSet, i);

        //Store RB mapping configurations at UE data structure
        //Downlink
        nodebRrc->dlDcchBitSet->set(nodebSrbConfig.dlDcchId[i-1]);
        UmtsRbMapping* dlRbMapping = (UmtsRbMapping*)
                                     MEM_malloc(sizeof(UmtsRbMapping));
        dlRbMapping->ueId = ueId;
        dlRbMapping->rbId = (char)i;
        dlRbMapping->direction = UMTS_CH_DOWNLINK;
        dlRbMapping->logChType = UMTS_LOGICAL_DCCH;
        dlRbMapping->logChId = nodebSrbConfig.dlDcchId[i-1];
        dlRbMapping->logChPrio = (unsigned char)i;
        dlRbMapping->trChType = UMTS_TRANSPORT_DCH;
        dlRbMapping->trChId = nodebSrbConfig.dlDchId;
        nodebRrc->rbMappingInfo->push_back(dlRbMapping);

        //Store physical mapping info
        UmtsRbPhChMapping* rbPhMapping =
            new UmtsRbPhChMapping(ueId, (char)i);
        rbPhMapping->AddPhCh(UmtsPhChInfo(UMTS_PHYSICAL_DPDCH,
                                          nodebSrbConfig.dlDpdchId));
        nodebRrc->rbPhChMappingList->push_back(rbPhMapping);
    }

    // update the allocated UL resource in this NodeB
    nodebRrc->ulResUtil += nodebRrc->totalUlBwAlloc *
                           (node->getNodeTime() -
                           nodebRrc->lastUlResUpdateTime) / SECOND;
    nodebRrc->totalUlBwAlloc += 3400;
    if (nodebRrc->totalUlBwAlloc > nodebRrc->peakUlBwAlloc)
    {
        nodebRrc->peakUlBwAlloc = nodebRrc->totalUlBwAlloc;
    }
    nodebRrc->lastUlResUpdateTime = node->getNodeTime();

    // update resource utilization before alloc
    nodebRrc->dlResUtil += nodebRrc->totalDlBwAlloc *
                           (node->getNodeTime() -
                           nodebRrc->lastDlResUpdateTime) / SECOND;

    nodebRrc->totalDlBwAlloc += 3400;
    if (nodebRrc->totalDlBwAlloc > nodebRrc->peakDlBwAlloc)
    {
        nodebRrc->peakDlBwAlloc = nodebRrc->totalDlBwAlloc;
    }
    nodebRrc->lastDlResUpdateTime = node->getNodeTime();
}

// /**
// FUNCTION   :: UmtsLayer3RncStoreUeConfigForSrbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Check NodeB resources for setting SRB1-3 for a UE
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc* : Pointer to the UE RRC data
// RETURN     :: void
// **/
static
void UmtsLayer3RncStoreUeConfigForSrbSetup(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcUeDataInRnc* ueRrc)
{
    //Store uplink DPDCH information
    UmtsUlDpdchInfo* ulDpdch = (UmtsUlDpdchInfo*)
                               MEM_malloc(sizeof(UmtsUlDpdchInfo));
    memset(ulDpdch, 0, sizeof(UmtsUlDpdchInfo));
    ueRrc->ulDpdchInfo[(int)UMTS_SRB_DCH_ID] = ulDpdch;

    //store uplink DCH information
    UmtsUlDchInfo* ulDch = (UmtsUlDchInfo*)
                           MEM_malloc(sizeof(UmtsUlDchInfo));
    memset(ulDch, 0, sizeof(UmtsUlDchInfo));
    ueRrc->ulDchInfo[(int)UMTS_SRB_DPDCH_ID] = ulDch;

    for (int i = 1; i <= 3; i++)
    {
        UmtsSetBit<UInt32>(ulDpdch->rbBitSet, i);
        UmtsSetBit<UInt32>(ulDch->rbBitSet, i);

        ueRrc->ulDcchBitSet->set(i-1);
        UmtsRbMapping* ulRbMapping = (UmtsRbMapping*)
                                     MEM_malloc(sizeof(UmtsRbMapping));
        ulRbMapping->ueId = ueRrc->ueId;
        ulRbMapping->rbId = (char)i;
        ulRbMapping->direction = UMTS_CH_UPLINK;
        ulRbMapping->logChType = UMTS_LOGICAL_DCCH;
        ulRbMapping->logChId = (unsigned char)(i-1);
        ulRbMapping->logChPrio = (unsigned char)i;
        ulRbMapping->trChType = UMTS_TRANSPORT_DCH;
        ulRbMapping->trChId = UMTS_SRB_DCH_ID;
        ueRrc->rbMappingInfo->push_back(ulRbMapping);

        UmtsRbPhChMapping* rbPhMapping =
            new UmtsRbPhChMapping(ueRrc->ueId, (char)i);
        rbPhMapping->AddPhCh(UmtsPhChInfo(UMTS_PHYSICAL_DPDCH,
                                            UMTS_SRB_DPDCH_ID));
        ueRrc->rbPhChMappingList->push_back(rbPhMapping);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRrcReleaseToNodeb
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RRC CONNECTION RELEASE message to Nodeb
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : Node ID of UE
// + cellId    : UInt32                  : cell ID of NodeB
// + nodebSrbConfig : UmtsNodebSrbConfigPara& : configuration parameters
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendRrcReleaseToNodeb(
    Node* node,
    UmtsLayer3Data *umtsL3,
    NodeId ueId,
    UInt32 cellId,
    UmtsNodebSrbConfigPara& nodebSrbConfig)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *)(umtsL3->dataPtr);
    UmtsCellCacheInfo* cacheCell =
        rncL3->cellCacheMap->find(cellId)->second;
    UmtsRrcNodebDataInRnc* nodebRrc = UmtsLayer3RncFindNodebRrc(
                                            node,
                                            rncL3,
                                            cacheCell->nodebId);

    UmtsLayer3RncReleaseNodebConfigForSrbRelease(
        node,
        umtsL3,
        nodebRrc,
        ueId,
        nodebSrbConfig);

    Message* msg = MESSAGE_Alloc(
                       node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(nodebSrbConfig),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           &nodebSrbConfig,
           sizeof(nodebSrbConfig));


    UmtsLayer3RncSendNbapMessageToNodeb(
        node,
        umtsL3,
        msg,
        UMTS_NBAP_MSG_TYPE_RRC_SETUP_RELEASE,
        0,
        nodebRrc->nodebId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendRrcReleaseToNodebViaIur
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RRC CONNECTION RELEASE message to Nodeb through
//               IUR interface
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + cellId    : UInt32                 : cell ID of NodeB
// + drncId    : NodeId                 : Drift RNC id
// + ueId      : NodeId                 : Node ID of UE
// + nodebSrbConfig : UmtsNodebSrbConfigPara& : configuration parameters
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendRrcReleaseToNodebViaIur(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UInt32 cellId,
    NodeId drncId,
    NodeId ueId,
    const UmtsNodebSrbConfigPara& nodebSrbConfig)
{
    int msgSize = sizeof(nodebSrbConfig);
    Message* msg = MESSAGE_Alloc(node, 
                                 NETWORK_LAYER,
                                 MAC_PROTOCOL_CELLULAR, 
                                 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        msgSize,
                        TRACE_UMTS_LAYER3);
    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);

    memcpy(&packet[index], &nodebSrbConfig, sizeof(nodebSrbConfig));

    UmtsLayer3RncSendIurMsg(
        node,
        umtsL3,
        msg,
        ueId,
        drncId,
        UMTS_RNSAP_MSG_TYPE_SRB_RELEASE_REQUEST,
        (char*)&cellId,
        sizeof(cellId));
}


// /**
// FUNCTION   :: UmtsLayer3RncReqNodebToReleaseSrb
// LAYER      :: Layer3 RRC
// PURPOSE    :: Request to Nodeb to release SRB 1-3
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc* : UE RRC data
// RETURN     :: void
// **/
static
void UmtsLayer3RncReqNodebToReleaseSrb(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcUeDataInRnc* ueRrc)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*)(umtsL3->dataPtr);
    std::list<UmtsMntNodebInfo*>::iterator mntIt;
    for (mntIt = ueRrc->mntNodebList->begin();
         mntIt != ueRrc->mntNodebList->end();
         ++mntIt)
    {
        UmtsNodebSrbConfigPara nodebPara;
        nodebPara.uePriSc = ueRrc->primScCode;

        if ((*mntIt)->cellStatus != UMTS_CELL_ACTIVE_SET &&
            (*mntIt)->cellStatus != UMTS_CELL_MONIT_TO_ACT_SET)
        {
            continue;
        }
        UmtsCellCacheMap::iterator nodebIt =
            rncL3->cellCacheMap->find((*mntIt)->cellId);
        ERROR_Assert(nodebIt != rncL3->cellCacheMap->end(),
            "cannot find the NodeB info in the monitored set.");
        if (nodebIt->second->rncId == node->nodeId)
        {
            UmtsLayer3RncSendRrcReleaseToNodeb(
                node,
                umtsL3,
                ueRrc->ueId,
                (*mntIt)->cellId,
                nodebPara);
        }
        else
        {
            UmtsLayer3RncSendRrcReleaseToNodebViaIur(
                node,
                umtsL3,
                (*mntIt)->cellId,
                nodebIt->second->rncId,
                ueRrc->ueId,
                nodebPara);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncInitSignalRb1To3Config
// LAYER      :: Layer3 RRC
// PURPOSE    :: Initiate configuring signaling RB 1-3 by sending
//               NBAP RADIO LINK SETUP REQUEST message to the related
//               NodeB
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + ueId      : NodeId             : The UE Id to be set up
// + ueRrc     : UmtsRrcUeDataInRnc*: The UE RRC data stored in the RNC
// + nodebId   : NodeId             : The Nodeb Id to be set up
// RETURN     :: BOOL               : TRUE if resources can be allocated
// **/
static
BOOL UmtsLayer3RncInitSignalRb1To3Config(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    NodeId              ueId,
    UmtsRrcUeDataInRnc* ueRrc,
    NodeId              nodebId)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char errorMsg[MAX_STRING_LENGTH];
    UmtsRrcNodebDataInRnc* nodebRrc = UmtsLayer3RncFindNodebRrc(
                                            node,
                                            rncL3,
                                            nodebId);
    sprintf(errorMsg, "UmtsLayer3RncInitSignalRb1To3Config: "
        "Node: %d (RNC) cannot find a NodeB RRC data for NodeB: %d",
        node->nodeId, nodebId);
    ERROR_Assert(nodebRrc, "Cannot find NodeB information in the RNC.");

    //Calculate the required channel resources at the NodeB
    UmtsNodebSrbConfigPara nodebSrbConfig;
    nodebSrbConfig.uePriSc = ueRrc->primScCode;
    if (FALSE == UmtsLayer3RncCheckNodebForSrbSetup(node,
                                                    umtsL3,
                                                    nodebRrc,
                                                    nodebSrbConfig))
    {
        return FALSE;
    }

    UmtsLayer3RncStoreNodebConfigForSrbSetup(
        node,
        umtsL3,
        nodebRrc,
        ueId,
        nodebSrbConfig);

    UmtsLayer3RncStoreUeConfigForSrbSetup(
        node,
        umtsL3,
        ueRrc);

    UmtsLayer3RncSendRrcSetupToNodeb(
        node,
        umtsL3,
        nodebRrc,
        ueId,
        &nodebSrbConfig);

    //store pending UE downlink DPDCH information
    UmtsRcvPhChInfo* ueDlDpdchInfo =
                            (UmtsRcvPhChInfo*)
                            MEM_malloc(sizeof(UmtsRcvPhChInfo));
    ueDlDpdchInfo->sfFactor = nodebSrbConfig.sfFactor;
    ueDlDpdchInfo->chCode = nodebSrbConfig.chCode;
    ueRrc->pendingDlDpdchInfo = ueDlDpdchInfo;

    return TRUE;
}

// /**
// FUNCTION   :: UmtsLayer3RncStartConfigRrcConn
// LAYER      :: Layer3 RRC
// PURPOSE    :: Start to config UE RRC CONNECTION
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + ueId      : NodeId             : The UE Id to be set up
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Node RRC data at RNC
// RETURN     :: BOOL               : TRUE if resources can be allocated
// **/
static
UmtsRrcUeDataInRnc* UmtsLayer3RncStartConfigRrcConn(
                    Node*               node,
                    UmtsLayer3Data*     umtsL3,
                    NodeId              ueId,
                    UmtsRrcNodebDataInRnc* nodebRrc)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *)(umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncInitUeRrc(
                                    node,
                                    ueId,
                                    nodebRrc);

    if (TRUE == UmtsLayer3RncInitSignalRb1To3Config(
                        node,
                        umtsL3,
                        ueId,
                        ueRrc,
                        nodebRrc->nodebId))
    {
        rncL3->ueRrcs->push_back(ueRrc);
    }
    else
    {
        //update stat
        rncL3->stat.numRrcConnSetupRejSent ++;

        UmtsLayer3RncSendRrcConnRej(
             node,
             umtsL3,
             0,
             ueId,
             nodebRrc->nodebId);
        UmtsLayer3RncFinalizeUeRrc(node, ueRrc);
        ueRrc = NULL;
    }
    return ueRrc;
}

// /**
// FUNCTION   :: UmtsLayer3RncInitRrcConnRelease
// LAYER      :: Layer3 RRC
// PURPOSE    :: Initiate releasing signaling RB 1-3
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : The UE RRC data stored in the RNC
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Pointer to the NodeB RRC data 
// + BOOL      : releaseUeSrb           : rlease UE res. for SRB?
// RETURN     :: Message*               : Pointer to the message to 
//                                        release SRB in the NodeB
// **/
static
void UmtsLayer3RncInitRrcConnRelease(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsRrcUeDataInRnc*     ueRrc,
    BOOL                    networkInit)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *)(umtsL3->dataPtr);

    ueRrc->rrcRelReq.requested = TRUE;
    ueRrc->rrcRelReq.networkInit = networkInit;

    //Check current RAB request serving queue, remove those
    //belonging to this UE, if the first one belong to the UE,
    //start to serve the first queued request not belonging to the UE
    BOOL curServReq = FALSE;
    UmtsLayer3RncRmvRabReqUponConnRel(node, rncL3, ueRrc->ueId, curServReq);

    UmtsLayer3RncReleaseUeConfigForSrbRelease(
        node,
        umtsL3,
        ueRrc);

    UmtsLayer3RncSendRrcConnRelease(
        node,
        umtsL3,
        ueRrc);

    ueRrc->state = UMTS_RRC_STATE_IDLE;

    if (curServReq)
    {
        //start to serve this request
        UmtsLayer3RncServRabReqQueue(node, umtsL3, rncL3);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncCheckNodebForRbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Check NodeB resources for Radio Bearer Setup
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebId   : NodeId             : The Nodeb Id to be set up
// RETURN     :: BOOL               : TRUE if resources can be allocated
// **/
static
BOOL UmtsLayer3RncCheckNodebForRbSetup(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    const UmtsRABServiceInfo*   rabPara,
    UmtsNodebRbSetupPara* rbSetupPara)
{
    NodeId ueId = rbSetupPara->ueId;
    int dlDtchId = UmtsGetPosForNextZeroBit(
                            *(nodebRrc->dlDtchBitSet), 0);
    int dlDchId = UmtsGetPosForNextZeroBit(
                            *(nodebRrc->dlDchBitSet), 0);
    if (dlDtchId == -1 || dlDchId == -1 )
        return FALSE;

    int dataRate = rabPara->maxBitRate;
    int dpdchAvail = (int)(nodebRrc->dlDpdchBitSet->size()
                           - nodebRrc->dlDpdchBitSet->count());
    if (!dpdchAvail)
    {
        return FALSE;
    }

    // Uplink admission control
    if (!UmtsLayer3RncUlCallAdmissionControl(
        node, umtsL3, nodebRrc,
        rbSetupPara->ulBwAlloc,
        rabPara->maxBitRate))
    {
        return FALSE;
    }

    // downlink admission control
    if (!UmtsLayer3RncDlCallAdmissionControl(
        node, umtsL3, nodebRrc,
        rbSetupPara->dlBwAlloc,
        rabPara->maxBitRate,
        rbSetupPara->hsdpaRb))
    {
        return FALSE;
    }

    // downlink CAC is omited here by asusmimg dl power can be satisfied
    // thus leave the CAC to OVSF code allocation alone

    if (UMTS_MAX_DLDPDCH_PER_RAB > dpdchAvail)
    {
        rbSetupPara->numDpdch = (char)dpdchAvail;
    }
    else
    {
        rbSetupPara->numDpdch = (char)UMTS_MAX_DLDPDCH_PER_RAB;
    }
    //UmtsSpreadFactor sfFactor[UMTS_MAX_DLDPDCH_PER_RAB];
    //unsigned int chCode[UMTS_MAX_DLDPDCH_PER_RAB];
    UmtsModulationType modType;
    UmtsCodingType codingType;

    // Find the right modulation and coding type according to the
    // error rate requirement andother QoS paras
    // By Default use QPSK and TURBO 1/3
    modType = UMTS_MOD_QPSK;
    codingType = UMTS_CONV_3;//UMTS_TURBO_3;

    int numDpdch = UMTS_MAX_DLDPDCH_PER_RAB;

    if (UmtsLayer3RncAllocNodebOvsf(
        node,
        umtsL3,
        nodebRrc,
        dataRate,
        modType,
        codingType,
        rbSetupPara->sfFactor,
        rbSetupPara->chCode,
        &numDpdch,
        rbSetupPara->hsdpaRb))
    {
        rbSetupPara->rlSetupNeeded = TRUE;
    }

    if (!numDpdch && rabPara->maxBitRate > 0)
    {
        return FALSE;
    }


    //TODO: Admission Control and allocating channelization code if new
    //physical channel needs to be added according to the QoS requirments

    //Use the DPDCH used by SRBs for RB that does not have a
    //bandwidth requirement
    if (rabPara->maxBitRate == 0)
    {
        numDpdch = 1;
        //Find the DPDCH ID for the SRBs
        std::list<UmtsRbPhChMapping*>::iterator phMapPos;
        phMapPos = std::find_if(
                    nodebRrc->rbPhChMappingList->begin(),
                    nodebRrc->rbPhChMappingList->end(),
                    UmtsFindItemByUeIdAndRbId<UmtsRbPhChMapping>
                                           (ueId, UMTS_SRB1_ID));
        ERROR_Assert(phMapPos != nodebRrc->rbPhChMappingList->end(),
                "Could not find the SRB/physical channel mapping data");
        rbSetupPara->dlDpdchId[0] =
            (unsigned char)((*phMapPos)->phChList->front()->chId);

        std::list<UmtsDlDpdchInfo*>::iterator dlDpdchIter =
            std::find_if(nodebRrc->dlDpdchInfo->begin(),
                         nodebRrc->dlDpdchInfo->end(),
                         UmtsFindItemByChId<UmtsDlDpdchInfo>
                            (rbSetupPara->dlDpdchId[0]));
        ERROR_Assert(dlDpdchIter != nodebRrc->dlDpdchInfo->end(),
                "Could not DPDCH info for SRBs");
        rbSetupPara->sfFactor[0] = (*dlDpdchIter)->sfFactor;
        rbSetupPara->chCode[0] = (*dlDpdchIter)->dlChCode;

    }
    else
    {
        int dlDpdchId = 0;
        for (int i = 0; i < rbSetupPara->numDpdch; i++)
        {
            dlDpdchId = UmtsGetPosForNextZeroBit(
                            *(nodebRrc->dlDpdchBitSet),
                            i == 0? 0 : dlDpdchId + 1);
            if (dlDpdchId == -1)
            {
               return FALSE;
            }
            rbSetupPara->dlDpdchId[i] = (unsigned char)dlDpdchId;
        }
    }

    rbSetupPara->numDpdch = (unsigned char)numDpdch;
    rbSetupPara->dlDchId = (unsigned char)dlDchId;
    rbSetupPara->dlDtchId = (unsigned char)dlDtchId;

    return TRUE;
}

// /**
// FUNCTION   :: UmtsLayer3RncCheckUeForRbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Check Ue resources for Radio Bearer Setup
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc* : Pointer to the UE RRC data
// + rbSetupPara : UmtsUeRbSetupPara* : RB setup parameters
// + ulRabPara : const UmtsRABServiceInfo* : UL RAB para
// RETURN     :: BOOL               : TRUE if resources can be allocated
// **/
static
BOOL UmtsLayer3RncCheckUeForRbSetup(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    UmtsUeRbSetupPara*  rbSetupPara,
    const UmtsRABServiceInfo*   ulRabPara)
{
    char rbId = UMTS_INVALID_RB_ID;
    char rabId;
    UmtsSpreadFactor sf = UMTS_SF_4;

    // find Spread Factor
    rabId = rbSetupPara->rabId;

    //  assume rate 1/3 is  used
    if (UmtsGetBestUlSpreadFactorForRate(
        (ueRrc->rabInfos[(int)rabId]->ulRabPara.maxBitRate +
            ueRrc->curBwReq) * 3,
            &sf))
    {
        rbSetupPara->sfFactor = sf;
        ueRrc->curSf = sf;
        rbSetupPara->numUlDPdch = (unsigned char)
                ceil(((double)(ueRrc->rabInfos[(int)rabId]->
                       ulRabPara.maxBitRate) +
                ueRrc->curBwReq) * 3 / 960000);   
                     // SF4 960000bps per channel
    }
    else
    {
        // TODO: admission control
        // rbSetupPara->sfFactor = ueRrc->curSf;
        // rbSetupPara->sfFactor = sf;
        // Currently, only one channel is supported at UE
        // so the maximum UL data rate for a single UE
        // is 1920kbps (inlcuidng overhead)
        // The Following is for Temporary Use ONLY, 
        // assume 1/3 rate is used
        if ((ueRrc->rabInfos[(int)rabId]->ulRabPara.maxBitRate +
            ueRrc->curBwReq) * 3 <= UMTS_MAX_UL_CAPACITY_3 * 3)
        {
            rbSetupPara->sfFactor = sf;
            rbSetupPara->numUlDPdch = (unsigned char)
                ceil(((double)(ueRrc->rabInfos[(int)rabId]->
                               ulRabPara.maxBitRate) +
                ueRrc->curBwReq) * 3 / 960000); 
                                   // SF4 960000bps per channel
            if (rbSetupPara->numUlDPdch > 6)
            {
                rbSetupPara->numUlDPdch = 6;
            }
        }
        else
        {
            return FALSE;
        }
    }

    for (int i = 0; i < UMTS_MAX_RB_ALL_RAB; i++)
    {
        if (ueRrc->rbRabId[i] == UMTS_INVALID_RAB_ID)
        {
            rbId = (char)(i + UMTS_RBINRAB_START_ID);
            break;
        }
    }
    if (rbId == UMTS_INVALID_RB_ID)
        return FALSE;

    int ulDtchId = UmtsGetPosForNextZeroBit(
                            *(ueRrc->ulDtchBitSet),
                            0);
    int ulDchId = -1;
    for (int i = 0; i < UMTS_MAX_TRCH; i++)
    {
        if (!ueRrc->ulDchInfo[i])
        {
            ulDchId = i;
            break;
        }
    }
    int ulDpdchId = -1;
    //If uplink has no bandwidth requirement, map
    //the uplink RB into the DPDCH the SRBs are mapping into
    ulDpdchId = UMTS_SRB_DPDCH_ID;
    //if (ulRabPara->maxBitRate == 0)
    //{
    //    ulDpdchId = UMTS_SRB_DPDCH_ID;
    //}
    //else
    //{
    //    for (int i = 0; i < UMTS_MAX_DPDCH_UL; i++)
    //    {
    //        if (!ueRrc->ulDpdchInfo[i])
    //        {
    //            ulDpdchId = i;
    //            break;
    //        }
    //    }
    //}
    if (ulDtchId == -1 || ulDchId == -1 || ulDpdchId == -1 )
        return FALSE;

    // to see if need to adjust the radio link
    // right now. it is not in effect
    if (ulRabPara->maxBitRate == 0)
    {
        rbSetupPara->rlSetupNeeded = FALSE;
    }
    else
    {
        // rbSetupPara->rlSetupNeeded = TRUE;
        rbSetupPara->rlSetupNeeded = FALSE;
        // currently, no new UL Data PH Ch is added
        // all RB use only one single code
    }

    if (rbSetupPara->hsdpaRb)
    {
        // for HSDPA ul MxbitRate should be 0
        ueRrc->hsdpaInfo.numHsdpaRb ++;
        if (ueRrc->hsdpaInfo.numHsdpaRb == 1)
        {
            // for the first HSDPA RB
            // request UE to add Uplink HS-DPCCH
            rbSetupPara->rlSetupNeeded = TRUE;
            if (DEBUG_HSDPA)
            {
                UmtsLayer3PrintRunTimeInfo(
                    node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\t need to request UE setup HS-DPCCH\n");
            }
        }
    }

    rbSetupPara->rbId = rbId;
    rbSetupPara->ulDtchId = (char)ulDtchId;
    rbSetupPara->ulDchId = (char)ulDchId;
    rbSetupPara->ulDpdchId = (char)ulDpdchId;

    return TRUE;
}

// /**
// FUNCTION   :: UmtsLayer3RncReleaseNodebConfigForRbRelease
// LAYER      :: Layer3 RRC
// PURPOSE    :: Store Nodeb Resource channges for RB Setup
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : NodeB RRC data
// + nodebPara : UmtsNodebRbSetupPara& : NodeB RB setup parameters
// RETURN     :: void
// **/
static
BOOL UmtsLayer3RncReleaseNodebConfigForRbRelease(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    UmtsNodebRbSetupPara& nodebPara)
{
    BOOL retVal = TRUE;
    NodeId ueId = nodebPara.ueId;
    std::list<UmtsRbPhChMapping*>::iterator phChMapPos;
    phChMapPos = std::find_if(nodebRrc->rbPhChMappingList->begin(),
                              nodebRrc->rbPhChMappingList->end(),
                              UmtsFindItemByUeIdAndRbId
                                <UmtsRbPhChMapping>(ueId, nodebPara.rbId));
    std::list<UmtsRbMapping*>::iterator dlRbMapPos;
    dlRbMapPos = std::find_if(nodebRrc->rbMappingInfo->begin(),
                              nodebRrc->rbMappingInfo->end(),
                              UmtsFindItemByUeIdAndRbId
                                <UmtsRbMapping>(ueId, nodebPara.rbId));

    if (phChMapPos == nodebRrc->rbPhChMappingList->end() &&
            dlRbMapPos == nodebRrc->rbMappingInfo->end())
    {
        retVal = FALSE;
    }
    else if (phChMapPos != nodebRrc->rbPhChMappingList->end() &&
            dlRbMapPos != nodebRrc->rbMappingInfo->end())
    {
        std::list<UmtsPhChInfo*>* dlPhChList = (*phChMapPos)->phChList;
        std::list<UmtsPhChInfo*>::iterator phChPos;
        int j;

        // HSDPA
        if (nodebPara.hsdpaRb)
        {
            nodebRrc->hsdpaInfo.numHsdpaRb --;
            if (nodebRrc->hsdpaInfo.numHsdpaRb == 0)
            {
                // request nodeB to release HSDPSCH and HSSCCH
                nodebPara.rlSetupNeeded = TRUE;
                nodebRrc->hsdpaInfo.numHspdschAlloc = 0;
                nodebRrc->hsdpaInfo.numHsscchAlloc = 0;
            }
            else
            {
                // if not the last one HSDPA RB, do not
                // release physicl channel
                nodebPara.rlSetupNeeded = FALSE;
            }
        }
        else
        {
            // for regular Rb, release PH CH immediately
            nodebPara.rlSetupNeeded = TRUE;
        }
        for (j = 0, phChPos = dlPhChList->begin() ;
             phChPos != dlPhChList->end(); ++phChPos, j++)
        {
            nodebPara.dlDpdchId[j] = (unsigned char)((*phChPos)->chId);

            std::list<UmtsDlDpdchInfo*>::iterator dpdchPos;
            dpdchPos = std::find_if(nodebRrc->dlDpdchInfo->begin(),
                                    nodebRrc->dlDpdchInfo->end(),
                                    UmtsFindItemByChId<UmtsDlDpdchInfo>
                                        (nodebPara.dlDpdchId[j]));
            if (dpdchPos != nodebRrc->dlDpdchInfo->end())
            {
                UmtsClearBit<UInt32>((*dpdchPos)->rbBitSet, nodebPara.rbId);
                if ((*dpdchPos)->rbBitSet == 0)
                {
                    nodebPara.sfFactor[j] = (*dpdchPos)->sfFactor;
                    nodebPara.chCode[j] = (*dpdchPos)->dlChCode;
                    nodebPara.numDpdch ++;
                    MEM_free(*dpdchPos);
                    nodebRrc->dlDpdchInfo->erase(dpdchPos);

                    //release resource in the OVSF tree
                    // normal rb release
                    // no need for HSDPA release if no more HSDPA rb
                    if (!nodebPara.hsdpaRb)
                    {
                        UmtsLayer3RncReleaseNodebOvsf(
                            node,
                            umtsL3,
                            nodebRrc,
                            nodebPara.numDpdch,
                            nodebPara.chCode);
                    }
                }
            }
        }
        delete *phChMapPos;
        nodebRrc->rbPhChMappingList->erase(phChMapPos);

        nodebPara.rlcType = (*dlRbMapPos)->rlcType;
        nodebPara.dlDtchId = (*dlRbMapPos)->logChId;
        nodebPara.dlDchId = (*dlRbMapPos)->trChId;
        nodebPara.logPriority = (*dlRbMapPos)->logChPrio;

        MEM_free(*dlRbMapPos);
        nodebRrc->rbMappingInfo->erase(dlRbMapPos);

        nodebRrc->dlDtchBitSet->reset(nodebPara.dlDtchId);

        std::list<UmtsDlDchInfo*>::iterator dlDchPos;
        dlDchPos = std::find_if(nodebRrc->dlDchInfo->begin(),
                                nodebRrc->dlDchInfo->end(),
                                UmtsFindItemByChId<UmtsDlDchInfo>
                                    (nodebPara.dlDchId));
        if (dlDchPos != nodebRrc->dlDchInfo->end())
        {
            UmtsClearBit<UInt32>((*dlDchPos)->rbBitSet, nodebPara.rbId);
            if ((*dlDchPos)->rbBitSet == 0)
            {
                MEM_free((*dlDchPos));
                nodebRrc->dlDchInfo->erase(dlDchPos);
                nodebRrc->dlDchBitSet->reset(nodebPara.dlDchId);
            }
        }
    }
    else
    {
        ERROR_ReportError("UmtsLayer3RncReleaseNodebConfigForRbRelease: "
            "inconsistent configurations stored in RNC.");
    }

    if (!nodebPara.hsdpaRb)
    {
        // update UL BW allocation
        nodebRrc->ulResUtil += nodebRrc->totalUlBwAlloc *
                                           (node->getNodeTime() -
                                           nodebRrc->lastUlResUpdateTime) /
                                           SECOND;
        nodebRrc->totalUlBwAlloc -= nodebPara.ulBwAlloc;
        nodebRrc->lastUlResUpdateTime = node->getNodeTime();


        // update Dl BW Allocation
        nodebRrc->dlResUtil += nodebRrc->totalDlBwAlloc *
                                           (node->getNodeTime() -
                                           nodebRrc->lastDlResUpdateTime) / 
                                           SECOND;
        nodebRrc->totalDlBwAlloc -= nodebPara.dlBwAlloc;
        nodebRrc->lastDlResUpdateTime = node->getNodeTime();
    }
    else
    {
        // update
        nodebRrc->hsdpaInfo.totalHsBwReq -= nodebPara.dlBwAlloc;
        // nodebRrc->hsdpaInfo.totalHsBwAllo -= nodebPara.dlBwAlloc;

    }

    return retVal;
}

// /**
// FUNCTION   :: UmtsLayer3RncStoreNodebConfigForRbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Store Nodeb Resource channges for RB Setup
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : NodeB RRC data
// + nodebPara : const UmtsNodebRbSetupPara& : NodeB RB setup parameters
// + dlRabPara : const UmtsRABServiceInfo* : DL RAB parameters
// RETURN     :: void
// **/
static
void UmtsLayer3RncStoreNodebConfigForRbSetup(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    const UmtsNodebRbSetupPara& nodebPara,
    const UmtsRABServiceInfo*   dlRabPara)
{
    NodeId ueId = nodebPara.ueId;
    UmtsRbPhChMapping* rbPhMapping =
        new UmtsRbPhChMapping(ueId, nodebPara.rbId);

    for (int i = 0; i < nodebPara.numDpdch; i++)
    {
        UmtsDlDpdchInfo* dlDpdch;
        std::list<UmtsDlDpdchInfo*>::iterator dlDpdchIter =
            std::find_if(nodebRrc->dlDpdchInfo->begin(),
                         nodebRrc->dlDpdchInfo->end(),
                         UmtsFindItemByChId<UmtsDlDpdchInfo>
                            (nodebPara.dlDpdchId[i]));
        if (dlDpdchIter == nodebRrc->dlDpdchInfo->end())
        {
            dlDpdch = (UmtsDlDpdchInfo*)
                      MEM_malloc(sizeof(UmtsDlDpdchInfo));
            memset(dlDpdch, 0, sizeof(UmtsDlDpdchInfo));
            dlDpdch->chId = nodebPara.dlDpdchId[i];
            dlDpdch->sfFactor = nodebPara.sfFactor[i];
            dlDpdch->dlChCode = nodebPara.chCode[i];
            nodebRrc->dlDpdchInfo->push_back(dlDpdch);
            nodebRrc->dlDpdchBitSet->set(nodebPara.dlDpdchId[i]);
        }
        else
        {
            dlDpdch = (*dlDpdchIter);
        }
        UmtsSetBit<UInt32>(dlDpdch->rbBitSet, nodebPara.rbId);
        if (!nodebPara.hsdpaRb)
        {
            rbPhMapping->AddPhCh(UmtsPhChInfo(
                                    UMTS_PHYSICAL_DPDCH,
                                    nodebPara.dlDpdchId[i]));
        }
        else
        {
            if (nodebPara.sfFactor[i] == UMTS_SF_16)
            {
                rbPhMapping->AddPhCh(UmtsPhChInfo(
                                    UMTS_PHYSICAL_HSPDSCH,
                                    nodebPara.dlDpdchId[i]));
            }
            else if (nodebPara.sfFactor[i] == UMTS_SF_128)
            {
                rbPhMapping->AddPhCh(UmtsPhChInfo(
                                    UMTS_PHYSICAL_HSSCCH,
                                    nodebPara.dlDpdchId[i]));
            }
            else
            {

            }
        }

    }
    nodebRrc->rbPhChMappingList->push_back(rbPhMapping);

    //store downlink DCH information at the NodeB
    UmtsDlDchInfo* dlDch;
    std::list<UmtsDlDchInfo*>::iterator dlDchIter =
        std::find_if(
            nodebRrc->dlDchInfo->begin(),
            nodebRrc->dlDchInfo->end(),
            UmtsFindItemByChId<UmtsDlDchInfo>(nodebPara.dlDchId));
    if (dlDchIter == nodebRrc->dlDchInfo->end())
    {
        dlDch = (UmtsDlDchInfo*) MEM_malloc(sizeof(UmtsDlDchInfo));
        memset(dlDch, 0, sizeof(UmtsDlDchInfo));
        dlDch->chId = nodebPara.dlDchId;
        nodebRrc->dlDchInfo->push_back(dlDch);
        nodebRrc->dlDchBitSet->set(nodebPara.dlDchId);
    }
    else
    {
        dlDch = *dlDchIter;
    }
    UmtsSetBit<UInt32>(dlDch->rbBitSet, nodebPara.rbId);

    //Store RB mapping info
    //Downlink
    UmtsRbMapping* dlRbMapping = (UmtsRbMapping*)
                                 MEM_malloc(sizeof(UmtsRbMapping));
    dlRbMapping->ueId = ueId;
    dlRbMapping->rbId = nodebPara.rbId;
    dlRbMapping->rlcType = nodebPara.rlcType;
    dlRbMapping->direction = UMTS_CH_DOWNLINK;
    dlRbMapping->logChType = UMTS_LOGICAL_DTCH;
    dlRbMapping->logChId = nodebPara.dlDtchId;
    dlRbMapping->logChPrio = nodebPara.logPriority;
    if (!nodebPara.hsdpaRb)
    {
        dlRbMapping->trChType = UMTS_TRANSPORT_DCH;
    }
    else
    {
        dlRbMapping->trChType = UMTS_TRANSPORT_HSDSCH;
    }

    dlRbMapping->trChId = nodebPara.dlDchId;

    nodebRrc->rbMappingInfo->push_back(dlRbMapping);
    nodebRrc->dlDtchBitSet->set(nodebPara.dlDtchId);

    if (!nodebPara.hsdpaRb)
    {
        // update the allocated UL resource in this NodeB
        nodebRrc->ulResUtil += nodebRrc->totalUlBwAlloc *
                               (node->getNodeTime() -
                               nodebRrc->lastUlResUpdateTime) / SECOND;
        nodebRrc->totalUlBwAlloc += nodebPara.ulBwAlloc;
        if (nodebRrc->totalUlBwAlloc > nodebRrc->peakUlBwAlloc)
        {
            nodebRrc->peakUlBwAlloc = nodebRrc->totalUlBwAlloc;
        }
        nodebRrc->lastUlResUpdateTime = node->getNodeTime();

        // update resource utilization before alloc
        nodebRrc->dlResUtil += nodebRrc->totalDlBwAlloc *
                               (node->getNodeTime() -
                               nodebRrc->lastDlResUpdateTime) / SECOND;

        nodebRrc->totalDlBwAlloc += nodebPara.dlBwAlloc;
        if (nodebRrc->totalDlBwAlloc > nodebRrc->peakDlBwAlloc)
        {
            nodebRrc->peakDlBwAlloc = nodebRrc->totalDlBwAlloc;
        }
        nodebRrc->lastDlResUpdateTime = node->getNodeTime();
    }
    else
    {
        // update the Bw requirement
        /*nodebRrc->hsdpaInfo.bwReqPerPrio[dlRabPara->trafficClass] +=
            dlRabPara->maxBitRate;*/
        nodebRrc->hsdpaInfo.totalHsBwReq += dlRabPara->maxBitRate;
        nodebRrc->hsdpaInfo.totalHsBwAllo += nodebPara.dlBwAlloc;
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncPreStoreUeConfigForRbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Store Nodeb Resource channges for RB Setup
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc* : UE Data In RNC
// + uePara    : const UmtsUeRbSetupPara& : UE RB Setup para
// + domain    : UmtsLayer3CnDomainId     : Domina ID
// + connMap   : const UmtsRabConnMap*    : Rb conn Map
// + rabPara   : const UmtsRABServiceInfo* : RAB Info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncPreStoreUeConfigForRbSetup(
    Node*                     node,
    UmtsLayer3Data*           umtsL3,
    UmtsRrcUeDataInRnc*       ueRrc,
    const UmtsUeRbSetupPara&  uePara,
    UmtsLayer3CnDomainId      domain,
    const UmtsRabConnMap*     connMap,
    const UmtsRABServiceInfo* rabPara)
{
    NodeId ueId = ueRrc->ueId;
    char rbId = uePara.rbId;
    char rabId = uePara.rabId;
    char ulDpdchId = uePara.ulDpdchId;
    char ulDchId = uePara.ulDchId;
    char ulDtchId = uePara.ulDtchId;

    UmtsRrcRabInfo* rabInfo = ueRrc->rabInfos[(int)rabId];
    rabInfo->rbIds[0] = rbId;
    ueRrc->rbRabId[rbId - UMTS_RBINRAB_START_ID] = rabId;
    ueRrc->pendingRab[(int)rabId] = new UmtsRrcPendingRabInfo(uePara);

    //Store uplink DPDCH information at the UE
    UmtsUlDpdchInfo* ulDpdch = ueRrc->ulDpdchInfo[(int)ulDpdchId];
    if (!ulDpdch)
    {
        ulDpdch = (UmtsUlDpdchInfo*) MEM_malloc(sizeof(UmtsUlDpdchInfo));
        memset(ulDpdch, 0, sizeof(UmtsUlDpdchInfo));
        ueRrc->ulDpdchInfo[(int)ulDpdchId] = ulDpdch;
    }
    UmtsSetBit<UInt32>(ulDpdch->rbBitSet, rbId);

    //store uplink DCH information at the UE
    UmtsUlDchInfo* ulDch = ueRrc->ulDchInfo[(int)ulDchId];
    if (!ulDch)
    {
        ulDch = (UmtsUlDchInfo*) MEM_malloc(sizeof(UmtsUlDchInfo));
        memset(ulDch, 0, sizeof(UmtsUlDchInfo));
        ueRrc->ulDchInfo[(int)ulDchId] = ulDch;
    }
    UmtsSetBit<UInt32>(ulDch->rbBitSet, rbId);

    UmtsRbPhChMapping* rbPhMapping = new UmtsRbPhChMapping(ueId, rbId);
    if (!uePara.hsdpaRb)
    {
        rbPhMapping->AddPhCh(UmtsPhChInfo(UMTS_PHYSICAL_DPDCH,
                                          ulDpdchId));
    }
    else
    {
        rbPhMapping->AddPhCh(UmtsPhChInfo(
                             UMTS_PHYSICAL_DPDCH, //UMTS_PHYSICAL_HSDPCCH,
                             ulDpdchId));
    }
    ueRrc->rbPhChMappingList->push_back(rbPhMapping);

    UmtsRbMapping* ulRbMapping = (UmtsRbMapping*)
                                 MEM_malloc(sizeof(UmtsRbMapping));
    ulRbMapping->ueId = ueId;
    ulRbMapping->rbId = rbId;
    ulRbMapping->rlcType = uePara.rlcType;
    ulRbMapping->direction = UMTS_CH_UPLINK;
    ulRbMapping->logChType = UMTS_LOGICAL_DTCH;
    ulRbMapping->logChId = ulDtchId;
    ulRbMapping->logChPrio = uePara.logPriority;
    if (uePara.hsdpaRb)
    {
        ulRbMapping->trChType = UMTS_TRANSPORT_HSDSCH;
    }
    else
    {
        ulRbMapping->trChType = UMTS_TRANSPORT_DCH;
    }

    ulRbMapping->trChId = ulDchId;
    ueRrc->rbMappingInfo->push_back(ulRbMapping);
    ueRrc->ulDtchBitSet->set(ulDtchId);
}

// /**
// FUNCTION   :: UmtsLayer3RncReleaseUeConfigForRbRelease
// LAYER      :: Layer3 RRC
// PURPOSE    :: Store Nodeb Resource channges for RB Setup
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*: UE Data in RNC
// + rabId     : char               : RAB Id 
// RETURN     :: void
// **/
static
void UmtsLayer3RncReleaseUeConfigForRbRelease(
    Node*                     node,
    UmtsLayer3Data*           umtsL3,
    UmtsRrcUeDataInRnc*       ueRrc,
    char                      rabId)
{
    //Find the RB ID assigned to this RAB
    UmtsRrcRabInfo* rabInfo = ueRrc->rabInfos[(int)rabId];
    UmtsRrcPendingRabInfo* pendingRab = ueRrc->pendingRab[(int)rabId];

    if (!rabInfo || pendingRab)
        return;

    UmtsUeRbSetupPara& uePara = pendingRab->uePara;
    uePara.rabId = rabId;

    //Assume each RAB only contains a RB.
    int subflowId = 0;
    uePara.rbId = rabInfo->rbIds[subflowId];
    uePara.domain = rabInfo->cnDomainId;
    ERROR_Assert(uePara.rbId != UMTS_INVALID_RB_ID,
        "There is no allocated RB for the RAB." );
    ueRrc->rbRabId[uePara.rbId - UMTS_RBINRAB_START_ID]
                                        = UMTS_INVALID_RAB_ID;

    std::list<UmtsRbPhChMapping*>::iterator phChMapPos;
    phChMapPos = std::find_if(ueRrc->rbPhChMappingList->begin(),
                              ueRrc->rbPhChMappingList->end(),
                              UmtsFindItemByRbId
                                <UmtsRbPhChMapping>(uePara.rbId));

    if (phChMapPos != ueRrc->rbPhChMappingList->end())
    {
        std::list<UmtsPhChInfo*>* ulPhChList = (*phChMapPos)->phChList;
        uePara.ulDpdchId = (char)(ulPhChList->front()->chId);
        UmtsUlDpdchInfo* ulDpdch = ueRrc->ulDpdchInfo[(int)uePara.ulDpdchId];
        if (ulDpdch)
        {
            UmtsClearBit<UInt32>(ulDpdch->rbBitSet, uePara.rbId);
            if (ulDpdch->rbBitSet == 0)
            {
                MEM_free(ulDpdch);
                ueRrc->ulDpdchInfo[(int)uePara.ulDpdchId] = NULL;
            }
        }
        delete *phChMapPos;
        ueRrc->rbPhChMappingList->erase(phChMapPos);
    }

    //Search for uplink RB mapping info
    std::list<UmtsRbMapping*>::iterator ulRbMapPos;
    ulRbMapPos = std::find_if(ueRrc->rbMappingInfo->begin(),
                              ueRrc->rbMappingInfo->end(),
                              UmtsFindItemByRbIdAndChDir
                                <UmtsRbMapping, UmtsChannelDirection>
                                (uePara.rbId, UMTS_CH_UPLINK));
    if (ulRbMapPos != ueRrc->rbMappingInfo->end())
    {
        uePara.rlcType = (*ulRbMapPos)->rlcType;
        uePara.ulDtchId = (*ulRbMapPos)->logChId;
        uePara.ulDchId = (*ulRbMapPos)->trChId;

        if ((*ulRbMapPos)->trChType == UMTS_TRANSPORT_HSDSCH)
        {
            ueRrc->hsdpaInfo.numHsdpaRb --;
            uePara.hsdpaRb = TRUE;

            if (ueRrc->hsdpaInfo.numHsdpaRb == 0)
            {
                uePara.rlSetupNeeded = TRUE;
                if (DEBUG_HSDPA)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_RRC);
                    printf("\n\t need to request UE release HS-DPCCH\n");
                }
            }
        }
        MEM_free(*ulRbMapPos);
        ueRrc->rbMappingInfo->erase(ulRbMapPos);

        ueRrc->ulDtchBitSet->reset(uePara.ulDtchId);
        UmtsUlDchInfo* ulDch = ueRrc->ulDchInfo[(int)uePara.ulDchId];
        if (ulDch)
        {
            UmtsClearBit<UInt32>(ulDch->rbBitSet,
                                 uePara.rbId);
            if (ulDch->rbBitSet == 0)
            {
                MEM_free(ulDch);
                ueRrc->ulDchInfo[(int)uePara.ulDchId] = NULL;
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncConfigNodebForRbRelease
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Check nodeb
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc          : UmtsRrcUeDataInRnc*    : UE RRC data in RNC
// + rabId          : char                   : RAB Id
// +initNodebPara   : const UmtsNodebRbSetupPara& : RB setup para
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncConfigNodebForRbRelease(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    UmtsRrcUeDataInRnc*         ueRrc,
    char                        rabId,
    const UmtsNodebRbSetupPara& initNodebPara)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcPendingRabInfo* pendingRab = ueRrc->pendingRab[(int)rabId];

    ERROR_Assert (pendingRab, "Cannot find pending RAB info. "
                    "no cell radio resource info.");

    std::list<UmtsMntNodebInfo*>::iterator mntIt;
    for (mntIt = ueRrc->mntNodebList->begin();
         mntIt != ueRrc->mntNodebList->end();
         ++mntIt)
    {
        //if cell state is in transition from monitior to active
        // block the RAB setup utill the transistion is finished
        if ((*mntIt)->cellStatus != UMTS_CELL_ACTIVE_SET &&
            (*mntIt)->cellStatus != UMTS_CELL_MONIT_TO_ACT_SET)
        {
            continue;
        }

        //If there is no RB setup response information of 
        // the cell in the pending RAB,
        //then no need to release any radio resource for this cell
        if (pendingRab->nodebResList->end() ==
            find_if(pendingRab->nodebResList->begin(),
                    pendingRab->nodebResList->end(),
                    UmtsFindItemByCellId<UmtsRrcNodebResInfo>((*mntIt)->cellId)))
        {
            continue;
        }

        UmtsCellCacheMap::iterator nodebIt =
            rncL3->cellCacheMap->find((*mntIt)->cellId);
        ERROR_Assert(nodebIt != rncL3->cellCacheMap->end(),
            "cannot find the NodeB info in the monitored set.");
        if (nodebIt->second->rncId == node->nodeId)
        {
            UmtsRrcNodebDataInRnc* nodebRrc
                = UmtsLayer3RncFindNodebRrc(
                        node,
                        rncL3,
                        nodebIt->second->nodebId);
            ERROR_Assert(nodebRrc,
                "cannot find the NodeB RRC data in the RNC.");

            UmtsNodebRbSetupPara nodebPara = initNodebPara;
            nodebPara.cellId = (*mntIt)->cellId;
            if (TRUE == UmtsLayer3RncReleaseNodebConfigForRbRelease(
                    node,
                    umtsL3,
                    nodebRrc,
                    nodebPara))
            {
                UmtsLayer3RncSendRbReleaseToNodeb(
                    node,
                    umtsL3,
                    nodebRrc,
                    nodebPara);
            }
        }
        //send a message to DRNC to process the request
        else
        {
            UmtsNodebRbSetupPara nodebPara = initNodebPara;
            nodebPara.cellId = (*mntIt)->cellId;

            Message* msg = MESSAGE_Alloc(
                                node, NETWORK_LAYER, 
                                MAC_PROTOCOL_CELLULAR, 0);
            MESSAGE_PacketAlloc(node,
                                msg,
                                sizeof(nodebPara),
                                TRACE_UMTS_LAYER3);
            memcpy(MESSAGE_ReturnPacket(msg),
                   &nodebPara,
                   sizeof(nodebPara));

            UmtsLayer3RncSendIurMsg(
                node,
                umtsL3,
                msg,
                ueRrc->ueId,
                nodebIt->second->rncId,
                UMTS_RNSAP_MSG_TYPE_RADIO_BEARER_RELEASE_REQUEST,
                (char*)&(*mntIt)->cellId,
                sizeof(UInt32));
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleNodebRbSetupRes
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handle RB setup response from one Nodeb, it may
//               come indirectly from a Nodeb through a drifting RNC
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId           : NodeId                 : The ue id originating the msg
// + rabId          : char                   : RAB ID
// + rbId           : char                   : RB ID
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncHandleNodebRbSetupRes(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    BOOL                        success,
    const UmtsNodebRbSetupPara& nodebPara)
{
    NodeId ueId = nodebPara.ueId;
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (DEBUG_ASSERT)
    {
        ERROR_Assert(ueRrc, "cannot find the UE RRC data in the RNC.");
    }
    else if (!ueRrc)
    {
        return;
    }
    char rbId = nodebPara.rbId;
    char rabId = ueRrc->rbRabId[rbId - UMTS_RBINRAB_START_ID];
    UmtsRrcPendingRabInfo* pendingRab = ueRrc->pendingRab[(int)rabId];
    ERROR_Assert(pendingRab, "cannot find the pending RAB info.");

    pendingRab->numNodebRes++;

    //If a cell can allocate radio resources for the RAB, 
    //record its allocated resource
    if (success)
    {
        if (pendingRab->nodebResList->end() ==
                std::find_if(pendingRab->nodebResList->begin(),
                             pendingRab->nodebResList->end(),
                             UmtsFindItemByCellId
                                <UmtsRrcNodebResInfo>(nodebPara.cellId)))
        {
            UmtsRrcNodebResInfo* nodebRes = new UmtsRrcNodebResInfo();
            nodebRes->cellId = nodebPara.cellId;
            nodebRes->numDlDpdch = nodebPara.numDpdch;
            memcpy(&nodebRes->sfFactor,
                   &nodebPara.sfFactor,
                   nodebRes->numDlDpdch * sizeof(UmtsSpreadFactor));
            memcpy(&nodebRes->chCode,
                   &nodebPara.chCode,
                   nodebRes->numDlDpdch * sizeof(unsigned int));

            pendingRab->nodebResList->push_back(nodebRes);
        }
    }

    int activeSetSize = (int)std::count_if(
                               ueRrc->mntNodebList->begin(),
                               ueRrc->mntNodebList->end(),
                               UmtsIsCellInActiveSet<UmtsMntNodebInfo>());
    //If responses from all active set members have arrived
    if (pendingRab->numNodebRes == activeSetSize)
    {
        //If no cell can allocate radio resource or 
        //the primary cell cannot allocate
        //radio resource, report RAB assignment failure
        if (pendingRab->nodebResList->size() == 0 )
        {
            //RESPONSE to RAB Setup Request to report fail to setup RAB.
            UmtsLayer3RncRespToRabAssgt(
                node,
                umtsL3,
                ueId,
                FALSE,
                ueRrc->rabInfos[(int)rabId]->cnDomainId,
                &(ueRrc->rabInfos[(int)rabId]->connMap));

            //Remove prestored UE RB configuration
            UmtsLayer3RncReleaseUeConfigForRbRelease(
                    node,
                    umtsL3,
                    ueRrc,
                    rabId);

            //Remove RAB data structure
            UmtsLayer3RncRemoveRab(node, ueRrc, rabId);

            UmtsLayer3RncEndRabAssgtCheckQueue(node, umtsL3, rncL3);
        }
        else if (pendingRab->nodebResList->end() ==
                 find_if(pendingRab->nodebResList->begin(),
                         pendingRab->nodebResList->end(),
                         UmtsFindItemByCellId<UmtsRrcNodebResInfo>(
                             ueRrc->primCellId)))
        {
            //Release allocated resource in some cells
            UmtsUeRbSetupPara& uePara =
                ueRrc->pendingRab[(int)rabId]->uePara;
            UmtsTransportFormat transFormat;
            UmtsNodebRbSetupPara nodebPara(node->nodeId,
                                           ueId,
                                           uePara.rbId,
                                           uePara.rlcType,
                                           uePara.rlcConfig,
                                           transFormat,
                                           ueRrc->rabInfos[(int)rabId]->
                                                 ulRabPara.maxBitRate,
                                           ueRrc->rabInfos[(int)rabId]->
                                                 dlRabPara.maxBitRate);
            nodebPara.uePriSc = ueRrc->primScCode;
            UmtsLayer3RncConfigNodebForRbRelease(
                    node,
                    umtsL3,
                    ueRrc,
                    rabId,
                    nodebPara);

            //RESPONSE to RAB Setup Request to report fail to setup RAB.
            UmtsLayer3RncRespToRabAssgt(
                node,
                umtsL3,
                ueId,
                FALSE,
                ueRrc->rabInfos[(int)rabId]->cnDomainId,
                &(ueRrc->rabInfos[(int)rabId]->connMap));

            //Remove prestored UE RB configuration
            UmtsLayer3RncReleaseUeConfigForRbRelease(
                    node,
                    umtsL3,
                    ueRrc,
                    rabId);

            //Remove RAB data structure
            UmtsLayer3RncRemoveRab(node, ueRrc, rabId);
            UmtsLayer3RncEndRabAssgtCheckQueue(node, umtsL3, rncL3);
        }
        //Otherwise, if all cell accept the allocation of the RAB
        else if (pendingRab->nodebResList->size() ==
                 (unsigned)(pendingRab->numNodebRes))
        {
            if (ueRrc->rabInfos[(int)rabId]->state ==
                    UMTS_RAB_STATE_WAITFORNODEBSETUP)
            {
                UmtsLayer3RncSendRbSetupToUe(
                        node,
                        umtsL3,
                        ueRrc,
                        pendingRab,
                        rabId);
                ueRrc->rabInfos[(int)rabId]->state =
                    UMTS_RAB_STATE_WAITFORUESETUP;
            }

            //This case should be impossible since a RAB can 
            //only be scheduled to released
            //after
        }
        //This case, some cells cannot allocate radio resource while
        //other cells (include primary cell) can allocate radio
        //Simple stratege implemented here is to reject RAB assignment.
        //In the future, do a active set removing for each cell unable to
        //allocate radio resource.
        else
        {
            //Release allocated resource in some cells
            UmtsUeRbSetupPara& uePara =
                ueRrc->pendingRab[(int)rabId]->uePara;
            UmtsTransportFormat transFormat;
            UmtsNodebRbSetupPara nodebPara(node->nodeId,
                                           ueId,
                                           uePara.rbId,
                                           uePara.rlcType,
                                           uePara.rlcConfig,
                                           transFormat,
                                           ueRrc->rabInfos[(int)rabId]->
                                                     ulRabPara.maxBitRate,
                                           ueRrc->rabInfos[(int)rabId]->
                                                     dlRabPara.maxBitRate);
            nodebPara.uePriSc = ueRrc->primScCode;
            UmtsLayer3RncConfigNodebForRbRelease(
                    node,
                    umtsL3,
                    ueRrc,
                    rabId,
                    nodebPara);

            //RESPONSE to RAB Setup Request to report fail to setup RAB.
            UmtsLayer3RncRespToRabAssgt(
                node,
                umtsL3,
                ueId,
                FALSE,
                ueRrc->rabInfos[(int)rabId]->cnDomainId,
                &(ueRrc->rabInfos[(int)rabId]->connMap));

            //Remove prestored UE RB configuration
            UmtsLayer3RncReleaseUeConfigForRbRelease(
                    node,
                    umtsL3,
                    ueRrc,
                    rabId);

            //Remove RAB data structure
            UmtsLayer3RncRemoveRab(node, ueRrc, rabId);
            UmtsLayer3RncEndRabAssgtCheckQueue(node, umtsL3, rncL3);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncConfigNodebForRbSetup
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Check nodeb
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc          : UmtsRrcUeDataInRnc*    : UE RRC data in RNC
// + initNodebPara  : const UmtsNodebRbSetupPara& : init Rb setup para
// + ulRabPara      : const UmtsRABServiceInfo*   : UL RAB info
// + dlRabPara      : const UmtsRABServiceInfo*   : DL RAB info
// RETURN     :: BOOL : TRUE if resource can be allocated in the nodeB
// **/
static
BOOL UmtsLayer3RncConfigNodebForRbSetup(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    UmtsRrcUeDataInRnc*         ueRrc,
    const UmtsNodebRbSetupPara& initNodebPara,
    const UmtsRABServiceInfo*   ulRabPara,
    const UmtsRABServiceInfo*   dlRabPara)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    std::list<UmtsMntNodebInfo*>::iterator mntIt;
    for (mntIt = ueRrc->mntNodebList->begin();
         mntIt != ueRrc->mntNodebList->end();
         ++mntIt)
    {
        //If cell state is in transition from monitior to active
        // block the RAB setup utill the transition is finished
        if ((*mntIt)->cellStatus != UMTS_CELL_ACTIVE_SET)
        {
            continue;
        }
        UmtsCellCacheMap::iterator nodebIt =
            rncL3->cellCacheMap->find((*mntIt)->cellId);
        ERROR_Assert(nodebIt != rncL3->cellCacheMap->end(),
            "cannot find the NodeB info in the monitored set.");
        if (nodebIt->second->rncId == node->nodeId)
        {
            UmtsRrcNodebDataInRnc* nodebRrc
                = UmtsLayer3RncFindNodebRrc(
                        node,
                        rncL3,
                        nodebIt->second->nodebId);
            ERROR_Assert(nodebRrc,
                "cannot find the NodeB RRC data in the RNC.");

            UmtsNodebRbSetupPara nodebPara = initNodebPara;
            nodebPara.cellId = (*mntIt)->cellId;
            if (FALSE == UmtsLayer3RncCheckNodebForRbSetup(
                            node,
                            umtsL3,
                            nodebRrc,
                            dlRabPara,
                            &nodebPara))
            {
                UmtsLayer3RncHandleNodebRbSetupRes(
                            node,
                            umtsL3,
                            FALSE,
                            nodebPara);
            }
            else
            {
                // TODO
                nodebPara.dlBwAlloc = dlRabPara->maxBitRate;
                nodebPara.ulBwAlloc = ulRabPara->maxBitRate;

                UmtsLayer3RncStoreNodebConfigForRbSetup(
                    node,
                    umtsL3,
                    nodebRrc,
                    nodebPara,
                    dlRabPara);

                // HSDPA
                if (nodebPara.hsdpaRb)
                {
                    nodebRrc->hsdpaInfo.numHsdpaRb ++;
                }
                //Send Radio Bearer Setup Message to the NodeB
                UmtsLayer3RncSendRbSetupToNodeb(node,
                                                umtsL3,
                                                nodebRrc,
                                                nodebPara);
            }
        }

        //send a message to DRNC to process the request
        else
        {
            UmtsNodebRbSetupPara nodebPara = initNodebPara;
            nodebPara.cellId = (*mntIt)->cellId;

            Message* msg = MESSAGE_Alloc(node, 
                                         NETWORK_LAYER, 
                                         MAC_PROTOCOL_CELLULAR,
                                         0);
            MESSAGE_PacketAlloc(
                node,
                msg,
                sizeof(nodebPara) + sizeof(UmtsRABServiceInfo),
                TRACE_UMTS_LAYER3);
            memcpy(MESSAGE_ReturnPacket(msg),
                   &nodebPara,
                   sizeof(nodebPara));
            memcpy(MESSAGE_ReturnPacket(msg) + sizeof(nodebPara),
                   dlRabPara,
                   sizeof(UmtsRABServiceInfo));

            UmtsLayer3RncSendIurMsg(
                node,
                umtsL3,
                msg,
                ueRrc->ueId,
                nodebIt->second->rncId,
                UMTS_RNSAP_MSG_TYPE_RADIO_BEARER_SETUP_REQUEST,
                (char*)&(*mntIt)->cellId,
                sizeof(UInt32));
        }
    }
    return TRUE;
}


// /**
// FUNCTION   :: UmtsLayer3RncStartToSetupRab
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Initiate Radio Access Bearer Setup process, invoked
//               when handling a RAB ESTABLISHMENT REQUEST from the SGSN
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc          : UmtsRrcUeDataInRnc*    : Pointer to the UE RRC data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncStartToSetupRab(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    UmtsRrcUeDataInRnc*         ueRrc)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    ERROR_Assert(!ueRrc->rrcRelReq.requested,
        "UmtsLayer3RncStartToSetupRab: RRC "
        "connection release has been initiated");

    //First store the RAB request information
    UmtsRncRabAssgtReqInfo* rabReq = rncL3->rabServQueue->front();
    UmtsRrcRabInfo* rabInfo = (UmtsRrcRabInfo*)
                              MEM_malloc(sizeof(UmtsRrcRabInfo));
    rabInfo->state = UMTS_RAB_STATE_ESTREQQUEUED;
    rabInfo->cnDomainId = rabReq->domain;
    for (int i = 0; i < UMTS_MAX_RB_PER_RAB; i++)
    {
        rabInfo->rbIds[i] = UMTS_INVALID_RB_ID;
    }
    memcpy(&(rabInfo->connMap),
           &(rabReq->connMap),
           sizeof(UmtsRabConnMap));
    memcpy(&(rabInfo->ulRabPara),
           &(rabReq->ulRabPara),
           sizeof(UmtsRABServiceInfo));
    memcpy(&(rabInfo->dlRabPara),
           &(rabReq->dlRabPara),
           sizeof(UmtsRABServiceInfo));
    ueRrc->rabInfos[(int)rabReq->rabId] = rabInfo;
    ERROR_Assert(ueRrc->pendingRab[(int)rabReq->rabId] == NULL,
        "UmtsLayer3RncStartToSetupRab: pending RAB info exists.");

    unsigned char ulPriority = 5;
    unsigned char dlPriority = 5;

    //Choose RLC configuration parameters according to QoS requirments
    UmtsRlcEntityType rlcType;
    UmtsRlcConfig     ueRlcConfig;
    UmtsRlcConfig     nodeBRlcConfig;
    UmtsTransportFormat ulTransFormat;
    UmtsTransportFormat dlTransFormat;
    if (!UmtsLayer3RncChooseRbParaUponQoS(
                node,
                umtsL3,
                ueRrc,
                &rabInfo->ulRabPara,
                &rabInfo->dlRabPara,
                rabInfo->cnDomainId,
                rlcType,
                ueRlcConfig,
                nodeBRlcConfig,
                ulPriority,
                dlPriority,
                ulTransFormat,
                dlTransFormat))
    {
        UmtsLayer3RncRespToRabAssgt(
                node,
                umtsL3,
                ueRrc->ueId,
                FALSE,
                rabInfo->cnDomainId,
                &rabInfo->connMap);
        UmtsLayer3RncRemoveRab(node, ueRrc, rabReq->rabId);
        UmtsLayer3RncEndRabAssgtCheckQueue(node, umtsL3, rncL3);

        return;
    }

    UmtsUeRbSetupPara uePara(rabReq->rabId,
                             rabInfo->cnDomainId,
                             rlcType,
                             ueRlcConfig,
                             ulTransFormat);

    // get the format
    uePara.logPriority = ulPriority;
    if (FALSE == UmtsLayer3RncCheckUeForRbSetup(node,
                                                umtsL3,
                                                ueRrc,
                                                &uePara,
                                                &rabInfo->ulRabPara))
    {
        UmtsLayer3RncRespToRabAssgt(
                node,
                umtsL3,
                ueRrc->ueId,
                FALSE,
                rabInfo->cnDomainId,
                &rabInfo->connMap);
        UmtsLayer3RncRemoveRab(node, ueRrc, rabReq->rabId);
        UmtsLayer3RncEndRabAssgtCheckQueue(node, umtsL3, rncL3);

        return;
    }

    //Record UE changes to reserve resources so that other
    //RB setup process won't disrupt it.
    UmtsLayer3RncPreStoreUeConfigForRbSetup(
        node,
        umtsL3,
        ueRrc,
        uePara,
        rabInfo->cnDomainId,
        &rabInfo->connMap,
        &rabInfo->ulRabPara);
    rabInfo->state = UMTS_RAB_STATE_WAITFORNODEBSETUP;

    UmtsNodebRbSetupPara initNodebPara(node->nodeId,
                                       ueRrc->ueId,
                                       uePara.rbId,
                                       rlcType,
                                       nodeBRlcConfig,
                                       dlTransFormat,
                                       rabInfo->ulRabPara.maxBitRate,
                                       0);
    initNodebPara.uePriSc = ueRrc->primScCode;
    initNodebPara.logPriority = dlPriority;
    UmtsLayer3RncConfigNodebForRbSetup(
        node,
        umtsL3,
        ueRrc,
        initNodebPara,
        &(rabInfo->ulRabPara),
        &(rabInfo->dlRabPara));
}

// /**
// FUNCTION   :: UmtsLayer3RncInitRabSetup
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Initiate Radio Access Bearer Setup process, invoked
//               when handling a RAB ESTABLISHMENT REQUEST from the SGSN
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId           : NodeId                 : The ue id originating the msg
// + rabId          : char                   : RAB ID
// + ulRabPara      : const UmtsRABServiceInfo*   : UL RAB info
// + dlRabPara      : const UmtsRABServiceInfo*   : DL RAB info
// + domain         : UmtsLayer3CnDomainId   : Domain ID
// + connMap        : UmtsRabConnMap*        : RAB connection map
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncInitRabSetup(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    NodeId                      ueId,
    char                        rabId,
    const UmtsRABServiceInfo*   ulRabPara,
    const UmtsRABServiceInfo*   dlRabPara,
    UmtsLayer3CnDomainId        domain,
    const UmtsRabConnMap*       connMap)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    ERROR_Assert(ueRrc, "cannot find the UE RRC data in the RNC.");
    ERROR_Assert(ueRrc->state == UMTS_RRC_STATE_CONNECTED,
        "UmtsLayer3RncInitRabSetup: UE is not connected. ");

    //check should've been made before the procedure is called
    ERROR_Assert(!ueRrc->rrcRelReq.requested,
        "RAB SETUP procedure shouldn't be initiated if the UE is scheduled"
        " to be released.");

    //IF no active set update process is ongoing, start to
    //setup RAB immediately
    if (ueRrc->mntNodebList->end() ==
            std::find_if(ueRrc->mntNodebList->begin(),
                         ueRrc->mntNodebList->end(),
                         UmtsIsCellInTransitSet<UmtsMntNodebInfo>()))
    {
        UmtsLayer3RncStartToSetupRab(
                    node,
                    umtsL3,
                    ueRrc);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncStartToReleaseRab
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Initiate Radio Access Bearer Release process,
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId           : NodeId                 : The ue id originating the msg
// + rabId          : char                   : RAB ID
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncStartToReleaseRab(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    UmtsRrcUeDataInRnc*         ueRrc,
    char                        rabId)
{
    UmtsRrcRabInfo* rabInfo = ueRrc->rabInfos[(int)rabId];

    if (ueRrc->rrcRelReq.requested == TRUE)
    {
        return;
    }
    ERROR_Assert(rabInfo->state == UMTS_RAB_STATE_RELREQQUEUED,
        "UmtsLayer3RncStartToReleaseRab: invalid RAB state.");

    UmtsLayer3RncReleaseUeConfigForRbRelease(
            node,
            umtsL3,
            ueRrc,
            rabId);

    UmtsLayer3RncSendRbReleaseToUe(
            node,
            umtsL3,
            ueRrc,
            rabId);
    rabInfo->state = UMTS_RAB_STATE_WAITFORUEREL;
}

// /**
// FUNCTION   :: UmtsLayer3RncInitRabRelease
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Initiate Radio Access Bearer release process
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : The ue id originating the message
// + rabId     : char                   : RAB ID
// + BOOL      : releaseUeRb            : Whether to rlease UE 
//                                        resources for this RAB
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncInitRabRelease(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    rabId)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);

    ERROR_Assert(ueRrc->state == UMTS_RRC_STATE_CONNECTED,
        "UmtsLayer3RncInitRabRelease: UE is not connected. ");

    if (ueRrc->rrcRelReq.requested == TRUE)
    {
        return;
    }
    UmtsRrcRabInfo* rabInfo = ueRrc->rabInfos[(int)rabId];
    if (DEBUG_ASSERT)
    {
        ERROR_Assert(rabInfo, "UmtsLayer3RncInitRabRelease: "
            "cannot find RAB info. ");
    }
    if (!rabInfo)
    {
        return;
    }

    ERROR_Assert(rabInfo->state == UMTS_RAB_STATE_ESTED,
        "RAB must be established before being released.");

    {
        rabInfo->state = UMTS_RAB_STATE_RELREQQUEUED;
        if (ueRrc->mntNodebList->end() ==
            std::find_if(ueRrc->mntNodebList->begin(),
                         ueRrc->mntNodebList->end(),
                         UmtsIsCellInTransitSet<UmtsMntNodebInfo>()))
        {
            UmtsLayer3RncStartToReleaseRab(
                        node,
                        umtsL3,
                        ueRrc,
                        rabId);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncReleaseRrcConnInNetwork
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Release network side resource for RRC CONNECTION RELEASE
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + rncL3     : UmtsLayer3Rnc*         : RNC layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : Pointer to the UE rrc data
// RETURN     :: void
// **/
static
void UmtsLayer3RncReleaseRrcConnInNetwork(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsLayer3Rnc*          rncL3,
    UmtsRrcUeDataInRnc*     ueRrc)
{
    for (int i = 0; i < UMTS_MAX_RAB_SETUP; i++)
    {
        if (!ueRrc->rabInfos[i])
        {
           continue;
        }

        if (ueRrc->rabInfos[i]->state != UMTS_RAB_STATE_NULL ||
            ueRrc->rabInfos[i]->state !=
                UMTS_RAB_STATE_ESTREQQUEUED)
        {
            const UmtsUeRbSetupPara& uePara
                = ueRrc->pendingRab[i]->uePara;
            UmtsNodebRbSetupPara nodebPara(node->nodeId,
                                           ueRrc->ueId,
                                           uePara.rbId,
                                           uePara.rlcType,
                                           uePara.rlcConfig);
            nodebPara.ulBwAlloc = ueRrc->rabInfos[i]->ulRabPara.maxBitRate;
            nodebPara.dlBwAlloc = ueRrc->rabInfos[i]->dlRabPara.maxBitRate;

            nodebPara.uePriSc = ueRrc->primScCode;

            // hsdpa
            if (uePara.hsdpaRb)
            {
                nodebPara.hsdpaRb = TRUE;
            }

            UmtsLayer3RncConfigNodebForRbRelease(
                    node,
                    umtsL3,
                    ueRrc,
                    (char)i,
                    nodebPara);
        }
    }

    UmtsLayer3RncReqNodebToReleaseSrb(
        node,
        umtsL3,
        ueRrc);

    //Erase UE-SRNC pair in the map
    UmtsUeSrncMap::iterator mapIter = rncL3->ueSrncMap->find(ueRrc->ueId);
    ERROR_Assert(mapIter != rncL3->ueSrncMap->end(), 
                 "cannot find UE SRNC map");
    if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID)
    {
        printf("SRNC: %u remove the UE-SRNC map "
           " for UE: %u due to RRC connection release\n",
           node->nodeId, ueRrc->ueId);
    }
    MEM_free(mapIter->second);
    rncL3->ueSrncMap->erase(mapIter);

    UmtsLayer3RncRemoveUeRrc(node, ueRrc->ueId, rncL3);
}

// /**
// FUNCTION   :: UmtsLayer3RncServRabReqQueue
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Check RAB request serving queue to see if there is new
//               RAB request to be served
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + rncL3     : UmtsLayer3Rnc*         : RNC layer3 data
// RETURN     :: void
// **/
static
void UmtsLayer3RncServRabReqQueue(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsLayer3Rnc*          rncL3)
{
    if (rncL3->rabServQueue->size() > 1)
    {
#ifdef _WIN64
        rncL3->rabServQueue->sort(std::greater<UmtsRncRabAssgtReqInfo*>());
#else
        rncL3->rabServQueue->
            sort(UmtsPointedItemLess<UmtsRncRabAssgtReqInfo>());
#endif
    }

    while (!rncL3->rabServQueue->empty())
    {
        UmtsRncRabAssgtReqInfo* rabReq = rncL3->rabServQueue->front();

        UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                        node,
                                        rncL3,
                                        rabReq->ueId);
        if (!ueRrc || ueRrc->rrcRelReq.requested == TRUE
            || ueRrc->state != UMTS_RRC_STATE_CONNECTED)
        {
            UmtsLayer3RncRespToRabAssgt(
                node,
                umtsL3,
                rabReq->ueId,
                FALSE,
                rabReq->domain,
                &(rabReq->connMap));
            delete rabReq;
            rncL3->rabServQueue->pop_front();
            continue;
        }

        int rabId = UMTS_INVALID_RAB_ID;
        for (int i = 0; i < UMTS_MAX_RAB_SETUP; i++)
        {
            if (ueRrc->rabInfos[i] == NULL)
            {
                rabId = i;
                break;
            }
        }
        if (rabId != UMTS_INVALID_RAB_ID)
        {
            rabReq->rabId = (char)rabId;
            UmtsLayer3RncInitRabSetup(
                node,
                umtsL3,
                rabReq->ueId,
                (char)rabId,
                &rabReq->ulRabPara,
                &rabReq->dlRabPara,
                rabReq->domain,
                &rabReq->connMap);
            break;
        }
        else
        {
            UmtsLayer3RncRespToRabAssgt(
                    node,
                    umtsL3,
                    rabReq->ueId,
                    FALSE,
                    rabReq->domain,
                    &(rabReq->connMap));
            delete rabReq;
            rncL3->rabServQueue->pop_front();
            continue;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncEndRabAssgtCheckQueue
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Finish the current RAB assigment procedure,
//               Check RAB request serving queue to see if there is new
//               RAB request to be served
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + rncL3     : UmtsLayer3Rnc*         : RNC layer3 data
// RETURN     :: void
// **/
static
void UmtsLayer3RncEndRabAssgtCheckQueue(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsLayer3Rnc*          rncL3)
{
    delete rncL3->rabServQueue->front();
    rncL3->rabServQueue->pop_front();

    UmtsLayer3RncServRabReqQueue(node, umtsL3, rncL3);
}

// /**
// FUNCTION   :: UmtsLayer3RncRmvRabReqUponConnRel
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Remove queued RAB request upon RRC CONN release
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + rncL3    : UmtsLayer3Rnc *         : Pointer to RNC layer3 data
// + ueId     : NodeId                  : UE Id
// + curServReq: BOOL&                  : return whether this UE has 
//                                        an ongoing RAB assignment process
// RETURN     :: void
// **/
static
void UmtsLayer3RncRmvRabReqUponConnRel(
    Node*           node,
    UmtsLayer3Rnc*  rncL3,
    NodeId          ueId,
    BOOL&           curServReq)
{
    curServReq = FALSE;
    if (!rncL3->rabServQueue->empty() &&
        rncL3->rabServQueue->front()->ueId == ueId)
    {
        curServReq = TRUE;
    }
    UmtsRabAssgtReqList::iterator rabReqIter;
    for (rabReqIter = rncL3->rabServQueue->begin();
         rabReqIter != rncL3->rabServQueue->end();
         ++rabReqIter)
    {
        if ((*rabReqIter)->ueId == ueId)
        {
            delete *rabReqIter;
            *rabReqIter = NULL;
        }
    }
    rncL3->rabServQueue->remove(NULL);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendJoinActvSetToNodeb
// LAYER      :: Layer3 RRC
// PURPOSE    :: The RNC request a NodeB to join the active set
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Nodeb RRC data in RNC
// + ueId      : NodeId                 : Nodeid of NodeB
// + nodebSrbConfig : UmtsNodebSrbConfigPara& : Singalling Rb setup para
// + nodebParaVec : std::vector<UmtsNodebRbSetupPara*>& : nodeB BR setup
//                                                   parameters
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendJoinActvSetToNodeb(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    NodeId ueId,
    UmtsNodebSrbConfigPara& nodebSrbConfig,
    std::vector<UmtsNodebRbSetupPara*>& nodebParaVec)
{
    std::string msgBuf;
    msgBuf.reserve(10 + sizeof(UmtsNodebSrbConfigPara) +
                   sizeof(UmtsNodebRbSetupPara)*nodebParaVec.size());
    msgBuf.append((char*)&nodebSrbConfig, sizeof(nodebSrbConfig));
    msgBuf += (char) nodebParaVec.size();
    for (unsigned int i = 0; i < nodebParaVec.size(); i++)
    {
        msgBuf.append((char*)nodebParaVec[i],
                      sizeof(UmtsNodebRbSetupPara));
    }

    Message* msg = MESSAGE_Alloc(
                       node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf.data(),
           msgBuf.size());

    UmtsLayer3RncSendNbapMessageToNodeb(
        node,
        umtsL3,
        msg,
        UMTS_NBAP_MSG_TYPE_JOIN_ACTIVE_SET_REQUEST,
        0,
        nodebRrc->nodebId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendLeaveActvSetToNodeb
// LAYER      :: Layer3 RRC
// PURPOSE    :: The RNC request a NodeB to join the active set
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : Nodeb RRC data in RNC
// + ueId      : NodeId                 : Nodeid of NodeB
// + nodebSrbConfig : UmtsNodebSrbConfigPara& : Singalling Rb setup para
// + nodebParaVec : std::vector<UmtsNodebRbSetupPara*>& : nodeB BR setup
//                                                   parameters
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendLeaveActvSetToNodeb(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    NodeId ueId,
    UmtsNodebSrbConfigPara& nodebSrbConfig,
    std::vector<UmtsNodebRbSetupPara*>& nodebParaVec)
{
    std::string msgBuf;
    msgBuf.reserve(10 + sizeof(UmtsNodebSrbConfigPara) +
                   sizeof(UmtsNodebRbSetupPara)*nodebParaVec.size());
    msgBuf.append((char*)&nodebSrbConfig, sizeof(nodebSrbConfig));
    msgBuf += (char) nodebParaVec.size();
    for (unsigned int i = 0; i < nodebParaVec.size(); i++)
    {
        msgBuf.append((char*)nodebParaVec[i],
                      sizeof(UmtsNodebRbSetupPara));
    }

    Message* msg = MESSAGE_Alloc(
                       node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf.data(),
           msgBuf.size());

    UmtsLayer3RncSendNbapMessageToNodeb(
        node,
        umtsL3,
        msg,
        UMTS_NBAP_MSG_TYPE_LEAVE_ACTIVE_SET_REQUEST,
        0,
        nodebRrc->nodebId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3RncSendSwitchPrimCell
// LAYER      :: Layer3 RRC
// PURPOSE    :: The RNC request the NodeB cease to be the primary cell
//               or a NodeB
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + rncL3     : UmtsLayer3Rnc*         : RNC layer data
// + ueRrc     : UmtsRrcUeDataInRnc*    : UE RRC data in RNC
// + cellId    : NodeId                 : cellId of NodeB
// + enableCell : BOOL                  : enabel cell or not
// + extraInfo  : const char*           : extra information of the switch
// + extraInfosize : UInt64             : size of the extra info
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendSwitchPrimCell(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsLayer3Rnc* rncL3,
    UmtsRrcUeDataInRnc* ueRrc,
    NodeId cellId,
    BOOL   enableCell,
    const char* extraInfo = NULL,
    UInt64 extraInfoSize = 0)
{
    UmtsCellCacheInfo* cachedCell = rncL3->cellCacheMap->find(
                                        cellId)->second;
    std::string msgBuf;
    msgBuf.append((char*)&ueRrc->primScCode, sizeof(ueRrc->primScCode));
    msgBuf += (char)enableCell;

    Message* msg = 
        MESSAGE_Alloc(node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf.data(),
           msgBuf.size());

    if (enableCell)
    {
        MESSAGE_AddInfo(node,
                        msg,
                        (int)(sizeof(UmtsCellSwitchInfo) + extraInfoSize),
                        INFO_TYPE_UmtsCellSwitch);
        UmtsCellSwitchInfo* switchInfo =
            (UmtsCellSwitchInfo*)MESSAGE_ReturnInfo(
                                     msg, INFO_TYPE_UmtsCellSwitch);
        switchInfo->contextBufSize = extraInfoSize;

        memcpy((char*)switchInfo + sizeof(UmtsCellSwitchInfo),
               extraInfo,
               (size_t)(extraInfoSize));
    }

    if (cachedCell->rncId == node->nodeId)
    {
        if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID && 0)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("\n\tsend Switch Primary Cell Message for UE %d: "
                "primScCode: %u to Cell: %u \n",
                ueRrc->ueId, ueRrc->primScCode, cellId);
        }
        UmtsLayer3RncSendNbapMessageToNodeb(
            node,
            umtsL3,
            msg,
            UMTS_NBAP_MSG_TYPE_SWITCH_CELL_REQUEST,
            0,
            cachedCell->nodebId,
            ueRrc->ueId);
    }
    else
    {
        //send the message to the DRNC to forward to the
        //target cell
        if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("\n\tsend a IUR  Switch Cell Message for UE %d: "
                "to DRNC: %u \n",
                ueRrc->ueId, cachedCell->rncId);
        }

        UmtsLayer3RncSendIurMsg(
            node,
            umtsL3,
            msg,
            ueRrc->ueId,
            cachedCell->rncId,
            UMTS_RNSAP_MSG_TYPE_SWITCH_CELL_REQUEST,
            (char*)&cellId,
            sizeof(cellId));
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncRmvCellRadioLinkFromRab
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Remove radio link info of a cell from stored RAB info
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : UE RRC data in RNC
// + cellId    : UInt32                 : cellId of NodeB
// RETURN     :: void
// **/
static
void UmtsLayer3RncRmvCellRadioLinkFromRab(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    UmtsRrcUeDataInRnc*         ueRrc,
    UInt32                      cellId)
{
    for (int i = 0 ; i < UMTS_MAX_RAB_SETUP; i++)
    {
        UmtsRrcPendingRabInfo* rabInfo = ueRrc->pendingRab[i];
        if (rabInfo)
        {
            std::list<UmtsRrcNodebResInfo*>::iterator iter;
            iter = std::find_if(rabInfo->nodebResList->begin(),
                                rabInfo->nodebResList->end(),
                                UmtsFindItemByCellId
                                    <UmtsRrcNodebResInfo>(cellId));
            if (iter != rabInfo->nodebResList->end())
            {
                delete *iter;
                rabInfo->nodebResList->erase(iter);
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncAddCellRadioLinkFromRab
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Add radio link info of a cell from stored RAB info
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : UE RRC data in RNC
// + cellId    : UInt32                 : cellId of NodeB
// + cellInfo  : const char*            : cell info
// + cellifnoSize : int                 : size of cell info
// RETURN     :: void
// **/
static
void UmtsLayer3RncAddCellRadioLinkFromRab(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    UmtsRrcUeDataInRnc*         ueRrc,
    UInt32                      cellId,
    const char*                 cellInfo,
    int                         cellInfoSize)
{
    int index = 0;
    index += (sizeof(UInt32) + sizeof(UmtsSpreadFactor) + sizeof(UInt32));
    char numRbs;

    numRbs = cellInfo[index ++];
    while (index < cellInfoSize)
    {
        char rbId = cellInfo[index ++];
        char rabId = ueRrc->rbRabId[rbId - UMTS_RBINRAB_START_ID];
        UmtsRrcPendingRabInfo* pendingRab = ueRrc->pendingRab[(int)rabId];

        UmtsRrcNodebResInfo* nodebRes = new UmtsRrcNodebResInfo();
        nodebRes->cellId = cellId;
        nodebRes->numDlDpdch = cellInfo[index ++];
        for (int j = 0; j < nodebRes->numDlDpdch; j++)
        {
            memcpy(&nodebRes->sfFactor,
                   &cellInfo[index],
                   sizeof(UmtsSpreadFactor));
            index += sizeof(UmtsSpreadFactor);
            memcpy(&nodebRes->chCode,
                   &cellInfo[index],
                   sizeof(unsigned int));
            index += sizeof(unsigned int);
        }
        pendingRab->nodebResList->push_back(nodebRes);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleNodebActvSetUpRes
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handle JOIN/LEAVE ACTIVE SET RESPONSE from Nodeb, it may
//               come indirectly from a Nodeb through a drifting RNC
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + cellId         : UInt32                 : Cell Id
// + success        : BOOL                   : Join succesfully or not
// + ueId           : NodeId                 : The ue id originating the msg
// + join           : BOOL                   : join or not
// + cellinfo       : const char*            : cell info
// + cellInfoSize   : int                    : size of cell Info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncHandleNodebActvSetUpRes(
    Node*                       node,
    UmtsLayer3Data*             umtsL3,
    UInt32                      cellId,
    BOOL                        sucess,
    NodeId                      ueId,
    BOOL                        join = TRUE,
    const char*                 cellInfo = NULL,
    int                         cellInfoSize = 0)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*)(umtsL3->dataPtr);
    if (sucess)
    {
        UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                        node,
                                        rncL3,
                                        ueId);
        if (DEBUG_ASSERT)
        {
            ERROR_Assert(ueRrc, "Cannot find UE RRC data in RNC.");
        }
        else if (!ueRrc)
        {
            return;
        }
        //Add or remove channel code info in pending RAB struct
        if (join == TRUE)
        {
            UmtsLayer3RncAddCellRadioLinkFromRab(
                node,
                umtsL3,
                ueRrc,
                cellId,
                cellInfo,
                cellInfoSize);
        }
        else
        {
            UmtsLayer3RncRmvCellRadioLinkFromRab(
                node,
                umtsL3,
                ueRrc,
                cellId);
        }
        UmtsLayer3RncSendActvSetUp(
            node,
            umtsL3,
            ueId,
            join,
            cellInfo,
            cellInfoSize);
    }
    else
    {
        //TODO, how to handle the cell if it fails
        //to satisfy the request?
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncReqNodebToJoinActvSet
// LAYER      :: Layer3 RRC
// PURPOSE    :: The RNC request a NodeB to join the active set
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : NodeB data in RNC
// + ueId      : NodeId                 : Ue Id
// + nodebSrbConfig : UmtsNodebSrbConfigPara& : signallign RB setup para
// + rabParaVec  : const std::vector<UmtsRABServiceInfo*>& : RAB info
// + nodebParaVec : std::vector<UmtsNodebRbSetupPara*>& : nodeb RB setup
// RETURN     :: NULL
// **/
static
BOOL UmtsLayer3RncReqNodebToJoinActvSet(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    NodeId ueId,
    UmtsNodebSrbConfigPara& nodebSrbConfig,
    const std::vector<UmtsRABServiceInfo*>& rabParaVec,
    std::vector<UmtsNodebRbSetupPara*>& nodebParaVec)
{
    if (FALSE == UmtsLayer3RncCheckNodebForSrbSetup(node,
                                                    umtsL3,
                                                    nodebRrc,
                                                    nodebSrbConfig))
    {
        return FALSE;
    }

    //Have to store changes to decide whether can allocate resources
    //for other RBs
    UmtsLayer3RncStoreNodebConfigForSrbSetup(
        node,
        umtsL3,
        nodebRrc,
        ueId,
        nodebSrbConfig);

    int failIndex = -1;
    for (unsigned int i = 0; i < rabParaVec.size(); i++)
    {
        if (FALSE == UmtsLayer3RncCheckNodebForRbSetup(
                    node,
                    umtsL3,
                    nodebRrc,
                    rabParaVec[i],
                    nodebParaVec[i]))
        {
            failIndex = i;
            break;
        }
        else
        {
            UmtsLayer3RncStoreNodebConfigForRbSetup(
                node,
                umtsL3,
                nodebRrc,
                *nodebParaVec[i],
                rabParaVec[i]);
        }
    }

    if (failIndex != -1)
    {
        for (int i = failIndex; i >= 0; i--)
        {
            UmtsLayer3RncReleaseNodebConfigForRbRelease(
                node,
                umtsL3,
                nodebRrc,
                *nodebParaVec[i]);
        }

        UmtsLayer3RncReleaseNodebConfigForSrbRelease(
                node,
                umtsL3,
                nodebRrc,
                ueId,
                nodebSrbConfig);
        return FALSE;
    }

    //send a messages to the NODEB to add it into active set
    UmtsLayer3RncSendJoinActvSetToNodeb(
        node,
        umtsL3,
        nodebRrc,
        ueId,
        nodebSrbConfig,
        nodebParaVec);
    return TRUE;
}

// /**
// FUNCTION   :: UmtsLayer3RncReqNodebToJoinActvSetViaIur
// LAYER      :: Layer3 RRC
// PURPOSE    :: The RNC request a NodeB to join the active set through
//               IUR interface
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + cellId    : UInt32                 : Cell Id
// + drncId    : NodeId                 : Drgift RNC node Id
// + ueId      : NodeId                 : Ue Id
// + nodebSrbConfig : UmtsNodebSrbConfigPara& : signallign RB setup para
// + rabParaVec  : const std::vector<UmtsRABServiceInfo*>& : RAB info
// + nodebParaVec : std::vector<UmtsNodebRbSetupPara*>& : nodeb RB setup
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncReqNodebToJoinActvSetViaIur(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UInt32 cellId,
    NodeId drncId,
    NodeId ueId,
    UmtsNodebSrbConfigPara& nodebSrbConfig,
    const std::vector<UmtsRABServiceInfo*>& rabParaVec,
    std::vector<UmtsNodebRbSetupPara*>& nodebParaVec)
{
    int msgSize = (int)(sizeof(NodeId) + sizeof(nodebSrbConfig)
                        + rabParaVec.size()*sizeof(UmtsRABServiceInfo)
                        + nodebParaVec.size()*sizeof(UmtsNodebRbSetupPara));
    Message* msg = 
        MESSAGE_Alloc(node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        msgSize,
                        TRACE_UMTS_LAYER3);
    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);

    memcpy(&packet[index], &node->nodeId, sizeof(NodeId));
    index += sizeof(NodeId);
    memcpy(&packet[index], &nodebSrbConfig, sizeof(nodebSrbConfig));
    index += sizeof(nodebSrbConfig);
    for (unsigned int i = 0; i < nodebParaVec.size(); i++)
    {
        memcpy(&packet[index], rabParaVec[i], sizeof(UmtsRABServiceInfo));
        index += sizeof(UmtsRABServiceInfo);
        memcpy(&packet[index],
               nodebParaVec[i],
               sizeof(UmtsNodebRbSetupPara));
        index += sizeof(UmtsNodebRbSetupPara);
    }

    UmtsLayer3RncSendIurMsg(
        node,
        umtsL3,
        msg,
        ueId,
        drncId,
        UMTS_RNSAP_MSG_TYPE_JOIN_ACTIVE_SET_REQUEST,
        (char*)&cellId,
        sizeof(cellId));
}

// /**
// FUNCTION   :: UmtsLayer3RncReqNodebToLeaveActvSet
// LAYER      :: Layer3 RRC
// PURPOSE    :: The RNC request a NodeB to join the active set
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + nodebRrc  : UmtsRrcNodebDataInRnc* : NodeB data in RNC
// + ueId      : NodeId                 : Ue Id
// + nodebSrbConfig : UmtsNodebSrbConfigPara& : signallign RB setup para
// + nodebParaVec : std::vector<UmtsNodebRbSetupPara*>& : nodeb RB setup
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncReqNodebToLeaveActvSet(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcNodebDataInRnc* nodebRrc,
    NodeId ueId,
    UmtsNodebSrbConfigPara& nodebSrbConfig,
    std::vector<UmtsNodebRbSetupPara*>& nodebParaVec)
{
    for (unsigned int i = 0; i < nodebParaVec.size(); i++)
    {
        UmtsLayer3RncReleaseNodebConfigForRbRelease(
                    node,
                    umtsL3,
                    nodebRrc,
                    *(nodebParaVec[i]));
    }
    UmtsLayer3RncReleaseNodebConfigForSrbRelease(
        node,
        umtsL3,
        nodebRrc,
        ueId,
        nodebSrbConfig);

    //send a messages to the NODEB to remove it from the active set
    UmtsLayer3RncSendLeaveActvSetToNodeb(
        node,
        umtsL3,
        nodebRrc,
        ueId,
        nodebSrbConfig,
        nodebParaVec);
}

// /**
// FUNCTION   :: UmtsLayer3RncReqNodebToLeaveActvSetViaIur
// LAYER      :: Layer3 RRC
// PURPOSE    :: The RNC request a NodeB to join the active set
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + cellId    : UInt32                 : Cell Id
// + drncId    : NodeId                 : Drgift RNC node Id
// + ueId      : NodeId                 : Ue Id
// + nodebSrbConfig : UmtsNodebSrbConfigPara& : signallign RB setup para
// + nodebParaVec : std::vector<UmtsNodebRbSetupPara*>& : nodeb RB setup
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncReqNodebToLeaveActvSetViaIur(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UInt32 cellId,
    NodeId drncId,
    NodeId ueId,
    UmtsNodebSrbConfigPara& nodebSrbConfig,
    std::vector<UmtsNodebRbSetupPara*>& nodebParaVec)
{
    Message* msg = 
        MESSAGE_Alloc(node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    int msgSize = (int)(sizeof(nodebSrbConfig)
                    + nodebParaVec.size()*sizeof(UmtsNodebRbSetupPara));
    MESSAGE_PacketAlloc(node,
                        msg,
                        msgSize,
                        TRACE_UMTS_LAYER3);
    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);

    memcpy(&packet[index], &nodebSrbConfig, sizeof(nodebSrbConfig));
    index += sizeof(nodebSrbConfig);
    for (unsigned int i = 0; i < nodebParaVec.size(); i++)
    {
        memcpy(&packet[index],
               nodebParaVec[i],
               sizeof(UmtsNodebRbSetupPara));
        index += sizeof(UmtsNodebRbSetupPara);
    }

    UmtsLayer3RncSendIurMsg(
        node,
        umtsL3,
        msg,
        ueId,
        drncId,
        UMTS_RNSAP_MSG_TYPE_LEAVE_ACTIVE_SET_REQUEST,
        (char*)&cellId,
        sizeof(cellId));
}

// /**
// FUNCTION   :: UmtsLayer3RncAtmptAddActvSet
// LAYER      :: Layer3 RRC
// PURPOSE    :: Attempt to add a nodeb into the active set
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : The UE RRC data in RNC
// + nodebInfo : UmtsMntNodebInfo*      : The monitored nodeb info
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncAtmptAddActvSet(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    UmtsMntNodebInfo*   nodebInfo)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsCellCacheMap::iterator nodebIt =
        rncL3->cellCacheMap->find(nodebInfo->cellId);

    if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\t starts to add Nodeb: %d CellID %d into the active"
           " set of UE %d\n",
           nodebIt->second->nodebId, nodebInfo->cellId, ueRrc->ueId);
    }

    if (ueRrc->rrcRelReq.requested == TRUE)
    {
        return;
    }

    UmtsNodebSrbConfigPara nodebSrbConfig = UmtsNodebSrbConfigPara();
    nodebSrbConfig.uePriSc = ueRrc->primScCode;

    std::vector<UmtsNodebRbSetupPara*> nodebParaVec;
    std::vector<UmtsRABServiceInfo*>   rabParaVec;
    for (int i = 0; i < UMTS_MAX_RAB_SETUP; i++)
    {
        if (ueRrc->rabInfos[i] != NULL &&
                ueRrc->rabInfos[i]->state == UMTS_RAB_STATE_ESTED)
        {
            rabParaVec.push_back(
                new UmtsRABServiceInfo(ueRrc->rabInfos[i]->dlRabPara));

            UmtsRlcEntityType rlcType;
            UmtsRlcConfig ueRlcConfig;
            UmtsRlcConfig nodeBRlcConfig;
            unsigned char ulLogPriority =
                (unsigned char)ueRrc->rabInfos[i]->ulRabPara.trafficClass;
            unsigned char dlLogPriority =
                (unsigned char)ueRrc->rabInfos[i]->dlRabPara.trafficClass;
            UmtsTransportFormat ulTransFormat;
            UmtsTransportFormat dlTransFormat;

            if (!UmtsLayer3RncChooseRbParaUponQoS(
                node,
                umtsL3,
                ueRrc,
                &(ueRrc->rabInfos[i]->ulRabPara),
                &(ueRrc->rabInfos[i]->dlRabPara),
                ueRrc->rabInfos[i]->cnDomainId,
                rlcType,
                ueRlcConfig,
                nodeBRlcConfig,
                ulLogPriority,
                dlLogPriority,
                ulTransFormat,
                dlTransFormat))
            {
                // send  reject notificaiton
                UmtsLayer3RncHandleNodebActvSetUpRes(
                    node,
                    umtsL3,
                    nodebInfo->cellId,
                    FALSE,
                    ueRrc->ueId);

                return;
            }

            UmtsNodebRbSetupPara* nodebPara =
                    new UmtsNodebRbSetupPara(
                        node->nodeId,
                        ueRrc->ueId,
                        ueRrc->rabInfos[i]->rbIds[0],
                        rlcType,
                        nodeBRlcConfig,
                        dlTransFormat,
                        ueRrc->rabInfos[i]->ulRabPara.maxBitRate,
                        ueRrc->rabInfos[i]->dlRabPara.maxBitRate);
            nodebPara->logPriority = dlLogPriority;     
            nodebPara->uePriSc = ueRrc->primScCode;
            nodebParaVec.push_back(nodebPara);
        }
    }

    if (nodebIt == rncL3->cellCacheMap->end() )
    {
        UmtsLayer3RncInitCellLookup(node,
                                    umtsL3,
                                    nodebInfo->cellId);
        //Send a message to SGSN to request operation
        //OR just report failure
        UmtsLayer3RncHandleNodebActvSetUpRes(
                node,
                umtsL3,
                nodebInfo->cellId,
                FALSE,
                ueRrc->ueId);
    }
    else if (nodebIt->second->rncId != node->nodeId)
    {
        UmtsLayer3RncReqNodebToJoinActvSetViaIur(
                node,
                umtsL3,
                nodebInfo->cellId,
                nodebIt->second->rncId,
                ueRrc->ueId,
                nodebSrbConfig,
                rabParaVec,
                nodebParaVec);
        nodebInfo->cellStatus = UMTS_CELL_MONIT_TO_ACT_SET;
    }
    else
    {
        UmtsRrcNodebDataInRnc* nodebRrc =
                        UmtsLayer3RncFindNodebRrc(
                            node,
                            rncL3,
                            nodebIt->second->nodebId);
        if (FALSE == UmtsLayer3RncReqNodebToJoinActvSet(
                        node,
                        umtsL3,
                        nodebRrc,
                        ueRrc->ueId,
                        nodebSrbConfig,
                        rabParaVec,
                        nodebParaVec))
        {
            UmtsLayer3RncHandleNodebActvSetUpRes(
                    node,
                    umtsL3,
                    nodebInfo->cellId,
                    FALSE,
                    ueRrc->ueId);
        }
        else
        {
            nodebInfo->cellStatus = UMTS_CELL_MONIT_TO_ACT_SET;
        }
    }

    for (unsigned int i = 0; i < nodebParaVec.size(); i++)
    {
        delete nodebParaVec[i];
        delete rabParaVec[i];
    }

}

// /**
// FUNCTION   :: UmtsLayer3RncAtmptRmvActvSet
// LAYER      :: Layer3 RRC
// PURPOSE    :: Attempt to remove a nodeb into the active set
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : The UE RRC data in RNC
// + nodebInfo : UmtsMntNodebInfo*      : The monitored nodeb info
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncAtmptRmvActvSet(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    UmtsMntNodebInfo*   nodebInfo)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsCellCacheMap::iterator nodebIt =
        rncL3->cellCacheMap->find(nodebInfo->cellId);

    //If cached cell info hasn't been found, then the cache lookup
    //is not replied yet, wait for the another chance of adding
    //the cell into the active set
    if (nodebIt == rncL3->cellCacheMap->end())
    {
        return;
    }

    if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tstarts to remove Nodeb: %d CellID %d int the active"
           " set of UE %d\n",
           nodebIt->second->nodebId, nodebInfo->cellId, ueRrc->ueId);
    }

    if (ueRrc->rrcRelReq.requested == TRUE
        || nodebInfo->cellStatus != UMTS_CELL_ACTIVE_SET)
    {
        return;
    }

    UmtsNodebSrbConfigPara nodebSrbConfig;
    nodebSrbConfig.uePriSc = ueRrc->primScCode;

    std::vector<UmtsNodebRbSetupPara*> nodebParaVec;
    for (int i = 0; i < UMTS_MAX_RAB_SETUP; i++)
    {
        if (ueRrc->rabInfos[i] != NULL &&
                ueRrc->rabInfos[i]->state == UMTS_RAB_STATE_ESTED)
        {
            UmtsNodebRbSetupPara* nodebPara =
                    new UmtsNodebRbSetupPara();
            memset(nodebPara, 0, sizeof(UmtsNodebRbSetupPara));
            nodebPara->srncId = node->nodeId;
            nodebPara->ueId = ueRrc->ueId;
            nodebPara->uePriSc= ueRrc->primScCode;
            nodebPara->rbId = ueRrc->rabInfos[i]->rbIds[0];

            nodebPara->ulBwAlloc = ueRrc->rabInfos[i]->ulRabPara.maxBitRate;
            nodebPara->dlBwAlloc = ueRrc->rabInfos[i]->dlRabPara.maxBitRate;

            nodebParaVec.push_back(nodebPara);
        }
    }

    if (nodebIt->second->rncId != node->nodeId)
    {
        //send a message to the RNC to request operation
        UmtsLayer3RncReqNodebToLeaveActvSetViaIur(
            node,
            umtsL3,
            nodebInfo->cellId,
            nodebIt->second->rncId,
            ueRrc->ueId,
            nodebSrbConfig,
            nodebParaVec);
        nodebInfo->cellStatus = UMTS_CELL_ACT_TO_MONIT_SET;
    }
    else
    {
        UmtsRrcNodebDataInRnc* nodebRrc =
                        UmtsLayer3RncFindNodebRrc(
                            node,
                            rncL3,
                            nodebIt->second->nodebId);
        UmtsLayer3RncReqNodebToLeaveActvSet(
            node,
            umtsL3,
            nodebRrc,
            ueRrc->ueId,
            nodebSrbConfig,
            nodebParaVec);
        nodebInfo->cellStatus = UMTS_CELL_ACT_TO_MONIT_SET;
    }

    for (unsigned int i = 0; i < nodebParaVec.size(); i++)
    {
        delete nodebParaVec[i];
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncInitPrimCellSwitch
// LAYER      :: Layer3 RRC
// PURPOSE    :: Start to switch primary nodeb
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : The UE RRC data in RNC
// + cellId    : UInt32                 : Cell Id
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncInitPrimCellSwitch(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    UInt32   cellId)
{
    //If a switching is ongoing, stop
    if (ueRrc->primCellSwitch || cellId == ueRrc->primCellId)
        return;

    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*)(umtsL3->dataPtr);
    UmtsCellCacheInfo* cellCache;
    cellCache = rncL3->cellCacheMap->
                    find(cellId)->second;

    ueRrc->primCellSwitch = TRUE;
    ueRrc->prevPrimCellId = ueRrc->primCellId;
    ueRrc->primCellId  = cellId;

    //Send a message to the prevPrimCell
    //to stop sending/receiving packets.
    UmtsLayer3RncSendSwitchPrimCell(
        node,
        umtsL3,
        rncL3,
        ueRrc,
        ueRrc->prevPrimCellId,
        FALSE);
}

// /**
// FUNCTION   :: UmtsLayer3RncCompletePrimCellSwitch
// LAYER      :: Layer3 RRC
// PURPOSE    :: Complete the Primary Cell Switch process
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueRrc     : UmtsRrcUeDataInRnc*    : UE RRC data at RNC
// + msg       : Message*               : message
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncCompletePrimCellSwitch(
    Node* node,
    UmtsLayer3Data* umtsL3,
    UmtsRrcUeDataInRnc* ueRrc,
    Message* msg)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*)(umtsL3->dataPtr);
    UmtsCellSwitchInfo* switchInfo =
        (UmtsCellSwitchInfo*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_UmtsCellSwitch);
    ERROR_Assert(switchInfo, 
        "UmtsLayer3RncCompletePrimCellSwitch: "
        "cannot get cell switch context info "
        "from switching response message.");

    //for the cell switch packet to the nodeb
    UmtsLayer3RncSendSwitchPrimCell(
        node,
        umtsL3,
        rncL3,
        ueRrc,
        ueRrc->primCellId,
        TRUE,
        (char*)switchInfo + sizeof(UmtsCellSwitchInfo),
        switchInfo->contextBufSize);

    ueRrc->primCellSwitch = FALSE;

    //Send queued packet to this NodeB
    for (std::vector<UmtsCellSwitchQuePkt*>::iterator it
            = ueRrc->cellSwitchQueue->begin();
         it != ueRrc->cellSwitchQueue->end(); ++it)
    {
        UmtsLayer3RncSendPktToUeOnDch(node,
                                      umtsL3,
                                      (*it)->msg,
                                      ueRrc->ueId,
                                      (*it)->rbId);
        delete *it;
        *it = NULL;
    }
    ueRrc->cellSwitchQueue->clear();
}


// /**
// FUNCTION   :: UmtsLayer3RncSendPaging
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Send a PAGE TYPE 1 or PAGE TYPE 2 message
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : The ue id originating the msg
// + domain    : UmtsLayer3CnDomainId   : Domain ID
// + cause     : UmtsPagingCause        : Paging cause
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncSendPaging(
             Node* node,
             UmtsLayer3Data *umtsL3,
             NodeId ueId,
             UmtsLayer3CnDomainId domain,
             UmtsPagingCause cause)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*)(umtsL3->dataPtr);

    UmtsRRMessageType pagingMsgType = UMTS_RR_MSG_TYPE_PAGING_TYPE1;
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (ueRrc && ueRrc->state == UMTS_RRC_STATE_CONNECTED
        && ueRrc->subState == UMTS_RRC_SUBSTATE_CELL_DCH)
    {
        pagingMsgType = UMTS_RR_MSG_TYPE_PAGING_TYPE2;
    }

    int packetSize = sizeof(ueId) + 2;
    Message* msg = 
        MESSAGE_Alloc(node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        packetSize,
                        TRACE_UMTS_LAYER3);

    char* packet = MESSAGE_ReturnPacket(msg);
    memcpy(packet, &ueId, sizeof(ueId));
    packet += sizeof(ueId);
    (*packet++) = (char) domain;
    (*packet) = (char) cause;

    //Add a layer3 Header for RRC
    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)pagingMsgType);


    if (pagingMsgType == UMTS_RR_MSG_TYPE_PAGING_TYPE1)
    {
        ListItem* item = rncL3->nodebList->first;
        while (item)
        {
            Message* dupMsg = MESSAGE_Duplicate(node, msg);
            UmtsLayer3NodebInfo* nodebInfo =
                (UmtsLayer3NodebInfo*)item->data;
            NodeId nodebId = nodebInfo->nodeId;
            UmtsLayer3RncSendRrcMessageThroughNodeb(
                node,
                umtsL3,
                dupMsg,
                ueId,
                UMTS_PCCH_RB_ID,
                nodebId);
            item = item->next;
        }
        MESSAGE_Free(node, msg);
    }
    else
    {
        UmtsLayer3RncSendPktToUeOnDch(
            node,
            umtsL3,
            msg,
            ueId,
            UMTS_SRB2_ID);
    }
}

//--------------------------------------------------------------------------
// Handle msg/cmd FUNCTIONs
//--------------------------------------------------------------------------
// handle Timers
// /**
// FUNCTION   :: UmtsLayer3RncHandleT3101
// LAYER      :: Layer 3
// PURPOSE    :: Handle a timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleT3101(Node *node,
                              UmtsLayer3Data *umtsL3,
                              Message *msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    NodeId ueId;
    memcpy(&ueId,
           MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer),
           sizeof(ueId));
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (ueRrc)
    {
        if (DEBUG_TIMER)
        {
            char timeStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
            printf("%s:\t, RNC: %u receives a T3101 "
                "timer message for UE: %u\n",
                timeStr, node->nodeId, ueId);
        }
        //T3101 being expired means RRC CONNECTION setup fails

        UmtsNodebSrbConfigPara nodebPara;
        nodebPara.uePriSc = ueRrc->primScCode;
        UmtsLayer3RncSendRrcReleaseToNodeb(
            node,
            umtsL3,
            ueId,
            ueRrc->primCellId,
            nodebPara);

        UmtsLayer3RncRemoveUeRrc(node, ueId, rncL3);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleTimerMeasRepCheck
// LAYER      :: Layer 3
// PURPOSE    :: Handle a timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleTimerMeasRepCheck(Node *node,
                                          UmtsLayer3Data *umtsL3,
                                          Message *msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    NodeId ueId;
    memcpy(&ueId,
           MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer),
           sizeof(ueId));
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);

    if (ueRrc)
    {
        if (ueRrc->state != UMTS_RRC_STATE_CONNECTED
             || ueRrc->subState != UMTS_RRC_SUBSTATE_CELL_DCH)
        {
            ueRrc->timerMeasureCheck = NULL;
        }
        else if (ueRrc->tsLastMeasRep + UMTS_MEASREP_EXP_TRH
                < node->getNodeTime())
        {
            ueRrc->timerMeasureCheck = NULL;
            if (DEBUG_RR_RELEASE && DEBUG_UE_ID == ueRrc->ueId)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\tRelease RRC CONNECTION due to link failure "
                    "with UE %d\n", ueRrc->ueId);
            }

            for (int i = 0; i <  UMTS_MAX_CN_DOMAIN; i++)
            {
                UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId) i;
                //if (ueRrc->signalConn[domain])
                //{
                    UmtsLayer3RncReportIuRelease(
                        node,
                        umtsL3,
                        ueRrc->ueId,
                        domain,
                        FALSE);
                //}
            }
            UmtsLayer3RncReleaseRrcConnInNetwork(
                node,
                umtsL3,
                rncL3,
                ueRrc);
        }
        else
        {
            ueRrc->timerMeasureCheck =
                                UmtsLayer3SetTimer(
                                        node,
                                        umtsL3,
                                        UMTS_LAYER3_TIMER_MEASREPCHECK,
                                        UMTS_LAYER3_TIMER_MEASREPCHECK_VAL,
                                        NULL,
                                        &ueId,
                                        sizeof(ueId));
        }
    }
}

//
// handle RRC Msg
//
// /**
// FUNCTION   :: UmtsLayer3RncHandleRrcConnReq
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RRC CONNETION REQUEST message from UE
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + interfaceIndex : int         : Interface from which packet was received
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRrcConnReq(
             Node* node,
             UmtsLayer3Data *umtsL3,
             int interfaceIndex,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char* msgContent = MESSAGE_ReturnPacket(msg);
    UmtsRrcEstCause cause;
    NodeId initUeId;
    BOOL protErrIndi;
    UmtsLayer3CnDomainId domainIndi;

    int index = 0;
    cause = (UmtsRrcEstCause)(msgContent[index++]);
    memcpy(&initUeId, msgContent + index, sizeof(initUeId));
    index += sizeof(initUeId);
    protErrIndi = (BOOL)(msgContent[index++]);
    domainIndi = (UmtsLayer3CnDomainId)(msgContent[index++]);

    if (DEBUG_RR && initUeId == DEBUG_UE_ID )
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, RNC: %d received a RRC_CONNECTION_REQUEST message "
            "from UE:  %d with Domain: %d\n",
            timeStr, node->nodeId, initUeId, domainIndi);
    }

    // update stat
    rncL3->stat.numRrcConnReqRcvd ++;

    UmtsLayer3NodebInfo *nodeBInfo = NULL;
    nodeBInfo = (UmtsLayer3NodebInfo *)
              FindItem(node,
                       rncL3->nodebList,
                       sizeof(NodeId) + sizeof(Address),
                       (char *) &(interfaceIndex),
                       sizeof(int));

    UmtsRrcNodebDataInRnc* nodebRrc =
                            UmtsLayer3RncFindNodebRrc(
                                    node,
                                    rncL3,
                                    nodeBInfo->nodeId);

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    initUeId);
    if (ueRrc != NULL )
    {
        //If this RRC CONN REQUEST is a retransmitted message
        if ((ueRrc->state == UMTS_RRC_STATE_IDLE) && 
            (ueRrc->rrcRelReq.requested == FALSE))
        {
            if (ueRrc->timerT3101 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ueRrc->timerT3101);
                ueRrc->timerT3101 = NULL;
            }
            //If UE is still in the same cell
            //Resend the RRC_CONNECTION_SETUP message
            if (ueRrc->primCellId == nodebRrc->cellId)
            {
                UmtsLayer3RncSendRrcConnSetup(
                    node,
                    umtsL3,
                    ueRrc->ueId,
                    ueRrc->pendingDlDpdchInfo);

                Message* timerMsg = UmtsLayer3SetTimer(
                                        node,
                                        umtsL3,
                                        UMTS_LAYER3_TIMER_T3101,
                                        UMTS_LAYER3_TIMER_T3101_VAL,
                                        NULL,
                                        &initUeId,
                                        sizeof(NodeId));

                if (ueRrc->timerT3101)
                {
                    MESSAGE_CancelSelfMsg(node, ueRrc->timerT3101);
                    ueRrc->timerT3101 = NULL;
                }
                ueRrc->timerT3101 = timerMsg;
            }
            else
            {
                //Send message to old NodeB to release
                //radio link and transport channel
                UmtsNodebSrbConfigPara nodebPara;
                nodebPara.uePriSc = ueRrc->primScCode;
                UmtsLayer3RncSendRrcReleaseToNodeb(
                    node,
                    umtsL3,
                    ueRrc->ueId,
                    ueRrc->primCellId,
                    nodebPara);

                UmtsLayer3RncRemoveUeRrc(
                    node,
                    initUeId,
                    rncL3);

                //Send message to the new NodeB to setup
                //radio link and transport channel
                //Waiting for response
                ueRrc = UmtsLayer3RncStartConfigRrcConn(
                            node,
                            umtsL3,
                            initUeId,
                            nodebRrc);
            }
        }
        //UE RRC is in inconsistent state
        else
        {
            //If there is queued RAB assignment for the UE, remove them
            BOOL curServReq = FALSE;
            UmtsLayer3RncRmvRabReqUponConnRel(
                node, rncL3, ueRrc->ueId, curServReq);
            if (ueRrc->state == UMTS_RRC_STATE_CONNECTED)
            {
            //Release the old UE RRC CONNECTION
            for (int i = 0; i <  UMTS_MAX_CN_DOMAIN; i++)
            {
                UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId) i;
                //if (ueRrc->signalConn[domain])
                {
                    ueRrc->signalConn[domain] = FALSE;
                    UmtsLayer3RncReportIuRelease(
                        node,
                        umtsL3,
                        ueRrc->ueId,
                        domain,
                        FALSE);
                    }
                    }
                }
            UmtsLayer3RncReleaseRrcConnInNetwork(
                node,
                umtsL3,
                rncL3,
                ueRrc);
            ueRrc = NULL;

            ueRrc = UmtsLayer3RncStartConfigRrcConn(
                        node,
                        umtsL3,
                        initUeId,
                        nodebRrc);

            if (curServReq)
            {
                UmtsLayer3RncServRabReqQueue(node, umtsL3, rncL3);
            }
        }
    }
    else
    {
        //If this RNC is not a active controll RNC for this UE,
        //it can handle the RRC connection request from it.
        UmtsUeSrncMap::iterator mapIter = rncL3->ueSrncMap->find(initUeId);
        if (mapIter == rncL3->ueSrncMap->end())
        {
            UmtsLayer3RncStartConfigRrcConn(node,
                                            umtsL3,
                                            initUeId,
                                            nodebRrc);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRrcConnSetupComp
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RRC CONNETION SETUP COMPLETE message from UE
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : char         : Transaction Id
// + interfaceIndex : int   : Interface from which packet was received
// + ueId      : NodeId     : The ue id originating the message
// + rbId      : UInt8      : RB ID the message belongs to
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRrcConnSetupComp(
             Node* node,
             UmtsLayer3Data *umtsL3,
             int interfaceIndex,
             NodeId ueId,
             UInt8 rbId,
             char transctId,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char* msgContent;

    msgContent = MESSAGE_ReturnPacket(msg);

    if (DEBUG_RR  && ueId == DEBUG_UE_ID )
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, RNC: %d received a RRC_CONNECTION_SETUP_COMPLETE "
            "message from UE: %d \n",
            timeStr, node->nodeId, ueId);
    }

    // update stat
    rncL3->stat.numRrcConnSetupCompRcvd ++;

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (!ueRrc || ueRrc->state == UMTS_RRC_STATE_CONNECTED)
    {
        return;
    }

    ueRrc->state = UMTS_RRC_STATE_CONNECTED;
    ueRrc->subState = UMTS_RRC_SUBSTATE_CELL_DCH;
    if (ueRrc->timerT3101)
    {
        MESSAGE_CancelSelfMsg(node, ueRrc->timerT3101);
        ueRrc->timerT3101 = NULL;
    }

    MEM_free(ueRrc->pendingDlDpdchInfo);
    ueRrc->pendingDlDpdchInfo = NULL;

    //Link this RNC as the SRNC of this UE
    UmtsSrncInfo* srncInfo = (UmtsSrncInfo*)
                             MEM_malloc(sizeof(UmtsSrncInfo));
    srncInfo->srncId = node->nodeId;
    srncInfo->refCount = 1;
    (*rncL3->ueSrncMap)[ueId] = srncInfo;

    if (ueRrc->timerMeasureCheck)
    {
        MESSAGE_CancelSelfMsg(node, ueRrc->timerMeasureCheck);
        ueRrc->timerMeasureCheck = NULL;
    }

    ueRrc->timerMeasureCheck = UmtsLayer3SetTimer(
                                        node,
                                        umtsL3,
                                        UMTS_LAYER3_TIMER_MEASREPCHECK,
                                        UMTS_LAYER3_TIMER_MEASREPCHECK_VAL,
                                        NULL,
                                        &ueId,
                                        sizeof(ueId));
    ueRrc->tsLastMeasRep = node->getNodeTime();
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRrcConnRelComp
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RRC CONNETION RELEASE COMPLETE message from UE
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRrcConnRelComp(
             Node* node,
             UmtsLayer3Data *umtsL3,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    NodeId ueId;
    memcpy(&ueId, MESSAGE_ReturnPacket(msg), sizeof(NodeId));

    if (DEBUG_RR_RELEASE && ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\trecvd RRC CONNECTION RELEASE COMPLETE "
            " from UE %d\n", ueId);
    }
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (ueRrc)
    {
        UmtsLayer3RncReleaseRrcConnInNetwork(
                node,
                umtsL3,
                rncL3,
                ueRrc);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleInitDirectTransfer
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling a RRC Initial Direct Transfer message from UE
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + interfaceIndex : int    : Interface from which packet was received
// + ueId      : NodeId             : The ue id originating the message
// + rbId      : UInt8              : RB ID the message belongs to
// + transctId : char               : Transaction Id
// + msg       : Message*           : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleInitDirectTransfer(
             Node* node,
             UmtsLayer3Data *umtsL3,
             int interfaceIndex,
             NodeId ueId,
             UInt8 rbId,
             char transctId,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char* packet = MESSAGE_ReturnPacket(msg);
    char* nasMsg = packet + 1;
    int nasMsgSize = MESSAGE_ReturnPacketSize(msg) - 1;

    UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId)(*packet);

    if (DEBUG_RR && ueId == DEBUG_UE_ID )
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, RNC: %d received a INIT DIRECT TRANSER "
            "message from UE: %d, with domain: %d\n",
            timeStr, node->nodeId, ueId, domain);
    }

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);

    if (ueRrc && ueRrc->state == UMTS_RRC_STATE_CONNECTED)
    {
        //Signal connection is considered established when
        //downlink direct transfer has been forwarded
        //ueRrc->signalConn[domain] = TRUE;

        //Build a Initial UE RANAP message, send it to SGSN
        //TODO, add size for other information including LAC, RAC
        int packetSize = 1 + nasMsgSize;
        Message* ranapMsg = 
            MESSAGE_Alloc(node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
        MESSAGE_PacketAlloc(node,
                            ranapMsg,
                            packetSize,
                            TRACE_UMTS_LAYER3);
        char* ranapPkt = MESSAGE_ReturnPacket(ranapMsg);
        (*ranapPkt++) = (char)domain;
        //TODO, add LAC, RAC
        memcpy(ranapPkt, nasMsg, nasMsgSize);

        UmtsLayer3RncSendRanapMsgToSgsn(
            node,
            umtsL3,
            ranapMsg,
            ueId,
            UMTS_RANAP_MSG_TYPE_INITIAL_UE_MESSAGE);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleUlDirectTransfer
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling a RRC Uplink Direct Transfer message from UE
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + interfaceIndex : int   : Interface from which packet was received
// + ueId      : NodeId             : The ue id originating the message
// + rbId      : UInt8              : RB ID the message belongs to
// + transctId : char               : Transaction Id
// + msg       : Message*           : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleUlDirectTransfer(
             Node* node,
             UmtsLayer3Data *umtsL3,
             int interfaceIndex,
             NodeId ueId,
             UInt8 rbId,
             char transctId,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char* packet = MESSAGE_ReturnPacket(msg);
    char* nasMsg = packet + 1;
    int nasMsgSize = MESSAGE_ReturnPacketSize(msg) - 1;

    UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId)(*packet);
    if (DEBUG_RR && 0)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, RNC: %d received a UL DIRECT TRANSER "
            "message from UE: %d, with domain: %d\n",
            timeStr, node->nodeId, ueId, domain);
    }

    //Build a Initial UE RANAP message, send it to SGSN
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                node,
                                rncL3,
                                ueId);

    if (ueRrc && ueRrc->state == UMTS_RRC_STATE_CONNECTED)
    {
        int packetSize = 1 + nasMsgSize;
        Message* ranapMsg = 
            MESSAGE_Alloc(node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
        MESSAGE_PacketAlloc(node,
                            ranapMsg,
                            packetSize,
                            TRACE_UMTS_LAYER3);
        char* ranapPkt = MESSAGE_ReturnPacket(ranapMsg);
        (*ranapPkt++) = (char)domain;
        memcpy(ranapPkt, nasMsg, nasMsgSize);

        UmtsLayer3RncSendRanapMsgToSgsn(
            node,
            umtsL3,
            ranapMsg,
            ueId,
            UMTS_RANAP_MSG_TYPE_DIRECT_TRANSFER);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRrcStatus
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RRC CONNETION RELEASE COMPLETE message from UE
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + interfaceIndex : int         : Interface from which packet was received
// + ueId      : NodeId           : The ue id originating the message
// + rbId      : UInt8            : RB ID the message belongs to
// + msg       : Message*         : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRrcStatus(
             Node* node,
             UmtsLayer3Data *umtsL3,
             int interfaceIndex,
             NodeId ueId,
             UInt8 rbId,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char* msgContent = MESSAGE_ReturnPacket(msg);
    int index = 0;
    char transactId;

    UmtsRRMessageType rcvMsgType = (UmtsRRMessageType)
                                   msgContent[index++];
    transactId = msgContent[index++];
    UmtsRrcPtclErrCause errCause = (UmtsRrcPtclErrCause)
                                   msgContent[index++];

    if (DEBUG_RR && 0)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, RNC: %d received a RRC_STATUS message from "
            "UE: %d, with receive message type: %d, "
            "protocol error cause: %d \n",
            timeStr, node->nodeId, ueId, rcvMsgType, errCause);
    }

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (DEBUG_ASSERT)
    {
        char errorMsg[MAX_STRING_LENGTH];
        sprintf(errorMsg, "UmtsLayer3RncHandleRrcStatus: "
            " cannot find UE RRC information at RNC for UE: %d",
            ueId);
        ERROR_Assert( ueRrc != NULL, errorMsg);
    }
    else if (!ueRrc)
    {
        return;
    }

    //TODO, Handle the reported error
    switch (rcvMsgType)
    {
        case UMTS_RR_MSG_TYPE_DL_DIRECT_TRANSFER:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRbSetupCmpl
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RADIO Bearer Setup Complete message from UE
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : NodeId            : The ue id originating the message
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRbSetupCmpl(
             Node* node,
             UmtsLayer3Data *umtsL3,
             NodeId ueId,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;
    char rbId;

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (!ueRrc || ueRrc->state != UMTS_RRC_STATE_CONNECTED)
    {
        return;
    }
    rbId = packet[index ++];
    char rabId = packet[index ++];

    UmtsRrcRabInfo* rabInfo = ueRrc->rabInfos[(int)rabId];
    ERROR_Assert(rabInfo, "cannot find RAB info in the RNC.");

    if (rabInfo->state == UMTS_RAB_STATE_WAITFORUESETUP)
    {
        rabInfo->state = UMTS_RAB_STATE_ESTED;
        UmtsLayer3RncRespToRabAssgt(
                node,
                umtsL3,
                ueId,
                TRUE,
                rabInfo->cnDomainId,
                &(rabInfo->connMap),
                rabId);
        UmtsLayer3RncEndRabAssgtCheckQueue(node, umtsL3, rncL3);
    }
    else
    {
        ERROR_ReportWarning("UmtsLayer3RncHandleRbSetupCmpl: "
            "RAB is in an invalid state.");
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRbReleaseCmpl
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RADIO Bearer Release Complete message from UE
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : NodeId            : The ue id originating the message
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRbReleaseCmpl(
             Node* node,
             UmtsLayer3Data *umtsL3,
             NodeId ueId,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char* packet = MESSAGE_ReturnPacket(msg);
    char rbId;
    int index = 0;

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (!ueRrc || ueRrc->state != UMTS_RRC_STATE_CONNECTED)
    {
        return;
    }

    rbId = packet[index ++];
    char rabId = packet[index ++];

    const UmtsUeRbSetupPara& uePara = ueRrc->pendingRab[(int)rabId]->uePara;
    UmtsNodebRbSetupPara nodebPara(node->nodeId,
                                   ueId,
                                   uePara.rbId,
                                   uePara.rlcType,
                                   uePara.rlcConfig);

    nodebPara.ulBwAlloc = ueRrc->rabInfos[(int)rabId]->ulRabPara.maxBitRate;
    nodebPara.dlBwAlloc = ueRrc->rabInfos[(int)rabId]->dlRabPara.maxBitRate;

    nodebPara.uePriSc = ueRrc->primScCode;

    // HSDPA
    if (uePara.hsdpaRb)
    {
        nodebPara.hsdpaRb = TRUE;
    }
    UmtsLayer3RncConfigNodebForRbRelease(
            node,
            umtsL3,
            ueRrc,
            rabId,
            nodebPara);

    UmtsLayer3RncRemoveRab(node, ueRrc, rabId);

}

// /**
// FUNCTION    :: UmtsLayer3RncUpdateUeMonitoedCellInfo
// DESCRIPTION :: update the monitored Cell Info for UE
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + rncl3     : UmtsLayer3Rnc*   : pOinter to the RNC data
// + nodeBList : std::list<UmtsMntNodebInfo*>* : NodeB List
// + cellId    : UInt32           : The cell id of the monitored cell
// + measVal   : signed char      : measurement val
// **/
static
void UmtsLayer3RncUpdateUeMonitoedCellInfo(
         Node* node,
         UmtsLayer3Data *umtsL3,
         UmtsLayer3Rnc* rncL3,
         UmtsRrcUeDataInRnc* ueRrc,
         UInt32 cellId,
         signed char measVal)
{
     std::list<UmtsMntNodebInfo*>* mntNodeBList
                                = ueRrc->mntNodebList;
    UmtsMntNodebInfo* nodeBInfo = NULL;
    std::list<UmtsMntNodebInfo*>::iterator it;
    for (it = mntNodeBList->begin();
        it != mntNodeBList->end();
        it ++)
    {
        if ((*it)->cellId == cellId)
        {
            nodeBInfo = (*it);
            break;
        }
    }
    if (it == mntNodeBList->end())
    {
        //Start to cache the CELL info if not done yet
        UmtsCellCacheMap::iterator nodebIt =
            rncL3->cellCacheMap->find(cellId);
        if (nodebIt == rncL3->cellCacheMap->end())
        {
            UmtsLayer3RncInitCellLookup(
                node,
                umtsL3,
                cellId);
        }

        nodeBInfo = new UmtsMntNodebInfo(cellId);
        nodeBInfo->dlRscpMeas = measVal;
        nodeBInfo->timeStamp = node->getNodeTime();

        mntNodeBList->push_back(nodeBInfo);
        nodeBInfo->cellStatus = UMTS_CELL_MONITORED_SET;
        if (DEBUG_MEASUREMENT && ueRrc->ueId == DEBUG_UE_ID)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("\n\t add new Monitored NodeB, cellId: %d for UE %u\n",
                cellId, ueRrc->ueId);
        }
    }

    nodeBInfo->dlRscpMeas = measVal;
    nodeBInfo->timeStamp = node->getNodeTime();

    if (DEBUG_MEASUREMENT && ueRrc->ueId == DEBUG_UE_ID)
    {
        printf("\n\t CPICH RSCP to cellId(%d) for UE %u  is %d\n",
            cellId, ueRrc->ueId, measVal);
    }

    // remove out of dated monitor NodeB
#if 0
    mntNodeBList->remove_if(
        UmtsMeasTimeLessThan<UmtsMntNodebInfo>(
            node->getNodeTime() -
            rncL3->para.shoEvalTime));
#endif // 0
}

// /**
// FUNCTION    :: UmtsLayer3RncCheckUeHandoff
// DESCRIPTION :: update the monitored Cell Info for UE
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + rncl3     : UmtsLayer3Rnc*   : pOinter to the RNC data
// + ueRrc     : UmtsRrcUeDataInRnc* : UeInfo at RRC
// **/
static
void UmtsLayer3RncCheckUeHandoff(
         Node* node,
         UmtsLayer3Data *umtsL3,
         UmtsLayer3Rnc* rncL3,
         UmtsRrcUeDataInRnc* ueRrc)
{
    std::list<UmtsMntNodebInfo*>::iterator it;
    UmtsMntNodebInfo* bestActiveCell = NULL;
    double bestActiveSig =0;
    UmtsMntNodebInfo* worstActiveCell = NULL;
    double worstActiveSig = 0;
    UmtsMntNodebInfo* bestCandCell = NULL;
    double bestCandSig = 0;
    UmtsMntNodebInfo* primCell = NULL;

    // 1. sort the signal strength
#ifdef _WIN64
    ueRrc->mntNodebList->sort(std::greater<UmtsMntNodebInfo*>());
#else
    ueRrc->mntNodebList->sort(UmtsPointedItemLess<UmtsMntNodebInfo>());
#endif

    // 2. find the strongest signal in the active set
    //    find the worst signal in the active set
    //    find the strongest signal in the monitored set
    //    find the signal of primary cell
    for (it = ueRrc->mntNodebList->begin();
         it != ueRrc->mntNodebList->end();
         it ++)
    {
        if ((*it)->cellStatus == UMTS_CELL_ACTIVE_SET)
        {
            if (bestActiveCell == NULL)
            {
                bestActiveCell = (*it);
                bestActiveSig = (*it)->dlRscpMeas;
            }
            else if (bestActiveSig < (*it)->dlRscpMeas)
            {
                bestActiveCell = (*it);
                bestActiveSig = (*it)->dlRscpMeas;
            }
            if (worstActiveCell == NULL)
            {
                worstActiveCell = (*it);
                worstActiveSig = (*it)->dlRscpMeas;
            }
            else if (worstActiveSig > (*it)->dlRscpMeas)
            {
                worstActiveCell = (*it);
                worstActiveSig = (*it)->dlRscpMeas;
            }
        }
        else if ((*it)->cellStatus == UMTS_CELL_MONITORED_SET)
        {
            if (bestCandCell == NULL)
            {
                bestCandCell = (*it);
                bestCandSig = (*it)->dlRscpMeas;
            }
            else if (bestCandSig < (*it)->dlRscpMeas)
            {
                bestCandCell = (*it);
                bestCandSig = (*it)->dlRscpMeas;
            }
        }

        if ((*it)->cellId == ueRrc->primCellId)
        {
            primCell = *it;
        }
    }

    int activeSetSize = (int)std::count_if(
                              ueRrc->mntNodebList->begin(),
                              ueRrc->mntNodebList->end(),
                              UmtsIsCellInActiveSet<UmtsMntNodebInfo>());

    if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID && activeSetSize)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tFor UE %u best Active Cell %d & RSCP %f,"
            "\n\tworst Active Cell %d & RSCP %f,"
            "\n\tpri cell %d & RSCP %f,"
            "\n\tactiveSetSize %d, ASTh %f, AsThHyst%f\n",
            ueRrc->ueId,
            bestActiveCell->cellId, bestActiveCell->dlRscpMeas,
            worstActiveCell->cellId, worstActiveCell->dlRscpMeas,
            primCell->cellId, primCell->dlRscpMeas,
            activeSetSize, rncL3->para.shoAsTh, rncL3->para.shoAsThHyst);
        if (bestCandCell)
        {
            printf("\n\tBest Candidate cell %d & RSCP %f\n",
                bestCandCell->cellId, bestCandSig);
        }

    }

    for (it = ueRrc->mntNodebList->begin();
        it != ueRrc->mntNodebList->end();
        it ++)
    {
        // 3. If Meas_Sign of active cell is below 
        // (Best_Ss - As_Th - As_Th_Hyst)
        //    for a period of T remove Worst cell in the Active Set
        // if it is the only active set, skip the 
        // remomve operation at this point
        if (activeSetSize > 1 &&
            (*it)->cellStatus == UMTS_CELL_ACTIVE_SET &&
            (*it)->dlRscpMeas  <
                (bestActiveSig -
                (rncL3->para.shoAsTh +
                rncL3->para.shoAsThHyst)))
        {
            //If this cell is the primary cell, switch it
            if ((*it)->cellId == ueRrc->primCellId &&
                    ueRrc->primCellSwitch == FALSE)
            {
                if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_RRC);
                    printf("\n\tPriCell signal is below the "
                        "best Cell too much, switch pri cell\n");
                }
                UmtsLayer3RncInitPrimCellSwitch(
                    node,
                    umtsL3,
                    ueRrc,
                    bestActiveCell->cellId);
            }
            else if (FALSE == UmtsLayer3RncCheckUeOngoingRabProc(
                                node,
                                umtsL3,
                                ueRrc)
                && (*it)->cellId != ueRrc->primCellId
                && !(worstActiveCell->cellId == ueRrc->prevPrimCellId
                         && ueRrc->primCellSwitch == TRUE))
            {
                if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_RRC);
                    printf("\n\tOne non Pri ActiveCell signal is below the "
                        "best Cell too much, remove it from active set\n");
                }
                UmtsLayer3RncAtmptRmvActvSet(
                    node,
                    umtsL3,
                    ueRrc,
                    (*it));
            }
            break;
        }
        // 4. If Meas_Sign is greater than (Best_Ss - As_Th + As_Th_Hyst)
        // for a period of T and the Active Set is not full
        // add Best cell outside the Active Set in the Active Set
        else if (activeSetSize <= UMTS_MAX_ACTIVE_SET &&
            (*it) == bestCandCell &&
            (*it)->dlRscpMeas >
                (bestActiveSig -
                (rncL3->para.shoAsTh -
                rncL3->para.shoAsThHyst)))
        {
            // add to active set
            if (FALSE == UmtsLayer3RncCheckUeOngoingRabProc(
                                node,
                                umtsL3,
                                ueRrc))
            {
                if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_RRC);
                    printf("\n\tOne Candidate Cell signal is above the "
                        "Threshold,ActiveSet is not full, "
                        "add it to active set\n");
                }

                UmtsLayer3RncAtmptAddActvSet(
                    node,
                    umtsL3,
                    ueRrc,
                    (*it));
            }
            break;
        }

        // 5. If Active Set is full and Best_Cand_Ss is greater than
        // (Worst_Old_Ss + As_Rep_Hyst) for a period of T add Best cell
        // outside Active Set and Remove Worst cell in the Active Set.
        else if (activeSetSize == UMTS_MAX_ACTIVE_SET &&
            (*it) == bestCandCell &&
            (*it)->dlRscpMeas >
                (worstActiveSig -
                (rncL3->para.shoAsTh +
                rncL3->para.shoAsRepHyst)))
        {
            if (worstActiveCell->cellId == ueRrc->primCellId &&
                    ueRrc->primCellSwitch == FALSE)
            {
                if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_RRC);
                    printf("\n\tOne Candidate Cell signal is above the "
                        "wrst also pri cell too mcuh, active set is full,"
                        "swtich pri cell\n");
                }
                UmtsLayer3RncInitPrimCellSwitch(
                    node,
                    umtsL3,
                    ueRrc,
                    bestActiveCell->cellId);
            }
            // repalce a active cell
            // by move the worst active cell to monitor cell
            // add the best candidate cell to active set
            else if (FALSE == UmtsLayer3RncCheckUeOngoingRabProc(
                                node,
                                umtsL3,
                                ueRrc)
                    && worstActiveCell->cellId != ueRrc->primCellId
                    && !(worstActiveCell->cellId == ueRrc->prevPrimCellId
                         && ueRrc->primCellSwitch == TRUE))
            {
                if (DEBUG_HO && ueRrc->ueId == DEBUG_UE_ID)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_RRC);
                    printf("\n\tOne Candidate Cell signal is above the "
                        "worst but not pri cell too mcuh, active set "
                        " is full, remove worst active cell\n");
                }
                UmtsLayer3RncAtmptRmvActvSet(
                    node,
                    umtsL3,
                    ueRrc,
                    worstActiveCell);
            }
            break;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleMeasReportFromUe
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling MEASUREMENT REPORT message from UE
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : NodeId            : The ue id originating the message
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleMeasReportFromUe(
             Node* node,
             UmtsLayer3Data *umtsL3,
             NodeId ueId,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;
    int i = 0;
    UInt32 cellId = 0;
    signed char measVal = 0;
    UmtsLayer3MeasurementReport measReport;

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (!ueRrc || ueRrc->state != UMTS_RRC_STATE_CONNECTED)
    {
        return;
    }

    if (DEBUG_MEASUREMENT && ueRrc->ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\trecvd  MEASUREMENT RERORT from UE %d\n", ueId);
    }
    memcpy(&measReport, packet, sizeof(measReport));
    index += sizeof(UmtsLayer3MeasurementReport);

    for (i = 0; i < measReport.numMeas; i++)
    {
        memcpy(&cellId, &packet[index], sizeof(cellId));
        index += sizeof(cellId);
        measVal = (signed char)packet[index];
        index ++;

        // update the report
        UmtsLayer3RncUpdateUeMonitoedCellInfo(
            node,
            umtsL3,
            rncL3,
            ueRrc,
            cellId,
            measVal);
        if (DEBUG_MEASUREMENT && ueId == DEBUG_UE_ID)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("\n\trcvd  meas Val of CELL %d for UE %u is %d\n",
                cellId, ueRrc->ueId, measVal);
        }
    }

    ueRrc->tsLastMeasRep = node->getNodeTime();

    // determine if handoff is needed
    UmtsLayer3RncCheckUeHandoff(node, umtsL3, rncL3, ueRrc);
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleActvSetUpCompFromUe
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RADIO Bearer Setup Complete message from UE
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : NodeId            : The ue id originating the message
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleActvSetUpCompFromUe(
             Node* node,
             UmtsLayer3Data *umtsL3,
             NodeId ueId,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);

    if (!ueRrc || ueRrc->state != UMTS_RRC_STATE_CONNECTED)
    {
        return;
    }

    UInt32 cellId;
    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);
    BOOL addCell = (BOOL) packet[index++];
    memcpy(&cellId, &packet[index], sizeof(UInt32));
    index += sizeof(UInt32);

    if (DEBUG_HO && ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\trcvd  ACTIVE SET UPDATE COMPLETE from UE %d\n", ueId);
    }

    std::list<UmtsMntNodebInfo*>::iterator mntNodebIt;
    mntNodebIt = std::find_if(
                    ueRrc->mntNodebList->begin(),
                    ueRrc->mntNodebList->end(),
                    UmtsFindItemByCellId<UmtsMntNodebInfo>(cellId));
    if (addCell)
    {
        (*mntNodebIt)->cellStatus = UMTS_CELL_ACTIVE_SET;
    }
    else
    {
        (*mntNodebIt)->cellStatus = UMTS_CELL_MONITORED_SET;
    }

    //IF no active set update process is ongoing, start to
    //setup/release RAB if there are queued request
    if (ueRrc->mntNodebList->end() ==
            std::find_if(ueRrc->mntNodebList->begin(),
                         ueRrc->mntNodebList->end(),
                         UmtsIsCellInTransitSet<UmtsMntNodebInfo>()))
    {
        if (!rncL3->rabServQueue->empty() &&
            rncL3->rabServQueue->front()->ueId == ueRrc->ueId)
        {
            UmtsLayer3RncStartToSetupRab(
                    node,
                    umtsL3,
                    ueRrc);
        }
        for (int i = 0; i < UMTS_MAX_RAB_SETUP; i++)
        {
            if (!ueRrc->rabInfos[i])
                continue;
            if (ueRrc->rabInfos[i]->state
                        == UMTS_RAB_STATE_RELREQQUEUED)
            {
                //start to release RAB
                UmtsLayer3RncStartToReleaseRab(
                                node,
                                umtsL3,
                                ueRrc,
                                (char)i);
            }
            else if (ueRrc->rabInfos[i]->state
                        == UMTS_RAB_STATE_NULL)
            {
                UmtsLayer3RncRemoveRab(node, ueRrc, (char)i);
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleSigRelIndiFromUe
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling SIGNAL CONNECTION RELEASE INDICATION msg from UE
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : NodeId            : The ue id originating the message
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleSigRelIndiFromUe(
             Node* node,
             UmtsLayer3Data *umtsL3,
             NodeId ueId,
             Message* msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (!ueRrc || ueRrc->state != UMTS_RRC_STATE_CONNECTED)
    {
        return;
    }

    UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId)
                                  (*MESSAGE_ReturnPacket(msg));

    if (DEBUG_RR_RELEASE && DEBUG_UE_ID == ueId)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\trecvd SIGNAL CONNECTION RELEASE INDICATION "
            " from UE %d, domain: %d\n", ueId, domain);
    }

    if (ueRrc->signalConn[domain])
    {
        ueRrc->signalConn[domain] = FALSE;
        UmtsLayer3RncReportIuRelease(
                node,
                umtsL3,
                ueRrc->ueId,
                domain,
                FALSE);
    }

    UmtsLayer3CnDomainId cmpVal[UMTS_MAX_CN_DOMAIN];
    memset(cmpVal,
           0,
           sizeof(UmtsLayer3CnDomainId) * UMTS_MAX_CN_DOMAIN);

    if (!memcmp(ueRrc->signalConn,
               cmpVal,
               sizeof(UmtsLayer3CnDomainId) * UMTS_MAX_CN_DOMAIN))
    {
        UmtsLayer3RncInitRrcConnRelease(
            node,
            umtsL3,
            ueRrc,
            TRUE);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleAmRlcErr
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling AM RLC ERROR report
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : NodeId       : The ue id originating the message
// + cellId    : UInt32       : Cell ID
// + packet    : char*        : packet to be sent
// + packetSize : int         : size of the packet
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleAmRlcErr(
             Node* node,
             UmtsLayer3Data *umtsL3,
             NodeId ueId,
             UInt32 cellId,
             char* packet,
             int   packetSize)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);

    if (!ueRrc || ueRrc->state != UMTS_RRC_STATE_CONNECTED)
    {
        return;
    }

    //Assume the RLC ERROR is caused by the RADIO LINK failure,
    //and only AM RLC entities of the primary cell can detect the
    //error, release RRC connection by releasing all radio resources
    //of all related NodeBs.
    for (int i = 0; i <  UMTS_MAX_CN_DOMAIN; i++)
    {
        UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId) i;
        //if (ueRrc->signalConn[domain])
        {
            ueRrc->signalConn[domain] = FALSE;
            UmtsLayer3RncReportIuRelease(
                node,
                umtsL3,
                ueRrc->ueId,
                domain,
                FALSE);
        }
    }

    UmtsLayer3RncReleaseRrcConnInNetwork(
            node,
            umtsL3,
            rncL3,
            ueRrc);
}

//
// handle NBAP message
//
// /**
// FUNCTION   :: UmtsLayer3RncHandleRrcSetupResFromNodeb
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RRC CONNETION SETUP RESPONSE message from NodeB
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*     : RRC message.
// + transctId : unsigned char         : Transaction Id
// + nodebId   : NodeId       : NodeB Id
// + ueId      : NodeId       : UE ID
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRrcSetupResFromNodeb(
    Node* node,
    UmtsLayer3Data* umtsL3,
    Message* msg,
    unsigned char transctId,
    NodeId nodebId,
    NodeId ueId)
{
    if (DEBUG_NBAP )
    {
        printf("RNC: %u received a RRC SETUP RESPONSE from NodeB"
           " for UE: %u\n", node->nodeId, ueId);
    }
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (DEBUG_ASSERT)
    {
        ERROR_Assert(ueRrc, "cannot find UE RRC data in the RNC.");
    }
    else if (!ueRrc)
    {
        return;
    }
    ERROR_Assert(ueRrc->pendingDlDpdchInfo != NULL,
        "There should be a downlink DPDCH info in the pending setup list.");
    
    UmtsRcvPhChInfo* dlDpdchInfo = ueRrc->pendingDlDpdchInfo;

    ueRrc->curBwReq += 3400;

    UmtsLayer3RncSendRrcConnSetup(
        node,
        umtsL3,
        ueId,
        dlDpdchInfo);

    //If success, set a timer waiting for RRC_CONNECTION_SETUP_COMPLETE
    Message* timerMsg = UmtsLayer3SetTimer(
                            node,
                            umtsL3,
                            UMTS_LAYER3_TIMER_T3101,
                            UMTS_LAYER3_TIMER_T3101_VAL,
                            NULL,
                            &ueId,
                            sizeof(NodeId));
    if (DEBUG_TIMER)
    {
        char timeStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s:\t, RNC: %u sends a T3101 timer for UE: %u\n",
            timeStr, node->nodeId, ueId);
    }
    if (ueRrc->timerT3101)
    {
        MESSAGE_CancelSelfMsg(node, ueRrc->timerT3101);
        ueRrc->timerT3101 = NULL;
    }
    ueRrc->timerT3101 = timerMsg;
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRrcReleaseResFromNodeb
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RRC CONNETION SETUP RESPONSE message from NodeB
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*     : RRC message.
// + transctId : unsigned char         : Transaction Id
// + nodebId   : NodeId       : NodeB Id
// + ueId      : NodeId       : UE ID
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRrcReleaseResFromNodeb(
    Node* node,
    UmtsLayer3Data* umtsL3,
    Message* msg,
    unsigned char transctId,
    NodeId nodebId,
    NodeId ueId)
{
    if (DEBUG_NBAP )
    {
        printf("RNC: %u received a RRC RELEASE RESPONSE from NodeB"
           " for UE: %u\n", node->nodeId, ueId);
    }
}

//
// /**
// FUNCTION   :: UmtsLayer3RncHandleRbSetupResFromNodeb
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RADIO BEARER SETUP RESPONSE message from NodeB
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*     : RRC message.
// + transctId : unsigned char         : Transaction Id
// + nodebId   : NodeId       : NodeB Id
// + ueId      : NodeId       : UE ID
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRbSetupResFromNodeb(
    Node* node,
    UmtsLayer3Data* umtsL3,
    Message* msg,
    unsigned char transctId,
    NodeId nodebId,
    NodeId ueId)
{
    if (DEBUG_NBAP )
    {
        printf("RNC: %u received a RADIO BEARER SETUP RESPONSE from NodeB"
           " for UE: %u\n", node->nodeId, ueId);
    }
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsNodebRbSetupPara rbSetupPara;
    memcpy(&rbSetupPara,
           MESSAGE_ReturnPacket(msg),
           sizeof(rbSetupPara));

    UmtsUeSrncMap::iterator mapIt = rncL3->ueSrncMap->find(ueId);
    if (mapIt == rncL3->ueSrncMap->end())
    {
        return;
    }
    if (node->nodeId == mapIt->second->srncId)
    {
        UmtsLayer3RncHandleNodebRbSetupRes(
                    node,
                    umtsL3,
                    TRUE,
                    rbSetupPara);
    }
    else
    {
        //send response message to the SRNC
        UmtsLayer3RncSendIurNodebRbSetupRes(
                    node,
                    umtsL3,
                    TRUE,
                    ueId,
                    rbSetupPara);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRbReleaseResFromNodeb
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RADIO BEARER RELEASE RESPONSE message from the NodeB
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + msg       : Message*           : RRC message.
// + transctId : unsigned char         : Transaction Id
// + nodebId   : NodeId       : NodeB Id
// + ueId      : NodeId             : UE ID
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleRbReleaseResFromNodeb(
    Node* node,
    UmtsLayer3Data* umtsL3,
    Message* msg,
    unsigned char transctId,
    NodeId nodebId,
    NodeId ueId)
{
    if (DEBUG_NBAP )
    {
        printf("RNC: %u received a RADIO BEARER RELEASE RESPONSE from NodeB"
           " for UE: %u\n", node->nodeId, ueId);
    }
}

//
// /**
// FUNCTION   :: UmtsLayer3RncHandleJoinActvSetResFromNodeb
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RADIO BEARER SETUP RESPONSE message from NodeB
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*     : RRC message.
// + transctId : unsigned char         : Transaction Id
// + nodebId   : NodeId       : NodeB Id
// + ueId      : NodeId       : UE ID
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleJoinActvSetResFromNodeb(
    Node* node,
    UmtsLayer3Data* umtsL3,
    Message* msg,
    unsigned char transctId,
    NodeId nodebId,
    NodeId ueId)
{
    if (DEBUG_HO  && ueId == DEBUG_UE_ID)
    {
        printf("RNC: %u received a JOIN ACTIVE SET RESPONSE from NodeB"
           " for UE: %u\n", node->nodeId, ueId);
    }
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsUeSrncMap::iterator mapIt = rncL3->ueSrncMap->find(ueId);

    if (mapIt == rncL3->ueSrncMap->end())
    {

        if (DEBUG_ASSERT)
        {
            char errorMsg[MAX_STRING_LENGTH];
            sprintf(errorMsg, 
                    "cannot find serving RNC ID for UE: %d", ueId);
            ERROR_ReportError(errorMsg);
        }

        return;
    }


    UInt32 cellId;
    memcpy(&cellId, MESSAGE_ReturnPacket(msg), sizeof(cellId));
    if (node->nodeId == mapIt->second->srncId)
    {
        UmtsLayer3RncHandleNodebActvSetUpRes(
                    node,
                    umtsL3,
                    cellId,
                    TRUE,
                    ueId,
                    TRUE,
                    MESSAGE_ReturnPacket(msg),
                    MESSAGE_ReturnPacketSize(msg));
    }
    else
    {
        UmtsLayer3RncSendIurNodebJoinActvSetRes(
                    node,
                    umtsL3,
                    TRUE,
                    ueId,
                    cellId,
                    MESSAGE_ReturnPacket(msg),
                    MESSAGE_ReturnPacketSize(msg));
    }
}

//
// /**
// FUNCTION   :: UmtsLayer3RncHandleLeaveActvSetResFromNodeb
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling RADIO BEARER SETUP RESPONSE message from NodeB
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*     : RRC message.
// + transctId : unsigned char         : Transaction Id
// + nodebId   : NodeId       : NodeB Id
// + ueId      : NodeId       : UE ID
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleLeaveActvSetResFromNodeb(
    Node* node,
    UmtsLayer3Data* umtsL3,
    Message* msg,
    unsigned char transctId,
    NodeId nodebId,
    NodeId ueId)
{
    if (DEBUG_HO && ueId == DEBUG_UE_ID)
    {
        printf("RNC: %u received a LEAVE ACTIVE SET RESPONSE from NodeB"
           " for UE: %u\n", node->nodeId, ueId);
    }
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsUeSrncMap::iterator mapIt = rncL3->ueSrncMap->find(ueId);
    if (mapIt == rncL3->ueSrncMap->end())
    {

        if (DEBUG_ASSERT)
        {
            char errorMsg[MAX_STRING_LENGTH];
            sprintf(errorMsg,
                    "cannot find serving RNC ID for UE: %d", ueId);
            ERROR_ReportError(errorMsg);
        }

        return;
    }

    UInt32 cellId;
    memcpy(&cellId, MESSAGE_ReturnPacket(msg), sizeof(cellId));
    if (node->nodeId == mapIt->second->srncId)
    {
        UmtsLayer3RncHandleNodebActvSetUpRes(
                    node,
                    umtsL3,
                    cellId,
                    TRUE,
                    ueId,
                    FALSE,
                    MESSAGE_ReturnPacket(msg),
                    MESSAGE_ReturnPacketSize(msg));
    }
    else
    {
        UmtsLayer3RncSendIurNodebLeaveActvSetRes(
                    node,
                    umtsL3,
                    ueId,
                    cellId,
                    MESSAGE_ReturnPacket(msg),
                    MESSAGE_ReturnPacketSize(msg));
        //Erase UE-SRNC pair in the map if neccessary
        mapIt->second->refCount--;
        if (DEBUG_HO && ueId == DEBUG_UE_ID)
        {
            printf("CRNC: %u reduce the reference count of UE-SRNC map "
               " for UE: %u due to the removing of one ACTIVE SET member\n",
               node->nodeId, ueId);
        }
        if (mapIt->second->refCount == 0)
        {
            if (DEBUG_HO && ueId == DEBUG_UE_ID)
            {
                printf("RNC: %u remove the UE-SRNC map "
                   " for UE: %u\n", node->nodeId, ueId);
            }
            MEM_free(mapIt->second);
            rncL3->ueSrncMap->erase(mapIt);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleCellSwitchResFromNodeb
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling SWITCH CELL RESPONSE message from NodeB
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*     : RRC message.
// + nodebId   : NodeId       : NodeB Id
// + ueId      : NodeId       : UE ID
// RETURN     :: BOOL         : whether message is reused in the function
// **/
static
BOOL UmtsLayer3RncHandleCellSwitchResFromNodeb(
    Node* node,
    UmtsLayer3Data* umtsL3,
    Message* msg,
    NodeId nodebId,
    NodeId ueId)
{
    BOOL msgReUse = FALSE;
    if (DEBUG_HO && ueId == DEBUG_UE_ID && 0 )
    {
        printf("RNC: %u received a SWITCH CELL RESPONSE from NodeB"
           " for UE: %u\n", node->nodeId, ueId);
    }
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsUeSrncMap::iterator mapIt = rncL3->ueSrncMap->find(ueId);
    if (mapIt == rncL3->ueSrncMap->end())
    {
        return msgReUse;
    }
    if (node->nodeId == mapIt->second->srncId)
    {
        UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                        node,
                                        rncL3,
                                        ueId);
        if (!ueRrc || !ueRrc->primCellSwitch)
        {
            return msgReUse;
        }
        UmtsLayer3RncCompletePrimCellSwitch(
                node,
                umtsL3,
                ueRrc,
                msg);
    }
    else
    {
        UmtsLayer3RncSendIurMsg(
            node,
            umtsL3,
            msg,
            ueId,
            mapIt->second->srncId,
            UMTS_RNSAP_MSG_TYPE_SWITCH_CELL_RESPONSE);
        msgReUse = TRUE;
    }
    return msgReUse;
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleAmRlcErrRepFromNodeb
// LAYER      :: LAYER3 RRC
// PURPOSE    :: Handling REPORT RLC ERROR message from NodeB
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*     : RRC message.
// + nodebId   : NodeId       : NodeB Id
// + ueId      : NodeId       : UE ID
// RETURN     :: BOOL         : whether message is reused in the function
// **/
static
BOOL UmtsLayer3RncHandleAmRlcErrRepFromNodeb(
    Node* node,
    UmtsLayer3Data* umtsL3,
    Message* msg,
    NodeId nodebId,
    NodeId ueId)
{
    BOOL msgReUse = FALSE;
    if (DEBUG_IUR)
    {
        printf("RNC: %u received a REPORT AM RLC ERROR from NodeB"
           " for UE: %u\n", node->nodeId, ueId);
    }
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsUeSrncMap::iterator mapIt = rncL3->ueSrncMap->find(ueId);
    if (mapIt == rncL3->ueSrncMap->end())
    {
        return msgReUse;
    }

    UmtsRrcNodebDataInRnc* nodebRrc = UmtsLayer3RncFindNodebRrc(
                                        node,
                                        rncL3,
                                        nodebId);


    if (node->nodeId == mapIt->second->srncId)
    {
        UmtsLayer3RncHandleAmRlcErr(
            node,
            umtsL3,
            ueId,
            nodebRrc->cellId,
            MESSAGE_ReturnPacket(msg),
            MESSAGE_ReturnPacketSize(msg));
    }
    else
    {
        UmtsLayer3RncSendIurMsg(
            node,
            umtsL3,
            msg,
            ueId,
            mapIt->second->srncId,
            UMTS_RNSAP_MSG_TYPE_REPORT_AMRLC_ERROR,
            (char*)&(nodebRrc->cellId),
            sizeof(nodebRrc->cellId));
        msgReUse = TRUE;
    }
    return msgReUse;
}

//
// handle RANAP message
//
// /**
// FUNCTION   :: UmtsLayer3RncHandleRanapDirectTransfer
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Direct Transfer RANAP message from the SGSN.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleRanapDirectTransfer(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (DEBUG_ASSERT)
    {
        ERROR_Assert(ueRrc, "cannot find UE RRC data in the RNC.");
    }

    if (ueRrc == NULL || ueRrc->state != UMTS_RRC_STATE_CONNECTED)
    {
        UmtsLayer3RncReportIuRelease(
                        node,
                        umtsL3,
                        ueId,
                        UMTS_LAYER3_CN_DOMAIN_PS,
                        FALSE);
        UmtsLayer3RncReportIuRelease(
                node,
                umtsL3,
                ueId,
                UMTS_LAYER3_CN_DOMAIN_CS,
                FALSE);
        return;
    }

    char* packet = MESSAGE_ReturnPacket(msg);
    int packetSize = MESSAGE_ReturnPacketSize(msg);
    UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId)(*packet);

    Message* rrcMsg = 
        MESSAGE_Alloc(node, NETWORK_LAYER, MAC_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        rrcMsg,
                        packetSize,
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(rrcMsg),
           packet,
           packetSize);

    //Add a layer3 Header for RRC
    UmtsLayer3AddHeader(
        node,
        rrcMsg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_DL_DIRECT_TRANSFER);

    if (DEBUG_RR && 0)
    {
        printf("Rnc: %u sends a DL_DIRECT message\n ",
                node->nodeId);
#if 0
        printf("packet: \t\t");
        for (int i = 0; i < msg->packetSize; i++)
            printf("%d", (char) msg->packet[i]);
        printf("\n");
#endif
    }
    UmtsLayer3RncSendPktToUeOnDch(
        node,
        umtsL3,
        rrcMsg,
        ueId,
        UMTS_SRB3_ID);

    //Signal connection is considered established when
    //downlink direct transfer has been forwarded
    if (!ueRrc->signalConn[domain])
    {
        ueRrc->signalConn[domain] = TRUE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRanapPaging
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RANAP Paging message from the SGSN.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleRanapPaging(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg)
{
    char* packet = MESSAGE_ReturnPacket(msg);
    ERROR_Assert(MESSAGE_ReturnPacketSize(msg) == sizeof(NodeId) + 2,
        "UmtsLayer3RncHandleRanapPaging: incorrect paging format.");

    NodeId ueId;
    UmtsLayer3CnDomainId domain;
    UmtsPagingCause cause;
    memcpy(&ueId, packet, sizeof(ueId));
    packet += sizeof(ueId);
    domain = (UmtsLayer3CnDomainId)(*packet++);
    cause = (UmtsPagingCause)(*packet);

    if (DEBUG_RANAP && ueId == DEBUG_UE_ID)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, RNC: %u receives a RANAP paging message for UE: "
            "%u, domain: %d, cause: %d\n",
            timeStr, node->nodeId, ueId, domain, cause);
    }

    UmtsLayer3RncSendPaging(
        node,
        umtsL3,
        ueId,
        domain,
        cause);
}


// /**
// FUNCTION   :: UmtsLayer3RncHandleRabAssgtReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RAB ASSIGNMENT REQUEST message from the SGSN.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleRabAssgtReq(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    //GET RAB ASSIGNMENT request parameters from the message
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;
    UmtsRABServiceInfo ulRabPara;
    UmtsRABServiceInfo dlRabPara;
    UmtsRabConnMap connMap;

    UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId)
                                  packet[index++];

    memcpy(&(connMap), &packet[index], sizeof(connMap));
    index += sizeof(connMap);

    if (domain == UMTS_LAYER3_CN_DOMAIN_PS)
    {
        memcpy(&ulRabPara, &packet[index], sizeof(UmtsRABServiceInfo));
        index += sizeof(UmtsRABServiceInfo);
        memcpy(&dlRabPara, &packet[index], sizeof(UmtsRABServiceInfo));
        index += sizeof(UmtsRABServiceInfo);
    }
    else if (domain == UMTS_LAYER3_CN_DOMAIN_CS &&
             connMap.flowMap.pd == UMTS_LAYER3_PD_CC)
    {
        memset(&ulRabPara, 0, sizeof(ulRabPara));
        memset(&dlRabPara, 0, sizeof(dlRabPara));
        ulRabPara.guaranteedBitRate = 12200;
        ulRabPara.maxBitRate = 12200;
        dlRabPara.guaranteedBitRate = 12200;
        dlRabPara.maxBitRate = 12200;
    }

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (DEBUG_ASSERT)
    {
        ERROR_Assert(ueRrc, "cannot find UE RRC data in the RNC.");
    }

    if (!ueRrc || ueRrc->rrcRelReq.requested == TRUE)
    {
        UmtsLayer3RncRespToRabAssgt(
                    node,
                    umtsL3,
                    ueId,
                    FALSE,
                    domain,
                    &(connMap));
        return;
    }

    //Start to do RAB assignment for this request immediately if the
    //serving queue is empty
    // TODO:
    // determine if this RAB can be accpeted this time
    // otherwise queue this request and carry some call admissioncontrol
    // to free some resource for this req if this req is of high priority
    if (rncL3->rabServQueue->empty())
    {
        int rabId = UMTS_INVALID_RAB_ID;
        for (int i = 0; i < UMTS_MAX_RAB_SETUP; i++)
        {
            if (ueRrc->rabInfos[i] == NULL)
            {
                rabId = i;
                break;
            }
        }
        if (rabId != UMTS_INVALID_RAB_ID)
        {
            //Push the request into the serving queue to block
            //requests arriving later
            rncL3->rabServQueue->push_back(new UmtsRncRabAssgtReqInfo(
                                                ueId,
                                                domain,
                                                ulRabPara,
                                                dlRabPara,
                                                connMap,
                                                (char)rabId));
            UmtsLayer3RncInitRabSetup(
                node,
                umtsL3,
                ueId,
                (char)rabId,
                &ulRabPara,
                &dlRabPara,
                domain,
                &connMap);
        }
        else
        {
            UmtsLayer3RncRespToRabAssgt(
                    node,
                    umtsL3,
                    ueId,
                    FALSE,
                    domain,
                    &(connMap));
        }
    }
    //otherwise, push the request into the end the serving queue
    //Sorting the serving queue will be executed each time the RNC finishes
    //a RAB assignment request.
    else
    {
        rncL3->rabServQueue->push_back(new UmtsRncRabAssgtReqInfo(
                                            ueId,
                                            domain,
                                            ulRabPara,
                                            dlRabPara,
                                            connMap));
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRabRelease
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RAB ASSIGNMENT REQUEST message from the SGSN.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleRabRelease(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (DEBUG_ASSERT)
    {
        ERROR_Assert(ueRrc, "cannot find UE RRC data in the RNC.");
    }

    if (!ueRrc)
    {
        return;
    }

    char* packet = MESSAGE_ReturnPacket(msg);
    char rabId = packet[0];

    UmtsLayer3RncInitRabRelease(
                    node,
                    umtsL3,
                    ueId,
                    rabId);
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleIuRelease
// LAYER      :: Layer 3
// PURPOSE    :: Handle an IU RELEASE COMMAND message from the SGSN.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIuRelease(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId)
{

    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (DEBUG_ASSERT)
    {
        ERROR_Assert(ueRrc, "cannot find UE RRC data in the RNC.");
    }
    if (!ueRrc)
    {
        return;
    }

    UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId)
                                  (*MESSAGE_ReturnPacket(msg));

    if (ueRrc->signalConn[domain])
    {
        ueRrc->signalConn[domain] = FALSE;

        UmtsLayer3CnDomainId cmpVal[UMTS_MAX_CN_DOMAIN];
        memset(cmpVal,
               0,
               sizeof(UmtsLayer3CnDomainId)*UMTS_MAX_CN_DOMAIN);

        if (!memcmp(ueRrc->signalConn,
                   cmpVal,
                   sizeof(UmtsLayer3CnDomainId)*UMTS_MAX_CN_DOMAIN))
        {
            UmtsLayer3RncInitRrcConnRelease(
                node,
                umtsL3,
                ueRrc,
                TRUE);
        }
        else
        {
            UmtsLayer3RncSendSignalConnRel(
                node,
                umtsL3,
                ueRrc,
                domain);
        }

        UmtsLayer3RncReportIuRelease(
                node,
                umtsL3,
                ueRrc->ueId,
                domain,
                TRUE);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleCellLookupReply
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RAB ASSIGNMENT REQUEST message from the SGSN.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleCellLookupReply(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    char* packet = MESSAGE_ReturnPacket(msg);

    int index = 0;
    UInt32 cellId;
    NodeId nodebId;
    NodeId rncId;
    memcpy(&cellId, &packet[index], sizeof(cellId));
    index += sizeof(cellId);
    memcpy(&nodebId, &packet[index], sizeof(nodebId));
    index += sizeof(nodebId);
    memcpy(&rncId, &packet[index], sizeof(rncId));
    index += sizeof(rncId);

    UmtsCellCacheMap::iterator it
        = rncL3->cellCacheMap->find(cellId);
    if (rncL3->cellCacheMap->end() == it)
    {
        rncL3->cellCacheMap->insert(
                std::make_pair(
                    cellId,
                    new UmtsCellCacheInfo(
                        nodebId,
                        rncId)));
    }
    else
    {
        it->second->nodebId = nodebId;
        it->second->rncId = rncId;
    }
}

// **/
// /**
// FUNCTION   :: UmtsLayer3RncHandleRrMsg
// LAYER      :: Layer3
// PURPOSE    :: Handle RR msg.
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
// + interfaceIndex : int        : Interface from which packet was received
// + ueId      : NodeId            : The ue id originating the message
// + rbId      : UInt8             : RB ID the message belongs to
// + msgType   : char              : Radio resource control msg type
// + transactId: char              : Message transation Id
// + msg       : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncHandleRrMsg(Node* node,
                              UmtsLayer3Data *umtsL3,
                              int interfaceIndex,
                              NodeId ueId,
                              UInt8 rbId,
                              char msgType,
                              char transactId,
                              Message* msg)
{
    UmtsRRMessageType rrMsgType = (UmtsRRMessageType) msgType;
    switch (rrMsgType)
    {
        case UMTS_RR_MSG_TYPE_RRC_CONNECTION_REQUEST:
        {
            UmtsLayer3RncHandleRrcConnReq(
                node,
                umtsL3,
                interfaceIndex,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RRC_CONNECTION_SETUP_COMPLETE:
        {
            UmtsLayer3RncHandleRrcConnSetupComp(
                node,
                umtsL3,
                interfaceIndex,
                ueId,
                rbId,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RRC_CONNECTION_RELEASE_COMPLETE:
        {
            UmtsLayer3RncHandleRrcConnRelComp(
                node,
                umtsL3,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_INITIAL_DIRECT_TRANSFER:
        {
            UmtsLayer3RncHandleInitDirectTransfer(
                node,
                umtsL3,
                interfaceIndex,
                ueId,
                rbId,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_UL_DIRECT_TRANSFER:
        {
            UmtsLayer3RncHandleUlDirectTransfer(
                node,
                umtsL3,
                interfaceIndex,
                ueId,
                rbId,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RRC_STATUS:
        {
            UmtsLayer3RncHandleRrcStatus(
                node,
                umtsL3,
                interfaceIndex,
                ueId,
                rbId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RADIO_BEARER_SETUP_COMPLETE:
        {
            UmtsLayer3RncHandleRbSetupCmpl(
                node,
                umtsL3,
                ueId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RADIO_BEARER_RELEASE_COMPLETE:
        {
            UmtsLayer3RncHandleRbReleaseCmpl(
                node,
                umtsL3,
                ueId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_MEASUREMENT_REPORT:
        {
            UmtsLayer3RncHandleMeasReportFromUe(
                node,
                umtsL3,
                ueId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_ACTIVE_SET_UPDATE_COMPLETE:
        {
            UmtsLayer3RncHandleActvSetUpCompFromUe(
                node,
                umtsL3,
                ueId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_SIGNALLING_CONNECTION_RELEASE_INDICATION:
        {
            UmtsLayer3RncHandleSigRelIndiFromUe(
                node,
                umtsL3,
                ueId,
                msg);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(RNC) receivers a RR"
                    "msg with unknown type %d",
                    node->nodeId,
                    rrMsgType);
            ERROR_ReportWarning(errStr);

            break;
        }
    }
    MESSAGE_Free(node, msg);
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleDataPacketFromUe
// LAYER      :: Layer 3
// PURPOSE    :: Handle data packet from UE
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message containing the packet
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : NodeId           : The ue id originating the message
// + rbId      : UInt8            : RB ID the message belongs to
// + srcAddr   : Address          : Source node address
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncHandleDataPacketFromUe(
    Node *node,
    Message *msg,
    UmtsLayer3Data *umtsL3,
    NodeId ueId,
    UInt8 rbId,
    Address srcAddr)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    if (DEBUG_RR && 0 )
    {
        printf ("RNC: %u receives a data packet from UE %u "
            "in RB: %d \n", node->nodeId, ueId, rbId);
    }

    // update statistics
    rncL3->stat.numDataPacketFromUe ++;

//GuiStart
    if (node->guiOption == TRUE)
    {
        NodeAddress previousHopNodeId;

        previousHopNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                node,
                                srcAddr);
        GUI_Receive(previousHopNodeId, node->nodeId, GUI_NETWORK_LAYER,
                    GUI_DEFAULT_DATA_TYPE,
                    DEFAULT_INTERFACE,
                    DEFAULT_INTERFACE,
                    node->getNodeTime());
    }
//GuiEnd

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (DEBUG_ASSERT)
    {
        ERROR_Assert(ueRrc, "UmtsLayer3RncHandleDataPacketFromUe: "
            "cannot not find UE RRC data in the RNC.");
    }

    if (ueRrc == NULL)
    {
        // no RRC info, free the message and update stat
        MESSAGE_Free(node, msg);
        rncL3->stat.numDataPacketFromUeDroppedNoRrc ++;
        return;
    }

    MESSAGE_SetLayer(msg, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR);

    char rabId = ueRrc->rbRabId[rbId - UMTS_RBINRAB_START_ID];
    UmtsRrcRabInfo* rabInfo = ueRrc->rabInfos[(int)rabId];

    if (DEBUG_ASSERT)
    {
        ERROR_Assert(rabInfo, "UmtsLayer3RncHandleDataPacketFromUe: "
            "cannot not find RAB info in the RNC.");
    }

    if (rabInfo == NULL)
    {
        // no RAB info, free the message and update stat
        MESSAGE_Free(node, msg);
        rncL3->stat.numDataPacketFromUeDroppedNoRab ++;
    }

    if (rabInfo->cnDomainId == UMTS_LAYER3_CN_DOMAIN_PS)
    {
        unsigned char firstByte = UMTS_GTP_HEADER_FIRST_BYTE;
        UmtsAddGtpHeader(node,
                         msg,
                         firstByte,
                         UMTS_GTP_G_PDU,
                         rabInfo->connMap.tunnelMap.ulTeId,
                         NULL,
                         0);

        // Add GTP header
        UmtsAddGtpBackboneHeader(node,
                                 msg,
                                 TRUE,
                                 ueId);
        // update statistics
        rncL3->stat.numPsDataPacketToSgsn ++;
    }
    else if (rabInfo->cnDomainId == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        UmtsLayer3AddIuCsDataHeader(node, msg, ueId, rabId);
        // update statistics
        rncL3->stat.numCsDataPacketToSgsn ++;
    }
    else
    {
        ERROR_ReportError("RAB info is not valid.");
    }

    // send to my SGSN
    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        rncL3->sgsnInfo.nodeAddr,
        rncL3->sgsnInfo.interfaceIndex,
        IPTOS_PREC_INTERNETCONTROL,
        1);
}

// /**
// FUNCTION   :: UmtsLayer3SrncHandleUePacket
// LAYER      :: Layer 3
// PURPOSE    :: Handle L3 message from UE
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + interfaceIndex : int         : Interface from which packet was received
// + ueId      : NodeId           : The ue id originating the message
// + rbId      : UInt8            : RB ID the message belongs to
// + srcAddr   : Address          : Source node address
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SrncHandleUePacket(Node *node,
                                  UmtsLayer3Data *umtsL3,
                                  Message *msg,
                                  int interfaceIndex,
                                  NodeId ueId,
                                  char rbId,
                                  Address srcAddr)
{
    //If ueId and rbId indicate that it is a data message
    if (rbId > UMTS_SRB4_ID && rbId < UMTS_BCCH_RB_ID)
    {
        UmtsLayer3RncHandleDataPacketFromUe(
            node,
            msg,
            umtsL3,
            ueId,
            rbId,
            srcAddr);
    }
    else
    {
        //Otherwise, it is a signal message
        char pd;
        char tiSpd;
        char msgType;
        UmtsLayer3RemoveHeader(node, msg, &pd, &tiSpd, &msgType);
        switch (pd)
        {
            case UMTS_LAYER3_PD_RR:  // RR management message
            {
                UmtsLayer3RncHandleRrMsg(node,
                                         umtsL3,
                                         interfaceIndex,
                                         ueId,
                                         rbId,
                                         msgType,
                                         tiSpd,
                                         msg);
                break;
            }
            default:
            {
                char infoStr[MAX_STRING_LENGTH];
                sprintf(infoStr,
                        "UMTS Layer3: Node%d(RNC) receives a Layer3 msg "
                        "with an unknown Protocol Descriptor (PD) as %d",
                        node->nodeId,
                        pd);
                ERROR_ReportWarning(infoStr);
            }
        }
    }
}

//
// Handle Iur Messag
//
// /**
// FUNCTION   :: UmtsLayer3RncHandleIurReqNodebToJoinActvSet
// LAYER      :: Layer 3
// PURPOSE    :: Handle a JOIN ACTIVE SET IUR message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + cellId         : UInt32                 : cell Id of nodeB
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurReqNodebToJoinActvSet(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    UInt32          cellId)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;

    char* packet = MESSAGE_ReturnPacket(msg);
    int packetSize = MESSAGE_ReturnPacketSize(msg);
    int index = 0;

    NodeId srncId;
    memcpy(&srncId, &packet[index], sizeof(srncId));
    index += sizeof(srncId);

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives a JOIN ACTIVE SET message from SRNC: %d "
            "with CellID %d and UE Id: %d \n",
            srncId, cellId, ueId);
    }

    UmtsNodebSrbConfigPara nodebSrbConfig;
    std::vector<UmtsRABServiceInfo*> rabParaVec;
    std::vector<UmtsNodebRbSetupPara*> nodebParaVec;

    memcpy(&nodebSrbConfig, &packet[index], sizeof(nodebSrbConfig));
    index += sizeof(nodebSrbConfig);
    while (index < packetSize)
    {
        UmtsRABServiceInfo rabPara;
        memcpy(&rabPara, &packet[index], sizeof(rabPara));
        index += sizeof(rabPara);
        rabParaVec.push_back(new UmtsRABServiceInfo(rabPara));

        UmtsNodebRbSetupPara nodebPara;
        memcpy(&nodebPara, &packet[index], sizeof(nodebPara));
        index += sizeof(nodebPara);
        nodebParaVec.push_back(new UmtsNodebRbSetupPara(nodebPara));
    }

    UmtsCellCacheInfo* cellInfo = rncL3->cellCacheMap->find(
                                        cellId)->second;
    UmtsRrcNodebDataInRnc* nodebRrc = UmtsLayer3RncFindNodebRrc(
                                            node,
                                            rncL3,
                                            cellInfo->nodebId);

    if (FALSE == UmtsLayer3RncReqNodebToJoinActvSet(
                    node,
                    umtsL3,
                    nodebRrc,
                    ueId,
                    nodebSrbConfig,
                    rabParaVec,
                    nodebParaVec))
    {
        //send a response message to the SRNC
        UmtsLayer3RncSendIurNodebJoinActvSetRes(
                    node,
                    umtsL3,
                    FALSE,
                    ueId,
                    cellId);
    }
    else
    {
        //If the cell can be added into the active set, add UE-SRNC map
        //or increases its count
        UmtsUeSrncMap::iterator mapIter = rncL3->ueSrncMap->find(ueId);
        if (mapIter == rncL3->ueSrncMap->end())
        {
            if (DEBUG_HO && ueId == DEBUG_UE_ID)
            {
                printf("CRNC: %u adds new UE-SRNC map for UE: %u "
                   " due to the adding of one ACTIVE SET member\n",
                   node->nodeId, ueId);
            }
            UmtsSrncInfo* srncInfo = (UmtsSrncInfo*)
                                 MEM_malloc(sizeof(UmtsSrncInfo));
            srncInfo->srncId = srncId;
            srncInfo->refCount = 1;
            (*rncL3->ueSrncMap)[ueId] = srncInfo;
        }
        else
        {
            if (DEBUG_HO && ueId == DEBUG_UE_ID)
            {
                printf(
                   "CRNC: %u increases the reference count of UE-SRNC map "
                   " for UE: %u due to the adding of "
                   "one ACTIVE SET member\n",
                   node->nodeId, ueId);
            }
            ERROR_Assert(mapIter->second->srncId == srncId,
                "UE SRNC MAP stores wrong map information.");
            mapIter->second->refCount++;
        }
    }

    for (unsigned int i = 0; i < rabParaVec.size(); i++)
    {
        delete rabParaVec[i];
        delete nodebParaVec[i];
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleIurReqNodebToLeaveActvSet
// LAYER      :: Layer 3
// PURPOSE    :: Handle a LEAVE ACTIVE SET IUR message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + cellId         : UInt32                 : cell Id of nodeB
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurReqNodebToLeaveActvSet(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    UInt32          cellId)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;
    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);
    int packetSize = MESSAGE_ReturnPacketSize(msg);

    UmtsNodebSrbConfigPara nodebSrbConfig;
    std::vector<UmtsNodebRbSetupPara*> nodebParaVec;

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives a LEAVE ACTIVE SET message "
            "with CellID %d and UE Id: %d \n",
            cellId, ueId);
    }

    memcpy(&nodebSrbConfig, &packet[index], sizeof(nodebSrbConfig));
    index += sizeof(nodebSrbConfig);
    while (index < packetSize)
    {
        UmtsNodebRbSetupPara nodebPara;
        memcpy(&nodebPara, &packet[index], sizeof(nodebPara));
        index += sizeof(nodebPara);
        nodebParaVec.push_back(new UmtsNodebRbSetupPara(nodebPara));
    }

    UmtsCellCacheInfo* cellInfo = rncL3->cellCacheMap->find(
                                        cellId)->second;
    UmtsRrcNodebDataInRnc* nodebRrc = UmtsLayer3RncFindNodebRrc(
                                            node,
                                            rncL3,
                                            cellInfo->nodebId);
    UmtsLayer3RncReqNodebToLeaveActvSet(
        node,
        umtsL3,
        nodebRrc,
        ueId,
        nodebSrbConfig,
        nodebParaVec);
    for (unsigned int i = 0; i < nodebParaVec.size(); i++)
    {
        delete nodebParaVec[i];
    }
}

// FUNCTION   :: UmtsLayer3RncHandleIurNodebJoinActvSetRes
// LAYER      :: Layer 3
// PURPOSE    :: Handle a JOIN/LEAVE ACTIVE SET RESPONSE IUR message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + cellId         : UInt32                 : cell Id of nodeB
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurNodebJoinActvSetRes(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    UInt32          cellId)
{
    char* packet = MESSAGE_ReturnPacket(msg);
    int packetSize = MESSAGE_ReturnPacketSize(msg);

    BOOL success = (BOOL)packet[0];
    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR JOIN ACTIVE SET RES message "
            "with CELL Id: %d UE Id: %d \n", cellId, ueId);
    }

    UmtsLayer3RncHandleNodebActvSetUpRes(
                node,
                umtsL3,
                cellId,
                success,
                ueId,
                TRUE,
                packet + sizeof(char),
                packetSize - sizeof(char));
}

// FUNCTION   :: UmtsLayer3RncHandleIurNodebLeaveActvSetRes
// LAYER      :: Layer 3
// PURPOSE    :: Handle a JOIN/LEAVE ACTIVE SET RESPONSE IUR message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + cellId         : UInt32                 : cell Id of nodeB
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurNodebLeaveActvSetRes(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    UInt32          cellId)
{
    char* packet = MESSAGE_ReturnPacket(msg);
    int packetSize = MESSAGE_ReturnPacketSize(msg);

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR LEAVE ACTIVE SET RES message "
            "with CELL Id: %d, UE Id: %d \n", cellId, ueId);
    }

    UmtsLayer3RncHandleNodebActvSetUpRes(
                node,
                umtsL3,
                cellId,
                TRUE,
                ueId,
                FALSE,
                packet,
                packetSize);
}

// FUNCTION   :: UmtsLayer3RncHandleIurRbSetupReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle an IUR RB SETUP REQUEST message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + cellId         : UInt32                 : cell Id of nodeB
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurRbSetupReq(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    UInt32          cellId)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;

    UmtsNodebRbSetupPara nodebPara;
    UmtsRABServiceInfo rabPara;
    memcpy(&nodebPara, MESSAGE_ReturnPacket(msg), sizeof(nodebPara));
    memcpy(&rabPara,
           MESSAGE_ReturnPacket(msg) + sizeof(nodebPara),
           sizeof(UmtsRABServiceInfo));
    UmtsCellCacheInfo* cellCacheInfo =
                rncL3->cellCacheMap->find(cellId)->second;

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR RB SETUP REQUEST message "
            "with CELL Id: %d, UE Id: %d \n", cellId, ueId);
    }

    UmtsRrcNodebDataInRnc* nodebRrc = UmtsLayer3RncFindNodebRrc(
                                        node,
                                        rncL3,
                                        cellCacheInfo->nodebId);

    if (FALSE == UmtsLayer3RncCheckNodebForRbSetup(
                    node,
                    umtsL3,
                    nodebRrc,
                    &rabPara,
                    &nodebPara))
    {
        UmtsLayer3RncSendIurNodebRbSetupRes(
                    node,
                    umtsL3,
                    FALSE,
                    ueId,
                    nodebPara);
    }
    else
    {
        UmtsLayer3RncStoreNodebConfigForRbSetup(
            node,
            umtsL3,
            nodebRrc,
            nodebPara,
            &rabPara);

        UmtsLayer3RncSendRbSetupToNodeb(node,
                                        umtsL3,
                                        nodebRrc,
                                        nodebPara);
    }
}

// FUNCTION   :: UmtsLayer3RncHandleIurRbSetupRes
// LAYER      :: Layer 3
// PURPOSE    :: Handle an IUR RB SETUP RESPONSE message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurRbSetupRes(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId)
{
    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);
    BOOL success = (char)packet[index ++];

    UmtsNodebRbSetupPara nodebPara;
    memcpy(&nodebPara, &packet[index], sizeof(nodebPara));

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR RB SETUP RESPONSE message "
            "with CELL Id: %d, UE Id: %d \n", nodebPara.cellId, ueId);
    }

    UmtsLayer3RncHandleNodebRbSetupRes(
                    node,
                    umtsL3,
                    success,
                    nodebPara);
}

// FUNCTION   :: UmtsLayer3RncHandleIurRbReleaseReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle an IUR RB RELEASE REQUEST message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + cellId         : UInt32                 : cell Id of nodeB
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurRbReleaseReq(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    UInt32          cellId)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;
    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR RB RELEASE REQUEST message "
            "with CELL Id: %d, UE Id: %d \n", cellId, ueId);
    }
    UmtsNodebRbSetupPara nodebPara;
    memcpy(&nodebPara, MESSAGE_ReturnPacket(msg), sizeof(nodebPara));

    UmtsCellCacheMap::iterator nodebIt =
        rncL3->cellCacheMap->find((cellId));
    ERROR_Assert(nodebIt != rncL3->cellCacheMap->end(),
        "cannot find the NodeB info in the monitored set.");
    UmtsRrcNodebDataInRnc* nodebRrc
        = UmtsLayer3RncFindNodebRrc(
                node,
                rncL3,
                nodebIt->second->nodebId);
    ERROR_Assert(nodebRrc,
        "cannot find the NodeB RRC data in the RNC.");

    if (TRUE == UmtsLayer3RncReleaseNodebConfigForRbRelease(
                    node,
                    umtsL3,
                    nodebRrc,
                    nodebPara))
    {
        UmtsLayer3RncSendRbReleaseToNodeb(
            node,
            umtsL3,
            nodebRrc,
            nodebPara);
    }

}

// FUNCTION   :: UmtsLayer3RncHandleIurRbReleaseRes
// LAYER      :: Layer 3
// PURPOSE    :: Handle an IUR RB RELEASE REQUEST message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurRbReleaseRes(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId)
{
    return;
}

// FUNCTION   :: UmtsLayer3RncHandleIurSrbReleaseReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle an IUR SRB RELEASE REQUEST message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + cellId         : UInt32                 : cell Id of nodeB
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurSrbReleaseReq(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    UInt32          cellId)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;
    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR SRB RELEASE REQUEST message "
            "with CELL Id: %d, UE Id: %d \n", cellId, ueId);
    }

    //Erase UE-SRNC pair in the map
    UmtsUeSrncMap::iterator mapIter = rncL3->ueSrncMap->find(ueId);
    if (mapIter == rncL3->ueSrncMap->end())
    {
        return;
    }
    if (DEBUG_HO && ueId == DEBUG_UE_ID)
    {
        printf("RNC: %u reduces the reference count of UE-SRNC map "
           " for UE: %u due to SRB release\n", node->nodeId, ueId);
    }
    mapIter->second->refCount--;
    if (mapIter->second->refCount == 0)
    {
        if (DEBUG_HO && ueId == DEBUG_UE_ID)
        {
            printf("RNC: %u remove UE-SRNC map "
               " for UE: %u \n", node->nodeId, ueId);
        }
        MEM_free(mapIter->second);
        rncL3->ueSrncMap->erase(mapIter);
    }

    UmtsNodebSrbConfigPara nodebPara;
    memcpy(&nodebPara, MESSAGE_ReturnPacket(msg), sizeof(nodebPara));

    UmtsLayer3RncSendRrcReleaseToNodeb(
        node,
        umtsL3,
        ueId,
        cellId,
        nodebPara);
}

// FUNCTION   :: UmtsLayer3RncHandleIurCellSwitchReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CELL_SWITCH_REQUEST IUR message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + cellId         : UInt32                 : cell Id of nodeB
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurCellSwitchReq(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    UInt32          cellId)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR SWITCH CELL REQUEST message "
            "with CELL Id: %d, UE Id: %d \n", cellId, ueId);
    }

    UmtsCellCacheInfo* cachedCell = rncL3->cellCacheMap->find(
                                        cellId)->second;

    UmtsLayer3RncSendNbapMessageToNodeb(
        node,
        umtsL3,
        msg,
        UMTS_NBAP_MSG_TYPE_SWITCH_CELL_REQUEST,
        0,
        cachedCell->nodebId,
        ueId);
}

// FUNCTION   :: UmtsLayer3RncHandleIurCellSwitchRes
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CELL_SWITCH_REQUEST IUR message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurCellSwitchRes(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR SWITCH CELL RESPONSE message "
            "with UE Id: %d \n",  ueId);
    }
    if (DEBUG_ASSERT)
    {
        ERROR_Assert(rncL3->ueSrncMap->find(ueId)->second->srncId
                        == node->nodeId,
           "Current node should be the serving RNC of the UE.");
    }

    UmtsRrcUeDataInRnc* ueRrc = UmtsLayer3RncFindUeRrc(
                                    node,
                                    rncL3,
                                    ueId);
    if (!ueRrc || !ueRrc->primCellSwitch)
    {
        return;
    }

    UmtsLayer3RncCompletePrimCellSwitch(
            node,
            umtsL3,
            ueRrc,
            msg);
}

// FUNCTION   :: UmtsLayer3RncHandleIurDlTransfer
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CELL_SWITCH_REQUEST IUR message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + cellId         : UInt32                 : cell Id of nodeB
// + rbId           : char                   : RB ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurDlTransfer(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    UInt32          cellId,
    char            rbId)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR DL TRANSFER message "
            "with UE Id: %d \n",  ueId);
    }

    UmtsCellCacheInfo* cachedCell = rncL3->cellCacheMap->find(
                                        cellId)->second;
    ERROR_Assert(cachedCell->rncId == node->nodeId,
        "Received wrong IUR downlink transfer message.");

    UmtsLayer3RncSendRrcMessageThroughNodeb(
        node,
        umtsL3,
        msg,
        ueId,
        rbId,
        cachedCell->nodebId);
}

// FUNCTION   :: UmtsLayer3RncHandleIurUlTransfer
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CELL_SWITCH_REQUEST IUR message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + rbId           : char                   : cell Id of nodeB
// + srcAddr        : Address                : Source node address
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurUlTransfer(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    char            rbId,
    Address         srcAddr)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR UL TRANSFER message "
            "with UE Id: %d \n",  ueId);
    }

    if (DEBUG_ASSERT)
    {
        ERROR_Assert(rncL3->ueSrncMap->find(ueId)->second->srncId
                     == node->nodeId,
            "Received wrong IUR uplink transfer message.");
    }
    UmtsLayer3SrncHandleUePacket(
        node,
        umtsL3,
        msg,
        ANY_INTERFACE,
        ueId,
        rbId,
        srcAddr);
}

// FUNCTION   :: UmtsLayer3RncHandleIurSrncUp
// LAYER      :: Layer 3
// PURPOSE    :: Handle an IUR UPDATE SRNC message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurSrncUp(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId)
{
    UmtsLayer3Rnc* rncL3 = (UmtsLayer3Rnc*) umtsL3->dataPtr;
    char* packet = MESSAGE_ReturnPacket(msg);

    NodeId srncId;
    memcpy(&srncId, packet, sizeof(srncId));

    if (DEBUG_HO && ueId == DEBUG_UE_ID)
    {
        printf("RNC: %u receives UE-SRNC map update"
            " for UE: %u with new SRNC as:%d\n", 
            node->nodeId, ueId, srncId);
    }
    UmtsUeSrncMap::iterator iter = rncL3->ueSrncMap->find(ueId);
    if (iter != rncL3->ueSrncMap->end())
    {
        iter->second->srncId = srncId;
    }
    else
    {
        UmtsSrncInfo* srncInfo = (UmtsSrncInfo*)
                             MEM_malloc(sizeof(UmtsSrncInfo));
        srncInfo->srncId = node->nodeId;
        srncInfo->refCount = 1;
        (*rncL3->ueSrncMap)[ueId] = srncInfo;
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleHelloPacket
// LAYER      :: Layer 3
// PURPOSE    :: Handle a hello packet from neighbor NodeBs or RNCs.
//               This is used to find out the RNCs and NodeBs that this
//               RNC connects to
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message containing the packet
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + interfaceIndex : int         : Interface from which packet was received
// + srcAddr   : Address          : Address of the source node
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncHandleHelloPacket(Node *node,
                                    Message *msg,
                                    UmtsLayer3Data *umtsL3,
                                    int interfaceIndex,
                                    Address srcAddr)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsLayer3HelloPacket helloPkt;

    memcpy(&helloPkt, MESSAGE_ReturnPacket(msg), sizeof(helloPkt));
    if (helloPkt.nodeType == CELLULAR_NODEB)
    {
        int index = sizeof(UmtsLayer3HelloPacket);
        char *packetPtr = MESSAGE_ReturnPacket(msg);
        UInt32 cellId;
        memcpy(&cellId, packetPtr + index, sizeof(cellId));
        index += sizeof(cellId);

        if (DEBUG_BACKBONE)
        {
            printf("Node%d(RNC) discovered NodeB %d with cellID %d "
                   " from inf %d\n",
                   node->nodeId, helloPkt.nodeId,
                   cellId, interfaceIndex);
        }

        UmtsLayer3NodebInfo *itemPtr = NULL;

        itemPtr = (UmtsLayer3NodebInfo *)
                  FindItem(node,
                           rncL3->nodebList,
                           0,
                           (char *) &(helloPkt.nodeId),
                           sizeof(helloPkt.nodeId));
        if (itemPtr == NULL)
        {
            // not exist, allocate memory and insert
            itemPtr = (UmtsLayer3NodebInfo *)
                      MEM_malloc(sizeof(UmtsLayer3NodebInfo));
            memset(itemPtr, 0, sizeof(UmtsLayer3NodebInfo));
            itemPtr->nodeId = helloPkt.nodeId;
            ListInsert(node, rncL3->nodebList, node->getNodeTime(), itemPtr);
        }

        itemPtr->nodeAddr = srcAddr;
        itemPtr->interfaceIndex = interfaceIndex;

        //Create new Nodeb RRC data in RNC
        std::list<UmtsRrcNodebDataInRnc*>::iterator itemPos;
        itemPos = std::find_if(
                    rncL3->nodebRrcs->begin(),
                    rncL3->nodebRrcs->end(),
                    UmtsFindItemByNodebId
                        <UmtsRrcNodebDataInRnc>(itemPtr->nodeId));
        if (itemPos == rncL3->nodebRrcs->end())
        {
            // to see if any cell in list  has the same cellId with this one
            UmtsCellCacheMap::iterator nodebIt =
                    rncL3->cellCacheMap->find(cellId);
            ERROR_Assert(nodebIt == rncL3->cellCacheMap->end(),
            "Conflict of NodeB's scrambling code set index!\n"
            "Please configure different "
            "PHY-UMTS-NodeB-SCRAMBLING-CODE-SET-INDEX for each NodeB.");
            UmtsRrcNodebDataInRnc* nodebRrc =
                UmtsLayer3RncInitNodebRrc(node,
                                          itemPtr->nodeId,
                                          cellId);
            rncL3->nodebRrcs->push_back(nodebRrc);

            rncL3->cellCacheMap->insert(
                        std::make_pair(
                                cellId,
                                new UmtsCellCacheInfo(
                                    itemPtr->nodeId,
                                    node->nodeId)));

            //If sgsn is known, report the cell information
            //to the sgsn
            if (rncL3->sgsnInfo.nodeId != ANY_NODEID)
            {
                std::string buffer;
                int numCellItems = 1;
                buffer.append((char*)&numCellItems, sizeof(numCellItems));
                buffer.append((char*)&cellId, sizeof(cellId));
                buffer.append((char*)&itemPtr->nodeId,
                              sizeof(itemPtr->nodeId));
                UmtsLayer3SendHelloPacket(
                    node,
                    umtsL3,
                    rncL3->sgsnInfo.interfaceIndex,
                    (int)buffer.size(),
                    buffer.data());
            }
        }

        // handle subnet address segment info
        while ((index + sizeof(NodeAddress) + sizeof(int)) <=
               (unsigned int)MESSAGE_ReturnPacketSize(msg))
        {
            NodeAddress subnetAddress;
            int numHostBits;
            UmtsLayer3AddressInfo *addrInfo;

            memcpy((char*)&subnetAddress,
                   packetPtr + index,
                   sizeof(subnetAddress));
            index += sizeof(subnetAddress);
            memcpy((char*)&numHostBits,
                   packetPtr + index,
                   sizeof(numHostBits));
            index += sizeof(numHostBits);

            // if not in the list, insert it
            addrInfo = (UmtsLayer3AddressInfo *)
                       FindItem(node,
                           rncL3->addrInfoList,
                           0,
                           (char *)&subnetAddress,
                           sizeof(subnetAddress));
            if (addrInfo == NULL)
            {
                // not exist, allocate memory and insert
                addrInfo = (UmtsLayer3AddressInfo *)
                           MEM_malloc(sizeof(UmtsLayer3AddressInfo));
                memset(addrInfo, 0, sizeof(UmtsLayer3AddressInfo));
                addrInfo->subnetAddress = subnetAddress;
                addrInfo->numHostBits = numHostBits;
                ListInsert(node,
                           rncL3->addrInfoList,
                           node->getNodeTime(),
                           addrInfo);
            }
        }
    }
    else if (helloPkt.nodeType == CELLULAR_RNC)
    {
        if (DEBUG_BACKBONE)
        {
            printf("Node%d(RNC) discovered neighbor RNC %d from inf %d\n",
                   node->nodeId, helloPkt.nodeId, interfaceIndex);
        }

        UmtsLayer3RncInfo *itemPtr = NULL;

        itemPtr = (UmtsLayer3RncInfo *)
                  FindItem(node,
                           rncL3->neighRncList,
                           0,
                           (char *) &(helloPkt.nodeId),
                           sizeof(helloPkt.nodeId));
        if (itemPtr == NULL)
        {
            // not exist, allocate memory and insert
            itemPtr = (UmtsLayer3RncInfo *)
                      MEM_malloc(sizeof(UmtsLayer3RncInfo));
            memset(itemPtr, 0, sizeof(UmtsLayer3RncInfo));
            itemPtr->nodeId = helloPkt.nodeId;
            ListInsert(
                node, rncL3->neighRncList, node->getNodeTime(), itemPtr);
        }

        itemPtr->nodeAddr = srcAddr;
        itemPtr->interfaceIndex = interfaceIndex;
    }
    else if (helloPkt.nodeType == CELLULAR_SGSN ||
             helloPkt.nodeType == CELLULAR_GGSN)
    {
        if (DEBUG_BACKBONE)
        {
            printf("Node%d(RNC) discovered neighbor SGSN %d from inf %d\n",
                   node->nodeId, helloPkt.nodeId, interfaceIndex);
        }

        if (rncL3->sgsnInfo.nodeId == ANY_NODEID ||
            rncL3->sgsnInfo.nodeId == helloPkt.nodeId)
        {
            rncL3->sgsnInfo.nodeId = helloPkt.nodeId;
            rncL3->sgsnInfo.nodeAddr = srcAddr;
            rncL3->sgsnInfo.interfaceIndex = interfaceIndex;

            //report the cell information to the SGSN
            std::string buffer;
            UmtsCellCacheMap::iterator it;
            int numCellItems = (int)rncL3->cellCacheMap->size();
            buffer.append((char*)&numCellItems, sizeof(numCellItems));
            for (it = rncL3->cellCacheMap->begin();
                 it != rncL3->cellCacheMap->end(); ++it)
            {
                buffer.append((char*)&(it->first), sizeof(it->first));
                buffer.append((char*)&(it->second->nodebId),
                              sizeof(it->second->nodebId));
            }

            // report cell address info
            UmtsLayer3AddressInfo *addrInfo;
            int numAddrInfo = rncL3->addrInfoList->size;
            buffer.append((char*)&numAddrInfo, sizeof(numAddrInfo));
            ListItem *item = rncL3->addrInfoList->first;
            while (item)
            {
                addrInfo = (UmtsLayer3AddressInfo *) item->data;
                buffer.append((char*)&addrInfo->subnetAddress,
                              sizeof(addrInfo->subnetAddress));
                buffer.append((char*)&addrInfo->numHostBits,
                              sizeof(addrInfo->numHostBits));
                item = item->next;
            }

            UmtsLayer3SendHelloPacket(
                node,
                umtsL3,
                rncL3->sgsnInfo.interfaceIndex,
                (int)buffer.size(),
                buffer.data());
        }
    }
    else
    {
        // I am not supposed to receive hello packet from other type of node
        // Do nothing
        if (DEBUG_BACKBONE)
        {
            printf("Node%d(RNC) discovered nbor %d from inf %d\n",
                   node->nodeId, helloPkt.nodeId, interfaceIndex);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleIurMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a IUR message from another RNC
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + msgType        : UmtsRnsapMessageType   : Iur message type
// + ueId           : NodeId                 : UE ID
// + additionInfo   : const char*            : additonal information
// + additionInfoSize : int                  : size of additional info
// + srcAddr        : Address                : Source node address
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3RncHandleIurMsg(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    UmtsRnsapMessageType msgType,
    NodeId          ueId,
    const char*     additionInfo,
    int             additionInfoSize,
    Address         srcAddr)
{
    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR message for UE: %u \n",
            ueId);
        UmtsPrintMessage(std::cout, msg);
    }
    switch (msgType)
    {
        case UMTS_RNSAP_MSG_TYPE_SRNC_UPDATE:
        {
            UmtsLayer3RncHandleIurSrncUp(
                node,
                umtsL3,
                msg,
                ueId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_UPLINK_TRANSFER:
        {
            char   rbId;
            ERROR_Assert(sizeof(char) == additionInfoSize,
                " Wrong IUR Header format.");
            rbId = additionInfo[0];
            UmtsLayer3RncHandleIurUlTransfer(
                node,
                umtsL3,
                msg,
                ueId,
                rbId,
                srcAddr);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_DOWNLINK_TRANSFER:
        {
            UInt32 cellId;
            char   rbId;
            ERROR_Assert(sizeof(cellId) + sizeof(rbId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            rbId = additionInfo[sizeof(cellId)];
            UmtsLayer3RncHandleIurDlTransfer(
                node,
                umtsL3,
                msg,
                ueId,
                cellId,
                rbId);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_SRB_RELEASE_REQUEST:
        {
            UInt32 cellId;
            ERROR_Assert(sizeof(cellId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            UmtsLayer3RncHandleIurSrbReleaseReq(
                node,
                umtsL3,
                msg,
                ueId,
                cellId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_RADIO_BEARER_SETUP_REQUEST:
        {
            UInt32 cellId;
            ERROR_Assert(sizeof(cellId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            UmtsLayer3RncHandleIurRbSetupReq(
                node,
                umtsL3,
                msg,
                ueId,
                cellId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_RADIO_BEARER_SETUP_RESPONSE:
        {
            UmtsLayer3RncHandleIurRbSetupRes(
                node,
                umtsL3,
                msg,
                ueId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_RADIO_BEARER_RELEASE_REQUEST:
        {
            UInt32 cellId;
            ERROR_Assert(sizeof(cellId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            UmtsLayer3RncHandleIurRbReleaseReq(
                node,
                umtsL3,
                msg,
                ueId,
                cellId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_RADIO_BEARER_RELEASE_RESPONSE:
        {
            UmtsLayer3RncHandleIurRbReleaseRes(
                node,
                umtsL3,
                msg,
                ueId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_JOIN_ACTIVE_SET_REQUEST:
        {
            UInt32 cellId;
            ERROR_Assert(sizeof(cellId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            UmtsLayer3RncHandleIurReqNodebToJoinActvSet(
                node,
                umtsL3,
                msg,
                ueId,
                cellId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_JOIN_ACTIVE_SET_RESPONSE:
        {
            UInt32 cellId;
            ERROR_Assert(sizeof(cellId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            UmtsLayer3RncHandleIurNodebJoinActvSetRes(
                node,
                umtsL3,
                msg,
                ueId,
                cellId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_LEAVE_ACTIVE_SET_REQUEST:
        {
            UInt32 cellId;
            ERROR_Assert(sizeof(cellId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            UmtsLayer3RncHandleIurReqNodebToLeaveActvSet(
                node,
                umtsL3,
                msg,
                ueId,
                cellId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_LEAVE_ACTIVE_SET_RESPONSE:
        {
            UInt32 cellId;
            ERROR_Assert(sizeof(cellId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            UmtsLayer3RncHandleIurNodebLeaveActvSetRes(
                node,
                umtsL3,
                msg,
                ueId,
                cellId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_SWITCH_CELL_REQUEST:
        {
            UInt32 cellId;
            ERROR_Assert(sizeof(cellId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            UmtsLayer3RncHandleIurCellSwitchReq(
                node,
                umtsL3,
                msg,
                ueId,
                cellId);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_SWITCH_CELL_RESPONSE:
        {
            UmtsLayer3RncHandleIurCellSwitchRes(
                node,
                umtsL3,
                msg,
                ueId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RNSAP_MSG_TYPE_REPORT_AMRLC_ERROR:
        {
            UInt32 cellId;
            ERROR_Assert(sizeof(cellId) == additionInfoSize,
                " Wrong IUR Header format.");
            memcpy(&cellId, additionInfo, sizeof(cellId));
            UmtsLayer3RncHandleAmRlcErr(
                node,
                umtsL3,
                ueId,
                cellId,
                MESSAGE_ReturnPacket(msg),
                MESSAGE_ReturnPacketSize(msg));
            MESSAGE_Free(node, msg);
            break;
        }
        default:
            ERROR_ReportError("UmtsLayer3RncHandleIurMsg: "
                "Received unknown type of message from RNC.");
    }
}


// /**
// FUNCTION   :: UmtsLayer3RncHandlePacketFromUe
// LAYER      :: Layer 3
// PURPOSE    :: Handle L3 message from UE
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + interfaceIndex : int         : Interface from which packet was received
// + ueId      : NodeId           : The ue id originating the message
// + rbId      : UInt8            : RB ID the message belongs to
// + srcAddr   : Address          : Source node address
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncHandlePacketFromUe(Node *node,
                                     UmtsLayer3Data *umtsL3,
                                     Message *msg,
                                     int interfaceIndex,
                                     NodeId ueId,
                                     char rbId,
                                     Address srcAddr)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    if (rbId == UMTS_CCCH_RB_ID || rbId == UMTS_BCCH_RB_ID ||
        rbId == UMTS_PCCH_RB_ID || rbId == UMTS_CTCH_RB_ID)
    {
        UmtsLayer3SrncHandleUePacket(
            node,
            umtsL3,
            msg,
            interfaceIndex,
            ueId,
            rbId,
            srcAddr);
    }
    else
    {
        UmtsUeSrncMap::iterator iter = rncL3->ueSrncMap->find(ueId);

        if (rncL3->ueSrncMap->end() == iter
            || iter->second->srncId == node->nodeId)
        {
            UmtsLayer3SrncHandleUePacket(
                node,
                umtsL3,
                msg,
                interfaceIndex,
                ueId,
                rbId,
                srcAddr);
        }
        else
        {
            UmtsLayer3RncSendIurMsg(
                node,
                umtsL3,
                msg,
                ueId,
                iter->second->srncId,
                UMTS_RNSAP_MSG_TYPE_UPLINK_TRANSFER,
                &rbId,
                sizeof(rbId));
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleIubMessage
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Iub message from Nodeb, it may be a NBAP
//               message or a RRC message originated from a UE
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message containing the packet
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + interfaceIndex : int         : Interface from which packet was received
// + srcNodeId : NodeId           : The source node id of the message
// + rbIdOrMsgType: UInt8         : RB ID or NBAP message type
// + srcAddr   : Address          : Source node address
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3RncHandleIubMessage(Node *node,
                                   Message *msg,
                                   UmtsLayer3Data *umtsL3,
                                   int interfaceIndex,
                                   NodeId srcNodeId,
                                   UInt8 rbIdOrMsgType,
                                   Address srcAddr)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsLayer3NodebInfo *nodeBInfo = NULL;
    nodeBInfo = (UmtsLayer3NodebInfo *)
              FindItem(node,
                       rncL3->nodebList,
                       sizeof(NodeId) + sizeof(Address),
                       (char *) &(interfaceIndex),
                       sizeof(int));

    ERROR_Assert(nodeBInfo, "UmtsLayer3RncHandleIubMessage: "
        "cannot find NodeB info. ");

    BOOL msgReUse = FALSE;
    //If the source node is ANY_NODEID, then it is a NBAP message
    if (srcNodeId == ANY_NODEID)
    {
        UmtsNbapMessageType nbapMsgType =
            (UmtsNbapMessageType)rbIdOrMsgType;
        unsigned char transctId;
        NodeId ueId;
        UmtsRemoveNbapHeader(node,
                             msg,
                             &transctId,
                             &ueId);

        switch (nbapMsgType)
        {
            case UMTS_NBAP_MSG_TYPE_RRC_SETUP_RESPONSE:
            {
                UmtsLayer3RncHandleRrcSetupResFromNodeb(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    nodeBInfo->nodeId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_RRC_RELEASE_RESPONSE:
            {
                UmtsLayer3RncHandleRrcReleaseResFromNodeb(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    nodeBInfo->nodeId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_RADIO_BEARER_SETUP_RESPONSE:
            {
                UmtsLayer3RncHandleRbSetupResFromNodeb(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    nodeBInfo->nodeId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_RADIO_BEARER_RELEASE_RESPONSE:
            {
                UmtsLayer3RncHandleRbReleaseResFromNodeb(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    nodeBInfo->nodeId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_JOIN_ACTIVE_SET_RESPONSE:
            {
                UmtsLayer3RncHandleJoinActvSetResFromNodeb(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    nodeBInfo->nodeId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_LEAVE_ACTIVE_SET_RESPONSE:
            {
                UmtsLayer3RncHandleLeaveActvSetResFromNodeb(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    nodeBInfo->nodeId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_SWITCH_CELL_RESPONSE:
            {
                msgReUse = UmtsLayer3RncHandleCellSwitchResFromNodeb(
                                node,
                                umtsL3,
                                msg,
                                nodeBInfo->nodeId,
                                ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_REPORT_AMRLC_ERROR:
            {
                msgReUse = UmtsLayer3RncHandleAmRlcErrRepFromNodeb(
                                node,
                                umtsL3,
                                msg,
                                nodeBInfo->nodeId,
                                ueId);
                break;
            }
            default:
                ERROR_ReportError("UmtsLayer3RncHandleIubMessage: "
                    "Received unknown type of message from NodeB.");
        }
        if (FALSE == msgReUse)
        {
            MESSAGE_Free(node, msg);
        }
    }
    //Otherwise, it is a RRC message originated from a UE
    else
    {
        UmtsLayer3RncHandlePacketFromUe(node,
                                        umtsL3,
                                        msg,
                                        interfaceIndex,
                                        srcNodeId,
                                        rbIdOrMsgType,
                                        srcAddr);
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleRanapMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RANAP message from the SGSN.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + msgType        : UmtsRanapMessageType   : RANAP message type
// + NodeId         : ueId                   : UE ID
// + srcAddr        : Address                : source node address
// RETURN           :: void
// **/
static
void UmtsLayer3RncHandleRanapMsg(
        Node *node,
        UmtsLayer3Data *umtsL3,
        Message *msg,
        UmtsRanapMessageType msgType,
        NodeId nodeId,
        Address srcAddr)
{
    switch (msgType)
    {
        case UMTS_RANAP_MSG_TYPE_DIRECT_TRANSFER:
        {
            UmtsLayer3RncHandleRanapDirectTransfer(
                node,
                umtsL3,
                msg,
                nodeId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_PAGING:
        {
            UmtsLayer3RncHandleRanapPaging(
                node,
                umtsL3,
                msg);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_RAB_ASSIGNMENT_REQUEST:
        {
            UmtsLayer3RncHandleRabAssgtReq(
                node,
                umtsL3,
                msg,
                nodeId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_RAB_ASSIGNMENT_RELEASE:
        {
            UmtsLayer3RncHandleRabRelease(
                node,
                umtsL3,
                msg,
                nodeId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_IU_RELEASE_COMMAND:
        {
            UmtsLayer3RncHandleIuRelease(
                node,
                umtsL3,
                msg,
                nodeId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_CELL_LOOKUP_REPLY:
        {
            UmtsLayer3RncHandleCellLookupReply(
                node,
                umtsL3,
                msg);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_IUR_MESSAGE_FORWARD:
        {
            UmtsBackboneMessageType backboneMsgType;
            char info[UMTS_BACKBONE_HDR_MAX_INFO_SIZE];
            int infoSize = UMTS_BACKBONE_HDR_MAX_INFO_SIZE;
            UmtsRemoveBackboneHeader(node,
                                     msg,
                                     &backboneMsgType,
                                     info,
                                     infoSize);

            ERROR_Assert(backboneMsgType
                == UMTS_BACKBONE_MSG_TYPE_IUR,
                "RANAP only forwards IUR message");

            int index = 0;
            UmtsRnsapMessageType rnaspMsgType
                = (UmtsRnsapMessageType)(info[index++]);
            NodeId ueId;
            memcpy(&ueId, &info[index], sizeof(ueId));
            index += sizeof(ueId);

            UmtsLayer3RncHandleIurMsg(
                node,
                umtsL3,
                msg,
                rnaspMsgType,
                ueId,
                &info[index],
                infoSize - index,
                srcAddr);
            //Note:: whether message should be freed is decided
            //by individual handlers for Iur messages.
            break;
        }
        default:
        {
            char infoStr[MAX_STRING_LENGTH];
            sprintf(infoStr,
                    "UMTS Layer3: Node%d(RNC) receives a RANAP message "
                    "with an unknown RANAP message type as %d",
                    node->nodeId, msgType);
            ERROR_ReportWarning(infoStr);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleGtpMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP message
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + ueId      : UInt32           : ID of the UE
// + srcAddr   : Address          : Source Address
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3RncHandleGtpMsg(Node *node,
                               UmtsLayer3Data *umtsL3,
                               Message *msg,
                               UInt32 ueId,
                               Address srcAddr)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);
    UmtsGtpHeader gtpHeader;
    char extHeader[MAX_STRING_LENGTH];
    int extHeaderSize;
    UmtsRrcUeDataInRnc* ueRrc = NULL;
    UmtsRrcRabInfo* rabInfo = NULL;
    int i;

    // retrieve UE ID
    UmtsRemoveGtpHeader(node, msg, &gtpHeader, extHeader, &extHeaderSize);
    ERROR_Assert(gtpHeader.msgType == UMTS_GTP_G_PDU,
                 "UMTS: Wrong GTP message type!");

    if (DEBUG_GTP)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("receives GTP message to UE %d\n", ueId);
    }

//GuiStart
    if (node->guiOption == TRUE)
    {
        NodeAddress previousHopNodeId;

        previousHopNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                node,
                                srcAddr);
        GUI_Receive(previousHopNodeId, node->nodeId, GUI_NETWORK_LAYER,
                    GUI_DEFAULT_DATA_TYPE,
                    DEFAULT_INTERFACE,
                    DEFAULT_INTERFACE,
                    node->getNodeTime());
    }
//GuiEnd

    // update statistics
    rncL3->stat.numDataPacketFromSgsn ++;

    // find UE info
    ueRrc = UmtsLayer3RncFindUeRrc(node, rncL3, ueId);

    if (DEBUG_ASSERT)
    {
        ERROR_Assert(ueRrc, "UMTS: Cannot find UE info at RNC!");
    }

    if (ueRrc == NULL)
    {
        // no RRC info, free the message and update stat
        MESSAGE_Free(node, msg);
        rncL3->stat.numDataPacketFromSgsnDroppedNoRrc ++;

        UmtsLayer3RncReportIuRelease(
                        node,
                        umtsL3,
                        ueId,
                        UMTS_LAYER3_CN_DOMAIN_PS,
                        FALSE);
        return;
    }

    // find RB info
    for (i = 0; i < UMTS_MAX_RAB_SETUP; i++)
    {
        rabInfo = ueRrc->rabInfos[i];

        if (rabInfo != NULL &&
            rabInfo->cnDomainId == UMTS_LAYER3_CN_DOMAIN_PS &&
            (unsigned)(rabInfo->connMap.tunnelMap.dlTeId) == gtpHeader.teId)
        {
            break;
        }
    }

    if (DEBUG_ASSERT)
    {
        ERROR_Assert(rabInfo, "UMTS: Cannot find RAB info!");
    }

    if (rabInfo == NULL)
    {
        // no RAB info, free the message and update stat
        MESSAGE_Free(node, msg);
        rncL3->stat.numDataPacketFromSgsnDroppedNoRab ++;
    }

    UmtsLayer3RncSendPktToUeOnDch(
        node,
        umtsL3,
        msg,
        ueId,
        rabInfo->rbIds[0]);

    // update statistics
    rncL3->stat.numPsDataPacketToUe ++;
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleCsPacketFromMsc
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CS data packet from the MSC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + ueId      : UInt32           : ID of the UE
// + srcAddr   : Address          : Source Address
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3RncHandleCsPacketFromMsc(
       Node *node,
       UmtsLayer3Data *umtsL3,
       Message *msg,
       NodeId ueId,
       char rabId,
       Address srcAddr)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    UmtsRrcUeDataInRnc* ueRrc = NULL;
    UmtsRrcRabInfo* rabInfo = NULL;

//GuiStart
    if (node->guiOption == TRUE)
    {
        NodeAddress previousHopNodeId;

        previousHopNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                node,
                                srcAddr);
        GUI_Receive(previousHopNodeId, node->nodeId, GUI_NETWORK_LAYER,
                    GUI_DEFAULT_DATA_TYPE,
                    DEFAULT_INTERFACE,
                    DEFAULT_INTERFACE,
                    node->getNodeTime());
    }
//GuiEnd

    // update statistics
    rncL3->stat.numDataPacketFromSgsn ++;

    // find UE info
    ueRrc = UmtsLayer3RncFindUeRrc(node, rncL3, ueId);

    if (DEBUG_ASSERT)
    {
        ERROR_Assert(ueRrc, "UMTS: Cannot find UE info at RNC!");
    }

    if (ueRrc == NULL)
    {
        // no RRC info, free the message and update stat
        MESSAGE_Free(node, msg);
        rncL3->stat.numDataPacketFromSgsnDroppedNoRrc ++;

        UmtsLayer3RncReportIuRelease(
                        node,
                        umtsL3,
                        ueId,
                        UMTS_LAYER3_CN_DOMAIN_CS,
                        FALSE);
        return;
    }

    rabInfo = ueRrc->rabInfos[(int)rabId];

    if (DEBUG_ASSERT)
    {
        ERROR_Assert(rabInfo, "UMTS: Cannot find RAB info!");
    }

    if (rabInfo == NULL)
    {
        // no RAB info, free the message and update stat
        MESSAGE_Free(node, msg);
        rncL3->stat.numDataPacketFromSgsnDroppedNoRab ++;
    }

    UmtsLayer3RncSendPktToUeOnDch(
        node,
        umtsL3,
        msg,
        ueId,
        rabInfo->rbIds[0]);

    // update statistics
    rncL3->stat.numCsDataPacketToUe ++;
}

// /**
// FUNCTION   :: UmtsLayer3RncHandleTimerMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// RETURN     :: NULL
// **/
static
void UmtsLayer3RncHandleTimerMsg(Node *node,
                                 UmtsLayer3Data *umtsL3,
                                 Message *msg)
{
    UmtsLayer3TimerType timerType;
    UmtsLayer3Timer *timerInfo;

    // get timer type first
    timerInfo = (UmtsLayer3Timer *) MESSAGE_ReturnInfo(msg);
    timerType = timerInfo->timerType;

    switch (timerType)
    {
        case UMTS_LAYER3_TIMER_T3101:
        {
            // the UE is powered on
            UmtsLayer3RncHandleT3101(node, umtsL3, msg);

            break;
        }
        case UMTS_LAYER3_TIMER_MEASREPCHECK:
        {
            UmtsLayer3RncHandleTimerMeasRepCheck(node, umtsL3, msg);

            break;
        }
        case UMTS_LAYER3_TIMER_HELLO:
        {
            UmtsLayer3SendHelloPacket(node, umtsL3, ANY_INTERFACE);

            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];

            sprintf(errStr,
                    "UMTS Layer3: Node%d(Rnc) receivers a timer"
                    "event with unknown type %d",
                    node->nodeId,
                    timerType);
            ERROR_ReportWarning(errStr);

            break;
        }
    }
}

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: UmtsLayer3RncInit
// LAYER      :: Layer 3
// PURPOSE    :: Initialize RNC data at UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3RncInit(Node *node,
                       const NodeInput *nodeInput,
                       UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Rnc *rncL3;

    // initialize the basic UMTS layer3 data
    rncL3 = (UmtsLayer3Rnc *) MEM_malloc(sizeof(UmtsLayer3Rnc));
    ERROR_Assert(rncL3 != NULL, "UMTS: Out of memory!");
    memset(rncL3, 0, sizeof(UmtsLayer3Rnc));

    umtsL3->dataPtr = (void *) rncL3;

    // read configuration parameters
    UmtsLayer3RncInitParameter(node, nodeInput, rncL3);

    if (DEBUG_PARAMETER)
    {
        UmtsLayer3RncPrintParameter(node, rncL3);
    }

    // initialize the lists
    ListInit(node, &(rncL3->nodebList));
    ListInit(node, &(rncL3->neighRncList));
    ListInit(node, &(rncL3->addrInfoList));
    rncL3->sgsnInfo.nodeId = ANY_NODEID;

    rncL3->ueRrcs = new std::list<UmtsRrcUeDataInRnc*>;
    rncL3->nodebRrcs = new std::list<UmtsRrcNodebDataInRnc*>;
    rncL3->cellCacheMap = new UmtsCellCacheMap;
    rncL3->ueSrncMap = new UmtsUeSrncMap;
    rncL3->ulCacType = UMTS_DEFAULT_CAC_TYPE;
    rncL3->dlCacType = UMTS_DEFAULT_CAC_TYPE;// UMTS_CAC_POWER_BASED;

    rncL3->rabServQueue = new UmtsRabAssgtReqList;

    // broadcast hello packet to discover my nodeBs and neighbor RNCs, SGSNs
    UmtsLayer3SetTimer(node, umtsL3,
                       UMTS_LAYER3_TIMER_HELLO,
                       UMTS_LAYER3_TIMER_HELLO_START,
                       NULL, NULL, 0);
}

// /**
// FUNCTION   :: UmtsLayer3RncLayer
// LAYER      :: Layer 3
// PURPOSE    :: Handle RNC specific timers and layer messages.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message for node to interpret
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3RncLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3)
{
    switch((msg->eventType))
    {
        case MSG_NETWORK_CELLULAR_TimerExpired:
        {
            UmtsLayer3RncHandleTimerMsg(node, umtsL3, msg);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(RNC) got an event with a "
                    "unknown type (%d)",
                    node->nodeId, MESSAGE_GetEvent(msg));
            ERROR_ReportWarning(errStr);
        }
    }

    MESSAGE_Free(node, msg);
}

// /**
// FUNCTION   :: UmtsLayer3RncHandlePacket
// LAYER      :: Layer 3
// PURPOSE    :: Handle packets received from wired network
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message containing the packet
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + interfaceIndex : int         : Interface from which packet was received
// + srcAddr   : Address          : Address of the source node
// RETURN     :: void : NULL
// **/
void UmtsLayer3RncHandlePacket(Node *node,
                               Message *msg,
                               UmtsLayer3Data *umtsL3,
                               int interfaceIndex,
                               Address srcAddr)
{
    UmtsBackboneMessageType backboneMsgType;
    char info[UMTS_BACKBONE_HDR_MAX_INFO_SIZE];
    int infoSize = UMTS_BACKBONE_HDR_MAX_INFO_SIZE;

    UmtsRemoveBackboneHeader(node,
                             msg,
                             &backboneMsgType,
                             info,
                             infoSize);

    switch (backboneMsgType)
    {
        case UMTS_BACKBONE_MSG_TYPE_IUB:
        {
            //Receive a message from NodeB
            NodeId srcNodeId;
            memcpy(&srcNodeId, info, sizeof(NodeId));
            UInt32 rbIdOrMsgType = info[sizeof(NodeId)];
            UmtsLayer3RncHandleIubMessage(node,
                                          msg,
                                          umtsL3,
                                          interfaceIndex,
                                          srcNodeId,
                                          (UInt8)rbIdOrMsgType,
                                          srcAddr);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_HELLO:
        {
            // this is the hello packet for discovering connectivity
            char pd;
            char tiSpd;
            char msgType;
            UmtsLayer3RemoveHeader(node, msg, &pd, &tiSpd, &msgType);
            UmtsLayer3RncHandleHelloPacket(node,
                                           msg,
                                           umtsL3,
                                           interfaceIndex,
                                           srcAddr);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_IU_CSDATA:
        {
            //Receive a message from core network conaining CS domain data
            NodeId ueId;
            char rabId;
            memcpy(&ueId, &info[0], sizeof(ueId));
            rabId = info[sizeof(ueId)];
            UmtsLayer3RncHandleCsPacketFromMsc(
                node,
                umtsL3,
                msg,
                ueId,
                rabId,
                srcAddr);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_GTP:
        {
            //Receive a message from core network conaining GTP data
            //(PS domain data and core network signalling)
            NodeId ueId;
            memcpy(&ueId, &info[0], sizeof(NodeId));
            UmtsLayer3RncHandleGtpMsg(node,
                                      umtsL3,
                                      msg,
                                      ueId,
                                      srcAddr);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_IU:
        {
            //Receive a message from SGSN conaining RANAP signalling message
            UmtsRanapMessageType ranapMsgType = (UmtsRanapMessageType)
                                                (info[0]);
            NodeId nodeId;
            memcpy(&nodeId, &info[1], sizeof(NodeId));
            UmtsLayer3RncHandleRanapMsg(node,
                                        umtsL3,
                                        msg,
                                        ranapMsgType,
                                        nodeId,
                                        srcAddr);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_IUR:
        {
            //Receive a message from another RNC
            int index = 0;
            UmtsRnsapMessageType rnaspMsgType = (UmtsRnsapMessageType)
                                                (info[index++]);
            NodeId ueId;
            memcpy(&ueId, &info[index], sizeof(ueId));
            index += sizeof(ueId);
            UmtsLayer3RncHandleIurMsg(node,
                                      umtsL3,
                                      msg,
                                      rnaspMsgType,
                                      ueId,
                                      &info[index],
                                      infoSize - index,
                                      srcAddr);
            break;
        }
        default:
        {
            char infoStr[MAX_STRING_LENGTH];
            sprintf(infoStr,
                    "UMTS Layer3: Node%d(RNC) receives a backbone message "
                    "with an unknown Backbone message type as %d",
                    node->nodeId,
                    backboneMsgType);
            ERROR_ReportWarning(infoStr);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3RncFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print RNC specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3RncFinalize(Node *node, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Rnc *rncL3 = (UmtsLayer3Rnc *) (umtsL3->dataPtr);

    if (umtsL3->collectStatistics)
    {
        UmtsLayer3RncPrintStats(node, rncL3);
    }

    // Free the lists
    ListFree(node, rncL3->nodebList, FALSE);
    ListFree(node, rncL3->neighRncList, FALSE);
    ListFree(node, rncL3->addrInfoList, FALSE);

    std::list<UmtsRrcUeDataInRnc*>::iterator uePos;
    std::list<UmtsRrcNodebDataInRnc*>::iterator nodebPos;
    for (uePos = rncL3->ueRrcs->begin();
         uePos != rncL3->ueRrcs->end();
         ++uePos)
    {
        UmtsLayer3RncFinalizeUeRrc(node, *uePos);
        *uePos = NULL;
    }
    rncL3->ueRrcs->remove(NULL);
    delete rncL3->ueRrcs;

    for (nodebPos = rncL3->nodebRrcs->begin();
         nodebPos != rncL3->nodebRrcs->end();
         ++nodebPos)
    {
        UmtsLayer3RncFinalizeNodebRrc(node, *nodebPos);
        *nodebPos = NULL;
    }
    rncL3->nodebRrcs->remove(NULL);
    delete rncL3->nodebRrcs;

    for (UmtsCellCacheMap::iterator it = rncL3->cellCacheMap->begin();
         it != rncL3->cellCacheMap->end();)
    {
        UmtsCellCacheMap::iterator itTemp = it ++;
        delete itTemp->second;
        rncL3->cellCacheMap->erase(itTemp);
    }
    delete rncL3->cellCacheMap;

    UmtsUeSrncMap::iterator mapIter;
    for (mapIter = rncL3->ueSrncMap->begin();
        mapIter != rncL3->ueSrncMap->end();
        )
    {
        UmtsUeSrncMap::iterator mapIterTemp;

        mapIterTemp = mapIter ++;
        MEM_free(mapIterTemp->second);
        rncL3->ueSrncMap->erase(mapIterTemp);
    }
    delete rncL3->ueSrncMap;

    UmtsClearStlContDeep(rncL3->rabServQueue);
    delete rncL3->rabServQueue;

    MEM_free(rncL3);
    umtsL3->dataPtr = NULL;
}

