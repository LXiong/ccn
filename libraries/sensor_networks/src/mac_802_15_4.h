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

#ifndef MAC_802_15_4_H
#define MAC_802_15_4_H

#include "api.h"
#include "mac_802_15_4_cmn.h"
#include "csma_802_15_4.h"
#include "sscs_802_15_4.h"
#include "phy_802_15_4.h"
#include "mac_802_15_4_transac.h"

// --------------------------------------------------------------------------
// #define's
// --------------------------------------------------------------------------
// /**
// CONSTANT    :: aNumSuperframeSlots : 16
// DESCRIPTION :: # of slots contained in a superframe
// **/
const UInt8 aNumSuperframeSlots = 16;

// /**
// CONSTANT    :: aBaseSlotDuration : 60
// DESCRIPTION :: # of symbols comprising a superframe slot of order 0
// **/
const UInt8 aBaseSlotDuration = 60;

// /**
// CONSTANT    :: aBaseSuperframeDuration :
//                      aBaseSlotDuration*aNumSuperframeSlots
// DESCRIPTION :: # of symbols comprising a superframe of order 0
// **/
const UInt16 aBaseSuperframeDuration
        = aBaseSlotDuration*aNumSuperframeSlots;

// /**
// CONSTANT    :: aMaxBE : 5
// DESCRIPTION :: max value of the backoff exponent in the CSMA-CA algorithm
// **/
const UInt8 aMaxBE = 6;


// /**
// CONSTANT    :: aGTSDescPersistenceTime : 4
// DESCRIPTION :: # of superframes that a GTS descriptor exists in the beacon
//                  frame of a PAN coordinator
// **/
const UInt8 aGTSDescPersistenceTime = 4;


// /**
// CONSTANT    :: aMaxNumGts : 7
// DESCRIPTION :: # of of GTSs that can be allocated by PAN coordinator
// **/
const UInt8 aMaxNumGts = 7;

// /**
// CONSTANT    :: aMaxNumGtsSlotsRequested : 15 
// DESCRIPTION :: Maximum number of GTS slots that can be requested in 
//                             a GTS request
// **/
const UInt8 aMaxNumGtsSlotsRequested = 15;

// /**
// CONSTANT    :: aMaxFrameOverhead : 25
// DESCRIPTION :: max # of octets added by the MAC sublayer to its payload
//                  w/o security.
// **/
const UInt8 aMaxFrameOverhead = 25;

// /**
// CONSTANT    :: aMaxFrameResponseTime : 10000
// DESCRIPTION :: max # of symbols (or CAP symbols) to wait for a response
//                  frame
// **/
const UInt16 aMaxFrameResponseTime  = 10000;

// /**
// CONSTANT    :: aMaxFrameRetries : 3
// DESCRIPTION :: max # of retries allowed after a transmission failures
// **/
const UInt8 aMaxFrameRetries = 3;

// /**
// CONSTANT    :: aMaxLostBeacons : 4
// DESCRIPTION :: max # of consecutive beacons the MAC sublayer can miss w/o
//                  declaring a loss of synchronization
// **/
const UInt8 aMaxLostBeacons = 4;

// /**
// CONSTANT    :: aMaxMACFrameSize : aMaxPHYPacketSize - aMaxFrameOverhead
// DESCRIPTION :: max # of octets that can be transmitted in the MAC frame
//                  payload field
// **/
const UInt8 aMaxMACFrameSize = aMaxPHYPacketSize - aMaxFrameOverhead;

// /**
// CONSTANT    :: aMaxSIFSFrameSize : 18
// DESCRIPTION :: max size of a frame, in octets, that can be followed by a
//                  SIFS period
// **/
const UInt8 aMaxSIFSFrameSize = 18;

// /**
// CONSTANT    :: aMinCAPLength : 440
// DESCRIPTION :: min # of symbols comprising the CAP
// **/
const UInt16 aMinCAPLength = 440;

// /**
// CONSTANT    :: aMinLIFSPeriod : 40
// DESCRIPTION :: min # of symbols comprising a LIFS period
// **/
const UInt8 aMinLIFSPeriod = 40;

// /**
// CONSTANT    :: aMinSIFSPeriod : 12
// DESCRIPTION :: min # of symbols comprising a SIFS period
// **/
const UInt8 aMinSIFSPeriod = 12;

// /**
// CONSTANT    :: aResponseWaitTime : 32 * aBaseSuperframeDuration
// DESCRIPTION :: max # of symbols a device shall wait for a response command
//                  following a request command
// **/
const UInt16 aResponseWaitTime = 32 * aBaseSuperframeDuration;

// /**
// CONSTANT    :: aUnitBackoffPeriod : 20
// DESCRIPTION :: # of symbols comprising the basic time period used by the
//                  CSMA-CA algorithm
// **/
const UInt8 aUnitBackoffPeriod = 20;

// /**
// CONSTANT    :: numPsduOctetsInAckFrame : 5
// DESCRIPTION :: Acknowledgement frame size in Octets
// **/
const UInt8 numPsduOctetsInAckFrame = 5;

// /**
// CONSTANT    :: numBitsPerOctet : 8
// DESCRIPTION :: Number of bits in an Octet
// **/
const UInt8 numBitsPerOctet = 8;

// maximum propagation delay
const clocktype max_pDelay = (100 * SECOND/200000000);

// String size
#define M802_15_4_STRING_SIZE          81

// Maximum GTS descriptor count
#define M802_15_4_GTS_DESC_COUNT       7

// /**
// CONSTANT    :: M802_15_4_ACKWAITDURATION : 200
// DESCRIPTION :: The maximum number of symbols to wait for an acknowledgment
//                 frame to arrive following a transmitted data frame.
// **/
#define M802_15_4_ACKWAITDURATION                  650

// /**
// CONSTANT    :: M802_15_4_ASSOCIATIONPERMIT : FALSE
// DESCRIPTION :: Indicates whether a co-ordinator is allowing association.
// **/
#define M802_15_4_ASSOCIATIONPERMIT                FALSE

// /**
// CONSTANT    :: M802_15_4_AUTOREQUEST : TRUE
// DESCRIPTION :: Indicates whether a device automatically sends data
//                  request command if its address is listed in beacon frame.
// **/
#define M802_15_4_AUTOREQUEST                      TRUE

// /**
// CONSTANT    :: M802_15_4_BATTLIFEEXT : FALSE
// DESCRIPTION :: Indicates whether BLE is enabled.
// **/
#define M802_15_4_BATTLIFEEXT                      FALSE

// /**
// CONSTANT    :: M802_15_4_BATTLIFEEXTPERIODS : 6
// DESCRIPTION :: In BLE mode, the number of backoff periods during which
//                  the receiver is enabled after the IFS following a beacon.
// **/
#define M802_15_4_BATTLIFEEXTPERIODS               6

// /**
// CONSTANT    :: M802_15_4_BEACONPAYLOAD : ""
// DESCRIPTION :: The contents of the beacon payload.
// **/
#define M802_15_4_BEACONPAYLOAD                    ""

// /**
// CONSTANT    :: M802_15_4_BEACONPAYLOADLENGTH : 0
// DESCRIPTION :: Beacon payload length in octets.
// **/
#define M802_15_4_BEACONPAYLOADLENGTH              0

// /**
// CONSTANT    :: M802_15_4_BEACONORDER : 15
// DESCRIPTION :: Specification of how often the coordinator transmits its
//                  beacon.
// **/
#define M802_15_4_BEACONORDER                      15

// /**
// CONSTANT    :: M802_15_4_BEACONTXTIME : 0x000000
// DESCRIPTION :: The time that the device transmitted its last beacon
//                  frame, in symbol periods.
// **/
#define M802_15_4_BEACONTXTIME                     0x000000

// /**
// CONSTANT    :: M802_15_4_BSN : 0
// DESCRIPTION :: Beacon sequence number
// **/
#define M802_15_4_BSN                0

// /**
// CONSTANT    :: M802_15_4_COORDEXTENDEDADDRESS : 0xffff
// DESCRIPTION :: The 64-bit address of the coordinator through which the
//                  device is associated.
// **/
#define M802_15_4_COORDEXTENDEDADDRESS             0xffff

// /**
// CONSTANT    :: M802_15_4_COORDSHORTADDRESS : 0xffff
// DESCRIPTION :: The 16-bit address of the coordinator through which the
//                  device is associated.
// **/
#define M802_15_4_COORDSHORTADDRESS                0xffff

// /**
// CONSTANT    :: M802_15_4_DSN : 0
// DESCRIPTION :: Beacon sequence number
// **/
#define M802_15_4_DSN                0

// /**
// CONSTANT    :: M802_15_4_GTSPERMIT : TRUE
// DESCRIPTION :: Indicates whether PAN co-ordinator accepts GTS requests.
// **/
#define M802_15_4_GTSPERMIT                        TRUE

// /**
// CONSTANT    :: M802_15_4_MAXCSMABACKOFFS : 4
// DESCRIPTION :: The maximum value of the backoff exponent, BE, in the
//                  CSMA-CA algorithm.
// **/
#define M802_15_4_MAXCSMABACKOFFS                  4

// /**
// CONSTANT    :: M802_15_4_MINBE : 3
// DESCRIPTION :: The minimum value of the backoff exponent, BE, in the
//                  CSMA-CA algorithm.
// **/
#define M802_15_4_MINBE                            3

// /**
// CONSTANT    :: M802_15_4_PANID : 0xffff
// DESCRIPTION :: The 16-bit identifier of the PAN on which the device is
//                  operating.
// **/
#define M802_15_4_PANID                            0xffff

// /**
// CONSTANT    :: M802_15_4_PROMISCUOUSMODE : FALSE
// DESCRIPTION :: Indicates whether MAC sublayer is in a promiscuous mode
// **/
#define M802_15_4_PROMISCUOUSMODE                  FALSE

// /**
// CONSTANT    :: M802_15_4_RXONWHENIDLE : FALSE
// DESCRIPTION :: Indicates whether MAC is to enable its receiver during idle
//                  periods.
// **/
#define M802_15_4_RXONWHENIDLE                     FALSE

// /**
// CONSTANT    :: M802_15_4_SHORTADDRESS : 0xffff
// DESCRIPTION :: The 16-bit address that the device uses to communicate
//                  in the PAN.
// **/
#define M802_15_4_SHORTADDRESS                     0xffff

// /**
// CONSTANT    :: M802_15_4_SUPERFRAMEORDER : 15
// DESCRIPTION :: The length of the active portion of the outgoing
//                  superframe, including the beacon frame.
// **/
#define M802_15_4_SUPERFRAMEORDER                  15

// /**
// CONSTANT    :: M802_15_4_TRANSACTIONPERSISTENCETIME : 0x01f4
// DESCRIPTION :: The maximum time (in unit periods) that a transaction is
//                  stored by a coordinator and indicated in its beacon.
// **/
#define M802_15_4_TRANSACTIONPERSISTENCETIME       0x01f4

// /**
// CONSTANT    :: M802_15_4_ACLENTRYDESCRIPTORSET : NULL
// DESCRIPTION ::
// **/
#define M802_15_4_ACLENTRYDESCRIPTORSET            NULL

// /**
// CONSTANT    :: M802_15_4_ACLENTRYDESCRIPTORSETSIZE : 0x00
// DESCRIPTION ::
// **/
#define M802_15_4_ACLENTRYDESCRIPTORSETSIZE        0x00

// /**
// CONSTANT    :: M802_15_4_DEFAULTSECURITY : FALSE
// DESCRIPTION ::
// **/
#define M802_15_4_DEFAULTSECURITY                  FALSE

// /**
// CONSTANT    :: M802_15_4_ACLDEFAULTSECURITYMATERIALLENGTH : 0x15
// DESCRIPTION ::
// **/
#define M802_15_4_ACLDEFAULTSECURITYMATERIALLENGTH 0x15

