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
#include "antenna.h"
#include "phy_abstract.h"
#ifdef MILITARY_RADIOS_LIB
#include "mac_link16.h"
#endif // MILITARY_RADIOS_LIB

#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#ifdef ADDON_BOEINGFCS
#include "network_ces_inc_sincgars.h"
#include "link_ces_wnw_adaptation.h"
#include "mac_wnw_main.h"
#include "network_ces_inc_eplrs.h"
#endif // ADDON_BOEINGFCS

#ifdef ALE_ASAPS_LIB
#include "mac_ale.h"
#endif

#define DEBUG 0
#define DYNAMIC_STATS 0
#ifdef ADDON_BOEINGFCS
#define DEBUG_PCOM 0
#endif

#include "dynamic.h"
#include "partition.h"

#ifdef JNE_LIB
#include "mib_wnw_sis_datastore.h"
#endif


// FUNCTION   phy_abstract_stats_str
// PURPOSE    Constructor for stucture PhyAbstractStats
// PARAMETERS void
PhyAbstractStats::phy_abstract_stats_str(void)
{
    turnOnTime = 0;
#ifdef CYBER_LIB
    totalRxSignalsToMacDuringJam = 0;
    totalSignalsLockedDuringJam = 0;
    totalSignalsWithErrorsDuringJam = 0;
#endif
    // dynamic stats IDs
    totalTxSignalsId = 0;
    totalSignalsDetectedId = 0;
    totalSignalsLockedId = 0;
    totalSignalsWithErrorsId = 0;
    totalRxSignalsToMacId = 0;
    currentSINRId = 0; // for debugging.
    energyConsumedId = 0;
    timer = 0.0;

#ifdef ADDON_BOEINGFCS
    totalSignalDuration = 0.0;
#endif

#ifdef ADDON_DB
    txSendTime = 0;
    numberRxSignals = 0;
#endif

}
// FUNCTION   struct_phy_abstract_str
// PURPOSE    Constructor for stucture PhyDataAbstract
// PARAMETERS void
PhyDataAbstract::struct_phy_abstract_str(void)
{
    nodeLinkLostStats = FALSE;
    nodeLinkDelayStats = FALSE;
    thisPhy = NULL;
    txPower_dBm = 0.0;
    rxSensitivity_mW = 0.0;
    rxThreshold_mW = 0.0;
#ifdef ADDON_BOEINGFCS
    //added to support multiple rates
    numDataRates = 0;
#endif
    syncTime = 0;
    rxMsg = NULL;
    rxMsgPower_mW = 0.0;
    rxTimeEvaluated = 0;
    rxMsgError = FALSE;
    rxEndTime = 0;
    rxDOA.azimuth = 0;
    rxDOA.elevation = 0;
    txEndTimer = NULL;
    noisePower_mW = 0.0;
    interferencePower_mW = 0.0;

    mode = PHY_IDLE;
    previousMode = PHY_IDLE;
    radioBusyPercentStats = FALSE;
    batteryPowerStats = FALSE;
    interferenceStats = FALSE;
#ifdef ADDON_BOEINGFCS
    linkAdaptationEnabled = FALSE;
#endif

    directionalAntennaGain_dB = 0.0;
}

void PhyAbstractChangeState(
        Node* node,
        int phyIndex,
        PhyStatusType newStatus)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract *phy_abstract = (PhyDataAbstract *)thisPhy->phyVar;

    phy_abstract->previousMode = phy_abstract->mode;
    phy_abstract->mode = newStatus;

    Phy_ReportStatusToEnergyModel(
        node,
        phyIndex,
        phy_abstract->previousMode,
        newStatus);
}

static
void PhyAbstractReportExtendedStatusToMac(
    Node *node,
    int phyNum,
    PhyStatusType status,
    clocktype receiveDuration,
    const Message* potentialIncomingPacket)
{
    PhyData* thisPhy = node->phyData[phyNum];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;

    assert(status == phy_abstract->mode);

    MAC_ReceivePhyStatusChangeNotification(
        node, thisPhy->macInterfaceIndex,
        phy_abstract->previousMode, status,
        receiveDuration, potentialIncomingPacket, phyNum);
}



static /*inline*/
void PhyAbstractReportStatusToMac(
    Node *node, int phyNum, PhyStatusType status)
{
    PhyAbstractReportExtendedStatusToMac(
        node, phyNum, status, 0, NULL);
}



static //inline//
void PhyAbstractLockSignal(
    Node* node,
    PhyDataAbstract* phy_abstract,
    PropRxInfo* propRxInfo,
    Message *msg,
    double rxPower_mW,
    clocktype rxEndTime,
    int channelIndex)
{
    phy_abstract->rxMsg = msg;
    phy_abstract->rxMsgError = FALSE;
    phy_abstract->rxMsgPower_mW = rxPower_mW;
    phy_abstract->rxTimeEvaluated = node->getNodeTime();
    phy_abstract->rxEndTime = rxEndTime;
    phy_abstract->txNodeId = propRxInfo->txNodeId;
    phy_abstract->rxChannelIndex = propRxInfo->channelIndex;
    phy_abstract->pathloss_dB = propRxInfo->pathloss_dB;

    if (phy_abstract->thisPhy->phyStats)
    {
        phy_abstract->thisPhy->stats->AddSignalLockedDataPoints(
            node,
            propRxInfo,
            phy_abstract->thisPhy,
            channelIndex,
            rxPower_mW);
    }
}

static //inline//
void PhyAbstractUnlockSignal(
         PhyDataAbstract* phy_abstract)
{
    phy_abstract->rxMsg = NULL;
    phy_abstract->rxMsgError = FALSE;
    phy_abstract->rxMsgPower_mW = 0.0;
    phy_abstract->rxTimeEvaluated = 0;
    phy_abstract->rxEndTime = 0;
    phy_abstract->txNodeId = 0;
    phy_abstract->rxChannelIndex = -1;
    phy_abstract->pathloss_dB = 0.0;
}



static /*inline*/
BOOL PhyAbstractCarrierSensing(Node* node,
                               PhyDataAbstract* phy_abstract)
{
    double rxSensitivity_mW = phy_abstract->rxSensitivity_mW;

    if (!ANTENNA_IsInOmnidirectionalMode(node, phy_abstract->
         thisPhy->phyIndex))
    {
        rxSensitivity_mW =
                NON_DB(IN_DB(rxSensitivity_mW) +
                phy_abstract->directionalAntennaGain_dB);
    }

    if ((phy_abstract->interferencePower_mW + phy_abstract->noisePower_mW) >
        rxSensitivity_mW)
    {
        return TRUE;
    }

    return FALSE;
}



static //inline//
BOOL PhyAbstractCheckRxPacketError(Node* node,
                                   PhyDataAbstract* phy_abstract,
                                   BOOL isHeader,
                                   double rxPower_mW,
                                   double *sinrp)
{
    double sinr;
    double BER;
    BOOL packeterror = FALSE;  // added for BER_BASED support

    assert(phy_abstract->rxMsgError == FALSE);

    if (isHeader)
    {
        sinr = rxPower_mW /
            (phy_abstract->interferencePower_mW +
             phy_abstract->thisPhy->noise_mW_hz *
             phy_abstract->bandwidth);
    }
    else
    {
        sinr = phy_abstract->rxMsgPower_mW /
            (phy_abstract->interferencePower_mW +
            phy_abstract->thisPhy->noise_mW_hz *
            phy_abstract->bandwidth);
    }

    if (sinrp)
    {
        *sinrp = sinr;
    }

    if (DEBUG) {
        printf("DEBUG: Node %d calculated SINR of %f for current signal\n",
               node->nodeId, IN_DB(sinr));
        fflush(stdout);
    }
    if (DYNAMIC_STATS) {
        GUI_SendRealData(node->nodeId,
                         phy_abstract->stats.currentSINRId,
                         IN_DB(sinr),
                         node->getNodeTime());
    }

    if (phy_abstract->thisPhy->phyRxModel == SNR_THRESHOLD_BASED) {

        if (sinr < phy_abstract->thisPhy->phyRxSnrThreshold) {

            packeterror = TRUE;
            double interferenceThreshold = 0;

            interferenceThreshold =
                phy_abstract->rxMsgPower_mW/
                (phy_abstract->thisPhy->phyRxSnrThreshold
                 - (phy_abstract->thisPhy->noise_mW_hz
                    * phy_abstract->bandwidth));

            if (phy_abstract->interferencePower_mW > interferenceThreshold && phy_abstract->thisPhy->phyStats)
            {
                phy_abstract->thisPhy->stats->AddSignalWithInterferenceErrorsDataPoints(node);
            }
        } else {
            packeterror = FALSE;
        }

    }
    else if (phy_abstract->thisPhy->phyRxModel == BER_BASED) {

        BER = PHY_BER(phy_abstract->thisPhy,
                      0,
                      sinr);

        if (BER != 0.0) {
            double numBits;

            if (isHeader)
            {
               numBits =  (double)phy_abstract->syncTime *
                          (double)phy_abstract->dataRate / (double)SECOND;
            }
            else
            {
                numBits =
                        ((double)(node->getNodeTime() - phy_abstract->rxTimeEvaluated)
                        * (double)phy_abstract->dataRate /(double)SECOND);

            }

            double errorProbability = 1.0 - pow((1.0 - BER), numBits);
            double rand = RANDOM_erand(phy_abstract->thisPhy->seed);

            assert((errorProbability >= 0.0) && (errorProbability <= 1.0));

            if (errorProbability > rand) {
                packeterror = TRUE;
            }else{
                packeterror = FALSE;
            } //if errorProbability
        } // if BER
    }  // if BER_BASED

    return packeterror;
}

#ifdef ADDON_BOEINGFCS
static //inline//
BOOL PhyAbstractPcomRxPacketError(Node* node,
                                  PhyData* thisPhy,
                                  NodeId txNodeId)
{
    BOOL packeterror = FALSE;

    double pcomValue;
    double rand = RANDOM_erand(thisPhy->seed);
    ERROR_Assert(thisPhy->pcomTable[txNodeId] != NULL, "Pcom data structure is null");
    pcomValue = thisPhy->pcomTable[txNodeId]->pcom;

    if (rand > pcomValue)
    {
        if (DEBUG_PCOM)
        {
            printf("  Node %d from %d ~ pcom %f | rand %f : ",
                   node->nodeId, txNodeId, pcomValue, rand);
            printf("comm fails\n");
        }
        packeterror = TRUE;
    }
    else
    {
        if (DEBUG_PCOM)
        {
            printf("  Node %d from %d ~ pcom %f | rand %f : ",
                   node->nodeId, txNodeId, pcomValue, rand);
            printf("comm success!\n");
        }
    }

    return packeterror;
}
#endif


void PhyAbstractSetDataRate(
    Node* node,
    int phyIndex,
    Int64 dataRate)
{

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;

    phy_abstract->dataRate = dataRate;


    if (DEBUG)
    {
        printf("Set Data Rate as %d\n", (Int32) phy_abstract->dataRate);
        fflush(stdout);
    }

    return;
}

