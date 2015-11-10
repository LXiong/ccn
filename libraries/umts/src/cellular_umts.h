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
// PACKAGE     :: CELLULAR_UMTS
// DESCRIPTION :: Defines constants, enums, structures used in the UMTS Model
//                This file contains the basic ones shared by different
//                layers.
// **/

#ifndef _CELLULAR_UMTS_H_
#define _CELLULAR_UMTS_H_

#include <list>
#include <bitset>
#include <algorithm>


#include "cellular.h"
#include "umts_qos.h"
#include <iostream>
using namespace std;
///////////////////////////////////////////////////////////////////////////
// CONSTANT
///////////////////////////////////////////////////////////////////////////

// //////
// DYNAMIC STATISTICS
//
// /**
// CONSTANT    :: UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW
// DESCRIPTION :: time window for the statistics calculation
// **/
#define UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW     2 * SECOND

// /**
// ENUM        :: UmtsTimeWindowStatAvgType
// DESCRITION  :: Type of average
// **/
typedef enum 
{
    // add all then devide by the number
    UMTS_STAT_TIME_WINDOW_AVG_NUMBER = 0,
    // add all then devide by time, useful for throughout
    UMTS_STAT_TIME_WIMDOW_AVG_TIME = 1, 
}UmtsTimeWindowStatAvgType;

// /////
// APPLICATION
// ////

// /**
// CONSTANT    :: UMTS_MAX_APP_CLASS_TYPE 7
// DESCRIPTION :: How many differnt type applicaiton classfier in the syste,
//                Phone call,CONVERSATIONAL,STREAMING, INTERACTIVE 1,
//                INTERACTIVE 2, INTERACTIVE 3, and background
//                It is different from QoS Traffic Class type
#define  UMTS_MAX_APP_CLASS_TYPE 7

// /**
// CONSTANT    :: UMTS_APP_DEFAULT_PORT 0
// DESCRITPION :: Port number used for phone call when
//                classifying the application
// **/
#define  UMTS_APP_DEFAULT_PORT 0

//////////
// Identity
//////////
// /**
// CONSTANT    :: UMTS_MAX_PACKET_BUFFER_SIZE : 512
// DESCRIPTION :: Max size of a UMTS packet buffer for building a packet
// **/
#define UMTS_MAX_PACKET_BUFFER_SIZE    512

// /**
// CONSTANT    :: CELLULAR_IMSI_LENGTH : 15
// DESCRIPTION :: Defines the length of IMSI in this implementation
// **/
#define CELLULAR_IMSI_LENGTH    15

// /**
// CONSTANT    :: CELLULAR_COMPACT_IMSI_LENGTH : 8
// DESCRIPTION :: Length of compact IMSI where half byte for one digit
// **/
#define CELLULAR_COMPACT_IMSI_LENGTH    8

// /**
// CONSTANT    :: CELLULAR_MCC_LENGTH : 3
// DESCRIPTION :: Length of the Mobile Contry Code (MCC)
// **/
#define CELLULAR_MCC_LENGTH    3

// /**
// CONSTANT    :: CELLULAR_MNC_LENGTH : 3
// DESCRIPTION :: Length of the Mobile Network Code (MNC)
// **/
#define CELLULAR_MNC_LENGTH    3

// /**
// CONSTANT    :: CELLULAR_MSIN_LENGTH : 9
// DESCRIPTION :: Length of the MSIN of IMSI
// **/
#define CELLULAR_MSIN_LENGTH   \
    (CELLULAR_IMSI_LENGTH - CELLULAR_MCC_LENGTH - CELLULAR_MNC_LENGTH)

// /**
// CONSTANT    :: CELLULAR_MCC_MNC_LENGTH : 3
// DESCRIPTION :: Compact format for the MCC and MNC together using 3 bytes
//                First 1.5 bytes is MCC field and second half is MNC filed
// **/
#define CELLULAR_MCC_MNC_LENGTH    3

////////////////
//Radio Bearer
////////////////
// /**
// CONSTANT    :: UMTS_CCCH_RB_ID = 0
// DESCRIPTION :: the bearer Id for CCCH (RACH(UL)/FACH(DL))
// **/
const unsigned char UMTS_CCCH_RB_ID = 0;

// /**
// CONSTANT    :: UMTS_SRB1_ID = 1
// DESCRIPTION :: the ID of signalling RB 1
// **/
const unsigned char UMTS_SRB1_ID = 1;

// /**
// CONSTANT    :: UMTS_SRB2_ID = 2
// DESCRIPTION :: the ID of signalling RB 2
// **/
const unsigned char UMTS_SRB2_ID = 2;

// /**
// CONSTANT    :: UMTS_SRB3_ID = 3
// DESCRIPTION :: the ID of signalling RB 3
// **/
const unsigned char UMTS_SRB3_ID = 3;

// /**
// CONSTANT    :: UMTS_SRB4_ID = 4
// DESCRIPTION :: the ID of signalling RB 4
// **/
const unsigned char UMTS_SRB4_ID = 4;

// /**
// CONSTANT    :: UMTS_RBINRAB_START_ID = 5
// DESCRIPTION :: the starting ID of a RB in a RAB
// **/
const unsigned char UMTS_RBINRAB_START_ID = 5;

// /**
// CONSTANT    :: UMTS_BCCH_RB_ID = 33
// DESCRIPTION :: the bearer Id for BCCH related
// **/
const unsigned char UMTS_BCCH_RB_ID = 33;

// /**
// CONSTANT    :: UMTS_PCCH_RB_ID = 34
// DESCRIPTION :: the bearer Id for PCCH related
// **/
const unsigned char UMTS_PCCH_RB_ID = 34;

// /**
// CONSTANT    :: UMTS_CTCH_RB_ID = 35
// DESCRIPTION :: the bearer Id for CTCH related
// **/
const unsigned char UMTS_CTCH_RB_ID = 35;

// /**
// CONSTANT    :: UMTS_SRB_DCH_ID
// DESCRIPTION :: DCH ID for signaling RBs at UE side
// **/
const char UMTS_SRB_DCH_ID = 0;

// /**
// CONSTANT    :: UMTS_SRB_DPDCH_ID
// DESCRIPTION :: DCH ID for signaling RBs at UE side
// **/
const char UMTS_SRB_DPDCH_ID = 0;

// /**
// CONSTANT    :: UMTS_INVALID_RAB_ID
// DESCRIPTION :: Invalid RAB ID
// **/
const char UMTS_INVALID_RAB_ID = -1;

// /**
// CONSTANT    :: UMTS_INVALID_RB_ID
// DESCRIPTION :: Invalid RB ID
// **/
const char UMTS_INVALID_RB_ID = -1;

// /**
// CONSTANT    :: UMTS_INVALID_TEID_ID
// DESCRIPTION :: Invalid TE ID
// **/
const UInt32 UMTS_INVALID_TE_ID = 0xFFFFFFFF;

// /**
// CONSTANT    :: UMTS_INVALID_CELL_ID
// DESCRIPTION :: Invalid cell ID
// **/
const UInt32 UMTS_INVALID_CELL_ID = 0xFFFFFFFF;

// /**
// CONSTANT    :: UMTS_INVALID_NODEB_ID
// DESCRIPTION :: Invalid cell ID
// **/
const UInt32 UMTS_INVALID_NODEB_ID = 0xFFFFFFFF;

// /**
// CONSTANT    :: UMTS_INVALID_REG_AREA_ID
// DESCRIPTION :: Invalid REGISTRATION AREA ID
// **/
const UInt16 UMTS_INVALID_REG_AREA_ID = 0;

///////////
//PHY
//////////

// /**
// CONSTANT FOR PHY
// **/
// /**
// CONSTANT    :: PHY_UMTS_MAX_TX_POWER_dBm
// DESCRIPTION :: Default value of maximum Tx power
// **/
#define PHY_UMTS_MAX_TX_POWER_dBm  30.0   //dBm

// /**
// CONSTANT    :: PHY_UMTS_MIN_TX_POWER_dBm
// DESCRIPTION :: Default value of minimum Tx power
// **/
#define PHY_UMTS_MIN_TX_POWER_dBm  -30.0  //dBm

// /**
// CONSTANT    :: PHY_UMTS_DEFAULT_TX_POWER_dBm
// DESCRIPTION :: Default value of Tx power
// **/
#define PHY_UMTS_DEFAULT_TX_POWER_dBm  10.0   // dbm

// /**
// CONSTANT    :: PHY_UMTS_DEFAULT_POWER_RAMP_STEP
// DESCRIPTION :: Default value of power ramp step
// **/
#define PHY_UMTS_DEFAULT_POWER_RAMP_STEP  3  // db

// /**
// CONSTANT    :: PHY_UMTS_DEFAULT_TARGET_SIR
// DESCRIPTION :: Default SIR target
// **/
#define PHY_UMTS_DEFAULT_TARGET_SIR 7 // dB

// /**
// CONSTANT    :: PHY_UMTS_DEFAULT_POWER_STEP_SIZE
// DESCRIPTION :: Default power control step size
// **/
#define PHY_UMTS_DEFAULT_POWER_STEP_SIZE 1

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_1
// DESCRIPTION :: gain factor coefficient for SF 1
// **/
#define PHY_UMTS_GAIN_FACTOR_1           1.00000

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_2
// DESCRIPTION :: gain factor coefficient for SF 2
// **/
#define PHY_UMTS_GAIN_FACTOR_2           0.50000

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_4
// DESCRIPTION :: gain factor coefficient for SF 4
// **/
#define PHY_UMTS_GAIN_FACTOR_4           0.25000

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_8
// DESCRIPTION :: gain factor coefficient for SF 8
// **/
#define PHY_UMTS_GAIN_FACTOR_8           0.12500

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_16
// DESCRIPTION :: gain factor coefficient for SF 16
// **/
#define PHY_UMTS_GAIN_FACTOR_16          0.06250

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_32
// DESCRIPTION :: gain factor coefficient for SF 32
// **/
#define PHY_UMTS_GAIN_FACTOR_32          0.03125

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_64
// DESCRIPTION :: gain factor coefficient for SF 64
// **/
#define PHY_UMTS_GAIN_FACTOR_64          0.01562

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_128
// DESCRIPTION :: gain factor coefficient for SF 128
// **/
#define PHY_UMTS_GAIN_FACTOR_128         0.00781

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_256
// DESCRIPTION :: gain factor coefficient for SF 256
// **/
#define PHY_UMTS_GAIN_FACTOR_256         0.00390

// /**
// CONSTANT    :: PHY_UMTS_GAIN_FACTOR_512
// DESCRIPTION :: gain factor coefficient for SF 512
// **/
#define PHY_UMTS_GAIN_FACTOR_512         0.00195

// /**
// CONSTANT    :: UMTS_MAX_SC_GROUP : 64
// DESCRIPTION :: MAX number of scrambling code grouop
// **/
const unsigned char UMTS_MAX_SC_GROUP = 64;

// /**
// CONSTANT    :: UMTS_CHIP_RTAE_128 : 1280000 (chip per second)
// DESCRIPTION :: Chip rate 1.28Mcps
// **/
const unsigned int UMTS_CHIP_RTAE_128 = 1280000;

// /**
// CONSTANT    :: UMTS_CHIP_RTAE_384 : 3840000 (chip per second)
// DESCRIPTION :: Chip rate 3.84Mcps
// **/
const unsigned int UMTS_CHIP_RTAE_384 = 3840000;

// /**
// CONSTANT    :: UMTS_CHIP_RTAE_768 : 7680000 (chip per second)
// DESCRIPTION :: Chip rate 7.68Mcps
// **/
const unsigned int UMTS_CHIP_RTAE_768 = 7680000;

// /**
// CONSTANT    :: UMTS_CHIP_DURATION_384 = 1 / 3840000;
// DESCRIPTION :: chip duration 1 second / 3840000
// **/
const clocktype UMTS_CHIP_DURATION_384 = (clocktype)((1 * SECOND)/3840000);

// /**
// CONSTANT    :: UMTS_SLOT_DURATION_384 = UMTS_CHIP_DURATION_384* 2560
// DESCRIPTION :: slot duration
// **/
const clocktype UMTS_SLOT_DURATION_384 = UMTS_CHIP_DURATION_384 * 2560;

// /**
// CONSTANT    :: UMTS_RADIO_FRAME_DURATION_384 =  15
// DESCRIPTION :: raido frame duration in SLOT DURATION
// **/
const char UMTS_RADIO_FRAME_DURATION_384 =  15;

// /**
// CONSTANT    :: UMTS_SUBFRAME_DURATION_384 = 3;
// DESCRIPTION :: subframe duration in SLOT DURATION
// **/
const clocktype UMTS_SUBFRAME_DURATION_384 =  3;

// /**
// CONSTANT    :: UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME_ =  15
// DESCRIPTION :: raido frame duration in SLOT DURATION
// **/
const unsigned char UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME =  15;

