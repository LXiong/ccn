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

// Note: Implementation according to draft-ietf-manet-aodv-08.txt and
//       then upgraded to draft-ietf-manet-aodv-09.txt

// Future work: 1.  Shifting the protocol in application layer to use
//                  udp/654.
//              2.  Implementation of subnet leader.
//              3.  Implementation of route acknowledgement.
//              4.  Network layer link detection using passive
//                  acknowledgement.
//              5.  Hello interval extension.
//              6.  Multicast extension.
//              7.  Dynamic timeout determination.
//              8.  Implementation of infrastructure router

// Assumption: 1. When a node will try to find a better route if it does, it
//             will delete the previous valid route and will initiate a
//             route request
//             2. For Sending Gratuitous Route reply draft 10 specification
//             has been used instead of draft 9
//             3. For updating lifetime, when forwarding a packet draft 10
//             specification has been taken instead of draft 9 which says to
//             update the lifetime of the next hop towards the destination
//             4. Route timeout time for neighbors has been taken from draft
//             10, instead of draft 9 which is ALLOWED_HELLO_LOSS *
//             HELLO_INTERVAL instead of ACTIVE_ROUTE_TIMEOUT

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"
#include "ipv6.h"
#include "routing_aodv.h"
#include "buffer.h"
#include "external_socket.h"

#define  AODV_DEBUG 0
#define  AODV_DEBUG_INIT 0
#define  AODV_DEBUG_LOCAL_REPAIR 0
#define  AODV_DEBUG_ROUTE_TABLE 0
#define  AODV_DEBUG_AODV_TRACE 0
#define  AODV_DEBUG_HELLO 0

#define AODV_PC_ERAND(aodvJitterSeed) (RANDOM_nrand(aodvJitterSeed)\
    % AODV_BROADCAST_JITTER)

/*
double AodvRand(RandomSeed aodvJitterSeed)
{
    double val = RANDOM_nrand(aodvJitterSeed);
    return val;
}
*/

void D_AodvPrint::ExecuteAsString(const std::string& in, std::string& out)
{
    AodvRoutingTable* routeTable = &aodv->routeTable;
    AodvRouteEntry* rtEntry = NULL;
    int i = 0;
    EXTERNAL_VarArray v;
    char str[MAX_STRING_LENGTH];

    EXTERNAL_VarArrayInit(&v, 400);

    EXTERNAL_VarArrayConcatString(
        &v,
        "The Routing Table is:\n"
        "      Dest       DestSeq  HopCount  Intf     NextHop    "
        "activated   lifetime    precursors\n"
        "-------------------------------------------------------------"
        "--------------------------\n");
    for (i = 0; i < AODV_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (rtEntry = routeTable->routeHashTable[i]; rtEntry != NULL;
            rtEntry = rtEntry->hashNext)
        {
            char time[MAX_STRING_LENGTH];
            char dest[MAX_STRING_LENGTH];
            char nextHop[MAX_STRING_LENGTH];
            char trueOrFalse[6];

            IO_ConvertIpAddressToString(
                &rtEntry->destination,
                dest);

            IO_ConvertIpAddressToString(
                &rtEntry->nextHop,
                nextHop);

            TIME_PrintClockInSecond(rtEntry->lifetime, time);

            if (rtEntry->activated)
            {
                strcpy(trueOrFalse, "TRUE");
            }
            else
            {
                strcpy(trueOrFalse, "FALSE");
            }

            sprintf(str, "%15s  %5u  %5d    %5d  %15s  %5s    %9s  ", dest,
                rtEntry->destSeqNum,
                rtEntry->hopCount,
                rtEntry->outInterface,
                nextHop,
                trueOrFalse,
                time);

            EXTERNAL_VarArrayConcatString(&v, str);

            if (rtEntry->precursors.size == 0)
            {
                EXTERNAL_VarArrayConcatString(&v, "NULL\n");
            }
            else
            {
                AodvPrecursorsNode* precursors = NULL;

                char destaddr[20];

                IO_ConvertIpAddressToString(&(precursors->destAddr),
                    destaddr);


                for (precursors = rtEntry->precursors.head;
                    precursors != NULL;precursors = precursors->next)
                {
                    sprintf(str, "%s ", destaddr);
                    EXTERNAL_VarArrayConcatString(&v, str);
                }
                EXTERNAL_VarArrayConcatString(&v, "\n");
            }
        }
    }
    EXTERNAL_VarArrayConcatString(
        &v,
        "-------------------------------------------------------------"
        "-----------------------");

    out = v.data;
    EXTERNAL_VarArrayFree(&v);
}

// /**
// AODV Memory Manager
// **/

// /**
// FUNCTION : AodvMemoryChunkAlloc
// LAYER    : NETWORK
// PURPOSE  : Function to allocate a chunk of memory
// PARAMETERS:
// +aodv:AodvData*:Pointer to AodvData
// RETURN   ::void:NULL
// **/

static
void AodvMemoryChunkAlloc(AodvData* aodv)
{
    int i = 0;
    AodvMemPollEntry* freeList = NULL;

    aodv->freeList = (AodvMemPollEntry*)MEM_malloc(AODV_MEM_UNIT
                                            * sizeof(AodvMemPollEntry));

    ERROR_Assert(aodv->freeList != NULL, " No available Memory");

    freeList = aodv->freeList;

    for (i = 0; i < AODV_MEM_UNIT - 1; i++)
    {
        freeList[i].next = &freeList[i + 1];
    }

    freeList[AODV_MEM_UNIT - 1].next = NULL;
}


// /**
// FUNCTION  : AodvMemoryMalloc
// LAYER     : NETWORK
// PURPOSE   : Function to allocate a single cell of
//             memory from the memory chunk
// PARAMETERS:
// +aodv:AodvData*:Pointer to Aodv main data structure
// RETURN    :
// temp:AodvRouteEntry*:Address of free memory cell
// **/

static
AodvRouteEntry* AodvMemoryMalloc(AodvData* aodv)
{
    AodvRouteEntry* temp = NULL;

    if (!aodv->freeList)
    {
        AodvMemoryChunkAlloc(aodv);
    }

    temp = &aodv->freeList->routeEntry;
    aodv->freeList = aodv->freeList->next;
    return temp;
}


// /**
// FUNCTION : AodvMemoryFree
// LAYER    : NETWORK
// PURPOSE  : Function to return a memory cell to the memory pool
// PARAMETERS:
// +aodv:AodvData*:Pointer to Aodv main data structure
// +ptr:AodvRouteEntry*: Pointer  to aodv route entry
// RETURN   ::void:NULL
// **/

static
void AodvMemoryFree(AodvData* aodv,AodvRouteEntry* ptr)
{
    AodvMemPollEntry* temp = (AodvMemPollEntry*)ptr;
    temp->next = aodv->freeList;
    aodv->freeList = temp;
}


// /**
// FUNCTION :AodvPrintTrace()
// LAYER    :NETWORK
// PURPOSE  :Trace printing function to call for Aodv packet.
// ASSUMPTION   :None
// PARAMETERS:
// +node:Node*:Pointer to node
// +msg:Message*:Pointer to message
// +sendorReceive:char:value specify whether send or received
// RETURN   ::void:NULL
// **/

static
void AodvPrintTrace(
         Node* node,
         Message* msg,
         char sendOrReceive,
         BOOL ipv6Flag)
{
    unsigned int* pktPtr = (unsigned int *) MESSAGE_ReturnPacket(msg);
    char clockStr[MAX_STRING_LENGTH];
    FILE* fp = fopen("aodv.trace", "a");
    char dest[MAX_STRING_LENGTH];
    char src[MAX_STRING_LENGTH];

    if (!fp)
    {
        ERROR_ReportError("Can't open aodv.trace\n");
    }

    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

    // print packet ID
    fprintf(fp, "%u, %d; %s; %u %c; ",
        msg->originatingNodeId,
        msg->sequenceNumber,
        clockStr,
        node->nodeId,
        sendOrReceive);

    if ((*pktPtr >> 24) == AODV_RREQ)
    {
        // Print content of route request packets
        if (ipv6Flag)
        {
            Aodv6RreqPacket* rreq6Pkt = (Aodv6RreqPacket *) pktPtr;
            IO_ConvertIpAddressToString
                (&rreq6Pkt->destination.address,
                dest);
            IO_ConvertIpAddressToString(&rreq6Pkt->source.address, src);

            fprintf(fp, "%u, %u, %u, %u, %u, %u, %u, %u, %s, %u, %s, %u",
                AODV_GetType(rreq6Pkt->info.typeBitsHopcounts),
                (rreq6Pkt->info.typeBitsHopcounts & AODV_J_BIT) >> 23,
                (rreq6Pkt->info.typeBitsHopcounts & AODV_RREQ_R_BIT) >> 22,
                (rreq6Pkt->info.typeBitsHopcounts & AODV_G_BIT) >> 21,
                (rreq6Pkt->info.typeBitsHopcounts & AODV_G_BIT) >> 20,
                0,//rreqPkt->reserved,
                rreq6Pkt->info.typeBitsHopcounts & AODV_HOP_COUNT_BITS,
                rreq6Pkt->info.floodingId,
                dest,
                rreq6Pkt->destination.seqNum,
                src,
                rreq6Pkt->source.seqNum);
        }
        else
        {
            AodvRreqPacket* rreqPkt = (AodvRreqPacket *) pktPtr;

            IO_ConvertIpAddressToString(rreqPkt->destination.address, dest);
            IO_ConvertIpAddressToString(rreqPkt->source.address, src);

            fprintf(fp, "%u, %u, %u, %u, %u, %u, %u, %u, %s, %u, %s, %u",
                AODV_GetType(rreqPkt->info.typeBitsHopcounts),
                (rreqPkt->info.typeBitsHopcounts & AODV_J_BIT) >> 23,
                (rreqPkt->info.typeBitsHopcounts & AODV_RREQ_R_BIT) >> 22,
                (rreqPkt->info.typeBitsHopcounts & AODV_G_BIT) >> 21,
                (rreqPkt->info.typeBitsHopcounts & AODV_G_BIT) >> 20,
                0,//rreqPkt->reserved,
                rreqPkt->info.typeBitsHopcounts & AODV_HOP_COUNT_BITS,
                rreqPkt->info.floodingId,
                dest,
                rreqPkt->destination.seqNum,
                src,
                rreqPkt->source.seqNum);
        }
    }
    if ((*pktPtr >> 24) == AODV_RREP)
    {
        // Print content of route reply packets
        if (ipv6Flag)
        {
            Aodv6RrepPacket* rrep6Pkt = (Aodv6RrepPacket *) pktPtr;
            IO_ConvertIpAddressToString(
                                        &rrep6Pkt->destination.address,
                                        dest);
            IO_ConvertIpAddressToString(&rrep6Pkt->sourceAddr, src);

            fprintf(fp, "%u, %u, %u, %u, %u, %u, %s, %u, %s, %u",
            AODV_GetType(rrep6Pkt->typeBitsPrefixSizeHop),
            //here position of R-bit same as J bit for RREQ
            (rrep6Pkt->typeBitsPrefixSizeHop & AODV_R_BIT) >> 23,
            (rrep6Pkt->typeBitsPrefixSizeHop & AODV_A_BIT) >> 22,
            0,//rrepPkt->reserved,
            (rrep6Pkt->typeBitsPrefixSizeHop & AODV_PREFIX_SIZE_BITS) >> 8,
            rrep6Pkt->typeBitsPrefixSizeHop & AODV_DEST_COUNT_BITS,
            dest,
            rrep6Pkt->destination.seqNum,
            src,
            rrep6Pkt->lifetime);
        }
        else
        {
            AodvRrepPacket* rrepPkt = (AodvRrepPacket *) pktPtr;
            IO_ConvertIpAddressToString(rrepPkt->destination.address, dest);
            IO_ConvertIpAddressToString(rrepPkt->sourceAddr, src);

            fprintf(fp, "%u, %u, %u, %u, %u, %u, %s, %u, %s, %u",
                AODV_GetType(rrepPkt->typeBitsPrefixSizeHop),
                //here position of R-bit same as J bit for RREQ
                (rrepPkt->typeBitsPrefixSizeHop & AODV_R_BIT) >> 23,
                (rrepPkt->typeBitsPrefixSizeHop & AODV_A_BIT) >> 22,
                0,//rrepPkt->reserved,
                (rrepPkt->typeBitsPrefixSizeHop & AODV_PREFIX_SIZE_BITS) >>
                                                                          8,
                rrepPkt->typeBitsPrefixSizeHop & AODV_DEST_COUNT_BITS,
                dest,
                rrepPkt->destination.seqNum,
                src,
                rrepPkt->lifetime);
        }
    }
    if ((*pktPtr >> 24) == AODV_RERR)
    {
        // Print content of route error packets
        unsigned int i;
        UInt32 destCount = 0;
        AodvRerrPacket* rerrPkt = (AodvRerrPacket *) pktPtr;
        if (ipv6Flag)
        {
            Aodv6AddrSeqInfo* addr6Seq = NULL;
            fprintf(fp, "%u, %u, %u, %u ",
                AODV_GetType(rerrPkt->typeBitsDestCount),
                (rerrPkt->typeBitsDestCount & AODV_N_BIT) >> 23,
                0,//rerrPkt->reserved,
                rerrPkt->typeBitsDestCount & AODV_DEST_COUNT_BITS);

            addr6Seq = (Aodv6AddrSeqInfo *) (pktPtr + sizeof
                                                    (AodvRerrPacket));

            destCount = rerrPkt->typeBitsDestCount & AODV_DEST_COUNT_BITS;

            for (i = 0; i < destCount; i++)
            {
                IO_ConvertIpAddressToString(&addr6Seq[i].address, dest);
                fprintf(fp, "%s, %u, ", dest, addr6Seq[i].seqNum);
            }
        }
        else
        {
            AodvAddrSeqInfo* addrSeq = NULL;
            fprintf(fp, "%u, %u, %u, %u ",
                AODV_GetType(rerrPkt->typeBitsDestCount),
                (rerrPkt->typeBitsDestCount & AODV_N_BIT) >> 23,
                0,//rerrPkt->reserved,
                rerrPkt->typeBitsDestCount & AODV_DEST_COUNT_BITS);

            addrSeq = (AodvAddrSeqInfo *) (pktPtr + sizeof(AodvRerrPacket));

            destCount = rerrPkt->typeBitsDestCount & AODV_DEST_COUNT_BITS;

            for (i = 0; i < destCount; i++)
            {
                IO_ConvertIpAddressToString(addrSeq[i].address, dest);
                fprintf(fp, "%s, %u, ", dest, addrSeq[i].seqNum);
            }
        }
    }
    fprintf(fp, "\n");
    fclose(fp);
}

// /**
// FUNCTION   :: AodvPrintTraceXML
// LAYER      :: NETWORK
// PURPOSE    :: Print out packet trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + mntMsg  : Message* : Pointer to Message
// RETURN ::  void : NULL
// **/

void AodvPrintTraceXML(Node* node, Message* msg, NetworkType netType)
{

    char buf[MAX_STRING_LENGTH];
    char dest[MAX_STRING_LENGTH];
    char src[MAX_STRING_LENGTH];
    BOOL IPV6 = FALSE;

    if (msg == NULL)
    {
        return;
    }
    if (netType == NETWORK_IPV6)
    {
        IPV6 = TRUE;

    }

    UInt32* pktPtr = (UInt32* )MESSAGE_ReturnPacket(msg);

    sprintf(buf, "<aodv>");
    TRACE_WriteToBufferXML(node, buf);

    switch (*pktPtr >> 24)
    {
        case AODV_RREQ:
        {
            sprintf(buf, "<rreq>");
            TRACE_WriteToBufferXML(node, buf);
            if (IPV6)
            {
                Aodv6RreqPacket* rreq6Pkt = (Aodv6RreqPacket *)pktPtr;
                IO_ConvertIpAddressToString(&rreq6Pkt->destination.address,
                                            dest);
                IO_ConvertIpAddressToString(&rreq6Pkt->source.address, src);
                sprintf(buf,
                        "%u %u %u %u %u %u %u %u %s %u %s %u",
                AODV_GetType(rreq6Pkt->info.typeBitsHopcounts),
                (rreq6Pkt->info.typeBitsHopcounts & AODV_J_BIT) >> 23,
                (rreq6Pkt->info.typeBitsHopcounts & AODV_RREQ_R_BIT) >> 22,
                (rreq6Pkt->info.typeBitsHopcounts & AODV_G_BIT) >> 21,
                (rreq6Pkt->info.typeBitsHopcounts & AODV_D_BIT) >> 20,
                0,//rreqPkt->reserved,
                rreq6Pkt->info.typeBitsHopcounts & AODV_HOP_COUNT_BITS,
                rreq6Pkt->info.floodingId,
                dest,
                rreq6Pkt->destination.seqNum,
                src,
                rreq6Pkt->source.seqNum);
                TRACE_WriteToBufferXML(node, buf);
            }
            else
            {
                AodvRreqPacket* rreqPkt = (AodvRreqPacket *)pktPtr;
                IO_ConvertIpAddressToString(rreqPkt->destination.address,
                                            dest);
                IO_ConvertIpAddressToString(rreqPkt->source.address, src);
                sprintf(buf,
                        "%u %u %u %u %u %u %u %u %s %u %s %u",
                AODV_GetType(rreqPkt->info.typeBitsHopcounts),
                (rreqPkt->info.typeBitsHopcounts & AODV_J_BIT) >> 23,
                (rreqPkt->info.typeBitsHopcounts & AODV_RREQ_R_BIT) >> 22,
                (rreqPkt->info.typeBitsHopcounts & AODV_G_BIT) >> 21,
                (rreqPkt->info.typeBitsHopcounts & AODV_D_BIT) >> 20,
                0,//rreqPkt->reserved,
                rreqPkt->info.typeBitsHopcounts & AODV_HOP_COUNT_BITS,
                rreqPkt->info.floodingId,
                dest,
                rreqPkt->destination.seqNum,
                src,
                rreqPkt->source.seqNum);
                TRACE_WriteToBufferXML(node, buf);
            }

            sprintf(buf, "</rreq>");
            TRACE_WriteToBufferXML(node, buf);
            break;
        }

        case AODV_RREP:
        {
            sprintf(buf, "<rrep>");
            TRACE_WriteToBufferXML(node, buf);
            char time[MAX_STRING_LENGTH];
            if (IPV6)
            {
                Aodv6RrepPacket* rrep6Pkt = (Aodv6RrepPacket *) pktPtr;
                IO_ConvertIpAddressToString(&rrep6Pkt->destination.address,
                                            dest);
                IO_ConvertIpAddressToString(&rrep6Pkt->sourceAddr, src);
                TIME_PrintClockInSecond(rrep6Pkt->lifetime, time);
                sprintf(buf, "%u %u %u %u %u %u %s %u %s %s",
                AODV_GetType(rrep6Pkt->typeBitsPrefixSizeHop),
                //here position of R-bit same as J bit for RREQ
                (rrep6Pkt->typeBitsPrefixSizeHop & AODV_R_BIT) >> 23,
                (rrep6Pkt->typeBitsPrefixSizeHop & AODV_A_BIT) >> 22,
                0,//rrepPkt->reserved,
                (rrep6Pkt->typeBitsPrefixSizeHop & AODV_PREFIX_SIZE_BITS) >>
                                                                        8,
                rrep6Pkt->typeBitsPrefixSizeHop & AODV_DEST_COUNT_BITS,
                dest,
                rrep6Pkt->destination.seqNum,
                src,
                time);
                TRACE_WriteToBufferXML(node, buf);
            }
            else
            {
                AodvRrepPacket* rrepPkt = (AodvRrepPacket *) pktPtr;
                IO_ConvertIpAddressToString(rrepPkt->destination.address,
                                            dest);
                IO_ConvertIpAddressToString(rrepPkt->sourceAddr, src);
                TIME_PrintClockInSecond(rrepPkt->lifetime, time);
                sprintf(buf, "%u %u %u %u %u %u %s %u %s %s",
                AODV_GetType(rrepPkt->typeBitsPrefixSizeHop),
                //here position of R-bit same as J bit for RREQ
                (rrepPkt->typeBitsPrefixSizeHop & AODV_R_BIT) >> 23,
                (rrepPkt->typeBitsPrefixSizeHop & AODV_A_BIT) >> 22,
                0,//rrepPkt->reserved,
                (rrepPkt->typeBitsPrefixSizeHop & AODV_PREFIX_SIZE_BITS) >>
                                                                        8,
                rrepPkt->typeBitsPrefixSizeHop & AODV_DEST_COUNT_BITS,
                dest,
                rrepPkt->destination.seqNum,
                src,
                time);
                TRACE_WriteToBufferXML(node, buf);
            }
            sprintf(buf, "</rrep>");
            TRACE_WriteToBufferXML(node, buf);
            break;
        }
        case AODV_RERR:
        {

            sprintf(buf, "<rerr>");
            TRACE_WriteToBufferXML(node, buf);
            // Print content of route error packets
            unsigned int i;
            UInt32 destCount = 0;
            AodvRerrPacket* rerrPkt = (AodvRerrPacket *) pktPtr;
            int rerrReserved = 0;

            if (IPV6)
            {
                Aodv6AddrSeqInfo* addr6Seq = NULL;
                sprintf(buf, "%u %hu %hu %hu",
                    AODV_GetType(rerrPkt->typeBitsDestCount),
                    (rerrPkt->typeBitsDestCount & AODV_N_BIT) >> 23,
                    rerrReserved,//rerrPkt->reserved,
                    rerrPkt->typeBitsDestCount & AODV_DEST_COUNT_BITS);
                    TRACE_WriteToBufferXML(node, buf);

                     addr6Seq = (Aodv6AddrSeqInfo *)
                            (((char *)rerrPkt) + sizeof(AodvRerrPacket));

                     destCount =
                        rerrPkt->typeBitsDestCount & AODV_DEST_COUNT_BITS;

                    for (i = 0; i < destCount; i++)
                    {
                        IO_ConvertIpAddressToString(&addr6Seq[i].address,
                                                    dest);
                        sprintf(buf, "<linksLost> %s %hu</linksLost>",
                                    dest,
                                    addr6Seq[i].seqNum);
                        TRACE_WriteToBufferXML(node, buf);
                    }
            }
            else
            {
                AodvAddrSeqInfo* addrSeq = NULL;
                sprintf(buf, "%u %hu %hu %hu",
                    AODV_GetType(rerrPkt->typeBitsDestCount),
                    (rerrPkt->typeBitsDestCount & AODV_N_BIT) >> 23,
                    rerrReserved,//rerrPkt->reserved,
                    rerrPkt->typeBitsDestCount & AODV_DEST_COUNT_BITS);
                    TRACE_WriteToBufferXML(node, buf);
                    addrSeq = (AodvAddrSeqInfo *)
                        (((char *)rerrPkt) + sizeof(AodvRerrPacket));

                    destCount =
                    rerrPkt->typeBitsDestCount & AODV_DEST_COUNT_BITS;

                    for (i = 0; i < destCount; i++)
                    {
                        IO_ConvertIpAddressToString(addrSeq[i].address,
                                                    dest);
                        sprintf(buf, "<linksLost> %s %hu</linksLost>",
                                dest,
                                addrSeq[i].seqNum);
                        TRACE_WriteToBufferXML(node, buf);
                    }

            }//end of else
            sprintf(buf, "</rerr>");
            TRACE_WriteToBufferXML(node, buf);
            break;
        }//end of case

    }//end of switch
    sprintf(buf, "</aodv>");
    TRACE_WriteToBufferXML(node, buf);
}

// /**
// FUNCTION     :AodvInitTrace()
// LAYER        :NETWORK
// PURPOSE      :Enabling Aodv trace. The output will go in file aodv.trace
// ASSUMPTION   :None
// PARAMETERS   :
// + node : Node* : Pointer to node
// + nodeInput    : const NodeInput* : Pointer to NodeInput
// RETURN   ::void:NULL
//**/

static
void AodvInitTrace(Node* node, const NodeInput* nodeInput)
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
        "TRACE-AODV",
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
                "TRACE-AODV should be either \"YES\" or \"NO\".\n");
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
            TRACE_EnableTraceXMLFun(node, TRACE_AODV,
                "AODV", AodvPrintTraceXML, writeMap);
    }
    else
    {
            TRACE_DisableTraceXML(node, TRACE_AODV,
                "AODV", writeMap);
    }
    writeMap = FALSE;

    if (AODV_DEBUG_AODV_TRACE)
    {
        // Empty or create a file named aodv.trace to print the packet
        // contents
        FILE* fp = fopen("aodv.trace", "w");
        fclose(fp);
    }
}

// /**
// FUNCTION : AodvGetFloodingId
// LAYER    : NETWORK
// PURPOSE  : Obtains the broadcast ID for the outgoing packet
// PARAMETERS:
// aodv:AodvData*:Pointer to aodv main data structure.
// RETURN:
// +bcast:unsigned int:The flooding Id
// NOTE     : This function increments the flooding id by 1
// **/

static
unsigned int AodvGetFloodingId(AodvData* aodv)
{
    unsigned int bcast;
    bcast = aodv->floodingId;
    aodv->floodingId++;
    return bcast;
}


// /**
// FUNCTION : AodvSetTimer
// LAYER    : NETWORK
// PURPOSE  : Set timers for protocol events
// PARAMETERS:
// +node:Node*:Pointer to node which is scheduling an event
// +eventType:int:The event type of the message
// +destAddr:Address:Destination for which the event has been sent (if
//                      necessary)
// +delay:clocktype:Time after which the event will expire
//RETURN    ::void:NULL
// **/

static
void AodvSetTimer(
         Node* node,
         int eventType,
         Address destAddr,
         clocktype delay)
{
    Message* newMsg = NULL;
    Address* info = NULL;
    NetworkRoutingProtocolType protocolType;

    if (destAddr.networkType == NETWORK_IPV6)
    {
        protocolType = ROUTING_PROTOCOL_AODV6;
    }
    else
    {
        protocolType = ROUTING_PROTOCOL_AODV;
    }
    if (AODV_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];

        char address[MAX_STRING_LENGTH];
                 IO_ConvertIpAddressToString(&destAddr, address);


        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        printf("\t\tnow %s\n", clockStr);

        TIME_PrintClockInSecond((node->getNodeTime() + delay), clockStr);

        printf("\t\ttimer to expire at %s\n", clockStr);

        if (((destAddr.interfaceAddr.ipv4 != ANY_IP)
            && (destAddr.networkType == NETWORK_IPV4))
            || ((!IS_MULTIADDR6(destAddr.interfaceAddr.ipv6))
            && (destAddr.networkType == NETWORK_IPV6)))
        {
            printf("\t\tdestination %s\n", address);
        }
    }

    // Allocate message for the timer
    newMsg = MESSAGE_Alloc(
                 node,
                 NETWORK_LAYER,
                 protocolType,
                 eventType);

    // Assign the address for which the timer is meant for
    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Address));

    info = (Address *) MESSAGE_ReturnInfo(newMsg);

    memcpy(info, &destAddr, sizeof(Address));

    // Schedule the timer after the specified delay
    MESSAGE_Send(node, newMsg, delay);
}


// /**
// FUNCTION     :   AodvPlaceRouteEntry
// LAYER        :   NETWORK
// PURPOSE      :   Insert route entry in expiry list.
// PARAMETERS   :
// +routeTable:AodvRoutingTable*:pointer to routing table
// +routeEntry:AodvRouteEntry*: pointer to route entry to be
//                              inserted into expirylist
// RETURN       ::void:NULL
// **/

static
void AodvPlaceRouteEntry(
     AodvRoutingTable* routeTable,
     AodvRouteEntry* routeEntry)
{

    if (routeTable->routeExpireTail == NULL)
    {
        routeTable->routeExpireTail = routeEntry;
        routeTable->routeExpireHead = routeEntry;
        routeEntry->expirePrev = NULL;
        routeEntry->expireNext = NULL;
    }
    else if (routeTable->routeExpireTail == routeTable->routeExpireHead)
    {
        if (routeTable->routeExpireTail->lifetime > routeEntry->lifetime)
        {
            routeEntry->expireNext = routeTable->routeExpireTail;
            routeEntry->expirePrev = NULL;
            routeTable->routeExpireHead = routeEntry;
            routeTable->routeExpireTail->expirePrev = routeEntry;
        }
        else
        {
            routeEntry->expireNext = NULL;
            routeEntry->expirePrev = routeTable->routeExpireHead;
            routeTable->routeExpireTail = routeEntry;
            routeTable->routeExpireHead->expireNext = routeEntry;
        }
    }
    else
    {
        AodvRouteEntry* current = routeTable->routeExpireTail;
        AodvRouteEntry* previous = current;

        while (current && current->lifetime > routeEntry->lifetime)
        {
            previous = current;
            current = current->expirePrev;
        }

        routeEntry->expirePrev = current;

        if (routeTable->routeExpireTail == current)
        {
            routeTable->routeExpireTail = routeEntry;
            routeEntry->expireNext = NULL;
        }
        else
        {
            previous->expirePrev = routeEntry;
            routeEntry->expireNext = previous;

        }

        if (previous == routeTable->routeExpireHead)
        {
            routeTable->routeExpireHead = routeEntry;
        }
        else
        {
            current->expireNext = routeEntry;
        }
    }
}


// /**
// FUNCTION     : AodvMoveRouteEntry
// LAYER        : NETWORK
// PURPOSE      :  move entry into expiry table
// PARAMETERS   :
// +routeTable:AodvRoutingTable*:pointer to routing table
// +routeEntry:AodvRouteEntry*:pointer to route entry to be moved into
//                             expiry list
// RETURN       ::void:NULL
// **/

static
void AodvMoveRouteEntry(
         AodvRoutingTable* routeTable,
         AodvRouteEntry* routeEntry)
{
    if (routeTable->routeExpireTail != routeEntry)
    {
        if (routeTable->routeExpireHead == routeEntry)
        {
            routeTable->routeExpireHead = routeEntry->expireNext;
        }
        else
        {
            routeEntry->expirePrev->expireNext = routeEntry->expireNext;
        }
        routeEntry->expireNext->expirePrev = routeEntry->expirePrev;
        AodvPlaceRouteEntry(routeTable,routeEntry);
    }
}

// /**
// FUNCTION   :: AodvIsSmallerAddress
// LAYER      :: NETWORK
// PURPOSE    :: Check if address1 is smaller than address2.
//               If IPv6 address then host bits are compared.
// PARAMETERS ::
//  +destAddr:  Address : Destination Address.
//  +sent:  AodvRreqSentTable* : Pointer to sent table.
// RETURN     :: TRUE if Smaller.
// **/
static BOOL
AodvIsSmallerAddress(Address address1, Address address2)
{
    if (address1.networkType != address2.networkType)
    {
        ERROR_Assert(FALSE, "Address of same type not compared \n");
    }
    else if (address1.networkType == NETWORK_IPV6)
    {
        if (Ipv6CompareAddr6(
                address1.interfaceAddr.ipv6,
                address2.interfaceAddr.ipv6) < 0)
        {
            return TRUE;
        }
    }
    else
    {
        if (address1.interfaceAddr.ipv4 < address2.interfaceAddr.ipv4)
        {
            return TRUE;
        }
    }
    return FALSE;
}

// /**
// FUNCTION : AodvGetLastHopCount
// LAYER    : NETWORK
// PURPOSE  :  Obtains the last hop count known to the destination node
// PARAMETERS:
// +aodv:Aodv*:Pointer to Aodv data structure
// +destAddr:Address:for which the next hop is wanted
// +routeTable:AodvRouteTable*:Aodv routing table
// RETURN   ::void:NULL
// **/

static
int AodvGetLastHopCount(
        AodvData* aodv,
        Address destAddr,
        AodvRoutingTable* routeTable)
{
    AodvRouteEntry* current = NULL;
    if (destAddr.networkType == NETWORK_IPV6)
    {
        current = routeTable->routeHashTable[destAddr.AODV_Ip6HostBit
                                              % AODV_ROUTE_HASH_TABLE_SIZE];
    }
    else
    {
        current = routeTable->routeHashTable[destAddr.interfaceAddr.ipv4
                                              % AODV_ROUTE_HASH_TABLE_SIZE];
    }

    while (current && AodvIsSmallerAddress(current->destination, destAddr))
    {
        current = current->hashNext;
    }
    if (current && Address_IsSameAddress(&current->destination,&destAddr))
    {
        ERROR_Assert(current->lastHopCount > 0,
            "AODV: invalid last hop count.\n");

        // Got the matching destination so return the hop count
        return current->lastHopCount;
    }
    else
    {
        // No match found
        return AODV_TTL_START;
    }
}


// /**
// FUNCTION : AodvPrintRoutingTable
// LAYER    : NETWORK
// PURPOSE  : Printing the different fields of the routing table of Aodv
// PARAMETERS:
// +node:Node*:Pointer to the node printing the routing table
// +routeTable:AodvRouteTable*:Pointer to Aodv routing table
// RETURN   ::void:NULL
// **/

static
void AodvPrintRoutingTable(
         Node* node,
         AodvRoutingTable* routeTable)
{
    AodvRouteEntry* rtEntry = NULL;
    int i = 0;
    printf("The Routing Table of Node %u is:\n"
        "      Dest       DestSeq  HopCount  Intf     NextHop    "
        "activated   lifetime    precursors\n"
        "-------------------------------------------------------------"
        "--------------------------\n", node->nodeId);
    for (i = 0; i < AODV_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (rtEntry = routeTable->routeHashTable[i]; rtEntry != NULL;
            rtEntry = rtEntry->hashNext)
        {
            char time[MAX_STRING_LENGTH];
            char dest[MAX_STRING_LENGTH];
            char nextHop[MAX_STRING_LENGTH];
            char trueOrFalse[6];

            IO_ConvertIpAddressToString(
                &rtEntry->destination,
                dest);

            IO_ConvertIpAddressToString(
                &rtEntry->nextHop,
                nextHop);

            TIME_PrintClockInSecond(rtEntry->lifetime, time);

            if (rtEntry->activated)
            {
                strcpy(trueOrFalse, "TRUE");
            }
            else
            {
                strcpy(trueOrFalse, "FALSE");
            }

            printf("%15s  %5u  %5d    %5d  %15s  %5s    %9s  ", dest,
                rtEntry->destSeqNum, rtEntry->hopCount,
                rtEntry->outInterface, nextHop, trueOrFalse, time);

            if (rtEntry->precursors.size == 0)
            {
                printf("%s\n", "NULL");
            }
            else
            {
                AodvPrecursorsNode* precursors = NULL;
                char pdestAddr[MAX_STRING_LENGTH];


                for (precursors = rtEntry->precursors.head;
                    precursors != NULL;precursors = precursors->next)
                {
                    static int count =0;
                    count++;
                    IO_ConvertIpAddressToString(
                        &precursors->destAddr,
                        pdestAddr);
                    //printf("%u ", precursors->destAddr);

                    printf("%s %d", pdestAddr, count);
                }
                printf("\n");
            }
        }
    }
    printf("-------------------------------------------------------------"
        "-----------------------\n\n");
}



