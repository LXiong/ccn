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
// PROTOCOL   :: MAC802.16
// LAYER      :: MAC
// REFERENCES ::
// + IEEE Std 802.16-2004 : "Part 16: Air Interface for Fixed Broadband
//                           Wireless Access Systems"
// + IEEE Std 802.16-2004 : "Part 16: Air Interface for Fixed Broadband
//                           Wireless Access Systems"
// COMMENTS   :: This is the implementation of the IEEE 802.16 MAC
// **/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"

#include "ipv6.h"
#include "network_dualip.h"

#include "mac_dot16.h"
#include "mac_dot16_bs.h"
#include "dot16_backbone.h"

#define DEBUG_PAGING        0

// /**
// FUNCTION   :: Dot16BackboneRemoveMgmtMsgHeader
// LAYER      :: Layer3
// PURPOSE    :: Remove Message Layer 3 header.
// PARAMETERS ::
// + node      : Node*                     : Pointer to node.
// + msg       : Message*                  : Point to message
//                                         : for node to process.
// + msgType   : Dot16BackbobeMgmtMsgType* : Type of mgmt message
// RETURN     :: void : NULL
// **/
static
void Dot16BackboneRemoveMgmtMsgHeader(
        Node* node,
        Message* msg,
        Dot16BackboneMgmtMsgHeader* headInfo)
{
    Dot16BackboneMgmtMsgHeader* msgHeader;

    msgHeader = (Dot16BackboneMgmtMsgHeader*) msg->packet;
    (*headInfo).msgType = msgHeader->msgType;

    MESSAGE_RemoveHeader(
        node,
        msg,
        sizeof(Dot16BackboneMgmtMsgHeader),
        TRACE_DOT16);
} // Dot16BackboneRemoveMgmtMsgHeader

// /**
// FUNCTION   :: Dot16BackboneAddMgmtMsgHeader
// LAYER      :: Layer3
// PURPOSE    :: Add Message Layer 3 header.
// PARAMETERS ::
// + node      : Node*                     : Pointer to node.
// + msg       : Message*                  : Point to message for
//                                         : node to process.
// + msgType   : Dot16BackbobeMgmtMsgType  : Type of mgmt message
// RETURN     :: void : NULL
// **/
//*************************************************************************
static
void Dot16BackboneAddMgmtMsgHeader(
        Node *node,
        Message **msg,
        Dot16BackboneMgmtMsgHeader headInfo)
{
    int hdrSize;
    Dot16BackboneMgmtMsgHeader *msgHeader;

    hdrSize = sizeof(Dot16BackboneMgmtMsgHeader);
    MESSAGE_AddHeader(node, (*msg), hdrSize, TRACE_DOT16);

    msgHeader = (Dot16BackboneMgmtMsgHeader *) MESSAGE_ReturnPacket((*msg));

    msgHeader->msgType = headInfo.msgType;

} // Dot16BackboneAddMgmtMsgHeader

