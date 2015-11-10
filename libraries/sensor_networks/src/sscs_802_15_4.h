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

// This is the header file for MAC protocol for IEEE 802.15.4 standards.

#ifndef SSCS_802_15_4_H
#define SSCS_802_15_4_H

#include "api.h"
#include "partition.h"
#include "mac_802_15_4_cmn.h"
#include "mac_802_15_4.h"

// --------------------------------------------------------------------------
// #define's
// --------------------------------------------------------------------------

// /**
// CONSTANT    :: S802_15_4DEFBOVAL : 3
// DESCRIPTION :: Default BO value
// **/
#define S802_15_4DEF_BOVAL   3

// /**
// CONSTANT    :: S802_15_4DEF_GTS_SLOT_LENGTH : 1
// DESCRIPTION :: Default GTS slot length
// **/
#define S802_15_4DEF_GTS_SLOT_LENGTH 1

// /**
// CONSTANT    :: S802_15_4DEF_GTS_TRIGGER_PRECEDENCE : 1
// DESCRIPTION :: Default GTS trigger precedence
// **/
#define S802_15_4DEF_GTS_TRIGGER_PRECEDENCE 1

// /**
// CONSTANT    :: S802_15_4DEFSOVAL : 3
// DESCRIPTION :: Default SO value
// **/
#define S802_15_4DEF_SOVAL   3

// /**
// CONSTANT    :: S802_15_4DEF_SCANDUR : 3
// DESCRIPTION :: Default Scan Duration value
// **/
#define S802_15_4DEF_SCANDUR   3

// /**
// CONSTANT    :: S802_15_4SCANCHANNELS : 0x00003800
// DESCRIPTION :: Channels to be scanned : only first 3 channels in 2.4G
// **/
#define S802_15_4SCANCHANNELS   0x00003800

// /**
// CONSTANT    :: S802_15_4ASSORETRY_INTERVAL : 1 * SECOND
// DESCRIPTION :: Retry Association interval
// **/
#define S802_15_4ASSORETRY_INTERVAL     1 * SECOND
// --------------------------------------------------------------------------
// typedef's enums
// --------------------------------------------------------------------------

// /**
// ENUM        :: S802_15_4TimerType
// DESCRIPTION :: Timers used by SSCS
// **/
typedef enum
{
    // SSCS sub-layer timers
    S802_15_4STARTDEVICE,
    S802_15_4STOPDEVICE,
    S802_15_4STARTBEACON,
    S802_15_4STOPBEACON,
    S802_15_4ASSORETRY,
    S802_15_4POLLINT
}S802_15_4TimerType;

// /**
// ENUM        :: S802_15_4State
// DESCRIPTION :: SSCS states
// **/
typedef enum
{
    S802_15_4NULL,
    S802_15_4SCANREQ,
    S802_15_4STARTREQ,
    S802_15_4ASSOREQ,
    S802_15_4DISASSOREQ,
    S802_15_4UP
}S802_15_4State;

// /**
// ENUM        :: S802_15_4FFdModes
// DESCRIPTION :: FFD modes
// **/
typedef enum
{
    S802_15_4PANCOORD,
    S802_15_4COORD,
    S802_15_4DEVICE
}S802_15_4FFdModes;


// --------------------------------------------------------------------------
// typedef's struct
// --------------------------------------------------------------------------
//                       Layer structure
// --------------------------------------------------------------------------

// /**
// STRUCT      :: S802_15_4Timer
// DESCRIPTION :: Timer type for SSCS 802.15.4
// **/
typedef struct sscs_802_15_4_timer_str
{
    S802_15_4TimerType timerType;
}S802_15_4Timer;

// /**
// STRUCT      :: S802_15_4Statistics
// DESCRIPTION :: Statistics of SSCS802.15.4
// **/
typedef struct sscs_802_15_4_stats_str
{
    long numAssociationAcptd;
    long numAssociationRejctd;
    long numSyncLoss;
}S802_15_4Statistics;

// /**
// STRUCT      :: SscsData802_15_4
// DESCRIPTION :: Layer structure of SSCS802.15.4
// **/
typedef struct sscs_802_15_4_str
{
    BOOL t_isCT;
    BOOL t_txBeacon;
    BOOL t_isFFD;
    BOOL t_assoPermit;
    BOOL t_gtsPermit;
    UInt8 t_gtsSlotLength;
    UInt8 t_gtsTriggerPrecedence;
    BOOL t_dataAcks;
    UInt8 t_BO;
    UInt8 t_SO;
    Int32 ffdMode;    // 0=PAN-cord, 1=coord, 2=device
    clocktype pollInt;

    // for cluster tree
    UInt16 rt_myDepth;
    UInt16 rt_myNodeID;
    UInt16 rt_myParentNodeID;

    UInt32 ScanChannels;
    BOOL neverAsso;

    // --- store results returned from MLME_SCAN_confirm() ---
    UInt32 T_UnscannedChannels;
    UInt8  T_ResultListSize;
    UInt8* T_EnergyDetectList;
    M802_15_4PanEle* T_PANDescriptorList;
    M802_15_4PanEle startDevice_panDes;
    UInt8 Channel;
    UInt8  startDevice_Channel;
    
    // SSCS state
    S802_15_4State state;

    // Statistics
    S802_15_4Statistics stats;

    // Seed
    RandomSeed seed;
}SscsData802_15_4;

