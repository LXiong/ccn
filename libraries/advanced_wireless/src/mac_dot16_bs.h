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

#ifndef MAC_DOT16_BS_H
#define MAC_DOT16_BS_H

//
// This is the header file of the implementation of Base Station (BS) of
// IEEE 802.16 MAC
//
#include "mac_dot16.h"
#include "mac_dot16e.h"
//--------------------------------------------------------------------------
// Default values of various parameters
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_FRAME_DURATION: 20MS
// DESCRIPTION :: Default value of the MAC frame duration
// **/
#define DOT16_BS_DEFAULT_FRAME_DURATION    (20 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_TDD_DL_DURATION: 10MS
// DESCRIPTION :: Default value of the duration of DL part in a MAC frame
// **/
#define DOT16_BS_DEFAULT_TDD_DL_DURATION    (10 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_TTG: 10US
// DESCRIPTION :: Default value of the transmit/receive transition gap
// **/
#define DOT16_BS_DEFAULT_TTG    (10 * MICRO_SECOND)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_RTG: 10US
// DESCRIPTION :: Default value of the receive/transmit transition gap
// **/
#define DOT16_BS_DEFAULT_RTG    (10 * MICRO_SECOND)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_SSTG: 4US
// DESCRIPTION :: Default value of the subscriber station transition gap
// **/
#define DOT16_BS_DEFAULT_SSTG    (4 * MICRO_SECOND)

// /**
// CONSTANT    :: DOT16_BS_MAX_DCD_INTERVAL : 10S
// DESCRIPTION :: Maximum interval between transmissions of DCD messages
// **/
#define DOT16_BS_MAX_DCD_INTERVAL    (10 * SECOND)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_DCD_INTERVAL : 5S
// DESCRIPTION :: Default interval between transmissions of DCD messages
// **/
#define DOT16_BS_DEFAULT_DCD_INTERVAL    (5 * SECOND)

// /**
// CONSTANT    :: DOT16_BS_MAX_UCD_INTERVAL : 10S
// DESCRIPTION :: Maximum interval between transmissions of UCD messages
// **/
#define DOT16_BS_MAX_UCD_INTERVAL    (10 * SECOND)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_UCD_INTERVAL : 5S
// DESCRIPTION :: Default interval between transmissions of UCD messages
// **/
#define DOT16_BS_DEFAULT_UCD_INTERVAL    (5 * SECOND)

// /**
// CONSTANT    :: DOT16_BS_MAX_INIT_RANGING_INTERVAL : 2S
// DESCRIPTION :: Max time between initial ranging regions assigned by the
//                BS
// **/
#define DOT16_BS_MAX_INIT_RANGING_INTERVAL    (2 * SECOND)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_INIT_RANGING_INTERVAL : 1S
// DESCRIPTION :: Default time between initial ranging regions assigned by
//                the BS
// **/
#define DOT16_BS_DEFAULT_INIT_RANGING_INTERVAL    (1 * SECOND)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_T9_INTERVAL: 300MS
// DESCRIPTION :: Default value of the timeout interval for waitting the
//                SBC-REQ after intial ranging finished
// **/
#define DOT16_BS_DEFAULT_T9_INTERVAL    (300 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_BS_MIN_T9_INTERVAL: 300MS
// DESCRIPTION :: Min value of the timeout interval for waitting the
//                SBC-REQ after intial ranging finished
// **/
#define DOT16_BS_MIN_T9_INTERVAL    (300 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_BS_MIN_T17_INTERVAL: (5 * MINUTE)
// DESCRIPTION :: Min value of the timeout interval for SS to
//                complete SS authorization and key exchange
// **/
#define DOT16_BS_MIN_T17_INTERVAL    (5 * MINUTE)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_T17_INTERVAL: (5 * MINUTE)
// DESCRIPTION :: Default value of the timeout interval for SS to
//                complete SS authorization and key exchange
// **/
#define DOT16_BS_DEFAULT_T17_INTERVAL    (5 * MINUTE)

// /**
// CONSTANT    :: DOT16_BS_MIN_RNG_RSP_PROC_INTERVAL: 10MS
// DESCRIPTION :: Min value of the timeout interval for waitting
//                to send polling init ranging after last rng-rsp
// **/
#define DOT16_BS_MIN_RNG_RSP_PROC_INTERVAL   (10 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_RNG_RSP_PROC_INTERVAL: 10MS
// DESCRIPTION :: Default value of the timeout interval for waitting
//                to send polling init ranging after last rng-rsp
// **/
#define DOT16_BS_DEFAULT_RNG_RSP_PROC_INTERVAL    (10 * MILLI_SECOND)

// **
// CONSTANT    :: DOT16_BS_MIN_T27_IDLE_INTERVAL
//                DOT16_BS_MIN_REQ_RSP_PROC_INTERVAL
// DESCRIPTION :: Min Value of the maximum time between unicast grants to SS
//                when BS believes SS uplink tx quality is good enough
// **/
#define DOT16_BS_MIN_T27_IDLE_INTERVAL DOT16_BS_MIN_RNG_RSP_PROC_INTERVAL

// **
// CONSTANT    :: DOT16_BS_DEFAULT_T27_IDLE_INTERVAL
//                DOT16_BS_DEFAULT_REQ_RSP_PROC_INTERVAL
// DESCRIPTION :: Min Value of the maximum time between unicast grants to SS
//                when BS believes SS uplink tx quality is good enough
// **/
#define DOT16_BS_DEFAULT_T27_IDLE_INTERVAL \
       (DOT16_BS_DEFAULT_RNG_RSP_PROC_INTERVAL * 5)
// **
// CONSTANT    :: DOT16_BS_MIN_T27_IDLE_INTERVAL
//                DOT16_BS_MIN_REQ_RSP_PROC_INTERVAL
// DESCRIPTION :: Min Value of the maximum time between unicast grants to SS
//                when BS believes SS uplink tx quality is not good enough
// **/
#define DOT16_BS_MIN_T27_ACTIVE_INTERVAL DOT16_BS_MIN_RNG_RSP_PROC_INTERVAL

