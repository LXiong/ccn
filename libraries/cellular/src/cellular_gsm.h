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

// /**
// PACKAGE     :: GSM
// DESCRIPTION :: This file defines the data structures used by GSM models.
// **/

#ifndef _GSM_H_
#define _GSM_H_

// /**
// CONSTANT    :: GSM_BURST_SIZE  :   19
// DESCRIPTION :: GSM burst size in bytes
// **/
#define     GSM_BURST_SIZE                  19 // bytes , ~156.25 bits

// /**
// CONSTANT    :: GSM_BURST_DATA_SIZE  :  14
// DESCRIPTION :: Size of usable part of a GSM burst bytes
// **/
#define     GSM_BURST_DATA_SIZE             14 // bytes , ~114 bits

// /**
// CONSTANT    :: GSM_CHANNEL_BANDWIDTH  :   200000
// DESCRIPTION :: GSM Channel Bandwidth in Hz
// **/
#define     GSM_CHANNEL_BANDWIDTH           200000

// /**
// CONSTANT    :: GSM_MAX_FRAME_SIZE  : 2715647
// DESCRIPTION :: Defines the Hyperframe size (3h 28m 53s 760ms)
// Maximum Frame Number FN_MAX: 0 to (26 x 51 x 2048 - 1)
// **/
#define     GSM_MAX_FRAME_SIZE              2715647

// /**
// CONSTANT    :: GSM_MAX_SUPERFRAME  : 2048
// DESCRIPTION :: Number of superframes in a hyperframe
// **/
#define     GSM_MAX_SUPERFRAME              2048

// /**
// CONSTANT    :: GSM_SUPERFRAME_SIZE  : 1326
// DESCRIPTION :: Defines the size of GSM superframe
// **/
// 6.12s, 26 x 51 multiframes
#define     GSM_SUPERFRAME_SIZE             1326

// /**
// CONSTANT    :: GSM_SACCH_MULTIFRAME_SIZE  :  104
// DESCRIPTION :: Defines GSM SACCH MULTIFRAME SIZE
// **/
// 26 x 4 FRAMES = 480MS
#define     GSM_SACCH_MULTIFRAME_SIZE       104

// /**
// CONSTANT    :: GSM_CONTROL_MULTIFRAME_SIZE : 51
// DESCRIPTION :: Defines the GSM CONTROL MULTIFRAME SIZE
// **/
// ~235ms
#define     GSM_CONTROL_MULTIFRAME_SIZE     51

// /**
// CONSTANT    :: GSM_TRAFFIC_MULTIFRAME_SIZE : 26
// DESCRIPTION :: Specifies the GSM TRAFFIC MULTIFRAME SIZE
// **/
// 120ms
#define     GSM_TRAFFIC_MULTIFRAME_SIZE     26

// /**
// CONSTANT    :: GSM_SLOTS_PER_FRAME  :  8
// DESCRIPTION :: Defines the number of slots per GSM frame
// 4.61ms (577us x 8)
// **/
#define     GSM_SLOTS_PER_FRAME             8

// /**
// CONSTANT    :: GSM_MAX_NUM_MSC :  1
// DESCRIPTION :: Defines the maximum no of MSC
// **/
#define     GSM_MAX_NUM_MSC             1


// /**
// CONSTANT    :: GSM_MAX_BS_PER_MSC :  7
// DESCRIPTION :: Specifies maximum number of base stations per MSC
// **/
#define     GSM_MAX_BS_PER_MSC              7

// /**
// CONSTANT    :: GSM_MAX_MS_PER_BS :  100
// DESCRIPTION :: Specifies SCCP connection limit
// **/
#define     GSM_MAX_MS_PER_BS               100

// /**
// CONSTANT    :: GSM_MAX_MS_PER_MSC  :  200
// DESCRIPTION :: VLR limit
// **/
#define     GSM_MAX_MS_PER_MSC              200

// /**
// CONSTANT    :: GSM_MAX_SIMALT_CALLS :  50
// DESCRIPTION :: Max simultaneous calls & traffic connections
// **/
#define     GSM_MAX_SIMALT_CALLS            50

// /**
// CONSTANT    :: GSM_MAX_NUM_CALLS :  50
// DESCRIPTION :: Max number of calls
// **/
#define     GSM_MAX_NUM_CALLS               50

// /**
// CONSTANT    :: GSM_MIN_NUM_CHANNELS_PER_BS : 2
// DESCRIPTION :: Minimum no. of channels per BS
// Is <= MAX_NUM_CHANNELS; always an even number (Uplink/Downlink)
// **/
#define     GSM_MIN_NUM_CHANNELS_PER_BS     2

// /**
// CONSTANT    :: GSM_MAX_NUM_CHANNELS_PER_BS : 8
// DESCRIPTION :: Maximum no of channels per BS
// **/
#define     GSM_MAX_NUM_CHANNELS_PER_BS     8

// /**
// CONSTANT    :: GSM_NUM_NEIGHBOUR_CELLS : 6
// DESCRIPTION :: Defines the no. of NEIGHBOUR CELLS
// **/
#define     GSM_NUM_NEIGHBOUR_CELLS         6

