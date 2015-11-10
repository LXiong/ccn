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

#ifndef PHY_DOT16_H
#define PHY_DOT16_H

#include <list>
#include "dynamic.h"
#include "mac_dot16.h"

#define PHY802_16_CHANNEL_BANDWIDTH    20000000 //    20 MHz

#define PHY802_16_RX_TX_TURNAROUND_TIME  (2 * MICRO_SECOND)

#define PHY802_16_PHY_DELAY PHY802_16_RX_TX_TURNAROUND_TIME

#define PHY802_16_NUM_DATA_RATES 8

#define PHY802_16_SAMPING_FACTOR  (8.0/7.0)
#define PHY802_16_BANDWIDTH_FACTOR   8000.0

#define MAX_NUM_BURST_PROFILES      256

#define PHY802_16_QPSK_R_1_2   0
#define PHY802_16_QPSK_R_3_4   1
#define PHY802_16_16QAM_R_1_2  2
#define PHY802_16_16QAM_R_3_4  3
#define PHY802_16_64QAM_R_1_2  4
#define PHY802_16_64QAM_R_2_3  5
#define PHY802_16_64QAM_R_3_4  6
#define PHY802_16_BPSK         7

#define PHY802_16_DEFAULT_TX_POWER_dBm  20.0
/* Bug fix start for 5397*/
#define PHY802_16_DEFAULT_TX_MAX_POWER_dBm  50.0
/* Bug fix end for 5397*/
#define PHY802_16_DEFAULT_MODULATION_TYPE  QPSK
#define PHY802_16_DEFAULT_CODING_RATE  encodingRate_1_2
#define PHY802_16_DEFAULT_FFT_SIZE  FFT_2048
#define PHY802_16_DEFAULT_MODULATION_ENCODING  PHY802_16_QPSK_R_1_2
#define PHY802_16_CYCPREFIX          8.0
#define MAX_NUM_SUBCHANNELS          70

//Sensitivity for channel with 1 MHz bandwidth

#define PHY802_16_DEFAULT_SENSITIVITY_QPSK_R_1_2    -97.0
#define PHY802_16_DEFAULT_SENSITIVITY_QPSK_R_3_4    -94.0
#define PHY802_16_DEFAULT_SENSITIVITY_16QAM_R_1_2   -91.5
#define PHY802_16_DEFAULT_SENSITIVITY_16QAM_R_3_4   -88.0
#define PHY802_16_DEFAULT_SENSITIVITY_64QAM_R_1_2   -86.0
#define PHY802_16_DEFAULT_SENSITIVITY_64QAM_R_2_3   -84.0
#define PHY802_16_DEFAULT_SENSITIVITY_64QAM_R_3_4   -82.0
#define PHY802_16_DEFAULT_SENSITIVITY_BPSK          -96.0
#define PHY802_16_DEFAULT_CDMA_SPREADING_FACTOR     114.0

#define PHY802_16_SENSITIVITY_ADJUSTMENT_FACTOR      60.0
#define PHY802_16_NOISE_FACTOR_REFERENCE             11.0    //in dB    see P.627
#define PHY802_16_IMPLEMENTATION_LOSS                 0.0     //in dB
#define PHY802_16_CDMA_RANGING_THRESHOLD             11.0
#define PHY802_16_CDMA_RANGING_OCCUPIED_SUBCHANNELS   6.0

//
// PHY power consumption rate in mW.
// Note:
// BATTERY_SLEEP_POWER is not used at this point for the following reasons:
// * Monitoring the channel is assumed to consume as much power as receiving
//   signals, thus the phy mode is either TX or RX in ad hoc networks.
// * Power management between APs and stations needs to be modeled in order
// to simulate the effect of the sleep (or doze) mode for WLAN environment.
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

// there is no coding and pilot subcarrier in CDMA ranging
#define PHY802_16_CDMA_RANGING_PAYLOAD_PER_SUBCHANNEL_BPSK          24.0

//Useful data payload for a subchannel in bits
#define PHY802_16_DOWNLINK_DATA_PAYLOAD_PER_SUBCHANNEL_QPSK_R_1_2    24.0
#define PHY802_16_DOWNLINK_DATA_PAYLOAD_PER_SUBCHANNEL_QPSK_R_3_4    36.0
#define PHY802_16_DOWNLINK_DATA_PAYLOAD_PER_SUBCHANNEL_16QAM_R_1_2   48.0
#define PHY802_16_DOWNLINK_DATA_PAYLOAD_PER_SUBCHANNEL_16QAM_R_3_4   72.0
#define PHY802_16_DOWNLINK_DATA_PAYLOAD_PER_SUBCHANNEL_64QAM_R_1_2   72.0
#define PHY802_16_DOWNLINK_DATA_PAYLOAD_PER_SUBCHANNEL_64QAM_R_2_3   96.0
#define PHY802_16_DOWNLINK_DATA_PAYLOAD_PER_SUBCHANNEL_64QAM_R_3_4  108.0