// **
// CONSTANT    :: DOT16_BS_DEFAULT_T27_IDLE_INTERVAL
//                DOT16_BS_DEFAULT_REQ_RSP_PROC_INTERVAL
// DESCRIPTION :: Min Value of the maximum time between unicast grants to SS
//                when BS believes SS uplink tx quality is not good enough
// **/
#define DOT16_BS_DEFAULT_T27_ACTIVE_INTERVAL    \
        (DOT16_BS_DEFAULT_RNG_RSP_PROC_INTERVAL * 10)

// /**
// CONSTANT    :: DOT16_BS_MIN_UCD_TRANSTION_INTERVAL
//                (2 * DOT16_BS_DEFAULT_FRAME_DURATION)
// DESCRIPTION :: Min value of the time the BS shall wait after repeating
//                a UCD message with incremented change count before issuing
//                a UL MAP message referring to the new DCD message
// **/
#define DOT16_BS_MIN_UCD_TRANSTION_INTERVAL     \
        (2 * DOT16_BS_DEFAULT_FRAME_DURATION)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_UCD_TRANSTION_INTERVAL
//                (2 * DOT16_BS_DEFAULT_FRAME_DURATION)
// DESCRIPTION :: Default value of the time the BS shall wait after
//                repeating a UCD message with incremented change count
//                before issuing a UL MAP message referring to the new
//                DCD message
// **/
#define DOT16_BS_DEFAULT_UCD_TRANSTION_INTERVAL     \
        (2 * DOT16_BS_DEFAULT_FRAME_DURATION)

// /**
// CONSTANT    :: DOT16_BS_MIN_DCD_TRANSTION_INTERVAL
//                (2 * DOT16_BS_DEFAULT_FRAME_DURATION)
// DESCRIPTION :: Min value of the time the BS shall wait after repeating
//                a DCD message with incremented change count before issuing
//                a DL MAP message referring to the new DCD message
// **/
#define DOT16_BS_MIN_DCD_TRANSTION_INTERVAL   \
        (2 * DOT16_BS_DEFAULT_FRAME_DURATION)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_DCD_TRANSTION_INTERVAL
//                (2 * DOT16_BS_DEFAULT_FRAME_DURATION)
// DESCRIPTION :: Default value of the time the BS shall wait after
//                before repeating a DCD message with incremented change
//                count issuing a DL MAP message referring to the new DCD
//                message
// **/
#define DOT16_BS_DEFAULT_DCD_TRANSTION_INTERVAL   \
        (2 * DOT16_BS_DEFAULT_FRAME_DURATION)

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_DL_MEASURE_REPORT_INTERVAL (3 * SECOND)
// DESCRIPTION :: Default Interval for BS to request SS report the DL
//                measurement
// **/
#define DOT16_BS_DEFAULT_DL_MEASURE_REPORT_INTERVAL (3 * SECOND)
////////////////////////////////////////////////////////////////////////////
// start dot16e constant definition
////////////////////////////////////////////////////////////////////////////

// /**
// CONSTANT    :: DOT16e_BS_MOB_ADV_INTERVAL    (3 * SECOND)
// DESCRIPTION :: Nominal time between transmission of MOB_NBR_ADV msg
// **/
#define DOT16_BS_MOB_ADV_ADV_INTERVAL   (3 * SECOND)

// /**
// CONSTANT    :: DOT16e_BS_PAGING_RETRY_COUNT    3
// DESCRIPTION :: Number of retries on paging transmission.
// **/
#define DOT16e_BS_PAGING_RETRY_COUNT   3

// /**
// CONSTANT    :: DOT16e_BS_IDLE_MODE_SYSTEM_TIMEOUT    (4096 * SECOND)
// DESCRIPTION :: Timer interval to receive MS location update
// **/
#define DOT16e_BS_IDLE_MODE_SYSTEM_TIMEOUT   (4096 * SECOND)

// /**
// CONSTANT    :: DOT16e_BS_MGMT_RSRC_HOLDING_TIMEOUT   (500 * MILLI_SECOND)
// DESCRIPTION :: Time the BS maintains connection info with MS after DREG
// **/
#define DOT16e_BS_MGMT_RSRC_HOLDING_TIMEOUT   (500 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16e_BS_DREG_REQ_RETRY_COUNT   3
// DESCRIPTION :: Number of retries for DREG-CMD
// **/
#define DOT16e_BS_DREG_REQ_RETRY_COUNT   3

// /**
// CONSTANT    :: DOT16e_BS_DEFAULT_T46_INTERVAL (250 * MILLI_SECOND)
// DESCRIPTION :: Default waiting time for receiving DREG-REQ
// **/
#define DOT16e_BS_DEFAULT_T46_INTERVAL   (250 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16e_BS_MAX_PAGING_GROUPS    32
// DESCRIPTION :: The maximum number of Paging groups to which a BS might be
//                 associated
// **/
#define DOT16e_BS_MAX_PAGING_GROUPS   32

////////////////////////////////////////////////////////////////////////////
// end dot16e constant definition
////////////////////////////////////////////////////////////////////////////


// /**
// CONSTANT    :: DOT16_BS_SS_HASH_SIZE : m
// DESCRIPTION :: Size of the hash table to hold registered SS list
// **/
#define DOT16_BS_SS_HASH_SIZE    DOT16_CID_BLOCK_M

// /**
// CONSTANT    :: DOT16_MAX_CH_DESC_SENT_IN_TRANSITION   2
// DESCRIPTION :: Max number of Channel description sent in transition
// **/
#define DOT16_MAX_CH_DESC_SENT_IN_TRANSITION    2

// /**
// MACRO       :: MacDot16BsHashBasicCid
// DESCRIPTION :: Hash a basic CID to its index in the SS table
// **/
#define MacDot16BsHashBasicCid(cid) (cid - DOT16_BASIC_CID_START)

// /**
// MACRO       :: MacDot16BsHashPrimaryCid
// DESCRIPTION :: Hash a primary CID to its index in the SS table
// **/
#define MacDot16BsHashPrimaryCid(cid) (cid - DOT16_PRIMARY_CID_START)