// /**
// CONSTANT    :: GSM_MAX_ARFCN_PER_BS : (GSM_MAX_NUM_CHANNELS_PER_BS / 2)
// DESCRIPTION :: Defines GSM max ARF channels per BS
// Always an even number
// **/
#define     GSM_MAX_ARFCN_PER_BS    (GSM_MAX_NUM_CHANNELS_PER_BS / 2)

// /**
// CONSTANT    :: GSM_MAX_ARFCN_NUM :  124
// DESCRIPTION :: Defines GSM MAX ARF Channels= 124(0-123)
// **/
#define     GSM_MAX_ARFCN_NUM               124

// /**
// CONSTANT    :: GSM_MAX_NUM_CONTROL_CHANNELS_PER_MS : 7
// DESCRIPTION :: Defines GSM MAX NUM of CONTROL CHANNELS PER MS
// **/
#define     GSM_MAX_NUM_CONTROL_CHANNELS_PER_MS    7

// /**
// CONSTANT    :: GSM_MAX_IMSI_LENGTH : 10
// DESCRIPTION :: Defines the constant MAX IMSI LENGTH
// **/
#define     GSM_MAX_IMSI_LENGTH             10

// /**
// CONSTANT    :: GSM_MAX_MSISDN_LENGTH : 10
// DESCRIPTION :: Specifies the GSM MAX MSISDN LENGTH
// **/
#define     GSM_MAX_MSISDN_LENGTH           10

// /**
// CONSTANT    :: GSM_MIN_CALL_DURATION : (1 * SECOND)
// DESCRIPTION :: Defines the GSM MIN CALL DURATION
// **/
#define     GSM_MIN_CALL_DURATION           (1 * SECOND)

// /**
// CONSTANT    :: GSM_AVG_CALL_DURATION : (10 * SECOND)
// DESCRIPTION :: Defines the GSM Average CALL DURATION
// **/
#define     GSM_AVG_CALL_DURATION           (10 * SECOND)

// /**
// CONSTANT    :: GSM_BCCH_REFRESH_PERIOD : (5 * SECOND)
// DESCRIPTION :: Defines the 'timeout' period for BCCH decoding
// **/
#define     GSM_BCCH_REFRESH_PERIOD         (5 * SECOND)

// /**
// CONSTANT    :: GSM_TRAFFIC_INPUT_RATE_DELAY : (20 * MILLI_SECOND)
// DESCRIPTION :: Defines the GSM TRAFFIC INPUT RATE DELAY
// **/
#define     GSM_TRAFFIC_INPUT_RATE_DELAY    (20 * MILLI_SECOND)

// /**
// CONSTANT    :: GSM_CONTROL_CH_MSG_BLOCK_SIZE :  23
// DESCRIPTION :: Defines the GSM CONTROL Channel message BLOCK SIZE
// **/
#define     GSM_CONTROL_CH_MSG_BLOCK_SIZE   23 // bytes, 184 bits

// /**
// CONSTANT    :: GSM_MAX_INFO_OCTETS_IN_CCCH_MSG :  23
// DESCRIPTION :: Specifies the GSM MAX INFO OCTETS IN CCCH MSG
// N201(max info size) values for the control channels
// CCCH = BCCH, PCH & AGCH
// **/
#define     GSM_MAX_INFO_OCTETS_IN_CCCH_MSG         23

// /**
// CONSTANT    :: GSM_MAX_INFO_OCTETS_IN_FACCH_SDCCH_MSG : 21
// DESCRIPTION :: Defines the GSM max information octets in FACCH SDCCH_MSG
// **/
#define     GSM_MAX_INFO_OCTETS_IN_FACCH_SDCCH_MSG  21

// /**
// CONSTANT    :: GSM_MAX_INFO_OCTETS_IN_SACCH_MSG  :  18
// DESCRIPTION :: Defines the GSM max information octets in SACCH_MSG
// **/
#define     GSM_MAX_INFO_OCTETS_IN_SACCH_MSG        18

// /**
// CONSTANT    :: GSM_TRAFFIC_CH_MSG_BLOCK_SIZE : 260
// DESCRIPTION :: Defines the GSM traffic channel MSG_BLOCK_SIZE
// For TCH, FACCH & SACCH
// **/
#define     GSM_TRAFFIC_CH_MSG_BLOCK_SIZE   260  // bits, 32.5 bytes

// /**
// CONSTANT    :: GSM_CODED_FRAME_SIZE : 57
// DESCRIPTION :: Defines coded frame size
// For TCH 1: speech frame, for Control CH: one message
// **/
#define     GSM_CODED_FRAME_SIZE            57 // bytes, 456 bits

// /**
// CONSTANT    :: GSM_MAC_FILL_OCTET  : 0x2b
// DESCRIPTION :: Defines GSM MAC FILL_OCTET
// **/
#define     GSM_MAC_FILL_OCTET              0x2b // 0010 1011

// /**
// CONSTANT    :: GSM_DUMMY_BURST_OCTET  :  0xfd
// DESCRIPTION :: Defines GSM DUMMY_BURST_OCTET ,section GSM 05.02 may be
// referred for exact formatting. Used primarily to distinguish from other
// messages.
// **/
#define     GSM_DUMMY_BURST_OCTET           0xfd


// Power, Measurement & Handover related constants. See GSM 05.08

