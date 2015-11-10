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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"

#include "lte_common.h"
#include "log_lte.h"

#define LTE_COMMON_PRINT_ROUTE 0

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: LteGetTxSchemeString
// LAYER      :: LTE
// PURPOSE    :: get the tx scheme as the string.
// PARAMETERS ::
// + mode      : LteTxScheme   : tx scheme.
// RETURN     :: char* : the string of tx scheme.
// **/
const char* LteGetTxSchemeString(LteTxScheme mode)
{
    if (mode == TX_SCHEME_SINGLE_ANTENNA)
    {
        return "SIMO";
    }
    else if (mode == TX_SCHEME_DIVERSITY)
    {
        return "SFBC";
    }
    else if (mode == TX_SCHEME_OL_SPATIAL_MULTI)
    {
        return "OLSM";
    }else
    {
        return "INVALID";
    }
}

// /**
// FUNCTION   :: LteFreeMsg
// LAYER      :: LTE
// PURPOSE    :: free message
// PARAMETERS ::
// + node        : Node*         : pointer to Node
// + msg         : Message**     : message should be deleted
// RETURN     :: void : NULL
// **/
void LteFreeMsg(
        Node* node,
        Message** msg)
{
    ERROR_Assert(*msg, "msg is NULL");
    MESSAGE_Free(node, *msg);
    *msg = NULL;
}



// /**
// FUNCTION   :: LteAddRouteToForwardingTable
// LAYER      :: LTE
// PURPOSE    :: add route to forwarding table of QualNet (call QualNet API)
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + destAddr              : NodeAddress   : destination address
// + destMask              : NodeAddress   : destination mask
// + nextHop               : NodeAddress   : next hop
// + outgoingInterfaceIndex: int           : outgoing interface index
// RETURN     :: void : NULL
// **/
void
LteAddRouteToForwardingTable(
       Node *node,
       NodeAddress destAddr,
       NodeAddress destMask,
       NodeAddress nextHop,
       int outgoingInterfaceIndex)
{
#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, -1,
            LTE_STRING_LAYER_TYPE_RRC,
            LTE_STRING_CATEGORY_TYPE_ROUTING","
            LTE_STRING_FORMAT_RNTI","
            "[add],destAddr,%x,destMask,%x,nextHop,%x,"
            "outGoingInterfaceIndex,%d",
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            destAddr, destMask, nextHop, outgoingInterfaceIndex);
    }
#endif

    NetworkRoutingProtocolType type = ROUTING_PROTOCOL_STATIC;
    int cost = 0;

#if LTE_COMMON_PRINT_ROUTE
    // before
    NetworkPrintForwardingTable(node);
#endif
    NetworkUpdateForwardingTable(
        node,
        destAddr,
        destMask,
        nextHop,
        outgoingInterfaceIndex,
        cost,
        type);

#if LTE_COMMON_PRINT_ROUTE
    // after
    NetworkPrintForwardingTable(node);
#endif
}
// /**
// FUNCTION   :: LteDeleteRouteFromForwardingTable
// LAYER      :: LTE
// PURPOSE    :: delete route from forwarding table of QualNet
//                                              (modify the table directly)
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + destAddr              : NodeAddress   : destination address
// + destMask              : NodeAddress   : destination mask
// RETURN     :: void : NULL
// **/
void
LteDeleteRouteFromForwardingTable(
       Node *node,
       NodeAddress destAddr,
       NodeAddress destMask)
{
#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, -1,
            LTE_STRING_LAYER_TYPE_RRC,
            LTE_STRING_CATEGORY_TYPE_ROUTING","
            LTE_STRING_FORMAT_RNTI","
            "[delete],destAddr,%x,destMask,%x",
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            destAddr, destMask);
    }
#endif

#if LTE_COMMON_PRINT_ROUTE
    // before
    NetworkPrintForwardingTable(node);
#endif

    NetworkRoutingProtocolType type = ROUTING_PROTOCOL_STATIC;

    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    NetworkForwardingTable *rt = &ip->forwardTable;

    int i = 0;

    // Go through the routing table...

    while (i < rt->size)
    {
        // Delete entries that corresponds to the routing protocol used
        if (rt->row[i].protocolType == type
                && rt->row[i].destAddress == destAddr
                && rt->row[i].destAddressMask == destMask)
        {
            int j = i + 1;

            // Move all other entries down
            while (j < rt->size)
            {
                rt->row[j - 1] = rt->row[j];
                j++;
            }

            // Update forwarding table size.
            rt->size--;
        }
        else
        {
            i++;
        }
    }

#if LTE_COMMON_PRINT_ROUTE
    // after
    NetworkPrintForwardingTable(node);
#endif // LTE_COMMON_PRINT_ROUTE
}

