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
#include "antenna.h"
#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"
#include "phy_802_11.h"
#include "phy_802_11n.h"

#include "mac_csma.h"
#include "mac_dot11.h"
#include "mac_dot11-sta.h"
#include "mac_phy_802_11n.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#undef DEBUG
#define DEBUG 0

static
void Phy802_11ChangeState(
    Node* node,
    int phyIndex,
    PhyStatusType newStatus)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11 *phy802_11 = (PhyData802_11 *)thisPhy->phyVar;
    phy802_11->previousMode = phy802_11->mode;
    phy802_11->mode = newStatus;


    Phy_ReportStatusToEnergyModel(
        node,
        phyIndex,
        phy802_11->previousMode,
        newStatus);
}

double
Phy802_11GetSignalStrength(Node *node,PhyData802_11* phy802_11)
{
    double rxThreshold_mW;
    int dataRateToUse;
    Phy802_11GetLowestTxDataRateType(phy802_11->thisPhy, &dataRateToUse);
    rxThreshold_mW = phy802_11->rxSensitivity_mW[dataRateToUse];
    return IN_DB(rxThreshold_mW);
}


double
Phy802_11nGetSignalStrength(Node* node,PhyData802_11* phy802_11)
{
    double rxThreshold_mW;
    MAC_PHY_TxRxVector vector;
    Phy802_11nGetTxVectorForLowestDataRate(phy802_11->thisPhy, vector);
    UInt8 i = vector.chBwdth
            == (ChBandwidth)CHBWDTH_20MHZ? 0 : 1;
    rxThreshold_mW = phy802_11->pDot11n->Min_Rx_Senstivity[i][0];
    return rxThreshold_mW;
}

static
void Phy802_11ReportExtendedStatusToMac(
    Node *node,
    int phyNum,
    PhyStatusType status,
    clocktype receiveDuration,
    Message* potentialIncomingPacket)
{
    PhyData* thisPhy = node->phyData[phyNum];
    PhyData802_11* phy802_11 = (PhyData802_11 *)thisPhy->phyVar;

    assert(status == phy802_11->mode);

    if (potentialIncomingPacket != NULL) {
        MESSAGE_RemoveHeader(
            node,
            potentialIncomingPacket,
            sizeof(Phy802_11PlcpHeader),
            TRACE_802_11);
    }

    MAC_ReceivePhyStatusChangeNotification(
        node, thisPhy->macInterfaceIndex,
        phy802_11->previousMode, status,
        receiveDuration, potentialIncomingPacket);

    if (potentialIncomingPacket != NULL) {
        MESSAGE_AddHeader(
            node,
            potentialIncomingPacket,
            sizeof(Phy802_11PlcpHeader),
            TRACE_802_11);
    }
}

static
void Phy802_11ReportStatusToMac(
    Node *node, int phyNum, PhyStatusType status)
{
    Phy802_11ReportExtendedStatusToMac(
        node, phyNum, status, 0, NULL);
}

static
void Phy802_11AddPlcpHeader(Node* node,
                            PhyData802_11* phy802_11,
                            Message* packet)
{
    MESSAGE_AddHeader(node, packet, sizeof(Phy802_11PlcpHeader),
                      TRACE_802_11);

    Phy802_11PlcpHeader *plcp = (Phy802_11PlcpHeader*)
                                    MESSAGE_ReturnPacket(packet);
    plcp->txPhyModel = phy802_11->thisPhy->phyModel;
    if (phy802_11->thisPhy->phyModel == PHY802_11n) {
        phy802_11->pDot11n->FillPlcpHdr(plcp);
    }
    else {
        plcp->rate = phy802_11->txDataRateType;
    }
}

static //inline//
void Phy802_11LockSignal(
    Node* node,
    PhyData802_11* phy802_11,
    PropRxInfo* propRxInfo,
    Message* msg,
    double rxPower_mW,
    clocktype rxEndTime,
    Int32 channelIndex,
    const Orientation& txDOA,
    const Orientation& rxDOA,
    int   txNumAtnaElmts,
    double txAtnaElmtSpace)
{

    Phy802_11PlcpHeader* plcp = (Phy802_11PlcpHeader*)
                                    MESSAGE_ReturnPacket(msg);
    if (DEBUG)
    {
        char currTime[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), currTime);
        printf("\ntime %s: LockSignal from %d at %d\n",
               currTime,
               msg->originatingNodeId,
               node->nodeId);
    }

    phy802_11->rxMsg = msg;
    phy802_11->rxMsgError = FALSE;
    phy802_11->rxMsgPower_mW = rxPower_mW;
    phy802_11->rxTimeEvaluated = node->getNodeTime();
    phy802_11->rxEndTime = rxEndTime;
    phy802_11->txNodeId = propRxInfo->txNodeId;
    phy802_11->rxChannelIndex = propRxInfo->channelIndex;
    phy802_11->pathloss_dB = propRxInfo->pathloss_dB;

    if (plcp->txPhyModel == PHY802_11n &&
            phy802_11->thisPhy->phyModel == PHY802_11n)
    {
        phy802_11->pDot11n->LockSignal(node,
                                       channelIndex,
                                       plcp,
                                       txDOA,
                                       rxDOA,
                                       txNumAtnaElmts,
                                       txAtnaElmtSpace);
    }
    else
    {
        phy802_11->rxDataRateType = plcp->rate;
    }

    if (phy802_11->thisPhy->phyStats)
    {
        phy802_11->thisPhy->stats->AddSignalLockedDataPoints(
            node,
            propRxInfo,
            phy802_11->thisPhy,
            channelIndex,
            rxPower_mW);
    }
}

static //inline//
void Phy802_11UnlockSignal(PhyData802_11* phy802_11) {
    phy802_11->rxMsg = NULL;
    phy802_11->rxMsgError = FALSE;
    phy802_11->rxMsgPower_mW = 0.0;
    phy802_11->rxTimeEvaluated = 0;
    phy802_11->rxEndTime = 0;
    phy802_11->txNodeId = 0;
    phy802_11->rxChannelIndex = -1;
    phy802_11->pathloss_dB = 0.0;

    if (phy802_11->pDot11n) {
        phy802_11->pDot11n->UnlockSignal();
    }
}

static
BOOL Phy802_11CarrierSensing(Node* node, PhyData802_11* phy802_11) {

    double rxSensitivity_mW = phy802_11->rxSensitivity_mW[0];

    if (!ANTENNA_IsInOmnidirectionalMode(node, phy802_11->
        thisPhy->phyIndex)) {
        rxSensitivity_mW =
            NON_DB(IN_DB(rxSensitivity_mW) +
                   phy802_11->directionalAntennaGain_dB);
    }//if//

    if ((phy802_11->interferencePower_mW + phy802_11->noisePower_mW) >
        rxSensitivity_mW)
    {
        return TRUE;
    }

    return FALSE;
}

static //inline//
BOOL Phy802_11CheckRxPacketError(
    Node* node,
    PhyData802_11* phy802_11,
    double *sinrPtr)
{
    double sinr;
    double BER;
    double noise =
        phy802_11->thisPhy->noise_mW_hz * phy802_11->channelBandwidth;
    double dataRate;

    assert(phy802_11->rxMsgError == FALSE);

    if (phy802_11->thisPhy->phyModel == PHY802_11b &&
        (phy802_11->rxDataRateType == PHY802_11b__6M ||
         phy802_11->rxDataRateType == PHY802_11b_11M))
    {
        noise = noise * 11.0;
    }

    sinr = (phy802_11->rxMsgPower_mW /
            (phy802_11->interferencePower_mW + noise));

    if (sinrPtr != NULL)
    {
        *sinrPtr = sinr;
    }
    BOOL containAMPDU = FALSE;
    if (phy802_11->thisPhy->phyModel == PHY802_11n) {

        BER = phy802_11->pDot11n->CheckBer(sinr);

        dataRate = phy802_11->pDot11n->GetDataRateOfRxSignal();
        Phy802_11PlcpHeader *hdr = (Phy802_11PlcpHeader *)
            MESSAGE_ReturnHeader(phy802_11->rxMsg, 0);
        if (hdr->containAMPDU) {
            containAMPDU = TRUE;
            double *BERInfo = NULL;
            BERInfo = (double *)MESSAGE_AddInfo(node,
                phy802_11->rxMsg, sizeof(double), INFO_TYPE_Dot11nBER);
            *((double*) BERInfo) = BER;

        }
    }
    else {
        assert(phy802_11->rxDataRateType >= 0 &&
           phy802_11->rxDataRateType < phy802_11->numDataRates);

        BER = PHY_BER(phy802_11->thisPhy,
                  phy802_11->rxDataRateType,
                  sinr);
        dataRate = (double)phy802_11->dataRate[phy802_11->rxDataRateType];
    }

    if (BER != 0.0) {
        double numBits = 0;
        if (!containAMPDU)
        {
            numBits =
            ((double)(node->getNodeTime() - phy802_11->rxTimeEvaluated) *
             dataRate/(double)SECOND);
        }else {
            numBits = sizeof(Phy802_11PlcpHeader) * 8;
        }

        double errorProbability = 1.0 - pow((1.0 - BER), numBits);
        double rand = RANDOM_erand(phy802_11->thisPhy->seed);

        // Fiddling with the lookahead time can cause timing imprecisions
        if (node->partitionData->looseSynchronization)
        {
            if (errorProbability < 0.0) 
            {
                return false;
            }
            if (errorProbability > 1.0) 
            {
                return true;
            }
        }
        else
        {
            assert((errorProbability >= 0.0) && (errorProbability <= 1.0));
        }

        if (errorProbability > rand) {
            return TRUE;
        }
    }

    return FALSE;
}

