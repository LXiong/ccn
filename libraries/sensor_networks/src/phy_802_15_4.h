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

#ifndef PHY_802_15_4_H
#define PHY_802_15_4_H

// #include "battery_model.h"

#define PHY802_15_4_DEFAULT_TX_POWER_dBm                0.0

#define PHY802_15_4_DEFAULT_MODULATION               O_QPSK

#define PHY802_15_4_800MHZ_DEFAULT_CHANNEL_BANDWIDTH       600000  // 800 MHz
#define PHY802_15_4_DEFAULT_CHANNEL_BANDWIDTH             2000000  // 2400
                                                // and 900 MHz frequency bands

#define PHY802_15_4_BPSK_PREAMBLE_LENGTH               32
#define PHY802_15_4_800MHZ_ASK_PREAMBLE_LENGTH          2
#define PHY802_15_4_900MHZ_ASK_PREAMBLE_LENGTH          6
#define PHY802_15_4_O_QPSK_PREAMBLE_LENGTH              8

#define PHY802_15_4_BPSK_SFD_LENGTH                     8  // in symbols
#define PHY802_15_4_ASK_SFD_LENGTH                      1  // in symbols
#define PHY802_15_4_O_QPSK_SFD_LENGTH                   2  // in symbols

#define PHY802_15_4_PHR_LENGTH                          8  // in bits

#define PHY802_15_4_BPSK_BITS_PER_SYMBOL                1  // 800 and 900 MHz
#define PHY802_15_4_800MHZ_ASK_BITS_PER_SYMBOL         20
#define PHY802_15_4_900MHZ_ASK_BITS_PER_SYMBOL          5
#define PHY802_15_4_O_QPSK_BITS_PER_SYMBOL              4

// threshold in linear value
#define PHY802_15_4_BPSK_THRESHOLD                    1.2  // 800 and 900 MHz
                                                           // frequency bands
#define PHY802_15_4_800MHZ_ASK_THRESHOLD              2.2
#define PHY802_15_4_900MHZ_ASK_THRESHOLD              0.7
#define PHY802_15_4_O_QPSK_THRESHOLD                  3.1  // 800 and 900 MHz
                                                           // frequency bands
#define PHY802_15_4_2400MHZ_O_QPSK_THRESHOLD          1.6

#define PHY802_15_4_BPSK_THRESHOLD                    1.2  // 800 and 900 MHz
                                                           // frequency bands
#define PHY802_15_4_800MHZ_ASK_THRESHOLD              2.2
#define PHY802_15_4_900MHZ_ASK_THRESHOLD              0.7
#define PHY802_15_4_O_QPSK_THRESHOLD                  3.1  // 800 and 900 MHz
                                                           // frequency bands
#define PHY802_15_4_2400MHZ_O_QPSK_THRESHOLD          1.6

#define PHY802_15_4_DEFAULT_CCA_MODE        CARRIER_SENSE

// 10dB above the speicifed receiver sensitivity
#define PHY802_15_4_BPSK_ED_THRESHOLD                 -82  // 800 and 900 MHz
                                                           // frequency bands
#define PHY802_15_4_ED_THRESHOLD                      -75  //


#define PHY802_15_4_DEFAULT_BER_CONSTANT              0.5

#define PHY802_15_4_800MHZ_O_QPSK_SYMBOL_RATE       25000 // per symbol 4 bits
#define PHY802_15_4_O_QPSK_SYMBOL_RATE              62500 // 900 and 2400 MHz
                                                         // per symbol 4 bits

#define PHY802_15_4_O_QPSK_SPREAD_FACTOR               16  // 800 and 900
                                                           // frequency band
#define PHY802_15_4_2400MHZ_O_QPSK_SPREAD_FACTOR       32

#define PHY802_15_4_900MHZ_ASK_SYMBOL_RATE          50000  // per symbol 5
                                                           // bits
#define PHY802_15_4_800MHZ_ASK_SYMBOL_RATE          12500  // per symbol
                                                           // 20 bits
#define PHY802_15_4_ASK_SPREAD_FACTOR                  32  // 800 and 900
                                                           // frequency bands


#define PHY802_15_4_800MHZ_BPSK_SYMBOL_RATE         20000
#define PHY802_15_4_900MHZ_BPSK_SYMBOL_RATE         40000
#define PHY802_15_4_BPSK_SPREAD_FACTOR                 15  // 800 and 900
                                                           // frequency bands


#define PHY802_15_4_BER_CONSTANT                         8.0/240.0
#define PHY802_15_4_BER_UPPER_BOUND_SUMMATION            17
#define PHY802_15_4_DEFAULT_EXPONENT_FACTOR              10.0  // 800 AND
                                                               // 900 MHz
