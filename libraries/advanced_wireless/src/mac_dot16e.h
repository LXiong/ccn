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

#ifndef MAC_DOT16e_H
#define MAC_DOT16e_H

//
// This is the main header file of the implementation of
// IEEE 802.16e MAC, as an extension to mac_dot16.h
//

//--------------------------------------------------------------------------
// type definitions
//--------------------------------------------------------------------------

//
// Constants for IEEE 802.16e (mobility) support
//

// /**
// CONSTANT    :: DOT16e_DEFAULT_MAX_NBR_BS : 20
// DESCRIPTION :: MAX number of neighbor BS
// **/
#define DOT16e_DEFAULT_MAX_NBR_BS    20

// /**
// CONSTANT    :: DOT16e_DEFAULT_NBR_SCAN_RSS_TRIGGER : -76.0 dBm
// DESCRIPTION :: RSS level to trigger neighbor BS scan
// **/
#define DOT16e_DEFAULT_NBR_SCAN_RSS_TRIGGER   (-76.0)

// /**
// CONSTANT    :: DOT16e_DEFAULT_HO_RSS_TRIGGER : -78.0 dBm
// DESCRIPTION :: RSS level to trigger handover decision
// **/
#define DOT16e_DEFAULT_HO_RSS_TRIGGER   (-78.0)

// /**
// CONSTANT    :: DOT16e_DEFAULT_TRIGGER_AVG_DURATION : 200
// DESCRIPTION :: average duration for trigger (UNIT: MILLI_SECOND)
// **/
#define DOT16e_DEFAULT_TRIGGER_AVG_DURATION 200

// /**
// CONSTANT    :: DOT16e_DEFAULT_HO_RSS_MARGIN : 1.0 dBm
// DESCRIPTION :: Margin in RSS level for choosing the new BS
// **/
#define DOT16e_DEFAULT_HO_RSS_MARGIN   1.0

// constants for trigger

// /**
// CONSTANT    :: DOT16e_TRIGGER_METRIC_TYPE_CINR
// DESCRIPTION :: CINR is used as trigger type
// **/
#define DOT16e_TRIGEGER_METRIC_TYPE_CINR     0x0

// /**
// CONSTANT    :: DOT16e_TRIGGER_METRIC_TYPE_RSSI
// DESCRIPTION :: RSSI is used as trigger type
// **/
#define DOT16e_TRIGEGER_METRIC_TYPE_RSSI     0x1

// /**
// CONSTANT    :: DOT16e_TRIGGER_METRIC_TYPE_RTD
// DESCRIPTION :: RTD is used as trigger type
// **/
#define DOT16e_TRIGEGER_METRIC_TYPE_RTD     0x2

// /**
// CONSTANT    :: DOT16e_TRIGGER_METRIC_FUNC_NBR_GREATER_ABSO
// DESCRIPTION :: Metric of NBR BS is greater than absolute value
// **/
#define DOT16e_TRIGEGER_METRIC_FUNC_NBR_GREATER_ABSO     0x1

// /**
// CONSTANT    :: DOT16e_TRIGGER_METRIC_FUNC_NBR_LESS_ABSO
// DESCRIPTION :: Metric of NBR BS is less than absolute value
// **/
#define DOT16e_TRIGEGER_METRIC_FUNC_NBR_LESS_ABSO     0x2

// /**
// CONSTANT    :: DOT16e_TRIGGER_METRIC_FUNC_NBR_GREATER_SERV
// DESCRIPTION :: Metric of NBR BS > serving BS by realtive vlaue
// **/
#define DOT16e_TRIGEGER_METRIC_FUNC_NBR_GREATER_SERV     0x3

// /**
// CONSTANT    :: DOT16e_TRIGGER_METRIC_FUNC_NBR_LESS_SERV
// DESCRIPTION :: Metric of NBR BS < serving BS by realtive vlaue
// **/
#define DOT16e_TRIGEGER_METRIC_FUNC_NBR_LESS_SERV     0x4