// /**
// FUNCTION   :: Dot16BsSendMgmtMsgToNbrBsOverBackbone
// LAYER      :: Layer 3
// PURPOSE    :: Send a MAC msg to nbr BS over backbone for
//            :: management purpose
//               currently only IP is supported
// PARAMETERS ::
// + node      : Node*             : Pointer to node
// + dot16     : MacDataDot16*     : Pointer to DOT16 structure
// + destNodeAddress : NodeAddress : Node Id of the beighboring BS
// + msg       : Message*          : Message to be sent
// + headerInfo : Dot16BackboneMgmtMsgHeader : header info
// + delay     : clocktype         : delay for the transmission of msg
// RETURN     :: void              : NULL
// **/
/*
void Dot16BsSendMgmtMsgToNbrBsOverBackbone(
         Node* node,
         MacDataDot16* dot16,
         NodeAddress destNodeAddress,
         Message* msg,
         Dot16BackboneMgmtMsgHeader headerInfo,
         clocktype delay)
{
    Dot16BackboneAddMgmtMsgHeader(node, &msg, headerInfo);

    msg->layerType = NETWORK_LAYER;
    msg->protocolType = NETWORK_PROTOCOL_IP;
    msg->eventType = MSG_NETWORK_FromTransportOrRoutingProtocol;

    NetworkIpSendRawMessageWithDelay(
        node,
        msg,
         //sourceAddr
        //MAPPING_GetDefaultInterfaceAddressFromNodeId(node, node->nodeId),
        ANY_IP,
        destNodeAddress,            //destinationAddress,
        ANY_INTERFACE,              //outgoingInterface,
        IPTOS_PREC_INTERNETCONTROL, // priority
        IPPROTO_DOT16,
        IPDEFTTL,                   // ttl, default is used
        delay);
}
*/
// /**
// FUNCTION   :: Dot16BsSendMgmtMsgToNbrBsOverBackbone
// LAYER      :: Layer 3
// PURPOSE    :: Send a MAC msg to nbr BS over backbone for
//            :: management purpose
//               currently only IP is supported
// PARAMETERS ::
// + node      : Node*             : Pointer to node
// + dot16     : MacDataDot16*     : Pointer to DOT16 structure
// + destNodeAddress : Address     : Node address of the beighboring BS
// + msg       : Message*          : Message to be sent
// + headerInfo : Dot16BackboneMgmtMsgHeader : header info
// + delay     : clocktype         : delay for the transmission of msg
// RETURN     :: void              : NULL
// **/
void Dot16BsSendMgmtMsgToNbrBsOverBackbone(
         Node* node,
         MacDataDot16* dot16,
         Address destNodeAddress,
         Message* msg,
         Dot16BackboneMgmtMsgHeader headerInfo,
         clocktype delay)
{
    Address sourceAddress;

    sourceAddress = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
                        node,
                        node->nodeId,
                        destNodeAddress.networkType);

    Dot16BackboneAddMgmtMsgHeader(node, &msg, headerInfo);

    if ((node->networkData.networkProtocol == IPV4_ONLY ||
            node->networkData.networkProtocol == DUAL_IP)
        && (sourceAddress.networkType == NETWORK_IPV4)
        && (destNodeAddress.networkType == NETWORK_IPV4))
    {
        msg->layerType = NETWORK_LAYER;
        msg->protocolType = NETWORK_PROTOCOL_IP;
        msg->eventType = MSG_NETWORK_FromTransportOrRoutingProtocol;

        NetworkIpSendRawMessageWithDelay(
            node,
            msg,
            GetIPv4Address(sourceAddress),            //sourceAddr
            GetIPv4Address(destNodeAddress),         //destinationAddress,
            ANY_INTERFACE,              //outgoingInterface,
            IPTOS_PREC_INTERNETCONTROL, // priority
            IPPROTO_DOT16,
            IPDEFTTL,                   // ttl, default is used
            delay);
    }
    else if (((node->networkData.networkProtocol == IPV6_ONLY)
              || (node->networkData.networkProtocol == DUAL_IP))
             && (sourceAddress.networkType == NETWORK_IPV6)
             && (destNodeAddress.networkType == NETWORK_IPV6))
    {
        msg->layerType = NETWORK_LAYER;
        msg->protocolType = NETWORK_PROTOCOL_IPV6;
        msg->eventType = MSG_NETWORK_FromTransportOrRoutingProtocol;

        Ipv6SendRawMessage(
            node,
            msg,
            GetIPv6Address(sourceAddress),
            GetIPv6Address(destNodeAddress),
            ANY_INTERFACE,              //outgoingInterface,
            IPTOS_PREC_INTERNETCONTROL, // priority
            IPPROTO_DOT16,
            IPDEFTTL);
    }
}

