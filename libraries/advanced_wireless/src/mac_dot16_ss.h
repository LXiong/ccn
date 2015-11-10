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

#ifndef MAC_DOT16_SS_H
#define MAC_DOT16_SS_H

#include "mac_dot16_bs.h"
#include "mac_dot16_phy.h"
//
// This is the header file of the implementation of Subscriber Station (SS)
// of IEEE 802.16 MAC
//

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T1_INTERVAL :
//                          (5 * DOT16_BS_DEFAULT_DCD_INTERVAL)
// DESCRIPTION :: Default value of the timeout interval for waiting DCD
// **/
#define DOT16_SS_DEFAULT_T1_INTERVAL    (5 * DOT16_BS_DEFAULT_DCD_INTERVAL)

// /**
// CONSTANT    :: DOT16_SS_MAX_T1_INTERVAL: (5 * DOT16_BS_MAX_DCD_INTERVAL)
// DESCRIPTION :: Max value of the timeout interval for waiting DCD
// **/
#define DOT16_SS_MAX_T1_INTERVAL    (5 * DOT16_BS_MAX_DCD_INTERVAL)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T2_INTERVAL :
//                          (5 * DOT16_BS_DEFAULT_INIT_RANGING_INTERVAL)
// DESCRIPTION :: Default value of the timeout interval
//                for waiting broadcast raning
// **/
#define DOT16_SS_DEFAULT_T2_INTERVAL    \
        (5 * DOT16_BS_DEFAULT_INIT_RANGING_INTERVAL)

// /**
// CONSTANT    :: DOT16_SS_MAX_T2_INTERVAL :
//                              (5 * DOT16_BS_MAX_INIT_RANGING_INTERVAL)
// DESCRIPTION :: Max value of the timeout interval
//                for waiting broadcast raning
// **/
#define DOT16_SS_MAX_T2_INTERVAL (5 * DOT16_BS_MAX_INIT_RANGING_INTERVAL)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T3_INTERVAL: 200MS
// DESCRIPTION :: Default value of the timeout interval for waitting the
//                RNG-RSP after transmission of RNG-REQ
// **/
#define DOT16_SS_DEFAULT_T3_INTERVAL    (200 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_SS_MAX_T3_INTERVAL: 200MS
// DESCRIPTION :: Max value of the timeout interval for waitting the
//                RNG-RSP after transmission of RNG-REQ
// **/
#define DOT16_SS_MAX_T3_INTERVAL    (200 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_SS_MIN_T4_INTERVAL: 30 * SECOND
// DESCRIPTION :: Max value of the timeout interval for waitting the
//                unicast ranging opportunity
// **/
#define DOT16_SS_MIN_T4_INTERVAL    (30 * SECOND)

// /**
// CONSTANT    :: DOT16_SS_MAX_T4_INTERVAL: 35 * SECOND
// DESCRIPTION :: Max value of the timeout interval for waitting the
//                unicast ranging opportunity
// **/
#define DOT16_SS_MAX_T4_INTERVAL    (35 * SECOND)

// /**
// CONSTANT    :: DOT16_SS_MAX_T6_INTERVAL: 3 * SECOND
// DESCRIPTION :: Max value of the timeout interval for waitting the
//                registration response
// **/
#define DOT16_SS_MAX_T6_INTERVAL    (3 * SECOND)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T6_INTERVAL: 3 * SECOND
// DESCRIPTION :: Default value of the timeout interval for waitting the
//                registration response
// **/
#define DOT16_SS_DEFAULT_T6_INTERVAL    (3 * SECOND)

// /**
// CONSTANT    :: DOT16_SS_MAX_T10_INTERVAL: 3 * SECOND
// DESCRIPTION :: Max value of the timeout interval for waitting the
//                transaction end
// **/
#define DOT16_SS_MAX_T10_INTERVAL    (3 * SECOND)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T10_INTERVAL: 3 * SECOND
// DESCRIPTION :: Default value of the timeout interval for waitting the
//                transaction end
// **/
#define DOT16_SS_DEFAULT_T10_INTERVAL    (3 * SECOND)
// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T12_INTERVAL:
//                              (5 * DOT16_BS_DEFAULT_UCD_INTERVAL)
// DESCRIPTION :: Default value of the timeout interval for waiting UCD
// **/
#define DOT16_SS_DEFAULT_T12_INTERVAL    (5 * DOT16_BS_DEFAULT_UCD_INTERVAL)

// /**
// CONSTANT    :: DOT16_SS_MAX_T12_INTERVAL: (5 * DOT16_BS_MAX_UCD_INTERVAL)
// DESCRIPTION :: Max value of the timeout interval for waiting UCD
// **/
#define DOT16_SS_MAX_T12_INTERVAL    (5 * DOT16_BS_MAX_UCD_INTERVAL)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T14_INTERVAL: (200 * MILLI_SECOND)
// DESCRIPTION :: Default value of the timeout interval for waiting DSX-RVD
// **/
#define DOT16_SS_DEFAULT_T14_INTERVAL    (200 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_SS_MAX_T14_INTERVAL: (200 * MILLI_SECOND)
// DESCRIPTION :: Max value of the timeout interval for waiting DSX-RVD
// **/
#define DOT16_SS_MAX_T14_INTERVAL    (200 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T16_INTERVAL: 100MS
// DESCRIPTION :: Default value of the timeout interval for waitting the
//                bandwidth grant after transmission of bandwidth request
// **/
#define DOT16_SS_DEFAULT_T16_INTERVAL    (100 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_SS_MIN_T16_INTERVAL: 10MS
// DESCRIPTION :: Min value of the timeout interval for waitting the
//                bandwidth grant after transmission of bandwidth request
// **/
#define DOT16_SS_MAX_T16_INTERVAL    (10 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T18_INTERVAL: 50MS
// DESCRIPTION :: Default value of the timeout interval for waitting the
//                SBC-RSP
// **/
#define DOT16_SS_DEFAULT_T18_INTERVAL    (50 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_T20_INTERVAL :
//                                  (2 * DOT16_BS_DEFAULT_FRAME_DURATION)
// DESCRIPTION :: Min valuse of time the SS searches for
//                preambles on a given channel (2 MAC frame)
// /**
#define DOT16_SS_DEFAULT_T20_INTERVAL    \
        (2 * DOT16_BS_DEFAULT_FRAME_DURATION)


