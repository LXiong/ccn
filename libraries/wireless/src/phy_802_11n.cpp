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

#include "api.h"
#include "phy_802_11.h"
#include "phy_802_11n.h"
#include "partition.h"
#include "propagation.h"
#include "mac_phy_802_11n.h"

#define DEBUG 0
#define DEBUG_CHANMATRIX  0

#define MCS_STRUCT MCS_Params[MAC_PHY_TxRxVector.chBwdth][MAC_PHY_TxRxVector.mcs]

const clocktype Phy802_11n::T_Sym=
             Phy802_11n::T_Dft + Phy802_11n::T_Gi;  //Symbol interval 
const clocktype Phy802_11n::T_Syms=
           Phy802_11n::T_Dft + Phy802_11n::T_Gis;   //short GI symbol interval

const MAC_PHY_TxRxVector Phy802_11n::Def_TxRxVector;

const Phy802_11n::MCSParam 
  Phy802_11n::MCS_Params[CHBWDTH_NUMS][Phy802_11n::Max_EQM_MCS] = 
    { { { 1, CODERATE_1_2,  BPSK,   26, {   6500000,  7200000 } },
        { 1, CODERATE_1_2,  QPSK,   52, {  13000000, 14400000 } },
        { 1, CODERATE_3_4,  QPSK,   78, {  19500000, 21700000 } },
        { 1, CODERATE_1_2, QAM16,  104, {  26000000, 28900000 } },
        { 1, CODERATE_3_4, QAM16,  156, {  39000000, 43300000 } },
        { 1, CODERATE_2_3, QAM64,  208, {  52000000, 57800000 } },
        { 1, CODERATE_3_4, QAM64,  234, {  58500000, 65000000 } },
        { 1, CODERATE_5_6, QAM64,  260, {  65000000, 72200000 } },
        { 2, CODERATE_1_2,  BPSK,   52, {  13000000, 14400000 } }, 
        { 2, CODERATE_1_2,  QPSK,  104, {  26000000, 28900000 } },
        { 2, CODERATE_3_4,  QPSK,  156, {  39000000, 43300000 } },
        { 2, CODERATE_1_2, QAM16,  208, {  52000000, 57800000 } },
        { 2, CODERATE_3_4, QAM16,  312, {  78000000, 86700000 } },
        { 2, CODERATE_2_3, QAM64,  416, { 104000000,115600000 } },
        { 2, CODERATE_3_4, QAM64,  468, { 117000000,130000000 } },
        { 2, CODERATE_5_6, QAM64,  520, { 130000000,144400000 } },
        { 3, CODERATE_1_2,  BPSK,   78, {  19500000, 21700000 } }, 
        { 3, CODERATE_1_2,  QPSK,  156, {  39000000, 43300000 } },
        { 3, CODERATE_3_4,  QPSK,  234, {  58500000, 65000000 } },
        { 3, CODERATE_1_2, QAM16,  312, {  78000000, 86700000 } },
        { 3, CODERATE_3_4, QAM16,  468, { 117000000,130000000 } },
        { 3, CODERATE_2_3, QAM64,  624, { 156000000,173300000 } },
        { 3, CODERATE_3_4, QAM64,  702, { 175500000,195000000 } },
        { 3, CODERATE_5_6, QAM64,  780, { 195000000,216700000 } },
        { 4, CODERATE_1_2,  BPSK,  104, {  26000000, 28900000 } }, 
        { 4, CODERATE_1_2,  QPSK,  208, {  52000000, 57800000 } },
        { 4, CODERATE_3_4,  QPSK,  312, {  78000000, 86700000 } },
        { 4, CODERATE_1_2, QAM16,  416, { 104000000,115600000 } },
        { 4, CODERATE_3_4, QAM16,  624, { 156000000,173300000 } },
        { 4, CODERATE_2_3, QAM64,  832, { 208000000,231100000 } },
        { 4, CODERATE_3_4, QAM64,  936, { 234000000,260000000 } },
        { 4, CODERATE_5_6, QAM64, 1040, { 260000000,288900000 } } },
      { { 1, CODERATE_1_2,  BPSK,   54, {  13500000, 15000000 } },
        { 1, CODERATE_1_2,  QPSK,  108, {  27000000, 30000000 } },
        { 1, CODERATE_3_4,  QPSK,  162, {  40500000, 45000000 } },
        { 1, CODERATE_1_2, QAM16,  216, {  54000000, 60000000 } },
        { 1, CODERATE_3_4, QAM16,  324, {  81000000, 90000000 } },
        { 1, CODERATE_2_3, QAM64,  432, { 108000000,120000000 } },
        { 1, CODERATE_3_4, QAM64,  486, { 121500000,135000000 } },
        { 1, CODERATE_5_6, QAM64,  540, { 135000000,150000000 } },
        { 2, CODERATE_1_2,  BPSK,  108, {  27000000, 30000000 } }, 
        { 2, CODERATE_1_2,  QPSK,  216, {  54000000, 60000000 } },
        { 2, CODERATE_3_4,  QPSK,  324, {  81000000, 90000000 } },
        { 2, CODERATE_1_2, QAM16,  432, { 108000000,120000000 } },
        { 2, CODERATE_3_4, QAM16,  648, { 162000000,180000000 } },
        { 2, CODERATE_2_3, QAM64,  864, { 216000000,240000000 } },
        { 2, CODERATE_3_4, QAM64,  972, { 243000000,270000000 } },
        { 2, CODERATE_5_6, QAM64, 1080, { 270000000,300000000 } },
        { 3, CODERATE_1_2,  BPSK,  162, {  40500000, 45000000 } }, 
        { 3, CODERATE_1_2,  QPSK,  324, {  81000000, 90000000 } },
        { 3, CODERATE_3_4,  QPSK,  486, { 121500000,135000000 } },
        { 3, CODERATE_1_2, QAM16,  648, { 162000000,180000000 } },
        { 3, CODERATE_3_4, QAM16,  972, { 243000000,270000000 } },
        { 3, CODERATE_2_3, QAM64, 1296, { 324000000,360000000 } },
        { 3, CODERATE_3_4, QAM64, 1458, { 364500000,405000000 } },
        { 3, CODERATE_5_6, QAM64, 1620, { 405000000,450000000 } },
        { 4, CODERATE_1_2,  BPSK,  216, {  54000000, 60000000 } }, 
        { 4, CODERATE_1_2,  QPSK,  432, { 108000000,120000000 } },
        { 4, CODERATE_3_4,  QPSK,  648, { 162000000,180000000 } },
        { 4, CODERATE_1_2, QAM16,  864, { 216000000,240000000 } },
        { 4, CODERATE_3_4, QAM16, 1296, { 324000000,360000000 } },
        { 4, CODERATE_2_3, QAM64, 1728, { 432000000,480000000 } },
        { 4, CODERATE_3_4, QAM64, 1944, { 486000000,540000000 } },
        { 4, CODERATE_5_6, QAM64, 2160, { 540000000,600000000 } } } };

