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

#ifndef PHY_GSM_H
#define PHY_GSM_H

/*
 * synchronization time for GSM PHY
 */

#define PHY_GSM_CAPTURE_THRESHOLD    1.0

#define PHY_GSM_RX_TX_TURNAROUND_TIME  0
//(5 * MICRO_SECOND)

#define PHY_GSM_PHY_DELAY PHY_GSM_RX_TX_TURNAROUND_TIME

// default MS_TX_PWR_MAX_CCH value if not specified in config file
#define PHY_GSM_DEFAULT_TX_POWER_dBm        15.0

#define PHY_GSM_DEFAULT_RX_SENSITIVITY_dBm -91.0

// RXLEV_ACCESS_MIN
#define PHY_GSM_DEFAULT_RX_THRESHOLD_dBm   -92.0

#define PHY_GSM_DEFAULT_DATA_RATE           270833

// 200 kHz
#define    PHY_GSM_CHANNEL_BANDWIDTH           200000

// The maximum TX power level an MS may use when accessing the
// system until otherwise commanded. (0-31 mapped value, based on
// GSM 05.05 Section 4.11, on BCCH D/L)
#define PHY_GSM_MS_TXPWR_MAX_CCH            31

// 'P' - Maximum RF output power of the MS (0 to 39dBm)
#define PHY_GSM_P_dBm                       39.0    // P = 2W in Class 4 MS

// Maximum downlink RF power permitted in the cell
#define PHY_GSM_MAX_BS_TX_POWER_dBm         33.0

// Maximum TX power a MS may use in the adjacent cell "n". Range (5, 39 dBm)
// for GSM. Set to a fixed value for all 'N' cells.
#define PHY_GSM_MS_TXPWR_MAX_N_dBm          20.0

// Maximum TX power a MS may use in the serving cell. Range (5, 39 dBm)
// for GSM
#define PHY_GSM_MS_TXPWR_MAX_dBm            20.0

// For info on cell selection and handover related parameters see GSM 05.08


//
// PHY power consumption rate in mW.
// Note:
// BATTERY_SLEEP_POWER is not used at this point for the following reasons:
// * Monitoring the channel is assumed to consume as much power as receiving
//   signals, thus the phy mode is either TX or RX in ad hoc networks.
// * Power management between APs and stations needs to be modeled in order to
//   simulate the effect of the sleep (or doze) mode for WLAN environment.
// Also, the power consumption for transmitting signals is calculated as
// (BATTERY_TX_POWER_COEFFICIENT * txPower_mW + BATTERY_TX_POWER_OFFSET).
//
// The values below are set based on these assumptions and the WaveLAN
// specifications.
//
// *** There is no guarantee that the assumptions above are correct! ***
//
#define BATTERY_SLEEP_POWER          (50.0 / SECOND)
#define BATTERY_RX_POWER             (900.0 / SECOND)
#define BATTERY_TX_POWER_OFFSET      BATTERY_RX_POWER
#define BATTERY_TX_POWER_COEFFICIENT (16.0 / SECOND)

/*
 * Structure for phy statistics variables
 */
typedef struct phy_Gsm_stats_str
{
    int         totalTxSignals;
    int         totalRxSignalsToMac;
    double      energyConsumed;
    clocktype   turnOnTime;
#ifdef CYBER_LIB
    int totalRxSignalsToMacDuringJam;
#endif
} PhyGsmStats;


/*
 * Structure for Phy.
 */
typedef struct struct_phy_Gsm_str
{
    PhyData*        thisPhy;

    float           txPower_dBm;
    double          rxSensitivity_mW;
    double          rxThreshold_mW;
    int             dataRate;

    float           rxLev;
    double          rxBer;


    Message*        rxMsg;
    double          rxMsgPower_mW;
    clocktype       rxTimeEvaluated;

    double          noisePower_mW;
    double          interferencePower_mW;

    PhyStatusType   mode;
    PhyStatusType   previousMode;

    PhyGsmStats     stats;
    BOOL            rxMsgError;
} PhyDataGsm;


static /*inline*/
PhyStatusType
PhyGsmGetStatus(
    Node* node,
    int phyNum)
{
    PhyDataGsm* phyGsm  = ( PhyDataGsm* )node->phyData[phyNum]->phyVar;
    return ( phyGsm->mode );
}

void PhyGsmInit(
    Node* node,
    const int phyIndex,
    const NodeInput* nodeInput);


void PhyGsmTransmissionEnd(
    Node* node,
    int phyIndex);


void PhyGsmFinalize(
    Node* node,
    const int phyIndex);


void PhyGsmSignalArrivalFromChannel(
    Node* node,
    int phyIndex,
    uddtChannelIndex channelIndex,
    PropRxInfo* propRxInfo);


void PhyGsmSignalEndFromChannel(
    Node* node,
    int phyIndex,
    uddtChannelIndex channelIndex,
    PropRxInfo* propRxInfo);


void PhyGsmSetTransmitPower(
    PhyData* thisPhy,
    double newTxPower_mW);


void PhyGsmGetTransmitPower(
    PhyData* thisPhy,
    double* txPower_mW);


clocktype PhyGsmGetTransmissionDuration(
    int size,
    int dataRate);

void PhyGsmStartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne);


int PhyGsmGetDataRate(
    PhyData* thisPhy);

float PhyGsmGetRxLevel(
    Node* node,
    int phyIndex);

double PhyGsmGetRxQuality(
    Node* node,
    int phyIndex);

void PhyGsmInterferenceArrivalFromChannel(
    Node* node,
    int phyIndex,
    uddtChannelIndex channelIndex,
    PropRxInfo* propRxInfo,
    double sigPower_mW);

void PhyGsmInterferenceEndFromChannel(
    Node* node,
    int phyIndex,
    uddtChannelIndex channelIndex,
    PropRxInfo* propRxInfo,
    double sigPower_mw);

#endif /* PHY_Gsm_H */

