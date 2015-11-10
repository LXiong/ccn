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
// software, hardware, product or service

#include "cellular_gsm.h"
#include "mac_gsm.h"

#ifndef GSM_LAYER3_H
#define GSM_LAYER3_H


// Constants
// 'A' interface cause values. GSM 08.08
typedef enum
{
    // Normal Event
    GSM_AIF_CAUSE_RADIO_IF_MESSAGE_FAILURE                  = 0x0,
    GSM_AIF_CAUSE_RADIO_IF_FAILURE                          = 0x1,
    GSM_AIF_CAUSE_UPLINK_QUALITY                            = 0x2,
    GSM_AIF_CAUSE_UPLINK_STRENGTH                           = 0x3,
    GSM_AIF_CAUSE_DOWNLINK_QUALITY                          = 0x4,
    GSM_AIF_CAUSE_DOWNLINK_STRENGTH                         = 0x5,
    GSM_AIF_CAUSE_DISTANCE                                  = 0x6,
    GSM_AIF_CAUSE_OM_INTERVENTION                           = 0x7,
    GSM_AIF_CAUSE_RESPONSE_TO_MSC_INVOCATION                = 0x8,
    GSM_AIF_CAUSE_CALL_CONTROL                              = 0x9,
    GSM_AIF_CAUSE_RADIO_IF_FAILURE_REVERT_OLD_TO_CHANNEL    = 0xA,
    GSM_AIF_CAUSE_HANDOVER_SUCCESSFUL                       = 0xB,
    GSM_AIF_CAUSE_BETTER_CELL                               = 0xC,
    GSM_AIF_CAUSE_DIRECTED_RETRY                            = 0xD,
    GSM_AIF_CAUSE_JOINED_GROUP_CALL_CHANNEL                 = 0xE,
    GSM_AIF_CAUSE_TRAFFIC                                   = 0xF,

    // Resource unavailable
    GSM_AIF_CAUSE_EQUIPMENT_FAILURE                         = 0x20,
    GSM_AIF_CAUSE_NO_RADIO_RESOURCE_AVAILABLE               = 0x21,
    GSM_AIF_CAUSE_REQUESTED_TERRESTRIAL_RESOURCE_UNAVIL     = 0x22,
    GSM_AIF_CAUSE_CCCH_OVERLOADED                           = 0x23,
    GSM_AIF_CAUSE_PROCESSOR_OVERHEAD                        = 0x24,
    GSM_AIF_CAUSE_BSS_NOT_EQUIPPED                          = 0x25,
    GSM_AIF_CAUSE_MS_NOT_EQUIPPED                           = 0x26,
    GSM_AIF_CAUSE_INVALID_CELL                              = 0x27,
    GSM_AIF_CAUSE_TRAFFIC_LOAD                              = 0x28,
    GSM_AIF_CAUSE_PREEMPTION                                = 0x29,

    //
    GSM_AIF_CAUSE_REQUESTED_TRANSCODING_UNAVAIL             = 0x30,
    GSM_AIF_CAUSE_CIRCUIT_POOL_MISMATCH                     = 0x31,
    GSM_AIF_CAUSE_SWITCH_CIRCUIT_POOL                       = 0x32,
    GSM_AIF_CAUSE_REQUESTED_SPPECH_VERSION_UNAVAIL          = 0x33,
    GSM_AIF_CAUSE_LSA_NOT_ALLOWED                           = 0x34,

    //
    GSM_AIF_CAUSE_CIPHERING_ALGORITHM_NOT_SUPPORTED         = 0x40,

    //
    GSM_AIF_CAUSE_TERRESTRIAL_CIRCUIT_ALREADY_ALLOCATED     = 0x50,
    GSM_AIF_CAUSE_INVALID_MESSAGE_CONTENTS                  = 0x51,
    GSM_AIF_CAUSE_INFORMATION_ELEMENT_FIELD_MISSING         = 0x52,
    GSM_AIF_CAUSE_INCORRECT_VALUE                           = 0x53,
    GSM_AIF_CAUSE_UNKNOWN_MESSAGE_TYPE                      = 0x54,
    GSM_AIF_CAUSE_UNKNOWN_INFORMATION_ELEMENT               = 0x55,

    //
    GSM_AIF_CAUSE_PROTOCOL_ERROR_BSS_MSC                    = 0x60
} GsmAIfCause_e;

/* Name:        GsmRrState_e
 * Purpose:     Radio Resource Management States - Network side
 *              entity.
 */
typedef enum
{
    //GSM_NW_RR_STATE_NULL = 0,
    GSM_NW_RR_STATE_IDLE    = 0,   // No dedicated channel established
    GSM_NW_RR_STATE_CON_PEND,   // Connection pending
    GSM_NW_RR_STATE_DT_1,   // Data Transfer 1, dedicated channel established
    GSM_NW_RR_STATE_DT_2,   // Data Transfer 2, dedicated channel established
    //                  ciphering mode set
} GsmNwRrState_e;


/* Name:        GsmMsRrState_e
 * Purpose:     Radio Resource Management states - MS side
 */
typedef enum
{
    GSM_MS_RR_STATE_NULL    = 0,
    GSM_MS_RR_STATE_IDLE,
    GSM_MS_RR_STATE_CON_PEND,
    GSM_MS_RR_STATE_DEDICATED
} GsmMsRrState_e;


typedef enum
{
    GSM_MS_MM_STATE_NULL                                        = 0,
    GSM_MS_MM_STATE_LOCATION_UPDATING_INITIATED                 = 3,
    GSM_MS_MM_STATE_WAIT_FOR_OUTGOING_MM_CONNECTION             = 5,
    GSM_MS_MM_STATE_MM_CONNECTION_ACTIVE                        = 6,
    GSM_MS_MM_STATE_IMSI_DETACH_INITIATED                       = 7,
    GSM_MS_MM_STATE_WAIT_FOR_NETWORK_COMMAND                    = 9,
    GSM_MS_MM_STATE_LOCATION_UPDATE_REJECTED                    = 10,
    GSM_MS_MM_STATE_WAIT_FOR_RR_CONNECTION_LOCATION_UPDATE      = 13,
    GSM_MS_MM_STATE_WAIT_FOR_RR_CONNECTION_MM_CONN              = 14,
    GSM_MS_MM_STATE_WAIT_FOR_RR_CONNECTION_IMSI_DETACH          = 15,
    GSM_MS_MM_STATE_WAIT_FOR_REESTABLISH                        = 17,
    GSM_MS_MM_STATE_WAIT_FOR_RR_ACTIVE                          = 18,
    GSM_MS_MM_STATE_MM_IDLE                                     = 19,
    GSM_MS_MM_STATE_WAIT_FOR_ADDITIONAL_OUTGOING_MM_CONNECTION  = 20
} GsmMsMmState_e;