static
void Phy802_11aInitializeDefaultParameters(Node* node, int phyIndex) {
    PhyData802_11* phy802_11a =
        (PhyData802_11*)(node->phyData[phyIndex]->phyVar);


    phy802_11a->numDataRates = PHY802_11a_NUM_DATA_RATES;

    phy802_11a->txDefaultPower_dBm[PHY802_11a__6M] =
        PHY802_11a_DEFAULT_TX_POWER__6M_dBm;
    phy802_11a->txDefaultPower_dBm[PHY802_11a__9M] =
        PHY802_11a_DEFAULT_TX_POWER__9M_dBm;
    phy802_11a->txDefaultPower_dBm[PHY802_11a_12M] =
        PHY802_11a_DEFAULT_TX_POWER_12M_dBm;
    phy802_11a->txDefaultPower_dBm[PHY802_11a_18M] =
        PHY802_11a_DEFAULT_TX_POWER_18M_dBm;
    phy802_11a->txDefaultPower_dBm[PHY802_11a_24M] =
        PHY802_11a_DEFAULT_TX_POWER_24M_dBm;
    phy802_11a->txDefaultPower_dBm[PHY802_11a_36M] =
        PHY802_11a_DEFAULT_TX_POWER_36M_dBm;
    phy802_11a->txDefaultPower_dBm[PHY802_11a_48M] =
        PHY802_11a_DEFAULT_TX_POWER_48M_dBm;
    phy802_11a->txDefaultPower_dBm[PHY802_11a_54M] =
        PHY802_11a_DEFAULT_TX_POWER_54M_dBm;


    phy802_11a->rxSensitivity_mW[PHY802_11a__6M] =
        NON_DB(PHY802_11a_DEFAULT_RX_SENSITIVITY__6M_dBm);
    phy802_11a->rxSensitivity_mW[PHY802_11a__9M] =
        NON_DB(PHY802_11a_DEFAULT_RX_SENSITIVITY__9M_dBm);
    phy802_11a->rxSensitivity_mW[PHY802_11a_12M] =
        NON_DB(PHY802_11a_DEFAULT_RX_SENSITIVITY_12M_dBm);
    phy802_11a->rxSensitivity_mW[PHY802_11a_18M] =
        NON_DB(PHY802_11a_DEFAULT_RX_SENSITIVITY_18M_dBm);
    phy802_11a->rxSensitivity_mW[PHY802_11a_24M] =
        NON_DB(PHY802_11a_DEFAULT_RX_SENSITIVITY_24M_dBm);
    phy802_11a->rxSensitivity_mW[PHY802_11a_36M] =
        NON_DB(PHY802_11a_DEFAULT_RX_SENSITIVITY_36M_dBm);
    phy802_11a->rxSensitivity_mW[PHY802_11a_48M] =
        NON_DB(PHY802_11a_DEFAULT_RX_SENSITIVITY_48M_dBm);
    phy802_11a->rxSensitivity_mW[PHY802_11a_54M] =
        NON_DB(PHY802_11a_DEFAULT_RX_SENSITIVITY_54M_dBm);


    phy802_11a->dataRate[PHY802_11a__6M] = PHY802_11a_DATA_RATE__6M;
    phy802_11a->dataRate[PHY802_11a__9M] = PHY802_11a_DATA_RATE__9M;
    phy802_11a->dataRate[PHY802_11a_12M] = PHY802_11a_DATA_RATE_12M;
    phy802_11a->dataRate[PHY802_11a_18M] = PHY802_11a_DATA_RATE_18M;
    phy802_11a->dataRate[PHY802_11a_24M] = PHY802_11a_DATA_RATE_24M;
    phy802_11a->dataRate[PHY802_11a_36M] = PHY802_11a_DATA_RATE_36M;
    phy802_11a->dataRate[PHY802_11a_48M] = PHY802_11a_DATA_RATE_48M;
    phy802_11a->dataRate[PHY802_11a_54M] = PHY802_11a_DATA_RATE_54M;


    phy802_11a->numDataBitsPerSymbol[PHY802_11a__6M] =
        PHY802_11a_NUM_DATA_BITS_PER_SYMBOL__6M;
    phy802_11a->numDataBitsPerSymbol[PHY802_11a__9M] =
        PHY802_11a_NUM_DATA_BITS_PER_SYMBOL__9M;
    phy802_11a->numDataBitsPerSymbol[PHY802_11a_12M] =
        PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_12M;
    phy802_11a->numDataBitsPerSymbol[PHY802_11a_18M] =
        PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_18M;
    phy802_11a->numDataBitsPerSymbol[PHY802_11a_24M] =
        PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_24M;
    phy802_11a->numDataBitsPerSymbol[PHY802_11a_36M] =
        PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_36M;
    phy802_11a->numDataBitsPerSymbol[PHY802_11a_48M] =
        PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_48M;
    phy802_11a->numDataBitsPerSymbol[PHY802_11a_54M] =
        PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_54M;


    phy802_11a->lowestDataRateType = PHY802_11a_LOWEST_DATA_RATE_TYPE;
    phy802_11a->highestDataRateType = PHY802_11a_HIGHEST_DATA_RATE_TYPE;
    phy802_11a->txDataRateTypeForBC = PHY802_11a_DATA_RATE_TYPE_FOR_BC;

    phy802_11a->channelBandwidth = PHY802_11a_CHANNEL_BANDWIDTH;
    phy802_11a->rxTxTurnaroundTime = PHY802_11a_RX_TX_TURNAROUND_TIME;

    return;
}