#define PHY802_15_4_2400MHZ_DEFAULT_EXPONENT_FACTOR      20.0  // 2400 MHz

#define PHY802_15_4_RX_TX_TURNAROUND_TIME_IN_SYMBOL_PERIODS  12 // 12 symbol
                                                                // duration

#define PHY802_15_4_800MHZ_RANGE_START 868000000U
#define PHY802_15_4_800MHZ_RANGE_END 868600000U
#define PHY802_15_4_900MHZ_RANGE_START 902000000U
#define PHY802_15_4_900MHZ_RANGE_END 928000000U
#define PHY802_15_4_2400MHZ_RANGE_START 2400000000U
#define PHY802_15_4_2400MHZ_RANGE_END 2483500000U


//
// PHY power consumption rate in mW.

// Note:
// The values below are set based on the reference "Performance Analysis
// of IEEE 802.15.4 and ZigBee for Large-Scale Wireless Sensor Network
// Applications"
//
// *** There is no guarantee that these values are correct! ***
//

#define BATTERY_SLEEP__POWER          (1.5 / SECOND)
#define BATTERY_RX__POWER             (56.5 / SECOND)
#define BATTERY_TX_POWER__OFFSET      (48.0 / SECOND)
#define BATTERY_TX_POWER__COEFFICIENT (12.6 / SECOND)

enum Modulation
{
    ASK,      // amplitude shift keying
    BPSK,     // binary phase shift keying
    O_QPSK    // offset quadrature phase shift keying
};

enum PLMEsetTrxState
{
    RX_ON,
    TX_ON,
    TRX_OFF,
    FORCE_TRX_OFF
};

enum phyCCAmode
{
    ENERGY_ABOVE_THRESHOLD,
    CARRIER_SENSE,
    CARRIER_SENSE_WITH_ENERGY_ABOVE_THRESHOLD
};

// PHY PIB attributes (Table 19)
enum PPIBAenum
{
    phyCurrentChannel = 0x00,
    phyChannelsSupported,
    phyTransmitPower,
    phyCCAMode
};

struct PHY_PIB
{
    Int32        phyCurrentChannel;
    Int32        phyChannelsSupported;
    double     phyTransmitPower;
    phyCCAmode phyCCAMode;
};

/*
 * Structure for phy statistics variables
 */
typedef struct phy802_15_4_stats_str {
    Int32 totalTxSignals;
    Int32 totalSignalsDetected;
    Int32 totalSignalsLocked;
    Int32 totalSignalsWithErrors;
    Int32 totalRxSignalsToMac;
#ifdef CYBER_LIB
    Int32 totalRxSignalsToMacDuringJam;
    Int32 totalSignalsLockedDuringJam;
    Int32 totalSignalsWithErrorsDuringJam;
#endif
    double energyConsumed;
    clocktype turnOnTime;
    clocktype turnOnPeriod;
    Int32 totalTxSignalsId;
    Int32 totalSignalsDetectedId;
    Int32 totalSignalsLockedId;
    Int32 totalSignalsWithErrorsId;
    Int32 totalRxSignalsToMacId;
    Int32 currentSINRId; // for debugging.
    Int32 energyConsumedId;
} Phy802_15_4Stats;

/*
 * Structure for Phy.
 */
typedef struct struct_phy802_15_4_str {
    PhyData*  thisPhy;

    float     txPower_dBm;
    double    linkQuality;
    double    rxSensitivity_mW;
    double    EDthreshold_mW;
    Int32       dataRate;
    double    channelBandwidth;

    // PowerCosts*       powerConsmpTable;// added for energy modeling

    Message*  rxMsg;
    double    rxMsgPower_mW;
    clocktype rxTimeEvaluated;
    BOOL      rxMsgError;
    clocktype rxEndTime;
    clocktype txOnPeriod;
    clocktype rxOnPeriod;
    clocktype turnOffPeriod;
    clocktype activationTime;
    clocktype deactivationTime;
    double    noisePower_mW;
    double    interferencePower_mW;

    PhyStatusType mode;
    PhyStatusType previousMode;
    Phy802_15_4Stats stats;

    phyCCAmode    CCAmode;
    Modulation modulation;
    clocktype symbolPeriod;
    clocktype preambleHeaderPeriod;
    clocktype RxTxTurnAroundTime;
    Int32  spreadFactor;

    Message* phyTimerEndMessage;
} PhyData802_15_4;

