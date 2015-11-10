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
#include "partition.h"
#include "network_ip.h"
#include "antenna.h"

#include <list>
#include "phy_cellular.h"
#include "phy_umts.h"

#define DEBUG_SPECIFIED_NODE_ID 0 // DEFAULT shoud be 0
#define DEBUG_SINGLE_NODE   (node->nodeId == DEBUG_SPECIFIED_NODE_ID)
#define DEBUG_PHY_CHANNEL (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_TX (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RX (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG    (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_SNR (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_BURST (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_DETAIL (0 || (0 &&  DEBUG_SINGLE_NODE))

// /**
// FUNCTION   :: PhyUmtsPrintBurstInfo
// LAYER      :: PHY
// PURPOSE    :: Print out the burst detailed info for debug purpose.
// PARAMETERS ::
// + node      : Node*                 : Pointer to node.
// + burstInfo : PhyUmtsCdmaBurstInfo* : Pointer to info of a burst
// RETURN     :: void : NULL
// **/
static
void PhyUmtsPrintBurstInfo(Node* node, PhyUmtsCdmaBurstInfo* burstInfo)
{
    if (DEBUG_BURST || DEBUG_DETAIL)
    {
        printf("CDMA Burst info:\n "
               "chtype %d, cdDriection %d, sf %d, "
               "chCodeIndex %d, scCodeIdnex %d,\n "
               "chPahse %d, gainFactor %f, moduType %d, "
               "codingType %d\n burstPower %e inter"
               "ferencePower %e\n",
               burstInfo->phChType,burstInfo->chDirection,
               burstInfo->spreadFactor, burstInfo->chCodeIndex,
               burstInfo->scCodeIndex, burstInfo->chPhase,
               burstInfo->gainFactor, burstInfo->moduType,
               burstInfo->codingType, burstInfo->burstPower_mW,
               burstInfo->interferencePower_mW);
        printf("\n");
    }
}

// /**
// FUNCTION   :: PhyUmtsPrintRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Print out the maintained info of a receiving signal or
//               interference signal for debug purpose.
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + rxMsgInfo : PhyUmtsRxMsgInfo* : Pointer to info of a signal
// RETURN     :: void : NULL
// **/
static
void PhyUmtsPrintRxMsgInfo(Node* node, PhyUmtsRxMsgInfo* rxMsgInfo)
{
    Message* tmpMsg;
    PhyUmtsCdmaBurstInfo* burstInfo;
    int burst = 0;

    printf("\nNode%d receiving msg information of the signal\n",
           node->nodeId);

    tmpMsg = rxMsgInfo->rxMsg;

    while (tmpMsg != NULL)
    {
        burstInfo = (PhyUmtsCdmaBurstInfo*)MESSAGE_ReturnInfo(tmpMsg);

         printf("\nCDMA Burst info:\n "
                   "burst %d chtype %d, cdDriection %d, sf %d, "
                   "chCodeIndex %d, scCodeIdnex %d,\n "
                   "chPahse %d, gainFactor %f, moduType %d, "
                   "codingType %d\n burstPower %e inter"
                   "ferencePower %e\n",

                   ++ burst,
                   burstInfo->phChType,burstInfo->chDirection,
                   burstInfo->spreadFactor, burstInfo->chCodeIndex,
                   burstInfo->scCodeIndex, burstInfo->chPhase,
                   burstInfo->gainFactor, burstInfo->moduType,
                   burstInfo->codingType, rxMsgInfo->rxburstPower_mW,
                   burstInfo->interferencePower_mW);
        printf("\n");
        tmpMsg = tmpMsg->next;
    }
}

// /**
// FUNCTION   :: PhyUmtsCreateRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Retrive the burst information from the signal and create
//               a structure to store them
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + PhyUmts   : PhyUmtsData*       : Pointer to the UMTS PHY structure
// + propRxInfo: PropRxInfo*        : Information of the arrived signal
// + rxPower_mW: double             : Receiving power of the signal
// RETURN     :: PhyUmtsRxMsgInfo* : Created structure keepting burst info
// **/
static
PhyUmtsRxMsgInfo* PhyUmtsCreateRxMsgInfo(
                       Node* node,
                       PhyUmtsData* PhyUmts,
                       PropRxInfo* propRxInfo,
                       double rxPower_mW)
{
    PhyUmtsRxMsgInfo* rxMsgInfo;
    PhyUmtsCdmaBurstInfo* burstInfo;
    double totalGainfactor = 0.0;
    int numBursts = 0;

    Message* msgList;
    Message* tmpMsg;

    //clocktype relativeStartTime;
    clocktype currentTime;
    //clocktype startTime;
    //clocktype endTime;

    rxMsgInfo = (PhyUmtsRxMsgInfo*) MEM_malloc(sizeof(PhyUmtsRxMsgInfo));
    ERROR_Assert(rxMsgInfo != NULL, "PHY_UMTS: Out of memory!");
    memset(rxMsgInfo, 0, sizeof(PhyUmtsRxMsgInfo));

    currentTime = propRxInfo->rxStartTime;

    rxMsgInfo->propRxInfo = propRxInfo;

    rxMsgInfo->rxBeginTime = currentTime;
    rxMsgInfo->rxEndTime = currentTime + propRxInfo->duration;


    msgList = UmtsUnpackMessage(node,propRxInfo->txMsg,FALSE,FALSE);
    rxMsgInfo->rxMsg = msgList;

    ERROR_Assert(msgList != NULL, "PHY_UMTS: Message is null!");
    // get the information of bursts

    tmpMsg = msgList;

    while (tmpMsg != NULL)
    {

        burstInfo = (PhyUmtsCdmaBurstInfo*)MESSAGE_ReturnInfo(tmpMsg);

        if (burstInfo != NULL)
        {
            burstInfo->interferencePower_mW = 0.0;
            burstInfo->rxStartTime = currentTime;
            burstInfo->inError = FALSE;
            burstInfo->rxEvaluatedEndTime =
                currentTime + (burstInfo->burstEndTime -
                burstInfo->burstStartTime);
            totalGainfactor += burstInfo->gainFactor;
            numBursts ++;
        }

        tmpMsg = tmpMsg->next;
    }

    rxMsgInfo->rxburstPower_mW = rxPower_mW / totalGainfactor;
    rxMsgInfo->numBursts = numBursts;


    if (DEBUG_DETAIL)
    {
        PhyUmtsPrintRxMsgInfo(node, rxMsgInfo);
    }

    return rxMsgInfo;
}

// /**
// FUNCTION   :: PhyUmtsFreeRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Free the structure keeping info of a signal.
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + rxMsgInfo : PhyUmtsRxMsgInfo*  : Pointer to the struc of a signal
// RETURN     :: void               : NULL
// **/
static
void PhyUmtsFreeRxMsgInfo(Node* node, PhyUmtsRxMsgInfo* rxMsgInfo)
{
    Message* msg;
    Message* tmpMsg;

    // free messages
    msg = rxMsgInfo->rxMsg;
    while (msg != NULL)
    {
        tmpMsg = msg;
        msg = msg->next;
        MESSAGE_Free(node, tmpMsg);
    }

    // free the rxMsgInfo itself
    rxMsgInfo->rxMsg = NULL;
    MEM_free(rxMsgInfo);
}

#if 0
// /**
// FUNCTION   :: PhyUmtsGetRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Find the info of a signal from the maintained list.
//               If requested, may remove it from the list
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + rxMsgList : PhyUmtsRxMsgInfo* : List of signal info structures
// + rxMsg     : Message*           : Message of the signal as a key
// + removeIt  : BOOL               : Indicate whether remove from the list
// RETURN     :: PhyUmtsRxMsgInfo* : Pointer to the structure if found
// **/
static
PhyUmtsRxMsgInfo* PhyUmtsGetRxMsgInfo(Node* node,
                                      PhyUmtsRxMsgInfo** rxMsgList,
                                      Message* rxMsg,
                                      BOOL removeIt)
{
    PhyUmtsRxMsgInfo* rxMsgInfo;
    PhyUmtsRxMsgInfo* prevMsgInfo;

    rxMsgInfo = *rxMsgList;
    prevMsgInfo = rxMsgInfo;
    while (rxMsgInfo != NULL)
    {
        if (rxMsgInfo->rxMsg == rxMsg)
        { // found it
            if (removeIt)
            {
                if (prevMsgInfo == rxMsgInfo)
                {
                    // first msg in the list
                    *rxMsgList = rxMsgInfo->next;
                }
                else {
                    prevMsgInfo->next = rxMsgInfo->next;
                }
            }

            break;
        }

        prevMsgInfo = rxMsgInfo;
        rxMsgInfo = rxMsgInfo->next;
    }

    return rxMsgInfo;
}
#endif

