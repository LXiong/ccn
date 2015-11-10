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


#ifndef MAC_GSM_H
#define MAC_GSM_H

#include "cellular_gsm.h"

typedef enum
{
    GSM_STATUS_TX,
    GSM_STATUS_RX,
    GSM_STATUS_IDLE
} GsmMacStatus_e;


/* Name:        GsmMSRrState_e
 * Purpose:     Represents the state of Radio Resource Management
 *              entity.
 * Reference:   GSM 04.07 Section 10.1
 */
typedef enum
{
    GSM_RR_STATE_NULL,
    GSM_RR_STATE_IDLE,
    GSM_RR_STATE_CONNECTION_PENDING,
    GSM_RR_STATE_DEDICATED_1,
    GSM_RR_STATE_DEDICATED_2,
} GsmMSRrState_e;



/* NOTE:
 * The meaning of Data Link (DL) primitives is slightly
 * different from those defined in the standards.
 * NEEDS MORE INVESTIGATION
 */
typedef enum
{
    // Data Link (DL) Primitives, Ref: GSM 04.06 ver 8.2.0
    // Refer Annex A for Random Access procedures
    // Also refer: GSM 04.07 ver 4.3.1, Annex A: Figure A.1 - A.5

    DL_GENERIC_MESSAGE,
    DL_GSM_MESSAGE
}GsmServicePrimitive_e;


typedef enum
{
    GSM_L3_MAC_CHANNEL_START_LISTEN,
    GSM_L3_MAC_CHANNEL_STOP_LISTEN,
    GSM_L3_MAC_HANDOVER,
    GSM_L3_MAC_CHANNEL_RELEASE,
    GSM_L3_MAC_SET_CHANNEL,
    GSM_L3_MAC_NEIGHBOURING_CELL_CHANNELS,
    GSM_L3_MAC_CELL_INFORMATION,
    GSM_L3_MAC_MEASUREMENT_REPORT,
    GSM_L3_MAC_CELL_SELECTION
} GsmLayer3ToMacInternalMessage_e;


typedef enum
{
    GSM_MAC_L3_PROP_DELAY_START_MESSAGE,
    GSM_MAC_L3_PROP_DELAY_STOP_MESSAGE
} GsmMacToLayer3InternalMessageType_e;


typedef enum
{
    // RR Service Primitives
    RR_EST_REQ,      // Layer3 Msg sent in SABM frame
    RR_EST_IND
} GsmRRServicePrimitives_e;


///////////////////////////////////////////////////////////
// Structures for Inter-Layer (Mac-Layer3) Messaging
//
typedef struct struct_gsm_inter_layer_message_str
{
    unsigned char   servicePrimitive;

    // Caller must allocate enough space for this message
    // and message Data. msgData can be casted and used
    // as ptr to set the message data.
    unsigned char   msgData[1];
} GsmMacLayer3InternalMessage;


typedef struct struct_gsm_channel_info_str
{
    uddtSlotNumber      slotNumber;
    uddtChannelIndex    channelIndex;
    GsmChannelType_e    channelType;
} GsmChannelInfo;


typedef struct struct_gsm_channel_list_str
{
    int                 numChannels;
    uddtChannelIndex    channelIndex[GSM_NUM_NEIGHBOUR_CELLS];
} GsmChannelList;

typedef struct struct_gsm_cell_info_str
{
    uddtCellId                  cellIdentity;
    uddtLAC                     lac;

    unsigned char               txInteger;
    unsigned char               maxReTrans;

    uddtChannelIndex            channelIndexStart;
    uddtChannelIndex            channelIndexEnd;
    uddtNumNeighBcchChannels    numNeighBcchChannels;

    uddtChannelIndex
    neighBcchChannelIndex[GSM_NUM_NEIGHBOUR_CELLS];
} GsmCellInfo;


typedef struct  struct_channel_measurement_str
{
    uddtSlotNumber      slotNumber;
    uddtChannelIndex    channelIndex;
    float               rxLevel_dBm;
    double              ber;
    int                 numRecorded;
    clocktype           lastUpdatedAt;
} GsmChannelMeasurement;


typedef struct struct_gsm_measurement_info_str
{
    uddtSlotNumber      slotNumber;
    uddtChannelIndex    channelIndex;

    // call back function
    //void    (*callBackFuncPtr)(Node *node);
    void    (
    *callBackFuncPtr) (
    Node* node,
    uddtSlotNumber   slotNumber,
    uddtChannelIndex   channelIndex,
    GsmChannelMeasurement* chMeas);
} GsmMeasurementInfo;


