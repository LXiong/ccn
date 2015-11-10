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
// PACKAGE     :: UMTS_CONSTANTS
// DESCRIPTION :: Defines constants in UMTS Model
//                This file contains some default cross-layer configuration 
// **/

#ifndef UMTS_CONSTANTS
#define UMTS_CONSTANTS

#include "cellular_umts.h"
//
// RRC constant Reference 3GPP TS 25.331 v7.3.0 pp783-785
//
// 1. CN Information

// /**
// CONSTANT    :: UMTS_MAX_CN_DOMAIN         2
// DESCRIPTION :: max number of CN domains, standard is 4
// **/
#define UMTS_MAX_CN_DOMAIN 2

// 2. UTRAN mobility infomation

// /**
// CONSTANT    :: UMTS_MAX_URA         8
// DESCRIPTION :: max number of URAs in a cell
// **/
#define UMTS_MAX_URA 8

// /**
// CONSTANT    :: UMTS_MAX_RAB_SETUP         16
// DESCRIPTION :: max number of RABs to be established 
// **/
#define UMTS_MAX_RAB_SETUP 16

// 3. UE  information

// /** 
// CONSTANT    :: UMTS_MAX_TRANSACTION   25
// DESCRIPTION :: max number of paraelle transactions in downlink
// **/
#define UMTS_MAX_TRANSACTION   25

// /** 
// CONSTANT    :: UMTS_MAX_PAGE1   8
// DESCRIPTION :: max number of UEs paged in the paging type 1 msg
// **/
#define UMTS_MAX_PAGE1   8

// 4. RB Information 

// /**
// CONSTANT    :: UMTS_MAX_PREDEF_CONFIG 16
// DESCRIPTION :: max number of predefined  configuration
// **/
#define UMTS_MAX_PREDEF_CONFIG 16

// /**
// CONSTANT    :: UMTS_MAX_RB 32
// DESCRIPTION :: max number of RBs
// **/
#define UMTS_MAX_RB 32

// /**
// CONSTANT    :: UMTS_MAX_SRB_SETUP 8
// DESCRIPTION :: max number of Signaling RBs
// **/
#define UMTS_MAX_SRB_SETUP 8

// /**
// CONSTANT    :: UMTS_MAX_RB_PER_RAB 8
// DESCRIPTION :: max number of RBs per RAB
// **/
#define UMTS_MAX_RB_PER_RAB 8

// /**
// CONSTANT    :: UMTS_MAX_RB_ALL_RAB 27
// DESCRIPTION :: max number of non Signaling RBs
// **/
#define UMTS_MAX_RB_ALL_RAB 27

// /**
// CONSTANT    :: UMTS_MAX_RB_PER_TRCH 16
// DESCRIPTION :: max number of RBs per TrCh
// **/
#define UMTS_MAX_RB_PER_TRCH 16

// /**
// CONSTANT    :: UMTS_MAX_LOCH 16
// DESCRIPTION :: max number of Logical Channel ID per type per direction
// **/
#define UMTS_MAX_LOCH 16

// /**
// CONSTANT    :: UMTS_MAX_LOCH_PER_CELL 256
// DESCRIPTION :: max number of Log. Ch. ID per type per direction per cell
// **/
#define UMTS_MAX_LOCH_PER_CELL 256

// /**
// CONSTANT    :: UMTS_MAX_LOCH_TYPE 10
// DESCRIPTION :: max number of Logical Channel type
// **/
#define UMTS_MAX_LOCH_TYPE 10

// /**
// CONSTANT    :: UMTS_MAX_LOCH_PER_RLC 2
// DESCRIPTION :: max number of LogCh per RLC
// **/
#define UMTS_MAX_LOCH_PER_RLC 2

// 5. TrCh information

// /**
// CONSTANT    :: UMTS_MAX_TRCH 32
// DESCRIPTION :: max number of trCh per type used in one direction UL/DL
// **/
#define UMTS_MAX_TRCH 32

// /**
// CONSTANT    :: UMTS_MAX_TRCH_PER_CELL 256
// DESCRIPTION :: max number of DL trCh per cell for each type 
// **/
#define UMTS_MAX_DL_TRCH_PER_CELL 256

// /**
// CONSTANT    :: UMTS_MAX_TRCH_TYPE 9
// DESCRIPTION :: max number of trCh type 
// **/
#define UMTS_MAX_TRCH_TYPE 9

// /**
// CONSTANT    :: UMTS_MAX_TRCH_PRECONFIG 16
// DESCRIPTION :: max number of preconfiged trCh used in one direction UL/DL
// **/
#define UMTS_MAX_TRCH_PRECONFIG 16

// /**
// CONSTANT    :: UMTS_MAX_CCTrCH 8
// DESCRIPTION :: max number of CCTrCh
// **/
#define UMTS_MAX_CCTrCH 8