// /**
// FUNCTION   :: PhyUmtsGetRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Find the info of a signal from the maintained list.
//               If requested, may remove it from the list
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + rxMsgList : PhyUmtsRxMsgInfo*  : List of signal info structures
// + propRxInfo: PropRxInfo*        : prop Info of  this propagation
// + removeIt  : BOOL               : Indicate whether remove from the list
// RETURN     :: PhyUmtsRxMsgInfo* : Pointer to the structure if found
// **/
static
PhyUmtsRxMsgInfo* PhyUmtsGetRxMsgInfo(Node* node,
                                      PhyUmtsRxMsgInfo** rxMsgList,
                                      PropRxInfo* propRxInfo,
                                      BOOL removeIt)
{
    PhyUmtsRxMsgInfo* rxMsgInfo;
    PhyUmtsRxMsgInfo* prevMsgInfo;

    rxMsgInfo = *rxMsgList;
    prevMsgInfo = rxMsgInfo;
    while (rxMsgInfo != NULL)
    {
        if (propRxInfo == rxMsgInfo->propRxInfo)
        {
           if (removeIt)
            {
                if (prevMsgInfo == rxMsgInfo)
                {
                    // first msg in the list
                    *rxMsgList = rxMsgInfo->next;
                }
                else {
                    prevMsgInfo->next = rxMsgInfo->next;
                }
            }

            break;
        }
        prevMsgInfo = rxMsgInfo;
        rxMsgInfo = rxMsgInfo->next;
    }

    return rxMsgInfo;
}

// /**
// FUNCTION   :: PhyUmtsGetModCodeType
// LAYER      :: PHY
// PURPOSE    :: Get the ModcodeType and the coding rate.
// PARAMETERS ::
// + codingType      : UmtsCodingType          : coding type.
// + moduType        : UmtsModulationType      : modulation type
// + *codingRate     : double                  : coding rate
// RETURN     :: int : modCodeType
// **/
int PhyUmtsGetModCodeType(UmtsCodingType codingType,
                          UmtsModulationType moduType,
                          double *codingRate)
{
    int modCodeType = 0;
    *codingRate = 3.0;

    if (moduType == UMTS_MOD_QPSK)
    {
        if (codingType == UMTS_CONV_3)
        {
            modCodeType = 1;
            *codingRate = 3.0;
        }
        else if (codingType == UMTS_CONV_2)
        {
            modCodeType = 2;
            *codingRate = 2.0;
        }
    }
    else if (moduType == UMTS_MOD_16QAM)
    {
        if (codingType == UMTS_TURBO_3)
        {
            modCodeType = 3;
            *codingRate = 3.0;
        }
    }

    return modCodeType;
}


// /**
// FUNCTION   :: PhyUmtsCheckBurstError
// LAYER      :: PHY
// PURPOSE    :: Check if a burst of the receiving signal has error.
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + phyIndex  : int               : Index of the PHY
// + rxMsgInfo : PhyUmtsRxMsgInfo* : Info of signal containing the burst
// + cdmaBurst: PhyUmtsCdmaBurst*  : Info of the cdma burst to be checked
// RETURN     :: void : NULL
// **/
static
void PhyUmtsCheckBurstError(Node* node,
                            int phyIndex,
                            PhyUmtsRxMsgInfo* rxMsgInfo,
                            PhyUmtsCdmaBurstInfo* cdmaBurst)
{
    clocktype currentTime;
    clocktype startTime;
    clocktype endTime;
    double sinr;
    double BER;
    double signalPower = 0.0;
    double interferencePower = 0.0;
    double numBits = 0.0;
    double noisePower_mW = 0.0;
    int modCodeType = 0;
    double codingRate = 3.0;
    int raw_numBits = 0;

    PhyData *thisPhy  = node->phyData[phyIndex];
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;

    signalPower = rxMsgInfo->rxburstPower_mW;
    interferencePower = cdmaBurst->interferencePower_mW;
    noisePower_mW = phyUmts->noisePower_mW;

    currentTime = node->getNodeTime();

    if (cdmaBurst->rxTimeEvaluated > cdmaBurst->rxStartTime)
    {
        startTime = cdmaBurst->rxTimeEvaluated;
    }
    else
    {
        startTime = cdmaBurst->rxStartTime;
    }

    if (currentTime <= cdmaBurst->rxEvaluatedEndTime)
    {
        endTime = currentTime;
    }
    else
    {
        endTime = cdmaBurst->rxEvaluatedEndTime;
    }

    if (startTime > endTime)
    {
        return;
    }

    cdmaBurst->inError = FALSE;
    signalPower = signalPower * cdmaBurst->gainFactor;

    sinr =
         signalPower /
        (interferencePower + noisePower_mW  /
        cdmaBurst->spreadFactor);

    cdmaBurst->cinr = sinr;
    cdmaBurst->burstPower_mW = signalPower;

    modCodeType = PhyUmtsGetModCodeType(cdmaBurst->codingType,
                          cdmaBurst->moduType,
                          &codingRate);

    BER = PHY_BER(thisPhy, modCodeType, sinr);

    if (DEBUG_SNR)
    {
        printf("Node%d check burst error from UE %d sf %d with "
                "rxPower_mW = %e, interferencePower_mW = %e "
                "sinr = %f dB BER = %f\n",
                node->nodeId, cdmaBurst->scCodeIndex,
                cdmaBurst->spreadFactor, signalPower,
                interferencePower,
                IN_DB(sinr), BER);
        PhyUmtsPrintBurstInfo(node, cdmaBurst);
    }

    if (BER != 0.0)
    {
        numBits = (double( endTime - startTime)
             * phyUmts->chipRate)/cdmaBurst->spreadFactor/ double(SECOND);

        raw_numBits = (int) (numBits / codingRate);

        double errorProbability = 1.0 - pow((1.0 - BER), raw_numBits);

        ERROR_Assert(errorProbability >= 0 &&
                             errorProbability <= 1.0,
                             "PHYUMTS: Wrong error probability!");

        double rand = RANDOM_erand(phyCellular->randSeed);

        if (errorProbability > rand)
        {
            cdmaBurst->inError = TRUE;
        }
    }
}

// /**
// FUNCTION   :: PhyUmtsCheckRxPacketError
// LAYER      :: PHY
// PURPOSE    :: Check if any receiving frame has error.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + phyIndex  : int           : Index of the PHY
// RETURN     :: void : NULL
// **/
static
void PhyUmtsCheckRxPacketError(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;

    PhyUmtsCdmaBurstInfo* cdmaBurst;
    PhyUmtsRxMsgInfo* rxMsgInfo;

    // go through all signals under receiving

    rxMsgInfo = phyUmts->rxMsgList;

    while (rxMsgInfo != NULL)
    {
        Message* msgList;
        msgList = rxMsgInfo->rxMsg;

        while (msgList != NULL)
        {
            // check each burst one by one
            cdmaBurst = (PhyUmtsCdmaBurstInfo*)MESSAGE_ReturnInfo(msgList);
            // only check if not already in error

            if (cdmaBurst->inError == FALSE)
            {
                if ((cdmaBurst->phChType != UMTS_PHYSICAL_PSCH) &&
                    (cdmaBurst->phChType != UMTS_PHYSICAL_SSCH))
                {
                    PhyUmtsCheckBurstError(node,
                                      phyIndex,
                                      rxMsgInfo,
                                      cdmaBurst);

                    cdmaBurst->rxTimeEvaluated = node->getNodeTime();

                    if (cdmaBurst->rxTimeEvaluated >=
                        cdmaBurst->rxEvaluatedEndTime)
                    {
                        cdmaBurst->rxTimeEvaluated =
                            cdmaBurst->rxEvaluatedEndTime;
                    }

                    if (cdmaBurst->inError)
                    {
                        phyUmts->stats.totalRxBurstsWithErrors++;
                    }
                } // cdmaBurst->phChType
            } // cdmaBurst->inError

            phyUmts->stats.totalRxBurstsToMac++;
            msgList = msgList->next;
        } // while (msgList)

        rxMsgInfo = rxMsgInfo->next;
    } // while (rxMsgInfo)
}