// /**
// CONSTANT    :: GSM_NUM_MEAS_SAMPLES : 32
// DESCRIPTION :: Defines the GSM number of measurement samples constant
// **/
#define     GSM_NUM_MEAS_SAMPLES            32

// /**
// CONSTANT    :: GSM_RXLEV_MIN_DEF : 20
// DESCRIPTION :: Defines the default GSM minimum receive level
// Mapped value
// **/
#define     GSM_RXLEV_MIN_DEF               20 // Mapped value

// /**
// CONSTANT    :: GSM_L_RXLEV_DL_H :  -90
// DESCRIPTION :: Defines  downlink receive level
// **/
#define     GSM_L_RXLEV_DL_H                -90 // -103 to -73 dBm

// /**
// CONSTANT    :: GSM_L_RXLEV_UL_H : -80
// DESCRIPTION :: Defines the GSM uplink receive level constant
// **/
#define     GSM_L_RXLEV_UL_H                -80 // -103 to -73 dBm

// /**
// CONSTANT    :: GSM_L_RXQUAL_DL_H : 0.08
// DESCRIPTION :: Defines the GSM DL receive quality for handover
// **/
#define     GSM_L_RXQUAL_DL_H               0.08 // ~ mapped 6

// /**
// CONSTANT    :: GSM_L_RXQUAL_UL_H :  0.08
// DESCRIPTION :: Defines the GSM UL receive quality for handover
// **/
#define     GSM_L_RXQUAL_UL_H               0.08 // ~ mapped 6

// /**
// CONSTANT    :: GSM_HREQAVE : 8
// DESCRIPTION :: Defines GSM handover required average
// **/
// GSM_HREQAVE x GSM_HREQT < 32
#define     GSM_HREQAVE                     8

// /**
// CONSTANT    :: GSM_HREQT : 4
// DESCRIPTION :: Defines GSM_HREQT
// **/
#define     GSM_HREQT                       4

// /**
// CONSTANT    :: GSM_INVALID_SLOT_NUMBER : -1
// DESCRIPTION :: Defines GSM INVALID SLOT NUMBER
// **/
#define     GSM_INVALID_SLOT_NUMBER     -1

// /**
// CONSTANT    :: GSM_INVALID_CHANNEL_INDEX : -1
// DESCRIPTION :: Defines the GSM INVALID CHANNEL INDEX
// **/
#define     GSM_INVALID_CHANNEL_INDEX   -1

// /**
// CONSTANT    :: GSM_INVALID_CELL_IDENTITY : -1
// DESCRIPTION :: Defines the GSM INVALID CELL IDENTITY
// **/
#define     GSM_INVALID_CELL_IDENTITY   -1

// /**
// CONSTANT    :: GSM_INVALID_LAC_IDENTITY : -1
// DESCRIPTION :: Defines GSM INVALID LAC_IDENTITY    -1
// **/
#define     GSM_INVALID_LAC_IDENTITY    -1

// /**
// CONSTANT    :: GSM_INVALID_CHANNEL_TYPE : 255
// DESCRIPTION :: Defines the GSM INVALID CHANNEL TYPE
// **/
#define     GSM_INVALID_CHANNEL_TYPE    255


// Defines the Random Access variables
// See GSM 04.08 Section 3.3.1, Table 3.1 for values to use

// /**
// CONSTANT    :: GSM_TX_INTEGER : 8
// DESCRIPTION :: Defines the GSM Transmission integer
// **/
#define     GSM_TX_INTEGER              8

// /**
// CONSTANT    :: GSM_MAX_RETRANS :  4
// DESCRIPTION :: Defines the GSM maximum retransmission value
// **/
#define     GSM_MAX_RETRANS             4

// /**
// CONSTANT    :: GSM_RANDACCESS_S : 55
// DESCRIPTION :: Defines the 'S' parameter from Table 3.1
// **/
#define     GSM_RANDACCESS_S            55

// /**
// CONSTANT    :: GSM_CellReselectionParameter_C2 : 2.0
// DESCRIPTION :: GSM 03.22, section 3.5.2.2
// Cells are reselected on the basis of a parameter called C2
// and the C2 value for each cell is given a positive or negative
// offset to encourage or discourage MSs to reselect that cell.
// **/
#define   GSM_CellReselectionParameter_C2 2.0

// /**
// CONSTANT    :: GSM_BSMinimumSACCH_UpdationPeriod : 5 * SECOND
// DESCRIPTION ::
// **/
#define   GSM_BSMinimumSACCH_UpdationPeriod ( 5 * SECOND)

// /**
// CONSTANT    :: GSM_BCCHRefreshVariable : 15
// DESCRIPTION :: Since bound checking is not present in the QualNet So
//                refresh the variable after received fix no of records.
// **/
#define   GSM_BCCHRefreshVariable  ( 15)

// /**
// CONSTANT    :: DefaultGsmSlotDuration : (577 * MICRO_SECOND)
// DESCRIPTION :: Specifies the default GsmSlotDuration
// **/
static const clocktype  DefaultGsmSlotDuration                          = (
        577 * MICRO_SECOND );

// /**
// CONSTANT    :: DefaultGsmGuardTime : 0
// DESCRIPTION :: Specifies default guard time between time slots
// **/
static const clocktype  DefaultGsmGuardTime                             = 0;

