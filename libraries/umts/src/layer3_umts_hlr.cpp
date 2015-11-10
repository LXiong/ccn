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
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"

#include "cellular.h"
#include "cellular_layer3.h"
#include "cellular_umts.h"
#include "layer3_umts.h"
#include "layer3_umts_hlr.h"


#define DEBUG_PARAMETER    0
#define DEBUG_PACKET       0
#define DEBUG_TABLE        0
#define DEBUG_BACKBONE     0

// /**
// FUNCTION   :: UmtsLayer3HlrPrintParameter
// LAYER      :: Layer 3
// PURPOSE    :: Print out HLR specific parameters
// PARAMETERS ::
// + node      : Node*           : Pointer to node.
// + hlrL3     : UmtsLayer3Hlr * : Pointer to UMTS HLR Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrPrintParameter(Node *node, UmtsLayer3Hlr *hlrL3)
{
    // start here
    return;
}

// /**
// FUNCTION   :: UmtsLayer3HlrPrintHlr
// LAYER      :: Layer 3
// PURPOSE    :: Print out the entries in HLR data base
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrPrintHlr(Node *node, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Hlr *hlrL3 = (UmtsLayer3Hlr *)(umtsL3->hlrData);
    UmtsHlrMap::iterator pos;
    UmtsHlrEntry *hlrEntry;
    char strBuff[MAX_STRING_LENGTH];

    printf("UMTS Node%d (HLR)'s HLR data base has %u entries:\n",
           node->nodeId,
           (unsigned int)hlrL3->hlrMap->size());

    printf("UeID\t\tSGSN\t\tIMSI\n");
    for (pos = hlrL3->hlrMap->begin(); pos != hlrL3->hlrMap->end(); pos ++)
    {
        hlrEntry = (UmtsHlrEntry *) pos->second;
        hlrEntry->imsi.print(strBuff);
        printf("%d\t\t\t%d\t\t%s\n",
               hlrEntry->hashKey,
               hlrEntry->sgsnId,
               strBuff);
    }
}

// /**
// FUNCTION   :: UmtsLayer3HlrInitParameter
// LAYER      :: Layer 3
// PURPOSE    :: Initialize HLR specific parameters
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + hlrL3     : UmtsLayer3Hlr *  : Pointer to UMTS HLR Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrInitParameter(Node *node,
                                const NodeInput *nodeInput,
                                UmtsLayer3Hlr *hlrL3)
{
    // start here
    return;
}


// /**
// FUNCTION   :: UmtsLayer3HlrPrintStats
// LAYER      :: Layer 3
// PURPOSE    :: Print out HLR specific statistics
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + hlrL3     : UmtsLayer3Hlr *  : Pointer to UMTS HLR Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrPrintStats(Node *node, UmtsLayer3Hlr *hlrL3)
{
    char buff[MAX_STRING_LENGTH];

    // HLR Routing Area Updates Received
    sprintf(buff,
            "Number of Routing Area updates received = %d",
            hlrL3->stat.numRoutingUpdateRcvd);
    IO_PrintStat(node,
                 "Layer3",
                 "UMTS HLR",
                 ANY_DEST,
                 -1,
                 buff);

    // HLR Routing Area Queries Received
    sprintf(buff,
            "Number of Routing Area queries received = %d",
            hlrL3->stat.numRoutingQueryRcvd);
    IO_PrintStat(node,
                 "Layer3",
                 "UMTS HLR",
                 ANY_DEST,
                 -1,
                 buff);

    // HLR Successful Routing Area Query Replies Sent
    sprintf(buff,
            "Number of successful Routing Area query replied = %d",
            hlrL3->stat.numRoutingQuerySuccReplySent);
    IO_PrintStat(node,
                 "Layer3",
                 "UMTS HLR",
                 ANY_DEST,
                 -1,
                 buff);

    // HLR Routing Area Query Failures Sent
    sprintf(buff,
            "Number of Routing Area query failures sent = %d",
            hlrL3->stat.numRoutingQueryFailReplySent);
    IO_PrintStat(node,
                 "Layer3",
                 "UMTS HLR",
                 ANY_DEST,
                 -1,
                 buff);
    return;
}