// /**
// CONSTANT    :: UMTS_MAX_TF 32
// DESCRIPTION :: max number of transport format can be included in the 
//                transport format set for one transport channel
// **/
#define UMTS_MAX_TF 32

// /**
// CONSTANT    :: UMTS_MAX_TFC 1024
// DESCRIPTION :: max number of transport format combination
// **/
#define UMTS_MAX_TFC 1024

// /**
// CONSTANT    :: UMTS_MAX_TFC_SUB 1024
// DESCRIPTION :: max number of transport format combination subset
// **/
#define UMTS_MAX_TFC_SUB 1024

// /**
// CONSTANT    :: UMTS_MAX_SIB_PER_MSG 16
// DESCRIPTION :: max number of complete system information blocks per 
//                SYSTEM INFORMATION msg
// **/
#define UMTS_MAX_SIB_PER_MSG 16

// /**
// CONSTANT    :: UMTS_MAX_SIB 32
// DESCRIPTION :: max number of references to other 
//                SYSTEM INFORMATION block
// **/
#define UMTS_MAX_SIB 32

// 6. phyCH informaiton
// /** 
// CONSTANT    :: UMTS_MAX_AC 16
// DESCRIPTION :: max number of access class
// **/
#define UMTS_MAX_AC 16

// /** 
// CONSTANT    :: UMTS_MAX_ASC 8
// DESCRIPTION :: max number of access service class
// **/
#define UMTS_MAX_ASC 8

// /** 
// CONSTANT    :: UMTS_MAX_ASC_MAP 7
// DESCRIPTION :: max number of AC to ASC map
// **/
#define UMTS_MAX_ASC_MAP 7

// /** 
// CONSTANT    :: UMTS_MAX_PRACH 16
// DESCRIPTION :: max number of PRACH in one cell
// **/
#define UMTS_MAX_PRACH 16

// /** 
// CONSTANT    :: UMTS_MAX_RL 8
// DESCRIPTION :: max number of radio links 
// **/
#define UMTS_MAX_RL 8

// /** 
// CONSTANT    :: UMTS_MAX_EDCH_RL 4
// DESCRIPTION :: max number of EDCH radio links 
// **/
#define UMTS_MAX_EDCH_RL 4

// /** 
// CONSTANT    :: UMTS_MAX_SCCPCH 16
// DESCRIPTION :: max number of S-PCCPCH per cell
// **/
#define UMTS_MAX_SCCPCH 16

// /** 
// CONSTANT    :: UMTS_MAX_DPDCH_UL 6
// DESCRIPTION :: max number of UL DPDCH
// **/
#define UMTS_MAX_DPDCH_UL 6

// /** 
// CONSTANT    :: UMTS_MAX_DPCH_DL 8
// DESCRIPTION :: max number of DL DPCH
// **/
#define UMTS_MAX_DPCH_DL 8

// /** 
// CONSTANT    :: UMTS_MAX_DPCH_DL_PER_CELL 256
// DESCRIPTION :: max number of DL DPCH per cell
// **/
#define UMTS_MAX_DL_DPCH_PER_CELL 256

// /** 
// CONSTANT    :: UMTS_MAX_ACTIVE_SET 3
// DESCRIPTION :: max number of members of active set
// **/
#define UMTS_MAX_ACTIVE_SET 6

// /** 
// CONSTANT    :: UMTS_MAX_DLDPDCH_PER_RAB 6
// DESCRIPTION :: max number of DPDCH channels added for 
//                setting up a RAB
// **/
#define UMTS_MAX_DLDPDCH_PER_RAB 6

// 7. measurement information 

// 8. ohters
// /** 
// CONSTANT    :: UMTS_MAX_SFN 4095
// DESCRIPTION :: max system famre number
// **/
#define UMTS_MAX_SFN 4095

// /** 
// CONSTANT    :: UMTS_MAX_NASPI 16
// DESCRIPTION :: max NSAPI
// **/
#define UMTS_MAX_NASPI 16

// /** 
// CONSTANT    :: UMTS_MAX_TRANS_MOBILE 7
// DESCRIPTION :: max tranaction at UE
// **/
#define UMTS_MAX_TRANS_MOBILE 7

// /**
// DEFAULT TTI for typical Transport channels
#define UMTS_TRANSPORT_BCH_DEFAULT_TTI    20 * MILLI_SECOND 
#define UMTS_TRANSPORT_PCH_DEFAULT_TTI    10 * MILLI_SECOND
#define UMTS_TRANSPORT_FACH_DEFAULT_TTI   10 * MILLI_SECOND
#define UMTS_TRANSPORT_RACH_DEFAULT_TTI   20 * MILLI_SECOND 
#define UMTS_TRANSPORT_DCH_DEFAULT_TTI    10 * MILLI_SECOND
#define UMTS_TRANSPORT_DSCH_DEFAULT_TTI   10 * MILLI_SECOND
#define UMTS_TRANSPORT_USCH_DEFAULT_TTI   10 * MILLI_SECOND
#define UMTS_TRANSPORT_HSDSCH_DEFAULT_TTI 2 * MILLI_SECOND
// **/