typedef enum
{
    GSM_MS_MM_IDLE_NORMAL_SERVICE               = 191,  // 19.1
    GSM_MS_MM_IDLE_ATTEMPTING_TO_UPDATE         = 192,
    GSM_MS_MM_IDLE_LIMITED_SERVICE              = 193,
    GSM_MS_MM_IDLE_NO_IMSI                      = 194,
    GSM_MS_MM_IDLE_NO_CELL_AVAILABLE            = 195,
    GSM_MS_MM_IDLE_LOCATION_UPDATE_NEEDED       = 196,
    GSM_MS_MM_IDLE_PLMN_SEARCH                  = 197,
    GSM_MS_MM_IDLE_PLMN_SEARCH_NORMAL_SERVICE   = 198
} GsmMsMmIdleSubState_e;


typedef enum
{
    GSM_MS_MM_UPDATE_STATUS_U1_UPDATED              = 1,
    GSM_MS_MM_UPDATE_STATUS_U2_NOT_UPDATED          = 2,
    GSM_MS_MM_UPDATE_STATUS_U3_ROAMING_NOT_ALLOWED  = 3
} GsmMsMmUpdateStatus_e;


typedef enum
{
    GSM_NW_MM_STATE_IDLE                                = 1,
    GSM_NW_MM_STATE_WAIT_FOR_RR_CONNECTION              = 2,
    GSM_NW_MM_STATE_MM_CONNECTION_ACTIVE                = 3,
    GSM_NW_MM_STATE_IDENTIFICATION_INITIATED            = 4,
    GSM_NW_MM_STATE_AUTHENTICATION_INITIATED            = 5,
    GSM_NW_MM_STATE_TMSI_REALLOCATION_INITIATED         = 6,
    GSM_NW_MM_STATE_CIPHERING_MODE_INITIATED            = 7,
    GSM_NW_MM_STATE_WAIT_FOR_MOBILE_ORIG_MM_CONNECTION  = 8,
    GSM_NW_MM_STATE_WAIT_FOR_REESTABLISHMENT            = 9
} GsmNwMmState_e;


/* Name:        GsmMsCcState_e
 * Purpose:     Call Control states of a GSM MS
 * Reference:   GSM 04.08 Section 5.1.2.1, Figure 5.1a
 * Comment:     MO => In Call Originating MS
 *              MT => In Call Terminating MS
 */
typedef enum
{
    GSM_MS_CC_STATE_NULL                        = 0,    // U0
    GSM_MS_CC_STATE_MM_CONNECTION_PENDING       = 1,    // U0.1 MO
    GSM_MS_CC_STATE_CALL_INITIATED              = 2,    // U1   MO
    GSM_MS_CC_STATE_MOBILE_ORIG_CALL_PROCEEDING = 3,    // U3   MO
    GSM_MS_CC_STATE_CALL_DELIVERED              = 4,    // U4   MO
    GSM_MS_CC_STATE_CALL_PRESENT                = 6,    // U6   MT
    GSM_MS_CC_STATE_CALL_RECEIVED               = 7,    // U7   MT
    GSM_MS_CC_STATE_CONNECT_REQUEST             = 8,    // U8   MT
    GSM_MS_CC_STATE_MOBILE_TERM_CALL_CONFIRMED  = 9,    // U9   MT
    GSM_MS_CC_STATE_ACTIVE                      = 10,   // U10  Both
    GSM_MS_CC_STATE_DISCONNECT_REQUEST          = 11,   // U11  Both
    GSM_MS_CC_STATE_DISCONNECT_INDICATION       = 12,   // U12  From Nw
    GSM_MS_CC_STATE_RELEASE_REQUEST             = 13,   // U19  MO
    GSM_MS_CC_STATE_MOBILE_ORIG_MODIFY          = 14,   // U26
    GSM_MS_CC_STATE_MOBILE_TERM_MODIFY          = 15    // U27
} GsmMsCcState_e;


/* Name:        GsmNwCcState_e
 * Purpose:     Call Control states of the Network
 * Reference:   GSM 04.08 Section 5.1.2.2 Figure 5.1b
 * Comment:
 */
typedef enum
{
    GSM_NW_CC_STATE_NULL                        = 0,    // N0
    GSM_NW_CC_MM_CONNECTION_PENDING             = 1,    // N0.1 - MT
    GSM_NW_CC_STATE_CALL_INITIATED              = 2,    // N1 - MO
    GSM_NW_CC_STATE_MOBILE_ORIG_CALL_PROCEEDING = 3,    // N3 - MO
    GSM_NW_CC_STATE_CALL_DELIVERED              = 4,    // N4 - MO
    GSM_NW_CC_STATE_CALL_PRESENT                = 6,    // N6 - MT
    GSM_NW_CC_STATE_CALL_RECEIVED               = 7,    // N7 - MT
    GSM_NW_CC_STATE_CONNECT_REQUEST             = 8,    // N8 - MT
    GSM_NW_CC_STATE_MOBILE_TERM_CALL_CONFIRMED  = 9,    // N9 - MT
    GSM_NW_CC_STATE_ACTIVE                      = 10,   // N10 - MO & MT
    GSM_NW_CC_STATE_DISCONNECT_INDICATION       = 12,   // N12 - From Nw
    GSM_NW_CC_STATE_RELEASE_REQUEST             = 19,   // N19 - By MS
    GSM_NW_CC_STATE_MOBILE_ORIG_MODIFY          = 26,   // N26 - MO
    GSM_NW_CC_STATE_MOBILE_TERM_MODIFY          = 27,   // N27 - MT
    GSM_NW_CC_STATE_CONNECT_INDICATION          = 28    // N28 - MO
} GsmNwCcState_e;


typedef enum
{
    GSM_NORMAL_LOCATION_UPDATING    = 0,
    GSM_PERIODIC_LOCATION_UPDATING  = 1,
    GSM_IMSI_ATTACH                 = 2,
} GsmLocationUpdatingType_e;