// /**
// CONSTANT    :: M802_15_4_DEFAULTSECURITYMATERIAL : NULL
// DESCRIPTION ::
// **/
#define M802_15_4_DEFAULTSECURITYMATERIAL          NULL

// /**
// CONSTANT    :: M802_15_4_DEFAULTGTSPOSITION : 255
// DESCRIPTION :: Default Position descriptor
// **/
#define M802_15_4_DEFAULTGTSPOSITION 255

// /**
// CONSTANT    :: M802_15_4_DEFAULTSECURITYSUITE : 0x00
// DESCRIPTION ::
// **/
#define M802_15_4_DEFAULTSECURITYSUITE             0x00

// /**
// CONSTANT    :: M802_15_4_SECURITYMODE : 0x00
// DESCRIPTION ::
// **/
#define M802_15_4_SECURITYMODE                     0x00

// /**
// CONSTANT    :: M802_15_4_DATARATE_INDEX : 0x00
// DESCRIPTION ::
// **/
#define M802_15_4_DEFAULT_DATARATE_INDEX            0x00

// /**
// CONSTANT    :: M802_15_4_DEFAULTDEVCAP : 0xc1
// DESCRIPTION :: Default device capabilities
//                  alterPANCoor = true
//                  FFD = true
//                  mainsPower = false
//                  recvOnWhenIdle = false
//                  secuCapable = false
//                  alloShortAddr = true
// **/
#define M802_15_4_DEFAULTDEVCAP                    0xc1

#define M802_15_4DEFFRMCTRL_TYPE_BEACON      0x00
#define M802_15_4DEFFRMCTRL_TYPE_DATA        0x01
#define M802_15_4DEFFRMCTRL_TYPE_ACK         0x02
#define M802_15_4DEFFRMCTRL_TYPE_MACCMD      0x03

#define M802_15_4DEFFRMCTRL_ADDRMODENONE     0x01
#define M802_15_4DEFFRMCTRL_ADDRMODE16       0x02
#define M802_15_4DEFFRMCTRL_ADDRMODE64       0x03


// Broadcast Queue size in bytes
#define M802_15_4BROADCASTQUEUESIZE          10000


// --------------------------------------------------------------------------
// typedef's enums
// --------------------------------------------------------------------------

// /**
// ENUM        :: M802_15_4TimerType
// DESCRIPTION :: Timers used by MAC
// **/
typedef enum
{
    M802_15_4TXOVERTIMER,
    M802_15_4TXTIMER,
    M802_15_4EXTRACTTIMER,
    M802_15_4ASSORSPWAITTIMER,
    M802_15_4DATAWAITTIMER,
    M802_15_4RXENABLETIMER,
    M802_15_4SCANTIMER,
    M802_15_4BEACONTXTIMER,
    M802_15_4BEACONRXTIMER,
    M802_15_4BEACONSEARCHTIMER,
    M802_15_4TXCMDDATATIMER,
    M802_15_4BACKOFFBOUNDTIMER,
    M802_15_4ORPHANRSPTIMER,
    M802_15_4IFSTIMER,
    M802_15_4CHECKOUTGOING,


    M802_15_4BROASCASTTIMER,

    M802_15_4GTSTIMER,
    M802_15_4GTSDEALLOCATETIMER
}M802_15_4TimerType;

// --------------------------------------------------------------------------
// typedef's struct
// --------------------------------------------------------------------------

typedef struct mac_802_15_4_taskPending
{
    // ----------------
    BOOL      mcps_data_request;
    UInt8     mcps_data_request_STEP;

    BOOL      mcps_broadcast_request;
    UInt8     mcps_broadcast_request_STEP;

    char      mcps_data_request_frFunc[M802_15_4_STRING_SIZE];
    UInt8     mcps_data_request_TxOptions;
    UInt8     mcps_data_request_Last_TxOptions;
    Message*  mcps_data_request_pendPkt;

    BOOL      mlme_associate_request;
    UInt8     mlme_associate_request_STEP;
    char      mlme_associate_request_frFunc[M802_15_4_STRING_SIZE];
    BOOL      mlme_associate_request_SecurityEnable;
    UInt8     mlme_associate_request_CoordAddrMode;
    Message*  mlme_associate_request_pendPkt;

    BOOL      mlme_associate_response;
    UInt8     mlme_associate_response_STEP;
    char      mlme_associate_response_frFunc[M802_15_4_STRING_SIZE];
    MACADDR   mlme_associate_response_DeviceAddress;
    Message*  mlme_associate_response_pendPkt;

    BOOL      mlme_disassociate_request;
    UInt8     mlme_disassociate_request_STEP;
    char      mlme_disassociate_request_frFunc[M802_15_4_STRING_SIZE];
    BOOL      mlme_disassociate_request_toCoor;
    Message*  mlme_disassociate_request_pendPkt;

    BOOL      mlme_orphan_response;
    UInt8     mlme_orphan_response_STEP;
    char      mlme_orphan_response_frFunc[M802_15_4_STRING_SIZE];
    MACADDR   mlme_orphan_response_OrphanAddress;

    BOOL      mlme_reset_request;
    UInt8     mlme_reset_request_STEP;
    char      mlme_reset_request_frFunc[M802_15_4_STRING_SIZE];
    BOOL      mlme_reset_request_SetDefaultPIB;

    BOOL      mlme_rx_enable_request;
    UInt8     mlme_rx_enable_request_STEP;
    char      mlme_rx_enable_request_frFunc[M802_15_4_STRING_SIZE];
    UInt32    mlme_rx_enable_request_RxOnTime;
    UInt32    mlme_rx_enable_request_RxOnDuration;
    clocktype mlme_rx_enable_request_currentTime;

    BOOL      mlme_scan_request;
    UInt8     mlme_scan_request_STEP;
    char      mlme_scan_request_frFunc[M802_15_4_STRING_SIZE];
    UInt8     mlme_scan_request_ScanType;
    UInt8     mlme_scan_request_orig_macBeaconOrder;
    UInt8     mlme_scan_request_orig_macBeaconOrder2;
    UInt8     mlme_scan_request_orig_macBeaconOrder3;
    UInt16    mlme_scan_request_orig_macPANId;
    UInt32    mlme_scan_request_ScanChannels;
    UInt8     mlme_scan_request_ScanDuration;
    UInt8     mlme_scan_request_CurrentChannel;
    UInt8     mlme_scan_request_ListNum;
    UInt8     mlme_scan_request_EnergyDetectList[27];
    M802_15_4PanEle mlme_scan_request_PANDescriptorList[27];

    BOOL      mlme_start_request;
    UInt8     mlme_start_request_STEP;
    char      mlme_start_request_frFunc[M802_15_4_STRING_SIZE];
    UInt8     mlme_start_request_BeaconOrder;
    UInt8     mlme_start_request_SuperframeOrder;
    BOOL      mlme_start_request_BatteryLifeExtension;
    BOOL      mlme_start_request_SecurityEnable;
    BOOL      mlme_start_request_PANCoordinator;
    UInt16    mlme_start_request_PANId;
    UInt8     mlme_start_request_LogicalChannel;

    BOOL      mlme_sync_request;
    UInt8     mlme_sync_request_STEP;
    char      mlme_sync_request_frFunc[M802_15_4_STRING_SIZE];
    UInt8     mlme_sync_request_numSearchRetry;
    BOOL      mlme_sync_request_tracking;

    BOOL      mlme_poll_request;
    UInt8     mlme_poll_request_STEP;
    char      mlme_poll_request_frFunc[M802_15_4_STRING_SIZE];
    UInt8     mlme_poll_request_CoordAddrMode;
    UInt16    mlme_poll_request_CoordPANId;
    MACADDR   mlme_poll_request_CoordAddress;
    BOOL      mlme_poll_request_SecurityEnable;
    BOOL      mlme_poll_request_autoRequest;
    BOOL      mlme_poll_request_pending;

    BOOL      mlme_gts_request;
    UInt8     mlme_gts_request_STEP;
    UInt8     gtsChracteristics;
    char      mlme_gts_request_frFunc[M802_15_4_STRING_SIZE];
    BOOL      mcps_gts_data_request;
    UInt8     mcps_gts_data_request_STEP;
    char      mlme_gts_data_request_frFunc[M802_15_4_STRING_SIZE];

    BOOL      CCA_csmaca;
    UInt8     CCA_csmaca_STEP;

    BOOL      RX_ON_csmaca;
    UInt8     RX_ON_csmaca_STEP;
}M802_15_4TaskPending;


// /**
// STRUCT      :: M802_15_4Timer
// DESCRIPTION :: Timer type for MAC 802.15.4
// **/
typedef struct mac_802_15_4_timer_str
{
    M802_15_4TimerType timerType;
}M802_15_4Timer;


// /**
// ENUM        :: mac_802_15_4_TransacLink_Operation
// DESCRIPTION :: Various transaction link operation
// **/
enum mac_802_15_4_TransacLink_Operation
{
    OPER_DELETE_TRANSAC = 1,
    OPER_PURGE_TRANSAC,
    OPER_CHECK_TRANSAC
};


// /**
// ENUM        :: mac_802_15_4_DeviceLink_Operation
// DESCRIPTION :: Various device link operation
// **/
enum mac_802_15_4_DeviceLink_Operation
{
    OPER_DELETE_DEVICE_REFERENCE = 1,
    OPER_CHECK_DEVICE_REFERENCE,
    OPER_GET_DEVICE_REFERENCE
};

// /**
// ENUM        :: mac_802_15_4_Gts_Operation
// DESCRIPTION :: Steps while sending packets in GTS
// **/
enum mac_802_15_4_Gts_Operation
{
    GTS_INIT_STEP,
    GTS_PKT_SENT_STEP,
    GTS_ACK_STATUS_STEP,
    GTS_PKT_RETRY_STEP
};

// /**
// ENUM        :: mac_802_15_4_Gts_Request_Operation
// DESCRIPTION :: Steps while sending GTS request
// **/
enum mac_802_15_4_Gts_Request_Operation
{
    GTS_REQUEST_INIT_STEP,
    GTS_REQUEST_CSMA_STATUS_STEP,
    GTS_REQUEST_PKT_SENT_STEP,
    GTS_REQUEST_ACK_STATUS_STEP
};

// /**
// ENUM        :: mac_802_15_4_Indirect_Operation
// DESCRIPTION :: Steps while sending packets via indirect mode
// **/
enum mac_802_15_4_Indirect_Operation
{
    INDIRECT_INIT_STEP,
    INDIRECT_PKT_SENT_STEP,
    INDIRECT_TIMER_EXPIRE_STEP
};

// /**
// ENUM        :: mac_802_15_4_Direct_Operation
// DESCRIPTION :: Steps while sending packets via direct mode
// **/
enum mac_802_15_4_Direct_Operation
{
    DIRECT_INIT_STEP,
    DIRECT_CSMA_STATUS_STEP,
    DIRECT_PKT_SENT_STEP,
    DIRECT_ACK_STATUS_STEP
};

// /**
// ENUM        :: mac_802_15_4_Poll_Operation
// DESCRIPTION :: Steps while sending poll request to parent
// **/
enum mac_802_15_4_Poll_Operation
{
    POLL_INIT_STEP,
    POLL_CSMA_STATUS_STEP,
    POLL_PKT_SENT_STEP,
    POLL_ACK_STATUS_STEP,
    POLL_DATA_RCVD_STATUS_STEP
};

// /**
// ENUM        :: mac_802_15_4_Sync_Operation
// DESCRIPTION :: Steps while syncing with the parent
// **/
enum mac_802_15_4_Sync_Operation
{
    SYNC_INIT_STEP,
    SYNC_BEACON_RCVD_STATUS_STEP
};

// /**
// ENUM        :: mac_802_15_4_Backoff_Operation
// DESCRIPTION :: Backoff states
// **/
enum mac_802_15_4_Backoff_Operation
{
    BACKOFF_RESET,
    BACKOFF_SUCCESSFUL,
    BACKOFF_FAILED,
    BACKOFF = 99,
};