// /**
// CONSTANT    :: DOT16e_TRIGGER_METRIC_FUNC_SERV_GREATER_ABSO
// DESCRIPTION :: Metric of serving BS is greater than absolute value
// **/
#define DOT16e_TRIGEGER_METRIC_FUNC_SERV_GREATER_ABSO     0x5

// /**
// CONSTANT    :: DOT16e_TRIGGER_METRIC_FUNC_SERV_LESS_ABSO
// DESCRIPTION :: Metric of serving BS is less than absolute value
// **/
#define DOT16e_TRIGEGER_METRIC_FUNC_SERV_LESS_ABSO     0x6

// action performed upon reacing trigger consition
// /**
// CONSTANT    :: DOT16e_TRIGGER_ACTION_MOB_SCN_REP
// DESCRIPTION :: Respond with MOB_SCN-REP after scanning interval
// **/
#define DOT16e_TRIGGER_ACTION_MOB_SCN_REP   0x1

// /**
// CONSTANT    :: DOT16e_TRIGGER_ACTION_MOB_MSHO_REQ
// DESCRIPTION :: Respond with MOB_MSHO-REQ
// **/
#define DOT16e_TRIGGER_ACTION_MOB_MSHO_REQ   0x2

// /**
// CONSTANT    :: DOT16e_TRIGGER_ACTION_MOB_SCN_REQ
// DESCRIPTION :: Respond with MOB_SCN-REQ
// **/
#define DOT16e_TRIGGER_ACTION_MOB_SCN_REQ   0x3

// /**
// CONSTANT    :: DOT16e_MAX_HoReqRetry    3
// DESCRIPTION :: Max numberof retry to send HO REQ
// **/
#define DOT16e_MAX_HoReqRetry    3

// /**
// CONSTANT    :: DOT16e_MAX_HoReqRetry    3
// DESCRIPTION :: Max numberof retry to send SCAN REQ
// **/
#define DOT16e_MAX_ScanReqRetry    3

// /**
// CONSTANT    :: DOT16e_RSSI_BASE       -103.75
// DESCRIPTION :: base of the received signal strength (in dBm)
// **/
#define DOT16e_RSSI_BASE    -103.75

// /**
// CONSTANT    :: DOT16e_HO_SYTEM_RESOURCE_RETAIN_TIME  (1 * SECOND)
// DESCRIPTION :: The duration that BS retain the resource
//                for a outgoing handover MS
// **/
#define DOT16e_HO_SYTEM_RESOURCE_RETAIN_TIME   (1 * SECOND)

// /**
// CONSTANT    :: DOT16e_BS_INIT_HO_RSP_WAIT_TIME    ( 3 * SECOND)
// DESCRIPTION :: The waiting time before BS reset its state after send
//                out a BS HO REQ
// **/
#define DOT16e_BS_INIT_HO_RSP_WAIT_TIME   (3 * SECOND)

// /**
// CONSTANT    :: DOT16e_DEFAULT_PAGING_HASHSKIP_THRESHOLD   0
// DESCRIPTION :: Default value of MAC hash skip threshold
// **/
#define DOT16e_DEFAULT_PAGING_HASHSKIP_THRESHOLD   0

// /**
// CONSTANT    :: DOT16e_DEFAULT_PAGING_INTERVAL_LENGTH   5
// DESCRIPTION :: Time interval of paging duratin of the BS
// **/
#define DOT16e_BS_DEFAULT_PAGING_INTERVAL_LENGTH   5

// /**
// CONSTANT    :: DOT16e_DEFAULT_PAGING_CYCLE   200
// DESCRIPTION :: Time interval of paging cycle of the BS
// **/
#define DOT16e_BS_DEFAULT_PAGING_CYCLE   200

// /**
// CONSTANT    :: DOT16e_DEFAULT_PAGING_OFFSET   5
// DESCRIPTION :: Time interval of paging offset of the BS
// **/
#define DOT16e_BS_DEFAULT_PAGING_OFFSET   5