const double Phy802_11n::Min_Rx_Senstivity[CHBWDTH_NUMS]
                                          [Phy802_11n::MCS_NUMS_SINGLE_SS] = 
    { { -82.0, -79.0, -77.0, -74.0, -70.0, -66.0, -65.0, -64.0 },
      { -79.0, -76.0, -74.0, -71.0, -67.0, -63.0, -62.0, -61.0 } }; 



clocktype Phy802_11n::GetFrameDuration(const MAC_PHY_TxRxVector& txParam) 
{
    // Reference: IEEE 802.11 2009 20.4.3
    size_t dataBits;
    size_t numSymbols;
    clocktype frameDur = 0;
    clocktype symbolDur = (txParam.gi == GI_LONG ? T_Sym : T_Syms);  
    unsigned int numEs = 1;                                             //Number of BCC coder, 
    unsigned int numSts;                                               //Number of spatial streams
    unsigned int mStbc = 1;

    // section 20.3.11.3
    if (MCS_Params[txParam.chBwdth][txParam.mcs].dataRate[txParam.gi] > 300000000u) {
        numEs = 2;         
    }

    switch (txParam.format) {
        case MODE_HT_GF:
        {
            if (txParam.stbc) {
                mStbc = 2;
            }
            numSts = GetNSts(txParam.stbc, 
                MCS_Params[txParam.chBwdth][txParam.mcs].nSpatialStream);

            dataBits = txParam.length*8 + Ppdu_Service_Bits_Size + Ppdu_Tail_Bits_Size*numEs;
            numSymbols = mStbc * (size_t)ceil((double)dataBits/
                   (MCS_Params[txParam.chBwdth][txParam.mcs].nDataBitsPerSymbol*mStbc));

            frameDur = PreambDur_HtGf(numSts, txParam.numEss) 
                            + (numSymbols * symbolDur);
            break;
        }
        case MODE_HT_MF:
        {
            if (txParam.stbc) {
                mStbc = 2;
            }
            numSts = GetNSts(txParam.stbc, 
                MCS_Params[txParam.chBwdth][txParam.mcs].nSpatialStream);

            dataBits = txParam.length*8 + Ppdu_Service_Bits_Size + Ppdu_Tail_Bits_Size*numEs;
            numSymbols = mStbc * (size_t)ceil((double)dataBits/
                   (MCS_Params[txParam.chBwdth][txParam.mcs].nDataBitsPerSymbol*mStbc));

            if (txParam.gi == GI_LONG) {
                frameDur = PreambDur_HtMixed(numSts, txParam.numEss) 
                             + numSymbols * T_Sym;
            }
            else
            {
                frameDur = PreambDur_HtMixed(numSts, txParam.numEss) 
                             + T_Sym * ((size_t)ceil((double)numSymbols * T_Syms/T_Sym));
            }
            break;
        }
        case MODE_NON_HT:
        {
            dataBits = txParam.length*8 + Ppdu_Service_Bits_Size + Ppdu_Tail_Bits_Size;
            numSymbols = (size_t)ceil((double)dataBits/
                   MCS_Params[txParam.chBwdth][txParam.mcs].nDataBitsPerSymbol);

            frameDur = PreambDur_NonHt() + (numSymbols * symbolDur);
            break;
        }
        default:
            ;
    }

    {
        char durStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(frameDur, durStr);
        DEBUG_PRINT("Phy802_11n::GetFrameDuration: frame length(bytes): %"
            TYPES_SIZEOFMFT "d, "
            "frame MCS: %u, frame duration %s \n", 
            txParam.length, txParam.mcs, durStr);
    }
    return frameDur;
}