// /**
// FUNCTION   :: Dot16eBackboneBdcstPagAnn
// LAYER      :: Layer3
// PURPOSE    :: Broadcast Paging Announce to all the BS in the paging grp
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Pointer to dot16 data structure
// + msg              : Message*          : Message for node to interpret
// + pgGrpId          : UInt16            : Paging Group Id
// RETURN     :: void : NULL
// **/
static
void Dot16eBackboneBdcstPagAnn(Node* node,
                               MacDataDot16* dot16,
                               Message* msg,
                               UInt16 pgGrpId)
{
    MacDot16ePagingCtrl* dot16PagingCtrl =
            ((MacDot16Bs*)dot16->bsData)->pgCtrlData;
    MacDot16ePagingGrpIdInfo* pgGrpInfo = dot16PagingCtrl->pgGrpInfo;
    Dot16BackboneMgmtMsgHeader headInfo;

    if(DEBUG_PAGING)
    {
        printf("At %" TYPES_64BITFMT "d : Node %d (PC) :"
               "Sending Paging Announce msg to group "
                "id %d\n",
                node->getNodeTime(), node->nodeId, pgGrpId);
    }

    headInfo.msgType = DOT16e_BACKBONE_MGMT_Paging_announce;
    while(pgGrpInfo)
    {
        if (pgGrpInfo->pagingGrpId == pgGrpId)
        {
            Dot16BsSendMgmtMsgToNbrBsOverBackbone(
                    node,
                    dot16,
                    pgGrpInfo->bsIpAddress,
                    MESSAGE_Duplicate(node, msg),
                    headInfo,
                    0);
         if(DEBUG_PAGING)
            {
                char tmpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(pgGrpInfo->bsId, tmpStr);
                printf("At %" TYPES_64BITFMT "d : Node %d (PC) :"
                       "Sending Paging Announce msg to %s\n",
                       node->getNodeTime(), node->nodeId, tmpStr);
            }

        }
        pgGrpInfo = pgGrpInfo->next;
    }
    MESSAGE_Free(node, msg);
}
// /**
// FUNCTION   :: Dot16eBackboneHandleImEntryMsg
// LAYER      :: Layer3
// PURPOSE    :: Add SS info to Paging Controller cache on IM entry
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Pointer to dot16 data structure
// + msg              : Message*          : Message for node to interpret
// RETURN     :: void : NULL
// **/
static
void Dot16eBackboneHandleImEntryMsg(
                Node* node,
                MacDataDot16* dot16,
                Message* msg)
{
    MacDot16ePagingCtrl* dot16PagingCtrl =
            ((MacDot16Bs*)dot16->bsData)->pgCtrlData;
    Dot16eIMEntryReqPdu* imEntry = (Dot16eIMEntryReqPdu*)
            MESSAGE_ReturnPacket(msg);
    MacDot16eSSIdleInfo* ssInfo = NULL;
    int i;

    if(DEBUG_PAGING)
    {
        char tmpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(imEntry->BSId, tmpStr);
        printf("At %" TYPES_64BITFMT "d Node %d (PC) :"
               "Received IM Entry Msg from Node %s\n",
               node->getNodeTime(), node->nodeId, tmpStr);
    }

    dot16PagingCtrl->stats.numImEntryReqRcvd++;
    for (i = 0; i < DOT16e_PC_MAX_SS_IDLE_INFO; i++)
    {
        ssInfo = &dot16PagingCtrl->ssInfo[i];
        if (ssInfo->isValid == FALSE || MacDot16SameMacAddress(
            imEntry->msMacAddress, ssInfo->msMacAddress))
        {
            memcpy(&ssInfo->bsIpAddress, &imEntry->bsIpAddress,
                sizeof(Address));
            memcpy(&ssInfo->initialBsIpAddress, &imEntry->initialBsIpAddress,
                sizeof(Address));
            ssInfo->BSId = imEntry->BSId;
            ssInfo->initialBsId = imEntry->initialBsId;
            ssInfo->pagingGroupId = imEntry->pagingGroupId;
            ssInfo->pagingCycle = imEntry->pagingCycle;
            ssInfo->basicCid = imEntry->basicCid;
            ssInfo->primaryCid = imEntry->primaryCid;
            ssInfo->secondaryCid = imEntry->secondaryCid;
            ssInfo->numCidSupport = imEntry->numCidSupport;
            ssInfo->pagingOffset = imEntry->pagingOffset;
            ssInfo->idleModeRetainInfo = imEntry->idleModeRetainInfo;
            memcpy(&ssInfo->msMacAddress, &imEntry->msMacAddress,
                DOT16_MAC_ADDRESS_LENGTH);
            memcpy(&ssInfo->ssAuthKeyInfo, &imEntry->ssAuthKeyInfo,
                sizeof(MacDot16SsAuthKeyInfo));
            memcpy(&ssInfo->ssBasicCapability, &imEntry->ssBasicCapability,
                sizeof(MacDot16SsBasicCapability));
            ssInfo->lastUpdate = node->getNodeTime();
            ssInfo->isValid = TRUE;
            //reset the timer
            if (ssInfo->timerIdleModeSys)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerIdleModeSys);
                ssInfo->timerIdleModeSys = NULL;
            }

            ssInfo->timerIdleModeSys = MacDot16SetTimer(
                node,
                dot16,
                DOT16e_TIMER_IdleModeSystem,
                DOT16e_BS_IDLE_MODE_SYSTEM_TIMEOUT,
                NULL,
                ssInfo->BSId,
                ssInfo->basicCid);
            break;
        }
    }
    if (i == DOT16e_PC_MAX_SS_IDLE_INFO)
    {
        ERROR_ReportWarning("Paging Controller : No space left to "
                            "cache more devices in idle mode");
    }
}