// /**
// CONSTANT    :: DOT16e_BS_DEFAULT_PAGING_GROUP_ID   1
// DESCRIPTION :: Default Paging Group ID for the BS
// **/
#define DOT16e_BS_DEFAULT_PAGING_GROUP_ID 1

// /**
// CONSTANT    :: DOT16e_D24_POLYNOMIAL   0xC3267D800000
// DESCRIPTION :: D24 polynomial (0x18364CFB) followed by 23 0s
// **/
#define DOT16e_D24_POLYNOMIAL   0xC3267D800000ULL

// /**
// CONSTANT    :: DOT16e_MAC_HASH_LENGTH : 3
// DESCRIPTION :: Mac Has is 24-bit calculated using D24
// **/
#define DOT16e_MAC_HASH_LENGTH   3

// /**
// CONSTANT    :: DOT16e_PC_MAX_SS_IDLE_INFO    255
// DESCRIPTION :: The maximum number of SSs to which a PC will keep idle info
// **/
#define     DOT16e_PC_MAX_SS_IDLE_INFO     255

#define MacDot16eMacAddressToUInt64(macAddr) \
            ((((UInt64)((macAddr)[0])) << 40) | \
            (((UInt64)((macAddr)[1])) << 32) | \
            (((UInt64)((macAddr)[2])) << 24) | \
            (((UInt64)((macAddr)[3])) << 16) | \
            (((UInt64)((macAddr)[4])) << 8) | \
            ((UInt64)((macAddr)[5])))

#define MacDot16eUInt64ToMacHash(macHash, uint64val) \
            (macHash)[0] = (unsigned char)((uint64val) >> 16); \
            (macHash)[1] = (unsigned char)((uint64val) >> 8); \
            (macHash)[2] = (unsigned char)(uint64val)

#define MacDot16eSameMacHash(hash1, hash2) \
            ((hash1)[0] == (hash2)[0] && (hash1)[1] == (hash2)[1] && \
             (hash1)[2] == (hash2)[2])
//
// End of constants for IEEE 802.16e support
//

//--------------------------------------------------------------------------
// Type enum definitions
//--------------------------------------------------------------------------
// /**
// ENUM        :: MacDot16eScanType
// DESCRIPTION :: Scanning type of SS
// **/
typedef enum
{
    DOT16e_SCAN_SacnNoAssoc = 0,
    DOT16e_SCAN_ScanAssocLevel1 = 1,
    DOT16e_SCAN_ScanAssocLevel2 = 2,
    DOT16e_SCAN_ScanAssocLevel3 = 3,
}MacDot16eScanType;

// /**
// ENUM        :: MacDot16eScanReportMode
// DESCRIPTION :: Report mode
// **/
typedef enum
{
    DOT16e_SCAN_NoReport = 0,
    DOT16e_SCAN_PeriodicReport = 1,
    DOT16e_SCAN_EventTrigReport = 2,
}MacDot16eScanReportMode;

// /**
// ENUM        :: MacDot16eReportMetric
// DESCRIPTION :: Report Metric used in MSHO_REQ
// **/
typedef enum
{
    DOT16e_MSHO_ReportMetricCINR = 1,
    DOT16e_MSHO_ReportMetricRSSI = 2,
    DOT16e_MSHO_ReportMetricRelativeDelay = 4,
    DOT16e_MSHO_ReportMetricRTD = 8,
}MacDot16eReportMetric;

// /**
// ENUM        :: MacDot16eBsHoRspMode
// DESCRIPTION :: mode used in BSHO_RSP/BSHO-REQ
// **/
typedef enum
{
    DOT16e_BSHO_MODE_HoRequest = 0,
    DOT16e_BSHO_MODE_MdhoAnchorCid = 1,
    DOT16e_BSHO_MODE_MdhoAnchorNoCid = 2,
    // ...

}MacDot16eBsHoRspMode;

// /**
// ENUM        :: MacDot16eBsHoOperationMode
// DESCRIPTION :: operation mode of HO
// **/
typedef enum
{
    DOT16e_BSHO_RecomHo = 0,
    DOT16e_BSHO_MandHo = 1,
}MacDot16eBsHoOperationMode;