double Phy802_11n::GetDataRate(const MAC_PHY_TxRxVector& txRxVector) 
{
     return (double) (MCS_Params[txRxVector.chBwdth][txRxVector.mcs]).dataRate[txRxVector.gi];
    //return (double)(MCS_STRUCT[].dataRate[txRxVector.gi]);
}

size_t Phy802_11n::GetNumSs(const MAC_PHY_TxRxVector& txRxVector)
{
    return  MCS_Params[txRxVector.chBwdth][txRxVector.mcs].nSpatialStream;
}

Phy802_11n::Phy802_11n(PhyData802_11* p802_11)
        : m_ParentData(p802_11)
{
    m_Mode          = MODE_HT_GF;       //Make Greenfield default operating mode
    m_ChBwdth       = CHBWDTH_20MHZ;
    m_NumAtnaElmts  = 1;
    m_AtnaSpace     = 0.5;
    m_ShortGiCapable= FALSE;
    m_StbcCapable   = FALSE;
    m_ShortGiEnabled= FALSE;

    for (int i = 0; i < CHBWDTH_NUMS; i++) {
        for (int j = 0; j < MCS_NUMS_SINGLE_SS; j++) {
            m_RxSensitivity_mW[i][j] = NON_DB(Min_Rx_Senstivity[i][j]);
        }
    }

    // Setup additional parameters used in parentData
    m_ParentData->channelBandwidth = (m_ChBwdth + 1)*Channel_Bandwidth_Base;
    m_ParentData->txPower_dBm = PHY802_11a_DEFAULT_TX_POWER__6M_dBm;
    m_ParentData->rxTxTurnaroundTime = PHY802_11a_RX_TX_TURNAROUND_TIME;
    m_ParentData->numDataRates = PHY802_11a_NUM_DATA_RATES;
    m_ParentData->lowestDataRateType = PHY802_11a_LOWEST_DATA_RATE_TYPE;
    m_ParentData->highestDataRateType = PHY802_11a_HIGHEST_DATA_RATE_TYPE;
}