// /**
// FUNCTION   :: UmtsLayer3HlrSendMsg
// LAYER      :: Layer 3
// PURPOSE    :: Response a reply message to an HLR request
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message *        : Pointer to message to be sent
// + destAddr  : Address          : Address of the destination node
// + destNodeType : CellularNodeType : Node type of the dest node
// + cmdTgt    : UmtsHlrCmdTgt    : command target, UE or cell 
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrSendMsg(Node *node,
                          UmtsLayer3Data *umtsL3,
                          Message *msg,
                          Address destAddr,
                          CellularNodeType destNodeType,
                          UmtsHlrCmdTgt  cmdTgt)
{
    UmtsBackboneMessageType msgType;

    if (destNodeType == CELLULAR_SGSN)
    {
        msgType = UMTS_BACKBONE_MSG_TYPE_GR;
    }
    else if (destNodeType == CELLULAR_GGSN)
    {
        msgType = UMTS_BACKBONE_MSG_TYPE_GC;
    }
    else
    {
        ERROR_ReportWarning("UMTS: HLR can only comm with SGSN or GGSN");
        MESSAGE_Free(node, msg);
        return;
    }

    char info = (char)cmdTgt;
    UmtsAddBackboneHeader(node, msg, msgType, &info, sizeof(info));

    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        destAddr,
        ANY_INTERFACE,
        IPTOS_PREC_INTERNETCONTROL,
        IPDEFTTL);
}

// /**
// FUNCTION   :: UmtsLayer3HlrSendReply
// LAYER      :: Layer 3
// PURPOSE    :: Response a reply message to an HLR request
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + command   : UmtsHlrCommand   : Command of the packet
// + result    : UmtsHlrCommandStatus : Result of the request
// + imsi      : CellularIMSI *   : IMSI of the related UE
// + hlrEntry  : UmtsHlrEntry *   : Point to HLR entry, maybe NULL
// + destAddr  : Address          : Address of the destination node
// + destNodeType : CellularNodeType : Node type of the dest node
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrSendReply(Node *node,
                            UmtsLayer3Data *umtsL3,
                            UmtsHlrCommand command,
                            UmtsHlrCommandStatus result,
                            CellularIMSI *imsi,
                            UmtsHlrEntry *hlrEntry,
                            Address destAddr,
                            CellularNodeType destNodeType)
{
    UmtsHlrReply replyPkt;
    UmtsHlrLocationInfo locInfo;
    Message *msg = NULL;

    replyPkt.command = (unsigned char)command;  // type of reply
    replyPkt.result = (unsigned char)result;    // result of request
    imsi->getCompactIMSI((char*)&(replyPkt.imsi[0]));

    // for some replies, may need to add location info of the UE
    if (result == UMTS_HLR_COMMAND_SUCC && command == UMTS_HLR_QUERY_REPLY)
    {
        memcpy(locInfo.mcc, hlrEntry->mcc, sizeof(CellularMCC));
        memcpy(locInfo.mnc, hlrEntry->mnc, sizeof(CellularMNC));
        locInfo.sgsnId = hlrEntry->sgsnId;
        locInfo.sgsnAddr = hlrEntry->sgsnAddr;
    }

    // allocate message first
    msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);

    MESSAGE_PacketAlloc(
        node, 
        msg, 
        sizeof(replyPkt) + sizeof(locInfo), 
        TRACE_UMTS_LAYER3);

    memcpy(MESSAGE_ReturnPacket(msg), (char*)&replyPkt, sizeof(replyPkt));
    memcpy(MESSAGE_ReturnPacket(msg) + sizeof(replyPkt),
           (char*)&locInfo,
           sizeof(locInfo));

    UmtsLayer3HlrSendMsg(node,
                         umtsL3,
                         msg,
                         destAddr,
                         destNodeType,
                         UMTS_HLR_COMMAND_MS);
}