// /**
// ENUM        :: MacDot16eResrcRetainFlag
// DESCRIPTION :: serving BS retain resource or not when rcvd HoReq
// **?
typedef enum
{
    DOT16e_HO_ResrcRelease = 0,
    DOT16e_HO_ResrcRetain = 1,
}MacDot16eResrcRetainFlag;

// /**
// ENUM        :: MacDot16eHoIndMode
// DESCRIPTION :: HO indication mode
// **/
typedef enum
{
    DOT16e_HOIND_Ho = 0,
    DOT16e_HOIND_AnchorBsUpdate = 1,
    DOt16e_HOIND_DiversitySetUpdate = 2,
}MacDot16eHoInMode;

// /**
// ENUM        :: MacDot16eHoIndType
// DESCRIPTION :: Handover indication type
// **/
typedef enum
{
    DOT16e_HOIND_ReleaseServBs = 0,
    DOT16e_HOIND_HoCancel = 1,
    Dot16e_HOIND_HOReject = 2,
}MacDot16eHoIndType;

// /**
// ENUM        :: MacDot16eDregReqCode
// DESCRIPTION :: DREG-REQ req code
// **/
typedef enum
{
    DOT16e_DREGREQ_FromBS = 0,  // Simple de-registration
    DOT16e_DREGREQ_MSReq = 1,   // De-registration and Idle mode
    DOT16e_DREGREQ_MSRes = 2,   // response to BS initiated idle mode
}MacDot16eDregReqCode;

// /**
// ENUM        :: MacDot16eDregCmdActCode
// DESCRIPTION :: DREG-CMD Action code
// **/
typedef enum
{
    DOT16e_DREGCMD_Code0 = 0,    //SS shall immediately terminate service
                //with the BS and should attempt network entry at another BS.

    DOT16e_DREGCMD_Code1 = 1,   //SS shall listen to the current BS but shall
                //not transmit until an RES-CMD message or DREG-CMD with
                //Action Code 02 or 03 is received.

    DOT16e_DREGCMD_Code2 = 2,   //SS shall listen to the current BS but only
                //transmit on the Basic, and Primary Management Connections.

    DOT16e_DREGCMD_Code3 = 3,   //SS shall return to normal operation and may
                //transmit on any of its active connections.

    DOT16e_DREGCMD_Code4 = 4,   //SS shall terminate current Normal
                //Operations with the BS; the BS shall transmit this action
                //code only in response to any SS DREG-REQ message.

    DOT16e_DREGCMD_Code5 = 5,   //MS shall immediately begin
                //de-registration from serving BS and request initiation of
                //MS Idle Mode.

    DOT16e_DREGCMD_Code6 = 6,   //The MS may retransmit the DREG-REQ message
                //after the time duration (REQ-duration) provided in the msg

    DOT16e_DREGCMD_Code7 = 7,   //The MS shall not retransmit the DREG-REQ
                //message and shall wait the DREG-CMD message. BS transmittal
                //of a subsequent DREG-CMD with Action Code 03 shall cancel
                //this restriction.

}MacDot16eDregCmdActCode;

// /**
// ENUM        :: MacDot16eIdleModeRetain
// DESCRIPTION :: Idle Mode retain info
// **/
typedef enum
{
    DOT16e_RETAIN_Sbc = 1,
    DOT16e_RETAIN_Pkm = 2,
    DOT16e_RETAIN_Reg = 4,
    DOT16e_RETAIN_Nw =  8,
    DOT16e_RETAIN_Tod = 16,
    DOT16e_RETAIN_Tftp = 32,
    DOT16e_RETAIN_srv = 64,
    DOT16e_RETAIN_PgPrf = 128
}MacDot16eIdleModeRetain;