// /**
// FUNCTION : AodvInsertBuffer
// LAYER   : NETWORK
// PURPOSE  : Insert a packet into the buffer if no route is available
// PARAMETERS:
// +node:Node*:Pointer to node
// +msg:Message*: Pointer to the message waiting for a route to destination
// +destAddr:Address:The destination address of the packet
// +previousHop:Address:Previous hop address
// +buffer:AodvMessageBuffer*:Pointer to the buffer to store the message
// RETURN   ::void:NULL
// **/

static
void AodvInsertBuffer(
         Node* node,
         Message* msg,
         Address destAddr,
         Address previousHop,
         AodvMessageBuffer* buffer)
{
    AodvBufferNode* current = NULL;
    AodvBufferNode* previous = NULL;
    AodvBufferNode* newNode = NULL;
    AodvData* aodv = NULL;
    BOOL IPV6 = FALSE;

    if (destAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData*) NetworkIpGetRoutingProtocol(
                                    node,
                                    ROUTING_PROTOCOL_AODV6,
                                    NETWORK_IPV6);
        IPV6 = TRUE;
    }
    else
    {
        aodv = (AodvData*) NetworkIpGetRoutingProtocol(
                                     node,
                                     ROUTING_PROTOCOL_AODV,
                                     NETWORK_IPV4);
    }

    // if the buffer exceeds silently drop the packet
    // if no buffer size is specified in bytes it will only check for
    // number of packet.

    if (aodv->bufferSizeInByte == 0)
    {
        if (buffer->size == aodv->bufferSizeInNumPacket)
        {

            //Trace drop
            ActionData acnData;
            acnData.actionType = DROP;
            acnData.actionComment = DROP_BUFFER_SIZE_EXCEED;
            TRACE_PrintTrace(node,
                             msg,
                             TRACE_NETWORK_LAYER,
                             PACKET_IN,
                             &acnData,
                             aodv->defaultInterfaceAddr.networkType);
            MESSAGE_Free(node, msg);
            aodv->stats.numDataDroppedForOverlimit++;
            return;
        }
    }
    else
    {
        if ((buffer->numByte + MESSAGE_ReturnPacketSize(msg)) >
            aodv->bufferSizeInByte)
        {
            //Trace drop
            ActionData acnData;
            acnData.actionType = DROP;
            acnData.actionComment = DROP_BUFFER_SIZE_EXCEED;
            TRACE_PrintTrace(node,
                             msg,
                             TRACE_NETWORK_LAYER,
                             PACKET_IN,
                             &acnData,
                             aodv->defaultInterfaceAddr.networkType);
            MESSAGE_Free(node, msg);
            aodv->stats.numDataDroppedForOverlimit++;
            return;
        }
    }

    // Find Insertion point.  Insert after all address matches.
    // This is to maintain a sorted list in ascending order of the
    // destination address
    previous = NULL;
    current = buffer->head;

    while (current
            &&
            (AodvIsSmallerAddress(current->destAddr, destAddr)
            || Address_IsSameAddress(&current->destAddr, &destAddr)))
    {
        previous = current;
        current = current->next;
    }
    newNode = (AodvBufferNode*) MEM_malloc(sizeof(AodvBufferNode));
    // Store the allocate message along with the destination number and
    // the time at which the packet has been inserted

    if (IPV6)
    {
        SetIPv6AddressInfo(&newNode->destAddr,
                            destAddr.interfaceAddr.ipv6);
        SetIPv6AddressInfo(&newNode->previousHop,
                             previousHop.interfaceAddr.ipv6);
    }
    else
    {
        SetIPv4AddressInfo(&newNode->destAddr,
                                destAddr.interfaceAddr.ipv4);
        SetIPv4AddressInfo(&newNode->previousHop,
                                previousHop.interfaceAddr.ipv4);
    }
    newNode->msg = msg;
    newNode->timestamp = node->getNodeTime();
    newNode->next = current;

    // Increase the size of the buffer
    ++(buffer->size);
    buffer->numByte += MESSAGE_ReturnPacketSize(msg);

    // Got the insertion point
    if (previous == NULL)
    {
        // The is the first message in the buffer or to be
        // inserted in the first
        buffer->head = newNode;
    }
    else
    {
        // This is an intermediate node in the list
        previous->next = newNode;
    }
}


// /**
// FUNCTION : AodvGetBufferedPacket
// LAYER    : NETWORK
// PURPOSE  : Extract the packet that was buffered
// PARAMETERS:
// +destAddr:Address:the destination address of the packet to be
//                   retrieved
// +previousHop:Address: Previous hop address
// +buffer:AodvMessageBuffer*:Pointer to the message buffer
// RETURN   :
// +pkttoDest:Message*:The message for this destination
// **/

static
Message* AodvGetBufferedPacket(
    Node* node,
    Address destAddr,
    Address* previousHop,
    AodvMessageBuffer* buffer)
{
    AodvBufferNode* current = buffer->head;
    Message* pktToDest = NULL;
    AodvBufferNode* toFree = NULL;
    BOOL IPV6 = FALSE;

    if (destAddr.networkType == NETWORK_IPV6)
    {
        IPV6 = TRUE;
        previousHop->networkType = NETWORK_IPV6;
        CLR_ADDR6(previousHop->interfaceAddr.ipv6);
    }
    else
    {
        previousHop->networkType = NETWORK_IPV4;
        previousHop->interfaceAddr.ipv4 = 0;
    }
    if (!current)
    {
        // No packet in the buffer so nothing to do
    }
    else if (Address_IsSameAddress(&current->destAddr, &destAddr))
    {
        // The first packet is the desired packet
        toFree = current;
        buffer->head = toFree->next;

        pktToDest = toFree->msg;
        if (IPV6)
        {
            SetIPv6AddressInfo(previousHop,
                                   toFree->previousHop.interfaceAddr.ipv6);
        }
        else
        {
            SetIPv4AddressInfo(previousHop,
                                   toFree->previousHop.interfaceAddr.ipv4);
        }


        buffer->numByte -= MESSAGE_ReturnPacketSize(toFree->msg);
        MEM_free(toFree);
        --(buffer->size);
    }
    else
    {
        while (current->next
                && AodvIsSmallerAddress(current->next->destAddr, destAddr))
        {
            current = current->next;
        }

        if (current->next
            && Address_IsSameAddress(&current->next->destAddr,&destAddr))
        {
            // Got the matched destination so return the packet
            toFree = current->next;
            if (IPV6)
            {
                SetIPv6AddressInfo(previousHop,
                                    toFree->previousHop.interfaceAddr.ipv6);
            }
            else
            {
                SetIPv4AddressInfo(previousHop,
                                    toFree->previousHop.interfaceAddr.ipv4);
            }
            pktToDest = toFree->msg;
            buffer->numByte -= MESSAGE_ReturnPacketSize(toFree->msg);
            current->next = toFree->next;
            MEM_free(toFree);
            --(buffer->size);
        }
    }

    return pktToDest;
}

// /**
// FUNCTION : AodvIncreaseTtl
// LAYER    : NETWORK
// PURPOSE  : Increase the TTL value of a destination, to which rreq has
//            been sent
// PARAMETERS:
// +aodv:AodvData*: Pointer to Aodv main structure
// +current:AodvRreqSentNode*:The entry in sent, for which the ttl to be
//                            incremented
// RETURN   ::void:NULL
// **/

static
void AodvIncreaseTtl(
         AodvData* aodv,
         AodvRreqSentNode* current)
{
    current->ttl += AODV_TTL_INCREMENT;

    if (current->ttl > AODV_TTL_THRESHOLD)
    {
        // over ttl threshold ttl will be net diameter
        current->ttl = AODV_NET_DIAMETER;
    }
    return;
}


// /**
// FUNCTION : AodvGetSeq
// LAYER    : NETWROK
// PURPOSE  : Obtains the sequence number of the destination node, if there
//            is any entry in the routing table for the destination. If
//            there is no entry for the destination a sequence number 0 is
//            returned
// PARAMETERS:
// +destAddr:Address:The destination address for which a sequence number
//                   is wanted.
// +routeTable:AodvRoutingTable*:Pointer to aodv routing table.
// RETURN   :
// +sequence number:unsigned int:Sequence number If there is one existing
//                               expired or invalid route, 0 otherwise.
// **/

static
unsigned int AodvGetSeq(
                 Address destAddress,
                 AodvRoutingTable* routeTable)
{
    AodvRouteEntry* current = NULL;
    if (destAddress.networkType == NETWORK_IPV6)
    {
        current = routeTable->routeHashTable[destAddress.AODV_Ip6HostBit
                                               % AODV_ROUTE_HASH_TABLE_SIZE];
    }
    else
    {
        current = routeTable->routeHashTable[destAddress.interfaceAddr.ipv4
                                              % AODV_ROUTE_HASH_TABLE_SIZE];
    }

    // Skip entries with smaller destination address
    while (current && AodvIsSmallerAddress(current->destination, destAddress))
    {
        current = current->hashNext;
    }

    if (current && Address_IsSameAddress(&current->destination,&destAddress))
    {
        // Got the desired destination
        return current->destSeqNum;
    }
    else
    {
        // No entry for the destination so return 0
        return 0;
    }
}


// /**
// FUNCTION : AodvFreePrecursorsTable
// LAYER    : NETWOK
// PURPOSE  : To free the precursors list of a route
// PARAMETERS:
// +precursors:AodvPrecursors*:Pointer to the precursors table of a route
// RETURN   ::void:NULL
// **/

static
void AodvFreePrecursorsTable(AodvPrecursors* precursors)
{
    AodvPrecursorsNode* current = precursors->head;
    AodvPrecursorsNode* next = NULL;

    // Delete all the entries in the precursors table
    while (current != NULL)
    {
        next = current->next;
        MEM_free(current);
        current = next;
    }

    // Reinitialize the precursors table
    precursors->head = NULL;
    precursors->size = 0;
}

// /**
// FUNCTION   :: AodvDeleteFromPrecursorsTable
// LAYER      :: NETWORK
// PURPOSE    :: To delete a particular destination from the precursors table.
// PARAMETERS ::
//  +address:  Address : Precursor address.
//  +precursors:  AodvPrecursors* : Pointer to precorsor table.
// RETURN     :: void : NULL.
// **/
static
void AodvDeleteFromPrecursorsTable(
         Address address,
         AodvPrecursors* precursors)
{
    AodvPrecursorsNode* toFree = NULL;
    AodvPrecursorsNode* current = precursors->head;

    if (!current)
    {
        // The precursors table is empty, nothing to do
    }
    else if (Address_IsSameAddress(&current->destAddr, &address))
    {
        // The first entry is the desired entry
        toFree = current;
        precursors->head = toFree->next;

        MEM_free(toFree);

        --(precursors->size);
    }
    else
    {
        // Search for the entry
        while (current->next
                && AodvIsSmallerAddress(current->next->destAddr, address))
        {
            current = current->next;
        }

        if (current->next
            && Address_IsSameAddress(&current->next->destAddr,&address))
        {
            // Got the desired entry, delete the entry
            toFree = current->next;
            current->next = toFree->next;

            MEM_free(toFree);

            --(precursors->size);
        }
    }
}


// /**
// FUNCTION :: AodvDeleteFromPrecursor
// LAYER    ::NETWROK
// PURPOSE  :: for deleting an address from the precursor's list of
//            all the routes
// PARAMETERS::
//  +address:Address:Address to be deleted
//  +routeTable:AodvRoutingTable*:Pointer to the routing table
// RETURN   ::void: NULL
// **/

static
void AodvDeleteFromPrecursor(
         Address address,
         AodvRoutingTable* routeTable)
{
    AodvRouteEntry* current = NULL;
    int i = 0;

    for (i = 0; i < AODV_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (current = routeTable->routeHashTable[i];
             current != NULL;
             current = current->hashNext)
        {
            // For all the entries in the routing table delete the address from
            // its precursors list
            AodvDeleteFromPrecursorsTable(
                address,
                &current->precursors);
        }
    }
}


// /**
// FUNCTION : AodvDisableRoute
// LAYER    : NETWROK
// PURPOSE  : disabling an active route
// ARGUMENTS:
//  +node:Node*: Pointer to the node disabling the route
//  +current:AodvRouteEntry*:Pointer to the current entry of the routing
//                           table
//  +routeTable:AodvRoutingTable*:Pointer to aodv routing table
//  +incrementSeq:BOOL:Flag for determining if seq number to be updated
// RETURN   ::void:NULL
// **/

static
unsigned int AodvDisableRoute(
                 Node* node,
                 AodvRouteEntry* current,
                 AodvRoutingTable* routeTable,
                 BOOL incrementSeq)
{

    AodvData* aodv = NULL;
    BOOL IPV6 = FALSE;

    if (current->destination.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
            node,
            ROUTING_PROTOCOL_AODV6,
            NETWORK_IPV6);
        IPV6 = TRUE;
    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
            node,
            ROUTING_PROTOCOL_AODV,
            NETWORK_IPV4);
    }
    // Got the destination disable it by making the hop count
    // infinity

    ERROR_Assert(current->activated == TRUE,
                 "AODV:  Route should be activated, but it is not.\n");

    // Empty the precursors table for the route
    AodvFreePrecursorsTable(&current->precursors);


    // Copy the hop count field in the last hop count
    current->lastHopCount = current->hopCount;
    current->hopCount = AODV_INFINITY;

    current->activated = FALSE;

    // Set timer to delete the route after delete period
    current->lifetime = node->getNodeTime() + AODV_DELETE_PERIOD;

    if (incrementSeq)
    {
        // Increment the destination sequence number field
        current->destSeqNum++;
    }

    if (routeTable->routeExpireHead == routeTable->routeExpireTail)
    {
        routeTable->routeExpireHead = NULL;
        routeTable->routeExpireTail = NULL;
    }
    else if (routeTable->routeExpireHead == current)
    {
        routeTable->routeExpireHead = current->expireNext;
        routeTable->routeExpireHead->expirePrev = NULL;
    }
    else if (routeTable->routeExpireTail == current)
    {
        routeTable->routeExpireTail = current->expirePrev;
        routeTable->routeExpireTail->expireNext = NULL;
    }
    else
    {
        current->expireNext->expirePrev = current->expirePrev;
        current->expirePrev->expireNext = current->expireNext;
    }

    current->expireNext = NULL;
    current->expirePrev = NULL;

    if (routeTable->routeDeleteTail == NULL
        && routeTable->routeDeleteHead == NULL)
    {
        routeTable->routeDeleteHead = current;
        routeTable->routeDeleteTail = current;
        current->deleteNext = NULL;
        current->deletePrev = NULL;
    }
    else
    {
        current->deletePrev = routeTable->routeDeleteTail;
        current->deleteNext = NULL;
        routeTable->routeDeleteTail->deleteNext = current;
        routeTable->routeDeleteTail = current;
    }

    // An expired routing table entry SHOULD NOT be expunged before
    // (current_time + DELETE_PERIOD) (see section 8.13).  Otherwise,
    // the soft state corresponding to the route (e.g., Last Hop
    // Count) will be lost. Sec: 8.4

    if (AODV_DEBUG)
    {
        printf("Node %u is setting timer MSG_NETWORK_DeleteRoute\n",
            node->nodeId);
    }


    if (aodv->isDeleteTimerSet == FALSE)
    {
        aodv->isDeleteTimerSet = TRUE;

        AodvSetTimer(
            node,
            MSG_NETWORK_DeleteRoute,
            current->destination,
            (clocktype) AODV_DELETE_PERIOD);
    }

    if (AODV_DEBUG_ROUTE_TABLE)
    {
        char time[MAX_STRING_LENGTH];
        char address[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            &current->destination,
            address);

        TIME_PrintClockInSecond(node->getNodeTime(), time);

        printf("Node %u disabled route to %s at %s\n", node->nodeId,
            address, time);

        AodvPrintRoutingTable(node, &aodv->routeTable);
    }

    return current->destSeqNum;
}


// /**
// FUNCTION : AodvGetTimes
// LAYER    : NETWORK
// PURPOSE  : Obtains the number of times the RREQ was sent in
//            TTL = NET_DIAMETER
// PARAMETRS:
//  +destAddr:Address:Destination address for which to know
//            the number of times rreq sent with ttl = net
//            diameter
//  +sent:AodvRreqSentTable*: Pointer to the list where information about
//                            rreq message was sent
// RETURN   :
// +times:int:Number of times
// **/

static
int AodvGetTimes(
        Address destAddr,
        AodvRreqSentTable* sent)
{
    AodvRreqSentNode* current = NULL;

    if (destAddr.networkType == NETWORK_IPV6)
    {
        current = sent->sentHashTable[destAddr.AODV_Ip6HostBit
                                            % AODV_SENT_HASH_TABLE_SIZE];
    }
    else
    {
        current = sent->sentHashTable[destAddr.interfaceAddr.ipv4
                                             % AODV_SENT_HASH_TABLE_SIZE];
    }
    // Skip smaller destinations
    while (current && AodvIsSmallerAddress(current->destAddr, destAddr))
    {
        current = current->hashNext;
    }

    if (current && Address_IsSameAddress(&current->destAddr,&destAddr))
    {
        // Got the destination, return the number of times
        return current->times;
    }
    else
    {
        // No existence of the destination return 0
        return 0;
    }
}

// /**
// FUNCTION : AodvUpdateLifetime
// LAYER    : NETWORK
// PURPOSE  : Update the lifetime field of the destination entry in the route
//            table
// ARGUMENTS:
//  +node:Node*:Pointer toNode
//  +aodv:AodvData*: Pointer to data structure for Aodv internal variables
//  +destAddr:Address: The destination for which the life time to be
//                     updated
//  +routeTable:AodvRoutingTable*:Pointer to Aodv routing table
//  +hopcount:int: HopCount value
// RETURN   ::void:NULL
// **/

static
void AodvUpdateLifetime(
         Node* node,
         AodvData* aodv,
         Address destAddr,
         AodvRoutingTable* routeTable,
         int hopCount)
{

    AodvRouteEntry* current = NULL;

    if ((destAddr.networkType != NETWORK_INVALID) )
    {
    if (destAddr.networkType == NETWORK_IPV6)
    {
        current=routeTable->routeHashTable
            [destAddr.AODV_Ip6HostBit % AODV_ROUTE_HASH_TABLE_SIZE];

    }
    else
    {
        current=routeTable->routeHashTable[destAddr.interfaceAddr.ipv4
                                             % AODV_ROUTE_HASH_TABLE_SIZE];

   }
    }
    else
    {
         //do nothing
         ERROR_ReportWarning(
                           "Invalid Previous Hop Address (Target Address)!");
         return;
    }
    //end
    // Skip entries with smaller destination address
    while (current && AodvIsSmallerAddress(current->destination,destAddr))
    {
        current = current->hashNext;
    }



    if (current && Address_IsSameAddress(&current->destination, &destAddr))
    {
        if (current->activated == TRUE)
        {
            if (current->lifetime < node->getNodeTime()
                + AODV_ACTIVE_ROUTE_TIMEOUT)
            {
                current->lifetime = node->getNodeTime()
                    + AODV_ACTIVE_ROUTE_TIMEOUT;


                AodvMoveRouteEntry(routeTable, current);

                if (AODV_DEBUG)
                {
                    printf("Node %u is setting timer "
                        "MSG_NETWORK_CheckRouteTimeout\n", node->nodeId);
                }
            }
        }
        else
        {
            current->activated = TRUE;
            current->lifetime = node->getNodeTime()
                + AODV_ACTIVE_ROUTE_TIMEOUT;

            if (routeTable->routeDeleteHead == routeTable->routeDeleteTail)
            {
                routeTable->routeDeleteHead = NULL;
                routeTable->routeDeleteTail = NULL;
            }
            else if (routeTable->routeDeleteHead == current)
            {
                routeTable->routeDeleteHead = current->deleteNext;
                routeTable->routeDeleteHead->deletePrev = NULL;
            }
            else if (routeTable->routeDeleteTail == current)
            {
                routeTable->routeDeleteTail = current->deletePrev;
                routeTable->routeDeleteTail->deleteNext = NULL;
            }
            else
            {
                current->deleteNext->deletePrev = current->deletePrev;
                current->deletePrev->deleteNext = current->deleteNext;
            }
            if (hopCount != -1)
            {
                current->hopCount = hopCount;
            }
            else
            {
                current->hopCount = current->lastHopCount;
            }

            current->deleteNext = NULL;
            current->deletePrev = NULL;

            AodvPlaceRouteEntry(routeTable,current);
        }
    }
}


// /**
// FUNCTION : AodvGetNextHop
// LAYER    : NETWORK
// PURPOSE  : Looks up the routing table to obtain next hop to the destination
// ARGUMENTS:
//  +destAddr:Address:The destination address for which next hop is wanted
//  +routeTable:AodvRoutingTable*:Pointer to Aodv routing table
//  +nextHop:Address*:Pointer to be assigned the next hop address
//  +interfaceIndex:int*:Pointer to the interface index through which the
//                       message is to be sent
// RETURN   :
//  +TRUE:BOOL:If one route is found
//  +FALSE:BOOL: otherwise
// **/

static
BOOL AodvGetNextHop(
         Address destAddr,
         AodvRoutingTable* routeTable,
         NodeAddress* nextHop,
         int* interfaceIndex)
{
    BOOL IPV6 = FALSE;
    AodvRouteEntry* current = NULL;
    if (destAddr.networkType == NETWORK_IPV6)
    {
        IPV6 = TRUE;
        current = routeTable->routeHashTable[destAddr.AODV_Ip6HostBit
                                              % AODV_ROUTE_HASH_TABLE_SIZE];
    }
    else
    {
        current = routeTable->routeHashTable[destAddr.interfaceAddr.ipv4
                                              % AODV_ROUTE_HASH_TABLE_SIZE];
    }

    *interfaceIndex = -1;
    *nextHop = 0;

    while (current
            && AodvIsSmallerAddress(current->destination,
                                        destAddr))
    {
        current = current->hashNext;
    }

    if (current &&
        (Address_IsSameAddress(&current->destination, &destAddr)
            && (current->activated == TRUE)))
    {
        // Got the route and this is a valid route
        // Assign next hop and interface index
        if (!IPV6)
        {
            *nextHop = GetIPv4Address(current->nextHop);
        }

        *interfaceIndex = current->outInterface;
        return TRUE;
    }
    else
    {
        // The entry doesn't exist
        return FALSE;
    }
}


// **
// FUNCTION : AodvCheckRouteExist
// LAYER    : NETWORK
// PURPOSE  : To check whether route to a particular destination exist.
//            this function serves dual purpose, in case of invalid route it
//            return the pointer to the route with setting isValid flag to
//            FALSE. And in case of valid routes it returns the valid route
//            pointer with setting the isValid flag to TRUE
// ARGUMENTS:
//  +destAddr:Address:Destination address of the packet
//  +routeTable:AodvRoutingTable*: Pointer to aodv routing table to store
//                                   possible routes
//  +isValid:BOOL*:to return if the route is a valid route or invalid
//                 route
// RETURN   :
//  +current:AodvRouteEntry*:pointer to the route if it exists in the
//                           routing table, NULL otherwise
// **/

static
AodvRouteEntry* AodvCheckRouteExist(
                    Address destAddr,
                    AodvRoutingTable* routeTable,
                    BOOL* isValid)
{
    AodvRouteEntry* current = NULL;
    if (destAddr.networkType == NETWORK_IPV6)
    {
        current = routeTable->
                        routeHashTable
                        [destAddr.AODV_Ip6HostBit
                            % AODV_ROUTE_HASH_TABLE_SIZE];
    }
    else
    {
        current = routeTable->
                    routeHashTable
                        [destAddr.interfaceAddr.ipv4 %
                                AODV_ROUTE_HASH_TABLE_SIZE];

    }

    while (current && AodvIsSmallerAddress(current->destination, destAddr))
    {
            current = current->hashNext;
    }


    *isValid = FALSE;

    if (current && Address_IsSameAddress(&current->destination,&destAddr))
    {
        // Found the entry
        if (current->activated == TRUE)
        {
            // The entry is a valid route
            *isValid = TRUE;
        }
        return current;
    }
    else
    {
        // The entry doesn't exists
        return NULL;
    }
}


// /**
// FUNCTION : AodvInsertPrecursorsTable
// LAYER    : NETWORK
// PURPOSE  : Adding a destination in the precursors list
// ARGUMENTS:
//  +destAddr:Address:The address to insert
//  +precursors:AodvPrecursors*:Pointer to the precursors table
// RETURN   ::void:NULL
// **/

static
void AodvInsertPrecursorsTable(
         Address destAddr,
         AodvPrecursors* precursors)
{
    AodvPrecursorsNode* current = NULL;
    AodvPrecursorsNode* previous = NULL;

    AodvPrecursorsNode* newNode = NULL;

    // Find Insertion point.  Insert after all address matches.
    previous = NULL;
    current = precursors->head;
    BOOL IPV6 = FALSE;
    if (destAddr.networkType == NETWORK_IPV6)
    {
        IPV6 = TRUE;
    }

    while (current && AodvIsSmallerAddress(current->destAddr,destAddr))
    {
        previous = current;
        current = current ->next;
    }

    if (current && Address_IsSameAddress(&current->destAddr, &destAddr))
    {
        // The entry is already there so exit without
        // adding a new entry
    }
    else
    {
        // At this point we are adding the node into the precursors list.
        // We need to modify the sequence number for the destination.

        ++(precursors->size);

        newNode = (AodvPrecursorsNode *)
                      MEM_malloc(sizeof(AodvPrecursorsNode));
        if (IPV6)
        {
            SetIPv6AddressInfo(&newNode->destAddr,
                                destAddr.interfaceAddr.ipv6);
        }
        else
        {
            SetIPv4AddressInfo(&newNode->destAddr,
                                 destAddr.interfaceAddr.ipv4);
        }
        newNode->next = current;

        if (previous == NULL)
        {
            precursors->head = newNode;
        }
        else
        {
            previous->next = newNode;
        }
    }
}


// /**
// FUNCTION : AodvInsertBlacklistTable
// LAYER    : NETWORK
// PURPOSE  : Adding a destination in black list nodes table
//            the blacklist table works when a node fails to send rrep to a
//            particular node. The node then marks the next hop towards the
//            destination of the rrep as black listed and ignores all the
//            rreq's received from any node in its blacklist
// ARGUMENTS:
//  +node:Node*: Pointer to the node inserting into black list table
//  +aodv:AodvData*:Pointe to aodv main data structure
//  +destAddr:Address:The address to insert into blacklistTable
//  +blacklistTable:AodvBlacklistTable*:Blacklist neighbors table
// RETURN   ::void:NULL
// **/

static
void AodvInsertBlacklistTable(
         Node* node,
         AodvData* aodv,
         Address destAddr,
         AodvBlacklistTable* blacklistTable)
{
    AodvBlacklistNode* current = blacklistTable->head;
    AodvBlacklistNode* previous = NULL;

    AodvBlacklistNode* newNode = NULL;

    // Find Insertion point.  Insert after all address matches.
    previous = NULL;
    current = blacklistTable->head;

    while (current && AodvIsSmallerAddress(current->destAddr, destAddr))
    {
        previous = current;
        current = current->next;
    }

    if (current && Address_IsSameAddress(&current->destAddr,&destAddr))
    {
        // The entry is already there in the precursors list so we
        // don't need to add another
    }
    else if (destAddr.networkType != NETWORK_INVALID)
    {
        // At this point we are adding the node into the blacklist.
        ++(blacklistTable->size);

        newNode = (AodvBlacklistNode *)
                      MEM_malloc(sizeof(AodvBlacklistNode));

        if (destAddr.networkType == NETWORK_IPV6)
        {
            SetIPv6AddressInfo(&newNode->destAddr,
                                        destAddr.interfaceAddr.ipv6);
        }
        else
        {
            SetIPv4AddressInfo(&newNode->destAddr,
                                        destAddr.interfaceAddr.ipv4);
        }

        if (AODV_DEBUG)
        {
            printf("Node %u is setting timer MSG_NETWORK_BlacklistTimeout\n",
                node->nodeId);
        }

        AodvSetTimer(
            node,
            MSG_NETWORK_BlacklistTimeout,
            destAddr,
            (clocktype) AODV_BLACKLIST_TIMEOUT);

        if (previous == NULL)
        {
            blacklistTable->head = newNode;
        }
        else
        {
            previous->next = newNode;
        }
        newNode->next = NULL;
    }
}


// /**
// FUNCTION : AodvDeleteFromBlacklistTable
// LAYER    : NETWROK
// PURPOSE  : Deleting one entry from blacklist table
// ARGUMENTS:
//  +address:Address:The address to be deleted
//  +blacklistTable:AodvBlacklistTable*:Blacklist neighbors table
// RETURN   ::void:NULL
// **/

static
void AodvDeleteFromBlacklistTable(
         Address address,
         AodvBlacklistTable* blacklistTable)
{
    AodvBlacklistNode* toFree = NULL;
    AodvBlacklistNode* current = blacklistTable->head;

    if (!current)
    {
        // Nothing to do, no entry in the table
    }
    else if (Address_IsSameAddress(&current->destAddr, &address))
    {
        // The first entry in the list is the matching
        toFree = blacklistTable->head;
        blacklistTable->head = toFree->next;
        MEM_free(toFree);
        --(blacklistTable->size);
    }
    else
    {
        // Search for the address and delete it
        while (current->next
                && AodvIsSmallerAddress(current->next->destAddr,address))
        {
            current = current->next;
        }

        if (current->next
            && Address_IsSameAddress(&current->next->destAddr,&address))
        {
            toFree = current->next;
            current->next = toFree->next;
            MEM_free(toFree);
            --(blacklistTable->size);
        }
    }
}


// /**
// FUNCTION : AodvCheckBlacklistTable
// LAYER    : NETWORK
// PURPOSE  : Returns TRUE if the destination is in Blacklist
// ARGUMENTS:
//  +destAddr:Address:Destination address to search
//  +blacklistTable:AodvBlacklistTable*:Pointer to blacklistTable
// RETURN   :
//  +TRUE:BOOL:If address exists
//  +FALSE:BOOL:otherwise
// **/

static
BOOL AodvCheckBlacklistTable(
         Address destAddr,
         AodvBlacklistTable* blacklistTable)
{
    AodvBlacklistNode* current = blacklistTable->head;

    while (current && AodvIsSmallerAddress(current->destAddr,destAddr))
    {
        current = current->next;
    }

    if (current && Address_IsSameAddress(&current->destAddr,&destAddr))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


// /**
// FUNCTION : AodvCheckSent
// LAYER    : NETWORK
// PURPOSE  : Check if RREQ has been sent; return TRUE if sent
// ARGUMENTS:
//  +destAddress:Address:destination address of the packet sent
//  +sent:AodvRreqSentTable*:Pointer to the structure to mark the packets
//                           for which RREQ has been sent
// RETURN   :
//  +AodvRreqSentNode*:Pointer to the sent node if exists, null otherwise
// **/

static
AodvRreqSentNode* AodvCheckSent(
                      Address destAddr,
                      AodvRreqSentTable* sent)
{

    AodvRreqSentNode* current = NULL;
    if (destAddr.networkType == NETWORK_IPV6)
    {
        int queue = destAddr.interfaceAddr.ipv6.s6_addr32[3] %
                                                AODV_SENT_HASH_TABLE_SIZE;
        current = sent->sentHashTable[queue];
    }
    else
    {
        current = sent->sentHashTable
                  [destAddr.interfaceAddr.ipv4 % AODV_SENT_HASH_TABLE_SIZE];

    }
    while (current && AodvIsSmallerAddress(current->destAddr, destAddr))
    {
        current = current->hashNext;
    }

    if (current && Address_IsSameAddress(&current->destAddr,&destAddr))
    {
        return current;
    }


    return NULL;

}


// /**
// FUNCTION : AodvInsertSent
// LAYER    : NETWORK
// PURPOSE  :  Insert an entry into the sent table if RREQ is sent
// ARGUMENTS:
//  +destAddr:Address:The destination address for which the rreq has been
//                    sent
//  +ttl:int:The time to leave of the rreq
//  +sent:AodvRreqSentTable*:The structure to store information about the
//                           destinations for which rreq has been sent
// RETURN   :
//  +AodvRreqSentNode*:The node just inserted
//**/

static
AodvRreqSentNode* AodvInsertSent(
                      Address destAddr,
                      int ttl,
                      AodvRreqSentTable* sent)
{
    int queueNo = 0;
    BOOL IPV6 = FALSE;
    if (destAddr.networkType == NETWORK_IPV6)
    {
        queueNo = destAddr.interfaceAddr.ipv6.s6_addr32[3] %
                                                AODV_SENT_HASH_TABLE_SIZE;
        IPV6 = TRUE;
    }
    else
    {
        queueNo = destAddr.interfaceAddr.ipv4 % AODV_SENT_HASH_TABLE_SIZE;
    }

    AodvRreqSentNode* current = sent->sentHashTable[queueNo];

    AodvRreqSentNode* previous = NULL;

    // Find Insertion point.  Insert after all address matches.
    // To make the list sorted in ascending order
    while (current && AodvIsSmallerAddress(current->destAddr,destAddr))
    {
        previous = current;
        current = current->hashNext;
    }

    if (current && Address_IsSameAddress(&current->destAddr,&destAddr))
    {
        // The entry already exists so nothing to do
        return current;
    }
    else
    {
        AodvRreqSentNode* newNode = (AodvRreqSentNode *)
            MEM_malloc(sizeof(AodvRreqSentNode));

        if (IPV6)
        {
            SetIPv6AddressInfo(&newNode->destAddr,
                                destAddr.interfaceAddr.ipv6);
        }
        else
        {
               SetIPv4AddressInfo(&newNode->destAddr,
                                destAddr.interfaceAddr.ipv4);
        }
        newNode->ttl = ttl;
        newNode->times = 0;
        newNode->hashNext = current;

        (sent->size)++;

        if (previous == NULL)
        {
            sent->sentHashTable[queueNo] = newNode;
        }
        else
        {
            previous->hashNext = newNode;
        }
        return newNode;
    }
}



// /**
// FUNCTION   :: AodvDeleteSent
// LAYER      :: NETWORK
// PURPOSE    :: Remove an entry from the sent table.
// PARAMETERS ::
//  +destAddr:  Address : Destination Address.
//  +sent:  AodvRreqSentTable* : Pointer to sent table.
// RETURN     :: void : NULL.
// **/
static
void AodvDeleteSent(
         Address destAddr,
         AodvRreqSentTable* sent)
{
    int queueNo = -1;

    AodvRreqSentNode* toFree = NULL;

    AodvRreqSentNode* current = NULL;

    if (destAddr.networkType == NETWORK_IPV6)
    {
        queueNo = destAddr.AODV_Ip6HostBit % AODV_SENT_HASH_TABLE_SIZE;
        current = sent->sentHashTable[queueNo];
    }
    else
    {
        queueNo = destAddr.interfaceAddr.ipv4 % AODV_SENT_HASH_TABLE_SIZE;
        current = sent->sentHashTable[queueNo];
    }

    if (!current)
    {
        // Table is empty so nothing to do
    }
    else if (Address_IsSameAddress(&current->destAddr, &destAddr))
    {
        toFree = current;
        sent->sentHashTable[queueNo] = toFree->hashNext;

        MEM_free(toFree);

        --(sent->size);
    }
    else
    {
        while (current->hashNext
                && AodvIsSmallerAddress(
                    current->hashNext->destAddr,
                    destAddr))
        {
            current = current->hashNext;
        }

        if (current->hashNext
            && Address_IsSameAddress(&current->hashNext->destAddr,&destAddr))
        {
            toFree = current->hashNext;
            current->hashNext = toFree->hashNext;

            MEM_free(toFree);

            --(sent->size);
        }
    }
}


// /**
// FUNCTION : AodvInsertSeenTable
// LAYER    : NETWORK
// PURPOSE  : Insert an entry into the seen table
// ARGUMENTS:
//  +node:Node*:Pointer to the node which is inserting into the table
//  +srcAddr:Address:The source address of RREQ packet
//  +floodingId:unsigned int: The flooding Id in the RREQ from the source
//  +seenTable: Pointer to the table to store source and broadcast id pair
// RETURN   ::void:NULL
// **/

static
void AodvInsertSeenTable(
         Node* node,
         Address srcAddr,
         unsigned int floodingId,
         AodvRreqSeenTable* seenTable)
{
    AodvData* aodv = NULL;
    BOOL IPV6 = FALSE;
    Address broadcastAddress;
    if (srcAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(node,
                                ROUTING_PROTOCOL_AODV6,
                                NETWORK_IPV6);
        IPV6 = TRUE;
    }
    else
    {
        aodv = (AodvData* ) NetworkIpGetRoutingProtocol(node,
                                       ROUTING_PROTOCOL_AODV,
                                       NETWORK_IPV4);
    }


    // Always add in the rear of the list and send one timer for expire.
    // In time of deletion, it will be always from the front
    if (seenTable->size == 0)
    {
        seenTable->rear = (AodvRreqSeenNode *)
            MEM_malloc(sizeof(AodvRreqSeenNode));
        seenTable->rear->previous = NULL;
        seenTable->front = seenTable->rear;
    }
    else
    {
        seenTable->rear->next = (AodvRreqSeenNode *)
            MEM_malloc(sizeof(AodvRreqSeenNode));
        seenTable->rear->next->previous = seenTable->rear;
        seenTable->rear = seenTable->rear->next;
    }
    if (IPV6)
    {
        SetIPv6AddressInfo(
            &seenTable->rear->srcAddr,
            srcAddr.interfaceAddr.ipv6);
        SetIPv6AddressInfo(
            &broadcastAddress,
            aodv->broadcastAddr.interfaceAddr.ipv6);
    }
    else
    {
        SetIPv4AddressInfo(&seenTable->rear->srcAddr,
                            srcAddr.interfaceAddr.ipv4);
        SetIPv4AddressInfo(&broadcastAddress, ANY_IP);
    }
    seenTable->rear->floodingId = floodingId;
    seenTable->rear->next = NULL;

    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&srcAddr, address);
        printf("Node %u is adding to seen table(%d), Source Address: %s,"
                " Flood ID: %d \n",
                node->nodeId,
                aodv->seenTable.size,
                address,
                floodingId);
    }

    ++(seenTable->size);

    // Keep track of the seen tables max size
    aodv->stats.numMaxSeenTable = MAX((unsigned int) seenTable->size,
                                       aodv->stats.numMaxSeenTable);

    if (AODV_DEBUG)
    {
        printf("Node %u is setting timer MSG_NETWORK_FlushTables\n",
            node->nodeId);
    }

    AodvSetTimer(
        node,
        MSG_NETWORK_FlushTables,
        broadcastAddress,
        (clocktype) AODV_FLOOD_RECORD_TIME);
}