// /**
// FUNCTION   :: UmtsLayer3HlrSendCellReply
// LAYER      :: Layer 3
// PURPOSE    :: Response a reply message to an HLR request
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + command   : UmtsHlrCommand   : Command of the packet
// + replyInfo : const char*      : info in carried in the cellReply msg
// + replyInfoSize : int          : info size 
// + destAddr  : Address          : Address of the destination node
// + destNodeType : CellularNodeType : Node type of the dest node
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrSendCellReply(
    Node *node,
    UmtsLayer3Data *umtsL3,
    UmtsHlrCommand command,
    const char* replyInfo,
    int replyInfoSize,
    Address destAddr,
    CellularNodeType destNodeType)
{
    Message* msg = MESSAGE_Alloc(
                       node, 
                       NETWORK_LAYER, 
                       NETWORK_PROTOCOL_CELLULAR,
                       0);
    MESSAGE_PacketAlloc(
        node,
        msg,
        replyInfoSize + sizeof(char),
        TRACE_UMTS_LAYER3);

    char* packet = MESSAGE_ReturnPacket(msg);
    packet[0] = (char)command;
    memcpy(packet + sizeof(char),
           replyInfo,
           replyInfoSize);

    UmtsLayer3HlrSendMsg(node,
                         umtsL3,
                         msg,
                         destAddr,
                         destNodeType,
                         UMTS_HLR_COMMAND_CELL);
}

// /**
// FUNCTION   :: UmtsLayer3HlrHandleCellQuery
// LAYER      :: Layer 3
// PURPOSE    :: Response a reply message to an HLR request
//               for cell information
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + hlrL3     : UmtsLayer3Hlr *  : Pointer to the HLR layer3 data
// + cellId    : UInt32           : Cell Id to be queried
// + queryRncId :NodeId           : NodeId of the querying RNC
// + destAddr  : Address          : Address of the destination node
// + destNodeType : CellularNodeType : Node type of the dest node
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrHandleCellQuery(
    Node *node,
    UmtsLayer3Data *umtsL3,
    UmtsLayer3Hlr *hlrL3,
    UInt32 cellId,
    NodeId queryRncId,
    Address destAddr,
    CellularNodeType destNodeType)
{
    UmtsCellCacheMap::iterator cellIter
            = hlrL3->cellCacheMap->find(cellId);
    if (cellIter == hlrL3->cellCacheMap->end())
    {
        return;
    }
    UmtsCellCacheInfo* cellInfo = cellIter->second;
    UmtsRncCacheInfo* rncInfo
        = hlrL3->rncCacheMap->find(cellInfo->rncId)->second;

    std::string msgBuf;
    msgBuf.append((char*)&cellId, sizeof(cellId));
    msgBuf.append((char*)&queryRncId, sizeof(queryRncId));
    msgBuf.append((char*)&cellInfo->nodebId, sizeof(cellInfo->nodebId));
    msgBuf.append((char*)&cellInfo->rncId, sizeof(cellInfo->rncId));
    msgBuf.append((char*)&rncInfo->sgsnId, sizeof(rncInfo->sgsnId));
    msgBuf.append((char*)&rncInfo->sgsnAddr, sizeof(rncInfo->sgsnAddr));
    UmtsLayer3HlrSendCellReply(
        node,
        umtsL3,
        UMTS_HLR_QUERY_CELL_REPLY,
        msgBuf.data(),
        (int)msgBuf.size(),
        destAddr,
        destNodeType);

    //Send to the SGSN being queried the information of the inquiring RNC
    UmtsRncCacheMap::iterator rncIterator;
    rncIterator = hlrL3->rncCacheMap->find(queryRncId);
    if (rncIterator != hlrL3->rncCacheMap->end())
    {
        UmtsRncCacheInfo* queryRncInfo;

        queryRncInfo = (UmtsRncCacheInfo*) rncIterator->second;
        std::string buffer;
        buffer.append((char*)&queryRncId, sizeof(queryRncId));
        buffer.append((char*)&queryRncInfo->sgsnId, sizeof(NodeId));
        buffer.append((char*)&queryRncInfo->sgsnAddr, sizeof(Address));
        UmtsLayer3HlrSendCellReply(
            node,
            umtsL3,
            UMTS_HLR_RNC_UPDATE,
            buffer.data(),
            (int)buffer.size(),
            rncInfo->sgsnAddr,
            CELLULAR_SGSN);
    }
}

