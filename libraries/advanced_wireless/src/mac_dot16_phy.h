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

#ifndef MAC_DOT16_PHY_H
#define MAC_DOT16_PHY_H

//
// This is the header file of the 802.16 PHY specific part
//

// /**
// STRUCT      :: MacDot16PhyStats
// DESCRIPTION :: Statistics of the 802.16 PHY sublayer
// **/
typedef struct
{
    int numFramesSent;    // # of MAC frames sent
    int numFramesRcvd;    // # of MAC frames received
} MacDot16PhyStats;

// /**
// CONSTANT    :: DOT16_PHY_RADIO_TURNAROUND_TIME: 2US
// DESCRIPTION :: Turn around time of the wireles radio
// **/
#define DOT16_PHY_RADIO_TURNAROUND_TIME    (100 * NANO_SECOND)

//--------------------------------------------------------------------------
// OFDMA PHY related constants, enums, and structures
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: DOT16_OFDMA_NUM_SYMBOLS_PER_DL_PS    : 2
// DESCRPTION  :: Number of OFDM symbols per physical slot on donwlink.
// **/
#define DOT16_OFDMA_NUM_SYMBOLS_PER_DL_PS    2

// /**
// CONSTANT    :: DOT16_OFDMA_NUM_SYMBOLS_PER_UL_PS    : 3
// DESCRPTION  :: Number of OFDM symbols per physical slot on uplink.
// **/
#define DOT16_OFDMA_NUM_SYMBOLS_PER_UL_PS    3

// /**
// CONSTANT    :: DOT16_OFDMA_NUM_DIUC    : 16
// DESCRPTION  :: 802.16 OFDMA support 16 DIUC currently
// **/
#define DOT16_OFDMA_NUM_DIUC    16

// /**
// CONSTANT    :: DOT16_OFDMA_NUM_UIUC    : 16
// DESCRPTION  :: 802.16 OFDMA support 16 UIUC currently
// **/
#define DOT16_OFDMA_NUM_UIUC    16

// /**
// CONSTANT    :: DOT16_OFDMA_NUM_CDMA_CODE    : 256
// DESCRIPTION :: 802.16 OFDMA support 256 ranging code
// **/
#define DOT16_OFDMA_NUM_CDMA_CODE    256

// /**
// CONSTANT    :: DOT16_OFDMA_DEFAULT_NUM_INITIAL_RANGING_CODE    : N
// DESCRIPTION :: The first N codes starting from S (subgroup number) to
//                S + N-1 for initial ranging
// **/
#define DOT16_OFDMA_DEFAULT_NUM_INITIAL_RANGING_CODE 16

// /**
// CONSTANT    :: DOT16_OFDMA_DEFAULT_NUM_PERIODIC_RANGING_CODE    : M
// DESCRIPTION :: The first N codes starting from S + N (subgroup number)
//                to S + N + M -1 for periodic ranging
// **/
#define DOT16_OFDMA_DEFAULT_NUM_PERIODIC_RANGING_CODE 16

// /**
// CONSTANT    :: DOT16_OFDMA_DEFAULT_NUM_BANDWIDTH_RQUEST_CODE    : L
// DESCRIPTION :: The first N codes starting from S + M + N
//                (subgroup number) to S + M+ N + L -1 for initial ranging
// **/
#define DOT16_OFDMA_DEFAULT_NUM_BANDWIDTH_RQUEST_CODE 16

// /**
// ENUM        :: MacDot16OfdmaCdmaCodeType
// DESCRIPTION :: Types of OFDMA CDMA codes
// **/
typedef enum
{
    DOT16_OFDMA_INITIAL_RANGING_CODE    =    0,
    DOT16_OFDMA_PERIODIC_RANGING_CODE   =    1,
    DOT16_OFDMA_BANDWIDTH_REQUEST_CODE  =    2
}MacDot16OfdmaCdmaCodeType;