static
void Phy802_11aSetUserConfigurableParameters(
    Node* node,
    int phyIndex,
    const NodeInput *nodeInput)
{
    double rxSensitivity_dBm;
    double txPower_dBm;
    BOOL   wasFound;

    PhyData802_11* phy802_11a =
        (PhyData802_11*)(node->phyData[phyIndex]->phyVar);


    //
    // Set PHY802_11a-TX-POWER's
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-TX-POWER--6MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11a->txDefaultPower_dBm[PHY802_11a__6M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-TX-POWER--9MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11a->txDefaultPower_dBm[PHY802_11a__9M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-TX-POWER-12MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11a->txDefaultPower_dBm[PHY802_11a_12M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-TX-POWER-18MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11a->txDefaultPower_dBm[PHY802_11a_18M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-TX-POWER-24MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11a->txDefaultPower_dBm[PHY802_11a_24M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-TX-POWER-36MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11a->txDefaultPower_dBm[PHY802_11a_36M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-TX-POWER-48MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11a->txDefaultPower_dBm[PHY802_11a_48M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-TX-POWER-54MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11a->txDefaultPower_dBm[PHY802_11a_54M] = (float)txPower_dBm;
    }


    //
    // Set PHY802_11a-RX-SENSITIVITY's
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-RX-SENSITIVITY--6MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11a->rxSensitivity_mW[PHY802_11a__6M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-RX-SENSITIVITY--9MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11a->rxSensitivity_mW[PHY802_11a__9M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-RX-SENSITIVITY-12MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11a->rxSensitivity_mW[PHY802_11a_12M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-RX-SENSITIVITY-18MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11a->rxSensitivity_mW[PHY802_11a_18M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-RX-SENSITIVITY-24MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11a->rxSensitivity_mW[PHY802_11a_24M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-RX-SENSITIVITY-36MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11a->rxSensitivity_mW[PHY802_11a_36M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-RX-SENSITIVITY-48MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11a->rxSensitivity_mW[PHY802_11a_48M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11a-RX-SENSITIVITY-54MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11a->rxSensitivity_mW[PHY802_11a_54M] =
            NON_DB(rxSensitivity_dBm);
    }


    return;
}



static
void Phy802_11bInitializeDefaultParameters(Node* node, int phyIndex) {
    PhyData802_11* phy802_11b =
        (PhyData802_11*)(node->phyData[phyIndex]->phyVar);


    phy802_11b->numDataRates = PHY802_11b_NUM_DATA_RATES;

    phy802_11b->txDefaultPower_dBm[PHY802_11b__1M] =
        PHY802_11b_DEFAULT_TX_POWER__1M_dBm;
    phy802_11b->txDefaultPower_dBm[PHY802_11b__2M] =
        PHY802_11b_DEFAULT_TX_POWER__2M_dBm;
    phy802_11b->txDefaultPower_dBm[PHY802_11b__6M] =
        PHY802_11b_DEFAULT_TX_POWER__6M_dBm;
    phy802_11b->txDefaultPower_dBm[PHY802_11b_11M] =
        PHY802_11b_DEFAULT_TX_POWER_11M_dBm;


    phy802_11b->rxSensitivity_mW[PHY802_11b__1M] =
        NON_DB(PHY802_11b_DEFAULT_RX_SENSITIVITY__1M_dBm);
    phy802_11b->rxSensitivity_mW[PHY802_11b__2M] =
        NON_DB(PHY802_11b_DEFAULT_RX_SENSITIVITY__2M_dBm);
    phy802_11b->rxSensitivity_mW[PHY802_11b__6M] =
        NON_DB(PHY802_11b_DEFAULT_RX_SENSITIVITY__6M_dBm);
    phy802_11b->rxSensitivity_mW[PHY802_11b_11M] =
        NON_DB(PHY802_11b_DEFAULT_RX_SENSITIVITY_11M_dBm);


    phy802_11b->dataRate[PHY802_11b__1M] = PHY802_11b_DATA_RATE__1M;
    phy802_11b->dataRate[PHY802_11b__2M] = PHY802_11b_DATA_RATE__2M;
    phy802_11b->dataRate[PHY802_11b__6M] = PHY802_11b_DATA_RATE__6M;
    phy802_11b->dataRate[PHY802_11b_11M] = PHY802_11b_DATA_RATE_11M;


    phy802_11b->numDataBitsPerSymbol[PHY802_11b__1M] =
        PHY802_11b_NUM_DATA_BITS_PER_SYMBOL__1M;
    phy802_11b->numDataBitsPerSymbol[PHY802_11b__2M] =
        PHY802_11b_NUM_DATA_BITS_PER_SYMBOL__2M;
    phy802_11b->numDataBitsPerSymbol[PHY802_11b__6M] =
        PHY802_11b_NUM_DATA_BITS_PER_SYMBOL__6M;
    phy802_11b->numDataBitsPerSymbol[PHY802_11b_11M] =
        PHY802_11b_NUM_DATA_BITS_PER_SYMBOL_11M;


    phy802_11b->lowestDataRateType = PHY802_11b_LOWEST_DATA_RATE_TYPE;
    phy802_11b->highestDataRateType = PHY802_11b_HIGHEST_DATA_RATE_TYPE;
    phy802_11b->txDataRateTypeForBC = PHY802_11b_DATA_RATE_TYPE_FOR_BC;

    phy802_11b->rxDataRateType = PHY802_11b_LOWEST_DATA_RATE_TYPE;

    phy802_11b->channelBandwidth = PHY802_11b_CHANNEL_BANDWIDTH;
    phy802_11b->rxTxTurnaroundTime = PHY802_11b_RX_TX_TURNAROUND_TIME;

    return;
}



static
void Phy802_11bSetUserConfigurableParameters(
    Node* node,
    int phyIndex,
    const NodeInput *nodeInput)
{
    double rxSensitivity_dBm;
    double txPower_dBm;
    BOOL   wasFound;

    PhyData802_11* phy802_11b =
        (PhyData802_11*)(node->phyData[phyIndex]->phyVar);


    //
    // Set PHY802_11b-TX-POWER's
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11b-TX-POWER--1MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11b->txDefaultPower_dBm[PHY802_11b__1M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11b-TX-POWER--2MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11b->txDefaultPower_dBm[PHY802_11b__2M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11b-TX-POWER--6MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11b->txDefaultPower_dBm[PHY802_11b__6M] = (float)txPower_dBm;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11b-TX-POWER-11MBPS",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy802_11b->txDefaultPower_dBm[PHY802_11b_11M] = (float)txPower_dBm;
    }


    //
    // Set PHY802_11b-RX-SENSITIVITY's
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11b-RX-SENSITIVITY--1MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11b->rxSensitivity_mW[PHY802_11b__1M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11b-RX-SENSITIVITY--2MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11b->rxSensitivity_mW[PHY802_11b__2M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11b-RX-SENSITIVITY--6MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11b->rxSensitivity_mW[PHY802_11b__6M] =
            NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11b-RX-SENSITIVITY-11MBPS",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy802_11b->rxSensitivity_mW[PHY802_11b_11M] =
            NON_DB(rxSensitivity_dBm);
    }


    return;
}



void Phy802_11Init(
    Node *node,
    const int phyIndex,
    const NodeInput *nodeInput,
    BOOL  initN)
{
    BOOL   wasFound;
    BOOL   yes;
    int dataRateForBroadcast;
    int i;
    int numChannels = PROP_NumberChannels(node);

    PhyData802_11 *phy802_11 =
        (PhyData802_11 *)MEM_malloc(sizeof(PhyData802_11));

    memset(phy802_11, 0, sizeof(PhyData802_11));

    node->phyData[phyIndex]->phyVar = (void*)phy802_11;

    phy802_11->thisPhy = node->phyData[phyIndex];

    std::string path;
    D_Hierarchy *h = &node->partitionData->dynamicHierarchy;

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "802.11",
            "energyConsumed",
            path))
    {
        h->AddObject(
            path,
            new D_Float64Obj(&phy802_11->stats.energyConsumed));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "802.11",
            "turnOnTime",
            path))
    {
        h->AddObject(
            path,
            new D_ClocktypeObj(&phy802_11->stats.turnOnTime));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "802.11",
            "txPower_dBm",
            path))
    {
        h->AddObject(
            path,
            new D_Float32Obj(&phy802_11->txPower_dBm));
    }

    if (h->CreatePhyPath(
            node,
            phyIndex,
            "802.11",
            "channelBandwidth",
            path))
    {
        h->AddObject(
            path,
            new D_Int32Obj(&phy802_11->channelBandwidth));
    }

    if (phy802_11->thisPhy->phyStats)
    {
        phy802_11->thisPhy->stats = new STAT_PhyStatistics(node);
    }

    //
    // Antenna model initialization
    //
    ANTENNA_Init(node, phyIndex, nodeInput);

    if (node->phyData[phyIndex]->phyModel == PHY802_11a) {
        Phy802_11aInitializeDefaultParameters(node, phyIndex);
        Phy802_11aSetUserConfigurableParameters(node, phyIndex, nodeInput);
    }
    else if (node->phyData[phyIndex]->phyModel == PHY802_11b) {
        Phy802_11bInitializeDefaultParameters(node, phyIndex);
        Phy802_11bSetUserConfigurableParameters(node, phyIndex, nodeInput);
    }
    else if (node->phyData[phyIndex]->phyModel != PHY802_11n) {
        ERROR_ReportError(
            "Phy802_11Init() needs to be called for an 802.11 model");
    }

    ERROR_Assert(phy802_11->thisPhy->phyRxModel != SNR_THRESHOLD_BASED,
                 "802.11 PHY model does not work with "
                 "SNR_THRESHOLD_BASED reception model.\n");

    //
    // Data rate options
    //
    if (node->phyData[phyIndex]->phyModel == PHY802_11a ||
        node->phyData[phyIndex]->phyModel == PHY802_11b) {

        IO_ReadBool(
            node,
            node->nodeId,
            node->phyData[phyIndex]->macInterfaceIndex,
            nodeInput,
            "PHY802.11-AUTO-RATE-FALLBACK",
            &wasFound,
            &yes);

        if (!wasFound || yes == FALSE) {
            BOOL wasFound1;
            int dataRate;

            IO_ReadInt(
                node,
                node->nodeId,
                node->phyData[phyIndex]->macInterfaceIndex,
                nodeInput,
                "PHY802.11-DATA-RATE",
                &wasFound1,
                &dataRate);

            if (wasFound1) {
                for (i = 0; i < phy802_11->numDataRates; i++) {
                    if (dataRate == phy802_11->dataRate[i]) {
                        break;
                    }
                }
                if (i >= phy802_11->numDataRates) {
                    ERROR_ReportError(
                        "Specified PHY802.11-DATA-RATE is not "
                        "in the supported data rate set");
                }

                phy802_11->lowestDataRateType = i;
                phy802_11->highestDataRateType = i;
            }
            else {
                ERROR_ReportError(
                    "PHY802.11-DATA-RATE not set without "
                    "PHY802.11-AUTO-RATE-FALLBACK turned on");
            }
        }

        Phy802_11SetLowestTxDataRateType(phy802_11->thisPhy);

        //
        // Set PHY802_11-DATA-RATE-FOR-BROADCAST
        //
        IO_ReadInt(
            node,
            node->nodeId,
            node->phyData[phyIndex]->macInterfaceIndex,
            nodeInput,
            "PHY802.11-DATA-RATE-FOR-BROADCAST",
            &wasFound,
            &dataRateForBroadcast);

        if (wasFound) {
            for (i = 0; i < phy802_11->numDataRates; i++) {
                if (dataRateForBroadcast == phy802_11->dataRate[i]) {
                    break;
                }
            }

            if (i < phy802_11->lowestDataRateType ||
                i > phy802_11->highestDataRateType)
            {
                ERROR_ReportError(
                    "Specified PHY802.11-DATA-RATE-FOR-BROADCAST is not "
                    "in the data rate set");
            }

            phy802_11->txDataRateTypeForBC = i;
        }
        else {
            phy802_11->txDataRateTypeForBC =
                MIN(phy802_11->highestDataRateType,
                    MAX(phy802_11->lowestDataRateType,
                        phy802_11->txDataRateTypeForBC));
        }
    }

    //
    // Set PHY802_11-ESTIMATED-DIRECTIONAL-ANTENNA-GAIN
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11-ESTIMATED-DIRECTIONAL-ANTENNA-GAIN",
        &wasFound,
        &(phy802_11->directionalAntennaGain_dB));

    if (!wasFound &&
        (phy802_11->thisPhy->antennaData->antennaModelType
            != ANTENNA_OMNIDIRECTIONAL &&
          phy802_11->thisPhy->antennaData->antennaModelType
            != ANTENNA_PATTERNED))
    {
        ERROR_ReportError(
            "PHY802.11-ESTIMATED-DIRECTIONAL-ANTENNA-GAIN is missing\n");
    }


    //
    // Initialize phy statistics variables
    //
    phy802_11->stats.energyConsumed = 0.0;
#ifdef CYBER_LIB
    phy802_11->stats.totalRxSignalsToMacDuringJam = 0;
    phy802_11->stats.totalSignalsLockedDuringJam = 0;
    phy802_11->stats.totalSignalsWithErrorsDuringJam = 0;
#endif
    phy802_11->stats.turnOnTime = node->getNodeTime();
#ifdef ADDON_DB
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

    //
    // Initialize status of phy
    //
    phy802_11->rxMsg = NULL;
    phy802_11->rxMsgError = FALSE;
    phy802_11->rxMsgPower_mW = 0.0;
    phy802_11->interferencePower_mW = 0.0;
    phy802_11->txNodeId = 0;
    phy802_11->rxChannelIndex = -1;
    phy802_11->pathloss_dB = 0.0;
    phy802_11->noisePower_mW =
        phy802_11->thisPhy->noise_mW_hz * phy802_11->channelBandwidth;
    phy802_11->rxTimeEvaluated = 0;
    phy802_11->rxEndTime = 0;
    phy802_11->rxDOA.azimuth = 0;
    phy802_11->rxDOA.elevation = 0;
    phy802_11->previousMode = PHY_IDLE;
    phy802_11->mode = PHY_IDLE;
    Phy802_11ChangeState(node,phyIndex, PHY_IDLE);

    //
    // Setting up the channel to use for both TX and RX
    //
    for (i = 0; i < numChannels; i++) {
        if (phy802_11->thisPhy->channelListening[i] == TRUE) {
            break;
        }
    }
    ERROR_Assert(
        i != numChannels,
        "802.11 radio must listen to at least one channel");
    PHY_SetTransmissionChannel(node, phyIndex, i);

    //Initialize 802.11n data
    if (initN) {
        phy802_11->pDot11n = new Phy802_11n(phy802_11);
        phy802_11->pDot11n->Init(node, nodeInput);
    }
    return;
}


void Phy802_11ChannelListeningSwitchNotification(
   Node* node,
   int phyIndex,
   int channelIndex,
   BOOL startListening)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11 *)thisPhy->phyVar;

   if (phy802_11 == NULL)
    {
        // not initialized yet, return;
        return;
    }

    if (startListening == TRUE)
    {
        if (phy802_11->mode == PHY_TRANSMITTING)
        {
            Phy802_11TerminateCurrentTransmission(node,phyIndex);
        }
        PHY_SignalInterference(
            node,
            phyIndex,
            channelIndex,
            NULL,
            NULL,
            &(phy802_11->interferencePower_mW));

        if (Phy802_11CarrierSensing(node, phy802_11) == TRUE) {
            Phy802_11ChangeState(node,phyIndex, PHY_SENSING);
        }
        else {
            Phy802_11ChangeState(node,phyIndex, PHY_IDLE);
        }

        if (phy802_11->previousMode != phy802_11->mode)
            Phy802_11ReportStatusToMac(node, phyIndex, phy802_11->mode);
    }
    else if (phy802_11->mode != PHY_TRANSMITTING)
    {

        if (phy802_11->mode == PHY_RECEIVING)
        {
            BOOL frameError;
            clocktype endTime;

            Phy802_11TerminateCurrentReceive(node,
                                             phyIndex,
                                             FALSE,
                                             &frameError,
                                             &endTime);
        }

        Phy802_11ChangeState(node,phyIndex, PHY_TRX_OFF);
    }

}


void Phy802_11TransmissionEnd(Node *node, int phyIndex) {
    if (DEBUG)
    {
        char clockStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("PHY_802_11: node %d transmission end at time %s\n",
               node->nodeId, clockStr);
    }
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11 *)thisPhy->phyVar;
    int channelIndex;

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    assert(phy802_11->mode == PHY_TRANSMITTING);

    phy802_11->txEndTimer = NULL;
    //GuiStart
    if (node->guiOption == TRUE) {
        GUI_EndBroadcast(node->nodeId,
                         GUI_PHY_LAYER,
                         GUI_DEFAULT_DATA_TYPE,
                         thisPhy->macInterfaceIndex,
                         node->getNodeTime());
    }
    //GuiEnd

    if (!ANTENNA_IsLocked(node, phyIndex)) {
        ANTENNA_SetToDefaultMode(node, phyIndex);
    }//if//

    PHY_StartListeningToChannel(node, phyIndex, channelIndex);

}


BOOL Phy802_11MediumIsIdle(Node* node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*)thisPhy->phyVar;
    BOOL IsIdle;
    int channelIndex;
    double oldInterferencePower = phy802_11->interferencePower_mW;

    assert((phy802_11->mode == PHY_IDLE) ||
           (phy802_11->mode == PHY_SENSING));

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    PHY_SignalInterference(
        node,
        phyIndex,
        channelIndex,
        NULL,
        NULL,
        &(phy802_11->interferencePower_mW));

    IsIdle = (!Phy802_11CarrierSensing(node, phy802_11));
    phy802_11->interferencePower_mW = oldInterferencePower;

    return IsIdle;
}


BOOL Phy802_11MediumIsIdleInDirection(Node* node, int phyIndex,
                                      double azimuth) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*)thisPhy->phyVar;
    BOOL IsIdle;
    int channelIndex;
    double oldInterferencePower = phy802_11->interferencePower_mW;

    assert((phy802_11->mode == PHY_IDLE) ||
           (phy802_11->mode == PHY_SENSING));

    if (ANTENNA_IsLocked(node, phyIndex)) {
       return (phy802_11->mode == PHY_IDLE);
    }//if//

    // ZEB - Removed to support patterned antenna types
//    assert(ANTENNA_IsInOmnidirectionalMode(node, phyIndex));

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);


    ANTENNA_SetBestConfigurationForAzimuth(node, phyIndex, azimuth);

    PHY_SignalInterference(
        node,
        phyIndex,
        channelIndex,
        NULL,
        NULL,
        &(phy802_11->interferencePower_mW));

    IsIdle = (!Phy802_11CarrierSensing(node, phy802_11));
    phy802_11->interferencePower_mW = oldInterferencePower;
    ANTENNA_SetToDefaultMode(node, phyIndex);

    return IsIdle;
}


void Phy802_11SetSensingDirection(Node* node, int phyIndex,
                                  double azimuth) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*)thisPhy->phyVar;
    int channelIndex;

    assert((phy802_11->mode == PHY_IDLE) ||
           (phy802_11->mode == PHY_SENSING));

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);
    ANTENNA_SetBestConfigurationForAzimuth(node, phyIndex, azimuth);

    PHY_SignalInterference(
        node,
        phyIndex,
        channelIndex,
        NULL,
        NULL,
        &(phy802_11->interferencePower_mW));
}