// /**
// CONSTANT    :: UMTS_DEFAULT_SIB_INTERVAL : 500 MS
// DESCRIPTION :: Default value of the system information broadcast interval
//                IMPORTANT: This value should be the same as TTI of 
// **/
#define UMTS_DEFAULT_SIB_INTERVAL    UMTS_TRANSPORT_BCH_DEFAULT_TTI

// /**
// CONSTANT
// MAX SLOT FORMAT FOR VARIOUS CHANNEL TYPE
// **/
#define UMTS_MAX_UL_DPCCH_SLOT_FORMAT 9
#define UMTS_MAX_UL_DPDCH_SLOT_FORMAT 7
#define UMTS_MAX_DL_DPCH_SLOT_FORMAT 49
#define UMTS_MAX_PRACH_SLOT_FORMAT 4
#define UMTS_MAX_HSDSCH_SLOT_FORMAT 2


//#define UMTS_DEFAULT_COMMON_PCCH_SF UMTS_SF_256
//#define UMTS_DEFAULT_PRACH_PREAMBLE_SF UMTS_SF_256
//#define UMTS_DEFAULT_PRACH_MSG_SF UMTS_SF_256

/////////
// DPDCH/DPCCH format
////////

// /**
// ENUM        :: UmtsDlDpchSlotFormatType
// DESCRIPTION :: DL DPCH Slot Format
// **/
typedef enum
{
    UMTS_DL_DPCH_SLOT_FORMAT_0 = 0,
    UMTS_DL_DPCH_SLOT_FORMAT_0A = 1,
    UMTS_DL_DPCH_SLOT_FORMAT_0B,
    UMTS_DL_DPCH_SLOT_FORMAT_1,
    UMTS_DL_DPCH_SLOT_FORMAT_1B,
    UMTS_DL_DPCH_SLOT_FORMAT_2,
    UMTS_DL_DPCH_SLOT_FORMAT_2A,
    UMTS_DL_DPCH_SLOT_FORMAT_2B,
    UMTS_DL_DPCH_SLOT_FORMAT_3,
    UMTS_DL_DPCH_SLOT_FORMAT_3A,
    UMTS_DL_DPCH_SLOT_FORMAT_3B,
    UMTS_DL_DPCH_SLOT_FORMAT_4,
    UMTS_DL_DPCH_SLOT_FORMAT_4A,
    UMTS_DL_DPCH_SLOT_FORMAT_4B,
    UMTS_DL_DPCH_SLOT_FORMAT_5,
    UMTS_DL_DPCH_SLOT_FORMAT_5A,
    UMTS_DL_DPCH_SLOT_FORMAT_5B,
    UMTS_DL_DPCH_SLOT_FORMAT_6,
    UMTS_DL_DPCH_SLOT_FORMAT_6A,
    UMTS_DL_DPCH_SLOT_FORMAT_6B,
    UMTS_DL_DPCH_SLOT_FORMAT_7,
    UMTS_DL_DPCH_SLOT_FORMAT_7A,
    UMTS_DL_DPCH_SLOT_FORMAT_7B,
    UMTS_DL_DPCH_SLOT_FORMAT_8,
    UMTS_DL_DPCH_SLOT_FORMAT_8A,
    UMTS_DL_DPCH_SLOT_FORMAT_8B,
    UMTS_DL_DPCH_SLOT_FORMAT_9,
    UMTS_DL_DPCH_SLOT_FORMAT_9A,
    UMTS_DL_DPCH_SLOT_FORMAT_9B,
    UMTS_DL_DPCH_SLOT_FORMAT_10,
    UMTS_DL_DPCH_SLOT_FORMAT_10A,
    UMTS_DL_DPCH_SLOT_FORMAT_10B,
    UMTS_DL_DPCH_SLOT_FORMAT_11,
    UMTS_DL_DPCH_SLOT_FORMAT_11A,
    UMTS_DL_DPCH_SLOT_FORMAT_11B,
    UMTS_DL_DPCH_SLOT_FORMAT_12,
    UMTS_DL_DPCH_SLOT_FORMAT_12A,
    UMTS_DL_DPCH_SLOT_FORMAT_12B,
    UMTS_DL_DPCH_SLOT_FORMAT_13,
    UMTS_DL_DPCH_SLOT_FORMAT_13A,
    UMTS_DL_DPCH_SLOT_FORMAT_13B,
    UMTS_DL_DPCH_SLOT_FORMAT_14,
    UMTS_DL_DPCH_SLOT_FORMAT_14A,
    UMTS_DL_DPCH_SLOT_FORMAT_14B,
    UMTS_DL_DPCH_SLOT_FORMAT_15,
    UMTS_DL_DPCH_SLOT_FORMAT_15A,
    UMTS_DL_DPCH_SLOT_FORMAT_15B,
    UMTS_DL_DPCH_SLOT_FORMAT_16,
    UMTS_DL_DPCH_SLOT_FORMAT_16A,
}UmtsDlDpchSlotFormatType;

