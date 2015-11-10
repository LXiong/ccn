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
//
// Name: odmrp.cpp
// Purpose: To simulate On-Demand Multicast Routing Protocol (ODMRP)
//
// /**
// PROTOCOL   :: ODMRP
// LAYER    ::   Network
// REFERENCES :: None
// COMMENTS   :: None
// **/
//
//
//-----------------------------------------------------------------


// Bundling Macro: please add any public functions here.
// This will be stripped from the source file that is
// released as part of the bundle.

#ifdef DISABLED
#include "api.h"
void OdmrpInit(Node* node,
               const NodeInput* nodeInput,
               int interfaceIndex) {}
void OdmrpFinalize(Node* node) {}
void OdmrpHandleProtocolPacket(Node* node,
                               Message* msg,
                               NodeAddress srcAddr,
                               NodeAddress destAddr){}

void OdmrpHandleProtocolEvent(Node* node, Message* msg) {}
void OdmrpLeaveGroup(Node* node, NodeAddress mcastAddr) {}
void OdmrpJoinGroup(Node* node,
                    NodeAddress mcastAddr) {}
void OdmrpPrintTraceXML(Node* node, Message* msg) {}
BOOL
OdmrpCheckIfItIsDuplicatePacket(Node* node, Message* msg) { return 0; }

#else


#define ODMRP_DEBUG     0
#define ODMRP_PRINT_TRACE 0
#define ODMRP_PASSIVE_DEBUG 0

//Comment this to avoid piggybacking data packet for peroidic Join Query
//#define PIGGYBACKING

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "network_ip.h"
#include "multicast_odmrp.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

//----------------------------------------------------------------

static
void OdmrpActivatePassiveClustering(OdmrpData* odmrp,
                                    Node* node,
                                    const Message*  msg);
static
void OdmrpUpdateExternalState(Node* node,
                              OdmrpData* odmrp);

// /**
// FUNCTION   :: OdmrpPrintTraceXML
// LAYER      :: NETWORK
// PURPOSE    :: Printing Odmrp Specific Trace
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + msg    : Message* : Pointer to Message
// RETURN ::  void : NULL
// **/

void OdmrpPrintTraceXML(Node* node, Message* msg)
{
    char buf[MAX_STRING_LENGTH];
    char addr1[20];
    OdmrpJoinReply*     odmrpJoinReply = 0;
    OdmrpAck*           odmrpAckPkt = 0;
    OdmrpJoinTupleForm* tupleForm = 0;
    unsigned char*      pktType = 0;
    OdmrpJoinQuery * odmrpJoinQuery = 0;
    pktType = (unsigned char *) MESSAGE_ReturnPacket(msg);

    sprintf(buf, "<odmrp>");
    TRACE_WriteToBufferXML(node, buf);

    switch (*pktType)
    {
        case ODMRP_JOIN_QUERY:
            odmrpJoinQuery = (OdmrpJoinQuery *) MESSAGE_ReturnPacket(msg);
            sprintf(buf, "<joinQuery>%hu",odmrpJoinQuery->pktType);
            TRACE_WriteToBufferXML(node, buf);
            sprintf(buf, " %hu", odmrpJoinQuery->reserved);
            TRACE_WriteToBufferXML(node, buf);
            sprintf(buf, " %hu", odmrpJoinQuery->ttlVal);
            TRACE_WriteToBufferXML(node, buf);
            sprintf(buf, " %hu", odmrpJoinQuery->hopCount);
            TRACE_WriteToBufferXML(node, buf);
            sprintf(buf, " %hu", odmrpJoinQuery->mcastAddress);
            TRACE_WriteToBufferXML(node, buf);
            sprintf(buf, " %d",odmrpJoinQuery->seqNumber);
            TRACE_WriteToBufferXML(node, buf);
            sprintf(buf, " %hu", odmrpJoinQuery->srcAddr);
            TRACE_WriteToBufferXML(node, buf);
            sprintf(buf, " %hu",odmrpJoinQuery->lastAddr);
            TRACE_WriteToBufferXML(node, buf);
            sprintf(buf, " %hu", odmrpJoinQuery->protocol);
            TRACE_WriteToBufferXML(node, buf);

            sprintf(buf, "</joinQuery>");
            TRACE_WriteToBufferXML(node, buf);
            break;
        case ODMRP_ACK:
            odmrpAckPkt  = (OdmrpAck *) MESSAGE_ReturnPacket(msg);

            sprintf(buf, "<ack>%hu ", odmrpAckPkt->pktType);
            TRACE_WriteToBufferXML(node, buf);

            IO_ConvertIpAddressToString(odmrpAckPkt->mcastAddr, addr1);
            sprintf(buf, "%s ", addr1);
            TRACE_WriteToBufferXML(node, buf);

            IO_ConvertIpAddressToString(odmrpAckPkt->srcAddr, addr1);
            sprintf(buf, "%s", addr1);
            TRACE_WriteToBufferXML(node, buf);

            sprintf(buf, "</ack>");
            TRACE_WriteToBufferXML(node, buf);
            break;
        case ODMRP_JOIN_REPLY:
            unsigned int i;
            odmrpJoinReply = (OdmrpJoinReply *)
                MESSAGE_ReturnPacket(msg);
            sprintf(buf, "<joinReply>%hu ", odmrpJoinReply->pktType);
            TRACE_WriteToBufferXML(node, buf);
            sprintf(buf, "%hu %hu %hu %hu ", odmrpJoinReply->replyCount,
                                OdmrpJoinReplyGetAckReq(odmrpJoinReply->ojr),
                                OdmrpJoinReplyGetIAmFG(odmrpJoinReply->ojr),
                                OdmrpJoinReplyGetReserved(odmrpJoinReply->ojr));
            TRACE_WriteToBufferXML(node, buf);

            IO_ConvertIpAddressToString(
                                odmrpJoinReply->multicastGroupIPAddr,
                                addr1);
            sprintf(buf, "%s ", addr1);
            TRACE_WriteToBufferXML(node, buf);

            IO_ConvertIpAddressToString(
                                odmrpJoinReply->previousHopIPAddr,
                                addr1);
            sprintf(buf, "%s ",addr1);
            TRACE_WriteToBufferXML(node, buf);

            sprintf(buf, "%d ", odmrpJoinReply->seqNum);
            TRACE_WriteToBufferXML(node, buf);

            tupleForm = (OdmrpJoinTupleForm *) (odmrpJoinReply + 1);

            for (i = 0; i < odmrpJoinReply->replyCount; i++)
            {

                IO_ConvertIpAddressToString(tupleForm[i].srcAddr, addr1);
                sprintf(buf, "<joinTuple>%s ", addr1);
                TRACE_WriteToBufferXML(node, buf);

                IO_ConvertIpAddressToString(tupleForm[i].nextAddr, addr1);
                sprintf(buf, "%s</joinTuple>", addr1);
                TRACE_WriteToBufferXML(node, buf);
            }

            sprintf(buf, "</joinReply>");
            TRACE_WriteToBufferXML(node, buf);
            break;
        default:
            ERROR_Assert(FALSE,
                "ODMRP received packet of unknown type\n");
     }
    sprintf(buf, "</odmrp>");
    TRACE_WriteToBufferXML(node, buf);
} // End of OdmrpPrintTraceXML Function

// /**
// FUNCTION   :: OdmrpInitTrace
// LAYER      :: NETWORK
// PURPOSE    :: Initialization Odmrp Specific Trace
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + nodeInput    : NodeInput* : Pointer to NodeInput
// RETURN ::  void : NULL
// **/

static
void OdmrpInitTrace(Node* node, const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL traceAll = TRACE_IsTraceAll(node);
    BOOL trace = FALSE;
    static BOOL writeMap = TRUE;


    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-ODMRP",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            trace = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            trace = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "TRACE-ODMRP should be either \"YES\" or \"NO\".\n");
        }
    }
    else
    {
        if (traceAll || node->traceData->layer[TRACE_NETWORK_LAYER])
        {
            trace = TRUE;
        }
    }

    if (trace)
    {
        TRACE_EnableTraceXML(node, TRACE_ODMRP,
                "ODMRP", OdmrpPrintTraceXML, writeMap);
    }
    else
    {
        TRACE_DisableTraceXML(node, TRACE_ODMRP,
                "ODMRP", writeMap);
    }
    writeMap = FALSE;
}

//-----------------------------------------------------
// FUNCTION     OdmrpPeekFunction
// PURPOSE      While a source node receive a multicast packet with source address
//              same as that of source node, then this function process that packet.
//
// Parameters:
//  node       : Node
//  msg        : Massage to be processed
//  prevoiusHop: Address of prevoius hop
//----------------------------------------------------
void OdmrpPeekFunction(Node *node, const Message *msg, NodeAddress previousHop)
{

    if (ODMRP_DEBUG)
    {
        printf("\n in peek function Node =%d\n",node->nodeId);
    }

    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);

    IpHeaderType* ipHeader = (IpHeaderType *) msg->packet;


    if ((ipHeader->ip_p == IPPROTO_ODMRP)
        && (ipHeader->ip_src == NetworkIpGetInterfaceAddress(node,
        GetDefaultInterfaceIndex(node, NETWORK_IPV4))))
    {
        OdmrpActivatePassiveClustering(
            odmrp,
            node,
            msg);
    }
}// end of OdmrpPeekFunction


static
void OdmrpPrintHead(OdmrpClsHeadList* clsHeadList)
{
    OdmrpHeadList*  current = clsHeadList->head;
    if (current == NULL)
        return;
    while (current != NULL)
    {
        printf("Cluster Head %d\n",current->nodeId);
        current = current->next;
    }
    return;
}

static
void OdmrpPrintClsHeadPair(OdmrpClsHeadPair* headPair)
{
    while (headPair != NULL)
    {

        printf("1st cluster Head %d\n",headPair->clsId1);
        printf("2nd cluster Head %d\n",headPair->clsId2);
        headPair = headPair->next;
    }
    return;
}
//-----------------------------------------------------
// FUNCTION     OdmrpPrintGateway
// PURPOSE      Print Gateway Information
//
// Parameters:
//  clsDistGwList: List of  Gateways
//----------------------------------------------------
static
void OdmrpPrintGateway(OdmrpClsGwList* clsGwList)
{
    OdmrpGwList*  current = clsGwList->head;
    if (current == NULL)
        return;
    while (current != NULL)
    {
        printf("Gateway Node Head %d\n",current->nodeId);
        current = current->next;
    }
    return;
} // End of OdmrpPrintGateway

//-----------------------------------------------------
// FUNCTION     OdmrpPrintDistGateway
// PURPOSE      Print Distributed Gateway Information
//
// Parameters:
//  clsDistGwList: List of Distributed Gateway
//----------------------------------------------------
static
void OdmrpPrintDistGateway(OdmrpClsDistGwList* clsDistGwList)
{
    OdmrpDistGwList*  current = clsDistGwList->head;
    if (current == NULL)
        return;
    while (current != NULL)
    {
        printf("Gateway Node Head %d\n",current->nodeId);
        printf("Cluster Head-1 is %d \n",current->clsId1);
        printf("Cluster Head-2 is %d \n",current->clsId2);
        current = current->next;
    }
    return;
}// End of OdmrpPrintDistGateway

//-----------------------------------------------------
// FUNCTION     OdmrpValidateTimer
// PURPOSE      Validate Timer Value
//
// Parameters:
//  valueRead: Timer value to be checked
//----------------------------------------------------

static
void OdmrpValidateTimer(clocktype valueRead)
{
    char errStr[MAX_STRING_LENGTH] = {0};

    if (valueRead <= 0)
    {
        sprintf(errStr, "Timer Value of is wrongly set\n"
            "Must be greater than zero\n");
        ERROR_ReportError(errStr);
    }
    return;

} // End of OdmrpValidateTimer Function

//-----------------------------------------------------
// FUNCTION     OdmrpValidateExpireTime
// PURPOSE      Validate Forwarding Group Expire Time
//
// Parameters:
//  refreshTime: Join Query Refresh Time
//   expireTime: Forwarding Group Expire Time
//----------------------------------------------------

static
void OdmrpValidateExpireTime(clocktype refreshTime,
                             clocktype expireTime)
{
    char errStr[MAX_STRING_LENGTH] = {0};

    if (expireTime <= refreshTime)
    {
        sprintf(errStr, "Forwarding Group Expire Time value must"
            "be greater than Join Query Refresh Time\n");
        ERROR_ReportError(errStr);
    }
    return;

} // End of OdmrpValidateExpireTime Function

//-----------------------------------------------------
// FUNCTION     OdmrpValidateTTLValue
// PURPOSE      Check the validity the TTL (Time To Live)
//              value
// Parameters:
//     ttlValue : TTL value
//   lowestValue: Lower range of TTL value
//  highestValue: Higher range of TTL value
//----------------------------------------------------

static
void OdmrpValidateTTLValue(int ttlValue,
                           int  lowestValue,
                           int  highestValue)
{
    char errStr[MAX_STRING_LENGTH] = {0};

    if (ttlValue < lowestValue)
    {
        sprintf(errStr, "TTL Value is wrongly set to (%d)\n"
            "Must be greater than the lowest value zero\n",
            ttlValue);
        ERROR_ReportError(errStr);
    }

    if (ttlValue > highestValue)
    {
        sprintf(errStr, "TTL Value is wrongly set to (%d)\n"
            "Must not be greater than the highest value (%d)\n",
            ttlValue,
            highestValue);

        ERROR_ReportError(errStr);
    }

} // End of OdmrpValidateTTLValue Function

//---------------------------------------------------------
// FUNCTION     OdmrpPopulateReplyWithMemTable
// PURPOSE      Populate Reply Tuple with Member Table
//
// Parameters:
// tupleForm : Join Tuple Form
// buffer    : Pointer to Data Buffer Structure
// mcastAddr : Multicast Address
//----------------------------------------------------------

static
void  OdmrpPopulateReplyWithMemTable(OdmrpJoinTupleForm* tupleForm,
                                     DataBuffer* buffer,
                                     NodeAddress mcastAddr)
{
    int i;
    int count;
    int numSrc;
    OdmrpMtNode*  memberTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    OdmrpMtSnode*  memSrcTable;
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpMtNode);

    for (i = 0; i < numEntry; i++)
    {
        if ((memberTable[i]).mcastAddr == mcastAddr)
        {
            memSrcTable = (OdmrpMtSnode *)
                BUFFER_GetData(&(memberTable[i].head));
            numSrc = BUFFER_GetCurrentSize(&(memberTable[i].head))
                / sizeof(OdmrpMtSnode);

            for (count = 0; count < numSrc; count++)
            {
                tupleForm[count].srcAddr
                    = memSrcTable[count].srcAddr;

            }
        }
    }

} // End of Function OdmrpPopulateReplyWithMemTable

//-------------------------------------------------------
// FUNCTION     OdmrpInitializeBuffer
// PURPOSE      Initialise Odmrp DataBuffer Structure
//
// Parameters:
//    odmrp : Pointer to OdmrpData
//------------------------------------------------------

static
void OdmrpInitializeBuffer(OdmrpData* odmrp)
{
    //Initialize Ack Table
    BUFFER_InitializeDataBuffer(&(odmrp->ackTable),
        sizeof(OdmrpAtNode)
        * ODMRP_INITIAL_ENTRY);

    //Initialize Forwarding Table
    BUFFER_InitializeDataBuffer(&(odmrp->fgFlag),
        sizeof(OdmrpFgFlag)
        * ODMRP_INITIAL_ENTRY);

    //Initialize Member Table
    BUFFER_InitializeDataBuffer(&(odmrp->memberTable),
        sizeof(OdmrpMtNode)
        * ODMRP_INITIAL_ENTRY);

    //Initialize Response Table
    BUFFER_InitializeDataBuffer(&(odmrp->responseTable),
        sizeof(OdmrpRptNode)
        * ODMRP_INITIAL_ENTRY);

    //Initialize Temp Table
    BUFFER_InitializeDataBuffer(&(odmrp->tempTable),
        sizeof(OdmrpTtNode)
        * ODMRP_INITIAL_ENTRY);

    // Initialize Route Table
    BUFFER_InitializeDataBuffer(&(odmrp->routeTable),
        sizeof(OdmrpRt)
        * ODMRP_INITIAL_ROUTE_ENTRY);

    // Initialize Sent Table
    BUFFER_InitializeDataBuffer(&(odmrp->sentTable),
        sizeof(OdmrpSs)
        * ODMRP_INITIAL_ENTRY);

    // Initialize MemberShip Table
    BUFFER_InitializeDataBuffer(&(odmrp->memberFlag),
        sizeof(OdmrpMembership)
        * ODMRP_INITIAL_ENTRY);
    return;

} // End of OdmrpInitializeBuffer Function

//---------------------------------------------------------
// FUNCTION     OdmrpInitMessageCache
// PURPOSE      Initialize the message cache.
//
// Parameters:
//      messageCache: Pointer to Message Cache structure
//---------------------------------------------------------

static
void OdmrpInitMessageCache(OdmrpMc* messageCache)
{
    OdmrpCacheTable* cacheTable = 0;
    int          i;

    messageCache->cacheTable = (OdmrpCacheTable *) MEM_malloc
        (ODMRP_MAX_HASH_KEY_VALUE * sizeof(OdmrpCacheTable));
    cacheTable = (OdmrpCacheTable *)messageCache->cacheTable;

    for (i = 0; i < ODMRP_MAX_HASH_KEY_VALUE; i++)
    {
        cacheTable[i].front = NULL;
        cacheTable[i].rear = NULL;
    }

} // End of OdmrpInitMessageCache Function

//--------------------------------------------------------------
// FUNCTION     OdmrpInitStats
// PURPOSE      Initialize all the stats variables.
//
// Parameters:
//     node: Node that is initializing the stats variables.
//--------------------------------------------------------------

static
void OdmrpInitStats(OdmrpData* odmrp)
{

    // Total number of Join Query message forwarded
    odmrp->stats.numQueryTxed = 0;

    //Total number fo Join Query originated
    odmrp->stats.numQueryOriginated = 0;

    // Total number of join Reply packets originated/Retransmitted
    odmrp->stats.numReplySent = 0;
    odmrp->stats.numReplyTransmitted = 0;

    // Total number of join Reply packets forwarded
    odmrp->stats.numReplyForwarded = 0;

    // Total number of explicit acks sent.
    odmrp->stats.numAckSent = 0;

    // Total number of data packets sent by the source.
    odmrp->stats.numDataSent = 0;

    // Total number of data packets received by the destination.
    odmrp->stats.numDataReceived = 0;

    odmrp->stats.numDataTxed = 0;

    if (odmrp->pcEnable)
    {
        // Total no of Give up message sent and received is zero
        odmrp->stats.numGiveUpTx = 0;
        odmrp->stats.numGiveUpRx = 0;
    }

} // End of OdmrpInitStats Function

//--------------------------------------------------------
// FUNCTION     OdmrpInitStates
// PURPOSE      Initialize States of the node supporting
//              Passive Clustering
//
// Parameters:
//--------------------------------------------------------

static
void OdmrpInitStates(OdmrpData* odmrp)
{
    odmrp->internalState = (unsigned char) ODMRP_UNKNOWN_STATE;
    odmrp->externalState = (unsigned char) ODMRP_INITIAL_NODE;
    return;
} // End of OdmrpInitStates Function

//--------------------------------------------------------
// FUNCTION     OdmrpInitClusterHead
// PURPOSE      Initialize Cluster Head Related Information
//
// Parameters:
//     node: Node that is initializing Cluster Head Related
//           Information.
//--------------------------------------------------------

static
void OdmrpInitClusterHead(OdmrpClsHeadList* clsHeadList)
{

    clsHeadList->head = NULL;
    clsHeadList->size = 0;
} // End of OdmrpInitClusterHead Function

//--------------------------------------------------------
// FUNCTION     OdmrpInitGateway
// PURPOSE      Initialize Gateway Related Information
//
// Parameters:
//     node: Node that is initializing Gateway Realated
//           Information.
//--------------------------------------------------------

static
void OdmrpInitGateway(OdmrpClsGwList* clsGwList)
{
    clsGwList->head = NULL;
    clsGwList->size = 0;
} // End of OdmrpInitGateway Function

//--------------------------------------------------------
// FUNCTION     OdmrpInitDistGateway
// PURPOSE      Initialize Dist Gateway Related Information
//
// Parameters:
//     node: Node that is initializing Gateway Realated
//           Information.
//--------------------------------------------------------

static
void OdmrpInitDistGateway(OdmrpClsDistGwList* clsDistGwList)
{
    clsDistGwList->head = NULL;
    clsDistGwList->size = 0;
} // End of OdmrpInitDistGateway Function

//--------------------------------------------------------
// FUNCTION     OdmrpInitGatewayEntry
// PURPOSE      Initialize Gateway Entry. Generally Gateway
//              Entry consists of Node Ids of two
//              Cluster Heads or Node Id of its
//              Primary Cluster and Node Id of DIST_GW
//
// Parameters:
//     node: Node that is initializing Gateway Realated
//           Information.
//--------------------------------------------------------

static
void OdmrpInitGatewayEntry(OdmrpGwClsEntry* gwEntry)
{
    gwEntry->clsId1 = 0;
    gwEntry->clsId2 = 0;
}// End of OdmrpInitGatewayEntry Function


//--------------------------------------------------------------
// FUNCTION     OdmrpSetTimer
// PURPOSE      Install ODMRP Timer for an event
//              to occur after a certain interval of
//              time
//
// Parameters:
//   eventType  : Type of the event
//   messageInfo: Void Pointer
//      infoSize: Size of Message Info
//       delay  : The time delay after the event will
//                occur
//-------------------------------------------------------------

static
void OdmrpSetTimer(Node* node,
                   Int32 eventType,
                   void*  messageInfo,
                   unsigned int infoSize,
                   clocktype delay)
{
    Message* msg = MESSAGE_Alloc(node,
        NETWORK_LAYER,
        MULTICAST_PROTOCOL_ODMRP,
        eventType);

    MESSAGE_InfoAlloc(node, msg, infoSize);
    memcpy(MESSAGE_ReturnInfo(msg), messageInfo, infoSize);

    MESSAGE_Send(node, msg, delay);
} // End of OdmrpSetTimer Function

//-----------------------------------------------------------
// FUNCTION     OdmrpGetIpOptions
// PURPOSE      Extract the IP Option field from the
//              packet.
//
// Parameters:
//      msg         : Pointer to message structure
//---------------------------------------------------

static
OdmrpIpOptionType OdmrpGetIpOptions(const Message* msg)
{
    IpOptionsHeaderType* ipOption =
        FindAnIpOptionField((IpHeaderType*)msg->packet, IPOPT_ODMRP);
    OdmrpIpOptionType OdmrpIpOption;

    //if ipotion is not found return empty option.
    memset (&OdmrpIpOption, 0, sizeof(OdmrpIpOptionType));

    if (!ipOption)
    {
            // return empty option;
            return OdmrpIpOption;
    }

    memcpy(&OdmrpIpOption,
        ((char*)ipOption + sizeof(IpOptionsHeaderType)),
        sizeof(OdmrpIpOptionType));
    return OdmrpIpOption;
} // End of OdmrpGetIpOptions Function

//----------------------------------------------------
// FUNCTION     OdmrpGetQueryOptions
// PURPOSE      Extract Query Options field from IP Packet
//
//
// Parameters:
//      msg         : Pointer to message structure
//-----------------------------------------------------

static
OdmrpJoinQuery OdmrpGetQueryOptions(const Message* msg)
{
    IpOptionsHeaderType* ipOption =
        FindAnIpOptionField((IpHeaderType*)msg->packet,
        IPOPT_ODMRP);
    OdmrpJoinQuery OdmrpIpOption;
    memcpy(&OdmrpIpOption,
        ((char*)ipOption + sizeof(IpOptionsHeaderType)),
        sizeof(OdmrpJoinQuery));
    return OdmrpIpOption;

}//End of OdmrpGetQueryOptions Function

//----------------------------------------------------
// FUNCTION     OdmrpSetIpOptions
// PURPOSE      Set IP Option field of the packet
//
// Parameters:
//      msg  : Pointer to message structure
//  odmrpIpOption : Pointer to IP Option field
//---------------------------------------------------

static
void OdmrpSetIpOptions(Message* msg,
                       const OdmrpIpOptionType* OdmrpIpOption)
{
    IpOptionsHeaderType* ipOption =
        FindAnIpOptionField((IpHeaderType*)msg->packet, IPOPT_ODMRP);
    memcpy(((char*)ipOption + sizeof(IpOptionsHeaderType)),
        OdmrpIpOption,
        sizeof(OdmrpIpOptionType));
}// End of OdmrpSetIpOptions Function

//------------------------------------------------------------
// FUNCTION     OdmrpSetQueryOptions
// PURPOSE      Set IP Option field of the packet
//
// Parameters:
//      msg  : Pointer to message structure
//  odmrpIpOption : Pointer to IP Query Options
//------------------------------------------------------------

static
void OdmrpSetQueryOptions(Message* msg,
                          const OdmrpJoinQuery* OdmrpIpOption)
{
    IpOptionsHeaderType* ipOption =
        FindAnIpOptionField((IpHeaderType*)msg->packet, IPOPT_ODMRP);
    memcpy(((char*)ipOption + sizeof(IpOptionsHeaderType)),
        OdmrpIpOption,
        sizeof(OdmrpJoinQuery));
} // End of OdmrpSetQueryOptions Function

//----------------------------------------------
// FUNCTION     OdmrpGiveUpClusterHead
// PURPOSE      Delete Cluster Head from Cluster Head List
//              when giveup message come
// Paremeters:
//     odmrp
// clsHeadList:  Pointer to the list of Cluster Head
//       nodeId: Node Idof the Packet
// deleteCH : Cluster head to be deleted.
//------------------------------------------------

static
void  OdmrpGiveUpClusterHead(OdmrpData* odmrp,
                             Node* node,
                             int deleteCH)
{
    OdmrpClsHeadList* clsHeadList;
    clsHeadList = &odmrp->clsHeadList;
    OdmrpHeadList* toFree = 0;
    OdmrpHeadList* current = 0;
    OdmrpHeadList* temp = 0;

    if (clsHeadList->size == 0 || clsHeadList->head == NULL)
    {
        return;
    }

    temp = clsHeadList->head;
    current = clsHeadList->head;

    while (current != NULL)
    {
        if (current->nodeId == (unsigned) deleteCH)
        {
            toFree = current;

            // The first node exists only
            if (clsHeadList->size == 1)
            {
                clsHeadList->head = NULL;
                MEM_free(toFree);
                --(clsHeadList->size);
                return;

            }
            else if (temp == current)
            {
                clsHeadList->head = temp->next;
                temp = temp->next;
            }
            else if (current->next == NULL)
            {
                temp->next = NULL;
            }
            else
            {
                temp->next = current->next;
            }
            current = current->next;
            MEM_free(toFree);
            --(clsHeadList->size);
        }
        else
        {
            temp = current;
            current = current->next;
        }
    }
} // End of OdmrpGiveUpClusterHead Function
// end

//------------------------------------------------------------
// FUNCTION     OdmrpCreatePassiveClsPacket
// PURPOSE      Populate Passive Cluster Related Information
//              where External State of the node is an INITIAL
//              NODE.
// Parameters:
//    node : Pointer to node structure
//    msg  : Pointer to Message Structure
//------------------------------------------------------------

