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


// /**
// PROTOCOL :: PHY-SATELLITE-RSV
// SUMMARY :: An implementation of a simple one or two channel PHY
//            protocol for satellite networks.
// LAYER :: PHY
// STATISTICS ::
//+              Signals transmitted : The number of signals transmitted
//               by this physical layer process.
//+              Signals received and forwarded to MAC : The number of
//               signals received by this physical layer process and
//               subsequently forward to the MAC layer for further
//               processing.
//+              Signals locked on by PHY : The number of signals that
//               triggered logic to lock the transceiver onto an
//               incoming signal.
//+              Signals received but with errors : The number of signals
//               that were successfully received by the MAC but had
//               errors due to interference or noise corruption.
// APP_PARAM ::
// CONFIG_PARAM ::
//+              PHY-SATELLITE-RSV-CHANNEL-BANDWIDTH : Double : The bandwidth
//               of the channel in Hz.  This is the net channel rate after
//               filtering, shaping and other overheads are removed.
//+              PHY-SATELLITE-RSV-PREAMBLE-SIZE : Integer : The size of the
//               preamble in bytes.  This is directly added to the
//               coded packet size for transmission.
//+              PHY-SATELLITE_RSV-USE-SHORTEN-LAST-CODEWORD : String :
//               This is a textual flag to indicate whether the
//               Reed-Solomon logic is able to shorten the last codeword
//               of a transmission and create a (n-s,k-s) codeword from
//               the original (n,k) codeword for s elementOf Integer >= 0.
//+              PHY-SATELLITE-RSV-BLOCK-CODING : String : The type of
//               outer block coding supported by the system.  Currently
//               the only value available is REED-SOLOMON-204-188.
//+              PHY-SATELLITE-RSV-TRANSMISSION-MODE : String : A textual
//               description of the type of digital modulation to be used
//               in the system.  This currently can be BPSK|QPSK|8PSK.  Not
//               all modulation profiles can be used with all convolutional
//               codes.  See PHY-SATELLITE-RSV-CONVOLUTIONAL-CODING for
//               more information.
//+              PHY-SATELLITE-RSV-CONVOLUTIONAL-CODING : String : The
//               type of convolutional code to be used for the inner code
//               of the digital communications layer.  The choices are
//               VITERBI-1-2 | VITERBI-2-3 | VITERBI-3-4 |
//               VITERBI-5-6 | VITERBI-7-8 | VITERBI-8-9
//               Not all convolutional codes may be used with all modulation
//               modes.  The table below describes the allowable modes
//               for each convolutional coding mode.  An "X" indicates that
//               the given mode is supported by the associated digital
//               modulation mode.
//
//               Convolutional Code | BPSK | QPSK | 8PSK
//               VITERBI-1-2        |   X  |   X  |
//               VITERBI-2-3        |   X  |      |   X
//               VITERBI-3-4        |   X  |   X  |
//               VITERBI-5-6        |   X  |      |   X
//               VITERBI-7-8        |   X  |   X  |
//               VITERBI-8-9        |   X  |      |   X
//
//+              PHY-SATELLITE-RSV-GUARD-TIME : Time : The amount of time
//               to be used the guard time interval in the TDMA system.
//               This amount of time is added to each transmission to
//               account for ramp-up, ramp-down and other communications
//               effects during transmission.
//+              PHY-SATELLITE-RSV-ADJACENT-CHANNEL-INTERFERENCE : Double :
//               This specifies the adjacent channel interference imposed
//               on the current channel by all other channels in the system.
//               It is specified in dBc.
//+              PHY-SATELLITE-RSV-ADJACENT-SATELLITE-INTERFERENCE : Double :
//               This specifies the adjacent channel interference imposed
//               on the current channel by all other satellites using the
//               same channel in the system.  It is specified in dBc.
//+              PHY-SATELLITE-RSV-INTERMODULATION-INTERFERENCE : Double :
//               This parameter specifies the amount of intermodulation
//               interference that is caused by nonlinearities in the
//               transmission amplier stages.  It is measured and dBc
//               and should be the nominal value at the peak operating
//               point of the amplifier.
//+              PHY-SATELLITE-RSV-EBNO-THRESHOLD : Double : This is the
//               level of power per user (uncoded) bit scaled by the
//               noise of the system.  If the reception signal level is
//               higher than this threshold, the packet is received
//               without error.  If it is lower, then the packet is
//               marked as having data errors.  This model is consistent
//               with coding systems that have a sharp drop-off between
//               quasi-error-free operation and significant bit error
//               rates.
// SPECIAL_CONFIG ::
// VALIDATION ::
// IMPLEMENTED_FEATURES ::
// +             Reed-Solomon Viterbi Coding : Implements a general GF256
//               RSV codign system with code shortening.
// +             Accurate framing : Includes explicit generation of the
//               physical tranmission frame from coding and modulation
//               parameters.
// +             Effects of interference.  This model incorporates
//               abstract effects of other sources of interferences such
//               as adjacent satellite, adjacent channel and
//               intermodulation.
// OMITTED_FEATURES ::
// +             BER curves : No BER curves are included in this model
//               and a cliff 7 dB Eb/No is assumed.  The actual Eb/No
//               requirement depends on the type of coding being used.
//               For example, punctured convolutional codes tends to
//               require more Eb/No than their mother code.
// ASSUMPTIONS ::
// STANDARDS :: DVB-S (For RSV parameters)
// RELATED ::
// **/

#ifndef PHY_SATELLITE_RSV_H
#define PHY_SATELLITE_RSV_H