// /**
// STRUCT      ::  UmtsDlDpchFieldFormat
// DESCRIPTION :: Dl DPCH field format
// **/
typedef struct
{
    UmtsDlDpchSlotFormatType slotForamt;
    UInt16 chBitRate;
    double chSymbolRate;
    UmtsSpreadFactor sf;
    UInt32 numBitPerSlot;
    UInt32 numData1;
    UInt32  numData2;
    unsigned char numTpc;
    unsigned char numTfci;
    unsigned char numPilot;
    unsigned char maxTxSlotLower;
    unsigned char maxTxSlotUpper;
}UmtsDlDpchFieldFormat;

// /**
// CONSTANT    ::  UmtsDlDpchFieldFormat
// DESCRIPTION ::  Dl DPCH field format defined in standard, total 49
// **/
const UmtsDlDpchFieldFormat 
umtsDlDpchFormat[UMTS_MAX_DL_DPCH_SLOT_FORMAT] =
{  
    {UMTS_DL_DPCH_SLOT_FORMAT_0,                  15,                  7.5,                  UMTS_SF_512,                  10,                  0,                  4,                  2,                  0,                  4,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_0A,                  15,                  7.5,                  UMTS_SF_512,                  10,                  0,                  4,                  2,                  0,                  4,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_0B,                  30,                  15,                  UMTS_SF_256,                  20,                  0,                  8,                  4,                  0,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_1,                  15,                  7.5,                  UMTS_SF_512,                  10,                  0,                  2,                  2,                  2,                  4,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_1B,                  30,                  15,                  UMTS_SF_256,                  20,                  0,                  4,                  4,                  4,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_2,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  14,                  2,                  0,                  2,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_2A,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  14,                  2,                  0,                  2,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_2B,                  60,                  30,                  UMTS_SF_128,                  40,                  4,                  28,                  4,                  0,                  4,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_3,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  12,                  2,                  2,                  2,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_3A,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  10,                  2,                  4,                  2,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_3B,                  60,                  30,                  UMTS_SF_128,                  40,                  4,                  24,                  4,                  4,                  4,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_4,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  12,                  2,                  0,                  4,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_4A,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  12,                  2,                  0,                  4,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_4B,                  60,                  30,                  UMTS_SF_128,                  40,                  4,                  24,                  4,                  0,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_5,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  10,                  2,                  2,                  4,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_5A,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  8,                  2,                  4,                  4,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_5B,                  60,                  30,                  UMTS_SF_128,                  40,                  4,                  20,                  4,                  4,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_6,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  8,                  2,                  0,                  8,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_6A,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  8,                  2,                  0,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_6B,                  60,                  30,                  UMTS_SF_128,                  40,                  4,                  16,                  4,                  0,                  16,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_7,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  6,                  2,                  2,                  8,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_7A,                  30,                  15,                  UMTS_SF_256,                  20,                  2,                  4,                  2,                  4,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_7B,                  60,                  30,                  UMTS_SF_128,                  40,                  4,                  12,                  4,                  4,                  16,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_8,                  60,                  30,                  UMTS_SF_128,                  40,                  6,                  28,                  2,                  0,                  4,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_8A,                  60,                  30,                  UMTS_SF_128,                  40,                  6,                  28,                  2,                  0,                  4,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_8B,                  120,                  60,                  UMTS_SF_64,                  80,                  12,                  56,                  4,                  0,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_9,                  60,                  30,                  UMTS_SF_128,                  40,                  6,                  26,                  2,                  2,                  4,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_9A,                  60,                  30,                  UMTS_SF_128,                  40,                  6,                  24,                  2,                  4,                  4,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_9B,                  120,                  60,                  UMTS_SF_64,                  80,                  12,                  52,                  4,                  4,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_10,                  60,                  30,                  UMTS_SF_128,                  40,                  6,                  24,                  2,                  0,                  8,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_10A,                  60,                  30,                  UMTS_SF_128,                  40,                  6,                  24,                  2,                  0,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_10B,                  120,                  60,                  UMTS_SF_64,                  80,                  12,                  48,                  4,                  0,                  16,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_11,                  60,                  30,                  UMTS_SF_128,                  40,                  6,                  22,                  2,                  2,                  8,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_11A,                  60,                  30,                  UMTS_SF_128,                  40,                  6,                  20,                  2,                  4,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_11B,                  120,                  60,                  UMTS_SF_64,                  80,                  12,                  44,                  4,                  4,                  16,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_12,                  120,                  60,                  UMTS_SF_64,                  80,                  12,                  48,                  4,                  8,                  8,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_12A,                  120,                  60,                  UMTS_SF_64,                  80,                  12,                  40,                  4,                  16,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_12B,                  240,                  120,                  UMTS_SF_32,                  160,                  24,                  96,                  8,                  16,                  16,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_13,                  240,                  120,                  UMTS_SF_32,                  160,                  28,                  112,                  4,                  8,                  8,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_13A,                  240,                  120,                  UMTS_SF_32,                  160,                  28,                  104,                  4,                  16,                  8,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_13B,                  480,                  240,                  UMTS_SF_16,                  320,                  56,                  224,                  8,                  16,                  16,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_14,                  480,                  240,                  UMTS_SF_16,                  320,                  56,                  232,                  8,                  8,                  16,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_14A,                  480,                  240,                  UMTS_SF_16,                  320,                  56,                  224,                  8,                  16,                  16,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_14B,                  960,                  480,                  UMTS_SF_8,                  640,                  112,                  464,                  16,                  16,                  32,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_15,                  960,                  480,                  UMTS_SF_8,                  640,                  120,                  488,                  8,                  8,                  16,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_15A,                  960,                  480,                  UMTS_SF_8,                  640,                  120,                  480,                  8,                  16,                  16,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_15B,                  1920,                  960,                  UMTS_SF_4,                  1280,                  240,                  976,                  16,                  16,                  32,                  8,  14},
    {UMTS_DL_DPCH_SLOT_FORMAT_16,                  1920,                  960,                  UMTS_SF_4,                  1280,                  248,                  1000,                  8,                  8,                  16,                  15, 15},
    {UMTS_DL_DPCH_SLOT_FORMAT_16A,                  1920,                  960,                  UMTS_SF_4,                  1280,                  248,                  992,                  8,                  16,                  16,                  8,  14}
};

