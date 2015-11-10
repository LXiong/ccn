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
// PACKAGE     :: PHY_UMTS
// DESCRIPTION :: Defines the data structures used in the UMTS Model
//                Most of the time the data structures and functions start
//                with PhyUmts**
// **/

#ifndef _PHY_UMTS_H_
#define _PHY_UMTS_H_

#include <list>
#include <vector>
#include <algorithm>

#include "cellular_umts.h"
#include "dynamic.h"
#include "umts_constants.h"

// /**
// CONSTANT    :: PHY_UMTS_CHANNEL_BANDWIDTH
// DESCRIPTION :: Channel bandwidth for UMTS system
// **/

#define PHY_UMTS_CHANNEL_BANDWIDTH    5000000    //    5 MHz

// /**
// CONSTANT    :: PHY_UMTS_CHIP_RATE
// DESCRIPTION :: Chip rate for UMTS system
//                only 3840000 is ocnsidered in this release
// **/
#define PHY_UMTS_CHIP_RATE            3840000

// /**
// CONSTANT    :: PHY_UMTS_RX_TX_TURNAROUND_TIME
// DESCRIPTION :: Transmission and reception turnaround time
// **/
#define PHY_UMTS_RX_TX_TURNAROUND_TIME  (25 * MICRO_SECOND)

// /**
// CONSTANT    :: PHY_UMTS_PHY_DELAY
// DESCRIPTION :: Delay incurred at PHY layer
// **/
#define PHY_UMTS_PHY_DELAY PHY_UMTS_RX_TX_TURNAROUND_TIME

// /**
// CONSTANT    :: PHY_UMTS_QPSK_R_1_3
// DESCRPTION  :: coding cheme: QPSK 1/3
// **/
#define PHY_UMTS_QPSK_R_1_3   0

// /**
// CONSTANT    :: PHY_UMTS_QPSK_R_1_2
// DESCRPTION  :: coding cheme: QPSK 1/2
// **/
#define PHY_UMTS_QPSK_R_1_2   1

// /**
// CONSTANT    :: PHY_UMTS_16QAM_R_1_3
// DESCRPTION  :: coding cheme: 16QAM 1/3
// **/
#define PHY_UMTS_16QAM_R_1_3  2

// /**
// CONSTANT    :: PHY_UMTS_16QAM_R_1_2
// DESCRPTION  :: coding cheme: 16QAM 1/2
// **/
#define PHY_UMTS_16QAM_R_1_2  3

// /**
// CONSTANT    :: PHY_UMTS_DEFAULT_SPREAD_FACTOR
// DESCRIPTION :: default spred factor
// **/
#define PHY_UMTS_DEFAULT_SPREAD_FACTOR 1.0

// /**
// CONSTANT    :: PHY_UMTS_BRANCH_FACTOR
// DESCRIPTION :: Brach factor in UMTS
// **/
#define PHY_UMTS_BRANCH_FACTOR         2.0

// /**
// CONSTANT    :: PHY_UMTS_DEFAULT_MODULATION_TYPE
// DESCRIPTION :: default modulation scheme
// **/
#define PHY_UMTS_DEFAULT_MODULATION_TYPE  QPSK

// /**
// CONSTANT    :: PHY_UMTS_DEFAULT_CODING_RATE
// DESCRIPTION :: default coding rate
// **/
#define PHY_UMTS_DEFAULT_CODING_RATE  encodingRate_1_2

// /**
// CONSTANT    :: PHY_UMTS_DEFAULT_MODULATION_ENCODING
// DESCRIPTION :: Default modulation and coding scheme
// **/
#define PHY_UMTS_DEFAULT_MODULATION_ENCODING  PHY_UMTS_QPSK_R_1_2

// /**
// CONSTANT
// DESCRIPTION :: Some constants defined for radio Tx/Rx
//                including sensitivity, implementation loss,
//                noise factor, threshold shift,
//                Sensitivity for channel with 1 MHz bandwidth
// **/
#define PHY_UMTS_DEFAULT_SENSITIVITY_QPSK_R_1_3    -99
#define PHY_UMTS_DEFAULT_SENSITIVITY_QPSK_R_1_2    -97
#define PHY_UMTS_DEFAULT_SENSITIVITY_16QAM_R_1_3   -93
#define PHY_UMTS_DEFAULT_SENSITIVITY_16QAM_R_1_2   -91

#define PHY_UMTS_NUM_DATA_RATES                    4
#define PHY_UMTS_SENSITIVITY_ADJUSTMENT_FACTOR     60.0
#define PHY_UMTS_IMPLEMENTATION_LOSS        0.0    //in dB
#define PHY_UMTS_NOISE_FACTOR_REFERENCE     11.0   // in dB
#define UMTS_THRESHOLD_SHIFT                15.0   // in dB

// /**
// CONSTANT    :: PHY_UMTS_CQI_REPORT_TAU
// DESCRIPTION :: Parmeters controlingthe Cqi reporting mechanism
//                refer to stndard for more informaiton
// **/
#define PHY_UMTS_CQI_REPORT_TAU 10