void PhyAbstractInit(
    Node *node,
    const int phyIndex,
    const NodeInput *nodeInput)
{
    double rxSensitivity_dBm;
    double rxThreshold_dBm;
    double txPower_dBm;
    Int64  dataRate;
    BOOL   wasFound;
    int i;
    int numChannels = PROP_NumberChannels(node);

    PhyDataAbstract *phy_abstract = new PhyDataAbstract;

    node->phyData[phyIndex]->phyVar = (void *)phy_abstract;

    phy_abstract->thisPhy = node->phyData[phyIndex];

    //
    // Antenna model initialization
    //
    ANTENNA_Init(node, phyIndex, nodeInput);

    ERROR_Assert(((phy_abstract->thisPhy->antennaData->antennaModelType
                    == ANTENNA_OMNIDIRECTIONAL) ||
                (phy_abstract->thisPhy->antennaData->antennaModelType
                    == ANTENNA_PATTERNED) ||
                (phy_abstract->thisPhy->antennaData->antennaModelType
                    == ANTENNA_STEERABLE) ||
                (phy_abstract->thisPhy->antennaData->antennaModelType
                    == ANTENNA_SWITCHED_BEAM)) ,
                "Illegal antennaModelType.\n");

    //
    // Set phy_abstract-DATA-RATE
    //

    IO_ReadInt64(
        node,
        node->nodeId,
        phy_abstract->thisPhy->macInterfaceIndex,
        nodeInput,
        "PHY-ABSTRACT-DATA-RATE",
        &wasFound,
        &dataRate);

    if (wasFound) {
        phy_abstract->dataRate = dataRate;
    }//if//
    else {
        phy_abstract->dataRate = PHY_ABSTRACT_DEFAULT_DATA_RATE;
    }

    //
    // Set PHY-ABSTRACT-BANDWIDTH
    //
    int    bandwidth;

    IO_ReadInt(
        node,
        node->nodeId,
        phy_abstract->thisPhy->macInterfaceIndex,
        nodeInput,
        "PHY-ABSTRACT-BANDWIDTH",
        &wasFound,
        &bandwidth);

    if (wasFound) {
        phy_abstract->bandwidth = bandwidth;
    }//if//
    else {
        phy_abstract->bandwidth = PHY_ABSTRACT_DEFAULT_CHANNEL_BANDWIDTH;
    }

    ERROR_Assert(phy_abstract->bandwidth > 0.0,
                "The value for parameter PHY-ABSTRACT-BANDWIDTH"
                " should be greater than 0.0\n");

    // Dynamic Hierarchy
    D_Hierarchy *hierarchy = &node->partitionData->dynamicHierarchy;
    BOOL createDataRatePath = FALSE;
    BOOL createBandwidthPath = FALSE;

    // Add path and object when dynamic api enabled
    if (hierarchy->IsEnabled()) {
        std::string dataRatePath;
        std::string bandwidthPath;

        // Create a node interface path
        createDataRatePath =
            hierarchy->CreatePhyInterfacePath(
                node,
                phyIndex,
                "phyAbstract",
                "PHY-ABSTRACT-DATA-RATE",
                dataRatePath,
                TRUE);
        // add object to path
        if (createDataRatePath) {
            hierarchy->AddObject(
                dataRatePath,
                new D_Int64Obj(&phy_abstract->dataRate));
        }

        // Create a node interface path
        createBandwidthPath =
            hierarchy->CreatePhyInterfacePath(node,
                                               phyIndex,
                                               "phyAbstract",
                                               "PHY-ABSTRACT-BANDWIDTH",
                                               bandwidthPath,
                                               TRUE);
        // add object to path
        if (createBandwidthPath) {
            hierarchy->AddObject(
                bandwidthPath,
                new D_Int32Obj(&phy_abstract->bandwidth));


            // AddLink for ifSpeed.
            char mibsPath [MAX_STRING_LENGTH];
            sprintf(mibsPath,
                "/node/%d/snmp/1.3.6.1.2.1.2.2.1.5.%d",
                node->nodeId,
                phyIndex);
            hierarchy->AddLink(mibsPath , bandwidthPath);

            // AddLink for ifHighSpeed
            sprintf(mibsPath,
                "/node/%d/snmp/1.3.6.1.2.1.31.1.1.1.15.%d",
                node->nodeId,
                phyIndex);
            hierarchy->AddLink(mibsPath, bandwidthPath);
        }
    }

    //
    // Set phy_abstract-TX-POWER
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        phy_abstract->thisPhy->macInterfaceIndex,
        nodeInput,
        "PHY-ABSTRACT-TX-POWER",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phy_abstract->txPower_dBm = (float)txPower_dBm;
    }
    else {
        phy_abstract->txPower_dBm = PHY_ABSTRACT_DEFAULT_TX_POWER_dBm;
    }


    //
    // Set phy_abstract-RX-SENSITIVITY
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        phy_abstract->thisPhy->macInterfaceIndex,
        nodeInput,
        "PHY-ABSTRACT-RX-SENSITIVITY",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phy_abstract->rxSensitivity_mW = NON_DB(rxSensitivity_dBm);
    }
    else {
        phy_abstract->rxSensitivity_mW =
            NON_DB(PHY_ABSTRACT_DEFAULT_RX_SENSITIVITY_dBm);
    }


    //
    // Set phy_abstract-RX-THRESHOLD
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        phy_abstract->thisPhy->macInterfaceIndex,
        nodeInput,
        "PHY-ABSTRACT-RX-THRESHOLD",
        &wasFound,
        &rxThreshold_dBm);

    if (wasFound) {
        phy_abstract->rxThreshold_mW = NON_DB(rxThreshold_dBm);
    }
    else {
        phy_abstract->rxThreshold_mW = NON_DB(
        PHY_ABSTRACT_DEFAULT_RX_THRESHOLD_dBm);
    }

    // set the synchronization time to acount overhead of header
    //PHY_ABSTRACT_SYNCHRONIZATION_TIME
    phy_abstract->syncTime =
        (clocktype) (
            (PHY_ABSTRACT_DEFAULT_HEADER_SIZE * SECOND) /
            phy_abstract->dataRate);



;
    char lbuf[MAX_STRING_LENGTH];

        // Node Link Loss Stats
        IO_ReadString(
            node,
            node->nodeId,
            phy_abstract->thisPhy->macInterfaceIndex,
            nodeInput,
            "LINK-LOSS-STATISTICS",
            &wasFound,
            lbuf);

        if (wasFound) {
            if (strcmp(lbuf, "YES") == 0 )
            {
                phy_abstract->nodeLinkLostStats = TRUE;
            }
            else
            {
                phy_abstract->nodeLinkLostStats = FALSE;
            }
        } else {
            phy_abstract->nodeLinkLostStats = FALSE;
        }

        IO_ReadString(
            node,
            node->nodeId,
            phy_abstract->thisPhy->macInterfaceIndex,
            nodeInput,
            "LINK-DELAY-STATISTICS",
            &wasFound,
            lbuf);

        if (wasFound) {
            if (strcmp(lbuf, "YES") == 0)
            {
                phy_abstract->nodeLinkDelayStats = TRUE;
            }
            else
            {
                phy_abstract->nodeLinkDelayStats = FALSE;
            }
        } else {
            phy_abstract->nodeLinkDelayStats = FALSE;
        }

    //
    // Set ESTIMATED-DIRECTIONAL-ANTENNA-GAIN
    //
    IO_ReadDouble(
            node,
            node->nodeId,
            node->phyData[phyIndex]->macInterfaceIndex,
            nodeInput,
            "ESTIMATED-DIRECTIONAL-ANTENNA-GAIN",
            &wasFound,
            &(phy_abstract->directionalAntennaGain_dB));

    if (!wasFound &&
         (phy_abstract->thisPhy->antennaData->antennaModelType
             != ANTENNA_OMNIDIRECTIONAL &&
         phy_abstract->thisPhy->antennaData->antennaModelType
             != ANTENNA_PATTERNED))
    {
        phy_abstract->directionalAntennaGain_dB =
                PHY_ABSTRACT_DEFAULT_DIRECTIONAL_ANTENNA_GAIN;
    }

    //
    // Initialize phy statistics variables
    //
    phy_abstract->stats.energyConsumed         = 0.0;
    phy_abstract->stats.turnOnTime             = node->getNodeTime();
    phy_abstract->stats.numUpdates = 0;
#ifdef CYBER_LIB
    phy_abstract->stats.totalRxSignalsToMacDuringJam = 0;
    phy_abstract->stats.totalSignalsLockedDuringJam = 0;
    phy_abstract->stats.totalSignalsWithErrorsDuringJam = 0;
#endif

#ifdef ADDON_BOEINGFCS
    phy_abstract->stats.totalSignalDuration = 0;
#endif
#ifdef ADDON_DB
    StatsDb* db = node->partitionData->statsDb;
    // For phy aggregate table
    phy_abstract->stats.numberRxSignals        = 0;
    phy_abstract->stats.txSendTime             = 0;

    // For phy summary table
    if (db != NULL && (db->statsSummaryTable->createPhySummaryTable
                       || db->statsAggregateTable->createPhyAggregateTable))
    {
        node->phyData[phyIndex]->oneHopData =
            new std::vector<PhyOneHopNeighborData>();
    }
#endif
#ifdef ADDON_BOEINGFCS

    BOOL retVal;
    char buf[MAX_STRING_LENGTH];
    // Radio Busy Stats
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "RADIO-BUSY-PERCENT-STATISTICS",
        &retVal,
        buf);

    if (retVal) {
        if (strcmp(buf, "YES") == 0)
        {
            phy_abstract->radioBusyPercentStats = TRUE;
        }
        else
        {
            phy_abstract->radioBusyPercentStats = FALSE;
        }
    } else {
        phy_abstract->radioBusyPercentStats = FALSE;
    }

    // Battery Power Stats
    char buf2[MAX_STRING_LENGTH];
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "BATTERY-POWER-STATISTICS",
        &retVal,
        buf2);

    if (retVal) {
        if (strcmp(buf2, "YES") == 0)
        {
            phy_abstract->batteryPowerStats = TRUE;
        }
        else
        {
            phy_abstract->batteryPowerStats = FALSE;
        }
    } else {
        phy_abstract->batteryPowerStats = FALSE;
    }


    // Interference Power Stats
    char buf3[MAX_STRING_LENGTH];
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "INTERFERENCE-POWER-STATISTICS",
        &retVal,
        buf3);

    if (retVal) {
        if (strcmp(buf3, "YES") == 0)
        {
            phy_abstract->interferenceStats = TRUE;
        }
        else
        {
            phy_abstract->interferenceStats = FALSE;
        }
    } else {
        phy_abstract->interferenceStats = FALSE;
    }
#endif

    if (DYNAMIC_STATS) {
         phy_abstract->stats.totalTxSignalsId =
             GUI_DefineMetric("Number of Signals Transmitted",
                              node->nodeId, GUI_PHY_LAYER, 0,
                              GUI_INTEGER_TYPE, GUI_CUMULATIVE_METRIC);
         phy_abstract->stats.totalSignalsDetectedId =
             GUI_DefineMetric("Number of Signals Detected",
                              node->nodeId, GUI_PHY_LAYER, 0,
                              GUI_INTEGER_TYPE, GUI_CUMULATIVE_METRIC);
         phy_abstract->stats.totalSignalsLockedId =
             GUI_DefineMetric("Number of Signals Locked onto",
                              node->nodeId, GUI_PHY_LAYER, 0,
                              GUI_INTEGER_TYPE, GUI_CUMULATIVE_METRIC);
         phy_abstract->stats.totalSignalsWithErrorsId =
             GUI_DefineMetric("Number of Signals Received with Errors",
                              node->nodeId, GUI_PHY_LAYER, 0,
                              GUI_INTEGER_TYPE, GUI_CUMULATIVE_METRIC);
         phy_abstract->stats.totalRxSignalsToMacId =
             GUI_DefineMetric("Number of Signals Forwarded to MAC layer",
                              node->nodeId, GUI_PHY_LAYER, 0,
                              GUI_INTEGER_TYPE, GUI_CUMULATIVE_METRIC);
         phy_abstract->stats.currentSINRId =
             GUI_DefineMetric("Signal-to-Noise Ratio (dB)",
                              node->nodeId, GUI_PHY_LAYER, 0,
                              GUI_DOUBLE_TYPE, GUI_AVERAGE_METRIC);
         phy_abstract->stats.energyConsumedId =
             GUI_DefineMetric("Energy Consumption (mWhr)",
                              node->nodeId, GUI_PHY_LAYER, 0,
                              GUI_DOUBLE_TYPE, GUI_CUMULATIVE_METRIC);
    }
    if (phy_abstract->thisPhy->phyStats)
    {
        phy_abstract->thisPhy->stats = new STAT_PhyStatistics(node);
    }
    //
    // Initialize status of phy
    //
#ifdef ADDON_BOEINGFCS
    phy_abstract->linkAdaptationEnabled = FALSE;