// /**
// ENUM        :: mac_802_15_4_Txop
// DESCRIPTION :: Transmission options
// **/
enum mac_802_15_4_Txop
{
    TxOp_Acked = 0x01,
    TxOp_GTS = 0x02,
    TxOp_Indirect = 0x04,
    TxOp_SecEnabled = 0x08
};

// /**
// ENUM        :: mac_802_15_4_Association_Operation
// DESCRIPTION :: Steps while sending association request to parent
// **/
enum mac_802_15_4_Association_Operation
{
    ASSOC_INIT_STEP,
    ASSOC_CSMA_STATUS_STEP,         // 1
    ASSOC_PKT_SENT_STEP,            // 2
    ASSOC_ACK_STATUS_STEP,          // 3
    ASSOC_DATAREQ_INIT_STEP,        // 4
    ASSOC_DATAREQ_CSMA_STATUS_STEP, // 5
    ASSOC_DATAREQ_PKT_SENT_STEP,    // 6
    ASSOC_DATAREQ_ACK_STATUS_STEP,  // 7
    ASSOC_RESPONSE_STATUS_STEP      // 8
};

// /**
// ENUM        :: mac_802_15_4_ED_Scan_Operation
// DESCRIPTION :: Steps while performing ED scanning
// **/
enum mac_802_15_4_ED_Scan_Operation
{
    ED_SCAN_INIT_STEP,
    ED_SCAN_SET_CONFIRM_STATUS_STEP,  // 1
    ED_SCAN_TRX_STATE_STATUS_STEP,    // 2
    ED_SCAN_ED_CONFIRM_STATUS_STEP,   // 3
    ED_SCAN_CONFIRM_STEP              // 4
};

// /**
// ENUM        :: mac_802_15_4_Active_Passive_Scan_Operation
// DESCRIPTION :: Steps while performing Active or Passive scanning
// **/
enum mac_802_15_4_Active_Passive_Scan_Operation
{
    SCAN_INIT_STEP,
    SCAN_CREATE_BEACON_OR_SET_TRX_STATE_STEP,   // 1
    ACTIVE_SCAN_CSMA_STATUS_STEP,               // 2
    ACTIVE_SCAN_SET_TRX_STATE_STEP,             // 3
    SCAN_TRX_STATE_STATUS_STEP,                 // 4
    SCAN_BEACON_RCVD_STATUS_STEP,               // 5
    SCAN_CHANNEL_POS_CHANGE_STEP,               // 6
    SCAN_CONFIRM_STEP                           // 7
};

// /**
// ENUM        :: mac_802_15_4_Orphan_Scan_Operation
// DESCRIPTION :: Steps while performing Orphan scanning
// **/
enum mac_802_15_4_Orphan_Scan_Operation
{
    ORPHAN_SCAN_INIT_STEP,
    ORPHAN_SCAN_CREATE_BEACON_STEP,                 // 1
    ORPHAN_SCAN_CSMA_STATUS_STEP,                   // 2
    ORPHAN_SCAN_BEACON_SENT_STATUS_STEP,            // 3
    ORPHAN_SCAN_TRX_STATE_STATUS_STEP,              // 4
    ORPHAN_SCAN_COOR_REALIGNMENT_RECVD_STATUS_STEP, // 5
    ORPHAN_SCAN_CONFIRM_STEP                        // 6
};

// /**
// ENUM        :: mac_802_15_4_Start_Operation
// DESCRIPTION :: Steps while starting a device
// **/
enum mac_802_15_4_Start_Operation
{
    START_INIT_STEP,
    START_CSMA_STATUS_STEP,                   // 1
    START_TRX_STATE_STATUS_STEP,              // 2
    START_BEACON_SENT_STATUS_STEP             // 3
};


// /**
// STRUCT      :: M802_15_4PanAddrInfo
// DESCRIPTION :: PAN Address Information
// **/
typedef struct mac_802_15_4_panaddrinfo_str
{
    UInt16 panID;
    union
    {
        UInt16 addr_16;
        MACADDR addr_64; // using 48-bit instead of 64-bit
    };
}M802_15_4PanAddrInfo;

// /**
// STRUCT      :: M802_15_4GTSDescriptor
// DESCRIPTION :: GTS descriptor
// **/
typedef struct mac_802_15_4_gtsdescriptor_str
{
    UInt16 devAddr16;
    UInt8 slotSpec;    // --(0123):    GTS starting slot
                // --(4567):    GTS length
}M802_15_4GTSDescriptor;

// /**
// STRUCT      :: M802_15_4GTSFields
// DESCRIPTION :: GTS fields
// **/
typedef struct mac_802_15_4_gtsfields_str
{
    UInt8 spec;        // GTS specification
                // --(012): GTS descriptor count
                // --(3456):    reserved
                // --(7):   GTS permit
    UInt8 dir;     // GTS directions
                // --(0123456): for up to 7 descriptors:
                // 1 = receive only (w.r.t. data transmission by the device)
                // 0 = transmit only (w.r.t. data transmission by the device)
                // --(7):   reserved
    M802_15_4GTSDescriptor list[M802_15_4_GTS_DESC_COUNT];
                 // GTS descriptor list
}M802_15_4GTSFields;

// /**
// STRUCT      :: M802_15_4GTSSpec
// DESCRIPTION :: GTS specifications
// **/
typedef struct mac_802_15_4_gtsspec_str
{
    M802_15_4GTSFields fields;
    UInt8 count;       // GTS descriptor count
    BOOL permit;        // GTS permit
    BOOL recvOnly[M802_15_4_GTS_DESC_COUNT];   // reception only
    UInt8 slotStart[M802_15_4_GTS_DESC_COUNT];    // starting slot
    UInt8 length[M802_15_4_GTS_DESC_COUNT];   // length in slots
    Message *msg[M802_15_4_GTS_DESC_COUNT];  // messages pending to be sent
    clocktype endTime[M802_15_4_GTS_DESC_COUNT];
    Message *deAllocateTimer[M802_15_4_GTS_DESC_COUNT];
    UInt8 aGtsDescPersistanceCount[M802_15_4_GTS_DESC_COUNT];
    UInt16 expiraryCount[M802_15_4_GTS_DESC_COUNT];
    Queue* queue[M802_15_4_GTS_DESC_COUNT];
    UInt8 retryCount[M802_15_4_GTS_DESC_COUNT];
    short appPort[M802_15_4_GTS_DESC_COUNT];
}M802_15_4GTSSpec;

// /**
// STRUCT      :: M802_15_4PendAddrField
// DESCRIPTION :: Pending Address field
// **/
typedef struct mac_802_15_4_pendaddrfield_str
{
    UInt8 spec;        // Pending address specification field
                // --(012): num of short addresses pending
                // --(3):   reserved
                // --(456): num of extended addresses pending
                // --(7):   reserved
    MACADDR addrList[7];    // pending address list
}M802_15_4PendAddrField;

// /**
// STRUCT      :: M802_15_4PendAddrSpec
// DESCRIPTION :: Pending Address specification
// **/
typedef struct mac_802_15_4_pendaddrspec_str
{
    M802_15_4PendAddrField fields;
    UInt8 numShortAddr;    // num of short addresses pending
    UInt8 numExtendedAddr; // num of extended addresses pending
}M802_15_4PendAddrSpec;

// /**
// STRUCT      :: M802_15_4DevCapability
// DESCRIPTION :: Device capabilities
// **/
typedef struct mac_802_15_4_devcapability_str
{
    UInt8 cap;  // --(0):   alternate PAN coordinator
                // --(1):   device type (1=FFD,0=RFD)
                // --(2):   power source(1=mains powered,0=non mains powered)
                // --(3):   receiver on when idle
                // --(45):  reserved
                // --(6):   security capability
                // --(7):   allocate address)
    BOOL alterPANCoor;
    BOOL FFD;
    BOOL mainsPower;
    BOOL recvOnWhenIdle;
    BOOL secuCapable;
    BOOL alloShortAddr;
}M802_15_4DevCapability;

// /**
// STRUCT      :: M802_15_4FrameCtrl
// DESCRIPTION :: MAC Frame control
// **/
typedef struct mac_802_15_4_framectrl_str
{
    UInt16 frmCtrl; // (PSDU/MPDU) Frame Control (Figure 35)
                    // --leftmost bit numbered 0 and transmitted first
                    // --(012): Frame type (Table 65)
                    //       --(210)=000:       Beacon
                    //       --(210)=001:       Data
                    //       --(210)=010:       Ack.
                    //       --(210)=011:       MAC command
                    //       --(210)=others:    Reserved
                    // --(3):   Security enabled
                    // --(4):   Frame pending
                    // --(5):   Ack. req.
                    // --(6):   Intra PAN
                    // --(789): Reserved
                    // --(ab):  Dest. addressing mode (Table 66)
                    //       --(ba)=00: PAN ID and Addr. field not present
                    //       --(ba)=01: Reserved
                    //       --(ba)=10: 16-bit short address
                    //       --(ba)=11: 64-bit extended address
                    // --(cd):  Reserved
                    // --(ef):  Source addressing mode
    UInt8 frmType;
    BOOL secu;
    BOOL frmPending;
    BOOL ackReq;
    BOOL intraPan;
    UInt8 dstAddrMode;
    UInt8 srcAddrMode;
}M802_15_4FrameCtrl;

// /**
// STRUCT      :: M802_15_4SuperframeSpec
// DESCRIPTION :: MAC Superframe specification
// **/
typedef struct mac_802_15_4_superframespec_str
{
    UInt16 superSpec;      // (MSDU) Superframe Specification (Figure 40)
                    // --(0123):    Beacon order
                    // --(4567):    Superframe order
                    // --(89ab):    Final CAP slot
                    // --(c):   Battery life extension
                    // --(d):   Reserved
                    // --(e):   PAN Coordinator
                    // --(f):   Association permit
    UInt8 BO;
    UInt32 BI;
    UInt8 SO;
    UInt32 SD;
    UInt32 sd;
    UInt8 FinCAP;
    BOOL BLE;
    BOOL PANCoor;
    BOOL AssoPmt;
}M802_15_4SuperframeSpec;


// /**
// STRUCT      :: M802_15_4BeaconFrame
// DESCRIPTION :: 802_15_4 Beacon Frame Header
// **/
typedef struct mac_802_15_4_beacon_frame_str
{
    // ---beacon frame (Figures 10,37)---
    UInt16     MSDU_SuperSpec;     // (MSDU) Superframe Specification
                        // --(0123):    Beacon order
                        // --(4567):    Superframe order
                        // --(89ab):    Final CAP slot
                        // --(c):   Battery life extension
                        // --(d):   Reserved
                        // --(e):   PAN Coordinator
                        // --(f):   Association permit
    M802_15_4GTSFields   MSDU_GTSFields;     // GTS Fields (Figure 38)
    M802_15_4PendAddrField  MSDU_PendAddrFields;    // (MSDU) Address Fields
                        // --(012): # of short addressing pending
                        // --(3):   Reserved
                        // --(456): # of extended addressing pending
                        // --(7):   Reserved
    // MSDU_BeaconPL;            // (MSDU) Beacon Payload
//     UInt16     MFR_FCS;        // (PSDU/MPDU) FCS
}M802_15_4BeaconFrame;

// /**
// STRUCT      :: M802_15_4DataFrame
// DESCRIPTION :: 802_15_4 Data Frame Header
// **/
typedef struct mac_802_15_4_data_frame_str
{
    // ---data frame
}M802_15_4DataFrame;

// /**
// STRUCT      :: M802_15_4AckFrame
// DESCRIPTION :: 802_15_4 Ack Frame Header
// **/
typedef struct mac_802_15_4_ack_frame_str
{
    // ---acknowledgement frame
}M802_15_4AckFrame;