typedef struct struct_gsm_ms_layer3_stats_str
{
    int pktsRcvdTraffic;
    int msgsRcvdControl;
    int pktsSentTraffic;
    int msgsSentControl;

    int msgsRcvdUnknownControl;

    // Layer 3 message stats

    // RR Statistics
    int msgsRcvdRrLayer;

    int sysInfoType2Rcvd;
    int sysInfoType3Rcvd;

    int channelRequestSent;
    int immediateAssignmentRcvd;
    int channelReleaseRcvd;
    int numChannelRequestAttemptsFailed;

    int pagingRequestType1Rcvd;
    int pagingResponseSent;

    int riHandoverCommandRcvd;

    // MM Statistics
    int msgsRcvdMmLayer;
    int mmConnectionFailures;

    int locationUpdateRequestSent;
    int locationUpdateAcceptRcvd;
    int cmServiceRequestSent;
    int cmServiceAcceptRcvd;

    // CC messages
    int msgsRcvdCcLayer;

    int setupSent; // MO
    int setupRcvd; // MT

    int alertingSent;
    int alertingRcvd;

    int callProceedingRcvd;

    int connectSent;
    int connectRcvd;

    int connectAckSent;
    int connectAckRcvd;

    int releaseRcvd;
    int releaseCompleteSent;

    int releaseSent;
    int releaseCompleteRcvd;

    int disconnectSent;
    int disconnectRcvd;


    int locationUpdateFailures;
    int numCallsInitiated;      // Calls Initiated by this MS
    int numCallsReceived;       // Calls Received
    int numCallsConnected;      // Calls connected (received & initiated)
    int numCallsCompleted;      // Connected calls released gracefully
    //int     numCallsDropped;        // Calls terminated due to errors

    // Timer related stats: set, expired...

    int errorMsgs;
} GsmLayer3MsStats;


typedef struct struct_gsm_message_list_item_str
{
    Message*                                msg;
    short                                   servicePrimitive;

    uddtSlotNumber                          slotNumber;
    uddtChannelIndex                        channelIndex;
    GsmChannelType_e                        channelType;

    struct struct_gsm_message_list_item_str*
    nextListItem;

    struct struct_gsm_message_list_item_str*
    endItem;
} MessageListItem;


typedef struct struct_gsm_layer3_ms_info_str
{
    BOOL                        isBcchDecoded;
    BOOL                        isSysInfoType2Decoded;
    BOOL                        isSysInfoType3Decoded;

    clocktype                   bcchDecodedAt;
    clocktype                   sysInfoType2DecodedAt;
    clocktype                   sysInfoType3DecodedAt;
    uddtChannelIndex            downLinkControlChannelIndex;

    // Values received from selected cell

    int                         txInteger;
    int                         maxReTrans;

    unsigned char               gsmImsi[GSM_MAX_IMSI_LENGTH];   // 10 digits
    unsigned char               gsmMsIsdn[GSM_MAX_MSISDN_LENGTH];   // 10 digits

    int                         interfaceIndex;
    // Layer 3 Uplink msgs to be sent to the MAC layer
    int                         numMsgsToSendToMac[GSM_SLOTS_PER_FRAME];
    MessageListItem*            msgListToMac[GSM_SLOTS_PER_FRAME];

    // Timers
    Message*                    timerT303Msg;      // Call Present Timer
    Message*                    timerT305Msg;      // Call Disconnect Timer
    Message*                    timerT308Msg;      // Call Release Timer
    Message*                    timerT310Msg;      // Call Proceeding Timer
    Message*                    timerT313Msg;      // Connect Request Timer

    Message*                    timerT3210Msg;     // Location Update Timer
    Message*                    timerT3211Msg;     // Restart Location Update on failure
    Message*                    timerT3212Msg;     // Periodic Update Timer
    Message*                    timerT3230Msg;     // CM Service Request Timer
    Message*                    timerT3240Msg;
    Message*                    timerT3213Msg;
    unsigned char               numtimerT3213Attempts;
    int                         timerT308Expirations;

    // Channel Request information
    unsigned char               rand_disc;
    unsigned char               est_cause;
    int                         numChannelRequestAttempts;
    BOOL                        isRequestingChannel;
    int                         slotsToWait;
    Message*                    channelRequestMsg;
    Message*                    channelRequestTimer;

    // Location Update information
    GsmLocationUpdatingType_e   locationUpdatingType;
    //clocktype                    t3212TimeoutValue;
    short                       locationUpdateAttemptCounter;

    // Call related information
    BOOL                        isCallOriginator;
    unsigned char               disconnectCause[2];

    // Dest MS's MSISDN number (phone number)
    unsigned char               destMsIsdn[GSM_MAX_MSISDN_LENGTH];
    clocktype                   callStartTime;
    clocktype                   callEndTime;

    // Handover related information
    unsigned char               hoReference;
    // Bug # 3436 fix - start
    BOOL isHandoverInProgress;
    // Bug # 3436 fix - end
    // hoStartTime... see GsmRIHOCommandMsg

    // Neighbouring cell/BS information
    uddtNumNeighBcchChannels    numNeighBcchChannels;
    short
    downLinkNeighBcchChannels[GSM_MAX_NUM_CONTROL_CHANNELS_PER_MS];

    // Assigned Resources
    BOOL                        isDedicatedChannelAssigned;
    BOOL                        isAssignedChannelForTraffic;    // default: FALSE => Signalling

    GsmChannelType_e            assignedChannelType;
    uddtSlotNumber              assignedSlotNumber;
    uddtChannelIndex            assignedChannelIndex;   // Uplink Channel Index

    Message*                    trafficTimerMsg;


    // change the data types
    // MS state
    GsmMsMmState_e              mmState;
    GsmMsRrState_e              rrState;
    GsmMsCcState_e              ccState;
    GsmMsMmIdleSubState_e       mmIdleSubState;
    GsmMsMmUpdateStatus_e       mmUpdateStatus;

    // Transactions
    //BOOL isTransactionInProcess;
    //int transactionIdentifier;
    int                         numTransactions;

    GsmLayer3MsStats            stats;
} GsmLayer3MsInfo;

typedef struct struct_paging_msg_str    GsmPagingMsg;

// slotNumber of the MS is used to index the array of
// struct's of this type in the channelDesc member of
// GsmLayer3BsInfo.
typedef struct struct_gsm_slot_assignment_str
{
    BOOL                inUse;
    BOOL                isSeizedByMs;       // Related to Timer T3101
    GsmChannelType_e    channelType;
    uddtSlotNumber      slotNumber;         // Self-reference
    short               channelPairIndex;   // To index BsInfo.channelDesc

    // Dividing downlink channelIndex by 2 gives channelPair/ARFCN#
    uddtChannelIndex    dlChannelIndex; // Downlink
    uddtChannelIndex    ulChannelIndex; // Uplink (downlink + 1)
    BOOL                isUsedForTraffic;   // default: FALSE => signalling

    BOOL                isHandoverBeingAttempted;
    BOOL                isHandoverInProgress;
    unsigned char       hoReference;


    // IMSI of MS using this slot/channel
    char                msImsi[GSM_MAX_IMSI_LENGTH];

    // Connection state in network
    unsigned char       mmState;
    unsigned char       rrState;
    clocktype           startTime;

    // SCCP related information
    int                 sccpConnectionId;

    // traffic connection with MSC id
    int                 networkTrafficConnectionId;

    // Handover related measurements

    // Set when BS receives a SACCH msg and retrieves its own
    // measurements on TCH in to the callback func gsmBsProcessMeasReport
    BOOL                isMeasReportReceived;
    clocktype           lastSACCHupdate;

    int                 measDlSampleNum;    // modulo 32
    int                 rxLevDl[GSM_NUM_MEAS_SAMPLES];
    int                 rxQualDl[GSM_NUM_MEAS_SAMPLES];

    int                 measUlSampleNum;
    int                 rxLevUl[GSM_NUM_MEAS_SAMPLES];
    int                 rxQualUl[GSM_NUM_MEAS_SAMPLES];

    // Same first index used to access the following arrays.
    // This index corresponds to the bsInfo.neighBcchChannels array
    int                 neighCellSampleNum[GSM_NUM_NEIGHBOUR_CELLS];
    int
    rxLevBcchDl[GSM_NUM_NEIGHBOUR_CELLS][GSM_NUM_MEAS_SAMPLES];

    // Averaged values
    BOOL                atLeastHreqAvgValues;
    int                 numAvgValues;   // index to the following arrays
    int                 avgRxLevUl[GSM_HREQAVE];
    int                 avgRxLevDl[GSM_HREQAVE];

    int                 avgRxQualUl[GSM_HREQAVE];
    int                 avgRxQualDl[GSM_HREQAVE];

    int                 numAvgBcchValues[GSM_NUM_NEIGHBOUR_CELLS];
    BOOL                atLeastHreqAvgBcchValues[GSM_NUM_NEIGHBOUR_CELLS];
    int                 avgRxLevBcchDl[GSM_NUM_NEIGHBOUR_CELLS][GSM_HREQAVE];

    int                 count;
}GsmSlotUsageInfo;


