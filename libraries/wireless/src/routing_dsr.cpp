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

// Reference: The implementation is according to dsr draft 7
//     http://www.ietf.org/internet-drafts/draft-ietf-manet-dsr-07.txt

// Future works:
//         1. FIFO strategy for immature packet drop from
//            packet buffer for buffer overflowing.
//         2. Only option implemented is propagating and non
//            propagating broadcast of RREQ. Expanding ring
//            search can be implemented.
//         3. Optimization for preventing route reply storm
//         4. Gratuitous route reply table
//         5. Blacklist table
//         6. Network layer acknowledgement
//         7. Automatic route shortening.
//         8. Multiple interface support
//         9. Route cache has been implemented a path cache, other
//            available option is link cache
//        10. Initiating Route discovery to send Reply back to source
//            with Piggybacking the Reply option.


// Assumptions:
//         1. Physical link needs bidirectional communication
//            to send unicast packets.
//         2. Route reply will be reversed back to the source
//         3. Route request will be sent as a separate packet
//         4. This protocol is only applicable for wireless scenario
//         5. MAC layer has packet transmission acknowledgement method

// Note: #1
// According to the Draft
// "decrement the value of the Segments Left field by 1.  Let i equal n minus
// Segments Left.  This is the index of the next address to be visited in the
// Address vector."
// If a path list contains addr[1] .....addr[i]..........addr[n], and
// i = (n - segsleft) then addr[i + 1] should give the next hop not i. In C
// programming the equation is correct as index actually starts from 0 not 1
//
// According to the Draft
// "Specifically, the node copies the hop addresses of the source  route into
//  sequential Address[i] fields in the DSR Source Route option, for i = 1,
// 2, ..., n.Address[1] here is the address of the salvaging node itself (the
// first address in the source route found from this node to the IP
// Destination Address of the packet).  The value n here is the number of hop
// addresses in this source route, excluding the destination of the packet
// (which is instead already represented in the Destination Address field in
// the packet's IP header)."
// "Initialize the Segments Left field in the DSR Source Route option to n as
//  defined above"
//
// With this specification above rule for calculating next hop is not going to
// work
// If a path list contains addr[1] .....addr[i]..........addr[n], and
// i = (n - segsleft) consider the 1st hop in the path, there i will be equal
// to 1 which is the previous hop not next Hop. So the equation should be
// next hop = addr[i + 2]. We took a different approach. We have set segsleft
// to n - 1 instead of n. Then processing will be same as above.


// Note: #2
// The draft says, if the destination is one hop away, then a node don't need
// to add source route option with the packet and should send the packet
// directly to the destination, without adding a DSR header.
// To make the work of DSR symmetric while receiving a packet, in the
// implementation, even when a node adds a source route option with a packet,
// that source route only travels up to the previous hop of the destination.
// The previous hop to the destination strips out DSR source route option
// and DSR header iff the packet contains only data, and sends the only data
// portion to the destination.
// This might result in a Routing Loop in some special cases. If the topology is
// prone to change , then number of salvaging a packet might be higher. In
// those cases salvage information (like Salvage Count) might be lost at the
// previous hop to the destination as DSR header, at least the source route,
// will be stripped out there. So there might be no way to check
// MAX_SALVAGE_COUNT to stop a packet to be salvaged. This might cause a loop of
// salvaging the packet continiously.
//

#include<stdio.h>
#include<stdlib.h>
#include<limits.h>

#include "api.h"
#include "network_ip.h"
#include "routing_dsr.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#define DEBUG_TRACE               0

#define DEBUG_ROUTE_CACHE         0

#define DEBUG_ROUTING_TABLE       0

#define DEBUG_ERROR               0

#define DEBUG_MAINTENANCE_BUFFER  0

#define DEBUG_SEND_BUFFER         0

#define DEBUG_RREQ_BUFFER         0

#define DEBUG_DISCOVERY           0


//------------------------------------------
// Dsr Memory Manager
//------------------------------------------

//-------------------------------------------------------------------------
// FUNCTION: DsrMemoryChunkAlloc
// PURPOSE: Function to allocate a chunk of memory
// ARGUMENTS: Pointer to Dsr main data structure
// RETURN: void
//-------------------------------------------------------------------------

static
void DsrMemoryChunkAlloc(DsrData* dsr)
{
    int i = 0;
    DsrMemPollEntry* freeList = NULL;

    dsr->freeList = (DsrMemPollEntry *) MEM_malloc(
                        DSR_MEM_UNIT * sizeof(DsrMemPollEntry));

    ERROR_Assert(dsr->freeList != NULL, " No available Memory");

    freeList = dsr->freeList;

    for (i = 0; i < DSR_MEM_UNIT - 1; i++)
    {
        freeList[i].next = &freeList[i+1];
    }

    freeList[DSR_MEM_UNIT - 1].next = NULL;
}


//-------------------------------------------------------------------------
// FUNCTION: DsrMemoryMalloc
// PURPOSE: Function to allocate a single cell of memory from the memory
//          chunk
// ARGUMENTS: Pointer to Dsr main data structure
// RETURN: Address of free memory cell
//-------------------------------------------------------------------------

static
DsrRouteEntry* DsrMemoryMalloc(DsrData* dsr)
{
    DsrRouteEntry* temp = NULL;

    if (!dsr->freeList)
    {
        DsrMemoryChunkAlloc(dsr);
    }

    temp = (DsrRouteEntry*)dsr->freeList;
    dsr->freeList = dsr->freeList->next;
    return temp;
}


//-------------------------------------------------------------------------
// FUNCTION: DsrMemoryFree
// PURPOSE: Function to return a memory cell to the memory pool
// ARGUMENTS: Pointer to Dsr main data structure,
//            pointer to route entry
// RETURN: void
//-------------------------------------------------------------------------


static
void DsrMemoryFree(DsrData* dsr,DsrRouteEntry* ptr)
{
    DsrMemPollEntry* temp = (DsrMemPollEntry*)ptr;
    temp->next = dsr->freeList;
    dsr->freeList = temp;
}


//-------------------------------------------------------------------------
// Dsr Utility functions

//-------------------------------------------------------------------------
// FUNCTION: DsrCheckIfAddressExists
// PURPOSE: Function to check whether a particular address exists in a path
// ARGUMENTS: Pointer to path, path length and the address to check
// RETURN: TRUE if the address exists in the path, FALSE otherwise
//-------------------------------------------------------------------------