// /**
// STRUCT      :: M802_15_4CommandFrame
// DESCRIPTION :: 802_15_4 MAC command Frame Header
// **/
typedef struct mac_802_15_4_cmd_frame_str
{
    // ---MAC command frame
    UInt8      MSDU_CmdType;       // (MSDU) Command Type/Command frame
                                   // identifier
                        // --0x01:  Association request
                        // --0x02:  Association response
                        // --0x03:  Disassociation notification
                        // --0x04:  Data request
                        // --0x05:  PAN ID conflict notification
                        // --0x06:  Orphan notification
                        // --0x07:  Beacon request
                        // --0x08:  Coordinator realignment
                        // --0x09:  GTS request
                        // --0x0a-0xff: Reserved
    // MSDU_CmdPL;               // (MSDU) Command Payload
    UInt16     MFR_FCS;        // same as above
}M802_15_4CommandFrame;

// /**
// STRUCT      :: M802_15_4Header
// DESCRIPTION :: 802_15_4 MAC header
// **/
typedef struct mac_802_15_4_header_str
{
    // ---MAC header---
    UInt16     MHR_FrmCtrl;
    UInt8      MHR_BDSN;
    M802_15_4PanAddrInfo MHR_DstAddrInfo;
    M802_15_4PanAddrInfo MHR_SrcAddrInfo;
    UInt16     MHR_FCS;
    UInt8      MSDU_CmdType;
    UInt8      msduHandle;
    BOOL       indirect;
    UInt8      phyCurrentChannel;

    /*    // ---MAC sublayer---
    UInt16     MFR_FCS;
    UInt16     MSDU_SuperSpec;
    M802_15_4GTSFields   MSDU_GTSFields;
    M802_15_4PendAddrField  MSDU_PendAddrFields;
    UInt8      MSDU_CmdType;
    UInt8      MSDU_PayloadLen;
    UInt16     pad;
    UInt8      MSDU_Payload[aMaxMACFrameSize];
    BOOL       SecurityUse;
    UInt8      ACLEntry;*/
}M802_15_4Header;


// --------------------------------------------------------------------------
//                       Layer structure
// --------------------------------------------------------------------------

// /**
// STRUCT      :: M802_15_4Statistics
// DESCRIPTION :: Statistics of MAC802.15.4
// **/
typedef struct mac_802_15_4_stats_str
{
    // Data related
    UInt32 numDataPktSent;
    UInt32 numDataPktRecd;

    // Mgmt related
    UInt32 numDataReqSent;
    UInt32 numDataReqRecd;
    UInt32 numAssociateReqSent;
    UInt32 numAssociateReqRecd;
    UInt32 numAssociateResSent;
    UInt32 numAssociateResRecd;
    UInt32 numDisassociateReqSent;
    UInt32 numDisassociateReqRecd;
    UInt32 numOrphanIndRecd;
    UInt32 numOrphanResSent;
    UInt32 numBeaconsSent;
    UInt32 numBeaconsReqSent;
    UInt32 numBeaconsReqRecd;
    UInt32 numBeaconsRecd;
    UInt32 numPollReqSent;

    UInt32 numGtsAllocationReqSent;
    UInt32 numGtsDeAllocationReqSent;
    UInt32 numGtsReqRetried;
    UInt32 numGtsReqRecd;
    UInt32 numGtsDeAllocationReqRecd;
    UInt32 numDataPktsSentInGts;
    UInt32 numDataPktsQueuedForGts;
    UInt32 numDataPktsDeQueuedForGts;
    UInt32 numGtsReqIgnored;
    UInt32 numGtsExpired;
    UInt32 numGtsRequestsRejectedByPanCoord;
    UInt32 numGtsAllocationConfirmedByPanCoord;
    UInt32 numDataPktsQueuedForCap;
    UInt32 numDataPktsDeQueuedForCap;
    // common
    UInt32 numAckSent;
    UInt32 numAckRecd;
    UInt32 numPktDropped;
    UInt32 numDataPktDroppedNoAck;
    UInt32 numDataPktRetriedForNoAck;
    UInt32 numDataPktDroppedChannelAccessFailure;
}M802_15_4Statistics;


// /**
// STRUCT      :: gts_request_parameters
// DESCRIPTION :: GTS request paramters.
// **/
typedef struct gts_request_parameters
{
    BOOL receiveOnly;
    BOOL allocationRequest;
    UInt8 numSlots;
    BOOL active;
}GtsRequestData;


// /**
// STRUCT      :: MacData802_15_4
// DESCRIPTION :: Layer structure of MAC802.15.4
// **/
typedef struct mac_802_15_4_str
{
    MacData* myMacData;
    M802_15_4PIB mpib;
    PHY_PIB tmp_ppib;
    M802_15_4DevCapability capability;    // device capability

    // --- for beacon ---
    // (most are temp. variables which should be populated before being used)
    BOOL secuBeacon;
    M802_15_4SuperframeSpec sfSpec,sfSpec2,sfSpec3;  // superframe spec
    M802_15_4GTSSpec gtsSpec,gtsSpec2, gtsSpec3;      // GTS specification
    M802_15_4PendAddrSpec pendAddrSpec;      // pending address specification
    UInt8 beaconPeriods,beaconPeriods2;    // # of backoff periods it takes
                                           // to transmit the beacon
    M802_15_4PanEle panDes,panDes2;         // PAN descriptor
    Message* rxBeacon;           // the beacon packet just received
    clocktype  macBcnTxTime;           // the time last beacon sent (in symbol)
    clocktype  macBcnRxTime;   // the time last beacon received
                            //  from within PAN (in clocktype)
    clocktype macBcnRxLastTime;
    clocktype  macBcnOtherRxTime;  // the time last beacon received
                                // from outside PAN (in symbol)
    UInt8  macBeaconOrder2;
    UInt8  macSuperframeOrder2;
    UInt8  macBeaconOrder3;
    UInt8  macSuperframeOrder3;
    UInt8  macBeaconOrder_last;

    BOOL    oneMoreBeacon;
    UInt8  numLostBeacons;         // # of beacons lost in a row
    // ------------------
    UInt16 rt_myNodeID;

    UInt8 energyLevel;

    MACADDR aExtendedAddress;

    BOOL isPANCoor;         // is a PAN coordinator?
    BOOL isCoor;            // is a coordinator?
    BOOL isCoorInDeviceMode;
    BOOL isSyncLoss;        // has loss the synchronization with CO?

//     Phy802_15_4 *phy;
    void* csma;
    void* sscs;

    PLMEsetTrxState trx_state_req;      // tranceiver state required:
    BOOL inTransmission;        // in the middle of transmission
    BOOL beaconWaiting;     // it's about time to transmit beacon (suppress
                            // all other transmissions)
    Message* txBeacon;      // beacon packet to be transmitted (w/o using
                            // CSMA-CA)
    Message* txAck;          // ack. packet to be transmitted (no waiting)
    Message* txBcnCmd;       // beacon or command packet waiting for
        // transmission (using CSMA-CA) -- triggered by receiving a packet
    Message* txBcnCmd2;      // beacon or command packet waiting for
        // transmission (using CSMA-CA) -- triggered by upper layer
    Message* txData;        // data packet waiting for transmission
                            // (using    CSMA-CA)
    Message* txCsmaca;      // for which packet (txBcnCmd/txBcnCmd2/txData)
                            // is CSMA-CA performed
    Message* txPkt;         // packet (any type) currently being transmitted
    Message* rxData;        // data packet received (waiting for passing up)
    Message* rxCmd;         // command packet received (will be handled after
                            // the transmission of ack.)
    UInt32 txPkt_uid;      // for debug purpose
    clocktype rxDataTime;      // the time when data packet received by MAC
    BOOL waitBcnCmdAck;     // only used if (txBcnCmd): waiting for an ack
    BOOL waitBcnCmdAck2;    // only used if (txBcnCmd2): waiting for an ack
    BOOL waitDataAck;       // only used if (txData): waiting for an ack
    UInt8 backoffStatus;       // 0=no backoff yet;1=backoff
            // successful;2=backoff failed;99=in the middle of backoff
    UInt8 numDataRetry;         // # of retries (retransmissions) for data
                                // packet
    UInt8 numBcnCmdRetry;       // # of retries (retransmissions) for beacon
                                // or command packet
    UInt8 numBcnCmdRetry2;      // # of retries (retransmissions) for beacon
                                // or command packet

    // seed for generating random backoff
    RandomSeed seed;

    // Statistics
    M802_15_4Statistics stats;

    // Timers
    Message* txOverT;   // Transmission over timer
    Message* txT;       // Tx timer
    Message* extractT;     // Extraction timer
    Message* assoRspWaitT;     // Association resp timer
    Message* dataWaitT;       // Data wait
    Message* rxEnableT;       // Receive enable
    Message* scanT;               // Scan
    Message* bcnTxT;       // beacon transmission timer
    Message* bcnRxT;       // beacon reception timer
    Message* bcnSearchT;   // beacon search timer
    Message* txCmdDataT;
    Message* backoffBoundT;
    Message* orphanT;
    Message* IFST;   // Interframe space

    // MAC state
    M802_15_4TaskPending taskP;

    // for association and transaction
    M802_15_4DEVLINK* deviceLink1;
    M802_15_4DEVLINK* deviceLink2;
    M802_15_4TRANSLINK* transacLink1;
    M802_15_4TRANSLINK* transacLink2;

    BOOL isBroadCastPacket;
    BOOL isCalledAfterTransmittingBeacon;
    Queue* broadCastQueue;
    Message* broadcastT;
    BOOL isPollRequestPending;
    BOOL ifsTimerCalledAfterReceivingBeacon;

    BOOL sendGtsRequestToPancoord;
    BOOL sendGtsConfirmationPending;
    UInt8 sendGtsTrackCount;

    BOOL receiveGtsConfirmationPending;
    UInt8 receiveGtsTrackCount;

    UInt8 currentFinCap;
    Queue* capQueue;
    Message* txGts;
    UInt8 currentGtsPositionDesc;

    Message* gtsT;
    BOOL CheckPacketsForTransmission;
    GtsRequestData gtsRequestData;
    GtsRequestData gtsRequestPendingData;
    BOOL gtsRequestPending;
    BOOL gtsRequestExhausted;
    BOOL displayGtsStats;

    BOOL isDisassociationPending;
}MacData802_15_4;


// --------------------------------------------------------------------------
// FUNCTION DECLARATIONS
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// API functions between MAC and PHY
// --------------------------------------------------------------------------

// /**
// FUNCTION   :: Mac802_15_4PD_DATA_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of Data request
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + status         : PhyStatusType       : Status received from phy
// RETURN  :: None
// **/
void Mac802_15_4PD_DATA_confirm(Node* node,
                                Int32 interfaceIndex,
                                PhyStatusType status);

// /**
// FUNCTION   :: Mac802_15_4PLME_CCA_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of CCA request
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + status         : PhyStatusType       : Status received from phy
// RETURN  :: None
// **/
void Mac802_15_4PLME_CCA_confirm(Node* node,
                                 Int32 interfaceIndex,
                                 PhyStatusType status);

// /**
// FUNCTION   :: Mac802_15_4PLME_ED_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of ED request
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + status         : PhyStatusType       : Status received from phy
// + EnergyLevel    : UInt8         : Energy level
// RETURN  :: None
// **/
void Mac802_15_4PLME_ED_confirm(Node* node,
                                Int32 interfaceIndex,
                                PhyStatusType status,
                                UInt8 EnergyLevel);

// /**
// FUNCTION   :: Mac802_15_4PLME_GET_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of GET request
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + status         : PhyStatusType       : Status received from phy
// + PIBAttribute   : PPIBAenum     : Attribute id
// + PIBAttributeValue: PHY_PIB*    : Attribute value
// RETURN  :: None
// **/
void Mac802_15_4PLME_GET_confirm(Node* node,
                                 Int32 interfaceIndex,
                                 PhyStatusType status,
                                 PPIBAenum PIBAttribute,
                                 PHY_PIB* PIBAttributeValue);