// /**
// CONSTANT    :: DOT16_SS_MIN_T20_INTERVAL :
//                          (2 * DOT16_BS_DEFAULT_FRAME_DURATION)
// DESCRIPTION :: Min valuse of time the SS searches for
//                preambles on a given channel (2 MAC frame)
// /**
#define DOT16_SS_MIN_T20_INTERVAL    \
        (2 * DOT16_BS_DEFAULT_FRAME_DURATION)


// the unit of this variable is in no of frames
#define DOT16e_WAIT_FOR_SLP_RSP     3


// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_T43_INTERVAL :
//                          (2 * DOT16_BS_DEFAULT_FRAME_DURATION)
// DESCRIPTION :: Default valuse of time the SS wait for SLA-RSP
//                preambles on a given channel (2 MAC frame)
// /**
#define DOT16e_SS_DEFAULT_T43_INTERVAL    \
        (DOT16e_WAIT_FOR_SLP_RSP * DOT16_BS_DEFAULT_FRAME_DURATION)

// CONSTANT    :: DOT16_SS_DEFAULT_T21_INTERVAL: 2S
// DESCRIPTION :: Default value of time for searching DL-MAP
// **/
#define DOT16_SS_DEFAULT_T21_INTERVAL    (2 * SECOND)

// /**
// CONSTANT    :: DOT16_SS_MAX_T21_INTERVAL: 10S
// DESCRIPTION :: Max value of time for searching DL-MAP
// **/
#define DOT16_SS_MAX_T21_INTERVAL    (10 * SECOND)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_LOST_DL_MAP_INTERVAL: 600MS
// DESCRIPTION :: Default value of the Lost DL-MAP Interval.
//                This is the time since last received DL-MAP
//                message before downlink synchronization is
//                considered lost. If lost, SS will re-initialize.
// **/
#define DOT16_SS_DEFAULT_LOST_DL_MAP_INTERVAL    (600 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_LOST_UL_MAP_INTERVAL: 600MS
// DESCRIPTION :: Default value of the Lost UL-MAP Interval.
//                This is the time since last received UL-MAP
//                message before uplink synchronization is
//                considered lost. If lost, SS will re-initialize.
// **/
#define DOT16_SS_DEFAULT_LOST_UL_MAP_INTERVAL    (600 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_SS_POWER_UNIT_LEVEL    :   0.25
// DESCRIPTION :: Power level adjust unit (dBm)
// **/
#define DOT16_SS_DEFAULT_POWER_UNIT_LEVEL   0.25
//
// Constants for IEEE 802.16e (mobility) support
//
// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_NBR_SCAN_DURATION: 4
// DESCRIPTION :: Duration of neighbor scan period in # of frames
// **/
#define DOT16_SS_DEFAULT_NBR_SCAN_DURATION    4

// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_NBR_SCAN_INTERVAL: 8
// DESCRIPTION :: Interleaving interval of nbr scan in # of frames
// **/
#define DOT16_SS_DEFAULT_NBR_SCAN_INTERVAL    8

// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_NBR_SCAN_ITERATION: 4
// DESCRIPTION :: # of iterating nbr scan periods
// **/
#define DOT16_SS_DEFAULT_NBR_SCAN_ITERATION    2

// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_NBR_SCAN_MIN_GAP: 5S
// DESCRIPTION :: Minimal gap between consecutive nbr scanning
// **/
#define DOT16_SS_DEFAULT_NBR_SCAN_MIN_GAP    (5 * SECOND)

// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_NBR_MEA_LIFETIME :
//                DOT16_SS_DEFAULT_NBR_SCAN_MIN_GAP + 1 * SECOND
// DESCRIPTION :: Valid time of the signal measurement of a nbr BS
// **/
#define DOT16_SS_DEFAULT_NBR_MEA_LIFETIME    \
        (DOT16_SS_DEFAULT_NBR_SCAN_MIN_GAP + 1 * SECOND)

// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_T41_INTERVAL (1 * SECOND)
// DESCRIPTION :: Default waiting time for MOB_BSHO_RSP
// **/
#define DOT16e_SS_DEFAULT_T41_INTERVAL   (1 * SECOND)

// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_T42_INTERVAL (1 * SECOND)
// DESCRIPTION :: Default waiting time for ending handoff in holding down
// **/
#define DOT16e_SS_DEFAULT_T42_INTERVAL   (1 * SECOND)

// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_T44_INTERVAL (1 * SECOND)
// DESCRIPTION :: Default waiting time for MOB_SCN_RSP
// **/
#define DOT16e_SS_DEFAULT_T44_INTERVAL   (1 * SECOND)

// /**
// CONSTANT    :: DOT16e_SS_DEFAULT_T45_INTERVAL (250 * MILLI_SECOND)
// DESCRIPTION :: Default waiting time for receiving DREG-CMD
// **/
#define DOT16e_SS_DEFAULT_T45_INTERVAL   (250 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16e_SS_IDLE_MODE_TIMEOUT    (4096 * SECOND)
// DESCRIPTION :: Timer interval to send MS location update
// **/
#define DOT16e_SS_IDLE_MODE_TIMEOUT   (4096 * SECOND)

// /**
// CONSTANT    :: DOT16e_SS_LOC_UPD_TIMEOUT    DOT16e_SS_IDLE_MODE_TIMEOUT
//                                    - (128 * SECOND)
// DESCRIPTION :: Timer interval to send MS location update
// **/
#define DOT16e_SS_LOC_UPD_TIMEOUT   DOT16e_SS_IDLE_MODE_TIMEOUT - \
                                    (128 * SECOND)

