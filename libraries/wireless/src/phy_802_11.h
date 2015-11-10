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


#ifndef PHY_802_11_H
#define PHY_802_11_H

#include "dynamic.h"
#include "stats_phy.h"
#include "phy_802_11n.h"
#include "mac_phy_802_11n.h"

/*
 * 802.11a parameters OFDM PHY
 */

// PLCP_SIZE's for 802.11a microseconds
#define PHY802_11a_SHORT_TRAINING_SIZE  8 // 10 short symbols
#define PHY802_11a_LONG_TRAINING_SIZE   8 // 2 OFDM symbols
#define PHY802_11a_SIGNAL_SIZE          4 // 1 OFDM symbol
#define PHY802_11a_OFDM_SYMBOL_DURATION 4 // 4 usec

#define PHY802_11a_CHANNEL_BANDWIDTH    20000000 //    20 MHz
#define PHY802_11b_CHANNEL_BANDWIDTH     2000000 // 22/11 MHz

#define PHY802_11a_SERVICE_BITS_SIZE    16 // 2bytes * 8 = 16bits.
#define PHY802_11a_TAIL_BITS_SIZE       6  // 6bits  / 8 = 3/4bytes.

#define PHY802_11a_PREAMBLE_AND_SIGNAL \
    (PHY802_11a_SHORT_TRAINING_SIZE + \
     PHY802_11a_LONG_TRAINING_SIZE + \
     PHY802_11a_SIGNAL_SIZE)

#define PHY802_11a_SYNCHRONIZATION_TIME \
    (PHY802_11a_PREAMBLE_AND_SIGNAL * MICRO_SECOND)

#define PHY802_11b_SYNCHRONIZATION_TIME  (192 * MICRO_SECOND)

#define PHY802_11a_RX_TX_TURNAROUND_TIME  (2 * MICRO_SECOND)
#define PHY802_11a_PHY_DELAY PHY802_11a_RX_TX_TURNAROUND_TIME

#define PHY802_11b_RX_TX_TURNAROUND_TIME  (5 * MICRO_SECOND)
#define PHY802_11b_PHY_DELAY PHY802_11b_RX_TX_TURNAROUND_TIME

#define PHY802_11_NUM_DATA_RATES 8

/*
 * Table 78  Rate-dependent parameters
 * Data rate|Mod Coding rate|Coded bits per sym|Data bits per OFDM sym (Ndbps)
 *      6   | BPSK     1/2  |   1              |  24
 *      9   | BPSK     3/4  |   1              |  36
 *      12  | QPSK     1/2  |   2              |  48
 *      18  | QPSK     3/4  |   2              |  72
 *      24  | 16-QAM   1/2  |   4              |  96
 *      36  | 16-QAM   3/4  |   4              |  144
 *      48  | 64-QAM   2/3  |   6              |  192
 *      54  | 64-QAM   3/4  |   6              |  216
 */
#define PHY802_11a__6M  0
#define PHY802_11a__9M  1
#define PHY802_11a_12M  2
#define PHY802_11a_18M  3
#define PHY802_11a_24M  4
#define PHY802_11a_36M  5
#define PHY802_11a_48M  6
#define PHY802_11a_54M  7

#define PHY802_11a_LOWEST_DATA_RATE_TYPE  PHY802_11a__6M
#define PHY802_11a_HIGHEST_DATA_RATE_TYPE PHY802_11a_54M
#define PHY802_11a_DATA_RATE_TYPE_FOR_BC  PHY802_11a__6M

#define PHY802_11a_NUM_DATA_RATES 8
#define PHY802_11a_NUM_BER_TABLES  8

#define PHY802_11a_DATA_RATE__6M   6000000
#define PHY802_11a_DATA_RATE__9M   9000000
#define PHY802_11a_DATA_RATE_12M  12000000
#define PHY802_11a_DATA_RATE_18M  18000000
#define PHY802_11a_DATA_RATE_24M  24000000
#define PHY802_11a_DATA_RATE_36M  36000000
#define PHY802_11a_DATA_RATE_48M  48000000
#define PHY802_11a_DATA_RATE_54M  54000000

#define PHY802_11a_NUM_DATA_BITS_PER_SYMBOL__6M   24
#define PHY802_11a_NUM_DATA_BITS_PER_SYMBOL__9M   36
#define PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_12M   48
#define PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_18M   72
#define PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_24M   96
#define PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_36M  144
#define PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_48M  192
#define PHY802_11a_NUM_DATA_BITS_PER_SYMBOL_54M  216