// /**
// ENUM        :: MacDot16eMobPagAdvActCode
// DESCRIPTION :: MOB_PAG-ADV action code
// **/
typedef enum
{
    DOT16e_MOB_PAG_ADV_NoAct = 0,
    DOT16e_MOB_PAG_ADV_PerRng = 1,
    DOT16e_MOB_PAG_ADV_EntNw = 2
}MacDot16eMobPagAdvActCode;
//--------------------------------------------------------------------------
// TLVs for dot16e (start)
//--------------------------------------------------------------------------
// /**
// ENUM        :: MacDot16eMobNbrAdvTlvType
// DESCRIPTION :: TLV types in MOB_NBR_ADV msg
// **/
typedef enum
{
    TLV_MOB_NBR_ADV_DcdSettings = 1,           // DCD settings
    TLV_MOB_NBR_ADV_UcdSettings = 2,           // UCD settings
    TLV_MOB_NBR_ADV_NbrBsTrigger = 4,          // neighbor BS trigger
}MacDot16eMobNbrAdvTlvType;

// /**
// ENUM        :: MacDot16eMobNbrAdvTriggerTlvType
// DESCRIPTION :: TLV types for triggers in MOB_NBR_ADV msg
// **/
typedef enum
{
    TLV_MOB_NBR_ADV_TriggerTypeFuncAction = 1, // trigger type/func/action
    TLV_MOB_NBR_ADV_TriggerValue = 2,          // trigger value
    TLV_MOB_NBR_ADV_TriggerAvgDuration = 3,    // trigger average duration
}MacDot16eMobNbrAdvTriggerTlvType;

// /**
// ENUM        :: MacDot16eDregTlvType
// DESCRIPTION :: TLV types for DREG REQ/CMD
// **/
typedef enum
{
    TLV_DREG_PagingInfo = 1,    // Paging cycle, offset, groupid
    TLV_DREG_ReqDuration = 2,   // Waiting value for dreg
    TLV_DREG_PagingCtrlId = 3,  // Paging Controller ID
    TLV_DREG_IdleModeRetainInfo = 4,    // Idle mode retain information
    TLV_DREG_MacHashSkipThshld = 5,     //MAC Hask skip Threshold
    TLV_DREG_PagingCycleReq = 52,       //Requested cycle
}MacDot16eDregTlvType;
//--------------------------------------------------------------------------
// TLVs for dot16e (end)
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
// 802.16e(mobility) related management messages
//
//--------------------------------------------------------------------------

// /**
// STRUCT      :: MacDot16eMobNbrAdvMsg
// DESCRIPTION :: Structure of the neighbor advertisement message
// **/
typedef  struct
{
        unsigned char type;               // msg type, DOT16e_MOB_NBR_ADV
        unsigned char opFiledBitMap;
        unsigned char configChangeCount;  // inc when asso.
                                          // nbr BS change info

        unsigned char fragmentaion;

        // Be sure to put numNeighbors as the last one member
        unsigned char numNeighbors;       // number of neighbor BSs

}MacDot16eMobNbrAdvMsg;

// /**
// STRUCT      :: MacDot16eMobScnReqMsg
// DESCRIPTION :: Structure of the scanning interval request msg
// **/
typedef struct
{
    unsigned char type;        // message type, must be DOT16e_MOB_SCN_REQ
    unsigned char duration;    // duration of the scan period, unit: frames
    unsigned char interval;    // interleaving interval, in # of frames
    unsigned char iteration;   // # of iterating scanning intervals

    unsigned char numBsIndex;  // # of nbr BS (by BS index) to be scanned
    unsigned char numBsId;     // # of nbr BS (by BS IDs) to be scanned

    // variable size of indices of BSs to be scanned

    // variable size of IDs of BSs to be scanned

    // variable size TLVs
} MacDot16eMobScnReqMsg;

// /**
// STRUCT      :: MacDot16eMobScnRspMsg
// DESCRIPTION :: Structure of the scanning interval response msg
// **/
typedef struct
{
    unsigned char type;     // message type, must be DOT16e_MOB_SCN_RSP
    unsigned char duration; // duration of the scan period, in # of frames

    unsigned char reportMode : 2, // report mode,
                  reserved   : 6; // shalle be set to zero
    unsigned char reportPeriod;   // report period in frames
    unsigned char reportMetric;   // Indicating metrics for report trigger

    // variable size, if duration is not 0, description of the allocated
    // durations and recommended BSs

    // variable size TLVs
} MacDot16eMobScnRspMsg;