// /**
// STRUCT      ::  UmtsUlDpdchFieldFormat
// DESCRIPTION ::  UL DPDCH field format
// **/
typedef struct
{
    unsigned char slotForamt;
    UInt16 chBitRate;
    UInt16 chSymbolRate;
    UmtsSpreadFactor sf;
    UInt32 numBitPerFrame;
    UInt32 numBitPerSlot;
    UInt32 numData;
}UmtsUlDpdchFieldFormat;

// /**
// CONSTANT    ::  UmtsUlDpdchFieldFormat
// DESCRIPTION ::  UL DPDCH field format defined in the standard
// **/
const UmtsUlDpdchFieldFormat umtsUlDpdchFormat[UMTS_MAX_UL_DPDCH_SLOT_FORMAT] =
{
    {0, 15,  15,  UMTS_SF_256, 150,  10,  10},
    {1, 30,  30,  UMTS_SF_128, 300,  20,  20},
    {2, 60,  60,  UMTS_SF_64,  600,  40,  40},
    {3, 120, 120, UMTS_SF_32,  1200, 80,  80},
    {4, 240, 240, UMTS_SF_16,  2400, 160, 160},
    {5, 480, 480, UMTS_SF_8,   4800, 320, 320},
    {6, 960, 960, UMTS_SF_4,   9600, 640, 640}
};

// /**
// ENUM        ::  UmtsUlDpcchSlotFormatType
// DESCRIPTION ::  UL DPCCH slot format 
// **/
typedef enum
{
    UMTS_UL_DPCCH_SLOT_FORMAT_0 = 0,
    UMTS_UL_DPCCH_SLOT_FORMAT_0A,
    UMTS_UL_DPCCH_SLOT_FORMAT_0B,
    UMTS_UL_DPCCH_SLOT_FORMAT_1,
    UMTS_UL_DPCCH_SLOT_FORMAT_2,
    UMTS_UL_DPCCH_SLOT_FORMAT_2A,
    UMTS_UL_DPCCH_SLOT_FORMAT_2B,
    UMTS_UL_DPCCH_SLOT_FORMAT_3  
}UmtsUlDpcchSlotFormatType;

// /**
// STRUCT        ::  UmtsUlDpcchFieldFormat
// DESCRIPTION   ::    UL DPCCH slot format 
// **/
typedef struct
{
    UmtsUlDpcchSlotFormatType slotForamt;
    UInt16 chBitRate;
    double chSymbolRate;
    UmtsSpreadFactor sf;
    unsigned char numBitPerFrame;
    unsigned char numBitPerSlot;
    unsigned char numPilot;
    unsigned char numTpc;
    unsigned char numTfci;
    unsigned char numFbi;
    unsigned char maxTxSlotLower;
    unsigned char maxTxSlotUpper;
}UmtsUlDpcchFieldFormat;