typedef struct struct_gsm_handover_info_str
{
    uddtSlotNumber      slotNumber;
    uddtChannelIndex    channelIndex;
    GsmChannelType_e    channelType;
    uddtChannelIndex    bcchChannelIndex;
    //unsigned char        hoReference;
} GsmHandoverInfo;

//
///////////////////////////////////////////////////////////


typedef struct gsm_sch_message_str
{
    unsigned char   bsic;   // BS Identity Code / 'Color Code'

    uddtFrameNumber frameNumber;
    clocktype       startTimeOfFrame;
} GsmSchMessage;


#ifdef GSM_LAPDM_SUPPORT

// Not used yet

/*
 * Purpose:     Describes the LAPD (Link Access Procedure - D channel) for
 *              Mobile frame structure. It is used as the data link frame
 *              for communication between the MS & BSS.
 * Reference:   GSM 04.06, Section 3
 *              GSM Text, Chapter 5
 */
typedef struct struct_gsm_lapdm_iframe_str
{
    // address fields
    unsigned char   addr_spare  : 1,
                    addr_lpd    : 2,     // Link Protocol Discriminator
                    addr_sapi   : 3,    // Service Access Point Identifier
                    addr_cr     : 1,      // Command/Response
                    addr_ea     : 1;      // Extended Address
    // = 1 => final address field octet
    // control fields
    unsigned char   ctrl_nr     : 3,  // Transmitter receive sequence number
                    ctrl_p      : 1,   // Poll bit for cmds
                    ctrl_ns     : 3,  // Transmitter send sequence number
                    spare       : 1;    // always set to 0

    // information: 0 to 21 octets
} GsmLapdmIFrame;

typedef struct struct_gsm_lapdm_sframe_str
{
    // address fields
    unsigned char   addr_spare  : 1,
                    addr_lpd    : 2,     // Link Protocol Discriminator
                    addr_sapi   : 3,    // Service Access Point Identifier
                    addr_cr     : 1,      // Command/Response
                    addr_ea     : 1;      // Extended Address
    // = 1 => final address field octet
    // control fields
    unsigned char   ctrl_nr     : 3,
                    // Transmitter receive sequence number
                    ctrl_pf     : 1,
                    // Poll bit for cmds, final bit for resp.
                    ctrl_ss     : 2,
                    //  Supervisory function bit
                    spare       : 2;    // always set to 01

    // information: 0 to 21 octets
} GsmLapdmSFrame;

typedef struct struct_gsm_lapdm_uframe_str
{
    // address fields
    unsigned char   addr_spare  : 1,
                    addr_lpd    : 2,     // Link Protocol Discriminator
                    addr_sapi   : 3,    // Service Access Point Identifier
                    addr_cr     : 1,      // Command/Response
                    addr_ea     : 1;      // Extended Address
    // = 1 => final address field octet
    // control fields
    unsigned char   ctrl_u3     : 3,
                    // Unnumbered function bit
                    ctrl_pf     : 1,
                    // Poll bit for cmds, final bit for resp.
                    ctrl_u2     : 2,
                    // Unnumbered function bit
                    spare       : 2;    // always set to 11

    // information: 0 to 21 octets
} GsmLapdmUFrame;


/*
 * Reference:       GSM 04.06, Section 3.7
 *                             Section 5.8.3 for max length size
 */
typedef struct struct_gsm_dl_length_indicator_field_str
{
    unsigned char   extLengh    : 1, // el, 1 => this is final octet of field
                    more        : 1,     // 1 => segmentation of layer 3 msgs
                    length      : 6;   // Length (in octets) of the info. field
    // Max (N201 in stds) is
    //      18 for SACCH
    //      20 for FACCH & SDCCH
    //      23 for BCCH, AGCH & PCH
} GsmDlLengthIndicator;


#endif // GSM_LAPDM_SUPPORT //

typedef struct struct_mac_gsm_stats_str
{
    int count;
} MacGsmStats;


typedef struct struct_gsm_bs_stats_str
{
    int count;
} MacGsmBsStats;


typedef struct struct_gsm_ms_stats_str
{
    int numCellSelections;
    int numCellSelectionsFailed;
    int numCellReSelectionAttempts;
} MacGsmMsStats;