// The ARFCN number indicates 2 physical channels
// one each: downlink and uplink(next consecutive higher number).
typedef struct struct_gsm_channel_desc_str
{
    BOOL                inUse; // TRUE if any one of the 8 slots is in use

    /*
    short   dlChannelIndex; // Downlink
    short   ulChannelIndex; // Uplink (downlink + 1)
    */
    GsmSlotUsageInfo    slotUsageInfo[GSM_SLOTS_PER_FRAME];
}GsmChannelDescription;


typedef struct struct_gsm_sccp_routing_label_str
{
    char    sourcePointCode[3];
    char    destPointCode[3];

    // signaling link selection for load distribution
} GsmSccpRoutingLabel;


typedef struct struct_gsm_sccp_connection_description_str
{
    BOOL                inUse;
    clocktype           useAfter;
    unsigned char       connectionState; // Not used yet.
    GsmSlotUsageInfo*   msSlotInfo;

    // Use channel pair#, add dnLinkChannelIndex ???
} GsmSccpConnectionDescription;


typedef struct struct_gsm_traffic_connection_description_str
{
    BOOL                inUse;
    GsmSlotUsageInfo*   msSlotInfo;

    //int trafficConnectionId;
} GsmTrafficConnectionDescription;


typedef struct struct_gsm_bs_statistics_str
{
    int pktsRcvdOnAirTraffic;
    int pktsSentOnAirTraffic;

    int numChannelsAssigned; // TS & frequency
    int numChannelsReleased; // TS & frequency

    int numSccpConnAssigned;
    int numSccpConnReleased;

    int numT3101Expirations;

    // Air/'Um' interface: BS-MS
    // RR
    int channelRequestRcvd;
    int channelAssignmentAttemptsFailed; // Failed to assign a channel
    int immediateAssignmentSent;
    int channelReleaseSent;

    int pagingResponseRcvd;
    int pagingRequestSent;

    int riHandoverCompleteRcvd;
    int riHandoverCommandSent;
    int handoverAccessRcvd;

    int measurementReportRcvd;


    // MM
    int locationUpdateRequestRcvd;
    int cmServiceRequestRcvd;

    // CC

    // 'A' interface: BS-MSC
    int channelReleaseRcvd;
    int completeLayer3InfoSent;

    int handoverRequiredSent;
    int handoverDetectSent;
    int handoverCompleteSent;

    int handoverRequestRcvd;
    int handoverRequestAckSent;
    int handoverCommandRcvd;

    int handoverRequiredRejectRcvd;

    int clearCommandRcvd;
    int clearCompleteSent;


    int pagingRcvd;
} GsmLayer3BsStats;


typedef struct struct_gsm_layer3_bs_info_str
{
    int                             interfaceIndex; // of GSM MAC layer
    uddtChannelIndex                controlChannelIndex;

    NodeId                          mscNodeId;
    NodeAddress                     mscNodeAddress;

    // Updated from the MAC layer on receiving a msg
    uddtFrameNumber                 frameNumber;

    // Layer 3 downlink msgs to be sent to the MAC layer
    int
    numMsgsToSendToMac[GSM_SLOTS_PER_FRAME][GSM_MAX_NUM_CHANNELS_PER_BS];
    MessageListItem*
    msgListToMac[GSM_SLOTS_PER_FRAME][GSM_MAX_NUM_CHANNELS_PER_BS];

    // RR connection information

    // Assigned Channel information
    uddtChannelIndex                channelIndexStart;
    uddtChannelIndex                channelIndexEnd;

    // Channel usage information
    short                           numChannelPairsInUse;    // Num of channel pairs in use
    short                           totalNumChannelPairs;    // upto GSM_MAX_ARFCN_PER_BS
    GsmChannelDescription           channelDesc[GSM_MAX_ARFCN_PER_BS];

    // Neighbouring BS BCCH (C0) channel information.
    // Same index is used to access the measurement results/averages in
    // the GsmSlotUsageInfo structs.
    uddtNumNeighBcchChannels        numNeighBcchChannels;

    int
    neighBcchChannels[GSM_NUM_NEIGHBOUR_CELLS];
    uddtLAC                         neighLac[GSM_NUM_NEIGHBOUR_CELLS];
    uddtCellId
    neighCellIdentity[GSM_NUM_NEIGHBOUR_CELLS];

    // SCCP connection information (BS<->MSC)
    int                             numSccpConnections;

    GsmSccpConnectionDescription    sccpConnectionDesc[GSM_MAX_MS_PER_BS];

    GsmTrafficConnectionDescription trafficConnectionDesc[GSM_MAX_MS_PER_BS];

    double                          handoverMargin;

    Message*                        timerSACCHMsg;      //
    GsmLayer3BsStats                stats;
} GsmLayer3BsInfo;


