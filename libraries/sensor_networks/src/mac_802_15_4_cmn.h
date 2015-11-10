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

//This is the header file for MAC protocol for IEEE 802.15.4 standards.

#ifndef MAC_802_15_4_CMN_H
#define MAC_802_15_4_CMN_H

#include "phy_802_15_4.h"

const UInt8 aMaxPHYPacketSize = 127;
const UInt8 aTurnaroundTime  = 12;

static
void Phy802_15_4PLME_SET_request(Node* node,
                                 Int32 phyIndex,
                                 PPIBAenum PIBAttribute,
                                 PHY_PIB *PIBAttributeValue)
{
    // not implemented yet
}

static
void Phy802_15_4PLME_GET_request(Node* node,
                                 Int32 phyIndex,
                                 PPIBAenum PIBAttribute)
{
    // not implemented yet
}

void Phy802_15_4PLME_CCA_request(Node* node, Int32 interfaceIndex);


// /**
// CONSTANT    :: aMaxBeaconOverhead : 75
// DESCRIPTION :: max # of octets added by the MAC sublayer to the payload of
//                  its beacon frame
// **/
const UInt8 aMaxBeaconOverhead     = 75;

// /**
// CONSTANT    :: MACADDR : UInt16
// DESCRIPTION :: Define MACADDR as unsinged int16. Should be 64-bit but use
//                  16-bit for simulation.
// **/
#define     MACADDR     UInt16

// /**
// CONSTANT    :: aMaxBeaconPayloadLength :
//                      aMaxPHYPacketSize - aMaxBeaconOverhead
// DESCRIPTION :: max size, in octets, of a beacon payload
// **/
const UInt8 aMaxBeaconPayloadLength
        = aMaxPHYPacketSize - aMaxBeaconOverhead;

// /**
// ENUM        :: M802_15_4_enum
// DESCRIPTION :: MAC enumerations description (Table 78), 2006
// **/
typedef enum
{
    M802_15_4_SUCCESS = 0,
    //--- following from Table 68) ---
    M802_15_4_PAN_AT_CAPACITY,
    M802_15_4_PAN_ACCESS_DENIED,
    //--------------------------------
    M802_15_4_BEACON_LOSS = 0xe0,
    M802_15_4_CHANNEL_ACCESS_FAILURE,
    M802_15_4_DENIED,
    M802_15_4_DISABLE_TRX_FAILURE,
    M802_15_4_FAILED_SECURITY_CHECK,
    M802_15_4_FRAME_TOO_LONG,
    M802_15_4_INVALID_GTS,
    M802_15_4_INVALID_HANDLE,
    M802_15_4_INVALID_PARAMETER,
    M802_15_4_NO_ACK,
    M802_15_4_NO_BEACON,
    M802_15_4_NO_DATA,
    M802_15_4_NO_SHORT_ADDRESS,
    M802_15_4_OUT_OF_CAP,
    M802_15_4_PAN_ID_CONFLICT,
    M802_15_4_REALIGNMENT,
    M802_15_4_TRANSACTION_EXPIRED,
    M802_15_4_TRANSACTION_OVERFLOW,
    M802_15_4_TX_ACTIVE,
    M802_15_4_UNAVAILABLE_KEY,
    M802_15_4_UNSUPPORTED_ATTRIBUTE,
    M802_15_4_UNDEFINED         //added this for handling any case not
                                //specified in the draft
}M802_15_4_enum;

// /**
// ENUM        :: M802_15_4_PIBA_enum
// DESCRIPTION :: MAC PIB attributes (Tables 86,88), 2006
// **/
typedef enum
{
    //attributes from Table 86
    macAckWaitDuration,
    macAssociationPermit,
    macAutoRequest,
    macBattLifeExt,
    macBattLifeExtPeriods,
    macBeaconPayload,
    macBeaconPayloadLength,
    macBeaconOrder,
    macBeaconTxTime,
    macBSN,
    macCoordExtendedAddress,
    macCoordShortAddress,
    macDSN,
    macGTSPermit,
    macMaxCSMABackoffs,
    macMinBE,
    macPANId,
    macPromiscuousMode,
    macRxOnWhenIdle,
    macShortAddress,
    macSuperframeOrder,
    macTransactionPersistenceTime,
    macGtsTriggerPrecedence,
    macDataAcks,
    //attributes from Table 88 (security attributes)
    macACLEntryDescriptorSet,
    macACLEntryDescriptorSetSize,
    macDefaultSecurity,
    macACLDefaultSecurityMaterialLength,
    macDefaultSecurityMaterial,
    macDefaultSecuritySuite,
    macSecurityMode
}M802_15_4_PIBA_enum;