// /**
// CONSTANT :: PhySatelliteRsvDefaultAntennaGaindB : 43.0
// DESCRIPTION :: The default antenna gain (in dBi) of the
// satellite antenna.
// **/

static double const PhySatelliteRsvDefaultAntennaGaindB = 43.0;

// /**
// CONSTANT :: PhySatelliteRsvDefaultAntennaHeightm : 1.5
// DESCRIPTION :: The default antenna height for each node.
// **/

static double const PhySatelliteRsvDefaultAntennaHeightm = 1.5;

// /**
// CONSTANT :: SATELLITE_CODE_RATE_188_204 : 188.0 / 204.0
// DESCRIPTION :: The code rate of a RS(204,188) code
// **/

static const double SATELLITE_CODE_RATE_188_204 = ((double)(188.0/204.0));

// /**
// CONSTANT :: SATELLITE_BITS_PER_SYMBOL_BPSK : 1
// DESCRIPTION :: The number of bits per symbol encoded in
// BPSK modulation.
// **/

static const double SATELLITE_BITS_PER_SYMBOL_BPSK = ((double)1.0);

// /**
// CONSTANT :: SATELLITE_BITS_PER_SYMBOL_QPSK : 2
// DESCRIPTION :: The number of bits per symbol encoded in
// QPSK modulation.
// **/

static const double SATELLITE_BITS_PER_SYMBOL_QPSK = ((double)2.0);

// /**
// CONSTANT :: SATELLITE_BITS_PER_SYMBOL_8PSK : 3
// DESCRIPTION :: The number of bits encoded into each 8PSK symbol.
// **/
static const double SATELLITE_BITS_PER_SYMBOL_8PSK = ((double)3.0);

// /**
// CONSTANT :: SATELLITE_CODE_RATE_VITERBI_1_2 : 1/2
// DESCRIPTION :: The code rate of a Viterbi (1,2) convolutional code.
// **/

static const double SATELLITE_CODE_RATE_VITERBI_1_2 = ((double)(1.0/2.0));

// /**
// CONSTANT :: SATELLITE_CODE_RATE_VITERBI_2_3 : 2/3
// DESCRIPTION :: The code rate of a Viterbi (2,3) convolutional code.
// **/

static const double SATELLITE_CODE_RATE_VITERBI_2_3 = ((double)(2.0/3.0));

// /**
// CONSTANT :: SATELLITE_CODE_RATE_VITERBI_3_4 : 3/4
// DESCRIPTION :: The code rate for a Viterbi (3,4) convolutional code.
// **/

static const double SATELLITE_CODE_RATE_VITERBI_3_4 = ((double)(3.0/4.0));

// /**
// CONSTANT :: SATELLITE_CODE_RATE_VITERBI_5_6 : 5/6
// DESCRIPTION :: The code rate for a Viterbi (5,6) convolutional code.
// **/

static const double SATELLITE_CODE_RATE_VITERBI_5_6 = ((double)(5.0/6.0));

// /**
// CONSTANT :: SATELLITE_CODE_RATE_VITERBI_7_8 : 7/8
// DESCRIPTION :: The code rate for a Viterbi (7,8) convolutional code.
// **/

static const double SATELLITE_CODE_RATE_VITERBI_7_8 = ((double)(7.0/8.0));

// /**
// CONSTANT :: SATELLITE_CODE_RATE_VITERBI_8_9 : 8/9
// DESCRIPTION :: The code rate for a Viterbi (8,9) convolutional code.
// **/

static const double SATELLITE_CODE_RATE_VITERBI_8_9 = ((double)(8.0/9.0));

// /**
// ENUM :: PhySatelliteRsvDataRateType
// DESCRIPTION :: A enumated set of data rates available to the RSV
// PHY.
// **/

enum PhySatelliteRsvDataRateType {
    PhySatelliteRsvDataRateZero = 0,
    PhySatelliteRsvDataRateOne,
    PhySatelliteRsvDataRateTwo,
    PhySatelliteRsvUserDefined
};

// /**
// STRUCT :: PhySatelliteRsvModulationProfile
// DESCRIPTION :: A data structure that describes the set of parameters
// associated with a given digital modulation profile.
// **/

struct PhySatelliteRsvModulationProfile {
    double sensitivitydBm;
    double transmitPowermW;
    double bitsPerSymbol;
    //double dataRate;
};

// /**
// ENUM :: PhySatelliteRsvModulationType
// DESCRIPTION :: An enumerated list of modulation types for the system.
// **/

enum PhySatelliteRsvModulationType {
    PhySatelliteRsvModulationBpsk = 1,
    PhySatelliteRsvModulationQpsk = 2,
    PhySatelliteRsvModulation8psk = 3,
    PhySatelliteRsvModulation16qam = 4,
    PhySatelliteRsvModulationUserDefined = 5
};

// /**
// ENUM :: PhySatelliteRsvConvolutionalCodingType
// DESCRIPTION :: An enumerated list of convolutional coding
// profiles supported by the physical layer process.
// **/

enum PhySatelliteRsvConvolutionalCodingType {
    PhySatelliteRsvConvolutionalCoding1_2 = 1,
    PhySatelliteRsvConvolutionalCoding2_3 = 2,
    PhySatelliteRsvConvolutionalCoding3_4 = 3,
    PhySatelliteRsvConvolutionalCoding5_6 = 4,
    PhySatelliteRsvConvolutionalCoding7_8 = 5,
    PhySatelliteRsvConvolutionalCoding8_9 = 6,
};