// /**
// CONSTANT    :: DOT16e_SS_DREG_REQ_RETRY_COUNT   3
// DESCRIPTION :: Number of retries for DREG-REQ
// **/
#define DOT16e_SS_DREG_REQ_RETRY_COUNT   3


#define DOT16e_SS_PS_MIN_SLEEP_WINDOW_INTERVAL             4//no of frames
#define DOT16e_SS_PS_MAX_LISTEN_WINDOW_INTERVAL           64//no of frames
#define DOT16e_SS_PS_MAX_SLEEP_WINDOW_INTERVAL          1024//no of frames

#define DOT16e_SS_DEFAULT_PS1_LISTENING_INTERVAL            8
#define DOT16e_SS_DEFAULT_PS1_INITIAL_SLEEP_WINDOW         24
#define DOT16e_SS_DEFAULT_PS1_FINAL_SLEEP_WINDOW_EXPONENT   3
#define DOT16e_SS_DEFAULT_PS1_FINAL_SLEEP_WINDOW_BASE     576

#define DOT16e_SS_DEFAULT_PS2_LISTENING_INTERVAL            16
#define DOT16e_SS_DEFAULT_PS2_INITIAL_SLEEP_WINDOW          64

// mapping parameter
#define DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_NRTPS\
        POWER_SAVING_CLASS_1
#define DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_BE\
        POWER_SAVING_CLASS_1
#define DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_ERTPS\
        POWER_SAVING_CLASS_2
#define DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_RTPS\
        POWER_SAVING_CLASS_2
#define DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_UGS\
        POWER_SAVING_CLASS_2
#define DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_MULTICAST\
    POWER_SAVING_CLASS_3
#define DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_MANAGEMENT\
    POWER_SAVING_CLASS_3

//
// End of constants for IEEE 802.16e (mobility) support
//

// /**
// ENUM        :: MacDot16InitStatus
// DESCRIPTION :: Network entry and initialization staus
typedef enum
{
    //PMP status
    DOT16_SS_INIT_PreEntry = 0, // canned for implementation
    DOT16_SS_INIT_ScanDlCh,
    DOT16_SS_INIT_ObtainUlParam,
    DOT16_SS_INIT_RangingAutoAdjust,
    DOT16_SS_INIT_NegotiateBasicCapability,
    DOT16_SS_INIT_AuthorizationKeyExchange,
    DOT16_SS_INIT_RegisterWithBs,
    DOT16_SS_INIT_EstIPConnectivity,
    DOT16_SS_INIT_EstTimeOfDate,
    DOT16_SS_INIT_TransferOperationalPapam,
    DOT16_SS_INIT_EstProvisionedConnections,
    DOT16_SS_INIT_Oprational

    // Mesh specific status should goes to the above in the right order

}MacDot16InitStatus;

// /**
// ENUM         :: MacDot16SsRngState
// DESCRIPTION  :: ranging states
// **/
typedef enum
{
    DOT16_SS_RNG_Null,
    DOT16_SS_RNG_WaitBcstInitRngOpp,
    DOT16_SS_RNG_WaitInvitedRngRspForInitOpp, // SS wait for rng-rsp
    DOT16_SS_RNG_WaitRngRspForInitOpp, // SS wait for rng-rsp
    DOT16_SS_RNG_WaitBcstRngOpp,     // SS wait for range opp
                                     // (conention/unicast)
    DOT16_SS_RNG_WaitInvitedRngOpp,  // SS wait for unicast/invited
                                     // range opp
    DOT16_SS_RNG_BcstRngOppAlloc,    // SS con.-based init rng opp found
                                     // in UL map
    DOT16_SS_RNG_Backoff,            // SS backoff
    DOT16_SS_RNG_RngInvited,         // SS got invited range opp
    DOT16_SS_RNG_WaitRngRsp,         // SS wait for rng-rsp
}MacDot16SsRngState;