void Phy802_11Finalize(Node *node, const int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;
    char buf[MAX_STRING_LENGTH];

    if (thisPhy->phyStats == FALSE) {
        return;
    }

    phy802_11->thisPhy->stats->Print(node,
                                     "Physical",
                                     "802.11",
                                     ANY_ADDRESS,
                                     phyIndex);

#ifdef CYBER_LIB
    if (node->phyData[phyIndex]->jammerStatistics == TRUE)
    {
        char durationStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->phyData[phyIndex]->jamDuration, durationStr, node);
        sprintf(buf, "Signals received and forwarded to MAC during the jam duration = %d",
            (int) phy802_11->stats.totalRxSignalsToMacDuringJam);
        IO_PrintStat(node, "Physical", "802.11", ANY_DEST, phyIndex, buf);
        sprintf(buf, "Signals locked on by PHY during the jam duration = %d",
            (int) phy802_11->stats.totalSignalsLockedDuringJam);
        IO_PrintStat(node, "Physical", "802.11", ANY_DEST, phyIndex, buf);

        sprintf(buf, "Signals received but with errors during the jam duration = %d",
            (int) phy802_11->stats.totalSignalsWithErrorsDuringJam);
        IO_PrintStat(node, "Physical", "802.11", ANY_DEST, phyIndex, buf);
        sprintf(buf, "Total jam duration in (s) = %s", durationStr);
        IO_PrintStat(node, "Physical", "802.11", ANY_DEST, phyIndex, buf);
    }
#endif


    Phy802_11ChangeState(node,phyIndex, PHY_IDLE);

}