// /**
// ENUM :: PhySatelliteRsvBlockCodingType
// DESCRIPTION :: The set of block codes support by the physical
// layer process.
// **/

enum PhySatelliteRsvBlockCodingType {
    PhySatelliteRsvBlockCoding188_204 = 1
};

// /**
// CONSTANT :: PhySatelliteRsvDefaultModulationProfileCount : 3
// DESCRIPTION :: The number of modulation profiles supported in this
// model.
// **/

#define PhySatelliteRsvDefaultModulationProfileCount (4)

// /**
// CONSTANT :: PhySatelliteRsvBpskDefaultTransmitPowermW : 3000
// DESCRIPTION :: The default transmission power in the BPSK modulation
// profile.
// **/

static double const PhySatelliteRsvBpskDefaultTransmitPowermW = 3000.0;

// /**
// CONSTANT :: PhySatelliteRsvBpskDefaultSensitivitydBm : -91
// DESCRIPTION :: The default sensitivity of the BPSK receiver.
// **/

static double const PhySatelliteRsvBpskDefaultSensitivitydBm = -91.0;

// /**
// CONSTANT :: PhySatelliteRsvBpskDefaultBitsPerSymbol :
// SATELLITE_BITS_PER_SYMBOL_BPSK
// DESCRIPTION :: The bits per symbol of the BPSK modulator.
// **/

static double const PhySatelliteRsvBpskDefaultBitsPerSymbol
    = SATELLITE_BITS_PER_SYMBOL_BPSK;

// /**
// CONSTANT :: PhySatelliteRsvBpskDefaultDataRate : 1,280,000
// DESCRIPTION :: The default data rate for the BPSK modulator.
// **/

//static double const PhySatelliteRsvBpskDefaultDataRate = 1280.0e3;

// /**
// CONSTANT :: PhySatelliteRsvDefaultBpskProfile : {
// PhySatelliteRsvBpskDefaultSensitivitydBm,
// PhySatelliteRsvBpskDefaultTransmitPowermW,
// PhySatelliteRsvBpskDefaultBitsPerSymbol,
// PhySatelliteRsvBpskDefaultDataRate
// }
// DESCRIPTION :: The default BPSK modulation profile.
// **/

static PhySatelliteRsvModulationProfile PhySatelliteRsvDefaultBpskProfile = {
    PhySatelliteRsvBpskDefaultSensitivitydBm,
    PhySatelliteRsvBpskDefaultTransmitPowermW,
    PhySatelliteRsvBpskDefaultBitsPerSymbol,
    //PhySatelliteRsvBpskDefaultDataRate
};

// /**
// CONSTANT :: PhySatelliteRsvQpskDefaultTransmitPower : 3000
// DESCRIPTION :: The default transmit power for the QPSK modulator.
// **/

static double const PhySatelliteRsvQpskDefaultTransmitPowermW = 3000.0;

// /**
// CONSTANT :: PhySatelliteRsvQpskDefaultSensitivitydBm : -88
// DESCRIPTION :: The default sensitivity of the receiver.
// **/

static double const PhySatelliteRsvQpskDefaultSensitivitydBm = -88.0;

// /**
// CONSTANT :: PhySatelliteRsvQpskDefaultBitsPerSymbol :
// SATELLITE_BITS_PER_SYMBOL_QPSK
// DESCRIPTION :: The number of bits encoded per symbol in QPSK.
// **/
static double const PhySatelliteRsvQpskDefaultBitsPerSymbol
    = SATELLITE_BITS_PER_SYMBOL_QPSK;

// /**
// CONSTANT :: PhySatelliteRsvQpskDefaultDataRate : 2,560,000
// DESCRIPTION :: The default signaling rate of the QPSK modulator
// in bits/sec.
// **/

//static double const PhySatelliteRsvQpskDefaultDataRate = 2560.0e3;

// /**
// CONSTANT :: PhyStelliteRsvDefaultQpskProfile : {
// PhySatelliteRsvQpskDefaultSensitivitydBm,
// PhySatelliteRsvQpskDefaultTransmitPowermW,
// PhySatelliteRsvQpskDefaultBitsPerSymbol,
// PhySatelliteRsvQpskDefaultDataRate
// }
// DESCRIPTION :: The static default parameters associated with a QPSK
// modem.
// **/

static PhySatelliteRsvModulationProfile PhySatelliteRsvDefaultQpskProfile = {
    PhySatelliteRsvQpskDefaultSensitivitydBm,
    PhySatelliteRsvQpskDefaultTransmitPowermW,
    PhySatelliteRsvQpskDefaultBitsPerSymbol,
    //PhySatelliteRsvQpskDefaultDataRate
};

// /**
// CONSTANT :: PhySatelliteRsv8pskDefaultTransmitPowermW : 3,000
// DESCRIPTION :: The default transmitter power for the 8PSK transmitter.
// **/

static double const PhySatelliteRsv8pskDefaultTransmitPowermW = 3000.0;

// /**
// CONSTANT :: PhySatelliteRsv8pskDefaultSensitivitydBm : -85
// DESCRIPTION :: The default sensitivyt of the 8PSK receiver.
// **/

static double const PhySatelliteRsv8pskDefaultSensitivitydBm = -85.0;

// /**
// CONSTANT :: PhySatelliteRsv8pskDefaultBitsPerSymbol :
// SATELLITE_BITS_PER_SYMBOL_8PSK
// DESCRIPTION :: The number of bits per symbol transmitted by the 8PSK
// modulator.
// **/

static double const PhySatelliteRsv8pskDefaultBitsPerSymbol
    = SATELLITE_BITS_PER_SYMBOL_8PSK;