// /**
// STRUCT      :: MacDot16SsPara
// DESCRIPTION :: Data structure for storing dot16 SS parameters
// **/
typedef struct
{
    BOOL managed;           // whether this node is a managed SS
    BOOL authPolicySupport; // support authorization policy (PKM) or not

    unsigned char contRsvTimeout; // # UL-MAPs before retry cont based rev
    clocktype rtg;                // Receive/transmit Transition Gap
    clocktype ttg;                // Transmit/receive Transition Gap
    int sstgInPs;                 // burst ramp time
    Int16 bs_EIRP;                // signed in units of 1 dBm
    Int16 rssIRmax;               // signed in units of 1 dBm
    MacDot16MacVer macVersion;    // mac version used in this SS

    double minTxPower;            //minimum transmission power

    unsigned short requestOppSize;// cont. based bw request opp size in PS
    unsigned short rangeOppSize;  // cont. based ranging opp size in PS
    unsigned char rangeBOStart;   // Ranging Backoff Start
    unsigned char rangeBOEnd;     // Ranging Backoff End
    unsigned char requestBOStart; // Request Backoff Start
    unsigned char requestBOEnd;   // Request Backoff End

    unsigned char initialRangeRetries;  // Max # of retry for init ranging
    unsigned char periodicRangeRetries; // Max # of periodic ranging retry
    unsigned char requestRetries;       // Max # of retries for request
    unsigned char registrationRetries;  // Max # of retries of registration

    unsigned char dsxReqRetries;       // Max # of retry for dsa/dsc/dsd req
    unsigned char dsxRspRetries;       // Max # of retry for dsa/dsc/dsd rsp

    // intervals for timers
    clocktype t1Interval;   // Interval for waiting DCD
    clocktype t2Interval;   // Interval for waiting broadcast ranging
    clocktype t3Interval;   // Interval for waitting RNG-RSP after RNG-REQ
    clocktype t4Interval;   // Interval for waiting unicast rng opportunity
    clocktype t6Interval;   // Interval for waiting for REG RSP

    clocktype t7Interval;   // Interval for waiting for DSA/DSC/DSD RSP
    clocktype t8Interval;   // Interval for waiting for DSx acknowledge
    clocktype t10Interval;  // Interval for waiting for transation end
    clocktype t12Interval;  // Interval for waiting UCD descriptor
    clocktype t14Interval;  // Interval for waiting DSX-RVD message
    clocktype t16Interval;  // Interval for waitting BW grant after BW REQ

    clocktype t18Interval;  // Interval for waiting SBC-RSP
    clocktype t19Interval;  // Interval for dl-channel remains unusable
    clocktype t20Interval;  // Interval for searching for preambles on a CH
    clocktype t21Interval;  // Interval for scanning DL-MAP on a channel

    clocktype t43Interval;  // Interval for waiting SLP-RSP

    clocktype lostDlInterval; // Lost DL-MAP Interval
    clocktype lostUlInterval; // Lost UL-MAP Interval

    //paging/idle mode
    clocktype t45Interval;
    clocktype tIdleModeInterval;
    clocktype tMSPagingInterval;
    clocktype tMSPagingUnavlblInterval;
    clocktype tLocUpdInterval;
    UInt16 pagingCycle;      // in number of frames
    UInt8 pagingOffset;     // in number of frames
    //

    // timeout values
    clocktype flowTimeout;  // If a service flow remains empty with
                            // this interval long, it will be deleted

    // IEEE 802.16e parameters
    //scan
    int nbrScanDuration;     // Duration of nbr scan period in frames
    int nbrScanInterval;     // Interleaving interval of nbr scan in frames
    int nbrScanIteration;    // # of iterating nbr scan period
    clocktype nbrScanMinGap; // Minimal gap between consecutive nbr scans
    clocktype t44Interval;   // Interval for waiting for MOB_SCN-RSP

    // handover
    clocktype t41Interval;
    clocktype t42Interval;
    clocktype t29Interval;
    clocktype nbrMeaLifetime; // timeout for signal quality measure of a BS

    double hoRssTrigger;      // RSS level to trigger handover
    double hoRssMargin;       // Margin of RSS level to select new BS
    // end of 802.16e parameters
} MacDot16SsPara;
// /**
// STRUCT      :: MacDot16SsInitInfo
// DESCRIPTION :: The information of SS's initilization
// **/
typedef struct struct_mac_dot16_ss_init_info_str
{
    MacDot16InitStatus initStatus;

    BOOL dlSynchronized;
    BOOL ulParamAccquired;
    BOOL initRangingCompleted;

    // ranging
    MacDot16SsRngState rngState;
    BOOL initRangePolled;

    //PHY specific
    // keep a reference when BS cannot decode RNG-REQ, SCs, OFDM
    unsigned char initRngFrameNum;
    unsigned char initRngFrmaeOpp;
    unsigned char ofdmaRngSymbolNum;       // OFDMA only
    unsigned char ofdmaRngSubchannelIndex; //OFDMA only

    BOOL adjustAnmolies;
    unsigned char rngAnomalies;

    BOOL basicCapabilityNegotiated;
    BOOL authorized;

    // registration related variables
    BOOL registered;

    BOOL ipConnectCompleted;
    BOOL timeOfDayEstablished;
    BOOL paramTransferCompleted;

}MacDot16SsInitInfo;

// /**
// ENUM        :: MacDot16SsAnomalyInfo
// DESCRIPTION :: anomalies infomation in ranging
// **/
typedef enum
{
    ANOMALY_NO_ERROR,
    ANOMALY_ERROR

}MacDot16SsAnomalyInfo;
// /**
// STRUCT      :: MacDot16SsPeriodicRangeInfo
// DESCRIPTION :: Infomation for periodic ranging
// **/
typedef struct struct_mac_dot16_ss_periodic_range_info_str
{
    MacDot16RangeStatus lastStatus;
    MacDot16SsAnomalyInfo lastAnomaly;
    BOOL toSendRngReq;
    BOOL rngReqScheduled;
}MacDot16SsPeriodicRangeInfo;