// /**
// CONSTANT    :: UMTS_PICH_TIMING_OFFSET = 7680 (unit chips)
// DESCRIPTION :: The timing offset between PICH and its associated SCCPCH
// **/
const unsigned int UMTS_PICH_TIMING_OFFSET = 7680;

// /**
// CONSTANT    :: UMTS_DEFUALT_SCCPCH_TIMING_PARAM_TK = 0 (unit chips)
// DESCRIPTION :: The timing offset between PCCPCH and this SCCPCH
// **/
const unsigned int UMTS_DEFUALT_SCCPCH_TIMING_PARAM_TK = 0;

// /**
// CONSTANT    :: UMTS_DEFUALT_DPCH_TIMING_PARAM_TN = 0 (unit chips)
// DESCRIPTION :: The timing offset between  PCCPCH and DPCH
// **/
const unsigned int UMTS_DEFUALT_DPCH_TIMING_PARAM_TN = 0;

// /**
// CONSTANT FOR RLC PARAMENTER
// **/
// START
// Default PDU SIZE
const unsigned int UMTS_RLC_DEF_PDUSIZE = 18;
// UM LI LEN
const unsigned int UMTS_RLC_DEF_UMDLLILEN = 7;
// Default UM MAX PDU SIZE
const unsigned int UMTS_RLC_DEF_UMMAXULPDUSIZE = 125;
// Default MAX DAT
const unsigned int UMTS_RLC_DEF_MAXDAT = 15;
// Deafult MAX RST
const unsigned int UMTS_RLC_DEF_MAXRST = 1;
// Default MAX WINDOWN SIZE
const unsigned int UMTS_RLC_DEF_WINSIZE = 32;
// Default MIN AM PDU SIZE
const unsigned int UMTS_RLC_MIN_AMPDUSIZE = 12;
// Default Rest Timer Interval
const clocktype UMTS_RLC_DEF_RSTTIMER_VAL = 300 * MILLI_SECOND;
// Default Poliing Timer Interval
const clocktype UMTS_RLC_DEF_POLLTIMER_DUR = 300*MILLI_SECOND;
// END

////////////////////////////////////////////////////////////////////////////
// Enums
///////////////////////////////////////////////////////////////////////////

// /**
// ENUM        :: UmtsPlmnType
// DESCRIPTION :: The type of a PLMN
// **/
typedef enum
{
    PLMN_GSM_MAP,    // European
    PLMN_ANSI_41,    // North America
    PLMN_GSM_ANSI    // Both GSM_MAP and ANSI_41
} UmtsPlmnType;

// /**
// ENUM        :: UmtsOperationMode
// DESCRIPTION :: UMTS Operation Mode
// **/
typedef enum
{
    UMTS_OP_MODE_PS = 0, // Packet Swtich Mode
    UMTS_OP_MODE_CS,     // Circuit Switch Mode
    UMTS_OP_MODE_PS_CS,  // PS/CS Mode
}UmtsOperationMode;

// /**
// ENUM        :: UmtsUeCellStatus
// DESCRIPTION :: UMTS cell monitoringdetect/active state
// **/
typedef enum
{
    UMTS_CELL_DETECTED_SET = 0,
    UMTS_CELL_MONITORED_SET,
    UMTS_CELL_ACT_TO_MONIT_SET,
    UMTS_CELL_MONIT_TO_ACT_SET,
    UMTS_CELL_ACTIVE_SET
}UmtsUeCellStatus;

// L2 related

// /**
// ENUM        :: UmtsLogicalChannelType
// DESCRIPTION :: UMTS Logical Channel Type
// Reference   :: 3GPP TS 25.321
// **/
typedef enum
{
    // Control Logical Channel
    UMTS_LOGICAL_BCCH = 0, // Broadcast Control CH
    UMTS_LOGICAL_PCCH,     // Paging Control CH
    UMTS_LOGICAL_DCCH,     // Dedicated COntrol CH
    UMTS_LOGICAL_CCCH,     // Common Chontrol CH
    UMTS_LOGICAL_SHCCH,    // Shared Common Control CH
    UMTS_LOGICAL_MCCH,     // MBMS point-to-point Control CH
    UMTS_LOGICAL_MSCH,     // MBMS point-to-point Scheduling CH

    // Logical Traffic Channel
    UMTS_LOGICAL_DTCH,     // Dedicated Traffic CH
    UMTS_LOGICAL_CTCH,     // Common Traffic CH
    UMTS_LOGICAL_MTCH      // MBMS point-to-point Traffic CH
}UmtsLogicalChannelType;

// /**
// ENUM:: UmtsTransportChannelType
// DESCRIPTION :: UMTS Transport Channel Type
// Reference   :: 3GPP TS 25.321
// **/
typedef enum
{
    // Control Transport Channel
    UMTS_TRANSPORT_RACH = 0, // Random Access CH
    UMTS_TRANSPORT_FACH,     // Forward Access CH
    UMTS_TRANSPORT_DSCH,     // DL Shared CH, TDD only
    UMTS_TRANSPORT_HSDSCH,   // High Speed DL Shared CH
    UMTS_TRANSPORT_USCH,     // UL Shared CH, TDD only
    UMTS_TRANSPORT_BCH,      // Broadcast CH
    UMTS_TRANSPORT_PCH,      // Paging CH

    // Dedicated Transport Channel
    UMTS_TRANSPORT_DCH,      // Dedicated CH
    UMTS_TRANSPORT_EDCH,     // Enhanced Dedicated CH,
                             // UL FDD and 3.84/7.68 Mcps TDD only

    UMTS_TRANSPORT_ACKCH     // used to recv ACK only,canned for imple.
}UmtsTransportChannelType;

//PHY related

// /**
// ENUM:: UmtsPhysicalChannelType
// DESCRIPTION :: UMTS Physical Channel Type
// Reference   :: 3GPP TS 25.321
// **/
typedef enum
{
    UMTS_PHYSICAL_DPDCH = 0, // Dedicated Physical Data CH
    UMTS_PHYSICAL_DPCCH,     // Dedicated Physical Control CH
    UMTS_PHYSICAL_FDPCH,     // Fractional Dedicated Physical CH
    UMTS_PHYSICAL_EDPDCH,    // E-DCH Dedicated Physical Data CH
    UMTS_PHYSICAL_EDPCCH,    // E-DCH Dedicated Phyical Control CH
    UMTS_PHYSICAL_EAGCH,     // E-DCH Absolute Grant CH
    UMTS_PHYSICAL_ERGCH,     // E-DCH Relative Grant CH
    UMTS_PHYSICAL_EHICH,     // E-DCH  Hybrid ARQ Indication CH
    UMTS_PHYSICAL_PRACH,     // Physical Random Access CH
    UMTS_PHYSICAL_CPICH,     // Common  Pilot Channel
    UMTS_PHYSICAL_PCCPCH,    // Primary Common Control Physical CH
    UMTS_PHYSICAL_SCCPCH,    // Secondary Common Control Physical CH
    UMTS_PHYSICAL_PSCH,      // Synchronization CH
    UMTS_PHYSICAL_SSCH,      // Secondary Synchronization CH
    UMTS_PHYSICAL_AICH,      // Acquisition Indication CH
    UMTS_PHYSICAL_PICH,      // Paging Indicator CH
    UMTS_PHYSICAL_MICH,      // MBMS Notification Indication CH
    UMTS_PHYSICAL_HSPDSCH,   // High Speed Physical DL Shared CH
    UMTS_PHYSICAL_HSSCCH,    // HS-DSCH-related Shared Control CH
    UMTS_PHYSICAL_HSDPCCH,    // Dedicated Phy Control CH (UL) for HS-DSCH
    UMTS_PHYSICAL_INVALID_TYPE
}UmtsPhysicalChannelType;

// /**
// ENUM:: UmtsDuplexMode
// DESCRIPTION :: UMTS Duplex Mode
// **/
typedef enum
{
    UMTS_DUPLEX_MODE_TDD = 0, // TDD mode
    UMTS_DUPLEX_MODE_FDD,     // FDD mode
}UmtsDuplexMode;

// /**
// ENUM:: UmtsModulationType
// DESCRIPTION :: UMTS modulation type
// **/
typedef enum
{
    UMTS_MOD_BPSK = 0,        // BPSK
    UMTS_MOD_QPSK,            // QPSK
    UMTS_MOD_8PSK,            // 8PSK
    UMTS_MOD_16QAM            // 16QAM
}UmtsModulationType;

// /**
// ENUM:: UmtsCodingType
// DESCRIPTION :: UMTS coding type
// **/
typedef enum
{
    UMTS_NO_CODING = 0,
    UMTS_CONV_2 = 1,        // convolutioal coding 1/2
    UMTS_CONV_3,            // convolutioal coding 1/3
    UMTS_TURBO_3,           // TURBO coding 1/3
}UmtsCodingType;

// /**
// ENUM:: UmtsCrcSizeType
// DESCRIPTION :: UMTS CRC size
// **/
typedef enum
{
    UMTS_CRC_NONE = 0,  // No CRC
    UMTS_CRC_8 = 8,     // 8 bits CRC
    UMTS_CRC_12 = 12,   // 12 bits CRC
    UMTS_CRC_16 = 16,   // 16 bits CRC
    UMTS_CRC_24 = 24    // 24 bits CRC
}UmtsCrcSizeType;
// /**
// ENUM:: UmtsPhyChRelativePhase
// DESCRIPTION :: UMTS physical Channel relative phase
// **/
typedef enum
{
    UMTS_PHY_CH_BRANCH_I = 0,        // 0 phase
    UMTS_PHY_CH_BRANCH_Q = 1,        // 1/2 pi phase
    UMTS_PHY_CH_BRANCH_IQ,           // both phases are used
}UmtsPhyChRelativePhase;

// /**
// ENUM:: UmtsChannelDirection
// DESCRIPTION :: UMTS Channel direction
// **/
typedef enum
{
    UMTS_CH_UPLINK = 0, // up link
    UMTS_CH_DOWNLINK,   // DOWN LINK
}UmtsChannelDirection;

// /**
// ENUM:: UmtsSpreadFactor
// DESCRIPTION :: UMTS Spread Factor
// **/
typedef enum
{
    UMTS_SF_1 = 1,
    UMTS_SF_2 = 2,
    UMTS_SF_4 = 4,
    UMTS_SF_8 = 8,
    UMTS_SF_16 = 16,
    UMTS_SF_32 = 32,
    UMTS_SF_64 = 64,
    UMTS_SF_128 = 128,
    UMTS_SF_256 = 256,
    UMTS_SF_512 = 512
}UmtsSpreadFactor;

// /**
// ENUM:: UmtsChipRateType
// DESCRIPTION :: UMTS Physical Channel Chip Rate Type
// Reference   :: 3GPP TS 25.321
// **/
typedef enum
{
    UMTS_CHIP_RATE_384,
    UMTS_CHIP_RATE_768,
    UMTS_CHIP_RATE_128,
}UmtsChipRateType;

// cross layer enum

// /*
// ENUM        :: UmtsInterlayerCommandType
// DESCRIPTION :: Inter layer command type
// **/
typedef enum
{
    // RLC-RRC
    UMTS_CRLC_CONFIG_ESTABLISH,
    UMTS_CRLC_CONFIG_RELEASE,

    // MAC-RLC
    UMTS_MAC_DATA_REQ,      // MAC-DATA-request
    UMTS_MAC_DATA_IND,      // MAC-DATA-Indication
    UMTS_MAC_STATUS_IND,    // MAC-STATUS-Indication
    UMTS_MAC_STATUS_RSP,    // MAC-STATUS-Response

    // MAC-RRC
    UMTS_CMAC_CONFIG_REQ_UE,     // UE info element
    UMTS_CMAC_CONFIG_REQ_RB,     // RB info element
    UMTS_CMAC_CONFIG_REQ_TrCh,   // TrCh Info element
    UMTS_CMAC_CONFIG_REQ_RACH,   // RACH info element
    UMTS_CMAC_CONFIG_REQ_CIPHER, // Ciphering info element
    UMTS_CMAC_CONFIG_REQ_MBMS,   // MBMS info element
    UMTS_CMAC_CONFIG_REQ_EDCH,   // E-DCH info element

    UMTS_CMAC_MEASUREMENT_REQ, // CMAC-MEASUREMENT-Request
    UMTS_CMAC_MEASUREMENT_IND, // CMAC-MEASUREMENT-Indication
    UMTS_CMAC_STATUS_IND,      // CMAC-STATUS-Indication

    // L1-L2
    UMTS_PHY_ACCESS_REQ,    // PHY-ACCESS-Request
    UMTS_PHY_ACCESS_CNF,    // PHY-ACCESS-Confirm
    UMTS_PHY_DATA_REQ,      // PHY-DATA-Request, impl. as interlayer API
    UMTS_PHY_DATA_IND,      // PHY-DATA-Indication, impl. as interlayer API
    UMTS_PHY_STATUS_REQ,    // PHY-STATUS-Request
    UMTS_PHY_STATUS_IND,    // PHY-STATUS-Indication

    // L1-L3
    UMTS_CPHY_SYNC_IND,     // CPHY-SYNC-Indication
    UMTS_CPHY_OUT_OF_SYNC_IND, // CPHY-OUT-OF-SYNC-Indication
    UMTS_CPHY_MEASUREMENT_REQ, // CPHY-MEASUREMENT-Request
    UMTS_CPHY_MEASUREMENT_IND, // CPHY-MEASUREMENT-Indication
    UMTS_CPHY_ERROR_IND,       // CPHY-ERROR-indication

    UMTS_CPHY_TrCH_CONFIG_REQ,     // CPHY-TrCH-Config
    UMTS_CPHY_TrCH_RELEASE_REQ,    // CPHY_TrCH-Release
    UMTS_CPHY_RL_SETUP_REQ,        // CPHY-RL-Setup
    UMTS_CPHY_RL_RELEASE_REQ,      // CPHY-RL-Release
    UMTS_CPHY_RL_MODIFY_REQ,       // CPHY-RL-Modify
    UMTS_CPHY_COMMIT_REQ,          // CPHY-COMMIT
    UMTS_CPHY_OUT_OF_SYNC_CONFIG_REQ, // CPHY-OUT-OF-SYNC-CONFIG-REQ
    UMTS_CPHY_MBMS_CONFIG_REQ,     // CPHY-MBMS_CONFIG_REQ
}UmtsInterlayerCommandType;

