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
 */

#ifndef PHY_ABSTRACT_H
#define PHY_ABSTRACT_H

#include "dynamic.h"
#include "stats_phy.h"

#define PHY_ABSTRACT_DEFAULT_TX_POWER_dBm        15.0
#define PHY_ABSTRACT_DEFAULT_RX_SENSITIVITY_dBm -91.0
#define PHY_ABSTRACT_DEFAULT_RX_THRESHOLD_dBm   -81.0
#define PHY_ABSTRACT_DEFAULT_HEADER_SIZE         192
#define PHY_ABSTRACT_DEFAULT_DATA_RATE           TYPES_ToInt64(2000000)
#define PHY_ABSTRACT_DEFAULT_CHANNEL_BANDWIDTH   2000000
#define PHY_ABSTRACT_DEFAULT_DIRECTIONAL_ANTENNA_GAIN 15.0

#define PHY_ABSTRACT_SYNCHRONIZATION_TIME \
        (PHY_ABSTRACT_DEFAULT_HEADER_SIZE * MICRO_SECOND)

#define PHY_ABSTRACT_RX_TX_TURNAROUND_TIME  (5 * MICRO_SECOND)

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

// NOTE FOR JNE:
//   In WNW, Phy-Abstract is the PHY model used to represent the SiS Layer.
//   The SiS is required to provide support for SNMP. This is done by the
//   addition of a "MIB Datastore" for SNMP.
//
#ifdef JNE_LIB
    class JtrsWnwSisMibDatastore; // forward declaration
#endif

/*
 * Structure for phy statistics variables
 */
typedef struct phy_abstract_stats_str {
    int totalTxSignals;
    int totalSignalsDetected;
    int totalSignalsLocked;
    int totalSignalsWithErrors;
    int totalSignalsInterferenceErrors;
    int totalRxSignalsToMac;
    D_Float64 energyConsumed;
    clocktype turnOnTime;
#ifdef CYBER_LIB
    int totalRxSignalsToMacDuringJam;
    int totalSignalsLockedDuringJam;
    int totalSignalsWithErrorsDuringJam;
#endif
    // dynamic stats IDs
    int totalTxSignalsId;
    int totalSignalsDetectedId;
    int totalSignalsLockedId;
    int totalSignalsWithErrorsId;
    int totalRxSignalsToMacId;
    int currentSINRId; // for debugging.
    int energyConsumedId;
    D_Float64 timebusyPercent;
    D_Float64 timebusy;
    Float64 timer;
    D_Float64 totalInterference_mW;
    D_Float64 totalInterference_dB;
    D_Float64 avgInterference_mW;
    D_Float64 avgInterference_dB;
    D_Int64 numUpdates;

#ifdef ADDON_BOEINGFCS
    clocktype totalSignalDuration;
    std::map <int, clocktype> totalSignalDurationMap;
#endif

#ifdef ADDON_DB
    double totalSignalPower;
    double totalPathLoss;
    clocktype txSendTime;
    clocktype totalDelay;
    int numberRxSignals;
#endif
    phy_abstract_stats_str(void);
} PhyAbstractStats;

/*
 * Structure for Phy.
 */
typedef struct struct_phy_abstract_str {
    BOOL nodeLinkLostStats;
    BOOL nodeLinkDelayStats;
    PhyData*  thisPhy;

    float     txPower_dBm;

    double    rxSensitivity_mW;
    double    rxThreshold_mW;
    D_Int64   dataRate;

#ifdef ADDON_BOEINGFCS
    //added to support multiple rates
    int       numDataRates;
    D_Int32   rateIndex;
#endif
    clocktype syncTime;
    D_Int32   bandwidth;

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
    double    noisePower_mW;
    double    interferencePower_mW;

    PhyStatusType mode;
    PhyStatusType previousMode;

    PhyAbstractStats  stats;
    BOOL radioBusyPercentStats; // CLEANUP: remove, phy_stats no longer used
    BOOL batteryPowerStats; // CLEANUP: remove, phy_stats no longer used
    BOOL interferenceStats; // CLEANUP: remove, phy_stats no longer used
#ifdef ADDON_BOEINGFCS
    BOOL linkAdaptationEnabled;
#endif

    double    directionalAntennaGain_dB;
    struct_phy_abstract_str(void);

#ifdef JNE_LIB
    // Datastore for SNMP MIBs
    //
    JtrsWnwSisMibDatastore* mibDatastore;
#endif

} PhyDataAbstract;