// /**
// CONSTANT    :: DefaultGsmFrameDuration : 4.616 MILLI_SECOND
// DESCRIPTION :: Specifies the default frame duration (8 time slots)
// **/
static const clocktype  DefaultGsmFrameDuration                         =
    577 * MICRO_SECOND* GSM_SLOTS_PER_FRAME;

// /**
// CONSTANT    :: DefaultGsmHandoverMargin : 0.0
// DESCRIPTION :: Specifies the default Handover Margin of a Base Station.
// Prevents repetitive handover between adjacent cells. Range (0 to 24 dB).
// **/
static const clocktype  DefaultGsmHandoverMargin                        = 0;

// /**
// CONSTANT    :: DefaultGsmInterFrameTime : 0
// DESCRIPTION :: Specifies the default GsmInterFrameTime
// **/
static const clocktype  DefaultGsmInterFrameTime                        = 0;

// /**
// CONSTANT    :: DefaultGsmCellSelectionTime : (3 * SECOND)
// DESCRIPTION :: Specifies the default Gsm Cell SelectionTime
// **/
static const clocktype  DefaultGsmCellSelectionTime                     = (
        3 * SECOND );

// /**
// CONSTANT    :: DefaultGsmCellReSelectionTime : (5 * SECOND)
// DESCRIPTION :: Specifies default Gsm Cell ReSelectionTime
// **/
static const clocktype  DefaultGsmCellReSelectionTime                   = (
        5 * SECOND );

// /**
// CONSTANT    :: DefaultGsmHandoverStartTimeDelay : (10 * MILLI_SECOND)
// DESCRIPTION :: Specifies default Handover StartTimeDelay
// **/
static const clocktype  DefaultGsmHandoverStartTimeDelay                = (
        10 * MILLI_SECOND );
//
// /**
// CONSTANT    :: DefaultGsmAlertingTimer_T301Time : (20 * SECOND)
// DESCRIPTION :: Defines the default Alerting (ringing) Timer duration
// **/
static const clocktype  DefaultGsmAlertingTimer_T301Time                = (
        20 * SECOND );

// /**
// CONSTANT    :: DefaultGsmCmServiceAcceptTimer_UDT1Time : (10 * SECOND)
// DESCRIPTION :: Defines the default Cm service accept Timer duration
// **/
static const clocktype  DefaultGsmCmServiceAcceptTimer_UDT1Time         = (
        10 * SECOND );


// /**
// CONSTANT    :: DefaultGsmCallPresentTimer_T303Time : (10 * SECOND)
// DESCRIPTION :: Defines the default Call Present Timer duration
// **/
static const clocktype  DefaultGsmCallPresentTimer_T303Time             = (
        10 * SECOND );

// /**
// CONSTANT    :: DefaultGsmDisconnectTimer_T305Time : (10 * SECOND)
// DESCRIPTION :: Defines the default Call Disconnect Timer duration
// **/
static const clocktype  DefaultGsmDisconnectTimer_T305Time              = (
        10 * SECOND );

// /**
// CONSTANT    :: DefaultGsmReleaseTimer_T308Time : (10 * SECOND)
// DESCRIPTION :: Defines the default Call Release Timer duration
// **/
static const clocktype  DefaultGsmReleaseTimer_T308Time                 = (
        10 * SECOND );

// /**
// CONSTANT    :: DefaultGsmCallProceedingTimer_T310Time : (10 * SECOND)
// DESCRIPTION :: Defines the default Call Proceeding Timer duration
// **/
static const clocktype  DefaultGsmCallProceedingTimer_T310Time          = (
        10 * SECOND );

// /**
// CONSTANT    :: DefaultGsmConnectTimer_T313Time : (10 * SECOND)
// DESCRIPTION :: Defines the default Connect Timer duration
// **/
static const clocktype  DefaultGsmConnectTimer_T313Time                 = (
        10 * SECOND );

// /**
// CONSTANT    :: DefaultGsmImmediateAssignmentTimer_T3101Time : (500 * MILLI_SECOND)
// DESCRIPTION :: Defines the GSM Immediate Assignment Timer duration
// **/
static const clocktype  DefaultGsmImmediateAssignmentTimer_T3101Time    = (
        500 * MILLI_SECOND );

// /**
// CONSTANT    :: DefaultGsmHandoverTimer_T3103Time : (2 * SECOND)
// DESCRIPTION :: Defines the GSM Handover Timer duration
// **/
static const clocktype  DefaultGsmHandoverTimer_T3103Time               = (
        2 * SECOND );

// /**
// CONSTANT    :: DefaultGsmMsChannelReleaseTimer_T3110Time : (200 * MILLI_SECOND)
// DESCRIPTION :: Defines the GSM MS Channel Release Timer duration
// **/
static const clocktype  DefaultGsmMsChannelReleaseTimer_T3110Time       = (
        200 * MILLI_SECOND );

// /**
// CONSTANT    :: DefaultGsmBsChannelReleaseTimer_T3111Time : (200 * MILLI_SECOND)
// DESCRIPTION :: Defines the GSM BS Channel Release Timer duration
// **/
static const clocktype  DefaultGsmBsChannelReleaseTimer_T3111Time       = (
        200 * MILLI_SECOND );