// /**
// FUNCTION   :: PhyUmtsCalculateInterference
// LAYER      :: PHY
// PURPOSE    :: Compute the interference power on each burst
//
// PARAMETERS ::
// + interferenceRxBurstPower_mW
//   : double    : interference power in mW
// + desiredCdmaBurst
//   : PhyUmtsCdmaBurstInfo*  : Pointer to desired CDMA burst
// + interferenceCdmaBurst
//   : PhyUmtsCdmaBurstInfo*  : Pointer to interference CDMA burst
//
// RETURN     :: void : NULL
// **/

static
void PhyUmtsCalculateInterference(
     double interferenceRxBurstPower_mW,
     PhyUmtsCdmaBurstInfo* desiredCdmaBurst,
     PhyUmtsCdmaBurstInfo* interferenceCdmaBurst)
{
    double   spradingGain;
    unsigned int scramblingcode;
    unsigned int channelizationCode;
    UmtsPhyChRelativePhase branchInfo;

    scramblingcode = desiredCdmaBurst->scCodeIndex;
    channelizationCode = desiredCdmaBurst->chCodeIndex;
    branchInfo = desiredCdmaBurst->chPhase;
    spradingGain = desiredCdmaBurst->spreadFactor;

    if (scramblingcode ==
        interferenceCdmaBurst->scCodeIndex &&
        channelizationCode ==
        interferenceCdmaBurst->chCodeIndex)
    {
        spradingGain = PHY_UMTS_DEFAULT_SPREAD_FACTOR;
    }

    if (branchInfo == UMTS_PHY_CH_BRANCH_IQ ||
        branchInfo == interferenceCdmaBurst->chPhase)
    {
        desiredCdmaBurst->interferencePower_mW +=
                 interferenceRxBurstPower_mW *
                 interferenceCdmaBurst->gainFactor / spradingGain;
    }
    else
    {
        if (interferenceCdmaBurst->chPhase == UMTS_PHY_CH_BRANCH_IQ)
        {
            desiredCdmaBurst->interferencePower_mW +=
                 (interferenceRxBurstPower_mW /
                 PHY_UMTS_BRANCH_FACTOR) *
                 interferenceCdmaBurst->gainFactor / spradingGain;
        }
    }
}

// /**
// FUNCTION   :: PhyUmtsCalculateTotalInterference
// LAYER      :: PHY
// PURPOSE    :: Compute the total interference power on each burst
//
// PARAMETERS ::
// + node         : Node*                 : Pointer to node.
// + rxMsgInfoL   : PhyUmtsRxMsgInfo*     : Pointer to PhyUmtsRxMsgInfo
// + cdmaBurst    : PhyUmtsCdmaBurstInfo* : Pointer to PhyUmtsCdmaBurstInfo
// + iflist       : BOOL                  : Select the print out info
//
// RETURN     :: void : NULL
// **/

static
void PhyUmtsCalculateTotalInterference(
    Node *node,
    PhyUmtsRxMsgInfo* rxMsgInfoL,
    PhyUmtsCdmaBurstInfo* cdmaBurst,
    BOOL iflist)
{
       clocktype burstStartTime;
    clocktype burstEndTime;
    clocktype listburstStartTime;
    clocktype listburstEndTime;

     burstStartTime = cdmaBurst->rxStartTime;
    burstEndTime = cdmaBurst->rxEvaluatedEndTime;

    while (rxMsgInfoL != NULL)
    {

        Message* msgListL2 = rxMsgInfoL->rxMsg;

        while (msgListL2 != NULL)
        {
            PhyUmtsCdmaBurstInfo* listburst;

            listburst = (PhyUmtsCdmaBurstInfo*)
                         MESSAGE_ReturnInfo(msgListL2);

            listburstStartTime = listburst->rxStartTime;
            listburstEndTime = listburst->rxEvaluatedEndTime;

            if ((cdmaBurst != listburst) &&
              (!(listburstStartTime >= burstEndTime ||
                   listburstEndTime <= burstStartTime)))
            {
                if (DEBUG_BURST)
                {
                    if (iflist)
                    {
                        printf(
                            "node %d: calculate IfList burst from node %d "
                            "interference to the studied CDMA burst\n",
                            node->nodeId, listburst->syncTimeInfo);
                    }
                    else
                    {
                        printf(
                            "node %d: calculate RxList burst from node %d "
                            "interference to the studied CDMA burst\n",
                            node->nodeId, listburst->syncTimeInfo);
                    }

                    PhyUmtsPrintBurstInfo(node, listburst);
                }

                PhyUmtsCalculateInterference(
                        rxMsgInfoL->rxburstPower_mW,
                        cdmaBurst,
                        listburst);

                if (DEBUG_BURST)
                {
                    if (iflist)
                    {
                        printf("node %d: after calculate IfList burst "
                                "interference to the studied CDMA burst\n",
                                node->nodeId);
                    }
                    else
                    {
                        printf(
                            "node %d: after calculate RxList burst "
                            "interference to the studied CDMA burst\n",
                            node->nodeId);
                    }

                    PhyUmtsPrintBurstInfo(node, cdmaBurst);
                }
            } // if ((cdmaBurst...))

            msgListL2 = msgListL2->next;
        } // while msgListL2, msg in L2

       rxMsgInfoL = rxMsgInfoL->next;
    } // while rxMsgInfoL
}