// /*
// ENUM        :: UmtsInterlayerMsgType
// DESCRIPTION :: Inter layer message type
// **/
typedef enum
{
    UMTS_REPORT_AMRLC_ERROR
} UmtsInterlayerMsgType;

// /*
// ENUM        :: UmtsL1MeasurementType
// DESCRIPTION :: Measurement type at L1/L2/L3
// **/
typedef enum
{
    // UE side
    UMTS_MEASURE_L1_SFN_CFN_TIME_DIFF,   // SFN-CFN observed time diff.
    UMTS_MEASURE_L1_CPICH_Ec_No,         // CPICH Ec/No
    UMTS_MEASURE_L1_CPICH_RSCP,          // CPICH RSCP
    UMTS_MEASURE_L1_UTRA_CARRIER_RSSI,   // UTRA Carrier Rssi
    UMTS_MEASURE_L1_UE_TRCH_BLER,           // Transport CH block error rate
    UMTS_MEASURE_L1_UE_TX_POWER,         // UE transmitted power
    UMTS_MEASURE_L1_UE_RX_TX_TIME_DIFF,  // UE Rx-Tx time diff.
    UMTS_MEASURE_L1_UE_SFN_SFN_TIME_DIFF,  // SFN-SFN observed time diff.

    // UTRAN side
    UMTS_MEASURE_L1_RCVD_WIDEBAND_POWER, // recvd total wide band power
    UMTS_MEASURE_L1_TX_CARRIER_POWER,    // transmitted carrier power
    UMTS_MEASURE_L1_TX_CODE_POWER,       // transmitted code power
    UMTS_MEASURE_L1_PHCH_BER,            // physical channel BER
    UMTS_MEASURE_L1_UTRAN_TRCH_BLER,     // transport CH block error rate
    UMTS_MEASURE_L1_RX_TIME_DEVIATION,   // Rx timing deviation
    UMTS_MEASURE_L1_RSCP,                // RSCP
    UMTS_MEASURE_L1_RTT,                 // round trip time
    UMTS_MEASURE_L1_ACK_PRACH_PREAMBLE,  // acknowledged PRACH preambles
    UMTS_MEASURE_L1_SIR,                 // SIR
    UMTS_MEASURE_L1_PRACH_PROP_DELAY,    // PRACH propagation delay
    UMTS_MEASURE_L1_SIR_ERROR,           // SIR error
    UMTS_MEASURE_L1_RCVD_SYNC_UL_TIME_DEVIATION, // Rcvd sycn_ul timing dev.
    UMTS_MEASURE_L1_CELL_SYNC_BURST_TIME, // cell sync burst timing
    UMTS_MEASURE_L1_CELL_SYNC_BURST_SIR,  // cell sync burst SIR
    UMTS_MEASURE_L1_UTRAN_SFN_SFN_TIME_DIFF,  // SFN-SFN observed time diff.
    UMTS_MEASURE_L1_DL_TX_BRACH_LOAD,    // DL  transmission brach load
}UmtsL1MeasurementType;

///////
//POWER CONTROL
//////
// /*
// ENUM        :: PhyUmtsPowerControlAlgorithm
// DESCRIPTION :: Power COntrol Algorithm
// **/
typedef enum
{
    UMTS_POWER_CONTROL_ALGO_1 = 0, // Algorithm 1
    UMTS_POWER_CONTROL_ALGO_2,     // Algorithm 2
}PhyUmtsPowerControlAlgorithm;
// /*
// ENUM        :: PhyUmtsPowerControlCommand
// DESCRIPTION :: Power COntrol Algorithm
// **/
typedef enum
{
    UMTS_POWER_CONTROL_CMD_DEC = 0,
    UMTS_POWER_CONTROL_CMD_INC = 1,
    UMTS_POWER_CONTROL_CMD_INVALID
}PhyUmtsPowerControlCommand;

// /*
// ENUM        :: PhyUmtsDpcMode
// DESCRIPTION :: DL POWER  CONTORL MODE
// **/
typedef enum
{
    UMTS_DL_POWER_CONTROL_MODE_0 = 0, // send powCOntrl cmd every slot
    UMTS_DL_POWER_CONTROL_MODE_1 = 1, // repeat powCtrl cmd in 3 slots
}PhyUmtsDpcMode;

/////////
// ADMISSION CONTROL
/////////
// /*
// ENUM        :: UmtsAdmissionControlType
// DESCRIPTION :: Admission control type
// **/
typedef enum
{
    // Maximize the numer of users NCAC
    // Maximal number of users
    // simple to implement
    // flexible : priorities between classes
    // designed for 2G, not adequate for CDMA
    UMTS_CAC_MAXIMUM_USER = 0,

    // Interference based CAC ICAC
    // CDMA interference-limited
    // if total interference < threshold, accept call
    // For voice networks : ICAC = NCAC
    UMTS_CAC_INTERFERENCE_BASED,

    // Power based CAC PCAC
    // Power-based CAC (PCAC)
    // emitted power < maximal transmission power
    // in the downlink : PCAC = ICAC
    // in the uplink : PCAC determines the coverage
    UMTS_CAC_POWER_BASED,

    // SIR-based CAC (SCAC)
    // accept call if SIR > SIRthreshold
    // Equivalently, if Eb/N0 > (Eb/N0)min
    // More accurate than ICAC
    UMTS_CAC_SIR_BASED,

    UMTS_CAC_TESTING

}UmtsAdmissionControlType;

// /**
// CONSTANT : UMTS_DEFAULT_CAC_TYPE
// DESCRIPTION : Default call admisison control method
// **/
#define UMTS_DEFAULT_CAC_TYPE  UMTS_CAC_TESTING

// /**
// CONSTANT :: MAX UL CPACITY
// code rate 3
#define UMTS_MAX_UL_CAPACITY_3 1920000 // 1.92Mbps
// code rate 2
#define UMTS_MAX_UL_CAPACITY_2 2280000 // 2.28Mbps
// **/

///////////////////////////////////////////////////////////////////////////
// MESSAGES / STRUCTURES
///////////////////////////////////////////////////////////////////////////

// General

typedef char CellularIMSIInDigit[CELLULAR_IMSI_LENGTH];
typedef char CellularMCC[CELLULAR_MCC_LENGTH];
typedef char CellularMNC[CELLULAR_MNC_LENGTH];
typedef char CellularMSIN[CELLULAR_MSIN_LENGTH];
typedef char CellularMCCMNC[CELLULAR_MCC_MNC_LENGTH];
typedef UInt32 TMSI;
typedef UInt32 PTMSI;

// /**
// STRUCT      :: UmtsPlmnIdentity
// DESCRIPTION :: Information identifies a PLMN
// **/
typedef struct
{
    CellularMCC mcc;  // Mobile Contry Code
    CellularMNC mnc;  // Mobile Network Code
} UmtsPlmnIdentity;

// /**
// STRUCT      :: UmtsLocationAreaId
// DESCRIPTION :: Location Area Identification (LAI)
// **/
typedef struct
{
    CellularMCCMNC mccMnc;  // Mobile Contry Code (MCC)
    char lac[4];       // Loc. Area Code (LAC), 2 bytes, we use
                      // 4 bytes here to use NodeB id as the LAC
} UmtsLocationAreaId;

// /**
// STRUCT      :: UmtsRoutingAreaId
// DESCRIPTION :: Routing Area Identification (RAI)
// **/
typedef struct
{
    CellularMCCMNC mccMnc;  // Mobile Contry Code (MCC)
    char lac[4];      // Loc. Area Code (LAC), should be 2 bytes, we use
                      // 4 bytes here to use NodeB id as the LAC
    char rac;         // Routing Area Code (RAC)
} UmtsRoutingAreaId;

// /**
// STRUCT      :: CellularIMSI
// DESCRIPTION :: Definition of the IMSI
//                (International Mobile Subscriber Identity)
// **/
class CellularIMSI
{
  private:
    unsigned char bytes[CELLULAR_IMSI_LENGTH];

  public:

    // /**
    // FUNCTION   :: CellularIMSI::getImsiInDigit
    // LAYER      :: Layer 3
    // PURPOSE    :: Get the IMSI in array of bytes as in digit format
    // PARAMETERS ::
    // + imsiInDigit : CellularImsiInDigit : For return the IMSI in digit
    // RETURN     :: void : NULL
    // **/
    inline void getIMSIInDigit(CellularIMSIInDigit imsiInDigit)
    {
        memcpy(imsiInDigit, bytes, CELLULAR_IMSI_LENGTH);
    }

    // /**
    // FUNCTION   :: CellularIMSI::getCompactIMSI
    // LAYER      :: Layer 3
    // PURPOSE    :: Get the IMSI in 8 bytes compact format
    // PARAMETERS ::
    // + buff : char* : For return the IMSI in 8 bytes format
    // RETURN     :: UInt32 : Length of bytes copied
    // **/
    inline int getCompactIMSI(char *buff)
    {
        int i;

        for (i = 0; i < CELLULAR_IMSI_LENGTH / 2; i ++)
        {
            buff[i] = (bytes[i * 2] << 4) + bytes[i * 2 + 1];
        }
        buff[i] = bytes[i * 2] << 4;

        return i + 1;
    }

    // /**
    // FUNCTION   :: CellularIMSI::getMCC
    // LAYER      :: Layer 3
    // PURPOSE    :: Get the MCC part of the IMSI in digit
    // PARAMETERS ::
    // + mcc : CellularMCC : For return the MCC of the IMSI in digit
    // RETURN     :: void : NULL
    // **/
    inline void getMCC(CellularMCC mcc)
    {
        memcpy(mcc, bytes, CELLULAR_MCC_LENGTH);
    }

    // /**
    // FUNCTION   :: CellularIMSI::getMCC
    // LAYER      :: Layer 3
    // PURPOSE    :: Get the MCC part of the IMSI in UInt32
    // PARAMETERS ::
    // RETURN     :: UInt32 : MCC code in integer
    // **/
    inline UInt32 getMCC()
    {
        int i;
        UInt32 val = 0;

        for (i = 0; i < CELLULAR_MCC_LENGTH; i ++)
        {
            val = val * 10 + bytes[i];
        }

        return val;
    }

    // /**
    // FUNCTION   :: CellularIMSI::getMNC
    // LAYER      :: Layer 3
    // PURPOSE    :: Get the MNC part of the IMSI
    // PARAMETERS ::
    // + mnc : CellularMNC : For return the MNC of the IMSI in digit
    // RETURN     :: void : NULL
    // **/
    inline void getMNC(CellularMNC mnc)
    {
        memcpy(mnc, &(bytes[CELLULAR_MCC_LENGTH]), CELLULAR_MNC_LENGTH);
    }

    // /**
    // FUNCTION   :: CellularIMSI::getMNC
    // LAYER      :: Layer 3
    // PURPOSE    :: Get the MNC part of the IMSI in UInt32
    // PARAMETERS ::
    // RETURN     :: UInt32 : MCC code in integer
    // **/
    inline UInt32 getMNC()
    {
        int i;
        UInt32 val = 0;

        for (i = CELLULAR_MCC_LENGTH;
             i < CELLULAR_MCC_LENGTH + CELLULAR_MNC_LENGTH;
             i ++)
        {
            val = val * 10 + bytes[i];
        }

        return val;
    }