static
void OdmrpCreatePassiveClsPacket(OdmrpData* odmrp,
                                 Node* node,
                                 Message* msg)
{
    IpOptionsHeaderType* ipOption =
        FindAnIpOptionField((IpHeaderType*)msg->packet,
        IPOPT_ODMRP_CLUSTER);
    OdmrpPassiveClsPkt   passiveClsPkt;
    OdmrpGwClsEntry*     gwClsEntry = 0;

    if (ipOption == NULL)
    {
        AddIpOptionField(node,
            msg,
            IPOPT_ODMRP_CLUSTER,
            sizeof(OdmrpPassiveClsPkt));
    }
    ipOption =
        FindAnIpOptionField((IpHeaderType*)msg->packet,
        IPOPT_ODMRP_CLUSTER);

    if (ODMRP_DEBUG)
    {
        printf("Node %u: IPOPT_ODMRP_CLUSTER option key added \n",
         node->nodeId);
    }

    OdmrpPassiveClsPktSetState(&(passiveClsPkt.opcp), odmrp->externalState);
    OdmrpPassiveClsPktSetGiveUpFlag(&(passiveClsPkt.opcp), FALSE);
    OdmrpPassiveClsPktSetHelloFlag(&(passiveClsPkt.opcp), FALSE);
    OdmrpPassiveClsPktSetReserved(&(passiveClsPkt.opcp), 0);
    passiveClsPkt.nodeId = node->nodeId;

    switch (odmrp->externalState)
    {
        case ODMRP_ORDINARY_NODE:
        case ODMRP_INITIAL_NODE:
        case ODMRP_CLUSTER_HEAD:
            passiveClsPkt.clsId1 = 0;
            passiveClsPkt.clsId2 = 0;
            break;
        case ODMRP_FULL_GW:
        case ODMRP_DIST_GW:
            gwClsEntry  = &(odmrp->gwClsEntry);
            passiveClsPkt.clsId1 = gwClsEntry->clsId1;
            passiveClsPkt.clsId2 = gwClsEntry->clsId2;
            break;
        default:
            ERROR_ReportError("Unknown External State of the node\n");
            break;
        }

    memcpy(((char*)ipOption + sizeof(IpOptionsHeaderType)),
        &passiveClsPkt,
        sizeof(OdmrpPassiveClsPkt));
    return;
} // End of OdmrpCreatePassiveClsPacket Function

//--------------------------------------------------------
// FUNCTION     OdmrpSendFreePool
// PURPOSE      Remove an entry from the message cache
//
// Parameters:
//     messageCache:  Pointer to Message Cache structure
//   OdmrpCacheDeleteEntry :  Pointer to Message Delete structure
//---------------------------------------------------------

static
void OdmrpSendFreePool(MemPool* memoryPool,
                       OdmrpMsgCacheEntry* cacheEntry)
{
    int* saveCurrentPointer;
    OdmrpFreeBufferPool* freePool
        = (OdmrpFreeBufferPool *)cacheEntry;
    saveCurrentPointer = memoryPool->currentPointer;
    memoryPool->currentPointer = (int *)freePool;
    freePool->nextPointer = (int *)saveCurrentPointer;

} // End of OdmrpSendFreePool Function

//--------------------------------------------------------
// FUNCTION     OdmrpDeleteMsgCache
// PURPOSE      Remove an entry from the message cache
//
// Parameters:
//     messageCache:  Pointer to Message Cache structure
//   OdmrpCacheDeleteEntry :  Pointer to Message Delete structure
//---------------------------------------------------------

static
void OdmrpDeleteMsgCache(OdmrpData* odmrp,
                         OdmrpMc*  msgCache,
                         OdmrpCacheDeleteEntry* msgDelEntry)
{
    NodeAddress           srcAddr;
    int                   seqNum;
    OdmrpMsgCacheEntry*   current = 0;
    OdmrpMsgCacheEntry*   tempList = 0;
    OdmrpCacheTable*      cacheTable = 0;
    int                   arrayIndex;

    srcAddr = msgDelEntry->srcAddr;
    seqNum  = msgDelEntry->seqNumber;
    cacheTable = (OdmrpCacheTable *)msgCache->cacheTable;
    arrayIndex = ((ODMRP_IP_HASH_KEY * srcAddr) +
        (ODMRP_SEQ_HASH_KEY * seqNum))
        % ODMRP_MAX_HASH_KEY_VALUE;

    if (cacheTable[arrayIndex].front == NULL)
    {
        // Here the control never comes
    }
    current = cacheTable[arrayIndex].front;
    tempList = current;
    while (current != NULL)
    {
        if ((current->srcAddress == srcAddr)
            && (current->seqNumber == seqNum))
        {
            break;
        }
        tempList = current;
        current = current->next;
    }
    if (tempList == current)
    {
        // First Entry
        if (current->next == NULL)
        {
            // No Other node
            cacheTable[arrayIndex].front = NULL;
            cacheTable[arrayIndex].rear = NULL;
            OdmrpSendFreePool(&odmrp->freePool,current);
        }
        else
        {
            cacheTable[arrayIndex].front = current->next;
            OdmrpSendFreePool(&odmrp->freePool, current);
        }
    }
    else if (current->next == NULL)
    {
        tempList->next = NULL;
        cacheTable[arrayIndex].rear = tempList;
        OdmrpSendFreePool(&odmrp->freePool, current);
    }
    else
    {
        tempList->next = current->next;
        OdmrpSendFreePool(&odmrp->freePool,current);
    }
} // End of OdmrpDeleteMsgCache Function

//------------------------------------------------------
// FUNCTION     OdmrpLookupMessageCache
// PURPOSE      Check if the join query/data packet is seen before.
//
//  Parameters:
//    srAddr          : Originating node of the packet.
//    seqNum          : Sequece number of the packet.
//    messageCache    : Pointer to Message Cache Structure.
//
//    Return: TRUE if seen before; FALSE otherwise.
//
//---------------------------------------------------------------

static
BOOL OdmrpLookupMessageCache(NodeAddress srcAddr,
                             int seqNum,
                             OdmrpMc* messageCache)
{
    OdmrpCacheTable*      cacheTable = 0;
    OdmrpMsgCacheEntry*   current = 0;
    int                   arrayIndex;

    cacheTable = (OdmrpCacheTable *)messageCache->cacheTable;
    arrayIndex =  ((ODMRP_IP_HASH_KEY * srcAddr)
        + (ODMRP_SEQ_HASH_KEY * seqNum )) %
        ODMRP_MAX_HASH_KEY_VALUE ;
    if (cacheTable[arrayIndex].front == NULL)
    {
        return FALSE;
    }

    for (current = cacheTable[arrayIndex].front;
    current != NULL;
    current = current->next)
    {
        if (current->srcAddress == srcAddr)
        {
            if (current->seqNumber == seqNum)
            {
                return TRUE;
            }
        }
    }
    return (FALSE);
} // End of OdmrpLookupMessageCache Function

//--------------------------------------------------------------
// FUNCTION     OdmrpLookupMembership
// PURPOSE      Check if the node is a member of the multicast group.
//
// Parameters:
//     mcastAddr:  Multicast group to check.
//     buffer   :  Pointer to Data Buffer
//
// Return: TRUE if member; FALSE otherwise.
//---------------------------------------------------------------

static
BOOL OdmrpLookupMembership(NodeAddress mcastAddr,
                           DataBuffer* buffer)
{
    int i;
    OdmrpMembership*   memTable = (OdmrpMembership *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpMembership);
    for (i = 0; i < numEntry; i++)
    {
        if (memTable[i].mcastAddr == mcastAddr)
        {
            return TRUE;
        }
    }
    return FALSE;
} // End of OdmrpLookupMembership Function

//-------------------------------------------------------
// FUNCTION     OdmrpLookupFgFlag
// PURPOSE      Check if the node is a forwarding group.
//
// Parameters:
//     mcastAddr: Multicast group to check.
//     fgFlag: Pointer to Data Buffer
//
// Return: TRUE if member; FALSE otherwise.
//-------------------------------------------------------------

static
BOOL OdmrpLookupFgFlag(NodeAddress mcastAddr,
                       DataBuffer* buffer)
{
    int i;
    OdmrpFgFlag*    forwardTable = (OdmrpFgFlag *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpFgFlag);

    if (numEntry <= 0)
    {
        return FALSE;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (forwardTable[i].mcastAddr == mcastAddr)
        {
            return TRUE;
        }
    }
    return FALSE;
} // End of OdmrpLookupFgFlag Function

//-----------------------------------------------------------------
// FUNCTION     OdmrpLookupMemberTable
// PURPOSE      Check if there exists valid sources for the multicast
//              group.
//
// Parameters:
//     mcastAddr:  Multicast group to check.
//     memberTable: Pointer to DataBuffer structure
//
// Return: TRUE if exists; FALSE otherwise.
//---------------------------------------------------------------

static
BOOL OdmrpLookupMemberTable(NodeAddress mcastAddr,
                            DataBuffer* buffer)
{
    int i;
    OdmrpMtNode*   memTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpMtNode);
    if (numEntry == 0)
    {
        return FALSE;
    }
    for (i = 0; i < numEntry; i++)
    {
        if ((memTable[i].mcastAddr == mcastAddr)
            && (memTable[i].size > 0))
        {
            return TRUE;
        }
    }
    return FALSE;
} // End of OdmrpLookupMemberTable Function

//-------------------------------------------------------------
// FUNCTION     OdmrpLookupSentTable
// PURPOSE      Check if sent initial data for multicast group.
//
// Parameters:
//     mcastAddr: Multicast group.
//     sentTable: Pointer to Data Buffer
//
// Return: TRUE if sent; FALSE otherwise.
//-------------------------------------------------------------

static
BOOL OdmrpLookupSentTable(NodeAddress mcastAddr,
                          DataBuffer* buffer)
{
    int  i;
    OdmrpSs*     sentTable = (OdmrpSs *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpSs);
    if (numEntry <= 0)
    {
        return FALSE;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (sentTable[i].mcastAddr == mcastAddr)
        {
            return TRUE;
        }
    }
    return FALSE;
} // End of OdmrpLookupSentTable Function

//---------------------------------------------------------------
// FUNCTION     OdmrpCheckFgExpired
// PURPOSE      Check if FG Flag has been updated within timeout.
//
// Parameters:
//    node: Node that is handling the ODMRP packet
//mcastAddr:  Multicast group.
//
// Return: TRUE if expired; FALSE otherwise.
//--------------------------------------------------------

static
BOOL OdmrpCheckFgExpired(Node* node,
                         NodeAddress mcastAddr,
                         OdmrpData* odmrp)
{
    int i;
    OdmrpFgFlag* forwardTable = (OdmrpFgFlag *)
        BUFFER_GetData(&(odmrp->fgFlag));
    int  numEntry = BUFFER_GetCurrentSize(&(odmrp->fgFlag))
        / sizeof(OdmrpFgFlag);

    if (numEntry <= 0)
    {
        return FALSE;
    }
    for (i = 0; i < numEntry; i++)
    {
        if ((forwardTable[i].mcastAddr == mcastAddr)
            && (node->getNodeTime() - forwardTable[i].timestamp
            >= odmrp->expireTime))
        {
            return TRUE;
        }
    }
    return FALSE;

} //End of OdmrpCheckFgExpired Function

//-------------------------------------------------------
// FUNCTION     OdmrpResetMemberFlag
// PURPOSE      Reset membership flag for a particular multicast group.
//
// Parameters:
//     mcastAddr:  Multicast group.
//     memberFlag: Pointer to DataBuffer
//--------------------------------------------------------

static
void OdmrpResetMemberFlag(NodeAddress mcastAddr,
                          DataBuffer* memberFlag)
{
    int i;
    OdmrpMembership* odmrpMemTable = (OdmrpMembership*)
        BUFFER_GetData(memberFlag);
    int numOfEntry = BUFFER_GetCurrentSize(memberFlag)
        / sizeof(OdmrpMembership);

    for (i = 0; i < numOfEntry; i++)
    {
        if (odmrpMemTable[i].mcastAddr == mcastAddr)
        {
            BUFFER_RemoveDataFromDataBuffer(memberFlag,
                (char*) &odmrpMemTable[i],
                sizeof(OdmrpMembership));
            break;
        }
        else
        {
            continue;
        }
    }

} // End of OdmrpResetMemberFlag Function

//----------------------------------------------------------
// FUNCTION     OdmrpUpdateFgFlag
// PURPOSE      Update forwarding flag for a particular multicast
//              group.
//
// Parameters:
//    node: Node that is handling the ODMRP protocol.
//     mcastAddr:  Multicast group.
//     memberFlag: Pointer to Membership Table
//--------------------------------------------------------------

static
void OdmrpUpdateFgFlag(Node* node,
                       NodeAddress mcastAddr,
                       DataBuffer* buffer)
{
    int i;
    OdmrpFgFlag*    forwardTable = (OdmrpFgFlag *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpFgFlag);
    for (i = 0; i < numEntry; i++)
    {
        if (forwardTable[i].mcastAddr == mcastAddr)
        {
            forwardTable[i].timestamp = node->getNodeTime();
            break;
        }
    }
} // End of OdmrpUpdateFgFlag Function

//---------------------------------------------------------
// FUNCTION     OdmrpSetFgFlag
// PURPOSE      Set fg flag for a particular multicast group.
//
// Parameters:
//    node      : Node that is handling the ODMRP protocol
//     mcastAddr: Multicast group.
//        fgFlag: Pointer to Data Buffer
//-------------------------------------------------------------

static
void OdmrpSetFgFlag(Node* node,
                    NodeAddress mcastAddr,
                    DataBuffer* fgFlag)
{
    OdmrpFgFlag  forwardTable;

    forwardTable.mcastAddr = mcastAddr;
    forwardTable.timestamp = node->getNodeTime();

    BUFFER_AddDataToDataBuffer(fgFlag,
        (char *) &forwardTable,
        sizeof(OdmrpFgFlag));
} // End of OdmrpSetFgFlag Function

//----------------------------------------------------------
// FUNCTION     OdmrpResetFgFlag
// PURPOSE      Reset fg flag for a particular multicast group.
//
// Parameters:
//     mcastAddr: Multicast group.
//     fgFlag:    Pointer to Forwarding Table
//---------------------------------------------------------

static
void OdmrpResetFgFlag(NodeAddress mcastAddr,
                      DataBuffer* fgFlag)
{
    int numOfEntry;
    int i;

    OdmrpFgFlag*   odmrpForwardTable = (OdmrpFgFlag*)
        BUFFER_GetData(fgFlag);
    numOfEntry = BUFFER_GetCurrentSize(fgFlag) / sizeof(OdmrpFgFlag);

    for (i = 0; i < numOfEntry; i++)
    {
        if (odmrpForwardTable[i].mcastAddr == mcastAddr)
        {
            BUFFER_RemoveDataFromDataBuffer(fgFlag,
                (char*) &odmrpForwardTable[i],
                sizeof(OdmrpFgFlag));
            break;
        }
        else
        {
            continue;
        }
    }
} // End of OdmrpResetFgFlag Function

//-------------------------------------------------------------
// FUNCTION     OdmrpSetSent
// PURPOSE      Set sent flag for a particular multicast group.
//
// Parameters:
//     mcastAddr:  Multicast group.
//-----------------------------------------------------------

static
void OdmrpSetSent(Node* node,
                  NodeAddress mcastAddr,
                  OdmrpData* odmrp)
{

    OdmrpSs  ssTable;

    ssTable.mcastAddr = mcastAddr;
    ssTable.minExpireTime = node->getNodeTime() + odmrp->refreshTime;
    ssTable.lastSent = node->getNodeTime();
    ssTable.nextQuerySend = FALSE;

    BUFFER_AddDataToDataBuffer(&(odmrp->sentTable),
        (char*) &ssTable,
        sizeof(OdmrpSs));

} // End of OdmrpSetSent Function

//-----------------------------------------------------
// FUNCTION     OdmrpDeleteSourceSent
// PURPOSE     Delete Data Source for the
//              multicast group.
//
// Parameters:
//     mcastAddr: Multicast group.
//   sentTable  : Pointer to DataBuffer
//----------------------------------------------------

static
void OdmrpDeleteSourceSent(NodeAddress mcastAddr,
                           DataBuffer* sentTable)
{
    int i;
    OdmrpSs*  ssTable = (OdmrpSs *)
        BUFFER_GetData(sentTable);

    int numOfEntry = BUFFER_GetCurrentSize(sentTable)
        / sizeof(OdmrpSs);
    for (i = 0; i < numOfEntry; i++)
    {
        if (ssTable[i].mcastAddr == mcastAddr)
        {

            BUFFER_RemoveDataFromDataBuffer(sentTable,
                (char*) &ssTable[i],
                sizeof(OdmrpSs));
            break;
        }
        else
        {
            continue;
        }
    }

} // End of OdmrpDeleteSourceSent Function

//-----------------------------------------------------
// FUNCTION     OdmrpGetMTEntry
// PURPOSE     Searches for a multicast Address
//             in Member Table
//
// Parameters:
//     mcastAddr:    Multicast group.
//     memberTable : Pointer to Databuffer Structure
//-----------------------------------------------------

static
OdmrpMtNode* OdmrpGetMTEntry(NodeAddress mcastAddr,
                             DataBuffer *buffer)
{
    int i;
    OdmrpMtNode*  memberTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpMtNode);

    if (numEntry == 0)
    {
        return NULL;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (memberTable[i].mcastAddr == mcastAddr)
        {
            return (OdmrpMtNode *)&memberTable[i];
        }
    }
    return NULL;
} // End of OdmrpGetMTEntry Function

//---------------------------------------------------
// FUNCTION     OdmrpCheckMSExistAndUpdate
// PURPOSE     Check if a source for the multicast
//             group exists or not in Member Table
// Parameters:
//     srcAddr  :      Originating node of the packet
//     memberTable:    Pointer to DataBuffer Structure
//
//
//      Returns     TRUE if it exists FALSE otherwise
//--------------------------------------------------

static
BOOL OdmrpCheckMSExistAndUpdate(Node* node,
                                NodeAddress srcAddr,
                                DataBuffer* buffer)
{
    int i;
    OdmrpMtSnode*  memSrcTable = (OdmrpMtSnode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpMtSnode);

    if (numEntry == 0)
    {
        return FALSE;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (memSrcTable[i].srcAddr == srcAddr)
        {
            memSrcTable[i].timestamp = node->getNodeTime();
            return TRUE;
        }
    }
    return FALSE;
} // End of OdmrpCheckMSExistAndUpdate Function

//--------------------------------------------------------
// FUNCTION     OdmrpInsertMemberSource
// PURPOSE      Insert Source for a particular Multicast
//              Group in Member Table structure
// Parameters:
//    node: Node that is handling the ODMRP protocol
//     srcAddr  :  Originating node of the packet
//     mcast    :  Pointer to the indirection of Member
//                 Table structure
//-------------------------------------------------------

static
void OdmrpInsertMemberSource(Node* node,
                             NodeAddress srcAddr,
                             OdmrpMtNode* memberTable)
{

    OdmrpMtSnode  memSrcTable;

    if (!OdmrpCheckMSExistAndUpdate(node,
        srcAddr,
        &(memberTable->head)))
    {
        ++(memberTable->size);
        memberTable->queryLastReceived = node->getNodeTime();
        memSrcTable.srcAddr = srcAddr;
        memSrcTable.timestamp = node->getNodeTime();
        BUFFER_AddDataToDataBuffer(&(memberTable->head),
            (char *) &memSrcTable,
            sizeof(OdmrpMtSnode));
    }
    else
    {
        memberTable->queryLastReceived = node->getNodeTime();
        return;
    }
} // End of OdmrpInsertMemberSource Function

//--------------------------------------------------------
// FUNCTION     OdmrpAddSrcToMemTable
// PURPOSE      Insert Source for a particular Multicast
//              Group in Member Table structure
// Parameters:
//   buffer : Pointer to DataBuffer
//-------------------------------------------------------

static
void OdmrpAddSrcToMemTable(Node* node,
                           DataBuffer *buffer,
                           NodeAddress srcAddr)
{
    OdmrpMtSnode memSrcTable;

    memSrcTable.srcAddr = srcAddr;
    memSrcTable.timestamp = node->getNodeTime();

    BUFFER_AddDataToDataBuffer(buffer,
        (char *) &memSrcTable,
        sizeof(OdmrpMtSnode));
} // End of OdmrpAddSrcToMemTable Function

//-----------------------------------------------------
// FUNCTION     OdmrpInsertMemberTable
// PURPOSE      Populate Member Table structure
//
// Parameters:
//    node: Node that is handling the ODMRP protocol
//     mcastAddr:      Multicast group.
//     sourceAddr:     Originating node of the packet.
//     memberTable:    Pointer to DataBuffer Structure
//--------------------------------------------------------

static
void OdmrpInsertMemberTable(Node* node,
                            NodeAddress mcastAddr,
                            NodeAddress srcAddr,
                            DataBuffer* buffer)
{

    OdmrpMtNode   memberTable;
    OdmrpMtNode*  memTable;
    int numOfEntry;

    memberTable.mcastAddr = mcastAddr;
    memberTable. size = 1;
    memberTable.sent = FALSE;
    memberTable.lastSent = 0;
    memberTable.queryLastReceived = node->getNodeTime();

    BUFFER_AddDataToDataBuffer(buffer,
        (char *) &memberTable,
        sizeof(OdmrpMtNode));
    memTable = (OdmrpMtNode *)BUFFER_GetData(buffer);
    numOfEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpMtNode);
    numOfEntry = numOfEntry - 1;

    BUFFER_InitializeDataBuffer(&(memTable[numOfEntry].head),
        sizeof(OdmrpMtSnode)
        * ODMRP_INITIAL_CHUNK);
    OdmrpAddSrcToMemTable(node,
        &(memTable[numOfEntry].head),
        srcAddr);

} // End of OdmrpInsertMemberTable Function

//------------------------------------------------------
// FUNCTION     OdmrpDeleteMemberTable
// PURPOSE      Delete Member Table structure for a given multicast
//              address
//
// Parameters:
//     mcastAddr: Multicast Group
//   memberTable: Pointer to DataBuffer Structure
//------------------------------------------------------

static
void OdmrpDeleteMemberTable(NodeAddress mcastAddr,
                            DataBuffer* buffer)
{
    int i;
    OdmrpMtNode*  memberTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpMtNode);

    if (numEntry == 0)
    {
        return;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (memberTable[i].mcastAddr == mcastAddr)
        {
            BUFFER_DestroyDataBuffer(&(memberTable[i].head));
            BUFFER_RemoveDataFromDataBuffer(buffer,
                (char*) &memberTable[i],
                sizeof(OdmrpMtNode));
        }
    }
} //End of OdmrpDeleteMemberTable Function

//---------------------------------------------------------
// FUNCTION     OdmrpCheckSourceExpired
// PURPOSE      Check if source has sent msg within timeout.
//              and populate Member Table structure
//
// Parameters:
//     mcastAddr:      Multicast group.
//     memberTable:    Pointer to DataBuffer Structure.
//---------------------------------------------------------

static
void OdmrpCheckSourceExpired(Node* node,
                             NodeAddress mcastAddr,
                             DataBuffer* buffer)
{
    int i;
    int j;
    int numSrc;
    OdmrpMtNode*  memberTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    OdmrpMtSnode*  memSrcTable;
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpMtNode);

    if (numEntry == 0)
    {
        return;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (memberTable[i].mcastAddr == mcastAddr)
        {
            memSrcTable = (OdmrpMtSnode *)
                BUFFER_GetData(&(memberTable[i].head));

            numSrc = BUFFER_GetCurrentSize(&(memberTable[i].head))
                / sizeof(OdmrpMtSnode);

            for (j = 0; j < numSrc; j++)
            {
                if (node->getNodeTime() - memSrcTable[j].timestamp >=
                    ODMRP_MEM_TIMEOUT)
                {
                    memberTable[i].size =  memberTable[i].size - 1;

                    BUFFER_ClearDataFromDataBuffer(
                        &(memberTable[i].head),
                        (char*) &memSrcTable[j],
                        sizeof(OdmrpMtSnode),
                        FALSE);
                    j--;
                    numSrc--;

                }
                if (memberTable[i].size == 0)
                {
                    BUFFER_RemoveDataFromDataBuffer(buffer,
                        (char*) &memberTable[i],
                        sizeof(OdmrpMtNode));
                    return;
                }
            }
            return;
        }
    }
} // End of OdmrpCheckSourceExpired Function

//------------------------------------------------------
// FUNCTION     OdmrpGetTTEntry
// PURPOSE      Searches the Temp Table for a
//              particular multicast group
// Parameters:
//     mcastAddr:      Multicast group.
//     memberTable:    Pointer to Databuffer Structure
//-----------------------------------------------------

static
OdmrpTtNode* OdmrpGetTTEntry(NodeAddress mcastAddr,
                             DataBuffer *buffer)
{
    int i;

    OdmrpTtNode*  tempTable = (OdmrpTtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpTtNode);

    if (numEntry == 0)
    {
        return NULL;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (tempTable[i].mcastAddr == mcastAddr)
        {
            return (OdmrpTtNode *)&tempTable[i];

        }
    }
    return NULL;
}

//--------------------------------------------------
// FUNCTION     OdmrpCheckTSExistAndUpdate
// PURPOSE      Check a particular source exists
//              for a given multicast group in Temp
//              Table structure
//
// Parameters:
//     srcAddr   : Sources for the multicast group
//     mcast     : Pointer to DataBuffer Structure
//
//--------------------------------------------------

static
BOOL OdmrpCheckTSExistAndUpdate(Node* node,
                                NodeAddress srcAddr,
                                DataBuffer* buffer,
                                clocktype  timeVal)
{
    int i;
    OdmrpTtSnode*  tempSrcTable = (OdmrpTtSnode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpTtSnode);

    if (numEntry == 0)
    {
        return FALSE;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (tempSrcTable[i].srcAddr == srcAddr)
        {
            tempSrcTable[i].timestamp = node->getNodeTime();
            tempSrcTable[i].FGExpireTime = node->getNodeTime()
                + timeVal;
            return TRUE;

        }
    }
    return FALSE;
} // End of OdmrpCheckTSExistAndUpdate Function

//------------------------------------------------------
// FUNCTION     OdmrpInsertTempSource
// PURPOSE      Populate Temp Table for a given multicast group
//              and Multicast Source
// Parameters:
//    node     : Node that is handling the ODMRP protocol
//     srcAddr : Sources for the multicast group
//     mcast   : Pointer to the indirection of Temp Table structure
//------------------------------------------------------

static
void  OdmrpInsertTempSource(OdmrpData* odmrp,
                            Node* node,
                            NodeAddress srcAddr,
                            OdmrpTtNode* tempTable)
{
    OdmrpTtSnode  tempSrcTable;

    if (!OdmrpCheckTSExistAndUpdate(node,
        srcAddr,
        &(tempTable->head),
        odmrp->expireTime))
    {
        ++(tempTable->size);
        tempTable->sent = FALSE;
        tempSrcTable.srcAddr = srcAddr;
        tempSrcTable.timestamp = node->getNodeTime();
        tempSrcTable.FGExpireTime = node->getNodeTime() + odmrp->expireTime;
        BUFFER_AddDataToDataBuffer(&(tempTable->head),
            (char *) &tempSrcTable,
            sizeof(OdmrpTtSnode));
    }
    else
    {
        return;
    }
} // End of OdmrpInsertTempSource Function

//----------------------------------------------------------
// FUNCTION     OdmrpAddToTempTable
// PURPOSE      Add a new Source for a given Multicast
//              Group in the TempTable
//
// Parameters:
//           node:  Node that is handling the ODMRP protocol
//         buffer : Pointer to Data Buffer
//        srcAddr : Source Address
//        timeVal : Timer Value
//-----------------------------------------------------

static
void OdmrpAddToTempTable(Node* node,
                         DataBuffer *buffer,
                         NodeAddress srcAddr,
                         clocktype timeVal)
{
    OdmrpTtSnode tempSrcTable;

    tempSrcTable.srcAddr = srcAddr;
    tempSrcTable.timestamp = node->getNodeTime();
    tempSrcTable.FGExpireTime = node->getNodeTime() + timeVal;

    BUFFER_AddDataToDataBuffer(buffer,
        (char *) &tempSrcTable,
        sizeof(OdmrpTtSnode));

}