// /**
// MACRO       :: MacDot16BsHashTransportCid
// DESCRIPTION :: Hash a transport CID to its storage index
// **/
#define MacDot16BsHashTransportCid(cid) (cid - DOT16_TRANSPORT_CID_START)

//  /**
// STRUCT      :: MacDot16NbrBsSignalMeasInfo
// DESCRIPTION :: Data structure for the NBR bs signal measurement
// **/
typedef struct
{
    unsigned char bsId[DOT16_MAC_ADDRESS_LENGTH]; // bsId
    MacDot16SignalMeasurementInfo dlMeanMeas;  // mean of measument
    // MacDot16SignalMeasurementInfo ulMeanMeas;  // mean of measument
    BOOL bsValid; // whether the SS  still in the range of this BS
} MacDot16NbrBsSignalMeasInfo;

// /**
// STRUCT      :: MacDot16BsSsInfo
// DESCRIPTION :: Data structure for keeping info of a registered SS
// **/
typedef struct mac_dot16_bs_ss_info_str
{
    BOOL managed;                                       // managed SS or not
    unsigned char macAddress[DOT16_MAC_ADDRESS_LENGTH]; // MAC address of SS
    MacDot16MacVer macVersion;                          // Mac version

    // management CIDs assigned
    Dot16CIDType basicCid;                             // basic mgmt CID
    Dot16CIDType primaryCid;                           // primary mgmt CID
    Dot16CIDType secondaryCid;                         // secondary mgmt CID

    // DIUC an UIUC agreed with this SS for DL and UL transmissions
    unsigned char diuc;
    unsigned char uiuc;

    //basic capability of SS
    MacDot16SsBasicCapability ssBasicCapability;

    // auth and key info
    MacDot16SsAuthKeyInfo ssAuthKeyInfo;

    short numCidSupport;

    // whether it has finish the registration
    BOOL isRegistered;

    // the last time a bandwidth grant to the SS for periodic ranging
    clocktype lastRangeGrantTime;

    // Scheduling required variables and information
    BOOL needUcastPoll;   // Whether to alloc req IE for ucast polling
    BOOL needUcastPollNumFrame; // Alloc req IE after certain frames
    int bwGranted;        // Bandwidth granted to this SS for uplink tx
    int mgmtBWGranted;    // Badnwidth granted to this SS's mgmt CIDs

    int basicMgmtBwRequested;        // basic Mgmt BW requested
    int lastBasicMgmtBwRequested;
    int basicMgmtBwGranted;          // basic Mgmt BW granted
    int priMgmtBwRequested;          // primary Mgmt BW requested
    int lastPriMgmtBwRequested;
    int priMgmtBwGranted;            // primary Mgmt BW granted
    int secMgmtBwRequested;          // secondary Mgmt BW requested
    int lastSecMgmtBwRequested;
    int secMgmtBwGranted;            // secondary mgmt BW granted

    UInt16 rangePsAllocated;  // # of PSs alloced in UL part for ranging
    UInt16 requestPsAllocated;// # of PSs alloced in UL part for bw request
    UInt16 dlPsAllocated;     // # of PSs alloced in DL part of cur. frame
    UInt16 ulPsAllocated;     // # of PSs alloced in UL part of cur. frame

    Message* outBuffHead; // Head of list of PDUs scheduled for cur. frame
    Message* outBuffTail; // Tail of list of PDUs scheduled for cur. frame
    int      bytesInOutBuff; // # of bytes of all PDUs scheduled
    // end of scheduling related variables

    // service flows established with this SS. Separated lists are kept
    // for different service types
    MacDot16ServiceFlowList dlFlowList[DOT16_NUM_SERVICE_TYPES];
    MacDot16ServiceFlowList ulFlowList[DOT16_NUM_SERVICE_TYPES];

    // Ranging related variables
    char txPowerAdjust;            // power level adjustment
    BOOL needInitRangeGrant;       // Whether to schedule for init rng
    BOOL initRangePolled;          // init rng polled
    BOOL initRangeCompleted;       // init rng complete or not
    BOOL needPeriodicRangeGrant;   // Whether to schedule for periodic rng
    BOOL ucastTxOppGranted;        // whether a data grant(invited
                                   // range opp/data grant) is allocated
    BOOL ucastTxOppUsed;           // whether SS used the allocated opp

    int invitedRngRetry;           // invited ranging retries
    int rngCorrectRetry;           // ranging correction retries

    BOOL dlBurstRequestedBySs;     // whether SS request DIUC change

    //Timer
    Message* timerT9;              // waiting for SBC-REQ
    Message* timerT27;             // periodic ranging
    Message* timerRngRspProc;      // wait time beofer issue a ucst rng opp
    Message* timerT17;             // timeout interval for SS to complete SS
                                   // authorization and key exchang

    // added for paging/idle mode
    Message* timerT46;             // DREG-REQ receive timer
    Message* timerMgmtRsrcHldg;    // For deleting MS connection info
    Message* timerMSPgngLstng;     // MS paging listening timer

    // signal strength measurement for the serving BS
    // UL
    MacDot16SignalMeasurementInfo ulMeasInfo[DOT16_MAX_NUM_MEASUREMNT];
     // Only CINR is used right now
    clocktype lastUlMeaTime;        // time, last signal quality measure
    double ulCinrMean;              // avg  CINR, in dBm
    double ulRssMean;               // avg RSS, in dBm

    // DL
    MacDot16SignalMeasurementInfo dlMeasInfo[DOT16_MAX_NUM_MEASUREMNT];
    clocktype lastDlMeaReportTime;  // time, last signal quality reported
    double dlCinrMean;              // avg  CINR, in dBm
    double dlRssMean;               // avg RSS, in dBm
    BOOL repReqSent;

    // variables for supporting IEEE 802.16e (mobility)
    BOOL inNbrScan;            // whether the SS is in nbr scan mode or not
    int scanDuration;          // # of frames in a scan interval
    int scanInterval;          // # of frames in an interleaving interval
    int scanIteration;         // # of scan intervals

    int numScanFramesLeft;     // # of frames left for current scan interval
    int numInterFramesLeft;    // # of frames left in this interleaving
                               // interval
    int numScanIterationsLeft; // # of iterations left for whole scan period

    Message* resrcRetainTimer;  // timer to release the retained resource
    BOOL inHandover;            // whether the SS is in handover
    BOOL bsInitHoStart;         // whether BS init a handover
    clocktype lastBsHoReqSent;  // the time when BS init HO req sent
    unsigned char hoTargetBsId[DOT16_BASE_STATION_ID_LENGTH]; // target BS
    unsigned char prevServBsId[DOT16_BASE_STATION_ID_LENGTH];
    MacDot16RngReqPurpose lastRngReqPurpose;

    // DL signal measurement of nbr  BS
    MacDot16NbrBsSignalMeasInfo nbrBsSignalMeas[DOT16e_DEFAULT_MAX_NBR_BS];

    UInt8 mobilitySupportedParameters;
    BOOL isSleepEnabled;
    BOOL mobTrfSent;
    MacDot16ePSClasses psClassInfo[DOT16e_MAX_POWER_SAVING_CLASS];

    //paging/idle
    BOOL isIdle;
    BOOL sentIniPagReq;
    UInt8 idleModeRetainInfo;
    UInt16 pagingCycle;
    UInt16 macHashSkipThshld;
    UInt16 macHashSkipCount;
    UInt16 dregCmdRetCount; // DREG-CMD retry count
    Address pagingController;
    // end of 802.16e

    // pointer to next record falls in the same hash entry
    struct mac_dot16_bs_ss_info_str* next;
} MacDot16BsSsInfo;