// STRUCT      :: MacDot16SsStats
// DESCRIPTION :: Data structure for storing dot16 SS statistics
// **/
typedef struct
{
    // Data packets related statistics
    int numPktsFromUpper; // # of data packets from upper layer
    int numPktsToLower;   // # of data packets sent in MAC frames
    int numPktsFromLower; // # of data packets received in MAC frames

    // Statistics for management messages
    int numDlMapRcvd;    // # of DL-MAP messages received
    int numUlMapRcvd;    // # of UL-MAP messages received
    int numDcdRcvd;      // # of DCD messages received
    int numUcdRcvd;      // # of UCD messages received

    int numRngReqSent;   // # of RNG-REQ message sent
    int numRngRspRcvd;   // # of RNG-RSP message received
    int numCdmaRngCodeSent; // # No of CDMA rangeing code sent
    int numCdmaBwRngCodeSent; // # No of CDMA rangeing code sent for bw req

    // ARQ related stats
    int numARQBlockSent;   //# of ARQ blocks
    int numARQBlockRcvd;   //# of ARQ blocks rcvd
    int numARQBlockDiscard; //# of ARQ blocks discarded
    int numSbcReqSent;   // # of SBC-REQ sent
    int numSbcRspRcvd;   // # of SBC-RSP rceived

    int numPkmReqSent;   // # of PKM-REQ sent
    int numPkmRspRcvd;   // # of PKM-RSP received

    int numRegReqSent;   // # of REG-REQ sent
    int numRegRspRcvd;   // # of REG-RSP received

    int numRepReqRcvd;   // # of REP-REQ received
    int numRepRspSent;   // # of REP-RSP sent

    int numNetworkEntryPerformed; // # of net entry and init performed

    // DSx related statistics
    int numDsaReqSent;   // # of DSA-REQ sent
    int numDsaReqRcvd;   // # of DSA-REQ received
//    int numDsaReqRej;  // # of DSA-REQ rejected

    int numDsxRvdSent;   // # of DSX-RVD sent
    int numDsxRvdRcvd;   // # of DSX-RVD received
    int numDsaRspSent;   // # of DSA-RSP sent
    int numDsaRspRcvd;   // # of DSA-RSP received
    int numDsaAckSent;   // # of DSA-ACK sent
    int numDsaAckRcvd;   // # of DSA-ACK received

    // DSD
    int numDsdReqSent;   // # of DSD-REQ sent
    int numDsdReqRcvd;   // # of DSD-REQ received
    int numDsdRspSent;   // # of DSD-RSP sent
    int numDsdRspRcvd;   // # of DSD-RSP received

    // DSC
    int numDscReqSent;   // # of DSC-REQ sent
    int numDscReqRcvd;   // # of DSC-REQ received
    int numDscRspSent;   // # of DSC-RSP sent;
    int numDscRspRcvd;   // # of DSC-RSP received
    int numDscAckSent;   // # of DSC-ACK sent
    int numDscAckRcvd;   // # of DSC-ACK received

    // IEEE 802.16e related statistics
    int numNbrScan;        // # of neighbor BS scan performed
    int numHandover;       // # of handover performed
    int numNbrAdvRcvd;     // # of BNR-ADV received
    int numNbrScanReqSent; // # of NBR-SCAN-REQ sent
    int numNbrScanRspRcvd; // # of NBR-SCAN-RSP received
    int numNbrScanRepSent; // # of NBR-SCAN-REP sent
    int numMsHoReqSent;    // # of MSHO-REQ sent
    int numBsHoRspRcvd;    // # of BSHO-RSP received
    int numBsHoReqRcvd;    // # of BSHO-REQ received
    int numHoIndSent;      // # of Ho-Ind sent

    int numFragmentsRcvd;  // # of fragments received
    int numFragmentsSent;  // # of fragments sent
    int numMobSlpReqSent;
    int numMobSlpRspRcvd;
    int numMobTrfIndRcvd;

    //paging/idle mode
    int numDregReqSent;     //number of DREG-REQ sent
    int numDregCmdRcvd;     //number of DREG-CMD received
    int numLocUpd;          //number of Location updates perfomed
    int numMobPagAdvRcvd;   //number of MOB_PAG-ADV received

    // Dynamic statistics
    int dlCinrGuiId;       // GUI handle for current CINR of downlink
    int dlRssiGuiId;       // GUI handle for current RSSI of downlink

    int numPktToPhyGuiId;  // GUI handle for # of pkts sent to PHY
    int numPktFromPhyGuiId;// GUI handle for # of pkts from PHY

    int numNbrBsScanned;   // # of active nbr BSs discovered
    int numNbrBsGuiId;     // GUI handle for # of active nbr BSs discovered
} MacDot16SsStats;

// /**
// STRUCT      :: MacDot16SsTimerCache
// DESCRIPTION :: A structure keeps tracking timers fired in order to
//                cancel one or more timers later
// **/
typedef struct
{
    //Timer for downlink syschrnization
    Message* timerT20; // Timer for searching for preambles on a channel
    Message* timerT1;  // Timer for waiting for DCD
    Message* timerT12; // timer for UCD descriptor
    Message* timerT21; // Timer for scan a channel
    //initial ranging
    Message* timerT2;  // Timer for waitng for broadcast ranging
    Message* timerT3;  // Timer for waitting RNG-RSP
    Message* timerT16; // Timer for waitting BW grant after request

    // SBC
    Message* timerT18;

    // registration
    Message* timerT6;

    //periodic ranging
    Message* timerT4;     // wait for unicast ranging opportunity

    Message* timerLostDl; // Timer for detect of lost of sync. with downlink
    Message* timerLostUl; // Timer for detect of lost of sync. with uplink

    Message* frameEnd;    // FrameEnd timer

    // dot16e (start)
    Message* timerT44;
    Message* timerT41;
    Message* timerT42;
    Message* timerT29;
    Message* timerT43;
    //dot16e (end)

    Message* timerT19; // Scan for next downlink channel
    //paging/idle mode
    Message* timerT45;
    //Message* timerIdleMode;
    Message* timerMSPaging;
    Message* timerMSPagingUnavlbl;
    Message* timerLocUpd;
    //Message* timerDregReqDuration; // DREG-REQ duration timer
    //
} MacDot16SsTimerCache;

// /**
// STRUCT      :: MacDot16UlBurst
// DESCRIPTION :: UL burst allocation assigned to me
// **/
typedef struct mac_dot16_ss_ul_burst_str
{
    int index;
    Dot16CIDType cid;
    unsigned char uiuc;   // incidate type of the burst

    clocktype startTime;  // start time of the burst in clock time
    int durationOffset;   // sum of durations of all previous bursts in PS
    int duration;         // duration of this burst in PS

    MacDot16GenericPhyOfdmaUlMapIE ulCdmaInfo; // for CDMA
    BOOL isCdmaAllocationIEReceived;
    BOOL needSendPaddingInDataBurst;
    struct mac_dot16_ss_ul_burst_str* next;
} MacDot16UlBurst;

// /**
// STRUCT      :: MacDot16SsBWReqRecord
// DESCRIPTION :: Record BW request to be made
// **/
typedef struct mac_dot16_ss_bw_req_record_str
{
    MacDot16MacHeader macHeader;
    MacDot16ServiceType serviceType;
    BOOL rangingCodeSent;
    BOOL rngRspRecvd;
    BOOL isCDMAAllocationIERecvd;
    unsigned char serviceFlowIndex;
    int queuePriority;
    MacDot16RngCdmaInfo cdmaInfo;
    struct mac_dot16_ss_bw_req_record_str* next;
} MacDot16SsBWReqRecord;