void PhyAbstractChangeState(
        Node* node,
        int phyIndex,
        PhyStatusType newStatus);

static /*inline*/
PhyStatusType PhyAbstractGetStatus(Node *node, int phyNum) {
    PhyDataAbstract* phy_abstract =
       (PhyDataAbstract*)node->phyData[phyNum]->phyVar;
    return (phy_abstract->mode);
}

void PhyAbstractInit(
    Node* node,
    const int phyIndex,
    const NodeInput *nodeInput);

void PhyAbstractSignalArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo);

void PhyAbstractSignalEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo);

void PhyAbstractFinalize(Node *node, const int phyIndex);

void PhyAbstractTerminateCurrentReceive(
    Node *node,
    int phyIndex,
    BOOL terminateOnlyOnReceiveError,
    BOOL *receiveError,
    clocktype *endSignalTime);

void PhyAbstractTerminateCurrentTransmission(Node* node, int phyIndex);

void PhyAbstractTransmissionEnd(Node *node, int phyIndex);

void PhyAbstractStartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne);

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
    clocktype initDelayUntilAirborne);

//overloaded with  specified bit size parameter
void PhyAbstractStartTransmittingSignal(
    Node* node,
    int phyIndex,
    Message* packet,
    int bitSize,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne);


void PhyAbstractChannelListeningSwitchNotification(
   Node* node,
   int phyIndex,
   int channelIndex,
   BOOL startListening);

clocktype PhyAbstractGetFrameDuration(PhyData* thisPhy, int size, Int64 dataRate);
//computes frame duration using specified bit size

clocktype PhyAbstractGetFrameDurationFromBits(PhyData* thisPhy,
                                              int bitSize,
                                              int dataRate);

void PhyAbstractSetTransmitPower(PhyData *thisPhy, double newTxPower_mW);

void PhyAbstractGetTransmitPower(PhyData *thisPhy, double *txPower_mW);

Int64 PhyAbstractGetDataRate(PhyData *thisPhy);

void PhyAbstractSetDataRate(Node* node, int phyIndex, Int64 dataRate);

void PhyAbstractRunTimeStat(Node* node, PhyData *thisPhy);

BOOL PhyAbstractMediumIsIdle(Node* node, int phyIndex);

void PhyAbstractCollisionWindowEnd(Node *node, int phyIndex);

double PhyAbstractGetLastAngleOfArrival(Node* node, int phyIndex);

void PhyAbstractStartTransmittingSignalDirectionally(
    Node* node,
    int phyIndex,
    Message* packet,
    BOOL useMacLayerSpecifiedDelay,
    clocktype initDelayUntilAirborne,
    double azimuthAngle);

void PhyAbstractLockAntennaDirection(Node* node, int phyNum);

void PhyAbstractUnlockAntennaDirection(Node* node, int phyNum);

BOOL PhyAbstractMediumIsIdleInDirection(Node* node, int phyIndex, double azimuth);

void PhyAbstractSetSensingDirection(Node* node, int phyIndex, double azimuth);


BOOL PhyAbstractProcessSignal(Node* node,
                              int phyIndex,
                              PropRxInfo* propRxInfo,
                              double rxPower_dBm);

double PhyAbstractGetTxPower(Node* node, int phyIndex);

void PhyAbstractChangeState(
        Node* node,
        int phyIndex,
        PhyStatusType newStatus);

BOOL PhyAbstractIsSignalForMe(Node* node,int phyIndex, Message* msg);

void PhyAbstractInterferenceArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo,
    double sigPower_mW);

void PhyAbstractInterferenceEndFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo,
    double sigPower_mW);

#endif /* PHY_ABSTRACT_H */