// /**
// FUNCTION   :: Mac802_15_4PLME_SET_TRX_STATE_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of Set TRX request
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + status         : PhyStatusType : Status received from phy
// + toParent       : BOOL          : Communicating towards parent?
// RETURN  :: None
// **/
void Mac802_15_4PLME_SET_TRX_STATE_confirm(Node* node,
                                           Int32 interfaceIndex,
                                           PhyStatusType status,
                                           BOOL toParent = FALSE);

// /**
// FUNCTION   :: Mac802_15_4PLME_SET_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of GET request
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + status         : PhyStatusType       : Status received from phy
// + PIBAttribute   : PPIBAenum     : Attribute id
// RETURN  :: None
// **/
void Mac802_15_4PLME_SET_confirm(Node* node,
                                 Int32 interfaceIndex,
                                 PhyStatusType status,
                                 PPIBAenum PIBAttribute);

// --------------------------------------------------------------------------
// API functions between MAC and SSCS
// --------------------------------------------------------------------------

// /**
// FUNCTION   :: Mac802_15_4MCPS_DATA_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request data transfer from SSCS to peer entity
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + SrcAddrMode    : UInt8         : Source address mode
// + SrcPANId       : UInt16        : source PAN id
// + SrcAddr        : MACADDR    : Source address
// + DstAddrMode    : UInt8         : Destination address mode
// + DstPANId       : UInt16        : Destination PAN id
// + DstAddr        : MACADDR    : Destination Address
// + msduLength     : Int32         : MSDU length
// + msdu           : Message*      : MSDU
// + msduHandle     : UInt8         : Handle associated with MSDU
// + TxOptions      : UInt8         : Transfer options (3bits)
//                                    bit-1 = ack(1)/unack(0) tx
//                                    bit-2 = GTS(1)/CAP(0) tx
//                                    bit-3 = Indirect(1)/Direct(0) tx
// RETURN  :: None
// **/
void Mac802_15_4MCPS_DATA_request(Node* node,
                                  MacData802_15_4* M802_15_4,
                                  UInt8 SrcAddrMode,
                                  UInt16 SrcPANId,
                                  MACADDR SrcAddr,
                                  UInt8 DstAddrMode,
                                  UInt16 DstPANId,
                                  MACADDR DstAddr,
                                  Int32 msduLength,
                                  Message* msdu,
                                  UInt8 msduHandle,
                                  UInt8 TxOptions);

// /**
// FUNCTION   :: Mac802_15_4MCPS_DATA_indication
// LAYER      :: Mac
// PURPOSE    :: Primitive to indicate the transfer of a data SPDU to SSCS
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + SrcAddrMode    : UInt8         : Source address mode
// + SrcPANId       : UInt16        : source PAN id
// + SrcAddr        : MACADDR    : Source address
// + DstAddrMode    : UInt8         : Destination address mode
// + DstPANId       : UInt16        : Destination PAN id
// + DstAddr        : MACADDR    : Destination Address
// + msduLength     : Int32         : MSDU length
// + msdu           : Message*      : MSDU
// + mpduLinkQuality: UInt8         : LQI value measured during reception of
//                                    the MPDU
// + SecurityUse    : BOOL          : whether security is used
// + ACLEntry       : UInt8         : ACL entry
// RETURN  :: None
// **/
void Mac802_15_4MCPS_DATA_indication(Node* node,
                                     Int32 interfaceIndex,
                                     UInt8 SrcAddrMode,
                                     UInt16 SrcPANId,
                                     MACADDR SrcAddr,
                                     UInt8 DstAddrMode,
                                     UInt16 DstPANId,
                                     MACADDR DstAddr,
                                     Int32 msduLength,
                                     Message* msdu,
                                     UInt8 mpduLinkQuality,
                                     BOOL SecurityUse,
                                     UInt8 ACLEntry);

// /**
// FUNCTION   :: Mac802_15_4MCPS_PURGE_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request purging an MSDU from transaction queue
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + msduHandle     : UInt8         : Handle associated with MSDU
// RETURN  :: None
// **/
void Mac802_15_4MCPS_PURGE_request(Node* node,
                                   Int32 interfaceIndex,
                                   UInt8 msduHandle);

// /**
// FUNCTION   :: Mac802_15_4MLME_ASSOCIATE_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request an association with a coordinator
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + LogicalChannel : UInt8         : Channel on which attempt is to be done
// + CoordAddrMode  : UInt8         : Coordinator address mode
// + CoordPANId     : UInt16        : Coordinator PAN id
// + CoordAddress   : MACADDR    : Coordinator address
// + CapabilityInformation : UInt8  : capabilities of associating device
// + SecurityEnable : BOOL          : Whether enable security or not
// RETURN  :: None
// **/
void Mac802_15_4MLME_ASSOCIATE_request(Node* node,
                                       Int32 interfaceIndex,
                                       UInt8 LogicalChannel,
                                       UInt8 CoordAddrMode,
                                       UInt16 CoordPANId,
                                       MACADDR CoordAddress,
                                       UInt8 CapabilityInformation,
                                       BOOL SecurityEnable);

// /**
// FUNCTION   :: Mac802_15_4MLME_ASSOCIATE_response
// LAYER      :: Mac
// PURPOSE    :: Primitive to initiate a response from SSCS sublayer
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + DeviceAddress      : MACADDR    : Address of device requesting assoc
// + AssocShortAddress  : UInt16        : Short address allocated by coord
// + status             : M802_15_4_enum: Status of association attempt
// + SecurityEnable     : BOOL          : Whether enabled security or not
// RETURN  :: None
// **/
void Mac802_15_4MLME_ASSOCIATE_response(Node* node,
                                        Int32 interfaceIndex,
                                        MACADDR DeviceAddress,
                                        UInt16 AssocShortAddress,
                                        M802_15_4_enum status,
                                        BOOL SecurityEnable);

// /**
// FUNCTION   :: Mac802_15_4MLME_DISASSOCIATE_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to indicate coordinator intent to leave
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + DeviceAddress      : MACADDR    : Add of device requesting dis-assoc
// + DisassociateReason : UInt8         : Reason for disassociation
// + SecurityEnable     : BOOL          : Whether enabled security or not
// RETURN  :: None
// **/
void Mac802_15_4MLME_DISASSOCIATE_request(Node* node,
                                          Int32 interfaceIndex,
                                          MACADDR DeviceAddress,
                                          UInt8 DisassociateReason,
                                          BOOL SecurityEnable);

// /**
// FUNCTION   :: Mac802_15_4MLME_DISASSOCIATE_indication
// LAYER      :: Mac
// PURPOSE    :: Primitive to indicate reception of disassociation command
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + DeviceAddress      : MACADDR    : Add of device requesting dis-assoc
// + DisassociateReason : UInt8         : Reason for disassociation
// + SecurityUse        : BOOL          : Whether enabled security or not
// + ACLEntry           : UInt8         : ACL entry
// RETURN  :: None
// **/
void Mac802_15_4MLME_DISASSOCIATE_indication(Node* node,
                                             Int32 interfaceIndex,
                                             MACADDR DeviceAddress,
                                             UInt8 DisassociateReason,
                                             BOOL SecurityUse,
                                             UInt8 ACLEntry);

// /**
// FUNCTION   :: Mac802_15_4MLME_GET_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request info about PIB attribute
// PARAMETERS ::
// + node           : Node*                 : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + PIBAttribute   : M802_15_4_PIBA_enum   : PIB attribute id
// RETURN  :: None
// **/
void Mac802_15_4MLME_GET_request(Node* node,
                                 Int32 interfaceIndex,
                                 M802_15_4_PIBA_enum PIBAttribute);

// /**
// FUNCTION   :: Mac802_15_4MLME_GTS_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request to the PAN coordinator to allocate a
//               new GTS or to deallocate an existing GTS
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + GTSCharacteristics : UInt8         : characteristics of GTS req
// + SecurityEnable     : BOOL          : Whether enabled security or not
// RETURN  :: None
// **/
void Mac802_15_4MLME_GTS_request(Node* node,
                                 Int32 interfaceIndex,
                                 UInt8 GTSCharacteristics,
                                 BOOL SecurityEnable,
                                 PhyStatusType status);

// /**
// FUNCTION   :: Mac802_15_4MLME_GTS_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report the result of a GTS req
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + GTSCharacteristics : UInt8         : characteristics of GTS req
// + status             : M802_15_4_enum: status of GTS req
// RETURN  :: None
// **/
void Mac802_15_4MLME_GTS_confirm(Node* node,
                                 Int32 interfaceIndex,
                                 UInt8 GTSCharacteristics,
                                 M802_15_4_enum status);

// /**
// FUNCTION   :: Mac802_15_4MLME_GTS_indication
// LAYER      :: Mac
// PURPOSE    :: Primitive to indicates that a GTS has been allocated or that
//               a previously allocated GTS has been deallocated.
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + DevAddress         : UInt16        : Short address of device
// + GTSCharacteristics : UInt8         : characteristics of GTS req
// + SecurityUse        : BOOL          : Whether enabled security or not
// + ACLEntry           : UInt8         : ACL entry
// RETURN  :: None
// **/
void Mac802_15_4MLME_GTS_indication(Node* node,
                                    Int32 interfaceIndex,
                                    UInt16 DevAddress,
                                    UInt8 GTSCharacteristics,
                                    BOOL SecurityUse,
                                    UInt8 ACLEntry);

// /**
// FUNCTION   :: Mac802_15_4MLME_ORPHAN_response
// LAYER      :: Mac
// PURPOSE    :: Primitive to respond to the MLME-ORPHAN.indication primitive
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + OrphanAddress      : MACADDR    : Address of orphan device
// + ShortAddress       : UInt16        : Short address of device
// + AssociatedMember   : BOOL          : Associated or not
// + SecurityEnable     : BOOL          : Whether enabled security or not
// RETURN  :: None
// **/
void Mac802_15_4MLME_ORPHAN_response(Node* node,
                                     Int32 interfaceIndex,
                                     MACADDR OrphanAddress,
                                     UInt16 ShortAddress,
                                     BOOL AssociatedMember,
                                     BOOL SecurityEnable);

// /**
// FUNCTION   :: Mac802_15_4MLME_RESET_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request that the MLME performs a reset
// PARAMETERS ::
// + node           : Node* : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + SetDefaultPIB  : BOOL  : Whether to reset PIB to default
// RETURN  :: None
// **/
void Mac802_15_4MLME_RESET_request(Node* node,
                                   Int32 interfaceIndex,
                                   BOOL SetDefaultPIB);

// /**
// FUNCTION   :: Mac802_15_4MLME_RX_ENABLE_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request that the receiver is either enabled for
//               a finite period of time or disabled
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + DeferPermit    : BOOL      : If defer till next superframe permitted
// + RxOnTime       : UInt32    : No. of symbols from start of superframe
//                                after which receiver is enabled
// + RxOnDuration   : UInt32    : No. of symbols for which receiver is to be
//                                enabled
// RETURN  :: None
// **/
void Mac802_15_4MLME_RX_ENABLE_request(Node* node,
                                       Int32 interfaceIndex,
                                       BOOL DeferPermit,
                                       UInt32 RxOnTime,
                                       UInt32 RxOnDuration);

// /**
// FUNCTION   :: Mac802_15_4MLME_SCAN_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to initiate a channel scan over a given list of
//               channels
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + ScanType       : UInt8     : Type of scan (0-ED/Active/Passive/3-Orphan)
// + ScanChannels   : UInt32    : Channels to be scanned
// + ScanDuration   : UInt8     : Duration of scan, ignored for orphan scan
// RETURN  :: None
// **/
void Mac802_15_4MLME_SCAN_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 ScanType,
                                  UInt32 ScanChannels,
                                  UInt8 ScanDuration);