typedef struct struct_bchh_list_info_str
{
    uddtChannelIndex    channelIndex;   // DownLink value

    BOOL                isSelected;     // if currently serving cell/BS
    BOOL                isNeighChannel; // if neighbour to currently serving cell

    uddtFrameNumber     nextFrameNumber;
    clocktype           lastDecodedAt;  //
    clocktype           startTimeOfNextFrame;   // Start time of next frame
    clocktype           slotTimeDiff;           // time diff bet. with serv cell

    BOOL                isCellSelected;
    BOOL                isFcchDetected;
    BOOL                isSchDetected;
    BOOL                isSyncInfoDecoded;

    BOOL                isBcchDecoded;

    clocktype           fcchDetectedAt;
    clocktype           schDetectedAt;

    // slot/frame setting

    // Measurement information
    float               rxLevel_dBm;
    double              ber;
    int                 numRecorded;
    clocktype           lastUpdatedAt;
    double              C1; // dBm, cell selection criteria, atleast 5 values rec.

    // Bug # 3436 fix - start
    clocktype listeningStartTime;
    int numSlotsListened;
    // Bug # 3436 fix - end

} GsmBcchListEntry;


typedef struct struct_mac_gsm_msinfo_str
{
    // MS properties
    double                      txPower_dBm;    // PHY_GSM_DEFAULT_TX_POWER_dBm or config file
    double                      maxTxPower_dBm; // PHY_GSM_MAX_TX_POWER_dBm or config file
    double                      rxLevAccessMin_dBm;  // PHY_GSM_DEFAULT_RX_THRESHOLD_dBm or config

    BOOL                        isFcchDetected; // For synchronization
    clocktype                   fcchDetectedAt;
    BOOL                        isSchDetected;

    BOOL                        isCellSelected; // Set on camping on a cell
    BOOL                        isBcchDecoded;
    BOOL                        isSysInfoType2Decoded;
    BOOL                        isSysInfoType3Decoded;

    BOOL                        isHandoverInProgress;
    GsmBcchListEntry*           hoCellBcchEntry;

    int                         numControlChannels;
    short
    controlChannels[GSM_MAX_NUM_CONTROL_CHANNELS_PER_MS];

    // read from the config file
    int                         numBcchChannels;
    uddtNumNeighBcchChannels    numNeighBcchChannels;
    GsmBcchListEntry            bcchList[GSM_MAX_NUM_CONTROL_CHANNELS_PER_MS];

    // for currently serving BS
    GsmBcchListEntry*           controlChannelBcchEntry;   // Quick access ptr
    clocktype                   timeSinceCellSelected;

    uddtChannelIndex            downLinkControlChannelIndex;
    uddtChannelIndex            upLinkControlChannelIndex;
    float                       rxLev;

    uddtCellId                  cellIdentity;   /* Ref: GSM 03.03 */
    uddtLAC                     lac;            /* Location Area Code, Ref: GSM 03.03 */
    // Access scheme & time-slot multiplexing
    uddtSlotNumber              slotNumber;     // Timeslot (0-7) being processed
    uddtFrameNumber             controlFrameNumber; // 0-50, "51 frame multiframe"
    uddtFrameNumber             trafficFrameNumber; // 0-25, "26 frame multiframe"
    uddtFrameNumber             frameNumber;        // 0 to GSM_MAX_FRAME_SIZE (2715647)

    clocktype                   currentSlotStartTime;
    clocktype                   startTimeOfNextFrame;

    // Used for determining BCCH listening periods while on TCH/SDCCH
    BOOL                        isIdleSlot_Rx_Tx_Interval;

    int                         sacchBlockNumber; // 0-3 (4x26)=104 frame SACCH multiframe=480MS

    // Rand number between 0 to max(8, TX-INTEGER)
    int                         randTxInteger;
    int                         randAccessFrame;

    short                       rand_disc;
    BOOL                        isRequestingChannel;
    int                         numChannelRequestAttempts;
    Message*                    channelReqMsg;

    // Allocated resources; assigned downlink channel = uplink - 1
    BOOL                        isDedicatedChannelAssigned; // => RR link exists

    uddtSlotNumber              assignedSlotNumber;         // Downlink (Rx) Slot
    uddtSlotNumber              assignedUplinkSlotNumber;   // Uplink (Tx) Slot
    uddtChannelIndex            assignedChannelIndex;       // Uplink channel index
    GsmChannelType_e            assignedChannelType;

    clocktype                   timingAdvance;
    Message*                    msgToSend;
    Message*                    cellSelectionTimer;

    // Measurements for assigned channel.
    float                       assignedChRxLevel_dBm;
    double                      assignedChBer;

    BOOL                        sendSChMeasFailure;
    clocktype                   assignedChMeasLastUpdatedAt;
    int                         numAssignedChMeasurements;

    // Used to determine the neighbour channel & timing for listening
    // during 'idle' slots while MS is on a TCH.
    int                         nextBcchChannelToListen; // index to bcchList array

    GsmMSRrState_e              rrState;

    MacGsmMsStats               stats;
    int                         count;

    // S counter to measure SACCH info at MS end
    // Minimum value will be one
    int                         sacchSCounter;
    // Bug # 3436 fix - start
    BOOL supportFlag;
    BOOL idleFramePassed;
    // Bug # 3436 fix - end
} MacGsmMsInfo;