    // /**
    // FUNCTION   :: CellularIMSI::getMCCMNC
    // LAYER      :: Layer 3
    // PURPOSE    :: Get the MCC and MNC in compact format
    // PARAMETERS ::
    // + mccMnc : CellularMCCMNC : For return the compact MCC and MNC
    // RETURN     :: UInt32 : MCC code in integer
    // **/
    inline void getMCCMNC(CellularMCCMNC mccMnc)
    {
        int i;

        for (i = 0; i < CELLULAR_MCC_MNC_LENGTH; i ++)
        {
            mccMnc[i] = (bytes[i * 2] << 4) + bytes[i * 2 + 1];
        }
    }

    // /**
    // /**
    // FUNCTION   :: CellularIMSI::getMSIN
    // LAYER      :: Layer 3
    // PURPOSE    :: Get the MSIN part of the IMSI
    // PARAMETERS ::
    // + msin : CellularMSIN : For return the MSIN of the IMSI in digit
    // RETURN     :: void : NULL
    // **/
    inline void getMSIN(CellularMSIN msin)
    {
        memcpy(msin,
               &(bytes[CELLULAR_MCC_LENGTH + CELLULAR_MNC_LENGTH]),
               CELLULAR_MSIN_LENGTH);
    }

    // /**
    // FUNCTION   :: CellularIMSI::getId
    // LAYER      :: Layer 3
    // PURPOSE    :: Get an unique ID from IMSI
    // PARAMETERS ::
    // RETURN     :: UInt32 : Id from IMSI
    // **/
    inline UInt32 getId()
    {
        UInt32 uniqueId = 0;
        int i;

        for (i = CELLULAR_MCC_LENGTH + CELLULAR_MNC_LENGTH;
             i < CELLULAR_IMSI_LENGTH;
             i ++)
        {
            uniqueId = uniqueId * 10 + bytes[i];
        }

        return uniqueId;
    }

    // /**
    // FUNCTION   :: CellularIMSI::setMCC
    // LAYER      :: Layer 3
    // PURPOSE    :: Set the MCC part of the IMSI in digit
    // PARAMETERS ::
    // + mcc : CellularMCC : Source MCC for setting MCC of IMSI in digit
    // RETURN     :: void : NULL
    // **/
    inline void setMCC(CellularMCC mcc)
    {
        memcpy(bytes, mcc, CELLULAR_MCC_LENGTH);
    }

    // /**
    // FUNCTION   :: CellularIMSI::setMCC
    // LAYER      :: Layer 3
    // PURPOSE    :: Set the MCC part of the IMSI in integer
    // PARAMETERS ::
    // + mcc : UInt32 : Source MCC for setting MCC of IMSI
    // RETURN     :: void : NULL
    // **/
    inline void setMCC(UInt32 mcc)
    {
        int i;

        for (i = CELLULAR_MCC_LENGTH; i > 0; i --)
        {
            bytes[i - 1] = (unsigned char)(mcc % 10);
            mcc /= 10;
        }
    }

    // /**
    // FUNCTION   :: CellularIMSI::setMNC
    // LAYER      :: Layer 3
    // PURPOSE    :: Set the MNC part of the IMSI
    // PARAMETERS ::
    // + mnc : CellularMNC : For setting the MNC of the IMSI in digit
    // RETURN     :: void : NULL
    // **/
    inline void setMNC(CellularMNC mnc)
    {
        memcpy(&(bytes[CELLULAR_MCC_LENGTH]), mnc, CELLULAR_MNC_LENGTH);
    }

    // /**
    // FUNCTION   :: CellularIMSI::setMNC
    // LAYER      :: Layer 3
    // PURPOSE    :: Set the MNC part of the IMSI
    // PARAMETERS ::
    // + mnc : UInt32 : For setting the MNC of the IMSI
    // RETURN     :: void : NULL
    // **/
    inline void setMNC(UInt32 mnc)
    {
        int i;

        for (i = CELLULAR_MCC_LENGTH + CELLULAR_MNC_LENGTH;
             i > CELLULAR_MCC_LENGTH;
             i --)
        {
            bytes[i - 1] = (unsigned char)(mnc % 10);
            mnc /= 10;
        }
    }

    // /**
    // FUNCTION   :: CellularIMSI::setMSIN
    // LAYER      :: Layer 3
    // PURPOSE    :: Set the MSIN part of the IMSI
    // PARAMETERS ::
    // + msin : CellularMSIN : For setting the MSIN of the IMSI in digit
    // RETURN     :: void : NULL
    // **/
    inline void setMSIN(CellularMSIN msin)
    {
        memcpy(&(bytes[CELLULAR_MCC_LENGTH + CELLULAR_MNC_LENGTH]),
               msin,
               CELLULAR_MSIN_LENGTH);
    }

    // /**
    // FUNCTION   :: CellularIMSI::setId
    // LAYER      :: Layer 3
    // PURPOSE    :: Set an unique ID for IMSI
    // PARAMETERS ::
    // + id : UInt32 : Unique ID for IMSI
    // RETURN     :: void : NULL
    // **/
    inline void setId(UInt32 id)
    {
        int i;
        UInt32 tmpId = id;

        for (i = CELLULAR_IMSI_LENGTH;
             i > CELLULAR_MCC_LENGTH + CELLULAR_MNC_LENGTH;
             i --)
        {
            bytes[i - 1] = (unsigned char)(tmpId % 10);
            tmpId = tmpId / 10;
        }
    }

    // /**
    // FUNCTION   :: CellularIMSI::setCompactIMSI
    // LAYER      :: Layer 3
    // PURPOSE    :: Set the IMSI using 8 bytes compact format
    // PARAMETERS ::
    // + buff : char* : IMSI value in 8 bytes format
    // RETURN     :: void : NULL
    // **/
    inline void setCompactIMSI(char *buff)
    {
        int i;

        for (i = 0; i < CELLULAR_IMSI_LENGTH / 2; i ++)
        {
            bytes[i * 2] = (buff[i] >> 4) & 0x0F;
            bytes[i * 2 + 1] = buff[i] & 0x0F;
        }
        bytes[i * 2] = (buff[i] >> 4) & 0x0F;
    }

    // /**
    // FUNCTION   :: CellularIMSI::print
    // LAYER      :: Layer 3
    // PURPOSE    :: Print out the IMSI
    // PARAMETERS ::
    // + buff : char * : Buff to store print out
    // RETURN     :: void : NULL
    // **/
    inline void print(char *buff)
    {
        int i;
        int index = 0;

        buff[index ++] = '[';
        for (i = 0; i < CELLULAR_IMSI_LENGTH; i ++)
        {
            if (i == CELLULAR_MCC_LENGTH ||
                i == CELLULAR_MCC_LENGTH + CELLULAR_MNC_LENGTH)
            {
                buff[index ++] = ':';
            }

            buff[index ++] = '0' + bytes[i];
        }
        buff[index] = ']';
        buff[index + 1] = 0;
    }

    // /**
    // FUNCTION   :: CellularIMSI::operator==
    // LAYER      :: Layer 3
    // PURPOSE    :: Reload operator "==" for this class.
    //               To tell if two IMSIs are equal
    // PARAMETERS ::
    // + imsi      : CellularIMSI : Another IMSI to be compared with this
    // RETURN     :: bool : true if equal, otherwise, false
    // **/
    inline bool operator==(CellularIMSI imsi)
    {
        return memcmp(bytes, imsi.bytes, CELLULAR_IMSI_LENGTH) == 0;
    }

    // /**
    // FUNCTION   :: CellularIMSI::operator!=
    // LAYER      :: Layer 3
    // PURPOSE    :: Reload operator "!=" for this class.
    //               To tell if two IMSIs are not equal
    // PARAMETERS ::
    // + imsi      : CellularIMSI : Another IMSI to be compared with this
    // RETURN     :: bool : true if not equal, otherwise, false
    // **/
    inline bool operator!=(CellularIMSI imsi)
    {
        return memcmp(bytes, imsi.bytes, CELLULAR_IMSI_LENGTH) != 0;
    }

    // /**
    // FUNCTION   :: CellularIMSI::~CellularIMSI
    // LAYER      :: Layer 3
    // PURPOSE    :: Destruction function of the IMS
    // PARAMETERS ::
    // RETURN     :: void : NULL
    // **/
    ~CellularIMSI() {};

    // /**
    // FUNCTION   :: CellularIMSI::CellularIMSI
    // LAYER      :: Layer 3
    // PURPOSE    :: Initialization of the IMSI without any input
    // PARAMETERS ::
    // RETURN     :: void : NULL
    // **/
    inline CellularIMSI()
    {
        memset(bytes, 0, CELLULAR_IMSI_LENGTH);
    }

    // /**
    // FUNCTION   :: CellularIMSI::CellularIMSI
    // LAYER      :: Layer 3
    // PURPOSE    :: Initialization of the IMSI with input
    // PARAMETERS ::
    // + imsiInDigit : char * : IMSI for init the new IMSI
    // RETURN     :: void : NULL
    // **/
    inline CellularIMSI(char *imsiInDigit)
    {
        memcpy(bytes, imsiInDigit, CELLULAR_IMSI_LENGTH);
    }

    // /**
    // FUNCTION   :: CellularIMSI::CellularIMSI
    // LAYER      :: Layer 3
    // PURPOSE    :: Initialization of the IMSI with input
    // PARAMETERS ::
    // + mcc       : CellularMCC  : MCC part of the IMSI
    // + mnc       : CellularMNC  : MNC part of the IMSI
    // + msin      : CellularMSIN : MSIN part of the IMSI
    // RETURN     :: void     : NULL
    // **/
    inline CellularIMSI(CellularMCC mcc,
                                      CellularMNC mnc,
                                      CellularMSIN msin)
    {
        memset(bytes, 0, CELLULAR_IMSI_LENGTH);
        setMCC(mcc);
        setMNC(mnc);
        setMSIN(msin);
    }

    // /**
    // FUNCTION   :: CellularIMSI::CellularIMSI
    // LAYER      :: Layer 3
    // PURPOSE    :: Initialization of the IMSI based on an ID. In this case
    //               default MCC and default MNC will be used. And the ID
    //               will be put in the last 4 byte of the bytes array.
    // PARAMETERS ::
    // + id        : UInt32 : ID for generating the IMSI
    // RETURN     :: void   : NULL
    // **/
    inline CellularIMSI(UInt32 id)
    {
        memset(bytes, 0, CELLULAR_IMSI_LENGTH);
        setId(id);
    }

    // /**
    // FUNCTION   :: CellularIMSI::CellularIMSI
    // LAYER      :: Layer 3
    // PURPOSE    :: Initialization of the IMSI based on MCC, MNC and ID.
    //               MCC, MNC and ID are all in integer format, not digit
    // PARAMETERS ::
    // + mcc       : UInt32 : Integer number of MCC
    // + mnc       : UInt32 : Integer number of MNC
    // + id        : UInt32 : ID for generating the IMSI
    // RETURN     :: void   : NULL
    // **/
    inline CellularIMSI(UInt32 mcc, UInt32 mnc, UInt32 id)
    {
        memset(bytes, 0, CELLULAR_IMSI_LENGTH);
        setMCC(mcc);
        setMNC(mnc);
        setId(id);
    }
};

//
// L2 CMAC
//

//
// Radio bearer related
//

// transport format
struct UmtsTransportFormat
{
    // define the dynamic part
    UInt32 transBlkSize; // tranpsort block size, 0-5000
    unsigned int TTI; // TTI in slot 10,20,40,80
    UInt64 transBlkSetSize; // transport lock set size, 0-200000
//    unsigned int TTI; // TTI in slot 10,20,40,80

    UmtsModulationType modType; // modulation type useful for HS-DSCH

    // define the static part
    UmtsCodingType codingType; // type of channel coding
                               // including code type and rate
    UmtsCrcSizeType crcSize;

    // DCH/E-DCH/HSDPA transport format
    UmtsTransportChannelType trChType;