// /**
// FUNCTION : AodvLookupSeenTable
// LAYER    : NETWORK
// PURPOSE  : Returns TRUE if the broadcast packet is processed before
// ARGUMENTS:
//  +aodv:AodvData*:Pointer to aodv data
//  +srcAddr:Address:Source of RREQ
//  +floodingId:unsigned int:FloodingId id in the received RREQ
//  +seenTable:AodvLookupSeenTable*:pointer to table where information of
//                                  seen RREQ's has been stored
// RETURN   ::void:NULL
// **/

static
BOOL AodvLookupSeenTable(
         AodvData* aodv,
         Address srcAddr,
         unsigned int floodingId,
         AodvRreqSeenTable* seenTable)
{
    AodvRreqSeenNode* current = NULL;

    if (seenTable->size == 0)
    {
        return FALSE;
    }

    // Check if the last found node from table matches.
    if ((seenTable->lastFound ) &&
        (Address_IsSameAddress(&seenTable->lastFound->srcAddr,&srcAddr))
        &&(seenTable->lastFound->floodingId == floodingId))
    {
        aodv->stats.numLastFoundHits++;
        return TRUE;
    }

    // Traverses the list from the rear
    // to check the most recent entries first
    for (current = seenTable->rear;
         current != NULL;
         current = current->previous)
    {
        if (Address_IsSameAddress(&current->srcAddr,&srcAddr) &&
            (current->floodingId == floodingId))
        {
            seenTable->lastFound = current; // Remember the last found node
            return TRUE;
        }
    }

    return FALSE;

}

// /**
// FUNCTION   :: AodvDeleteSeenTable
// LAYER      :: NETWORK
// PURPOSE    :: Remove an entry from the seen table, deletion will be always
//            from the front of the table and insertion from the rear.
// PARAMETERS ::
//  +seenTable:  AodvRreqSeenTable* : Pointer to  seen table.
// RETURN     :: void : NULL.
// **/
static
void AodvDeleteSeenTable(AodvRreqSeenTable* seenTable)
{
    AodvRreqSeenNode* toFree = NULL;

    toFree = seenTable->front;
    seenTable->front = toFree->next;

    // Clean up lastfound if it is going to be freed
    if (seenTable->lastFound == toFree){
        seenTable->lastFound = NULL;
    }

    MEM_free(toFree);

    --(seenTable->size);

    if (seenTable->size == 0)
    {
        seenTable->rear = NULL;
    }
    else
    {
        // Update the previous node link to NULL
        // (We're at the front of the list)
        seenTable->front->previous = NULL;
    }
}


// /**
// FUNCTION   :: AodvReplaceInsertRouteTable
// LAYER      :: NETWORK
// PURPOSE    :: Insert/Update an entry into the route table.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +destAddr:  Address : Dest Address.
//  +destSeq:  UInt32 : Dest Sequence Number.
//  +hopCount:  Int32 : Dest hop Count.
//  +nextHop:  Address : Next Hop.
//  +lifetime:  clocktype : Life time of route.
//  +activated:  BOOL : Is route activated or not.
//  +interfaceIndex:  int : Interface Index.
//  +routeTable:  AodvRoutingTable* : Pointer to Route Table.
// RETURN     :: void : NULL.
// **/
static
AodvRouteEntry* AodvReplaceInsertRouteTable(
                    Node* node,
                    Address destAddr,
                    UInt32 destSeq,
                    Int32 hopCount,
                    Address nextHop,
                    clocktype lifetime,
                    BOOL activated,
                    int  interfaceIndex,
                    AodvRoutingTable* routeTable)
{

    AodvData* aodv = NULL;
    BOOL IPV6 = FALSE;
    AodvRouteEntry* theNode = NULL;
    AodvRouteEntry* previous = NULL;
    AodvRouteEntry* current = NULL;
    int queueNo;

    if (destAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData* )
                  NetworkIpGetRoutingProtocol(node,
                  ROUTING_PROTOCOL_AODV6,
                  NETWORK_IPV6);
        IPV6 = TRUE;
        queueNo = destAddr.AODV_Ip6HostBit % AODV_ROUTE_HASH_TABLE_SIZE;
        current = routeTable->routeHashTable[queueNo];
   }
    else
    {
        aodv = (AodvData* )
            NetworkIpGetRoutingProtocol(node,
            ROUTING_PROTOCOL_AODV,
            NETWORK_IPV4);
        queueNo = destAddr.interfaceAddr.ipv4 % AODV_ROUTE_HASH_TABLE_SIZE;
        current = routeTable->routeHashTable[queueNo];

    }


    previous = current;
    while (current && AodvIsSmallerAddress(current->destination,destAddr))
    {
        previous = current;
        current = current->hashNext;
    }


    if (current && Address_IsSameAddress(&current->destination,&destAddr))
    {
        if (current->activated)
        {
            if (current->lifetime < lifetime)
            {
                current->lifetime = lifetime;
                AodvMoveRouteEntry(routeTable,current);
            }
        }
        else
        {
            current->activated = activated;
            current->lifetime = lifetime;

            if (routeTable->routeDeleteHead == routeTable->routeDeleteTail)
            {
                routeTable->routeDeleteHead = NULL;
                routeTable->routeDeleteTail = NULL;
            }
            else if (routeTable->routeDeleteHead == current)
            {
                routeTable->routeDeleteHead = current->deleteNext;
                routeTable->routeDeleteHead->deletePrev = NULL;
            }
            else if (routeTable->routeDeleteTail == current)
            {
                routeTable->routeDeleteTail = current->deletePrev;
                routeTable->routeDeleteTail->deleteNext = NULL;
            }
            else
            {
                current->deleteNext->deletePrev = current->deletePrev;
                current->deletePrev->deleteNext = current->deleteNext;
            }
            current->deleteNext = NULL;
            current->deletePrev = NULL;
            AodvPlaceRouteEntry(routeTable,current);
        }
        theNode = current;
    }
    else
    {
        ++(routeTable->size);

        // Adding a new Entry here
        theNode = AodvMemoryMalloc(aodv);
        memset(theNode, 0,sizeof(AodvRouteEntry));

        theNode->deleteNext = NULL;
        theNode->deletePrev = NULL;

        theNode->lifetime = lifetime;
        theNode->activated = activated;
        if (IPV6)
        {
            SetIPv6AddressInfo(&theNode->destination,
                                   destAddr.interfaceAddr.ipv6);
        }
        else
        {
            SetIPv4AddressInfo(&theNode->destination,
                                   destAddr.interfaceAddr.ipv4);
        }


        theNode->lastHopCount = hopCount;

        ERROR_Assert(theNode->lastHopCount > 0,
            "Last hopcount can not be negetive\v");

        // Need to initialize precursors list
        theNode->precursors.head = NULL;
        theNode->precursors.size = 0;

        if (current == routeTable->routeHashTable[queueNo])
        {
           // First entry in the queue
           theNode->hashNext = current;
           theNode->hashPrev = NULL;
           routeTable->routeHashTable[queueNo] = theNode;

           if (current)
           {
               current->hashPrev = theNode;
           }
        }
        else
        {
           theNode->hashNext = current;
           theNode->hashPrev = previous;
           previous->hashNext = theNode;
           if (current)
           {
                current->hashPrev = theNode;
           }
        }
        AodvPlaceRouteEntry(routeTable, theNode);
    }

    theNode->locallyRepairable = FALSE;
    theNode->destSeqNum = destSeq;
    theNode->hopCount = hopCount;
    if (IPV6)
    {
        SetIPv6AddressInfo(&theNode->nextHop, nextHop.interfaceAddr.ipv6);

    }
    else
    {
        SetIPv4AddressInfo(&theNode->nextHop, nextHop.interfaceAddr.ipv4);
    }
    theNode->outInterface = interfaceIndex;

    if (AODV_DEBUG_ROUTE_TABLE)
    {
        char address[MAX_STRING_LENGTH];
        char time[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), time);
        IO_ConvertIpAddressToString(&destAddr,address);
        printf("After adding or updating entry of %s at %s\n",
            address, time);
        AodvPrintRoutingTable(node, routeTable);
    }

    return theNode;
}


// /**
// FUNCTION : AodvDeleteRouteTable
// LAYER    : NETWORK
// PURPOSE  : Remove an entry from the route table
// ARGUMENTS:
//  +node:Node*:Pointe to the node deleting the route entry
//  +toFree:AodvRouteEntry*:Entry to be freed
//  +routeTable:AodvRoutingTable*:Pointe to Aodv routing table
// RETURN   ::void:NULL
// **/

static
void AodvDeleteRouteTable(
         Node* node,
         AodvRouteEntry* toFree,
         AodvRoutingTable* routeTable)
{


    AodvData* aodv = NULL;

    int queueNo = 0;

    if (toFree->destination.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData* )
            NetworkIpGetRoutingProtocol(node,
            ROUTING_PROTOCOL_AODV6,
            NETWORK_IPV6);
        queueNo = toFree->destination.AODV_Ip6HostBit %
                            AODV_ROUTE_HASH_TABLE_SIZE;
    }
    else
    {
        aodv = (AodvData* )
            NetworkIpGetRoutingProtocol(node,
            ROUTING_PROTOCOL_AODV,
            NETWORK_IPV4);
        queueNo = toFree->destination.interfaceAddr.ipv4 %
                            AODV_ROUTE_HASH_TABLE_SIZE;
    }

    if (routeTable->routeDeleteHead == routeTable->routeDeleteTail)
    {
        routeTable->routeDeleteHead = NULL;
        routeTable->routeDeleteTail = NULL;
    }
    else
    {
        routeTable->routeDeleteHead = toFree->deleteNext;
        routeTable->routeDeleteHead->deletePrev = NULL;
    }

    if (routeTable->routeHashTable[queueNo] == toFree)
    {
        routeTable->routeHashTable[queueNo] = toFree->hashNext;
        if (routeTable->routeHashTable[queueNo])
        {
            routeTable->routeHashTable[queueNo]->hashPrev = NULL;
        }
    }
    else
    {
        toFree->hashPrev->hashNext = toFree->hashNext;

        if (toFree->hashNext)
        {
            toFree->hashNext->hashPrev = toFree->hashPrev;
        }
    }

    if (AODV_DEBUG_ROUTE_TABLE)
        {
            char time[MAX_STRING_LENGTH];
            char address[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&toFree->destination,
                                            address);
            TIME_PrintClockInSecond(node->getNodeTime(), time);

            printf("Node %u deleted Entry to %s at %s\n", node->nodeId,
                 address,time);
            AodvPrintRoutingTable(node, routeTable);
        }

    // Before deleting the entry from the routing table free the
    // precursors list of this route
    AodvFreePrecursorsTable(&toFree->precursors);

    AodvMemoryFree(aodv, toFree);

    --(routeTable->size);
}


// /**
// FUNCTION : AodvCalculateNumInactiveRouteByDest
// LAYER    : NETWROK
// PURPOSE  : After receiving a RERR message, checking which of the
//            destinations in the list of the rerr should be included in
//            the forwarding RERR. With that this function removes the
//            unreachable destination from precursor list of other routes
//            and then disables the route
// ARGUMENTS:
//  +node:Node*:Pointer to the node which has received the rerr
//  +msg:Message*:pointer to the message containing rerr packet
//  +source:Address:Last forwarding node of the RERR
//  +routeTable:AodvRoutingTable*:Aodv routing table
//  +ifOneUpstream:BOOL:True if there is only one node to which the
//                      rreq should be forwarded back.
//  +upsteamAddr:Address*:If there is only one neighbor to which the rerr
//                        message to be forwarded back, the address of
//                        the node.
// RETURN   :
//  DataBuffer:List of addresses which should be included in the forwarded
//            rerr message along with their sequence number
// **/

static
DataBuffer AodvCalculateNumInactiveRouteByDest(
               Node* node,
               Message* msg,
               Address source,
               AodvRoutingTable* routeTable,
               BOOL* ifOneUpstream,
               Address* upstreamAddr)
{
    AodvRerrPacket* rerrPkt = NULL;
    AodvAddrSeqInfo* destAddrSeq = NULL;
    Aodv6AddrSeqInfo * dest6AddrSeq = NULL;
    UInt32 i = 0;
    UInt32 destCount = 0;
    DataBuffer unreachableDestList;
    BOOL isNumUpstreamUnique = TRUE;
    Address previousPrecursorNode;
    BOOL IPV6 = FALSE;
    int sizeofAddrSeqInfo = 0;
    previousPrecursorNode.networkType = NETWORK_INVALID;

    rerrPkt = (AodvRerrPacket *) MESSAGE_ReturnPacket(msg);
    if (source.networkType == NETWORK_IPV6)
    {
        IPV6 = TRUE;
        sizeofAddrSeqInfo = sizeof(Aodv6AddrSeqInfo);
    }
    else
    {
        sizeofAddrSeqInfo = sizeof(AodvAddrSeqInfo);
    }


    BUFFER_InitializeDataBuffer(
        &unreachableDestList,
        sizeofAddrSeqInfo * 5);
    if (IPV6)
    {
        dest6AddrSeq = (Aodv6AddrSeqInfo *)
                  (((char *)rerrPkt) + sizeof(AodvRerrPacket));
    }
    else
    {
        destAddrSeq = (AodvAddrSeqInfo *)
                  (((char *)rerrPkt) + sizeof(AodvRerrPacket));
    }

    if (AODV_DEBUG)
    {
        printf("\tRecived route error with %u unreachable dest\n",
            rerrPkt->typeBitsDestCount & AODV_DEST_COUNT_BITS);
    }

    destCount = rerrPkt->AODV_REER_DestCount;

    // Go through all the unreachable destination list
    for (i = 0; i < destCount; i++)
    {
        AodvRouteEntry* current = NULL;
        BOOL isValidRt = FALSE;
        Address destination;
        unsigned int sequence = 0;

        if (IPV6)
        {
            SetIPv6AddressInfo(&destination, dest6AddrSeq[i].address);

            sequence = dest6AddrSeq[i].seqNum;
        }
        else
        {
            SetIPv4AddressInfo(&destination, destAddrSeq[i].address);

            sequence = destAddrSeq[i].seqNum;
        }



        if (AODV_DEBUG)
        {
            char address[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&destination, address);
            printf("\t\t destination %s\tsequence %u\n",
                address, sequence);
        }

        // Check if a valid route entry exists in the routing table for
        // the unreachable destination

        current = AodvCheckRouteExist(
                      destination,
                      routeTable,
                      &isValidRt);

        if (isValidRt)
        {
            // Valid route entry exists
            if (Address_IsSameAddress(&current->nextHop, &source))
            {
                // Last forwarding node is the next hop of the route
                if (current->precursors.size)
                {
                    // precursors list is not empty
                    AodvAddrSeqInfo unreachableDest;
                    Aodv6AddrSeqInfo unreachable6Dest;

                    if (isNumUpstreamUnique &&
                        (current->precursors.size > 1 ||
                         (BUFFER_GetCurrentSize(&unreachableDestList) &&
                          (!Address_IsSameAddress(&previousPrecursorNode,
                             &current->precursors.head->destAddr)))))
                    {
                        isNumUpstreamUnique = FALSE;
                    }
                    if (IPV6)
                    {
                        SetIPv6AddressInfo(&previousPrecursorNode,
                                            current->precursors.head->
                                                destAddr.interfaceAddr.ipv6);
                        COPY_ADDR6(
                            current->destination.interfaceAddr.ipv6,
                            unreachable6Dest.address);
                        unreachable6Dest.seqNum = sequence;
                    }
                    else
                    {
                        SetIPv4AddressInfo(&previousPrecursorNode,
                                            current->precursors.head->
                                                destAddr.interfaceAddr.ipv4);
                        unreachableDest.address = current->destination.
                                                    interfaceAddr.ipv4;
                        unreachableDest.seqNum = sequence;
                    }


                    current->destSeqNum = sequence;



                    // Add the destination and destination sequence in the
                    // buffer to be forwarded towards upstream
                    if (IPV6)
                    {
                        BUFFER_AddDataToDataBuffer(
                            &unreachableDestList,
                            (char *) &unreachable6Dest,
                            sizeof(Aodv6AddrSeqInfo));
                    }
                    else
                    {
                        BUFFER_AddDataToDataBuffer(
                            &unreachableDestList,
                            (char *) &unreachableDest,
                            sizeof(AodvAddrSeqInfo));
                    }

                }
                else
                {
                    // The precursors list is empty so don't need to include
                    // the destination in the RERR to be forward back.
                    // Just disable the route entry

                    current->destSeqNum = sequence;
                }

                // Disable the route
                AodvDisableRoute(
                    node,
                    current,
                    routeTable,
                    FALSE);
            }
        }
    }
    *ifOneUpstream = isNumUpstreamUnique;
    *upstreamAddr = previousPrecursorNode;
    return unreachableDestList;
}


// /**
// FUNCTION : AodvMarkRouteAsLocallyRepairable
// LAYER    : NETWORK
// PURPOSE  : Mark routes as locally repairable for failure of particular
//            nextHop.
// ARGUMENTS:
//  +node:Node*:Pointer to the node deleting the route entry
//  +aodv:AodvData*:Pointer to the Aodv main data structure
//  +nextHopAddress:Address:Address of the next hop
// RETURN   ::void:NULL
// **/

static
void AodvMarkRouteAsLocallyRepairable(
         Node* node,
         AodvData* aodv,
         Address nextHopAddress)
{
    AodvRouteEntry* current = NULL;

    AodvRoutingTable* routeTable = &aodv->routeTable;
    int i = 0;

    for (i = 0; i < AODV_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (current = routeTable->routeHashTable[i];
             current != NULL;
             current = current->hashNext)
        {
            if ((current->activated == TRUE) &&
                Address_IsSameAddress(&current->nextHop, &nextHopAddress))
            {

                current->locallyRepairable = TRUE;

                // Disable the route
                AodvDisableRoute(
                    node,
                    current,
                    &aodv->routeTable,
                    TRUE);
            }
        }
    }

    // The next hop is not valid so delete it from all routes precursors list
    AodvDeleteFromPrecursor(
        nextHopAddress,
        routeTable);
}


// /**
// FUNCTION : AodvCalculateNumInactiveRouteByNextHop
// LAYER    : NETWORK
// PURPOSE  : To find number of destinations using the next hop as
//            their next hop toward the destination.
// ARGUMENTS:
//  +routeTable:AodvRoutingTable*:Pointer to Aodv routing table
//  +nextHop:Address: nextHop which is not reachable now
//  +numberDestinations:int *:number of routes using the next hop
//  +isUniqueUpstream:BOOL*:If there is only one node to forward back
//                          RERR
//  +upstreamAddr:Address*:If there is one node in the backward
//                         path, its address.
// RETURN   ::void:NULL
// **/

static
void AodvCalculateNumInactiveRouteByNextHop(
         AodvRoutingTable* routeTable,
         Address nextHop,
         int* numberDestinations,
         BOOL* isUniqueUpstream,
         Address* upstreamAddr)
{
    AodvRouteEntry* current = NULL;
    *isUniqueUpstream = TRUE;
    int i;

    // Delete the disable route from all route's precursor list
    AodvDeleteFromPrecursor(
        nextHop,
        routeTable);

    for (i = 0; i < AODV_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (current = routeTable->routeHashTable[i];
             current != NULL;
             current = current->hashNext)
        {
            if (Address_IsSameAddress(&current->nextHop, &nextHop)
                && (current->activated == TRUE))
            {
                if (current->precursors.size)
                {
                    // precursors list is not null so need to advertise the
                    // failure of this route
                    (*numberDestinations)++;

                    // check if there is only one upstream to which the route
                    // error should be sent
                    if (isUniqueUpstream && (current->precursors.size > 1
                        || ((*numberDestinations > 1)
                        && (!Address_IsSameAddress(
                                upstreamAddr,
                                &current->precursors.head->destAddr)))))
                    {
                        *isUniqueUpstream = FALSE;
                    }

                    memcpy(
                        upstreamAddr,
                        &current->precursors.head->destAddr,
                        sizeof(Address));
                }
            }
        }
    }
}


// /**
// FUNCTION : AodvTransmitData
// LAYER    : NETWORK
// PURPOSE  : Forward the data packet to the next hop
// ARGUMENTS:
//  +node:Node*:Pointer to the node which is transmitting or forwarding data
//  +msg:Message*:The packet to be forwarded
//  +rtEntryToDest:AodvRouteEntry*:Pointer to entry to the destination
//  +previousHopAddress:Address:Previous Hop Address
// RETURN   ::void:NULL
// **/
static
void AodvTransmitData(
         Node* node,
         Message* msg,
         AodvRouteEntry* rtEntryToDest,
         Address previousHopAddress)
{
    AodvData* aodv = NULL;
    IpHeaderType* ipHeader = NULL;
    ip6_hdr* ip6Header = NULL;
    Address src;
    Address dst;
    BOOL IPV6 = FALSE;

    if (previousHopAddress.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *)NetworkIpGetRoutingProtocol
                            (node,ROUTING_PROTOCOL_AODV6,
                            NETWORK_IPV6);
        ip6Header = (ip6_hdr *) MESSAGE_ReturnPacket(msg);
        SetIPv6AddressInfo(&src,ip6Header->ip6_src);
        SetIPv6AddressInfo(&dst,
                            rtEntryToDest->destination.interfaceAddr.ipv6);
        IPV6 = TRUE;
    }
    else
    {
        aodv = (AodvData*)NetworkIpGetRoutingProtocol
                           (node, ROUTING_PROTOCOL_AODV,
                           NETWORK_IPV4);
        ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
        SetIPv4AddressInfo(&src,ipHeader->ip_src);
        SetIPv4AddressInfo(&dst,
                            rtEntryToDest->destination.interfaceAddr.ipv4);
    }

    MESSAGE_SetLayer(msg, MAC_LAYER, 0);

    MESSAGE_SetEvent(msg, MSG_MAC_FromNetwork);

    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];
        char clockstr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clockstr);

        IO_ConvertIpAddressToString
            (&rtEntryToDest->destination,
                address);
        printf("Node %u is sending packet\n"
            "\tdestination %s\n", node->nodeId,
            address);
        IO_ConvertIpAddressToString
            (&rtEntryToDest->nextHop,
            address);
        printf("\tsending packet out to next hop %s on interface %u\n",
            address, rtEntryToDest->outInterface);

    }


    // Each time a route is used to forward a data packet, its Lifetime
    // field is updated to be no less than the current time plus
    // ACTIVE_ROUTE_TIMEOUT. Sec: 8.2

    if (rtEntryToDest->lifetime < node->getNodeTime()
        + AODV_ACTIVE_ROUTE_TIMEOUT)
    {
        rtEntryToDest->lifetime = node->getNodeTime() +
                                      AODV_ACTIVE_ROUTE_TIMEOUT;

        if (AODV_DEBUG)
        {
            printf("Node %u is setting timer "
                   "MSG_NETWORK_CheckRouteTimeout\n",
                node->nodeId);
        }

        AodvMoveRouteEntry(&aodv->routeTable, rtEntryToDest);
    }

    // Since the route between each source and
    // destination pair are expected to be symmetric, the Lifetime
    // for the previous hop, along the reverse path back to the IP
    // source, is also updated to be no less than the current time plus
    // ACTIVE_ROUTE_TIMEOUT. Sec 8.2
    if (((previousHopAddress.interfaceAddr.ipv4 != ANY_IP)
          && (previousHopAddress.networkType == NETWORK_IPV4))
          || ((!IS_MULTIADDR6(previousHopAddress.interfaceAddr.ipv6))
               && (previousHopAddress.networkType == NETWORK_IPV6)))
    {
        AodvUpdateLifetime(
            node,
            aodv,
            previousHopAddress,
            &aodv->routeTable,
            1);
    }



    // Update lifetime of source
    if (!Address_IsSameAddress(&previousHopAddress,&src))
    {
        AodvUpdateLifetime(
            node,
            aodv,
            src,
            &aodv->routeTable,
            -1);
    }

    // Update the lifetime of next hop towards the destination.
    AodvUpdateLifetime(
        node,
        aodv,
        rtEntryToDest->nextHop,//dst,
        &aodv->routeTable,
        1);

    if (IPV6)
    {
        Ipv6SendPacketToMacLayer(
            node,
            msg,
            dst.interfaceAddr.ipv6,
            rtEntryToDest->nextHop.interfaceAddr.ipv6,
            rtEntryToDest->outInterface);
    }
    else
    {
        NetworkIpSendPacketToMacLayer(
            node,
            msg,
            rtEntryToDest->outInterface,
            rtEntryToDest->nextHop.interfaceAddr.ipv4);
    }
}

// /**
// FUNCTION   :: AodvIsEligibleInterface
// LAYER      :: NETWORK
// PURPOSE    :: Check whether interface is valid for AODV for IPv4 or IPv6.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +destAddr:  Address* : Pointer to Dest Address.
//  +iface:  AodvInterfaceInfo* : Pointer to AODV Interface.
// RETURN     :: TRUE if Eligible, FALSE otherwise.
// **/
static
BOOL AodvIsEligibleInterface(
    Node* node,
    Address* destAddr,
    AodvInterfaceInfo* iface)
{
    if (((destAddr->networkType == NETWORK_IPV4)
        && (iface->aodv4eligible == TRUE))
        || ((destAddr->networkType == NETWORK_IPV6)
        && (iface->aodv6eligible == TRUE)))
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: AodvSendPacket
// LAYER      :: NETWORK
// PURPOSE    :: Send AODV Packets.
// PARAMETERS ::
//  +node:  Node* : Pointer to Node.
//  +msg:  Message* : Pointer to message.
//  +srcAddr:  Address : Source Address.
//  +destAddr:  Address : Dest Address.
//  +interfaceIndex:  int : Interface Index.
//  +ttl: int : TTL value for the message.
//  +nextHopAddress:  Address : Next Hop used by AODV for IPv4.
//  +delay:  clocktype : Delay used by AODV for IPv4.
// RETURN     :: void : NULL.
// **/
void
AodvSendPacket(
    Node* node,
    Message* msg,
    Address srcAddr,
    Address destAddr,
    int interfaceIndex,
    int ttl,
    NodeAddress nextHopAddress,
    clocktype delay,
    BOOL isDelay)
{

    if (srcAddr.networkType == NETWORK_IPV4)
    {
        if (isDelay)
        {
            //Trace sending packet
            ActionData acnData;
            acnData.actionType = SEND;
            acnData.actionComment = NO_COMMENT;
            TRACE_PrintTrace(node, msg, TRACE_NETWORK_LAYER,
                  PACKET_OUT, &acnData , srcAddr.networkType);


            NetworkIpSendRawMessageToMacLayerWithDelay(
                node,
                msg,
                srcAddr.interfaceAddr.ipv4,
                destAddr.interfaceAddr.ipv4,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_AODV,
                ttl,
                interfaceIndex,
                nextHopAddress,
                delay);
        }
        else
        {
            //Trace sending packet
            ActionData acnData;
            acnData.actionType = SEND;
            acnData.actionComment = NO_COMMENT;
            TRACE_PrintTrace(node, msg, TRACE_NETWORK_LAYER,
                      PACKET_OUT, &acnData, srcAddr.networkType);

            NetworkIpSendRawMessageToMacLayer(
                node,
                msg,
                srcAddr.interfaceAddr.ipv4,
                destAddr.interfaceAddr.ipv4,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_AODV,
                1,
                interfaceIndex,
                nextHopAddress);
        }

    }
    else
    {
        //if (isDelay)
        {
            //Trace sending packet
            ActionData acnData;
            acnData.actionType = SEND;
            acnData.actionComment = NO_COMMENT;
            TRACE_PrintTrace(node, msg, TRACE_NETWORK_LAYER,
                      PACKET_OUT, &acnData , srcAddr.networkType);

           Ipv6SendRawMessageToMac(
                node,
                msg,
                srcAddr.interfaceAddr.ipv6,
                destAddr.interfaceAddr.ipv6,
                interfaceIndex,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_AODV,
                ttl);
        }

    }
}

// /**
// FUNCTION   :: AodvBroadcastHelloMessage
// LAYER      :: NETWORK
// PURPOSE    :: Function to advertise hello message if a node wants to.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +aodv:  AodvData* : Pointer to AODV Data.
// RETURN     :: void : NULL.
// **/
static
void AodvBroadcastHelloMessage(Node* node, AodvData* aodv, Address* destAddr)
{
    IPv6Data *ipv6 = (IPv6Data *) node->networkData.networkVar->ipv6;
    Message* newMsg = NULL;
    AodvRrepPacket* rrepPkt = NULL;
    NetworkRoutingProtocolType protocolType = ROUTING_PROTOCOL_AODV;
    char* pktPtr = NULL;
    int pktSize = sizeof(AodvRrepPacket);
    int i= 0;
    clocktype lifetime = (unsigned int) (AODV_ACTIVE_ROUTE_TIMEOUT /
                                                        MILLI_SECOND);
    UInt32 typeBitsPrefixSizeHop = 0;
    BOOL isDelay = TRUE;
    BOOL IPV6 = FALSE;
    Address broadcastAddress;

    Aodv6RrepPacket* rrep6Pkt = NULL;
    if (destAddr->networkType == NETWORK_IPV6)
    {
        pktSize = sizeof(Aodv6RrepPacket);
        protocolType = ROUTING_PROTOCOL_AODV6;
        IPV6 = TRUE;
    }

    if (AODV_DEBUG)
    {
        char time[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), time);
        printf("Node %u is sending Hello packet at %s\n",
            node->nodeId, time);
    }

    newMsg = MESSAGE_Alloc(
                 node,
                 NETWORK_LAYER,
                 protocolType,
                 MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(
        node,
        newMsg,
        pktSize,
        TRACE_AODV);

    pktPtr = (char *) MESSAGE_ReturnPacket(newMsg);

    memset(pktPtr, 0, pktSize);


    // Section 8.4 of draft-ietf-manet-aodv-08.txt
    // Allocate the message and then broadcast to all interfaces
    typeBitsPrefixSizeHop |= (AODV_RREP << 24);
    typeBitsPrefixSizeHop &= (~AODV_R_BIT);
    typeBitsPrefixSizeHop &= (~AODV_A_BIT);

    if (destAddr->networkType == NETWORK_IPV6)
    {
        rrep6Pkt = (Aodv6RrepPacket *) pktPtr;
        rrep6Pkt->typeBitsPrefixSizeHop = typeBitsPrefixSizeHop;
        rrep6Pkt->sourceAddr = ipv6->broadcastAddr;
        rrep6Pkt->destination.seqNum = aodv->seqNumber;
        rrep6Pkt->lifetime = (UInt32)lifetime;

    }
    else
    {
        rrepPkt = (AodvRrepPacket *) pktPtr;
        rrepPkt->typeBitsPrefixSizeHop = typeBitsPrefixSizeHop;
        rrepPkt->sourceAddr = ANY_IP;
        rrepPkt->destination.seqNum = aodv->seqNumber;
        rrepPkt->lifetime = (UInt32)lifetime;
    }

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (AodvIsEligibleInterface(node, destAddr, &aodv->iface[i])
                                                                == FALSE)
        {
            continue;
        }

        clocktype delay =
            (clocktype) AODV_PC_ERAND(aodv->aodvJitterSeed);

        if (destAddr->networkType == NETWORK_IPV6)
        {
            rrep6Pkt = (Aodv6RrepPacket *) MESSAGE_ReturnPacket(newMsg);

            memcpy(
                &rrep6Pkt->destination.address,
                &aodv->iface[i].address.interfaceAddr.ipv6,
                sizeof(in6_addr));
            SetIPv6AddressInfo(
                &broadcastAddress,
                aodv->broadcastAddr.interfaceAddr.ipv6);
        }
        else
        {
            rrepPkt = (AodvRrepPacket *) MESSAGE_ReturnPacket(newMsg);
            rrepPkt->destination.address =
                aodv->iface[i].address.interfaceAddr.ipv4;
            SetIPv4AddressInfo(&broadcastAddress,ANY_DEST);
        }


        if (AODV_DEBUG_AODV_TRACE)
        {
            AodvPrintTrace(node, newMsg, 'S',IPV6);
        }

        AodvSendPacket(
            node,
            MESSAGE_Duplicate(node, newMsg),
            aodv->iface[i].address,
            *destAddr,
            i,
            1,
            ANY_DEST,
            delay,
            isDelay);
    }

    MESSAGE_Free(node, newMsg);

    aodv->stats.numHelloSent++;

}