void Phy802_11SignalArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo)
{
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;
    assert(phy802_11->mode != PHY_TRANSMITTING);

    if (phy802_11->thisPhy->phyStats)
    {
        phy802_11->thisPhy->stats->AddSignalDetectedDataPoints(node);
    }
    if (DEBUG)
    {
        char currTime[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), currTime);
        printf("\ntime %s: SignalArrival from %d at %d\n",
               currTime,
               propRxInfo->txMsg->originatingNodeId,
               node->nodeId);
    }

    Phy802_11PlcpHeader* plcp
        = (Phy802_11PlcpHeader*)MESSAGE_ReturnPacket(propRxInfo->txMsg);
    ERROR_Assert(plcp != NULL,"Plcp header not found");
    if (plcp->txPhyModel != phy802_11->thisPhy->phyModel)
    {
        if (plcp->txPhyModel == PHY802_11n
            || phy802_11->thisPhy->phyModel == PHY802_11n)
        {
            char buf[MAX_STRING_LENGTH];
            sprintf(buf, "\nPHY802_11n: Only GreenField mode is currently "
                "supported. Please set phy model to PHY802_11n at node %d",
                propRxInfo->txNodeId);
            ERROR_ReportError(buf);
        }
    }

    switch (phy802_11->mode) {
        case PHY_RECEIVING:
        {
            PHY_NotificationOfPacketDrop(
                node,
                phyIndex,
                channelIndex,
                propRxInfo->txMsg,
                "PHY Busy in Receiving",
                NON_DB(ANTENNA_GainForThisSignal(node,
                                                 phyIndex,
                                                 propRxInfo)
                       + propRxInfo->rxPower_dBm),
                phy802_11->interferencePower_mW,
                propRxInfo->pathloss_dB);

            double rxPower_mW =
                NON_DB(ANTENNA_GainForThisSignal(node, phyIndex,
                        propRxInfo) + propRxInfo->rxPower_dBm);

            if (!phy802_11->rxMsgError) {
               phy802_11->rxMsgError = Phy802_11CheckRxPacketError(
                   node,
                   phy802_11,
                   NULL);
            }//if//

            phy802_11->rxTimeEvaluated = node->getNodeTime();
            phy802_11->interferencePower_mW += rxPower_mW * propRxInfo->txNumAtnaElmts;

            DEBUG_PRINT("Signal Arrival in receiving Status: interference power: %lf \n",
                phy802_11->interferencePower_mW);
            break;
        }

        //
        // If the phy is idle or sensing,
        // check if it can receive this signal.
        //
        case PHY_IDLE:
        case PHY_SENSING:
        {
            double rxInterferencePower_mW = NON_DB(
                ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
                propRxInfo->rxPower_dBm) * propRxInfo->txNumAtnaElmts;

            double rxPowerInOmni_mW = NON_DB(
                ANTENNA_DefaultGainForThisSignal(node, phyIndex, propRxInfo)
                + propRxInfo->rxPower_dBm);

            if (rxPowerInOmni_mW >= phy802_11->rxSensitivity_mW[0]) {
                PropTxInfo *propTxInfo
                    = (PropTxInfo *)MESSAGE_ReturnInfo(propRxInfo->txMsg);
                clocktype txDuration = propTxInfo->duration;
                double rxPower_mW;

                if (!ANTENNA_IsLocked(node, phyIndex)) {
                   ANTENNA_SetToBestGainConfigurationForThisSignal(
                       node, phyIndex, propRxInfo);

                   PHY_SignalInterference(
                       node,
                       phyIndex,
                       channelIndex,
                       propRxInfo->txMsg,
                       &rxPower_mW,
                       &(phy802_11->interferencePower_mW));
                }
                else {
                    // 802_11 will listen to this signal, taken care of in SignalEnd
                    rxPower_mW = rxInterferencePower_mW/propRxInfo->txNumAtnaElmts;
                }

                Phy802_11LockSignal(
                    node,
                    phy802_11,
                    propRxInfo,
                    propRxInfo->txMsg,
                    rxPower_mW,
                    (propRxInfo->rxStartTime + propRxInfo->duration),
                    channelIndex,
                    propRxInfo->txDOA,
                    propRxInfo->rxDOA,
                    propRxInfo->txNumAtnaElmts,
                    propRxInfo->txAtnaElmtSpace);

#ifdef CYBER_LIB
                if (node->phyData[phyIndex]->jammerStatistics == TRUE)
                {
                    if (node->phyData[phyIndex]->jamInstances > 0)
                    {
                        phy802_11->stats.totalSignalsLockedDuringJam++;
                    }
                }
#endif
                Phy802_11ChangeState(node,phyIndex, PHY_RECEIVING);
                Phy802_11ReportExtendedStatusToMac(
                    node,
                    phyIndex,
                    PHY_RECEIVING,
                    txDuration,
                    propRxInfo->txMsg);

                DEBUG_PRINT("Signal Arrival in IDLE/SENSING, change to Receiving,"
                    "interference power: %lf \n", phy802_11->interferencePower_mW);
            }
            else {
                //
                // Otherwise, check if the signal changes the phy status
                //
                PHY_NotificationOfPacketDrop(
                    node,
                    phyIndex,
                    channelIndex,
                    propRxInfo->txMsg,
                    "Signal below Rx Threshold",
                    NON_DB(ANTENNA_GainForThisSignal(node,
                                                     phyIndex,
                                                     propRxInfo)
                           + propRxInfo->rxPower_dBm),
                    phy802_11->interferencePower_mW,
                    propRxInfo->pathloss_dB);

                PhyStatusType newMode;

                phy802_11->interferencePower_mW += rxInterferencePower_mW;

                if (Phy802_11CarrierSensing(node, phy802_11)) {
                   newMode = PHY_SENSING;
                } else {
                   newMode = PHY_IDLE;
                }//if//

                if (newMode != phy802_11->mode) {
                    Phy802_11ChangeState(node,phyIndex, newMode);
                    Phy802_11ReportStatusToMac(
                        node,
                        phyIndex,
                        newMode);

                }//if//

                DEBUG_PRINT("Signal Arrival in IDLE/SENSING, change to IDLE/SENSING,"
                    "interference power: %lf \n", phy802_11->interferencePower_mW);
            }//if//

            break;
        }

        default:
            abort();

    }//switch (phy802_11->mode)//
}



void Phy802_11TerminateCurrentReceive(
    Node* node, int phyIndex, const BOOL terminateOnlyOnReceiveError,
    BOOL* frameError,
    clocktype* endSignalTime)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*)thisPhy->phyVar;


    *endSignalTime = phy802_11->rxEndTime;

    if (!phy802_11->rxMsgError) {
       phy802_11->rxMsgError = Phy802_11CheckRxPacketError(
           node,
           phy802_11,
           NULL);
    }//if//

    *frameError = phy802_11->rxMsgError;

    if ((terminateOnlyOnReceiveError) && (!phy802_11->rxMsgError)) {
        return;
    }//if//

    if (thisPhy->antennaData->antennaModelType == ANTENNA_OMNIDIRECTIONAL) {
        int channelIndex;
        PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);
        PHY_SignalInterference(
            node,
            phyIndex,
            channelIndex,
            NULL,
            NULL,
            &(phy802_11->interferencePower_mW));
    }
    else {
        int channelIndex;
        PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

        ERROR_Assert(((thisPhy->antennaData->antennaModelType
                    == ANTENNA_SWITCHED_BEAM) ||
               (thisPhy->antennaData->antennaModelType
                    == ANTENNA_STEERABLE) ||
               (thisPhy->antennaData->antennaModelType
                    == ANTENNA_PATTERNED)) ,
                "Illegal antennaModelType");

        if (!ANTENNA_IsLocked(node, phyIndex)) {
            ANTENNA_SetToDefaultMode(node, phyIndex);
        }//if//

        PHY_SignalInterference(
            node,
            phyIndex,
            channelIndex,
            NULL,
            NULL,
            &(phy802_11->interferencePower_mW));
    }//if//

    Int32 chIndex;
    PHY_GetTransmissionChannel(node, phyIndex, &chIndex);
    PHY_NotificationOfPacketDrop(
        node,
        phyIndex,
        chIndex,
        phy802_11->rxMsg,
        "Rx Terminated by MAC",
        phy802_11->rxMsgPower_mW,
        phy802_11->interferencePower_mW,
        0.0);

    if (phy802_11->thisPhy->phyStats)
    {
        phy802_11->thisPhy->stats->AddSignalTerminatedDataPoints(
                                        node,
                                        thisPhy,
                                        phy802_11->rxMsg,
                                        phy802_11->rxChannelIndex,
                                        phy802_11->txNodeId,
                                        phy802_11->pathloss_dB,
                                        phy802_11->rxTimeEvaluated,
                                        phy802_11->interferencePower_mW,
                                        phy802_11->rxMsgPower_mW);
    }

    Phy802_11UnlockSignal(phy802_11);

    if (Phy802_11CarrierSensing(node, phy802_11))
    {
        Phy802_11ChangeState(node, phyIndex, PHY_SENSING);
    }
    else
    {
        Phy802_11ChangeState(node, phyIndex, PHY_IDLE);

    } // if//
}

void Phy802_11TerminateCurrentTransmission(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*)thisPhy->phyVar;
     int channelIndex;
    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    //GuiStart
    if (node->guiOption == TRUE) {
        GUI_EndBroadcast(node->nodeId,
                         GUI_PHY_LAYER,
                         GUI_DEFAULT_DATA_TYPE,
                         thisPhy->macInterfaceIndex,
                         node->getNodeTime());
    }
    //GuiEnd
    assert(phy802_11->mode == PHY_TRANSMITTING);

    if (!ANTENNA_IsLocked(node, phyIndex)) {
        ANTENNA_SetToDefaultMode(node, phyIndex);
    }

    //Cancel the timer end message so that 'Phy802_11TransmissionEnd' is
    // not called.
    if (phy802_11->txEndTimer)
    {
        MESSAGE_CancelSelfMsg(node, phy802_11->txEndTimer);
        phy802_11->txEndTimer = NULL;
    }
}