typedef struct struct_gsm_msc_call_state_str
{
    BOOL            isCallOriginator;
    unsigned char   imsi[GSM_MAX_IMSI_LENGTH];
    unsigned char   msIsdn[GSM_MAX_MSISDN_LENGTH];

    // change the data types
    GsmNwRrState_e  nwRrState;
    GsmNwMmState_e  nwMmState;
    GsmNwCcState_e  nwCcState;

    // change the data types int to unsigned char
    unsigned char   transactionIdentifier;
    int             sccpConnectionId;
    int             hoSccpConnectionId;
    int             trafficConnectionId;

    NodeId          bsNodeId;
    NodeAddress     bsNodeAddress;
    BOOL            isBsTryingHo;
    NodeId          hoTargetBsNodeId;
    NodeAddress     hoTargetBsNodeAddress;

    // Timers
    Message*        timerT301Msg;  // Call Received Timer
    Message*        timerT303Msg;  // Call Present Timer
    Message*        timerT305Msg;  // Disconnect Timer
    Message*        timerT308Msg;  // Release Timer
    Message*        timerT310Msg;  // Call Proceeding Timer
    Message*        timerT313Msg;  // Connect Timer
    Message*        timerT3103Msg; // Handover Timer
    Message*        timerUDT1Msg; // Cm Service Accept

    int             numT308Expirations;
} GsmMscCallState;


typedef struct struct_gsm_msc_call_information_str
{
    BOOL            inUse;

    int             numTransactions;

    GsmMscCallState originMs;
    GsmMscCallState termMs;

    Message*        timerT3103Msg; // Handover Timer
    Message*        timerT3113Msg; // Paging Request Timer

    int             numPagingAttempts;
    GsmPagingMsg*   pagingMsg;

    unsigned int    trafPktsFromOrigMs;
    unsigned int    trafPktsFromTermMs;
} GsmMscCallInfo;


typedef struct struct_msc_timer_data_str
{
    GsmMscCallInfo* callInfo;
    BOOL            isSetForOriginMs;
    unsigned char   cause[2];
} GsmMscTimerData;


typedef struct struct_traffic_conn_info_str
{
    //int   trafficConnectionId;    // redundant, array index is used
    BOOL            inUse;
    GsmMscCallInfo* callInfo;
} GsmTrafficConnectionInfo;


typedef struct struct_gsm_vlr_entry_str
{
    BOOL            inUse;

    unsigned char   imsi[GSM_MAX_IMSI_LENGTH];
    unsigned char   msIsdn[GSM_MAX_MSISDN_LENGTH];

    // location information of the MS
    NodeId          bsNodeId;
    NodeAddress     bsNodeAddress;

    uddtCellId      cellIdentity;   /* Ref: GSM 03.03 */
    uddtLAC         lac;            /* Location Area Code, Ref: GSM 03.03 */

    // Time of creation of this entry/record in the VLR
    // Class, permissions & authentication information for the MS
} GsmVlrEntry;


typedef struct struct_gsm_temp_ho_info_str
{
    int             tempHoId;
    GsmMscCallInfo* callInfo;
} GsmTempHoInfo;


typedef struct struct_gsm_msc_statistics_str
{
    int numCallRequests;
    int numCallRequestsConnected;
    int numCallsCompleted;
    int numHandovers;
    int numTrafficPkts;

    // RR
    int pagingResponseRcvd;
    int channelReleaseSent;

    // MM
    int locationUpdateRequestRcvd;
    int locationUpdateAcceptSent;
    int cmServiceRequestRcvd;
    int cmServiceAcceptSent;
    int completeLayer3InfoRcvd;

    // CC
    int setupSent;

    int setupRcvd;
    int callProceedingSent;
    int pagingSent;

    int callConfirmedRcvd;

    int alertingRcvd;
    int alertingSent;

    int connectRcvd;
    int connectSent;

    int connectAckRcvd;
    int connectAckSent;

    int disconnectRcvd;
    int disconnectSent;
    int releaseSent;

    int releaseCompleteRcvd;

    int releaseRcvd;
    int releaseCompleteSent;



    int handoverRequestAckRcvd;
    int handoverCommandSent;

    int handoverRequiredRcvd;
    int handoverRequestSent;

    int handoverDetectRcvd;
    int handoverFailureRcvd;

    int handoverCompleteRcvd;
    int clearCommandSent;

    int clearCompleteRcvd;
} GsmMscStats;


typedef struct struct_gsm_msc_info_str
{
    int                         numSccpConnections;
    int                         numTrafficConnections;

    int                         numTransactionIdentifiers;

    // BS's linked to this MSC
    int                         numBs;
    NodeId                      bsNodeId[GSM_MAX_BS_PER_MSC];
    NodeAddress                 bsNodeAddress[GSM_MAX_BS_PER_MSC];
    uddtLAC                     bsLac[GSM_MAX_BS_PER_MSC];
    uddtCellId                  bsCellIdentity[GSM_MAX_BS_PER_MSC];

    // Indexed by trafficId's
    GsmTrafficConnectionInfo    trafficConnInfo[GSM_MAX_MS_PER_MSC];


    GsmMscCallInfo              callInfo[GSM_MAX_MS_PER_MSC];

    int                         tempHoId;
    GsmTempHoInfo               tempHoInfo[GSM_MAX_MS_PER_MSC];

    // Integrated with the MSC, no separate interface.
    GsmVlrEntry                 gsmVlr[GSM_MAX_MS_PER_MSC];

    GsmMscStats                 stats;
} GsmLayer3MscInfo;


typedef struct struct_layer3_gsm_str
{
    // Must be set at initialization for every GSM node
    GsmNodeType_e       nodeType;

    int                 interfaceIndex;

    RandomSeed          randSeed;

    // Each node has only one of the following based on its type
    GsmLayer3MsInfo*    msInfo;    // GSM_MS
    GsmLayer3BsInfo*    bsInfo;    // GSM_BS
    GsmLayer3MscInfo*   mscInfo;   // GSM_MSC

    // Layer 3 msgs to be sent to the MAC layer
    // int numMsgsToSendToMac[GSM_SLOTS_PER_FRAME];
    // MessageListItem *msgListToMac[GSM_SLOTS_PER_FRAME];

    BOOL                collectStatistics;
} GsmLayer3Data;


typedef struct struct_gsm_sccp_message_header_str
{
    unsigned char       messageTypeCode; // See gsm.h:GsmSccpMessageType_e

    int                 sccpConnectionId;
    //BOOL      isMsgConnectionLess;      // SCCP Connectionless service
    NodeId              sourceNodeId;          // Sender's address/ source local ref.

    GsmSccpRoutingLabel routingLabel;

    //unsigned char    *msg;   // Message to be sent
} GsmSccpMsg;


/* Name:
 * Purpose:     For setting up & conveying traffic messages
 *              between the BS & MSC
 */
typedef struct struct_gsm_traffic_message_on_network_header_str
{
    unsigned char   messageTypeCode; // See gsm.h:GsmTrafficMessageType_e
    // See GSM 05.02, Section 3.2.2.2 Circuit Identity Code
    // which shows the usage of an E1 connection (2048 kbps, 32 timeslots)
    int             trafficConnectionId;
    int             packetSequenceNumber;
} GsmTrafficMessageOnNetworkHeader;