// /**
// STRUCT      :: MacDot16BsPara
// DESCRIPTION :: Data structure for storing dot16 BS parameters
// **/
typedef struct
{
    BOOL authPolicySupport;    // support authorization policy (PKM) or not

    clocktype frameDuration;   // duration of a MAC frame
    clocktype tddDlDuration;   // duration of DL part of the MAC frame

    clocktype dcdInterval;     // Time between transmission of DCD messages
    clocktype ucdInterval;     // Time between transmission of UCD messages

    clocktype t7Interval;      // Interval for waiting DSA/DSC/DSD response
    clocktype t8Interval;      // Interval for waiting for DSx acknowledge
    clocktype t10Interval;     // Interval for waiting transaction end

    clocktype t9Interval;            // Timeout value for waitng SBC-REQ
    clocktype rngRspProcInterval;    // Timeout before sending init rng poll
    clocktype t27IdleInterval;       // Perioidic rng, T27 idle interval
    clocktype t27ActiveInterval;     // Periodic rng, T27 active Interval
    clocktype t17Interval;           // Wait SS auth & key exchange
    clocktype ucdTransitionInterval; // Timeout before issue new UCD
    clocktype dcdTransitionInterval; // TImeout before issue new DCD
    clocktype flowTimeout;
    clocktype dlMeasReportInterval;

//added for paging/idle mode
    clocktype t46Interval;
    clocktype tMgmtRsrcHldgInterval;    // For deleting MS connection info
    clocktype tBSPgngInterval;          // BS paging interval timer
    clocktype tMSPgngLstngInterval;     // MS paging listening timer
    UInt8 tDregReqInterval;               // DREG-REQ duration (in frames)
//

    unsigned char dsxReqRetries;     // Max # of retry for dsa/dsc/dsd req
    unsigned char dsxRspRetries;     // Max # of retry for dsa/dsc/dsd rsp

    unsigned short requestOppSize;   // cont. based bw request opp size in
                                     // PS
    unsigned short rangeOppSize;     // cont. based ranging opp size in PS
    unsigned char rangeBOStart;      // Ranging Backoff Start
    unsigned char rangeBOEnd;        // Ranging Backoff End
    unsigned char requestBOStart;    // Request Backoff Start
    unsigned char requestBOEnd;      // Request Backoff End

    clocktype rtg;                   // Receive/transmit Transition Gap
    clocktype ttg;                   // Transmit/receive Transition Gap
    clocktype sstg;                  // Subscriber Station Transition Gap
    int sstgInBytes;                 // translate the SSTG to number of
                                     // bytes

    Int16 bs_EIRP;                   // EIRP, signed in units of 1dBm
    Int16 rssIRmax;                  // RSS, signed in units of 1dBm
    // MacDot16MacVer macVersion;
    int sstgInPs;                    // translate the SSTG to number of
                                     // bytes

    unsigned char contRsvTimeout;    // # UL-MAPs before retry cont based
                                     // rev.

    // dot16e
    clocktype resrcRetainTimeout;    // timeout before releasing resource
    double nbrScanRssTrigger;        // RSS level to trigger neighbor scan
    double hoRssTrigger;             // RSS level to trigger handover
    double hoRssMargin;              // Margin of RSS level to init Bs HO
    //dot16e
    //idle/paging
    UInt16 macHashSkipThshld;
    UInt16 pagingCycle;      // in number of frames
    UInt8 pagingOffset;     // in number of frames
    UInt8 pagingInterval;   // in number of frames
} MacDot16BsPara;