// /**
// CONSTANT    :: DOT16_OFDMA_NUM_FRAME_DURATION : 9
// DESCRIPTION :: 802.16 OFDMA supports 9 frame durations currently.
// **/
#define DOT16_OFDMA_NUM_FRAME_DURATION    9

// /**
// CONSTANT    :: DOT16_OFDMA_PREAMBLE_SYMBOL_LENGTH : 1
// DESCRIPTION :: Preable of the OFDMA PHY is 1 OFDMA symbol
// **/
#define DOT16_OFDMA_PREAMBLE_SYMBOL_LENGTH    1

// /**
// STRUCT      :: MacDot16PhyOfdmaFch
// DESCRIPTION :: PHY syncrhonization field of OFDMA PHY
//                To eliminate by alignment problem, the order of fields
//                is slightly adjusted, which won't affect sim results.
// **/
typedef struct
{
    unsigned char preamble;
    unsigned char usedSubchannelMap: 6,
                  repCodingIndication: 2;
    unsigned char rangeChangeIndication: 1,
                  codingIndication: 3,
                  reserved: 4;
    unsigned char dlMapLength;
} MacDot16PhyOfdmaFch;

// /**
// STRUCT      :: MacDot16PhyOfdmaSyncField
// DESCRIPTION :: PHY syncrhonization field of OFDMA PHY
// **/
typedef struct
{
    unsigned int durationCode:8,    // frame duration code
                 frameNumber:24;    // frame number, modulo 2^24
} MacDot16PhyOfdmaSyncField;

// /**
// STRUCT      :: MacDot16PhyOfdmaDlMapIE
// DESCRIPTION :: Data structure of OFMDA DL-MAP_IE
// **/
typedef struct
{
    unsigned char diuc: 4, // DIUC used in the DL burst
              padding: 4;
    unsigned int ofdmaSymbolOffset: 8, // OFDMA symbol offset
                  subchannelOffset: 6, // sunchannel offset
                          boosting: 3, // power boost, 000: normal
                   numOfdmaSymbols: 7, // number of OFDMA  symbols
                    numSubchannels: 6, // number of subchannels
               repCodingIndication: 2; // repeatition code


} MacDot16PhyOfdmaDlMapIE;

// /**
// STRUCT      :: MacDot16PhyOfdmaUlMapIE
// DESCRIPTION :: Data structure of OFMDA UL-MAP_IE
//                This is only for normal UL-MAP_IE whose UIUC
//                is not one of 12, 14, and 15
// **/
typedef struct
{
    unsigned int cid: 16,                 // CID that the IE assigned to
                 uiuc: 4,                 // UIUC used for the burst
                 duration: 10,            // duration of the burst
                 repCodingIndication: 2;  // Indicates the repetition code
} MacDot16PhyOfdmaUlMapIE;

typedef struct
{
    Dot16CIDType cid;                 // CID that the IE assigned to
    unsigned char uiuc: 4,                 // UIUC used for the burst
                  paddingNibble: 4;

    unsigned int ofmaSynbolOffset:8,      // Ofdma synbol number
                 subchannelOffset:7,      // OFDMA symbol subchannel
                 noOfOfdmaSymbols:7,      // number of OFDMA symbols
                 noOfSubchannels:7,          // number of subchannels
                 rangingMethod:2,          // the ranging method type
                 rangingIndicator: 1;       // Dedicated ranging indicator
} MacDot16PhyOfdmaUlMapIEuiuc12;

typedef struct
{
    Dot16CIDType cid;                 // CID that the IE assigned to
    unsigned short int uiuc: 4,                 // UIUC used for the burst
                       duration: 6,             // duration of the burst
                       uiuc_Transmission: 4,    // UIUC for transmission
                       repCodingIndication: 2;  // Indicates the repetition code
    unsigned char frameNumber: 4, // LSBs of relevant frame number
                  paddingNibble: 4;
    unsigned char rangingCode; // Ranging Code 8 bits
    unsigned char rangingSymbol; // Ranging Symbol 8 bits
    unsigned char rangingSubchannel: 7, // Ranging subchannel 7 bits
                  bwRequest: 1; // BW request mandatory 1 bit 1= yes, 0= no
} MacDot16PhyOfdmaUlMapIEuiuc14;