#endif
    phy_abstract->rxMsg = NULL;
    phy_abstract->rxMsgError = FALSE;
    phy_abstract->rxMsgPower_mW = 0.0;
    phy_abstract->interferencePower_mW = 0.0;
    phy_abstract->txNodeId = 0;
    phy_abstract->rxChannelIndex = -1;
    phy_abstract->pathloss_dB = 0.0;

    phy_abstract->noisePower_mW =
        phy_abstract->thisPhy->noise_mW_hz * phy_abstract->bandwidth;

    phy_abstract->rxTimeEvaluated = 0;
    phy_abstract->rxEndTime = 0;
    phy_abstract->rxDOA.azimuth = 0;
    phy_abstract->rxDOA.elevation = 0;

    phy_abstract->previousMode = PHY_IDLE;
    phy_abstract->mode = PHY_IDLE;
    PhyAbstractChangeState(node,phyIndex, PHY_IDLE);

    if (phy_abstract->noisePower_mW >= phy_abstract->rxSensitivity_mW)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "PHY-ABSTRACT-RX-SENSITIVITY (%f) needs to be "
                          "greater than noise power (%f)\n",
                          IN_DB(phy_abstract->rxSensitivity_mW),
                          IN_DB(phy_abstract->noisePower_mW));

        ERROR_ReportError(errorStr);
    }

    double sinr = -1.0;
    BOOL headerError =
                    PhyAbstractCheckRxPacketError(
                        node,
                        phy_abstract,
                        TRUE,
#ifdef ADDON_BOEINGFCS
                        phy_abstract->rxSensitivity_mW / 10.0,
#else
                        phy_abstract->rxThreshold_mW / 10.0,
#endif
                        &sinr);

    if (!headerError)
    {
        char errorStr[MAX_STRING_LENGTH];
#ifdef ADDON_BOEINGFCS
        sprintf(errorStr, "PHY-ABSTRACT-RX-SENSITIVITY (%f) needs to be reduced\n",
                          IN_DB(phy_abstract->rxSensitivity_mW));
#else
        sprintf(errorStr, "PHY-ABSTRACT-RX-THRESHOLD (%f) needs to be reduced\n",
                          IN_DB(phy_abstract->rxThreshold_mW));
#endif
        ERROR_ReportWarning(errorStr);
    }

    //
    // Setting up the channel to use for both TX and RX
    //
    for (i = 0; i < numChannels; i++) {
        if (phy_abstract->thisPhy->channelListening[i] == TRUE) {
            break;
        }
    }
    assert(i != numChannels);

    //
    // Validation check. Abstract PHY supports single channel.
    //
    int j;
    char errStr[MAX_STRING_LENGTH];
    for (j = i+1; j < numChannels; j++) {
        if (phy_abstract->thisPhy->channelListening[j] == TRUE) {
            sprintf(errStr, "PHY-ABSTRACT Radio currently support"
                            " listening on single channel."
                            " Node %d is listening to multiple channels.\n",
                            node->nodeId);

            ERROR_ReportWarning(errStr);
            break;
        }
    }

    PHY_SetTransmissionChannel(node, phyIndex, i);

#ifdef JNE_LIB
    phy_abstract->mibDatastore = new JtrsWnwSisMibDatastore();
#endif

    return;
}



void PhyAbstractSignalArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo)
{
#ifdef ADDON_BOEINGFCS
    // JDL: This PCOM code used to be in propagation_private.cpp.  I moved it here since
    // it is only used by phy_abstract.  This block of code acts as an initial gate to see
    // if the packet can be received.  This can probably be improved further but this is
    // a first step.
    if (node->phyData[phyIndex]->phyRxModel == PCOM_BASED)
    {
        BOOL isReceived = FALSE;
        PhyPcomItem *iterator;
        PhyPcomItem *temp;
        clocktype now = node->getNodeTime();
        iterator = node->phyData[phyIndex]->pcomTable[propRxInfo->txMsg->originatingNodeId];
        if (iterator == NULL)
        {
            if (node->nodeId != propRxInfo->txMsg->originatingNodeId)
            {
                if (DEBUG_PCOM)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(now, clockStr);
                    printf("Time %s: Node %d, interface %d, originating node %d\n",
                           clockStr, node->nodeId, phyIndex,
                           propRxInfo->txMsg->originatingNodeId);

                    ERROR_ReportWarning("No PCOM Value found, dropping packet");
                }
            }
        }
        else
        {
            //we should never delete all pcom items since the last one
            //should have an end time of clocktype_max
            while (iterator && iterator->endTime < now)
            {
                temp = iterator;
                iterator = iterator->next;
                if (DEBUG_PCOM)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(temp->startTime, clockStr);
                    char clockStr2[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(temp->endTime, clockStr2);
                    printf("Node %d from %d deleting entry start %s end %s pcom %f\n",
                           node->nodeId, propRxInfo->txMsg->originatingNodeId, clockStr,
                           clockStr2, temp->pcom);
                }
                node->phyData[phyIndex]->pcomTable[propRxInfo->txMsg->originatingNodeId] =
                        iterator;
                MEM_free(temp);//remove expired entries
            }
            ERROR_Assert(iterator != NULL, "No pcom values in list");

            if (iterator->pcom >= node->phyData[phyIndex]->minPcomValue)
            {
                if (DEBUG_PCOM)
                {
                    printf("Node %d from %d has pcom higher than min\n",
                           node->nodeId, propRxInfo->txMsg->originatingNodeId);
                }

                // TODO: Receive it
                isReceived = TRUE;
            }
        }

        if (!isReceived)
        {
            return;
        }
    }
#endif

    PhyData *thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;
    PhyAbstractStats* stats = &(phy_abstract->stats);

#ifdef ADDON_BOEINGFCS
    MacData *macCesData =
            node->macData[thisPhy->macInterfaceIndex];
    if (macCesData && ( macCesData->macProtocol == MAC_PROTOCOL_CES_SINCGARS))
    {
        // for SINCGARS, signal arrival means network sensing....
        // need to report txStartTime and Duration, also need to decode the
        // transmission header to get the msg precedence embeded in the rx
        PropTxInfo *propTxInfoCes
                = (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);
        clocktype txDurationCes = propTxInfoCes->duration;
        clocktype txStartTimeCes = propTxInfoCes->txStartTime;
        NetworkCesIncSincgarsNacSenseSignal(
                node,
                thisPhy->macInterfaceIndex,
                propRxInfo->txMsg,
                txStartTimeCes,
                txDurationCes);
    }
#endif
#ifdef MILITARY_RADIOS_LIB
    int tadilNetNumber = propRxInfo->txMsg->subChannelIndex;
    MacData *mac = node->macData[thisPhy->macInterfaceIndex];
    if (mac->macProtocol == MAC_PROTOCOL_TADIL_LINK16)
    {
        MacDataLink16 *link16 = (MacDataLink16 *)mac->macVar;
        if (!PHY_IsListeningToChannel(node,
                                      link16->myMac->phyNumber,
                                      link16->channelIndex))
        {
            return;
        }
        int prevSlotId =
            (link16->presentSlotId + LINK16_SLOTS_PER_FRAME - 1)
            % LINK16_SLOTS_PER_FRAME;
        Link16SlotInfo *slotInfo = &link16->slot[prevSlotId];
        switch (slotInfo->slotType)
        {
            case LINK16_CONTENTION_ACCESS:
            {
                Link16NpgInfo *npgInfo = slotInfo->infoList;
                while (npgInfo)
                {
                    if (npgInfo->slotType == LINK16_CONTENTION_ACCESS &&
                        npgInfo->tadilNetNumber == tadilNetNumber)
                    {
                        break;
                    }
                    npgInfo = npgInfo->next;
                }
                if (!npgInfo)
                {
                    // Signals are from TADIL nets I am not listening to,
                    // thus do not received.
                    return;
                }
                // Otherwise, receiving the signal
                break;
            }
            case LINK16_RELAY:
            {
                Link16NpgInfo *npgInfo = slotInfo->infoList;
                while (npgInfo)
                {
                    if (npgInfo->slotType == LINK16_RELAY &&
                        npgInfo->tadilNetNumber == tadilNetNumber)
                    {
                        break;
                    }
                    npgInfo = npgInfo->next;
                }
                if (!npgInfo)
                {
                    // Signals are from TADIL nets I am not listening to,
                    // thus do not received.
                    return;
                }
                // Otherwise, receiving the signal
                break;
            }
            case LINK16_RECEIVE:
            {
                if (slotInfo->tadilNetNumber != tadilNetNumber)
                {
                    // Signals are from TADIL nets I am not listening to,
                    // thus do not received.
                    return;
                }

                // Receiving the signal
                break;
            }
            default:
            {
                // My TADIL device is not listening
                return;
            }
        } // switch

        if (DEBUG)
        {
            char clockStr[MAX_CLOCK_STRING_LENGTH];

            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            printf("Time %s: Link16 signal arrived on node %u in net %d\n",
                   clockStr, node->nodeId, tadilNetNumber);
        }
    } // if LINK16
#endif // MILITARY_RADIOS_LIB
    double rxPower_mW =
        NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
               propRxInfo->rxPower_dBm);
    double sinr = -1.0;

    if (phy_abstract->thisPhy->phyStats)
    {
        phy_abstract->thisPhy->stats->AddSignalDetectedDataPoints(node);

        if (DYNAMIC_STATS) {
            GUI_SendUnsignedData(node->nodeId,
                                 phy_abstract->stats.totalSignalsDetectedId,
                                 (unsigned)phy_abstract->thisPhy->stats->GetSignalsDetected().GetValue(node->getNodeTime()),
                                 node->getNodeTime());
        }
    }
    assert(phy_abstract->mode != PHY_TRANSMITTING);

    switch (phy_abstract->mode) {
        case PHY_RECEIVING: {
            PHY_NotificationOfPacketDrop(
                node,
                phyIndex,
                channelIndex,
                propRxInfo->txMsg,
                "PHY Busy in Receiving",
                NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
                       propRxInfo->rxPower_dBm),
                phy_abstract->interferencePower_mW,
                propRxInfo->pathloss_dB);

#ifdef ADDON_BOEINGFCS
            if (DEBUG_PCOM)
            {
                printf("node %d from %d arrives at phy\n", node->nodeId,
                       propRxInfo->txMsg->originatingNodeId);
            }
#endif
            if (!phy_abstract->rxMsgError) {
#ifdef ADDON_BOEINGFCS
                if (!phy_abstract->linkAdaptationEnabled)
                {
                    if (thisPhy->phyRxModel == PCOM_BASED)
                    {
                        assert(phy_abstract->rxMsgError == FALSE);
                        if (thisPhy->pcomTable[propRxInfo->txMsg->originatingNodeId] == NULL)
                        {
                            printf("Node %d at signal arrival from %d\n", node->nodeId,
                                propRxInfo->txMsg->originatingNodeId);
                        }
                        phy_abstract->rxMsgError =
                            PhyAbstractPcomRxPacketError(node,
                                                         thisPhy,
                                      propRxInfo->txMsg->originatingNodeId);
                    }
                    else
                    {
#endif
                        phy_abstract->rxMsgError =
                            PhyAbstractCheckRxPacketError(
                                node,
                                phy_abstract,
                                FALSE,
                                0.0,
                                &sinr);
#ifdef ADDON_BOEINGFCS
                    }
                }
#endif /* ADDON_BOEINGFCS */

#ifdef ADDON_BOEINGFCS
// added for link adaptation routing
                if (phy_abstract->linkAdaptationEnabled)
                {
                    MacData *macData = node->macData[thisPhy->macInterfaceIndex];
                    if (macData
                        && macData->macProtocol == MAC_PROTOCOL_CES_WNW_MDL)
                    {
                        LinkAdaptationTxParameters* tmp = NULL;
                        tmp = (LinkAdaptationTxParameters*)
                            MESSAGE_ReturnInfo(propRxInfo->txMsg,
                            INFO_TYPE_LinkCesWnwAdaptation);
                        if (!tmp)
                        {
                            ERROR_ReportError("InfoField "
                                "INFO_TYPE_LinkCesWnwAdaptation does "
                                "not exist !!! \n");
                        }
                        phy_abstract->rxMsgError =
                            LinkCesWnwCheckRxPacketError(node,
                                                     phyIndex,
                                                     channelIndex,
                                                     tmp->txStateId);
                    }
                }
#endif
            }//if//

#ifdef ADDON_BOEINGFCS
// ----- "Temporary solution" for Rake Receiver : Begin
            // During TDMA control slot(s), a node may receives
            // multiple identical signals (packets) at the same time from different nodes.
            // Thoes signals should not be considered interference to each other,
            // and in fact together they produce a better signal at receiver.

            MacData *macData = node->macData[thisPhy->macInterfaceIndex];
            if (macData != NULL)
            {
                if (macData->macProtocol == MAC_PROTOCOL_CES_EPLRS)
                {
                    NetworkCesIncEplrsCTUHeaderInfo* headerInfoTxMsg =
                        (NetworkCesIncEplrsCTUHeaderInfo*) MESSAGE_ReturnInfo
                        (propRxInfo->txMsg, INFO_TYPE_NetworkCesIncEplrsCTU);
                    NetworkCesIncEplrsCTUHeaderInfo* headerInfoRxMsg =
                        (NetworkCesIncEplrsCTUHeaderInfo*) MESSAGE_ReturnInfo
                        (phy_abstract->rxMsg, INFO_TYPE_NetworkCesIncEplrsCTU);

                    if (headerInfoTxMsg != NULL && headerInfoRxMsg != NULL)
                    {
                        if (headerInfoRxMsg->queue_index == 0 &&
                                headerInfoTxMsg->queue_index == 0)
                        {
                            // CSMA Needline.
                            if (phy_abstract->rxMsg->originatingNodeId ==
                                    propRxInfo->txMsg->originatingNodeId ||
                                    phy_abstract->rxMsg->sequenceNumber ==
                                    propRxInfo->txMsg->sequenceNumber)
                            {
                                phy_abstract->rxMsgPower_mW += rxPower_mW;
                                break;
                            }
                        }
                    }
                }
            }
// ----- "Temporary solution" for Rake Receiver : End
#endif /* ADDON_BOEINGFCS */

            phy_abstract->rxTimeEvaluated = node->getNodeTime();
            phy_abstract->interferencePower_mW += rxPower_mW;
            stats->numUpdates++;
            break;
        }

        //
        // If the phy is idle or sensing,
        // check if it can receive this signal.
        //
        case PHY_IDLE:
        case PHY_SENSING:
        {
            double rxPowerInOmni_mW =
            NON_DB(ANTENNA_DefaultGainForThisSignal(node, phyIndex, propRxInfo) +
                propRxInfo->rxPower_dBm);
#ifdef ADDON_BOEINGFCS
            if (DEBUG_PCOM)
            {
                char now[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), now);
                printf("node %d from %d arrives at phy SENSING at %s\n", node->nodeId,
                       propRxInfo->txMsg->originatingNodeId, now);
                if (thisPhy->collisionWindowActive)
                {
                    char timeStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
                    printf(  "collision window is active, signal will not be heard! %s\n",
                           timeStr);
                }
            }