// /**
// CONSTANT    :: DefaultGsmBsTimer_SACCHTime : (3 * SECOND)
// DESCRIPTION :: Defines the GSM BS SACCH Timer duration
// **/
static const clocktype  DefaultGsmBsTimer_SACCHTime                     = (
        3 * SECOND );

// /**
// CONSTANT    :: DefaultGsmPagingTimer_T3113Time : (5 * SECOND)
// DESCRIPTION :: Specifies the default Paging Timer duration
// **/
static const clocktype  DefaultGsmPagingTimer_T3113Time                 = (
        5 * SECOND );

// /**
// CONSTANT    :: DefaultGsmLocationUpdateRequestTimer_T3210Time : (10 * SECOND)
// DESCRIPTION :: Defines the default Location Update Request Timer duration
// **/
static const clocktype  DefaultGsmLocationUpdateRequestTimer_T3210Time  = (
        10 * SECOND );


// /**
// CONSTANT    :: DefaultGsmLocationUpdateFailureTimer_T3211Time : (10 * SECOND)
// DESCRIPTION :: Defines the default Location Update Request Timer duration
// **/
static const clocktype  DefaultGsmLocationUpdateFailureTimer_T3211Time  = (
        10 * SECOND );

// /**
// CONSTANT    :: DefaultGsmPeriodicLocationUpdateTimer_T3212Time : (6 * MINUTE)
// DESCRIPTION :: Defines the default Periodic Location Update Timer duration
// **/
static const clocktype  DefaultGsmPeriodicLocationUpdateTimer_T3212Time = (
        6 * MINUTE );

// /**
// CONSTANT    :: DefaultGsmPeriodTimer_T3213Time : (10 * SECOND)
// DESCRIPTION ::
//  Ref::  GSM 4.08, section 11.2, table 11.1
// **/
static const clocktype  DefaultGsmPeriodTimer_T3213Time                 = (
        10 * SECOND );


// /**
// CONSTANT    :: DefaultGsmCmServiceRequestTimer_T3230Time : (15 * SECOND)
// DESCRIPTION :: Defines the default CM Service Request Timer duration
// **/
static const clocktype  DefaultGsmCmServiceRequestTimer_T3230Time       = (
        15 * SECOND );

// /**
// CONSTANT    :: DefaultGsmTimer_T3240Time : (10 * SECOND)
// DESCRIPTION :: Defines the default Timer T3240 duration
// **/
static const clocktype  DefaultGsmTimer_T3240Time                       = (
        10 * SECOND );

// Here uddt mean user define data type
// uddtLAC used to indicate a Location Area Code
typedef short           uddtLAC;
// uddtCellId used to indicate a CellIdentity
typedef int             uddtCellId;
// uddtChannelIndex used to indicate a ChannelIndex
typedef int             uddtChannelIndex;
// uddtNumNeighBcchChannels used to indicate a NumNeighBcchChannels
typedef char            uddtNumNeighBcchChannels;
// uddtSlotNumber used to indicate a SlotNumber
typedef char            uddtSlotNumber;
// uddtFrameNumber used to indicate a FrameNumber
typedef unsigned int    uddtFrameNumber;



// /**
// ENUM        :: GsmNodeType_e
// DESCRIPTION :: Identifies node type in MacDataGsm struct
// **/
typedef enum
{
    GSM_MS  = 1,
    GSM_BS  = 2,
    GSM_MSC = 3
} GsmNodeType_e;

// /**
// ENUM        :: GsmMsType_e
// DESCRIPTION :: Identifies the call type in MS
// **/
typedef enum
{
    GSM_MS_MO,  // Call Originating MS
    GSM_MS_MT   // Call Terminating MS
} GsmMsType_e;

// /**
// ENUM        :: GsmMsIdentityType_e
// DESCRIPTION :: Specifies the type of identity used for a MS
// **/
typedef enum
{
    GSM_IMSI,
    GSM_MSISDN
} GsmMsIdentityType_e;

// /**
// ENUM        :: GsmChannelType_e
// DESCRIPTION :: Identifies the type of GSM control channel
// **/
enum
{
    GSM_CHANNEL_FCCH,
    GSM_CHANNEL_SCH,
    GSM_CHANNEL_BCCH,
    GSM_CHANNEL_RACCH,
    GSM_CHANNEL_PAGCH, // AGCH + PCH togather are called as CCCH
    GSM_CHANNEL_SACCH,
    GSM_CHANNEL_FACCH,
    GSM_CHANNEL_SDCCH, // SDCCH + SACCH + FACCH are togather called as DCCH
    GSM_CHANNEL_TCH // Full-rate
};

typedef unsigned char   GsmChannelType_e;

// /**
// CONSTANT    :: GSM_EST_CAUSE_NORMAL_CALL : 0xe0
// DESCRIPTION :: Defines GSM SPEECH CALL channel request cause
// **/
// Reference:   GSM 04.08, Section 9.1.8, Table 9.9
// Comment: Lower 4 bits for rand# should be masked by function creating
// the channel request msg.

#define GSM_EST_CAUSE_NORMAL_CALL 0xe0