// /**
// STRUCT      :: MacDot16SsBsInfo
// DESCRIPTION :: The information of a BS maintained at the SS
// **/
typedef struct mac_dot16_ss_bs_info_str
{
    // base station ID of the BS (BSID)
    unsigned char bsId[DOT16_BASE_STATION_ID_LENGTH];
    clocktype frameDuration;  // frame duration learned from BS
    int frameNumber;
    clocktype frameStartTime;
    BOOL associated;

    int dlChannel;              // downlink channel index
    int ulChannel;              // uplink channel index

    // CID related variables
    Dot16CIDType basicCid;      // basic management CID
    Dot16CIDType primaryCid;    // primary management CID
    Dot16CIDType secondaryCid;  // secondary management CID, only used for
                                // managed SSs
    //short numCidSupport;
    UInt16 numCidSupport;

    // DCD and UCD configuration count
    unsigned char dcdCount;
    unsigned char ucdCount;
    unsigned char nextGenDcdCount;
    unsigned char nextGenUcdCount;

    // DL and UL burst profiles
    unsigned char diuc;
    unsigned char uiuc;
    MacDot16DlBurstProfile dlBurstProfile[DOT16_NUM_DL_BURST_PROFILES];
    MacDot16UlBurstProfile ulBurstProfile[DOT16_NUM_UL_BURST_PROFILES];
    MacDot16DlBurstProfile nextGenDlBurstProf[DOT16_NUM_DL_BURST_PROFILES];
    MacDot16UlBurstProfile nextGenUlBurstProf[DOT16_NUM_UL_BURST_PROFILES];
    unsigned char leastRobustDiuc;

    // ranging variables
    unsigned char rangeRetries;   // # of ranging retries so far
    unsigned char rangeBOCount;   // count of current ranging backoff
    int rangeBOValue;             // remaining ranging backoff value
    double maxTxPowerInitRng;     // max transmission power for init rng

    clocktype timingOffset;       // not used yet
    double ulChFreq;              // not used yet
    double dlChFreq;              // not used yet
    double curerntTxPower;
    double minTxPower;

    // basic capability
    unsigned char sbcRequestRetries;
    MacDot16SsBasicCapability ssBasicCapability;

    // authorization
    MacDot16SsAuthKeyInfo authKeyInfo;

    // bandwidth request variables
    unsigned char requestRetries; // # of request retries so far
    unsigned char requestBOCount; // count of current request backoff
    int requestBOValue;           // remaining request backoff value

    // registration variables
    unsigned char regRetries;     // number of registrations performed

    // Measurements of link quality to this BS
    // Only CINR is used right now
    clocktype lastMeaTime;        // time, last signal quality measure
    double cinrMean;              // avg  CINR, in dB
    double rssMean;               // avg RSS, in dBm
    clocktype lastReportTime;     // time, last measure reprot time
    int numEmergecyMeasRep;

    // measurement histroy (DL only)
    MacDot16SignalMeasurementInfo measInfo[DOT16_MAX_NUM_MEASUREMNT];

    // variables for dot16e
    int nbrBsIndex;             // the index in the nbr adv msg, nbr BS only
    unsigned char configChangeCount;
    MacDot16eTriggerInfo trigger;
    // end for dot16e

    //paging
    UInt16 pagingCycle;      // in number of frames
    UInt8 pagingOffset;     // in number of frames
    UInt8 pagingInterval;   // in number of frames
    UInt16 pagingGroup;  // paging group id
    UInt8 dregDuration;     //Dreg-Req duration requested by BS
    UInt16 macHashSkipThshld;
    // pointer to possible next BS
    struct mac_dot16_ss_bs_info_str* next;
} MacDot16SsBsInfo;

//
// Structures for IEEE 802.16e (mobility) support
//

// /**
// ENUM        :: MacDot16eSsHandoverStatus
// DESCRIPTION :: Handover status
// **/
typedef enum
{
    DOT16e_SS_HO_None = 0,    // no handover action
    DOT16e_SS_HO_WaitMshoReq, // waitting to send MOB-MSHO-REQ
    DOT16e_SS_HO_WaitBshoRsp, // have sent MSHO-REQ, waitting for reply
    DOT16e_SS_HO_WaitHoInd,   // waitting to send MOB-HO-IND
    DOT16e_SS_HO_HoIndSent,   // have sent the MOB-HO-IND
    DOT16e_SS_HO_HoOngoing,   // in actual handover
    DOT16e_SS_HO_HoFinished,   // HO has finished

    DOT16e_SS_HO_LOCAL_Begin,     // start the HO
    DOT16e_SS_HO_LOCAL_WaitHoRsp, // after sending MSHO-REQ, wait for HO-RSP
    DOT16e_SS_HO_LOCAL_RetryExhausted, // exhaust retry for MSHO-REQ
    DOT16e_SS_HO_LOCAL_HoldingDown, // wait for final notificaiton
    DOT16e_SS_HO_LOCAL_Done,  // finish/end (in case cancellation or faild)
} MacDot16eSsHandoverStatus;

// /**
// ENUM        :: MacDot16eSsNbrScanStatus
// DESCRIPTION :: Status of the neighbor BS scan
// **/
typedef enum
{
    DOT16e_SS_NBR_SCAN_None = 0,   // not in neighbor scan mode
    DOT16e_SS_NBR_SCAN_WaitScnReq, // waitting to send scan request
    DOT16e_SS_NBR_SCAN_WaitScnRsp, // have sent request, waitting for reply
    DOT16e_SS_NBR_SCAN_Started,    // started, in start frame period
    DOT16e_SS_NBR_SCAN_InScan,     // scanning neighboring BS
    DOT16e_SS_NBR_SCAN_Interleave  // in interleaved normal operation period
} MacDot16eSsNbrScanStatus;

// /**
// ENUM        :: MacDot16eSsIdleSubStatus
// DESCRIPTION :: Idle mode intermediate/sub status
// **/
typedef enum
{
    DOT16e_SS_IDLE_None = 0,   // not in idle mode
    DOT16e_SS_IDLE_WaitDregCmd, // Waiting for DREG-CMD
    DOT16e_SS_IDLE_Idle,        // Idle mode
    DOT16e_SS_IDLE_MSListen,    // Listening to Paging Advt.
    DOT16e_SS_IDLE_BSInitiated, // BS initiated idle mode
    DOT16e_SS_IDLE_UpdLocation  // Updating location
} MacDot16eSsIdleSubStatus;