// /**
// FUNCTION   :: Dot16eBackboneHandleImExitMsg
// LAYER      :: Layer3
// PURPOSE    :: Delete Node related info from PC on IM exit
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Pointer to dot16 data structure
// + msg              : Message*          : Message for node to interpret
// RETURN     :: void : NULL
// **/
static
void Dot16eBackboneHandleImExitMsg(
                        Node* node,
                        MacDataDot16* dot16,
                        Message* msg)
{
    MacDot16ePagingCtrl* dot16PagingCtrl =
            ((MacDot16Bs*)dot16->bsData)->pgCtrlData;
    Dot16eIMExitReqPdu* imExit = (Dot16eIMExitReqPdu*)
            MESSAGE_ReturnPacket(msg);
    Message* pgMsg;
    Message* delMsMsg;
    Dot16BackboneMgmtMsgHeader headInfo;
    Dot16ePagingAnnouncePdu pgAnn;
    MacDot16eSSIdleInfo* ssInfo = NULL;
    int i;

    if(DEBUG_PAGING)
    {
        char tmpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(imExit->BSId, tmpStr);
        printf("At %" TYPES_64BITFMT "d : Node %d (PC) :"
               "Received IM Exit Msg from Node %s\n",
               node->getNodeTime(), node->nodeId, tmpStr);
    }

    dot16PagingCtrl->stats.numImExitReqRcvd++;
    for (i = 0; i < DOT16e_PC_MAX_SS_IDLE_INFO; i++)
    {
        ssInfo = &dot16PagingCtrl->ssInfo[i];
        if (ssInfo->isValid == TRUE && MacDot16SameMacAddress(
            imExit->msMacAddress, ssInfo->msMacAddress))
        {
            if (ssInfo->timerIdleModeSys != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerIdleModeSys);
                ssInfo->timerIdleModeSys = NULL;
            }
            ssInfo->isValid = FALSE;

            headInfo.msgType = DOT16e_BACKBONE_MGMT_Delete_Ms_entry;
            delMsMsg = MESSAGE_Alloc(node, 0, 0, 0);
            MESSAGE_PacketAlloc(node, delMsMsg, DOT16_MAC_ADDRESS_LENGTH,
                                TRACE_DOT16);
            MacDot16CopyMacAddress(MESSAGE_ReturnPacket(delMsMsg),
                                        imExit->msMacAddress);
            //send Del MS entry
            Dot16BsSendMgmtMsgToNbrBsOverBackbone(
                    node,
                    dot16,
                    ssInfo->initialBsIpAddress,
                    delMsMsg,
                    headInfo,
                    0);
            dot16PagingCtrl->stats.numDeleteMsEntrySent++;

            pgAnn.lastBSId = ssInfo->BSId;
            MacDot16CopyMacAddress(pgAnn.msMacAddress, ssInfo->msMacAddress);
            pgAnn.pagingStart = FALSE;

            pgMsg = MESSAGE_Alloc(node, 0, 0, 0);
            MESSAGE_PacketAlloc(node, pgMsg, sizeof(Dot16ePagingAnnouncePdu),
                                TRACE_DOT16);
            memcpy(MESSAGE_ReturnPacket(pgMsg),
                   &pgAnn,
                   sizeof(Dot16ePagingAnnouncePdu));

            if(DEBUG_PAGING)
            {
                printf("At %" TYPES_64BITFMT "d : Node %d (PC) :"
                       " Send Paging Announce message to all BS"
                       "in the paging group to stop paging \n",
                        node->getNodeTime(), node->nodeId);
            }

            Dot16eBackboneBdcstPagAnn(node, dot16, pgMsg,
                ssInfo->pagingGroupId);
            dot16PagingCtrl->stats.numPagingAnnounceSent++;
            break;
        }
    }
    if (i == DOT16e_PC_MAX_SS_IDLE_INFO)
    {
        ERROR_ReportWarning("Paging Controller : Could not find "
                "the device in cache");
    }
}