// /**
// FUNCTION   :: Mac802_15_4MLME_SET_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to set PIB attribute
// PARAMETERS ::
// + node               : Node*                 : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + PIBAttribute       : M802_15_4_PIBA_enum   : PIB attribute id
// + PIBAttributeValue  : M802_15_4PIB*         : Attribute value
// RETURN  :: None
// **/
void Mac802_15_4MLME_SET_request(Node* node,
                                 Int32 interfaceIndex,
                                 M802_15_4_PIBA_enum PIBAttribute,
                                 M802_15_4PIB* PIBAttributeValue);

// /**
// FUNCTION   :: Mac802_15_4MLME_START_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to allow the PAN coordinator to initiate a new PAN
//               or to begin using a new superframe configuration
// PARAMETERS ::
// + node                   : Node*     : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + PANId                  : UInt16    : PAN id
// + LogicalChannel         : UInt8     : Logical channel
// + BeaconOrder            : UInt8     : How often a beacon is tx'd.
// + SuperframeOrder        : UInt8     : Length of active portion of s-frame
// + PANCoordinator         : BOOL      : If device is PAN coordinator
// + BatteryLifeExtension   : BOOL      : for battery saving mode
// + CoordRealignment       : BOOL      : If coordinator realignment command
//                     needs to be sent prior to changing superframe config
// + SecurityEnable         : BOOL      : If security is enabled
// RETURN  :: None
// **/
void Mac802_15_4MLME_START_request(Node* node,
                                   Int32 interfaceIndex,
                                   UInt16 PANId,
                                   UInt8 LogicalChannel,
                                   UInt8 BeaconOrder,
                                   UInt8 SuperframeOrder,
                                   BOOL PANCoordinator,
                                   BOOL BatteryLifeExtension,
                                   BOOL CoordRealignment,
                                   BOOL SecurityEnable);

// /**
// FUNCTION   :: Mac802_15_4MLME_SYNC_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request synchronize with the coordinator
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + LogicalChannel : UInt8 : Logical channel
// + TrackBeacon    : BOOL  : Whether to synchronize with all future beacons
// RETURN  :: None
// **/
void Mac802_15_4MLME_SYNC_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 LogicalChannel,
                                  BOOL TrackBeacon);

// /**
// FUNCTION   :: Mac802_15_4MLME_POLL_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to prompt device to request data from coordinator
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + CoordAddrMode  : UInt8         : Coordinator address mode
// + CoordPANId     : UInt16        : Coordinator PAN id
// + CoordAddress   : MACADDR    : Coordinator address
// + SecurityEnable : BOOL          : Whether enable security or not
// RETURN  :: None
// **/
void Mac802_15_4MLME_POLL_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 CoordAddrMode,
                                  UInt16 CoordPANId,
                                  MACADDR CoordAddress,
                                  BOOL SecurityEnable);


Message* Mac802_15_4SetTimer(Node* node,
                             MacData802_15_4* mac802_15_4,
                             M802_15_4TimerType timerType,
                             clocktype delay,
                             Message* msg);

// /**
// FUNCTION     Mac802_15_4Init
// PURPOSE      Initialization function for 802.15.4 protocol of MAC layer
// PARAMETERS   Node* node
//                  Node being initialized.
//              NodeInput* nodeInput
//                  Structure containing contents of input file.
//              SubnetMemberData* subnetList
//                  Number of nodes in subnet.
//              Int32 nodesInSubnet
//                  Number of nodes in subnet.
//              Int32 subnetListIndex
//              NodeAddress subnetAddress
//                  Subnet address.
// RETURN       None
// NOTES        None
// **/
void Mac802_15_4Init(Node* node,
                     const NodeInput* nodeInput,
                     Int32 interfaceIndex,
                     NodeAddress subnetAddress,
                     SubnetMemberData* subnetList,
                     Int32 nodesInSubnet);

// /**
// FUNCTION     Mac802_15_4Layer
// PURPOSE      Models the behaviour of the MAC layer with the 802.15.4
//              protocol on receiving the message enclosed in msgHdr
// PARAMETERS   Node *node
//                  Node which received the message.
//              Int32 interfaceIndex
//                  Interface index.
//              Message* msg
//                  Message received by the layer.
// RETURN       None
// NOTES        None
// **/
void Mac802_15_4Layer(Node* node, Int32 interfaceIndex, Message* msg);


// /**
// FUNCTION     Mac802_15_4Finalize
// PURPOSE      Called at the end of simulation to collect the results of
//              the simulation of 802.15.4 protocol of the MAC Layer.
// PARAMETERS   Node* node
//                  Node which received the message.
//              Int32 interfaceIndex
//                  Interface index.
// RETURN       None
// NOTES        None
// **/
void Mac802_15_4Finalize(Node* node, Int32 interfaceIndex);


// /**
// NAME         Mac802_15_4NetworkLayerHasPacketToSend
// PURPOSE      To notify 802.15.4 that network has something to send
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacData_802_15_4* M802_15_4
//                  802.15.4 data structure
// RETURN       None
// NOTES        None
// **/
void Mac802_15_4NetworkLayerHasPacketToSend(Node* node,
                                            MacData802_15_4* M802_15_4);


// /**
// NAME         Mac802_15_4ReceivePacketFromPhy
// PURPOSE      To recieve packet from the physical layer
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacData_802_15_4* M802_15_4
//                  802.15.4 data structure
//              Message* msg
//                  Message received by the layer.
// RETURN       None.
// NOTES        None
// **/
void Mac802_15_4ReceivePacketFromPhy(Node* node,
                                     MacData802_15_4* M802_15_4,
                                     Message* msg);


// /**
// NAME         Mac802_15_4ReceivePhyStatusChangeNotification
// PURPOSE      Receive notification of status change in physical layer
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacData_802_15_4* M802_15_4
//                  802.15.4 data structure.
//              PhyStatusType oldPhyStatus
//                  The old physical status.
//              PhyStatusType newPhyStatus
//                  The new physical status.
//              clocktype receiveDuration
//                  The receiving duration.
// RETURN       None.
// NOTES        None
// **/
void Mac802_15_4ReceivePhyStatusChangeNotification(
    Node* node,
    MacData802_15_4* M802_15_4,
    PhyStatusType oldPhyStatus,
    PhyStatusType newPhyStatus,
    clocktype receiveDuration);



// --------------------------------------------------------------------------
// STATIC FUNCTIONS
// --------------------------------------------------------------------------

// /**
// FUNCTION   :: Mac802_15_4FrameCtrlParse
// LAYER      :: Mac
// PURPOSE    :: This function is called for parsing Frame Control fields.
// PARAMETERS ::
// + frameCtrl  : M802_15_4FrameCtrl*   : Pointer to Frame Control
// RETURN  :: None.
// **/
static
void Mac802_15_4FrameCtrlParse(M802_15_4FrameCtrl* frameCtrl)
{
    frameCtrl->frmType = (UInt8)((frameCtrl->frmCtrl & 0xe000) >> 13);
    // taking deviation from coding guidelines and using ternary operator
    frameCtrl->secu = ((frameCtrl->frmCtrl & 0x1000) == 0)?FALSE:TRUE;
    frameCtrl->frmPending = ((frameCtrl->frmCtrl & 0x0800) == 0)?FALSE:TRUE;
    frameCtrl->ackReq = ((frameCtrl->frmCtrl & 0x0400) == 0)?FALSE:TRUE;
    frameCtrl->intraPan = ((frameCtrl->frmCtrl & 0x0200) == 0)?FALSE:TRUE;
    frameCtrl->dstAddrMode = (UInt8)((frameCtrl->frmCtrl & 0x0030) >> 4);
    frameCtrl->srcAddrMode = (UInt8)(frameCtrl->frmCtrl & 0x0003);
}

// /**
// FUNCTION   :: Mac802_15_4FrameCtrlSetFrmType
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting Frame Type.
// PARAMETERS ::
// + frameCtrl  : M802_15_4FrameCtrl*   : Pointer to Frame Control
// + frmType    : UInt8                 : Frame type
// RETURN  :: None.
// **/
static
void Mac802_15_4FrameCtrlSetFrmType(M802_15_4FrameCtrl* frameCtrl,
                                    UInt8 frmType)
{
    frameCtrl->frmType = frmType;
    frameCtrl->frmCtrl = (frameCtrl->frmCtrl & 0x1fff) + (frmType << 13);
}

// /**
// FUNCTION   :: Mac802_15_4FrameCtrlSetSecu
// LAYER      :: Mac
// PURPOSE    :: TThis function is called for setting security.
// PARAMETERS ::
// + frameCtrl  : M802_15_4FrameCtrl*   : Pointer to Frame Control
// + sc         : BOOL                  : Security enabled or disabled
// RETURN  :: None.
// **/
static
void Mac802_15_4FrameCtrlSetSecu(M802_15_4FrameCtrl* frameCtrl, BOOL sc)
{
    frameCtrl->secu = sc;
    frameCtrl->frmCtrl = (frameCtrl->frmCtrl & 0xefff);
    if (sc == TRUE)
    {
        frameCtrl->frmCtrl += 0x1000;
    }
}

// /**
// FUNCTION   :: Mac802_15_4FrameCtrlSetFrmPending
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting Pending flag.
// PARAMETERS ::
// + frameCtrl  : M802_15_4FrameCtrl*   : Pointer to Frame Control
// + pending    : BOOL                  : Pending true or not
// RETURN  :: None.
// **/
static
void Mac802_15_4FrameCtrlSetFrmPending(M802_15_4FrameCtrl* frameCtrl,
                                       BOOL pending)
{
    frameCtrl->frmPending = pending;
    frameCtrl->frmCtrl = (frameCtrl->frmCtrl & 0xf7ff);
    if (pending == TRUE)
    {
        frameCtrl->frmCtrl += 0x0800;
    }
}

// /**
// FUNCTION   :: Mac802_15_4FrameCtrlSetAckReq
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting ACK flag.
// PARAMETERS ::
// + frameCtrl  : M802_15_4FrameCtrl*   : Pointer to Frame Control
// + ack        : BOOL                  : ACK true or not
// RETURN  :: None.
// **/
static
void Mac802_15_4FrameCtrlSetAckReq(M802_15_4FrameCtrl* frameCtrl, BOOL ack)
{
    frameCtrl->ackReq = ack;
    frameCtrl->frmCtrl = (frameCtrl->frmCtrl & 0xfbff);
    if (ack == TRUE)
    {
        frameCtrl->frmCtrl += 0x0400;
    }
}

// /**
// FUNCTION   :: Mac802_15_4FrameCtrlSetIntraPan
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting IntraPAN flag.
// PARAMETERS ::
// + frameCtrl  : M802_15_4FrameCtrl*   : Pointer to Frame Control
// + intrapan   : BOOL                  : Intra-PAN true or not
// RETURN  :: None.
// **/
static
void Mac802_15_4FrameCtrlSetIntraPan(M802_15_4FrameCtrl* frameCtrl,
                                     BOOL intrapan)
{
    frameCtrl->intraPan = intrapan;
    frameCtrl->frmCtrl = (frameCtrl->frmCtrl & 0xfdff);
    if (intrapan)
    {
        frameCtrl->frmCtrl += 0x0200;
    }
}

// /**
// FUNCTION   :: Mac802_15_4FrameCtrlSetDstAddrMode
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting destination addr mode.
// PARAMETERS ::
// + frameCtrl  : M802_15_4FrameCtrl*   : Pointer to Frame Control
// + dstmode    : UInt8                 : Destination mode
// RETURN  :: None.
// **/
static
void Mac802_15_4FrameCtrlSetDstAddrMode(M802_15_4FrameCtrl* frameCtrl,
                                        UInt8 dstmode)
{
    frameCtrl->dstAddrMode = dstmode;
    frameCtrl->frmCtrl = (frameCtrl->frmCtrl & 0xffcf) + (dstmode << 4);
}