// /**
// FUNCTION   :: UmtsLayer3HlrHandleRequest
// LAYER      :: Layer 3
// PURPOSE    :: Handle HLR request packet
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + pktBuff   : char*            : Point to beginning of the request
// + interfaceIndex : int         : Interface from which packet was received
// + srcAddr   : Address          : Address of the source node
// + srcNodeType : CellularNodeType : Node type of the source node
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrHandleRequest(Node *node,
                                UmtsLayer3Data *umtsL3,
                                char *pktBuff,
                                int interfaceIndex,
                                Address srcAddr,
                                CellularNodeType srcNodeType)
{
    UmtsLayer3Hlr *hlrL3 = (UmtsLayer3Hlr *) (umtsL3->hlrData);
    UmtsHlrRequest hlrReq;
    UmtsHlrEntry *hlrEntry;
    UmtsHlrMap::iterator pos;
    UInt32 hashKey;
    CellularIMSI tmpImsi;
    int index = 0;

    // get basic info
    memcpy(&hlrReq, pktBuff, sizeof(hlrReq));
    index += sizeof(hlrReq);
    tmpImsi.setCompactIMSI((char*)&(hlrReq.imsi[0]));
    hashKey = tmpImsi.getId();

    if (DEBUG_PACKET)
    {
        char addrStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(&srcAddr, addrStr);
        printf(
            "UMTS Node%d(HLR) receives a request from node %s for UE %d:",
            node->nodeId, addrStr, hashKey);
    }

    switch (hlrReq.command)
    {
        case UMTS_HLR_UPDATE:
        {
            // Add or update the entry in HLR
            UmtsHlrLocationInfo locInfo;

            memcpy(&locInfo, &pktBuff[index], sizeof(locInfo));
            // retrieve location info

            if (DEBUG_PACKET)
            {
                printf("UPDATE packet to register UE %d under SGSN %d\n",
                       hashKey, locInfo.sgsnId);
            }

            // update statistics
            hlrL3->stat.numRoutingUpdateRcvd ++;

            // search to see if already exist
            pos = hlrL3->hlrMap->find(hashKey);
            if (pos == hlrL3->hlrMap->end())
            {
                // not exist, add it
                hlrEntry = (UmtsHlrEntry *)MEM_malloc(sizeof(UmtsHlrEntry));
                memset((char *)hlrEntry, 0, sizeof(UmtsHlrEntry));

                hlrEntry->hashKey = hashKey;
                hlrEntry->imsi.setCompactIMSI((char*)&(hlrReq.imsi[0]));

                // insert into HLR map
                hlrL3->hlrMap->insert(
                    UmtsHlrMap::value_type(hlrEntry->hashKey, hlrEntry));
            }
            else
            {
                // already exist
                hlrEntry = pos->second;

                if (hlrEntry->sgsnId != locInfo.sgsnId)
                {
                    // UE changed SGSN, tell old SGSN to remove from its VLR
                    UmtsLayer3HlrSendReply(node,
                                           umtsL3,
                                           UMTS_HLR_REMOVE,
                                           UMTS_HLR_COMMAND_SUCC,
                                           &tmpImsi,
                                           hlrEntry,
                                           hlrEntry->sgsnAddr,
                                           CELLULAR_SGSN);
                }
            }

            // copy current location info of UE
            memcpy(hlrEntry->mcc, locInfo.mcc, sizeof(CellularMCC));
            memcpy(hlrEntry->mnc, locInfo.mnc, sizeof(CellularMNC));
            hlrEntry->sgsnId = locInfo.sgsnId;
            hlrEntry->sgsnAddr = srcAddr;

            if (DEBUG_TABLE)
            {
                UmtsLayer3HlrPrintHlr(node, umtsL3);
            }

            // send a reply for the UPDATE request
            UmtsLayer3HlrSendReply(node,
                                   umtsL3,
                                   UMTS_HLR_UPDATE_REPLY,
                                   UMTS_HLR_COMMAND_SUCC,
                                   &tmpImsi,
                                   hlrEntry,
                                   srcAddr,
                                   srcNodeType);
            break;
        }

        case UMTS_HLR_REMOVE:
        {
            // remove the UE from the HLR
            if (DEBUG_PACKET)
            {
                printf("REMOVE packet to remove UE %d from HLR\n", hashKey);
            }

            UmtsHlrCommandStatus result;

            // search to see if already exist
            pos = hlrL3->hlrMap->find(hashKey);
            if (pos != hlrL3->hlrMap->end())
            {
                // found, and only remove if this node is the one who
                // registered the UE in HLR
                hlrEntry = pos->second;

                if (memcmp(&(hlrEntry->sgsnAddr),
                           &srcAddr,
                           sizeof(Address)) == 0)
                {
                    // remove from HLR
                    hlrL3->hlrMap->erase(hashKey);
                    result = UMTS_HLR_COMMAND_SUCC;
                }
                else
                {
                    // don't remove
                    result = UMTS_HLR_COMMAND_FAIL;
                }

                if (DEBUG_TABLE)
                {
                    UmtsLayer3HlrPrintHlr(node, umtsL3);
                }
            }
            else
            {
                // not exist, equal to remove success
                result = UMTS_HLR_COMMAND_SUCC;
            }

            // send reply for the remove request
            UmtsLayer3HlrSendReply(node,
                                   umtsL3,
                                   UMTS_HLR_REMOVE_REPLY,
                                   result,
                                   &tmpImsi,
                                   NULL,
                                   srcAddr,
                                   srcNodeType);
            break;
        }

        case UMTS_HLR_QUERY:
        {
            // query for look up location info of the UE
            if (DEBUG_PACKET)
            {
                printf("QUERY packet to lookup UE %d in HLR\n", hashKey);
            }

            UmtsHlrCommandStatus result;

            // update statistics
            hlrL3->stat.numRoutingQueryRcvd ++;

            // search to see if already exist
            pos = hlrL3->hlrMap->find(hashKey);
            if (pos != hlrL3->hlrMap->end())
            {
                // found
                hlrEntry = pos->second;
                result = UMTS_HLR_COMMAND_SUCC;

                // update statistics
                hlrL3->stat.numRoutingQuerySuccReplySent ++;
            }
            else
            {
                // not found
                hlrEntry = NULL;
                result = UMTS_HLR_COMMAND_FAIL;

                // update statistics
                hlrL3->stat.numRoutingQueryFailReplySent ++;
            }

            // send a reply to the querier
            UmtsLayer3HlrSendReply(node,
                                   umtsL3,
                                   UMTS_HLR_QUERY_REPLY,
                                   result,
                                   &tmpImsi,
                                   hlrEntry,
                                   srcAddr,
                                   srcNodeType);
            break;
        }

        default:
        {
            char errStr[MAX_STRING_LENGTH];

            sprintf(errStr,
                    "UMTS Node%d(HLR) receives request w/ unknown  "
                    "type %d.\n",
                    node->nodeId,
                    pktBuff[0]);
            ERROR_ReportWarning(errStr);
            break;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3HlrHandleCellReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle HLR request packet
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + packet    : char*
// + srcAddr   : Address          : Address of the source node
// + srcNodeType : CellularNodeType : Node type of the source node
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3HlrHandleCellReq(Node *node,
                                UmtsLayer3Data *umtsL3,
                                const char* packet,
                                Address srcAddr,
                                CellularNodeType srcNodeType)
{
    UmtsLayer3Hlr *hlrL3 = (UmtsLayer3Hlr *) (umtsL3->hlrData);

    int index = 0;
    UmtsHlrCommand command = (UmtsHlrCommand) packet[index++];

    if (command == UMTS_HLR_UPDATE_CELL)
    {
        UInt32 cellId;
        NodeId nodebId;
        NodeId rncId;
        NodeId sgsnId;

        memcpy(&cellId, &packet[index], sizeof(cellId));
        index += sizeof(cellId);
        memcpy(&nodebId, &packet[index], sizeof(nodebId));
        index += sizeof(nodebId);
        memcpy(&rncId, &packet[index], sizeof(rncId));
        index += sizeof(rncId);
        memcpy(&sgsnId, &packet[index], sizeof(sgsnId));
        index += sizeof(sgsnId);

        if (DEBUG_BACKBONE)
        {
            printf("Node%d(HLR) received CELL info "
                   " CELLID: %u, NODEBID: %u RNCID: %u SGSNID: %u\n",
                   node->nodeId, cellId, nodebId, rncId, sgsnId);
        }

        UmtsRncCacheMap::iterator it = hlrL3->rncCacheMap->find(rncId);
        if (it == hlrL3->rncCacheMap->end())
        {
            hlrL3->rncCacheMap->insert(
                std::make_pair(rncId,
                               new UmtsRncCacheInfo(
                                   sgsnId,
                                   srcAddr)));
        }
        hlrL3->cellCacheMap->insert(
                    std::make_pair(
                            cellId,
                            new UmtsCellCacheInfo(
                                nodebId,
                                rncId)));

        std::string msgBuf;
        msgBuf.append((char*)&cellId, sizeof(cellId));
        UmtsLayer3HlrSendCellReply(
            node,
            umtsL3,
            UMTS_HLR_UPDATE_CELL_REPLY,
            msgBuf.data(),
            (int)msgBuf.size(),
            srcAddr,
            srcNodeType);
    }
    else if (command == UMTS_HLR_QUERY_CELL)
    {
        UInt32 cellId;
        NodeId queryRncId;
        memcpy(&cellId, &packet[index], sizeof(cellId));
        index += sizeof(cellId);
        memcpy(&queryRncId, &packet[index], sizeof(queryRncId));
        UmtsLayer3HlrHandleCellQuery(
            node,
            umtsL3,
            hlrL3,
            cellId,
            queryRncId,
            srcAddr,
            srcNodeType);
    }
    else
    {
        ERROR_ReportError("Rcvd invalid HLR command type");
    }
}

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: UmtsLayer3HlrHandlePacket
// LAYER      :: Layer 3
// PURPOSE    :: Handle packets received from lower layer
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message containing the packet
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + interfaceIndex : int         : Interface from which packet was received
// + srcAddr   : Address          : Address of the source node
// RETURN     :: void : NULL
// **/
void UmtsLayer3HlrHandlePacket(Node *node,
                               Message *msg,
                               UmtsLayer3Data *umtsL3,
                               int interfaceIndex,
                               Address srcAddr)
{
    //UmtsLayer3Hlr *hlrL3 = (UmtsLayer3Hlr *) (umtsL3->hlrData);
    UmtsBackboneMessageType backboneMsgType;
    char info;
    int infoSize = sizeof(info);
    UmtsRemoveBackboneHeader(node,
                             msg,
                             &backboneMsgType,
                             &info,
                             infoSize);
    UmtsHlrCmdTgt cmdTgt = (UmtsHlrCmdTgt)(info);

    switch (backboneMsgType)
    {
        case UMTS_BACKBONE_MSG_TYPE_GR:
        {
            //Receive a signalling message from SGSN
            if (DEBUG_PACKET)
            {
                printf("UMTS Node%d (HLR) receives a packet from SGSN\n",
                       node->nodeId);
            }

            if (cmdTgt == UMTS_HLR_COMMAND_MS)
            {
                UmtsLayer3HlrHandleRequest(node,
                                           umtsL3,
                                           MESSAGE_ReturnPacket(msg),
                                           interfaceIndex,
                                           srcAddr,
                                           CELLULAR_SGSN);
            }
            else if (cmdTgt == UMTS_HLR_COMMAND_CELL)
            {
                UmtsLayer3HlrHandleCellReq(
                    node,
                    umtsL3,
                    MESSAGE_ReturnPacket(msg),
                    srcAddr,
                    CELLULAR_SGSN);
            }
            else
            {
                ERROR_ReportError("Received a message with invalid"
                    " command target.");
            }
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_GC:
        {
            //Receive a signalling message from GGSN
            break;
        }
        default:
        {
            char infoStr[MAX_STRING_LENGTH];
            sprintf(infoStr,
                    "UMTS Layer3: Node%d(HLR) receives a backbone message "
                    "with an unknown Backbone message type as %d",
                    node->nodeId,
                    backboneMsgType);
            ERROR_ReportWarning(infoStr);
        }
    }
    MESSAGE_Free(node, msg);
}

// /**
// FUNCTION   :: UmtsLayer3HlrInit
// LAYER      :: Layer 3
// PURPOSE    :: Initialize HLR data at UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3HlrInit(Node *node,
                       const NodeInput *nodeInput,
                       UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Hlr *hlrL3;

    // initialize the basic UMTS layer3 data
    hlrL3 = (UmtsLayer3Hlr *) MEM_malloc(sizeof(UmtsLayer3Hlr));
    ERROR_Assert(hlrL3 != NULL, "UMTS: Out of memory!");
    memset(hlrL3, 0, sizeof(UmtsLayer3Hlr));

    umtsL3->hlrData = (void *) hlrL3;

    hlrL3->hlrNumber = node->nodeId;

    // read configuration parameters
    UmtsLayer3HlrInitParameter(node, nodeInput, hlrL3);

    if (DEBUG_PARAMETER)
    {
        UmtsLayer3HlrPrintParameter(node, hlrL3);
    }

    hlrL3->hlrMap = new UmtsHlrMap;
    hlrL3->cellCacheMap = new UmtsCellCacheMap;
    hlrL3->rncCacheMap = new UmtsRncCacheMap;
}

// /**
// FUNCTION   :: UmtsLayer3HlrLayer
// LAYER      :: Layer 3
// PURPOSE    :: Handle HLR specific timers and layer messages.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message for node to interpret
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3HlrLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3)
{
    // start here
    return;
}

// /**
// FUNCTION   :: UmtsLayer3HlrFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print HLR specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3HlrFinalize(Node *node, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Hlr *hlrL3 = (UmtsLayer3Hlr *) (umtsL3->hlrData);

    if (umtsL3->collectStatistics)
    {
        // print HLR specific statistics
        UmtsLayer3HlrPrintStats(node, hlrL3);
    }

    for (UmtsCellCacheMap::iterator it = hlrL3->cellCacheMap->begin();
         it != hlrL3->cellCacheMap->end(); ++it)
    {
        delete it->second;
    }
    delete hlrL3->cellCacheMap;

    for (UmtsRncCacheMap::iterator it = hlrL3->rncCacheMap->begin();
         it != hlrL3->rncCacheMap->end(); ++it)
    {
        delete it->second;
    }
    delete hlrL3->rncCacheMap;

    for (UmtsHlrMap::iterator it = hlrL3->hlrMap->begin();
        it != hlrL3->hlrMap->end(); ++it)
    {
        delete it->second;
    }
    delete hlrL3->hlrMap;

    MEM_free(hlrL3);
    umtsL3->hlrData = NULL;
}