// /**
// FUNCTION   :: PhyUmtsSignalInterference
// LAYER      :: PHY
// PURPOSE    :: Recompute the interference power on each burst
//               Expired signals may be removed from maintained list
// PARAMETERS ::
// + node                 : Node*    : Pointer to node.
// + phyIndex             : int      : Index of the PHY
// + channelIndex         : int      : Index of the channel
// + msg                  : Message* : Message under receiving, not used
//                                     here
//
// RETURN     :: void : NULL
// **/
static
void PhyUmtsSignalInterference(
         Node *node,
         int phyIndex,
         int channelIndex,
         Message *msg)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
    PhyUmtsRxMsgInfo* rxMsgList = NULL;
    PhyUmtsRxMsgInfo* ifMsgList = NULL;
    PhyUmtsRxMsgInfo* rxMsgInfo = NULL;

    int numSignals;
    PropRxInfo *ptr = NULL;

    rxMsgList = phyUmts->rxMsgList;
    ifMsgList = phyUmts->ifMsgList;
    phyUmts->rxMsgList = NULL;
    phyUmts->ifMsgList = NULL;

    numSignals = node->propData[channelIndex].numSignals;

    ptr = node->propData[channelIndex].rxSignalList;

    while (ptr)
    {
        // check if it is in the rxMsgList
        rxMsgInfo = PhyUmtsGetRxMsgInfo(node,
                                         &rxMsgList,
                                         ptr,
                                         TRUE);

        if (rxMsgInfo != NULL)
        {
            // if still in rxMsgList, add it
            rxMsgInfo->next = phyUmts->rxMsgList;
            phyUmts->rxMsgList = rxMsgInfo;
        }
        else
        {
            // check if it is in the interference list
            rxMsgInfo = PhyUmtsGetRxMsgInfo(node,
                                             &ifMsgList,
                                             ptr,
                                             TRUE);
            if (rxMsgInfo != NULL)
            {
                rxMsgInfo->next = phyUmts->ifMsgList;
                phyUmts->ifMsgList = rxMsgInfo;
            }
            else
            {
                // not in interference list, add it
                double rxPower_mW = NON_DB(ANTENNA_GainForThisSignal(
                                               node,
                                               phyIndex,
                                               ptr)
                                               + ptr->rxPower_dBm);

                rxMsgInfo = PhyUmtsCreateRxMsgInfo(
                                node,
                                phyUmts,
                                ptr,
                                rxPower_mW);

                rxMsgInfo->next = phyUmts->ifMsgList;
                phyUmts->ifMsgList = rxMsgInfo;
            }
        } // if (rxMsgInfo == NULL)

        ptr = ptr->next;
    } // while

    // free signals which already expire
    while (rxMsgList != NULL)
    {
        rxMsgInfo = rxMsgList;
        rxMsgList = rxMsgList->next;
        PhyUmtsFreeRxMsgInfo(node, rxMsgInfo);
    }

    // free if signals which already expire
    while (ifMsgList != NULL)
    {
        rxMsgInfo = ifMsgList;
        ifMsgList = ifMsgList->next;
        PhyUmtsFreeRxMsgInfo(node, rxMsgInfo);
    }

    PhyUmtsRxMsgInfo* rxMsgInfoL1 = phyUmts->rxMsgList;
   // int numburst =0;
    while (rxMsgInfoL1 != NULL)
    {
        Message* msgList = rxMsgInfoL1->rxMsg;

        while (msgList != NULL)
        {
            PhyUmtsCdmaBurstInfo* cdmaBurst;

            cdmaBurst = (PhyUmtsCdmaBurstInfo*) MESSAGE_ReturnInfo(msgList);

            if (cdmaBurst != NULL)
            {
                cdmaBurst->interferencePower_mW = 0.0;

                if (DEBUG_BURST)
                {
                    printf("node %d: before calculate interferece "
                         "for studied CDMA burst from nodeId %d\n",
                         node->nodeId, cdmaBurst->syncTimeInfo);
                    PhyUmtsPrintBurstInfo(node, cdmaBurst);
                }

                PhyUmtsRxMsgInfo* rxMsgInfoL2;

                rxMsgInfoL2 = phyUmts->rxMsgList;

                PhyUmtsCalculateTotalInterference(
                        node,
                        rxMsgInfoL2,
                        cdmaBurst,
                        FALSE);

                PhyUmtsRxMsgInfo* rxMsgInfoL3;

                rxMsgInfoL3 = phyUmts->ifMsgList;

                PhyUmtsCalculateTotalInterference(
                        node,
                        rxMsgInfoL3,
                        cdmaBurst,
                        TRUE);

                msgList = msgList->next;
            } // if
        }// msg list in L1
        rxMsgInfoL1 = rxMsgInfoL1->next;
    }// L1
}

//--------------------------------------------------------------------------
//UE part
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: PhyUmtsUeInitDynamicStats
// LAYER      :: PHY
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + phyIndex  : const int     : Interface index where UMTS PHY is running
// + phyUmts   : PhyUmtsData*  : Pointer to PHY UMTS structure
// + phyUeData : PhyUmtsUeData : POinter to PHY UMTS UE data
// RETURN     :: void : NULL
// **/
static
void PhyUmtsUeInitDynamicStats(Node *node,
                               const int phyIndex,
                               PhyUmtsData* phyUmts,
                               PhyUmtsUeData* phyUeData)
{
    if (!node->guiOption)
    {
        return;
    }

    phyUeData->stats.txPowerGuiId =
        GUI_DefineMetric("UMTS UE: Transmission Power (dBm)",
                         node->nodeId,
                         GUI_PHY_LAYER,
                         phyIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_CUMULATIVE_METRIC);
}

// /**
// FUNCTION   :: PhyUmtsUeInit
// LAYER      :: PHY
// PURPOSE    :: Initialize the UMTS PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + phyUmts   : PhyUmtsData*     : Pointer to PhyUmtsData
// + nodeInput : const NodeInput* : Pointer to the node input
// RETURN     :: void             : NULL
// **/
static
void PhyUmtsUeInit(Node *node,
                   const int phyIndex,
                   PhyUmtsData* phyUmts,
                   const NodeInput *nodeInput)
{
    PhyUmtsUeData*  phyDataUe;
    phyDataUe = (PhyUmtsUeData *) MEM_malloc(sizeof(PhyUmtsUeData));
    ERROR_Assert(phyDataUe != NULL, "UMTS: Out of memory!");
    memset(phyDataUe, 0, sizeof(PhyUmtsUeData));

    phyUmts->phyDataUe = phyDataUe;

    phyDataUe->syncTimeInfo = new std::vector<NodeAddress>;
    phyDataUe->dpchSfInUse = UMTS_SF_256; // default use 4

    // up layer will reset the scrambling code afterwards
    phyDataUe->priScCodeInUse = UMTS_INVLAID_UL_SCRAMBLING_CODE_INDEX;
    phyDataUe->secScCodeInUse = UMTS_INVLAID_UL_SCRAMBLING_CODE_INDEX;

    // after power on start PHY  cell search
    phyDataUe->cellSearchNeeded = TRUE;
    memset(&phyDataUe->ueCellSearch, 0 , sizeof(UeCellSearchInfo));

    phyDataUe->ueNodeBInfo = new std::list <PhyUmtsUeNodeBInfo*>;
    //phyDataUe->activeNodeBInfo = new std::list <PhyUmtsUeNodeBInfo*>;
    //phyDataUe->detectedNodeBInfo = new std::list <PhyUmtsUeNodeBInfo*>;

    phyDataUe->phyChList = new std::list<PhyUmtsChannelData*>;

    phyDataUe->targetDlSir = PHY_UMTS_DEFAULT_TARGET_SIR;
    phyDataUe->targetUlSir = PHY_UMTS_DEFAULT_TARGET_SIR;

    // power control
    phyDataUe->pca = UMTS_POWER_CONTROL_ALGO_1;
    phyDataUe->stepSize = PHY_UMTS_DEFAULT_POWER_STEP_SIZE; // dB

    // initialize the dyanmic statistic
    PhyUmtsUeInitDynamicStats(node, phyIndex, phyUmts, phyDataUe);
}

// /**
// FUNCTION   :: PhyUmtsUeFinalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the UE UMTS PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + phyUmts   : PhyUmtsData*  : Pointer to PHY UMTS structure
// + phyUeData : PhyUmtsUeData : POinter to PHY UMTS UE data
// RETURN     :: void             : NULL
// **/
static
void PhyUmtsUeFinalize(Node *node,
                       const int phyIndex,
                       PhyUmtsData* phyUmts,
                       PhyUmtsUeData* phyUeData)
{
    std::list<PhyUmtsUeNodeBInfo*>::iterator itNodeB;
    std::list<PhyUmtsChannelData*>::iterator itCh;

    // delete the UENodeB Info list
    for (itNodeB = phyUeData->ueNodeBInfo->begin();
        itNodeB != phyUeData->ueNodeBInfo->end();
        itNodeB ++)
    {
        delete (*itNodeB);
        *itNodeB = NULL;
    }
    phyUeData->ueNodeBInfo->remove(NULL);
    delete phyUeData->ueNodeBInfo;

    // delete the channel list
    for (itCh = phyUeData->phyChList->begin();
         itCh != phyUeData->phyChList->end();
         itCh ++)
    {
        std::list<Message*>::iterator it;

        for (it = (*itCh)->txMsgList->begin();
            it != (*itCh)->txMsgList->end();
            it ++)
        {
            if (*it)
            {
                MESSAGE_Free(node, (*it));
            }
        }
        delete (*itCh);
        *itCh = NULL;
    }

    phyUeData->phyChList->remove(NULL);
    delete phyUeData->phyChList;

    if (phyUeData->syncTimeInfo != NULL)
    {
        delete phyUeData->syncTimeInfo;
        phyUeData->syncTimeInfo = NULL;
    }

    if (phyUeData->raInfo)
    {
        MEM_free(phyUeData->raInfo);
        phyUeData = NULL;
    }

    // free the UEData
    MEM_free(phyUmts->phyDataUe);
    phyUmts->phyDataUe = NULL;
}