// /**
// STRUCT      :: MacDot16eMobScnRspInterval
// DESCRIPTION :: Description of the allocated scanning interval
// **/
typedef struct
{
    unsigned char startFrame : 4, // interval starts after these frames
                  reserved : 1,   // reserved
                  padding : 3;
    unsigned char interval;       // interleaving interval, in # of frames
    unsigned char iteration;      // # of iterating scanning intervals

    unsigned char numBsIndex;     // # of nbr BSs (by BS index) to be
                                  // scanned
    unsigned char numBsId;        // # of nbr BSs (by BS IDs) to be scanned

    // variable size of indices of BSs to be scanned

    // variable size of IDs of BSs to be scanned
} MacDot16eMobScnRspInterval;

// /**
// STRUCT      :: MacDot16eMobScnRepMsg
// DESCRIPTION ::Structure of the MOB_SCN_REP
// **/
typedef struct
{
    unsigned char type;
    unsigned char reportMode : 1, // report mode, 0 event-driven; 1 periodic
                  comp_Nbr_BsId_Ind : 1,
                  num_current_BSs : 3,
                  padding1 : 3;
    unsigned char reportMetric;  // bitmap for the presence of cetain
                                 // metrics
    unsigned char numBsIndex;     // # of nbr BSs (by BS index) to be
                                  // scanned
    unsigned char numBsId;        // # of nbr BSs (by BS IDs) to be scanned

} MacDot16eMobScnRepMsg;

// /**
// STRUCT      :: MacDot16eMobMshoReqMsg
// DESCRIPTION :: Structure of the MOB_MSHO-REQ message
// **/

typedef struct
{
    unsigned char type;           // msg type, must be DOT16e_MOB_MSHO_REQ
    unsigned char reportMetric;   // indicating presence of metric in msg
    unsigned char numNewBsIndex;  // # of new recommended BSs
    unsigned char numNewBsId;     // # of new recommned BSs with full BS ID

    // variable size of BS information
} MacDot16eMobMshoReqMsg;

// /**
// STRUCT      :: MacDot16eMobBshoReqMsg
// DESCRIPTION :: Structure of the MOB_BSHO_REQ message
// **/

typedef struct
{
    unsigned char type;            // msg type, must be DOT16e_MOB_BSHO_REQ
    unsigned char netAssistHo : 1, // indicate serving BS support
                                   // network assisted HO
                         mode : 3, // HO mode
                     padding1 : 4; // shall be set to 0
    unsigned char    hoOpMode : 1, // HO operation mode, 0 recm, 1 mandatory
              resrcRetainFlag : 1, // resource retain flag at the serving BS
                     padding2 : 6; // shall be set to 0

    unsigned char  actionTime;     // # of frames to wait before target BS
                                   // allocate dedicated range opp

    // various TLVs

} MacDot16eMobBshoReqMsg;


// /**
// STRUCT      :: MacDot16eMobBshoRspMsg
// DESCRIPTION :: Structure of the MOB_BSHO_RSP message
// **/

typedef struct
{
    unsigned char type;            // msg type, must be DOT16e_MOB_BSHO_RSP
    unsigned char        mode : 3, // ho request mode
                     reserved : 5;
    unsigned char    hoOpMode : 1, // HO operation mode, 0 recm, 1 mandatory
              resrcRetainFlag : 1, // resource retain flag at the serving BS
                     padding2 : 6; // shall be set to 0
    unsigned char actionTime;      // # of frames to wait  before target BS
                                   // allocate dedicated range opp
    // variable size of BS information
} MacDot16eMobBshoRspMsg;


// /**
// STRUCT      :: MacDot16eMobHoIndMsg
// DESCRIPTION :: Structure of the MOB_HO-IND message
// **/