#define PHY802_11a_DEFAULT_TX_POWER__6M_dBm  20.0
#define PHY802_11a_DEFAULT_TX_POWER__9M_dBm  20.0
#define PHY802_11a_DEFAULT_TX_POWER_12M_dBm  19.0
#define PHY802_11a_DEFAULT_TX_POWER_18M_dBm  19.0
#define PHY802_11a_DEFAULT_TX_POWER_24M_dBm  18.0
#define PHY802_11a_DEFAULT_TX_POWER_36M_dBm  18.0
#define PHY802_11a_DEFAULT_TX_POWER_48M_dBm  16.0
#define PHY802_11a_DEFAULT_TX_POWER_54M_dBm  16.0

#define PHY802_11a_DEFAULT_RX_SENSITIVITY__6M_dBm  -85.0
#define PHY802_11a_DEFAULT_RX_SENSITIVITY__9M_dBm  -85.0
#define PHY802_11a_DEFAULT_RX_SENSITIVITY_12M_dBm  -83.0
#define PHY802_11a_DEFAULT_RX_SENSITIVITY_18M_dBm  -83.0
#define PHY802_11a_DEFAULT_RX_SENSITIVITY_24M_dBm  -78.0
#define PHY802_11a_DEFAULT_RX_SENSITIVITY_36M_dBm  -78.0
#define PHY802_11a_DEFAULT_RX_SENSITIVITY_48M_dBm  -69.0
#define PHY802_11a_DEFAULT_RX_SENSITIVITY_54M_dBm  -69.0

#define PHY802_11b__1M  0
#define PHY802_11b__2M  1
#define PHY802_11b__6M  2
#define PHY802_11b_11M  3

#define PHY802_11b_LOWEST_DATA_RATE_TYPE  PHY802_11b__1M
#define PHY802_11b_HIGHEST_DATA_RATE_TYPE PHY802_11b_11M
#define PHY802_11b_DATA_RATE_TYPE_FOR_BC  PHY802_11b__2M

#define PHY802_11b_NUM_DATA_RATES  4
#define PHY802_11b_NUM_BER_TABLES  4

#define PHY802_11b_DATA_RATE__1M   1000000
#define PHY802_11b_DATA_RATE__2M   2000000
#define PHY802_11b_DATA_RATE__6M   5500000
#define PHY802_11b_DATA_RATE_11M  11000000

#define PHY802_11b_NUM_DATA_BITS_PER_SYMBOL__1M   1
#define PHY802_11b_NUM_DATA_BITS_PER_SYMBOL__2M   2
#define PHY802_11b_NUM_DATA_BITS_PER_SYMBOL__6M   5.5
#define PHY802_11b_NUM_DATA_BITS_PER_SYMBOL_11M  11

#define PHY802_11b_DEFAULT_TX_POWER__1M_dBm  15.0
#define PHY802_11b_DEFAULT_TX_POWER__2M_dBm  15.0
#define PHY802_11b_DEFAULT_TX_POWER__6M_dBm  15.0
#define PHY802_11b_DEFAULT_TX_POWER_11M_dBm  15.0

#define PHY802_11b_DEFAULT_RX_SENSITIVITY__1M_dBm  -94.0
#define PHY802_11b_DEFAULT_RX_SENSITIVITY__2M_dBm  -91.0
#define PHY802_11b_DEFAULT_RX_SENSITIVITY__6M_dBm  -87.0
#define PHY802_11b_DEFAULT_RX_SENSITIVITY_11M_dBm  -83.0

// Control overhead for PHY 802.11a and 802.11b
#define PHY802_11a_CONTROL_OVERHEAD_SIZE \
    ((PHY802_11a_SYNCHRONIZATION_TIME \
                     * PHY802_11a_DATA_RATE__6M / SECOND) \
    + PHY802_11a_SERVICE_BITS_SIZE               \
    + PHY802_11a_TAIL_BITS_SIZE)

#define PHY802_11b_CONTROL_OVERHEAD_SIZE \
    (PHY802_11b_SYNCHRONIZATION_TIME / MICRO_SECOND)

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


struct Phy802_11PlcpHeader {
    unsigned char           txPhyModel;  //Transmiting Phy model
    int                     rate;        //data rate, only used for a/b/g
    Mode                    format;      //Transmit format
    ChBandwidth             chBwdth;     //Channel bandwidth
    UInt32                  length;      //PSDU length, which may not be the same
                                         //as packetSize as padding may be added.
    BOOL                    sounding;    //Whether a sounding packet
    BOOL                    containAMPDU;//Whether contains a A-MPDU
    GI                      gi;          //800ns or 400ns GI
    unsigned char           mcs;         //MCS index
    unsigned char           stbc;        //STBC, currently always 0 unless STBC is supported
    unsigned char           numEss;      //Number of extension spatial streams
};