typedef struct struct_mac_gsm_bsinfo_str
{
    uddtCellId                  cellIdentity;   /* Ref: GSM 03.03 */
    uddtLAC                     lac;            /* Location Area Code, Ref: GSM 03.03 */

    uddtChannelIndex            downLinkControlChannelIndex;
    uddtChannelIndex            upLinkControlChannelIndex;

    // Channels assigned to this BS/Cell.
    // The first two form the down & up link control channels.
    uddtChannelIndex            channelIndexStart;
    uddtChannelIndex            channelIndexEnd;
    int                         numChannels;

    // One phy device for each channel
    // Number of phy devices will run from
    // 0 to (channelIndexEnd - channelIndexStart).

    // Only for Uplink dedicated channel Measurements.
    // slot# is first index, channel pair# is second one.
    // channelPair# = (uplinkChannel# - upLinkControlChannel)/2
    GsmChannelMeasurement
    upLinkChMeas[GSM_SLOTS_PER_FRAME][GSM_MAX_NUM_CHANNELS_PER_BS / 2];

    // accessed by channel pair#
    BOOL
    ulChannelsInUse[GSM_MAX_NUM_CHANNELS_PER_BS / 2];

    // Neighbouring BS's BCCH channels
    short                       neighBcchChannels[GSM_NUM_NEIGHBOUR_CELLS];
    uddtNumNeighBcchChannels    numNeighBcchChannels;

    // current slot, frame information
    uddtSlotNumber              slotNumber;         // Slot# (0-7) within a frame
    uddtFrameNumber             controlFrameNumber; // 0-50, "51-frame multiframe"
    uddtFrameNumber             trafficFrameNumber; // 0-25, "26-frame multiframe"
    uddtFrameNumber             frameNumber;        // 0 to  (GSM_MAX_FRAME_SIZE - 1)

    clocktype                   startTimeOfCurrentSlot;

    //reduced Frame Number??? Calculate/convert when needed?

    unsigned                    numMsActive;

    // Pointer for messages that are sent often
    Message*                    sysInfoType2Msg;
    Message*                    sysInfoType3Msg;

    Message*                    sysInfoType5Msg;
    Message*                    dummyBurstMsg;

    Message*                    fcchMsg;
    Message*                    schMsg;

    MacGsmBsStats               stats;

    // Bug # 3436 fix - start
    BOOL supportFlag;
    // Bug # 3436 fix - end
} MacGsmBsInfo;


typedef struct struct_mac_gsm_str
{
    MacData*            myMacData;

    GsmNodeType_e       nodeType;

    // One of following is initialized based on node type
    MacGsmMsInfo*       msInfo;
    MacGsmBsInfo*       bsInfo;

    clocktype           bcchDelay; // Only used by Base Stations

    Message*            slotTimerMsg;

    int                 maxReTrans;
    int                 txInteger;

    RandomSeed          randSeed;

    int                 currentStatus;
    int                 currentSlotId;

    uddtChannelIndex    currentChannelIndex;
    int                 nextStatus;
    int                 nextSlotId;

    uddtChannelIndex    nextChannelIndex;

    int                 currentReceiveSlotId;

    Message*            timerMsg;
    clocktype           timerExpirationTime;

    int                 numSlotsPerFrame;

    clocktype           slotDuration;
    clocktype           guardTime;
    clocktype           interFrameTime;
    clocktype           frameDuration;

    BOOL                collectStatistics;
    MacGsmStats         stats;
    // Bug # 3436 fix - start
    Message*            idleTimerMsg;
    // Bug # 3436 fix - end
} MacDataGsm;


// Sent by a BS on its C0/Bcch channel if no other
// message is scheduled to be sent. Allows MS's to
// perform power measurements.
typedef struct struct_dummy_burst_str
{
    unsigned char   content[GSM_BURST_SIZE];
} GsmDummyBurstMsg;


/* NAME: System Information Type 3 Message
 * PURPOSE: Sent by BS over the Control Channel (BCCH) to MS
 *          giving info on RACH, location, cell identity etc.
 * REFERENCE: GSM 04.08, sec 9.1.35, Table 9.32
 * COMMENT:
 */