//--------------------------------------------------------------------------
// NODE-B part
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: PhyUmtsNodeBInitDynamicStats
// LAYER      :: PHY
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + phyIndex  : const int     : Interface index where UMTS PHY is running
// + phyUmts   : PhyUmtsData*  : Pointer to PHY UMTS structure
// + phyNodeBData : PhyUmtsNodeBData : POinter to PHY UMTS NodeB data
// RETURN     :: void : NULL
// **/
static
void PhyUmtsNodeBInitDynamicStats(Node *node,
                               const int phyIndex,
                               PhyUmtsData* phyUmts,
                               PhyUmtsNodeBData* phyNodeBData)
{
    if (!node->guiOption)
    {
        return;
    }

    phyNodeBData->stats.txPowerGuiId =
        GUI_DefineMetric("UMTS NodeB: Transmission Power (dBm)",
                         node->nodeId,
                         GUI_PHY_LAYER,
                         phyIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_CUMULATIVE_METRIC);

    phyNodeBData->stats.numHsdpaChInUseGuiId =
        GUI_DefineMetric("UMTS NodeB: Number of Active HSDPA Channels",
                         node->nodeId,
                         GUI_PHY_LAYER,
                         phyIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);

}

// /**
// FUNCTION   :: PhyUmtsNodeBInit
// LAYER      :: PHY
// PURPOSE    :: Initialize the NodeB UMTS PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + phyUmts   : PhyUmtsData*  : Pointer to PHY UMTS structure
// + nodeInput : const NodeInput* : Pointer to the node input
// RETURN     :: void             : NULL
// **/
static
void PhyUmtsNodeBInit(Node *node,
                      const int phyIndex,
                      PhyUmtsData* phyUmts,
                      const NodeInput *nodeInput)
{
    int i = 0;
    int scSetIndex;
    BOOL retVal;

    PhyUmtsNodeBData*  phyDataNodeB;
    phyDataNodeB = (PhyUmtsNodeBData *)MEM_malloc(sizeof(PhyUmtsNodeBData));
    ERROR_Assert(phyDataNodeB != NULL, "UMTS: Out of memory!");
    memset(phyDataNodeB, 0, sizeof(PhyUmtsNodeBData));

    phyUmts->phyDataNodeB = phyDataNodeB;

    // read the NodeB scrambling code set index
    IO_ReadInt(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY-UMTS-NodeB-SCRAMBLING-CODE-SET-INDEX",
        &retVal,
        &scSetIndex);

    if (!retVal )
    {
        scSetIndex = node->nodeId % UMTS_MAX_DL_SCRAMBLING_CODE_SET;
    }
    else if (scSetIndex >= UMTS_MAX_DL_SCRAMBLING_CODE_SET ||
        scSetIndex < 0)
    {
        ERROR_ReportWarning("NodeB's scrambling coe set idnex "
            "is not supported,the value should be in [0,511],"
            "default is used instead\n");

        scSetIndex = node->nodeId % UMTS_MAX_DL_SCRAMBLING_CODE_SET;
    }

    phyDataNodeB->reference_threshold_dB =
            IN_DB(phyUmts->noisePower_mW) + UMTS_THRESHOLD_SHIFT;

    // use the first one in the set as priSC for this nodeb, same as cellId
    phyDataNodeB->pscCodeIndex = scSetIndex * 16;

    phyDataNodeB->sscCodeGroupIndex = scSetIndex / 8;

    for (i = 0;
         i < UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME;
         i++)
    {
        phyDataNodeB->sscCodeSeq[i] =
            UMTS_SSC_TABLE[phyDataNodeB->sscCodeGroupIndex][i];
    }
    phyDataNodeB->phyChList = new std::list<PhyUmtsChannelData*>;

    phyDataNodeB->ueInfo = new std::list<PhyUmtsNodeBUeInfo*>;

    // initialize the dyanmic statistic
    PhyUmtsNodeBInitDynamicStats(node, phyIndex, phyUmts, phyDataNodeB);
}

// /**
// FUNCTION   :: PhyUmtsNodeBFinalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the NODEB UMTS PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + phyUmts   : PhyUmtsData*  : Pointer to PHY UMTS structure
// + phyNodeBData : PhyUmtsNodeBData : POinter to PHY UMTS NodeB data
// RETURN     :: void             : NULL
// **/
static
void PhyUmtsNodeBFinalize(Node *node,
                       const int phyIndex,
                       PhyUmtsData* phyUmts,
                       PhyUmtsNodeBData* phyNodeBData)
{
    std::list<PhyUmtsNodeBUeInfo*>::iterator itUe;
    std::list<PhyUmtsChannelData*>::iterator itCh;

    // delete the UENodeB Info list
    for (itUe = phyNodeBData->ueInfo->begin();
        itUe != phyNodeBData->ueInfo->end();
        itUe ++)
    {
        delete (*itUe);
        *itUe = NULL;
    }

    phyNodeBData->ueInfo->remove(NULL);
    delete phyNodeBData->ueInfo;

    // delete the channel list
    for (itCh = phyNodeBData->phyChList->begin();
         itCh != phyNodeBData->phyChList->end();
         itCh ++)
    {
        std::list<Message*>::iterator it;

        for (it = (*itCh)->txMsgList->begin();
            it != (*itCh)->txMsgList->end();
            it ++)
        {
            if (*it)
            {
                MESSAGE_Free(node, (*it));
            }
        }
        delete (*itCh);
        *itCh = NULL;
    }

    phyNodeBData->phyChList->remove(NULL);
    delete phyNodeBData->phyChList;

    // free the UEData
    MEM_free(phyUmts->phyDataNodeB);
    phyUmts->phyDataNodeB = NULL;
}

//--------------------------------------------------------------------------
//  Key API functions of the UMTS PHY
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: PhyUmtsStartTransmittingSignal
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + packet : Message* : Frame to be transmitted
// + duration : clocktype : Duration of the transmission
// + useMacLayerSpecifiedDelay : BOOL :
//                               Use MAC specified delay or calculate it
// + initDelayUntilAirborne    : clocktype : The MAC specified delay
// RETURN     :: void : NULL
// **/
void PhyUmtsStartTransmittingSignal(
         Node* node,
         int phyIndex,
         Message* packet,
         clocktype duration,
         BOOL useMacLayerSpecifiedDelay,
         clocktype initDelayUntilAirborne)
{
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
    clocktype delayUntilAirborne = initDelayUntilAirborne;
    int channelIndex;
    Message *endMsg;

    if (DEBUG_TX)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf(
            "Time %s: Node%d starts transmitting a signal with power %f\n",
            clockStr, node->nodeId, Float32(phyUmts->txPower_dBm));
    }

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    if (!useMacLayerSpecifiedDelay)
    {
        delayUntilAirborne = phyUmts->rxTxTurnaroundTime;
    }

    PROP_ReleaseSignal(
        node,
        packet,
        phyIndex,
        channelIndex,
        Float32(phyUmts->txPower_dBm),
        duration,
        delayUntilAirborne);

    // This is to track how many transmissions active now
    phyUmts->numTxMsgs ++;

    //GuiStart
    if (node->guiOption == TRUE && phyUmts->numTxMsgs == 1)
    {
        GUI_Broadcast(node->nodeId,
                      GUI_PHY_LAYER,
                      GUI_DEFAULT_DATA_TYPE,
                      node->phyData[phyIndex]->macInterfaceIndex,
                      node->getNodeTime());
    }
    //GuiEnd

    endMsg = MESSAGE_Alloc(node,
                           PHY_LAYER,
                           0,
                           MSG_PHY_TransmissionEnd);

    MESSAGE_SetInstanceId(endMsg, (short) phyIndex);
    MESSAGE_Send(node, endMsg, delayUntilAirborne + duration + 1);

    // Keep track of phy statistics and battery computations
    if (phyCellular->nodeType == CELLULAR_UE)
    {
        phyUmts->phyDataUe->stats.totalTxSignals ++;
        phyUmts->stats.totalTxSignals ++;
        phyUmts->phyDataUe->stats.energyConsumed
            += duration * (BATTERY_TX_POWER_COEFFICIENT
                           * NON_DB(Float32(phyUmts->txPower_dBm))
                           + BATTERY_TX_POWER_OFFSET
                           - BATTERY_RX_POWER);
    }
    else if (phyCellular->nodeType == CELLULAR_NODEB)
    {
        phyUmts->phyDataNodeB->stats.totalTxSignals ++;
        phyUmts->stats.totalTxSignals ++;
        phyUmts->phyDataNodeB->stats.energyConsumed
            += duration * (BATTERY_TX_POWER_COEFFICIENT
                           * NON_DB(Float32(phyUmts->txPower_dBm))
                           + BATTERY_TX_POWER_OFFSET
                           - BATTERY_RX_POWER);
    }

    if (DEBUG_TX)
    {
        printf("node %d: tx on phyIndex  %d ..done... in a slot\n",
               node->nodeId, phyIndex);
    }
}