// /**
// CONSTANT    :: UMTS_PSCH_CODE 0xfca9
// DESCRIPTION :: Primary synchronization code
//                psc = <x,x,x,-x,-x,x,x,x,x,x,x,-x,x,-x,x,x>
//                x =  0xfca9
//                psc = x to similify the process
const UInt16 UMTS_PSCH_CODE = 0xfca9;

// /**
// CONSTANT    :: UMTS_INVLAID_UL_SCRAMBLING_CODE_INDEX 2^24
// DESCRIPTION :: invalid UPlink scrambling code index
//                start from 0 to  2^24 -1
// **/
const int UMTS_INVLAID_UL_SCRAMBLING_CODE_INDEX = 2^24;

// /**
// CONSTANT    :: UMTS_INVLAID_DL_SCRAMBLING_CODE_INDEX 2^24
// DESCRIPTION :: invalid UPlink channel index
//                start from 0 to  2^18 -1
// **/
const int UMTS_INVLAID_DL_SCRAMBLING_CODE_INDEX = 2^18;

// /**
// CONSTANT    :: UMTS_MAX_DL_SCRAMBLING_CODE_SET 512
// DESCRIPTION :: invalid UPlink channel index
//                start from 0 to  2^18 -1
// **/
const int UMTS_MAX_DL_SCRAMBLING_CODE_SET = 512;

// /**
// CONSTANT    :: UMTS_INVLAID_CHANNEL_INDEX
// DESCRIPTION :: invalid channel index
// **/
const int UMTS_INVALID_CHANNEL_INDEX = -1;