//----------------------------------------------------------
// FUNCTION     OdmrpInsertTempTable
// PURPOSE      Insert new entry into temp table.
//
// Parameters:
//    node: Node that is handling the ODMRP protocol
//     mcastAddr : Multicast group.
//     sourceAddr: Sources for the multicast.
//     tempTable : Pointer to Temp table structure
//
//-----------------------------------------------------

static
void OdmrpInsertTempTable(OdmrpData* odmrp,
                          Node* node,
                          NodeAddress mcastAddr,
                          NodeAddress srcAddr,
                          DataBuffer* buffer)
{
    OdmrpTtNode   tempTable;
    OdmrpTtNode*  tTTable;
    int numOfEntry;

    tempTable.mcastAddr = mcastAddr;
    tempTable. size = 1;
    tempTable.sent = FALSE;

    BUFFER_AddDataToDataBuffer(buffer,
        (char *) &tempTable,
        sizeof(OdmrpTtNode));

    tTTable = (OdmrpTtNode *)BUFFER_GetData(buffer);

    numOfEntry = BUFFER_GetCurrentSize(buffer)
        /sizeof(OdmrpTtNode);
    numOfEntry = numOfEntry - 1;

    BUFFER_InitializeDataBuffer(&(tTTable[numOfEntry].head),
        sizeof(OdmrpTtNode)
        * ODMRP_INITIAL_CHUNK);
    OdmrpAddToTempTable(node,
        &(tTTable[numOfEntry].head),
        srcAddr,
        odmrp->expireTime);

} //End of OdmrpInsertTempTable Function

//--------------------------------------------------------
// FUNCTION     OdmrpCheckTempExpired
// PURPOSE      Remove the source for a given Temp
//              Table structure
//
// Parameters:
//      node      :
//      mcastAddr : Multicast Group
//      tempTable : Pointer to Temp Table structure
//--------------------------------------------------------

static
void OdmrpCheckTempExpired(Node* node,
                           NodeAddress mcastAddr,
                           DataBuffer* buffer)
{
    int i;
    int j;
    int numSrc;
    OdmrpTtNode*  tempTable = (OdmrpTtNode *)
        BUFFER_GetData(buffer);
    OdmrpTtSnode* tempSrcTable;
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpTtNode);
    if (numEntry == 0)
    {
        return;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (tempTable[i].mcastAddr == mcastAddr)
        {
            tempSrcTable = (OdmrpTtSnode *)
                BUFFER_GetData(&(tempTable[i].head));

            numSrc = BUFFER_GetCurrentSize(&(tempTable[i].head))
                / sizeof(OdmrpTtSnode);

            for (j = 0;j < numSrc; j++)
            {
                if (node->getNodeTime() - tempSrcTable[j].timestamp >=
                    ODMRP_JR_LIFETIME)
                {
                    tempTable[i].size =  tempTable[i].size - 1;
                    BUFFER_ClearDataFromDataBuffer(
                        &(tempTable[i].head),
                        (char*) &tempSrcTable[j],
                        sizeof(OdmrpTtSnode),
                        FALSE);
                    j--;
                    numSrc--;
                }
                if (tempTable[i].size == 0)
                {
                    BUFFER_RemoveDataFromDataBuffer(buffer,
                        (char *) &tempTable[i],
                        sizeof(OdmrpTtNode));
                    return;
                }
            }
            return;
        }
    }
} // End of OdmrpCheckTempExpired Function

//------------------------------------------------------
// FUNCTION     OdmrpGetRPTEntry
// PURPOSE      Get the pointer to the Response Table
//              Structure for a given Multicast Address
//
// Parameters:
//      mcastAddr: Multicast Group
//    rspnsTable : Pointer to DataBuffer
//------------------------------------------------------

static
OdmrpRptNode* OdmrpGetRPTEntry(NodeAddress mcastAddr,
                               DataBuffer* buffer)
{
    int i;
    OdmrpRptNode*  rspnsTable = (OdmrpRptNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpRptNode);

    if (numEntry == 0)
    {
        return NULL;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (rspnsTable[i].mcastAddr == mcastAddr)
        {
            return (OdmrpRptNode *)&rspnsTable[i];
        }
    }
    return NULL;
} // End of OdmrpGetRPTEntry Function

//---------------------------------------------------
// FUNCTION     OdmrpCheckRPSExist
// PURPOSE      Checks whether a paricular source exist
//              for a given multicast group in Response
//              Table structure.
// Parameters:
//     srcAddr  :  Source Address
//     buffer    :  Pointer to DataBuffer Structure
//
//---------------------------------------------------

static
BOOL OdmrpCheckRPSExist(NodeAddress srcAddr,
                        DataBuffer* buffer)
{
    int i;
    OdmrpRptSnode*  rspnsSrcTable = (OdmrpRptSnode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpRptSnode);

    if (numEntry <= 0)
    {
        return FALSE;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (rspnsSrcTable[i].srcAddr == srcAddr)
        {
            return TRUE;
        }
    }
    return FALSE;
} // End of OdmrpCheckRPSExist Function

//-----------------------------------------------------------
// FUNCTION     OdmrpInsertResponseSource
// PURPOSE      Populate Response Table structure
//              with the source address for a given
//              multicast group
// Parameters:
//     srcAddr  :  Source Address
//     mcast    :  Pointer to indirection of Response
//                 Table structure
//-----------------------------------------------------------

static
void OdmrpInsertResponseSource(NodeAddress srcAddr,
                               OdmrpRptNode* rspnsTable)
{
    OdmrpRptSnode  responseSrcTable;
    int numEntry = BUFFER_GetCurrentSize(&(rspnsTable->head))
        / sizeof(OdmrpRptSnode);

    if (numEntry == 0)
    {
        BUFFER_InitializeDataBuffer(&(rspnsTable->head),
            sizeof(OdmrpRptSnode)
            * ODMRP_INITIAL_CHUNK);
        rspnsTable->size = 1;
        responseSrcTable.srcAddr = srcAddr;

        BUFFER_AddDataToDataBuffer(&(rspnsTable->head),
            (char *) &responseSrcTable,
            sizeof(OdmrpRptSnode));
        return;
    }

    if (!OdmrpCheckRPSExist(srcAddr,&(rspnsTable->head)))
    {
        ++(rspnsTable->size);
        responseSrcTable.srcAddr = srcAddr;

        BUFFER_AddDataToDataBuffer(&(rspnsTable->head),
            (char *) &responseSrcTable,
            sizeof(OdmrpRptSnode));
    }
    else
    {
        return;
    }
} // End of OdmrpInsertResponseSource Function

//-----------------------------------------------------------
// FUNCTION     OdmrpAddSrcToResponseTable
// PURPOSE      Add a new Source  to Response Table structure
//              for a given multicast group
// Parameters
//   buffer    : Pointer to DataBuffer
//-----------------------------------------------------------

static
void OdmrpAddSrcToResponseTable(DataBuffer *buffer,
                                NodeAddress srcAddr)
{
    OdmrpRptSnode rspnsSrcTable;

    rspnsSrcTable.srcAddr = srcAddr;
    BUFFER_AddDataToDataBuffer(buffer,
        (char *) &rspnsSrcTable,
        sizeof(OdmrpRptSnode));
} // End of OdmrpAddSrcToResponseTable Function

//-----------------------------------------------------------
// FUNCTION     OdmrpInsertResponseTable
// PURPOSE      Populate Response Table structure
//
// Parameters:
//     mcastAddr:        Multicast address
//     srcAddr  :        Source Address
//     responseTable:    Pointer to DataBuffer Structure
//-----------------------------------------------------------

static
void OdmrpInsertResponseTable(NodeAddress mcastAddr,
                              NodeAddress srcAddr,
                              DataBuffer* buffer)
{
    OdmrpRptNode  rspnsTable;
    OdmrpRptNode* responseTable;
    int numOfEntry;

    rspnsTable.mcastAddr = mcastAddr;
    rspnsTable. size = 1;

    BUFFER_AddDataToDataBuffer(buffer,
        (char *) &rspnsTable,
        sizeof(OdmrpRptNode));

    responseTable = (OdmrpRptNode *)BUFFER_GetData(buffer);

    numOfEntry = BUFFER_GetCurrentSize(buffer)
        /sizeof(OdmrpRptNode);
    numOfEntry = numOfEntry - 1;

    BUFFER_InitializeDataBuffer(&(responseTable[numOfEntry].head),
        sizeof(OdmrpRptSnode)
        * ODMRP_INITIAL_CHUNK);
    OdmrpAddSrcToResponseTable(&(responseTable[numOfEntry].head),
        srcAddr);

} // End of OdmrpInsertResponseTable Function


//--------------------------------------------------------
// FUNCTION     OdmrpGetATEntry
// PURPOSE      Searches for the multicast address in Ack Table
//              and returns a pointer to Ack Table
// Parameters:
//     mcastAddr:  Multicast Group
//     ackTable :  Pointer to Ack Table
//--------------------------------------------------------

static
OdmrpAtNode* OdmrpGetATEntry(NodeAddress mcastAddr,
                             DataBuffer* buffer)
{
    int i;
    OdmrpAtNode*  ackTable = (OdmrpAtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpAtNode);

    if (numEntry == 0)
    {
        return NULL;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (ackTable[i].mcastAddr == mcastAddr)
        {
            return (OdmrpAtNode *)&ackTable[i];
        }
    }
    return NULL;

} // End of OdmrpGetATEntry Function

//-----------------------------------------------------------
// FUNCTION     OdmrpCheckAsExistAndUpdate
// PURPOSE      Searches for a particular source for given multicast
//              address in Ack Table Structure and update Data
//              Member.
//
// Parameters:
//  srcAddr  :  Source Address
//  nextAddr :  Next Address
//  buffer   :  Pointer to DataBuffer Structure
//
//----------------------------------------------------------

static
BOOL OdmrpCheckASExistAndUpdate(NodeAddress srcAddr,
                                NodeAddress nextAddr,
                                DataBuffer* buffer)
{
    int i;
    OdmrpAtSnode*  ackSrcTable = (OdmrpAtSnode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpAtSnode);

    if (numEntry == 0)
    {
        return FALSE;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (ackSrcTable[i].srcAddr == srcAddr)
        {
            ackSrcTable[i].nextAddr = nextAddr;
            ackSrcTable[i].numTx++;
            return TRUE;
        }
    }
    return FALSE;
} // End of OdmrpCheckASExistAndUpdate Function

//--------------------------------------------------------
// FUNCTION     OdmrpInsertAckSource
// PURPOSE      Populate Ack Table with the given
//              Join Reply Message.
//
// Parameters:
//    node       : Node that is handling the ODMRP protocol
//  srcAddr      :  Source Address
//  nextAddr     :  Next Hop Address
//  mcast        :  Pointer to the indirecton of Ack Table
//                  structure.
//--------------------------------------------------------

static
void  OdmrpInsertAckSource(Node* node,
                           NodeAddress srcAddr,
                           NodeAddress nextAddr,
                           OdmrpAtNode* ackTable)
{
    OdmrpAtSnode  ackSrcTable;

    if (!OdmrpCheckASExistAndUpdate(srcAddr,
        nextAddr,
        &(ackTable->head)))
    {
        ++(ackTable->size);
        ackTable->lastSent = node->getNodeTime();
        ackSrcTable.srcAddr = srcAddr;
        ackSrcTable.nextAddr = nextAddr;
        ackSrcTable.numTx = 1;

        BUFFER_AddDataToDataBuffer(&(ackTable->head),
            (char *) &ackSrcTable,
            sizeof(OdmrpAtSnode));
        return;

    }
    else
    {
        return;
    }

} // End of OdmrpInsertAckSource Function

//--------------------------------------------------------
// FUNCTION     OdmrpInitialiseAckTable
// PURPOSE      Insert a new Source for a given multicast
//              group in Ack Table Structure
//
// Parameters:
//    buffer   : Pointer to Data Buffer
//    srcAddr  : Source Address
//    nextAddr : Next Address
//--------------------------------------------------------

static
void OdmrpAddSrcToAckTable(DataBuffer *buffer,
                           NodeAddress srcAddr,
                           NodeAddress nextAddr)
{
    OdmrpAtSnode ackSrcTable;

    ackSrcTable.srcAddr = srcAddr;
    ackSrcTable.nextAddr = nextAddr;
    ackSrcTable.numTx = 1;

    BUFFER_AddDataToDataBuffer(buffer,
        (char *) &ackSrcTable,
        sizeof(OdmrpAtSnode));

} // End of OdmrpAddSrcToAckTable Function

//-----------------------------------------------
// FUNCTION     OdmrpInsertAckTable
// PURPOSE      Populate Ack Table for a given multicast address
//              and a received Join Reply
//
// Parameters:
//    node       : Node that is handling the ODMRP protocol.
//  mcastAddr    :  Multicast Group
//  ackTable     :  Pointer to Ack Table
//  reply        :  Pointer to Reply message
//
//-----------------------------------------------

static
void OdmrpInsertAckTable(Node* node,
                         NodeAddress mcastAddr,
                         DataBuffer* buffer,
                         OdmrpJoinReply  *replyPkt,
                         OdmrpJoinTupleForm* tupleForm)
{
    OdmrpAtNode   ackTable;
    OdmrpAtNode*  atTable;
    DataBuffer*   savedBuffer;
    int numOfEntry;
    int i;

    ackTable.mcastAddr = mcastAddr;
    ackTable.size = replyPkt->replyCount;
    ackTable.lastSent = node->getNodeTime();

    BUFFER_AddDataToDataBuffer(buffer,
        (char *) &ackTable,
        sizeof(OdmrpAtNode));
    atTable = (OdmrpAtNode *)BUFFER_GetData(buffer);

    numOfEntry = BUFFER_GetCurrentSize(buffer) /sizeof(OdmrpAtNode);
    numOfEntry = numOfEntry - 1;

    BUFFER_InitializeDataBuffer(&(atTable[numOfEntry].head),
        sizeof(OdmrpAtSnode)
        * ODMRP_INITIAL_CHUNK);
    savedBuffer = &(atTable[numOfEntry].head);
    for (i = 0; i < (signed) replyPkt->replyCount; i++)
    {
        OdmrpAddSrcToAckTable(savedBuffer,
            tupleForm[i].srcAddr,
            tupleForm[i].nextAddr);
        savedBuffer = &(atTable[numOfEntry].head);
    }

}// End of OdmrpInsertAckTable Function

//----------------------------------------------------
// FUNCTION     OdmrpCheckRouteExist
// PURPOSE      Check whether the route to a particular
//              Destination exists or not in Route Table
//
// Parameters:
//     destAddr:  Destination Address.
//   routeTable:  Pointer to DataBuffer structure
//
//----------------------------------------------------

static
BOOL OdmrpCheckRouteExist(NodeAddress destAddr,
                          DataBuffer* buffer)
{
    int i;
    int numOfEntry;
    OdmrpRt*  routeTable = (OdmrpRt *)
        BUFFER_GetData(buffer);
    numOfEntry = BUFFER_GetCurrentSize(buffer)/sizeof(OdmrpRt);

    if (numOfEntry <= 0)
    {
        return FALSE;
    }
    for (i = 0; i < numOfEntry; i++)
    {
        if ((routeTable[i]).destAddr == destAddr)
        {
            return TRUE;
        }
    }
    return FALSE;
} // End of OdmrpCheckRouteExist Function

//-----------------------------------------------------------
// FUNCTION     OdmrpInsertRouteTable
// PURPOSE      Insert new entry into route table.
//
// Parameters:
//     destAddr:      Destination node.
//     nextAddr:      Next node towards the destination.
//     routeTable:    Pointer to Data Buffer
//------------------------------------------------------------

static
void OdmrpInsertRouteTable(Node* node,
                           NodeAddress destAddr,
                           NodeAddress nextAddr,
                           DataBuffer* buffer)
{
    int i;
    OdmrpRt    rtTable;
    OdmrpRt*   routeTable = (OdmrpRt *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpRt);

    for (i = 0; i < numEntry; i++)
    {
        if (routeTable[i].destAddr == destAddr)

        {
            routeTable[i].nextAddr = nextAddr;
            return;
        }
    }
    rtTable.destAddr = destAddr;
    rtTable.nextAddr = nextAddr;

    BUFFER_AddDataToDataBuffer(buffer,
        (char*) &rtTable,
        sizeof(OdmrpRt));

} // End of OdmrpInsertRouteTable Function

//----------------------------------------------------
// FUNCTION     OdmrpInitMemoryPool
// PURPOSE      Allocate a large Chunk of memmory
//
// Parameters:
//     memPool: Pointer to Memory Pool
//----------------------------------------------------

static
void OdmrpInitMemoryPool(MemPool*  memPool)
{
    char* freeBufferPool;
    OdmrpFreeBufferPool* savedBufferPool;
    int i;
    memPool->head = (OdmrpFreeBufferPool *)
        MEM_malloc (ODMRP_MAX_BUFFER_POOL *
        sizeof(OdmrpFreeBufferPool));
    memPool->currentPointer = (int *)memPool->head;
    savedBufferPool = memPool->head;

    for (i = 0; i < ODMRP_MAX_BUFFER_POOL; i++)
    {
        if (i == ODMRP_MAX_BUFFER_POOL - 1)
        {
            savedBufferPool->nextPointer = NULL;
        }
        else
        {
            freeBufferPool = (char *)savedBufferPool;
            freeBufferPool = freeBufferPool
                + sizeof(OdmrpFreeBufferPool);
            savedBufferPool->nextPointer = (int *) freeBufferPool;
            savedBufferPool = (OdmrpFreeBufferPool *)freeBufferPool;

        }
    }
    return;
}// End of OdmrpInitMemoryPool Function

//----------------------------------------------------
// FUNCTION     OdmrpGetFreePool
// PURPOSE      Return a free memory from the memory pool
//
// Parameters:
//     memPool: Pointer to Memory Pool
//----------------------------------------------------

static
OdmrpMsgCacheEntry* OdmrpGetFreePool(MemPool* memoryPool)

{
    OdmrpFreeBufferPool* newMemoryPool = 0;
    char*                freeBufferPool = 0;
    int                  i;
    OdmrpFreeBufferPool* freePool = (OdmrpFreeBufferPool *)
        memoryPool->currentPointer;

    if (freePool->nextPointer != NULL)
    {
        memoryPool->currentPointer = (int *)freePool->nextPointer;
        return (OdmrpMsgCacheEntry *)&(freePool->cacheEntry);
    }
    else
    {
        newMemoryPool = (OdmrpFreeBufferPool *)
            MEM_malloc (ODMRP_MAX_BUFFER_POOL *
            sizeof(OdmrpFreeBufferPool));

        freePool->nextPointer = (int *) newMemoryPool;
        memoryPool->currentPointer = (int *) newMemoryPool;
        freeBufferPool = (char *) newMemoryPool;

        for (i = 0;i < ODMRP_MAX_BUFFER_POOL;i++)
        {
            if (i == ODMRP_MAX_BUFFER_POOL - 1)
            {
                newMemoryPool->nextPointer = NULL;
            }
            else
            {
                freeBufferPool = (char *) newMemoryPool;
                freeBufferPool = freeBufferPool
                    + sizeof(OdmrpFreeBufferPool);
                newMemoryPool->nextPointer = (int *) freeBufferPool;
                newMemoryPool = (OdmrpFreeBufferPool *)freeBufferPool;
            }
        }
        return (OdmrpMsgCacheEntry *)&(freePool->cacheEntry);
    }

} // End of OdmrpGetFreePool Function


//---------------------------------------------------------------
// FUNCTION     OdmrpInsertMessageCache
// PURPOSE      Populate Message Cache structure
//              with Source IP Address and Sequence Number
// Parameters:
//     node    :  Node that is handling the data
//     srcAddr :  Source Address
//    seqNumber:  Sequence Number of the Data Packet
//   routeTable:    Pointer to Message Cache Table
//-------------------------------------------------------------

static
void OdmrpInsertMessageCache(OdmrpData* odmrp,
                             Node* node,
                             NodeAddress srcAddr,
                             int seqNumber,
                             OdmrpMc* messageCache)
{
    OdmrpCacheTable*       cacheTable = 0;
    OdmrpMsgCacheEntry*    tempEntry  = 0;
    OdmrpMsgCacheEntry*    tempMsgCacheEntry = 0;
    int                    arrayIndex;
    OdmrpCacheDeleteEntry  msgDelEntry;

    cacheTable = (OdmrpCacheTable *) messageCache->cacheTable;

    arrayIndex =  ((ODMRP_IP_HASH_KEY * srcAddr) +
        (ODMRP_SEQ_HASH_KEY * seqNumber))
        % ODMRP_MAX_HASH_KEY_VALUE;


    tempMsgCacheEntry = OdmrpGetFreePool(&(odmrp->freePool));

    tempMsgCacheEntry->srcAddress = srcAddr;
    tempMsgCacheEntry->seqNumber = seqNumber;
    tempMsgCacheEntry->next = NULL;

    if (cacheTable[arrayIndex].front == NULL)
    {
        // Insert into the hash
        cacheTable[arrayIndex].front = tempMsgCacheEntry;
        cacheTable[arrayIndex].rear = tempMsgCacheEntry;
    }
    else
    {
        // Go Along the link list to insert a node
        tempEntry = cacheTable[arrayIndex].rear;
        tempEntry->next = tempMsgCacheEntry;
        cacheTable[arrayIndex].rear = tempMsgCacheEntry;
    }
    msgDelEntry.srcAddr  = srcAddr;
    msgDelEntry.seqNumber = seqNumber;

    OdmrpSetTimer(node,
        MSG_NETWORK_FlushTables,
        &msgDelEntry,
        sizeof(OdmrpCacheDeleteEntry),
        ODMRP_FLUSH_INTERVAL);

}// End of OdmrpInsertMessageCache Function

//------------------------------------------------------------
// FUNCTION     OdmrpGetSeq
// PURPOSE      Get the sequence number of the packet to send.
//
//
// Return: a sequence number.
//------------------------------------------------------------

static
int OdmrpGetSeq(OdmrpData* odmrp)
{
    int number = odmrp->seqTable;
    odmrp->seqTable++;
    return number;
} //End of OdmrpGetSeq Function

//----------------------------------------------------------------
// FUNCTION     OdmrpGetNextNode
// PURPOSE      Extract next node to destination from route table.
//
// Parameters:
//     destAddr:   Destination node.
//     routeTable: Pointer to Databuffer structure
//
// Return: Next node.
//-----------------------------------------------------------------

static
NodeAddress OdmrpGetNextNode(NodeAddress destAddr,
                             DataBuffer* buffer)
{
    int i;
    OdmrpRt*   routeTable = (OdmrpRt *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpRt);

    for (i = 0; i < numEntry; i++)
    {
        if (routeTable[i].destAddr == destAddr)
        {
            return routeTable[i].nextAddr;
        }
    }
    return ANY_DEST;
} //End of OdmrpGetNextNode Function

//---------------------------------------------------
// FUNCTION     OdmrpGetTempCount
// PURPOSE      Return the number of sources for
//              a given Multicast Group in the Temp
//              Table structure
//
// Parameters:
//  mcastAddr : Multicast Group
//  tempTable : Pointer to Temp Table structure
//
//---------------------------------------------------

static
int OdmrpGetTempCount(NodeAddress mcastAddr,
                      DataBuffer* buffer)
{
    int i;
    OdmrpTtNode*   tempTable = (OdmrpTtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpTtNode);

    if (numEntry == 0)
    {
        return 0;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (tempTable[i].mcastAddr == mcastAddr)
        {
            return tempTable[i].size;
        }
    }
    return 0;
}// End of OdmrpGetTempCount Function

//----------------------------------------------------
// FUNCTION     OdmrpGetMemberCount
// PURPOSE      Find the number of Sources for a given
//              multicast group in Member Table structure
//
// Parameters:
//  mcastAddr   : Multicast Group
//  memberTable : Pointer to DataBuffer Structure
//----------------------------------------------------

static
int OdmrpGetMemberCount(NodeAddress mcastAddr,
                        DataBuffer* buffer)
{
    int i;
    OdmrpMtNode*   memTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpMtNode);

    if (numEntry == 0)
    {
        return 0;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (memTable[i].mcastAddr == mcastAddr)
        {
            return memTable[i].size;
        }
    }
    return 0;
}// End of OdmrpGetMemberCount Function

//-------------------------------------------------
// FUNCTION     OdmrpGetAckCount
// PURPOSE      Find the number of Sources for a given
//              multicast group in Ack Table structure
//
// Parameters:
//  mcastAddr   : Multicast Group
//  ackTable    : Pointer to Ack Table structure
//-------------------------------------------------

static
int OdmrpGetAckCount(NodeAddress mcastAddr,
                     DataBuffer* buffer)
{
    int i;
    OdmrpAtNode*  ackTable = (OdmrpAtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpAtNode);

    if (numEntry == 0)
    {
        return 0;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (ackTable[i].mcastAddr == mcastAddr)
        {
            return ackTable[i].size;
        }
    }
    return 0;
} // End of OdmrpGetAckCount Function

//------------------------------------------------------------
// FUNCTION     OdmrpJoinGroup
// PURPOSE      Join a new multicast group.
//
// Parameters:
//     node:          Node that is joining the group.
//     mcastAddr:     Multicast group to join.
//------------------------------------------------------------

void OdmrpJoinGroup(Node* node,
                    NodeAddress mcastAddr)
{
    OdmrpMembership    memberFlag;
    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);

    // Join group if not already a member.
    if (!OdmrpLookupMembership(mcastAddr, &odmrp->memberFlag))
    {
        if (ODMRP_DEBUG)
        {
            char addressStr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(mcastAddr, addressStr);
            printf("Node %d joined group %s\n", node->nodeId, addressStr);
        }

        memberFlag.mcastAddr = mcastAddr;
        memberFlag.timestamp = node->getNodeTime();

        BUFFER_AddDataToDataBuffer(&odmrp->memberFlag,
            (char*) &memberFlag,
            sizeof(OdmrpMembership));

    }
} // End of OdmrpJoinGroup Function

//-----------------------------------------------------------
// FUNCTION     OdmrpLeaveGroup
// PURPOSE      Leave a multicast group
//
// Parameters:
//     node:          Node that is leaving the group.
//     mcastAddr:     Multicast group to leave.
//------------------------------------------------------------

void OdmrpLeaveGroup(Node* node, NodeAddress mcastAddr)
{
    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);

    // Leave group if a member
    if (OdmrpLookupMembership(mcastAddr, &odmrp->memberFlag))
    {
        if (ODMRP_DEBUG)
        {
            char addressStr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(mcastAddr, addressStr);
            printf("Node %d left group %s\n", node->nodeId, addressStr);
        }
        OdmrpResetMemberFlag(mcastAddr, &odmrp->memberFlag);
        OdmrpDeleteSourceSent(mcastAddr, &odmrp->sentTable);
        OdmrpDeleteMemberTable(mcastAddr, &odmrp->memberTable);
    }
} // End of OdmrpLeaveGroup Function

//-------------------------------------------------------
// FUNCTION      OdmrpCheckLastSent
// PURPOSE       Check to make sure not to send table too often.
//
// Parameters:
//     mcastAddr:   Multicast group.
//     memberTable: Pointer to DataBuffer structure.
//
// Return: TRUE if sending is allowed; FALSE otherwise
//-------------------------------------------------------

static
BOOL OdmrpCheckLastSent(Node* node,
                        NodeAddress mcastAddr,
                        DataBuffer* buffer)
{
    int i;
    OdmrpMtNode*   memTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpMtNode);

    for (i = 0; i < numEntry; i++)
    {
        if ((memTable[i].mcastAddr == mcastAddr) &&
            (node->getNodeTime() - memTable[i].lastSent
            > ODMRP_JR_PAUSE_TIME))
        {
            return TRUE;
        }

    }
    return FALSE;
} //End of OdmrpCheckLastSent Function