// /**
// FUNCTION   :: Mac802_15_4FrameCtrlSetSrcAddrMode
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting source address mode.
// PARAMETERS ::
// + frameCtrl  : M802_15_4FrameCtrl*   : Pointer to Frame Control
// + srcmode    : UInt8                 : Source mode
// RETURN  :: None.
// **/
static
void Mac802_15_4FrameCtrlSetSrcAddrMode(M802_15_4FrameCtrl* frameCtrl,
                                        UInt8 srcmode)
{
    frameCtrl->srcAddrMode = srcmode;
    frameCtrl->frmCtrl = (frameCtrl->frmCtrl & 0xfffc) + srcmode;
}

// /**
// FUNCTION   :: Mac802_15_4SuperFrameParse
// LAYER      :: Mac
// PURPOSE    :: This function is called for parsing Super Frame fields.
// PARAMETERS ::
// + frameCtrl  : M802_15_4SuperframeSpec*   : Pointer to Super Frame Spec
// RETURN  :: None.
// **/
static
void Mac802_15_4SuperFrameParse(M802_15_4SuperframeSpec* superSpec)
{
    superSpec->BO = (UInt8)((superSpec->superSpec & 0xf000) >> 12);
    superSpec->BI = aBaseSuperframeDuration * (1 << superSpec->BO);
    superSpec->SO = (UInt8)((superSpec->superSpec & 0x0f00) >> 8);
    superSpec->SD = aBaseSuperframeDuration * (1 << superSpec->SO);// duration
    superSpec->sd = aBaseSlotDuration * (1 << superSpec->SO);
    superSpec->FinCAP = (UInt8)((superSpec->superSpec & 0x00f0) >> 4);
    superSpec->BLE = ((superSpec->superSpec & 0x0008) == 0)?FALSE:TRUE;
    superSpec->PANCoor = ((superSpec->superSpec & 0x0002) == 0)?FALSE:TRUE;
    superSpec->AssoPmt = ((superSpec->superSpec & 0x0001) == 0)?FALSE:TRUE;
}

// /**
// FUNCTION   :: Mac802_15_4SuperFrameSetBO
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting BO.
// PARAMETERS ::
// + frameCtrl  : M802_15_4SuperframeSpec*  : Pointer to Super Frame Spec
// + bo         : UInt8                     : BO value
// RETURN  :: None.
// **/
static
void Mac802_15_4SuperFrameSetBO(M802_15_4SuperframeSpec* superSpec,
                                UInt8 bo)
{
    superSpec->BO = bo;
    superSpec->BI = aBaseSuperframeDuration * (1 << superSpec->BO);
    superSpec->superSpec = (superSpec->superSpec & 0x0fff) + (bo << 12);
}

// /**
// FUNCTION   :: Mac802_15_4SuperFrameSetSO
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting SO.
// PARAMETERS ::
// + frameCtrl  : M802_15_4SuperframeSpec*  : Pointer to Super Frame Spec
// + so         : UInt8                     : SO value
// RETURN  :: None.
// **/
static
void Mac802_15_4SuperFrameSetSO(M802_15_4SuperframeSpec* superSpec,
                                UInt8 so)
{
    superSpec->SO = so;
    superSpec->SD = aBaseSuperframeDuration * (1 << superSpec->SO);
    superSpec->sd = aBaseSlotDuration * (1 << superSpec->SO);
    superSpec->superSpec = (superSpec->superSpec & 0xf0ff) + (so << 8);
}

// /**
// FUNCTION   :: Mac802_15_4SuperFrameSetFinCAP
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting CAP.
// PARAMETERS ::
// + frameCtrl  : M802_15_4SuperframeSpec*  : Pointer to Super Frame Spec
// + fincap     : UInt8                     : fincap value
// RETURN  :: None.
// **/
static
void Mac802_15_4SuperFrameSetFinCAP(M802_15_4SuperframeSpec* superSpec,
                                    UInt8 fincap)
{
    superSpec->FinCAP = fincap;
    superSpec->superSpec = (superSpec->superSpec & 0xff0f) + (fincap << 4);
}

// /**
// FUNCTION   :: Mac802_15_4SuperFrameSetBLE
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting BLE.
// PARAMETERS ::
// + frameCtrl  : M802_15_4SuperframeSpec*  : Pointer to Super Frame Spec
// + ble        : BOOL                     : BLE value
// RETURN  :: None.
// **/
static
void Mac802_15_4SuperFrameSetBLE(M802_15_4SuperframeSpec* superSpec,
                                 BOOL ble)
{
    superSpec->BLE = ble;
    superSpec->superSpec = (superSpec->superSpec & 0xfff7);
    if (ble == TRUE)
    {
        superSpec->superSpec += 8;
    }
}

// /**
// FUNCTION   :: Mac802_15_4SuperFrameSetPANCoor
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting whether PAN coordinator.
// PARAMETERS ::
// + frameCtrl  : M802_15_4SuperframeSpec*  : Pointer to Super Frame Spec
// + pancoor    : BOOL                      : PAN Coordinator
// RETURN  :: None.
// **/
static
void Mac802_15_4SuperFrameSetPANCoor(M802_15_4SuperframeSpec* superSpec,
                                     BOOL pancoor)
{
    superSpec->PANCoor = pancoor;
    superSpec->superSpec = (superSpec->superSpec & 0xfffd);
    if (pancoor == TRUE)
    {
        superSpec->superSpec += 2;
    }
}

// /**
// FUNCTION   :: Mac802_15_4SuperFrameSetAssoPmt
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting whether Association is
//                  permitted
// PARAMETERS ::
// + frameCtrl  : M802_15_4SuperframeSpec*  : Pointer to Super Frame Spec
// + assopmt    : BOOL                      : Association permitted
// RETURN  :: None.
// **/
static
void Mac802_15_4SuperFrameSetAssoPmt(M802_15_4SuperframeSpec* superSpec,
                                     BOOL assopmt)
{
    superSpec->AssoPmt = assopmt;
    superSpec->superSpec = (superSpec->superSpec & 0xfffe);
    if (assopmt == TRUE)
    {
        superSpec->superSpec += 1;
    }
}


// /**
// FUNCTION   :: Mac802_15_4GTSSpecParse
// LAYER      :: Mac
// PURPOSE    :: This function is called for parsing GTS specification
// PARAMETERS ::
// + gtsSpec    : M802_15_4GTSSpec*         : Pointer to GTS Spec
// RETURN  :: None.
// **/
static
void Mac802_15_4GTSSpecParse(M802_15_4GTSSpec* gtsSpec)
{
    Int32 i = 0;
    for (i = 0; i < gtsSpec->count; i++)
    {
        // reset the existing gts info
        gtsSpec->recvOnly[i] = 0;
        gtsSpec->slotStart[i] = 0;
        gtsSpec->length[i] = 0;
    }
    gtsSpec->count = (gtsSpec->fields.spec & 0xe0) >> 5;
    gtsSpec->permit = ((gtsSpec->fields.spec & 0x01) != 0)?TRUE:FALSE;
    for (Int32 i=0; i < gtsSpec->count; i++)
    {
        gtsSpec->recvOnly[i] = ((gtsSpec->fields.dir & (1<<(7-i))) != 0);
        gtsSpec->slotStart[i]
            = (gtsSpec->fields.list[i].slotSpec & 0xf0) >> 4;
        gtsSpec->length[i] = (gtsSpec->fields.list[i].slotSpec & 0x0f);
    }
}

// /**
// FUNCTION   :: Mac802_15_4GTSSpecSetCount
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting GTS descriptor count
// PARAMETERS ::
// + gtsSpec    : M802_15_4GTSSpec*         : Pointer to GTS Spec
// + cnt        : UInt8                     : Count
// RETURN  :: None.
// **/
static
void Mac802_15_4GTSSpecSetCount(M802_15_4GTSSpec* gtsSpec,
                                UInt8 cnt)
{
    gtsSpec->count = cnt;
    gtsSpec->fields.spec = (gtsSpec->fields.spec & 0x1f) + (cnt << 5);
}


// /**
// FUNCTION   :: Mac802_15_4GTSSpecSetPermit
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting GTS permit
// PARAMETERS ::
// + gtsSpec    : M802_15_4GTSSpec*         : Pointer to GTS Spec
// + pmt        : BOOL                      : Permit
// RETURN  :: None.
// **/
static
void Mac802_15_4GTSSpecSetPermit(M802_15_4GTSSpec* gtsSpec,
                                 BOOL pmt)
{
    gtsSpec->permit = pmt;
    gtsSpec->fields.spec = (gtsSpec->fields.spec & 0xfe);
    if (pmt == TRUE)
    {
        gtsSpec->fields.spec += 1;
    }
}

// /**
// FUNCTION   :: Mac802_15_4GTSSpecSetRecvOnly
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting receive only on ith slot
// PARAMETERS ::
// + gtsSpec    : M802_15_4GTSSpec*         : Pointer to GTS Spec
// + ith        : UInt8                     : ith slot
// + rvonly     : BOOL                      : Receive only?
// RETURN  :: None.
// **/
static
void Mac802_15_4GTSSpecSetRecvOnly(M802_15_4GTSSpec* gtsSpec,
                                   UInt8 ith,
                                   BOOL rvonly)
{
    gtsSpec->recvOnly[ith] = rvonly;
    gtsSpec->fields.dir = gtsSpec->fields.dir & ((1<<(7-ith))^0xff);
    if (rvonly == TRUE)
    {
        gtsSpec->fields.dir += (1<<(7-ith));
    }
}

// /**
// FUNCTION   :: Mac802_15_4GTSSpecSetSlotStart
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting slot start on ith slot
// PARAMETERS ::
// + gtsSpec    : M802_15_4GTSSpec*         : Pointer to GTS Spec
// + ith        : UInt8                     : ith slot
// + st         : UInt8                     : Start
// RETURN  :: None.
// **/
static
void Mac802_15_4GTSSpecSetSlotStart(M802_15_4GTSSpec* gtsSpec,
                                    UInt8 ith,
                                    UInt8 st)
{
    gtsSpec->slotStart[ith] = st;
    gtsSpec->fields.list[ith].slotSpec =
            (gtsSpec->fields.list[ith].slotSpec & 0x0f) + (st << 4);
}

// /**
// FUNCTION   :: Mac802_15_4GTSSpecSetLength
// LAYER      :: Mac
// PURPOSE    :: This function is called for setting slot length on ith slot
// PARAMETERS ::
// + gtsSpec    : M802_15_4GTSSpec*         : Pointer to GTS Spec
// + ith        : UInt8                     : ith slot
// + len        : UInt8                     : Length
// RETURN  :: None.
// **/
static
void Mac802_15_4GTSSpecSetLength(M802_15_4GTSSpec* gtsSpec,
                                 UInt8 ith,
                                 UInt8 len)
{
    gtsSpec->length[ith] = len;
    gtsSpec->fields.list[ith].slotSpec
        = (gtsSpec->fields.list[ith].slotSpec & 0xf0) + len;
}

// /**
// FUNCTION   :: Mac802_15_4GTSSpecSize
// LAYER      :: Mac
// PURPOSE    :: This function is called for getting GTS size
// PARAMETERS ::
// + gtsSpec    : M802_15_4GTSSpec*         : Pointer to GTS Spec
// RETURN  :: Int32   : GTS size
// **/
static
Int32 Mac802_15_4GTSSpecSize(M802_15_4GTSSpec* gtsSpec)
{
    gtsSpec->count = (gtsSpec->fields.spec & 0xe0) >> 5;
    return 1 + 1 + 3 * gtsSpec->count;
}