// NOTE ON GSM Layer 3 MESSAGE struct's:
// All Messages containing protocol discriminator and message
// type fields should declare them as the first two elements
// (unsigned char) of the struct to allow proper decoding on
// the receiving end.
//
//
// Some messages like Channel Request are exceptions.
//



typedef struct struct_gsm_generic_layer3_message_header_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
} GsmGenericLayer3MsgHdr;



/* NAME: System Information Type 3 Message
 * PURPOSE: Sent by BS over the Control Channel (BCCH) to MS
 *          giving info on RACH, location, cell identity etc.
 * REFERENCE: GSM 04.08, sec 9.1.35, Table 9.32
 * COMMENT:
 *
typedef struct struct_gsm_sys_info_type3_str
{
    char    protocolDiscriminator;
    char    messageType;
    short   cellIdentity;
    short   lac;    //Location Area Code, instead of LAI


    // The FCCH & SCH functionality is included in this
    // message by adding the following parameter.
    // Time when the next slot0 (i.e. a new frame) begins
    clocktype startTimeOfNextFrame;
    short   nextFrameNumber;    // 51 frames of control channels
                                // Should add (reduced) frame number???

    //some RACH related info. See GSM Text page 368, sec 6.3.1.1
    //short   tx_integer;  //3-50, avg. time between repetitions (rand)
    //short   max_retrans; //1, 2, 4 or 7 number of repetitions

} GsmSysInfoType3Msg;
*/

/*
 * Sent from: MS-BSS->MSC
 * Purpose  : To request establishment of a connection for a
 *            circuit switched call and other services.
 * Reference: GSM 04.08, section 9.2.9
 * Comment  :
 *
 */
typedef struct struct_gsm_cm_service_request_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   cmServiceType;  // GSM_CM_SERVICE_REQUEST_MO_CALL


    unsigned char   gsmImsi[GSM_MAX_IMSI_LENGTH];   // 10 digits
    // Other fields are ignored for now
} GsmCmServiceRequestMsg;


/*
 * Sent from:   MSC-BSS->MS
 * Purpose  :   Acknowledge receipt of a MS's call request
 *
 * Reference:   GSM 04.08, section 9.2.9
 * Comment  :
 *
 */
typedef struct struct_gsm_cm_service_accept_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;

    // Other fields are ignored for now
} GsmCmServiceAcceptMsg;


/* NAME:        Channel Request Message
 * PURPOSE:     Sent (on RACH channel) to request setup of a
 *              channel (MS->BS)
 * REFERENCE:   GSM 04.08, sec 9.1.8, Table 9.9
 * COMMENT:     Does not have the first two fields as protocol
 *              discriminator & message type respectively.
 */
typedef struct struct_gsm_channel_request_str
{
    unsigned char   est_cause;    // MO Call or paging
    unsigned char   rand_disc;    // Using requesting MS's nodeId
    // Will never cause a problem.
    // Should be changed to simulate
    // possible problems in using a rand.

    /* Should frame number be added??? */
} GsmChannelRequestMsg;


typedef struct struct_gsm_immediate_assignment_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    uddtSlotNumber  slotNumber;   // Range: 0-7, for now 1-7 till multiple
    // phy devices per mac support is added
    short           channelDescription; // ARFCN: 0-1023, Channel pair numbers
    // Value 0: 0 downlink, 1 uplink
    // Value 1: 2 downlink, 3 uplink

    // Return the channel request values back for reference
    unsigned char   est_cause; /* set to GSM_EST_CAUSE_NORMAL_CALL */
    unsigned char   rand_disc;
    unsigned char   timingAdvance;
} GsmImmediateAssignmentMsg;


typedef struct struct_gsm_mo_setup_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;
    // TODO: per std. ti is coded into 4 bits of first octet
    //       See 04.07, Section 11.2.3.1.3 on coding of ti values

    // Bearer capability information
    // and other parameters not used yet.

    unsigned char   signal; // See 04.08 sec 10.5.4.23 & 5.2.2.3.2
    // Each digit can be packed in 4 bit fields => 2 digits per byte/char
    unsigned char   calledPartyNumber[GSM_MAX_MSISDN_LENGTH];
} GsmMoSetupMsg;


/*
 * Purpose: Network to calling MS to indicate acceptance of requested call
 *          establishment.
 * Reference: GSM 04.08, sec 9.3.3
 *            GSM 03.18, sec 5.1 MO call information flow
 */
typedef struct struct_call_proceeding_msg_sgr
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    unsigned char   progressIndicator; // GSM_END_TO_END_PLMN_ISDN_CALL
} GsmCallProceedingMsg;


/* Purpose:     Initiates Mobile Terminated Call Setup
 * Reference:   GSM 04.08, Section 9.3.23.1
 */
typedef struct struct_gsm_mt_setup_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    unsigned char   signal;

    unsigned char   callingPartyBcdNumber[( GSM_MAX_MSISDN_LENGTH /
                                              2 ) + ( GSM_MAX_MSISDN_LENGTH %
                                                        2 )];
    unsigned char   calledPartyBcdNumber[( GSM_MAX_MSISDN_LENGTH /
                                             2 ) + ( GSM_MAX_MSISDN_LENGTH %
                                                       2 )];
} GsmMtSetupMsg;


/* Purpose:     Terminating MS to network, confirms an incoming call request
 * Reference:   GSM 04.08, Section 9.3.2
 */
typedef struct struct_gsm_call_confirmed_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    //unsigned char cause;  // Section 10.5.4.11
}GsmCallConfirmedMsg;


/* Purpose:     Call Terminating MS to Network, called user alerting
 *              has been initiated.
 * Reference:   GSM 04.08, Section 9.3.1.2
 */
typedef struct struct_gsm_mt_alerting_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;
} GsmMtAlertingMsg;


/* Purpose:     Network to Call Originating MS, indicates terminating MS
 *              has been alerted.
 * Reference:   GSM 04.08, Section 9.3.1.1
 */
typedef struct struct_gsm_mo_alerting_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;
    // has other elements not used yet.
} GsmMoAlertingMsg;


/* Purpose:     Call Terminating MS to Network, indicates call acceptance
 * Reference:   GSM 04.08,Section 9.3.5.2
 */
typedef struct struct_mt_connect_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;
    // Other fields not needed at present
} GsmMtConnectMsg;


/* Purpose:     Network to Call Originating MS, indicates call acceptance
 *              by call terminating MS (called user).
 * Reference:   GSM 04.08,Section 9.3.5.1
 */
typedef struct struct_mo_connect_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    unsigned char   connectedNumber[GSM_MAX_IMSI_LENGTH];
    // Other fields not needed at present
} GsmMoConnectMsg;


/* Purpose:     Network to Call Terminating MS to indicate start of call.
 *              Call Originating MS to Network to ack start of call.
 * Reference:   GSM 04.08, Section 9.3.6
 * Comment:     This message/struct is used in both directions:
 *              MS <-> Network.
 *              The interface of BS on which it is received will determine
 *              the further direction.
 */
