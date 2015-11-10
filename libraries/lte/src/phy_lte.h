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

#ifndef _PHY_LTE_H_
#define _PHY_LTE_H_

#include <list>
#include <map>
#include <set>

#include "types.h"
#include "lte_common.h"
#include "lte_transport_block_size_table.h"
#include "matrix_calc.h"
#include "layer2_lte.h"
#include "lte_rrc_config.h"

#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif // LTE_LIB_LOG
#include "layer3_lte_measurement.h"
#include "layer3_lte.h"

///////////////////////////////////////////////////////////////
// swtiches
///////////////////////////////////////////////////////////////
// #define ERROR_DETECTION_DISABLE
#define PHY_LTE_APPLY_SAME_FADING_FOR_DL_AND_UL (0)
#define PHY_LTE_USE_IDEAL_PATHLOSS_FOR_FILTERING (0)
#define PHY_LTE_APPLY_SENSITIVITY_FOR_CELL_SELECTION (1)
#define PHY_LTE_ENABLE_INTERFERENCE_FILTERING (1)
#define PHY_LTE_INTERFERENCE_FILTERING_IN_DBM (0)

#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
class LteExponentialMean;
#endif

///////////////////////////////////////////////////////////////
// define
///////////////////////////////////////////////////////////////
#define PHY_LTE_MCS_INDEX_LEN           (32)
#define PHY_LTE_NUM_BER_TALBES          (64)
#define PHY_LTE_REGULAR_MCS_INDEX_LEN   (29)
#define PHY_LTE_CQI_INDEX_LEN           (16)
#define PHY_LTE_PRACH_CONFIG_INDEX_LEN  (32)
#define PHY_LTE_MAX_NUM_AVAILABLE_SUBFRAME          (10)
#define PHY_LTE_PREAMBLE_FORMAT_NOT_AVAILABLE       (-1)
#define PHY_LTE_SUBFRAME_NUMBER_NOT_AVAILABLE       (-1)
#define PHY_LTE_MAX_NUM_RB \
        (PHY_LTE_CHANNEL_BANDWIDTH_20MHZ_NUM_RB) //100
#define PHY_LTE_NUM_OFDM_SYMBOLS               (7)
#define PHY_LTE_NUM_SUBCARRIER_PER_RB          (12)
#define PHY_LTE_NUM_SUBCARRIER_SPACING_HZ      (15)
#define PHY_LTE_NUM_RS_RESOURCE_ELEMENTS_IN_RB (2)
#define PHY_LTE_RX_TX_TURNAROUND_TIME   (0)
#define PHY_LTE_IMPLEMENTATION_LOSS     (0.0) //in dB
#define PHY_LTE_DELTAFREQUENCY_HZ       (15000)   // 15kHz
#define PHY_LTE_MIN_DL_CHANNEL_NO       (0)
#define PHY_LTE_MIN_UL_CHANNEL_NO       (0)
#define PHY_LTE_MIN_NUM_TX_ANTENNAS      (1)
#define PHY_LTE_MAX_NUM_TX_ANTENNAS      (2)
#define PHY_LTE_MAX_UE_NUM_TX_ANTENNAS   (1)
#define PHY_LTE_MIN_NUM_RX_ANTENNAS      (1)
#define PHY_LTE_MAX_NUM_RX_ANTENNAS      (2)
#define PHY_LTE_DL_CQI_SNR_TABLE_LEN    (16)
#define PHY_LTE_DL_CQI_SNR_TABLE_OFFSET (1)
#define PHY_LTE_MAX_MCS_INDEX_LEN       (28)
#define PHY_LTE_MAX_RX_SENSITIVITY_LEN  (1)
#define PHY_LTE_MIN_TPC_ALPHA   (0)
#define PHY_LTE_MAX_TPC_ALPHA   (1)
#define PHY_LTE_MIN_CQI_REPORTING_INTERVAL     (1)
#define PHY_LTE_MAX_CQI_REPORTING_INTERVAL     (INT_MAX)
#define PHY_LTE_MIN_CQI_REPORTING_OFFSET       (0)
#define PHY_LTE_MAX_CQI_REPORTING_OFFSET       (INT_MAX)
#define PHY_LTE_MIN_RI_REPORTING_INTERVAL      (1)
#define PHY_LTE_MAX_RI_REPORTING_INTERVAL      (INT_MAX)
#define PHY_LTE_MIN_RI_REPORTING_OFFSET        (0)
#define PHY_LTE_MAX_RI_REPORTING_OFFSET        (INT_MAX)
#define PHY_LTE_MIN_SRS_TRANSMISSION_INTERVAL   (1)
#define PHY_LTE_MAX_SRS_TRANSMISSION_INTERVAL   (INT_MAX)
#define PHY_LTE_MIN_SRS_TRANSMISSION_OFFSET     (0)
#define PHY_LTE_MAX_SRS_TRANSMISSION_OFFSET     (INT_MAX)
#define PHY_LTE_MIN_CHECKING_CONNECTION_INTERVAL (1)
#define PHY_LTE_MAX_CHECKING_CONNECTION_INTERVAL (CLOCKTYPE_MAX)

#define PHY_LTE_DCI_RECEPTION_DELAY (4)
#define PHY_LTE_RA_PRACH_FREQ_OFFSET_MAX (-6)
// CPLength[nsec]=(144 * (1 / (15000 * 2048))) * SECOND = 4687.5
#define PHY_LTE_CP_LENGTH_NS (4687)
#define PHY_LTE_SIGNAL_DURATION_REDUCTION (PHY_LTE_CP_LENGTH_NS) // > 0
// 24576*Ts = 24576*(1/(1500*2048))[sec]
#define PHY_LTE_PREAMBLE_DURATION (800000)
#define PHY_LTE_PRACH_RB_NUM        (6)
#define PHY_LTE_PSS_INTERVAL (10)
#define PHY_LTE_MIN_PUCCH_OVERHEAD       (0)
#define PHY_LTE_MIN_RA_PRACH_FREQ_OFFSET (0)

#define PHY_LTE_SNR_OFFSET_FOR_CQI_SELECTION   (5.0) // dB
//Constants for invalid values
#define PHY_LTE_INVALID_CQI (-1)
#define PHY_LTE_INVALID_RI (-1)
#define PHY_LTE_INVALID_MCS (255)

//default parameter
#define PHY_LTE_DEFAULT_CHANNEL_BANDWIDTH \
         (PHY_LTE_CHANNEL_BANDWIDTH_10MHZ)
#define PHY_LTE_DEFAULT_TX_POWER_DBM  (23.0)
#define PHY_LTE_DEFAULT_DL_CANNEL     (0)
#define PHY_LTE_DEFAULT_UL_CANNEL           (1)
#define PHY_LTE_DEFAULT_NUM_TX_ANTENNAS      (1)
#define PHY_LTE_DEFAULT_NUM_RX_ANTENNAS      (1)
#define PHY_LTE_DEFAULT_NUM_PUCCH_OVERHEAD  (0)
#define PHY_LTE_DEFAULT_TPC_P_O_PUSCH       (-90.0)
#define PHY_LTE_DEFAULT_TPC_ALPHA           (1)
#define PHY_LTE_DEFAULT_CQI_REPORTING_OFFSET    (0)
#define PHY_LTE_DEFAULT_CQI_REPORTING_INTERVAL  (10)
#define PHY_LTE_DEFAULT_RI_REPORTING_OFFSET     (1)
#define PHY_LTE_DEFAULT_RI_REPORTING_INTERVAL   (10)
#define PHY_LTE_DEFAULT_CELL_SELECTION_MONITORING_PERIOD \
        (30 * MILLI_SECOND)
#define PHY_LTE_DEFAULT_NON_SERVING_CELL_MEASUREMENT_FOR_HO_PERIOD \
        (1 * MILLI_SECOND)
#define PHY_LTE_DEFAULT_NON_SERVING_CELL_MEASUREMENT_PERIOD \
        (200 * MILLI_SECOND)
#define PHY_LTE_CELL_SELECTION_MIN_SERVING_DURATION (1 * SECOND)
#define PHY_LTE_DEFAULT_CELL_SELECTION_RXLEVEL_MIN_DBM        (-140.0)
#define PHY_LTE_DEFAULT_CELL_SELECTION_RXLEVEL_MIN_OFF_DBM    (0.0)
#define PHY_LTE_DEFAULT_CELL_SELECTION_QUALLEVEL_MIN_DB       (-19.5)
#define PHY_LTE_DEFAULT_CELL_SELECTION_QUALLEVEL_MINOFFSET_DB (0.0)
#define PHY_LTE_DEFAULT_RA_PRACH_FREQ_OFFSET        (0)
#define PHY_LTE_DEFAULT_RA_DETECTION_THRESHOLD_DBM  (-100.0)
#define PHY_LTE_DEFAULT_SRS_TRANSMISSION_INTERVAL   (10)
#define PHY_LTE_DEFAULT_SRS_TRANSMISSION_OFFSET     (0)
#define PHY_LTE_DEFAULT_BER_TABLE_LEN (64)
#define PHY_LTE_DEFAULT_RXSENSITIVITY_DBM LTE_NEGATIVE_INFINITY_POWER_dBm
#define PHY_LTE_DEFAULT_PATHLOSS_FILTER_COEFFICIENT (40.0)
#define PHY_LTE_DEFAULT_CHECKING_CONNECTION_INTERVAL (1 * SECOND)
#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
#define PHY_LTE_DEFAULT_INTERFERENCE_FILTER_COEFFICIENT (40.0)
#endif
#define PHY_LTE_DEFAULT_INTERVAL_SUBFRAME_NUM_INTRA_FREQ    200 // once/200ms
#define PHY_LTE_DEFAULT_OFFSET_SUBFRAME_INTRA_FREQ          0
#define PHY_LTE_DEFAULT_FILTER_COEF_RSRP                    4
#define PHY_LTE_DEFAULT_FILTER_COEF_RSRQ                    4