    // skip the reesulting ratio after static rate matching
    UmtsTransportFormat(){};
    UmtsTransportFormat(UInt32 blkSize,
                        UInt64 blkSetSize,
                        int ttiVal,
                        UmtsCodingType coding,
                        UmtsCrcSizeType crc)
    {
        transBlkSize = blkSize;
        transBlkSetSize = blkSetSize;
        TTI = ttiVal;
        codingType = coding;
        crcSize = crc;
        modType = UMTS_MOD_QPSK;
        trChType = UMTS_TRANSPORT_DCH;
    }
    UmtsTransportFormat(UInt32 blkSize,
                        UInt64 blkSetSize,
                        int ttiVal,
                        UmtsCodingType coding,
                        UmtsCrcSizeType crc,
                        UmtsModulationType modulation)
    {
        transBlkSize = blkSize;
        transBlkSetSize = blkSetSize;
        TTI = ttiVal;
        codingType = coding;
        crcSize = crc;
        modType = modulation;
        trChType = UMTS_TRANSPORT_DCH;
    }
    void UpdateFormat(UInt32 blkSize,
                 UInt64 blkSetSize,
                 int ttiVal,
                 UmtsCodingType coding,
                 UmtsCrcSizeType crc)
    {
        transBlkSize = blkSize;
        transBlkSetSize = blkSetSize;
        TTI = ttiVal;
        codingType = coding;
        crcSize = crc;
    }
    void UpdateFormat(UInt32 blkSize,
                 UInt64 blkSetSize,
                 int ttiVal,
                 UmtsCodingType coding,
                 UmtsCrcSizeType crc,
                 UmtsModulationType modulation)
    {
        transBlkSize = blkSize;
        transBlkSetSize = blkSetSize;
        TTI = ttiVal;
        codingType = coding;
        crcSize = crc;
        modType = modulation;
    }
    void SetTransBlkSize(UInt32 blkSize)
    {
        transBlkSize = blkSize;
    }
    UInt32 GetTransBlkSize()
    {
        return transBlkSize;
    }
    void SetTransBlkSetSize(UInt64 blkSetSize)
    {
        transBlkSetSize = blkSetSize;
    }
    UInt64 GetTransBlkSetSize()
    {
        return transBlkSetSize;
    }
    void SetCodingType(UmtsCodingType coding)
    {
        codingType = coding;
    }
    UmtsCodingType GetCodingType()
    {
        return codingType;
    }
    void SetCrcSize(UmtsCrcSizeType crc)
    {
        crcSize = crc;
    }
    UmtsCrcSizeType GetCrcSize()
    {
        return crcSize;
    }
    void SetModulationType(UmtsModulationType modulation)
    {
        modType = modulation;
    }
    UmtsModulationType GetModulationType()
    {
        return modType;
    }
};

// define the transport format set
// typedef std::list<UmtsTransportFormat> UmtsTransportFormatSet;
// define the transport format combination set

// cross layer msg/cmd info structure

// /**
// STRUCT      :: UmtsCmacConfigReqUeInfo
// DESCRIPTION :: the info field structure in CMAC-CONFIG-REQ-UE cmd
// **/
typedef struct
{
    // S-RNTI
    // SRNC id
    // C-RNTI
    // Activation time
    // Primary E-RNTI configured
    // sceondary E-RNTI configured
}UmtsCmacConfigReqUeInfo;

// /**
// STRUCT      :: UmtsCmacConfigReqRbInfo
// DESCRIPTION :: the info field structure in CMAC-CONFIG-REQ-RB cmd
// **/
struct UmtsCmacConfigReqRbInfo
{
    unsigned char rbId;
    BOOL releaseRb;

    // ueId is used at NodeB to distinguish if it is common
    // or for specific UE
    NodeId ueId;
    unsigned int priSc;
    UmtsChannelDirection chDir;

    UmtsLogicalChannelType logChType;
    unsigned char logChId;
    UmtsTransportChannelType trChType;
    unsigned char trChId;
    unsigned char logChPrio;
};

// /**
// STRUCT      :: UmtsCmacConfigTrChInfo
// DESCRIPTION :: the info of Transport channel in CMAC-CONFIG-REQ-TrCH
typedef struct
{
    unsigned char trChId;
    // transport format set
}UmtsCmacConfigReqTrChInfo;

// /**
// STRUCTURE   :: UmtsRachAscInfo
// DESCRIPTION :: the asc mapping defined in PRACH partitions
// **/
typedef struct
{
    unsigned char ascIndex;
    unsigned char sigStartIndex;
    unsigned char sigEndIndex;
    UInt16 assignedSubCh; // 3 repeats of 4 bits
    double persistFactor;
}UmtsRachAscInfo;

// /**
// STRUCT      :: UmtsCmacConfigReqRachInfo
// DESCRIPTION :: the info of Transport channel in CMAC-CONFIG-REQ-RACH
typedef struct
{
    BOOL releaseRachInfo;

    unsigned char Mmax; // Maximum number of preamble cycles
    unsigned char NB01min; // Sets lower bound for random back-off
    unsigned char NB01max; // Sets upper bound for random back-off
    std::list<UmtsRachAscInfo*>* prachPartition; //ASC info
    unsigned char ascIndex; // the selected ASC for this routnd RACH proc.
}UmtsCmacConfigReqRachInfo;

// /**
// STRUCT      :: UmtsCmacMeasReqInfo
// DESCRIPTION :: the info of measurement in UMTS_CMAC_MEASUREMENT_REQ
typedef struct
{

}UmtsCmacMeasReqInfo;
// /**
// STRUCT      :: UmtsCmacMeasIndInfo
// DESCRIPTION :: the info of measurement in UMTS_CMAC_MEASUREMENT_Ind
typedef struct
{

}UmtsCmacMeasIndInfo;

//
// CPHY transport CH related
//
// /**
// STRUCT      :: UmtsCphyTrChConfigReqInfo
// DESCRIPTION :: the info of the transport channel info
//                UMTS_CPHY_TrCH_CONFIG_REQ
typedef struct
{
    NodeAddress ueId; // used to distiguish if it is a common TrCh or not

    UmtsTransportChannelType trChType;
    unsigned char trChId;
    UmtsChannelDirection trChDir;

    // transport format info
    UmtsTransportFormat transFormat;

    // logical channel prio
    unsigned char logChPrio;
}UmtsCphyTrChConfigReqInfo;

// /**
// STRUCT      :: UmtsCphyTrChReleaseReqInfo
// DESCRIPTION :: the info of the transport channel info
//                UMTS_CPHY_TrCH_CONFIG_REQ
typedef struct
{
    UmtsTransportChannelType trChType;
    unsigned char trChId;
    UmtsChannelDirection trChDir;

    // transport format info
}UmtsCphyTrChReleaseReqInfo;

//
// Raido link related
//
// /**
// STRUCTURE   :: UmtsCphyChDescPSCH
// DESCRIPTION :: channel description for PSCH
// **/
typedef struct
{
    BOOL txDevisityMode;
}UmtsCphyChDescPSCH;

// /**
// STRUCTURE   :: UmtsCphyChDescSSCH
// DESCRIPTION :: channel description for SSCH
// **/
typedef struct
{
    BOOL txDevisityMode;
}UmtsCphyChDescSSCH;

// /**
// STRUCTURE   :: UmtsCphyChDescPCCPCH
// DESCRIPTION :: channel description for PCCPCH
// **/
typedef struct
{
    double freq;
    unsigned int dlScCode;
    BOOL txDevisityMode;
    // skip TDD
}UmtsCphyChDescPCCPCH;

// /**
// STRUCTURE   :: UmtsCphyChDescSCCPCH
// DESCRIPTION :: channel description for SCCPCH
// **/
typedef struct
{
    double freq;
    unsigned int dlScCode;
    unsigned int chCode;
    UmtsSpreadFactor sfFactor;
    BOOL txDevisityMode;
    // skip TDD

    // for implementation purpose
    unsigned char sccpchIndex; // K  th S-CCPCH
    unsigned int offsetInChips; // from P-CCPCH
}UmtsCphyChDescSCCPCH;

// /**
// STRUCTURE   :: UmtsCphyChDescPRACH
// DESCRIPTION :: channel description for PRACH
// **/
typedef struct
{
    unsigned char accessSlot;       // access slot
    unsigned int preScCode;         // preamble scrambling code
    unsigned int availPreSigniture; // available preamble signiture
    UmtsSpreadFactor dataSpreadFactor; // spread factor for data part
    unsigned char maxRetry;

    // power ocntrol info
    double ulTargetSIN;          // UL target SIR
    double pccpchDlTxPower;      // pccpch dl tx power
    double ulInterference;       // UL interference
    double powerOffset;
    double initTxPower;
    unsigned char powerRampStep;

    // access service class info
    unsigned int ascAvailPreSigniture;
    // skip TDD
    unsigned int ascAvailSubCh;

    unsigned char aichTxTimingOpt; // only 0 and 1 are defined

    // canned for implmentation
    unsigned char slotFormatIndex;
}UmtsCphyChDescPRACH;

// /**
// STRUCTURE   :: UmtsCphyChDescUlDPCH
// DESCRIPTION :: channel description for Uplink DPCH(DPDCH and DPCCH)
// **/
typedef struct
{
    unsigned int ulScCode;
    unsigned char lenNpilot;
    unsigned char lenNtpc;
    unsigned char lenNtfci;
    unsigned char lenNfbi;
    clocktype txTimeOffset;

    // skip TDD

    // canned for implmentation
    unsigned char slotFormatIndex;
}UmtsCphyChDescUlDPCH;

// /**
// STRUCTURE   :: UmtsCphyChDescUlHsdpcch
// DESCRIPTION :: channel description for Uplink DPCH(DPDCH and DPCCH)
// **/
typedef struct
{
    unsigned int ulScCode;
    UmtsSpreadFactor sfFactor;
    unsigned int chCode;

    unsigned int nodebPriSc;

    // canned for implmentation
    unsigned char slotFormatIndex;
}UmtsCphyChDescUlHsdpcch;

// /**
// STRUCTURE   :: UmtsCphyChDescDlDPCH
// DESCRIPTION :: channel description for Downlink DPCH(DPDCH and DPCCH)
// **/
typedef struct
{
    clocktype txTimeOffset;

    unsigned int dlScCode;
    UmtsSpreadFactor sfFactor;
    unsigned int dlChCode;
    BOOL  txDiversityMode;

    unsigned char lenNpilot;
    unsigned char lenNtpc;
    unsigned char lenNtfci;
    unsigned char lenNfbi;
    unsigned int lenNdata1;
    unsigned int lenNdata2;

    // canned for implmentation
    unsigned char slotFormatIndex;

    // skip TDD
}UmtsCphyChDescDlDPCH;

// /**
// STRUCTURE   :: UmtsCphyChDescPICH
// DESCRIPTION :: channel description for PICH
// **/
typedef struct
{
    unsigned int scCode;
    UmtsSpreadFactor sfFactor;
    unsigned int chCode;
    // skip TDD

    // for implementation purpose
    unsigned char pichIndex; // associated with K th S-CCPCH
    unsigned int offsetInChips; // from its associated S-CCPCH
}UmtsCphyChDescPICH;

// /**
// STRUCTURE   :: UmtsCphyChDescAICH
// DESCRIPTION :: channel description for AICH
// **/
typedef struct
{
    unsigned int scCode;
    UmtsSpreadFactor sfFactor;
    unsigned int chCode;
    BOOL txDivesityMode;
    // skip TDD
}UmtsCphyChDescAICH;

// /**
// STRUCTURE   :: UmtsCphyChDescHspdsch
// DESCRIPTION :: Channel description for HsPDSCH
// **/
typedef struct
{
    unsigned int scCode;
    UmtsSpreadFactor sfFactor;
    unsigned int chCode;
    BOOL txDiversityMode;
}UmtsCphyChDescHspdsch;

// /**
// STRUCTURE   :: UmtsCphyChDescHsscch
// DESCRIPTION :: Channel description for HS-SCCH
// **/
typedef struct
{
    unsigned int scCode;
    UmtsSpreadFactor sfFactor;
    unsigned int chCode;
}UmtsCphyChDescHsscch;

// /**
// STRUCT      :: UmtsCphyRadioLinkSetupReqInfo
// DESCRIPTION :: the info of the radio link info
//                UMTS_CPHY_RL_SETUP_REQ
// **/
typedef struct
{
    // ueId is used at NodeB to distinguish if it is common
    // or for specific UE
    NodeAddress ueId;
    unsigned int priSc;

    UmtsPhysicalChannelType phChType;
    unsigned char phChId;
    UmtsChannelDirection phChDir;

    void* phChDesc; // channel description

}UmtsCphyRadioLinkSetupReqInfo;

// /**
// STRUCT      :: UmtsCphyRadioLinkModifyReqInfo
// DESCRIPTION :: the info of the radio link info
//                UMTS_CPHY_RL_MODIFY_REQ
// **/
typedef UmtsCphyRadioLinkSetupReqInfo UmtsCphyTrChModifyReqInfo;

// /**
// STRUCT      :: UmtsCphyRadioLinkRelReqInfo
// DESCRIPTION :: the info of the radio link info
//                UMTS_CPHY_RL_Release_REQ
// **/
typedef struct
{
    UmtsPhysicalChannelType phChType;
    unsigned char phChId;
    UmtsChannelDirection phChDir;

}UmtsCphyRadioLinkRelReqInfo;

//
// L1 random aceess req confirm
//

// /**
// STRUCT      :: UmtsPhyAccessReqInfo
// DESCRIPTION :: the info in the UMTS_PHY_ACCESS_REQ command
// **/
typedef struct
{
    // TODO transport format for this time RACH
    unsigned char ascIndex;

    // the following four are for the ASC info and PHY configuration
    // to simplify, these infomation is passed from MAC rahter
    // than derived by PHY itself
    UInt16 subChAssigned;
    unsigned char sigStartIndex;
    unsigned char sigEndIndex;
    unsigned char maxRetry;

}UmtsPhyAccessReqInfo;