void Phy802_11SignalEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo)
{
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;
    double sinr = -1.0;

    BOOL receiveErrorOccurred = FALSE;

    PropProfile* propProfile = node->propChannel[channelIndex].profile;

    if (propProfile->enableChannelOverlapCheck) {

        PHY_SignalInterference(
                node,
                phyIndex,
                channelIndex,
                NULL,
                NULL,
                &(phy802_11->interferencePower_mW));
    }

    if (DEBUG)
    {
        char currTime[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), currTime);
        printf("\ntime %s: SignalEnd from %d at %d\n",
               currTime,
               propRxInfo->txMsg->originatingNodeId,
               node->nodeId);
    }
    assert(phy802_11->mode != PHY_TRANSMITTING);

    if (phy802_11->mode == PHY_RECEIVING) {
        if (phy802_11->rxMsgError == FALSE) {
            phy802_11->rxMsgError = Phy802_11CheckRxPacketError(
                node,
                phy802_11,
                &sinr);
            phy802_11->rxTimeEvaluated = node->getNodeTime();
        }//if
    }//if//

    receiveErrorOccurred = phy802_11->rxMsgError;

    //
    // If the phy is still receiving this signal, forward the frame
    // to the MAC layer.
    //

    if ((phy802_11->mode == PHY_RECEIVING)
         && (phy802_11->rxMsg == propRxInfo->txMsg))
    {

#ifdef ADDON_DB
        Phy802_11UpdateEventsTable(node,
                                   phyIndex,
                                   channelIndex,
                                   propRxInfo,
                                   phy802_11->rxMsgPower_mW,
                                   phy802_11->rxMsg,
                                   "PhyReceiveSignal");
#endif
        Message* newMsg = NULL;

        if (!ANTENNA_IsLocked(node, phyIndex)) {
            ANTENNA_SetToDefaultMode(node, phyIndex);

            PHY_SignalInterference(
                node,
                phyIndex,
                channelIndex,
                NULL,
                NULL,
                &(phy802_11->interferencePower_mW));
        }//if//

        //Perform Signal measurement
        PhySignalMeasurement sigMeasure;
        sigMeasure.rxBeginTime = propRxInfo->rxStartTime;

        double noise = phy802_11->thisPhy->noise_mW_hz *
                           phy802_11->channelBandwidth;
        if (phy802_11->thisPhy->phyModel == PHY802_11b &&
            (phy802_11->rxDataRateType == PHY802_11b__6M ||
             phy802_11->rxDataRateType == PHY802_11b_11M))
        {
            noise = noise * 11.0;
        }

        sigMeasure.rss = IN_DB(phy802_11->rxMsgPower_mW);
        sigMeasure.snr = IN_DB(phy802_11->rxMsgPower_mW /noise);
        sigMeasure.cinr = IN_DB(phy802_11->rxMsgPower_mW /
                             (phy802_11->interferencePower_mW + noise));

        double rxMsgPower_mW = phy802_11->rxMsgPower_mW;

        Phy802_11UnlockSignal(phy802_11);

        if (Phy802_11CarrierSensing(node, phy802_11) == TRUE) {

            Phy802_11ChangeState(node,phyIndex, PHY_SENSING);
        }
        else {
            Phy802_11ChangeState(node,phyIndex, PHY_IDLE);
        }

        if (!receiveErrorOccurred)
        {
            newMsg = MESSAGE_Duplicate(node, propRxInfo->txMsg);

            MESSAGE_RemoveHeader(
                node, newMsg, sizeof(Phy802_11PlcpHeader), TRACE_802_11);

            PhySignalMeasurement* signalMeaInfo;
            MESSAGE_InfoAlloc(node,
                              newMsg,
                              sizeof(PhySignalMeasurement));
            signalMeaInfo = (PhySignalMeasurement*)
                            MESSAGE_ReturnInfo(newMsg);
            memcpy(signalMeaInfo,&sigMeasure,sizeof(PhySignalMeasurement));


            MESSAGE_SetInstanceId(newMsg, (short) phyIndex);

            phy802_11->rxDOA = propRxInfo->rxDOA;

#ifdef ADDON_DB
            Phy802_11UpdateEventsTable(node,
                                       phyIndex,
                                       channelIndex,
                                       propRxInfo,
                                       rxMsgPower_mW,
                                       newMsg,
                                       "PhySendToUpper");
#endif
            MAC_ReceivePacketFromPhy(
                node,
                node->phyData[phyIndex]->macInterfaceIndex,
                newMsg);

            // Get the path profile
            PropTxInfo* txInfo = (PropTxInfo*) MESSAGE_ReturnInfo(
                                                     propRxInfo->txMsg);

            // Compute the delay
            clocktype txDelay = propRxInfo->rxStartTime
                                - txInfo->txStartTime;

            if (phy802_11->thisPhy->phyStats)
            {
                phy802_11->thisPhy->stats->AddSignalToMacDataPoints(
                    node,
                    propRxInfo,
                    thisPhy,
                    channelIndex,
                    txDelay,
                    phy802_11->interferencePower_mW,
                    propRxInfo->pathloss_dB,
                    rxMsgPower_mW);
            }

#ifdef CYBER_LIB
            if (node->phyData[phyIndex]->jammerStatistics == TRUE)
            {
                if (node->phyData[phyIndex]->jamInstances > 0)
                {
                    phy802_11->stats.totalRxSignalsToMacDuringJam++;
                }
            }
#endif
        }
        else {
            Phy802_11ReportStatusToMac(
                node,
                phyIndex,
                phy802_11->mode);


            PHY_NotificationOfPacketDrop(
                node,
                phyIndex,
                channelIndex,
                propRxInfo->txMsg,
                "Signal Received with Error",
                rxMsgPower_mW,
                phy802_11->interferencePower_mW,
                propRxInfo->pathloss_dB);


            if (phy802_11->thisPhy->phyStats)
            {
                phy802_11->thisPhy->stats->AddSignalWithErrorsDataPoints(
                    node,
                    propRxInfo,
                    phy802_11->thisPhy,
                    channelIndex,
                    phy802_11->interferencePower_mW,
                    rxMsgPower_mW);
            }

#ifdef CYBER_LIB
            if (node->phyData[phyIndex]->jammerStatistics == TRUE)
            {
                if (node->phyData[phyIndex]->jamInstances > 0)
                {
                    phy802_11->stats.totalSignalsWithErrorsDuringJam++;
                }
            }
#endif
        }//if//
        DEBUG_PRINT("Signal ends in receiving and it's the receiving signal,"
            "interference power: %lf \n", phy802_11->interferencePower_mW);
    }
    else {
        PhyStatusType newMode;

        double rxPower_mW =
            NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
                   propRxInfo->rxPower_dBm);

        phy802_11->interferencePower_mW -= rxPower_mW * propRxInfo->txNumAtnaElmts;

        if (phy802_11->interferencePower_mW < 0.0) {
            phy802_11->interferencePower_mW = 0.0;
        }


        if (phy802_11->mode != PHY_RECEIVING) {
           if (Phy802_11CarrierSensing(node, phy802_11) == TRUE) {
               newMode = PHY_SENSING;
           } else {
               newMode = PHY_IDLE;
           }//if//

           if (newMode != phy802_11->mode) {
                Phy802_11ChangeState(node,phyIndex, newMode);
               Phy802_11ReportStatusToMac(
                   node,
                   phyIndex,
                   newMode);
           }//if//
        }//if//
        DEBUG_PRINT("Signal ends in other status,"
            "interference power: %lf \n", phy802_11->interferencePower_mW);
    }//if//
}



void Phy802_11SetTransmitPower(PhyData *thisPhy, double newTxPower_mW) {
    PhyData802_11 *phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    phy802_11->txPower_dBm = (float)IN_DB(newTxPower_mW);
}


void Phy802_11GetTransmitPower(PhyData *thisPhy, double *txPower_mW) {
    PhyData802_11 *phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    *txPower_mW = NON_DB(phy802_11->txPower_dBm);
}



int Phy802_11GetTxDataRate(PhyData *thisPhy) {
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    return phy802_11->dataRate[phy802_11->txDataRateType];
}


int Phy802_11GetRxDataRate(PhyData *thisPhy) {
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    return phy802_11->dataRate[phy802_11->rxDataRateType];
}


int Phy802_11GetTxDataRateType(PhyData *thisPhy) {
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    return phy802_11->txDataRateType;
}


int Phy802_11GetRxDataRateType(PhyData *thisPhy) {
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    return phy802_11->rxDataRateType;
}


void Phy802_11SetTxDataRateType(PhyData* thisPhy, int dataRateType) {
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    assert(dataRateType >= 0 && dataRateType < phy802_11->numDataRates);
    phy802_11->txDataRateType = dataRateType;

    if (phy802_11->thisPhy->phyModel == PHY802_11a ||
            phy802_11->thisPhy->phyModel == PHY802_11b ) {
    phy802_11->txPower_dBm =
        phy802_11->txDefaultPower_dBm[phy802_11->txDataRateType];
    }
    return;
}


void Phy802_11GetLowestTxDataRateType(PhyData* thisPhy, int* dataRateType) {
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    *dataRateType = phy802_11->lowestDataRateType;

    return;
}


void Phy802_11SetLowestTxDataRateType(PhyData* thisPhy) {
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    phy802_11->txDataRateType = phy802_11->lowestDataRateType;
    if (phy802_11->thisPhy->phyModel == PHY802_11a ||
            phy802_11->thisPhy->phyModel == PHY802_11b ) {
    phy802_11->txPower_dBm =
        phy802_11->txDefaultPower_dBm[phy802_11->txDataRateType];
    }

    return;
}


void Phy802_11GetHighestTxDataRateType(PhyData* thisPhy,
                                       int* dataRateType) {
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    *dataRateType = phy802_11->highestDataRateType;

    return;
}


void Phy802_11SetHighestTxDataRateType(PhyData* thisPhy) {
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    phy802_11->txDataRateType = phy802_11->highestDataRateType;
    phy802_11->txPower_dBm =
        phy802_11->txDefaultPower_dBm[phy802_11->txDataRateType];

    return;
}

void Phy802_11GetHighestTxDataRateTypeForBC(
    PhyData* thisPhy,
    int* dataRateType)
{
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    *dataRateType = phy802_11->txDataRateTypeForBC;

    return;
}


void Phy802_11SetHighestTxDataRateTypeForBC(
    PhyData* thisPhy)
{
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    phy802_11->txDataRateType = phy802_11->txDataRateTypeForBC;

    if (phy802_11->thisPhy->phyModel == PHY802_11a ||
            phy802_11->thisPhy->phyModel == PHY802_11b ) {
    phy802_11->txPower_dBm =
        phy802_11->txDefaultPower_dBm[phy802_11->txDataRateType];
    }
    return;
}


clocktype Phy802_11GetFrameDuration(
    PhyData *thisPhy,
    int dataRateType,
    int size)
{
    switch (thisPhy->phyModel) {
        case PHY802_11a: {
            const PhyData802_11* phy802_11a = (PhyData802_11*)
                                                thisPhy->phyVar;
            const int numOfdmSymbols =
                (int)ceil((size * 8 +
                           PHY802_11a_SERVICE_BITS_SIZE +
                           PHY802_11a_TAIL_BITS_SIZE) /
                          phy802_11a->numDataBitsPerSymbol[dataRateType]);
            return
                PHY802_11a_SYNCHRONIZATION_TIME +
                (numOfdmSymbols * PHY802_11a_OFDM_SYMBOL_DURATION) *
                MICRO_SECOND;
        }

        case PHY802_11b: {
            const PhyData802_11* phy802_11b =
                        (PhyData802_11*) thisPhy->phyVar;
            const int numSymbols =
                (int)ceil(size * 8 / phy802_11b->numDataBitsPerSymbol
                                                       [dataRateType]);
            return PHY802_11b_SYNCHRONIZATION_TIME + numSymbols
                                                    * MICRO_SECOND;
        }

        default:
        {
            ERROR_ReportError("Unknown PHY model!\n");
            break;
        }
    }

    // never reachable
    return 0;
}