// /**
// CONSTANT    :: PHY_LTE_DEFAULT_CQI_SNR_TABLE
// DESCRIPTION :: Default value for CQI-SNR table.
//                Threshold value of SINR satisfying PER=1.0e-2 under
//                the assumption of 10 RBs allocation.
//                Note : Could be modified in later release.
// **/
const double PHY_LTE_DEFAULT_CQI_SNR_TABLE[PHY_LTE_DL_CQI_SNR_TABLE_LEN] = {
        -5.00, -4.42, -3.40, -1.70, -0.19, 1.34, 2.64, 5.16, 6.71, 8.18,
        10.43, 11.84, 13.32, 15.53, 16.20, 22.38 };

///////////////////////////////////////////////////////////////
// typedef
///////////////////////////////////////////////////////////////
typedef Int32 PhyLteRandomNumberSeedType;

///////////////////////////////////////////////////////////////
// typedef enum
///////////////////////////////////////////////////////////////

// /**
// ENUM        :: LteChannelBandwidth
// DESCRIPTION ::
// **/
typedef enum {
    PHY_LTE_CHANNEL_BANDWIDTH_1400KHZ = 1400000,
    PHY_LTE_CHANNEL_BANDWIDTH_3MHZ    = 3000000,
    PHY_LTE_CHANNEL_BANDWIDTH_5MHZ    = 5000000,
    PHY_LTE_CHANNEL_BANDWIDTH_10MHZ   = 10000000,
    PHY_LTE_CHANNEL_BANDWIDTH_15MHZ   = 15000000,
    PHY_LTE_CHANNEL_BANDWIDTH_20MHZ   = 20000000
} LteChannelBandwidth;

// /**
// ENUM        :: LteChannelBandwidth
// DESCRIPTION ::
// **/
typedef enum {
    PHY_LTE_CHANNEL_BANDWIDTH_1400KHZ_NUM_RB = 6,
    PHY_LTE_CHANNEL_BANDWIDTH_3MHZ_NUM_RB    = 15,
    PHY_LTE_CHANNEL_BANDWIDTH_5MHZ_NUM_RB    = 25,
    PHY_LTE_CHANNEL_BANDWIDTH_10MHZ_NUM_RB   = 50,
    PHY_LTE_CHANNEL_BANDWIDTH_15MHZ_NUM_RB   = 75,
    PHY_LTE_CHANNEL_BANDWIDTH_20MHZ_NUM_RB   = 100
} LteChannelBandwidthNumRb;

// /**
// ENUM        :: LteMcsTableForPdsch
// DESCRIPTION :: MCS Table for PD-SCH
// **/
typedef enum {
    PHY_LTE_PDSCH_MODULATION_ORDER,
    PHY_LTE_PDSCH_TBS_INDEX, PHY_LTE_PDSCH_LEN
} LteMcsTableForPdsch;

// /**
// ENUM        :: Modulation
// DESCRIPTION ::
// **/
typedef enum {
    PHY_LTE_MOD_INVALID,
    PHY_LTE_MOD_QPSK,
    PHY_LTE_MOD_16QAM,
    PHY_LTE_MOD_64QAM
} LteModulation;

// /**
// ENUM        :: CqiTableIndex
// DESCRIPTION ::
// **/
typedef enum {
    PHY_LTE_CQI_INDEX,
    PHY_LTE_MODULATION,
    PHY_LTE_CODE_RATELTE,
    PHY_LTE_EFFICIENCY,
    PHY_LTE_CQI_TABLE_LEN
} LteCqiTableIndex;

// /**
// ENUM        :: PhyLteState
// DESCRIPTION ::
// **/
typedef enum {
    PHY_LTE_POWER_OFF,
    // Cell selection
    PHY_LTE_IDLE_CELL_SELECTION,
    PHY_LTE_CELL_SELECTION_MONITORING,
    // Random access
    PHY_LTE_IDLE_RANDOM_ACCESS,
    PHY_LTE_RA_PREAMBLE_TRANSMISSION_RESERVED,
    PHY_LTE_RA_PREAMBLE_TRANSMITTING,
    PHY_LTE_RA_GRANT_WAITING,
    // Stationary status
    PHY_LTE_DL_IDLE,
    PHY_LTE_UL_IDLE,
    // eNB status
    PHY_LTE_DL_FRAME_RECEIVING,
    PHY_LTE_UL_FRAME_TRANSMITTING,
    // UE status
    PHY_LTE_UL_FRAME_RECEIVING,
    PHY_LTE_DL_FRAME_TRANSMITTING,
    PHY_LTE_STATUS_TYPE_LEN
} PhyLteState;

// /**
// ENUM        :: PhyLteMonitoringState
// DESCRIPTION :: monitoring state during network connected status
// **/
typedef enum {
    PHY_LTE_NON_SERVING_CELL_MONITORING,
    PHY_LTE_NON_SERVING_CELL_MONITORING_IDLE
} PhyLteMonitoringState;

// /**
// ENUM        :: PhyLteSystemFrameNumber
// DESCRIPTION ::
// **/
typedef enum {
    PHY_LTE_SYSTEM_FRAME_NUMBER_EVEN,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_NOT_AVAILABLE
} PhyLteSystemFrameNumber;

///////////////////////////////////////////////////////////////
// Structure for phy statistics variables
///////////////////////////////////////////////////////////////

// /**
// STRUCT:: PhyLteCqiTableColumn
// DESCRIPTION:: PhyLteCqiTableColumn
// **/
typedef struct struct_phy_cqi_table_column {
    UInt8 cqiIndex;
    LteModulation modulation;
    UInt32 coderate;
    double efficiency;
} PhyLteCqiTableColumn;

// /**
// STRUCT:: PhyLtePdschTableColumn
// DESCRIPTION:: PhyLtePdschTableColumn
// **/
typedef struct struct_phy_pdsch_table_column {
    UInt8 modulationOrder;
    UInt8 tbsIndex;
} PhyLtePdschTableColumn;

// /**
// STRUCT:: PhyLtePuschTableColumn
// DESCRIPTION:: PhyLtePuschTableColumn
// **/
typedef struct struct_phy_pusch_table_column {
    UInt8 modulationOrder;
    UInt8 tbsIndex;
    UInt8 version;
} PhyLtePuschTableColumn;

// /**
// STRUCT:: PhyLteStats
// DESCRIPTION:: Stats
// **/
typedef struct phy_lte_stats_str {
    Int32 totalTxSignals;
    Int32 totalSignalsLocked;
    Int32 totalRxTbsToMac;
    Int32 totalRxTbsWithErrors;
    Int32 totalRxInterferenceSignals;
    UInt64 totalBitsToMac;
} PhyLteStats;

// /**
// STRUCT:: PhyLteTxInfo
// DESCRIPTION:: lteinfo
// **/
typedef struct phy_lte_tx_info {
    LteStationType txStationType;
    D_Float32 txPower_dBm; // Transmission power per RB
    UInt8 numTxAnntena;
} PhyLteTxInfo;

// /**
// STRUCT:: PhyLteRxMsgInfo
// DESCRIPTION:: packaged Rx-massage info
// **/
typedef struct phy_lte_rx_pack_msg_info_str {
    Message* rxMsg;
    Message* propRxMsgAddress;
    double txPower_mW;
    PropTxInfo propTxInfo; // Register PropTxInfo at Signal Arrival Timing,
            // Used for fading calculation when interference signal arrives
    PhyLteTxInfo lteTxInfo; // Register LteTxInfo at Signal Arrival Timing,
            // Used for fading calculation when interference signal arrives
    double rxPower_mW;
    double geometry;
    clocktype rxArrivalTime;
} PhyLteRxMsgInfo;

// /**
// STRUCT:: PhyLteTbsInfo
// DESCRIPTION:: information of transport block
// **/
typedef struct phy_lte_tbs_info_str {
    BOOL isError;
    clocktype rxTimeEvaluated;
    int transportBlockSize;
#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    double sinr;
    UInt8 mcs;
#endif
#endif
} PhyLteTbsInfo;

// /**
// STRUCT::
// DESCRIPTION::
// **/
typedef struct phy_lte_control_phy_to_mac_info {
    BOOL isError;
    LteRnti srcRnti;
} PhyLtePhyToMacInfo;

// /**
// STRUCT::
// DESCRIPTION::
// **/
typedef struct phy_lte_sinrs {
    double sinr0; // For spatial stream 0
    double sinr1; // For spatial stream 1
} PhyLteSinrs;

// /**
// STRUCT::
// DESCRIPTION::
// **/
typedef struct phy_lte_cqi {
    int cqi0; // For spatial stream 0
    int cqi1; // For spatial stream 1
} PhyLteCqi;

// /**
// STRUCT::
// DESCRIPTION::
// **/
typedef struct phy_lte_cqi_report_info {
    PhyLteCqi cqiInfo;
    int riInfo;
} PhyLteCqiReportInfo;

// /**
// CLASS           :: CqiReceiver
// DESCRIPTION     :: Management CQI receiving
// **/
class CqiReceiver {
    Node* _node;
    int _phyIndex;

    enum CqiReceiverState {
        INIT, NOCQI, NORI, PENDING, STATIONARY
    };

    CqiReceiverState _state;

    // Valid CQIs
    PhyLteCqiReportInfo _cqiReportInfo;

    // Newest CQIs
    PhyLteCqi _newestCqi;
    int _newestRi;

public:
    CqiReceiver();
    ~CqiReceiver();

    void init(Node* node, int phyIndex);
public:

    void notifyCqiUpdate(const PhyLteCqi& cqis);
    void notifyRiUpdate(int ri);
    bool getCqiReportInfo(PhyLteCqiReportInfo& cqiReportInfo);

protected:
    CqiReceiverState changeState(CqiReceiverState newStatus);
};