// /**
// STRUCT        ::  UmtsUlDpcchFieldFormat
// DESCRIPTION   ::  UL DPCCH slot format defined in standard 
// **/
const UmtsUlDpcchFieldFormat 
umtsUlDpcchFormat[UMTS_MAX_UL_DPCCH_SLOT_FORMAT] =
{
    {UMTS_UL_DPCCH_SLOT_FORMAT_0,                  15,                  15,                  UMTS_SF_256,                  150,                  10,                  6,                  2,                  2,                  0,                  15, 15},
    {UMTS_UL_DPCCH_SLOT_FORMAT_0A,                  15,                  15,                  UMTS_SF_256,                  150,                  10,                  5,                  2,                  3,                  0,                  10, 14},
    {UMTS_UL_DPCCH_SLOT_FORMAT_0B,                  15,                  15,                  UMTS_SF_256,                  150,                  10,                  4,                  2,                  4,                  0,                  8,  9},
    {UMTS_UL_DPCCH_SLOT_FORMAT_1,                  15,                  15,                  UMTS_SF_256,                  150,                  10,                  8,                  2,                  0,                  0,                  8, 15},
    {UMTS_UL_DPCCH_SLOT_FORMAT_2,                  15,                  15,                  UMTS_SF_256,                  150,                  10,                  5,                  2,                  2,                  1,                  15, 15},
    {UMTS_UL_DPCCH_SLOT_FORMAT_2A,                  15,                  15,                  UMTS_SF_256,                  150,                  10,                  4,                  2,                  3,                  1,                  10, 14},
    {UMTS_UL_DPCCH_SLOT_FORMAT_2B,                  15,                  15,                  UMTS_SF_256,                  150,                  10,                  3,                  2,                  4,                  1,                  8,  9},
    {UMTS_UL_DPCCH_SLOT_FORMAT_3,                  15,                  15,                  UMTS_SF_256,                  150,                  10,                  7,                  2,                  0,                  1,                  8, 15}
};

// /**
// STRUCT        ::  UmtsPrachMsgFieldFormat
// DESCRIPTION   ::  PRACH slot format  
// **/
typedef struct
{
    unsigned char slotForamt;
    UInt16 chBitRate;
    UInt16 chSymbolRate;
    UmtsSpreadFactor sf;
    UInt32 numBitPerFrame;
    UInt32 numBitPerSlot;
    UInt32 numData;
}UmtsPrachMsgFieldFormat;

// /**
// CONSTANT        ::  UmtsPrachMsgFieldFormat
// DESCRIPTION     ::  PRACH slot format defined in standard 
// **/
const UmtsPrachMsgFieldFormat 
umtsPrachMsgFormat[UMTS_MAX_PRACH_SLOT_FORMAT] =
{
    {0, 15,  15,  UMTS_SF_256, 150,  10,  10},
    {1, 30,  30,  UMTS_SF_128, 300,  20,  20},
    {2, 60,  60,  UMTS_SF_64,  600,  40,  40},
    {3, 120, 120, UMTS_SF_32,  1200, 80,  80}
};

// /**
// ENUM          ::  UmtsHsDschSlotFormatType
// DESCRIPTION   ::  HS-DSCH slot format  
// **/
typedef enum 
{
    UMTS_HSDSCH_SLOT_FORMAT_QPSK = 0,
    UMTS_HSDSCH_SLOT_FORMAT_16QAM,
}UmtsHsDschSlotFormatType;

// /**
// STRUCT        ::  UmtsHsdschFieldFormat
// DESCRIPTION   ::  HS-DSCH slot format  
// **/
typedef struct
{
    UmtsHsDschSlotFormatType slotFormat;
    UInt16 chBitRte;
    UInt16 chSymbolRate;
    UmtsSpreadFactor sf;
    UInt32 numBitPerSubframe;
    UInt32 numBitPerSlot;
    UInt32 numData;
}UmtsHsdschFieldFormat;

// /**
// STRUCT        ::  UmtsHsdschFieldFormat
// DESCRIPTION   ::  HS-DSCH slot format defined in the standard 
// **/
const UmtsHsdschFieldFormat 
umtsHsdschFormat[UMTS_MAX_HSDSCH_SLOT_FORMAT] =
{
    {UMTS_HSDSCH_SLOT_FORMAT_QPSK, 480,  240,  UMTS_SF_16, 960,  320,  320},
    {UMTS_HSDSCH_SLOT_FORMAT_16QAM, 960,  240,  UMTS_SF_16, 1920, 640,  640}
};