// /**
// FUNCTION : AodvFloodRREQ
// LAYER    : NETWORK
// PURPOSE  : Function to flood RREQ in all interfaces
// ARGUMENTS:
//  +node:Node*:Pointer to the node which is flooding RREQ
//  +aodv:AodvData*:Pointer to Aodv internal data structure
//  +J:int:J flag in the request
//  +R:int:R flag in the request
//  +G:BOOL:G flag in the request
//  +hopCount:UInt32:hop count in request
//  +floodingId:int:flooding id in request
//  +destAddr:Address:Destination in the request
//  +destSeq:unsigned int:Destination sequence number
//  +srcAddr:Address:Home address
//  +srcSeqNum:unsigned int:Own sequence number
//  +ttl:int:The ttl to be set for the request
//  +chkRplyDelay;clocktype:To wait for the reply
//  +isRelay:BOOL:Is this a RREQ being relayed
// RETURN   ::void:NULL
// **/

static
void AodvFloodRREQ(
         Node* node,
         AodvData* aodv,
         int J,
         int R,
         BOOL G,
         BOOL D,
         UInt32 hopCount,
         int floodingId,
         Address destAddr,
         unsigned int destSeq,
         Address srcAddr,
         unsigned int srcSeq,
         int ttl,
         clocktype chkRplyDelay,
         BOOL isRelay)
{
    Message* newMsg = NULL;
    AodvRreqPacket* rreqPkt = NULL;
    Aodv6RreqPacket* rreq6Pkt = NULL;
    int pktSize = 0;
    int i=0;
    BOOL IPV6 = FALSE;
    UInt32 typeBitsHopcounts = 0;
    BOOL isDelay = TRUE;
    int routingProtocol = 0;

    if (srcAddr.networkType == NETWORK_IPV6)
    {
        pktSize = sizeof(Aodv6RreqPacket);
        IPV6 = TRUE;
        routingProtocol = ROUTING_PROTOCOL_AODV6;
    }
    else
    {
        pktSize = sizeof(AodvRreqPacket);
        routingProtocol = ROUTING_PROTOCOL_AODV;
    }


    // Allocate the route request packet
    newMsg = MESSAGE_Alloc(
                 node,
                 MAC_LAYER,
                 routingProtocol,
                 MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(
        node,
        newMsg,
        pktSize,
        TRACE_AODV);

    if (IPV6)
    {
        rreq6Pkt = (Aodv6RreqPacket *) MESSAGE_ReturnPacket(newMsg);
        memset(rreq6Pkt, 0, pktSize);
    }
    else
    {
        rreqPkt = (AodvRreqPacket *) MESSAGE_ReturnPacket(newMsg);
        memset(rreqPkt, 0, pktSize);
    }

    typeBitsHopcounts |= (AODV_RREQ << 24);

    if (J)
    {
        typeBitsHopcounts |= AODV_J_BIT ; // Not used now
    }
    else
    {
        typeBitsHopcounts &= ~AODV_J_BIT ; // Not used now
    }

    if (R)
    {
        typeBitsHopcounts |= AODV_RREQ_R_BIT ; // Not used now
    }
    else
    {
        typeBitsHopcounts &= ~AODV_RREQ_R_BIT ; // Not used now
    }

    if (G)
    {
        typeBitsHopcounts |= AODV_G_BIT ;
    }
    else
    {
        typeBitsHopcounts &= ~AODV_G_BIT ;
    }

    if (D)
    {
        typeBitsHopcounts |= AODV_D_BIT ;
    }
    else
    {
        typeBitsHopcounts &= ~AODV_D_BIT ;
    }

    typeBitsHopcounts |= hopCount & AODV_HOP_COUNT_BITS;
    if (IPV6)
    {
        rreq6Pkt->info.typeBitsHopcounts = typeBitsHopcounts;
        rreq6Pkt->info.floodingId = floodingId;

        COPY_ADDR6(
            destAddr.interfaceAddr.ipv6,
            rreq6Pkt->destination.address);

        rreq6Pkt->destination.seqNum = destSeq;

        COPY_ADDR6(srcAddr.interfaceAddr.ipv6, rreq6Pkt->source.address);

        rreq6Pkt->source.seqNum = srcSeq;
    }
    else
    {
        rreqPkt->info.typeBitsHopcounts = typeBitsHopcounts;
        rreqPkt->info.floodingId = floodingId;
        rreqPkt->destination.address = GetIPv4Address(destAddr);
        rreqPkt->destination.seqNum = destSeq;
        rreqPkt->source.address = GetIPv4Address(srcAddr);
        rreqPkt->source.seqNum = srcSeq;
    }

    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&destAddr, address);
        printf("\tdestination: %s\n"
            "\tsequence: %u\n"
            "\tflooding id: %u\n"
            "\tttl: %u\n", address, destSeq,
            floodingId, ttl);
    }

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (AodvIsEligibleInterface(node, &destAddr, &aodv->iface[i])
                                                                 == FALSE)
        {
            continue;
        }

        clocktype delay = (clocktype) AODV_PC_ERAND(aodv->aodvJitterSeed);

        if (AODV_DEBUG_AODV_TRACE)
        {
            AodvPrintTrace(node, newMsg, 'S',IPV6);
        }

        if (IPV6)
        {

            AodvSendPacket(node,
                           MESSAGE_Duplicate(node, newMsg),
                           aodv->iface[i].address,
                           aodv->broadcastAddr,
                           i,
                           ttl,
                           ANY_DEST,
                           delay,
                           isDelay);


        }
        else
        {
            Address broadcastAddress;
            broadcastAddress.networkType = NETWORK_IPV4;
            broadcastAddress.interfaceAddr.ipv4 = ANY_DEST;
                AodvSendPacket(node,
                               MESSAGE_Duplicate(node, newMsg),
                               aodv->iface[i].address,
                               broadcastAddress,
                               i,
                               ttl,
                               ANY_DEST,
                               delay,
                               isDelay);
        }




        if (AODV_DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(delay, clockStr);
            printf("\tsending packet out on interface %u\n"
                "\tdelay %s\n", i, clockStr);
        }
    }


    // If this is a relay, it is already added from AodvHandleRequest
    if (!isRelay)
    {
        AodvInsertSeenTable(
            node,
            srcAddr,
            floodingId,
            &aodv->seenTable);
    }

    // The RREQ packet has been copied and broadcasted to all the interfaces
    // so destroy the initially allocated message
    MESSAGE_Free(node, newMsg);

    aodv->lastBroadcastSent = node->getNodeTime();

    // To prevent unnecessary network-wide floods of RREQs, the source node
    // SHOULD use an expanding ring search technique as an optimization.
    // Sec: 8.4

    if (chkRplyDelay)
    {
        if (AODV_DEBUG)
        {
            printf("Node %u is setting timer MSG_NETWORK_CheckReplied\n",
                node->nodeId);
        }

        AodvSetTimer(
            node,
            MSG_NETWORK_CheckReplied,
            destAddr,
            (clocktype) chkRplyDelay);
    }
}

// /**
// FUNCTION     : AodvInitiateRREQ
// LAYER        : NETWORK
// PURPOSE      : Initiate a Route Request packet when no route to
//                destination is known
// PARAMETERS   :
//  +node:Node*: Pointer to the node which is sending the Route Request
//  +destAddr:Address:The destination to which a route Aodv is
//                    looking for
// RETURN       ::void:NULL
// **/

static
void AodvInitiateRREQ(Node* node, Address destAddr)
{
    // A node floods a RREQ when it determines that it needs a route to
    // a destination and does not have one available.  This can happen
    // if the destination is previously unknown to the node, or if a
    // previously valid route to the destination expires or is broken
    // Sec: 8.3
    AodvData* aodv = NULL;
    BOOL IPV6 = FALSE;

    if (destAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData*)NetworkIpGetRoutingProtocol(
                node,
                ROUTING_PROTOCOL_AODV6,
                NETWORK_IPV6);
        IPV6 = TRUE;
    }
    else
    {
        aodv = (AodvData*)NetworkIpGetRoutingProtocol(
                node,
                ROUTING_PROTOCOL_AODV,
                NETWORK_IPV4);
    }

    AodvRreqSentNode* sentNode = NULL;
    int ttl = 0;
    BOOL G = FALSE;
    BOOL D = FALSE;

    ttl = AodvGetLastHopCount(
              aodv,
              destAddr,
              &aodv->routeTable);

    ERROR_Assert(ttl >= 1, "TTL needs to be greater than 0.\n");

    if (AODV_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %u initiating RREQ at %s\n", node->nodeId, clockStr);
    }

    if (aodv->biDirectionalConn)
    {
        G = TRUE;
    }
    else
    {
        G = FALSE;
    }

    if (aodv->Dflag)
    {
        D = TRUE;
    }

    // Increase own sequence number before flooding route request
    aodv->seqNumber++;


        // The message will be broadcasted to all the interfaces which are
        // running Aodv as there routing protocol

        AodvFloodRREQ(
            node,
            aodv,
            0,                       // J
            0,                       // R
            G,
            D,
            0,                       // HopCount
            AodvGetFloodingId(aodv),
            destAddr,
            AodvGetSeq(destAddr, &aodv->routeTable),
            aodv->defaultInterfaceAddr,
            aodv->seqNumber,
            ttl,
            2 * ttl * AODV_NODE_TRAVERSAL_TIME,
            FALSE);
//    }

    sentNode = AodvInsertSent(destAddr, ttl, &aodv->sent);

    AodvIncreaseTtl(aodv, sentNode);

    // Increase the statistical variable to store the number of
    // request sent
    aodv->stats.numRequestInitiated++;
}


// /**
// FUNCTION     : AodvInitiateRREQForLocalRepair
// LAYER        : NETWORK
// PURPOSE      : Initiate a Route Request packet for locally finding
//                a route in case of link break to a destination.
// PARAMETERS   :
//  +node:Node*:Pointer to the node which is sending the Route Request
//  +destAddr:Address:The destination to which a route Aodv
//                    is looking for
//  +ttl:int:The ttl to be used in the RREQ
// RETURN       ::void:NULL
// **/

static
void AodvInitiateRREQForLocalRepair(
         Node* node,
         Address destAddr,
         int ttl)
{
    // Section 8.3 of draft-ietf-manet-aodv-09.txt

    AodvData* aodv = NULL;
    //Address interfaceAddress;
    unsigned int destSeqNumber = 0;
    BOOL IPV6 = FALSE;
    if (destAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(node,
                                                    ROUTING_PROTOCOL_AODV6,
                                                    NETWORK_IPV6);
        IPV6 = TRUE;
    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(node,
                                                     ROUTING_PROTOCOL_AODV,
                                                     NETWORK_IPV4);
    }
    BOOL G = FALSE;
    BOOL D = FALSE;

    if (AODV_DEBUG_LOCAL_REPAIR)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %u initiating RREQ for local repair at %s\n",
            node->nodeId, clockStr);
    }

    if (aodv->biDirectionalConn)
    {
        G = TRUE;
    }
    else
    {
        G = FALSE;
    }

    if (aodv->Dflag)
    {
        D = TRUE;
    }

    // Increase own sequence number before flooding route request
    aodv->seqNumber++;

    destSeqNumber = AodvGetSeq(destAddr, &aodv->routeTable);

    destSeqNumber += 1;

        AodvFloodRREQ(
            node,
            aodv,
            0,                       // J
            0,                       // R
            G,
            D,
            0,                       // HopCount
            AodvGetFloodingId(aodv),
            destAddr,
            destSeqNumber,
            aodv->defaultInterfaceAddr,
            aodv->seqNumber,
            ttl,
            (clocktype) AODV_NET_TRAVERSAL_TIME,
            FALSE);
 //   }

    AodvInsertSent(destAddr, ttl, &aodv->sent);

    // Increase the statistical variable to store the number of
    // request sent
    aodv->stats.numRequestForLocalRepair++;
}


// /**
// FUNCTION: AodvRetryRREQ
// LAYER    : NETWORK
// PURPOSE:  Send RREQ again after not receiving any RREP
// ARGUMENTS:
//  +node:Node*:Pointer to the node sending rreq
//  +destAddr:Address: The destination for which rreq should be
//                      rebroadcasted
// RETURN   ::void:NULL
// **/

static
void AodvRetryRREQ(Node* node, Address destAddr)
{
    AodvData* aodv = NULL;
    BOOL IPV6 = FALSE;

    if (destAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData*)
            NetworkIpGetRoutingProtocol(node,
            ROUTING_PROTOCOL_AODV6,
            NETWORK_IPV6);
        IPV6 = TRUE;
    }
    else
    {
        aodv = (AodvData*)
            NetworkIpGetRoutingProtocol(node,
            ROUTING_PROTOCOL_AODV,
            NETWORK_IPV4);
    }

    AodvRreqSentNode* sentNode = NULL;
    int ttl = 0;
    BOOL G = FALSE;
    BOOL D = FALSE;
    sentNode = AodvCheckSent(destAddr, &aodv->sent);

    ERROR_Assert(sentNode != NULL,
        "Sent node must have entry of destinatoin");

    ttl = sentNode->ttl;

    if (AODV_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %u retrying RREQ at %s\n", node->nodeId, clockStr);
    }

    if (aodv->biDirectionalConn)
    {
        G = TRUE;
    }
    else
    {
        G = FALSE;
    }

    if (aodv->Dflag)
    {
        D = TRUE;
    }

        AodvFloodRREQ(
            node,
            aodv,
            0,                       // J
            0,                       // R
            G,
            D,
            0,                       // HopCount
            AodvGetFloodingId(aodv),
            destAddr,
            AodvGetSeq(destAddr, &aodv->routeTable),
            aodv->defaultInterfaceAddr,
            aodv->seqNumber,
            ttl,
            2 * ttl * AODV_NODE_TRAVERSAL_TIME,
            FALSE);
  //  }

        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
       // printf("Node %u relaying RREQ at %s\n", node->nodeId, clockStr);

    AodvIncreaseTtl(aodv, sentNode);

    if (ttl >= AODV_NET_DIAMETER)
    {
        sentNode->times++;
    }
    aodv->stats.numRequestResent++;
}


// /**
// FUNCTION: AodvRelayRREQ
// LAYER    : NETWORK
// PURPOSE:  Forward (re-broadcast) the RREQ
// ARGUMENTS:
//  +node:Node*:Pointer to the node forwarding the Route Request
//  +msg:Message*:Pointer to the Rreq packet
//  +destSeq:unsigned int:Destination Sequence Number
//  +ttl:int:Time to leave of the message
// RETURN   ::void:NULL
// **/

static
void AodvRelayRREQ(
         Node* node,
         Message* msg,
         unsigned int destSeq,
         int ttl,
         BOOL ipv6Flag)
{
    AodvData* aodv = NULL;
    AodvRreqPacket* oldRreq = NULL;
    Aodv6RreqPacket* old6Rreq = NULL;
    Address sourceAddress;
    Address destinationAddress;

    if (ipv6Flag)
    {
        aodv = (AodvData *)
                 NetworkIpGetRoutingProtocol(node,
                 ROUTING_PROTOCOL_AODV6,
                 NETWORK_IPV6);
        old6Rreq = (Aodv6RreqPacket *) MESSAGE_ReturnPacket(msg);
        SetIPv6AddressInfo(&sourceAddress,old6Rreq->source.address);
        SetIPv6AddressInfo(&destinationAddress,
                            old6Rreq->destination.address);

    }
    else
    {
        aodv = (AodvData *)
                 NetworkIpGetRoutingProtocol(node,
                 ROUTING_PROTOCOL_AODV,
                 NETWORK_IPV4);
        oldRreq = (AodvRreqPacket *) MESSAGE_ReturnPacket(msg);
        SetIPv4AddressInfo(&sourceAddress,oldRreq->source.address);
        SetIPv4AddressInfo(&destinationAddress,
                            oldRreq->destination.address);
    }


    if (AODV_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %u relaying RREQ at %s\n", node->nodeId, clockStr);
    }

    //Relay the packet after decreasing the TTL
    ttl = ttl - IP_TTL_DEC;

    // The message will be broadcasted to all the interfaces which are
    // running Aodv as there routing protocol
    if (ipv6Flag)
    {
        AodvFloodRREQ(
           node,
            aodv,
            (old6Rreq->info.typeBitsHopcounts & AODV_J_BIT) >> 23,
            (old6Rreq->info.typeBitsHopcounts & AODV_RREQ_R_BIT) >> 22,
            (old6Rreq->info.typeBitsHopcounts & AODV_G_BIT) >> 21,
            (old6Rreq->info.typeBitsHopcounts & AODV_D_BIT) >> 20,
            (old6Rreq->info.typeBitsHopcounts & AODV_HOP_COUNT_BITS) + 1,
            old6Rreq->info.floodingId,
            destinationAddress,
            destSeq,
            sourceAddress,
            old6Rreq->source.seqNum,
            ttl,
            0,  // No wait for Route Reply in this case
            TRUE);
    }
    else
    {
        AodvFloodRREQ(
            node,
            aodv,
            (oldRreq->info.typeBitsHopcounts & AODV_J_BIT) >> 23,
            (oldRreq->info.typeBitsHopcounts & AODV_RREQ_R_BIT) >> 22,
            (oldRreq->info.typeBitsHopcounts & AODV_G_BIT) >> 21,
            (oldRreq->info.typeBitsHopcounts & AODV_D_BIT) >> 20,
            (oldRreq->info.typeBitsHopcounts & AODV_HOP_COUNT_BITS) + 1,
            oldRreq->info.floodingId,
            destinationAddress,
            destSeq,
            sourceAddress,
            oldRreq->source.seqNum,
            ttl,
            0,  // No wait for Route Reply in this case
            TRUE);
    }

    aodv->stats.numRequestRelayed++;
}


// /**
// FUNCTION: AodvInitiateRREP
// LAYER    : NETWORK
// PURPOSE:  Initiating route reply message
// ARGUMENTS:
// +node:Node*:Pointer to the node sending the route reply
// +aodv:AodvData*:Pointer to Aodv main data structure
// +msg:Message*: Received Route request message
// +interfaceIndex:int:The interface through which the RREP should
//                     be sent
// +nextHop:Address:The node to which Route reply should be sent
// RETURN   ::void:NULL
// **/

static
void AodvInitiateRREP(
         Node* node,
         AodvData* aodv,
         Message* msg,
         int interfaceIndex,
         Address nextHop)
{
    Message* newMsg = NULL;
    AodvRreqPacket* rreqPkt = NULL;
    Aodv6RreqPacket* rreq6Pkt = NULL;
    AodvRrepPacket* rrepPkt = NULL;
    Aodv6RrepPacket* rrep6Pkt = NULL;
    char* pktPtr = NULL;
    unsigned int typeBitsPrefixSizeHop = 0;
    BOOL IPV6 = FALSE;
    int pktSize = 0;
    Address destinationAddress;
    //Address sourceAddress;
    BOOL isDelay = FALSE;
    int routingProtocol = 0;

    if (nextHop.networkType == NETWORK_IPV6)
    {
        IPV6 = TRUE;
    }

    if (IPV6)
    {
        pktSize = sizeof(Aodv6RrepPacket);
        rreq6Pkt = (Aodv6RreqPacket *) MESSAGE_ReturnPacket(msg);
        SetIPv6AddressInfo(&destinationAddress,
                                rreq6Pkt->destination.address);
        routingProtocol = ROUTING_PROTOCOL_AODV6;

    }
    else
    {
        pktSize = sizeof(AodvRrepPacket);
        rreqPkt = (AodvRreqPacket *) MESSAGE_ReturnPacket(msg);
        SetIPv4AddressInfo(&destinationAddress,
                                rreqPkt->destination.address);
        routingProtocol = ROUTING_PROTOCOL_AODV;
    }


    newMsg = MESSAGE_Alloc(
                 node,
                 MAC_LAYER,
                 routingProtocol,
                 MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(
        node,
        newMsg,
        pktSize,
        TRACE_AODV);

    pktPtr = (char *) MESSAGE_ReturnPacket(newMsg);

    memset(pktPtr, 0, pktSize);



    // Section 8.4 of draft-ietf-manet-aodv-08.txt
    typeBitsPrefixSizeHop |= (AODV_RREP << 24);
    typeBitsPrefixSizeHop &= (~AODV_R_BIT);

    if (aodv->iface[interfaceIndex].AFlag)
    {
        typeBitsPrefixSizeHop |= AODV_A_BIT;
    }
    else
    {
        typeBitsPrefixSizeHop &= (~AODV_A_BIT);
    }

    if (IPV6)
    {
        rrep6Pkt = (Aodv6RrepPacket *) pktPtr;
        rrep6Pkt->typeBitsPrefixSizeHop = typeBitsPrefixSizeHop;
        COPY_ADDR6(rreq6Pkt->source.address,rrep6Pkt->sourceAddr);
        COPY_ADDR6(rreq6Pkt->destination.address,
                        rrep6Pkt->destination.address);
        rrep6Pkt->destination.seqNum = aodv->seqNumber;
        rrep6Pkt->lifetime = (unsigned int) (AODV_MY_ROUTE_TIMEOUT /
                            MILLI_SECOND);
    }
    else
    {
        rrepPkt = (AodvRrepPacket *) pktPtr;
        rrepPkt->typeBitsPrefixSizeHop = typeBitsPrefixSizeHop;
        rrepPkt->sourceAddr = rreqPkt->source.address;
        rrepPkt->destination.address = rreqPkt->destination.address;
        rrepPkt->destination.seqNum = aodv->seqNumber;
        rrepPkt->lifetime = (unsigned int) (AODV_MY_ROUTE_TIMEOUT /
                            MILLI_SECOND);
    }


    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        IO_ConvertIpAddressToString(&destinationAddress, address);
        printf("Node %u initiating RREP at %s\n"
            "\tdestination: %s\n"
            "\tsequece: %u\n"
            "\thop count: %u\n", node->nodeId, clockStr, address,
            aodv->seqNumber,
            typeBitsPrefixSizeHop & AODV_HOP_COUNT_BITS);

        IO_ConvertIpAddressToString(&nextHop, address);
        printf("\tsending reply to: %s\n"
            "\tsending packet out on interface %u\n", address,
            interfaceIndex);
    }

    if (AODV_DEBUG_AODV_TRACE)
    {
        AodvPrintTrace(node, newMsg, 'S',IPV6);
    }

    AodvSendPacket(
        node,
        newMsg,
        aodv->iface[interfaceIndex].address,
        nextHop,
        interfaceIndex,
        1,
        nextHop.interfaceAddr.ipv4,
        0,
        isDelay);

    aodv->stats.numReplyInitiatedAsDest++;
}


// /**
// FUNCTION: AodvSendGratuitousRREP
// LAYER    : NETWORK
// PURPOSE:  Sending Gratuitous route reply towards the destination if the
//           node has a fresh route to the destination and if the G flag of
//           the route request message is set.
// ARGUMENTS:
//  +node:Node*:Pointe to the node sending the route reply
//  +aodv:AodvData*:Pointe to Aodv main data structure
//  +msg:Message*:Received Route request message
//  +rtEntryToDest:AodvRouteEntry*:Route entry toward the destination
//  +rtEntryToSrc:AodvRouteEntry*:Route entry toward the source
// RETURN   ::void:NULL
// **/

static
void AodvSendGratuitousRREP(
         Node* node,
         AodvData* aodv,
         Message* msg,
         AodvRouteEntry* rtEntryToDest,
         AodvRouteEntry* rtEntryToSrc)
{
    Message* newMsg = NULL;
    AodvRreqPacket* rreqPkt = NULL;
    Aodv6RreqPacket* rreq6Pkt = NULL;
    AodvRrepPacket* rrepPkt = NULL;
    Aodv6RrepPacket* rrep6Pkt = NULL;
    char* pktPtr = NULL;
    BOOL IPV6 = FALSE;
    Address sourceAddress;
    Address destinationAddress;
    //Address interfaceAddress;
    UInt32 sseqNum;
    int routingProtocol;
    unsigned int typeBitsPrefixSizeHop;

    int pktSize = 0;
    if (rtEntryToDest->destination.networkType == NETWORK_IPV6)
    {
        IPV6 = TRUE;
        pktSize = sizeof(Aodv6RrepPacket);
    }
    else
    {
        pktSize = sizeof(AodvRrepPacket);
    }

    // we don't want to send RREP if life time becomes 0
    if ((unsigned int) ((rtEntryToDest->lifetime - node->getNodeTime()) /
        MILLI_SECOND) == 0)
    {
        return;
    }

    if (IPV6)
    {
        rreq6Pkt = (Aodv6RreqPacket *) MESSAGE_ReturnPacket(msg);
        SetIPv6AddressInfo(&sourceAddress,
                                rreq6Pkt->source.address);
        SetIPv6AddressInfo(&destinationAddress,
                                rreq6Pkt->destination.address);
        sseqNum = rreq6Pkt->source.seqNum;
        routingProtocol = ROUTING_PROTOCOL_AODV6;
    }
    else
    {
        rreqPkt = (AodvRreqPacket *) MESSAGE_ReturnPacket(msg);
        SetIPv4AddressInfo(&sourceAddress,
                                rreqPkt->source.address);
        SetIPv4AddressInfo(&destinationAddress,
                                rreqPkt->destination.address);
        sseqNum = rreqPkt->source.seqNum;
        routingProtocol = ROUTING_PROTOCOL_AODV;
    }

    newMsg = MESSAGE_Alloc(
                 node,
                 MAC_LAYER,
                 routingProtocol,
                 MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(
        node,
        newMsg,
        pktSize,
        TRACE_AODV);

    pktPtr = (char *) MESSAGE_ReturnPacket(newMsg);

    memset(pktPtr, 0, pktSize);

    if (IPV6)
    {
        rrep6Pkt = (Aodv6RrepPacket *) pktPtr;
        rrep6Pkt->typeBitsPrefixSizeHop |= (AODV_RREP << 24);
        COPY_ADDR6(destinationAddress.interfaceAddr.ipv6,
                        rrep6Pkt->sourceAddr);
        COPY_ADDR6(sourceAddress.interfaceAddr.ipv6,
                        rrep6Pkt->destination.address);
        rrep6Pkt->destination.seqNum = sseqNum;
        rrep6Pkt->lifetime = (unsigned int) ((rtEntryToSrc->lifetime -
                            node->getNodeTime()) / MILLI_SECOND);
        typeBitsPrefixSizeHop = rrep6Pkt->typeBitsPrefixSizeHop;
    }
    else
    {
        rrepPkt = (AodvRrepPacket *) pktPtr;
        rrepPkt->typeBitsPrefixSizeHop |= (AODV_RREP << 24);
        rrepPkt->sourceAddr = destinationAddress.interfaceAddr.ipv4;
        rrepPkt->destination.address = sourceAddress.interfaceAddr.ipv4;
        rrepPkt->destination.seqNum = sseqNum;
        rrepPkt->lifetime = (unsigned int) ((rtEntryToSrc->lifetime -
                            node->getNodeTime()) / MILLI_SECOND);
        typeBitsPrefixSizeHop = rrepPkt->typeBitsPrefixSizeHop;
    }



    // ref: draft-ietf-manet-aodv-10.txt sec:6.6.3
    // draft-ietf-manet-aodv-9.txt sec :8.6.3  has not been followed
    // since that may result in the destination of the gratuitous RREP
    // to get a hop count 1 less than the actual hop count.
    // To make the value in hop count message field of gratuitous RREP
    // to be as specified in draft 9 sec: 8.6.3 , replace
    // "rrepPkt->hopCount = rtEntryToSrc->hopCount;"
    // with
    // "rrepPkt->hopCount = rreqPkt->hopCount;"

    typeBitsPrefixSizeHop |=
        (rtEntryToSrc->hopCount & AODV_HOP_COUNT_BITS);

    if (AODV_DEBUG_AODV_TRACE)
    {
        AodvPrintTrace(node, newMsg, 'S',IPV6);
    }

    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        IO_ConvertIpAddressToString(&destinationAddress, address);
        printf("Node %u initiating Gratuitous RREP at %s\n"
            "\tdestination: %s\n"
            "\tsequece: %u\n"
            "\thop count: %u\n", node->nodeId, clockStr, address,
            aodv->seqNumber,
            typeBitsPrefixSizeHop & AODV_HOP_COUNT_BITS);

        IO_ConvertIpAddressToString(
            &rtEntryToDest->nextHop,
            address);

        printf("\tsending reply to %s\n"
                "\tsending packet out on interface %u\n",
            address,
            rtEntryToDest->outInterface);
    }

    AodvSendPacket(
        node,
        newMsg,
        aodv->iface[rtEntryToDest->outInterface].address,//interfaceAddress,
        rtEntryToDest->nextHop,
        rtEntryToDest->outInterface,
        1,
        rtEntryToDest->nextHop.interfaceAddr.ipv4,
        0,
        FALSE);

    aodv->stats.numGratReplySent++;
}


// /**
// FUNCTION: AodvRelayRREP
// LAYER    :NETWORK
// PURPOSE:  Forward the RREP packet
// ARGUMENTS:
//  +node:Node*:Pointer to the node relaying reply
//  +msg:Message*:Message containing received route reply packet
//  +destRouteEntry:AodvRouteEntry*:Pointer to the destination route
// RETURN   ::void:NULL
// **/

static
void AodvRelayRREP(
         Node* node,
         Message* msg,
         AodvRouteEntry* destRouteEntry)
{
    Message* newMsg = NULL;
    AodvRrepPacket* newRrep = NULL;
    Aodv6RrepPacket* new6Rrep = NULL;
    char* pktPtr = NULL;
    BOOL isRtToSrcExist = FALSE;
    Address oldsrcAddr;
    Address olddstAddr;
    int pktSize = 0;
    AodvData* aodv = NULL;
    AodvRrepPacket* oldRrep = NULL;
    Aodv6RrepPacket* old6Rrep = NULL;
    BOOL IPV6 = FALSE;
    int routingProtocol;
    unsigned int destSeq;
    UInt32 typeBitsPrefixSizeHop;

    if (destRouteEntry->destination.networkType == NETWORK_IPV6)
    {
        pktSize = sizeof(Aodv6RrepPacket);
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
            node,
            ROUTING_PROTOCOL_AODV6,
            NETWORK_IPV6);
        old6Rrep = (Aodv6RrepPacket *) MESSAGE_ReturnPacket(msg);
        SetIPv6AddressInfo(&oldsrcAddr,old6Rrep->sourceAddr);
        SetIPv6AddressInfo(&olddstAddr,old6Rrep->destination.address);
        IPV6 = TRUE;
        routingProtocol = ROUTING_PROTOCOL_AODV6;
    }
    else
    {
        pktSize = sizeof(AodvRrepPacket);
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
            node,
            ROUTING_PROTOCOL_AODV,
            NETWORK_IPV4);
        oldRrep = (AodvRrepPacket *) MESSAGE_ReturnPacket(msg);
        SetIPv4AddressInfo(&oldsrcAddr,oldRrep->sourceAddr);
        SetIPv4AddressInfo(&olddstAddr,oldRrep->destination.address);
        routingProtocol = ROUTING_PROTOCOL_AODV;
    }


    AodvRouteEntry* rtToSource = AodvCheckRouteExist(
                                     oldsrcAddr,
                                     &aodv->routeTable,
                                     &isRtToSrcExist);


    // If the current node is not the source node as indicated by the Source
    // IP Address in the RREP message AND a forward route has been created
    // or updated as described before, the node consults its route table
    // entry for the source node to determine the next hop for the RREP
    // packet, and then forwards the RREP towards the source with its Hop
    // Count incremented by one. Sec: 8.7

    if (!isRtToSrcExist)
    {
        return;
    }

    newMsg = MESSAGE_Alloc(
                 node,
                 MAC_LAYER,
                 routingProtocol,
                 MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(
        node,
        newMsg,
        pktSize,
        TRACE_AODV);

    pktPtr = (char *) MESSAGE_ReturnPacket(newMsg);

    memset(pktPtr, 0, pktSize);

    if (IPV6)
    {
        new6Rrep = (Aodv6RrepPacket *) pktPtr;
        new6Rrep->typeBitsPrefixSizeHop |= (AODV_RREP << 24);

        // If a node forwards a RREP over a link that is likely to have errors
        // or be unidirectional, the node SHOULD set the `A' flag to require
        // that the recipient of the RREP acknowledge receipt of the RREP by
        // sending a RREP-ACK message back Sec: 8.7.
        if (aodv->processAck)
        {
            new6Rrep->typeBitsPrefixSizeHop |= AODV_A_BIT;
        }
        else
        {
            new6Rrep->typeBitsPrefixSizeHop &= (~AODV_A_BIT);
        }

        COPY_ADDR6(old6Rrep->sourceAddr,new6Rrep->sourceAddr);
        COPY_ADDR6(old6Rrep->destination.address,
                            new6Rrep->destination.address);
        new6Rrep->destination.seqNum = old6Rrep->destination.seqNum;
        new6Rrep->typeBitsPrefixSizeHop |=
            ((old6Rrep->typeBitsPrefixSizeHop & AODV_HOP_COUNT_BITS) + 1);

        new6Rrep->lifetime = old6Rrep->lifetime;
        typeBitsPrefixSizeHop = new6Rrep->typeBitsPrefixSizeHop;
        destSeq = new6Rrep->destination.seqNum;
    }
    else
    {
        newRrep = (AodvRrepPacket *) pktPtr;
        newRrep->typeBitsPrefixSizeHop |= (AODV_RREP << 24);

        // If a node forwards a RREP over a link that is likely to have errors
        // or be unidirectional, the node SHOULD set the `A' flag to require
        // that the recipient of the RREP acknowledge receipt of the RREP by
        // sending a RREP-ACK message back Sec: 8.7.
        if (aodv->processAck)
        {
            newRrep->typeBitsPrefixSizeHop |= AODV_A_BIT;
        }
        else
        {
            newRrep->typeBitsPrefixSizeHop &= (~AODV_A_BIT);
        }

        newRrep->sourceAddr = oldRrep->sourceAddr;
        newRrep->destination.address = oldRrep->destination.address;
        newRrep->destination.seqNum = oldRrep->destination.seqNum;
        newRrep->typeBitsPrefixSizeHop |=
            ((oldRrep->typeBitsPrefixSizeHop & AODV_HOP_COUNT_BITS) + 1);

        newRrep->lifetime = oldRrep->lifetime;
        typeBitsPrefixSizeHop = newRrep->typeBitsPrefixSizeHop;
        destSeq = newRrep->destination.seqNum;
    }

    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        IO_ConvertIpAddressToString(&olddstAddr, address);
        printf("Node %u relaying RREP at %s\n"
            "\tdestination: %s\n"
            "\tsequence: %u\n"
            "\thop count: %u\n", node->nodeId, clockStr, address,
            destSeq,
            typeBitsPrefixSizeHop & AODV_HOP_COUNT_BITS);

        IO_ConvertIpAddressToString(
            &rtToSource->nextHop,
            address);

        printf("\tseding reply to %s\n"
            "\tsending packet out on interface %u\n", address,
            rtToSource->outInterface);
    }

    // When any node generates or forwards a RREP, the precursor list for
    // the corresponding destination node is updated by adding to it the
    // next hop node to which the RREP is forwarded. Sec: 8.7

    AodvInsertPrecursorsTable(
        rtToSource->nextHop,
        &destRouteEntry->precursors);

    if (AODV_DEBUG_AODV_TRACE)
    {
        AodvPrintTrace(node, newMsg, 'S',IPV6);
    }

    AodvSendPacket(node,
                    newMsg,
                    aodv->iface[rtToSource->outInterface].address,//interfaceAddress,
                    rtToSource->nextHop,
                    rtToSource->outInterface,
                    1,
                    rtToSource->nextHop.interfaceAddr.ipv4,
                    0,
                    FALSE);





    aodv->stats.numReplyForwarded++;

    // Also, at each node the (reverse) route used to forward a RREP has
    // its lifetime changed to current time plus ACTIVE_ROUTE_TIMEOUT.

    AodvUpdateLifetime(
        node,
        aodv,
        rtToSource->nextHop,
        &aodv->routeTable,
        1);
}