// /**
// STRUCT::
// DESCRIPTION::
// **/
typedef struct phy_lte_connected_ue_info_str {
    double maxTxPower_dBm;

    LteDciFormat0 dciFormat0;

    // CQI receiver
    CqiReceiver cqiReceiver;

    // UL instant pathloss measured by SRS
    std::vector < double > ulInstantPathloss_dB;

    // UE lost detection
    BOOL isDetecting;
    clocktype connectedTime;
    BOOL isFirstChecking;
} PhyLteConnectedUeInfo;

// /**
// STRUCT::
// DESCRIPTION::
// **/
typedef struct phy_lte_rx_dci_for_ue_info_str {
    LteDciFormat0 dciFormat0;
    clocktype rxTime;
    UInt64 rxTtiNumber;
} PhyLteRxDciForUlInfo;

// /**
// STRUCT     :: PhyLteRaPreamble
// DESCRIPTION:: Information of the RA preamble
// **/
typedef struct phy_lte_ra_preamble_str {
    LteRnti raRnti;
    LteRnti targetEnbRnti;
    unsigned int prachResourceStart;
    unsigned int preambleIndex;
    double txPower_mW;
    BOOL isHandingOverRa;
} PhyLteRaPreamble;

// /**
// STRUCT     :: PhyLteRrcSetupComplete
// DESCRIPTION:: Information of the RRC setup complete
// **/
typedef struct phy_lte_rrc_setup_complete_str {
    LteRnti enbRnti;
    double maxTxPower_dBm;
} PhyLteRrcSetupComplete;



// /**
// STRUCT     :: PhyLteRrcReconfComplete
// DESCRIPTION:: Information of the RRC setup complete
// **/
typedef struct phy_lte_rrc_reconf_complete_str {
    LteRnti enbRnti;
    double maxTxPower_dBm;
} PhyLteRrcReconfComplete;



// /**
// STRUCT     :: PhyLteReceivedBroadcast
// DESCRIPTION:: Information of the received broadcast
// **/
typedef struct phy_lte_received_broadcast_str {
    PhyLteRaPreamble raPreamble;
    clocktype duration;
    bool collisionFlag;
    double rxPower_mW;
} PhyLteReceivedBroadcast;

// /**
// STRUCT     :: PhyLteReceivingRaPreamble
// DESCRIPTION:: Information of the receiving preamble
// **/
typedef struct phy_lte_receiving_ra_preamble_str {
    PhyLteRaPreamble raPreamble;
    clocktype arrivalTime;
    clocktype duration;
    BOOL collisionFlag;
    double rxPower_mW;
    Message* txMsg;
} PhyLteReceivingRaPreamble;

// /**
// STRUCT     :: PhyLteEstablishmentData
// DESCRIPTION:: Data about establishment
// **/
typedef struct phy_lte_establishment_data_str {
    // Cell selection
    std::map < LteRnti, LteRrcConfig >* rxRrcConfigList;
    // Random Access
    LteRnti selectedRntieNB;
    LteRrcConfig selectedRrcConfig;
    std::list < PhyLteReceivingRaPreamble >* rxPreambleList;

    // eNB lost detection
    BOOL isDetectingSelectedEnb;

    // measurement config for HO (intra-frequency)
    BOOL measureIntraFreq;
    int intervalSubframeNum_intraFreq;
    int offsetSubframe_intraFreq;
    // measurement config for HO (inter-frequency)
    BOOL measureInterFreq;
    // common
    int filterCoefRSRP;
    int filterCoefRSRQ;
} PhyLteEstablishmentData;

// /**
// STRUCT     :: PhyLteSrsInfo
// DESCRIPTION:: SRS Information
// **/
typedef struct phy_lte_srs_info_str {
    LteRnti dstRnti;
} PhyLteSrsInfo;

typedef struct struct_phy_lte_str {
    // ----------------------------------------------------------- //
    //                        Common parameters
    // ----------------------------------------------------------- //
    PhyData* thisPhy;

    const NodeInput* nodeInput;

    // PHY state
    PhyLteState txState;
    PhyLteState previousTxState;

    PhyLteState rxState;
    PhyLteState previousRxState;

    // Station type eNB or UE
    LteStationType stationType;

    // rx parameters
    int rxModel;
    int numRxSensitivity;
    double* rxSensitivity_mW;

    // Message holders
    Int32 numTxMsgs;
    std::list < Message* >* txEndMsgList; // list of Tx End msgs for cancel
    std::map < int, Message* >* eventMsgList;// list of event msgs for cancel
    std::list < PhyLteRxMsgInfo* >* rxPackMsgList; // list of desired signals

    // list of interference signals
    std::list < PhyLteRxMsgInfo* >* ifPackMsgList;
    // error flag for each TBs
    std::map < Message*, PhyLteTbsInfo >* rxMsgErrorStat;

    // interference power on each RBs
    double* interferencePower_mW;

#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
    // Filtered interference power on each RBs
    LteExponentialMean* filteredInterferencePower;
#endif

    // Antenna parameters
    UInt8 numTxAntennas;
    UInt8 numRxAntennas;

    // Thermal noise power in RB unit
    double rbNoisePower_mW;

    // Maximum transmission power
    double maxTxPower_dBm;

    // eNB & UE Common
    UInt8 subframePerTti;

    // Bandwidth parameters
    Int32 channelBandwidth;
    UInt8 numResourceBlocks;

    // Channel parameters
    int dlChannelNo;
    int ulChannelNo;

    // Frame parameters
    clocktype rxTxTurnaroundTime;
    clocktype txDuration;
    clocktype propagationDelay;

    // CQI-SNR table
    Float64* dlCqiSnrTable;

    // Filter coefficient for pathloss measurement
    double pathlossFilterCoefficient;

#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
    // Filter coefficient for interference power
    double interferenceFilterCoefficient;
#endif

    // Statistics for LTE PHY layer
    PhyLteStats stats;

    // CheckingConnection timer interval
    clocktype checkingConnectionInterval;

    // Establishment data
    PhyLteEstablishmentData* establishmentData;

    // ----------------------------------------------------------- //
    //                            For eNB
    // ----------------------------------------------------------- //

    // Map of connected UE information
    std::map < LteRnti, PhyLteConnectedUeInfo >* ueInfoList;

    // TPC parameters
    double tpcPoPusch_dBmPerRb;
    UInt8 tpcAlpha;

    // Cell selection parameters
    double cellSelectionRxLevelMin_dBm;
    double cellSelectionRxLevelMinOff_dBm;
    double cellSelectionQualLevelMin_dB;
    double cellSelectionQualLevelMinOffset_dB;

    // Random access parameters
    UInt8 raPrachFreqOffset;
    double raDetectionThreshold_dBm;

    // PUCCH overhead in RB unit
    UInt32 numPucchOverhead;

    // RA grant send UEs
    std::set < LteRnti >* raGrantsToSend;

    // ----------------------------------------------------------- //
    //                            For UE
    // ----------------------------------------------------------- //

    // CQI informations which UE report to eNB next at feedback timing.
    PhyLteCqi nextReportedCqiInfo;
    int nextReportedRiInfo;

    // CQI reporting parameters
    int cqiReportingOffset;
    int cqiReportingInterval;
    int riReportingOffset;
    int riReportingInterval;

    // Selection time of current eNB
    clocktype phySelectedTime;

    // Cell selection parameters
    clocktype nonServingCellMeasurementInterval;
    clocktype nonServingCellMeasurementPeriod;
    clocktype nonServingCellMeasurementForHoPeriod;
    clocktype cellSelectionMinServingDuration;

    // SRS transmission parameters
    int srsTransmissionInterval;
    int srsTransmissionOffset;

    // Monitoring state
    PhyLteMonitoringState monitoringState;

    // Flag for RRC setup complete signal transmission
    BOOL rrcSetupCompleteFlag;
    BOOL rrcReconfigCompleteFlag;

    // Holder for DCI information
    std::list < PhyLteRxDciForUlInfo >* rxDciForUlInfo;
    std::list < MeasurementReport >* rrcMeasurementReportList;
    RrcConnReconfInclMoblityControlInfomationMap* rrcConnReconfMap;

    // ----------------------------------------------------------- //
    //                      Other parameters
    // ----------------------------------------------------------- //

#ifdef LTE_LIB_LOG
    lte::Aggregator* aggregator;
#endif // LTE_LIB_LOG

#ifdef ERROR_DETECTION_DISABLE
    double debugPPER;
#endif

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    std::map < LteRnti, lte::LogLteAverager >* avgReceiveSinr;
    std::map < LteRnti, lte::LogLteAverager >* avgTxPower;
    std::map < LteRnti, lte::LogLteAverager >* avgMcs;
    std::map < LteRnti, lte::LogLteAverager >* bler;
    std::map < LteRnti, lte::LogLteAverager >* totalReceivedBits;
#endif
#endif

} PhyDataLte;

//--------------------------------------------------------------------------
//  Constant array
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: PHY_LTE_CQI_TABLE
// DESCRIPTION ::
// **/
#ifdef __PHY_LTE_CPP__
const PhyLteCqiTableColumn PHY_LTE_CQI_TABLE[PHY_LTE_CQI_INDEX_LEN] =
{
    {   0, PHY_LTE_MOD_INVALID, 0, 0.0},
    {   1, PHY_LTE_MOD_QPSK, 78, 0.1523},
    {   2, PHY_LTE_MOD_QPSK, 120, 0.2344},
    {   3, PHY_LTE_MOD_QPSK, 193, 0.3770},
    {   4, PHY_LTE_MOD_QPSK, 308, 0.6016},
    {   5, PHY_LTE_MOD_QPSK, 449, 0.8770},
    {   6, PHY_LTE_MOD_QPSK, 602, 1.1758},
    {   7, PHY_LTE_MOD_16QAM, 378, 1.4766},
    {   8, PHY_LTE_MOD_16QAM, 490, 1.9141},
    {   9, PHY_LTE_MOD_16QAM, 616, 2.4063},
    {   10, PHY_LTE_MOD_64QAM, 466, 2.7305},
    {   11, PHY_LTE_MOD_64QAM, 567, 3.3223},
    {   12, PHY_LTE_MOD_64QAM, 666, 3.9023},
    {   13, PHY_LTE_MOD_64QAM, 772, 4.5234},
    {   14, PHY_LTE_MOD_64QAM, 873, 5.1152},
    {   15, PHY_LTE_MOD_64QAM, 948, 5.5547}
};
#else
extern PhyLteCqiTableColumn PHY_LTE_CQI_TABLE[PHY_LTE_CQI_INDEX_LEN];
#endif