typedef struct struct_connect_ack_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;
} GsmConnectAcknowledgeMsg;

/*
 * Ref: GSM 04.08, sec 9.1.22, 3.
 */
typedef struct struct_gsm_paging_request_type1_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   pageMode; // GSM_PAGING_MODE_NORMAL
    unsigned char   channelNeeded;
    unsigned char   imsi[GSM_MAX_IMSI_LENGTH];  // MS being paged
} GsmPagingRequestType1Msg;


/*
 * Purpose: From MSC to BS to page the terminating MS.
 * Reference: GSM 08.08, sec 3.2.1.19
 */
typedef struct struct_paging_msg_str
{
    unsigned char   messageTypeCode;
    unsigned char   imsi[GSM_MAX_IMSI_LENGTH];
    uddtLAC         lac;
    uddtCellId      cellIdentity;
} GsmPagingMsg;


/* Purpose: Response to signalling link setup due to a paging msg
 *          MS (call terminating) to network
 * Reference: 04.08, Section 9.1.25
 */
typedef struct struct_paging_response_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;

    unsigned char   imsi[GSM_MAX_IMSI_LENGTH];  // MS being paged (dest)
} GsmPagingResponseMsg;


/*
 * Reference:       GSM 04.08, Section 9.3.7.2
 *                  GSM 04.07 Figure 4.3 for messages in a MO call release
 *                  & channel release successful case.
 */
typedef struct struct_gsm_disconnect_by_ms_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    unsigned char   cause[2]; // byte 1: location (user)
    // byte 2: cause value
    // facility
    // user-user
    // ss version
} GsmDisconnectByMsMsg;


typedef struct struct_gsm_disconnect_by_network_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    unsigned char   cause[2]; // byte 1: location ()
    // byte 2: cause value

    // facility
    unsigned char   progressIndicator[2]; // byte 1: location
    //   public network-local user
    // byte 2: end-to-end PLMN/ISDN call
    //   or in-band tones indication(#8)
    // user-user
} GsmDisconnectByNetworkMsg;


/* Name:        GsmReleaseByNetworkMsg
 * Purpose:     Indicate that the network intends to release the transaction
 *              identifier, and that the receiving equipment shall release the
 *              transaction identifier after sending RELEASE COMPLETE.

 * Reference:   GSM 04.08, Section 9.3.18.1
 */
typedef struct struct_gsm_release_by_network_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    unsigned char   cause[2]; // byte1: location(public nw serving local user)
    // byte2: cause value = normal event
    // second cause
    // facility
    // user-user
} GsmReleaseByNetworkMsg;


typedef struct struct_gsm_release_by_ms_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    unsigned char   cause[2]; // byte1: location(user)
    // byte2: cause value  = normal event
    // second cause
    // facility
    // user-user
    // ss version
} GsmReleaseByMsMsg;


/*
 * Purpose:     Sent from the mobile station to the network to indicate that
 *              the mobile station has released the transaction identifier
 *              and that the network shall release the transaction id.
 * Reference:   GSM 04.08, Section 9.3.19.2
 */

typedef struct struct_release_complete_by_ms_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    unsigned char   cause[2]; // byte 1: location (user)
    // byte 2: cause value
    // facility
    // user-user
    // ss version
} GsmReleaseCompleteByMsMsg;


typedef struct struct_release_complete_by_network_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   transactionIdentifier;

    unsigned char   cause[2]; // byte 1: location (plmn/isdn network)
    // byte 2: cause value
    // facility
    // user-user
} GsmReleaseCompleteByNetworkMsg;



typedef struct struct_gsm_channel_release_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;

    unsigned char   rrCause;

    // BA Range. Used for cell selection procedure.
} GsmChannelReleaseMsg;


/*
 * Not used yet
 */
typedef struct struct_gsm_assignment_request_msg_str
{
    unsigned char       messageType; // Sec 3.2.2.1
    GsmChannelType_e    channelType[3]; // byte 1 = signalling/traffic
    // byte 2 = channel type
    // byte 3 = rate for traffic
    // unsigned char   layer3Header[2];    // byte 1 = protocol Discriminator
    // byte 2 = transaction identifier
    // priority
    unsigned char       circuitIdentityCode[2];
    // downlink dtx flag
    // interference band to be used
    // classmark 2
} GsmAssignmentRequestMsg;


/*
 * Reference:   GSM 04.08 Section 9.2.15
 */
typedef struct struct_gsm_location_request_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    //GsmLocationUpdatingType_e   locationUpdatingType;
    unsigned char   locationUpdatingType;
    uddtLAC         lac;            /* Location Area Code, Ref: GSM 03.03 */

    // mobile station classmark
    unsigned char   imsi[GSM_MAX_IMSI_LENGTH];
} GsmLocationUpdateRequestMsg;


typedef struct struct_gsm_location_update_accept_msg_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    uddtLAC         lac;            /* Location Area Code, Ref: GSM 03.03 */

    unsigned char   imsi[GSM_MAX_IMSI_LENGTH];

    // Not used yet.
    //BOOL followOnProceed;  // Indicates that a MM connection can
    // be established using an existing RR
    // connection. For e.g. CM_SERVICE_REQUEST
} GsmLocationUpdateAcceptMsg;


// BSS - MSC messages

/*
 * Purpose  : Sent from BSS to MSC, on the A interface,
 *            carries the CM Service Request message.
 * Reference: GSM 08.08, Sec 3.2.1.32
 */
typedef struct struct_gsm_complete_layer3_information_str
{
    // first byte
    unsigned char   messageType; // Sec 3.2.2.1
    uddtLAC         lac;
    uddtCellId      cellIdentity;
    unsigned char   layer3Message; // CM Service Request(Originating MS)
    // or Paging Response(Terminating MS)
    // unsigned char channelType; // Optional
} GsmCompleteLayer3InfoMsg;


// See Section 6 of GSM 05.05 for Intra-MSC Handover procedures

// GSM 08.08 Section 3.2.1.9
// Serving BS to MSC
typedef struct struct_gsm_handover_required_str
{
    unsigned char   messageType;
    unsigned char   cause;
    // BOOL responseRequest;
    // Cell Identifier List
    unsigned char   cellIdDiscriminator;
    int             numCells;
    uddtLAC         lac[GSM_NUM_NEIGHBOUR_CELLS];
    uddtCellId      cellIdentity[GSM_NUM_NEIGHBOUR_CELLS];
    //unsigned char *currentChannel;
} GsmHandoverRequiredMsg;


// GSM 08.08 Section 3.2.1.16
// MSC to handover source BS
typedef struct struct_gsm_handover_required_reject_str
{
    unsigned char   messageType;
    unsigned char   cause;
} GsmHandoverRequiredRejectMsg;