#endif
#ifndef ADDON_BOEINGFCS
            if (rxPowerInOmni_mW >= phy_abstract->rxSensitivity_mW) {
#else
            if (rxPowerInOmni_mW >= phy_abstract->rxThreshold_mW ||
                (thisPhy->phyRxModel == PCOM_BASED &&
                !thisPhy->collisionWindowActive)) {
#endif
                PropTxInfo *propTxInfo
                    = (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);
                clocktype txDuration = propTxInfo->duration;

#ifdef ADDON_BOEINGFCS
                Message *collisionWindowMsg;

                if (thisPhy->phyRxModel == PCOM_BASED)
                {
                    thisPhy->collisionWindowActive = TRUE;

                    if (DEBUG_PCOM)
                    {
                        char timeStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime()+thisPhy->syncCollisionWindow, timeStr);
                        printf("Node %d: collision window active until %s\n",
                            node->nodeId, timeStr);
                    }

                    //notify yourself when collision window has passed
                    collisionWindowMsg = MESSAGE_Alloc(node,
                                                    PHY_LAYER,
                                                    0,
                                                    MSG_PHY_CollisionWindowEnd);

                    MESSAGE_SetInstanceId(collisionWindowMsg, (short) phyIndex);
                    MESSAGE_Send(node, collisionWindowMsg, thisPhy->syncCollisionWindow);
                }

#endif
                if (!ANTENNA_IsLocked(node, phyIndex)) {
                    ANTENNA_SetToBestGainConfigurationForThisSignal(
                            node, phyIndex, propRxInfo);
                }

                PHY_SignalInterference(
                        node,
                        phyIndex,
                        channelIndex,
                        propRxInfo->txMsg,
                        &rxPower_mW,
                        &(phy_abstract->interferencePower_mW));

                BOOL isHeaderError =
                    PhyAbstractCheckRxPacketError(
                        node,
                        phy_abstract,
                        TRUE,
                        rxPower_mW,
                        &sinr);

                if ((rxPower_mW >= phy_abstract->rxThreshold_mW) &&
                    (!isHeaderError)) {

                    PhyAbstractLockSignal(
                        node,
                        phy_abstract,
                        propRxInfo,
                        propRxInfo->txMsg,
                        rxPower_mW,
                        (propRxInfo->rxStartTime + propRxInfo->duration),
                        channelIndex);
#ifdef CYBER_LIB
                    if (node->phyData[phyIndex]->jammerStatistics == TRUE)
                    {
                        if (node->phyData[phyIndex]->jamInstances > 0)
                        {
                            phy_abstract->stats.totalSignalsLockedDuringJam++;
                        }
                    }
#endif

                    if (DEBUG) {
                        printf("DEBUG: Node %d locked on to new signal "
                               "from node %d with rxPower (dB) = %f\n",
                               node->nodeId,
                               propRxInfo->txMsg->originatingNodeId,
                               propRxInfo->rxPower_dBm);
                        fflush(stdout);
                    }

                    if (DYNAMIC_STATS) {
                        if (phy_abstract->thisPhy->phyStats)
                        {
                            GUI_SendRealData(node->nodeId,
                                           phy_abstract->stats.totalSignalsLockedId,
                                           (unsigned)phy_abstract->thisPhy->stats->GetSignalsLocked().GetValue(node->getNodeTime()),
                                           node->getNodeTime());
                        }
                    }

                    PhyAbstractChangeState(node,phyIndex, PHY_RECEIVING);

#ifdef ALE_ASAPS_LIB
                    unsigned int *info;
                    MESSAGE_AddInfo(node,
                                    propRxInfo->txMsg,
                                    sizeof(unsigned int),
                                    INFO_TYPE_ALE_ChannelIndex);
                    info = (unsigned int *) MESSAGE_ReturnInfo
                                           (propRxInfo->txMsg,
                                            INFO_TYPE_ALE_ChannelIndex);
                    *info = channelIndex;
#endif
                    PhyAbstractReportExtendedStatusToMac(
                        node,
                        phyIndex,
                        PHY_RECEIVING,
                        txDuration,
                        propRxInfo->txMsg);
                } // no error in header
                else {
                    //
                    // there is error in header, check if the signal change its status
                    //
                    PHY_NotificationOfPacketDrop(
                        node,
                        phyIndex,
                        channelIndex,
                        propRxInfo->txMsg,
                        "Header In Error",
                        rxPower_mW,
                        phy_abstract->interferencePower_mW,
                        propRxInfo->pathloss_dB);

                    PhyStatusType newMode;

                    if (DEBUG) {
                        printf("DEBUG: Node %d unable to lock on to new signal "
                               "from node %d with rxPower (dB) = %f\n",
                               node->nodeId,
                               propRxInfo->txMsg->originatingNodeId,
                               IN_DB(rxPower_mW));
                        fflush(stdout);
                    }

                    phy_abstract->interferencePower_mW += rxPower_mW;
                    stats->numUpdates++;

                    if (PhyAbstractCarrierSensing(node, phy_abstract))
                    {
                        newMode = PHY_SENSING;
                    } else {
                        newMode = PHY_IDLE;
                    }//if//

                    if (newMode != phy_abstract->mode) {
                        PhyAbstractChangeState(node,phyIndex, newMode);
                        PhyAbstractReportStatusToMac(
                            node,
                            phyIndex,
                            newMode);
                    }//if//
                }//header in error && signal power > phy_abstract->rxThreshold_mW
            } // signal power > phy_abstract->rxThreshold_mW
            else {
                //
                // Otherwise, check if the signal change its status
                //
                PHY_NotificationOfPacketDrop(
                    node,
                    phyIndex,
                    channelIndex,
                    propRxInfo->txMsg,
                    "Signal below Rx Threshold",
                    rxPowerInOmni_mW,
                    phy_abstract->interferencePower_mW,
                    propRxInfo->pathloss_dB);

                PhyStatusType newMode;

                if (DEBUG) {
                    printf("DEBUG: Node %d unable to lock on to new signal "
                           "from node %d with rxPower (dB) = %f\n",
                           node->nodeId,
                           propRxInfo->txMsg->originatingNodeId,
                           IN_DB(rxPowerInOmni_mW));
                    fflush(stdout);
                }

                phy_abstract->interferencePower_mW += rxPowerInOmni_mW;
                stats->totalInterference_mW += rxPowerInOmni_mW;
                stats->numUpdates++;

                if (PhyAbstractCarrierSensing(node, phy_abstract))
                {
                    newMode = PHY_SENSING;
                }
                else {
                    newMode = PHY_IDLE;
                }//if//

                if (newMode != phy_abstract->mode) {
                    PhyAbstractChangeState(node,phyIndex, newMode);
                    PhyAbstractReportStatusToMac(
                        node,
                        phyIndex,
                        newMode);
                }//if//
            }//signal power < phy_abstract->rxThreshold_mW


            break;
        }

        default:
            abort();

    }//switch (phy_abstract->mode)//
}


void PhyAbstractTerminateCurrentReceive(
    Node        *node,
    int         phyIndex,
    const BOOL  terminateOnlyOnReceiveError,
    BOOL        *frameError,
    clocktype   *endSignalTime)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract *phy_abstract = (PhyDataAbstract *)thisPhy->phyVar;
    double sinr;

    *endSignalTime = phy_abstract->rxEndTime;

    if (!phy_abstract->rxMsgError) {
       phy_abstract->rxMsgError =
           PhyAbstractCheckRxPacketError(
               node,
               phy_abstract,
               FALSE,
               0.0,
               &sinr);
    }//if//

    *frameError = phy_abstract->rxMsgError;

    if ((terminateOnlyOnReceiveError) && (!phy_abstract->rxMsgError)) {
        return;
    }//if//

    if (thisPhy->antennaData->antennaModelType == ANTENNA_OMNIDIRECTIONAL)
    {
        phy_abstract->interferencePower_mW += phy_abstract->rxMsgPower_mW;
    }
    else
    {
        int channelIndex;
        PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

        ERROR_Assert(((thisPhy->antennaData->antennaModelType
                        == ANTENNA_SWITCHED_BEAM) ||
                    (thisPhy->antennaData->antennaModelType
                        == ANTENNA_STEERABLE) ||
                    (thisPhy->antennaData->antennaModelType
                        == ANTENNA_PATTERNED)),
                    "Illegal antennaModelType");

        if (!ANTENNA_IsLocked(node, phyIndex))
        {
            ANTENNA_SetToDefaultMode(node, phyIndex);
        }

         PHY_SignalInterference(node, phyIndex, channelIndex, NULL, NULL,
                                   &(phy_abstract->interferencePower_mW));
    }

    // if the phy was receiving a signal, now it has to drop the signal
    // by calling unlock the signal
    int chIndex;
    PHY_GetTransmissionChannel(node, phyIndex, &chIndex);
    PHY_NotificationOfPacketDrop(
        node,
        phyIndex,
        chIndex,
        phy_abstract->rxMsg,
        "Rx Terminated by MAC",
        phy_abstract->rxMsgPower_mW,
        phy_abstract->interferencePower_mW,
        0.0);

    if (phy_abstract->thisPhy->phyStats)
    {
        phy_abstract->thisPhy->stats->AddSignalTerminatedDataPoints(
                                        node,
                                        thisPhy,
                                        phy_abstract->rxMsg,
                                        phy_abstract->rxChannelIndex,
                                        phy_abstract->txNodeId,
                                        phy_abstract->pathloss_dB,
                                        phy_abstract->rxTimeEvaluated,
                                        phy_abstract->interferencePower_mW,
                                        phy_abstract->rxMsgPower_mW);
    }
    PhyAbstractUnlockSignal(phy_abstract);

    phy_abstract->previousMode = phy_abstract->mode;
    if (PhyAbstractCarrierSensing(node, phy_abstract))
    {
        PhyAbstractChangeState(node, phyIndex, PHY_SENSING);
    }
    else
    {
        PhyAbstractChangeState(node, phyIndex, PHY_IDLE);
    } // if//