// /**
// CONSTANT    :: PHY_LTE_PDSCH_TABLE
// DESCRIPTION ::
// **/
const PhyLtePdschTableColumn PHY_LTE_PDSCH_TABLE[PHY_LTE_MCS_INDEX_LEN] = {
    { 2,  0 }, { 2,  1 }, { 2,  2 }, { 2,  3 }, { 2,  4 }, { 2,  5 },
    { 2,  6 }, { 2,  7 }, { 2,  8 }, { 2,  9 }, { 4,  9 }, { 4, 10 },
    { 4, 11 }, { 4, 12 }, { 4, 13 }, { 4, 14 }, { 4, 15 }, { 6, 15 },
    { 6, 16 }, { 6, 17 }, { 6, 18 }, { 6, 19 }, { 6, 20 }, { 6, 21 },
    { 6, 22 }, { 6, 23 }, { 6, 24 }, { 6, 25 }, { 6, 26 },
    { 2, 0 }, // Reserved
    { 4, 0 }, // Reserved
    { 6, 0 }  // Reserved
};

// /**
// CONSTANT    :: PHY_LTE_PUSCH_TABLE
// DESCRIPTION ::
// **/
const PhyLtePuschTableColumn PHY_LTE_PUSCH_TABLE[PHY_LTE_MCS_INDEX_LEN] = {
    { 2,  0, 0 }, { 2,  1, 0 }, { 2,  2, 0 }, { 2,  3, 0 }, { 2, 4, 0 },
    { 2,  5, 0 }, { 2,  6, 0 }, { 2,  7, 0 }, { 2,  8, 0 }, { 2, 9, 0 },
    { 2, 10, 0 }, { 4, 10, 0 }, { 4, 11, 0 }, { 4, 12, 0 }, { 4, 13, 0 },
    { 4, 14, 0 }, { 4, 15, 0 }, { 4, 16, 0 }, { 4, 17, 0 }, { 4, 18, 0 },
    { 4, 19, 0 }, { 6, 19, 0 }, { 6, 20, 0 }, { 6, 21, 0 }, { 6, 22, 0 },
    { 6, 23, 0 }, { 6, 24, 0 }, { 6, 25, 0 }, { 6, 26, 0 },
    { 0, 0, 1 }, // Reserved
    { 0, 0, 2 }, // Reserved
    { 0, 0, 3 }  // Reserved
};

// /**
// CONSTANT    :: PHY_LTE_PRACH_PREAMBLE_FORMAT_TABLE
// DESCRIPTION ::
// **/
const int
PHY_LTE_PRACH_PREAMBLE_FORMAT_TABLE[PHY_LTE_PRACH_CONFIG_INDEX_LEN] = {
    0, // PRACH Configuration Index: 0
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, // PRACH Configuration Index: 10
    0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, // PRACH Configuration Index: 20
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    PHY_LTE_PREAMBLE_FORMAT_NOT_AVAILABLE, // PRACH Configuration Index: 30
    1 };

// /**
// CONSTANT    :: PHY_LTE_PRACH_SYSTEM_FRAME_NUMBER_TABLE
// DESCRIPTION ::
// **/
const PhyLteSystemFrameNumber
PHY_LTE_PRACH_SYSTEM_FRAME_NUMBER_TABLE[PHY_LTE_PRACH_CONFIG_INDEX_LEN] = {
    PHY_LTE_SYSTEM_FRAME_NUMBER_EVEN, // PRACH Config Index: 0
    PHY_LTE_SYSTEM_FRAME_NUMBER_EVEN,
    PHY_LTE_SYSTEM_FRAME_NUMBER_EVEN,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY, // PRACH Config Index: 10
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_EVEN,
    PHY_LTE_SYSTEM_FRAME_NUMBER_EVEN,
    PHY_LTE_SYSTEM_FRAME_NUMBER_EVEN,
    PHY_LTE_SYSTEM_FRAME_NUMBER_EVEN,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY, // PRACH Config Index: 20
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_ANY,
    PHY_LTE_SYSTEM_FRAME_NUMBER_NOT_AVAILABLE, // PRACH Config Index: 30
    PHY_LTE_SYSTEM_FRAME_NUMBER_EVEN, };