// /**
// CONSTANT    :: UMTS_SSC_TABLE
// DESCRIPTION :: Secondary scrambiling code group sequence
// **/
const unsigned char
UMTS_SSC_TABLE[UMTS_MAX_SC_GROUP][UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME]
= {
    {1,  1,  2,  8,  9,  10, 15, 8,  10, 16, 2,  7,  15, 7,  16},
    {1,  1,  5,  16, 7,  3,  14, 16, 3,  10, 5,  12, 14, 12, 10},
    {1,  2,  1,  15, 5,  5,  12, 16, 6,  11, 2,  16, 11, 15, 12},
    {1,  2,  3,  1,  8,  6,  5,  2,  5,  8,  4,  4,  6,  3,  7},
    {1,  2,  16, 6,  6,  11, 15, 5,  12, 1,  15, 12, 16, 11, 2},
    {1,  3,  4,  7,  4,  1,  5,  5,  3,  6,  2,  8,  7,  6,  8},
    {1,  4,  11, 3,  4,  10, 9,  2,  11, 2,  10, 12, 12, 9,  3},
    {1,  5,  6,  6,  14, 9,  10, 2,  13, 9,  2,  5,  14, 1,  13},
    {1,  6,  10, 10, 4,  11, 7,  13, 16, 11, 13, 6,  4,  1,  16},
    {1,  6,  13, 2,  14, 2,  6,  5,  5,  13, 10, 9,  1,  14, 10},
    {1,  7,  8,  5,  7,  2,  4,  3,  8,  3,  2,  6,  6,  4,  5},
    {1,  7,  10, 9,  16, 7,  9,  15, 1,  8,  16, 8,  15, 2,  2},
    {1,  8,  12, 9,  9,  4,  13, 16, 5,  1,  13, 5,  12, 4,  8},
    {1,  8,  14, 10, 14, 1,  15, 15, 8,  5,  11, 4,  10, 5,  4},
    {1,  9,  2,  15, 15, 16, 10, 7,  8,  1,  10, 8,  2,  16, 9},
    {1,  9,  15, 6,  16, 2,  13, 14, 10, 11, 7,  4,  5,  12, 3},
    {1,  10, 9,  11, 15, 7,  6,  4,  16, 5,  2,  12, 13, 3,  14},
    {1,  11, 14, 4,  13, 2,  9,  10, 12, 16, 8,  5,  3,  15, 6},
    {1,  12, 12, 13, 14, 7,  2,  8,  14, 2,  1,  13, 11, 8,  11},
    {1,  12, 15, 5,  4,  14, 3,  16, 7,  8,  6,  2,  10, 11, 13},
    {1,  15, 4,  3,  7,  6,  10, 13, 12, 5,  14, 16, 8,  2,  11},
    {1,  16, 3,  12, 11, 9,  13, 5,  8,  2,  14, 7,  4,  10, 15},
    {2,  2,  5,  10, 16, 11, 3,  10, 11, 8,  5,  13, 3,  13, 8},
    {2,  2,  12, 3,  15, 5,  8,  3,  5,  14, 12, 9,  8,  9,  14},
    {2,  3,  6,  16, 12, 16, 3,  13, 13, 6,  7,  9,  2,  12, 7},
    {2,  3,  8,  2,  9,  15, 14, 3,  14, 9,  5,  5,  15, 8,  12},
    {2,  4,  7,  9,  5,  4,  9,  11, 2,  14, 5,  14, 11, 16, 16},
    {2,  4,  13, 12, 12, 7,  15, 10, 5,  2,  15, 5,  13, 7,  4},
    {2,  5,  9,  9,  3,  12, 8,  14, 15, 12, 14, 5,  3,  2,  15},
    {2,  5,  11, 7,  2,  11, 9,  4,  16, 7,  16, 9,  14, 14, 4},
    {2,  6,  2,  13, 3,  3,  12, 9,  7,  16, 6,  9,  16, 13, 12},
    {2,  6,  9,  7,  7,  16, 13, 3,  12, 2,  13, 12, 9,  16, 6},
    {2,  7,  12, 15, 2,  12, 4,  10, 13, 15, 13, 4,  5,  5,  10},
    {2,  7,  14, 16, 5,  9,  2,  9,  16, 11, 11, 5,  7,  4,  14},
    {2,  8,  5,  12, 5,  2,  14, 14, 8,  15, 3,  9,  12, 15, 9},
    {2,  9,  13, 4,  2,  13, 8,  11, 6,  4,  6,  8,  15, 15, 11},
    {2,  10, 3,  2,  13, 16, 8,  10, 8,  13, 11, 11, 16, 3,  5},
    {2,  11, 15, 3,  11, 6,  14, 10, 15, 10, 6,  7,  7,  14, 3},
    {2,  16, 4,  5,  16, 14, 7,  11, 4,  11, 14, 9,  9,  7,  5},
    {3,  3,  4,  6,  11, 12, 13, 6,  12, 14, 4,  5,  13, 5,  14},
    {3,  3,  6,  5,  16, 9,  15, 5,  9,  10, 6,  4,  15, 4,  10},
    {3,  4,  5,  14, 4,  6,  12, 13, 5,  13, 6,  11, 11, 12, 14},
    {3,  4,  9,  16, 10, 4,  16, 15, 3,  5,  10, 5,  15, 6,  6},
    {3,  4,  16, 10, 5,  10, 4,  9,  9,  16, 15, 6,  3,  5,  15},
    {3,  5,  12, 11, 14, 5,  11, 13, 3,  6,  14, 6,  13, 4,  4},
    {3,  6,  4,  10, 6,  5,  9,  15, 4,  15, 5,  16, 16, 9,  10},
    {3,  7,  8,  8,  16, 11, 12, 4,  15, 11, 4,  7,  16, 3,  15},
    {3,  7,  16, 11, 4,  15, 3,  15, 11, 12, 12, 4,  7,  8,  16},
    {3,  8,  7,  15, 4,  8,  15, 12, 3,  16, 4,  16, 12, 11, 11},
    {3,  8,  15, 4,  16, 4,  8,  7,  7,  15, 12, 11, 3,  16, 12},
    {3,  10, 10, 15, 16, 5,  4,  6,  16, 4,  3,  15, 9,  6,  9},
    {3,  13, 11, 5,  4,  12, 4,  11, 6,  6,  5,  3,  14, 13, 12},
    {3,  14, 7,  9,  14, 10, 13, 8,  7,  8,  10, 4,  4,  13, 9},
    {5,  5,  8,  14, 16, 13, 6,  14, 13, 7,  8,  15, 6,  15, 7},
    {5,  6,  11, 7,  10, 8,  5,  8,  7,  12, 12, 10, 6,  9,  11},
    {5,  6,  13, 8,  13, 5,  7,  7,  6,  16, 14, 15, 8,  16, 15},
    {5,  7,  9,  10, 7,  11, 6,  12, 9,  12, 11, 8,  8,  6,  10},
    {5,  9,  6,  8,  10, 9,  8,  12, 5,  11, 10, 11, 12, 7,  7},
    {5,  10, 10, 12, 8,  11, 9,  7,  8,  9,  5,  12, 6,  7,  6},
    {5,  10, 12, 6,  5,  12, 8,  9,  7,  6,  7,  8,  11, 11, 9},
    {5,  13, 15, 15, 14, 8,  6,  7,  16, 8,  7,  13, 14, 5,  16},
    {9,  10, 13, 10, 11, 15, 15, 9,  16, 12, 14, 13, 16, 14, 11},
    {9,  11, 12, 15, 12, 9,  13, 13, 11, 14, 10, 16, 15, 14, 16},
    {9,  12, 10, 15, 13, 14, 9,  14, 15, 11, 11, 13, 12, 16, 10}
};

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


/*
 * Structure for phy statistics variables
 */

// /**
// STRUCT      :: PhyUmtsStats
// DESCRIPTION :: statistics in general phy umts
// **/
typedef struct phy_umts_stats_str {
    UInt32 totalTxSignals;
    UInt32 totalRxSignalsToMac;
    UInt32 totalSignalsLocked;
    UInt32 totalSignalsWithErrors;
    UInt32 totalRxBurstsToMac;
    UInt32 totalRxBurstsWithErrors;
    UInt32 totalUlBurstsDropped;
    Float64 energyConsumed;
    clocktype turnOnTime;
}PhyUmtsStats;