// /**
// CONSTANT :: PhySatelliteRsv8pskDefaultDataRate : 3,840,000
// DESCRIPTION :: The default data rate for the 8PSK transmitter
// **/

//static double const PhySatelliteRsv8pskDefaultDataRate = 3840.0e3;


// /**
// CONSTANT :: PhySatelliteRsvDefault8pskProfile : {
// PhySatelliteRsv8pskDefaultSensitivitydBm,
// PhySatelliteRsv8pskDefaultTransmitPowermW,
// PhySatelliteRsv8pskDefaultBitsPerSymbol,
// PhySatelliteRsv8pskDefaultDataRate
// }
// DESCRIPTION :: A data structure containing the profile of the
// default 8PSK transceiver.
// **/

static PhySatelliteRsvModulationProfile PhySatelliteRsvDefault8pskProfile = {
    PhySatelliteRsv8pskDefaultSensitivitydBm,
    PhySatelliteRsv8pskDefaultTransmitPowermW,
    PhySatelliteRsv8pskDefaultBitsPerSymbol,
    //PhySatelliteRsv8pskDefaultDataRate
};

// /**
// CONSTANT :: PhySatelliteRsvDefaultChannelWidth : 1280000
// DESCRIPTION :: The default channel width (in Hz) of available
// bandwidth.
// **/

static double const PhySatelliteRsvDefaultChannelBandwidth = 1280.0e3;

// /**
// CONSTANT :: PhySatelliteRsvStateDirectionalAntennaGaindB : 0
// DESCRIPTION :: The default directional antenna gain above omnidirectional
// gain in dBi.
// **/

static double const PhySatelliteRsvStateDirectionalAntennaGaindB = 0.0;

// /**
// CONSTANT :: PhySatelliteRsvStateProfiles : {
// &PhySatelliteRsvDefaultBpskProfile,
// &PhySatelliteRsvDefaultQpskProfile,
// &PhySatelliteRsvDefault8pskProfile
// }
// DESCRIPTION :: A table of pointers to the available modulation profiles
// of the system.  Presently this is filled via initialization code due to
// limitations across all supported compilers.
// **/

static PhySatelliteRsvModulationProfile*
    PhySatelliteRsvStateProfiles[PhySatelliteRsvDefaultModulationProfileCount];

// /**
// CONSTANT :: PhySatelliteRsvDefaultPreambleSize : 64
// DESCRIPTION :: The default preamble size for transmissions expressed in
// bytes.  This is fairly long (512 bits which is from 172 to 512 symbols)
// although in general preambles for transmissions on satellite networks
// are substantially longer due to lower Eb/No levels.
// **/

static int const PhySatelliteRsvDefaultPreambleSize = 64;

// /**
// CONSTANT :: PhySatelliteRsvDefaultGuardTime : 10 us
// DESCRIPTION :: This is the default time added to each transmission to
// account for transmission effects such as ramp-up and ramp down of the
// shaping filters and also propagation effects such as multipath.
// **/

static clocktype const PhySatelliteRsvDefaultGuardTime = 10 * MICRO_SECOND;

// /**
// CONSTANT :: PhySatelliteRsvDefaultShortenLastCodeword : TRUE
// DESCRIPTION :: The default flag to indicate to the system whether
// it should shorted the last Reed-Solomon codeword in a transmission to
// make the block coding more efficient.
// **/

static BOOL const PhySatelliteRsvDefaultShortenLastCodeword = TRUE;

// /**
// CONSTANT :: PhySatelliteRsvDefaultBlockCoding :
// PhySatelliteRsvBlockCoding188_204
// DESCRIPTION :: The default level of block coding for the RS outer code.
// **/

static const PhySatelliteRsvBlockCodingType PhySatelliteRsvDefaultBlockCoding
    = PhySatelliteRsvBlockCoding188_204;

// /**
// CONSTANT :: PhySatelliteRsvDefaultConvolutionalCoding :
// PhySatelliteRsvConvolutionalCoding1_2
// DESCRIPTION :: The default level of convolutional coding if none is
// specified in the configuration file.
// **/

static const PhySatelliteRsvConvolutionalCodingType
    PhySatelliteRsvDefaultConvolutionalCoding
    = PhySatelliteRsvConvolutionalCoding1_2;

// /**
// CONSTANT :: PhySatelliteRsvDefaultTransmissionMode :
// PhySatelliteRsvDataRateZero
// DESCRIPTION :: The default transmission mode for the digital
// modulator.
// **/

static const PhySatelliteRsvDataRateType
    PhySatelliteRsvDefaultTransmissionMode = PhySatelliteRsvDataRateZero;

// /**
// CONSTANT :: PhySatelliteRsvDefaultAci : 18
// DESCRIPTION :: The default level of adjacent channel interference
// in terms of dBc to be used if none are specified in the configuration
// file.
// **/

static double const PhySatelliteRsvDefaultAci = 18.0;

// /**
// CONSTANT :: PhySatelliteRsvDefaultAsi : 30
// DESCRIPTION :: This constant indicates the default
// adjacent satellite interference level, in dBc, that should
// be used if none is specified in the simulation configuration
// file.
// **/

static double const PhySatelliteRsvDefaultAsi = 30.0;

// /**
// CONSTANT : PhySatelliteRsvDefaultIm3 : 25
// Description :: This constant indicates the default
// intermodulation interference level, in dBc, that should
// be used if none is specified in the simulation configuration
// file.
// **/

static double const PhySatelliteRsvDefaultIm3 = 25.0;