// /**
// CONSTANT    :: PHY_LTE_PUSCH_TABLE
// DESCRIPTION ::
// **/
const int
PHY_LTE_PRACH_SUBFRAME_NUMBER_TABLE
    [PHY_LTE_PRACH_CONFIG_INDEX_LEN][PHY_LTE_MAX_NUM_AVAILABLE_SUBFRAME] =
{
    {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PRACH Config Index: 0
    {  4, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  7, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  4, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  7, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1,  6, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  2,  7, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  3,  8, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1,  4,  7, -1, -1, -1, -1, -1, -1, -1 },
    {  2,  5,  8, -1, -1, -1, -1, -1, -1, -1 }, // PRACH Config Index: 10
    {  3,  6,  9, -1, -1, -1, -1, -1, -1, -1 },
    {  0,  2,  4,  6,  8, -1, -1, -1, -1, -1 },
    {  1,  3,  5,  7,  9, -1, -1, -1, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9 },
    {  9, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  4, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  7, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  4, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PRACH Config Index: 20
    {  7, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1,  6, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  2,  7, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  3,  8, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1,  4,  7, -1, -1, -1, -1, -1, -1, -1 },
    {  2,  5,  8, -1, -1, -1, -1, -1, -1, -1 },
    {  3,  6,  9, -1, -1, -1, -1, -1, -1, -1 },
    {  0,  2,  4,  6,  8, -1, -1, -1, -1, -1 },
    {  1,  3,  5,  7,  9, -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PRACH Config Index: 30
    {  9, -1, -1, -1, -1, -1, -1, -1, -1, -1 } };

// /**
// CONSTANT    :: PHY_LTE_PUSCH_TABLE
// DESCRIPTION ::
// **/
const UInt8 PHY_LTE_SIZE_OF_RESOURCE_BLOCK_GROUP_TABLE[4] = { 1, 2, 3, 4 };

//------------------------------------------------------------------------
//  API functions
//------------------------------------------------------------------------

// /**
// FUNCTION   :: PhyLteGetNumTxAntennas
// LAYER      :: PHY
// PURPOSE    :: Get the number of transmit antennas.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : Number of antennas
// **/
UInt8 PhyLteGetNumTxAntennas(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetNumRxAntennas
// LAYER      :: PHY
// PURPOSE    :: Get the number of receiver antennas.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : Number of antennas
// **/
UInt8 PhyLteGetNumRxAntennas(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetMaxTxPower_dBm
// LAYER      :: PHY
// PURPOSE    :: Get the maximum transmit power.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: double : Maximum transmit power(dBm)
// **/
double PhyLteGetMaxTxPower_dBm(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteSetTxPower
// LAYER      :: PHY
// PURPOSE    :: Calculate and setup of transmission signal power
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : Int32        : Index of the PHY
// + packet       : Message:     : Message to send
// RETURN     :: double : Transmission power per RB [ dBm ]
// **/
double PhyLteGetTxPower(Node* node, Int32 phyIndex, Message* packet);

// /**
// FUNCTION   :: PhyLteGetUlFilteredPathloss
// LAYER      :: PHY
// PURPOSE    :: Get the uplink pathloss.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + target   : LteRnti : target node rnti.
// + pathloss : std::vector<double>* : pathloss.
// RETURN     :: clocktype : propagation delay.
// **/
BOOL PhyLteGetUlFilteredPathloss(Node* node, int phyIndex, LteRnti target,
        std::vector < double >* pathloss_dB);

// /**
// FUNCTION   :: PhyLteGetUlInstantPathloss
// LAYER      :: PHY
// PURPOSE    :: Get the uplink pathloss.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + target   : LteRnti : target node rnti.
// + pathloss : std::vector<double>* : pathloss.
// RETURN     :: clocktype : propagation delay.
// **/
BOOL PhyLteGetUlInstantPathloss(Node* node, int phyIndex, LteRnti target,
        std::vector < double >* pathloss_dB);

// /**
// FUNCTION   :: PhyLteGetNumTxAntennas
// LAYER      :: PHY
// PURPOSE    :: Get the number of transmit antennas.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : Number of antennas
// **/
UInt8 PhyLteGetNumTxAntennas(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetNumRxAntennas
// LAYER      :: PHY
// PURPOSE    :: Get the number of receiver antennas.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : Number of antennas
// **/
UInt8 PhyLteGetNumRxAntennas(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetMaxTxPower_dBm
// LAYER      :: PHY
// PURPOSE    :: Get the maximum transmit power.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: double : Maximum transmit power(dBm)
// **/
double PhyLteGetMaxTxPower_dBm(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetUlRxIfPower
// LAYER      :: PHY
// PURPOSE    :: Get the interference power received.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// + interferencePower_mW
//            : LteExponentialMean**
//            : Pointer to the buffer of the pointer to the filtered value
// + len      : int   : The length of the array of interference power.
// RETURN     :: void : NULL
// **/
void PhyLteGetUlRxIfPower(Node* node, int phyIndex,
        double** interferencePower_mW, int* len);

#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
// /**
// FUNCTION   :: PhyLteGetUlFilteredRxIfPower
// LAYER      :: PHY
// PURPOSE    :: Get the filtered interference power received.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// + filteredInterferencePower_dBm
//            : LteExponentialMean**
//            : Pointer to the buffer of the pointer to the filtered value
// + len      : int   : The length of the array of interference power.
// RETURN     :: void : NULL
// **/
void PhyLteGetUlFilteredRxIfPower(Node* node, int phyIndex,
        LteExponentialMean** filteredInterferencePower_dBm, int* len);
#endif

// /**
// FUNCTION   :: PhyLteGetNumResourceBlocks
// LAYER      :: PHY
// PURPOSE    :: Get the number of resource blocks.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : number of resource blocks.
// **/
UInt8 PhyLteGetNumResourceBlocks(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetUlCtrlChannelOverhead
// LAYER      :: PHY
// PURPOSE    :: Get the uplink control channel overhead.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : PucchOverhead.
// **/
UInt32 PhyLteGetUlCtrlChannelOverhead(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetDlChannelFrequency
// LAYER      :: PHY
// PURPOSE    :: Get the downlink channel frequency.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: double : frequency.
// **/
double PhyLteGetDlChannelFrequency(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetUlChannelFrequency
// LAYER      :: PHY
// PURPOSE    :: Get the uplink channel frequency.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: double : frequency.
// **/
double PhyLteGetUlChannelFrequency(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetNumSubcarriersPerRb
// LAYER      :: PHY
// PURPOSE    :: Get the number of subcarriers.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : number of subcarriers.
// **/
UInt8 PhyLteGetNumSubcarriersPerRb(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetNumSymbolsPerRb
// LAYER      :: PHY
// PURPOSE    :: Get the number of symbols.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: UInt8 : number of symbols.
// **/
UInt8 PhyLteGetSymbolsPerRb(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetSubcarrierSpacing
// LAYER      :: PHY
// PURPOSE    :: Get the subcarrier spacing.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: int : subcarrier spacing.
// **/
int PhyLteGetSubcarrierSpacing(Node* node, int phyIndex);

//UInt8 PhyLteGetRbsPerSubframe(Node* node, int phyIndex); //TODO: Remove

// /**
// FUNCTION   :: PhyLteGetDlMcsTable
// LAYER      :: PHY
// PURPOSE    :: Get the downlink mcs table.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// + len : int*       : Pointer to The length of the mcs table.
// RETURN     :: PhyLtePdschTableColumn :
//               Pointer to downlink mcs table structure.
// **/
const PhyLtePdschTableColumn* PhyLteGetDlMcsTable(Node* node, int phyIndex,
        int* len);

// /**
// FUNCTION   :: PhyLteGetUlMcsTable
// LAYER      :: PHY
// PURPOSE    :: Get the uplink mcs table.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// + len : int*       : Pointer to The length of the mcs table.
// RETURN     :: PhyLtePuschTableColumn :
//               Pointer to uplink mcs table structure.
// **/
const PhyLtePuschTableColumn* PhyLteGetUlMcsTable(Node* node, int phyIndex,
        int* len);

// /**
// FUNCTION   :: PhyLteGetPrecodingMatrixList
// LAYER      :: PHY
// PURPOSE    :: Get the precoding matrix list.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: Cmat : precoding matrix list.
// **/
Cmat PhyLteGetPrecodingMatrixList(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetRbGroupsSize
// LAYER      :: PHY
// PURPOSE    :: Get the RB groups size.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// + numRb    : int   : The number of resource blocks.
// RETURN     :: UInt8 : RB groups size.
// **/
UInt8 PhyLteGetRbGroupsSize(Node* node, int phyIndex, int numRb);

// /**
// FUNCTION   :: PhyLteGetDlTxBlockSize
// LAYER      :: PHY
// PURPOSE    :: Get the downlink transmission block size.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// + mcsIndex : int   : Index of the MCS.
// + numRb    : int   : The number of resource blocks.
// RETURN     :: int : downlink transmission block size.
// **/
int
PhyLteGetDlTxBlockSize(Node* node, int phyIndex, int mcsIndex, int numRbs);

// /**
// FUNCTION   :: PhyLteGetUlTxBlockSize
// LAYER      :: PHY
// PURPOSE    :: Get the uplink transmission block size.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// + mcsIndex : int   : Index of the MCS.
// + numRb    : int   : The number of resource blocks.
// RETURN     :: int : uplink transmission block size.
// **/
int
PhyLteGetUlTxBlockSize(Node* node, int phyIndex, int mcsIndex, int numRbs);

// /**
// FUNCTION   :: PhyLteIsMessageNoTransportBlock
// LAYER      :: PHY
// PURPOSE    :: Check if msg has no transport block info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY.
// + msg      : Message* : Pointer to message.
// RETURN     :: BOOL : TRUE found.
//                      FALSE not found.
// **/
BOOL PhyLteIsMessageNoTransportBlock(Node* node, int phyIndex, Message* msg);

// /**
// FUNCTION   :: PhyLteGetBerTable
// LAYER      :: PHY
// PURPOSE    :: Get the ber table.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// RETURN     :: PhyBerTable* : Pointer to ber table.
// **/
const PhyBerTable* PhyLteGetBerTable(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetReceptionModel
// LAYER      :: PHY
// PURPOSE    :: Get the reception model.
// PARAMETERS ::
// + node     : Node* : Pointer to node.
// + phyIndex : int   : Index of the PHY.
// RETURN     :: PhyRxModel : reception model.
// **/
PhyRxModel PhyLteGetReceptionModel(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetCqiInfoFedbackFromUe
// LAYER      :: PHY
// PURPOSE    :: Get the cqi info.
// PARAMETERS ::
// + node     : Node*   : Pointer to node.
// + phyIndex : int     : Index of the PHY.
// + target   : LteRnti : target node rnti.
// RETURN     :: PhyLteCqiReportInfo* : cqi report info.
// **/
bool PhyLteGetCqiInfoFedbackFromUe(Node* node, int phyIndex, LteRnti target,
        PhyLteCqiReportInfo* getValue);

// /**
// FUNCTION   :: PhyLteGetCqiSnrTable
// LAYER      :: PHY
// PURPOSE    :: Get the cqi snr table.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + cqiTable : Float64** : Pointer to the cqi table.
// + len      : int*      : Pointer to The length of the cqi table.
// RETURN     :: PhyLteCqiReportInfo* : cqi report info.
// **/
void PhyLteGetCqiSnrTable(Node* node, int phyIndex, Float64** cqiTable,
        int* len);

// /**
// FUNCTION   :: PhyLteGetRsResourceElementsInRb
// LAYER      :: PHY
// PURPOSE    :: Get the rs ofdm symbol in rb.
// PARAMETERS ::
// + node     : Node* : Pointer to node
// + phyIndex : int   : Index of the PHY
// RETURN     :: int : RS OFDM symbol in RB.
// **/
int PhyLteGetRsResourceElementsInRb(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetDciForUl
// LAYER      :: PHY
// PURPOSE    :: Get newest DCI for UE.
//               When pop-flag is true, update newest-data.
// PARAMETERS ::
// + node        : Node*  : Pointer to node.
// + phyIndex    : int    : Index of the PHY
// + ttiNumber   : UInt64 : Current TTI number
// + pop         : BOOL : when true, popping target one and remove older ones
// + setValue    : LteDciFormat0* : data pointer of DCI for UE
// RETURN       ::  BOOL    : when true, successful of get data.
// **/
BOOL PhyLteGetDciForUl(Node* node, int phyIndex, UInt64 ttiNumber, BOOL pop,
        LteDciFormat0* setValue);

// /**
// FUNCTION   :: PhyLteGetPropagationDelay
// LAYER      :: PHY
// PURPOSE    :: Get the propagation delay.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// RETURN     :: clocktype : propagation delay.
// **/
clocktype PhyLteGetPropagationDelay(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteChangeState
// LAYER      :: PHY
// PURPOSE    :: To change a given state.
// PARAMETERS ::
// + node     : Node*       : Pointer to node.
// + phyIndex : int         : Index of the PHY.
// + phyLte   : PhyDataLte* : Pointer to the LTE PHY structure
// + status   : PhyLteState : State transition.
// RETURN     :: void : NULL
// **/
void PhyLteChangeState(Node* node, int phyIndex, PhyDataLte* phyLte,
        PhyLteState status);

//**
// FUNCTION   :: PhyLteGetMessageTxInfo
// LAYER      :: PHY
// PURPOSE    :: Get the transmit info.
// PARAMETERS ::
// + node     : Node*   : Pointer to node.
// + phyIndex : int     : Index of the PHY.
// RETURN     :: PhyLteTxInfo* : Pointer to LTE PHY transmit info.
// **/
PhyLteTxInfo* PhyLteGetMessageTxInfo(Node* node, int phyIndex, Message* msg);

// /**
// FUNCTION   :: PhyLteRrcConnectedNotification
// LAYER      :: PHY
// PURPOSE    :: Notify PHY layer of RRC connected complete.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + handingover    : BOOL  : whether handingover
// RETURN     :: void : NULL
// **/
void PhyLteRrcConnectedNotification(Node* node, int phyIndex,
                                    BOOL handingover = FALSE);



// /**
// FUNCTION   :: PhyLteHashInputsToMakeSeed
// LAYER      :: PHY
// PURPOSE    ::
// PARAMETERS ::
// + node      : PhyLteRandomNumberSeedType*  : Pointer to seed.
// + phyIndex  : int              : hash
// RETURN     :: void             : NULL
// **/
inline PhyLteRandomNumberSeedType PhyLteHashInputsToMakeSeed(
        const PhyLteRandomNumberSeedType seed, const int hashingInput)
{
    UInt64 hash1 = seed;
    UInt64 hash2 = hashingInput;
    return static_cast < PhyLteRandomNumberSeedType > ((9838572817LL * hash1
            + 77138234763LL * ~hash2) % INT_MAX);
}

// /**
// FUNCTION   :: PhyLteHashInputsToMakeSeed
// LAYER      :: PHY
// PURPOSE    ::
// PARAMETERS ::
// + node      : PhyLteRandomNumberSeedType*  : seed.
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// RETURN     :: void             : NULL
// **/
inline PhyLteRandomNumberSeedType PhyLteHashInputsToMakeSeed(
        const PhyLteRandomNumberSeedType seed, const int hashingInput1,
        const int hashingInput2) {
    return PhyLteHashInputsToMakeSeed(PhyLteHashInputsToMakeSeed(seed,
            hashingInput1), hashingInput2);
}

// /**
// FUNCTION   :: PhyLteHashInputsToMakeSeed
// LAYER      :: PHY
// PURPOSE    ::
// PARAMETERS ::
// + node      : PhyLteRandomNumberSeedType*  : seed.
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// RETURN     :: void             : NULL
// **/
inline PhyLteRandomNumberSeedType PhyLteHashInputsToMakeSeed(
        const PhyLteRandomNumberSeedType seed, const int hashingInput1,
        const int hashingInput2, const int hashingInput3) {
    return PhyLteHashInputsToMakeSeed(PhyLteHashInputsToMakeSeed(seed,
            hashingInput1), hashingInput2, hashingInput3);
}

// /**
// FUNCTION   :: PhyLteHashInputsToMakeSeed
// LAYER      :: PHY
// PURPOSE    ::
// PARAMETERS ::
// + node      : PhyLteRandomNumberSeedType*  : seed.
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// RETURN     :: void             : NULL
// **/
inline PhyLteRandomNumberSeedType PhyLteHashInputsToMakeSeed(
        const PhyLteRandomNumberSeedType seed, const int hashingInput1,
        const int hashingInput2, const int hashingInput3,
        const int hashingInput4) {
    return PhyLteHashInputsToMakeSeed(PhyLteHashInputsToMakeSeed(seed,
            hashingInput1), hashingInput2, hashingInput3, hashingInput4);
}

// /**
// FUNCTION   :: PhyLteHashInputsToMakeSeed
// LAYER      :: PHY
// PURPOSE    ::
// PARAMETERS ::
// + node      : PhyLteRandomNumberSeedType*  : Pointer to seed.
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// + phyIndex  : int              : hash
// RETURN     :: void             : NULL
// **/
inline PhyLteRandomNumberSeedType PhyLteHashInputsToMakeSeed(
        const PhyLteRandomNumberSeedType seed, const Int32 hashingInput1,
        const Int32 hashingInput2, const Int32 hashingInput3,
        const Int32 hashingInput4, const Int32 hashingInput5) {
    return PhyLteHashInputsToMakeSeed(PhyLteHashInputsToMakeSeed(seed,
            hashingInput1), hashingInput2, hashingInput3, hashingInput4,
            hashingInput5);
}

// /**
// FUNCTION   :: PhyLteGetChannelMatrix
// LAYER      :: PHY
// PURPOSE    :: Calculate channel matrix.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + phyLte       : PhyDataLte*  : PHY data structure
// + propTxInfo   : PropTxInfo*   : message of propTx.
// + lteTxInfo    : PhyLteTxInfo* : LTE-info of propRx.
// + matH         : Cmat& :
//                    Reference to the buffer channel matrix is set to.
// RETURN     :: void  : NULL
// **/
void PhyLteGetChannelMatrix(Node* node, int phyIndex, PhyDataLte* phyLte,
        PropTxInfo* propTxInfo, PhyLteTxInfo* lteTxInfo, Cmat& matH);

// /**
// FUNCTION   :: PhyLteGetChannelMatrix
// LAYER      :: PHY
// PURPOSE    :: Calculate channel matrix including pathloss.
// PARAMETERS ::
// + node         : Node*         : Pointer to node.
// + phyIndex     : int           : Index of the PHY
// + phyLte       : PhyDataLte*   : PHY data structure
// + propTxInfo   : PropTxInfo*   : message of propTx.
// + lteTxInfo    : PhyLteTxInfo* : LTE-info of propRx.
// + pathloss_dB  : double        : Pathloss of propRx.
// + matH         : Cmat& :
//                    Reference to the buffer channel matrix is set to.
// RETURN     :: void : NULL
// **/
void PhyLteGetChannelMatrix(Node* node, int phyIndex, PhyDataLte* phyLte,
        PropTxInfo* propTxInfo, PhyLteTxInfo* lteTxInfo, double pathloss_dB,
        Cmat& matHhat);

// /**
// FUNCTION   :: PhyLteChannelMatrixMultiplyPathloss
// LAYER      :: PHY
// PURPOSE    :: Apply pathloss for channel matrix.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + phyLte       : PhyDataLte*  : PHY data structure
// + lteTxInfo    : PhyLteTxInfo* : LTE-info of propRx.
// + pathloss_dB  : double        : Pathloss
// + matHhat      : Cmat          : Channel matrix to apply pathloss_dB
// RETURN     :: void : NULL
// **/
void PhyLteChannelMatrixMultiplyPathloss(Node* node, int phyIndex,
        PhyDataLte* phyLte, PhyLteTxInfo* lteTxInfo, double pathloss_dB,
        Cmat& matHhat);

// /**
// FUNCTION   :: PhyLteSetMessageTxInfo
// LAYER      :: PHY
// PURPOSE    :: Set the transmit info.
// PARAMETERS ::
// + node        : Node*       : Pointer to node.
// + phyIndex    : int         : Index of the PHY.
// + phyLte      : PhyDataLte* : Pointer to the LTE PHY structure.
// + txPower_dBm : double      : Transmission power of RB in dBm.
// + msg         : The massage address to append "info".
// RETURN     :: void : NULL
// **/
void PhyLteSetMessageTxInfo(Node* node, int phyIndex, PhyDataLte* phyLte,
        double txPower_dBm, Message* msg);

// /**
// FUNCTION   :: PhyLteSetGrantInfo
// LAYER      :: PHY
// PURPOSE    :: Set the Grant info.
// PARAMETERS ::
// + node     : Node*    : Pointer to node.
// + phyIndex : int      : Index of the PHY
// + msg      : Message* : The massage address to append RA grant info.
// RETURN     :: void : NULL
// **/
BOOL PhyLteSetGrantInfo(Node* node, int phyIndex, Message* msg);

// /**
// FUNCTION   :: PhyLteGetSinr
// LAYER      :: PHY
// PURPOSE    :: Calculate SINR of each transport block.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + txScheme     : LteTxScheme  : Transmission scheme
// + phyLte       : PhyDataLte*  : Pointer to LTE PHY structure
// + usedRB_list  : UInt8[]      : RB allocation array
// + matHhat      : Cmat&        : Channel matrix
// + geometry     : double       : geometry
// + txPower_mW   : double       : Transmission power in mW
// + useFilteredInterferencePower
//                : bool         : Whether to use filtered interference power
// + isForCqi     : bool         :
//                    Indicate SINR calculation is for CQI or not ( For log)
// + txRnti       : LteRnti      : RNTI of tx node. ( For log)
// RETURN     :: double : SINR
// **/
std::vector < double > PhyLteCalculateSinr(
        Node* node,
        int phyIndex,
        LteTxScheme txScheme,
        PhyDataLte* phyLte,
        UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB],
        Cmat& matHhat,
        double geometry,
        double txPower_mW,
        bool useFilteredInterferencePower,
        bool isForCqi, // for log
        LteRnti txRnti); // for log

// /**
// FUNCTION   :: PhyLteGetSinr
// LAYER      :: PHY
// PURPOSE    :: Calculate SINR of each transport block.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + txScheme     : LteTxScheme  : Transmission scheme
// + phyLte       : PhyDataLte*  : PHY data structure
// + geometry     : double       : geometry.
// + txPower_mW   : double       : Tx-signal power.
// + ifPower_mW   : double       : Interference signal power.
// RETURN     :: double : SINR
// **/
std::vector < double > PhyLteCalculateSinr(Node* node, int phyIndex,
        LteTxScheme txScheme, PhyDataLte* phyLte, Cmat& matHhat,
        double geometry, double txPower_mW, double ifPower_mW);

// /**
// FUNCTION   :: PhyLteCreateRxMsgInfo
// LAYER      :: PHY
// PURPOSE    :: Create a unpacked received message structure.
// PARAMETERS ::
// + node          : Node*  : Pointer to node.
// + phyIndex      : int    : Index of the PHY.
// + phyLte        : PhyDataLte* : Pointer to the LTE PHY structure.
// + propRxInfo    : PropRxInfo* : Information of the arrived signal.
// + rxpathloss_mW : double : Path loss.
// + txPower_mW    : double : Transmit power.
// RETURN     :: PhyLteRxMsgInfo* : Pointer to a structure created.
// **/
PhyLteRxMsgInfo* PhyLteCreateRxMsgInfo(Node* node, int phyIndex,
        PhyDataLte* phyLte, PropRxInfo* propRxInfo, double rxpathloss_mW,
        double txPower_mW);

// /**
// FUNCTION   :: PhyLteSetPropagationDelay
// LAYER      :: PHY
// PURPOSE    :: Calculate propagation delay of each EU.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + propTxInfo   : PropTxInfo*  : message of propTx.
// RETURN     :: void : NULL
// **/
void
PhyLteSetPropagationDelay(Node* node, int phyIndex, PropTxInfo* propTxInfo);

// /**
// FUNCTION   :: LteTerminateCurrentTransmissions
// LAYER      :: PHY
// PURPOSE    :: Terminate all ongoing transmissions
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// RETURN     :: void : NULL
// **/
void PhyLteTerminateCurrentTransmissions(Node* node, int phyIndex);

// /**
// FUNCTION   :: LteGetPhyStateString
// LAYER      :: LTE
// PURPOSE    :: Get the PHY state as the string.
// PARAMETERS ::
// + state      : PhyLteState   : phy state.
// RETURN     :: char* : the string of phy state.
// **/
static const char* LteGetPhyStateString(PhyLteState state);

// /**
// FUNCTION   :: PhyLteGetResourceAllocationInfo
// LAYER      :: LTE
// PURPOSE    :: get the list of RB.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + txScheme  : LteTxScheme : Transmission scheme
// + numCodeWords  : int*    : Number of *codewords*
// + tb2cwSwapFlag : bool*   : Codeword to transport block swap flag
//                             (Valid for Dic2aInfo)
// + usedRB_list   : UInt8*  : List of resource blocks allocated
// + numRb     : int*        : Number of resource blocks allocated
// + mcsIndex  : std::vector<UInt8*> : List of mcsIndex for
//                                     each *transport blocks*.
// RETURN     :: Message*  :  rfMsg->next if 2 transport blocks detected
//                            rfMsg       otherwise.
// **/
Message* PhyLteGetResourceAllocationInfo(Node* node, int phyIndex,
        Message* rfMsg, LteTxScheme txScheme, int* numCodeWords,
        bool* tb2cwSwapFlag, UInt8* usedRB_list, int* numRb,
        std::vector < UInt8 >* mcsIndex);

// /**
// FUNCTION   :: PhyLteGetResourceAllocationInfo
// LAYER      :: LTE
// PURPOSE    :: Get the list of RB.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + rfMsg     : Message*  : received message
// + usedRB_list : UInt8*  : Pointer to the buffer for list of RB
// + numRb     : int*      : Pointer to the buffer for number of RBs
// RETURN     :: BOOL      :  TRUE  : If DCI found
//                            FALSE : otherwise
// **/
BOOL PhyLteGetResourceAllocationInfo(Node* node, int phyIndex,
        Message* rfMsg, UInt8* usedRB_list, int* numRb);

// /**
// FUNCTION   :: PhyLteInit
// LAYER      :: PHY
// PURPOSE    :: Initialize the LTE PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to the node input
// RETURN     :: void             : NULL
// **/
void PhyLteInit(Node* node, const int phyIndex, const NodeInput* nodeInput);

// /**
// FUNCTION   :: PhyLteFinalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the LTE PHY, print out statistics
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : const int : Index of the PHY
// RETURN     :: void      : NULL
// **/
void PhyLteFinalize(Node* node, const int phyIndex);

// /**
// FUNCTION   :: PhyLteNotifyPowerOff
// LAYER      :: PHY
// PURPOSE    :: Notify PHY layer of power off.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// RETURN     :: void             : NULL
// **/
void PhyLteNotifyPowerOff(Node* node, const int phyIndex);

// /**
// FUNCTION   :: PhyLteNotifyPowerOn
// LAYER      :: PHY
// PURPOSE    :: Notify PHY layer of power on.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// RETURN     :: void             : NULL
// **/
void PhyLteNotifyPowerOn(Node* node, const int phyIndex);

// /**
// FUNCTION   :: PhyLteAddInterferencePower
// LAYER      :: PHY
// PURPOSE    :: Add interference power
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + rxMsgInfo    : int   : PhyLteRxMsgInfo structure of interference signal
// RETURN     :: void : NULL
// **/
void PhyLteAddInterferencePower(Node* node,
        int phyIndex, PhyLteRxMsgInfo* rxMsgInfo);

// /**
// FUNCTION   :: PhyLteSubtractInterferencePower
// LAYER      :: PHY
// PURPOSE    :: Subtract interference power
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + rxMsgInfo    : int   : PhyLteRxMsgInfo structure of interference signal
// RETURN     :: void : NULL
// **/
void PhyLteSubtractInterferencePower(Node* node,
        int phyIndex, PhyLteRxMsgInfo* rxMsgInfo);

// /**
// FUNCTION   :: LteSignalArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the channel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyLteSignalArrivalFromChannel(Node* node, int phyIndex,
        int channelIndex, PropRxInfo* propRxInfo);

// /**
// FUNCTION   :: LteSignalEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyLteSignalEndFromChannel(Node* node, int phyIndex, int channelIndex,
        PropRxInfo* propRxInfo);

// /**
// FUNCTION   :: LteTerminateCurrentReceive
// LAYER      :: PHY
// PURPOSE    :: Terminate all signals current under receiving.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// RETURN     :: void : NULL
// **/
void PhyLteTerminateCurrentReceive(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteStartTransmittingSignal
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + packet : Message* : Frame to be transmitted
// + useMacLayerSpecifiedDelay : BOOL : Use MAC specified
//                                      delay or calculate it
// + initDelayUntilAirborne    : clocktype : The MAC specified delay
// RETURN     :: void : NULL
// **/
void PhyLteStartTransmittingSignal(Node* node, int phyIndex, Message* packet,
        BOOL useMacLayerSpecifiedDelay, clocktype initDelayUntilAirborne);

// /**
// FUNCTION   :: PhyLteTransmissionEnd
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + msg       : Message* : The Tx End event
// RETURN     :: void  : NULL
// **/
void PhyLteTransmissionEnd(Node* node, int phyIndex, Message* msg);

// /**
// FUNCTION   :: LteChannelListeningSwitchNotification
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
void PhyLteChannelListeningSwitchNotification(Node* node, int phyIndex,
        int channelIndex, BOOL startListening);

// /**
// FUNCTION   :: PhyLteAddConnectedUe
// LAYER      :: PHY
// PURPOSE    :: Add connected-UE/Serving-eNB.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + ueRnti       : LteRnti      : RNTI of UE to add
// + phyLte       : PhyDataLte*  : PHY data structure
// RETURN     :: BOOL: TRUE  : registration of new UE is succeed,
//                     FALSE : otherwise.
// **/
BOOL PhyLteAddConnectedUe(Node* node, int phyIndex, LteRnti ueRnti,
        PhyDataLte* phyLte);

// /**
// FUNCTION   :: PhyLteGetUlMaxTxPower_dBm
// LAYER      :: PHY
// PURPOSE    :: Get the uplink Maximum transmit power(dBm).
// PARAMETERS ::
// + node     : Node*   : Pointer to node.
// + phyIndex : int     : Index of the PHY.
// + target   : LteRnti : target node rnti.
// + maxTxPower_dBm : double* : Pointer to Maximum transmit power.
// RETURN     :: BOOL : TRUE target found & get.
//                      FALSE target not found.
// **/
BOOL PhyLteGetUlMaxTxPower_dBm(Node* node, int phyIndex, LteRnti target,
        double* maxTxPower_dBm);

// /**
// FUNCTION   :: PhyLteGetTpcPoPusch
// LAYER      :: PHY
// PURPOSE    :: Get TPC parameter P_O_PUSCH
// PARAMETERS ::
// + node      : Node*  : Pointer to node.
// + phyIndex  : int    : Index of the PHY
// RETURN     :: void  : TPC parameter P_O_PUSCH ( mW/RB)
// **/
double PhyLteGetTpcPoPusch(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetTpcAlpha
// LAYER      :: PHY
// PURPOSE    :: Get TPC parameter alpha
// PARAMETERS ::
// + node      : Node*  : Pointer to node.
// + phyIndex  : int    : Index of the PHY
// RETURN     :: double : TPC parameter alpha
// **/
double PhyLteGetTpcAlpha(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteGetThermalNoise
// LAYER      :: PHY
// PURPOSE    :: Get thermal noise
// PARAMETERS ::
// + node      : Node*  : Pointer to node.
// + phyIndex  : int    : Index of the PHY
// RETURN     :: double : Thermal noise [ mW per RB ]
// **/
double PhyLteGetThermalNoise(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteCheckAllOfRxPacketError
// LAYER      :: PHY
// PURPOSE    :: Check if any receiving message has error.
// PARAMETERS ::
// + node       : Node*         : Pointer to node.
// + phyIndex   : int           : Index of the PHY.
// + propRxInfo : PropRxInfo*   : Propagation reception information
// RETURN     :: void : NULL
// **/
void PhyLteCheckAllOfRxPacketError(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteLessRxSensitivity
// LAYER      :: PHY
// PURPOSE    :: Check the reception signals sensitivity.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + phyIndex     : int          : Index of the PHY
// + phyLte       : PhyDataLte*  : PHY data structure
// + rxPower_mW   : double       : Rx-signals power
// RETURN     ::  BOOL    : when true, Greater than or equal to receive sensitivity.
// **/
BOOL PhyLteLessRxSensitivity(Node* node,
                             int phyIndex,
                             PhyDataLte* phyLte,
                             double rxPower_mW);

// /**
// FUNCTION   :: PhyLteJudgeTxScheme
// LAYER      :: PHY
// PURPOSE    :: Judge transmission scheme from the DCI information
//               on the message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + phyIndex  : int          : Index of the PHY
// + msg       : Message*     : Message structure to be judged
// RETURN     ::  LteTxScheme : Transmission scheme.
//                              TX_SCHEME_INVALID if no tx scheme determined
// **/
LteTxScheme PhyLteJudgeTxScheme(Node* node, int phyIndex, Message* msg);

// /**
// FUNCTION   :: PhyLteDetermineTransmissiomScheme
// LAYER      :: PHY
// PURPOSE    :: Determine transmission scheme from rankIndicator and
//               configured transmission mode.
// PARAMETERS ::
// + node      : Node*   : Pointer to node.
// + phyIndex  : int     : Index of the PHY
// + rankIndicator : int : Rank indicator
// + numLayer  : int* :
//                 Pointer to the buffer for number of layer.
// + numTransportBlocksToSend : int* :
//                 Pointer to buffer for number of transport blocks to send.
// RETURN     :: LteTxScheme : Transmission scheme
// **/
LteTxScheme PhyLteDetermineTransmissiomScheme(Node* node, int phyIndex,
        int rankIndicator, int* numLayer, int* numTransportBlocksToSend);

// /**
// FUNCTION   :: PhyLteCalculateRankIndicator
// LAYER      :: PHY
// PURPOSE    :: Calculate rank indicator
// PARAMETERS ::
// + node         : Node*         : Pointer to node.
// + phyIndex     : int           : Index of the PHY.
// + phyLte       : PhyDataLte*   : Pointer to LTE PHY structure
// + propTxInfo   : PropTxInfo*   : Pointer to propagation tx info
// + lteTxInfo    : PhyLteTxInfo* : Pointer to LTE transmission info
// + pathloss_dB  : double        : Pathloss of received signal in dB
// RETURN     :: int : Rank indicator
// **/
int PhyLteCalculateRankIndicator(
    Node* node,
    int phyIndex,
    PhyDataLte* phyLte,
    PropTxInfo* propTxInfo, // Tx info of desired signal
    PhyLteTxInfo* lteTxInfo, // Tx info of desired signal
    double pathloss_dB);

// /**
// FUNCTION   :: PhyLteCalculateCqi
// LAYER      :: PHY
// PURPOSE    :: Calculate CQI
// PARAMETERS ::
// + node           : Node*         : Pointer to node.
// + phyIndex       : int           : Index of the PHY.
// + phyLte         : PhyDataLte*   : Pointer to LTE PHY structure
// + propTxInfo     : PropTxInfo*   : Pointer to propagation tx info
// + lteTxInfo      : PhyLteTxInfo* : Pointer to LTE transmission info
// + rankIndicator  : int           : Rank indicator
// + pathloss_dB    : double        : Pathloss of received signal in dB
// + forCqiFeedback : bool          : For CQI feedback or not
//                                    (For aggregation log)
// RETURN     :: int : Rank indicator
// **/
PhyLteCqi PhyLteCalculateCqi(
    Node* node,
    int phyIndex,
    PhyDataLte* phyLte,
    PropTxInfo* propTxInfo, // Tx info of desired signal
    PhyLteTxInfo* lteTxInfo, // Tx info of desired signal
    int rankIndicator,
    double pathloss_dB,
    bool forCqiFeedback); // for log

// /**
// FUNCTION   :: PhyLteGetDlCqiSnrTableIndex
// LAYER      :: PHY
// PURPOSE    :: Get the downlink cqi snr table index.
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex  : int   : Index of the PHY.
// + phyLte    : PhyDataLte* : Pointer to the LTE PHY structure.
// + sinr      : double  : Sinr.
// RETURN     ::  void    : NULL
// **/
int PhyLteGetDlCqiSnrTableIndex(Node* node, int phyIndex, PhyDataLte* phyLte,
        Float64 sinr);

// /**
// FUNCTION   :: PhyLteGetPathloss_dB
// LAYER      :: PHY
// PURPOSE    :: Calculate pathloss excluding fading
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex   : int     : Index of the PHY
// + lteTxInfo  : PhyLteTxInfo* : Pointer to LTE transmission info.
// + propRxInfo : PropRxInfo*   : Pointer to propagation rx info.
// RETURN     :: double : Pathloss excluding fading in dB
// **/
double PhyLteGetPathloss_dB(Node* node,
                            int phyIndex,
                            PhyLteTxInfo* lteTxInfo,
                            PropRxInfo* propRxInfo);

#ifdef LTE_LIB_LOG
lte::Aggregator* PhyLteGetAggregator(Node* node, int phyIndex);
#endif // LTE_LIB_LOG
// /**
// FUNCTION   :: PhyLteSetCheckingConnectionTimer
// LAYER      :: PHY
// PURPOSE    :: Set a CheckingConnection Timer
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteSetCheckingConnectionTimer(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteCheckingConnectionExpired
// LAYER      :: PHY
// PURPOSE    :: Checking Connection
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + phyIndex  : int      : Index of the PHY
// + msg       : Message* : CheckingConnection timer message
// RETURN     :: void  : NULL
// **/
void PhyLteCheckingConnectionExpired(Node* node, int phyIndex, Message* msg);

// /**
// FUNCTION   :: PhyLteSetCheckingConnectionTimer
// LAYER      :: PHY
// PURPOSE    :: Set a CheckingConnection Timer
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteSetInterferenceMeasurementTimer(Node* node, int phyIndex);

// /**
// FUNCTION   :: PhyLteInterferenceMeasurementTimerExpired
// LAYER      :: PHY
// PURPOSE    :: Timer for measuring interference power
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + phyIndex  : int      : Index of the PHY
// + msg       : Message* : InterferenceMeasurement timer message
// RETURN     :: void  : NULL
// **/
void PhyLteInterferenceMeasurementTimerExpired(Node* node, int phyIndex,
        Message* msg);

// /**
// FUNCTION   :: PhyLteIndicateDisconnectToUe
// LAYER      :: PHY
// PURPOSE    :: Indicate disconnection to UE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + phyIndex  : int            : Index of the PHY
// + ueRnti    : const LteRnti& : UE's RNTI
// RETURN     :: void  : NULL
// **/
void PhyLteIndicateDisconnectToUe(Node* node, int phyIndex,
        const LteRnti& ueRnti);

// /**
// FUNCTION   :: PhyLteGetCqiTable
// LAYER      :: PHY
// PURPOSE    :: Get the cqi table.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + phyIndex : int       : Index of the PHY.
// + len      : int*      : Pointer to The length of the cqi table.
// RETURN     :: PhyLteCqiTableColumn* : Pointer to the cqi table structure.
// **/
const PhyLteCqiTableColumn* PhyLteGetCqiTable(Node* node, int phyIndex,
        int* len);

#ifdef LTE_LIB_LOG

void PhyLteDebugOutputRxMsgInfoList(
        Node* node, int phyIndex, std::list < PhyLteRxMsgInfo* >* msgList);

void PhyLteDebugOutputRxMsgInfo(Node* node, int phyIndex,
        PhyLteRxMsgInfo* rxMsgInfo);
#endif



// /**
// FUNCTION   :: PhyLteClearInfo
// LAYER      :: PHY
// PURPOSE    :: clear information about opposite RNTI specified
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : int              : Index of the PHY
// + rnti      : const LteRnti&   : opposite RNTI
// RETURN     :: void  : NULL
// **/
void PhyLteClearInfo(Node* node, int phyIndex, const LteRnti& rnti);


// /**
// FUNCTION   :: PhyLteResetToPowerOnState
// LAYER      :: PHY
// PURPOSE    :: reset to poweron state
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// RETURN     :: void             : NULL
// **/
void PhyLteResetToPowerOnState(Node* node, const int phyIndex);


// /**
// FUNCTION   :: LteInterferenceArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle interference signal arrival from the chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving interference signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// + sigPower_mW  : double      : The inband interference power in mW
// RETURN     :: void : NULL
// **/
void PhyLteInterferenceArrivalFromChannel(Node* node,
                                    int phyIndex,
                                    int channelIndex,
                                    PropRxInfo *propRxInfo,
                                    double sigPower_mW);


// /**
// FUNCTION   :: LteInterferenceEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle interference signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving interference signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// + sigPower_mW  : double      : The inband interference power in mW
// RETURN     :: void : NULL
// **/
void PhyLteInterferenceEndFromChannel(Node* node,
                                int phyIndex,
                                int channelIndex,
                                PropRxInfo *propRxInfo,
                                double sigPower_mW);


#ifdef ADDON_DB
// /**
// FUNCTION   :: PhyLteGetPhyControlInfoSize
// LAYER      :: PHY
// PURPOSE    :: To get size of various control infos attached during packet
//               transmition in APIs PhyLteStartTransmittingSignal(...), and
//               PhyLteStartTransmittingSignalInEstablishment(...).
//               This API needs an update on adding new control infos in LTE.
// PARAMETERS ::
// + node           : Node*       : Pointer to node.
// + phyIndex       : Int32       : Index of the PHY.
// + channelIndex   : Int32       : Index of the channel
// + msg            : Message*    : Pointer to the Message
// RETURN     :: Int32            : Returns various control size attached.
// **/
Int32 PhyLteGetPhyControlInfoSize(Node* node,
                                  Int32 phyIndex,
                                  Int32 channelIndex,
                                  Message* msg);

// /**
// FUNCTION   :: PhyLteUpdateEventsTable
// LAYER      :: PHY
// PURPOSE    :: Updates Stats-DB phy events table for the received messages
// PARAMETERS ::
// + node           : Node*       : Pointer to node.
// + phyIndex       : Int32       : Index of the PHY.
// + channelIndex   : Int32       : Index of the channel
// + propRxInfo     : PropRxInfo* : Pointer to propRxInfo
// + msgToMac       : Message*    : Pointer to the Message
// + eventStr       : std::string : Receives eventType
// RETURN     :: void : NULL
// **/
void PhyLteUpdateEventsTable(Node* node,
                             Int32 phyIndex,
                             Int32 channelIndex,
                             PropRxInfo* propRxInfo,
                             Message* msgToMac,
                             std::string eventStr);

// /**
// FUNCTION   :: PhyLteNotifyPacketDropForMsgInRxPackedMsg
// LAYER      :: PHY
// PURPOSE    :: This API is used to Update event "PhyDrop" in Stats-DB phy
//               events table for those messages only are a part of the
//               Packed message received on PHY. For others places,
//               default API PHY_NotificationOfPacketDrop() is used.
// PARAMETERS ::
// + node                 : Node*       : Pointer to node.
// + phyIndex             : Int32       : Index of the PHY.
// + channelIndex         : Int32       : Index of the channel
// + propRxInfoTxMsg      : Message*    : Pointer to propRxInfo->txMsg
// + dropType             : std::string : Receives drop reason
// + rxPower_dB           : double      : Rx power in dB
// + pathloss_dB          : double      : Pathloss in dB
// + msgInRxPackedMsg     : Message*    : Pointer to the message in received
//                                        packed message.
// RETURN     :: void     : NULL
// **/
void PhyLteNotifyPacketDropForMsgInRxPackedMsg(
                                    Node* node,
                                    Int32 phyIndex,
                                    Int32 channelIndex,
                                    Message* propRxInfoTxMsg,
                                    const string& dropType,
                                    double rxPower_dB,
                                    double pathloss_dB,
                                    Message* msgInRxPackedMsg);
#endif // ADDON_DB
#endif /* _PHY_LTE_H_ */