static
BOOL DsrCheckIfAddressExists(
    unsigned char* path,
    int hopCount,
    NodeAddress addrToChk)
{
    int i = 0;

    for (i = 0; i < hopCount; i++)
    {
        NodeAddress addrInPath = 0;
        memcpy(&addrInPath, path, sizeof(NodeAddress));

        if (addrInPath == addrToChk)
        {
            // Found the desired address
            return TRUE;
        }
        path += sizeof(NodeAddress);
    }

    // The address doesn't exist
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION: SizeOfPathInRouteRequest
// PURPOSE: Returns path length in Route Request option
// ARGUMENTS: Pointer to the Route Request option in a packet
// RETURN: Size in integer
//-------------------------------------------------------------------------

static
unsigned int SizeOfPathInRouteRequest(unsigned char* rreq)
{
    return *(rreq + sizeof(unsigned char))
               - DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH_TYPE_LEN;
}


//-------------------------------------------------------------------------
// FUNCTION: SizeOfPathInRouteReply
// PURPOSE: Returns path length in Route Reply option
// ARGUMENTS: Pointer to the Route Reply option in a packet
// RETURN: Size in integer
//-------------------------------------------------------------------------

static
unsigned int SizeOfPathInRouteReply(unsigned char* rrep)
{
    return *(rrep + sizeof(unsigned char)) -
        DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH_TYPE_LEN;
}


//-------------------------------------------------------------------------
// FUNCTION: SizeOfPathInSourceRoute
// PURPOSE: Returns path length in Source Route option
// ARGUMENTS: Pointer to the Source Route option in a packet
// RETURN: Size in integer
//-------------------------------------------------------------------------

static
unsigned int SizeOfPathInSourceRoute(unsigned char* srcRoute)
{
    return *(srcRoute + sizeof(unsigned char))
                - DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH_TYPE_LEN;
}



// Functions to print traces of Dsr packets

//-------------------------------------------------------------------------
// FUNCTION: DsrTraceFileInit
// PURPOSE: Initialize the file to write Dsr packet traces
// ARGUMENTS: Handler of the file
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrTraceFileInit(FILE *fp)
{
    fprintf(fp, "ROUTING_Dsr Trace\n"
        "\n"
        "Fields are space separated. The format of each line is:\n"
        "1.  Running serial number (for cross-reference)\n"
        "2.  Node ID at which trace is captured\n"
        "3.  Time in seconds (There is delay before PHY transmit)\n"
        "4.  IP Source & Destination address\n"
        "5.  A character indicating S)end R)eceive\n"
        "6.  Payload length\n"
        "7.  Type of option (Request, Reply, SrcRoute, ...)\n"
        "    --- (separator)\n"
        "    Fields as necessary (depending on option type)\n"
        "\n");
}


//-------------------------------------------------------------------------
// FUNCTION: DsrTraceWriteNodeAddrAsDotIP
// PURPOSE: Printing IP address in dotted decimal format in trace file
// ARGUMENTS: IP address to print and trance file handler
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrTraceWriteNodeAddrAsDotIP(
    NodeAddress address,
    FILE *fp)
{
    char addrStr[MAX_ADDRESS_STRING_LENGTH];

    IO_ConvertIpAddressToString(address, addrStr);

    fprintf(fp, "%s ", addrStr);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrTraceWriteCommonHeader
// PURPOSE: Printing Dsr header in trace file
// ARGUMENTS: IP address to print and trance file handler
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrTraceWriteCommonHeader(
    Node *node,
    unsigned char* dsrPkt,
    const char *flag,
    NodeAddress destAddr,
    NodeAddress srcAddr,
    FILE *fp)
{
    char clockStr[MAX_CLOCK_STRING_LENGTH];
    DsrHeader *hdr = (DsrHeader *) dsrPkt;

    fprintf(fp, "%4u ", node->nodeId);

    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
    fprintf(fp, "%s ", clockStr);

    DsrTraceWriteNodeAddrAsDotIP(srcAddr, fp);
    DsrTraceWriteNodeAddrAsDotIP(destAddr, fp);

    fprintf(fp, "%c %4u ", *flag, hdr->payloadLength);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrTraceWriteRequest
// PURPOSE: Printing Route request option
// ARGUMENTS: Pointer to route request option
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrTraceWriteRequest(
    char *rreqOpt,
    unsigned char optLen,
    FILE *fp)
{
    unsigned short id;
    NodeAddress targetAddr;
    char *dataPtr;
    unsigned char remainLen;

    memcpy(&id, rreqOpt + DSR_SIZE_OF_TYPE_LEN, sizeof(short));
    memcpy(&targetAddr, rreqOpt + sizeof(unsigned int), sizeof(NodeAddress));

    dataPtr = rreqOpt + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH;
    remainLen = (unsigned char) (optLen
        - DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH_TYPE_LEN);

    fprintf(fp, "Request ID = %u  Target = ", id);
    DsrTraceWriteNodeAddrAsDotIP(targetAddr, fp);
    fprintf(fp, "\n\t HopSequence");

    if (!remainLen)
    {
        fprintf(fp, "  (None)");
    }

    while (remainLen)
    {
        NodeAddress thisHop;

        memcpy(&thisHop, dataPtr, sizeof(NodeAddress));
        fprintf(fp, "\n\t ");
        DsrTraceWriteNodeAddrAsDotIP(thisHop, fp);

        dataPtr += sizeof(NodeAddress);
        remainLen -= sizeof(NodeAddress);
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrTraceWriteReply
// PURPOSE: Printing Route reply option
// ARGUMENTS: Pointer to route reply option
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrTraceWriteReply(
    char *rrepOpt,
    unsigned char optLen,
    FILE *fp)
{
    NodeAddress targetAddr;
    char *dataPtr;
    unsigned char remainLen;

    memcpy(&targetAddr, rrepOpt + DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH,
        sizeof(NodeAddress));

    dataPtr = rrepOpt + DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH;
    remainLen = (unsigned char) (optLen
        - DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH_TYPE_LEN);

    fprintf(fp, "Reply\n\t HopSequence ");

    while (remainLen)
    {
        NodeAddress thisHop;

        memcpy(&thisHop, dataPtr, sizeof(NodeAddress));
        fprintf(fp, "\n\t ");
        DsrTraceWriteNodeAddrAsDotIP(thisHop, fp);

        dataPtr += sizeof(NodeAddress);
        remainLen -= sizeof(NodeAddress);
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrTraceWriteRouteError
// PURPOSE: Printing error option
// ARGUMENTS: Pointer to route error option
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrTraceWriteRouteError(
    char *rerrOpt,
    FILE *fp)
{
    NodeAddress srcAddr;
    NodeAddress destAddr;
    NodeAddress unreachableDest;
    unsigned char errType;
    unsigned char salvage;

    errType = *(rerrOpt + DSR_SIZE_OF_TYPE_LEN);

    salvage = *(rerrOpt + DSR_SIZE_OF_TYPE_LEN + sizeof(char));

    memcpy(&srcAddr, rerrOpt + 4 * sizeof(char), sizeof(NodeAddress));
    memcpy(&destAddr, rerrOpt + 4 * sizeof(char) + sizeof(unsigned int),
        sizeof(NodeAddress));
    memcpy(&unreachableDest, rerrOpt + DSR_SIZE_OF_ROUTE_ERROR_WITHOUT_PATH,
        sizeof(NodeAddress));

    fprintf(fp, "Route Error Type = %s  salvage = %u  error source = ",
        "node unreachable",
        salvage);

    DsrTraceWriteNodeAddrAsDotIP(srcAddr, fp);

    fprintf(fp, "error destination = ");
    DsrTraceWriteNodeAddrAsDotIP(destAddr, fp);

    fprintf(fp, "unreachable destination = ");
    DsrTraceWriteNodeAddrAsDotIP(unreachableDest, fp);
}

//-------------------------------------------------------------------------
// FUNCTION: DsrTraceWriteSrcRoute
// PURPOSE: Printing source route option
// ARGUMENTS: Pointer to source route option
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrTraceWriteSrcRoute(
    char *srcRtOpt,
    unsigned char optLen,
    FILE *fp)
{
    unsigned short flag = 0;

    char *dataPtr;
    unsigned char remainLen;

    dataPtr = srcRtOpt + DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH;
    remainLen = (unsigned char) (optLen - sizeof(short));

    fprintf(fp, "Source Route ");
    memcpy(&flag, srcRtOpt + DSR_SIZE_OF_TYPE_LEN, sizeof(short));

    fprintf(fp, "salvage = %u  segLeft = %u  \n\t HopSequence",
        (flag & DSR_SALVAGE_MASK) >> 6,
        (flag & DSR_SEGS_LEFT_MASK));

    if (!remainLen)
    {
        fprintf(fp, "  (None)");
    }

    while (remainLen)
    {
        NodeAddress thisHop;

        memcpy(&thisHop, dataPtr, sizeof(NodeAddress));
        fprintf(fp, "\n\t ");
        DsrTraceWriteNodeAddrAsDotIP(thisHop, fp);

        dataPtr += sizeof(NodeAddress);
        remainLen -= sizeof(NodeAddress);
    }
}

//-------------------------------------------------------------------------
// FUNCTION: DsrTrace
// PURPOSE: Printing packet in a trace file
// ARGUMENTS: The received packet
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrTrace(
    Node *node,
    DsrData *dsr,
    unsigned char* dsrPkt,
    const char *flag,
    NodeAddress destAddr,
    NodeAddress srcAddr)
{
    FILE *fp;
    DsrHeader *hdr;
    unsigned char *thisOpt;
    unsigned char optLen;
    unsigned short payloadLen;

    if (dsr->trace == ROUTING_DSR_TRACE_NO)
    {
        return;
    }

    fp = fopen("dsrTrace.asc", "a");
    ERROR_Assert(fp != NULL,
        "ROUTING_DSR_Trace: file open error.\n");

    if (strcmp(flag, "Send") && strcmp(flag, "Receive"))
    {
        fprintf(fp, "Node %d %s \n", node->nodeId, flag);
        fclose(fp);
        return;
    }

    DsrTraceWriteCommonHeader(node, dsrPkt, flag, destAddr, srcAddr, fp);

    hdr = (DsrHeader *) dsrPkt;
    thisOpt = (unsigned char *) (hdr + 1);

    memcpy(&payloadLen, &hdr->payloadLength, sizeof(unsigned short));

    do
    {
        if (*thisOpt != DSR_PAD_1)
        {
            memcpy(&optLen, thisOpt + 1, sizeof(char));
        }
        else
        {
            optLen = 0;
        }

        switch (*thisOpt)
        {
            case DSR_PAD_1:
            case DSR_PAD_N:
            {
                fclose(fp);
                ERROR_Assert(FALSE,
                    "Pad 1 and Pad N option should not occur");
                break;
            }
            case DSR_ROUTE_REQUEST:
            {
                DsrTraceWriteRequest((char *)thisOpt, optLen, fp);
                break;
            }
            case DSR_ROUTE_REPLY:
            {
                DsrTraceWriteReply((char *)thisOpt, optLen, fp);
                break;
            }
            case DSR_ROUTE_ERROR:
            {
                DsrTraceWriteRouteError((char*)thisOpt, fp);
                break;
            }
            case DSR_SRC_ROUTE:
            {
                DsrTraceWriteSrcRoute((char *)thisOpt, optLen, fp);
                break;
            }
            case DSR_ACK_REQUEST:
            case DSR_ACK:
            {
                fclose(fp);
                ERROR_Assert(FALSE, "Un implemented dsr option");
                break;
            }
            default:
            {
                fclose(fp);
                ERROR_Assert(FALSE, "Unknown dsr option");
                break;
            }
        }

        payloadLen = (unsigned short) (payloadLen - (optLen + 2));
        thisOpt = thisOpt + (optLen + 2);

        fprintf(fp, "\n\t\t");
    } while (payloadLen != 0);

    fprintf(fp, "\n");

    fclose(fp);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrTraceInit
// PURPOSE: Initializing Dsr trace
// ARGUMENTS: Main input file
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrTraceInit(
    Node *node,
    const NodeInput *nodeInput,
    DsrData *dsr)
{
    char yesOrNo[MAX_STRING_LENGTH];
    BOOL retVal;

    // Initialize trace values for the node
    // <TraceType> is one of
    //      NO    the default if nothing is specified
    //      YES   an ASCII format
    // Format is: TRACE-DSR YES | NO

    IO_ReadString(
        node->nodeId,
        (unsigned)ANY_INTERFACE,
        nodeInput,
        "TRACE-DSR",
        &retVal,
        yesOrNo);

    if (retVal == TRUE)
    {
        if (!strcmp(yesOrNo, "NO"))
        {
            dsr->trace = ROUTING_DSR_TRACE_NO;
        }
        else if (!strcmp(yesOrNo, "YES"))
        {
            FILE* fp = NULL;
            dsr->trace = ROUTING_DSR_TRACE_YES;
            fp = fopen("dsrTrace.asc", "w");

            ERROR_Assert(fp != NULL,
                "DSR Trace: file initial open error.\n");

            DsrTraceFileInit(fp);
            fclose(fp);
        }
        else
        {
            ERROR_Assert(FALSE,
                "DsrTraceInit: "
                "Unknown value of TRACE-DSR in configuration file.\n"
                "Expecting YES or NO\n");
        }
    }
    else
    {
        dsr->trace = ROUTING_DSR_TRACE_NO;
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrSetTimer
// PURPOSE: Setting a timer
// ARGUMENTS: event type of the timer, delay for the timer to expire, any
//            other information to be included with the timer
// RETURN: None
//-------------------------------------------------------------------------

static
void DsrSetTimer(
    Node* node,
    void* data,
    unsigned int size,
    int eventType,
    clocktype delay)
{
    Message* newMsg = NULL;

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_DSR,
                eventType);

    if (data != NULL && size != 0)
    {
        MESSAGE_InfoAlloc(node, newMsg, size);
        memcpy(MESSAGE_ReturnInfo(newMsg), data, size);
    }

    MESSAGE_Send(node, newMsg, delay);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrMod
// PURPOSE: Computing mod value of an integer
// ARGUMENTS: an integer
// RETURN: None
//-------------------------------------------------------------------------

static
int DsrMod(int x)
{
    if (x < 0)
    {
        return -x;
    }
    else
    {
        return x;
    }
}

// Functions related to adding different dsr option fields

//-------------------------------------------------------------------------
// FUNCTION: DsrAddCommonHeader
// PURPOSE:  Adding Dsr common header with a packet
// ARGUMENTS: pointer to the position of the packet where dsr header
//            should be added, next header value, total option length
// RETURN:   None
// ASSUMPTION: None
//-------------------------------------------------------------------------

static
void DsrAddCommonHeader(
    unsigned char* pktPtr,
    unsigned char nextHdr,
    unsigned short payloadLen)
{
    // next header field in dsr header
    *pktPtr = nextHdr;
    pktPtr++;

    // assign reserved field
    *pktPtr = 0;
    pktPtr++;

    // assign payload length
    memcpy(pktPtr, &payloadLen, sizeof(unsigned short));
    pktPtr += sizeof(unsigned short);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrAddSrcRouteOption
// PURPOSE:  Adding Dsr source route option with a packet
// ARGUMENTS: pointer to the position of the packet where the source route
//            option should be added, source route, hop count in the source
//            route, F, L, salvage value, segleft value
// RETURN:   None
// ASSUMPTION: None
//-------------------------------------------------------------------------

static
void DsrAddSrcRouteOption(
    DsrData* dsr,
    unsigned char* pktPtr,
    unsigned char* srcRt,
    unsigned int hopCountInSrcRoute,
    unsigned int F,
    unsigned int L,
    unsigned int salvage,
    unsigned int segsLeft)
{
    // assign value for total flag by bit shifting individual elements
    unsigned short fLReservedSalvageSegleft = (unsigned short) (segsLeft
                                                    + (salvage << 6)
                                                    + (0 << 10)
                                                    + (L << 14)
                                                    + (F << 15));
    // Set option type
    *pktPtr = DSR_SRC_ROUTE;
    pktPtr++;

    // Set option data length
    if (salvage)
    {
        *pktPtr = (unsigned char) ((hopCountInSrcRoute + 1)
            * sizeof(NodeAddress) + sizeof(short));
    }
    else
    {
        *pktPtr = (unsigned char) (hopCountInSrcRoute
            * sizeof(NodeAddress) + sizeof(short));
    }
    pktPtr++;

    // Set next two bytes, F, L, reserved and salvage and seg left
    memcpy(pktPtr, &fLReservedSalvageSegleft, sizeof(unsigned short));
    pktPtr += sizeof(unsigned short);

    // Set path list
    if (salvage)
    {
        memcpy(pktPtr, &dsr->localAddress, sizeof(NodeAddress));
        pktPtr += sizeof(NodeAddress);
    }

    memcpy(pktPtr, srcRt, hopCountInSrcRoute * sizeof(NodeAddress));
}


//-------------------------------------------------------------------------
// FUNCTION: DsrAddRouteRequestOption
// PURPOSE:  Adding route request option with a dsr packet
// ARGUMENTS: pointer to that position of the packet where the route request
//            option should be added, the record route to be added, hop count
//            in the record route, identification, target address
// RETURN:   None
// ASSUMPTION: None
//-------------------------------------------------------------------------

static
void DsrAddRouteRequestOption(
    unsigned char* pktPtr,
    unsigned char* recordRt,
    unsigned int hopCountInRecordRoute,
    unsigned short identification,
    NodeAddress targetAddress)
{
    // Set option type
    *pktPtr = DSR_ROUTE_REQUEST;
    pktPtr++;

    // Set option data length
    *pktPtr = (unsigned char) (hopCountInRecordRoute * sizeof(NodeAddress)
                  + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH_TYPE_LEN);
    pktPtr++;

    // Set identification value
    memcpy(pktPtr, &identification, sizeof(unsigned short));
    pktPtr += sizeof(unsigned short);

    // Set Target address
    memcpy(pktPtr, &targetAddress, sizeof(NodeAddress));
    pktPtr += sizeof(NodeAddress);

    // Set record route lists
    if (hopCountInRecordRoute)
    {
        memcpy(pktPtr, recordRt, hopCountInRecordRoute
            * sizeof(NodeAddress));
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrAddRouteReplyOption
// PURPOSE:  Adding route reply option with a dsr packet
// ARGUMENTS: pointer to that position of the packet where the route reply
//            option to be added, reply path list, hop count, L
// RETURN:   None
// ASSUMPTION: None
//-------------------------------------------------------------------------

static
void DsrAddRouteReplyOption(
    unsigned char* pktPtr,
    unsigned char* pathList,
    unsigned int hopCount,
    unsigned int L)
{
    unsigned char lReserved = (unsigned char) (L << 7);
    // Set option type
    *pktPtr = DSR_ROUTE_REPLY;
    pktPtr++;

    // Set option data length
    *pktPtr = (unsigned char) (hopCount * sizeof(NodeAddress)
                  + DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH_TYPE_LEN);
    pktPtr++;

    // Set next byte, L and reserved
    memcpy(pktPtr, &lReserved, sizeof(unsigned char));
    pktPtr++;

    // Set path list
    memcpy(pktPtr, pathList, hopCount * sizeof(NodeAddress));
}


//-------------------------------------------------------------------------
// FUNCTION: DsrAddRouteErrorOption
// PURPOSE:  Adding route error option with a dsr packet
// ARGUMENTS: pointer to that position of the packet where the route reply
//            option to be added, reply path list, hop count, L
// RETURN:   None
// ASSUMPTION: None
//-------------------------------------------------------------------------

static
void DsrAddRouteErrorOption(
    unsigned char* pktPtr,
    unsigned char salvage,
    NodeAddress errSrcAddr,
    NodeAddress errDestAddr,
    NodeAddress unreachableDest)
{
    // Set option type
    *pktPtr = DSR_ROUTE_ERROR;
    pktPtr++;

    // Set option data length
    *pktPtr = DSR_SIZE_OF_ROUTE_ERROR_WITHOUT_TYPE_LEN;
    pktPtr++;

    // Set error type
    *pktPtr = DSR_ERROR_NODE_UNREACHABLE;
    pktPtr++;

    // Set reserved and salvage field
    *pktPtr = salvage;
    pktPtr++;

    // Set error source address
    memcpy(pktPtr, &errSrcAddr, sizeof(NodeAddress));
    pktPtr += sizeof(NodeAddress);

    // Set error dest address
    memcpy(pktPtr, &errDestAddr, sizeof(NodeAddress));
    pktPtr += sizeof(NodeAddress);

    // Set unreachable destination
    memcpy(pktPtr, &unreachableDest, sizeof(NodeAddress));
}


// Functions related to request table

//-------------------------------------------------------------------------
// FUNCTION: DsrInitRequestTable
// PURPOSE:  Initializing Request table which will store originated and
//           forwarded Route request information
// ARGUMENTS: Pointer to the request table
// RETURN:   None
// ASSUMPTION: None
//-------------------------------------------------------------------------

static
void DsrInitRequestTable(DsrRequestTable* reqTable)
{
    int i = 0;
    for (i = 0; i < DSR_REPLY_HASH_TABLE_SIZE; i++)
    {
        reqTable->seenHashTable[i] = NULL;
        reqTable->sentHashTable[i] = NULL;
    }

    reqTable->LRUListHead = NULL;
    reqTable->LRUListTail = NULL;
    reqTable->count = 0;
}


//-------------------------------------------------------------------------
// FUNCTION: DsrGetLRUEntry
// PURPOSE:  Finding Least Recently Used entry in the Route request table
// ARGUMENTS: Pointer to the request table
// RETURN:   None
// ASSUMPTION: None
//-------------------------------------------------------------------------

static
DsrRequestEntry* DsrGetLRUEntry(DsrRequestTable* reqTable)
{
    return reqTable->LRUListHead;
}


//-------------------------------------------------------------------------
// FUNCTION: DsrSearchRequestEntry
// PURPOSE:  Searching for an entry in the request table
// ARGUMENTS: Pointer to the request table
//            ip address of the entry
//            entryType, whether the entry is of seen request or of
//                       originated or forwarded request
// RETURN:   None
//-------------------------------------------------------------------------
static
DsrRequestEntry* DsrSearchRequestEntry(
    DsrRequestTable* reqTable,
    NodeAddress addr,
    DsrReqEntryType type)
{
    DsrRequestEntry* tableEntry = NULL;
    if (type == ROUTING_DSR_SEEN_ENTRY)
    {
        tableEntry = reqTable->seenHashTable[addr
                         % DSR_REPLY_HASH_TABLE_SIZE];
    }
    else if (type == ROUTING_DSR_SENT_ENTRY)
    {
        tableEntry = reqTable->sentHashTable[addr
                         % DSR_REPLY_HASH_TABLE_SIZE];
    }
    while (tableEntry)
    {
        if (tableEntry->addr == addr)
        {
            return tableEntry;
        }
        tableEntry = tableEntry->next;
    }
    return NULL;
}

//-------------------------------------------------------------------------
// FUNCTION: DsrRemoveFromRequestTable
// PURPOSE:  Deleting an entry from the route request table
// ARGUMENTS: Pointer to the request table
//            entryType, whether the entry is of seen request or of
//                       originated or forwarded request
//            ip address of the entry to be deleted
// RETURN:   None
// ASSUMPTION: None
//-------------------------------------------------------------------------

static
void DsrRemoveFromRequestTable(
    DsrRequestTable* reqTable,
    DsrRequestEntry* deletingEntry)
{

    DsrReqEntryType type = deletingEntry->type;
    int queueNo = deletingEntry->addr % DSR_REPLY_HASH_TABLE_SIZE;
    DsrRequestEntry** ptr = NULL;

    if (type == ROUTING_DSR_SENT_ENTRY)
    {
        ptr = (DsrRequestEntry**)reqTable->sentHashTable;
    }
    else if (type == ROUTING_DSR_SEEN_ENTRY)
    {
        ptr = (DsrRequestEntry**)reqTable->seenHashTable;
    }
    else
    {
        ERROR_Assert(FALSE,"Type mismatch");
    }

    if (deletingEntry == reqTable->LRUListHead)
    {
        reqTable->LRUListHead = deletingEntry->LRUNext;
        if (reqTable->LRUListHead)
        {
            reqTable->LRUListHead->LRUPrev = NULL;
        }
        else
        {
            reqTable->LRUListTail = NULL;
        }
    }
    else if (deletingEntry == reqTable->LRUListTail)
    {
        reqTable->LRUListTail = deletingEntry->LRUPrev;
        reqTable->LRUListTail->LRUNext = NULL;
    }
    else
    {
        deletingEntry->LRUNext = deletingEntry->LRUPrev->LRUNext;
        deletingEntry->LRUNext->LRUPrev = deletingEntry->LRUPrev;
    }


    if (deletingEntry == ptr[queueNo])
    {
        ptr[queueNo]= deletingEntry->next;
        if (deletingEntry->next)
        {
            deletingEntry->next->prev = NULL;
        }
    }
    else
    {
        deletingEntry->prev->next = deletingEntry->next;
        if (deletingEntry->next)
        {
            deletingEntry->next->prev = deletingEntry->prev;
        }
    }

    MEM_free(deletingEntry->data);
    MEM_free(deletingEntry);

    reqTable->count--;
}


//-------------------------------------------------------------------------
// FUNCTION: DsrAddToRequestTable
// PURPOSE:  Adding an entry in the request table
// ARGUMENTS: Pointer to the request table
//            entryType, whether the entry is of seen request or of
//                       originated or forwarded request
//            addr, ip address of the entry
//            data, entryType specific information
//            timeStamp, current time
// RETURN:   None
// ASSUMPTION: The entries are sorted in ascending order of their Ip address
//-------------------------------------------------------------------------

static
void DsrAddToRequestTable(
    DsrRequestTable* reqTable,
    DsrReqEntryType type,
    NodeAddress addr,
    void* data,
    clocktype timeStamp)
{
    DsrRequestEntry* tableEntry = NULL;
    DsrRequestEntry** ptr = NULL;
    DsrRequestEntry* prevEntry = NULL;
    DsrRequestEntry* newEntry = NULL;
    int queueNo = addr % DSR_REPLY_HASH_TABLE_SIZE;

    if (type == ROUTING_DSR_SENT_ENTRY)
    {
        ptr = (DsrRequestEntry**)reqTable->sentHashTable;
    }
    else if (type == ROUTING_DSR_SEEN_ENTRY)
    {
        ptr = (DsrRequestEntry**)reqTable->seenHashTable;
    }
    else
    {
        ERROR_Assert(FALSE,"Type mismatch");
    }

    // If table is full delete least recently used entry
    if (reqTable->count == DSR_REQUEST_TABLE_SIZE)
    {
        DsrRequestEntry* LRUEntry = NULL;

        LRUEntry = DsrGetLRUEntry(reqTable);
        ERROR_Assert(LRUEntry, "Didn't found LRU entry in request table\n");

        DsrRemoveFromRequestTable(reqTable,LRUEntry);
    }

    // Allocate and assign different fields of the new entry
    newEntry = (DsrRequestEntry *)
        MEM_malloc(sizeof(DsrRequestEntry));

    newEntry->timeStamp = timeStamp;
    newEntry->addr = addr;
    newEntry->data = data;
    newEntry->type = type;

    newEntry->LRUNext = NULL;
    newEntry->LRUPrev = reqTable->LRUListTail;
    reqTable->LRUListTail = newEntry;

    if (reqTable->LRUListHead == NULL)
    {
        reqTable->LRUListHead = newEntry;
    }
    else
    {
        newEntry->LRUPrev->LRUNext = newEntry;
    }


    tableEntry = ptr[queueNo];
    prevEntry = tableEntry;

    while (tableEntry && addr > tableEntry->addr)
    {
        prevEntry = tableEntry;
        tableEntry = tableEntry->next;
    }

    if (prevEntry == tableEntry)
    {
        newEntry->prev = NULL;
        newEntry->next = prevEntry;
        ptr[queueNo] = newEntry;
        if (prevEntry != NULL)
        {
            prevEntry->prev = newEntry;
        }
    }
    else
    {
        newEntry->prev = prevEntry;
        newEntry->next = tableEntry;
        prevEntry->next = newEntry;
        if (tableEntry)
        {
            tableEntry->prev = newEntry;
        }

    }
    reqTable->count++;
}

//-------------------------------------------------------------------------
// Functions related to request seen table

//-------------------------------------------------------------------------
// FUNCTION: DsrCheckSeenEntryForIDAndTarget
// PURPOSE:  Searching for an Request Seen entry in seen table by
//           dest address and request Id
// ARGUMENTS: Pointer to seen entry
//            Address and sequence number
// RETURN:   None
//-------------------------------------------------------------------------

static
BOOL DsrCheckSeenEntryForIDAndTarget(
    unsigned short seqNum,
    NodeAddress targetAddr,
    DsrSeenEntry* tableEntry)
{
    int index = tableEntry->front;
    int count = 0;

    while (count < tableEntry->size)
    {
        if (tableEntry->seqNumber[index] == seqNum
            && tableEntry->targetAddr[index] == targetAddr)
        {
            return TRUE;
        }

        count++;
        index = (index + 1) % DSR_REQUEST_TABLE_IDS;
    }

    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION: DsrCheckSeenEntry
// PURPOSE:  Searching for an Request Seen entry in the request table by
//           source address, a source address can contain 16 request ids
// ARGUMENTS: Pointer to request table
//            source address and request id
//            destination address
// RETURN:   None
//-------------------------------------------------------------------------

static
BOOL DsrCheckSeenEntry(
    NodeAddress srcAddr,
    unsigned short seqNum,
    NodeAddress targetAddr,
    DsrRequestTable* reqTable)
{
    DsrRequestEntry* reqEntry = NULL;

    // Check if an entry exist for this source
    reqEntry = DsrSearchRequestEntry(reqTable, srcAddr,
        ROUTING_DSR_SEEN_ENTRY);

    if (reqEntry && DsrCheckSeenEntryForIDAndTarget(seqNum,
        targetAddr, (DsrSeenEntry *) reqEntry->data))
    {
        return TRUE;
    }

    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION: DsrAddSeenEntry
// PURPOSE:  Adding a request seen entry in the seen table
// ARGUMENTS: Pointer to request table
//            source address and request id
//            destination address
// RETURN:   None
//-------------------------------------------------------------------------

static
BOOL DsrAddSeenEntry(
    Node* node,
    NodeAddress srcAddr,
    unsigned short seqNum,
    NodeAddress targetAddr,
    DsrRequestTable* reqTable)
{
    DsrSeenEntry* seenEntry = NULL;
    DsrRequestEntry* reqEntry = NULL;
    BOOL retVal = FALSE;

    // Check if an entry exist for this source
    reqEntry = DsrSearchRequestEntry(
                   reqTable, srcAddr,ROUTING_DSR_SEEN_ENTRY);

    if (!reqEntry)
    {
        // Create a new entry
        seenEntry = (DsrSeenEntry *) MEM_malloc(sizeof(DsrSeenEntry));

        seenEntry->front = 0;
        seenEntry->rear = 0;
        seenEntry->size = 1;
        seenEntry->seqNumber[seenEntry->rear] = seqNum;
        seenEntry->targetAddr[seenEntry->rear] = targetAddr;

        DsrAddToRequestTable(reqTable, ROUTING_DSR_SEEN_ENTRY,
            srcAddr, (void *) seenEntry, node->getNodeTime());

        retVal = TRUE;
    }
    else if (DsrCheckSeenEntryForIDAndTarget(seqNum,
        targetAddr, (DsrSeenEntry *) reqEntry->data))
    {
        retVal = FALSE;
    }
    else
    {
        // Insert seqNum and targetAddr into FIFO cache
        seenEntry = (DsrSeenEntry *) reqEntry->data;

        // If queue is full drop front entry
        if (seenEntry->size == DSR_REQUEST_TABLE_IDS)
        {
            seenEntry->front = (seenEntry->front + 1) %
                                    DSR_REQUEST_TABLE_IDS;
            seenEntry->size--;
        }

        seenEntry->rear = (seenEntry->rear + 1) % DSR_REQUEST_TABLE_IDS;
        seenEntry->size++;

        seenEntry->seqNumber[seenEntry->rear] = seqNum;
        seenEntry->targetAddr[seenEntry->rear] = targetAddr;

        reqEntry->timeStamp = node->getNodeTime();

        retVal = TRUE;
    }

    return retVal;
}


//-------------------------------------------------------------------------
// Functions related to request sent table

//-------------------------------------------------------------------------
// FUNCTION: DsrAddSentEntry
// PURPOSE:  Adding a request sent entry in the sent table
// ARGUMENTS: Pointer to request table
//            destination address and ttl
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrAddSentEntry(
    Node* node,
    NodeAddress destAddr,
    int ttl,
    DsrRequestTable* reqTable)
{
    DsrSentEntry* newNode = NULL;

    // Create a new entry
    newNode = (DsrSentEntry *) MEM_malloc(sizeof(DsrSentEntry));

    newNode->backoffInterval = DSR_NON_PROP_REQUEST_TIMEOUT;
    newNode->ttl = ttl;
    newNode->count = 1;

    DsrAddToRequestTable(reqTable, ROUTING_DSR_SENT_ENTRY,
            destAddr, (void *) newNode, node->getNodeTime());
}


//-------------------------------------------------------------------------
// FUNCTION: DsrDeleteSentEntry
// PURPOSE:  Deleting a request sent entry from the sent table
// ARGUMENTS: Pointer to request table
//            destination address to be deleted
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrDeleteSentEntry(
    DsrRequestTable* reqTable,
    NodeAddress addr)
{

    DsrRequestEntry* deletingEntry = DsrSearchRequestEntry(
                                         reqTable,
                                         addr,
                                         ROUTING_DSR_SENT_ENTRY);
    DsrRemoveFromRequestTable(reqTable, deletingEntry);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrCheckSentEntry
// PURPOSE:  Searching sent table for a specified destination address
// ARGUMENTS: Pointer to request table
//            destination address
// RETURN:   None
//-------------------------------------------------------------------------

static
DsrRequestEntry* DsrCheckSentEntry(
    NodeAddress destAddr,
    DsrRequestTable* reqTable)
{
    return DsrSearchRequestEntry(
                   reqTable, destAddr,ROUTING_DSR_SENT_ENTRY);
}


//////////////////////////////////////////////////////////////////////
// Functions related to message buffer

//-------------------------------------------------------------------------
// FUNCTION: DsrInitBuffer
// PURPOSE:  Initializing dsr packet buffer where packets waiting for a
//           route are to be stored
// ARGUMENTS: Pointer to the message buffer
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrInitBuffer(DsrBuffer *msgBuffer)
{
    // Initialize message buffer
    msgBuffer->head = NULL;
    msgBuffer->sizeInPacket = 0;
    msgBuffer->sizeInByte = 0;
}


//--------------------------------------------------------------------------
// FUNCTION: DsrInsertBuffer
// PURPOSE:  Inserting a new packet in message buffer in ascending order
//           of destination address
// ARGUMENTS: node, the node inserting the message
//            msg,  the message to be inserted
//            destAddr, the destination to which the message to be sent
//            msgBuffer, dsr buffer to store message temporarily
// RETURN:   None
//--------------------------------------------------------------------------

static
void DsrInsertBuffer(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    BOOL isErrorPacket,
    DsrBuffer* msgBuffer)
{
    DsrBufferEntry* newEntry = NULL;
    DsrBufferEntry* current = msgBuffer->head;
    DsrBufferEntry* previous = NULL;
    IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

    unsigned char* addrId = NULL;
    unsigned short ipId = 0;

    DsrData* dsr = (DsrData*)NetworkIpGetRoutingProtocol(
                       node,
                       ROUTING_PROTOCOL_DSR);

    // if the buffer exceeds silently drop first packet
    // if no buffer size is specified in bytes it will only check for
    // number of packet.

    if (dsr->bufferMaxSizeInByte == 0)
    {
        if (msgBuffer->sizeInPacket == dsr->bufferMaxSizeInPacket)
        {
            MESSAGE_Free(node, msg);
            dsr->stats.numDataDroppedForOverlimit++;
            return;
        }
    }
    else
    {
        if ((msgBuffer->sizeInByte + MESSAGE_ReturnPacketSize(msg)) >
            dsr->bufferMaxSizeInByte)
        {
            MESSAGE_Free(node, msg);
            dsr->stats.numDataDroppedForOverlimit++;
            return;
        }
    }

    // Find Insertion point.  Insert after all address matches.
    // This is to maintain a sorted list in ascending order of the
    // destination address
    while ((current != NULL) && (current->destAddr <= destAddr))
    {
        previous = current;
        current = current->next;
    }

    // Allocate space for the new message
    newEntry = (DsrBufferEntry *)
                    MEM_malloc(sizeof(DsrBufferEntry));

    // Store the allocate message along with the destination number and the
    // the time at which the packet has been inserted
    newEntry->destAddr = destAddr;
    newEntry->msg = msg;
    newEntry->timeStamp = node->getNodeTime();
    newEntry->next = current;
    newEntry->isErrorPacket = isErrorPacket;

    // Increase the size of the buffer
    ++(msgBuffer->sizeInPacket);
    msgBuffer->sizeInByte += MESSAGE_ReturnPacketSize(msg);

    // Here we are adding one message in the buffer. So set the timeout
    // of it.
    addrId = (unsigned char *) MEM_malloc(sizeof(NodeAddress)
        + sizeof(unsigned short));

    memcpy(addrId, &destAddr, sizeof(NodeAddress));

    ipId = (unsigned short) ipHdr->ip_id;
    memcpy(addrId + sizeof(NodeAddress), &ipId, sizeof(unsigned short));

    DsrSetTimer(
        node,
        addrId,
        sizeof(NodeAddress) + sizeof(unsigned short),
        MSG_NETWORK_PacketTimeout,
        DSR_SEND_BUFFER_TIMEOUT);


    if (DEBUG_SEND_BUFFER)
    {
        char clockStr[100];
        TIME_PrintClockInSecond(newEntry->timeStamp, clockStr);
        printf("Node-%u Adding a packet into the SEND Buffer:\n",
            node->nodeId);
        printf("Packet Destination:- %u at %s IpId:- %u\n",
            newEntry->destAddr,
            clockStr,
            ipId);
    }

    MEM_free(addrId);

    // Got the insertion point
    if (previous == NULL)
    {
        // The is the first message in the buffer
        msgBuffer->head = newEntry;
    }
    else
    {
        // This is an intermediate node in the list
        previous->next = newEntry;
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrGetPacketFromBuffer
// PURPOSE:  Get a packet from the message buffer for a specified
//           destination for which a route has been found
// ARGUMENTS: Pointer to the message buffer and destination address
// RETURN:   None
//-------------------------------------------------------------------------

static
Message* DsrGetPacketFromBuffer(
    NodeAddress destAddr,
    DsrBuffer* buffer,
    BOOL *isErrorPacket)
{
    DsrBufferEntry* current = buffer->head;
    Message* pktToDest = NULL;
    DsrBufferEntry* toFree = NULL;

    if (!current)
    {
        // No packet in the buffer so nothing to do
    }
    else if (current->destAddr == destAddr)
    {
        // The first packet is the desired packet
        toFree = current;
        buffer->head = toFree->next;

        pktToDest = current->msg;

        buffer->sizeInByte -= MESSAGE_ReturnPacketSize(toFree->msg);
    *isErrorPacket = toFree->isErrorPacket;
        MEM_free(toFree);
        --(buffer->sizeInPacket);
    }
    else
    {
        while (current->next && current->next->destAddr < destAddr)
        {
            current = current->next;
        }

        if (current->next && current->next->destAddr == destAddr)
        {
            // Got the matched destination so return the packet
            toFree = current->next;
            current->next = toFree->next;

            pktToDest = toFree->msg;

            buffer->sizeInByte -= MESSAGE_ReturnPacketSize(toFree->msg);
        *isErrorPacket = toFree->isErrorPacket;
            MEM_free(toFree);
            --(buffer->sizeInPacket);
        }
    }
    return pktToDest;
}


//-------------------------------------------------------------------------
// FUNCTION: DsrLookupBuffer
// PURPOSE:  Checking the message buffer if it contains any packet for a
//           specific destination
// ARGUMENTS: Pointer to the message buffer and destination address
// RETURN:   None
//-------------------------------------------------------------------------

static
BOOL DsrLookupBuffer(NodeAddress destAddr, DsrBuffer* buffer)
{
    DsrBufferEntry* current = buffer->head;

    while (current && current->destAddr < destAddr)
    {
        current = current->next;
    }

    if (current && current->destAddr == destAddr)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrFindAndDropPacketWithDestAndId
// PURPOSE:  Dropping a specific packet which has timed out
// ARGUMENTS: Pointer to the message buffer and destination address and id
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrFindAndDropPacketWithDestAndId(
    Node* node,
    DsrData* dsr,
    NodeAddress destAddr,
    unsigned short packetId,
    DsrBuffer* msgBuffer)
{
    DsrBufferEntry* current = msgBuffer->head;
    DsrBufferEntry* prevEntry = current;
    DsrBufferEntry* toFree = NULL;
    IpHeaderType* ipHdr = NULL;

    while (current && current->destAddr < destAddr)
    {
        prevEntry = current;
        current = current->next;
    }

    if (current)
    {
        while (current && current->destAddr == destAddr)
        {
            ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(current->msg);

            if (ipHdr->ip_id == packetId)
            {
                // Found the desired matching
                if (prevEntry == current)
                {
                    // The first node matched
                    msgBuffer->head = current->next;
                }
                else
                {
                    prevEntry->next = current->next;
                }
                toFree = current;
                break;
            }
            else
            {
                prevEntry = current;
                current = current->next;
            }
        }
        if (toFree)
        {
            // Update statistics for packet dropped for no route
            dsr->stats.numDataDroppedForNoRoute++;

            msgBuffer->sizeInByte -= MESSAGE_ReturnPacketSize(toFree->msg);
            --(msgBuffer->sizeInPacket);

            if (DEBUG_SEND_BUFFER)
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("Node-%u Deleting a packet from the SEND Buffer"
                       " due to time out:\n",node->nodeId);
                printf("IpId:- %u Time:- %s\n",packetId, clockStr);
            }

            MESSAGE_Free(node, toFree->msg);
            MEM_free(toFree);
        }
    }
}

//------------------------------------------------------------------------
// Functions related to handle retransmit buffer

//-------------------------------------------------------------------------
// FUNCTION: DsrInitRexmtBuffer
// PURPOSE:  Initializing Dsr retransmit buffer, where packets will wait
//           for next hop confirmation
// ARGUMENTS: Pointer to Retransmit buffer
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrInitRexmtBuffer(DsrRexmtBuffer *rexmtBuffer)
{
    // Initialize message buffer
    rexmtBuffer->head = NULL;
    rexmtBuffer->sizeInPacket = 0;
}


//-------------------------------------------------------------------------
// FUNCTION: DsrInsertRexmtBuffer
// PURPOSE:  Inserting a packet in the retransmit buffer
// ARGUMENTS: Pointer to Retransmit buffer, the packet pointer, next hop for
//            the packet
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrInsertRexmtBuffer(
    Node* node,
    Message* msg,
    NodeAddress nextHop,
    DsrRexmtBuffer* rexmtBuffer)
{
    DsrRexmtBufferEntry* newEntry = NULL;
    DsrRexmtBufferEntry* current = rexmtBuffer->head;
    DsrRexmtBufferEntry* previous = NULL;
    // char* addrId = (char *) MEM_malloc(sizeof(NodeAddress) +
    //    sizeof(unsigned short));

    IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
    NodeAddress destAddr = ipHdr->ip_dst;
    NodeAddress srcAddr = ipHdr->ip_src;
    // unsigned short ipId = 0;

    // if the buffer exceeds silently drop first packet
    // if no buffer size is specified in bytes it will only check for
    // number of packet.
    if (rexmtBuffer->sizeInPacket == DSR_REXMT_BUFFER_SIZE)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    // Find Insertion point.  Insert after all address matches.
    // This is to maintain a sorted list in ascending order of the
    // destination address

    while ((current) && (current->destAddr <= destAddr))
    {
        previous = current;
        current = current->next;
    }

    // Allocate space for the new message
    newEntry = (DsrRexmtBufferEntry *)
                    MEM_malloc(sizeof(DsrRexmtBufferEntry));

    // Store the allocate message along with the destination number and the
    // the time at which the packet has been inserted
    if (DEBUG_MAINTENANCE_BUFFER)
    {
        printf("Node %u Inserting packet in the rexmt buffer with:\n"
            "destAddr: %u\n"
            "srcAddr: %u\n"
            "ipId: %u\n", node->nodeId, srcAddr, destAddr, ipHdr->ip_id);
    }

    newEntry->destAddr = destAddr;
    newEntry->srcAddr = srcAddr;
    newEntry->nextHop = nextHop;
    newEntry->count = 0;
    newEntry->msgId = (unsigned short) ipHdr->ip_id;
    newEntry->msg = msg;
    newEntry->timeStamp = node->getNodeTime();
    newEntry->next = current;

    // Increase the size of the buffer
    ++(rexmtBuffer->sizeInPacket);

    // Here we are adding one message in the buffer. So set the timeout
    // of it.
    // memcpy(addrId, &srcAddr, sizeof(NodeAddress));

    // ipId = ipHdr->ip_id;

    // memcpy(addrId + sizeof(NodeAddress), &ipId, sizeof(unsigned short));

    // The timer below is not necessary now. But can be required if
    // underlying MAC don't have any mechanism for acknowledgement

    // DsrSetTimer(
    //     node,
    //     addrId,
    //     sizeof(NodeAddress) + sizeof(unsigned short),
    //     MSG_NETWORK_RexmtTimeout,
    //     DSR_MAIN_THOLDOFF_TIME);

    //MEM_free(addrId);

    // Got the insertion point
    if (previous == NULL)
    {
        // The is the first message in the buffer
        rexmtBuffer->head = newEntry;
    }
    else
    {
        // This is an intermediate node in the list
        previous->next = newEntry;
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrDeleteRexmtBufferByNextHop
// PURPOSE:  Deleting all entries from the retransmit buffer which are sent
//           to a particular next hop
// ARGUMENTS: Pointer to Retransmit buffer, next hop address
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrDeleteRexmtBufferByNextHop(
    Node* node,
    DsrRexmtBuffer* rexmtBuffer,
    NodeAddress nextHop)
{
    DsrRexmtBufferEntry* current = rexmtBuffer->head;
    DsrRexmtBufferEntry* prev = NULL;
    DsrRexmtBufferEntry* toFree;

    while (current)
    {
        if (current->nextHop == nextHop)
        {
            toFree = current;

            if (prev == NULL)
            {
                rexmtBuffer->head = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            current = current->next;

            --(rexmtBuffer->sizeInPacket);

            MESSAGE_Free(node, toFree->msg);
            MEM_free(toFree);
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrDeleteRexmtBufferByEntry
// PURPOSE:  Deleting a specific entry from the retransmit buffer
// ARGUMENTS: Pointer to Retransmit buffer, pointer to the entry
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrDeleteRexmtBufferByEntry(
    Node* node,
    DsrRexmtBuffer* rexmtBuffer,
    DsrRexmtBufferEntry* rexmtBufEntry)
{
    DsrRexmtBufferEntry* current = rexmtBuffer->head;
    DsrRexmtBufferEntry* prev = NULL;

    while (current && current != rexmtBufEntry)
    {
        prev = current;
        current = current->next;
    }

    ERROR_Assert(current,
        "Deleting entry from RexmtBuffer that doesn't exist");

    if (DEBUG_ERROR)
    {
        printf("Deleted rexmt buffer entry with:\n"
            "source address: %u\n"
            "dest Address: %u\n"
            "ipId: %u\n",
            current->srcAddr,
            current->destAddr,
            current->msgId);
    }

    if (prev == NULL)
    {
        rexmtBuffer->head = current->next;
    }
    else
    {
        prev->next = current->next;
    }
    --(rexmtBuffer->sizeInPacket);

    MESSAGE_Free(node, current->msg);
    MEM_free(current);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrSearchRexmtBuffer
// PURPOSE:  Searching Retransmit buffer for a specific entry by
//           source address destination address and id of ip header
// ARGUMENTS: Pointer to Retransmit buffer, source address, destination
//            address and id of ip header
// RETURN:   None
//-------------------------------------------------------------------------

static
DsrRexmtBufferEntry* DsrSearchRexmtBuffer(
    DsrRexmtBuffer* rexmtBuffer,
    NodeAddress destAddr,
    NodeAddress sourceAddr,
    NodeAddress nextHop,
    unsigned short ipId)
{
    DsrRexmtBufferEntry* current = rexmtBuffer->head;

    if (DEBUG_MAINTENANCE_BUFFER)
    {
        printf("Trying to find a packet with:\n"
            "\tdestAddr %u\n"
            "\tsourceAddr %u\n"
            "\tipId %u\n", destAddr, sourceAddr, ipId);
    }

    while (current && current->destAddr <= destAddr)
    {
        if (DEBUG_MAINTENANCE_BUFFER)
        {
            printf("Content of current packet:\n"
                "\tdestAddr %u\n"
                "\tsourceAddr %u\n"
                "\tipId %u\n"
                "\tcount %u\n"
                "\tsequence %u\n",
                current->destAddr,
                current->srcAddr,
                current->msgId,
                current->count,
                current->msg->sequenceNumber);
        }

        if (current->destAddr == destAddr
            && current->srcAddr == sourceAddr
            && current->msgId == ipId
            && current->nextHop == nextHop)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

//-------------------------------------------------------------------------
// FUNCTION: DsrShrinkHeader
// PURPOSE:  Shrinks the DSR header
// ARGUMENTS: Pointer to node, Pointer to message, size by which to shrink
//            the header
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrShrinkHeader(Node* node, Message* msg, Int32 size)
{
    MESSAGE_ShrinkPacket(node, msg, size);

    // As we are shrinking DSR header, the header size saved in array
    // 'msg->headerSizes' needs to be updated

    msg->headerSizes[msg->numberOfHeaders - 1] -= size;
}

//-------------------------------------------------------------------------
// FUNCTION: DsrExpandHeader
// PURPOSE:  Expands the DSR header
// ARGUMENTS: Pointer to node, Pointer to message, size by which to expand
//            the header
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrExpandHeader(Node* node, Message* msg, Int32 size)
{
    MESSAGE_ExpandPacket(node, msg, size);

    // As we are expanding DSR header, the header size saved in array
    // 'msg->headerSizes' needs to be updated

    msg->headerSizes[msg->numberOfHeaders - 1] += size;
}

//--------------------------------------------------------------------------
// FUNCTION: DsrTransmitDataWithSrcRoute
// PURPOSE:  Sending a data packet after adding Dsr source route option
// ARGUMENTS: node pointer, dsr structure pointer, incoming packet,
//            destination address, pointer to the route for the destination
// ASSUMPTION: None
//--------------------------------------------------------------------------

static
void DsrTransmitDataWithSrcRoute(
    Node* node,
    DsrData* dsr,
    Message* msg,
    NodeAddress destAddr,
    DsrRouteEntry* rtPtr,
    unsigned int salvage)
{
    unsigned char *dataPtr;
    int dataLen;
    unsigned short payloadLen;
    NodeAddress nextHop;
    int numNodesInSrcRt = 0;

    IpHeaderType* ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

    int ipHeaderSize = IpHeaderSize(ipHeader);
    IpHeaderType origIpHeader;

    NetworkDataIp* ipNetworkData = (NetworkDataIp *) node->networkData.networkVar;

    if (salvage)
    {
        numNodesInSrcRt = rtPtr->hopCount;
    }
    else
    {
        numNodesInSrcRt = rtPtr->hopCount - 1;
    }

    if (rtPtr->hopCount > 1)
    {
        // Back up original IP header
        memcpy(&origIpHeader, ipHeader, ipHeaderSize);

        // Remove IP header
        MESSAGE_RemoveHeader(node, msg, ipHeaderSize, TRACE_IP);

        if (origIpHeader.ip_p == IPPROTO_DSR)
        {
            // We don't need to add DSR header. only the optional
            // fields should be added and header option size should
            // be modified as per new option length.

            unsigned short oldPayloadLen = 0;

            memcpy(&oldPayloadLen, MESSAGE_ReturnPacket(msg)
                + sizeof(unsigned short), sizeof(unsigned short));

            dataLen = DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH
                          + (numNodesInSrcRt) * sizeof(NodeAddress);

            payloadLen = (unsigned short) (oldPayloadLen + dataLen);
            DsrExpandHeader(node, msg, dataLen);
        }
        else
        {
            // Need to add dsr header
            dataLen = DSR_SIZE_OF_HEADER
                          + DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH
                              + (numNodesInSrcRt) * sizeof(NodeAddress);

            payloadLen = (unsigned short) (dataLen - DSR_SIZE_OF_HEADER);
            MESSAGE_AddHeader(node, msg, dataLen, TRACE_DSR);
        }

        dataPtr = (unsigned char*) MESSAGE_ReturnPacket(msg);

        // Add common header
        DsrAddCommonHeader(
            dataPtr,
            origIpHeader.ip_p,
            payloadLen);

        dataPtr += DSR_SIZE_OF_HEADER;

        // Insert DSR Source Route option
        ERROR_Assert(rtPtr->destAddr == destAddr, "No route\n");

        // Add source route option
        DsrAddSrcRouteOption(
            dsr,
            dataPtr,
            (unsigned char *) rtPtr->path,
            rtPtr->hopCount - 1,
            0,
            0,
            salvage,
            rtPtr->hopCount - 1);

        MESSAGE_AddHeader(node, msg, ipHeaderSize, TRACE_IP);

        // Change different fields of ip header
        ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
        memcpy(ipHeader, &origIpHeader, ipHeaderSize);
        IpHeaderSetIpDontFrag(&(ipHeader->ipFragment), 1);
        IpHeaderSetIpLength(&(ipHeader->ip_v_hl_tos_len),
            MESSAGE_ReturnPacketSize(msg));
        ipHeader->ip_ttl = (unsigned char) rtPtr->hopCount;
        ipHeader->ip_p = IPPROTO_DSR;
    }
    else
    {
        IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
        IpHeaderSetIpDontFrag(&(ipHdr->ipFragment), 1);
        ipHeader->ip_ttl = 1;
    }

    memcpy(&nextHop, &rtPtr->path[0], sizeof(NodeAddress));

    if (DEBUG_TRACE)
    {
        IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

        if (ipHdr->ip_p == IPPROTO_DSR)
        {
            int ipHdrLen = IpHeaderSize(ipHdr);
            unsigned char* dsrPktPtr = ((unsigned char *) ipHdr) + ipHdrLen;

            DsrTrace(
                node,
                dsr,
                dsrPktPtr,
                "Send",
                ipHdr->ip_dst,
                ipHdr->ip_src);
        }
    }

    // Send out the packet

    ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

    if (IpHeaderGetIpLength(ipHeader->ip_v_hl_tos_len) <
        (unsigned int) ipNetworkData->maxPacketLength )
    {
        // Insert the packet in retransmit buffer
        DsrInsertRexmtBuffer(
            node,
            MESSAGE_Duplicate(node, msg),
            nextHop,
            &dsr->rexmtBuffer);

        NetworkIpSendPacketToMacLayer(
            node,
            msg,
            DEFAULT_INTERFACE,
            nextHop);
    }
    else
    {
        dsr->stats.numPacketsGreaterMTU++;
        MESSAGE_Free(node,msg);
    }
}


//-------------------------------------------------------------------------
// Functions related to Route Cache

//--------------------------------------------------------------------------
// FUNCTION: DsrPrintRoute
// PURPOSE:  Function to print a route entry, to be used for debugging
// ARGUMENTS: pointer to a route entry
// ASSUMPTION: None
//--------------------------------------------------------------------------

static
void DsrPrintRoute(DsrRouteEntry* rtEntry)
{
    char addrStr[MAX_ADDRESS_STRING_LENGTH];
    int i;

    IO_ConvertIpAddressToString(
        rtEntry->path[rtEntry->hopCount - 1],
        addrStr);

    printf("Dest Addr = %s    cost = %d\n", addrStr, rtEntry->hopCount);

    for (i = 0; i < rtEntry->hopCount; i++)
    {
        IO_ConvertIpAddressToString(rtEntry->path[i], addrStr);

        printf("\tHop[%d] = %s\n", i + 1, addrStr);
    }
}


//--------------------------------------------------------------------------
// FUNCTION: DsrPrintRoutingTable
// PURPOSE:  Function to print a route cache, to be used for debugging
// ARGUMENTS: route cache pointer
// ASSUMPTION: None
//--------------------------------------------------------------------------

static
void DsrPrintRoutingTable(Node* node)
{
    DsrData* dsr = (DsrData *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);
    DsrRouteCache* rtCache = &dsr->routeCache;
    DsrRouteEntry* current = NULL;

    printf("----------------------------------------------------\n"
           "          Routing Table for Node %u\n"
           "----------------------------------------------------\n",
           node->nodeId);

    if (!rtCache->count)
    {
        printf("\tNo routes in routing table\n\n");
        return;
    }

    for (current = rtCache->deleteListHead; current != NULL; current = current->deleteNext)
    {
        DsrPrintRoute(current);
    }
    printf("\n");
}


//--------------------------------------------------------------------------
// FUNCTION: DsrInitRouteCache
// PURPOSE:  Function to initialize Dsr route cache
// ARGUMENTS: route cache pointer
// ASSUMPTION: None
//--------------------------------------------------------------------------

static
void DsrInitRouteCache(DsrRouteCache* routeCache)
{
    // Initialize DSR route Cache
    int i = 0;
    for (i = 0; i < DSR_ROUTE_HASH_TABLE_SIZE; i++)
    {
        routeCache->hashTable[i] = NULL;
    }
    routeCache->deleteListHead = NULL;
    routeCache->deleteListTail = NULL;
    routeCache->count = 0;
 }

//--------------------------------------------------------------------------
// FUNCTION: DsrInsertRoute
// PURPOSE:  Function to insert a new route entry in the route cache
// ARGUMENTS: route cache pointer, source route and source route length
// ASSUMPTION: None
//--------------------------------------------------------------------------

static
DsrRouteEntry* DsrInsertRoute(
    Node* node,
    DsrRouteCache* routeCache,
    unsigned char* pathList,
    int pathListLength)
{

    DsrData* dsr = (DsrData *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    DsrRouteEntry* current = NULL;
    DsrRouteEntry* newEntry = NULL;
    NodeAddress destAddr;
    int hopCount;
    Message *msg = NULL;
    BOOL isErrorPacket = FALSE;

    DsrRouteEntry* previous = NULL;

    int queueNo  = 0;

    // Get hop count and destination address
    hopCount = pathListLength / sizeof(NodeAddress);
    memcpy(&destAddr,
        pathList + ((hopCount - 1) * sizeof(NodeAddress)),
        sizeof(NodeAddress));

    queueNo  = destAddr % DSR_ROUTE_HASH_TABLE_SIZE;

    //point to the first element
    current = routeCache->hashTable[queueNo];
    previous = current;

    // Insert route in sorted order
    while (current
           && (current->destAddr < destAddr
              ||(current->destAddr == destAddr
                 && current->hopCount <= hopCount)))
    {
        // Check if the two routes are same
        if (current->destAddr == destAddr
            && current->hopCount == hopCount
            && !memcmp(current->path, pathList, pathListLength))
        {

            // Move the entry at the tail.
            // if it is presently at the tail do nothing.
            if (routeCache->deleteListTail != current)
            {
                // Remove from current position

                current->deleteNext->deletePrev = current->deletePrev;

                if (routeCache->deleteListHead == current)
                {
                    routeCache->deleteListHead = current->deleteNext;
                }
                else
                {
                    current->deletePrev->deleteNext = current->deleteNext;
                }

                // Add at the last position
                current->deletePrev = routeCache->deleteListTail;
                current->deleteNext = NULL;
                routeCache->deleteListTail->deleteNext = current;
                routeCache->deleteListTail = current;

            }

            current->routeEntryTime = node->getNodeTime();
            return current;
        }
        previous = current;
        current = current->next;
    }

    // Update statistics for number of route selected and sum of their
    // hop count
    dsr->stats.numRoutes++;
    dsr->stats.numHops += hopCount;

   // newEntry = (DsrRouteEntry *) MEM_malloc(sizeof(DsrRouteEntry));
    newEntry = DsrMemoryMalloc(dsr);

    newEntry->destAddr = destAddr;
    newEntry->hopCount = hopCount;
    newEntry->routeEntryTime = node->getNodeTime();
    newEntry->prev = NULL;
    newEntry->next = NULL;
    newEntry->deletePrev = NULL;
    newEntry->deleteNext = NULL;


    newEntry->path = (NodeAddress *) MEM_malloc(pathListLength);
    memcpy(newEntry->path, pathList, pathListLength);

    if (dsr->isTimerSet == FALSE)
    {
        dsr->isTimerSet = TRUE; // it prevent to schedule another
                              // time out timer

        DsrSetTimer(
            node,
            NULL,
            0,
            MSG_NETWORK_DeleteRoute,
            DSR_ROUTE_CACHE_TIMEOUT);
    }

    if (!current)
    {
        // This can happen in two cases
        // 1) last element of the cache or
        // 2) no route in route cache

        if (routeCache->hashTable[queueNo] == NULL)
        {
            // There is no entry in route cache
            routeCache->hashTable[queueNo] = newEntry;

        }
        else
        {
            // Last entry of the route cache
            newEntry->prev = previous;
            previous->next = newEntry;
        }
    }
    else
    {
        // Insert before current route
        newEntry->prev = current->prev;
        newEntry->next = current;
        // If it's the first element
        if (current == previous)
        {
            routeCache->hashTable[queueNo] = newEntry;
        }
        else
        {
            current->prev->next = newEntry;
        }
        current->prev = newEntry;


    }

    newEntry->deletePrev = routeCache->deleteListTail;
    newEntry->deleteNext = NULL;

    if (routeCache->deleteListTail)
    {
        routeCache->deleteListTail->deleteNext = newEntry;
    }
    else
    {
        routeCache->deleteListHead = newEntry;
    }

    routeCache->deleteListTail = newEntry;

    routeCache->count++;

    msg = DsrGetPacketFromBuffer(destAddr, &dsr->msgBuffer, &isErrorPacket);

    while (msg)
    {
        // Update statistical variable for number of data transmitted
        IpHeaderType* ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

        if (ipHeader->ip_p != IPPROTO_DSR && !isErrorPacket)
        {
            dsr->stats.numDataInitiated++;
        }

        DsrTransmitDataWithSrcRoute(node, dsr, msg, destAddr, newEntry, 0);

        msg = DsrGetPacketFromBuffer(destAddr, &dsr->msgBuffer, &isErrorPacket);
    }

    if (DEBUG_ROUTING_TABLE)
    {
        printf("********** After Adding a Route at Node %u **********\n",
            node->nodeId);

        DsrPrintRoutingTable(node);
    }
    return newEntry;
}

//--------------------------------------------------------------------------
// FUNCTION: DsrRemoveRouteEntry
// PURPOSE:  Function to remove a route entry from the route cache
// ARGUMENTS: route cache pointer, route entry to be removed and queue number
// ASSUMPTION: None
//--------------------------------------------------------------------------

static
void DsrRemoveRouteEntry(DsrRouteCache* routeCache,DsrRouteEntry* current, int queueNo)
{

    // delete the route
    routeCache->count--;

    if (routeCache->deleteListHead == current)
    {
        routeCache->deleteListHead = current->deleteNext;
    }
    else
    {
        current->deletePrev->deleteNext = current->deleteNext;
    }

    if (current->deleteNext)
    {
        current->deleteNext->deletePrev = current->deletePrev;
    }
    else
    {
        routeCache->deleteListTail = current->deletePrev;
    }


    if (current->prev)
    {
        current->prev->next = current->next;
    }
    else
    {
        routeCache->hashTable[queueNo] = current->next;
    }

    if (current->next)
    {
        current->next->prev = current->prev;
    }
}

//--------------------------------------------------------------------------
// FUNCTION: DsrDeleteRoute
// PURPOSE:  Function to delete a route entry from the route cache
// ARGUMENTS: Node Pointer,route cache pointer,Message Pointer
// ASSUMPTION: None
//--------------------------------------------------------------------------

void DsrDeleteRoute(
    Node* node,
    DsrRouteCache* routeCache,
    Message* msg)
{

    DsrData* dsr = (DsrData *)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    clocktype nextTimeOutTime = 0;
    DsrRouteEntry* current = routeCache->deleteListHead;
    DsrRouteEntry* toFree = NULL;
    int queueNo   = 0;

    if (routeCache->count)
    {
        nextTimeOutTime = current->routeEntryTime
                          + DSR_ROUTE_CACHE_TIMEOUT;
    }

    while (current && nextTimeOutTime <= node->getNodeTime())
    {
        queueNo  = current->destAddr % DSR_ROUTE_HASH_TABLE_SIZE;
        toFree   = current;
        current  = current->deleteNext;

        DsrRemoveRouteEntry(routeCache, toFree, queueNo);

        if (DEBUG_ROUTING_TABLE)
        {
            printf("********** After Delete a Route at Node %u **********\n",
                node->nodeId);
            DsrPrintRoutingTable(node);
        }

        if (current)
        {
            nextTimeOutTime = current->routeEntryTime
                              + DSR_ROUTE_CACHE_TIMEOUT;
        }

        MEM_free(toFree->path);
        DsrMemoryFree(dsr,toFree);
    }



    nextTimeOutTime -= node->getNodeTime();

    if (routeCache->count == 0)
    {
        dsr->isTimerSet = FALSE;
        MESSAGE_Free(node, msg);
    }
    else
    {
        MESSAGE_Send(node, msg, nextTimeOutTime);
    }
}

//--------------------------------------------------------------------------
// FUNCTION: DsrDeleteRouteByLink
// PURPOSE: Delete a route entry with specific link with first node addr1
//          and second node addr2. if address 1 is this nodes address then
//          needs to delete those paths starting with addr2
// ARGUMENTS: route cache pointer, two end addresses of the link
// ASSUMPTION: None
//--------------------------------------------------------------------------

static
void DsrDeleteRouteByLink(
    Node* node,
    DsrRouteCache* routeCache,
    NodeAddress addr1,
    NodeAddress addr2)
{
    DsrRouteEntry* current = routeCache->deleteListHead;
    DsrRouteEntry* toFree = NULL;
    DsrData* dsr = (DsrData *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    while (current)
    {
        unsigned char* path = (unsigned char *) current->path;
        NodeAddress currentNode = 0;
        NodeAddress nextNode = 0;
        int i = 0;
        int queueNo = 0;
        toFree = NULL;

        if (addr1 == dsr->localAddress)
        {
            memcpy(&currentNode, &path[0], sizeof(NodeAddress));

            if (currentNode == addr2)
            {
                toFree = current;
            }
        }
        else
        {
            for (i = 0; i < current->hopCount; i++)
            {
                memcpy(&currentNode, &path[i * sizeof(NodeAddress)],
                    sizeof(NodeAddress));

                if (currentNode == addr1 && i != (current->hopCount - 1))
                {
                    memcpy(&nextNode, &path[(i + 1) * sizeof(NodeAddress)]
                        , sizeof(NodeAddress));

                    if (nextNode == addr2)
                    {
                        toFree = current;
                    }
                    break;
                }
            }
        }

        queueNo  = current->destAddr % DSR_ROUTE_HASH_TABLE_SIZE;
        current = current->deleteNext;

        if (toFree)
        {
            DsrRemoveRouteEntry(routeCache, toFree, queueNo);

            if (DEBUG_ROUTING_TABLE)
            {
                printf("******* After Delete a Route at Node %u *******\n",
                    node->nodeId);

                DsrPrintRoutingTable(node);
            }

            MEM_free(toFree->path);
            DsrMemoryFree(dsr,toFree);
        }
    } // End While
}


//--------------------------------------------------------------------------
// FUNCTION: DsrCheckRouteCache
// PURPOSE: Check if a route exists for a destination
// ARGUMENTS: route cache pointer, destination address
// ASSUMPTION: None
//--------------------------------------------------------------------------

static
DsrRouteEntry* DsrCheckRouteCache(
    NodeAddress destAddr,
    DsrRouteCache* routeCache)
{
    DsrRouteEntry* current = routeCache->hashTable[destAddr % DSR_ROUTE_HASH_TABLE_SIZE];

    while (current && current->destAddr < destAddr)
    {
        current = current->next;
    }

    if (current && current->destAddr == destAddr)
    {
        return current;
    }
    else
    {
        return NULL;
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrInitiateRREP
// PURPOSE:  Initiating a route reply by a node when itself is the
//           destination node.
// ARGUMENTS: the received request, the source address of the route request.
// RETURN:   None
// ASSUMPTION: The destination will reverse back the path in the route
//             request to send the reply.
//-------------------------------------------------------------------------

static
void DsrInitiateRREP(
    Node* node,
    DsrData* dsr,
    unsigned char* recvRreq,
    NodeAddress srcAddr)
{
    Message* newMsg = NULL;
    unsigned char* pktPtr = NULL;

    // size of path in the route reply will be 4 greater than the path size
    // in route request path as current nodes address will be listed in that
    unsigned int sizeOfPathInReply = SizeOfPathInRouteRequest(recvRreq)
                                + sizeof(NodeAddress);

    // Target address will not be included in the source route it will list
    // only the intermediate hops
    unsigned int sizeOfPathInSrcRoute = SizeOfPathInRouteRequest(recvRreq);

    unsigned int hopCountInReply = sizeOfPathInReply / sizeof(NodeAddress);

    unsigned int hopCountInSrcRoute = sizeOfPathInSrcRoute
                                          / sizeof(NodeAddress);

    unsigned int pktSize = DSR_SIZE_OF_HEADER
                               + DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH
                               + sizeOfPathInReply;

    unsigned short payloadLen = (unsigned short) (pktSize
                                    - DSR_SIZE_OF_HEADER);

    unsigned char* replyPathInfo = NULL;

    NetworkDataIp* ipNetworkData = (NetworkDataIp *) node->networkData.networkVar;

    IpHeaderType* ipHeader = NULL;


    NodeAddress nextHop = srcAddr;

    if (sizeOfPathInSrcRoute)
    {
        pktSize += DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH
                      + sizeOfPathInSrcRoute;

        payloadLen = (unsigned short) (payloadLen + (DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH
                      + sizeOfPathInSrcRoute));
    }

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0, MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(node, newMsg, pktSize, TRACE_DSR);

    pktPtr = (unsigned char*) MESSAGE_ReturnPacket(newMsg);

    // Add common header in the packet
    DsrAddCommonHeader(
        pktPtr,
        DSR_NO_MORE_HEADER,
        payloadLen);

    pktPtr += DSR_SIZE_OF_HEADER;

    // Add source route in the packet if needed, that is if the
    // source is more than one hop away

    if (sizeOfPathInSrcRoute)
    {
        // allocate space for source route
        unsigned char* srcRt = NULL;
        unsigned int i = 0;

        srcRt = (unsigned char*)MEM_malloc(sizeOfPathInSrcRoute);

        for (i = 0; i < hopCountInSrcRoute; i++)
        {
            NodeAddress* hops = (NodeAddress *) srcRt;
            memcpy(hops + hopCountInSrcRoute - i - 1,
                recvRreq + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH
                + i * sizeof(NodeAddress),
                sizeof(NodeAddress));
        }

        memcpy(&nextHop, srcRt, sizeof(NodeAddress));

        // Add source route option in dsr packet.
        DsrAddSrcRouteOption(
            dsr,
            pktPtr,
            srcRt,
            hopCountInSrcRoute,
            0,
            0,
            0,
            hopCountInSrcRoute);

        MEM_free(srcRt);

        pktPtr += sizeOfPathInSrcRoute
                      + DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH;
    }

    // Add route reply option

    // Allocate reply path
    replyPathInfo = (unsigned char*)MEM_malloc(sizeOfPathInReply);

    // Assign path.
    memcpy(replyPathInfo, recvRreq + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH,
        sizeOfPathInReply - sizeof(NodeAddress));

    memcpy(replyPathInfo + sizeOfPathInReply - sizeof(NodeAddress),
        &dsr->localAddress, sizeof(NodeAddress));

    DsrAddRouteReplyOption(
        pktPtr,
        replyPathInfo,
        hopCountInReply,
        0);

    MEM_free(replyPathInfo);

    if (DEBUG_TRACE)
    {
        DsrTrace(
            node,
            dsr,
            (unsigned char*) MESSAGE_ReturnPacket(newMsg),
            "Send",
            srcAddr,
            dsr->localAddress);
    }

    // Add ip header to it
    NetworkIpAddHeader(
        node,
        newMsg,
        dsr->localAddress,
        srcAddr,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_DSR,
        hopCountInSrcRoute + 1);

    // update statistical variable for number of route reply initiated
    // as destination node
    dsr->stats.numReplyInitiatedAsDest++;


    ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(newMsg);

    if (IpHeaderGetIpLength(ipHeader->ip_v_hl_tos_len) <
        (unsigned int) ipNetworkData->maxPacketLength )
    {
#ifdef ADDON_DB
        // only added when we are sending packets. 
        // not forwarding ...
        HandleNetworkDBEvents(
            node,
            newMsg,
            DEFAULT_INTERFACE,            
            "NetworkReceiveFromUpper",
            "",
            dsr->localAddress,
            srcAddr,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_DSR);
#endif
        NetworkIpSendPacketToMacLayerWithDelay(
            node,
            newMsg,
            DEFAULT_INTERFACE,
            nextHop,
            (clocktype) (RANDOM_erand(dsr->seed) * DSR_BROADCAST_JITTER));
    }
    else
    {
        dsr->stats.numPacketsGreaterMTU++;
        MESSAGE_Free(node,newMsg);
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrInitiateRREPByIn
// PURPOSE:  Initiating a cached route reply.
// ARGUMENTS: the received request, the source address of the route request.
//            pointer to the cached route entry
// RETURN:   None
// ASSUMPTION: The destination will reverse back the path in the route
//             request to send the reply.
//-------------------------------------------------------------------------

static
void DsrInitiateRREPByIn(
    Node* node,
    DsrData* dsr,
    unsigned char* recvRreq,
    DsrRouteEntry* rtEntry,
    NodeAddress srcAddr,
    BOOL* isSendingCachedReply)
{
    BOOL isLoopInPath = FALSE;
    NodeAddress nextHop = srcAddr;


    *isSendingCachedReply = FALSE;

    // First Check if there is loop in the conjuncted path

    // Check duplicate for the source address
    isLoopInPath = DsrCheckIfAddressExists(
                       (unsigned char *) rtEntry->path,
                       rtEntry->hopCount,
                       srcAddr);

    IpHeaderType* ipHeader = NULL;
    NetworkDataIp* ipNetworkData = (NetworkDataIp *) node->networkData.networkVar;


    // If there is no duplicate source address then check
    // for other addresses recorded in the record route of the
    // reply

    if (!isLoopInPath)
    {
        unsigned int sizeOfPathInRequest =
            SizeOfPathInRouteRequest(recvRreq);
        unsigned int hopCountInRecordRt  =
            sizeOfPathInRequest / sizeof(NodeAddress);

        unsigned int i = 0;

        for (i = 0; i < hopCountInRecordRt; i++)
        {
            NodeAddress addrInRequest;
            memcpy(&addrInRequest,
                (recvRreq + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH
                + i * sizeof(NodeAddress)), sizeof(NodeAddress));

            isLoopInPath = DsrCheckIfAddressExists(
                               (unsigned char *) rtEntry->path,
                               rtEntry->hopCount,
                               addrInRequest);
            if (isLoopInPath)
            {
                break;
            }
        }
    }

    // If there is no loop in the path then send the cached route reply
    // or proceed by forwarding the route request

    if (isLoopInPath)
    {
        *isSendingCachedReply = FALSE;
    }
    else
    {
        // No loop in the accumulated path so send out the cached route
        // reply where the source route is
        // initiator, path information in route request, cached path
        // assuming cached path information includes the destination
        // address

        unsigned int sizeOfPathInRequest =
            SizeOfPathInRouteRequest(recvRreq);

        // final destination is to be excluded in the source route
        unsigned int hopCountInSrcRt =
                    sizeOfPathInRequest / sizeof(NodeAddress);

        unsigned int hopCountInReply = hopCountInSrcRt
                                    + rtEntry->hopCount
                                    + 1; // for inclusion of its own address

        unsigned int pktSize =
            DSR_SIZE_OF_HEADER
                + DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH
                + hopCountInReply * sizeof(NodeAddress);

        unsigned char* pathInReply = NULL;
        unsigned char* srcRt = NULL;

        unsigned char* pktPtr = NULL;
        Message* newMsg = NULL;

        if (((hopCountInReply * sizeof(NodeAddress)) +
            DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH_TYPE_LEN) >
            UCHAR_MAX)
        {
            *isSendingCachedReply = FALSE;
            return;
        }

        *isSendingCachedReply = TRUE;

        if (hopCountInSrcRt)
        {
            pktSize += DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH
                + hopCountInSrcRt * sizeof(NodeAddress);
        }

        pathInReply = (unsigned char*)MEM_malloc(hopCountInReply * sizeof(NodeAddress));

        // Allocate the packet for route reply

        newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0, MSG_MAC_FromNetwork);
        MESSAGE_PacketAlloc(node, newMsg, pktSize, TRACE_DSR);

        pktPtr = (unsigned char*) MESSAGE_ReturnPacket(newMsg);

        // Add common header in the packet
        DsrAddCommonHeader(
            pktPtr,
            (unsigned char) DSR_NO_MORE_HEADER,
            (unsigned short) (pktSize - DSR_SIZE_OF_HEADER));

        pktPtr += DSR_SIZE_OF_HEADER;

        // Add source route if necessary
        if (hopCountInSrcRt)
        {
            unsigned int i = 0;
            srcRt = (unsigned char*)MEM_malloc(hopCountInSrcRt * sizeof(NodeAddress));

            for (i = 0; i < hopCountInSrcRt; i++)
            {
                NodeAddress* hops = (NodeAddress *) srcRt;
                memcpy(hops + hopCountInSrcRt - i - 1,
                    recvRreq + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH
                    + i * sizeof(NodeAddress),
                    sizeof(NodeAddress));
            }

            memcpy(&nextHop, srcRt, sizeof(NodeAddress));

            // Add source route option in dsr packet.
            DsrAddSrcRouteOption(
                dsr,
                pktPtr,
                srcRt,
                hopCountInSrcRt,
                0,
                0,
                0,
                hopCountInSrcRt);

            MEM_free(srcRt);

            pktPtr += hopCountInSrcRt * sizeof(NodeAddress)
                          + DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH;
        }

        // Allocate path for the reply

        // Copy the record route listed in the Route request message
        memcpy(pathInReply, recvRreq
            + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH,
            sizeOfPathInRequest);

        // Append own address
        memcpy(pathInReply + sizeOfPathInRequest, &dsr->localAddress,
            sizeof(NodeAddress));

        // Copy the cached route entry
        memcpy(pathInReply + sizeOfPathInRequest + sizeof(NodeAddress),
            rtEntry->path, rtEntry->hopCount * sizeof(NodeAddress));

        DsrAddRouteReplyOption(
            pktPtr,
            pathInReply,
            hopCountInReply,
            0);

        MEM_free(pathInReply);

        if (DEBUG_TRACE)
        {
            DsrTrace(
                node,
                dsr,
                (unsigned char*) MESSAGE_ReturnPacket(newMsg),
                "Send",
                srcAddr,
                dsr->localAddress);
        }

        // Add ip header to it
        NetworkIpAddHeader(
            node,
            newMsg,
            dsr->localAddress,
            srcAddr,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_DSR,
            hopCountInSrcRt + 1);

        // Update statistical variable for number of cached route reply sent
        dsr->stats.numReplyInitiatedAsIntermediate++;



        ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(newMsg);

        if (IpHeaderGetIpLength(ipHeader->ip_v_hl_tos_len) <
            (unsigned int) ipNetworkData->maxPacketLength )
        {
#ifdef ADDON_DB
            // only added when we are sending packets. 
            // not forwarding ...
            HandleNetworkDBEvents(
                node,
                newMsg,
                DEFAULT_INTERFACE,                
                "NetworkReceiveFromUpper",
                "",
                dsr->localAddress,
                srcAddr,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_DSR);
#endif
            NetworkIpSendPacketToMacLayerWithDelay(
                node,
                newMsg,
                DEFAULT_INTERFACE,
                nextHop,
                (clocktype) (RANDOM_erand(dsr->seed) * DSR_BROADCAST_JITTER));
        }
        else
        {
            dsr->stats.numPacketsGreaterMTU++;
            MESSAGE_Free(node,newMsg);
        }
    }
}


//-------------------------------------------------------------------------
// Functions related Route Request sending


//-------------------------------------------------------------------------
// FUNCTION: DsrInitiateRREQ
// PURPOSE:  Initiating a Route Request packet
// ARGUMENTS: node, The node sending route request
//            dsr,  Dsr internal structure
//            destAddr, The dest for which the message is going to be sent
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrInitiateRREQ(
    Node* node,
    DsrData* dsr,
    NodeAddress destAddr,
    DsrRequestEntry* reqEntry)
{
    Message* newMsg = NULL;
    unsigned char* pktPtr = NULL;

    int pktSize = DSR_SIZE_OF_HEADER
                      + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH;

    unsigned short payloadLen = DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH;

    DsrSentEntry* sentEntry = NULL;

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0, MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(node, newMsg, pktSize, TRACE_DSR);

    pktPtr = (unsigned char*) MESSAGE_ReturnPacket(newMsg);

    // Add common header in the packet
    DsrAddCommonHeader(
        pktPtr,
        (unsigned char) DSR_NO_MORE_HEADER,
        (unsigned short) payloadLen);

    pktPtr += DSR_SIZE_OF_HEADER;

    // Add route request option
    DsrAddRouteRequestOption(
        pktPtr,
        NULL,
        0,
        dsr->identificationNumber,
        destAddr);

    if (DEBUG_TRACE)
    {
        DsrTrace(
            node,
            dsr,
            (unsigned char*) MESSAGE_ReturnPacket(newMsg),
            "Send",
            ANY_DEST,
            dsr->localAddress);
    }

    NetworkIpSendRawMessageToMacLayerWithDelay(
        node,
        newMsg,
        dsr->localAddress,
        ANY_DEST,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_DSR,
        DSR_NON_PROPAGATING_TTL,
        DEFAULT_INTERFACE,
        ANY_DEST,
        (clocktype) (RANDOM_erand(dsr->seed) * DSR_BROADCAST_JITTER));


    if (!reqEntry)
    {
        // Add entry to Request sent table
        DsrAddSentEntry(
            node,
            destAddr,
            DSR_NON_PROPAGATING_TTL,
            &dsr->reqTable);
    }
    else
    {
        // Entry already exists
        sentEntry = (DsrSentEntry *) reqEntry->data;

        reqEntry->timeStamp = node->getNodeTime();
        sentEntry->count = 1;
        sentEntry->backoffInterval = DSR_NON_PROP_REQUEST_TIMEOUT;
        sentEntry->ttl = DSR_NON_PROPAGATING_TTL;
    }

    // Increase sequence number of dsr before sending route request
    dsr->identificationNumber++;

    // update statistical variable for route request initiated
    dsr->stats.numRequestInitiated++;

    DsrSetTimer(
        node,
        &destAddr,
        sizeof(NodeAddress),
        MSG_NETWORK_CheckReplied,
        (clocktype) DSR_NON_PROP_REQUEST_TIMEOUT);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrRetryRREQ
// PURPOSE:  Retrying route request
// ARGUMENTS: node, The node  sending route request
//            dsr,  Dsr internal structure
//            destAddr, The dest for which the message is going to be sent
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrRetryRREQ(Node* node, DsrData* dsr, NodeAddress destAddr,
    DsrRequestEntry* reqEntry)
{
    Message* newMsg = NULL;
    unsigned char* pktPtr = NULL;
    int pktSize = DSR_SIZE_OF_HEADER
                      + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH;

    int payloadLen = DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH;

    DsrSentEntry* sentEntry = (DsrSentEntry *) reqEntry->data;

    // Get sent entry for this request.

    if (sentEntry->count > DSR_MAX_REQUEST_REXMT + 1)
    {
        // can't retransmit the packet assuming the destination is not
        // reachable again re-initiate route discovery

        DsrInitiateRREQ(
            node,
            dsr,
            destAddr,
            reqEntry);

        return;
    }

    // Going to retry route request

    // Track number of request sent.
    reqEntry->timeStamp = node->getNodeTime();
    sentEntry->count++;

    if (DSR_EXPANDING_RING)
    {
        sentEntry->ttl *= 2;
    }
    else
    {
        // Set hop limit.
        sentEntry->ttl = DSR_PROPAGATING_TTL;
    }


    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0, MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(node, newMsg, pktSize, TRACE_DSR);

    pktPtr = (unsigned char*) MESSAGE_ReturnPacket(newMsg);

    // Add common header in the packet
    DsrAddCommonHeader(
        pktPtr,
        (unsigned char) DSR_NO_MORE_HEADER,
        (unsigned short) payloadLen);

    // Skipping dsr header
    pktPtr += DSR_SIZE_OF_HEADER;

    // Add route request option
    DsrAddRouteRequestOption(
        pktPtr,
        NULL,
        0,
        dsr->identificationNumber,
        destAddr);

    if (DEBUG_TRACE)
    {
        DsrTrace(
            node,
            dsr,
            (unsigned char*) MESSAGE_ReturnPacket(newMsg),
            "Send",
            ANY_DEST,
            dsr->localAddress);
    }

    NetworkIpSendRawMessageToMacLayerWithDelay(
        node,
        newMsg,
        dsr->localAddress,
        ANY_DEST,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_DSR,
        sentEntry->ttl,
        DEFAULT_INTERFACE,
        ANY_DEST,
        (clocktype) (RANDOM_erand(dsr->seed) * DSR_BROADCAST_JITTER));

   // Increase sequence number of dsr before sending route request
    dsr->identificationNumber++;

    // Update statistical variable for request resent
    dsr->stats.numRequestResent++;

    if ((sentEntry->backoffInterval * 2) <= DSR_MAX_REQUEST_PERIOD)
    {
        if (sentEntry->count == 2)
        {
            sentEntry->backoffInterval = DSR_REQUEST_PERIOD;
        }
        else
        {
            // Doubles back off interval.
            sentEntry->backoffInterval = sentEntry->backoffInterval *= 2;
        }
    }
    else
    {
        // used fixed value after maximum backoff interval.
        sentEntry->backoffInterval = DSR_MAX_REQUEST_PERIOD;
    }

    DsrSetTimer(
        node,
        &destAddr,
        sizeof(NodeAddress),
        MSG_NETWORK_CheckReplied,
        sentEntry->backoffInterval);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrRelayRREQ
// PURPOSE:  Relaying a route request
// ARGUMENTS: node, The node sending route request
//            dsr,  Dsr internal structure
//            destAddr, The dest for which the message is going to be sent
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrRelayRREQ(
    Node* node,
    DsrData* dsr,
    Message* recvMsg,
    unsigned char* recvRreq,
    NodeAddress srcAddr,
    int ttl)
{
    Message* newMsg = NULL;

    unsigned char* newPktPtr = NULL;
    unsigned char* recvPktPtr = NULL;

    unsigned int sizeOfRecvPath = 0;
    unsigned short previousPayloadLen = 0;

    unsigned int recvPktSize = (unsigned int)
        MESSAGE_ReturnPacketSize(recvMsg);
    unsigned int newPktSize = 0;
    unsigned int sizeBeforeRequestOpt = 0;

    sizeOfRecvPath = SizeOfPathInRouteRequest(recvRreq);

    if (sizeOfRecvPath
        + sizeof(NodeAddress)
        + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH_TYPE_LEN
            > UCHAR_MAX)
    {
        // path size can't be accommodated in request message so discard it
        return;
    }

    newPktSize = recvPktSize + sizeof(NodeAddress);

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0, MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(node, newMsg, newPktSize, TRACE_DSR);

    newPktPtr = (unsigned char *) MESSAGE_ReturnPacket(newMsg);

    recvPktPtr = (unsigned char *) MESSAGE_ReturnPacket(recvMsg);

    // Calculate number of bytes before the request.
    // That portion of the message will go as it is
    sizeBeforeRequestOpt = DsrMod((int)(recvRreq - (unsigned char *)
        MESSAGE_ReturnPacket(recvMsg)));

    // Copy the portion of previous message, that is before route request
    // option.
    memcpy(newPktPtr, recvPktPtr, sizeBeforeRequestOpt);

    // Header Payload size should be incremented by 4 for allocating
    // a new node in path.
    memcpy(&previousPayloadLen, (newPktPtr + sizeBeforeRequestOpt
        - sizeof(short)),
        sizeof(unsigned short));

    previousPayloadLen += sizeof(NodeAddress);

    memcpy((newPktPtr + sizeof(short)), &previousPayloadLen, sizeof(short));

    newPktPtr += sizeBeforeRequestOpt;
    recvPktPtr += sizeBeforeRequestOpt;

    // copy previous nodes route request part
    memcpy(newPktPtr, recvPktPtr, DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH);

    // OptDataLen in Dsr Route Request should be changed
    (*(newPktPtr + sizeof(unsigned char))) += sizeof(NodeAddress);

    newPktPtr = newPktPtr + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH;
    recvPktPtr = recvPktPtr + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH;


    // Copy the path fields
    memcpy(newPktPtr, recvPktPtr, sizeOfRecvPath);

    memcpy((newPktPtr + sizeOfRecvPath), &dsr->localAddress,
        sizeof(NodeAddress));

    // Copy other contents of Data
    newPktPtr += sizeOfRecvPath + sizeof(NodeAddress);
    recvPktPtr += sizeOfRecvPath;

    memcpy(newPktPtr, recvPktPtr, (recvPktSize
        - DsrMod((int)(recvPktPtr
        - (unsigned char *) MESSAGE_ReturnPacket(recvMsg)))));

    if (DEBUG_TRACE)
    {
        DsrTrace(
            node,
            dsr,
            (unsigned char*) MESSAGE_ReturnPacket(newMsg),
            "Send",
            ANY_DEST,
            srcAddr);
    }

    // Update statistical variable for number of request relayed
    dsr->stats.numRequestRelayed++;

    //request is forwarded after decreasing the ttl
    ttl = ttl - IP_TTL_DEC;

    // Done with packet copying
    NetworkIpSendRawMessageToMacLayerWithDelay(
        node,
        newMsg,
        srcAddr,
        ANY_DEST,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_DSR,
        ttl,
        DEFAULT_INTERFACE,
        ANY_DEST,
        (clocktype) (RANDOM_erand(dsr->seed) * DSR_BROADCAST_JITTER));
}


//-------------------------------------------------------------------------
// FUNCTION: DsrSendRERR
// PURPOSE:  Function to send and route error
// ARGUMENTS: node, The node sending route request
//            dsr,  Dsr internal structure
//            droppedMsg, the packet dropped by MAC
//            nextHop, neighboring node address to which transmission failed
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrSendRERR(
    Node* node,
    DsrData* dsr,
    const Message* droppedMsg,
    NodeAddress nextHop)
{
    // Create a dsr packet with route error option
    Message* newMsg = NULL;
    unsigned char* newPktPtr = NULL;
    int dataSize = DSR_SIZE_OF_HEADER + DSR_SIZE_OF_ROUTE_ERROR;
    unsigned short payloadLen = DSR_SIZE_OF_ROUTE_ERROR;
    BOOL packetWasRouted = TRUE;
    unsigned short srcRtFlags;
    NodeAddress ipDest;

    unsigned char salvage = 0;

    IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(droppedMsg);
    int ipHdrLen = IpHeaderSize(ipHdr);

    if (DEBUG_ERROR)
    {
        printf("Node %u sending route error to Node %u "
            "for broken next hop %u\n",
            node->nodeId, ipHdr->ip_src, nextHop);
    }

    ipDest = ipHdr->ip_src;

    if (ipHdr->ip_p == IPPROTO_DSR)
    {
        unsigned char* dsrPktPtr = (unsigned char *) ipHdr + ipHdrLen;
        unsigned char* optPtr = dsrPktPtr + DSR_SIZE_OF_HEADER;

        if (*optPtr == DSR_SRC_ROUTE)
        {
            memcpy(&srcRtFlags, optPtr + DSR_SIZE_OF_TYPE_LEN,
                sizeof(unsigned short));
            salvage = (unsigned char) ((srcRtFlags & DSR_SALVAGE_MASK)
                >> DSR_NUM_BITS_IN_SEG_LEFT);

            if (salvage)
            {
                memcpy(&ipDest, optPtr + DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH,
                    sizeof(NodeAddress));
            }
        }
    }

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0, MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(node, newMsg, dataSize, TRACE_DSR);

    newPktPtr = (unsigned char *) MESSAGE_ReturnPacket(newMsg);

    // Add Dsr Common Header
    DsrAddCommonHeader(
        newPktPtr,
        DSR_NO_MORE_HEADER,
        payloadLen);

    newPktPtr += DSR_SIZE_OF_HEADER;

    // Add route error option with the packet
    DsrAddRouteErrorOption(
        newPktPtr,
        salvage,
        dsr->localAddress,
        ipDest,
        nextHop);

    // Add ip header to it
    NetworkIpAddHeader(
        node,
        newMsg,
        dsr->localAddress,
        ipDest,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_DSR,
        IPDEFTTL);

    // Update statistical variable for number of route error originated
    dsr->stats.numRerrInitiated++;

    // Try to route and send out the packet (This can simply be done by
    // calling the existing router function)
    DsrRouterFunction(
        node,
        newMsg,
        ipDest,
        ANY_IP,
        &packetWasRouted);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrSalvagePacket
// PURPOSE:  Try to salvage a dropped packet, i.e.. try to send the packet
//           through an alternate route if exists
// ARGUMENTS: node, The node sending route request
//            dsr,  Dsr internal structure
//            droppedMsg, the packet dropped by MAC
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrSalvagePacket(
    Node* node,
    DsrData* dsr,
    const Message* droppedMsg)
{
    IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(droppedMsg);
    int ipHdrLen = IpHeaderSize(ipHdr);

    // Check if an alternate address exists for the destination
    DsrRouteEntry* rtEntry = DsrCheckRouteCache(ipHdr->ip_dst,
            &dsr->routeCache);

    if (rtEntry)
    {
        // An alternate address exists for the destination
        // Now if the packet contains dsr source route option
        // then check if the packet is eligible for packet salvaging
        // if there is no source route option, this can happen if previously
        // the destination was one hop away then always try for salvaging
        // the packet
        unsigned char* dsrPktPtr = (unsigned char *) ipHdr + ipHdrLen;

        if (ipHdr->ip_p == IPPROTO_DSR
            && *(dsrPktPtr + DSR_SIZE_OF_HEADER) == DSR_SRC_ROUTE)
        {
            // Need to re-construct the packet previous source route option
            // should be removed and new salvage value should be added
            unsigned short srcRtFlags = 0;
            unsigned char* optPtr = dsrPktPtr + DSR_SIZE_OF_HEADER;
            int salvage = 0;

            memcpy(&srcRtFlags, optPtr + DSR_SIZE_OF_TYPE_LEN,
                sizeof(unsigned short));
            salvage = (srcRtFlags & DSR_SALVAGE_MASK)
                >> DSR_NUM_BITS_IN_SEG_LEFT;

            if (salvage < DSR_MAX_SALVAGE_COUNT)
            {
                // The packet is subject to do packet salvaging
                // Create a new packet with new source route and send out
                // the packet
                Message* newMsg = MESSAGE_Duplicate(node, droppedMsg);
                unsigned char oldNextHdr = *dsrPktPtr;
                unsigned short oldPayloadLen;
                unsigned char protocol = ipHdr->ip_p;

                int sizeOfSrcRouteOption = *(optPtr + 1)
                    + DSR_SIZE_OF_TYPE_LEN;

                memcpy(&oldPayloadLen, dsrPktPtr + sizeof(unsigned short),
                    sizeof(unsigned short));

                MESSAGE_RemoveHeader(node, newMsg, ipHdrLen, TRACE_IP);

                // check if there are other DSR options in the packet in
                // addition to SrcRouteOption

                if (oldPayloadLen > sizeOfSrcRouteOption)
                {
                    // There are more dsr options with the packet so
                    // need to add dsr header

                    DsrShrinkHeader(node, newMsg, sizeOfSrcRouteOption);

                    DsrAddCommonHeader(
                        (unsigned char *) MESSAGE_ReturnPacket(newMsg),
                        oldNextHdr,
                        (unsigned short) (oldPayloadLen
                            - sizeOfSrcRouteOption));
                }
                else
                {
                    // Only data packet now
                    protocol = oldNextHdr;
                    MESSAGE_RemoveHeader(node,
                                         newMsg,
                                         sizeOfSrcRouteOption
                                            + DSR_SIZE_OF_HEADER,
                                         TRACE_DSR);
                }

                MESSAGE_AddHeader(node, newMsg, sizeof(IpHeaderType), TRACE_IP);

                memcpy(MESSAGE_ReturnPacket(newMsg), MESSAGE_ReturnPacket(droppedMsg),
                    ipHdrLen);

                ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(newMsg);

                IpHeaderSetIpLength(&(ipHdr->ip_v_hl_tos_len),
                    MESSAGE_ReturnPacketSize(newMsg));

                ipHdr->ip_ttl = (unsigned char) rtEntry->hopCount;
                ipHdr->ip_p = protocol;

                // Update statistics for number of salvage
                dsr->stats.numSalvagedPackets++;

                DsrTransmitDataWithSrcRoute(
                    node,
                    dsr,
                    newMsg,
                    ipHdr->ip_dst,
                    rtEntry,
                    ++salvage);
            }
        }
        else
        {
            // Update statistics for number of salvage
            dsr->stats.numSalvagedPackets++;

            // Packet doesn't contain any dsr source route option
            DsrTransmitDataWithSrcRoute(
                node,
                dsr,
                MESSAGE_Duplicate(node, droppedMsg),
                ipHdr->ip_dst,
                rtEntry,
                1); // First time the route is going to be salvaged.
        }
    }
    else
    {
        // No alternate route exists for the destination so
        // packet salvaging is not possible
        dsr->stats.numDataDroppedRexmtTO++;
    }
}

//-------------------------------------------------------------------------
// FUNCTION: DsrStoreForwardDirectionalRoutes
// PURPOSE:  Add forward route from source route to routing table
// ARGUMENTS: source route, number of nodes included in the source route,
//            previous hop address,
//            whether this nodes own address is included in the path or not,
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrStoreForwardDirectionalRoutes(
    Node* node,
    DsrData* dsr,
    unsigned char* path,
    int   numNodes,
    NodeAddress prevHop,
    BOOL isSelfIncluded)
{
    unsigned char* addrPtr = path;
    int numNodesInCurrentPath = numNodes;
    BOOL allocatedNewPath = FALSE;

    if (DEBUG_ROUTE_CACHE)
    {
        int i = 0;
        NodeAddress address;
        printf("Node id = %u\n",node->nodeId);
        printf("Previous Hop = %u\n", prevHop);
        printf("Bool is Included = %u\n",isSelfIncluded);
        for (i = 0; i < numNodes; i++)
        {
            memcpy(&address, &path[i * sizeof(NodeAddress)],
                sizeof(NodeAddress));
            printf("%i th addr = %u\n",i,address);
        }

    }

    if (isSelfIncluded)
    {
        // Own address is included in path so
        // that should be the beginnings
        NodeAddress addrInPath = 0;
        int i = 0;
        for (i = 0; i < numNodes; i++)
        {
            memcpy(&addrInPath, addrPtr, sizeof(NodeAddress));
            numNodesInCurrentPath--;
            addrPtr += sizeof(NodeAddress);

            if (addrInPath == dsr->localAddress)
            {
                // Got the position where this node is included
                break;
            }
        }
        ERROR_Assert(addrInPath == dsr->localAddress,
            "Self address is not included in path");
    }
    else
    {
        // Own address is not included in path so
        // search for the previous hop address and previous hop is
        // going to be the immediate next hop

        NodeAddress addrInPath = 0;
        int i = 0;
        for (i = 0; i < numNodes; i++)
        {
            memcpy(&addrInPath, addrPtr, sizeof(NodeAddress));

            if (addrInPath == prevHop)
            {
                // Got the position where previousHopIsIncluded
                // One hop is required to get to previous hop
                break;
            }
            numNodesInCurrentPath--;
            addrPtr += sizeof(NodeAddress);
        }

        if (addrInPath != prevHop && prevHop != 0)
        {
            numNodesInCurrentPath = numNodes + 1;
            addrPtr = (unsigned char*)MEM_malloc((numNodes + 1) * sizeof(NodeAddress));
            allocatedNewPath = TRUE;
            memcpy(addrPtr + sizeof(NodeAddress), path,
                numNodes * sizeof(NodeAddress));
            memcpy(addrPtr, &prevHop, sizeof(NodeAddress));
            path = addrPtr;
        }
    }

    while (numNodesInCurrentPath)
    {
        int pathLength = (numNodesInCurrentPath) * sizeof(NodeAddress);

        if (DEBUG_ROUTE_CACHE)
        {
            int i = 0;
            NodeAddress address;
            printf("Adding Route = %u\n",addrPtr[(numNodesInCurrentPath-1)
                * sizeof(NodeAddress)]);
            for (i = 0; i < numNodesInCurrentPath; i++)
            {
                memcpy(&address, &addrPtr[i * sizeof(NodeAddress)],
                    sizeof(NodeAddress));
                printf("%i th addr = %u\n",i,address);
            }
        }

        // Do not store any route information which can not be accommodated in a DSR
        // packet.

        if (pathLength <= (UCHAR_MAX -
            DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH_TYPE_LEN))
        {

            DsrInsertRoute(
                node,
                &dsr->routeCache,
                (unsigned char *) addrPtr,
                pathLength);
        }

        numNodesInCurrentPath--;
    }

    if (allocatedNewPath)
    {
        MEM_free(path);
    }
}


//-------------------------------------------------------------------------
// FUNCTION: DsrExtractUsableRoutesFromSrcRoute
// PURPOSE:  Collect all usable routes from a source route and add them to
//           its own routing table
// ARGUMENTS: source route, number of nodes included in the source route,
//            previous hop address,
//            whether this nodes own address is included in the path or not,
//            whether routes to be collected only in forward direction or in
//            backward direction or in both direction
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrExtractUsableRoutesFromSrcRoute(
    Node* node,
    unsigned char* path,
    int numNodes,
    NodeAddress prevHop,
    DsrPathDirection direction)
{
    int i = 0;
    BOOL isSelfIncluded = FALSE;
    DsrData* dsr = (DsrData *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    for (i = 0; i < numNodes; i++)
    {
        NodeAddress thisAddr;
        memcpy(&thisAddr, &path[i * sizeof(NodeAddress)],
            sizeof(NodeAddress));

        if (thisAddr == dsr->localAddress)
        {
            isSelfIncluded = TRUE;
            break;
        }
    }

    if (direction == DSR_FORWARD_DIRECTION)
    {
        if (DEBUG_ROUTE_CACHE)
        {
           printf(" Storing Forward Direction Route.\n");
        }

        DsrStoreForwardDirectionalRoutes(
            node,
            dsr,
            path,
            numNodes,
            prevHop,
            isSelfIncluded);
    }
    else
    {
        unsigned char* reversePath = (unsigned char*) MEM_malloc((numNodes)
                                         * sizeof(NodeAddress));
        int i = 0;

        for (i = 0; i < numNodes; i++)
        {
            memcpy(reversePath + (i * sizeof(NodeAddress)), path
                + ((numNodes - i - 1) * sizeof(NodeAddress)),
                sizeof(NodeAddress));
        }
        if (DEBUG_ROUTE_CACHE)
        {
           printf(" Storing Backward Direction Route.\n");
        }

        DsrStoreForwardDirectionalRoutes(
            node,
            dsr,
            reversePath,
            numNodes,
            prevHop,
            isSelfIncluded);

        MEM_free(reversePath);

        if (direction == DSR_BOTH_DIRECTION)
        {
            if (DEBUG_ROUTE_CACHE)
            {
               printf(" Storing Forward Direction Route.\n");
            }

            DsrStoreForwardDirectionalRoutes(
                node,
                dsr,
                path,
                numNodes,
                prevHop,
                isSelfIncluded);
        }
    }

    MEM_free(path);
}


//--------------------------------------------------------------------------
// FUNCTION: DsrCheckIfOptionExist
// PURPOSE:  Checking if a particular option exists in the dsr packet
// ARGUMENTS: The received message, pointer to the first option and option
//            size
// RETURN:   None
//--------------------------------------------------------------------------

static
BOOL DsrCheckIfOptionExist(
    unsigned char* optPtr,
    unsigned short sizeOfOption,
    DsrPacketType optType)
{
    while (sizeOfOption)
    {
        if (*optPtr == optType)
        {
            return TRUE;
        }
        else
        {
            sizeOfOption = (unsigned short) (sizeOfOption - (*(optPtr + 1)
                                + DSR_SIZE_OF_TYPE_LEN));
            optPtr += (unsigned short) (*(optPtr + 1)
                                + DSR_SIZE_OF_TYPE_LEN);
        }
    }
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION: DsrHandleRouteError
// PURPOSE:  Processing route error option of a dsr packet
// ARGUMENTS: The received message, pointer to route error option
// RETURN:   None
//--------------------------------------------------------------------------

static
void DsrHandleRouteError(
    Node* node,
    unsigned char* optPtr)
{
    // Need to delete links
    NodeAddress unreachableNodeAddress;
    NodeAddress errSourceAddress;
    NodeAddress errDestAddress;
    DsrData* dsr = (DsrData *)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    memcpy(&errSourceAddress, optPtr + DSR_SIZE_OF_TYPE_LEN + sizeof(short),
            sizeof(NodeAddress));

    memcpy(&errDestAddress, optPtr + DSR_SIZE_OF_TYPE_LEN
        + sizeof(unsigned int) + sizeof(short), sizeof(NodeAddress));

    memcpy(&unreachableNodeAddress, optPtr
        + DSR_SIZE_OF_ROUTE_ERROR_WITHOUT_PATH, sizeof(NodeAddress));

    // Update statistical variable for number of route error received
    dsr->stats.numRerrRecved++;

    if (errDestAddress == dsr->localAddress)
    {
        // update statistical variable for number of error received as source
        dsr->stats.numRerrRecvedAsSource++;
    }

    // Need to delete link with <errSourceAddress, unreachableNodeAddress>
    if (DEBUG_ERROR)
    {
        printf("Node %u received route error and deleting link\n"
            "%u --- %u\n",
            node->nodeId,
            errSourceAddress,
            unreachableNodeAddress);
    }

    DsrDeleteRouteByLink(
        node,
        &dsr->routeCache,
        errSourceAddress,
        unreachableNodeAddress);
}


//-------------------------------------------------------------------------
// FUNCTION: DsrHandlePacketDrop
// PURPOSE:  Handling packet drop notification from mac, try to retransmit
//           and salvage packet
// ARGUMENTS: node, The node sending route request
//            dsr,  Dsr internal structure
//            droppedMsg, the packet dropped by MAC
// RETURN:   None
//-------------------------------------------------------------------------

static
void DsrHandlePacketDrop(
    Node* node,
    const Message* msg,
    NodeAddress nextHopAddress)
{
    DsrData* dsr = (DsrData *)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);
    DsrRexmtBufferEntry* rexmtBufEntry = NULL;
    IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
    // Check if the packet is subject to retransmission


    if (DEBUG_MAINTENANCE_BUFFER)
    {
        printf("Node %u searching an entry in rexmt buffer\n", node->nodeId);
    }

    rexmtBufEntry = DsrSearchRexmtBuffer(
                        &dsr->rexmtBuffer,
                        ipHdr->ip_dst,
                        ipHdr->ip_src,
                        nextHopAddress,
                        (unsigned short) ipHdr->ip_id);

    if (DEBUG_MAINTENANCE_BUFFER)
    {
        if (rexmtBufEntry && (rexmtBufEntry->count >= DSR_MAX_MAIN_TREXMT))
        {
            printf("Got the entry in rexmt buf. but count exceed\n");
        }
    }

    if (rexmtBufEntry && rexmtBufEntry->count < DSR_MAX_MAIN_TREXMT)
    {
        if (DEBUG_ERROR)
        {
            char time[200];
            TIME_PrintClockInSecond(node->getNodeTime(), time);
            printf("Node %u at %s\n", node->nodeId, time);
            printf("Trying to retransmit, retransmission count %u\n",
                rexmtBufEntry->count);
        }

        // Increment retransmit counter
        rexmtBufEntry->count++;

        // update time stamp of the message
        rexmtBufEntry->timeStamp = node->getNodeTime();


        if (DEBUG_TRACE)
        {
            IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
            int ipHdrLen = IpHeaderSize(ipHdr);

            if (ipHdr->ip_p == IPPROTO_DSR)
            {
                unsigned char* dsrPktPtr = ((unsigned char *) ipHdr)
                                                + ipHdrLen;
                DsrTrace(
                    node,
                    dsr,
                    dsrPktPtr,
                    "Send",
                    ipHdr->ip_dst,
                    ipHdr->ip_src);
            }
        }

        // Retransmit the packet
        NetworkIpSendPacketToMacLayer(
            node,
            MESSAGE_Duplicate(node, msg),
            DEFAULT_INTERFACE,
            rexmtBufEntry->nextHop);
    }
    else
    {
        // Can't retransmit the packet and following steps are to be taken
        // 1. Delete the routes currently using the next hop which has been
        //    detected "not connected"
        // 2. Send route error
        // 3. Try to salvage packet if possible
        // 4. If the packet is in retransmit buffer then delete it

        // Delete routes using the link
        if (DEBUG_ERROR)
        {
            printf("Can't retransmit so drop packet and consider"
                " it as broken link\n");
        }

        DsrDeleteRouteByLink(
            node,
            &dsr->routeCache,
            dsr->localAddress,
            nextHopAddress);

        // don't need to send route error if it originated the packet
        if (ipHdr->ip_src != dsr->localAddress)
        {
            DsrSendRERR(
                node,
                dsr,
                msg,
                nextHopAddress);
        }

        // Try to salvage packet if possible
        DsrSalvagePacket(node, dsr, msg);

        // Delete the packet from retransmit buffer
        if (rexmtBufEntry)
        {
            DsrDeleteRexmtBufferByEntry(
                node,
                &dsr->rexmtBuffer,
                rexmtBufEntry);
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION: DsrHandlePacketWithSrcRoute
// PURPOSE:  Processing source route option of a dsr packet
// ARGUMENTS: The received message, pointer to source route option
// RETURN:   None
//--------------------------------------------------------------------------

static
void DsrHandlePacketWithSrcRoute(
    Node* node,
    Message* msg,
    NodeAddress prevHop,
    BOOL isOverheard)
{
    DsrData* dsr = (DsrData *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

    if (((IpHeaderGetIpMoreFrag(ipHdr->ipFragment)) == 1) ||
        ((IpHeaderGetIpFragOffset(ipHdr->ipFragment)) != 0))
    {
        ERROR_ReportError("DSR doesn't support IP Fragmentation\n");
        return;
    }

    unsigned int ipHdrLen = (unsigned int) IpHeaderSize(ipHdr);
    unsigned char* dsrPktPtr = (unsigned char *) ipHdr + ipHdrLen;
    unsigned char* ptrToSrcRtOpt = dsrPktPtr + DSR_SIZE_OF_HEADER;
    unsigned char* ptrToSrcRt = ptrToSrcRtOpt
                                    + DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH;

    unsigned int sizeOfPath = SizeOfPathInSourceRoute(ptrToSrcRtOpt);
    unsigned int numNodes = sizeOfPath / sizeof(NodeAddress);
    unsigned int segsLeft = 0;
    unsigned int salvage = 0;
    unsigned short fLReservedSalvageSegsLeft;
    NodeAddress nextHop;
    unsigned short sizeOfOption = 0;
    DsrPathDirection direction = DSR_BOTH_DIRECTION;
    unsigned char* extractedSrcRt = NULL;
    unsigned int   numNodesInExtractedSrcRt = 0;
    unsigned char nextHeader = *dsrPktPtr;
    BOOL isAReply = FALSE;


    if (DEBUG_DISCOVERY)
    {
        char time[100];
        TIME_PrintClockInSecond(node->getNodeTime(), time);
        printf("Node %u received Source route at %s\n", node->nodeId, time);
    }

    memcpy(&sizeOfOption, (dsrPktPtr + sizeof(unsigned short))
        , sizeof(unsigned short));

    memcpy(&fLReservedSalvageSegsLeft, ptrToSrcRtOpt
        + DSR_SIZE_OF_TYPE_LEN, sizeof(unsigned short));

    segsLeft = fLReservedSalvageSegsLeft & DSR_SEGS_LEFT_MASK;
    salvage = (fLReservedSalvageSegsLeft & DSR_SALVAGE_MASK)
        >> DSR_NUM_BITS_IN_SEG_LEFT;


    // If packet contains route reply option with source route then,
    // only the path already traversed should be stored in the route cache

    isAReply = DsrCheckIfOptionExist(ptrToSrcRtOpt, sizeOfOption,
        DSR_ROUTE_REPLY);

    if (DEBUG_ROUTE_CACHE)
    {
        printf(" Calling From Processing Source Route.\n");
    }

    if (!isAReply)
    {
        if (!salvage)
        {
            // path will include source address and destination address
            numNodesInExtractedSrcRt = numNodes + 2;
            extractedSrcRt = (unsigned char*)MEM_malloc(numNodesInExtractedSrcRt *
                sizeof(NodeAddress));
            memcpy(extractedSrcRt, &ipHdr->ip_src, sizeof(NodeAddress));
            memcpy(extractedSrcRt + sizeof(NodeAddress), ptrToSrcRt, sizeOfPath);
            memcpy(extractedSrcRt + sizeof(NodeAddress) + sizeOfPath,
                &ipHdr->ip_dst, sizeof(NodeAddress));
        }
        else
        {
            // path will include only destination address
            numNodesInExtractedSrcRt = numNodes + 1;
            extractedSrcRt = (unsigned char*)MEM_malloc(numNodesInExtractedSrcRt *
                sizeof(NodeAddress));
            memcpy(extractedSrcRt, ptrToSrcRt, sizeOfPath);
            memcpy(extractedSrcRt + sizeOfPath, &ipHdr->ip_dst,
                sizeof(NodeAddress));
        }

        DsrExtractUsableRoutesFromSrcRoute(
            node,
            extractedSrcRt,
            numNodesInExtractedSrcRt,
            prevHop,
            direction);
    }


    if (isOverheard)
    {
        return;
    }


    ERROR_Assert(segsLeft <= numNodes,
        "seg left can't be greater than number of nodes in src route");

    ERROR_Assert(segsLeft,
        "seg left 0 should be passed to destination node");

    segsLeft--;

    // copy next hop = address[n - segsLeft]
    if (segsLeft)
    {
        memcpy(&nextHop, ptrToSrcRt +
            (numNodes - segsLeft) * sizeof(NodeAddress),
            sizeof(NodeAddress));

        fLReservedSalvageSegsLeft--;

        memcpy(ptrToSrcRtOpt + DSR_SIZE_OF_TYPE_LEN,
            &fLReservedSalvageSegsLeft, sizeof(unsigned short));

        if (DEBUG_DISCOVERY)
        {
            printf("\tSegs left is non zero so forwarding "
                "the packet as it is\n");
            printf("\tTo next hop %u\n", nextHop);
        }
    }
    else
    {
        // Need to remove the source route option from dsr packet
        IpHeaderType origIpHeader;
        unsigned char* origDsrHeader = NULL;

        unsigned short oldPayloadLen = 0;
        unsigned short newPayloadLen = 0;

        unsigned int srcRtOptLen =
            DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH + sizeOfPath;

        nextHop = ipHdr->ip_dst;

        if (DEBUG_DISCOVERY)
        {
            printf("\tSegs left is zero so removing source route option "
                "and forwarding packet\n");
            printf("\tTo next hop %u\n", nextHop);
        }

        memcpy(&origIpHeader, MESSAGE_ReturnPacket(msg),
            sizeof(IpHeaderType));

        MESSAGE_RemoveHeader(node, msg, sizeof(IpHeaderType), TRACE_IP);

        memcpy(&oldPayloadLen, (unsigned char*)dsrPktPtr + sizeof(unsigned short),
            sizeof(unsigned short));

        origDsrHeader = (unsigned char*)MEM_malloc(oldPayloadLen + DSR_SIZE_OF_HEADER);

        memcpy(origDsrHeader, MESSAGE_ReturnPacket(msg),
            oldPayloadLen + DSR_SIZE_OF_HEADER);

        newPayloadLen = (unsigned short) (oldPayloadLen - srcRtOptLen);

        if (newPayloadLen)
        {
            // There are other optional fields other than source route
            // option so need to add those optional fields along with
            // dsr common header
            unsigned char* pktPtr = NULL;

            DsrShrinkHeader(node, msg, srcRtOptLen);

            pktPtr = (unsigned char *) MESSAGE_ReturnPacket(msg);

            // Add Dsr Common Header
            DsrAddCommonHeader(
                pktPtr,
                *origDsrHeader,
                newPayloadLen);

            pktPtr += DSR_SIZE_OF_HEADER;

            memcpy(pktPtr, origDsrHeader + srcRtOptLen + DSR_SIZE_OF_HEADER,
                newPayloadLen);


            // Now add ip header
            IpHeaderSetIpLength(&(origIpHeader.ip_v_hl_tos_len),
                (IpHeaderGetIpLength(origIpHeader.ip_v_hl_tos_len)
                - srcRtOptLen));
            MESSAGE_AddHeader(node, msg, sizeof(IpHeaderType), TRACE_IP);
            memcpy(MESSAGE_ReturnPacket(msg), &origIpHeader,
                sizeof(IpHeaderType));
        }
        else
        {
            // No other optional fields so need to remove Dsr header
            // completely

            MESSAGE_RemoveHeader(node,
                                 msg,
                                 oldPayloadLen + DSR_SIZE_OF_HEADER,
                                 TRACE_DSR);

            MESSAGE_AddHeader(node, msg, sizeof(IpHeaderType), TRACE_IP);
            memcpy(&origIpHeader.ip_p, origDsrHeader, sizeof(char));
            IpHeaderSetIpLength(&(origIpHeader.ip_v_hl_tos_len),
                (IpHeaderGetIpLength(origIpHeader.ip_v_hl_tos_len) -
                (srcRtOptLen + DSR_SIZE_OF_HEADER)));
            memcpy(MESSAGE_ReturnPacket(msg), &origIpHeader,
                sizeof(IpHeaderType));
        }
        MEM_free(origDsrHeader);
    }

    if (nextHeader != DSR_NO_MORE_HEADER)
    {
        // update statistical variable for number of data forwarded
        dsr->stats.numDataForwarded++;
    }

    // Insert the packet in retransmit buffer
    if (!isAReply)
    {
        DsrInsertRexmtBuffer(
            node,
            MESSAGE_Duplicate(node, msg),
            nextHop,
            &dsr->rexmtBuffer);
    }

    if (DEBUG_TRACE)
    {
        IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

        if (ipHdr->ip_p == IPPROTO_DSR)
        {
            int ipHdrLen = IpHeaderSize(ipHdr);
            unsigned char* dsrPktPtr = ((unsigned char *) ipHdr) + ipHdrLen;

            DsrTrace(
                node,
                dsr,
                dsrPktPtr,
                "Send",
                ipHdr->ip_dst,
                ipHdr->ip_src);
        }
    }

    // send out the packet towards destination.
    NetworkIpSendPacketToMacLayer(
            node,
            msg,
            DEFAULT_INTERFACE,
            nextHop);
}


//--------------------------------------------------------------------------
// FUNCTION: DsrHandleReply
// PURPOSE:  Processing a received reply
// ARGUMENTS: received packet and pointer to the route reply option
// RETURN:   None
//--------------------------------------------------------------------------

static
void DsrHandleReply(
    Node* node,
    unsigned char* recvRrep,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    NodeAddress prevHop)
{
    DsrData* dsr = (DsrData *)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    unsigned char* path = NULL;

    unsigned int numNodesInPath = SizeOfPathInRouteReply(recvRrep)
        / sizeof(NodeAddress);

    if (DEBUG_DISCOVERY)
    {
        char time[100];
        TIME_PrintClockInSecond(node->getNodeTime(), time);
        printf("Node %u received reply at %s\n", node->nodeId, time);
        printf("\tsource: %u\n"
            "\tdestination: %u\n",
            srcAddr,
            destAddr);
    }

    // extracted source route in route reply will contain
    // initiator, address[i] in the path so actual number of
    // of nodes should be number of nodes in the path + 1
    numNodesInPath++;

    path = (unsigned char*) MEM_malloc(numNodesInPath * sizeof(NodeAddress));

    // copy initiators address at the beginning, which is the destination
    // address recorded in the ip header
    memcpy(path, &destAddr, sizeof(NodeAddress));
    memcpy(path + sizeof(NodeAddress), recvRrep
        + DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH, (numNodesInPath - 1)
        * sizeof(NodeAddress));

    // Update statistical variable for number of route reply received
    dsr->stats.numReplyRecved++;

    if (destAddr == dsr->localAddress)
    {
        // Resent sent table entry for the destination;
        DsrRequestEntry* reqEntry = NULL;
        NodeAddress targetAddress;
        DsrSentEntry* sentEntry;
        memcpy(&targetAddress, recvRrep
            + DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH
            + (numNodesInPath - 2) * sizeof(NodeAddress)
            , sizeof(NodeAddress));

        reqEntry = DsrCheckSentEntry(
                       targetAddress, &dsr->reqTable);

        if (reqEntry)
        {
            sentEntry = (DsrSentEntry *) reqEntry->data;
            sentEntry->count = 0;
        }

        // update statistical variable for number of reply received
        // for which it was source
        dsr->stats.numReplyRecvedAsSource++;
    }
    if (DEBUG_ROUTE_CACHE)
    {
       printf(" Calling From Handle Reply.\n");
    }

    DsrExtractUsableRoutesFromSrcRoute(
        node,
        path,
        numNodesInPath,
        prevHop,
        DSR_BOTH_DIRECTION);

    // This is the destination received a route reply for a route
    // request. So handle the packet accordingly. (sec: 6.1.4 and
    // sec: 6.2.5)
}


//--------------------------------------------------------------------------
// FUNCTION: DsrHandleRequest
// PURPOSE:  Processing route request option of a dsr packet
// ARGUMENTS: The received message, pointer to request option
// RETURN:   None
//--------------------------------------------------------------------------

static
BOOL DsrHandleRequest(
    Node* node,
    Message* recvMsg,
    unsigned char* recvRreq,
    NodeAddress srcAddr,
    int ttl,
    NodeAddress prevHop)
{
    // Function to work with a received request
    DsrData* dsr = (DsrData *)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    NodeAddress targetAddress = 0;
    unsigned short identification = 0;
    unsigned char* path = recvRreq + DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH;
    unsigned int numNodesInSrcRt =
               SizeOfPathInRouteRequest(recvRreq) / sizeof(NodeAddress);

    unsigned char* srcRtInRequest = NULL;

    BOOL isMsgDiscarded = FALSE;

    // update statistical variable for number of request received
    dsr->stats.numRequestRecved++;

    if (DEBUG_ROUTE_CACHE)
    {
        printf(" Calling From Handle Request.\n");
    }

    memcpy(&identification, (recvRreq + sizeof(short)), sizeof(short));

    memcpy(&targetAddress, (recvRreq + sizeof(int)), sizeof(NodeAddress));

    // Add necessary routing information collected from the recorded
    // route in the route request
    srcRtInRequest = (unsigned char*)MEM_malloc((numNodesInSrcRt +1) * sizeof(NodeAddress));

    memcpy(srcRtInRequest, &srcAddr, sizeof(NodeAddress));

    memcpy(srcRtInRequest + sizeof(NodeAddress),path,
        numNodesInSrcRt * sizeof(NodeAddress));

    DsrExtractUsableRoutesFromSrcRoute(
        node,
        srcRtInRequest,
        numNodesInSrcRt + 1,
        prevHop,
        DSR_BOTH_DIRECTION);

    if (DEBUG_DISCOVERY)
    {
        char time[100];
        TIME_PrintClockInSecond(node->getNodeTime(), time);
        printf("Node %u received request at %s\n", node->nodeId, time);
        printf("\tsource: %u\n"
            "\tidentification: %u\n"
            "\ttarget: %u\n",
            srcAddr,
            identification,
            targetAddress);
    }

    // Check if I am the destination
    if (NetworkIpIsMyIP(node, targetAddress))
    {
        if (DEBUG_DISCOVERY)
        {
            printf("\tDestination so initiating Route Reply\n");
        }

        // This is the desired destination node. So need to sent back a reply
        DsrInitiateRREP(node, dsr, recvRreq, srcAddr);

        // Update statistical variable for number of request for this node
        dsr->stats.numRequestRecvedAsDest++;
    }

    // Check for loop, the address of this node exists in the path of the
    // Route Request
    else if (srcAddr == dsr->localAddress
        || DsrCheckIfAddressExists(
               path,
               numNodesInSrcRt,
               dsr->localAddress))

    {
        if (DEBUG_DISCOVERY)
        {
            printf("\tLoop in the packet so discarding it\n");
        }

        // Loop in the Route Request so discard it
        isMsgDiscarded = TRUE;

        // update statistical variable for number request discarded for loop
        dsr->stats.numRequestInLoop++;
    }
    else if (!DsrCheckSeenEntry(
                  srcAddr,
                  identification,
                  targetAddress,
                  &dsr->reqTable)) // Process Only if this is not
                                   // a duplicate
    {
        BOOL catchErr;
        DsrRouteEntry* rtEntry = NULL;
        BOOL isSendingCachedReply = FALSE;

        if (DEBUG_DISCOVERY)
        {
            printf("\tNot in Seen table so adding there\n");
        }

        // Add an entry to Route  request table.
        catchErr = DsrAddSeenEntry(
                       node,
                       srcAddr,
                       identification,
                       targetAddress,
                       &dsr->reqTable);

        ERROR_Assert(catchErr, "Already exist Request seen entry\n");

        rtEntry = DsrCheckRouteCache(targetAddress,
            &dsr->routeCache);

        if (rtEntry)
        {
            if (DEBUG_DISCOVERY)
            {
                printf("\tKnow a route so send cached reply\n");
            }

            // Has a route to the destination in the route cache
            DsrInitiateRREPByIn(
                node,
                dsr,
                recvRreq,
                rtEntry,
                srcAddr,
                &isSendingCachedReply);
        }

        if (!isSendingCachedReply)
        {
            //as the ttl has not been decreased as yet at the ip-layer --
            //the comparision needs to be done with 1
            //Also, the ttl would need to be decreased here before sending 
            //to mac layer.
            if (ttl > 1)
            {
                if (DEBUG_DISCOVERY)
                {
                    printf("\tRelaying the request\n");
                }

                DsrRelayRREQ(
                    node,
                    dsr,
                    recvMsg,
                    recvRreq,
                    srcAddr,
                    ttl);
            }
            else
            {
                // update statistical variable for number of request ttl
                // expired
                if (DEBUG_DISCOVERY)
                {
                    printf("\tttl expired\n");
                }

                dsr->stats.numRequestTtlExpired++;
                isMsgDiscarded = TRUE;
            }
        }
    }
    else
    {
        // update statistical variable for number of duplicate request
        // received

        if (DEBUG_DISCOVERY)
        {
            printf("\tAlready processed the request\n");
        }

        dsr->stats.numRequestDuplicate++;
        isMsgDiscarded = TRUE;
    }

    return isMsgDiscarded;
}


//--------------------------------------------------------------------------
// FUNCTION: DsrHandleOptions
// PURPOSE:  Processing different Dsr options included in a dsr packet.
// ARGUMENTS: The received message, pointer to the first option and total
//            size of options included with dsr header
// RETURN:   None
//--------------------------------------------------------------------------

static
void DsrHandleOptions(
    Node* node,
    Message* recvMsg,
    unsigned char* options,
    unsigned short optionSize,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    int ttl,
    NodeAddress prevHop,
    BOOL isOverheard)
{
    while (optionSize)
    {
        if (*options == DSR_PAD_1)
        {
            // For pad1 option no action is to be taken except skipping
            // one byte
            options++;
            optionSize--;
        }
        else if (*options == DSR_PAD_N)
        {
            // For padN option no action is to be taken except skipping
            // N bytes
            optionSize = (unsigned short) (optionSize - (*(options + 1)
                                                + DSR_SIZE_OF_TYPE_LEN));
            options += (*(options + 1) + DSR_SIZE_OF_TYPE_LEN);
        }
        else if (*options == DSR_ROUTE_REQUEST)
        {
            // Need to process route request option. If the packet is to be
            // discarded then set option size to zero.

            BOOL isPktDiscarded = FALSE;

            isPktDiscarded = DsrHandleRequest(
                                 node,
                                 recvMsg,
                                 options,
                                 srcAddr,
                                 ttl,
                                 prevHop);

            if (!isPktDiscarded)
            {
                optionSize = (unsigned short) (optionSize - (*(options + 1)
                                                    + DSR_SIZE_OF_TYPE_LEN));
                options += (*(options + 1) + DSR_SIZE_OF_TYPE_LEN);
            }
            else
            {
                optionSize = 0;
            }
        }
        else if (*options == DSR_ROUTE_REPLY)
        {
            // Need to process route reply option. The function will only
            // add usable route if there is any with the reply

            DsrHandleReply(
                node,
                options,
                srcAddr,
                destAddr,
                prevHop);

            optionSize = (unsigned short) (optionSize - (*(options + 1)
                              + DSR_SIZE_OF_TYPE_LEN));
            options += (*(options + 1) + DSR_SIZE_OF_TYPE_LEN);
        }
        else if (*options == DSR_ROUTE_ERROR)
        {
            DsrHandleRouteError(node, options);
            optionSize = (unsigned short) (optionSize - (*(options + 1)
                              + DSR_SIZE_OF_TYPE_LEN));
            options += (*(options + 1) + DSR_SIZE_OF_TYPE_LEN);
        }
        else if (*options == DSR_ACK_REQUEST)
        {
            ERROR_Assert(FALSE, "This option is not yet implemented\n");
        }
        else if (*options == DSR_ACK)
        {
            ERROR_Assert(FALSE, "This option is not yet implemented\n");
        }
        else if (*options == DSR_SRC_ROUTE)
        {
            Message* duplicateMsg = NULL;

            // If the packet is not an overheard packet then the node
            // should forward the packet keeping a copy of the original
            // message. and the node will proceed with processing the
            // original message for further optional fields

            if (!isOverheard)
            {
                duplicateMsg = MESSAGE_Duplicate(node, recvMsg);
            }
            else
            {
                duplicateMsg = recvMsg;
            }

            DsrHandlePacketWithSrcRoute(
                node,
                duplicateMsg,
                prevHop,
                isOverheard);

            optionSize = (unsigned short) (optionSize - (*(options + 1)
                + DSR_SIZE_OF_TYPE_LEN));
            options += (*(options + 1)
                + (unsigned char) DSR_SIZE_OF_TYPE_LEN);
        }
        else
        {
            ERROR_Assert(!optionSize, "Unknown Dsr packet type\n");
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION: DsrHandleProtocolPacketWithIPHeader
// PURPOSE:  Process packets as an intermediate nodes
// ARGUMENTS: The received message and previous hop from which the packet
//            has been received
// RETURN:   None
//--------------------------------------------------------------------------

static
void DsrHandleProtocolPacketWithIPHeader(
    Node* node,
    DsrData* dsr,
    Message* msg,
    NodeAddress previousHop)
{
    // Reply and source route packets can get into this function.
    IpHeaderType* ipHdr = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
    int ipHdrLen = IpHeaderSize(ipHdr);
    unsigned char* dsrPktPtr = (unsigned char *) (((char *) ipHdr)
        + ipHdrLen);
    unsigned char* optPtr = dsrPktPtr + DSR_SIZE_OF_HEADER;
    unsigned short payloadLen = 0;

    memcpy(&payloadLen, dsrPktPtr + sizeof(unsigned short),
        sizeof(unsigned short));

    ERROR_Assert(ipHdr->ip_p == IPPROTO_DSR,
        "protocol must be dsr\n");

    if (DEBUG_TRACE)
    {
        DsrTrace(
            node,
            dsr,
            dsrPktPtr,
            "Receive",
            ipHdr->ip_dst,
            ipHdr->ip_src);
    }

    // Need to process dsr specific information
    DsrHandleOptions(
        node,
        msg,
        optPtr,
        payloadLen,
        ipHdr->ip_src,
        ipHdr->ip_dst,
        ipHdr->ip_ttl,
        previousHop,
        FALSE);

    MESSAGE_Free(node, msg);
}


//--------------------------------------------------------------------------
// FUNCTION: DsrRouterFunction
// PURPOSE:  Determine routing action to take for a the given data packet
// ARGUMENTS: Received packet, destination of the packet, last hop address
// RETURN:   PacketWasRouted variable is TRUE if no further handling of
//           this packet by IP is necessary. Internally this variable is also
//           used to check route error packet. As route error transmission
//           function called this router function it will make packetWasRouted
//           as TRUE to mark it as a route error packet. It will be necessary
//           to identify route error packet for collecting statistics.
// ASSUMPTION: None
//--------------------------------------------------------------------------

void DsrRouterFunction(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL* packetWasRouted)
{
    DsrData* dsr = (DsrData *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    IpHeaderType* ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

    BOOL isErrorPacket = *packetWasRouted;

    if (NetworkIpIsMyIP(node, ipHeader->ip_src) &&
        previousHopAddress == ANY_IP)
    {
        // Packet is originated in this node
        if (NetworkIpIsMyIP(node, ipHeader->ip_dst))
        {
            // Can't route a packet destined to itself
            *packetWasRouted = FALSE;
            return;
        }
        else
        {
            DsrRouteEntry *rtPtr = NULL;

            // This is a packet originated in this node so
            // need to route the packet.
            *packetWasRouted = TRUE;

            rtPtr = DsrCheckRouteCache(destAddr, &dsr->routeCache);

            if (rtPtr)
            {
                if (ipHeader->ip_p != IPPROTO_DSR && isErrorPacket != TRUE)
                {
                    // Update statistical variable for number of data
                    // transmitted
                    dsr->stats.numDataInitiated++;
                }

                // this is source and route to destination is known
                DsrTransmitDataWithSrcRoute(
                    node,
                    dsr,
                    msg,
                    destAddr,
                    rtPtr,
                    0);
            }
            else
            {
                DsrRequestEntry* reqEntry = DsrCheckSentEntry(
                    destAddr, &dsr->reqTable);

                if (!reqEntry ||
                    (node->getNodeTime() > reqEntry->timeStamp
                      + ((DsrSentEntry *) reqEntry->data)->backoffInterval))
                {

                    // Route to destination is not known. No request is sent
                    // for the destination.Initiate route discovery process.

                    DsrInitiateRREQ(node, dsr, destAddr, reqEntry);
                }

                // insert the msg into buffer
                DsrInsertBuffer(
                    node,
                    msg,
                    destAddr,
                    isErrorPacket,
                    &dsr->msgBuffer);
            }
        }
    }
    else
    {
        // Packet is not originated in this node
        if (NetworkIpIsMyIP(node, ipHeader->ip_dst))
        {
            // source route option should be eliminated in the previous node.
            // if the packet contains only source route option then the
            // packet will not have any dsr header in it.

            // If it contains any other option that will be handled by
            // Handle protocol packet.

            // So this portion of the code will do nothing

            // update statistics for number of data packet received. If IP
            // protocol is DSR it might be a Route Reply or a Route Error.

            if (ipHeader->ip_p != IPPROTO_DSR)
            {
                dsr->stats.numDataRecved++;
            }

            return;
        }
        else
        {
            // Neither source nor destination. An intermediate node.
            // it should route the packet.
            *packetWasRouted = TRUE;
            DsrHandleProtocolPacketWithIPHeader(
                node,
                dsr,
                msg,
                previousHopAddress);

            return;
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION:  DsrHandleProtocolPacket
// PURPOSE:   Processing a received packet for which this node is the desired
//            destination.
// ARGUMENTS: node, The node received a packet
//            msg,  The packet received
//            srcAddr, Ip header's source address field
//            destAddr, Ip header's destination address field
//            ttl,    Ip header's time to leave field
// RETURN:    None
//--------------------------------------------------------------------------

void
DsrHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    int ttl,
    NodeAddress prevHop)
{
    unsigned char* pktPtr = (unsigned char *) MESSAGE_ReturnPacket(msg);

    unsigned char* optionPtr = pktPtr;
    unsigned short optionSize = 0;

    memcpy(&optionSize, (optionPtr + sizeof(short)), sizeof(unsigned short));

    optionPtr += DSR_SIZE_OF_HEADER;

    if (DEBUG_TRACE)
    {
        DsrData* dsr = (DsrData *)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

        DsrTrace(
            node,
            dsr,
            (unsigned char*) MESSAGE_ReturnPacket(msg),
            "Receive",
            destAddr,
            srcAddr);
    }

    DsrHandleOptions(
        node,
        msg,
        optionPtr,
        optionSize,
        srcAddr,
        destAddr,
        ttl,
        prevHop,
        FALSE);

    // Free the incoming message
    MESSAGE_Free(node, msg);
}


//--------------------------------------------------------------------------
// FUNCTION:  DsrHandleProtocolEvent
// PURPOSE:   Handle different dsr events like self timers
// ARGUMENTS: node, The node received an event
//            msg,  Message containing information about the event
// RETURN:    None
//--------------------------------------------------------------------------

void
DsrHandleProtocolEvent(Node *node, Message *msg)
{
    DsrData* dsr = (DsrData *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);

    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_NETWORK_CheckReplied:
        {
            NodeAddress destAddr;

            memcpy(&destAddr, MESSAGE_ReturnInfo(msg), sizeof(NodeAddress));

            // Check if we get requested route already
            if (!DsrCheckRouteCache(destAddr, &dsr->routeCache))
            {
                if (DsrLookupBuffer(destAddr, &dsr->msgBuffer))
                {
                    // Need to make sure there is at least one packet in the
                    // buffer for the destination.

                    // No route to the dest is known.
                    DsrRequestEntry* reqEntry = DsrCheckSentEntry(
                                                    destAddr,
                                                    &dsr->reqTable);
                    DsrSentEntry* sentEntry = NULL;

                    if (reqEntry)
                    {
                        sentEntry = (DsrSentEntry*)reqEntry->data;

                        if (node->getNodeTime() >=
                            reqEntry->timeStamp + sentEntry->backoffInterval)
                        {
                            // Retry route request
                            DsrRetryRREQ(node, dsr, destAddr, reqEntry);
                        }
                    }
                }
            }

            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_NETWORK_PacketTimeout:
        {
            NodeAddress destAddr = 0;
            unsigned short packetId = 0;
            memcpy(&destAddr, MESSAGE_ReturnInfo(msg),
                sizeof(NodeAddress));
            memcpy(&packetId, MESSAGE_ReturnInfo(msg) +
                sizeof(NodeAddress), sizeof(unsigned short));

            // As soon as there is a route for a destination all packets
            // destined to that node will be sent to mac buffer so don't
            // need to check if any route exists or not

            DsrFindAndDropPacketWithDestAndId(
                node,
                dsr,
                destAddr,
                packetId,
                &dsr->msgBuffer);

            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_NETWORK_DeleteRoute:
        {
            // Check timeout for the route
            DsrDeleteRoute(
                node,
                &dsr->routeCache,
                msg);

            break;
        }

        case MSG_NETWORK_RexmtTimeout:
        {
            // Nothing to be done as sooner or later mac is going to
            // inform about the packet drop, dsr will handle necessary
            // tasks of link breakage then. This event will only be
            // useful if Mac doesn't have an acknowledgement mechanism

            MESSAGE_Free(node, msg);
            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Unknown Dsr event type\n");
            break;
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION: DsrMacLayerStatusHandler
// PURPOSE:  Function to process mac layer feedback about packet transmission.
// ARGUMENTS: next hop address to which transmission failed, the packet whose
//            transmission failed
// RETURN:   None
//--------------------------------------------------------------------------

void
DsrMacLayerStatusHandler(
    Node* node,
    const Message* msg,
    const NodeAddress nextHopAddress,
    const int interfaceIndex)
{
    IpHeaderType* ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
    int ipHdrLen = IpHeaderSize(ipHeader);
    DsrData* dsr = (DsrData*)NetworkIpGetRoutingProtocol(
                       node,
                       ROUTING_PROTOCOL_DSR);

    // Update statistics for broken link
    dsr->stats.numLinkBreaks++;

    if (DEBUG_ERROR)
    {
        printf("Node %u received link failure for next hop %u\n",
            node->nodeId, nextHopAddress);
    }

    if (ipHeader->ip_p == IPPROTO_DSR)
    {
        // The packet contains dsr header
        unsigned char* dsrPktPtr = (unsigned char *) ipHeader + ipHdrLen;
        unsigned char* optPtr = dsrPktPtr + DSR_SIZE_OF_HEADER;
        unsigned short sizeOfOption = 0;

        memcpy(&sizeOfOption,
            (dsrPktPtr + sizeof(unsigned short)),
            sizeof(unsigned short));

        if ((*optPtr == DSR_ROUTE_REQUEST)
            || DsrCheckIfOptionExist(optPtr, sizeOfOption, DSR_ROUTE_REPLY))
        {
            // Don't need to process anything for route request option
            // the node will re initiate a route request in its own logic
            if (DEBUG_ERROR)
            {
                 printf("A drop for route request or reply so do nothing\n");
            }
        }
        else
        {
            if (DEBUG_ERROR)
            {
                printf("A drop dsr options other than route request\n");
            }

            DsrHandlePacketDrop(node, msg, nextHopAddress);
        }
    }
    else
    {
        if (DEBUG_ERROR)
        {
            printf("A drop for a packet without dsr header\n");
        }

        // The packet doesn't contain any dsr header
        DsrHandlePacketDrop(node, msg, nextHopAddress);
    }
}


//--------------------------------------------------------------------------
// FUNCTION: DsrPeekFunction
// PURPOSE:  Processing a overheard packet.
// ARGUMENTS: node, The node initializing Dsr
//            msg, Packet received for overhearing the channel
//            previousHop, The node from which the packet has been received
// RETURN:   None
//--------------------------------------------------------------------------

void
DsrPeekFunction(Node *node, const Message *msg, NodeAddress previousHop)
{
    IpHeaderType* ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
    int ipHdrLen = IpHeaderSize(ipHeader);

    if (ipHeader->ip_p == IPPROTO_DSR)
    {
        unsigned char* dsrPktPtr = (unsigned char *) ipHeader + ipHdrLen;

        // This is dsr control packet
        unsigned char* optPtr = dsrPktPtr + DSR_SIZE_OF_HEADER;
        unsigned short payloadLen = 0;
        memcpy(&payloadLen, dsrPktPtr + sizeof(unsigned short),
            sizeof(unsigned short));

        if (*optPtr == DSR_ROUTE_REQUEST)
        {
            // Process overheard route request
            // Nothing to do as request is sent as
            // broadcast so the will have or already
            // has processed one
            return;
        }
        else
        {
            // Need to process dsr specific information
            Message* duplicateMsg = MESSAGE_Duplicate(node, msg);

            if (DEBUG_TRACE)
            {
                DsrData* dsr = (DsrData *)
                    NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSR);
                DsrTrace(
                    node,
                    dsr,
                    dsrPktPtr,
                    "Receive",
                    ipHeader->ip_dst,
                    ipHeader->ip_src);
            }

            DsrHandleOptions(
                node,
                duplicateMsg,
                optPtr,
                payloadLen,
                ipHeader->ip_src,
                ipHeader->ip_dst,
                ipHeader->ip_ttl,
                previousHop,
                TRUE);

            MESSAGE_Free(node, duplicateMsg);
        }
    }
    else
    {
        // This packet does not contain any dsr information so there
        // is nothing to do
    }
}


//--------------------------------------------------------------------------
// FUNCTION: DsrMacAckHandler
// PURPOSE:  Handling mac layer acknowledgement that it has successfully
//           transmitted one packet
// ARGUMENTS: interface Index, transmitted msg, next hop address to which
//            transmission is successful
// RETURN:   None
//--------------------------------------------------------------------------

static
void DsrMacAckHandler(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    NodeAddress nextHop)
{
    // Delete all packets stored in the retransmit buffer with
    // the same next hop

    DsrData* dsr = (DsrData*)NetworkIpGetRoutingProtocol(
                       node,
                       ROUTING_PROTOCOL_DSR);

    DsrDeleteRexmtBufferByNextHop(
        node,
        &dsr->rexmtBuffer,
        nextHop);
}


//--------------------------------------------------------------------------
// FUNCTION: DsrInit
// PURPOSE:  Initializing Dsr internal variables and structures
//           as well as providing network with a router function
//           a Mac status handler and a promiscuous mode operation
//           function.
// ARGUMENTS: node, The node initializing Dsr
//            dsrPtr, Space to allocate Dsr internal structure
//            nodeInput, Qualnet main configuration file
//            interfaceIndex, The interface where dsr has been assigned
//                             as a routing protocol
// RETURN:   None
//--------------------------------------------------------------------------

void
DsrInit(
    Node *node,
    DsrData **dsrPtr,
    const NodeInput *nodeInput,
    int interfaceIndex)
{
    BOOL retVal = FALSE;
    char buf[MAX_STRING_LENGTH];

    if (MAC_IsWiredNetwork(node, interfaceIndex))
    {
        ERROR_ReportError("DSR can only support wireless interfaces");
    }

    if (node->numberInterfaces > 1)
    {
        ERROR_ReportError("Dsr only supports one interface of node");
    }

    (*dsrPtr) = (DsrData*)MEM_malloc(sizeof(DsrData));

    ERROR_Assert((*dsrPtr) != NULL, "DSR: Cannot alloc memory for DSR "
                                    "struct!\n");

    RANDOM_SetSeed((*dsrPtr)->seed,
                   node->globalSeed,
                   node->nodeId,
                   ROUTING_PROTOCOL_DSR,
                   interfaceIndex);

    (*dsrPtr)->isTimerSet = FALSE;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTING-STATISTICS",
        &retVal,
        buf);

    if ((retVal == FALSE) || (strcmp(buf, "NO") == 0))
    {
        ((*dsrPtr)->statsCollected) = FALSE;
    }
    else if (strcmp(buf, "YES") == 0)
    {
        ((*dsrPtr)->statsCollected) = TRUE;
    }
    else
    {
        ERROR_ReportError("Need YES or NO for ROUTING-STATISTICS");
    }

    // Initialize statistics
    memset(&((*dsrPtr)->stats), 0, sizeof(DsrStats));
    (*dsrPtr)->statsPrinted = FALSE;

    IO_ReadInt(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, 0),
        nodeInput,
        "DSR-BUFFER-MAX-PACKET",
        &retVal,
        &((*dsrPtr)->bufferMaxSizeInPacket));

    if (retVal == FALSE)
    {
        (*dsrPtr)->bufferMaxSizeInPacket = DSR_REXMT_BUFFER_SIZE;
    }

    ERROR_Assert(((*dsrPtr)->bufferMaxSizeInPacket) > 0,
        "DSR-BUFFER-MAX-PACKET needs to be a positive number\n");

    IO_ReadInt(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, 0),
        nodeInput,
        "DSR-BUFFER-MAX-BYTE",
        &retVal,
        &((*dsrPtr)->bufferMaxSizeInByte));

    if (retVal == FALSE)
    {
        (*dsrPtr)->bufferMaxSizeInByte = 0;
    }

    ERROR_Assert((*dsrPtr)->bufferMaxSizeInByte >= 0,
        "DSR-BUFFER-MAX-BYTE cannot be negative\n");

    // Initialize dsr internal structures

    // Initialize statistical variables
    memset(&(*dsrPtr)->stats, 0, sizeof(DsrStats));

    // Allocate chunk of memory
    DsrMemoryChunkAlloc(*dsrPtr);

    // Initialize request table
    DsrInitRequestTable(&(*dsrPtr)->reqTable);

    // Initialize route cache
    DsrInitRouteCache(&(*dsrPtr)->routeCache);

    // Initialize message buffer;
    DsrInitBuffer(&(*dsrPtr)->msgBuffer);

    DsrInitRexmtBuffer(&(*dsrPtr)->rexmtBuffer);

    // Initialize sequence number
    (*dsrPtr)->identificationNumber = 0;

    // Assign 0 th interface address as dsr local address.
    // This message should be used to send any request or reply

    (*dsrPtr)->localAddress = NetworkIpGetInterfaceAddress(node, 0);

    // Set network router function
    NetworkIpSetRouterFunction(
        node,
        &DsrRouterFunction,
        interfaceIndex);

    // Set mac layer status event handler
    NetworkIpSetMacLayerStatusEventHandlerFunction(
        node,
        &DsrMacLayerStatusHandler,
        interfaceIndex);

    // Set promiscuous message peek function
    NetworkIpSetPromiscuousMessagePeekFunction(
        node,
        &DsrPeekFunction,
        interfaceIndex);

    NetworkIpSetMacLayerAckHandler(
        node,
        &DsrMacAckHandler,
        interfaceIndex);

    if (DEBUG_TRACE)
    {
        DsrTraceInit(node, nodeInput, *dsrPtr);
    }

    // registering RoutingDsrHandleAddressChangeEvent function
    NetworkIpAddAddressChangedHandlerFunction(node,
                    &RoutingDsrHandleChangeAddressEvent);
}


//--------------------------------------------------------------------------
// FUNCTION: DsrFinalize
// PURPOSE:  Printing statistics collected during simulation
// ARGUMENTS: node, the node which is printing statistical information
// RETURN:   None
//--------------------------------------------------------------------------

void
DsrFinalize(Node* node)
{
    char buf[MAX_STRING_LENGTH];
    DsrData* dsr = (DsrData*)NetworkIpGetRoutingProtocol(
                       node,
                      ROUTING_PROTOCOL_DSR);

    if (DEBUG_ROUTING_TABLE)
    {
        DsrPrintRoutingTable(node);
    }

    if (dsr->statsCollected && !dsr->statsPrinted)
    {
        dsr->statsPrinted = TRUE;

        sprintf(buf, "Number of RREQ Initiated = %u",
            dsr->stats.numRequestInitiated);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Retried = %u",
            dsr->stats.numRequestResent);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Forwarded = %u",
            dsr->stats.numRequestRelayed);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREQ Received = %u",
            dsr->stats.numRequestRecved);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Duplicate RREQ Received = %u",
            dsr->stats.numRequestDuplicate);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);
        sprintf(buf, "Number of RREQ TTL Expired = %u",
            dsr->stats.numRequestTtlExpired);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);
        sprintf(buf, "Number of RREQ Received by Destination = %u",
            dsr->stats.numRequestRecvedAsDest);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);
        sprintf(buf, "Number of RREQ Discarded for Loop = %u",
            dsr->stats.numRequestInLoop);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);
        sprintf(buf, "Number of RREP Initiated as Destination = %u",
            dsr->stats.numReplyInitiatedAsDest);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREP Initiated as Intermediate Node = %u",
            dsr->stats.numReplyInitiatedAsIntermediate);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREP Received = %u",
            dsr->stats.numReplyRecved);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RREP Received as Source = %u",
            dsr->stats.numReplyRecvedAsSource);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Initiated = %u",
            dsr->stats.numRerrInitiated);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Received as Source = %u",
            dsr->stats.numRerrRecvedAsSource);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of RERR Received = %u",
            dsr->stats.numRerrRecved);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Data Packets Sent as Source = %u",
            dsr->stats.numDataInitiated);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Data Packets Forwarded = %u",
            dsr->stats.numDataForwarded);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Data Packets Received = %u",
            dsr->stats.numDataRecved);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Data Packets Dropped for No Route = %u",
            dsr->stats.numDataDroppedForNoRoute);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf,
            "Number of Data Packets Dropped for Buffer Overflow = %u",
            dsr->stats.numDataDroppedForOverlimit);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf,
            "Number of times Retransmission Failed = %u",
            dsr->stats.numDataDroppedRexmtTO);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Routes Selected = %u",
            dsr->stats.numRoutes);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Hop Counts = %u", dsr->stats.numHops);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);
        sprintf(buf, "Number of Packets Salvaged = %u",
            dsr->stats.numSalvagedPackets);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);
        sprintf(buf, "Number of times MAC Failed to Transmit Data "
                     "due to Link Failure = %u",
            dsr->stats.numLinkBreaks);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);

        sprintf(buf, "Number of Packets Dropped for Exceeding MTU = %u"
            ,dsr->stats.numPacketsGreaterMTU);
        IO_PrintStat(
            node,
            "Network",
            "DSR",
            ANY_DEST,
            -1,
            buf);
    }
}

//--------------------------------------------------------------------------
// FUNCTION   :: RoutingDsrHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
// PARAMETERS ::
// + node                    : Node*   : Pointer to Node structure
// + interfaceIndex          : Int32   : interface index
// + oldAddress              : Address*: old address
// + NetworkType networkType : type of network protocol
// RETURN :: void : NULL
//--------------------------------------------------------------------------
void RoutingDsrHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* old_addr,
    NodeAddress subnetMask,
    NetworkType networkType)
{
    DsrData* dsr = (DsrData*)NetworkIpGetRoutingProtocol(
                       node,
                      ROUTING_PROTOCOL_DSR);
    dsr->localAddress = NetworkIpGetInterfaceAddress(node, 0);
}