// /**
// FUNCTION: AodvInitiateRREPbyIN
// LAYER    : NETWORK
// PURPOSE:  An intermediate node that knows the route to the destination
//           sends the RREP
// ARGUMENTS:
//  +node:Node*:Pointer to the node generating rrep.
//  +aodv:AodvData*: aodv main data structure
//  + msg:Message*:rreq received
//  +lastHopAddress:Address:last hop address in routing table for the
//                          destination
//  +interfaceIndex:int:The interface from which rreq received
//  +rtToDest:AodvRouteEntry*:entry to the destination
// RETURN   ::void:NULL
// **/

static
void AodvInitiateRREPbyIN(
         Node* node,
         AodvData* aodv,
         Message* msg,
         Address lastHopAddress,
         int interfaceIndex,
         AodvRouteEntry* rtToDest)
{
    Message* newMsg = NULL;
    AodvRreqPacket* rreqPkt = NULL;
    AodvRrepPacket* rrepPkt = NULL;
    Aodv6RreqPacket* rreq6Pkt = NULL;
    Aodv6RrepPacket* rrep6Pkt = NULL;
    char* pktPtr = NULL;
    int pktSize = 0;
    BOOL IPV6 = FALSE;
    Address destinationAddress;
    UInt32 typeBitsPrefixSizeHop = 0;
    UInt32 dseqNum;
    int routingProtocol;

    if (lastHopAddress.networkType == NETWORK_IPV6)
    {
        pktSize = sizeof(Aodv6RrepPacket);
        IPV6 = TRUE;
        routingProtocol = ROUTING_PROTOCOL_AODV6;
    }
    else
    {
        pktSize = (sizeof(AodvRrepPacket));
        routingProtocol = ROUTING_PROTOCOL_AODV;
    }
    // we don't want to send RREP if life time becomes 0
    if ((unsigned int) ((rtToDest->lifetime - node->getNodeTime()) /
                            MILLI_SECOND) == 0)
    {
        return;
    }

    newMsg = MESSAGE_Alloc(
                 node,
                 MAC_LAYER,
                 routingProtocol,
                 MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(
        node,
        newMsg,
        pktSize,
        TRACE_AODV);

    pktPtr = (char *) MESSAGE_ReturnPacket(newMsg);

    memset(pktPtr, 0, pktSize);
    if (IPV6)
    {
        rrep6Pkt = (Aodv6RrepPacket *) pktPtr;
        rreq6Pkt = (Aodv6RreqPacket *) MESSAGE_ReturnPacket(msg);
        rrep6Pkt->typeBitsPrefixSizeHop |= (AODV_RREP << 24);
        COPY_ADDR6(rreq6Pkt->source.address,rrep6Pkt->sourceAddr);
        COPY_ADDR6(rreq6Pkt->destination.address,
                      rrep6Pkt->destination.address);
        SetIPv6AddressInfo(&destinationAddress,
                             rreq6Pkt->destination.address);

        // If node generating the RREP is not the destination node, but
        // instead is an intermediate hop along the path from the source to the
        // destination, it copies the last known destination sequence number in
        // the Destination Sequence Number field in the RREP message. Sec: 8.6.2

        rrep6Pkt->destination.seqNum =
            rtToDest->destSeqNum;

        dseqNum = rrep6Pkt->destination.seqNum;
        // The Lifetime field of the RREP is calculated by subtracting
        // the current time from the expiration time in its route table entry.
        // Sec: 8.6.2

        rrep6Pkt->lifetime = (unsigned int) ((rtToDest->lifetime -
                            node->getNodeTime()) / MILLI_SECOND);

        // If node generating the RREP is not the destination node, but
        // instead is an intermediate hop along the path from the source to the
        // destination, it copies the last known destination sequence number in
        // the Destination Sequence Number field in the RREP message.

        rrep6Pkt->typeBitsPrefixSizeHop |=
            (rtToDest->hopCount & AODV_HOP_COUNT_BITS);

        typeBitsPrefixSizeHop = rrep6Pkt->typeBitsPrefixSizeHop;

    }
    else
    {
        rrepPkt = (AodvRrepPacket *) pktPtr;
        rreqPkt = (AodvRreqPacket *) MESSAGE_ReturnPacket(msg);
        rrepPkt->typeBitsPrefixSizeHop |= (AODV_RREP << 24);
        rrepPkt->sourceAddr = rreqPkt->source.address;
        rrepPkt->destination.address = rreqPkt->destination.address;
        SetIPv4AddressInfo(&destinationAddress,
                             rreqPkt->destination.address);
        // If node generating the RREP is not the destination node, but
        // instead is an intermediate hop along the path from the source to the
        // destination, it copies the last known destination sequence number in
        // the Destination Sequence Number field in the RREP message. Sec: 8.6.2

        rrepPkt->destination.seqNum =
            rtToDest->destSeqNum;

        dseqNum = rrepPkt->destination.seqNum;

        // The Lifetime field of the RREP is calculated by subtracting
        // the current time from the expiration time in its route table entry.
        // Sec: 8.6.2

        rrepPkt->lifetime = (unsigned int) ((rtToDest->lifetime -
                            node->getNodeTime()) / MILLI_SECOND);

        // If node generating the RREP is not the destination node, but
        // instead is an intermediate hop along the path from the source to the
        // destination, it copies the last known destination sequence number in
        // the Destination Sequence Number field in the RREP message.

        rrepPkt->typeBitsPrefixSizeHop |=
            (rtToDest->hopCount & AODV_HOP_COUNT_BITS);

        typeBitsPrefixSizeHop = rrepPkt->typeBitsPrefixSizeHop;
    }

    if ((typeBitsPrefixSizeHop & AODV_HOP_COUNT_BITS)
           > (unsigned int) AODV_NET_DIAMETER)
    {
        char errStr[MAX_STRING_LENGTH];
        strcpy(errStr, "AODV: Intermediate node sending RREP with "
               "ttl > AODV-NET-DIAMETER.\n");
    }

    if (AODV_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        char address[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        IO_ConvertIpAddressToString(&destinationAddress, address);
        printf("Node %u initiating RREP by intermediate node at %s\n"
            "\tdestination: %s\n"
            "\tsequence: %u\n"
            "\thop count: %u\n", node->nodeId, clockStr, address,
            dseqNum,
            typeBitsPrefixSizeHop & AODV_HOP_COUNT_BITS);

        IO_ConvertIpAddressToString(&lastHopAddress, address);
        printf("\tsending reply to %s\n"
            "\tsending packet out on interface %u\n", address,
            interfaceIndex);
    }

    if (AODV_DEBUG_AODV_TRACE)
    {
        AodvPrintTrace(node, newMsg, 'S',IPV6);
    }

    AodvSendPacket(
            node,
            newMsg,
            aodv->iface[interfaceIndex].address,
            lastHopAddress,
            interfaceIndex,
            1,
            lastHopAddress.interfaceAddr.ipv4,
            0,
            FALSE);

    aodv->stats.numReplyInitiatedAsIntermediate++;

    // Also, at each node the (reverse) route used to forward a RREP has
    // its lifetime changed to current time plus ACTIVE_ROUTE_TIMEOUT.

    AodvUpdateLifetime(
        node,
        aodv,
        lastHopAddress,
        &aodv->routeTable,
        rtToDest->hopCount);
}



// /**
// FUNCTION   :: AodvCreateIpv6BroadcastAddress
// LAYER      :: NETWORK
// PURPOSE    :: Create IPv6 Broadcast Address (ff02::1).
// PARAMETERS ::
//  +node:  Node* : Pointer to Node.
//  +rerrPacket:  AodvRerrPacket* : Pointer to RERR Packet.
//  +packetSize:  int : Packet size.
//  +ifOneUpstream:  BOOL : Bool.
//  +upstreamAddr:  Address : Upstream Address.
// RETURN     :: void : NULL.
// **/
void
AodvCreateIpv6BroadcastAddress(Node* node, Address* multicastAddr)
{
    multicastAddr->networkType = NETWORK_IPV6;

    Ipv6CreateBroadcastAddress(node, &multicastAddr->interfaceAddr.ipv6);
}

// /**
// FUNCTION   :: AodvSendRouteErrorPacket
// LAYER      :: NETWORK
// PURPOSE    :: Sending route error message to other nodes if route is not
//               not available to any destination.
// PARAMETERS ::
//  +node:  Node* : Pointer to Node.
//  +rerrPacket:  AodvRerrPacket* : Pointer to RERR Packet.
//  +packetSize:  int : Packet size.
//  +ifOneUpstream:  BOOL : Bool.
//  +upstreamAddr:  Address : Upstream Address.
// RETURN     :: void : NULL.
// **/
static
void AodvSendRouteErrorPacket(
         Node* node,
         AodvRerrPacket* rerrPacket,
         int   packetSize,
         BOOL   ifOneUpstream,
         Address upstreamAddr)
{
    int i;
    Message* newMsg = NULL;
    int interfaceIndex;
    NodeAddress  nextHopAddress;
    BOOL isDelay = TRUE;
    BOOL IPV6 = FALSE;
    AodvData* aodv =NULL;
    int routingProtocol = 0;

    if (upstreamAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
            node,
            ROUTING_PROTOCOL_AODV6,
            NETWORK_IPV6);
        IPV6 = TRUE;
        routingProtocol = ROUTING_PROTOCOL_AODV6;
    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
            node,
            ROUTING_PROTOCOL_AODV,
            NETWORK_IPV4);
        routingProtocol = ROUTING_PROTOCOL_AODV;
    }

   newMsg = MESSAGE_Alloc(node, 0, routingProtocol, 0);


    if (AODV_DEBUG && upstreamAddr.networkType != NETWORK_INVALID)
    {
        unsigned int i = 0;
        AodvAddrSeqInfo* addrSeq = NULL;
        Aodv6AddrSeqInfo* addr6Seq = NULL;
        char address[MAX_STRING_LENGTH];
        UInt32 destCount = 0;

        printf("Node %u sending RERR to ", node->nodeId);

        if (ifOneUpstream)
        {
            IO_ConvertIpAddressToString(&upstreamAddr, address);

            printf("%s\n", address);
        }
        else
        {
            printf("All neighbors");
        }

        printf("\tRerr packet dest count = %u\n",
            rerrPacket->typeBitsDestCount & AODV_DEST_COUNT_BITS);

        if (upstreamAddr.networkType == NETWORK_IPV4)
        {
            addrSeq = (AodvAddrSeqInfo *)
                  (((char *)rerrPacket) + sizeof(AodvRerrPacket));
        }
        else
        {
            addr6Seq = (Aodv6AddrSeqInfo *)
                  (((char *)rerrPacket) + sizeof(AodvRerrPacket));
        }

        destCount = rerrPacket->AODV_REER_DestCount;

        for (i = 0; i < destCount; i++)
        {
            UInt32 seqNum = 0;

            if (!IPV6)
            {
                IO_ConvertIpAddressToString(addrSeq[i].address, address);

                seqNum = addrSeq[i].seqNum;
            }
            else
            {
                IO_ConvertIpAddressToString(&addr6Seq[i].address, address);

                seqNum = addr6Seq[i].seqNum;
            }

            printf("\tunreachable dest %u = %s seq %u\n", i + 1,
                address, seqNum);
        }

        printf("\n");
    }

    MESSAGE_PacketAlloc(
        node,
        newMsg,
        packetSize,
        TRACE_AODV);

    memcpy(
        MESSAGE_ReturnPacket(newMsg),
        rerrPacket,
        packetSize);

    // If one upstream then unicast the rerr message towards the upstream
    // otherwise if there are more than one upstream broadcast the rerr

    if (ifOneUpstream &&
        AodvGetNextHop(
            upstreamAddr,
            &aodv->routeTable,
            &nextHopAddress,
            &interfaceIndex))
    {
    //    Address interfaceAddress;
        // Unicast route error
        clocktype delay = (clocktype) AODV_PC_ERAND(aodv->aodvJitterSeed);

        if (AODV_DEBUG_AODV_TRACE)
        {
            AodvPrintTrace(node, newMsg, 'S',IPV6);
        }
        AodvSendPacket(
            node,
            newMsg,
            aodv->iface[interfaceIndex].address,//interfaceAddress,
            upstreamAddr,
            interfaceIndex,
            1,
            nextHopAddress,
            delay,
            isDelay);
    }

    else
    {
        // Broadcast route error

        for (i = 0; i < node->numberInterfaces; i++)
        {
            AodvInterfaceInfo* iface = &aodv->iface[i];
            Address destAddr;

            if (((upstreamAddr.networkType == NETWORK_IPV4)
                && (iface->aodv4eligible == FALSE))
                || ((upstreamAddr.networkType == NETWORK_IPV6)
                && (iface->aodv6eligible == FALSE)))

            {
                continue;
            }

            clocktype delay = (clocktype) AODV_PC_ERAND(aodv->aodvJitterSeed);


            if (AODV_DEBUG_AODV_TRACE)
            {
                AodvPrintTrace(node, newMsg, 'S',IPV6);
            }
            if (IPV6)
            {
                AodvSendPacket(
                    node,
                    MESSAGE_Duplicate(node, newMsg),
                    iface->address,
                    aodv->broadcastAddr,
                    i,
                    1,
                    ANY_DEST,
                    delay,
                    isDelay);
            }
            else
            {
                SetIPv4AddressInfo(&destAddr, ANY_DEST);

                AodvSendPacket(
                    node,
                    MESSAGE_Duplicate(node, newMsg),
                    iface->address,
                    destAddr,
                    i,
                    1,
                    ANY_DEST,
                    delay,
                    isDelay);
            }

            if (AODV_DEBUG)
            {
                printf("\tsending packet out on interface %u\n", i);
            }

        }
        MESSAGE_Free(node, newMsg);
    }

    MEM_free(rerrPacket);
}


// /**
// FUNCTION: AodvSendRouteErrorForUnreachableDest
// LAYER    : NETWORK
// PURPOSE:  Sending route error message for link failure to a particular
//           destination
// ARGUMENT:
//  +node:Node*: The node sending the route error message
//  +aodv:AodvData*: Aodv main data structure
//  +destAddr:Address:Unreachable destination
//  +previousHopAddress:Address:upstream towards the source if
//                              isOneUpstream is true
//  +isOneUpstream:BOOL:True if there is one upstream towards the source
// RETURN   ::void:NULL
// **/

static
void AodvSendRouteErrorForUnreachableDest(
         Node* node,
         AodvData* aodv,
         Address destAddr,
         Address previousHopAddress,
         BOOL isOneUpstream)
{
    // Broken Route.  Drop Packet, send RERR again to make them stop
    // sending more.
    unsigned int destSeq;
    AodvRerrPacket* newRerrPacket = NULL;
    AodvAddrSeqInfo* AddrSeq = NULL;
    Aodv6AddrSeqInfo* addr6Seq = NULL;
    BOOL isValidRoute = FALSE;
    int addrseqInfoSize = 0;
    BOOL IPV6 = FALSE;

    AodvRouteEntry* rtToDest = AodvCheckRouteExist(
                                destAddr,
                                &aodv->routeTable,
                                &isValidRoute);

    int size = 0;

    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];
        char time[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&destAddr, address);
        TIME_PrintClockInSecond(node->getNodeTime(), time);

        printf("\troute broken to %s at %s, so send RRER\n", address, time);
    }
    if (destAddr.networkType == NETWORK_IPV6)
    {
        addrseqInfoSize = sizeof(Aodv6AddrSeqInfo);
        IPV6 = TRUE;
    }
    else
    {
        addrseqInfoSize = sizeof(AodvAddrSeqInfo);
    }


    newRerrPacket = (AodvRerrPacket *) MEM_malloc(sizeof(
        AodvRerrPacket) + addrseqInfoSize);

    memset(newRerrPacket, 0, sizeof(AodvRerrPacket) + addrseqInfoSize);

    newRerrPacket->typeBitsDestCount |= (AODV_RERR << 24);

    newRerrPacket->typeBitsDestCount &= (~AODV_N_BIT);
    newRerrPacket->typeBitsDestCount |= 1;
    if (IPV6)
    {
        addr6Seq = (Aodv6AddrSeqInfo *)
                    (((char *)newRerrPacket) + sizeof(AodvRerrPacket));
        COPY_ADDR6(destAddr.interfaceAddr.ipv6,addr6Seq->address);
    }
    else
    {
        AddrSeq = (AodvAddrSeqInfo *)
              (((char *)newRerrPacket) + sizeof(AodvRerrPacket));
        AddrSeq->address = destAddr.interfaceAddr.ipv4;
    }

    if (rtToDest)
    {
        destSeq = rtToDest->destSeqNum;
    }
    else
    {
        destSeq = aodv->seqNumber;
    }

    if (IPV6)
    {
        addr6Seq->seqNum = destSeq;
    }
    else
    {
        AddrSeq->seqNum = destSeq;
    }

    size = sizeof(AodvRerrPacket)
            + (newRerrPacket->AODV_REER_DestCount)
            * addrseqInfoSize;

    if (isOneUpstream)
    {
        AodvSendRouteErrorPacket(
            node,
            newRerrPacket,
            size,
            TRUE,
            previousHopAddress);
    }
    else
    {
        AodvSendRouteErrorPacket(
            node,
            newRerrPacket,
            size,
            FALSE,
            previousHopAddress);
    }
}


// /**
// FUNCTION : AodvIpIsMyIP()
// LAYER    : NETWORK
// PURPOSE  : Returns true of false depending upon the address matching.
// PARAMETERS:
//  +node:Node*:Pointer to node
//  +destAddr:Address:Address to be compared
// RETURN:
//  +TRUE:BOOL:If its own packet.
//  +FALSE:BOOL: If address do ot matches.
//  **/
static
BOOL AodvIpIsMyIP(Node* node,Address destAddr)
{
    if (destAddr.networkType == NETWORK_IPV6)
    {
        return(Ipv6IsMyPacket(node,&destAddr.interfaceAddr.ipv6));
    }
    else
    {
        return(NetworkIpIsMyIP(node,destAddr.interfaceAddr.ipv4));
    }
}


// /**
// FUNCTION: AodvHandleRequest
// LAYER    : NETWORK
// PURPOSE:  Processing procedure when RREQ is received
// ARGUMENTS:
//  +node:Node*: The node which has received the RREQ
//  +msg:Message*:The message contain the RREQ packet
//  +srcAddr:Address:Previous hop
//  +ttl:int:The ttl of the message
//  +interfaceIndex:int:The interface index through which the RREQ has
//                      been received.
// RETURN   ::void:NULL
// **/

static
void AodvHandleRequest(
         Node* node,
         Message* msg,
         Address srcAddr,
         int ttl,
         int interfaceIndex)
{
    AodvData* aodv = NULL;
    AodvRreqPacket* rreqPkt = NULL;
    Aodv6RreqPacket* rreq6Pkt = NULL;
    Address sourceAddress;
    Address destinationAddress;
    BOOL IPV6 = FALSE;
    BOOL dFlag = FALSE;
    UInt32 floodingId;
    UInt32 dseqNum;
    UInt32 sseqNum;
    UInt32 typeBitsHopCounts = 0;

    if (srcAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(node,
                                            ROUTING_PROTOCOL_AODV6,
                                            NETWORK_IPV6);
        rreq6Pkt = (Aodv6RreqPacket *) MESSAGE_ReturnPacket(msg);
        IPV6 = TRUE;
        SetIPv6AddressInfo(&sourceAddress,
                             rreq6Pkt->source.address);

        SetIPv6AddressInfo(&destinationAddress,
                             rreq6Pkt->destination.address);
        floodingId = rreq6Pkt->info.floodingId;
        dseqNum = rreq6Pkt->destination.seqNum;
        sseqNum = rreq6Pkt->source.seqNum;
        dFlag = (rreq6Pkt->info.typeBitsHopcounts & AODV_D_BIT) >> 20;
        typeBitsHopCounts = rreq6Pkt->info.typeBitsHopcounts;

    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(node,
                                                ROUTING_PROTOCOL_AODV,
                                                NETWORK_IPV4);
        rreqPkt = (AodvRreqPacket *) MESSAGE_ReturnPacket(msg);
        SetIPv4AddressInfo(&sourceAddress,
                            rreqPkt->source.address);
        SetIPv4AddressInfo(&destinationAddress,
                             rreqPkt->destination.address);
        floodingId = rreqPkt->info.floodingId;
        dseqNum = rreqPkt->destination.seqNum;
        sseqNum = rreqPkt->source.seqNum;
        dFlag = (rreqPkt->info.typeBitsHopcounts & AODV_D_BIT) >> 20;
        typeBitsHopCounts = rreqPkt->info.typeBitsHopcounts;
    }


    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&sourceAddress, address);
        printf("\trreqPkt->srcAddr = %s\n", address);

        IO_ConvertIpAddressToString(&destinationAddress, address);
        printf("\trreqPkt->destAddr = %s\n"
            "\trreqPkt->floodingId = %u\n"
            "\trreqPkt->destSeq = %u\n", address, floodingId,
            dseqNum);
    }

    aodv->stats.numRequestRecved++;

    // if the previous hop of the rreq is in the blacklist of this
    // node then ignore the request

    if (AodvCheckBlacklistTable(srcAddr, &aodv->blacklistTable))
    {
        if (AODV_DEBUG)
        {
            char address[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&srcAddr, address);
            printf("\tsource of the rreq %s is in black list\n"
                "\tso discard the message\n", address);
        }


        ActionData acnData;
        acnData.actionType = DROP;
        acnData.actionComment = DROP_NO_ROUTE;
        TRACE_PrintTrace(node,
                         msg,
                         TRACE_NETWORK_LAYER,
                         PACKET_IN,
                         &acnData,
        aodv->defaultInterfaceAddr.networkType);
        aodv->stats.numSenderInBlacklist++;
        return;
    }

    // Do not process a request if the hop count recorded in it is greater
    // than AODV net diameter.
    if ((typeBitsHopCounts & AODV_HOP_COUNT_BITS)
        > (unsigned int) AODV_NET_DIAMETER)
        {
            ActionData acnData;
                    acnData.actionType = DROP;
                    acnData.actionComment = DROP_EXCEED_NET_DIAMETER;
                    TRACE_PrintTrace(node,
                                     msg,
                                     TRACE_NETWORK_LAYER,
                                     PACKET_IN,
                                     &acnData,
                     aodv->defaultInterfaceAddr.networkType);
            aodv->stats.numMaxHopExceed++;
            return;
        }



    // When a node receives a flooded RREQ, it first checks to determine
    // whether it has received a RREQ with the same Source IP Address and
    // Flooding ID within at least the last FLOOD_RECORD_TIME milliseconds.
    // If such a RREQ has been received, the node silently discards the
    // newly received RREQ. Sec: 8.5

    if (FALSE == AodvLookupSeenTable(aodv,
                                     sourceAddress,
                                     floodingId,
                                     &aodv->seenTable))
    {
        clocktype revRtLifetime = 0;

        BOOL isValidSrc = FALSE;
        BOOL replyByIntermediate = FALSE;

        AodvRouteEntry* rtToSrc = NULL;
        AodvRouteEntry* rtEntryToDest = NULL;

        // This is not a duplicate packet so process the request

        if (AODV_DEBUG)
        {
            printf("\tnot a duplicate, so process it\n");
        }

        // Insert the source and the broadcast id in seen table to protect
        // processing duplicates

        AodvInsertSeenTable(
            node,
            sourceAddress,
            floodingId,
            &aodv->seenTable);

        // The node is the destination of the route
        if (AodvIpIsMyIP(node, destinationAddress))
        {
            // The node itself the destination so send rrep back

            // Since this is the destination the reverse route lifetime will be
            // added as active route timeout

            revRtLifetime = AODV_ACTIVE_ROUTE_TIMEOUT;

            aodv->stats.numRequestRecvedAsDest++;

            if (AODV_DEBUG)
            {
                printf("\ti'm the destination\n");
            }

            // Route Reply Generation by the Destination

            // If the generating node is the destination itself, it MUST
            // update its own sequence number to the maximum of its current
            // sequence number and the destination sequence number in the
            // RREQ packet.  The destination node places the value zero in
            // the Hop Count field of the RREP. Sec: 8.6.1

            aodv->seqNumber = MAX(aodv->seqNumber,
                (unsigned int) dseqNum);

            AodvInitiateRREP(
                node,
                aodv,
                msg,
                interfaceIndex,
                srcAddr);
        }

        else
        {
            // The node is not the destination for the packet so check
            // whether it has an active route to the destination

            BOOL isValidDest = FALSE;

            UInt32 destSeq = 0;

            // Since this is an intermediate node the reverse route lifetime
            // will be added as specified for reverse route until data
            // passes through the route

            revRtLifetime = AODV_REV_ROUTE_LIFE
                - (typeBitsHopCounts & AODV_HOP_COUNT_BITS)
                * AODV_NODE_TRAVERSAL_TIME;

            if (AODV_DEBUG)
            {
                printf("\ti'm not the destination\n");
            }

            rtEntryToDest = AodvCheckRouteExist(
                                destinationAddress,
                                &aodv->routeTable,
                                &isValidDest);

            if (dFlag || (isValidDest == FALSE) ||
                ((signed)(rtEntryToDest->destSeqNum -
                 dseqNum) < 0 )
               )
            {
                // If the node does not have an active route,
                // and the incoming IP header has TTL larger than 1,
                // it broadcasts the RREQ from all of its configured
                // interface(s) (see section 8.15).  The Destination Sequence
                // Number in the RREQ is updated to the maximum of the
                // existing Destination Sequence Number in the RREQ and
                // the destination sequence number in the routing table
                // (if an entry exists) of the current node.  The TTL
                // or hop limit field in the outgoing IP header is decreased
                // by one. The Hop Count field in the broadcast RREQ message
                // is incremented by one, to account for the new hop through
                // the intermediate node.


                // If the node, on the other hand, does have an active route
                // for the destination, it compares the destination sequence
                // number for that route with the Destination Sequence Number
                // field of the incoming RREQ. If the existing destination
                // sequence number is smaller than the Destination Sequence
                // Number field of the RREQ, the node again retransmits the
                // RREQ just as if it did not have an active route to the
                // destination.

                // Note: We don't need to decrement ttl as that is done in
                // ip. so instead on 1 we need to check for 0

                if (AODV_DEBUG)
                {
                    printf("\tdon't have route to or not fresh route\n");
                }

                //Now, the ttl has to be compared with 1, as the ttl would
                //not have been decremented till now; and we would need to
                //decrease it by 1
                if (ttl > 1)
                {
                    if (!rtEntryToDest)
                    {
                        destSeq = dseqNum;
                    }
                    else
                    {
                        destSeq = MAX(rtEntryToDest->destSeqNum,
                                    dseqNum);
                    }

                    AodvRelayRREQ(node, msg, destSeq, ttl, IPV6);
                }
                else
                {
                    if (AODV_DEBUG)
                    {
                        printf("\tttl expired\n");
                    }

                    ActionData acnData;
                    acnData.actionType = DROP;
                    acnData.actionComment = DROP_TTL_ZERO;
                    TRACE_PrintTrace(node,
                                     msg,
                                     TRACE_NETWORK_LAYER,
                                     PACKET_IN,
                                     &acnData,
                    aodv->defaultInterfaceAddr.networkType);

                    aodv->stats.numRequestTtlExpired++;
                }
            }
            else if (!dFlag)
            {
                // has a fresh route to the destination and intermediate
                // node can reply
                if (AODV_DEBUG)
                {
                    printf("\thas a fresh route to destination\n");
                }
                replyByIntermediate = TRUE;
            } // else
        } // else (not dest)


        // The node always creates or updates a reverse route to the Source
        // IP Address in its routing table.  If a route to the Source IP
        // Address already exists, it is updated only if either

        // (i)       the Source Sequence Number in the RREQ is higher than
        //           the destination sequence number of the Source IP Addr
        //           in the route table, or

        // (ii)      the sequence numbers are equal, but the hop count as
        //           specified by the RREQ, plus one, is now smaller than the
        //           existing hop count in the routing table.
        // Sec: 8.5

        rtToSrc = AodvCheckRouteExist(
                      sourceAddress,
                      &aodv->routeTable,
                      &isValidSrc);

        if (!rtToSrc
            || (((signed)(rtToSrc->destSeqNum - sseqNum)) < 0)
            || ((rtToSrc->destSeqNum == sseqNum)
            && ((!isValidSrc) || rtToSrc->hopCount
            > (int) ((typeBitsHopCounts & AODV_HOP_COUNT_BITS) + 1))))
        {
            // 1. the Source Sequence Number from the RREQ is copied to the
            //    corresponding destination sequence number;
            //
            // 2. the next hop in the routing table becomes the node
            //    transmitting the RREQ (it is obtained from the source IP
            //    address in the IP header and is often not equal to the
            //    Source IP Address field in the RREQ message);
            //
            // 3. the hop count is copied from the Hop Count in the RREQ
            //    message and incremented by one;
            // Sec: 8.5

            rtToSrc = AodvReplaceInsertRouteTable(
                          node,
                          sourceAddress,
                          sseqNum,
                          (typeBitsHopCounts & AODV_HOP_COUNT_BITS) + 1,
                          srcAddr,
                          node->getNodeTime() + revRtLifetime,
                          TRUE,
                          interfaceIndex,
                          &aodv->routeTable);

            if (replyByIntermediate)
            {
                // Furthermore, the intermediate
                // node puts the next hop towards the destination in the
                // precursor list for the reverse route entry -- i.e.,
                // the entry for the Source IP Address field of the RREQ
                // message data. Sec: 8.6.2
                AodvInsertPrecursorsTable(
                    rtEntryToDest->nextHop,
                    &rtToSrc->precursors);
            }

            // At this point , received RREQ has updated the routing table.
            // AODV buffer is cheked if theres any pending packet for source
            // of this RREQ. If there is pending packet, packet is sent and
            // entry is deleted from the sent table (if its there).
            Message* newMsg = NULL;
            Address previousHop;
            AodvDeleteSent(sourceAddress, &aodv->sent);
            newMsg = AodvGetBufferedPacket(
                        node,
                        sourceAddress,
                        &previousHop,
                        &aodv->msgBuffer);

            if (newMsg)
            {
                aodv->stats.numRoutes++;
                aodv->stats.numHops += rtToSrc->hopCount;
            }
            // Send any buffered packets to the destination
            while (newMsg)
            {
                aodv->stats.numDataInitiated++;

                AodvTransmitData(
                    node,
                    newMsg,
                    rtToSrc,
                    previousHop);

                newMsg = AodvGetBufferedPacket(
                            node,
                            sourceAddress,
                            &previousHop,
                            &aodv->msgBuffer);
            } // while
        }


        if (AODV_DEBUG)
        {
            printf("Node %u is setting timer "
                "MSG_NETWORK_CheckRouteTimeout\n", node->nodeId);
        }

        if (aodv->isExpireTimerSet == FALSE)
        {

            aodv->isExpireTimerSet = TRUE;

            AodvSetTimer(
                node,
                MSG_NETWORK_CheckRouteTimeout,
                sourceAddress,
                (clocktype) revRtLifetime);
        }

        if (replyByIntermediate)
        {
            // No node will send a reply as intermediate node if
            // summation of hop count to source and hop count to destination
            // exceeds Aodv Net Diameter.

            if (((typeBitsHopCounts & AODV_HOP_COUNT_BITS)
                + rtEntryToDest->hopCount)
                 > (unsigned int) AODV_NET_DIAMETER)
            {
                aodv->stats.numMaxHopExceed++;
                return;
            }

            // When the intermediate node updates its route table for
            // the source of the RREQ, it puts the last hop node
            // (from which it received the RREQ, as indicated by the
            // source IP address field in the IP header) into the
            // precursor list for the forward path route entry -- i.e.,
            // the entry for the Destination IP Address. Sec: 8.6.2

            AodvInsertPrecursorsTable(
                srcAddr,
                &rtEntryToDest->precursors);

            // In order that the destination learn of routes to the
            // originating node, the originating node SHOULD set the
            // ``gratuitous RREP'' ('G') flag in the RREQ if the session
            // is going to be run over TCP, or if the destination should
            // receive the gratuitous RREP for any other reason.  If an
            // intermediate node returns a RREP in response to a RREQ
            // with the 'G' flag set, it MUST also unicast a gratuitous
            // RREP to the destination node. sec: 8.6.3

            if (typeBitsHopCounts & AODV_G_BIT)
            {
                AodvSendGratuitousRREP(
                    node,
                    aodv,
                    msg,
                    rtEntryToDest,
                    rtToSrc);
            }

            // Send a Route Reply
            AodvInitiateRREPbyIN(
                node,
                aodv,
                msg,
                srcAddr,
                interfaceIndex,
                rtEntryToDest);
        }


        // When a node receives an AODV control packet from a neighbor, it
        // checks its route table for an entry for that neighbor.  In the
        // event that there is no corresponding entry for that neighbor, an
        // entry is created.  The sequence number is either determined from
        // the information contained in the control packet (i.e.,
        // the neighbor is the source of a RREQ), or else it is initialized
        // to zero if the sequence number for that node can not be
        // determined.  The lifetime for the routing table entry is either
        // determined from the control packet (i.e., the neighbor is the
        // originator of a RREP for itself), or it is initialized to
        // ALLOWED_HELLO_LOSS * HELLO_INTERVAL. The hop count to the neighbor
        // is set to one. sec 6.2 draft 10

        if (!Address_IsSameAddress(&sourceAddress,  &srcAddr))
        {
            AodvRouteEntry* ptrToPrevHop;
            ptrToPrevHop = AodvReplaceInsertRouteTable(
                               node,
                               srcAddr,
                               0,
                               1,
                               srcAddr,
                               node->getNodeTime() + AODV_ACTIVE_ROUTE_TIMEOUT,
                               TRUE,
                               interfaceIndex,
                               &aodv->routeTable);

            if (AODV_DEBUG)
            {
                printf("Node %u is setting timer "
                    "MSG_NETWORK_CheckRouteTimeout\n", node->nodeId);
            }

            if (aodv->isExpireTimerSet == FALSE)
            {

                aodv->isExpireTimerSet = TRUE;

                AodvSetTimer(
                    node,
                    MSG_NETWORK_CheckRouteTimeout,
                    srcAddr,
                    (clocktype) AODV_ACTIVE_ROUTE_TIMEOUT);
            }
        }
    }
    else
    {
        // If this RREQ already has been processed then silently discard the
        // message

        ActionData acnData;
        acnData.actionType = DROP;
        acnData.actionComment = DROP_DUPLICATE_PACKET;
        TRACE_PrintTrace(node,
                         msg,
                         TRACE_NETWORK_LAYER,
                         PACKET_IN,
                         &acnData,
                         aodv->defaultInterfaceAddr.networkType);

        aodv->stats.numRequestDuplicate++;
        if (AODV_DEBUG)
        {
            printf("\talready seen it!\n");
        }
    }
}