typedef union
{
    MacDot16PhyOfdmaUlMapIE ulMapIE;
    MacDot16PhyOfdmaUlMapIEuiuc12 ulMapIE12;
    MacDot16PhyOfdmaUlMapIEuiuc14 ulMapIE14;
} MacDot16GenericPhyOfdmaUlMapIE;
// /**
// STRUCT      :: MacDot16PhyOfdma
// DESCRIPTION :: Data structure of Dot16 OFDMA PHY sublayer
// **/
typedef struct struct_mac_dot16_phy_ofdma_str
{
    int frameNumber;    // frame number
    int numSubchannels; // # of sub-channels

    // frame duration code
    clocktype frameDurationList[DOT16_OFDMA_NUM_FRAME_DURATION];
    unsigned char frameDurationCode;

    MacDot16PhyStats stats;
} MacDot16PhyOfdma;

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16PhyAddPhySyncField
// LAYER      :: MAC
// PURPOSE    :: Add the PHY synchronization field
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + macFrame  : unsigned char*: Pointer to the MAC frame
// + startIndex: int           : starting position in MAC frame
// + frameLengh: int           : total length of the DL part of the frame
// RETURN     :: int : Number of bytes added
// **/
int MacDot16PhyAddPhySyncField(Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex,
                               int frameLength);


// /**
// FUNCTION   :: MacDot16PhyGetFrameNumber
// LAYER      :: MAC
// PURPOSE    :: Get frame number from PHY synchronization field
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + phySyncField  : unsigned char*: Pointer to PHY sync field
// RETURN     :: int : return frame number in PHY sync field
// **/
int MacDot16PhyGetFrameNumber(Node* node,
                                      MacDataDot16* dot16,
                                      unsigned char* macFrame);

// /**
// FUNCTION   :: MacDot16PhyGetFrameDuration
// LAYER      :: MAC
// PURPOSE    :: Get frame duration from PHY synchronization field
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + phySyncField  : unsigned char*: Pointer to PHY sync field
// RETURN     :: clocktype : return frame duration in PHY sync field
// **/
clocktype MacDot16PhyGetFrameDuration(Node* node,
                                MacDataDot16* dot16,
                                unsigned char* macFrame);

// /**
// FUNCTION   :: MacDot16PhySetFrameDuration
// LAYER      :: MAC
// PURPOSE    :: Set and validate the frame duration according to PHY type
// PARAMETERS ::
// + node          : Node*         : Pointer to node
// + dot16         : MacDataDot16* : Pointer to 802.16 data structure
// + frameDuration : clocktype     : Duration of the frame
// RETURN     :: void : NULL
// **/
void MacDot16PhySetFrameDuration(Node* node,
                                 MacDataDot16* dot16,
                                 clocktype frameDuration);
// /**
// FUNCTION   :: MacDot16PhyAddDlMapIE
// LAYER      :: MAC
// PURPOSE    :: Add DL MAP IE for one  burst
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 data struct
// + macFrame  : unsigned char* : Pointer to the MAC frame
// + dlMapIEIndex : int         : Index of the DL MAP IE in DL MAP
// + cid       :                : CID for this  burst
// + diuc      :                : diuc used in the busrt
// + burstInfo : Dot16BurstInfo : busrt info
// RETURN     :: int : Number of bytes added
// **/
int MacDot16PhyAddDlMapIE(Node* node,
                          MacDataDot16* dot16,
                          unsigned char* macFrame,
                          int startIndex,
                          Dot16CIDType cid,
                          unsigned char diuc,
                          Dot16BurstInfo burstInfo);