// /**
// ENUM        :: UmtsPhyAccessCnfInfoType
// DESCRIPTION :: the info type in the UMTS_PHY_ACCESS_CFN command
// **/
typedef enum
{
    UMTS_PHY_ACCESS_CFN_NACK,
    UMTS_PHY_ACCESS_CFN_ACK,
    UMTS_PHY_ACCESS_CFN_NO_AICH_RSP,
}UmtsPhyAccessCnfInfoType;

// /**
// STRUCT      :: UmtsPhyAccessCnfInfo
// DESCRIPTION :: the info in the UMTS_PHY_ACCESS_CFN command
// **/
typedef struct
{
    UmtsPhyAccessCnfInfoType cnfType;
}UmtsPhyAccessCnfInfo;

//
// L1 MEASUREMENT related
//

// STRUCT      :: UmtsCphyMeasReqInfo
// DESCRIPTION :: the info of the measurement info
//                UMTS_CPHY_MEASUREMENT_REQ
// **/
typedef struct
{
    unsigned int measId;
    UmtsL1MeasurementType measType;
    void* measInfo;
}UmtsCphyMeasReqInfo;

// /**
// STRUCT      :: UmtsCphyMeasIndInfo
// DESCRIPTION :: the info of the measurement info
//                UMTS_CPHY_MEASUREMENT_IND
// **/
typedef struct
{
    unsigned int measId;
    UmtsL1MeasurementType measType;
    void* measInfo;
}UmtsCphyMeasIndInfo;

// /**
// STRUCT      :: UmtsMeasCpichRscp
// DESCRIPTION :: the info of the measurement info
//                of CPICH RSCP
// **/
typedef struct
{
    unsigned int priSc;
    double measVal;
}UmtsMeasCpichRscp;

// /**
// STRUCT      :: UmtsMeasCpichEcNo
// DESCRIPTION :: the info of the measurement info
//                of CPICH EcNo
// **/
typedef UmtsMeasCpichRscp UmtsMeasCpichEcNo;
// TODO: add more

// /**
// STRUCT      :: UmtsTrCh2PhChMappingInfo
// DESCRIPTION :: The mapping info between trnaposrt ch. and physical ch.
// **/
typedef struct
{
    unsigned char rbId;
    NodeAddress ueId;
    unsigned int priSc;

    UmtsChannelDirection chDir;
    UmtsTransportChannelType trChType;
    unsigned char trChId;

    UmtsPhysicalChannelType phChType;
    unsigned int phChId;
    // std::list<unsigned char> phChId; // should be a list

    // transport format
    UmtsTransportFormat transFormat;
}UmtsTrCh2PhChMappingInfo;

// /**
// STRUCT      :: UmtsRcvPhChInfo
// DESCRIPTION :: Physical channel information stored at
//                the receiving side so that the signals from
//                other physical channel can be screened out
// **/
struct UmtsRcvPhChInfo
{
    UmtsSpreadFactor        sfFactor;
    unsigned int            chCode;
};

// /**
// STRUCT      :: UmtsPhChUpdateInfo
// DESCRIPTION :: Info for update Physical channel
// **/
typedef struct
{
    UmtsModulationType* modType;
    double* gainFactor;
    NodeAddress*  nodeId;
}UmtsPhChUpdateInfo;

///////////////////////////////////////////////////////////////////////////
// RLC Configuration Structures
///////////////////////////////////////////////////////////////////////////
// /**
// STRUCT:: UmtsRlcTmConfig
// DESCRIPTION::
//      TM RLC entity configuration parameters
//      used by RRC to setup/config TM entities
// **/
typedef struct
{
    BOOL                        segIndi;
} UmtsRlcTmConfig;

// /**
// STRUCT:: UmtsRlcUmConfig
// DESCRIPTION::
//      UM RLC entity configuration parameters
//      used by RRC to setup/config UM entities
// **/
typedef struct
{
    unsigned int                maxUlPduSize;
    unsigned int                dlLiLen;
    BOOL                        alterEbit;
} UmtsRlcUmConfig;

// /**
// STRUCT:: UmtsRlcAmConfig
// DESCRIPTION::
//      AM RLC entity configuration parameters
//      used by RRC to setup/config AM entities
// **/
typedef struct
{
    unsigned int                sndPduSize;
    unsigned int                rcvPduSize;
    unsigned int                sndWnd;
    unsigned int                rcvWnd;

    unsigned int                maxDat;
    unsigned int                maxRst;
    clocktype                   rstTimerVal;
} UmtsRlcAmConfig;

// /**
// UNION:: UmtsRlcConfig
// DESCRIPTION::
//      TM RLC entity configuration parameters
// **/
typedef union
{
    UmtsRlcTmConfig  tmConfig;
    UmtsRlcUmConfig  umConfig;
    UmtsRlcAmConfig  amConfig;
} UmtsRlcConfig;

// /**
// ENUM::   UmtsRlcEntityDirect
// DESCRIPTION::
//      Traffic direction of a RLC entity
// **/
typedef enum
{
    UMTS_RLC_ENTITY_TX,
    UMTS_RLC_ENTITY_RX,
    UMTS_RLC_ENTITY_BI
} UmtsRlcEntityDirect;

// /**
// ENUM:: UmtsRlcEntityType
// DESCRIPTION:: RLC Entites
// **/
typedef enum
{
    UMTS_RLC_ENTITY_TM = 1,
    UMTS_RLC_ENTITY_UM,
    UMTS_RLC_ENTITY_AM
} UmtsRlcEntityType;

// /**
// STRUCT      :: UmtsRbMapping
// DESCRIPTION :: UE RB mapping info data structure stored in the RNC
// **/
struct UmtsRbMapping
{
    NodeId                  ueId;
    char                    rbId;
    UmtsChannelDirection    direction;      //Uplink/downlink

    UmtsRlcEntityType       rlcType;        //RLC entity type

    UmtsLogicalChannelType  logChType;
    unsigned char           logChId;
    unsigned char           logChPrio;

    UmtsTransportChannelType trChType;
    unsigned char           trChId;

    UmtsRbMapping() {};
    UmtsRbMapping(NodeId ue,
                  unsigned char rb,
                  UmtsChannelDirection dir)
                  : ueId(ue), rbId(rb), direction(dir) { }
};

// /**
// STRUCT      :: UmtsPhChInfo
// DESCRIPTION :: physical channel info
// **/
struct UmtsPhChInfo
{
    UmtsPhysicalChannelType chType;
    unsigned int            chId;
    UmtsPhChInfo(UmtsPhysicalChannelType type,
                 unsigned int id)
                 : chType(type), chId(id)
    { }
};

// /**
// STRUCT      :: UmtsRbPhChMapping
// DESCRIPTION :: Downlink RB physical channel mapping info
// **/
struct UmtsRbPhChMapping
{
    NodeId ueId;
    char rbId;
    std::list<UmtsPhChInfo*>* phChList;

    UmtsRbPhChMapping(NodeId ue, char rb)
        : ueId(ue), rbId(rb)
    {
       phChList = new std::list<UmtsPhChInfo*>;
    }

    ~UmtsRbPhChMapping()
    {
        for (std::list<UmtsPhChInfo*>::iterator it = phChList->begin();
             it != phChList->end(); ++it)
        {
            delete *it;
        }
        delete phChList;
    }

    void AddPhCh(const UmtsPhChInfo& info)
    {
        phChList->push_back(new UmtsPhChInfo(info));
    }
};

// STRUCT:: UmtsRlcRrcEstablishArgs
// DESCRIPTION:: RRC-establish command argument
// **/
struct UmtsRlcRrcEstablishArgs
{
    NodeId          ueId;
    char            rbId;
    UmtsRlcEntityDirect direction;
    UmtsRlcEntityType entityType;
    void*           entityConfig;
};

// /**
// STRUCT:: UmtsRlcRrcReleaseArgs
// DESCRIPTION:: RRC-Release command argument
// **/
struct UmtsRlcRrcReleaseArgs
{
    NodeId     ueId;
    char    rbId;
    UmtsRlcEntityDirect direction;
};

// /**
// STRUCT:: UmtsLayer2PktToUpperlayerInfo
// DESCRIPTION::
//      Information needed to submit a packet to upper layer
// **/
typedef struct
{
    unsigned int rbId;
    NodeAddress ueId;
} UmtsLayer2PktToUpperlayerInfo;

///////////////
// MEASUREMENT
///////////////

// /**
// STRUCT      :: UmtsMeasElement
// DESCRIPTION :: Element of a measurement
// **/
struct UmtsMeasElement
{
    double measVal;
    clocktype timeStamp;
    UmtsMeasElement(){};
    UmtsMeasElement(double val, clocktype stamp)
        :measVal(val),timeStamp(stamp){ }
};

// /**
// CLASS           :: UmtsMeasTimeLessThan
// DESCRIPTION     :: Compare to measurement based on time stamp
// **/
template<typename T>
class UmtsMeasTimeLessThan
{
    clocktype measTime;
public:
    explicit UmtsMeasTimeLessThan(clocktype stamp)
                : measTime(stamp) { }
    bool operator() (const T& item) const
    {
        return item.timeStamp < measTime;
    }
};

// /**
// CLASS        :: UmtsIsCellInActiveSet
// DESCRIPTION  :: Decide if cell is in active
// **/
template<typename T>
class UmtsIsCellInActiveSet
{
public:
    bool operator() (const T* cellInfo) const
    {
        return cellInfo->cellStatus
                == UMTS_CELL_ACTIVE_SET;
    }
};

// /**
// CLASS           :: UmtsIsCellInTransitSet
// DESCRIPTION     :: Decide if cell is in transmit status
// **/
template<typename T>
class UmtsIsCellInTransitSet
{
public:
    bool operator() (const T* cellInfo) const
    {
        return cellInfo->cellStatus == UMTS_CELL_ACT_TO_MONIT_SET ||
                cellInfo->cellStatus == UMTS_CELL_MONIT_TO_ACT_SET;
    }
};


// /**
// STRUCT      :: UmtsMeasTimeWindow
// DESCRIPTION :: TIme windows based measurement
// **/
struct UmtsMeasTimeWindow
{
    std::list<UmtsMeasElement>* measValList;
    clocktype timeWindowSize;
    clocktype lastTimeStamp;
    double avgVal;

    UmtsMeasTimeWindow(clocktype timeWindow)
    {
        measValList = new std::list<UmtsMeasElement>;
        timeWindowSize = timeWindow;
        lastTimeStamp = 0;
        avgVal = 0;
    }

    ~UmtsMeasTimeWindow()
    {
        delete measValList;
    }

    void RemoveOutdatedElement(clocktype curTime)
    {
        std::list<UmtsMeasElement>::iterator itMeas;
        for(itMeas = measValList->begin();
            itMeas != measValList->end();)
        {
            if ((*itMeas).timeStamp < (curTime - timeWindowSize))
            {
                std::list<UmtsMeasElement>::iterator itOld = itMeas ++;
                measValList->erase(itOld);
            }
            else
            {
                itMeas ++;
            }
        }
    }

#if 0
    void RemoveOutdatedElement(clocktype curTime)
    {
        std::list<UmtsMeasElement>::iterator it;
        measValList->remove_if(
            UmtsMeasTimeLessThan<UmtsMeasElement>(
                (curTime - timeWindowSize)));

    }
#endif

    void AddMeasElement(const UmtsMeasElement& measInfo,
                        clocktype curTime)
    {
        measValList->push_back(UmtsMeasElement(measInfo));
        lastTimeStamp = measInfo.timeStamp;
        RemoveOutdatedElement(curTime);
    }

    void SetAvgMeas(double val)
    {
        avgVal = val;
    }

    void CalAvgMeas(clocktype curTime, 
        UmtsTimeWindowStatAvgType avgType)
    {
        // remove outdated element before call this function
        std::list<UmtsMeasElement>::iterator it;
        double totalVal = 0;
        clocktype oldestMeas = curTime - 1 * NANO_SECOND;
        int i = 0;

        RemoveOutdatedElement(curTime);

        for ( it = measValList->begin();
              it != measValList->end();
              it ++)
        {
            totalVal += it->measVal;
            if (it->timeStamp < oldestMeas)
            {
                oldestMeas = it->timeStamp;
            }
            i  ++;
        }
        if (totalVal && i)
        {
            if (avgType == UMTS_STAT_TIME_WIMDOW_AVG_TIME)
            {
                // in sceond unit
                totalVal = totalVal * SECOND / (curTime - oldestMeas);
            }
            else
            {
                totalVal = totalVal / i;
            }
        }
        SetAvgMeas(totalVal);
    }
    double GetAvgMeas(clocktype curTime, UmtsTimeWindowStatAvgType avgType)
    {
        CalAvgMeas(curTime, avgType);
        return avgVal;
    }
    double GetAvgMeas(clocktype curTime)
    {
        CalAvgMeas(curTime, UMTS_STAT_TIME_WINDOW_AVG_NUMBER);
        return avgVal;
    }
    double GetAvgMeas()
    {
        return avgVal;
    }
};