// /**
// FUNCTION   :: PhyUmtsTransmissionEnd
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyUmtsTransmissionEnd(
         Node* node,
         int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;

    if (DEBUG_TX)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf("Time %s: Node%d transmission end\n",
               clockStr, node->nodeId);
    }

    phyUmts->numTxMsgs --;

    // if all transmissions end, clean the status
    if (phyUmts->numTxMsgs == 0)
    {
        //GuiStart
        if (node->guiOption == TRUE)
        {
            GUI_EndBroadcast(node->nodeId,
                             GUI_PHY_LAYER,
                             GUI_DEFAULT_DATA_TYPE,
                             thisPhy->macInterfaceIndex,
                             node->getNodeTime());
        }
        //GuiEnd
    }
}

// /**
// FUNCTION   :: PhyUmtsSignalArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the channel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyUmtsSignalArrivalFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;

    PhyUmtsRxMsgInfo* rxMsgInfo = NULL;
    double rxPower_mW;

    rxPower_mW =
        NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo)
        + propRxInfo->rxPower_dBm);
    phyUmts->numRxMsgs ++;
    phyUmts->rxLev = IN_DB(rxPower_mW);

    if (DEBUG_RX)
    {
        char clockStr[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), clockStr);
        printf("\n At %s Node%d (PHY_UMTS): receives a signal arrival from "
               "txNode %d channel %d with power %e %f\n",
               clockStr, node->nodeId, propRxInfo->txMsg->originatingNodeId,
               channelIndex, rxPower_mW, phyUmts->rxLev);
    }

    // get rx msg info of the incoming signal
    rxMsgInfo = PhyUmtsCreateRxMsgInfo(node,
                                        phyUmts,
                                        propRxInfo,
                                        rxPower_mW);
    if (UmtsIsNodeB(node))
    {
        if (rxPower_mW >= phyUmts->rxSensitivity_mW[0] / UMTS_SF_256)
        {
            phyUmts->synchronisation = TRUE;
        }
    }
    else
    {
        Message* msg = rxMsgInfo->rxMsg;

        while (msg != NULL)
        {
            if (!phyUmts->synchronisation)
            {
                PhyUmtsCdmaBurstInfo* burstInfo =
                        (PhyUmtsCdmaBurstInfo*) MESSAGE_ReturnInfo(msg);

                if ((burstInfo->phChType == UMTS_PHYSICAL_PSCH ||
                    burstInfo->phChType == UMTS_PHYSICAL_SSCH ) &&
                    (rxPower_mW >= phyUmts->rxSensitivity_mW[0] /
                    UMTS_SF_256))
                {
                    phyUmts->synchronisation = TRUE;
                }
            }

            msg = msg->next;
        }
    }

    // if signal is stronger than sensitivity, check if needs to receive

    if (phyUmts->synchronisation)
    {
        // start receiving it

        if (DEBUG)
        {
            printf("    This is a good signal to be received\n");
        }

        // put into the receiving signal list
        if (phyUmts->rxMsgList == NULL)
        {
            phyUmts->rxMsgList = rxMsgInfo;
        }
        else
        {
            rxMsgInfo->next = phyUmts->rxMsgList;
            phyUmts->rxMsgList = rxMsgInfo;
        }

        phyUmts->stats.totalSignalsLocked++;
    }
    else
    {
        // treat as interference signal

        if (DEBUG)
        {
            printf("    This is an interference signal\n");
        }

        // add into the interference signal list
        if (phyUmts->ifMsgList == NULL)
        {
            phyUmts->ifMsgList = rxMsgInfo;
        }
        else
        {
            rxMsgInfo->next = phyUmts->ifMsgList;
            phyUmts->ifMsgList = rxMsgInfo;
        }

    }

    if (phyUmts->numRxMsgs != 1)
    {
        PhyUmtsCheckRxPacketError(node, phyIndex);
    }

    PhyUmtsSignalInterference(
                node,
                phyIndex,
                channelIndex,
                NULL);
}

// /**
// FUNCTION   :: PhyUmtsSignalEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyUmtsSignalEndFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
    PhyUmtsRxMsgInfo* rxMsgInfo = NULL;

    if (DEBUG || DEBUG_RX)
    {
        char clockStr[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), clockStr);
        printf("PHY_UMTS: Node%d receives a signal end from "
               "channel %d from node %d at time %s\n",
               node->nodeId, channelIndex,
               propRxInfo->txMsg->originatingNodeId, clockStr);
    }

    phyUmts->numRxMsgs --;
    PhyUmtsCheckRxPacketError(node, phyIndex);

    // search if in receiving signal list, if found, remove it too
    rxMsgInfo = PhyUmtsGetRxMsgInfo(node,
                                     &(phyUmts->rxMsgList),
                                     propRxInfo,
                                     TRUE);

    //
    // If the phy is still receiving this signal, forward the frame
    // to the MAC layer if no error.
    //

    if (rxMsgInfo != NULL)
    {

        Message *msg;
        msg = rxMsgInfo->rxMsg;

        if (DEBUG_RX)
        {
            PhyUmtsPrintRxMsgInfo(node, rxMsgInfo);
        }
        rxMsgInfo->rxMsg = NULL;

        MAC_ReceivePacketFromPhy(
                  node,
                  node->phyData[phyIndex]->macInterfaceIndex,
                  msg);

        phyUmts->stats.totalRxSignalsToMac ++;

        // free the rxMsgInfo
        MEM_free(rxMsgInfo);

        rxMsgInfo = NULL;
    }
    else
    {
        // search if in interference signal list, if found, remove it
        rxMsgInfo = PhyUmtsGetRxMsgInfo(node,
                                         &(phyUmts->ifMsgList),
                                         propRxInfo,
                                         TRUE);

        // if in interference list, delete from interference power
        if (rxMsgInfo != NULL)
        {
            if (DEBUG)
            {
                printf("    Signal end of interference signal\n");
            }

            // free the rxMsgInfo
            PhyUmtsFreeRxMsgInfo(node, rxMsgInfo);
            rxMsgInfo = NULL;
        }
        else
        {
            if (DEBUG)
            {
                printf("    Signal not found in list\n");
            }
        }

        phyUmts->stats.totalSignalsWithErrors ++;
    }

    phyUmts->synchronisation = FALSE;

    PhyUmtsSignalInterference(
               node,
               phyIndex,
               channelIndex,
               NULL);
}