//
// End of structures for IEEE 802.16e support
//

// /**
// STRUCT      :: MacDot16SsData
// DESCRIPTION :: Data structure of Dot16 SS node
// **/
typedef struct struct_mac_dot16_ss_str
{
    //general
    MacDot16MacVer macVersion;

    //network netry and initilization
    BOOL operational;
    MacDot16SsInitInfo initInfo;

    MacDot16SsPeriodicRangeInfo periodicRngInfo;

    clocktype lastDlMapHead;  // time when last DL-MAP is heard
    unsigned int transId;     // transaction count

    MacDot16SsPara para;      // system parameters
    MacDot16SsStats stats;    // statistics of the SS

    // channel related variables
    int chScanIndex;

    // information of the current serving BS
    MacDot16SsBsInfo* servingBs;

    // variables for 802.16e support
    MacDot16eSsNbrScanStatus nbrScanStatus;  // BS scanning status
    MacDot16eSsHandoverStatus hoStatus;      // status of handover
    unsigned char prevServBsId[DOT16_BASE_STATION_ID_LENGTH];
    int hoNumRetry;                          // # of retries for ho msg
    int scanReqRetry;                        // # of retries for scn req

    int nbrChScanIndex; // dl channel on which searching neighbor BS

    int scanDuration;   // # of frames per scan interval
    int scanInterval;   // # of interleaving frames

    clocktype lastScanTime; // Time the last nbr scan finished
    int startFrames;    // # of frames before start of nbr scan
    int numScanFrames;  // # of frames left in current scan interval
    int numInterFrames; // # of interleaving frames left
    int numIterations;  // # of iterations left for scanning
    BOOL needReportScanResult; // whether MOB-SCN-REP needed
    unsigned char scanReportMode;
    clocktype scanReportInterval;
    unsigned char scanReportMetric;

    MacDot16SsBsInfo* targetBs;  // pointing to BS for scan or ho
    MacDot16SsBsInfo* nbrBsList; // list of neighboring BSs
    // end of variables for 802.16e

    // bandwidth situation
    BOOL isWaitBwGrant;  // whether currently waitting for BW grant
    int mgmtBWRequested; // Mgmt BW requested
    int mgmtBWGranted;   // Mgmt BW granted

    int bwRequestedForOutBuffer;
    int basicMgmtBwRequested;      // basic Mgmt BW requested
    int lastBasicMgmtBwRequest;
    int basicMgmtBwGranted;        // basic Mgmt BW granted
    BOOL needPiggyBackBasicBwReq;  // BW can be piggyback with the data PDU
    int priMgmtBwRequested;        // primary Mgmt BW requested
    int priMgmtBwGranted;          // primary Mgmt BW granted
    int lastPriMgmtBwRequest;
    BOOL needPiggyBackPriBwReq;    // BW can be piggyback with the data PDU
    int secMgmtBwRequested;        // secondary mgmt BW requested
    int lastSecMgmtBwRequest;
    int secMgmtBwGranted;          // secondary mgmt BW granted
    BOOL needPiggyBackSecBwReq;    // BW can be piggyback with the data PDU

    int aggBWRequested;            // Aggregated BW requested
    int aggBWGranted;              // Aggregated BW granted by BS so far

    MacDot16SsBWReqRecord* bwReqInContOpp;

    MacDot16SsBWReqRecord* bwReqHead; // Head of outgoing BW request list
    MacDot16SsBWReqRecord* bwReqTail; // Tail of outgoing BW request list

    BOOL ucastBwReqOpp;
    BOOL contBwReqOpp;

    //BOOL needSendPaddingInDataBurst;
    // UL burst information
    clocktype ulStartTime;         // the allocated start time from UL-MAP
    int ulDurationInPs;            // The duration of UL part in # of PSs
    MacDot16UlBurst* ulBurstHead;  // Header of my burst list
    MacDot16UlBurst* ulBurstTail;  // Tail of my burst list

    // scheduling variables, we create separate schedulers for different
    // service types and one additional scheduler for management messages.
    Scheduler* outScheduler[DOT16_NUM_SERVICE_TYPES];
    Scheduler* mgmtScheduler;
    Message* outBuffHead;
    Message* outBuffTail;
    int ulBytesAllocated;

    // service flow lists
    MacDot16ServiceFlowList ulFlowList[DOT16_NUM_SERVICE_TYPES];
    MacDot16ServiceFlowList dlFlowList[DOT16_NUM_SERVICE_TYPES];

    // timer cache which stores the pointer of timer messages in order
    // to reuse them or cancel them.
    MacDot16SsTimerCache timers;

    BOOL isPackingEnabled; // Is Packing enabled
    BOOL isCRCEnabled;  // Is CRC enabled
    BOOL isFragEnabled; // Is fragmentation enabled
    BOOL isARQEnabled;  // Is ARQ enabled
    MacDot16ARQParam*  arqParam ; //ARQ parameters
    MacDot16RangeType rngType;  // type of the station
    MacDot16BWReqType bwReqType;  // type of the station
    MacDot16RngCdmaInfo sendCDMAInfo;
    BOOL isCdmaAllocationIERcvd;
    Message* cdmaAllocationMsgList;

    UInt8 numOfDLBurstProfileInUsed; // FOR CDMA it is 8 else 7
    UInt8 numOfULBurstProfileInUsed;
    // Dot16e sleep mode variable
    BOOL isSleepModeEnabled;
    //BOOL mappingprocedure; // Between service type and PS mode classes
                // 0 mean use default mapping else used user defined mapping
    MacDot16ePSClasses psClassParameter[DOT16e_MAX_POWER_SAVING_CLASS];
    Message* mobTrfIndPdu;
    BOOL isPowerDown;
//    BOOL isSleepTimerActive;

    //paging/idle
    BOOL isIdleEnabled;
    BOOL performLocationUpdate;
    BOOL performNetworkReentry;
    UInt16 currentPagingGroup;
    MacDot16eSsIdleSubStatus idleSubStatus;
    UInt16 dregReqRetCount;     //DREG-REQ request retry count
    UInt16 macHashSkipCount;
    Address pgCtrlId;
    clocktype lastCSPacketRcvd;
    BOOL isSSSendDregPDU;
} MacDot16Ss;