// /**
// FUNCTION   :: MacDot16PhyGetDlMapIE
// LAYER      :: MAC
// PURPOSE    :: Get one DL-MAP_IE
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16*       : Pointer to 802.16 data structure
// + macFrame  : unsigned char*  : Pointer to the MAC frame
// + startIndex: int             : starting position in MAC frame
// + cid       : Dot16CIDType*   : CID that the IE assigned to
// + diuc      : unsigned char*  : DIUC value
// + burstInfo : Dot16BurstInfo* : Pointer to the burst Info
// RETURN     :: int : Number of bytes processed
// **/
int MacDot16PhyGetDlMapIE(Node* node,
                          MacDataDot16* dot16,
                          unsigned char* macFrame,
                          int startIndex,
                          Dot16CIDType* cid,
                          unsigned char* diuc,
                          Dot16BurstInfo* burstInfo);

// /**
// FUNCTION   :: MacDot16PhyAddUlMapIE
// LAYER      :: MAC
// PURPOSE    :: Add one UL-MAP_IE
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + macFrame  : unsigned char*: Pointer to the MAC frame
// + startIndex: int           : starting position in MAC frame
// + frameLengh: int           : total length of the DL part of the frame
// + cid       : Dot16CIDType  : CID that the IE assigned to
// + uiuc      : unsigned char : UIUC value
// + duration  : int           : Duration of the burst
// RETURN     :: int : Number of bytes added
// **/
int MacDot16PhyAddUlMapIE(Node* node,
                          MacDataDot16* dot16,
                          unsigned char* macFrame,
                          int startIndex,
                          int frameLength,
                          Dot16CIDType cid,
                          unsigned char uiuc,
                          int duration,
                          unsigned char flag
                          = DOT16_CDMA_INITIAL_RANGING_OVER_2SYMBOL);

// /**
// FUNCTION   :: MacDot16PhyGetUlMapIE
// LAYER      :: MAC
// PURPOSE    :: Get one UL-MAP_IE
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + macFrame  : unsigned char*: Pointer to the MAC frame
// + startIndex: int           : starting position in MAC frame
// + cid       : Dot16CIDType* : CID that the IE assigned to
// + uiuc      : unsigned char*: UIUC value
// + duration  : clocktype*    : Duration of the burst
// RETURN     :: int : Number of bytes processed
// **/
int MacDot16PhyGetUlMapIE(Node* node,
                          MacDataDot16* dot16,
                          unsigned char* macFrame,
                          int startIndex,
                          Dot16CIDType* cid,
                          unsigned char* uiuc,
                          clocktype* duration,
                          MacDot16GenericPhyOfdmaUlMapIE* cdmaInfo);

// /**
// FUNCTION   :: MacDot16PhyTransmitMacFrame
// LAYER      :: MAC
// PURPOSE    :: Transmit a MAC frame
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// + dlMapLength      : unsigned char: Length of the DL-MAP
// RETURN     :: void : NULL
// **/
void MacDot16PhyTransmitMacFrame(Node* node,
                                 MacDataDot16* dot16,
                                 Message* msg,
                                 unsigned char dlMapLength,
                                 clocktype duration);

// /**
// FUNCTION   :: MacDot16PhyTransmitUlBurst
// LAYER      :: MAC
// PURPOSE    :: Transmit a Uplink burst
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// + channelIndex     : int          : Channel for transmission
// + delayInPs        : int          : Delay in PS for transmitting the
//                                     burst
// RETURN     :: void : NULL
// **/
void MacDot16PhyTransmitUlBurst(Node* node,
                                MacDataDot16* dot16,
                                Message* msg,
                                int channelIndex,
                                int delayInPs);


// /**
// FUNCTION   :: MacDot16PhyInit
// LAYER      :: MAC
// PURPOSE    :: Initialize 802.16 PHY sublayer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16PhyInit(Node* node,
                     int interfaceIndex,
                     const NodeInput* nodeInput);

// /**
// FUNCTION   :: MacDot16PhyFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16PhyFinalize(Node *node, int interfaceIndex);