#ifdef ADDON_BOEINGFCS
    PhyAbstractReportStatusToMac(node, phyIndex, phy_abstract->mode);
#endif
} /* End of PhyAbstractTerminateCurrentReceive */

void PhyAbstractTerminateCurrentTransmission(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract *phy_abstract = (PhyDataAbstract *)thisPhy->phyVar;
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
    assert(phy_abstract->mode == PHY_TRANSMITTING);

    if (!ANTENNA_IsLocked(node, phyIndex)) {
        ANTENNA_SetToDefaultMode(node, phyIndex);
    }

    //Cancel the timer end message so that 'PhyAbstractTransmissionEnd' is
    // not called.
    if (phy_abstract->txEndTimer)
    {
        MESSAGE_CancelSelfMsg(node, phy_abstract->txEndTimer);
        phy_abstract->txEndTimer = NULL;
    }
}

void PhyAbstractSignalEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo)
{
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;
    double rxPower_mW;

#ifdef MILITARY_RADIOS_LIB
    int tadilNetNumber = propRxInfo->txMsg->subChannelIndex;
    PHY_SignalInterference(
        node,
        phyIndex,
        channelIndex,
        propRxInfo->txMsg,
        &rxPower_mW,
        &(phy_abstract->interferencePower_mW),
        tadilNetNumber);
#else // MILITARY_RADIOS_LIB
    PHY_SignalInterference(
        node,
        phyIndex,
        channelIndex,
        propRxInfo->txMsg,
        &rxPower_mW,
        &(phy_abstract->interferencePower_mW));
#endif // MILITARY_RADIOS_LIB

#ifdef MILITARY_RADIOS_LIB
    MacData *mac = node->macData[thisPhy->macInterfaceIndex];
    if (mac->macProtocol == MAC_PROTOCOL_TADIL_LINK16)
    {
        MacDataLink16 *link16 = (MacDataLink16 *)mac->macVar;
        if (!PHY_IsListeningToChannel(node,
                                      link16->myMac->phyNumber,
                                      link16->channelIndex))
        {
            return;
        }
        int prevSlotId =
            (link16->presentSlotId + LINK16_SLOTS_PER_FRAME - 1) % LINK16_SLOTS_PER_FRAME;
        Link16SlotInfo *slotInfo = &link16->slot[prevSlotId];
        switch (slotInfo->slotType)
        {
            case LINK16_CONTENTION_ACCESS:
            {
                Link16NpgInfo *npgInfo = slotInfo->infoList;
                while (npgInfo)
                {
                    if (npgInfo->slotType == LINK16_CONTENTION_ACCESS &&
                        npgInfo->tadilNetNumber == tadilNetNumber)
                    {
                        break;
                    }
                    npgInfo = npgInfo->next;
                }
                if (!npgInfo)
                {
                    // Signals are from TADIL nets I am not listening to,
                    // thus do not affect this channel
                    return;
                }
                // Otherwise, receiving the signal
                break;
            }
            case LINK16_RELAY:
            {
                Link16NpgInfo *npgInfo = slotInfo->infoList;
                while (npgInfo)
                {
                    if (npgInfo->slotType == LINK16_RELAY &&
                        npgInfo->tadilNetNumber == tadilNetNumber)
                    {
                        break;
                    }
                    npgInfo = npgInfo->next;
                }
                if (!npgInfo)
                {
                    // Signals are from TADIL nets I am not listening to,
                    // thus do not affect this channel
                    return;
                }
                // Otherwise, receiving the signal
                break;
            }
            case LINK16_RECEIVE:
            {
                if (slotInfo->tadilNetNumber != tadilNetNumber)
                {
                    return;
                }

                // Receiving the signal
                break;
            }
            default:
            {
                // My TADIL device is not listening
                return;
            }
        } // switch

        if (DEBUG)
        {
            char clockStr[MAX_CLOCK_STRING_LENGTH];

            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            printf("Time %s: Link16 signal end on node %u in net %d\n",
                   clockStr, node->nodeId, tadilNetNumber);
        }
    } // if LINK16
#endif // MILITARY_RADIOS_LIB
    double sinr;
#ifdef ADVANCED_WIRELESS_LIB
    double rss_mW = phy_abstract->rxMsgPower_mW;
#endif // ADVANCED_WIRELESS_LIB

    BOOL receiveErrorOccurred = FALSE;

    assert(phy_abstract->mode != PHY_TRANSMITTING);

#ifdef ADDON_BOEINGFCS
    if (DEBUG_PCOM)
    {
        printf("Node %d from %d at signal end from channel\n",
               node->nodeId, propRxInfo->txMsg->originatingNodeId);
    }
#endif

    if (phy_abstract->mode == PHY_RECEIVING &&
        phy_abstract->rxMsg == propRxInfo->txMsg)
    {
        // to record signal arrive event
        PHY_NotificationOfSignalReceived(
            node,
            phyIndex,
            channelIndex,
            phy_abstract->rxMsg,
            phy_abstract->rxMsgPower_mW,
            phy_abstract->interferencePower_mW,
            propRxInfo->pathloss_dB,
            PHY_ABSTRACT_DEFAULT_HEADER_SIZE);
    }

    if (phy_abstract->mode == PHY_RECEIVING) {
        if (phy_abstract->rxMsgError == FALSE) {
#ifdef ADDON_BOEINGFCS
            if (!phy_abstract->linkAdaptationEnabled) {
            if (thisPhy->phyRxModel == PCOM_BASED)
            {
                assert(phy_abstract->rxMsgError == FALSE);
                if (thisPhy->pcomTable[propRxInfo->txMsg->originatingNodeId] == NULL)
                {
                    printf("Node %d at signal end from %d\n", node->nodeId,
                            propRxInfo->txMsg->originatingNodeId);
                }
                phy_abstract->rxMsgError =
                        PhyAbstractPcomRxPacketError(node,
                                                     thisPhy,
                                      propRxInfo->txMsg->originatingNodeId);
            }
            else
            {
#endif
                phy_abstract->rxMsgError =
                    PhyAbstractCheckRxPacketError(
                        node,
                        phy_abstract,
                        FALSE,
                        0.0,
                        &sinr);
#ifdef ADDON_BOEINGFCS
            }
            }
#endif

#ifdef ADDON_BOEINGFCS
// added for link adaptation routing
            if (phy_abstract->linkAdaptationEnabled)
            {
                MacData *macData = node->macData[thisPhy->macInterfaceIndex];
                if (macData
                    && macData->macProtocol == MAC_PROTOCOL_CES_WNW_MDL)
                {
                    LinkAdaptationTxParameters* tmp = NULL;
                    tmp = (LinkAdaptationTxParameters*)
                            MESSAGE_ReturnInfo(propRxInfo->txMsg,
                            INFO_TYPE_LinkCesWnwAdaptation);
                    if (!tmp)
                {
                        ERROR_ReportError("InfoField "
                                "INFO_TYPE_LinkCesWnwAdaptation does "
                                "not exist !!! \n");
                    }
                    phy_abstract->rxMsgError =
                        LinkCesWnwCheckRxPacketError(node,
                                                     phyIndex,
                                                     channelIndex,
                                                     tmp->txStateId);
                }
            }
#endif
            phy_abstract->rxTimeEvaluated = node->getNodeTime();
        }//if
    }//if//

    receiveErrorOccurred = phy_abstract->rxMsgError;
    //
    // If the phy is still receiving this signal, forward the frame
    // to the MAC layer.
    //

    if ((phy_abstract->mode == PHY_RECEIVING) &&
            (phy_abstract->rxMsg == propRxInfo->txMsg))
    {
#ifdef ADDON_BOEINGFCS
// routing for link adaptation
    if (phy_abstract->linkAdaptationEnabled)
    {
        MacData *macData = node->macData[thisPhy->macInterfaceIndex];

        if (macData && !receiveErrorOccurred &&
            macData->macProtocol == MAC_PROTOCOL_CES_WNW_MDL)
        {
            if (MAC_InterfaceIsEnabled(node, thisPhy->macInterfaceIndex))
            {
                LinkCesWnwAdaptationUpdateLinkQualityMetricsInfo(
                    node,
                    phyIndex,
                    channelIndex,
                    propRxInfo);
            }
        }
    }
#endif

        Message *newMsg;

        //Perform Signal measurement
        PhySignalMeasurement sigMeasure;
        sigMeasure.rxBeginTime = propRxInfo->rxStartTime;

        sigMeasure.rss = IN_DB(phy_abstract->rxMsgPower_mW);
        sigMeasure.snr = IN_DB(phy_abstract->rxMsgPower_mW /
                                    phy_abstract->noisePower_mW);
        sigMeasure.cinr = IN_DB(phy_abstract->rxMsgPower_mW /
                             (phy_abstract->interferencePower_mW
                             + phy_abstract->noisePower_mW));
#ifdef ADDON_DB
        StatsDb* db = node->partitionData->statsDb;
        NodeId *nextHop = (NodeId*)
                MESSAGE_ReturnInfo(propRxInfo->txMsg,
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

        if (!receiveErrorOccurred)
        {
            // Compute the delay
            PropTxInfo *txInfo = (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);
            phy_abstract->stats.numberRxSignals++;

            if (db != NULL && db->statsEventsTable->createPhyEventsTable &&
               (rcvdId == node->nodeId || rcvdId == ANY_DEST))
            {
                StatsDBMappingParam *mapParamInfo = (StatsDBMappingParam*)
                    MESSAGE_ReturnInfo(propRxInfo->txMsg, INFO_TYPE_StatsDbMapping);
                ERROR_Assert(mapParamInfo ,
                    "Error in StatsDB handling ReceiveSignal!");
                int msgSize = 0;
                if (!propRxInfo->txMsg->isPacked)
                {
                    msgSize = MESSAGE_ReturnPacketSize(propRxInfo->txMsg);
                }
                else
                {
                    msgSize = MESSAGE_ReturnActualPacketSize(propRxInfo->txMsg);
                }
                StatsDBPhyEventParam phyParam(node->nodeId,
                                              (std::string) mapParamInfo->msgId,
                                              phyIndex,
                                              msgSize,
                                              "PhySendToUpper");
                phyParam.SetChannelIndex(channelIndex);
                phyParam.SetControlSize(0);
                phyParam.SetPathLoss(propRxInfo->pathloss_dB);

                if (phy_abstract->rxMsgPower_mW )
                {
                    phyParam.SetSignalPower(IN_DB(phy_abstract->rxMsgPower_mW)) ;
                }
                else
                {
                    phyParam.SetSignalPower(0) ;
                }
                STATSDB_HandlePhyEventsTableInsert(node, phyParam);
            }
        }
#endif

        if (receiveErrorOccurred)
        {
            PHY_NotificationOfPacketDrop(
                node,
                phyIndex,
                channelIndex,
                propRxInfo->txMsg,
                "Signal Received with Error",
                phy_abstract->rxMsgPower_mW,
                phy_abstract->interferencePower_mW,
                propRxInfo->pathloss_dB);
        }

        double signalPower_mW = 0;
        if (phy_abstract)
        {
            signalPower_mW = phy_abstract->rxMsgPower_mW;
        }

        PhyAbstractUnlockSignal(phy_abstract);

        if (!ANTENNA_IsLocked(node, phyIndex)) {
            ANTENNA_SetToDefaultMode(node, phyIndex);
        }

        if (PhyAbstractCarrierSensing(
                node,
                phy_abstract) == TRUE)
        {
             PhyAbstractChangeState(node,phyIndex, PHY_SENSING);
        }
        else {
            PhyAbstractChangeState(node,phyIndex, PHY_IDLE);
#ifdef ADDON_BOEINGFCS
            PhyAbstractReportStatusToMac(node, phyIndex, phy_abstract->mode);
#endif
        }

        if (!receiveErrorOccurred) {

#ifdef ADDON_BOEINGFCS_LOG_PROP
            CES_HandlePropagationSuccessReceiver(
                node,
                propRxInfo->txMsg,
                sinr);
#endif /* ADDON_BOEINGFCS_LOG_PROP */

            newMsg = MESSAGE_Duplicate(node, propRxInfo->txMsg);

            MESSAGE_SetInstanceId(newMsg, (short) phyIndex);

            PhySignalMeasurement* signalMeaInfo;
            MESSAGE_InfoAlloc(node,
                              newMsg,
                              sizeof(PhySignalMeasurement));
            signalMeaInfo = (PhySignalMeasurement*)
                            MESSAGE_ReturnInfo(newMsg);
            memcpy(signalMeaInfo,&sigMeasure,sizeof(PhySignalMeasurement));

            phy_abstract->rxDOA = propRxInfo->rxDOA;

            MAC_ReceivePacketFromPhy(
                node,
                node->phyData[phyIndex]->macInterfaceIndex,
                newMsg,
                phyIndex);

            // Get the path profile
            PropTxInfo *txInfo = (PropTxInfo*) MESSAGE_ReturnInfo(propRxInfo->txMsg);

            // Compute the delay
            clocktype txDelay = propRxInfo->rxStartTime - txInfo->txStartTime;

            if (phy_abstract->thisPhy->phyStats)
            {
                phy_abstract->thisPhy->stats->AddSignalToMacDataPoints(
                    node,
                    propRxInfo,
                    thisPhy,
                    channelIndex,
                    txDelay,
                    phy_abstract->interferencePower_mW,
                    propRxInfo->pathloss_dB,
                    signalPower_mW);

                if (DYNAMIC_STATS)
                {
                    GUI_SendUnsignedData(node->nodeId,
                        phy_abstract->stats.totalRxSignalsToMacId,
                        (unsigned) phy_abstract->thisPhy->stats->GetSignalsToMac().GetValue(node->getNodeTime()),
                        node->getNodeTime());
                }
            }
#ifdef CYBER_LIB
            if (node->phyData[phyIndex]->jammerStatistics == TRUE)
            {
                if (node->phyData[phyIndex]->jamInstances > 0)
                {
                    phy_abstract->stats.totalRxSignalsToMacDuringJam++;
                }
            }
#endif
        }
        else {
            if (phy_abstract->thisPhy->phyStats)
            {
                phy_abstract->thisPhy->stats->AddSignalWithErrorsDataPoints(
                    node,
                    propRxInfo,
                    phy_abstract->thisPhy,
                    channelIndex,
                    phy_abstract->interferencePower_mW,
                    signalPower_mW);

                if (DYNAMIC_STATS)
                {
                    GUI_SendUnsignedData(node->nodeId,
                        phy_abstract->stats.totalSignalsWithErrorsId,
                        (unsigned)phy_abstract->thisPhy->stats->GetSignalsWithErrors().GetValue(node->getNodeTime()),
                        node->getNodeTime());
                }
            }
#ifdef CYBER_LIB
            if (node->phyData[phyIndex]->jammerStatistics == TRUE)
            {
                if (node->phyData[phyIndex]->jamInstances > 0)
                {
                    phy_abstract->stats.totalSignalsWithErrorsDuringJam++;
                }
            }
#endif
            PhyAbstractReportStatusToMac(node, phyIndex, phy_abstract->mode);

#ifdef ADDON_BOEINGFCS_LOG_PROP
            CES_HandlePropagationFailureReceiver(
                node,
                propRxInfo->txMsg,
                sinr);
#endif /* ADDON_BOEINGFCS_LOG_PROP */

        }
    }
    else {

        PhyStatusType newMode;

        if (phy_abstract->mode != PHY_RECEIVING) {
            if (PhyAbstractCarrierSensing(
                node,
                phy_abstract) == TRUE)
            {
                newMode = PHY_SENSING;
            }
            else {
                newMode = PHY_IDLE;
            }//if//

            if (newMode != phy_abstract->mode) {
                PhyAbstractChangeState(node,phyIndex, newMode);
                PhyAbstractReportStatusToMac(
                   node,
                   phyIndex,
                   newMode);
            }//if//

            if (!ANTENNA_IsLocked(node, phyIndex)) {
                ANTENNA_SetToDefaultMode(node, phyIndex);
            }
        }//if//
    }//if//
}


void PhyAbstractFinalize(Node *node, const int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;
    char buf[MAX_STRING_LENGTH];

    if (thisPhy->phyStats == FALSE) {
        return;
    }

    assert(thisPhy->phyStats == TRUE);

#ifdef CYBER_LIB

    if (node->phyData[phyIndex]->jammerStatistics == TRUE)
    {
        char durationStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->phyData[phyIndex]->jamDuration, durationStr, node);
        sprintf(buf, "Signals received and forwarded to MAC during the jam duration = %d",
            phy_abstract->stats.totalRxSignalsToMacDuringJam);
        IO_PrintStat(node, "Physical", "Abstract", ANY_DEST, phyIndex, buf);
        sprintf(buf, "Signals locked on by PHY during the jam duration = %d",
            phy_abstract->stats.totalSignalsLockedDuringJam);
        IO_PrintStat(node, "Physical", "Abstract", ANY_DEST, phyIndex, buf);

        sprintf(buf, "Signals received but with errors during the jam duration = %d",
            phy_abstract->stats.totalSignalsWithErrorsDuringJam);
        IO_PrintStat(node, "Physical", "Abstract", ANY_DEST, phyIndex, buf);
        sprintf(buf, "Total jam duration in (s) = %s", durationStr);
        IO_PrintStat(node, "Physical", "Abstract", ANY_DEST, phyIndex, buf);
    }

#endif

    phy_abstract->thisPhy->stats->Print(node,
        "Physical",
        "Abstract",
        ANY_ADDRESS,
        phyIndex);

#ifdef ADDON_BOEINGFCS
    if (phy_abstract->linkAdaptationEnabled)
    {

        PhyLinkCesWnwAdaptationFinalize(node, phyIndex);
    }
#endif // ADDON_BOEINGFCS

    PhyAbstractChangeState(node,phyIndex, PHY_IDLE);

#ifdef JNE_LIB
                // Delete Sis Layer's SNMP MIB Datastore
                //
                delete phy_abstract->mibDatastore;
#endif
}