///////////////////////////////////////////////////////////////////////////
// API and utility functions
///////////////////////////////////////////////////////////////////////////
// /**
// CLASS::     UmtsFindItemByRbId
// DESCRIPTION::
//      Function object used to find a data item with
//      its rbId field equaling to a given value
// **
template<class T>
class UmtsFindItemByRbId
{
    char rbId;
public:
    explicit UmtsFindItemByRbId(char rbIdentifier)
                : rbId(rbIdentifier) { }
    bool operator() (const T* item) const
    {
        return item->rbId == rbId;
    }
};

// /**
// CLASS::     UmtsFindItemByRabId
// DESCRIPTION::
//      Function object used to find a data item with
//      its rabId field equaling to a given value
// **/
template<class T>
class UmtsFindItemByRabId
{
    char rabId;
public:
    explicit UmtsFindItemByRabId(char rabIdentifier)
                : rabId(rabIdentifier) { }
    bool operator() (const T* item) const
    {
        return item->rabId == rabId;
    }
};

// /**
// CLASS::     UmtsFindItemByUeId
// DESCRIPTION::
//      Function object used to find a data item with
//      its UEID field equaling to a given value
// **/
template<class T>
class UmtsFindItemByUeId
{
    NodeId ueId;
public:
    explicit UmtsFindItemByUeId(NodeId ueIdentifier)
                : ueId(ueIdentifier) { }
    bool operator() (const T* item) const
    {
        return item->ueId == ueId;
    }
};

// /**
// CLASS::     UmtsFindItemByNodebId
// DESCRIPTION::
//      Function object used to find a data item with
//      its Nodeb ID field equaling to a given value
// **/
template<class T>
class UmtsFindItemByNodebId
{
    NodeId nodebId;
public:
    explicit UmtsFindItemByNodebId(NodeId nodeb)
                : nodebId(nodeb) { }
    bool operator() (const T* item) const
    {
        return item->nodebId == nodebId;
    }
};

// /**
// CLASS::     UmtsFindItemByCellId
// DESCRIPTION::
//      Function object used to find a data item with
//      its CellId field equaling to a given value
// **/
template<class T>
class UmtsFindItemByCellId
{
    UInt32 cellId;
public:
    explicit UmtsFindItemByCellId(UInt32 cell)
                : cellId(cell) { }
    bool operator() (const T* item) const
    {
        return item->cellId == cellId;
    }
};


// /**
// CLASS::     UmtsFindItemByChId
// DESCRIPTION::
//      Function object used to find a data item with
//      its chId field equal to a given value
// **/
template<class T>
class UmtsFindItemByChId
{
    unsigned int chId;
public:
    explicit UmtsFindItemByChId(unsigned int chIdentifier)
                : chId(chIdentifier) { }
    bool operator() (const T* item) const
    {
        return item->chId == chId;
    }
};

// /**
// CLASS::     UmtsFindItemByOvsf
// DESCRIPTION::
//      Function object used to find a data item with
//      its chCode and sfFactor equal to specified values
// **/
template<class T>
class UmtsFindItemByOvsf
{
    UInt32 chCode;
    UmtsSpreadFactor sfFactor;
public:
    explicit UmtsFindItemByOvsf(UInt32 code,
                                UmtsSpreadFactor sf)
                : chCode(code), sfFactor(sf) { }
    bool operator() (const T* item) const
    {
        return item->chCode == chCode &&
               item->sfFactor == sfFactor;
    }
};

// CLASS::     UmtsFindItemByUeIdAndRbId
// DESCRIPTION::
//      Function object used to find a data item with
//      its rbId field and UeId equaling to given values
// **
template<class T>
class UmtsFindItemByUeIdAndRbId
{
    NodeId ueId;
    char  rbId;

public:
    explicit UmtsFindItemByUeIdAndRbId(
                NodeId ue, char rb)
                : ueId(ue), rbId(rb) { }
    bool operator() (const T* item) const
    {
        return ( item->rbId == rbId &&
                 item->ueId == ueId);
    }
};


// CLASS::     UmtsFindItemByRbIdAndChDir
// DESCRIPTION::
//      Function object used to find a data item with
//      its rbId field and direction equaling to given values
//      TDIR can be UmtsChannelDirection or UmtsRlcEntityDirect
//      provided T must have a rbId and direction field.
// **
template<class T, class TDIR>
class UmtsFindItemByRbIdAndChDir
{
    char  rbId;
    TDIR  direction;
public:
    explicit UmtsFindItemByRbIdAndChDir(
                char rbIdentifier,
                TDIR  dir)
                : rbId(rbIdentifier), direction(dir) { }
    bool operator() (const T* item) const
    {
        return ( item->rbId == rbId &&
                 item->direction == direction);
    }
};

// /**
// CLASS::     UmtsFindItemByDlTeId
// DESCRIPTION::
//      Function object used to find a data item with
//      its dlTeId field equaling to a given value
// **/
template<class T>
class UmtsFindItemByDlTeId
{
    unsigned int dlTeId;
public:
    explicit UmtsFindItemByDlTeId(unsigned int dlTeID)
                : dlTeId(dlTeID) { }
    bool operator() (const T* item) const
    {
        return (item->dlTeId == dlTeId);
    }
};

// /**
// STRUCT:: UmtsPointedItemLess
// DESCRIPTION::
//      A function object class comparing the objects pointed
//      by pointers, primarily used as Less function in a STL
//      set that store pointers to the real objects.
//      Note class T must implement operator<
// **/
template<class T>
class UmtsPointedItemLess : 
    public std::binary_function<const T*, const T*, bool>
{
public:
    bool operator() (const T* lPtr,
                     const T* rPtr) const
    {
        return *lPtr < *rPtr;
    }
};

// /**
// STRUCT:: UmtsPointedItemEqual
// DESCRIPTION::
//      A function object class comparing the objects pointed
//      by pointers
// **/
template<class T>
class UmtsPointedItemEqual : 
    public std::binary_function<const T*, const T*, bool>
{
public:
    bool operator() (const T* lPtr,
                     const T* rPtr) const
    {
        return *lPtr == *rPtr;
    }
};

// /**
// STRUCT::     UmtsFreeStlMsgPtr
// DESCRIPTION::
//      Function object used to free Message pointer in STL container
// **/
class UmtsFreeStlMsgPtr
{
    Node* node;
public:
    explicit UmtsFreeStlMsgPtr(Node* ndPtr):node(ndPtr) { }
    void operator() (Message* msg)
    {
#if 0
        while(msg)
        {
            Message* nextMsg = msg->next;
            MESSAGE_Free(node, msg);
            msg = nextMsg;
        }
#endif // 0
        MESSAGE_Free(node, msg);
    }
};

// /**
// STRUCT::     UmtsFreeStlItem
// DESCRIPTION::
//      Function object used to release allocated memory by
//      STL items, which should not contain any pointer pointing
//      to heap-allocated memory
// USAGE: std::for_each(xxx.begin(), xxx.end(), UmtsFreeStlItem()),
//        where xxx is an STL container.
// **/
template<typename T>
class UmtsFreeStlItem
{
public:
    void operator() (T* item)
    {
        MEM_free(item);
    }
};

// /**
// STRUCT::     UmtsDeletePtr
// DESCRIPTION::
//      Function object used to deconstruct the object pointed by
//      a pointer stored in a STL container
// **/
class UmtsDeletePtr
{
public:
    template<typename T>
    void operator() (T* objPtr)
    {
        delete objPtr;
    }
};

// /**
// FUNCTION::     UmtsClearStlContDeep
// DESCRIPTION::
//      Function object used to clear pointer items in a STL container
//      DEEP: means it will release the objects allocated in free-store
//      space, pointed by each pointer stored in the container
// **/
template<typename T>
void UmtsClearStlContDeep(T* stlCont)
{
    std::for_each(stlCont->begin(),
                  stlCont->end(),
                  UmtsDeletePtr());
    stlCont->clear();
}

// /**
// FUNCTION   :: UmtsSetBit
// LAYER      :: Layer 3
// PURPOSE    :: Set one bit in an integer number
// PARAMETERS ::
// + number   :: The number to be operated on
// + pos      :: The bit position (0-31 from right to left) to be set
// RETURN     :: NULL
// **/
template<typename T>
inline
void UmtsSetBit(
    T&      number,
    size_t  pos)
{
    number |= (0x1 << pos);
}

// /**
// FUNCTION   :: UmtsClearBit
// LAYER      :: Layer 3
// PURPOSE    :: Clear one bit in an unsigned int
// PARAMETERS ::
// + number   :: The number to be operated on
// + pos      :: The bit position (0-31 from right to left) to be set
// RETURN     :: NULL
// **/
template<typename T>
inline
void UmtsClearBit(
    T&      number,
    size_t  pos)
{
    number &= ~(0x1 << pos);
}

// /**
// FUNCTION   :: UmtsGetBit
// LAYER      :: Layer 3
// PURPOSE    :: Get one bit in an unsigned int
// PARAMETERS ::
// + number   :: The number to be operated on
// + pos      :: The bit position (0-31 from right to left) to be set
// RETURN     :: BOOL
// **/
template<typename T>
inline
BOOL UmtsGetBit(
    T       number,
    size_t  pos)
{
    return 0x1 & (number >> pos);
}

// /**
// FUNCTION::       UmtsGetPosForNextZeroBit
// LAYER::          ALL
// PURPOSE::        Get the position of next zero bit in a bitset.
// PARAMETERS::
//    + bitSet:     The bitset to be searched.
//    + startPos:   The start position of searching
// RETURN::         NULL
// **/
template<typename T>
int UmtsGetPosForNextZeroBit(
    T       number,
    size_t  startPos = 0)
{
    int retPos = -1;
    for (size_t i = startPos; i < sizeof(T)*8; i++)
    {
        if (!UmtsGetBit<T>(i))
        {
            retPos = (int)i;
            break;
        }
    }
    return retPos;
}

// /**
// FUNCTION::       UmtsGetPosForNextZeroBit
// LAYER::          ALL
// PURPOSE::        Get the position of next zero bit in a STL bitset.
// PARAMETERS::
//    + bitSet:     The bitset to be searched.
//    + startPos:   The start position of searching
// RETURN::         NULL
// **/
template<size_t N>
int UmtsGetPosForNextZeroBit(
    const std::bitset<N>& bitSet,
    size_t  startPos = 0)
{
    int retPos = -1;
    for (size_t i = startPos; i < N; ++i)
    {
        if ( !bitSet[i] )
        {
            retPos = (int)i;
            break;
        }
    }
    return retPos;
}

// /**
// FUNCTION   :: UmtsSetBitsInOctet
// LAYER      :: Layer 3
// PURPOSE    :: Set several bits in an octet with desired value
// PARAMETERS ::
// + octet    :: The octet to be operated
// + mask     :: Bit mask
// + value    :: The value to be set
// + shift    :: number of bits to be shifted
// RETURN     :: NULL
// **/
inline
void UmtsSetBitsInOctet(
    UInt8& octet,
    UInt8 mask,
    UInt8 value,
    unsigned int shift)
{
    octet &= ~(mask);
    octet |= ((value << shift) & mask);
}

// /**
// FUNCTION   :: UmtsGetBitsFromOctet
// LAYER      :: Layer 3
// PURPOSE    :: Get the value of certain bits in an octet.
// PARAMETERS ::
// + octet    :: The octet to be operated
// + mask     :: Bit mask
// + value    :: The value to be set
// + shift    :: number of bits to be shifted
// RETURN     :: UInt8:: The desired value
// **/
inline
UInt8 UmtsGetBitsFromOctet(
    UInt8 octet,
    UInt8 mask,
    unsigned int shift)
{
    return ((UInt8)((octet & mask) >> shift));
}
// /**
// FUNCTION   :: UmtsGetRateParaFromCodingType
// LAYER      :: Layer 3
// PURPOSE    :: Get the the rate coeffiecientfrom coding type .
// PARAMETERS ::
// + codingType :: UmtsCodingType : coding type
// RETURN     :: UmtsLayer3Data * : Pointer to UMTS layer3 data or NULL
// **/
inline
unsigned char UmtsGetRateParaFromCodingType(
                  UmtsCodingType codingType)
{
    if (codingType == UMTS_CONV_2)
    {
        return 2;
    }
    else if (codingType == UMTS_CONV_3)
    {
        return 3;
    }
    else if (codingType == UMTS_TURBO_3)
    {
        return 3;
    }
    else
    {
        return 1;
    }
}


// /**
// FUNCTION   :: UmtsGetNodeType
// LAYER      :: Layer 3
// PURPOSE    :: Get the node type of the node.
// PARAMETERS ::
// + node      : Node *           : Pointer to node.
// RETURN     :: CellularNodeType : node type of this node
// **/
CellularNodeType UmtsGetNodeType(Node *node);