// /**
// CONSTANT    :: GSM_EST_CAUSE_LOCATION_UPDATING : 0x00
// DESCRIPTION :: Defines GSM location updating cause
// **/
#define GSM_EST_CAUSE_LOCATION_UPDATING 0x00

// /**
// CONSTANT    :: GSM_EST_CAUSE_ANSWER_TO_PAGING_TCH_CHANNEL : 0x40
// DESCRIPTION :: Defines establishment cause to paging channel request
// **/
// Reference:  GSM 04.08, Section 9.1.8, Table 9.9a
// Comment  :  Lower 4 bits for rand# should be masked by function
// creating the channel request msg.
#define GSM_EST_CAUSE_ANSWER_TO_PAGING_TCH_CHANNEL 0x40

// /**
// CONSTANT    :: GSM_PAGING_MODE_NORMAL : 0x1
// DESCRIPTION :: Identifies the GSM Paging request type 1
// **/
// See GSM Paging Request Type 1, GSM 04.08, sec
#define GSM_PAGING_MODE_NORMAL  0x1

// /**
// CONSTANT    :: GSM_PROGRESS_INDICATOR_END_TO_END_PLMN_ISDN_CALL : 0x32
// DESCRIPTION :: Specifies the end to end call progress indicator
// **/
// See GSM 04.08, Section 10.5.4.21 & Call Proceeding Msg (9.3.3)
#define GSM_PROGRESS_INDICATOR_END_TO_END_PLMN_ISDN_CALL 0x32

// /**
// CONSTANT    :: GSM_LOCATION_USER : 0x0
// DESCRIPTION :: Defines the value for MS originated Disconnect Message
// **/
// May refer to GSM 04.08, Section 10.5.4.11 'Cause' sub-element: location
// Following value is used in the MS originated Disconnect Message
#define GSM_LOCATION_USER   0x0

// /**
// CONSTANT    :: GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER : 0x02
// DESCRIPTION :: Defines the local user of GSM public network
// **/
#define GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER    0x02

// /**
// CONSTANT    :: GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING : 0x10
// DESCRIPTION :: Defines cause value for call clearing: normal
// **/
// May refer GSM 04.08, Section 10.5.4.11 'Cause' sub-element: cause value
// Cause number 16 in Table 10.86
#define GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING    0x10

// /**
// CONSTANT    :: GSM_CAUSE_CALL_COMPLETION_FAILED : 0x11
// DESCRIPTION :: Defines cause value for call clearing: due to radio link failure
// **/
#define GSM_CAUSE_CALL_COMPLETION_FAILED        0x11


// /**
// CONSTANT    :: GSM_CAUSE_VALUE_USER_NOT_RESPONDING  : 0x12
// DESCRIPTION :: Defines cause value for call clearing: user not responding
// **/
// May refer GSM 04.08, Section 10.5.4.11 'Cause' sub-element: cause value
// Cause number 18 in Table 10.86
#define GSM_CAUSE_VALUE_USER_NOT_RESPONDING     0x12


// /**
// CONSTANT    :: GSM_CAUSE_VALUE_USER_ALERTING_NO_ANSWER : 0x13
// DESCRIPTION :: Defines cause value for call clearing: no response to alert
// **/
// May refer GSM 04.08, Section 10.5.4.11 'Cause' sub-element: cause value
// Cause number 19 in Table 10.86
#define GSM_CAUSE_VALUE_USER_ALERTING_NO_ANSWER     0x13


// /**
// CONSTANT    :: GSM_CAUSE_VALUE_CALL_REJECTED : 0x15
// DESCRIPTION :: Defines cause value for call rejection
// **/
// May refer GSM 04.08, Section 10.5.4.11 'Cause' sub-element: cause value
// Cause number 21 in Table 10.86
#define GSM_CAUSE_VALUE_CALL_REJECTED   0x15


// /**
// CONSTANT    :: GSM_CAUSE_VALUE_NORMAL_UNSPECIFIED : 0x1F
// DESCRIPTION :: Defines cause value for call rejection
// **/
// May refer GSM 04.08, Section 10.5.4.11 'Cause' sub-element: cause value
// Cause number 31 in Table 10.86
#define GSM_CAUSE_VALUE_NORMAL_UNSPECIFIED  0x1F

// /**
// CONSTANT    :: GSM_CAUSE_VALUE_RECOVERY_ON_TIMER_EXPIRTY : 0x66
// DESCRIPTION :: Defines cause value for recovery on timer expiration
// **/
// May refer GSM 04.08, Section 10.5.4.11 'Cause' sub-element: cause value
// Cause number 102 in Table 10.86
#define GSM_CAUSE_VALUE_RECOVERY_ON_TIMER_EXPIRY    0x66


// /**
// CONSTANT    :: GSM_RR_CAUSE_NORMAL_EVENT : 0x0
// DESCRIPTION :: Specifies the normal RR Cause
// **/
// reference GSM 04.08, Section 10.5.2.31 RR Cause
#define GSM_RR_CAUSE_NORMAL_EVENT   0x0