// /**
// STRUCT      :: PhyUmtsCdmaBurstInfo
// DESCRIPTIOn :: the informaiton about the PHY CH
//
typedef struct phy_umts_cdma_burst_str
{
    UmtsPhysicalChannelType phChType;
    UmtsChannelDirection chDirection;
    UmtsSpreadFactor spreadFactor;
    unsigned int chCodeIndex;  // optinal sometimes
    unsigned int scCodeIndex;
    UmtsPhyChRelativePhase chPhase;
    double gainFactor;
    UmtsModulationType moduType;
    UmtsCodingType codingType;

    // actual time information derived from signal arrival time
    clocktype burstStartTime;  // start time of the burst
    clocktype burstEndTime;    // end time as of the burst
    clocktype rxTimeEvaluated;  // rx evaluted time of the burst
    clocktype rxStartTime;     // rx start time of the burst
    clocktype rxEvaluatedEndTime; // rx evaluated end time of the burst

    // Phy Header description
    BOOL phyHeader; // TRUE if with header otherwise FALSE
    unsigned int headerLen;

    // reception info
    BOOL      inError;    // whether this burst has error
    double    cinr;       // CINR of this burst
    double interferencePower_mW;
    double burstPower_mW;

    // We use the nodeBId to represent the time synchrozation info in
    // a arrival signal, so we can simplify the
    // simulation of match filter.
    NodeAddress syncTimeInfo;

    // the folowing is to simulate the PRACH/AICH preamble
    BOOL prachPreamble;
    unsigned char preSlot;
    unsigned char preSig;

    // the following simulating the power control
    BOOL dlDpcchOnly;
    signed char pcCmd;
    unsigned char slotFormat;

    // hsdpa related
    NodeAddress hsdpaUeId;
    unsigned char cqiVal;

}PhyUmtsCdmaBurstInfo;

// /**
// STRUCT      :: PhyUmtsRxMsgInfo
// DESCRIPTION :: rx messge info at UMTS PHY
// **/
typedef struct phy_umts_rx_msg_info_str
{
    // information identifies the signal
    PropRxInfo* propRxInfo;// propagation receiving info structure
    Message* rxMsg;        // the packet associated with the signal

    // signal reception info
    clocktype rxBeginTime; // signal arrival time
    clocktype rxEndTime;   // end time of the signal
    double rxburstPower_mW;  // receiving signal power
    BOOL isDownlink;       // whether this signal is DL tx or UL tx
    BOOL inError;          // whether this signal is totally in error

    // burst information
    int numBursts;         // # of bursts included in this tx

    struct phy_umts_rx_msg_info_str* next;
}PhyUmtsRxMsgInfo;


// /**
// STRUCT      :: PhyUmtsChannelData
// DESCRIPTION :: Physical Channel data of the cellular UMTS
// **/
struct PhyUmtsChannelData
{
    // channel Index is the index inside the same type
    // channelIndex, channelType, chDirection detemine the channel
    unsigned char channelIndex;
    UmtsPhysicalChannelType phChType;
    UmtsChannelDirection chDirection;
    UmtsSpreadFactor spreadFactor;
    unsigned int chCodeIndex;  // optinal sometimes
    unsigned int scCodeIndex;
    UmtsPhyChRelativePhase chPhase;
    double gainFactor;
    UmtsModulationType moduType;

    UmtsCodingType codingType;

    // more channel dependent info
    unsigned char slotFormatInUse;
    void* moreChInfo;
    std::list<Message*>* txMsgList;    //Transmission msg List

    // HSDPA related
    // only HSDPA channel set this value
    NodeAddress targetNodeId;