//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16SsAddServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Add a new service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + serviceType : MacDot16ServiceType : Service type of the flow
// + csfId : unsigned int : Classifier ID
// + qosInfo : MacDot16QoSParameter* : QoS parameter of the flow
// + pktInfo : MacDot16CsIpv4Classifier* : Classifier of the flow
// + transId : unsigned int : transaction Id, no meaning for ul flow
// + sfDirection : MacDot16ServiceFlowDirection  : Direction of flow
// + sfInitial : MacDot16ServiceFlowInitial : Locally/remotely initiated
// + isARQEnabled : BOOL : Indicates whether ARQ is to be enabled for the
//                         flow
// + arqParam : MacDot16ARQParam* : Pointer to the structure containing the
//                                  ARQ Parameters to be used in case ARQ is
//                                  enabled.
// RETURN     :: Dot16CIDType : Basic CID of SS that the flow intended for
// **/
Dot16CIDType MacDot16SsAddServiceFlow(
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
                 BOOL isPackingEnabled,
                 BOOL isFixedLengthSDU,
                 unsigned int fixedLengthSDUSize,
                 TraceProtocolType appType,
//ARQ related paramaters
                 BOOL isARQEnabled,
                 MacDot16ARQParam* arqParam
                                     );
//--------------------------End MAC802.16Phase3---------------------------//

// /**
// FUNCTION   :: MacDot16SsChangeServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Change a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + sFlow : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     ::
// **/
void MacDot16SsChangeServiceFlow(
                 Node* node,
                 MacDataDot16* dot16,
                 MacDot16ServiceFlow* sFlow,
                 MacDot16QoSParameter *newQoSInfo);
// /**
// FUNCTION   :: MacDot16SsDeleteServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Delete a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + sFlow : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     ::
// **/
void MacDot16SsDeleteServiceFlow(
                 Node* node,
                 MacDataDot16* dot16,
                 MacDot16ServiceFlow* sFlow);

// /**
// FUNCTION   :: MacDot16SsGetServiceFlowByCid
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by CID of the flow
// PARAMETERS ::
// + node      : Node*                : Pointer to node
// + dot16     : MacDataDot16*        : Pointer to DOT16 structure
// + cid       : unsinged int         : CID of the service flow
// RETURN     :: MacDot16ServiceFlow* : Point to the sflow if found
// **/
MacDot16ServiceFlow* MacDot16SsGetServiceFlowByCid(
                         Node* node,
                         MacDataDot16* dot16,
                         unsigned int cid);
// /**
// FUNCTION   :: MacDot16SsEnqueuePacket
// LAYER      :: MAC
// PURPOSE    :: Enqueue packet of a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + macAddress : unsigned char* : MAC address of next hop
// + csfId : unsigned int : ID of the classifier at CS sublayer
// + serviceType : MacDot16ServiceType : Type of the data service flow
// + msg   : Message* : The PDU from upper layer
// + flow  : MacDot16ServiceFlow** : Pointer to the service flow
// + msgDropped  : BOOL*    : Parameter to determine whether msg was dropped
// RETURN     :: BOOL : TRUE: successfully enqueued, FALSE, dropped
// **/

BOOL MacDot16SsEnqueuePacket(
         Node* node,
         MacDataDot16* dot16,
         unsigned char* macAddress,
         unsigned int csfId,
         MacDot16ServiceType serviceType,
         Message* msg,
         MacDot16ServiceFlow** flow,
         BOOL* msgDropped);


// /**
// FUNCTION   :: MacDot16SsInit
// LAYER      :: MAC
// PURPOSE    :: Initialize dot16 SS at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16SsInit(Node* node,
                    int interfaceIndex,
                    const NodeInput* nodeInput);

// /**
// FUNCTION   :: MacDot16SsLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacDot16SsLayer(Node *node,
                     int interfaceIndex,
                     Message *msg);

// /**
// FUNCTION   :: MacDot16SsFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16SsFinalize(Node *node, int interfaceIndex);

// /**
// FUNCTION   :: MacDot16SsReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacDot16SsReceivePacketFromPhy(Node* node,
                                    MacDataDot16* dot16,
                                    Message* msg);

// /**
// FUNCTION   :: MacDot16SsReceivePhyStatusChangeNotification
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
void MacDot16SsReceivePhyStatusChangeNotification(
         Node* node,
         MacDataDot16* dot16,
         PhyStatusType oldPhyStatus,
         PhyStatusType newPhyStatus,
         clocktype receiveDuration);

void MacDot16SsEnqueueMgmtMsg(Node* node,
                              MacDataDot16* dot16,
                              MacDot16CidType cidType,
                              Message* mgmtMsg);
//**
// FUNCTION   :: MacDot16eSsInitiateIdleMode
// LAYER      :: MAC
// PURPOSE    :: SS initiates Idle mode
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
void MacDot16eSsInitiateIdleMode(Node * node,
                                 MacDataDot16* dot16);

// /**
// FUNCTION   :: MacDot16SsGetServiceFlowByTransId
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by
//               transaction ID of the flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + transId   : unsinged int : Trans. Id assoctated w/ the service flow
// RETURN     :: MacDot16ServiceFlow* : Point to the sflow if found
// **/
MacDot16ServiceFlow* MacDot16SsGetServiceFlowByTransId(
                         Node* node,
                         MacDataDot16* dot16,
                         unsigned int transId);

#endif // MAC_DOT16_SS_H
