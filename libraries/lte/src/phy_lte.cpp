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

#define __PHY_LTE_CPP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <vector>

#include "api.h"
#include "antenna.h"
#include "main.h"
#include "clock.h"
#include "random.h"
#include "partition.h"
#include "propagation.h"
#include "message.h"
#include "phy.h"

#include "phy_lte.h"
#include "phy_lte_establishment.h"
#include "lte_common.h"
#include "layer3_lte_filtering.h"
#include "layer2_lte_rlc.h"

#ifdef ADDON_DB
#include "stats_phy.h"
#include "dbapi.h"
#endif

#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif

#define DEBUG        0

#ifdef LTE_LIB_LOG

// /**
// FUNCTION   :: STR_COMPLEX
// LAYER      :: PHY
// PURPOSE    :: Convert complex value to string.
// PARAMETERS ::
// + buf      : char*                : Pointer to buffer
// + c        : std::complex<double> : Complex value to convert
// RETURN     :: const char* : Pointer to the buffer
// **/
const char* STR_COMPLEX(char* buf, std::complex < double > c)
{
    sprintf(buf,"%.10e%c%.10ei",
        c.real(),
        (c.imag() >= 0.0 ? '+' : '-'),
        ABS(c.imag()));

    return buf;
}

// /**
// FUNCTION   :: PhyLteDebugOutputChannelResponseLog
// LAYER      :: PHY
// PURPOSE    :: Output debug log of channel response matrix
// PARAMETERS ::
// + node         : Node*         : Pointer to buffer
// + phyIndex     : int           : CIndex of the PHY
// + channelIndex : int           : Channel index
// + lteTxInfo    : PhyLteTxInfo* : LTE transmission info
// + propTxInfo   : PropTxInfo*   : Propagation tx info
// RETURN     :: void : NULL
// **/
void PhyLteDebugOutputChannelResponseLog(Node* node,
                                   int phyIndex,
                                   int channelIndex,
                                   PhyLteTxInfo* lteTxInfo,
                                   PropTxInfo* propTxInfo)
{
    char buf[MAX_STRING_LENGTH];

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    LteRnti txRnti(propTxInfo->txNodeId, propTxInfo->phyIndex);

    int matrixSize = phyLte->numRxAntennas * lteTxInfo->numTxAnntena;
    Cmat matH(LTE_DEFAULT_DCOMP,matrixSize);

    PhyLteGetChannelMatrix(
                    node, phyIndex, phyLte, propTxInfo, lteTxInfo, matH);

    stringstream log;

    log << "Rnti=," << STR_RNTI(buf, txRnti) << ","
        << "Size=," << (int)phyLte->numRxAntennas << ","
        << (int)lteTxInfo->numTxAnntena << ","
        << "ChRsp=,";

    for (int i = 0; i < phyLte->numRxAntennas; ++i)
    {
        for (int j = 0; j < lteTxInfo->numTxAnntena; ++j)
        {
            Dcomp Hij = matH[ MATRIX_INDEX(i,j,lteTxInfo->numTxAnntena)];
            log << STR_COMPLEX(buf,Hij) << ",";
        }
    }

    lte::LteLog::DebugFormat(
        node,
        node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PROP,
        "%s,%s",
        LTE_STRING_CATEGORY_TYPE_CHRSP,
        log.str().c_str());

}

// /**
// FUNCTION   :: PhyLteDebugOutputChannelResponseLog
// LAYER      :: PHY
// PURPOSE    :: Output debug log of channel response matrix
// PARAMETERS ::
// + node         : Node*         : Pointer to buffer
// + phyIndex     : int           : Index of the PHY
// + channelIndex : int           : Channel index
// + lteTxInfo    : PhyLteTxInfo* : LTE transmission info
// + propTxInfo   : PropTxInfo*   : Propagation tx info
// + propRxInfo   : PropRxInfo*   : Propagation rx info
// RETURN     :: void : NULL
// **/
void PhyLteDebugOutputChannelResponseLog(Node* node,
                                   int phyIndex,
                                   int channelIndex,
                                   PhyLteTxInfo* lteTxInfo,
                                   PropTxInfo* propTxInfo,
                                   PropRxInfo* propRxInfo)
{
    char buf[MAX_STRING_LENGTH];

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    // 1. Output channel matrix between "node" and the node
    //    from which receiving a message now.
    std::set < LteRnti > outputRnti;
    std::list < PhyLteRxMsgInfo* > ::iterator it_rxMsg;
    for (it_rxMsg = phyLte->rxPackMsgList->begin();
        it_rxMsg != phyLte->rxPackMsgList->end();
        it_rxMsg++)
    {
       LteRnti rxMsgTxRnti(
           (*it_rxMsg)->propTxInfo.txNodeId,
           (*it_rxMsg)->propTxInfo.phyIndex);

       ERROR_Assert(outputRnti.find(rxMsgTxRnti) == outputRnti.end(),
           "Multiple message from the same user are exist.");

       PhyLteDebugOutputChannelResponseLog(node, phyIndex, channelIndex,
           &(*it_rxMsg)->lteTxInfo,
           &(*it_rxMsg)->propTxInfo);

       outputRnti.insert(rxMsgTxRnti);

    }

    // 2. Output channel matrix between "node" and the node
    //    from which receiving a message this time.
    LteRnti txRnti(propTxInfo->txNodeId, propTxInfo->phyIndex);

    if (outputRnti.find(txRnti) != outputRnti.end())
    {
        // Exit if channel matrix for txRnti is already output.
        return;
    }

    PhyLteDebugOutputChannelResponseLog(node, phyIndex, channelIndex,
        lteTxInfo,
        propTxInfo);
}

// /**
// FUNCTION   :: PhyLteDebugOutputIfPowerLog
// LAYER      :: PHY
// PURPOSE    :: Output debug log of interference power
// PARAMETERS ::
// + node         : Node*         : Pointer to buffer
// + phyIndex     : int           : Index of the PHY
// RETURN     :: void : NULL
// **/
void PhyLteDebugOutputIfPowerLog(Node* node,
                                 int phyIndex)
{
    char buf[MAX_STRING_LENGTH];

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    stringstream log;

    for (int i = 0; i < phyLte->numResourceBlocks; i++)
    {
        log << std::scientific <<  phyLte->interferencePower_mW[i] << ",";
    }

    lte::LteLog::DebugFormat(
        node,
        node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PROP,
        "%s,%s",
        LTE_STRING_CATEGORY_TYPE_IFPOWER,
        log.str().c_str());
}

// /**
// FUNCTION   :: PhyLteGetAggregator
// LAYER      :: PHY
// PURPOSE    :: Get aggregator instance
// PARAMETERS ::
// + node         : Node*         : Pointer to buffer
// + phyIndex     : int           : Index of the PHY
// RETURN     :: lte::Aggregator* : Pointer to the aggregator
// **/
lte::Aggregator* PhyLteGetAggregator(Node* node,
                                     int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    return phyLte->aggregator;
}



#endif // LTE_LIB_LOG

#ifdef LTE_LIB_LOG




struct InfoProfile
{
    int info;
    const char* name;
    const char* symbol;

    const char* (*outFunc)
                (Message* msg, const InfoProfile* infoProfile, char* buf);
};

const char* PhyLteDebugOutput_INFO_TYPE_Default(
    Message* msg, const InfoProfile* infoProfile, char* buf)
{
    void* info =
        MESSAGE_ReturnInfo(msg, (unsigned short)infoProfile->info);

    if (info)
    {
        sprintf(buf, "%s",
            infoProfile->symbol);
    }else
    {
        buf[0] = 0;
    }

    return buf;
}


const char* PhyLteDebugOutput_INFO_TYPE_LteMacDestinationInfo(
    Message* msg, const InfoProfile* infoProfile, char* buf)
{
    MacLteMsgDestinationInfo* dstInfo =
        (MacLteMsgDestinationInfo*)MESSAGE_ReturnInfo(msg,
        (unsigned short)INFO_TYPE_LteMacDestinationInfo);

    if (dstInfo)
    {
        sprintf(buf, "%s:{[%d,%d]}",
            infoProfile->symbol,
            dstInfo->dstRnti.nodeId,
            dstInfo->dstRnti.interfaceIndex);
    }else
    {
        buf[0] = 0;
    }

    return buf;
}

// Symbols for INFOs
static const int numInfoToCheck = 25;
static const InfoProfile infoToCheck[numInfoToCheck] =
{
    {
        INFO_TYPE_LtePhyDlTtiInfo,
        "INFO_TYPE_LtePhyDlTtiInfo",
        "DlTti",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyTxInfo,
        "INFO_TYPE_LtePhyTxInfo",
        "PhyTx",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyCqiInfo,
        "INFO_TYPE_LtePhyCqiInfo",
        "Cqi",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyRiInfo,
        "INFO_TYPE_LtePhyRiInfo",
        "Ri",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhySrsInfo,
        "INFO_TYPE_LtePhySrsInfo",
        "Srs",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyPss,
        "INFO_TYPE_LtePhyPss",
        "Pss",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyRandamAccessGrant,
        "INFO_TYPE_LtePhyRandamAccessGrant",
        "RAG",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyRrcConnectionSetupComplete,
        "INFO_TYPE_LtePhyRrcConnectionSetupComplete",
        "SetupComp",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyRrcConnectionReconfComplete,
        "INFO_TYPE_LtePhyRrcConnectionReconfComplete",
        "ReconfComp",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyRandamAccessTransmitPreamble,
        "INFO_TYPE_LtePhyRandamAccessTransmitPreamble",
        "RATP",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyRandamAccessTransmitPreambleTimerDelay,
        "INFO_TYPE_LtePhyRandamAccessTransmitPreambleTimerDelay",
        "RATPTD",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyRandamAccessTransmitPreambleInfo,
        "INFO_TYPE_LtePhyRandamAccessTransmitPreambleInfo",
        "RATPI",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LtePhyLtePhyToMacInfo,
        "INFO_TYPE_LtePhyLtePhyToMacInfo",
        "P2M",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LteDci0Info,
        "INFO_TYPE_LteDci0Info",
        "Dci0",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LteDciForUl,
        "INFO_TYPE_LteDciForUl",
        "DciForUl",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LteDci1Info,
        "INFO_TYPE_LteDci1Info",
        "Dci1",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LteDci2aInfo,
        "INFO_TYPE_LteDci2aInfo",
        "Dci2a",
        PhyLteDebugOutput_INFO_TYPE_Default},
    {
        INFO_TYPE_LteMacDestinationInfo,
        "INFO_TYPE_LteMacDestinationInfo",
        "Dest",
        PhyLteDebugOutput_INFO_TYPE_LteMacDestinationInfo
    },
    {
        INFO_TYPE_LteMacNoTransportBlock,
        "INFO_TYPE_LteMacNoTransportBlock",
        "NoTB",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LteMacPeriodicBufferStatusReport,
        "INFO_TYPE_LteMacPeriodicBufferStatusReport",
        "PedBSR",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LteMacRRELCIDFLWith7bitSubHeader,
        "INFO_TYPE_LteMacRRELCIDFLWith7bitSubHeader",
        "7bitSubHd",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LteMacRRELCIDFLWith15bitSubHeader,
        "INFO_TYPE_LteMacRRELCIDFLWith15bitSubHeader",
        "15bitSubHd",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LteMacRRELCIDSubHeader,
        "INFO_TYPE_LteMacRRELCIDSubHeader",
        "CIDSubHd",
        PhyLteDebugOutput_INFO_TYPE_Default
    },
    {
        INFO_TYPE_LteMacTxInfo,
        "INFO_TYPE_LteMacTxInfo",
        "MacTx",
        PhyLteDebugOutput_INFO_TYPE_Default
    }
};

// /**
// FUNCTION   :: PhyLteDebugOutputMessage
// LAYER      :: PHY
// PURPOSE    :: Print information of Message
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + msg       : Message* : Pointer to message
// + prefix    : const char* : Prefix string
// + propRxInfo : PropRxInfo* :
//                 Pointer to propagation rx info.
// RETURN     :: void : NULL
// **/
void PhyLteDebugOutputMessage(Node* node,
                              int phyIndex,
                              Message* msg,
                              const char* prefix,
                              PropRxInfo* propRxInfo = NULL)
{
    std::stringstream ssRxMsgInfo;

    std::stringstream ssRfMsg;
    char buf2[1024];

    for (int i = 0; i < numInfoToCheck; ++i)
    {
        (*infoToCheck[i].outFunc)(msg, &infoToCheck[i], buf2);

        if (buf2[0] != 0)
        {
            ssRfMsg << buf2 << ",";
        }
    }

    ssRxMsgInfo << "[" << ssRfMsg.str().c_str() << "]";


    printf("%llu,Node#%003d,FromNode#%003d,%s,DumpMsg:%s\n",
            node->getNodeTime(),
            node->nodeId,
            (propRxInfo ? propRxInfo->txNodeId : node->nodeId),
            prefix,ssRxMsgInfo.str().c_str());
}

// /**
// FUNCTION   :: PhyLteDebugOutputRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Print information of received message.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + rxMsgInfo : PhyLteRxMsgInfo* :
//                 Pointer to received message info.
// RETURN     :: void : NULL
// **/
void PhyLteDebugOutputRxMsgInfo(Node* node,
                              int phyIndex,
                              PhyLteRxMsgInfo* rxMsgInfo)
{
    Message* rfMsg = rxMsgInfo->rxMsg;

    std::stringstream ssRxMsgInfo;

    while (rfMsg)
    {
        std::stringstream ssRfMsg;
        char buf2[1024];

        for (int i = 0; i < numInfoToCheck; ++i)
        {
            (*infoToCheck[i].outFunc)(rfMsg, &infoToCheck[i], buf2);

            if (buf2[0] != 0)
            {
                ssRfMsg << buf2 << ",";
            }
        }
        rfMsg = rfMsg->next;

        ssRxMsgInfo << "[" << ssRfMsg.str().c_str() << "],";
    }

    printf("%llu,Node#%003d,DumpRxMsgInfo:%s\n",
        node->getNodeTime(),
        node->nodeId,
        ssRxMsgInfo.str().c_str());
}

// /**
// FUNCTION   :: PhyLteDebugOutputRxMsgInfoList
// LAYER      :: PHY
// PURPOSE    :: Print information of received messages.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + msgList   : std::list<PhyLteRxMsgInfo *>* :
//                 Pointer to unpackaged received message info list.
// RETURN     :: void : NULL
// **/
void PhyLteDebugOutputRxMsgInfoList(Node* node,
                              int phyIndex,
                              std::list < PhyLteRxMsgInfo* >* msgList)
{
    std::stringstream ss;

    std::list < PhyLteRxMsgInfo* > ::iterator it;
    for (it = msgList->begin(); it != msgList->end(); ++it)
    {
        PhyLteRxMsgInfo* rxMsgInfo = *it;
        Message* rfMsg = rxMsgInfo->rxMsg;

        PhyLteDebugOutputRxMsgInfo(
            node, phyIndex, rxMsgInfo);
    }
}

#endif

// /**
// FUNCTION   :: PhyLteCreateRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Create a unpacked received message structure.
// PARAMETERS ::
// + node          : Node*  : Pointer to node.
// + phyIndex      : int    : Index of the PHY.
// + phyLte        : PhyDataLte* : Pointer to the LTE PHY structure.
// + propRxInfo    : PropRxInfo* : Information of the arrived signal.
// + rxpathloss_mW : double : Path loss.
// + txPower_mW    : double : Transmit power.
// RETURN     :: PhyLteRxMsgInfo* : Pointer to a structure created.
// **/
PhyLteRxMsgInfo* PhyLteCreateRxMsgInfo(Node* node,
                                               int phyIndex,
                                               PhyDataLte* phyLte,
                                               PropRxInfo* propRxInfo,
                                               double rxpathloss_mW,
                                               double txPower_mW)
{
    Message* msgList = NULL;

    if ((!PhyLteIsMessageNoTransportBlock(node, phyIndex, propRxInfo->txMsg))
        && ((MESSAGE_ReturnPacketSize(propRxInfo->txMsg) > 0)
            || (MESSAGE_ReturnVirtualPacketSize(propRxInfo->txMsg) > 0)))
    {
        msgList = MESSAGE_UnpackMessage(
                                node, propRxInfo->txMsg, FALSE, FALSE);
    }

    PhyLteRxMsgInfo* rxMsgInfo =
        (PhyLteRxMsgInfo*) MEM_malloc(sizeof(PhyLteRxMsgInfo));

    ERROR_Assert(rxMsgInfo != NULL, "PHYLTE: Out of memory!");
    memset(rxMsgInfo, 0, sizeof(PhyLteRxMsgInfo));

    if (msgList != NULL)
    {
        rxMsgInfo->propRxMsgAddress = propRxInfo->txMsg;
        rxMsgInfo->rxMsg = msgList;
    }
    else
    {
        rxMsgInfo->propRxMsgAddress = propRxInfo->txMsg;
        rxMsgInfo->rxMsg = NULL;
    }

    return rxMsgInfo;
}

// /**
// FUNCTION   :: PhyLteGetRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Search for a structure that matches
//               the unpackaged received message structure.
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + msgList : std::list<PhyLteRxMsgInfo *>*
//               : Pointer to unpackaged received message info list.
// + rxMsg     : Message*           : Message of the signal as a key
// + removeIt  : BOOL               : Indicate whether remove from the list
// RETURN     :: PhyLteRxMsgInfo* : Pointer to a structure that matches.
// **/
static
PhyLteRxMsgInfo* PhyLteGetRxMsgInfo(Node* node,
                                    std::list < PhyLteRxMsgInfo* >* msgList,
                                    Message* rxMsg,
                                    BOOL removeIt)
{
    std::list < PhyLteRxMsgInfo* > ::iterator it;
    for (it = msgList->begin(); it != msgList->end(); it++)
    {
        if ((*it)->propRxMsgAddress == rxMsg)
        {
            return &(*it)[0];
        }
    }
    return NULL;
}

// /**
// FUNCTION   :: PhyLteFreeRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Free the unpacked received message structure.
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + msg       : PhyLteRxMsgInfo* : PhyLteRxMsgInfo * :
//             : Pointer to unpackaged received message info.
// + msgList : std::list<PhyLteRxMsgInfo *>* :
//               : Free the unpackaged received message structure list.
// RETURN     :: BOOL : Success or failure
// **/
BOOL PhyLteFreeRxMsgInfo(Node* node,
                         int phyIndex,
                         PhyLteRxMsgInfo* rxMsgInfo,
                         std::list < PhyLteRxMsgInfo* >* msgList)
{
    if ((rxMsgInfo != NULL) && (msgList->empty() == FALSE))
    {
        // Search indicated rx message
        std::list < PhyLteRxMsgInfo* > ::iterator it;

#ifdef LTE_LIB_LOG
        int cntSignalMatch = 0;

        // For memory leak checking
        for (it = msgList->begin(); it != msgList->end(); it++)
        {
            if ((*it)->propRxMsgAddress == rxMsgInfo->propRxMsgAddress)
            {
                ++cntSignalMatch;
            }
        }

        ERROR_Assert(cntSignalMatch <= 1,
            "Multiple signals from the same node exist at the same time");
#endif


        for (it = msgList->begin(); it != msgList->end(); it++)
        {
            if ((*it)->propRxMsgAddress == rxMsgInfo->propRxMsgAddress)
            {
                ERROR_Assert(*it == rxMsgInfo, "Invalid message coming.");

#ifdef LTE_LIB_LOG
                lte::LteLog::Debug2Format(
                    node,
                    node->phyData[phyIndex]->macInterfaceIndex,
                    "PHY:DEBUG:PhyLteFreeRxMsgInfo:freeMsg:",
                    "%p,%p,%p,%p",
                    rxMsgInfo->rxMsg,
                    rxMsgInfo->propRxMsgAddress,
                    (*it)->rxMsg,
                    (*it)->propRxMsgAddress);
#endif // LTE_LIB_LOG

                MESSAGE_FreeList(node, (*it)->rxMsg);
                msgList->erase(it);

                // Free PhyLteRxMsgInfo (= *it)
                MEM_free(rxMsgInfo);

                return TRUE;
            }
        }

    }

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
                              node->phyData[phyIndex]->macInterfaceIndex,
                              "PHY:DEBUG:PhyLteFreeRxMsgInfo:",
                              "%p",
                              msgList);
#endif // LTE_LIB_LOG

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteCalculateCsi
// LAYER      :: PHY
// PURPOSE    :: Calculate channel state indicator (CQI and RI)
// PARAMETERS ::
// + node         : Node*         : Pointer to node.
// + phyIndex     : int           : Index of the PHY.
// + phyLte       : PhyDataLte*   : Pointer to LTE PHY structure
// + propTxInfo   : PropTxInfo*   : Pointer to propagation tx info
// + lteTxInfo    : PhyLteTxInfo* : Pointer to LTE transmission info
// + pathloss_dB  : double        : Pathloss of received signal in dB
// RETURN     :: void : NULL
// **/
void PhyLteCalculateCsi(Node* node,
                     int phyIndex,
                     PhyDataLte* phyLte,
                     PropTxInfo* propTxInfo, // Tx info of desired signal
                     PhyLteTxInfo* lteTxInfo, // Tx info of desired signal
                     double pathloss_dB)
{
    ERROR_Assert(phyLte->stationType == LTE_STATION_TYPE_UE,
                "No need to calculate SCI on eNB");

    LteRnti txRnti(propTxInfo->txNodeId, propTxInfo->phyIndex); // for log

    // To minimize the calculation overhead, CQI or RI is calculated
    // at the moment of feedback
    UInt64 ttiCount = MacLteGetTtiNumber(node, phyIndex);
    ++ttiCount;

    int cqiTiming = (ttiCount + phyLte->cqiReportingOffset) %
        phyLte->cqiReportingInterval;
    int riTiming = (ttiCount + phyLte->riReportingOffset) %
        phyLte->riReportingInterval;

    // 0. Judge if RRC config is already received
    //-------------------------------------------

    BOOL rrcConfigReceived = PhyLteIsInStationaryState(node, phyIndex);

    if (rrcConfigReceived == FALSE)
    {
        return;
    }

    // 1. Rank indicator and CQI calculation
    //----------------------------------------

    if (riTiming == 0)
    {
        phyLte->nextReportedRiInfo =
                PhyLteCalculateRankIndicator(
                        node, phyIndex,
                        phyLte, propTxInfo, lteTxInfo, pathloss_dB);

        ERROR_Assert(phyLte->nextReportedRiInfo >= 1 &&
                        phyLte->nextReportedRiInfo <= 2,
                    "Invalid RI detected");

    }else if (cqiTiming == 0)
    {
        if (phyLte->nextReportedRiInfo != PHY_LTE_INVALID_RI)
        {
            phyLte->nextReportedCqiInfo =
                PhyLteCalculateCqi(
                        node, phyIndex,
                        phyLte, propTxInfo, lteTxInfo,
                        phyLte->nextReportedRiInfo,
                        pathloss_dB,
                        true);
        }else
        {
            // Can't calculate CQI since RI is not decided yet.
        }
    }else
    {
        // Do nothing
    }
}

// /**
// FUNCTION   :: PhyLteCalculateRankIndicator
// LAYER      :: PHY
// PURPOSE    :: Calculate rank indicator
// PARAMETERS ::
// + node         : Node*         : Pointer to node.
// + phyIndex     : int           : Index of the PHY.
// + phyLte       : PhyDataLte*   : Pointer to LTE PHY structure
// + propTxInfo   : PropTxInfo*   : Pointer to propagation tx info
// + lteTxInfo    : PhyLteTxInfo* : Pointer to LTE transmission info
// + pathloss_dB  : double        : Pathloss of received signal in dB
// RETURN     :: int : Rank indicator
// **/
int PhyLteCalculateRankIndicator(
        Node* node, int phyIndex,
        PhyDataLte* phyLte,
        PropTxInfo* propTxInfo, // Tx info of desired signal
        PhyLteTxInfo* lteTxInfo, // Tx info of desired signal
        double pathloss_dB)
{
    ERROR_Assert(phyLte->stationType == LTE_STATION_TYPE_UE,
                "No need to calculate SCI on eNB");
    LteRnti txRnti(propTxInfo->txNodeId, propTxInfo->phyIndex); // for log

    // SINR for CQI is calculated assume transmission scheme
    // determined by transmission mode and RI applied for certain RBs
    // Whether the RI have already conveyed to eNB is not considered.

#ifdef LTE_LIB_LOG
    std::vector < LteTxScheme > txSchemes;
#endif

    std::vector < int > candidateNumTransportBlocksToSend;
    std::vector < int > candidateRankIndicators;

    char errBuf[MAX_STRING_LENGTH] = {0};

    int transmissionMode = GetLteMacConfig(node,
            node->phyData[phyIndex]->macInterfaceIndex)->transmissionMode;

    switch (transmissionMode){
    case 1:
#ifdef LTE_LIB_LOG
        txSchemes.push_back(TX_SCHEME_SINGLE_ANTENNA);
#endif
        candidateNumTransportBlocksToSend.push_back(1);
        candidateRankIndicators.push_back(1);
        break;
    case 2:
#ifdef LTE_LIB_LOG
        txSchemes.push_back(TX_SCHEME_DIVERSITY);
#endif
        candidateNumTransportBlocksToSend.push_back(1);
        candidateRankIndicators.push_back(1);
        break;
    case 3:
#if 1
#ifdef LTE_LIB_LOG
        txSchemes.push_back(TX_SCHEME_DIVERSITY);
#endif
        candidateNumTransportBlocksToSend.push_back(1);
        candidateRankIndicators.push_back(1);
#endif

        // If number of rx antennas is not 2, spatial multiplexing
        // is not used.
        if (phyLte->numRxAntennas == 2)
        {
#ifdef LTE_LIB_LOG
            txSchemes.push_back(TX_SCHEME_OL_SPATIAL_MULTI);
#endif
            candidateNumTransportBlocksToSend.push_back(2);
            candidateRankIndicators.push_back(2);
        }
        break;
    default:
        sprintf(errBuf,
            "Transmission mode %d is not supported. ", transmissionMode);
        ERROR_Assert(FALSE,errBuf);
    }

    int len;
    const PhyLteCqiTableColumn* cqiTable =
            PhyLteGetCqiTable(node, phyIndex, &len);

    double maxEfficiency = 0.0;
    int maxEfficiencyIndex = 0;


    for (size_t i = 0; i < candidateRankIndicators.size(); ++i)
    {
        double efficiency = 0.0;

        PhyLteCqi cqis =
            PhyLteCalculateCqi(
                    node, phyIndex,
                    phyLte, propTxInfo, lteTxInfo,
                    candidateRankIndicators[i],
                    pathloss_dB,
                    false);

        for (int tbIndex = 0;
            tbIndex < candidateNumTransportBlocksToSend[i];
            ++tbIndex)
        {
            int cqi = (tbIndex == 0 ?
                    cqis.cqi0 : cqis.cqi1);

            efficiency += cqiTable[cqi].efficiency;
        }
#ifdef LTE_LIB_LOG
        std::stringstream ss;
        ss << "ri=," << candidateRankIndicators[i] << ","
           << "txScheme=," << LteGetTxSchemeString(txSchemes[i]) << ","
           << "cqi=," << cqis.cqi0 << "," << cqis.cqi1 << ","
           << "efficiency=," << efficiency << ",";

        lte::LteLog::DebugFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,%s",
            "RiCalculationDetail",
            ss.str().c_str());
#endif

        if (efficiency > maxEfficiency)
        {
            maxEfficiency = efficiency;
            maxEfficiencyIndex = i;
        }
    }

#ifdef LTE_LIB_LOG
    std::stringstream ss;
    ss << "ri=," << candidateRankIndicators[maxEfficiencyIndex] << ","
       << "txScheme=," << LteGetTxSchemeString(txSchemes[maxEfficiencyIndex])
       << "," << "efficiency=," << maxEfficiency << ",";

    lte::LteLog::DebugFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s,%s",
        "RiCalculation",
        ss.str().c_str());

#endif

    // Adopt highest efficiency realizing CQI as a next reported CQI info.
    return candidateRankIndicators[maxEfficiencyIndex];
}

// /**
// FUNCTION   :: PhyLteCalculateCqi
// LAYER      :: PHY
// PURPOSE    :: Calculate CQI
// PARAMETERS ::
// + node           : Node*         : Pointer to node.
// + phyIndex       : int           : Index of the PHY.
// + phyLte         : PhyDataLte*   : Pointer to LTE PHY structure
// + propTxInfo     : PropTxInfo*   : Pointer to propagation tx info
// + lteTxInfo      : PhyLteTxInfo* : Pointer to LTE transmission info
// + rankIndicator  : int           : Rank indicator
// + pathloss_dB    : double        : Pathloss of received signal in dB
// + forCqiFeedback : bool          : For CQI feedback or not
//                                    (For aggregation log)
// RETURN     :: int : Rank indicator
// **/
PhyLteCqi PhyLteCalculateCqi(
        Node* node, int phyIndex,
        PhyDataLte* phyLte,
        PropTxInfo* propTxInfo, // Tx info of desired signal
        PhyLteTxInfo* lteTxInfo, // Tx info of desired signal
        int rankIndicator,
        double pathloss_dB,
        bool forCqiFeedback) // for log
{
    ERROR_Assert(phyLte->stationType == LTE_STATION_TYPE_UE,
                "No need to calculate SCI on eNB");
    LteRnti txRnti(propTxInfo->txNodeId, propTxInfo->phyIndex); // for log

    // Retrieve channel matrix
    int matrixSize = phyLte->numRxAntennas * lteTxInfo->numTxAnntena;
    Cmat matHhat(LTE_DEFAULT_DCOMP, matrixSize);

    PhyLteGetChannelMatrix(
        node, phyIndex, phyLte,
        propTxInfo, lteTxInfo, pathloss_dB, matHhat);

    int numTransportBlocksToSend = 0;
    char errBuf[MAX_STRING_LENGTH] = {0};

    LteTxScheme txScheme = TX_SCHEME_INVALID;

    int transmissionMode = GetLteMacConfig(node,
            node->phyData[phyIndex]->macInterfaceIndex)->transmissionMode;

    switch (transmissionMode){
    case 1:
        txScheme = TX_SCHEME_SINGLE_ANTENNA;
        numTransportBlocksToSend = 1;
        break;
    case 2:
        txScheme = TX_SCHEME_DIVERSITY;
        numTransportBlocksToSend = 1;
        break;
    case 3:
        if (rankIndicator == 1)
        {
            txScheme = TX_SCHEME_DIVERSITY;
            numTransportBlocksToSend = 1;
        }else if (rankIndicator == 2)
        {
            txScheme = TX_SCHEME_OL_SPATIAL_MULTI;
            numTransportBlocksToSend = 2;
        }else
        {
            ERROR_Assert(FALSE, "Invalid RI");
        }
        break;
    default:
        sprintf(errBuf,
            "Transmission mode %d is not supported. ", transmissionMode);
            ERROR_Assert(FALSE,errBuf);
    }

    // Select the txScheme achieving highest efficiency

    UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB];

    // 1. Wideband CQI is calculated assume all of the RBs are
    //    used for transmission.
    for (int rbIndex = 0; rbIndex < PHY_LTE_MAX_NUM_RB; ++rbIndex)
    {
        usedRB_list[rbIndex] = 1;
    }

    double txPower_mW = NON_DB(lteTxInfo->txPower_dBm);

    std::vector < double > sinr = PhyLteCalculateSinr(
        node, phyIndex,
        txScheme, phyLte, usedRB_list,
        matHhat, 1.0/NON_DB(pathloss_dB),
        txPower_mW,
        true,
        true,
        txRnti);

    PhyLteCqi cqis = {0,0};

    for (int tbIndex = 0; tbIndex < numTransportBlocksToSend; ++tbIndex)
    {
        int& cqi = (tbIndex == 0 ?
                cqis.cqi0 : cqis.cqi1);

        double sinrForCqiSelection_dB =
                IN_DB(sinr[tbIndex]) - PHY_LTE_SNR_OFFSET_FOR_CQI_SELECTION;

        cqi =
            PhyLteGetDlCqiSnrTableIndex(node,
                                        phyIndex,
                                        phyLte,
                                        sinrForCqiSelection_dB);

#ifdef LTE_LIB_LOG
        if (forCqiFeedback)
        {
            phyLte->aggregator->regist(
                txRnti,
                lte::Aggregator::CQI_TB_SINR,
                IN_DB(sinr[tbIndex]));
            phyLte->aggregator->regist(txRnti, lte::Aggregator::CQI, cqi);
        }
#endif
    }

    return cqis;
}


// /**
// FUNCTION   :: PhyLteNoOverlap
// LAYER      :: PHY
// PURPOSE    :: Check if the new signal has overlap with existing signals
//               under receiving already.
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + phyLte    : PhyDataLte*        : Pointer to the LTE PHY structure
// + newRxMsgList : newRxMsgInfo* : Information of new signal
// RETURN     :: BOOL : TRUE if has collision, FALSE otherwise
// **/
static
BOOL PhyLteNoOverlap(Node* node,
                     PhyDataLte* phyLte,
                     PhyLteRxMsgInfo* newRxMsgInfo)
{
    // Not supported for first release.
    return FALSE;
}


// /**
// FUNCTION   :: PhyLteCheckRxPacketError
// LAYER      :: PHY
// PURPOSE    :: Check if any receiving message has error.
// PARAMETERS ::
// + node       : Node*         : Pointer to node.
// + phyIndex   : int           : Index of the PHY.
// + phyLte     : PhyDataLte*   : Pointer to the LTE PHY structure.
// + mcsIndex   : UInt8         : Mcs index allocated
// + numRb      : int           : Number of RBs allocated
// + sinr       : double        : Sinr
// + propTxInfo : PropTxInfo*   : Pointer to propagation tx info.
// RETURN     :: BOOL          : TRUE Reception failure
//                             : FALSE Reception Success
// **/
BOOL PhyLteCheckRxPacketError(Node* node,
                              int phyIndex,
                              PhyDataLte* phyLte,
                              PhyLteTbsInfo* tbsInfo,
                              UInt8 mcsIndex,
                              int numRb,
                              double sinr,
                              PropTxInfo* propTxInfo) // Just for log
{
    ERROR_Assert(tbsInfo->isError == FALSE, "TB already has an error.");

    double BER = 0.0;
    double PPER = 0.0;
    double partOfPacketSize = 0.0;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    // check if exist msg in list and no error.

    clocktype ttiLength = MacLteGetTtiLength(node, interfaceIndex);

    int berTableIndex;

    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        berTableIndex = mcsIndex + PHY_LTE_MCS_INDEX_LEN;
    }
    else
    {
        berTableIndex = mcsIndex;
    }

    BER = PHY_BER(phyLte->thisPhy, berTableIndex, sinr);

    int txBlockSize = 0;
    // DownLink
    if (phyLte->stationType == LTE_STATION_TYPE_UE)
    {
        txBlockSize
            = PhyLteGetDlTxBlockSize(node,
                                     phyIndex,
                                     mcsIndex,
                                     numRb);
    }
    // UpLink
    else
    {
        txBlockSize
            = PhyLteGetUlTxBlockSize(node,
                                     phyIndex,
                                     mcsIndex,
                                     numRb);
    }

    tbsInfo->transportBlockSize = txBlockSize;

    int numSfTti = MacLteGetNumSubframePerTti(node, interfaceIndex);

    partOfPacketSize = (double)txBlockSize * numSfTti
        * ((double)(node->getNodeTime() - tbsInfo->rxTimeEvaluated) /
        (ttiLength - PHY_LTE_SIGNAL_DURATION_REDUCTION));

    PPER = 1.0 - pow((1.0 - BER), partOfPacketSize);

#ifdef LTE_LIB_LOG
    LteRnti txRnti(propTxInfo->txNodeId, propTxInfo->phyIndex);
    double logPPER = (PPER == 0.0 ? -10.0 : log10(PPER));
    phyLte->aggregator->regist(txRnti, lte::Aggregator::PPER, logPPER);
#endif

    ERROR_Assert(PPER >= 0.0 && PPER <= 1.0, "Invalid PPER");

#ifdef ERROR_DETECTION_DISABLE

    PPER = phyLte->debugPPER;

#endif

    double rand = RANDOM_erand(phyLte->thisPhy->seed);

    if (PPER > rand) {
        tbsInfo->isError = TRUE;
    }else{
        tbsInfo->isError = FALSE;
    }

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    tbsInfo->sinr = sinr;
    tbsInfo->mcs = mcsIndex;
#endif
#endif

#ifdef LTE_LIB_LOG
    char timeStr[20];
    TIME_PrintClockInSecond(tbsInfo->rxTimeEvaluated, timeStr);
    lte::LteLog::DebugFormat(node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            LTE_STRING_LAYER_TYPE_PHY,
                            "%s,Rnti=,"LTE_STRING_FORMAT_RNTI
                            ",Sinr_dB=,%e,Mcs=,%d,BER=,%e,PrevEvalTime=,%s,"
                            "PartOfPacketSize=,%f,Pper=,%e,IsError=,%s",
                            LTE_STRING_CATEGORY_TYPE_PPER,
                            propTxInfo->txNodeId, propTxInfo->phyIndex,
                            IN_DB(sinr),
                            (int)mcsIndex,
                            BER,
                            timeStr,
                            partOfPacketSize,
                            PPER,
                            tbsInfo->isError ? "TRUE" : "FALSE");
#endif
    tbsInfo->rxTimeEvaluated = node->getNodeTime();


    return tbsInfo->isError;
}

// /**
// FUNCTION   :: PhyLteCheckAllOfRxPacketError
// LAYER      :: PHY
// PURPOSE    :: Check if any receiving message has error.
// PARAMETERS ::
// + node       : Node*         : Pointer to node.
// + phyIndex   : int           : Index of the PHY.
// + propRxInfo : PropRxInfo*   : Propagation reception information
// RETURN     :: void : NULL
// **/
void PhyLteCheckAllOfRxPacketError(Node* node,
                                  int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    PhyLteRxMsgInfo* rxMsgInfo = NULL;

    // Error check for all of the desired signal
    std::list < PhyLteRxMsgInfo* > ::iterator it;
    for (it = phyLte->rxPackMsgList->begin();
         it != phyLte->rxPackMsgList->end();
         it++)
    {
        rxMsgInfo = (*it);
        // Calculate SINR for each TB and check error
        Message* rfMsg = rxMsgInfo->rxMsg;

        PhyLteTxInfo* lteTxInfo = &rxMsgInfo->lteTxInfo;

        // Calculate channel matrix
        int matrixSize = phyLte->numRxAntennas * lteTxInfo->numTxAnntena;
        Cmat matHhat(LTE_DEFAULT_DCOMP,matrixSize);
        PhyLteGetChannelMatrix(node,
                                 phyIndex,
                                 phyLte,
                                 &rxMsgInfo->propTxInfo,
                                 lteTxInfo,
                                 IN_DB(1.0/rxMsgInfo->geometry),
                                 matHhat);

        while (rfMsg != NULL)
        {
            // If rfMsg is for me, check error. If not, skip.
            MacLteMsgDestinationInfo* dstInfo =
                (MacLteMsgDestinationInfo*)MESSAGE_ReturnInfo(rfMsg,
                (unsigned short)INFO_TYPE_LteMacDestinationInfo);
            ERROR_Assert(dstInfo != NULL,
                    "PHY-LTE: not detect INFO_TYPE_LteMacDestinationInfo!");

            bool isForMe = (dstInfo->dstRnti ==
                        LteLayer2GetRnti(node, thisPhy->macInterfaceIndex));

            // DCI 0 is skipped
            LteDciFormat0* DciForUl =
                (LteDciFormat0*)MESSAGE_ReturnInfo(
                    rfMsg,
                    (unsigned short)INFO_TYPE_LteDci0Info);

            if ((DciForUl == NULL) && (isForMe == true))
            {
                UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB];
                memset(usedRB_list, 0, sizeof(usedRB_list));

                int numCodeWords;
                bool tb2cwSwapFlag;
                int numRb;
                std::vector < UInt8 > mcsIndex;

                // Judge transmission scheme used
                LteTxScheme txScheme = PhyLteJudgeTxScheme(
                                                    node, phyIndex, rfMsg);

                // Retrieve resource allocation information for each
                // transport blocks
                Message* nextRfMsg = PhyLteGetResourceAllocationInfo(
                    node,
                    phyIndex,
                    rfMsg,
                    txScheme,
                    &numCodeWords,
                    &tb2cwSwapFlag,
                    usedRB_list,
                    &numRb,
                    &mcsIndex);

                // 1. Calculate average SINR

                LteRnti txRnti(
                    rxMsgInfo->propTxInfo.txNodeId,
                    rxMsgInfo->propTxInfo.phyIndex);  // for log

                // Sinrs for each transport blocks
                std::vector<double> sinr =
                    PhyLteCalculateSinr(
                        node,
                        phyIndex,
                        txScheme,
                        phyLte,
                        usedRB_list,
                        matHhat,
                        rxMsgInfo->geometry,
                        rxMsgInfo->txPower_mW,
                        false,
                        false,
                        txRnti);

                for (int cwIndex = 0; cwIndex < numCodeWords; ++cwIndex)
                {
                    Message* tbMsg = (cwIndex == 0) ? rfMsg : nextRfMsg;
                    int tbIndex = (tb2cwSwapFlag ? 1 - cwIndex : cwIndex);

#ifdef LTE_LIB_LOG
                    stringstream ss;
                    char buf[MAX_STRING_LENGTH];
                    ss << "rnti=," << STR_RNTI(buf, txRnti) << ","
                       << "tbNo=," << (int)(tbIndex) <<","
                       << "Non_dB=," << sinr[tbIndex] << ","
                       << "dB=," << IN_DB(sinr[tbIndex]) << ",";

                    lte::LteLog::DebugFormat(
                        node,
                        node->phyData[phyIndex]->macInterfaceIndex,
                        LTE_STRING_LAYER_TYPE_PHY,
                        "%s,%s",
                        LTE_STRING_CATEGORY_TYPE_TB_SINR,
                        ss.str().c_str()
                    );
#endif

#ifdef LTE_LIB_LOG
                    phyLte->aggregator->regist(txRnti,
                                               lte::Aggregator::TB_SINR,
                                               IN_DB(sinr[tbIndex]));
#endif

                    // Check packet error
                    std::map < Message*, PhyLteTbsInfo > ::iterator it
                        = phyLte->rxMsgErrorStat->find(tbMsg);

                    ERROR_Assert(
                        it != phyLte->rxMsgErrorStat->end(),
                        "Reception signal not found");

                    if (it->second.isError == FALSE)
                    {
                        PhyLteCheckRxPacketError(node,
                                                 phyIndex,
                                                 phyLte,
                                                 &it->second,
                                                 mcsIndex[tbIndex],
                                                 numRb,
                                                 sinr[tbIndex],
                                                 &rxMsgInfo->propTxInfo);
                    }
                } // For TB

                // If 2 transport blocks are detected nextRfMsg = rfMsg->next
                rfMsg = nextRfMsg;
            }
            // Next message
            rfMsg = rfMsg->next;
        }
    }
}


// /**
// FUNCTION   :: PhyLteSetNumResourceBlocks
// LAYER      :: PHY
// PURPOSE    :: Set the number of RBs used by this PHY
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
static
void PhyLteSetNumResourceBlocks(Node* node, int phyIndex)
{
    PhyDataLte* phyLte =
        (PhyDataLte*)(node->phyData[phyIndex]->phyVar);

    switch (phyLte->channelBandwidth)
    {
    case PHY_LTE_CHANNEL_BANDWIDTH_1400KHZ:
        phyLte->numResourceBlocks =
            PHY_LTE_CHANNEL_BANDWIDTH_1400KHZ_NUM_RB;
        break;
    case PHY_LTE_CHANNEL_BANDWIDTH_3MHZ:
        phyLte->numResourceBlocks =
            PHY_LTE_CHANNEL_BANDWIDTH_3MHZ_NUM_RB;
        break;
    case PHY_LTE_CHANNEL_BANDWIDTH_5MHZ:
        phyLte->numResourceBlocks =
            PHY_LTE_CHANNEL_BANDWIDTH_5MHZ_NUM_RB;
        break;
    case PHY_LTE_CHANNEL_BANDWIDTH_10MHZ:
        phyLte->numResourceBlocks =
            PHY_LTE_CHANNEL_BANDWIDTH_10MHZ_NUM_RB;
        break;
    case PHY_LTE_CHANNEL_BANDWIDTH_15MHZ:
        phyLte->numResourceBlocks =
            PHY_LTE_CHANNEL_BANDWIDTH_15MHZ_NUM_RB;
        break;
    case PHY_LTE_CHANNEL_BANDWIDTH_20MHZ:
        phyLte->numResourceBlocks =
            PHY_LTE_CHANNEL_BANDWIDTH_20MHZ_NUM_RB;
        break;
    default:
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
                "PHY-LTE: Channel bandwidth as %d is not supported."
                "Only 1400000, 3000000, 5000000, 10000000, 20000000"
                "are available.",
                phyLte->channelBandwidth);
        ERROR_Assert(FALSE, errorStr);
    }
    return;
}


// /**
// FUNCTION   :: PhyLteCheckParameterBer
// LAYER      :: PHY
// PURPOSE    :: To determine the number of imported files.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteCheckParameterBer(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    if (thisPhy->phyRxModel == PHY_LTE_BER_BASED)
    {
        if (thisPhy->numBerTables != PHY_LTE_DEFAULT_BER_TABLE_LEN)
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                "PHY-LTE: Ber Table Len as %d is not supported."
                "Only %d is available.",
                thisPhy->numBerTables,
                PHY_LTE_DEFAULT_BER_TABLE_LEN);
            ERROR_Assert(FALSE, errorStr);
        }
#if 0
#ifdef LTE_LIB_LOG
        int i;
        for (i = 0; i < thisPhy->numBerTables; i++)
        {
            lte::LteLog::Debug2Format(
                node,
                node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteChackParameterBer",
                "%d,%d,%s,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
                i,
                thisPhy->numBerTables,
                thisPhy->snrBerTables[i].fileName,
                thisPhy->snrBerTables[i].numEntries,
                thisPhy->snrBerTables[i].isFixedInterval,
                thisPhy->snrBerTables[i].interval,
                thisPhy->snrBerTables[i].snrStart,
                thisPhy->snrBerTables[i].snrEnd,
                thisPhy->snrBerTables[i].entries[0].snr,
                thisPhy->snrBerTables[i].entries[0].ber,
                thisPhy->snrBerTables[i].entries
                            [thisPhy->snrBerTables[i].numEntries - 1].snr,
                thisPhy->snrBerTables[i].entries
                            [thisPhy->snrBerTables[i].numEntries - 1].ber);
        }
#endif // LTE_LIB_LOG
#endif
    }
    return;
}


// /**
// FUNCTION   :: PhyLteInitDefaultParameters
// LAYER      :: PHY
// PURPOSE    :: Initialize the parameters.
//               Parameters that depend on other parameters and
//               non-configurable parameters.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
static
void PhyLteInitDefaultParameters(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    PhyLteSetNumResourceBlocks(node, phyIndex);

    phyLte->rxTxTurnaroundTime = PHY_LTE_RX_TX_TURNAROUND_TIME;

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(
                              node,
                              node->phyData[phyIndex]->macInterfaceIndex,
                              "PHY:DEBUG:PhyLteInitDefaultParameters",
                              "%lld",
                              phyLte->rxTxTurnaroundTime);
#endif // LTE_LIB_LOG

    return;
}

// /**
// FUNCTION   :: PhyLteSetParametersTxPower
// LAYER      :: PHY
// PURPOSE    :: Initialize the maximum transmit power.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to node input
// + phyLte    : PhyDataLte*  : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersTxPower(Node* node,
                                int phyIndex,
                                const NodeInput* nodeInput,
                                PhyDataLte* phyLte,
                                BOOL* wasFound)
{
    double txPower_dBm = 0;

    IO_ReadDouble(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-TX-POWER",
                  wasFound,
                  &txPower_dBm);

    if (*wasFound)
    {
        if ((LTE_NEGATIVE_INFINITY_POWER_dBm <= txPower_dBm) &&
            (txPower_dBm <= LTE_INFINITY_POWER_dBm))
        {
            phyLte->maxTxPower_dBm = txPower_dBm;
        }else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                "PHY-LTE: Invalid transmission power : %f dBm. "
                "Transmission power should be in the range [%f, %f] dBm",
                txPower_dBm, LTE_NEGATIVE_INFINITY_POWER_dBm,
                LTE_INFINITY_POWER_dBm);
            ERROR_Assert(FALSE, errorStr);
        }
    }else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Transmission power should be set."
            "Change Transmission power to %f.",
            PHY_LTE_DEFAULT_TX_POWER_DBM);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->maxTxPower_dBm = PHY_LTE_DEFAULT_TX_POWER_DBM;
    }

}


// /**
// FUNCTION   :: PhyLteSetParametersChannelBandwidth
// LAYER      :: PHY
// PURPOSE    :: Initialize the channel bandwidth
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + subnetInterfase   : NodeAddress : Subnet interface.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void             : NULL
// **/
void PhyLteSetParametersChannelBandwidth(Node* node,
                                         int phyIndex,
                                         const NodeInput* nodeInput,
                                         NodeAddress subnetInterfase,
                                         PhyDataLte* phyLte,
                                         BOOL* wasFound)
{
    int channelbandwidth = 0;

    IO_ReadInt(node->nodeId,
               subnetInterfase,
               nodeInput,
               "PHY-LTE-CHANNEL-BANDWIDTH",
               wasFound,
               &channelbandwidth);

    if (*wasFound)
    {
        if (channelbandwidth == PHY_LTE_CHANNEL_BANDWIDTH_1400KHZ ||
            channelbandwidth == PHY_LTE_CHANNEL_BANDWIDTH_3MHZ    ||
            channelbandwidth == PHY_LTE_CHANNEL_BANDWIDTH_5MHZ    ||
            channelbandwidth == PHY_LTE_CHANNEL_BANDWIDTH_10MHZ   ||
            channelbandwidth == PHY_LTE_CHANNEL_BANDWIDTH_15MHZ   ||
            channelbandwidth == PHY_LTE_CHANNEL_BANDWIDTH_20MHZ)
        {
            phyLte->channelBandwidth = channelbandwidth;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "PHY-LTE: Channel bandwidth as %d is not supported."
                    "Only 1400000, 3000000, 5000000, 10000000, 20000000 "
                    "are available.",
                    channelbandwidth);
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Channel bandwidth should be set."
            "Change Channel bandwidth to %d.",
            PHY_LTE_DEFAULT_CHANNEL_BANDWIDTH);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->channelBandwidth = PHY_LTE_DEFAULT_CHANNEL_BANDWIDTH;
    }
}


// /**
// FUNCTION   :: PhyLteSetParametersDlChannelNo
// LAYER      :: PHY
// PURPOSE    :: Initializing the number of Downlink.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersDlChannelNo(Node* node,
                                    int phyIndex,
                                    const NodeInput* nodeInput,
                                    PhyDataLte* phyLte,
                                    BOOL* wasFound)
{
    char string[MAX_STRING_LENGTH];
    Int32 channelIndex = -1;

    if (phyLte->stationType == LTE_STATION_TYPE_UE)
    {
        phyLte->dlChannelNo = 0;
        return;
    }

    IO_ReadString(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-DL-CHANNEL",
                  wasFound,
                  string);

    if (*wasFound)
    {
        if (IO_IsStringNonNegativeInteger(string))
        {
            channelIndex = (Int32)strtoul(string, NULL, 10);
            if (channelIndex < node->numberChannels)
            {
                phyLte->dlChannelNo = channelIndex;
            }
            else
            {
                ERROR_ReportErrorArgs("PHY-LTE-DL-CHANNEL: Downlink Channel"
                                      " should be an integer between %d to %d"
                                      " or a valid channel name.",
                                      PHY_LTE_DEFAULT_DL_CANNEL,
                                      (node->numberChannels - 1));
            }
        }
        else if (isalpha(*string) && PHY_ChannelNameExists(node, string))
        {
            phyLte->dlChannelNo = PHY_GetChannelIndexForChannelName(node,
                                                                    string);
        }
        else
        {
            ERROR_ReportErrorArgs("PHY-LTE-DL-CHANNEL: Downlink Channel should"
                                  " be an integer between %d to %d or a"
                                  " valid channel name.",
                                  PHY_LTE_DEFAULT_DL_CANNEL,
                                  (node->numberChannels - 1));
        }
    }
    else
    {
        ERROR_ReportWarningArgs(
            "PHY-LTE-DL-CHANNEL: Downlink Channel has not been configured."
            " Channel %d will be used as the Downlink Channel.",
            PHY_LTE_DEFAULT_DL_CANNEL);
        // default
        phyLte->dlChannelNo = PHY_LTE_DEFAULT_DL_CANNEL;
    }
}

// /**
// FUNCTION   :: PhyLteSetParametersUlChannelNo
// LAYER      :: PHY
// PURPOSE    :: Initializing the number of uplink.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersUlChannelNo(Node* node,
                                    int phyIndex,
                                    const NodeInput* nodeInput,
                                    PhyDataLte* phyLte,
                                    BOOL* wasFound)
{
    char string[MAX_STRING_LENGTH];
    Int32 channelIndex = -1;

    if (phyLte->stationType == LTE_STATION_TYPE_UE)
    {
        phyLte->ulChannelNo = 1;
        return;
    }

    IO_ReadString(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-UL-CHANNEL",
                  wasFound,
                  string);

    if (*wasFound)
    {
        if (IO_IsStringNonNegativeInteger(string))
        {
            channelIndex = (Int32)strtoul(string, NULL, 10);

            if (channelIndex < node->numberChannels)
            {
                phyLte->ulChannelNo = channelIndex;
            }
            else
            {
                ERROR_ReportErrorArgs("PHY-LTE-UL-CHANNEL: Uplink Channel"
                                      " should be an integer between %d to %d"
                                      " or a valid channel name.",
                                      PHY_LTE_DEFAULT_UL_CANNEL,
                                      (node->numberChannels - 1));
            }
        }
        else if (isalpha(*string) && PHY_ChannelNameExists(node, string))
        {
            phyLte->ulChannelNo = PHY_GetChannelIndexForChannelName(node,
                                                                    string);
        }
        else
        {
            ERROR_ReportErrorArgs("PHY-LTE-UL-CHANNEL: Uplink Channel should"
                                  " be an integer between %d to %d or a "
                                  "valid channel name.",
                                  PHY_LTE_DEFAULT_UL_CANNEL,
                                  (node->numberChannels - 1));
        }
    }
    else
    {
        ERROR_ReportWarningArgs(
            "PHY-LTE-UL-CHANNEL: Uplink Channel has not been configured."
            " Channel %d will be used as the Uplink Channel.",
            PHY_LTE_DEFAULT_UL_CANNEL);
        // default
        phyLte->ulChannelNo = PHY_LTE_DEFAULT_UL_CANNEL;
    }
}


// /**
// FUNCTION   :: PhyLteSetParametersTxAntennas
// LAYER      :: PHY
// PURPOSE    :: Initialization the number of transmit antennas.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersTxAntennas(Node* node,
                                   int phyIndex,
                                   const NodeInput* nodeInput,
                                   PhyDataLte* phyLte,
                                   BOOL* wasFound)
{

    int numTxAntennas = 0;
    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-NUM-TX-ANTENNAS",
               wasFound,
               &numTxAntennas);

    if (*wasFound)
    {
        if (PHY_LTE_MIN_NUM_TX_ANTENNAS <= numTxAntennas
            && numTxAntennas <= PHY_LTE_MAX_NUM_TX_ANTENNAS)
        {
            phyLte->numTxAntennas = (UInt8)numTxAntennas;

        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: Num Tx Antennas "
                    "should be %d to %d",
                    PHY_LTE_MIN_NUM_TX_ANTENNAS,
                    PHY_LTE_MAX_NUM_TX_ANTENNAS);
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Num Tx Antennas should be set."
            "Change Num Tx Antennas to %d.",
            (UInt8)PHY_LTE_DEFAULT_NUM_TX_ANTENNAS);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->numTxAntennas = (UInt8)PHY_LTE_DEFAULT_NUM_TX_ANTENNAS;
    }

    // check UE num tx antennas
    if ((phyLte->stationType == LTE_STATION_TYPE_UE) &&
        (phyLte->numTxAntennas > PHY_LTE_MAX_UE_NUM_TX_ANTENNAS))
    {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "PHY-LTE: Num UE Tx Antennas "
                    "should be %d",
                    PHY_LTE_MAX_UE_NUM_TX_ANTENNAS);
            ERROR_Assert(FALSE, errorStr);
    }
}


// /**
// FUNCTION   :: PhyLteSetParametersRxAntennas
// LAYER      :: PHY
// PURPOSE    :: Initializing the number of receiving antennas.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersRxAntennas(Node* node,
                                   int phyIndex,
                                   const NodeInput* nodeInput,
                                   PhyDataLte* phyLte,
                                   BOOL* wasFound)
{
    int numRxAntennas = 0;

    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-NUM-RX-ANTENNAS",
               wasFound,
               &numRxAntennas);

    if (*wasFound)
    {
        if (PHY_LTE_MIN_NUM_RX_ANTENNAS <= numRxAntennas
            && numRxAntennas <= PHY_LTE_MAX_NUM_RX_ANTENNAS)
        {
            phyLte->numRxAntennas = (UInt8)numRxAntennas;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: Num Rx Antennas "
                    "should be %d to %d",
                    PHY_LTE_MIN_NUM_RX_ANTENNAS,
                    PHY_LTE_MAX_NUM_RX_ANTENNAS);
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Num Rx Antennas should be set."
            "Change Num Rx Antennas to %d.",
            (UInt8)PHY_LTE_DEFAULT_NUM_RX_ANTENNAS);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->numRxAntennas = (UInt8)PHY_LTE_DEFAULT_NUM_RX_ANTENNAS;
    }


}


// /**
// FUNCTION   :: PhyLteSetParametersPucchOverhead
// LAYER      :: PHY
// PURPOSE    :: Initializing the PucchOverhead.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersPucchOverhead(Node* node,
                                      int phyIndex,
                                      const NodeInput* nodeInput,
                                      PhyDataLte* phyLte,
                                      BOOL* wasFound)
{
    int numPucchOverhead = 0;

    Layer2DataLte* lte =
        (Layer2DataLte*)(node->macData[phyIndex]->macVar);
    if (lte->stationType == LTE_STATION_TYPE_UE)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-PUCCH-OVERHEAD",
               wasFound,
               &numPucchOverhead);

    if (*wasFound)
    {
        if (PHY_LTE_MIN_PUCCH_OVERHEAD <= numPucchOverhead &&
            numPucchOverhead <= phyLte->numResourceBlocks)
        {
            phyLte->numPucchOverhead = (UInt32)numPucchOverhead;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: PUCCH overhead "
                         "should be %d to %d",
                         PHY_LTE_MIN_PUCCH_OVERHEAD,
                         phyLte->numResourceBlocks);
            ERROR_Assert(FALSE, errorStr);


        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: PUCCH overhead should be set."
            "Change PUCCH overhead to %d.",
            (UInt32)PHY_LTE_DEFAULT_NUM_PUCCH_OVERHEAD);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->numPucchOverhead =
            (UInt32)PHY_LTE_DEFAULT_NUM_PUCCH_OVERHEAD;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersTpcPoPusch
// LAYER      :: PHY
// PURPOSE    :: Initializing the TpcPoPusch.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersTpcPoPusch(Node* node,
                                   int phyIndex,
                                   const NodeInput* nodeInput,
                                   PhyDataLte* phyLte,
                                   BOOL* wasFound)
{
    double tpcPoPusch_mWPerRb = 0;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_UE)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadDouble(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-TPC-P_O_PUSCH",
                  wasFound,
                  &tpcPoPusch_mWPerRb);

    if (*wasFound)
    {
        if (LTE_NEGATIVE_INFINITY_POWER_dBm <= tpcPoPusch_mWPerRb &&
            tpcPoPusch_mWPerRb <= LTE_INFINITY_POWER_dBm)
        {
            phyLte->tpcPoPusch_dBmPerRb = tpcPoPusch_mWPerRb;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: P_O_PUSCH for UL TPC "
                    "should be %.2lf to %.2lf",
                    (double)LTE_NEGATIVE_INFINITY_POWER_dBm,
                    (double)LTE_INFINITY_POWER_dBm);
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: P_O_PUSCH for UL TPC should be set."
            "Change P_O_PUSCH for UL TPC to %.2lf.",
            PHY_LTE_DEFAULT_TPC_P_O_PUSCH);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->tpcPoPusch_dBmPerRb =
            PHY_LTE_DEFAULT_TPC_P_O_PUSCH;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersTpcAlpha
// LAYER      :: PHY
// PURPOSE    :: Initializing the TpcAlpha.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void : NULL
// **/
void PhyLteSetParametersTpcAlpha(Node* node,
                                 int phyIndex,
                                 const NodeInput* nodeInput,
                                 PhyDataLte* phyLte,
                                 BOOL* wasFound)
{
    int tpcAlpha = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_UE)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-TPC-ALPHA",
               wasFound,
               &tpcAlpha);

    if (*wasFound)
    {
        if (PHY_LTE_MIN_TPC_ALPHA <= tpcAlpha &&
            tpcAlpha <= PHY_LTE_MAX_TPC_ALPHA)
        {
            phyLte->tpcAlpha = (UInt8)tpcAlpha;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: TPC alpha "
                    "should be %d to %d.",
                    PHY_LTE_MIN_TPC_ALPHA,
                    PHY_LTE_MAX_TPC_ALPHA);
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: TPC alpha should be set."
            "Change TPC alpha to %d.",
            (UInt8)PHY_LTE_DEFAULT_TPC_ALPHA);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->tpcAlpha =
            (UInt8)PHY_LTE_DEFAULT_TPC_ALPHA;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersCqiReportingInterval
// LAYER      :: PHY
// PURPOSE    :: Initializing the cqi reporting interval.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersCqiReportingInterval(Node* node,
                                             int phyIndex,
                                             const NodeInput* nodeInput,
                                             PhyDataLte* phyLte,
                                             BOOL* wasFound)
{
    int cqiReportingInterval = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_ENB)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-CQI-REPORTING-INTERVAL",
               wasFound,
               &cqiReportingInterval);

    if (*wasFound)
    {
        if (PHY_LTE_MIN_CQI_REPORTING_INTERVAL <= cqiReportingInterval &&
            cqiReportingInterval < PHY_LTE_MAX_CQI_REPORTING_INTERVAL)
        {
            phyLte->cqiReportingInterval = cqiReportingInterval;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: CQI reporting interval "
                    "should be %d to %d.",
                    PHY_LTE_MIN_CQI_REPORTING_INTERVAL,
                    (PHY_LTE_MAX_CQI_REPORTING_INTERVAL - 1));
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: CQI reporting interval should be set."
            "Change CQI reporting interval to %d.",
            PHY_LTE_DEFAULT_CQI_REPORTING_INTERVAL);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->cqiReportingInterval =
            PHY_LTE_DEFAULT_CQI_REPORTING_INTERVAL;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersCqiReportingOffset
// LAYER            :: PHY
// PURPOSE    :: Initialize the cqi reporting offset.
// PARAMETERS       ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersCqiReportingOffset(Node* node,
                                           int phyIndex,
                                           const NodeInput* nodeInput,
                                           PhyDataLte* phyLte,
                                           BOOL* wasFound)
{
    int cqiReportingOffset = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_ENB)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-CQI-REPORTING-OFFSET",
               wasFound,
               &cqiReportingOffset);

    if (*wasFound)
    {
        if (PHY_LTE_MIN_CQI_REPORTING_OFFSET <= cqiReportingOffset &&
            cqiReportingOffset < PHY_LTE_MAX_CQI_REPORTING_OFFSET)
        {
            phyLte->cqiReportingOffset =
                cqiReportingOffset;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: CQI reporting offset "
                    "should be %d to %d.",
                    PHY_LTE_MIN_CQI_REPORTING_OFFSET,
                    (PHY_LTE_MAX_CQI_REPORTING_OFFSET - 1));
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: CQI reporting offset should be set."
            "Change CQI reporting offset to %d.",
            PHY_LTE_DEFAULT_CQI_REPORTING_OFFSET);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->cqiReportingOffset =
            PHY_LTE_DEFAULT_CQI_REPORTING_OFFSET;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersRiReportingInterval
// LAYER            :: PHY
// PURPOSE    :: Initialize the ri reporting interval.
// PARAMETERS       ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersRiReportingInterval(Node* node,
                                            int phyIndex,
                                            const NodeInput* nodeInput,
                                            PhyDataLte* phyLte,
                                            BOOL* wasFound)
{
    int riReportingInterval = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_ENB)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-RI-REPORTING-INTERVAL",
               wasFound,
               &riReportingInterval);
    if (*wasFound)
    {
        if (PHY_LTE_MIN_RI_REPORTING_INTERVAL <= riReportingInterval &&
            riReportingInterval < PHY_LTE_MAX_RI_REPORTING_INTERVAL)
        {
            phyLte->riReportingInterval =
                riReportingInterval;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: Ri reporting interval "
                    "should be %d to %d.",
                    PHY_LTE_MIN_RI_REPORTING_INTERVAL,
                    (PHY_LTE_MAX_RI_REPORTING_INTERVAL - 1));
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Ri reporting interval should be set."
            "Change Ri reporting interval to %d.",
            PHY_LTE_DEFAULT_RI_REPORTING_INTERVAL);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->riReportingInterval =
            PHY_LTE_DEFAULT_RI_REPORTING_INTERVAL;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersRiReportingOffset
// LAYER      :: PHY
// PURPOSE    :: Initialize the ri reporting offset.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersRiReportingOffset(Node* node,
                                          int phyIndex,
                                          const NodeInput* nodeInput,
                                          PhyDataLte* phyLte,
                                          BOOL* wasFound)
{
    int riReportingOffset = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_ENB)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-RI-REPORTING-OFFSET",
               wasFound,
               &riReportingOffset);

    if (*wasFound)
    {
        if (PHY_LTE_MIN_RI_REPORTING_OFFSET <= riReportingOffset &&
            riReportingOffset < PHY_LTE_MAX_RI_REPORTING_OFFSET)
        {
            phyLte->riReportingOffset =
                riReportingOffset;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: Ri reporting offset "
                    "should be %d to %d.",
                    PHY_LTE_MIN_RI_REPORTING_OFFSET,
                    (PHY_LTE_MAX_RI_REPORTING_OFFSET - 1));
            ERROR_Assert(FALSE, errorStr);

        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Ri reporting offset should be set."
            "Change Ri reporting offset to %d.",
            PHY_LTE_DEFAULT_RI_REPORTING_OFFSET);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->riReportingOffset =
            PHY_LTE_DEFAULT_RI_REPORTING_OFFSET;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersNonServingCellMeasurement
// LAYER      :: PHY
// PURPOSE    :: Initialize the non serving cell measurement.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersNonServingCellMeasurement(
                                              Node* node,
                                              int phyIndex,
                                              const NodeInput* nodeInput,
                                              PhyDataLte* phyLte,
                                              BOOL* wasFound)
{
    clocktype nonServingCellMeasurementInterval = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_ENB)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    // parameter of measurement for handover period
    {
        // default
        phyLte->nonServingCellMeasurementForHoPeriod =
            PHY_LTE_DEFAULT_NON_SERVING_CELL_MEASUREMENT_FOR_HO_PERIOD;
    }

    // parameters of measurement of attach
    {
        // check voided parameter
        IO_ReadTime(node->nodeId,
                     node->phyData[phyIndex]->networkAddress,
                     nodeInput,
                     "PHY-LTE-NON-SERVING-CELL-MEASUREMENT-INTERVAL",
                     wasFound,
                     &nonServingCellMeasurementInterval);
        if (*wasFound)
        {
            printf("WARNING: PHY-LTE-NON-SERVING-CELL-MEASUREMENT-INTERVAL"
                " was voided. This was replaced to Measurement Gap.\n");
        }

        clocktype nonServingCellMeasurementPeriod = 0;
        IO_ReadTime(node->nodeId,
                     node->phyData[phyIndex]->networkAddress,
                     nodeInput,
                     "PHY-LTE-NON-SERVING-CELL-MEASUREMENT-PERIOD",
                     wasFound,
                     &nonServingCellMeasurementPeriod);
        if (*wasFound)
        {
            if (0 <= nonServingCellMeasurementPeriod &&
                nonServingCellMeasurementPeriod < CLOCKTYPE_MAX)
            {
                phyLte->nonServingCellMeasurementPeriod =
                    nonServingCellMeasurementPeriod;
            }
            else
            {
                ERROR_Assert(FALSE,
                             "PHY-LTE: Non Serving cell Measurement Period "
                             "should be 0 to CLOCKTYPE_MAX.");
            }
        }
        else
        {
            char warnBuf[MAX_STRING_LENGTH];
            sprintf(warnBuf,
                "Phy-LTE: Non Serving cell Measurement Period should be set."
                "Change Non Serving cell Measurement Period to %"
                TYPES_64BITFMT "d.",
                PHY_LTE_DEFAULT_NON_SERVING_CELL_MEASUREMENT_PERIOD);
            ERROR_ReportWarning(warnBuf);
            // default
            phyLte->nonServingCellMeasurementPeriod =
                PHY_LTE_DEFAULT_NON_SERVING_CELL_MEASUREMENT_PERIOD;
        }
    }
}

// /**
// FUNCTION   :: PhyLteSetParametersCellSelectionMinServingDuration
// LAYER      :: PHY
// PURPOSE    :: Initialize the cell selection min serving duration.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersCellSelectionMinServingDuration(
                                              Node* node,
                                              int phyIndex,
                                              const NodeInput* nodeInput,
                                              PhyDataLte* phyLte,
                                              BOOL* wasFound)
{
    clocktype cellSelectionMinServingDuration = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_ENB)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadTime(node->nodeId,
                 node->phyData[phyIndex]->networkAddress,
                 nodeInput,
                 "PHY-LTE-CELL-SELECTION-MIN-SERVING-DURATION",
                 wasFound,
                 &cellSelectionMinServingDuration);

    if (*wasFound)
    {
        if (0 <= cellSelectionMinServingDuration &&
            cellSelectionMinServingDuration < CLOCKTYPE_MAX)
        {
            phyLte->cellSelectionMinServingDuration =
                cellSelectionMinServingDuration;
        }
        else
        {
            ERROR_Assert(FALSE,
                         "PHY-LTE: CELL Selection min serving duration "
                         "should be 0 to CLOCKTYPE_MAX.");
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: CELL Selection min serving duration should be set."
            "Change CELL Selection min serving duration to %"
            TYPES_64BITFMT "d.",
            PHY_LTE_CELL_SELECTION_MIN_SERVING_DURATION);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->cellSelectionMinServingDuration =
            PHY_LTE_CELL_SELECTION_MIN_SERVING_DURATION;
    }
}

// /**
// FUNCTION   :: PhyLteSetParametersCellSelectionRxLevelMin_dBm
// LAYER      :: PHY
// PURPOSE    :: Initialize the cell selection rx level min(dBm).
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersCellSelectionRxLevelMin_dBm(
    Node* node,
    int phyIndex,
    const NodeInput* nodeInput,
    PhyDataLte* phyLte,
    BOOL* wasFound)
{
    double cellSelectionRxLevelMin_dBm = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_UE)
    {
        // This Parameter is not own station type Parameter.
        return;
    }


    IO_ReadDouble(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-CELL-SELECTION-RX-LEVEL-MIN",
                  wasFound,
                  &cellSelectionRxLevelMin_dBm);

    if (*wasFound)
    {
        if ((LTE_NEGATIVE_INFINITY_POWER_dBm <=
             cellSelectionRxLevelMin_dBm) &&
            (cellSelectionRxLevelMin_dBm <=
             LTE_INFINITY_POWER_dBm))
        {
            phyLte->cellSelectionRxLevelMin_dBm =
                cellSelectionRxLevelMin_dBm;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: Cell selection rx level min "
                    "should be %.2lf to %.2lf",
                    (double)LTE_NEGATIVE_INFINITY_POWER_dBm,
                    (double)LTE_INFINITY_POWER_dBm);
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Cell selection rx level min should be set."
            "Change Cell selection rx level min to %.2lf.",
            (double)PHY_LTE_DEFAULT_CELL_SELECTION_RXLEVEL_MIN_DBM);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->cellSelectionRxLevelMin_dBm =
            PHY_LTE_DEFAULT_CELL_SELECTION_RXLEVEL_MIN_DBM;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersCellSelectionRxLevelMinOff_dBm
// LAYER      :: PHY
// PURPOSE    :: Initialize the cell selection rx level min off (dBm).
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersCellSelectionRxLevelMinOff_dBm(
    Node* node,
    int phyIndex,
    const NodeInput* nodeInput,
    PhyDataLte* phyLte,
    BOOL* wasFound)
{
    double cellSelectionRxLevelMinOff_dBm = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_UE)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadDouble(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-CELL-SELECTION-RX-LEVEL-MIN-OFFSET",
                  wasFound,
                  &cellSelectionRxLevelMinOff_dBm);

    if (*wasFound)
    {
        if ((LTE_NEGATIVE_INFINITY_POWER_dBm <=
             cellSelectionRxLevelMinOff_dBm) &&
            (cellSelectionRxLevelMinOff_dBm <=
             LTE_INFINITY_POWER_dBm))
        {
            phyLte->cellSelectionRxLevelMinOff_dBm =
                cellSelectionRxLevelMinOff_dBm;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: Cell selection rx level min off "
                    "should be %.2lf to %.2lf",
                    (double)LTE_NEGATIVE_INFINITY_POWER_dBm,
                    (double)LTE_INFINITY_POWER_dBm);
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Cell selection rx level min off should be set."
            "Change Cell selection rx level min off to %.2lf.",
            (double)PHY_LTE_DEFAULT_CELL_SELECTION_RXLEVEL_MIN_OFF_DBM);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->cellSelectionRxLevelMinOff_dBm =
            PHY_LTE_DEFAULT_CELL_SELECTION_RXLEVEL_MIN_OFF_DBM;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersCellSelectionQualLevel_dB
// LAYER      :: PHY
// PURPOSE    :: Initialize the cell selection qual level(dB).
// PARAMETERS ::
// + node        : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     ::   void  : NULL
// **/
void PhyLteSetParametersCellSelectionQualLevel_dB(Node* node,
                                                  int phyIndex,
                                                  const NodeInput* nodeInput,
                                                  PhyDataLte* phyLte,
                                                  BOOL* wasFound)
{
    double cellSelectionQualLevelMin_dB = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_UE)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadDouble(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-CELL-SELECTION-QUAL-LEVEL-MIN",
                  wasFound,
                  &cellSelectionQualLevelMin_dB);

    if (*wasFound)
    {
        if ((LTE_NEGATIVE_INFINITY_SINR_dB <=
             cellSelectionQualLevelMin_dB) &&
            (cellSelectionQualLevelMin_dB <=
             LTE_INFINITY_SINR_dB))
        {
            phyLte->cellSelectionQualLevelMin_dB =
                cellSelectionQualLevelMin_dB;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: Cell selection qual level min "
                    "should be %.2lf to %.2lf",
                    (double)LTE_NEGATIVE_INFINITY_SINR_dB,
                    (double)LTE_INFINITY_SINR_dB);
            ERROR_Assert(FALSE, errorStr);

        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Cell selection qual level min should be set."
            "Change Cell selection qual level min to %.2lf.",
            (double)PHY_LTE_DEFAULT_CELL_SELECTION_QUALLEVEL_MIN_DB);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->cellSelectionQualLevelMin_dB =
            PHY_LTE_DEFAULT_CELL_SELECTION_QUALLEVEL_MIN_DB;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersCellSelectionQualLevelOffset_dB
// LAYER      :: PHY
// PURPOSE    :: Initialize the cell selection qual level offset (dB)
// PARAMETERS ::
// + node         : Node*  : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersCellSelectionQualLevelOffset_dB(
    Node* node,
    int phyIndex,
    const NodeInput* nodeInput,
    PhyDataLte* phyLte,
    BOOL* wasFound)
{
    double cellSelectionQualLevelMinOffset_dB = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_UE)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadDouble(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-CELL-SELECTION-QUAL-LEVEL-MIN-OFFSET",
                  wasFound,
                  &cellSelectionQualLevelMinOffset_dB);

    if (*wasFound)
    {
        if ((LTE_NEGATIVE_INFINITY_SINR_dB <=
             cellSelectionQualLevelMinOffset_dB) &&
            (cellSelectionQualLevelMinOffset_dB <=
             LTE_INFINITY_SINR_dB))
        {
            phyLte->cellSelectionQualLevelMinOffset_dB =
                cellSelectionQualLevelMinOffset_dB;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: Cell selection qual level min offset "
                    "should be %.2lf to %.2lf",
                    (double)LTE_NEGATIVE_INFINITY_SINR_dB,
                    (double)LTE_INFINITY_SINR_dB);
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Cell selection qual level min offset should be set."
            "Change Cell selection qual level min offset to %.2lf.",
            (double)PHY_LTE_DEFAULT_CELL_SELECTION_QUALLEVEL_MINOFFSET_DB);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->cellSelectionQualLevelMinOffset_dB =
            (double)PHY_LTE_DEFAULT_CELL_SELECTION_QUALLEVEL_MINOFFSET_DB;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersRaPrachFreqOffset
// LAYER      :: PHY
// PURPOSE    :: Initialize the RA PRACH frequency offset.
// PARAMETERS ::
// + node       : Node*  : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     ::  void   : NULL
// **/
void PhyLteSetParametersRaPrachFreqOffset(Node* node,
                                          int phyIndex,
                                          const NodeInput* nodeInput,
                                          PhyDataLte* phyLte,
                                          BOOL* wasFound)
{
    // ! This parameter is not a configurable parameter for first release !
    phyLte->raPrachFreqOffset =
        (UInt8)PHY_LTE_DEFAULT_RA_PRACH_FREQ_OFFSET;
}

// /**
// FUNCTION   :: PhyLteSetParametersRaDetectionThreshold
// LAYER      :: PHY
// PURPOSE    :: Initialize the RA detection threshold.
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     ::  void    : NULL
// **/
void PhyLteSetParametersRaDetectionThreshold(Node* node,
                                             int phyIndex,
                                             const NodeInput* nodeInput,
                                             PhyDataLte* phyLte,
                                             BOOL* wasFound)
{
    double raDetectionThreshold_dBm = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_UE)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadDouble(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-RA-DETECTION-THRESHOLD",
                  wasFound,
                  &raDetectionThreshold_dBm);

    if (*wasFound)
    {
        if ((LTE_NEGATIVE_INFINITY_POWER_dBm <=
             raDetectionThreshold_dBm) &&
            (raDetectionThreshold_dBm <=
             LTE_INFINITY_POWER_dBm))
        {
            phyLte->raDetectionThreshold_dBm =
                raDetectionThreshold_dBm;
        }
        else
        {
            ERROR_Assert(FALSE,
                         "PHY-LTE: Random access detection threshold "
                         "should be LTE_NEGATIVE_INFINITY_POWER_DB "
                         "to LTE_INFINITY_POWER_DB.");
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Random access detection threshold should be set."
            "Change Random access detection threshold to %.2lf.",
            PHY_LTE_DEFAULT_RA_DETECTION_THRESHOLD_DBM);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->raDetectionThreshold_dBm =
            PHY_LTE_DEFAULT_RA_DETECTION_THRESHOLD_DBM;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersSrsTransmissionInterval
// LAYER      :: PHY
// PURPOSE    :: Initialize the SRS transmission interval.
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     ::  void    : NULL
// **/
void PhyLteSetParametersSrsTransmissionInterval(Node* node,
                                                int phyIndex,
                                                const NodeInput* nodeInput,
                                                PhyDataLte* phyLte,
                                                BOOL* wasFound)
{
    int srsTransmissionInterval = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_ENB)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-SRS-TRANSMISSION-INTERVAL",
               wasFound,
               &srsTransmissionInterval);

    if (*wasFound)
    {
        if ((PHY_LTE_MIN_SRS_TRANSMISSION_INTERVAL
                <= srsTransmissionInterval) &&
            (srsTransmissionInterval
                < PHY_LTE_MAX_SRS_TRANSMISSION_INTERVAL))
        {
            phyLte->srsTransmissionInterval =
                srsTransmissionInterval;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: SRS transmission interval "
                    "should be %d to %d",
                    PHY_LTE_MIN_SRS_TRANSMISSION_INTERVAL,
                    (PHY_LTE_MAX_SRS_TRANSMISSION_INTERVAL - 1));
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: SRS transmission interval should be set."
            "Change SRS transmission interval to %d.",
            PHY_LTE_DEFAULT_SRS_TRANSMISSION_INTERVAL);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->srsTransmissionInterval =
            PHY_LTE_DEFAULT_SRS_TRANSMISSION_INTERVAL;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersSrsTransmissionOffset
// LAYER      :: PHY
// PURPOSE    :: Initialize the SRS transmission offset.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersSrsTransmissionOffset(Node* node,
                                              int phyIndex,
                                              const NodeInput* nodeInput,
                                              PhyDataLte* phyLte,
                                              BOOL* wasFound)
{
    int srsTransmissionOffset = 0;

    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    if (lte->stationType == LTE_STATION_TYPE_ENB)
    {
        // This Parameter is not own station type Parameter.
        return;
    }

    IO_ReadInt(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-SRS-TRANSMISSION-OFFSET",
               wasFound,
               &srsTransmissionOffset);

    if (*wasFound)
    {
        if ((PHY_LTE_MIN_SRS_TRANSMISSION_OFFSET
                <= srsTransmissionOffset)
            && (srsTransmissionOffset
                < PHY_LTE_MAX_SRS_TRANSMISSION_OFFSET))
        {
            phyLte->srsTransmissionOffset =
                srsTransmissionOffset;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                         "PHY-LTE: SRS transmission offset "
                    "should be %d to %d",
                    PHY_LTE_MIN_SRS_TRANSMISSION_OFFSET,
                    (PHY_LTE_MAX_SRS_TRANSMISSION_OFFSET - 1));
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: SRS transmission offset should be set."
            "Change SRS transmission offset to %d.",
            PHY_LTE_DEFAULT_SRS_TRANSMISSION_OFFSET);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->srsTransmissionOffset =
            PHY_LTE_DEFAULT_SRS_TRANSMISSION_OFFSET;
    }
}

// /**
// FUNCTION   :: PhyLteSetParametersRxSensitivity_mW
// LAYER      :: PHY
// PURPOSE    :: Initialize the rx sensitivity (mW)
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersRxSensitivity_mW(Node* node,
                                         int phyIndex,
                                         const NodeInput* nodeInput,
                                         PhyDataLte* phyLte,
                                         BOOL* wasFound)
{
    phyLte->numRxSensitivity = PHY_LTE_MAX_RX_SENSITIVITY_LEN;

    phyLte->rxSensitivity_mW =
        (double*) MEM_malloc((sizeof(double) * phyLte->numRxSensitivity));
    ERROR_Assert(phyLte->rxSensitivity_mW != NULL, "LTE: Out of memory!");
    memset(phyLte->rxSensitivity_mW,
           0,
           (sizeof(double) * phyLte->numRxSensitivity));

    double rxSensitivity_dBm = 0;
    int i;
    for (i = 0;i < phyLte->numRxSensitivity; i++){
        IO_ReadDoubleInstance(node->nodeId,
                              node->phyData[phyIndex]->networkAddress,
                              nodeInput,
                              "PHY-LTE-RX-SENSITIVITY",
                              i,
                              TRUE,
                              wasFound,
                              &rxSensitivity_dBm);

        if (*wasFound)
        {
            if ((LTE_NEGATIVE_INFINITY_POWER_dBm <= rxSensitivity_dBm) &&
                (rxSensitivity_dBm <= LTE_INFINITY_POWER_dBm))
            {
                phyLte->rxSensitivity_mW[i] = NON_DB(rxSensitivity_dBm);
            }
            else
            {
                char errorStr[MAX_STRING_LENGTH];
                sprintf(errorStr,
                        "PHY-LTE: Receive Sensitivity "
                        "should be %.2lf to %.2lf",
                        (double)LTE_NEGATIVE_INFINITY_POWER_dBm,
                        (double)LTE_INFINITY_POWER_dBm);
                ERROR_Assert(FALSE, errorStr);
            }
        }
        else
        {
            char warnBuf[MAX_STRING_LENGTH];
            sprintf(warnBuf,
                "Phy-LTE: Receive Sensitivity should be set."
                "Change Receive Sensitivity to %.2lf.",
                NON_DB(PHY_LTE_DEFAULT_RXSENSITIVITY_DBM));
            ERROR_ReportWarning(warnBuf);
            // default
            phyLte->rxSensitivity_mW[i] =
                NON_DB(PHY_LTE_DEFAULT_RXSENSITIVITY_DBM);
        }
    }
}

// /**
// FUNCTION   :: PhyLteSetParametersDlCqiSnrTable
// LAYER      :: PHY
// PURPOSE    :: Initialize the downlink cqi snr table.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + subnetInterfase   : NodeAddress : Subnet interface.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersDlCqiSnrTable(Node* node,
                                      int phyIndex,
                                      const NodeInput* nodeInput,
                                      NodeAddress subnetInterfase,
                                      PhyDataLte* phyLte,
                                      BOOL* wasFound)
{
    int memSize = sizeof(Float64) * PHY_LTE_DL_CQI_SNR_TABLE_LEN;
    phyLte->dlCqiSnrTable = (Float64*) MEM_malloc(memSize);
    ERROR_Assert(phyLte->dlCqiSnrTable != NULL,
                 "PHYLTE: Out of memory!");
    memset(phyLte->dlCqiSnrTable, 0, memSize);

    double snr = 0.0;
    int i;
    for (i = 0;i < PHY_LTE_DL_CQI_SNR_TABLE_LEN; i++){
        IO_ReadDoubleInstance(
            node->nodeId,
            subnetInterfase,
            nodeInput,
            "PHY-LTE-DL-CQI-SNR-TABLE",
            i,
            TRUE,
            wasFound,
            &snr);

        if (*wasFound)
        {
            if ((LTE_NEGATIVE_INFINITY_POWER_dBm <= snr) &&
                (snr <= LTE_INFINITY_POWER_dBm))
            {
                phyLte->dlCqiSnrTable[i] = snr;
            }
            else
            {
                char errorStr[MAX_STRING_LENGTH];
                sprintf(errorStr,
                        "PHY-LTE: PHY-LTE-DL-CQI-SNR-TABLE[%d]"
                        "should be %.2lf to %.2lf",
                        i,
                        (double)LTE_NEGATIVE_INFINITY_POWER_dBm,
                        (double)LTE_INFINITY_POWER_dBm);
                ERROR_Assert(FALSE, errorStr);
            }
        }
        else
        {
            char warnBuf[MAX_STRING_LENGTH];
            sprintf(warnBuf,
                "Phy-LTE: PHY-LTE-DL-CQI-SNR-TABLE should be set."
                "Change PHY-LTE-DL-CQI-SNR-TABLE[%d] to %.2lf.",
                i, PHY_LTE_DEFAULT_CQI_SNR_TABLE[i]);
            ERROR_ReportWarning(warnBuf);
            // Use default values
            phyLte->dlCqiSnrTable[i] =
                PHY_LTE_DEFAULT_CQI_SNR_TABLE[i];

        }
    }



}


// /**
// FUNCTION   :: PhyLteGetDlCqiSnrTableIndex
// LAYER      :: PHY
// PURPOSE    :: Get the downlink cqi snr table index.
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + sinr      : double  : Sinr.
// RETURN     ::  void    : NULL
// **/
int PhyLteGetDlCqiSnrTableIndex(Node* node,
                                int phyIndex,
                                PhyDataLte* phyLte,
                                double sinr_dB)
{
    int i = PHY_LTE_DL_CQI_SNR_TABLE_LEN - 1;
    for (; i > 0; --i)
    {
        if (sinr_dB >= phyLte->dlCqiSnrTable[i])
        {
            break;
        }
    }

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
                              node->phyData[phyIndex]->macInterfaceIndex,
                              "PHY:DEBUG:PhyLteGetDlCqiSnrTableIndex",
                              "%lf,%d,%lf",
                              sinr_dB,
                              i,
                              phyLte->dlCqiSnrTable[i]);
#endif // LTE_LIB_LOG

    return i;
}



// /**
// FUNCTION   :: PhyLteSetParametersPathlossFilterCoefficient
// LAYER      :: PHY
// PURPOSE    :: Initialize the pathloss filter coefficient.
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     ::  void    : NULL
// **/
void PhyLteSetParametersPathlossFilterCoefficient(Node* node,
                                             int phyIndex,
                                             const NodeInput* nodeInput,
                                             PhyDataLte* phyLte,
                                             BOOL* wasFound)
{
    double pathlossFilterCoefficient = 0;
    IO_ReadDouble(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-PATHLOSS-FILTER-COEFFICIENT",
                  wasFound,
                  &pathlossFilterCoefficient);

    if (*wasFound)
    {
        if (pathlossFilterCoefficient >= 0.0)
        {
            phyLte->pathlossFilterCoefficient =
                pathlossFilterCoefficient;
        }
        else
        {
            ERROR_Assert(FALSE,
                         "PHY-LTE: Pathloss filter coefficient "
                         "should be greater or equal to 0.0");
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Pathloss filter coefficient should be set."
            "Change Pathloss filter coefficient to %.2lf.",
            PHY_LTE_DEFAULT_PATHLOSS_FILTER_COEFFICIENT);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->pathlossFilterCoefficient =
            PHY_LTE_DEFAULT_PATHLOSS_FILTER_COEFFICIENT;
    }

}

// /**
// FUNCTION   :: PhyLteSetParametersInterferenceFilterCoefficient
// LAYER      :: PHY
// PURPOSE    :: Initialize the interference filter coefficient.
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     ::  void    : NULL
// **/
void PhyLteSetParametersInterferenceFilterCoefficient(Node* node,
                                             int phyIndex,
                                             const NodeInput* nodeInput,
                                             PhyDataLte* phyLte,
                                             BOOL* wasFound)
{
#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
    double interferenceFilterCoefficient = 0;

    IO_ReadDouble(node->nodeId,
                  node->phyData[phyIndex]->networkAddress,
                  nodeInput,
                  "PHY-LTE-INTERFERENCE-FILTER-COEFFICIENT",
                  wasFound,
                  &interferenceFilterCoefficient);

    if (*wasFound)
    {
        if (interferenceFilterCoefficient >= 0.0)
        {
            phyLte->interferenceFilterCoefficient =
                    interferenceFilterCoefficient;
        }
        else
        {
            ERROR_Assert(FALSE,
                         "PHY-LTE: Interference filter coefficient "
                         "should be greater or equal to 0.0");
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Interference filter coefficient should be set."
            "Change Interference filter coefficient to %.2lf.",
            PHY_LTE_DEFAULT_INTERFERENCE_FILTER_COEFFICIENT);
        ERROR_ReportWarning(warnBuf);
        // default
        phyLte->interferenceFilterCoefficient =
            PHY_LTE_DEFAULT_INTERFERENCE_FILTER_COEFFICIENT;
    }
#endif
}

// /**
// FUNCTION   :: PhyLteSetParametersCheckingConnectionInterval
// LAYER      :: PHY
// PURPOSE    :: Initializing the Checking connection interval.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + nodeInput : const NodeInput* : Pointer to node input.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + wasFound  : BOOL  : Parameter read flag pointer.
// RETURN     :: void  : NULL
// **/
void PhyLteSetParametersCheckingConnectionInterval(Node* node,
                                                  int phyIndex,
                                                  const NodeInput* nodeInput,
                                                  PhyDataLte* phyLte,
                                                  BOOL* wasFound)
{
    clocktype checkingConnectionInterval = 0;
    IO_ReadTime(node->nodeId,
                node->phyData[phyIndex]->networkAddress,
                nodeInput,
                "PHY-LTE-CHECKING-CONNECTION-INTERVAL",
                wasFound,
                &checkingConnectionInterval);
    if (*wasFound)
    {
        if ((PHY_LTE_MIN_CHECKING_CONNECTION_INTERVAL <=
             checkingConnectionInterval) &&
            (checkingConnectionInterval <
             PHY_LTE_MAX_CHECKING_CONNECTION_INTERVAL))
        {
            phyLte->checkingConnectionInterval =
                checkingConnectionInterval;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "PHY-LTE: Checking connection interval "
                    "should be 0 to CLOCKTYPE_MAX.");
            ERROR_Assert(FALSE, errorStr);
        }
    }
    else
    {
#ifdef LTE_LIB_USE_ONOFF
        // POWER ON/OFF functions is not supported for current release.
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Phy-LTE: Checking Connection Interval should be set."
            "Change Checking Connection Interval to %d.",
            PHY_LTE_DEFAULT_CHECKING_CONNECTION_INTERVAL);
        ERROR_ReportWarning(warnBuf);
#endif
        phyLte->checkingConnectionInterval =
            PHY_LTE_DEFAULT_CHECKING_CONNECTION_INTERVAL;
    }
}

// /**
// FUNCTION   :: PhyLteInitConfigurableParameters
// LAYER      :: PHY
// PURPOSE    :: Initialize configurable parameters
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : int              : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to node input
// RETURN     :: void             : NULL
// **/
void PhyLteInitConfigurableParameters(Node* node,
                                      int phyIndex,
                                      const NodeInput* nodeInput)
{

    PhyDataLte* phyLte =
        (PhyDataLte*)(node->phyData[phyIndex]->phyVar);

    BOOL wasFound = FALSE;
    NodeAddress subnetInterfase;

    subnetInterfase = MAPPING_GetSubnetAddressForInterface(
                                node,
                                node->nodeId,
                                node->phyData[phyIndex]->macInterfaceIndex);


    PhyLteSetParametersTxPower(node,
                               phyIndex,
                               nodeInput,
                               phyLte,
                               &wasFound);

#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(node,
                                  node->phyData[phyIndex]->macInterfaceIndex,
                                  "PHY:DEBUG:PhyLteSetParametersTxPower",
                                  "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersChannelBandwidth(node,
                                        phyIndex,
                                        nodeInput,
                                        subnetInterfase,
                                        phyLte,
                                        &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            "PHY:DEBUG:PhyLteSetParametersChannelBandwidth",
                            "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersDlChannelNo(node,
                                   phyIndex,
                                   nodeInput,
                                   phyLte,
                                   &wasFound);

#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(node,
                                  node->phyData[phyIndex]->macInterfaceIndex,
                                  "PHY:DEBUG:PhyLteSetParametersDlChannelNo",
                                  "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersUlChannelNo(node,
                                   phyIndex,
                                   nodeInput,
                                   phyLte,
                                   &wasFound);

#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(node,
                                  node->phyData[phyIndex]->macInterfaceIndex,
                                  "PHY:DEBUG:PhyLteSetParametersUlChannelNo",
                                  "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersTxAntennas(node,
                                  phyIndex,
                                  nodeInput,
                                  phyLte,
                                  &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(node,
                                  node->phyData[phyIndex]->macInterfaceIndex,
                                  "PHY:DEBUG:PhyLteSetParametersTxAntennas",
                                  "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersRxAntennas(node,
                                  phyIndex,
                                  nodeInput,
                                  phyLte,
                                  &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(node,
                                  node->phyData[phyIndex]->macInterfaceIndex,
                                  "PHY:DEBUG:PhyLteSetParametersRxAntennas",
                                  "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersTpcPoPusch(node,
                                  phyIndex,
                                  nodeInput,
                                  phyLte,
                                  &wasFound);

#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(node,
                                  node->phyData[phyIndex]->macInterfaceIndex,
                                  "PHY:DEBUG:PhyLteSetParametersTpcPoPusch",
                                  "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersTpcAlpha(node,
                                phyIndex,
                                nodeInput,
                                phyLte,
                                &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(node,
                                  node->phyData[phyIndex]->macInterfaceIndex,
                                  "PHY:DEBUG:PhyLteSetParametersTpcAlpha",
                                  "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersCqiReportingInterval(node,
                                            phyIndex,
                                            nodeInput,
                                            phyLte,
                                            &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                        node,
                        node->phyData[phyIndex]->macInterfaceIndex,
                        "PHY:DEBUG:PhyLteSetParametersCqiReportingInterval",
                        "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersCqiReportingOffset(node,
                                          phyIndex,
                                          nodeInput,
                                          phyLte,
                                          &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                        node,
                        node->phyData[phyIndex]->macInterfaceIndex,
                        "PHY:DEBUG:PhyLteSetParametersCqiReportingOffset",
                        "default:");
    }
#endif // LTE_LIB_LOG



    PhyLteSetParametersRiReportingInterval(node,
                                           phyIndex,
                                           nodeInput,
                                           phyLte,
                                           &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                        node,
                        node->phyData[phyIndex]->macInterfaceIndex,
                        "PHY:DEBUG:PhyLteSetParametersRiReportingInterval",
                        "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersRiReportingOffset(node,
                                         phyIndex,
                                         nodeInput,
                                         phyLte,
                                         &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            "PHY:DEBUG:PhyLteSetParametersRiReportingOffset",
                            "default:");
    }
#endif // LTE_LIB_LOG

    PhyLteSetParametersNonServingCellMeasurement(
                                                     node,
                                                     phyIndex,
                                                     nodeInput,
                                                     phyLte,
                                                     &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
              node,
              node->phyData[phyIndex]->macInterfaceIndex,
              "PHY:DEBUG:PhyLteSetParametersNonServingCellMeasurement",
              "default:");
    }
#endif // LTE_LIB_LOG

    PhyLteSetParametersCellSelectionMinServingDuration(
                                                     node,
                                                     phyIndex,
                                                     nodeInput,
                                                     phyLte,
                                                     &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
          node,
          node->phyData[phyIndex]->macInterfaceIndex,
          "PHY:DEBUG:PhyLteSetParametersCellSelectionMinServingDuration",
          "default:");
    }
#endif // LTE_LIB_LOG
    PhyLteSetParametersCellSelectionRxLevelMin_dBm(node,
                                                   phyIndex,
                                                   nodeInput,
                                                   phyLte,
                                                   &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                node,
                node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteSetParametersCellSelectionRxLevelMin_dBm",
                "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersCellSelectionRxLevelMinOff_dBm(node,
                                                      phyIndex,
                                                      nodeInput,
                                                      phyLte,
                                                      &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteSetParametersCellSelectionRxLevelMinOff_dBm",
            "default:");
    }
#endif // LTE_LIB_LOG



    PhyLteSetParametersCellSelectionQualLevel_dB(node,
                                                 phyIndex,
                                                 nodeInput,
                                                 phyLte,
                                                 &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                    node,
                    node->phyData[phyIndex]->macInterfaceIndex,
                    "PHY:DEBUG:PhyLteSetParametersCellSelectionQualLevel_dB",
                    "default:");
    }
#endif // LTE_LIB_LOG



    PhyLteSetParametersCellSelectionQualLevelOffset_dB(node,
                                                       phyIndex,
                                                       nodeInput,
                                                       phyLte,
                                                       &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteSetParametersCellSelectionQualLevelOffset_dB",
            "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersRaDetectionThreshold(node,
                                         phyIndex,
                                         nodeInput,
                                         phyLte,
                                         &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                        node,
                        node->phyData[phyIndex]->macInterfaceIndex,
                        "PHY:DEBUG:PhyLteSetParametersRaDetectionThreshold",
                        "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersSrsTransmissionInterval(node,
                                            phyIndex,
                                            nodeInput,
                                            phyLte,
                                            &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                    node,
                    node->phyData[phyIndex]->macInterfaceIndex,
                    "PHY:DEBUG:PhyLteSetParametersSrsTransmissionInterval",
                    "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersSrsTransmissionOffset(node,
                                               phyIndex,
                                               nodeInput,
                                               phyLte,
                                               &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                        node,
                        node->phyData[phyIndex]->macInterfaceIndex,
                        "PHY:DEBUG:PhyLteSetParametersSrsTransmissionOffset",
                        "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersDlCqiSnrTable(node,
                                             phyIndex,
                                             nodeInput,
                                     subnetInterfase,
                                             phyLte,
                                             &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                                node,
                                node->phyData[phyIndex]->macInterfaceIndex,
                                "PHY:DEBUG:PhyLteSetParametersDlCqiSnrTable",
                                "default:");
    }
#endif // LTE_LIB_LOG

    PhyLteSetParametersRxSensitivity_mW(node,
                                             phyIndex,
                                             nodeInput,
                                             phyLte,
                                             &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            "PHY:DEBUG:PhyLteSetParametersRxSensitivity_mW",
                            "default:");
    }
#endif // LTE_LIB_LOG

    PhyLteInitDefaultParameters(node, phyIndex);


    PhyLteSetParametersPucchOverhead(node,
                                     phyIndex,
                                     nodeInput,
                                     phyLte,
                                     &wasFound);
#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                                node,
                                node->phyData[phyIndex]->macInterfaceIndex,
                                "PHY:DEBUG:PhyLteSetParametersPucchOverhead",
                                "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersRaPrachFreqOffset(node,
                                        phyIndex,
                                        nodeInput,
                                        phyLte,
                                        &wasFound);

#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            "PHY:DEBUG:PhyLteSetParametersRaPrachFreqOffset",
                            "default:");
    }
#endif // LTE_LIB_LOG


    PhyLteSetParametersCheckingConnectionInterval(node,
                                                 phyIndex,
                                                 nodeInput,
                                                 phyLte,
                                                 &wasFound);

#ifdef LTE_LIB_LOG
    if (wasFound == FALSE)
    {
        lte::LteLog::Debug2Format(
                node,
                node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteSetParametersCheckingConnectionInterval",
                "default:");
    }
#endif // LTE_LIB_LOG

    PhyLteCheckParameterBer(node, phyIndex);
#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
                              node->phyData[phyIndex]->macInterfaceIndex,
                              "PHY:DEBUG:PhyLteInitConfigurableParameters",
                              "%lf,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
                              "%lf,%d"/*,%lld*/",%lf,%lf,%lf,%lf,%d,%lf,%d,"
                              "%d,%d,%lf",
                              phyLte->maxTxPower_dBm,
                              phyLte->channelBandwidth,
                              phyLte->dlChannelNo,
                              phyLte->ulChannelNo,
                              phyLte->numTxAntennas,
                              phyLte->numRxAntennas,
                              phyLte->cqiReportingOffset,
                              phyLte->cqiReportingInterval,
                              phyLte->riReportingOffset,
                              phyLte->riReportingInterval,
                              phyLte->tpcPoPusch_dBmPerRb,
                              phyLte->tpcAlpha,
                              phyLte->cellSelectionRxLevelMin_dBm,
                              phyLte->cellSelectionRxLevelMinOff_dBm,
                              phyLte->cellSelectionQualLevelMin_dB,
                              phyLte->cellSelectionQualLevelMinOffset_dB,
                              phyLte->raPrachFreqOffset,
                              phyLte->raDetectionThreshold_dBm,
                              phyLte->srsTransmissionInterval,
                              phyLte->srsTransmissionOffset,
                              phyLte->numRxSensitivity,
                              phyLte->rxSensitivity_mW[0]);
#endif // LTE_LIB_LOG

    PhyLteSetParametersPathlossFilterCoefficient(node,
                                                 phyIndex,
                                                 nodeInput,
                                                 phyLte,
                                                 &wasFound);

    PhyLteSetParametersInterferenceFilterCoefficient(node,
                                                 phyIndex,
                                                 nodeInput,
                                                 phyLte,
                                                 &wasFound);

#ifdef ERROR_DETECTION_DISABLE

    IO_ReadDouble(node->nodeId,
               node->phyData[phyIndex]->networkAddress,
               nodeInput,
               "PHY-LTE-DEBUG-PPER",
               &wasFound,
               & phyLte->debugPPER);

    if (wasFound == FALSE)
    {
        phyLte->debugPPER = 0.0;
    }

#endif

}

// /**
// FUNCTION   :: PhyLteGetTxDataRate
// LAYER      :: PHY
// PURPOSE    :: Get the transmission data rate.
// PARAMETERS ::
// + thisPhy   : PhyData* : Pointer to PHY structure.
// RETURN     :: int      : Transmission data rate in bps.
// **/
int PhyLteGetTxDataRate(PhyData* thisPhy)
{
    // ! Not used for first release !
    int dataRate = 0;

    return dataRate;
}


// /**
// FUNCTION   :: PhyLteSetStationType
// LAYER      :: PHY
// PURPOSE    :: Used by MAC layer to tell PHY whether this station
//               is eNodeB (eNB) or User Equipment (UE)
// PARAMETERS ::
// + node        : Node* : Pointer to node.
// + phyIndex    : int   : Index of the PHY.
// + stationType : char  : Type of the station.
// RETURN     ::  void    : NULL
// **/
void PhyLteSetStationType(Node* node,
                          int phyIndex,
                          LteStationType stationType)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    phyLte->stationType = stationType;
}

// /**
// FUNCTION   :: GetRandomSeed
// LAYER      :: PHY
// PURPOSE    :: Get the array of seed
// PARAMETERS ::
// + hash    : PhyLteRandomNumberSeedType  : hash
// + seed    : RandomSeed&                 : Pointer to seed
// RETURN     :: void : NULL
// **/
void
GetRandomSeed(PhyLteRandomNumberSeedType hash, RandomSeed& seed)
{
    seed[0] = 0;
    seed[1] = static_cast < unsigned short > ((hash >> 16) % 0xffff);
    seed[2] = static_cast < unsigned short > (hash % 0xffff);
}

// /**
// FUNCTION   :: PhyLteRandomizeGaussianComponentStartingPoint
// LAYER      :: PHY
// PURPOSE    :: Create a random starting point.
// PARAMETERS ::
// + globalSeed   : UInt32      : Global Seed.
// + txNodeId     : NodeAddress : Source node address id.
// + rxNodeId     : NodeAddress : Recipient node address id.
// + txAntenna    : UInt8       : Transmitting antenna source id.
// + rxAntenna    : UInt8       : Receiving antenna receiver id.
// + channelIndex : int         : Index of the channel.
// + arraySize    : int         : Number of Gaussian component elements.
// RETURN     :: int          : start point.
// **/
static
int PhyLteRandomizeGaussianComponentStartingPoint(UInt32 globalSeed,
                                                  NodeAddress txNodeId,
                                                  NodeAddress rxNodeId,
                                                  UInt8 txAntenna,
                                                  UInt8 rxAntenna,
                                                  int channelIndex,
                                                  int arraySize)
{
    if (arraySize <= 0)
    {
        ERROR_Assert(FALSE, "PHY-LTE: division by zero.");
    }

    PhyLteRandomNumberSeedType seed =
        (PhyLteRandomNumberSeedType)globalSeed;
    PhyLteRandomNumberSeedType hashInput1;
    PhyLteRandomNumberSeedType hashInput2;
    PhyLteRandomNumberSeedType hashInput3;
    PhyLteRandomNumberSeedType hashInput4;
    PhyLteRandomNumberSeedType hashInput5;
    PhyLteRandomNumberSeedType hashedSeed;


    if (txNodeId > rxNodeId)
    {
        hashInput1 = (PhyLteRandomNumberSeedType)txNodeId;
        hashInput2 = (PhyLteRandomNumberSeedType)rxNodeId;
    }
    else
    {
        hashInput1 = (PhyLteRandomNumberSeedType)rxNodeId;
        hashInput2 = (PhyLteRandomNumberSeedType)txNodeId;
    }

    if (txNodeId > rxNodeId)
    {
        hashInput3 = (PhyLteRandomNumberSeedType)txAntenna;
        hashInput4 = (PhyLteRandomNumberSeedType)rxAntenna;
    }
    else
    {
        hashInput3 = (PhyLteRandomNumberSeedType)rxAntenna;
        hashInput4 = (PhyLteRandomNumberSeedType)txAntenna;
    }

    hashInput5 = (PhyLteRandomNumberSeedType)channelIndex;

    hashedSeed = PhyLteHashInputsToMakeSeed(seed,
                                            hashInput1,
                                            hashInput2,
                                            hashInput3,
                                            hashInput4,
                                            hashInput5);

    RandomSeed randomSeed;
    GetRandomSeed(hashedSeed, randomSeed);
    RandomDistribution < Int32 > randomNum;
    randomNum.init();
    randomNum.setDistributionUniform(0, arraySize - 1);
    Int32 startingPoint = randomNum.getRandomNumber(randomSeed);

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(
                NULL,
                -1,
                "PHY:DEBUG:PhyLteRandomizeGaussianComponentStartingPoint",
                ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                globalSeed,
                txNodeId,
                rxNodeId,
                txAntenna,
                rxAntenna,
                channelIndex,
                arraySize,
                seed,
                hashInput1,
                hashInput2,
                hashInput3,
                hashInput4,
                hashInput5,
                hashedSeed,
                startingPoint);
#endif

    return startingPoint;
}

// /**
// FUNCTION   :: PhyLteCalculateFading
// LAYER      :: PHY
// PURPOSE    :: Calculation of fading in consideration of
//               the number of antennas.
// PARAMETERS ::
// + propTxInfo   : PropTxInfo*  : Information of the transmit signal.
// + node2        : Node*        : Pointer to node
// + txAntennaId  : UInt8        : Transmitting antenna source id.
// + rxAntennaId  : UInt8        : Receiving antenna receiver id.
// + channelIndex : int          : Index of the channel.
// + currentTime  : clocktype    : Current time
// + fading_Comp  : Dcomp*       : Pointer to the fading value
// RETURN     :: void : NULL
// **/
void PhyLteCalculateFading(PropTxInfo* propTxInfo,
                           Node* node2,
                           UInt8 txAntennaId,
                           UInt8 rxAntennaId,
                           int channelIndex,
                           clocktype currentTime,
                           Dcomp* fading_Comp)
{
    PropChannel* propChannel = node2->partitionData->propChannel;
    PropProfile* propProfile = propChannel[channelIndex].profile;
    PropProfile* propProfile0 = propChannel[0].profile;

    if (propProfile->fadingModel == RICEAN)
    {
        int arrayIndex;
        double arrayIndexInDouble;

        const float kFactor = (float)propProfile->kFactor;
        const int numGaussianComponents =
                                        propProfile0->numGaussianComponents;
        const int startingPoint =
            PhyLteRandomizeGaussianComponentStartingPoint(
                                            propTxInfo->txNode->globalSeed,
                                            propTxInfo->txNodeId,
                                            node2->nodeId,
                                            txAntennaId,
                                            rxAntennaId,
                                            channelIndex,
                                            numGaussianComponents);

        if (propProfile->motionEffectsEnabled)
        {
            PROP_MotionObtainfadingStretchingFactor(propTxInfo,
                                                    node2,
                                                    channelIndex);
        }

        arrayIndexInDouble =
            node2->propData[channelIndex].fadingStretchingFactor *
            (double)currentTime;

        arrayIndexInDouble -=
            (double)numGaussianComponents *
            floor(arrayIndexInDouble / (double)numGaussianComponents);

        arrayIndex =
            (RoundToInt(arrayIndexInDouble) + startingPoint) %
            numGaussianComponents;

        double real = (propProfile0->gaussianComponent1[arrayIndex]
                       + sqrt(2.0 * kFactor))
                      / sqrt(2.0 * (kFactor + 1));
        double imag = propProfile0->gaussianComponent2[arrayIndex]
                      / sqrt(2.0 * (kFactor + 1));
        *fading_Comp = Dcomp(real, imag);

#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(propTxInfo->txNode,
                                  -1,
                                  "PHY:DEBUG:PhyLteCalculateFading",
                                  "%f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld,%f,%f",
                                  kFactor,
                                  propTxInfo->txNode->globalSeed,
                                  propTxInfo->txNodeId,
                                  node2->nodeId,
                                  txAntennaId,
                                  rxAntennaId,
                                  channelIndex,
                                  numGaussianComponents,
                                  startingPoint,
                                  currentTime,
                                  fading_Comp->real(),
                                  fading_Comp->imag());
#endif

    }
    else
    {
        *fading_Comp = Dcomp(1.0, 0.0);
    }

}

// /**
// FUNCTION   :: PhyLteGetNumTxAntennas
// LAYER      :: PHY
// PURPOSE    :: Get the number of transmit antennas.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : Number of antennas
// **/
UInt8 PhyLteGetNumTxAntennas(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    return phyLte->numTxAntennas;
}

// /**
// FUNCTION   :: PhyLteGetNumRxAntennas
// LAYER      :: PHY
// PURPOSE    :: Get the number of receiver antennas.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : Number of antennas
// **/
UInt8 PhyLteGetNumRxAntennas(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    return phyLte->numRxAntennas;
}

// /**
// FUNCTION   :: PhyLteGetMaxTxPower_dBm
// LAYER      :: PHY
// PURPOSE    :: Get the maximum transmit power.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: double : Maximum transmit power(dBm)
// **/
double PhyLteGetMaxTxPower_dBm(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    return phyLte->maxTxPower_dBm;
}

// /**
// FUNCTION   :: PhyLteGetUlRxIfPower
// LAYER      :: PHY
// PURPOSE    :: Get the interference power received.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// + interferencePower_mW
//            : LteExponentialMean**
//            : Pointer to the buffer of the pointer to the filtered value
// + len      : int   : The length of the array of interference power.
// RETURN     :: void : NULL
// **/
void PhyLteGetUlRxIfPower(Node* node,
                          int phyIndex,
                          double** interferencePower_mW,
                          int* len)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    *interferencePower_mW = phyLte->interferencePower_mW;

    *len = (int)phyLte->numResourceBlocks;
}

// /**
// FUNCTION   :: PhyLteGetUlFilteredRxIfPower
// LAYER      :: PHY
// PURPOSE    :: Get the filtered interference power received.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// + filteredInterferencePower_dBm
//            : LteExponentialMean**
//            : Pointer to the buffer of the pointer to the filtered value
// + len      : int   : The length of the array of interference power.
// RETURN     :: void : NULL
// **/
#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
void PhyLteGetUlFilteredRxIfPower(Node* node,
                          int phyIndex,
                          LteExponentialMean** filteredInterferencePower_dBm,
                          int* len)
{

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    *filteredInterferencePower_dBm = phyLte->filteredInterferencePower;
    *len = (int)phyLte->numResourceBlocks;
}
#endif

// /**
// FUNCTION   :: PhyLteGetNumResourceBlocks
// LAYER      :: PHY
// PURPOSE    :: Get the number of resource blocks.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : number of resource blocks.
// **/
UInt8 PhyLteGetNumResourceBlocks(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    return (UInt8)phyLte->numResourceBlocks;
}

// /**
// FUNCTION   :: PhyLteGetUlCtrlChannelOverhead
// LAYER      :: PHY
// PURPOSE    :: Get the uplink control channel overhead.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : PucchOverhead.
// **/
UInt32 PhyLteGetUlCtrlChannelOverhead(Node* node,
                                      int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        return phyLte->numPucchOverhead;
    }
    else
    {
        LtePhyConfig* config = GetLtePhyConfig(node, interfaceIndex);
        return config->numPucchOverhead;
    }

}

// /**
// FUNCTION   :: PhyLteGetDlChannelFrequency
// LAYER      :: PHY
// PURPOSE    :: Get the downlink channel frequency.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: double : frequency.
// **/
double PhyLteGetDlChannelFrequency(Node* node,
                                   int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    return node->propChannel[phyLte->dlChannelNo].profile->frequency;
}

// /**
// FUNCTION   :: PhyLteGetUlChannelFrequency
// LAYER      :: PHY
// PURPOSE    :: Get the uplink channel frequency.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: double : frequency.
// **/
double PhyLteGetUlChannelFrequency(Node* node,
                                   int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    return node->propChannel[phyLte->ulChannelNo].profile->frequency;
}

// /**
// FUNCTION   :: PhyLteGetNumSubcarriersPerRb
// LAYER      :: PHY
// PURPOSE    :: Get the number of subcarriers.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : number of subcarriers.
// **/
UInt8 PhyLteGetNumSubcarriersPerRb(Node* node,
                                   int phyIndex)
{
    return PHY_LTE_NUM_SUBCARRIER_PER_RB;
}

// /**
// FUNCTION   :: PhyLteGetNumSymbolsPerRb
// LAYER      :: PHY
// PURPOSE    :: Get the number of symbols.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : number of symbols.
// **/
UInt8 PhyLteGetSymbolsPerRb(Node* node,
                            int phyIndex)
{
    return PHY_LTE_NUM_OFDM_SYMBOLS;
}

// /**
// FUNCTION   :: PhyLteGetSubcarrierSpacing
// LAYER      :: PHY
// PURPOSE    :: Get the subcarrier spacing.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: int : subcarrier spacing.
// **/
int PhyLteGetSubcarrierSpacing(Node* node,
                               int phyIndex)
{
    return PHY_LTE_NUM_SUBCARRIER_SPACING_HZ;
}

// /**
// FUNCTION   :: PhyLteGetRsResourceElementsInRb
// LAYER      :: PHY
// PURPOSE    :: Get the RS OFDM symbol in RB.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: int : RS OFDM symbol in RB.
// **/
int PhyLteGetRsResourceElementsInRb(Node* node,
                    int phyIndex)
{
    return PHY_LTE_NUM_RS_RESOURCE_ELEMENTS_IN_RB;
}

// /**
// FUNCTION   :: PhyLteGetDlMcsTable
// LAYER      :: PHY
// PURPOSE    :: Get the downlink mcs table.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// + len : int*       : Pointer to The length of the mcs table.
// RETURN     :: PhyLtePdschTableColumn :
//               Pointer to downlink mcs table structure.
// **/
const PhyLtePdschTableColumn* PhyLteGetDlMcsTable(Node* node,
                                                  int phyIndex,
                                                  int* len)
{
    *len = PHY_LTE_MCS_INDEX_LEN;
    return &PHY_LTE_PDSCH_TABLE[0];
}

// /**
// FUNCTION   :: PhyLteGetUlMcsTable
// LAYER      :: PHY
// PURPOSE    :: Get the uplink mcs table.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// + len : int*       : Pointer to The length of the mcs table.
// RETURN     :: PhyLtePuschTableColumn :
//               Pointer to uplink mcs table structure.
// **/
const PhyLtePuschTableColumn* PhyLteGetUlMcsTable(Node* node,
                                 int phyIndex,
                                 int* len)
{
    *len = PHY_LTE_MCS_INDEX_LEN;
    return &PHY_LTE_PUSCH_TABLE[0];
}

// /**
// FUNCTION   :: PhyLteGetPrecodingMatrixList
// LAYER      :: PHY
// PURPOSE    :: Get the precoding matrix list.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: Cmat : precoding matrix list.
// **/
Cmat PhyLteGetPrecodingMatrixList(Node* node, int phyIndex)
{
    // Fixed precoding matrix is supported for first release.
    Dcomp p_elm(1/(sqrt(2.0)), 0);
    Cmat matP(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
    matP = GetDiagMatrix(p_elm, p_elm);

    return matP;
}

// /**
// FUNCTION   :: PhyLteGetRbGroupsSize
// LAYER      :: PHY
// PURPOSE    :: Get the RB groups size.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// + numRb    : int   : The number of resource blocks.
// RETURN     :: UInt8 : RB groups size.
// **/
UInt8 PhyLteGetRbGroupsSize(Node* node, int phyIndex, int numRb)
{
    UInt8 ret = 0;
    if (numRb < 0)
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
            "PHY-LTE: PhyLteGetRbGroupsSize numRb:%d error ",
            numRb);
        ERROR_Assert(FALSE, errorStr);
    }
    else if (numRb <= 10)
    {
        ret = PHY_LTE_SIZE_OF_RESOURCE_BLOCK_GROUP_TABLE[0];
    }
    else if (numRb <= 26)
    {
        ret = PHY_LTE_SIZE_OF_RESOURCE_BLOCK_GROUP_TABLE[1];
    }
    else if (numRb <= 63)
    {
        ret = PHY_LTE_SIZE_OF_RESOURCE_BLOCK_GROUP_TABLE[2];
    }
    else if (numRb <= 110)
    {
        ret = PHY_LTE_SIZE_OF_RESOURCE_BLOCK_GROUP_TABLE[3];
    }
    else
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
            "PHY-LTE: PhyLteGetRbGroupsSize numRb:%d error ",
            numRb);
        ERROR_Assert(FALSE, errorStr);
    }

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteGetRbGroupsSize"
        ,"%u,%u", numRb, ret);
#endif // LTE_LIB_LOG

    return ret;
}

// /**
// FUNCTION   :: PhyLteGetDlTxBlockSize
// LAYER      :: PHY
// PURPOSE    :: Get the downlink transmission block size.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// + mcsIndex : int   : Index of the MCS.
// + numRb    : int   : The number of resource blocks.
// RETURN     :: int : downlink transmission block size.
// **/
int PhyLteGetDlTxBlockSize(Node* node,
                           int phyIndex,
                           int mcsIndex,
                           int numRbs)
{
    int tbs_bit = 0;
    int tbsIndex = 0;

    if ((mcsIndex < 0) || (PHY_LTE_MAX_MCS_INDEX_LEN < mcsIndex))
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
            "PHY-LTE: PhyLteGetDlTxBlockSize mcsIndex:%d error ",
            mcsIndex);
        ERROR_Assert(FALSE, errorStr);
    }

    if ((numRbs < 1) ||
        (LTE_TRANSMISSION_BANDWIDTH_CONFIGURATION_NRB_LEN < numRbs))
{
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
            "PHY-LTE: PhyLteGetDlTxBlockSize numRbs:%d error ",
            numRbs);
        ERROR_Assert(FALSE, errorStr);
}

#if 0
    // If 1 transport block mapped to 2 layers, use
    // spatilmulplxTransportBlockSize table
    // Not supported so far
    if (SpatialMulplx)
    {
        tbsIndex =
            PHY_LTE_PDSCH_TABLE[mcsIndex].tbsIndex;
        tbs_bit = spatialmulplxTransportBlockSize[tbsIndex][numRbs - 1];
    }
    else
#endif
    {
        tbsIndex =
            PHY_LTE_PDSCH_TABLE[mcsIndex].tbsIndex;
        tbs_bit = transportBlockSize[tbsIndex][numRbs - 1];
    }

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteGetDlTxBlockSize:"
        ,"%d,%d,%d,%d",
        mcsIndex, numRbs, tbsIndex, tbs_bit);
#endif // LTE_LIB_LOG

    return tbs_bit;
}


// /**
// FUNCTION   :: PhyLteGetUlTxBlockSize
// LAYER      :: PHY
// PURPOSE    :: Get the uplink transmission block size.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// + mcsIndex : int   : Index of the MCS.
// + numRb    : int   : The number of resource blocks.
// RETURN     :: int : uplink transmission block size.
// **/
int PhyLteGetUlTxBlockSize(Node* node,
                           int phyIndex,
                           int mcsIndex,
                           int numRbs)
{
    int tbs_bit = 0;
    int tbsIndex = 0;

    if ((mcsIndex < 0) || (PHY_LTE_MAX_MCS_INDEX_LEN < mcsIndex))
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
            "PHY-LTE: PhyLteGetUlTxBlockSize mcsIndex:%d error ",
            mcsIndex);
        ERROR_Assert(FALSE, errorStr);
    }

    if ((numRbs < 1) ||
        (LTE_TRANSMISSION_BANDWIDTH_CONFIGURATION_NRB_LEN < numRbs))
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
            "PHY-LTE: PhyLteGetUlTxBlockSize numRbs:%d error ",
            numRbs);
        ERROR_Assert(FALSE, errorStr);
    }

    tbsIndex =
        PHY_LTE_PUSCH_TABLE[mcsIndex].tbsIndex;
    tbs_bit = transportBlockSize[tbsIndex][numRbs - 1];


#ifdef LTE_LIB_LOG

    lte::LteLog::Debug2Format(node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteGetUlTxBlockSize:"
        ,"%d,%d,%d,%d", mcsIndex, numRbs, tbsIndex, tbs_bit);
#endif // LTE_LIB_LOG

    return tbs_bit;
}

// /**
// FUNCTION   :: PhyLteGetBerTable
// LAYER      :: PHY
// PURPOSE    :: Get the ber table.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// RETURN     :: PhyBerTable* : Pointer to ber table.
// **/
const PhyBerTable* PhyLteGetBerTable(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    return thisPhy->snrBerTables;
}

// /**
// FUNCTION   :: PhyLteGetReceptionModel
// LAYER      :: PHY
// PURPOSE    :: Get the reception model.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// RETURN     :: PhyRxModel : reception model.
// **/
PhyRxModel PhyLteGetReceptionModel(Node* node,
                                   int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    return thisPhy->phyRxModel;
}

// /**
// FUNCTION   :: PhyLteGetCqiInfoFedbackFromUe
// LAYER      :: PHY
// PURPOSE    :: Get the cqi info.
// PARAMETERS ::
// + node     : Node*   : Pointer to node.
// + phyIndex : int     : Index of the PHY.
// + target   : LteRnti : target node rnti.
// RETURN     :: PhyLteCqiReportInfo* : cqi report info.
// **/
bool PhyLteGetCqiInfoFedbackFromUe(Node* node,
                                    int phyIndex,
                                    LteRnti target,
                                    PhyLteCqiReportInfo* getValue)
{
    bool ret = false;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator it;

    it = phyLte->ueInfoList->find(target);
    if (it != phyLte->ueInfoList->end())
    {
        ret = it->second.cqiReceiver.getCqiReportInfo(*getValue);
    }

    return ret;
}

// /**
// FUNCTION   :: PhyLteGetCqiSnrTable
// LAYER      :: PHY
// PURPOSE    :: Get the cqi snr table.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + cqiTable : Float64** : Pointer to the cqi table.
// + len      : int*      : Pointer to The length of the cqi table.
// RETURN     :: PhyLteCqiReportInfo* : cqi report info.
// **/
void PhyLteGetCqiSnrTable(Node* node,
                          int phyIndex,
                          Float64** cqiTable,
                          int* len)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    *cqiTable = phyLte->dlCqiSnrTable;
    *len = PHY_LTE_DL_CQI_SNR_TABLE_LEN;
    return;
}

// /**
// FUNCTION   :: PhyLteGetCqiTable
// LAYER      :: PHY
// PURPOSE    :: Get the cqi table.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + len      : int*      : Pointer to The length of the cqi table.
// RETURN     :: PhyLteCqiTableColumn* : Pointer to the cqi table structure.
// **/
const PhyLteCqiTableColumn* PhyLteGetCqiTable(Node* node,
                                              int phyIndex,
                                              int* len)
{
    *len = PHY_LTE_CQI_INDEX_LEN;
    return &PHY_LTE_CQI_TABLE[0];
}

// /**
// FUNCTION   :: PhyLteGetPropagationDelay
// LAYER      :: PHY
// PURPOSE    :: Get the propagation delay.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// RETURN     :: clocktype : propagation delay.
// **/
clocktype PhyLteGetPropagationDelay(Node* node,
                                    int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    return phyLte->propagationDelay;
}

// /**
// FUNCTION   :: PhyLteGetUlFilteredPathloss
// LAYER      :: PHY
// PURPOSE    :: Get the uplink pathloss.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + target   : LteRnti : terget node rnti.
// + pathloss : std::vector<double>* : pathloss.
// RETURN     :: clocktype : propagation delay.
// **/
BOOL PhyLteGetUlFilteredPathloss(Node* node,
                            int phyIndex,
                            LteRnti target,
                            std::vector < double >* pathloss)
{
    BOOL ret = FALSE;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    pathloss->clear();
    double pathloss_tmp = 0.0;

    LteLayer3Filtering* lteFiltering
        = LteLayer2GetLayer3Filtering(node,
            node->phyData[phyIndex]->macInterfaceIndex);
    int i;
    for (i = 0; i < phyLte->numRxAntennas; i++)
    {
        if (i == 0)
        {
            ret = lteFiltering->get(
                            target, LTE_LIB_UL_PATHLOSS_RX0, &pathloss_tmp);
        }
        else if (i == 1)
        {
            ret = lteFiltering->get(
                            target, LTE_LIB_UL_PATHLOSS_RX1, &pathloss_tmp);
        }
        pathloss->push_back(pathloss_tmp);
    }

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteGetUlFilteredPathloss:"
        ,"%d,%d,%d",
        target.interfaceIndex,
        target.nodeId,
        ret);
#endif // LTE_LIB_LOG

    return ret;
}


// /**
// FUNCTION   :: PhyLteGetUlInstantPathloss
// LAYER      :: PHY
// PURPOSE    :: Get the uplink pathloss.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + target   : LteRnti : target node rnti.
// + pathloss : std::vector<double>* : pathloss.
// RETURN     :: clocktype : propagation delay.
// **/
BOOL PhyLteGetUlInstantPathloss(Node* node,
                            int phyIndex,
                            LteRnti target,
                            std::vector < double >* pathloss_dB)
{
    BOOL ret = FALSE;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    pathloss_dB->clear();

    std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator it;

    it = phyLte->ueInfoList->find(target);
    if (it != phyLte->ueInfoList->end())
    {
        pathloss_dB->assign(it->second.ulInstantPathloss_dB.begin(),
            it->second.ulInstantPathloss_dB.end());

        ret = TRUE;
    }

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteGetUlInstantPathloss:"
        ,"%d,%d,%d",
        target.interfaceIndex,
        target.nodeId,
        ret);
#endif // LTE_LIB_LOG

    return ret;
}

// /**
// FUNCTION   :: PhyLteGetUlMaxTxPower_dBm
// LAYER      :: PHY
// PURPOSE    :: Get the uplink Maximum transmit power(dBm).
// PARAMETERS ::
// + node     : Node*   : Pointer to node.
// + phyIndex : int     : Index of the PHY.
// + target   : LteRnti : target node rnti.
// + maxTxPower_dBm : double* : Pointer to Maximum transmit power.
// RETURN     :: BOOL : TRUE target found & get.
//                      FALSE target not found.
// **/
BOOL PhyLteGetUlMaxTxPower_dBm(Node* node,
                               int phyIndex,
                               LteRnti target,
                               double* maxTxPower_dBm)
{
    BOOL ret = FALSE;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator it;

    it = phyLte->ueInfoList->find(target);
    if (it != phyLte->ueInfoList->end())
    {
        *maxTxPower_dBm = it->second.maxTxPower_dBm;
        ret = TRUE;
    }

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteGetUlMaxTxPower_dBm:"
        ,"%d,%d,%d,%lf",
        target.interfaceIndex,
        target.nodeId,
        ret,
        maxTxPower_dBm);
#endif // LTE_LIB_LOG
    return ret;
}

// /**
// FUNCTION   :: PhyLteSetUlMaxTxPower_dBm
// LAYER      :: PHY
// PURPOSE    :: Set the uplink Maximum transmit power(dBm).
// PARAMETERS ::
// + node     : Node*   : Pointer to node.
// + phyIndex : int     : Index of the PHY.
// + target   : LteRnti : target node rnti.
// + maxTxPower_dBm : double : Maximum transmit power.
// RETURN     :: BOOL : TRUE target found & set.
//                      FALSE target not found.
// **/
BOOL PhyLteSetUlMaxTxPower_dBm(Node* node,
                               int phyIndex,
                               LteRnti target,
                               double maxTxPower_dBm)
{
    BOOL ret = FALSE;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator it;

    it = phyLte->ueInfoList->find(target);
    if (it != phyLte->ueInfoList->end())
    {
        it->second.maxTxPower_dBm = maxTxPower_dBm;
        ret = TRUE;
    }
#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteSetUlMaxTxPower_dBm:"
        ,"%d,%d,%d,%lf",
        target.interfaceIndex,
        target.nodeId,
        ret,
        maxTxPower_dBm);
#endif // LTE_LIB_LOG

    return ret;
}

// /**
// FUNCTION   :: PhyLteSetRrcConfig
// LAYER      :: PHY
// PURPOSE    :: Set the RRC config.(eNB side)
// PARAMETERS ::
// + node     : Node*   : Pointer to node.
// + phyIndex : int     : Index of the PHY.
// RETURN     :: void   : NULL
// **/
void PhyLteSetRrcConfig(Node* node,
                         int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    LteRrcConfig* config = NULL;
    config = GetLteRrcConfig(node, interfaceIndex);

    config->ltePhyConfig.dlChannelNo = phyLte->dlChannelNo;
    config->ltePhyConfig.ulChannelNo = phyLte->ulChannelNo;
    config->ltePhyConfig.maxTxPower_dBm = phyLte->maxTxPower_dBm;
    config->ltePhyConfig.numTxAntennas = phyLte->numTxAntennas;
    config->ltePhyConfig.numRxAntennas = phyLte->numRxAntennas;
    config->ltePhyConfig.numPucchOverhead = phyLte->numPucchOverhead;
    config->ltePhyConfig.tpcPoPusch_mWPerRb = phyLte->tpcPoPusch_dBmPerRb;
    config->ltePhyConfig.tpcAlpha = phyLte->tpcAlpha;
#if 0
    // Not supported for first release
    config->ltePhyConfig.cqiReportingInterval = phyLte->cqiReportingInterval;
    config->ltePhyConfig.cqiReportingOffset = phyLte->cqiReportingOffset;
    config->ltePhyConfig.riReportingInterval = phyLte->riReportingInterval;
    config->ltePhyConfig.riReportingOffset = phyLte->riReportingOffset;
#endif
    config->ltePhyConfig.cellSelectionRxLevelMin_dBm =
                                phyLte->cellSelectionRxLevelMin_dBm;
    config->ltePhyConfig.cellSelectionRxLevelMinOff_dBm =
                                phyLte->cellSelectionRxLevelMinOff_dBm;
    config->ltePhyConfig.cellSelectionQualLevelMin_dB =
                                phyLte->cellSelectionQualLevelMin_dB;
    config->ltePhyConfig.cellSelectionQualLevelMinOffset_dB =
                                phyLte->cellSelectionQualLevelMinOffset_dB;
    config->ltePhyConfig.raPrachFreqOffset = phyLte->raPrachFreqOffset;
#if 0
    // Not supported for first release
    config->ltePhyConfig.srsTransmissionInterval =
                                phyLte->srsTransmissionInterval;
    config->ltePhyConfig.srsTransmissionOffset =
                                phyLte->srsTransmissionOffset;
    config->ltePhyConfig.nonServingCellMeasurementInterval =
                                phyLte->nonServingCellMeasurementInterval;
    config->ltePhyConfig.nonServingCellMeasurementPeriod =
                                phyLte->nonServingCellMeasurementPeriod;
    config->ltePhyConfig.cellSelectionMinServingDuration =
                                phyLte->cellSelectionMinServingDuration;
#endif
    config->ltePhyConfig.rxSensitivity_dBm =
                                NON_DB(phyLte->rxSensitivity_mW[0]);
}



// /**
// FUNCTION   :: PhyLteSetRrcMeasReportInfo
// LAYER      :: PHY
// PURPOSE    :: Append infomation of MeasReport to the message,
//               when one or more measurement reports exist.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + msg          : Message      : pointer to message
// RETURN     ::  BOOL    : when true, successful of set data.
// **/
BOOL PhyLteSetRrcMeasReportInfo(Node* node, int phyIndex, Message* msg)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte *)thisPhy->phyVar;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    // there are measurement report to send
    if (phyLte->rrcMeasurementReportList->size() > 0)
    {
        // get entire size
        int infoSize = 0;
        for (std::list<MeasurementReport>::iterator it =
            phyLte->rrcMeasurementReportList->begin();
            it != phyLte->rrcMeasurementReportList->end(); it++)
        {
            infoSize += it->measResults.Size();
        }

        // allocate info
        MeasReportContainer* container = 
            (MeasReportContainer*)MESSAGE_AppendInfo(node,
                       msg,
                       sizeof(MeasReportContainer) + infoSize,
                       (unsigned short)INFO_TYPE_LtePhyRrcMeasReport);

        // set each field
        container->dstEbnRnti =
            Layer3LteGetConnectedEnb(node, interfaceIndex);
        container->srcUeRnti.nodeId = node->nodeId;
        container->srcUeRnti.interfaceIndex = interfaceIndex;
        container->reportNum = phyLte->rrcMeasurementReportList->size();
        // set to info
        char* store = container->data;
        for (std::list<MeasurementReport>::iterator it =
            phyLte->rrcMeasurementReportList->begin();
            it != phyLte->rrcMeasurementReportList->end(); it++)
        {
            it->measResults.Serialize(store);
            store += it->measResults.Size();
        }
        // clear
        phyLte->rrcMeasurementReportList->clear();

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: PhyLteGetMessageMeasurementReportInfo
// LAYER      :: PHY
// PURPOSE    :: Get the measurement report info.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + msg      : Message*  : Pointer to message.
// + srcRnti  : LteRnti*  : Poionter to RNTI of Node who sent reports.
// + measurementReport : std::list<MeasurementReport>* :
//                   Pointer to list of measurement report
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageMeasurementReportInfo(
                             Node* node,
                             int phyIndex,
                             Message* msg,
                             LteRnti* srcRnti,
                             std::list<MeasurementReport>* measurementReport)
{
    MeasReportContainer* container =
        (MeasReportContainer*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LtePhyRrcMeasReport);

    if (container != NULL)
    {
        // retrieve eNB's RNTI and copy to return buffer
        LteRnti dstRnti = container->dstEbnRnti;
        if (dstRnti != LteRnti(node->nodeId,
            LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex)))
        {
            // not to me
            return FALSE;
        }

        // retrieve UE's RNTI and copy to return buffer
        *srcRnti = container->srcUeRnti;

        // get num of report
        int numOfReports = container->reportNum;

        char* reports = container->data;
        for (int i = 0; i < numOfReports; i++)
        {
            // deserialize
            MeasResults newResults(reports);
            MeasurementReport newReport(newResults);
            // store
            measurementReport->push_back(newReport);
            // next pointer
            reports += newResults.Size();
        }

        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteSetRrcConnReconfInfo
// LAYER      :: PHY
// PURPOSE    :: Append infomation of RrcConnReconf to the message,
//               when one or more measurement reports exist.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + msg          : Message      : pointer to message
// RETURN     ::  BOOL    : when true, successful of set data.
// **/
BOOL PhyLteSetRrcConnReconfInfo(Node* node, int phyIndex, Message* msg)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte *)thisPhy->phyVar;
    RrcConnReconfInclMoblityControlInfomationMap* rrcConnReconfMap
        = phyLte->rrcConnReconfMap;

    int reconfNum = rrcConnReconfMap->size();
    if (reconfNum > 0)
    {
        // allocate info
        RrcConnReconfInclMoblityControlInfomationContainer* container = 
            (RrcConnReconfInclMoblityControlInfomationContainer*)
            MESSAGE_AppendInfo(
                node,
                msg,
                sizeof(RrcConnReconfInclMoblityControlInfomationContainer)
                    + reconfNum
                    * sizeof(RrcConnReconfInclMoblityControlInfomation),
                (unsigned short)INFO_TYPE_LtePhyRrcConnReconf);

        // copy to msg
        int counter = 0;
        container->num = reconfNum;
        for (RrcConnReconfInclMoblityControlInfomationMap::iterator it =
            rrcConnReconfMap->begin(); it != rrcConnReconfMap->end(); ++it)
        {
            container->reconfList[counter++] = it->second;
        }

        // clear the flag
        rrcConnReconfMap->clear();

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: PhyLteGetMessageRrcConnReconf
// LAYER      :: PHY
// PURPOSE    :: Get the RrcConnReconf info.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + msg      : Message*  : Pointer to message.
// + reconf  : RrcConnReconfInclMoblityControlInfomation*  : received value
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageRrcConnReconf(
    Node* node,
    int phyIndex,
    Message* msg,
    RrcConnReconfInclMoblityControlInfomation* reconf)
{
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);
    RrcConnReconfInclMoblityControlInfomationContainer* container =
        (RrcConnReconfInclMoblityControlInfomationContainer*)
            MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LtePhyRrcConnReconf);

    if (container != NULL)
    {
        ERROR_Assert(container->num > 0, "invalid rrcConnReconf received");

        for (int i = 0; i < container->num; ++i)
        {
            // retrieve dest
            LteRnti dstRnti = container->reconfList[i].hoParticipator.ueRnti;
            if (dstRnti == LteRnti(node->nodeId, interfaceIndex))
            {
                // this is a message to me
                // copy to return buffer
                *reconf = container->reconfList[i];
                return TRUE;
            }
        }
    }

    return FALSE;
}



// /**
// FUNCTION   :: PhyLteSetDciForUl
// LAYER      :: PHY
// PURPOSE    :: Set the DCI for UE.
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex   : int     : Index of the PHY.
// + propRxInfo: PropRxInfo*        : Information of the arrived signal
// + dciForUE   : LteDciFormat0* : Pointer to DCI for UE.
// RETURN     :: void   : NULL
// **/
void PhyLteSetDciForUl(Node* node,
                          int phyIndex,
                          PropRxInfo* propRxInfo,
                          LteDciFormat0* dciForUl)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    UInt64 currentTtiNum = MacLteGetTtiNumber(
                                        node, thisPhy->macInterfaceIndex);

    PhyLteRxDciForUlInfo rxDciForUlInfo;
    rxDciForUlInfo.dciFormat0 = *dciForUl;
    rxDciForUlInfo.rxTime = propRxInfo->rxStartTime;
    rxDciForUlInfo.rxTtiNumber = currentTtiNum; // TtiNumber must be updated
                                               // before this function called

    // Not store multiple DCIs for phase 1
    phyLte->rxDciForUlInfo->push_back(rxDciForUlInfo);


#ifdef LTE_LIB_LOG
    std::stringstream ss;
    std::list < PhyLteRxDciForUlInfo > ::const_iterator it;
    it = phyLte->rxDciForUlInfo->begin();
    ss << "RxTti=,";
    for (; it != phyLte->rxDciForUlInfo->end(); ++it)
    {
        ss << it->rxTtiNumber << ",";
    }
    lte::LteLog::DebugFormat(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,%s",
            "AppendDci0",
            ss.str().c_str());
#endif
}


// /**
// FUNCTION   :: PhyLteSetMessageTxInfo
// LAYER      :: PHY
// PURPOSE    :: Set the transmit info.
// PARAMETERS ::
// + node        : Node*       : Pointer to node.
// + phyIndex    : int         : Index of the PHY.
// + phyLte      : PhyDataLte* : Pointer to the LTE PHY structure.
// + txPower_dBm : double      : Transmission power of RB in dBm.
// + msg         : The massage address to append "info".
// RETURN     :: void : NULL
// **/
void PhyLteSetMessageTxInfo(Node* node,
                            int phyIndex,
                            PhyDataLte* phyLte,
                            double txPower_dBm,
                            Message* msg)
{
    MESSAGE_AppendInfo(node,
                       msg,
                       sizeof(PhyLteTxInfo),
                       (unsigned short)INFO_TYPE_LtePhyTxInfo);

    PhyLteTxInfo* txInfo = (PhyLteTxInfo*)MESSAGE_ReturnInfo(
                                msg, (unsigned short)INFO_TYPE_LtePhyTxInfo);

    txInfo->txStationType = phyLte->stationType;
    txInfo->txPower_dBm = (float)txPower_dBm;
    txInfo->numTxAnntena = phyLte->numTxAntennas;
}

// /**
// FUNCTION   :: PhyLteIsMessageNoTransportBlock
// LAYER      :: PHY
// PURPOSE    :: Check if msg has no transport block info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + msg      : Message* : Pointer to message.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteIsMessageNoTransportBlock(Node* node, int phyIndex, Message* msg)
{
    if (NULL != MESSAGE_ReturnInfo(
                    msg, (unsigned short) INFO_TYPE_LteMacNoTransportBlock))
    {
        return TRUE;
    }
    return FALSE;
}

// /**
// FUNCTION   :: PhyLteGetMessageTxInfo
// LAYER      :: PHY
// PURPOSE    :: Get the transmit info.
// PARAMETERS ::
// + node     : Node*   : Pointer to node.
// + phyIndex : int     : Index of the PHY.
// RETURN     :: PhyLteTxInfo* : Pointer to LTE PHY transmit info.
// **/
PhyLteTxInfo* PhyLteGetMessageTxInfo(Node* node, int phyIndex, Message* msg)
{
    return (PhyLteTxInfo*)MESSAGE_ReturnInfo(
                                msg, (unsigned short)INFO_TYPE_LtePhyTxInfo);
}


// /**
// FUNCTION   :: PhyLteSetMessageDlTtiInfo
// LAYER      :: PHY
// PURPOSE    :: Get the TTI number.
// PARAMETERS ::
// + node     : Node*      : Pointer to node.
// + phyIndex : int        : Index of the PHY.
// + msg      : Message*   : Pointer to message.
// + ttiNumber: DlTtiInfo* : Pointer to the down link TTI info.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteSetMessageDlTtiInfo(Node* node,
                               int phyIndex,
                               Message* msg,
                               const DlTtiInfo* dlTtiInfo)
{
    MESSAGE_AppendInfo(node,
        msg,
        sizeof(DlTtiInfo),
        (unsigned short)INFO_TYPE_LtePhyDlTtiInfo);

    DlTtiInfo* info = (DlTtiInfo*)MESSAGE_ReturnInfo(
                            msg, (unsigned short)INFO_TYPE_LtePhyDlTtiInfo);
    if (info != NULL)
    {
        *info = *dlTtiInfo;
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteGetMessageDlTtiInfo
// LAYER      :: PHY
// PURPOSE    :: Get downlink TTI information
// PARAMETERS ::
// + node     : Node*      : Pointer to node.
// + phyIndex : int        : Index of the PHY.
// + msg      : Message*   : Pointer to message.
// + ttiNumber: DlTtiInfo* : Pointer to the downlink TTI info.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageDlTtiInfo(Node* node,
                               int phyIndex,
                               Message* msg,
                               DlTtiInfo* dlTtiInfo)
{
    DlTtiInfo *info = (DlTtiInfo*)MESSAGE_ReturnInfo(
                            msg, (unsigned short)INFO_TYPE_LtePhyDlTtiInfo);

    if (info != NULL)
    {
        memcpy(dlTtiInfo, info, sizeof(DlTtiInfo));
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteSetMessageCqiInfo
// LAYER      :: PHY
// PURPOSE    :: Set CQI information
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + cqi      : PhyLteCqi : CQI structure.
// + msg      : Message*  : The massage address to append CQI information
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteSetMessageCqiInfo(Node* node,
                             int phyIndex,
                             PhyLteCqi cqi,
                             Message* msg)
{
    MESSAGE_AppendInfo(node,
        msg,
        sizeof(PhyLteCqi),
        (unsigned short)INFO_TYPE_LtePhyCqiInfo);

    PhyLteCqi* info = (PhyLteCqi*)MESSAGE_ReturnInfo(
                            msg, (unsigned short)INFO_TYPE_LtePhyCqiInfo);
    if (info != NULL)
    {
        *info = cqi;
#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,%s=,%d,%d",
            LTE_STRING_CATEGORY_TYPE_CQI_TX,
            LTE_STRING_CATEGORY_TYPE_CQI,
            cqi.cqi0,
            cqi.cqi1);
#endif // LTE_LIB_LOG
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteGetMessageCqiInfo
// LAYER      :: PHY
// PURPOSE    :: Get the CQI info.
// PARAMETERS ::
// + node     : Node*      : Pointer to node.
// + phyIndex : int        : Index of the PHY.
// + msg      : Message*   : Pointer to message.
// + cqi      : PhyLteCqi* : Pointer to CQI structure.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageCqiInfo(Node* node,
                             int phyIndex,
                             Message* msg,
                             PhyLteCqi* cqi)
{
    PhyLteCqi* info = (PhyLteCqi*)MESSAGE_ReturnInfo(
                            msg, (unsigned short)INFO_TYPE_LtePhyCqiInfo);

    if (info != NULL)
    {
        memcpy(cqi, info, sizeof(PhyLteCqi));
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteSetMessageRiInfo
// LAYER      :: PHY
// PURPOSE    :: Set the RI info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + ri       : int*     : Pointer to RI.
// + msg      : Message* : Pointer to message.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteSetMessageRiInfo(Node* node, int phyIndex, int ri, Message* msg)
{
    MESSAGE_AppendInfo(node,
                       msg,
                       sizeof(int),
                       (unsigned short)INFO_TYPE_LtePhyRiInfo);

    int* info = (int*)MESSAGE_ReturnInfo(
                                msg, (unsigned short)INFO_TYPE_LtePhyRiInfo);

    if (info != NULL)
    {
        *info = ri;
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteGetMessageRiInfo
// LAYER      :: PHY
// PURPOSE    :: Get the RI info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + msg      : Message* : Pointer to message.
// + ri       : int*     : Pointer to RI.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageRiInfo(Node* node, int phyIndex, Message* msg, int* ri)
{

    int* info = (int*)MESSAGE_ReturnInfo(
                                msg, (unsigned short)INFO_TYPE_LtePhyRiInfo);
    if (info != NULL)
    {
        memcpy(ri, info, sizeof(int));
        return TRUE;
    }
    return FALSE;
}

// /**
// FUNCTION   :: PhyLteGetMessageSrsInfo
// LAYER      :: PHY
// PURPOSE    :: Get the SRS info.
// PARAMETERS ::
// + node     : Node*          : Pointer to node.
// + phyIndex : int            : Index of the PHY.
// + msg      : Message*       : Pointer to message.
// + srs      : PhyLteSrsInfo* : Pointer to SRS.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageSrsInfo(
    Node* node, int phyIndex, Message* msg, PhyLteSrsInfo* srs)
{

    PhyLteSrsInfo* info =
        (PhyLteSrsInfo*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LtePhySrsInfo);
    if (info != NULL)
    {
        memcpy(srs, info, sizeof(PhyLteSrsInfo));
        return TRUE;
    }
    return FALSE;
}

// /**
// FUNCTION   :: PhyLteSetMessageSrsInfo
// LAYER      :: PHY
// PURPOSE    :: Set the SRS info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + msg      : Message* : Pointer to message to append SRS info.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteSetMessageSrsInfo(Node* node, int phyIndex, Message* msg)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    MESSAGE_AppendInfo(node,
                       msg,
                       sizeof(PhyLteSrsInfo),
                       (unsigned short)INFO_TYPE_LtePhySrsInfo);

    PhyLteSrsInfo* info =
        (PhyLteSrsInfo*)MESSAGE_ReturnInfo(msg,
                                  (unsigned short)INFO_TYPE_LtePhySrsInfo);

    if (info != NULL)
    {
        info->dstRnti = phyLte->establishmentData->selectedRntieNB;

#ifdef LTE_LIB_LOG
    lte::LteLog::DebugFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s",
        LTE_STRING_CATEGORY_TYPE_SRS_TX);
#endif
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteIsMessageSrsInfo
// LAYER      :: PHY
// PURPOSE    :: Check if msg includes SRS info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + msg      : Message* : Pointer to message.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteIsMessageSrsInfo(Node* node, int phyIndex, Message* msg)
{

    char* info = (char*)MESSAGE_ReturnInfo(
                        msg,
        (unsigned short)INFO_TYPE_LtePhySrsInfo);
    if (info != NULL)
    {
        return TRUE;
    }
    return FALSE;
}

// /**
// FUNCTION   :: PhyLteSetMessagePssInfo
// LAYER      :: PHY
// PURPOSE    :: Set the PSS info.
// PARAMETERS ::
// + node     : Node*        : Pointer to node.
// + phyIndex : int          : Index of the PHY.
// + config   : LteRrcConfig : Pointer to RRC config.
// + msg      : Address of the message to append PSS info.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteSetMessagePssInfo(Node* node,
                             int phyIndex,
                             LteRrcConfig* config,
                             Message* msg)
{

    MESSAGE_AppendInfo(node,
                       msg,
                       sizeof(LteRrcConfig),
                       (unsigned short)INFO_TYPE_LtePhyPss);

    LteRrcConfig* info =
        (LteRrcConfig*)MESSAGE_ReturnInfo(msg,
                                 (unsigned short)INFO_TYPE_LtePhyPss);

    if (info != NULL)
    {
        memcpy(info, config, sizeof(LteRrcConfig));
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteGetMessagePssInfo
// LAYER      :: PHY
// PURPOSE    :: Get the PSS info.
// PARAMETERS ::
// + node     : Node*        : Pointer to node.
// + phyIndex : int          : Index of the PHY.
// + config   : LteRrcConfig : Pointer to RRC config.
// + msg      : Message*     : Pointer to message.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessagePssInfo(Node* node,
                             int phyIndex,
                             LteRrcConfig* config,
                             Message* msg)
{
    LteRrcConfig* info =
        (LteRrcConfig*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LtePhyPss);
    if (info != NULL)
    {
        memcpy(config, info , sizeof(LteRrcConfig));
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteSetGrantInfo
// LAYER      :: PHY
// PURPOSE    :: Set the Grant info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY
// + msg      : Message* : The massage address to append RA grant info.
// RETURN     :: void : NULL
// **/
BOOL PhyLteSetGrantInfo(Node* node,
                        int phyIndex,
                        Message* msg)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    LteRaGrant* grantInfo = NULL;

    ERROR_Assert(phyLte->raGrantsToSend, "phyLte->raGrantsToSend is NULL");

    if (phyLte->raGrantsToSend->size() > 0)
    {
        MESSAGE_AppendInfo(
                    node,
                    msg,
                    phyLte->raGrantsToSend->size() * sizeof(LteRaGrant),
                    (unsigned short)INFO_TYPE_LtePhyRandamAccessGrant);

        grantInfo = (LteRaGrant*)MESSAGE_ReturnInfo(msg,
                        (unsigned short)INFO_TYPE_LtePhyRandamAccessGrant);
    }

    if (grantInfo != NULL)
    {
        std::set < LteRnti > ::iterator it = phyLte->raGrantsToSend->begin();
        int count = 0;
        while (it != phyLte->raGrantsToSend->end())
        {
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,Tx,%s,UE=,"LTE_STRING_FORMAT_RNTI,
                LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
                LTE_STRING_MESSAGE_TYPE_RA_GRANT,
                it->nodeId, it->interfaceIndex);
#endif
            grantInfo[count++].ueRnti = *it;
            PhyLteAddConnectedUe(node, phyIndex, *it, phyLte);
            phyLte->raGrantsToSend->erase(it++);
        }

        return TRUE;
    }
    return FALSE;
}

// /**
// FUNCTION   ::
// LAYER      :: PHY
// PURPOSE    ::
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + phyLte   : PhyDataLte* : Pointer to the LTE PHY structure
// + error    : BOOL     : Reception of the desired signal.
//                         TRUE Reception failure
//                         FALSE Reception Success
// + srcRnti  : LteRnti  : Source node rnti.
// + msg      : The massage address to append "PHY to MAC info".
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
void PhyLteSetMessagePhyLtePhyToMacInfo(Node* node,
                                        int phyIndex,
                                        PhyDataLte* phyLte,
                                        BOOL error,
                                        LteRnti srcRnti,
                                        Message* msg)
{
    MESSAGE_AppendInfo(node,
                       msg,
                       sizeof(PhyLteTxInfo),
                       (unsigned short)INFO_TYPE_LtePhyLtePhyToMacInfo);

    PhyLtePhyToMacInfo* info =
        (PhyLtePhyToMacInfo*)MESSAGE_ReturnInfo(
                            msg,
                            (unsigned short)INFO_TYPE_LtePhyLtePhyToMacInfo);

    info->isError = error;
    info->srcRnti = srcRnti;
}

// /**
// FUNCTION   :: PhyLteIsMessageBsrInfo
// LAYER      :: PHY
// PURPOSE    :: Check the buffer status report info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + msg      : Message* : Pointer to message.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteIsMessageBsrInfo(Node* node, int phyIndex, Message* msg)
{
    LteBsrInfo* info =
        (LteBsrInfo*)MESSAGE_ReturnInfo(
                msg,
                (unsigned short)INFO_TYPE_LteMacPeriodicBufferStatusReport);
    if (info != NULL)
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteGetMessageBsrInfo
// LAYER      :: PHY
// PURPOSE    :: Get the buffer status report info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + msg      : Message* : Pointer to message.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageBsrInfo(Node* node,
                             int phyIndex,
                             Message* msg,
                             LteBsrInfo* bsr)
{
    LteBsrInfo* info =
        (LteBsrInfo*)MESSAGE_ReturnInfo(
                msg,
                (unsigned short)INFO_TYPE_LteMacPeriodicBufferStatusReport);
    if (info != NULL)
    {
        memcpy(bsr, info , sizeof(LteBsrInfo));
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: PhyLteSetMessageRrcCompleteInfo
// LAYER      :: PHY
// PURPOSE    :: Set the RRC complete info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + maxTxPower_dBm      : double* : Pointer to Maximum transmit power.
// + msg      : Message* : The massage address to append RRC complete info.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteSetMessageRrcCompleteInfo(Node* node,
                                     int phyIndex,
                                     double* maxTxPower_dBm,
                                     Message* msg)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    MESSAGE_AppendInfo(node,
               msg,
               sizeof(PhyLteRrcSetupComplete),
               (unsigned short)INFO_TYPE_LtePhyRrcConnectionSetupComplete);

    PhyLteRrcSetupComplete* info =
        (PhyLteRrcSetupComplete*)MESSAGE_ReturnInfo(
                msg,
                (unsigned short)INFO_TYPE_LtePhyRrcConnectionSetupComplete);

    if (info != NULL)
    {
        info->maxTxPower_dBm = *maxTxPower_dBm;
        info->enbRnti        = establishmentData->selectedRntieNB;
        return TRUE;
    }

    return FALSE;
}



// /**
// FUNCTION   :: PhyLteSetMessageRrcReconfCompleteInfo
// LAYER      :: PHY
// PURPOSE    :: Set the RRC complete info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + maxTxPower_dBm      : double* : Pointer to Maximum transmit power.
// + msg      : Message* : The massage address to append RRC complete info.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteSetMessageRrcReconfCompleteInfo(Node* node,
                                     int phyIndex,
                                     double* maxTxPower_dBm,
                                     Message* msg)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    MESSAGE_AppendInfo(node,
               msg,
               sizeof(PhyLteRrcReconfComplete),
               (unsigned short)INFO_TYPE_LtePhyRrcConnectionReconfComplete);

    PhyLteRrcReconfComplete* info =
        (PhyLteRrcReconfComplete*)MESSAGE_ReturnInfo(
                msg,
                (unsigned short)INFO_TYPE_LtePhyRrcConnectionReconfComplete);

    if (info != NULL)
    {
        info->maxTxPower_dBm = *maxTxPower_dBm;
        info->enbRnti        = establishmentData->selectedRntieNB;
        return TRUE;
    }

    return FALSE;
}



// /**
// FUNCTION   :: PhyLteGetMessageRrcCompleteInfo
// LAYER      :: PHY
// PURPOSE    :: Get the RRC complete info.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + msg      : Message*  : Pointer to message.
// + rrcSetupCompleteinfo : PhyLteRrcSetupComplete* :
//                   Pointer to the message info of the RRC setup complete.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageRrcCompleteInfo(
                             Node* node,
                             int phyIndex,
                             Message* msg,
                             PhyLteRrcSetupComplete* rrcSetupCompleteinfo)
{
    PhyLteRrcSetupComplete* info =
        (PhyLteRrcSetupComplete*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LtePhyRrcConnectionSetupComplete);

    if (info != NULL)
    {
        memcpy(rrcSetupCompleteinfo, info, sizeof(PhyLteRrcSetupComplete));
        return TRUE;
    }

    return FALSE;
}



// /**
// FUNCTION   :: PhyLteGetMessageRrcReconfCompleteInfo
// LAYER      :: PHY
// PURPOSE    :: Get the RRC complete info.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + msg      : Message*  : Pointer to message.
// + rrcSetupCompleteinfo : PhyLteRrcSetupComplete* :
//                   Pointer to the message info of the RRC setup complete.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageRrcReconfCompleteInfo(
                             Node* node,
                             int phyIndex,
                             Message* msg,
                             PhyLteRrcReconfComplete* rrcReconfCompleteinfo)
{
    PhyLteRrcReconfComplete* info =
        (PhyLteRrcReconfComplete*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LtePhyRrcConnectionReconfComplete);

    if (info != NULL)
    {
        memcpy(rrcReconfCompleteinfo, info, sizeof(PhyLteRrcReconfComplete));
        return TRUE;
    }

    return FALSE;
}



// /**
// FUNCTION   :: PhyLteGetMessageRlcResetData
// LAYER      :: PHY
// PURPOSE    :: Get the RLC reset info.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + msg      : Message*  : Pointer to message.
// + rlcResetData : std::vector< LteRlcAmResetData >& :
//                  List of RLC Reset information.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteGetMessageRlcResetData(
    Node* node,
    int phyIndex,
    Message* msg,
    std::vector < LteRlcAmResetData > &rlcResetData)
{
    BOOL ret = FALSE;
    rlcResetData.clear();
    int infoSize = MESSAGE_ReturnInfoSize(msg, INFO_TYPE_LteRlcAmResetData);
    if (infoSize > 0)
    {
        ret = TRUE;
        LteRlcAmResetData* info =
            (LteRlcAmResetData*)MESSAGE_ReturnInfo(
                msg, INFO_TYPE_LteRlcAmResetData);
        int size = infoSize / sizeof(LteRlcAmResetData);
        rlcResetData.resize(size);

        for (int i = 0; i < size; i++)
        {
            rlcResetData[i] = info[i];
        }
    }

    return ret;
}

// /**
// FUNCTION   :: PhyLteChangeState
// LAYER      :: PHY
// PURPOSE    :: To change a given state.
// PARAMETERS ::
// + node     : Node*       : Pointer to node.
// + phyIndex : int         : Index of the PHY.
// + phyLte   : PhyDataLte* : Pointer to the LTE PHY structure
// + status   : PhyLteState : State transition.
// RETURN     :: void : NULL
// **/
void PhyLteChangeState(Node* node,
                       int phyIndex,
                       PhyDataLte* phyLte,
                       PhyLteState status)
{

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
                              node->phyData[phyIndex]->macInterfaceIndex,
                              "PHY:DEBUG:PhyLteChangeState:start"
                              ,"%d,%d,%d,%d,%d",
                              status,
                              phyLte->previousTxState,
                              phyLte->previousRxState,
                              phyLte->txState,
                              phyLte->rxState);
#endif // LTE_LIB_LOG
#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_PHY_STATE,
            LteGetPhyStateString(status));
#endif // LTE_LIB_LOG

    switch (status){
    case PHY_LTE_POWER_OFF:
        phyLte->previousTxState = status;
        phyLte->previousRxState = status;
        phyLte->txState = status;
        phyLte->rxState = status;
        break;

    case PHY_LTE_IDLE_CELL_SELECTION:
    case PHY_LTE_CELL_SELECTION_MONITORING:
    case PHY_LTE_IDLE_RANDOM_ACCESS:
    case PHY_LTE_RA_PREAMBLE_TRANSMISSION_RESERVED:
    case PHY_LTE_RA_PREAMBLE_TRANSMITTING:
    case PHY_LTE_RA_GRANT_WAITING:
        phyLte->previousTxState = phyLte->txState;
        phyLte->previousRxState = phyLte->rxState;
        phyLte->txState = status;
        phyLte->rxState = status;
        break;

    case PHY_LTE_DL_IDLE:
        if (phyLte->stationType == LTE_STATION_TYPE_ENB)
        {
            phyLte->previousTxState = phyLte->txState;
            phyLte->txState = status;
        }
        else
        {
            phyLte->previousRxState = phyLte->rxState;
            phyLte->rxState = status;
        }

        break;

    case PHY_LTE_UL_IDLE:
        if (phyLte->stationType == LTE_STATION_TYPE_ENB)
        {
            phyLte->previousRxState = phyLte->rxState;
            phyLte->rxState = status;
        }
        else
        {
            phyLte->previousTxState = phyLte->txState;
            phyLte->txState = status;
        }
        break;

    case PHY_LTE_UL_FRAME_RECEIVING:
        if (phyLte->stationType == LTE_STATION_TYPE_ENB)
        {
            phyLte->previousRxState = phyLte->rxState;
            phyLte->rxState = status;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "PHY-LTE: PhyLteState %d %d error",
                    (int)status, (int)phyLte->stationType);
            ERROR_Assert(FALSE, errorStr);
        }
        break;

    case PHY_LTE_DL_FRAME_TRANSMITTING:
        if (phyLte->stationType == LTE_STATION_TYPE_ENB)
        {
            phyLte->previousTxState = phyLte->txState;
            phyLte->txState = status;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "PHY-LTE: PhyLteState %d %d error",
                    (int)status, (int)phyLte->stationType);
            ERROR_Assert(FALSE, errorStr);
        }
        break;

    case PHY_LTE_DL_FRAME_RECEIVING:
        if (phyLte->stationType == LTE_STATION_TYPE_UE){
            phyLte->previousRxState = phyLte->rxState;
            phyLte->rxState = status;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "PHY-LTE: PhyLteState %d %d error",
                    (int)status, (int)phyLte->stationType);
            ERROR_Assert(FALSE, errorStr);
        }
        break;

    case PHY_LTE_UL_FRAME_TRANSMITTING:
        if (phyLte->stationType == LTE_STATION_TYPE_UE){
            phyLte->previousTxState = phyLte->txState;
            phyLte->txState = status;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "PHY-LTE: PhyLteState %d %d error",
                    (int)status, (int)phyLte->stationType);
            ERROR_Assert(FALSE, errorStr);
        }
        break;

    default:
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
                "PHY-LTE: PhyLteState %d error",
                (int)status);
        ERROR_Assert(FALSE, errorStr);
    }


#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
                              node->phyData[phyIndex]->macInterfaceIndex,
                              "PHY:DEBUG:PhyLteChangeState:end"
                              ,"%d,%d,%d,%d,%d",
                              status,
                              phyLte->previousTxState,
                              phyLte->previousRxState,
                              phyLte->txState,
                              phyLte->rxState);
#endif // LTE_LIB_LOG


    return;
}

// /**
// FUNCTION   :: PhyLteSetMessageTraceData
// LAYER      :: PHY
// PURPOSE    :: Setup to the message trace data.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + msg          : Message      : target massage
// RETURN     :: void  : NULL
// **/
void PhyLteSetMessageTraceData(Node* node, int phyIndex, Message* msg)
{
    // Add TRACE_PHY_LTE header to msg.
    MESSAGE_AddHeader(node, msg, 0, TRACE_PHY_LTE);

    return;
}

// /**
// FUNCTION   :: PhyLteInitConnectedUeInfo
// LAYER      :: PHY
// PURPOSE    :: Initialize connected-UE information structure.
// PARAMETERS ::
// + node     : Node*       : Pointer to node.
// + phyIndex : int         : Index of the PHY
// + phyLte   : PhyDataLte* : Pointer to PHY LTE structure
// RETURN     :: void  : NULL
// **/
void PhyLteInitConnectedUe(Node* node, int phyIndex, PhyDataLte* phyLte)
{
    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        phyLte->ueInfoList->clear();

#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteInit:PhyLteInitConnectedUe:"
        ,"");
#endif // LTE_LIB_LOG

    }
    else
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
                "PHY-LTE: PhyLteInitConnectedUe error");
        ERROR_Assert(FALSE, errorStr);
    }

    return;
}


// /**
// FUNCTION   :: PhyLteInitConnectedUeInfo
// LAYER      :: PHY
// PURPOSE    :: Initialize connected-UE information structure.
// PARAMETERS ::
// + node     : Node*        : Pointer to node.
// + phyIndex : int          : Index of the PHY
// + ueInfo   : PhyLteConnectedUeInfo* : connected-UE information structure.
// RETURN     :: void  : NULL
// **/
void PhyLteInitConnectedUeInfo(Node* node,
                               int phyIndex,
                               PhyLteConnectedUeInfo* ueInfo)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    ueInfo->maxTxPower_dBm = LTE_NEGATIVE_INFINITY_POWER_dBm;
    ueInfo->cqiReceiver.init(node, phyIndex);
    ueInfo->ulInstantPathloss_dB.resize(
                phyLte->numRxAntennas, LTE_NEGATIVE_INFINITY_PATHLOSS_dB);

    ueInfo->isDetecting = FALSE;
    ueInfo->connectedTime = node->getNodeTime();
    ueInfo->isFirstChecking = TRUE;

    return;
}

// /**
// FUNCTION   :: PhyLteAddConnectedUe
// LAYER      :: PHY
// PURPOSE    :: Add connected-UE/Serving-eNB.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + ueRnti       : LteRnti      : RNTI of UE to add
// + phyLte       : PhyDataLte*  : PHY data structure
// RETURN     :: BOOL: TRUE : registration of new UE is succeed,
//                     FALSE : otherwise.
// **/
BOOL PhyLteAddConnectedUe(Node* node,
                          int phyIndex,
                          LteRnti ueRnti,
                          PhyDataLte* phyLte)
{
    BOOL ret = FALSE;

    std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator it
        = phyLte->ueInfoList->find(ueRnti);

    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        ERROR_Assert(it == phyLte->ueInfoList->end(),
            "Existing UE is requested to connect.");

        PhyLteConnectedUeInfo ueInfo;
        PhyLteInitConnectedUeInfo(node, phyIndex, &ueInfo);
        phyLte->ueInfoList->insert(
            std::pair < LteRnti, PhyLteConnectedUeInfo > (
            ueRnti,
            ueInfo));
        ret = TRUE;

#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteInit:PhyLteAddConnectedUe:"
        ,"%d,%d",
        ueRnti.interfaceIndex,
        ueRnti.nodeId);
#endif // LTE_LIB_LOG

    }

    return ret;
}


// /**
// FUNCTION   :: PhyLteCheckRxDisconnected
// LAYER      :: PHY
// PURPOSE    :: Check if indicated UE/eNB is disconnected
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + phyLte       : PhyDataLte*  : PHY data structure
// + target       : LteRnti       : RNTI of Tx-node.
// RETURN     :: BOOL: when true, target is connected.
// **/
BOOL PhyLteCheckRxDisconnected(Node* node,
                               int phyIndex,
                               PhyDataLte* phyLte,
                               LteRnti target)
{
    // uplink
    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator it;
        it = phyLte->ueInfoList->find(target);
        if (it != phyLte->ueInfoList->end())
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    // downlink
    else // phyLte->stationType == LTE_STATION_TYPE_UE
    {
        PhyLteEstablishmentData* establishmentData
                                        = phyLte->establishmentData;
        // Signal from serving eNB
        if (target == establishmentData->selectedRntieNB)
        {
            return FALSE; // not interference
        }
        // Signal from non-serving eNB
        else
        {
            return TRUE; // interference
        }
    }
}

// /**
// FUNCTION   :: PhyLteGetMessageControlInfo
// LAYER      :: PHY
// PURPOSE    :: Check and receive control messages.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + propRxInfo   : PropRxInfo*  : message of propRx.
// + arrival      : BOOL         : Indicate "arrival" or "signal end"
// RETURN     :: BOOL: when true, Rx-message is control message.
// **/
static
BOOL PhyLteGetMessageControlInfo(Node* node,
                                 int phyIndex,
                                 PhyDataLte* phyLte,
                                 PropRxInfo* propRxInfo,
                                 BOOL arrival)
{
#ifdef LTE_LIB_USE_ONOFF
    // If PHY state is not stationary state, no control message is received.
    // #5
    if (PhyLteIsInStationaryState(node, phyIndex) == FALSE)
    {
        return FALSE;
    }
#endif
    BOOL keepFlag = FALSE;
    Message* msg = propRxInfo->txMsg;
    PropTxInfo* propTxInfo =
        (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);
    LteRnti txRnti = LteRnti(propTxInfo->txNodeId,
        LteGetMacInterfaceIndexFromPhyIndex(
        propTxInfo->txNode, propTxInfo->phyIndex));
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    // Control messages to receive on "arrival" timing
    if (arrival)
    {
        // If downlink TTI info from serving eNB is detected,
        // notify it to MAC layer
        DlTtiInfo ttiInfo;
        if ((phyLte->stationType == LTE_STATION_TYPE_UE)
            && (FALSE == PhyLteCheckRxDisconnected(node,
                                                   phyIndex,
                                                   phyLte,
                                                   txRnti))
            && (TRUE == PhyLteGetMessageDlTtiInfo(node,
                                                  phyIndex,
                                                  msg,
                                                  &ttiInfo)))
        {
            MacLteSetTtiNumber(node, interfaceIndex, ttiInfo.ttiNumber);
        }

        // If CQI info is included, notify it to CQI receiver.
        PhyLteCqi cqiInfo;
        if (PhyLteGetMessageCqiInfo(node,
                                   phyIndex,
                                   msg,
                                   &cqiInfo))
        {
            std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator it;
            it = phyLte->ueInfoList->find(txRnti);
            if (it != phyLte->ueInfoList->end())
            {
                it->second.cqiReceiver.notifyCqiUpdate(cqiInfo);
#ifdef LTE_LIB_LOG
                lte::LteLog::InfoFormat(
                    node, node->phyData[phyIndex]->macInterfaceIndex,
                    LTE_STRING_LAYER_TYPE_PHY,
                    "%s,%s=,"LTE_STRING_FORMAT_RNTI
                    ",%s=,%d,%d",
                    LTE_STRING_CATEGORY_TYPE_CQI_RX,
                    LTE_STRING_STATION_TYPE_UE,
                    it->first.nodeId,
                    it->first.interfaceIndex,
                    LTE_STRING_CATEGORY_TYPE_CQI,
                    cqiInfo.cqi0,
                    cqiInfo.cqi1);
#endif // LTE_LIB_LOG
            }
        }

        // If RI info is included, notify it to CQI receiver.
        int riInfo;
        if (PhyLteGetMessageRiInfo(node,
                                  phyIndex,
                                  msg,
                                  &riInfo))
        {
            std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator it;
            it = phyLte->ueInfoList->find(txRnti);

            if (it != phyLte->ueInfoList->end())
            {
                it->second.cqiReceiver.notifyRiUpdate(riInfo);
            }
        }

        // If BSR is included, notify it to MAC layer
        LteBsrInfo bsr;
        if (PhyLteGetMessageBsrInfo(node,
            phyIndex,
            msg,
            &bsr))
        {
            MacLteNotifyBsrFromPhy(node, interfaceIndex, bsr);

#ifdef LTE_LIB_LOG
            lte::LteLog::Debug2Format(node,
                node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteGetMessageControlInfo:"
                ,"BsrInfo");
#endif // LTE_LIB_LOG
        }

        // IF RLC reset info is included, notify it to RLC layer.
        std::vector < LteRlcAmResetData > resetData;
        if (PhyLteGetMessageRlcResetData(node,
                                        phyIndex,
                                        msg,
                                        resetData))
        {
            std::vector < LteRlcAmResetData > ::iterator itr;
            for (itr  = resetData.begin();
                itr != resetData.end();
                ++itr)
            {
                if (itr->dst == LteRnti(node->nodeId, interfaceIndex))
                {
                    LteRlcAmIndicateResetWindow(
                        node, node->phyData[phyIndex]->macInterfaceIndex,
                        itr->src, itr->bearerId);
                }
            }
        }

        // IF RRC connected complete is included, notify it to RRC layer.
        PhyLteRrcSetupComplete rrcSetupCompleteinfo;
        if (PhyLteGetMessageRrcCompleteInfo(node,
                                            phyIndex,
                                            msg,
                                            &rrcSetupCompleteinfo))
        {
            // Register received MAX transmission power of connected UE
            PhyLteSetUlMaxTxPower_dBm(node,
                                      phyIndex,
                                      txRnti,
                                      rrcSetupCompleteinfo.maxTxPower_dBm);

            // Notify RRC Connection Setup Complete
            RrcLteNotifyRrcConnectionSetupComplete(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            rrcSetupCompleteinfo.enbRnti,
                            txRnti);

#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,Rx,%s",
                LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
                LTE_STRING_MESSAGE_TYPE_RRC_CONNECTION_SETUP_COMPLETE);

            lte::LteLog::InfoFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,%s,%s=,"LTE_STRING_FORMAT_RNTI,
                LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
                LTE_STRING_MESSAGE_TYPE_RRC_CONNECTION_SETUP_COMPLETE,
                LTE_STRING_STATION_TYPE_UE,
                txRnti.nodeId, txRnti.interfaceIndex);
#endif

#ifdef LTE_LIB_LOG
            lte::LteLog::Debug2Format(node,
                node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteSetUlMaxTxPower_dBm:"
                ,"PhyLteGetMessageRrcCompleteInfo:%lf"
                ,rrcSetupCompleteinfo.maxTxPower_dBm);
#endif // LTE_LIB_LOG
        }

        // IF RRC connected reconf complete is included,
        // notify it to RRC layer.
        PhyLteRrcReconfComplete rrcReconfCompleteinfo;
        if (PhyLteGetMessageRrcReconfCompleteInfo(node,
                                            phyIndex,
                                            msg,
                                            &rrcReconfCompleteinfo))
        {
            // Register received MAX transmission power of connected UE
            PhyLteSetUlMaxTxPower_dBm(node,
                                      phyIndex,
                                      txRnti,
                                      rrcReconfCompleteinfo.maxTxPower_dBm);

            // Notify RRC Connection Setup Complete
            RrcLteNotifyRrcConnectionReconfComplete(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            rrcReconfCompleteinfo.enbRnti,
                            txRnti);
        }

        // If PSS is included, register RRC config info
        LteRrcConfig config;
        if ((PhyLteGetMessagePssInfo(node,
                                     phyIndex,
                                     &config,
                                     msg))
            && (FALSE == PhyLteCheckRxDisconnected(node,
                                                   phyIndex,
                                                   phyLte,
                                                   txRnti)))
        {
            // Register RRC config
            SetLteRrcConfig(node, phyIndex, config);

#ifdef LTE_LIB_LOG
            lte::LteLog::Debug2Format(node,
                node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteGetMessageControlInfo:"
                ,"PssInfo");
#endif // LTE_LIB_LOG
        }

        // Check SRS
        if (PhyLteIsMessageSrsInfo(node,
                                  phyIndex,
                                  msg))
        {
#ifdef LTE_LIB_LOG
           lte::LteLog::Debug2Format(node,
               node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteGetMessageControlInfo:",
                "SrsInfo");
#endif // LTE_LIB_LOG
#ifdef LTE_LIB_LOG
            lte::LteLog::DebugFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,%s=,"LTE_STRING_FORMAT_RNTI,
                LTE_STRING_CATEGORY_TYPE_SRS_RX,
                LTE_STRING_STATION_TYPE_UE,
                txRnti.nodeId,
                txRnti.interfaceIndex);
#endif // LTE_LIB_LOG
        }


        // IF RRC measurement report is included, notify it to RRC layer.
        LteRnti srcRnti;
        std::list<MeasurementReport> measurementReport;
        if (PhyLteGetMessageMeasurementReportInfo(
                                                node,
                                                phyIndex,
                                                msg,
                                                &srcRnti,
                                                &measurementReport))
        {
#ifdef LTE_LIB_LOG
            for (std::list<MeasurementReport>::iterator it =
                measurementReport.begin(); it != measurementReport.end();
                it++)
            {
                lte::LteLog::InfoFormat(
                    node, node->phyData[phyIndex]->macInterfaceIndex,
                    LTE_STRING_LAYER_TYPE_RRC,
                    LAYER3_LTE_MEAS_CAT_MEASUREMENT","
                    LTE_STRING_FORMAT_RNTI","
                    "[receive reports],"
                    "%s,",
                    srcRnti.nodeId, srcRnti.interfaceIndex,
                    it->ToString().c_str()
                    );
            }
#endif // LTE_LIB_LOG
            // send report list to Handover decision
            Layer3LteIFHPNotifyMeasurementReportReceived(
                node, interfaceIndex, srcRnti, &measurementReport);
        }

    }
    // Control signals to check on SignalEnd timing.
    else
    {
        // get RRC conn reconf incl mobilityControlInformation
        RrcConnReconfInclMoblityControlInfomation reconf;
        if (PhyLteGetMessageRrcConnReconf(
                                                node,
                                                phyIndex,
                                                msg,
                                                &reconf))
        {
            // notify reconf to L3 to execute handover
            Layer3LteNotifyRrcConnReconf(
                            node,
                            interfaceIndex,
                            reconf.hoParticipator,
                            reconf.reconf);
        }

        if ((keepFlag == FALSE) &&
            (MESSAGE_ReturnInfo(
                        msg,
                        (unsigned short)INFO_TYPE_LteMacNoTransportBlock)))
        {
#ifdef LTE_LIB_LOG
           lte::LteLog::Debug2Format(node,
               node->phyData[phyIndex]->macInterfaceIndex,
               "PHY:DEBUG:PhyLteGetMessageControlInfo:"
               ,"INFO_TYPE_LteMacNoTransportBlock");
#endif // LTE_LIB_LOG
            return TRUE;
        }
    }
    return FALSE;
}

// /**
// FUNCTION   :: PhyLteGetDciForUl
// LAYER      :: PHY
// PURPOSE    :: Get newest DCI for UE.
//               When pop-flag is true, update newest-data.
// PARAMETERS ::
// + node        : Node*  : Pointer to node.
// + phyIndex    : int    : Index of the PHY
// + ttiNumber   : UInt64 : Current TTI number
// + pop       : BOOL   : when true, popping target one and remove older ones
// + setValue    : LteDciFormat0* : data pointer of DCI for UE
// RETURN       ::  BOOL    : when true, successful of get data.
// **/
BOOL PhyLteGetDciForUl(Node* node,
            int phyIndex,
            UInt64 ttiNumber,
            BOOL pop,
            LteDciFormat0* setValue)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;


    // First of all, purge old DCI format 0 in the buffer, if exists.

    UInt64 rxTtiNumber = (ttiNumber - PHY_LTE_DCI_RECEPTION_DELAY);

    ERROR_Assert(PHY_LTE_DCI_RECEPTION_DELAY > 0,
                "Invalid DCI reception delay");

    while ((phyLte->rxDciForUlInfo->size() > 0) &&
           (phyLte->rxDciForUlInfo->front().rxTtiNumber < rxTtiNumber))
    {
        phyLte->rxDciForUlInfo->pop_front();
    }

    // Front DCI is the candidate DCI

    BOOL ret = FALSE;

    if (phyLte->rxDciForUlInfo->empty() != true)
    {
        if (phyLte->rxDciForUlInfo->front().rxTtiNumber == rxTtiNumber)
        {
            *setValue = phyLte->rxDciForUlInfo->front().dciFormat0;
            ret = TRUE;

            if (pop == TRUE)
            {
                phyLte->rxDciForUlInfo->pop_front();
            }
        }
    }

    return ret;
};

// /**
// FUNCTION   :: PhyLteGetChannelMatrix
// LAYER      :: PHY
// PURPOSE    :: Calculate channel matrix.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + phyLte       : PhyDataLte*  : PHY data structure
// + propTxInfo   : PropTxInfo*   : message of propTx.
// + lteTxInfo    : PhyLteTxInfo* : LTE-info of propRx.
// + matH         : Cmat& :
//                    Reference to the buffer channel matrix is set to.
// RETURN     :: void  : NULL
// **/
void PhyLteGetChannelMatrix(Node* node,
                            int phyIndex,
                            PhyDataLte* phyLte,
                            PropTxInfo* propTxInfo,
                            PhyLteTxInfo* lteTxInfo,
                            Cmat& matH)
{
    int chNo;

    ERROR_Assert(matH.size() ==
                    (size_t)(phyLte->numRxAntennas * lteTxInfo->numTxAnntena),
                "Invalid matrix size.");

    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
#if PHY_LTE_APPLY_SAME_FADING_FOR_DL_AND_UL
        chNo = phyLte->dlChannelNo;
#else
        chNo = phyLte->ulChannelNo;
#endif
    }
    else
    {
        chNo = phyLte->dlChannelNo;
    }

    for (UInt8 row = 0; row < phyLte->numRxAntennas; row++)
    {
        for (UInt8 col = 0; col < lteTxInfo->numTxAnntena; col++)
        {
            Dcomp fading_Comp(0.0, 0.0);
            PhyLteCalculateFading(propTxInfo,
                node,
                col,
                row,
                chNo,
                node->getNodeTime(),
                &fading_Comp);
            // H
            int mIndex = MATRIX_INDEX(row,col,lteTxInfo->numTxAnntena);
            matH[mIndex] = fading_Comp;
        }
    }
}

// /**
// FUNCTION   :: PhyLteChannelMatrixMultiplyPathloss
// LAYER      :: PHY
// PURPOSE    :: Apply pathloss for channel matrix.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + phyLte       : PhyDataLte*  : PHY data structure
// + lteTxInfo    : PhyLteTxInfo* : LTE-info of propRx.
// + pathloss_dB  : double        : Pathloss
// + matHhat      : Cmat          : Channel matrix to apply pathloss_dB
// RETURN     :: void : NULL
// **/
void PhyLteChannelMatrixMultiplyPathloss(Node* node,
                            int phyIndex,
                            PhyDataLte* phyLte,
                            PhyLteTxInfo* lteTxInfo,
                            double pathloss_dB,
                            Cmat& matHhat)
{
    for (UInt8 row = 0; row < phyLte->numRxAntennas; row++)
    {
        for (UInt8 col = 0; col < lteTxInfo->numTxAnntena; col++)
        {
            // H^ = GH
            int mIndex = MATRIX_INDEX(row,col,lteTxInfo->numTxAnntena);
            matHhat[mIndex] *= sqrt(1.0/NON_DB(pathloss_dB));
        }
    }
}

// /**
// FUNCTION   :: PhyLteGetChannelMatrix
// LAYER      :: PHY
// PURPOSE    :: Calculate channel matrix including pathloss.
// PARAMETERS ::
// + node         : Node*         : Pointer to node.
// + phyIndex     : int           : Index of the PHY
// + phyLte       : PhyDataLte*   : PHY data structure
// + propTxInfo   : PropTxInfo*   : message of propTx.
// + lteTxInfo    : PhyLteTxInfo* : LTE-info of propRx.
// + pathloss_dB  : double        : Pathloss of propRx.
// + matH         : Cmat& :
//                    Reference to the buffer channel matrix is set to.
// RETURN     :: void : NULL
// **/
void PhyLteGetChannelMatrix(Node* node,
                            int phyIndex,
                            PhyDataLte* phyLte,
                            PropTxInfo* propTxInfo,
                            PhyLteTxInfo* lteTxInfo,
                            double pathloss_dB,
                            Cmat& matHhat)
{
    ERROR_Assert(matHhat.size() ==
                   (size_t)( phyLte->numRxAntennas * lteTxInfo->numTxAnntena),
                "Invalid matrix size.");

    PhyLteGetChannelMatrix(
        node,
        phyIndex,
        phyLte,
        propTxInfo,
        lteTxInfo,
        matHhat);

    PhyLteChannelMatrixMultiplyPathloss(
        node,
        phyIndex,
        phyLte,
        lteTxInfo,
        pathloss_dB,
        matHhat);
}

// /**
// FUNCTION   :: PhyLteGetSinr
// LAYER      :: PHY
// PURPOSE    :: Calculate SINR of each transport block.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + txScheme     : LteTxScheme  : Transmission scheme
// + phyLte       : PhyDataLte*  : Pointer to LTE PHY structure
// + usedRB_list  : UInt8[]      : RB allocation array
// + matHhat      : Cmat&        : Channel matrix
// + geometry     : double       : geometry
// + txPower_mW   : double       : Transmission power in mW
// + useFilteredInterferencePower
//                : bool         : Whether to use filtered interference power
// + isForCqi     : bool         :
//                    Indicate SINR calculation is for CQI or not (For log)
// + txRnti       : LteRnti      : RNTI of tx node. (For log)
// RETURN     :: double : SINR
// **/
std::vector < double > PhyLteCalculateSinr(Node* node,
                     int phyIndex,
                     LteTxScheme txScheme,
                     PhyDataLte* phyLte,
                     UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB],
                     Cmat& matHhat,
                     double geometry,
                     double txPower_mW,
                     bool useFilteredInterferencePower,
                     bool isForCqi,  // For log
                     LteRnti txRnti) // For log
{
#if !PHY_LTE_ENABLE_INTERFERENCE_FILTERING
    useFilteredInterferencePower = false;
#endif

    int numUsedRBs = 0;

    int numSinrs = (txScheme == TX_SCHEME_OL_SPATIAL_MULTI ? 2 : 1);
    std::vector < double > sinr(numSinrs, 0.0);

    for (UInt8 i = 0; i < phyLte->numResourceBlocks; i++)
    {
        if (usedRB_list[i] != 0)
        {
            double ifPower_mW = 0.0;
            
            if (useFilteredInterferencePower){
#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
#if PHY_LTE_INTERFERENCE_FILTERING_IN_DBM
                double ifPower_dBm;
                phyLte->filteredInterferencePower[i].get(&ifPower_dBm);
                ifPower_mW = NON_DB(ifPower_dBm);
#else
                phyLte->filteredInterferencePower[i].get(&ifPower_mW);
#endif
#endif
            }else{
                ifPower_mW = phyLte->interferencePower_mW[i];
            }

            std::vector < double > sinrForRb = PhyLteCalculateSinr(
                node, phyIndex,
                txScheme, phyLte,
                matHhat, geometry,
                txPower_mW, ifPower_mW);

#ifdef LTE_LIB_LOG
            for (int tbIndex = 0; tbIndex < sinrForRb.size(); ++tbIndex){
                stringstream ss;
                char buf[MAX_STRING_LENGTH];
                ss << "isForCqi=," << isForCqi << ","
                   << "rnti=," << STR_RNTI(buf,txRnti) << ","
                   << "txScheme=," << LteGetTxSchemeString(txScheme) << ","
                   << "tbNo=," << (int)(tbIndex) <<","
                   << "rbNo=," << (int)i << ","
                   << "Non_dB=," << sinrForRb[tbIndex] << ","
                   << "dB=," << IN_DB(sinrForRb[tbIndex]) << ","
                   << "txPower_mW=," << txPower_mW << ","
                   << "ifPower_mW=," << ifPower_mW << ","
                   << "rbNoisePower_mW=," << phyLte->rbNoisePower_mW << ","
                   << "geometry=," << geometry << ","
                   << "pathloss_dB=," << IN_DB(1.0/geometry) << ","
                   << "ChRspHat=,";

                for (int mi = 0; mi < matHhat.size(); ++mi)
                {
                    char buf[MAX_STRING_LENGTH];
                    ss << STR_COMPLEX(buf, matHhat[mi]) << ",";
                }

                lte::LteLog::DebugFormat(
                    node,
                    node->phyData[phyIndex]->macInterfaceIndex,
                    LTE_STRING_LAYER_TYPE_PHY,
                    "%s,%s",
                    LTE_STRING_CATEGORY_TYPE_RB_SINR,
                    ss.str().c_str());
            }
#endif

            for (size_t s = 0; s < sinr.size(); ++s)
            {
                sinr[s] += sinrForRb[s];
            }

            numUsedRBs++;
        }
    }

    ERROR_Assert(numUsedRBs > 0,
                "Number of RBs for SINR calculation is 0.\n");

    for (size_t s = 0; s < sinr.size(); ++s)
    {
        sinr[s] = sinr[s] / numUsedRBs;
    }

    return sinr;
}


// /**
// FUNCTION   :: PhyLteGetSinr
// LAYER      :: PHY
// PURPOSE    :: Calculate SINR of each transport block.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + txScheme     : LteTxScheme  : Transmission scheme
// + phyLte       : PhyDataLte*  : PHY data structure
// + geometry     : double       : geometry.
// + txPower_mW   : double       : Tx-signal power.
// + ifPower_mW   : double       : Interference signal power.
// RETURN     :: double : SINR
// **/
std::vector < double > PhyLteCalculateSinr(Node* node,
                     int phyIndex,
                     LteTxScheme txScheme,
                     PhyDataLte* phyLte,
                     Cmat& matHhat,
                     double geometry,
                     double txPower_mW,
                     double ifPower_mW)
{
    int numSinrs = (txScheme == TX_SCHEME_OL_SPATIAL_MULTI ? 2 : 1);
    std::vector < double > sinr(numSinrs, 0.0);

    // Single antenna transmission
    if (txScheme == TX_SCHEME_SINGLE_ANTENNA)
    {
        //SISO
        if (phyLte->numRxAntennas == 1)
        {
            sinr[0] = norm(matHhat[MATRIX_INDEX(0,0,1)])
                * txPower_mW
                / (phyLte->rbNoisePower_mW + ifPower_mW);
        }
        //ReceveDiversity
        else
        {
            sinr[0]  = norm(matHhat[MATRIX_INDEX(0,0,1)])
                   * txPower_mW
                   / (phyLte->rbNoisePower_mW + ifPower_mW);
            sinr[0] += norm(matHhat[MATRIX_INDEX(1,0,1)])
                    * txPower_mW
                    / (phyLte->rbNoisePower_mW + ifPower_mW);
        }
    }
    // TxDiversty (SFBC)
    else if (txScheme == TX_SCHEME_DIVERSITY)
    {
        double txPower_mW_per_antenna = txPower_mW / 2.0;

        if (phyLte->numRxAntennas == 1)
        {
            sinr[0] = (norm(matHhat[MATRIX_INDEX(0,0,2)])
                   + norm(matHhat[MATRIX_INDEX(0,1,2)]))
                   * txPower_mW_per_antenna
                   / (phyLte->rbNoisePower_mW + ifPower_mW);
        }
        else
        {
            sinr[0] = (norm(matHhat[MATRIX_INDEX(0,0,2)])
                   + norm(matHhat[MATRIX_INDEX(0,1,2)]))
                   * txPower_mW_per_antenna
                   / (phyLte->rbNoisePower_mW + ifPower_mW);
            sinr[0] += (norm(matHhat[MATRIX_INDEX(1,0,2)])
                   + norm(matHhat[MATRIX_INDEX(1,1,2)]))
                   * txPower_mW_per_antenna
                   / (phyLte->rbNoisePower_mW + ifPower_mW);
        }
    }
    // OpenLoopSpatialMultiplexing
    else //TX_SCHEME_OL_SPATIAL_MULTI
    {
        if (phyLte->numRxAntennas == 1)
        {
            ERROR_Assert(FALSE, "PHYLTE:numRxAntennas");
        }
        else
        {
            // P
            Cmat matP(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
            matP = PhyLteGetPrecodingMatrixList(node, phyIndex);
            // H~ = H^P
            Cmat matHtilde(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
            matHtilde = MulMatrix(matHhat, matP);
            // H~*
            Cmat matHtildeConjT(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
            matHtildeConjT = GetConjugateTransposeMatrix(matHtilde);
            // R
            double r_real =
                (phyLte->rbNoisePower_mW + ifPower_mW) / txPower_mW;
            Dcomp r_elm(r_real, 0);
            Cmat matR(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
            matR = GetDiagMatrix(r_elm, r_elm);
            // W (MMSE weight) = inv(H~*H~ + R) H~*
            Cmat matW_HH(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
            matW_HH = MulMatrix(matHtildeConjT, matHtilde);
            Cmat matW_HHR(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
            matW_HHR = SumMatrix(matW_HH, matR);
            Cmat matW_HHRinv(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
            matW_HHRinv = GetInvertMatrix(matW_HHR);
            Cmat matW(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
            matW = MulMatrix(matW_HHRinv, matHtildeConjT);
            // WH~
            Cmat matWHtilde(LTE_DEFAULT_DCOMP,4); // as 2x2 matrix
            matWHtilde = MulMatrix(matW, matHtilde);

            double Psignal = 0.0;
            double Pself = 0.0;
            double Pnoise = 0.0;

            for (int tbIndex = 0; tbIndex < 2; ++tbIndex)
            {
                // Psignal
                Psignal = norm(matWHtilde[MATRIX_INDEX(tbIndex,tbIndex,2)])
                    * txPower_mW;

                // Pself
                Pself =
                    norm(matWHtilde[MATRIX_INDEX(tbIndex, 1 - tbIndex, 2)])
                    * txPower_mW;

                // Pnoise
                Pnoise = norm(matW[MATRIX_INDEX(tbIndex,0,2)])
                    * (phyLte->rbNoisePower_mW + ifPower_mW)
                    + norm(matW[MATRIX_INDEX(tbIndex,1,2)])
                    * (phyLte->rbNoisePower_mW + ifPower_mW);

                sinr[tbIndex] = Psignal / (Pself + Pnoise);
            }
        }
    }

    return sinr;
}

// /**
// FUNCTION   :: PhyLteSetPropagationDelay
// LAYER      :: PHY
// PURPOSE    :: Calculate propagation delay of each EU.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + propTxInfo   : PropTxInfo*  : message of propTx.
// RETURN     :: void : NULL
// **/
void PhyLteSetPropagationDelay(Node* node,
                               int phyIndex,
                               PropTxInfo* propTxInfo)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    phyLte->propagationDelay =
        node->getNodeTime() - propTxInfo->txStartTime;

#if LTE_LAYER2_DEFAULT_USE_SPECIFIED_DELAY
    phyLte->propagationDelay += LTE_LAYER2_DEFAULT_DELAY_UNTIL_AIRBORN;
#endif

}


// /**
// FUNCTION   :: PhyLteSetUlPathloss
// LAYER      :: PHY
// PURPOSE    :: Calculate filtered UL pathloss at eNB.
// PARAMETERS ::
// + node         : Node*         : Pointer to node.
// + phyIndex     : int           : Index of the PHY
// + target       : LteRnti       : RNTI of Tx-node.
// + propTxInfo   : PropTxInfo*   : Propagation tx info.
// + lteTxInfo    : PhyLteTxInfo* : LTE transmission info.
// + pathloss_dB : double        : Pathloss in dB.
// RETURN     ::  BOOL    : when true, successful of get data.
// **/
BOOL PhyLteSetUlPathloss(Node* node,
                                int phyIndex,
                                LteRnti target,
                                PropTxInfo* propTxInfo,
                                PhyLteTxInfo* lteTxInfo,
                                double pathloss_dB)
{
    BOOL ret = FALSE;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator it;
    it = phyLte->ueInfoList->find(target);
    if (it != phyLte->ueInfoList->end())
    {
        int matrixSize = phyLte->numRxAntennas * lteTxInfo->numTxAnntena;
        Cmat matHhat(LTE_DEFAULT_DCOMP,matrixSize);

        PhyLteGetChannelMatrix(node,
                                 phyIndex,
                                 phyLte,
                                 propTxInfo,
                                 lteTxInfo,
                                 pathloss_dB,
                                 matHhat);

        LteLayer3Filtering* lteFiltering
            = LteLayer2GetLayer3Filtering(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex);
#ifdef LTE_LIB_LOG
        std::ostringstream instant;
        std::ostringstream filtering;
#endif
        // If this is the first time to register DL pathloss to
        // filtering module, registered value will be the
        // ideal average pathloss
        double currentInstantPathloss;
        BOOL exists =
                lteFiltering->get(target,
                                  LTE_LIB_UL_PATHLOSS_RX0,
                                  &currentInstantPathloss);

        int i;
        for (i = 0; i < phyLte->numRxAntennas; i++)
        {
            // Always use tx antenna #0 for UE

            double ulInstantPathloss;

            if (exists == TRUE)
            {
#if PHY_LTE_USE_IDEAL_PATHLOSS_FOR_FILTERING
                ulInstantPathloss = NON_DB( pathloss_dB );
#else
                ulInstantPathloss =
                    1.0 / norm(matHhat[MATRIX_INDEX(
                                            i,0,lteTxInfo->numTxAnntena)]);
#endif
            }else
            {
                ulInstantPathloss = NON_DB(pathloss_dB);
            }

            if (i == 0)
            {
                lteFiltering->update(target,
                                     LTE_LIB_UL_PATHLOSS_RX0,
                                     IN_DB(ulInstantPathloss),
                                     phyLte->pathlossFilterCoefficient);
#ifdef LTE_LIB_LOG
                instant << IN_DB(ulInstantPathloss) << ",";
                double filteredPathloss1_dB = LTE_NEGATIVE_INFINITY_SINR_dB;
                lteFiltering->get(target,
                                  LTE_LIB_UL_PATHLOSS_RX0,
                                  &filteredPathloss1_dB);
                filtering << filteredPathloss1_dB << ",";
#endif
            }
            else if (i == 1)
            {
                lteFiltering->update(target,
                                     LTE_LIB_UL_PATHLOSS_RX1,
                                     IN_DB(ulInstantPathloss),
                                     phyLte->pathlossFilterCoefficient);
#ifdef LTE_LIB_LOG
                instant << IN_DB(ulInstantPathloss);
                double filteredPathloss2_dB = LTE_NEGATIVE_INFINITY_SINR_dB;
                lteFiltering->get(target,
                                  LTE_LIB_UL_PATHLOSS_RX1,
                                  &filteredPathloss2_dB);
                filtering << filteredPathloss2_dB;
#endif
            }
            it->second.ulInstantPathloss_dB.at(i) = IN_DB(ulInstantPathloss);
        }
#ifdef LTE_LIB_LOG
        LteRnti txRnti = LteRnti(propTxInfo->txNodeId,
            LteGetMacInterfaceIndexFromPhyIndex(
            propTxInfo->txNode, propTxInfo->phyIndex));
        lte::LteLog::DebugFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,Rnti=,"LTE_STRING_FORMAT_RNTI
            ",Instant_dB=,%s,Filterd_dB=,%s",
            LTE_STRING_CATEGORY_TYPE_UL_PATHLOSS,
            target.nodeId, target.interfaceIndex,
            instant.str().c_str(),
            filtering.str().c_str());
#endif

        ret = TRUE;
    }

    return ret;
}

// /**
// FUNCTION   :: PhyLteSetDlPathloss
// LAYER      :: PHY
// PURPOSE    :: Calculate filtered DL pathloss at eNB.
// PARAMETERS ::
// + node         : Node*         : Pointer to node.
// + phyIndex     : int           : Index of the PHY
// + target       : LteRnti       : RNTI of Tx-node.
// + propTxInfo   : PropTxInfo*   : Propagation tx info.
// + lteTxInfo    : PhyLteTxInfo* : LTE transmission info.
// + pathloss_dB : double        : Pathloss in dB.
// RETURN     ::  BOOL    : when true, successful of get data.
// **/
BOOL PhyLteSetDlPathloss(Node* node,
                                int phyIndex,
                                LteRnti target,
                                PropTxInfo* propTxInfo,
                                PhyLteTxInfo* lteTxInfo,
                                double pathloss_dB)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    /**
     *  Calculate the channel matrix
     */

    if (FALSE == PhyLteCheckRxDisconnected(node, phyIndex, phyLte, target))
    {

        int matrixSize = phyLte->numRxAntennas * lteTxInfo->numTxAnntena;
        Cmat channelMatrix(LTE_DEFAULT_DCOMP, matrixSize);
        PhyLteGetChannelMatrix(node,
                                phyIndex,
                                phyLte,
                                propTxInfo,
                                lteTxInfo,
                                pathloss_dB,
                                channelMatrix);


        LteLayer3Filtering* lteFiltering
                        = LteLayer2GetLayer3Filtering(
                            node,
                            thisPhy->macInterfaceIndex);

        // If this is the first time to register DL pathloss to
        // filtering module, registered value will be the
        // ideal average pathloss
        double currentInstantPathloss;
        BOOL exists =
                lteFiltering->get(target,
                                  LTE_LIB_DL_PATHLOSS_RX0,
                                  &currentInstantPathloss);

        int index = MATRIX_INDEX(0,0,lteTxInfo->numTxAnntena);

        double instantPathloss;

        if (exists == TRUE)
        {
#if PHY_LTE_USE_IDEAL_PATHLOSS_FOR_FILTERING
            instantPathloss = NON_DB(pathloss_dB);
#else
            instantPathloss = 1.0 / norm(channelMatrix[index]);
#endif
        }else
        {
            // First time
            instantPathloss = NON_DB(pathloss_dB);
        }

        lteFiltering->update(
            target,
            LTE_LIB_DL_PATHLOSS_RX0,
            IN_DB(instantPathloss),
            phyLte->pathlossFilterCoefficient);

#ifdef LTE_LIB_LOG
        double filteredPathloss_dB;
        lteFiltering->get(
                    target, LTE_LIB_DL_PATHLOSS_RX0, &filteredPathloss_dB);
        lte::LteLog::DebugFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,Rnti=,"LTE_STRING_FORMAT_RNTI
            ",Instant_dB=,%e,Filterd_dB=,%e",
            LTE_STRING_CATEGORY_TYPE_DL_PATHLOSS,
            target.nodeId, target.interfaceIndex,
            IN_DB(instantPathloss),
            filteredPathloss_dB);
#endif
    }

    return TRUE;
}


// /**
// FUNCTION   :: PhyLteRrcConnectedNotification
// LAYER      :: PHY
// PURPOSE    :: Notify PHY layer of RRC connected complete.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + handingover    : BOOL  : whether handingover
// RETURN     :: void : NULL
// **/
void PhyLteRrcConnectedNotification(Node* node, int phyIndex,
                                    BOOL handingover)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    if (handingover == TRUE)
    {
        phyLte->rrcReconfigCompleteFlag = TRUE;
    }
    else
    {
        phyLte->rrcSetupCompleteFlag = TRUE;
    }
}


#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
void PhyLteAddStatsForAllOfConnectedNodes(
        Node* node, const int phyIndex,
        std::map < LteRnti, lte::LogLteAverager > &enabledStats,
        const LteRnti* key = NULL)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*) thisPhy->phyVar;

    std::vector < lte::ConnectionInfo > ciList;

    lte::LteLog::getConnectionInfoConnectingWithMe(
                node->nodeId,
                phyLte->stationType == LTE_STATION_TYPE_ENB,
                ciList);

    // Add UEs existing in ciList and not existing in enabledStats

    if (!key)
    {
        for (int i = 0; i < ciList.size(); ++i){
            LteRnti oppositeRnti = LteRnti(ciList[i].nodeId,0);
            if (enabledStats.find(oppositeRnti)
                    == enabledStats.end())
            {
                enabledStats.insert(
                                std::pair < LteRnti, lte::LogLteAverager >
                                (oppositeRnti,lte::LogLteAverager()));
            }
        }
    }else
    {
        enabledStats.insert(std::pair < LteRnti, lte::LogLteAverager >
                                    (*key, lte::LogLteAverager()));
    }
}
#endif
#endif

// /**
// FUNCTION   :: PhyLteInit
// LAYER      :: PHY
// PURPOSE    :: Initialize the LTE PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to the node input
// RETURN     :: void             : NULL
// **/
void PhyLteInit(Node* node,
                const int phyIndex,
                const NodeInput* nodeInput)
{
    PhyDataLte* phyLte;
    int memSize;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    phyLte = (PhyDataLte*) MEM_malloc(sizeof(PhyDataLte));
    ERROR_Assert(phyLte!= NULL, "LTE: Out of memory!");
    memset(phyLte, 0, sizeof(PhyDataLte));

    phyLte->nodeInput = nodeInput;

    // set pointer in upper structure
    node->phyData[phyIndex]->phyVar = (void*)phyLte;

    // get a reference pointer to upper structure
    phyLte->thisPhy = node->phyData[phyIndex];

    // setup station type
    PhyLteSetStationType(node,
                         phyIndex,
                         LteLayer2GetStationType(node, interfaceIndex));

    // Initialize the antenna model
    ANTENNA_Init(node, phyIndex, nodeInput);

    // initialize parameters
    PhyLteInitConfigurableParameters(node, phyIndex, nodeInput);
    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        if (PHY_IsListeningToChannel(node, phyIndex, phyLte->dlChannelNo)
            == TRUE)
        {
            PHY_StopListeningToChannel(node, phyIndex, phyLte->dlChannelNo);
        }
    }
    else
    {
        if (PHY_IsListeningToChannel(node, phyIndex, phyLte->ulChannelNo)
            == TRUE)
        {
            PHY_StopListeningToChannel(node, phyIndex, phyLte->ulChannelNo);
        }
    }



    // Initialize status and other variables
    phyLte->txState = PHY_LTE_POWER_OFF;
    phyLte->rxState = PHY_LTE_POWER_OFF;
    phyLte->previousTxState = PHY_LTE_POWER_OFF;
    phyLte->previousRxState = PHY_LTE_POWER_OFF;

    phyLte->rbNoisePower_mW =
        phyLte->thisPhy->noise_mW_hz *
        PHY_LTE_NUM_SUBCARRIER_PER_RB *
        PHY_LTE_DELTAFREQUENCY_HZ *
        NON_DB(PHY_LTE_IMPLEMENTATION_LOSS);

#ifdef LTE_LIB_LOG
            lte::LteLog::DebugFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,mW=,%e",
                LTE_STRING_CATEGORY_TYPE_RB_THERMAL_NOISE,
                phyLte->rbNoisePower_mW);
#endif

    memSize = sizeof(double) * phyLte->numResourceBlocks;
    phyLte->interferencePower_mW = (double*) MEM_malloc(memSize);
    ERROR_Assert(phyLte->interferencePower_mW != NULL,
                 "PHYLTE: Out of memory!");
    memset(phyLte->interferencePower_mW, 0, memSize);

#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
    phyLte->filteredInterferencePower =
            new LteExponentialMean[phyLte->numResourceBlocks];
    ERROR_Assert(phyLte->filteredInterferencePower != NULL,
                 "PHYLTE: Out of memory!");
#endif

    phyLte->txEndMsgList = new std::list < Message* >;
    phyLte->eventMsgList = new std::map < int, Message* >;

    phyLte->rxPackMsgList = new std::list < PhyLteRxMsgInfo* >;
    phyLte->ifPackMsgList = new std::list < PhyLteRxMsgInfo* >;
    phyLte->rxMsgErrorStat = new std::map < Message*, PhyLteTbsInfo >;

    phyLte->establishmentData = new PhyLteEstablishmentData;

    phyLte->establishmentData->rxRrcConfigList
                        = new std::map < LteRnti, LteRrcConfig >;
    phyLte->establishmentData->rxPreambleList
                        = new std::list < PhyLteReceivingRaPreamble >;
    phyLte->establishmentData->selectedRntieNB = LTE_INVALID_RNTI;
    phyLte->establishmentData->isDetectingSelectedEnb = FALSE;
    phyLte->establishmentData->measureIntraFreq = FALSE;
    phyLte->establishmentData->intervalSubframeNum_intraFreq
                        = PHY_LTE_DEFAULT_INTERVAL_SUBFRAME_NUM_INTRA_FREQ;
    phyLte->establishmentData->offsetSubframe_intraFreq
                        = PHY_LTE_DEFAULT_OFFSET_SUBFRAME_INTRA_FREQ;
    phyLte->establishmentData->measureInterFreq = FALSE;
    phyLte->establishmentData->filterCoefRSRP
                        = PHY_LTE_DEFAULT_FILTER_COEF_RSRP;
    phyLte->establishmentData->filterCoefRSRQ
                        = PHY_LTE_DEFAULT_FILTER_COEF_RSRQ;

    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        phyLte->ueInfoList = new std::map < LteRnti, PhyLteConnectedUeInfo >;

        phyLte->raGrantsToSend = new std::set < LteRnti >;

        // NULL reset
        phyLte->rxDciForUlInfo = NULL;

    }
    else // LTE_STATION_TYPE_UE
    {
        phyLte->rxDciForUlInfo = new std::list < PhyLteRxDciForUlInfo >;

        // NULL reset
        phyLte->ueInfoList = NULL;

        // NULL reset
        phyLte->raGrantsToSend = NULL;
    }

    {
        // init buffer for MeasurementReport to send
        phyLte->rrcMeasurementReportList = new std::list<MeasurementReport>;
        // init flag of send rrcConnReconf message to FALSE
        phyLte->rrcConnReconfMap =
            new RrcConnReconfInclMoblityControlInfomationMap();
    }

    // set the default transmission channel & setconfig
    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        PHY_SetTransmissionChannel(node, phyIndex, phyLte->dlChannelNo);
        PhyLteSetRrcConfig(node, phyIndex);
    }
    else
    {
        PHY_SetTransmissionChannel(node, phyIndex, phyLte->ulChannelNo);
    }

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
                              node->phyData[phyIndex]->macInterfaceIndex,
                              "PHY:DEBUG:PhyLteInit"
                              ,"%d,%d,%d,%d",
                              //,"%d,%d,%d,%d,%p",
                              phyLte->txState,
                              phyLte->rxState,
                              phyLte->previousTxState,
                              phyLte->previousRxState);
#endif // LTE_LIB_LOG

    // Calculate transmission signal duration
    clocktype ttiInterval_ns = MacLteGetTtiLength(node, interfaceIndex);
    phyLte->txDuration = (clocktype)
                        (ttiInterval_ns - PHY_LTE_SIGNAL_DURATION_REDUCTION);

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
                              node->phyData[phyIndex]->macInterfaceIndex,
                              "PHY:DEBUG:PhyLteInit:ttiInterval_ns",
                              "%lld,%lld,%lf,%lld",
                              phyLte->rxTxTurnaroundTime,
                              ttiInterval_ns,
                              PHY_LTE_CP_LENGTH_NS,
                              phyLte->txDuration);
#endif // LTE_LIB_LOG

    // Initialize LTE statistics
    phyLte->stats.totalRxInterferenceSignals = 0;
    phyLte->stats.totalRxTbsToMac            = 0;
    phyLte->stats.totalRxTbsWithErrors       = 0;
    phyLte->stats.totalSignalsLocked         = 0;
    phyLte->stats.totalTxSignals             = 0;
    phyLte->stats.totalBitsToMac             = 0;

#ifdef ADDON_DB
    if (phyLte->thisPhy->phyStats)
    {
        phyLte->thisPhy->stats = new STAT_PhyStatistics(node,
                                                        PHY_NOLOCKING);
    }

    StatsDb* db = node->partitionData->statsDb;

    // For phy summary table
    if (db != NULL
        && (db->statsSummaryTable->createPhySummaryTable
            || db->statsAggregateTable->createPhyAggregateTable))
    {
        node->phyData[phyIndex]->oneHopData =
            new std::vector<PhyOneHopNeighborData>();
    }
#endif

#ifdef LTE_LIB_LOG
    phyLte->aggregator = new lte::Aggregator();
#endif

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    phyLte->avgReceiveSinr = new std::map < LteRnti, lte::LogLteAverager >;
    phyLte->avgMcs = new std::map < LteRnti, lte::LogLteAverager >;
    phyLte->bler = new std::map < LteRnti, lte::LogLteAverager >;
    phyLte->totalReceivedBits =
                            new std::map < LteRnti, lte::LogLteAverager >;

    PhyLteAddStatsForAllOfConnectedNodes(
                                node, phyIndex, *phyLte->avgReceiveSinr);
    PhyLteAddStatsForAllOfConnectedNodes(node, phyIndex, *phyLte->avgMcs);
    PhyLteAddStatsForAllOfConnectedNodes(node, phyIndex, *phyLte->bler);
    PhyLteAddStatsForAllOfConnectedNodes(
                                node, phyIndex, *phyLte->totalReceivedBits);

    // For UE only
    phyLte->avgTxPower = NULL;

    if (phyLte->stationType == LTE_STATION_TYPE_UE)
    {
        phyLte->avgTxPower = new std::map < LteRnti, lte::LogLteAverager >;
        PhyLteAddStatsForAllOfConnectedNodes(
            node,
            phyIndex,
            *phyLte->avgTxPower,
            &LteRnti(node->nodeId,
            LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex)));
    }
#endif
#endif
}

#ifdef LTE_LIB_LOG
void PhyLteDebugOutputAggregationLog(Node* node,
                                     int phyIndex,
                                     lte::Aggregator::ValueType valueType,
                                     const char* logName)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*) thisPhy->phyVar;

    std::vector < LteRnti > keys;

    keys = phyLte->aggregator->getKeys(valueType);

    for (int i = 0; i < keys.size(); ++i)
    {
        char buf[MAX_STRING_LENGTH];
        lte::LteLog::InfoFormat(node,
                node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,Rnti=,%s,%s",
                logName,
                STR_RNTI(buf, keys[i]),
                phyLte->aggregator->toString(keys[i],valueType).c_str());
    }
}
#endif

// /**
// FUNCTION   :: PhyLteFinalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the LTE PHY, print out statistics
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : const int : Index of the PHY
// RETURN     :: void      : NULL
// **/
void PhyLteFinalize(Node* node, const int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*) thisPhy->phyVar;

    char buf[MAX_STRING_LENGTH];

    if (thisPhy->phyStats == FALSE)
    {
        return;
    }

    sprintf(buf, "Signals transmitted = %d",
            phyLte->stats.totalTxSignals);
    IO_PrintStat(node, "Physical", "LTE", ANY_DEST, phyIndex, buf);

    sprintf(buf, "Signals locked on by PHY = %d",
            phyLte->stats.totalSignalsLocked);
    IO_PrintStat(node, "Physical", "LTE", ANY_DEST, phyIndex, buf);

    sprintf(buf, "Transport blocks received and forwarded to MAC = %d",
            phyLte->stats.totalRxTbsToMac);
    IO_PrintStat(node, "Physical", "LTE", ANY_DEST, phyIndex, buf);

    sprintf(buf, "Transport blocks received but with errors = %d",
            phyLte->stats.totalRxTbsWithErrors);
    IO_PrintStat(node, "Physical", "LTE", ANY_DEST, phyIndex, buf);

    sprintf(buf, "Interference signals received = %d",
            phyLte->stats.totalRxInterferenceSignals);
    IO_PrintStat(node, "Physical", "LTE", ANY_DEST, phyIndex, buf);

    sprintf(buf, "Total bits sent to MAC = %" TYPES_64BITFMT "u",
            phyLte->stats.totalBitsToMac);
    IO_PrintStat(node, "Physical", "LTE", ANY_DEST, phyIndex, buf);

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2(node,
                        thisPhy->macInterfaceIndex,
                        "PHY:DEBUG","PhyLteFinalize");
#endif // LTE_LIB_LOG


#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(node,
                node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteFinalize"
                ,"%d,%d,%p,%p,",
                phyLte->rxPackMsgList->size(),
                phyLte->ifPackMsgList->size(),
                phyLte->interferencePower_mW[0],
                phyLte);
#endif // LTE_LIB_LOG


#ifdef LTE_LIB_LOG

    // Output PDF of SINR for TB
    //---------------------------
    PhyLteDebugOutputAggregationLog(node, phyIndex,
        lte::Aggregator::TB_SINR,
        LTE_STRING_CATEGORY_TYPE_TB_SINR_PDF);

    // Output PDF of DL estimated SINR for TB
    //----------------------------------------
    PhyLteDebugOutputAggregationLog(node, phyIndex,
        lte::Aggregator::DL_ESTIMATED_TB_SINR,
        LTE_STRING_CATEGORY_TYPE_DL_ESTIMATED_TB_SINR_PDF);

    // Output PDF of SINR for TB when calculating CQI
    //-----------------------------------------------
    PhyLteDebugOutputAggregationLog(node, phyIndex,
        lte::Aggregator::CQI_TB_SINR,
        LTE_STRING_CATEGORY_TYPE_CQI_TB_SINR_PDF);

    // Output PDF of UL estimated SINR for TB
    //----------------------------------------
    PhyLteDebugOutputAggregationLog(node, phyIndex,
        lte::Aggregator::UL_ESTIMATED_TB_SINR,
        LTE_STRING_CATEGORY_TYPE_UL_ESTIMATED_TB_SINR_PDF);

    // Output PDF of CQI
    //--------------------
    PhyLteDebugOutputAggregationLog(node, phyIndex,
        lte::Aggregator::CQI,
        LTE_STRING_CATEGORY_TYPE_CQI_PDF);

    // Output PDF of DL_MCS (eNB only)
    //--------------------------------
    PhyLteDebugOutputAggregationLog(node, phyIndex,
        lte::Aggregator::DL_MCS,
        LTE_STRING_CATEGORY_TYPE_DL_MCS_PDF);

    // Output PDF of UL_MCS (eNB only)
    //--------------------------------
    PhyLteDebugOutputAggregationLog(node, phyIndex,
        lte::Aggregator::UL_MCS,
        LTE_STRING_CATEGORY_TYPE_UL_MCS_PDF);

    // Output PDF of PPER
    //--------------------------------
    PhyLteDebugOutputAggregationLog(node, phyIndex,
        lte::Aggregator::PPER,
        LTE_STRING_CATEGORY_TYPE_PPER_PDF);

    delete phyLte->aggregator;
#endif

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    std::map < LteRnti, lte::LogLteAverager > ::iterator it;


    // AvgReceiveSinr
    it = phyLte->avgReceiveSinr->begin();
    for (; it != phyLte->avgReceiveSinr->end(); ++it)
    {
        lte::LteLog::InfoFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,%d,%e",
                "AvgReceiveSinr",
                it->first.nodeId,
                it->second.get());
    }

    delete phyLte->avgReceiveSinr;

    // AvgTxPower
    if (phyLte->avgTxPower)
    {
        it = phyLte->avgTxPower->begin();
        for (; it != phyLte->avgTxPower->end(); ++it)
        {
            double avgTxPower_dB = it->second.get() <= 0.0 ?
                                            LTE_NEGATIVE_INFINITY_POWER_dBm :
                                            it->second.get();

            lte::LteLog::InfoFormat(
                    node, node->phyData[phyIndex]->macInterfaceIndex,
                    LTE_STRING_LAYER_TYPE_PHY,
                    "%s,%e",
                    "AvgTxPower",
                    avgTxPower_dB);
        }

        delete phyLte->avgTxPower;
    }

    // AvgMcs
    it = phyLte->avgMcs->begin();
    for (; it != phyLte->avgMcs->end(); ++it)
    {
        lte::LteLog::InfoFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,%d,%e",
                "AvgMcs",
                it->first.nodeId,
                it->second.get());
    }

    delete phyLte->avgMcs;

    // Bler
    it = phyLte->bler->begin();
    for (; it != phyLte->bler->end(); ++it)
    {
        lte::LteLog::InfoFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,%d,%e",
                "Bler",
                it->first.nodeId,
                it->second.get());
    }

    delete phyLte->bler;


    // PHY throughput
    it = phyLte->totalReceivedBits->begin();
    double simTime = node->getNodeTime()/(double)SECOND
                                - lte::LteLog::getValidationStatOffset();

    if (simTime > 0.0)
    {
        for (;it != phyLte->totalReceivedBits->end(); ++it)
        {
            lte::LteLog::InfoFormat(
                    node, node->phyData[phyIndex]->macInterfaceIndex,
                    LTE_STRING_LAYER_TYPE_PHY,
                    "%s,%d,%e",
                    "PhyThroughput",
                    it->first.nodeId,
                    (double)(it->second.getSum() / simTime));
        }
    }

    delete phyLte->totalReceivedBits;


#endif
#endif


    delete phyLte->ifPackMsgList;
    delete phyLte->rxPackMsgList;
    delete phyLte->txEndMsgList;
    delete phyLte->eventMsgList;


    if (phyLte->ueInfoList)
    {
        delete phyLte->ueInfoList;
    }

    if (phyLte->rxDciForUlInfo)
    {
        delete phyLte->rxDciForUlInfo;
    }

    if (phyLte->rrcMeasurementReportList)
    {
        delete phyLte->rrcMeasurementReportList;
    }

    if (phyLte->rrcConnReconfMap)
    {
        delete phyLte->rrcConnReconfMap;
    }

    if (phyLte->establishmentData)
    {
        if (phyLte->establishmentData->rxPreambleList)
        {
            phyLte->establishmentData->rxPreambleList->clear();
            delete phyLte->establishmentData->rxPreambleList;
        }
        if (phyLte->establishmentData->rxRrcConfigList)
        {
            phyLte->establishmentData->rxRrcConfigList->clear();
            delete phyLte->establishmentData->rxRrcConfigList;
        }

        delete phyLte->establishmentData;
    }

    if (phyLte->raGrantsToSend)
    {
        delete phyLte->raGrantsToSend;
    }

#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
    if (phyLte->filteredInterferencePower)
    {
        delete [] phyLte->filteredInterferencePower;
    }
#endif

    MEM_free(phyLte->interferencePower_mW);
    MEM_free(phyLte->dlCqiSnrTable);
    MEM_free(phyLte);
}

// /**
// FUNCTION   :: PhyLteNotifyPowerOff
// LAYER      :: PHY
// PURPOSE    :: Notify PHY layer of power off.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// RETURN     :: void             : NULL
// **/
void PhyLteNotifyPowerOff(Node* node, const int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*) thisPhy->phyVar;

    // Cancel the receiving signal

    PhyLteTerminateCurrentReceive(node,
                                  phyIndex);

    // Cancel the TransmissionEnd messages
    PhyLteTerminateCurrentTransmissions(node, phyIndex);

    // Cancel the event timers
    std::map < int, Message* > ::iterator it;
    for (it = phyLte->eventMsgList->begin();
         it != phyLte->eventMsgList->end();
         it++)
    {
        MESSAGE_CancelSelfMsg(node, it->second);
        it->second = NULL;
    }
    phyLte->eventMsgList->clear();

    // Clear the lists
    phyLte->ifPackMsgList->clear();
    phyLte->rxPackMsgList->clear();
    if (phyLte->rxDciForUlInfo)
    {
        phyLte->rxDciForUlInfo->clear();
    }

    if (phyLte->establishmentData->rxPreambleList)
    {
        phyLte->establishmentData->rxPreambleList->clear();
    }
    if (phyLte->establishmentData->rxRrcConfigList)
    {
        phyLte->establishmentData->rxRrcConfigList->clear();
    }
    phyLte->establishmentData->isDetectingSelectedEnb = FALSE;
    phyLte->establishmentData->selectedRntieNB = LTE_INVALID_RNTI;

    // For eNB
    if (phyLte->ueInfoList)
    {
        phyLte->ueInfoList->clear();
    }

    if (phyLte->raGrantsToSend)
    {
        phyLte->raGrantsToSend->clear();
    }

    // Change state
    PhyLteChangeState(node,
                      phyIndex,
                      phyLte,
                      PHY_LTE_POWER_OFF);
}

// /**
// FUNCTION   :: PhyLteNotifyPowerOn
// LAYER      :: PHY
// PURPOSE    :: Notify PHY layer of power on.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// RETURN     :: void             : NULL
// **/
void PhyLteNotifyPowerOn(Node* node, const int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    int memSize = sizeof(double) * phyLte->numResourceBlocks;
    memset(phyLte->interferencePower_mW, 0, memSize);

    phyLte->establishmentData->selectedRntieNB = LTE_INVALID_RNTI;

    // set the default transmission channel & setconfig
    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        PHY_SetTransmissionChannel(node, phyIndex, phyLte->dlChannelNo);
        PhyLteSetRrcConfig(node, phyIndex);
        PhyLteSetCheckingConnectionTimer(node, phyIndex);
    }
    else
    {
        PHY_SetTransmissionChannel(node, phyIndex, phyLte->ulChannelNo);
    }

#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
        PhyLteSetInterferenceMeasurementTimer(node, phyIndex);
#endif

    // Calculate transmission signal duration.
    clocktype ttiInterval_ns = MacLteGetTtiLength(node, interfaceIndex);
    phyLte->txDuration = (clocktype)
                        (ttiInterval_ns - PHY_LTE_SIGNAL_DURATION_REDUCTION);

    // Initialization of CQI values (UE only)
    phyLte->nextReportedCqiInfo.cqi0 = PHY_LTE_INVALID_CQI;
    phyLte->nextReportedCqiInfo.cqi1 = PHY_LTE_INVALID_CQI;
    phyLte->nextReportedRiInfo = PHY_LTE_INVALID_RI;

    // Change state
    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        PhyLteChangeState(node,
                          phyIndex,
                          phyLte,
                          PHY_LTE_DL_IDLE);
        PhyLteChangeState(node,
                          phyIndex,
                          phyLte,
                          PHY_LTE_UL_IDLE);
    }
    else
    {
        PhyLteChangeState(node,
                          phyIndex,
                          phyLte,
                          PHY_LTE_IDLE_CELL_SELECTION);
    }
}

// /**
// FUNCTION   :: PhyLteLessRxSensitivity
// LAYER      :: PHY
// PURPOSE    :: Check the reception signals sensitivity.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + phyLte       : PhyDataLte*  : PHY data structure
// + rxPower_mW   : double       : Rx-signals power
// RETURN :: BOOL : when true, Greater than or equal to receive sensitivity
// **/
BOOL PhyLteLessRxSensitivity(Node* node,
                             int phyIndex,
                             PhyDataLte* phyLte,
                             double rxPower_mW)
{
    if (rxPower_mW >= phyLte->rxSensitivity_mW[0])
    {
        return FALSE;
    }
    else
    {

        return TRUE;
    }
}

// /**
// FUNCTION   :: PhyLteJudgeTxScheme
// LAYER      :: PHY
// PURPOSE    :: Judge transmission scheme
//               from the DCI information on the message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + phyIndex  : int          : Index of the PHY
// + msg       : Message*     : Message structure to be judged
// RETURN     ::  LteTxScheme : Transmission scheme.
//                              TX_SCHEME_INVALID  if no tx scheme determined
// **/
LteTxScheme PhyLteJudgeTxScheme(Node* node,
                             int phyIndex,
                             Message* msg)
{
    char errBuf[MAX_STRING_LENGTH];

    PhyData* phyData = node->phyData[phyIndex];

    int transmissionMode =
        GetLteMacConfig(node, phyData->macInterfaceIndex)->transmissionMode;


    // DCI format-0?
    LteDciFormat0* Dci0Info =
        (LteDciFormat0*)MESSAGE_ReturnInfo(
                                msg, (unsigned short)INFO_TYPE_LteDciForUl);

    // DCI format-1?
    LteDciFormat1* Dci1Info =
        (LteDciFormat1*)MESSAGE_ReturnInfo(
                                msg, (unsigned short)INFO_TYPE_LteDci1Info);

    // DCI format-2a?
    LteDciFormat2a* Dci2aInfo =
        (LteDciFormat2a*)MESSAGE_ReturnInfo(
                                msg, (unsigned short)INFO_TYPE_LteDci2aInfo);


    if (Dci0Info)
    {
        ERROR_Assert(Dci1Info == NULL,
                    "Error: Dci0 and Dci1 exists on the same Message\n");
        ERROR_Assert(Dci2aInfo == NULL,
                    "Error: Dci0 and Dci2a exists on the same Message\n");

        return TX_SCHEME_SINGLE_ANTENNA;
    }


    switch (transmissionMode)
    {
        case 1:
            ERROR_Assert(Dci2aInfo == NULL,
                    "Error: Dci2a exists even if transmission mode is 1\n");
            ERROR_Assert(Dci1Info != NULL,
                    "Error: Dci1 does not exist. "
                    "Dci1 should be exist when transmission mode is 1\n");

            return TX_SCHEME_SINGLE_ANTENNA;

        case 2:
            ERROR_Assert(Dci2aInfo == NULL,
                    "Error: Dci2a exists even if transmission mode is 2\n");
            ERROR_Assert(Dci1Info != NULL,
                    "Error: Dci1 does not exist. "
                    "Dci1 should be exist when transmission mode is 2\n");

            return TX_SCHEME_DIVERSITY;

        case 3:
            if (Dci1Info)
            {
                return TX_SCHEME_DIVERSITY;
            }else if (Dci2aInfo)
            {
                // When msg is the message which includes 1st transport block
                return TX_SCHEME_OL_SPATIAL_MULTI;
            }else
            {
                // When msg is the message which includes 2nd transport block
                return TX_SCHEME_OL_SPATIAL_MULTI;
            }
        default:
            sprintf(errBuf,
                "Transmission mode %d is not supported. ", transmissionMode);
            ERROR_Assert(FALSE, errBuf);
    }

    return TX_SCHEME_INVALID;
}

// /**
// FUNCTION   :: LteSignalArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyLteSignalArrivalFromChannel(Node* node,
                                    int phyIndex,
                                    int channelIndex,
                                    PropRxInfo* propRxInfo)
{
    // Get signal type
    PhyLteTxInfo* lteTxInfo = PhyLteGetMessageTxInfo(node,
                                                     phyIndex,
                                                     propRxInfo->txMsg);
#ifdef LTE_LIB_LOG
    if (lteTxInfo != NULL){
        lte::LteLog::Debug2Format(node,
                                  node->phyData[phyIndex]->macInterfaceIndex,
                                  "PHY:DEBUG:PhyLteSignalArrivalFromChannel",
                                  "%d,%p,%p,%d",
                                  channelIndex,
                                  propRxInfo,
                                  propRxInfo->txMsg,
                                  lteTxInfo->txStationType);
    }
#endif // LTE_LIB_LOG

    // If received message is not LTE signal, do nothing.
    if (lteTxInfo == NULL)
    {
        return;
    }

#ifdef LTE_LIB_LOG
#if 0
    PhyLteDebugOutputMessage(
                node, phyIndex, propRxInfo->txMsg, "Arrival", propRxInfo);
#endif
#endif

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    if (phyLte->rxState == PHY_LTE_POWER_OFF)
    {
        return;
    }

#ifdef ADDON_DB
    if (phyLte->thisPhy->phyStats)
    {
        phyLte->thisPhy->stats->AddSignalDetectedDataPoints(node);
    }
#endif
    BOOL interference = FALSE;

    // Process signals related to establishment
    PhyLteSignalArrivalFromChannelInEstablishment(node,
                                                  phyIndex,
                                                  channelIndex,
                                                  propRxInfo);

    // If received channel is not the one "node" using for reception,
    // do nothing.
    if (((channelIndex != phyLte->ulChannelNo)
         && (phyLte->stationType == LTE_STATION_TYPE_ENB))
        || ((channelIndex != phyLte->dlChannelNo)
            && (phyLte->stationType == LTE_STATION_TYPE_UE)))
    {
        return;
    }

    // Check if transmission node type is same as reception node type.
    // If so, channel configuration is invalid.
    if (phyLte->stationType == lteTxInfo->txStationType)
    {
        ERROR_Assert(phyLte->stationType != lteTxInfo->txStationType,
            "Invalid channel configuration. "
            "DL and UL channel must not be the same over all of the subnets."
            " Please check channel configuration.");
    }

    // Process control messages
    PhyLteGetMessageControlInfo(node,
                                phyIndex,
                                phyLte,
                                propRxInfo,
                                TRUE);

    PropTxInfo* propTxInfo =
        (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);

#ifdef LTE_LIB_LOG
    // Channel response matrix log
    PhyLteDebugOutputChannelResponseLog(
        node, phyIndex, channelIndex, lteTxInfo, propTxInfo, propRxInfo);
#endif

    LteRnti target = LteRnti(propTxInfo->txNodeId,
            LteGetMacInterfaceIndexFromPhyIndex(
            propTxInfo->txNode, propTxInfo->phyIndex));

    // Calculate propagation delay only for UE
    if ((phyLte->stationType == LTE_STATION_TYPE_UE)
        && PhyLteIsInStationaryState(node, phyIndex) == true
        && (FALSE == PhyLteCheckRxDisconnected(node,
                                               phyIndex,
                                               phyLte,
                                               target)))
    {
        PhyLteSetPropagationDelay(node, phyIndex, propTxInfo);
    }

    // Calculate pathloss excluding fading.
    double pathloss_dB =
        PhyLteGetPathloss_dB(node, phyIndex, lteTxInfo, propRxInfo);

#ifdef LTE_LIB_LOG
        LteRnti txRnti = LteRnti(propTxInfo->txNodeId,
            LteGetMacInterfaceIndexFromPhyIndex(
            propTxInfo->txNode, propTxInfo->phyIndex));
        lte::LteLog::DebugFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,Rnti=,"LTE_STRING_FORMAT_RNTI
            ",Non_dB=,%e,dB=,%e",
            LTE_STRING_CATEGORY_TYPE_PATHLOSS,
            propTxInfo->txNodeId, propTxInfo->phyIndex,
            NON_DB(pathloss_dB),
            pathloss_dB);
#endif

    // The message with NoTransportBlockInfo and the message whose
    // payload size is 0 don't interfere in the desired signal.
    if ((PhyLteIsMessageNoTransportBlock(node,
                                         phyIndex,
                                         propRxInfo->txMsg)) ||
        ((!PhyLteIsMessageNoTransportBlock(node,
                                           phyIndex,
                                           propRxInfo->txMsg)) &&
         (MESSAGE_ReturnPacketSize(propRxInfo->txMsg) == 0) &&
         (MESSAGE_ReturnVirtualPacketSize(propRxInfo->txMsg) == 0)))
    {
#ifdef ADDON_DB
        // having some PHY control info
        if (phyLte->thisPhy->phyStats)
        {
            phyLte->thisPhy->stats->AddSignalLockedDataPoints(
                            node,
                            propRxInfo,
                            phyLte->thisPhy,
                            channelIndex,
                            NON_DB(lteTxInfo->txPower_dBm - pathloss_dB));
        }
#endif
        return;
    }

    PhyLteRxMsgInfo* newRxMsgInfo = NULL;
    newRxMsgInfo = PhyLteCreateRxMsgInfo(node,
                                         phyIndex,
                                         phyLte,
                                         propRxInfo,
                                         NON_DB(pathloss_dB),
                                         NON_DB(lteTxInfo->txPower_dBm));
    if (newRxMsgInfo == NULL)
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteSignalArrivalFromChannel:newRxMsgInfo is NULL",
            "");
#endif // LTE_LIB_LOG
        return;
    }

    newRxMsgInfo->rxArrivalTime = node->getNodeTime();
    newRxMsgInfo->geometry = 1.0 / NON_DB(pathloss_dB);
    newRxMsgInfo->txPower_mW = NON_DB(lteTxInfo->txPower_dBm);

    newRxMsgInfo->rxPower_mW =
        NON_DB(lteTxInfo->txPower_dBm - pathloss_dB);

    newRxMsgInfo->propTxInfo = *propTxInfo; // Copy
    newRxMsgInfo->lteTxInfo = *lteTxInfo;   // Copy

    // Check if the tx node is disconnected
    // (eNB:check connected UE, UE:check serving eNB)
    if (interference != TRUE)
    {
        interference = PhyLteCheckRxDisconnected(node,
                                                 phyIndex,
                                                 phyLte,
                                                 target);
    }

    // Check receive power is greater than or equal to
    // receive sensitivy
    if (interference != TRUE)
    {
        interference =
            PhyLteLessRxSensitivity(node,
                                    phyIndex,
                                    phyLte,
                                    newRxMsgInfo->rxPower_mW);
    }

    if (interference != TRUE)
    {
        interference = PhyLteNoOverlap(node, phyLte, newRxMsgInfo);
    }

#ifdef LTE_LIB_USE_ONOFF
    // If current state is not stationary, all the signals are treated
    // as interference signal. #2
    if (interference != TRUE)
    {
        interference = ! PhyLteIsInStationaryState(node, phyIndex);
    }
#endif

#ifdef LTE_LIB_LOG
    PhyLteDebugOutputIfPowerLog(node, phyIndex);
#endif

    // Process inteference signals
    if (interference)
    {
        // Check packet error
        // Note that error check must be performed before
        // adding interference power.
        PhyLteCheckAllOfRxPacketError(node, phyIndex);

        // Append new receive message to interference message list.
        phyLte->ifPackMsgList->push_back(newRxMsgInfo);

#ifdef LTE_LIB_LOG
#if 0
        PhyLteDebugOutputRxMsgInfoList(
                                    node, phyIndex, phyLte->ifPackMsgList);
#endif
#endif
        // Add interference power
        PhyLteAddInterferencePower(node, phyIndex, newRxMsgInfo);

        phyLte->stats.totalRxInterferenceSignals ++;
    }
    // Desired signals
    else
    {
        // Add received message to receiving message list.
        phyLte->rxPackMsgList->push_back(newRxMsgInfo);

        Message* rxMsg = newRxMsgInfo->rxMsg;
        Message* prevMsg = NULL;

        while (rxMsg != NULL)
        {
            // Retrieve DCI format 0
            LteDciFormat0* DciForUl =
                (LteDciFormat0*)MESSAGE_ReturnInfo(
                    rxMsg,
                    (unsigned short)INFO_TYPE_LteDci0Info);

            // Retrieve destination information
            MacLteMsgDestinationInfo* dstInfo =
                (MacLteMsgDestinationInfo*)MESSAGE_ReturnInfo(
                    rxMsg,
                (unsigned short)INFO_TYPE_LteMacDestinationInfo);

            ERROR_Assert(dstInfo != NULL,
                    "PHY-LTE: not detect INFO_TYPE_LteMacDestinationInfo!");

            // Whether this message is for me or not.
            bool isForMe = (dstInfo->dstRnti ==
                        LteLayer2GetRnti(node, thisPhy->macInterfaceIndex));

            if (isForMe == true){

                if (DciForUl != NULL)
                // DCI for UE
                {
                    ERROR_Assert(phyLte->stationType == LTE_STATION_TYPE_UE,
                                "DciForUl detected on eNB");

                    PhyLteSetDciForUl(node,
                                      phyIndex,
                                      propRxInfo,
                                      DciForUl);

                }
                // has transport blocks
                else
                {
                    // create new entry for error status of TB(rxMsg).
                    std::map < Message*, PhyLteTbsInfo > ::iterator it
                        = phyLte->rxMsgErrorStat->find(rxMsg);
                    if (it == phyLte->rxMsgErrorStat->end())
                    {
                        PhyLteTbsInfo tbsErrInfo;
                        tbsErrInfo.transportBlockSize = 0;
                        tbsErrInfo.isError = FALSE;
                        tbsErrInfo.rxTimeEvaluated =
                            newRxMsgInfo->rxArrivalTime;
                        phyLte->rxMsgErrorStat->insert(
                            std::pair < Message*, PhyLteTbsInfo >
                                    (rxMsg, tbsErrInfo));
                    }
                }
            }

            // next
            prevMsg = rxMsg;
            rxMsg = rxMsg->next;
        }
        phyLte->stats.totalSignalsLocked++;

#ifdef ADDON_DB
        if (phyLte->thisPhy->phyStats)
        {
            phyLte->thisPhy->stats->AddSignalLockedDataPoints(
                node,
                propRxInfo,
                phyLte->thisPhy,
                channelIndex,
                NON_DB(lteTxInfo->txPower_dBm - pathloss_dB));
        }
#endif
    }

    // State transition
    if (PhyLteIsInStationaryState(node, phyIndex) == true)
    {
        if (phyLte->stationType == LTE_STATION_TYPE_UE)
        {
            PhyLteChangeState(node, phyIndex, phyLte,
                              PHY_LTE_DL_FRAME_RECEIVING);
        }
        else // LTE_STATION_TYPE_ENB
        {
            PhyLteChangeState(node, phyIndex, phyLte,
                              PHY_LTE_UL_FRAME_RECEIVING);
        }
    }
}


// /**
// FUNCTION   :: PhyLteAddInterferencePower
// LAYER      :: PHY
// PURPOSE    :: Add interference power
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + rxMsgInfo    : int   : PhyLteRxMsgInfo structure of interference signal
// RETURN     :: void : NULL
// **/
void PhyLteAddInterferencePower(Node* node,
                                int phyIndex,
                                PhyLteRxMsgInfo* rxMsgInfo)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    // Add interference power
    Message* ifMsg = rxMsgInfo->rxMsg;

    while (ifMsg != NULL)
    {
        UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB];
        memset(usedRB_list, 0, sizeof(usedRB_list));
        BOOL hasDci = FALSE;

        // Retrieve RB allocation info
        LteDciFormat0* DciForUl =
            (LteDciFormat0*)MESSAGE_ReturnInfo(
                ifMsg,
                (unsigned short)INFO_TYPE_LteDci0Info);

       // Skip a DCI format 0 message
        if (DciForUl != NULL)
        {
            ifMsg = ifMsg->next;
            continue;
        }

        // Retrieve resource allocation infomation
        hasDci = PhyLteGetResourceAllocationInfo(
            node,
            phyIndex,
            ifMsg,
            usedRB_list,
            NULL);

        if (hasDci)
        {
            // Add interference power
            for (UInt8 i = 0; i < phyLte->numResourceBlocks; i++)
            {
                if (usedRB_list[i] != 0)
                {
                    phyLte->interferencePower_mW[i]
                        += rxMsgInfo->rxPower_mW;
                }
            }
        }

        ifMsg = ifMsg->next;
    }
}

// /**
// FUNCTION   :: PhyLteSubtractInterferencePower
// LAYER      :: PHY
// PURPOSE    :: Subtract interference power
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + rxMsgInfo    : int   : PhyLteRxMsgInfo structure of interference signal
// RETURN     :: void : NULL
// **/
void PhyLteSubtractInterferencePower(Node* node,
                                     int phyIndex,
                                     PhyLteRxMsgInfo* rxMsgInfo)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    // Subtract interference power
    Message* ifMsg = rxMsgInfo->rxMsg;
    while (ifMsg != NULL)
    {
        UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB];
        memset(usedRB_list, 0, sizeof(usedRB_list));
        BOOL hasDci = FALSE;

        // Retrieving RB allocation information
        LteDciFormat0* DciForUl =
            (LteDciFormat0*)MESSAGE_ReturnInfo(
                ifMsg,
                (unsigned short)INFO_TYPE_LteDci0Info);

        // Skip a message which has DciForUe
        if (DciForUl != NULL)
        {
            ifMsg = ifMsg->next;
            continue;
        }

        // Retrieve resource allocation infomation
        hasDci = PhyLteGetResourceAllocationInfo(
            node,
            phyIndex,
            ifMsg,
            usedRB_list,
            NULL);

        if (hasDci)
        {
            // Eliminate interference power
            for (UInt8 i = 0; i < phyLte->numResourceBlocks; i++)
            {
                if (usedRB_list[i] != 0)
                {
                    phyLte->interferencePower_mW[i]
                        -= rxMsgInfo->rxPower_mW;
                }
            }
        }

        ifMsg = ifMsg->next;

    }
}


// /**
// FUNCTION   :: LteSignalEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyLteSignalEndFromChannel(Node* node,
                                int phyIndex,
                                int channelIndex,
                                PropRxInfo* propRxInfo)
{
    PhyLteTxInfo* lteTxInfo = PhyLteGetMessageTxInfo(node,
                                                     phyIndex,
                                                     propRxInfo->txMsg);

    // If received message is not LTE signal, do nothing.
    if (lteTxInfo == NULL)
    {
        return;
    }

#ifdef LTE_LIB_LOG
#if 0
    PhyLteDebugOutputMessage(
                node, phyIndex, propRxInfo->txMsg, "EndFrom", propRxInfo);
#endif
#endif

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PropTxInfo* propTxInfo =
        (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);

    LteRnti target = LteRnti(propTxInfo->txNodeId,
            LteGetMacInterfaceIndexFromPhyIndex(
            propTxInfo->txNode, propTxInfo->phyIndex));

    if (phyLte->rxState == PHY_LTE_POWER_OFF)
    {
        return;
    }


#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(node,
                              node->phyData[phyIndex]->macInterfaceIndex,
                              "PHY:DEBUG:PhyLteSignalEndFromChannel"
                              ,"%d,%d,%p,%p,%d",
                              phyIndex,
                              channelIndex,
                              propRxInfo,
                              propRxInfo->txMsg,
                              lteTxInfo->txStationType);
#endif // LTE_LIB_LOG

    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        PhyLteSrsInfo srsInfo;

        if (PhyLteGetMessageSrsInfo(
            node, phyIndex, propRxInfo->txMsg, &srsInfo) == TRUE)
        {
            const LteRnti& dstRnti = srsInfo.dstRnti;
            const LteRnti& srcRnti =
                LteRnti(propRxInfo->txNodeId,
                    LteGetMacInterfaceIndexFromPhyIndex(
                    propTxInfo->txNode, propTxInfo->phyIndex));
            const LteRnti& ownRnti =
                LteLayer2GetRnti(node, thisPhy->macInterfaceIndex);
            std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator itr;
            itr = phyLte->ueInfoList->find(srcRnti);

            // If SRS from connected UE is received
            if (itr != phyLte->ueInfoList->end())
            {
                // If SRS is not for me,
                // source UE is assumed to connect another eNB.
                if (dstRnti != ownRnti)
                {
#ifdef LTE_LIB_LOG
                    lte::LteLog::InfoFormat(
                        node, node->phyData[phyIndex]->macInterfaceIndex,
                        LTE_STRING_LAYER_TYPE_PHY,
                        "%s,SRS,%s=,"LTE_STRING_FORMAT_RNTI,
                        LTE_STRING_CATEGORY_TYPE_LOST_DETECTION,
                        LTE_STRING_STATION_TYPE_UE,
                        srcRnti.nodeId, srcRnti.interfaceIndex);
#endif
                    Layer3LteNotifyLostDetection(
                        node, thisPhy->macInterfaceIndex, srcRnti);
                    return;
                }
                // If not, hold connection.
                else
                {
                    itr->second.isDetecting = TRUE;
                }
            }
        }
    }
    else
    {
        const LteRnti& srcRnti = LteRnti(propRxInfo->txNodeId,
                LteGetMacInterfaceIndexFromPhyIndex(
                propTxInfo->txNode, propTxInfo->phyIndex));

        // If downlink subframe from serving eNB is received,
        if (phyLte->establishmentData->selectedRntieNB == srcRnti)
        {
            if (phyLte->establishmentData->isDetectingSelectedEnb == FALSE)
            {
#ifdef LTE_LIB_LOG
                lte::LteLog::InfoFormat(
                    node, node->phyData[phyIndex]->macInterfaceIndex,
                    LTE_STRING_LAYER_TYPE_PHY,
                    "%s,Detecting,%s=,"LTE_STRING_FORMAT_RNTI,
                    LTE_STRING_CATEGORY_TYPE_LOST_DETECTION,
                    LTE_STRING_STATION_TYPE_ENB,
                    srcRnti.nodeId, srcRnti.interfaceIndex);
#endif
                phyLte->establishmentData->isDetectingSelectedEnb = TRUE;
            }
        }
    }

    // Process signals related to establishment
    PhyLteSignalEndFromChannelInEstablishment(node,
                                              phyIndex,
                                              channelIndex,
                                              propRxInfo);


    // If received channel is not the one "node" using for reception,
    // do nothing.
    if (((channelIndex != phyLte->ulChannelNo)
         && (phyLte->stationType == LTE_STATION_TYPE_ENB))
        || ((channelIndex != phyLte->dlChannelNo)
            && (phyLte->stationType == LTE_STATION_TYPE_UE)))
    {
        return;
    }

    // Check if transmission node type is same as reception node type.
    // If so, channel configuration is invalid.
    if (phyLte->stationType == lteTxInfo->txStationType)
    {
        ERROR_Assert(phyLte->stationType != lteTxInfo->txStationType,
            "Invalid channel configuration. "
            "DL and UL channel must not be the same over all of the subnets."
            " Please check channel configuration.");
    }

    // Process control messages
    PhyLteGetMessageControlInfo(node,
                                phyIndex,
                                phyLte,
                                propRxInfo,
                                FALSE);
#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(node,
                                  node->phyData[phyIndex]->macInterfaceIndex,
                                  "PHY:DEBUG:PhyLteSignalEndFromChannel"
                                  ,"%d,%p,%p",
                                  phyIndex,
                                  propRxInfo,
                                  propRxInfo->txMsg);
#endif // LTE_LIB_LOG


    // UL / DL instant pathloss measurements
    //--------------------------------------

    double pathloss_dB =
        PhyLteGetPathloss_dB(node, phyIndex, lteTxInfo, propRxInfo);


    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        // UL pathloss mesurement
        if (PhyLteIsMessageSrsInfo(node, phyIndex, propRxInfo->txMsg))
        {
            PhyLteSetUlPathloss(
                node,
                phyIndex,
                target,
                propTxInfo,
                lteTxInfo,
                pathloss_dB);
        }
    }else
    // UE
    {
        PhyLteSetDlPathloss(
            node,
            phyIndex,
            target,
            propTxInfo,
            lteTxInfo,
            pathloss_dB);
    }

#ifdef LTE_LIB_LOG
    // Channel response matrix log
    PhyLteDebugOutputChannelResponseLog(
        node, phyIndex, channelIndex, lteTxInfo, propTxInfo, propRxInfo);
#endif

#ifdef LTE_LIB_LOG
    PhyLteDebugOutputIfPowerLog(node, phyIndex);
#endif


    // If signals from serving eNB is received, calculate CSI
    if (phyLte->stationType == LTE_STATION_TYPE_UE)
    {
        LteRnti oppositeRnti = LteRnti(
            propTxInfo->txNodeId,
            LteGetMacInterfaceIndexFromPhyIndex(
            propTxInfo->txNode, propTxInfo->phyIndex));

        if (FALSE == PhyLteCheckRxDisconnected(node,
                                  phyIndex,
                                  phyLte,
                                  oppositeRnti))
        {
            PhyLteCalculateCsi(node,
                phyIndex,
                phyLte,
                propTxInfo,
                lteTxInfo,
                pathloss_dB);
        }
    }

    // The message with NoTransportBlockInfo and the message whose
    // payload size is 0 don't interfere in the desired signal.
    if ((PhyLteIsMessageNoTransportBlock(node,
                                         phyIndex,
                                         propRxInfo->txMsg)) ||
        ((!PhyLteIsMessageNoTransportBlock(node,
                                           phyIndex,
                                           propRxInfo->txMsg)) &&
         (MESSAGE_ReturnPacketSize(propRxInfo->txMsg) == 0) &&
         (MESSAGE_ReturnVirtualPacketSize(propRxInfo->txMsg) == 0)))
    {
#ifdef ADDON_DB
        // having some PHY control info
        PhyLteUpdateEventsTable(node,
                                phyIndex,
                                channelIndex,
                                propRxInfo,
                                NULL,
                                "PhyReceiveSignal");
        if (phyLte->thisPhy->phyStats)
        {
            phyLte->thisPhy->stats->AddSignalUnLockedDataPoints(
                                                node,
                                                propRxInfo->txMsg);
        }
#endif
        return;
    }
#ifdef ADDON_DB
    // add signal receive event
    PhyLteUpdateEventsTable(node,
                           phyIndex,
                           channelIndex,
                           propRxInfo,
                           NULL,
                           "PhyReceiveSignal");
#endif
    // Check packet error for all of the receiving signals
    PhyLteCheckAllOfRxPacketError(node, phyIndex);

    PhyLteRxMsgInfo* rxMsgInfo = NULL;

    // search if in desired signal list, if found, check error.
    rxMsgInfo = PhyLteGetRxMsgInfo(node,
                                   phyLte->rxPackMsgList,
                                   propRxInfo->txMsg,
                                   TRUE);
    // end of desired receiving signal?
    if (rxMsgInfo != NULL)
    {

#ifdef LTE_LIB_LOG
    lte::LteLog::DebugFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s,Rnti=,"LTE_STRING_FORMAT_RNTI",mW=,%e,dBm=,%e",
        LTE_STRING_CATEGORY_TYPE_RB_RX_POWER,
        propTxInfo->txNodeId, propTxInfo->phyIndex,
        rxMsgInfo->rxPower_mW,
        IN_DB(rxMsgInfo->rxPower_mW));
#endif
        Message* rfMsg = rxMsgInfo->rxMsg;
        BOOL isOriginalPackedMsg = TRUE;

        while (rfMsg != NULL)
        {
            // Retrieve destination information
            MacLteMsgDestinationInfo* dstInfo =
                (MacLteMsgDestinationInfo*)MESSAGE_ReturnInfo(
                    rfMsg, (unsigned short)INFO_TYPE_LteMacDestinationInfo);

            // Destination information must be set.
            ERROR_Assert(dstInfo, "Destination information not found.");

            bool isForMe = (dstInfo->dstRnti ==
                        LteLayer2GetRnti(node, thisPhy->macInterfaceIndex));

            // There exists transport blocks and DCI for UE messages
            // in rfMsg list.
            // Check if this message is DCI for UE or not.
            LteDciFormat0* DciForUl =
                (LteDciFormat0*)MESSAGE_ReturnInfo(
                    rfMsg,
                    (unsigned short)INFO_TYPE_LteDci0Info);

            // Dci for UE message and TBs not for me should not be forwarded
            // to MAC layer.
            if ((DciForUl == NULL) && (isForMe == true))
            {
                // rxMsgErrorStat must not be empty
                ERROR_Assert(phyLte->rxMsgErrorStat->empty() == false,
                            "rxMsgErrorStat is empty");

                LteRnti srcRnti = LteRnti(propTxInfo->txNodeId,
                            LteGetMacInterfaceIndexFromPhyIndex(
                            propTxInfo->txNode, propTxInfo->phyIndex));
                std::map < Message*, PhyLteTbsInfo > ::iterator it
                    = phyLte->rxMsgErrorStat->find(rfMsg);


                // rfMsg must be in rxMsgErrorStat.
                ERROR_Assert(it != phyLte->rxMsgErrorStat->end(),
                        "Rx message is not exist in the error stats map");


                PhyLteSetMessagePhyLtePhyToMacInfo(node,
                                                   phyIndex,
                                                   phyLte,
                                                   it->second.isError,
                                                   srcRnti,
                                                   rfMsg);

                Message* phy2mac = MESSAGE_Duplicate(node, rfMsg);

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG

                if (node->getNodeTime() >=
                    lte::LteLog::getValidationStatOffset()*SECOND)
                {
                    std::stringstream ss;
                    ss << srcRnti.nodeId << ","
                       << IN_DB(it->second.sinr) << ",";
                    lte::LteLog::InfoFormat(
                        node, node->phyData[phyIndex]->macInterfaceIndex,
                            LTE_STRING_LAYER_TYPE_PHY,
                            "%s,%s",
                            "ReceiveSinr",
                            ss.str().c_str());

                    phyLte->avgReceiveSinr->
                        operator[](srcRnti).regist(IN_DB(it->second.sinr));
                    phyLte->avgMcs->
                        operator[](srcRnti).regist(it->second.mcs);


                    if (it->second.isError == FALSE)
                    {
                        phyLte->totalReceivedBits->operator []
                            (srcRnti).regist(it->second.transportBlockSize);
                        phyLte->bler->operator [](srcRnti).regist(0.0);
                    }else
                    {
                        phyLte->bler->operator [](srcRnti).regist(1.0);
                    }
                }
#endif
#endif

                if (it->second.isError == FALSE)
                {
#ifdef ADDON_DB
                    PhyLteUpdateEventsTable(node,
                                            phyIndex,
                                            channelIndex,
                                            propRxInfo,
                                            phy2mac,
                                            "PhySendToUpper");

                    if (phyLte->thisPhy->phyStats)
                    {
                        phyLte->thisPhy->stats->
                            AddSignalToMacDataPointsForMsgInRecvPackedMsg(
                               node,
                               propRxInfo,
                               phyLte->thisPhy,
                               channelIndex,
                               propRxInfo->duration,
                               *phyLte->interferencePower_mW,
                               pathloss_dB,
                               NON_DB(lteTxInfo->txPower_dBm - pathloss_dB),
                               isOriginalPackedMsg);
                    }
#endif
                    MESSAGE_SetInstanceId(phy2mac, (short) phyIndex);

                    MAC_ReceivePacketFromPhy(
                        node,
                        node->phyData[phyIndex]->macInterfaceIndex,
                        phy2mac);

                    phyLte->stats.totalRxTbsToMac ++;
                    phyLte->stats.totalBitsToMac +=
                                            it->second.transportBlockSize;

                }
                else
                {
#ifdef ADDON_DB
                     PhyLteNotifyPacketDropForMsgInRxPackedMsg(
                               node,
                               phyIndex,
                               channelIndex,
                               propRxInfo->txMsg,
                               "Signal Received with Error",
                               (lteTxInfo->txPower_dBm - pathloss_dB),
                               pathloss_dB,
                               phy2mac);

                    if (phyLte->thisPhy->phyStats)
                    {
                        phyLte->thisPhy->stats->
                          AddSignalWithErrorsDataPointsForMsgInRecvPackedMsg(
                               node,
                               propRxInfo,
                               phyLte->thisPhy,
                               channelIndex,
                               *phyLte->interferencePower_mW,
                               pathloss_dB,
                               NON_DB(lteTxInfo->txPower_dBm - pathloss_dB),
                               isOriginalPackedMsg);
                    }
#endif
                    // Notify to MAC layer, even if error occured.
                    MAC_ReceivePacketFromPhy(
                        node,
                        node->phyData[phyIndex]->macInterfaceIndex,
                        phy2mac);

                    phyLte->stats.totalRxTbsWithErrors ++;
                }

                phyLte->rxMsgErrorStat->erase(it);
            }
            else
            {
                // DCI for UE or (TBs not for me)
                // Do nothing
#ifdef ADDON_DB
                // We are doing a check here for consistency of stats-DB LTE
                // Layer2 MAC entry of DCI control packet sent from eNB MAC.
                // The information is actually retrieved and saved by API
                // PhyLteSetDciForUl() on PHY at
                // PhyLteSignalArrivalFromChannel(), and later retrieved
                // on Layer 2 by API PhyLteGetDciForUl()

                if (DciForUl != NULL
                    && isForMe == true
                    && phyLte->stationType == LTE_STATION_TYPE_UE)
                {
                    Int32 interfaceIndex =
                                  node->phyData[phyIndex]->macInterfaceIndex;
                    Message* ulDciMsg = rfMsg;

                    PhyLteUpdateEventsTable(node,
                                            phyIndex,
                                            channelIndex,
                                            propRxInfo,
                                            ulDciMsg,
                                            "PhySendToUpper");

                    if (phyLte->thisPhy->phyStats)
                    {
                        phyLte->thisPhy->stats->
                            AddSignalToMacDataPointsForMsgInRecvPackedMsg(
                               node,
                               propRxInfo,
                               phyLte->thisPhy,
                               channelIndex,
                               propRxInfo->duration,
                               *phyLte->interferencePower_mW,
                               pathloss_dB,
                               NON_DB(lteTxInfo->txPower_dBm - pathloss_dB),
                               isOriginalPackedMsg);
                    }

                    // add PhyToMac info before log in MacDBEvent
                    LteRnti srcRnti = LteRnti(propTxInfo->txNodeId,
                                              propTxInfo->phyIndex);
                    PhyLteSetMessagePhyLtePhyToMacInfo(node,
                                                       phyIndex,
                                                       phyLte,
                                                       FALSE,
                                                       srcRnti,
                                                       rfMsg);

                    HandleMacDBEvents(
                        node,
                        ulDciMsg,
                        node->macData[interfaceIndex]->phyNumber,
                        interfaceIndex,
                        MAC_ReceiveFromPhy,
                        node->macData[interfaceIndex]->macProtocol);

                    if (node->macData[interfaceIndex]->macStats)
                    {
                        LteStatsDbSduPduInfo* ulDciMsgInfo =
                            (LteStatsDbSduPduInfo*)MESSAGE_ReturnInfo(
                                     ulDciMsg,
                                     (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);

                        node->macData[interfaceIndex]->stats->
                                    AddFrameReceivedDataPoints(
                                                  node,
                                                  ulDciMsg,
                                                  STAT_Unicast,
                                                  ulDciMsgInfo->ctrlSize,
                                                  ulDciMsgInfo->dataSize,
                                                  interfaceIndex);
                    }
                }
#endif
            }
            isOriginalPackedMsg = FALSE;
            rfMsg = rfMsg->next;
        }

#ifdef ADDON_DB
        if (phyLte->thisPhy->phyStats)
        {
            phyLte->thisPhy->stats->AddSignalUnLockedDataPoints(
                                            node,
                                            rxMsgInfo->propRxMsgAddress);
        }
#endif
        // Remove rxMsgInfo from receiving message list
        PhyLteFreeRxMsgInfo(node,
                            phyIndex,
                            rxMsgInfo,
                            phyLte->rxPackMsgList);
        rxMsgInfo = NULL;
    }
    // end of interference signal
    else
    {
        // search if in interference signal list, if found, remove it
        rxMsgInfo = PhyLteGetRxMsgInfo(node,
                                           phyLte->ifPackMsgList,
                                           propRxInfo->txMsg, TRUE);

        // if in interference list, delete from interference power
        if (rxMsgInfo != NULL)
        {

#ifdef LTE_LIB_LOG
            {
                lte::LteLog::Debug2Format(
                    node,
                    node->phyData[phyIndex]->macInterfaceIndex,
                    "PHY:DEBUG:PhyLteSignalEndFromChannel:",
                    "interference start%lf",
                    phyLte->interferencePower_mW[0]);
            }
#endif // LTE_LIB_LOG

            // Subtract interference power
            PhyLteSubtractInterferencePower(node, phyIndex, rxMsgInfo);

#ifdef LTE_LIB_LOG
            {
                lte::LteLog::Debug2Format(
                    node,
                    node->phyData[phyIndex]->macInterfaceIndex,
                    "PHY:DEBUG:PhyLteSignalEndFromChannel:",
                    "interference end%lf",
                    phyLte->interferencePower_mW);
            }
#endif // LTE_LIB_LOG

            // free the rxMsgInfo(Unpacked message=TBs)
            PhyLteFreeRxMsgInfo(node,
                                phyIndex,
                                rxMsgInfo,
                                phyLte->ifPackMsgList);
            rxMsgInfo = NULL;
        }
        else
        {
#ifdef LTE_LIB_LOG
                lte::LteLog::Debug2Format(
                    node,
                    node->phyData[phyIndex]->macInterfaceIndex,
                    "PHY:DEBUG:PhyLteSignalEndFromChannel",
                    "SIGNAL NOT FOUND IN RX_LIST and IF_LIST");

#endif // LTE_LIB_LOG
        }
    }

    // State transition
    if ((phyLte->rxPackMsgList->empty() == TRUE) &&
        (phyLte->ifPackMsgList->empty() == TRUE))
    {
        if (PhyLteIsInStationaryState(node, phyIndex) == true)
        {
            if (phyLte->stationType == LTE_STATION_TYPE_UE)
            {
                PhyLteChangeState(node, phyIndex, phyLte,
                                  PHY_LTE_DL_IDLE);
            }
            else // LTE_STATION_TYPE_ENB
            {
                PhyLteChangeState(node, phyIndex, phyLte,
                                  PHY_LTE_UL_IDLE);
            }
        }
    }
    else
    {
        // NOT LTE MODEL
#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteSignalEndFromChannel:NOT LTE MODEL:",
            "%d,%d,%p,%p",
            phyIndex,
            channelIndex,
            propRxInfo,
            propRxInfo->txMsg);
#endif // LTE_LIB_LOG
    }
}

// /**
// FUNCTION   :: LteTerminateCurrentReceive
// LAYER      :: PHY
// PURPOSE    :: Terminate all signals current under receiving.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// RETURN     :: void : NULL
// **/
void PhyLteTerminateCurrentReceive(Node* node,
                                   int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteTerminateCurrentReceive",
            "");
#endif // LTE_LIB_LOG

    std::list < PhyLteRxMsgInfo* > ::iterator rcv_it;
    rcv_it = phyLte->rxPackMsgList->begin();

    while (rcv_it != phyLte->rxPackMsgList->end())
    {
        if ((*rcv_it) != NULL)
        {
            Message* rfMsg = (*rcv_it)->rxMsg;

            while (rfMsg != NULL)
            {
                LteDciFormat0* DciForUl =
                    (LteDciFormat0*)MESSAGE_ReturnInfo(
                                rfMsg,
                                (unsigned short)INFO_TYPE_LteDci0Info);
                if (DciForUl == NULL)
                {
                    PropTxInfo* propTxInfo
                        = (PropTxInfo*)MESSAGE_ReturnInfo(
                                            (*rcv_it)->propRxMsgAddress);
                    Node* txNode = propTxInfo->txNode;
                    LteRnti srcRnti = LteRnti(propTxInfo->txNodeId,
                                              txNode->phyData[propTxInfo->
                                              phyIndex]->macInterfaceIndex);

                    PhyLteSetMessagePhyLtePhyToMacInfo(node,
                                                       phyIndex,
                                                       phyLte,
                                                       TRUE,
                                                       srcRnti,
                                                       rfMsg);

                    Message* phy2mac = MESSAGE_Duplicate(node, rfMsg);


                    MESSAGE_SetInstanceId(phy2mac, (short) phyIndex);

                    MAC_ReceivePacketFromPhy(
                        node,
                        node->phyData[phyIndex]->macInterfaceIndex,
                        phy2mac);

                    phyLte->stats.totalRxTbsWithErrors ++;
                }
                rfMsg = rfMsg->next;
            }

#ifdef ADDON_DB
            Int32 channelIndex = 0;
            PHY_GetTransmissionChannel(node,
                                       phyIndex,
                                       &channelIndex);
            PHY_NotificationOfPacketDrop(
                node,
                phyIndex,
                channelIndex,
                (*rcv_it)->propRxMsgAddress,
                "Rx Terminated by MAC",
                (*rcv_it)->rxPower_mW,
                *phyLte->interferencePower_mW,
                0.0);

            if (phyLte->thisPhy->phyStats)
            {
                phyLte->thisPhy->stats->AddSignalUnLockedDataPoints(
                                        node,
                                        (*rcv_it)->propRxMsgAddress);
            }
#endif
            PhyLteRxMsgInfo* tmpRxMsgInfo = *rcv_it;
            rcv_it++;

            // Free the message of the desired signal
            PhyLteFreeRxMsgInfo(node,
                                phyIndex,
                                tmpRxMsgInfo,
                                phyLte->rxPackMsgList);
        }
    }
    phyLte->rxMsgErrorStat->clear();

    std::list < PhyLteRxMsgInfo* > ::iterator i_it;
    i_it = phyLte->ifPackMsgList->begin();

    while (i_it != phyLte->ifPackMsgList->end())
    {
        PhyLteRxMsgInfo* tmpRxMsgInfo = *i_it;
        i_it++;

        // Subtract interference power
        PhyLteSubtractInterferencePower(node, phyIndex, tmpRxMsgInfo);

        // Free the message of the interference
        PhyLteFreeRxMsgInfo(node,
                            phyIndex,
                            tmpRxMsgInfo,
                            phyLte->ifPackMsgList);
    }
}

// /**
// FUNCTION   :: LteTerminateCurrentTransmissions
// LAYER      :: PHY
// PURPOSE    :: Terminate all ongoing transmissions
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// RETURN     :: void : NULL
// **/
void PhyLteTerminateCurrentTransmissions(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    if (phyLte->numTxMsgs > 0)
    {
        // cancel all scheduled Transmission End msgs
        std::list < Message* > ::iterator it;

        for (it = phyLte->txEndMsgList->begin();
             it != phyLte->txEndMsgList->end();
             it++)
        {
            MESSAGE_CancelSelfMsg(node, (*it));
            *it = NULL;
        }
        phyLte->txEndMsgList->clear();
        phyLte->numTxMsgs = 0;

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
// FUNCTION   :: PhyLteSetCqiReportInfo
// LAYER      :: PHY
// PURPOSE    :: Append infomation of RI or CQI to the message,
//               when the time to send of RI/CQI.
//               Priority: RI > CQI
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + msg          : Message      : The masseage address to append "info"
// RETURN     ::  BOOL    : when true, successful of set data.
// **/
BOOL PhyLteSetCqiReportInfo(Node* node, int phyIndex, Message* msg)
{
    BOOL ret = FALSE;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    UInt64 cqiTiming;
    UInt64 riTiming;
    UInt64 ttiCount;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    // CQI timing is decided according to the TTI number
    // retrieved from MAC layer plus 1
    ttiCount = MacLteGetTtiNumber(node, interfaceIndex);
    ++ttiCount;

    cqiTiming = (ttiCount + phyLte->cqiReportingOffset) %
        phyLte->cqiReportingInterval;
    riTiming = (ttiCount + phyLte->riReportingOffset) %
        phyLte->riReportingInterval;



    // RI is sent if CQI and RI collides.
    if (riTiming == 0)
    {
        // RI is sent if only valid one exists.
        // In case no RRC-config is received, RI could be invalid value
        if (phyLte->nextReportedRiInfo != PHY_LTE_INVALID_RI)
        {
            ret = PhyLteSetMessageRiInfo(
                            node, phyIndex, phyLte->nextReportedRiInfo, msg);

#ifdef LTE_LIB_LOG
            lte::LteLog::Debug2Format(
                node,
                node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteSetSrsInfo:",
                "PhyLteSetMessageRiInfo:"
                "%d,%llu,%llu,%llu",
                ret,
                ttiCount,
                cqiTiming,
                riTiming);
#endif // LTE_LIB_LOG
        }
    }
    else if (cqiTiming == 0)
    {
        // CQI is sent if only valid one exists.
        // In case no RRC-config is received, CQI could be invalid value
        if (phyLte->nextReportedCqiInfo.cqi0 != PHY_LTE_INVALID_CQI)
        {
            ret = PhyLteSetMessageCqiInfo(
                        node, phyIndex, phyLte->nextReportedCqiInfo, msg);

#ifdef LTE_LIB_LOG
            lte::LteLog::Debug2Format(
                node,
                node->phyData[phyIndex]->macInterfaceIndex,
                "PHY:DEBUG:PhyLteSetSrsInfo:",
                "PhyLteSetMessageCqiInfo:"
                ",%d,%llu,%llu,%llu",
                ret,
                ttiCount,
                cqiTiming,
                riTiming);
#endif // LTE_LIB_LOG
        }
    }

    return ret;
}

// /**
// FUNCTION   :: PhyLteSetSrsInfo
// LAYER      :: PHY
// PURPOSE    :: Append infomation of SRS to the message,
//               when the time to send of SRS.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + msg          : Message      : The masseage address to append "info"
// RETURN     ::  BOOL    : when true, successful of set data.
// **/
BOOL PhyLteSetSrsInfo(Node* node, int phyIndex, Message* msg)
{
    BOOL ret = FALSE;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    UInt64 srsTiming;
    UInt64 ttiCount;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    // SRS timing is decided according to the TTI number
    // retrieved from MAC layer plus 1
    ttiCount = MacLteGetTtiNumber(node, interfaceIndex);
    ++ttiCount;

    srsTiming = (ttiCount + phyLte->srsTransmissionOffset)
                 % phyLte->srsTransmissionInterval;

    if (srsTiming == 0)
    {
        ret = PhyLteSetMessageSrsInfo(node, interfaceIndex, msg);
    }

#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteSetSrsInfo:",
            "PhyLteSetMessageSrsInfo:"
            ",%d,%llu,%llu",
            ret,
            ttiCount,
            srsTiming);
#endif // LTE_LIB_LOG

    return ret;
}




// /**
// FUNCTION   :: PhyLteSetPssInfo
// LAYER      :: PHY
// PURPOSE    :: Append infomation of PSS to the message,
//               when the time to send of PSS.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + msg          : Message      : The masseage address to append "info"
// RETURN     ::  BOOL    : when true, successful of set data.
// **/
BOOL PhyLteSetPssInfo(Node* node, int phyIndex, Message* msg)
{
    BOOL ret = FALSE;
    UInt64 pssTiming;
    UInt64 ttiCount;
    int subframePerTti;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    // PSS timing is decided according to the TTI number
    // retrieved from MAC layer plus 1
    ttiCount = MacLteGetTtiNumber(node, interfaceIndex);
    ++ttiCount;
    // Get number of subframe per TTI from Mac
    subframePerTti = MacLteGetNumSubframePerTti(node, interfaceIndex);

    // PSS
    pssTiming = ttiCount % (PHY_LTE_PSS_INTERVAL / subframePerTti);

    if (pssTiming == 0)
    {
        LteRrcConfig* config = GetLteRrcConfig(node, interfaceIndex);
        ret = PhyLteSetMessagePssInfo(node, phyIndex, config, msg);
    }

#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteSetSrsInfo:",
            "PhyLteSetMessagePssInfo"
            ",%d,%llu,%llu",
            ret,
            ttiCount,
            pssTiming);
#endif // LTE_LIB_LOG

    return ret;
}


// /**
// FUNCTION   :: PhyLteSetRrcConnectedInfo
// LAYER      :: PHY
// PURPOSE    :: Append infomation of RRC-connected to the message,
//               when flagged RRC-connected compleate.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + msg          : Message    : The masseage address to append RRC connected
// RETURN     ::  BOOL    : when true, successful of set data.
// **/
BOOL PhyLteSetRrcConnectedInfo(Node* node, int phyIndex, Message* msg)
{
    BOOL ret = FALSE;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    ERROR_Assert(!(phyLte->rrcSetupCompleteFlag == TRUE
        && phyLte->rrcReconfigCompleteFlag == TRUE), "unexpected error");

    if (phyLte->rrcSetupCompleteFlag == TRUE)
    {
        ret = PhyLteSetMessageRrcCompleteInfo(node,
                                        phyIndex,
                                        &phyLte->maxTxPower_dBm,
                                        msg);
        phyLte->rrcSetupCompleteFlag = FALSE;
#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,Tx,%s",
            LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
            LTE_STRING_MESSAGE_TYPE_RRC_CONNECTION_SETUP_COMPLETE);
#endif

#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteSetSrsInfo:",
            "PhyLteSetRrcConnectedInfo"
            ",%d,%d,%lf",
            ret,
            phyLte->rrcSetupCompleteFlag,
            phyLte->maxTxPower_dBm);
#endif
    }

    if (phyLte->rrcReconfigCompleteFlag == TRUE)
    {
        ret = PhyLteSetMessageRrcReconfCompleteInfo(node,
                                        phyIndex,
                                        &phyLte->maxTxPower_dBm,
                                        msg);
        phyLte->rrcReconfigCompleteFlag = FALSE;
    }

    return ret;
}

// /**
// FUNCTION   :: PhyLteSetRlcResetData
// LAYER      :: PHY
// PURPOSE    :: Append infomation of RLC Reset data to the message,
//               when RLC reset Data is exist.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + msg          : Message   : The masseage address to append RLC reset info
// RETURN     ::  BOOL    : when true, successful of set data.
// **/
BOOL PhyLteSetRlcResetData(Node* node, int phyIndex, Message* msg)
{
    BOOL ret = FALSE;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);
    std::vector < LteRlcAmResetData > rlcResetData;
    std::vector < LteRlcAmResetData > ::iterator itr;
    LteRlcAmGetResetData(node, interfaceIndex, rlcResetData);
    if (rlcResetData.size() > 0)
    {
        ret = TRUE;
        LteRlcAmResetData* info =
            (LteRlcAmResetData*)MESSAGE_AddInfo(
                node,
                msg,
                sizeof(LteRlcAmResetData) * rlcResetData.size(),
                INFO_TYPE_LteRlcAmResetData);
        ERROR_Assert(info != NULL, "MESSAGE_AddInfo is failed");
        for (size_t i = 0; i < rlcResetData.size(); i++)
        {
            info[i] = rlcResetData[i];
        }
    }
    return ret;
}

// /**
// FUNCTION   :: PhyLteSetTxPower
// LAYER      :: PHY
// PURPOSE    :: Calculate and setup of transmission signal power
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : Int32        : Index of the PHY
// + packet       : Message:     : Message to send
// RETURN     :: double : Transmission power per RB [ dBm ]
// **/
double PhyLteGetTxPower(Node* node, Int32 phyIndex, Message* packet)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    double txPower_dBm;

    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        txPower_dBm = phyLte->maxTxPower_dBm
                            - IN_DB((double)(phyLte->numResourceBlocks));

#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteGetTxPower(eNB)",
            "%lf,%lf,%d",
            txPower_dBm,
            NON_DB(phyLte->maxTxPower_dBm),
            phyLte->numResourceBlocks);
#endif // LTE_LIB_LOG

    }
    else
    // UE
    {
        LtePhyConfig* enbConfig = GetLtePhyConfig(node, interfaceIndex);
        double alpha = enbConfig->tpcAlpha;
        double pousch = enbConfig->tpcPoPusch_mWPerRb;

        const LteRnti& enbRnti = phyLte->establishmentData->selectedRntieNB;

        // pathloss
        LteLayer3Filtering* meanMod
            = LteLayer2GetLayer3Filtering(node, interfaceIndex);

        double filteredPathloss_dB;
        meanMod->get(enbRnti, LTE_LIB_DL_PATHLOSS_RX0, &filteredPathloss_dB);

        // Retrieve number of RBs to send
        MacLteTxInfo* macTxInfo = (MacLteTxInfo*)
            MESSAGE_ReturnInfo(packet, INFO_TYPE_LteMacTxInfo);

        int numResourceBlocksToSend;
        if (macTxInfo != NULL)
        {
            numResourceBlocksToSend = macTxInfo->numResourceBlocks;
        }else
        {
            // If no transport blocks sent on this Message
            numResourceBlocksToSend = 1;
        }


        double txAllPower_dBm
            = MIN(phyLte->maxTxPower_dBm,
                  IN_DB((double)(numResourceBlocksToSend))
                  + (pousch + (alpha * filteredPathloss_dB)));

        // Transmissin power per RB
        txPower_dBm =
            IN_DB(NON_DB(txAllPower_dBm) / numResourceBlocksToSend);

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
        // UE, PUSCH transmission power
        if (macTxInfo != NULL)
        {
            if (node->getNodeTime()*SECOND >=
                                    lte::LteLog::getValidationStatOffset())
            {
                int interfaceIndex = 
                        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);
                phyLte->avgTxPower->operator [](
                    LteRnti(node->nodeId,interfaceIndex)).regist(txAllPower_dBm);
            }
        }
#endif
#endif

#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,txPower_mW=,%lf,txPower_dBm=,%lf,alpha=,%lf,POPUSCH=,%lf,"
                "filteredPloss_dB=,%lf,numRbsToSend=,%d,NoTb=,%d",
            LTE_STRING_CATEGORY_TYPE_UL_TX_POWER_RB_DETAIL,
            NON_DB(txPower_dBm),
            txPower_dBm,
            alpha,
            pousch,
            filteredPathloss_dB,
            numResourceBlocksToSend,
            (macTxInfo ? 0 : 1));
#endif // LTE_LIB_LOG

    }

    return txPower_dBm;
}

// /**
// FUNCTION   :: PhyLteStartTransmittingSignal
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + packet : Message* : Frame to be transmitted
// + useMacLayerSpecifiedDelay : BOOL : Use MAC specified delay
//                                      or calculate it
// + initDelayUntilAirborne    : clocktype : The MAC specified delay
// RETURN     :: void : NULL
// **/
void PhyLteStartTransmittingSignal(Node* node,
                                   int phyIndex,
                                   Message* packet,
                                   BOOL useMacLayerSpecifiedDelay,
                                   clocktype initDelayUntilAirborne)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    clocktype delayUntilAirborne = initDelayUntilAirborne;
    Int32 channelIndex = 0;
    Message* endMsg;
    clocktype duration;

    // Note that INFO_TYPE_LteMacNoTransportBlock means MAC layer has packet
    // to send. Subframe should be sent when PHY layer has control
    // information to send, even if MAC layer doesn't have any packet to send
    BOOL TransportFlag =
            MESSAGE_ReturnInfo(packet, INFO_TYPE_LteMacNoTransportBlock) ?
                    FALSE : TRUE ;

    // Set information for TRACE
    PhyLteSetMessageTraceData(node, phyIndex, packet);

    if (phyLte->stationType == LTE_STATION_TYPE_UE)
    {
        if (!PhyLteIsInStationaryState(node, phyIndex))
        {
            // Note that 1st TTI after the RA grant received
            // physical state is still establishment state.
            // Signal transmission is canceled even if there is packet
            // to sent
            TransportFlag = false;
        }
        else if (phyLte->txState != PHY_LTE_UL_IDLE)
        {
            // Note that this case does not happen normally.
            ERROR_ReportWarning(
                    "Signal transmission canceled due to the"
                    " busy transmission state.");

            // Signal transmission canceled even if there is packet to sent.
            TransportFlag = false;
        }
        else if (phyLte->txState == PHY_LTE_UL_IDLE)
        {
            TransportFlag =
                (TransportFlag
                 | PhyLteSetCqiReportInfo(node, phyIndex, packet));

            TransportFlag =
                (TransportFlag
                 | PhyLteSetSrsInfo(node, phyIndex, packet));

            TransportFlag =
                (TransportFlag
                 | PhyLteIsMessageBsrInfo(node, phyIndex, packet));

            TransportFlag =
                (TransportFlag
                 | PhyLteSetRrcConnectedInfo(node, phyIndex, packet));

            TransportFlag =
                (TransportFlag
                 | PhyLteSetRlcResetData(node, phyIndex, packet));
            
            TransportFlag =
                (TransportFlag
                 | PhyLteSetRrcMeasReportInfo(node, phyIndex, packet));
        }
    }
    else if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        // This function should be called only when
        // UE is in stationary status
        ERROR_Assert(phyLte->txState >= PHY_LTE_DL_IDLE,
                "Invalid transmission state detected");

        if (phyLte->txState != PHY_LTE_DL_IDLE)
        {
            ERROR_ReportWarning(
                    "Signal transmission canceled due to the"
                    " busy transmission status.");

            // Signal transmission canceled even if there is packet to sent.
            TransportFlag = false;
        }
        else
        {
            DlTtiInfo dlTtiInfo;
            dlTtiInfo.ttiNumber = MacLteGetTtiNumber(
                node, node->phyData[phyIndex]->macInterfaceIndex);

            TransportFlag =
                    (TransportFlag
                     | PhyLteSetMessageDlTtiInfo(node,
                                                 phyIndex,
                                                 packet,
                                                 &dlTtiInfo));
            TransportFlag =
                    (TransportFlag
                     | PhyLteSetPssInfo(node, phyIndex, packet));

            TransportFlag =
                    (TransportFlag
                     | PhyLteSetGrantInfo(node, phyIndex, packet));
            TransportFlag =
                (TransportFlag
                 | PhyLteSetRlcResetData(node, phyIndex, packet));
            TransportFlag =
                (TransportFlag
                 | PhyLteSetRrcConnReconfInfo(node, phyIndex, packet));
        }

    }

    // If there is no signal to transmit, free message
    if (TransportFlag == FALSE)
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::Debug2Format(
            node,
            node->phyData[phyIndex]->macInterfaceIndex,
            "PHY:DEBUG:PhyLteStartTransmittingSignal:",
            "NoTransportFreeMSG");
#endif // LTE_LIB_LOG

        MESSAGE_FreeList(node, packet);
        return;
    }
    else
    {
        // State transition
        if (PhyLteIsInStationaryState(node, phyIndex) == true)
        {
            if (phyLte->stationType == LTE_STATION_TYPE_UE)
            {
                PhyLteChangeState(node, phyIndex, phyLte,
                                  PHY_LTE_UL_FRAME_TRANSMITTING);
            }
            else // LTE_STATION_TYPE_ENB
            {
                PhyLteChangeState(node, phyIndex, phyLte,
                                  PHY_LTE_DL_FRAME_TRANSMITTING);
            }
        }
    }

    // Retrive transmission channel
    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    // Retrieve transmission power
    double txPower_dBm =
        PhyLteGetTxPower(node, phyIndex, packet);

#ifdef LTE_LIB_LOG
    lte::LteLog::DebugFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s,dBm=,%e,mW=,%e",
        LTE_STRING_CATEGORY_TYPE_TX_POWER_RB,
        txPower_dBm,
        NON_DB(txPower_dBm));
#endif

    // Set additional information used between Tx PHY layer and Rx PHY layer
    PhyLteSetMessageTxInfo(node, phyIndex, phyLte, txPower_dBm, packet);

    duration = phyLte->txDuration +
        phyLte->rxTxTurnaroundTime;

#ifdef ADDON_DB
    // check whether the message is from MAC layer
    if (MESSAGE_ReturnActualPacketSize(packet))
    {
        StatsDB_PhyRecordStartTransmittingSignal(node, phyIndex, packet);
    }

    if (phyLte->thisPhy->phyStats)
    {
        phyLte->thisPhy->stats->AddSignalTransmittedDataPoints(node,
                                                               duration);
    }
#endif
    // Transmit signal
    PROP_ReleaseSignal(node,
                       packet,
                       phyIndex,
                       channelIndex,
                       (float)txPower_dBm, // Tx power per RB
                       duration,
                       delayUntilAirborne);

#ifdef LTE_LIB_LOG
#if 0
    PhyLteDebugOutputMessage(node, phyIndex, packet, "StartTx", NULL);
#endif
#endif

    phyLte->numTxMsgs++;

    //GuiStart
    if (node->guiOption == TRUE && phyLte->numTxMsgs == 1)
    {
        GUI_Broadcast(node->nodeId,
                      GUI_PHY_LAYER,
                      GUI_DEFAULT_DATA_TYPE,
                      thisPhy->macInterfaceIndex,
                      node->getNodeTime());
    }
    //GuiEnd

    // Set timer for transmission end
    endMsg = MESSAGE_Alloc(node,
                           PHY_LAYER,
                           0,
                           MSG_PHY_TransmissionEnd);

    MESSAGE_SetInstanceId(endMsg, (short) phyIndex);

    // endMsg is stored for the purpose of cancellation of TxEndEvent
    phyLte->txEndMsgList->push_back(endMsg);
    MESSAGE_Send(node, endMsg, duration);

    // Keep track of phy statistics
    phyLte->stats.totalTxSignals ++;
}

// /**
// FUNCTION   :: PhyLteTransmissionEnd
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + msg       : Message* : The Tx End event
// RETURN     :: void  : NULL
// **/
void PhyLteTransmissionEnd(Node* node, int phyIndex, Message* msg)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    if (0)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf("Time %s: Node%d transmission end\n",
               clockStr, node->nodeId);
    }

    // Remove from the message list kept for cancellation purpose
    std::list < Message* > ::iterator it;
    for (it = phyLte->txEndMsgList->begin();
         it != phyLte->txEndMsgList->end();
         it++)
    {
        if ((*it) == msg)
        {
            phyLte->txEndMsgList->erase(it);
            break;
        }
    }
    phyLte->numTxMsgs--;

    // If all transmissions end, clean the status
    if (phyLte->numTxMsgs == 0)
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

    // State transition
    if (PhyLteIsInStationaryState(node, phyIndex) == true)
    {
        if (phyLte->stationType == LTE_STATION_TYPE_UE)
        {
            PhyLteChangeState(node, phyIndex, phyLte,
                              PHY_LTE_UL_IDLE);
        }
        else // LTE_STATION_TYPE_ENB
        {
            PhyLteChangeState(node, phyIndex, phyLte,
                              PHY_LTE_DL_IDLE);
        }
    }
}

// /**
// FUNCTION   :: LteChannelListeningSwitchNotification
// LAYER      :: PHY
// PURPOSE    :: Response to the channel listening status changes when
//               MAC switches channels.
// PARAMETERS ::
// + node      : Node*  : Pointer to node.
// + phyIndex  : int    : Index of the PHY
// + channelIndex : int : channel that the node starts/stops listening to
// + startListening : BOOL : TRUE if the node starts listening to the ch
//                           FALSE if the node stops listening to the ch
// RETURN     :: void  : NULL
// **/
void PhyLteChannelListeningSwitchNotification(Node* node,
                                              int phyIndex,
                                              int channelIndex,
                                              BOOL startListening)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    if (phyLte== NULL)
    {
        // not initialized yet, return;
        return;
    }

    return;
}



// /**
// FUNCTION   :: PhyLteGetTpcPoPusch
// LAYER      :: PHY
// PURPOSE    :: Get TPC parameter P_O_PUSCH
// PARAMETERS ::
// + node      : Node*  : Pointer to node.
// + phyIndex  : int    : Index of the PHY
// RETURN     :: void  : TPC parameter P_O_PUSCH (mW/RB)
// **/
double PhyLteGetTpcPoPusch(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        return phyLte->tpcPoPusch_dBmPerRb;
    }else
    {
        // UE
        return GetLtePhyConfig(node, interfaceIndex)->tpcPoPusch_mWPerRb;
    }
}

// /**
// FUNCTION   :: PhyLteGetTpcAlpha
// LAYER      :: PHY
// PURPOSE    :: Get TPC parameter alpha
// PARAMETERS ::
// + node      : Node*  : Pointer to node.
// + phyIndex  : int    : Index of the PHY
// RETURN     :: double : TPC parameter alpha
// **/
double PhyLteGetTpcAlpha(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        return phyLte->tpcAlpha;
    }else
    {
        // UE
        return GetLtePhyConfig(node, interfaceIndex)->tpcAlpha;
    }
}

// /**
// FUNCTION   :: PhyLteGetThermalNoise
// LAYER      :: PHY
// PURPOSE    :: Get thermal noise
// PARAMETERS ::
// + node      : Node*  : Pointer to node.
// + phyIndex  : int    : Index of the PHY
// RETURN     :: double : Thermal noise [ mW per RB ]
// **/
double PhyLteGetThermalNoise(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    return phyLte->rbNoisePower_mW;
}

// /**
// FUNCTION   :: PhyLteDetermineTransmissiomScheme
// LAYER      :: PHY
// PURPOSE    :: Determine transmission scheme from rankIndicator and
//               configured transmission mode.
// PARAMETERS ::
// + node      : Node*   : Pointer to node.
// + phyIndex  : int     : Index of the PHY
// + rankIndicator : int : Rank indicator
// + numLayer  : int* :
//                 Pointer to the buffer for number of layer.
// + numTransportBlocksToSend : int* :
//                 Pointer to buffer for number of transport blocks to send.
// RETURN     :: LteTxScheme : Transmission scheme
// **/
LteTxScheme PhyLteDetermineTransmissiomScheme(Node* node,
                                  int phyIndex,
                                  int rankIndicator,
                                  int* numLayer,
                                  int* numTransportBlocksToSend)
{
    char errBuf[MAX_STRING_LENGTH] = {0};

    LteTxScheme txScheme = TX_SCHEME_INVALID;

    int transmissionMode = GetLteMacConfig(node,
            node->phyData[phyIndex]->macInterfaceIndex)->transmissionMode;

    switch (transmissionMode){
    case 1:
        txScheme = TX_SCHEME_SINGLE_ANTENNA;
        *numLayer = 1;
        *numTransportBlocksToSend = 1;
        break;
    case 2:
        txScheme = TX_SCHEME_DIVERSITY;
        *numLayer = PhyLteGetNumTxAntennas(node, phyIndex);
        *numTransportBlocksToSend = 1;
        break;
    case 3:
        if (rankIndicator == 1)
        {
            txScheme = TX_SCHEME_DIVERSITY;
            *numLayer = PhyLteGetNumTxAntennas(node, phyIndex);
            *numTransportBlocksToSend = 1;
        }else if (rankIndicator == 2)
        {
            txScheme = TX_SCHEME_OL_SPATIAL_MULTI;
            *numLayer = rankIndicator;
            *numTransportBlocksToSend = rankIndicator;
        }
        else
        {
            sprintf(errBuf,
                "Invalid rank indicator %d", rankIndicator);
            ERROR_Assert(FALSE,errBuf);
        }
        break;
    default:
        sprintf(errBuf,
            "Transmission mode %d is not supported. ", transmissionMode);
            ERROR_Assert(FALSE,errBuf);
    }

    return txScheme;
}

// /**
// FUNCTION   :: PhyLteGetPathloss_dB
// LAYER      :: PHY
// PURPOSE    :: Calculate pathloss excluding fading
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex   : int     : Index of the PHY
// + lteTxInfo  : PhyLteTxInfo* : Pointer to LTE transmission info.
// + propRxInfo : PropRxInfo*   : Pointer to propagation rx info.
// RETURN     :: double : Pathloss excluding fading in dB
// **/
double PhyLteGetPathloss_dB(Node* node,
                            int phyIndex,
                            PhyLteTxInfo* lteTxInfo,
                            PropRxInfo* propRxInfo)
{
    double pathloss_dB = lteTxInfo->txPower_dBm
         - (ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo)
         + propRxInfo->rxPower_dBm);

    return pathloss_dB;
}

// /**
// FUNCTION   :: LteGetPhyStateString
// LAYER      :: LTE
// PURPOSE    :: Get the PHY state as the string.
// PARAMETERS ::
// + state      : PhyLteState   : phy state.
// RETURN     :: char* : the string of phy state.
// **/
static const char* LteGetPhyStateString(PhyLteState state)
{
    switch (state){
    case PHY_LTE_POWER_OFF:
        return "POWER_OFF";
    case PHY_LTE_IDLE_CELL_SELECTION:
        return "IDLE_CELL_SELECTION";
    case PHY_LTE_CELL_SELECTION_MONITORING:
        return "CELL_SELECTION_MONITORING";
    case PHY_LTE_IDLE_RANDOM_ACCESS:
        return "IDLE_RANDOM_ACCESS";
    case PHY_LTE_RA_PREAMBLE_TRANSMISSION_RESERVED:
        return "RA_PREAMBLE_TRANSMISSION_RESERVED";
    case PHY_LTE_RA_PREAMBLE_TRANSMITTING:
        return "RA_PREAMBLE_TRANSMITTING";
    case PHY_LTE_RA_GRANT_WAITING:
        return "RA_GRANT_WAITING";
    case PHY_LTE_DL_IDLE:
        return "DL_IDLE";
    case PHY_LTE_UL_IDLE:
        return "UL_IDLE";
    case PHY_LTE_UL_FRAME_RECEIVING:
        return "UL_FRAME_RECEIVING";
    case PHY_LTE_DL_FRAME_TRANSMITTING:
        return "DL_FRAME_TRANSMITTING";
    case PHY_LTE_DL_FRAME_RECEIVING:
        return "DL_FRAME_RECEIVING";
    case PHY_LTE_UL_FRAME_TRANSMITTING:
        return "UL_FRAME_TRANSMITTING";
    default:
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
                "PHY-LTE: PhyLteState %d error",
                (int)state);
        ERROR_Assert(FALSE, errorStr);
    }
    return "INVALID_STATE";
}

// /**
// FUNCTION   :: PhyLteGetResourceAllocationInfo
// LAYER      :: LTE
// PURPOSE    :: get the list of RB.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + txScheme  : LteTxScheme : Transmission scheme
// + numCodeWords  : int*    : Number of *codewords*
// + tb2cwSwapFlag : bool*   : Codeword to transport block swap flag
//                             (Valid for Dic2aInfo)
// + usedRB_list   : UInt8*  : List of resource blocks allocated
// + numRb     : int*        : Number of resource blocks allocated
// + mcsIndex  : std::vector<UInt8*> : List of mcsIndex for each
//                                     *transport blocks*
// RETURN     :: Message*  :  rfMsg->next if 2 transport blocks detected
//                            rfMsg       otherwise.
// **/
Message* PhyLteGetResourceAllocationInfo(Node* node,
                      int phyIndex,
                      Message* rfMsg,
                      LteTxScheme txScheme,
                      int* numCodeWords,
                      bool* tb2cwSwapFlag,
                      UInt8* usedRB_list,
                      int* numRb,
                      std::vector < UInt8 >* mcsIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    MacLteMsgDestinationInfo* dstInfo =
        (MacLteMsgDestinationInfo*)MESSAGE_ReturnInfo(rfMsg,
        (unsigned short)INFO_TYPE_LteMacDestinationInfo);
    ERROR_Assert(dstInfo != NULL,
                 "PHY-LTE: not detect INFO_TYPE_LteMacDestinationInfo!");

    // Normally, tb2cwSwapFlag is false.
    *tb2cwSwapFlag = false;

    // Normally, number of codewords is 1
    *numCodeWords = 1;

    // Retrive RB allocation information
    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        LteDciFormat0* Dci0Info =
            (LteDciFormat0*)MESSAGE_ReturnInfo(
                rfMsg,
                (unsigned short)INFO_TYPE_LteDciForUl);
        ERROR_Assert(Dci0Info != NULL,
                     "PHY-LTE: not detect DCI!");
        for (UInt8 i = 0; i < Dci0Info->usedRB_length; i ++)
        {
            usedRB_list[Dci0Info->usedRB_start + i] = 1;
        }
        if (mcsIndex)
        {
            mcsIndex->push_back(Dci0Info->mcsID);
        }
    }
    else //phyLte->stationType == LTE_STATION_TYPE_UE
    {
        if ((txScheme == TX_SCHEME_SINGLE_ANTENNA)
            || (txScheme == TX_SCHEME_DIVERSITY))
        {
            LteDciFormat1* Dci1Info =
                (LteDciFormat1*)MESSAGE_ReturnInfo(
                    rfMsg,
                    (unsigned short)INFO_TYPE_LteDci1Info);
            ERROR_Assert(Dci1Info != NULL,
                         "PHY-LTE: not detect DCI!");
            memcpy(usedRB_list, Dci1Info->usedRB_list,
                   sizeof(UInt8)*PHY_LTE_MAX_NUM_RB);
            if (mcsIndex)
            {
                mcsIndex->push_back(Dci1Info->mcsID);
            }
        }
        else if (txScheme == TX_SCHEME_OL_SPATIAL_MULTI)
        {
            LteDciFormat2a* Dci2aInfo =
                (LteDciFormat2a*)MESSAGE_ReturnInfo(
                    rfMsg,
                    (unsigned short)INFO_TYPE_LteDci2aInfo);

            ERROR_Assert(Dci2aInfo != NULL,
                         "PHY-LTE: not detect DCI!");

            *tb2cwSwapFlag = Dci2aInfo->tb2cwSwapFlag;

            // Judge if next message is second  TB
            if (rfMsg->next)
            {
                // If next message is sameas message of current rfMsg

                // Retrieve destination information of next message
                MacLteMsgDestinationInfo* dstInfoNext =
                    (MacLteMsgDestinationInfo*)MESSAGE_ReturnInfo(
                        rfMsg->next,
                        (unsigned short)INFO_TYPE_LteMacDestinationInfo);
                ERROR_Assert(dstInfoNext != NULL,
                             "PHY-LTE: not detect "
                             "INFO_TYPE_LteMacDestinationInfo!");

                //Is same destination ?
                bool isSameDst = (dstInfoNext->dstRnti == dstInfo->dstRnti);

                LteDciFormat0* DciForUl =
                    (LteDciFormat0*)MESSAGE_ReturnInfo(
                        rfMsg->next,
                        (unsigned short)INFO_TYPE_LteDci0Info);

                if ((DciForUl == NULL) && (isSameDst == true))
                {
                    LteDciFormat2a* Dci2aInfoNext =
                        (LteDciFormat2a*)MESSAGE_ReturnInfo(
                            rfMsg->next,
                            (unsigned short)INFO_TYPE_LteDci2aInfo);

                    // LteDciFormat2a * must not * be allocated to next rfMsg
                    ERROR_Assert(Dci2aInfoNext == NULL,
                                "PHY-LTE: Invalid DCI information applied.");

                    // Increment rfMsg
                    rfMsg = rfMsg->next;

                    *numCodeWords = 2;
                }
            }

            if (mcsIndex)
            {
                mcsIndex->resize(2);
                mcsIndex->at(0) = Dci2aInfo->forTB[0].mcsID;
                mcsIndex->at(1) = Dci2aInfo->forTB[1].mcsID;
            }

            memcpy(usedRB_list, Dci2aInfo->usedRB_list,
                   sizeof(UInt8)*PHY_LTE_MAX_NUM_RB);
        }
        else
        {
            ERROR_Assert(FALSE, "PHY-LTE: illegal Tx-scheme!");
        }
    }

    if (numRb)
    {
        *numRb = 0;

        for (int i = 0; i < PHY_LTE_MAX_NUM_RB; i++)
        {
            if (usedRB_list[i] != 0)
            {
                (*numRb)++;
            }
        }
    }

    return rfMsg;
}

// /**
// FUNCTION   :: PhyLteGetResourceAllocationInfo
// LAYER      :: LTE
// PURPOSE    :: Get the list of RB.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + rfMsg     : Message*  : received message
// + usedRB_list : UInt8*  : Pointer to the buffer for list of RB
// + numRb     : int*      : Pointer to the buffer for number of RBs
// RETURN     :: BOOL      :  TRUE  : If DCI found
//                            FALSE : otherwise
// **/
BOOL PhyLteGetResourceAllocationInfo(Node* node,
                      int phyIndex,
                      Message* rfMsg,
                      UInt8* usedRB_list,
                      int* numRb)
{
    // DCI format-0?
    LteDciFormat0* Dci0Info = (LteDciFormat0*)MESSAGE_ReturnInfo(rfMsg,
                                    (unsigned short)INFO_TYPE_LteDciForUl);

    // DCI format-1?
    LteDciFormat1* Dci1Info = (LteDciFormat1*)MESSAGE_ReturnInfo(rfMsg,
                                    (unsigned short)INFO_TYPE_LteDci1Info);

    // DCI format-2a?
    LteDciFormat2a* Dci2aInfo = (LteDciFormat2a*)MESSAGE_ReturnInfo(rfMsg,
                                    (unsigned short)INFO_TYPE_LteDci2aInfo);


    if (Dci0Info)
    {
        ERROR_Assert(Dci1Info == NULL, "DCI conflicting");
        ERROR_Assert(Dci2aInfo == NULL, "DCI conflicting");

        LteDciFormat0* Dci0Info =
            (LteDciFormat0*)MESSAGE_ReturnInfo(
                rfMsg,
                (unsigned short)INFO_TYPE_LteDciForUl);

        for (UInt8 i = 0; i < Dci0Info->usedRB_length; i ++)
        {
            usedRB_list[Dci0Info->usedRB_start + i] = 1;
        }
    }else if (Dci1Info)
    {
        ERROR_Assert(Dci0Info == NULL, "DCI conflicting");
        ERROR_Assert(Dci2aInfo == NULL, "DCI conflicting");

        memcpy(usedRB_list, Dci1Info->usedRB_list,
               sizeof(UInt8)*PHY_LTE_MAX_NUM_RB);
    }else if (Dci2aInfo)
    {
        ERROR_Assert(Dci0Info == NULL, "DCI conflicting");
        ERROR_Assert(Dci1Info == NULL, "DCI conflicting");

        memcpy(usedRB_list, Dci2aInfo->usedRB_list,
               sizeof(UInt8)*PHY_LTE_MAX_NUM_RB);
    }else
    {
        return FALSE;
    }

    if (numRb)
    {
        *numRb = 0;

        for (int i = 0; i < PHY_LTE_MAX_NUM_RB; i++)
        {
            if (usedRB_list[i] != 0)
            {
                (*numRb)++;
            }
        }
    }

    return TRUE;
}

// /**
// FUNCTION   :: PhyLteSetCheckingConnectionTimer
// LAYER      :: PHY
// PURPOSE    :: Set a CheckingConnection Timer
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteSetCheckingConnectionTimer(
                                Node* node,
                                int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    std::map < int, Message* > ::iterator itr;
    itr = phyLte->eventMsgList->find(MSG_PHY_LTE_CheckingConnection);
    if (itr != phyLte->eventMsgList->end())
    {
        MESSAGE_CancelSelfMsg(node, itr->second);
        phyLte->eventMsgList->erase(itr);
    }

    Message* newMsg;
    newMsg = MESSAGE_Alloc(node,
                           PHY_LAYER,
                           0,
                           MSG_PHY_LTE_CheckingConnection);

    MESSAGE_SetInstanceId(newMsg, (short) phyIndex);

    phyLte->eventMsgList->insert(
            std::pair < int, Message* > (
            MSG_PHY_LTE_CheckingConnection, newMsg));
#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s,SetTimer",
        LTE_STRING_CATEGORY_TYPE_LOST_DETECTION);
#endif // LTE_LIB_LOG
    MESSAGE_Send(node, newMsg, phyLte->checkingConnectionInterval);
}

// /**
// FUNCTION   :: PhyLteCheckingConnectionExpired
// LAYER      :: PHY
// PURPOSE    :: Checking Connection
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + phyIndex  : int      : Index of the PHY
// + msg       : Message* : CheckingConnection timer message
// RETURN     :: void  : NULL
// **/
void PhyLteCheckingConnectionExpired(
                                Node* node,
                                int phyIndex,
                                Message* msg)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    BOOL lostDectionOccuredOnUe = FALSE;

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s,ExpireTimer",
        LTE_STRING_CATEGORY_TYPE_LOST_DETECTION);
#endif
    if (phyLte->stationType == LTE_STATION_TYPE_ENB)
    {
        std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator itr;
        if (phyLte->ueInfoList->empty() == false)
        {
            itr = phyLte->ueInfoList->begin();
            while (itr != phyLte->ueInfoList->end())
            {
                PhyLteConnectedUeInfo* ueInfo = &(itr->second);
                const LteRnti& ueRnti = itr->first;
                BOOL erased = FALSE;
                if (ueInfo->isDetecting == FALSE &&
                   ueInfo->isFirstChecking == FALSE)
                {
                    clocktype prevTime =
                        node->getNodeTime() -
                        phyLte->checkingConnectionInterval;
                    if (prevTime > ueInfo->connectedTime)
                    {
#ifdef LTE_LIB_LOG
                        lte::LteLog::InfoFormat(
                            node, node->phyData[phyIndex]->macInterfaceIndex,
                            LTE_STRING_LAYER_TYPE_PHY,
                            "%s,Periodic,%s=,"LTE_STRING_FORMAT_RNTI,
                            LTE_STRING_CATEGORY_TYPE_LOST_DETECTION,
                            LTE_STRING_STATION_TYPE_UE,
                            ueRnti.nodeId, ueRnti.interfaceIndex);
#endif
                        itr++;  // this increment should be before erase
                        Layer3LteNotifyLostDetection(
                            node, thisPhy->macInterfaceIndex, ueRnti);
                        erased = TRUE;
                    }
                }
                else
                {
                    ueInfo->isFirstChecking = FALSE;
                }

                if (erased == FALSE)
                {
                    ueInfo->isDetecting = FALSE;
                    itr++;
                }
            }
        }
    }
    else if (phyLte->stationType == LTE_STATION_TYPE_UE)
    {
        PhyLteEstablishmentData* establishmentData =
            phyLte->establishmentData;
        if (establishmentData->isDetectingSelectedEnb == TRUE)
        {
            // do nothing
        }
        else
        {
#ifdef LTE_LIB_LOG
            const LteRnti& enbRnti = establishmentData->selectedRntieNB;
            lte::LteLog::InfoFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,Periodic,%s=,"LTE_STRING_FORMAT_RNTI,
                LTE_STRING_CATEGORY_TYPE_LOST_DETECTION,
                LTE_STRING_STATION_TYPE_ENB,
                enbRnti.nodeId, enbRnti.interfaceIndex);
#endif
            Layer3LteNotifyLostDetection(
                node, thisPhy->macInterfaceIndex,
                establishmentData->selectedRntieNB);
            lostDectionOccuredOnUe = TRUE;
        }
        establishmentData->isDetectingSelectedEnb = FALSE;
    }

    phyLte->eventMsgList->erase(MSG_PHY_LTE_CheckingConnection);

    // set next CheckingConnection timer
    if (phyLte->stationType == LTE_STATION_TYPE_ENB
        || (phyLte->stationType == LTE_STATION_TYPE_UE
            && lostDectionOccuredOnUe == FALSE))
    {
        PhyLteSetCheckingConnectionTimer(node, phyIndex);
    }
}

// /**
// FUNCTION   :: PhyLteSetCheckingConnectionTimer
// LAYER      :: PHY
// PURPOSE    :: Set a CheckingConnection Timer
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteSetInterferenceMeasurementTimer(
                                Node* node,
                                int phyIndex)
{
#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING

    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    Message* newMsg;
    newMsg = MESSAGE_Alloc(node,
                           PHY_LAYER,
                           0,
                           MSG_PHY_LTE_InterferenceMeasurementTimerExpired);

    MESSAGE_SetInstanceId(newMsg, (short) phyIndex);

    phyLte->eventMsgList->insert(std::pair < int, Message* > (
                            MSG_PHY_LTE_InterferenceMeasurementTimerExpired,
                            newMsg));


    clocktype currentTime = node->getNodeTime();
    clocktype ttiLength = MacLteGetTtiLength(node, interfaceIndex);

    // Measurement timing is the moment of half of the TTI
    clocktype delay;

    // Measurement timing is half of the TTI
    delay = ttiLength - (currentTime + ttiLength / 2) % ttiLength;

    // Note that if currentTime is just the moment of half of TTI,
    // delay equals to ttiLength.

    MESSAGE_Send(node, newMsg, delay);

#endif
}

// /**
// FUNCTION   :: PhyLteInterferenceMeasurementTimerExpired
// LAYER      :: PHY
// PURPOSE    :: Tiemr for measuring interference power
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + phyIndex  : int      : Index of the PHY
// + msg       : Message* : InterferenceMeasurement timer message
// RETURN     :: void  : NULL
// **/
void PhyLteInterferenceMeasurementTimerExpired(
                                Node* node,
                                int phyIndex,
                                Message* msg)
{
#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING

    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    phyLte->eventMsgList->erase(
                        MSG_PHY_LTE_InterferenceMeasurementTimerExpired);

    for (int rbIndex = 0; rbIndex < phyLte->numResourceBlocks; ++rbIndex)
    {
        double instantValue;
#if PHY_LTE_INTERFERENCE_FILTERING_IN_DBM
        if (phyLte->interferencePower_mW[rbIndex] <= 0.0)
        {
            instantValue = LTE_NEGATIVE_INFINITY_POWER_dBm;
        }else
        {
            instantValue = MAX(
                    IN_DB(phyLte->interferencePower_mW[rbIndex]),
                    LTE_NEGATIVE_INFINITY_POWER_dBm);
        }
#else
        instantValue = phyLte->interferencePower_mW[rbIndex];
#endif


        phyLte->filteredInterferencePower[rbIndex].update(
                instantValue,
                phyLte->interferenceFilterCoefficient,
                true);
    }

    PhyLteSetInterferenceMeasurementTimer(node, phyIndex);

#ifdef LTE_LIB_LOG
    stringstream ss;

    for (int rbIndex = 0; rbIndex < phyLte->numResourceBlocks; ++rbIndex)
    {
        double filteredIfPower;
        phyLte->filteredInterferencePower[rbIndex].get(&filteredIfPower);
        ss << filteredIfPower << ",";
    }

    for (int rbIndex = 0; rbIndex < phyLte->numResourceBlocks; ++rbIndex)
    {
        ss <<  phyLte->interferencePower_mW[rbIndex] << ",";
    }

    lte::LteLog::Debug2Format(
        node,
        node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s,%s",
        "FilteredIfPower",
        ss.str().c_str());
#endif


#endif
}


// /**
// FUNCTION   :: PhyLteIndicateDisconnectToUe
// LAYER      :: PHY
// PURPOSE    :: Indicate disconnection to UE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + phyIndex  : int            : Index of the PHY
// + ueRnti    : const LteRnti& : UE's RNTI
// RETURN     :: void  : NULL
// **/
void PhyLteIndicateDisconnectToUe(
                                Node* node,
                                int phyIndex,
                                const LteRnti& ueRnti)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    ERROR_Assert(phyLte->stationType != LTE_STATION_TYPE_UE,
        "PhyLteIndicateDisconnectToUe is called only eNB.");

    std::map < LteRnti, PhyLteConnectedUeInfo > ::iterator itr;
    itr = phyLte->ueInfoList->find(ueRnti);
    if (itr != phyLte->ueInfoList->end())
    {
        phyLte->ueInfoList->erase(itr);
    }
}

// /**
// FUNCTION   :: CqiReceiver::CqiReceiver
// LAYER      :: PHY
// PURPOSE    :: Initialize CqiReceiver
// PARAMETERS ::
// RETURN     :: void  : NULL
// **/
CqiReceiver::CqiReceiver()
{
    init(NULL, -1);
}

// /**
// FUNCTION   :: CqiReceiver::init
// LAYER      :: PHY
// PURPOSE    :: Initialize CqiReceiver
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + phyIndex  : int            : Index of the PHY
// RETURN     :: void  : NULL
// **/
void CqiReceiver::init(Node* node, int phyIndex)
{
    _node = node;
    _phyIndex = phyIndex;
    _state = INIT;

    _cqiReportInfo.cqiInfo.cqi0 = PHY_LTE_INVALID_CQI;
    _cqiReportInfo.cqiInfo.cqi1 = PHY_LTE_INVALID_CQI;
    _cqiReportInfo.riInfo = PHY_LTE_INVALID_RI;

    _newestCqi.cqi0 = PHY_LTE_INVALID_CQI;
    _newestCqi.cqi1 = PHY_LTE_INVALID_CQI;
    _newestRi = PHY_LTE_INVALID_RI;
}

// /**
// FUNCTION   :: CqiReceiver::~CqiReceiver
// LAYER      :: PHY
// PURPOSE    :: Finalize CqiReceiver
// PARAMETERS ::
// RETURN     :: void  : NULL
// **/
CqiReceiver::~CqiReceiver()
{

}

// /**
// FUNCTION   :: CqiReceiver::notifyCqiUpdate
// LAYER      :: PHY
// PURPOSE    :: Notify CQI receiver of new CQI
// PARAMETERS ::
// + cqis      : const PhyLteCqi& : CQI information
// RETURN     :: void  : NULL
// **/
void CqiReceiver::notifyCqiUpdate(const PhyLteCqi& cqis)
{
    ERROR_Assert(cqis.cqi0 != PHY_LTE_INVALID_CQI, "Invalid CQI received.");

    _newestCqi = cqis;
    _cqiReportInfo.cqiInfo = _newestCqi;

    switch (_state)
    {
    case INIT:
        changeState(NORI);
        break;
    case NOCQI:
        _cqiReportInfo.riInfo = _newestRi;
        changeState(STATIONARY);
        break;
    case NORI:
        break;
    case PENDING:
        _cqiReportInfo.riInfo = _newestRi;
        changeState(STATIONARY);
        break;
    case STATIONARY:
        break;
    default:
        ERROR_Assert(FALSE, "Invalid CqiReceiver status");
    }
}

// /**
// FUNCTION   :: CqiReceiver::notifyRiUpdate
// LAYER      :: PHY
// PURPOSE    :: Notify CQI receiver of new RI
// PARAMETERS ::
// + ri       : int : RI information
// RETURN     :: void  : NULL
// **/
void CqiReceiver::notifyRiUpdate(int ri)
{
    ERROR_Assert(ri >= 1 && ri <= 2, "Invalid RI received.");

    _newestRi = ri;

    switch (_state)
    {
    case INIT:
        changeState(NOCQI);
        break;
    case NOCQI:
        break;
    case NORI:
        changeState(NOCQI);
        break;
    case PENDING:
        break;
    case STATIONARY:
        changeState(PENDING);
        break;
    default:
        ERROR_Assert(FALSE, "Invalid CqiReceiver status");
    }
}

// /**
// FUNCTION   :: CqiReceiver::getCqiReportInfo
// LAYER      :: PHY
// PURPOSE    :: Get current valid CQI information
// PARAMETERS ::
// + cqiReportInfo : PhyLteCqiReportInfo& : Current CQI information
// RETURN     :: bool : true, if there is valid CQI info. false, oterwise.
// **/
bool CqiReceiver::getCqiReportInfo(PhyLteCqiReportInfo& cqiReportInfo)
{
    cqiReportInfo = _cqiReportInfo;

    if (_state == PENDING || _state == STATIONARY)
    {
        return true;
    }else
    {
        return false;
    }
}

// /**
// FUNCTION   :: CqiReceiver::changeState
// LAYER      :: PHY
// PURPOSE    :: Change state of CQI receiver
// PARAMETERS ::
// + cqiReportInfo : PhyLteCqiReportInfo& : Current CQI information
// RETURN     :: bool : true, if there is valid CQI info. false, otherwise.
// **/
CqiReceiver::CqiReceiverState CqiReceiver::changeState(
        CqiReceiver::CqiReceiverState newState)
{
    CqiReceiver::CqiReceiverState oldState = _state;
    _state = newState;

    return oldState;
}



// /**
// FUNCTION   :: PhyLteClearInfo
// LAYER      :: PHY
// PURPOSE    :: clear information about opposite RNTI specified
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : int              : Index of the PHY
// + rnti      : const LteRnti&   : opposite RNTI
// RETURN     :: void  : NULL
// **/
void PhyLteClearInfo(Node* node, int phyIndex, const LteRnti& rnti)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*) thisPhy->phyVar;

    // Cancel the receiving signal

    PhyLteTerminateCurrentReceive(node,
                                  phyIndex);

    // Cancel the TransmissionEnd messages
    PhyLteTerminateCurrentTransmissions(node, phyIndex);

    // Cancel the event timers
    // Note : Interference measurement timer must not be
    //        cancelled as long as POWER is ON.
    std::map < int, Message* > ::iterator it;
    for (it = phyLte->eventMsgList->begin();
         it != phyLte->eventMsgList->end();
         )
    {
        if (it->second->eventType != MSG_PHY_LTE_InterferenceMeasurementTimerExpired){
            MESSAGE_CancelSelfMsg(node, it->second);
            phyLte->eventMsgList->erase(it++);
        }else{
            it++;
        }
    }

    // Clear the lists
    phyLte->ifPackMsgList->clear();
    phyLte->rxPackMsgList->clear();
    if (phyLte->rxDciForUlInfo)
    {
        phyLte->rxDciForUlInfo->clear();
    }

    if (phyLte->establishmentData->rxPreambleList)
    {
        phyLte->establishmentData->rxPreambleList->clear();
    }
    if (phyLte->establishmentData->rxRrcConfigList)
    {
        phyLte->establishmentData->rxRrcConfigList->clear();
    }
    phyLte->establishmentData->isDetectingSelectedEnb = FALSE;
    phyLte->establishmentData->selectedRntieNB = LTE_INVALID_RNTI;

    // For eNB (specified RNTI is used in these blocks)
    if (phyLte->ueInfoList)
    {
        for (std::map < LteRnti, PhyLteConnectedUeInfo >::iterator it =
            phyLte->ueInfoList->begin(); it != phyLte->ueInfoList->end();
            ++it)
        {
            if (it->first == rnti)
            {
                phyLte->ueInfoList->erase(it);
                break;
            }
        }
    }
    if (phyLte->raGrantsToSend)
    {
        phyLte->raGrantsToSend->erase(rnti);
    }
    
    // reset channels
    int numChannels = PROP_NumberChannels(node);
    for (int i = 0; i < numChannels; i++)
    {
        if ((phyLte->dlChannelNo != i) &&
            (PHY_IsListeningToChannel(node, phyIndex, i)))
        {
            PHY_StopListeningToChannel(node, phyIndex, i);
        }
        if ((phyLte->dlChannelNo == i) &&
            (!PHY_IsListeningToChannel(node, phyIndex, i)))
        {
            PHY_StartListeningToChannel(node, phyIndex, i);
        }
    }
    PHY_SetTransmissionChannel(node, phyIndex, phyLte->ulChannelNo);
}

// /**
// FUNCTION   :: PhyLteResetToPowerOnState
// LAYER      :: PHY
// PURPOSE    :: reset to poweron state
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// RETURN     :: void             : NULL
// **/
void PhyLteResetToPowerOnState(Node* node, const int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    int memSize = sizeof(double) * phyLte->numResourceBlocks;
    memset(phyLte->interferencePower_mW, 0, memSize);

    phyLte->establishmentData->selectedRntieNB = LTE_INVALID_RNTI;

    PHY_SetTransmissionChannel(node, phyIndex, phyLte->ulChannelNo);

    // Calculate transmission signal duration.
    clocktype ttiInterval_ns = MacLteGetTtiLength(node, phyIndex);
    phyLte->txDuration = (clocktype)
                        (ttiInterval_ns - PHY_LTE_SIGNAL_DURATION_REDUCTION);

    // Initialization of CQI values (UE only)
    phyLte->nextReportedCqiInfo.cqi0 = PHY_LTE_INVALID_CQI;
    phyLte->nextReportedCqiInfo.cqi1 = PHY_LTE_INVALID_CQI;
    phyLte->nextReportedRiInfo = PHY_LTE_INVALID_RI;

    PhyLteChangeState(node,
                          phyIndex,
                          phyLte,
                          PHY_LTE_IDLE_CELL_SELECTION);
}


// /**
// FUNCTION   :: LteInterferenceArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle interference signal arrival from the chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving interference signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// + sigPower_mW  : double      : The inband interference power in mW
// RETURN     :: void : NULL
// **/
void PhyLteInterferenceArrivalFromChannel(Node* node,
                                    int phyIndex,
                                    int channelIndex,
                                    PropRxInfo *propRxInfo,
                                    double sigPower_mW)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    // Get signal type
    PhyLteTxInfo* lteTxInfo =
                    PhyLteGetMessageTxInfo(node,
                                   phyIndex,
                                   propRxInfo->txMsg);
    if (lteTxInfo == NULL)
    {
        return;
    }

    if (phyLte->rxState == PHY_LTE_POWER_OFF)
    {
        return;
    }

    // If received channel is not the one "node" using for
    // reception, do nothing.
    if (((channelIndex != phyLte->ulChannelNo)
         && (phyLte->stationType == LTE_STATION_TYPE_ENB))
        || ((channelIndex != phyLte->dlChannelNo)
            && (phyLte->stationType == LTE_STATION_TYPE_UE)))
    {
        return;
    }

    PropTxInfo* propTxInfo =
        (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);

    LteRnti target = LteRnti(propTxInfo->txNodeId, propTxInfo->phyIndex);

    // Calculate propagation delay only for UE
    if ((phyLte->stationType == LTE_STATION_TYPE_UE)
        && PhyLteIsInStationaryState(node, phyIndex) == true
        && (FALSE == PhyLteCheckRxDisconnected(node,
                                               phyIndex,
                                               phyLte,
                                               target)))
    {
        PhyLteSetPropagationDelay(node, phyIndex, propTxInfo);
    }

    // Calculate pathloss excluding fading.
    double pathloss_dB =
        PhyLteGetPathloss_dB(node, phyIndex, lteTxInfo, propRxInfo);

    // The message with NoTransportBlockInfo and the message whose
    // payload size is 0 don't interfere in the desired signal.
    if ((PhyLteIsMessageNoTransportBlock(node,
                                         phyIndex,
                                         propRxInfo->txMsg)) ||
        ((!PhyLteIsMessageNoTransportBlock(node,
                                           phyIndex,
                                           propRxInfo->txMsg)) &&
         (MESSAGE_ReturnPacketSize(propRxInfo->txMsg) == 0) &&
         (MESSAGE_ReturnVirtualPacketSize(propRxInfo->txMsg) == 0)))
    {
        return;
    }

    PhyLteRxMsgInfo* newRxMsgInfo = NULL;
    newRxMsgInfo =
        PhyLteCreateRxMsgInfo(node,
                              phyIndex,
                              phyLte,
                              propRxInfo,
                              NON_DB(pathloss_dB),
                              NON_DB(lteTxInfo->txPower_dBm));
    if (newRxMsgInfo == NULL)
    {
        return;
    }

    newRxMsgInfo->rxArrivalTime = node->getNodeTime();
    newRxMsgInfo->geometry = 1.0 / NON_DB(pathloss_dB);
    newRxMsgInfo->txPower_mW = NON_DB(lteTxInfo->txPower_dBm);

    newRxMsgInfo->rxPower_mW =
        NON_DB(lteTxInfo->txPower_dBm - pathloss_dB);

    newRxMsgInfo->propTxInfo = *propTxInfo; // Copy
    newRxMsgInfo->lteTxInfo = *lteTxInfo;   // Copy

    // Check packet error
    PhyLteCheckAllOfRxPacketError( node, phyIndex );

    // Process inteference signals
    // Append new receive message to interference message list.
    phyLte->ifPackMsgList->push_back(newRxMsgInfo);

    // Append interference power cheking RB allocation infor for each TBs
    Message* ifMsg = newRxMsgInfo->rxMsg;
    Message* prevMsg = NULL;

    double interferencePower_mW =
        NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
               IN_DB(sigPower_mW));

    while (ifMsg != NULL)
    {
        UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB];
        memset(usedRB_list, 0, sizeof(usedRB_list));
        BOOL hasDci = FALSE;

        // Retrieve RB allocation info
        LteDciFormat0* DciForUl =
            (LteDciFormat0*)MESSAGE_ReturnInfo(
                ifMsg,
                (unsigned short)INFO_TYPE_LteDci0Info);

       // Skip a DCI format 0 message
        if (DciForUl != NULL)
        {
            ifMsg = ifMsg->next;
            continue;
        }

        // Retrieve resource allocation infomation
        hasDci = PhyLteGetResourceAllocationInfo(
            node,
            phyIndex,
            ifMsg,
            usedRB_list,
            NULL);

        if (hasDci)
        {
            // Add interference power
            for (UInt8 i = 0; i < phyLte->numResourceBlocks; i++)
            {
                if (usedRB_list[i] != 0)
                {
                    phyLte->interferencePower_mW[i]
                        += interferencePower_mW;
                }
            }
        }

        ifMsg = ifMsg->next;
    }

    // State transition
    if (PhyLteIsInStationaryState(node, phyIndex) == true)
    {
        if (phyLte->stationType == LTE_STATION_TYPE_UE)
        {
            PhyLteChangeState(node, phyIndex, phyLte,
                              PHY_LTE_DL_FRAME_RECEIVING);
        }
        else // LTE_STATION_TYPE_ENB
        {
            PhyLteChangeState(node, phyIndex, phyLte,
                              PHY_LTE_UL_FRAME_RECEIVING);
        }
    }
}


// /**
// FUNCTION   :: LteInterferenceEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle interference signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving interference signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// + sigPower_mW  : double      : The inband interference power in mW
// RETURN     :: void : NULL
// **/
void PhyLteInterferenceEndFromChannel(Node* node,
                                int phyIndex,
                                int channelIndex,
                                PropRxInfo *propRxInfo,
                                double sigPower_mW)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PropTxInfo* propTxInfo =
        (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);

    PhyLteTxInfo* lteTxInfo =
                    PhyLteGetMessageTxInfo(node,
                                       phyIndex,
                                       propRxInfo->txMsg);
    double interferencePower_mW =
        NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
               IN_DB(sigPower_mW));

    if (lteTxInfo == NULL)
    {
        return;
    }

    if (phyLte->rxState == PHY_LTE_POWER_OFF)
    {
        return;
    }

    // If received channel is not the one "node" using for reception, do nothing.
    if (((channelIndex != phyLte->ulChannelNo)
         && (phyLte->stationType == LTE_STATION_TYPE_ENB))
        || ((channelIndex != phyLte->dlChannelNo)
            && (phyLte->stationType == LTE_STATION_TYPE_UE)))
    {
        return;
    }

    // The message with NoTransportBlockInfo and the message whose
    // payload size is 0 don't interfere in the desired signal.
    if ((PhyLteIsMessageNoTransportBlock(node,
                                         phyIndex,
                                         propRxInfo->txMsg)) ||
        ((!PhyLteIsMessageNoTransportBlock(node,
                                           phyIndex,
                                           propRxInfo->txMsg)) &&
         (MESSAGE_ReturnPacketSize(propRxInfo->txMsg) == 0) &&
         (MESSAGE_ReturnVirtualPacketSize(propRxInfo->txMsg) == 0)))
    {
        return;
    }

    // Check packet error for all of the receiving signals
    PhyLteCheckAllOfRxPacketError(node, phyIndex);

    PhyLteRxMsgInfo* rxMsgInfo = NULL;

    // search if in interference signal list, if found, remove it
    rxMsgInfo = PhyLteGetRxMsgInfo(node,
                                   phyLte->ifPackMsgList,
                                   propRxInfo->txMsg,
                                   TRUE);

    // if in interference list, delete from interference power
    if (rxMsgInfo != NULL)
    {
        // Eliminate interference power
        Message* ifMsg = rxMsgInfo->rxMsg;
        while (ifMsg != NULL)
        {
            UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB];
            memset(usedRB_list, 0, sizeof(usedRB_list));
            BOOL hasDci = FALSE;

            // Retrieving RB allocation information
            LteDciFormat0* DciForUl =
                (LteDciFormat0*)MESSAGE_ReturnInfo(
                    ifMsg,
                    (unsigned short)INFO_TYPE_LteDci0Info);

            // Skip a message which has DciForUe
            if (DciForUl != NULL)
            {
                ifMsg = ifMsg->next;
                continue;
            }

            // Retrieve resource allocation infomation
            hasDci = PhyLteGetResourceAllocationInfo(
                node,
                phyIndex,
                ifMsg,
                usedRB_list,
                NULL);

            if (hasDci)
            {
                // Eliminate interference power
                for (UInt8 i = 0; i < phyLte->numResourceBlocks; i++)
                {
                    if (usedRB_list[i] != 0)
                    {
                        phyLte->interferencePower_mW[i] -=
                            interferencePower_mW;

                        if (phyLte->interferencePower_mW[i] < 0.0)
                        {
                            phyLte->interferencePower_mW[i] = 0.0;
                        }
                    }
                }
            }

            ifMsg = ifMsg->next;

        }

        // free the rxMsgInfo(Unpacked message=TBs)
        PhyLteFreeRxMsgInfo(node,
                                phyIndex,
                                rxMsgInfo,
                                phyLte->ifPackMsgList);
        rxMsgInfo = NULL;
    }

    // State transition
    if ((phyLte->rxPackMsgList->empty() == TRUE) &&
        (phyLte->ifPackMsgList->empty() == TRUE))
    {
        if (PhyLteIsInStationaryState(node, phyIndex) == true)
        {
            if (phyLte->stationType == LTE_STATION_TYPE_UE)
            {
                PhyLteChangeState(node, phyIndex, phyLte,
                                  PHY_LTE_DL_IDLE);
            }
            else // LTE_STATION_TYPE_ENB
            {
                PhyLteChangeState(node, phyIndex, phyLte,
                                  PHY_LTE_UL_IDLE);
            }
        }
    }
}


#ifdef ADDON_DB
// /**
// FUNCTION   :: PhyLteGetPhyControlInfoSize
// LAYER      :: PHY
// PURPOSE    :: To get size of various control infos attached during packet
//               transmition in APIs PhyLteStartTransmittingSignal(...), and
//               PhyLteStartTransmittingSignalInEstablishment(...).
//               This API needs an update on adding new control infos in LTE.
// PARAMETERS ::
// + node           : Node*       : Pointer to node.
// + phyIndex       : Int32       : Index of the PHY.
// + channelIndex   : Int32       : Index of the channel
// + msg            : Message*    : Pointer to the Message
// RETURN     :: Int32            : Returns various control size attached.
// **/
Int32 PhyLteGetPhyControlInfoSize(Node* node,
                                  Int32 phyIndex,
                                  Int32 channelIndex,
                                  Message* msg)
{
    Int32 controlSize = 0;

    // Check whether the received msg is a preamble message
    Int32 preambleInfoSize = MESSAGE_ReturnInfoSize(msg,
                       (UInt16)INFO_TYPE_LtePhyRandamAccessTransmitPreamble);

    if (preambleInfoSize)
    {
        controlSize += preambleInfoSize;

        // currrently LTE PHY Preamble Msg which sent through API
        // PhyLteStartTransmittingSignalInEstablishment(...) does not contain
        // any other control info, so just return here with the Preamble size
        return controlSize;
    }

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    // Check for RI info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                                          (UInt16)INFO_TYPE_LtePhyRiInfo);

    // Check for CQI info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                                          (UInt16)INFO_TYPE_LtePhyCqiInfo);

    // Check for SRS info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                                          (UInt16)INFO_TYPE_LtePhySrsInfo);

    // Check for BSR info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                         (UInt16)INFO_TYPE_LteMacPeriodicBufferStatusReport);

    // Check for RRC-connected info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                         (UInt16)INFO_TYPE_LtePhyRrcConnectionSetupComplete);

    // Check for RLC reset info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                                        (UInt16)INFO_TYPE_LteRlcAmResetData);

    // Check for DL TTI info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                                          (UInt16)INFO_TYPE_LtePhyDlTtiInfo);

    // Check for PSS info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                                          (UInt16)INFO_TYPE_LtePhyPss);

    // Check for RA Grant info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                                  (UInt16)INFO_TYPE_LtePhyRandamAccessGrant);

    // Check for RLC reset info
    controlSize += MESSAGE_ReturnInfoSize(msg,
                                        (UInt16)INFO_TYPE_LteRlcAmResetData);
    return controlSize;
}


// /**
// FUNCTION   :: PhyLteUpdateEventsTable
// LAYER      :: PHY
// PURPOSE    :: Updates Stats-DB phy events table for the received messages
// PARAMETERS ::
// + node           : Node*       : Pointer to node.
// + phyIndex       : Int32       : Index of the PHY.
// + channelIndex   : Int32       : Index of the channel
// + propRxInfo     : PropRxInfo* : Pointer to propRxInfo
// + msgToMac       : Message*    : Pointer to the Message
// + eventStr       : std::string : Receives eventType
// RETURN     :: void : NULL
// **/
void PhyLteUpdateEventsTable(Node* node,
                             Int32 phyIndex,
                             Int32 channelIndex,
                             PropRxInfo* propRxInfo,
                             Message* msgToMac,
                             std::string eventStr)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    StatsDb* db = node->partitionData->statsDb;

    NodeId* nextHop = (NodeId*)MESSAGE_ReturnInfo(propRxInfo->txMsg,
                                              INFO_TYPE_MessageNextPrevHop);
    NodeId rcvdId;

    if (nextHop != NULL)
    {
        // use ANY_ADDRESS
        rcvdId  = *nextHop;
    }
    else
    {
        rcvdId = ANY_DEST;
    }

    if (db != NULL
        && db->statsEventsTable->createPhyEventsTable
        && (rcvdId == node->nodeId || rcvdId == ANY_DEST))
    {
        StatsDBMappingParam* mapParamInfo = (StatsDBMappingParam*)
                                    MESSAGE_ReturnInfo(
                                            propRxInfo->txMsg,
                                            INFO_TYPE_StatsDbMapping);

        if (!mapParamInfo)
        {
            ERROR_ReportWarning("Failed to retrieve mapping info from "
                    " the received msg in PhyLteUpdateEventsTable()\n");
            return;
        }

        Int32 controlSize = 0;
        Int32 msgSize = 0;

        if (!msgToMac)
        {
            if (!propRxInfo->txMsg->isPacked)
            {
                msgSize = MESSAGE_ReturnPacketSize(propRxInfo->txMsg);
            }
            else
            {
                msgSize = MESSAGE_ReturnActualPacketSize(propRxInfo->txMsg);
            }
        }
        else
        {
            if (!msgToMac->isPacked)
            {
                msgSize = MESSAGE_ReturnPacketSize(msgToMac);
            }
            else
            {
                msgSize = MESSAGE_ReturnActualPacketSize(msgToMac);
            }
        }

        StatsDBPhyEventParam phyParam(node->nodeId,
                                      (std::string)mapParamInfo->msgId,
                                      phyIndex,
                                      msgSize,
                                      eventStr);

        phyParam.SetChannelIndex(channelIndex);

        if (!eventStr.compare("PhyReceiveSignal"))
        {
            // Get signal type
            PhyLteTxInfo* lteTxInfo = PhyLteGetMessageTxInfo(node,
                                                         phyIndex,
                                                         propRxInfo->txMsg);
            // Calculate pathloss excluding fading.
            double pathloss_dB =
                PhyLteGetPathloss_dB(node, phyIndex, lteTxInfo, propRxInfo);

            phyParam.SetPathLoss(pathloss_dB);
            phyParam.SetSignalPower(lteTxInfo->txPower_dBm - pathloss_dB);

            if (*phyLte->interferencePower_mW)
            {
                phyParam.SetInterference(
                                       IN_DB(*phyLte->interferencePower_mW));
            }
            else
            {
                phyParam.SetInterference(0.0);
            }
        }

        if (eventStr != "PhySendToUpper")
        {
            controlSize = PhyLteGetPhyControlInfoSize(node,
                                                      phyIndex,
                                                      channelIndex,
                                                      propRxInfo->txMsg);
        }

        phyParam.SetControlSize(controlSize);
        STATSDB_HandlePhyEventsTableInsert(node, phyParam);
    }
}


// /**
// FUNCTION   :: PhyLteNotifyPacketDropForMsgInRxPackedMsg
// LAYER      :: PHY
// PURPOSE    :: This API is used to Update event "PhyDrop" in Stats-DB phy
//               events table for those messages only are a part of the
//               Packed message received on PHY. For others places,
//               default API PHY_NotificationOfPacketDrop() is used.
// PARAMETERS ::
// + node                 : Node*       : Pointer to node.
// + phyIndex             : Int32       : Index of the PHY.
// + channelIndex         : Int32       : Index of the channel
// + propRxInfoTxMsg      : Message*    : Pointer to propRxInfo->txMsg
// + dropType             : std::string : Receives drop reason
// + rxPower_dB           : double      : Rx power in dB
// + pathloss_dB          : double      : Pathloss in dB
// + msgInRxPackedMsg     : Message*    : Pointer to the message in received
//                                        packed message
// RETURN     :: void     : NULL
// **/
void PhyLteNotifyPacketDropForMsgInRxPackedMsg(
                                    Node* node,
                                    Int32 phyIndex,
                                    Int32 channelIndex,
                                    Message* propRxInfoTxMsg,
                                    const string& dropType,
                                    double rxPower_dB,
                                    double pathloss_dB,
                                    Message* msgInRxPackedMsg)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    StatsDb* db = node->partitionData->statsDb;

    NodeId* nextHop = (NodeId*)MESSAGE_ReturnInfo(propRxInfoTxMsg,
                                              INFO_TYPE_MessageNextPrevHop);
    NodeId rcvdId;

    if (nextHop != NULL)
    {
        // use ANY_ADDRESS
        rcvdId  = *nextHop;
    }
    else
    {
        rcvdId = ANY_DEST;
    }

    if (db != NULL
        && db->statsEventsTable->createPhyEventsTable
        && (rcvdId == node->nodeId || rcvdId == ANY_DEST))
    {
        StatsDBMappingParam* mapParamInfo = (StatsDBMappingParam*)
                                    MESSAGE_ReturnInfo(
                                            msgInRxPackedMsg,
                                            INFO_TYPE_StatsDbMapping);

        if (!mapParamInfo)
        {
            ERROR_ReportWarning("Failed to retrieve mapping info from "
                    " the received msg in "
                    "PhyLtePacketDropForMsgInRxPackedMsg()\n");
            return;
        }

        Int32 controlSize = 0;
        Int32 msgSize = 0;

        if (!msgInRxPackedMsg->isPacked)
        {
            msgSize = MESSAGE_ReturnPacketSize(msgInRxPackedMsg);
        }
        else
        {
            msgSize = MESSAGE_ReturnActualPacketSize(msgInRxPackedMsg);
        }

        StatsDBPhyEventParam phyParam(node->nodeId,
                                      (std::string)mapParamInfo->msgId,
                                      phyIndex,
                                      msgSize,
                                      "PhyDrop");

        phyParam.SetChannelIndex(channelIndex);
        phyParam.SetSignalPower(rxPower_dB);
        phyParam.SetPathLoss(pathloss_dB);

        if (*phyLte->interferencePower_mW)
        {
            phyParam.SetInterference(IN_DB(*phyLte->interferencePower_mW));
        }
        else
        {
            phyParam.SetInterference(0.0);
        }

        phyParam.SetMessageFailureType(dropType);
        phyParam.SetControlSize(controlSize);

        STATSDB_HandlePhyEventsTableInsert(node, phyParam);
    }
}
#endif // ADDON_DB