// /**
// STRUCT      :: MacDot16BsStats
// DESCRIPTION :: Data structure for storing dot16 BS statistics
// **/
typedef struct
{
    // Data packet related statistics
    int numPktsFromUpper; // data packets from upper layer
    int numPktsDroppedUnknownSs; // # of packets from upper
                                                // layer to unmanged SS.
                                                // routing is outdated

    int numPktsToLower;   // data packets sent in MAC frames
    int numPktsFromLower; // # of data packets received in MAC frames
    int numPktsDroppedRemovedSs; // # of pakcets dropped at MAC
                                 // queue due to SS has been
                                 // removed from managed list
    int numPktsDroppedSsInHandover; // # of packets dropped
                                    // from MAC queue due to
                                    // SS is now in handover

    // Statistics of management messages
    int numDlMapSent;  // # of DL-MAP messages sent
    int numUlMapSent;  // # of UL-MAP messages sent
    int numDcdSent;    // # of DCD messages sent
    int numUcdSent;    // # of UCD messages sent
    int numRngRspSent; // # of RNG-RSP message sent
    int numRngReqRcvd; // # of RNG-REQ message received
    int numSbcReqRcvd; // # of SBC-REQ message received
    int numSbcRspSent; // # of SBC-RSP message sent
    int numPkmReqRcvd; // # of PKM-REQ message received
    int numPkmRspSent; // # of PKM-RSP message sent
    int numRegReqRcvd; // # of REG-REQ received
    int numRegRspSent; // # of REG-RSP sent
    int numRepReqSent; // # of REP-REQ sent
    int numRepRspRcvd; // # of REP-RSP received

    int numCdmaRngCodeRcvd;   //# of CDMA codes received
    int numCdmaBwRngCodeRcvd; // # of CDMA B/W codes received
    int numARQBlockSent;   //# of ARQ blocks
    int numARQBlockRcvd;   //# of ARQ blocks rcvd
    int numARQBlockDiscard; //# of ARQ block discarded
//--------
    // DSA related statistics
    int numDsaReqRcvd; // # of DSA-REQ received
    int numDsaReqSent; // # of DSA-REQ sent
    int numDsxRvdSent; // # of DSA-RVD sent
    int numDsxRvdRcvd; // # of DSA-RVD received
    int numDsaRspSent; // # of DSA-RSP sent
    int numDsaRspRcvd; // # of DSA-RSP received
    int numDsaAckSent; // # of DSA-ACK sent
    int numDsaAckRcvd; // # of DSA-ACK received
    int numDsaReqRej;  // # of DSA-REQ rejected

    // DSC related statistics
    int numDscReqRcvd; // # of DSC-REQ received
    int numDscReqSent; // # of DSC-REQ sent
    int numDscRspSent; // # of DSC-RSP sent
    int numDscRspRcvd; // # of DSC-RSP received
    int numDscAckSent; // # of DSC-ACK sent
    int numDscAckRcvd; // # of DSC-ACK received
    int numDscReqRej;  // # of DSC-REQ rejected

    // DSD related statistics
    int numDsdReqSent; // # of DSD-REQ sent
    int numDsdReqRcvd; // # of DSD-REQ received
    int numDsdRspSent; // # of DSD-RSP sent
    int numDsdRspRcvd; // # of DSD-RSP received

    // IEEE 802.16e related statistics
    int numInterBsHelloSent;  // # of Inter-BS Hello Msg sent
    int numInterBsHelloRcvd;  // # of INter-BS Hello Msg received
    int numNbrAdvSent;        // # of NBR-ADV sent
    int numNbrScanReqRcvd;    // # of NBR-SCAN-REQ received
    int numNbrScanRspSent;    // # of NBR-SCAN-RSP sent
    int numNbrScanRepRcvd;    // # of NBR-SCAN-REP received
    int numMsHoReqRcvd;       // # of MSHO-REQ received
    int numBsHoRspSent;       // # of BSHO-RSP received
    int numBsHoReqSent;       // # of BSHO-REQ sent
    int numHoIndRcvd;         // # of HO-IND received
    int numInterBsHoFinishNotifyRcvd; // # of inter BS HO finish msg rcvd
    int numInterBsHoFinishNotifySent; // # of inter BS HO finish msg sent

    int numFragmentsRcvd;  // # of fragments received
    int numFragmentsSent;  // # of fragments sent
    int numMobSlpReqRcvd;
    int numMobSlpRspSent;
    int numMobTrfIndSent;
    //paging/idle mode
    int numDregCmdSent;     //number of DREG-CMD sent
    int numDregReqRcvd;     //number of DREG-REQ received
    int numMobPagAdvSent;   //number of MOB_PAG-ADV sent
    int numPktsDroppedSsInIdleMode;

    // Dynamic statistics
    int numPktToPhyGuiId;  // GUI handle for # of pkts sent to PHY
    int numPktFromPhyGuiId;// GUI handle for # of pkts from PHY
    int numPktDropInHoGuiId; // GUI handle for # pkts drop in HO
} MacDot16BsStats;

typedef enum
{
    DOT16_ACU_NONE = 0,
    DOT16_ACU_BASIC = 1,
} MacDot16BsACUScheme;

// /**
// ENUM        :: MacDot16BsChDescUpdateState
// DESCRIPTION :: Channel Descriptor change
// **/
typedef enum
{
    // The init state is for initialization
    // after initialization it will turn to NULL
    // turn to START from NULL when change command rcvd
    // turn to COUNTDOWN when start transmission timer
    // turn to END when timer expire, after sending
    // out the first new generagtion xL-MPA, turn to NULL

    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_INIT = 0,
    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_NULL = 1,
    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_START = 2,
    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_COUNTDOWN = 3,
    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_END = 4
}MacDot16BsChDescUpdateState;

// /**
// STRUCT      :: MacDot16BsChUpdateInfo
// DESCRIPTIO  :: BS channel update infomation
// **/
typedef struct
{
    MacDot16BsChDescUpdateState chUpdateState;
    unsigned char numChDescSentInTransition;
    Message* timerChDescTransition;
}MacDot16BsChUpdateInfo;

// /**
// STRUCT      :: MacDot16BsServiceClassTable
// DESCIPRTION :: Data structure of DOT16 BS' service class
// **/
typedef struct struct_mac_dot16_bs_service_class_table_str
{
    // int serviceClassIndex;
    MacDot16ServiceClass seviceClassName; // service class name,
                                          // such as Premium, gold, silver
    MacDot16QoSParameter qosProfile;      // qos parameters set
    struct_mac_dot16_bs_service_class_table_str* next;
}MacDot16BsServiceClassTable;