// /**
// ENUM        :: GsmProtocolDiscriminator_e
// DESCRIPTION :: Identifies the protocol discriminator
// **/
// REFERENCE: GSM 04.08, sec 10.2
typedef enum GsmProtocolDiscriminator_e
{
    GSM_PD_CC   = 0x03,   // Call Control
    GSM_PD_MM   = 0x05,   // Mobility Management
    GSM_PD_RR   = 0x06    // Radio Resource Management
}GsmProtocolDiscriminator_e;


// /**
// ENUM        :: GsmCCMessageType_e
// DESCRIPTION :: Idetifies the Call control related message types
// **/
// Reference GSM 04.08, Section 10.4, Table 10.1
// Each message type is unique within its Protocol Discriminator grouping
// TODO: For bit 7, see above referenced section. Msgs from MS,
// bit 7 is send sequence number. Msgs from Network, bit 7 is 0.
typedef enum
{
    // Call Establishment Messages: 0x0_
    GSM_CC_MESSAGE_TYPE_ALERTING            = 0x01,
    GSM_CC_MESSAGE_TYPE_CALL_CONFIRMED      = 0x08,
    GSM_CC_MESSAGE_TYPE_CALL_PROCEEDING     = 0x02,
    GSM_CC_MESSAGE_TYPE_CONNECT             = 0x07,
    GSM_CC_MESSAGE_TYPE_CONNECT_ACKNOWLEDGE = 0x0f,
    GSM_CC_MESSAGE_TYPE_SETUP               = 0x05,

    // Call Information Phase Messages: 0x1_

    // Call Clearing Messages: 0x2_
    GSM_CC_MESSAGE_TYPE_DISCONNECT          = 0x25,
    GSM_CC_MESSAGE_TYPE_RELEASE_COMPLETE    = 0x2a,
    GSM_CC_MESSAGE_TYPE_RELEASE             = 0x2d
} GsmCCMessageType_e;


// /**
// ENUM        :: GsmMMMessageType_e
// DESCRIPTION :: Identifies Mobility management related messages types
// **/
typedef enum
{
    // Mobility Management related msgs
    // Registration Messages
    GSM_MM_MESSAGE_TYPE_IMSI_DETACH_INDICATION      = 0X01,
    GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_ACCEPT      = 0x02,
    GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_REJECT      = 0x04,
    GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_REQUEST     = 0x08,

    // Security Messages

    // Connection Management Messages
    GSM_MM_MESSAGE_TYPE_CM_SERVICE_ACCEPT           = 0x21,
    GSM_MM_MESSAGE_TYPE_CM_SERVICE_REJECT           = 0x22,
    GSM_MM_MESSAGE_TYPE_CM_SERVICE_ABORT            = 0x23,
    GSM_MM_MESSAGE_TYPE_CM_SERVICE_REQUEST          = 0x24,
    GSM_MM_MESSAGE_TYPE_CM_RE_ESTABLISHMENT_REQUEST = 0x28,
    GSM_MM_MESSAGE_TYPE_CM_ABORT                    = 0x29,

    // Miscellaneous Messages
    GSM_MESSAGE_TYPE_MM_STATUS                      = 0x31
} GsmMMMessageType_e;


// /**
// ENUM        :: GsmRRMessageType_e
// DESCRIPTION :: Identifies the Radio Resource management message types
// **/
typedef enum
{
    // Radio Resource Management Message Types

    // Paging messages (0x2-)
    GSM_RR_MESSAGE_TYPE_PAGING_REQUEST_TYPE1        = 0x21,
    GSM_RR_MESSAGE_TYPE_PAGING_RESPONSE             = 0x27,

    // Channel establishment messages (0x3-)
    GSM_RR_MESSAGE_TYPE_IMMEDIATE_ASSIGNMENT        = 0x3f,

    // System information messages (0x1-)
    GSM_RR_MESSAGE_TYPE_MEASUREMENT_REPORT          = 0x15,
    GSM_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE2    = 0x1a,
    GSM_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE3    = 0x1b,
    GSM_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE5    = 0x1d,

    // Handover (0x2-). RI - Radio Interface
    GSM_RR_MESSAGE_TYPE_RI_HANDOVER_COMMAND         = 0x2b,
    GSM_RR_MESSAGE_TYPE_RI_HANDOVER_COMPLETE        = 0x2c,

    // Channel Release (0x0-)
    GSM_RR_MESSAGE_TYPE_CHANNEL_RELEASE             = 0x0d
} GsmRRMessageType_e;

// /**
// CONSTANT    :: GSM_CM_SERVICE_REQUEST_TYPE_MO_CALL : 0x01
// DESCRIPTION :: Defines  CM Service Type :ref GSM 04.08, sec 10.5.3.3
// **/
#define GSM_CM_SERVICE_REQUEST_TYPE_MO_CALL  0x01