    PhyUmtsChannelData(unsigned char chIndex,
                       UmtsPhysicalChannelType chType,
                       UmtsChannelDirection chDir,
                       UmtsSpreadFactor sf,
                       unsigned int chCode,
                       unsigned int scCode,
                       UmtsPhyChRelativePhase phase,
                       double gf,
                       UmtsModulationType mdType,
                       UmtsCodingType coding)
    {
        channelIndex = chIndex;
        phChType = chType;
        chDirection = chDir;
        spreadFactor = sf;
        chCodeIndex = chCode;
        scCodeIndex = scCode;

        if (chDir == UMTS_CH_UPLINK)
        {
            chPhase = phase;
        }
        else
        {
            chPhase = UMTS_PHY_CH_BRANCH_IQ;
        }
        gainFactor = gf;
        moduType = mdType;
        codingType = coding;
        moreChInfo = NULL;
        txMsgList = new std::list<Message*>;
        slotFormatInUse = 0;
        targetNodeId = 0;

        if (phChType == UMTS_PHYSICAL_DPDCH &&
            chDirection  == UMTS_CH_UPLINK)
        {
            switch(spreadFactor)
            {
                case UMTS_SF_4:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_4;
                    break;
                }
                case UMTS_SF_8:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_8;
                    break;
                }
                case UMTS_SF_16:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_16;
                    break;
                }
                case UMTS_SF_32:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_32;
                    break;
                }
                case UMTS_SF_64:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_64;
                    break;
                }
                case UMTS_SF_128:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_128;
                    break;
                }
                case UMTS_SF_256:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_256;
                    break;
                }
                default:
                {
                    // SF_1, SF_2, SF_512 not used
                    break;
                }
            }
        }
        else if ((phChType == UMTS_PHYSICAL_DPDCH||
                  phChType == UMTS_PHYSICAL_DPCCH) &&
                  chDirection  == UMTS_CH_DOWNLINK)
        {
            switch(spreadFactor)
            {
                case UMTS_SF_4:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_4;
                    break;
                }
                case UMTS_SF_8:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_8;
                    break;
                }
                case UMTS_SF_16:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_16;
                    break;
                }
                case UMTS_SF_32:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_32;
                    break;
                }
                case UMTS_SF_64:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_64;
                    break;
                }
                case UMTS_SF_128:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_128;
                    break;
                }
                case UMTS_SF_256:
                {
                    slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_256;
                    break;
                }
                case UMTS_SF_512:
                {
                     slotFormatInUse =
                        (unsigned char)UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_512;
                    break;
                }
                default:
                {
                    // SF_1, SF_2 not used
                    break;
                }
            }
        }
        else if (phChType == UMTS_PHYSICAL_PRACH)
        {
            switch (spreadFactor)
            {
                case UMTS_SF_32:
                {
                    slotFormatInUse = 3;
                    break;
                }
                case UMTS_SF_64:
                {
                    slotFormatInUse = 2;
                    break;
                }
                case UMTS_SF_128:
                {
                    slotFormatInUse = 1;
                    break;
                }
                case UMTS_SF_256:
                {
                    slotFormatInUse = 0;
                    break;
                }
                default:
                {
                    slotFormatInUse = 0;
                    break;
                }
            }
        }
        // ohter channels use SF_256 which use slotFormat 0 or has
        // no slot format defined
    }

    ~PhyUmtsChannelData()
    {
        if (moreChInfo)
        {
            MEM_free(moreChInfo);
        }
        delete txMsgList;
    }

    void SetSlotFormat(unsigned char newSlotFormat)
    {
        slotFormatInUse = newSlotFormat;
    }
    unsigned char GetSlotFormat()
    {
        return slotFormatInUse;
    }
    void SetSpreadFactor(UmtsSpreadFactor sf)
    {
        spreadFactor = sf;
    }
    UmtsSpreadFactor GetSpreadFactor()
    {
        return spreadFactor;
    }
    void SetGainFactor(double gf)
    {
        gainFactor = gf;
    }
    double GetGainFactor()
    {
        return gainFactor;
    }
    void SetRelativePhase(UmtsPhyChRelativePhase phase)
    {
        chPhase = phase;
    }
    UmtsPhyChRelativePhase GetRelativePhase()
    {
        return chPhase;
    }
    void SetModulationType(UmtsModulationType mdType)
    {
        moduType = mdType;
    }
    UmtsModulationType GetModulationType()
    {
        return moduType;
    }
    void SetCodingType(UmtsCodingType coding)
    {
        codingType = coding;
    }
    UmtsCodingType GetCodingType()
    {
        return codingType;
    }
    void SetTargetNodeId(NodeAddress nodeId)
    {
        targetNodeId = nodeId;
    }
    NodeAddress GetTargetNodeId()
    {
        return targetNodeId;
    }
};

// /**
// STRUCT      :: PhyUmtsUlPowerControl
// DESCRPTION  :: Power Control Information for UL link
// **/
struct PhyUmtsUlPowerControl
{
    PhyUmtsPowerControlAlgorithm pca;
    unsigned char stepSize; // 1 or 2dB for Algo. 1,
                            // Algo 2 uses 2dB
    signed char deltaPower;
    BOOL needAdjustPower;
    PhyUmtsUlPowerControl()
    {
        pca = UMTS_POWER_CONTROL_ALGO_1;
        stepSize = 1;
        needAdjustPower = FALSE;
    }
};

// /**
// STRUCT      :: PhyUmtsDllPowerControl
// DESCRPTION  :: Power Control Information for DL link
// **/
struct PhyUmtsDlPowerControl
{
    PhyUmtsPowerControlAlgorithm pca;
    unsigned char stepSize; // 1 or 2dB for Algo. 1,
                            // Algo 2 uses 2dB
    signed char deltaPower;
    BOOL needAdjustPower;
    PhyUmtsDlPowerControl()
    {
        pca = UMTS_POWER_CONTROL_ALGO_1;
        stepSize = 1;
        needAdjustPower = FALSE;
    }
};