#define PHY802_16_DOWNLINK_NUMBER_SUBCHANNEL_FFT_2048   60
#define PHY802_16_DOWNLINK_NUMBER_SUBCHANNEL_FFT_1024   30
#define PHY802_16_DOWNLINK_NUMBER_SUBCHANNEL_FFT_512    15
#define PHY802_16_DOWNLINK_NUMBER_SUBCHANNEL_FFT_128     3


#define PHY802_16_UPLINK_DATA_PAYLOAD_PER_SUBCHANNEL_QPSK_R_1_2    16.0
#define PHY802_16_UPLINK_DATA_PAYLOAD_PER_SUBCHANNEL_QPSK_R_3_4    24.0
#define PHY802_16_UPLINK_DATA_PAYLOAD_PER_SUBCHANNEL_16QAM_R_1_2   32.0
#define PHY802_16_UPLINK_DATA_PAYLOAD_PER_SUBCHANNEL_16QAM_R_3_4   48.0
#define PHY802_16_UPLINK_DATA_PAYLOAD_PER_SUBCHANNEL_64QAM_R_1_2   48.0
#define PHY802_16_UPLINK_DATA_PAYLOAD_PER_SUBCHANNEL_64QAM_R_2_3   64.0
#define PHY802_16_UPLINK_DATA_PAYLOAD_PER_SUBCHANNEL_64QAM_R_3_4   72.0

#define PHY802_16_UPLINK_NUMBER_SUBCHANNEL_FFT_2048     70
#define PHY802_16_UPLINK_NUMBER_SUBCHANNEL_FFT_1024     35
#define PHY802_16_UPLINK_NUMBER_SUBCHANNEL_FFT_512      17
#define PHY802_16_UPLINK_NUMBER_SUBCHANNEL_FFT_128       4


///////////////////////////////////////////////////////////////

typedef enum
{
    BPSK_802_16,
    QPSK,
    QAM_16,
    QAM_64,
} ModulationType;

typedef enum
{
    encodingRate_1_2,
    encodingRate_2_3,
    encodingRate_3_4,
} EncodingRate;

typedef enum
{
    FFT_2048,
    FFT_1024,
    FFT_512,
    FFT_128,
} FFTsize;


/*
 * Structure for phy statistics variables
 */
typedef struct phy_dot16_stats_str {
    Int32 totalTxSignals;
    Int32 totalRxSignalsToMac;
    Int32 totalSignalsLocked;
    Int32 totalSignalsWithErrors;
    Int32 totalRxBurstsToMac;
    Int32 totalRxBurstsWithErrors;
    Int32 totalUlBurstsDropped;

    // dynamic statistics
    int rxBurstsToMacGuiId; // GUI handler
} PhyDot16Stats;

typedef struct phy_dot16_tx_info_str
{
    MacDot16StationType stationType;
    clocktype duration;
} PhyDot16TxInfo;

typedef struct phy_dot16_ofdma_burst_str
{
    Message* pduMsgList;           // list of PDUs inside this burst

    // basic OFDMA info of the burst. The subchannels used must be
    // continuous For downlink, it must be a rectangle allocation.
    unsigned char modCodeType;     // FEC modulation and coding type
    unsigned char startSubchannel; // first subchannel used by this burst
    unsigned char endSubchannel;   // last subchannel used by this burst
    unsigned char symbolOffset;    // index of first symbol
    unsigned char numSymbols;      // for DL, it is width of the burst,
                                   // for UL, it is total number of symbols

    unsigned char signalType;      // type of the signal normal / cdma
                                   // code used for CDMA signal
    unsigned char cdmaCodeIndex;
    unsigned char cdmaSpreadingFactor;

    // actual time information derived from signal arrival time
    clocktype burstStartTime;  // start time of the earliest symbol
    clocktype burstEndTime;    // end time as of the latest symbol
    clocktype firstStartTime;  // start time of the first symbol, for UL
    clocktype lastEndTime;     // end time of the last symbol, only for UL

    // reception info
    BOOL      inError;    // whether this burst has error
    double    cinr;       // CINR of this burst

    struct phy_dot16_ofdma_burst_str* next;
} PhyDot16OfdmaBurst;