// /**
// CONSTANT : PhySatelliteRsvDefaultEbnoThreshold : 7
// Description :: This constant indicates the default
// signal energy (per user-bit), as a ration to ambient noise
// level, that is required to
// successfully receive a transmission.  This value is used if
// none is specified in the simulation configuration
// file.
// **/

static double const PhySatelliteRsvDefaultEbnoThreshold = 7.0;

// /**
// STRUCT :: PhySatelliteRsvHeader
// DESCRIPTION :: The PHY header applied to all MAC SDUs
// transferred to the physical layer process for tranmission.
// **/

struct PhySatelliteRsvHeader {
    clocktype signature;
    int preambleSize;

    BOOL im3Valid;
    BOOL aciValid;
    BOOL asiValid;
    BOOL uplinkSnrValid;

    // Note that the float values in this header are used instead of
    // double values.  This is because packing on different compilers
    // and different processors tend to cause more verification
    // differences on 64-bit quantities but not 32-bit quantities.  Since
    // these are decibel ratios, very little is lost in accuracy.

    float im3;
    float aci;
    float asi;
    float uplinkSnr;
};

// /**
// STRUCT :: PhySatelliteRsvStatistics
// DESCRIPTION :: This data structure holds all counters for
// statistics gathered by the physical layer process.  These are
// printed out when the RuntimeStat API is used.
// **/

struct PhySatelliteRsvStatistics {
    UInt32 totalTxSignals;
    UInt32 totalRxSignalsToMac;
    UInt32 totalSignalsLocked;
    UInt32 totalSignalsWithErrors;
};


struct PhySatelliteRsvChannel {
    int channelIndex;
    PhyStatusType currentMode;
    PhyStatusType previousMode;

    PhySatelliteRsvStatistics stats;

    double interferencePowermW; // storage for current interference power
    double noisePowermW;

    Message* rxMsg; // current message undergoing reception
    double rxMsgPowermW; // power of packet
    BOOL rxMsgError; // error status of packet
    clocktype rxTimeEvaluated; // last signal evaluation time
};

struct PhySatelliteRsvTxRxChannelInfo
{
    PhySatelliteRsvChannel* txChannelInfo;
    PhySatelliteRsvChannel* rxChannelInfo;
};


// /**
// STRUCT :: PhySatelliteRsvState
// DESCRIPTION :: The state data for the Satellite RSV physical
// layer process.  This state data is part of the overall node and
// generic physical layer state.
// **/

struct PhySatelliteRsvState {
    PhyData* thisPhy; // back-pointer to parent data structure

    double channelBandwidth;
    double asi;
    double aci;
    double im3;
    BOOL useShortenLastCodeword;

    PhySatelliteRsvBlockCodingType bcType;
    PhySatelliteRsvConvolutionalCodingType ccType;
    PhySatelliteRsvModulationType modType;
    PhySatelliteRsvDataRateType transmissionMode;

    int preambleSize;
    clocktype guardTime;
    double rxThreshold;

    double transmitPowermW;
    double transmitDataRate;
    double receiveSensitivitydBm;
    double receiveDataRate;
    double bitsPerSymbol;
    double codingRate;

    PhySatelliteRsvDataRateType transmitDataRateType;
    PhySatelliteRsvDataRateType receiveDataRateType;

    BOOL statsWritten;
    int dataRateCount;

    double averageEbnoStat;
    int averageEbnoCount;

    //for satellite and ground station
    LinkedList* upDownChannelInfoList;
};


// /**
// ENUM :: PhySatelliteRsvEventType
// DESCRIPTION :: A list of enumerated events in this physical
// layer process.
// **/

enum PhySatelliteRsvEventType {
    PhySatelliteRsvUlArrival = 1
};

// /**
// FUNCTION :: PHY_PhyStatusTypeToString
// LAYER :: PHY
// PURPOSE :: This function translates a PhyStatusType enumeration
// into a human-readable text string.
// PARAMETERS ::
// +       type : PhyStatusType : An enumerated value indicating
//         a status layer type.  If the value is unrecognized, an
//         error handling function will be invoked.
// RETURN : char* : A pointer to a character array containing a
//          text message.  This point should be held const and not modified.
// **/

static
const char* PHY_PhyStatusTypeToString(PhyStatusType type)
{
    switch(type) {
        case PHY_IDLE: return "[IDLE]"; break;
        case PHY_SENSING: return "[SENSING]"; break;
        case PHY_RECEIVING: return "[RECEIVING]"; break;
        case PHY_TRANSMITTING: return "[TRANSMITTING]"; break;
        default: ERROR_ReportError("Unknown PhyStatusType.");
    }

    return "[UNKNOWN]";
}

// /**
// FUNCTION :: PhySatelliteRsvInit
// LAYER :: PHY
// PURPOSE :: This function initializes the Satellite RSV PHY process.  It is
//            generally called at the beginning of the simulation at time
//            t = 0 but may be called at other times under dynamic node
//            creation conditions.  This function is called by other functions
//            outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        nodeInput : NodeInput* : A pointer to the deserialized data
//          structure corresponding to the current simulation configuration
//          file and and any auxillary files references by the simulation
//          configuration file
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvInit(Node* node,
                         int phyIndex,
                         const NodeInput* nodeInput);

// /**
// FUNCTION :: PhySatelliteRsvFinalize
// LAYER :: PHY
// PURPOSE :: This function finalizes the Satellite RSV PHY process.  It is
//            generally called at the end of the simulation at the epoch but
//            may be called at other times under dynamic node
//            destruction conditions.  This function is called by other functions
//            outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvFinalize(Node* node,
                             int phyNum);