void Phy802_11n::ReadCfgParams(Node* node, const NodeInput* nodeInput)
{
    BOOL    wasFound;
    char    cfgStr[MAX_STRING_LENGTH];
    char    errStr[MAX_STRING_LENGTH];
    double  rxSensitivity_dBm;
    double  txPower_dBm;
    int phyIndex = m_ParentData->thisPhy->phyIndex; 

    //
    // Number of attenna elements 
    //
    IO_ReadInt(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-NUM-ANTENNA-ELEMENTS",
        &wasFound,
        &m_NumAtnaElmts);

    if (m_NumAtnaElmts <= 0 || m_NumAtnaElmts > 4) 
    {
        sprintf(errStr, "The range of PHY802.11n-NUM-ANTENNA-ELEMENTS" 
            "is between 1 and 4!");
        ERROR_ReportError(errStr);
    }

    //
    // inter-attenna-element space
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-ANTENNA-ELEMENT-SPACE",
        &wasFound,
        &m_AtnaSpace);

    if (m_AtnaSpace <= 0) 
    {
        sprintf(errStr, "The range of PHY802.11n-ANTENNA-ELEMENT-SPACE" 
            "is larger than 0");
        ERROR_ReportError(errStr);
    }

    // 
    // Whether short GI is supported
    //
    IO_ReadBool(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-SHORT-GI-CAPABLE",
        &wasFound,
        &m_ShortGiCapable);

    //
    // Whether STBC is supported
    //
    IO_ReadBool(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-STBC-CAPABLE",
        &wasFound,
        &m_StbcCapable);

    //
    // Minimum reception senstivity
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-20MHz-MCS0",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_20MHZ][0] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-20MHz-MCS1",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_20MHZ][1] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-20MHz-MCS2",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_20MHZ][2] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-20MHz-MCS3",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_20MHZ][3] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-20MHz-MCS4",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_20MHZ][4] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-20MHz-MCS5",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_20MHZ][5] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-20MHz-MCS6",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_20MHZ][6] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-20MHz-MCS7",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_20MHZ][7] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-40MHz-MCS0",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_40MHZ][0] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-40MHz-MCS1",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_40MHZ][1] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-40MHz-MCS2",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_40MHZ][2] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-40MHz-MCS3",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_40MHZ][3] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-40MHz-MCS4",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_40MHZ][4] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-40MHz-MCS5",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_40MHZ][5] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-40MHz-MCS6",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_40MHZ][6] = NON_DB(rxSensitivity_dBm);
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-RX-SENSITIVITY-40MHz-MCS7",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        m_RxSensitivity_mW[CHBWDTH_40MHZ][7] = NON_DB(rxSensitivity_dBm);
    }

    //
    // operation channel bandwidth
    //
    IO_ReadString(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-CHANNEL-BANDWIDTH",
        &wasFound,
        cfgStr);

    if (wasFound) {
        if (!strcmp(cfgStr, "40MHz") ||
            !strcmp(cfgStr, "40MHZ") ||
            !strcmp(cfgStr, "40Mhz")) {
            m_ChBwdth = CHBWDTH_40MHZ;
        }
        else if (! (!strcmp(cfgStr, "20MHz") ||
            !strcmp(cfgStr, "20MHZ") ||
            !strcmp(cfgStr, "20Mhz"))) {
            sprintf(errStr, "Error: Value for PHY802_11n-CHANNEL-BANDWIDTH "
                "must be either 20MHz or 40MHz\n");
            ERROR_ReportError(errStr);
        }
    }

    SetParamsUponBwdthChg();

    //
    // Whether use short guard interval
    //
    IO_ReadBool(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-ENABLE-SHORT-GI",
        &wasFound,
        &m_ShortGiEnabled);

    //
    // Set PHY802_11n-TX-POWER
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY802.11n-TX-POWER",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        m_ParentData->txPower_dBm = (float)txPower_dBm;
    }
} 

void Phy802_11n::SetParamsUponBwdthChg()
{
    m_ParentData->channelBandwidth = (m_ChBwdth + 1)*Channel_Bandwidth_Base;
    m_ParentData->noisePower_mW =
        m_ParentData->thisPhy->noise_mW_hz * m_ParentData->channelBandwidth;
    if (m_ChBwdth == CHBWDTH_40MHZ) {
        memcpy(&m_ParentData->rxSensitivity_mW[0],
               &m_RxSensitivity_mW[CHBWDTH_40MHZ][0],
               sizeof(m_RxSensitivity_mW[CHBWDTH_40MHZ]));
    }
    else {
        memcpy(&m_ParentData->rxSensitivity_mW[0],
               &m_RxSensitivity_mW[CHBWDTH_20MHZ][0],
               sizeof(m_RxSensitivity_mW[CHBWDTH_20MHZ]));
    }
} 

void Phy802_11n::Init(Node* node, const NodeInput* nodeInput)
{
    ReadCfgParams(node, nodeInput);
}

void Phy802_11n::Finalize(Node* node, int phyIndex)
{
}

void Phy802_11n::SetTxVector(const MAC_PHY_TxRxVector& txVector)
{
    m_TxVector = txVector;
}