//-------------------------------------------------------------
// FUNCTION      OdmrpUpdateLastSent
// PURPOSE       Update last sent entry of member table.
//
//     mcastAddr:      Multicast group.
//     memberTable: Member table.
//-----------------------------------------------------

static
void OdmrpUpdateLastSent(Node* node,
                         NodeAddress mcastAddr,
                         DataBuffer* buffer)
{
    int i;
    OdmrpMtNode*   memTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpMtNode);

    for (i = 0; i < numEntry; i++)
    {
        if (memTable[i].mcastAddr == mcastAddr)
        {
            memTable[i].lastSent = node->getNodeTime();
            return;
        }
    }
    return;
} //End of OdmrpUpdateLastSent Function

//---------------------------------------------------------------
// FUNCTION     OdmrpCheckAckTable
// PURPOSE      Check to see if sending a join table is necessary.
//              For each entry of the ack table, see if number of retx
//              is less than the number of max tx allowed. If alrealdy
//              maximum, remove entry.
//
// Parameters:
//     mcastAddr: Multicast group
//     ackTable:  Ack table
//
// Return TRUE if retx is required, FALSE otherwise.
//-----------------------------------------------------------------

static
BOOL OdmrpCheckAckTable(NodeAddress mcastAddr,
                        DataBuffer* buffer)
{
    int i;
    int j;
    int numSrc;
    OdmrpAtNode*  ackTable = (OdmrpAtNode *)
        BUFFER_GetData(buffer);
    OdmrpAtSnode*  ackSrcTable;

    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpAtNode);

    if (numEntry == 0)
    {
        return FALSE;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (ackTable[i].mcastAddr == mcastAddr)
        {
            ackSrcTable = (OdmrpAtSnode *)
                BUFFER_GetData(&(ackTable[i].head));

            numSrc = BUFFER_GetCurrentSize(&(ackTable[i].head))
                / sizeof(OdmrpAtSnode);

            for (j = 0; j < numSrc; j++)
            {
                if (ackSrcTable[j].numTx < ODMRP_MAX_NUM_TX)
                {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
} // End of OdmrpCheckAckTable Function

//--------------------------------------------------
// FUNCTION     OdmrpDeleteAckTable
// PURPOSE      Delete Ack Table entry if it is not
//              updated.
//
// Parameters:
//     mcastAddr: Multicast group
//     srcAddr  : Source Address
//     lastAddr : Next Address
//     ackTable : Pointer to Ack table structure
//--------------------------------------------------

static
void OdmrpDeleteAckTable(NodeAddress mcastAddr,
                         NodeAddress srcAddr,
                         NodeAddress lastAddr,
                         DataBuffer* buffer)
{
    int i;
    int j;
    int numSrc;
    OdmrpAtNode*  ackTable = (OdmrpAtNode *)
        BUFFER_GetData(buffer);
    OdmrpAtSnode*  ackSrcTable;

    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpAtNode);

    if (numEntry == 0)
    {
        return;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (ackTable[i].mcastAddr == mcastAddr)
        {
            ackSrcTable = (OdmrpAtSnode *)
                BUFFER_GetData(&(ackTable[i].head));

            numSrc = BUFFER_GetCurrentSize(&(ackTable[i].head))
                / sizeof(OdmrpAtSnode);

            for (j = 0; j < numSrc; j++)
            {
                if ((ackSrcTable[j].numTx >= ODMRP_MAX_NUM_TX)
                    || ((ackSrcTable[j].srcAddr == srcAddr) &&
                    (ackSrcTable[j].nextAddr == lastAddr)))
                {
                    ackTable[i].size = ackTable[i].size - 1;

                    BUFFER_ClearDataFromDataBuffer(
                        &(ackTable[i].head),
                        (char*) &ackSrcTable[j],
                        sizeof(OdmrpAtSnode),
                        FALSE);
                    if (j != numSrc - 1)
                    {
                        j--;
                        numSrc--;
                    }
                    if (ackTable[i].size == 0)
                    {
                        return;
                    }

                }

            }
        }
    }
    return;
}

//-------------------------------------------------------
// FUNCTION     OdmrpDeleteResponseTable
// PURPOSE      Delete Response Table structure
//              for a given multicast address
//
// Parameters:
//     mcastAddr: Multicast Group
//   rspnsTable : Pointer to DataBuffer structure
//-------------------------------------------------------

static
void OdmrpDeleteResponseTable(NodeAddress mcastAddr,
                              DataBuffer* buffer)
{
    int numEntry;
    BOOL isFound = FALSE;
    int i;
    OdmrpRptNode*  rspnsTable = (OdmrpRptNode *)
        BUFFER_GetData(buffer);

    numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpRptNode);

    if (numEntry == 0)
    {
        return;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (rspnsTable[i].mcastAddr == mcastAddr)
        {
            isFound = TRUE;
            break;
        }
    }

    if (isFound == FALSE)
    {
        return;
    }
    else
    {
        BUFFER_DestroyDataBuffer(&(rspnsTable[i].head));
        BUFFER_RemoveDataFromDataBuffer(buffer,
            (char*) &rspnsTable[i],
            sizeof(OdmrpRptNode));
    }
}// End of OdmrpDeleteResponseTable Function

//----------------------------------------------------
// FUNCTION     OdmrpCheckResponseMatch
// PURPOSE      Search for a particular Source for a given
//              multicast address in Response Table structure
//
// Parameters:
//     mcastAddr: Multicast group
//      srcAddr : Source Address
//   rspnsTable : Pointer to DataBuffer structure
//
//-----------------------------------------------------

static
BOOL OdmrpCheckResponseMatch(NodeAddress mcastAddr,
                             NodeAddress srcAddr,
                             DataBuffer* buffer)
{
    OdmrpRptSnode* rspnsSrcTable;
    BOOL isFound = FALSE;
    int i;
    int numEntry;
    OdmrpRptNode* rspnsTable = (OdmrpRptNode *)
        BUFFER_GetData(buffer);

    numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpRptNode);

    if (numEntry == 0)
    {
        return FALSE;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (rspnsTable[i].mcastAddr == mcastAddr)
        {
            isFound = TRUE;
            break;
        }
    }

    if (isFound == FALSE)
    {
        return FALSE;

    }
    else
    {
        rspnsSrcTable = (OdmrpRptSnode *)
            BUFFER_GetData(&(rspnsTable[i].head));
        numEntry = BUFFER_GetCurrentSize(&(rspnsTable[i].head))
            / sizeof(OdmrpRptSnode);

        for (i = 0; i < numEntry; i++)
        {
            if (rspnsSrcTable[i].srcAddr == srcAddr)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}// End of OdmrpCheckResponseMatch Function

//------------------------------------------------------
// FUNCTION     OdmrpCheckMinExpTime
// PURPOSE      Sent Table structure is maintained by
//              each Source. Check whether Source has
//              been expired for a particular
//              multicast group
//
// Parameters:
//     mcastAddr: Multicast group
//     sentTable: Pointer to Sent Table structure
//------------------------------------------------------

static
BOOL OdmrpCheckMinExpTime(Node* node,
                          NodeAddress mcastAddr,
                          DataBuffer* buffer)
{
    int i;
    OdmrpSs*     sentTable = (OdmrpSs *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpSs);

    for (i = 0; i < numEntry; i++)
    {
        if ((sentTable[i].mcastAddr == mcastAddr)
            && (node->getNodeTime() > (sentTable[i]).minExpireTime))
        {
            return TRUE;

        }
    }
    return FALSE;
}// End of OdmrpCheckMinExpTime Function

//-----------------------------------------------------
// FUNCTION     OdmrpCheckCongestionTime
// PURPOSE      Check the network congestion while
//              sending a Join Query message
//
//
// Parameters:
//        node  :
//     mcastAddr: Multicast group
//   memberTable: Pointer to DataBuffer structure
//-----------------------------------------------------

static
BOOL OdmrpCheckCongestionTime(Node* node,
                              NodeAddress mcastAddr,
                              DataBuffer* buffer)
{
    int i;
    OdmrpMtNode*  memTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpMtNode);

    if (numEntry == 0)
    {
        return TRUE;
    }

    for (i = 0; i < numEntry; i++)
    {
        if ((memTable[i].mcastAddr == mcastAddr) &&
            (node->getNodeTime() - memTable[i].queryLastReceived
            < ODMRP_CONGESTION_TIME))
        {
            return FALSE;
        }
    }
    return TRUE;

}// End of OdmrpCheckCongestionTime Function

//-------------------------------------------------
// FUNCTION     OdmrpCheckSendQuery
// PURPOSE      Check whether Join Query is sent recently
//              If TRUE,then data packet will be sent not
//              Join Query message
// Parameters:
//     mcastAddr: Multicast group
//   sentTable  : Pointer to DataBuffer
//-------------------------------------------------

static
BOOL OdmrpCheckSendQuery(NodeAddress mcastAddr,
                         DataBuffer* buffer)
{
    int numEntry;
    int i;
    OdmrpSs*     sentTable = (OdmrpSs *)
        BUFFER_GetData(buffer);
    numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpSs);

    if (numEntry <= 0)
    {
        return FALSE;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (sentTable[i].mcastAddr == mcastAddr)

        {
            return sentTable[i].nextQuerySend;
        }
    }
    return FALSE;
}// End of OdmrpCheckSendQuery Function

//-------------------------------------------------
// FUNCTION     OdmrpSetSendQuery
// PURPOSE      It sets nextQuerySend variable to TRUE
//              Next time the Source will send Join
//              Query packet with data payload piggybacked
//
// Parameters:
//     mcastAddr: Multicast group
//   sentTable  : Pointer to DataBuffer
//-------------------------------------------------

static
void OdmrpSetSendQuery(NodeAddress mcastAddr,
                       DataBuffer* buffer)
{

    int numEntry;
    int i;
    OdmrpSs*     sentTable = (OdmrpSs *)
        BUFFER_GetData(buffer);
    numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpSs);

    if (numEntry <= 0)
    {
        return;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (sentTable[i].mcastAddr == mcastAddr)

        {
            sentTable[i].nextQuerySend = TRUE;
            return;
        }
    }
    return;

}// End of OdmrpSetSendQuery Function

//----------------------------------------------------
// FUNCTION     OdmrpUnsetSendQuery
// PURPOSE      It sets nextQuerySend variable to FALSE
//              Next time the Source will not send Join
//              Query packet with data payload piggybacked
//
// Parameters:
//     mcastAddr: Multicast group
//   sentTable  : Pointer to DataBuffer
//----------------------------------------------------

static
void OdmrpUnsetSendQuery(NodeAddress mcastAddr,
                         DataBuffer* buffer)
{
    int numEntry;
    int i;
    OdmrpSs*     sentTable = (OdmrpSs *)
        BUFFER_GetData(buffer);

    numEntry = BUFFER_GetCurrentSize(buffer) / sizeof(OdmrpSs);

    if (numEntry <= 0)
    {
        return;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (sentTable[i].mcastAddr == mcastAddr)

        {
            sentTable[i].nextQuerySend = FALSE;
            return;
        }
    }
    return;
}// End of OdmrpUnsetSendQuery Function

//-------------------------------------------------
// FUNCTION     OdmrpSetMinExpireTime
// PURPOSE      Set the Expire Time for the source
//              for a given multicast group
//
//
// Parameters:
//        node  :
//     mcastAddr: Multicast group
//-------------------------------------------------

static
void OdmrpSetMinExpireTime(Node* node,
                           NodeAddress mcastAddr,
                           OdmrpData* odmrp)
{
    int numEntry;
    int i;
    OdmrpSs*     sentTable = (OdmrpSs *)
        BUFFER_GetData(&(odmrp->sentTable));
    numEntry = BUFFER_GetCurrentSize(&(odmrp->sentTable))
        / sizeof(OdmrpSs);

    if (numEntry <= 0)
    {
        return;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (sentTable[i].mcastAddr == mcastAddr)

        {
            sentTable[i].minExpireTime
                = node->getNodeTime() + odmrp->refreshTime;
            return;
        }
    }
    return;
} //End of OdmrpSetMinExpireTime Function

//----------------------------------------------------
// FUNCTION     OdmrpCheckTempSent
// PURPOSE      Check the variable sent in Temp Table structure.
//              If the value of this variable is TRUE,then do not
//              collect Join Reply message otherwise collect and
//              forward Join Reply message
// Parameters:
//     mcastAddr: Multicast group
//   tempTable  : Pointer to DataBuffer structure
//----------------------------------------------------

static
BOOL OdmrpCheckTempSent(NodeAddress mcastAddr,
                        DataBuffer* buffer)
{
    int i;
    OdmrpTtNode*  tempTable = (OdmrpTtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpTtNode);

    if (numEntry == 0)
    {
        return FALSE;
    }
    for (i = 0; i < numEntry; i++)
    {
        if (tempTable[i].mcastAddr == mcastAddr)
        {
            return tempTable[i].sent;
        }
    }
    return  FALSE;
}// End of OdmrpCheckTempSent Function

//--------------------------------------------------
// FUNCTION     OdmrpSetTempSent
// PURPOSE      Set the variable "sent" in Temp
//              Table structure.
// Parameters:
//     mcastAddr: Multicast group
//   tempTable  : Pointer to Temp Table structure
//--------------------------------------------------

static
void OdmrpSetTempSent(NodeAddress mcastAddr,
                      DataBuffer* buffer)
{
    int i;
    OdmrpTtNode*  tempTable = (OdmrpTtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpTtNode);

    for (i = 0; i < numEntry; i++)
    {
        if (tempTable[i].mcastAddr == mcastAddr)
        {
            tempTable[i].sent = TRUE;
            return;
        }
    }
    return;
}// End of OdmrpSetTempSent Function

//--------------------------------------------------
// FUNCTION     OdmrpUnsetTempSent
// PURPOSE      Unset the variable "sent" in Temp
//              Table structure.
// Parameters:
//     mcastAddr: Multicast group
//   tempTable  : Pointer to Temp Table structure
//--------------------------------------------------

static
void OdmrpUnsetTempSent(NodeAddress mcastAddr,
                        DataBuffer* buffer)
{
    int i;
    OdmrpTtNode*  tempTable = (OdmrpTtNode *)
        BUFFER_GetData(buffer);

    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpTtNode);

    for (i = 0; i < numEntry; i++)
    {
        if (tempTable[i].mcastAddr == mcastAddr)
        {
            tempTable[i].sent = FALSE;
            return;
        }
    }
    return;

}// End of OdmrpUnsetTempSent Function

//------------------------------------------------------
// FUNCTION     OdmrpSetMemberSent
// PURPOSE      Set the variable "sent" in Member Table structure.
// Parameters:
//     mcastAddr  : Multicast group
//   memberTable  : Pointer to DataBuffer structure
//------------------------------------------------------

static
void OdmrpSetMemberSent(NodeAddress mcastAddr,
                        DataBuffer* buffer)
{
    int i;
    OdmrpMtNode*  memTable = (OdmrpMtNode *)
        BUFFER_GetData(buffer);
    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpMtNode);

    for (i = 0; i < numEntry; i++)
    {
        if (memTable[i].mcastAddr == mcastAddr)
        {
            memTable[i].sent = TRUE;
            return;
        }
    }
    return;
}// End of OdmrpSetMemberSent Function

//--------------------------------------------------------------
// FUNCTION     OdmrpRetxReply
// PURPOSE      Retransmit ODMRP Join Reply message
//
//
// Parameters:
//     node     : Node that is transmitting Join Reply
//   mcastAddr  : Multicast Group
//  ackTable    : Pointer to Ack Table
//--------------------------------------------------------------

static
void OdmrpRetxReply(OdmrpData* odmrp,
                    Node* node,
                    NodeAddress mcastAddr,
                    DataBuffer* buffer)

{
    Message*         msg = 0;
    NodeAddress      nodeIPAddress;
    OdmrpJoinReply   tempReplyPkt;
    OdmrpAtNode*     ackTable;
    OdmrpAtSnode*    ackSrcTable = NULL;
    int numEntry;
    int i;
    int j;
    int count = 0;
    int tupleFormSize;
    char* pktPtr = 0;
    int tupleSize = 2 * sizeof(NodeAddress);
    int pktSize = sizeof(OdmrpJoinReply);
    clocktype delay;

    nodeIPAddress = NetworkIpGetInterfaceAddress(
        node,
        GetDefaultInterfaceIndex(node, NETWORK_IPV4)),
        tempReplyPkt.replyCount = (unsigned char)OdmrpGetAckCount(mcastAddr,
        &odmrp->ackTable);

    // Used for implicit ACK.
    OdmrpJoinReplySetIAmFG(&(tempReplyPkt.ojr),
        OdmrpLookupFgFlag(mcastAddr,&odmrp->fgFlag));
    ackTable = (OdmrpAtNode *)
        BUFFER_GetData(&odmrp->ackTable);

    numEntry = BUFFER_GetCurrentSize(&odmrp->ackTable)
        / sizeof(OdmrpAtNode);

    for (i = 0; i < numEntry; i++)
    {
        if (ackTable[i].mcastAddr == mcastAddr)
        {
            ackSrcTable = (OdmrpAtSnode *)
                BUFFER_GetData(&(ackTable[i].head));
            count = i;
            break;
        }
    }

    if (tempReplyPkt.replyCount > 0)
    {
        delay = (clocktype) (RANDOM_erand(odmrp->retxJitterSeed) * ODMRP_JR_RETX_JITTER);

        if (ODMRP_DEBUG)
        {
            char clockStr[100];
            ctoa(node->getNodeTime() + delay, clockStr);
            printf("    sending at time %s\n", clockStr);
        }

        // Filling up the remaining field
        tempReplyPkt.pktType = (unsigned char) ODMRP_JOIN_REPLY;
        OdmrpJoinReplySetReserved(&(tempReplyPkt.ojr), 0);
        tempReplyPkt.multicastGroupIPAddr = mcastAddr;
        tempReplyPkt.previousHopIPAddr =  nodeIPAddress;
        tempReplyPkt.seqNum = OdmrpGetSeq(odmrp);
        tupleFormSize = tempReplyPkt.replyCount * tupleSize;
        pktSize = pktSize + tupleFormSize;

        msg = MESSAGE_Alloc(node,
            MAC_LAYER,
            0,
            MSG_MAC_FromNetwork);

        MESSAGE_PacketAlloc(node,
            msg,
            pktSize,
            TRACE_ODMRP);

        pktPtr = (char *) MESSAGE_ReturnPacket(msg);

        memcpy(pktPtr,
            &tempReplyPkt,
            sizeof(OdmrpJoinReply));

        pktPtr = pktPtr + sizeof(OdmrpJoinReply);

        for (i = 0; i < (signed) tempReplyPkt.replyCount; i++)
        {
            memcpy(pktPtr,
                &(ackSrcTable[i].srcAddr),
                sizeof(NodeAddress));
            pktPtr = pktPtr + sizeof(NodeAddress);
            memcpy(pktPtr,
                &(ackSrcTable[i].nextAddr),
                sizeof(NodeAddress));
            pktPtr = pktPtr + sizeof(NodeAddress);
        }

        NetworkIpSendRawMessageToMacLayerWithDelay(node,
            msg,
            nodeIPAddress,
            ANY_DEST,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_ODMRP,
            IPDEFTTL,
            DEFAULT_INTERFACE,
            ANY_DEST,
            delay);
        odmrp->stats.numReplyTransmitted++;

        if (ODMRP_DEBUG)
        {
            printf("Node %d retransmitting a Join Reply\n",
                node->nodeId);
        }

        OdmrpSetTimer(node,
            MSG_NETWORK_CheckAcked,
            &mcastAddr,
            sizeof(NodeAddress),
            ODMRP_CHECKACK_INTERVAL);

        for (j = 0; j < (signed) tempReplyPkt.replyCount; j++)
        {
            OdmrpInsertAckSource(node,
                ackSrcTable[j].srcAddr,
                ackSrcTable[j].nextAddr,
                (OdmrpAtNode *)&ackTable[count]);
        }

    }

} // End of OdmrpRetxReply Function


//--------------------------------------------------------------
//  FUNCTION     OdmrpInit
//  PURPOSE      Initialization function for ODMRP protocol of
//               NETWORK layer
//
//  Parameters:
//     node       : Node being initialized
//     nodeInput  : Reference to input file.
// interfaceIndex : Interface Index
//
//-------------------------------------------------------------

void OdmrpInit(Node* node,
               const NodeInput* nodeInput,
               int interfaceIndex)
{
    // read the config file to know whether PASSIVE CLUSTERING on
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    NodeAddress interfaceAddress;
    interfaceAddress = NetworkIpGetInterfaceAddress(node,interfaceIndex);

    OdmrpData* odmrp =
        (OdmrpData *)MEM_malloc(sizeof(OdmrpData));

    RANDOM_SetSeed(odmrp->retxJitterSeed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_ODMRP,
                   interfaceIndex);
    RANDOM_SetSeed(odmrp->jrJitterSeed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_ODMRP,
                   interfaceIndex + 1);

    RANDOM_SetSeed(odmrp->broadcastJitterSeed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_ODMRP,
                   interfaceIndex + 2);

    IO_ReadString(node->nodeId,
        interfaceAddress,
        nodeInput,
        "ODMRP-PASSIVE-CLUSTERING",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf,"YES")==0)
        {
            odmrp->pcEnable = TRUE;
        }
        else if (strcmp(buf,"NO")==0)
        {
            odmrp->pcEnable = FALSE;
        }
        else
        {
            ERROR_ReportError("ODMRP-PASSIVE-CLUSTERING "
                "should be either YES or NO\n");
        }
    }
    else
    {
        odmrp->pcEnable = FALSE;
    }

    ERROR_Assert(!MAC_IsWiredNetwork(node, interfaceIndex),
                 "ODMRP can only support wireless interfaces\n");

    NetworkIpSetMulticastRoutingProtocol(node,
                                         odmrp,
                                         interfaceIndex);

    ERROR_Assert(odmrp != NULL,
                 "ODMRP: Cannot alloc memory for ODMRP struct!\n");

    NetworkIpSetMulticastRouterFunction(node,
                                        &OdmrpRouterFunction,
                                        interfaceIndex);

    // Initialize Odmrp Specific Data Buffer Structure
    OdmrpInitializeBuffer(odmrp);
    OdmrpInitStats(odmrp);
    OdmrpInitMessageCache(&odmrp->messageCache);
    OdmrpInitMemoryPool(&odmrp->freePool);

    // trace init
    OdmrpInitTrace(node, nodeInput);
    // Initialize Sequence Table
    odmrp->seqTable = 0;

    if (odmrp->pcEnable)
    {
        OdmrpInitStates(odmrp);
        OdmrpInitClusterHead(&odmrp->clsHeadList);
        OdmrpInitGateway(&odmrp->clsGwList);
        OdmrpInitDistGateway(&odmrp->clsDistGwList);
        OdmrpInitGatewayEntry(&odmrp->gwClsEntry);

        NetworkIpSetPromiscuousMessagePeekFunction(node,
            &OdmrpPeekFunction,
            interfaceIndex);
    }
    IO_ReadTime(node->nodeId,
        interfaceAddress,
        nodeInput,
        "ODMRP-JR-REFRESH",
        &retVal,
        &odmrp->refreshTime);

    if (retVal == FALSE)
    {
        odmrp->refreshTime = ODMRP_JR_REFRESH;
    }
    OdmrpValidateTimer(odmrp->refreshTime);

    IO_ReadTime(node->nodeId,
        interfaceAddress,
        nodeInput,
        "ODMRP-FG-TIMEOUT",
        &retVal,
        &odmrp->expireTime);

    if (retVal == FALSE)
    {
        odmrp->expireTime = ODMRP_FG_TIMEOUT;
    }
    OdmrpValidateExpireTime(odmrp->refreshTime,
        odmrp->expireTime);

    IO_ReadInt(node->nodeId,
        interfaceAddress,
        nodeInput,
        "ODMRP-DEFAULT-TTL",
        &retVal,
        &odmrp->ttlValue);

    if (retVal == FALSE)
    {
        odmrp->ttlValue = ODMRP_DEFAULT_TTL;
    }

    OdmrpValidateTTLValue(odmrp->ttlValue,
        0,
        ODMRP_MAX_TTL_VALUE);


    if (odmrp->pcEnable)
    {
        IO_ReadTime(node->nodeId,
            interfaceAddress,
            nodeInput,
            "ODMRP-CLUSTER-TIMEOUT",
            &retVal,
            &odmrp->clsExpireTime);

        if (retVal == FALSE)
        {
            odmrp->clsExpireTime = ODMRP_CLUSTER_TIMEOUT;
        }
        OdmrpValidateTimer(odmrp->clsExpireTime);
    }
} // End of OdmrpInit Function

//-------------------------------------------------------------
//  FUNCTION     OdmrpPopulateReplyWithRouteTable
//  PURPOSE      Populate "Join Reply" packet with
//               Route Table
//
//  Parameters:
//    tupleForm   : Odmrp Join Reply Form
//    routeTable  : Pointer to Data Buffer
//    replyPkt    : Odmrp Join Reply Packet
//--------------------------------------------------------------

static
void OdmrpPopulateReplyWithRouteTable(OdmrpJoinTupleForm *tupleForm,
                                      DataBuffer* routeTable,
                                      OdmrpJoinReply *replyPkt)
{
    unsigned char count;

    for (count = 0; count < replyPkt->replyCount; count++)
    {
        tupleForm[count].nextAddr =
            OdmrpGetNextNode(tupleForm[count].srcAddr,
            routeTable);

    }
} // End of OdmrpPopulateReplyWithRouteTable Function

//-------------------------------------------------------------
//  FUNCTION     OdmrpPopulateReplyWithTempTable
//  PURPOSE      Populate "Join Reply" packet with
//               Temp Table
//
//  Parameters:
//    tupleForm   : Odmrp Join Reply TupleForm
//    mcastAddr   : Multicast Address
//    buffer      : Pointer to Data Buffer
//    routeTable  : Pointer to Data Buffer
//    replyPkt    : Odmrp Join Reply Packet
//--------------------------------------------------------------

static
void OdmrpPopulateReplyWithTempTable(OdmrpJoinTupleForm* tupleForm,
                                     NodeAddress mcastAddr,
                                     DataBuffer* buffer,
                                     DataBuffer* routeTable,
                                     OdmrpJoinReply *replyPkt)
{
    int i;
    int count;
    int numSrc;
    BOOL done;
    int k;
    OdmrpTtNode*  tempTable = (OdmrpTtNode *)
        BUFFER_GetData(buffer);
    OdmrpTtSnode*  tempSrcTable;

    int numEntry = BUFFER_GetCurrentSize(buffer)
        / sizeof(OdmrpTtNode);

    if (numEntry == 0)
    {
        return;
    }

    for (i = 0; i < numEntry; i++)
    {
        if (tempTable[i].mcastAddr == mcastAddr)
        {
            tempSrcTable = (OdmrpTtSnode *)
                BUFFER_GetData(&(tempTable[i].head));

            numSrc = BUFFER_GetCurrentSize(&(tempTable[i].head))
                / sizeof(OdmrpTtSnode);

            for (count = 0; count < numSrc; count++)
            {
                done = FALSE;
                for (k = 0;k < (signed) replyPkt->replyCount;k++)
                {
                    if (tupleForm[k].srcAddr ==
                        tempSrcTable[count].srcAddr)
                    {
                        done = TRUE;
                        break;
                    }

                }
                if (done == FALSE)
                {
                    tupleForm[replyPkt->replyCount].srcAddr =
                        tempSrcTable[count].srcAddr;
                    tupleForm[replyPkt->replyCount].nextAddr =
                        OdmrpGetNextNode(tempSrcTable[count].srcAddr,
                        routeTable);
                    replyPkt->replyCount++;
                }
            }
        }
    }
} // End of OdmrpPopulateReplyWithTempTable Function