// /**
// FUNCTION :: PhySatelliteRsvTransmissionEnd
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to be executed when
//            the system has completed transmitting a packet.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvTransmissionEnd(Node* node,
                                    int phyIndex,
                                    Message* msg);

// /**
// FUNCTION :: PhySatelliteRsvGetStatus
// LAYER :: PHY
// PURPOSE :: This function returns the current transceiver status for the
//            phyisical layer process. This function is called by other
//            functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: PhyStatusType : This function returns the instantaneous sample
//           of the mode of the physical layer transceiver.
// **/

PhyStatusType PhySatelliteRsvGetStatus(Node* node,
                                       int phyNum);

PhyStatusType PhySatelliteRsvGetStatus(Node* node,
                                       int phyNum,
                                       int channelIndex);

// /**
// FUNCTION :: PhySatelliteRsvStartTransmittingSignal
// LAYER :: PHY
// PURPOSE :: This function implements the algorithm to transmit a signal
//            on the satellite subnet wireless channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyNum : int : The logical index of the current interface
// +        msg : Message* : A pointer to a data structure representing the
//          frame to be sent on the wireless satellite channel.
// +        useMacLayerSpecifiedDelay : BOOL : An indicator to use the
//          standard MAC-specified delay value
// +        delayUntilAirborne : clocktype : The number of nanoseconds until
//          the delay reaches the transmitter.
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvStartTransmittingSignal(Node* node,
                                         int phyNum,
                                         Message *msg,
                                         BOOL useMacLayerSpecifiedDelay,
                                         clocktype delayUntilAirborne);

// /**
// FUNCTION :: PhySatelliteRsvSignalArrivalFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the algorithm to respond to a
//            signal arriving from the channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        channelIndex : int :  The logical index of the channel upon
//          which the channel is arriving.
// +        propRxInfo : PropRxInfo* : A pointer to a data structure
//          containing the propagation reception information at the
//          receiver.  Note this may contain data that would be
//          unavailable to a physically realizable implementation of a
//          receiver.
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvSignalArrivalFromChannel(Node* node,
                                             int phyIndex,
                                             int channelIndex,
                                             PropRxInfo* propRxInfo);

// /**
// FUNCTION :: PhySatelliteRsvSignalEndFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to respond to
//            a completed reception of a packet.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyNum : int : The logical index of the current interface
// +        msg : Message* : A pointer to a data structure representing the
//          frame to be sent on the wireless satellite channel.
// +        channelIndex : int : The logical channel index the channel
//          that is receiving the frame.
// +        propRxInfo : PropRxInfo* : A pointer to a data structure
//          containing the local receiver environment for frame
//          reception.
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvSignalEndFromChannel(Node* node,
                                         int phyIndex,
                                         int channelIndex,
                                         PropRxInfo *propRxInfo);

// /**
// FUNCTION :: PhySatelliteRsvGetTxDataRate
// LAYER :: PHY
// PURPOSE :: This function calculates the current transmission rate
//            of the currently selected transmission channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : The current transmission rate expressed as bits/sec
// **/

int PhySatelliteRsvGetTxDataRate(PhyData* phyData);

// /**
// FUNCTION :: PhySatelliteRsvGetRxDataRate
// LAYER :: PHY
// PURPOSE :: This function calculates the current signaling rate
//            of the packet currently being received.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : The current reception rate expressed as bits/sec
// **/

int PhySatelliteRsvGetRxDataRate(PhyData* phyData);

// /**
// FUNCTION :: PhySatelliteRsvSetTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function implements model logic to set the current
//            data rate for transmission to a value consistent with
//            the provided data rate type.  This function is called
//            by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// +        dateRateType : int : An enumerated set of available data
//          rate types.  Note that this must be a valid data rate
//          type of the program will exhibit unexpected behavior.
// RETURN :: void : This function does not return any value.
// **/

void PhySatelliteRsvSetTxDataRateType(PhyData* thisPhy,
                                      int dataRateType);

// /**
// FUNCTION :: PhySatelliteRsvGetRxDataRateType
// LAYER :: PHY
// PURPOSE :: This function returns the current data rate type for
//            the receive function of physical layer process as
//            an enumerated value.
//            This function is called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : This function returns an enumerated data rate type.
// **/

int PhySatelliteRsvGetRxDataRateType(PhyData* thisPhy);

// /**
// FUNCTION :: PhySatelliteRsvGetTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function returns the current data rate type for
//            the transmit function of physical layer process as
//            an enumerated value.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : This function returns an enumerated data rate type.
// **/

int PhySatelliteRsvGetTxDataRateType(PhyData* thisPhy);

// /**
// FUNCTION :: PhySatelliteRsvSetLowestTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function executes logic that sets the transmission rate
//            for the channel to be the lowest available rate for that
//            physical layer process.  This function is called by other
//            functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetLowestTxDataRateType(PhyData* thisPhy);

// /**
// FUNCTION :: PhySatelliteRsvGetLowestTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function returns the lowest available transmission
//            rate available for the current physical layer process.  This
//            function is called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// +        dataRateType : int* : A pointer to a memory region sufficiently
//          large and aligned to hold an integer data type.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvGetLowestTxDataRateType(PhyData* thisPhy,
                                            int* dataRateType);

// /**
// FUNCTION :: PhySatelliteRsvSetHighestTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function sets the transmission rate of the local
//            physical layer process to its maximum rate possible.  This
//            function is called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetHighestTxDataRateType(PhyData* thisPhy);