// /**
// CONSTANT
// DESCRIPTION :: Default slot flomat for various channel type
// **/
// DL
#define UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_512 UMTS_DL_DPCH_SLOT_FORMAT_0
#define UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_256 UMTS_DL_DPCH_SLOT_FORMAT_4
#define UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_128 UMTS_DL_DPCH_SLOT_FORMAT_8
#define UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_64 UMTS_DL_DPCH_SLOT_FORMAT_12
#define UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_32 UMTS_DL_DPCH_SLOT_FORMAT_13
#define UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_16 UMTS_DL_DPCH_SLOT_FORMAT_14
#define UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_8 UMTS_DL_DPCH_SLOT_FORMAT_15
#define UMTS_DEFAULT_DL_DPCH_SLOT_FORMAT_SF_4 UMTS_DL_DPCH_SLOT_FORMAT_16

// UL
#define UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_256 0
#define UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_128 1
#define UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_64 2
#define UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_32 3
#define UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_16 4
#define UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_8 5
#define UMTS_DEFAULT_UL_DPDCH_SLOT_FORMAT_SF_4 6

#define UMTS_DEFAULT_UL_DPCCH_SLOT_FORMAT 0

#define UMTS_DEFAULT_PRACH_SLOT_FORMAT 0

#define UMTS_DEFAULT_HSDSCH_SLOT_FORMAT 1

// /**
// CONSTANT
// DESCRIPTION :: Default transport format for differnt channel and services
// **/
// Default Transport Format for BCH
const UmtsTransportFormat umtsDefaultBchTransFormat = 
          UmtsTransportFormat(246, 246, 
                              UMTS_TRANSPORT_BCH_DEFAULT_TTI /
                                  UMTS_SLOT_DURATION_384,
                              UMTS_CONV_2,
                              UMTS_CRC_16);

// Default Transport Format for PCH
const UmtsTransportFormat umtsDefaultPchTransFormat = 
          UmtsTransportFormat(144, 144, 
                              UMTS_TRANSPORT_PCH_DEFAULT_TTI / 
                                  UMTS_SLOT_DURATION_384,
                              UMTS_CONV_2,
                              UMTS_CRC_16);

// Default Transport Format for FACH
const UmtsTransportFormat umtsDefaultFachTransFormat = 
          UmtsTransportFormat(144, 144, 
                              UMTS_TRANSPORT_FACH_DEFAULT_TTI / 
                                  UMTS_SLOT_DURATION_384,
                              UMTS_CONV_2,
                              UMTS_CRC_16);

// Default Transport Format for RACH
const UmtsTransportFormat umtsDefaultRachTransFormat = 
          UmtsTransportFormat(144, 144, 
                              UMTS_TRANSPORT_RACH_DEFAULT_TTI / 
                                  UMTS_SLOT_DURATION_384,
                              UMTS_CONV_2,
                              UMTS_CRC_16);

// Default Transport Format for Signalling TCH 
// refer to TS25.331  v7.3.0 pp.1122
// 3.4kbps signalling 
const UmtsTransportFormat umtsDefaultSigDchTransFormat_3400 = 
          UmtsTransportFormat(144, 144, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 4 / 
                                  UMTS_SLOT_DURATION_384, // 40 ms
                              UMTS_CONV_3, // convolutional 1/3
                              UMTS_CRC_16);
// 13.6kbps signalling 
const UmtsTransportFormat umtsDefaultSigDchTransFormat_13600 = 
          UmtsTransportFormat(144, 144, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI / 
                                  UMTS_SLOT_DURATION_384, // 10 ms
                              UMTS_CONV_3, // convolutional 1/3
                              UMTS_CRC_16);