void PhyAbstractTransmissionEnd(Node *node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract *)thisPhy->phyVar;
    int channelIndex;

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    assert(phy_abstract->mode == PHY_TRANSMITTING);

    phy_abstract->txEndTimer = NULL;
    //GuiStart
    if (node->guiOption == TRUE) {
        GUI_EndBroadcast(node->nodeId,
                         GUI_PHY_LAYER,
                         GUI_DEFAULT_DATA_TYPE,
                         thisPhy->macInterfaceIndex,
                         node->getNodeTime());
    }
    //GuiEnd

    PHY_StartListeningToChannel(node, phyIndex, channelIndex);
}



void PhyAbstractCollisionWindowEnd(Node *node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

#ifdef ADDON_BOEINGFCS
    thisPhy->collisionWindowActive = FALSE;

    if (DEBUG_PCOM)
    {
        char timeStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("Node %d collision window end at %s\n", node->nodeId, timeStr);
    }
#endif
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
    clocktype delayUntilAirborne = initDelayUntilAirborne;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract *)thisPhy->phyVar;
    int channelIndex;
    Message *endMsg;
    int packetsize = 0;
    clocktype duration;

    if (!packet->isPacked)
    {
        packetsize = MESSAGE_ReturnPacketSize(packet);
    }
    else
    {
        packetsize = MESSAGE_ReturnActualPacketSize(packet);
    }

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    if (!useMacLayerSpecifiedDelay) {
        delayUntilAirborne = PHY_ABSTRACT_RX_TX_TURNAROUND_TIME;
    }//if//

    assert(phy_abstract->mode != PHY_TRANSMITTING);

    if (sendDirectionally) {
        ANTENNA_SetBestConfigurationForAzimuth(node, phyIndex,
                                               azimuthAngle);
    }//if//


    if (phy_abstract->mode == PHY_RECEIVING)
    {
        if (thisPhy->antennaData->antennaModelType ==
            ANTENNA_OMNIDIRECTIONAL)
        {
            phy_abstract->interferencePower_mW += phy_abstract->rxMsgPower_mW;
        }
        else
        {
            if (!sendDirectionally)
            {
                ANTENNA_SetToDefaultMode(node, phyIndex);
            }//if//

            //GuiStart
            if (node->guiOption)
            {
                GUI_SetPatternIndex(node,
                                    thisPhy->macInterfaceIndex,
                                    ANTENNA_OMNIDIRECTIONAL_PATTERN,
                                    node->getNodeTime());
            }
            //GuiEnd

            PHY_SignalInterference(
                    node,
                    phyIndex,
                    channelIndex,
                    NULL,
                    NULL,
                    &(phy_abstract->interferencePower_mW));
        }
        // if the phy was receiving a signal, now it has to drop the signal
        // in order to transmit, by calling unlock the signal
        PHY_NotificationOfPacketDrop(
            node,
            phyIndex,
            channelIndex,
            phy_abstract->rxMsg,
            "PHY Stop Rx for Tx",
            phy_abstract->rxMsgPower_mW,
            phy_abstract->interferencePower_mW,
            0.0);

        if (phy_abstract->thisPhy->phyStats)
        {
            phy_abstract->thisPhy->stats->AddSignalTerminatedDataPoints(
                                        node,
                                        thisPhy,
                                        phy_abstract->rxMsg,
                                        phy_abstract->rxChannelIndex,
                                        phy_abstract->txNodeId,
                                        phy_abstract->pathloss_dB,
                                        phy_abstract->rxTimeEvaluated,
                                        phy_abstract->interferencePower_mW,
                                        phy_abstract->rxMsgPower_mW);
        }
        PhyAbstractUnlockSignal(phy_abstract);
    }

    PhyAbstractChangeState(node, phyIndex, PHY_TRANSMITTING);

#ifdef ADDON_BOEINGFCS
    PhyAbstractReportStatusToMac(node, phyIndex, phy_abstract->mode);

    if (phy_abstract->linkAdaptationEnabled)
    {

        LinkAdaptationTxParameters* tmp;
        tmp = (LinkAdaptationTxParameters*)
              MESSAGE_ReturnInfo(packet, INFO_TYPE_LinkCesWnwAdaptation);
        if (tmp)
        {
            phy_abstract->dataRate = tmp->dataRate;
            phy_abstract->txPower_dBm = tmp->txPower * STEP_SIZE;

            clocktype packetDuration =
                PhyAbstractGetFrameDuration(
                    thisPhy, packetsize, phy_abstract->dataRate);

            phy_abstract->stats.totalSignalDuration += packetDuration;

            PhyLinkCesWnwAdaptationSetPartialRateDuration(
                                             node,
                                             phyIndex,
                                             packetDuration);
        }else
        {
            ERROR_ReportError("InfoField INFO_TYPE_LinkCesWnwAdaptation does not exist !!! \n");
        }

    }
#endif
    duration = PhyAbstractGetFrameDuration(
            thisPhy, packetsize, PHY_GetTxDataRate(node, phyIndex));

    if (PHY_IsListeningToChannel(node, phyIndex, channelIndex))
    {
        PHY_StopListeningToChannel(node, phyIndex, channelIndex);
    }

    if ((thisPhy->antennaData->antennaModelType == ANTENNA_SWITCHED_BEAM)
       ||(thisPhy->antennaData->antennaModelType == ANTENNA_STEERABLE))
    {
        if (!sendDirectionally)
        {
            PROP_ReleaseSignal(
                    node,
                    packet,
                    phyIndex,
                    channelIndex,
                    phy_abstract->txPower_dBm,
                    duration,
                    delayUntilAirborne);
        }
        else
        {
            PROP_ReleaseSignal(
                    node,
                    packet,
                    phyIndex,
                    channelIndex,
                    (float)(phy_abstract->txPower_dBm -
                            phy_abstract->directionalAntennaGain_dB),
                    duration,
                    delayUntilAirborne);
        }
    }
    else
    {

        PROP_ReleaseSignal(
                    node,
                    packet,
                    phyIndex,
                    channelIndex,
                    phy_abstract->txPower_dBm,
                    duration,
                    delayUntilAirborne);
    }

    //GuiStart
    if (node->guiOption == TRUE)
    {
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

    phy_abstract->txEndTimer = endMsg;
    /* Keep track of phy statistics and battery computations */
    phy_abstract->stats.energyConsumed
            += duration * (BATTERY_TX_POWER_COEFFICIENT
            * NON_DB(phy_abstract->txPower_dBm)
            + BATTERY_TX_POWER_OFFSET
            - BATTERY_RX_POWER);

    if (phy_abstract->thisPhy->phyStats)
    {
        phy_abstract->thisPhy->stats->AddSignalTransmittedDataPoints(node, duration);
    }
}