/*
 * Structure for phy statistics variables
 */
typedef struct phy_802_11_stats_str {
    D_Float64 energyConsumed;
    D_Clocktype turnOnTime;

#ifdef CYBER_LIB
    D_Int32 totalRxSignalsToMacDuringJam;
    D_Int32 totalSignalsLockedDuringJam;
    D_Int32 totalSignalsWithErrorsDuringJam;
#endif
} Phy802_11Stats;

/*
 * Structure for Phy.
 */
struct PhyData802_11 {
    PhyData*  thisPhy;

    int       txDataRateTypeForBC;
    int       txDataRateType;
    D_Float32 txPower_dBm;
    float     txDefaultPower_dBm[PHY802_11_NUM_DATA_RATES];

    int       rxDataRateType;
    double    rxSensitivity_mW[PHY802_11_NUM_DATA_RATES];

    int       numDataRates;
    int       dataRate[PHY802_11_NUM_DATA_RATES];
    double    numDataBitsPerSymbol[PHY802_11_NUM_DATA_RATES];
    int       lowestDataRateType;
    int       highestDataRateType;

    double    directionalAntennaGain_dB;

    Message*  rxMsg;
    double    rxMsgPower_mW;
    clocktype rxTimeEvaluated;
    BOOL      rxMsgError;
    clocktype rxEndTime;
    Orientation rxDOA;
    NodeAddress txNodeId;
    Int32     rxChannelIndex;
    double    pathloss_dB;

    Message*  txEndTimer;
    D_Int32   channelBandwidth;
    clocktype rxTxTurnaroundTime;
    double    noisePower_mW;
    double    interferencePower_mW;

    PhyStatusType mode;
    PhyStatusType previousMode;

    Phy802_11n  *pDot11n;
    Phy802_11Stats  stats;
};


double
Phy802_11GetSignalStrength(Node *node,PhyData802_11* phy802_11);


static /*inline*/
PhyStatusType Phy802_11GetStatus(Node *node, int phyNum) {
    PhyData802_11* phy802_11 =
       (PhyData802_11*)node->phyData[phyNum]->phyVar;
    return (phy802_11->mode);
}

void Phy802_11Init(
    Node* node,
    const int phyIndex,
    const NodeInput *nodeInput,
    BOOL  initN = FALSE);

void Phy802_11ChannelListeningSwitchNotification(
   Node* node,
   int phyIndex,
   int channelIndex,
   BOOL startListening);


void Phy802_11TransmissionEnd(Node *node, int phyIndex);


void Phy802_11Finalize(Node *node, const int phyIndex);


void Phy802_11SignalArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo);


void Phy802_11SignalEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo);


void Phy802_11SetTransmitPower(PhyData *thisPhy, double newTxPower_mW);


void Phy802_11GetTransmitPower(PhyData *thisPhy, double *txPower_mW);

double Phy802_11GetAngleOfArrival(Node* node, int phyIndex);

clocktype Phy802_11GetTransmissionDelay(PhyData *thisPhy, int size);
clocktype Phy802_11GetReceptionDuration(PhyData *thisPhy, int size);

void Phy802_11StartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne);

void Phy802_11StartTransmittingSignalDirectionally(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne,
    double azimuthAngle);



int Phy802_11GetTxDataRate(PhyData *thisPhy);
int Phy802_11GetRxDataRate(PhyData *thisPhy);
int Phy802_11GetTxDataRateType(PhyData *thisPhy);
int Phy802_11GetRxDataRateType(PhyData *thisPhy);

void Phy802_11SetTxDataRateType(PhyData* thisPhy, int dataRateType);
void Phy802_11GetLowestTxDataRateType(PhyData* thisPhy, int* dataRateType);
void Phy802_11SetLowestTxDataRateType(PhyData* thisPhy);
void Phy802_11GetHighestTxDataRateType(PhyData* thisPhy, int* dataRateType);
void Phy802_11SetHighestTxDataRateType(PhyData* thisPhy);
void Phy802_11GetHighestTxDataRateTypeForBC(
    PhyData* thisPhy, int* dataRateType);