// /**
// FUNCTION   :: AodvHandleReply
// LAYER      :: NETWORK
// PURPOSE    :: Processing procedure when RREP is received.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +msg:  Message* : Pointer to Message.
//  +srcAddr:  Address : Source Address.
//  +interfaceIndex:  int : Interface Index.
// RETURN     :: void : NULL.
// **/
static
void AodvHandleReply(
         Node* node,
         Message* msg,
         Address srcAddr,
         int interfaceIndex,
         Address destAddr)
{
    AodvData* aodv = NULL;

    AodvRrepPacket* rrepPkt = NULL;

    clocktype lifetime;

    AodvRouteEntry* prevRtPtr = NULL;
    AodvRouteEntry* newRtPtr = NULL;
    AodvRouteEntry* rtToDest = NULL;
    Address sourceAddress;
    Address destinationAddress;
    BOOL IPV6 = FALSE;
    UInt32 dseqNum = 0;

    // When a node receives a RREP message, it first increments the hop
    // count value in the RREP by one, to account for the new hop through
    // the intermediate node. Sec: 8.7

    int hopCount = 0;

    BOOL newRtAdded = FALSE;
    BOOL routeUpdated = FALSE;
    BOOL isHelloMsg = FALSE;
    BOOL isValidRt = FALSE;
    int routingProtocol = 0;

    Aodv6RrepPacket* rrep6Pkt = NULL;
    if (srcAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                    node,
                                    ROUTING_PROTOCOL_AODV6,
                                    NETWORK_IPV6);

        routingProtocol = ROUTING_PROTOCOL_AODV6;

        rrep6Pkt = (Aodv6RrepPacket *) MESSAGE_ReturnPacket(msg);
        hopCount = (rrep6Pkt->AODV_RREP_HopCount) + 1;
        SetIPv6AddressInfo(&sourceAddress,rrep6Pkt->sourceAddr);
        SetIPv6AddressInfo(&destinationAddress,
                            rrep6Pkt->destination.address);
        // clocktype must be copied to access the field of that type
        lifetime = (clocktype) rrep6Pkt->lifetime * MILLI_SECOND;
        IPV6 = TRUE;
        dseqNum = rrep6Pkt->destination.seqNum;
    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                    node,
                                    ROUTING_PROTOCOL_AODV,
                                    NETWORK_IPV4);

        routingProtocol = ROUTING_PROTOCOL_AODV;
        rrepPkt = (AodvRrepPacket *) MESSAGE_ReturnPacket(msg);
        hopCount = (rrepPkt->AODV_RREP_HopCount) + 1;
        SetIPv4AddressInfo(&sourceAddress,rrepPkt->sourceAddr);
        SetIPv4AddressInfo(&destinationAddress,
                                rrepPkt->destination.address);
        // clocktype must be copied to access the field of that type
        lifetime = (clocktype) rrepPkt->lifetime * MILLI_SECOND;
        dseqNum = rrepPkt->destination.seqNum;
    }


    if ((rrepPkt && (rrepPkt->typeBitsPrefixSizeHop & AODV_A_BIT ))
            || (rrep6Pkt && (rrep6Pkt->typeBitsPrefixSizeHop & AODV_A_BIT)))
    {
        Message* newMsg = NULL;
        unsigned short packet = 4;

        newMsg = MESSAGE_Alloc(
                 node,
                 MAC_LAYER,
                 routingProtocol,
                 MSG_MAC_FromNetwork);

        MESSAGE_PacketAlloc(
            node,
            newMsg,
            sizeof(unsigned short),
            TRACE_AODV);

        *((unsigned short*)MESSAGE_ReturnPacket(newMsg)) = packet;

        AodvSendPacket(
            node,
            newMsg,
            destAddr,
            srcAddr,
            interfaceIndex,
            1,
            srcAddr.interfaceAddr.ipv4,
            0,
            FALSE);
    }


    // Don't process a reply packet if the hop count in the packet is
    // greater than aodv net diameter.
    if (hopCount > AODV_NET_DIAMETER)
    {

        ActionData acnData;
        acnData.actionType = DROP;
        acnData.actionComment = DROP_EXCEED_NET_DIAMETER;
        TRACE_PrintTrace(node,
                         msg,
                         TRACE_NETWORK_LAYER,
                         PACKET_IN,
                         &acnData,
        aodv->defaultInterfaceAddr.networkType);
        aodv->stats.numMaxHopExceed++;
        return;
    }


    if (lifetime == 0)
    {

        ActionData acnData;
        acnData.actionType = DROP;
        acnData.actionComment = DROP_LIFETIME_EXPIRY;
        TRACE_PrintTrace(node,
                         msg,
                         TRACE_NETWORK_LAYER,
                         PACKET_OUT,
                         &acnData,
                         aodv->defaultInterfaceAddr.networkType);
        return;
    }

    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        IO_ConvertIpAddressToString(&sourceAddress, address);

        printf("\trrepPkt->srcAddr = %s\n", address);

        IO_ConvertIpAddressToString(&destinationAddress, address);

        printf("\trrepPkt->destAddr = %s\n"
            "\trrepPkt->destSeq = %u\n", address,
            dseqNum);
    }


    if ((rrepPkt && (rrepPkt->sourceAddr == ANY_IP)) ||
       (rrep6Pkt && SAME_ADDR6(rrep6Pkt->sourceAddr,
                               aodv->broadcastAddr.interfaceAddr.ipv6)))
    {
        isHelloMsg = TRUE;
    }


    if (isHelloMsg)
    {
        if (AODV_DEBUG_HELLO)
        {
            char clockStr[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

            printf("Received hello message at %s\n", clockStr);
        }

        // Whenever a node receives a HELLO packet from a neighbor, the node
        // SHOULD make sure that it has an active route to the neighbor,
        // and create one if necessary.  If a route already exists, then the
        // Lifetime for the route should be increased if necessary to be at
        // least ACTIVE_ROUTE_TIMEOUT. In any case, the route to the
        // neighbor should be updated to contain the latest Destination
        // Sequence Number from the HELLO message. Sec: 8.9
        rtToDest = AodvReplaceInsertRouteTable(
                       node,
                       destinationAddress,
                       dseqNum,
                       hopCount,
                       srcAddr,
                       node->getNodeTime() + AODV_ACTIVE_ROUTE_TIMEOUT,
                       TRUE,
                       interfaceIndex,
                       &aodv->routeTable);

        if (rtToDest && aodv->processHello)
        {
            Message* newMsg = NULL;
            Address* info = NULL;
            int protocolType = -1;

            if (srcAddr.networkType == NETWORK_IPV6)
            {
                protocolType = ROUTING_PROTOCOL_AODV6;
            }
            else
            {
                protocolType = ROUTING_PROTOCOL_AODV;
            }


            rtToDest->helloSeqNum++;

            // Allocate message for the timer
            newMsg = MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        protocolType,
                        MSG_NETWORK_CheckNeighborTimeout);

            // Assign the address for which the timer is meant for
            MESSAGE_InfoAlloc(
                node,
                newMsg,
                sizeof(Address) + sizeof(UInt32));

            info = (Address *) MESSAGE_ReturnInfo(newMsg);

            memcpy(info, &srcAddr, sizeof(Address));

            memcpy(info + 1, &rtToDest->helloSeqNum, sizeof(UInt32));

            // Schedule the timer after the specified delay
            MESSAGE_Send(node, newMsg, 4 * aodv->helloInterval);



        }
        if (aodv->isExpireTimerSet == FALSE)
        {
            aodv->isExpireTimerSet = TRUE;
            AodvSetTimer(
                node,
                MSG_NETWORK_CheckRouteTimeout,
                destinationAddress,
                (clocktype) AODV_ACTIVE_ROUTE_TIMEOUT);
        }

        aodv->stats.numHelloRecved++;

        return;
    }

    aodv->stats.numReplyRecved++;

    // When a node receives an AODV control packet from a neighbor, it
    // checks its route table for an entry for that neighbor.  In the
    // event that there is no corresponding entry for that neighbor, an
    // entry is created.  The sequence number is either determined from
    // the information contained in the control packet (i.e.,
    // the neighbor is the source of a RREQ), or else it is initialized
    // to zero if the sequence number for that node can not be
    // determined.  The lifetime for the routing table entry is either
    // determined from the control packet (i.e., the neighbor is the
    // originator of a RREP for itself), or it is initialized to
    // ALLOWED_HELLO_LOSS * HELLO_INTERVAL. The hopcount to the neighbor
    // is set to one. sec 6.2 draft 10

    if (!Address_IsSameAddress(&destinationAddress,&srcAddr))
    {
        AodvRouteEntry* ptrToPrevHop = NULL;

        ptrToPrevHop = AodvReplaceInsertRouteTable(
                           node,
                           srcAddr,
                           0,
                           1,
                           srcAddr,
                           node->getNodeTime() + AODV_ACTIVE_ROUTE_TIMEOUT,
                           TRUE,
                           interfaceIndex,
                           &aodv->routeTable);

        if (aodv->isExpireTimerSet == FALSE)
        {
            aodv->isExpireTimerSet = TRUE;

            if (AODV_DEBUG)
            {
                printf("Node %u is setting timer "
                    "MSG_NETWORK_CheckRouteTimeout\n", node->nodeId);
            }

            AodvSetTimer(
                node,
                MSG_NETWORK_CheckRouteTimeout,
                srcAddr,
                (clocktype) AODV_ACTIVE_ROUTE_TIMEOUT);
        }
    }

    prevRtPtr = AodvCheckRouteExist(
                    destinationAddress,
                    &aodv->routeTable,
                    &isValidRt);

    rtToDest = prevRtPtr;

    // The forward route for this destination is created or updated only if
    // (i) the Destination Sequence Number in the RREP is greater than the
    //     node's copy of the destination sequence number, or
    // (ii) the sequence numbers are the same, but the route is no longer
    //      active or the incremented Hop Count in RREP is smaller than
    //      the hop count in route table entry. Sec: 8.7

    if (!prevRtPtr
        ||(((signed)(prevRtPtr->destSeqNum - dseqNum)) < 0 )
        || ((prevRtPtr->destSeqNum
        == dseqNum)
        && (!isValidRt || prevRtPtr->hopCount > hopCount)))
    {
        BOOL rplyForLocalRepair = FALSE;

        // Add the forward route entry in the routing table
        // Check if the rrep is for a local repair

        if (prevRtPtr && (prevRtPtr->locallyRepairable == TRUE))
        {
            // This is a reply for local repair
            rplyForLocalRepair = TRUE;
            aodv->stats.numReplyRecvedForLocalRepair++;

            if (AODV_DEBUG_LOCAL_REPAIR)
            {
                printf("\tReceived reply for local repair\n");
            }
        }

        if (AODV_DEBUG)
        {
            printf("\tadding new route or updating route\n");
        }

        // Add the forward route entry in the routing table
        newRtPtr = AodvReplaceInsertRouteTable(
                       node,
                       destinationAddress,
                       dseqNum,
                       hopCount,
                       srcAddr,
                       node->getNodeTime() + lifetime,
                       TRUE,
                       interfaceIndex,
                       &aodv->routeTable);

        if (aodv->isExpireTimerSet == FALSE)
        {
            aodv->isExpireTimerSet = TRUE;
            if (AODV_DEBUG)
            {
                printf("Node %u is setting timer "
                    "MSG_NETWORK_CheckRouteTimeout\n", node->nodeId);
            }

            AodvSetTimer(
                node,
                MSG_NETWORK_CheckRouteTimeout,
                destinationAddress,
                lifetime);
        }

        if (rplyForLocalRepair)
        {
            Message* newMsg = NULL;
            Address previousHop;
            prevRtPtr->locallyRepairable = FALSE;
            // The reply is against a local repair rreq
            AodvDeleteSent(destinationAddress, &aodv->sent);

            if (AODV_DEBUG_LOCAL_REPAIR)
            {
                printf("\tlast hop count = %u, new hop count = %u\n",
                    newRtPtr->lastHopCount, newRtPtr->hopCount);
            }

            // Send any buffered packets to the destination
            newMsg = AodvGetBufferedPacket(
                        node,
                        destinationAddress,
                        &previousHop,
                        &aodv->msgBuffer);

            while (newMsg)
            {
                aodv->stats.numDataForwarded++;

                AodvTransmitData(
                    node,
                    newMsg,
                    newRtPtr,
                    previousHop);

                newMsg = AodvGetBufferedPacket(
                            node,
                            destinationAddress,
                            &previousHop,
                            &aodv->msgBuffer);
            }

            if (newRtPtr->hopCount > newRtPtr->lastHopCount)
            {
                // If the hop count of the newly determined route to the
                // destination is greater than the hop count of the
                // previously known route, as recorded in the last hop count
                // field, the node SHOULD create a RERR message for the
                // destination, with the 'N' bit set. Sec: 8.12

                AodvRerrPacket* newRerrPacket = NULL;
                AodvAddrSeqInfo* AddrSeq = NULL;
                Aodv6AddrSeqInfo * addr6Seq = NULL;
                int addrSeqPktSize = 0;
                int size = 0;
                Address broadcastAddress;

                if (AODV_DEBUG_LOCAL_REPAIR)
                {
                    printf("\tsending route error with N bit set\n");
                }

                aodv->stats.numRerrInitiatedWithNFlag++;
                if (IPV6)
                {
                    addrSeqPktSize = sizeof(Aodv6AddrSeqInfo);
                }
                else
                {
                    addrSeqPktSize = sizeof(AodvAddrSeqInfo);
                }
                newRerrPacket = (AodvRerrPacket *) MEM_malloc(sizeof(
                        AodvRerrPacket) + addrSeqPktSize);

                memset(
                        newRerrPacket,
                        0,
                        sizeof(AodvRerrPacket) + addrSeqPktSize);

                newRerrPacket->typeBitsDestCount |= (AODV_RERR << 24);
                newRerrPacket->typeBitsDestCount |= AODV_N_BIT;
                newRerrPacket->typeBitsDestCount |= 1;
                if (IPV6)
                {
                    addr6Seq = (Aodv6AddrSeqInfo *)
                    (((char *)newRerrPacket) + sizeof(AodvRerrPacket));
                    COPY_ADDR6(destinationAddress.interfaceAddr.ipv6,
                                addr6Seq->address);
                    addr6Seq->seqNum = dseqNum;
                    SetIPv6AddressInfo(
                        &broadcastAddress,
                        aodv->broadcastAddr.interfaceAddr.ipv6);
                }
                else
                {
                    AddrSeq = (AodvAddrSeqInfo *)
                        (((char *)newRerrPacket) + sizeof(AodvRerrPacket));
                    AddrSeq->address = rrepPkt->destination.address;
                    AddrSeq->seqNum = rrepPkt->destination.seqNum;
                    SetIPv4AddressInfo(&broadcastAddress,ANY_DEST);
                }

                size = sizeof(AodvRerrPacket)
                    + (newRerrPacket->AODV_REER_DestCount)
                    * addrSeqPktSize;

                AodvSendRouteErrorPacket(
                    node,
                    newRerrPacket,
                    size,
                    FALSE,
                    broadcastAddress);
            }

            return;
        }

        if (!prevRtPtr)
        {
            if (AODV_DEBUG)
            {
                printf("\tfirst RREP received\n");
            }

            aodv->stats.numRoutes++;
            aodv->stats.numHops += hopCount;

            newRtAdded = TRUE;
            rtToDest = newRtPtr;
        }
        else
        {
            aodv->stats.numRoutes++;
            aodv->stats.numHops += hopCount;
            routeUpdated = TRUE;
            rtToDest = prevRtPtr;

            if (AODV_DEBUG)
            {
                printf("\tcontains better route compared to known one\n");
            }
        }
    }

    // Source of the route
    if (AodvIpIsMyIP(node, sourceAddress))
    {
        if (routeUpdated || newRtAdded)
        {
            Message* newMsg = NULL;
            Address previousHop;
            if (AODV_DEBUG)
            {
                printf("\tis my packet\n");
            }

            aodv->stats.numReplyRecvedAsSource++;

            AodvDeleteSent(destinationAddress, &aodv->sent);

            newMsg = AodvGetBufferedPacket(
                        node,
                        destinationAddress,
                        &previousHop,
                        &aodv->msgBuffer);

            // Send any buffered packets to the destination
            while (newMsg)
            {
                aodv->stats.numDataInitiated++;

                AodvTransmitData(
                    node,
                    newMsg,
                    rtToDest,
                    previousHop);

                newMsg = AodvGetBufferedPacket(
                            node,
                            destinationAddress,
                            &previousHop,
                            &aodv->msgBuffer);
            } // while
        }
    } // if source

    // Intermediate node of the route
    else
    {
        if (AODV_DEBUG)
        {
            printf("\tintermediate node of the route\n");
        }
        if (newRtAdded || routeUpdated)
        {
            // Forward the packet to the upstream of the route
            AodvRelayRREP(node, msg, rtToDest);
        } // if new route

        else
        {
            if (AODV_DEBUG)
            {
                printf("\told seqno\n");
            }
        }
    }
}

// /**
// FUNCTION   :: AodvSendRouteErrorForLinkFailure
// LAYER      :: NETWORK
// PURPOSE    :: Processing procedure when the node fails to deliver a data
//               packet to a particular destination, and need to send a
//               route error.
// PARAMETERS ::
//  +node:  Node* : Pointer to Node.
//  +aodv:  AodvData* : Pointer to AODV data.
//  +nextHopAddress:  Address : Next Hop Address.
// RETURN     :: void : NULL.
// **/
static
void AodvSendRouteErrorForLinkFailure(
         Node* node,
         AodvData* aodv,
         Address nextHopAddress)
{
    BOOL isUpstreamUnique = TRUE;
    Address upstreamAddr;
    int numberRouteDestinations = 0;
    AodvRerrPacket* newRerrPkt = NULL;
    AodvAddrSeqInfo* destAddrSeq = NULL;
    Aodv6AddrSeqInfo* dest6AddrSeq = NULL;
    AodvRouteEntry* current = NULL;
    int i = 0;
    BOOL IPV6 = FALSE;

    memset(&upstreamAddr, 0, sizeof(Address));

    // Calculate number of destination which are not reachable for the lost
    // of the link toward this next hop

    AodvCalculateNumInactiveRouteByNextHop(
        &aodv->routeTable,
        nextHopAddress,
        &numberRouteDestinations,
        &isUpstreamUnique,
        &upstreamAddr);

    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            &nextHopAddress, address);

        printf("\tnext hop to the destination is %s\n"
            "\tnumber of destinations using the route is %u\n",
            address, numberRouteDestinations);
    }

    if (numberRouteDestinations)
    {
        //  There are unreachable destinations so allocate rerr packet and
        // send it
        int size = 0;
        aodv->stats.numRerrInitiated++;

        if (nextHopAddress.networkType == NETWORK_IPV6)
        {
            newRerrPkt = (AodvRerrPacket *) MEM_malloc(
                                                sizeof(AodvRerrPacket)
                                                + sizeof(Aodv6AddrSeqInfo)
                                                * numberRouteDestinations);

            memset(
                newRerrPkt,
                0,
                sizeof(AodvRerrPacket) + sizeof(Aodv6AddrSeqInfo)
                * numberRouteDestinations);

            dest6AddrSeq = (Aodv6AddrSeqInfo *)
                      (((char *)newRerrPkt) + sizeof(AodvRerrPacket));
            IPV6 = TRUE;
        }
        else
        {
            newRerrPkt = (AodvRerrPacket *) MEM_malloc(
                                                sizeof(AodvRerrPacket)
                                                + sizeof(AodvAddrSeqInfo)
                                                * numberRouteDestinations);

            memset(
                newRerrPkt,
                0,
                sizeof(AodvRerrPacket) + sizeof(AodvAddrSeqInfo)
                * numberRouteDestinations);

            destAddrSeq = (AodvAddrSeqInfo *)
                      (((char *)newRerrPkt) + sizeof(AodvRerrPacket));
        }

        // Now allocate the rerr packet
        newRerrPkt->typeBitsDestCount |= (AODV_RERR << 24);
        newRerrPkt->typeBitsDestCount &= (~AODV_N_BIT);
        newRerrPkt->typeBitsDestCount |=
            (numberRouteDestinations & AODV_DEST_COUNT_BITS);


        // Allocate the destinations in the inactive destination field

        for (i = 0; i < AODV_ROUTE_HASH_TABLE_SIZE; i++)
        {
            for (current = (&aodv->routeTable)->routeHashTable[i];
                 current != NULL;
                 current = current->hashNext)
            {
                if (Address_IsSameAddress(&current->nextHop,&nextHopAddress)
                    && (current->activated == TRUE))
                {
                    if (current->precursors.size)
                    {
                        if (!IPV6)
                        {
                            destAddrSeq->address =
                                current->destination.interfaceAddr.ipv4;
                            destAddrSeq->seqNum = (current->destSeqNum) + 1;
                            destAddrSeq++;
                        }
                        else
                        {
                            COPY_ADDR6(
                                current->destination.interfaceAddr.ipv6,
                                dest6AddrSeq->address);

                            dest6AddrSeq->seqNum = (current->destSeqNum)+ 1;
                            dest6AddrSeq++;
                        }

                    }
                    if (AODV_DEBUG)
                    {
                        char address[MAX_STRING_LENGTH];

                        IO_ConvertIpAddressToString(
                            &current->destination,
                            address);

                        printf("\tdisabling route to dest %s\n", address);
                    }
                }
            }
        }

        if (IPV6)
        {
            size = sizeof(AodvRerrPacket) + (newRerrPkt->AODV_REER_DestCount)
                    * sizeof(Aodv6AddrSeqInfo);
        }
        else
        {
            size = sizeof(AodvRerrPacket) + (newRerrPkt->AODV_REER_DestCount)
                    * sizeof(AodvAddrSeqInfo);
        }

        // AodvRerrPacket allocation is complete so now send the packet
        AodvSendRouteErrorPacket(
            node,
            newRerrPkt,
            size,
            isUpstreamUnique,
            upstreamAddr);
    }

    for (i = 0; i < AODV_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (current = (&aodv->routeTable)->routeHashTable[i];
             current != NULL;
             current = current->hashNext)
        {

            if (Address_IsSameAddress(&current->nextHop, &nextHopAddress)
                && (current->activated == TRUE))
            {
                AodvDisableRoute(
                    node,
                    current,
                    &aodv->routeTable,
                    TRUE);
            }
        }
    }
}


// /**
// FUNCTION: AodvHandleLinkFailure
// LAYER    : NETWORK
// PURPOSE:  Processing procedure when the node fails to deliver a data
//           packet to a particular destination, it may try to locally
//           repair the link ie. finding another route toward the
//           destination or may send route error
// ARGUMENTS:
//  +node:Node*:The node failed to send data
//  +aodv:AodvData*:Aodv main data structure
//  +msg:Message*:The message whose transmission failed
//  +nextHopAddress:Address:Next hop for the destination
//  +destAddr:Address:Destination of the message
// RETURN:
// BOOL:TRUE or FALSE, depending on whether the node is doing local
//      repair.
// **/

static
BOOL AodvHandleLinkFailure(
    Node* node,
    AodvData* aodv,
    const Message* msg,
    Address nextHopAddress,
    Address destAddr)
{
    // Removes the message from queue destined to the next hop to which
    // message transmission failed
    BOOL isDoingLocalRepair = FALSE;

    char errorStr[MAX_STRING_LENGTH];



    if (nextHopAddress.networkType != NETWORK_INVALID)
    {
    //Change accordingly for IPv6
    if (nextHopAddress.networkType == NETWORK_IPV6)
    {
        Ipv6DeleteOutboundPacketsToANode(
            node,
            nextHopAddress.interfaceAddr.ipv6,
            aodv->broadcastAddr.interfaceAddr.ipv6,
            FALSE);
    }
    else
    {
        NetworkIpDeleteOutboundPacketsToANode(
            node,
            nextHopAddress.interfaceAddr.ipv4,
            ANY_IP,
            FALSE);
    }
    }
    else
    {
         //do nothing
         sprintf(errorStr,"Invalid Previous Hop Address !");
         ERROR_ReportWarning(errorStr);
         return isDoingLocalRepair;
    }//end


    if (((nextHopAddress.interfaceAddr.ipv4 == ANY_IP)
        && (nextHopAddress.networkType == NETWORK_IPV4))
        || ((IS_MULTIADDR6(nextHopAddress.interfaceAddr.ipv6))
        && (nextHopAddress.networkType == NETWORK_IPV6)))
    {
        // Mac failed to broadcast so do nothing
        return isDoingLocalRepair;
    }

    if (aodv->localRepair == FALSE)
    {
        // Local repair is not enabled in this node so directly send rerr
        AodvSendRouteErrorForLinkFailure(node, aodv, nextHopAddress);
    }
    else
    {
        // Local repair is enabled in this node so try to locally repair for
        // the destination to which packet transmission failed if the source
        // is not the node itself.

        AodvData* aodv = NULL;
        ip6_hdr* ip6Header = NULL;
        IpHeaderType* ipHeader = NULL;
        Address src;
        Address nullAddress;
        if (destAddr.networkType == NETWORK_IPV6)
        {
            aodv = (AodvData*) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_AODV6,
                                NETWORK_IPV6);

            ip6Header = (ip6_hdr*) MESSAGE_ReturnPacket(msg);

            SetIPv6AddressInfo(&src,
                                 ip6Header->ip6_src);
            nullAddress.networkType = NETWORK_IPV6;
            CLR_ADDR6(nullAddress.interfaceAddr.ipv6);

        }
        else
        {
            aodv = (AodvData*) NetworkIpGetRoutingProtocol(node,
                                            ROUTING_PROTOCOL_AODV,
                                            NETWORK_IPV4);
            ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
            SetIPv4AddressInfo(&src,
                                 ipHeader->ip_src);
            SetIPv4AddressInfo(&nullAddress,0);
        }

        BOOL isValidRt = FALSE;
        BOOL isValidSrc = FALSE;

        AodvRouteEntry* rtEntry = AodvCheckRouteExist(
                                      destAddr,
                                      &aodv->routeTable,
                                      &isValidRt);

        AodvRouteEntry* rtToSource = AodvCheckRouteExist(
                src, &aodv->routeTable, &isValidSrc);

        unsigned int lastHopCountToDest = 0;

        /*if (isValidRt)
        {
            lastHopCountToDest = rtEntry->hopCount;
        }
        else*/ if (rtEntry)
        {
            lastHopCountToDest = rtEntry->lastHopCount;
        }
        else
        {
            lastHopCountToDest = (unsigned int) AODV_INFINITY;
        }

        AodvMarkRouteAsLocallyRepairable(
            node,
            aodv,
            nextHopAddress);

        if (AODV_DEBUG_LOCAL_REPAIR)
        {
            printf("Node %u is trying to do local repair\n", node->nodeId);
        }

        // We don't need local repair if source detects the failure. In this
        // case it will proceed the usual way that is to deleting the route
        // entry and if necessary will search for another route

        if (!AodvIpIsMyIP(node,src) &&
            (lastHopCountToDest < AODV_MAX_REPAIR_TTL) &&
         //   (isValidRt || rtEntry->locallyRepairable) &&
            isValidSrc)
        {
            unsigned int ttl = 0;

            // Make sure that the source node don't get a copy of the rreq
            ttl = MAX(AODV_MIN_REPAIR_TTL,
                (unsigned int) rtToSource->hopCount / 2) +
                AODV_LOCAL_ADD_TTL;

            if (AODV_DEBUG_LOCAL_REPAIR)
            {
                printf("\tttl %d and source hop count %u\n",
                    ttl, rtToSource->hopCount);
            }

            if (ttl >= (unsigned int) rtToSource->hopCount)
            {
                if (AODV_DEBUG_LOCAL_REPAIR)
                {
                    printf("\tCan't do local repair as it will reach "
                        "source\n");
                }

                // Can't do local repair as with the ttl the rreq will reach
                // the source node
                if (AODV_DEBUG_LOCAL_REPAIR)
                {
                    char address[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(&destAddr, address);
                    printf("\tsending route error for %s\n", address);
                }

                aodv->stats.numRerrInitiated++;

                AodvSendRouteErrorForUnreachableDest(
                    node,
                    aodv,
                    destAddr,
                    nullAddress,
                    FALSE);

                rtEntry->locallyRepairable = FALSE;
            }
            else
            {
                // Other routes using the same link MUST be marked as
                // broken, but the node handling the local repair MAY flag
                // each such newly broken route as locally repairable;
                // this local repair flag in the route table MUST be reset
                // when the route times out (i.e., after the route has
                // been not been active for ACTIVE_ROUTE_TIMEOUT). Sec: 8.12


                if (AODV_DEBUG_LOCAL_REPAIR)
                {
                    printf("\tinitiating local repair\n");
                }

                isDoingLocalRepair = TRUE;

                // Send rreq for the destination.
                AodvInitiateRREQForLocalRepair(node, destAddr, ttl);
            }
        }
        else
        {
           if (!AodvCheckSent(destAddr, &aodv->sent))
           {
               // Send route error if the node is not trying to local repair
               if (rtEntry && !(rtEntry->locallyRepairable))
               {
                   AodvSendRouteErrorForLinkFailure(
                       node,
                       aodv,
                       nextHopAddress);
               }
               else
               {
                   aodv->stats.numRerrInitiated++;

                   AodvSendRouteErrorForUnreachableDest(
                       node,
                       aodv,
                       destAddr,
                       nextHopAddress,
                       TRUE);
               }

               if (rtEntry)
               {
                   rtEntry->locallyRepairable = FALSE;
               }
           }
        }
    }
    return isDoingLocalRepair;
}


// /**
// FUNCTION: AodvHandleRouteErrorWithNBitSet
// LAYER    : NETWORK
// PURPOSE:  Processing procedure when the node receives a route error
//           message with N bit set. If it not received from next hop
//           towards the destination, the message is ignored else the
//           node may forward the rerr toward the source if it has
//           non empty precursors list.
// ARGUMENTS:
//  +node:Node*:The node received rerr
//  +msg:Message*:Message containing rerr packet
//  +srcAddr:Address:address of the node
// RETURN   ::void:NULL
// **/

static
void AodvHandleRouteErrorWithNBitSet(
         Node* node,
         Message* msg,
         Address srcAddr)
{
    BOOL isValidRt = FALSE;
    AodvData* aodv =NULL;
    AodvRerrPacket* rerrPkt = (AodvRerrPacket *) MESSAGE_ReturnPacket(msg);
    AodvAddrSeqInfo* destAddrSeqInfo = NULL;
    Address destAddr;
    BOOL IPV6 = FALSE;
    Aodv6AddrSeqInfo* destAddr6SeqInfo = NULL;
    if (srcAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
            node,
            ROUTING_PROTOCOL_AODV6,
            NETWORK_IPV6);
        destAddr6SeqInfo =
        (Aodv6AddrSeqInfo *) (((char *)rerrPkt) + sizeof(AodvRerrPacket));
        SetIPv6AddressInfo(&destAddr,destAddr6SeqInfo->address);
        IPV6 = TRUE;
    }
    else
    {
         aodv = (AodvData *) NetworkIpGetRoutingProtocol(
             node,
             ROUTING_PROTOCOL_AODV,
             NETWORK_IPV4);
         destAddrSeqInfo =
          (AodvAddrSeqInfo *) (((char *)rerrPkt) + sizeof(AodvRerrPacket));
         SetIPv4AddressInfo(&destAddr,destAddrSeqInfo->address);
    }

    AodvRouteEntry* rtEntry = AodvCheckRouteExist(
        destAddr, &aodv->routeTable, &isValidRt);

    if (AODV_DEBUG_LOCAL_REPAIR)
    {
        char address[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&destAddr, address);
        printf("Node %u received route error with N bit set\n"
            "\tdestination %s\n", node->nodeId, address);
    }

    // A node which receives a RERR message with the 'N' flag set MUST
    // NOT delete the route to that destination.  The only action taken
    // should be the retransmission of the message, if the RERR arrived
    // from the next hop along that route, and if there are one or more
    // precursor nodes for that route to the destination.  When the source
    // node receives a RERR message with the 'N' flag set, if this message
    // came from its next hop along its route to the destination then the
    // source node MAY choose to reinitiate route discovery. Sec: 8.12

    if ((rtEntry) && (Address_IsSameAddress(&rtEntry->nextHop,&srcAddr) &&
        (rtEntry->precursors.size == 0)))
    {
        // The node is the source it may want to re initiate route to
        // the destination.

        if (AODV_DEBUG_LOCAL_REPAIR)
        {
            printf("\tAssuming this node is the source node\n"
                    "\tSize of precursor %u\n",
                rtEntry->precursors.size);
        }

        if (aodv->findAlternateRtIfNSet == TRUE)
        {
            // initiate RREQ

            if (AODV_DEBUG_LOCAL_REPAIR)
            {
                printf("\tThis is source and initiating RREQ\n");
            }

            aodv->stats.numRequestForAlternateRt++;

            if (rtEntry->activated == TRUE)
            {
                AodvDisableRoute(node, rtEntry, &aodv->routeTable, TRUE);
            }

            AodvInitiateRREQ(node, destAddr);
        }
    }
    else if (rtEntry && (Address_IsSameAddress(&rtEntry->nextHop,&srcAddr)
            && (rtEntry->precursors.size)))
    {
        // Relay the rerr
        AodvRerrPacket* newRerrPacket = NULL;
        AodvAddrSeqInfo* AddrSeq = NULL;
        Aodv6AddrSeqInfo* addr6Seq = NULL;
        int size;
        int sizeofAddrSeqinfo = 0;
        Address broadcastAddress;

        if (AODV_DEBUG_LOCAL_REPAIR)
        {
            printf("\trelaying route error with N bit set\n");
        }

        if (IPV6)
        {
             sizeofAddrSeqinfo = sizeof(Aodv6AddrSeqInfo);
             SetIPv6AddressInfo(
                 &broadcastAddress,
                 aodv->broadcastAddr.interfaceAddr.ipv6);
        }
        else
        {
            sizeofAddrSeqinfo = sizeof(AodvAddrSeqInfo);
            SetIPv4AddressInfo(&broadcastAddress,ANY_IP);
        }

        newRerrPacket = (AodvRerrPacket *) MEM_malloc(
                                sizeof(AodvRerrPacket)
                                + sizeofAddrSeqinfo);

        memset(
            newRerrPacket,
            0,
            sizeof(AodvRerrPacket) + sizeofAddrSeqinfo);

        newRerrPacket->typeBitsDestCount |= (AODV_RERR << 24);

        newRerrPacket->typeBitsDestCount |= AODV_N_BIT;
        newRerrPacket->typeBitsDestCount |= 1;
        if (IPV6)
        {
            addr6Seq = (Aodv6AddrSeqInfo *)
                  (((char *)newRerrPacket) + sizeof(AodvRerrPacket));
            COPY_ADDR6(destAddr.interfaceAddr.ipv6,addr6Seq->address);
            addr6Seq->seqNum = destAddr6SeqInfo->seqNum;
        }
        else
        {
            AddrSeq = (AodvAddrSeqInfo *)
                  (((char *)newRerrPacket) + sizeof(AodvRerrPacket));
            AddrSeq->address = destAddr.interfaceAddr.ipv4;
            AddrSeq->seqNum = destAddrSeqInfo->seqNum;
        }

        size = sizeof(AodvRerrPacket) + (newRerrPacket->AODV_REER_DestCount)
                   * sizeofAddrSeqinfo;

        aodv->stats.numRerrForwardedWithNFlag++;

        if (rtEntry->precursors.size == 1)
        {
            AodvSendRouteErrorPacket(
                node,
                newRerrPacket,
                size,
                TRUE,
                rtEntry->precursors.head->destAddr);
        }
        else
        {
            AodvSendRouteErrorPacket(
                node,
                newRerrPacket,
                size,
                FALSE,
                broadcastAddress);
        }
    }
    else
    {
        if (AODV_DEBUG_LOCAL_REPAIR)
        {
            printf("\tdiscarding route error with N bit set\n");
        }

        // Do nothing
        aodv->stats.numRerrDiscarded++;
    }
}