//-------------------------------------------------------------
//  FUNCTION     OdmrpPopulateReplyWithResponseTable
//  PURPOSE      Populate "Join Reply" packet with
//               Response Table
//
//  Parameters:
//    mcastAddr      : Multicast Address
//    tupleForm      :  Odmrp Join Tuple Form
//    responseTable  : Pointer to Data Buffer
//    replyPkt       : Odmrp Join Reply Packet
//--------------------------------------------------------------

static
void OdmrpPopulateReplyWithResponseTable(NodeAddress mcastAddr,
                                         OdmrpJoinTupleForm* tupleForm,
                                         DataBuffer* responseTable,
                                         OdmrpJoinReply* replyPkt)
{
    BOOL match;
    int l;
    int m;

    l = 0;
    // Check each entry in JR if in response.
    while (l < (signed) replyPkt->replyCount) {
        match = FALSE;
        match = OdmrpCheckResponseMatch(mcastAddr,
            tupleForm[l].srcAddr,
            responseTable);
        // Not found in response table, delete source.
        if (!match)
        {
            for (m = l; m < (signed) replyPkt->replyCount; m++)
            {
                tupleForm[m].srcAddr =
                    tupleForm[m+1].srcAddr;
                tupleForm[m].nextAddr =
                    tupleForm[m+1].nextAddr;
            }
            replyPkt->replyCount--;
        }
        if (match)
        {
            l++;
        }
    }

} // End of OdmrpPopulateReplyWithResponseTable Function

//------------------------------------------------
// FUNCTION     OdmrpSendReply
// PURPOSE      Send ODMRP Join Reply message to the
//              broadcast Address.
//
// Parameters:
//     node:      Node that is sending Join Reply
//   mcastAddr  : Multicast Group
//   messageInfo: Void Pointer
//   memberTable: Pointer to DataBuffer structure
//    tempTable : Pointer to DataBuffer structure
//
//------------------------------------------------

static
void OdmrpSendReply(Node* node,
                    NodeAddress mcastAddr,
                    DataBuffer* memberTable,
                    DataBuffer* tempTable)
{

    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);
    Message* msg = NULL;
    OdmrpJoinReply tempReplyPkt = {0};
    char errString[MAX_STRING_LENGTH];
    NodeAddress    nodeIPAddress;
    BOOL   notFree;
    OdmrpAtNode*   mcastEntry = NULL;
    OdmrpJoinTupleForm  tupleFormArray[50];
    OdmrpJoinTupleForm*  tupleForm;
    char*  pktPtr = NULL;
    int    tupleSize;
    int    tupleFormSize;
    int    maxTupleFormSize;
    OdmrpTtNode*   ttTable;
    OdmrpTtSnode*  ttSrcTable;
    int    memberCount;
    int    tempCount;
    int    n;
    int    l;
    int    i;
    int    totalPktSize;
    int    numEntry;
    int    count;
    int    numSrc;
    int    pktSize = sizeof(OdmrpJoinReply);


    memset(tupleFormArray, 0 ,sizeof(OdmrpJoinTupleForm) * 50);

    tupleSize  =  2 * sizeof(NodeAddress);
    nodeIPAddress = NetworkIpGetInterfaceAddress(node,
        GetDefaultInterfaceIndex(node, NETWORK_IPV4));
    memberCount = OdmrpGetMemberCount(mcastAddr,
        &odmrp->memberTable);
    tempCount =   OdmrpGetTempCount(mcastAddr,
        &odmrp->tempTable);

    maxTupleFormSize = memberCount + tempCount + 1;

    if (maxTupleFormSize <= 50)
    {
        tupleForm = &tupleFormArray[0];
        notFree = TRUE;
    }
    else
    {
        tupleForm = (OdmrpJoinTupleForm *)MEM_malloc
            (maxTupleFormSize * sizeof(OdmrpJoinTupleForm));
        memset(tupleForm, 0,
            sizeof(OdmrpJoinTupleForm) * maxTupleFormSize);
        notFree = FALSE;
    }

    // Stop collecting JR from down stream.
    OdmrpUnsetTempSent(mcastAddr, &odmrp->tempTable);

    // if the node is a member of the group.
    if (OdmrpLookupMembership(mcastAddr, &odmrp->memberFlag))
    {

        if (ODMRP_DEBUG)
        {
            printf("Member node %d sending a Join Reply\n",
                node->nodeId);
        }

        // Remove sources that have been expired.
        OdmrpCheckSourceExpired(node,
            mcastAddr,
            &odmrp->memberTable);

        // Send table only if valid source members exist in table.
        if (OdmrpLookupMemberTable(mcastAddr,
            &odmrp->memberTable) &&
            OdmrpCheckLastSent(node,
            mcastAddr,
            &odmrp->memberTable))
        {
            if (ODMRP_DEBUG)
            {
                printf("       There are members!\n");
            }
            tempReplyPkt.replyCount = (unsigned char)
                OdmrpGetMemberCount(mcastAddr, &odmrp->memberTable);

            // Used for implicit ACK
            OdmrpJoinReplySetIAmFG(&(tempReplyPkt.ojr),
                OdmrpLookupFgFlag(mcastAddr, &odmrp->fgFlag));

            OdmrpPopulateReplyWithMemTable(tupleForm,
                memberTable,
                mcastAddr);
            OdmrpPopulateReplyWithRouteTable(tupleForm,
                &odmrp->routeTable,
                &tempReplyPkt);
            OdmrpPopulateReplyWithTempTable(tupleForm,
                mcastAddr,
                tempTable,
                &odmrp->routeTable,
                &tempReplyPkt);
            OdmrpPopulateReplyWithResponseTable(mcastAddr,
                tupleForm,
                &odmrp->responseTable,
                &tempReplyPkt);

            if (tempReplyPkt.replyCount > 0)
            {
                // Clear response table.
                OdmrpDeleteResponseTable(mcastAddr,
                    &odmrp->responseTable);


                odmrp->stats.numReplySent++;

                // Set sent flag to true (stop collecting route).
                OdmrpSetMemberSent(mcastAddr, &odmrp->memberTable);

                // Record when the table was sent last.
                OdmrpUpdateLastSent(node,
                    mcastAddr,
                    &odmrp->memberTable);

                mcastEntry = OdmrpGetATEntry(mcastAddr,
                    &odmrp->ackTable);
                if (mcastEntry == NULL)
                {
                    OdmrpInsertAckTable(node,
                        mcastAddr,
                        &odmrp->ackTable,
                        &tempReplyPkt,
                        tupleForm);
                }
                else
                {
                    for (n = 0; n < (signed) tempReplyPkt.replyCount; n++)
                    {
                        OdmrpInsertAckSource(node,
                            tupleForm[n].srcAddr,
                            tupleForm[n].nextAddr,
                            mcastEntry);
                    }
                }

            }

        } // If member exist and not sent recently
        else if (OdmrpLookupMemberTable(mcastAddr,
            &odmrp->memberTable))
        {
            if (ODMRP_DEBUG)
            {
                printf(
                    "Node %d send Join Reply recently."
                    "Sending DELAYED\n", node->nodeId);
            }
            OdmrpSetTimer(node,
                MSG_NETWORK_SendReply,
                &mcastAddr,
                sizeof(NodeAddress),
                ODMRP_JR_PAUSE_TIME);
            if (notFree == FALSE)
            {
                MEM_free(tupleForm);
            }
            return;
        }

    } //  If the node is a member of the multicast group

    // Send joing table for forwarding group.
    else if (OdmrpLookupFgFlag(mcastAddr, &odmrp->fgFlag))
    {
        if (ODMRP_DEBUG)
        {
            printf("FG node %d sending a Join Reply\n", node->nodeId);
        }

        tempReplyPkt.replyCount = (unsigned char)
            OdmrpGetTempCount(mcastAddr, &odmrp->tempTable);

        // Set Forwarding Flag
        OdmrpJoinReplySetIAmFG(&(tempReplyPkt.ojr),
            OdmrpLookupFgFlag(mcastAddr, &odmrp->fgFlag));

        ttTable = (OdmrpTtNode *)
            BUFFER_GetData(&odmrp->tempTable);

        numEntry = BUFFER_GetCurrentSize(&odmrp->tempTable)
            / sizeof(OdmrpTtNode);

        for (i = 0; i < numEntry; i++)
        {
            if (ttTable[i].mcastAddr == mcastAddr)
            {
                ttSrcTable = (OdmrpTtSnode *)
                    BUFFER_GetData(&(ttTable[i].head));

                numSrc = BUFFER_GetCurrentSize(&(ttTable[i].head))
                    / sizeof(OdmrpTtSnode);
                for (count = 0; count < numSrc; count++)
                {
                    tupleForm[count].srcAddr
                        = ttSrcTable[count].srcAddr;

                }
            }
        }

        for (i = 0; i < (signed) tempReplyPkt.replyCount; i++)
        {
            tupleForm[i].nextAddr =
                OdmrpGetNextNode(tupleForm[i].srcAddr,
                &odmrp->routeTable);
        }

        OdmrpPopulateReplyWithResponseTable(mcastAddr,
            tupleForm,
            &odmrp->responseTable,
            &tempReplyPkt);
        odmrp->stats.numReplyForwarded++;

        if (tempReplyPkt.replyCount > 0)
        {
            // Clean response table.
            OdmrpDeleteResponseTable(mcastAddr,
                &odmrp->responseTable);

            mcastEntry = OdmrpGetATEntry(mcastAddr,
                &odmrp->ackTable);

            if (mcastEntry == NULL)
            {
                OdmrpInsertAckTable(node,
                    mcastAddr,
                    &odmrp->ackTable,
                    &tempReplyPkt,
                    tupleForm);
            }
            else
            {
                for (n = 0; n < (signed) tempReplyPkt.replyCount; n++)
                {
                    OdmrpInsertAckSource(node,
                        tupleForm[n].srcAddr,
                        tupleForm[n].nextAddr,
                        mcastEntry);

                }
            }
        }
    }

    if (ODMRP_DEBUG)
    {
        char addressStr1[MAX_STRING_LENGTH] , addressStr2[MAX_STRING_LENGTH];// Added

        for (l = 0; l < (signed) tempReplyPkt.replyCount; l++)  {
            IO_ConvertIpAddressToString(tupleForm[l].srcAddr, addressStr1);
            IO_ConvertIpAddressToString(tupleForm[l].nextAddr, addressStr2);
            printf("src[%d] = %s, nxt[%d] = %s\n",
                l, addressStr1, l,
                addressStr2);
        }
    }

    if (ODMRP_DEBUG)
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("    sending at time %s\n", clockStr);
    }

    if (tempReplyPkt.replyCount <= 0)
    {
        if (notFree == FALSE)
        {
            MEM_free(tupleForm);
        }
        return;
    }

    totalPktSize = sizeof(OdmrpJoinTupleForm) * tempReplyPkt.replyCount
        + sizeof(OdmrpJoinReply) + sizeof(IpHeaderType);

    if (totalPktSize > GetNetworkIPFragUnit(node, (int)DEFAULT_INTERFACE))
    {
        sprintf(errString,"ODMRP doesn't support IP Fragmentation, "
            "increase MAX_NW_PKT_SIZE value and run again\n");

        ERROR_ReportError(errString);
        return;
    }

    // Filling up the remaining field
    tempReplyPkt.pktType = (unsigned char) ODMRP_JOIN_REPLY;
    OdmrpJoinReplySetReserved(&(tempReplyPkt.ojr), 0);
    tempReplyPkt.multicastGroupIPAddr = mcastAddr;
    tempReplyPkt.previousHopIPAddr =  nodeIPAddress;
    tempReplyPkt.seqNum = OdmrpGetSeq(odmrp);

    tupleFormSize = tempReplyPkt.replyCount * tupleSize;
    pktSize = pktSize + tupleFormSize;

    msg = MESSAGE_Alloc(node,
        MAC_LAYER,
        0,
        MSG_MAC_FromNetwork);
    MESSAGE_PacketAlloc(node,
        msg,
        pktSize,
        TRACE_ODMRP);
    pktPtr = (char *) MESSAGE_ReturnPacket(msg);

    memcpy(pktPtr,
        &tempReplyPkt,
        sizeof(OdmrpJoinReply));

    pktPtr = pktPtr + sizeof(OdmrpJoinReply);


    memcpy(pktPtr,
        &(tupleForm[0].srcAddr),
        sizeof(OdmrpJoinTupleForm)* tempReplyPkt.replyCount);
    pktPtr = pktPtr +sizeof(OdmrpJoinTupleForm)* tempReplyPkt.replyCount;


    //Trace sending pkt
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_NETWORK_LAYER,PACKET_OUT, &acnData);

    NetworkIpSendRawMessageToMacLayer(node,
        msg,
        nodeIPAddress,
        ANY_DEST,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_ODMRP,
        IPDEFTTL,
        DEFAULT_INTERFACE,
        ANY_DEST);

    OdmrpSetTimer(node,
        MSG_NETWORK_CheckAcked,
        &mcastAddr,
        sizeof(NodeAddress),
        ODMRP_CHECKACK_INTERVAL);

    if (notFree == FALSE)
    {
        MEM_free(tupleForm);
    }

}  // End of Function OdmrpSendReply Function

//-----------------------------------------------------
// FUNCTION     OdmrpSendData
// PURPOSE      Send Data packet and no Join Query
//              message is attached with it
//
// Parameters:
//      node        :
//      msg         : Pointer to message structure
//   mcastAddr      : Multicast Group
//  originalProtocol: The protocol field in IP packet
//-----------------------------------------------------

static
void OdmrpSendData(OdmrpData* odmrp,
                   Node* node,
                   Message* msg,
                   NodeAddress mcastAddr,
                   unsigned int originalProtocol)
{
    OdmrpIpOptionType option;
    IpHeaderType* ipHeader =  0;
    NodeAddress  nodeIPAddress;
    char errString[MAX_STRING_LENGTH];
    clocktype randDelay = (clocktype)
        (RANDOM_erand(odmrp->broadcastJitterSeed) * ODMRP_BROADCAST_JITTER);
    //clocktype randDelay = 0;
    MESSAGE_SetLayer(msg, MAC_LAYER, 0);
    MESSAGE_SetEvent(msg, MSG_MAC_FromNetwork);

    nodeIPAddress = NetworkIpGetInterfaceAddress(node,
        GetDefaultInterfaceIndex(node, NETWORK_IPV4));

    AddIpOptionField(node,
        msg,
        IPOPT_ODMRP,
        sizeof(OdmrpIpOptionType));

    option.seqNumber = OdmrpGetSeq(odmrp);
    option.protocol = originalProtocol;

    if (odmrp->pcEnable)
    {
        OdmrpUpdateExternalState(node, odmrp);

        switch (odmrp->externalState)
        {
            case ODMRP_INITIAL_NODE:
                if (odmrp->internalState == ODMRP_CH_READY)
                {
                    odmrp->externalState = ODMRP_CLUSTER_HEAD;
                    OdmrpCreatePassiveClsPacket(odmrp, node, msg);
                }
                else
                {
                    // Do Nothing
                }
                break;

            case ODMRP_ORDINARY_NODE:
            case ODMRP_CLUSTER_HEAD:
            case ODMRP_FULL_GW:
            case ODMRP_DIST_GW:
                OdmrpCreatePassiveClsPacket(odmrp, node, msg);
                break;

            default:
                ERROR_ReportError("Unknown External State of the node\n");
                break;
            }
    }

    OdmrpSetIpOptions(msg, &option);

    if (MESSAGE_ReturnPacketSize(msg) >
        GetNetworkIPFragUnit(node, (int)DEFAULT_INTERFACE))
    {
        sprintf(errString,"ODMRP doesn't support IP Fragmentation, "
            "increase MAX_NW_PKT_SIZE value and run again\n");

        ERROR_ReportError(errString);
        return;
    }
    if (ODMRP_DEBUG)
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("    sending at time %s\n", clockStr);
    }

    ipHeader = (IpHeaderType *)MESSAGE_ReturnPacket(msg);


    NetworkIpSendPacketToMacLayerWithDelay(node,
        msg,
        DEFAULT_INTERFACE,
        ANY_DEST,
        randDelay);

    OdmrpInsertMessageCache(odmrp,
        node,
        NetworkIpGetInterfaceAddress(node,
        GetDefaultInterfaceIndex(node, NETWORK_IPV4)),
        option.seqNumber,
        &odmrp->messageCache);
    odmrp->stats.numDataSent++;
    odmrp->stats.numDataTxed++;

} // End of OdmrpSendData Function

//------------------------------------------------------
// FUNCTION     OdmrpSendQuery
// PURPOSE      Broadcast Join Query message
//
// Parameters:
//     node:          Node that is sending the query packet
//      msg         : Pointer to message structure
//   mcastAddr      : Multicast Group
//  originalProtocol: The protocol field in IP packet
//------------------------------------------------------

static
void OdmrpSendQuery(OdmrpData* odmrp,
                    Node* node,
                    Message* msg,
                    NodeAddress mcastAddr,
                    unsigned int originalProtocol)
{
    IpHeaderType* ipHeader =  0;
    OdmrpIpOptionType odmrpIpOption;
    OdmrpJoinQuery option;
    char errString[MAX_STRING_LENGTH];
    NodeAddress  nodeIPAddress;

    clocktype randDelay = (clocktype)
        (RANDOM_erand(odmrp->broadcastJitterSeed) * ODMRP_BROADCAST_JITTER);
    //clocktype randDelay = 0;

    MESSAGE_SetLayer(msg, MAC_LAYER, 0);
    MESSAGE_SetEvent(msg, MSG_MAC_FromNetwork);

    nodeIPAddress = NetworkIpGetInterfaceAddress(node,
                               GetDefaultInterfaceIndex(node, NETWORK_IPV4));

    AddIpOptionField(node,
        msg,
        IPOPT_ODMRP,
        sizeof(OdmrpIpOptionType));

    odmrpIpOption.seqNumber = OdmrpGetSeq(odmrp);
    odmrpIpOption.protocol = originalProtocol;

    OdmrpSetIpOptions(msg, &odmrpIpOption);

    // Set variou field for Query
    option.pktType = (unsigned char)ODMRP_JOIN_QUERY;
    option.reserved = 0;
    option.ttlVal = (unsigned char)odmrp->ttlValue;
    option.hopCount = 1;
    option.mcastAddress = mcastAddr;
    option.seqNumber = OdmrpGetSeq(odmrp);
    option.srcAddr = nodeIPAddress;
    option.lastAddr = nodeIPAddress;
    option.protocol = originalProtocol;


    if (odmrp->pcEnable)
    {
        OdmrpCreatePassiveClsPacket(odmrp, node, msg);
    }

    // ODMRP add message header for join query
    MESSAGE_AddHeader(node, msg, sizeof(OdmrpJoinQuery),
        TRACE_ODMRP);

    memcpy(msg->packet, (char*)(&option), sizeof(OdmrpJoinQuery));

    // Add IP Header for Join Query control packet
    NetworkIpAddHeader(
        node,
        msg,
        nodeIPAddress,
        ANY_DEST,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_ODMRP,
        odmrp->ttlValue);



    if (ODMRP_DEBUG)
    {
        // check for Query Header
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        ipHeader = (IpHeaderType *)MESSAGE_ReturnPacket(msg);

        printf("Node %u: after adding query ip_src %x ip_dst %x \n",
            node->nodeId, ipHeader->ip_src, ipHeader->ip_dst);

        printf("    sending Query at time %s\n", clockStr);
    }

    if (MESSAGE_ReturnPacketSize(msg) >
        GetNetworkIPFragUnit(node, (int)DEFAULT_INTERFACE))
    {
        sprintf(errString,"ODMRP doesn't support IP Fragmentation, "
            "increase MAX_NW_PKT_SIZE value and run again\n");

        ERROR_ReportError(errString);
        return;
    }

    NetworkIpSendPacketToMacLayerWithDelay(node,
        msg,
        DEFAULT_INTERFACE,
        ANY_DEST,
        randDelay);

    odmrp->stats.numDataSent++;
    odmrp->stats.numDataTxed++;
    odmrp->stats.numQueryOriginated++;

    OdmrpInsertMessageCache(odmrp,
        node,
        NetworkIpGetInterfaceAddress(node,
        GetDefaultInterfaceIndex(node, NETWORK_IPV4)),
        option.seqNumber,
        &odmrp->messageCache);

    // Congestion prevention.
    if (OdmrpCheckSendQuery(mcastAddr, &odmrp->sentTable))
    {
        OdmrpUnsetSendQuery(mcastAddr, &odmrp->sentTable);
    }

    if (!OdmrpLookupSentTable(mcastAddr, &odmrp->sentTable))
    {
        // Mark that J-Q is sent. Just send Data from now on
        OdmrpSetSent(node, mcastAddr, odmrp);
    }
    else
    {
        OdmrpSetMinExpireTime(node, mcastAddr, odmrp);
    }
    OdmrpSetTimer(node,
        MSG_NETWORK_CheckTimeoutAlarm,
        &mcastAddr,
        sizeof(NodeAddress),
        ODMRP_TIMER_INTERVAL + randDelay);

}// End of OdmrpSendQuery Function

//------------------------------------------------------
// FUNCTION     OdmrpSendPeriodicQuery
// PURPOSE      Broadcast Periodic Join Query message
//
// Parameters:
//     node:          Node that is sending the query packet
//   mcastAddr      : Multicast Group
//  originalProtocol: The protocol field in IP packet
//------------------------------------------------------

static
void OdmrpSendPeriodicQuery(OdmrpData* odmrp,
                            Node* node,
                            NodeAddress mcastAddr,
                            unsigned int originalProtocol)
{
    Message* newMsg = 0;
    OdmrpJoinQuery option;
    NodeAddress  nodeIPAddress;

    clocktype randDelay = 0;

    newMsg = MESSAGE_Alloc(node,
                    MAC_LAYER,
                    0,
                    MSG_MAC_FromNetwork);

    nodeIPAddress = NetworkIpGetInterfaceAddress(node,
                                                 DEFAULT_INTERFACE);


    // Set various field for Query
    option.pktType = (unsigned char)ODMRP_JOIN_QUERY;
    option.reserved = 0;
    option.ttlVal = (unsigned char)odmrp->ttlValue;
    option.hopCount = 1;
    option.mcastAddress = mcastAddr;
    option.seqNumber = OdmrpGetSeq(odmrp);
    option.srcAddr = nodeIPAddress;
    option.lastAddr = nodeIPAddress;
    option.protocol = originalProtocol;

    // ODMRP add message header for join query
    MESSAGE_PacketAlloc(node, newMsg, sizeof(OdmrpJoinQuery),
        TRACE_ODMRP);

    memcpy(newMsg->packet, (char*)(&option), sizeof(OdmrpJoinQuery));

    // Add IP Header for Join Query control packet
    NetworkIpAddHeader(
        node,
        newMsg,
        nodeIPAddress,
        ANY_DEST,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_ODMRP,
        odmrp->ttlValue);

    if (ODMRP_DEBUG)
    {
        // check for Query Header
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);

        printf("Node %u: sending Join Query at time %s "
               "without piggybacking.\n", node->nodeId, clockStr);
    }

    NetworkIpSendPacketToMacLayerWithDelay(node,
        newMsg,
        DEFAULT_INTERFACE,
        ANY_DEST,
        randDelay);

    odmrp->stats.numQueryOriginated++;

    OdmrpInsertMessageCache(odmrp,
        node,
        NetworkIpGetInterfaceAddress(node,
        DEFAULT_INTERFACE),
        option.seqNumber,
        &odmrp->messageCache);

    // Congestion prevention.
    if (OdmrpCheckSendQuery(mcastAddr, &odmrp->sentTable))
    {
        OdmrpUnsetSendQuery(mcastAddr, &odmrp->sentTable);
    }

    if (!OdmrpLookupSentTable(mcastAddr, &odmrp->sentTable))
    {
        // Mark that J-Q is sent. Just send Data from now on
        OdmrpSetSent(node, mcastAddr, odmrp);
    }
    else
    {
        OdmrpSetMinExpireTime(node, mcastAddr, odmrp);
    }
    OdmrpSetTimer(node,
        MSG_NETWORK_CheckTimeoutAlarm,
        &mcastAddr,
        sizeof(NodeAddress),
        ODMRP_TIMER_INTERVAL + randDelay);

}// End of OdmrpSendPeriodicQuery Function


//----------------------------------------------------------
//
// FUNCTION     OdmrpHandelProtocolEvent
                    // PURPOSE      Handles ODMRP specific events.
                    //
                    // Parameters:
                    //    node: Node that is handling the ODMRP event.
//    msg:  Event that needs to be handled.
//
//---------------------------------------------------------

void OdmrpHandleProtocolEvent(Node* node,
                              Message* msg)
{
    char buf[MAX_STRING_LENGTH];

    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);

    switch (msg->eventType)
    {
        case MSG_NETWORK_CheckAcked:
            {
                // This timer event is installed when a node
                // originates a Join Reply,transnmits a Join Reply
                // retransmits a Join Reply.

                NodeAddress* mcastAddr =
                    (NodeAddress *)MESSAGE_ReturnInfo(msg);

                if (OdmrpCheckAckTable(*mcastAddr,
                    &odmrp->ackTable))
                {
                    if (ODMRP_DEBUG)
                    {
                        printf("Node %d not acked. Retx!\n", node->nodeId);
                    }

                    // Node is not acknowledged.
                    // Retransmit a Join Reply message once again

                    OdmrpRetxReply(odmrp,
                        node,
                        *mcastAddr,
                        &odmrp->ackTable);
                }
                break;
            }


        case MSG_NETWORK_CheckTimeoutAlarm:
            {
                // This timer event is installed when a node
                //sends periodic Join Query message to refresh
                //routing table information

                NodeAddress* mcastAddr =
                    (NodeAddress *) MESSAGE_ReturnInfo(msg);

                // Check I am the source of the multicast Group
                if (OdmrpLookupSentTable(*mcastAddr,
                    &odmrp->sentTable))
                {
                    if (OdmrpCheckMinExpTime(node,
                        *mcastAddr,
                        &odmrp->sentTable) &&
                        OdmrpCheckCongestionTime(node,
                        *mcastAddr,
                        &odmrp->memberTable) &&
                        !OdmrpCheckSendQuery(*mcastAddr,
                        &odmrp->sentTable))
                    {

                        // Next time when the source wants to
                        // send any data packet it will set
                        // IP option.query to TRUE in order enable other
                        // nodes to originate Join Reply message for the
                        // group.

                        OdmrpSetSendQuery(*mcastAddr,
                            &odmrp->sentTable);
                    }
                    else
                    {
                        OdmrpSetTimer(node,
                            MSG_NETWORK_CheckTimeoutAlarm,
                            mcastAddr,
                            sizeof(NodeAddress),
                            ODMRP_TIMER_INTERVAL);
                    }
                }

                break;
            }

        case MSG_NETWORK_FlushTables:
            {

                // This event is called at every ODMRP_FLUSH_INTERVAL
                // to delete message cache. The draft does not specify
                // the time Interval value. But message cache
                // is to be deleted according to FIFO or LRU

                OdmrpCacheDeleteEntry* msgDelEntry =
                    (OdmrpCacheDeleteEntry *) MESSAGE_ReturnInfo(msg);
                OdmrpDeleteMsgCache(odmrp,
                    &odmrp->messageCache,
                    msgDelEntry);
                break;
            }


        case MSG_NETWORK_SendReply:
            {

                // This event is called when a node wants to send
                // Join Reply message

                NodeAddress* mcastAddr =
                    (NodeAddress *)MESSAGE_ReturnInfo(msg);
                OdmrpSendReply(node,
                    *mcastAddr,
                    &odmrp->memberTable,
                    &odmrp->tempTable);

                break;
            }

            // Check if FG flag has been expired.
        case MSG_NETWORK_CheckFg:
            {

                // This event is called at every ODMRP_FG_TIMEOUT interval
                // The value of this interval is not specified in the draft

                NodeAddress* mcastAddr =
                    (NodeAddress *)MESSAGE_ReturnInfo(msg);

                if (OdmrpLookupFgFlag(*mcastAddr, &odmrp->fgFlag))
                {
                    // If FG expired, reset the flag.
                    if (OdmrpCheckFgExpired(node,
                        *mcastAddr,
                        odmrp))
                    {
                        OdmrpResetFgFlag(*mcastAddr,
                            &odmrp->fgFlag);
                    }
                    else
                    {

                        OdmrpSetTimer(node,
                            MSG_NETWORK_CheckFg,
                            mcastAddr,
                            sizeof(NodeAddress),
                            odmrp->expireTime);
                    }
                }
                break;
            }
        default:
            {
                sprintf(buf,
                    "Node %u received message of unknown\n"
                    "type %d.\n",
                    node->nodeId, msg->eventType);
                ERROR_Assert(FALSE, buf);
            }
    }

    MESSAGE_Free(node, msg);
} // End of OdmrpHandelProtocolEvent Function

