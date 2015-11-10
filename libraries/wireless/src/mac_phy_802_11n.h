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

#ifndef mac_phy_802_11n
#define mac_phy_802_11n

///** Mode
// *  The enum types for 802.11n HT operation modes and PPDU formats
// */
enum Mode {
    MODE_NON_HT,         // NON HT
    MODE_HT_MF,          // mixed mode
    MODE_HT_GF,          // green field mode
};

 ///** ChBandwidth
 //* The enum types for 802.11n channel bandwidth
 //*/
enum ChBandwidth {
    CHBWDTH_20MHZ,
    CHBWDTH_40MHZ,
    CHBWDTH_NUMS         // Total possible number of channel bandwidths
};

 ///** GI
 //* The enum types for 802.11n guard interval
 //*/
enum GI {
    GI_LONG,             // Default 800ns GI
    GI_SHORT,            // 400ns GI
    GI_NUMS              // Total number of guard intervals
};


///**MAC_PHY_TxRxVector
// * MAC_PHY_TxRxVector defines per-packet transmit parameters supplied by 802.11 MAC
// */
typedef struct MAC_PHY_TxRxVector{
    Mode                format;                 // Transmit format
    ChBandwidth         chBwdth;                // Channel bandwidth
    size_t              length;                 // PSDU length, which may not be the same
                                                // as packetSize as padding may be added.
    BOOL                sounding;               // Whether a sounding packet
    BOOL                containAMPDU;           // Whether contains a A-MPDU
    GI                  gi;                     // 800ns or 400ns GI
    unsigned char       mcs;                    // MCS index
    unsigned char       stbc;                   // STBC, currently always 0 unless STBC is supported
    unsigned char       numEss;  // Number of extension spatial streams

    // constuctor
    MAC_PHY_TxRxVector()
    {
        format = MODE_HT_GF;
        chBwdth = CHBWDTH_20MHZ;
        length = 0;
        sounding = FALSE;
        containAMPDU = FALSE;
        gi = GI_LONG;
        mcs = 0;
        stbc = 0;
        numEss = 0;
    }
}MAC_PHY_TxRxVector;

#endif