double Phy802_11GetLastAngleOfArrival(Node* node, int phyIndex)
        {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11 *)thisPhy->phyVar;
    return phy802_11->rxDOA.azimuth;

}

static
void ReleaseSignalToChannel(
    Node *node,
    Message *packet,
    int phyIndex,
    int channelIndex,
    float txPower_dBm,
    clocktype duration,
    clocktype delayUntilAirborne,
    double directionalAntennaGain_dB)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11 *)thisPhy->phyVar;
    if (thisPhy->phyModel == PHY802_11n) {
        phy802_11->pDot11n->ReleaseSignalToChannel(
           node,
           packet,
           phyIndex,
           channelIndex,
           txPower_dBm,
           duration,
           delayUntilAirborne,
           directionalAntennaGain_dB);
    }
    else
    {
        PROP_ReleaseSignal(
            node,
            packet,
            phyIndex,
            channelIndex,
            (float)(txPower_dBm - directionalAntennaGain_dB),
            duration,
            delayUntilAirborne);
    }
}

static
void StartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne,
    BOOL sendDirectionally,
    double azimuthAngle)
{
    if (DEBUG)
    {
        char clockStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("802_11.cpp: node %d start transmitting at time %s "
               "originated from node %d at time %15" TYPES_64BITFMT "d\n",
               node->nodeId, clockStr,
               packet->originatingNodeId, packet->packetCreationTime);
    }

    clocktype delayUntilAirborne = initDelayUntilAirborne;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11 *)thisPhy->phyVar;
    int channelIndex;
    Message *endMsg;
    int packetsize = MESSAGE_ReturnPacketSize(packet);
    clocktype duration;

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    if (!useMacLayerSpecifiedDelay) {
        delayUntilAirborne = phy802_11->rxTxTurnaroundTime;
#ifdef CYBER_LIB
        // Wormhole nodes do not delay
        if (node->macData[thisPhy->macInterfaceIndex]->isWormhole)
        {
            delayUntilAirborne = 0;
        }
#endif // CYBER_LIB
    }//if//

    assert(phy802_11->mode != PHY_TRANSMITTING);

    if (sendDirectionally) {
        ANTENNA_SetBestConfigurationForAzimuth(node, phyIndex,
                                               azimuthAngle);
    }


    if (phy802_11->mode == PHY_RECEIVING) {
        if (thisPhy->antennaData->antennaModelType ==
        ANTENNA_OMNIDIRECTIONAL) {
            phy802_11->interferencePower_mW += phy802_11->rxMsgPower_mW;
        }
        else {
            PHY_SignalInterference(node,
                                   phyIndex,
                                   channelIndex,
                                   NULL,
                                   NULL,
                                   &(phy802_11->interferencePower_mW));
        }

        PHY_NotificationOfPacketDrop(node,
                                     phyIndex,
                                     channelIndex,
                                     phy802_11->rxMsg,
                                     "PHY Stop Rx for Tx",
                                     phy802_11->rxMsgPower_mW,
                                     phy802_11->interferencePower_mW,
                                     0.0);

        if (phy802_11->thisPhy->phyStats)
        {
            phy802_11->thisPhy->stats->AddSignalTerminatedDataPoints(
                                        node,
                                        thisPhy,
                                        phy802_11->rxMsg,
                                        phy802_11->rxChannelIndex,
                                        phy802_11->txNodeId,
                                        phy802_11->pathloss_dB,
                                        phy802_11->rxTimeEvaluated,
                                        phy802_11->interferencePower_mW,
                                        phy802_11->rxMsgPower_mW);
        }
        Phy802_11UnlockSignal(phy802_11);
    }
    Phy802_11ChangeState(node, phyIndex, PHY_TRANSMITTING);

    if (thisPhy->phyModel == PHY802_11n)
    {
        duration =
            Phy802_11nGetFrameDuration(
                thisPhy,
                ((PhyData802_11*)thisPhy->phyVar)->pDot11n->GetTxVector());
    }
    else
    {
        duration = Phy802_11GetFrameDuration(thisPhy,
                                             phy802_11->txDataRateType,
                                             packetsize);
    }

    Phy802_11AddPlcpHeader(node, phy802_11, packet);

    if (PHY_IsListeningToChannel(node, phyIndex, channelIndex))
    {
        PHY_StopListeningToChannel(node, phyIndex, channelIndex);
    }

    if (thisPhy->antennaData->antennaModelType == ANTENNA_PATTERNED)
    {
        if (!sendDirectionally) {
            ReleaseSignalToChannel(
                node,
                packet,
                phyIndex,
                channelIndex,
                phy802_11->txPower_dBm,
                duration,
                delayUntilAirborne,
                0.0);
        }
        else {
            ReleaseSignalToChannel(
                node,
                packet,
                phyIndex,
                channelIndex,
                phy802_11->txPower_dBm,
                duration,
                delayUntilAirborne,
                phy802_11->directionalAntennaGain_dB);
        }
    }
    else {
        if (ANTENNA_IsInOmnidirectionalMode(node, phyIndex)) {
            ReleaseSignalToChannel(
                node,
                packet,
                phyIndex,
                channelIndex,
                phy802_11->txPower_dBm,
                duration,
                delayUntilAirborne,
                0.0);
        } else {
            ReleaseSignalToChannel(
                node,
                packet,
                phyIndex,
                channelIndex,
                phy802_11->txPower_dBm,
                duration,
                delayUntilAirborne,
                phy802_11->directionalAntennaGain_dB);
        }//if//
    }

    //GuiStart
     if (node->guiOption == TRUE) {
        GUI_Broadcast(node->nodeId,
                      GUI_PHY_LAYER,
                      GUI_DEFAULT_DATA_TYPE,
                      thisPhy->macInterfaceIndex,
                      node->getNodeTime());
    }
    //GuiEnd

    endMsg = MESSAGE_Alloc(node,
                            PHY_LAYER,
                            0,
                            MSG_PHY_TransmissionEnd);

    MESSAGE_SetInstanceId(endMsg, (short) phyIndex);
    MESSAGE_Send(node, endMsg, delayUntilAirborne + duration + 1);

    phy802_11->txEndTimer = endMsg;
    /* Keep track of phy statistics and battery computations */

    phy802_11->stats.energyConsumed
        += duration * (BATTERY_TX_POWER_COEFFICIENT
                       * NON_DB(phy802_11->txPower_dBm)
                       + BATTERY_TX_POWER_OFFSET
                       - BATTERY_RX_POWER);
    if (phy802_11->thisPhy->phyStats)
    {
        phy802_11->thisPhy->stats->AddSignalTransmittedDataPoints(node,
                                                                  duration);
    }
}



//
// Used by the MAC layer to start transmitting a packet.
//
void Phy802_11StartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne)
{
    StartTransmittingSignal(
        node, phyIndex,
        packet,
        useMacLayerSpecifiedDelay,
        initDelayUntilAirborne,
        FALSE, 0.0);
}


void Phy802_11StartTransmittingSignalDirectionally(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne,
    double azimuthAngle)
{
    StartTransmittingSignal(
        node, phyIndex,
        packet,
        useMacLayerSpecifiedDelay,
        initDelayUntilAirborne,
        TRUE, azimuthAngle);
}



void Phy802_11LockAntennaDirection(Node* node, int phyNum) {
    ANTENNA_LockAntennaDirection(node, phyNum);
}



void Phy802_11UnlockAntennaDirection(Node* node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11 *)thisPhy->phyVar;

    ANTENNA_UnlockAntennaDirection(node, phyIndex);

    if ((phy802_11->mode != PHY_RECEIVING) &&
        (phy802_11->mode != PHY_TRANSMITTING))
    {
        int channelIndex;

        PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

        ANTENNA_SetToDefaultMode(node, phyIndex);

        PHY_SignalInterference(
            node,
            phyIndex,
            channelIndex,
            NULL,
            NULL,
            &(phy802_11->interferencePower_mW));
    }//if//
}


// for phy connectivity table used in stats db and connection manager
BOOL Phy802_11ProcessSignal(Node* node,
                            int phyIndex,
                            PropRxInfo* propRxInfo,
                            double rxPower_dBm)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11 *)thisPhy->phyVar;

    double rxPowerInOmni_mW = NON_DB(
        ANTENNA_DefaultGainForThisSignal(node, phyIndex, propRxInfo)
        + rxPower_dBm);

    if (rxPowerInOmni_mW >= phy802_11->rxSensitivity_mW[0])
    {
        return TRUE;
    }

    return FALSE;
}

double Phy802_11GetTxPower(Node* node, int phyIndex) {

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11 *)thisPhy->phyVar;

    if (ANTENNA_IsInOmnidirectionalMode(node, phyIndex)) {
        return phy802_11->txPower_dBm;
    } else {
        return (double)(phy802_11->txPower_dBm -
             phy802_11->directionalAntennaGain_dB);
    }//if//
}