// /**
// FUNCTION   :: Dot16eBackboneHandleLuUpdMsg
// LAYER      :: Layer3
// PURPOSE    :: Update Paging group id for the node
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Pointer to dot16 data structure
// + msg              : Message*          : Message for node to interpret
// RETURN     :: void : NULL
// **/
static
void Dot16eBackboneHandleLuUpdMsg(
                        Node* node,
                        MacDataDot16* dot16,
                        Message* msg)
{
    MacDot16ePagingCtrl* dot16PagingCtrl =
            ((MacDot16Bs*)dot16->bsData)->pgCtrlData;
    Dot16eLUPdu* luUpd = (Dot16eLUPdu*) MESSAGE_ReturnPacket(msg);
    MacDot16eSSIdleInfo* ssInfo = NULL;
    int i;

    if(DEBUG_PAGING)
    {
        char tmpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(luUpd->BSId, tmpStr);
        printf("At %" TYPES_64BITFMT "d : Node %d (PC) :"
               "Received Location update Msg from"
               " Node %s\n",node->getNodeTime(), node->nodeId, tmpStr);
    }

    dot16PagingCtrl->stats.numLuReqRcvd++;
    for (i = 0; i < DOT16e_PC_MAX_SS_IDLE_INFO; i++)
    {
        ssInfo = &dot16PagingCtrl->ssInfo[i];
        if (ssInfo->isValid == TRUE  && MacDot16SameMacAddress(
            luUpd->msMacAddress, ssInfo->msMacAddress))
        {
            ssInfo->lastUpdate = node->getNodeTime();
            ssInfo->pagingGroupId = luUpd->pagingGroupId;
            ssInfo->pagingCycle = luUpd->pagingCycle;
            ssInfo->pagingOffset = luUpd->pagingOffset;
            //reset the timer
            if (ssInfo->timerIdleModeSys)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerIdleModeSys);
                ssInfo->timerIdleModeSys = NULL;
            }

            ssInfo->timerIdleModeSys = MacDot16SetTimer(node,
                dot16,
                DOT16e_TIMER_IdleModeSystem,
                DOT16e_BS_IDLE_MODE_SYSTEM_TIMEOUT,
                NULL,
                ssInfo->BSId,
                ssInfo->basicCid);
            break;
        }
    }
    if (i == DOT16e_PC_MAX_SS_IDLE_INFO)
    {
        ERROR_ReportWarning("Paging Controller : Could not find "
                "record to be updated");
    }
}

// /**
// FUNCTION   :: Dot16eBackboneHandleIniPagMsg
// LAYER      :: Layer3
// PURPOSE    :: Send paging anounce to the paging group in which the node
//                  was last reported.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Pointer to dot16 data structure
// + msg              : Message*          : Message for node to interpret
// RETURN     :: void : NULL
// **/
static
void Dot16eBackboneHandleIniPagMsg(
                        Node* node,
                        MacDataDot16* dot16,
                        Message* msg)
{
    MacDot16ePagingCtrl* dot16PagingCtrl =
            ((MacDot16Bs*)dot16->bsData)->pgCtrlData;
    Dot16eInitiatePagingPdu* iniPag = (Dot16eInitiatePagingPdu*)
                                        MESSAGE_ReturnPacket(msg);
    Message* pgMsg;
    Dot16ePagingAnnouncePdu pgAnn;
    MacDot16eSSIdleInfo* ssInfo = NULL;
    int i;

    if(DEBUG_PAGING)
    {
        char tmpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(iniPag->BSId, tmpStr);
        printf("At %" TYPES_64BITFMT "d : Node %d (PC) :"
               "Received Initiate Paging Msg from "
               "Node %s\n",
               node->getNodeTime(), node->nodeId, tmpStr);
    }

    dot16PagingCtrl->stats.numInitiatePagingReqRcvd++;
    for (i = 0; i < DOT16e_PC_MAX_SS_IDLE_INFO; i++)
    {
        ssInfo = &dot16PagingCtrl->ssInfo[i];
        if (ssInfo->isValid == TRUE  && MacDot16SameMacAddress(
            iniPag->msMacAddress, ssInfo->msMacAddress))
        {
            pgAnn.pagingGroupId = ssInfo->pagingGroupId;
            pgAnn.pagingCycle = ssInfo->pagingCycle;
            pgAnn.pagingOffset = ssInfo->pagingOffset;
            pgAnn.lastBSId = ssInfo->BSId;
            MacDot16CopyMacAddress(pgAnn.msMacAddress, ssInfo->msMacAddress);
            pgAnn.pagingStart = TRUE;

            pgMsg = MESSAGE_Alloc(node, 0, 0, 0);
            MESSAGE_PacketAlloc(node, pgMsg, sizeof(Dot16ePagingAnnouncePdu),
                                TRACE_DOT16);
            memcpy(MESSAGE_ReturnPacket(pgMsg), &pgAnn,
                   sizeof(Dot16ePagingAnnouncePdu));

            if(DEBUG_PAGING)
            {
                printf("At %" TYPES_64BITFMT "d : Node %d (PC) :"
                       "Send Paging Announce message to all BS"
                       " in the paging group to start paging \n",
                       node->getNodeTime(), node->nodeId);
            }
            Dot16eBackboneBdcstPagAnn(node, dot16, pgMsg,
                ssInfo->pagingGroupId);
            dot16PagingCtrl->stats.numPagingAnnounceSent++;
            break;
        }
    }
    if (i == DOT16e_PC_MAX_SS_IDLE_INFO)
    {
        ERROR_ReportWarning("Paging Controller : Could not find "
                "record for which paging is to be initiated");
    }
}