typedef struct struct_gsm_system_information_type3_str
{
    unsigned char   protocolDiscriminator; // Should use an enum
    unsigned char   messageType;


    uddtCellId      cellIdentity;
    uddtLAC         lac;//Location Area Code, instead of LAI

    // Synchronization info.
    // comment out these variables
    //int     nextFrameNumber;
    //clocktype startTimeOfNextFrame;
} GsmSysInfoType3Msg;


// Reference: GSM 04.08 Section 9.1.37
// Sent on SACCH, provides the BA(SACCH) list.
typedef struct struct_gsm_system_information_type5_str
{
    unsigned char               protocolDiscriminator;
    unsigned char               messageType;

    // Must be last element of struct
    uddtNumNeighBcchChannels    numNeighBcchChannels;

    unsigned char               neighBcchChannels[GSM_NUM_NEIGHBOUR_CELLS];
} GsmSysInfoType5Msg;


// Reference: GSM 04.08 Section 9.1.32
// Sent on BCCH, provides BA(BCCH) list & RACH parameters
typedef struct struct_gsm_system_information_type2_str
{
    unsigned char               protocolDiscriminator;
    unsigned char               messageType;

    // RACH control parameters (~slotted Aloha) for channel access
    unsigned char               txInteger;  //3-50, avg. time between repetitions (rand)
    unsigned char               maxReTrans; //1, 2, 4 or 7: number of repetitions

    // Must be last element of struct
    uddtNumNeighBcchChannels    numNeighBcchChannels;

    unsigned char               neighBcchChannels[GSM_NUM_NEIGHBOUR_CELLS];
} GsmSysInfoType2Msg;


typedef struct struct_gsm_measurement_report_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;

    unsigned char   rxLevFullServingCell;
    unsigned char   rxQualFullServingCell;

    unsigned char   numNeighCells;
    unsigned char   rxLevNeighCell[6];
    unsigned char   neighCellChannel[6];
} GsmMeasurementReportMsg;


/*
 * FUNCTION    MacGsmInit
 * PURPOSE     Initialization function for TDMA protocol of MAC layer.
 *
 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file
 */
void MacGsmInit(
    Node* node,
    int interfaceIndex,
    const NodeInput* nodeInput,
    const int slotsperframe);

/*
 * FUNCTION    MacGsmLayer
 * PURPOSE     Models the behaviour of the MAC layer with the TDMA protocol
 *             on receiving the message enclosed in msgHdr.
 *
 * Parameters:
 *     node:     node which received the message
 *     msgHdr:   message received by the layer
 */
void MacGsmLayer(
    Node* node,
    int interfaceIndex,
    Message* msg);


/*
 * FUNCTION    MacGsmFinalize
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of GSM protocol of the MAC Layer.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */
void MacGsmFinalize(
    Node* node,
    int interfaceIndex);

/*
 * FUNCTION    MacGsmNetworkLayerHasPacketToSend
 * PURPOSE     To tell GSM that the network layer has a packet to send.
 */

void MacGsmNetworkLayerHasPacketToSend(
    Node* node,
    MacDataGsm* gsm);


void MacGsmReceivePacketFromPhy(
    Node* node,
    MacDataGsm* gsm,
    Message* msg);


void MacGsmReceivePhyStatusChangeNotification(
    Node* node,
    MacDataGsm* gsm,
    PhyStatusType oldPhyStatus,
    PhyStatusType newPhyStatus);


void MacGsmStartListeningToChannel(
    Node* node,
    MacDataGsm* gsm,
    uddtChannelIndex       channelIndex);


int GsmParseChannelList(
    Node* node,
    unsigned    char* channelString,
    short* controlChannels);


BOOL GsmParseAdditionalBsInfo(
    unsigned    char* inputString,
    int* field1,
    int* field2,
    int* field3);


BOOL GsmParseAdditionalInfo(
    char* inputString,
    int* field1,
    uddtLAC* field2,
    uddtCellId* field3);


void MacGsmReceiveInternalMessageFromLayer3(
    Node* node,
    GsmMacLayer3InternalMessage* interLayerMsg,
    int     interfaceIndex);


BOOL MacGsmIsSacchMsgFrame(
    uddtSlotNumber   assignedSlotNumber,
    uddtFrameNumber     frameNumber);


unsigned char GsmMapRxLev(
    float rxLev_dBm);


unsigned char GsmMapRxQual(
    double ber);


// GSM 05.05 Section 4.11
// powerControlLevel range is 0-31 for GSM 900
// returns the output power in dBm
float mapGsmPowerControlLevelToDbm(
    int   powerControlLevel);

#endif