// /**
// FUNCTION   :: MacDot16PhyReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer.
//               The PHY sublayer will first handle it
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacDot16PhyReceivePacketFromPhy(Node* node,
                                     MacDataDot16* dot16,
                                     Message* msg);

// /**
// FUNCTION   :: MacDot16PhyGetFrameOverhead
// LAYER      :: MAC
// PURPOSE    :: Get the additional overhead that PHY adds to the beginning
//               of each MAC frame in terms of number of bytes
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: int : PHY overhead in terms of number of bytes
// **/
int MacDot16PhyGetFrameOverhead(Node* node,
                                MacDataDot16* dot16);

// /**
// FUNCTION   :: MacDot16PhyGetPsDuration
// LAYER      :: MAC
// PURPOSE    :: Get the duration of a physical slot.
//               The physical slot duration is different for
//               different 802.16 PHY models. It may be different for
//               DL and UL.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + dot16        : MacDataDot16*: Pointer to DOT16 structure
// + subframeType : MacDot16SubframeType : Indicate DL or UL
// RETURN     :: clocktype : Duration of a physical slot.
// **/
clocktype MacDot16PhyGetPsDuration(Node* node,
                                   MacDataDot16* dot16,
                                   MacDot16SubframeType subframeType);

// /**
// FUNCTION   :: MacDot16PhyBitsPerPs
// LAYER      :: MAC
// PURPOSE    :: Get the number of Bits per PS given the burst profile
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + dot16        : MacDataDot16*: Pointer to DOT16 structure
// + burstProfile : void*        : Point to burst profile
// + subframeType : MacDot16SubframeType : Indicate DL or UL
// RETURN     :: int : # of bits per Ps
// **/
int MacDot16PhyBitsPerPs(Node* node,
                         MacDataDot16* dot16,
                         void* burstProfile,
                         MacDot16SubframeType subframeType);

// /**
// FUNCTION   :: MacDot16PhyBytesToPs
// LAYER      :: MAC
// PURPOSE    :: Get the number of physical slots needed to transmit
//               the given number of bytes
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + dot16        : MacDataDot16*: Pointer to DOT16 structure
// + size         : int          : Size in bytes
// + burstProfile : void*        : Point to the burst profile
// + subframeType : MacDot16SubframeType : Indicate DL or UL
// RETURN     :: int : # of physical slots needed
// **/
int MacDot16PhyBytesToPs(Node* node,
                         MacDataDot16* dot16,
                         int size,
                         void* burstProfile,
                         MacDot16SubframeType subframeType);

// /**
// FUNCTION   :: MacDot16PhySymbolsPerPs
// LAYER      :: MAC
// PURPOSE    :: Return # of OFDM symbols per PS
// PARAMETERS ::
// + subframeType : MacDot16SubframeType : Indicate DL or UL
// RETURN     :: int : # of OFMD symbols
// **/
static inline
int MacDot16PhySymbolsPerPs(MacDot16SubframeType subframeType)
{
    if (subframeType == DOT16_DL)
    {
        return DOT16_OFDMA_NUM_SYMBOLS_PER_DL_PS;
    }
    else
    {
        return DOT16_OFDMA_NUM_SYMBOLS_PER_UL_PS;
    }
}

// /**
// FUNCTION   :: MacDot16PhyGetNumSubchannels
// LAYER      :: MAC
// PURPOSE    :: Get number of subchannels supported by PHY layer
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: int  : Number of subcahnnels supported
// **/
int MacDot16PhyGetNumSubchannels(Node* node,
                                 MacDataDot16* dot16,
                                 MacDot16SubframeType subChannelType
                                 = DOT16_DL);

// /**
// FUNCTION   :: MacDot16PhyGetUplinkNumSubchannels
// LAYER      :: MAC
// PURPOSE    :: Get uplink number of subchannels supported by PHY layer
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: int  : Number of subcahnnels supported
// **/
int MacDot16PhyGetUplinkNumSubchannels(Node* node, MacDataDot16* dot16);

#endif // MAC_DOT16_PHY_H