// /**
// FUNCTION   :: Dot16eBackboneHandleIdleModeSystemTimeout
// LAYER      :: Layer3
// PURPOSE    ::Handle Idle Mode System timeout
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Pointer to dot16 data structure
// + msg              : Message*          : Message for node to interpret
// RETURN     :: void : NULL
// **/
void Dot16eBackboneHandleIdleModeSystemTimeout(Node* node,
                                               MacDataDot16* dot16,
                                               Message* msg)
{
    MacDot16ePagingCtrl* dot16PagingCtrl =
            ((MacDot16Bs*)dot16->bsData)->pgCtrlData;
    int i;
    MacDot16Timer* timerInfo;
    timerInfo = (MacDot16Timer*) MESSAGE_ReturnInfo(msg);

    for (i = 0; i < DOT16e_PC_MAX_SS_IDLE_INFO; i++)
    {
        MacDot16eSSIdleInfo* pagSsInfo = &dot16PagingCtrl->ssInfo[i];
        if (pagSsInfo->isValid == TRUE && pagSsInfo->BSId == timerInfo->info
            && pagSsInfo->basicCid == timerInfo->Info2)
        {
            // delete the contents
            dot16PagingCtrl->ssInfo[i].isValid = FALSE;
            dot16PagingCtrl->ssInfo[i].timerIdleModeSys = NULL;
            if (DEBUG_PAGING)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Dot16eBackboneHandleIdleModeSystemTimeout. Deleting"
                       "ssinfo for SS %d:%d:%d:%d:%d:%d\n",
                        dot16PagingCtrl->ssInfo[i].msMacAddress[0],
                        dot16PagingCtrl->ssInfo[i].msMacAddress[1],
                        dot16PagingCtrl->ssInfo[i].msMacAddress[2],
                        dot16PagingCtrl->ssInfo[i].msMacAddress[3],
                        dot16PagingCtrl->ssInfo[i].msMacAddress[4],
                        dot16PagingCtrl->ssInfo[i].msMacAddress[5]);
            }

            break;
        }
    }
}

// /**
// FUNCTION   :: Dot16eBackboneHandlePagInfoMsg
// LAYER      :: Layer3
// PURPOSE    :: Populate PC with BS Info (paging group id)
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Pointer to dot16 data structure
// + msg              : Message*          : Message for node to interpret
// RETURN     :: void : NULL
// **/
static
void Dot16eBackboneHandlePagInfoMsg(
                    Node* node,
                    MacDataDot16* dot16,
                    Message* msg)
{
    char errorMsg[MAX_STRING_LENGTH];
    Dot16eBackbonePagingInfo* pagInfo = (Dot16eBackbonePagingInfo*)
                                                MESSAGE_ReturnPacket(msg);
    if(DEBUG_PAGING)
    {
        char tmpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(pagInfo->bsId, tmpStr);
        printf("At %" TYPES_64BITFMT "d : Node %d (PC) :"
               "Received Page Info Msg from Node %s \n",
               node->getNodeTime(), node->nodeId, tmpStr);
    }

    MacDot16ePagingGrpIdInfo* pgGrpInfoNode = (MacDot16ePagingGrpIdInfo*)
            MEM_malloc(sizeof(MacDot16ePagingGrpIdInfo));
    memset((char*)pgGrpInfoNode, 0, sizeof(MacDot16ePagingGrpIdInfo));
    memcpy(&pgGrpInfoNode->bsIpAddress, &pagInfo->bsIPAddress,
        sizeof(Address));
    pgGrpInfoNode->bsId = pagInfo->bsId;
    pgGrpInfoNode->pagingGrpId = pagInfo->pagingGroupId;
    sprintf(errorMsg,"bsId: %d : This BS is not a Paging Controller.\n",pgGrpInfoNode->bsId);
    ERROR_Assert(((MacDot16Bs*)dot16->bsData)->pgCtrlData!=NULL,errorMsg);
    pgGrpInfoNode->next =
                ((MacDot16Bs*)dot16->bsData)->pgCtrlData->pgGrpInfo;
    ((MacDot16Bs*)dot16->bsData)->pgCtrlData->pgGrpInfo = pgGrpInfoNode;
}