// 28.8kpbs Conversational
const UmtsTransportFormat umtsDefaultDataDchTransFormatConv_28800 = 
          UmtsTransportFormat(576, 576 * 2, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 4/ 
                                  UMTS_SLOT_DURATION_384, // 40 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// 32.0kbps Conversational
const UmtsTransportFormat umtsDefaultDataDchTransFormatConv_32000 = 
          UmtsTransportFormat(640, 640 * 1, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 2/ 
                                  UMTS_SLOT_DURATION_384, // 20 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// 64.0kbps Conversational
const UmtsTransportFormat umtsDefaultDataDchTransFormatConv_64000 = 
          UmtsTransportFormat(640, 640 * 2, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 2/ 
                                  UMTS_SLOT_DURATION_384, // 20 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// 128.0kbps Conversational
const UmtsTransportFormat umtsDefaultDataDchTransFormatConv_128000 = 
          UmtsTransportFormat(640, 640 * 4, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 2/ 
                                  UMTS_SLOT_DURATION_384, // 20 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// 14.4kbps Streaming
const UmtsTransportFormat umtsDefaultDataDchTransFormatStream_14400 = 
          UmtsTransportFormat(576, 576 * 1, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 4/ 
                                  UMTS_SLOT_DURATION_384, // 40 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// 28.80kbps Streaming
const UmtsTransportFormat umtsDefaultDataDchTransFormatStream_28800 = 
          UmtsTransportFormat(576, 576 * 2, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 4/ 
                                  UMTS_SLOT_DURATION_384, // 40 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// 57.6kbps Streaming
const UmtsTransportFormat umtsDefaultDataDchTransFormatStream_57600 = 
          UmtsTransportFormat(576, 576 * 4, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 4/ 
                                  UMTS_SLOT_DURATION_384, // 40 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// 115.20kbps Streaming
const UmtsTransportFormat umtsDefaultDataDchTransFormatStream_115200 = 
          UmtsTransportFormat(576, 576 * 8, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 4/ 
                                  UMTS_SLOT_DURATION_384, // 40 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// Background
const UmtsTransportFormat umtsDefaultDataDchTransFormatBackground = 
          UmtsTransportFormat(640, 640 * 1, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 4/ 
                                  UMTS_SLOT_DURATION_384, // 40 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// Interactive
const UmtsTransportFormat umtsDefaultDataDchTransFormatInteractive = 
          UmtsTransportFormat(640, 640 * 1, 
                              UMTS_TRANSPORT_DCH_DEFAULT_TTI * 4/ 
                                  UMTS_SLOT_DURATION_384, // 40 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_16);

// HS_DSCH
const UmtsTransportFormat umtsDefaultDataHsdschTransFormat = 
          UmtsTransportFormat(137, 137 * 1, 
                              UMTS_TRANSPORT_HSDSCH_DEFAULT_TTI / 
                                  UMTS_SLOT_DURATION_384, // 2 ms
                              UMTS_TURBO_3, // turbo 1/3
                              UMTS_CRC_24);

// /**
// STRUCT     :: UmtsHsdpaCqiUeCatMapping
// SECRIPTION :: HSPA Cqi and category mapping 
// **/
typedef struct
{
    unsigned char cqiVal;
    unsigned int transBlkSize;
    unsigned char numHspdsch;
    UmtsModulationType mdType;
    signed char refPowerAdj;
    // int valNir;
    // int valXrv;
}UmtsHsdpaCqiUeCatMapping;

// /**
// CONSTANT     :: UmtsHsdpaCqiUeCatMapping
// SECRIPTION   :: HSPA Cqi and category mapping defined in the standard
// **/
const UmtsHsdpaCqiUeCatMapping umtsHsdpaTransBlkSizeCat1To6[31] = 
{
    {0,  0,    0, UMTS_MOD_QPSK,   0},
    {1,  137,  1, UMTS_MOD_QPSK,   0},
    {2,  173,  1, UMTS_MOD_QPSK,   0},
    {3,  233,  1, UMTS_MOD_QPSK,   0},
    {4,  317,  1, UMTS_MOD_QPSK,   0},
    {5,  377,  1, UMTS_MOD_QPSK,   0},
    {6,  461,  1, UMTS_MOD_QPSK,   0},
    {7,  650,  2, UMTS_MOD_QPSK,   0},
    {8,  792,  2, UMTS_MOD_QPSK,   0},
    {9,  931,  2, UMTS_MOD_QPSK,   0},
    {10, 1262, 3, UMTS_MOD_QPSK,   0},
    {11, 1483, 3, UMTS_MOD_QPSK,   0},
    {12, 1742, 3, UMTS_MOD_QPSK,   0},
    {13, 2279, 4, UMTS_MOD_QPSK,   0},
    {14, 2583, 4, UMTS_MOD_QPSK,   0},
    {15, 3319, 5, UMTS_MOD_QPSK,   0},
    {16, 3565, 5, UMTS_MOD_16QAM,  0},
    {17, 4189, 5, UMTS_MOD_16QAM,  0},
    {18, 4664, 5, UMTS_MOD_16QAM,  0},
    {19, 5287, 5, UMTS_MOD_16QAM,  0},
    {20, 5887, 5, UMTS_MOD_16QAM,  0},
    {21, 6554, 5, UMTS_MOD_16QAM,  0},
    {22, 7168, 5, UMTS_MOD_16QAM,  0},
    {23, 7168, 5, UMTS_MOD_16QAM,  -1},
    {24, 7168, 5, UMTS_MOD_16QAM,  -2},
    {25, 7168, 5, UMTS_MOD_16QAM,  -3},
    {26, 7168, 5, UMTS_MOD_16QAM,  -4},
    {27, 7168, 5, UMTS_MOD_16QAM,  -5},
    {28, 7168, 5, UMTS_MOD_16QAM,  -6},
    {29, 7168, 5, UMTS_MOD_16QAM,  -7},
    {30, 7168, 5, UMTS_MOD_16QAM,  -8}
};

#endif