// /**
// FUNCTION :: PhySatelliteRsvGetHighestTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function calculates the maximum transmission rate
//            of the local physical layer process.  This
//            function is called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// +        dataRateType : int* : A pointer to a memory region sufficiently
//          large and aligned to hold an integer data type.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvGetHighestTxDataRateType(PhyData* thisPhy,
                                             int* dataRateType);

// /**
// FUNCTION :: PhySatelliteRsvSetHighestTxDataRateTypeForBC
// LAYER :: PHY
// PURPOSE :: This function sets the transmission rate of the local
//            physical layer process to its maximum rate possible
//            for broadcast transmissions.  This function is called by
//            other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetHighestTxDataRateTypeForBC(PhyData* thisPhy);

// /**
// FUNCTION :: PhySatelliteRsvGetHighestTxDataRateTypeForBC
// LAYER :: PHY
// PURPOSE :: This function calculates the maximum transmission rate for
//            broadcast transmissions of the local physical layer process.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// +        dataRateType : int* : A pointer to a memory region sufficiently
//          large and aligned to hold an integer data type.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvGetHighestTxDataRateTypeForBC(PhyData* thisPhy,
                                                  int* dataRateType);

// /**
// FUNCTION :: PhySatelliteRsvGetTransmissionDuration
// LAYER :: PHY
// PURPOSE :: This function calculates the amount of signaling
//            time required to send a set number of bits through
//            the channel. This function is called by other functions
//            outside of this file.
// PARAMETERS ::
// +         size : int : The number of bits to be sent on the channel.
// +         dataRate : int : The available bit rate on this channel
// RETURN :: clocktype : An integer number of nanoseconds required to
//           complete the transmission.
// **/

clocktype PhySatelliteRsvGetTransmissionDuration(int size,
                                                 int dataRate);

// /**
// FUNCTION :: PhySatelliteRsvSetTransmitPower
// LAYER :: PHY
// PURPOSE :: This function sets the internal state of the physical
//            layer process to a new value of the power available to
//            the local transmitter.  This
//            function is called by other functions outside of this file.
// PARAMETERS ::
// +         phyData : PhyData* : A pointer to a data structure containing
//           generic physical layer process data
// +         newTxPower_mW : double : A non-negative value for the power
//           of the transmit process of the transceiver
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetTransmitPower(PhyData* phyData,
                                     double newTxPower_mW);

// /**
// FUNCTION :: PhySatelliteRsvGetTransmitPower
// LAYER :: PHY
// PURPOSE :: This function calculates the internal power available to
//            the transmission component of the physical layer process.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         phyData : PhyData* : A pointer to a data structure containing
//           generic physical layer process data
// +         txPower_mW : double* : A pointer to a memory region that is
//           sufficiently large and properly aligned to store a double
//           length floating point number.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvGetTransmitPower(PhyData* phyData,
                                     double* txPower_mW);

// /**
// FUNCTION :: PhySatelliteRsvGetLastAngleOfArrival
// LAYER :: PHY
// PURPOSE :: This function calculates the angle of arrival of the last frame
//            received by this transceiver process.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to a memory region that contains the
//           generic node state information for the currently executing
//           event.
// +         phyNum : int : The logical index of the physical layer
//           process within the node context.
// RETURN :: double : The return value is the angle of arrival calculated
//           as a rotated angle from boresight.  This is a projection of
//           phi and theta errors into a single off-axis angle.

double PhySatelliteRsvGetLastAngleOfArrival(Node* node,
                                            int phyNum);

// /**
// FUNCTION :: PhySatelliteRsvTerminateCurrentReceive
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to reset the
//            receive state machine during transmission.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyIndex : int : The logical index of the interface of the
//           currently executing node.
// +         terminateOnlyOnRecieveError : BOOL : A flag to indicate
//           whether or not this call should complete successfully
//           (terminate the reception process) only in the case that
//           the current reception is already known to be in error.
// +         receiveError : BOOL* : A pointer to a suitably sized and
//           appropriately aligned memory region to hold a copy of the
//           current reception status of the active frame.
// +         endSignalTime : clocktype* : A pointer to a memory region that
//           is suitably large and properly aligned to hold a copy of the
//           present time estimated for the packet to complete reception
//           at the local node.
// RETURN :: void : NULL
// **/

void
PhySatelliteRsvTerminateCurrentReceive(Node* node,
                                       int phyIndex,
                                       const BOOL terminateOnlyOnReceiveError,
                                       BOOL* receiveError,
                                       clocktype* endSignalTime);

// /**
// FUNCTION :: PhySatelliteRsvTerminateCurrentReceive
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to reset the
//            receive state machine during transmission.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyIndex : int : The logical index of the interface of the
//           currently executing node.
// +         terminateOnlyOnRecieveError : BOOL : A flag to indicate
//           whether or not this call should complete successfully
//           (terminate the reception process) only in the case that
//           the current reception is already known to be in error.
// +         receiveError : BOOL* : A pointer to a suitably sized and
//           appropriately aligned memory region to hold a copy of the
//           current reception status of the active frame.
// +         endSignalTime : clocktype* : A pointer to a memory region that
//           is suitably large and properly aligned to hold a copy of the
//           present time estimated for the packet to complete reception
//           at the local node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvTerminateCurrentReceive(Node* node,
                                    int phyIndex,
                                    const BOOL terminateOnlyOnReceiveError,
                                    BOOL* receiveError,
                                    clocktype* endSignalTime);