// /**
// STRUCT      :: PhyUmtsUeNodeBInfo
// DESCRIPTION :: NodeB data of the cellular UMTS at UE
// **/
struct PhyUmtsUeNodeBInfo
{
    unsigned int sscCodeGroupIndex;
    unsigned int primaryScCode;     // the same as cell ID
    double cpichRscp; // CPICH RSCP
    double cpichEcNo; // CPICH Ec/No
    clocktype lastReportTime;
    unsigned int sfnPccpch;
    UmtsUeCellStatus cellStatus;
    BOOL synchronized;

    // power control
    PhyUmtsUlPowerControl *ulPowCtrl; // for ul powCtrl
    BOOL needDlPowerControl; // need send control command for dl powCtrl
    PhyUmtsPowerControlCommand dlPowCtrlCmd; // cmd to send
    PhyUmtsDpcMode dpcMode;  // dl power control mode

    //downlink DPDCH information
    std::list<UmtsRcvPhChInfo*>* dlPhChInfoList;

    // unsigned int cellId;
    PhyUmtsUeNodeBInfo(int scCodeIndex): primaryScCode(scCodeIndex)
    {
        sscCodeGroupIndex = scCodeIndex / 128;
        dlPhChInfoList = new std::list<UmtsRcvPhChInfo*>;
        ulPowCtrl = new PhyUmtsUlPowerControl;
    }

    ~PhyUmtsUeNodeBInfo()
    {
        std::for_each(dlPhChInfoList->begin(),
                      dlPhChInfoList->end(),
                      UmtsFreeStlItem<UmtsRcvPhChInfo>());
        delete dlPhChInfoList;
        delete ulPowCtrl;
    }
};

// /**
// STRUCT      :: PhyUmtsUeStats
// DESCRIPTION :: stat data of the cellular UMTS at UE
// **/
typedef struct
{
    int totalTxSignals;
    double energyConsumed;

    // dynamic statistics
    int txPowerGuiId;
}PhyUmtsUeStats;

// /**
// STRUCT      :: UeULDediPhChConfig
// DESCRIPTION :: UL dedicated physical channel config at UE
// **/
typedef struct
{
    unsigned char numDPDCH;
    unsigned char numHsDPCCH;
    unsigned char numEDPDCH;
    unsigned char numEDPCCH;
}UeULDediPhChConfig;

// /**
// STRUCT      :: UeCellSearchInfo
// DESCRIPTION :: Cell search info
// **/
typedef struct
{
    int slotCount;

    // we use the nodeBId to represent the time synchrozation info in
    // a arrival signal, so we can simplify the simulation of match filter.
    NodeAddress syncTimeInfo;

    BOOL signalLocked;
    BOOL ueSync;
    UInt16 secScInfo[UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME];
    BOOL scGroupDetected;
    BOOL priScDetected;
}UeCellSearchInfo;

// /**
// STRUCT      :: PhyUmtsUePrachInfo
// DESCRPTION  :: PRACH  info
// **/
typedef UmtsCphyChDescPRACH PhyUmtsUePrachInfo;

// /**
// STRUCT      :: UmtsPhyRandomAccessInfo
// DESCRIPTION :: random access procedure at PHY
// **/
typedef struct
{
    // config info from MAC
    unsigned char retryMax;
    UInt16 assignedSubCh;
    unsigned char preambleSig;
    unsigned char preambleSlot;
    unsigned char sigStartIndex;
    unsigned char sigEndIndex;

    // active info
    BOOL needSendRachPreamble;
    BOOL waitForAich;
    BOOL needSendRachData;
    unsigned char rachPreambleWaitTime; // count in slot
    unsigned char rachDataWaitTime; // count in slot
    unsigned char aichWiatTime;
    unsigned char retryCount;
    unsigned char ascIndex;
    double cmdPrePower;
}UmtsPhyRandomAccessInfo;

// /**
// STRUCT      :: PhyUmtsUeHsdpaInfo
// DESCRIPTION :: HSDPA related information at UE pHY
// **/
typedef struct
{
    BOOL rcvdHspdschSig;
    BOOL repCqiNeeded;
    double lastRepCqiVal;
    clocktype repPerioid;
    clocktype lastRepTime;
    unsigned int ServHsdpaNodeBPriScCode;
}PhyUmtsUeHsdpaInfo;