// **
// FUNCTION   :: Phy802_15_4GetStatus
// LAYER      :: PHY
// PURPOSE    :: Obtain the PHY status
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyNum    : Int32              : Index of the PHY
// RETURN     :: PhyStatusType    : PHY status
// **/
static
PhyStatusType Phy802_15_4GetStatus(Node* node, Int32 phyNum)
{
    PhyData802_15_4* phy802_15_4
        = (PhyData802_15_4*)node->phyData[phyNum]->phyVar;
    return (phy802_15_4->mode);
}

// **
// FUNCTION   :: Phy802_15_4Init
// LAYER      :: PHY
// PURPOSE    :: Initialize configurable parameters
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const Int32        : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to node input
// RETURN     :: void             : NULL
// **/
void Phy802_15_4Init(Node* node,
                     const Int32 phyIndex,
                     const NodeInput *nodeInput);
// /**
// FUNCTION   :: Phy802_15_4SignalArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the channel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : Int32   : Index of the PHY
// + channelIndex : Int32   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void Phy802_15_4SignalArrivalFromChannel(Node* node,
                                         Int32 phyIndex,
                                         Int32 channelIndex,
                                         PropRxInfo *propRxInfo);

// /**
// FUNCTION   :: Phy802_15_4SignalEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a channel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : Int32   : Index of the PHY
// + channelIndex : Int32   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/

void Phy802_15_4SignalEndFromChannel(Node* node,
                                     Int32 phyIndex,
                                     Int32 channelIndex,
                                     PropRxInfo *propRxInfo);
// /**
// FUNCTION   :: Phy802_15_4Finalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the 802.15.4 PHY, print out statistics
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : const Int32 : Index of the PHY
// RETURN     :: void      : NULL
// **/
void Phy802_15_4Finalize(Node* node, const Int32 phyIndex);

// /**
// FUNCTION   :: Phy802_15_4TerminateCurrentReceive
// LAYER      :: PHY
// PURPOSE    :: Terminate all signals current under receiving.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : Int32          : Index of the PHY
// + terminateOnlyOnReceiveError : const BOOL : Only terminate if in error
// + frameError   : BOOL*        : For returning if frame is in error
// + endSignalTime: clocktype*   : For returning expected signal end time
// RETURN     :: void : NULL
// **/
void Phy802_15_4TerminateCurrentReceive(Node* node,
                                        Int32 phyIndex,
                                        BOOL terminateOnlyOnReceiveError,
                                        BOOL* receiveError,
                                        clocktype *endSignalTime);
// **
// FUNCTION   :: Phy802_15_4TransmissionEnd
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : Int32   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void Phy802_15_4TransmissionEnd(Node* node,
                                Int32 phyIndex);

// **
// FUNCTION   :: Phy802_15_4StartTransmittingSignal
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : Int32   : Index of the PHY
// + packet : Message* : Frame to be transmitted
// + useMacLayerSpecifiedDelay : Use MAC specified delay or calculate it
// + initDelayUntilAirborne    : clocktype : The MAC specified delay
// RETURN     :: void : NULL
// **/
void Phy802_15_4StartTransmittingSignal(Node* node,
                                        Int32 phyIndex,
                                        Message* packet,
                                        BOOL useMacLayerSpecifiedDelay,
                                        clocktype initDelayUntilAirborne);

// /**
// API                        :: Phy802_15_4StartTransmittingSignal
// LAYER                      :: Physical
// PURPOSE                    :: Used by the MAC layer to start transmitting
//                               a packet.Accepts a parameter called
//                               duration which specifies transmission delay.
//                               Function is being overloaded
// PARAMETERS                 ::
// + node                      : Node *    : node pointer to node
// + phyIndex                  : Int32       : index
// + packet                    : Message*  : packet to be sent
// + duration                  : clocktype : packet transmission delay
// + useMacLayerSpecifiedDelay : BOOL      : use delay specified by MAC
// + initDelayUntilAirborne    : clocktype : delay until airborne
// RETURN                     :: void
// **/
void Phy802_15_4StartTransmittingSignal(Node* node,
                                        Int32 phyIndex,
                                        Message* packet,
                                        clocktype duration,
                                        BOOL useMacLayerSpecifiedDelay,
                                        clocktype initDelayUntilAirborne);