// /**
// STRUCT      :: MacDot16BsProvisionedSfTable
// DESCIPRTION :: Data structure of DOT16 BS' provisioned service class
// **/
typedef struct struct_mac_dot16_bs_provisioned_service_flow_table_str
{
    unsigned char macAddress[DOT16_MAC_ADDRESS_LENGTH]; // MAC address of SS
    MacDot16ServiceClass seviceClassName;              // service class name
    MacDot16ServiceFlowDirection sfDirection;      // service flow direction
    struct_mac_dot16_bs_provisioned_service_flow_table_str* next;
} MacDot16BsProvisionedSfTable;

// /**
// STRUCT      :: MacDot16BsData
// DESCRIPTION :: Data structure of Dot16 BS node
// **/
typedef struct struct_mac_dot16_bs_str
{
    // basic properties
    MacDot16MacVer macVersion;

    // DL and UL burst profiles, indexed by DIUC or (UIUC - 1)
    MacDot16DlBurstProfile dlBurstProfile[DOT16_NUM_DL_BURST_PROFILES];
    MacDot16UlBurstProfile ulBurstProfile[DOT16_NUM_UL_BURST_PROFILES];

    // counters for allocating various IDs
    unsigned int serviceFlowId;  // a counter to allocate service flow IDs
    unsigned short transId;      // a counter to allocate transaction ID

    // downlink and uplink channels, should be same for TDD
    int dlChannel;           // downlink channel index
    int ulChannel;           // uplink channel
    // DCD related variables
    BOOL dcdChanged;         // Whether DCD msg content need to be changed
    BOOL dcdScheduled;       // Whether to include DCD in next frame
    unsigned char dcdCount;  // DCD config count to be bcasted, modulo 256
    Message* dcdPdu;         // Store a copy of the DCD message
    clocktype lastDcdSentTime;

    // UCD related variables
    BOOL ucdChanged;         // Whether UCD msg content needs to be changed
    BOOL ucdScheduled;       // Whether to include UCD in next frame
    unsigned char ucdCount;  // UCD config count to be bcasted, modulo 256
    Message* ucdPdu;         // Stores a copy of the UCD message
    clocktype lastUcdSentTime;

    // DL-MAP and  UL-MAP related
    int numDlMapIEScheduled;
    int numUlMapIEScheduled;

    // channel update
    MacDot16BsChUpdateInfo dcdUpdateInfo;
    MacDot16BsChUpdateInfo ucdUpdateInfo;

    // system parameters
    MacDot16BsPara para;

    // BS statistics
    MacDot16BsStats stats;

    // list of SSs registered with this BS
    MacDot16BsSsInfo* ssHash[DOT16_BS_SS_HASH_SIZE];
    int lastSsHashIndex;

    // list of availability transport Cid assigned to this BS
    unsigned char transportCidUsage[DOT16_TRANSPORT_CID_END -
                                    DOT16_TRANSPORT_CID_START + 1];
    Dot16CIDType lastBasicCidAssigned;
    Dot16CIDType lastTransCidAssigned;

    // List of mangaed SS's pre-porvisioned service flow info
    MacDot16BsProvisionedSfTable *provisonedSf;

    // List of service class
    MacDot16BsServiceClassTable *serviceClassTable;

    // DL multicast/broadcast data service flows
    MacDot16ServiceFlowList mcastFlowList[DOT16_NUM_SERVICE_TYPES];

    // scheduling related variables
    Scheduler* outScheduler[DOT16_NUM_SERVICE_TYPES]; // for data packets
    Scheduler* mgmtScheduler;  // for holding management messages
    Message* bcastOutBuffHead; // point to beginning of bcast/mcast PDU list
    Message* bcastOutBuffTail; // tail of the list for easy insertion.
    int bytesInBcastOutBuff;   // # of bytes of all PDUs scheduled
    UInt16 bcastPsAllocated;   // number of bytes allocated for bcast in DL
    UInt16 rangePsAllocated;   // cont. based range opps in this frame
    UInt16 requestPsAllocated; // cont. based BW request opps in this frame

    UInt16 cdmaAllocationIEPsAllocated;
    // burst allocation variables
    UInt16 dlBurstIndex;       // to count number of bursts alloced
    clocktype dlDuration;      // duration of the DL sub-frame in time
    UInt16 numDlPs;            // # of PSs of DL sub-frame on time axis
    char* dlAllocMap;          // allocation map of DL sub-frame

    // start dot16e related
    unsigned char configChangeCount; // it is diff. from dcdCount & ucdCount
                                     // inc 1 whenever nbr change anything
    clocktype lastNbrAdvSent;        // last time sent NbrAdv

    // trigger info
    MacDot16eTriggerInfo trigger;

    // neighbor BS info
    int numNbrBs;
    MacDot16eNbrBsInfo nbrBsInfo[DOT16e_DEFAULT_MAX_NBR_BS];
    MacDot16eResrcRetainFlag resrcRetainFlag;

    //idle/paging
    int numPagingGrp;
    UInt16 pagingGroup[DOT16e_BS_MAX_PAGING_GROUPS];     // paging group id
    MacDot16eMsPageInfo msPgInfo[DOT16e_PC_MAX_SS_IDLE_INFO];
    Message* timerBSPgng;          // BS paging interval timer
    //paging controller address
    Address pagingController;
    MacDot16ePagingCtrl* pgCtrlData;    //populated if the BS is also PC
    // end dot16e related

    BOOL isPackingEnabled; // Is Packing enabled
    BOOL isCRCEnabled;  // Is CRC enabled
    BOOL isFragEnabled; // Is fragmentation enabled
    BOOL isARQEnabled;  // Is ARQ enabled
    MacDot16BsACUScheme acuAlgorithm; // Admission control unit algorithm
    double maxAllowedUplinkLoadLevel; // Uplimit of the load that BS can handle
    double maxAllowedDownlinkLoadLevel; // Download of the load that BS can handle

    MacDot16RangeType rngType;  // type of the station
    MacDot16BWReqType bwReqType;  // type of the station
    MacDot16RngCdmaInfoList* recCDMARngList;
    MacDot16ARQParam* arqParam;
    UInt8 numOfDLBurstProfileInUsed; // FOR CDMA it is 8 else 7
    UInt8 numOfULBurstProfileInUsed;
    UInt16 psSleepId;
    BOOL isSleepEnabled;
    MacDot16ePSClasses psClass3;
    //idle/paging
    BOOL isIdleEnabled;
    int pagingFrameNo;
} MacDot16Bs;


