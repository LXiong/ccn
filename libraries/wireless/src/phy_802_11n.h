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


#ifndef PHY_802_11N_H
#define PHY_802_11N_H

#include "util_mini_matrix.h"
#include "mac_phy_802_11n.h"

/*
 * forward declarations
 */
struct PhyData802_11;
struct Phy802_11PlcpHeader;

/**Phy802_11n *
 * Class for 802.11n model 
 */ 
class Phy802_11n {
  public:
    /**
     * ENUMS
     */
    
    /** CodeRate
     * The enum types for 802.11n coding rate
     */
    enum CodeRate {
        CODERATE_1_2,        // 1/2
        CODERATE_2_3,        // 2/3
        CODERATE_3_4,        // 3/4
        CODERATE_5_6,        // 5/6
    };
    
    /** Modulation
     * The enum types for 802.11n modulation
     */
    enum Modulation {
        BPSK,
        QPSK,
        QAM16,
        QAM64,
    };
    

    /** 
     * classes and structs
     */
    /** MCSParam
     *  Modulation and coding parameters corresponding to each MCS index
     */
    struct MCSParam {
        unsigned char   nSpatialStream;            //number of spatial streams
        CodeRate        codeRate;                  //coding rate
        Modulation      modulation;                //modulation
        unsigned int    nDataBitsPerSymbol;        //number of data bits per OFDM symbol
        unsigned int    dataRate[GI_NUMS];         //data rate
    }; 

     /**TxRxVector
     ** TxRxVector defines per-packet transmit parameters supplied by 802.11 MAC */
    struct TxRxVector {
        Mode                format;                 //Transmit format
        ChBandwidth         chBwdth;                //Channel bandwidth
        size_t              length;                 //PSDU length, which may not be the same
                                                    //as packetSize as padding may be added.
        BOOL                sounding;               //Whether a sounding packet
        BOOL                containAMPDU;           //Whether contains a A-MPDU
        GI                  gi;                     //800ns or 400ns GI
        unsigned char       mcs;                    //MCS index
        unsigned char       stbc;                   //STBC, currently always 0 unless STBC is supported
        unsigned char       numEss;                 //Number of extension spatial streams
    };

    /**Stats
     * statistics
     */
    struct Stats {
        unsigned int totalTxGfSignals;              //Greenfield signals
        unsigned int totalRxGfSignalsToMac;
        unsigned int totalGfSignalsLocked;
        unsigned int totalGfSignalsWithErrors;
        unsigned int totalTxMfSignals;              //Mixed mode signals
        unsigned int totalRxMfSignalsToMac;
        unsigned int totalMfSignalsLocked;
        unsigned int totalMfSignalsWithErrors;
    };

    /**
     * Constants
     */
    static const int Channel_Bandwidth_Base = 20000000;   // 20 MHz
    static const int Max_EQM_MCS            = 32;         // Maximum EQM MCS index   
    static const MCSParam MCS_Params[CHBWDTH_NUMS][Max_EQM_MCS];
    static const MAC_PHY_TxRxVector Def_TxRxVector;               //Default TxRxVector value
    
    //constants related to preamble timing parameters, refer 802.11-2009 Table 20-5
    static const clocktype T_Dft    = 3200 * NANO_SECOND;          //IDFT/DFT period
    static const clocktype T_Gi     = 800 * NANO_SECOND;           //GI
    static const clocktype T_Gis    = 400 * NANO_SECOND;           //short GI
    static const clocktype T_Sym;                                  //Symbol interval 
    static const clocktype T_Syms;                                 //short GI symbol interval
    static const clocktype T_L_Stf  = 8 * MICRO_SECOND;            //Non-HT short traning symbol period
    static const clocktype T_L_Ltf  = 8 * MICRO_SECOND;            //Non-HT long training symbol period
    static const clocktype T_Gf_Stf = 8 * MICRO_SECOND;            //Greenfield short training period. 
    static const clocktype T_Ht_Sig = 8 * MICRO_SECOND;            //HT signal field duration
    static const clocktype T_Ht_Stf = 4 * MICRO_SECOND;            //HT short training field duration
    static const clocktype T_Gf_Ltf1= 8 * MICRO_SECOND;            //HT Greenfield first long training field duration
    static const clocktype T_Ht_Ltf1= 4 * MICRO_SECOND;            //HT first long training field duration 
    static const clocktype T_Ht_Ltfs= 4 * MICRO_SECOND;            //HT subsequent long training field duration 