void PhyAbstractStartTransmittingSignalDirectionally(
        Node* node,
        int phyIndex,
        Message* packet,
        BOOL useMacLayerSpecifiedDelay,
        clocktype initDelayUntilAirborne,
        double azimuthAngle)
{
    StartTransmittingSignal(
            node,
            phyIndex,
            packet,
            useMacLayerSpecifiedDelay,
            initDelayUntilAirborne,
            TRUE, azimuthAngle);
}

void PhyAbstractLockAntennaDirection(Node* node, int phyNum)
{
    ANTENNA_LockAntennaDirection(node, phyNum);
}

void PhyAbstractUnlockAntennaDirection(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract *)thisPhy->phyVar;

    ANTENNA_UnlockAntennaDirection(node, phyIndex);

    if ((phy_abstract->mode != PHY_RECEIVING) &&
         (phy_abstract->mode != PHY_TRANSMITTING))
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
                &(phy_abstract->interferencePower_mW));
    }//if//
}

void PhyAbstractSetSensingDirection(Node* node, int phyIndex,
                               double azimuth)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;
    int channelIndex;

    assert((phy_abstract->mode == PHY_IDLE) ||
            (phy_abstract->mode == PHY_SENSING));

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);
    ANTENNA_SetBestConfigurationForAzimuth(node, phyIndex, azimuth);

    PHY_SignalInterference(
            node,
            phyIndex,
            channelIndex,
            NULL,
            NULL,
            &(phy_abstract->interferencePower_mW));
}


BOOL PhyAbstractMediumIsIdleInDirection(Node* node, int phyIndex, double azimuth)
{
    //printf("+++medium is idle in direction %f\n", azimuth);
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;
    BOOL IsIdle;
    int channelIndex;
    double oldInterferencePower = phy_abstract->interferencePower_mW;

    assert((phy_abstract->mode == PHY_IDLE) ||
            (phy_abstract->mode == PHY_SENSING));

    if (ANTENNA_IsLocked(node, phyIndex))
    {
        return (phy_abstract->mode == PHY_IDLE);
    }

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);
    ANTENNA_SetBestConfigurationForAzimuth(node, phyIndex, azimuth);

    PHY_SignalInterference(node, phyIndex, channelIndex, NULL, NULL,
                           &(phy_abstract->interferencePower_mW));

    IsIdle = (!PhyAbstractCarrierSensing(
                node,
                phy_abstract));

    phy_abstract->interferencePower_mW = oldInterferencePower;
    ANTENNA_SetToDefaultMode(node, phyIndex);

    return IsIdle;
}


double PhyAbstractGetLastAngleOfArrival(Node* node, int phyIndex)
        {

    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;
    return phy_abstract->rxDOA.azimuth;

}

//
// Used by the MAC layer to start transmitting a packet.
//
void PhyAbstractStartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne)
{
    clocktype delayUntilAirborne = initDelayUntilAirborne;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;
    PhyAbstractStats* stats = &(phy_abstract->stats);
    int channelIndex;
    Message *endMsg;
    int packetsize = 0;
    clocktype duration;

    if (!packet->isPacked)
    {
        packetsize = MESSAGE_ReturnPacketSize(packet);
    }
    else
    {
        packetsize = MESSAGE_ReturnActualPacketSize(packet);
    }

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    if (thisPhy->antennaData->antennaModelType !=
            ANTENNA_OMNIDIRECTIONAL)
    {
        ANTENNA_SetToDefaultMode(node, phyIndex);
    }

    if (!useMacLayerSpecifiedDelay) {
        delayUntilAirborne = PHY_ABSTRACT_RX_TX_TURNAROUND_TIME;
    }//if//

    assert(phy_abstract->mode != PHY_TRANSMITTING);

    if (phy_abstract->mode == PHY_RECEIVING) {

        // if the phy was receiving a signal, now it has to drop the signal
        // in order to transmit, by calling unlock the signal
        PHY_NotificationOfPacketDrop(
            node,
            phyIndex,
            channelIndex,
            phy_abstract->rxMsg,
            "PHY Stop Rx for Tx",
            phy_abstract->rxMsgPower_mW,
            phy_abstract->interferencePower_mW,
            0.0);

        phy_abstract->interferencePower_mW += phy_abstract->rxMsgPower_mW;

        stats->numUpdates++;

        if (phy_abstract->thisPhy->phyStats)
        {
            phy_abstract->thisPhy->stats->AddSignalTerminatedDataPoints(
                                        node,
                                        thisPhy,
                                        phy_abstract->rxMsg,
                                        phy_abstract->rxChannelIndex,
                                        phy_abstract->txNodeId,
                                        phy_abstract->pathloss_dB,
                                        phy_abstract->rxTimeEvaluated,
                                        phy_abstract->interferencePower_mW,
                                        phy_abstract->rxMsgPower_mW);
        }
        PhyAbstractUnlockSignal(phy_abstract);
    }
    PhyAbstractChangeState(node, phyIndex, PHY_TRANSMITTING);

#ifdef ADDON_BOEINGFCS
    PhyAbstractReportStatusToMac(node, phyIndex, phy_abstract->mode);

    if (phy_abstract->linkAdaptationEnabled)
    {

        LinkAdaptationTxParameters* tmp;
        tmp = (LinkAdaptationTxParameters*)
              MESSAGE_ReturnInfo(packet, INFO_TYPE_LinkCesWnwAdaptation);
        if (tmp)
        {
            phy_abstract->dataRate = tmp->dataRate;
            phy_abstract->txPower_dBm = tmp->txPower * STEP_SIZE;

            clocktype packetDuration =
                PhyAbstractGetFrameDuration(
                    thisPhy, packetsize, phy_abstract->dataRate);

            phy_abstract->stats.totalSignalDuration += packetDuration;

           PhyLinkCesWnwAdaptationSetPartialRateDuration(
                                            node,
                                            phyIndex,
                                            packetDuration);
        }
        else
        {
            ERROR_ReportError("InfoField INFO_TYPE_LinkCesWnwAdaptation does not exist !!! \n");
        }
    }
#endif
    duration =
        PhyAbstractGetFrameDuration(
            thisPhy, packetsize, PHY_GetTxDataRate(node, phyIndex));

    if (PHY_IsListeningToChannel(node, phyIndex, channelIndex))
    {
        PHY_StopListeningToChannel(node, phyIndex, channelIndex);
    }

    PROP_ReleaseSignal(
        node,
        packet,
        phyIndex,
        channelIndex,
        phy_abstract->txPower_dBm,
        duration,
        delayUntilAirborne);

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

    phy_abstract->txEndTimer = endMsg;

    /* Keep track of phy statistics and battery computations */
    phy_abstract->stats.energyConsumed
        += duration * (BATTERY_TX_POWER_COEFFICIENT
                       * NON_DB(phy_abstract->txPower_dBm)
                       + BATTERY_TX_POWER_OFFSET
                       - BATTERY_RX_POWER);
    if (phy_abstract->thisPhy->phyStats)
    {
        phy_abstract->thisPhy->stats->AddSignalTransmittedDataPoints(node, duration);
        if (DYNAMIC_STATS)
        {
            GUI_SendUnsignedData(node->nodeId,
                 phy_abstract->stats.totalTxSignalsId,
                 (UInt32)phy_abstract->thisPhy->stats->
                    GetSignalsTransmitted().GetValue(node->getNodeTime()),
                 node->getNodeTime());
        }
    }

    if (DEBUG) {
        printf("DEBUG: Node %d transmitting signal\n",
               node->nodeId);
        fflush(stdout);
    }
    if (DYNAMIC_STATS) {
        GUI_SendRealData(node->nodeId,
                         phy_abstract->stats.energyConsumedId,
                         phy_abstract->stats.energyConsumed / 3600.0,
                         node->getNodeTime());
    }
}

// /**
// API                        :: PhyAbstractStartTransmittingSignal
// LAYER                      :: Physical
// PURPOSE                    :: Used by the MAC layer to start transmitting
//                               a packet.Accepts a parameter called
//                               duration which specifies transmission delay.
//                               Function is being overloaded
// PARAMETERS                 ::
// + node                      : Node *    : node pointer to node
// + phyIndex                  : int       : index
// + packet                    : Message*  : packet to be sent
// + duration                  : clocktype : packet transmission delay
// + useMacLayerSpecifiedDelay : BOOL      : use delay specified by MAC
// + initDelayUntilAirborne    : clocktype : delay until airborne
// RETURN                     :: void
// **/
void PhyAbstractStartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    clocktype duration,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne)
{
    clocktype delayUntilAirborne = initDelayUntilAirborne;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;
    PhyAbstractStats* stats = &(phy_abstract->stats);
    int channelIndex;
    Message *endMsg;

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    if (thisPhy->antennaData->antennaModelType !=
            ANTENNA_OMNIDIRECTIONAL)
    {
        ANTENNA_SetToDefaultMode(node, phyIndex);
    }

    if (!useMacLayerSpecifiedDelay) {
        delayUntilAirborne = PHY_ABSTRACT_RX_TX_TURNAROUND_TIME;
    }

    assert(phy_abstract->mode != PHY_TRANSMITTING);

    if (phy_abstract->mode == PHY_RECEIVING) {

        // if the phy was receiving a signal, now it has to drop the signal
        // in order to transmit, by calling unlock the signal
        PHY_NotificationOfPacketDrop(
            node,
            phyIndex,
            channelIndex,
            phy_abstract->rxMsg,
            "PHY Stop Rx for Tx",
            phy_abstract->rxMsgPower_mW,
            phy_abstract->interferencePower_mW,
            0.0);

        phy_abstract->interferencePower_mW += phy_abstract->rxMsgPower_mW;

        stats->numUpdates++;

        if (phy_abstract->thisPhy->phyStats)
        {
            phy_abstract->thisPhy->stats->AddSignalTerminatedDataPoints(
                                        node,
                                        thisPhy,
                                        phy_abstract->rxMsg,
                                        phy_abstract->rxChannelIndex,
                                        phy_abstract->txNodeId,
                                        phy_abstract->pathloss_dB,
                                        phy_abstract->rxTimeEvaluated,
                                        phy_abstract->interferencePower_mW,
                                        phy_abstract->rxMsgPower_mW);
        }
        PhyAbstractUnlockSignal(phy_abstract);
    }

    PhyAbstractChangeState(node, phyIndex, PHY_TRANSMITTING);

#ifdef ADDON_BOEINGFCS
    PhyAbstractReportStatusToMac(node, phyIndex, phy_abstract->mode);
#endif

    if (PHY_IsListeningToChannel(node, phyIndex, channelIndex))
    {
        PHY_StopListeningToChannel(node, phyIndex, channelIndex);
    }

#ifdef ADDON_BOEINGFCS
    //compute duration from the bit size
    if (phy_abstract->linkAdaptationEnabled)
    {
        LinkAdaptationTxParameters* tmp;
        tmp = (LinkAdaptationTxParameters*)
              MESSAGE_ReturnInfo(packet, INFO_TYPE_LinkCesWnwAdaptation);
        if (tmp)
        {
            phy_abstract->dataRate = tmp->dataRate;
            phy_abstract->txPower_dBm = tmp->txPower * STEP_SIZE;

            phy_abstract->stats.totalSignalDuration += duration;

            PhyLinkCesWnwAdaptationSetPartialRateDuration(
                                            node,
                                            phyIndex,
                                            duration);
        }else
        {
            ERROR_ReportError("InfoField INFO_TYPE_LinkCesWnwAdaptation does not exist !!! \n");
        }

    }
#endif

    PROP_ReleaseSignal(
        node,
        packet,
        phyIndex,
        channelIndex,
        phy_abstract->txPower_dBm,
        duration,
        delayUntilAirborne);

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

    /* Keep track of phy statistics and battery computations */
    phy_abstract->stats.energyConsumed
        += duration * (BATTERY_TX_POWER_COEFFICIENT
                       * NON_DB(phy_abstract->txPower_dBm)
                       + BATTERY_TX_POWER_OFFSET
                       - BATTERY_RX_POWER);
    if (phy_abstract->thisPhy->phyStats)
    {
        phy_abstract->thisPhy->stats->AddSignalTransmittedDataPoints(node, duration);
    }
}