//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16BsGetSsByMacAddress
// LAYER      :: MAC
// PURPOSE    :: Get a record of the SS by its MAC address
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + macAddress : unsigned char* : MAC address of the SS
// RETURN     :: MacDot16BsSsInfo* : Pointer to the SS
// **/
MacDot16BsSsInfo* MacDot16BsGetSsByMacAddress(
                      Node* node,
                      MacDataDot16* dot16,
                      unsigned char* macAddress);

// /**
// FUNCTION   :: MacDot16BsGetSsByCid
// LAYER      :: MAC
// PURPOSE    :: Get a record of the SS using its basic CID or primary CID
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + cid   : Dot16CIDType  : Basic or primary CID of the SS
// RETURN     :: MacDot16BsSsInfo* : Pointer to the added entry
// **/
MacDot16BsSsInfo* MacDot16BsGetSsByCid(
                      Node* node,
                      MacDataDot16* dot16,
                      Dot16CIDType cid);


// /**
// FUNCTION   :: MacDot16BsGetServiceFlowByCid
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by CID of the flow
// PARAMETERS ::
// + node      : Node*                 : Pointer to node
// + dot16     : MacDataDot16*         : Pointer to DOT16 structure
// + cid       : Dot16CIDType          : CID of the service flow
// + ssInfoPtr : MacDot16BsSsInfo**    : To return the SS info pointer
// + sflowPtr  : MacDot16ServiceFlow** : To return the SFlow pointer
// RETURN     :: void : NULL
// **/
void MacDot16BsGetServiceFlowByCid(Node* node,
                                   MacDataDot16* dot16,
                                   Dot16CIDType cid,
                                   MacDot16BsSsInfo** ssInfoPtr,
                                   MacDot16ServiceFlow** sflowPtr);

// /**
// FUNCTION   :: MacDot16BsAddServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Add a new service flow to a SS
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + macAddress: unsigned char* : MAC address of nextHop
// + serviceType : MacDot16ServiceType : Service type of the flow
// + csfId : unsigned int : Classifier ID
// + qosInfo : MacDot16QoSParameter : QoS parameter of the flow
// + pktInfo : MacDot16CsIpv4Classifier* : Classifier of the flow
// + transId : unsigned int : trnasaction id for this service flow,
//                            it is only meaningful for uplink service flow
// + sfDirection : MacDot16ServiceFlowDirection : flow direction, ul or dl
// + sfInitial : MacDot16ServiceFlowInitial : Locally/remotely initaited
// + ssCid   : Dot16CIDType : SS's primary cid
// + transportCid : Dot16CIDType* : Pointer to the trans. Cid for the flow
// + isARQEnabled : BOOL : Indicates whether ARQ is to be enabled for the
//                         flow
// + arqParam : MacDot16ARQParam* : Pointer to the structure containing the
//                                  ARQ Parameters to be used in case ARQ is
//                                  enabled.
// RETURN     :: Dot16CIDType : Basic CID of SS that the flow intended for
// **/
Dot16CIDType MacDot16BsAddServiceFlow(
                 Node* node,
                 MacDataDot16* dot16,
                 unsigned char* macAddress,
                 MacDot16ServiceType serviceType,
                 unsigned int csfId,
                 MacDot16QoSParameter* qosInfo,
                 MacDot16CsClassifier* pktInfo,
                 unsigned int transId,
                 MacDot16ServiceFlowDirection sfDirection,
                 MacDot16ServiceFlowInitial sfInitial,
                 Dot16CIDType ssCid,
                 Dot16CIDType* transportCid,
                 BOOL isPackingEnabled,
                 BOOL isFixedLengthSDU,
                 unsigned int fixedLengthSDUSize,
                 TraceProtocolType appType,
                 BOOL isARQEnabled,
                 MacDot16ARQParam* arqParam);

// /**
// FUNCTION   :: MacDot16BsChangeServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Change a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + ssInfo : MacDot16BsSsInfo* : Pointer to the SS Info
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + sFlow : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     ::
// **/
void MacDot16BsChangeServiceFlow(
                 Node* node,
                 MacDataDot16* dot16,
                 MacDot16BsSsInfo* ssInfo,
                 MacDot16ServiceFlow* sFlow,
                 MacDot16QoSParameter *newQoSInfo);

// /**
// FUNCTION   :: MacDot16BsDeleteServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Delete a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo : MacDot16BsSsInfo* : Pointer to the SS Info
// + sFlow : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     ::
// **/
void MacDot16BsDeleteServiceFlow(
                 Node* node,
                 MacDataDot16* dot16,
                 MacDot16BsSsInfo* ssInfo,
                 MacDot16ServiceFlow* sFlow);
// /**
// FUNCTION   :: MacDot16BsEnqueuePacket
// LAYER      :: MAC
// PURPOSE    :: Enqueue packet of a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + basicCid : Dot16CIDType  : basic CID of the SS
// + csfId : unsigned int : ID of the classifier at CS sublayer
// + serviceType : MacDot16ServiceType : Type of the data service flow
// + msg   : Message* : The PDU from upper layer
// + flow  : MacDot16ServiceFlow** : Pointer to the service flow
// + msgDropped  : BOOL*    : Parameter to determine whether msg was dropped
// RETURN     :: BOOL : TRUE: successfully enqueued, FALSE, dropped
// **/
BOOL MacDot16BsEnqueuePacket(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType basicCid,
         unsigned int csfId,
         MacDot16ServiceType serviceType,
         Message* msg,
         MacDot16ServiceFlow** flow,
         BOOL* msgDropped);