    static const size_t Ppdu_Service_Bits_Size  = 16;
    static const size_t Ppdu_Tail_Bits_Size     = 6;

    //receiving sensetivity 
    static const size_t MCS_NUMS_SINGLE_SS  = 8; //Number of MCS for one spatial stream
                                                 //this is same as PHY802_11_NUM_DATA_RATES
    static const double Min_Rx_Senstivity[CHBWDTH_NUMS][MCS_NUMS_SINGLE_SS]; //Minimum reception
                                                                               //sensitivity

    /**
     * static functions
     */
    //Get STBC field from number of space-time streams and number of spatial streams
    static unsigned char GetStbc(unsigned char numSts, unsigned char numSs) {
        return numSts - numSs;
    }
    
    //Get number of space-time streams from STBC and number of spatial stream
    static unsigned char GetNSts(unsigned char stbc, unsigned char numSs) {
        return stbc + numSs;
    }

    //Get the preamble duration at HT greenfield mode 
    static clocktype PreambDur_HtGf(unsigned char numSts,
                                    unsigned char numEss)
    {
        unsigned char numHtDltf = (numSts == 3 ? 4 : numSts);
        unsigned char numHtEltf = (numEss == 3 ? 4 : numEss);
        return T_Gf_Stf + T_Gf_Ltf1 + T_Ht_Sig + (numHtDltf - 1 + numHtEltf)*T_Ht_Ltfs;
    }

    //Get the preamble duration at HT mixed mode
    static clocktype PreambDur_HtMixed(unsigned char numSts,
                                       unsigned char numEss)
    {
        unsigned char numHtDltf = (numSts == 3 ? 4 : numSts);
        unsigned char numHtEltf = (numEss == 3 ? 4 : numEss);
        return PreambDur_NonHt() + T_Ht_Sig 
            + T_Ht_Stf + T_Ht_Ltf1 + (numHtDltf  - 1 + numHtEltf)*T_Ht_Ltfs;
    }

    //Get the preamble duration at non-HT mode
    static clocktype PreambDur_NonHt() {
        return T_L_Stf + T_L_Ltf + T_Sym; 
    }

    //Get frame duration
    static clocktype GetFrameDuration(const MAC_PHY_TxRxVector& txParam); 

    /* GetDataRate()
     * Get datarate of a signal giving its associated TxRxVector
     */
    static double GetDataRate(const MAC_PHY_TxRxVector& txRxVector);

    /* GetNumSs()
     * Get number of spatial stream of a signal giving its associated TxRxVector
     */
    static size_t GetNumSs(const MAC_PHY_TxRxVector& txRxVector);

    /**
     ** Constructors and destructor          Sagarsuneja

     */
    Phy802_11n(PhyData802_11* p802_11); 
    ~Phy802_11n() { }

    /**
     * Public APIs
     */
    void Init(Node* node, const NodeInput* nodeInput);

    void Finalize(Node* node, int phyIndex);

    int    GetNumAtnaElems() const { return m_NumAtnaElmts; }

    double GetAtnaElemSpace() const { return m_AtnaSpace; }

    BOOL ShortGiCapable() const { return m_ShortGiCapable; }
    
    BOOL StbcCapable() const { return m_StbcCapable; }

    void SetTxVector(const MAC_PHY_TxRxVector& txVector);

    MAC_PHY_TxRxVector GetTxVector() const { return m_TxVector; }