typedef struct phy_dot16_rx_msg_info_str
{
    // information identifies the signal
    PropRxInfo* propRxInfo;// propagation receiving info structure
    Message* rxMsg;        // the packet associated with the signal

    // signal reception info
    clocktype rxBeginTime; // signal arrival time
    clocktype rxEndTime;   // end time of the signal
    double rxMsgPower_mW[MAX_NUM_SUBCHANNELS];  // receiving signal power
    BOOL isDownlink;       // whether this signal is DL tx or UL tx
    BOOL inError;          // whether this signal is totally in error

    // burst information
    int numBursts;         // # of bursts included in this tx
    PhyDot16OfdmaBurst* ofdmaBurst; // list of bursts within this signal
    PhyDot16OfdmaBurst* ofdmaBurstTail; // point to the last burst of list

    // subchannel usage information of the signal aggregated from all bursts
    unsigned char startSubchannel;  // first subchannel used by this signal
    unsigned char endSubchannel;    // last subchannel used by this signal
    clocktype subchannelStartTime[MAX_NUM_SUBCHANNELS];
    clocktype subchannelEndTime[MAX_NUM_SUBCHANNELS];

    struct phy_dot16_rx_msg_info_str* next;
} PhyDot16RxMsgInfo;

typedef struct phy_dot16_plcp_header
{
    BOOL isDownlink;
    clocktype duration;
} PhyDot16PlcpHeader;

typedef struct struct_phy_dot16_str
{
    PhyData*  thisPhy;

    // current mode and previous mode of the radio.
    // If txing on any subchannel, mode is PHY_TRANSMITTING
    // Else, if rxing from any subchannel, mode is PHY_RECEIVING
    // Else, mode is PHY_IDLE
    PhyStatusType mode;
    PhyStatusType previousMode;

    // some general parameters
    char      stationType;
    double   channelBandwidth;
    FFTsize   fftSize;
    double    cyclicPrefix;

    int       downlink_numSubchannels;
    int       uplink_numSubchannels;
    clocktype rxTxTurnaroundTime;
    double    noisePower_mW;
    clocktype ofdmSymbolDuration;

    // tx parameters
    int       txDataRateType;
    D_Float32 txPower_dBm;
    /* Bug fix start for 5397*/
    D_Float32 txMaxPower_dBm;
    /* Bug fix end for 5397*/
    float     txDefaultPower_dBm;

    // rx parameters
    int       rxDataRateType;
    double    rxSensitivity_mW[PHY802_16_NUM_DATA_RATES];

    // data rate for different FEC coding modulation combinations
    int       dataRate[PHY802_16_NUM_DATA_RATES];
    double    uplinkNumDataBitsPerSubchannel[PHY802_16_NUM_DATA_RATES];
    double    downlinkNumDataBitsPerSubchannel[PHY802_16_NUM_DATA_RATES];
    int       numDataRates;

    int numTxMsgs;
    std::list <Message *>* txEndMsgList; // list of Tx End msgs for cancel
    PhyDot16RxMsgInfo* rxMsgList;  // list of receiving signals
    PhyDot16RxMsgInfo* ifMsgList;  // list of interference signals
    clocktype rxTimeEvaluated;

    PhyDot16Stats  stats;
    double PowerLossDueToCPTime;
    // interference power on each subchannes
    double*   interferencePower_mW;
} PhyDataDot16;


//--------------------------------------------------------------------------
//  API functions for setting and getting information of the 802.16 PHY
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: PhyDot16GetStatus
// LAYER      :: PHY
// PURPOSE    :: Get the status of the PHY. This is an inline function
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + phyNum    : int           : Index of the PHY
// RETURN     :: PhyStatusType : Status of the PHY
// **/
static inline
PhyStatusType PhyDot16GetStatus(Node *node, int phyNum)
{
    PhyDataDot16* phyDot16 =
       (PhyDataDot16*)node->phyData[phyNum]->phyVar;
    return (phyDot16->mode);
}

// /**
// FUNCTION         :: PhyDot16GetDataRate
// LAYER            :: PHY
// PURPOSE          :: Get the raw data rate
// PARAMETERS       ::
// + thisPhy         : PhyData* : Pointer to PHY structure
// + moduCodeType    : int   : Modulation encoding type
// RETURN           :: double      : Data rate in bps
// **/

double PhyDot16GetDataRate(PhyData *thisPhy,
                           int moduCodeType);

// /**
// FUNCTION   :: PhyDot16GetTxDataRate
// LAYER      :: PHY
// PURPOSE    :: Get the transmission data rate
// PARAMETERS ::
// + thisPhy   : PhyData* : Pointer to PHY structure
// RETURN     :: int      : Transmission data rate in bps
// **/