// /**
// FUNCTION   :: Dot16BackboneReceivePacketOverIp
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret
// + sourceAddress    : NodeAddress       : Message from node
// RETURN     :: void : NULL
// **/
void Dot16BackboneReceivePacketOverIp(Node *node,
                                      Message *msg,
                                      NodeAddress sourceAddress)
{
    Dot16BackboneMgmtMsgHeader headInfo;
    MacDataDot16* dot16 = NULL;
    int i = 0;
    int index = 0;
    BOOL isFound = FALSE;

    while( i < node->numberInterfaces && isFound == FALSE)
    {
        if (node->macData[i]->macProtocol == MAC_PROTOCOL_DOT16)
        {
            dot16 = (MacDataDot16*) node->macData[i]->macVar;
            if(dot16->stationType == DOT16_BS)
            {
                // examinie it is a BS interface
                isFound = TRUE;
                index = i;
            }
        }
        i ++;
    }

    if (!isFound)
    {
        MESSAGE_Free(node, msg);

        return;
    }

    Dot16BackboneRemoveMgmtMsgHeader(node, msg,  &headInfo);

    switch(headInfo.msgType)
    {
        case DOT16_BACKBONE_MGMT_BS_HELLO:
        {
            NodeAddress srcBsNodeId;

            srcBsNodeId =
                MAPPING_GetNodeIdFromInterfaceAddress(node, sourceAddress);
            MacDot16eBsHandleHelloMsg(node,
                                      dot16,
                                      msg,
                                      srcBsNodeId);

            break;
        }
        case DOT16_BACKBONE_MGMT_HO_FINISH_NOTIFICATION:
        {
            MacDot16eBsHandleHoFinishedMsg(node, dot16, msg);

            break;
        }
        case DOT16e_BACKBONE_MGMT_IM_Entry_State_Change_req:
        {
            Dot16eBackboneHandleImEntryMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_IM_Entry_State_Change_rsp:
        {
            break;
        }
        case DOT16e_BACKBONE_MGMT_IM_Exit_State_Change_req:
        {
            Dot16eBackboneHandleImExitMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_IM_Exit_State_Change_rsp:
        {
            MacDot16eBsHandleImExitRsp(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_LU_Req:
        {
            Dot16eBackboneHandleLuUpdMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_LU_Rsp:
        {
            break;
        }
        case DOT16e_BACKBONE_MGMT_Initiate_Paging_Req:
        {
            Dot16eBackboneHandleIniPagMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_Initiate_Paging_Rsp:
        {
            break;
        }
        case DOT16e_BACKBONE_MGMT_Paging_announce:
        {
            MacDot16eBsHandlePagingAnounce(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_Paging_info:
        {
            Dot16eBackboneHandlePagInfoMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_Delete_Ms_entry:
        {
            MacDot16eBsHandleDeleteMsEntry(node, dot16, msg);
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
// FUNCTION   :: Dot16BackboneReceivePacketOverIp
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages. (reloaded for ipv6)
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret
// + sourceAddress    : Address           : Message from node
// RETURN     :: void : NULL
// **/
void Dot16BackboneReceivePacketOverIp(Node *node,
                                      Message *msg,
                                      Address sourceAddress)
{
    Dot16BackboneMgmtMsgHeader headInfo;
    MacDataDot16* dot16 = NULL;
    int i = 0;
    int index = 0;
    BOOL isFound = FALSE;

    while( i < node->numberInterfaces && isFound == FALSE)
    {
        if (node->macData[i]->macProtocol == MAC_PROTOCOL_DOT16)
        {
            dot16 = (MacDataDot16*) node->macData[i]->macVar;
            if(dot16->stationType == DOT16_BS)
            {
                // examinie it is a BS interface
                isFound = TRUE;
                index = i;
            }
        }
        i ++;
    }

    if ( !isFound)
    {
        MESSAGE_Free(node, msg);

        return;
    }

    Dot16BackboneRemoveMgmtMsgHeader(node, msg,  &headInfo);

    switch(headInfo.msgType)
    {
        case DOT16_BACKBONE_MGMT_BS_HELLO:
        {
            NodeAddress srcBsNodeId;

            srcBsNodeId =
                MAPPING_GetNodeIdFromInterfaceAddress(node, sourceAddress);
            MacDot16eBsHandleHelloMsg(node,
                                      dot16,
                                      msg,
                                      srcBsNodeId);

            break;
        }
        case DOT16_BACKBONE_MGMT_HO_FINISH_NOTIFICATION:
        {
            MacDot16eBsHandleHoFinishedMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_IM_Entry_State_Change_req:
        {
            Dot16eBackboneHandleImEntryMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_IM_Entry_State_Change_rsp:
        {
            MacDot16eBsHandleImEntryRsp(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_IM_Exit_State_Change_req:
        {
            Dot16eBackboneHandleImExitMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_IM_Exit_State_Change_rsp:
        {
            MacDot16eBsHandleImExitRsp(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_LU_Req:
        {
            Dot16eBackboneHandleLuUpdMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_LU_Rsp:
        {
            MacDot16eBsHandleLURsp(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_Initiate_Paging_Req:
        {
            Dot16eBackboneHandleIniPagMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_Initiate_Paging_Rsp:
        {
            MacDot16eBsHandleIniPagRsp(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_Paging_announce:
        {
            MacDot16eBsHandlePagingAnounce(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_Paging_info:
        {
            Dot16eBackboneHandlePagInfoMsg(node, dot16, msg);
            break;
        }
        case DOT16e_BACKBONE_MGMT_Delete_Ms_entry:
        {
            MacDot16eBsHandleDeleteMsEntry(node, dot16, msg);
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
// FUNCTION   :: Dot16BackbonePrintStats
// LAYER      :: Layer 3
// PURPOSE    :: Print out statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + interfaceIndex : int      : Interface index
// RETURN     :: void : NULL
// **/
void Dot16BackbonePrintStats(Node* node,
                            MacDataDot16* dot16,
                            int interfaceIndex)
{
    MacDot16ePagingCtrl* dot16PagingCtrl =
            ((MacDot16Bs*)dot16->bsData)->pgCtrlData;
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Number of IM entry requests rcvd = %d",
            dot16PagingCtrl->stats.numImEntryReqRcvd);
    IO_PrintStat(node,
                 "BACKBONE",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
// for future use
//     sprintf(buf, "Number of IM entry responses sent = %d",
//             dot16PagingCtrl->stats.numImEntryRspSent);
//     IO_PrintStat(node,
//                  "BACKBONE",
//                  "802.16",
//                  ANY_DEST,
//                  interfaceIndex,
//                  buf);

    sprintf(buf, "Number of IM exit requests rcvd = %d",
            dot16PagingCtrl->stats.numImExitReqRcvd);
    IO_PrintStat(node,
                 "BACKBONE",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of location update requests rcvd = %d",
            dot16PagingCtrl->stats.numLuReqRcvd);
    IO_PrintStat(node,
                 "BACKBONE",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
// for future use
//     sprintf(buf, "Number of location  update responses sent = %d",
//             dot16PagingCtrl->stats.numLuRspSent);
//     IO_PrintStat(node,
//                  "BACKBONE",
//                  "802.16",
//                  ANY_DEST,
//                  interfaceIndex,
//                  buf);

    sprintf(buf, "Number of initiate paging requests rcvd = %d",
            dot16PagingCtrl->stats.numInitiatePagingReqRcvd);
    IO_PrintStat(node,
                 "BACKBONE",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
// for future use
//     sprintf(buf, "Number of initiate paging responses sent = %d",
//             dot16PagingCtrl->stats.numInitiatePagingRspSent);
//     IO_PrintStat(node,
//                  "BACKBONE",
//                  "802.16",
//                  ANY_DEST,
//                  interfaceIndex,
//                  buf);

    sprintf(buf, "Number of paging announcements sent = %d",
            dot16PagingCtrl->stats.numPagingAnnounceSent);
    IO_PrintStat(node,
                 "BACKBONE",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
// for future use
//     sprintf(buf, "Number of paging info requests rcvd = %d",
//             dot16PagingCtrl->stats.numPagingInfoRcvd);
//     IO_PrintStat(node,
//                  "BACKBONE",
//                  "802.16",
//                  ANY_DEST,
//                  interfaceIndex,
//                  buf);

    sprintf(buf, "Number of delete MS entry msg sent = %d",
            dot16PagingCtrl->stats.numDeleteMsEntrySent);
    IO_PrintStat(node,
                 "BACKBONE",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
}