// /**
// FUNCTION :: PhySatelliteRsvStartTransmittingSignalDirectionally
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to begin the
//            transmission of a frame using the specified azimuth
//            angle.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// +         msg : Message* : A pointer to a memor region holding a
//           Message data structure that represents the frame that is to
//           be sent on the currently configured channel.
// +         useMacLayerSpecifiedDelay : BOOL : A flag to indicate whether
//           or not the process should use the generic value in the MAC
//           layer as the specified latency through the MAC layer.
// +         delayUntilAirborne : clocktype : The number of nanoseconds
//           until the signal should become active at the output of the
//           transceiver.
// +         azimuthAngle : double : The current azimuth of the physical
//           system.  It is assumed that the directional antenna can
//           control this one degree of freedom in the model API.
// RETURN :: void : NULL
// **/

void
PhySatelliteRsvStartTransmittingSignalDirectionally(Node* node,
                                            int phyNum,
                                            Message* msg,
                                            BOOL useMacLayerSpecifiedDelay,
                                            clocktype delayUntilAirborne,
                                            double azimuthAngle);

// /**
// FUNCTION :: PhySatelliteRsvLockAntennaDirection
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to lock the
//            transceiver antenna in its current physical configuration.
//            When this call completes the transceiver will not modify its
//            gain configuration until an equivalent call to
//            unlock the antenna direction.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvLockAntennaDirection(Node* node,
                                         int phyNum);

// /**
// FUNCTION :: PhySatelliteRsvUnlockAntennaDirection
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to unlock the
//            transceiver antenna in its current physical configuration.
//            After this call completes, the transceiver will be allowed
//            to freely modify its antenna gain pattern until any future
//            call to lock the gain pattern.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvUnlockAntennaDirection(Node* node,
                                           int phyNum);

// /**
// FUNCTION :: PhySatelliteRsvMediumIsIdleInDirection
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to calculate
//            whether or not the physical, wireless, network is idle
//            in the given azimuth angle.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// +         azimuth : double : The azimuth angle at which the calculation
//           is to occur.
// RETURN :: BOOL : A binary response that indicates whether or not the
//           signal level is sufficiently high to trigger the transceiver
//           or not.
// **/

BOOL PhySatelliteRsvMediumIsIdleInDirection(Node* node,
                                            int phyNum,
                                            double azimuth);

// /**
// FUNCTION :: PhySatelliteRsvSetSensingDirection
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to set the
//            sensing direction of the transceiver.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// +         azimuth : double : The new azimuth angle for the
//           transceiver.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetSensingDirection(Node* node,
                                        int phyNum,
                                        double azimuth);

void
PhySatelliteRsvAddUpDownLinkChannel(
    Node* node,
    int phyIndex,
    int upLinkChannelId,
    int channelId,
    const char* role);

PhySatelliteRsvChannel*
PhySatelliteRsvGetRxLinkChannelInfo(
    Node* node,
    int phyIndex,
    int channelId);

PhySatelliteRsvChannel*
PhySatelliteRsvGetTxLinkChannelInfo(
    Node* node,
    int phyIndex,
    int upLinkChannelId);

PhySatelliteRsvChannel*
PhySatelliteRsvGetLinkChannelInfo(
    Node* node,
    int phyIndex,
    int channelId);

// /**
// FUNCTION :: PhySatelliteRsvGetLinkChannelPairNum
// LAYER :: PHY
// PURPOSE :: This function returns number of rx-tx pair in the linked list.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: int : This function returns number of rx-tx pair in
//          the linked list.
// **/

int
PhySatelliteRsvGetLinkChannelPairNum(
    Node* node,
    int phyIndex);

// /**
// FUNCTION :: PhySatelliteRsvGetLinkChannelPairInfoByIndex
// LAYER :: PHY
// PURPOSE :: This function returns TxRxChannelInfoPair by index
//          from the linked list.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        index : int : index of rx-tx channel pair in the linked list.
// RETURN :: PhySatelliteRsvTxRxChannelInfo* : This function returns
//          TxRxChannelInfoPair by index from the linked list.
// **/

PhySatelliteRsvTxRxChannelInfo*
PhySatelliteRsvGetLinkChannelPairInfoByIndex(
    Node* node,
    int phyIndex,
    int index);

// /**
// FUNCTION :: PhySatelliteRsvInterferenceArrivalFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the algorithm to respond to a
//            interference signal arriving from the channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        channelIndex : int :  The logical index of the channel upon
//          which the channel is arriving.
// +        propRxInfo : PropRxInfo* : A pointer to a data structure
//          containing the propagation reception information at the
//          receiver.  Note this may contain data that would be
//          unavailable to a physically realizable implementation of a
//          receiver.
// +        sigPowermW : double      : the interference signal power in mW
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvInterferenceArrivalFromChannel(Node* node,
                                             int phyIndex,
                                             int channelIndex,
                                             PropRxInfo* propRxInfo,
                                             double sigPowermW);

// /**
// FUNCTION :: PhySatelliteRsvInterferenceEndFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to respond to
//            a intereference end event.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyNum : int : The logical index of the current interface
// +        msg : Message* : A pointer to a data structure representing the
//          frame to be sent on the wireless satellite channel.
// +        channelIndex : int : The logical channel index the channel
//          that is receiving the frame.
// +        propRxInfo : PropRxInfo* : A pointer to a data structure
//          containing the local receiver environment for frame
//          reception.
// +        sigPowermW : double : the interference signal power in mW
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvInterferenceEndFromChannel(Node *node,
                                         int phyIndex,
                                         int channelIndex,
                                         PropRxInfo *propRxInfo,
                                         double sigPower);
#endif /* PHY_SATELLITE_RSV_H */