// /**
// FUNCTION   :: Phy802_15_4StartTransmittingSignal
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node                      : Node*     : Pointer to node.
// + phyIndex                  : Int32       : Index of the PHY
// + packet                    : Message*  : Frame to be transmitted
// + bitSize                   : Int32       : Packet size in bits
// + useMacLayerSpecifiedDelay : Use MAC specified delay or calculate it
// + initDelayUntilAirborne    : clocktype : The MAC specified delay
// RETURN     :: void : NULL
// **/
void Phy802_15_4StartTransmittingSignal(Node* node,
                                        Int32 phyIndex,
                                        Message* packet,
                                        Int32 bitSize,
                                        BOOL useMacLayerSpecifiedDelay,
                                        clocktype initDelayUntilAirborne);

// /**
// FUNCTION   :: Phy802_15_4GetFrameDuration
// LAYER      :: PHY
// PURPOSE    :: Obtain the duration of frame
// PARAMETERS ::
// + thisPhy   : PhyData*  : Pointer to PhyData
// + size      : Int32       : Packet size in Bytes
// + dataRate  : Int32       : Data rate in bits/s
// RETURN     :: clocktype : Duration of the frame
// **/
clocktype Phy802_15_4GetFrameDuration(PhyData* thisPhy,
                                      Int32 size,
                                      Int32 dataRate);

// /**
// FUNCTION   :: Phy802_15_4GetFrameDurationFromBits
// LAYER      :: PHY
// PURPOSE    :: Obtain the duration of frame
// PARAMETERS ::
// + thisPhy   : PhyData*  : Pointer to PhyData
// + bitSize   : Int32       : Packet size in Bits
// + dataRate  : Int32       : Data rate in bits/s
// RETURN     :: clocktype : Duration of the frame
// **/

// computes frame duration using specified bit size

clocktype Phy802_15_4GetFrameDurationFromBits(PhyData* thisPhy,
                                              Int32 bitSize,
                                              Int32 dataRate);

// /**
// FUNCTION   :: Phy802_15_4SetTransmitPower
// LAYER      :: PHY
// PURPOSE    :: Set the transmit power of the PHY
// PARAMETERS ::
// + thisPhy   : PhyData*    : Pointer to PHY data
// + newTxPower_mW : double : Transmit power in mW
// RETURN     ::  void   : NULL
// **/
void Phy802_15_4SetTransmitPower(
    PhyData* thisPhy,
    double newTxPower_mW);

// /**
// FUNCTION   :: Phy802_15_4GetTransmitPower
// LAYER      :: PHY
// PURPOSE    :: Retrieve the transmit power of the PHY in mW
// PARAMETERS ::
// + thisPhy   : PhyData*    : Pointer to PHY data
// + txPower_mW : double* : For returning the transmit power
// RETURN     ::  void    : NULL
// **/
void Phy802_15_4GetTransmitPower(PhyData* thisPhy, double* txPower_mW);

Int32 Phy802_15_4GetDataRate(PhyData* thisPhy);

Int32 Phy802_15_4GetSymbolRate(PhyData* thisPhy);

Int32 Phy802_15_4GetPhyHeaderDurationInSymbols(PhyData* thisPhy);

void Phy802_15_4SetHeaderSize(PhyData* thisPhy, Int32 headerSize);

void Phy802_15_4SetDataRate(Node* node,
                            Int32 phyIndex,
                            Int32 dataRate);

void Phy802_15_4RunTimeStat(Node* node, PhyData* thisPhy);

BOOL Phy802_15_4MediumIsIdle(Node* node,Int32 phyIndex);

void Phy802_15_4GetLinkQuality(Node* node,
                               Int32 phyIndex,
                               double* sinr,
                               double* signalPower_mW);

void Phy802_15_4TerminateCurrentTransmission(Node* node, Int32 phyIndex);

Int32 Phy802_15_4getChannelNumber(Node* node, Int32 phyIndex);

void Phy802_15_4PlmeSetTRX_StateRequest(Node* node,
                                        Int32 phyIndex,
                                        PLMEsetTrxState state);

void Phy802_15_4PLMECCArequest(Node* node,
                               Int32 phyIndex,
                               BOOL* channelIsFree );

void Phy802_15_4GetNodeStatus(Node* node,
                              Int32 phyIndex,
                              PhyStatusType status);

void Phy802_15_4DeactivationNode(Node* node, Int32 phyIndex);

void Phy802_15_4ActivationNode(Node* node, Int32 phyIndex);

void Phy802_15_4InterferenceArrivalFromChannel(
    Node* node,
    int phyIndex,
    int channelIndex,
    PropRxInfo *propRxInfo,
    double inbandPower_mW);

void Phy802_15_4InterferenceEndFromChannel(
        Node* node,
        int phyIndex,
        int channelIndex,
        PropRxInfo *propRxInfo,
        double inbandPower_mW);

#endif /* PHY_802_15_4_H */