    void SetOperationChBwdth(ChBandwidth chBwdth) 
    {
        m_ChBwdth = chBwdth;
        SetParamsUponBwdthChg();
    }
    ChBandwidth GetOperationChBwdth() const
    {
        return m_ChBwdth;
    }
    
    BOOL IsGiEnabled() const { return m_ShortGiEnabled; } 
    void EnableGi(BOOL enable) { m_ShortGiEnabled = enable; }

    void GetRxVectorOfLockedSignal(MAC_PHY_TxRxVector& rxVector) const 
    {
        rxVector = m_RxSignalInfo.rxVector;
    }

    void GetPreviousRxVector(MAC_PHY_TxRxVector& rxVector) const 
    {
        rxVector = m_PrevRxVector;
    }

    void FillPlcpHdr(Phy802_11PlcpHeader* plcpHdr) const;

    void LockSignal(Node* node,
                    int channelIndex,
                    Phy802_11PlcpHeader* plcpHdr, 
                    const Orientation& txDOA, 
                    const Orientation& rxDOA,
                    int    txNumAtnaElmts,
                    double txAtnaElmtSpace);
    void UnlockSignal();

    void ReleaseSignalToChannel(
        Node *node,
        Message *packet,
        int phyIndex,
        int channelIndex,
        float txPower_dBm,
        clocktype duration,
        clocktype delayUntilAirborne,
        double directionalAntennaGain_dB);

    /* CheckBer
     * Get BER for the current receiving signa;
     */
    double CheckBer(double snr) const;

    double GetDataRateOfRxSignal() const 
    {
        return GetDataRate(m_RxSignalInfo.rxVector);
    }

    double GetSensitivity()
    {
        return m_RxSensitivity_mW[GetOperationChBwdth()][0];
    }

  protected:
    struct  RxSignalInfo {
        MAC_PHY_TxRxVector          rxVector;           //RxVector
        int                 txNumAtnaElmts;
        double              txAtnaElmtSpace;
        CplxMiniMatrix      chnlEstMatrix;      //Channel estimation matrix       
    };

    PhyData802_11*  m_ParentData;       //Pointer to general Phy802.11 data

    /*
     * HT operation information
     * The initial values of operation information are read from configuration 
     * file, later, mobile stations's values are set according to the AP's values
     * when associated with it.
     */
    Mode            m_Mode;             //operation mode
    ChBandwidth     m_ChBwdth;          //operation channel bandwidth
    
    /*
     * HT-Capability info
     */
    int             m_NumAtnaElmts;     //Number of antenna array elements
    double          m_AtnaSpace;        //Space between antenna array elements
    BOOL            m_ShortGiCapable;   //Whether to support short GI 
    BOOL            m_StbcCapable;      //Whether to supoort STBC
    BOOL            m_ShortGiEnabled;   //Whether to enable short GI transmission

    double          m_RxSensitivity_mW[CHBWDTH_NUMS][MCS_NUMS_SINGLE_SS];
                                        //Minimum reception senstivity in MW.
    /*
     * Tx Rx signal related information
     */
    MAC_PHY_TxRxVector      m_TxVector;         //Transmission parameters for the
                                        //outgoing packet
    RxSignalInfo    m_RxSignalInfo;     //Receiving signal information 

    MAC_PHY_TxRxVector m_PrevRxVector;

    /* SetParamsUponBwdthChg
     * Set certain parameters associated with channel bandwidth whenever
     * operating channel bandwidth is set.  
     */
    void SetParamsUponBwdthChg();
    
    /* ReadCfgParams
     * Read configuration parameters particular to 802.11n
     */
    void ReadCfgParams(Node* node, const NodeInput* nodeInput);

};

void Phy802_11nSetBerTable(PhyData* thisPhy);

double PHY_MIMOBER(PhyData *thisPhy,
                   double snr,
                   MAC_PHY_TxRxVector rxVector,
                   double dataRate,
                   double bandwidth,
                   int numAtnaElmts,
                   CplxMiniMatrix channelMatrix);
#endif //PHY_802_11N_H