// /**
// FUNCTION   :: Mac802_15_4PendAddSpecAddShortAddr
// LAYER      :: Mac
// PURPOSE    :: Add new short address to pending list
// PARAMETERS ::
// + pendAddSpec    : M802_15_4PendAddrSpec*    : Pointer to Pending Addr
// + sa             : UInt16                    : Short Address
// RETURN  :: UInt8 : Number of Short and extended addresses in list
// **/
static
UInt8 Mac802_15_4PendAddSpecAddShortAddr(M802_15_4PendAddrSpec* pendAddSpec,
                                         MACADDR sa)
{
    Int32 i = 0;
    if (pendAddSpec->numShortAddr + pendAddSpec->numExtendedAddr >= 7)
    {
        return pendAddSpec->numShortAddr + pendAddSpec->numExtendedAddr;
    }

    // only unique address added
    for (i = 0; i < pendAddSpec->numShortAddr; i++)
    {
        if (pendAddSpec->fields.addrList[i] == sa)
        {
            return pendAddSpec->numShortAddr + pendAddSpec->numExtendedAddr;
        }
    }

    pendAddSpec->fields.addrList[pendAddSpec->numShortAddr] = sa;
    pendAddSpec->numShortAddr++;
    return pendAddSpec->numShortAddr + pendAddSpec->numExtendedAddr;
}

// /**
// FUNCTION   :: Mac802_15_4PendAddSpecAddExtendedAddr
// LAYER      :: Mac
// PURPOSE    :: Add new extended address to pending list
// PARAMETERS ::
// + pendAddSpec    : M802_15_4PendAddrSpec*    : Pointer to Pending Addr
// + ea             : MACADDR                : Extended Address
// RETURN  :: UInt8 : Number of Short and extended addresses in list
// **/
static
UInt8 Mac802_15_4PendAddSpecAddExtendedAddr(
    M802_15_4PendAddrSpec* pendAddSpec,
    MACADDR ea)
{
    Int32 i = 0;

    if (pendAddSpec->numShortAddr + pendAddSpec->numExtendedAddr >= 7)
    {
        return pendAddSpec->numShortAddr + pendAddSpec->numExtendedAddr;
    }
        // only unique address added
    for (i = 6;i > 6 - pendAddSpec->numExtendedAddr; i--)
    {
        if (pendAddSpec->fields.addrList[i] == ea)
        {
            return pendAddSpec->numShortAddr + pendAddSpec->numExtendedAddr;
        }
    }
        // save the extended address in reverse order
    pendAddSpec->fields.addrList[6 - pendAddSpec->numExtendedAddr] = ea;
    pendAddSpec->numExtendedAddr++;
    return pendAddSpec->numShortAddr + pendAddSpec->numExtendedAddr;
}

// /**
// FUNCTION   :: Mac802_15_4PendAddSpecFormat
// LAYER      :: Mac
// PURPOSE    :: Realign Pending Address Specification
// PARAMETERS ::
// + pendAddSpec    : M802_15_4PendAddrSpec*    : Pointer to Pending Addr
// RETURN  :: None
// **/
static
void Mac802_15_4PendAddSpecFormat(M802_15_4PendAddrSpec* pendAddSpec)
{
    Int32 i = 0;
    MACADDR tmpAddr;

    // restore the order of extended addresses
    for (i = 0; i < pendAddSpec->numExtendedAddr; i++)
    {
        if ((7 - pendAddSpec->numExtendedAddr + i) < (6 - i))
        {
            tmpAddr
                = pendAddSpec->fields.addrList[7 -
                                        pendAddSpec->numExtendedAddr + i];
            pendAddSpec->fields.addrList[7 - pendAddSpec->numExtendedAddr + i]
                = pendAddSpec->fields.addrList[6 - i];
            pendAddSpec->fields.addrList[6 - i] = tmpAddr;
        }
    }

    // attach the extended addresses to short addresses
    for (i = 0; i < pendAddSpec->numExtendedAddr; i++)
    {
        pendAddSpec->fields.addrList[pendAddSpec->numShortAddr + i]
        = pendAddSpec->fields.addrList[7 - pendAddSpec->numExtendedAddr + i];
    }

    // update address specification
    pendAddSpec->fields.spec
        = ((pendAddSpec->numShortAddr) << 5)
            + (pendAddSpec->numExtendedAddr << 1);
}

// /**
// FUNCTION   :: Mac802_15_4PendAddSpecParse
// LAYER      :: Mac
// PURPOSE    :: Parsing the Pending address spec
// PARAMETERS ::
// + pendAddSpec    : M802_15_4PendAddrSpec*    : Pointer to Pending Addr
// RETURN  :: None
// **/
static
void Mac802_15_4PendAddSpecParse(M802_15_4PendAddrSpec* pendAddSpec)
{
    pendAddSpec->numShortAddr = (pendAddSpec->fields.spec & 0xe0) >> 5;
    pendAddSpec->numExtendedAddr = (pendAddSpec->fields.spec & 0x0e) >> 1;
}

#if 0
// /**
// FUNCTION   :: Mac802_15_4PendAddSpecSize
// LAYER      :: Mac
// PURPOSE    :: Add Pending Address spec size
// PARAMETERS ::
// + pendAddSpec    : M802_15_4PendAddrSpec*    : Pointer to Pending Addr
// RETURN  :: Int32   :
// **/
static
Int32 Mac802_15_4PendAddSpecSize(M802_15_4PendAddrSpec* pendAddSpec)
{
    Mac802_15_4PendAddSpecParse(pendAddSpec);
    return 1 + pendAddSpec->numShortAddr * 2
                + pendAddSpec->numExtendedAddr * 8;
}
#endif

// /**
// FUNCTION   :: Mac802_15_4DevCapParse
// LAYER      :: Mac
// PURPOSE    :: Parses Device capabilities
// PARAMETERS ::
// + devCap    : M802_15_4DevCapability*    : Pointer to Pending Addr
// RETURN  :: None
// **/
static
void Mac802_15_4DevCapParse(M802_15_4DevCapability* devCap)
{
    devCap->alterPANCoor = ((devCap->cap & 0x80) != 0)?TRUE:FALSE;
    devCap->FFD = ((devCap->cap & 0x40) != 0)?TRUE:FALSE;
    devCap->mainsPower = ((devCap->cap & 0x20) != 0)?TRUE:FALSE;
    devCap->recvOnWhenIdle = ((devCap->cap & 0x10) != 0)?TRUE:FALSE;
    devCap->secuCapable = ((devCap->cap & 0x02) != 0)?TRUE:FALSE;
    devCap->alloShortAddr = ((devCap->cap & 0x01) != 0)?TRUE:FALSE;
}

#if 0
// /**
// FUNCTION   :: Mac802_15_4DevCapSetAlterPANCoor
// LAYER      :: Mac
// PURPOSE    :: Sets whether alternate PAN coordinator
// PARAMETERS ::
// + devCap     : M802_15_4DevCapability*   : Pointer to Pending Addr
// + alterPC    : BOOL                      : Alternate PAN Coordinator
// RETURN  :: None
// **/
static
void Mac802_15_4DevCapSetAlterPANCoor(M802_15_4DevCapability* devCap,
                                      BOOL alterPC)
{
    devCap->alterPANCoor = alterPC;
    devCap->cap = (devCap->cap & 0x7f);
    if (alterPC == TRUE)
    {
        devCap->cap += 0x80;
    }
}

// /**
// FUNCTION   :: Mac802_15_4DevCapSetFFD
// LAYER      :: Mac
// PURPOSE    :: Sets whether Full function device
// PARAMETERS ::
// + devCap     : M802_15_4DevCapability*   : Pointer to Pending Addr
// + ffd        : BOOL                      : FFD
// RETURN  :: None
// **/
static
void Mac802_15_4DevCapSetFFD(M802_15_4DevCapability* devCap, BOOL ffd)
{
    devCap->FFD = ffd;
    devCap->cap = (devCap->cap & 0xbf);
    if (ffd == TRUE)
    {
        devCap->cap += 0x40;
    }
}

// /**
// FUNCTION   :: Mac802_15_4DevCapSetMainPower
// LAYER      :: Mac
// PURPOSE    :: Sets whether on main power (or battery)
// PARAMETERS ::
// + devCap     : M802_15_4DevCapability*   : Pointer to Pending Addr
// + mainsPowered : BOOL                    : Main power
// RETURN  :: None
// **/
static
void Mac802_15_4DevCapSetMainPower(M802_15_4DevCapability* devCap,
                                   BOOL mainsPowered)
{
    devCap->mainsPower = mainsPowered;
    devCap->cap = (devCap->cap & 0xdf);
    if (mainsPowered == TRUE)
    {
        devCap->cap += 0x20;
    }
}

// /**
// FUNCTION   :: Mac802_15_4DevCapSetRecvOnWhenIdle
// LAYER      :: Mac
// PURPOSE    :: Sets whether device receives on idle
// PARAMETERS ::
// + devCap     : M802_15_4DevCapability*   : Pointer to Pending Addr
// + onIdle     : BOOL                      : Recv on idle
// RETURN  :: None
// **/
static
void Mac802_15_4DevCapSetRecvOnWhenIdle(M802_15_4DevCapability* devCap,
                                        BOOL onIdle)
{
    devCap->recvOnWhenIdle = onIdle;
    devCap->cap = (devCap->cap & 0xef);
    if (onIdle == TRUE)
    {
        devCap->cap += 0x10;
    }
}

// /**
// FUNCTION   :: Mac802_15_4DevCapSetSecuCapable
// LAYER      :: Mac
// PURPOSE    :: Sets whether security capabilities are enabled
// PARAMETERS ::
// + devCap     : M802_15_4DevCapability*   : Pointer to Pending Addr
// + securityEnable : BOOL                  : Security capability
// RETURN  :: None
// **/
static
void Mac802_15_4DevCapSetSecuCapable(M802_15_4DevCapability* devCap,
                                     BOOL securityEnable)
{
    devCap->secuCapable = securityEnable;
    devCap->cap = (devCap->cap & 0xfd);
    if (securityEnable == TRUE)
    {
        devCap->cap += 0x02;
    }
}

// /**
// FUNCTION   :: Mac802_15_4DevCapSetAlloShortAddr
// LAYER      :: Mac
// PURPOSE    :: Sets whether coordinator should allocate short address
// PARAMETERS ::
// + devCap     : M802_15_4DevCapability*   : Pointer to Pending Addr
// + alloc      : BOOL                      : Allocate short address
// RETURN  :: None
// **/
static
void Mac802_15_4DevCapSetAlloShortAddr(M802_15_4DevCapability* devCap,
                                       BOOL alloc)
{
    devCap->alloShortAddr = alloc;
    devCap->cap = (devCap->cap & 0xfe);
    if (alloc == TRUE)
    {
        devCap->cap += 0x01;
    }
}
#endif
static
clocktype Mac802_15_4TrxTime(Node* node, Int32 interfaceIndex, Message* msg)
{
    clocktype trxTime = 0;
    trxTime = PHY_GetTransmissionDuration(
                                node,
                                interfaceIndex,
                                M802_15_4_DEFAULT_DATARATE_INDEX,
                                MESSAGE_ReturnPacketSize(msg));
    return trxTime;
}

// /**
// FUNCTION   :: MAC802_15_4IPv4AddressToHWAddress
// LAYER      :: MAC
// PURPOSE    :: Convert IPv4 addtess to Hardware address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + ipv4Address : NodeAddress : IPv4 address
// + macAddr : MacHWAddress* : Pointer to Hardware address structure
// RETURN     :: void :
// **/
void MAC802_15_4IPv4AddressToHWAddress(Node* node,
                                       NodeAddress ipv4Address,
                                       MacHWAddress* macAddr);

void Mac802_15_4Dispatch(Node* node,
                         Int32 interfaceIndex,
                         PhyStatusType status,
                         const char* frFunc,
                         PLMEsetTrxState req_state,
                         M802_15_4_enum mStatus);

#endif /*MAC_802_15_4*/