// /**
// FUNCTION   :: PhyUmtsInitDefaultParameters
// LAYER      :: PHY
// PURPOSE    :: Initialize non-configurable parameters
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
static
void PhyUmtsInitDefaultParameters(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
    double bwDb;
    double AdjustmentFactorDueNoiseFactor;

    bwDb = 10.0 * log10((double)phyUmts->channelBandwidth);

    AdjustmentFactorDueNoiseFactor =
            IN_DB(thisPhy->noiseFactor)
            - PHY_UMTS_NOISE_FACTOR_REFERENCE
            + PHY_UMTS_IMPLEMENTATION_LOSS;

    phyUmts->numDataRates = PHY_UMTS_NUM_DATA_RATES;

    phyUmts->rxSensitivity_mW[PHY_UMTS_QPSK_R_1_3] =
            NON_DB(PHY_UMTS_DEFAULT_SENSITIVITY_QPSK_R_1_3 + bwDb
            - PHY_UMTS_SENSITIVITY_ADJUSTMENT_FACTOR
            + AdjustmentFactorDueNoiseFactor);

    phyUmts->rxSensitivity_mW[PHY_UMTS_QPSK_R_1_2] =
            NON_DB(PHY_UMTS_DEFAULT_SENSITIVITY_QPSK_R_1_2 + bwDb
            - PHY_UMTS_SENSITIVITY_ADJUSTMENT_FACTOR
            + AdjustmentFactorDueNoiseFactor);

    phyUmts->rxSensitivity_mW[PHY_UMTS_16QAM_R_1_3] =
            NON_DB(PHY_UMTS_DEFAULT_SENSITIVITY_16QAM_R_1_3 + bwDb
            - PHY_UMTS_SENSITIVITY_ADJUSTMENT_FACTOR
            + AdjustmentFactorDueNoiseFactor);

    phyUmts->rxSensitivity_mW[PHY_UMTS_16QAM_R_1_2] =
            NON_DB(PHY_UMTS_DEFAULT_SENSITIVITY_16QAM_R_1_2 + bwDb
            - PHY_UMTS_SENSITIVITY_ADJUSTMENT_FACTOR
            + AdjustmentFactorDueNoiseFactor);

    phyUmts->rxTxTurnaroundTime = PHY_UMTS_RX_TX_TURNAROUND_TIME;

    return;
}

// /**
// FUNCTION   :: PhyUmtsInitConfigurableParameters
// LAYER      :: PHY
// PURPOSE    :: Initialize configurable parameters
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : int              : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to node input
// RETURN     :: void             : NULL
// **/
static
void PhyUmtsInitConfigurableParameters(
         Node* node,
         int phyIndex,
         const NodeInput *nodeInput)
{
    double maxtxPower_dBm;
    double mintxPower_dBm;
    BOOL   wasFound;
    char buf[MAX_STRING_LENGTH];

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;

    //
    // Set PHY_UMTS-TX-MAX-POWER's
    //

    IO_ReadDouble(node,
                  node->nodeId,
                  node->phyData[phyIndex]->macInterfaceIndex,
                  nodeInput,
                  "PHY-UMTS-MAX-TX-POWER",
                  &wasFound,
                  &maxtxPower_dBm);

    if (wasFound)
    {
        phyUmts->maxTxPower_dBm = (float)maxtxPower_dBm;
    }
    else
    {
        phyUmts->maxTxPower_dBm = PHY_UMTS_MAX_TX_POWER_dBm;
    }

    //
    // Set PHY_UMTS-TX-MIN-POWER's
    //
    IO_ReadDouble(node,
                  node->nodeId,
                  node->phyData[phyIndex]->macInterfaceIndex,
                  nodeInput,
                  "PHY-UMTS-MIN-TX-POWER",
                  &wasFound,
                  &mintxPower_dBm);

    if (wasFound)
    {
        phyUmts->minTxPower_dBm = (float)mintxPower_dBm;
    }
    else
    {
        phyUmts->minTxPower_dBm = PHY_UMTS_MIN_TX_POWER_dBm;
    }

    if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        // UE
        phyUmts->txPower_dBm = (float)((Float32(phyUmts->maxTxPower_dBm) +
                               Float32(phyUmts->minTxPower_dBm)) / 2.0) ;
    }
    else
    {
        // nodeB
        phyUmts->txPower_dBm = phyUmts->maxTxPower_dBm;
    }

    // get the HSDPA
    IO_ReadString(node,
                  node->nodeId,
                  node->phyData[phyIndex]->macInterfaceIndex,
                  nodeInput,
                  "PHY-UMTS-HSDPA-CAPABLE",
                  &wasFound,
                  buf);
    if (wasFound  && strcmp(buf, "YES") == 0)
    {
        phyUmts->hspdaEnabled = TRUE;
    }
    else if (!wasFound  || strcmp(buf, "NO") == 0)
    {
        phyUmts->hspdaEnabled = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Specify PHY-UMTS-HSDPA-CAPABLE as YES|NO! Default value"
            "NO is used.");
        phyUmts->hspdaEnabled = FALSE;
    }

    phyUmts->channelBandwidth = PHY_UMTS_CHANNEL_BANDWIDTH;
    phyUmts->synchronisation = FALSE;

    // TODO : decide Rate Type and duplex mode
    phyUmts->duplexMode = UMTS_DUPLEX_MODE_FDD;
    phyUmts->chipRateType = UMTS_CHIP_RATE_384;

    if (phyUmts->chipRateType == UMTS_CHIP_RATE_384)
    {
        phyUmts->chipRate = UMTS_CHIP_RTAE_384;
    }
    else
    {
        //  Not done yet
    }
    phyUmts->noisePower_mW =
        thisPhy->noise_mW_hz * phyUmts->channelBandwidth;

}

// /**
// FUNCTION   :: PhyUmtsInit
// LAYER      :: PHY
// PURPOSE    :: Initialize the UMTS PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to the node input
// RETURN     :: void             : NULL
// **/
void PhyUmtsInit(Node *node,
                 const int phyIndex,
                 const NodeInput *nodeInput)
{
    PhyData* thisPhy;
    PhyUmtsData* phyUmts;

    thisPhy = (PhyData*) node->phyData[phyIndex];
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;

    phyUmts = (PhyUmtsData *) MEM_malloc(sizeof(PhyUmtsData));

    ERROR_Assert(phyUmts != NULL, "PHY_UMTS: Out of memory!");
    memset(phyUmts, 0, sizeof(PhyUmtsData));

    phyCellular->phyUmtsData = phyUmts;

     // Initialize the antenna model
    ANTENNA_Init(node, phyIndex, nodeInput);
/*    ERROR_Assert((thisPhy->antennaData->antennaModelType ==
                 ANTENNA_OMNIDIRECTIONAL),
                 "PHY_UMTS: Only omnidirectional antenna is supported!\n"); */

    // Read basic parameters
    // initialize parameters
    PhyUmtsInitConfigurableParameters(node, phyIndex, nodeInput);
    PhyUmtsInitDefaultParameters(node, phyIndex);


    phyCellular->nodeType = UmtsGetNodeType(node);// read the station type

    if (!(phyCellular->nodeType == CELLULAR_UE ||
        phyCellular->nodeType == CELLULAR_NODEB))
    {
        ERROR_ReportWarning(
        "UMTS: Wrong value of CELLULAR-NODE-TYPE,"
        "shoudle be UE or NODEB  at L1. Use default value as UE.");

        // by default, a node is subscriber station
        phyCellular->nodeType = CELLULAR_UE;

    }

    if (phyCellular->nodeType == CELLULAR_UE)
    {
        // init UE Phy
        PhyUmtsUeInit(node, phyIndex, phyUmts, nodeInput);
    }
    else if (phyCellular->nodeType == CELLULAR_NODEB)
    {
        // inti nodeB phy
        PhyUmtsNodeBInit(node, phyIndex, phyUmts, nodeInput);
    }

    // set the default transmission channel, MAC may reset
    int numChannels = PROP_NumberChannels(node);
    int i;

    for (i = 0; i < numChannels; i++)
    {
        if (thisPhy->channelListening[i] == TRUE)
        {
            break;
        }
    }

    if (i == numChannels)
    {
        i = 0;
    }

    PHY_SetTransmissionChannel(node, phyIndex, i);
}

// /**
// FUNCTION   :: PhyUmtsFinalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the UMTS PHY, print out statistics
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : const int : Index of the PHY
// RETURN     :: void      : NULL
// **/