typedef struct
{
    unsigned char type;      // message type, must be DOT16e_MOB_HO_IND;
    unsigned char reserved : 6,  // reserved, shall be set to zero
                  mode : 2;  // HO mode,
                             //   0b00 : HO,
                             //   0b01 : MDHO/FBSS: Anchor BS update
                             //   0b10 : MDHO/FBSS: Diversity Set update
                             //   0b11 : reserved
    unsigned char preambleIndex; // PHY specific preable for target BS

    // different fields based on mode

    // variable size padding

    // variable size TLVs
} MacDot16eMobHoIndMsg;

// /**
// STRUCT      :: MacDot16eMobHoIndHoMod
// DESCRIPTION :: Structure of the fields when mode field is 0b00 in
//                the MOB_HO-IND message
// **/

typedef struct
{
    unsigned char hoIndType : 2, // HO-IND type:
                                 // 0b00:serving BS release,
                                 // 0b01: HO cancel,
                                 // 0b10: HO reject,
                                 // 0b11: reserved
                  rangeParaValidInd : 2, // Ranging params valid indicator
                  reserved : 4;

    // if hoIndType is 0b00, add target BS ID, 48 bits
} MacDot16eMobHoIndHoMod;

// /**
// STRUCT      :: MacDot16eDregReqMsg
// DESCRIPTION :: Structure of the DREG-REQ message
// **/
typedef struct
{
    unsigned char type;
    unsigned char reqCode;

    //variable size TLVs
} MacDot16eDregReqMsg;

// /**
// STRUCT      :: MacDot16eDregCmdMsg
// DESCRIPTION :: Structure of the DREG-CMD message
// **/
typedef struct
{
    unsigned char type;
    unsigned char actCode;  //action code

    //variable size TLVs
} MacDot16eDregCmdMsg;

// /**
// STRUCT      :: MacDot16eMobPagAdvMsg
// DESCRIPTION :: Structure of the MOB_PAG-ADV
// **/
/*
typedef struct
{
    unsigned char type;        // message type, must be DOT16e_MOB_PAG_ADV
    UInt16 numPagingGrpId;     // number of paging group ids
    UInt16 numMac;             // number of MAC Address Hash

    // variable size of Paging group Ids
    // variable size of MAC Hash
    // variable size padding
    // variable size TLVs
} MacDot16eMobPagAdvMsg;
*/
// End of IEEE802.16e message structures
//
//--------------------------------------------------------------------------
// structure for dot16e (start)
//--------------------------------------------------------------------------

typedef struct
{
    unsigned char triggerType: 2,    // trigger type
                  triggerFunc: 3,    // trigger fucniton
                triggerAction: 3;    // trigger action
    signed char triggerValue;
    unsigned char triggerAvgDuration; // in UNIT of MILLI_SECOND
}MacDot16eTriggerInfo;

// /**
// STRUCT      :: MacDot16eNbrBsInfo
// /**

typedef struct
{
    int nbrBsIndex;
    NodeAddress bsNodeId;
    Address bsDefaultAddress;
    unsigned char nbrBsId[DOT16_BASE_STATION_ID_LENGTH];
    BOOL isActive;

    unsigned char dcdChangeCount;
    int dlChannel;
    unsigned char ucdChangeCount;
    int ulChannel;

    clocktype rtg;                   // Receive/transmit Transition Gap
    clocktype ttg;                   // Transmit/receive Transition Gap
    clocktype sstg;                  // Subscriber Station Transition Gap

    Int16 bs_EIRP;                   // EIRP, signed in units of 1dBm
    Int16 rssIRmax;                  // RSS, signed in units of 1dBm

    // DL and UL burst profiles, indexed by DIUC or (UIUC - 1)
    MacDot16DlBurstProfile dlBurstProfile[DOT16_NUM_DL_BURST_PROFILES];
    MacDot16UlBurstProfile ulBurstProfile[DOT16_NUM_UL_BURST_PROFILES];

    MacDot16eTriggerInfo trigger;

}MacDot16eNbrBsInfo;