// /**
// STRUCT      :: PhyUmtsUeData
// DESCRIPTION :: PHY data of the cellular UMTS at UE
// **/
typedef struct
{
    // scrambling code in use
    unsigned int priScCodeInUse;
    unsigned int secScCodeInUse;

    // cell search
    BOOL cellSearchNeeded;
    UeCellSearchInfo ueCellSearch;
    std::vector <NodeAddress> *syncTimeInfo;


    // ramdom access channel info
    // build when recv ACCESS-REQ from MAC
    // remove when exit the random procedure
    // in three cases:
    // 1. exit with  no ack on AICH
    // 2. exit with NACK on AICH
    // 3. exit with rach msg transmitted
    UmtsPhyRandomAccessInfo* raInfo;

    //spread factor for dedicated PHY CH
    UmtsSpreadFactor dpchSfInUse;
    int numDataBitsInBuffer;

    // PHY channel configuration
    // L3 will singalling the value after rcvd BCH signal
    UeULDediPhChConfig ulDediPhChConfig;

    // PhyUmtsUePrachInfo* prachInfo;
    std::list <PhyUmtsChannelData*>* phyChList;

    std::list <PhyUmtsUeNodeBInfo*>* ueNodeBInfo;
    //std::list <PhyUmtsUeNodeBInfo*>* detectedNodeBInfo;
    //std::list <PhyUmtsUeNodeBInfo*>* monitoredNodeBInfo;
    //std::list <PhyUmtsUeNodeBInfo*>* activeNodeBInfo;

    // UE measurement
    double utraCarrierRssi;

    // power control
    double prachSucPower;
    double targetUlSir;
    double targetDlSir;

    PhyUmtsPowerControlAlgorithm pca;
    unsigned char stepSize; // 1 or 2dB for Algo. 1,
                            // Algo 2 uses 2dB

    // HSDPA
    PhyUmtsUeHsdpaInfo hsdpaInfo;
    PhyUmtsUeStats stats;
}PhyUmtsUeData;

// /**
// STRUCT      :: PhyUmtsNodeBUeCqiRepInfo
// DESCRIPTION :: Dat structure to store the CQI info for UE at nodeB
typedef struct
{
    unsigned char repVal;
    clocktype lastRepTime;
    clocktype repPeriod;
}PhyUmtsNodeBUeCqiRepInfo;

// /**
// STRUCT      :: PhyUmtsNodeBUeInfo
// DESCRIPTION :: UE Info (PHY related at NodeB)
// **/
struct PhyUmtsNodeBUeInfo
{
    // UE identity
    NodeId ueId;
    unsigned int priUlScAssigned;
    double targetUlSir;
    double targetDlSir;

    PhyUmtsNodeBUeCqiRepInfo cqiInfo;

    // power control
    PhyUmtsDlPowerControl *dlPowCtrl; // for Dl powCtrl
    BOOL powCtrlAftCellSwitch;
    BOOL needUlPowerControl; // need send control command for ul powCtrl
    PhyUmtsPowerControlCommand ulPowCtrlCmd; // cmd to send
    int ulPowCtrlSteps;

    BOOL        isSelfPrimCell;     //whether this cell is the primary cell

    // dedicated channel for this UE
    // std::list <PhyUmtsChannelData*>* phyChList;
    PhyUmtsNodeBUeInfo(NodeId id,
                       UInt32 scCode,
                       BOOL primCell)
        :ueId(id), priUlScAssigned(scCode), isSelfPrimCell(primCell)
    {
        targetUlSir = PHY_UMTS_DEFAULT_TARGET_SIR;
        targetDlSir = PHY_UMTS_DEFAULT_TARGET_SIR;

        dlPowCtrl = new PhyUmtsDlPowerControl;

        needUlPowerControl = FALSE;
        ulPowCtrlCmd = UMTS_POWER_CONTROL_CMD_DEC;
        powCtrlAftCellSwitch = FALSE;
        ulPowCtrlSteps = 0;

    }
    ~PhyUmtsNodeBUeInfo()
    {
        delete dlPowCtrl;
    }
};

// /**
// STRUCT      :: PhyUmtsNodeBStats
// DESCRIPTION :: stat data of the cellular UMTS at NodeB
// **/
typedef struct
{
    int totalTxSignals;
    double energyConsumed;

    // dynamic statistics
    int txPowerGuiId;
    int numHsdpaChInUseGuiId;
}PhyUmtsNodeBStats;

// /**
// ENUM        :: PhyUmtsNodeBSigRcvStatus
// DESCRIPTION :: sig staus
// **/
typedef enum
{
    PHY_UMTS_SIG_NOT_RCVD = 0, // not used in the slot
    PHY_UMTS_SIG_SUCC , // success
    PHY_UMTS_SIG_COLLISION, // more than two UE using the same Sig
    PHY_UMTS_SIG_ERROR, // need boost Tx power
}PhyUmtsNodeBSigRcvStatus;

// /**
// STRUCT      :: PhyUmtsNodeBPrachAichInfo
// DESCRIPTION :: info for prach and aich
// **/
typedef struct
{
    PhyUmtsNodeBSigRcvStatus sigRcvInfo[16];
    BOOL aichMsgNeeded;
    unsigned char rcvdSlotIndex;
}PhyUmtsNodeBPrachAichInfo;

// /**
// STRUCT      :: PhyUmtsNodeBPrachAichInfo
// DESCRIPTION :: info for prach and aich
// **/
typedef struct
{
   unsigned char numHspdschAlloc;
   unsigned int hspdschChId[15];
   int numBitsInBuffer[15];

   // assume only one HS-SCCH  allocated
   unsigned char numHsscchAlloc;
   unsigned int hsscchChId[15];
   int numActiveCh;
}PhyUmtsNodeBHsdpaConfig;