// GSM 08.08 Section 3.2.1.37
// BS to MSC
typedef struct struct_gsm_handover_failure_str
{
    unsigned char   messageType;
    unsigned char   cause;
    unsigned char   rrCause;
} GsmHandoverFailureMsg;


// GSM 08.08 Section 3.2.1.8
// MSC to target BS
typedef struct struct_gsm_handover_request_str
{
    unsigned char   messageType;
    //unsigned char   channelType;
    //classMark information 1/2

    uddtLAC         servingBsLac;
    uddtCellId      servingBsCellIdentity;
    uddtLAC         targetBsLac;
    uddtCellId      targetBsCellIdentity;
    //unsigned char currentChannel;
    unsigned char   cause;
    int             trafficConnectionId;

    int             tempHoId; // non-std. used for ref in MSC
} GsmHandoverRequestMsg;


// Radio Interface Handover Message
// GSM 04.08 Section 9.1.15
// Sent inside of GsmHandoverRequestAckMsg and GsmHandOverCommandMgs
typedef struct struct_gsm_radio_interface_handover_command_str
{
    unsigned char       protocolDiscriminator;
    unsigned char       messageType;

    // Cell Description
    unsigned char       bcchArfcn;
    //short           bcchArfcn;

    // Channel Description of channel after time
    GsmChannelType_e    channelType;
    uddtSlotNumber      slotNumber;
    unsigned char       channelArfcn;
    unsigned char       hoReference;
    // power command and access type

    // synchronization indication
    //unsigned char syncIndication;

    //int             startingFrameNumber;
    clocktype           startTime;
} GsmRIHandoverCommandMsg;


// MSC to HO target BS
typedef struct struct_gsm_handover_request_ack_str
{
    unsigned char           messageType;
    unsigned char           chosenChannel;
    int                     tempHoId;   // used for ref in MSC to lookup target BS
    GsmRIHandoverCommandMsg riHoCmdMsg;
    //unsigned char   layer3MsgLength;
    //unsigned char   *layer3Msg; // GsmRIHandoverCommandMsg
    // until SCCP connection is established.
} GsmHandoverRequestAckMsg;


// MSC to HO originating/source BS
// GSM 08.08 Section 3.2.1.11
typedef struct struct_gsm_handover_command_str
{
    unsigned char           messageType;
    // which BS? target BS?
    uddtLAC                 lac;
    uddtCellId              cellIdentity;
    GsmRIHandoverCommandMsg riHoCmdMsg;
    //unsigned char   layer3MsgLength;
    //unsigned char   *layer3Msg;
} GsmHandoverCommandMsg;


// MS to source HO BS
// GSM 04.08 Section 9.1.16
typedef struct gsm_radio_interface_handover_complete_str
{
    unsigned char   protocolDiscriminator;
    unsigned char   messageType;
    unsigned char   rrCause;    // Normal Event
    // mobile observed time difference
} GsmRIHandoverCompleteMsg;


typedef struct struct_gsm_radio_interface_handover_access_msg_str
{
    unsigned char   hoReference;
} GsmRIHandoverAccessMsg;


// HO source BS to MSC
// GSM 08.08 Section
typedef struct struct_gsm_handover_complete_str
{
    unsigned char   messageType;
    unsigned char   rrCause;
} GsmHandoverCompleteMsg;


// MSC to old BS
// Release channel
// GSM 08.08 Section 3.2.1.21
typedef struct struct_gsm_clear_command_str
{
    unsigned char   messageType;
    // layer3Header
    //unsigned char    cause;
} GsmClearCommandMsg;


// BS to MSC, to initiate clear the current call
typedef struct struct_gsm_call_clear_command_str
{
    unsigned char   messageType;
    // layer3Header
    unsigned char   cause;
} GsmCallClearCommandMsg;


// old BS to MSC after HO procedures are over
// GSM 08.08 Section 3.2.1.22
typedef struct struct_gsm_clear_complete_str
{
    unsigned char   messageType;
} GsmClearCompleteMsg;


/* Layer 3 GSM functions */

void GsmLayer3Init(
    Node* node,
    const NodeInput* nodeInput);



void GsmLayer3Layer(
    Node* node,
    Message* msg);


void GsmLayer3Finalize(
    Node* node);


/* NAME:    GsmLayer3DequeuePacket
 * PURPOSE: Dequeue a packet from the network layer
 *          Called by the GSM MAC layer when it is
 *          ready to send a message.
 */
void GsmLayer3DequeuePacket(
    Node* node,
    short   slotToCheck,
    short   channelToCheck,
    Message** msg,
    uddtSlotNumber* slotNumber,
    uddtChannelIndex* channelIndex,
    GsmChannelType_e* channelType);


/* NAME:    GsmLayer3PeekAtNextPacket
 * PURPOSE: Peek at next packet in the network layer queue
 *          Called by the GSM MAC layer when (begining of a TS)
 *          it is ready to send a message.
 */
void GsmLayer3PeekAtNextPacket(
    Node* node,
    short   slotToCheck,
    short   channelToCheck,
    Message** msg,
    uddtSlotNumber* slotNumber,
    uddtChannelIndex* channelIndex,
    GsmChannelType_e* channelType);


/* NAME:    GsmLayer3SendPacket
 * PURPOSE: Enqueue packet for sending to MAC layer.
 *          Currently the queue holds only one packet.
 *          Memory allocated at GSM Network layer initialization.
 */
void GsmLayer3SendPacket(
    Node* node,
    Message* msg,
    uddtSlotNumber   slotNumber,
    uddtChannelIndex   channelIndex,
    GsmChannelType_e   channelType);


/* NAME:    GsmLayer3ReceivePacketFromMac
 * PURPOSE: Process packets received from MAC
 */

void GsmLayer3ReceivePacketFromMac(
    Node* node,
    Message* msg,
    uddtSlotNumber   recvSlotNumber,
    uddtFrameNumber     recvFrameNumber,
    uddtChannelIndex   recvChannelIndex,
    int     recvInterfaceIndex);


void printGsmImsi(
    unsigned char* imsiStr,
    int length);


void GsmLayer3ReceivePacketOverIp(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress);


void GsmLayer3ReceiveTrafficPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress);

void gsmBsProcessMeasReport(
    Node* node,
    uddtSlotNumber   slotNumber,
    uddtChannelIndex   channelIndex,
    GsmChannelMeasurement* chMeas);


void GsmLayer3ReceivePacketFromGsmAInterface(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress);


void GsmLayer3SendPacketViaGsmAInterface(
    Node* node,
    NodeId         destNodeId,
    NodeId         sourceNodeId,
    int                 sccpConnectionId,
    GsmSccpRoutingLabel routingLabel,
    unsigned char       messageTypeCode,
    unsigned char* msgData,
    int                 msgDataSize);

#endif