void PhyUmtsFinalize(Node *node, const int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
    PhyUmtsUeData* phyUeData = NULL;
    PhyUmtsNodeBData* phyNodeBData = NULL;
    char buf[MAX_STRING_LENGTH];

    if (thisPhy->phyStats && phyCellular->collectStatistics)
    {
        sprintf(buf, "Signals transmitted = %d",
                phyUmts->stats.totalTxSignals);
        IO_PrintStat(node, "Physical", "UMTS", ANY_DEST, phyIndex, buf);

        sprintf(buf, "Signals received and forwarded to MAC = %d",
                phyUmts->stats.totalRxSignalsToMac);
        IO_PrintStat(node, "Physical", "UMTS", ANY_DEST, phyIndex, buf);

        sprintf(buf, "Signals locked on by PHY = %d",
                phyUmts->stats.totalSignalsLocked);
        IO_PrintStat(node, "Physical", "UMTS", ANY_DEST, phyIndex, buf);

        sprintf(buf, "Signals received but with errors = %d",
                phyUmts->stats.totalSignalsWithErrors);
        IO_PrintStat(node, "Physical", "UMTS", ANY_DEST, phyIndex, buf);

        sprintf(buf, "CDMA bursts received and forwarded to MAC = %d",
                phyUmts->stats.totalRxBurstsToMac);
        IO_PrintStat(node, "Physical", "UMTS", ANY_DEST, phyIndex, buf);

        sprintf(buf, "CDMA bursts received but with errors = %d",
                phyUmts->stats.totalRxBurstsWithErrors);
        IO_PrintStat(node, "Physical", "UMTS", ANY_DEST, phyIndex, buf);

        phyUmts->stats.energyConsumed
            += (double) BATTERY_RX_POWER
               * (node->getNodeTime() - phyUmts->stats.turnOnTime);

        sprintf(buf, "Energy consumption (in mWhr) = %.3f",
               phyUmts->stats.energyConsumed / 3600.0);
        IO_PrintStat(node, "Physical", "UMTS", ANY_DEST, phyIndex, buf);
    }

    // free the memory
    if (phyCellular->nodeType == CELLULAR_UE)
    {
        // finalize the UE
        phyUeData = phyUmts->phyDataUe;

        PhyUmtsUeFinalize(node, phyIndex, phyUmts, phyUeData);
    }
    else if (phyCellular->nodeType == CELLULAR_NODEB)
    {
        // finalize the NodeB
        phyNodeBData = phyUmts->phyDataNodeB;
        PhyUmtsNodeBFinalize(node, phyIndex, phyUmts, phyNodeBData);
    }

    // free the msg in RxMsgList or IfMsgList

    PhyUmtsRxMsgInfo* rxMsgList = NULL;
    PhyUmtsRxMsgInfo* ifMsgList = NULL;
    PhyUmtsRxMsgInfo* rxMsgInfo = NULL;

    rxMsgList = phyUmts->rxMsgList;
    ifMsgList = phyUmts->ifMsgList;

    while (rxMsgList != NULL)
    {
        rxMsgInfo = rxMsgList;
        rxMsgList = rxMsgList->next;
        PhyUmtsFreeRxMsgInfo(node, rxMsgInfo);
    }

    while (ifMsgList != NULL)
    {
        rxMsgInfo = ifMsgList;
        ifMsgList = ifMsgList->next;
        PhyUmtsFreeRxMsgInfo(node, rxMsgInfo);
    }

    MEM_free(phyUmts);
    phyCellular->phyUmtsData = NULL;
}

//  /**
// FUNCITON   :: PhyUmtsHandleInterLayerCommand
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer command
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + cmdType          : UmtsInterlayerCommandType : command type
// + cmd              : Message*          : cmd to handle
// RETURN     :: void : NULL
void PhyUmtsHandleInterLayerCommand(
            Node* node,
            UmtsInterlayerCommandType cmdType,
            Message* cmd)
{
    // start here
}


// /**
// FUNCTION   :: PhyUmtsSetTransmitPower
// LAYER      :: PHY
// PURPOSE    :: Set the transmit power of the PHY
// PARAMETERS ::
// + node       : Node*  : Pointer to node.
// + phyIndex   : int    : Index of the PHY
// + txPower_mW : double : Transmit power in mW unit
// RETURN     ::  void   : NULL
// **/
void PhyUmtsSetTransmitPower(Node *node, int phyIndex, double txPower_mW)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;

    if (txPower_mW >= NON_DB(Float32(phyUmts->maxTxPower_dBm)))
    {
        phyUmts->txPower_dBm = (float) phyUmts->maxTxPower_dBm;
    }
    else if ((txPower_mW < NON_DB(Float32(phyUmts->maxTxPower_dBm)))&&
             (txPower_mW >= NON_DB(Float32(phyUmts->minTxPower_dBm))))
    {
        phyUmts->txPower_dBm = (float)IN_DB(txPower_mW);
    }
    else
    {
       phyUmts->txPower_dBm =  (float) phyUmts->minTxPower_dBm;
    }

    // update GUI
    if (node->guiOption)
    {
        if (phyCellular->nodeType == CELLULAR_UE)
        {
            PhyUmtsUeData* phyUeData = NULL;
            phyUeData = phyUmts->phyDataUe;
            GUI_SendRealData(node->nodeId,
                             phyUeData->stats.txPowerGuiId,
                             Float32(phyUmts->txPower_dBm),
                             node->getNodeTime());
        }
        else if (phyCellular->nodeType == CELLULAR_NODEB)
        {
            PhyUmtsNodeBData* phyNodeBData = NULL;
            phyNodeBData = phyUmts->phyDataNodeB;
            GUI_SendRealData(node->nodeId,
                             phyNodeBData->stats.txPowerGuiId,
                             Float32(phyUmts->txPower_dBm),
                             node->getNodeTime());
        }
        else
        {
            // do nothing
        }
    }
}

// /**
// FUNCTION   :: PhyUmtsGetTransmitPower
// LAYER      :: PHY
// PURPOSE    :: Retrieve the transmit power of the PHY in mW unit
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex   : int     : Index of the PHY
// + txPower_mW : double* : For returning the transmit power
// RETURN     ::  void    : NULL
// **/
void PhyUmtsGetTransmitPower(Node *node, int phyIndex, double *txPower_mW)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;

    *txPower_mW = NON_DB(Float32(phyUmts->txPower_dBm));
}


// /**
// FUNCTION   :: PhyUmtsInterferenceArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle interference signal arrival from the channel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// + sigPower_mw  : double      : The inband interference signalmpower in mW
// RETURN     :: void : NULL
// **/
void PhyUmtsInterferenceArrivalFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo,
         double sigPower_mW)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;

    PhyUmtsRxMsgInfo* rxMsgInfo = NULL;
    double interferencePower_mW;

    PhyUmtsCheckRxPacketError(node, phyIndex);

    interferencePower_mW =
        NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo)
        + IN_DB(sigPower_mW));

    phyUmts->numRxMsgs ++;

    // get rx msg info of the interference signal
    rxMsgInfo = PhyUmtsCreateRxMsgInfo(node,
                                phyUmts,
                                propRxInfo,
                                interferencePower_mW);

    // add into the interference signal list
    if (phyUmts->ifMsgList == NULL)
    {
        phyUmts->ifMsgList = rxMsgInfo;
    }
    else
    {
        rxMsgInfo->next = phyUmts->ifMsgList;
        phyUmts->ifMsgList = rxMsgInfo;
    }

    if (phyUmts->numRxMsgs != 1)
    {
        PhyUmtsCheckRxPacketError(node, phyIndex);
    }

    PhyUmtsSignalInterference(
                node,
                phyIndex,
                channelIndex,
                NULL);
}

// /**
// FUNCTION   :: PhyUmtsInterferenceEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle interference signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyUmtsInterferenceEndFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyUmtsData* phyUmts;
    PhyCellularData *phyCellular =
        (PhyCellularData*)thisPhy->phyVar;
    phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
    PhyUmtsRxMsgInfo* rxMsgInfo = NULL;

    phyUmts->numRxMsgs --;
    PhyUmtsCheckRxPacketError(node, phyIndex);

    // search in interference signal list
    rxMsgInfo = PhyUmtsGetRxMsgInfo(node,
                             &(phyUmts->ifMsgList),
                             propRxInfo,
                             TRUE);

    // delete from interference power
    if (rxMsgInfo != NULL)
    {
        // free the rxMsgInfo
        PhyUmtsFreeRxMsgInfo(node, rxMsgInfo);
        rxMsgInfo = NULL;
    }

    phyUmts->synchronisation = FALSE;

    PhyUmtsSignalInterference(
               node,
               phyIndex,
               channelIndex,
               NULL);
}