// /**
// STRUCT      :: PhyUmtsNodeBData
// DESCRIPTION :: PHY data of the cellular UMTS at NodeB
// **/
typedef struct
{
    unsigned int pscCodeIndex;  // Primary  scrambling code Index,
                                // same as cellId
    unsigned int sscCodeGroupIndex;
    UInt16 sscCodeSeq[UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME];

    double reference_threshold_dB;

    // system frame numer
    unsigned int sfnPccpch;

    // prahc,aich info
    PhyUmtsNodeBPrachAichInfo prachAichInfo;

    // this is the phCH list for all phyCHs
    std::list <PhyUmtsChannelData*>* phyChList;

    // phy Ch info for each UE associate with this NodeB
    std::list <PhyUmtsNodeBUeInfo*>* ueInfo;

    // HSDPA config info
    PhyUmtsNodeBHsdpaConfig hsdpaConfig;

    PhyUmtsNodeBStats stats;
}PhyUmtsNodeBData;

// /**
// STRUCT      :: PhyUmtsData
// DESCRIPTION :: PHY data of the cellular UMTS
// **/

typedef struct str_phy_umts_data
{
    PhyUmtsUeData* phyDataUe;
    PhyUmtsNodeBData* phyDataNodeB;

    unsigned char slotIndex;
    double noisePower_mW;

    PhyData*  thisPhy;

    // some general parameters
    double   channelBandwidth;

    clocktype rxTxTurnaroundTime;

    // tx parameters
    D_Float32 txPower_dBm;
    D_Float32 maxTxPower_dBm;
    D_Float32 minTxPower_dBm;

    //float     txDefaultPower_dBm;

    // rx parameters
    int       rxDataRateType;
    double    rxSensitivity_mW[PHY_UMTS_NUM_DATA_RATES];

    // data rate for different FEC coding modulation combinations
    int       dataRate[PHY_UMTS_NUM_DATA_RATES];
    int       numDataRates;

    int numTxMsgs;
    int numRxMsgs;

    PhyUmtsRxMsgInfo* rxMsgList;  // list of receiving signals
    PhyUmtsRxMsgInfo* ifMsgList;  // list of interference signals
    clocktype rxTimeEvaluated;
    BOOL radiolinkFailure;
    BOOL synchronisation;
    PhyUmtsStats  stats;

    BOOL hspdaEnabled;
    UmtsChipRateType chipRateType;
    UmtsDuplexMode duplexMode;
    int   chipRate;
    double    rxLev;    //received signal power in dB
}PhyUmtsData;

//--------------------------------------------------------------------------
//  Key API functions of the UMTS PHY
//--------------------------------------------------------------------------

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
                  const NodeInput *nodeInput);

// /**
// FUNCTION   :: PhyUmts6Finalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the UMTS PHY, print out statistics
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : const int : Index of the PHY
// RETURN     :: void      : NULL
// **/
void PhyUmtsFinalize(Node *node, const int phyIndex);

// /**
// FUNCTION   :: PhyUmtsStartTransmittingSignal
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + packet : Message* : Frame to be transmitted
// + duration : clocktype : Duration of the transmission
// + useMacLayerSpecifiedDelay : BOOL : Use MAC specified delay or calculate it
// + initDelayUntilAirborne    : clocktype : The MAC specified delay
// RETURN     :: void : NULL
// **/
void PhyUmtsStartTransmittingSignal(
         Node* node,
         int phyIndex,
         Message* packet,
         clocktype duration,
         BOOL useMacLayerSpecifiedDelay,
         clocktype initDelayUntilAirborne);

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
         int phyIndex);

// /**
// FUNCTION   :: PhyUmtsSignalArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the chanel
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
         PropRxInfo* propRxInfo);

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
         PropRxInfo* propRxInfo);

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
            Message* cmd);

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
void PhyUmtsSetTransmitPower(Node *node, int phyIndex, double txPower_mW);

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
void PhyUmtsGetTransmitPower(Node *node, int phyIndex, double *txPower_mW);

// /**
// FUNCTION   :: PhyUmtsSetBerTable
// LAYER      :: PHY
// PURPOSE    :: Set teh BER table for UMTS
// PARAMETERS ::
// + thisPhy       : PhyData*   : Pointer to PHY data.
// RETURN     ::  void    : NULL
// **/
void PhyUmtsSetBerTable(PhyData* thisPhy);

// /**
// FUNCTION   :: PhyUmtsGetModCodeType
// LAYER      :: PHY
// PURPOSE    :: Get the ModcodeType and the coding rate.
// PARAMETERS ::
// + codingType      : UmtsCodingType          : coding type.
// + moduType        : UmtsModulationType      : modulation type
// + codingRate      : double*                 : coding rate
// RETURN     :: int : modCodeType
// **/
int PhyUmtsGetModCodeType(UmtsCodingType codingType,
                          UmtsModulationType moduType,
                          double *codingRate);

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
         double sigPower_mW);

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
         PropRxInfo* propRxInfo);

#endif /* _PHY_UMTS_H_ */