//--------------------------------------------------------
// Following Common Functions
// 1. OdmrpDeleteClsHeadPair
// 1. OdmrpPopulateClsPair
// 2. OdmrpCheckClsEqual
// 3. OdmrpCalculateMemState
//--------------------------------------------------------

static
void OdmrpDeleteClsHeadPair(OdmrpClsHeadPair* headPair)
{
    OdmrpClsHeadPair*  tempHeadPair;
    while (headPair != NULL)
    {
        tempHeadPair = headPair;
        headPair = headPair->next;
        MEM_free(tempHeadPair);
    }
} //End of OdmrpDeleteClsHeadPair Function

//----------------------------------------------------------
//
// FUNCTION     OdmrpPopulateClsPair
// PURPOSE      Populate Cluster Haed Pair from Cluster Head
//              List
//
// Parameters:
// clsHeadList: Pointer to Cluster Head List
//
//---------------------------------------------------------

static
OdmrpClsHeadPair* OdmrpPopulateClsPair(OdmrpClsHeadList* clsHeadList)
{
    OdmrpHeadList* preceding = 0;
    OdmrpHeadList* following = 0;
    OdmrpClsHeadPair*  clsPrevHeadPair = NULL;
    OdmrpClsHeadPair*  toFree;
    OdmrpClsHeadPair*  newOnePair = 0;
    OdmrpClsHeadPair*  currentHeadPair = 0;
    OdmrpClsHeadPair*  headPair = 0;

    headPair = (OdmrpClsHeadPair *)
        MEM_malloc(sizeof(OdmrpClsHeadPair));
    currentHeadPair = headPair;

    for (preceding = clsHeadList->head;
    preceding != NULL;
    preceding = preceding->next)
    {
        for (following = preceding->next;
        following != NULL;
        following = following->next)
        {
            currentHeadPair->clsId1 = preceding->nodeId;
            currentHeadPair->clsId2 = following->nodeId;
            newOnePair = (OdmrpClsHeadPair *)
                MEM_malloc(sizeof(OdmrpClsHeadPair));
            currentHeadPair->next = newOnePair;
            clsPrevHeadPair = currentHeadPair;
            currentHeadPair = currentHeadPair->next;
        }
    }

    toFree = clsPrevHeadPair->next;
    clsPrevHeadPair->next = NULL;
    MEM_free(toFree);
    return headPair;
} // End of OdmrpPopulateClsPair Function

//--------------------------------------------------
// FUNCTION     OdmrpCheckClsEqual
// PURPOSE      Check the equality of two pairs of
//              of Cluster Heads. If they are equal
//              return TRUE, FALSE otherwise
//
// Paremeters:
//    clsId1   : Node Id of Cluster Head
//    clsId2   : Node Id of Cluster Head
//    clsId3   : Node Id of Cluster Head
//    clsId4   : Node Id of Cluster Head
//-------------------------------------------------