void Phy802_11n::FillPlcpHdr(Phy802_11PlcpHeader* plcpHdr) const
{
    plcpHdr->format     = m_TxVector.format;
    plcpHdr->chBwdth    = m_TxVector.chBwdth;
    plcpHdr->length     = m_TxVector.length;
    plcpHdr->sounding   = m_TxVector.sounding;
    plcpHdr->containAMPDU = m_TxVector.containAMPDU ;
    plcpHdr->gi         = m_TxVector.gi;
    plcpHdr->mcs        = m_TxVector.mcs;
    plcpHdr->stbc       = m_TxVector.stbc;
    plcpHdr->numEss     = m_TxVector.numEss;
}

void Phy802_11n::LockSignal(Node* node,
                            int channelIndex,
                            Phy802_11PlcpHeader* plcpHdr, 
                            const Orientation& txDOA, 
                            const Orientation& rxDOA,
                            int    txNumAtnaElmts,
                            double txAtnaElmtSpace)
{
    //Store tx parameters for the receiving parameters
    m_RxSignalInfo.rxVector.format      = plcpHdr->format;
    m_RxSignalInfo.rxVector.chBwdth     = plcpHdr->chBwdth;
    m_RxSignalInfo.rxVector.length      = plcpHdr->length;
    m_RxSignalInfo.rxVector.sounding    = plcpHdr->sounding;
    m_RxSignalInfo.rxVector.containAMPDU= plcpHdr->containAMPDU;
    m_RxSignalInfo.rxVector.gi          = plcpHdr->gi;
    m_RxSignalInfo.rxVector.mcs         = plcpHdr->mcs;
    m_RxSignalInfo.rxVector.stbc        = plcpHdr->stbc;
    m_RxSignalInfo.rxVector.numEss      = plcpHdr->numEss;
    
    m_RxSignalInfo.txNumAtnaElmts       = txNumAtnaElmts;
    m_RxSignalInfo.txAtnaElmtSpace      = txAtnaElmtSpace;

    // Calculate MIMO channel matrix estimation
    PropProfile* propProfile = 
        node->partitionData->propChannel[channelIndex].profile;

    m_RxSignalInfo.chnlEstMatrix = MIMO_EstimateChnlMatrix(
                                     propProfile,
                                     m_ParentData->thisPhy->seed,
                                     txAtnaElmtSpace,
                                     txNumAtnaElmts,
                                     m_AtnaSpace,
                                     m_NumAtnaElmts,
                                     txDOA,
                                     rxDOA);
    

    m_PrevRxVector = m_RxSignalInfo.rxVector;

    if (DEBUG_CHANMATRIX) 
    {
        std::cout << "At time:" << node->getNodeTime() << " node:" 
            << node->nodeId <<"receives a signal.Number of transmit antennas:"
            << txNumAtnaElmts << " number of receive antennas "
            << m_NumAtnaElmts <<" Channel matrix: " << std::endl; 
        std::cout << m_RxSignalInfo.chnlEstMatrix << std::endl;
    }
}

void Phy802_11n::UnlockSignal()
{
    m_RxSignalInfo.rxVector = Def_TxRxVector;
    m_RxSignalInfo.txNumAtnaElmts = 0;
    m_RxSignalInfo.txAtnaElmtSpace = 0.0;
    m_RxSignalInfo.chnlEstMatrix.Clear();
}

void Phy802_11n::ReleaseSignalToChannel(
        Node *node,
        Message *packet,
        int phyIndex,
        int channelIndex,
        float txPower_dBm,
        clocktype duration,
        clocktype delayUntilAirborne,
        double directionalAntennaGain_dB)
{
    double txPowerPerAtna_mw = NON_DB(txPower_dBm)/m_NumAtnaElmts;
    PROP_ReleaseSignal(
       node,
       packet,
       phyIndex,
       channelIndex,
       (float)(IN_DB(txPowerPerAtna_mw) - directionalAntennaGain_dB),
       duration,
       delayUntilAirborne,
       m_NumAtnaElmts,
       m_AtnaSpace);
}

double Phy802_11n::CheckBer(double snr) const
{
    double      ber;
    double      bandwidth = (m_RxSignalInfo.rxVector.chBwdth + 1)*Channel_Bandwidth_Base;
    double      dataRate = GetDataRateOfRxSignal();

    ber = PHY_MIMOBER(m_ParentData->thisPhy,
                      snr,
                      m_RxSignalInfo.rxVector,
                      dataRate,
                      bandwidth,
                      std::min(m_RxSignalInfo.txNumAtnaElmts, m_NumAtnaElmts),
                      m_RxSignalInfo.chnlEstMatrix);
    return ber;
}