// /**
// FUNCTION   :: LteGetPhyIndexFromMacInterfaceIndex
// LAYER      :: LTE
// PURPOSE    :: Get PhyIndex(PhyNumber) from MAC I/F index
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + interfaceIndex        : int           : LTE's MAC I/F index
// RETURN     :: int : PhyIndex
//               if there is no LTE PHY, return ANY_INTERFACE
// **/
int LteGetPhyIndexFromMacInterfaceIndex(
       Node *node,
       int interfaceIndex)
{
    return node->macData[interfaceIndex]->phyNumber;
}

// /**
// FUNCTION   :: LteGetMacInterfaceIndexFromPhyIndex
// LAYER      :: LTE
// PURPOSE    :: Get MAC I/F index from PhyIndex(PhyNumber)
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + phyIndex              : int           : LTE's PhyIndex
// RETURN     :: int : MAC I/F index
//               if there is no LTE MAC, return ANY_INTERFACE
// **/
int LteGetMacInterfaceIndexFromPhyIndex(
       Node *node,
       int phyIndex)
{
    return node->phyData[phyIndex]->macInterfaceIndex;
}

#ifdef ADDON_DB
// /**
// FUNCTION   :: LteMacStatsDBCheckForMsgSequenceNum
// LAYER      :: MAC
// PURPOSE    :: Check for msg seq num, if found 0 then provide a valid value
//               Additionally, should check for msg Id. Add it if not exist.
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// RETURN     :: void   : NULL
// **/
void LteMacStatsDBCheckForMsgSequenceNum(Node* node,
                                         Message* msg)
{
    if (!msg->sequenceNumber)
    {
        msg->sequenceNumber = node->packetTraceSeqno++;
    }

    if (!msg->originatingNodeId)
    {
        msg->originatingNodeId = node->nodeId;
    }
    StatsDBAddMessageMsgIdIfNone(node, msg);
}