static
BOOL OdmrpCheckClsEqual(NodeAddress clsId1,
                        NodeAddress clsId2,
                        NodeAddress clsId3,
                        NodeAddress clsId4)
{
    if (clsId1 == clsId3)
    {
        if (clsId2 == clsId4)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else if (clsId1 == clsId4)
    {
        if (clsId2 == clsId3)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
} // End of OdmrpCheckClsEqual Function

//--------------------------------------------------
// FUNCTION     OdmrpCalculateMemState
// PURPOSE      This function is called when a node
//              receives a packet. An Ordinary node
//              may change to GW_READY while a node
//              with GW_READY state changes to ORDINARY
//              node.
//
// Paremeters:
//
//-------------------------------------------------

static
void OdmrpCalculateMemState(Node* node,
                            OdmrpData* odmrp)
{
    OdmrpClsHeadList*   clsHeadList = &(odmrp->clsHeadList);
    OdmrpClsGwList*     clsGwList = &(odmrp->clsGwList);
    OdmrpClsDistGwList* clsDistGwList = &(odmrp->clsDistGwList);
    OdmrpGwList*        currentGwList;
    OdmrpDistGwList*    currentDistGwList;
    BOOL                gatewayFound = FALSE;
    OdmrpHeadList*      clsList = 0;
    OdmrpClsHeadPair*   clsHeadPair = 0;
    OdmrpClsHeadPair*   currentPair = 0;

    if (odmrp->externalState != ODMRP_CLUSTER_HEAD )
    {
        if ((odmrp->externalState != ODMRP_FULL_GW)
            && (odmrp->externalState != ODMRP_DIST_GW))
        {
            if (ODMRP_PASSIVE_DEBUG)
            {
                printf("-----------------------------------\n");
                printf("The Node id %d\n",node->nodeId);
                printf("The External State of the node %d\n",
                    odmrp->externalState);
                printf("The Internal State of the node %d\n",
                    odmrp->internalState);
                printf("------------------------------------\n");
            }
            for (currentDistGwList = clsDistGwList->head;
            currentDistGwList != NULL;
            currentDistGwList = currentDistGwList->next)
            {
                gatewayFound = FALSE;
                for (currentGwList = clsGwList->head;
                currentGwList != NULL;
                currentGwList = currentGwList->next)
                {
                    if (currentGwList->nodeId ==
                        currentDistGwList->nodeId)
                    {
                        continue;
                    }
                    else
                    {
                        if ((currentGwList->state == ODMRP_DIST_GW)
                            && (currentGwList->clsId2 ==
                            currentDistGwList->clsId1))
                        {
                            gatewayFound = TRUE;
                            break;
                        }
                        else if ((currentGwList->state == ODMRP_FULL_GW)
                            && ((currentGwList->clsId1
                            == currentDistGwList->clsId1)
                            || (currentGwList->clsId2
                            == currentDistGwList->clsId1)))
                        {
                            gatewayFound = TRUE;
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
                if (gatewayFound == FALSE)
                {
                    odmrp->internalState
                        = (unsigned char) ODMRP_GW_READY;
                    if (ODMRP_PASSIVE_DEBUG)
                    {
                        printf("-----------------------------------\n");
                        printf("The Node id %d\n",node->nodeId);
                        printf("The External State of the node %d\n",
                            odmrp->externalState);
                        printf("The Internal State of the node %d\n",
                            odmrp->internalState);
                        printf("------------------------------------\n");
                    }

                    return;
                }
            }

            if (clsHeadList->size >= 2)
            {
                clsHeadPair = OdmrpPopulateClsPair(clsHeadList);

                if (ODMRP_PASSIVE_DEBUG)
                {
                    printf("////////////////////////////////\n");
                    printf("Node Id %d\n",node->nodeId);
                    OdmrpPrintHead(clsHeadList);
                }
                if (ODMRP_PASSIVE_DEBUG)
                {
                    printf("////////////////////////////////\n");
                    OdmrpPrintClsHeadPair(clsHeadPair);
                    printf("////////////////////////////////\n");
                }

                for (currentPair = clsHeadPair;
                currentPair != NULL;
                currentPair = currentPair->next)
                {
                    gatewayFound = FALSE;
                    for (currentGwList = clsGwList->head;
                    currentGwList != NULL;
                    currentGwList = currentGwList->next)
                    {

                        if (currentGwList->state != ODMRP_DIST_GW)
                        {
                            continue;
                        }
                        else
                        {
                            if (OdmrpCheckClsEqual(currentGwList->clsId1,
                                currentGwList->clsId2,
                                currentPair->clsId1,
                                currentPair->clsId2))
                            {
                                gatewayFound = TRUE;
                                break;
                            }
                            else
                            {
                                continue;
                            }
                        }
                    }

                    if (gatewayFound == FALSE)
                    {
                        odmrp->internalState
                            = (unsigned char)ODMRP_GW_READY;
                        if (ODMRP_PASSIVE_DEBUG)
                        {
                            printf("-----------------------------------\n");
                            printf("The Node id %d\n",node->nodeId);
                            printf("The External State of the node %d\n",
                                odmrp->externalState);
                            printf("The Internal State of the node %d\n",
                                odmrp->internalState);
                            printf("------------------------------------\n");
                        }
                        OdmrpDeleteClsHeadPair(clsHeadPair);
                        return;
                    }

                } //End  of For each loop
                OdmrpDeleteClsHeadPair(clsHeadPair);
            }
            else if (clsHeadList->size == 1)
            {
                if (clsGwList->size >= 3)
                {
                    odmrp->externalState
                        = (unsigned char) ODMRP_ORDINARY_NODE;

                    return;
                }
                else if (clsGwList->size  == 2)
                {
                    clsList = clsHeadList->head;
                    for (currentGwList = clsGwList->head;
                    currentGwList != NULL;
                    currentGwList = currentGwList->next)
                    {
                        if ((currentGwList->state != ODMRP_DIST_GW)
                            || (currentGwList->clsId1 != clsList->nodeId))
                        {
                            odmrp->internalState =
                                (unsigned char)ODMRP_GW_READY;
                            return;
                        }
                        else
                        {
                            continue;
                        }
                    }
                    odmrp->externalState = (unsigned char)ODMRP_ORDINARY_NODE;
                    if (ODMRP_PASSIVE_DEBUG)
                    {
                        printf("-----------------------------------\n");
                        printf("The Node id %d\n",node->nodeId);
                        printf("The External State of the node %d\n",
                            odmrp->externalState);
                        printf("The Internal State of the node %d\n",
                            odmrp->internalState);
                        printf("------------------------------------\n");
                    }
                    return;
                }
                else
                {
                    odmrp->internalState = (unsigned char)ODMRP_GW_READY;
                    if (ODMRP_PASSIVE_DEBUG)
                    {
                        printf("-----------------------------------\n");
                        printf("The Node id %d\n",node->nodeId);
                        printf("The External State of the node %d\n",
                            odmrp->externalState);
                        printf("The Internal State of the node %d\n",
                            odmrp->internalState);
                        printf("------------------------------------\n");
                    }
                    return;
                }
            }
            else
            {
                // Do Nothing
            }

            if (ODMRP_PASSIVE_DEBUG)
            {   printf("\n after updation \n");
            printf("-----------------------------------\n");
            printf("The Node id %d\n",node->nodeId);
            printf("The External State of the node %d\n",
                odmrp->externalState);
            printf("The Internal State of the node %d\n",
                odmrp->internalState);
            printf("------------------------------------\n");
            }
       }
    }
 } // End of OdmrpCalculateMemState Function

 //----------------------------------------------
 // FUNCTION     OdmrpDeleteDistGwList
 // PURPOSE      Update Gateway Related Information
 // Paremeters:
 //----------------------------------------------

 static
     void OdmrpDeleteDistGwList(OdmrpData* odmrp,
     Node* node)
 {
     OdmrpDistGwList* toFree = 0;
     OdmrpDistGwList* current = 0;
     OdmrpDistGwList* temp = 0;
     OdmrpClsDistGwList* clsDistGwList = &odmrp->clsDistGwList;

     if (clsDistGwList->size == 0 || clsDistGwList->head == NULL)
     {
         return;
     }
     temp = clsDistGwList->head;
     current = clsDistGwList->head;

     while (current != NULL) {
         if (node->getNodeTime() - current->lastRecvTime >
             odmrp->clsExpireTime)
         {
             toFree = current;
             if (ODMRP_PASSIVE_DEBUG)
             {   printf("--------------------------------\n");
             OdmrpPrintDistGateway(clsDistGwList);
             printf("--------------------------------\n");
             }
             // The first node exists only
             if (clsDistGwList->size == 1)
             {
                 clsDistGwList->head = NULL;
                 MEM_free(toFree);
                 --(clsDistGwList->size);
                 return;
             }
             else if (temp == current)
             {
                 clsDistGwList->head = temp->next;
                 temp = temp->next;
             }
             else if (current->next == NULL)
             {
                 temp->next = NULL;
             }
             else
             {
                 temp->next = current->next;
             }
             current = current->next;
             MEM_free(toFree);
             --(clsDistGwList->size);
         }
         else
         {
             temp = current;
             current = current->next;
         }
     }
 }// End of OdmrpDeleteDistGwList Function

 //----------------------------------------------
 // FUNCTION     OdmrpDeleteClusterHead
 // PURPOSE      Delete Cluster Head from Cluster Head List
 // Paremeters:
 //     odmrp
 // clsHeadList:  Pointer to the list of Cluster Head
 //       nodeId: Node Id of the Packet
 //------------------------------------------------

 static
     void  OdmrpDeleteClusterHead(OdmrpData* odmrp,
     Node* node,
     OdmrpClsHeadList* clsHeadList)
 {
     OdmrpHeadList* toFree = 0;
     OdmrpHeadList* current = 0;
     OdmrpHeadList* temp = 0;

     if (clsHeadList->size == 0 || clsHeadList->head == NULL)
     {
         return;
     }

     temp = clsHeadList->head;
     current = clsHeadList->head;

     while (current != NULL)
     {
         if (node->getNodeTime() - current->lastRecvTime >
             odmrp->clsExpireTime)
         {
             toFree = current;

             // The first node exists only
             if (clsHeadList->size == 1)
             {
                 clsHeadList->head = NULL;
                 MEM_free(toFree);
                 --(clsHeadList->size);
                 return;

             }
             else if (temp == current)
             {
                 clsHeadList->head = temp->next;
                 temp = temp->next;
             }
             else if (current->next == NULL)
             {
                 temp->next = NULL;
             }
             else
             {
                 temp->next = current->next;
             }
             current = current->next;
             MEM_free(toFree);
             --(clsHeadList->size);
         }
         else
         {
             temp = current;
             current = current->next;
         }
     }
 } // End of OdmrpDeleteClusterHead Function

 //----------------------------------------------
 // FUNCTION     OdmrpClusterDeleteAndUpdate
 // PURPOSE      Update Cluster Head List
 // Paremeters:
 //------------------------------------------------

 static
     void  OdmrpClusterDeleteAndUpdate(Node* node,
     OdmrpData* odmrp)
 {
     OdmrpHeadList* toFree = 0;
     OdmrpHeadList* current = 0;
     OdmrpHeadList* temp = 0;
     OdmrpGwClsEntry*  clsGwEntry = 0;

     OdmrpClsHeadList* clsHeadList = &(odmrp->clsHeadList);
     clsGwEntry  = &(odmrp->gwClsEntry);

     if (clsHeadList->size == 0 || clsHeadList->head == NULL)
     {
         return;
     }

     temp = clsHeadList->head;
     current = clsHeadList->head;

     while (current != NULL) {
         if (node->getNodeTime() - current->lastRecvTime >
             odmrp->clsExpireTime)
         {
             if ((current->nodeId == clsGwEntry->clsId1)
                 || (current->nodeId == clsGwEntry->clsId2))
             {
                 odmrp->externalState = (unsigned char)ODMRP_ORDINARY_NODE;
                 odmrp->internalState  = (unsigned char)ODMRP_GW_READY;
             }
             toFree = current;

             // The first node exists only
             if (clsHeadList->size == 1)
             {
                 clsHeadList->head = NULL;
                 MEM_free(toFree);
                 --(clsHeadList->size);
                 return;

             }
             else if (temp == current)
             {
                 clsHeadList->head = temp->next;
                 temp = temp->next;
             }
             else if (current->next == NULL)
             {
                 temp->next = NULL;
             }
             else
             {
                 temp->next = current->next;
             }
             current = current->next;
             MEM_free(toFree);
             --(clsHeadList->size);
         }
         else
         {
             temp = current;
             current = current->next;
         }
     }
 } // End of  OdmrpClusterDeleteAndUpdate Function

 //----------------------------------------------
 // FUNCTION     OdmrpDeleteGatewayList
 // PURPOSE      Update Gateway Related Information
 // Paremeters:
 //----------------------------------------------

 static
     void OdmrpDeleteGatewayList(OdmrpData* odmrp,
     Node* node)
 {
     OdmrpGwList* toFree = 0;
     OdmrpGwList* current = 0;
     OdmrpGwList* temp = 0;
     OdmrpClsGwList* clsGwList = &odmrp->clsGwList;

     if (clsGwList->size == 0 || clsGwList->head == NULL)
     {
         return;
     }

     temp = clsGwList->head;
     current = clsGwList->head;

     while (current != NULL) {
         if (node->getNodeTime() - current->lastRecvTime >
             odmrp->clsExpireTime)
         {
             toFree = current;
             // The first node exists only
             if (ODMRP_PASSIVE_DEBUG)
             {
                 printf("-----------------------------\n");
                 OdmrpPrintGateway(clsGwList);
                 printf("-----------------------------\n");
             }
             if (clsGwList->size == 1)
             {
                 clsGwList->head = NULL;
                 MEM_free(toFree);
                 --(clsGwList->size);
                 return;
             }
             else if (temp == current)
             {
                 clsGwList->head = temp->next;
                 temp = temp->next;
             }
             else if (current->next == NULL)
             {
                 temp->next = NULL;
             }
             else
             {
                 temp->next = current->next;
             }
             current = current->next;
             MEM_free(toFree);
             --(clsGwList->size);
         }
         else
         {
             temp = current;
             current = current->next;
         }
     }
 } // End of OdmrpDeleteGatewayList Function

 //---------------------------------------------
 //  Two Check functions.
 //  1. OdmrpCheckGateway
 //  2. OdmrpCheckClsHead
 //
 //---------------------------------------------

 //----------------------------------------------
 // FUNCTION     OdmrpCheckGateway
 // PURPOSE      Update Gateway Related Information
 // Paremeters:
 // clsGwList:  Pointer to the list of Cluster Gateway List
 //----------------------------------------------

 static
     void OdmrpCheckGateway(Node* node,
     OdmrpData* odmrp)
 {
     OdmrpDeleteGatewayList(odmrp,
         node);
     OdmrpDeleteDistGwList(odmrp,
         node);
     OdmrpCalculateMemState(node,
         odmrp);
 } // Endof OdmrpCheckGateway Function

 //----------------------------------------------
 // FUNCTION     OdmrpFindClsHead
 // PURPOSE      Search Cluster Head List Return TRUE
 //              if found FALSE otherwise
 // Paremeters:
 // clsGwList:  Pointer to the list of Cluster Gateway List
 //----------------------------------------------

 static
     BOOL OdmrpFindClsHead(NodeAddress nodeId,
     OdmrpClsHeadList* clsHeadList)
 {
     OdmrpHeadList* currentHeadList = 0;

     for (currentHeadList = clsHeadList->head;
     currentHeadList != NULL;
     currentHeadList = currentHeadList->next)
     {
         if (currentHeadList->nodeId == nodeId)
         {
             return TRUE;
         }
     }
     return FALSE;
 } // End of OdmrpFindClsHead Function

 //----------------------------------------------
 // FUNCTION     OdmrpCheckClsHead
 // PURPOSE      Check and Update Cluster Head Related
 //              Information
 // Paremeters:
 //----------------------------------------------

 static
     void OdmrpCheckClsHead(Node* node,
     OdmrpData* odmrp)
 {
     OdmrpClsHeadList* clsHeadList;
     clsHeadList = &odmrp->clsHeadList;

     if ((odmrp->externalState == ODMRP_FULL_GW)
         || (odmrp->externalState == ODMRP_DIST_GW))
     {
         OdmrpClusterDeleteAndUpdate(node,
             odmrp);
     }
     else
     {
         OdmrpDeleteClusterHead(odmrp,
             node,
             clsHeadList);
     }

     if (odmrp->externalState != ODMRP_CLUSTER_HEAD)
     {
         if (clsHeadList->size == 0)
         {

             odmrp->externalState = (unsigned char) ODMRP_INITIAL_NODE;

         }
     }
 }// End of OdmrpCheckClsHead Function


 //-----------------------------------------------------
 // Three Addition Functions.
 // 1. OdmrpAddClsHeadList()
 // 2. OdmrpAddDistGwList()
 // 3. OdmrpAddGwList()
 //------------------------------------------------------

 //--------------------------------------------------
 // FUNCTION     OdmrpAddClsHeadList
 // PURPOSE      Add a new Cluster Head in the list
 //              of Cluster Heads. Cluster Head Information
 //              is obtained from the received Cluster Info
 //              Packet.
 // Paremeters:
 //     node:  Node handling the data packet.
 //     odmrp : Pointer to Odmrp Specific Data
 //
 //--------------------------------------------------

 static
     void OdmrpAddClsHeadList(Node*  node,
     OdmrpClsHeadList *clsHeadList,
     NodeAddress nodeId)


 {
     OdmrpHeadList*     current = 0;
     OdmrpHeadList*     prevNode = 0;
     OdmrpHeadList*     newOne = 0;
     BOOL               isFound = FALSE;

     if (clsHeadList->head == NULL)
     {
         newOne = (OdmrpHeadList *)MEM_malloc(sizeof(OdmrpHeadList));
         ERROR_Assert(newOne != NULL,
             "newOne is not equal to NULL");
         newOne->nodeId = nodeId;
         newOne->lastRecvTime =  node->getNodeTime();
         newOne->next = NULL;
         clsHeadList->head = newOne;
         clsHeadList->size = 1;
     }
     else
     {
         current = clsHeadList->head;
         while (current != NULL)
         {
             if (current->nodeId == nodeId)
             {
                 current->lastRecvTime =  node->getNodeTime();
                 isFound = TRUE;
                 break;
             }
             else
             {
                 prevNode = current;
                 current = current->next;
             }
         }
         if (isFound == FALSE)
         {
             newOne = (OdmrpHeadList *)MEM_malloc(sizeof(OdmrpHeadList));
             ERROR_Assert(newOne != NULL,
                 "newOne is not equal to NULL");
             newOne->nodeId = nodeId;
             newOne->lastRecvTime =  node->getNodeTime();
             newOne->next = NULL;
             prevNode->next = newOne;
             clsHeadList->size++;
         }
     }
 } // End of OdmrpAddClsHeadList Function

 //--------------------------------------------------
 // FUNCTION     OdmrpAddDistGwList
 // PURPOSE      Add a new Dist gateway in the list
 //              of Dist Gateways. Gateway info is obtained
 //              from the received cluster Info packet.
 // Paremeters:
 //     node:  Node handling the data packet.
 //     odmrp : Pointer to Odmrp Specific Data
 //--------------------------------------------------

 static
     void OdmrpAddDistGwList(Node *node,
     OdmrpClsDistGwList* clsDistGwList,
     OdmrpPassiveClsPkt* gatewayInfoPkt)
 {
     OdmrpDistGwList* newOne;
     OdmrpDistGwList* current = 0;
     OdmrpDistGwList* prevNode = 0;
     BOOL   isFound;

     if (clsDistGwList->head == NULL)
     {
         newOne = (OdmrpDistGwList *)
             MEM_malloc(sizeof(OdmrpDistGwList));
         ERROR_Assert(newOne != NULL,
             "Unable to allocate memory\n");

         newOne->nodeId = gatewayInfoPkt->nodeId;
         newOne->clsId1 = gatewayInfoPkt->clsId1;
         newOne->clsId2 = gatewayInfoPkt->clsId2;
         newOne->lastRecvTime =  node->getNodeTime();
         newOne->next = NULL;
         clsDistGwList->head = newOne;
         clsDistGwList->size = 1;
     }
     else
     {
         current = clsDistGwList->head;
         prevNode = current;
         isFound = FALSE;
         while (current != NULL)
         {
             if ((current->nodeId == gatewayInfoPkt->nodeId)
                 && (OdmrpCheckClsEqual(current->clsId1,
                 current->clsId2,
                 gatewayInfoPkt->clsId1,
                 gatewayInfoPkt->clsId2)))

             {
                 current->lastRecvTime =  node->getNodeTime();
                 isFound = TRUE;
                 break;
             }
             else
             {
                 prevNode = current;
                 current = current->next;
             }
         }

         if (isFound == FALSE)
         {
             newOne = (OdmrpDistGwList *)
                 MEM_malloc(sizeof(OdmrpDistGwList));
             ERROR_Assert(newOne != NULL,
                 "Unable to Allocate memory\n");

             newOne->nodeId = gatewayInfoPkt->nodeId;
             newOne->clsId1 = gatewayInfoPkt->clsId1;
             newOne->clsId2 = gatewayInfoPkt->clsId2;
             newOne->lastRecvTime =  node->getNodeTime();
             newOne->next = NULL;

             prevNode->next = newOne;
             clsDistGwList->size++;
         }
     }
 } // End of OdmrpAddDistGwList Function

 //-----------------------------------------------------
 // FUNCTION     OdmrpAddGwList
 // PURPOSE      Add a new gateway in the list
 //              of gateways. Gateway info is obtained
 //              from the received cluster Info packet.
 // Paremeters:
 //     node:  Node handling the data packet.
 //     odmrp : Pointer to Odmrp Specific Data
 //------------------------------------------------------

 static
     void  OdmrpAddGwList(Node* node,
     OdmrpClsGwList* clsGwList,
     OdmrpPassiveClsPkt* gatewayInfoPkt)

 {
     OdmrpGwList* newOne;
     OdmrpGwList* current = 0;
     OdmrpGwList* prevNode = 0;
     BOOL         isFound = FALSE;

     if (clsGwList->head == NULL)
     {
         newOne = (OdmrpGwList *)
             MEM_malloc(sizeof(OdmrpGwList));

         ERROR_Assert(newOne != NULL,
             "Unable to allocate memory\n");
         newOne->nodeId = gatewayInfoPkt->nodeId;
         newOne->state = OdmrpPassiveClsPktGetState(gatewayInfoPkt->opcp);
         newOne->clsId1 = gatewayInfoPkt->clsId1;
         newOne->clsId2 = gatewayInfoPkt->clsId2;
         newOne->lastRecvTime =  node->getNodeTime();
         newOne->next = NULL;
         clsGwList->head = newOne;
         clsGwList->size = 1;
     }
     else
     {
         current = clsGwList->head;
         prevNode = current;
         while (current != NULL)
         {
             if ((current->nodeId == gatewayInfoPkt->nodeId)
                 && (current->state ==
                 OdmrpPassiveClsPktGetState(gatewayInfoPkt->opcp))
                 && (OdmrpCheckClsEqual(current->clsId1,
                 current->clsId2,
                 gatewayInfoPkt->clsId1,
                 gatewayInfoPkt->clsId2)))

             {
                 current->lastRecvTime =  node->getNodeTime();
                 isFound = TRUE;
                 break;
             }
             else
             {
                 prevNode = current;
                 current = current->next;

             }
         }

         if (isFound == FALSE)
         {
             newOne = (OdmrpGwList *)
                 MEM_malloc(sizeof(OdmrpGwList));

             ERROR_Assert(newOne != NULL,
                 "Unable to Allocate memory\n");

             newOne->nodeId = gatewayInfoPkt->nodeId;
             newOne->state = OdmrpPassiveClsPktGetState(
                 gatewayInfoPkt->opcp);
             newOne->clsId1 = gatewayInfoPkt->clsId1;
             newOne->clsId2 = gatewayInfoPkt->clsId2;
             newOne->lastRecvTime =  node->getNodeTime();
             newOne->next = NULL;
             prevNode->next = newOne;
             clsGwList->size++;
         }
     }
 } // End of OdmrpAddGwList Function

 //----------------------------------------------------
 //  Update External State of a node
 //  Following Functions are being used
 //  1. OdmrpUpdateStateTwoClsHeads
 //  2. OdmrpUpdateStateSingleClsHead
 //  3. OdmrpUpdateExternalState
 //-----------------------------------------------------

 //-----------------------------------------------------
 // FUNCTION     OdmrpUpdateStateTwoClsHeads
 // PURPOSE      If a node knows more than two Cluster Heads,
 //              this function updates its external state
 // Paremeters:
 //     node:  Node handling the data packet.
 //     odmrp : Pointer to Odmrp Specific Data
 //-----------------------------------------------------

 static
     void OdmrpUpdateStateTwoClsHeads(Node* node,
     OdmrpData* odmrp)
 {
     OdmrpHeadList*      headList = NULL;
     OdmrpClsHeadList*   clsHeadList = &(odmrp->clsHeadList);
     OdmrpClsGwList*     clsGwList  = &(odmrp->clsGwList);
     OdmrpGwList*        currentGwList = NULL;
     OdmrpGwClsEntry*    gwClsEntry = &(odmrp->gwClsEntry);
     OdmrpClsDistGwList* clsDistGwList = &(odmrp->clsDistGwList);
     OdmrpDistGwList*    currentDistGwList = NULL;
     OdmrpClsHeadPair*   clsHeadPair = NULL;
     OdmrpClsHeadPair*   currentPair = NULL;
     BOOL                gatewayFound;

     clsHeadPair = OdmrpPopulateClsPair(clsHeadList);

     for (currentPair = clsHeadPair;
     currentPair != NULL;
     currentPair = currentPair->next)
     {
         gatewayFound = FALSE;

         for (currentGwList = clsGwList->head;
         currentGwList != NULL;
         currentGwList = currentGwList->next)
         {
             if (currentGwList->state == ODMRP_DIST_GW)
             {
                 continue;
             }
             if (OdmrpCheckClsEqual(currentGwList->clsId1,
                 currentGwList->clsId2,
                 currentPair->clsId1,
                 currentPair->clsId2))
             {
                 gatewayFound = TRUE;
                 break;
             }
             else
             {
                 continue;
             }
         }

         if (gatewayFound == FALSE)
         {
             odmrp->externalState = ODMRP_FULL_GW;
             gwClsEntry->clsId1 = currentPair->clsId1;
             gwClsEntry->clsId2 = currentPair->clsId2;

             OdmrpDeleteClsHeadPair(clsHeadPair);
             return;
         }
         else
         {
             continue;
         }
     }

     OdmrpDeleteClsHeadPair(clsHeadPair);

     for (currentDistGwList = clsDistGwList->head;
     currentDistGwList != NULL;
     currentDistGwList = currentDistGwList->next)
     {
         gatewayFound = FALSE;
         if (OdmrpFindClsHead(currentDistGwList->clsId1,
             clsHeadList))
         {
             continue;
         }
         else
         {
             for (currentGwList = clsGwList->head;
             currentGwList != NULL;
             currentGwList = currentGwList->next)
             {
                 if (currentDistGwList->nodeId == currentGwList->nodeId)
                 {
                     continue;
                 }
                 if ((currentGwList->state == ODMRP_DIST_GW)
                     && (currentGwList->clsId2
                     == currentDistGwList->clsId2))
                 {
                     gatewayFound = TRUE;
                     break;
                 }
                 else if ((currentGwList->state == ODMRP_FULL_GW)
                     && ((currentGwList->clsId1
                     == currentDistGwList->clsId1)
                     ||
                     (currentGwList->clsId1
                     == currentDistGwList->clsId2)))

                 {
                     gatewayFound = TRUE;
                     break;
                 }
                 else
                 {
                     continue;
                 }
             }
             if (gatewayFound == FALSE)
             {
                 odmrp->externalState = (unsigned char)ODMRP_DIST_GW;
                 headList = clsHeadList->head;
                 gwClsEntry->clsId1 = headList->nodeId;
                 gwClsEntry->clsId2 = currentDistGwList->clsId1;

                 if (ODMRP_PASSIVE_DEBUG)
                 {
                     printf("////////////////////////////////////////\n");
                     printf("1st Cluster Head %d\n",gwClsEntry->clsId1);
                     printf("2nd Cluster Head %d\n",gwClsEntry->clsId2);
                     printf("/////////////////////////////////////////\n");
                 }
                 return;
             }
             else
             {
                 continue;
             }
         }
     }

     if (odmrp->internalState == ODMRP_GW_READY)
     {
         odmrp->externalState = (unsigned char)ODMRP_ORDINARY_NODE;
     }
} // End of OdmrpUpdateStateTwoClsHeads Function

//---------------------------------------------------------
// FUNCTION     OdmrpUpdateStateSingleClsHead
// PURPOSE      If a node knows only one Cluster Head,this
//              function updates its external state
// Paremeters:
//     node:  Node handling the data packet.
//     odmrp : Pointer to Odmrp Specific Data
//----------------------------------------------------------

static
void OdmrpUpdateStateSingleClsHead(Node* node,
                                   OdmrpData* odmrp)
{
    OdmrpClsDistGwList* clsDistGwList = 0;
    OdmrpDistGwList*    currentDistGwList = 0;
    OdmrpClsGwList*     clsGwList = 0;
    OdmrpGwList*        currentGwList = 0;
    OdmrpGwClsEntry*    gwClsEntry = 0;
    BOOL                gatewayFound;
    OdmrpClsHeadList*   clsHeadList = 0;
    OdmrpHeadList*      clsHeadEntry = 0;

    clsDistGwList = &(odmrp->clsDistGwList);
    clsHeadList   = &(odmrp->clsHeadList);
    clsGwList     = &(odmrp->clsGwList);
    gwClsEntry    = &(odmrp->gwClsEntry);
    clsHeadEntry = clsHeadList->head;

    for (currentDistGwList = clsDistGwList->head;
    currentDistGwList != NULL;
    currentDistGwList = currentDistGwList->next)
    {
        gatewayFound = FALSE;
        if (currentDistGwList->clsId1 == clsHeadEntry->nodeId)
        {
            continue;
        }
        else
        {
            for (currentGwList = clsGwList->head;
            currentGwList != NULL;
            currentGwList = currentGwList->next)
            {
                if ((currentGwList->state == ODMRP_DIST_GW)
                    && (currentGwList->clsId1 == clsHeadEntry->nodeId)
                    && (currentGwList->clsId2 == currentDistGwList->clsId1))
                {
                    gatewayFound = TRUE;
                    break;
                }
                else if ((currentGwList->state == ODMRP_FULL_GW)
                    && (OdmrpCheckClsEqual(currentGwList->clsId1,
                    currentGwList->clsId2,
                    clsHeadEntry->nodeId,
                    currentDistGwList->clsId1)))

                {
                    gatewayFound = TRUE;
                    break;
                }

            }
            if (gatewayFound == FALSE)
            {
                odmrp->externalState = (unsigned char)ODMRP_DIST_GW;
                gwClsEntry->clsId1 = clsHeadEntry->nodeId;
                gwClsEntry->clsId2 = currentDistGwList->clsId1;
                return;
            }
        }
    }

    if (odmrp->internalState == ODMRP_GW_READY)
    {
        for (currentDistGwList = clsDistGwList->head;
        currentDistGwList != NULL;
        currentDistGwList = currentDistGwList->next)
        {
            if (currentDistGwList->clsId1 == clsHeadEntry->nodeId)
            {
                odmrp->externalState = (unsigned char)ODMRP_ORDINARY_NODE;
                return;
            }
            else
            {
                continue;
            }
        }
    }

    if (odmrp->internalState == ODMRP_GW_READY)
    {
        odmrp->externalState = (unsigned char)ODMRP_DIST_GW;
        gwClsEntry->clsId1 = clsHeadEntry->nodeId;
        gwClsEntry->clsId2 =  ODMRP_UNKNOWN_ID;
    }
} // End of OdmrpUpdateStateSingleClsHead Function

//----------------------------------------------
// FUNCTION     OdmrpUpdateExternalState
// PURPOSE      The Processing of Outgoing Packets
//              for a member node.
// Paremeters:
//     node:  Node handling the data packet.
//     odmrp : Pointer to Odmrp Specific Data
//------------------------------------------------------

static
void OdmrpUpdateExternalState(Node* node,
                              OdmrpData* odmrp)
{

    OdmrpClsHeadList*   clsHeadList = &odmrp->clsHeadList;


    if (odmrp->externalState != ODMRP_CLUSTER_HEAD)
    {


        if (odmrp->internalState == ODMRP_GW_READY)
        {
            if (clsHeadList->size >= 2)
            {
                OdmrpUpdateStateTwoClsHeads(node,
                    odmrp);

            }
            else if (clsHeadList->size == 1)
            {
                OdmrpUpdateStateSingleClsHead(node,
                    odmrp);
            }
            else
            {
                // Do Nothing
            }
        }
        else
        {
            // Do Nothing
        }
    }
} // End of OdmrpUpdateExternalState Function

//------------------------------------------------------
//  Addition function
//  1. OdmrpAddGateway
//  2. OdmrpAddClusterHead
//  3. OdmrpAddInitial
//------------------------------------------------------

//------------------------------------------------------
// FUNCTION     OdmrpAddGateway
// PURPOSE      Update Gateway list after receiving
//              Cluster Information present
//              in Data Packet.
// Paremeters:
//     odmrp
//     node:  Node handling the data packet
//     msg:   The data packet.
//------------------------------------------------------

static
void OdmrpAddGateway(Node* node,
                     OdmrpData* odmrp,
                     OdmrpPassiveClsPkt* gatewayInfoPkt)

{
    OdmrpGwList*  current = NULL;
    BOOL isFound = FALSE;
    OdmrpClsGwList* clsGwList = &odmrp->clsGwList;
    OdmrpGwClsEntry* gwClsEntry = &odmrp->gwClsEntry;

    if (OdmrpPassiveClsPktGetState(gatewayInfoPkt->opcp) == ODMRP_DIST_GW)
    {
        if (ODMRP_PASSIVE_DEBUG)
        {
            printf("/////////////////////////////////\n");
            printf("Node Id %d\n",node->nodeId);
            printf("Before Dist Gateway Addition\n");
            OdmrpPrintDistGateway(&odmrp->clsDistGwList);
            printf("///////////////////////////////////\n");
        }
        if (ODMRP_PASSIVE_DEBUG)
        {
            printf("////////////////////////////////////\n");
            printf("Dist Gateway Node %d\n",gatewayInfoPkt->nodeId);
            printf("Dist Gateway Node %d\n",gatewayInfoPkt->clsId1);
            printf("Dist Gateway Node %d\n",gatewayInfoPkt->clsId2);
            printf("////////////////////////////////////\n");

        }
        OdmrpAddDistGwList(node,
            &odmrp->clsDistGwList,
            gatewayInfoPkt);
        if (ODMRP_PASSIVE_DEBUG)
        {
            printf("/////////////////////////////////\n");
            printf("Node Id %d\n",node->nodeId);
            printf("After Dist Gateway Addition\n");
            OdmrpPrintDistGateway(&odmrp->clsDistGwList);
            printf("//////////////////////////////////\n");
        }

        OdmrpDeleteDistGwList(odmrp, node);
        if (ODMRP_PASSIVE_DEBUG)
        {
            printf("//////////////////////////////////\n");
            printf("Node Id %d\n",node->nodeId);
            printf("After Dist Gateway Deletion\n");
            OdmrpPrintDistGateway(&odmrp->clsDistGwList);
            printf("//////////////////////////////////\n");
        }
    }

    OdmrpAddGwList(node,
        clsGwList,
        gatewayInfoPkt);

    OdmrpDeleteGatewayList(odmrp,
        node);

    if (odmrp->externalState  == ODMRP_FULL_GW)
    {
        if ((OdmrpCheckClsEqual(gatewayInfoPkt->clsId1,
            gatewayInfoPkt->clsId2,
            gwClsEntry->clsId1,
            gwClsEntry->clsId2))
            && (gatewayInfoPkt->nodeId < node->nodeId))
        {
            odmrp->externalState = (unsigned char)ODMRP_ORDINARY_NODE;
        }
        else
        {
            current = clsGwList->head;
            isFound = FALSE;
            while (current == NULL)
            {
                if (OdmrpCheckClsEqual(current->clsId1,
                    current->clsId2,
                    gatewayInfoPkt->clsId1,
                    gatewayInfoPkt->clsId2))
                {
                    odmrp->internalState = (unsigned char)ODMRP_GW_READY;
                    isFound = TRUE;
                    break;
                }
                else
                {
                    current = current->next;
                    continue;
                }
            }

            if (isFound == FALSE)
            {
                gwClsEntry->clsId1 = gatewayInfoPkt->clsId1;
                gwClsEntry->clsId2 = gatewayInfoPkt->clsId2;
            }
        }
    }

    OdmrpCheckClsHead(node, odmrp);
    OdmrpCalculateMemState(node, odmrp);
                     }// End of OdmrpAddGateway Function

//----------------------------------------------
// FUNCTION     OdmrpAddClusterHead
// PURPOSE      Add Cluster Head Related Information
// Paremeters:
//     odmrp
// clsHeadList:  Pointer to the list of Cluster Head
//       nodeId: Node Id of the Packet
//------------------------------------------------------

static
void OdmrpAddClusterHead(Node* node,
                         OdmrpData *odmrp,
                         NodeAddress nodeId)
{
    OdmrpClsHeadList*  clsHeadList;
    int                storeSize;

    clsHeadList = &(odmrp->clsHeadList);
    storeSize = clsHeadList->size;

    if (ODMRP_PASSIVE_DEBUG)
    {
        printf("---------------------------------------\n");
        printf(" Before Addition\n");
        printf("No of Cluster Heads %d\n",clsHeadList->size);
        OdmrpPrintHead(clsHeadList);
        printf("----------------------------------\n");

    }

    OdmrpAddClsHeadList(node,
        clsHeadList,
        nodeId);

    if (ODMRP_PASSIVE_DEBUG)
    {
        printf("-------------------------------------\n");
        printf("After Addition Before Deletion\n");
        printf("No of Cluster Heads %d\n",clsHeadList->size);
        OdmrpPrintHead(clsHeadList);
        printf("----------------------------------\n");
    }

    OdmrpDeleteClusterHead(odmrp,
        node,
        clsHeadList);

    if (ODMRP_PASSIVE_DEBUG)
    {
        printf("---------------------------------------\n");
        printf("After Deletion\n");
        printf("No of Cluster Heads %d\n",clsHeadList->size);
        OdmrpPrintHead(clsHeadList);
        printf("----------------------------------\n");
    }

    if ((odmrp->externalState == ODMRP_DIST_GW)
        && (clsHeadList->size > storeSize))
    {
        odmrp->internalState = (unsigned char)ODMRP_GW_READY;
    }
    OdmrpCheckGateway(node,
        odmrp);

} // End of OdmrpAddClusterHead Function

//----------------------------------------------
// FUNCTION     OdmrpAddInitial
// PURPOSE      Upddate Initail Node specific Information
// Paremeters:
//     node    :
//     odmrp   :  Pointer to the list of Cluster Head
//       nodeId: Node Idof the Packet
//------------------------------------------------------

static
void OdmrpAddInitial(Node* node,
                     OdmrpData *odmrp)
{
    // There is no need to insert Initial node information
    OdmrpCheckClsHead(node,
        odmrp);
    OdmrpCheckGateway(node,
        odmrp);
} // End of OdmrpAddInitial Function




//-----------------------------------------------------
// FUNCTION     OdmrpCreateGiveUpPacket
// PURPOSE      Create Give Up Message Packet with necessary sate information
// Paremeters:
// odmrp          : Data associated with node
// node           : Node handling the data packet.
// msg            : The data packet.
//------------------------------------------------------


static
void OdmrpCreateGiveUpPacket(OdmrpData* odmrp,
                             Node* node,
                             Message* msg)
{
    IpOptionsHeaderType* ipOption =
        FindAnIpOptionField((IpHeaderType*)msg->packet,
        IPOPT_ODMRP_CLUSTER);

    OdmrpPassiveClsPkt   passiveClsPkt;

    if (ipOption == NULL)
    {
        AddIpOptionField(node,
            msg,
            IPOPT_ODMRP_CLUSTER,
            sizeof(OdmrpPassiveClsPkt));
    }
    ipOption =
        FindAnIpOptionField((IpHeaderType*)msg->packet,
        IPOPT_ODMRP_CLUSTER);

    OdmrpPassiveClsPktSetState(&(passiveClsPkt.opcp), odmrp->externalState);
    OdmrpPassiveClsPktSetGiveUpFlag(&(passiveClsPkt. opcp), TRUE);//note it
    OdmrpPassiveClsPktSetHelloFlag(&(passiveClsPkt.opcp), FALSE);
    OdmrpPassiveClsPktSetReserved(&(passiveClsPkt.opcp), 0);
    passiveClsPkt.nodeId = node->nodeId;

    memcpy(((char*)ipOption + sizeof(IpOptionsHeaderType)),
        &passiveClsPkt,
        sizeof(OdmrpPassiveClsPkt));


} // end of odmrpCreateGiveUpPacket


//-----------------------------------------------------
// FUNCTION     OdmrpSendGiveUpMessage
// PURPOSE      Send Give Up message
// Paremeters:
// odmrp          : Data associated with node
// node           : Node handling the data packet.
// msg            : The data packet.
// mcastAddr      : Multicast address
//originalProtocol: Original Protocol
//------------------------------------------------------

static
void OdmrpSendGiveUpMessage(OdmrpData* odmrp,
                            Node* node,
                            Message* msg,
                            NodeAddress mcastAddr,
                            unsigned int originalProtocol)
{

    OdmrpIpOptionType option;
    IpHeaderType* ipHeader =  0;
    NodeAddress  nodeIPAddress;
    char errString[MAX_STRING_LENGTH];
    clocktype randDelay = (clocktype)
        (RANDOM_erand(odmrp->broadcastJitterSeed) * ODMRP_BROADCAST_JITTER);
    //clocktype randDelay = 0;
    MESSAGE_SetLayer(msg, MAC_LAYER, 0);
    MESSAGE_SetEvent(msg, MSG_MAC_FromNetwork);

    nodeIPAddress = NetworkIpGetInterfaceAddress(node,
        GetDefaultInterfaceIndex(node, NETWORK_IPV4));


    option.seqNumber = OdmrpGetSeq(odmrp);
    option.protocol = originalProtocol;

    OdmrpCreateGiveUpPacket(odmrp,
        node,
        msg);

    OdmrpSetIpOptions(msg, &option);

    if (MESSAGE_ReturnPacketSize(msg) >
        GetNetworkIPFragUnit(node, (int)DEFAULT_INTERFACE))
    {
        sprintf(errString,"ODMRP doesn't support IP Fragmentation, "
            "increase MAX_NW_PKT_SIZE value and run again\n");

        ERROR_ReportError(errString);
        return;
    }

    if (ODMRP_DEBUG)
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf(" Node = %d sending GiveUp time %s\n", node->nodeId,clockStr);
    }

    ipHeader = (IpHeaderType *)MESSAGE_ReturnPacket(msg);

    ipHeader->ip_dst = mcastAddr;

    NetworkIpSendPacketToMacLayerWithDelay(node,
        msg,
        DEFAULT_INTERFACE,
        ANY_DEST,
        randDelay);


} // End of OdmrpGiveUpMessage

//-----------------------------------------------------
// FUNCTION     OdmrpActivatePassiveClustering
// PURPOSE      Activate Passive clustering after receiving
//              Cluster Information present in Data Packet.
// Paremeters:
//   odmrp:
//    node:  Node handling the data packet.
//     msg:  The data packet.
//------------------------------------------------------

static
void OdmrpActivatePassiveClustering(OdmrpData* odmrp,
                                    Node* node,
                                    const Message*  msg)
{
    IpOptionsHeaderType*   ipOption = NULL;
    unsigned char          pktType;
    OdmrpPassiveClsPkt     passiveClsPkt;
    ipOption = FindAnIpOptionField((IpHeaderType*)msg->packet,
        IPOPT_ODMRP_CLUSTER);

    if (ipOption == NULL)
    {
        // No Passive Cluster Related IP Options are present
        // So we stop the processing of the packet

        return;
    }

    memcpy(&passiveClsPkt,
        ((char*)ipOption + sizeof(IpOptionsHeaderType)),
        sizeof(OdmrpPassiveClsPkt));
    pktType = OdmrpPassiveClsPktGetState(passiveClsPkt.opcp);

    if (pktType == ODMRP_CLUSTER_HEAD)
    {

        if ((odmrp->externalState == ODMRP_CLUSTER_HEAD)
            && (node->nodeId > passiveClsPkt.nodeId))
        {
            // Node should send Cluster Head Give up message
            // But since the message is optional we ignore it
            odmrp->externalState = (unsigned char)ODMRP_ORDINARY_NODE;
            // send giveUp message

            IpHeaderType* ipHeader = (IpHeaderType *) msg->packet;
            unsigned int originalProtocol = (unsigned int) ipHeader->ip_p;

            NodeAddress mcastAddr = ipHeader->ip_dst;
            Message* newMsg = NULL ;

            newMsg = MESSAGE_Duplicate(node, msg);

            OdmrpSendGiveUpMessage(
                odmrp,
                node,
                newMsg,
                mcastAddr,
                originalProtocol);
            // increament no of giveUp message sent
            odmrp->stats.numGiveUpTx++ ;
        }
        else if ((odmrp->externalState == ODMRP_CLUSTER_HEAD)
            && (node->nodeId < passiveClsPkt.nodeId))
        {
            // Since My Node-Id is small,so we should remain as
            // Cluster Head and so we ignore the message

            return;
        }

        // Adding Cluster Head Related Information
        OdmrpAddClusterHead(node,
            odmrp,
            passiveClsPkt.nodeId);
    }
    else if ((pktType == ODMRP_FULL_GW)
        || (pktType == ODMRP_DIST_GW))
    {
        OdmrpAddGateway(node,
            odmrp,
            &passiveClsPkt);
    }
    else if (pktType == ODMRP_INITIAL_NODE)
    {
        OdmrpAddInitial(node, odmrp);
    }
    else // that is for ORDINARY_NODE
    {
        if (OdmrpPassiveClsPktGetGiveUpFlag(passiveClsPkt.opcp) == TRUE)
        {
            if (ODMRP_PASSIVE_DEBUG)
            {
                printf("\nNode = %d got giveup message\n",node->nodeId);
            }

            // increment num of GiveUp message received
            odmrp->stats.numGiveUpRx++ ;

            OdmrpGiveUpClusterHead(odmrp,
                node,
                passiveClsPkt.nodeId);
            // Free the Message
            //MESSAGE_Free(node, msg);
            // and return
            return;
        }
    }

    if ((odmrp->externalState == ODMRP_INITIAL_NODE)
        && (pktType != ODMRP_CLUSTER_HEAD))
    {
        if (ODMRP_PASSIVE_DEBUG)
        {
            printf("/////////////////////////////////\n");
            printf("Node id is %d\n",node->nodeId);
            printf("Internal State of the node %d\n",
                odmrp->internalState);
        }

        odmrp->internalState = (unsigned char)ODMRP_CH_READY;

        if (ODMRP_PASSIVE_DEBUG)
        {
            printf("Internal State of the node %d\n",
                odmrp->internalState);
            printf("/////////////////////////////////\n");
        }
    }

}  // End of OdmrpActivatePassiveClustering Function


//-----------------------------------------------------------
// FUNCTION     OdmrpHandleJoinQuery
//  PURPOSE      Processing procedure when Data Join is received.
//
// Paremeters:
//    node:  Node handling the data join packet.
//    msg :   The data join packet.
//----------------------------------------------------------

static
void OdmrpHandleJoinQuery(Node* node,
                          Message* msg)
{
    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);

    NodeAddress                 sourceAddress;
    NodeAddress                 destinationAddress;
    unsigned char               IpProtocol;
    unsigned int                ttl = 0;
    TosType priority;
    clocktype                   delay;
    clocktype                   jrDelay;
    OdmrpMtNode*                mcastEntry = 0;
    OdmrpRptNode*               mEntry = 0;
    NodeAddress srcAddr ;
    NodeAddress mcastAddr;
    OdmrpJoinQuery option ;
    OdmrpJoinQuery tmpOption ;
    Message* newMsg             = NULL;
    int optionTtl;

    // Store Query related information
    memcpy(&option, msg->packet, sizeof(OdmrpJoinQuery));
    memcpy(&tmpOption, &option, sizeof(OdmrpJoinQuery));
    mcastAddr = option.mcastAddress;

    int packetSize = MESSAGE_ReturnPacketSize(msg);
    if (packetSize > (int) sizeof(OdmrpJoinQuery))
    {
        // Remove Query Header
        MESSAGE_RemoveHeader(node, msg, sizeof(OdmrpJoinQuery),
                              TRACE_ODMRP);
    }

    //ipHdr = (IpHeaderType *)MESSAGE_ReturnPacket(msg);
    //srcAddr = ipHdr->ip_src;
    srcAddr = option.srcAddr;

    if (ODMRP_DEBUG)
    {
        printf("Node %u: after removing odmrp Query Header of seq %d \n",
            node->nodeId, option.seqNumber);
        printf(" source address %x \n", option.srcAddr);
        //printf(" ip_src %x ip_dst %x \n",
        //    ipHdr->ip_src, ipHdr->ip_dst);
    }

    //if (odmrp->pcEnable)
    if (packetSize > (int)sizeof(OdmrpJoinQuery) && odmrp->pcEnable)
    {
        OdmrpActivatePassiveClustering(
            odmrp,
            node,
            msg);
    }

    // Process packet only if not duplicate.
    if (!OdmrpLookupMessageCache(srcAddr,
        option.seqNumber,
        &odmrp->messageCache))
    {

        if (ODMRP_DEBUG)
        {
            char addressStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(option.lastAddr, addressStr);
            printf("Node %d received Join Query from %s\n",
                node->nodeId, addressStr);
        }

        OdmrpInsertMessageCache(odmrp,
            node,
            srcAddr,
            option.seqNumber,
            &odmrp->messageCache);

        OdmrpInsertRouteTable(node,
            srcAddr,
            option.lastAddr,
            &odmrp->routeTable);

        // If the node is a member of the group
        if (OdmrpLookupMembership(mcastAddr,
            &odmrp->memberFlag))
        {
            if (ODMRP_DEBUG)
            {
                printf("        Member got it!\n");
            }

            if (packetSize > (int) sizeof(OdmrpJoinQuery))
            {
                newMsg = MESSAGE_Duplicate(node, msg);
                NetworkIpRemoveIpHeader(node,
                    newMsg,
                    &sourceAddress,
                    &destinationAddress,
                    &priority,
                    &IpProtocol,
                    &ttl);

                switch (option.protocol)
                {
                    case IPPROTO_UDP:
                        {

                            if (ODMRP_DEBUG)
                            {
                                printf(" Node %u: \t", node->nodeId);
                                printf(" Send to UDP: from odmrp \n");
                            }


                            SendToUdp(node,
                                newMsg,
                                priority,
                                sourceAddress,
                                destinationAddress,
                                DEFAULT_INTERFACE);

                            break;
                        }
                    default:
                        {
                            // Control does not come here
                            break;
                        }
                }

                odmrp->stats.numDataReceived++;
            }

            //
            // Insert the source member to the table and send
            // a Join Reply.

            mcastEntry = OdmrpGetMTEntry(mcastAddr,
                &odmrp->memberTable);
            if (mcastEntry == NULL)
            {
                OdmrpInsertMemberTable(node,
                    mcastAddr,
                    srcAddr,
                    &odmrp->memberTable);
            }
            else
            {
                OdmrpInsertMemberSource(node,
                    srcAddr,
                    mcastEntry);
            }
            OdmrpCheckSourceExpired(node,
                mcastAddr,
                &odmrp->memberTable);

            mEntry = OdmrpGetRPTEntry(mcastAddr,
                &odmrp->responseTable);
            if (mEntry == NULL)
            {
                OdmrpInsertResponseTable(mcastAddr,
                    srcAddr,
                    &odmrp->responseTable);
            }
            else
            {
                OdmrpInsertResponseSource(srcAddr,
                    mEntry);
            }

            jrDelay = (clocktype) (RANDOM_erand(odmrp->jrJitterSeed) * ODMRP_JR_JITTER);

            OdmrpSetTimer(node,
                MSG_NETWORK_SendReply,
                &mcastAddr,
                sizeof(NodeAddress),
                jrDelay);
        } // If multicast member

        memcpy(&option, &tmpOption, sizeof(OdmrpJoinQuery));


        optionTtl= option.ttlVal;

        // Relay the Query since ttl value is greater than zero
        if (optionTtl>= 0)
        {

            if (odmrp->pcEnable)
            {
                OdmrpUpdateExternalState(node, odmrp);
                if (packetSize > (int) sizeof(OdmrpJoinQuery))
                {
                    switch (odmrp->externalState)
                    {
                        case ODMRP_INITIAL_NODE:
                            if (odmrp->internalState == ODMRP_CH_READY)
                            {
                                odmrp->externalState = ODMRP_CLUSTER_HEAD;
                                OdmrpCreatePassiveClsPacket(odmrp,
                                    node,
                                    msg);
                            }
                            else
                            {
                                // Do Nothing
                            }
                            break;

                        case ODMRP_ORDINARY_NODE:
                            // Since the nodeis an ordinary node so it should
                            // not forward packets.So we call message free.
                            MESSAGE_Free(node, msg);
                            return;

                        case ODMRP_CLUSTER_HEAD:
                        case ODMRP_FULL_GW:
                        case ODMRP_DIST_GW:
                            OdmrpCreatePassiveClsPacket(odmrp, node, msg);
                            break;

                        default:
                            ERROR_ReportError(
                                "Unknown External State of the node\n");
                            break;
                    }
                }
                else
                {
                    switch (odmrp->externalState)
                    {
                        case ODMRP_ORDINARY_NODE:
                            // Since the nodeis an ordinary node so it should
                            // not forward packets.So we call message free.
                            MESSAGE_Free(node, msg);
                            return;
                    }
                }
            }

            memcpy(&option, &tmpOption, sizeof(OdmrpJoinQuery));

            option.lastAddr =
                NetworkIpGetInterfaceAddress(node,
                GetDefaultInterfaceIndex(node, NETWORK_IPV4));

            option.hopCount++;


            option.ttlVal = option.ttlVal - 1;

            if (packetSize > (int) sizeof(OdmrpJoinQuery))
            {
                // ODMRP add message header for join query
                MESSAGE_AddHeader(node, msg, sizeof(OdmrpJoinQuery),
                    TRACE_ODMRP);

                memcpy(msg->packet,(char*)(&option),sizeof(OdmrpJoinQuery));
            }
            else
            {
                MESSAGE_Free(node, msg);

                //generating new message
                newMsg = MESSAGE_Alloc(node,
                                        MAC_LAYER,
                                        0,
                                        MSG_MAC_FromNetwork);
                // ODMRP add message header for join query
                MESSAGE_PacketAlloc(node, newMsg, sizeof(OdmrpJoinQuery),
                                        TRACE_ODMRP);
                memcpy(newMsg->packet, (char*)(&option),
                                        sizeof(OdmrpJoinQuery));
            }
            delay = (clocktype)
                (RANDOM_erand(odmrp->broadcastJitterSeed) * ODMRP_BROADCAST_JITTER);
            //delay = 0;

            if (packetSize > (int) sizeof(OdmrpJoinQuery))
            {
                // Add IP Header for Join Query control packet
                NetworkIpAddHeader(
                    node,
                    msg,
                    option.srcAddr,
                    ANY_DEST,
                    IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_ODMRP,
                    option.ttlVal);

                if (ODMRP_DEBUG)
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime() + delay, clockStr);
                    printf("    fwding Query at time %s\n", clockStr);
                }
#ifdef ADDON_DB
                // only added when we are sending packets. 
                // not forwarding ...
                HandleNetworkDBEvents(
                    node,
                    msg,
                    DEFAULT_INTERFACE,                    
                    "NetworkReceiveFromUpper",
                    "",
                    option.srcAddr,
                    ANY_DEST,
                    IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_ODMRP);
#endif
                NetworkIpSendPacketToMacLayerWithDelay(node,
                    msg,
                    DEFAULT_INTERFACE,
                    ANY_DEST,
                    delay);
            }
            else
            {
                // Add IP Header for Join Query control packet
                NetworkIpAddHeader(
                    node,
                    newMsg,
                    option.srcAddr,
                    ANY_DEST,
                    IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_ODMRP,
                    option.ttlVal);

                if (ODMRP_DEBUG)
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime() + delay, clockStr);
                    printf("    fwding Query at time %s\n", clockStr);
                }

                NetworkIpSendPacketToMacLayerWithDelay(node,
                    newMsg,
                    DEFAULT_INTERFACE,
                    ANY_DEST,
                    delay);
            }

            //Update Join Query and Data Transmission statistics
            odmrp->stats.numDataTxed++;
            odmrp->stats.numQueryTxed++;

            if (ODMRP_DEBUG)
            {
                printf("    Relaying it\n");
            }
        } // If TTL  is less than zero
        else
        {
            MESSAGE_Free(node, msg);
        }
    } // If not duplicate
    else
    {
        if (ODMRP_DEBUG)
        {
            printf("It is a duplicate packet: so leave it \n");
        }

        MESSAGE_Free(node, msg);
    }
} // End of OdmrpHandleJoinQuery Function


