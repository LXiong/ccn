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

#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "api.h"
#include "partition.h"
#include "network_ip.h"
#include "list.h"
#include "mac.h"
#include "mac_802_15_4.h"
#include "transport_udp.h"

#include "sscs_802_15_4.h"

#define DEBUG 0
#define DEBUG_TIMER 0

// Forward declarations
static
void Mac802_15_4mlme_scan_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 ScanType,
                                  UInt32 ScanChannels,
                                  UInt8 ScanDuration,
                                  BOOL frUpper,
                                  PhyStatusType status);

static
void Mac802_15_4mlme_start_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt16 PANId,
                                  UInt8 LogicalChannel,
                                  UInt8 BeaconOrder,
                                  UInt8 SuperframeOrder,
                                  BOOL PANCoordinator,
                                  BOOL BatteryLifeExtension,
                                  BOOL CoordRealignment,
                                  BOOL SecurityEnable,
                                  BOOL frUpper,
                                  PhyStatusType status);

static
void Mac802_15_4mlme_associate_request(Node* node,
                                       Int32 interfaceIndex,
                                       UInt8 LogicalChannel,
                                       UInt8 CoordAddrMode,
                                       UInt16 CoordPANId,
                                       MACADDR CoordAddress,
                                       UInt8 CapabilityInformation,
                                       BOOL SecurityEnable,
                                       BOOL frUpper,
                                       PhyStatusType status,
                                       M802_15_4_enum mStatus);

static
void Mac802_15_4mlme_associate_response(Node* node,
                                        Int32 interfaceIndex,
                                        MACADDR DeviceAddress,
                                        UInt16 AssocShortAddress,
                                        M802_15_4_enum Status,
                                        BOOL SecurityEnable,
                                        BOOL frUpper,
                                        PhyStatusType status);

static
void Mac802_15_4mlme_orphan_response(Node* node,
                                     Int32 interfaceIndex,
                                     MACADDR OrphanAddress,
                                     UInt16 ShortAddress,
                                     BOOL AssociatedMember,
                                     BOOL SecurityEnable,
                                     BOOL frUpper,
                                     PhyStatusType status);
static
void Mac802_15_4mlme_disassociate_request(Node* node,
                                          Int32 interfaceIndex,
                                          MACADDR DeviceAddress,
                                          UInt8 DisassociateReason,
                                          BOOL SecurityEnable,
                                          BOOL frUpper,
                                          PhyStatusType status);

static
void Mac802_15_4mlme_sync_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 LogicalChannel,
                                  BOOL TrackBeacon,
                                  BOOL frUpper,
                                  PhyStatusType status);

static
void Mac802_15_4mlme_poll_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 CoordAddrMode,
                                  UInt16 CoordPANId,
                                  MACADDR CoordAddress,
                                  BOOL SecurityEnable,
                                  BOOL autoRequest,
                                  BOOL firstTime,
                                  PhyStatusType status);

static
void Mac802_15_4mlme_reset_request(Node* node,
                                   Int32 interfaceIndex,
                                   BOOL SetDefaultPIB,
                                   BOOL frUpper,
                                   PhyStatusType status);

static
void Mac802_15_4mlme_rx_enable_request(Node* node,
                                       Int32 interfaceIndex,
                                       BOOL DeferPermit,
                                       UInt32 RxOnTime,
                                       UInt32 RxOnDuration,
                                       BOOL frUpper,
                                       PhyStatusType status);

static
void Mac802_15_4mcps_data_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 SrcAddrMode,
                                  UInt16 SrcPANId,
                                  MACADDR SrcAddr,
                                  UInt8 DstAddrMode,
                                  UInt16 DstPANId,
                                  MACADDR DstAddr,
                                  Int32 msduLength,
                                  Message* msdu,
                                  UInt8 msduHandle,
                                  UInt8 TxOptions,
                                  BOOL frUpper,
                                  PhyStatusType status,
                                  M802_15_4_enum mStatus);

static
void Mac802_15_4CsmacaResume(Node* node, Int32 interfaceIndex);

static
void Mac802_15_4TxBcnCmdDataHandler(Node* node, Int32 interfaceIndex);

// /**
// FUNCTION   :: MAC_VariableHWAddressToTwoByteMacAddress
// LAYER      :: MAC
// PURPOSE    :: Retrieve  IP address from.MacHWAddress of type
//               HW_NODE_ID
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: NodeAddress : Ipv4 Address
// **/
static
MACADDR MAC_VariableHWAddressToTwoByteMacAddress(Node* node,
                                                 MacHWAddress* macAddr)
{
    if (macAddr->hwLength != MAC_NODEID_LINKADDRESS_LENGTH
        || macAddr->hwType == HW_TYPE_UNKNOWN)
    {
        return 0;
    }
    if (MAC_IsBroadcastHWAddress(macAddr))
    {
        return (MACADDR)ANY_DEST;
    }
    else
    {
        MACADDR nodeAddress;
        memcpy(&nodeAddress, macAddr->byte, MAC_NODEID_LINKADDRESS_LENGTH);
        return nodeAddress;
    }
}

// /**
// FUNCTION   :: ConvertVariableHWAddressToMacAddr
// LAYER      :: MAC
// PURPOSE    :: Convert Variable Hardware address to ZigBee MACADDR
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// + macAddr : MACADDR* : Pointer to ZigBee MAC address structure
// RETURN     :: Bool :
// **/
static
BOOL ConvertVariableHWAddressToMacAddr(Node* node,
                                       MacHWAddress* macHWAddr,
                                       MACADDR* macAddr)
{
    if (macHWAddr->hwLength == MAC_NODEID_LINKADDRESS_LENGTH)
    {
        memcpy(macAddr, macHWAddr->byte, macHWAddr->hwLength);
        return TRUE;
    }
    return FALSE;
}

// /**
// FUNCTION   :: ConvertMacAddrToVariableHWAddress
// LAYER      :: MAC
// PURPOSE    :: Convert ZigBee Mac addtess to Variable Hardware address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// + macAddr : MACADDR* : Pointer to mac 802 address structure
// RETURN     :: Bool :
// **/
static
void ConvertMacAddrToVariableHWAddress (Node* node,
                                        MacHWAddress* macHWAddr,
                                        MACADDR* macAddr)
{
    macHWAddr->hwLength = MAC_NODEID_LINKADDRESS_LENGTH;
    macHWAddr->hwType = HW_NODE_ID;
    (*macHWAddr).byte = (unsigned char*) MEM_malloc(
                        sizeof(unsigned char)*MAC_NODEID_LINKADDRESS_LENGTH);
    memcpy(macHWAddr->byte, macAddr, MAC_NODEID_LINKADDRESS_LENGTH);
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
                                       MacHWAddress* macAddr)
{
    MACADDR nodeId = (MACADDR)
                        MAPPING_GetNodeIdFromInterfaceAddress(
                            node,
                            ipv4Address);
    ConvertMacAddrToVariableHWAddress(node, macAddr, &nodeId);
}

// /**
// FUNCTION   :: Phy802_15_4PLME_CCA_request
// LAYER      :: MAC
// PURPOSE    :: To initiate Channel clear access request
// PARAMETERS ::
// + node           : Node*             : Pointer to node
// + interfaceIndex : Int32               : Interface index
// RETURN     :: None
// **/
void Phy802_15_4PLME_CCA_request(Node* node, Int32 interfaceIndex)
{
    BOOL isChannelFree = FALSE;
    PhyStatusType status;

    Phy802_15_4PLMECCArequest(node, interfaceIndex, &isChannelFree);

    if (isChannelFree == TRUE)
    {
        status = PHY_IDLE;
    }
    else
    {
        status = PHY_BUSY_TX;
    }
    Mac802_15_4PLME_CCA_confirm(node, interfaceIndex, status);
}

static
M802_15_4PIB MPIB =
{
    M802_15_4_ACKWAITDURATION,
    M802_15_4_ASSOCIATIONPERMIT,
    M802_15_4_AUTOREQUEST,
    M802_15_4_BATTLIFEEXT,
    M802_15_4_BATTLIFEEXTPERIODS,
    M802_15_4_BEACONPAYLOAD,
    M802_15_4_BEACONPAYLOADLENGTH,
    M802_15_4_BEACONORDER,
    M802_15_4_BEACONTXTIME,
    M802_15_4_BSN,
    M802_15_4_COORDEXTENDEDADDRESS,
    M802_15_4_COORDSHORTADDRESS,
    M802_15_4_DSN,
    M802_15_4_GTSPERMIT,
    M802_15_4_MAXCSMABACKOFFS,
    M802_15_4_MINBE,
    M802_15_4_PANID,
    M802_15_4_PROMISCUOUSMODE,
    M802_15_4_RXONWHENIDLE,
    M802_15_4_SHORTADDRESS,
    M802_15_4_SUPERFRAMEORDER,
    M802_15_4_TRANSACTIONPERSISTENCETIME,
    M802_15_4_ACLENTRYDESCRIPTORSET,
    M802_15_4_ACLENTRYDESCRIPTORSETSIZE,
    M802_15_4_DEFAULTSECURITY,
    M802_15_4_ACLDEFAULTSECURITYMATERIALLENGTH,
    M802_15_4_DEFAULTSECURITYMATERIAL,
    M802_15_4_DEFAULTSECURITYSUITE,
    M802_15_4_SECURITYMODE
};

// /**
// FUNCTION   :: Mac802_15_4SetTimer
// LAYER      :: MAC
// PURPOSE    :: Set a timer message. If a non-NULL message pointer is
//               passed in, this function will just send out that msg.
//               If NULL message is passed in, it will create a new
//               message and send it out. In either case, a pointer to
//               the message is returned, in case the caller wants to
//               cancel the message in the future.
// PARAMETERS ::
// + node      : Node*                  : Pointer to node
// + mac802_15_4: MacData802_15_4*      : Pointer to MAC802.15.4 structure
// + timerType : Mac802_15_4TimerType   : Type of the timer
// + delay     : clocktype              : Delay of this timer
// + msg       : Message*               : If non-NULL, use this message
// + infoVal   : unsigned Int32           : Additional info if needed
// RETURN     :: Message*               : Pointer to the timer message
// **/
Message* Mac802_15_4SetTimer(Node* node,
                             MacData802_15_4* mac802_15_4,
                             M802_15_4TimerType timerType,
                             clocktype delay,
                             Message* msg)
{
    Message* timerMsg = NULL;
    M802_15_4Timer* timerInfo;

    if (msg != NULL)
    {
        timerMsg = msg;
    }
    else
    {
        // allocate the timer message and send out
        timerMsg = MESSAGE_Alloc(node,
                                 MAC_LAYER,
                                 MAC_PROTOCOL_802_15_4,
                                 MSG_MAC_TimerExpired);

        MESSAGE_SetInstanceId(timerMsg,
                             (short)mac802_15_4->myMacData->interfaceIndex);

        MESSAGE_InfoAlloc(node,
                          timerMsg,
                          sizeof(M802_15_4Timer));
        timerInfo = (M802_15_4Timer*) MESSAGE_ReturnInfo(timerMsg);
        timerInfo->timerType = timerType;
    }

    MESSAGE_Send(node, timerMsg, delay);
    return (timerMsg);
}

// /**
// FUNCTION   :: Mac802_15_4ConstructPayload
// LAYER      :: Mac
// PURPOSE    :: Add MHR to a message
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + msg            : Message*  : Message pointer
// + frmType        : UInt8     : Frame type (beacon/data/ack/MAC cmd)
// + superSpec      : UInt16    : Superframe specification
// + GTSFields      : M802_15_4GTSFields: GTS fields - not used currently
// + PendAddrFields : M802_15_4PendAddrField : List of devices for whom data
//                                      is pending
// + CmdType        : UInt8     : Command type for MAC cmd frame
// RETURN  :: None
// **/
static
void Mac802_15_4ConstructPayload(Node* node,
                                 Int32 interfaceIndex,
                                 Message** msg,
                                 UInt8 frmType,
                                 UInt16 superSpec,
                                 M802_15_4GTSFields* GTSFields,
                                 M802_15_4PendAddrField*  PendAddrFields,
                                 UInt8 CmdType)
{
    MacData802_15_4* mac802_15_4 = NULL;
    M802_15_4BeaconFrame* bcFrm = NULL;
    M802_15_4CommandFrame* cmdFrm = NULL;
    Int32 payloadsize = 0;

    mac802_15_4 = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    *msg = MESSAGE_Alloc(node,
                         MAC_LAYER,
                         MAC_PROTOCOL_802_15_4,
                         MSG_MAC_802_15_4_Frame);

    switch (frmType)
    {
        case M802_15_4DEFFRMCTRL_TYPE_BEACON:  // Beacon
        {
            payloadsize = sizeof (M802_15_4BeaconFrame);
            break;
        }
        case M802_15_4DEFFRMCTRL_TYPE_MACCMD:  // Mac CMD
        {
            payloadsize = sizeof (M802_15_4CommandFrame);
            if (CmdType == 0x01 || CmdType == 0x09)
            {
                // association request or GTS Request
                payloadsize += 1;
            }
            else if (CmdType == 0x02)   // association response
            {
                payloadsize += sizeof(UInt16) + sizeof(M802_15_4_enum);
            }
            else if (CmdType == 0x03)   // disassociation notification
            {
                payloadsize += 1;
            }
            else if (CmdType == 0x08)    // realignment
            {
                payloadsize += 7;   // for extra payload.
            }
            break;
        }
        default:
        {
            payloadsize = 1;    // hack for ack
        }
    }

    MESSAGE_PacketAlloc(node, *msg, payloadsize, TRACE_MAC_802_15_4);

    if (frmType == M802_15_4DEFFRMCTRL_TYPE_BEACON)    // beacon
    {
        bcFrm = (M802_15_4BeaconFrame*) MESSAGE_ReturnPacket((*msg));
        bcFrm->MSDU_SuperSpec = superSpec;
        bcFrm->MSDU_GTSFields = *GTSFields;
        bcFrm->MSDU_PendAddrFields = *PendAddrFields;
    }
    else if (frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)   // MAC cmd
    {
        cmdFrm = (M802_15_4CommandFrame*) MESSAGE_ReturnPacket((*msg));
        cmdFrm->MSDU_CmdType = CmdType;
    }
}

// /**
// FUNCTION   :: Mac802_15_4AddCommandHeader
// LAYER      :: Mac
// PURPOSE    :: Add MHR to a message
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + msg            : Message*  : Message pointer
// + FrmType        : UInt8     : Frame type (beacon/data/ack/MAC cmd)
// + dstAddrMode    : UInt8     : Addressing mode (short/long)
// + dstPANId       : UInt16    : Destination PAN Id
// + dstAddr        : MACADDR: Destination address
// + srcAddrMode    : UInt8     : Source Address mode
// + srcPANId       : UInt16    : Source PAN Id
// + srcAddr        : MACADDR: Source address
// + secuEnable     : BOOL      : Whether security is enabled
// + pending        : BOOL      : Whether data pending
// + ackreq         : BOOL      : Whether Ack requested
// + FCS            : UInt16    : checksum - usually not calculated
// RETURN  :: None
// **/
static
void Mac802_15_4AddCommandHeader(Node* node,
                                 Int32 interfaceIndex,
                                 Message* msg,
                                 UInt8 FrmType,
                                 UInt8 dstAddrMode,
                                 UInt16 dstPANId,
                                 MACADDR dstAddr,
                                 UInt8 srcAddrMode,
                                 UInt16 srcPANId,
                                 MACADDR srcAddr,
                                 BOOL secuEnable,
                                 BOOL pending,
                                 BOOL ackreq,
                                 UInt8 CmdType,
                                 UInt16 FCS)
{
    MacData802_15_4* mac = NULL;
    M802_15_4Header* hdr = NULL;
    M802_15_4FrameCtrl frmCtrl = {0, 0};
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    MESSAGE_AddHeader(node,
                      msg,
                      sizeof(M802_15_4Header),
                      TRACE_MAC_802_15_4);

    hdr = (M802_15_4Header*) MESSAGE_ReturnPacket(msg);

    Mac802_15_4FrameCtrlSetFrmType(&frmCtrl, FrmType);
    Mac802_15_4FrameCtrlSetDstAddrMode(&frmCtrl, dstAddrMode);
    Mac802_15_4FrameCtrlSetSecu(&frmCtrl, secuEnable);
    Mac802_15_4FrameCtrlSetFrmPending(&frmCtrl, pending);
    Mac802_15_4FrameCtrlSetAckReq(&frmCtrl, ackreq);
    Mac802_15_4FrameCtrlSetSrcAddrMode(&frmCtrl, srcAddrMode);
    hdr->MHR_DstAddrInfo.panID = dstPANId;
    hdr->MHR_DstAddrInfo.addr_64 = dstAddr;
    hdr->MHR_SrcAddrInfo.panID = srcPANId;
    hdr->MHR_SrcAddrInfo.addr_64 = srcAddr;
    hdr->MHR_FrmCtrl = frmCtrl.frmCtrl;
    hdr->MHR_FCS = FCS;
    hdr->msduHandle = 0;
    hdr->indirect = 0;

    if (FrmType == M802_15_4DEFFRMCTRL_TYPE_BEACON)    // Beacon
    {
        hdr->MHR_BDSN = mac->mpib.macBSN++;
    }
    else if (FrmType == M802_15_4DEFFRMCTRL_TYPE_DATA
        || FrmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD) // Data or MAC cmd
    {
        hdr->MHR_BDSN = mac->mpib.macDSN++;
    }
    if (FrmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
    {
        hdr->MSDU_CmdType = CmdType;
    }
}

// /**
// FUNCTION   :: Mac802_15_4ConstructACK
// LAYER      :: Mac
// PURPOSE    :: Construct acknowledgement for a message
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + msg            : Message*  : Message pointer
// RETURN  :: None
// **/
static
void Mac802_15_4ConstructACK(Node* node,
                             Int32 interfaceIndex,
                             Message* msg)
{
    MacData802_15_4* mac = NULL;
    M802_15_4Header* wph1 = NULL;
    UInt16* ackHeaderIterator = NULL;
    M802_15_4FrameCtrl frmCtrl;
    BOOL intraPan = FALSE;
    UInt8 origFrmType = 0;
    Message* ackMsg = NULL;
    Int32 i = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    ackMsg = MESSAGE_Alloc(node,
                           MAC_LAYER,
                           MAC_PROTOCOL_802_15_4,
                           MSG_MAC_802_15_4_Frame);
    MESSAGE_PacketAlloc(node,
                        ackMsg,
                        numPsduOctetsInAckFrame,
                        TRACE_MAC_802_15_4);

    memset(MESSAGE_ReturnPacket(ackMsg), 0, numPsduOctetsInAckFrame);
    wph1 = (M802_15_4Header*) MESSAGE_ReturnPacket(msg);
    frmCtrl.frmCtrl = wph1->MHR_FrmCtrl;
    Mac802_15_4FrameCtrlParse(&frmCtrl);
    intraPan = frmCtrl.intraPan;
    origFrmType = frmCtrl.frmType;
    frmCtrl.frmCtrl = 0;
    Mac802_15_4FrameCtrlSetFrmType(&frmCtrl, M802_15_4DEFFRMCTRL_TYPE_ACK);
    Mac802_15_4FrameCtrlSetSecu(&frmCtrl, FALSE);
    if ((origFrmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)    // command packet
         && (wph1->MSDU_CmdType == 0x04))           // data request command
    {
        i = Mac802_15_4UpdateTransacLink(node,
                                         OPER_PURGE_TRANSAC,
                                         &mac->transacLink1,
                                         &mac->transacLink2,
                                         frmCtrl.srcAddrMode,
                                         wph1->MHR_SrcAddrInfo.addr_64);
        Mac802_15_4FrameCtrlSetFrmPending(&frmCtrl, i == 0);
    }
    else
    {
        Mac802_15_4FrameCtrlSetFrmPending(&frmCtrl, FALSE);
    }
    Mac802_15_4FrameCtrlSetAckReq(&frmCtrl, FALSE);
    Mac802_15_4FrameCtrlSetIntraPan(&frmCtrl, intraPan);
    Mac802_15_4FrameCtrlSetDstAddrMode(
            &frmCtrl,
            M802_15_4DEFFRMCTRL_ADDRMODENONE);
    Mac802_15_4FrameCtrlSetSrcAddrMode(
            &frmCtrl,
            M802_15_4DEFFRMCTRL_ADDRMODENONE);
    ackHeaderIterator = (UInt16*) MESSAGE_ReturnPacket(ackMsg);
    *ackHeaderIterator = frmCtrl.frmCtrl;
    ackHeaderIterator += 1; 
    *((UInt8*)ackHeaderIterator) = wph1->MHR_BDSN;
    if (mac->txAck)
    {
        MESSAGE_Free(node, mac->txAck);
        mac->txAck = NULL;
    }
    mac->txAck = ackMsg;
}

// /**
// FUNCTION   :: Mac802_15_4GetBattLifeExtSlotNum
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: Int32
// **/
Int32 Mac802_15_4GetBattLifeExtSlotNum(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    if (Phy802_15_4getChannelNumber(node, interfaceIndex) <= 10)
    {
        return 8;
    }
    else
    {
        return 6;
    }
}

// /**
// FUNCTION   :: Mac802_15_4GetCAP
// LAYER      :: Mac
// PURPOSE    :: Returns start of contention access period
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + small          : BOOL      :
// RETURN  :: clocktype
// **/
clocktype Mac802_15_4GetCAP(Node* node, Int32 interfaceIndex, BOOL toParent)
{
    MacData802_15_4* mac = NULL;
    clocktype bcnTxTime = 0;
    clocktype bcnRxTime = 0;
    clocktype bPeriod = 0;
    clocktype sSlotDuration = 0;
    clocktype sSlotDuration2 = 0;
    clocktype BI = 0,BI2 = 0;
    clocktype t_CAP1 = 0;
    clocktype t_CAP2 = 0;
    clocktype now = 0;
    clocktype oneDay = 0;
    clocktype tmpf = 0;
    clocktype len12s = 0;
    Int32 rate = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    now = node->getNodeTime();
    oneDay = now + 24 * 3600 * SECOND;

    // Non-beacom mode, canProceed function will always give green signal
    if ((mac->mpib.macBeaconOrder == 15)
        && (mac->macBeaconOrder2 == 15))
    {
        return oneDay;
    }

    rate = Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

    bcnTxTime = mac->macBcnTxTime;
    bcnRxTime = mac->macBcnRxTime;
    len12s = 12 * SECOND / rate;
    bPeriod = aUnitBackoffPeriod * SECOND / rate;
    sSlotDuration = mac->sfSpec.sd * SECOND / rate;
    sSlotDuration2 = mac->sfSpec2.sd * SECOND / rate;
    BI2 = mac->sfSpec2.BI * SECOND / rate;
    BI = mac->sfSpec.BI * SECOND / rate;
    if (mac->mpib.macBeaconOrder != 15)
    {
        if (mac->sfSpec.BLE)
        {

             tmpf = (mac->beaconPeriods +
                    Mac802_15_4GetBattLifeExtSlotNum(node,
                      interfaceIndex)) * aUnitBackoffPeriod * SECOND / rate;

            t_CAP1 = bcnTxTime + tmpf;
        }
        else
        {
            tmpf = (mac->sfSpec.FinCAP + 1) * sSlotDuration - len12s;
            t_CAP1 = bcnTxTime + tmpf;
        }
    }
    else
    {
        t_CAP1 = oneDay;
    }
    if (mac->macBeaconOrder2 != 15)
    {

         // take into account lost beacons (if any)
        while (bcnRxTime + BI2 < now)
        {
            bcnRxTime += BI2;
        }

        if (mac->sfSpec2.BLE)
        {

             tmpf = (mac->beaconPeriods2
                 + Mac802_15_4GetBattLifeExtSlotNum(node,
                                                    interfaceIndex))
                    * aUnitBackoffPeriod * SECOND / rate;

            t_CAP2 = bcnRxTime + tmpf;
        }
        else
        {
            tmpf = (mac->sfSpec2.FinCAP + 1) * sSlotDuration2;
            t_CAP2 = bcnRxTime + tmpf;
        }
    }
    else
    {
        t_CAP2 = oneDay;
    }

    if (toParent) // To parent
    {
        if (t_CAP2 < now || mac->mpib.macBeaconOrder == 15)
        {
            return t_CAP2;
        }
        else
        {
            if (t_CAP2 > t_CAP1)
            {
                return  t_CAP1;
            }
            else
            {
                return t_CAP2;
            }
        }
    }
    else  // To childs and other destinations
    {
        if (t_CAP1 < now || mac->macBeaconOrder2 == 15)
        {
            return t_CAP1;
        }
        else
        {
            if (t_CAP1 > t_CAP2)
            {
                return t_CAP2;
            }
            else
            {
                return t_CAP1;
            }
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4GetCAPbyType
// LAYER      :: Mac
// PURPOSE    :: Returns the start of contention access period
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + type           : Int32       :
// RETURN  :: clocktype
// **/
clocktype Mac802_15_4GetCAPbyType(Node* node, Int32 interfaceIndex, Int32 type)
{
    MacData802_15_4* mac = NULL;
    clocktype bcnTxTime = 0;
    clocktype bcnRxTime = 0;
    clocktype bcnOtherRxTime = 0;
    clocktype bPeriod = 0;
    clocktype sSlotDuration = 0;
    clocktype sSlotDuration2 = 0;
    clocktype sSlotDuration3 = 0;
    clocktype BI2 = 0;
    clocktype BI3 = 0;
    clocktype t_CAP = 0;
    clocktype t_CAP2 = 0;
    clocktype t_CAP3 = 0;
    clocktype now = 0;
    clocktype oneDay = 0;
    clocktype tmpf = 0;
    clocktype len12s = 0;
    Int32 rate = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    now = node->getNodeTime();
    oneDay = now + 24 * 3600 * SECOND;

    if ((mac->mpib.macBeaconOrder == 15)
        && (mac->macBeaconOrder2 == 15)
        && (mac->macBeaconOrder3 == 15))
    {
        return oneDay;
    }

    rate = Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

    bcnTxTime = mac->macBcnTxTime;
    bcnRxTime = mac->macBcnRxTime;
    bcnOtherRxTime = mac->macBcnOtherRxTime;
    len12s = 12 * SECOND / rate;
    bPeriod = aUnitBackoffPeriod * SECOND / rate;
    sSlotDuration = mac->sfSpec.sd * SECOND / rate;
    sSlotDuration2 = mac->sfSpec2.sd * SECOND / rate;
    sSlotDuration3 = mac->sfSpec3.sd * SECOND / rate;
    BI2 = mac->sfSpec2.BI * SECOND / rate;
    BI3 = mac->sfSpec3.BI * SECOND / rate;

    if (type == 1)
    {
        if (mac->mpib.macBeaconOrder != 15)
        {
            if (mac->sfSpec.BLE)
            {

                tmpf = (mac->beaconPeriods +
                        Mac802_15_4GetBattLifeExtSlotNum(node,
                     interfaceIndex)) * aUnitBackoffPeriod * SECOND / rate;

                t_CAP = bcnTxTime + tmpf;
            }
            else
            {
                tmpf = (mac->sfSpec.FinCAP + 1) * sSlotDuration - len12s;
                t_CAP = bcnTxTime + tmpf;
            }
            if (t_CAP >= now)
            {
                return t_CAP;
            }
            else
            {
                return oneDay;
            }
        }
        else
        {
            return oneDay;
        }
    }

    if (type == 2)
    {
        if (mac->macBeaconOrder2 != 15)
        {
            if (mac->sfSpec2.BLE)
            {
                tmpf = (mac->beaconPeriods2 +
                        Mac802_15_4GetBattLifeExtSlotNum(node,
                     interfaceIndex)) * aUnitBackoffPeriod * SECOND / rate;

                t_CAP2 = bcnRxTime + tmpf;
            }
            else
            {
                tmpf = (mac->sfSpec2.FinCAP + 1) * sSlotDuration2;
                t_CAP2 = bcnRxTime + tmpf;
            }

            tmpf = aMaxLostBeacons * BI2;
            if ((t_CAP2 < now) && (t_CAP2 + tmpf >= now)) // no more than
                // <aMaxLostBeacons> beacons missed
            {
                while (t_CAP2 < now)
                {
                    t_CAP2 += BI2;
                }
            }
            if (t_CAP2 >= now)
            {
                return t_CAP2;
            }
            else
            {
                return oneDay;
            }
        }
        else
        {
            return oneDay;
        }
    }

    if (type == 3)
    {
        if (mac->macBeaconOrder3 != 15)
        {
            {
                tmpf = (mac->sfSpec3.FinCAP + 1) * sSlotDuration3;
                t_CAP3 = bcnOtherRxTime + tmpf;
            }

            tmpf = aMaxLostBeacons * BI3;
            if ((t_CAP3 < now) && (t_CAP3 + tmpf >= now))
            {
                while (t_CAP3 < now)
                {
                    t_CAP3 += BI3;
                }
            }
            if (t_CAP3 >= now)
            {
                return t_CAP3;
            }
            else
            {
                return oneDay;
            }
        }
        else
        {
            return oneDay;
        }
    }
    return oneDay;
}

// /**
// FUNCTION   :: Mac802_15_4CheckForOutgoingPacket
// LAYER      :: Mac
// PURPOSE    :: Check if there is any pending packet in Network queue
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4CheckForOutgoingPacket(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    Mac802_15_4NetworkLayerHasPacketToSend(node, mac);
}

// /**
// FUNCTION   :: Mac802_15_4GetSpecks
// LAYER      :: Mac
// PURPOSE    :: Get the respective GTS and superframe specifications of a 
//               FFD/RFD etc
// PARAMETERS ::
// + node : Node* : Node receiving call
// + interfaceIndex : Int32 : Interface index
// + gtsSpec : M802_15_4GTSSpec** : Pointer to Gts specification
// + sfSpec  : M802_15_4SuperframeSpec**: Pointer to superframe 
//                                                 specifications
// + referenceBeaconTime : clocktype *  : Last beacon sent/receive time
// RETURN  :: None
// **/
static
void Mac802_15_4GetSpecks(Node* node,
                          MacData802_15_4* mac,
                          Int32 interfaceIndex,
                          M802_15_4GTSSpec** gtsSpec,
                          M802_15_4SuperframeSpec** sfSpec,
                          clocktype* referenceBeaconTime)
{
    clocktype currentTime = node->getNodeTime();
    Int32 rate = Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
    if (mac->isPANCoor)
    {
        if (sfSpec != NULL)
        {
            *sfSpec = &mac->sfSpec;
        }
        if (referenceBeaconTime != NULL)
        {
            *referenceBeaconTime = mac->macBcnTxTime;
        }
       *gtsSpec = &mac->gtsSpec;
    }
    else if (mac->isCoor || mac->isCoorInDeviceMode)
    {
        // FFD communicating with parent
        if (sfSpec != NULL)
        {
            *sfSpec = &mac->sfSpec2;
        }
        if (referenceBeaconTime != NULL)
        {
            *referenceBeaconTime = mac->macBcnRxTime  - max_pDelay;
        }
        *gtsSpec = &mac->gtsSpec2;
        if (sfSpec && referenceBeaconTime)
        {
            clocktype BI = ((aBaseSuperframeDuration
                * (1 << (*sfSpec)->BO)) * SECOND) / rate;
            while (*referenceBeaconTime + BI < currentTime)
            {
                *referenceBeaconTime += BI;
            }
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4CanProceedInGts
// LAYER      :: Mac
// PURPOSE    :: Check if data packet can be transmitted during GTS
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + mac: MacData802_15_4*      : Pointer to MAC802.15.4 structure
// + i : UInt8                  : Position of the GTS descriptor
// + msg : Message*             : Message that needs to be send
// RETURN  :: BOOL
// **/
BOOL Mac802_15_4CanProceedInGts(Node* node,
                                Int32 interfaceIndex,
                                MacData802_15_4* mac,
                                UInt8 gtsPosition,
                                Message* msg)
{
    M802_15_4SuperframeSpec* sfSpecsToCheck = NULL;
    clocktype referenceBeaconTime = 0;
    M802_15_4GTSSpec* gtsSpecsToCheck = NULL;
    Mac802_15_4GetSpecks(node,
                         mac,
                         interfaceIndex,
                         &gtsSpecsToCheck,
                         &sfSpecsToCheck,
                         &referenceBeaconTime);
    clocktype currentTime = node->getNodeTime();
    Int32 rate = Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
    clocktype sSlotDuration = (sfSpecsToCheck->sd * SECOND) / rate;

    // Check if the packet can be sent in the allocated GTS.
    // The below if condition takes into consideration the currenttime,
    // reference beacon timer (which will be last beacon sent time in case
    // of pan coordinator and last beacon receive time in case of other
    // devices), gts start slot and gts length allocated and propagation
    // delay

    if (((currentTime) >= referenceBeaconTime
                + (gtsSpecsToCheck->slotStart[gtsPosition] * sSlotDuration))
            && ((currentTime) < referenceBeaconTime
                + ((gtsSpecsToCheck->slotStart[gtsPosition]
                + gtsSpecsToCheck->length[gtsPosition]) * sSlotDuration)))
    {
        clocktype t_IFS = 0, t_transacTime = 0;
        if (MESSAGE_ReturnPacketSize(msg) <= aMaxSIFSFrameSize)
        {
            t_IFS = aMinSIFSPeriod * SECOND;
        }
        else
        {
            t_IFS = aMinLIFSPeriod * SECOND;
        }
        t_IFS /= rate;
        t_transacTime += Mac802_15_4TrxTime(node,
                                            interfaceIndex, 
                                            msg);
        t_transacTime += t_IFS;
        if (mac->mpib.macDataAcks)
        {
            t_transacTime += (mac->mpib.macAckWaitDuration) * SECOND
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
        }
        if (!mac->taskP.mcps_gts_data_request_STEP)
        {
            // include the MAC header size for non retries
            t_transacTime += (sizeof(M802_15_4Header) * numBitsPerOctet
                    * SECOND / PHY_GetTxDataRate(node, interfaceIndex));
        }
        if ((currentTime + t_transacTime)
                < (referenceBeaconTime
                    + ((gtsSpecsToCheck->slotStart[gtsPosition]
                    + gtsSpecsToCheck->length[gtsPosition]) * sSlotDuration)))
        {
            return TRUE;
        }
    }
    else
    {
        // Nothing to do in this if condition
    }
    return FALSE;
}


// /**
// FUNCTION   :: Mac802_15_4TaskFailed
// LAYER      :: Mac
// PURPOSE    :: Task Failed
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + task           : char      : Task
// + status         : M802_15_4_enum : MAC status
// + csmacaRes      : BOOL      : Whether to resume CSMA-CA
// RETURN  :: None
// **/
static
void Mac802_15_4TaskFailed(Node* node,
                           Int32 interfaceIndex,
                           char task,
                           M802_15_4_enum status,
                           BOOL csmacaRes)
{
    MacData802_15_4* mac = NULL;
    M802_15_4Header* wph = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if ((task == 'b')   // beacon
         || (task == 'a')    // ack.
         || (task == 'c'))   // command
    {
        // we don't handle the above failures here
        ERROR_Assert(FALSE, "Taskfailed function doesn't handles transmission"
                    " failures related to beacon, ack or command frame to child");
    }
    else if (task == 'C')   // command
    {
        MESSAGE_Free(node, mac->txBcnCmd2);
        mac->txBcnCmd2 = NULL;
        mac->waitBcnCmdAck2 = FALSE;

        if (mac->isDisassociationPending)
        {
            mac->isDisassociationPending = FALSE;
            Sscs802_15_4StopDevice(node,
                                   interfaceIndex,
                                   M802_15_4_SUCCESS);
        }
    }
    else if (task == 'd')   // data
    {
        Int32 rt = 0;
        Message* dupMsg = NULL;
        Message* msg = NULL;
        mac->waitDataAck = FALSE;
        if (status == M802_15_4_TRANSACTION_EXPIRED)
        {
            ERROR_Assert(mac->taskP.mcps_data_request_pendPkt,
                "mac->taskP.mcps_data_request_pendPkt should be non NULL");
            msg = mac->taskP.mcps_data_request_pendPkt;
            if (mac->taskP.mcps_data_request_pendPkt == mac->txData)
            {
                mac->txData = NULL;
            }
        }
        else
        {
            ERROR_Assert(mac->txData,"mac->txData should point to last sent"
                         " data packet");
            msg = mac->txData;
            mac->txData = NULL;
        }
        dupMsg = MESSAGE_Duplicate(node, msg);
        wph = (M802_15_4Header*) MESSAGE_ReturnPacket(dupMsg);
        if (status == M802_15_4_TRANSACTION_EXPIRED)
        {
            rt = Mac802_15_4UpdateTransacLinkByPktOrHandle(
                                                node,
                                                OPER_DELETE_TRANSAC,  // del
                                                &mac->transacLink1,
                                                &mac->transacLink2,
                                                msg,
                                                0);
            if (rt != 0)
            {
                ERROR_Assert(FALSE, "No matching item found in transaction list");
            }
       }
       else if (status == M802_15_4_CHANNEL_ACCESS_FAILURE)
       {
           if (mac->taskP.mcps_data_request_TxOptions & TxOp_Indirect)
           {
               // Do not drop or notify the higher layer in case of channel
               // access failure in indirect mode. Wait for another poll
               // request or transaction expire timer to fire.

               MESSAGE_Free(node, dupMsg);
               dupMsg = NULL;
               return;
           }
           else
           {
               mac->stats.numDataPktDroppedChannelAccessFailure++;
               MESSAGE_Free(node, msg);
               msg = NULL;
           }
       }
       else if (status == M802_15_4_NO_ACK)
       {
            mac->stats.numDataPktDroppedNoAck++;
            MESSAGE_Free(node, msg);
            msg = NULL;
       }
       else
       {
            MESSAGE_Free(node, msg);
            msg = NULL;
       }

        MacHWAddress macHWAddr;
        MESSAGE_RemoveHeader(node,
                             dupMsg,
                             sizeof(M802_15_4Header),
                             TRACE_MAC_802_15_4);
        ConvertMacAddrToVariableHWAddress(node,
                                          &macHWAddr,
                                          &wph->MHR_DstAddrInfo.addr_64);
        MAC_NotificationOfPacketDrop(node,
                                     macHWAddr,
                                     interfaceIndex,
                                     dupMsg);
        mac->stats.numPktDropped++;
        if (!mac->txData)
        {
            Mac802_15_4CheckForOutgoingPacket(node, interfaceIndex);
            Mac802_15_4CsmacaResume(node, interfaceIndex);
        }
    }
    else if (task == 'g')   // gts data
    {
        ERROR_Assert(mac->txGts,"mac->txGts should point to last sent data"
                     " during GTS");
        ERROR_Assert(mac->currentGtsPositionDesc
            != M802_15_4_DEFAULTGTSPOSITION,"currentGtsPositionDesc should"
            " have a default value");
        
        M802_15_4GTSSpec* tmpgtsSpec = NULL;
        Mac802_15_4GetSpecks(node,
                             mac,
                             interfaceIndex,
                             &tmpgtsSpec,
                             NULL,
                             NULL);
        ERROR_Assert(tmpgtsSpec->msg[mac->currentGtsPositionDesc]
            == mac->txGts,"The message pointers should match");

        mac->txGts = NULL;
        mac->waitDataAck = FALSE;
        tmpgtsSpec->retryCount[mac->currentGtsPositionDesc] = 0;
        if (status == M802_15_4_NO_ACK)
        {
            mac->stats.numDataPktDroppedNoAck++;
        }
        else
        {
            ERROR_Assert(FALSE,"Invalid status");
        }
        M802_15_4Header* wph = NULL;
        MacHWAddress macHWAddr;
        wph = (M802_15_4Header*) MESSAGE_ReturnPacket(
                               tmpgtsSpec->msg[mac->currentGtsPositionDesc]);
        ConvertMacAddrToVariableHWAddress(node,
                                          &macHWAddr,
                                          &wph->MHR_DstAddrInfo.addr_64);
        MESSAGE_RemoveHeader(node,
                             tmpgtsSpec->msg[mac->currentGtsPositionDesc],
                             sizeof(M802_15_4Header),
                             TRACE_MAC_802_15_4);

        Message* msgPtr = tmpgtsSpec->msg[mac->currentGtsPositionDesc];
        tmpgtsSpec->msg[mac->currentGtsPositionDesc] = NULL;
        mac->currentGtsPositionDesc = M802_15_4_DEFAULTGTSPOSITION;

        MAC_NotificationOfPacketDrop(
                             node,
                             macHWAddr,
                             interfaceIndex,
                             msgPtr);
    }
    else
    {
        ERROR_Assert(FALSE, "Unknown packet type");
    }

    if (csmacaRes)
    {
        Mac802_15_4CsmacaResume(node, interfaceIndex);
    }
}

// /**
// FUNCTION   :: Mac802_15_4CreateGtsChracteristics
// LAYER      :: Mac
// PURPOSE    :: Generates GTS characteristics
// PARAMETERS ::
// + gtsRequestType           : BOOL     : Gts request type
// + numslots : UInt8       : Number of slots
// + gtsDirection           : BOOL      : Gts direction
// RETURN  :: UInt8
// **/
static
UInt8  Mac802_15_4CreateGtsChracteristics(BOOL gtsRequestType,
                                          UInt8 numslots,
                                          BOOL gtsDirection)
{
    UInt8 gtsChracteristics = 0;
    gtsChracteristics += numslots;
    gtsChracteristics |= (gtsDirection) ? 16 : 0;
    gtsChracteristics |= (gtsRequestType) ? 32 : 0;
    return gtsChracteristics;
}


// /**
// FUNCTION   :: Mac802_15_4TaskSuccess
// LAYER      :: Mac
// PURPOSE    :: Task finalization
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + task           : char      : Task
// + csmacaRes      : BOOL      : Whether to resume CSMA-CA
// RETURN  :: None
// **/
static
void Mac802_15_4TaskSuccess(Node* node,
                            Int32 interfaceIndex,
                            char task,
                            BOOL csmacaRes)
{
    MacData802_15_4* mac = NULL;
    UInt16 t_CAP = 0;
    UInt8 ifs = 0;
    double tmpf = 0;
    
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (task == 'b')    // beacon
    {
        if (!mac->txBeacon)
        {
            ERROR_Assert(mac->txBcnCmd2,"mac->txBcnCmd2 should point to the"
                         " last sent command");
            mac->txBeacon = mac->txBcnCmd2;
            mac->txBcnCmd2 = NULL;
        }

        // --- calculate CAP ---
        Mac802_15_4SuperFrameParse(&mac->sfSpec);
        if (MESSAGE_ReturnPacketSize(mac->txBeacon) <= aMaxSIFSFrameSize)
        {
            ifs = aMinSIFSPeriod;
        }
        else
        {
            ifs = aMinLIFSPeriod;
        }

        {
            tmpf = (double)(Mac802_15_4TrxTime(node,
                                               interfaceIndex,
                                               mac->txBeacon)
                   * Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex])
                   / SECOND);
            tmpf += ifs;
            mac->beaconPeriods = (UInt8)(tmpf
                                    / aUnitBackoffPeriod );
        }

         tmpf = (double)(Mac802_15_4TrxTime(node,
                                            interfaceIndex,
                                            mac->txBeacon)
                * Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex])
                / SECOND);
        tmpf += ifs;
        if (fmod(tmpf,aUnitBackoffPeriod) > 0.0)
        {
            mac->beaconPeriods++;
        }

        t_CAP = (mac->sfSpec.FinCAP + 1)
                    * ((UInt16)mac->sfSpec.sd / aUnitBackoffPeriod)
                    - mac->beaconPeriods;

        if (t_CAP == 0)
        {
            csmacaRes = FALSE;
            mac->trx_state_req = TRX_OFF;
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               TRX_OFF);
        }
        else
        {
            mac->trx_state_req = RX_ON;
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               RX_ON);
        }
        // CSMA-CA may be waiting for the new beacon
        if (mac->backoffStatus == BACKOFF)
        {
            Csma802_15_4NewBeacon(node, interfaceIndex,'t');
        }
        mac->beaconWaiting = FALSE;
        if (mac->txBeacon)
        {
            MESSAGE_Free(node, mac->txBeacon);
            mac->txBeacon = NULL;
        }
    }
    else if (task == 'a')   // ack.
    {
        ERROR_Assert(mac->txAck, "mac->txAck should point to last sent ack");
        if (mac->txAck)
        {
            MESSAGE_Free(node, mac->txAck);
            mac->txAck = NULL;
        }
    }
    else if (task == 'c')   // command
    {
        Int32 rt = 0;
        ERROR_Assert(mac->txBcnCmd, "mac->txBcnCmd should point to last sent"
                     " command");

        // if it is a pending packet, delete it from the pending list
        rt = Mac802_15_4UpdateTransacLinkByPktOrHandle(
                                            node,
                                            OPER_DELETE_TRANSAC,
                                            &mac->transacLink1,
                                            &mac->transacLink2,
                                            mac->txBcnCmd,
                                            0);
        if (rt != 0)
        {
            MESSAGE_Free(node, mac->txBcnCmd);
        }
        mac->txBcnCmd = NULL;
        mac->waitBcnCmdAck = FALSE;
    }
    else if (task == 'C')   // command
    {
        ERROR_Assert(mac->txBcnCmd2, "mac->txBcnCmd2 should point to last sent"
                     " command");
        if (mac->txBcnCmd2)
        {
            MESSAGE_Free(node, mac->txBcnCmd2);
            mac->txBcnCmd2 = NULL;
        }
        mac->waitBcnCmdAck2 = FALSE;

        if (mac->isDisassociationPending)
        {
            mac->isDisassociationPending = FALSE;
            Sscs802_15_4StopDevice(node,
                                   interfaceIndex,
                                   M802_15_4_SUCCESS);
        }
        else if (mac->sendGtsRequestToPancoord)
        {

            // initiate GTS request
            if (!mac->gtsRequestData.receiveOnly)
            {
                return;
            }
            mac->sendGtsRequestToPancoord  = FALSE;
            ERROR_Assert(mac->gtsRequestData.active,"mac->gtsRequestData.active"
                         " should be TRUE at this point");
            
            UInt8 gtsChracteristics
                    = Mac802_15_4CreateGtsChracteristics(
                                      mac->gtsRequestData.allocationRequest,
                                      mac->gtsRequestData.numSlots,
                                      mac->gtsRequestData.receiveOnly);
            Mac802_15_4MLME_GTS_request(node,
                                        interfaceIndex,
                                        gtsChracteristics,
                                        FALSE,
                                        PHY_SUCCESS);
        }
        else if (mac->gtsRequestPending && !mac->sendGtsConfirmationPending)
        {
              mac->gtsRequestPending = FALSE;
              ERROR_Assert(!mac->gtsRequestData.active,"mac->gtsRequestData.active"
                           " should be FALSE at this point");
               ERROR_Assert(mac->gtsRequestPendingData.active,
                            "mac->gtsRequestPendingData.active should be TRUE "
                            " at this point");
               memcpy(&(mac->gtsRequestData),
                      &(mac->gtsRequestPendingData),
                      sizeof(GtsRequestData));
               mac->gtsRequestPendingData.active = FALSE;
               UInt8 gtsChracteristics
                   = Mac802_15_4CreateGtsChracteristics(
                                      mac->gtsRequestData.allocationRequest,
                                      mac->gtsRequestData.numSlots,
                                      mac->gtsRequestData.receiveOnly);
                Mac802_15_4MLME_GTS_request(node,
                                            interfaceIndex,
                                            gtsChracteristics,
                                            FALSE,
                                            PHY_SUCCESS);
        }
    }
    else if (task == 'd')   // data
    {
        Message* p = mac->txData;
        Int32 rt = 0;

        M802_15_4Header* wph = NULL;
        wph = (M802_15_4Header*) MESSAGE_ReturnPacket(p);
         MAC_MacLayerAcknowledgement(
            node,
            interfaceIndex,
            p,
            MAPPING_GetDefaultInterfaceAddressFromNodeId(
                    node, wph->MHR_DstAddrInfo.addr_64));

        // if it is a pending packet, delete it from the pending list
        rt = Mac802_15_4UpdateTransacLinkByPktOrHandle(
                                                node,
                                                OPER_DELETE_TRANSAC,
                                                &mac->transacLink1,
                                                &mac->transacLink2,
                                                p,
                                                0);
        if (rt != 0)
        {
            MESSAGE_Free(node, mac->txData);
        }
        mac->txData = NULL;
        mac->waitDataAck = FALSE;
        
        // Mac802_15_4CheckForOutgoingPacket(node, interfaceIndex);
        if (mac->sendGtsRequestToPancoord && !mac->sendGtsConfirmationPending)
        {
            // initiate GTS requesst
            ERROR_Assert(!mac->gtsRequestData.receiveOnly,
                        "mac->gtsRequestData.receiveOnly should be FALSE"
                        " at this point");
            mac->sendGtsRequestToPancoord = FALSE;
            ERROR_Assert(mac->gtsRequestData.active,"mac->gtsRequestData.active"
                         " should be TRUE at this point");
            UInt8 gtsChracteristics
                = Mac802_15_4CreateGtsChracteristics(
                                       mac->gtsRequestData.allocationRequest,
                                       mac->gtsRequestData.numSlots,
                                       mac->gtsRequestData.receiveOnly);
            Mac802_15_4MLME_GTS_request(node,
                                        interfaceIndex,
                                        gtsChracteristics,
                                        FALSE,
                                        PHY_SUCCESS);
        }
        else
        {
            Mac802_15_4CheckForOutgoingPacket(node, interfaceIndex);
        }
    }
    else if (task == 'g') // gts data
    {
       ERROR_Assert(mac->txGts,"mac->txGts should point to last sent"
                    " data packet during GTS");
       ERROR_Assert(mac->currentGtsPositionDesc
           != M802_15_4_DEFAULTGTSPOSITION,"currentGtsPositionDesc should"
           " not be set to the default value");
       clocktype t_IFS = 0;
        if (MESSAGE_ReturnPacketSize(mac->txGts) <= aMaxSIFSFrameSize)
        {
            t_IFS = aMinSIFSPeriod * SECOND;
        }
        else
        {
            t_IFS = aMinLIFSPeriod * SECOND;
        }
        t_IFS /= Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
        mac->CheckPacketsForTransmission = TRUE;
        Mac802_15_4SetTimer(node,
                            mac,
                            M802_15_4IFSTIMER,
                            t_IFS,
                            NULL);

        M802_15_4GTSSpec* tmpgtsSpec = NULL;
        Mac802_15_4GetSpecks(node,
                             mac,
                             interfaceIndex,
                             &tmpgtsSpec,
                             NULL,
                             NULL);
        ERROR_Assert(tmpgtsSpec->msg[mac->currentGtsPositionDesc]
                            == mac->txGts,"The message pointers should match");
        MESSAGE_Free(node, tmpgtsSpec->msg[mac->currentGtsPositionDesc]);
        tmpgtsSpec->msg[mac->currentGtsPositionDesc] = NULL;
        mac->txGts = NULL;
        mac->currentGtsPositionDesc = M802_15_4_DEFAULTGTSPOSITION;
        mac->waitDataAck = FALSE;
        tmpgtsSpec->retryCount[mac->currentGtsPositionDesc] = 0;
    }
    else
    {
        ERROR_Assert(FALSE, "Invalid packet type");
    }

    if (csmacaRes == TRUE)
    {
        Mac802_15_4CsmacaResume(node, interfaceIndex);
    }
}

// /**
// FUNCTION   :: Mac802_15_4CsmacaBegin
// LAYER      :: Mac
// PURPOSE    :: Begin CSMA-CA
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + pktType        : char      : Type of packet for which CSMA has to be
//                                  started
// RETURN  :: None
// **/
static
void Mac802_15_4CsmacaBegin(Node* node,
                            Int32 interfaceIndex,
                            char pktType)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                "Starting CSMA-CA\n",
                node->getNodeTime(), node->nodeId);
    }

    if (pktType == 'c')     // txBcnCmd
    {
        mac->waitBcnCmdAck = FALSE;  // beacon packet not yet transmitted
        mac->numBcnCmdRetry = 0;
        if (mac->backoffStatus == BACKOFF)        // backoffing for data packet
        {
            mac->backoffStatus = BACKOFF_RESET;
            Csma802_15_4Cancel(node, interfaceIndex);
        }
        Mac802_15_4CsmacaResume(node, interfaceIndex);
    }
    else if (pktType == 'C')    // txBcnCmd2
    {
        mac->waitBcnCmdAck2 = FALSE; // command packet not yet transmitted
        mac->numBcnCmdRetry2 = 0;
        if ((mac->backoffStatus == BACKOFF) // backoffing for data packet
                && (mac->txCsmaca != mac->txBcnCmd))
        {
            mac->backoffStatus = BACKOFF_RESET;
            Csma802_15_4Cancel(node, interfaceIndex);
        }
        Mac802_15_4CsmacaResume(node, interfaceIndex);
    }
    else // if (pktType == 'd')  // txData
    {
        mac->waitDataAck = FALSE; // data packet not yet transmitted
        mac->numDataRetry = 0;
        Mac802_15_4CsmacaResume(node, interfaceIndex);
    }
}
// /**
// FUNCTION   :: Mac802_15_4Reset_TRX
// LAYER      :: Mac
// PURPOSE    :: Reset Transceiver
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4Reset_TRX(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    PLMEsetTrxState t_state;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (mac->mpib.macRxOnWhenIdle || mac->isPANCoor || mac->waitBcnCmdAck2
        || mac->waitBcnCmdAck || mac->waitDataAck)
    {
         t_state = RX_ON;
    }
    else
    {
         t_state = TRX_OFF;
    }
    mac->trx_state_req = t_state;
    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                       interfaceIndex,
                                       t_state);
}



// /**
// FUNCTION   :: Mac802_15_4ParseGtsRequestChracteristics
// LAYER      :: Mac
// PURPOSE    :: Parses GTS characteristics
// PARAMETERS ::
// + gtsReqChrcts : UInt8*         : GTS chracteristics
// + gtsReqData : GtsRequestData*  : Pointer to the GTS request datastructure
// RETURN  :: None
// **/
static 
void Mac802_15_4ParseGtsRequestChracteristics(const UInt8 gtsReqChrcts,
                                              GtsRequestData *gtsReqData)
{
    gtsReqData->receiveOnly = (gtsReqChrcts & 16) ? 1 : 0;
    gtsReqData->numSlots = gtsReqChrcts & 15;
    gtsReqData->allocationRequest = gtsReqChrcts & 32? 1 : 0;
}

// --------------------------------------------------------------------------
//  Utility functions
// --------------------------------------------------------------------------
// /**
// NAME         ::Mac802_15_4RemoveGtsDescriptorFromDevice
// PURPOSE      ::Removes a GTS descriptor saved at a device
// PARAMETERS   ::
// + node : Node*               : Node which sends the message
// + mac : MacData_802_15_4*    : 802.15.4 data structure
// + j : Int32                    : The position of GTS descriptot in the array
// + confirmationPending : BOOL : Specifies if node is waiting for a GTS 
//                                confirmation from the pancoord via beacons
// RETURN       ::None
// NOTES        ::None
// **/
static
void Mac802_15_4RemoveGtsDescriptorFromDevice(Node* node,
                                              MacData802_15_4* mac,
                                              Int32 j,
                                              BOOL confirmationPending)
{
    while (!mac->gtsSpec2.queue[j]->isEmpty())
    {
        // Drop all packets in this GTS queue
        Message* msg = NULL;
        mac->gtsSpec2.queue[j]->retrieve(&msg,
                                         DEQUEUE_NEXT_PACKET,
                                         DEQUEUE_PACKET,
                                         0,
                                         NULL);
        mac->stats.numDataPktsDeQueuedForGts++;
        mac->stats.numPktDropped++;
        MESSAGE_Free(node, msg);
        msg = NULL;
    }
    if (j == mac->gtsSpec2.count - 1)
    {
        // no reshuffle
        mac->gtsSpec2.endTime[j] = 0;
        if (confirmationPending)
        {
            ERROR_Assert(!mac->gtsSpec2.deAllocateTimer[j], "GTS deallocation"
                         " for this GTS should not be set at this point");
        }
        else
        {
            if (mac->gtsSpec2.deAllocateTimer[j])
            {
                MESSAGE_CancelSelfMsg(node, mac->gtsSpec2.deAllocateTimer[j]);
                mac->gtsSpec2.deAllocateTimer[j] = NULL;
            }
        }

        mac->gtsSpec2.fields.list[j].devAddr16 = 0;
        mac->gtsSpec2.appPort[j] = 0;
        Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec2,
                                       j,
                                       0);
        Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec2,
                                      j,
                                      0);
        Mac802_15_4GTSSpecSetLength(&mac->gtsSpec2,
                                    j,
                                    0);
        Mac802_15_4GTSSpecSetCount(&mac->gtsSpec2,
                                   mac->gtsSpec2.count - 1);
    }
    else
    {
        // change required in the relative position of the descriptor saved
        // in the array

        if (confirmationPending)
        {
            ERROR_Assert(!mac->gtsSpec2.deAllocateTimer[j], "GTS deallocation"
                         " for this GTS should not be set at this point");
        }
        else
        {
            if (mac->gtsSpec2.deAllocateTimer[j])
            {
                MESSAGE_CancelSelfMsg(node, mac->gtsSpec2.deAllocateTimer[j]);
                mac->gtsSpec2.deAllocateTimer[j] = NULL;
            }
        }
        Int32 m = 0;
        for (m = j; m < mac->gtsSpec2.count - 1; m++)
        {
            mac->gtsSpec2.fields.list[m].devAddr16
                = mac->gtsSpec2.fields.list[m + 1].devAddr16;
            mac->gtsSpec2.appPort[m] = mac->gtsSpec2.appPort[m + 1];
            Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec2,
                                            m,
                                            mac->gtsSpec2.slotStart[m+1]);
            Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec2,
                                           m,
                                           mac->gtsSpec2.recvOnly[m+1]);
            Mac802_15_4GTSSpecSetLength(&mac->gtsSpec2,
                                        m,
                                        mac->gtsSpec2.length[m+1]);
            mac->gtsSpec2.endTime[m] = mac->gtsSpec2.endTime[m + 1];
            mac->gtsSpec2.deAllocateTimer[m] = mac->gtsSpec2.deAllocateTimer[m+1];
            while (!mac->gtsSpec2.queue[m+1]->isEmpty())
            {
                // Re-queue the packets at the new location
                Message* msg = NULL;
                BOOL isFull = FALSE;
                mac->gtsSpec2.queue[m+1]->retrieve(&msg,
                                                   DEQUEUE_NEXT_PACKET,
                                                   DEQUEUE_PACKET,
                                                   0,
                                                   NULL);
                mac->gtsSpec2.queue[m]->insert(msg,
                                               NULL,
                                               &isFull,
                                               0,
                                               0,
                                               0);
                if (!isFull)
                {
                    // nothing to do
                }
                else
                {
                    ERROR_Assert(FALSE," GTS queue is FULL");
                }
            }
        }

        mac->gtsSpec2.endTime[mac->gtsSpec2.count - 1] = 0;
        mac->gtsSpec2.fields.list[mac->gtsSpec2.count - 1].devAddr16 = 0;
        mac->gtsSpec2.appPort[mac->gtsSpec2.count - 1] = 0;
        Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec2,
                                       mac->gtsSpec2.count - 1,
                                       0);
        Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec2,
                                      mac->gtsSpec2.count - 1,
                                      0);
        Mac802_15_4GTSSpecSetLength(&mac->gtsSpec2,
                                    mac->gtsSpec2.count - 1,
                                    0);
        Mac802_15_4GTSSpecSetCount(&mac->gtsSpec2,
                                   mac->gtsSpec2.count - 1);
    }
}

void Mac802_15_4MLME_GTS_request(Node* node,
                                 Int32 interfaceIndex,
                                 UInt8 GTSCharacteristics,
                                 BOOL SecurityEnable,
                                 PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    M802_15_4FrameCtrl frmCtrl;
    UInt8 step = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    step = mac->taskP.mlme_gts_request_STEP;
    switch(step)
    {
        case GTS_REQUEST_INIT_STEP:
        {
            if (mac->txBcnCmd2)
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                           " Not gtsing"
                           " the coordinator as txBcnCmd2 is not NULL\n",
                           node->getNodeTime(),
                           node->nodeId);
                }
                mac->sendGtsRequestToPancoord = TRUE;
                return;
            }
            mac->gtsRequestData.active = FALSE;

            // check invalid parameters
            if (((mac->panDes2.CoordAddrMode
                    != M802_15_4DEFFRMCTRL_ADDRMODE16)
                && (mac->panDes2.CoordAddrMode
                    != M802_15_4DEFFRMCTRL_ADDRMODE64))
                || (mac->panDes2.CoordPANId == 0xffff))
            {
                return;
            } 

            mac->taskP.mlme_gts_request_STEP++;
            strcpy(mac->taskP.mlme_gts_request_frFunc,
                    "csmacaCallBack");

            // send a GTS request command
            Mac802_15_4ConstructPayload(node,
                                        interfaceIndex,
                                        &mac->txBcnCmd2,
                                        0x03,   // cmd
                                        0,
                                        0,
                                        0,
                                        0x09);  // GTS request

            UInt8* gtschracs = (UInt8*) MESSAGE_ReturnPacket(mac->txBcnCmd2)
                                    + sizeof(M802_15_4CommandFrame);

            // one slot requested
            // send only GTS
            // gts allocation
            // 00100001 = 33

            *gtschracs = GTSCharacteristics;

            ERROR_Assert(!mac->taskP.gtsChracteristics,"gtsChracteristics"
                         " should be 0");
            mac->taskP.gtsChracteristics = *gtschracs;
            Mac802_15_4AddCommandHeader(node,
                                        interfaceIndex,
                                        mac->txBcnCmd2,
                                        0x03,
                                        M802_15_4DEFFRMCTRL_ADDRMODE16,
                                        mac->mpib.macPANId,
                                        mac->mpib.macCoordShortAddress,
                                        M802_15_4DEFFRMCTRL_ADDRMODE16,
                                        mac->mpib.macPANId,
                                        mac->aExtendedAddress,
                                        SecurityEnable,
                                        FALSE,
                                        TRUE,
                                        0x09,
                                        0);

            if ((mac->mpib.macShortAddress != 0xfffe)
                && (mac->mpib.macShortAddress != 0xffff))
            {
                ((M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd2))->
                        MHR_SrcAddrInfo.addr_16
                    = mac->mpib.macShortAddress;

                frmCtrl.frmCtrl
                    = ((M802_15_4Header*) MESSAGE_ReturnPacket(
                            mac->txBcnCmd2))->MHR_FrmCtrl;
                Mac802_15_4FrameCtrlParse(&frmCtrl);

                Mac802_15_4FrameCtrlSetSrcAddrMode(
                                &frmCtrl,
                                M802_15_4DEFFRMCTRL_ADDRMODE16);
                                ((M802_15_4Header*) MESSAGE_ReturnPacket(
                                        mac->txBcnCmd2))->MHR_FrmCtrl
                    = frmCtrl.frmCtrl;
            }
            Mac802_15_4CsmacaBegin(node, interfaceIndex, 'C');
            break;
        }
        case GTS_REQUEST_CSMA_STATUS_STEP:
        {
            if (status == PHY_IDLE)
            {
                // Update the stats only when CSMA issues go ahead
                mac->taskP.mlme_gts_request_STEP++;
                strcpy(mac->taskP.mlme_gts_request_frFunc,
                        "PD_DATA_confirm");
                mac->trx_state_req = TX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX);
            }
            else
            {
                mac->taskP.mlme_gts_request_STEP = GTS_REQUEST_INIT_STEP;
                mac->taskP.gtsChracteristics = 0;
                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4TaskFailed(node,
                                      interfaceIndex,
                                      'C',
                                      M802_15_4_CHANNEL_ACCESS_FAILURE,
                                      TRUE);
            }
            break;
        }
        case GTS_REQUEST_PKT_SENT_STEP:
        {
            mac->taskP.mlme_gts_request_STEP++;
            strcpy(mac->taskP.mlme_gts_request_frFunc, "recvAck");

            // enable the receiver
            mac->trx_state_req = RX_ON;
            Phy802_15_4PlmeSetTRX_StateRequest(
                    node,
                    interfaceIndex,
                    RX_ON);

             mac->txT
                 = Mac802_15_4SetTimer(
                                    node,
                                    mac,
                                    M802_15_4TXTIMER,
                                    mac->mpib.macAckWaitDuration * SECOND
                                      / Phy802_15_4GetSymbolRate(
                                            node->phyData[interfaceIndex]),
                                    NULL);

            mac->waitBcnCmdAck2 = TRUE;
            break;
        }
        case GTS_REQUEST_ACK_STATUS_STEP:
        {
            if (status == PHY_SUCCESS)  // ack. received
            {
                mac->taskP.mlme_gts_request_STEP = GTS_REQUEST_INIT_STEP;
                GtsRequestData tempData;
                Mac802_15_4ParseGtsRequestChracteristics(
                    mac->taskP.gtsChracteristics, &tempData);
                mac->sendGtsConfirmationPending = TRUE;
                if (!tempData.allocationRequest)
                {
                    mac->sendGtsConfirmationPending = FALSE;
                    mac->taskP.gtsChracteristics = 0;
                    Int32 i = 0;
                    for (i = 0; i < mac->gtsSpec2.count; i++)
                    {
                        if (mac->gtsSpec2.recvOnly[i] == tempData.receiveOnly)
                        {
                            // Remove the saved GTS descriptor
                            if (i == 0)
                            {
                                if (mac->gtsSpec2.count == 1)
                                {
                                    if (mac->gtsT)
                                    {
                                        MESSAGE_CancelSelfMsg(node, mac->gtsT);
                                        mac->gtsT = NULL;
                                    }
                                }
                            }
                            else
                            {
                                ERROR_Assert(i == 1, "Invalid operation. i"
                                             " should be <= 1");
                                if (mac->gtsT)
                                {
                                    MESSAGE_CancelSelfMsg(node, mac->gtsT);
                                    mac->gtsT = NULL;
                                }
                                if (mac->numLostBeacons == 0)
                                {
                                    Int32 rate
                                        = Phy802_15_4GetSymbolRate(
                                                node->phyData[interfaceIndex]);
                                    clocktype sSlotDuration
                                         = (mac->sfSpec2.sd * SECOND) / rate;
                                    clocktype timeSinceLastBeacon
                                        = node->getNodeTime() - mac->macBcnRxTime;
                                    clocktype delay = mac->gtsSpec2.slotStart[0]
                                                        * sSlotDuration;
                                    delay -= timeSinceLastBeacon;
                                    delay -= max_pDelay;
                                    ERROR_Assert(delay > 0,"delay should be"
                                                 " more than zero");
                                    Message* timerMsg = NULL;
                                    timerMsg = MESSAGE_Alloc(node,
                                                   MAC_LAYER,
                                                   MAC_PROTOCOL_802_15_4,
                                                   MSG_MAC_TimerExpired);

                                    MESSAGE_SetInstanceId(timerMsg, (short)
                                    mac->myMacData->interfaceIndex);

                                    MESSAGE_InfoAlloc(node,
                                                      timerMsg,
                                                      sizeof(M802_15_4Timer));
                                    M802_15_4Timer* timerInfo
                                        = (M802_15_4Timer*)
                                            MESSAGE_ReturnInfo(timerMsg);

                                    timerInfo->timerType = M802_15_4GTSTIMER;
                                    MESSAGE_AddInfo(node,
                                                    timerMsg,
                                                    sizeof(UInt8),
                                                    INFO_TYPE_Gts_Slot_Start);
                                    char* gtsSlotInfo
                                        = MESSAGE_ReturnInfo(timerMsg,
                                                    INFO_TYPE_Gts_Slot_Start);
                                    *gtsSlotInfo = mac->gtsSpec2.slotStart[0];
                                    mac->gtsT = timerMsg;
                                    MESSAGE_Send(node, timerMsg, delay);
                                }
                            }

                            Mac802_15_4RemoveGtsDescriptorFromDevice(node,
                                                                     mac,
                                                                     i,
                                                                     TRUE);
                        }
                    }
                }
                if (tempData.allocationRequest)
                {
                   mac->stats.numGtsAllocationReqSent++;
                }
                else
                {
                   mac->stats.numGtsDeAllocationReqSent++;
                }
                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', TRUE);
            }
            else // time out when waiting for ack.
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC "
                                        ": Ack not received"
                                         " for gts request\n",
                                          node->getNodeTime(),
                                          node->nodeId);
                }

                mac->numBcnCmdRetry2++;
                if (mac->numBcnCmdRetry2 <= aMaxFrameRetries)
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: "
                                                "802.15.4MAC : Retrying"
                                                " gts request\n",
                                                node->getNodeTime(),
                                                node->nodeId);
                    }

                    mac->taskP.mlme_gts_request_STEP
                        = GTS_REQUEST_CSMA_STATUS_STEP;
                    strcpy(mac->taskP.mlme_gts_request_frFunc,
                            "csmacaCallBack");
                    mac->waitBcnCmdAck2 = FALSE;
                    mac->stats.numGtsReqRetried++;
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                }
                else
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15."
                                                  "4MAC : gts request"
                                                  " retries Exhausted\n",
                                                  node->getNodeTime(),
                                                  node->nodeId);
                    }

                    mac->taskP.mlme_gts_request_STEP = GTS_REQUEST_INIT_STEP;
                    mac->taskP.gtsChracteristics = 0;
                    Mac802_15_4Reset_TRX(node, interfaceIndex);

                    Mac802_15_4TaskFailed(node,
                                          interfaceIndex,
                                          'C',
                                          M802_15_4_NO_ACK,
                                          TRUE);
                }
            }
            break;
        }
        default:
        {
           break;
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4Dispatch
// LAYER      :: Mac
// PURPOSE    :: Dispatches an incoming event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + status         : PhyStatusType   : Phy status
// + frFunc         : char*     : Function name from which dispatch() has
//                                been called
// + req_state      : PLMEsetTrxState   :
// + mStatus        : M802_15_4_enum : MAC layer status
// RETURN  :: None
// **/
void Mac802_15_4Dispatch(Node* node,
                         Int32 interfaceIndex,
                         PhyStatusType status,
                         const char* frFunc,
                         PLMEsetTrxState req_state,
                         M802_15_4_enum mStatus)
{
    MacData802_15_4* mac = NULL;
    M802_15_4FrameCtrl frmCtrl;
    M802_15_4Header* wph = NULL;
    UInt8 ifs = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (strcmp(frFunc,"csmacaCallBack") == 0)
    {
        if (mac->txCsmaca == mac->txBcnCmd2)
        {
            if (mac->taskP.mlme_scan_request_STEP &&
                (strcmp(mac->taskP.mlme_scan_request_frFunc,frFunc) == 0))
            {
                if ((mac->taskP.mlme_scan_request_ScanType == 0x01) ||
                     (mac->taskP.mlme_scan_request_ScanType == 0x03))
                {
                    Mac802_15_4mlme_scan_request(
                                node,
                                interfaceIndex,
                                mac->taskP.mlme_scan_request_ScanType,
                                mac->taskP.mlme_scan_request_ScanChannels,
                                mac->taskP.mlme_scan_request_ScanDuration,
                                FALSE,
                                status);
                }
            }
            else if (mac->taskP.mlme_start_request_STEP &&
                     (strcmp(mac->taskP.mlme_start_request_frFunc,frFunc)
                       == 0))
            {
                Mac802_15_4mlme_start_request(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_start_request_PANId,
                        mac->taskP.mlme_start_request_LogicalChannel,
                        mac->taskP.mlme_start_request_BeaconOrder,
                        mac->taskP.mlme_start_request_SuperframeOrder,
                        mac->taskP.mlme_start_request_PANCoordinator,
                        mac->taskP.mlme_start_request_BatteryLifeExtension,
                        0,
                        mac->taskP.mlme_start_request_SecurityEnable,
                        FALSE,
                        status);
            }
            else if (mac->taskP.mlme_associate_request_STEP &&
                    (strcmp(mac->taskP.mlme_associate_request_frFunc,
                                frFunc) == 0))
            {
                Mac802_15_4mlme_associate_request(
                        node,
                        interfaceIndex,
                        0,
                        0,
                        0,
                        0,
                        0,
                        mac->taskP.mlme_associate_request_SecurityEnable,
                        FALSE,
                        status,
                        M802_15_4_SUCCESS);
            }
            else if (mac->taskP.mlme_disassociate_request_STEP &&
                     (strcmp(mac->taskP.mlme_disassociate_request_frFunc,
                      frFunc) == 0))
            {
                Mac802_15_4mlme_disassociate_request(node,
                                                     interfaceIndex,
                                                     0,
                                                     0,
                                                     FALSE,
                                                     FALSE,
                                                     status);
            }

            else if (mac->taskP.mlme_poll_request_STEP &&
                     (strcmp(mac->taskP.mlme_poll_request_frFunc, frFunc)
                        == 0))
            {
                Mac802_15_4mlme_poll_request(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_poll_request_CoordAddrMode,
                        mac->taskP.mlme_poll_request_CoordPANId,
                        mac->taskP.mlme_poll_request_CoordAddress,
                        mac->taskP.mlme_poll_request_SecurityEnable,
                        mac->taskP.mlme_poll_request_autoRequest,
                        FALSE,
                        status);
            }
            else if (mac->taskP.mlme_gts_request_STEP &&
                     (strcmp(mac->taskP.mlme_gts_request_frFunc, frFunc)
                        == 0))
            {
                Mac802_15_4MLME_GTS_request(node,
                                            interfaceIndex,
                                            0,
                                            FALSE,
                                            status);
            }
            else    // default handling for txBcnCmd2
            {
                if (status == PHY_IDLE)
                {
                    mac->trx_state_req = TX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       TX_ON);
                    Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                          interfaceIndex,
                                                          PHY_BUSY_TX);
                }
                else
                {
                    if (mac->txBcnCmd2)
                    {
                        MESSAGE_Free(node, mac->txBcnCmd2);
                        mac->txBcnCmd2 = NULL;
                    }
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                }
            }
        }
        else if (mac->txCsmaca == mac->txData)
        {

            ERROR_Assert( (mac->taskP.mcps_data_request_STEP
                      || mac->taskP.mcps_broadcast_request_STEP)
                      && (strcmp(mac->taskP.mcps_data_request_frFunc,frFunc)
                      == 0), "Invalid condition");


            if (mac->taskP.mcps_data_request_TxOptions & TxOp_GTS)
            {
                // GTS transmission
                // not handled here
            }
            // indirect transmission
            else if ((mac->taskP.mcps_data_request_TxOptions & TxOp_Indirect)
                      && (mac->capability.FFD &&
                      (Mac802_15_4NumberDeviceLink(&mac->deviceLink1) > 0)))
            {
                if (status != PHY_IDLE)
                {
                    Mac802_15_4mcps_data_request(
                                    node,
                                    interfaceIndex,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    mac->taskP.mcps_data_request_TxOptions,
                                    FALSE,
                                    PHY_BUSY_TX,
                                    M802_15_4_CHANNEL_ACCESS_FAILURE);
                }
                else
                {
                    strcpy(mac->taskP.mcps_data_request_frFunc,
                           "PD_DATA_confirm");
                    mac->trx_state_req = TX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       TX_ON);
                    
                    // assumed that above always returns true
                    Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                          interfaceIndex,
                                                          PHY_BUSY_TX);
                }
            }
            else // direct transmission: in this case, let
                // mcps_data_request() take care of everything
            {
                Mac802_15_4mcps_data_request(
                                    node,
                                    interfaceIndex,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    mac->taskP.mcps_data_request_TxOptions,
                                    FALSE,
                                    status, M802_15_4_SUCCESS);
            }
        }
        else if (mac->txCsmaca == mac->txBcnCmd) // def handling for txBcnCmd
        {
            wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd);
            frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
            Mac802_15_4FrameCtrlParse(&frmCtrl);
            if (status == PHY_IDLE)
            {
                if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                     && (wph->MSDU_CmdType == 0x02)) // association resp pkt
                {
                    strcpy(mac->taskP.mlme_associate_request_frFunc,
                           "PD_DATA_confirm");
                }
                else if ((frmCtrl.frmType ==
                          M802_15_4DEFFRMCTRL_TYPE_MACCMD)  // cmd pkt
                          && (wph->MSDU_CmdType == 0x08))
                {
                    // coordinator realignment response packet
                    strcpy(mac->taskP.mlme_orphan_response_frFunc,
                           "PD_DATA_confirm");
                }
                mac->trx_state_req = TX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);

                // assumed that above always returns true
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX);
            }
            else
            {
                if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                     && (wph->MSDU_CmdType == 0x02)) // ass resp pkt
                {
                    Mac802_15_4mlme_associate_response(
                           node,
                           interfaceIndex,
                           mac->taskP.mlme_associate_response_DeviceAddress,
                           0,
                           M802_15_4_CHANNEL_ACCESS_FAILURE,
                           0,
                           FALSE,
                           PHY_BUSY_TX);
                }
                else if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                          && (wph->MSDU_CmdType == 0x08))
                            // coordinator realignment response packet
                {
                    Mac802_15_4mlme_orphan_response(
                            node,
                            interfaceIndex,
                            mac->taskP.mlme_orphan_response_OrphanAddress,
                            0,
                            TRUE,
                            FALSE,
                            FALSE,
                            PHY_BUSY_TX);
                }
                else
                {
                    if (mac->txBcnCmd)
                    {
                        MESSAGE_Free(node, mac->txBcnCmd);
                        mac->txBcnCmd = NULL;
                    }
                    mac->waitBcnCmdAck = FALSE;
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                }
            }
        }
    }
    else if (strcmp(frFunc, "PD_DATA_confirm") == 0)
    {
        if (mac->txPkt == mac->txBeacon)
        {
            if (mac->taskP.mlme_start_request_STEP &&
                (strcmp(mac->taskP.mlme_start_request_frFunc,frFunc) == 0))
            {
                Mac802_15_4mlme_start_request(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_start_request_PANId,
                        mac->taskP.mlme_start_request_LogicalChannel,
                        mac->taskP.mlme_start_request_BeaconOrder,
                        mac->taskP.mlme_start_request_SuperframeOrder,
                        mac->taskP.mlme_start_request_PANCoordinator,
                        mac->taskP.mlme_start_request_BatteryLifeExtension,
                        0,
                        mac->taskP.mlme_start_request_SecurityEnable,
                        FALSE,
                        status);
            }
            else // default handling
            {

                if (! mac->broadCastQueue->isEmpty())
                 {
                    if (MESSAGE_ReturnPacketSize(mac->txBeacon) <=
                        aMaxSIFSFrameSize)
                    {
                        ifs = aMinSIFSPeriod;
                    }
                    else
                    {
                        ifs = aMinLIFSPeriod;
                    }
                 }

                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'b', TRUE);

                if (! mac->broadCastQueue->isEmpty())
                {
                    // If condition is added to stop processing a broadcast
                    // packet if CSMA is currently processing another
                    // request and is in backoff mode.

                    if (mac->backoffStatus != BACKOFF)
                    {
                        // the variable 'isCalledAfterTransmittingBeacon'
                        // makes sures broadcast data is dequeued only after
                        // a beacon is sent (i.e IFS handler is called after
                        // sending a beacon)

                        mac->isCalledAfterTransmittingBeacon = TRUE;
                        mac->IFST =
                  Mac802_15_4SetTimer(
                  node,
                  mac,
                  M802_15_4IFSTIMER,
                  ifs * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                  NULL);
                    }
                    else
                    {
                       // Dont do anything, let CSMA first handle the
                       // pending packet whose reference is in csmaca->txPkt
                       // Code for deferring the transmission of broadcast
                       // data can be added here if required.
                    }
                }

            }
        }
        else if (mac->txPkt == mac->txAck)
        {
            if (mac->rxCmd)
            {
                if (MESSAGE_ReturnPacketSize(mac->rxCmd)
                    <= aMaxSIFSFrameSize)
                {
                    ifs = aMinSIFSPeriod;
                }
                else
                {
                    ifs = aMinLIFSPeriod;
                }

            mac->IFST =
            Mac802_15_4SetTimer(
                node,
                mac,
                M802_15_4IFSTIMER,
                ifs * SECOND
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                NULL);

                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'a', TRUE);
            }
            else if (mac->rxData)
            {
                if (MESSAGE_ReturnPacketSize(mac->rxData)
                    <= aMaxSIFSFrameSize)
                {
                    ifs = aMinSIFSPeriod;
                }
                else
                {
                    ifs = aMinLIFSPeriod;
                }

             mac->IFST =
             Mac802_15_4SetTimer(
                  node,
                  mac,
                  M802_15_4IFSTIMER,
                  ifs * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                  NULL);

                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'a', TRUE);
            }
            else    // ack. for duplicated packet
            {
                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'a', TRUE);
            }
        }
        else if (mac->txPkt == mac->txBcnCmd)
        {
            wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd);
            frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
            Mac802_15_4FrameCtrlParse(&frmCtrl);
            if (frmCtrl.ackReq)
            {
                // ack required
                mac->trx_state_req = RX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   RX_ON);
                mac->waitBcnCmdAck = TRUE;
                mac->txT = Mac802_15_4SetTimer(
                                    node,
                                    mac,
                                    M802_15_4TXTIMER,
                                    mac->mpib.macAckWaitDuration * SECOND
                                        / Phy802_15_4GetSymbolRate(
                                            node->phyData[interfaceIndex]),
                                    NULL);
            }
            else
            {
                // assume success if ack not required
                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'c', TRUE);
            }
        }
        else if (mac->txPkt == mac->txBcnCmd2)
        {
            if (mac->taskP.mlme_scan_request_STEP &&
                ((mac->taskP.mlme_scan_request_ScanType == 0x01) ||
                (mac->taskP.mlme_scan_request_ScanType == 0x03)) &&
                (strcmp(mac->taskP.mlme_scan_request_frFunc, frFunc) == 0))
            {
                Mac802_15_4mlme_scan_request(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_scan_request_ScanType,
                        mac->taskP.mlme_scan_request_ScanChannels,
                        mac->taskP.mlme_scan_request_ScanDuration,
                        FALSE,
                        status);
            }
            else if (mac->taskP.mlme_start_request_STEP &&
                     (strcmp(mac->taskP.mlme_start_request_frFunc, frFunc)
                                == 0))
            {
                MESSAGE_Free(node, mac->txBeacon);
                mac->txBeacon = NULL;
                Mac802_15_4mlme_start_request(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_start_request_PANId,
                        mac->taskP.mlme_start_request_LogicalChannel,
                        mac->taskP.mlme_start_request_BeaconOrder,
                        mac->taskP.mlme_start_request_SuperframeOrder,
                        mac->taskP.mlme_start_request_PANCoordinator,
                        mac->taskP.mlme_start_request_BatteryLifeExtension,
                        0,
                        mac->taskP.mlme_start_request_SecurityEnable,
                        FALSE,
                        status);
            }
            else if (mac->taskP.mlme_associate_request_STEP &&
                   (strcmp(mac->taskP.mlme_associate_request_frFunc,
                        frFunc) == 0))
            {
                Mac802_15_4mlme_associate_request(
                        node,
                        interfaceIndex,
                        0, 0, 0, 0, 0,
                        mac->taskP.mlme_associate_request_SecurityEnable,
                        FALSE,
                        status,
                        M802_15_4_SUCCESS);
            }
            else if (mac->taskP.mlme_poll_request_STEP &&
                    (strcmp(mac->taskP.mlme_poll_request_frFunc,frFunc)
                            == 0))
            {
                Mac802_15_4mlme_poll_request(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_poll_request_CoordAddrMode,
                        mac->taskP.mlme_poll_request_CoordPANId,
                        mac->taskP.mlme_poll_request_CoordAddress,
                        mac->taskP.mlme_poll_request_SecurityEnable,
                        mac->taskP.mlme_poll_request_autoRequest,
                        FALSE,
                        status);
            }
            else if (mac->taskP.mlme_gts_request_STEP &&
                    (strcmp(mac->taskP.mlme_gts_request_frFunc,frFunc)
                            == 0))
            {
                Mac802_15_4MLME_GTS_request(node,
                                            interfaceIndex,
                                            0,
                                            FALSE,
                                            status);
            }
            else    // default handling
            {
                wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd2);
                frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
                Mac802_15_4FrameCtrlParse(&frmCtrl);
                if (frmCtrl.ackReq) // ack. required
                {
                    mac->trx_state_req = RX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(
                            node,
                            interfaceIndex,
                            RX_ON);

            mac->txT =
            Mac802_15_4SetTimer(
                node,
                mac,
                M802_15_4TXTIMER,
                mac->mpib.macAckWaitDuration * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                NULL);

                    mac->waitBcnCmdAck2 = TRUE;
                }
                else
                {
                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', TRUE);
                }
            }
        }
        else if (mac->txPkt == mac->txData)
        {
          if (!(mac->taskP.mcps_data_request_STEP)
             || !(strcmp(mac->taskP.mcps_data_request_frFunc, frFunc) == 0))
            {
                if (!(mac->taskP.mcps_broadcast_request_STEP))
                {
                    return;
                }
            }
            wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txData);
            frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
            Mac802_15_4FrameCtrlParse(&frmCtrl);

            if (mac->taskP.mcps_data_request_STEP
                    || mac->taskP.mcps_broadcast_request_STEP)
            {
                
                if ((mac->taskP.mcps_data_request_TxOptions
                            & TxOp_Indirect)  // indirect transmission
                          && (mac->capability.FFD &&
                            (Mac802_15_4NumberDeviceLink(&mac->deviceLink1)
                                > 0))) // Coordinator
                {
                    if (!frmCtrl.ackReq) // ack. not required
                    {
                       Mac802_15_4mcps_data_request(
                                node,
                                interfaceIndex,
                                0, 0, 0, 0, 0, 0, 0, 0, 0,
                                mac->taskP.mcps_data_request_TxOptions,
                                FALSE,
                                PHY_SUCCESS,
                                M802_15_4_SUCCESS);
                    }
                    else
                    {
                        strcpy(mac->taskP.mcps_data_request_frFunc,
                                    "recvAck");
                        mac->trx_state_req = RX_ON;
                        Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                           interfaceIndex,
                                                           RX_ON);

                         mac->txT =
                          Mac802_15_4SetTimer(
                                node,
                                mac,
                                M802_15_4TXTIMER,
                                mac->mpib.macAckWaitDuration * SECOND
                                  / Phy802_15_4GetSymbolRate(
                                        node->phyData[interfaceIndex]),
                                NULL);

                        mac->waitDataAck = TRUE;
                    }
                }
                else  // direct transmission
                {
                    Mac802_15_4mcps_data_request(
                                    node,
                                    interfaceIndex,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    mac->taskP.mcps_data_request_TxOptions,
                                    FALSE,
                                    status,
                                    M802_15_4_SUCCESS);
                }
            }
            else  // default handling (seems impossible)
            {
                if (frmCtrl.ackReq) // ack. required
                {
                    mac->trx_state_req = RX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       RX_ON);

                     mac->txT =
                     Mac802_15_4SetTimer(
                                node,
                                mac,
                                M802_15_4TXTIMER,
                                mac->mpib.macAckWaitDuration * SECOND
                                      / Phy802_15_4GetSymbolRate(
                                            node->phyData[interfaceIndex]),
                                NULL);

                    mac->waitDataAck = TRUE;
                }
                else
                {
                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'd', TRUE);
                }
            }
        }
       else if (mac->txPkt == mac->txGts)
       {
            if (mac->taskP.mcps_gts_data_request_STEP
                && (strcmp(mac->taskP.mlme_gts_data_request_frFunc,frFunc)
                    == 0))
            {
               Mac802_15_4mcps_data_request(
                                node,
                                interfaceIndex,
                                0, 0, 0, 0, 0, 0, 0, 0, 0,
                                mac->taskP.mcps_data_request_TxOptions,
                                FALSE,
                                status,
                                M802_15_4_SUCCESS);
            }
            else
            {
                ERROR_Assert(FALSE, "Invalid operation");
            }
        }
    }
    else if (strcmp(frFunc, "recvAck") == 0)
    {
        if (mac->txPkt == mac->txData)
        {
            if ((mac->taskP.mcps_data_request_STEP
                    || mac->taskP.mcps_broadcast_request_STEP)
                    && (strcmp(mac->taskP.mcps_data_request_frFunc, frFunc)
                        == 0))
            {
                Mac802_15_4mcps_data_request(
                                    node,
                                    interfaceIndex,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    mac->taskP.mcps_data_request_TxOptions,
                                    FALSE,
                                    PHY_SUCCESS,
                                    M802_15_4_SUCCESS);
            }
            else    // default handling for <txData>
            {
                if (mac->taskP.mcps_data_request_STEP
                        || mac->taskP.mcps_broadcast_request_STEP)
                {
                    if (mac->isBroadCastPacket)
                    {
                        mac->taskP.mcps_broadcast_request_STEP
                            = DIRECT_INIT_STEP;
                    }
                    else
                    {
                        mac->taskP.mcps_data_request_STEP = DIRECT_INIT_STEP;
                    }
                }

                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'd', TRUE);
            }
        }
        else if (mac->txPkt == mac->txBcnCmd2)
        {
            if (mac->taskP.mlme_associate_request_STEP
                    && (strcmp(mac->taskP.mlme_associate_request_frFunc,
                            frFunc)
                 == 0))
            {
                Mac802_15_4mlme_associate_request(
                        node,
                        interfaceIndex,
                        0, 0, 0, 0, 0,
                        mac->taskP.mlme_associate_request_SecurityEnable,
                        FALSE,
                        PHY_SUCCESS,
                        M802_15_4_SUCCESS);
            }
            else if (mac->taskP.mlme_poll_request_STEP
                    && (strcmp(mac->taskP.mlme_poll_request_frFunc,frFunc)
                  == 0))
            {
                Mac802_15_4mlme_poll_request(
                                node,
                                interfaceIndex,
                                mac->taskP.mlme_poll_request_CoordAddrMode,
                                mac->taskP.mlme_poll_request_CoordPANId,
                                mac->taskP.mlme_poll_request_CoordAddress,
                                mac->taskP.mlme_poll_request_SecurityEnable,
                                mac->taskP.mlme_poll_request_autoRequest,
                                FALSE,
                                PHY_SUCCESS);
            }
            else if (mac->taskP.mlme_gts_request_STEP
                    && (strcmp(mac->taskP.mlme_gts_request_frFunc,frFunc)
                 == 0))
            {
                Mac802_15_4MLME_GTS_request(node,
                                            interfaceIndex,
                                            0,
                                            FALSE,
                                            PHY_SUCCESS);
            }
            else
            {
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', TRUE);
            }
        }
        else if (mac->txPkt == mac->txBcnCmd)
        {
            wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd);
            frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
            Mac802_15_4FrameCtrlParse(&frmCtrl);
            if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                 && (wph->MSDU_CmdType == 0x02))  // association resp pkt
            {
                Mac802_15_4mlme_associate_response(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_associate_response_DeviceAddress,
                        0,
                        M802_15_4_SUCCESS,
                        0,
                        FALSE,
                        PHY_SUCCESS);
            }
            else if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                      && (wph->MSDU_CmdType == 0x08))// coord realig resp pkt
            {
                Mac802_15_4mlme_orphan_response(
                            node,
                            interfaceIndex,
                            mac->taskP.mlme_orphan_response_OrphanAddress,
                            0,
                            TRUE,
                            FALSE,
                            FALSE,
                            PHY_SUCCESS);
            }
            else
            {
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'c', TRUE);
            }
        }
        else if (mac->txPkt == mac->txGts)
        {
            if (mac->taskP.mcps_gts_data_request_STEP
                   && (strcmp(mac->taskP.mlme_gts_data_request_frFunc, frFunc)
                        == 0))
            {
                Mac802_15_4mcps_data_request(
                                node,
                                interfaceIndex,
                                0, 0, 0, 0, 0, 0, 0, 0, 0,
                                mac->taskP.mcps_data_request_TxOptions,
                                FALSE,
                                PHY_SUCCESS,
                                M802_15_4_SUCCESS);
            }
            else
            {
                ERROR_Assert(FALSE, "Invalid operation");
            }
        }
    }
    else if (strcmp(frFunc,"txHandler") == 0)
    {
        if (mac->txPkt == mac->txData)
        {
            if (mac->taskP.mcps_data_request_STEP
                    || mac->taskP.mcps_broadcast_request_STEP)
            {
                ERROR_Assert(!mac->taskP.mcps_broadcast_request_STEP,
                             "Broadcast request stem is non-zero");
                if ((mac->taskP.mcps_data_request_TxOptions 
                            & TxOp_Indirect) // indirect transmission
                         && (mac->capability.FFD
                            && (Mac802_15_4NumberDeviceLink(&mac->deviceLink1)
                      > 0)))  // coordinator
                {
                    Mac802_15_4mcps_data_request(
                                    node,
                                    interfaceIndex,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    mac->taskP.mcps_data_request_TxOptions,
                                    FALSE,
                                    PHY_BUSY_TX,
                                    M802_15_4_NO_ACK);
                }
                else        // direct transmission
                {
                    Mac802_15_4mcps_data_request(
                                    node,
                                    interfaceIndex,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    mac->taskP.mcps_data_request_TxOptions,
                                    FALSE,
                                    PHY_BUSY_TX,
                                    M802_15_4_SUCCESS);
                }
            }
            else if ((!mac->taskP.mcps_data_request_STEP
                    || !mac->taskP.mcps_broadcast_request_STEP)
                    || (strcmp(mac->taskP.mcps_data_request_frFunc,
                            "recvAck") != 0))
            {
                // we might have been waiting for poll request
                if (mac->taskP.mlme_poll_request_STEP &&
                    (strcmp(mac->taskP.mlme_poll_request_frFunc,
                     "recvAck") == 0))
                {
                    Mac802_15_4mlme_poll_request(
                            node,
                            interfaceIndex,
                            mac->taskP.mlme_poll_request_CoordAddrMode,
                            mac->taskP.mlme_poll_request_CoordPANId,
                            mac->taskP.mlme_poll_request_CoordAddress,
                            mac->taskP.mlme_poll_request_SecurityEnable,
                            mac->taskP.mlme_poll_request_autoRequest,
                            FALSE,
                            PHY_BUSY_TX); // status can anything but SUCCESS
                }
                return;
            }
            
        }
        else if (mac->txPkt == mac->txBcnCmd2)
        {
            if (mac->taskP.mlme_associate_request_STEP &&
                (strcmp(mac->taskP.mlme_associate_request_frFunc,
                    "recvAck") == 0))
            {
                Mac802_15_4mlme_associate_request(
                        node,
                        interfaceIndex,
                        0, 0, 0, 0, 0,
                        mac->taskP.mlme_associate_request_SecurityEnable,
                        FALSE,
                        PHY_BUSY_TX,
                        M802_15_4_SUCCESS);
            }
            else if (mac->taskP.mlme_poll_request_STEP &&
                    (strcmp(mac->taskP.mlme_poll_request_frFunc,
                        "recvAck") == 0))
            {
                Mac802_15_4mlme_poll_request(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_poll_request_CoordAddrMode,
                        mac->taskP.mlme_poll_request_CoordPANId,
                        mac->taskP.mlme_poll_request_CoordAddress,
                        mac->taskP.mlme_poll_request_SecurityEnable,
                        mac->taskP.mlme_poll_request_autoRequest,
                        FALSE,
                        PHY_BUSY_TX); // status can anything but PHY_SUCCESS
            }
            else if (mac->taskP.mlme_gts_request_STEP &&
                    (strcmp(mac->taskP.mlme_gts_request_frFunc,
                        "recvAck") == 0))
            {
                Mac802_15_4MLME_GTS_request(node,
                                            interfaceIndex,
                                            0,
                                            FALSE,
                                            PHY_BUSY_TX);
            }
            else
            {
                mac->numBcnCmdRetry2++;
                if (mac->numBcnCmdRetry2 <= aMaxFrameRetries)
                {
                    mac->waitBcnCmdAck2 = FALSE;
                }
                else
                {
                    MESSAGE_Free(node, mac->txBcnCmd2);
                    mac->txBcnCmd2 = NULL;
                }
                Mac802_15_4CsmacaResume(node, interfaceIndex);
            }
        }
        else if (mac->txPkt == mac->txBcnCmd)
        {
            wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd);
            frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
            Mac802_15_4FrameCtrlParse(&frmCtrl);
            if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                && (wph->MSDU_CmdType == 0x02)) 
                        // association response packet
            {
                Mac802_15_4mlme_associate_response(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_associate_response_DeviceAddress,
                        0,
                        M802_15_4_NO_ACK,
                        0,
                        FALSE,
                        PHY_BUSY_TX);
            }
            else if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                        && (wph->MSDU_CmdType == 0x08))
                            // coord realignment resp
            {
                mac->numBcnCmdRetry++;
                if (mac->numBcnCmdRetry <= aMaxFrameRetries)
                {
                    strcpy(mac->taskP.mlme_orphan_response_frFunc,
                        "csmacaCallBack");
                    mac->waitBcnCmdAck = FALSE;
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                }
                else
                {
                    Mac802_15_4mlme_orphan_response(
                            node,
                            interfaceIndex,
                            mac->taskP.mlme_orphan_response_OrphanAddress,
                            0,
                            TRUE,
                            FALSE,
                            FALSE,
                            PHY_BUSY_TX);
                }
            }
            else
            {
                MESSAGE_Free(node, mac->txBcnCmd);
                mac->txBcnCmd = NULL;
                mac->waitBcnCmdAck = FALSE;
                Mac802_15_4CsmacaResume(node, interfaceIndex);
            }
        }
        else if (mac->txPkt == mac->txGts)
        {
            ERROR_Assert(mac->taskP.mcps_gts_data_request_STEP, "GTS request"
                         "step should be non-zero");
            Mac802_15_4mcps_data_request(
                                    node,
                                    interfaceIndex,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    mac->taskP.mcps_data_request_TxOptions,
                                    FALSE,
                                    PHY_BUSY_TX,
                                    M802_15_4_NO_ACK);
        }
    }
    else if (strcmp(frFunc, "PLME_SET_TRX_STATE_confirm") == 0)
    {
        if (mac->trx_state_req == FORCE_TRX_OFF)
        {
            if (mac->taskP.mlme_reset_request_STEP
                && (strcmp(mac->taskP.mlme_reset_request_frFunc, frFunc)
                    == 0))
            {
                Mac802_15_4mlme_reset_request(
                            node,
                            interfaceIndex,
                            mac->taskP.mlme_reset_request_SetDefaultPIB,
                            FALSE,
                            status);
            }
        }
        if (mac->trx_state_req == RX_ON)
        {
            if (mac->taskP.mlme_scan_request_STEP
                && (strcmp(mac->taskP.mlme_scan_request_frFunc, frFunc)
                    == 0))
            {
                Mac802_15_4mlme_scan_request(
                            node,
                            interfaceIndex,
                            mac->taskP.mlme_scan_request_ScanType,
                            mac->taskP.mlme_scan_request_ScanChannels,
                            mac->taskP.mlme_scan_request_ScanDuration,
                            FALSE,
                            status);
            }
        }
        else if (mac->taskP.mlme_rx_enable_request_STEP
            && (strcmp(mac->taskP.mlme_rx_enable_request_frFunc, frFunc)
                    == 0))
        {
            Mac802_15_4mlme_rx_enable_request(
                            node,
                            interfaceIndex,
                            0,
                            mac->taskP.mlme_rx_enable_request_RxOnTime,
                            mac->taskP.mlme_rx_enable_request_RxOnDuration,
                            FALSE,
                            PHY_SUCCESS);
        }
    }
    else if (strcmp(frFunc, "PLME_SET_confirm") == 0)
    {
        if (mac->taskP.mlme_scan_request_STEP
            && (strcmp(mac->taskP.mlme_scan_request_frFunc, frFunc) == 0))
        {
            Mac802_15_4mlme_scan_request(
                                node,
                                interfaceIndex,
                                mac->taskP.mlme_scan_request_ScanType,
                                mac->taskP.mlme_scan_request_ScanChannels,
                                mac->taskP.mlme_scan_request_ScanDuration,
                                FALSE,
                                status);
        }
    }
    else if (strcmp(frFunc,"PLME_ED_confirm") == 0)
    {
        if (mac->taskP.mlme_scan_request_STEP
            && (mac->taskP.mlme_scan_request_ScanType == 0x00) // ED scan
            && (strcmp(mac->taskP.mlme_scan_request_frFunc, frFunc) == 0))
        {
            Mac802_15_4mlme_scan_request(
                                node,
                                interfaceIndex,
                                mac->taskP.mlme_scan_request_ScanType,
                                mac->taskP.mlme_scan_request_ScanChannels,
                                mac->taskP.mlme_scan_request_ScanDuration,
                                FALSE,
                                status);
        }
    }
    else if (strcmp(frFunc,"recvBeacon") == 0)
    {
        if (mac->taskP.mlme_scan_request_STEP
            && (strcmp(mac->taskP.mlme_scan_request_frFunc, frFunc) == 0))
        {
            Mac802_15_4mlme_scan_request(
                                node,
                                interfaceIndex,
                                mac->taskP.mlme_scan_request_ScanType,
                                mac->taskP.mlme_scan_request_ScanChannels,
                                mac->taskP.mlme_scan_request_ScanDuration,
                                FALSE,
                                PHY_SUCCESS);
        }
        else if (mac->taskP.mlme_rx_enable_request_STEP
            && (strcmp(mac->taskP.mlme_rx_enable_request_frFunc, frFunc)
                == 0))
        {
            Mac802_15_4mlme_rx_enable_request(
                            node,
                            interfaceIndex,
                            0,
                            mac->taskP.mlme_rx_enable_request_RxOnTime,
                            mac->taskP.mlme_rx_enable_request_RxOnDuration,
                            FALSE,
                            PHY_SUCCESS);
        }
        else if (mac->taskP.mlme_sync_request_STEP
            && (strcmp(mac->taskP.mlme_sync_request_frFunc, frFunc) == 0))
        {
            Mac802_15_4mlme_sync_request(
                                    node,
                                    interfaceIndex,
                                    0,
                                    mac->taskP.mlme_sync_request_tracking,
                                    FALSE,
                                    PHY_SUCCESS);
        }
    }
    else if (strcmp(frFunc, "scanHandler") == 0)
    {
        if (mac->taskP.mlme_scan_request_STEP)
        {
            Mac802_15_4mlme_scan_request(
                            node,
                            interfaceIndex,
                            mac->taskP.mlme_scan_request_ScanType,
                            mac->taskP.mlme_scan_request_ScanChannels,
                            mac->taskP.mlme_scan_request_ScanDuration,
                            FALSE,
                            PHY_BUSY_TX);
        }
    }
    else if (strcmp(frFunc, "extractHandler") == 0)
    {
        if (mac->taskP.mlme_associate_request_STEP
            && (strcmp(mac->taskP.mlme_associate_request_frFunc, frFunc)
                == 0))
        {
            Mac802_15_4mlme_associate_request(
                        node,
                        interfaceIndex,
                        0,
                        0,
                        0,
                        0,
                        0,
                        mac->taskP.mlme_associate_request_SecurityEnable,
                        FALSE,
                        PHY_BUSY_TX,
                        M802_15_4_SUCCESS);
        }
        else if (mac->taskP.mlme_poll_request_STEP
            && (strcmp(mac->taskP.mlme_poll_request_frFunc, "IFSHandler")
                 == 0))
        {
            Mac802_15_4mlme_poll_request(
                                node,
                                interfaceIndex,
                                mac->taskP.mlme_poll_request_CoordAddrMode,
                                mac->taskP.mlme_poll_request_CoordPANId,
                                mac->taskP.mlme_poll_request_CoordAddress,
                                mac->taskP.mlme_poll_request_SecurityEnable,
                                mac->taskP.mlme_poll_request_autoRequest,
                                FALSE,
                                PHY_BUSY_TX);
        }
    }
    else if (strcmp(frFunc, "assoRspWaitHandler") == 0)
    {
        if (mac->taskP.mlme_associate_response_STEP)
        {
            mac->taskP.mlme_associate_response_STEP = 2;
            Mac802_15_4mlme_associate_response(
                        node,
                        interfaceIndex,
                        mac->taskP.mlme_associate_response_DeviceAddress,
                        0,
                        M802_15_4_SUCCESS,
                        0,
                        FALSE,
                        PHY_BUSY_TX); // status ignored
        }
    }
    else if (strcmp(frFunc, "orphanRspHandler") == 0)
    {
        if (mac->taskP.mlme_orphan_response_STEP)
        {
            Mac802_15_4mlme_orphan_response(
                            node,
                            interfaceIndex,
                            mac->taskP.mlme_orphan_response_OrphanAddress,
                            0,
                            TRUE,
                            FALSE,
                            FALSE,
                            PHY_BUSY_TX);
        }
    }
    else if (strcmp(frFunc, "dataWaitHandler") == 0)
    {
        // Broadcast message handling not done here.
        if (mac->taskP.mcps_data_request_STEP)
        {
            mac->taskP.mcps_data_request_STEP = INDIRECT_INIT_STEP;

            // check if the transaction still pending
            Int32 i = Mac802_15_4UpdateTransacLinkByPktOrHandle(
                                        node,
                                        OPER_PURGE_TRANSAC,
                                        &mac->transacLink1,
                                        &mac->transacLink2,
                                        mac->taskP.mcps_data_request_pendPkt,
                                        0);
            if (i == 0) // still pending
            {
                Mac802_15_4Reset_TRX(node, interfaceIndex);
                M802_15_4DEVLINK* device = NULL;
                wph = (M802_15_4Header*)
                  MESSAGE_ReturnPacket(mac->taskP.mcps_data_request_pendPkt);
                MACADDR dstAddress = wph->MHR_DstAddrInfo.addr_64;

                /* OPER_GET_DEVICE_REFERENCE is sent in the call to function
                Mac802_15_4UpdateDeviceLink. 3 means check the
                devicelink table for the device entry and send back the
                reference of the device found. in variable "device".
                The variable "numTimesTrnsExpired" represents the
                count of number of times a transaction has expired
                for a device. If this count exceeds
                M802_15_4_MAXTIMESTANSACTIONEXPIRED, the device's
                entry is removed from the devicelink table */

                if (!Mac802_15_4UpdateDeviceLink(OPER_GET_DEVICE_REFERENCE,
                                                &mac->deviceLink1,
                                                &mac->deviceLink2,
                                                dstAddress,
                                                &device))
                {
                    if (device->numTimesTrnsExpired
                            < M802_15_4_MAXTIMESTANSACTIONEXPIRED)
                    {
                        device->numTimesTrnsExpired++;
                    }
                    else
                    {
                        // delete the device from the devicelink table
                        Mac802_15_4UpdateDeviceLink(
                                                OPER_DELETE_DEVICE_REFERENCE,
                                                &mac->deviceLink1,
                                                &mac->deviceLink2,
                                                dstAddress,
                                                NULL);
                    }
                }

                Csma802_15_4Cancel(node, interfaceIndex);
                Mac802_15_4TaskFailed(node,
                                      interfaceIndex,
                                      'd',
                                      M802_15_4_TRANSACTION_EXPIRED,
                                      FALSE);
            }
            else
            {
                mac->stats.numDataPktSent++;
                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4CheckForOutgoingPacket(node, interfaceIndex);
            }
        }
    }
    else if (strcmp(frFunc, "IFSHandler") == 0)
    {
        if (mac->taskP.mlme_associate_request_STEP)
        {
            if (strcmp(mac->taskP.mlme_associate_request_frFunc, frFunc)
                    == 0)
            {
                Mac802_15_4mlme_associate_request(
                        node,
                        interfaceIndex,
                        0,
                        0,
                        0,
                        0,
                        0,
                        mac->taskP.mlme_associate_request_SecurityEnable,
                        FALSE,
                        PHY_SUCCESS,
                        mStatus);
            }
            else if (mac->taskP.mlme_associate_request_STEP == 7)
            {
                // we have missed the ack but received data...let's proceed
                mac->taskP.mlme_associate_request_STEP++;
                if (mac->txT)
                {
                    MESSAGE_CancelSelfMsg(node, mac->txT);
                    mac->txT = NULL;
                }
                if (mac->backoffStatus == BACKOFF)
                {
                    mac->backoffStatus = BACKOFF_RESET;
                    Csma802_15_4Cancel(node, interfaceIndex);
                }
 
                // if the command was about to be transmitted, free it
                if (mac->txBcnCmd2)
                {
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', FALSE);
                }
                if (mac->txCmdDataT)
                {
                    MESSAGE_CancelSelfMsg(node, mac->txCmdDataT);
                    mac->txCmdDataT = NULL;
                }
                Mac802_15_4mlme_associate_request(
                        node,
                        interfaceIndex,
                        0,
                        0,
                        0,
                        0,
                        0,
                        mac->taskP.mlme_associate_request_SecurityEnable,
                        FALSE,
                        PHY_SUCCESS,
                        mStatus);
            }
        }
        else if (mac->taskP.mlme_poll_request_STEP
                && (strcmp(mac->taskP.mlme_poll_request_frFunc,frFunc) == 0))
        {
            Mac802_15_4mlme_poll_request(
                                node,
                                interfaceIndex,
                                mac->taskP.mlme_poll_request_CoordAddrMode,
                                mac->taskP.mlme_poll_request_CoordPANId,
                                mac->taskP.mlme_poll_request_CoordAddress,
                                mac->taskP.mlme_poll_request_SecurityEnable,
                                mac->taskP.mlme_poll_request_autoRequest,
                                FALSE,
                                PHY_SUCCESS);
        }
        else if (mac->taskP.mlme_scan_request_STEP
            && (strcmp(mac->taskP.mlme_scan_request_frFunc, frFunc) == 0))
        {
            Mac802_15_4mlme_scan_request(
                                node,
                                interfaceIndex,
                                mac->taskP.mlme_scan_request_ScanType,
                                mac->taskP.mlme_scan_request_ScanChannels,
                                mac->taskP.mlme_scan_request_ScanDuration,
                                FALSE,
                                PHY_SUCCESS);
        }
    }
    else if (strcmp(frFunc, "rxEnableHandler") == 0)
    {
        if (strcmp(mac->taskP.mlme_rx_enable_request_frFunc, frFunc) == 0)
        {
            Mac802_15_4mlme_rx_enable_request(
                    node,
                    interfaceIndex,
                    0,
                    mac->taskP.mlme_rx_enable_request_RxOnTime,
                    mac->taskP.mlme_rx_enable_request_RxOnDuration,
                    FALSE,
                    PHY_SUCCESS);
        }
    }
    else if (strcmp(frFunc, "beaconSearchHandler") == 0)
    {
        if (mac->taskP.mlme_sync_request_STEP
            && (strcmp(mac->taskP.mlme_sync_request_frFunc, "recvBeacon")
                == 0))
        {
            Mac802_15_4mlme_sync_request(
                    node,
                    interfaceIndex,
                    0,
                    mac->taskP.mlme_sync_request_tracking,
                    FALSE,
                    PHY_BUSY_TX); // status can anything but SUCCESS
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4CsmacaResume
// LAYER      :: Mac
// PURPOSE    :: Resume CSMA-CA
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4CsmacaResume(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    M802_15_4FrameCtrl frmCtrl;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if ((mac->backoffStatus != BACKOFF)           // not during backoff
         &&  (!mac->inTransmission))         // not during transmission
    {
        if ((mac->txBcnCmd) && (!mac->waitBcnCmdAck))
        {
            mac->backoffStatus = BACKOFF;
            frmCtrl.frmCtrl = ((M802_15_4Header*)
                           MESSAGE_ReturnPacket(mac->txBcnCmd))->MHR_FrmCtrl;
            Mac802_15_4FrameCtrlParse(&frmCtrl);
            mac->txCsmaca = mac->txBcnCmd;
            Csma802_15_4Start(node,
                              interfaceIndex,
                              TRUE,
                              mac->txBcnCmd,
                              frmCtrl.ackReq);
        }
        else if ((mac->txBcnCmd2) && (!mac->waitBcnCmdAck2))
        {
            mac->backoffStatus = BACKOFF;
            frmCtrl.frmCtrl = ((M802_15_4Header*)
                        MESSAGE_ReturnPacket(mac->txBcnCmd2))->MHR_FrmCtrl;
            Mac802_15_4FrameCtrlParse(&frmCtrl);
            mac->txCsmaca = mac->txBcnCmd2;
            Csma802_15_4Start(node,
                             interfaceIndex,
                             TRUE,
                             mac->txBcnCmd2,
                             frmCtrl.ackReq);

             CsmaData802_15_4* csmaca = (CsmaData802_15_4*)mac->csma;

             if (!csmaca->backoffT)
             {
                 // Added to stop the poll request from waiting till the
                 // next beacon

                 if (mac->taskP.mlme_poll_request_STEP)
                 {
                     csmaca->waitNextBeacon = FALSE;
                     mac->backoffStatus = BACKOFF_RESET;
                     csmaca->txPkt = NULL;
                     Mac802_15_4Dispatch(node,
                            interfaceIndex,
                            PHY_SENSING, // Anything apart from phy_idle
                            mac->taskP.mlme_poll_request_frFunc,
                            RX_ON, // Anything
                            M802_15_4_CHANNEL_ACCESS_FAILURE);
                 }
                 else if (mac->taskP.mlme_associate_request_STEP)
                 {
                     csmaca->waitNextBeacon = FALSE;
                     mac->backoffStatus = BACKOFF_RESET;
                     csmaca->txPkt = NULL;
                     Mac802_15_4Dispatch(node,
                            interfaceIndex,
                            PHY_SENSING, // Anything apart from phy_idle
                            mac->taskP.mlme_associate_request_frFunc,
                            RX_ON, // Anything
                            M802_15_4_CHANNEL_ACCESS_FAILURE);
                 }
             }
        }
        else if ((mac->txData) && (!mac->waitDataAck))
        {
            strcpy(mac->taskP.mcps_data_request_frFunc, "csmacaCallBack");

            if (mac->isBroadCastPacket)
            {
                mac->taskP.mcps_broadcast_request_STEP
                    = DIRECT_CSMA_STATUS_STEP;
            }
            else
            {
                mac->taskP.mcps_data_request_STEP = DIRECT_CSMA_STATUS_STEP;
            }

            mac->backoffStatus = BACKOFF;
            frmCtrl.frmCtrl = ((M802_15_4Header*)
                MESSAGE_ReturnPacket(mac->txData))->MHR_FrmCtrl;
            Mac802_15_4FrameCtrlParse(&frmCtrl);
            mac->txCsmaca = mac->txData;
            Csma802_15_4Start(node,
                              interfaceIndex,
                              TRUE,
                              mac->txData,
                              frmCtrl.ackReq);
        }
    }
}



// /**
// FUNCTION   :: Mac802_15_4CsmacaCallBack
// LAYER      :: Mac
// PURPOSE    :: Callback received from CSMA-CA
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + status         : PhyStatusType : PHY status
// RETURN  :: None
// **/
void Mac802_15_4CsmacaCallBack(Node* node,
                              Int32 interfaceIndex,
                              PhyStatusType status)
{
    MacData802_15_4* mac = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    if (((!mac->txBcnCmd) || (mac->waitBcnCmdAck))
           && ((!mac->txBcnCmd2) || (mac->waitBcnCmdAck2))
           && ((!mac->txData) || (mac->waitDataAck)))
    {
        return;
    }
    if (status == PHY_IDLE)
    {
        mac->backoffStatus = BACKOFF_SUCCESSFUL;
    }
    else
    {
        mac->backoffStatus = BACKOFF_FAILED;
    }
    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        status,
                        "csmacaCallBack",
                        RX_ON,
                        M802_15_4_SUCCESS);
}


BOOL Mac802_15_4ToParent(Node* node, Int32 interfaceIndex, Message* p)
{
    MacData802_15_4* mac = NULL;
    M802_15_4Header* wph = NULL;
    M802_15_4FrameCtrl frmCtrl;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    wph = (M802_15_4Header*) MESSAGE_ReturnPacket(p);

    frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
    Mac802_15_4FrameCtrlParse(&frmCtrl);
    if (((frmCtrl.dstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE16)
        && (wph->MHR_DstAddrInfo.addr_16 == mac->mpib.macCoordShortAddress))
        || ((frmCtrl.dstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE64)
        && (wph->MHR_DstAddrInfo.addr_64
               == mac->mpib.macCoordExtendedAddress)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: Mac802_15_4LocateBoundary
// LAYER      :: Mac
// PURPOSE    :: Locate slot boundry for beacon enabled PAN after given time
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + parent         : BOOL      : If from parent
// + wtime          : clocktype : Time after which slot boundry is to be
//                                  located
// RETURN  :: clocktype
// **/
clocktype Mac802_15_4LocateBoundary(Node* node,
                                    Int32 interfaceIndex,
                                    BOOL parent,
                                    clocktype wtime)
{

    Int32 align = 0;
    clocktype bcnTxRxTime = 0;
    clocktype bPeriod = 0;
    clocktype newtime = 0;
    clocktype tmpf = 0;
    MacData802_15_4* mac = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    if ((mac->mpib.macBeaconOrder == 15) && (mac->macBeaconOrder2 == 15))
    {
        return wtime;
    }

    if (parent)
    {
        align = (mac->macBeaconOrder2 == 15)?1:2;
    }
    else
    {
        align = (mac->mpib.macBeaconOrder == 15)?2:1;
    }

    bcnTxRxTime = (align == 1) ? (mac->macBcnTxTime) : (mac->macBcnRxTime);
    bPeriod = aUnitBackoffPeriod * SECOND
                   / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
    tmpf = node->getNodeTime() + wtime;
    tmpf -= bcnTxRxTime;
    newtime = tmpf % bPeriod;

    if (newtime > 1000)
    {
        tmpf = bPeriod - newtime;
        newtime = wtime + tmpf;
    }
    else
    {
        newtime = wtime;
    }
    return newtime;
}

// /**
// FUNCTION   :: Mac802_15_4CanProceedWOcsmaca
// LAYER      :: Mac
// PURPOSE    :: Find out if MAC layer can proceed with CSMA for a given pkt
//              in current CAP
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + msg              : Message*  : Message to be sent
// RETURN  :: BOOL
// **/
static
BOOL Mac802_15_4CanProceedWOcsmaca(Node* node,
                                   Int32 interfaceIndex,
                                   Message* msg)
{
    MacData802_15_4* mac = NULL;
    M802_15_4FrameCtrl frmCtrl;
    clocktype wtime = 0;
    clocktype t_IFS = 0;
    clocktype t_transacTime = 0;
    clocktype t_CAP = 0;
    clocktype tmpf = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if ((mac->mpib.macBeaconOrder == 15)
        && (mac->macBeaconOrder2 == 15)
        && (mac->macBeaconOrder3 == 15))
    {
        return TRUE;
    }
    else
    {
        frmCtrl.frmCtrl = ((M802_15_4Header*)
                                MESSAGE_ReturnPacket(msg))->MHR_FrmCtrl;
        Mac802_15_4FrameCtrlParse(&frmCtrl);
        wtime = 0;
        if (MESSAGE_ReturnPacketSize(msg) <= aMaxSIFSFrameSize)
        {

            t_IFS = aMinSIFSPeriod * SECOND
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

        }
        else
        {

            t_IFS = aMinLIFSPeriod * SECOND
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
        }

        // boundary location time
        t_transacTime = Mac802_15_4LocateBoundary(
                            node,
                            interfaceIndex,
                            Mac802_15_4ToParent(node, interfaceIndex, msg),
                            wtime) - wtime;

        // packet transmission time
        t_transacTime += Mac802_15_4TrxTime(node, interfaceIndex, msg);
        
        M802_15_4Header* wph = (M802_15_4Header*) MESSAGE_ReturnPacket(msg);
        if (frmCtrl.ackReq)
        {
              t_transacTime += PHY_GetTransmissionDuration(
                                    node,
                                    interfaceIndex,
                                    M802_15_4_DEFAULT_DATARATE_INDEX,
                                    numPsduOctetsInAckFrame);
              t_transacTime += max_pDelay;
        }
        if (wph->MHR_DstAddrInfo.addr_16 != M802_15_4_COORDSHORTADDRESS)
        {
            t_transacTime += max_pDelay;
            t_transacTime += t_IFS;
        }
        else
        {
            ERROR_Assert(FALSE, "Invalid operation");
        }
        BOOL toParent = Mac802_15_4ToParent(node,
                                            interfaceIndex,
                                            msg);
        ERROR_Assert(!toParent, "Indirect transmission towards parent is"
                      " taking place");
        t_CAP = Mac802_15_4GetCAP(node, interfaceIndex, toParent);
        tmpf = node->getNodeTime() + wtime;
        tmpf += t_transacTime;
        if (tmpf > t_CAP)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4CalculateBcnRxTimer
// LAYER      :: Mac
// PURPOSE    :: Calculates next Becon time
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: clocktype
// **/
static
clocktype Mac802_15_4CalculateBcnRxTimer(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    clocktype BI = 0;
    clocktype bcnRxTime = 0;
    clocktype now = 0;
    clocktype len12s = 0;
    clocktype wtime = 0;
    clocktype tmpf = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
     BI = ((aBaseSuperframeDuration * (1 << mac->macBeaconOrder2)) * SECOND)
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

    bcnRxTime = mac->macBcnRxTime;
    now = node->getNodeTime();
    while (now - bcnRxTime > BI)
    {
        bcnRxTime += BI;
    }

    len12s = 12 * SECOND
            / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

    tmpf = (now - bcnRxTime);
    wtime = BI - tmpf;

    if (wtime >= len12s)
    {
        wtime -= len12s;
    }

    tmpf = now + wtime;
   if (tmpf - mac->macBcnRxLastTime < BI - len12s)
    {
        tmpf = 2 * BI;
        tmpf = tmpf - now;
        tmpf = tmpf + bcnRxTime;
        wtime = tmpf - len12s;
    }

    mac->macBcnRxLastTime = now + wtime ;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Rx Timer"
                     " scheduled at %" TYPES_64BITFMT "d from "
                     " CalculateBcnRxTimer \n",
                    node->getNodeTime(),
                    node->nodeId,
                    now + wtime);
    }
    return wtime;
}



static
clocktype Mac802_15_4CalculateBcnRxTimer2(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    clocktype BI = 0;
    clocktype bcnRxTime = 0;
    clocktype now = 0;
    clocktype len12s = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    BI = ((aBaseSuperframeDuration * (1 << mac->macBeaconOrder2)) * SECOND)
              / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

    bcnRxTime = mac->macBcnRxTime;
    now = node->getNodeTime();
    len12s = 12 * SECOND
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
   return bcnRxTime + BI - len12s - now;
}

static
void Mac802_15_4TransmitCmdData(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    clocktype delay = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    if ((mac->mpib.macBeaconOrder != 15)
        || (mac->macBeaconOrder2 != 15))
    {
        delay = Mac802_15_4LocateBoundary(
                    node,
                    interfaceIndex,
                    Mac802_15_4ToParent(node, interfaceIndex, mac->txCsmaca),
                    0);
        if (delay)
        {
            mac->txCmdDataT = Mac802_15_4SetTimer(node,
                                                  mac,
                                                  M802_15_4TXCMDDATATIMER,
                                                  delay,
                                                  NULL);
            return;
        }
    }
    Mac802_15_4TxBcnCmdDataHandler(node, interfaceIndex);
}
// --------------------------------------------------------------------------
// Timer handler functions
// --------------------------------------------------------------------------

// /**
// FUNCTION   :: Mac802_15_4TxHandler
// LAYER      :: Mac
// PURPOSE    :: Handles Extract timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4TxOverHandler(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : TxOver Timer"
            " expired\n", node->getNodeTime(), node->nodeId);
    }

    ERROR_Assert(mac->txPkt, "mac->txPkt should point to the last sent packet");
    Mac802_15_4PD_DATA_confirm(node, interfaceIndex, (PhyStatusType)0);
}


// /**
// FUNCTION   :: Mac802_15_4TxHandler
// LAYER      :: Mac
// PURPOSE    :: Handles Extract timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4TxHandler(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Tx Timer"
                " expired\n", node->getNodeTime(), node->nodeId);
    }

    ERROR_Assert(mac->txBcnCmd || mac->txBcnCmd2|| mac->txData || mac->txGts,
                 "Invalid condition");
    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        PHY_BUSY_TX,
                        "txHandler",
                        RX_ON,
                        M802_15_4_SUCCESS);
}

// /**
// FUNCTION   :: Mac802_15_4ExtractHandler
// LAYER      :: Mac
// PURPOSE    :: Handles Extract timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4ExtractHandler(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Extract Timer"
            " expired\n", node->getNodeTime(), node->nodeId);
    }

    if (mac->taskP.mlme_associate_request_STEP)
    {
        strcpy(mac->taskP.mlme_associate_request_frFunc, "extractHandler");
    }
    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        PHY_BUSY_TX,
                        "extractHandler",
                        RX_ON,
                        M802_15_4_SUCCESS);
}

// /**
// FUNCTION   :: Mac802_15_4AssoRspWaitHandler
// LAYER      :: Mac
// PURPOSE    :: Handles Association response wait timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4AssoRspWaitHandler(Node* node, Int32 interfaceIndex)
{
    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Association"
            " response Timer expired\n", node->getNodeTime(), node->nodeId);
    }
    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        PHY_BUSY_TX,
                        "assoRspWaitHandler",
                        RX_ON,
                        M802_15_4_SUCCESS);
}

// /**
// FUNCTION   :: Mac802_15_4DataWaitHandler
// LAYER      :: Mac
// PURPOSE    :: Handles data wait timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4DataWaitHandler(Node* node, Int32 interfaceIndex)
{
    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Data wait"
               " Timer expired\n", node->getNodeTime(), node->nodeId);
    }
    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        PHY_BUSY_TX,
                        "dataWaitHandler",
                        RX_ON,
                        M802_15_4_SUCCESS);
}

// /**
// FUNCTION   :: Mac802_15_4RxEnableHandler
// LAYER      :: Mac
// PURPOSE    :: Handles receive enable timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4RxEnableHandler(Node* node, Int32 interfaceIndex)
{
    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : RxEnable"
               " Timer expired\n", node->getNodeTime(), node->nodeId);
    }
    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        PHY_SUCCESS,
                        "rxEnableHandler",
                        RX_ON,
                        M802_15_4_SUCCESS);
}

// /**
// FUNCTION   :: Mac802_15_4ScanHandler
// LAYER      :: Mac
// PURPOSE    :: Handles Scan timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4ScanHandler(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Scan Timer"
               " expired\n", node->getNodeTime(), node->nodeId);
    }

    if (mac->taskP.mlme_scan_request_ScanType == 0x01)
    {
        mac->taskP.mlme_scan_request_STEP++;
    }
    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        PHY_SUCCESS,
                        "scanHandler",
                        RX_ON,
                        M802_15_4_SUCCESS);
}


// /**
// NAME         ::Mac802_15_4DeallocateGts
// PURPOSE      ::Removes a GTS descriptor saved at PanCoordinator
// PARAMETERS   ::
// + node : Node*             : Node which sends the message
// + mac : MacData_802_15_4*  : 802.15.4 data structure
// + i : Int32                  : The position of GTS descriptot in the array
// RETURN       ::None
// NOTES        ::None
// **/
static
void Mac802_15_4DeallocateGts(Node* node, MacData802_15_4* mac, Int32 i)
{
    BOOL reshuffleDone = FALSE;
    while (!mac->gtsSpec.queue[i]->isEmpty())
    {
        Message* msg = NULL;
        mac->gtsSpec.queue[i]->retrieve(&msg,
                                        DEQUEUE_NEXT_PACKET,
                                        DEQUEUE_PACKET,
                                        0,
                                        NULL);
        mac->stats.numDataPktsDeQueuedForGts++;
        mac->stats.numPktDropped++;
        MESSAGE_Free(node, msg);
        msg = NULL;
    }
    if (i == mac->gtsSpec.count - 1)
    {
        // last entry to be deallocated
        // no reshuffle of startslots required.

        BOOL gtsDescriptorFound = FALSE;
        Int32 k = 0;
        for (k = 0; k < mac->gtsSpec3.count; k++)
        {
            if ((((mac->gtsSpec).fields).list[i]).devAddr16 
                    == (((mac->gtsSpec3).fields).list[k]).devAddr16)
            {
                if (mac->gtsSpec.recvOnly[i] == mac->gtsSpec3.recvOnly[k])
                {
                    // We already have a descriptor in the gtsSpec3
                    // set the start slot to zero & reset gtspersistence timer

                    ERROR_Assert(!gtsDescriptorFound," Valid GTS descriptor"
                                 "should not be present");
                    Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec3,
                                                   k,
                                                   0);
                    mac->gtsSpec3.slotStart[k] = 0;
                    mac->gtsSpec3.aGtsDescPersistanceCount[k]
                    = aGTSDescPersistenceTime;
                    gtsDescriptorFound = TRUE;
                }
            }
        }
        if (!gtsDescriptorFound && mac->gtsSpec3.count < aMaxNumGts)
        {
            // Add this descriptor for the beacon
            (((mac->gtsSpec3).fields).list[mac->gtsSpec3.count]).devAddr16 
                = (((mac->gtsSpec).fields).list[i]).devAddr16;
            Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec3,
                                           mac->gtsSpec3.count,
                                           0);
            Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec3,
                                          mac->gtsSpec3.count,
                                          mac->gtsSpec.recvOnly[i]);
            Mac802_15_4GTSSpecSetLength(&mac->gtsSpec3,
                                        mac->gtsSpec3.count,
                                        mac->gtsSpec.length[i]);
            mac->gtsSpec3.aGtsDescPersistanceCount[mac->gtsSpec3.count]
                = aGTSDescPersistenceTime;
            Mac802_15_4GTSSpecSetCount(&mac->gtsSpec3,
                                       mac->gtsSpec3.count + 1);
       }
    }
    else
    {
        reshuffleDone = TRUE;
        UInt8 gtslength = mac->gtsSpec.length[i];
        Int32 j = 0;
        for (j = i; j < mac->gtsSpec.count - 1; j++)
        {
            BOOL gtsDescriptorFound = FALSE;
            Int32 k = 0;
            for (k = 0; k < mac->gtsSpec3.count; k++)
            {
                if ((((mac->gtsSpec).fields).list[j]).devAddr16
                        == (((mac->gtsSpec3).fields).list[k]).devAddr16)
                {
                    if (mac->gtsSpec.recvOnly[j]
                            == mac->gtsSpec3.recvOnly[k])
                    {
                        // We already have a descriptor in the gtsSpec3
                        // set the start slot & reset gtspersistence time

                        UInt8 slotStart = 0;
                        if (i == j)
                        {
                            slotStart = 0;
                        }
                        else
                        {
                            slotStart
                                = mac->gtsSpec.slotStart[j] + gtslength;
                        }
                        Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec3,
                                                       k,
                                                       slotStart);
                        mac->gtsSpec3.aGtsDescPersistanceCount[k]
                            = aGTSDescPersistenceTime;
                        gtsDescriptorFound = TRUE;
                        break;
                    }
                }
            }
            if (!gtsDescriptorFound && mac->gtsSpec3.count < aMaxNumGts)
            {
                // Add this descriptor for the beacon
                (((mac->gtsSpec3).fields).list[mac->gtsSpec3.count]).devAddr16
                    = (((mac->gtsSpec).fields).list[j]).devAddr16;
                UInt8 slotStart = 0;
                if (i != j)
                {
                    slotStart = mac->gtsSpec.slotStart[j] + gtslength;
                }
                else
                {
                    // dont do anything
                }
                Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec3,
                                               mac->gtsSpec3.count,
                                               slotStart);
                Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec3,
                                              mac->gtsSpec3.count,
                                              mac->gtsSpec.recvOnly[j]);
                Mac802_15_4GTSSpecSetLength(&mac->gtsSpec3,
                                             mac->gtsSpec3.count,
                                             mac->gtsSpec.length[j]);
                mac->gtsSpec3.aGtsDescPersistanceCount[mac->gtsSpec3.count]
                    = aGTSDescPersistenceTime;
                Mac802_15_4GTSSpecSetCount(&mac->gtsSpec3,
                                           mac->gtsSpec3.count + 1);
           }
           (((mac->gtsSpec).fields).list[j]).devAddr16
                = (((mac->gtsSpec).fields).list[j+1]).devAddr16;
           mac->gtsSpec.slotStart[j]
                = mac->gtsSpec.slotStart[j+1] + gtslength;
           mac->gtsSpec.length[j] = mac->gtsSpec.length[j+1];
           mac->gtsSpec.recvOnly[j] = mac->gtsSpec.recvOnly[j+1];
           mac->gtsSpec.expiraryCount[j] = mac->gtsSpec.expiraryCount[j+1];
           mac->gtsSpec.appPort[j] = mac->gtsSpec.appPort[j+1];
           while (!mac->gtsSpec.queue[j+1]->isEmpty())
           {
                Message* msg = NULL;
                BOOL isFull = FALSE;
                mac->gtsSpec.queue[j+1]->retrieve(&msg,
                                                  DEQUEUE_NEXT_PACKET,
                                                  DEQUEUE_PACKET,
                                                  0,
                                                  NULL);
                mac->gtsSpec.queue[j]->insert(msg,
                                              NULL,
                                              &isFull,
                                              0,
                                              0,
                                              0);
                if (!isFull)
                {
                    // mac->stats.numDataPktsQueuedForGts++;
                }
                else
                {
                    ERROR_Assert(FALSE, "MAC queue is full");
                }
            }
        }

        // Add the last descriptor
        BOOL gtsDescriptorFound = FALSE;
        Int32 k = 0;
        for (k = 0; k < mac->gtsSpec3.count; k++)
        {
            if ((((mac->gtsSpec).fields).list[
                                    mac->gtsSpec.count - 2]).devAddr16
                    == (((mac->gtsSpec3).fields).list[k]).devAddr16)
            {
                if (mac->gtsSpec.recvOnly[mac->gtsSpec.count - 2] 
                                        == mac->gtsSpec3.recvOnly[k])
                {
                    // We already have a descriptor in the gtsSpec3
                    // set the start slot & reset gtspersistence timr
                    ERROR_Assert(!gtsDescriptorFound, "Valid GTS descriptor"
                                 " should not be present");
                    Mac802_15_4GTSSpecSetSlotStart(
                              &mac->gtsSpec3,
                              k,
                              mac->gtsSpec.slotStart[mac->gtsSpec.count - 2]);
                    mac->gtsSpec3.aGtsDescPersistanceCount[k]
                        = aGTSDescPersistenceTime;
                    gtsDescriptorFound = TRUE;
                    break;
                }
            }
        }
        if (!gtsDescriptorFound && mac->gtsSpec3.count < aMaxNumGts)
        {
             // Add this descriptor for the beacon
             (((mac->gtsSpec3).fields).list[mac->gtsSpec3.count]).devAddr16 
                  = (((mac->gtsSpec).fields).list[
                            mac->gtsSpec.count - 2]).devAddr16;
             Mac802_15_4GTSSpecSetSlotStart(
                            &mac->gtsSpec3,
                            mac->gtsSpec3.count,
                            mac->gtsSpec.slotStart[mac->gtsSpec.count - 2]);
             Mac802_15_4GTSSpecSetRecvOnly(
                            &mac->gtsSpec3,
                            mac->gtsSpec3.count,
                            mac->gtsSpec.recvOnly[mac->gtsSpec.count - 2]);
             Mac802_15_4GTSSpecSetLength(
                            &mac->gtsSpec3,
                            mac->gtsSpec3.count,
                            mac->gtsSpec.length[mac->gtsSpec.count - 2]);
             mac->gtsSpec3.aGtsDescPersistanceCount[mac->gtsSpec3.count]
                = aGTSDescPersistenceTime;
             Mac802_15_4GTSSpecSetCount(&mac->gtsSpec3,
                                        mac->gtsSpec3.count + 1);
        }
    }
    
    // Reset the freed gtsdescriptor chracteristics
    mac->gtsSpec.slotStart[mac->gtsSpec.count - 1] = 0;
    mac->gtsSpec.length[mac->gtsSpec.count - 1] = 0;
    mac->gtsSpec.recvOnly[mac->gtsSpec.count - 1] = 0;
    mac->gtsSpec.fields.list[mac->gtsSpec.count - 1].devAddr16 = 0;
    mac->gtsSpec.expiraryCount[mac->gtsSpec.count - 1] = 0;
    mac->gtsSpec.appPort[mac->gtsSpec.count - 1] = 0;
    mac->gtsSpec.count--;
    Mac802_15_4GTSSpecSetCount(&mac->gtsSpec,
                               mac->gtsSpec.count);

    // Update the GTS descriptor
    for (i = 0; i < mac->gtsSpec.count; i++)
    {
        Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec,
                                       i,
                                       mac->gtsSpec.slotStart[i]);
        Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec,
                                      i,
                                      mac->gtsSpec.recvOnly[i]);
        Mac802_15_4GTSSpecSetLength(&mac->gtsSpec,
                                    i,
                                    mac->gtsSpec.length[i]);
    }

    // update final cap
    if (mac->gtsSpec.count > 0)
    {
        mac->currentFinCap
            = mac->gtsSpec.slotStart[mac->gtsSpec.count - 1] - 1;
    }
    else
    {
        mac->currentFinCap = 15;
    }
}

// /**
// NAME         ::Mac802_15_4UpdateGtsDescriptorForBeacon
// PURPOSE      ::Updated the GTS descriptor in a beacon
// PARAMETERS   ::
// + node : Node*             : Node which sends the message
// + mac : MacData_802_15_4*  : 802.15.4 data structure
// RETURN       ::None
// NOTES        ::None
// **/
static
void Mac802_15_4UpdateGtsDescriptorForBeacon(Node* node,
                                             MacData802_15_4* mac)
{
    Int32 i = 0;
    while (i < mac->gtsSpec3.count)
    {
        mac->gtsSpec3.aGtsDescPersistanceCount[i] --;
        if (mac->gtsSpec3.aGtsDescPersistanceCount[i] == 0)
        {
            // remove this descriptor.
            if (i == mac->gtsSpec3.count - 1)
            {
                // no reshuffle required
            }
            else
            {
                // reset the gts descriptor
                Int32 j = 0;
                for (j = i; j < mac->gtsSpec3.count - 1; j++)
                {
                    mac->gtsSpec3.fields.list[j].devAddr16 
                            = mac->gtsSpec3.fields.list[j + 1].devAddr16;
                    Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec3,
                                               j,
                                               mac->gtsSpec3.slotStart[j+1]);
                    Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec3,
                                                 j,
                                                 mac->gtsSpec3.recvOnly[j+1]);
                    Mac802_15_4GTSSpecSetLength(&mac->gtsSpec3,
                                                j,
                                                mac->gtsSpec3.length[j+1]);
                    mac->gtsSpec3.aGtsDescPersistanceCount[j] 
                        = mac->gtsSpec3.aGtsDescPersistanceCount[j+1];
                }
            }
            // reset the descriptor
            mac->gtsSpec3.fields.list[mac->gtsSpec3.count - 1].devAddr16 = 0;
            Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec3,
                                           mac->gtsSpec3.count - 1,
                                           0);
            Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec3,
                                          mac->gtsSpec3.count - 1,
                                          0);
            Mac802_15_4GTSSpecSetLength(&mac->gtsSpec3,
                                        mac->gtsSpec3.count - 1,
                                        0);
             mac->gtsSpec3.aGtsDescPersistanceCount[mac->gtsSpec3.count - 1] 
                = 0;
             Mac802_15_4GTSSpecSetCount(&mac->gtsSpec3,
                                        mac->gtsSpec3.count - 1);
            i = 0;
            continue;
        }
        i++;
    }

    // check gts expiration
    i = 0;
    while (i < mac->gtsSpec.count)
    {
        if (!mac->gtsSpec.recvOnly[i] || mac->mpib.macDataAcks)
        {
            mac->gtsSpec.expiraryCount[i]--;
            if (mac->gtsSpec.expiraryCount[i] == 0)
            {
                mac->stats.numGtsExpired++;
                Mac802_15_4DeallocateGts(node, mac, i);
                i = 0;
                continue;
            }
        }
        i++;
    }
}


// /**
// NAME         ::Mac802_15_4ScheduleGtsTimerAtPanCoord
// PURPOSE      ::Schedules GTS timer at PanCoordinator
// PARAMETERS   ::
// + node : Node*             : Node which sends the message
// + interfaceIndex : Int32     : interface index
// + mac : MacData_802_15_4*  : 802.15.4 data structure
// RETURN       ::None
// NOTES        ::None
// **/
static
void Mac802_15_4ScheduleGtsTimerAtPanCoord(Node* node,
                                           Int32 interfaceIndex,
                                           MacData802_15_4* mac)
{
    if (mac->mpib.macGTSPermit)
    {
        ERROR_Assert(mac->isPANCoor, "Should be a pan coordinator");
        if (mac->gtsSpec.count > 0)
        {
            // A valid gts is available
            Int32 rate = Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
            clocktype sSlotDuration = mac->sfSpec.sd * SECOND / rate;
            clocktype delay = 
            mac->gtsSpec.slotStart[mac->gtsSpec.count - 1] * sSlotDuration;
            ERROR_Assert(!mac->gtsT, "mac->gtsT should be NULL");

            // allocate the timer message and send out
            Message* timerMsg;
            timerMsg = MESSAGE_Alloc(node,
                                     MAC_LAYER,
                                     MAC_PROTOCOL_802_15_4,
                                     MSG_MAC_TimerExpired);

            MESSAGE_SetInstanceId(timerMsg,
                                 (short)mac->myMacData->interfaceIndex);

            MESSAGE_InfoAlloc(node, timerMsg, sizeof(M802_15_4Timer));
            M802_15_4Timer* timerInfo
                = (M802_15_4Timer*) MESSAGE_ReturnInfo(timerMsg);

            timerInfo->timerType = M802_15_4GTSTIMER;
            MESSAGE_AddInfo(node,
                            timerMsg,
                            sizeof(UInt8),
                            INFO_TYPE_Gts_Slot_Start);
            char* gtsSlotInfo
                = MESSAGE_ReturnInfo(timerMsg, INFO_TYPE_Gts_Slot_Start);
            *gtsSlotInfo = mac->gtsSpec.slotStart[mac->gtsSpec.count - 1];
            mac->gtsT = timerMsg;
            MESSAGE_Send(node, timerMsg, delay);
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4BeaconTxHandler
// LAYER      :: Mac
// PURPOSE    :: Handles Beacon transmit timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4BeaconTxHandler(Node* node, Int32 interfaceIndex, BOOL forTX)
{
    MacData802_15_4* mac = NULL;
    M802_15_4FrameCtrl frmCtrl;
    M802_15_4Header* wph = NULL;
    M802_15_4TRANSLINK* tmp = NULL;
    Int32 i = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Beacon Tx"
               " Timer expired\n", node->getNodeTime(), node->nodeId);
    }

    if ((mac->mpib.macBeaconOrder != 15)     // beacon enabled
         || (mac->oneMoreBeacon))
    {
        if (forTX)
        {
            if (mac->capability.FFD)
            {
                mac->beaconWaiting = TRUE;
                mac->trx_state_req = FORCE_TRX_OFF;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   FORCE_TRX_OFF);
                if (mac->txAck)
                {
                    MESSAGE_Free(node, mac->txAck);
                    mac->txAck = NULL;
                }
                mac->trx_state_req = TX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);
                // assumed that above always returns true
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX);
            }
            else
            {
                ERROR_Assert(FALSE, "Should be an FFD");
            }
        }
        else    // send a beacon here
        {
            if (mac->capability.FFD)
            {
                if ((mac->taskP.mlme_start_request_STEP)
                    &&  (mac->mpib.macBeaconOrder != 15))
                {
                    if (mac->txAck || mac->backoffStatus == BACKOFF_SUCCESSFUL)
                    {
                        mac->beaconWaiting = FALSE;
                        if (mac->mpib.macBeaconOrder != 15)
                        {

           clocktype wtime =
                ((aBaseSuperframeDuration
                * (1 << mac->mpib.macBeaconOrder) - 12)
                * SECOND)
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

                            mac->bcnTxT =
                                    Mac802_15_4SetTimer(
                                        node,
                                        mac,
                                        M802_15_4BEACONTXTIMER,
                                        wtime,
                                        NULL);

                            mac->macBeaconOrder_last
                                = mac->mpib.macBeaconOrder;
                        }
                        else if (mac->macBeaconOrder_last != 15)
                        {

             clocktype wtime =
                ((aBaseSuperframeDuration
                * (1 << mac->macBeaconOrder_last) - 12)
                * SECOND)
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

                            mac->bcnTxT =
                                    Mac802_15_4SetTimer(
                                    node,
                                    mac,
                                    M802_15_4BEACONTXTIMER,
                                    wtime,
                                    NULL);
                        }
                        return;
                    }
                }
                if (mac->txBeacon != NULL)
                {
                    MESSAGE_Free(node, mac->txBeacon);
                    mac->txBeacon = NULL;
                }
                frmCtrl.frmCtrl = 0;
                Mac802_15_4FrameCtrlSetFrmType(
                            &frmCtrl,
                            M802_15_4DEFFRMCTRL_TYPE_BEACON);
                Mac802_15_4FrameCtrlSetSecu(&frmCtrl, mac->secuBeacon);
                Mac802_15_4FrameCtrlSetAckReq(&frmCtrl, FALSE);
                Mac802_15_4FrameCtrlSetDstAddrMode(
                            &frmCtrl,
                            M802_15_4DEFFRMCTRL_ADDRMODENONE);
                if (mac->mpib.macShortAddress == 0xfffe)
                {
                    Mac802_15_4FrameCtrlSetSrcAddrMode(
                            &frmCtrl,
                            M802_15_4DEFFRMCTRL_ADDRMODE64);
                }
                else
                {
                    Mac802_15_4FrameCtrlSetSrcAddrMode(
                            &frmCtrl,
                            M802_15_4DEFFRMCTRL_ADDRMODE16);
                }
                mac->sfSpec.superSpec = 0;
                Mac802_15_4SuperFrameSetBO(&mac->sfSpec,
                                           mac->mpib.macBeaconOrder);
                Mac802_15_4SuperFrameSetSO(&mac->sfSpec,
                                           mac->mpib.macSuperframeOrder);
                Mac802_15_4SuperFrameSetFinCAP(&mac->sfSpec,
                                               mac->currentFinCap);
                Mac802_15_4SuperFrameSetBLE(&mac->sfSpec,
                                            mac->mpib.macBattLifeExt);
                Mac802_15_4SuperFrameSetPANCoor(&mac->sfSpec,
                                                mac->isPANCoor);
                Mac802_15_4SuperFrameSetAssoPmt(
                                        &mac->sfSpec,
                                        mac->mpib.macAssociationPermit);
                Mac802_15_4GTSSpecSetPermit(&mac->gtsSpec3,
                                            mac->mpib.macGTSPermit);
                Mac802_15_4GTSSpecSetPermit(&mac->gtsSpec,
                                            mac->mpib.macGTSPermit);
                mac->pendAddrSpec.numShortAddr = 0;
                mac->pendAddrSpec.numExtendedAddr = 0;
                Mac802_15_4PurgeTransacLink(node,
                                            &mac->transacLink1,
                                            &mac->transacLink2);
                tmp = mac->transacLink1;
                i = 0;
                while (tmp != NULL)
                {
                    if (tmp->pendAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE16)
                    {
                        if (Mac802_15_4UpdateDeviceLink(
                                    OPER_CHECK_DEVICE_REFERENCE,
                                    &mac->deviceLink1,
                                    &mac->deviceLink2,
                                    tmp->pendAddr64) == 0)
                        {
                            i = Mac802_15_4PendAddSpecAddShortAddr(
                                    &mac->pendAddrSpec,
                                    tmp->pendAddr16);
                        }
                        // duplicated address filtered out
                    }
                    else
                    {
                        if (Mac802_15_4UpdateDeviceLink(
                                    OPER_CHECK_DEVICE_REFERENCE,
                                    &mac->deviceLink1,
                                    &mac->deviceLink2,
                                    tmp->pendAddr64) == 0)
                        {
                            i = Mac802_15_4PendAddSpecAddExtendedAddr(
                                    &mac->pendAddrSpec,
                                    tmp->pendAddr64);
                            // duplicated address filtered out
                        }
                    }
                    if (i >= 7)
                    {
                        break;
                    }
                    tmp = tmp->next;
                }

                // update gtsdescriptors
                Mac802_15_4UpdateGtsDescriptorForBeacon(node, mac);

                Mac802_15_4PendAddSpecFormat(&mac->pendAddrSpec);
                Mac802_15_4FrameCtrlSetFrmPending(&frmCtrl, i > 0);
                Mac802_15_4ConstructPayload(
                                        node,
                                        interfaceIndex,
                                        &mac->txBeacon,
                                        M802_15_4DEFFRMCTRL_TYPE_BEACON,
                                        mac->sfSpec.superSpec,
                                        &mac->gtsSpec3.fields,
                                        &mac->pendAddrSpec.fields,
                                        0);

                      // Add GTS precedence in info of beacon
                       MESSAGE_AddInfo(node,
                                     mac->txBeacon,
                                     sizeof(char),
                                     INFO_TYPE_Gts_Trigger_Precedence);
                       char* triggerPrecedence
                        = MESSAGE_ReturnInfo(
                                           mac->txBeacon,
                                           INFO_TYPE_Gts_Trigger_Precedence);
                  *triggerPrecedence = mac->mpib.macGtsTriggerPrecedence;

                if (! mac->broadCastQueue->isEmpty())
                {
                    Mac802_15_4AddCommandHeader(
                                            node,
                                            interfaceIndex,
                                            mac->txBeacon,
                                            M802_15_4DEFFRMCTRL_TYPE_BEACON,
                                            0,
                                            0,
                                            0xffff,
                                            frmCtrl.srcAddrMode,
                                            mac->mpib.macPANId,
                                            mac->aExtendedAddress,
                                            FALSE,
                                            TRUE,
                                            FALSE,
                                            0,
                                            0);
                }
                else
                {
                    Mac802_15_4AddCommandHeader(
                                            node,
                                            interfaceIndex,
                                            mac->txBeacon,
                                            M802_15_4DEFFRMCTRL_TYPE_BEACON,
                                            0,
                                            0,
                                            0xffff,
                                            frmCtrl.srcAddrMode,
                                            mac->mpib.macPANId,
                                            mac->aExtendedAddress,
                                            FALSE,
                                            FALSE,
                                            FALSE,
                                            0,
                                            0);
                }

                wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBeacon);
                if (mac->mpib.macShortAddress != 0xfffe)
                {
                   wph->MHR_SrcAddrInfo.addr_16 = mac->mpib.macShortAddress;
                }
                else
                {
                    wph->MHR_SrcAddrInfo.addr_64 = mac->aExtendedAddress;
                }
                wph->phyCurrentChannel
                    = mac->taskP.mlme_start_request_LogicalChannel;
                mac->beaconWaiting = TRUE;
                mac->txCsmaca = mac->txBeacon;
                mac->trx_state_req = TX_ON;

                // stop CSMA-CA if it is pending (it will be restored after
                // the transmission of Beacon)
                if (mac->backoffStatus == BACKOFF)
                {
                    mac->backoffStatus = BACKOFF_RESET;
                    Csma802_15_4Cancel(node, interfaceIndex);
                }
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   FORCE_TRX_OFF);

                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX);

                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                        " Sent Beacon\n", node->getNodeTime(), node->nodeId);
                }

                mac->stats.numBeaconsSent++;
                mac->mpib.macBeaconTxTime = node->getNodeTime();
                mac->macBcnTxTime = node->getNodeTime();
                mac->oneMoreBeacon = FALSE;
            }
            else
            {
                ERROR_Assert(FALSE, "Should be an FFD");
            }
        }
    }
    if (mac->mpib.macBeaconOrder != 15)
    {
        clocktype wtime
              = ((aBaseSuperframeDuration
                * (1 << mac->mpib.macBeaconOrder) /*- 12*/) * SECOND)
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

        mac->bcnTxT = Mac802_15_4SetTimer(node,
                                          mac,
                                          M802_15_4BEACONTXTIMER,
                                          wtime,
                                          NULL);
        if (DEBUG)
        {
            printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Next "
                    " beacon scheduled at "
                    "%" TYPES_64BITFMT "d\n",
                    node->getNodeTime(), node->nodeId,
                    wtime + node->getNodeTime());
        }
        mac->macBeaconOrder_last = mac->mpib.macBeaconOrder;

        // schedule timer fot 1st GTS slot
        Mac802_15_4ScheduleGtsTimerAtPanCoord(node, interfaceIndex, mac);
    }
    else if (mac->macBeaconOrder_last != 15)
    {
        clocktype wtime
            = ((aBaseSuperframeDuration *
                (1 << mac->macBeaconOrder_last) /*- 12*/) * SECOND)
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

        mac->bcnTxT = Mac802_15_4SetTimer(node,
                                          mac,
                                          M802_15_4BEACONTXTIMER,
                                          wtime,
                                          NULL);
    }
}


// /**
// FUNCTION   :: Mac802_15_4BeaconRxHandler
// LAYER      :: Mac
// PURPOSE    :: Handles beacon receive timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4BeaconRxHandler(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Beacon Rx"
                "Timer expired\n", node->getNodeTime(), node->nodeId);
    }

    if (mac->macBeaconOrder2 != 15)      // beacon enabled
    {
        if (mac->txAck)
        {
            MESSAGE_Free(node, mac->txAck);
            mac->txAck = NULL;
        }

        // enable the receiver
        mac->trx_state_req = RX_ON;
        Phy802_15_4PlmeSetTRX_StateRequest(node,
                                           interfaceIndex,
                                           RX_ON);
        if (mac->taskP.mlme_sync_request_tracking)
        {
            if (mac->numLostBeacons > aMaxLostBeacons)
            {
                Sscs802_15_4MLME_SYNC_LOSS_indication(node,
                                                      interfaceIndex,
                                                      M802_15_4_BEACON_LOSS);
                mac->numLostBeacons = 0;
            }
            else
            {
                mac->numLostBeacons++;

                if (DEBUG_TIMER)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                           "Set Rx timer"
                           " called from BeaconRxHandler\n",
                           node->getNodeTime(),
                           node->nodeId );
                }

                    mac->bcnRxT = Mac802_15_4SetTimer(
                                        node,
                                        mac,
                                        M802_15_4BEACONRXTIMER,
                                        Mac802_15_4CalculateBcnRxTimer(node,
                                            interfaceIndex),
                                        NULL);
            }
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4BeaconSearchHandler
// LAYER      :: Mac
// PURPOSE    :: Handles beacon search timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4BeaconSearchHandler(Node* node, Int32 interfaceIndex)
{
    if (DEBUG_TIMER)
    {
       printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Beacon Search"
             " Timer expired\n", node->getNodeTime(), node->nodeId);
    }

    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        PHY_BUSY_RX,
                        "beaconSearchHandler",
                        RX_ON,
                        M802_15_4_SUCCESS);
}

// /**
// FUNCTION   :: Mac802_15_4TxBcnCmdDataHandler
// LAYER      :: Mac
// PURPOSE    :: Handles tranmit BcnCmd Data event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4TxBcnCmdDataHandler(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    Int32 i = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : TxBcnCmd "
                " Timer expired\n", node->getNodeTime(), node->nodeId);
    }

    if (mac->taskP.mlme_scan_request_STEP)
    {
        if (mac->txBcnCmd2 != mac->txCsmaca)
        {
            return;  // terminate all other transmissions
        }
    }

    if (((M802_15_4Header*) MESSAGE_ReturnPacket(mac->txCsmaca))->indirect)
    {
        i = Mac802_15_4UpdateTransacLinkByPktOrHandle(node,
                                                      OPER_PURGE_TRANSAC,
                                                      &mac->transacLink1,
                                                      &mac->transacLink2,
                                                      mac->txCsmaca,
                                                      0);
        if (i != 0) // transaction expired
        {
            Mac802_15_4Reset_TRX(node, interfaceIndex);
            if (mac->txBcnCmd == mac->txCsmaca)
            {
                mac->txBcnCmd = NULL;
            }
            else if (mac->txBcnCmd2 == mac->txCsmaca)
            {
                mac->txBcnCmd2 = NULL;
            }
            else if (mac->txData == mac->txCsmaca)
            {
                mac->txData = NULL;
            }
            Mac802_15_4CsmacaResume(node, interfaceIndex);
            return;
        }
    }

    if (mac->inTransmission == TRUE)
    {
        return;
    }
    if (mac->txBeacon == mac->txCsmaca)
    {
        mac->txPkt = mac->txBeacon;
        mac->inTransmission = TRUE;
        Message* pktToPhy = MESSAGE_Duplicate(node, mac->txPkt);

        PHY_StartTransmittingSignal(node,
                                    interfaceIndex,
                                    pktToPhy,
                                    FALSE,
                                    0);
    }
    else if (mac->txBcnCmd == mac->txCsmaca)
    {
        mac->txPkt = mac->txBcnCmd;
        mac->inTransmission = TRUE;
        Message* pktToPhy = MESSAGE_Duplicate(node, mac->txPkt);

        PHY_StartTransmittingSignal(node,
                                    interfaceIndex,
                                    pktToPhy,
                                    FALSE,
                                    0);
    }
    else if (mac->txBcnCmd2 == mac->txCsmaca)
    {
        mac->txPkt = mac->txBcnCmd2;
        mac->inTransmission = TRUE;
        Message* pktToPhy = MESSAGE_Duplicate(node, mac->txPkt);

        PHY_StartTransmittingSignal(node,
                                    interfaceIndex,
                                    pktToPhy,
                                    FALSE,
                                    0);
    }
    else if (mac->txData == mac->txCsmaca)
    {
        mac->txPkt = mac->txData;
        mac->inTransmission = TRUE;
        Message* pktToPhy = MESSAGE_Duplicate(node,mac->txPkt);

        PHY_StartTransmittingSignal(node,
                                    interfaceIndex,
                                    pktToPhy,
                                    FALSE,
                                    0);
    }
}

// /**
// FUNCTION   :: Mac802_15_4IFSHandler
// LAYER      :: Mac
// PURPOSE    :: Handles inter frame space timer
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4IFSHandler(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    M802_15_4FrameCtrl frmCtrl;
    M802_15_4Header* wph = NULL;
    Message* pendPkt = NULL;
    M802_15_4_enum status;
    Int32 i = 0;
    char* payld = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : IFS Timer"
               " expired\n", node->getNodeTime(), node->nodeId);
    }
    if (mac->CheckPacketsForTransmission)
    {
        mac->CheckPacketsForTransmission = FALSE;
        Mac802_15_4CheckForOutgoingPacket(node, interfaceIndex);
        return;
    }
    if (mac->isCalledAfterTransmittingBeacon)
    {
        mac->isCalledAfterTransmittingBeacon = FALSE;
        if (!mac->broadCastQueue->isEmpty()
                && mac->backoffStatus != BACKOFF
                && !mac->inTransmission
                && !mac->taskP.mcps_broadcast_request_STEP
                && !mac->taskP.mcps_data_request_STEP)
        {
            mac->isBroadCastPacket = TRUE;
            Message* toBeBroadcastedMsg = NULL;
            mac->broadCastQueue->retrieve(&toBeBroadcastedMsg,
                                          DEQUEUE_NEXT_PACKET,
                                          DEQUEUE_PACKET,
                                          0,
                                          NULL);
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                        " Broadcast data Dequeued\n",
                         node->getNodeTime(),
                         node->nodeId);
            }
            Mac802_15_4mcps_data_request(
                            node,
                            (Int32)interfaceIndex,
                            M802_15_4DEFFRMCTRL_ADDRMODE16,
                            mac->mpib.macPANId,
                            (UInt16)node->nodeId,
                            M802_15_4DEFFRMCTRL_ADDRMODE16,
                            mac->mpib.macPANId,
                            0xffff,
                            MESSAGE_ReturnPacketSize(toBeBroadcastedMsg),
                            (toBeBroadcastedMsg),
                            0,
                            0,
                            TRUE,
                            PHY_SUCCESS,
                            M802_15_4_SUCCESS);
        }
        return;
    }

    if (mac->ifsTimerCalledAfterReceivingBeacon)
    {
        mac->ifsTimerCalledAfterReceivingBeacon = FALSE;
        if (mac->isPollRequestPending)
        {
            // send the pool request to extract data from the coordinator
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                        "Preparing to Poll from IFS handler\n",
                        node->getNodeTime(),
                        node->nodeId);
            }
            mac->isPollRequestPending = FALSE;
            Mac802_15_4mlme_poll_request(node,
                                         interfaceIndex,
                                         mac->panDes2.CoordAddrMode,
                                         mac->panDes2.CoordPANId,
                                         mac->panDes2.CoordAddress_64,
                                         mac->capability.secuCapable,
                                         TRUE,
                                         TRUE,
                                         PHY_SUCCESS);
        }
        return;
    }

    ERROR_Assert(mac->rxData || mac->rxCmd, "One of mac->rxData or"
                 " mac->rxCmd should be non null");

    if (mac->rxCmd)
    {
        wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->rxCmd);
        payld = (char*)((char *)wph + sizeof (M802_15_4Header)
            + sizeof (M802_15_4CommandFrame));
        frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
        Mac802_15_4FrameCtrlParse(&frmCtrl);

        if (wph->MSDU_CmdType == 0x01)      // Association request
        {
            Sscs802_15_4MLME_ASSOCIATE_indication(
                                            node,
                                            interfaceIndex,
                                            wph->MHR_SrcAddrInfo.addr_64,
                                            payld[0],
                                            frmCtrl.secu,
                                            0);
        }
        else if (wph->MSDU_CmdType == 0x02) // Association response
        {
            memcpy(&status, payld + 2, sizeof(M802_15_4_enum));
            // save the short address (if association successful)
            if (status == M802_15_4_SUCCESS)
            {
                mac->mpib.macShortAddress = *((UInt16 *)payld);
            }
            Mac802_15_4Dispatch(node,
                                interfaceIndex,
                                PHY_SUCCESS,
                                "IFSHandler",
                                RX_ON,
                                status);
        }
        else if (wph->MSDU_CmdType == 0x04) // Data request
        {
            if (mac->taskP.mcps_broadcast_request_STEP
                    || mac->inTransmission)
            {
                // broadcast request pending or phy in transmission state
                // do nothing, let MAC handle this request first
            }
            else
            {
                i = Mac802_15_4UpdateTransacLink(
                                            node,
                                            OPER_CHECK_TRANSAC,
                                            &mac->transacLink1,
                                            &mac->transacLink2,
                                            frmCtrl.srcAddrMode,
                                            wph->MHR_SrcAddrInfo.addr_64);
                if (i != 0)
                {
                    i =
                        Mac802_15_4UpdateTransacLink(
                                            node,
                                            OPER_PURGE_TRANSAC,
                                            &mac->transacLink1,
                                            &mac->transacLink2,
                                            frmCtrl.srcAddrMode,
                                            wph->MHR_SrcAddrInfo.addr_64);
                    if (i == 0)
                    {
                        i = 1;
                    }
                    else
                    {
                        i = 0;
                    }
                }
                else    // more than one packet pending
                {
                    i = 2;
                }
                if (i > 0)  // packet(s) pending
                {
                    pendPkt = Mac802_15_4GetPktFrTransacLink(
                                            &mac->transacLink1,
                                            frmCtrl.srcAddrMode,
                                            wph->MHR_SrcAddrInfo.addr_64);
                    wph = (M802_15_4Header*) MESSAGE_ReturnPacket(pendPkt);
                    wph->indirect = TRUE;
                    frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
                    Mac802_15_4FrameCtrlParse(&frmCtrl);
                    Mac802_15_4FrameCtrlSetFrmPending(&frmCtrl, i>1);
                    wph->MHR_FrmCtrl = frmCtrl.frmCtrl;
                    if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                    {
                        if (mac->txBcnCmd == pendPkt) // it's being processed
                        {
                            MESSAGE_Free(node, mac->rxCmd);
                            mac->rxCmd = NULL;
                            return;
                        }
                        ERROR_Assert(!mac->txBcnCmd, "mac->txBcnCmd should"
                                     " be NULL");
                        mac->txBcnCmd = pendPkt;
                        mac->waitBcnCmdAck = FALSE;
                        mac->numBcnCmdRetry = 0;

                    }
                    else if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_DATA)
                    {
                        if (mac->txData == pendPkt)
                        {
                            MESSAGE_Free(node, mac->rxCmd);
                            mac->rxCmd = NULL;
                            return;
                        }

                        mac->taskP.mcps_data_request_TxOptions = TxOp_Indirect;
                        mac->txData = pendPkt;
                        mac->waitDataAck = FALSE;
                        mac->numDataRetry = 0;
                    }
                    BOOL isChannelFree = FALSE;
                    Phy802_15_4PLMECCArequest(node,
                                              interfaceIndex,
                                              &isChannelFree);
                    if (Mac802_15_4CanProceedWOcsmaca(node,
                                                      interfaceIndex,
                                                      pendPkt)
                        && isChannelFree)
                    {

                        // Broadcast handling not done here as broadcast
                        // packet is notextracted indirectly by the RFD

                        if (mac->taskP.mcps_data_request_STEP)
                        {
                            if (strcmp(mac->taskP.mcps_data_request_frFunc,
                                       "csmacaCallBack") == 0)
                            {
                                strcpy(mac->taskP.mcps_data_request_frFunc,
                                       "PD_DATA_confirm");
                            }
                        }
                        else if (mac->taskP.mlme_associate_response_STEP)
                        {
                            if (strcmp(mac->
                                    taskP.mlme_associate_response_frFunc,
                                    "csmacaCallBack") == 0)
                            {
                               strcpy(mac->taskP.mlme_associate_response_frFunc,
                                    "PD_DATA_confirm");
                            }
                        }
                        mac->txCsmaca = pendPkt;
                        if (DEBUG)
                        {
                            wph = (M802_15_4Header*)
                                    MESSAGE_ReturnPacket(mac->txCsmaca);
                            printf("%" TYPES_64BITFMT "d : Node %d:"
                                    " 802.15.4MAC : Sending Data "
                                    "packet to Node %d with DSN %d, Size %d "
                                    "bytes\n",
                            node->getNodeTime(), node->nodeId,
                            wph->MHR_DstAddrInfo.addr_64, wph->MHR_BDSN,
                            MESSAGE_ReturnPacketSize(mac->txCsmaca));
                        }
                        mac->trx_state_req = TX_ON;
                        Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                           interfaceIndex,
                                                           TX_ON);
                        Mac802_15_4PLME_SET_TRX_STATE_confirm(
                                                            node,
                                                            interfaceIndex,
                                                            PHY_BUSY_TX);
                    }
                    else
                    {
                        Mac802_15_4CsmacaResume(node, interfaceIndex);
                    }
                }
            }
        }
        else if (wph->MSDU_CmdType == 0x08) // Coordinator realignment
        {
            mac->mpib.macPANId = *((UInt16 *)payld);
            memcpy(&(mac->mpib.macCoordShortAddress),
                    payld + 2,
                    sizeof(UInt16));
            mac->tmp_ppib.phyCurrentChannel = payld[4];
            Phy802_15_4PLME_SET_request(node,
                                        interfaceIndex,
                                        phyCurrentChannel,
                                        &mac->tmp_ppib);

            mac->mpib.macShortAddress = *((UInt16 *)(payld + 5));

            Mac802_15_4Dispatch(node,
                                interfaceIndex,
                                PHY_SUCCESS,
                                "IFSHandler",
                                RX_ON,
                                M802_15_4_SUCCESS);
        }
        MESSAGE_Free(node, mac->rxCmd);
        mac->rxCmd = NULL;
    }
    if (mac->rxData)
    {
        wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->rxData);
        M802_15_4PanAddrInfo srcAddrInfo;
        M802_15_4PanAddrInfo destAddrInfo;
        frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
        Mac802_15_4FrameCtrlParse(&frmCtrl);

        if (mac->taskP.mlme_poll_request_STEP)
        {
            Mac802_15_4Dispatch(node,
                                interfaceIndex,
                                PHY_SUCCESS,
                                "IFSHandler",
                                RX_ON,
                                M802_15_4_SUCCESS);
        }
        // else  // do nothing

        srcAddrInfo = wph->MHR_SrcAddrInfo;
        destAddrInfo = wph->MHR_DstAddrInfo;
        Mac802_15_4MCPS_DATA_indication(
                                    node,
                                    interfaceIndex,
                                    frmCtrl.srcAddrMode,
                                    srcAddrInfo.panID,
                                    srcAddrInfo.addr_64,
                                    frmCtrl.dstAddrMode,
                                    destAddrInfo.panID,
                                    destAddrInfo.addr_64,
                                    MESSAGE_ReturnPacketSize(mac->rxData),
                                    mac->rxData,
                                    0,
                                    FALSE,
                                    0);
        mac->rxData = NULL;

        if (mac->sendGtsRequestToPancoord)
        {
            // initiate GTS request
             if (!mac->gtsRequestData.receiveOnly)
             {
                 return;
             }
             mac->sendGtsRequestToPancoord  = FALSE;
             ERROR_Assert(mac->gtsRequestData.active,"mac->gtsRequestData."
                          " active should be TRUE");
             UInt8 gtsChracteristics
                 = Mac802_15_4CreateGtsChracteristics(
                                    mac->gtsRequestData.allocationRequest,
                                    mac->gtsRequestData.numSlots,
                                    mac->gtsRequestData.receiveOnly);
             Mac802_15_4MLME_GTS_request(node,
                                         interfaceIndex,
                                         gtsChracteristics,
                                         FALSE,
                                         PHY_SUCCESS);
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4BackoffBoundHandler
// LAYER      :: Mac
// PURPOSE    :: Handles back off bound timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4BackoffBoundHandler(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
               " BackoffBound Timer expired\n", 
               node->getNodeTime(), node->nodeId);
    }

    if (!mac->beaconWaiting)
    {
        if (mac->txAck)
        {
            mac->txPkt = mac->txAck;
            mac->inTransmission = TRUE;
            Message* pktToPhy = MESSAGE_Duplicate(node, mac->txPkt);
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               FORCE_TRX_OFF);
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               TX_ON);
            PHY_StartTransmittingSignal(node,
                                        interfaceIndex,
                                        pktToPhy,
                                        FALSE,
                                        0);
            mac->stats.numAckSent++;
            if (DEBUG)
            {
               M802_15_4Header* wph = (M802_15_4Header*)
                                            MESSAGE_ReturnPacket(mac->txPkt);
               printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                      " Sending Ack with DSN %d to Node %d\n",
                      node->getNodeTime(),
                      node->nodeId,
                      wph->MHR_BDSN, wph->MHR_DstAddrInfo.addr_64);
            }
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4OrphanRspHandler
// LAYER      :: Mac
// PURPOSE    :: Handles orphan response timeout timer event
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// RETURN  :: None
// **/
static
void Mac802_15_4OrphanRspHandler(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (DEBUG_TIMER)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Orphan"
               " timeout Timer expired\n", node->getNodeTime(), node->nodeId);
    }
    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        PHY_SUCCESS,
                        "orphanRspHandler",
                        RX_ON,
                        M802_15_4_SUCCESS);
}
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
                                PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    mac->inTransmission = FALSE;
    if (mac->txOverT != NULL)
    {
        MESSAGE_CancelSelfMsg(node, mac->txOverT);
        mac->txOverT = NULL;
    }
    if (mac->backoffStatus == BACKOFF_SUCCESSFUL)
    {
        mac->backoffStatus = BACKOFF_RESET;
    }
    if (status == PHY_SENSING || status == PHY_IDLE)
    {
        Mac802_15_4Dispatch(node,
                            interfaceIndex,
                            status,
                            "PD_DATA_confirm",
                            RX_ON,
                            M802_15_4_SUCCESS);
    }
    else if (mac->txPkt == mac->txBeacon)
    {
        mac->beaconWaiting = FALSE;
        MESSAGE_Free(node, mac->txBeacon);
        mac->txBeacon = NULL;
    }
    else if (mac->txPkt == mac->txAck)
    {
        MESSAGE_Free(node, mac->txAck);
        mac->txAck = NULL;
    }
}

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
                                 PhyStatusType status)
{
    MacData802_15_4* mac = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (mac->taskP.CCA_csmaca_STEP)
    {
        mac->taskP.CCA_csmaca_STEP = 0;
        Csma802_15_4CCA_confirm(node, interfaceIndex, status);
    }
}

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
                                UInt8 EnergyLevel)
{
    MacData802_15_4* mac = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    mac->energyLevel = EnergyLevel;
    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        status,
                        "PLME_ED_confirm",
                        RX_ON,
                        M802_15_4_SUCCESS);
}

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
                                 PHY_PIB* PIBAttributeValue)
{
    MacData802_15_4* mac = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    if (status == PHY_SUCCESS)
    {
        switch(PIBAttribute)
        {
            case phyCurrentChannel:
            {
                mac->tmp_ppib.phyCurrentChannel
                    = PIBAttributeValue->phyCurrentChannel;
                break;
            }
            case phyChannelsSupported:
            {
                mac->tmp_ppib.phyChannelsSupported
                    = PIBAttributeValue->phyChannelsSupported;
                break;
            }
            case phyTransmitPower:
            {
                mac->tmp_ppib.phyTransmitPower
                    = PIBAttributeValue->phyTransmitPower;
                break;
            }
            case phyCCAMode:
            {
                mac->tmp_ppib.phyCCAMode = PIBAttributeValue->phyCCAMode;
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4PLME_SET_TRX_STATE_confirm
// LAYER      :: Mac
// PURPOSE    :: Primitive to report result of Set TRX request
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + status         : PhyStatusType       : Status received from phy
// RETURN  :: None
// **/
void Mac802_15_4PLME_SET_TRX_STATE_confirm(Node* node,
                                           Int32 interfaceIndex,
                                           PhyStatusType status,
                                           BOOL toParent)
{
    MacData802_15_4* mac = NULL;
    clocktype delay = 0;
    PLMEsetTrxState state;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (status == PHY_SUCCESS)
    {
        state = mac->trx_state_req;
    }

    if (mac->backoffStatus == BACKOFF)
    {
        if (mac->trx_state_req == RX_ON)
        {
            if (mac->taskP.RX_ON_csmaca_STEP)
            {
                mac->taskP.RX_ON_csmaca_STEP = 0;
                Csma802_15_4RX_ON_confirm(node, interfaceIndex, RX_ON);
            }
        }
    }
    else
    {
        Mac802_15_4Dispatch(node,
                            interfaceIndex,
                            status,
                            "PLME_SET_TRX_STATE_confirm",
                            mac->trx_state_req,
                            M802_15_4_SUCCESS);
    }

    if (status != PHY_BUSY_TX)
    {
        return;
    }

    if (mac->beaconWaiting)
    {
       // Mac802_15_4TransmitCmdData(node, interfaceIndex);
        mac->txPkt = mac->txBeacon;
        mac->inTransmission = TRUE;
        Message* pktToPhy = MESSAGE_Duplicate(node, mac->txPkt);
        PHY_StartTransmittingSignal(node,
                                    interfaceIndex,
                                    pktToPhy,
                                    FALSE,
                                    0);
    }
    else if (mac->txGts)
    {
        mac->txPkt = mac->txGts;
        mac->inTransmission = TRUE;
        Message* pktToPhy = MESSAGE_Duplicate(node, mac->txPkt);
        PHY_StartTransmittingSignal(node,
                                    interfaceIndex,
                                    pktToPhy,
                                    FALSE,
                                    0);
    }
    else if (mac->txAck)
    {
        // although no CSMA-CA required for the transmission of ack.,
        // but we still need to locate the backoff period boundary if beacon
        // enabled

        if ((mac->mpib.macBeaconOrder == 15)
                && (mac->macBeaconOrder2 == 15))  // non-beacon enabled
        {
            delay = 0;
        }
        else     // beacon enabled
        {
            delay  = Mac802_15_4LocateBoundary(node,
                                               interfaceIndex,
                                               toParent,
                                               0);
        }
        if (delay == 0)
        {
            mac->txPkt = mac->txAck;
            mac->inTransmission = TRUE;
            Message* pktToPhy = MESSAGE_Duplicate(node, mac->txPkt);

            PHY_StartTransmittingSignal(node,
                                        interfaceIndex,
                                        pktToPhy,
                                        FALSE,
                                        0);
            mac->stats.numAckSent++;
        }
        else
        {
            mac->backoffBoundT = Mac802_15_4SetTimer(
                                                node,
                                                mac,
                                                M802_15_4BACKOFFBOUNDTIMER,
                                                delay,
                                                NULL);
        }
    }
    else
    {
        Mac802_15_4TransmitCmdData(node, interfaceIndex);
    }

}

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
                                 PPIBAenum PIBAttribute)
{
    MacData802_15_4* mac = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    if ((PIBAttribute == Phy802_15_4getChannelNumber(node, interfaceIndex)
            && (status == PHY_SUCCESS)))
    {
        Mac802_15_4Dispatch(node,
                            interfaceIndex,
                            status,
                            "PLME_SET_confirm",
                            RX_ON,
                            M802_15_4_SUCCESS);
    }
}

// /**
// NAME         Mac802_15_4GetMaxExpiraryCount
// PURPOSE      Caculates the MAX expirary count
// PARAMETERS   MacData_802_15_4* mac
//                  802.15.4 data structure
// RETURN       None
// NOTES        None
// **/
static
UInt16 Mac802_15_4GetMaxExpiraryCount(MacData802_15_4* mac)
{
    UInt16 expiraryCount = 0;
    if (mac->sfSpec.BO <= 8)
    {
        expiraryCount = 2 * (1 << (8 - mac->sfSpec.BO));
    }
    else
    {
        expiraryCount = 2 * 1;
    }
    return expiraryCount;
}

// --------------------------------------------------------------------------
// Helper APIs for MAC-SSCS interface
// --------------------------------------------------------------------------

// /**
// FUNCTION   :: Mac802_15_4mcps_data_request
// LAYER      :: Mac
// PURPOSE    :: Data transfer to peer entity
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
// + frUpper        : BOOL          : From upper layer
// + status         : PhyStatusType : PHY status
// + mStatus        : M802_15_4_enum: MAC status
// RETURN  :: None
// **/
static
void Mac802_15_4mcps_data_request(Node* node,
                                 Int32 interfaceIndex,
                                 UInt8 SrcAddrMode,
                                 UInt16 SrcPANId,
                                 MACADDR SrcAddr,
                                 UInt8 DstAddrMode,
                                 UInt16 DstPANId,
                                 MACADDR DstAddr,
                                 Int32 msduLength,
                                 Message* msdu,
                                 UInt8 msduHandle,
                                 UInt8 TxOptions,
                                 BOOL frUpper,
                                 PhyStatusType status,
                                 M802_15_4_enum mStatus)
{
    MacData802_15_4* mac = NULL;
    UInt8 step = 0;
    M802_15_4FrameCtrl frmCtrl;
    M802_15_4Header* wph = NULL;
    clocktype kpTime = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (TxOptions & TxOp_GTS)
    {
        step = mac->taskP.mcps_gts_data_request_STEP;
    }
    else if (mac->isBroadCastPacket)
    {
        step = mac->taskP.mcps_broadcast_request_STEP;
    }
    else
    {
        step = mac->taskP.mcps_data_request_STEP;
    }

    if (DEBUG)
    {
        printf("%" TYPES_64BITFMT "d : Node %d 802.15.4MAC: Sending Data"
            " packets to Destination "
            "(Direct/Indirect) Txoptions: %d Step: %d\n",
            node->getNodeTime(),
            node->nodeId,
            TxOptions,
            step);
    }
    if (step == 0)
    {
        // check if parameters valid or not
        if ((SrcAddrMode > 0x03) || (DstAddrMode > 0x03)
                || (msduLength > (Int32)aMaxMACFrameSize)
                || (TxOptions > 0x0f))
        {
            Sscs802_15_4MCPS_DATA_confirm(node,
                                          interfaceIndex,
                                          msduHandle,
                                          M802_15_4_INVALID_PARAMETER);

            if (mac->isBroadCastPacket)
            {
                mac->isBroadCastPacket = FALSE;
                mac->taskP.mcps_broadcast_request_STEP = 0;
            }

            ERROR_ReportWarning(
                    "Data packet size after MAC header is more than the"
                    " maximum size 802.15.4 supports (127 bytes)"
                    " at MAC layer. Packet will be dropped");

            MESSAGE_Free(node, msdu);
            msdu = NULL;
            if (TxOptions & TxOp_GTS)
            {
                M802_15_4GTSSpec* tmpgtsSpec = NULL;
                Mac802_15_4GetSpecks(node,
                                     mac,
                                     interfaceIndex,
                                     &tmpgtsSpec,
                                     NULL,
                                     NULL);
                tmpgtsSpec->msg[mac->currentGtsPositionDesc] = NULL;
                mac->currentGtsPositionDesc = M802_15_4_DEFAULTGTSPOSITION;
                mac->taskP.mcps_data_request_TxOptions
                    =  mac->taskP.mcps_data_request_Last_TxOptions;
            }
            mac->stats.numPktDropped++;
            return;
        }

        mac->taskP.mcps_data_request_TxOptions = TxOptions;
        frmCtrl.frmCtrl = 0;
        Mac802_15_4FrameCtrlSetFrmType(&frmCtrl,
                                       M802_15_4DEFFRMCTRL_TYPE_DATA);
        if (TxOptions & TxOp_Acked)
        {
            Mac802_15_4FrameCtrlSetAckReq(&frmCtrl, TRUE);
        }
        if (SrcPANId == DstPANId)
        {
            Mac802_15_4FrameCtrlSetIntraPan(&frmCtrl, TRUE);    // Intra PAN
        }
        Mac802_15_4FrameCtrlSetDstAddrMode(&frmCtrl, DstAddrMode);

        // we reverse the bit order -- note to use the required order
        // in implementation

        Mac802_15_4FrameCtrlSetSrcAddrMode(&frmCtrl, SrcAddrMode);

        // we reverse the bit order -- note to use the required order
        // in implementation

        BOOL ackRequired = FALSE;
        if (DstAddr != 0xFFFF && mac->mpib.macDataAcks)
        {
            ackRequired = TRUE;
        }
        Mac802_15_4AddCommandHeader(node,
                                    interfaceIndex,
                                    msdu,
                                    M802_15_4DEFFRMCTRL_TYPE_DATA,
                                    DstAddrMode,
                                    DstPANId,
                                    DstAddr,
                                    SrcAddrMode,
                                    SrcPANId,
                                    SrcAddr,
                                    FALSE,
                                    FALSE,
                                    ackRequired,
                                    0,
                                    0);
    }

    if (TxOptions & TxOp_GTS)   // GTS transmission
    {
        switch(step)
        {
            case GTS_INIT_STEP:
            {
                mac->taskP.mcps_gts_data_request_STEP++;
                ERROR_Assert(!mac->txGts, "mac->txGts should be NULL");
                mac->txGts = msdu;
                strcpy(mac->taskP.mlme_gts_data_request_frFunc,
                       "PD_DATA_confirm");
                    // enable the transmitter
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   FORCE_TRX_OFF);

                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX);
                mac->stats.numDataPktsSentInGts++;
                break;
            }
        case GTS_PKT_SENT_STEP:
            {
                wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txGts);
                frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
                Mac802_15_4FrameCtrlParse(&frmCtrl);
                if (frmCtrl.ackReq) // ack. required
                {
                    mac->taskP.mcps_gts_data_request_STEP++;
                    strcpy(mac->taskP.mlme_gts_data_request_frFunc,"recvAck");
                    // enable the receiver
                    mac->trx_state_req = RX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       RX_ON);

                    mac->txT =
                      Mac802_15_4SetTimer(
                        node,
                        mac,
                        M802_15_4TXTIMER,
                        mac->mpib.macAckWaitDuration * SECOND
                        / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                        NULL);
                    mac->waitDataAck = TRUE;
                }
                else
                {
                    mac->taskP.mcps_gts_data_request_STEP = GTS_INIT_STEP;
                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    mac->taskP.mcps_data_request_TxOptions
                        =  mac->taskP.mcps_data_request_Last_TxOptions;
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'g', FALSE);
                }
                break;
            }
        case GTS_ACK_STATUS_STEP:
            {
                if (status == PHY_SUCCESS)    // ack. received
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC "
                                ": Ack received "
                                "for data packet sent\n",
                                node->getNodeTime(), node->nodeId);
                    }
                     mac->taskP.mcps_gts_data_request_STEP = GTS_INIT_STEP;
                     mac->taskP.mcps_data_request_TxOptions
                        =  mac->taskP.mcps_data_request_Last_TxOptions;
                    
                    if (mac->isPANCoor
                         && mac->gtsSpec.recvOnly[mac->currentGtsPositionDesc]
                                == TRUE)
                    {
                        mac->gtsSpec.expiraryCount[mac->currentGtsPositionDesc] 
                                    = Mac802_15_4GetMaxExpiraryCount(mac);

                    }
                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'g', FALSE);
                }
                else                // time out when waiting for ack.
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: "
                                "802.15.4MAC : Timed out "
                                "while waiting for ack",
                                node->getNodeTime(), node->nodeId);
                    }
                    M802_15_4GTSSpec* tmpgtsSpec = NULL;
                    Mac802_15_4GetSpecks(node,
                                         mac,
                                         interfaceIndex,
                                         &tmpgtsSpec,
                                         NULL,
                                         NULL);
                    tmpgtsSpec->retryCount[mac->currentGtsPositionDesc]++;
                    if (tmpgtsSpec->retryCount[mac->currentGtsPositionDesc]
                            <= aMaxFrameRetries)
                    {
                        if (DEBUG)
                        {
                            printf(" retrying....\n");
                        }
                        ERROR_Assert(mac->txGts, "mac->txGts should point"
                                     " to a valid data packet");

                        // Check if data packet can be sent during GTS
                        if (Mac802_15_4CanProceedInGts(
                                node,
                                interfaceIndex,
                                mac,
                                mac->currentGtsPositionDesc,
                                tmpgtsSpec->msg[mac->currentGtsPositionDesc]))
                        {
                            mac->taskP.mcps_gts_data_request_STEP
                                = GTS_PKT_SENT_STEP;
                            strcpy(mac->taskP.mlme_gts_data_request_frFunc,
                                   "PD_DATA_confirm");

                            // enable the transmitter
                            Phy802_15_4PlmeSetTRX_StateRequest(
                                                        node,
                                                        interfaceIndex,
                                                        FORCE_TRX_OFF);

                            Phy802_15_4PlmeSetTRX_StateRequest(
                                                        node,
                                                        interfaceIndex,
                                                        TX_ON);
                            Mac802_15_4PLME_SET_TRX_STATE_confirm(
                                                        node,
                                                        interfaceIndex,
                                                        PHY_BUSY_TX);
                            mac->stats.numDataPktsSentInGts++;
                            mac->stats.numDataPktRetriedForNoAck++;
                        }
                        else
                        {
                            mac->taskP.mcps_gts_data_request_STEP
                                = GTS_INIT_STEP;
                            mac->txGts = NULL;
                            mac->taskP.mcps_data_request_TxOptions 
                                =  mac->taskP.mcps_data_request_Last_TxOptions;
                            mac->waitDataAck = FALSE;
                            mac->currentGtsPositionDesc
                                = M802_15_4_DEFAULTGTSPOSITION;
                        }
                    }
                    else
                    {
                        if (DEBUG)
                        {
                            printf("%" TYPES_64BITFMT "d : Node %d:"
                                   " 802.15.4MAC :"
                                   " Reached maximum number of retries."
                                   " Reporting to upper layer\n",
                                    node->getNodeTime(), node->nodeId);
                        }
                        mac->taskP.mcps_gts_data_request_STEP = GTS_INIT_STEP;
                        mac->taskP.mcps_data_request_TxOptions 
                            =  mac->taskP.mcps_data_request_Last_TxOptions;
                        Mac802_15_4Reset_TRX(node, interfaceIndex);
                        Mac802_15_4TaskFailed(node,
                                              interfaceIndex,
                                              'g',
                                              M802_15_4_NO_ACK,
                                              FALSE);
                    }
                }
                break;
            }
        case GTS_PKT_RETRY_STEP:
            {
                // retry with no addition of MAC header
                mac->taskP.mcps_gts_data_request_STEP = GTS_PKT_SENT_STEP;
                mac->stats.numDataPktsSentInGts++;
                mac->stats.numDataPktRetriedForNoAck++;
                ERROR_Assert(!mac->txGts, "mac->txGts should be non NULL");
                mac->txGts = msdu;
                strcpy(mac->taskP.mlme_gts_data_request_frFunc,
                       "PD_DATA_confirm");

                // enable the transmitter
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   FORCE_TRX_OFF);

                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);

                // mac->trx_state_req = TX_ON;
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if ((TxOptions & TxOp_Indirect) // indirect transmission
                && (mac->capability.FFD
                && (Mac802_15_4NumberDeviceLink(&mac->deviceLink1) 
                    > 0)))// coord
    {
        switch(step)
        {
            case INDIRECT_INIT_STEP:
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           " Transmitting using indirect mode to node %d\n",
                           node->getNodeTime(), node->nodeId, DstAddr);
                }
                mac->taskP.mcps_data_request_STEP++;
                mac->taskP.mcps_data_request_pendPkt = msdu;
                if ((DstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE16)
                    || (DstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE64))
                {

                    kpTime = (clocktype) (((aBaseSuperframeDuration)
                        * mac->mpib.macTransactionPersistenceTime)
                        / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]));
                    kpTime *= SECOND ;

                    Mac802_15_4ChkAddTransacLink(node,
                                                 &mac->transacLink1,
                                                 &mac->transacLink2,
                                                 DstAddrMode,
                                                 DstAddr,
                                                 msdu,
                                                 msduHandle,
                                                 kpTime);
                    strcpy(mac->taskP.mcps_data_request_frFunc,
                            "csmacaCallBack");
                    mac->dataWaitT = Mac802_15_4SetTimer(
                                        node,
                                        mac,
                                        M802_15_4DATAWAITTIMER,
                                        kpTime,
                                        NULL);
                }
                break;
            }
            case INDIRECT_PKT_SENT_STEP:
            {
                if (status == PHY_SUCCESS)    // data packet transmitted and,
                                            // if required, ack. received
                {
                    mac->taskP.mcps_data_request_STEP = INDIRECT_INIT_STEP;
                    mac->stats.numDataPktSent++;
                    if (mac->dataWaitT)
                    {
                        MESSAGE_CancelSelfMsg(node, mac->dataWaitT);
                        mac->dataWaitT = NULL;
                    }
                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'd', TRUE);
                }
                else                // data packet transmission failed
                {
                    // leave the packet in the queue waiting for next polling
                    strcpy(mac->taskP.mcps_data_request_frFunc,
                           "csmacaCallBack");
                    Mac802_15_4Reset_TRX(node, interfaceIndex);

                    Mac802_15_4TaskFailed(node,
                                          interfaceIndex,
                                          'd',
                                          M802_15_4_CHANNEL_ACCESS_FAILURE,
                                          TRUE);

                }
                break;
            }
            case INDIRECT_TIMER_EXPIRE_STEP:
            {
                // dataWait timer expired
                // The handling of this case has been shifted to function
                // "Mac802_15_4Dispatch"

                ERROR_Assert(FALSE, "Data wait timer expired unexpectedly");
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else                // direct transmission
    {
        switch(step)
        {
            case DIRECT_INIT_STEP:
            {
                if (mac->isBroadCastPacket)
                {
                    mac->taskP.mcps_broadcast_request_STEP++;
                }
                else
                {
                    mac->taskP.mcps_data_request_STEP++;
                }

                strcpy(mac->taskP.mcps_data_request_frFunc,
                       "csmacaCallBack");
                ERROR_Assert(!mac->txData, "mac->txData should be NULL");
                mac->txData = msdu;
                Mac802_15_4CsmacaBegin(node, interfaceIndex, 'd');
                break;
            }
            case DIRECT_CSMA_STATUS_STEP:
            {
                if (status == PHY_IDLE)
                {
                    if (mac->isBroadCastPacket)
                    {
                        mac->taskP.mcps_broadcast_request_STEP++;
                    }
                    else
                    {
                        mac->taskP.mcps_data_request_STEP++;
                    }

                    strcpy(mac->taskP.mcps_data_request_frFunc,
                           "PD_DATA_confirm");
                    // enable the transmitter
                    mac->trx_state_req = TX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       TX_ON);
                    // assumed that settrx_state() will always return true
                    Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                          interfaceIndex,
                                                          PHY_BUSY_TX);
                    mac->stats.numDataPktSent++;
                    if (DEBUG)
                    {
                        wph = (M802_15_4Header*)
                                MESSAGE_ReturnPacket(mac->txData);
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                                " : Sent Data "
                                "packet to Node %d with DSN %d, Size %d "
                                "bytes\n",
                                node->getNodeTime(), node->nodeId,
                                wph->MHR_DstAddrInfo.addr_64, wph->MHR_BDSN,
                                MESSAGE_ReturnPacketSize(mac->txData));
                    }
                }
                else
                {
                    wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txData);
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                                ": Unable to "
                                "send data to Node %d. Channel not IDLE\n",
                                node->getNodeTime(), node->nodeId,
                                wph->MHR_DstAddrInfo.addr_64);
                    }
                    if (wph->msduHandle)    // from SSCS
                    {
                        // let the upper layer handle the failure (no retry)
                         if (mac->isBroadCastPacket )
                        {
                            mac->isBroadCastPacket = FALSE;
                            mac->taskP.mcps_broadcast_request_STEP
                                = DIRECT_INIT_STEP;
                        }
                        else
                        {
                             mac->taskP.mcps_data_request_STEP
                                 = DIRECT_INIT_STEP;
                        }

                        Mac802_15_4Reset_TRX(node, interfaceIndex);
                        Mac802_15_4TaskFailed(
                                            node,
                                            interfaceIndex,
                                            'd',
                                            M802_15_4_CHANNEL_ACCESS_FAILURE,
                                            TRUE);
                    }
                    else
                    {
                        if (mac->isBroadCastPacket )
                        {
                            mac->isBroadCastPacket = FALSE;
                            mac->taskP.mcps_broadcast_request_STEP
                                = DIRECT_INIT_STEP;
                        }
                        else
                        {
                            mac->taskP.mcps_data_request_STEP
                                = DIRECT_INIT_STEP;
                        }

                        Mac802_15_4Reset_TRX(node, interfaceIndex);
                        Mac802_15_4TaskFailed(node,
                                              interfaceIndex,
                                              'd',
                                              M802_15_4_CHANNEL_ACCESS_FAILURE,
                                              FALSE);
                    }
                }
                break;
            }
            case DIRECT_PKT_SENT_STEP:
            {
                wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txData);
                frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
                Mac802_15_4FrameCtrlParse(&frmCtrl);
                if (frmCtrl.ackReq) // ack. required
                {
                    if (mac->isBroadCastPacket)
                    {
                         ERROR_Assert(FALSE, "Broadcast packet handling"
                                      " not done here");
                    }
                    mac->taskP.mcps_data_request_STEP++;
                    strcpy(mac->taskP.mcps_data_request_frFunc, "recvAck");

                    // enable the receiver
                    mac->trx_state_req = RX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       RX_ON);

                    mac->txT = Mac802_15_4SetTimer(
                                    node,
                                    mac,
                                    M802_15_4TXTIMER,
                                    mac->mpib.macAckWaitDuration * SECOND
                                    / Phy802_15_4GetSymbolRate(
                                        node->phyData[interfaceIndex]),
                                    NULL);

                    mac->waitDataAck = TRUE;
                }
                else        // assume success if ack. not required
                {

                    if (mac->isBroadCastPacket )
                    {
                        mac->isBroadCastPacket = FALSE;
                        mac->taskP.mcps_broadcast_request_STEP
                            = DIRECT_INIT_STEP;
                    }
                    else
                    {
                        // Review: if the following statement still needed.
                        mac->taskP.mcps_data_request_STEP
                            = DIRECT_INIT_STEP;
                    }

                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'd', TRUE);
                }
                break;
            }
            case DIRECT_ACK_STATUS_STEP:
            {
                if (status == PHY_SUCCESS)    // ack. received
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                               " : Ack received "
                               "for data packet sent\n",
                               node->getNodeTime(), node->nodeId);
                    }
                    mac->taskP.mcps_data_request_STEP = DIRECT_INIT_STEP;
                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'd', TRUE);
                }
                else                // time out when waiting for ack.
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                               " : Timed out "
                               "while waiting for ack",
                               node->getNodeTime(), node->nodeId);
                    }
                    mac->numDataRetry++;
                    if (mac->numDataRetry <= aMaxFrameRetries)
                    {
                        if (DEBUG)
                        {
                            printf(" retrying....\n");
                        }
                        mac->stats.numDataPktRetriedForNoAck++;
                        mac->taskP.mcps_data_request_STEP
                            = DIRECT_CSMA_STATUS_STEP;
                        strcpy(mac->taskP.mcps_data_request_frFunc,
                               "csmacaCallBack");
                        mac->waitDataAck = FALSE;
                        Mac802_15_4CsmacaResume(node, interfaceIndex);
                    }
                    else
                    {
                        if (DEBUG)
                        {
                            printf("%" TYPES_64BITFMT "d : Node %d:"
                                   " 802.15.4MAC :"
                                   " Reached maximum number of retries."
                                   " Reporting to upper layer\n",
                                   node->getNodeTime(), node->nodeId);
                        }
                        mac->taskP.mcps_data_request_STEP = DIRECT_INIT_STEP;
                        Mac802_15_4Reset_TRX(node, interfaceIndex);
                        Mac802_15_4TaskFailed(node,
                                              interfaceIndex,
                                              'd',
                                              M802_15_4_NO_ACK,
                                              TRUE);
                    }
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4mlme_scan_request
// LAYER      :: Mac
// PURPOSE    :: Initiate a channel scan over a given list of channels
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32       : Interface index
// + ScanType       : UInt8     : Type of scan (0-ED/Active/Passive/3-Orphan)
// + ScanChannels   : UInt32    : Channels to be scanned
// + ScanDuration   : UInt8     : Duration of scan, ignored for orphan scan
// + frUpper        : BOOL      : Whether from upper layer
// + status         : PhyStatusType   : Phy status
// RETURN  :: None
// **/
static
void Mac802_15_4mlme_scan_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 ScanType,
                                  UInt32 ScanChannels,
                                  UInt8 ScanDuration,
                                  BOOL frUpper,
                                  PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    UInt32 t_chanPos = 0;
    UInt8 step = 0;
    M802_15_4FrameCtrl frmCtrl;
    M802_15_4Header* wph = NULL;
    Int32 i = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    step = mac->taskP.mlme_scan_request_STEP;

    if (step == 0)
    {
        if ((ScanType > 3) || (ScanType != 3) && (ScanDuration > 16))
        {
            Sscs802_15_4MLME_SCAN_confirm(node,
                                          interfaceIndex,
                                          M802_15_4_INVALID_PARAMETER,
                                          ScanType,
                                          ScanChannels,
                                          0,
                                          NULL,
                                          NULL);
            return;
        }

        // disable the beacon
        mac->taskP.mlme_scan_request_orig_macBeaconOrder
            = mac->mpib.macBeaconOrder;
        mac->taskP.mlme_scan_request_orig_macBeaconOrder2
            = mac->macBeaconOrder2;
        mac->taskP.mlme_scan_request_orig_macBeaconOrder3
            = mac->macBeaconOrder3;
        mac->mpib.macBeaconOrder = 15;
        mac->macBeaconOrder2 = 15;
        mac->macBeaconOrder3 = 15;

        // stop the CSMA-CA if it is running
        if (mac->backoffStatus == BACKOFF)
        {
            mac->backoffStatus = BACKOFF_RESET;
            Csma802_15_4Cancel(node, interfaceIndex);
        }
        mac->taskP.mlme_scan_request_ScanType = ScanType;
    }

    if (ScanType == 0x00)       // ED scan
    {
        switch (step)
        {
            case ED_SCAN_INIT_STEP:
            {
                Phy802_15_4PLME_GET_request(
                        node,
                        interfaceIndex,
                        phyChannelsSupported);
                mac->taskP.mlme_scan_request_ScanChannels = ScanChannels;
                if ((mac->taskP.mlme_scan_request_ScanChannels &
                    mac->tmp_ppib.phyChannelsSupported) == 0)
                {
                    // restore the beacon order
                    mac->mpib.macBeaconOrder
                        = mac->taskP.mlme_scan_request_orig_macBeaconOrder;
                    mac->macBeaconOrder2
                        = mac->taskP.mlme_scan_request_orig_macBeaconOrder2;
                    mac->macBeaconOrder3
                        = mac->taskP.mlme_scan_request_orig_macBeaconOrder3;
                    Sscs802_15_4MLME_SCAN_confirm(node,
                                                  interfaceIndex,
                                                  M802_15_4_SUCCESS,
                                                  ScanType,
                                                  ScanChannels,
                                                  0,
                                                  NULL,
                                                  NULL);
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                    return;
                }
                mac->taskP.mlme_scan_request_STEP++;
                strcpy(mac->taskP.mlme_scan_request_frFunc,
                       "PLME_SET_confirm");
                mac->taskP.mlme_scan_request_CurrentChannel = 0;
                mac->taskP.mlme_scan_request_ListNum = 0;
                t_chanPos
                    = (1 << mac->taskP.mlme_scan_request_CurrentChannel);
               while ((t_chanPos & mac->taskP.mlme_scan_request_ScanChannels)
                        == 0
                    ||(t_chanPos & mac->tmp_ppib.phyChannelsSupported) == 0)
                {
                    mac->taskP.mlme_scan_request_CurrentChannel++;
                    t_chanPos =
                        (1 << mac->taskP.mlme_scan_request_CurrentChannel);
                }
                mac->tmp_ppib.phyCurrentChannel
                    = mac->taskP.mlme_scan_request_CurrentChannel;
                Phy802_15_4PLME_SET_request(node,
                                            interfaceIndex,
                                            phyCurrentChannel,
                                            &mac->tmp_ppib);
                break;
            }
            case ED_SCAN_SET_CONFIRM_STATUS_STEP:
            {
              mac->taskP.mlme_scan_request_STEP++;
              strcpy(mac->taskP.mlme_scan_request_frFunc,
                     "PLME_SET_TRX_STATE_confirm");
              mac->trx_state_req = RX_ON;
              Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                 interfaceIndex,
                                                 RX_ON);
              break;
            }
            case ED_SCAN_TRX_STATE_STATUS_STEP:
            {
                if (status == PHY_BUSY_RX)
                {
                    double sinr = 0;
                    double signalpower = 0;
                    mac->taskP.mlme_scan_request_STEP++;
                    strcpy(mac->taskP.mlme_scan_request_frFunc,
                            "PLME_ED_confirm");
                    Phy802_15_4GetLinkQuality(node,
                                              interfaceIndex,
                                              &sinr,
                                              &signalpower);
                    break;
                }
                // else  // fall through case 4
            }
            case ED_SCAN_ED_CONFIRM_STATUS_STEP:
            {
                // note: case 2 needs to fall through case 4
                if (step == ED_SCAN_ED_CONFIRM_STATUS_STEP)
                {
                    if (status == PHY_SUCCESS)
                    {
                        t_chanPos
                            = (1
                             << mac->taskP.mlme_scan_request_CurrentChannel);
                        mac->taskP.mlme_scan_request_ScanChannels &=
                                (t_chanPos ^ 0xffffffff);
                        mac->taskP.mlme_scan_request_EnergyDetectList
                            [mac->taskP.mlme_scan_request_ListNum]
                                =  mac->energyLevel;
                        mac->taskP.mlme_scan_request_ListNum++;
                    }
                }
                // fall through
            }
            case ED_SCAN_CONFIRM_STEP:
            {
                if ((mac->taskP.mlme_scan_request_ScanChannels
                        & mac->tmp_ppib.phyChannelsSupported) == 0)
                {
                    // restore the beacon order
                    mac->mpib.macBeaconOrder
                        = mac->taskP.mlme_scan_request_orig_macBeaconOrder;
                    mac->macBeaconOrder2
                        = mac->taskP.mlme_scan_request_orig_macBeaconOrder2;
                    mac->macBeaconOrder3
                        = mac->taskP.mlme_scan_request_orig_macBeaconOrder3;
                    mac->taskP.mlme_scan_request_STEP = ED_SCAN_INIT_STEP;

                    Sscs802_15_4MLME_SCAN_confirm(
                            node, interfaceIndex,
                            M802_15_4_SUCCESS, ScanType,
                            mac->taskP.mlme_scan_request_ScanChannels,
                            mac->taskP.mlme_scan_request_ListNum,
                            mac->taskP.mlme_scan_request_EnergyDetectList,
                            NULL);
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                    return;
                }
                mac->taskP.mlme_scan_request_STEP
                    = ED_SCAN_SET_CONFIRM_STATUS_STEP;
                strcpy(mac->taskP.mlme_scan_request_frFunc,
                       "PLME_SET_confirm");
                mac->taskP.mlme_scan_request_CurrentChannel++;
                t_chanPos =
                        (1 << mac->taskP.mlme_scan_request_CurrentChannel);
               while ((t_chanPos & mac->taskP.mlme_scan_request_ScanChannels)
                        == 0
                       ||(t_chanPos & mac->tmp_ppib.phyChannelsSupported)
                        == 0)
                {
                    mac->taskP.mlme_scan_request_CurrentChannel++;
                    t_chanPos
                        = (1 << mac->taskP.mlme_scan_request_CurrentChannel);
                }
                mac->tmp_ppib.phyCurrentChannel =
                        mac->taskP.mlme_scan_request_CurrentChannel;
                Phy802_15_4PLME_SET_request(node,
                                            interfaceIndex,
                                            phyCurrentChannel,
                                            &mac->tmp_ppib);
                break;
            }
            default:
            {
                break;
            }
        }
    }

    else if ((ScanType == 0x01) // active scan
              ||   (ScanType == 0x02))    // passive scan
    {
        switch (step)
        {
            case SCAN_INIT_STEP:
            {
                // no need to get supported channel and set that channel
                // as only one channel is currently supported.

                mac->taskP.mlme_scan_request_STEP++;
                mac->taskP.mlme_scan_request_orig_macPANId =
                        mac->mpib.macPANId;
                mac->mpib.macPANId = 0xffff;
                mac->taskP.mlme_scan_request_ScanDuration = ScanDuration;
                mac->taskP.mlme_scan_request_CurrentChannel = 0;
                mac->taskP.mlme_scan_request_ListNum = 0;
            }
            case SCAN_CREATE_BEACON_OR_SET_TRX_STATE_STEP:
            {
                if (ScanType == 0x01)       // active scan
                {
                    mac->taskP.mlme_scan_request_STEP++;
                    strcpy(mac->taskP.mlme_scan_request_frFunc,
                           "csmacaCallBack");

                    // --- send a beacon request command ---
                    ERROR_Assert(!mac->txBcnCmd2, "mac->txBcnCmd2 should be"
                                 " NULL");
                    Mac802_15_4ConstructPayload(node,
                                                interfaceIndex,
                                                &mac->txBcnCmd2,
                                                0x03,   // cmd
                                                0,
                                                0,
                                                0,
                                                0x07);  // Beacon request

                    Mac802_15_4AddCommandHeader(
                                        node,
                                        interfaceIndex,
                                        mac->txBcnCmd2,
                                        0x03,
                                        M802_15_4DEFFRMCTRL_ADDRMODE16,
                                        0xffff,
                                        0xffff,
                                        M802_15_4DEFFRMCTRL_ADDRMODENONE,
                                        0,
                                        (UInt16)node->nodeId,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        0x07,
                                        0);

                    Mac802_15_4CsmacaBegin(node, interfaceIndex, 'C');
                }
                else
                {
                    // skip steps only for passive scan
                    mac->taskP.mlme_scan_request_STEP
                        = SCAN_TRX_STATE_STATUS_STEP;
                    strcpy(mac->taskP.mlme_scan_request_frFunc,
                        "PLME_SET_TRX_STATE_confirm");
                    mac->trx_state_req = RX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(
                            node,
                            interfaceIndex,
                            RX_ON);
                }
                break;
            }
            case ACTIVE_SCAN_CSMA_STATUS_STEP:
            {
                if (status == PHY_IDLE)
                {
                    mac->taskP.mlme_scan_request_STEP++;
                    strcpy(mac->taskP.mlme_scan_request_frFunc,
                           "PD_DATA_confirm");
                    if (DEBUG)
                    {
                       printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                              " : Sending beacon request\n",
                              node->getNodeTime(), node->nodeId);
                    }
                    mac->trx_state_req = TX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       TX_ON);

                    // assumed that settrx_state() will always return true
                    Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                          interfaceIndex,
                                                          PHY_BUSY_TX);
                    mac->stats.numBeaconsReqSent++;
                    break;
                }
                else
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                                ": Channel not idle. Not sending request\n",
                                node->getNodeTime(), node->nodeId);
                    }
                    MESSAGE_Free(node, mac->txBcnCmd2);
                    mac->txBcnCmd2 = NULL;
                    // fall through case 7
                }
            }
            case ACTIVE_SCAN_SET_TRX_STATE_STEP:
            {
                if (step == ACTIVE_SCAN_SET_TRX_STATE_STEP)
                {
                    mac->taskP.mlme_scan_request_STEP++;
                    strcpy(mac->taskP.mlme_scan_request_frFunc,
                       "PLME_SET_TRX_STATE_confirm");
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'C',FALSE);
                    mac->trx_state_req = RX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       RX_ON);
                    // assumed that settrx_state() will always return true
                    Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                          interfaceIndex,
                                                          PHY_BUSY_RX);
                    break;
                }
            }
            case SCAN_TRX_STATE_STATUS_STEP:
            {
                if (step == SCAN_TRX_STATE_STATUS_STEP)
                {
                    if (status == PHY_BUSY_RX)
                    {
                        mac->taskP.mlme_scan_request_STEP++;
                        strcpy(mac->taskP.mlme_scan_request_frFunc,
                               "recvBeacon");
                        if (DEBUG)
                        {
                           printf("%" TYPES_64BITFMT "d : Node %d:"
                                  "802.15.4MAC : Listening for beacon\n",
                                  node->getNodeTime(), node->nodeId);
                        }

                        // schedule for next channel
                        mac->scanT
                            = Mac802_15_4SetTimer(
                                node,
                                mac,
                                M802_15_4SCANTIMER,
                                (aBaseSuperframeDuration * ((1 <<
                                mac->taskP.mlme_scan_request_ScanDuration)
                                + 1) * SECOND)
                                    / Phy802_15_4GetSymbolRate(
                                            node->phyData[interfaceIndex]),
                                NULL);
                        break;
                    }
                    // else  // fall through case 7
                }
            }
            case SCAN_BEACON_RCVD_STATUS_STEP:
            {
                if (step == SCAN_BEACON_RCVD_STATUS_STEP)
                {
                    // beacon received
                    // record the PAN descriptor if it is a new one

                    ERROR_Assert(mac->rxBeacon, "mac->rxBeacon should point"
                                 " to the received beacon message");
                    wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->rxBeacon);
                    frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
                    Mac802_15_4FrameCtrlParse(&frmCtrl);
                    for (i = 0; i < mac->taskP.mlme_scan_request_ListNum; i++)
                    {
                        if ((mac->
                           taskP.mlme_scan_request_PANDescriptorList[i].
                            LogicalChannel ==
                            mac->taskP.mlme_scan_request_CurrentChannel) &&
                            (mac->
                                taskP.mlme_scan_request_PANDescriptorList[i].
                                CoordAddrMode ==
                                frmCtrl.srcAddrMode) &&
                            (mac->
                                taskP.mlme_scan_request_PANDescriptorList[i].
                                CoordPANId  ==
                                wph->MHR_SrcAddrInfo.panID) &&
                            ((((frmCtrl.srcAddrMode ==
                                M802_15_4DEFFRMCTRL_ADDRMODE16) &&
                                (mac->taskP.
                                    mlme_scan_request_PANDescriptorList[i].
                                    CoordAddress_16 ==
                                    (wph->MHR_SrcAddrInfo.addr_16))) ||
                                ((frmCtrl.srcAddrMode ==
                                    M802_15_4DEFFRMCTRL_ADDRMODE64) &&
                                (mac->taskP.
                                    mlme_scan_request_PANDescriptorList[i].
                                    CoordAddress_64 ==
                                    wph->MHR_SrcAddrInfo.addr_64)))))
                            break;
                    }
                    if (i >= mac->taskP.mlme_scan_request_ListNum)  // unique
                    {
                        mac->taskP. mlme_scan_request_PANDescriptorList
                            [mac->taskP.mlme_scan_request_ListNum]
                                = mac->panDes2;
                        mac->taskP.mlme_scan_request_ListNum++;
                        if (mac->taskP.mlme_scan_request_ListNum >= 27)
                        {
                            // stop the timer
                            MESSAGE_CancelSelfMsg(node, mac->scanT);
                            mac->scanT = NULL;
                            // fall through case 7
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            case SCAN_CHANNEL_POS_CHANGE_STEP:
            {
                if (step == SCAN_CHANNEL_POS_CHANGE_STEP)
                {
                    t_chanPos =
                        (1 << mac->taskP.mlme_scan_request_CurrentChannel);
                    mac->taskP.mlme_scan_request_ScanChannels &=
                            (t_chanPos ^ 0xffffffff);
                    // fall through case 7
                }
            }
            case SCAN_CONFIRM_STEP:
            {
                if (((mac->taskP.mlme_scan_request_ScanChannels
                    & mac->tmp_ppib.phyChannelsSupported) == 0)
                    || (mac->taskP.mlme_scan_request_ListNum >= 27))
                {
                    mac->taskP.mlme_scan_request_STEP = SCAN_INIT_STEP;

                    Sscs802_15_4MLME_SCAN_confirm(
                            node, interfaceIndex,
                            M802_15_4_SUCCESS, ScanType,
                            mac->taskP.mlme_scan_request_ScanChannels,
                            mac->taskP.mlme_scan_request_ListNum,
                            NULL,
                            mac->taskP.mlme_scan_request_PANDescriptorList);
                    return;
                }
                mac->taskP.mlme_scan_request_STEP
                    = SCAN_CREATE_BEACON_OR_SET_TRX_STATE_STEP;
                strcpy(mac->taskP.mlme_scan_request_frFunc,
                       "PLME_SET_confirm");
                mac->taskP.mlme_scan_request_CurrentChannel++;
                t_chanPos =
                        (1 << mac->taskP.mlme_scan_request_CurrentChannel);
                while ((t_chanPos & mac->taskP.mlme_scan_request_ScanChannels)
                        == 0
                       || (t_chanPos & mac->tmp_ppib.phyChannelsSupported)
                        == 0)
                {
                    mac->taskP.mlme_scan_request_CurrentChannel++;
                    t_chanPos =
                        (1 << mac->taskP.mlme_scan_request_CurrentChannel);
                }
                mac->tmp_ppib.phyCurrentChannel
                    = mac->taskP.mlme_scan_request_CurrentChannel;
                Phy802_15_4PLME_SET_request(node,
                                            interfaceIndex,
                                            phyCurrentChannel,
                                            &mac->tmp_ppib);
                break;
            }
            default:
            {
                break;
            }
        }
    }

    else // if (ScanType == 0x03)    // orphan scan
    {
        switch (step)
        {
            case ORPHAN_SCAN_INIT_STEP:
            {
                mac->taskP.mlme_scan_request_STEP++;
            }
            case ORPHAN_SCAN_CREATE_BEACON_STEP:
            {
                mac->taskP.mlme_scan_request_STEP++;
                ERROR_Assert(!mac->txBcnCmd2, "mac->txBcnCmd2 should be"
                             " NULL");
                strcpy(mac->taskP.mlme_scan_request_frFunc,
                       "csmacaCallBack");
                Mac802_15_4ConstructPayload(node,
                                            interfaceIndex,
                                            &mac->txBcnCmd2,
                                            0x03,   // cmd
                                            0,
                                            0,
                                            0,
                                            0x06);  // Orphan notification

                Mac802_15_4AddCommandHeader(
                                        node,
                                        interfaceIndex,
                                        mac->txBcnCmd2,
                                        0x03,
                                        M802_15_4DEFFRMCTRL_ADDRMODE64,
                                        mac->mpib.macPANId,
                                        mac->mpib.macCoordExtendedAddress,
                                        M802_15_4DEFFRMCTRL_ADDRMODE64,
                                        mac->mpib.macPANId,
                                        mac->aExtendedAddress,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        0x06,
                                        0);
                Mac802_15_4CsmacaBegin(node, interfaceIndex, 'C');
                break;
            }
            case ORPHAN_SCAN_CSMA_STATUS_STEP:
            {
                if (status == PHY_IDLE)
                {
                    mac->taskP.mlme_scan_request_STEP++;
                    strcpy(mac->taskP.mlme_scan_request_frFunc,
                           "PD_DATA_confirm");
                    mac->trx_state_req = TX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       TX_ON);
                    Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                          interfaceIndex,
                                                          PHY_BUSY_TX);
                    break;
                }
                else
                {
                    MESSAGE_Free(node, mac->txBcnCmd2);
                    mac->txBcnCmd2 = NULL;
                    // fall through case ORPHAN_SCAN_CONFIRM_STEP
                }
            }
            case ORPHAN_SCAN_BEACON_SENT_STATUS_STEP:
            {
                if (step == ORPHAN_SCAN_BEACON_SENT_STATUS_STEP)
                {
                    mac->taskP.mlme_scan_request_STEP++;
                    strcpy(mac->taskP.mlme_scan_request_frFunc,
                            "PLME_SET_TRX_STATE_confirm");
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', FALSE);
                    mac->trx_state_req = RX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       RX_ON);
                    Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                          interfaceIndex,
                                                          PHY_BUSY_RX);
                    break;
                }
            }
            case ORPHAN_SCAN_TRX_STATE_STATUS_STEP:
            {
                if (step == ORPHAN_SCAN_TRX_STATE_STATUS_STEP)
                {
                    if (status == PHY_BUSY_RX)
                    {
                        mac->taskP.mlme_scan_request_STEP++;
                        strcpy(mac->taskP.mlme_scan_request_frFunc,
                               "IFSHandler");

                        mac->scanT =
                             Mac802_15_4SetTimer(
                                   node,
                                   mac,
                                   M802_15_4SCANTIMER,
                                   aResponseWaitTime * SECOND
                                   / Phy802_15_4GetSymbolRate(
                                        node->phyData[interfaceIndex]),
                                   NULL);
                        break;
                    }
                    // else  // fall through case ORPHAN_SCAN_CONFIRM_STEP
                }
            }
            case ORPHAN_SCAN_COOR_REALIGNMENT_RECVD_STATUS_STEP:
            {
                if (step == ORPHAN_SCAN_COOR_REALIGNMENT_RECVD_STATUS_STEP)
                {
                    if (status == PHY_SUCCESS)    // coordinator realignment
                    {
                        if (mac->scanT)
                        {
                            MESSAGE_CancelSelfMsg(node, mac->scanT);
                            mac->scanT = NULL;
                        }
                        mac->mpib.macBeaconOrder =
                           mac->taskP.mlme_scan_request_orig_macBeaconOrder;
                        mac->macBeaconOrder2 =
                          mac->taskP.mlme_scan_request_orig_macBeaconOrder2;
                        mac->macBeaconOrder3 =
                          mac->taskP.mlme_scan_request_orig_macBeaconOrder3;
                        mac->taskP.mlme_scan_request_STEP
                            = ORPHAN_SCAN_INIT_STEP;

                        t_chanPos =
                            (1 << mac->
                                taskP.mlme_scan_request_CurrentChannel);
                        mac->taskP.mlme_scan_request_ScanChannels &=
                            (t_chanPos ^ 0xffffffff);

                        Sscs802_15_4MLME_SCAN_confirm(
                                node, interfaceIndex,
                                M802_15_4_SUCCESS, ScanType,
                                mac->taskP.mlme_scan_request_ScanChannels,
                                0, NULL, NULL);
                        Mac802_15_4CsmacaResume(node, interfaceIndex);
                        break;
                    }
                    else    // time out
                    {
                       t_chanPos =
                           (1
                            << mac->taskP.mlme_scan_request_CurrentChannel);
                        mac->taskP.mlme_scan_request_ScanChannels
                          &= (t_chanPos ^ 0xffffffff);
                        // fall through case ORPHAN_SCAN_CONFIRM_STEP
                    }
                }
            }
            case ORPHAN_SCAN_CONFIRM_STEP:
            {
                if ((mac->taskP.mlme_scan_request_ScanChannels
                    & mac->tmp_ppib.phyChannelsSupported) == 0)
                {
                    mac->mpib.macBeaconOrder
                        = mac->taskP.mlme_scan_request_orig_macBeaconOrder;
                    mac->macBeaconOrder2 = 15;
                    mac->macBeaconOrder3
                        = mac->taskP.mlme_scan_request_orig_macBeaconOrder3;
                    mac->taskP.mlme_scan_request_STEP = ORPHAN_SCAN_INIT_STEP;

                    Sscs802_15_4MLME_SCAN_confirm(
                                node, interfaceIndex,
                                M802_15_4_NO_BEACON,
                                ScanType,
                                mac->taskP.mlme_scan_request_ScanChannels,
                                0,
                                NULL,
                                NULL);
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                    return;
                }
                mac->taskP.mlme_scan_request_STEP
                    = ORPHAN_SCAN_CREATE_BEACON_STEP;
                strcpy(mac->taskP.mlme_scan_request_frFunc,
                       "PLME_SET_confirm");
                mac->taskP.mlme_scan_request_CurrentChannel++;
                t_chanPos =
                        (1 << mac->taskP.mlme_scan_request_CurrentChannel);
               while ((t_chanPos & mac->taskP.mlme_scan_request_ScanChannels)
                        == 0
                       ||(t_chanPos & mac->tmp_ppib.phyChannelsSupported)
                       == 0)
                {
                    mac->taskP.mlme_scan_request_CurrentChannel++;
                    t_chanPos
                        = (1 << mac->taskP.mlme_scan_request_CurrentChannel);
                }
                mac->tmp_ppib.phyCurrentChannel
                    = mac->taskP.mlme_scan_request_CurrentChannel;
                Phy802_15_4PLME_SET_request(node,
                                            interfaceIndex,
                                            phyCurrentChannel,
                                            &mac->tmp_ppib);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4mlme_start_request
// LAYER      :: Mac
// PURPOSE    :: Allows the PAN coordinator to initiate a new PAN
//               or to begin using a new superframe configuration
// PARAMETERS ::
// + node                   : Node*     : Node receiving call
// + interfaceIndex         : Int32       : Interface index
// + PANId                  : UInt16    : PAN id
// + LogicalChannel         : UInt8     : Logical channel
// + BeaconOrder            : UInt8     : How often a beacon is tx'd.
// + SuperframeOrder        : UInt8     :Length of active portion of s-frame
// + PANCoordinator         : BOOL      : If device is PAN coordinator
// + BatteryLifeExtension   : BOOL      : for battery saving mode
// + CoordRealignment       : BOOL      : If coordinator realignment command
//                     needs to be sent prior to changing superframe config
// + SecurityEnable         : BOOL      : If security is enabled
// + frUpper                : BOOL      : Whether from upper layer
// + status                 : PhyStatusType   : Phy status
// RETURN  :: None
// **/
static
void Mac802_15_4mlme_start_request(Node* node,
                                   Int32 interfaceIndex,
                                   UInt16 PANId,
                                   UInt8 LogicalChannel,
                                   UInt8 BeaconOrder,
                                   UInt8 SuperframeOrder,
                                   BOOL PANCoordinator,
                                   BOOL BatteryLifeExtension,
                                   BOOL CoordRealignment,
                                   BOOL SecurityEnable,
                                   BOOL frUpper,
                                   PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    UInt8 origBeaconOrder = 0;
    UInt8 step = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    step = mac->taskP.mlme_start_request_STEP;
    switch (step)
    {
        case START_INIT_STEP:
        {
            if (mac->mpib.macShortAddress == 0xffff)
            {
                Sscs802_15_4MLME_START_confirm(node,
                                               interfaceIndex,
                                               M802_15_4_NO_SHORT_ADDRESS);
                return;
            }
            else if ((BeaconOrder > 15)
                || ((SuperframeOrder > BeaconOrder)
                && (SuperframeOrder != 15)))
            {
                Sscs802_15_4MLME_START_confirm(node,
                                               interfaceIndex,
                                               M802_15_4_INVALID_PARAMETER);
                return;
            }
            else if (mac->capability.FFD != TRUE)
            {
                Sscs802_15_4MLME_START_confirm(node,
                                               interfaceIndex,
                                               M802_15_4_UNDEFINED);
                return;
            }

            // change the configuration and begin to transmit beacons
            // after the transmission of the realignment command

            mac->taskP.mlme_start_request_BeaconOrder = BeaconOrder;
            mac->taskP.mlme_start_request_SuperframeOrder
                = SuperframeOrder;
            mac->taskP.mlme_start_request_BatteryLifeExtension
                = BatteryLifeExtension;
            mac->taskP.mlme_start_request_SecurityEnable
                = SecurityEnable;
            mac->taskP.mlme_start_request_PANCoordinator
                = PANCoordinator;
            mac->taskP.mlme_start_request_PANId = PANId;
            mac->taskP.mlme_start_request_LogicalChannel
                = LogicalChannel;

            if (CoordRealignment == TRUE)       // send a realignment command
                    // before changing configuration that affects the command
            {
                mac->taskP.mlme_start_request_STEP++;
                strcpy(mac->taskP.mlme_start_request_frFunc,
                       "csmacaCallBack");
                // broadcast a realignment command
                ERROR_Assert(!mac->txBcnCmd2, "mac->txBcnCmd2 should be"
                             " NULL");
                Mac802_15_4ConstructPayload(node,
                                            interfaceIndex,
                                            &mac->txBcnCmd2,
                                            0x03,   // cmd
                                            0,
                                            0,
                                            0,
                                            0x08);  // Realignmnet command

                char* payld = MESSAGE_ReturnPacket(mac->txBcnCmd2) +
                        sizeof (M802_15_4CommandFrame);

                *((UInt16*)payld) = PANId;
                memcpy(payld + 2, &(mac->mpib.macShortAddress),
                        sizeof(UInt16));
                payld[4] = LogicalChannel;
                memset(payld + 5, 0xffff, sizeof(UInt16));// short address

                Mac802_15_4AddCommandHeader(node,
                                            interfaceIndex,
                                            mac->txBcnCmd2,
                                            0x03,
                                            M802_15_4DEFFRMCTRL_ADDRMODE16,
                                            0xffff,
                                            0xffff,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->mpib.macPANId,
                                            mac->aExtendedAddress,
                                            FALSE,
                                            FALSE,
                                            FALSE,
                                            0,
                                            0);
                Mac802_15_4CsmacaBegin(node, interfaceIndex, 'C');
                break;
            }
            else
            {
                mac->taskP.mlme_start_request_STEP
                    = START_TRX_STATE_STATUS_STEP;
                step = START_TRX_STATE_STATUS_STEP;
                // fall through case 2
            }
        }
        case START_CSMA_STATUS_STEP:
        {
            if (step == START_CSMA_STATUS_STEP)
            {
                if (status == PHY_IDLE)
                {
                    mac->taskP.mlme_start_request_STEP++;
                    strcpy(mac->taskP.mlme_start_request_frFunc,
                           "PD_DATA_confirm");
                    mac->trx_state_req = TX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       TX_ON);
                    break;
                }
                else
                {
                    MESSAGE_Free(node, mac->txBcnCmd2);
                    mac->txBcnCmd2 = NULL;

                    // fall through case case 2 -- ignore the failure and
                    // continue to transmit beacons

                    mac->taskP.mlme_start_request_STEP
                        = START_TRX_STATE_STATUS_STEP;
                }
            }
        }
        case START_TRX_STATE_STATUS_STEP:
        {
            mac->taskP.mlme_start_request_STEP++;
            strcpy(mac->taskP.mlme_start_request_frFunc,
                   "PD_DATA_confirm"); // for beacon
            Mac802_15_4Reset_TRX(node, interfaceIndex);
            if (CoordRealignment == TRUE)
            {
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', FALSE);
            }

            // change the configuration
            origBeaconOrder = mac->mpib.macBeaconOrder;
            mac->mpib.macBeaconOrder = BeaconOrder;
            if (BeaconOrder == 15)
            {
                mac->mpib.macSuperframeOrder = 15;
            }
            else
            {
                mac->mpib.macSuperframeOrder = SuperframeOrder;
            }
            mac->mpib.macBattLifeExt = BatteryLifeExtension;
            mac->secuBeacon = SecurityEnable;
            mac->isPANCoor = PANCoordinator;
            if (PANCoordinator == TRUE)
            {
                mac->mpib.macPANId = PANId;
                mac->mpib.macCoordExtendedAddress = mac->aExtendedAddress;
                mac->mpib.macCoordShortAddress = mac->aExtendedAddress;
                mac->tmp_ppib.phyCurrentChannel = LogicalChannel;
                Phy802_15_4PLME_SET_request(node,
                                            interfaceIndex,
                                            phyCurrentChannel,
                                            &mac->tmp_ppib);
            }
            if (origBeaconOrder == BeaconOrder)
            {
                mac->taskP.mlme_start_request_STEP = START_INIT_STEP;
                Sscs802_15_4MLME_START_confirm(node,
                                               interfaceIndex,
                                               M802_15_4_SUCCESS);
                Mac802_15_4CsmacaResume(node, interfaceIndex);
            }
            else if ((origBeaconOrder == 15) && (BeaconOrder < 15))
            {
                if (mac->bcnTxT)
                {
                    MESSAGE_CancelSelfMsg(node, mac->bcnTxT);
                }

                // transmit beacon immediately
                mac->bcnTxT =
                        Mac802_15_4SetTimer(node,
                                            mac,
                                            M802_15_4BEACONTXTIMER,
                                            0,
                                            NULL);
            }
            else if ((origBeaconOrder < 15) && (BeaconOrder == 15))
            {
                mac->oneMoreBeacon = TRUE;
            }
            break;
        }
        case START_BEACON_SENT_STATUS_STEP:
        {
            mac->taskP.mlme_start_request_STEP = START_INIT_STEP;
            Sscs802_15_4MLME_START_confirm(node,
                                           interfaceIndex,
                                           M802_15_4_SUCCESS);
            Mac802_15_4TaskSuccess(node, interfaceIndex, 'b', TRUE);
            break;
        }
        default:
        {
            break;
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4mlme_associate_request
// LAYER      :: Mac
// PURPOSE    :: Request an association with a coordinator
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + LogicalChannel : UInt8         : Channel on which attempt is to be done
// + CoordAddrMode  : UInt8         : Coordinator address mode
// + CoordPANId     : UInt16        : Coordinator PAN id
// + CoordAddress   : MACADDR    : Coordinator address
// + CapabilityInformation : UInt8  : capabilities of associating device
// + SecurityEnable : BOOL          : Whether enable security or not
// + frUpper        : BOOL          : Whether from upper layer
// + status         : PhyStatusType       : Phy status
// + mStaus         : M802_15_4_enum: Mac status
// RETURN  :: None
// **/
static
void Mac802_15_4mlme_associate_request(Node* node,
                                       Int32 interfaceIndex,
                                       UInt8 LogicalChannel,
                                       UInt8 CoordAddrMode,
                                       UInt16 CoordPANId,
                                       MACADDR CoordAddress,
                                       UInt8 CapabilityInformation,
                                       BOOL SecurityEnable,
                                       BOOL frUpper,
                                       PhyStatusType status,
                                       M802_15_4_enum mStatus)
{
    MacData802_15_4* mac = NULL;
    M802_15_4FrameCtrl frmCtrl;
    UInt8 step = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    step = mac->taskP.mlme_associate_request_STEP;
    switch(step)
    {
        case ASSOC_INIT_STEP:
        {
            mac->tmp_ppib.phyCurrentChannel = LogicalChannel;
            Phy802_15_4PLME_SET_request(node,
                                        interfaceIndex,
                                        phyCurrentChannel,
                                        &mac->tmp_ppib);
            mac->mpib.macPANId = CoordPANId;
            mac->mpib.macCoordExtendedAddress = CoordAddress;
            mac->mpib.macCoordShortAddress = CoordAddress;
            mac->taskP.mlme_associate_request_STEP++;
            strcpy(mac->taskP.mlme_associate_request_frFunc,
                   "csmacaCallBack");
            mac->taskP.mlme_associate_request_CoordAddrMode = CoordAddrMode;
            mac->taskP.mlme_associate_request_SecurityEnable =
                SecurityEnable;
            // --- send an association request command ---
            ERROR_Assert(!mac->txBcnCmd2, "mac->txBcnCmd2 should be NULL");
            Mac802_15_4ConstructPayload(node,
                                        interfaceIndex,
                                        &mac->txBcnCmd2,
                                        0x03,   // cmd
                                        0,
                                        0,
                                        0,
                                        0x01);  // Association request

            char* payld = MESSAGE_ReturnPacket(mac->txBcnCmd2) +
                    sizeof (M802_15_4CommandFrame);

            payld[0] = mac->capability.cap;

            Mac802_15_4AddCommandHeader(node,
                                        interfaceIndex,
                                        mac->txBcnCmd2,
                                        0x03,
                                        CoordAddrMode,
                                        CoordPANId,
                                        CoordAddress,
                                        M802_15_4DEFFRMCTRL_ADDRMODE64,
                                        0xffff,
                                        mac->aExtendedAddress,
                                        SecurityEnable,
                                        FALSE,
                                        TRUE,
                                        0x01,
                                        0);
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       " Preparing to send "
                       "Associate request to Coordinator %d\n",
                node->getNodeTime(), node->nodeId, CoordAddress);
            }
            Mac802_15_4CsmacaBegin(node, interfaceIndex, 'C');
            break;
        }
        case ASSOC_CSMA_STATUS_STEP:
        {
            if (status == PHY_IDLE)
            {
                mac->taskP.mlme_associate_request_STEP++;
                strcpy(mac->taskP.mlme_associate_request_frFunc,
                       "PD_DATA_confirm");
                mac->trx_state_req = TX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);
                // assumed that settrx_state() will always return true
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX);
            }
            else
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           " Unable to send "
                           "Associate request.\n",
                    node->getNodeTime(), node->nodeId);
                }
                mac->taskP.mlme_associate_request_STEP = ASSOC_INIT_STEP;
                MESSAGE_Free(node, mac->txBcnCmd2);
                mac->txBcnCmd2 = NULL;

                // restore default values
                mac->mpib.macPANId = M802_15_4_PANID;
                mac->mpib.macCoordExtendedAddress
                    = M802_15_4_COORDEXTENDEDADDRESS;
                mac->mpib.macCoordShortAddress
                    = M802_15_4_COORDSHORTADDRESS;
                Sscs802_15_4MLME_ASSOCIATE_confirm(
                                        node,
                                        interfaceIndex,
                                        0,
                                        M802_15_4_CHANNEL_ACCESS_FAILURE);
                Mac802_15_4CsmacaResume(node, interfaceIndex);
                return;
            }
            break;
        }
        case ASSOC_PKT_SENT_STEP:
        {
            mac->taskP.mlme_associate_request_STEP++;
            strcpy(mac->taskP.mlme_associate_request_frFunc,
                   "recvAck");
            mac->trx_state_req = RX_ON;
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               RX_ON);

            mac->txT =
                Mac802_15_4SetTimer(node,
                                    mac,
                                    M802_15_4TXTIMER,
                                    mac->mpib.macAckWaitDuration * SECOND
                                    / Phy802_15_4GetSymbolRate(
                                        node->phyData[interfaceIndex]),
                                    NULL);

            mac->waitBcnCmdAck2 = TRUE;
            break;
        }
        case ASSOC_ACK_STATUS_STEP:
        {
            if (status == PHY_SUCCESS)    // ack. received
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           "Received ack for "
                           "Associate request.\n",
                    node->getNodeTime(), node->nodeId);
                }
                mac->taskP.mlme_associate_request_STEP++;
                strcpy(mac->taskP.mlme_associate_request_frFunc,
                       "extractHandler");
                mac->trx_state_req = TRX_OFF;

                // TRX state for coordinator/device should not be OFF if
                // macRxOnWhenIdle is TRUE.

                if (!mac->mpib.macRxOnWhenIdle)
                {
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       TRX_OFF);
                }

           Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', FALSE);

           mac->extractT =
                 Mac802_15_4SetTimer(
                     node,
                     mac,
                     M802_15_4EXTRACTTIMER,
                     aResponseWaitTime * SECOND
                     / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                     NULL);
            }
            else                // time out when waiting for ack.
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           " Timed out while "
                           "waiting for ack for Associate request.",
                    node->getNodeTime(), node->nodeId);
                }
                mac->numBcnCmdRetry2++;
                if (mac->numBcnCmdRetry2 <= aMaxFrameRetries)
                {
                    if (DEBUG)
                    {
                        printf(" retrying...\n");
                    }

                    mac->taskP.mlme_associate_request_STEP
                        = ASSOC_CSMA_STATUS_STEP;
                    strcpy(mac->taskP.mlme_associate_request_frFunc,
                           "csmacaCallBack");
                    mac->waitBcnCmdAck2 = FALSE;
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                }
                else
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                               ": Maximum"
                               " number of retries reached."
                               " Reporting failure to upper layer\n",
                               node->getNodeTime(),
                               node->nodeId);
                    }
                    mac->taskP.mlme_associate_request_STEP = ASSOC_INIT_STEP;
                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    MESSAGE_Free(node, mac->txBcnCmd2);
                    mac->txBcnCmd2 = NULL;

                    // restore default values
                    mac->mpib.macPANId = M802_15_4_PANID;
                    mac->mpib.macCoordExtendedAddress
                        = M802_15_4_COORDEXTENDEDADDRESS;
                    mac->mpib.macCoordShortAddress
                        = M802_15_4_COORDSHORTADDRESS;
                    Sscs802_15_4MLME_ASSOCIATE_confirm(node,
                                                       interfaceIndex,
                                                       0,
                                                       M802_15_4_NO_ACK);
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                    return;
                }
            }
            break;
        }
        case ASSOC_DATAREQ_INIT_STEP:
        {
            mac->taskP.mlme_associate_request_STEP++;
            strcpy(mac->taskP.mlme_associate_request_frFunc,
                   "csmacaCallBack");
            ERROR_Assert(!mac->txBcnCmd2, "mac->txBcnCmd2 should be NULL");
            Mac802_15_4ConstructPayload(node,
                                        interfaceIndex,
                                        &mac->txBcnCmd2,
                                        0x03,   // cmd
                                        0,
                                        0,
                                        0,
                                        0x04);  // Data request

            Mac802_15_4AddCommandHeader(
                            node,
                            interfaceIndex,
                            mac->txBcnCmd2,
                            0x03,
                            mac->taskP.mlme_associate_request_CoordAddrMode,
                            mac->mpib.macPANId,
                            mac->mpib.macCoordExtendedAddress,
                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                            mac->mpib.macPANId,
                            mac->aExtendedAddress,
                            SecurityEnable,
                            FALSE,
                            TRUE,
                            0x04,
                            0);

            if ((mac->mpib.macShortAddress != 0xfffe) &&
                 (mac->mpib.macShortAddress != 0xffff))
            {
                ((M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd2))->
                        MHR_SrcAddrInfo.addr_16 =
                            mac->mpib.macShortAddress;

                frmCtrl.frmCtrl =
                        ((M802_15_4Header*)
                            MESSAGE_ReturnPacket(mac->txBcnCmd2))->MHR_FrmCtrl;
                Mac802_15_4FrameCtrlParse(&frmCtrl);

                Mac802_15_4FrameCtrlSetSrcAddrMode(
                        &frmCtrl,
                        M802_15_4DEFFRMCTRL_ADDRMODE16);
                ((M802_15_4Header*)
                    MESSAGE_ReturnPacket(mac->txBcnCmd2))->MHR_FrmCtrl
                        = frmCtrl.frmCtrl;
            }

            mac->waitBcnCmdAck2 = FALSE;     // command packet not yet txed
            mac->numBcnCmdRetry2 = 0;
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       " Sending Data "
                       "request to Coordinator.\n",
                node->getNodeTime(), node->nodeId);
            }

            Mac802_15_4CsmacaBegin(node, interfaceIndex, 'C');
            break;
        }
        case ASSOC_DATAREQ_CSMA_STATUS_STEP:
        {
            if (status == PHY_IDLE)
            {
                mac->taskP.mlme_associate_request_STEP++;
                strcpy(mac->taskP.mlme_associate_request_frFunc,
                       "PD_DATA_confirm");
                mac->trx_state_req = TX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);
                // assumed that settrx_state() will always return true
                mac->stats.numDataReqSent++;
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX);
            }
            else
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           " Unable to send "
                           "Data request.\n",
                    node->getNodeTime(), node->nodeId);
                }
                mac->taskP.mlme_associate_request_STEP = ASSOC_INIT_STEP;
                MESSAGE_Free(node, mac->txBcnCmd2);
                mac->txBcnCmd2 = NULL;

                // restore default values
                mac->mpib.macPANId = M802_15_4_PANID;
                mac->mpib.macCoordExtendedAddress
                    = M802_15_4_COORDEXTENDEDADDRESS;
                mac->mpib.macCoordShortAddress
                    = M802_15_4_COORDSHORTADDRESS;
                Sscs802_15_4MLME_ASSOCIATE_confirm(
                                            node,
                                            interfaceIndex,
                                            0,
                                            M802_15_4_CHANNEL_ACCESS_FAILURE);
                Mac802_15_4CsmacaResume(node, interfaceIndex);
                return;
            }
            break;
        }
        case ASSOC_DATAREQ_PKT_SENT_STEP:
        {
            mac->taskP.mlme_associate_request_STEP++;
            strcpy(mac->taskP.mlme_associate_request_frFunc,
                   "recvAck");
            // enable the receiver
            mac->trx_state_req = RX_ON;
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               RX_ON);
            Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                  interfaceIndex,
                                                  PHY_BUSY_RX);

            mac->txT =
                 Mac802_15_4SetTimer(
                    node,
                    mac,
                    M802_15_4TXTIMER,
                    mac->mpib.macAckWaitDuration * SECOND
                    / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                    NULL);

            mac->waitBcnCmdAck2 = TRUE;
            break;
        }
        case ASSOC_DATAREQ_ACK_STATUS_STEP:
        {
            if (status == PHY_SUCCESS)    // ack. received
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           "Received ack for "
                           "Data request.\n",
                           node->getNodeTime(), node->nodeId);
                }
                mac->taskP.mlme_associate_request_STEP++;
                strcpy(mac->taskP.mlme_associate_request_frFunc,
                       "IFSHandler");
                mac->trx_state_req = RX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   RX_ON);
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_RX);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', FALSE);

                mac->extractT =
                    Mac802_15_4SetTimer(
                                        node,
                                        mac,
                                        M802_15_4EXTRACTTIMER,
                                        aResponseWaitTime * SECOND
                                        / Phy802_15_4GetSymbolRate(
                                            node->phyData[interfaceIndex]),
                                        NULL);

            }
            else                // time out when waiting for ack.
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           "Timed out while "
                           "waiting for ack for Data request.",
                           node->getNodeTime(), node->nodeId);
                }
                mac->numBcnCmdRetry2++;
                if (mac->numBcnCmdRetry2 <= aMaxFrameRetries)
                {
                    if (DEBUG)
                    {
                        printf(" retrying...\n");
                    }
                    mac->taskP.mlme_associate_request_STEP
                        = ASSOC_DATAREQ_CSMA_STATUS_STEP;
                    strcpy(mac->taskP.mlme_associate_request_frFunc,
                           "csmacaCallBack");
                    mac->waitBcnCmdAck2 = FALSE;
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                }
                else
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                               ": Maximum "
                               " number of retries reached. Reporting failure"
                               " to upper layer\n",
                               node->getNodeTime(),
                               node->nodeId);
                    }
                    mac->taskP.mlme_associate_request_STEP = ASSOC_INIT_STEP;
                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    MESSAGE_Free(node, mac->txBcnCmd2);
                    mac->txBcnCmd2 = NULL;
                    // restore default values
                    mac->mpib.macPANId = M802_15_4_PANID;
                    mac->mpib.macCoordExtendedAddress =
                            M802_15_4_COORDEXTENDEDADDRESS;
                    mac->mpib.macCoordShortAddress =
                        M802_15_4_COORDSHORTADDRESS;
                    Sscs802_15_4MLME_ASSOCIATE_confirm(
                                    node,
                                    interfaceIndex,
                                    0,
                                    M802_15_4_NO_DATA);  // assume no DATA
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                    return;
                }
            }
            break;
        }
        case ASSOC_RESPONSE_STATUS_STEP:
        {
            mac->taskP.mlme_associate_request_STEP = ASSOC_INIT_STEP;
            if (status == PHY_SUCCESS)        // response received
            {
                if (mStatus == M802_15_4_SUCCESS)
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                                " : Successfully "
                                "associated with the coordinator %d.\n",
                                node->getNodeTime(), node->nodeId,
                                mac->mpib.macCoordExtendedAddress);
                    }
                   // mac->isSyncLoss = FALSE;
                }
                else
                {
                    // restore default values
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                               ": Unable to "
                               "associate with the coordinator %d."
                               "Status = %d\n",
                               node->getNodeTime(), node->nodeId,
                               mac->mpib.macCoordExtendedAddress, mStatus);
                    }
                    mac->mpib.macPANId = M802_15_4_PANID;
                    mac->mpib.macCoordExtendedAddress =
                            M802_15_4_COORDEXTENDEDADDRESS;
                    mac->mpib.macCoordShortAddress =
                        M802_15_4_COORDSHORTADDRESS;
                }

                // stop the timer
                // This helps in eliminating packets received by an rfd in
                // in a non-beacon mode.

                mac->mpib.macCoordShortAddress
                     = mac->mpib.macCoordExtendedAddress;

                if (mac->extractT)
                {
                    MESSAGE_CancelSelfMsg(node, mac->extractT);
                    mac->extractT = NULL;
                }
                Sscs802_15_4MLME_ASSOCIATE_confirm(node,
                                                   interfaceIndex,
                                                   mac->rt_myNodeID,
                                                   mStatus);
            }
            else                    // time out when waiting for response
            {
                // restore default values
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           " Timed out while "
                           "waiting for Data response for associate request"
                            " from the coordinator %d.\n",
                            node->getNodeTime(), node->nodeId,
                            mac->mpib.macCoordExtendedAddress);
                }
                mac->mpib.macPANId = M802_15_4_PANID;
                mac->mpib.macCoordExtendedAddress =
                        M802_15_4_COORDEXTENDEDADDRESS;
                mac->mpib.macCoordShortAddress =
                        M802_15_4_COORDSHORTADDRESS;
                Sscs802_15_4MLME_ASSOCIATE_confirm(node,
                                                   interfaceIndex,
                                                   0,
                                                   M802_15_4_NO_DATA);
            }
            Mac802_15_4CsmacaResume(node, interfaceIndex);
            Mac802_15_4CheckForOutgoingPacket(node, interfaceIndex);
            break;
        }
        default:
        {
            break;
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4mlme_associate_response
// LAYER      :: Mac
// PURPOSE    :: Initiate a response from SSCS sublayer
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex     : Int32           : Interface index
// + DeviceAddress      : MACADDR    : Address of device requesting assoc
// + AssocShortAddress  : UInt16        : Short address allocated by coord
// + status             : M802_15_4_enum: Status of association attempt
// + SecurityEnable     : BOOL          : Whether enabled security or not
// + frUpper            : BOOL          : Whether from upper layer
// + status             : PhyStatusType       : Phy status
// RETURN  :: None
// **/
static
void Mac802_15_4mlme_associate_response(Node* node,
                                        Int32 interfaceIndex,
                                        MACADDR DeviceAddress,
                                        UInt16 AssocShortAddress,
                                        M802_15_4_enum Status,
                                        BOOL SecurityEnable,
                                        BOOL frUpper,
                                        PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    Message* rspPkt = NULL;
    clocktype kpTime = 0;
    UInt8 step = 0;
    Int32 i = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (frUpper)
    {
        if (mac->taskP.mlme_associate_response_STEP) // overflow
        {
            Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            DeviceAddress,
                                            M802_15_4_TRANSACTION_OVERFLOW) ;
            return;
        }
        mac->taskP.mlme_associate_response_STEP = 0;
    }
    step = mac->taskP.mlme_associate_response_STEP;
    switch(step)
    {
        case 0:
        {
            // check if parameters valid or not
            if ((Status != M802_15_4_SUCCESS) &&
                 (Status != M802_15_4_PAN_AT_CAPACITY) &&
                 (Status != M802_15_4_PAN_ACCESS_DENIED))
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                            " Association "
                            "response invoked with invalid parameter.\n",
                            node->getNodeTime(), node->nodeId);
                }
                Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            DeviceAddress,
                                            M802_15_4_INVALID_PARAMETER);
                return;
            }
            mac->taskP.mlme_associate_response_STEP++;
            strcpy(mac->taskP.mlme_associate_response_frFunc,
                   "csmacaCallBack");
           mac->taskP.mlme_associate_response_DeviceAddress = DeviceAddress;

           // -- construct an association response command packet and put it
           // in the pending list ---

           Mac802_15_4ConstructPayload(node,
                                       interfaceIndex,
                                       &rspPkt,
                                       0x03,   // cmd
                                       0,
                                       0,
                                       0,
                                       0x02);  // Association response

            char* payld = ((char*) MESSAGE_ReturnPacket(rspPkt) +
                            sizeof(M802_15_4CommandFrame));

            *((UInt16*)payld) = AssocShortAddress;
            memcpy((payld + 2), &Status, sizeof(M802_15_4_enum));

            Mac802_15_4AddCommandHeader(node,
                                        interfaceIndex,
                                        rspPkt,
                                        0x03,
                                        M802_15_4DEFFRMCTRL_ADDRMODE64,
                                        mac->mpib.macPANId,
                                        DeviceAddress,
                                        M802_15_4DEFFRMCTRL_ADDRMODE64,
                                        mac->mpib.macPANId,
                                        mac->aExtendedAddress,
                                        SecurityEnable,
                                        FALSE,
                                        TRUE,
                                        0x02,
                                        0);

            kpTime = (2 * aResponseWaitTime * SECOND)
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

            i = Mac802_15_4ChkAddTransacLink(
                                        node,
                                        &mac->transacLink1,
                                        &mac->transacLink2,
                                        M802_15_4DEFFRMCTRL_ADDRMODE64,
                                        DeviceAddress,
                                        rspPkt,
                                        0,
                                        kpTime);
            if (i != 0) // overflow or failed
            {
                if (DEBUG)
                {
                     printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                            " Cannot enqueue "
                            " association response packet.\n",
                            node->getNodeTime(), node->nodeId);
                }
                mac->taskP.mlme_associate_response_STEP = 0;
                MESSAGE_Free(node, rspPkt);
                rspPkt = NULL;
                Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            DeviceAddress,
                                            M802_15_4_TRANSACTION_OVERFLOW);
                return;
            }
            mac->assoRspWaitT = Mac802_15_4SetTimer(
                                                node,
                                                mac,
                                                M802_15_4ASSORSPWAITTIMER,
                                                kpTime,
                                                NULL);
            mac->taskP.mlme_associate_response_pendPkt = rspPkt;
            break;
        }
        case 1:
        {
            if (status == PHY_SUCCESS)    // response packet transmitted and
                                          // ack. received
            {
                mac->taskP.mlme_associate_response_STEP = 0;
                mac->stats.numAssociateResSent++;
                if (mac->assoRspWaitT)
                {
                    MESSAGE_CancelSelfMsg(node, mac->assoRspWaitT);
                    mac->assoRspWaitT = NULL;
                }
                if (mac->txT)
                {
                    MESSAGE_CancelSelfMsg(node, mac->txT);
                    mac->txT = NULL;
                }
                if (DEBUG)
                {
                   printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                          " Successfully sent "
                          "response packet. Ack received.\n",
                          node->getNodeTime(), node->nodeId);
                }
                Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            DeviceAddress,
                                            M802_15_4_SUCCESS);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'c', TRUE);
            }
            else                // response packet transmission failed
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                            "Transmission of "
                            "response packet failed.\n",
                            node->getNodeTime(), node->nodeId);
                }
                mac->taskP.mlme_associate_response_STEP = 1;
                Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            DeviceAddress,
                                            Status);
                mac->txBcnCmd = NULL;
                mac->waitBcnCmdAck = FALSE;
            }
            break;
        }
        case 2:
        {
            if (mac->txT)
            {
                MESSAGE_CancelSelfMsg(node, mac->txT);
                mac->txT = NULL;
            }
            mac->taskP.mlme_associate_response_STEP = 0;
            i = Mac802_15_4UpdateTransacLinkByPktOrHandle(
                                node,
                                OPER_PURGE_TRANSAC,
                                &mac->transacLink1,
                                &mac->transacLink2,
                                mac->taskP.mlme_associate_response_pendPkt,
                                0);
            if (i == 0) // still pending
            {
                // delete the packet from the transaction list immediately
                Mac802_15_4UpdateTransacLinkByPktOrHandle(
                            node,
                            OPER_DELETE_TRANSAC,      // delete
                            &mac->transacLink1,
                            &mac->transacLink2,
                            mac->taskP.mlme_associate_response_pendPkt,
                            0);

                // Association response packet dropped from the MAC queue.
                // Update the packet drop statistic.

                mac->stats.numPktDropped++;

                // stop the CSMA-CA if it is running
                if (mac->backoffStatus == BACKOFF)
                {
                    mac->backoffStatus = BACKOFF_RESET;
                    Csma802_15_4Cancel(node, interfaceIndex);
                }
                mac->txBcnCmd = NULL;
                mac->waitBcnCmdAck = FALSE;
                Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            DeviceAddress,
                                            M802_15_4_TRANSACTION_EXPIRED);
                return;
            }
            else    // being successfully extracted
            {
                mac->stats.numAssociateResSent++;
                Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            DeviceAddress,
                                            M802_15_4_SUCCESS);
                return;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4mlme_disassociate_request
// LAYER      :: Mac
// PURPOSE    :: Request for disassociation from a PAN
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex     : Int32           : Interface index
// + DeviceAddress      : MACADDR    : Add of device requesting dis-assoc
// + DisassociateReason : UInt8         : Reason for disassociation
// + SecurityEnable     : BOOL          : Whether enabled security or not
// + frUpper            : BOOL          : Whether from upper layer
// + status             : PhyStatusType : Phy status
// RETURN  :: None
// **/
static
void Mac802_15_4mlme_disassociate_request(Node* node,
                                          Int32 interfaceIndex,
                                          MACADDR DeviceAddress,
                                          UInt8 DisassociateReason,
                                          BOOL SecurityEnable,
                                          BOOL frUpper,
                                          PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    M802_15_4Header* wph = NULL;
    clocktype kpTime = 0;
    UInt8 step = 0;
    Int32 i = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    step = mac->taskP.mlme_disassociate_request_STEP;
    switch(step)
    {
        case 0:
        {
            // check if parameters valid or not
            if (DeviceAddress != mac->mpib.macCoordExtendedAddress)
                        // send to a device
            {
                if ((!mac->capability.FFD) ||
                     (Mac802_15_4NumberDeviceLink(&mac->deviceLink1) == 0))
                                                    // I am not a coordinator
                {
                    Sscs802_15_4MLME_DISASSOCIATE_confirm(
                                                node,
                                                interfaceIndex,
                                                M802_15_4_INVALID_PARAMETER);
                    return;
                }
            }
            mac->taskP.mlme_disassociate_request_toCoor =
                    (DeviceAddress == mac->mpib.macCoordExtendedAddress);

            // --- construct a disassociation notification command packet ---
            ERROR_Assert(!mac->txBcnCmd2, "mac->txBcnCmd2 should be NULL");
            Mac802_15_4ConstructPayload(node,
                                        interfaceIndex,
                                        &mac->txBcnCmd2,
                                        0x03,   // cmd
                                        0,
                                        0,
                                        0,
                                        0x03);  // Disassociation notification

            wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd2);
            char* payld = ((char*) MESSAGE_ReturnPacket(mac->txBcnCmd2) +
                    sizeof(M802_15_4CommandFrame));
            if (!mac->taskP.mlme_disassociate_request_toCoor)
            {
                wph->MHR_DstAddrInfo.addr_64 = DeviceAddress;
            }
            else
            {
                wph->MHR_DstAddrInfo.addr_64 =
                        mac->mpib.macCoordExtendedAddress;
            }
            *((UInt8*)payld) =
                    (mac->taskP.mlme_disassociate_request_toCoor) ?
                        0x02 : 0x01;
            Mac802_15_4AddCommandHeader(node,
                                        interfaceIndex,
                                        mac->txBcnCmd2,
                                        0x03,
                                        M802_15_4DEFFRMCTRL_ADDRMODE64,
                                        mac->mpib.macPANId,
                                        wph->MHR_DstAddrInfo.addr_64,
                                        M802_15_4DEFFRMCTRL_ADDRMODE64,
                                        mac->mpib.macPANId,
                                        mac->aExtendedAddress,
                                        SecurityEnable,
                                        FALSE,
                                        TRUE,
                                        0x03,
                                        0);
            mac->taskP.mlme_disassociate_request_STEP++;
            strcpy(mac->taskP.mlme_disassociate_request_frFunc,
                   "csmacaCallBack");
            mac->taskP.mlme_disassociate_request_pendPkt = mac->txBcnCmd2;
            if (!mac->taskP.mlme_disassociate_request_toCoor)
                // indirect transmission should be used
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Prepared "
                            "disassociation notification for Node %d.\n",
                            node->getNodeTime(), node->nodeId,
                            wph->MHR_DstAddrInfo.addr_64);
                }

                clocktype tmpf = 0;
                tmpf = aBaseSuperframeDuration
                  * (1 <<  mac->mpib.macBeaconOrder) * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

                kpTime = mac->mpib.macTransactionPersistenceTime * tmpf;

                i = Mac802_15_4ChkAddTransacLink(
                                            node,
                                            &mac->transacLink1,
                                            &mac->transacLink2,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            wph->MHR_DstAddrInfo.addr_64,
                                            mac->txBcnCmd2,
                                            0,
                                            kpTime);
                if (i != 0) // overflow or failed
                {
                    mac->taskP.mlme_disassociate_request_STEP = 0;
                    MESSAGE_Free(node, mac->txBcnCmd2);
                    mac->txBcnCmd2 = NULL;
                    Sscs802_15_4MLME_DISASSOCIATE_confirm(
                            node,
                            interfaceIndex,
                            M802_15_4_TRANSACTION_OVERFLOW);
                    return;
                }
                mac->extractT = Mac802_15_4SetTimer(node,
                                                    mac,
                                                    M802_15_4EXTRACTTIMER,
                                                    (kpTime),
                                                    NULL);
            }
            else
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                            " Sending "
                            "disassociation notification to Coordinator"
                            "%d.\n",
                            node->getNodeTime(), node->nodeId,
                            wph->MHR_DstAddrInfo.addr_64);
                }
                Mac802_15_4CsmacaBegin(node, interfaceIndex, 'C');
            }
            break;
        }
        case 1:
        {
            if (!mac->taskP.mlme_disassociate_request_toCoor)
                        // indirect transmission
            {
                // check if the transaction still pending
                wph = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd2);

                mac->taskP.mlme_disassociate_request_STEP = 0;
                i = Mac802_15_4UpdateTransacLinkByPktOrHandle(
                        node,
                        OPER_PURGE_TRANSAC,
                        &mac->transacLink1,
                        &mac->transacLink2,
                        mac->taskP.mlme_disassociate_request_pendPkt,
                        0);
                if (i == 0) // still pending
                {
                    // delete the packet from the transaction list immediately
                    Mac802_15_4UpdateTransacLinkByPktOrHandle(
                            node,
                            OPER_DELETE_TRANSAC,      // delete
                            &mac->transacLink1,
                            &mac->transacLink2,
                            mac->taskP.mlme_disassociate_request_pendPkt,
                            0);

                    Sscs802_15_4MLME_DISASSOCIATE_confirm(
                                            node,
                                            interfaceIndex,
                                            M802_15_4_TRANSACTION_EXPIRED);

                    return;
                }
                else    // being successfully extracted
                {
                    Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            DeviceAddress,
                                            M802_15_4_SUCCESS);
                    return;
                }
            }
            else
            {
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4mlme_sync_request
// LAYER      :: Mac
// PURPOSE    :: Synchronize with the coordinator
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + LogicalChannel : UInt8 : Logical channel
// + TrackBeacon    : BOOL  : Whether to synchronize with all future beacons
// + frUpper        : BOOL          : Whether from upper layer
// + status         : PhyStatusType       : Phy status
// RETURN  :: None
// **/
static
void Mac802_15_4mlme_sync_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 LogicalChannel,
                                  BOOL TrackBeacon,
                                  BOOL frUpper,
                                  PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    UInt8 step = 0;
    UInt8 BO = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (frUpper)
    {
        if (mac->bcnRxT)
        {
            MESSAGE_CancelSelfMsg(node, mac->bcnRxT);
            mac->bcnRxT = NULL;
        }
        mac->taskP.mlme_sync_request_STEP = SYNC_INIT_STEP;
        (mac->taskP.mlme_sync_request_frFunc)[0] = 0;
    }

    step = mac->taskP.mlme_sync_request_STEP;
    switch(step)
    {
        case SYNC_INIT_STEP:
        {
            mac->taskP.mlme_sync_request_STEP++;
            strcpy(mac->taskP.mlme_sync_request_frFunc,
                   "recvBeacon");
            mac->taskP.mlme_sync_request_numSearchRetry = 0;
            mac->taskP.mlme_sync_request_tracking = TrackBeacon;

            // set current channel
            mac->tmp_ppib.phyCurrentChannel = LogicalChannel;
            Phy802_15_4PLME_SET_request(node,
                                        interfaceIndex,
                                        phyCurrentChannel,
                                        &mac->tmp_ppib);

            // enable the receiver
            mac->trx_state_req = RX_ON;
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               RX_ON);
            if (mac->macBeaconOrder2 == 15)
            {
                BO = 14;
            }
            else
            {
                BO = mac->macBeaconOrder2;
            }
            if (mac->bcnSearchT)
            {
                MESSAGE_CancelSelfMsg(node, mac->bcnSearchT);
                mac->bcnSearchT = NULL;
            }

            mac->bcnSearchT =
            Mac802_15_4SetTimer(
                node,
                mac,
                M802_15_4BEACONSEARCHTIMER,
                aBaseSuperframeDuration
                  * ((1 << BO)+ 1) * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                NULL);
            break;
        }
        case SYNC_BEACON_RCVD_STATUS_STEP:
        {
            if (status == PHY_SUCCESS)    // beacon received
            {
                // no confirm primitive for the success - it's better to have
                // one

                mac->taskP.mlme_sync_request_STEP = SYNC_INIT_STEP;
                if (mac->bcnRxT)
                {
                    MESSAGE_CancelSelfMsg(node, mac->bcnRxT);
                    mac->bcnRxT = NULL;
                }
                if (mac->bcnSearchT)
                {
                    MESSAGE_CancelSelfMsg(node, mac->bcnSearchT);
                    mac->bcnSearchT = NULL;
                }

                // If beacon is received & control reached here after an
                // orphan scan by the device, isSynLoss variable should be
                // set to false to enable the devie to dequeue data from
                // network layer

                if (mac->isSyncLoss)
                {
                    mac->isSyncLoss = FALSE;
                }

                // continue to track the beacon if required
                if (TrackBeacon)
                {
                    mac->taskP.mlme_sync_request_STEP
                        = SYNC_BEACON_RCVD_STATUS_STEP;
                    mac->taskP.mlme_sync_request_numSearchRetry = 0;
                    if (mac->bcnRxT == NULL)
                    {

                        if (DEBUG_TIMER)
                        {
                            printf("%" TYPES_64BITFMT "d : Node %d:"
                                  " 802.15.4MAC : Set Rx"
                                  " timer from mlme_sync_request function\n",
                                           node->getNodeTime(),
                                           node->nodeId);
                        }

                        mac->bcnRxT =
                              Mac802_15_4SetTimer(
                                   node,
                                   mac,
                                   M802_15_4BEACONRXTIMER,
                                   Mac802_15_4CalculateBcnRxTimer2(node,
                                                            interfaceIndex),
                                                            NULL);

                    }
                }
                Mac802_15_4CsmacaResume(node, interfaceIndex);
            }
            else                // time out when waiting for beacon
            {
                mac->taskP.mlme_sync_request_numSearchRetry++;
                if (mac->taskP.mlme_sync_request_numSearchRetry
                    <= aMaxLostBeacons)
                {
                    mac->trx_state_req = RX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       RX_ON);
                    if (mac->macBeaconOrder2 == 15)
                    {
                        BO = 14;
                    }
                    else
                    {
                        BO = mac->macBeaconOrder2;
                    }

                    mac->bcnSearchT =
                        Mac802_15_4SetTimer(
                            node,
                            mac,
                            M802_15_4BEACONSEARCHTIMER,
                            aBaseSuperframeDuration * ((1 << BO) + 1)
                            * SECOND
                            / Phy802_15_4GetSymbolRate(
                                    node->phyData[interfaceIndex]),
                           NULL);
                }
                else
                {
                    Sscs802_15_4MLME_SYNC_LOSS_indication(
                                                    node,
                                                    interfaceIndex,
                                                    M802_15_4_BEACON_LOSS);

                    /*If the initial beacon location fails, no need to track
                    the beacon even it is required
                    *Note that not tracking does not mean the device will
                    not be able to receive beacons --
                    *but the reception may be not so reliable since there is
                    no synchronization.
                     */

                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                    return;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

static
void Mac802_15_4mlme_orphan_response(Node* node,
                                     Int32 interfaceIndex,
                                     MACADDR OrphanAddress,
                                     UInt16 ShortAddress,
                                     BOOL AssociatedMember,
                                     BOOL SecurityEnable,
                                     BOOL frUpper,
                                     PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    UInt8 step = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    step = mac->taskP.mlme_orphan_response_STEP;

    switch(step)
    {
        case 0:
        {
            if (AssociatedMember)
            {
                // send a coordinator realignment command
                mac->taskP.mlme_orphan_response_STEP++;
                strcpy(mac->taskP.mlme_orphan_response_frFunc,
                        "csmacaCallBack");
                mac->taskP.mlme_orphan_response_OrphanAddress =
                    OrphanAddress;
                ERROR_Assert(!mac->txBcnCmd, "mac->txBcnCmd should be NULL");
                Mac802_15_4ConstructPayload(node,
                                            interfaceIndex,
                                            &mac->txBcnCmd,
                                            0x03,   // cmd
                                            0,
                                            0,
                                            0,
                                            0x08);  // realignment

                char* payld = MESSAGE_ReturnPacket(mac->txBcnCmd) +
                        sizeof (M802_15_4CommandFrame);
                *((UInt16 *)payld) = mac->mpib.macPANId;
                memcpy(payld + 2, &(mac->mpib.macShortAddress),
                    sizeof(UInt16));

                memset(payld + 4, 0, sizeof(UInt8));
                memcpy(payld + 5, &ShortAddress, sizeof(UInt16));

                Mac802_15_4AddCommandHeader(node,
                                            interfaceIndex,
                                            mac->txBcnCmd,
                                            0x03,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            0xffff,
                                            OrphanAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->mpib.macPANId,
                                            mac->aExtendedAddress,
                                            SecurityEnable,
                                            FALSE,
                                            TRUE,
                                            0x08,
                                            0);
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                            " : Sending "
                            "Realignment command to Node %d.\n",
                            node->getNodeTime(), node->nodeId, OrphanAddress);
                }
                 Mac802_15_4CsmacaBegin(node, interfaceIndex, 'c');
                
                 mac->orphanT =
               Mac802_15_4SetTimer(
                node,
                mac,
                M802_15_4ORPHANRSPTIMER,
                aResponseWaitTime * SECOND
                   / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                NULL);

            }
            break;
        }
        case 1:
        {
            mac->taskP.mlme_orphan_response_STEP = 0;
            if (mac->orphanT)
            {
                MESSAGE_CancelSelfMsg(node, mac->orphanT);
                mac->orphanT = NULL;
            }
            if (status == PHY_SUCCESS)    // response packet transmitted and
                                          // ack. received
            {
                Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            OrphanAddress,
                                            M802_15_4_SUCCESS);
                Mac802_15_4TaskSuccess(node, interfaceIndex, 'c', TRUE);
            }
            else                // response packet transmission failed
            {
                Sscs802_15_4MLME_COMM_STATUS_indication(
                                            node,
                                            interfaceIndex,
                                            mac->mpib.macPANId,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            mac->aExtendedAddress,
                                            M802_15_4DEFFRMCTRL_ADDRMODE64,
                                            OrphanAddress,
                                            M802_15_4_CHANNEL_ACCESS_FAILURE);
                if (mac->txBcnCmd)
                {
                MESSAGE_Free(node, mac->txBcnCmd);
                mac->txBcnCmd = NULL;
                }
                mac->waitBcnCmdAck = FALSE;
                Mac802_15_4CsmacaResume(node, interfaceIndex);
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

static
void Mac802_15_4mlme_poll_request(Node* node,
                                  Int32 interfaceIndex,
                                  UInt8 CoordAddrMode,
                                  UInt16 CoordPANId,
                                  MACADDR CoordAddress,
                                  BOOL SecurityEnable,
                                  BOOL autoRequest,
                                  BOOL firstTime,
                                  PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    M802_15_4FrameCtrl frmCtrl;
    UInt8 step = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    if (firstTime)
    {
        if (mac->backoffStatus == BACKOFF || mac->inTransmission 
            || mac->taskP.mcps_gts_data_request_STEP 
            || mac->taskP.mcps_data_request_STEP)
        {
            // do not process poll request at this moment. 
            // currently processing another request

            return;
        }
        if (mac->taskP.mlme_poll_request_STEP)
        {
            mac->taskP.mlme_poll_request_STEP = POLL_INIT_STEP;
        }
        else
        {
            (mac->taskP.mlme_poll_request_frFunc)[0] = 0;
        }
    }

    if (DEBUG)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Polling the"
               " coordinator for any pending data step : %d\n",
               node->getNodeTime(),
               node->nodeId,
               mac->taskP.mlme_poll_request_STEP);
    }
    step = mac->taskP.mlme_poll_request_STEP;
    switch(step)
    {
        case POLL_INIT_STEP:
        {
            if (mac->txBcnCmd2)
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           "Not Polling"
                           " the coordinator as txBcnCmd2 is not NULL\n",
                           node->getNodeTime(),
                           node->nodeId);
                }
                return;
            }
            // check if parameters valid or not
            if (((CoordAddrMode != M802_15_4DEFFRMCTRL_ADDRMODE16) &&
                  (CoordAddrMode != M802_15_4DEFFRMCTRL_ADDRMODE64)) ||
                  (CoordPANId == 0xffff))
            {
                if (!autoRequest)
                {
                    Sscs802_15_4MLME_POLL_confirm(
                                                node,
                                                interfaceIndex,
                                                M802_15_4_INVALID_PARAMETER);
                }
                return;
            }
            mac->taskP.mlme_poll_request_STEP++;
            strcpy(mac->taskP.mlme_poll_request_frFunc,
                    "csmacaCallBack");
            mac->taskP.mlme_poll_request_CoordAddrMode = CoordAddrMode;
            mac->taskP.mlme_poll_request_CoordPANId = CoordPANId;
            mac->taskP.mlme_poll_request_CoordAddress = CoordAddress;
            mac->taskP.mlme_poll_request_SecurityEnable = SecurityEnable;
            mac->taskP.mlme_poll_request_autoRequest = autoRequest;
            
            // -- send a data request command ---
            Mac802_15_4ConstructPayload(node,
                                        interfaceIndex,
                                        &mac->txBcnCmd2,
                                        0x03,   // cmd
                                        0,
                                        0,
                                        0,
                                        0x04);  // Data request

            Mac802_15_4AddCommandHeader(node,
                                        interfaceIndex,
                                        mac->txBcnCmd2,
                                        0x03,
                                        CoordAddrMode,
                                        mac->mpib.macPANId,
                                        mac->mpib.macCoordExtendedAddress,
                                        M802_15_4DEFFRMCTRL_ADDRMODE64,
                                        mac->mpib.macPANId,
                                        mac->aExtendedAddress,
                                        SecurityEnable,
                                        FALSE,
                                        TRUE,
                                        0x04,
                                        0);

            if ((mac->mpib.macShortAddress != 0xfffe) &&
                 (mac->mpib.macShortAddress != 0xffff))
            {
                ((M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd2))->
                        MHR_SrcAddrInfo.addr_16
                    = mac->mpib.macShortAddress;

                frmCtrl.frmCtrl =
                        ((M802_15_4Header*)
                            MESSAGE_ReturnPacket(mac->txBcnCmd2))->MHR_FrmCtrl;
                Mac802_15_4FrameCtrlParse(&frmCtrl);

                Mac802_15_4FrameCtrlSetSrcAddrMode(
                        &frmCtrl,
                        M802_15_4DEFFRMCTRL_ADDRMODE16);
                ((M802_15_4Header*)
                        MESSAGE_ReturnPacket(mac->txBcnCmd2))->MHR_FrmCtrl
                     = frmCtrl.frmCtrl;
            }

            Mac802_15_4CsmacaBegin(node, interfaceIndex, 'C');
            break;
        }
        case POLL_CSMA_STATUS_STEP:
        {
            if (status == PHY_IDLE)
            {
                // Update the stats only when CSMA issues go ahead
                mac->stats.numDataReqSent++;

                 mac->taskP.mlme_poll_request_STEP++;
                strcpy(mac->taskP.mlme_poll_request_frFunc,
                        "PD_DATA_confirm");
                mac->trx_state_req = TX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX);
                break;
            }
            else
            {
                mac->taskP.mlme_poll_request_STEP = POLL_INIT_STEP;
                if (!autoRequest)
                {
                    Sscs802_15_4MLME_POLL_confirm(
                                            node,
                                            interfaceIndex,
                                            M802_15_4_CHANNEL_ACCESS_FAILURE);
                }
                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4TaskFailed(node,
                                      interfaceIndex,
                                      'C',
                                      M802_15_4_CHANNEL_ACCESS_FAILURE,
                                      TRUE);
                return;
            }
            break;
        }
        case POLL_PKT_SENT_STEP:
        {
            mac->taskP.mlme_poll_request_STEP++;
            strcpy(mac->taskP.mlme_poll_request_frFunc,
                    "recvAck");

            // enable the receiver
            mac->trx_state_req = RX_ON;
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               RX_ON);

             mac->txT =
                Mac802_15_4SetTimer(
                node,
                mac,
                M802_15_4TXTIMER,
                mac->mpib.macAckWaitDuration * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                NULL);

            mac->waitBcnCmdAck2 = TRUE;
            break;
        }
        case POLL_ACK_STATUS_STEP:
        {
            if (status == PHY_SUCCESS)    // ack. received
            {
                if (!mac->taskP.mlme_poll_request_pending)
                {
                    mac->taskP.mlme_poll_request_STEP = POLL_INIT_STEP;
                    if (!autoRequest)
                    {
                        Sscs802_15_4MLME_POLL_confirm(node,
                                                      interfaceIndex,
                                                      M802_15_4_NO_DATA);
                    }
                    Mac802_15_4Reset_TRX(node, interfaceIndex);
                    Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', TRUE);
                    return;
                }
                else
                {
                    mac->taskP.mlme_poll_request_STEP++;
                    strcpy(mac->taskP.mlme_poll_request_frFunc,
                            "IFSHandler");
                    mac->trx_state_req = RX_ON;
                    Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                       interfaceIndex,
                                                       RX_ON);
                   Mac802_15_4TaskSuccess(node, interfaceIndex, 'C', FALSE);

                   mac->extractT =
                         Mac802_15_4SetTimer(
                              node,
                              mac,
                              M802_15_4EXTRACTTIMER,
                              aMaxFrameResponseTime * SECOND
                              / Phy802_15_4GetSymbolRate(
                                            node->phyData[interfaceIndex]),
                              NULL);
                }
            }
            else                // time out when waiting for ack.
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           " Ack not received"
                           " for Poll request\n",
                           node->getNodeTime(),
                           node->nodeId);
                }

                mac->numBcnCmdRetry2++;
                if (mac->numBcnCmdRetry2 <= aMaxFrameRetries)
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                               ": Retrying Poll request\n",
                               node->getNodeTime(),
                               node->nodeId);
                    }

                    mac->taskP.mlme_poll_request_STEP = POLL_CSMA_STATUS_STEP;
                    strcpy(mac->taskP.mlme_poll_request_frFunc,
                           "csmacaCallBack");
                    mac->waitBcnCmdAck2 = FALSE;
                    Mac802_15_4CsmacaResume(node, interfaceIndex);
                }
                else
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                               " : Poll request retries Exhausted\n",
                               node->getNodeTime(),
                               node->nodeId);
                    }

                    mac->taskP.mlme_poll_request_STEP = POLL_INIT_STEP;
                    Mac802_15_4Reset_TRX(node, interfaceIndex);

                    Mac802_15_4TaskFailed(node,
                                          interfaceIndex,
                                          'C',
                                          M802_15_4_NO_ACK,
                                          TRUE);
                    if (!autoRequest)
                    {
                        Sscs802_15_4MLME_POLL_confirm(node,
                                                      interfaceIndex,
                                                      M802_15_4_NO_ACK);
                    }
                    return;
                }
            }
            break;
        }
        case POLL_DATA_RCVD_STATUS_STEP:
        {
            mac->taskP.mlme_poll_request_STEP = POLL_INIT_STEP;
            if (status == PHY_SUCCESS)        // data received
            {

                // stop the timer
                if (mac->extractT)
                {
                    MESSAGE_CancelSelfMsg(node, mac->extractT);
                    mac->extractT = NULL;
                }
                if (!autoRequest)
                {
                    Sscs802_15_4MLME_POLL_confirm(node,
                                                  interfaceIndex,
                                                  M802_15_4_SUCCESS);
                }

                // another step is to issue DATA.indication() which has been
                // done in IFSHandler()
                // poll again to see if there are more packets pending -- note
                // that, for each poll request, more than one confirm could be
                // passed to upper layer

                Mac802_15_4mlme_poll_request(node,
                                             interfaceIndex,
                                             CoordAddrMode,
                                             CoordPANId,
                                             CoordAddress,
                                             SecurityEnable,
                                             autoRequest,
                                             TRUE,
                                             PHY_SUCCESS);
            }
            else                    // time out when waiting for response
            {
                if (!autoRequest)
                {
                    Sscs802_15_4MLME_POLL_confirm(
                            node,
                            interfaceIndex,
                            M802_15_4_NO_DATA);
                }
                Mac802_15_4Reset_TRX(node, interfaceIndex);
                Mac802_15_4CsmacaResume(node, interfaceIndex);
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

static
void Mac802_15_4mlme_reset_request(Node* node,
                                   Int32 interfaceIndex,
                                   BOOL SetDefaultPIB,
                                   BOOL frUpper,
                                   PhyStatusType status)
{
    MacData802_15_4* mac = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    switch(mac->taskP.mlme_reset_request_STEP)
    {
        case 0:
        {
            mac->taskP.mlme_reset_request_STEP++;
            strcpy(mac->taskP.mlme_reset_request_frFunc,
                   "PLME_SET_TRX_STATE_confirm");
            mac->taskP.mlme_reset_request_SetDefaultPIB = SetDefaultPIB;
            mac->trx_state_req = FORCE_TRX_OFF;
            mac->backoffStatus = BACKOFF_RESET;
            Csma802_15_4Cancel(node, interfaceIndex);
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               FORCE_TRX_OFF);
            Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                  interfaceIndex,
                                                  PHY_SUCCESS);
            break;
        }
        case 1:
        {
            mac->taskP.mlme_reset_request_STEP = 0;
            if (mac->txOverT)
            {
                MESSAGE_CancelSelfMsg(node, mac->txOverT);
                mac->txOverT = NULL;
            }
            if (mac->txT)
            {
                MESSAGE_CancelSelfMsg(node, mac->txT);
                mac->txT = NULL;
            }
            if (mac->extractT)
            {
                MESSAGE_CancelSelfMsg(node, mac->extractT);
                mac->extractT = NULL;
            }
            if (mac->assoRspWaitT)
            {
                MESSAGE_CancelSelfMsg(node, mac->assoRspWaitT);
                mac->assoRspWaitT = NULL;
            }
            if (mac->dataWaitT)
            {
                MESSAGE_CancelSelfMsg(node, mac->dataWaitT);
                mac->dataWaitT = NULL;
            }
            if (mac->rxEnableT)
            {
                MESSAGE_CancelSelfMsg(node, mac->rxEnableT);
                mac->rxEnableT = NULL;
            }
            if (mac->scanT)
            {
                MESSAGE_CancelSelfMsg(node, mac->scanT);
                mac->scanT = NULL;
            }
            if (mac->bcnTxT)
            {
                MESSAGE_CancelSelfMsg(node, mac->bcnTxT);
                mac->bcnTxT = NULL;
            }
            if (mac->bcnRxT)
            {
                MESSAGE_CancelSelfMsg(node, mac->bcnRxT);
                mac->bcnRxT = NULL;
            }
            if (mac->bcnSearchT)
            {
                MESSAGE_CancelSelfMsg(node, mac->bcnSearchT);
                mac->bcnSearchT = NULL;
            }
            if (mac->txCmdDataT)
            {
                MESSAGE_CancelSelfMsg(node, mac->txCmdDataT);
                mac->txCmdDataT = NULL;
            }
            if (mac->backoffBoundT)
            {
                MESSAGE_CancelSelfMsg(node, mac->backoffBoundT);
                mac->backoffBoundT = NULL;
            }
            if (mac->IFST)
            {
                MESSAGE_CancelSelfMsg(node, mac->IFST);
                mac->IFST = NULL;
            }
            if (mac->broadcastT)
            {
               MESSAGE_CancelSelfMsg(node,mac->broadcastT);
               mac->broadcastT = NULL;
            }
            mac->secuBeacon = FALSE;
            mac->beaconWaiting = FALSE;
            mac->txBeacon = NULL;
            mac->txAck = NULL;
            mac->txBcnCmd = NULL;
            mac->txBcnCmd2 = NULL;
            mac->txData = NULL;
            mac->rxData = NULL;
            mac->rxCmd = NULL;
            Mac802_15_4EmptyDeviceLink(&mac->deviceLink1, &mac->deviceLink2);
            Mac802_15_4EmptyTransacLink(node,
                                        &mac->transacLink1,
                                        &mac->transacLink2);
            if (SetDefaultPIB)
            {
                mac->mpib = MPIB;
            }
            if (status == PHY_SUCCESS)
            {
                Sscs802_15_4MLME_RESET_confirm(node,
                                               interfaceIndex,
                                               M802_15_4_SUCCESS);
            }
            else
            {
                Sscs802_15_4MLME_RESET_confirm(node,
                                               interfaceIndex,
                                               M802_15_4_DISABLE_TRX_FAILURE);
            }
            break;
        }
        default:
        {
            break;
        }
    }

}

static
void Mac802_15_4mlme_rx_enable_request(Node* node,
                                       Int32 interfaceIndex,
                                       BOOL DeferPermit,
                                       UInt32 RxOnTime,
                                       UInt32 RxOnDuration,
                                       BOOL frUpper,
                                       PhyStatusType status)
{
    MacData802_15_4* mac = NULL;
    UInt8 step;
    UInt32 t_CAP;
    clocktype cutTime;
    clocktype tmpf;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    step = mac->taskP.mlme_rx_enable_request_STEP;

    if (step == 0)
    {
        if (RxOnDuration == 0)
        {
            Sscs802_15_4MLME_RX_ENABLE_confirm(node,
                                               interfaceIndex,
                                               M802_15_4_SUCCESS);
            mac->trx_state_req = TRX_OFF;
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               TRX_OFF);
            return;
        }
    }

    if (mac->macBeaconOrder2 != 15)      // beacon enabled
    {
        switch(step)
        {
            case 0:
            {
                mac->taskP.mlme_rx_enable_request_RxOnTime = RxOnTime;
                mac->taskP.mlme_rx_enable_request_RxOnDuration
                    = RxOnDuration;
                if (RxOnTime + RxOnDuration >= mac->sfSpec2.BI)
                {
                    Sscs802_15_4MLME_RX_ENABLE_confirm(
                                                node,
                                                interfaceIndex,
                                                M802_15_4_INVALID_PARAMETER);
                    return;
                }
                t_CAP = (mac->sfSpec2.FinCAP + 1) * mac->sfSpec2.sd;

                tmpf = node->getNodeTime();

                if ((RxOnTime - aTurnaroundTime) > t_CAP)
                {
                    Sscs802_15_4MLME_RX_ENABLE_confirm(node,
                                                       interfaceIndex,
                                                       M802_15_4_OUT_OF_CAP);
                    return;
                }

                else if ((tmpf - mac->macBcnRxTime)
                        < ((RxOnTime - aTurnaroundTime)
                        / Phy802_15_4GetSymbolRate(
                                node->phyData[interfaceIndex])
                        * SECOND))
                {
                    // can proceed in current superframe
                    mac->taskP.mlme_rx_enable_request_STEP++;
                    // just fall through case 1
                }
                else if (DeferPermit)
                {
                    // need to defer until next superframe
                    mac->taskP.mlme_rx_enable_request_STEP++;
                    strcpy(mac->taskP.mlme_rx_enable_request_frFunc,
                           "recvBeacon");
                    break;
                }
                else
                {
                    Sscs802_15_4MLME_RX_ENABLE_confirm(node,
                                                       interfaceIndex,
                                                       M802_15_4_OUT_OF_CAP);
                    return;
                }
            }
            case 1:
            {
                mac->taskP.mlme_rx_enable_request_STEP++;
                strcpy(mac->taskP.mlme_rx_enable_request_frFunc,
                       "rxEnableHandler");
                {
                    clocktype tmpf2 = 0;
                    tmpf = mac->macBcnRxTime;
                    tmpf = node->getNodeTime() - tmpf;

                    tmpf2 = RxOnTime * SECOND
                       / Phy802_15_4GetSymbolRate(
                            node->phyData[interfaceIndex]);

                    tmpf = tmpf2 - tmpf;
                    mac->rxEnableT = Mac802_15_4SetTimer(node,
                                                         mac,
                                                         M802_15_4RXENABLETIMER,
                                                         tmpf,
                                                         NULL);
                }
                break;
            }
            case 2:
            {
                mac->taskP.mlme_rx_enable_request_STEP++;
                strcpy(mac->taskP.mlme_rx_enable_request_frFunc,
                        "PLME_SET_TRX_STATE_confirm");
                mac->taskP.mlme_rx_enable_request_currentTime
                    = node->getNodeTime();
                mac->trx_state_req = RX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   RX_ON);
                break;
            }
            case 3:
            {
                strcpy(mac->taskP.mlme_rx_enable_request_frFunc,
                       "rxEnableHandler");
                mac->taskP.mlme_rx_enable_request_STEP++;
                if (status == PHY_BUSY_TX)
                {
                    Sscs802_15_4MLME_RX_ENABLE_confirm(node,
                                                       interfaceIndex,
                                                       M802_15_4_TX_ACTIVE);
                }
                else
                {
                    Sscs802_15_4MLME_RX_ENABLE_confirm(node,
                                                       interfaceIndex,
                                                       M802_15_4_SUCCESS);
                }

                // turn off the receiver before the CFP so as not to disturb
                // it, and we see no reason to turn it on again after the CFP
                // (i.e., inactive port of the superframe)

                t_CAP = (mac->sfSpec2.FinCAP + 1) * mac->sfSpec2.sd;

                cutTime = (RxOnTime + RxOnDuration - t_CAP) * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
                tmpf = RxOnDuration * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

                tmpf -= node->getNodeTime();
                tmpf += mac->taskP.mlme_rx_enable_request_currentTime;
                tmpf -= cutTime;
                mac->rxEnableT = Mac802_15_4SetTimer(node,
                                                     mac,
                                                     M802_15_4RXENABLETIMER,
                                                     tmpf,
                                                     NULL);
                break;
            }
            case 4:
            {
                mac->taskP.mlme_rx_enable_request_STEP = 0;
                strcpy(mac->taskP.mlme_rx_enable_request_frFunc, "");
                mac->trx_state_req = TRX_OFF;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TRX_OFF);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        switch(step)
        {
            case 0:
            {
                strcpy(mac->taskP.mlme_rx_enable_request_frFunc,
                            "PLME_SET_TRX_STATE_confirm");
                mac->taskP.mlme_rx_enable_request_STEP++;
                mac->taskP.mlme_rx_enable_request_RxOnDuration
                    = RxOnDuration;
                mac->taskP.mlme_rx_enable_request_currentTime
                    = node->getNodeTime();
                mac->trx_state_req = RX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   RX_ON);
                break;
            }
            case 1:
            {
                mac->taskP.mlme_rx_enable_request_STEP++;
                strcpy(mac->taskP.mlme_rx_enable_request_frFunc,
                        "rxEnableHandler");
                if (status == PHY_BUSY_TX)
                {
                    Sscs802_15_4MLME_RX_ENABLE_confirm(node,
                                                       interfaceIndex,
                                                       M802_15_4_TX_ACTIVE);
                }
                else
                {
                    Sscs802_15_4MLME_RX_ENABLE_confirm(node,
                                                       interfaceIndex,
                                                       M802_15_4_SUCCESS);
                }

                tmpf = RxOnDuration * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);

                tmpf -= node->getNodeTime();
                tmpf += mac->taskP.mlme_rx_enable_request_currentTime;
                mac->rxEnableT = Mac802_15_4SetTimer(node,
                                                     mac,
                                                     M802_15_4RXENABLETIMER,
                                                     tmpf,
                                                     NULL);
                break;
            }
            case 2:
            {
                strcpy(mac->taskP.mlme_rx_enable_request_frFunc, "");
                mac->trx_state_req = TRX_OFF;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TRX_OFF);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

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
                                  Int32 interfaceIndex,
                                  UInt8 SrcAddrMode,
                                  UInt16 SrcPANId,
                                  MACADDR SrcAddr,
                                  UInt8 DstAddrMode,
                                  UInt16 DstPANId,
                                  MACADDR DstAddr,
                                  Int32 msduLength,
                                  Message* msdu,
                                  UInt8 msduHandle,
                                  UInt8 TxOptions)
{
    Mac802_15_4mcps_data_request(node,
                                 interfaceIndex,
                                 SrcAddrMode,
                                 SrcPANId,
                                 SrcAddr,
                                 DstAddrMode,
                                 DstPANId,
                                 DstAddr,
                                 msduLength,
                                 msdu,
                                 msduHandle,
                                 TxOptions,
                                 TRUE,
                                 PHY_SUCCESS,
                                 M802_15_4_SUCCESS);
}

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
                                     UInt8 ACLEntry)
{
    UInt8 msduHandle = ((M802_15_4Header*)
                            MESSAGE_ReturnPacket(msdu))->msduHandle;

    MESSAGE_RemoveHeader(node,
                         msdu,
                         sizeof(M802_15_4Header),
                         TRACE_MAC_802_15_4);
    if (msduHandle != 0)
    {
        Sscs802_15_4MCPS_DATA_indication(node,
                                         interfaceIndex,
                                         SrcAddrMode,
                                         SrcPANId,
                                         SrcAddr,
                                         DstAddrMode,
                                         DstPANId,
                                         DstAddr,
                                         msduLength,
                                         msdu,
                                         mpduLinkQuality,
                                         SecurityUse,
                                         ACLEntry);
    }
    else
    {
        MacHWAddress lastHopHWAddr;
        ConvertMacAddrToVariableHWAddress(node,&lastHopHWAddr,&SrcAddr);
        MAC_HandOffSuccessfullyReceivedPacket(node,
                                              interfaceIndex,
                                              msdu,
                                              &lastHopHWAddr);
    }
}

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
                                   UInt8 msduHandle)
{
    // This function not yet implemented
}

// /**
// FUNCTION   :: Mac802_15_4MLME_ASSOCIATE_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request an association with a coordinator
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + logicalChannel : UInt8         : Channel on which attempt is to be done
// + coordAddrMode  : UInt8         : Coordinator address mode
// + coordPANId     : UInt16        : Coordinator PAN id
// + coordAddress   : MACADDR    : Coordinator address
// + CapabilityInformation : UInt8  : capabilities of associating device
// + securityEnable : BOOL          : Whether enable security or not
// RETURN  :: None
// **/
void Mac802_15_4MLME_ASSOCIATE_request(Node* node,
                                       Int32 interfaceIndex,
                                       UInt8 logicalChannel,
                                       UInt8 coordAddrMode,
                                       UInt16 coordPANId,
                                       MACADDR coordAddress,
                                       UInt8 capabilityInformation,
                                       BOOL securityEnable)
{
    MacData802_15_4* mac = (MacData802_15_4*)
                                node->macData[interfaceIndex]->macVar;
    Mac802_15_4mlme_associate_request(node,
                                      interfaceIndex,
                                      logicalChannel,
                                      coordAddrMode,
                                      coordPANId,
                                      coordAddress,
                                      capabilityInformation,
                                      securityEnable,
                                      TRUE,
                                      PHY_SUCCESS,
                                      M802_15_4_SUCCESS);
    mac->stats.numAssociateReqSent++;
}

// /**
// FUNCTION   :: Mac802_15_4MLME_ASSOCIATE_response
// LAYER      :: Mac
// PURPOSE    :: Primitive to initiate a response from SSCS sublayer
// PARAMETERS ::
// + node               : Node*         : Node receiving call
// + interfaceIndex     : Int32           : Interface index
// + DeviceAddress      : MACADDR    : Address of device requesting assoc
// + AssocShortAddress  : UInt16        : Short address allocated by coord
// + status             : M802_15_4_enum: Status of association attempt
// + securityEnable     : BOOL          : Whether enabled security or not
// RETURN  :: None
// **/
void Mac802_15_4MLME_ASSOCIATE_response(Node* node,
                                        Int32 interfaceIndex,
                                        MACADDR DeviceAddress,
                                        UInt16 AssocShortAddress,
                                        M802_15_4_enum status,
                                        BOOL securityEnable)
{
    Mac802_15_4mlme_associate_response(node,
                                       interfaceIndex,
                                       DeviceAddress,
                                       AssocShortAddress,
                                       status,
                                       securityEnable,
                                       TRUE,
                                       PHY_SUCCESS);
}

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
                                          BOOL SecurityEnable)
{
    MacData802_15_4* mac = (MacData802_15_4*)
                                node->macData[interfaceIndex]->macVar;
    Mac802_15_4mlme_disassociate_request(node,
                                         interfaceIndex,
                                         DeviceAddress,
                                         DisassociateReason,
                                         SecurityEnable,
                                         TRUE,
                                         PHY_SUCCESS);
    mac->stats.numDisassociateReqSent++;
}

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
                                             UInt8 ACLEntry)
{
    // This function not yet implemented
}

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
                                 M802_15_4_PIBA_enum PIBAttribute)
{
    // This function not yet implemented
}

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
                                 M802_15_4_enum status)
{
    // This function not yet implemented
}

// /**
// FUNCTION   :: Mac802_15_4MLME_GTS_indication
// LAYER      :: Mac
// PURPOSE    ::Primitive to indicates that a GTS has been allocated or that
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
                                    UInt8 ACLEntry)
{
    // This function not yet implemented
}

// /**
// FUNCTION   :: Mac802_15_4MLME_ORPHAN_response
// LAYER      :: Mac
// PURPOSE    ::Primitive to respond to the MLME-ORPHAN.indication primitive
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
                                     BOOL SecurityEnable)
{
    Mac802_15_4mlme_orphan_response(node,
                                    interfaceIndex,
                                    OrphanAddress,
                                    ShortAddress,
                                    AssociatedMember,
                                    SecurityEnable,
                                    TRUE,
                                    PHY_SUCCESS);
}

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
                                   BOOL SetDefaultPIB)
{
    Mac802_15_4mlme_reset_request(node,
                                  interfaceIndex,
                                  SetDefaultPIB,
                                  TRUE,
                                  PHY_SUCCESS);
}

// /**
// FUNCTION   :: Mac802_15_4MLME_RX_ENABLE_request
// LAYER      :: Mac
// PURPOSE    ::Primitive to request that the receiver is either enabled for
//               a finite period of time or disabled
// PARAMETERS ::
// + node           : Node*     : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + deferPermit    : BOOL      : If defer till next superframe permitted
// + rxOnTime       : UInt32    : No. of symbols from start of superframe
//                                after which receiver is enabled
// + rxOnDuration   : UInt32    : No. of symbols for which receiver is to be
//                                enabled
// RETURN  :: None
// **/
void Mac802_15_4MLME_RX_ENABLE_request(Node* node,
                                       Int32 interfaceIndex,
                                       BOOL deferPermit,
                                       UInt32 rxOnTime,
                                       UInt32 rxOnDuration)
{
    Mac802_15_4mlme_rx_enable_request(node,
                                      interfaceIndex,
                                      deferPermit,
                                      rxOnTime,
                                      rxOnDuration,
                                      TRUE,
                                      PHY_SUCCESS);
}

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
                                  UInt8 ScanDuration)
{
    MacData802_15_4* mac = NULL;
    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    if (!mac->taskP.mlme_scan_request_STEP &&
        !mac->taskP.mlme_associate_request_STEP)
        // only if not currently scanning and associating
    {
        if (mac->txBcnCmd2)
        {
            MESSAGE_Free(node, mac->txBcnCmd2);
            mac->txBcnCmd2 = NULL;
        }
        Mac802_15_4mlme_scan_request(node,
                                     interfaceIndex,
                                     ScanType,
                                     ScanChannels,
                                     ScanDuration,
                                     TRUE,
                                     PHY_SUCCESS);
    }
}

// /**
// FUNCTION   :: Mac802_15_4MLME_SET_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to set PIB attribute
// PARAMETERS ::
// + node               : Node*                 : Node receiving call
// + interfaceIndex     : Int32                   : Interface index
// + PIBAttribute       : M802_15_4_PIBA_enum   : PIB attribute id
// + PIBAttributeValue  : M802_15_4PIB*         : Attribute value
// RETURN  :: None
// **/
void Mac802_15_4MLME_SET_request(Node* node,
                                 Int32 interfaceIndex,
                                 M802_15_4_PIBA_enum PIBAttribute,
                                 M802_15_4PIB* PIBAttributeValue)
{
    MacData802_15_4* mac802_15_4 = NULL;
    PLMEsetTrxState p_state;
    M802_15_4_enum t_status;

    mac802_15_4 = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    t_status = M802_15_4_SUCCESS;
    switch(PIBAttribute)
    {
        case macAckWaitDuration:
        {
            Phy802_15_4PLME_GET_request(
                    node,
                    interfaceIndex,
                    phyCurrentChannel);   // value will be
                                        // returned in tmp_ppib
            if (((mac802_15_4->tmp_ppib.phyCurrentChannel <= 10)
                    && (PIBAttributeValue->macAckWaitDuration != 120))
                    || ((mac802_15_4->tmp_ppib.phyCurrentChannel > 10)
                    && (PIBAttributeValue->macAckWaitDuration != 54)))
            {
                t_status = M802_15_4_INVALID_PARAMETER;
            }
            else
            {
                mac802_15_4->mpib.macAckWaitDuration
                    = PIBAttributeValue->macAckWaitDuration;
            }
            break;
        }
        case macAssociationPermit:
        {
            mac802_15_4->mpib.macAssociationPermit
                = PIBAttributeValue->macAssociationPermit;
            break;
        }
        case macAutoRequest:
        {
            mac802_15_4->mpib.macAutoRequest
                = PIBAttributeValue->macAutoRequest;
            break;
        }
        case macBattLifeExt:
        {
            mac802_15_4->mpib.macBattLifeExt
                = PIBAttributeValue->macBattLifeExt;
            break;
        }
        case macBattLifeExtPeriods:
        {
            Phy802_15_4PLME_GET_request(
                    node,
                    interfaceIndex,
                    phyCurrentChannel);     // value will be
                                            // returned in tmp_ppib
            if (((mac802_15_4->tmp_ppib.phyCurrentChannel <= 10)
                    && (PIBAttributeValue->macBattLifeExtPeriods != 8))
                    || ((mac802_15_4->tmp_ppib.phyCurrentChannel > 10)
                    && (PIBAttributeValue->macBattLifeExtPeriods != 6)))
            {
                t_status = M802_15_4_INVALID_PARAMETER;
            }
            else
            {
                mac802_15_4->mpib.macBattLifeExtPeriods
                    = PIBAttributeValue->macBattLifeExtPeriods;
            }
            break;
        }
        case macBeaconPayload:
        {
                // <macBeaconPayloadLength> should be set first
            memcpy(mac802_15_4->mpib.macBeaconPayload,
                   PIBAttributeValue->macBeaconPayload,
                   mac802_15_4->mpib.macBeaconPayloadLength);
            break;
        }
        case macBeaconPayloadLength:
        {
            if (PIBAttributeValue->macBeaconPayloadLength >
                aMaxBeaconPayloadLength)
            {
                t_status = M802_15_4_INVALID_PARAMETER;
            }
            else
            {
                mac802_15_4->mpib.macBeaconPayloadLength
                    = PIBAttributeValue->macBeaconPayloadLength;
            }
            break;
        }
        case macBeaconOrder:
        {
            if (PIBAttributeValue->macBeaconOrder > 15)
            {
                t_status = M802_15_4_INVALID_PARAMETER;
            }
            else
            {
                mac802_15_4->mpib.macBeaconOrder
                    = PIBAttributeValue->macBeaconOrder;
            }
            break;
        }
        case macBeaconTxTime:
        {
            mac802_15_4->mpib.macBeaconTxTime
                = PIBAttributeValue->macBeaconTxTime;
            break;
        }
        case macBSN:
        {
            mac802_15_4->mpib.macBSN = PIBAttributeValue->macBSN;
            break;
        }
        case macCoordExtendedAddress:
        {
            mac802_15_4->mpib.macCoordExtendedAddress
                = PIBAttributeValue->macCoordExtendedAddress;
            break;
        }
        case macCoordShortAddress:
        {
            mac802_15_4->mpib.macCoordShortAddress
                = PIBAttributeValue->macCoordShortAddress;
            break;
        }
        case macDSN:
        {
            mac802_15_4->mpib.macDSN = PIBAttributeValue->macDSN;
            break;
        }
        case macGTSPermit:
        {
            mac802_15_4->mpib.macGTSPermit = PIBAttributeValue->macGTSPermit;
            break;
        }
        case macMaxCSMABackoffs:
        {
            if (PIBAttributeValue->macMaxCSMABackoffs > 5)
            {
                t_status = M802_15_4_INVALID_PARAMETER;
            }
            else
            {
                mac802_15_4->mpib.macMaxCSMABackoffs
                    = PIBAttributeValue->macMaxCSMABackoffs;
            }
            break;
        }
        case macMinBE:
        {
            if (PIBAttributeValue->macMinBE > 3)
            {
                t_status = M802_15_4_INVALID_PARAMETER;
            }
            else
            {
                mac802_15_4->mpib.macMinBE = PIBAttributeValue->macMinBE;
            }
            break;
        }
        case macPANId:
        {
            mac802_15_4->mpib.macPANId = PIBAttributeValue->macPANId;
            break;
        }
        case macPromiscuousMode:
        {
            mac802_15_4->mpib.macPromiscuousMode
                = PIBAttributeValue->macPromiscuousMode;
            mac802_15_4->mpib.macRxOnWhenIdle
                = PIBAttributeValue->macPromiscuousMode;
            if (mac802_15_4->mpib.macRxOnWhenIdle == TRUE)
            {
                p_state = RX_ON;
            }
            else
            {
                p_state = TRX_OFF;
            }
            Phy802_15_4PlmeSetTRX_StateRequest(
                    node,
                    interfaceIndex,
                    p_state);
            break;
        }
        case macRxOnWhenIdle:
        {
            mac802_15_4->mpib.macRxOnWhenIdle
                = PIBAttributeValue->macRxOnWhenIdle;
            break;
        }
        case macShortAddress:
        {
            mac802_15_4->mpib.macShortAddress
                = PIBAttributeValue->macShortAddress;
            break;
        }
        case macSuperframeOrder:
        {
            if (PIBAttributeValue->macSuperframeOrder > 15)
            {
                t_status = M802_15_4_INVALID_PARAMETER;
            }
            else
            {
                mac802_15_4->mpib.macSuperframeOrder =
                        PIBAttributeValue->macSuperframeOrder;
            }
            break;
        }
        case macTransactionPersistenceTime:
        {
            mac802_15_4->mpib.macTransactionPersistenceTime
                = PIBAttributeValue->macTransactionPersistenceTime;
            break;
        }
        case macGtsTriggerPrecedence:
        {
            mac802_15_4->mpib.macGtsTriggerPrecedence
                = PIBAttributeValue->macGtsTriggerPrecedence;
            break;
        }
        case macDataAcks:
        {
             mac802_15_4->mpib.macDataAcks = PIBAttributeValue->macDataAcks;
            break;
        }
        case macACLEntryDescriptorSet:
        case macACLEntryDescriptorSetSize:
        case macDefaultSecurity:
        case macACLDefaultSecurityMaterialLength:
        case macDefaultSecurityMaterial:
        case macDefaultSecuritySuite:
        case macSecurityMode:
        {
            break;      // currently security ignored in simulation
        }
        default:
        {
            t_status = M802_15_4_UNSUPPORTED_ATTRIBUTE;
            break;
        }
    }
    Sscs802_15_4MLME_SET_confirm(node,
                                 interfaceIndex,
                                 t_status,
                                 PIBAttribute);
}

// /**
// FUNCTION   :: Mac802_15_4MLME_START_request
// LAYER      :: Mac
// PURPOSE    ::Primitive to allow the PAN coordinator to initiate a new PAN
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
                                   BOOL SecurityEnable)
{
    Mac802_15_4mlme_start_request(node,
                                  interfaceIndex,
                                  PANId,
                                  LogicalChannel,
                                  BeaconOrder,
                                  SuperframeOrder,
                                  PANCoordinator,
                                  BatteryLifeExtension,
                                  CoordRealignment,
                                  SecurityEnable,
                                  TRUE,
                                  PHY_SUCCESS);
}

// /**
// FUNCTION   :: Mac802_15_4MLME_SYNC_request
// LAYER      :: Mac
// PURPOSE    :: Primitive to request synchronize with the coordinator
// PARAMETERS ::
// + node           : Node*         : Node receiving call
// + interfaceIndex : Int32           : Interface index
// + LogicalChannel : UInt8         : Logical channel
// + TrackBeacon    : BOOL   :Whether to synchronize with all future beacons
// RETURN  :: None
// **/
void Mac802_15_4MLME_SYNC_request(
        Node* node,
        Int32 interfaceIndex,
        UInt8 LogicalChannel,
        BOOL TrackBeacon)
{
    Mac802_15_4mlme_sync_request(node,
                                 interfaceIndex,
                                 LogicalChannel,
                                 TrackBeacon,
                                 TRUE,
                                 PHY_SUCCESS);
}

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
void Mac802_15_4MLME_POLL_request(
        Node* node,
        Int32 interfaceIndex,
        UInt8 CoordAddrMode,
        UInt16 CoordPANId,
        MACADDR CoordAddress,
        BOOL SecurityEnable)
{
    Mac802_15_4mlme_poll_request(node,
                                 interfaceIndex,
                                 CoordAddrMode,
                                 CoordPANId,
                                 CoordAddress,
                                 SecurityEnable,
                                 FALSE,
                                 TRUE,
                                 PHY_SUCCESS);
}

// /**
// NAME         ::Mac802_15_4HandleGtsChracteristics
// PURPOSE      ::Updates GTS chracteristics obtained in a beacon 
//                from panccoord
// PARAMETERS   
// + node : Node*               : Node which sends the message
// + mac : MacData_802_15_4*    : 802.15.4 data structure
// RETURN       ::None
// NOTES        ::None
// **/
static
void Mac802_15_4HandleGtsChracteristics(Node* node,
                                        MacData802_15_4* mac)
{
    // Parse and populate GTS characteristics for valid beacons only
    Mac802_15_4GTSSpecParse(&mac->gtsSpec3);
    mac->gtsSpec2.permit = mac->gtsSpec3.permit;
    if (mac->gtsSpec2.permit)
    {
        mac->displayGtsStats = TRUE;
    }
    mac->panDes2.GTSPermit = mac->gtsSpec2.permit;
    if (mac->sendGtsConfirmationPending)
    {
        ERROR_Assert(mac->taskP.gtsChracteristics, "gtsChracteristics should"
                     " be present");
        GtsRequestData tempData;
        Mac802_15_4ParseGtsRequestChracteristics(
                                        mac->taskP.gtsChracteristics,
                                        &tempData);
        BOOL validGtsDescriptorFound = FALSE;
        Int32 i = 0;
        for (i = 0; i < mac->gtsSpec3.count; i++)
        {
            if (mac->aExtendedAddress
                    == (((mac->gtsSpec3).fields).list[i]).devAddr16)
            {
                if ((mac->gtsSpec3).recvOnly[i] == tempData.receiveOnly)
                {
                    // valid gts found in the beacon
                    validGtsDescriptorFound = TRUE; 
                    if ((mac->gtsSpec3).slotStart[i] !=0)
                    {
                        ERROR_Assert(tempData.allocationRequest, "Should be"
                            " an alloation request");
                        BOOL wasFound = FALSE;
                        Int32 r = 0;
                        for (r =0; r < mac->gtsSpec2.count; r++)
                        {
                            if (mac->gtsSpec2.recvOnly[r]
                                    == (mac->gtsSpec3).recvOnly[i]
                                && mac->gtsSpec2.length[r] 
                                    == (mac->gtsSpec3).length[i])
                            {
                                mac->gtsSpec2.slotStart[r]
                                    = (mac->gtsSpec3).slotStart[i];
                                wasFound = TRUE;
                                break;
                            }
                        }
                        if (wasFound)
                        {
                            // GTS allocation confirmation received but we
                            // already have a valid GTS descriptor.
                            // Update the starting slot &
                            // break out of outer for loop
                            break;
                        }

                        // GTS Allocation confirmation received
                        if (tempData.allocationRequest)
                        {
                            // update the statistic
                            mac->stats.numGtsAllocationConfirmedByPanCoord++;
                        }

                        // Reset the GTS Request parameters
                        mac->sendGtsConfirmationPending = FALSE;
                        mac->sendGtsTrackCount = 0;
                        mac->taskP.gtsChracteristics = 0;
        
                        // Save the GTS descriptor in gtsSpec2 data structure
              (((mac->gtsSpec2).fields).list[mac->gtsSpec2.count]).devAddr16 
                                        = mac->mpib.macCoordShortAddress;
                        Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec2,
                                              mac->gtsSpec2.count,
                                              (mac->gtsSpec3).slotStart[i]);
                        Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec2,
                                                 mac->gtsSpec2.count,
                                                 mac->gtsSpec3.recvOnly[i]);
                        Mac802_15_4GTSSpecSetLength(&mac->gtsSpec2,
                                                  mac->gtsSpec2.count,
                                                  mac->gtsSpec3.length[i]);
                        Mac802_15_4GTSSpecSetCount(&mac->gtsSpec2,
                                                   mac->gtsSpec2.count + 1);

                        // Call GTS confirm with success
                        // Since Upper layers are not implemented. There is
                        // no real need to call GTS confirm
                    }
                    else
                    {
                        // gts request rejected by the pancoord
                        // or deallocation confirmation from pancoord
                        mac->sendGtsConfirmationPending = FALSE;
                        
                        BOOL gtsDescriptorPresent = FALSE;
                        Int32 j = 0;
                        for (j = 0; j < mac->gtsSpec2.count; j++)
                        {
                            if (mac->gtsSpec2.recvOnly[j]
                                    == mac->gtsSpec3.recvOnly[i])
                            {
                                // should be a de-allocate request confirmation
                                ERROR_Assert(!tempData.allocationRequest,
                                    "Should be an de-allocation request");
                                gtsDescriptorPresent = TRUE;
                                ERROR_Assert(mac->gtsSpec2.length[j]
                                              == mac->gtsSpec3.length[i],
                                        " GTS length in the GTS descriptor"
                                        " should match with the one saved"
                                        " at the devide");

                               // pancoord has deallocated this GTS
                               // remove this descriptor
                               // Remove the saved GTS descriptor

                               Mac802_15_4RemoveGtsDescriptorFromDevice(
                                                 node,
                                                 mac,
                                                 j,
                                                 TRUE);
                            }
                        }// for (j = 0; j < mac->gtsSpec2.count; j++)

                        // take action for not sending gts request again
                        // call gts confirmation with REJECTED

                        if (!gtsDescriptorPresent)
                        {
                            // should be gts allocation rejection by pancoord
                            ERROR_Assert(tempData.allocationRequest, "Should"
                                " be an allocation request");
                            mac->stats.numGtsRequestsRejectedByPanCoord++;
                            mac->gtsRequestExhausted = TRUE;
                        }
                        mac->taskP.gtsChracteristics = 0;
                    }
                }
            }
        }// for
        if (!validGtsDescriptorFound)
        {
            mac->sendGtsTrackCount++;
            if (mac->sendGtsTrackCount > aGTSDescPersistenceTime)
            {
               // no valid gts descriptor found till aGTSDescPersistenceTime 
               // superframes
               // call gts confirmation with NO_DATA

               mac->sendGtsTrackCount = 0;
               mac->sendGtsConfirmationPending = FALSE;
               mac->taskP.gtsChracteristics = 0;
               mac->gtsRequestExhausted = TRUE;
            }
        }// if (!validGtsDescriptorFound)
    }// 
    else
    {
        ERROR_Assert(!mac->taskP.gtsChracteristics
                        || (mac->taskP.gtsChracteristics 
                        && mac->taskP.mlme_gts_request_STEP), "Invalid State");
        Int32 i = 0;
        for (i = 0; i < mac->gtsSpec3.count; i++)
        {
            if (mac->aExtendedAddress
                == (((mac->gtsSpec3).fields).list[i]).devAddr16)
            {
                Int32 j = 0;
                for (j = 0; j < mac->gtsSpec2.count; j++)
                {
                    if (mac->gtsSpec2.recvOnly[j] == mac->gtsSpec3.recvOnly[i])
                    {
                        ERROR_Assert(mac->gtsSpec2.length[j]
                                        == mac->gtsSpec3.length[i], "GTS"
                                            " length saved at the device"
                                            " should be same as that"
                                            " obtained in the beacon");
                        if (mac->gtsSpec3.slotStart[i] == 0)
                        {
                            // pancoord has deallocated this GTS
                            // probably expired or beacons werer missed
                            // remove the saved descriptor anyway
                            // reset the gts descriptor

                            ERROR_Assert(mac->gtsSpec2.count <=2, "GTS count"
                                " should be less or equal to two");
                            Mac802_15_4RemoveGtsDescriptorFromDevice
                                                (node,
                                                 mac,
                                                 j,
                                                 FALSE);
                        }// if (mac->gtsSpec3.slotStart[i] == 0)
                        else
                        {
                            // gts maintenance
                            mac->gtsSpec2.slotStart[j]
                                = mac->gtsSpec3.slotStart[i];
                        }
                    }
                }
            }
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4UpdateGtsTriggerPrecedence
// LAYER      :: MAC
// PURPOSE    :: Updates GTS trigger precedence
// PARAMETERS ::
// + node           : Node*      : Pointer to node
// + interfaceIndex : Int32        : Interface index on MAC
// + bcnMsg         : Message*   : beacon message
// RETURN     :: None
// **/
static
void Mac802_15_4UpdateGtsTriggerPrecedence(Node* node,
                                           Int32 interfaceIndex,
                                           Message* bcnMsg)
{
    char* triggerPrecedence = MESSAGE_ReturnInfo(
                                         bcnMsg,
                                         INFO_TYPE_Gts_Trigger_Precedence);
    if (triggerPrecedence)
    {
        M802_15_4PIB t_mpib;
        t_mpib.macGtsTriggerPrecedence = *triggerPrecedence;
        Mac802_15_4MLME_SET_request(node,
                                    interfaceIndex,
                                    macGtsTriggerPrecedence,
                                    &t_mpib);
    }
    else
    {
        ERROR_Assert(FALSE, "INFO_TYPE_Gts_Trigger_Precedence not found");
    }
}

// /**
// FUNCTION   :: Mac802_15_4RecvBeacon
// LAYER      :: MAC
// PURPOSE    :: To process an incoming beacon
// PARAMETERS ::
// + node           : Node*             : Pointer to node
// + interfaceIndex : Int32               : Interface index on MAC
// + msg            : Message*          : Incoming message
// RETURN     :: None
// **/
static
void Mac802_15_4RecvBeacon(Node* node,
                           Int32 interfaceIndex,
                           Message* msg)
{
    MacData802_15_4* mac = NULL;
    M802_15_4Header* wph = NULL;
    M802_15_4BeaconFrame* bcnFrm;
    M802_15_4FrameCtrl frmCtrl;
    M802_15_4PendAddrSpec pendSpec;
    BOOL pending = FALSE;
    clocktype txtime = 0;
    UInt8 ifs = 0;
    Int32 i = 0;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    wph = (M802_15_4Header*) MESSAGE_ReturnPacket(msg);
    bcnFrm = (M802_15_4BeaconFrame*)((char *)wph + sizeof(M802_15_4Header));


    if (mac->isPANCoor)
    {
        // PAN coordinator Ignore this beacon
        MESSAGE_Free(node, msg);
        return;
    }

    // calculate the time when we received the first bit of the beacon
    txtime = Mac802_15_4TrxTime(node, interfaceIndex, msg);

    // calculate <beaconPeriods2>
    if (MESSAGE_ReturnPacketSize(msg) <= aMaxSIFSFrameSize)
    {
        ifs = aMinSIFSPeriod;
    }
    else
    {
        ifs = aMinLIFSPeriod;
    }
    clocktype tmpf;

    tmpf = txtime * Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex])
             / SECOND;
    tmpf += ifs ;
    mac->beaconPeriods2 = (UInt8)(tmpf / aUnitBackoffPeriod );

    if ((tmpf % aUnitBackoffPeriod ) > 0)
    {
        mac->beaconPeriods2++;
    }
    // update PAN descriptor
    frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
    Mac802_15_4FrameCtrlParse(&frmCtrl);
    mac->panDes2.CoordAddrMode = frmCtrl.srcAddrMode;
    mac->panDes2.CoordPANId = wph->MHR_SrcAddrInfo.panID;
    mac->panDes2.CoordAddress_64 = wph->MHR_SrcAddrInfo.addr_64;     // ok
                                        // even it is a 16-bit address
    mac->panDes2.LogicalChannel = wph->phyCurrentChannel;
    mac->panDes2.SuperframeSpec = bcnFrm->MSDU_SuperSpec;

    mac->panDes2.LinkQuality = 0;
    mac->panDes2.TimeStamp = mac->macBcnRxTime;
    mac->panDes2.SecurityUse = FALSE;
    mac->panDes2.ACLEntry = 0;
    mac->panDes2.SecurityFailure = FALSE;
    if ((mac->taskP.mlme_scan_request_STEP)
         || (mac->taskP.mlme_rx_enable_request_STEP))
    {
        if (DEBUG)
        {
            printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Received"
                   " beacon from  Co-ordinator %d with PAN Id %d\n",
                    node->getNodeTime(),
                    node->nodeId,
                    wph->MHR_SrcAddrInfo.addr_64,
                    wph->MHR_SrcAddrInfo.panID);
        }
        mac->rxBeacon = msg;
        Mac802_15_4Dispatch(node,
                            interfaceIndex,
                            PHY_SUCCESS,
                            "recvBeacon",
                            RX_ON, M802_15_4_SUCCESS);
    }

    if (mac->mpib.macPANId == 0xffff)
    {
        mac->macBeaconOrder2 = mac->sfSpec2.BO;
        mac->macSuperframeOrder2 = mac->sfSpec2.SO;
    }

    if ((mac->mpib.macPANId == 0xffff)
         || (mac->mpib.macPANId != mac->panDes2.CoordPANId)
         || (mac->taskP.mlme_associate_request_STEP)
         || (mac->mpib.macCoordExtendedAddress
         !=  wph->MHR_SrcAddrInfo.addr_64))
    {
        if (mac->taskP.mlme_associate_request_STEP
                || mac->taskP.mlme_scan_request_STEP)
        {
             // Populate macBcnRxTime only for valid beacons received
            {
                clocktype tmpf = 0;
                tmpf = node->getNodeTime() - txtime;
                mac->macBcnRxTime = tmpf;
                if (mac->mpib.macCoordExtendedAddress
                            ==  wph->MHR_SrcAddrInfo.addr_64)
                {
                    CsmaData802_15_4* csmaca
                        = (CsmaData802_15_4*)mac->csma;
                    if (csmaca->waitNextBeacon && mac->backoffStatus == 99)
                    {
                        Csma802_15_4NewBeacon(node, interfaceIndex, 'r');
                    }
                }
            }
        }
        if (DEBUG)
        {
            printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Dropping"
                   " beacon\n", node->getNodeTime(), node->nodeId);
        }

        MESSAGE_Free(node, msg);
        return;
    }

    // Populate macBcnRxTime only for valid beacons received
    tmpf = node->getNodeTime() - txtime;
    mac->macBcnRxTime = tmpf;

     mac->stats.numBeaconsRecd++;
    if (DEBUG)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Received"
               " beacon from Co-ordinator %d with PAN Id %d\n",
               node->getNodeTime(),
               node->nodeId,
               wph->MHR_SrcAddrInfo.addr_64,
               wph->MHR_SrcAddrInfo.panID);
    }

    mac->numLostBeacons = 0;
    mac->macBeaconOrder2 = mac->sfSpec2.BO;
    mac->macSuperframeOrder2 = mac->sfSpec2.SO;
    if (mac->mpib.macCoordShortAddress == M802_15_4_SHORTADDRESS)
    {
        if (frmCtrl.srcAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE16)
        {
            mac->mpib.macCoordShortAddress = wph->MHR_SrcAddrInfo.addr_16;
        }
    }

    mac->sfSpec2.superSpec = bcnFrm->MSDU_SuperSpec;
    Mac802_15_4SuperFrameParse(&mac->sfSpec2);
    mac->gtsSpec3.fields = bcnFrm->MSDU_GTSFields;

    // Update GTS Trigger precedence 
    Mac802_15_4UpdateGtsTriggerPrecedence(node, interfaceIndex, msg);
    Mac802_15_4HandleGtsChracteristics(node, mac);

    // Reset gts exhaust  count if pancoors deallocates any of the GTS
    // this is indicated by slot start been zero
    for (i = 0; i < mac->gtsSpec3.count; i++)
    {
        if (mac->gtsSpec3.fields.list[i].devAddr16 != 0
               && mac->gtsSpec3.slotStart[i] == 0
               && mac->gtsSpec3.fields.list[i].devAddr16
                    != mac->aExtendedAddress)
        {
            // pancoord has deallocated one of the gts
            // reset the exhaust count
            mac->gtsRequestExhausted = FALSE;
            break;
        }
   }

    // schedule gts timer
    if (mac->gtsSpec2.permit && mac->gtsSpec2.count > 0)
    {
        Int32 rate = Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
        clocktype sSlotDuration = (mac->sfSpec2.sd * SECOND) / rate;
        Int32 i = mac->gtsSpec2.count -1;
        if (mac->gtsSpec2.count > 1)
        {
           // debug asserts
           ERROR_Assert(mac->gtsSpec2.count <= 2, "GTS count should be"
                        " less than or equal to two");
           ERROR_Assert(mac->gtsSpec2.slotStart[mac->gtsSpec2.count - 1]
                            < mac->gtsSpec2.slotStart[mac->gtsSpec2.count - 2],
                        "Start slots should match");
        }
        ERROR_Assert(mac->gtsSpec2.fields.list[i].devAddr16
                    == mac->mpib.macCoordShortAddress, "Address should be same");
        clocktype delay
            = mac->gtsSpec2.slotStart[i] * sSlotDuration;
        delay -= max_pDelay;
        delay -= Mac802_15_4TrxTime(node, interfaceIndex, msg);
        ERROR_Assert(delay > 0, "delay should be more than zero");
        Message* timerMsg = NULL;
        timerMsg = MESSAGE_Alloc(node,
                               MAC_LAYER,
                               MAC_PROTOCOL_802_15_4,
                               MSG_MAC_TimerExpired);

        MESSAGE_SetInstanceId(timerMsg, (short)
             mac->myMacData->interfaceIndex);

        MESSAGE_InfoAlloc(node, timerMsg, sizeof(M802_15_4Timer));
        M802_15_4Timer* timerInfo
            = (M802_15_4Timer*) MESSAGE_ReturnInfo(timerMsg);

        timerInfo->timerType = M802_15_4GTSTIMER;
        MESSAGE_AddInfo(node,
                        timerMsg,
                        sizeof(UInt8),
                        INFO_TYPE_Gts_Slot_Start);
         char* gtsSlotInfo
                = MESSAGE_ReturnInfo(timerMsg, INFO_TYPE_Gts_Slot_Start);
         *gtsSlotInfo = mac->gtsSpec2.slotStart[i];
         ERROR_Assert(!mac->gtsT, "mac->gtsT should be NULL");
         mac->gtsT = timerMsg;
        MESSAGE_Send(node, timerMsg, delay);
    }

    Mac802_15_4Dispatch(node,
                        interfaceIndex,
                        PHY_SUCCESS,
                        "recvBeacon",
                        RX_ON,
                        M802_15_4_SUCCESS);
    if (wph->MHR_SrcAddrInfo.panID == mac->mpib.macPANId)
    {
        if (mac->backoffStatus == BACKOFF)
        {
            Csma802_15_4NewBeacon(node, interfaceIndex, 'r');
        }
    }

    // check if need to notify the upper layer
    if ((!mac->mpib.macAutoRequest) || (sizeof(M802_15_4BeaconFrame) > 0))
    {
        Sscs802_15_4MLME_BEACON_NOTIFY_indication(
                                    node,
                                    interfaceIndex,
                                    wph->MHR_BDSN,
                                    &mac->panDes2,
                                    bcnFrm->MSDU_PendAddrFields.spec,
                                    bcnFrm->MSDU_PendAddrFields.addrList,
                                    sizeof(M802_15_4BeaconFrame),
                                    NULL);
    }
    if (mac->mpib.macAutoRequest)
    {
        // handle the pending packet
        pendSpec.fields = bcnFrm->MSDU_PendAddrFields;
        Mac802_15_4PendAddSpecParse(&pendSpec);
        pending = FALSE;
        if (mac->taskP.mlme_poll_request_STEP)
        {
             Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                interfaceIndex,
                                                RX_ON);
        }
        for (i = 0; i < pendSpec.numShortAddr; i++)
        {
            if (pendSpec.fields.addrList[i] == mac->mpib.macShortAddress)
            {
                pending = TRUE;
                break;
            }
        }
        if (!pending)
        {
            for (i = 0; i < pendSpec.numExtendedAddr; i++)
            {
                if (pendSpec.fields.addrList[pendSpec.numShortAddr + i] ==
                    mac->aExtendedAddress)
                {
                    pending = TRUE;
                    break;
                }
            }
        }

        if (frmCtrl.frmPending)
        {
            // broadcast is pending
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       " Coordinator is about to send a broadcast\n",
                      node->getNodeTime(),
                      node->nodeId);
            }
            if (pending && !mac->taskP.mlme_poll_request_STEP)
            {
                // Unicast is also pending
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           " Unicast Frame is pending at the coordinator\n",
                           node->getNodeTime(),
                           node->nodeId);
                }
                mac->isPollRequestPending = TRUE;
            }

            // Set the Trx state to Rx ON & wait for the broadcast packet to
            // arive
            // Handle the unicast at the handler of broadcast timer

            Phy802_15_4PlmeSetTRX_StateRequest(
                    node,
                    interfaceIndex,
                    RX_ON);
            clocktype wtime = aMaxFrameResponseTime * SECOND
                / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
             mac->broadcastT =  Mac802_15_4SetTimer(
                                                  node,
                                                  mac,
                                                  M802_15_4BROASCASTTIMER,
                                                  wtime,
                                                  NULL);
        }
        else if (pending && !mac->taskP.mlme_poll_request_STEP)
        {
            // No broadcast is pending but unicast is pending
            // Send the poll request after IFS time.
            
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       " Unicast Frame is pending at the coordinator\n",
                       node->getNodeTime(),
                       node->nodeId);
            }
            mac->isPollRequestPending = TRUE;
            mac->ifsTimerCalledAfterReceivingBeacon = TRUE;
            if (MESSAGE_ReturnPacketSize(msg) <= aMaxSIFSFrameSize)
            {
                ifs = aMinSIFSPeriod;
            }
            else
            {
                ifs = aMinLIFSPeriod;
            }
            mac->IFST =
                Mac802_15_4SetTimer(
                node,
                mac,
                M802_15_4IFSTIMER,
                ifs * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                NULL);
        }
    }
    MESSAGE_Free(node, msg);
}

// /**
// FUNCTION   :: Mac802_15_4RecvAck
// LAYER      :: MAC
// PURPOSE    :: To process an incoming Ack message
// PARAMETERS ::
// + node           : Node*             : Pointer to node
// + interfaceIndex : Int32               : Interface index on MAC
// + msg            : Message*          : Incoming message
// RETURN     :: None
// **/
static
void Mac802_15_4RecvAck(Node* node,
                        Int32 interfaceIndex,
                        Message* msg)
{
    MacData802_15_4* mac = NULL;
    UInt16* ackHeaderIterator = NULL;
    M802_15_4FrameCtrl frmCtrl;
    Message* msgWaitAck = NULL;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    ackHeaderIterator = (UInt16*) MESSAGE_ReturnPacket(msg);
    frmCtrl.frmCtrl = *ackHeaderIterator;
    ackHeaderIterator +=1;

    if (((!mac->txBcnCmd)
        && (!mac->txBcnCmd2)
        && (!mac->txData)
        && (!mac->txGts))
        || mac->backoffStatus == BACKOFF)
    {
        if (DEBUG)
        {
            printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                   " Dropping Ack, not waiting for one\n",
                   node->getNodeTime(), node->nodeId);
        }
        MESSAGE_Free(node, msg);
        return;
    }
    
    if (mac->txGts)
    {
        msgWaitAck = mac->txGts;
    }
    else if (mac->txBcnCmd)
    {
        msgWaitAck = mac->txBcnCmd;
    }
    else if (mac->txBcnCmd2)
    {
        msgWaitAck = mac->txBcnCmd2;
    }
    else if (mac->txData)
    {
        msgWaitAck = mac->txData;
    }

    if (*((UInt8*)ackHeaderIterator)
        != ((M802_15_4Header*) MESSAGE_ReturnPacket(msgWaitAck))->MHR_BDSN)
    {
        if (DEBUG)
        {
            printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                   " Dropping Ack, DSN  mismatch\n",
                   node->getNodeTime(), node->nodeId);
        }
        MESSAGE_Free(node, msg);
        return;
    }

    if (mac->txT != NULL)
    {
        MESSAGE_CancelSelfMsg(node, mac->txT);
        mac->txT = NULL;
    }
    else
    {
    }

    if (DEBUG)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Received Ack "
               "with DSN %d\n",
               node->getNodeTime(),
               node->nodeId,
               *((UInt8*)ackHeaderIterator));
    }
    mac->stats.numAckRecd++;
    
    // set pending flag for data polling
    if (msgWaitAck == mac->txBcnCmd2)
    {
        if ((mac->taskP.mlme_poll_request_STEP)
             && (strcmp(mac->taskP.mlme_poll_request_frFunc, "recvAck")
                == 0))
        {
            Mac802_15_4FrameCtrlParse(&frmCtrl);
            mac->taskP.mlme_poll_request_pending = frmCtrl.frmPending;
        }
    }

    Mac802_15_4Dispatch(node, interfaceIndex, PHY_SUCCESS, "recvAck",
                        RX_ON, M802_15_4_SUCCESS);

    MESSAGE_Free(node, msg);
}

// /**
// NAME         ::Mac802_15_4CreateDescriptorForBeacon
// PURPOSE      ::Handle a GTS request
// PARAMETERS   ::
// + node      : Node*        : Pointer to node
// + mac: MacData802_15_4*    : Pointer to MAC802.15.4 structure
// + startSlot : UInt8        : GTS start slot
// + numGtsSlots : UInt8      : GTS length
// + gtsDirection : BOOL      : GTS direction
// + wph : M802_15_4Header*   : Pointer to the MAC header
// RETURN       ::None
// NOTES        ::None
// **/
static
void Mac802_15_4CreateDescriptorForBeacon(Node* node,
                                          MacData802_15_4* mac,
                                          UInt8 startSlot,
                                          UInt8 numGtsSlots,
                                          BOOL gtsDirection,
                                          M802_15_4Header* wph)
{
    BOOL gtsDescriptorFound = FALSE;
    Int32 k = 0;
    for (k = 0; k < mac->gtsSpec3.count; k++)
    {
        if (wph->MHR_SrcAddrInfo.addr_64
                == (((mac->gtsSpec3).fields).list[k]).devAddr16)
        {
            if (gtsDirection == mac->gtsSpec3.recvOnly[k])
            {
                // We already have a descriptor in the gtsSpec3
                // set the start slot to zero & reset gtspersistence timr
                
                ERROR_Assert(!gtsDescriptorFound, "gtsDescriptorFound"
                    " should be FALSE");
                Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec3,
                                             k,
                                             startSlot);
                Mac802_15_4GTSSpecSetLength(&mac->gtsSpec3,
                                          k,
                                          numGtsSlots);
                mac->gtsSpec3.aGtsDescPersistanceCount[k]
                    = aGTSDescPersistenceTime;
                gtsDescriptorFound = TRUE;
            }
        }
    }
    if (!gtsDescriptorFound && (mac->gtsSpec3.count < aMaxNumGts))
    {
        // Add this descriptor for the beacon
        (((mac->gtsSpec3).fields).list[mac->gtsSpec3.count]).devAddr16
            = wph->MHR_SrcAddrInfo.addr_64;
        Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec3,
                                       mac->gtsSpec3.count,
                                       startSlot);
        Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec3,
                                      mac->gtsSpec3.count,
                                      gtsDirection);
        Mac802_15_4GTSSpecSetLength(&mac->gtsSpec3,
                                    mac->gtsSpec3.count,
                                    numGtsSlots);
         mac->gtsSpec3.aGtsDescPersistanceCount[mac->gtsSpec3.count]
            = aGTSDescPersistenceTime;
        Mac802_15_4GTSSpecSetCount(&mac->gtsSpec3,
                                   mac->gtsSpec3.count + 1);
    }
}

// /**
// NAME         ::Mac802_15_4HandleGtsRequest
// PURPOSE      ::Handle a GTS request
// PARAMETERS   ::
// + node      : Node*        : Pointer to node
// + interfaceIndex : Int32     : interface index
// + mac: MacData802_15_4*    : Pointer to MAC802.15.4 structure
// + wph : M802_15_4Header*   : Pointer to the MAC header in the data packet
// + payld : char*            :GTS chracteristics
// RETURN       ::None
// NOTES        ::None
// **/
static
void Mac802_15_4HandleGtsRequest(Node* node,
                                 Int32 interfaceIndex,
                                 MacData802_15_4* mac,
                                 M802_15_4Header* wph,
                                 char* payld)
{
    Int32 rate = Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
    clocktype sSlotDuration = mac->sfSpec.sd * SECOND / rate;
    clocktype aMinCap = 440 * SECOND / rate;
    UInt8 numGtsSlots = 0;

    // get the gts request type from the 6th bit of the gts chracteristics
    // field. It would be either allocation or de-allocation request
    UInt8 gtsTypeRequested = *payld & 32;

    // get the gts direction requested from the 5th bit of the gts
    // chracteristics field
    UInt8 gtsDirectionRequested = (*payld & 16) ? 1 : 0;

    // get the number of gts slots requested from the first 4 bits of the gts
    // chracteristics field
    UInt8 numGtsSlotsRequested = *payld & 15;

    BOOL processGtsRequest = FALSE;
    BOOL gtsDescriptorAlreadyExists = FALSE;
    if (!((mac->gtsSpec).permit))
    {
        mac->stats.numGtsReqIgnored++;
        return;
    }
    Int32 i = 0;
    for (i = 0; i < mac->gtsSpec.count; i++)
    {
        if ((((mac->gtsSpec).fields).list[i]).devAddr16
            == wph->MHR_SrcAddrInfo.addr_64)
        {
            if (mac->gtsSpec.recvOnly[i] == gtsDirectionRequested)
            {
                gtsDescriptorAlreadyExists = TRUE;
            }
        }
    }
    if (gtsTypeRequested > 0)
    {
        // GTS Allocation Request
        if (gtsDescriptorAlreadyExists)
        {
            processGtsRequest = FALSE;
            mac->stats.numGtsReqIgnored++;
        }
        else
        {
            processGtsRequest = TRUE;
            mac->stats.numGtsReqRecd++;
        }
    }
    else
    {
        // GTS De-allocation Request
        if (gtsDescriptorAlreadyExists)
        {
            processGtsRequest = TRUE;
            mac->stats.numGtsDeAllocationReqRecd++;
        }
        else
        {
            processGtsRequest = FALSE;
            mac->stats.numGtsReqIgnored++;
        }
    }

    if (processGtsRequest && gtsTypeRequested > 0)
    {
        // gts Allocation request
        Int32 i = 0;
        for (i = 0; i < mac->gtsSpec.count; i++)
        {
            numGtsSlots += mac->gtsSpec.length[i];
        }
        Int32 numCapSlotsleft = 16 - numGtsSlots;
        clocktype t_cap = numCapSlotsleft * sSlotDuration;
        if (mac->gtsSpec.count < aMaxNumGts && t_cap > aMinCap)
        {
            // Pancoord can allocate moreGTS
            clocktype gtslengthRequested = numGtsSlotsRequested * sSlotDuration;
            if (t_cap  - gtslengthRequested >= aMinCap)
            {
                // gts can be allocated
                // allocate the new GTS

                UInt8 newGtsStartSlot
                    = aNumSuperframeSlots
                        - (numGtsSlots + numGtsSlotsRequested);
                (((mac->gtsSpec).fields).list[mac->gtsSpec.count]).devAddr16
                    = wph->MHR_SrcAddrInfo.addr_64;
                Mac802_15_4GTSSpecSetSlotStart(&mac->gtsSpec,
                                               mac->gtsSpec.count,
                                               newGtsStartSlot);
                Mac802_15_4GTSSpecSetRecvOnly(&mac->gtsSpec,
                                              mac->gtsSpec.count,
                                              gtsDirectionRequested > 0);
                Mac802_15_4GTSSpecSetLength(&mac->gtsSpec,
                                            mac->gtsSpec.count,
                                            numGtsSlotsRequested);
                mac->gtsSpec.expiraryCount[mac->gtsSpec.count] 
                    = Mac802_15_4GetMaxExpiraryCount(mac);
                Mac802_15_4GTSSpecSetCount(&mac->gtsSpec,
                                           mac->gtsSpec.count + 1);
                
                 ERROR_Assert(mac->sfSpec.BO != 15, "BO should be <= 15");
                
                 // update the final cap slot
                // mac->sfSpec.FinCAP = newGtsStartSlot - 1;
                mac->currentFinCap = newGtsStartSlot - 1;

                // Generate a temporary descriptor for the beacon
                Mac802_15_4CreateDescriptorForBeacon(node,
                                                  mac,
                                                  newGtsStartSlot,
                                                  numGtsSlotsRequested,
                                                  gtsDirectionRequested > 0,
                                                  wph);
            }
            else
            {
                // GTS cannot be allocated
                mac->stats.numGtsRequestsRejectedByPanCoord++;
                Mac802_15_4CreateDescriptorForBeacon(node,
                                                  mac,
                                                  0,
                                                  numGtsSlotsRequested,
                                                  gtsDirectionRequested > 0,
                                                  wph);
            }
        }
        else
        {
            // GTS cannot be allocated
            mac->stats.numGtsRequestsRejectedByPanCoord++; 
            Mac802_15_4CreateDescriptorForBeacon(node,
                                                 mac,
                                                 0,
                                                 numGtsSlotsRequested,
                                                 gtsDirectionRequested > 0,
                                                 wph);
        }
    }
    else if (processGtsRequest)
    {
        // gts deallocation
        Int32 i = 0;
        for (i = 0; i < mac->gtsSpec.count; i++)
        {
            if ((((mac->gtsSpec).fields).list[i]).devAddr16 
                == wph->MHR_SrcAddrInfo.addr_64)
            {
                if (mac->gtsSpec.recvOnly[i] == gtsDirectionRequested)
                { 
                    // start deallocation process fromm pancoord
                    Mac802_15_4DeallocateGts(node, mac, i);
                    break;
                }// if (mac->gtsSpec.recvOnly[i] == gtsDirectionRequested)
            }
        }// for (i = 0; i < mac->gtsSpec.count; i++)
    }
    
    // schedule the GTS timer
    if (mac->gtsT)
    {
         MESSAGE_CancelSelfMsg(node, mac->gtsT);
         mac->gtsT = NULL;
    }

    if (mac->gtsSpec.count > 0)
    {
        // A valid gts is available
        clocktype delay = 0;
        BOOL scheduleTimer = FALSE;
        Int32 i = 0;
        for (i = mac->gtsSpec.count - 1; i >= 0 ; i--)
        {
            delay = (mac->gtsSpec.slotStart[i] * sSlotDuration)
                        - (node->getNodeTime() - mac->macBcnTxTime);
            if (delay > 0)
            {
                scheduleTimer = TRUE;
                break;
            }
        }
        if (scheduleTimer)
        {
            //  allocate the timer message and send out
            Message* timerMsg = NULL;
            timerMsg = MESSAGE_Alloc(node,
                                     MAC_LAYER,
                                     MAC_PROTOCOL_802_15_4,
                                     MSG_MAC_TimerExpired);

            MESSAGE_SetInstanceId(timerMsg,
                                  (short) mac->myMacData->interfaceIndex);

            MESSAGE_InfoAlloc(node, timerMsg, sizeof(M802_15_4Timer));
            M802_15_4Timer* timerInfo
                = (M802_15_4Timer*) MESSAGE_ReturnInfo(timerMsg);

            timerInfo->timerType = M802_15_4GTSTIMER;
            MESSAGE_AddInfo(node,
                         timerMsg,
                        sizeof(UInt8),
                         INFO_TYPE_Gts_Slot_Start);
            char* gtsSlotInfo
                    = MESSAGE_ReturnInfo(timerMsg, INFO_TYPE_Gts_Slot_Start);
            *gtsSlotInfo = mac->gtsSpec.slotStart[i];
            mac->gtsT = timerMsg;
            MESSAGE_Send(node, timerMsg, delay);
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4RecvCommand
// LAYER      :: MAC
// PURPOSE    :: To process an incoming MAC command
// PARAMETERS ::
// + node           : Node*             : Pointer to node
// + interfaceIndex : Int32               : Interface index on MAC
// + msg            : Message*          : Incoming message
// RETURN     :: None
// **/
static
void Mac802_15_4RecvCommand(Node* node,
                            Int32 interfaceIndex,
                            Message* msg)
{
    MacData802_15_4* mac = NULL;
    M802_15_4Header* wph = NULL;
    char* payld = NULL;
    M802_15_4FrameCtrl frmCtrl;
    BOOL ackReq = FALSE;
    char cmdName[100];

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    wph = (M802_15_4Header*) MESSAGE_ReturnPacket(msg);
    payld = (char*) ((char*)wph + sizeof(M802_15_4Header)
                + sizeof(M802_15_4CommandFrame));

    ackReq = FALSE;
    switch(wph->MSDU_CmdType)
    {
        case 0x01:  // Association request
        {
            mac->stats.numAssociateReqRecd++;
            if (DEBUG)
            {
                strcpy(cmdName, "Association request");
            }
            ERROR_Assert(mac->rxCmd == NULL, "mac->rxCmd should be NULL");
            mac->rxCmd = msg;
            ackReq = TRUE;
            break;
        }
        case 0x02:  // Association response
        {
            mac->stats.numAssociateResRecd++;
            if (DEBUG)
            {
                strcpy(cmdName, "Association response");
            }
            ERROR_Assert(mac->rxCmd == NULL, "mac->rxCmd should be NULL");
            mac->rxCmd = msg;
            ackReq = TRUE;
            mac->rt_myNodeID = *((UInt16*)payld);
            if (mac->taskP.mlme_associate_request_STEP
                == ASSOC_DATAREQ_CSMA_STATUS_STEP)
            {
                // change associate request step - device might have received
                // response while it was trying to resend Data request

                mac->taskP.mlme_associate_request_STEP
                    = ASSOC_DATAREQ_ACK_STATUS_STEP;
            }
            break;
        }
        case 0x03:  // Disassociation notification
        {
            mac->stats.numDisassociateReqRecd++;
            if (DEBUG)
            {
                strcpy(cmdName, "Disassociation notification");
            }

            MESSAGE_Free(node, msg);

            break;
        }
        case 0x04:  // Data request
        {
            mac->stats.numDataReqRecd++;
            if (DEBUG)
            {
                strcpy(cmdName, "Data request");
            }
            ERROR_Assert(mac->rxCmd == NULL, "mac->rxCmd should be NULL");
            mac->rxCmd = msg;
            ackReq = TRUE;
            break;
        }
        case 0x05:  // PAN ID conflict notification
        {
            if (DEBUG)
            {
                strcpy(cmdName, "PAN ID conflict notification");
            }

            MESSAGE_Free(node, msg);
            break;
        }
        case 0x06:  // Orphan notification
        {
            mac->stats.numOrphanIndRecd++;
            if (DEBUG)
            {
                strcpy(cmdName, "Orphan notification");
            }

            // If already processing for orphan indication command
            if (mac->taskP.mlme_orphan_response_STEP == 1)
            {
                if (DEBUG)
                {
                   printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                          " Already processing"
                          " oprhan indication request. Discarding one received"
                          " from node %d\n", node->getNodeTime(), node->nodeId,
                          wph->MHR_SrcAddrInfo.addr_64);
                }
            }
            else
            {
                Sscs802_15_4MLME_ORPHAN_indication(node,
                                                interfaceIndex,
                                                wph->MHR_SrcAddrInfo.addr_64,
                                                FALSE,
                                                0);
            }
            MESSAGE_Free(node, msg);
            break;
        }
        case 0x07:  // Beacon request
        {
            mac->stats.numBeaconsReqRecd++;
            if (DEBUG)
            {
                strcpy(cmdName, "Beacon request");
            }
            if (mac->capability.FFD // I am an FFD
                && (mac->isCoor)   // Coordinator
                && (mac->mpib.macAssociationPermit) // association permitted
                && (mac->mpib.macShortAddress != 0xffff) // allowed beacons
                && (mac->mpib.macBeaconOrder == 15)) // non-beacon mode
            {
                // send a beacon using unslotted CSMA-CA
                ERROR_Assert(!mac->txBcnCmd, "mac->txBcnCmd should be NULL");
                frmCtrl.frmCtrl = 0;
                Mac802_15_4FrameCtrlSetFrmType(
                                        &frmCtrl,
                                        M802_15_4DEFFRMCTRL_TYPE_BEACON);
                Mac802_15_4FrameCtrlSetSecu(&frmCtrl, mac->secuBeacon);
                Mac802_15_4FrameCtrlSetAckReq(&frmCtrl, FALSE);
                Mac802_15_4FrameCtrlSetDstAddrMode(
                                        &frmCtrl,
                                        M802_15_4DEFFRMCTRL_ADDRMODENONE);
                if (mac->mpib.macShortAddress == 0xfffe)
                {
                    Mac802_15_4FrameCtrlSetSrcAddrMode(&frmCtrl,
                            M802_15_4DEFFRMCTRL_ADDRMODE64);
                }
                else
                {
                    Mac802_15_4FrameCtrlSetSrcAddrMode(&frmCtrl,
                            M802_15_4DEFFRMCTRL_ADDRMODE16);
                }
                mac->sfSpec.superSpec = 0;
                Mac802_15_4SuperFrameSetBO(&mac->sfSpec, 15);
                Mac802_15_4SuperFrameSetBLE(&mac->sfSpec,
                        mac->mpib.macBattLifeExt);
                Mac802_15_4SuperFrameSetPANCoor(&mac->sfSpec,
                                                mac->isPANCoor);
                Mac802_15_4SuperFrameSetAssoPmt(
                                            &mac->sfSpec,
                                            mac->mpib.macAssociationPermit);

                Mac802_15_4ConstructPayload(node,
                                            interfaceIndex,
                                            &mac->txBcnCmd,
                                            M802_15_4DEFFRMCTRL_TYPE_BEACON,
                                            mac->sfSpec.superSpec,
                                            &mac->gtsSpec.fields,
                                            &mac->pendAddrSpec.fields,
                                            0);
                Mac802_15_4AddCommandHeader(
                        node,
                        interfaceIndex,
                        mac->txBcnCmd,
                        M802_15_4DEFFRMCTRL_TYPE_BEACON,
                        0,
                        0,
                        ((M802_15_4Header*)
                            MESSAGE_ReturnPacket(msg))->MHR_SrcAddrInfo.addr_64,
                        frmCtrl.srcAddrMode,
                        mac->mpib.macPANId,
                        mac->aExtendedAddress,
                        FALSE,
                        FALSE,
                        FALSE,
                        0,
                        0);

                M802_15_4Header* wph1 = NULL;
                wph1 = (M802_15_4Header*) MESSAGE_ReturnPacket(mac->txBcnCmd);
                if (mac->mpib.macShortAddress != 0xfffe)
                {
                    wph1->MHR_SrcAddrInfo.addr_16
                        = mac->mpib.macShortAddress;
                }

                wph1->phyCurrentChannel
                    = mac->taskP.mlme_start_request_LogicalChannel;
                mac->stats.numBeaconsSent ++;
                Mac802_15_4CsmacaBegin(node, interfaceIndex, 'c');
            }
            MESSAGE_Free(node, msg);
            break;
        }
        case 0x08:  // Coordinator realignment
        {
            if (DEBUG)
            {
                strcpy(cmdName, "Coordinator realignment request");
            }
            frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
            Mac802_15_4FrameCtrlParse(&frmCtrl);
            if (frmCtrl.dstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE64)
            // directed to an orphan device
            {
                ERROR_Assert(mac->rxCmd == NULL, "mac->rxCmd should be NULL");
                mac->rxCmd = msg;
                ackReq = TRUE;
            }
            else if ((wph->MHR_SrcAddrInfo.addr_64
                        == mac->mpib.macCoordExtendedAddress)
                    && (wph->MHR_SrcAddrInfo.panID == mac->mpib.macPANId))
            {
                // no specification in the draft as how to handle this packet,
                // so use ns-2 implementation

                mac->mpib.macPANId = *((UInt16*)payld);
                memcpy(&(mac->mpib.macCoordShortAddress),
                        payld + 2,
                        sizeof(UInt16));
                mac->tmp_ppib.phyCurrentChannel = payld[4];
                Phy802_15_4PLME_SET_request(node,
                                            interfaceIndex,
                                            phyCurrentChannel,
                                            &mac->tmp_ppib);
                MESSAGE_Free(node, msg);
            }
            break;
        }
        case 0x09:  // GTS request
        {
            ERROR_Assert(mac->isPANCoor, "Should be a Pan coordinator");
            Mac802_15_4HandleGtsRequest(node, interfaceIndex, mac, wph, payld);
            MESSAGE_Free(node, msg);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Invalid command type");
            break;
        }
    }
    if (DEBUG)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : Received %s"
               " command from Node %d\n",
               node->getNodeTime(), node->nodeId,
               cmdName, wph->MHR_SrcAddrInfo.addr_64);
    }
}


// /**
// NAME         Mac802_15_4CalculateGtsLength
// PURPOSE      Calculates GTS length
// PARAMETERS   Node* node
// + node      : Node*               : Pointer to node
// + interfaceIndex : Int32            : interface index
// + mac: MacData802_15_4*           : Pointer to MAC802.15.4 structure
// + zigbeeAppInfo : ZigbeeAppInfo*  : Pointer to the zigbee info
// + msg : Message*                  : Pointer to the message
// RETURN       None
// NOTES        None
// **/
static
UInt8 Mac802_15_4CalculateGtsLength(Node* node,
                                    Int32 interfaceIndex,
                                    MacData802_15_4* mac,
                                    ZigbeeAppInfo* zigbeeAppInfo,
                                    Message* msg,
                                    BOOL frmPhyLayer)
{
     UInt8 numGtsSlots = 1;
     Float64 transportHeaderSize = 0;
     Float64 networkHeaderSize = 0;
     Float64 udpHeaderSize = sizeof(TransportUdpHeader);
     Float64 ipHeaderSize = sizeof(IpHeaderType);
     Float64 macHeaderSize = sizeof(M802_15_4Header);
     Float64 numFragsPerPkt = 1;
     Float64 fragSize = zigbeeAppInfo->zigbeeAppPktSize
                            + udpHeaderSize + ipHeaderSize;
     Float64 beaconInterval = 0;
     Float64 slotDuration = 0;
     M802_15_4SuperframeSpec* sfSpecsToCheck = NULL;
     clocktype referenceBeaconTime = 0;
     M802_15_4GTSSpec* gtsSpecsToCheck = NULL;

     // Get the GTS specs
     Mac802_15_4GetSpecks(node,
                          mac,
                          interfaceIndex,
                          &gtsSpecsToCheck,
                          &sfSpecsToCheck,
                          &referenceBeaconTime);

    UInt32 ip_v_hl_tos_len = 0;
    Int8 version = 0;
    IpHeaderType* ipHdr = NULL;

    if (frmPhyLayer)
    {
        Int8* pktItr = MESSAGE_ReturnPacket(msg);
        pktItr +=  sizeof(M802_15_4Header);
        ipHdr = (IpHeaderType*) pktItr;
    }
    else
    {
        ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    }

    ip_v_hl_tos_len = *((UInt32*) ipHdr);
    version = IpHeaderGetVersion(ip_v_hl_tos_len);
    if (version == IPVERSION4)
    {
        networkHeaderSize = ipHeaderSize;
        if (ipHdr->ip_p == IPPROTO_UDP)
        {
            transportHeaderSize = udpHeaderSize;
        }
        else
        {
              ERROR_Assert(FALSE, "Currently only UDP based applications are"
                           " supported by GTS service");
        }
    }
    else
    {
        ERROR_Assert(FALSE, "Currently only IPv4 based applications are"
                     " supported by GTS service");
    }

    // Calculate the beacon interval
    beaconInterval = ((Float64)sfSpecsToCheck->BI)
                        / (Float64)Phy802_15_4GetSymbolRate(
                                    node->phyData[interfaceIndex]);

    // Take into consideration fragementation at the network layer
    if (zigbeeAppInfo->zigbeeAppPktSize + transportHeaderSize
            + networkHeaderSize
         > zigbeeAppInfo->ipFragUnit)
    {
        numFragsPerPkt = ceil((zigbeeAppInfo->zigbeeAppPktSize
                                + transportHeaderSize)
                            /(zigbeeAppInfo->ipFragUnit - networkHeaderSize));
        fragSize = zigbeeAppInfo->ipFragUnit;
    }

    // Calculate the number of fragments required to be send per second
    Float64 numfragsPerSecond = ((Float64)SECOND
                                    / (Float64)zigbeeAppInfo->zigbeeAppInterval)
                                    * numFragsPerPkt;
    Float64 numFragsPerSuperframe = ceil(numfragsPerSecond * beaconInterval);
    slotDuration = (sfSpecsToCheck->sd)
                    /(Float64)Phy802_15_4GetSymbolRate(
                            node->phyData[interfaceIndex]);

    // Calculate the transmission time by a fragment
    clocktype transmissionTimePerFrag = PHY_GetTransmissionDuration(
                                         node,
                                         interfaceIndex,
                                         M802_15_4_DEFAULT_DATARATE_INDEX,
                                         (Int32)(fragSize + macHeaderSize));

    // Calculate the transmission time required to send the fragments in a
    // superframe

    Float64 totalTransmissionTimePerSuperFrame
     = (Float64)((Float64)transmissionTimePerFrag / (Float64)SECOND)
            * numFragsPerSuperframe;
    Float64 t_IFS = 0;

    if (fragSize <= aMaxSIFSFrameSize)
    {
        t_IFS = aMinSIFSPeriod;
    }
    else
    {
        t_IFS = aMinLIFSPeriod;
    }

    t_IFS /= Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
    Float64 totalIfsRequired = t_IFS * numFragsPerSuperframe;
    Float64 totalTime = totalTransmissionTimePerSuperFrame + totalIfsRequired;

    if (mac->mpib.macDataAcks)
    {
        // Take into consideration ack waiting time when data acks are
        // enabled

        Float64 ackWaitDurationPerFrag = (Float64)(mac->mpib.macAckWaitDuration)
                / (Float64)Phy802_15_4GetSymbolRate(
                    node->phyData[interfaceIndex]);
        Float64 totalAckWaitDurationPerSuperFrame
                = ackWaitDurationPerFrag * numFragsPerSuperframe;
        totalTime += totalAckWaitDurationPerSuperFrame;
    }

     Float64 slotDurationRequired = slotDuration;

     // Calculate the number of GTS slots required to support the data
     // rate configured at application layer

    while (totalTime > slotDurationRequired)
    {
        slotDurationRequired += slotDuration;
        numGtsSlots++;
        if (numGtsSlots > aMaxNumGtsSlotsRequested)
        {
            // Maximum number of GTS slots requsted should not exceed
            // aMaxNumGtsSlotsRequested (15)

            char buf1[MAX_STRING_LENGTH];
            char buf2[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(zigbeeAppInfo->srcAddress, buf2);
            sprintf(buf1, "\nData-rate configured at application layer "
                    "at IPv4 address %s is exceeding the capacity "
                    "provided by GTS service"
                    , buf2);
            ERROR_ReportWarning(buf1);
            return aMaxNumGtsSlotsRequested;
        }
    }
    return numGtsSlots;
}

// /**
// NAME         Mac802_15_4UpdateGtsChracteristics
// PURPOSE      Updates GTS characteristics when a data packet is received
//              at the node
// PARAMETERS   Node* node
//                  Node which sends the message
//              MacData_802_15_4* mac
//                  802.15.4 data structure
//              Message* msg
//                  Pointer to the data packet received
//              M802_15_4Header* wph
//                  Pointer to the MAC header in the data packet
// RETURN       None
// NOTES        None
// **/
static 
void Mac802_15_4UpdateGtsChracteristics(Node* node,
                                        Int32 interfaceIndex,
                                        MacData802_15_4* mac,
                                        Message* msg,
                                        M802_15_4Header* wph)
{
    // Extract the data rate information from the received packet
    ZigbeeAppInfo* zigbeeAppInfo
            = (ZigbeeAppInfo*)MESSAGE_ReturnInfo(msg, INFO_TYPE_ZigbeeApp_Info);
    if (zigbeeAppInfo
            && (mac->isCoor || mac->isCoorInDeviceMode)
            && !mac->isPANCoor)
    {
        if (zigbeeAppInfo->priority > mac->mpib.macGtsTriggerPrecedence)
        {
            // if priority is more than configured GTS trigger precedence
            if (wph->MHR_SrcAddrInfo.addr_64
                    == mac->mpib.macCoordShortAddress
                  && mac->gtsSpec2.permit
                  && !mac->sendGtsConfirmationPending)
            {
                ERROR_Assert(mac->sfSpec2.PANCoor, "Parent should be a pan"
                            "-coordinator ");
                BOOL validGtsDescriptorAvailable = FALSE;
                Int32 i = 0;
                for (i = 0; i < mac->gtsSpec2.count; i++)
                {
                    if (mac->gtsSpec2.recvOnly[i])
                    {
                        // have a valid receive GTS
                        validGtsDescriptorAvailable = TRUE;
                        if (mac->gtsSpec2.endTime[i] == 0
                            || zigbeeAppInfo->endTime
                                > mac->gtsSpec2.endTime[i])
                        {
                            // schedule or reschedule the timer for
                            // deallocating the allocated GTS

                            if (mac->gtsSpec2.deAllocateTimer[i])
                            {
                                MESSAGE_CancelSelfMsg(node,
                                mac->gtsSpec2.deAllocateTimer[i]);
                                mac->gtsSpec2.deAllocateTimer[i] = NULL;
                            }
                            clocktype delay = 0;
                            if (zigbeeAppInfo->endTime <= node->getNodeTime())
                            {
                               // start De-allocation process
                                mac->gtsSpec2.endTime[i]
                                                = node->getNodeTime();
                                delay = 0;
                            }
                            else
                            {
                                mac->gtsSpec2.endTime[i]
                                    = zigbeeAppInfo->endTime;
                                delay = zigbeeAppInfo->endTime - node->getNodeTime();
                            }
                             
                            Message* timerMsg = NULL;
                            timerMsg = MESSAGE_Alloc(node,
                                                     MAC_LAYER,
                                                     MAC_PROTOCOL_802_15_4,
                                                     MSG_MAC_TimerExpired);

                            MESSAGE_SetInstanceId(
                                  timerMsg,
                                  (short)mac->myMacData->interfaceIndex);

                            MESSAGE_InfoAlloc(node,
                                              timerMsg,
                                              sizeof(M802_15_4Timer));
                            M802_15_4Timer* timerInfo
                                  = (M802_15_4Timer*)
                                        MESSAGE_ReturnInfo(timerMsg);

                            timerInfo->timerType
                                = M802_15_4GTSDEALLOCATETIMER;
                            mac->gtsSpec2.deAllocateTimer[i] = timerMsg;
                            MESSAGE_Send(node, timerMsg, delay);
                        }
                    }
                }// for
               if (!validGtsDescriptorAvailable)
               {
                    if (!mac->sendGtsRequestToPancoord 
                              && !mac->gtsRequestExhausted 
                              && !mac->taskP.mlme_gts_request_STEP
                              && !mac->sendGtsConfirmationPending
                              && zigbeeAppInfo->endTime
                                    > node->getNodeTime())
                    {
                        ERROR_Assert(!mac->gtsRequestData.active, "There"
                                " should not be any previos GTS request"
                                " data");
                        mac->sendGtsRequestToPancoord = TRUE;
                        mac->gtsRequestData.allocationRequest = TRUE;
                        mac->gtsRequestData.receiveOnly = TRUE;
                        mac->gtsRequestData.numSlots
                            = Mac802_15_4CalculateGtsLength(node,
                                                            interfaceIndex,
                                                            mac,
                                                            zigbeeAppInfo,
                                                            msg,
                                                            TRUE);
                        mac->gtsRequestData.active = TRUE;
                    }
               }
            }
        }
    }
    else if (zigbeeAppInfo && mac->isPANCoor && mac->gtsSpec.permit)
    {
        // As we have received a packet.
        // Reset expirary count at the pan coordinator with GTS is allocated

        if (zigbeeAppInfo->priority > mac->mpib.macGtsTriggerPrecedence)
        {
            Int32 i = 0;
            for (i = 0; i < mac->gtsSpec.count; i++)
            {
                if (!mac->gtsSpec.recvOnly[i]
                        && wph->MHR_SrcAddrInfo.addr_64
                      == mac->gtsSpec.fields.list[i].devAddr16)
                {
                    // have a valid transmit GTS
                    if (i == 0)
                    {
                        if (!mac->gtsT)
                        {
                             // we are last slot of CFP
                             mac->gtsSpec.expiraryCount[i]
                                = Mac802_15_4GetMaxExpiraryCount(mac);
                        }
                        else
                        {
                            // we are in CAP
                        }
                    }
                    else
                    {
                       ERROR_Assert(mac->gtsT, "Gts timer should be set");
                       char* gtsSlotInfo
                                = (char*)MESSAGE_ReturnInfo(
                                                mac->gtsT,
                                                INFO_TYPE_Gts_Slot_Start);
                       if (*gtsSlotInfo == mac->gtsSpec.slotStart[i - 1])
                       {
                            mac->gtsSpec.expiraryCount[i]
                                = Mac802_15_4GetMaxExpiraryCount(mac);
                       }
                    }
                }
            }
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4RecvData
// LAYER      :: MAC
// PURPOSE    :: To process an incoming MAC command
// PARAMETERS ::
// + node           : Node*             : Pointer to node
// + interfaceIndex : Int32               : Interface index on MAC
// + msg            : Message*          : Incoming message
// RETURN     :: None
// **/
static
void Mac802_15_4RecvData(Node* node,
                         Int32 interfaceIndex,
                         Message* msg)
{
    MacData802_15_4* mac = NULL;
    M802_15_4Header* wph = NULL;
    UInt8 ifs = 0;
    M802_15_4FrameCtrl frmCtrl;

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;
    wph = (M802_15_4Header*) MESSAGE_ReturnPacket(msg);

    // pass the data packet to upper layer
    ERROR_Assert(mac->rxData == FALSE, "mac->rxData should be non NULL");
    mac->rxData = msg;
    frmCtrl.frmCtrl = wph->MHR_FrmCtrl;
    Mac802_15_4FrameCtrlParse(&frmCtrl);

    if (!Sscs802_15_4IsDeviceUp(node, interfaceIndex))
    {
        // Drop the data packets if the node is not Up.
        if (DEBUG)
        {
             if (wph->MHR_DstAddrInfo.addr_64 == 0xffff)
             {
                 printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                        " The Zigbee Node is not up. Dropping received "
                        " broadcast packet\n",
                        node->getNodeTime(),
                        node->nodeId);
             }
             else
             {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                       " The Zigbee Node is not up. Dropping received "
                       " Unicast packet\n",
                       node->getNodeTime(),
                       node->nodeId);
             }
        }
        MESSAGE_Free(node,mac->rxData);
        mac->rxData = NULL;
        mac->stats.numPktDropped++;
        return;
    }

    if (wph->MHR_DstAddrInfo.addr_64 == 0xffff)
    {
        // Received a broadcast packet
        if (!mac->isCoor
                && wph->MHR_SrcAddrInfo.addr_16
                != mac->mpib.macCoordShortAddress )
        {
            // Do not handoff broadcasts received from devices other than
            // its coordinator

            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       " Is RFD, Dropping broadcast not from its"
                       " coordinator\n",
                       node->getNodeTime(),
                       node->nodeId);
            }
            MESSAGE_Free(node,mac->rxData);
            mac->rxData = NULL;
            mac->stats.numPktDropped++;
            return;
        }
        if (DEBUG)
        {
            printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                   " Received Data packet from"
                   " Node %d with DSN %d, Size %d bytes\n",
                   node->getNodeTime(),
                   node->nodeId,
                   wph->MHR_SrcAddrInfo.addr_64,
                   wph->MHR_BDSN,
                   MESSAGE_ReturnPacketSize(mac->rxData));
        }
        mac->stats.numDataPktRecd++;
        mac->rxDataTime = node->getNodeTime();
        if (DEBUG)
        {
            printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                   " Received Broadcast\n",
                   node->getNodeTime(),
                   node->nodeId);
        }

        // cancel the broadcast timer and check if poll is pending.
        if (mac->broadcastT)
        {
           // Stop the broadcast timer if broadcast is received.
           MESSAGE_CancelSelfMsg(node,mac->broadcastT);
           mac->broadcastT = NULL;
        }

        // check if poll request is pending, if pending, poll the cordinator
        // after IFS.

        if (mac->isPollRequestPending)
        {
            // set the IFS timer.
            // ifsTimerCalledAfterReceivingBeacon is set to handle polling
            // in IFS handler

            mac->ifsTimerCalledAfterReceivingBeacon = TRUE;
            if (MESSAGE_ReturnPacketSize(mac->rxData)
                    <= aMaxSIFSFrameSize)
            {
                ifs = aMinSIFSPeriod;
            }
            else
            {
                ifs = aMinLIFSPeriod;
            }
            mac->IFST =
                Mac802_15_4SetTimer(
                node,
                mac,
                M802_15_4IFSTIMER,
                ifs * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                NULL);
        }

        // handoff the broadcast packet to the upper layer
        Mac802_15_4MCPS_DATA_indication(node,
                                        interfaceIndex,
                                        frmCtrl.srcAddrMode,
                                        wph->MHR_SrcAddrInfo.panID,
                                        wph->MHR_SrcAddrInfo.addr_64,
                                        frmCtrl.dstAddrMode,
                                        wph->MHR_DstAddrInfo.panID,
                                        wph->MHR_DstAddrInfo.addr_64,
                                        MESSAGE_ReturnPacketSize(mac->rxData),
                                        mac->rxData,
                                        0,
                                        FALSE,
                                        0);
        mac->rxData = NULL;
        return;
    }
    if (DEBUG)
    {
        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
               " Received Data packet from "
               "Node %d with DSN %d, Size %d bytes\n",
               node->getNodeTime(),
               node->nodeId,
               wph->MHR_SrcAddrInfo.addr_64,
               wph->MHR_BDSN,
               MESSAGE_ReturnPacketSize(mac->rxData));
    }

    mac->stats.numDataPktRecd++;
    mac->rxDataTime = node->getNodeTime();

    Mac802_15_4UpdateGtsChracteristics(node,
                                       interfaceIndex,
                                       mac,
                                       mac->rxData,
                                       wph);

    if (!frmCtrl.ackReq)
    {
        if (MESSAGE_ReturnPacketSize(mac->rxData) <= aMaxSIFSFrameSize)
        {
            ifs = aMinSIFSPeriod;
        }
        else
        {
            ifs = aMinLIFSPeriod;
        }
         mac->IFST =
            Mac802_15_4SetTimer(
                node,
                mac,
                M802_15_4IFSTIMER,
                ifs * SECOND
                  / Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]),
                NULL);
    }
}

// /**
// NAME         Mac802_15_4QueuePackets
// PURPOSE      Queues data packets in MAC queues
// PARAMETERS   Node* node
//                  Node which sends the message
//              MacData_802_15_4* mac
//                  802.15.4 data structure
// RETURN       None
// NOTES        None
// **/
static
void Mac802_15_4QueuePackets(Node* node,
                             MacData802_15_4* mac)
{
    Int32 networkType = 0;
    TosType priority = 0;
    Message* msg = NULL;
    MACADDR DstAddr;
    MacHWAddress destHWAddr;
    Int32 interfaceIndex = mac->myMacData->interfaceIndex;
    while (TRUE)
    {
       BOOL topPacketExists =
                    MAC_OutputQueueTopPacket(node,
                                             interfaceIndex,
                                             &msg,
                                             &destHWAddr,
                                             &networkType,
                                             &priority);
        if (topPacketExists)
        {
            DstAddr
                = MAC_VariableHWAddressToTwoByteMacAddress(node,
                                                           &destHWAddr); 
            ZigbeeAppInfo* zigbeeAppInfo
                 = (ZigbeeAppInfo*)MESSAGE_ReturnInfo(
                                                   msg,
                                                   INFO_TYPE_ZigbeeApp_Info);
           if (zigbeeAppInfo
                && zigbeeAppInfo->priority > mac->mpib.macGtsTriggerPrecedence
                && (mac->isPANCoor || mac->isCoor || mac->isCoorInDeviceMode))
            {
                // GTS priority packet
                // check for valid GTS

                M802_15_4GTSSpec* gtsSpecsToCheck = NULL;;
                UInt8 gtsDirectionToCheck = 0;
                MACADDR addressToCheck = 0;
                if (mac->isPANCoor)
                {
                    gtsSpecsToCheck = &mac->gtsSpec;
                    gtsDirectionToCheck = 1;
                    addressToCheck = DstAddr;
                }
                else if (mac->isCoor || mac->isCoorInDeviceMode)
                {
                    gtsSpecsToCheck = &mac->gtsSpec2;
                    gtsDirectionToCheck = 0;
                    addressToCheck = mac->mpib.macCoordShortAddress;
                }
                else
                {
                    // RFD device
                    // only CAP will be used.

                    ERROR_Assert(FALSE, "GTS should not be used by an RFD");
                }
                BOOL hasValidGtsDescriptor = FALSE;
                UInt8 i =0;
                for (i = 0; i <= gtsSpecsToCheck->count; i++)
                {
                   if (addressToCheck 
                       == (((gtsSpecsToCheck)->fields).list[i]).devAddr16)
                   {
                       if ((gtsSpecsToCheck)->recvOnly[i] 
                                                == gtsDirectionToCheck)
                       {
                            hasValidGtsDescriptor = TRUE;
                            break; // break out of for loop
                       }
                   }
                }// for

                if (hasValidGtsDescriptor)
                {
                    ERROR_Assert(gtsSpecsToCheck->queue[i],"GTS queue"
                        " should be non NULL");
                    BOOL isFull = FALSE;
                    if (!gtsSpecsToCheck->appPort[i])
                    {
                        gtsSpecsToCheck->appPort[i] = zigbeeAppInfo->srcPort;
                    }
                    if (gtsSpecsToCheck->appPort[i]
                                             == zigbeeAppInfo->srcPort
                       && !mac->sendGtsConfirmationPending)
                    {
                        if (gtsSpecsToCheck->queue[i]->freeSpaceInQueue()
                            > MESSAGE_ReturnPacketSize(msg))
                        {
                            MAC_OutputQueueDequeuePacket(node,
                                                         interfaceIndex,
                                                         &msg,
                                                         &destHWAddr,
                                                         &networkType,
                                                         &priority);
                            gtsSpecsToCheck->queue[i]->insert(msg,
                                                              NULL,
                                                              &isFull,
                                                              0,
                                                              0,
                                                              0);
                            if (!isFull)
                            {
                                mac->stats.numDataPktsQueuedForGts++;
                            }
                            else
                            {
                                ERROR_Assert(FALSE," Mac Queue is full");
                            }
                        }
                        else
                        {
                            break;
                        }
                        if (!mac->isPANCoor)
                        {
                           if (gtsSpecsToCheck->endTime[i] == 0
                              || zigbeeAppInfo->endTime
                                            > gtsSpecsToCheck->endTime[i])
                           {
                               // schedule or reschedule the timer for
                               // deallocating the GTS

                               if (gtsSpecsToCheck->deAllocateTimer[i])
                               {
                                    MESSAGE_CancelSelfMsg(
                                       node,
                                       gtsSpecsToCheck->deAllocateTimer[i]);
                                    gtsSpecsToCheck->deAllocateTimer[i] = NULL;
                               }
                               clocktype delay = 0;
                               if (zigbeeAppInfo->endTime <= node->getNodeTime())
                               {

                                   // start Deallocation process
                                   gtsSpecsToCheck->endTime[i]
                                                    = node->getNodeTime();
                                   delay = 0;
                               }
                               else
                               {
                                    gtsSpecsToCheck->endTime[i]
                                        = zigbeeAppInfo->endTime;
                                   delay = zigbeeAppInfo->endTime 
                                            - node->getNodeTime();
                               }
                                Message* timerMsg = NULL;
                                timerMsg = MESSAGE_Alloc(node,
                                                   MAC_LAYER,
                                                   MAC_PROTOCOL_802_15_4,
                                                   MSG_MAC_TimerExpired);

                                MESSAGE_SetInstanceId(timerMsg, (short)
                                        mac->myMacData->interfaceIndex);

                                MESSAGE_InfoAlloc(node,
                                                  timerMsg,
                                                  sizeof(M802_15_4Timer));
                                M802_15_4Timer* timerInfo
                                    = (M802_15_4Timer*)
                                            MESSAGE_ReturnInfo(timerMsg);

                                timerInfo->timerType
                                            = M802_15_4GTSDEALLOCATETIMER;
                                gtsSpecsToCheck->deAllocateTimer[i]
                                    = timerMsg;
                                MESSAGE_Send(node, timerMsg, delay);
                           }
                        }
                    }
                    else
                    {
                        // send in CAP
                        if (mac->capQueue->freeSpaceInQueue()
                            > MESSAGE_ReturnPacketSize(msg))
                        {
                            MAC_OutputQueueDequeuePacket(node,
                                                         interfaceIndex,
                                                         &msg,
                                                         &destHWAddr,
                                                         &networkType,
                                                         &priority);
                            mac->capQueue->insert(msg, NULL, &isFull, 0, 0, 0);
                            if (!isFull)
                            {
                                mac->stats.numDataPktsQueuedForCap++;
                            }
                            else
                            {
                                ERROR_Assert(FALSE," MAC queue is full");
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else
                {
                    // valid gts descriptor not found
                    // Use CAP for data transfer

                    if (mac->capQueue->freeSpaceInQueue()
                            > MESSAGE_ReturnPacketSize(msg))
                    {
                        // send GTS request also after transferring the 
                        // packet in CAP

                        NodeAddress thisInterfaceAddress
                            = NetworkIpGetInterfaceAddress(node,
                                                           interfaceIndex);
                        if (gtsSpecsToCheck->permit
                            && (mac->isCoor || mac->isCoorInDeviceMode)
                            && DstAddr == mac->mpib.macCoordShortAddress
                            && thisInterfaceAddress == zigbeeAppInfo->srcAddress
                            && zigbeeAppInfo->endTime > node->getNodeTime())
                        {
                            ERROR_Assert(mac->sfSpec2.PANCoor, "Parent"
                                " should be a Pan Coordinator");
                            if (!mac->sendGtsRequestToPancoord 
                                    && !mac->taskP.mlme_gts_request_STEP
                                    && !mac->gtsRequestExhausted 
                                    && !mac->sendGtsConfirmationPending)
                            {
                                mac->sendGtsRequestToPancoord = TRUE;
                                mac->gtsRequestData.allocationRequest = TRUE;
                                mac->gtsRequestData.receiveOnly = FALSE;
                                mac->gtsRequestData.numSlots
                                    = Mac802_15_4CalculateGtsLength(
                                                              node,
                                                              interfaceIndex,
                                                              mac,
                                                              zigbeeAppInfo,
                                                              msg,
                                                              FALSE);
                                mac->gtsRequestData.active = TRUE;
                            }
                        }

                        MAC_OutputQueueDequeuePacket(node,
                                                    interfaceIndex,
                                                    &msg,
                                                    &destHWAddr,
                                                    &networkType,
                                                    &priority);
                        BOOL isFull = FALSE;
                        mac->capQueue->insert(msg, NULL, &isFull, 0, 0, 0);
                        if (!isFull)
                        {
                            mac->stats.numDataPktsQueuedForCap++;
                        }
                        else
                        {
                            ERROR_Assert(FALSE," MAC queue is full");
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }// if
           else
           {
               // Use CAP for data transfer
               if (mac->capQueue->freeSpaceInQueue()
                            > MESSAGE_ReturnPacketSize(msg))
               {
                   MAC_OutputQueueDequeuePacket(node,
                                                interfaceIndex,
                                                &msg,
                                                &destHWAddr,
                                                &networkType,
                                                &priority);
                    BOOL isFull = FALSE;
                    mac->capQueue->insert(msg, NULL, &isFull, 0, 0, 0);
                    if (!isFull)
                    {
                        mac->stats.numDataPktsQueuedForCap++;
                    }
                    else
                    {
                        ERROR_Assert(FALSE," MAC queue is full");
                    }
               }
               else
               {
                   break;
               }
           }
       }
       else
       {
            break; // break out of while loop
       }
    } // while
}

// /**
// NAME         Mac802_15_4SendPacketsInCap
// PURPOSE      Send a packet during CAP
// PARAMETERS   Node* node
//                  Node which sends the message
//              Int32 interfaceIndex
//                  interfaceindex of the node.
//              MacData_802_15_4* mac
//                  802.15.4 data structure
// RETURN       None
// NOTES        None
// **/
static
void Mac802_15_4SendPacketsInCap(Node* node,
                                 Int32 interfaceIndex,
                                 MacData802_15_4* mac)
{
    MACADDR DstAddr;
    MacHWAddress destHWAddr;
    Message* msg = NULL;
    mac->capQueue->retrieve(&msg,
                            DEQUEUE_NEXT_PACKET,
                            DEQUEUE_PACKET,
                            0,
                            NULL);
    mac->stats.numDataPktsDeQueuedForCap++;
    QueuedPacketInfo* infoPtr = NULL;
    infoPtr = (QueuedPacketInfo *)MESSAGE_ReturnInfo(msg);
    destHWAddr.hwLength = infoPtr->hwLength;
    destHWAddr.hwType = infoPtr->hwType;

    // Added to avoid double memory allocation and hence memory leak
    if (destHWAddr.byte == NULL)
    {
        destHWAddr.byte = (unsigned char*) MEM_malloc(
                      sizeof(unsigned char)*infoPtr->hwLength);
    }
    memcpy(destHWAddr.byte,infoPtr->macAddress,infoPtr->hwLength);
    DstAddr = MAC_VariableHWAddressToTwoByteMacAddress(node,
                                                       &destHWAddr);
    if (!mac->capability.FFD)
    {
        if (DstAddr == 0xffff)
        {
            DstAddr = mac->mpib.macCoordExtendedAddress;
        }
        else if (DstAddr != mac->mpib.macCoordExtendedAddress)
        {
            // Notify upper layer if node has lossed Synchronization with CO.
            mac->stats.numPktDropped++;
            MAC_NotificationOfPacketDrop(node,
                                         destHWAddr,
                                         interfaceIndex,
                                         msg);
            return;
        }
    }

    if (mac->isCoor
        && (mac->mpib.macCoordShortAddress
            != DstAddr) && (DstAddr != 0xffff)
        && mac->mpib.macCoordExtendedAddress != DstAddr)
    {
        // check if the destination is one of my child.
         if (Mac802_15_4UpdateDeviceLink(OPER_CHECK_DEVICE_REFERENCE,
                                         &mac->deviceLink1,
                                         &mac->deviceLink2,
                                         DstAddr))
         {
             // Destination not my child, send data directly.
             Mac802_15_4MCPS_DATA_request(node,
                                         interfaceIndex,
                                         M802_15_4DEFFRMCTRL_ADDRMODE16,
                                         mac->mpib.macPANId,
                                         (UInt16)node->nodeId,
                                         M802_15_4DEFFRMCTRL_ADDRMODE16,
                                         mac->mpib.macPANId,
                                         DstAddr,
                                         MESSAGE_ReturnPacketSize(msg),
                                         msg,
                                         0,
                                         0);
         }
         else
         {
             // Destination is my child, send data in-directly.
             Mac802_15_4MCPS_DATA_request(node,
                                         interfaceIndex,
                                         M802_15_4DEFFRMCTRL_ADDRMODE16,
                                         mac->mpib.macPANId,
                                         (UInt16)node->nodeId,
                                         M802_15_4DEFFRMCTRL_ADDRMODE16,
                                         mac->mpib.macPANId,
                                         DstAddr,
                                         MESSAGE_ReturnPacketSize(msg),
                                         msg,
                                         0,
                                         TxOp_Indirect);
         }
    }
    else if (mac->isCoor && DstAddr == 0xffff)
    {
        // Broadcast data
        if (DEBUG)
        {
            printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                   "Broadcast data Enqueued\n",
                   node->getNodeTime(),
                   node->nodeId);
        }
        if (mac->mpib.macBeaconOrder != 15)
        {
            // Beacon enabled PAN.
            // broadcast packet will be enqueued and sent just after sending beacon
            // childs will be notified through beacon about the incomming
            // broadcast packet via framepending field in beacon frame.

            BOOL isFull = FALSE;
            mac->broadCastQueue->insert(msg, NULL, &isFull, 0, 0, 0);
            if (isFull)
            {
                // If queue is full, create space for the incoming packet by
                // dropping the packets from the front of the broadcast Queue.

                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                           " Queue is full\n",
                           node->getNodeTime(),
                           node->nodeId);
                }
                Int32 incomingMsgSize = MESSAGE_ReturnPacketSize(msg);
                Message* newMsg = NULL;
                BOOL isMsgRetrieved = FALSE;

                while (incomingMsgSize > 0)
                {
                     newMsg = NULL;
                     isMsgRetrieved = FALSE;
                     isMsgRetrieved =  mac->broadCastQueue->retrieve(
                                                        &newMsg,
                                                        0,
                                                        DEQUEUE_PACKET,
                                                        node->getNodeTime());
                     if (isMsgRetrieved)
                     {
                        incomingMsgSize -= MESSAGE_ReturnPacketSize(newMsg);
                         mac->stats.numPktDropped++;
                         MESSAGE_Free(node, newMsg);
                         newMsg = NULL;
                         if (DEBUG)
                         {
                             printf("%" TYPES_64BITFMT "d : Node %d: "
                                    "802.15.4MAC : Queue"
                                    " is full, dropping data packet from the"
                                    " front of the broadcast queue\n",
                                    node->getNodeTime(),
                                    node->nodeId);
                         }
                     }
                     else
                     {
                        break;
                     }
                }// end of while

                // Try to insert again after creating space for the msg.
                mac->broadCastQueue->insert(msg, NULL, &isFull, 0, 0, 0);
                if (isFull)
                {
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: "
                               "802.15.4MAC : Queue is"
                               " still full, dropping incoming broadcast"
                               " packet\n",
                               node->getNodeTime(),
                               node->nodeId);
                    }
                    mac->stats.numPktDropped++;
                    MESSAGE_Free(node, msg);
                    msg = NULL;
                }
            }
        }
        else
        {
            // Non-beacon mode.
            // Broadcast data will be sent directly.
            // No notification of broadcasts via beacons

            Mac802_15_4MCPS_DATA_request(node,
                                         interfaceIndex,
                                         M802_15_4DEFFRMCTRL_ADDRMODE16,
                                         mac->mpib.macPANId,
                                         (UInt16)node->nodeId,
                                         M802_15_4DEFFRMCTRL_ADDRMODE16,
                                         mac->mpib.macPANId,
                                         DstAddr,
                                         MESSAGE_ReturnPacketSize(msg),
                                         msg,
                                         0,
                                         0);
        }
    }
    else
    {
        // Non-beacon mode or
        // coordinator/device sending data to its parent

        Mac802_15_4MCPS_DATA_request(node,
                                     interfaceIndex,
                                     M802_15_4DEFFRMCTRL_ADDRMODE16,
                                     mac->mpib.macPANId,
                                     (UInt16)node->nodeId,
                                     M802_15_4DEFFRMCTRL_ADDRMODE16,
                                     mac->mpib.macPANId,
                                     DstAddr,
                                     MESSAGE_ReturnPacketSize(msg),
                                     msg,
                                     0,
                                     0);
    }
}

// /**
// NAME         Mac802_15_4SendPacketsInGts
// PURPOSE      Send a packet during GTS
// PARAMETERS   Node* node
//                  Node which sends the message
//              Int32 interfaceIndex
//                  interfaceindex of the node.
//              MacData_802_15_4* mac
//                  802.15.4 data structure
//              M802_15_4GTSSpec* gtsSpecsToCheck
//                  gtsSpecification at the node
//              clocktype referenceBeaconTime
//                  last beacon Tx or Rx time
//              clocktype sSlotDuration
//                  slot duration
// RETURN       None
// NOTES        None
// **/
static
void Mac802_15_4SendPacketsInGts(Node* node,
                                 Int32 interfaceIndex,
                                 MacData802_15_4* mac,
                                 M802_15_4GTSSpec* gtsSpecsToCheck,
                                 clocktype referenceBeaconTime,
                                 clocktype sSlotDuration)
{
    clocktype currentTime = node->getNodeTime();
    Int32 i = 0;
    for (i = 0; i < gtsSpecsToCheck->count; i++)
    {
        // Check if we are in the allocated GTS time.
        if ((currentTime >= referenceBeaconTime
                + (gtsSpecsToCheck->slotStart[i] * sSlotDuration))
            && (currentTime < referenceBeaconTime
                + ((gtsSpecsToCheck->slotStart[i]
                + gtsSpecsToCheck->length[i]) * sSlotDuration)))
        {
            if (mac->backoffStatus == BACKOFF)
            {
                // Cancel all backoff procedures
                mac->taskP.mcps_data_request_Last_TxOptions
                    = mac->taskP.mcps_data_request_TxOptions;
                mac->backoffStatus = BACKOFF_RESET;
                Csma802_15_4Cancel(node, interfaceIndex);
            }
            if (gtsSpecsToCheck->msg[i])
            {
                // check if packet can be sent in GTS
                if (Mac802_15_4CanProceedInGts(node,
                                               interfaceIndex,
                                               mac,
                                               i,
                                               gtsSpecsToCheck->msg[i]))
                {
                    // We can send the packet "gtsSpecsToCheck->msg[i]"
                    // during GTS

                    mac->currentGtsPositionDesc = i;
                    ERROR_Assert(!mac->txGts,"GTS timer should not be set");
                    ERROR_Assert(!mac->taskP.mcps_gts_data_request_STEP,
                                 "GTS data request step should be zero");
                    mac->taskP.mcps_gts_data_request_STEP = GTS_PKT_RETRY_STEP;
                    mac->taskP.mcps_data_request_TxOptions = TxOp_GTS;
                    
                    // Send the packet during GTS
                    Mac802_15_4MCPS_DATA_request(
                          node,
                          interfaceIndex,
                          M802_15_4DEFFRMCTRL_ADDRMODE16,
                          mac->mpib.macPANId,
                          (UInt16)node->nodeId,
                          M802_15_4DEFFRMCTRL_ADDRMODE16,
                          mac->mpib.macPANId,
                          gtsSpecsToCheck->fields.list[i].devAddr16,
                          MESSAGE_ReturnPacketSize(gtsSpecsToCheck->msg[i]),
                          gtsSpecsToCheck->msg[i],
                          0,
                          TxOp_GTS);
                }
            }
            else if (!gtsSpecsToCheck->queue[i]->isEmpty())
            {
                // Retreive the packet from the GTS queue
                Message* msg = NULL;
                gtsSpecsToCheck->queue[i]->retrieve(&msg,
                                                    DEQUEUE_NEXT_PACKET,
                                                    PEEK_AT_NEXT_PACKET,
                                                    0,
                                                    NULL);

                // Check if the packet can be sent during GTS
                if (Mac802_15_4CanProceedInGts(node,
                                               interfaceIndex,
                                               mac,
                                               i,
                                               msg))
                {
                    // We can proceed during this GTS
                    ERROR_Assert(!gtsSpecsToCheck->msg[i],"gtsSpecsToCheck->msg[i]"
                                 " should be NULL");
                    gtsSpecsToCheck->queue[i]->retrieve(
                                                &(gtsSpecsToCheck->msg[i]),
                                                 DEQUEUE_NEXT_PACKET,
                                                 DEQUEUE_PACKET,
                                                 0,
                                                 NULL);
                    mac->stats.numDataPktsDeQueuedForGts++;
                    mac->currentGtsPositionDesc = i;
                    ERROR_Assert(!mac->txGts,"mac->txGts should be NULL");
                    ERROR_Assert(!mac->taskP.mcps_gts_data_request_STEP,
                                 "GTS data request step should be zero");
                    Mac802_15_4MCPS_DATA_request(
                          node,
                          interfaceIndex,
                          M802_15_4DEFFRMCTRL_ADDRMODE16,
                          mac->mpib.macPANId,
                          (UInt16)node->nodeId,
                          M802_15_4DEFFRMCTRL_ADDRMODE16,
                          mac->mpib.macPANId,
                          gtsSpecsToCheck->fields.list[i].devAddr16,
                          MESSAGE_ReturnPacketSize(gtsSpecsToCheck->msg[i]),
                          gtsSpecsToCheck->msg[i],
                          0,
                          TxOp_GTS);
                }
            }
            break;
        }
    } // for
}
// /**
// NAME         Mac802_15_4NetworkLayerHasPacketToSend
// PURPOSE      To notify 802.15.4 that network has something to send
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacData_802_15_4* mac
//                  802.15.4 data structure
// RETURN       None
// NOTES        None
// **/
void Mac802_15_4NetworkLayerHasPacketToSend(Node* node, MacData802_15_4* mac)
{
    Int32 networkType = 0;
    TosType priority;
    Message* msg = NULL;
    MACADDR DstAddr;
    MacHWAddress destHWAddr;
    Int32 interfaceIndex = mac->myMacData->interfaceIndex;

    if (mac->isSyncLoss || !Sscs802_15_4IsDeviceUp(node, interfaceIndex))
    {
        BOOL dequeued = MAC_OutputQueueDequeuePacket(node,
                                                     interfaceIndex,
                                                     &msg,
                                                     &destHWAddr,
                                                     &networkType,
                                                     &priority);
        if (dequeued)
        {
        if (DEBUG)
        {
            if (mac->isSyncLoss)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                        "Dropping Data received"
                        " from upper layer. The device is "
                        "not up due to Synchronization loss\n",
                        node->getNodeTime(), node->nodeId);
            }
            else
            {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                            "Dropping Data received"
                            " from upper layer. The device is "
                            "not up\n",
                            node->getNodeTime(), node->nodeId);

            }
        }
              DstAddr
                  = MAC_VariableHWAddressToTwoByteMacAddress(node,
                                                             &destHWAddr);

        // Send notification of packet drop to network layer only for unicast
        // packets, for broadcast packets, increment the 'numPktDropped'
        // variable and drop the packets silently

        if (DstAddr != 0xffff)
        {
            MAC_NotificationOfPacketDrop(node,
                                         destHWAddr,
                                         interfaceIndex,
                                         msg);
        }
        else
        {
            MESSAGE_Free(node, msg);
            msg = NULL;
        }
        mac->stats.numPktDropped++;
        }// if (dequeued)
        return;
    } // if (mac->isSyncLoss || !Sscs802_15_4IsDeviceUp(node, interfaceIndex))

    // Queue packets in the MAC queue
    Mac802_15_4QueuePackets(node, mac);

    BOOL sendPacketInCap = FALSE;
   if ((mac->mpib.macBeaconOrder != 15) ||(mac->macBeaconOrder2 != 15))
    {
       clocktype currentTime = node->getNodeTime();
       M802_15_4SuperframeSpec sfSpecsToCheck;
       clocktype referenceBeaconTime = 0;
       M802_15_4GTSSpec* gtsSpecsToCheck = NULL;
       Int32 rate = Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
       clocktype sSlotDuration = 0;
       if (mac->isPANCoor)
        {
           sfSpecsToCheck = mac->sfSpec;
           referenceBeaconTime = mac->macBcnTxTime;
           gtsSpecsToCheck = &mac->gtsSpec;
        }
        else if (mac->isCoor || mac->isCoorInDeviceMode)
        {
            // FFD communicating with parent
            sfSpecsToCheck = mac->sfSpec2;
            referenceBeaconTime = mac->macBcnRxTime;
            gtsSpecsToCheck = &mac->gtsSpec2;
             clocktype BI = 
                 ((aBaseSuperframeDuration 
                        * (1 << sfSpecsToCheck.BO)) * SECOND) / rate;
            while (referenceBeaconTime + BI < currentTime)
            {
                referenceBeaconTime += BI;
            }
         }
         else
         {
             // RFD
            sfSpecsToCheck = mac->sfSpec2;
            referenceBeaconTime = mac->macBcnRxTime;
            gtsSpecsToCheck = &mac->gtsSpec2;

            clocktype BI
                    = ((aBaseSuperframeDuration
                            * (1 << sfSpecsToCheck.BO)) * SECOND) / rate;
             while (referenceBeaconTime + BI < currentTime)
            {
                 referenceBeaconTime += BI;
            }
        }
        sSlotDuration = (sfSpecsToCheck.sd * SECOND) / rate;
        if (currentTime < (referenceBeaconTime 
                        + ((sfSpecsToCheck.FinCAP + 1) * sSlotDuration)))
        {
            // we are in CAP
            // send CAP packets

            if (!mac->capQueue->isEmpty())
            {
                // send packet
                sendPacketInCap = TRUE;
            }
        }
        else if (currentTime >= (referenceBeaconTime
                            + ((sfSpecsToCheck.FinCAP + 1) * sSlotDuration))
                && currentTime < (referenceBeaconTime
                    + (aNumSuperframeSlots * sSlotDuration)))
        {
            // we are in CFP
            if (mac->taskP.mcps_gts_data_request_STEP)
            {
                // Mac is currently handling a GTS Request. Defer sending
                // GTS packet for now.

                return;
            }
            Mac802_15_4SendPacketsInGts(node,
                                        interfaceIndex,
                                        mac,
                                        gtsSpecsToCheck,
                                        referenceBeaconTime,
                                        sSlotDuration);
        }
        else
        {
            // we could be in inactive period of superframe
            if (sfSpecsToCheck.BO == sfSpecsToCheck.SO)
            {
                 ERROR_Assert(FALSE,"Illegal operation. With BO == SO"
                             " There should not be any in-active period");
            }
        }
    }
    else
    {
        // send packet
        if (!mac->capQueue->isEmpty())
        {
            // send packet
            sendPacketInCap = TRUE;
        }
    }

    if (sendPacketInCap)
    {
        // Since, one packet is handled at a time, it is necessary to stop any
        // other event which can interfere with the current event.

        if (mac->taskP.mcps_data_request_STEP
            || mac->taskP.mlme_poll_request_STEP || mac->backoffStatus == 99
            || mac->taskP.mcps_broadcast_request_STEP)
        {
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC : "
                        "Not sending data "
                        "in CAP. Currently "
                        "proceesing another request\n",
                        node->getNodeTime(), node->nodeId);
            }
            return;
        }
        Mac802_15_4SendPacketsInCap(node,
                                    interfaceIndex,
                                    mac);
    }
}


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
                                     MacData802_15_4* mac,
                                     Message* msg)
{
    M802_15_4Header* wph = NULL;
    M802_15_4FrameCtrl frmCtrl;
    M802_15_4SuperframeSpec t_sfSpec;
    BOOL noAck = FALSE;
    Int32 interfaceIndex = mac->myMacData->interfaceIndex;
    UInt16* headerIterator = (UInt16*) MESSAGE_ReturnPacket(msg);
    frmCtrl.frmCtrl = *headerIterator;
    Mac802_15_4FrameCtrlParse(&frmCtrl);
    if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_ACK)
    {
        if (mac->taskP.mlme_scan_request_STEP)
        {
            mac->stats.numPktDropped++;
            MESSAGE_Free(node, msg);
            msg = NULL;
            return;
        }
        Mac802_15_4Reset_TRX(node, interfaceIndex);
        Mac802_15_4RecvAck(node, interfaceIndex, msg);
        return;
    }

    wph = (M802_15_4Header*) MESSAGE_ReturnPacket(msg);
    MacHWAddress  destHWAddress;
    ConvertMacAddrToVariableHWAddress(node,
                                      &destHWAddress,
                                      &wph->MHR_DstAddrInfo.addr_64);

    if (!(MAC_IsMyAddress(node, &destHWAddress)
            || MAC_IsBroadcastHWAddress(&destHWAddress)))
    {
        if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_DATA)
        {
            // peek data packets only
            if (node->macData[interfaceIndex]->promiscuousMode)
            {
                MacHWAddress prevHopHWAddr;
                ConvertMacAddrToVariableHWAddress(node,
                                                  &prevHopHWAddr,
                                                  &wph->MHR_SrcAddrInfo.addr_64);
                MESSAGE_RemoveHeader(node,
                                     msg,
                                     sizeof(M802_15_4Header),
                                     TRACE_MAC_802_15_4);
                MAC_SneakPeekAtMacPacket(node,
                                interfaceIndex,
                                 msg,
                                 prevHopHWAddr,
                                 destHWAddress);
            }
        }
        MESSAGE_Free(node, msg);
        return;
    }

    if (mac->taskP.mlme_scan_request_STEP)
    {
        if (mac->taskP.mlme_scan_request_ScanType == 0x00)       // ED scan
        {
            mac->stats.numPktDropped++;
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       " Dropped Packets during ED scan\n",
                       node->getNodeTime(), node->nodeId);
            }
            MESSAGE_Free(node, msg);
            return;
        }
        else if (((mac->taskP.mlme_scan_request_ScanType == 0x01)   // Active
                ||(mac->taskP.mlme_scan_request_ScanType == 0x02)) // Passive
                && (frmCtrl.frmType != M802_15_4DEFFRMCTRL_TYPE_BEACON))
        {
            mac->stats.numPktDropped++;
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                        " Dropped Packets "
                        "during Active/Passive scan that is not Beacon\n",
                        node->getNodeTime(), node->nodeId);
            }
            MESSAGE_Free(node, msg);
            return;
        }
        else if ((mac->taskP.mlme_scan_request_ScanType == 0x03) // Orphan
                  && ((frmCtrl.frmType != M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                    || (wph->MSDU_CmdType != 0x08)))
        {
            mac->stats.numPktDropped++;
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                        " Dropped Packets "
                        "during Orphan scan that is not Coordinator "
                        "realignment\n",
                        node->getNodeTime(), node->nodeId);
            }
            MESSAGE_Free(node, msg);
            return;
        }
    }
    if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_BEACON)
    {
        M802_15_4BeaconFrame* bcnFrm = (M802_15_4BeaconFrame*)((char*)wph
                                            + sizeof(M802_15_4Header));
        t_sfSpec.superSpec = bcnFrm->MSDU_SuperSpec;
        Mac802_15_4SuperFrameParse(&t_sfSpec);
        if (t_sfSpec.BO != 15)
        {
            // update superframe specification
            mac->sfSpec3 = t_sfSpec;

            // calculate the time when we received the first bit of the beacon
            mac->macBcnOtherRxTime
                = node->getNodeTime()
                    - Mac802_15_4TrxTime(node, interfaceIndex, msg);

            // update beacon order and superframe order
            mac->macBeaconOrder3 = mac->sfSpec3.BO;
            mac->macSuperframeOrder3 = mac->sfSpec3.SO;
        }
    }

    // perform filtering if the PAN is currently not in promiscuous mode
    if (!mac->mpib.macPromiscuousMode)
    {
        // check packet type
        if ((frmCtrl.frmType != M802_15_4DEFFRMCTRL_TYPE_BEACON)
             && (frmCtrl.frmType != M802_15_4DEFFRMCTRL_TYPE_DATA)
             && (frmCtrl.frmType != M802_15_4DEFFRMCTRL_TYPE_ACK)
             && (frmCtrl.frmType != M802_15_4DEFFRMCTRL_TYPE_MACCMD))
        {
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       " Dropped Packets "
                       "of unknown type\n",
                       node->getNodeTime(), node->nodeId);
            }
            MESSAGE_Free(node, msg);
            return;
        }

        // check source PAN ID for beacon frame
        if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_BEACON)
             && (mac->mpib.macPANId != 0xffff)
             && (wph->MHR_SrcAddrInfo.panID != mac->mpib.macPANId))
        {
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       "Ignoring Beacon "
                       "from unknown PAN\n",
                       node->getNodeTime(), node->nodeId);
            }
            MESSAGE_Free(node, msg);
            return;
        }
        // check dest. PAN ID if it is included
        if (!frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_DATA)
        {
            if ((frmCtrl.dstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE16)
                 || (frmCtrl.dstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE64))
            {
                if ((wph->MHR_DstAddrInfo.panID != 0xffff)
                     && (wph->MHR_DstAddrInfo.panID != mac->mpib.macPANId))
                {
                    mac->stats.numPktDropped++;
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d Node %d: 802.15.4MAC :"
                                " Dropped Packet, "
                                "Destination PAN ID not valid. PANID is %d\n",
                                node->getNodeTime(), node->nodeId,
                                mac->mpib.macPANId);
                    }
                    MESSAGE_Free(node, msg);
                    return;
                }
            }
        }

        // check dest. address if it is included
        if (frmCtrl.dstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE16)
        {
            if ((wph->MHR_DstAddrInfo.addr_16 != 0xffff)
                 && (wph->MHR_DstAddrInfo.addr_16 !=
                        mac->mpib.macShortAddress))
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           " Discarding Packet"
                           ", not meant for this node\n",
                           node->getNodeTime(), node->nodeId);
                }
                MESSAGE_Free(node, msg);
                return;
            }

        }
        else if ((frmCtrl.dstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODE64)
                || (frmCtrl.dstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODENONE))
        {
            if (wph->MHR_DstAddrInfo.addr_64 != mac->aExtendedAddress)
            {
                if (DEBUG)
                {
                    printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                           " Discarding Packet"
                           ", not meant for this node\n",
                           node->getNodeTime(), node->nodeId);
                }
                MESSAGE_Free(node, msg);
                return;
            }
        }

        // check for Data/MacCmd frame only w/ source address
        if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_DATA)
             || (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD))
        {
            if (frmCtrl.dstAddrMode == M802_15_4DEFFRMCTRL_ADDRMODENONE)
            {
                if (((!mac->capability.FFD)
                       ||(Mac802_15_4NumberDeviceLink(&mac->deviceLink1) ==
                                                            0))  // not coord
                       ||(wph->MHR_SrcAddrInfo.panID != mac->mpib.macPANId))
                {
                    mac->stats.numPktDropped++;
                    if (DEBUG)
                    {
                        printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC"
                                ": Dropped Packet"
                                ", received from invalid source\n",
                                node->getNodeTime(), node->nodeId);
                    }
                    MESSAGE_Free(node, msg);
                    return;
                }
            }
        }
    }   // ---filtering done---

    if (frmCtrl.frmType != M802_15_4DEFFRMCTRL_TYPE_ACK 
       && (mac->waitDataAck || mac->waitBcnCmdAck || mac->waitBcnCmdAck2))
    {
        // Waiting for ack
        mac->stats.numPktDropped++;
        MESSAGE_Free(node, msg);
        return;
    }

    // send an acknowledgement if needed (no matter this is a duplicated
    // packet or not)
    if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_DATA)
         || (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD))
    {
        if (frmCtrl.ackReq) // acknowledgement required
        {
            if ((frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
                && (wph->MSDU_CmdType == 0x01))
            {
                if ((!mac->capability.FFD)           // not an FFD
                || (mac->mpib.macShortAddress == 0xffff)// not yet joined PAN
                || (!mac->mpib.macAssociationPermit))
                {
                    mac->stats.numPktDropped++;
                    MESSAGE_Free(node, msg);
                    return;
                }
            }

            noAck = FALSE;
            if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
            {
                if ((mac->rxCmd) || (mac->txBcnCmd))
                {
                    noAck = TRUE;
                }
            }
            if (!noAck)
            {
                Mac802_15_4ConstructACK(node, interfaceIndex, msg);

                // stop CSMA-CA if it is pending (it will be restored after
                // the transmission of ACK)

                if (mac->backoffStatus == BACKOFF)
                {
                    mac->backoffStatus = BACKOFF_RESET;
                    Csma802_15_4Cancel(node, interfaceIndex);
                }
                mac->trx_state_req = TX_ON;
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   TX_ON);
                BOOL toParent = FALSE;
                if (wph->MSDU_CmdType == 0x02
                    || wph->MSDU_CmdType == 0x08)
                {
                    toParent = TRUE;
                }
                Mac802_15_4PLME_SET_TRX_STATE_confirm(node,
                                                      interfaceIndex,
                                                      PHY_BUSY_TX,
                                                      toParent);
            }
        }
        else
        {
            // temporarily reset not done.
            // Mac802_15_4Reset_TRX(node, interfaceIndex);
        }
    }
    else
    {
       Mac802_15_4Reset_TRX(node, interfaceIndex);
    }


    if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
    {
        if ((mac->rxCmd) || (mac->txBcnCmd))
        {
            mac->stats.numPktDropped++;
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       " Dropped MAC command "
                       ", currently processing another\n",
                       node->getNodeTime(), node->nodeId);
            }
            MESSAGE_Free(node, msg);
            return;
        }
    }

    if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_DATA)
    {
        if (mac->rxData)
        {
            mac->stats.numPktDropped++;
            if (DEBUG)
            {
                printf("%" TYPES_64BITFMT "d : Node %d: 802.15.4MAC :"
                       " Dropped Data Packet "
                       ", currently processing another\n",
                       node->getNodeTime(), node->nodeId);
            }
            MESSAGE_Free(node, msg);
            return;
        }
    }

    // handle the beacon packet
    if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_BEACON)
    {
        Mac802_15_4RecvBeacon(node, interfaceIndex, msg);
    }

    // handle the command packet
    else if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_MACCMD)
    {
        Mac802_15_4RecvCommand(node, interfaceIndex, msg);
    }

    // handle the data packet
    else if (frmCtrl.frmType == M802_15_4DEFFRMCTRL_TYPE_DATA)
    {
        Mac802_15_4RecvData(node, interfaceIndex, msg);
    }
}


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
                                                MacData802_15_4* mac,
                                                PhyStatusType oldPhyStatus,
                                                PhyStatusType newPhyStatus,
                                                clocktype receiveDuration)
{
    Int32 interfaceIndex = mac->myMacData->interfaceIndex;

    // After calling PhyStartTransmittingSignal control is returned here
    Mac802_15_4PD_DATA_confirm(node, interfaceIndex, newPhyStatus);
}

// /**
// FUNCTION   :: Mac802_15_4PrintStats
// LAYER      :: MAC
// PURPOSE    :: Print out statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + interfaceIndex : Int32      : Interface index
// RETURN     :: void : NULL
// **/
static
void Mac802_15_4PrintStats(Node* node, Int32 interfaceIndex)
{
    MacData802_15_4* mac = NULL;
    char buf[MAX_STRING_LENGTH];

    mac = (MacData802_15_4*) node->macData[interfaceIndex]->macVar;

    sprintf(buf, "Number Of Data Packets Queued For CAP = %u",
            mac->stats.numDataPktsQueuedForCap);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

     sprintf(buf, "Number Of Data Packets De-Queued For CAP = %u",
             mac->stats.numDataPktsDeQueuedForCap);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Data Packets Sent In CAP = %u",
            mac->stats.numDataPktSent);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    if (mac->displayGtsStats)
    {
        sprintf(buf, "Number Of Data Packets Queued For GTS = %u",
                mac->stats.numDataPktsQueuedForGts);
        IO_PrintStat(node,
                     "MAC",
                     "MAC-802.15.4",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf, "Number Of Data Packets De-Queued For GTS = %u",
                 mac->stats.numDataPktsDeQueuedForGts);
        IO_PrintStat(node,
                     "MAC",
                     "MAC-802.15.4",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

         sprintf(buf, "Number Of Data Packets Sent In GTS = %u",
                mac->stats.numDataPktsSentInGts);
        IO_PrintStat(node,
                     "MAC",
                     "MAC-802.15.4",
                     ANY_DEST,
                     interfaceIndex,
                     buf);
    }


    sprintf(buf, "Number Of Data Packets Received = %u",
            mac->stats.numDataPktRecd);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Data Requests Sent = %u",
            mac->stats.numDataReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Data Requests Received = %u",
            mac->stats.numDataReqRecd);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    if (mac->displayGtsStats)
    {
        if (!mac->isPANCoor)
        {
            sprintf(buf, "Number Of GTS Allocation Requests Sent = %u",
                mac->stats.numGtsAllocationReqSent);
            IO_PrintStat(node,
                        "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            sprintf(buf, "Number Of GTS De-Allocation Requests Sent = %u",
                    mac->stats.numGtsDeAllocationReqSent);
            IO_PrintStat(node,
                         "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            sprintf(buf, "Number Of GTS Requests Retried = %u",
                    mac->stats.numGtsReqRetried);
            IO_PrintStat(node,
                         "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            sprintf(buf, "Number Of GTS Requests Rejected By PanCoord = %u",
                              mac->stats.numGtsRequestsRejectedByPanCoord);
            IO_PrintStat(node,
                         "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            sprintf(buf, "Number Of GTS Allocation Requests Confirmed By"
                         " PanCoord = %u",
                          mac->stats.numGtsAllocationConfirmedByPanCoord);
            IO_PrintStat(node,
                         "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);
        }
        else
        {
            sprintf(buf, "Number Of GTS Allocation Requests Received = %u",
                mac->stats.numGtsReqRecd);
            IO_PrintStat(node,
                         "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            sprintf(buf, "Number Of GTS De-Allocation Requests Received = %u",
                    mac->stats.numGtsDeAllocationReqRecd);
            IO_PrintStat(node,
                         "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            sprintf(buf, "Number Of GTS Requests Ignored = %u",
                        mac->stats.numGtsReqIgnored);
            IO_PrintStat(node,
                         "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            sprintf(buf, "Number Of GTS Requests Rejected = %u",
                          mac->stats.numGtsRequestsRejectedByPanCoord);
            IO_PrintStat(node,
                         "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            sprintf(buf, "Number Of GTS Expired = %u",
                          mac->stats.numGtsExpired);
            IO_PrintStat(node,
                         "MAC",
                         "MAC-802.15.4",
                         ANY_DEST,
                         interfaceIndex,
                         buf);
        }
    }

    sprintf(buf, "Number Of Ack Sent = %u",
            mac->stats.numAckSent);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Ack Received = %u",
            mac->stats.numAckRecd);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Associate Requests Sent = %u",
            mac->stats.numAssociateReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Associate Requests Received = %u",
            mac->stats.numAssociateReqRecd);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of Associate Response Sent = %u",
            mac->stats.numAssociateResSent);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Associate Response Received = %u",
            mac->stats.numAssociateResRecd);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Disassociate Requests Sent = %u",
            mac->stats.numDisassociateReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Disassociate Requests Received = %u",
            mac->stats.numDisassociateReqRecd);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Beacons Sent = %u",
            mac->stats.numBeaconsSent);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Beacons Received = %u",
            mac->stats.numBeaconsRecd);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Beacon Requests Sent = %u",
            mac->stats.numBeaconsReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Beacon Requests Received = %u",
            mac->stats.numBeaconsReqRecd);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Packets Dropped = %u",
            mac->stats.numPktDropped);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Times Data Packets Retried Due To No Ack = %u",
        mac->stats.numDataPktRetriedForNoAck);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
    sprintf(buf, "Number Of Data Packets Dropped Due To No Ack= %u",
        mac->stats.numDataPktDroppedNoAck);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number Of Data Packets Dropped Due To Channel Access"
                 " Failure = %u",
        mac->stats.numDataPktDroppedChannelAccessFailure);
    IO_PrintStat(node,
                 "MAC",
                 "MAC-802.15.4",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
}


void Mac802_15_4BroadcastHandler(Node* node,Int32 interfaceIndex)
{
    MacData802_15_4* mac = (MacData802_15_4*)
                                node->macData[interfaceIndex]->macVar;
    if (mac->isPollRequestPending)
    {
        // Send the poll request after broadcast timer has expired.
        // we reached here because no broadcast was received.

         mac->isPollRequestPending = FALSE;
         Mac802_15_4mlme_poll_request(node,
                                      interfaceIndex,
                                      mac->panDes2.CoordAddrMode,
                                      mac->panDes2.CoordPANId,
                                      mac->panDes2.CoordAddress_64,
                                      mac->capability.secuCapable,
                                      TRUE,
                                      TRUE,
                                      PHY_SUCCESS);
    }
    else
    {
        if (!mac->bcnRxT && !mac->bcnSearchT && !mac->mpib.macRxOnWhenIdle)
        {
            Phy802_15_4PlmeSetTRX_StateRequest(node,
                                               interfaceIndex,
                                               TRX_OFF);
        }
    }
}

// /**
// NAME         Mac802_15_4GTSHandler
// PURPOSE      Handle MAC functionality when a GTS timer expires
// PARAMETERS   Node* node
//                  Node which sends the message
//              Int32 interfaceIndex
//                  interfaceindex of the node
//              Message* msg
//                  Timer message
// RETURN       None
// NOTES        None
// **/
static
void Mac802_15_4GTSHandler(Node* node, Int32 interfaceIndex, Message* msg)
{
    MacData802_15_4* mac = (MacData802_15_4*)
                                    node->macData[interfaceIndex]->macVar;
    char* gtsSlotInfo
                  = (char*)MESSAGE_ReturnInfo(msg, INFO_TYPE_Gts_Slot_Start);
    M802_15_4SuperframeSpec sfSpecsToCheck;
    clocktype referenceBeaconTime = 0;
    M802_15_4GTSSpec* gtsSpecsToCheck = NULL;
    Int32 rate = Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]);
    clocktype sSlotDuration = 0;
    if (mac->isPANCoor)
    {
        // GTS timer expired at the pan coordinator
        sfSpecsToCheck = mac->sfSpec;
        referenceBeaconTime = mac->macBcnTxTime;
        gtsSpecsToCheck = &mac->gtsSpec;
    }
    else if (mac->isCoor || mac->isCoorInDeviceMode)
    {
        // GTS timer expired at the coordinator
        sfSpecsToCheck = mac->sfSpec2;
        referenceBeaconTime = mac->macBcnRxTime;
        gtsSpecsToCheck = &mac->gtsSpec2;
    }
    else
    {
        // RFD
        // GTS timer expired at the rfd

        sfSpecsToCheck = mac->sfSpec2;
        referenceBeaconTime = mac->macBcnRxTime;
        gtsSpecsToCheck = &mac->gtsSpec2;
    }
     sSlotDuration = (sfSpecsToCheck.sd * SECOND) / rate;
     Int32 i = 0;
     for (i = gtsSpecsToCheck->count - 1;
          i >= 0 && gtsSpecsToCheck->count > 0;
          i--)
    {
        if (*gtsSlotInfo == gtsSpecsToCheck->slotStart[i])
        {
            BOOL activateRx = FALSE;
            if (gtsSpecsToCheck->recvOnly[i])
            {
                // Receive only GTS. Enable the RX at PHY
                if (!mac->isPANCoor && (mac->isCoor || mac->isCoorInDeviceMode))
                {
                    activateRx = TRUE;
                }
            }
            else
            {
                if (mac->isPANCoor)
                {
                  activateRx = TRUE;
                }
            }
            if (activateRx)
            {
                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   FORCE_TRX_OFF);

                Phy802_15_4PlmeSetTRX_StateRequest(node,
                                                   interfaceIndex,
                                                   RX_ON);
            } //  if (activateRx)
            else
            {
                // Sendgts. Start sending the packets in GTS
                if (mac->taskP.mcps_gts_data_request_STEP)
                {
                    mac->taskP.mcps_gts_data_request_STEP = GTS_INIT_STEP;
                    if (mac->txT)
                    {
                        MESSAGE_CancelSelfMsg(node, mac->txT);
                        mac->txT = NULL;
                    }
                    mac->currentGtsPositionDesc
                        = M802_15_4_DEFAULTGTSPOSITION;
                    ERROR_Assert(mac->txGts,"mac->txGts should be non NULL");
                    mac->txGts = NULL;
                    mac->waitDataAck = FALSE;
                }
                if (mac->backoffStatus == BACKOFF)
                {
                    mac->taskP.mcps_data_request_Last_TxOptions 
                        = mac->taskP.mcps_data_request_TxOptions;
                    mac->backoffStatus = BACKOFF_RESET;
                    Csma802_15_4Cancel(node, interfaceIndex);
                }
                if (gtsSpecsToCheck->msg[i])
                {
                    if (Mac802_15_4CanProceedInGts(node,
                                                   interfaceIndex,
                                                   mac,
                                                   i,
                                                   gtsSpecsToCheck->msg[i]))
                    {
                        mac->currentGtsPositionDesc = i;
                        ERROR_Assert(!mac->txGts,"mac->txGts should be NULL");
                        ERROR_Assert(!mac->taskP.mcps_gts_data_request_STEP,
                                     "GTS data request should be set to zero");
                        mac->taskP.mcps_gts_data_request_STEP
                            = GTS_PKT_RETRY_STEP;
                        mac->taskP.mcps_data_request_TxOptions = TxOp_GTS;
                        Mac802_15_4MCPS_DATA_request(
                          node,
                          interfaceIndex,
                          M802_15_4DEFFRMCTRL_ADDRMODE16,
                          mac->mpib.macPANId,
                          (UInt16)node->nodeId,
                          M802_15_4DEFFRMCTRL_ADDRMODE16,
                          mac->mpib.macPANId,
                          gtsSpecsToCheck->fields.list[i].devAddr16,
                          MESSAGE_ReturnPacketSize(gtsSpecsToCheck->msg[i]),
                          gtsSpecsToCheck->msg[i],
                          0,
                          TxOp_GTS);
                    }
                }
                else if (!gtsSpecsToCheck->queue[i]->isEmpty())
                {
                    // Try sending data in this GTS
                    Message* msg = NULL;
                    gtsSpecsToCheck->queue[i]->retrieve(&msg,
                                                         DEQUEUE_NEXT_PACKET,
                                                         PEEK_AT_NEXT_PACKET,
                                                         0,
                                                         NULL);
                    if (Mac802_15_4CanProceedInGts(node,
                                                   interfaceIndex,
                                                   mac,
                                                   i,
                                                   msg))
                    {
                        // We can proceed in this GTS
                        // The message will be handled by MAC.now.

                        ERROR_Assert(!gtsSpecsToCheck->msg[i],"");
                        gtsSpecsToCheck->queue[i]->retrieve(
                                                  &(gtsSpecsToCheck->msg[i]),
                                                  DEQUEUE_NEXT_PACKET,
                                                  DEQUEUE_PACKET,
                                                  0,
                                                  NULL);
                        mac->stats.numDataPktsDeQueuedForGts++;
                        mac->currentGtsPositionDesc = i;
                        ERROR_Assert(!mac->txGts,"mac->txGts should be NULL");
                        ERROR_Assert(!mac->taskP.mcps_gts_data_request_STEP,
                                     "GTS data request should be set to zero");
                        Mac802_15_4MCPS_DATA_request(
                          node,
                          interfaceIndex,
                          M802_15_4DEFFRMCTRL_ADDRMODE16,
                          mac->mpib.macPANId,
                          (UInt16)node->nodeId,
                          M802_15_4DEFFRMCTRL_ADDRMODE16,
                          mac->mpib.macPANId,
                          gtsSpecsToCheck->fields.list[i].devAddr16,
                          MESSAGE_ReturnPacketSize(gtsSpecsToCheck->msg[i]),
                          gtsSpecsToCheck->msg[i],
                          0,
                          TxOp_GTS);
                    }
                }
            }

            // schedule next gts timer
            if ((i > 0) && (gtsSpecsToCheck->slotStart[i - 1] > 0))
            {
                clocktype delay = gtsSpecsToCheck->length[i] * sSlotDuration;
                Message* timerMsg = NULL;
                timerMsg = MESSAGE_Alloc(node,
                                         MAC_LAYER,
                                         MAC_PROTOCOL_802_15_4,
                                         MSG_MAC_TimerExpired);

                MESSAGE_SetInstanceId(timerMsg, (short)
                                      mac->myMacData->interfaceIndex);

                MESSAGE_InfoAlloc(node, timerMsg, sizeof(M802_15_4Timer));
                M802_15_4Timer* timerInfo
                            = (M802_15_4Timer*) MESSAGE_ReturnInfo(timerMsg);

                timerInfo->timerType = M802_15_4GTSTIMER;
                MESSAGE_AddInfo(node,
                                timerMsg,
                                sizeof(UInt8),
                                INFO_TYPE_Gts_Slot_Start);
                char* gtsSlotInfo
                    = MESSAGE_ReturnInfo(timerMsg, INFO_TYPE_Gts_Slot_Start);
                *gtsSlotInfo = gtsSpecsToCheck->slotStart[i - 1];
                mac->gtsT = timerMsg;
                MESSAGE_Send(node, timerMsg, delay);
            }
            else
            {
                // Probably last GTS in the superframe. 
                // No need to schedule next gts timer

                ERROR_Assert(mac->gtsT,"mac->gtsT should be non NULL");
                mac->gtsT = NULL;

                // the msg is freed elsewhere
            }
            break; // break out of outer for loop
        } // if (*gtsSlotInfo == gtsSpecsToCheck.slotStart[i])
    }// for (i=0; i < gtsSpecsToCheck.count; i++)
}

// /**
// NAME         Mac802_15_4GTSDeallocateHandler
// PURPOSE      Handle MAC functionality when a GTS deallocate timer expires
// PARAMETERS   Node* node
//                  Node which sends the message
//              Int32 interfaceIndex
//                  interfaceindex of the node
//              Message* msg
//                  Timer message
// RETURN       None
// NOTES        None
// **/
static
void Mac802_15_4GTSDeallocateHandler(Node* node,
                                     Int32 interfaceIndex, 
                                     Message* msg)
{
    MacData802_15_4* mac = (MacData802_15_4*)
                                    node->macData[interfaceIndex]->macVar;
    ERROR_Assert(mac->isCoor || mac->isCoorInDeviceMode,"It should be an"
                 " FFD either in coordinator or device mode");
    BOOL timerFound = FALSE;
    Int32 i = 0;
    for (i = 0; i < mac->gtsSpec2.count; i++)
    {
        ERROR_Assert(mac->gtsSpec2.fields.list[i].devAddr16
                                    == mac->mpib.macCoordShortAddress,
                     " The addresses should match");
        if (msg == mac->gtsSpec2.deAllocateTimer[i])
        {
            timerFound = TRUE;
            mac->gtsSpec2.deAllocateTimer[i] = NULL;
            mac->gtsSpec2.endTime[i] = 0;

            // initiate de-allocation processes
            if (mac->taskP.mlme_gts_request_STEP || mac->gtsRequestData.active)
            {
                mac->gtsRequestPendingData.allocationRequest = FALSE;
                mac->gtsRequestPendingData.receiveOnly
                    = mac->gtsSpec2.recvOnly[i];
                mac->gtsRequestPendingData.numSlots = mac->gtsSpec2.length[i];
                mac->gtsRequestPendingData.active = TRUE;
                mac->gtsRequestPending = TRUE;
                return;
            }

            mac->gtsRequestData.allocationRequest = FALSE;
            mac->gtsRequestData.receiveOnly = mac->gtsSpec2.recvOnly[i];
            mac->gtsRequestData.numSlots = mac->gtsSpec2.length[i];
            mac->gtsRequestData.active = TRUE;
            UInt8 gtsChracteristics
                = Mac802_15_4CreateGtsChracteristics(
                                    mac->gtsRequestData.allocationRequest,
                                    mac->gtsRequestData.numSlots,
                                    mac->gtsRequestData.receiveOnly);

            // send de-allocate request
            Mac802_15_4MLME_GTS_request(node,
                                        interfaceIndex,
                                        gtsChracteristics,
                                        FALSE,
                                        PHY_SUCCESS);
        }
    }
    ERROR_Assert(timerFound,"GTS timer expired unexpectedly");
}

// /**
// FUNCTION     Mac802_15_4Layer
// PURPOSE      Models the behaviour of the MAC layer with the 802.15.4
//              protocol on receiving the message enclosed in msgHdr
// PARAMETERS   Node* node
//                  Node which received the message.
//              Int32 interfaceIndex
//                  Interface index.
//              Message* msg
//                  Message received by the layer.
// RETURN       None
// NOTES        None
// **/
void Mac802_15_4Layer(Node* node, Int32 interfaceIndex, Message* msg)
{
    M802_15_4Timer* timerInfo;
    MacData802_15_4* mac802_15_4 = (MacData802_15_4*)
                                    node->macData[interfaceIndex]->macVar;

    switch(msg->eventType)
    {
        case MSG_MAC_TimerExpired:
        {
            timerInfo = (M802_15_4Timer*) MESSAGE_ReturnInfo(msg);
            switch (timerInfo->timerType)
            {
                case M802_15_4CHECKOUTGOING:
                {
                    Mac802_15_4CheckForOutgoingPacket(node, interfaceIndex);
                    Mac802_15_4SetTimer(node,
                                        mac802_15_4,
                                        M802_15_4CHECKOUTGOING,
                                        1 * SECOND,
                                        NULL);
                    break;
                }
                case M802_15_4TXOVERTIMER:
                {
                    Mac802_15_4TxOverHandler(node, interfaceIndex);
                    mac802_15_4->txOverT = NULL;
                    break;
                }
                case M802_15_4TXTIMER:
                {
                    Mac802_15_4TxHandler(node, interfaceIndex);
                    mac802_15_4->txT = NULL;
                    break;
                }
                case M802_15_4EXTRACTTIMER:
                {
                    Mac802_15_4ExtractHandler(node, interfaceIndex);
                    mac802_15_4->extractT = NULL;
                    break;
                }
                case M802_15_4ASSORSPWAITTIMER:
                {
                    Mac802_15_4AssoRspWaitHandler(node, interfaceIndex);
                    mac802_15_4->assoRspWaitT = NULL;
                    break;
                }
                case M802_15_4DATAWAITTIMER:
                {
                    mac802_15_4->dataWaitT = NULL;
                    Mac802_15_4DataWaitHandler(node, interfaceIndex);
                    break;
                }
                case M802_15_4RXENABLETIMER:
                {
                    Mac802_15_4RxEnableHandler(node, interfaceIndex);
                    mac802_15_4->rxEnableT = NULL;
                    break;
                }
                case M802_15_4SCANTIMER:
                {
                    mac802_15_4->scanT = NULL;
                    Mac802_15_4ScanHandler(node, interfaceIndex);
                    break;
                }
                case M802_15_4BEACONTXTIMER:
                {
                    mac802_15_4->bcnTxT = NULL;
                    Mac802_15_4BeaconTxHandler(node, interfaceIndex, FALSE);
                    break;
                }
                case M802_15_4BEACONRXTIMER:
                {
                    mac802_15_4->bcnRxT = NULL;
                    Mac802_15_4BeaconRxHandler(node, interfaceIndex);
                    break;
                }
                case M802_15_4BEACONSEARCHTIMER:
                {
                    Mac802_15_4BeaconSearchHandler(node, interfaceIndex);
                    mac802_15_4->bcnSearchT = NULL;
                    break;
                }
                case M802_15_4TXCMDDATATIMER:
                {
                    Mac802_15_4TxBcnCmdDataHandler(node, interfaceIndex);
                    mac802_15_4->txCmdDataT = NULL;
                    break;
                }
                case M802_15_4BACKOFFBOUNDTIMER:
                {
                    Mac802_15_4BackoffBoundHandler(node, interfaceIndex);
                    mac802_15_4->backoffBoundT = NULL;
                    break;
                }
                case M802_15_4ORPHANRSPTIMER:
                {
                    mac802_15_4->orphanT = NULL;
                    Mac802_15_4OrphanRspHandler(node, interfaceIndex);
                    break;
                }
                case M802_15_4IFSTIMER:
                {
                    Mac802_15_4IFSHandler(node, interfaceIndex);
                    mac802_15_4->IFST = NULL;
                    break;
                }
                case M802_15_4BROASCASTTIMER:
                {
                     Mac802_15_4BroadcastHandler(node, interfaceIndex);
                     mac802_15_4->broadcastT = NULL;
                     break;
                }
                case M802_15_4GTSTIMER:
                {
                    Mac802_15_4GTSHandler(node, interfaceIndex, msg);
                    // mac802_15_4->gtsT = NULL;
                    break;
                }
                case M802_15_4GTSDEALLOCATETIMER:
                 {
                     Mac802_15_4GTSDeallocateHandler(node, interfaceIndex, msg);
                     break;
                 }
                default:
                {
                    ERROR_ReportError(
                            "MAC802.15.4: Unknown Timer received.");
                }
            }
            break;
        }

        // Timer message for SSCS sublayer
        case MSG_SSCS_802_15_4_TimerExpired:
        {
            Sscs802_15_4Layer(node, interfaceIndex, msg);
            break;
        }

        // Timer message for CSMA-CA
        case MSG_CSMA_802_15_4_TimerExpired:
        {
            Csma802_15_4Layer(node, interfaceIndex, msg);
            break;
        }
        default:
        {
            break;
        }
    }
    MESSAGE_Free(node, msg);
}

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
//              Int32 numHostBits
//                  number of host bits.
// RETURN       None
// NOTES        None
// **/
void Mac802_15_4Init(Node* node,
                     const NodeInput* nodeInput,
                     Int32 interfaceIndex,
                     NodeAddress subnetAddress,
                     SubnetMemberData* subnetList,
                     Int32 nodesInSubnet)
{
    MacData802_15_4* mac802_15_4 = NULL;

    mac802_15_4 = (MacData802_15_4*)MEM_malloc(sizeof(MacData802_15_4));
    ERROR_Assert(mac802_15_4 != NULL,
                 "MAC-802.15.4: Unable to allocate memory "
                 "for MAC data struct.");
    memset(mac802_15_4, 0, sizeof(MacData802_15_4));

    mac802_15_4->myMacData = node->macData[interfaceIndex];
    mac802_15_4->myMacData->macVar = (void *) mac802_15_4;

    RANDOM_SetSeed(mac802_15_4->seed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_802_15_4,
                   interfaceIndex);

    mac802_15_4->aExtendedAddress = (UInt16)node->nodeId; // let it be nodeid
    mac802_15_4->capability.cap = M802_15_4_DEFAULTDEVCAP;
    Mac802_15_4DevCapParse(&mac802_15_4->capability);

    mac802_15_4->oneMoreBeacon = FALSE;
    mac802_15_4->isPANCoor = FALSE;
    mac802_15_4->inTransmission = FALSE;

    // initialize PIB to default values
    mac802_15_4->mpib = MPIB;
    mac802_15_4->mpib.macBSN =
        (UInt8)RANDOM_nrand(mac802_15_4->seed) % 0x100;
    mac802_15_4->mpib.macDSN =
        (UInt8)RANDOM_nrand(mac802_15_4->seed) % 0x100;

    mac802_15_4->mpib.macBeaconOrder = 15;
    mac802_15_4->macBeaconOrder2 = 15;
    mac802_15_4->macSuperframeOrder2 = M802_15_4_BEACONORDER;
    mac802_15_4->macBeaconOrder3 = 15;
    mac802_15_4->macSuperframeOrder3 = M802_15_4_BEACONORDER;

    mac802_15_4->numLostBeacons = 0;


    mac802_15_4->mpib.macRxOnWhenIdle = FALSE;

    Int32 bitsPerSymbol
        = (Int32)((double)Phy802_15_4GetDataRate(node->phyData[interfaceIndex])
            / (double)Phy802_15_4GetSymbolRate(node->phyData[interfaceIndex]));

    // macAckWaitDuration calculation is as per RFC
    // (IEE Sts 802.15.4-2006)section 7.4.2. Propagation delay is also
    // included  in calculation as this cannot be ignored in the current
    // QualNet/EXata implementation even though section 7.4.2 ignores this.
    // ceil is used in calculations below to ensure that calculated
    // propagation delay (in symbols) is never zero.

    mac802_15_4->mpib.macAckWaitDuration
        = (UInt32)(aUnitBackoffPeriod
            + aTurnaroundTime
            + Phy802_15_4GetPhyHeaderDurationInSymbols(
                                              node->phyData[interfaceIndex])
            + ceil(((double)numPsduOctetsInAckFrame * (double)numBitsPerOctet)
                / (double)bitsPerSymbol)
            + 2 * ceil(((double)max_pDelay / (double)SECOND)
                * (double)Phy802_15_4GetSymbolRate(
                                            node->phyData[interfaceIndex])));

    // Initialize CSMA-CA
    Csma802_15_4Init(node, nodeInput, interfaceIndex);

    mac802_15_4->isBroadCastPacket = FALSE;
    mac802_15_4->isCalledAfterTransmittingBeacon = FALSE;
    mac802_15_4->isPollRequestPending = FALSE;
    mac802_15_4->ifsTimerCalledAfterReceivingBeacon = FALSE;
    mac802_15_4->broadCastQueue = new Queue;
    mac802_15_4->broadCastQueue->SetupQueue(node,
                                            "FIFO",
                                            M802_15_4BROADCASTQUEUESIZE);

    mac802_15_4->displayGtsStats = FALSE;

    // Initialize SSCS sublayer
    Sscs802_15_4Init(node, nodeInput, interfaceIndex);
    Mac802_15_4SetTimer(node,
                        mac802_15_4,
                        M802_15_4CHECKOUTGOING,
                        1 * SECOND,
                        NULL);

    memset(&(mac802_15_4->gtsSpec), 0, sizeof(M802_15_4GTSSpec));
    memset(&(mac802_15_4->gtsSpec2), 0, sizeof(M802_15_4GTSSpec));
    mac802_15_4->sendGtsRequestToPancoord = FALSE;
    mac802_15_4->sendGtsConfirmationPending = FALSE;
    mac802_15_4->receiveGtsConfirmationPending = FALSE;
    mac802_15_4->sendGtsTrackCount = 0;
    mac802_15_4->receiveGtsTrackCount = 0;
    mac802_15_4->CheckPacketsForTransmission = FALSE;
    memset(&(mac802_15_4->gtsRequestData), 0, sizeof(GtsRequestData));
    mac802_15_4->gtsRequestExhausted = FALSE;
    
    mac802_15_4->capQueue = new Queue;
    mac802_15_4->capQueue->SetupQueue(node,
                                      "FIFO",
                                      M802_15_4BROADCASTQUEUESIZE);

     if (mac802_15_4->isCoor && mac802_15_4->sfSpec.BO != 15)
     {
         mac802_15_4->isSyncLoss = TRUE;
     }
     // mac802_15_4->taskP.gtsChracteristics = 0;

     mac802_15_4->isDisassociationPending = FALSE;

#ifdef PARALLEL // Parallel
    PARALLEL_SetProtocolIsNotEOTCapable(node);
    PARALLEL_SetMinimumLookaheadForInterface(node, 0);
#endif // endParallel
}

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
void Mac802_15_4Finalize(Node* node, Int32 interfaceIndex)
{
    if (node->macData[interfaceIndex]->macStats == TRUE)
    {
        Sscs802_15_4Finalize(node, interfaceIndex);
        Mac802_15_4PrintStats(node, interfaceIndex);
    }
}
