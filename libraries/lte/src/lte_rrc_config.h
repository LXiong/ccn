#ifndef _LTE_RRC_CONFIG_H_
#define _LTE_RRC_CONFIG_H_

#include "lte_common.h"

struct LtePdcpConfig
{
    // no member at Phase 1
};

struct LteRlcConfig
{
    // RLC-LTE-MAX-RETX-THRESHOLD
    UInt8 maxRetxThreshold;
    // RLC-LTE-POLL-PDU
    int pollPdu;
    // RLC-LTE-POLL-BYTE
    int pollByte;
    // RLC-LTE-POLL-RETRANSMIT
    int tPollRetransmit;
    // RLC-LTE-T-REORDERINNG
    int tReordering;
    // RLC-LTE-T-STATUS-PROHIBIT
    int tStatusProhibit;

    LteRlcConfig()
    {
        maxRetxThreshold = 0;
        pollPdu          = 0;
        pollByte         = 0;
        tPollRetransmit  = 0;
        tReordering     = 0;
        tStatusProhibit  = 0;
    }
};

struct LteMacConfig
{
    //MAC-LTE-RA-BACKOFF-TIME
    clocktype raBackoffTime;

    //MAC-LTE-RA-PREAMBLE-INITIAL-RECEIVED-TARGET-POWER
    double raPreambleInitialReceivedTargetPower;

    //MAC-LTE-RA-POWER-RAMPING-STEP
    double raPowerRampingStep;

    //MAC-LTE-RA-PREAMBLE-TRANS-MAX
    int raPreambleTransMax;

    //MAC-LTE-RA-RESPONSE-WINDOW-SIZE
    // ra-ResponseWindowSize
    int raResponseWindowSize;

    //MAC-LTE-RA-PRACH-CONFIG-INDEX
    // prach-ConfigIndex
    int raPrachConfigIndex; // 0-15

    //MAC-LTE-PERIODIC-BSR-TTI
    // in 3GPP specification, number of sending every X subframe
    // in LTE Library, number of sending every X TTI
    int periodicBSRTimer;

    //MAC-LTE-TRANSMISSION-MODE
    int transmissionMode;

    LteMacConfig()
    {
        raBackoffTime = 0;
        raPreambleInitialReceivedTargetPower = 0;
        raPowerRampingStep = 0;
        raPreambleTransMax = 0;
        raResponseWindowSize = 0;
        raPrachConfigIndex = 0;
        periodicBSRTimer = 0;
        transmissionMode = 0;
    };

};

struct LtePhyConfig
{
    //PHY-LTE-DL-CHANNEL
    int dlChannelNo;

    //PHY-LTE-UL-CHANNEL
    int ulChannelNo;

    //PHY-LTE-TX-POWER
    double maxTxPower_dBm;

    //PHY-LTE-NUM-TX-ANTENNAS
    UInt8 numTxAntennas;

    //PHY-LTE-NUM-RX-ANTENNAS
    UInt8 numRxAntennas;

    //PHY-LTE-PUCCH-OVERHEAD
    UInt32 numPucchOverhead;

    //PHY-LTE-TPC-P_O_PUSCH
    double tpcPoPusch_mWPerRb;

    //PHY-LTE-TPC-ALPHA
    UInt8 tpcAlpha;

    ////PHY-LTE-CQI-REPORTING-INTERVAL // Not supported for phase 1
    //int cqiReportingInterval;

    ////PHY-LTE-CQI-REPORTING-OFFSET
    //int cqiReportingOffset;

    ////PHY-LTE-RI-REPORTING-INTERVAL
    //int riReportingInterval;

    ////PHY-LTE-RI-REPORTING-OFFSET
    //int riReportingOffset;

    //PHY-LTE-CELL-SELECTION-RX-LEVEL-MIN
    double cellSelectionRxLevelMin_dBm;

    //PHY-LTE-CELL-SELECTION-RX-LEVEL-MIN-OFFSET
    double cellSelectionRxLevelMinOff_dBm;

    //PHY-LTE-CELL-SELECTION-QUAL-LEVEL-MIN
    double cellSelectionQualLevelMin_dB;

    //PHY-LTE-CELL-SELECTION-QUAL-LEVEL-MIN-OFFSET
    double cellSelectionQualLevelMinOffset_dB;

    //PHY-LTE-RA-PRACH-FREQ-OFFSET
    UInt8 raPrachFreqOffset;

    ////PHY-LTE-SRS-TRANSMISSION-INTERVAL // Not supported for phase 1
    //int srsTransmissionInterval;

    ////PHY-LTE-SRS-TRANSMISSION-OFFSET
    //int srsTransmissionOffset;

    //PHY-LTE-NON-SERVING-CELL-MEASUREMENT-INTERVAL
    clocktype nonServingCellMeasurementInterval;

    //PHY-LTE-NON-SERVING-CELL-MEASUREMENT-PREIOD
    clocktype nonServingCellMeasurementPeriod;

    //PHY-LTE-CELL-SELECTION-MIN-SERVING-DURATION
    clocktype cellSelectionMinServingDuration;

    //PHY-LTE-RX-SENSITIVITY
    double rxSensitivity_dBm;

    LtePhyConfig()
    {
        dlChannelNo = 0;
        ulChannelNo = 0;
        maxTxPower_dBm = LTE_NEGATIVE_INFINITY_POWER_dBm;
        numTxAntennas = 0;
        numRxAntennas = 0;
        numPucchOverhead = 0;
        tpcPoPusch_mWPerRb = 0;
        tpcAlpha = 0;
//        cqiReportingInterval = 0; // Not supported for phase 1
//        cqiReportingOffset = 0;
//        riReportingInterval = 0;
//        riReportingOffset = 0;
        cellSelectionRxLevelMin_dBm = LTE_NEGATIVE_INFINITY_POWER_dBm;
        cellSelectionRxLevelMinOff_dBm = 0;
        cellSelectionQualLevelMin_dB = 0;
        cellSelectionQualLevelMinOffset_dB = 0;
        raPrachFreqOffset = 0;
//        srsTransmissionInterval = 0;
//        srsTransmissionOffset = 0;
        nonServingCellMeasurementInterval = 0;
        nonServingCellMeasurementPeriod = 0;
        cellSelectionMinServingDuration = 0;
        rxSensitivity_dBm = 0;
    };
};

struct LteRrcConfig
{
    LtePdcpConfig ltePdcpConfig; // PDCP Config
    LteRlcConfig  lteRlcConfig;  // RLC Config
    LteMacConfig  lteMacConfig;  // MAC Config
    LtePhyConfig  ltePhyConfig;  // PHY Config

    double filterCoefficient; // RRC-LTE-MEAS-FILTER-COEFFICIENT

    LteRrcConfig()
    {
        filterCoefficient = 0.0;
    };
};

//// Getter
// Config Obj pointer
LteRrcConfig* GetLteRrcConfig(Node* node, int interfaceIndex);
LtePdcpConfig* GetLtePdcpConfig(Node* node, int interfaceIndex);
LteRlcConfig* GetLteRlcConfig(Node* node, int interfaceIndex);
LteMacConfig* GetLteMacConfig(Node* node, int interfaceIndex);
LtePhyConfig* GetLtePhyConfig(Node* node, int interfaceIndex);

//// Setter
void
SetLteRrcConfig(Node* node, int interfaceIndex, LteRrcConfig& rrcConfig);

#endif // _LTE_RRC_CONFIG_H_