// /**
// FUNCTION   :: LteMacAddStatsDBInfo
// LAYER      :: MAC
// PURPOSE    :: Add StatsDB Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// RETURN     :: void   : NULL
// **/
void LteMacAddStatsDBInfo(Node* node,
                          Message* msg)
{
    LteStatsDbSduPduInfo* info = (LteStatsDbSduPduInfo*)MESSAGE_ReturnInfo(
                                     msg,
                                    (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);

    if (info)
    {
        // info already exist, no need to add again
        return;
    }

    info = (LteStatsDbSduPduInfo*)MESSAGE_AddInfo(
                                    node,
                                    msg,
                                    sizeof(LteStatsDbSduPduInfo),
                                    (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);

    ERROR_Assert(info != NULL,
                 "INFO_TYPE_LteStatsDbSduPduInfo adding failed");

    info->dataSize = 0;
    info->ctrlSize = 0;
    info->isCtrlPacket = false;
}


// /**
// FUNCTION   :: LteMacRemoveStatsDBInfo
// LAYER      :: MAC
// PURPOSE    :: Remove StatsDB Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// RETURN     :: void   : NULL
// **/
void LteMacRemoveStatsDBInfo(Node* node,
                          Message* msg)
{
    MESSAGE_RemoveInfo(node, msg, (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);
}


// /**
// FUNCTION   :: LteMacUpdateStatsDBInfo
// LAYER      :: MAC
// PURPOSE    :: Update StatsDB Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// + dataSize           : IN : UInt32   : data size
// + ctrlSize           : IN : UInt32   : control size
// + isCtrlPacket       : IN : BOOL     : 'TRUE' for control packet
// RETURN     :: void   : NULL
// **/
void LteMacUpdateStatsDBInfo(Node* node,
                             Message* msg,
                             UInt32 dataSize,
                             UInt32 ctrlSize,
                             BOOL isCtrlPacket)
{
    LteStatsDbSduPduInfo* info = (LteStatsDbSduPduInfo*)MESSAGE_ReturnInfo(
                                     msg,
                                    (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);


    if (!info)
    {
        char errStr[MAX_STRING_LENGTH];
        sprintf(errStr, "INFO_TYPE_LteStatsDbSduPduInfo not found while "
                        " update, on Node %d.\n",
                        node->nodeId);
        ERROR_ReportWarning(errStr);
        return;
    }

    info->dataSize = dataSize;
    info->ctrlSize = ctrlSize;
    info->isCtrlPacket = isCtrlPacket;
}


// /**
// FUNCTION   :: LteMacAppendStatsDBControlInfo
// LAYER      :: MAC
// PURPOSE    :: Append StatsDB Control Size Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// + ctrlSize           : IN : UInt32   : control size
// RETURN     :: void   : NULL
// **/
void LteMacAppendStatsDBControlInfo(Node* node,
                                    Message* msg,
                                    UInt32 ctrlSize)
{
    LteStatsDbSduPduInfo* info = (LteStatsDbSduPduInfo*)MESSAGE_ReturnInfo(
                                     msg,
                                    (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);
    if (!info)
    {
        char errStr[MAX_STRING_LENGTH];
        sprintf(errStr, "INFO_TYPE_LteStatsDbSduPduInfo not found while "
                        " append, on Node %d.\n",
                        node->nodeId);
        ERROR_ReportWarning(errStr);
        return;
    }
    info->ctrlSize += ctrlSize;
}


// /**
// FUNCTION   :: LteMacReduceStatsDBControlInfo
// LAYER      :: MAC
// PURPOSE    :: Substract StatsDB Control Size Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// + ctrlSize           : IN : UInt32   : control size
// RETURN     :: void   : NULL
// **/
void LteMacReduceStatsDBControlInfo(Node* node,
                                    Message* msg,
                                    UInt32 ctrlSize)
{
    LteStatsDbSduPduInfo* info = (LteStatsDbSduPduInfo*)MESSAGE_ReturnInfo(
                                     msg,
                                    (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);

    if (!info)
    {
        char errStr[MAX_STRING_LENGTH];
        sprintf(errStr, "INFO_TYPE_LteStatsDbSduPduInfo not found while "
                        " reduce, on Node %d.\n",
                        node->nodeId);
        ERROR_ReportWarning(errStr);
        return;
    }
    info->ctrlSize = info->ctrlSize - ctrlSize;
}


// /**
// FUNCTION   :: LteMacSetStatsDBInfoInFragMsg
// LAYER      :: MAC
// PURPOSE    :: Reset StatsDB Info in the Fragmented Messages
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : Original message pointer
// + fragHead           : IN : Message* : First fragemented message pointer
// + fragTail           : IN : Message* : Second fragemented Message pointer
// + restSize           : IN : UInt32   : Size limt for fragHead
// + dataSizeHead       : IN : UInt32&  : Return data size of final fragHead
// + ctrlSizeHead       : IN : UInt32&  : Return ctrl size of final fragHead
// + addStatsDbInfo     : IN : BOOL     : if TRUE, then add info to frag msgs
//                                        Default value: FALSE
// RETURN     :: void   : NULL
// **/
void LteMacSetStatsDBInfoInFragMsg(Node* node,
                                   Message* msg,
                                   Message* fragHead,
                                   Message* fragTail,
                                   UInt32 restSize,
                                   UInt32& dataSizeHead,
                                   UInt32& ctrlSizeHead,
                                   BOOL addStatsDbInfo)
{
    LteStatsDbSduPduInfo* msgInfo = NULL;

    if (!msg)
    {
        // assuming fragHead and fragTail will already have
        // the copy of original message stats-db info and its values
        addStatsDbInfo = FALSE;
    }
    else
    {
        if (addStatsDbInfo)
        {
            msgInfo = (LteStatsDbSduPduInfo*)MESSAGE_ReturnInfo(
                                     msg,
                                    (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);
            ERROR_Assert(msgInfo != NULL,
                     "INFO_TYPE_LteStatsDbSduPduInfo not found in message");
        }
    }

    if (addStatsDbInfo)
    {
        LteMacAddStatsDBInfo(node, fragHead);
        LteMacAddStatsDBInfo(node, fragTail);
    }

    LteStatsDbSduPduInfo* headInfo = (LteStatsDbSduPduInfo*)
                            MESSAGE_ReturnInfo(
                                     fragHead,
                                     (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);

    ERROR_Assert(headInfo != NULL,
                 "INFO_TYPE_LteStatsDbSduPduInfo not found in fragHead");

    LteStatsDbSduPduInfo* tailInfo = (LteStatsDbSduPduInfo*)
                            MESSAGE_ReturnInfo(
                                     fragTail,
                                     (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);

    ERROR_Assert(tailInfo != NULL,
                 "INFO_TYPE_LteStatsDbSduPduInfo not found in fragTail");

    if (addStatsDbInfo)
    {
        headInfo->dataSize = msgInfo->dataSize;
        headInfo->ctrlSize = msgInfo->ctrlSize;
        headInfo->isCtrlPacket = msgInfo->isCtrlPacket;

        tailInfo->dataSize = msgInfo->dataSize;
        tailInfo->ctrlSize = msgInfo->ctrlSize;
        tailInfo->isCtrlPacket = msgInfo->isCtrlPacket;
    }

    // Now set/reset stats-db info values
    if (headInfo->ctrlSize >= restSize)
    {
        headInfo->ctrlSize = restSize;
        headInfo->dataSize = 0;
        tailInfo->ctrlSize = tailInfo->ctrlSize - restSize;
    }
    else if (headInfo->ctrlSize == 0)
    {
        headInfo->dataSize = restSize;
        tailInfo->dataSize = tailInfo->dataSize - restSize;
    }
    else
    {
        UInt32 leftoverSize = restSize - headInfo->ctrlSize;
        headInfo->dataSize = leftoverSize;
        tailInfo->ctrlSize = 0;
        tailInfo->dataSize = tailInfo->dataSize - leftoverSize;
    }

    dataSizeHead = headInfo->dataSize;
    ctrlSizeHead = headInfo->ctrlSize;
}
#endif