void Phy802_11SetHighestTxDataRateTypeForBC(
    PhyData* thisPhy);

void Phy802_11TerminateCurrentTransmission(Node* node, int phyIndex);
double Phy802_11GetLastAngleOfArrival(Node* node, int phyIndex);

void Phy802_11LockAntennaDirection(Node* node, int phyNum);

void Phy802_11UnlockAntennaDirection(Node* node, int phyNum);

void Phy802_11TerminateCurrentReceive(
    Node* node, int phyIndex, const BOOL terminateOnlyOnReceiveError,
    BOOL* frameError, clocktype* endSignalTime);

//
// The following functions are defined in phy_802_11_ber_*.c
//
void Phy802_11aSetBerTable(PhyData* thisPhy);
void Phy802_11bSetBerTable(PhyData* thisPhy);


void Phy802_11a_InitBerTable(void);
void Phy802_11b_InitBerTable(void);


clocktype Phy802_11GetFrameDuration(
    PhyData *thisPhy,
    int dataRateType,
    int size);

BOOL Phy802_11MediumIsIdle(
    Node* node,
    int phyIndex);

BOOL Phy802_11MediumIsIdleInDirection(
    Node* node,
    int phyIndex,
    double azimuth);

void Phy802_11SetSensingDirection(Node* node, int phyIndex, double azimuth);


// for phy connectivity table used in stats db and connectivity manager
BOOL Phy802_11ProcessSignal(Node* node,
                            int phyIndex,
                            PropRxInfo* propRxInfo,
                            double rxPower_dBm);

double Phy802_11GetTxPower(Node* node, int phyIndex);

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
                                std::string eventStr);
#endif

void Phy802_11InterferenceArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo,
    double sigPower);


void Phy802_11InterferenceEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo,
    double sigPower);

/*
 * 802_11n APIs
 */
/* Phy802_11nGetFrameDuration
 * Get the frame duration according to given transmission parameters
 */
clocktype Phy802_11nGetFrameDuration(PhyData *thisPhy,
                                     const MAC_PHY_TxRxVector& txVector);


/* Phy802_11nSetTxVector
 * Set the physical layer txVector parameter before sending out a PPDU
 */
void Phy802_11nSetTxVector(PhyData *thisPhy,
                           const MAC_PHY_TxRxVector& txVector);

/* Phy802_11nGetRxVector
 * Ret the physical layer rxVector parameters of currently locked signal
 */
void Phy802_11nGetRxVectorOfLockedSignal(PhyData *thisPhy,
                                         MAC_PHY_TxRxVector& rxVector);

/* Phy802_11nGetNumAtnaElems
 * Get the number of antenna elements of the PHY transeiver
 */
int Phy802_11nGetNumAtnaElems(PhyData *thisPhy);

/* Phy802_11nGetAtnaElemSpace
 * Get inter-antenna-element space (in meters) of the PHY transeiver
 */
double Phy802_11nGetAtnaElemSpace(PhyData *thisPhy);

/* Phy802_11nShortGiCapable()
 * Return whether the short GI is supported by PHY
 */
BOOL Phy802_11nShortGiCapable(PhyData *thisPhy);

/* Phy802_11nStbcCapable()
 * Return whether STBC is supported by PHY
 */
BOOL Phy802_11nStbcCapable(PhyData *thisPhy);

/* PHY802_11nGetOpertionChBwdth()
 * Get the current operation channel bandwidth of PHY
 */
ChBandwidth Phy802_11nGetOperationChBwdth(PhyData* thisPhy);

/* PHY802_11nGetOpertionChBwdth()
 * Get the current operation channel bandwidth of PHY
 */
void Phy802_11nSetOperationChBwdth(PhyData *thisPhy,
                                   ChBandwidth chBwdth);

void Phy802_11nGetTxVector(PhyData* thisPhy, MAC_PHY_TxRxVector& txVector);

void Phy802_11nGetRxVector(PhyData *thisPhy,
                           MAC_PHY_TxRxVector& rxVector);

void Phy802_11nGetTxVectorForBC(PhyData* thisPhy, MAC_PHY_TxRxVector& txVector);

void Phy802_11nGetTxVectorForHighestDataRate(PhyData* thisPhy, MAC_PHY_TxRxVector& txVector);

void Phy802_11nGetTxVectorForLowestDataRate(PhyData* thisPhy, MAC_PHY_TxRxVector& txVector);

double
Phy802_11nGetSignalStrength(Node *node,PhyData802_11* phy802_11);


#endif /* PHY_802_11_H */