// /**
// FUNCTION: AodvHandleRouteError
// LAYER: NETWORK
// PURPOSE:  Processing procedure when RERR is received
// PARAMETERS:
// +node:Node*:Pointer to node(The node received route error)
// +msg:Message *:Message containing route error packet
// +srcAddr:Address:address of the node sent the rerr
// +interfaceIndex:int:Interface Index
// RETURN   :void:NULL
// **/

static
void AodvHandleRouteError(
         Node* node,
         Message* msg,
         Address srcAddr,
         int interfaceIndex)
{
    AodvData* aodv = NULL;
    BOOL IPV6 = FALSE;
    int sizeofAddrseq = 0;
    if (srcAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *)
            NetworkIpGetRoutingProtocol(node,
            ROUTING_PROTOCOL_AODV6,
            NETWORK_IPV6);
        sizeofAddrseq = sizeof(Aodv6AddrSeqInfo);
        IPV6 = TRUE;
    }
    else
    {
        aodv = (AodvData *)
            NetworkIpGetRoutingProtocol(node,
            ROUTING_PROTOCOL_AODV,
            NETWORK_IPV4);
        sizeofAddrseq = sizeof(AodvAddrSeqInfo);
    }
    DataBuffer unreachableDestList;
    int destCount = 0;
    int i;
    BOOL ifOneUpstream = TRUE;
    Address upstreamAddr;
    AodvRouteEntry* ptrToPrevHop;

    AodvRerrPacket* rerrPkt = (AodvRerrPacket *) MESSAGE_ReturnPacket(msg);

    // When a node receives an AODV control packet from a neighbor, it
    // checks its route table for an entry for that neighbor.  In the
    // event that there is no corresponding entry for that neighbor, an
    // entry is created.  The sequence number is either determined from
    // the information contained in the control packet (i.e.,
    // the neighbor is the source of a RREQ), or else it is initialized
    // to zero if the sequence number for that node can not be
    // determined.  The lifetime for the routing table entry is either
    // determined from the control packet (i.e., the neighbor is the
    // originator of a RREP for itself), or it is initialized to
    // ALLOWED_HELLO_LOSS * HELLO_INTERVAL. The hopcount to the neighbor
    // is set to one. sec 6.2 draft 10

    ptrToPrevHop = AodvReplaceInsertRouteTable(
                       node,
                       srcAddr,
                       0,
                       1,
                       srcAddr,
                       node->getNodeTime() + AODV_ACTIVE_ROUTE_TIMEOUT,
                       TRUE,
                       interfaceIndex,
                       &aodv->routeTable);

    if (aodv->isExpireTimerSet == FALSE)
    {

        aodv->isExpireTimerSet = TRUE;

        if (AODV_DEBUG)
        {
            printf("Node %u is setting timer "
                "MSG_NETWORK_CheckRouteTimeout\n", node->nodeId);
        }


        AodvSetTimer(
            node,
            MSG_NETWORK_CheckRouteTimeout,
            srcAddr,
            (clocktype) AODV_ACTIVE_ROUTE_TIMEOUT);
    }


    if (rerrPkt->typeBitsDestCount & AODV_N_BIT)
    {
        // if the N bit is set we don't want to delete the entry of the
        // destination
        AodvHandleRouteErrorWithNBitSet(
            node,
            msg,
            srcAddr);

        aodv->stats.numRerrRecvedWithNFlag++;
        return;
    }

    // Rerr without N flag set
    aodv->stats.numRerrRecved++;

    // Check which of the destinations in the RERR has non empty precursors
    // list those entries should be included in the rerr that this node will
    // forward
    unreachableDestList = AodvCalculateNumInactiveRouteByDest(
                              node,
                              msg,
                              srcAddr,
                              &aodv->routeTable,
                              &ifOneUpstream,
                              &upstreamAddr);

    destCount = BUFFER_GetCurrentSize(&unreachableDestList) / sizeofAddrseq ;

    if (destCount)
    {
        // Allocate the rerr packet
        AodvRerrPacket* newRerrPacket = (AodvRerrPacket *)
                                            MEM_malloc(sizeof(AodvRerrPacket)
                                            + sizeofAddrseq
                                            * destCount);

        memset(
            newRerrPacket,
            0,
            sizeof(AodvRerrPacket) + sizeofAddrseq * destCount);

        AodvAddrSeqInfo* addrSeqToBeSent = NULL;
        Aodv6AddrSeqInfo* addr6SeqToBeSent = NULL;
        Aodv6AddrSeqInfo* dest6AddrSeq = NULL;
        AodvAddrSeqInfo* destAddrSeq = NULL;
        if (IPV6)
        {
            dest6AddrSeq = (Aodv6AddrSeqInfo *)
                                BUFFER_GetData(&unreachableDestList);
        }
        else
        {
            destAddrSeq = (AodvAddrSeqInfo *)
                                BUFFER_GetData(&unreachableDestList);
        }

        aodv->stats.numRerrForwarded++;

        newRerrPacket->typeBitsDestCount |= (AODV_RERR << 24);
        newRerrPacket->typeBitsDestCount &= (~AODV_N_BIT);
        newRerrPacket->typeBitsDestCount |=
            (destCount & AODV_DEST_COUNT_BITS);
        if (IPV6)
        {
            addr6SeqToBeSent = (Aodv6AddrSeqInfo *)
                          (((char *)newRerrPacket) + sizeof(AodvRerrPacket));
        }
        else
        {
            addrSeqToBeSent = (AodvAddrSeqInfo *)
                          (((char *)newRerrPacket) + sizeof(AodvRerrPacket));
        }

        for (i = 0; i < destCount; i++)
        {
            if (IPV6)
            {
                COPY_ADDR6(dest6AddrSeq[i].address,
                                    addr6SeqToBeSent[i].address);
                addr6SeqToBeSent[i].seqNum = dest6AddrSeq[i].seqNum;
                Ipv6DeleteOutboundPacketsToANode(
                    node,
                    srcAddr.interfaceAddr.ipv6,
                    dest6AddrSeq[i].address,
                    FALSE);
            }
            else
            {
                addrSeqToBeSent[i].address = destAddrSeq[i].address;
                addrSeqToBeSent[i].seqNum = destAddrSeq[i].seqNum;
                NetworkIpDeleteOutboundPacketsToANode(
                    node,
                    srcAddr.interfaceAddr.ipv4,
                    destAddrSeq[i].address,
                    FALSE);
            }
        }


        // rerr packet allocation is complete send out the packet
        AodvSendRouteErrorPacket(
            node,
            newRerrPacket,
            BUFFER_GetCurrentSize(&unreachableDestList) +
                sizeof(AodvRerrPacket),
            ifOneUpstream,
            upstreamAddr);
    }
    BUFFER_DestroyDataBuffer(&unreachableDestList);
}


// /**
// FUNCTION: AodvHandleData
// LAYER: NETWORK
// PURPOSE:  Processing procedure when data is received from another node.
//           this node is either intermediate hop or destination of the data
// PARAMETERS:
// +node: Node*:Pointer to node(The node which has received data)
// +msg:Message*:Pointer to message(The message received)
// +destAddr:Address:The destination for the packet
// +previousHopAddress:Address:Previous Hop Destination
// RETURN   ::void:Null
// **/

static
void AodvHandleData(
         Node* node,
         Message* msg,
         Address destAddr,
         Address previousHopAddress)
{
    AodvData* aodv =    NULL;
    IpHeaderType* ipHeader = NULL;
    ip6_hdr* ip6Header  =   NULL;
    Address sourceAddress;
    if (destAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_AODV6,
                                NETWORK_IPV6);
        ip6Header = (ip6_hdr *) MESSAGE_ReturnPacket(msg);
        SetIPv6AddressInfo(&sourceAddress,
                            ip6Header->ip6_src);


    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_AODV,
                                NETWORK_IPV4);
        ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
        SetIPv4AddressInfo(&sourceAddress,
                              ipHeader->ip_src);

    }

    // the node is the destination of the route
    if (AodvIpIsMyIP(node, destAddr))
    {
        aodv->stats.numDataRecved++;

        if (AODV_DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            printf("\tis my packet, so let IP handle it\n");
        }

        AodvUpdateLifetime(
            node,
            aodv,
            sourceAddress,
            &aodv->routeTable,
            -1);

        if (!Address_IsSameAddress(&sourceAddress, &previousHopAddress))
        {
            AodvUpdateLifetime(
                node,
                aodv,
                previousHopAddress,
                &aodv->routeTable,
                1);
        }
    }
    else
    {
        AodvRouteEntry* rtToDest = NULL;
        BOOL isValidRoute = FALSE;

        if (AODV_DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            printf("\tnot my packet, so need to route\n");
        }

        // The node is an intermediate node of the route.
        // Relay the packet to the next hop of the route
        rtToDest = AodvCheckRouteExist(
                       destAddr,
                       &aodv->routeTable,
                       &isValidRoute);

        if (rtToDest && (!isValidRoute) && aodv->localRepair)
        {
            int ttl = 0;
            ttl = rtToDest->lastHopCount + AODV_LOCAL_ADD_TTL;
            rtToDest->lifetime = node->getNodeTime() + AODV_DELETE_PERIOD;
            AodvInitiateRREQForLocalRepair(
                node,
                destAddr,
                ttl);
        }

        if (rtToDest &&  isValidRoute)
        {
            // There is a valid route towards the destination
            // update the lifetime for previous hop destination and
            // source

            aodv->stats.numDataForwarded++;

            AodvTransmitData(
                node,
                msg,
                rtToDest,
                previousHopAddress);

            if (AODV_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("\troute exists, so send packet out\n");
            }
        }
        else
        {
            // There is no valid route for the destination but rreq has been
            // sent for the destination so possibly the node is trying for
            // local repair. So buffer the packet

            if (AodvCheckSent(destAddr, &aodv->sent))
            {
                // Data packets waiting for a route (i.e., waiting for a
                // RREP after RREQ has been sent) SHOULD be buffered.
                // The buffering SHOULD be FIFO. Sec: 8.3

                AodvInsertBuffer(
                    node,
                    msg,
                    destAddr,
                    previousHopAddress,
                    &aodv->msgBuffer);
            }
            else
            {
                // No rreq has been sent for the route
                if (aodv->localRepair && rtToDest && rtToDest->
                    locallyRepairable)
                {
                    // Try to do local repair if possible
                    BOOL ifLocalRepairPossible = FALSE;

                    ifLocalRepairPossible =
                        AodvHandleLinkFailure(
                            node,
                            aodv,
                            msg,
                            rtToDest->nextHop,
                            destAddr);

                    if (ifLocalRepairPossible)
                    {
                        AodvInsertBuffer(
                            node,
                            msg,
                            destAddr,
                            previousHopAddress,
                            &aodv->msgBuffer);
                    }
                    else
                    {
                        aodv->stats.numDataDroppedForNoRoute++;
                        MESSAGE_Free(node, msg);
                    }
                }
                else
                {
                    aodv->stats.numRerrInitiated++;

                    AodvSendRouteErrorForUnreachableDest(
                        node,
                        aodv,
                        destAddr,
                        previousHopAddress,
                        TRUE);

                    aodv->stats.numDataDroppedForNoRoute++;

                    MESSAGE_Free(node,msg);
                }
            }
        }
    }
}


// /**
// FUNCTION: AodvMacLayerStatusHandler
// LAYER: NETWORK
// PURPOSE:  Reacts to the signal sent by the MAC protocol after link
//           failure
// PARAMETERS:
// +node:Node*:Pointer to Node
// +msg:Message*:Pointer to message,the message not delivered
// +nextHopAddress:Address:Next Hop Address
// +incomingInterface:int:The interface in which the message was sent
// RETURN   ::void:Null
// **/

void
AodvMacLayerStatusHandler(
    Node* node,
    const Message* msg,
    const Address genNextHopAddress,
    const int incomingInterface)
{
    AodvData* aodv = NULL;

    IpHeaderType* ipHeader = NULL;
    ip6_hdr* ip6Header = NULL;
    Address destAddr;
    BOOL IPV6 = FALSE;
    Address nextHopAddress;

    memcpy(&nextHopAddress, &genNextHopAddress, sizeof(Address));

    if (nextHopAddress.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
           node,
           ROUTING_PROTOCOL_AODV6,
           NETWORK_IPV6);
        IPV6 = TRUE;

    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
            node,
            ROUTING_PROTOCOL_AODV,
            NETWORK_IPV4);
    }

    ERROR_Assert(MESSAGE_GetEvent(msg) == MSG_NETWORK_PacketDropped,
                 "AODV: Unexpected event in MAC layer status handler.\n");
    if (IPV6)
    {
        ip6Header = (ip6_hdr *) MESSAGE_ReturnPacket(msg);
        SetIPv6AddressInfo(&destAddr,ip6Header->ip6_dst);
    }
    else
    {
        ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
        SetIPv4AddressInfo(&destAddr, ipHeader->ip_dst);
    }

    if (AODV_DEBUG)
    {
        char address[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&destAddr, address);
        printf("Node %u mac failed to deliver packet in interface %d\n"
            "\tdestination %s\n", node->nodeId, incomingInterface,
            address);
    }

    aodv->stats.numBrokenLinks++;

    if (IPV6)
    {
        if (Address_IsSameAddress(&nextHopAddress,&aodv->broadcastAddr))
        {
            return;
        }
    }
    else
    {
        if (GetIPv4Address(nextHopAddress) == ANY_DEST)
        {
            return;
        }
    }

    if ((ipHeader && ipHeader->ip_p == IPPROTO_AODV)
        || (ip6Header && ip6Header->ip6_nxt == IPPROTO_AODV))
    {
        //char* type = NULL;
        unsigned int type = 0;
        if (IPV6)
        {
            type = *((unsigned int*)(MESSAGE_ReturnPacket(msg)
                        + sizeof(ip6_hdr)));
        }
        else
        {
            type = *((unsigned int*)(MESSAGE_ReturnPacket(msg)
                        + (IpHeaderGetHLen(ipHeader->ip_v_hl_tos_len) * 4)));

        }

        type = type >> 24;

        if (type == AODV_RREP)
        {
            // Failed to send Route Reply
            // Add the next hop in black list

            // when a node detects that its transmission of an RREP message
            // has failed, it remembers the next-hop of the failed RREP
            // in a ``blacklist'' set.  A node ignores all RREQs received
            // from any node in its blacklist set. Nodes are removed from
            // the blacklist set after a BLACKLIST_TIMEOUT period. Sec: 8.8

            AodvInsertBlacklistTable(
                node,
                aodv,
                nextHopAddress,
                &aodv->blacklistTable);

        }
        return;
    }

    AodvHandleLinkFailure(
        node,
        aodv,
        msg,
        nextHopAddress,
        destAddr);
}


// /**
// FUNCTION: AodvRouterFunction
// LAYER   : NETWROK
// PURPOSE : Determine the routing action to take for the given data packet
//          set the PacketWasRouted variable to TRUE if no further handling
//          of this packet by IP is necessary
// PARAMETERS:
// +node:Node *::Pointer to node
// + msg:Message*:The packet to route to the destination
// +destAddr:Address:The destination of the packet
// +previousHopAddress:Address:Last hop of this packet
// +packetWasRouted:BOOL*:set to FALSE if ip is supposed to handle the
//                        routing otherwise TRUE
// RETURN   ::void:NULL
// **/

void
AodvRouterFunction(
    Node* node,
    Message* msg,
    Address destAddr,
    Address previousHopAddress,
    BOOL* packetWasRouted)
{
    AodvData* aodv=NULL;
    IpHeaderType* ipHeader = NULL;
    ip6_hdr* ip6Header = NULL;
    Address sourceAddress;
    BOOL IPV6 = FALSE;

    if (destAddr.networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_AODV6,
                                NETWORK_IPV6);
        ip6Header = (ip6_hdr *) MESSAGE_ReturnPacket(msg);
        IPV6 = TRUE;
        SetIPv6AddressInfo(&sourceAddress,
                             ip6Header->ip6_src);

    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_AODV,
                                NETWORK_IPV4);
        ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
        SetIPv4AddressInfo(&sourceAddress,
                              ipHeader->ip_src);

    }


    AodvRouteEntry* rtToDest = NULL;
    BOOL isValidRt = FALSE;

    // Control packets
 if ((ipHeader && (ipHeader->ip_p == IPPROTO_AODV))
      || (ip6Header && (ip6Header->ip6_nxt == IPPROTO_AODV)))
    {
        return;
    }

    if (AODV_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %u got packet\n", node->nodeId);
    }

    if (AodvIpIsMyIP(node, destAddr))
    {
        *packetWasRouted = FALSE;

    }
    else
    {
        *packetWasRouted = TRUE;
    }

    // intermediate node or destination of the route
    if (!AodvIpIsMyIP(node, sourceAddress))
    {
        AodvHandleData(node, msg, destAddr, previousHopAddress);
    }
    else
    {
        if (!(*packetWasRouted))//AodvIpIsMyIP(node, destAddr))
        {
            return;
        }

        rtToDest = AodvCheckRouteExist(
                       destAddr,
                       &aodv->routeTable,
                       &isValidRt);


        if (isValidRt)
        {
            // source has a route to the destination
            AodvTransmitData(node, msg, rtToDest, previousHopAddress);

            aodv->stats.numDataInitiated++;

            if (AODV_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("\thas route to destination, so send immediately\n");
            }
        }
        else if (!AodvCheckSent(destAddr, &aodv->sent))
        {
            // There is no route to the destination and RREQ has not been
            // sent

            AodvInsertBuffer(
                node,
                msg,
                destAddr,
                previousHopAddress,
                &aodv->msgBuffer);

            if (AODV_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("\tdon't have a route, so send RREQ\n");
            }

            AodvInitiateRREQ(node, destAddr);
        }
        else
        {
            // There is no route but RREQ has already been sent
            if (AODV_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("\talready sent RREQ, so buffer packet\n");
            }

            AodvInsertBuffer(
                node,
                msg,
                destAddr,
                previousHopAddress,
                &aodv->msgBuffer);
        }
    }
}

// /**
// FUNCTION     : AodvCheckDFlag
// LAYER        : NETWORK
// PURPOSE      : Returns true if D falg is to be set.
// PARAMETERS   :
// +node        :Node*              : Pointer to node
// +nodeInput   :const NodeInput*   : Address to be compared
// RETURN:
//  +TRUE       :BOOL               : If D falg to be set
//  +FALSE      :BOOL               : If D flag not to be set.
//  **/
BOOL
AodvCheckDFlag(Node* node, const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL wasFound;

    IO_ReadString(
        node->nodeId,
        ANY_DEST,
        nodeInput,
        "AODV-DEST-ONLY-NODE",
        &wasFound,
        buf);

    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            return TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            return FALSE;
        }
        else
        {
            ERROR_Assert(FALSE, "AODV-DEST-ONLY-NODE should be YES or NO\n");
        }
    }
    return FALSE;
}


// /**
// FUNCTION     : AodvCheckAFlag
// LAYER        : NETWORK
// PURPOSE      : Returns true if A falg is to be set.
// PARAMETERS   :
// +node        :Node*              : Pointer to node
// +nodeInput   :const NodeInput*   : Address to be compared
// RETURN:
//  +TRUE       :BOOL               : If A falg to be set
//  +FALSE      :BOOL               : If A flag not to be set.
//  **/
BOOL
AodvConfigureAFlag(Node* node, const NodeInput* nodeInput, int interfaceIndex)
{
    return FALSE;
}

// /**
// FUNCTION   :: AodvInitializeConfigurableParameters
// LAYER      :: NETWORK
// PURPOSE    :: To initialize the user configurable parameters or initialize
//               the corresponding variables with the default values as
//               specified in draft-ietf-manet-aodv-08.txt.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +nodeInput:  const NodeInput* : Pointer to Chached file.
//  +aodv:  AodvData* : Pointer to AODV Data.
//  +interfaceAddress:  Address : Interface Address.
// RETURN     :: void : NULL.
// **/
static
void AodvInitializeConfigurableParameters(
    Node* node,
    const NodeInput* nodeInput,
    AodvData* aodv,
    Address interfaceAddress)
{
    BOOL wasFound;
    char buf[MAX_STRING_LENGTH];
    UInt32 nodeId = node->nodeId;

    aodv->Dflag = AodvCheckDFlag(node, nodeInput);

    IO_ReadInt(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-NET-DIAMETER",
        &wasFound,
        &aodv->netDiameter);

    if (!wasFound)
    {
        aodv->netDiameter = AODV_DEFAULT_NET_DIAMETER;

    }
    else
    {
        ERROR_Assert(
                aodv->netDiameter > 0,
        "Invalid AODV-NET-DIAMETER configuration");
    }



    IO_ReadTime(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-NODE-TRAVERSAL-TIME",
        &wasFound,
        &aodv->nodeTraversalTime);

    if (!wasFound)
    {
        aodv->nodeTraversalTime = AODV_DEFAULT_NODE_TRAVERSAL_TIME;
    }
    else
    {
        ERROR_Assert(
               aodv->nodeTraversalTime > 0,
          "Invalid AODV_DEFAULT_NODE_TRAVERSAL_TIME configuration");
    }


    IO_ReadTime(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-ACTIVE-ROUTE-TIMEOUT",
        &wasFound,
        &aodv->activeRouteTimeout)
        ;
    if (!wasFound)
    {
        aodv->activeRouteTimeout = AODV_DEFAULT_ACTIVE_ROUTE_TIMEOUT;
    }
    else
    {
        ERROR_Assert(
                aodv->activeRouteTimeout > 0,
        "Invalid AODV_DEFAULT_ACTIVE_ROUTE_TIMEOUT configuration");
    }


    IO_ReadTime(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-MY-ROUTE-TIMEOUT",
        &wasFound,
        &aodv->myRouteTimeout);

    if (!wasFound)
    {
        aodv->myRouteTimeout = AODV_DEFAULT_MY_ROUTE_TIMEOUT;
    }
    else
    {
        ERROR_Assert(
                aodv->myRouteTimeout > 0,
        "Invalid AODV_DEFAULT_MY_ROUTE_TIMEOUT configuration");
    }


    IO_ReadInt(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-ALLOWED-HELLO-LOSS",
        &wasFound,
        &aodv->allowedHelloLoss);

    if (!wasFound)
    {
        aodv->allowedHelloLoss = AODV_DEFAULT_ALLOWED_HELLO_LOSS;
    }
    else
    {
        ERROR_Assert(
                aodv->allowedHelloLoss > 0,
        "Invalid AODV_DEFAULT_ALLOWED_HELLO_LOSS configuration");
    }


    IO_ReadInt(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-RREQ-RETRIES",
        &wasFound,
        &aodv->rreqRetries);

    if (!wasFound)
    {
        aodv->rreqRetries = AODV_DEFAULT_RREQ_RETRIES;
    }
    else
    {
        ERROR_Assert(
                aodv->rreqRetries >= 0,
        "Invalid AODV_DEFAULT_RREQ_RETRIES configuration");
    }


    IO_ReadTime(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-HELLO-INTERVAL",
        &wasFound,
        &aodv->helloInterval);

    if (!wasFound)
    {
        aodv->helloInterval = AODV_DEFAULT_HELLO_INTERVAL;
    }
    else
    {
        ERROR_Assert(
                aodv->helloInterval > 0,
        "Invalid AODV_DEFAULT_HELLO_INTERVAL configuration");
    }



    IO_ReadInt(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-ROUTE-DELETION-CONSTANT",
        &wasFound,
        &aodv->rtDeletionConstant);

    if (!wasFound)
    {
        aodv->rtDeletionConstant = AODV_DEFAULT_ROUTE_DELETE_CONST;
    }
    else
    {
        ERROR_Assert(
                aodv->rtDeletionConstant > 0,
        "Invalid AODV_DEFAULT_ROUTE_DELETE_CONST configuration");
    }


    IO_ReadString(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-PROCESS-HELLO",
        &wasFound,
        buf);

    if ((wasFound == FALSE) || (strcmp(buf, "NO") == 0))
    {
        aodv->processHello = FALSE;
        aodv->deletePeriod = (AODV_ROUTE_DELETE_CONST *
                                    AODV_ACTIVE_ROUTE_TIMEOUT);
    }
    else if (strcmp(buf, "YES") == 0)
    {
        aodv->processHello = TRUE;
        aodv->deletePeriod = (AODV_ROUTE_DELETE_CONST *
            MAX(AODV_ACTIVE_ROUTE_TIMEOUT, AODV_ALLOWED_HELLO_LOSS *
                                                AODV_HELLO_INTERVAL));
    }
    else
    {
        ERROR_ReportError("Needs YES/NO against AODV-PROCESS-HELLO");
    }


    IO_ReadString(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-LOCAL-REPAIR",
        &wasFound,
        buf);

    if ((wasFound == FALSE) || (strcmp(buf, "NO") == 0))
    {
        aodv->localRepair = FALSE;
    }
    else if (strcmp(buf, "YES") == 0)
    {
        aodv->localRepair = TRUE;
    }
    else
    {
        ERROR_ReportError("Needs YES/NO against AODV-LOCAL-REPAIR");
    }


    IO_ReadString(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-PROCESS-ACK",
        &wasFound,
        buf);

    if ((wasFound == FALSE) || (strcmp(buf, "NO") == 0))
    {
        aodv->processAck = FALSE;
    }
    else if (strcmp(buf, "YES") == 0)
    {
        aodv->processAck = TRUE;
    }
    else
    {
        ERROR_ReportError("Needs YES/NO against AODV-PROCESS-ACK");
    }


    IO_ReadString(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-SEARCH-BETTER-ROUTE",
        &wasFound,
        buf);

    if ((wasFound == FALSE) || (strcmp(buf, "NO") == 0))
    {
        aodv->findAlternateRtIfNSet = FALSE;
    }
    else if (strcmp(buf, "YES") == 0)
    {
        aodv->findAlternateRtIfNSet = TRUE;
    }
    else
    {
        ERROR_ReportError("Needs YES/NO against AODV-SEARCH-BETTER-ROUTE");
    }


    IO_ReadInt(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-BUFFER-MAX-PACKET",
        &wasFound,
        &aodv->bufferSizeInNumPacket);

    if (wasFound == FALSE)
    {
        aodv->bufferSizeInNumPacket = AODV_DEFAULT_MESSAGE_BUFFER_IN_PKT;
    }

    ERROR_Assert(aodv->bufferSizeInNumPacket > 0, "AODV-BUFFER-MAX-PACKET "
                 "needs to be a positive number\n");


    IO_ReadInt(
        node->nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-BUFFER-MAX-BYTE",
        &wasFound,
        &aodv->bufferSizeInByte);

    if (wasFound == FALSE)
    {
        aodv->bufferSizeInByte = 0;
    }

    ERROR_Assert(aodv->bufferSizeInByte >= 0, "AODV-BUFFER-MAX-BYTE "
                 "cannot be negative\n");

    IO_ReadString(
        nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-OPEN-BI-DIRECTIONAL-CONNECTION",
        &wasFound,
        buf);

    if ((wasFound == FALSE) || (strcmp(buf, "NO") == 0))
    {
        aodv->biDirectionalConn = FALSE;
    }
    else if (strcmp(buf, "YES") == 0)
    {
        aodv->biDirectionalConn = TRUE;
    }
    else
    {
        ERROR_ReportError("Needs YES/NO against "
            "AODV-OPEN-BI-DIRECTIONAL-CONNECTION");
    }

    IO_ReadInt(
        node->nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-TTL-START",
        &wasFound,
        &aodv->ttlStart);

    if (wasFound == FALSE)
    {
        aodv->ttlStart = AODV_DEFAULT_TTL_START;
    }

    ERROR_Assert(aodv->ttlStart > 0, "AODV_TTL_START should be > 0");

    IO_ReadInt(
        node->nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-TTL-INCREMENT",
        &wasFound,
        &aodv->ttlIncrement);

    if (wasFound == FALSE)
    {
        aodv->ttlIncrement = AODV_DEFAULT_TTL_INCREMENT;
    }

    ERROR_Assert(aodv->ttlIncrement > 0, "AODV_TTL_INCREMENT should be > 0");

    IO_ReadInt(
        node->nodeId,
        &interfaceAddress,
        nodeInput,
        "AODV-TTL-THRESHOLD",
        &wasFound,
        &aodv->ttlMax);

    if (wasFound == FALSE)
    {
        aodv->ttlMax = AODV_DEFAULT_TTL_THRESHOLD;
    }

    ERROR_Assert(aodv->ttlMax > 0, "AODV_TTL_THRESHOLD should be > 0");
}

// /*
// FUNCTION :: AodvInit.
// LAYER    :: NETWORK.
// PURPOSE  :: Initialization function for AODV protocol.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + aodvPtr : AodvData** : Pointer to pointer to AODV data.
// + nodeInput : const NodeInput* : Pointer to chached config file.
// + interfaceIndex : int : Interface Index.
// RETURN   :: void : NULL.
// **/
void
AodvInit(
    Node* node,
    AodvData** aodvPtr,
    const NodeInput* nodeInput,
    int interfaceIndex,
    NetworkRoutingProtocolType aodvProtocolType)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    AodvData* aodv = (AodvData *) MEM_malloc(sizeof(AodvData));

    BOOL retVal;
    char buf[MAX_STRING_LENGTH];
    int i = 0;
    Address destAddr;
    NetworkRoutingProtocolType protocolType;
    destAddr.networkType = NETWORK_INVALID;

    (*aodvPtr) = aodv;

    memset(aodv, 0, sizeof(AodvData));

    aodv->iface = (AodvInterfaceInfo *) MEM_malloc(
                                            sizeof(AodvInterfaceInfo)
                                            * node->numberInterfaces);

    memset(
        aodv->iface,
        0,
        sizeof(AodvInterfaceInfo) * node->numberInterfaces);

    RANDOM_SetSeed(aodv->aodvJitterSeed,
                   node->globalSeed,
                   node->nodeId,
                   aodvProtocolType);