// /**
// STRUCT      :: MacDot16eSSIdleInfo
// DESCIPRTION :: Data structure of SS IDLE mode info kept at PC
// **/
typedef struct struct_mac_dot16e_ss_idle_info_str
{
    BOOL isValid;
    clocktype lastUpdate;
    unsigned char msMacAddress[DOT16_MAC_ADDRESS_LENGTH];
    Address bsIpAddress;
    Address initialBsIpAddress;
    NodeAddress initialBsId;
    NodeAddress BSId;
    UInt8 idleModeRetainInfo;
    UInt16 pagingGroupId;
    UInt16 pagingCycle;
    UInt8 pagingOffset;
    // management CIDs assigned
    Dot16CIDType basicCid;                             // basic mgmt CID
    Dot16CIDType primaryCid;                           // primary mgmt CID
    Dot16CIDType secondaryCid;                         // secondary mgmt CID
    //basic capability of SS
    MacDot16SsBasicCapability ssBasicCapability;

    // auth and key info
    MacDot16SsAuthKeyInfo ssAuthKeyInfo;

    short numCidSupport;

    //Timers
    Message* timerIdleModeSys;      // Idle Mode System Timer
}MacDot16eSSIdleInfo;

// /**
// STRUCT      :: MacDot16ePagingGrpIdInfo
// DESCIPRTION :: Data structure of Paging Grp Id info
// **/
typedef struct struct_mac_dot16e_paging_grpid_info_str
{
    Address bsIpAddress;
    NodeAddress bsId;
    UInt16 pagingGrpId;

    struct struct_mac_dot16e_paging_grpid_info_str* next;
}MacDot16ePagingGrpIdInfo;

// /**
// STRUCT      :: Dot16BackboneStats
// DESCRIPTION :: Structure of Backbone stats
// **/
typedef struct
{
    int numImEntryReqRcvd;
    int numImEntryRspSent;
    int numImExitReqRcvd;
    int numLuReqRcvd;
    int numLuRspSent;
    int numInitiatePagingReqRcvd;
    int numInitiatePagingRspSent;
    int numPagingAnnounceSent;
    int numPagingInfoRcvd;
    int numDeleteMsEntrySent;
} Dot16BackboneStats;

// /**
// STRUCT      :: MacDot16ePagingCtrl
// DESCIPRTION :: Data structure of Paging Controller
// **/
typedef struct struct_mac_dot16e_paging_ctrl_str
{
    MacDot16eSSIdleInfo ssInfo[DOT16e_PC_MAX_SS_IDLE_INFO];
    MacDot16ePagingGrpIdInfo* pgGrpInfo;
    Dot16BackboneStats stats;
}MacDot16ePagingCtrl;

// /**
// STRUCT      :: MacDot16eMsPageInfo
// DESCIPRTION :: Data structure of MS for preparing MOB-PAG_ADV
// **/
typedef struct struct_mac_dot16e_ms_page_info_str
{
    unsigned char msMacAddress[DOT16_MAC_ADDRESS_LENGTH];
    UInt8 actCode;
    BOOL isValid;
}MacDot16eMsPageInfo;

//--------------------------------------------------------------------------
// structure for dot16e (end)
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16eCalculateMacHash
// LAYER      :: MAC
// PURPOSE    :: Calculate MAC Hash value from Mac Address using D24
// PARAMETERS ::
// + macAddress : unsigned char []  : MAC Address
// RETURN     :: UInt64             : MAC Hash value
// **/
static
UInt64 MacDot16eCalculateMacHash(unsigned char macAddress[])
{
    UInt64 remainder;
    UInt64 d24 = DOT16e_D24_POLYNOMIAL;
    int bit;

    remainder = MacDot16eMacAddressToUInt64(macAddress);
    remainder <<= 16;
    d24 <<= 16;
    for (bit = 48; bit > 0; --bit)
    {
        if (remainder & 0x8000000000000000ULL)
        {
            remainder ^= DOT16e_D24_POLYNOMIAL;
        }

        remainder = (remainder << 1);
    }

    remainder >>= 40;
    return remainder;
}
#endif // MAC_DOT16e_H