// /**
// FUNCTION   :: MacDot16eBsHandleHoFinishedMsg
// LAYER      :: MAC
// PURPOSE    :: Handle a HELLO message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + Message*  : msg           : hello msg from neighboring BS
// RETURN     :: void          : NULL
// **/
void MacDot16eBsHandleHoFinishedMsg(Node* node,
                                    MacDataDot16* dot16,
                                    Message* msg);

// /**
// FUNCTION   :: MacDot16eBsHandleHelloMsg
// LAYER      :: MAC
// PURPOSE    :: Handle a HELLO message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + Message*  : msg           : hello msg from neighboring BS
// + msgSrcNodeId : NodeAddress  : BS node address of src of the hello msg
// RETURN     :: void          : NULL
// **/
void MacDot16eBsHandleHelloMsg(Node* node,
                               MacDataDot16* dot16,
                               Message* msg,
                               NodeAddress msgSrcNodeId);


// /**
// FUNCTION   :: MacDot16BsInit
// LAYER      :: MAC
// PURPOSE    :: Initialize dot16 BS at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16BsInit(Node* node,
                    int interfaceIndex,
                    const NodeInput* nodeInput);

// /**
// FUNCTION   :: MacDot16BsLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacDot16BsLayer(Node *node,
                     int interfaceIndex,
                     Message *msg);

// /**
// FUNCTION   :: MacDot16BsFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16BsFinalize(Node *node, int interfaceIndex);

// /**
// FUNCTION   :: MacDot16BsReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacDot16BsReceivePacketFromPhy(Node* node,
                                    MacDataDot16* dot16,
                                    Message* msg);

// /**
// FUNCTION   :: MacDot16BsReceivePhyStatusChangeNotification
// LAYER      :: MAC
// PURPOSE    :: React to change in physical status.
// PARAMETERS ::
// + node             : Node*         : Pointer to node.
// + dot16            : MacDataDot16* : Pointer to Dot16 structure
// + oldPhyStatus     : PhyStatusType : The previous physical state
// + newPhyStatus     : PhyStatusType : New physical state
// + receiveDuration  : clocktype     : The receiving duration
// RETURN     :: void : NULL
// **/
void MacDot16BsReceivePhyStatusChangeNotification(
         Node* node,
         MacDataDot16* dot16,
         PhyStatusType oldPhyStatus,
         PhyStatusType newPhyStatus,
         clocktype receiveDuration);

// /**
// FUNCTION   :: MacDot16eBsHandlePagingAnounce
// LAYER      :: MAC
// PURPOSE    :: Handle a Paging announce message from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandlePagingAnounce(
                            Node* node,
                            MacDataDot16* dot16,
                            Message* msg);
// /**
// FUNCTION   :: MacDot16eBsHandleImEntryRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a IM Entry response from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleImEntryRsp(
                            Node* node,
                            MacDataDot16* dot16,
                            Message* msg);

// /**
// FUNCTION   :: MacDot16eBsHandleImExitRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a IM Exit response from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleImExitRsp(
                                Node* node,
                                MacDataDot16* dot16,
                                Message* msg);

// /**
// FUNCTION   :: MacDot16eBsHandleLURsp
// LAYER      :: MAC
// PURPOSE    :: Handle a LU response from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleLURsp(
                            Node* node,
                            MacDataDot16* dot16,
                            Message* msg);
// /**
// FUNCTION   :: MacDot16eBsHandleIniPagRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a Initiate Paging response from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleIniPagRsp(
                                Node* node,
                                MacDataDot16* dot16,
                                Message* msg);

// /**
// FUNCTION   :: MacDot16eBsInitiateIdleMode
// LAYER      :: MAC
// PURPOSE    :: BS initiates Idle mode
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo : MacDot16BsSsInfo* : Pointer to SS info
// + dregReqDur : UInt8         : Dreg-Req Duration, 0 = BS initiated
// RETURN     :: void : NULL
// **/
void MacDot16eBsInitiateIdleMode(Node * node,
                                 MacDataDot16* dot16,
                                 MacDot16BsSsInfo* ssInfo,
                                 UInt8 dregReqDur = 0);

// /**
// FUNCTION   :: MacDot16eBsHandleDeleteMsEntry
// LAYER      :: MAC
// PURPOSE    :: Handle a Delete MS entry msg from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleDeleteMsEntry(
                                    Node* node,
                                    MacDataDot16* dot16,
                                    Message* msg);
// /**
// FUNCTION   :: MacDot16BsScheduleMgmtMsgToSs
// LAYER      :: MAC
// PURPOSE    :: Add a mgmt message to be txed for a SS
// PARAMETERS ::
// + node   : Node* : Pointer to node
// + dot16  : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo : MacDot16BsSsInfo* : Pointer to a SS info record
// + cid    : DOT16CIDType : cid of the connection
// + pduMsg : Message* : Message containing the mgmt PDU
// RETURN     :: BOOL : TRUE if successful, otherwise FALSE
//                      Note: if non-successful, msg will be freed.
// **/
BOOL MacDot16BsScheduleMgmtMsgToSs(
         Node* node,
         MacDataDot16* dot16,
         MacDot16BsSsInfo* ssInfo,
         Dot16CIDType cid,
         Message* pduMsg);


// /**
// FUNCTION   :: MacDot16BsGetServiceFlowByTransId
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by transaction id
//               of the flow
// PARAMETERS ::
// + node      : Node*                 : Pointer to node
// + dot16     : MacDataDot16*         : Pointer to DOT16 structure
// + ssInfo    : MacDot16BsSsInfo*     : Pointer to BsSsInfo
// + transId   : unsinged int          : transaction Id assoctated
//                                       with the service flow
// + sflowPtr  : MacDot16ServiceFlow** : To return the SFlow pointer
// RETURN     :: void : NULL
// **/
void MacDot16BsGetServiceFlowByTransId(Node* node,
                                       MacDataDot16* dot16,
                                       MacDot16BsSsInfo* ssInfo,
                                       unsigned int transId,
                                       MacDot16ServiceFlow** sflowPtr);

#endif // MAC_DOT16_BS_H