int PhyDot16GetTxDataRate(PhyData *thisPhy);

// /**
// FUNCTION   :: PhyDot16GetRxDataRate
// LAYER      :: PHY
// PURPOSE    :: Get the reception data rate
// PARAMETERS ::
// + thisPhy   : PhyData* : Pointer to PHY structure
// RETURN     :: int      : Reception data rate in bps
// **/
int PhyDot16GetRxDataRate(PhyData *thisPhy);

// /**
// FUNCTION   :: PhyDot16GetNumberSubchannels
// LAYER      :: PHY
// PURPOSE    :: Obtain the number of subchannels.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: int   : Number of subchannels
// **/
int PhyDot16GetNumberSubchannels(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyDot16GetNumberSubchannelsReceiving
// LAYER      :: PHY
// PURPOSE    :: Obtain the number of subchannels for receiving.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: int   : Number of subchannels
// **/

int PhyDot16GetNumberSubchannelsReceiving(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyDot16GetOfdmSymbolDuration
// LAYER      :: PHY
// PURPOSE    :: Obtain the duration of an OFDM symbol
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : int       : Index of the PHY
// RETURN     :: clocktype : Duration of a OFDM symbol
// **/

clocktype PhyDot16GetOfdmSymbolDuration(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyDot16GetUplinkNumberSubchannels
// LAYER      :: PHY
// PURPOSE    :: Obtain the number of subchannels.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: int   : Number of subchannels
// **/

int PhyDot16GetUplinkNumberSubchannels(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyDot16GetDataBitsPayloadPerSubchannel
// LAYER      :: PHY
// PURPOSE    :: Obtain data payload in bits for per subchannel.
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + moduCodeType : int   : Modulation encoding type
// + subframeType : MacDot16SubframeType : Downlink or uplink
// RETURN     :: int      : Data payload in bits
// **/

int PhyDot16GetDataBitsPayloadPerSubchannel(
        Node* node,
        int phyIndex,
        int moduCodeType,
        MacDot16SubframeType subframeType);
// /**
// FUNCTION   :: PhyDot16GetModulationEncodingType
// LAYER      :: PHY
// PURPOSE    :: Get the modulation encoding combined type based on
//               modulation type and encoding rate.
// PARAMETERS ::
// + moduType     : ModulationType : Modulation type.
// + encodingRate : EncodingRate   : Encoding rate
// RETURN     ::    int            : Modulation and encoding combined type
// **/
int PhyDot16SetModulationEncoding(ModulationType moduType,
                                  EncodingRate encodingRate);

// /**
// FUNCTION   :: PhyDot16GetFFTSize
// LAYER      :: PHY
// PURPOSE    :: Get the FFT size.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: int   : FFT size
// **/
double PhyDot16GetFFTSize(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyDot16SetStationType
// LAYER      :: PHY
// PURPOSE    :: Used by MAC layer to tell PHY whether this station
//               is Base Station (BS) or Subscriber Station (SS)
// PARAMETERS ::
// + node        : Node* : Pointer to node.
// + phyIndex    : int   : Index of the PHY
// + stationType : char  : Type of the station
// RETURN     ::   void  : NULL
// **/
void PhyDot16SetStationType(Node* node, int phyIndex, char stationType);

// /**
// FUNCTION   :: PhyDot16GetRxSensitivity_mW
// LAYER      :: PHY
// PURPOSE    :: Return the receiving sensitivity in mW for a
//               givin modulation and encoding combined type
// PARAMETERS ::
// + node         : Node*  : Pointer to node.
// + phyIndex     : int    : Index of the PHY
// + moduCodeType : int    : Modulation & encoding type
// RETURN     ::    double : Receiving sensitivity in mW
// **/
double PhyDot16GetRxSensitivity_mW(Node *node,
                                   int phyIndex,
                                   int moduCodeType);

// /**
// FUNCTION   :: PhyDot16SetTransmitPower
// LAYER      :: PHY
// PURPOSE    :: Set the transmit power of the PHY
// PARAMETERS ::
// + node       : Node*  : Pointer to node.
// + phyIndex   : int    : Index of the PHY
// + txPower_mW : double : Transmit power in mW unit
// RETURN     ::  void   : NULL
// **/
void PhyDot16SetTransmitPower(Node *node, int phyIndex, double txPower_mW);

// /**
// FUNCTION   :: PhyDot16GetTransmitPower
// LAYER      :: PHY
// PURPOSE    :: Retrieve the transmit power of the PHY in mW unit
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex   : int     : Index of the PHY
// + txPower_mW : double* : For returning the transmit power
// RETURN     ::  void    : NULL
// **/
void PhyDot16GetTransmitPower(Node *node, int phyIndex, double *txPower_mW);

//--------------------------------------------------------------------------
//  Key API functions of the 802.16 PHY
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: PhyDot16Init
// LAYER      :: PHY
// PURPOSE    :: Initialize the 802.16 PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to the node input
// RETURN     :: void             : NULL
// **/
void PhyDot16Init(Node *node,
                  const int phyIndex,
                  const NodeInput *nodeInput);

// /**
// FUNCTION   :: PhyDot16Finalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the 802.16 PHY, print out statistics
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : const int : Index of the PHY
// RETURN     :: void      : NULL
// **/
void PhyDot16Finalize(Node *node, const int phyIndex);

// /**
// FUNCTION   :: PhyDot16SignalArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyDot16SignalArrivalFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo *propRxInfo);

// /**
// FUNCTION   :: PhyDot16SignalEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyDot16SignalEndFromChannel(
        Node* node,
        int phyIndex,
        int channelIndex,
        PropRxInfo *propRxInfo);

// /**
// FUNCTION   :: PhyDot16TerminateCurrentReceive
// LAYER      :: PHY
// PURPOSE    :: Terminate all signals current under receiving. Move them
//               to be as interference signals
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + terminateOnlyOnReceiveError : const BOOL : Only terminate if in error
// + frameError   : BOOL*        : For returning if frame is in error
// + endSignalTime: clocktype*   : For returning expected signal end time
// RETURN     :: void : NULL
// **/
void PhyDot16TerminateCurrentReceive(
         Node* node,
         int phyIndex,
         const BOOL terminateOnlyOnReceiveError,
         BOOL* frameError,
         clocktype* endSignalTime);

// /**
// FUNCTION   :: PhyDot16StartTransmittingSignal
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + packet : Message* : Frame to be transmitted
// + useMacLayerSpecifiedDelay : Use MAC specified delay or calculate it
// + initDelayUntilAirborne    : clocktype : The MAC specified delay
// RETURN     :: void : NULL
// **/
void PhyDot16StartTransmittingSignal(
         Node* node,
         int phyIndex,
         Message* packet,
         BOOL useMacLayerSpecifiedDelay,
         clocktype initDelayUntilAirborne);

// /**
// FUNCTION   :: PhyDot16TransmissionEnd
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + msg       : Message* : Tx End event
// RETURN     :: void  : NULL
// **/
void PhyDot16TransmissionEnd(Node *node, int phyIndex, Message* msg);

// /**
// FUNCTION   :: PhyDot16ChannelListeningSwitchNotification
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
void PhyDot16ChannelListeningSwitchNotification(Node *node,
                                                int phyIndex,
                                                int channelIndex,
                                                BOOL startListening);

void PhyDot16SetBerTable(PhyData* thisPhy);

/*As per section 10.3.4 and 8.4.6 (802.16-2004.pdf) Ps is calculated to
   send RTG and TTG value */
// /**
// FUNCTION   :: PhyDot16ConvertTTGRTGinPS
// LAYER      :: PHY
// PURPOSE    :: Calculate PS and convert RTG and TTG to PS
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : int       : Index of the PHY
// + rtgTtgValue : clocktype : value of RTG/TTG in clocktype
// RETURN     :: UInt8 : number of PS
// **/
unsigned char PhyDot16ConvertTTGRTGinPS(
         Node* node,
         int phyIndex,
         clocktype rtgTtgValue);


// /**
// FUNCTION   :: PhyDot16ConvertTTGRTGinclocktype
// LAYER      :: PHY
// PURPOSE    :: Calculate PS and convert RTG and TTG to time duration
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : int       : Index of the PHY
// + rtgTtgValue : unsigned char : number of PS
// RETURN     :: clocktype : RTG/TTG in time duration
// **/
clocktype PhyDot16ConvertTTGRTGinclocktype(
         Node* node,
         int phyIndex,
         unsigned char rtgTtgValue);

// /**
// FUNCTION   :: PhyDot16InterferenceArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle interference signal arrival from the chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// + sigPower_mw  : double      : The inband interference signal power in mW
// RETURN     :: void : NULL
// **/
void PhyDot16InterferenceArrivalFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo *propRxInfo,
         double sigPower_mW);

// /**
// FUNCTION   :: PhyDot16InterferenceEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle interference signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// + sigPower_mW  : double      : The inband interference power in mW
// RETURN     :: void : NULL
// **/
void PhyDot16InterferenceEndFromChannel(
        Node* node,
        int phyIndex,
        int channelIndex,
        PropRxInfo *propRxInfo);

#endif /* PHY_DOT16_H */