//----------------------------------------------------------------
// FUNCTION     OdmrpCheckIfItIsDuplicatePacket
// PURPOSE      Check if the packet is duplicated or not.
//
// Paremeters:
//     node:  Node handling the data packet.
//     msg:   The data packet.
//---------------------------------------------------------------

BOOL
OdmrpCheckIfItIsDuplicatePacket(Node* node, Message* msg)
{
    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);

    IpHeaderType* ipHdr = (IpHeaderType *)MESSAGE_ReturnPacket(msg);
    OdmrpIpOptionType option = OdmrpGetIpOptions(msg);

    if (OdmrpLookupMessageCache(ipHdr->ip_src,
        option.seqNumber,
        &odmrp->messageCache))
    {
        return TRUE;
    }

    return FALSE;
}


//----------------------------------------------------------------
// FUNCTION     OdmrpHandleData
// PURPOSE      Processing procedure when Data and Join Query
//              is received.
//
// Paremeters:
//     node:  Node handling the data packet.
//     msg:   The data packet.
//---------------------------------------------------------------

static
void OdmrpHandleData(Node* node, Message* msg)
{
    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);
    IpHeaderType* ipHdr = (IpHeaderType *)MESSAGE_ReturnPacket(msg);
    NodeAddress                 sourceAddress;
    NodeAddress                 destinationAddress;
    unsigned char               IpProtocol;
    unsigned int                ttl = 0;
    TosType priority;
    clocktype                   delay;
    NodeAddress srcAddr   = ipHdr->ip_src;
    NodeAddress mcastAddr = ipHdr->ip_dst;
    OdmrpIpOptionType option = OdmrpGetIpOptions(msg);
    Message* newMsg = NULL;

    if (odmrp->pcEnable)
    {
        OdmrpActivatePassiveClustering(
            odmrp,
            node,
            msg);
    }

    if (!OdmrpLookupMessageCache(srcAddr,
        option.seqNumber,
        &odmrp->messageCache))
    {

        OdmrpInsertMessageCache(odmrp,
            node,
            srcAddr,
            option.seqNumber,
            &odmrp->messageCache);

        // Check if the node is the member of the group
        if (OdmrpLookupMembership(mcastAddr,
            &odmrp->memberFlag))
        {
            if (ODMRP_DEBUG)
            {
                printf("Node %d received DATA\n", node->nodeId);
            }

            if (ODMRP_DEBUG)
            {
                printf("    Member got it!\n");
            }
            odmrp->stats.numDataReceived++;

            newMsg = MESSAGE_Duplicate(node, msg);

            NetworkIpRemoveIpHeader(node,
                newMsg,
                &sourceAddress,
                &destinationAddress,
                &priority,
                &IpProtocol,
                &ttl);
            MESSAGE_Free(node, newMsg) ;
            switch (option.protocol)
            {
                case IPPROTO_UDP:
                    {
                        //Sending the packet to the upper layer

                        if (ODMRP_DEBUG)
                        {
                            printf(" Node %u: \t", node->nodeId);
                            printf(" Sending to UDP from ODMRP \n");
                        }

                        break;
                    }
                default:
                    {
                        // Control does not come here
                        break;
                    }
            }

        }

        if (odmrp->pcEnable)
        {

            OdmrpUpdateExternalState(node, odmrp);

            switch (odmrp->externalState)
            {
                case ODMRP_INITIAL_NODE:
                    if (odmrp->internalState == ODMRP_CH_READY)
                    {
                        odmrp->externalState = ODMRP_CLUSTER_HEAD;
                        OdmrpCreatePassiveClsPacket(odmrp,
                            node,
                            msg);

                    }
                    else
                    {
                        // Do Nothing
                    }
                    break;
                case ODMRP_ORDINARY_NODE:
                    // Since the nodeis an ordinary node so it should
                    // not forward packets.So we call message free.

                    MESSAGE_Free(node, msg);
                    return;
                case ODMRP_CLUSTER_HEAD:
                case ODMRP_FULL_GW:
                case ODMRP_DIST_GW:
                    OdmrpCreatePassiveClsPacket(odmrp,
                        node,
                        msg);
                    break;
                default:
                    ERROR_ReportError("Unknown External State of the node\n");
                    break;
           }

            // Set the Forward flag for the group
            OdmrpSetFgFlag(node,
                mcastAddr,
                &odmrp->fgFlag);

        }

        // If the node is FG, forward the packet.
        if (OdmrpLookupFgFlag(mcastAddr, &odmrp->fgFlag))
        {



            if (ODMRP_DEBUG)
            {
                printf("Node %d received DATA\n", node->nodeId);
            }
            if (ODMRP_DEBUG)
            {
                printf("    FG. Forwarding it\n");
            }


            delay = (clocktype)
                (RANDOM_erand(odmrp->broadcastJitterSeed) * ODMRP_BROADCAST_JITTER);
            //delay = 0;
            if (ODMRP_DEBUG)
            {
                char clockStr[100];
                ctoa(node->getNodeTime() + delay, clockStr);
                printf("    sending at time %s\n", clockStr);
            }

            NetworkIpSendPacketToMacLayerWithDelay(node,
                msg,
                DEFAULT_INTERFACE,
                ANY_DEST,
                delay);

            odmrp->stats.numDataTxed++;
        }
        else
        {
            MESSAGE_Free(node, msg);
        }
    } // if not duplicate
    else
    {
        MESSAGE_Free(node, msg);
    }
} // End of OdmrpHandleData Function

//---------------------------------------------------------------
// FUNCTION     OdmrpHandleReply
// PURPOSE      Processing procedure when Join Table is received.
//
// Paremeters:
//     node: Node handling the packet.
//      msg:  The join reply packet.
//  lastAddr: I do not know why is it passing
// mcastAddr: Multicast Group
//
//----------------------------------------------------------------

static
void OdmrpHandleReply(Node* node,
                      Message* msg,
                      NodeAddress lastAddr)
{
    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);
    Message*          newMsg = 0;
    OdmrpAck*        ackPkt = 0;
    NodeAddress       nodeIPAddress;
    NodeAddress       mcastAddr;
    OdmrpJoinTupleForm*  tupleForm = 0;
    OdmrpJoinReply*  replyPkt = 0;
    OdmrpTtNode*    mcastEntry = 0;
    OdmrpRptNode*   mEntry = 0;
    clocktype         delay;
    int               i;
    char*             pktPtr = 0;
    int pktSize = sizeof(OdmrpAck);
    BOOL changed = FALSE;

    nodeIPAddress = NetworkIpGetInterfaceAddress(node,
        GetDefaultInterfaceIndex(node, NETWORK_IPV4));

    // trace recd pkt
    ActionData acnData;
    acnData.actionType = RECV;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_NETWORK_LAYER, PACKET_IN, &acnData);

    pktPtr = (char *) MESSAGE_ReturnPacket(msg);
    replyPkt = (OdmrpJoinReply *) pktPtr;
    mcastAddr = replyPkt->multicastGroupIPAddr;
    pktPtr = pktPtr + sizeof(OdmrpJoinReply);

    tupleForm = (OdmrpJoinTupleForm *) pktPtr;

    for (i = 0; i < (signed) replyPkt->replyCount; i++)
    {

        // Source only gives an Acknowledgement message
        if ((tupleForm[i].srcAddr ==  nodeIPAddress) &&
            (tupleForm[i].nextAddr ==  nodeIPAddress))
        {

            //Draft does not tell anything about Acknowledge packet
            // We design Acknowledgement of our own

            newMsg = MESSAGE_Alloc(node,
                MAC_LAYER,
                0,
                MSG_MAC_FromNetwork);
            MESSAGE_PacketAlloc(node,
                newMsg,
                pktSize,
                TRACE_ODMRP);

            pktPtr = (char *) MESSAGE_ReturnPacket(newMsg);
            ackPkt = (OdmrpAck *) pktPtr;
            ackPkt->pktType = (unsigned char)ODMRP_ACK;
            ackPkt->mcastAddr = mcastAddr;
            ackPkt->srcAddr = tupleForm[i].srcAddr;

            if (ODMRP_DEBUG)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("    sending at time %s\n", clockStr);
            }

            //Trace sending pkt
            ActionData acnData;
            acnData.actionType = SEND;
            acnData.actionComment = NO_COMMENT;
            TRACE_PrintTrace(node,
                             newMsg,
                             TRACE_NETWORK_LAYER,
                             PACKET_OUT,
                             &acnData);

            // Send Acknowledgement packet to an unicast address
            NetworkIpSendRawMessageToMacLayer(node,
                newMsg,
                nodeIPAddress,
                lastAddr,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_ODMRP,
                1,
                DEFAULT_INTERFACE,
                lastAddr);

            odmrp->stats.numAckSent++;

            if (ODMRP_DEBUG)
            {
                char addressStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(lastAddr, addressStr);
                printf("Node %d received a Join Reply from node %s\n",
                    node->nodeId, addressStr);
            }

            if (ODMRP_DEBUG)
            {
                char addressStr[MAX_STRING_LENGTH];
                char addressStr1[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(lastAddr, addressStr);
                printf("    sending Exp Ack to node %s\n", addressStr);
                IO_ConvertIpAddressToString(ackPkt->mcastAddr, addressStr);
                IO_ConvertIpAddressToString(ackPkt->srcAddr, addressStr1);
                printf("    mcast = %s, src = %s\n",
                    addressStr, addressStr1);
            }
        }

        // Update the ack table entries if the table comes from a FG.
        if (OdmrpJoinReplyGetIAmFG(replyPkt->ojr))
        {
            OdmrpDeleteAckTable(mcastAddr,
                tupleForm[i].srcAddr,
                lastAddr,
                &odmrp->ackTable);
        }

        // If the node is a forwarding group.
        if ((tupleForm[i].nextAddr == nodeIPAddress) &&
            (tupleForm[i].srcAddr != nodeIPAddress))
        {

            if (ODMRP_DEBUG)
            {
                char addressStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(lastAddr, addressStr);
                printf("Node %d received a Join Reply from node %s\n",
                    node->nodeId, addressStr);
            }

            if (ODMRP_DEBUG)
            {
                printf("    I'm a FG!\n");
            }
            // Check whether Forward flag is on
            if (OdmrpLookupFgFlag(mcastAddr, &odmrp->fgFlag))
            {
                // Update the Forward Flag for this multicast group
                OdmrpUpdateFgFlag(node,
                    mcastAddr,
                    &odmrp->fgFlag);
            }
            else
            {
                // Set the Forward flag for the group
                OdmrpSetFgFlag(node,
                    mcastAddr,
                    &odmrp->fgFlag);
            }

            // Populate the temp table from the Join Reply
            // message.It is required to send Join Reply
            // message

            mcastEntry = OdmrpGetTTEntry(mcastAddr,
                &odmrp->tempTable);
            if (mcastEntry == NULL)
            {
                // Populate the temp table with multicast group
                // and corresponding source
                OdmrpInsertTempTable(odmrp,
                    node,
                    mcastAddr,
                    tupleForm[i].srcAddr,
                    &odmrp->tempTable);
            }
            else
            {
                //Populate the temp table with source only
                OdmrpInsertTempSource(odmrp,
                    node,
                    tupleForm[i].srcAddr,
                    mcastEntry);
            }

            // Populate the response table from the Join
            // Reply message.Reponse Table is required

            mEntry = OdmrpGetRPTEntry(mcastAddr,
                &odmrp->responseTable);
            if (mEntry == NULL)
            {
                OdmrpInsertResponseTable(mcastAddr,
                    tupleForm[i].srcAddr,
                    &odmrp->responseTable);
            }
            else
            {
                OdmrpInsertResponseSource(tupleForm[i].srcAddr,
                    mEntry);
            }
            changed = TRUE;
            OdmrpCheckTempExpired(node,
                mcastAddr,
                &odmrp->tempTable);
        }  // I am the Forwarding Group
    } // End for for processing Join Reply message

    // If table content has changed and i'm not collecting anymore.
    if (changed
        && !OdmrpCheckTempSent(mcastAddr,
        &odmrp->tempTable))
    {
        if (ODMRP_DEBUG)
        {
            printf("  Changed and temp sent!\n");
        }

        // Start collecting.
        OdmrpSetTempSent(mcastAddr, &odmrp->tempTable);
        delay = (clocktype) (RANDOM_erand(odmrp->jrJitterSeed) * ODMRP_JR_JITTER +
            ODMRP_JR_PAUSE_TIME);
        OdmrpSetTimer(node,
            MSG_NETWORK_SendReply,
            &mcastAddr,
            sizeof(NodeAddress),
            delay);

    }
    if (OdmrpLookupFgFlag(mcastAddr, &odmrp->fgFlag))
    {
        OdmrpSetTimer(node,
            MSG_NETWORK_CheckFg,
            &mcastAddr,
            sizeof(NodeAddress),
            odmrp->expireTime);
    }

    MESSAGE_Free(node, msg);

} // End of OdmrpHandleReply Function

//--------------------------------------------------------
// FUNCTION     OdmrpHandleAck
// PURPOSE      Processing explicit ack when received.
//
// Paremeters:
//     node  : Node handling the ACK packet.
//     msg   : The ack packet.
// lastAddr  : IP Address of the node generating Ack message
// targetAddr : The Target Address of the Ack packet
//----------------------------------------------------------

static
void OdmrpHandleAck(Node* node,
                    Message* msg,
                    NodeAddress lastAddr,
                    NodeAddress targetAddr)
{

    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);
    OdmrpAck* ackPkt = (OdmrpAck *)MESSAGE_ReturnPacket(msg);
    NodeAddress     nodeIPAddress;
    nodeIPAddress = NetworkIpGetInterfaceAddress(node,
        GetDefaultInterfaceIndex(node, NETWORK_IPV4));

    // Process only if I'm the target
    if (targetAddr == nodeIPAddress)
    {
        if (ODMRP_DEBUG)
        {
            char addressStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(lastAddr, addressStr);
            printf("Node %d got Ack from node %s\n",
                node->nodeId, addressStr);
        }

        // trace recd pkt
        ActionData acnData;
        acnData.actionType = RECV;
        acnData.actionComment = NO_COMMENT;
        TRACE_PrintTrace(node,
                         msg,
                         TRACE_NETWORK_LAYER,
                         PACKET_IN,
                         &acnData);

        // Update the ack table entries.
        OdmrpDeleteAckTable(ackPkt->mcastAddr,
            ackPkt->srcAddr,
            lastAddr,
            &odmrp->ackTable);

    }
    MESSAGE_Free(node, msg);

} // End of  OdmrpHandleAck Function

//---------------------------------------------------------------
//  FUNCTION     OdmrpHandelProtocolPacket
//  PURPOSE      Handles ODMRP packet that was received from the MAC
//               layer with destination being some other node.
//
// Parameters:
//     node:       node that is handling the ODMRP packet.
//     msg:        msg, that event that needs to be handled.
//    srcAddr:     The address of the source that sent the packet.
//    destAddr:    The multicast address that the packet belongs to.
//
// Note:           This function is called when packets are received
//                 from MAC.  The IP header is already removed.
//---------------------------------------------------------------

void OdmrpHandleProtocolPacket(Node* node,
                               Message* msg,
                               NodeAddress srcAddr,
                               NodeAddress destAddr)
{
    ODMRP_PacketType* odmrpHeader = 0;
    unsigned char *pktType;


    odmrpHeader = (ODMRP_PacketType *)
        MESSAGE_ReturnPacket(msg);

    pktType = (unsigned char *) odmrpHeader;

    switch (*pktType)
    {
            case ODMRP_JOIN_QUERY:

                OdmrpHandleJoinQuery(node, msg);

            break;

        case ODMRP_ACK:

            OdmrpHandleAck(node,
                msg,
                srcAddr,
                destAddr);

            break;

        case ODMRP_JOIN_REPLY:

            OdmrpHandleReply(node,
                msg,
                srcAddr);

            break;

        default:
            printf("The Node-Id %d\n",node->nodeId);
            ERROR_Assert(FALSE,
                "ODMRP received packet of unknown type\n");
    }

} // End of OdmrpHandleProtocolPacket Function

//----------------------------------------------------------------
//
//  FUNCTION     OdmrpRouterFunction
//  PURPOSE      Handles ODMRP packet that was from the transport
//               layer or from the MAC layer with this node as the
//               destination.
//
//  Parameters:
//     node      : node that is handling the ODMRP event.
//     msg       : msg, that event that needs to be handled.
//     destAddr  : The multicast address that the packet belongs to.
// interfaceIndex: Interface Index
// packetWasRouted: Boolean variable
//
// Note: Packet is from IP with IP header already created.
//       ipHeader->ip_dst specifies the multicast destination.
//
//-------------------------------------------------------------

void  OdmrpRouterFunction(Node* node,
                          Message* msg,
                          NodeAddress destAddr,
                          int interfaceIndex,
                          BOOL* packetWasRouted,
                          NodeAddress prevHop)
{
    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_ODMRP);

    IpHeaderType* ipHeader = (IpHeaderType *) msg->packet;
    unsigned int originalProtocol = (unsigned int) ipHeader->ip_p;

    // The case where packet came from MAC but belongs to this node.
    *packetWasRouted = TRUE;


    // Let ODMRP take care of the work now
    // Process packet at the source
    if ((prevHop == ANY_IP) && (ipHeader->ip_src ==
           NetworkIpGetInterfaceAddress(node,
                             GetDefaultInterfaceIndex(node, NETWORK_IPV4))))
    {
        if (ODMRP_DEBUG)
        {
            printf("Node %d got DATA from transport layer\n",
                node->nodeId);
        }

        if (OdmrpLookupSentTable(destAddr, &odmrp->sentTable) &&
            !OdmrpCheckSendQuery(destAddr, &odmrp->sentTable))
        {
            if (ODMRP_DEBUG)
            {
                printf("    Sending Data\n");
            }
            // Sending only data packet
            OdmrpSendData(odmrp,
                node,
                msg,
                destAddr,
                originalProtocol);
        } // If group membership info. known
        else
        {
            if (ODMRP_DEBUG)
            {
                printf("    Sending Join Query\n");
            }

#ifdef PIGGYBACKING
            OdmrpSendQuery(odmrp,
                    node,
                    msg,
                    destAddr,
                    originalProtocol);
#else
            if (!OdmrpLookupSentTable(destAddr, &odmrp->sentTable))
            {
                //Piggybacking is done for the first query only
                OdmrpSendQuery(odmrp,
                    node,
                    msg,
                    destAddr,
                    originalProtocol);
            }
            else
            {
                //sending data packet seprately
                OdmrpSendData(odmrp,
                    node,
                    msg,
                    destAddr,
                    originalProtocol);

                //sending Periodic Join Query
                OdmrpSendPeriodicQuery(odmrp,
                    node,
                    //msg,
                    destAddr,
                    TRACE_ODMRP);
            }
#endif
        }
    }   // if source
    else
    {
        // If I am not a source just check & fwd data
        // No need to send query Again
        OdmrpHandleData(node, msg);

    }   // if not source



} // End of OdmrpRouterFunction Function


//---------------------------------------------------------------
//
// FUNCTION     OdmrpFinalize
// PURPOSE      Called at the end of simulation to collect the results of
//              the simulation of ODMRP protocol of the NETWORK layer.
//
// Parameter:
//     node:    node for which results are to be collected
//
//------------------------------------------------------------

void OdmrpFinalize(Node* node)
{
    char buf[MAX_STRING_LENGTH];
    OdmrpData* odmrp = (OdmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_ODMRP);

    sprintf(buf, "Join Queries Originated = %d",
        odmrp->stats.numQueryOriginated);
    IO_PrintStat(node,
        "Network",
        "ODMRP",
        ANY_DEST,
        -1,
        buf);

    sprintf(buf, "Join Queries Transmitted = %d",
        odmrp->stats.numQueryTxed);
    IO_PrintStat(node,
        "Network",
        "ODMRP",
        ANY_DEST,
        -1,
        buf);

    sprintf(buf, "Join Replies Sent = %d", odmrp->stats.numReplySent);
    IO_PrintStat(node,
        "Network",
        "ODMRP",
        ANY_DEST,
        -1,
        buf);

    sprintf(buf, "Join Replies Forwarded = %d",
        odmrp->stats.numReplyForwarded);
    IO_PrintStat(node,
        "Network",
        "ODMRP",
        ANY_DEST,
        -1,
        buf);

    sprintf(buf, "Join Replies Retransmitted = %d",
        odmrp->stats.numReplyTransmitted);
    IO_PrintStat(node,
        "Network",
        "ODMRP",
        ANY_DEST,
        -1,
        buf);

    sprintf(buf, "ACKs Sent = %d", odmrp->stats.numAckSent);
    IO_PrintStat(node,
        "Network",
        "ODMRP",
        ANY_DEST,
        -1,
        buf);

    sprintf(buf, "Data Packets Relayed = %d",
        odmrp->stats.numDataTxed - odmrp->stats.numDataSent);
    IO_PrintStat(node,
        "Network",
        "ODMRP",
        ANY_DEST,
        -1,
        buf);

    sprintf(buf, "Data Packets Sent As Data Source = %d",
        odmrp->stats.numDataSent);
    IO_PrintStat(node,
        "Network",
        "ODMRP",
        ANY_DEST,
        -1,
        buf);

    sprintf(buf, "Data Packets Received = %d", odmrp->stats.numDataReceived);
    IO_PrintStat(node,
        "Network",
        "ODMRP",
        ANY_DEST,
        -1,
        buf);

    if (odmrp->pcEnable)
    {
        sprintf(buf,"Total GiveUp Messages Sent = %d",odmrp->stats.numGiveUpTx);
        IO_PrintStat(node,
            "Network",
            "ODMRP",
            ANY_DEST,
            -1,
            buf);
        sprintf(buf,"Total GiveUp Messages Received = %d",
            odmrp->stats.numGiveUpRx);
        IO_PrintStat(node,
            "Network",
            "ODMRP",
            ANY_DEST,
            -1,
            buf);
    }
} // End of OdmrpFinalize Function

#endif