// /**
// FUNCTION   :: UmtsIsUe
// LAYER      :: Layer 3
// PURPOSE    :: Is the device a UE node?
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// RETURN     :: BOOL   : TRUE, if UE, FALSE elsewise
// **/
BOOL UmtsIsUe(Node *node);

// /**
// FUNCTION   :: UmtsIsUe
// LAYER      :: Layer 3
// PURPOSE    :: Is the device a NodeB node?
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// RETURN     :: BOOL   : TRUE, if NodeB, FALSE elsewise
// **/
BOOL UmtsIsNodeB(Node *node);

// /**
// FUNCTION   :: UmtsIsRnc
// LAYER      :: Layer 3
// PURPOSE    :: Is the device a RNC node?
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// RETURN     :: BOOL   : TRUE, if RNC, FALSE elsewise
// **/
BOOL UmtsIsRnc(Node *node);

// /**
// FUNCTION   :: UmtsIsGgsn
// LAYER      :: Layer 3
// PURPOSE    :: Is the device a GGSN node?
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// RETURN     :: BOOL   : TRUE, if GGSN, FALSE elsewise
// **/
BOOL UmtsIsGgsn(Node *node);

// /**
// FUNCTION   :: UmtsGetDuplexMode
// LAYER      :: Layer 2
// PURPOSE    :: return Duplex mode of Ue/NodeB of the specified interface
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// RETURN     :: UmtsDuplexMode   : duplex mode of the interface
// **/
UmtsDuplexMode UmtsGetDuplexMode(Node*node, int interfaceIndex);

// /**
// FUNCTION   :: UmtsNodeBGetCellId
// LAYER      :: Layer 2
// PURPOSE    :: return the cell id of the interface at nodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// RETURN     :: cellId   : cell Id of the nodeB
// **/
UInt32 UmtsNodeBGetCellId(Node*node, int interfaceIndex);

// /**
// FUNCTION   :: UmtsGetUlChIndex
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index of the interface at nodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// RETURN     :: channelIndex   : channel Index of the nodeB
// **/
int UmtsGetUlChIndex(Node*node, int interfaceIndex);

// /**
// FUNCTION   :: UmtsSetUlChIndex
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index of the interface at nodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + channelIndex  : int : chaneel index to be set
// RETURN     :: cellId   : cell Id of the nodeB
// **/
void UmtsSetUlChIndex(Node*node, int interfaceIndex, int channelIndex);

// /**
// FUNCTION   :: UmtsLayer3GetDlPhChDataBitRate
// LAYER      :: Layer 2
// PURPOSE    :: Get the data rate for the associated SF and DL Ph Ch Type
// PARAMETERS ::
// + sFactor  : UmtsSpreadFactor : Spread factor
// + slotFormatIndex : signed char* : slot format index
// + phChType : UmtsPhysicalChannelType :Phy Ch type
// RETURN     :: int : data rate can be support by the channel with the SF
// **/
int UmtsLayer3GetDlPhChDataBitRate(
       UmtsSpreadFactor sFactor,
       signed char* slotFormatIndex,
       UmtsPhysicalChannelType phChType = UMTS_PHYSICAL_DPDCH);

// /**
// FUNCTION   :: UmtsLayer3GetUlPhChDataBitRate
// LAYER      :: Layer 2
// PURPOSE    :: Get the data rate for the associated SF and UL Ph Ch Type
// PARAMETERS ::
// + sFactor  : UmtsSpreadFactor : Spread factor
// + slotFormatIndex : signed char* : slot format index
// + phChType : UmtsPhysicalChannelType :Phy Ch type
// RETURN     :: int : data rate can be support by the channel with the SF
// **/
int UmtsLayer3GetUlPhChDataBitRate(
       UmtsSpreadFactor sFactor,
       signed char* slotFormatIndex,
       UmtsPhysicalChannelType phChType = UMTS_PHYSICAL_DPDCH);

// /**
// FUNCTION   :: UmtsGetBestUlSpreadFactorForRate
// LAYER      :: Layer 2
// PURPOSE    :: Get the best SF for the specified rate requirement
// PARAMETERS ::
// + rateReq  : int : Data rate requirement 
// + sFactor  : UmtsSpreadFactor* : Spread factor
// RETURN     :: BOOL : whether a SF can be used to support this rate
// **/
BOOL UmtsGetBestUlSpreadFactorForRate(int rateReq, UmtsSpreadFactor* sf);

// /**
// FUNCTION   :: UmtsUeSetSpreadFactorInUse
// LAYER      :: Layer 2
// PURPOSE    :: Set the spread factor for the interface
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + sf        : UmtsSpreadFactor : spread factor to be set
// RETURN     :: cellId   : cell Id of the nodeB
// **/
void UmtsUeSetSpreadFactorInUse(Node* node,
                                int interfaceIndex,
                                UmtsSpreadFactor sf);
// /**
// FUNCTION   :: UmtsUeGetSpreadFactorInUse
// LAYER      :: Layer 2
// PURPOSE    :: Get the spread factor in use for the interface
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// RETURN     :: UmtsSpreadFactor   : spread factor in use
// **/
UmtsSpreadFactor UmtsUeGetSpreadFactorInUse(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: UmtsPhyCalculateGainFactor
// LAYER      :: UMTS LAYER2 MAC sublayer
// PURPOSE    :: Calculate the gain factor.
// PARAMETERS ::
// + spreadFactor     : int               : spread factor.
// RETURN     :: double : gain factor
// **/
double UmtsPhyCalculateGainFactor( UmtsSpreadFactor spreadFactor);

// /**
// FUNCTION   :: UmtsUeGetActiveDpdchNum
// LAYER      :: Layer 2
// PURPOSE    :: Get the number of active DPDCH in use for the interface
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// RETURN     :: int   : number of active DPDCH in use
// **/
int UmtsUeGetActiveDpdchNum(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: UmtsLayer3GetHsdpaCapability
// LAYER      :: Layer 2
// PURPOSE    :: Get the HSDPA capability for the interface
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// RETURN     :: BOOL   : HSDPA enabled or not
// **/
BOOL UmtsLayer3GetHsdpaCapability(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: UmtsSelectDefaultTransportFormat
// LAYER      :: UMTS L2 MAC sublayer
// PURPOSE    :: get the default TTI of a transport channel from its type
// PARAMETERS ::
// trChType    : UmtsTransportChannelType  : transport channel type
// format      : UmtsTransportFormat*      : transport format to be used
// qosInfo     : const UmtsRABServiceInfo* : QoS Info 
// RETURN     :: BOOL : whether a transport format can be supported
// **/
BOOL UmtsSelectDefaultTransportFormat(
            UmtsTransportChannelType  trChType,
            UmtsTransportFormat* format,
            const UmtsRABServiceInfo* qosInfo = NULL);


// FUNCTION   :: UmtsSerializeMessage
// LAYER      :: MAC
// PURPOSE    :: Dump a single message into a buffer so that the orignal
//               message can be recovered from the buffer
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Pointer to a messages
// + buffer    : string   : The string buffer the message will be 
//                          serialzed into (append to the end)
// RETURN     :: void     : NULL
// **/
void UmtsSerializeMessage(Node* node,
                          Message* msg,
                          std::string& buffer);


// FUNCTION   :: UmtsUnSerializeMessage
// LAYER      :: MAC
// PURPOSE    :: recover the orignal message from the buffer
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + buffer    : const char* : The string buffer containing the message 
//                          was serialzed into
// + bufIndex  : int  : the start pos. in the buf. pointing to the msg
//                          updated to the end of the msg 
//                          after the unserialization.
// RETURN     :: Message* : Message pointer to be recovered
// **/
Message* UmtsUnSerializeMessage(Node* node,
                                const char* buffer,
                                int& bufIndex);

// FUNCTION   :: UmtsSerializeMessageList
// LAYER      :: MAC
// PURPOSE    :: Dump a list of message into a buffer so that the orignal
//               messages can be recovered from the buffer
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Pointer to a message list
// + buffer    : string&  : The string buffer the messages will be 
//                          serialzed into (append to the end)
// RETURN     :: int   : number of messages in the list
// **/
int UmtsSerializeMessageList(Node* node,
                             Message* msgList,
                             std::string& buffer);

// FUNCTION   :: UmtsUnSerializeMessageList
// LAYER      :: MAC
// PURPOSE    :: recover the orignal message from the buffer
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + buffer    : const char*   : The string buffer containing the message
//                          list serialzed into
// + bufIndex  : int&  : the start position in the buffer pointing
//                          to the message list
//                          updated to the end of the message 
//                          list after the unserialization.
// + numMsgs   : int   : Number of messages in the list
// RETURN     :: Message* : Pointer to the message list to be recovered
// **/
Message* UmtsUnSerializeMessageList(Node* node,
                                    const char* buffer,
                                    int& bufIndex,
                                    int numMsgs);

// FUNCTION   :: UmtsPackMessage
// LAYER      :: MAC
// PURPOSE    :: Pack a list of messages to be one message structure
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msgList   : Message* : Pointer to a list of messages
// + origProtocol: TraceProtocolType : Protocol allocating this packet
// RETURN     :: Message* : The super msg contains a list of msgs as payload
// **/
Message* UmtsPackMessage(Node* node,
                         Message* msgList,
                         TraceProtocolType origProtocol);

// /**
// FUNCTION   :: UmtsUnpackMessage
// LAYER      :: MAC
// PURPOSE    :: Unpack a message to the original list of messages
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Pointer to the supper msg contains list of msgs
// + copyInfo  : BOOL     : Whether copy info from old msg to first msg
// + freeOld   : BOOL     : Whether the original message should be freed
// RETURN     :: Message* : A list of messages unpacked from original msg
// **/
Message* UmtsUnpackMessage(Node* node,
                           Message* msg,
                           BOOL copyInfo,
                           BOOL freeOld);

// FUNCTION   :: UmtsPackMessage
// LAYER      :: MAC
// PURPOSE    :: Pack a list of messages to be one message structure
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msgList   : list<Message*>& : a list of messages
// + origProtocol: TraceProtocolType : Protocol allocating this packet
// RETURN     :: Message* : The super msg contains a list of msgs as payload
// **/
Message* UmtsPackMessage(Node* node,
                         std::list<Message*>& msgList,
                         TraceProtocolType origProtocol);

// /**
// FUNCTION   :: UmtsUnpackMessage
// LAYER      :: MAC
// PURPOSE    :: Unpack a message to the original list of messages
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Pointer to the supper msg containing list of msgs
// + copyInfo  : BOOL     : Whether copy info from old msg to first msg
// + freeOld   : BOOL     : Whether the original message should be freed
// + msgList   : list<Message*> : an empty stl list to be used to 
//                                contain unpacked messages
// RETURN     :: NULL
// **/
void UmtsUnpackMessage(Node* node,
                       Message* msg,
                       BOOL copyInfo,
                       BOOL freeOld,
                       std::list<Message*>& msgList);


// FUNCTION   :: UmtsDuplicateMessageList
// LAYER      :: ANY
// PURPOSE    :: Duplicate a list of message
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msgList   : Message* : Pointer to a list of messages
// RETURN     :: Message* : The duplicated message list header
// **/
Message* UmtsDuplicateMessageList(
    Node* node,
    Message* msgList);

// /**
// FUNCTION::           UmtsPrintMessage
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Print part of message information for
//                      debugging purpose
// PARAMETERS::
//      +os:            output stream class
//      +msg:           Message to be printed
// RETURN::             ostream
// **/
std::ostream& UmtsPrintMessage(std::ostream& os, Message* msg);

// /**
// FUNCTION::       UmtsLinkMessages
// LAYER::          ALL
// PURPOSE::        Link messages contained in a STL list into 
//                  message linked by next field
// PARAMETERS::
//    + node:       pointer to the network node
//    + msgList:    A STL list containing the messages to be linked
// RETURN::         The first message in the message list, 
//                  the rest messages are linked
//                  via field next
// **/
Message* UmtsLinkMessages(
    Node* node,
    std::list<Message*>& msgList);

// /**
// FUNCTION::       UmtsUnLinkMessages
// LAYER::          ALL
// PURPOSE::        unlink a list of Messages into an STL list
// PARAMETERS::
//    + node:       pointer to the network node
//    + msgHead:    the pointe to the first message in the linked message list
//    + msgList:    An empty STL list to be used to contain messages
// RETURN::         NULL
// **/
void UmtsUnLinkMessages(
    Node* node,
    Message* msgHead,
    std::list<Message*>& msgList);

// /**
// FUNCTION   :: UmtsGetTotalPktSizeFromMsgList
// LAYER      :: ALL
// PURPOSE    :: Calculate the total Pakcet size in a messagge list.
// PARAMETERS ::
// + msgList  :: Message* : pointer to the message list
// RETURN     :: int : total packet size of teh message list
// **/
int UmtsGetTotalPktSizeFromMsgList(Message* msgList);

#endif /* _CELLULAR_UMTS_H_ */