// SG_ADD
    if (aodvProtocolType == ROUTING_PROTOCOL_AODV6){
        AodvCreateIpv6BroadcastAddress(node, &aodv->broadcastAddr);
    }else{
        SetIPv4AddressInfo(&aodv->broadcastAddr, ANY_DEST);
    }


    // Read whether statistics needs to be collected for the protocol
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTING-STATISTICS",
        &retVal,
        buf);

    if ((retVal == FALSE) || (strcmp(buf, "NO") == 0))
    {
        aodv->statsCollected = FALSE;
    }
    else if (strcmp(buf, "YES") == 0)
    {
        aodv->statsCollected = TRUE;
    }
    else
    {
        ERROR_ReportError("Needs YES/NO against STATISTICS");
    }

    aodv->statsPrinted = FALSE;

    // Check enability of AODV on particular interface and set respective
    // AODV flag for further use.
    for (i = 0; i < node->numberInterfaces; i++)
    {
        aodv->iface[i].AFlag = AodvConfigureAFlag(node, nodeInput, i);

        if (aodvProtocolType == ROUTING_PROTOCOL_AODV6
            && (NetworkIpGetInterfaceType(node, i) == NETWORK_IPV6
            || NetworkIpGetInterfaceType(node, i) == NETWORK_DUAL)
            && ip->interfaceInfo[i]->ipv6InterfaceInfo->
                    routingProtocolType == ROUTING_PROTOCOL_AODV6)
        {
            aodv->iface[i].address.networkType = NETWORK_IPV6;
            aodv->iface[i].ip_version = NETWORK_IPV6;

            Ipv6GetGlobalAggrAddress(
                node,
                i,
                &aodv->iface[i].address.interfaceAddr.ipv6);

            aodv->iface[i].aodv6eligible = TRUE;
            aodv->iface[i].aodv4eligible = FALSE;
        }
        else
        if (aodvProtocolType == ROUTING_PROTOCOL_AODV
            && (NetworkIpGetInterfaceType(node, i) == NETWORK_IPV4
            || NetworkIpGetInterfaceType(node, i) == NETWORK_DUAL)
            && ip->interfaceInfo[i]->routingProtocolType ==
                                    ROUTING_PROTOCOL_AODV)
        {
            aodv->iface[i].address.networkType = NETWORK_IPV4;
            aodv->iface[i].ip_version = NETWORK_IPV4;

            aodv->iface[i].address.interfaceAddr.ipv4 =
                        NetworkIpGetInterfaceAddress(node, i);

            aodv->iface[i].aodv4eligible = TRUE;
            aodv->iface[i].aodv6eligible = FALSE;
        }
    }


    AodvInitTrace(node, nodeInput);

    AodvInitializeConfigurableParameters(
        node,
        nodeInput,
        aodv,
        aodv->iface[interfaceIndex].address);

    // Initialize aodv routing table
    for (i = 0; i < AODV_ROUTE_HASH_TABLE_SIZE; i++)
    {
        (&aodv->routeTable)->routeHashTable[i] = NULL;
    }

    (&aodv->routeTable)->routeExpireHead = NULL;
    (&aodv->routeTable)->routeExpireTail = NULL;
    (&aodv->routeTable)->routeDeleteHead = NULL;
    (&aodv->routeTable)->routeDeleteTail = NULL;

    (&aodv->routeTable)->size = 0;

    // Initialize aodv structure to store RREQ information
    (&aodv->seenTable)->front = NULL;
    (&aodv->seenTable)->rear = NULL;
    (&aodv->seenTable)->lastFound = NULL;
    (&aodv->seenTable)->size = 0;

    // Initialize buffer to store packets which don't have any route
    (&aodv->msgBuffer)->head = NULL;
    (&aodv->msgBuffer)->size = 0;
    (&aodv->msgBuffer)->numByte = 0;

    // Initialize buffer to store information about the destinations
    // for which RREQ has been sent
    for (i = 0; i < AODV_SENT_HASH_TABLE_SIZE; i++)
    {
        (&aodv->sent)->sentHashTable[i] = NULL;
    }
    (&aodv->sent)->size = 0;

    // Initialize black listed neighbors
    (&aodv->blacklistTable)->head = NULL;
    (&aodv->blacklistTable)->size = 0;

    // Initialize Aodv sequence number
    aodv->seqNumber = 0;

    // Initialize Aodv Broadcast id
    aodv->floodingId = 0;

    // Initialize Last Broadcast sent
    aodv->lastBroadcastSent = (clocktype) 0;

    // Allocate chunk of memory
    AodvMemoryChunkAlloc(aodv);

    aodv->isExpireTimerSet = FALSE;
    aodv->isDeleteTimerSet = FALSE;

    if (AODV_DEBUG_INIT)
    {
        printf("Node %u\n", node->nodeId);
        printf("\tNode Traversal Time: %e Sec\n"
            "\tNet Diameter: %u\n"
            "\tMy Route Time out: %e Sec\n"
            "\tAllowed Hello Loss: %d\n"
            "\tActive Route Timeout: %e Sec\n"
            "\tRREQ retries: %u\n"
            "\tHello interval: %e\n\n",
            (double) aodv->nodeTraversalTime / SECOND,
             aodv->netDiameter,
            (double) aodv->myRouteTimeout / SECOND,
            aodv->allowedHelloLoss,
            (double) aodv->activeRouteTimeout / SECOND,
            aodv->rreqRetries,
            (double) aodv->helloInterval / SECOND);
    }

    if (aodv->iface[interfaceIndex].ip_version == NETWORK_IPV4)
    {
        // Set the mac status handler function
        NetworkIpSetMacLayerStatusEventHandlerFunction(
            node,
            &Aodv4MacLayerStatusHandler,
            interfaceIndex);

        // Set the router function
        NetworkIpSetRouterFunction(
            node,
            &Aodv4RouterFunction,
            interfaceIndex);

        destAddr.networkType = NETWORK_IPV4;
        destAddr.interfaceAddr.ipv4 = ANY_DEST;
        protocolType = ROUTING_PROTOCOL_AODV;

        // Set default Interface Info
        aodv->defaultInterface = interfaceIndex;

        SetIPv4AddressInfo(
            &aodv->defaultInterfaceAddr,
            NetworkIpGetInterfaceAddress(node, interfaceIndex));
    }
    else if (aodv->iface[interfaceIndex].ip_version == NETWORK_IPV6)
    {
        // Set the mac status handler function for IPv6
        Ipv6SetMacLayerStatusEventHandlerFunction(
            node,
            &Aodv6MacLayerStatusHandler,
            interfaceIndex);
        // Set the router function for IPv6
        Ipv6SetRouterFunction(
            node,
            &Aodv6RouterFunction,
            interfaceIndex);

        memcpy(&destAddr, &aodv->broadcastAddr, sizeof(Address));
        protocolType = ROUTING_PROTOCOL_AODV6;

        // Set default interface Info
        aodv->defaultInterface = interfaceIndex;

        aodv->defaultInterfaceAddr.networkType = NETWORK_IPV6;

        Ipv6GetGlobalAggrAddress(
                node,
                interfaceIndex,
                &aodv->defaultInterfaceAddr.interfaceAddr.ipv6);
    }

    if (aodv->processHello)
    {
        if (AODV_DEBUG)
        {
            printf("Node %u is setting timer "
                "MSG_NETWORK_SendHello\n", node->nodeId);
        }

        AodvSetTimer(
            node,
            MSG_NETWORK_SendHello,
            destAddr,
            AODV_HELLO_INTERVAL);
    }

    std::string path;
    D_Hierarchy *h = &node->partitionData->dynamicHierarchy;

    if (h->CreateRoutingPath(
            node,
            interfaceIndex,
            "aodv",
            "print",
            path))
    {
        h->AddObject(path, new D_AodvPrint(aodv));
    }

    if (h->CreateRoutingPath(
            node,
            interfaceIndex,
            "aodv",
            "numRequestInitiated",
            path))
    {
        h->AddObject(
            path,
            new D_UInt32Obj(&aodv->stats.numRequestInitiated));
    }

    // registering RoutingAodvHandleAddressChangeEvent function
    NetworkIpAddAddressChangedHandlerFunction(node,
                            &RoutingAodvHandleChangeAddressEvent);
    // registering RoutingAodvHandleChangeAddressEvent function
    Ipv6AddPrefixChangedHandlerFunction(node,
                        &RoutingAodvIPv6HandleChangeAddressEvent);
}


// /**
// FUNCTION : AodvFinalize
// LAYER    : NETWORK
// PURPOSE  :  Called at the end of the simulation to collect the results
// PARAMETERS:
//    +node: Node *:Pointer to Node
//    +i : int: The node for which the statistics are to be printed
// RETURN:    None
// **/

void
AodvFinalize(Node* node, int i, NetworkType networkType)
{
    AodvData* aodv = NULL;
    char buf[MAX_STRING_LENGTH];
    char aodvVerBuf[MAX_STRING_LENGTH];

    if (networkType == NETWORK_IPV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_AODV6,
                                NETWORK_IPV6);

        sprintf(aodvVerBuf, "AODV for IPv6");
    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_AODV,
                                NETWORK_IPV4);

        sprintf(aodvVerBuf, "AODV for IPv4");
    }

    //AodvPrintRoutingTable(node, &aodv->routeTable);

    if (aodv->statsCollected && !aodv->statsPrinted)
    {
        aodv->statsPrinted = TRUE;



        sprintf(buf, "Number of RREQ Packets Initiated = %u",
            (unsigned short) aodv->stats.numRequestInitiated);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Packets Retried = %u",
            aodv->stats.numRequestResent);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Packets Forwarded = %u",
            aodv->stats.numRequestRelayed);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Packets Initiated for local repair = %u",
            aodv->stats.numRequestForLocalRepair);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Packets sent for alternate route = %u",
            aodv->stats.numRequestForAlternateRt);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Packets received = %u",
            aodv->stats.numRequestRecved);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Duplicate RREQ Packets received = %u",
            aodv->stats.numRequestDuplicate);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Packets discarded for blacklist = %u",
            aodv->stats.numSenderInBlacklist);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Packets dropped due to ttl expiry = %u",
            aodv->stats.numRequestTtlExpired);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Packets received by Dest = %u",
            aodv->stats.numRequestRecvedAsDest);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREP Packets Initiated as Dest = %u",
            aodv->stats.numReplyInitiatedAsDest);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREP Packets Initiated as intermediate node = %u",
            aodv->stats.numReplyInitiatedAsIntermediate);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREP Packets Forwarded = %u",
            aodv->stats.numReplyForwarded);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Gratuitous RREP Packets sent = %u",
            aodv->stats.numGratReplySent);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREP Packets Received = %u",
            aodv->stats.numReplyRecved);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREP Packets Received for local repair = %u",
            aodv->stats.numReplyRecvedForLocalRepair);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREP Packets Received as Source = %u",
            aodv->stats.numReplyRecvedAsSource);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Hello messages sent = %u",
            aodv->stats.numHelloSent);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Hello messages received = %u",
            aodv->stats.numHelloRecved);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Packets Initiated = %u",
            aodv->stats.numRerrInitiated);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Packets Initiated with N flag = %u",
            aodv->stats.numRerrInitiatedWithNFlag);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Packets forwarded = %u",
            aodv->stats.numRerrForwarded);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Packets forwarded with N flag = %u",
            aodv->stats.numRerrForwardedWithNFlag);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Packets received = %u",
            aodv->stats.numRerrRecved);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Packets received with N flag = %u",
            aodv->stats.numRerrRecvedWithNFlag);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Packets discarded = %u",
            aodv->stats.numRerrDiscarded);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Data packets sent as Source = %u",
            aodv->stats.numDataInitiated);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Data Packets Forwarded = %u",
            aodv->stats.numDataForwarded);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Data Packets Received = %u",
            aodv->stats.numDataRecved);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Data Packets Dropped for no route = %u",
            aodv->stats.numDataDroppedForNoRoute);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf,
            "Number of Data Packets Dropped for buffer overflow = %u",
            aodv->stats.numDataDroppedForOverlimit);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf,
            "Number of Packets Dropped for exceeding Maximum Hop Count = %u",
            aodv->stats.numMaxHopExceed);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Total Hop Counts for all routes = %u",
            aodv->stats.numHops);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Routes Selected = %u",
            aodv->stats.numRoutes);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of times link broke = %u",
            aodv->stats.numBrokenLinks);

        IO_PrintStat(
            node,
            "Network",
            aodvVerBuf,
            ANY_DEST,
            -1,
            buf);
/*
        if (AODV_DEBUG)
        {

            sprintf(buf, "Max number of seen RREQ's cached at once = %u",
                    aodv->stats.numMaxSeenTable);

            IO_PrintStat(
                node,
                "Network",
                aodvVerBuf,
                ANY_DEST,
                -1,
                buf);


            sprintf(buf, "Max number of Last Found Hits = %u",
                    aodv->stats.numLastFoundHits);

            IO_PrintStat(
                node,
                "Network",
                aodvVerBuf,
                ANY_DEST,
                -1,
                buf);
        }
*/
        if (AODV_DEBUG_ROUTE_TABLE)
        {
            printf("Routing table at the end of simulation\n");

            AodvPrintRoutingTable(node, &aodv->routeTable);
        }
    }
}


// /**
// FUNCTION : AodvHandleProtocolPacket
// LAYER    : NETWORK
// PURPOSE  : Called when Aodv packet is received from MAC, the packets
//            may be of following types, Route Request, Route Reply,
//            Route Error, Route Acknowledgement
// PARAMETERS:
//    +node: Node*: The node received message
//    +msg: Message*:The message received
//    +srcAddr: Address:Source Address of the message
//    +destAddr: Address: Destination Address of the message
//    +ttl: int: Time to leave
//    +interfaceIndex: int :Receiving interface
// RETURN   : None
// **/

void
AodvHandleProtocolPacket(
    Node* node,
    Message* msg,
    Address srcAddr,
    Address destAddr,
    int ttl,
    int interfaceIndex)
{
    UInt32* packetType = (UInt32* )MESSAGE_ReturnPacket(msg);
    BOOL IPV6 = FALSE;

    if (srcAddr.networkType == NETWORK_IPV6)
    {
        IPV6 = TRUE;
    }

      //trace recd pkt
      ActionData acnData;
      acnData.actionType = RECV;
      acnData.actionComment = NO_COMMENT;
      TRACE_PrintTrace(node, msg, TRACE_NETWORK_LAYER,
          PACKET_IN, &acnData , srcAddr.networkType);

    if (AODV_DEBUG_AODV_TRACE)
    {
        AodvPrintTrace(node, msg, 'R',IPV6);
    }

    switch (*packetType >> 24)
    {
        case AODV_RREQ:
        {
            if (AODV_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                char address[MAX_STRING_LENGTH];

                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("Node %u got RREQ at time %s\n", node->nodeId,
                    clockStr);

                IO_ConvertIpAddressToString(
                    &srcAddr,
                    address);

                printf("\tfrom: %s\n", address);

                IO_ConvertIpAddressToString(
                    &destAddr,
                    address);

                printf("\tdestination: %s\n", address);
            }

            AodvHandleRequest(
                node,
                msg,
                srcAddr,
                ttl,
                interfaceIndex);

            MESSAGE_Free(node, msg);
            break;
        }

        case AODV_RREP:
        {
            if (AODV_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                char address[MAX_STRING_LENGTH];

                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                printf("Node %u got RREP at time %s\n", node->nodeId,
                    clockStr);

                IO_ConvertIpAddressToString(&srcAddr, address);

                printf("\tfrom: %s\n", address);

                IO_ConvertIpAddressToString(&destAddr, address);

                printf("\tdestination: %s\n", address);
            }


            AodvHandleReply(
                node,
                msg,
                srcAddr,
                interfaceIndex,
                destAddr);

            MESSAGE_Free(node, msg);

            break;
        }

        case AODV_RERR:
        {
            if (AODV_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                char address[MAX_STRING_LENGTH];

                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("Node %u got RERR at time %s\n", node->nodeId,
                    clockStr);

                IO_ConvertIpAddressToString(&srcAddr,address);
                printf("\tfrom: %s\n", address);
                IO_ConvertIpAddressToString(&destAddr,address);
                printf("\tdestination: %s\n", address);
            }

            AodvHandleRouteError(
                node,
                msg,
                srcAddr,
                interfaceIndex);

            MESSAGE_Free(node, msg);
            break;
        }

        default:
        {
           ERROR_Assert(FALSE, "Unknown packet type for Aodv");
           break;
        }
    }
}

// /**
// FUNCTION   :: AodvHandleProtocolEvent
// LAYER      :: NETWORK
// PURPOSE    :: Handles all the protocol events.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +msg:  Message* : Pointer to message.
// RETURN     :: void : NULL.
// **/
void
AodvHandleProtocolEvent(
    Node* node,
    Message* msg)
{
    AodvData* aodv = NULL;

    if (MESSAGE_GetProtocol(msg) == ROUTING_PROTOCOL_AODV6)
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_AODV6,
                                NETWORK_IPV6);
    }
    else
    {
        aodv = (AodvData *) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_AODV,
                                NETWORK_IPV4);
    }

    switch (MESSAGE_GetEvent(msg))
    {
        // Remove an entry from the RREQ Seen Table
        case MSG_NETWORK_FlushTables:
        {
            if (AODV_DEBUG)
            {
                char address[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(
                    &aodv->seenTable.front->srcAddr,
                    address);

                printf("Node %u is deleting from seen table(%d), "
                       "Source Address: %s, Flood ID: %d \n",
                       node->nodeId,
                       aodv->seenTable.size,
                       address,
                       aodv->seenTable.front->floodingId);
            }

            AodvDeleteSeenTable(&aodv->seenTable);

            MESSAGE_Free(node, msg);

            break;
        }
        // Check connectivity based on hello msg
        case MSG_NETWORK_CheckNeighborTimeout:
        {
            AodvRouteEntry* current = NULL;
            Address* pNeighborAddr;
            UInt32* pHelloSeqNum = 0;
            BOOL isValid = FALSE;

            if (aodv->processHello == FALSE)
            {
                MESSAGE_Free(node, msg);

                break;
            }

            pNeighborAddr = (Address* )MESSAGE_ReturnInfo(msg);

            pHelloSeqNum = (UInt32*)(pNeighborAddr + 1);

            current = AodvCheckRouteExist(
                        *pNeighborAddr,
                        &aodv->routeTable,
                        &isValid);

            if (current)
            {
                if (*pHelloSeqNum == current->helloSeqNum)
                {
                    // This is a neighbor to which the route doesn't exist
                    aodv->stats.numBrokenLinks++;

                    AodvSendRouteErrorForLinkFailure(
                        node,
                        aodv,
                        *pNeighborAddr);
                }
            }

            MESSAGE_Free(node, msg);

            break;
        }
        // Remove the route that has not been used for awhile
        case MSG_NETWORK_CheckRouteTimeout:
        {
            AodvRoutingTable* routeTable = &aodv->routeTable;
            AodvRouteEntry* current = routeTable->routeExpireHead;
            AodvRouteEntry* rtPtr = current;

            while (current && current->lifetime <= node->getNodeTime())
            {
                rtPtr = current;
                current = current->expireNext;

            /*    if (Address_IsSameAddress(
                        &rtPtr->nextHop,
                        &rtPtr->destination))
                {
                    // This is a neighbor to which the route doesn't exist
                    aodv->stats.numBrokenLinks++;

                    AodvSendRouteErrorForLinkFailure(
                        node,
                        aodv,
                        rtPtr->nextHop);

                    current = routeTable->routeExpireHead;

                }
                else*/
                {
                    // Disable the route and then delete it after delete
                    // period
                    AodvDisableRoute(
                        node,
                        rtPtr,
                        routeTable,
                        TRUE);
                }

                // flag in the route table MUST be reset when the route
                // times out (i.e., after the route has been not been active
                // for ACTIVE_ROUTE_TIMEOUT). Sec: 8.12
                rtPtr->locallyRepairable = FALSE;
            }

            if (current == NULL)
            {
                aodv->isExpireTimerSet = FALSE;
                MESSAGE_Free(node, msg);
            }
            else
            {
                MESSAGE_Send(node,
                             msg,
                             (current->lifetime - node->getNodeTime()));
            }

            break;
        }

        case MSG_NETWORK_DeleteRoute:
        {
            AodvRoutingTable* routeTable = &aodv->routeTable;
            AodvRouteEntry* current = routeTable->routeDeleteHead;
            AodvRouteEntry* rtPtr = current;

            while (current && current->lifetime <= node->getNodeTime())
            {
                rtPtr = current;
                current = current->deleteNext;

                AodvDeleteRouteTable(
                    node,
                    rtPtr,
                    routeTable);
            }

            if (current == NULL)
            {
                aodv->isDeleteTimerSet = FALSE;
                MESSAGE_Free(node, msg);
            }
            else
            {
                MESSAGE_Send(node,
                             msg,
                             (current->lifetime - node->getNodeTime()));
            }

            break;
        }

        // Check if RREP is received after sending RREQ
        case MSG_NETWORK_CheckReplied:
        {
            Address* destAddr = (Address *) MESSAGE_ReturnInfo(msg);

            if (AodvCheckSent(*destAddr, &aodv->sent))
            {
                // Route has not been obtained
                BOOL isValidRt = FALSE;
                AodvRouteEntry* rtEntry = NULL;

                rtEntry = AodvCheckRouteExist(
                              *destAddr,
                              &aodv->routeTable,
                              &isValidRt);

                if (rtEntry && rtEntry->locallyRepairable &&
                    !(isValidRt))
                {
                    rtEntry->locallyRepairable = FALSE;
                    // The route has not been obtained after trying to local
                    // repair so need to disable the entry and send route
                    // error message

                    Message* messageToDelete = NULL;
                    Address previousHop;
                    AodvDeleteSent(*destAddr, &aodv->sent);

                    if (AODV_DEBUG_LOCAL_REPAIR)
                    {
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                        printf("Local repair failed within time at %s\n",
                            clockStr);
                    }

                    // Remove all the messages destined to the destination

                    messageToDelete = AodvGetBufferedPacket(
                                        node,
                                        *destAddr,
                                        &previousHop,
                                        &aodv->msgBuffer);

                    while (messageToDelete)
                    {
                        aodv->stats.numDataDroppedForNoRoute++;

                        MESSAGE_Free(node, messageToDelete);

                        messageToDelete = AodvGetBufferedPacket(
                                            node,
                                            *destAddr,
                                            &previousHop,
                                            &aodv->msgBuffer);
                    }

                    aodv->stats.numRerrInitiated++;

                    AodvSendRouteErrorForUnreachableDest(
                        node,
                        aodv,
                        *destAddr,
                        previousHop,
                        TRUE);
                }
                else
                {
                    if (AodvGetTimes(*destAddr, &aodv->sent)
                        <= AODV_RREQ_RETRIES)
                    {
                        // If the RREP is not received within
                        // NET_TRAVERSAL_TIME milliseconds, the node MAY try
                        // again to flood the RREQ, up to a maximum of
                        // RREQ_RETRIES times.  Each new attempt MUST
                        // increment the Flooding ID field. Sec: 8.3

                        AodvRetryRREQ(node, *destAddr);
                    }
                    else
                    {
                        // If a RREQ has been flooded RREQ_RETRIES times
                        // without receiving any RREP, all data packets
                        // destined for the corresponding destination
                        // SHOULD be dropped from the buffer and a
                        // Destination Unreachable message delivered to the
                        // application. Sec: 8.3

                        // Note: The second part not done
                        Message* messageToDelete = NULL;
                        Address previousHop;

                        // Remove all the messages destined to the
                        // destination

                        messageToDelete = AodvGetBufferedPacket(
                                            node,
                                            *destAddr,
                                            &previousHop,
                                            &aodv->msgBuffer);

                        while (messageToDelete)
                        {
                            aodv->stats.numDataDroppedForNoRoute++;

                            MESSAGE_Free(node, messageToDelete);

                            messageToDelete = AodvGetBufferedPacket(
                                                node,
                                                *destAddr,
                                                &previousHop,
                                                &aodv->msgBuffer);
                        }

                        // Remove from sent table.
                        AodvDeleteSent(*destAddr, &aodv->sent);
                    }
                }
            }

            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_NETWORK_BlacklistTimeout:
        {
            Address* destAddr = (Address *) MESSAGE_ReturnInfo(msg);
            AodvDeleteFromBlacklistTable(*destAddr,
                &aodv->blacklistTable);
            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_NETWORK_SendHello:
        {
            Address* destAddr;
            clocktype delay = (clocktype) AODV_PC_ERAND(aodv->aodvJitterSeed);

            if (aodv->lastBroadcastSent < (node->getNodeTime() -
                AODV_HELLO_INTERVAL))
            {
                destAddr = (Address* ) MESSAGE_ReturnInfo(msg);

                AodvBroadcastHelloMessage(node, aodv, destAddr);

                aodv->lastBroadcastSent = node->getNodeTime();
            }

            MESSAGE_Send(node, msg, (clocktype) AODV_HELLO_INTERVAL + delay);

            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Aodv: Unknown MSG type!\n");
            break;
        }
    }
}

// /**
// FUNCTION: Aodv4RouterFunction
// LAYER   : NETWROK
// PURPOSE : Determine the routing action to take for a the given data packet
//          set the PacketWasRouted variable to TRUE if no further handling
//          of this packet by IP is necessary
// PARAMETERS:
// +node:Node *::Pointer to node
// + msg:Message*:The packet to route to the destination
// +destAddr:Address:The destination of the packet
// +previousHopAddress:Address:Last hop of this packet
// +packetWasRouted:BOOL*:set to FALSE if ip is supposed to handle the
//                        routing otherwise TRUE
// RETURN   ::void:NULL
// **/

void
Aodv4RouterFunction(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL* packetWasRouted)
{
    Address destAddress;
    Address previousHopAddr;

    destAddress.networkType=NETWORK_IPV4;
    destAddress.interfaceAddr.ipv4=destAddr;
    previousHopAddr.interfaceAddr.ipv4 = previousHopAddress;

   if (previousHopAddress)
   {
    previousHopAddr.networkType=NETWORK_IPV4;

   }
   else
   {
      //do nothing
       previousHopAddr.networkType = NETWORK_INVALID;
   }
   //end

    AodvRouterFunction(node,msg,destAddress,previousHopAddr,packetWasRouted);
}

// /**
// FUNCTION: Aodv6RouterFunction
// LAYER   : NETWROK
// PURPOSE : Determine the routing action to take for a the given data packet
//          set the PacketWasRouted variable to TRUE if no further handling
//          of this packet by IP is necessary
// PARAMETERS:
// +node:Node *::Pointer to node
// + msg:Message*:The packet to route to the destination
// +destAddr:Address:The destination of the packet
// +previousHopAddress:Address:Last hop of this packet
// +packetWasRouted:BOOL*:set to FALSE if ip is supposed to handle the
//                        routing otherwise TRUE
// RETURN   ::void:NULL
// **/

void
Aodv6RouterFunction(
    Node* node,
    Message* msg,
    in6_addr destAddr,
    in6_addr previousHopAddress,
    BOOL* packetWasRouted)
{
    Address destAddress;
    Address previousHopAddr;

    destAddress.networkType=NETWORK_IPV6;
    COPY_ADDR6(destAddr, destAddress.interfaceAddr.ipv6);
    COPY_ADDR6(previousHopAddress, previousHopAddr.interfaceAddr.ipv6);

    previousHopAddr.networkType=NETWORK_IPV6;

    AodvRouterFunction(node,msg,destAddress,previousHopAddr,packetWasRouted);
}

// /**
// FUNCTION: Aodv6MacLayerStatusHandler
// LAYER: NETWORK
// PURPOSE:  Reacts to the signal sent by the MAC protocol after link
//           failure for IPv6 and in turns call AodvMacLayerStatusHandler
// PARAMETERS:
// +node:Node*:Pointer to Node
// +msg:Message*:Pointer to message,the message not delivered
// +nextHopAddress:Address:Next Hop Address
// +incomingInterface:int:The interface in which the message was sent
// RETURN   ::void:Null
// **/

void Aodv6MacLayerStatusHandler(
                                Node* node,
                                const Message* msg,
                                const in6_addr genNextHopAddress,
                                const int incomingInterface)
{
    Address address;

    address.networkType = NETWORK_IPV6;

    if (!(IS_ANYADDR6(genNextHopAddress)))
    {

    COPY_ADDR6(genNextHopAddress,address.interfaceAddr.ipv6);
    }
    else
    {
       //do nothing
       ERROR_ReportWarning("Invalid Previous Hop Address !");
       return;
    }

    AodvMacLayerStatusHandler(node,msg,address,incomingInterface);
}


// /**
// FUNCTION: Aodv4MacLayerStatusHandler
// LAYER: NETWORK
// PURPOSE:  Reacts to the signal sent by the MAC protocol after link
//           failure for IPv4 and in turns call AodvMacLayerStatusHandler
// PARAMETERS:
// +node:Node*:Pointer to Node
// +msg:Message*:Pointer to message,the message not delivered
// +nextHopAddress:Address:Next Hop Address
// +incomingInterface:int:The interface in which the message was sent
// RETURN   ::void:Null
// **/
void Aodv4MacLayerStatusHandler(
                                Node* node,
                                const Message* msg,
                                const NodeAddress genNextHopAddress,
                                const int incomingInterface)
{
    Address address;
    address.networkType = NETWORK_IPV4;

   if (genNextHopAddress)
   {

    address.interfaceAddr.ipv4 = genNextHopAddress;
   }
   else
   {
      //do nothing
      ERROR_ReportWarning("Invalid Previous Hop Address !");
      return;
   }
   //end

    AodvMacLayerStatusHandler(node,msg,address,incomingInterface);
}
//--------------------------------------------------------------------------
// FUNCTION   :: RoutingAodvHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
//               due to DHCP feature
// PARAMETERS ::
// + node                    : Node*       : Pointer to Node structure
// + interfaceIndex          : Int32       : interface index
// + oldAddress              : Address*    : old address
// + subnetMask              : NodeAddress : subnetMask
// + NetworkType networkType : type of network protocol
// RETURN     ::             : void        : NULL
//--------------------------------------------------------------------------
void RoutingAodvHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType)
{
    AodvData* aodv = NULL;
    Address old_addr;
    Address new_addr;

    // initializing variables
    memset(&old_addr, 0, sizeof(Address));
    memset(&new_addr, 0, sizeof(Address));

    NetworkRoutingProtocolType routingProtocolType;

    NetworkDataIp* ip = node->networkData.networkVar;

    IpInterfaceInfoType* interfaceInfo =
                   ip->interfaceInfo[interfaceIndex];

    if (ip->interfaceInfo[interfaceIndex]->routingProtocolType !=
        ROUTING_PROTOCOL_AODV)
    {
        return;
    }

    if (networkType == NETWORK_IPV6)
    {
        return;
    }
    else if (networkType == NETWORK_IPV4)
    {
        old_addr.interfaceAddr.ipv4 = oldAddress->interfaceAddr.ipv4;
        old_addr.networkType = NETWORK_IPV4;

        // extracting new address
        new_addr.interfaceAddr.ipv4 = interfaceInfo->ipAddress;
        new_addr.networkType = NETWORK_IPV4;

        routingProtocolType = ROUTING_PROTOCOL_AODV;

        if (AODV_DEBUG)
        {
            char addrString1[MAX_STRING_LENGTH];
            char addrString2[MAX_STRING_LENGTH];
            char strTime[MAX_STRING_LENGTH];

            ctoa(node->getNodeTime(), strTime);
            IO_ConvertIpAddressToString(&old_addr, addrString1);
            IO_ConvertIpAddressToString(&new_addr, addrString2);

            printf("Receive notification of address change on node %d."
                   " Old Address: %s "
                   " New Address: %s "
                   " at simulation time %s\n",
                   node->nodeId,
                   addrString1,
                   addrString2,
                   strTime);
        }
    }

    // getting aodv pointer
    aodv = (AodvData*)NetworkIpGetRoutingProtocol(node,
                                                  routingProtocolType,
                                                  networkType);

    if (networkType == NETWORK_IPV4)
    {
        if (interfaceInfo->addressState == INVALID)
        {
            aodv->iface[interfaceIndex].aodv4eligible = FALSE;
        }
        else if (interfaceInfo->addressState == PREFERRED)
        {
            aodv->iface[interfaceIndex].aodv4eligible = TRUE;
        }
    }

    if (Address_IsSameAddress(&old_addr, &aodv->defaultInterfaceAddr))
    {
        // changing the node aodv default interface address
        NetworkGetInterfaceInfo(node,
                                interfaceIndex,
                                &aodv->defaultInterfaceAddr,
                                networkType);

        if (AODV_DEBUG)
        {
            char addrString1[MAX_STRING_LENGTH];
            char addrString2[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&old_addr, addrString1);
            IO_ConvertIpAddressToString(&aodv->defaultInterfaceAddr,
                                        addrString2);
            printf ("Changing node %d default aodv interface address"
                    " from %s to %s with prefix.\n",
                    node->nodeId,
                    addrString1,
                    addrString2);
        }
    }

   if (AodvIsEligibleInterface(
        node, &old_addr, &aodv->iface[interfaceIndex]))
    {
        // changing the interface address if aodv is enabled
        NetworkGetInterfaceInfo(node,
                                interfaceIndex,
                                &aodv->iface[interfaceIndex].address,
                                networkType);
        if (AODV_DEBUG)
        {
            char addrString1[MAX_STRING_LENGTH];
            char addrString2[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&old_addr, addrString1);
            IO_ConvertIpAddressToString(
                &aodv->iface[interfaceIndex].address,
                addrString2);
            printf("Changing node %d aodv other interface address"
                   " from %s to %s with prefix.\n",
                   node->nodeId,
                   addrString1,
                   addrString2);
        }
    }
}

//---------------------------------------------------------------------------
// FUNCTION   :: RoutingAodvIPv6HandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
//               due to autoconfiguration feature
// PARAMETERS ::
// + node               : Node* : Pointer to Node structure
// + interfaceIndex     : Int32 : interface index
// + oldGlobalAddress   : in6_addr* old global address
// RETURN :: void : NULL
//---------------------------------------------------------------------------
void RoutingAodvIPv6HandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    in6_addr* oldGlobalAddress)
{
    AodvData* aodv = NULL;
    Address old_addr;
    Address new_addr;

    //initializing variables
    memset(&old_addr,0,sizeof(Address));
    memset(&new_addr,0,sizeof(Address));

    IPv6Data* ipv6 = (IPv6Data *)node->networkData.networkVar->ipv6;

    IPv6InterfaceInfo* ipv6InterfaceInfo =
                  ipv6->ip->interfaceInfo[interfaceIndex]->ipv6InterfaceInfo;

    COPY_ADDR6(*oldGlobalAddress, old_addr.interfaceAddr.ipv6);
    old_addr.networkType = NETWORK_IPV6;

    // extracting new address
    COPY_ADDR6(ipv6InterfaceInfo->globalAddr, new_addr.interfaceAddr.ipv6);
    new_addr.networkType = NETWORK_IPV6;

    // getting aodv pointer
    aodv = (AodvData *) NetworkIpGetRoutingProtocol(node,
                                        ROUTING_PROTOCOL_AODV6,
                                        NETWORK_IPV6);

    if (AODV_DEBUG)
    {
        char addrString1[MAX_STRING_LENGTH];
        char addrString2[MAX_STRING_LENGTH];
        char strTime[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), strTime);
        IO_ConvertIpAddressToString(&old_addr,addrString1);
        IO_ConvertIpAddressToString(&new_addr,addrString2);

        printf ("Receive notification of address change on node %d."
                " Old Address: %s with prefix length %d"
                " New Address: %s with prefix length %d"
                " at simulation time %s\n",
                node->nodeId, addrString1,
                ipv6InterfaceInfo->autoConfigParam.depGlobalAddrPrefixLen,
                addrString2,
                ipv6InterfaceInfo->prefixLen,
                strTime);
    }

    if (Address_IsSameAddress(&old_addr, &aodv->defaultInterfaceAddr))
    {
        // changing the node aodv default interface address
        NetworkGetInterfaceInfo(node,
                                interfaceIndex,
                                &aodv->defaultInterfaceAddr,
                                NETWORK_IPV6);

        if (AODV_DEBUG)
        {
            char addrString1[MAX_STRING_LENGTH];
            char addrString2[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&old_addr,addrString1);
            IO_ConvertIpAddressToString(&aodv->defaultInterfaceAddr,addrString2);
            printf ("Changing node %d default aodv interface address"
                    " from %s to %s with prefix.\n",
                    node->nodeId,addrString1,addrString2);
        }
    }


   if (AodvIsEligibleInterface(
        node, &old_addr, &aodv->iface[interfaceIndex]))
    {
        // changing the interface address if aodv is enabled
        NetworkGetInterfaceInfo(node,
                                interfaceIndex,
                                &aodv->iface[interfaceIndex].address,
                                NETWORK_IPV6);
        if (AODV_DEBUG)
        {
            char addrString1[MAX_STRING_LENGTH];
            char addrString2[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&old_addr,addrString1);
            IO_ConvertIpAddressToString(&aodv->iface[interfaceIndex].address,
                                            addrString2);
            printf ("Changing node %d aodv other interface address"
                " from %s to %s with prefix.\n",
                node->nodeId,addrString1,addrString2);
        }
    }
}