// --------------------------------------------------------------------------
// FUNCTION DECLARATIONS
// --------------------------------------------------------------------------
// API functions between SSCS and MAC
// --------------------------------------------------------------------------

// /**
// FUNCTION   :: Sscs802_15_4IsDeviceUp
// LAYER      :: SSCS
// PURPOSE    :: Primitive to find out whether the device is up or not.
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32       : Interface Index
// RETURN  :: BOOL
// **/
BOOL Sscs802_15_4IsDeviceUp(Node* node,
                            Int32 interfaceIndex);

// /**
// FUNCTION   :: Sscs802_15_4MCPS_DATA_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report results of a request to tx a data SPDU
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + msduHandle     : UInt8         : Handle associated with MSDU
// + status         : M802_15_4_enum: Status of data sent request
// RETURN  :: None
// **/
void Sscs802_15_4MCPS_DATA_confirm(Node* node,
                                   Int32 interfaceIndex,
                                   UInt8 msduHandle,
                                   M802_15_4_enum status);

// /**
// FUNCTION   :: Sscs802_15_4MCPS_DATA_indication
// LAYER      :: Mac
// PURPOSE    :: Primitive to indicate the transfer of a data SPDU to SSCS
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + SrcAddrMode    : UInt8         : Source address mode
// + SrcPANId       : UInt16        : source PAN id
// + SrcAddr        : MACADDR    : Source address
// + DstAddrMode    : UInt8         : Destination address mode
// + DstPANId       : UInt16        : Destination PAN id
// + DstAddr        : MACADDR    : Destination Address
// + msduLength     : UInt8         : MSDU length
// + msdu           : Message*      : MSDU
// + mpduLinkQuality: UInt8         : LQI value measured during reception of
//                                    the MPDU
// + SecurityUse    : BOOL          : whether security is used
// + ACLEntry       : UInt8         : ACL entry
// RETURN  :: None
// **/
void Sscs802_15_4MCPS_DATA_indication(Node* node,
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
// FUNCTION   :: Sscs802_15_4MCPS_PURGE_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of purge request
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + msduHandle     : UInt8         : Handle associated with MSDU
// + status         : M802_15_4_enum: Status of purge request
// RETURN  :: None
// **/
void Sscs802_15_4MCPS_PURGE_confirm(Node* node,
                                    Int32 interfaceIndex,
                                    UInt8 msduHandle,
                                    M802_15_4_enum status);

// /**
// FUNCTION   :: Sscs802_15_4MLME_ASSOCIATE_indication
// LAYER      :: Mac
// PURPOSE    :: Primitive to indicate an incoming associate request
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + DeviceAddress      : MACADDR    : Address of device requesting
//                                        association
// + CapabilityInformation : UInt8  : capabilities of associating device
// + SecurityUse    : BOOL          : Whether enable security or not
// + ACLEntry       : UInt8         : ACL entry
// RETURN  :: None
// **/
void Sscs802_15_4MLME_ASSOCIATE_indication(Node* node,
                                           Int32 interfaceIndex,
                                           MACADDR DeviceAddress,
                                           UInt8 CapabilityInformation,
                                           BOOL SecurityUse,
                                           UInt8 ACLEntry);

// /**
// FUNCTION   :: Sscs802_15_4MLME_ASSOCIATE_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of associate request
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + AssocShortAddress  : UInt16        : Short address allocated by coord
// + status             : M802_15_4_enum: Status of association attempt
// RETURN  :: None
// **/
void Sscs802_15_4MLME_ASSOCIATE_confirm(Node* node,
                                        Int32 interfaceIndex,
                                        UInt16 AssocShortAddress,
                                        M802_15_4_enum status);

// /**
// FUNCTION   :: Sscs802_15_4MLME_DISASSOCIATE_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of disassociate request
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + status             : M802_15_4_enum: Status of disassociation attempt
// RETURN  :: None
// **/
void Sscs802_15_4MLME_DISASSOCIATE_confirm(Node* node,
                                           Int32 interfaceIndex,
                                           M802_15_4_enum status);

// /**
// FUNCTION   :: Sscs802_15_4MLME_BEACON_NOTIFY_indication
// LAYER      :: Mac
// PURPOSE    :: Primitive to send params contained within a beacon to SSCS
// PARAMETERS ::
// + node           : Node*             : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + BSN            : UInt8             : The beacon sequence number.
// + PANDescriptor  : M802_15_4PanEle*  : PANDescriptor for the recd beacon
// + PendAddrSpec   : UInt8             : Beacon pending address spec
// + AddrList       : MACADDR*       : list of addresses of the devices
//                                        for which beacon source has data
// + sduLength      : UInt8             : number of octets contained in the
//                                      beacon payload of the beacon frame
// + sdu            : UInt8*            : beacon payload to be transferred
// RETURN  :: None
// **/
void Sscs802_15_4MLME_BEACON_NOTIFY_indication(Node* node,
                                               Int32 interfaceIndex,
                                               UInt8 BSN,
                                               M802_15_4PanEle* PANDescriptor,
                                               UInt8 PendAddrSpec,
                                               MACADDR* AddrList,
                                               UInt8 sduLength,
                                               UInt8* sdu);

// /**
// FUNCTION   :: Sscs802_15_4MLME_GET_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of GET request
// PARAMETERS ::
// + node               : Node*                 : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + status             : M802_15_4_enum        : Status of GET attempt
// + PIBAttribute       : M802_15_4_PIBA_enum   : PIB attribute id
// + PIBAttributeValue  : M802_15_4PIB*         : Attribute value
// RETURN  :: None
// **/
void Sscs802_15_4MLME_GET_confirm(Node* node,
                                  Int32 interfaceIndex,
                                  M802_15_4_enum status,
                                  M802_15_4_PIBA_enum PIBAttribute,
                                  M802_15_4PIB* PIBAttributeValue);

// /**
// FUNCTION   :: Sscs802_15_4MLME_ORPHAN_indication
// LAYER      :: Mac
// PURPOSE    :: Primitive to notify presence of orphan device to SSCS
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + OrphanAddress      : MACADDR    : Address of orphan device
// + SecurityUse        : BOOL          : Whether enabled security or not
// + ACLEntry           : UInt8         : ACL entry
// RETURN  :: None
// **/
void Sscs802_15_4MLME_ORPHAN_indication(Node* node,
                                        Int32 interfaceIndex,
                                        MACADDR OrphanAddress,
                                        BOOL SecurityUse,
                                        UInt8 ACLEntry);

// /**
// FUNCTION   :: Sscs802_15_4MLME_RESET_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of RESET request
// PARAMETERS ::
// + node               : Node*                 : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + status             : M802_15_4_enum        : Status of RESET attempt
// RETURN  :: None
// **/
void Sscs802_15_4MLME_RESET_confirm(Node* node,
                                    Int32 interfaceIndex,
                                    M802_15_4_enum status);

// /**
// FUNCTION   :: Sscs802_15_4MLME_RX_ENABLE_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of RxEnable request
// PARAMETERS ::
// + node               : Node*                 : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + status             : M802_15_4_enum        : Status of RxEnable attempt
// RETURN  :: None
// **/
void Sscs802_15_4MLME_RX_ENABLE_confirm(Node* node,
                                        Int32 interfaceIndex,
                                        M802_15_4_enum status);

// /**
// FUNCTION   :: Sscs802_15_4MLME_SET_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of SET request
// PARAMETERS ::
// + node               : Node*                 : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + status             : M802_15_4_enum        : Status of SET attempt
// + PIBAttribute       : M802_15_4_PIBA_enum   : PIB attribute id
// RETURN  :: None
// **/
void Sscs802_15_4MLME_SET_confirm(Node* node,
                                  Int32 interfaceIndex,
                                  M802_15_4_enum status,
                                  M802_15_4_PIBA_enum PIBAttribute);

// /**
// FUNCTION   :: Sscs802_15_4MLME_SCAN_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of SCAN request
// PARAMETERS ::
// + node               : Node*             : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + status             : M802_15_4_enum    : Status of Scan attempt
// + ScanType           : UInt8             : Type of scan
// + UnscannedChannels  : UInt32            : Channels given in the request
//                                            not scanned
// + ResultListSize     : UInt8             : Number of elements returned
// + EnergyDetectList   : UInt8*            : List of energy measurements,
//                                            one for each channel
// + PANDescriptorList  : M802_15_4PanEle*  : List of PAN descriptors
// RETURN  :: None
// **/
void Sscs802_15_4MLME_SCAN_confirm(Node* node,
                                   Int32 interfaceIndex,
                                   M802_15_4_enum status,
                                   UInt8 ScanType,
                                   UInt32 UnscannedChannels,
                                   UInt8 ResultListSize,
                                   UInt8* EnergyDetectList,
                                   M802_15_4PanEle* PANDescriptorList);

// /**
// FUNCTION   :: Sscs802_15_4MLME_COMM_STATUS_indication
// LAYER      :: Mac
// PURPOSE    :: Primitive to indicate a communications status
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + PANId          : UInt16        : PAN id
// + SrcAddrMode    : UInt8         : Source address mode
// + SrcAddr        : MACADDR    : Source address
// + DstAddrMode    : UInt8         : Destination address mode
// + DstAddr        : MACADDR    : Destination Address
// + status         : M802_15_4_enum: Status of Communication
// RETURN  :: None
// **/
void Sscs802_15_4MLME_COMM_STATUS_indication(Node* node,
                                             Int32 interfaceIndex,
                                             UInt16 PANId,
                                             UInt8 SrcAddrMode,
                                             MACADDR SrcAddr,
                                             UInt8 DstAddrMode,
                                             MACADDR DstAddr,
                                             M802_15_4_enum status);

// /**
// FUNCTION   :: Sscs802_15_4MLME_START_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of START request
// PARAMETERS ::
// + node               : Node*                 : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + status             : M802_15_4_enum        : Status of START attempt
// RETURN  :: None
// **/
void Sscs802_15_4MLME_START_confirm(Node* node,
                                    Int32 interfaceIndex,
                                    M802_15_4_enum status);

// /**
// FUNCTION   :: Sscs802_15_4MLME_SYNC_LOSS_indication
// LAYER      :: Mac
// PURPOSE    :: Primitive to indicate the loss of synchronization with a
//               coordinator
// PARAMETERS ::
// + node               : Node*                 : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + LossReason         : M802_15_4_enum        : Reason for Sync Loss
// RETURN  :: None
// **/
void Sscs802_15_4MLME_SYNC_LOSS_indication(Node* node,
                                           Int32 interfaceIndex,
                                           M802_15_4_enum LossReason);

// /**
// FUNCTION   :: Sscs802_15_4MLME_POLL_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of POLL request
// PARAMETERS ::
// + node               : Node*                 : Node receiving call
// + interfaceIndex     : Int32                   : Interface Index
// + status             : M802_15_4_enum        : Status of POLL attempt
// RETURN  :: None
// **/
void Sscs802_15_4MLME_POLL_confirm(Node* node,
                                   Int32 interfaceIndex,
                                   M802_15_4_enum status);

// /**
// FUNCTION     Sscs802_15_4Init
// PURPOSE      Initialization function for 802.15.4 protocol of SSCS layer
// PARAMETERS   Node* node
//                  Node being initialized.
//              NodeInput* nodeInput
//                  Structure containing contents of input file.
//              MacData_802_15_4* M802_15_4
//                  802.15.4 data structure
// RETURN       None
// NOTES        None
// **/
void Sscs802_15_4Init(Node* node,
                      const NodeInput* nodeInput,
                      Int32 interfaceIndex);

// /**
// FUNCTION     Sscs802_15_4Layer
// PURPOSE      To handle timer events. This is called via Mac802_15_4Layer()
// PARAMETERS   Node *node
//                  Node which received the message.
//              Int32 interfaceInde
//                  Interface index on which message is received
//              Message* msg
//                  Message received by the layer.
// RETURN       None
// NOTES        None
// **/
void Sscs802_15_4Layer(Node* node, Int32 interfaceIndex, Message* msg);

// /**
// FUNCTION     Sscs802_15_4Finalize
// PURPOSE      Called at the end of simulation to collect the results of
//              the simulation of 802.15.4 protocol of the SSCS Layer.
// PARAMETERS   Node* node
//                  Node which received the message.
// RETURN       None
// NOTES        None
// **/
void Sscs802_15_4Finalize(Node* node, Int32 interfaceIndex);

// /**
// FUNCTION   :: Sscs802_15_4StopDevice
// LAYER      :: SSCS
// PURPOSE    :: Stops a Device
// PARAMETERS ::
// + node           : Node*          : Node receiving call
// + interfaceIndex : Int32          : Interface Index
// + status         : M802_15_4_enum : Status of request
// RETURN     :: None
// **/

void Sscs802_15_4StopDevice(Node* node,
                            Int32 interfaceIndex,
                            M802_15_4_enum status);

// --------------------------------------------------------------------------
// STATIC FUNCTIONS
// --------------------------------------------------------------------------

#endif /*SSCS_802_15_4*/