//
// Used by the MAC layer to start transmitting a packet.
// Accepts message size in bits. Used for non-byte aligned messages
// e.g. Link 11 and Link 16 frames

void PhyAbstractStartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    int bitSize,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne)
{
    clocktype delayUntilAirborne = initDelayUntilAirborne;
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;
    PhyAbstractStats* stats = &(phy_abstract->stats);
    int channelIndex;
    Message *endMsg;
    clocktype duration;

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    if (thisPhy->antennaData->antennaModelType !=
            ANTENNA_OMNIDIRECTIONAL)
    {
        ANTENNA_SetToDefaultMode(node, phyIndex);
    }

    if (!useMacLayerSpecifiedDelay) {
        delayUntilAirborne = PHY_ABSTRACT_RX_TX_TURNAROUND_TIME;
    }

    assert(phy_abstract->mode != PHY_TRANSMITTING);

    if (phy_abstract->mode == PHY_RECEIVING) {

        // if the phy was receiving a signal, now it has to drop the signal
        // in order to transmit, by calling unlock the signal
        PHY_NotificationOfPacketDrop(
            node,
            phyIndex,
            channelIndex,
            phy_abstract->rxMsg,
            "PHY Stop Rx for Tx",
            phy_abstract->rxMsgPower_mW,
            phy_abstract->interferencePower_mW,
            0.0);

        phy_abstract->interferencePower_mW += phy_abstract->rxMsgPower_mW;

        stats->numUpdates++;

        if (phy_abstract->thisPhy->phyStats)
        {
            phy_abstract->thisPhy->stats->AddSignalTerminatedDataPoints(
                                        node,
                                        thisPhy,
                                        phy_abstract->rxMsg,
                                        phy_abstract->rxChannelIndex,
                                        phy_abstract->txNodeId,
                                        phy_abstract->pathloss_dB,
                                        phy_abstract->rxTimeEvaluated,
                                        phy_abstract->interferencePower_mW,
                                        phy_abstract->rxMsgPower_mW);
        }
        PhyAbstractUnlockSignal(phy_abstract);
    }
    PhyAbstractChangeState(node, phyIndex, PHY_TRANSMITTING);
#ifdef ADDON_BOEINGFCS
    PhyAbstractReportStatusToMac(node, phyIndex, phy_abstract->mode);
#endif
#ifdef ADDON_BOEINGFCS
    //compute duration from the bit size
    if (phy_abstract->linkAdaptationEnabled)
    {
        LinkAdaptationTxParameters* tmp;
        tmp = (LinkAdaptationTxParameters*)
              MESSAGE_ReturnInfo(packet, INFO_TYPE_LinkCesWnwAdaptation);
        if (tmp)
        {
            phy_abstract->dataRate = tmp->dataRate;
            phy_abstract->txPower_dBm = tmp->txPower * STEP_SIZE;

            clocktype packetDuration =
                PhyAbstractGetFrameDurationFromBits(
                    thisPhy, bitSize, phy_abstract->dataRate);

            phy_abstract->stats.totalSignalDuration += packetDuration;

            PhyLinkCesWnwAdaptationSetPartialRateDuration(
                                            node,
                                            phyIndex,
                                            packetDuration);
        }else
        {
            ERROR_ReportError("InfoField INFO_TYPE_LinkCesWnwAdaptation does not exist !!! \n");
        }

    }
#endif
    duration =
        PhyAbstractGetFrameDurationFromBits(
            thisPhy, bitSize, PHY_GetTxDataRate(node, phyIndex));

    if (PHY_IsListeningToChannel(node, phyIndex, channelIndex))
    {
        PHY_StopListeningToChannel(node, phyIndex, channelIndex);
    }

    PROP_ReleaseSignal(
        node,
        packet,
        phyIndex,
        channelIndex,
        phy_abstract->txPower_dBm,
        duration,
        delayUntilAirborne);

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

    /* Keep track of phy statistics and battery computations */
    phy_abstract->stats.energyConsumed
        += duration * (BATTERY_TX_POWER_COEFFICIENT
                       * NON_DB(phy_abstract->txPower_dBm)
                       + BATTERY_TX_POWER_OFFSET
                       - BATTERY_RX_POWER);
    if (phy_abstract->thisPhy->phyStats)
    {
        phy_abstract->thisPhy->stats->AddSignalTransmittedDataPoints(node, duration);
    }
}

clocktype PhyAbstractGetFrameDuration(PhyData* thisPhy, int size,
                                      Int64 dataRate) {
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;

    clocktype delay = size * 8 * SECOND / dataRate + phy_abstract->syncTime;

    return delay;
}

//Function to compute the duration of the frame, given the bit size of
//the frame used for non-byte aligned messages e.g. Link 11 and Link 16
clocktype PhyAbstractGetFrameDurationFromBits(PhyData* thisPhy,
                                              int bitSize,
                                              int dataRate)
{
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;

    clocktype delay = bitSize * SECOND / dataRate + phy_abstract->syncTime;

    return delay;
}

void PhyAbstractSetTransmitPower(PhyData *thisPhy, double newTxPower_mW) {
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;

    phy_abstract->txPower_dBm = (float)IN_DB(newTxPower_mW);
}


void PhyAbstractGetTransmitPower(PhyData *thisPhy, double *txPower_mW) {
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;

    *txPower_mW = NON_DB(phy_abstract->txPower_dBm);
}


Int64 PhyAbstractGetDataRate(PhyData *thisPhy) {
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;

    return phy_abstract->dataRate;
}

// determines whether a signal will be processed by phyAbstract or not.
// used for stats DB and connection manager.
BOOL PhyAbstractProcessSignal(Node* node,
                              int phyIndex,
                              PropRxInfo* propRxInfo,
                              double rxPower_dBm)
{
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;

    double rxPower_mW = NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
                               rxPower_dBm);

    if (rxPower_mW >= phy_abstract->rxThreshold_mW)
    {
        return TRUE;
    }

    return FALSE;

}

double PhyAbstractGetTxPower(Node* node, int phyIndex)
    {
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;

    if (ANTENNA_IsInOmnidirectionalMode(node, phyIndex))
    {
        return phy_abstract->txPower_dBm;
    }
    else
    {
        return (double)(phy_abstract->txPower_dBm -
                        phy_abstract->directionalAntennaGain_dB);
    }//if//
}


BOOL PhyAbstractMediumIsIdle(Node* node, int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;
    BOOL IsIdle;
    int channelIndex;
    double oldInterferencePower = phy_abstract->interferencePower_mW;

    assert((phy_abstract->mode == PHY_IDLE) ||
            (phy_abstract->mode == PHY_SENSING));

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    PHY_SignalInterference(node, phyIndex, channelIndex, NULL, NULL,
                           &(phy_abstract->interferencePower_mW));

    IsIdle = (!PhyAbstractCarrierSensing(
                node,
                phy_abstract));

    phy_abstract->interferencePower_mW = oldInterferencePower;

    return IsIdle;
}


void PhyAbstractChannelListeningSwitchNotification(
   Node* node,
   int phyIndex,
   int channelIndex,
   BOOL startListening)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*)thisPhy->phyVar;

    if (phy_abstract == NULL)
    {
        // not initialized yet, return;
        return;
    }

    if (startListening == TRUE)
    {
        if (phy_abstract->mode == PHY_TRANSMITTING)
        {
            PhyAbstractTerminateCurrentTransmission(node,phyIndex);
        }

#ifdef MILITARY_RADIOS_LIB
        MacData *mac = node->macData[thisPhy->macInterfaceIndex];
        if (mac->macProtocol == MAC_PROTOCOL_TADIL_LINK16)
        {
            // Don't calculate the Link16 signal interference (not useful)

            PhyAbstractChangeState(node,phyIndex, PHY_IDLE);
        }
        else
        {
          // Otherwise, unchanged
#endif // MILITARY_RADIOS_LIB
        PHY_SignalInterference(
         node,
         phyIndex,
         channelIndex,
         NULL,
         NULL,
         &(phy_abstract->interferencePower_mW));
#ifdef MILITARY_RADIOS_LIB
        }
#endif // MILITARY_RADIOS_LIB

        if (PhyAbstractCarrierSensing(
                node,
                phy_abstract) == TRUE)
        {
            PhyAbstractChangeState(node,phyIndex, PHY_SENSING);
        }
        else
        {
            PhyAbstractChangeState(node,phyIndex, PHY_IDLE);
        }

        if (phy_abstract->previousMode != phy_abstract->mode)
        {
            PhyAbstractReportStatusToMac(node, phyIndex, phy_abstract->mode);
        }
    }
}

void PhyAbstractInterferenceArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo,
    double sigPower_mW)
{

    PhyData *thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;
    PhyAbstractStats* stats = &(phy_abstract->stats);

    double interferencePower_mW =
        NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
               IN_DB(sigPower_mW));
    double sinr = -1.0;

    switch (phy_abstract->mode) {
        case PHY_RECEIVING: {
            if (!phy_abstract->rxMsgError) {

                phy_abstract->rxMsgError =
                      PhyAbstractCheckRxPacketError(
                          node,
                          phy_abstract,
                          FALSE,
                          0,
                          &sinr);
            }//if//

            phy_abstract->rxTimeEvaluated = node->getNodeTime();
            phy_abstract->interferencePower_mW += interferencePower_mW;
            stats->numUpdates++;

            break;
        }

        //
        // If the phy is idle or sensing,
        // check if the signal change its status
        //
        case PHY_IDLE:
        case PHY_SENSING:
        {
            PhyStatusType newMode;

            phy_abstract->interferencePower_mW += interferencePower_mW;
            stats->numUpdates++;

            if (PhyAbstractCarrierSensing(node,
                                          phy_abstract))

            {
               newMode = PHY_SENSING;
            } else {

               newMode = PHY_IDLE;
            }//if//

            if (newMode != phy_abstract->mode) {

                PhyAbstractChangeState(node,phyIndex, newMode);

                PhyAbstractReportStatusToMac(
                            node,
                            phyIndex,
                            newMode);
            }//if//

            break;
        }

        default:
            abort();

    }//switch (phy_abstract->mode)//
}


void PhyAbstractInterferenceEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo,
    double sigPower_mW)
{
    PhyData *thisPhy = node->phyData[phyIndex];
    PhyDataAbstract* phy_abstract = (PhyDataAbstract*) thisPhy->phyVar;
    double sinr;

    if (phy_abstract->mode == PHY_RECEIVING) {
        if (phy_abstract->rxMsgError == FALSE) {

                phy_abstract->rxMsgError =
                    PhyAbstractCheckRxPacketError(
                        node,
                        phy_abstract,
                        FALSE,
                        0.0,
                        &sinr);

            phy_abstract->rxTimeEvaluated = node->getNodeTime();
        }//if
    }//if//

    PhyStatusType newMode;

    double interferencePower_mW =
            NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
                   IN_DB(sigPower_mW));

    phy_abstract->interferencePower_mW -= interferencePower_mW;

    if (phy_abstract->interferencePower_mW < 0.0) {
        phy_abstract->interferencePower_mW = 0.0;
    }

    if ((phy_abstract->mode != PHY_RECEIVING) &&
        (phy_abstract->mode != PHY_TRANSMITTING)) {

        if (PhyAbstractCarrierSensing(
                        node,
                        phy_abstract) == TRUE)
        {
            newMode = PHY_SENSING;

        }
        else {

            newMode = PHY_IDLE;
        }//if//

        if (newMode != phy_abstract->mode) {

            PhyAbstractChangeState(node,phyIndex, newMode);

            PhyAbstractReportStatusToMac(
                   node,
                   phyIndex,
                   newMode);
       }//if//
    }//if//
}