// /**
// STRUCT      :: M802_15_4ACL
// DESCRIPTION :: Access control list
// **/
typedef struct mac_802_15_4_acl_str
{
    MACADDR ACLExtendedAddress;
    UInt16 ACLShortAddress;
    UInt16 ACLPANId;
    UInt8  ACLSecurityMaterialLength;
    UInt8  *ACLSecurityMaterial;
    UInt8  ACLSecuritySuite;
}M802_15_4ACL;

// /**
// STRUCT      :: M802_15_4PanEle
// DESCRIPTION :: PAN element
// **/
typedef struct mac_802_15_4_pan_ele_str
{
    UInt8  CoordAddrMode;
    UInt16 CoordPANId;
    union
    {
        UInt16 CoordAddress_16;
        MACADDR CoordAddress_64;
    };
    UInt8  LogicalChannel;
    UInt16 SuperframeSpec;
    BOOL    GTSPermit;
    UInt8  LinkQuality;
    clocktype TimeStamp;
    BOOL    SecurityUse;
    UInt8  ACLEntry;
    BOOL    SecurityFailure;
    //add one field for cluster tree
    UInt16 clusTreeDepth;
}M802_15_4PanEle;

// /**
// STRUCT      :: M802_15_4PIB
// DESCRIPTION :: MAC Protocol Information Base
// **/
typedef struct mac_802_15_4_pib_str
{
    UInt32        macAckWaitDuration;
    BOOL          macAssociationPermit;
    BOOL          macAutoRequest;
    BOOL          macBattLifeExt;
    UInt8         macBattLifeExtPeriods;
    UInt8         macBeaconPayload[aMaxBeaconPayloadLength + 1];
    UInt8         macBeaconPayloadLength;
    UInt8         macBeaconOrder;
    clocktype     macBeaconTxTime;
    UInt8         macBSN;
    MACADDR       macCoordExtendedAddress;
    UInt16        macCoordShortAddress;
    UInt8         macDSN;
    BOOL          macGTSPermit;
    UInt8         macMaxCSMABackoffs;
    UInt8         macMinBE;
    UInt16        macPANId;
    BOOL          macPromiscuousMode;
    BOOL          macRxOnWhenIdle;
    UInt16        macShortAddress;
    UInt8         macSuperframeOrder;
    UInt16        macTransactionPersistenceTime;

    //security attributes
    M802_15_4ACL* macACLEntryDescriptorSet;
    UInt8         macACLEntryDescriptorSetSize;
    BOOL          macDefaultSecurity;
    UInt8         macACLDefaultSecurityMaterialLength;
    UInt8*        macDefaultSecurityMaterial;
    UInt8         macDefaultSecuritySuite;
    UInt8         macSecurityMode;
    UInt8         macGtsTriggerPrecedence;
    BOOL          macDataAcks;
}M802_15_4PIB;

// /**
// STRUCT      :: M802_15_4DEVLINK
// DESCRIPTION :: Device Link Structure
// **/
typedef struct mac_802_15_4_devlink_str
{
    MACADDR addr64;     //extended address of the associated device
    UInt16 addr16;     //assigned short address
    UInt8 capability;  //device capability
    UInt8 numTimesTrnsExpired;
    struct mac_802_15_4_devlink_str* last;
    struct mac_802_15_4_devlink_str* next;
}M802_15_4DEVLINK;

// /**
// STRUCT      :: M802_15_4TRANSLINK
// DESCRIPTION :: Transaction Link Structure
// **/
typedef struct mac_802_15_4_translink_str
{
    UInt8 pendAddrMode;
    union
    {
        UInt16 pendAddr16;
        MACADDR pendAddr64;
    };
    Message* pkt;
    UInt8 msduHandle;
    clocktype expTime;
    struct mac_802_15_4_translink_str* last;
    struct mac_802_15_4_translink_str* next;
}M802_15_4TRANSLINK;

BOOL Mac802_15_4ToParent(Node* node, Int32 interfaceIndex, Message* p);

clocktype Mac802_15_4LocateBoundary(Node* node,
                                    Int32 interfaceIndex,
                                    BOOL parent,
                                    clocktype wtime);

Int32 Mac802_15_4GetBattLifeExtSlotNum(Node* node, Int32 interfaceIndex);
clocktype Mac802_15_4GetCAP(Node* node, Int32 interfaceIndex, BOOL toParent);
clocktype Mac802_15_4GetCAPbyType(Node* node, Int32 interfaceIndex, Int32 type);

void Mac802_15_4CsmacaCallBack(Node* node,
                               Int32 interfaceIndex,
                               PhyStatusType status);

#endif /*MAC_802_15_4_CMN_H*/