// /**
// ENUM        :: GsmAInterfaceMessageType_e
// DESCRIPTION :: Identifies GSM A interface (BSC-MSC) message types
// **/
// Reference: GSM 08.08, Sec 3.2.2.1 Non-DTAP messages.
// Used to set first byte of msg.Should not match any protocol
// discriminator values.
typedef enum
{
    // Assignment Messages
    GSM_ASSIGNMENT_REQUEST          = 0x01,
    GSM_ASSIGNMENT_COMPLETE         = 0x02,
    GSM_ASSIGNMENT_FAILURE          = 0x03,

    // Handover Messages
    GSM_HANDOVER_REQUEST            = 0X10,
    GSM_HANDOVER_REQUIRED           = 0x11,
    GSM_HANDOVER_REQUEST_ACK        = 0x12,
    GSM_HANDOVER_COMMAND            = 0x13,
    GSM_HANDOVER_COMPLETE           = 0x14,
    GSM_HANDOVER_FAILURE            = 0x16,
    GSM_HANDOVER_PERFORMED          = 0x17,
    GSM_HANDOVER_CANDIDATE_ENQUIRE  = 0x18,
    GSM_HANDOVER_CANDIDATE_RESPONSE = 0x19,
    GSM_HANDOVER_REQUIRED_REJECT    = 0x1a,
    GSM_HANDOVER_DETECT             = 0x1b,

    // Release Messages
    GSM_CLEAR_COMMAND               = 0x20,
    GSM_CLEAR_COMPLETE              = 0x21,
    GSM_CLEAR_REQUEST               = 0x22,
    GSM_CALL_CLEAR_COMMAND          = 0x23,

    // Radio Resource Messages
    GSM_PAGING                      = 0x52,
    GSM_COMPLETE_LAYER3_INFORMATION = 0x57
} GsmAInterfaceMessageType_e;

// /**
// ENUM        :: GsmAInterfaceCause_e
// DESCRIPTION :: Describes the cause of GsmAInterface message
// **/
// Reference: GSM 08.08 Section 3.2.2.5
typedef enum
{
    // Normal Event
    GSM_CAUSE_RADIO_INTERFACE_FAILURE                   = 0x01,
    GSM_CAUSE_UPLINK_QUALITY                            = 0x02,
    GSM_CAUSE_UPLINK_STRENGTH                           = 0x03,
    GSM_CAUSE_DOWNLINK_QUALITY                          = 0x04,
    GSM_CAUSE_DOWNLINK_STRENGTH                         = 0x05,
    GSM_CAUSE_DISTANCE                                  = 0x06,
    GSM_CAUSE_O_AND_M_INTERVENTION                      = 0x07,
    GSM_CAUSE_RESP_TO_MSC_INVOCATION                    = 0x08,
    GSM_CAUSE_CALL_CONTROL                              = 0x09,
    GSM_CAUSE_RADIO_INTERFACE_FAILURE_REVERT_TO_OLD_CH  = 0x0a,
    GSM_CAUSE_HANDOVER_SUCCESSFULL                      = 0x0b
} GsmAInterfaceCause_e;


// Additional Message Types

// /**
// ENUM        :: GsmSccpMessageType_e
// DESCRIPTION :: Defines type of Gsm sccp message
// **/
typedef enum
{
    GSM_SCCP_MSG_TYPE_CONNECT_REQUEST       = 1,
    GSM_SCCP_MSG_TYPE_CONNECT_CONFIRM       = 2,
    GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE  = 3,
    GSM_SCCP_MSG_TYPE_CONNECTIONLESS
} GsmSccpMessageType_e;

// /**
// ENUM        :: GsmTrafficMessageType_e
// DESCRIPTION :: Used for setting up & conveying traffic messages
// between the BS & MSC
// **/
typedef enum
{
    GSM_TRAFFIC_MESSAGE_TYPE_CONNECT_REQUEST    = 10,
    GSM_TRAFFIC_MESSAGE_TYPE_CONNECT_CONFIRM    = 11,
    GSM_TRAFFIC_MESSAGE_TYPE_TRAFFIC            = 12
} GsmTrafficMessageType_e;


// /**
// ENUM        :: GsmControlChannelDescription
// DESCRIPTION :: Provides a variety of information about a cell.
// **/
typedef struct struct_gsm_control_channel_description_str
{
    // ATT
    // BS-AG-BLKS-RES
    // CCCH-CONF
    // BS-PA-MFRMS
    unsigned char   T3212; // Timeout value
} GsmControlChannelDescription;

// This structure will used to store the node specific parameters
// These parameters will be share between multiple layer
typedef struct struct_gsm_node_specific_parameter
{
    uddtLAC     lac;
    uddtCellId  cellIdentity;
} GSMNodeParameters;

#define returnGSMNodeLAC(node)      (node->gsmNodeParameters->lac)
#define returnGSMNodeCellId(node)   (node->gsmNodeParameters->cellIdentity)
#define setGSMNodeLAC(node, val)    (node->gsmNodeParameters->lac = val)
#define setGSMNodeCellId(node, val) (node->gsmNodeParameters->cellIdentity = val)

// Reference: GSM 05.08 Section 8.4
// Indexed by [assignedSlotNumber][iteration: 0-3]

// /**
// CONSTANT    :: GsmSacchMsgFrame
// DESCRIPTION :: Assigned slot Number in frame
// **/
extern const int    GsmSacchMsgFrame[8][4];

// /**
// CONSTANT    :: GsmIdleTchFrame
// DESCRIPTION :: Assigned slot Number in frame
// **/
extern const int    GsmIdleTchFrame[8][4];

// /**
// CONSTANT    :: GsmMeasRepPeriod
// DESCRIPTION :: Assigned slot Number in frame
// **/
extern const int    GsmMeasRepPeriod[8][2];


#endif