#ifdef ADDON_DB
//--------------------------------------------------------------------------
//  FUNCTION: Phy802_11UpdateEventsTable
//  PURPOSE : Updates Stats-DB phy events table
//  PARAMETERS: Node* node
//                  Pointer to node
//              Int32 phyIndex
//                  Phy Index
//              Int32 interfaceIndex
//                  Interface Index
//              PropRxInfo* propRxInfo
//                  Pointer to propRxInfo
//              double rxMsgPower_mW
//                  Received signal power
//              Message* msgToMac
//                  Pointer to the Message
//              std::string eventStr
//                  Receives eventType
//  RETURN TYPE: NONE
//--------------------------------------------------------------------------
void Phy802_11UpdateEventsTable(Node* node,
                                Int32 phyIndex,
                                Int32 channelIndex,
                                PropRxInfo* propRxInfo,
                                double rxMsgPower_mW,
                                Message* msgToMac,
                                std::string eventStr)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*)thisPhy->phyVar;
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
        Int32 controlSize = 0;
        StatsDBMappingParam* mapParamInfo = (StatsDBMappingParam*)
            MESSAGE_ReturnInfo(propRxInfo->txMsg,INFO_TYPE_StatsDbMapping);
        ERROR_Assert(mapParamInfo,
            "Error in StatsDB handling ReceiveSignal!");
        Int32 msgSize = 0;

        if (!msgToMac->isPacked)
        {
            msgSize = MESSAGE_ReturnPacketSize(msgToMac);
        }
        else
        {
            msgSize = MESSAGE_ReturnActualPacketSize(msgToMac);
        }

        StatsDBPhyEventParam phyParam(node->nodeId,
                                      (std::string)mapParamInfo->msgId,
                                      phyIndex,
                                      msgSize,
                                      eventStr);

        phyParam.SetChannelIndex(channelIndex);

        if (!eventStr.compare("PhyReceiveSignal"))
        {
        switch(node->phyData[phyIndex]->phyModel)
        {
            case PHY802_11a:
            {
                controlSize = PHY802_11a_CONTROL_OVERHEAD_SIZE;
                break;
            }
            case PHY802_11b:
            {
                controlSize = PHY802_11b_CONTROL_OVERHEAD_SIZE;
                break;
            }
            default:
            {
                return;
            }
        }

            phyParam.SetPathLoss(propRxInfo->pathloss_dB);

            if (rxMsgPower_mW)
            {
                phyParam.SetSignalPower(IN_DB(rxMsgPower_mW));
            }
            else
            {
                phyParam.SetSignalPower(0);
            }

            if (phy802_11->interferencePower_mW)
            {
                phyParam.SetInterference(
                               IN_DB(phy802_11->interferencePower_mW));
            }
            else
            {
                phyParam.SetInterference(0.0);
            }
        }

        phyParam.SetControlSize(controlSize);
        STATSDB_HandlePhyEventsTableInsert(node, phyParam);
    }
}
#endif


void Phy802_11InterferenceArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo,
    double sigPower)
{
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;

    if (DEBUG)
    {
        char currTime[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), currTime);
        printf("\ntime %s: InterferenceArrival from %d at %d\n",
               currTime,
               propRxInfo->txMsg->originatingNodeId,
               node->nodeId);
    }

    double rxInterferencePower_mW =
        NON_DB(ANTENNA_GainForThisSignal(node, phyIndex,
                propRxInfo) + IN_DB(sigPower));

    switch (phy802_11->mode) {
        case PHY_RECEIVING: {

            if (!phy802_11->rxMsgError) {
               phy802_11->rxMsgError = Phy802_11CheckRxPacketError(
                   node,
                   phy802_11,
                   NULL);
            }//if//

            phy802_11->interferencePower_mW +=
                        rxInterferencePower_mW  * propRxInfo->txNumAtnaElmts;
            phy802_11->rxTimeEvaluated = node->getNodeTime();

            break;
        }

        case PHY_IDLE:
        case PHY_SENSING:
        {
            //
            // check if the interference changes the phy status
            //

            PhyStatusType newMode;

            phy802_11->interferencePower_mW += rxInterferencePower_mW;

            if (Phy802_11CarrierSensing(node, phy802_11)) {
               newMode = PHY_SENSING;
            } else {
               newMode = PHY_IDLE;
            }//if//

            if (newMode != phy802_11->mode) {
                Phy802_11ChangeState(node,phyIndex, newMode);
                Phy802_11ReportStatusToMac(
                    node,
                    phyIndex,
                    newMode);

            }//if//

            break;
        }

        default:
            abort();

    }//switch (phy802_11->mode)//
}


void Phy802_11InterferenceEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo,
    double sigPower)
{
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyData802_11* phy802_11 = (PhyData802_11*) thisPhy->phyVar;
    double sinr = -1.0;

    if (DEBUG)
    {
        char currTime[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), currTime);
        printf("\ntime %s: InterferencelEnd from %d at %d\n",
               currTime,
               propRxInfo->txMsg->originatingNodeId,
               node->nodeId);
    }

    if (phy802_11->mode == PHY_RECEIVING) {
        if (phy802_11->rxMsgError == FALSE) {

            phy802_11->rxMsgError = Phy802_11CheckRxPacketError(
                node,
                phy802_11,
                &sinr);

            phy802_11->rxTimeEvaluated = node->getNodeTime();
        }//if
    }//if//

    PhyStatusType newMode;

    double interferencePower_mW =
        NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
               IN_DB(sigPower));

    phy802_11->interferencePower_mW -=
                interferencePower_mW * propRxInfo->txNumAtnaElmts;

    if (phy802_11->interferencePower_mW < 0.0) {
        phy802_11->interferencePower_mW = 0.0;
    }

    if ((phy802_11->mode != PHY_RECEIVING) &&
        (phy802_11->mode != PHY_TRANSMITTING)){
        if (Phy802_11CarrierSensing(node, phy802_11) == TRUE) {
            newMode = PHY_SENSING;
        } else {
            newMode = PHY_IDLE;
        }//if//

        if (newMode != phy802_11->mode) {

            Phy802_11ChangeState(node,phyIndex, newMode);

            Phy802_11ReportStatusToMac(
               node,
               phyIndex,
               newMode);
        }//if//
    }//if//
}

/*
 * 802.11n APIs
 */
clocktype Phy802_11nGetFrameDuration(
    PhyData* thisPhy,
    const MAC_PHY_TxRxVector& txVector)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    return pDot11n->GetFrameDuration(txVector);
}

void Phy802_11nSetTxVector(PhyData* thisPhy,
                           const MAC_PHY_TxRxVector& txVector)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    return pDot11n->SetTxVector(txVector);
}

int Phy802_11nGetNumAtnaElems(PhyData* thisPhy)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    return pDot11n->GetNumAtnaElems();
}

double Phy802_11nGetAtnaElemSpace(PhyData* thisPhy)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    return pDot11n->GetAtnaElemSpace();
}

BOOL Phy802_11nShortGiCapable(PhyData* thisPhy)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    return pDot11n->ShortGiCapable();
}

BOOL Phy802_11nStbcCapable(PhyData* thisPhy)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    return pDot11n->StbcCapable();
}

ChBandwidth Phy802_11nGetOperationChBwdth(PhyData* thisPhy)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    return pDot11n->GetOperationChBwdth();
}

void Phy802_11nSetOperationChBwdth(PhyData* thisPhy,
                                   ChBandwidth chBwdth)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    pDot11n->SetOperationChBwdth(chBwdth);
}

void Phy802_11nGetTxVector(PhyData* thisPhy, MAC_PHY_TxRxVector& txVector)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    txVector = pDot11n->GetTxVector();
}

BOOL Phy802_11nIsGiEnabled(PhyData* thisPhy)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    return pDot11n->IsGiEnabled();
}

void Phy802_11nEnableGi(PhyData* thisPhy, BOOL enable)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    pDot11n->EnableGi(enable);
}

void Phy802_11nGetTxVectorForBC(PhyData* thisPhy,
                                MAC_PHY_TxRxVector& txVector)
{
        txVector.chBwdth = Phy802_11nGetOperationChBwdth(thisPhy);
        txVector.containAMPDU = FALSE;
        txVector.format = (Mode)MODE_HT_GF;
        txVector.gi = (GI)GI_LONG;
        txVector.length = 0;
        txVector.mcs = 0;
        txVector.numEss = 0;
        txVector.sounding = FALSE;
        txVector.stbc = 0;
}

void Phy802_11nGetTxVectorForHighestDataRate(PhyData* thisPhy,
                            MAC_PHY_TxRxVector& txVector)
{
       Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
       int numAntennaElements = pDot11n->GetNumAtnaElems();
        ERROR_Assert(numAntennaElements <=4, "");

        // currently only 20Mhz & greenfiled mode is supported
        txVector.chBwdth = Phy802_11nGetOperationChBwdth(thisPhy);
        txVector.containAMPDU = FALSE;
        txVector.format = (Mode)MODE_HT_GF;
        txVector.gi = (GI)GI_LONG;
        txVector.length = 0;
        txVector.mcs =(numAntennaElements * 8) - 1;
        txVector.numEss = 0;
        txVector.sounding = FALSE;
        txVector.stbc = 0;
}

void Phy802_11nGetTxVectorForLowestDataRate(PhyData* thisPhy,
                            MAC_PHY_TxRxVector& txVector)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;

    // currently only 20Mhz & greenfiled mode is supported
    txVector.chBwdth = Phy802_11nGetOperationChBwdth(thisPhy);
    txVector.containAMPDU = FALSE;
    txVector.format = (Mode)MODE_HT_GF;
    txVector.gi = (GI)GI_LONG;
    txVector.length = 0;
    txVector.mcs =0;
    txVector.numEss = 0;
    txVector.sounding = FALSE;
    txVector.stbc = 0;
}

void Phy802_11nGetRxVector(PhyData* thisPhy,
                           MAC_PHY_TxRxVector& rxVector)
{
    Phy802_11n* pDot11n = ((PhyData802_11*)thisPhy->phyVar)->pDot11n;
    pDot11n->GetPreviousRxVector(rxVector);
}

