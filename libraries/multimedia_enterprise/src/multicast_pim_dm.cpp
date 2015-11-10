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

/*
*  File: multicast_pim_dm.cpp
*
*
*  PUPPOSE: To simulate Protocol Independent Multicast (Dense Mode) Routing
*           Protocol (PIM-DM) as described in draft-ietf-pim-v2-dm-03 and
*           RFC 3973.
*           It resides at the network layer just above IP.
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#include "network_ip.h"
#include "list.h"
#include "buffer.h"
#include "multicast_igmp.h"
#ifdef ADDON_BOEINGFCS
#include "network_ces_region.h"
#include "network_ces_inc.h"
#include "network_ces_inc_sincgars.h"
#include "network_ces_inc_eplrs.h"
#include "routing_ces_sdr.h"
#include "mac_ces_wintncw.h"
#include "routing_ces_mpr.h"
#endif
#include "multicast_pim.h"

#define DEBUG 0
#define DEBUG_HELLO 0
#define DEBUG_MPACKET 0
#define DEBUG_JOIN_PRUNE 0
#define DEBUG_GRAFT 0
#define DEBUG_ASSERT 0
#define DEBUG_URC 0
#define DEBUG_WIRELESS 0
/*  Internal function declaration */


/* ------------------------------------------------------------------------*/
/*       PROTOTYPE FOR DIFFERENT FUNCTIONS RELATING FORWARDING CACHE       */
/* ------------------------------------------------------------------------*/
static BOOL
RoutingPimDmLookUpForwardingCache(
    Node*  node,
    Message* msg,
    int interfaceIndex,
    BOOL* packetWasRouted,
    NodeAddress prevHop,
    NodeAddress truePrevHop);

static void
RoutingPimDmBuildNewEntry(
    Node* node,
    Message* msg,
    int interfaceIndex,
    NodeAddress prevHop,
    NodeAddress truePrevHop,
    BOOL* packetWasRouted);

/* ------------------------------------------------------------------------*/
/*                           OTHER FUNCTIONS                               */
/* ------------------------------------------------------------------------*/

static BOOL
RoutingPimDmCompareMetric(
    int localPreference,
    int localMetric,
    NodeAddress localAddr,
    int remotePreference,
    int remoteMetric,
    NodeAddress remoteAddr);


/* ------------------------End of function declaration---------------------*/

/*
* FUNCTION     : RoutingPimDmAssertPacketSetRPTBit()
* PURPOSE      : Set the value of RPTBit for RoutingPimDmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : void
*/
static void
RoutingPimDmAssertPacketSetRPTBit(UInt32 *metricBitPrefDm,
                                              BOOL RPTBit)
{
    UInt32 x = RPTBit;

    //masks RPTBit within boundry range
    x = x & maskInt(32, 32);

    //clears first bit
    *metricBitPrefDm = *metricBitPrefDm & (~(maskInt(1, 1)));

    //setting value of x in metric_bit_pref
    *metricBitPrefDm = *metricBitPrefDm | LshiftInt(x, 1);
}

/*
* FUNCTION     : RoutingPimDmAssertPacketSetMetricPref()
* PURPOSE      : Set the value of metricPreference for
*                RoutingPimDmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : void
*/
static void
RoutingPimDmAssertPacketSetMetricPref(UInt32 *metricBitPrefDm,
                                                  UInt32 metricPreference)
{
    //masks metricPreference within boundry range
    metricPreference = metricPreference & maskInt(2, 32);

    //clears all bits except first bit
    *metricBitPrefDm = *metricBitPrefDm & maskInt(1, 1);

    //setting value of metricPreference in metricBitPrefDm
    *metricBitPrefDm = *metricBitPrefDm | metricPreference;
}

/*
* FUNCTION     : RoutingPimDmAssertPacketGetMetricPref()
* PURPOSE      : Returns the value of metricPreference for
*                RoutingPimDmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : UInt32
*/
static UInt32
RoutingPimDmAssertPacketGetMetricPref(UInt32 metricBitPrefDm)
{
    UInt32 metricPreference = metricBitPrefDm;

    //clears first bit
    metricPreference = metricPreference & maskInt(2, 32);

    return metricPreference;
}

#if 0
/* These functions are defined for completeness sake but not used */

/*
* FUNCTION     : RoutingPimDmAssertPacketGetRPTBit()
* PURPOSE      : Returns the value of RPTBit for RoutingPimDmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : BOOL
*/
static BOOL RoutingPimDmAssertPacketGetRPTBit(UInt32 metricBitPrefDm)
{
    UInt32 RPTBit = metricBitPrefDm;

    //clears all bits except first bit
    RPTBit = RPTBit & maskInt(1, 1);

    //right shift 31 places so that last bit represents RPTBit
    RPTBit = RshiftInt(RPTBit, 1);

    return (BOOL)RPTBit;
}

/*
* FUNCTION     : RoutingPimDmAssertPacketSetRPTBit()
* PURPOSE      : Set the value of RPTBit for RoutingPimDmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : void
*/
static void RoutingPimDmAssertPacketSetRPTBit(UInt32 *metricBitPrefDm,
                                              BOOL RPTBit)
{
    UInt32 x = RPTBit;

    //masks RPTBit within boundry range
    x = x & maskInt(32, 32);

    //clears first bit
    *metricBitPrefDm = *metricBitPrefDm & (~(maskInt(1, 1)));

    //setting value of x in metric_bit_pref
    *metricBitPrefDm = *metricBitPrefDm | LshiftInt(x, 1);
}


/*
* FUNCTION     : RoutingPimDmAssertPacketSetMetricPref()
* PURPOSE      : Set the value of metricPreference for
*                RoutingPimDmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : void
*/
static void RoutingPimDmAssertPacketSetMetricPref(UInt32 *metricBitPrefDm,
                                                  UInt32 metricPreference)
{
    //masks metricPreference within boundry range
    metricPreference = metricPreference & maskInt(2, 32);

    //clears all bits except first bit
    *metricBitPrefDm = *metricBitPrefDm & maskInt(1, 1);

    //setting value of metricPreference in metricBitPrefDm
    *metricBitPrefDm = *metricBitPrefDm | metricPreference;
}


/*
* FUNCTION     : RoutingPimDmAssertPacketGetRPTBit()
* PURPOSE      : Returns the value of RPTBit for RoutingPimDmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : BOOL
*/
static BOOL RoutingPimDmAssertPacketGetRPTBit(UInt32 metricBitPrefDm)
{
    UInt32 RPTBit = metricBitPrefDm;

    //clears all bits except first bit
    RPTBit = RPTBit & maskInt(1, 1);

    //right shift 31 places so that last bit represents RPTBit
    RPTBit = RshiftInt(RPTBit, 1);

    return (BOOL)RPTBit;
}


/*
* FUNCTION     : RoutingPimDmAssertPacketGetMetricPref()
* PURPOSE      : Returns the value of metricPreference for
*                RoutingPimDmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : UInt32
*/
static UInt32 RoutingPimDmAssertPacketGetMetricPref(UInt32 metricBitPrefDm)
{
    UInt32 metricPreference = metricBitPrefDm;

    //clears first bit
    metricPreference = metricPreference & maskInt(2, 32);

    return metricPreference;
}
#endif

/*
*  FUNCTION     :RoutingPimDmInitForwardingTable()
*  PURPOSE      :Initializes forwarding table
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmInitForwardingTable(Node* node)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }

    RoutingPimDmForwardingTable* fwdTable = &pimDmData->fwdTable;
    int size =
        sizeof(RoutingPimDmForwardingTableRow)*  PIM_INITIAL_TABLE_SIZE;

    BUFFER_InitializeDataBuffer(&fwdTable->buffer, size);
    fwdTable->numEntries = 0;
}


/**************************************************************************/
/*                      FORWARDING MECHANISM                              */
/**************************************************************************/

/*
*  FUNCTION     :RoutingPimDmRouterFunction()
*  PURPOSE      :Handle multicast forwarding of packet
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmRouterFunction(
    Node* node,
    Message* msg,
    NodeAddress grpAddr,
    int interfaceIndex,
    BOOL* packetWasRouted,
    NodeAddress prevHop)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    BOOL retVal = FALSE;
    BOOL originateByMe = FALSE;
    NodeAddress nextHop = 0;
    int interfaceId = 0;
    int i = 0;

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*) pim->pimModePtr)->pimDm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }

    /*
     *   If packet is from transport layer, send out to specified interface
     *   and then act as if this packet was received over that interface
     */

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ipHeader->ip_src == pim->interface[i].ipAddress)
        {
            // if this packet is from transport layer then this node is
            // source
            if (prevHop == ANY_IP)
            {
                if (DEBUG_MPACKET)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(ipHeader->ip_src,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(ipHeader->ip_dst,
                                                grpStr);
                    char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout, "Node %u @ %s: I am originator of mcast "
                                    "packet (%s , %s) with seq no. %d\n",
                                    node->nodeId, clockStr, srcStr, grpStr,
                                    msg->sequenceNumber);
                }

                pimDmData->stats.numDataPktOriginate++;
#ifdef ADDON_DB
                //this check is to avoid the counter increase for loopback packet
                if (msg->originatingNodeId == node->nodeId)
                {
                    pim->pimMulticastNetSummaryStats.m_NumDataSent++;
                }
#endif

                originateByMe = TRUE;
                interfaceIndex = CPU_INTERFACE;
                break;
            }
            else
            {
                // this is a loopback packet therefore return and no need
                // to forward further
#ifdef ADDON_DB
                if (NetworkIpIsPartOfMulticastGroup(node, ipHeader->ip_dst))
                {
                    pim->pimMulticastNetSummaryStats.m_NumDataRecvd++;
                }
#endif
                return;
            }
        }
    }

    if (DEBUG_MPACKET)
    {
        if (originateByMe == FALSE)
        {
            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(ipHeader->ip_src,
                                        srcStr);
            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(ipHeader->ip_dst,
                                        grpStr);
            char prevHopStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(prevHop,
                                        prevHopStr);
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

            fprintf(stdout, "Node %u @ %s: Received mcast packet (%s , %s)"
                            "with seq no. %d from %s\n",
                            node->nodeId, clockStr, srcStr, grpStr,
                            msg->sequenceNumber, prevHopStr);
        }
    }

#ifdef ADDON_BOEINGFCS
    if ((interfaceIndex >= 0 &&
        pim->interface[interfaceIndex].interfaceType ==
        MULTICAST_CES_RPIM_INTERFACE) &&
        (pim->modeType == ROUTING_PIM_MODE_DENSE))
    {
#if 0
        if ((originateByMe) && (node->numberInterfaces == 1))
        {
            *packetWasRouted = TRUE;
            MESSAGE_Free(node, msg);
            return;
        }
#endif
        if (RPimLookupMessageCache(msg->originatingNodeId,
                                   msg->sequenceNumber,
                                   IpHeaderGetIpFragOffset(ipHeader->ipFragment),
                                   &pimDmData->messageCache))
        {
#ifdef ADDON_DB
            HandleNetworkDBEvents(
                node,
                msg,
                interfaceIndex,
                "NetworkPacketDrop",
                "Duplicate Multicast Packet, RPIM",
                0,
                0,
                0,
                0);

#endif
            *packetWasRouted = TRUE;
            MESSAGE_Free(node, msg);
            return;
        }

        RPimInsertMessageCache(pim,
                               node,
                               msg->originatingNodeId,
                               msg->sequenceNumber,
                               IpHeaderGetIpFragOffset(ipHeader->ipFragment),
                               &pimDmData->messageCache);
    }
#endif

   /*  A source with only one interface doesn't need to create a
    *   forwarding cache entry.
    */

    pimDmData->stats.numDataPktReceived++;
#ifdef ADDON_DB
    if (!originateByMe)
    {
        pim->pimMulticastNetSummaryStats.m_NumDataRecvd++;
    }
#endif
    if (originateByMe)
    {
        nextHop = NEXT_HOP_ME;
    }
    else
    {
        /*  Search routing table for a entry to the source */
        NetworkGetInterfaceAndNextHopFromForwardingTable(
            node,
            ipHeader->ip_src,
            &interfaceId,
            &nextHop);
#ifdef ADDON_BOEINGFCS
        if ((interfaceIndex >= 0 &&
            pim->interface[interfaceIndex].interfaceType ==
            MULTICAST_CES_RPIM_INTERFACE) &&
            (pim->modeType == ROUTING_PIM_MODE_DENSE))
        {
            if (NetworkCesIncEplrsActiveOnNode(node))
            {
                if (prevHop != ANY_IP)
                {
                    nextHop = prevHop;
                }
                else
                {
                    nextHop = ipHeader->ip_src;
                }

                interfaceId = interfaceIndex;
            }

            /* If there is no route back to source discard packet silently */
            //if ((nextHop == (unsigned) NETWORK_UNREACHABLE) || nextHop == 0)
            if (nextHop == (unsigned) NETWORK_UNREACHABLE)
            {
                if (DEBUG)
                {
                    fprintf(stdout, "Node %u drop data packet as there is no "
                                    "route back to source %x\n",
                                    node->nodeId, ipHeader->ip_src);
                }
#ifdef ADDON_DB

                // STATS DB CODE
                HandleNetworkDBEvents(
                    node,
                    msg,
                    interfaceIndex,
                    "NetworkPacketDrop",
                    "No Route to Source",
                    0,
                    0,
                    0,
                    0);
#endif
                *packetWasRouted = TRUE;
                MESSAGE_Free(node, msg);
                pimDmData->stats.numDataPktDiscard++;
#ifdef ADDON_DB
                if (!originateByMe)
                {
                    pim->pimMulticastNetSummaryStats.m_NumDataDiscarded++;
                }
#endif
                return;
            }


            if (nextHop != NEXT_HOP_ME && prevHop != ANY_DEST)
            {
                // since ROSPF gives exit point rather then immediate next hop
                //nextHop = prevHop;
            }
        }
        else
        {
            if (nextHop == (unsigned) NETWORK_UNREACHABLE)
            {
                if (DEBUG)
                {
                    fprintf(stdout, "Node %u drop data packet as there is no "
                                    "route back to source %u\n",
                                    node->nodeId, ipHeader->ip_src);
                }

#ifdef ADDON_DB

                // STATS DB CODE
                HandleNetworkDBEvents(
                    node,
                    msg,
                    interfaceIndex,
                    "NetworkPacketDrop",
                    "No Route to Source",
                    0,
                    0,
                    0,
                    0);
#endif
                *packetWasRouted = TRUE;
                MESSAGE_Free(node, msg);
                pimDmData->stats.numDataPktDiscard++;
#ifdef ADDON_DB
                if (!originateByMe)
                {
                    pim->pimMulticastNetSummaryStats.m_NumDataDiscarded++;
                }
#endif
                return;
            }

        }
    }
#else
        /*  If there is no route back to source discard packet silently */
        if (nextHop == (unsigned) NETWORK_UNREACHABLE)
        {
            if (DEBUG_MPACKET)
            {
                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(ipHeader->ip_src,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(ipHeader->ip_dst,
                                            grpStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %u @ %s: Drop mcast packet for "
                                "(%s , %s) with seq no. %d"
                                "as there is no route back to "
                                "source %s\n",
                                node->nodeId, clockStr, srcStr, grpStr,
                                msg->sequenceNumber,srcStr);
            }

            /* Search for matching entry */
            RoutingPimDmForwardingTableRow* rowPtr =
                RoutingPimDmGetFwdTableEntryForThisPair(
                            node,
                            ipHeader->ip_src,
                            grpAddr);

            if (rowPtr == NULL)
            {
                /* Build new entry but don't do the forwarding */
                RoutingPimDmBuildNewEntry(node,
                                          msg,
                                          interfaceIndex,
                                          nextHop,
                                          prevHop,
                                          packetWasRouted);
            }

#ifdef ADDON_DB
            HandleNetworkDBEvents(
                node,
                msg,
                interfaceIndex,
                "NetworkPacketDrop",
                "No Route to Source",
                0,
                0,
                0,
                0);

#endif
            *packetWasRouted = TRUE;
            MESSAGE_Free(node, msg);
            pimDmData->stats.numDataPktDiscard++;
#ifdef ADDON_DB
            if (!originateByMe)
            {
                pim->pimMulticastNetSummaryStats.m_NumDataDiscarded++;
            }
#endif
            return;
        }
    }
#endif

    /* Try to forward packet by looking forwarding cache */
    retVal = RoutingPimDmLookUpForwardingCache(node, msg,
        interfaceIndex, packetWasRouted, nextHop, prevHop);

    /* If forwarding cache doesn't contain an entry, build one */
    if (!retVal)
    {
        /* If packet didn't come from desired interface, discard it */
        if ((!originateByMe) && (interfaceId != interfaceIndex))
        {
            if (DEBUG_MPACKET)
            {
                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(ipHeader->ip_src,
                                            srcStr);

                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(ipHeader->ip_dst,
                                            grpStr);

                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout, "Node %u @ %s: Discard (%s , %s) mcast "
                                "packet with seq no. %d as it does "
                                "not come from "
                                "RPF-Interface(S) %d\n",
                                node->nodeId, clockStr, srcStr, grpStr,
                                msg->sequenceNumber, interfaceId);
            }

#ifdef ADDON_DB
            HandleNetworkDBEvents(
                node,
                msg,
                interfaceIndex,
                "NetworkPacketDrop",
                "Packet Not from the Upstream Interface",
                0,
                0,
                0,
                0);

#endif
            *packetWasRouted = TRUE;
            MESSAGE_Free(node, msg);
            pimDmData->stats.numDataPktDiscard++;
#ifdef ADDON_DB
            if (!originateByMe)
            {
                pim->pimMulticastNetSummaryStats.m_NumDataDiscarded++;
            }
#endif
            return;
        }

        /*  Build an entry to forwarding cache */
        RoutingPimDmBuildNewEntry(node, msg, interfaceIndex,
                nextHop, prevHop, packetWasRouted);

    }

    if (*packetWasRouted == TRUE)
    {
        pimDmData->stats.numDataPktForward++;
#ifdef ADDON_DB
        if (!originateByMe)
        {
            pim->pimMulticastNetSummaryStats.m_NumDataForwarded++;
        }
#endif
        MESSAGE_Free(node, msg);
    }
    else
    {

#ifdef ADDON_DB
        if (!originateByMe &&
            !(NetworkIpIsPartOfMulticastGroup(node, ipHeader->ip_dst)))
        {
            pim->pimMulticastNetSummaryStats.m_NumDataDiscarded++;
        }
#endif
       *packetWasRouted = TRUE;
        MESSAGE_Free(node, msg);
        pimDmData->stats.numDataPktDiscard++;
    }

    if (DEBUG)
    {
        RoutingPimDmPrintForwardingTable(node);
    }
}


/*
*  FUNCTION     :RoutingPimDmLookUpForwardingCache()
*  PURPOSE      :Forward packet using forwarding cache
*  ASSUMPTION   :None
*  RETURN VALUE :Return TRUE if matching entry found
*/
static BOOL
RoutingPimDmLookUpForwardingCache(
    Node*  node,
    Message* msg,
    int interfaceIndex,
    BOOL* packetWasRouted,
    NodeAddress prevHop,
    NodeAddress truePrevHop)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    ListItem* tempListItem = NULL;
    RoutingPimDmDownstreamListItem* downstream = NULL;
    RoutingPimDmDataTimeoutInfo cacheTimer;

    /*  Search for matching entry */
    rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node,
                     ipHeader->ip_src, ipHeader->ip_dst);

    if (rowPtr == NULL)
    {
        /*  We don't have any entry for this pair */
        return FALSE;
    }
#ifdef ADDON_BOEINGFCS
   if ((interfaceIndex >= 0) &&
       (pim->interface[interfaceIndex].interfaceType !=
        MULTICAST_CES_RPIM_INTERFACE) &&
        (pim->modeType == ROUTING_PIM_MODE_DENSE))
    {
        if (RoutingPimDmIsDownstreamInterface(rowPtr, interfaceIndex)
            && (ipHeader->ip_src != pim->interface[interfaceIndex].ipAddress))
        {
            downstream = RoutingPimDmGetDownstreamInfo(
                rowPtr,
                interfaceIndex);

            if (DEBUG)
            {
                NodeAddress intfAddr = NetworkIpGetInterfaceAddress(node,
                                             interfaceIndex);
                fprintf(stdout, "Node %u receive data packet from downstream "
                                "interface %u interfaceIndex = %d\n",
                                 node->nodeId, intfAddr, interfaceIndex);
            }

#ifdef ADDON_DB
            HandleNetworkDBEvents(
                node,
                msg,
                interfaceIndex,
                "NetworkPacketDrop",
                "Packet Not from the Upstream Interface",
                0,
                0,
                0,
                0);

#endif
            if (pim->interface[interfaceIndex].interfaceType ==
                    ROUTING_PIM_BROADCAST_INTERFACE)
            {
                if (downstream->assertState == Pim_Assert_NoInfo)
                {
                    //Asume that I am the assert winner

                    NetworkDataIp* ip = (NetworkDataIp*)
                    node->networkData.networkVar;

                    if (ip->interfaceInfo[interfaceIndex]->routingProtocolType
                            == ROUTING_PROTOCOL_NONE)
                    {
                        ERROR_Assert(FALSE, "PIM-DM requires a unicast routing "
                                        "protocol to be running.\n");
                    }

                    downstream->assertState = Pim_Assert_IWonAssert;

                    downstream->winnerAssertMetric.ipAddress
                    = pim->interface[interfaceIndex].ipAddress;

                    downstream->winnerAssertMetric.metricPreference=
                        NetworkRoutingGetAdminDistance(node,
                            ip->interfaceInfo[interfaceIndex]
                                ->routingProtocolType);

                    downstream->winnerAssertMetric.metric =
                    NetworkGetMetricForDestAddress(
                        node, ipHeader->ip_src);

                }

                if (downstream->assertState == Pim_Assert_IWonAssert)
                {
                    /*  Send Assert on the downstream interface */
                    RoutingPimDmSendAssertPacket(node, ipHeader->ip_src,
                        ipHeader->ip_dst, interfaceIndex);

                    if (downstream->assertTime == 0)
                    {
                        RoutingPimDmAssertTimerInfo timerInfo;

                        timerInfo.srcAddr = rowPtr->srcAddr;
                        timerInfo.grpAddr = rowPtr->grpAddr;
                        RoutingPimSetTimer(node,
                                           interfaceIndex,
                                           MSG_ROUTING_PimDmAssertTimeOut,
                                           (void*) &timerInfo,
                                           ROUTING_PIM_DM_ASSERT_TIMEOUT);
                    }

                    downstream->assertTime = node->getNodeTime() +
                        ROUTING_PIM_DM_ASSERT_TIMEOUT;
                    downstream->assertTimerRunning = TRUE;
                }
            }

            return TRUE;
        }
    }
#else
    if (interfaceIndex >= 0
        && RoutingPimDmIsDownstreamInterface(rowPtr, interfaceIndex)
        && (ipHeader->ip_src != pim->interface[interfaceIndex].ipAddress))
    {
        downstream = RoutingPimDmGetDownstreamInfo(
            rowPtr,
            interfaceIndex);

#ifdef ADDON_DB
        HandleNetworkDBEvents(
            node,
            msg,
            interfaceIndex,
            "NetworkPacketDrop",
            "Packet Not from the Upstream Interface",
            0,
            0,
            0,
            0);

#endif

        if (pim->interface[interfaceIndex].dmInterface->neighborCount > 1)
        {
            RoutingPimDmAssertSrcListItem* assertSrcListItem
            = RoutingPimDmSearchAssertSrcPair(node,
                                             ipHeader->ip_src,
                                             interfaceIndex);

            if (pim->interface[interfaceIndex].assertOptimization == FALSE)
            {
                if (downstream->assertState == Pim_Assert_NoInfo)
                {
                    //Assume that I am the assert winner

                    NetworkDataIp* ip = (NetworkDataIp*)
                        node->networkData.networkVar;

                    downstream->assertState = Pim_Assert_IWonAssert;

                    downstream->winnerAssertMetric.ipAddress
                        = pim->interface[interfaceIndex].ipAddress;

                    if (ip->interfaceInfo[interfaceIndex]->routingProtocolType
                    == ROUTING_PROTOCOL_NONE)
                    {
                        downstream->winnerAssertMetric.metricPreference = 0;
                    }
                    else
                    {
                        downstream->winnerAssertMetric.metricPreference =
                                NetworkRoutingGetAdminDistance(node,
                                    ip->interfaceInfo[interfaceIndex]
                                        ->routingProtocolType);
                    }

                    downstream->winnerAssertMetric.metric =
                        NetworkGetMetricForDestAddress(
                            node, ipHeader->ip_src);
                }
            }
            else
            {
                if (assertSrcListItem
                   &&
                   assertSrcListItem->assertState == Pim_Assert_NoInfo)
                {
                    //Assume that I am the assert winner

                    NetworkDataIp* ip = (NetworkDataIp*)
                        node->networkData.networkVar;

                    if (ip->interfaceInfo[interfaceIndex]->routingProtocolType
                                == ROUTING_PROTOCOL_NONE)
                    {
                        ERROR_Assert(FALSE, "PIM-DM requires a unicast routing "
                                            "protocol to be running.\n");
                    }

                    assertSrcListItem->assertState = Pim_Assert_IWonAssert;

                    assertSrcListItem->winnerAssertMetric.ipAddress
                        = pim->interface[interfaceIndex].ipAddress;

                    if (ip->interfaceInfo[interfaceIndex]->routingProtocolType
                    == ROUTING_PROTOCOL_NONE)
                    {
                        assertSrcListItem->winnerAssertMetric.metricPreference = 0;
                    }
                    else
                    {
                        assertSrcListItem->winnerAssertMetric.metricPreference =
                                NetworkRoutingGetAdminDistance(node,
                                    ip->interfaceInfo[interfaceIndex]
                                        ->routingProtocolType);
                    }

                    assertSrcListItem->winnerAssertMetric.metric =
                        NetworkGetMetricForDestAddress(
                            node, ipHeader->ip_src);
                }
            }

            if (pim->interface[interfaceIndex].assertOptimization == FALSE)
            {
                if (downstream->assertState == Pim_Assert_IWonAssert)
                {
                    if (DEBUG_MPACKET || DEBUG_ASSERT)
                    {
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(ipHeader->ip_src,
                                                    srcStr);
                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(ipHeader->ip_dst,
                                                    grpStr);
                        char truePrevHopStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(truePrevHop,
                                                    truePrevHopStr);
                        char addrStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(
                            NetworkIpGetInterfaceAddress(node, interfaceIndex),
                            addrStr);
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                        fprintf(stdout,"Node %u @ %s: Received mcast packet "
                                       "(%s , %s) with seq no. %d from %s on "
                                       "downstream interface %d(%s)."
                                       "\nHence,Triggering assert mechanism.\n",
                                       node->nodeId,
                                       clockStr,
                                       srcStr,
                                       grpStr,
                                       msg->sequenceNumber,
                                       truePrevHopStr,
                                       interfaceIndex,
                                       addrStr);
                    }

                    /* Send Assert on the downstream interface */
                    RoutingPimDmSendAssertPacket(node, ipHeader->ip_src,
                            ipHeader->ip_dst, interfaceIndex);

                    if (downstream->assertTime == 0)
                    {
                        RoutingPimDmAssertTimerInfo timerInfo;

                        timerInfo.srcAddr = rowPtr->srcAddr;
                        timerInfo.grpAddr = rowPtr->grpAddr;
                        RoutingPimSetTimer(node,
                            interfaceIndex,
                            MSG_ROUTING_PimDmAssertTimeOut,
                            (void*) &timerInfo, ROUTING_PIM_DM_ASSERT_TIMEOUT);
                    }

                    downstream->assertTimerRunning = TRUE;
                    downstream->assertTime = node->getNodeTime() +
                        ROUTING_PIM_DM_ASSERT_TIMEOUT;
                }
            }
            else
            {
                if (assertSrcListItem
                    &&
                    assertSrcListItem->assertState == Pim_Assert_IWonAssert)
                {
                    /*  Send Assert on the downstream interface */
                    RoutingPimDmSendAssertPacket(node,
                                                 ipHeader->ip_src,
                                                 ipHeader->ip_dst,
                                                 interfaceIndex);

                    if (assertSrcListItem->assertTime == 0)
                    {
                        RoutingPimDmAssertTimerInfo timerInfo;

                        timerInfo.srcAddr = rowPtr->srcAddr;
                        timerInfo.grpAddr = rowPtr->grpAddr;
                        RoutingPimSetTimer(node,
                            interfaceIndex,
                            MSG_ROUTING_PimDmAssertTimeOut,
                            (void*) &timerInfo, ROUTING_PIM_DM_ASSERT_TIMEOUT);
                    }

                    assertSrcListItem->assertTimerRunning = TRUE;
                    assertSrcListItem->assertTime = node->getNodeTime() +
                        ROUTING_PIM_DM_ASSERT_TIMEOUT;
                }
            }
        }

        return TRUE;
    }
#endif
    /*  If packet didn't come from desired interface */
    if (interfaceIndex >= 0
        && rowPtr->inIntf != 
            NetworkIpGetInterfaceAddress(node, interfaceIndex))
    {
#ifdef ADDON_DB
        HandleNetworkDBEvents(
            node,
            msg,
            interfaceIndex,
            "NetworkPacketDrop",
            "Packet Not from Desired Interface",
            0,
            0,
            0,
            0);

#endif
        return TRUE;
    }

    /*  So we've got the packet from correct upstream */
    if (rowPtr->state == ROUTING_PIM_DM_PRUNE)
    {
      /*
       *   From router's point of view at this point we have no downstream
       *   dependent. But we consider a router as well as a host at the same
       *   time. So check first if we belongs to this group at all. If not
       *   then for a Point-to-Point network send prune immediately. For a
       *   Broadcast network if we havn't seen a join request from any other
       *   neighbor send prune again. I hope this will optimize the rate
       *   limited prune sending conception
       */

        //To-Do: This condition does not hold good for wireless
        //Need to change to support wireless networks correctly

        if ((!NetworkIpIsPartOfMulticastGroup(node, rowPtr->grpAddr))
            && (interfaceIndex >= 0)
            && ((pim->interface[interfaceIndex].dmInterface->neighborCount
                                    == 1)
                || (rowPtr->joinSeen == FALSE)))
        {
            if (pim->interface[interfaceIndex].broadcastMode == FALSE)
            {
                /*  Send prune again */
                RoutingPimDmSendJoinPrunePacket(node, ipHeader->ip_src,
                        ipHeader->ip_dst, UPSTREAM, ROUTING_PIM_PRUNE_TREE,
                        ROUTING_PIM_DM_PRUNE_TIMEOUT);

                rowPtr->delayedJoinTimer = FALSE;
                rowPtr->graftRxmtTimer = FALSE;
            }
        }
#ifdef ADDON_DB
        HandleNetworkDBEvents(
            node,
            msg,
            interfaceIndex,
            "NetworkPacketDrop",
            "Multicast Leaf Node, Does not Forward",
            0,
            0,
            0,
            0);
#endif
        return TRUE;
    }

    /*  If we reach here, we are able to forward packet to downstream */
    tempListItem = rowPtr->downstream->first;

    NodeAddress nextHop = 0;
    while (tempListItem)
    {
        downstream = (RoutingPimDmDownstreamListItem*) tempListItem->data;

        /*  Check if this interface is pruned */
        BOOL dontSendData = FALSE;
        if (pim->interface[downstream->interfaceIndex].assertOptimization == FALSE)
        {
            if (downstream->isPruned
                ||
                downstream->assertState == Pim_Assert_ILostAssert)
            {
                dontSendData = TRUE;
            }
        }
        else
        {
            RoutingPimDmAssertSrcListItem* assertSrcListItem
                = RoutingPimDmSearchAssertSrcPair(
                                            node,
                                            ipHeader->ip_src,
                                            downstream->interfaceIndex);

            if (downstream->isPruned
                ||
                (assertSrcListItem && assertSrcListItem->assertState ==
                        Pim_Assert_ILostAssert))
            {
                dontSendData = TRUE;
            }
        }

        if (dontSendData)
        {
            /*  Skip this interface */
            tempListItem = tempListItem->next;
            continue;
        }
#ifdef ADDON_BOEINGFCS
        if ((interfaceIndex >= 0) &&
            (pim->interface[interfaceIndex].interfaceType ==
                 MULTICAST_CES_RPIM_INTERFACE) &&
            (pim->modeType == ROUTING_PIM_MODE_DENSE))
        {
            if (pim->interface[interfaceIndex].useMpr)
            {
                if ((downstream->interfaceIndex == interfaceIndex)
                    && (ipHeader->ip_src != pim->interface[interfaceIndex].ipAddress))
                {
                    if (!NetworkCesRegionIsRap(node, interfaceIndex))
                    {
                        NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
                        // only forward packet if we are MPR forwarder...
                        BOOL useRoutingCesMpr = ip->interfaceInfo[interfaceIndex]->useRoutingCesMpr;
                        if (!(useRoutingCesMpr &&
                              (RoutingCesMprLookupNeighborMprSet(node, interfaceIndex, truePrevHop) == TRUE)))
                        {
                           // Skip this interface
                            tempListItem = tempListItem->next;
                            continue;
                        }
                    }
                }
            }

            nextHop = NetworkIpGetInterfaceBroadcastAddress(node,
                downstream->interfaceIndex);

            // need to add a delay because we may be dealing with
            // a CSMA like mac protocol.
            //clocktype delay = (clocktype)(RANDOM_erand(pim->seed) * MILLI_SECOND);

            NetworkIpSendPacketToMacLayer(node,
                                          MESSAGE_Duplicate(node, msg),
                                          downstream->interfaceIndex,
                                          nextHop);

        }
        else
        {
            nextHop = NetworkIpGetInterfaceBroadcastAddress(node,
                downstream->interfaceIndex);

            NetworkIpSendPacketToMacLayer(node,
                                      MESSAGE_Duplicate(node, msg),
                                      downstream->interfaceIndex,
                                      nextHop);
        }
#else

        nextHop = NetworkIpGetInterfaceBroadcastAddress(
                        node,
                        downstream->interfaceIndex);

        NetworkIpSendPacketToMacLayer(node,
                                      MESSAGE_Duplicate(node, msg),
                                      downstream->interfaceIndex,
                                      nextHop);
#endif
#ifdef ADDON_DB
        StatsDBConnTable::MulticastConnectivity stats;

        stats.nodeId = node->nodeId;
        stats.destAddr = ipHeader->ip_dst;
        strcpy(stats.rootNodeType,"Source");
        stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                           ipHeader->ip_src);
        stats.outgoingInterface = downstream->interfaceIndex;

        if (prevHop != NEXT_HOP_ME)
        {
            stats.upstreamNeighborId =
                     MAPPING_GetNodeIdFromInterfaceAddress(node,prevHop);
            stats.upstreamInterface = interfaceIndex;
        }
        else
        {
            //this is my packet
            stats.upstreamNeighborId = stats.rootNodeId;
            //adding interface index as CPU_INTERFACE
            stats.upstreamInterface = CPU_INTERFACE;
        }

        //if we are using database, update the multicast connection table
        STATSDB_HandleMulticastConnCreation(node,stats);
#endif
        if (DEBUG_MPACKET)
        {
            int interfaceId = downstream->interfaceIndex;
            char clockStr[MAX_STRING_LENGTH];
            NodeAddress netAddr = 0;

            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            netAddr = NetworkIpGetInterfaceNetworkAddress(node, interfaceId);

            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                        srcStr);
            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                        grpStr);
            char netStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(netAddr,
                                        netStr);

            fprintf(stdout, "Node %u @ %s: Forward mcast packet (%s , %s) "
                            " with seq no. %d to network %s\n",
                            node->nodeId, clockStr, srcStr, grpStr,
                            msg->sequenceNumber, netStr);
        }

        *packetWasRouted = TRUE;
        tempListItem = tempListItem->next;
    }

    /*  Set DataTimeout timer */
    if (rowPtr->expirationTimer == 0)
    {
        cacheTimer.srcAddr = rowPtr->srcAddr;
        cacheTimer.grpAddr = rowPtr->grpAddr;
        RoutingPimSetTimer(
            node,
            interfaceIndex,
            MSG_ROUTING_PimDmDataTimeOut,
            (void*) &cacheTimer,
            (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
    }

    /*  Reset cache expiration timer */
    rowPtr->expirationTimer = node->getNodeTime() + ROUTING_PIM_DM_DATA_TIMEOUT;

    return TRUE;
}


/*
*  FUNCTION     :RoutingPimDmBuildNewEntry()
*  PURPOSE      :Build new entry in forwarding cache and forward packet
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
static void
RoutingPimDmBuildNewEntry(
        Node* node,
        Message* msg,
        int interfaceIndex,
        NodeAddress prevHop,
        NodeAddress truePrevHop,
        BOOL* packetWasRouted)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    RoutingPimDmForwardingTable* fwdTable = &pimDmData->fwdTable;
    RoutingPimDmForwardingTableRow newRow;
    memset(&newRow, 0, sizeof(RoutingPimDmForwardingTableRow));
    int i = 0;

    /*  Create new entry */
    newRow.srcAddr = ipHeader->ip_src;
    newRow.grpAddr = ipHeader->ip_dst;

    if (interfaceIndex >= 0)
    {
        newRow.inIntf = NetworkIpGetInterfaceAddress(node, interfaceIndex);
    }
    else
    {
        newRow.inIntf = (unsigned int)CPU_INTERFACE;
    }

    newRow.upstream = prevHop;

    /*  Make sure the previous hop is a PIM Router */
    if ((prevHop != NEXT_HOP_ME)
        &&
        (prevHop != ipHeader->ip_src))
    {
        RoutingPimNeighborListItem* neighbor = NULL;

        /*  Search neighbor in neighbor list */
        if (interfaceIndex >= 0)
        {
            if (DEBUG_HELLO)
            {
                RoutingPimDmPrintNeibhborList(node);
            }

            RoutingPimSearchNeighborList(
                pim->interface[interfaceIndex].dmInterface->neighborList,
                prevHop,
                &neighbor);
        }
#ifdef ADDON_BOEINGFCS
       if ((pim->interface[interfaceIndex].interfaceType ==
            MULTICAST_CES_RPIM_INTERFACE) &&
            (pim->modeType == ROUTING_PIM_MODE_DENSE))
        {
            /*if (neighbor == NULL &&
                !RoutingCesSdrActiveOnNode(node) &&
                !NetworkCesIncEplrsActiveOnNode(node))
            {
                if (DEBUG)
                {
                    fprintf(stdout, "Node %ld: Upstream %x is not a PIM router, "
                                    "Unable to create new entry\n",
                                    node->nodeId, prevHop);
                }

#ifdef ADDON_DB
                HandleNetworkDBEvents(
                    node,
                    msg,
                    interfaceIndex,
                    "NetworkPacketDrop",
                    "Upstream Is Not a PIM Router",
                    0,
                    0,
                    0,
                    0);

#endif
                return;
            }*/

            int wintInterface = MacCesWintNcwActiveOnInterface(node);

            if (wintInterface > -1)
            {
                if (ip->interfaceInfo[interfaceIndex]->routingProtocolType
                    == ROUTING_PROTOCOL_NONE)
                {
                    newRow.preference = (NetworkRoutingAdminDistanceType) 0;
                    newRow.metric = 0;
                }
                else
                {
                    newRow.preference = NetworkRoutingGetAdminDistance(node,
                        ip->interfaceInfo[interfaceIndex]->routingProtocolType);
                    newRow.metric = NetworkGetMetricForDestAddress(node,
                        ipHeader->ip_src);
                }
            }
            else
            {
                if (NetworkCesIncEplrsActiveOnInterface(node, interfaceIndex) &&
                    ip->interfaceInfo[interfaceIndex]->routingProtocolType
                    == ROUTING_PROTOCOL_NONE)
                {
                    newRow.preference = (NetworkRoutingAdminDistanceType) 0;
                    newRow.metric = 0;
                }
                else
                {
                    newRow.metric =
                        NetworkGetMetricForDestAddress(
                        node,
                        ipHeader->ip_src,
                        &(newRow.preference));
                }
            }
        }
        else
        {
            newRow.metric =
                NetworkGetMetricForDestAddress(
                    node,
                    ipHeader->ip_src,
                    &(newRow.preference));

        }

#else
        // If the upstream is not a router do not add it to fwding cache
        if (neighbor == NULL)
        {
            if (prevHop != ipHeader->ip_src)
            {
                if (DEBUG_MPACKET)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(ipHeader->ip_src,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(ipHeader->ip_dst,
                                                grpStr);
                    char truePrevHopStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(truePrevHop,
                                                truePrevHopStr);

                    printf("Node %u @ %s: Received multicast data packet "
                           "with seq no. %d from "
                           "a non-PIM neighbor %s for (%s , %s)."
                           "Hence,Dropping a packet.\n",node->nodeId,
                           clockStr, msg->sequenceNumber, truePrevHopStr,
                           srcStr, grpStr);
                }
            }

#ifdef ADDON_DB
            HandleNetworkDBEvents(
                node,
                msg,
                interfaceIndex,
                "NetworkPacketDrop",
                "Upstream Is Not a PIM Router",
                0,
                0,
                0,
                0);

#endif
            return;
        }

#endif
    }

    newRow.preference = ROUTING_ADMIN_DISTANCE_DEFAULT;
    newRow.metric = ROUTING_PIM_INFINITE_METRIC;

    newRow.state = ROUTING_PIM_DM_FORWARD;
    newRow.expirationTimer = 0;
    newRow.assertState = Pim_Assert_NoInfo;
    newRow.assertTime = 0;
    newRow.assertTimerRunning = FALSE;
    newRow.graftRxmtTimer = FALSE;
    newRow.delayedJoinTimer = FALSE;
    newRow.joinSeen = FALSE;
    newRow.lastJoinTimerEnd = 0;
    newRow.delayedJoinTimerSeqNo = 0;

    /*  Initializes downstream list */
    ListInit(node, &newRow.downstream);

    if (DEBUG_MPACKET)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char srcStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(newRow.srcAddr,
                                    srcStr);
        char grpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(newRow.grpAddr,
                                    grpStr);

        fprintf(stdout, "Node %u @ %s: Build new (%s , %s) entry\n",
                        node->nodeId,
                        clockStr,
                        srcStr,
                        grpStr);
    }

  /*
   *   Initially assume each interface other than the incoming
   *   one as downstream
   */

    NodeAddress nextHop = 0;
    for (i = 0; i < node->numberInterfaces; i++)
    {
#ifdef ADDON_BOEINGFCS
        if ((interfaceIndex >= 0 &&
            pim->interface[interfaceIndex].interfaceType !=
            MULTICAST_CES_RPIM_INTERFACE) &&
            (pim->modeType == ROUTING_PIM_MODE_DENSE))
        {
            if (i == interfaceIndex
                && (ipHeader->ip_src != pim->interface[i].ipAddress))
            {
                continue;
            }
        }
        else if (i == interfaceIndex
            && (ipHeader->ip_src != pim->interface[i].ipAddress))
       {
           if (interfaceIndex >= 0 &&
               pim->interface[interfaceIndex].useMpr)
           {
               if (!NetworkCesRegionIsRap(node, interfaceIndex))
               {
                   // only forward packet if we are MPR forwarder...
                   BOOL useRoutingCesMpr = ip->interfaceInfo[interfaceIndex]->useRoutingCesMpr;
                   if (!(useRoutingCesMpr &&
                         (RoutingCesMprLookupNeighborMprSet(node, interfaceIndex, truePrevHop) == TRUE)))
                   {
                       // Skip this interface
                       continue;
                   }
               }
           }
       }
#else


        /*  Skip incoming interface */
        if (i == interfaceIndex
            && (ipHeader->ip_src != pim->interface[i].ipAddress))
        {
            //add source entry in the source assert list
            if (pim->interface[i].assertOptimization == TRUE)
            {
                RoutingPimDmAddAssertSrcPair(node,
                                             ipHeader->ip_src,
                                             ipHeader->ip_dst,
                                             i,
                                             TRUE,
                                             prevHop);//isUpstream
            }
            continue;
        }
#endif
       /**  NOTE: <draft-ietf-mboned-intro-multicast-02.txt>
       *
       *   Unlike the DVMRP which calculates a set of child interfaces for
       *   each (S, G) pair, PIM-DM simply forwards multicast traffic on all
       *   downstream interfaces until explicit prune messages are received.
       *   PIM-DM is willing to accept packet duplication to eliminate
       *   routing protocol dependencies and to avoid the overhead inherent
       *   in determining the parent/child relationships.
       */

      /*
       *   Consider an interface as downstream:
       *                 if 1) It has a downstream neighbor
       *              or if 2) It has a Local member present for this group
       */

        //add source entry in the source assert list
        if (pim->interface[i].assertOptimization == TRUE)
        {
            RoutingPimDmAddAssertSrcPair(node,
                                            ipHeader->ip_src,
                                            ipHeader->ip_dst,
                                            i,
                                            FALSE,
                                            prevHop);//isUpstream
        }

        if ((pim->interface[i].dmInterface->neighborCount >= 1)
            || (RoutingPimDmHasLocalGroup(node, ipHeader->ip_dst, i))
            || NetworkIpIsPartOfMulticastGroup(node, ipHeader->ip_dst))

        {
            RoutingPimDmAddDownstream(node, &newRow, i);

            if (prevHop == (unsigned int)NETWORK_UNREACHABLE)
            {
                continue;
            }

            /*  Forward packet out this interface */
            nextHop = NetworkIpGetInterfaceBroadcastAddress(node, i);

/*#ifdef ADDON_BOEINGFCS
            if ((pim->interface[interfaceIndex].interfaceType ==
                 MULTICAST_CES_RPIM_INTERFACE) &&
                 (pim->modeType == ROUTING_PIM_MODE_DENSE))
            {

                // need to add a delay because we may be dealing with
                // a CSMA like mac protocol.

                clocktype delay =(clocktype)
                                 (RANDOM_erand(pim->seed) * MILLI_SECOND);

                NetworkIpSendPacketToMacLayerWithDelay(node,
                                              MESSAGE_Duplicate(node, msg),
                                              i,
                                              nextHop,
                                              delay);
            }
#else*/

            NetworkIpSendPacketToMacLayer(node,
                                          MESSAGE_Duplicate(node, msg),
                                          i,
                                          nextHop);
//#endif
#ifdef ADDON_DB
            StatsDBConnTable::MulticastConnectivity stats;
            BOOL originateByMe = FALSE;

            stats.nodeId = node->nodeId;
            stats.destAddr = ipHeader->ip_dst;
            strcpy(stats.rootNodeType,"Source");
            stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                           ipHeader->ip_src);
            stats.outgoingInterface = i;

            for (int j = 0; j < node->numberInterfaces; j++)
            {
                if (ipHeader->ip_src == pim->interface[j].ipAddress)
                {
                    originateByMe = TRUE;
                }
            }

            if (!originateByMe && prevHop == NEXT_HOP_ME)
            {
                prevHop = ipHeader->ip_src;
            }

            if (prevHop != NEXT_HOP_ME)
            {
                stats.upstreamNeighborId =
                         MAPPING_GetNodeIdFromInterfaceAddress(node,prevHop);
                stats.upstreamInterface = interfaceIndex;
            }
            else
            {
                //this is my packet
                stats.upstreamNeighborId = stats.rootNodeId;
                //adding interface index as CPU_INTERFACE
                stats.upstreamInterface = CPU_INTERFACE;
            }

            // if we are using database, update the multicast connection table
            STATSDB_HandleMulticastConnCreation(node,stats);
#endif

            if (DEBUG_MPACKET)
            {
                char clockStr[MAX_STRING_LENGTH];
                NodeAddress netAddr = 0;

                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                netAddr = NetworkIpGetInterfaceNetworkAddress(node, i);

                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(newRow.srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(newRow.grpAddr,
                                            grpStr);
                char netStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(netAddr,
                                            netStr);

                fprintf(stdout, "Node %u @ %s: Forward (%s , %s) mcast "
                                "packet with seq no. %d to network %s\n",
                                node->nodeId, clockStr, srcStr, grpStr,
                                msg->sequenceNumber, netStr);
            }

            *packetWasRouted = TRUE;
        }
    }

    if (newRow.expirationTimer == 0)
    {
        RoutingPimDmDataTimeoutInfo cacheTimer;

        /*  Set DataTimeout timer */
        cacheTimer.srcAddr = ipHeader->ip_src;
        cacheTimer.grpAddr = ipHeader->ip_dst;
        RoutingPimSetTimer(
            node,
            interfaceIndex,
            MSG_ROUTING_PimDmDataTimeOut,
            (void*) &cacheTimer,
            (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
    }

    /* Set DataTimeout timer */
    newRow.expirationTimer = node->getNodeTime() + ROUTING_PIM_DM_DATA_TIMEOUT;

    /* Insert new entry */
    fwdTable->numEntries++;
    BUFFER_AddDataToDataBuffer(&fwdTable->buffer, (char*) &newRow,
            sizeof(RoutingPimDmForwardingTableRow));

    /*
     *   If downstream list is empty send prune to upstream
     *   if it's not the source
     */

    if (newRow.downstream->size == 0
        && prevHop != (unsigned int) NETWORK_UNREACHABLE)
    {
        RoutingPimDmForwardingTableRow* rowPtr = NULL;

        if (DEBUG_MPACKET)
        {
            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(newRow.srcAddr,
                                        srcStr);
            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(newRow.grpAddr,
                                        grpStr);

            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

            fprintf(stdout, "Node %u @ %s: (%s , %s) entry has empty outgoing "
                            "interface list\n",
                            node->nodeId, clockStr, srcStr, grpStr);
        }

        rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node,
                         ipHeader->ip_src, ipHeader->ip_dst);
        ERROR_Assert(rowPtr, "Can't find row\n");

        rowPtr->state = ROUTING_PIM_DM_PRUNE;

        /*  Send prune to upstream */

        /*
         *   we check whether we belong to this group,
         *   before sending prune
         */
        if ((!NetworkIpIsPartOfMulticastGroup(node, rowPtr->grpAddr))
            && (interfaceIndex >= 0)
            && ((pim->interface[interfaceIndex].dmInterface->neighborCount
                                    == 1)
                || (rowPtr->joinSeen == FALSE)))
        {
            if (pim->interface[interfaceIndex].broadcastMode == FALSE)
            {
                /* Send prune again */
                RoutingPimDmSendJoinPrunePacket(node, ipHeader->ip_src,
                ipHeader->ip_dst, UPSTREAM, ROUTING_PIM_PRUNE_TREE,
                ROUTING_PIM_DM_PRUNE_TIMEOUT);
            }
        }
#ifdef ADDON_DB
        HandleNetworkDBEvents(
            node,
            msg,
            interfaceIndex,
            "NetworkPacketDrop",
            "Multicast Leaf Node, Does not Forward",
            0,
            0,
            0,
            0);
#endif
    }
}


/*
*  FUNCTION     :RoutingPimDmGetFwdTableEntryForThisPair()
*  PURPOSE      :Find a matcing entry for this (S, G) pair
*  ASSUMPTION   :None
*  RETURN VALUE :RoutingPimDmForwardingTableRow*
*/
RoutingPimDmForwardingTableRow*
        RoutingPimDmGetFwdTableEntryForThisPair(
        Node* node,
        NodeAddress srcAddr,
        NodeAddress grpAddr)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    unsigned int i = 0;

    rowPtr = (RoutingPimDmForwardingTableRow*)
                     BUFFER_GetData(&pimDmData->fwdTable.buffer);

    for (i = 0; i < pimDmData->fwdTable.numEntries; i++)
    {
        if ((rowPtr[i].srcAddr == srcAddr) && (rowPtr[i].grpAddr == grpAddr))
        {
            return &rowPtr[i];
        }
    }

    return NULL;
}


/*
*  FUNCTION     :RoutingPimDmIsDownstreamInterface()
*  PURPOSE      :Check to see if this interface is one of the downstream
*  ASSUMPTION   :None
*  RETURN VALUE :BOOL
*/
BOOL
RoutingPimDmIsDownstreamInterface(
    RoutingPimDmForwardingTableRow* rowPtr,
    int interfaceIndex)
{
    ListItem* listItem = rowPtr->downstream->first;

    while (listItem)
    {
        RoutingPimDmDownstreamListItem* downstream;

        downstream = (RoutingPimDmDownstreamListItem*) listItem->data;

        if (downstream->interfaceIndex == interfaceIndex)
        {
            return TRUE;
        }
        listItem = listItem->next;
    }
    return FALSE;
}

/*
*  FUNCTION     :RoutingPimDmGetDownstreamInfo()
*  PURPOSE      :Get downstream information
*  ASSUMPTION   :None
*  RETURN VALUE :Return desired information, return NULL if not found
*/
RoutingPimDmDownstreamListItem*
RoutingPimDmGetDownstreamInfo(RoutingPimDmForwardingTableRow* rowPtr,
                              int interfaceIndex)
{
    ListItem* listItem = rowPtr->downstream->first;
    RoutingPimDmDownstreamListItem* downstream = NULL;

    while (listItem)
    {
        downstream = (RoutingPimDmDownstreamListItem*) listItem->data;

        if (downstream->interfaceIndex == interfaceIndex)
        {
            return downstream;
        }
        listItem = listItem->next;
    }
    return NULL;
}


/*
*  FUNCTION     :RoutingPimDmRemoveDownstream()
*  PURPOSE      :Remove a downstream
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmRemoveDownstream(
    Node* node,
    RoutingPimDmForwardingTableRow* rowPtr,
    int interfaceId)
{
    ListItem* listItem = rowPtr->downstream->first;
    RoutingPimDmDownstreamListItem* downstream = NULL;

    while (listItem)
    {
        downstream = (RoutingPimDmDownstreamListItem*) listItem->data;

        if (downstream->interfaceIndex == interfaceId)
        {
            if (DEBUG_JOIN_PRUNE)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                            grpStr);
                char downstreamStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(downstream->intfAddr,
                                            downstreamStr);

                fprintf(stdout, "Node %u @ %s: Remove downstream interface "
                                "%s for (%s , %s)\n",
                                 node->nodeId, clockStr,
                                 downstreamStr, srcStr, grpStr);
            }

            ListGet(node, rowPtr->downstream, listItem, TRUE, FALSE);

#ifdef ADDON_DB
            // remove this entry from multicast_connectivity cache
            StatsDBConnTable::MulticastConnectivity stats;

            stats.nodeId = node->nodeId;
            stats.destAddr = rowPtr->grpAddr;
            strcpy(stats.rootNodeType,"Source");
            stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                            rowPtr->srcAddr);
            stats.outgoingInterface = interfaceId;
            stats.upstreamNeighborId =
                      MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                           rowPtr->upstream);
            stats.upstreamInterface =
                                NetworkIpGetInterfaceIndexFromAddress(node,
                                                             rowPtr->inIntf);

            // delete entry from  multicast_connectivity cache
            STATSDB_HandleMulticastConnDeletion(node, stats);
#endif
            break;
        }
        listItem = listItem->next;
    }
}




/*
*  FUNCTION     :RoutingPimDmAddDownstream()
*  PURPOSE      :Add new downstream
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmAddDownstream(
    Node* node,
    RoutingPimDmForwardingTableRow* rowPtr,
    int interfaceId)
{
    if (interfaceId < 0)
    {
        return;
    }

    NodeAddress nextHop = (unsigned int)NETWORK_UNREACHABLE;
    int outInterface = NETWORK_UNREACHABLE;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    RoutingPimDmDownstreamListItem* downstreamInfo =
        (RoutingPimDmDownstreamListItem*)
        MEM_malloc(sizeof(RoutingPimDmDownstreamListItem));
    memset(downstreamInfo, 0, sizeof (RoutingPimDmDownstreamListItem));

    downstreamInfo->intfAddr = NetworkIpGetInterfaceAddress(
        node,
        interfaceId);

    downstreamInfo->interfaceIndex = interfaceId;

    /*  Not used in simulation */
    downstreamInfo->ttl = IPDEFTTL;
    downstreamInfo->isPruned = FALSE;
    downstreamInfo->pruneTimer = (clocktype) 0;
    downstreamInfo->lastPruneReceived = (clocktype) 0;
    downstreamInfo->delayedPruneActive = FALSE;
    downstreamInfo->delayedPruneTimer = (clocktype) 0;

    downstreamInfo->assertState = Pim_Assert_NoInfo;
    downstreamInfo->assertTime = 0;
    downstreamInfo->assertTimerRunning = FALSE;

    NetworkGetInterfaceAndNextHopFromForwardingTable(node,
                                                     rowPtr->srcAddr,
                                                     &outInterface,
                                                     &nextHop);

    if (nextHop == (unsigned) NETWORK_UNREACHABLE)
    {
        downstreamInfo->winnerAssertMetric.metricPreference =
            ROUTING_ADMIN_DISTANCE_DEFAULT;
        downstreamInfo->winnerAssertMetric.metric =
            ROUTING_PIM_INFINITE_METRIC;
        downstreamInfo->winnerAssertMetric.ipAddress =
            (unsigned) NETWORK_UNREACHABLE;
    }
    else
    {
        if (ip->interfaceInfo[interfaceId]->routingProtocolType
                == ROUTING_PROTOCOL_NONE)
        {
            downstreamInfo->winnerAssertMetric.metricPreference = 0;
        }
        else
        {
            downstreamInfo->winnerAssertMetric.metricPreference =
                NetworkRoutingGetAdminDistance(node,
                    ip->interfaceInfo[interfaceId]->routingProtocolType);
        }

        downstreamInfo->winnerAssertMetric.metric =
            NetworkGetMetricForDestAddress(node,
                                           rowPtr->srcAddr);
        downstreamInfo->winnerAssertMetric.ipAddress =
            downstreamInfo->intfAddr;
    }

    /*  Insert item to downstream list */
    ListInsert(node, rowPtr->downstream,
            node->getNodeTime(), (void*) downstreamInfo);

    if (DEBUG_JOIN_PRUNE)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        char srcStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                    srcStr);
        char grpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                    grpStr);
        char downstreamStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(downstreamInfo->intfAddr,
                                    downstreamStr);

        fprintf(stdout, "Node %u @ %s: Add downstream interface %s for "
                        "(%s , %s)\n",
                         node->nodeId, clockStr, downstreamStr, srcStr,
                         grpStr);
    }
}

/**************************************************************************/
/*                     INTERACTION WITH IGMP                              */
/**************************************************************************/

/*
*  FUNCTION     :RoutingPimDmLocalMembersJoinOrLeave()
*  PURPOSE      :Handle local group membership events
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmLocalMembersJoinOrLeave(
    Node* node,
                                       NodeAddress groupAddr,
                                       int interfaceId,
                                       LocalGroupMembershipEventType event)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;

    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    RoutingPimDmDownstreamListItem* downstreamInfo = NULL;
    RoutingPimInterface* thisInterface = NULL;
    unsigned int i = 0;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
    rowPtr = (RoutingPimDmForwardingTableRow*)
                     BUFFER_GetData(&pimDmData->fwdTable.buffer);
    thisInterface = &pim->interface[interfaceId];
    int srcIntf = NETWORK_UNREACHABLE;
    NodeAddress nextHop = NETWORK_UNREACHABLE;


    switch (event)
    {
        case LOCAL_MEMBER_JOIN_GROUP:
        {
            if (DEBUG_JOIN_PRUNE)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(groupAddr,
                                            grpStr);
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(
                    NetworkIpGetInterfaceAddress(node, interfaceId),
                    addrStr);

                fprintf(stdout, "Node %u @ %s: Received IGMP Join "
                                "Notification for "
                                "group %s on interface %d(%s)\n",
                                 node->nodeId, clockStr, grpStr,
                                 interfaceId, addrStr);
            }

#ifdef CYBER_CORE
                NetworkDataIp* ip = (NetworkDataIp *)
                    node->networkData.networkVar;

                if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
                {
                    IAHEPCreateIgmpJoinLeaveMessage(
                        node,
                        groupAddr,
                        IGMP_MEMBERSHIP_REPORT_MSG);

                    if (IAHEP_DEBUG)
                    {
                        char addrStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(groupAddr, addrStr);
                        printf("\nRed Node: %d", node->nodeId);
                        printf("\tJoins Group: %s\n", addrStr);
                    }
                }
#endif

            /*  Search entire forwarding table for matching entry */
            for (i = 0; i < pimDmData->fwdTable.numEntries; i++)
            {
                if (rowPtr[i].grpAddr == groupAddr)
                {
 #ifdef ADDON_BOEINGFCS
                    if ((pim->interface[interfaceId].interfaceType!=
                        MULTICAST_CES_RPIM_INTERFACE) &&
                        (pim->modeType == ROUTING_PIM_MODE_DENSE))
                    {
                        if (rowPtr[i].srcAddr == thisInterface->ipAddress)
                        {
                            continue;
                        }
                    }
#else

                  /*
                   *   We don't need to add source interface explicitly as
                   *   multicast packet by default transmitted out this
                   *   interface always
                   */
                    if (rowPtr[i].srcAddr == thisInterface->ipAddress)
                    {
                        continue;
                    }

                    NetworkGetInterfaceAndNextHopFromForwardingTable(node,
                        rowPtr[i].srcAddr, &srcIntf, &nextHop);

                    if (srcIntf == interfaceId
#ifdef ADDON_BOEINGFCS
                        &&
                        pim->interface[interfaceId].interfaceType!=
                        MULTICAST_CES_RPIM_INTERFACE
#endif
                        )
                    {
                        continue;
                    }

#endif
#ifdef ADDON_BOEINGFCS
                    int srcIntf;
                    NodeAddress nextHop;
                    
                    NetworkGetInterfaceAndNextHopFromForwardingTable(node,
                        rowPtr[i].srcAddr, &srcIntf, &nextHop);
                    
                    if (srcIntf == interfaceId && pim->interface[interfaceId].interfaceType!=
                        MULTICAST_CES_RPIM_INTERFACE)
                    {
                        continue;
                    }
#endif
                    /*  If existing interface */
                    if (RoutingPimDmIsDownstreamInterface(&rowPtr[i],
                                                          interfaceId))
                    {
                        downstreamInfo = RoutingPimDmGetDownstreamInfo(
                                                 &rowPtr[i], interfaceId);
                        downstreamInfo->isPruned = FALSE;
                    }
                    /* Else create new one */
                    else
                    {
                        RoutingPimDmAddDownstream(node,
                            &rowPtr[i],
                            interfaceId);
                    }

                  /*
                   *   If Fwd table entry transitions to Fwd state,
                   *   send graft to upstream
                   */

                    if (rowPtr[i].state == ROUTING_PIM_DM_PRUNE)
                    {
                        RoutingPimDmDataTimeoutInfo cacheTimer;
                        rowPtr[i].state = ROUTING_PIM_DM_ACKPENDING;
                        RoutingPimDmSendGraftPacket(node, rowPtr[i].srcAddr,
                                rowPtr[i].grpAddr);

                        if (rowPtr[i].expirationTimer == 0)
                        {
                            cacheTimer.srcAddr = rowPtr[i].srcAddr;
                            cacheTimer.grpAddr = rowPtr[i].grpAddr;
                            RoutingPimSetTimer(
                            node,
                            interfaceId,
                            MSG_ROUTING_PimDmDataTimeOut,
                            (void*) &cacheTimer,
                            (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                        }
                        /*  Set DataTimeout timer */
                        rowPtr[i].expirationTimer = node->getNodeTime() +
                            ROUTING_PIM_DM_DATA_TIMEOUT;
                    }
                }
            }

            break;
        }

        case LOCAL_MEMBER_LEAVE_GROUP:
        {
            if (DEBUG_JOIN_PRUNE)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(groupAddr,
                                            grpStr);
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(
                    NetworkIpGetInterfaceAddress(node, interfaceId),
                    addrStr);

                fprintf(stdout, "Node %u @ %s: Received IGMP Leave "
                                "Notification for "
                                "group %s on interface %d(%s)\n",
                                 node->nodeId, clockStr, grpStr,
                                 interfaceId, addrStr);
            }

            /*  Search entire forwarding table for matching entry */
            for (i = 0; i < pimDmData->fwdTable.numEntries; i++)
            {
                if (rowPtr[i].grpAddr == groupAddr)
                {
                    /*
                    *   Match found, check if this interface was in outgoing
                    *   interface list
                    */
                    if (!RoutingPimDmIsDownstreamInterface(&rowPtr[i],
                                interfaceId))
                    {
                        continue;
                    }

                    /* Remove this interface */
                    RoutingPimDmRemoveDownstream(node, &rowPtr[i],
                            interfaceId);

                    /*
                    *   If all downstreams have been pruned
                    *   send prune to upstream
                    */
                    if (RoutingPimDmAllDownstreamPruned(&rowPtr[i]))
                    {
                        if (thisInterface->broadcastMode == FALSE)
                        {
                            RoutingPimDmDataTimeoutInfo cacheTimer;

                            rowPtr[i].state = ROUTING_PIM_DM_PRUNE;
                            RoutingPimDmSendJoinPrunePacket(
                                node,
                                rowPtr[i].srcAddr,
                                rowPtr[i].grpAddr,
                                UPSTREAM,
                                ROUTING_PIM_PRUNE_TREE,
                                ROUTING_PIM_DM_PRUNE_TIMEOUT);

                            rowPtr[i].graftRxmtTimer = FALSE;
                            rowPtr[i].delayedJoinTimer = FALSE;

                            if (rowPtr[i].expirationTimer == 0)
                            {
                                cacheTimer.srcAddr = rowPtr[i].srcAddr;
                                cacheTimer.grpAddr = rowPtr[i].grpAddr;
                                RoutingPimSetTimer(
                                    node,
                                    interfaceId,
                                    MSG_ROUTING_PimDmDataTimeOut,
                                    (void*) &cacheTimer,
                                    (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                            }
                            /*  Set DataTimeout timer */
                            rowPtr[i].expirationTimer = node->getNodeTime() +
                                ROUTING_PIM_DM_DATA_TIMEOUT;
                        }
                    }
                }
            }
            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Unknown IGMP Event\n");
        }
    }
}

/**************************************************************************/
/*                            PIM HELLO                                   */
/**************************************************************************/

/*
*  FUNCTION     :RoutingPimDmHandleHelloPacket()
*  PURPOSE      :Process received hello packet
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmHandleHelloPacket(
    Node* node,
    NodeAddress srcAddr,
    RoutingPimHelloPacket* helloPkt,
    int pktSize,
    int interfaceIndex)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }

    RoutingPimInterface* thisInterface = NULL;
    RoutingPimNeighborListItem* neighbor = NULL;
    unsigned short holdTime = 0;
    unsigned int genId = 0;
    unsigned int drPriority = 0;
    RoutingPimDmNbrTimerInfo timerInfo;
    BOOL isDRPrioritypresent = FALSE;


    if (!RoutingPimProcessHelloPacket(helloPkt, pktSize, &holdTime, &genId,
                                      &drPriority, &isDRPrioritypresent))
    {
        /*  There should be some error in processing option */
        return;
    }

    thisInterface = &pim->interface[interfaceIndex];

    if (DEBUG_HELLO)
    {
        RoutingPimDmPrintNeibhborList(node);
    }

    /*  Search neighbor in neighbor list */
    RoutingPimSearchNeighborList(thisInterface->dmInterface->neighborList,
                                 srcAddr,
                                 &neighbor);

    /*  If we already have an entry for this neighbor */
    if (neighbor)
    {
        if (holdTime == 0)
        {
            if (DEBUG_HELLO)
            {
                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(srcAddr,
                                            srcStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout, "Node %u @ %s: Neighbor %s says "
                                "it's going down "
                                "by sending \"holdtime = 0\"\n",
                                node->nodeId, clockStr, srcStr);
            }

           /*
            *   Neighbor says it's going down by sending "holdtime = 0"
            *   Remove this neighbor from neighbor list
            */
            RoutingPimDmDeleteNeighbor(node, srcAddr, interfaceIndex);
            return;
        }

        if (neighbor->lastGenIdReceive != genId)
        {
            if (DEBUG_HELLO)
            {
                char neighborStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(neighbor->ipAddr,
                                            neighborStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout, "Node %u @ %s: Observed that neighbor %s"
                                " has been restarted\n",
                                node->nodeId, clockStr, neighborStr);
            }

            Message* newMsg = NULL;
            clocktype delay = 0;

            /*
             * RFC 3973 section 4.3.2  If a Hello message is received from
             * an active neighbor but with a different GenID, the
             * receiving router SHOULD send its own Hello message after a
             * random delay between 0 and Triggered_Hello_Delay.
             */

            delay = (clocktype) (RANDOM_nrand(pim->seed)
                                            % ROUTING_PIM_TRIGGERED_HELLO_DELAY);
            newMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    MULTICAST_PROTOCOL_PIM,
                                    MSG_ROUTING_PimScheduleTriggeredHello);

            MESSAGE_AddInfo(node, newMsg, sizeof(int), INFO_TYPE_PhyIndex);

            memcpy(MESSAGE_ReturnInfo(newMsg, INFO_TYPE_PhyIndex),
                       &interfaceIndex,
                       sizeof(int));

            MESSAGE_Send(node, newMsg, delay);

        }

        /*  Else, refresh neighbor timeout timer */
        neighbor->lastHelloReceive = node->getNodeTime();
    }
    else
    {
        if (holdTime == 0)
        {
            /*
            *   New Neighbor but it's going down, discard the hello message
            */

            if (DEBUG_HELLO)
            {
                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(srcAddr,
                                            srcStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout, "Node %u @ %s: New Neighbor %s says "
                                "it's going down "
                    "by sending \"holdtime = 0\"\n",
                                node->nodeId, clockStr, srcStr);
            }
            return;
        }
        RoutingPimDmForwardingTableRow* rowPtr;
        unsigned int i;

        /*  Neighbor doesn't have an entry, so build one */

        if (DEBUG_HELLO)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(srcAddr,
                                        srcStr);
            char addrStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                addrStr);
            fprintf(stdout, "Node %u @ %s: Found new neighbor %s on "
                            "interface %d(%s)\n",
                            node->nodeId, clockStr, srcStr, interfaceIndex,
                            addrStr);

            RoutingPimDmPrintNeibhborList(node);
        }

        /*  Allocate space for new neighbor */
        neighbor = (RoutingPimNeighborListItem*)
                   MEM_malloc(sizeof(RoutingPimNeighborListItem));
        memset(neighbor, 0, sizeof(RoutingPimNeighborListItem));

        neighbor->ipAddr = srcAddr;
        neighbor->lastGenIdReceive = genId;
        neighbor->lastHelloReceive = node->getNodeTime();
        neighbor->holdTime = holdTime;
        neighbor->NLTTimer = 0;

        /*  Add to neighbor list */
        ListInsert(node,
                   thisInterface->dmInterface->neighborList,
                   node->getNodeTime(),
                   (void*) neighbor);

        thisInterface->dmInterface->neighborCount++;
        pimDmData->stats.numNeighbor++;

        Message* newMsg = NULL;
        clocktype delay = 0;
        /*  Send a PIM Hello message to let it know this routers existence */
        //RFC 3973 section 4.3.2  If a Hello message is received from a new
        //neighbor, the receiving router SHOULD send its own Hello message
        //after a random delay between 0 and Triggered_Hello_Delay.
        delay = (clocktype) (RANDOM_nrand(pim->seed)
                                        % ROUTING_PIM_TRIGGERED_HELLO_DELAY);
        newMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    MULTICAST_PROTOCOL_PIM,
                                    MSG_ROUTING_PimScheduleTriggeredHello);

        MESSAGE_AddInfo(node, newMsg, sizeof(int), INFO_TYPE_PhyIndex);

        memcpy(MESSAGE_ReturnInfo(newMsg, INFO_TYPE_PhyIndex),
                   &interfaceIndex,
                   sizeof(int));

        MESSAGE_Send(node, newMsg, delay);

       /*
        *   If the receiving interface is a pruned outgoing interface of
        *   a (S, G) entry, that interface should be turned into forward
        *   state. A Graft must be sent to the upstream RPF neighbor in case
        *   the (S, G) entry trnsitions into forward state from pruned state.
        */

        rowPtr = (RoutingPimDmForwardingTableRow*)
                         BUFFER_GetData(&pimDmData->fwdTable.buffer);

        /*  For each entry in cache */
        for (i = 0; i < pimDmData->fwdTable.numEntries; i++)
        {
            ListItem* listItem;
            RoutingPimDmDownstreamListItem* downstreamInfo;
#ifdef ADDON_BOEINGFCS
       if ((pim->interface[interfaceIndex].interfaceType!=
            MULTICAST_CES_RPIM_INTERFACE) &&
            (pim->modeType == ROUTING_PIM_MODE_DENSE))
        {
           if (rowPtr[i].inIntf == thisInterface->ipAddress)
            {
                continue;
            }
        }
#else
            /*  Skip to next entry if this is incoming interface */
            if (rowPtr[i].inIntf == thisInterface->ipAddress)
            {
                continue;
            }
#endif
            /*  If this is the first neighbor on this interface */
            if ((thisInterface->dmInterface->neighborCount == 1)
                && (!RoutingPimDmIsDownstreamInterface(&rowPtr[i],
                         interfaceIndex)))
            {
                RoutingPimDmAddDownstream(node, &rowPtr[i], interfaceIndex);

                if (rowPtr[i].state == ROUTING_PIM_DM_PRUNE)
                {
                    RoutingPimDmDataTimeoutInfo cacheTimer;

                    /*  Change to ackpending  state */
                    rowPtr[i].state = ROUTING_PIM_DM_ACKPENDING;
                    /*  Send graft to upstream */
                    RoutingPimDmSendGraftPacket(node, rowPtr[i].srcAddr,
                            rowPtr[i].grpAddr);

                    if (rowPtr[i].expirationTimer == 0)
                    {
                        cacheTimer.srcAddr = rowPtr[i].srcAddr;
                        cacheTimer.grpAddr = rowPtr[i].grpAddr;
                        RoutingPimSetTimer(node,
                            interfaceIndex,
                            MSG_ROUTING_PimDmDataTimeOut,
                                (void*) &cacheTimer,
                                (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                    }

                    /*  Set DataTimeout timer */
                    rowPtr[i].expirationTimer = node->getNodeTime() +
                        ROUTING_PIM_DM_DATA_TIMEOUT;
                }

                continue;
            }

            /*  Find proper downstream */
            for (listItem = rowPtr[i].downstream->first; listItem;
                    listItem = listItem->next)
            {
                downstreamInfo = (RoutingPimDmDownstreamListItem*)
                                         listItem->data;

                if ((thisInterface->ipAddress == downstreamInfo->intfAddr)
                    && (downstreamInfo->isPruned == TRUE))
                {
                    /*  Set interface state in Forward state */
                    downstreamInfo->isPruned = FALSE;

                    if (rowPtr[i].state == ROUTING_PIM_DM_PRUNE)
                    {
                        RoutingPimDmDataTimeoutInfo cacheTimer;

                        /*  Change to forward state */
                        rowPtr[i].state = ROUTING_PIM_DM_ACKPENDING;
                        /*  Send graft to upstream */
                        RoutingPimDmSendGraftPacket(node, rowPtr[i].srcAddr,
                                rowPtr[i].grpAddr);

                        if (rowPtr[i].expirationTimer == 0)
                        {
                            cacheTimer.srcAddr = rowPtr[i].srcAddr;
                            cacheTimer.grpAddr = rowPtr[i].grpAddr;
                            RoutingPimSetTimer(
                                node,
                                interfaceIndex,
                                MSG_ROUTING_PimDmDataTimeOut,
                                (void*) &cacheTimer,
                                (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                        }
                        /*  Set DataTimeout timer */
                        rowPtr[i].expirationTimer = node->getNodeTime() +
                            ROUTING_PIM_DM_DATA_TIMEOUT;
                    }
                }
            }//end for
        }//end for
    }

    if (holdTime != (unsigned short)ROUTING_PIM_INFINITE_HOLDTIME
        && holdTime > 0)
    {
        if (neighbor->NLTTimer == 0)
    {
        /*  Send neighbor expiry timer to self for this neighbor */
        timerInfo.nbrAddr = srcAddr;
        timerInfo.interfaceIndex = interfaceIndex;

        RoutingPimSetTimer(node,
            interfaceIndex,
            MSG_ROUTING_PimDmNeighborTimeOut,
            &timerInfo,
            (clocktype) holdTime * SECOND);
    }

        neighbor->NLTTimer = node->getNodeTime() +
                    (clocktype) holdTime * SECOND;

    }

    if (DEBUG_HELLO)
    {
        RoutingPimDmPrintNeibhborList(node);
    }
}

/*
*  FUNCTION     :RoutingPimDmDeleteNeighbor()
*  PURPOSE      :Delete neighbor information
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmDeleteNeighbor(
    Node* node,
    NodeAddress nbrAddr,
        int interfaceId)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }

    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    RoutingPimNeighborListItem* neighbor = NULL;
    RoutingPimInterface* thisInterface = NULL;
    ListItem* listItem = NULL;
    unsigned int i = 0;

    thisInterface = &pim->interface[interfaceId];
    listItem = thisInterface->dmInterface->neighborList->first;

    ERROR_Assert(listItem, "Can't delete neighbor. List is empty\n");

    while (listItem)
    {
        neighbor = (RoutingPimNeighborListItem*) listItem->data;

        if (neighbor->ipAddr == nbrAddr)
        {
            /*  Got the neighbor, so delete it */
            ListGet(
                node,
                thisInterface->dmInterface->neighborList,
                listItem,
                TRUE,
                FALSE);

            thisInterface->dmInterface->neighborCount--;
            pimDmData->stats.numNeighbor--;

            if (DEBUG_HELLO)
            {
                char nbrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(nbrAddr,
                                            nbrStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout, "Node %u @ %s: Remove neighbor %s from "
                                "neighbor list\n",
                                node->nodeId, clockStr, nbrStr);
            }

            break;
        }

        listItem = listItem->next;
    }

   /*
    *   If this was last neighbor on this interface,
    *   set the connected subnet as Leaf
    */
    if (thisInterface->dmInterface->neighborCount == 0)
    {
        rowPtr = (RoutingPimDmForwardingTableRow*)
                         BUFFER_GetData(&pimDmData->fwdTable.buffer);
        RoutingPimDmDownstreamListItem* downstreamInfo = NULL;

       /*
        *   Clear this interface from outigoing interface lists of all
        *   forwarding table entries without attached group members
        */
        for (i = 0; i < pimDmData->fwdTable.numEntries; i++)
        {
            /*  Skip to next entry if this is incoming interface */
            if (rowPtr[i].inIntf == thisInterface->ipAddress)
            {
                continue;
            }

            /*  Find proper downstream */
            listItem = rowPtr[i].downstream->first;
            bool isElementDeleted = false;
            while (listItem)
            {
                isElementDeleted = false;
                downstreamInfo = (RoutingPimDmDownstreamListItem*)
                                         listItem->data;
#ifdef ADDON_BOEINGFCS
                if (downstreamInfo == NULL)
                {
                    continue;
                }
#endif
                if ((thisInterface->ipAddress == downstreamInfo->intfAddr)
                    && (RoutingPimDmHasLocalGroup(node, rowPtr[i].grpAddr,
                            interfaceId) == FALSE))

                {
                    if (DEBUG_HELLO)
                    {
                        char ipStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(thisInterface->ipAddress,
                                                    ipStr);
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                        fprintf(stdout, "Node %u @ %s: Remove interface "
                                        "%s from "
                                        "downstream interface list \n",
                                         node->nodeId, clockStr, ipStr);
                    }

                    /*  Remove this interface */
                    listItem = listItem->next;
                    isElementDeleted = true;
                    RoutingPimDmRemoveDownstream(
                        node,
                        &rowPtr[i],
                        interfaceId);
                    
                    /*
                    *   If all downstreams have been pruned
                    *   send prune to upstream
                    */

                    if (RoutingPimDmAllDownstreamPruned(&rowPtr[i]))
                    {
                        if (thisInterface->broadcastMode == FALSE)
                        {
                            RoutingPimDmDataTimeoutInfo cacheTimer;

                            rowPtr[i].state = ROUTING_PIM_DM_PRUNE;
                            RoutingPimDmSendJoinPrunePacket(
                                node,
                                rowPtr[i].srcAddr,
                                rowPtr[i].grpAddr, UPSTREAM,
                                ROUTING_PIM_PRUNE_TREE,
                                ROUTING_PIM_DM_PRUNE_TIMEOUT
                                );

                            rowPtr[i].delayedJoinTimer = FALSE;
                            rowPtr[i].graftRxmtTimer = FALSE;

                            if (rowPtr[i].expirationTimer == 0)
                            {
                                cacheTimer.srcAddr = rowPtr[i].srcAddr;
                                cacheTimer.grpAddr = rowPtr[i].grpAddr;
                                RoutingPimSetTimer(
                                    node,
                                    interfaceId,
                                    MSG_ROUTING_PimDmDataTimeOut,
                                    (void*) &cacheTimer,
                                    (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                            }
                            /*  Set DataTimeout timer */
                            rowPtr[i].expirationTimer = node->getNodeTime() +
                                ROUTING_PIM_DM_DATA_TIMEOUT;
                        }//end of if

                        break ;
                    }//end of if
                }//end of if
                if (!isElementDeleted)
                {
                    listItem = listItem->next;
                }
            }//end of for
        }//end of for
    }//end of if

    /*
     *   Now, update forwarding table in case this neighbor
     *   was previous upstream
     */
    rowPtr = (RoutingPimDmForwardingTableRow*)
                     BUFFER_GetData(&pimDmData->fwdTable.buffer);
    NodeAddress nextHop = 0;
    int interfaceIndex = 0;

    for (i = 0; i < pimDmData->fwdTable.numEntries; i++)
    {
        if (rowPtr[i].upstream == nbrAddr)
        {
            /*  Try to replace upstream by searching IP Forwarding table */
            NetworkGetInterfaceAndNextHopFromForwardingTable(node,
                    rowPtr[i].srcAddr, &interfaceIndex, &nextHop);

            /*  If there is no route back to source delete this entry */
            if (nextHop == (unsigned) NETWORK_UNREACHABLE)
            {
                if (DEBUG_HELLO)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr[i].srcAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr[i].grpAddr,
                                                grpStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout, "Node %u @ %s: Delete (%s, %s) "
                                    "entry from "
                                    "forwarding cache as there is no route"
                                    "back to source\n",
                                    node->nodeId, clockStr, srcStr, grpStr);
                }

#ifdef ADDON_DB
                //remove this entry from multicast_connectivity cache
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = rowPtr[i].grpAddr;
                strcpy(stats.rootNodeType,"Source");
                stats.rootNodeId= MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                          rowPtr[i].srcAddr);
                stats.upstreamNeighborId =
                                  MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                         rowPtr[i].upstream);
                stats.upstreamInterface =
                                  NetworkIpGetInterfaceIndexFromAddress(node,
                                                           rowPtr[i].inIntf);

                ListItem* listItem = rowPtr[i].downstream->first;

                while (listItem)
                {
                    RoutingPimDmDownstreamListItem* downstream;
                    downstream =
                            (RoutingPimDmDownstreamListItem*) listItem->data;

                    stats.outgoingInterface = downstream->interfaceIndex;

                    // delete entry from  multicast_connectivity cache
                    STATSDB_HandleMulticastConnDeletion(node, stats);

                    listItem = listItem->next;
                }
#endif

                /*  Delete cache entry */
                ListFree(node, rowPtr[i].downstream, FALSE);

                BUFFER_RemoveDataFromDataBuffer(&pimDmData->fwdTable.buffer,
                     (char*) &rowPtr[i],
                     sizeof(RoutingPimDmForwardingTableRow));

                // reset rowPtr since it now can potentially point to a new
                // place in memory after the call to RemoveDataFromDataBuffer
                rowPtr = (RoutingPimDmForwardingTableRow*)
                            BUFFER_GetData(&pimDmData->fwdTable.buffer);

                pimDmData->fwdTable.numEntries -= 1;
                i--;
            }
            else if (interfaceIndex != interfaceId)
            {
                RoutingPimDmDownstreamListItem* downstreamInfo;

                /*  Change upstream and remove this from downstream list */
                rowPtr[i].inIntf = NetworkIpGetInterfaceAddress(node,
                                           interfaceIndex);
                rowPtr[i].upstream = nextHop;
                rowPtr[i].assertState = Pim_Assert_NoInfo;
                rowPtr[i].assertTime = 0;

                if (DEBUG_HELLO)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr[i].srcAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr[i].grpAddr,
                                                grpStr);
                    char nbrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr,
                                                nbrStr);
                    char nextHopStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nextHop,
                                                nextHopStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout, "Node %u @ %s: Change upstream "
                                    "from %s to %s "
                                    "for (%s , %s) entry\n",
                                    node->nodeId, clockStr, nbrStr,
                                    nextHopStr,
                                    srcStr, grpStr);
                }

                /*  Find proper downstream */
                for (listItem = rowPtr[i].downstream->first; listItem;
                        listItem = listItem->next)
                {
                    downstreamInfo = (RoutingPimDmDownstreamListItem*)
                                             listItem->data;

                    if (rowPtr[i].inIntf == downstreamInfo->intfAddr)
                    {
                        if (DEBUG_HELLO)
                        {
                            char inIntfStr[MAX_STRING_LENGTH];
                            IO_ConvertIpAddressToString(rowPtr[i].inIntf,
                                                        inIntfStr);
                            char clockStr[MAX_STRING_LENGTH];
                            TIME_PrintClockInSecond(node->getNodeTime(), 
                                                    clockStr);

                            fprintf(stdout, "Node %u @ %s: Remove "
                                            "interface "
                                            "%s from downstream interface "
                                            "list \n",
                                            node->nodeId, clockStr,
                                            inIntfStr);
                        }

                        /*  Remove this interface */
                        RoutingPimDmRemoveDownstream(node, &rowPtr[i],
                                interfaceIndex);

                       /*
                        *   If all downstreams have been pruned
                        *   send prune to upstream
                        */
                        if (RoutingPimDmAllDownstreamPruned(&rowPtr[i]))
                        {
                            if (thisInterface->broadcastMode == FALSE)
                            {
                                RoutingPimDmDataTimeoutInfo cacheTimer;

                                rowPtr[i].state = ROUTING_PIM_DM_PRUNE;

                                RoutingPimDmSendJoinPrunePacket(
                                    node,
                                    rowPtr[i].srcAddr,
                                    rowPtr[i].grpAddr, UPSTREAM,
                                    ROUTING_PIM_PRUNE_TREE,
                                    ROUTING_PIM_DM_PRUNE_TIMEOUT);

                                if (rowPtr[i].expirationTimer == 0)
                                {
                                    cacheTimer.srcAddr = rowPtr[i].srcAddr;
                                    cacheTimer.grpAddr = rowPtr[i].grpAddr;
                                    RoutingPimSetTimer(
                                        node,
                                        interfaceId,
                                        MSG_ROUTING_PimDmDataTimeOut,
                                        (void*) &cacheTimer,
                                        (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                                }
                                /*  Set DataTimeout timer */
                                rowPtr[i].expirationTimer = node->getNodeTime() +
                                    ROUTING_PIM_DM_DATA_TIMEOUT;
                            }
                        }
                        break;
                    }
                }//end for
            }
            else
            {
                /*  Only upstream has been changed */
                rowPtr[i].upstream = nextHop;
                rowPtr[i].assertState = Pim_Assert_NoInfo;
                rowPtr[i].assertTime = 0;

                if (DEBUG_HELLO)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr[i].srcAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr[i].grpAddr,
                                                grpStr);
                    char nbrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr,
                                                nbrStr);
                    char nextHopStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nextHop,
                                                nextHopStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout, "Node %u @ %s: Change upstream from %s "
                                    "to %s for (%s, %s) entry\n",
                                    node->nodeId, clockStr, nbrStr,
                                    nextHopStr,
                                    srcStr, grpStr);
                }
            }
        }//if (rowPtr[i].upstream == nbrAddr)
        else
        {
            RoutingPimDmDownstreamListItem* downstreamInfo =
                RoutingPimDmGetDownstreamInfo(&rowPtr[i],
                                              interfaceId);

            //Current Assert Winner’s NeighborLiveness Timer Expires
            if (downstreamInfo
                && downstreamInfo->winnerAssertMetric.ipAddress == nbrAddr
                && downstreamInfo->assertState == Pim_Assert_ILostAssert)
            {
                downstreamInfo->assertTimerRunning = FALSE;
                downstreamInfo->assertTime = 0;
                downstreamInfo->assertState = Pim_Assert_NoInfo;
                downstreamInfo->winnerAssertMetric.metricPreference =
                    ROUTING_ADMIN_DISTANCE_DEFAULT;
                downstreamInfo->winnerAssertMetric.metric =
                    ROUTING_PIM_INFINITE_METRIC;

                if (rowPtr[i].state == ROUTING_PIM_DM_PRUNE)
                {
                    //Change the upstream state so that data packets
                    //can arrive leading to change in the assert state

                    RoutingPimDmDataTimeoutInfo cacheTimer;
                    rowPtr->state = ROUTING_PIM_DM_ACKPENDING;
                    RoutingPimDmSendGraftPacket(node,
                                                rowPtr[i].srcAddr,
                                                rowPtr[i].grpAddr);

                    if (rowPtr[i].expirationTimer == 0)
                    {
                        cacheTimer.srcAddr = rowPtr[i].srcAddr;
                        cacheTimer.grpAddr = rowPtr[i].grpAddr;
                        RoutingPimSetTimer(
                            node,
                            interfaceId,
                            MSG_ROUTING_PimDmDataTimeOut,
                            (void*) &cacheTimer,
                            (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                    }
                    /*  Set DataTimeout timer */
                    rowPtr[i].expirationTimer = node->getNodeTime() +
                        ROUTING_PIM_DM_DATA_TIMEOUT;
                }
            }
        }
    }//End for
}


/***************************************************************************/
/*                          PIM JOIN/PRUNE                                 */
/***************************************************************************/

/*
*  FUNCTION     :RoutingPimDmHandleJoinPrunePacket()
*  PURPOSE      :Process received Join/Prune packet
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/

void
RoutingPimDmHandleJoinPrunePacket(
        Node* node,
        NodeAddress srcAddr,
        RoutingPimDmJoinPrunePacket* joinPrunePkt,
        int interfaceId)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
    RoutingPimDmJoinPruneGroupInfo* grpInfo = NULL;
    RoutingPimDmForwardingTableRow* rowPtr = NULL;

    char* dataPtr = NULL;

    NodeAddress targetRtr = 0;
    int numGrp = 0;
    clocktype holdTime = 0;
    NodeAddress grpAddr = 0;
    RoutingPimEncodedSourceAddr  mcastSrcAddr;
    memset(&mcastSrcAddr, 0, sizeof(RoutingPimEncodedSourceAddr));
    int numJoinedSrc = 0;
    int numPrunedSrc = 0;

    targetRtr = getNodeAddressFromCharArray(
        joinPrunePkt->upstreamNbr.unicastAddr);
    numGrp = joinPrunePkt->numGroups;
    holdTime = joinPrunePkt->holdTime*  SECOND;

    dataPtr = (char*) joinPrunePkt + sizeof(RoutingPimDmJoinPrunePacket);

    if (targetRtr != pim->interface[interfaceId].ipAddress)
    {
        RoutingPimNeighborListItem* nbrItem;

        if (DEBUG_HELLO)
        {
            RoutingPimDmPrintNeibhborList(node);
        }

        /*  Ensure that target router is neighbor of this router */
        RoutingPimSearchNeighborList(
            pim->interface[interfaceId].dmInterface->neighborList,
            targetRtr, &nbrItem);

        if (nbrItem == NULL)
        {
            return;
        }

        while (numGrp--)
        {
            grpInfo = (RoutingPimDmJoinPruneGroupInfo*) dataPtr;
            dataPtr += sizeof(RoutingPimDmJoinPruneGroupInfo);
            grpAddr = grpInfo->groupAddr.groupAddr;
            numJoinedSrc = grpInfo->numJoinedSrc;
            numPrunedSrc = grpInfo->numPrunedSrc;

            while (numJoinedSrc--)
            {
                pimDmData->stats.joinReceived++;

                memcpy(&mcastSrcAddr, dataPtr,
                    sizeof(RoutingPimEncodedSourceAddr));
                dataPtr += sizeof(RoutingPimEncodedSourceAddr);

                if (DEBUG_JOIN_PRUNE)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(grpAddr,
                                                grpStr);
                    char joinSrcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(srcAddr,
                                            joinSrcStr);
                    char addrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(
                        NetworkIpGetInterfaceAddress(node, interfaceId),
                        addrStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout, "Node %u @ %s: I am not the target "
                                    "upstream of this join\n",
                                    node->nodeId, clockStr);

                    fprintf(stdout,"Node %u @ %s: Received Join from %s "
                                   "for (%s , %s) on interface %d(%s)\n",
                                   node->nodeId,
                                   clockStr,
                                   joinSrcStr,
                                   srcStr,
                                   grpStr,
                                   interfaceId,
                                   addrStr);
                }


                rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                             node,
                             mcastSrcAddr.sourceAddr,
                             grpAddr);

              /*
               *   Join suppression: If this router delaying a join for
               *   this (S, G) pair, cancel the join timer
               */
                if (rowPtr && rowPtr->delayedJoinTimer)
                {
                    if (DEBUG_JOIN_PRUNE)
                    {
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                                    srcStr);

                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(grpAddr,
                                                    grpStr);
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                        fprintf(stdout, "Node %u: Cancel delayed join "
                                        "timer for (%s, %s) at time %s\n",
                                        node->nodeId, srcStr, grpStr,
                                        clockStr);
                    }

                    rowPtr->delayedJoinTimer = FALSE;
                    rowPtr->lastJoinTimerEnd = 0;
                }

                if (rowPtr && (rowPtr->state == ROUTING_PIM_DM_PRUNE))
                {
                    rowPtr->joinSeen = TRUE;
                }
            }

            while (numPrunedSrc--)
            {
                pimDmData->stats.pruneReceived++;

                memcpy(&mcastSrcAddr, dataPtr,
                    sizeof(RoutingPimEncodedSourceAddr));
                dataPtr += sizeof(RoutingPimEncodedSourceAddr);

                if (DEBUG_JOIN_PRUNE)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(grpAddr,
                                                grpStr);
                    char pruneSrcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(srcAddr,
                                            pruneSrcStr);
                    char addrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(
                        NetworkIpGetInterfaceAddress(node, interfaceId),
                        addrStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout, "Node %u @ %s: I am not the target "
                                    "upstream of this prune\n",
                                    node->nodeId, clockStr);

                    fprintf(stdout,"Node %u @ %s: Received Prune from %s "
                                   "for (%s , %s) on interface %d(%s)\n",
                                   node->nodeId,
                                   clockStr,
                                   pruneSrcStr,
                                   srcStr,
                                   grpStr,
                                   interfaceId,
                                   addrStr);
                }

               /*
                *   If this is a point-to-point link then ignore message
                *   as this is not addressed to this router
                */

                rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                              node,
                              mcastSrcAddr.sourceAddr,
                              grpAddr);

               /*
                *   Prune soliciting join: Schedule a delayed join if
                *   the router have downstream dependents
                */
                if (rowPtr &&
                    (rowPtr->state == ROUTING_PIM_DM_FORWARD
                    || rowPtr->state == ROUTING_PIM_DM_ACKPENDING))
                {
                    RoutingPimDmDelayedJoinTimerInfo timerInfo;
                    clocktype delay;

                    BOOL scheduleTimer = FALSE;
                    delay =
                        (clocktype) (ROUTING_PIM_DM_RANDOM_DELAY_JOIN_TIMEOUT
                                  * RANDOM_erand(pimDmData->seed));

                    if (pim->interface[interfaceId].joinPruneSuppression
                                            == FALSE)
                    {
                        if (rowPtr->lastJoinTimerEnd == 0)
                        {
                            scheduleTimer = TRUE;
                        }
                    }
                    else
                    {
                        if (rowPtr->lastJoinTimerEnd != 0)
                        {
                            if ((rowPtr->lastJoinTimerEnd - node->getNodeTime())
                                < delay)
                            {
                                // Schedule new override timer
                                scheduleTimer = TRUE;
                            }
                        }
                        else
                        {
                            scheduleTimer = TRUE;
                        }
                    }

                    if (scheduleTimer == TRUE)
                    {
                        timerInfo.srcAddr = mcastSrcAddr.sourceAddr;
                        timerInfo.grpAddr = grpAddr;
                        timerInfo.interfaceIndex = interfaceId;
                        timerInfo.seqNo = rowPtr->delayedJoinTimerSeqNo + 1;
                        rowPtr->delayedJoinTimerSeqNo++;

                        RoutingPimSetTimer(node,
                        interfaceId,
                        MSG_ROUTING_PimDmScheduleJoin,
                        (void*) &timerInfo, delay);

                        rowPtr->lastJoinTimerEnd = delay + node->getNodeTime();
                        rowPtr->delayedJoinTimer = TRUE;
                    }
                }
            }//while
        }//while
    }//if
    /*  Else, this is the target router */
    else
    {
        RoutingPimDmDownstreamListItem* downstreamInfo;

        while (numGrp--)
        {
            grpInfo = (RoutingPimDmJoinPruneGroupInfo*) dataPtr;
            dataPtr += sizeof(RoutingPimDmJoinPruneGroupInfo);
            grpAddr = grpInfo->groupAddr.groupAddr;
            numJoinedSrc = grpInfo->numJoinedSrc;
            numPrunedSrc = grpInfo->numPrunedSrc;

            while (numJoinedSrc--)
            {
                pimDmData->stats.joinReceived++;
                memcpy(&mcastSrcAddr, dataPtr,
                    sizeof(RoutingPimEncodedSourceAddr));
                dataPtr += sizeof(RoutingPimEncodedSourceAddr);

                if (DEBUG_JOIN_PRUNE)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(grpAddr,
                                                grpStr);
                    char joinSrcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(srcAddr,
                                            joinSrcStr);
                    char addrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(
                        NetworkIpGetInterfaceAddress(node, interfaceId),
                        addrStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout,"Node %u @ %s: Received Join from %s "
                                   "for (%s , %s) on interface %d(%s)\n",
                                   node->nodeId,
                                   clockStr,
                                   joinSrcStr,
                                   srcStr,
                                   grpStr,
                                   interfaceId,
                                   addrStr);
                }

                rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                              node,
                              mcastSrcAddr.sourceAddr,
                              grpAddr);


                if (rowPtr == NULL)
                {
                    continue;
                }

                downstreamInfo = RoutingPimDmGetDownstreamInfo(
                                    rowPtr,
                    interfaceId);

                /*  Cancel delayed prune that the router have scheduled */
                if (downstreamInfo && downstreamInfo->delayedPruneActive)
                {
                    if (DEBUG_JOIN_PRUNE)
                    {
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                                    srcStr);

                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(grpAddr,
                                                    grpStr);

                        char intfStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(downstreamInfo->intfAddr,
                                                    intfStr);
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                        fprintf(stdout, "Node %u @ %s: Cancel delayed "
                                        "prune on interface %s for "
                                        "(%s , %s)\n",
                                        node->nodeId, clockStr,
                                        intfStr, srcStr, grpStr);
                    }

                    downstreamInfo->delayedPruneActive = FALSE;
                }

                BOOL sendGraft = FALSE;
                if (pim->interface[interfaceId].assertOptimization == FALSE)
                {
                if (downstreamInfo
                    && downstreamInfo->isPruned
                    && !downstreamInfo->assertState == Pim_Assert_ILostAssert)
                    {
                        sendGraft = TRUE;
                    }
                }
                else
                {
                    RoutingPimDmAssertSrcListItem* assertSrcListItem =
                        RoutingPimDmSearchAssertSrcPair(
                            node,
                            mcastSrcAddr.sourceAddr,
                            interfaceId);

                    if (assertSrcListItem
                        &&
                        !(assertSrcListItem->assertState ==
                                                Pim_Assert_ILostAssert))
                    {
                        sendGraft = TRUE;
                    }
                }

                if (sendGraft == TRUE)
                {
                    if (DEBUG_JOIN_PRUNE)
                    {
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                                    srcStr);

                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(grpAddr,
                                                    grpStr);

                        char intfStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(downstreamInfo->intfAddr,
                                                    intfStr);
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                        fprintf(stdout, "Node %u @ %s: Cancel prune on "
                                        "interface %s for (%s , %s)\n",
                                        node->nodeId, clockStr, intfStr,
                                        srcStr, grpStr);
                    }


                    downstreamInfo->isPruned = FALSE;

                    /* If Fwd tble entry transitions to Fwd state, 
                    send graft to upstream */
                    if (rowPtr->state == ROUTING_PIM_DM_PRUNE)
                    {
                        RoutingPimDmDataTimeoutInfo cacheTimer;

                        rowPtr->state = ROUTING_PIM_DM_ACKPENDING;
                        RoutingPimDmSendGraftPacket(node,
                                                    mcastSrcAddr.sourceAddr,
                                                    grpAddr);

                        if (rowPtr->expirationTimer == 0)
                        {
                            cacheTimer.srcAddr = rowPtr->srcAddr;
                            cacheTimer.grpAddr = rowPtr->grpAddr;
                            RoutingPimSetTimer(node,
                                interfaceId,
                                MSG_ROUTING_PimDmDataTimeOut,
                                (void*) &cacheTimer,
                                (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                        }
                        /*  Set DataTimeout timer */
                        rowPtr->expirationTimer = node->getNodeTime() +
                            ROUTING_PIM_DM_DATA_TIMEOUT;
                    }
                }
#ifdef CYBER_CORE
                NetworkDataIp* ip = (NetworkDataIp *)
                    node->networkData.networkVar;

                if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
                {
                    IAHEPCreateIgmpJoinLeaveMessage(
                        node,
                        rowPtr->grpAddr,
                        IGMP_MEMBERSHIP_REPORT_MSG);

                    if (IAHEP_DEBUG)
                    {
                        char addrStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->grpAddr, addrStr);
                        printf("\nRed Node: %d", node->nodeId);
                        printf("\tJoins Group: %s\n", addrStr);
                    }
                }
#endif

                BOOL sendAssert = FALSE;
                if (pim->interface[interfaceId].assertOptimization == FALSE)
                {
                    //- sending of pim assert after receive of join or prune- bug number 203.
                    if (downstreamInfo &&
                       (pim->interface[interfaceId].dmInterface->neighborCount > 1) &&
                       downstreamInfo->assertState == Pim_Assert_ILostAssert)
                    {
                        sendAssert = TRUE;
                    }
                }
                else
                {
                    RoutingPimDmAssertSrcListItem* assertSrcListItem =
                        RoutingPimDmSearchAssertSrcPair(
                            node,
                            mcastSrcAddr.sourceAddr,
                            interfaceId);

                    if (assertSrcListItem
                       &&
                       (pim->interface[interfaceId].dmInterface->neighborCount > 1)
                       &&
                       assertSrcListItem->assertState
                                            == Pim_Assert_ILostAssert)
                    {
                        sendAssert = TRUE;
                    }
                }

                if (sendAssert == TRUE)
                {
                    RoutingPimDmSendAssertPacket(node,
                        mcastSrcAddr.sourceAddr,
                        grpAddr,
                        interfaceId);
               }
            }

            while (numPrunedSrc--)
            {
                pimDmData->stats.pruneReceived++;

                memcpy(&mcastSrcAddr, dataPtr,
                    sizeof(RoutingPimEncodedSourceAddr));
                dataPtr += sizeof(RoutingPimEncodedSourceAddr);

                if (DEBUG_JOIN_PRUNE)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(grpAddr,
                                                grpStr);
                    char pruneSrcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(srcAddr,
                                            pruneSrcStr);
                    char addrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(
                        NetworkIpGetInterfaceAddress(node, interfaceId),
                        addrStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout,"Node %u @ %s: Received Prune from %s "
                                   "for (%s , %s) on interface %d(%s)\n",
                                   node->nodeId,
                                   clockStr,
                                   pruneSrcStr,
                                   srcStr,
                                   grpStr,
                                   interfaceId,
                                   addrStr);
                }

                rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                               node,
                               mcastSrcAddr.sourceAddr,
                               grpAddr);

                if (rowPtr == NULL)
                {
                    continue;
                }

                downstreamInfo = RoutingPimDmGetDownstreamInfo(rowPtr,
                    interfaceId);
                if (!downstreamInfo)
                {
                    continue;
                }

                BOOL sendAssert = FALSE;
                if (pim->interface[interfaceId].assertOptimization == FALSE)
                {
                    if (downstreamInfo
                       && (pim->interface[interfaceId].dmInterface->neighborCount
                            > 1)
                       && downstreamInfo->assertState == Pim_Assert_ILostAssert)
                    {
                        sendAssert = TRUE;
                    }
                }
                else
                {
                    RoutingPimDmAssertSrcListItem* assertSrcListItem =
                        RoutingPimDmSearchAssertSrcPair(
                            node,
                            mcastSrcAddr.sourceAddr,
                            interfaceId);

                    if (assertSrcListItem
                       &&
                       (pim->interface[interfaceId].dmInterface->neighborCount > 1)
                       &&
                       assertSrcListItem->assertState ==
                                    Pim_Assert_ILostAssert)
                    {
                        sendAssert = TRUE;
                    }
                }

                if (sendAssert == TRUE)
                {
                    RoutingPimDmSendAssertPacket(node,
                        mcastSrcAddr.sourceAddr,
                        grpAddr,
                        interfaceId);
                }


                if (downstreamInfo->isPruned)
                {
                    //RFC 3973 (sec 4.4.2.3) The Prune Timer (PT(S,G,I))
                    //SHOULD be reset Prune Timer to the holdtime contained
                    //in the Prune(S,G) message if it is greater than the
                    //current value.

                    clocktype currentPruneTimeout =
                        downstreamInfo->lastPruneReceived +
                        downstreamInfo->pruneTimer;
                    clocktype newPruneTimeout = node->getNodeTime() +
                        holdTime;

                    if (newPruneTimeout > currentPruneTimeout)
                    {
                        downstreamInfo->lastPruneReceived = node->getNodeTime();
                        downstreamInfo->pruneTimer = holdTime ;

                        if (downstreamInfo->pruneTimer !=
                            ROUTING_PIM_INFINITE_HOLDTIME)
                        {
                            RoutingPimDmPruneTimerInfo timerInfo;
                            /*  Set timer to time out prune on this interface */
                            timerInfo.srcAddr = mcastSrcAddr.sourceAddr;
                            timerInfo.grpAddr = grpAddr;
                            timerInfo.downstreamIntf = downstreamInfo->intfAddr;

                            RoutingPimSetTimer(
                                node,
                                interfaceId,
                                MSG_ROUTING_PimDmPruneTimeoutAlarm,
                                (void*) &timerInfo, holdTime);
                        }
                    }

                    continue;
                }

                /*  If this is a point-to-point link then prune immediately */
                if (pim->interface[interfaceId].dmInterface->neighborCount
                                    == 1)
                {
                    RoutingPimDmPruneTimerInfo timerInfo;

                    if (RoutingPimDmHasLocalGroup(node, rowPtr->grpAddr,
                                interfaceId))
                    {
                        continue;
                    }

                    if (DEBUG_JOIN_PRUNE)
                    {
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                                    srcStr);

                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(grpAddr,
                                                    grpStr);

                        char intfStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(downstreamInfo->intfAddr,
                                                    intfStr);
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                        fprintf(stdout, "Node %u @ %s: Prune downstream "
                                        "interface "
                                        "%s for (%s , %s)\n",
                                        node->nodeId, clockStr,
                                        intfStr, srcStr, grpStr);
                    }

                    /*  Prune this interface */
                    downstreamInfo->isPruned = TRUE;
                    downstreamInfo->lastPruneReceived = node->getNodeTime();
                    downstreamInfo->pruneTimer = holdTime;

#ifdef ADDON_DB
                    //remove this entry from multicast_connectivity cache
                    StatsDBConnTable::MulticastConnectivity stats;

                    stats.nodeId = node->nodeId;
                    stats.destAddr = rowPtr->grpAddr;
                    strcpy(stats.rootNodeType,"Source");
                    stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                                      node, rowPtr->srcAddr);
                    stats.outgoingInterface = interfaceId;
                    stats.upstreamNeighborId =
                                  MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                           rowPtr->upstream);
                    stats.upstreamInterface =
                                NetworkIpGetInterfaceIndexFromAddress(node,
                                                             rowPtr->inIntf);

                    // delete entry from  multicast_connectivity cache
                    STATSDB_HandleMulticastConnDeletion(node, stats);
#endif
                if (downstreamInfo->pruneTimer !=
                            ROUTING_PIM_INFINITE_HOLDTIME)
                {
                    /*  Set timer to time out prune on this interface */
                    timerInfo.srcAddr = mcastSrcAddr.sourceAddr;
                    timerInfo.grpAddr = grpAddr;
                    timerInfo.downstreamIntf = downstreamInfo->intfAddr;

                    RoutingPimSetTimer(
                        node,
                        interfaceId,
                        MSG_ROUTING_PimDmPruneTimeoutAlarm,
                        (void*) &timerInfo, holdTime);
                }

                  /*
                   *  If all downstreams have been pruned and
                   *  node is not part of Multicast group,
                   *   send prune to upstream
                   */
#ifdef ADDON_BOEINGFCS
                    if (RoutingPimDmAllDownstreamPruned(rowPtr))
#else
                    if (RoutingPimDmAllDownstreamPruned(rowPtr)
                        && ((!NetworkIpIsPartOfMulticastGroup(node,
                                                            rowPtr->grpAddr))
                        &&
                        ((pim->interface[interfaceId].dmInterface->neighborCount == 1)
                            || (rowPtr->joinSeen == FALSE))))
#endif
                    {
                        RoutingPimDmDataTimeoutInfo cacheTimer;

                        rowPtr->state = ROUTING_PIM_DM_PRUNE;
                        RoutingPimDmSendJoinPrunePacket(
                            node,
                            mcastSrcAddr.sourceAddr,
                            grpAddr,
                            UPSTREAM,
                            ROUTING_PIM_PRUNE_TREE,
                            ROUTING_PIM_DM_PRUNE_TIMEOUT
                            );

                        rowPtr->delayedJoinTimer = FALSE;
                        rowPtr->graftRxmtTimer = FALSE;

                        if (rowPtr->expirationTimer == 0)
                        {
                            cacheTimer.srcAddr = rowPtr->srcAddr;
                            cacheTimer.grpAddr = rowPtr->grpAddr;
                            RoutingPimSetTimer(
                                node,
                                interfaceId,
                                MSG_ROUTING_PimDmDataTimeOut,
                                (void*) &cacheTimer,
                                (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                        }
                        /*  Set DataTimeout timer */
                        rowPtr->expirationTimer = node->getNodeTime() +
                            ROUTING_PIM_DM_DATA_TIMEOUT;

#ifdef CYBER_CORE
                       NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;
                       if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
                       {
                           IAHEPCreateIgmpJoinLeaveMessage(
                               node,
                               rowPtr->grpAddr,
                               IGMP_LEAVE_GROUP_MSG);

                            if (IAHEP_DEBUG)
                            {
                                char addrStr[MAX_STRING_LENGTH];
                                IO_ConvertIpAddressToString(rowPtr->grpAddr, addrStr);
                                printf("\nRed Node: %d", node->nodeId);
                                printf("\tLeaves Group: %s\n", addrStr);
                            }
                       }
#endif
                    }
                }
                else
                {
                    RoutingPimDmDelayedPruneTimerInfo timerInfo;

                    //Don't prune, has local group
                    if (RoutingPimDmHasLocalGroup(node, rowPtr->grpAddr,
                                interfaceId))
                    {
                        continue;
                    }

                    /* Schedule delayed prune */
                    if (downstreamInfo->delayedPruneActive == FALSE)
                    {
                        downstreamInfo->delayedPruneActive = TRUE;
                        downstreamInfo->delayedPruneTimer = node->getNodeTime();
                        downstreamInfo->lastPruneReceived = node->getNodeTime();
                        downstreamInfo->pruneTimer = holdTime;

                        timerInfo.srcAddr = mcastSrcAddr.sourceAddr;
                        timerInfo.grpAddr = grpAddr;
                        timerInfo.downstreamIntf = downstreamInfo->intfAddr;

                        RoutingPimSetTimer(node,
                            interfaceId,
                            MSG_ROUTING_PimDmJoinTimeOut,
                            (void*) &timerInfo,
                            (clocktype)(ROUTING_PIM_DM_RANDOM_DELAY_JOIN_TIMEOUT +
                            ROUTING_PIM_DM_LAN_PROPAGATION_DELAY));

                    }
                }//else
            }//while
        }//while
    }//else
}

/*
*  FUNCTION     :RoutingPimDmHandleCompoundJoinPrunePacket()
*  PURPOSE      :Process received Join/Prune packet
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/

void
RoutingPimDmHandleCompoundJoinPrunePacket(
    Node *node,
    int interfaceId,
    RoutingPimDmJoinPruneGroupInfo* grpInfo,
    RoutingPimEncodedSourceAddr* encodedSrcAddr,
    RoutingPimEncodedUnicastAddr upstreamNbr,
    RoutingPimJoinPruneMessageType isJoinOrPrune,
    clocktype holdTime)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }

    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    NodeAddress targetRtr = 0;
    RoutingPimEncodedSourceAddr  mcastSrcAddr;
    memset(&mcastSrcAddr, 0, sizeof(RoutingPimEncodedSourceAddr));

    targetRtr = getNodeAddressFromCharArray(upstreamNbr.unicastAddr);
    if (targetRtr != pim->interface[interfaceId].ipAddress)
    {
        RoutingPimNeighborListItem* nbrItem = NULL;

        if (DEBUG_JOIN_PRUNE)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %u @ %s: I am not the target upstream of "
                            "this join/prune\n",
                            node->nodeId, clockStr);
        }

        if (DEBUG_HELLO)
        {
            RoutingPimDmPrintNeibhborList(node);
        }

        /*  Ensure that target router is neighbor of this router */
        RoutingPimSearchNeighborList(
            pim->interface[interfaceId].dmInterface->neighborList,
            targetRtr, &nbrItem);

        if (nbrItem == NULL)
        {
            return;
        }
        if (isJoinOrPrune == ROUTING_PIM_JOIN)
        {
            memcpy(&mcastSrcAddr, (void*) encodedSrcAddr,
                   sizeof(RoutingPimEncodedSourceAddr));
            rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                                 node,
                                 mcastSrcAddr.sourceAddr,
                                 grpInfo->groupAddr.groupAddr);

            /*
             *   Join suppression: If this router delaying a join for
             *   this (S, G) pair, cancel the join timer
             */
            if (rowPtr && rowPtr->delayedJoinTimer)
            {
                if (DEBUG_JOIN_PRUNE)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(grpInfo->groupAddr.groupAddr,
                                                grpStr);
                    fprintf(stdout, "Node %u @ %s: Cancel delayed join timer "
                                    "for (%s, %s)\n",
                                    node->nodeId, clockStr, srcStr,
                                    grpStr);
                }

                rowPtr->delayedJoinTimer = FALSE;
                rowPtr->lastJoinTimerEnd = 0;
            }

            if (rowPtr && (rowPtr->state == ROUTING_PIM_DM_PRUNE))
            {
                rowPtr->joinSeen = TRUE;
            }
        }
        else
        {
            memcpy(&mcastSrcAddr, (void*) encodedSrcAddr,
                   sizeof(RoutingPimEncodedSourceAddr));
            /*
             *   If this is a point-to-point link then ignore message
             *   as this is not addressed to this router
             */

            if (pim->interface[interfaceId].interfaceType ==
                    ROUTING_PIM_POINT_TO_POINT_INTERFACE)
            {
                return;
            }

            rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                          node,
                          mcastSrcAddr.sourceAddr,
                          grpInfo->groupAddr.groupAddr);

            /*
             *   Prune soliciting join: Schedule a delayed join if
             *   the router have downstream dependents
             */
            if (rowPtr && (rowPtr->state == ROUTING_PIM_DM_FORWARD))
            {
                RoutingPimDmDelayedJoinTimerInfo timerInfo;
                clocktype delay = 0;

                BOOL scheduleTimer = FALSE;

                if (pim->interface[interfaceId].joinPruneSuppression == FALSE)
                {
                    if (rowPtr->lastJoinTimerEnd == 0)
                    {
                        scheduleTimer = TRUE;
                    }
                }
                else
                {
                    if (rowPtr->lastJoinTimerEnd != 0)
                    {
                        if ((rowPtr->lastJoinTimerEnd - node->getNodeTime())
                                < delay)
                        {
                            // Schedule new override timer
                            scheduleTimer = TRUE;
                        }
                    }
                    else
                    {
                        scheduleTimer = TRUE;
                    }
                }

                if (scheduleTimer == TRUE)
                {
                    timerInfo.srcAddr = mcastSrcAddr.sourceAddr;
                    timerInfo.grpAddr = grpInfo->groupAddr.groupAddr;
                    timerInfo.interfaceIndex = interfaceId;
                    timerInfo.seqNo = rowPtr->delayedJoinTimerSeqNo + 1;
                    rowPtr->delayedJoinTimerSeqNo++;
                    RoutingPimSetTimer(node,
                                       interfaceId,
                                       MSG_ROUTING_PimDmScheduleJoin,
                                       (void*) &timerInfo,
                                       delay);

                    rowPtr->lastJoinTimerEnd = delay + node->getNodeTime();
                    rowPtr->delayedJoinTimer = TRUE;
                }
            }
        }
    }//if
    /*  Else, this is the target router */
    else
    {
        RoutingPimDmDownstreamListItem* downstreamInfo;
        if (isJoinOrPrune == ROUTING_PIM_JOIN)
        {
            memcpy(&mcastSrcAddr, (void*) encodedSrcAddr,
                   sizeof(RoutingPimEncodedSourceAddr));
            rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                              node,
                              mcastSrcAddr.sourceAddr,
                              grpInfo->groupAddr.groupAddr);

            if (rowPtr == NULL)
            {
                return;
            }
            downstreamInfo = RoutingPimDmGetDownstreamInfo(rowPtr,
                interfaceId);

            /*  Cancel delayed prune that the router have scheduled */
            if (downstreamInfo && downstreamInfo->delayedPruneActive)
            {
                if (DEBUG_JOIN_PRUNE)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char downstreamStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(downstreamInfo->intfAddr,
                                                downstreamStr);
                    fprintf(stdout, "Node %u @ %s: Cancel delayed prune on "
                                    "interface %s\n",
                                    node->nodeId, clockStr, downstreamStr);
                }

                downstreamInfo->delayedPruneActive = FALSE;
            }

            if (downstreamInfo && downstreamInfo->isPruned &&
                !downstreamInfo->assertState == Pim_Assert_ILostAssert)
            {
                if (DEBUG_JOIN_PRUNE)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char downstreamStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(downstreamInfo->intfAddr,
                                                downstreamStr);
                    fprintf(stdout, "Node %u @ %s: Cancel prune on "
                                "interface %u\n",
                                node->nodeId, clockStr,
                                downstreamInfo->intfAddr);
                }

                downstreamInfo->isPruned=FALSE ;

                /*  If Fwd tble entry transitions to Fwd state,
                 *  send graft to upstream */
                if (rowPtr->state == ROUTING_PIM_DM_PRUNE)
                {
                    RoutingPimDmDataTimeoutInfo cacheTimer;

                    rowPtr->state = ROUTING_PIM_DM_ACKPENDING;
                    RoutingPimDmSendGraftPacket(node,
                                               mcastSrcAddr.sourceAddr,
                                               grpInfo->groupAddr.groupAddr);

                    if (rowPtr->expirationTimer == 0)
                    {
                        cacheTimer.srcAddr = rowPtr->srcAddr;
                        cacheTimer.grpAddr = rowPtr->grpAddr;
                        RoutingPimSetTimer(node,
                            interfaceId,
                            MSG_ROUTING_PimDmDataTimeOut,
                            (void*) &cacheTimer,
                            (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                    }
                    /*  Set DataTimeout timer */
                    rowPtr->expirationTimer = node->getNodeTime() +
                        ROUTING_PIM_DM_DATA_TIMEOUT;
                }
            }

            //sending of pim assert after receive of join or prune
            if (downstreamInfo &&
                pim->interface[interfaceId].interfaceType !=
                    ROUTING_PIM_POINT_TO_POINT_INTERFACE &&
                downstreamInfo->assertState == Pim_Assert_ILostAssert)
            {
                RoutingPimDmSendAssertPacket(node,
                        mcastSrcAddr.sourceAddr,
                        grpInfo->groupAddr.groupAddr,
                        interfaceId);
            }
        }
        else
        {
            memcpy(&mcastSrcAddr, (void*) encodedSrcAddr,
                   sizeof(RoutingPimEncodedSourceAddr));
            rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                               node,
                               mcastSrcAddr.sourceAddr,
                               grpInfo->groupAddr.groupAddr);

            if (rowPtr == NULL)
            {
                return;
            }

            downstreamInfo = RoutingPimDmGetDownstreamInfo(rowPtr,
                interfaceId);
            if (!downstreamInfo)
            {
                return;
            }

            if (downstreamInfo &&
               pim->interface[interfaceId].interfaceType ==
                    ROUTING_PIM_BROADCAST_INTERFACE &&
               downstreamInfo->assertState == Pim_Assert_ILostAssert)
            {
                RoutingPimDmSendAssertPacket(node,
                    mcastSrcAddr.sourceAddr,
                    grpInfo->groupAddr.groupAddr,
                    interfaceId);
            }

            if (downstreamInfo->isPruned)
            {
                //RFC 3973 (sec 4.4.2.3) The Prune Timer (PT(S,G,I))
                //SHOULD be reset Prune Timer to the holdtime contained
                //in the Prune(S,G) message if it is greater than the
                //current value.

                clocktype currentPruneTimeout =
                    downstreamInfo->lastPruneReceived +
                    downstreamInfo->pruneTimer;
                clocktype newPruneTimeout = node->getNodeTime() +
                    holdTime;

                if (newPruneTimeout > currentPruneTimeout)
                {
                    downstreamInfo->lastPruneReceived = node->getNodeTime();
                    downstreamInfo->pruneTimer = holdTime ;

                    if (downstreamInfo->pruneTimer !=
                        ROUTING_PIM_INFINITE_HOLDTIME)
                    {
                        RoutingPimDmPruneTimerInfo timerInfo;
                        /*  Set timer to time out prune on this interface */
                        timerInfo.srcAddr = mcastSrcAddr.sourceAddr;
                        timerInfo.grpAddr = grpInfo->groupAddr.groupAddr;
                        timerInfo.downstreamIntf = downstreamInfo->intfAddr;

                        RoutingPimSetTimer(
                            node,
                            interfaceId,
                            MSG_ROUTING_PimDmPruneTimeoutAlarm,
                            (void*) &timerInfo, holdTime);
                    }
                }
                return;
            }//end of if

            /* If this is a point-to-point link then prune immediately */
            if (pim->interface[interfaceId].interfaceType ==
                    ROUTING_PIM_POINT_TO_POINT_INTERFACE)
            {
                RoutingPimDmPruneTimerInfo timerInfo;

                if (RoutingPimDmHasLocalGroup(node, rowPtr->grpAddr,
                            interfaceId))
                {
                    return;
                }

                if (DEBUG_JOIN_PRUNE)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char downstreamStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(downstreamInfo->intfAddr,
                                                downstreamStr);

                    fprintf(stdout, "Node %u @ %s: Prune downstream "
                                    "interface %s\n",
                                    node->nodeId, clockStr, downstreamStr);
                }

                /*  Prune this interface */
                downstreamInfo->isPruned = TRUE;
                downstreamInfo->lastPruneReceived = node->getNodeTime();
                downstreamInfo->pruneTimer = holdTime;

                if (downstreamInfo->pruneTimer !=
                        ROUTING_PIM_INFINITE_HOLDTIME)
                {
                    /*  Set timer to time out prune on this interface */
                    timerInfo.srcAddr = mcastSrcAddr.sourceAddr;
                    timerInfo.grpAddr = grpInfo->groupAddr.groupAddr;
                    timerInfo.downstreamIntf = downstreamInfo->intfAddr;

                    RoutingPimSetTimer(
                        node,
                        interfaceId,
                        MSG_ROUTING_PimDmPruneTimeoutAlarm,
                        (void*) &timerInfo, holdTime);
                }

                /*
                *  If all downstreams have been pruned and
                *  node is not part of Multicast group,
                *   send prune to upstream
                */
#ifdef ADDON_BOEINGFCS
                if (RoutingPimDmAllDownstreamPruned(rowPtr))
#else
               if (RoutingPimDmAllDownstreamPruned(rowPtr)
                && ((!NetworkIpIsPartOfMulticastGroup(
                        node, rowPtr->grpAddr))
                    && (((pim->interface[interfaceId].dmInterface->neighborCount
                                    == 1))
                        || (rowPtr->joinSeen == FALSE))))
#endif
                {
                    if (pim->interface[interfaceId].broadcastMode == FALSE)
                    {
                            RoutingPimDmDataTimeoutInfo cacheTimer;

                            rowPtr->state = ROUTING_PIM_DM_PRUNE;
                            RoutingPimDmSendJoinPrunePacket(
                                node,
                                mcastSrcAddr.sourceAddr,
                                grpInfo->groupAddr.groupAddr,
                                UPSTREAM,
                                ROUTING_PIM_PRUNE_TREE,
                                ROUTING_PIM_DM_PRUNE_TIMEOUT);
            
                            rowPtr->delayedJoinTimer = FALSE;
                            rowPtr->graftRxmtTimer = FALSE;

                            if (rowPtr->expirationTimer == 0)
                            {
                                cacheTimer.srcAddr = rowPtr->srcAddr;
                                cacheTimer.grpAddr = rowPtr->grpAddr;
                                RoutingPimSetTimer(
                                    node,
                                    interfaceId,
                                    MSG_ROUTING_PimDmDataTimeOut,
                                    (void*) &cacheTimer,
                                    (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                            }
                            /*  Set DataTimeout timer */
                            rowPtr->expirationTimer = node->getNodeTime() +
                                ROUTING_PIM_DM_DATA_TIMEOUT;

                    }
                }//end of if
                else
                {
                    RoutingPimDmDelayedPruneTimerInfo timerInfo;

                    //Don't prune, has local group
                    if (RoutingPimDmHasLocalGroup(node, rowPtr->grpAddr,
                                interfaceId))
                    {
                        return;
                    }

                    downstreamInfo->delayedPruneActive = TRUE;
                    downstreamInfo->delayedPruneTimer = node->getNodeTime();
                    downstreamInfo->lastPruneReceived = node->getNodeTime();
                    downstreamInfo->pruneTimer = holdTime;

                    timerInfo.srcAddr = mcastSrcAddr.sourceAddr;
                    timerInfo.grpAddr = grpInfo->groupAddr.groupAddr;
                    timerInfo.downstreamIntf = downstreamInfo->intfAddr;

                    RoutingPimSetTimer(node,
                        interfaceId,
                        MSG_ROUTING_PimDmJoinTimeOut,
                        (void*) &timerInfo,
                        (clocktype)(ROUTING_PIM_DM_RANDOM_DELAY_JOIN_TIMEOUT +
                        ROUTING_PIM_DM_LAN_PROPAGATION_DELAY));
                }//else
            }//end of if
        }//else target router
    }
}
/*
*  FUNCTION     :RoutingPimDmAllDownstreamPruned()
*  PURPOSE      :Check to see whether all downstream is pruned
*  ASSUMPTION   :None
*  RETURN VALUE :BOOL
 */
BOOL
RoutingPimDmAllDownstreamPruned(
    RoutingPimDmForwardingTableRow* rowPtr)
{
    ListItem* listItem = rowPtr->downstream->first;
    RoutingPimDmDownstreamListItem* downstreamInfo = NULL;

    while (listItem)
    {
        downstreamInfo = (RoutingPimDmDownstreamListItem*) listItem->data;

        if (downstreamInfo->isPruned == FALSE)
        {
            return FALSE;
        }
        listItem = listItem->next;
    }

    return TRUE;
}

/*
*  FUNCTION     :RoutingPimDmSendJoinPrunePacket()
*  PURPOSE      :Send Join/Prune packet to upstream. The parameter nonRPFNbr
*                will be used when we are sending prune other than upstream.
*                When calling this functn for sending prune to upstream pass
*                'UPSTREAM' as argument. Otherwise pass desired node's IP.
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/

/*
*  NOTE:
*  PIM prune message storms can be generated if an implementation responds to
*  received multicast packets aggressively, e.g by sending one prune for each
*  data packet matching a negative cache entry. Implementation SHOULD either
*  respond to such packets in a very conservative rate-limited manner, or not
*  responding to such packets at all and rely on the expiration & recreation
*  of the entry to generate a prune. The latter is sufficient in most cases
*  when the probability of prune message loss is low.
*/
void RoutingPimDmSendJoinPrunePacket(Node* node, NodeAddress srcAddr,
        NodeAddress grpAddr, NodeAddress nonRPFNbr,
        RoutingPimJoinPruneActionType actionType,
        clocktype holdtime)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
#ifdef HITL_INTERFACE
    if (actionType == ROUTING_PIM_PRUNE_TREE) return;
#endif

    Message* joinPruneMsg = NULL;
    RoutingPimDmJoinPrunePacket* joinPrunePkt = NULL;
    RoutingPimDmJoinPruneGroupInfo grpInfo;
    memset(&grpInfo, 0, sizeof(RoutingPimDmJoinPruneGroupInfo));
    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    char* dataPtr = NULL;
    int size = 0;
    NodeAddress srcIntfAddr = 0;
    int interfaceId = 0;

    RoutingPimEncodedSourceAddr  encodedSrcAddr;
    memset(&encodedSrcAddr, 0, sizeof(RoutingPimEncodedSourceAddr));

    NodeAddress upstreamNeighbor = 0;

    rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node, srcAddr, grpAddr);
    ERROR_Assert(rowPtr != NULL, "Attempt to sent join/Prune "
                                 "for an unknown (S , G) pair\n");

    /*  Don't send prune if source is directly connected */
    if (((nonRPFNbr == UPSTREAM)
        && ((rowPtr->srcAddr == rowPtr->upstream)
            || (rowPtr->upstream == NEXT_HOP_ME)))
        || (nonRPFNbr == rowPtr->srcAddr))
    {
        if (DEBUG_JOIN_PRUNE)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                        srcStr);
            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                        grpStr);

            fprintf(stdout, "Node %u @ %s: Source %s is directly connected, "
                            "so don't send PRUNE for (%s , %s)\n",
                            node->nodeId, clockStr, srcStr, srcStr, grpStr);
        }

        return;
    }

    /*  Send Join or Prune for anly one (S, G) pair */
    joinPruneMsg = MESSAGE_Alloc(node,
                                  NETWORK_LAYER,
                                  MULTICAST_PROTOCOL_PIM,
                                  MSG_ROUTING_PimPacket);

    size = sizeof(RoutingPimDmJoinPrunePacket)
                   + sizeof(RoutingPimDmJoinPruneGroupInfo)
                   + sizeof(RoutingPimEncodedSourceAddr);

    MESSAGE_PacketAlloc(node, joinPruneMsg, size, TRACE_PIM);
    joinPrunePkt = (RoutingPimDmJoinPrunePacket*)
                           MESSAGE_ReturnPacket(joinPruneMsg);

    RoutingPimCreateCommonHeader(&joinPrunePkt->commonHeader,
                                 ROUTING_PIM_JOIN_PRUNE);

    joinPrunePkt->upstreamNbr.addressFamily = 1;/*for IPv4 */
    joinPrunePkt->upstreamNbr.encodingType = 0;

    if (nonRPFNbr == UPSTREAM)
    {
        upstreamNeighbor = rowPtr->upstream;
    }
    else
    {
        upstreamNeighbor = nonRPFNbr;
    }

    setNodeAddressInCharArray(joinPrunePkt->upstreamNbr.unicastAddr,
                              upstreamNeighbor);


    joinPrunePkt->reserved = 0;

    /*  Only one group per packet */
    joinPrunePkt->numGroups = 1;

    joinPrunePkt->holdTime = (unsigned short) (holdtime / SECOND);

    /*  Now, set group information */
    grpInfo.groupAddr.addressFamily = 1;
    grpInfo.groupAddr.encodingType = 0;
    grpInfo.groupAddr.reserved = 0;
    grpInfo.groupAddr.maskLength = ROUTING_PIM_MASK_LENGTH;
    grpInfo.groupAddr.groupAddr = rowPtr->grpAddr;


    if (actionType == ROUTING_PIM_JOIN_TREE)
    {
        grpInfo.numJoinedSrc = 1;
        grpInfo.numPrunedSrc = 0;
    }
    else if (actionType == ROUTING_PIM_PRUNE_TREE)
    {
        grpInfo.numJoinedSrc = 0;
        grpInfo.numPrunedSrc = 1;
    }

    /* Set the encoded join source field */
    encodedSrcAddr.addressFamily = 1;
    encodedSrcAddr.encodingType = 0;
    encodedSrcAddr.rpSimEsa = 0;//all bits are 0 for PIM-DM
    encodedSrcAddr.maskLength = ROUTING_PIM_MASK_LENGTH;
    encodedSrcAddr.sourceAddr = rowPtr->srcAddr;

    dataPtr = ((char*) joinPrunePkt) + sizeof(RoutingPimDmJoinPrunePacket);
    memcpy(dataPtr, &grpInfo, sizeof(RoutingPimDmJoinPruneGroupInfo));

    dataPtr += sizeof(RoutingPimDmJoinPruneGroupInfo);

    /*  Put source address */
    memcpy(dataPtr, &encodedSrcAddr, sizeof(RoutingPimEncodedSourceAddr));

    if (nonRPFNbr == UPSTREAM)
    {
        srcIntfAddr = rowPtr->inIntf;
        interfaceId = NetworkIpGetInterfaceIndexFromAddress(node,
                              rowPtr->inIntf);
    }
    else
    {
        interfaceId = NetworkGetInterfaceIndexForDestAddress(node, nonRPFNbr);
        srcIntfAddr = NetworkIpGetInterfaceAddress(node, interfaceId);
    }

    if (interfaceId >= 0)
    {
        /*  Now send packet out through this interface */
        NetworkIpSendRawMessageToMacLayer(
                                      node,
                                      MESSAGE_Duplicate(node, joinPruneMsg),
                                      srcIntfAddr,
                                      ALL_PIM_ROUTER,
                                      IPTOS_PREC_INTERNETCONTROL,
                                      IPPROTO_PIM,
                                      1,
                                      interfaceId,
                                      ANY_DEST);

        if (actionType == ROUTING_PIM_JOIN_TREE)
        {
            pimDmData->stats.joinSent++;
        }
        else if (actionType == ROUTING_PIM_PRUNE_TREE)
        {
            pimDmData->stats.pruneSent++;
        }

        pimDmData->stats.joinPruneSent++;
#ifdef ADDON_DB
        pimDmData->dmSummaryStats.m_NumJoinPruneSent++;
#endif

        if (DEBUG_JOIN_PRUNE)
        {
            char clockStr[MAX_STRING_LENGTH];
            char debugBuff[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            strcpy(debugBuff,
                    (actionType == ROUTING_PIM_JOIN_TREE ? "JOIN" : "PRUNE"));

            char upstreamStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(upstreamNeighbor,
                                        upstreamStr);

            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                        srcStr);
            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                        grpStr);
            char addrStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(
                        NetworkIpGetInterfaceAddress(node, interfaceId),
                        addrStr);

            fprintf(stdout, "Node %u @ %s: Sending %s to %s for (%s , %s) "
                            "on interface %d(%s)\n",
                            node->nodeId, clockStr, debugBuff, upstreamStr,
                            srcStr, grpStr, interfaceId, addrStr);
        }
    }

    MESSAGE_Free(node, joinPruneMsg);
}

/***************************************************************************/
/*                             PIM GRAFT                                   */
/***************************************************************************/

/*
*  FUNCTION     :RoutingPimDmHandleGraftPacket()
*  PURPOSE      :Handle received graft packet
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmHandleGraftPacket(
    Node* node,
    NodeAddress srcAddr,
    RoutingPimDmGraftPacket* graftPkt,
    int pktSize,
    int interfaceIndex)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
    Message* graftAckMsg = NULL;
    RoutingPimDmJoinPruneGroupInfo* grpInfo = NULL;
    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    RoutingPimDmDownstreamListItem* downstreamInfo = NULL;
    RoutingPimInterface* thisInterface = NULL;

    char* dataPtr = NULL;

    char* upstreamPart = NULL;
    RoutingPimEncodedUnicastAddr* encodedUpstream = NULL;

    RoutingPimEncodedSourceAddr mcastSrcAddr;
    memset(&mcastSrcAddr, 0, sizeof(RoutingPimEncodedSourceAddr));
    RoutingPimEncodedGroupAddr mcastGrpAddr;
    memset(&mcastGrpAddr, 0, sizeof(RoutingPimEncodedGroupAddr));

    thisInterface = &pim->interface[interfaceIndex];

    dataPtr = (char*) graftPkt;
    dataPtr += sizeof(RoutingPimDmGraftAckPacket);

    grpInfo = (RoutingPimDmJoinPruneGroupInfo*) dataPtr;
    dataPtr += sizeof(RoutingPimDmJoinPruneGroupInfo);

    /* Get Source and Group address */
    memcpy(&mcastGrpAddr, &grpInfo->groupAddr ,
            sizeof(RoutingPimEncodedGroupAddr));
    memcpy(&mcastSrcAddr, dataPtr, sizeof(RoutingPimEncodedSourceAddr));

    if (DEBUG_GRAFT)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char graftSrcStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(srcAddr,
                                    graftSrcStr);

        char srcStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                    srcStr);

        char grpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(mcastGrpAddr.groupAddr,
                                    grpStr);

        fprintf(stdout, "Node %u @ %s: Received GRAFT from %s for "
                        "(%s , %s)\n",
                        node->nodeId, clockStr,
                        graftSrcStr, srcStr, grpStr);
    }


    /*  Find a matching entry in cache */
    rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node,
                                                     mcastSrcAddr.sourceAddr,
                                                     mcastGrpAddr.groupAddr);

    if (!rowPtr)
    {
        if (DEBUG_GRAFT)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                        srcStr);

            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                        grpStr);
            fprintf(stdout, "Node %u @ %s: Doesn't find a matching entry"
                            "for (%s , %s).So, Ignoring this Graft"
                            "\n",node->nodeId, clockStr, srcStr, grpStr);
        }

        return;
    }

    /*  If existing interface */
    if (RoutingPimDmIsDownstreamInterface(rowPtr, interfaceIndex))
    {
        downstreamInfo = RoutingPimDmGetDownstreamInfo(rowPtr,
                                 interfaceIndex);

        BOOL pruneDownstream = FALSE;
        if (pim->interface[interfaceIndex].assertOptimization == FALSE)
        {
            if (downstreamInfo->assertState != Pim_Assert_ILostAssert)
            {
                pruneDownstream = TRUE;
            }
        }
        else
        {
            RoutingPimDmAssertSrcListItem* assertSrcListItem
            = RoutingPimDmSearchAssertSrcPair(node,
                                              rowPtr->srcAddr,
                                              interfaceIndex);

            if (assertSrcListItem
                &&
                assertSrcListItem->assertState != Pim_Assert_ILostAssert)
            {
                pruneDownstream = TRUE;
            }
        }

        if (pruneDownstream == TRUE)
        {
            downstreamInfo->isPruned = FALSE;
            downstreamInfo->delayedPruneActive=FALSE;

#ifdef ADDON_DB
            StatsDBConnTable::MulticastConnectivity stats;

            stats.nodeId = node->nodeId;
            stats.destAddr = rowPtr->grpAddr;
            strcpy(stats.rootNodeType,"Source");
            stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                rowPtr->srcAddr);
            stats.outgoingInterface = downstreamInfo->interfaceIndex;

            stats.upstreamNeighborId = MAPPING_GetNodeIdFromInterfaceAddress(
                                                         node,rowPtr->upstream);
            stats.upstreamInterface = NetworkIpGetInterfaceIndexFromAddress(node,
                                                                 rowPtr->inIntf);
            //if we are using database, update the multicast connection table
            STATSDB_HandleMulticastConnCreation(node,stats);
#endif
        }
    }
    /* Else create new one */
    else
    {
        RoutingPimDmAddDownstream(node, rowPtr, interfaceIndex);
        downstreamInfo = RoutingPimDmGetDownstreamInfo(rowPtr,
                                 interfaceIndex);
    }

    /*  Send GraftAck to sender */
    graftAckMsg = MESSAGE_Alloc(node, NETWORK_LAYER,
                           MULTICAST_PROTOCOL_PIM, MSG_ROUTING_PimPacket);
    MESSAGE_PacketAlloc(node, graftAckMsg, pktSize, TRACE_PIM);
    dataPtr = MESSAGE_ReturnPacket(graftAckMsg);

    /*  Copy graft packet and then change it's type */
    memcpy(dataPtr, graftPkt, pktSize);


    upstreamPart = dataPtr + sizeof(RoutingPimCommonHeaderType);
    encodedUpstream = (RoutingPimEncodedUnicastAddr*)upstreamPart;
    setNodeAddressInCharArray(encodedUpstream->unicastAddr , srcAddr);


    RoutingPimCreateCommonHeader((RoutingPimCommonHeaderType*) dataPtr,
            ROUTING_PIM_GRAFT_ACK);

    if (interfaceIndex >= 0)
    {
        NetworkIpSendRawMessageToMacLayer(
                                      node,
                                      graftAckMsg,
                                      thisInterface->ipAddress,
                                      srcAddr,
                                      IPTOS_PREC_INTERNETCONTROL,
                                      IPPROTO_PIM,
                                      1,
                                      interfaceIndex,
                                      srcAddr);
        pimDmData->stats.graftAckSent++;
#ifdef ADDON_DB
        pimDmData->dmSummaryStats.m_NumGraftAckSent++;
#endif
        if (DEBUG_GRAFT)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

            char graftAckDestStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(srcAddr,
                                        graftAckDestStr);

            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                        srcStr);

            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                        grpStr);

            fprintf(stdout, "Node %u @ %s: Send GraftAck to %s for "
                            "(%s , %s)\n",
                            node->nodeId, clockStr,
                            graftAckDestStr, srcStr, grpStr);
        }
    }

    /*  If Fwd tble entry transitions to Fwd state, send graft to upstream */
    if (rowPtr->state == ROUTING_PIM_DM_PRUNE)
    {
        RoutingPimDmDataTimeoutInfo cacheTimer;

        rowPtr->state = ROUTING_PIM_DM_ACKPENDING;
        RoutingPimDmSendGraftPacket(node,
                                    mcastSrcAddr.sourceAddr,
                                    mcastGrpAddr.groupAddr);


        if (rowPtr->expirationTimer == 0)
        {
            cacheTimer.srcAddr = rowPtr->srcAddr;
            cacheTimer.grpAddr = rowPtr->grpAddr;
            RoutingPimSetTimer(node,
                interfaceIndex,
                MSG_ROUTING_PimDmDataTimeOut,
                (void*) &cacheTimer,
                (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
        }
        /*  Set DataTimeout timer */
        rowPtr->expirationTimer = node->getNodeTime() +
            ROUTING_PIM_DM_DATA_TIMEOUT;
    }

    BOOL sendAssert = FALSE;
    if (pim->interface[interfaceIndex].assertOptimization == FALSE)
    {
        if (downstreamInfo &&
            pim->interface[interfaceIndex].dmInterface->neighborCount > 1
            && downstreamInfo->assertState == Pim_Assert_ILostAssert)
        {
            sendAssert = TRUE;
        }
    }
    else
    {
        RoutingPimDmAssertSrcListItem* assertSrcListItem
        = RoutingPimDmSearchAssertSrcPair(node,
                                             rowPtr->srcAddr,
                                             interfaceIndex);

        if (assertSrcListItem
           &&
           pim->interface[interfaceIndex].dmInterface->neighborCount > 1
           &&
           assertSrcListItem->assertState ==
                                Pim_Assert_ILostAssert)
        {
            sendAssert = TRUE;
        }
    }

    if (sendAssert == TRUE)
    {
        RoutingPimDmSendAssertPacket(node,
            rowPtr->srcAddr,
            rowPtr->grpAddr,
            interfaceIndex);
    }

#ifdef CYBER_CORE
    NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;
    if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
    {
        IAHEPCreateIgmpJoinLeaveMessage(
            node,
            mcastGrpAddr.groupAddr,
            IGMP_MEMBERSHIP_REPORT_MSG);

        if (IAHEP_DEBUG)
        {
            char addrStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(mcastGrpAddr.groupAddr, addrStr);
            printf("\nRed Node: %d", node->nodeId);
            printf("\tJoins Group: %s\n", addrStr);
        }
    }
#endif
}


/*
*  FUNCTION     :RoutingPimDmSendGraftPacket()
*  PURPOSE      :Send graft packet to upstream
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmSendGraftPacket(
    Node* node,
    NodeAddress srcAddr,
        NodeAddress grpAddr)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
    Message* graftMsg = NULL;
    RoutingPimDmGraftPacket* graftPkt = NULL;

    RoutingPimDmJoinPruneGroupInfo grpInfo;
    memset(&grpInfo, 0, sizeof(RoutingPimDmJoinPruneGroupInfo));
    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    int interfaceIndex = 0;
    char* dataPtr = NULL;
    RoutingPimDmGraftTimerInfo graftTimer;
    memset(&graftTimer, 0, sizeof(RoutingPimDmGraftTimerInfo));

    RoutingPimEncodedSourceAddr encodedSrcAddr;
    memset(&encodedSrcAddr, 0, sizeof(RoutingPimEncodedSourceAddr));

    rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node, srcAddr, grpAddr);
    ERROR_Assert(rowPtr, "Can't send graft, no cache entry found\n");

    /*  If directly connected to source, don't send */
    if ((rowPtr->srcAddr == rowPtr->upstream)
        || (rowPtr->upstream == NEXT_HOP_ME))
    {
        if (DEBUG_GRAFT)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                        srcStr);

            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                        grpStr);
            fprintf(stdout, "Node %u @ %s: Source %s is directly "
                            "connected, "
                            "so don't send GRAFT for (%s , %s)\n",
                            node->nodeId, clockStr, srcStr, srcStr, grpStr);
        }

        //change state to forwarding from ackpending
        rowPtr->state = ROUTING_PIM_DM_FORWARD;

        return;
    }

    /*  Allocate message for Graft */
    graftMsg = MESSAGE_Alloc(node, NETWORK_LAYER,
                       MULTICAST_PROTOCOL_PIM, MSG_ROUTING_PimPacket);
    MESSAGE_PacketAlloc(node,
                        graftMsg,
                        sizeof(RoutingPimDmGraftPacket)
                        + sizeof(RoutingPimDmJoinPruneGroupInfo)
                        + sizeof(RoutingPimEncodedSourceAddr),
                        TRACE_PIM);

    graftPkt = (RoutingPimDmGraftPacket*) MESSAGE_ReturnPacket(graftMsg);

    RoutingPimCreateCommonHeader(&graftPkt->commonHeader, ROUTING_PIM_GRAFT);


    graftPkt->upstreamNbr.addressFamily = 1;/*for IPv4 */
    graftPkt->upstreamNbr.encodingType = 0;
    setNodeAddressInCharArray(graftPkt->upstreamNbr.unicastAddr,
                              rowPtr->upstream);


    graftPkt->reserved = 0;
    graftPkt->numGroups = 1;
    graftPkt->holdTime = 0;


    /*  Now, set group information */
    grpInfo.groupAddr.addressFamily = 1;
    grpInfo.groupAddr.encodingType = 0;
    grpInfo.groupAddr.reserved = 0;
    grpInfo.groupAddr.maskLength = ROUTING_PIM_MASK_LENGTH;
    grpInfo.groupAddr.groupAddr = grpAddr;

    grpInfo.numJoinedSrc = 1;
    grpInfo.numPrunedSrc = 0;

  /* Set the encoded join source field */
    encodedSrcAddr.addressFamily = 1;
    encodedSrcAddr.encodingType = 0;
    encodedSrcAddr.rpSimEsa = 0;//all bits are 0 for PIM-DM
    encodedSrcAddr.maskLength = ROUTING_PIM_MASK_LENGTH;
    encodedSrcAddr.sourceAddr = srcAddr;

    dataPtr = ((char*) graftPkt) + sizeof(RoutingPimDmGraftPacket);
    memcpy(dataPtr, &grpInfo, sizeof(RoutingPimDmJoinPruneGroupInfo));

    dataPtr += sizeof(RoutingPimDmJoinPruneGroupInfo);

    /*  Put source address */
    memcpy(dataPtr, &encodedSrcAddr, sizeof(RoutingPimEncodedSourceAddr));


    /*  Send packet to upstream */
    interfaceIndex = NetworkIpGetInterfaceIndexFromAddress(
        node,
        rowPtr->inIntf);

    if (interfaceIndex >= 0)
    {
        NetworkIpSendRawMessageToMacLayer(
                                      node,
                                      graftMsg,
                                      rowPtr->inIntf,
                                      rowPtr->upstream,
                                      IPTOS_PREC_INTERNETCONTROL,
                                      IPPROTO_PIM,
                                      1,
                                      interfaceIndex,
                                      rowPtr->upstream);

        pimDmData->stats.graftSent++;
#ifdef ADDON_DB
        pimDmData->dmSummaryStats.m_NumGraftSent++;
#endif
        /*  Add to retransmission list */
        rowPtr->graftRxmtTimer = TRUE;
        graftTimer.srcAddr = srcAddr;
        graftTimer.grpAddr = grpAddr;
        RoutingPimSetTimer(node,
            interfaceIndex,
            MSG_ROUTING_PimDmGraftRtmxtTimeOut,
            (void*) &graftTimer, ROUTING_PIM_DM_GRAFT_RETRANS_PERIOD);

        if (DEBUG_GRAFT)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(srcAddr,
                                        srcStr);
            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(grpAddr,
                                        grpStr);
            char upstreamStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->upstream,
                                        upstreamStr);
            char addrStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(NetworkIpGetInterfaceAddress(
                                                    node,
                                                    interfaceIndex),
                                        addrStr);
            fprintf(stdout, "Node %u @ %s: Send GRAFT to %s for "
                            "(%s , %s) on interface %d(%s)\n",
                            node->nodeId, clockStr, upstreamStr,
                            srcStr, grpStr, interfaceIndex, addrStr);
        }
    }
}


/*
*  FUNCTION     :RoutingPimDmHandleGraftAckPacket()
*  PURPOSE      :Handle received graft packet
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmHandleGraftAckPacket(
    Node* node,
    NodeAddress srcAddr,
    RoutingPimDmGraftAckPacket* graftAckPkt)
{
    RoutingPimDmJoinPruneGroupInfo* grpInfo = NULL;
    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    char* dataPtr = NULL;

    RoutingPimEncodedSourceAddr mcastSrcAddr;
    memset(&mcastSrcAddr, 0, sizeof(RoutingPimEncodedSourceAddr));

    RoutingPimEncodedGroupAddr mcastGrpAddr;
    memset(&mcastGrpAddr, 0, sizeof(RoutingPimEncodedGroupAddr));

    dataPtr = (char*) graftAckPkt;
    dataPtr += sizeof(RoutingPimDmGraftAckPacket);
    grpInfo = (RoutingPimDmJoinPruneGroupInfo*) dataPtr;
    dataPtr += sizeof(RoutingPimDmJoinPruneGroupInfo);

    mcastGrpAddr.groupAddr = grpInfo->groupAddr.groupAddr;
    memcpy(&mcastSrcAddr, dataPtr, sizeof(RoutingPimEncodedSourceAddr));

    if (DEBUG_GRAFT)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        char graftAckSrcStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(srcAddr,
                                    graftAckSrcStr);
        char srcStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                    srcStr);
        char grpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(mcastGrpAddr.groupAddr,
                                    grpStr);

        fprintf(stdout, "Node %u @ %s: Received GRAFTACK from %s for "
                        "(%s , %s)\n",
                        node->nodeId, clockStr,
                        graftAckSrcStr, srcStr, grpStr);
    }

    /* Find a matching entry in cache */
    rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node,
                                                     mcastSrcAddr.sourceAddr,
                                                     mcastGrpAddr.groupAddr);


    if (!rowPtr)
    {
        if (DEBUG_GRAFT)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                        srcStr);
            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(mcastGrpAddr.groupAddr,
                                        grpStr);

            fprintf(stdout,
                "Node %u @ %s: Doesn't find a matching entry for (%s , %s)."
                "So, Ignoring this GraftAck.\n",node->nodeId, clockStr,
                srcStr, grpStr);
        }

        return;
    }

    /* Cancel retransmission timer */
    rowPtr->graftRxmtTimer = FALSE;
    rowPtr->state = ROUTING_PIM_DM_FORWARD;

    if (DEBUG_GRAFT)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        char srcStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(mcastSrcAddr.sourceAddr,
                                    srcStr);

        char grpStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(mcastGrpAddr.groupAddr,
                                    grpStr);

        fprintf(stdout, "Cancel Graft Retransmission Timer "
                        "for (%s , %s).\nChanging upstream state from "
                        "ACKPENDING to FORWARD for (%s , %s)\n", srcStr,
                        grpStr, srcStr, grpStr);
    }
}


/***************************************************************************/
/*                            PIM ASSERT                                   */
/***************************************************************************/

/*
*  FUNCTION     :RoutingPimDmHandleAssertPacket()
*  PURPOSE      :Process received Assert
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmHandleAssertPacket(
    Node* node,
    NodeAddress srcAddr,
    RoutingPimDmAssertPacket* assertPkt,
    int inIntfIndex)
{
    RoutingPimDmForwardingTableRow* rowPtr = NULL;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    NodeAddress mcastSrcAddr =
           getNodeAddressFromCharArray(assertPkt->sourceAddr.unicastAddr);

    if (pim->interface[inIntfIndex].assertOptimization == FALSE)
    {
        rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                        node,
                        mcastSrcAddr,
                        assertPkt->groupAddr.groupAddr);

        /*  If we doesn't have such entry, drop packet */
        if (rowPtr == NULL)
        {
            return;
        }

        /*  Assert received on incoming interface */
        if (rowPtr->inIntf == NetworkIpGetInterfaceAddress(node, inIntfIndex))
        {
            RoutingPimDmAssertTimerInfo timerInfo;
            BOOL localWins = FALSE;
            BOOL upstreamChange = FALSE;
            BOOL setAssertTimer = FALSE;

            if (DEBUG_ASSERT)
            {
                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                            grpStr);
                char assertSrcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(srcAddr,
                                        assertSrcStr);
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(
                    NetworkIpGetInterfaceAddress(node, inIntfIndex),
                    addrStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout,"Node %u @ %s: Received Assert from %s "
                               "for (%s , %s) on upstream interface %d(%s)\n",
                               node->nodeId,
                               clockStr,
                               assertSrcStr,
                               srcStr,
                               grpStr,
                               inIntfIndex,
                               addrStr);
            }

            if (rowPtr->assertState == Pim_Assert_NoInfo)
            {
                //Assume that the sender is the assert winner

                rowPtr->assertState = Pim_Assert_ILostAssert;
                if (rowPtr->upstream != srcAddr)
                {
                    upstreamChange = TRUE;
                    /*  Make new RPF neighbor a upstream */
                    rowPtr->upstream = srcAddr;
                }
                rowPtr->preference = (NetworkRoutingAdminDistanceType)
                      RoutingPimDmAssertPacketGetMetricPref(
                          assertPkt->metricBitPref);
                rowPtr->metric = assertPkt->metric;
                setAssertTimer = TRUE;
            }
            else
            {
                localWins = RoutingPimDmCompareMetric(
                    rowPtr->preference,
                    rowPtr->metric,
                    rowPtr->upstream,
                    RoutingPimDmAssertPacketGetMetricPref(
                                    assertPkt->metricBitPref),
                    assertPkt->metric, srcAddr);

                if (localWins)//Inferior
                {
                    //Receive Inferior Assert from Current Winner
                    if (rowPtr->upstream == srcAddr)
                    {
                        int outInterface = 0;
                        //The Assert state machine MUST
                        //transition to NoInfo (NI) state and cancel AT(S,G,I)
                        //The router MUST delete the previous Assert Winner’s
                        //address and metric and
                        //evaluate any possible transitions to its Upstream(S,G)
                        //state machine.
                        rowPtr->assertState = Pim_Assert_NoInfo;
                        rowPtr->assertTimerRunning = FALSE;
                        rowPtr->assertTime = 0;
                        rowPtr->preference = ROUTING_ADMIN_DISTANCE_DEFAULT;
                        rowPtr->metric = ROUTING_PIM_INFINITE_METRIC;
                        setAssertTimer = FALSE;

                        /*  Make new RPF neighbor a upstream */
                        /*  Search routing table for a entry to the source */
                        NetworkGetInterfaceAndNextHopFromForwardingTable(
                            node,
                            rowPtr->srcAddr,
                            &outInterface,
                            &rowPtr->upstream);
                    }
                }
                else//preferred
                {
                    rowPtr->assertState = Pim_Assert_ILostAssert;

                    if (rowPtr->upstream != srcAddr)
                    {
                        upstreamChange = TRUE;
                        /*  Make new RPF neighbor a upstream */
                        rowPtr->upstream = srcAddr;
                    }

                    rowPtr->preference = (NetworkRoutingAdminDistanceType)
                        RoutingPimDmAssertPacketGetMetricPref(
                      assertPkt->metricBitPref);
                    rowPtr->metric = assertPkt->metric;
                    setAssertTimer = TRUE;
                }
            }

            if (setAssertTimer)
            {
                if (rowPtr->assertTime == 0)
                {
                timerInfo.srcAddr = rowPtr->srcAddr;
                timerInfo.grpAddr = rowPtr->grpAddr;
                RoutingPimSetTimer(
                    node,
                    inIntfIndex,
                    MSG_ROUTING_PimDmAssertTimeOut,
                    (void*) &timerInfo, ROUTING_PIM_DM_ASSERT_TIMEOUT);
            }

                rowPtr->assertTimerRunning = TRUE;
                rowPtr->assertTime = node->getNodeTime() +
                    ROUTING_PIM_DM_ASSERT_TIMEOUT;
            }

            if (upstreamChange)
            {
                if (NetworkIpIsPartOfMulticastGroup(node, rowPtr->grpAddr)
                    || rowPtr->state != ROUTING_PIM_DM_PRUNE)
                {
                    if (DEBUG_ASSERT)
                    {
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                                    srcStr);
                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                                    grpStr);
                        char upstreamStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->upstream,
                                                    upstreamStr);
                        char nextHopStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(srcAddr,
                                                    nextHopStr);
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                        fprintf(stdout, "Node %u @ %s: Change upstream from "
                                        "%s to %s for (%s , %s)\n",
                                        node->nodeId, clockStr, upstreamStr,
                                        nextHopStr,
                                        srcStr, grpStr);
                    }

                    rowPtr->state = ROUTING_PIM_DM_ACKPENDING;
                    /*  Send Graft to new upstream */
                    RoutingPimDmSendGraftPacket(node, rowPtr->srcAddr,
                            rowPtr->grpAddr);
                }
                else
                {
                    /*  Make new RPF neighbor a upstream */
                    rowPtr->upstream = srcAddr;
                }
            }
        }
        /*  Else if it is downstream interface */
        else if (RoutingPimDmIsDownstreamInterface(rowPtr, inIntfIndex))
        {
            RoutingPimDmDownstreamListItem* downstreamInfo = NULL;
            BOOL localWins = FALSE;
            BOOL pruneDownstream = FALSE;
            BOOL resetAssertTimer = FALSE;

            downstreamInfo = RoutingPimDmGetDownstreamInfo(
                rowPtr,
                inIntfIndex);

            if (DEBUG_ASSERT)
            {
                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                            grpStr);
                char assertSrcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(srcAddr,
                                        assertSrcStr);
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(
                    NetworkIpGetInterfaceAddress(node, inIntfIndex),
                    addrStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout,"Node %u @ %s: Received Assert from %s "
                               "for (%s , %s) on downstream interface %d(%s)\n",
                               node->nodeId,
                               clockStr,
                               assertSrcStr,
                               srcStr,
                               grpStr,
                               inIntfIndex,
                               addrStr);
            }

            if (downstreamInfo->assertState == Pim_Assert_NoInfo)
            {
                //Assume that I am the assert winner

                NetworkDataIp* ip = (NetworkDataIp*)
                    node->networkData.networkVar;

                downstreamInfo->assertState = Pim_Assert_IWonAssert;

                downstreamInfo->winnerAssertMetric.ipAddress
                    = pim->interface[inIntfIndex].ipAddress;

                if (ip->interfaceInfo[inIntfIndex]->routingProtocolType
                    == ROUTING_PROTOCOL_NONE)
                {
                    downstreamInfo->winnerAssertMetric.metricPreference = 0;
                }
                else
                {
                    downstreamInfo->winnerAssertMetric.metricPreference =
                            NetworkRoutingGetAdminDistance(node,
                                ip->interfaceInfo[inIntfIndex]
                                    ->routingProtocolType);
                }

                downstreamInfo->winnerAssertMetric.metric =
                    NetworkGetMetricForDestAddress(
                        node, rowPtr->srcAddr);
            }

            if (downstreamInfo->assertState == Pim_Assert_IWonAssert)
            {
                if (DEBUG_ASSERT)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    fprintf(stdout,"Node %u @ %s: ", node->nodeId, clockStr);
                }

                localWins =
                    RoutingPimDmCompareMetric(
                        downstreamInfo->winnerAssertMetric.metricPreference,
                        downstreamInfo->winnerAssertMetric.metric,
                        downstreamInfo->winnerAssertMetric.ipAddress,
                        RoutingPimDmAssertPacketGetMetricPref(
                                assertPkt->metricBitPref),
                        assertPkt->metric,
                        srcAddr);

                if (DEBUG_ASSERT)
                {
                    if (localWins)
                    {
                        fprintf(stdout,"My Assert metric is higher\n");
                    }
                    else
                    {
                        fprintf(stdout,"My Assert metric is lower\n");
                    }
                }

                if (localWins)
                {
                    //RFC 3963 (Section 4.6.4.1 and 4.6.4.2) Received Inferior
                    //assert if I am the winner or no info. Send Assert(S,G)
                    //and reset Assert timer.
                    if (DEBUG_ASSERT)
                    {
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                                    srcStr);
                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                                    grpStr);
                        char addrStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(
                            NetworkIpGetInterfaceAddress(node, inIntfIndex),
                            addrStr);

                        fprintf(stdout, "Node %u @ %s: I am Assert Winner "
                                        "for (%s , %s) on interface %d(%s)\n",
                                        node->nodeId, clockStr, srcStr, grpStr,
                                        inIntfIndex, addrStr);
                    }

                    RoutingPimDmSendAssertPacket(node,
                        rowPtr->srcAddr,
                        rowPtr->grpAddr,
                        inIntfIndex);

                    resetAssertTimer = TRUE;
                }
                else
                {
                    //RFC 3973 (Section 4.6.4.1 and 4.6.4.2) Received preferred
                    //assert if I am the winner or no info.

                    downstreamInfo->assertState = Pim_Assert_ILostAssert;

                    downstreamInfo->winnerAssertMetric.ipAddress = srcAddr;
                    downstreamInfo->winnerAssertMetric.metricPreference=
                        (NetworkRoutingAdminDistanceType)
                            RoutingPimDmAssertPacketGetMetricPref(
                                assertPkt->metricBitPref);

                    downstreamInfo->winnerAssertMetric.metric =
                        assertPkt->metric;

                    resetAssertTimer = TRUE;
                    pruneDownstream = TRUE;

                    if (DEBUG_ASSERT)
                    {
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                                    srcStr);
                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                                    grpStr);
                        char assertWinnerStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(srcAddr,
                                                    assertWinnerStr);
                        fprintf(stdout, "Node %u @ %s: Assert Winner for "
                                        "(%s , %s) is %s\n", node->nodeId,
                                        clockStr,
                                        srcStr, grpStr, assertWinnerStr);
                    }
                }
            }
            else if (downstreamInfo->assertState == Pim_Assert_ILostAssert)
            {
                if (downstreamInfo->winnerAssertMetric.ipAddress == srcAddr)
                {
                    if (DEBUG_ASSERT)
                    {
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                        fprintf(stdout,"Node %u @ %s: ", node->nodeId, clockStr);
                    }

                    localWins =
                        RoutingPimDmCompareMetric(
                            NetworkRoutingGetAdminDistance(node,
                                ip->interfaceInfo[inIntfIndex]
                                    ->routingProtocolType),
                            NetworkGetMetricForDestAddress(
                                node, rowPtr->srcAddr),
                            pim->interface[inIntfIndex].ipAddress,
                            RoutingPimDmAssertPacketGetMetricPref(
                                assertPkt->metricBitPref),
                            assertPkt->metric,
                            srcAddr);

                    if (DEBUG_ASSERT)
                    {
                        if (localWins)
                        {
                            fprintf(stdout,"My Assert metric is higher\n");
                        }
                        else
                        {
                            fprintf(stdout,"My Assert metric is lower\n");
                        }
                    }

                    if (localWins)
                    {
                        //RFC 3963 (Section 4.6.4.3)Receive Inferior Assert
                        //from Current Winner

                        downstreamInfo->assertState = Pim_Assert_NoInfo;
                        downstreamInfo->assertTimerRunning = FALSE;
                        downstreamInfo->assertTime = 0;
                        downstreamInfo->winnerAssertMetric.metricPreference =
                                ROUTING_ADMIN_DISTANCE_DEFAULT;
                        downstreamInfo->winnerAssertMetric.metric =
                                ROUTING_PIM_INFINITE_METRIC;
                    }
                    else
                    {
                        resetAssertTimer = TRUE;
                    }
                }
                else
                {
                    if (DEBUG_ASSERT)
                    {
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                        fprintf(stdout,"Node %u @ %s: ", node->nodeId, clockStr);
                    }

                    localWins =
                        RoutingPimDmCompareMetric(
                            downstreamInfo->winnerAssertMetric.metricPreference,
                            downstreamInfo->winnerAssertMetric.metric,
                            downstreamInfo->winnerAssertMetric.ipAddress,
                            RoutingPimDmAssertPacketGetMetricPref(
                                assertPkt->metricBitPref),
                            assertPkt->metric,
                            srcAddr);

                    if (DEBUG_ASSERT)
                    {
                        if (localWins)
                        {
                            fprintf(stdout,"My Assert metric is higher\n");
                        }
                        else
                        {
                            fprintf(stdout,"My Assert metric is lower\n");
                        }
                    }

                    if (!localWins)
                    {
                        downstreamInfo->winnerAssertMetric.ipAddress = srcAddr;
                        downstreamInfo->winnerAssertMetric.metricPreference=
                            (NetworkRoutingAdminDistanceType)
                                RoutingPimDmAssertPacketGetMetricPref(
                                    assertPkt->metricBitPref);
                        downstreamInfo->winnerAssertMetric.metric =
                            assertPkt->metric;

                        pruneDownstream = TRUE;
                    }

                    resetAssertTimer = TRUE;
                }

                if (DEBUG_ASSERT)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                                grpStr);
                    char assertWinnerStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(
                        downstreamInfo->winnerAssertMetric.ipAddress,
                        assertWinnerStr);

                    fprintf(stdout, "Node %u @ %s: Assert Winner for (%s , %s) "
                                    "is %s\n", node->nodeId, clockStr,
                                    srcStr, grpStr, assertWinnerStr);
                }
            }

            if (resetAssertTimer)
            {
                if (downstreamInfo->assertTime == 0)
                {
                RoutingPimDmAssertTimerInfo timerInfo;
                timerInfo.srcAddr = rowPtr->srcAddr;
                timerInfo.grpAddr = rowPtr->grpAddr;
                RoutingPimSetTimer(
                    node,
                    inIntfIndex,
                    MSG_ROUTING_PimDmAssertTimeOut,
                    (void*) &timerInfo, ROUTING_PIM_DM_ASSERT_TIMEOUT);
                }

                downstreamInfo->assertTimerRunning = TRUE;
                downstreamInfo->assertTime = node->getNodeTime() +
                                ROUTING_PIM_DM_ASSERT_TIMEOUT;
            }

            if (pruneDownstream)
            {
                RoutingPimDmPruneTimerInfo timerInfo;

                /*  Prune this interface */
                downstreamInfo->isPruned = TRUE;
                downstreamInfo->lastPruneReceived = node->getNodeTime();
                downstreamInfo->pruneTimer = ROUTING_PIM_DM_PRUNE_TIMEOUT;

#ifdef ADDON_DB
                //remove this entry from multicast_connectivity cache
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = rowPtr->grpAddr;
                strcpy(stats.rootNodeType,"Source");
                stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                rowPtr->srcAddr);
                stats.outgoingInterface = downstreamInfo->interfaceIndex;
                stats.upstreamNeighborId =
                                      MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                               rowPtr->upstream);
                stats.upstreamInterface =
                                      NetworkIpGetInterfaceIndexFromAddress(node,
                                                                 rowPtr->inIntf);

                // delete entry from  multicast_connectivity cache
                STATSDB_HandleMulticastConnDeletion(node, stats);
#endif
                if (downstreamInfo->pruneTimer !=
                        ROUTING_PIM_INFINITE_HOLDTIME)
                {
                    /* Set timer to time out prune on this interface */
                    timerInfo.srcAddr = rowPtr->srcAddr;
                    timerInfo.grpAddr = rowPtr->grpAddr;
                    timerInfo.downstreamIntf = downstreamInfo->intfAddr;

                    RoutingPimSetTimer(node,
                        inIntfIndex,
                        MSG_ROUTING_PimDmPruneTimeoutAlarm,
                        (void*) &timerInfo, ROUTING_PIM_DM_PRUNE_TIMEOUT);
                }

                if (rowPtr->state != ROUTING_PIM_DM_PRUNE &&
                        RoutingPimDmAllDownstreamPruned(rowPtr) &&
                        !(NetworkIpIsPartOfMulticastGroup(node,
                            rowPtr->grpAddr)))
                {
                    RoutingPimDmDataTimeoutInfo cacheTimer;

                    rowPtr->state = ROUTING_PIM_DM_PRUNE;

                    /*  Send prune to upstream */
                    RoutingPimDmSendJoinPrunePacket(node,
                        rowPtr->srcAddr,
                        rowPtr->grpAddr, UPSTREAM, ROUTING_PIM_PRUNE_TREE,
                        ROUTING_PIM_DM_PRUNE_TIMEOUT
                        );

                    rowPtr->delayedJoinTimer = FALSE;
                    rowPtr->graftRxmtTimer = FALSE;

                    if (rowPtr->expirationTimer == 0)
                    {
                        cacheTimer.srcAddr = rowPtr->srcAddr;
                        cacheTimer.grpAddr = rowPtr->grpAddr;
                        RoutingPimSetTimer(node,
                            inIntfIndex,
                            MSG_ROUTING_PimDmDataTimeOut,
                                (void*) &cacheTimer,
                                (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                    }
                    /*  Set DataTimeout timer */
                    rowPtr->expirationTimer = node->getNodeTime() +
                        ROUTING_PIM_DM_DATA_TIMEOUT;
                }

                if (downstreamInfo->assertState == Pim_Assert_ILostAssert)
                {
                    //send prune to assert winner
                    RoutingPimDmSendJoinPrunePacket(node,
                        rowPtr->srcAddr,
                        rowPtr->grpAddr,
                        downstreamInfo->winnerAssertMetric.ipAddress,
                        ROUTING_PIM_PRUNE_TREE,
                        ROUTING_PIM_DM_ASSERT_TIMEOUT);
                }
            }

            if (downstreamInfo->assertState == Pim_Assert_NoInfo &&
                rowPtr->state == ROUTING_PIM_DM_PRUNE)
            {
                //Change the upstream state and send graft message

                RoutingPimDmDataTimeoutInfo cacheTimer;

                rowPtr->state = ROUTING_PIM_DM_ACKPENDING;
                RoutingPimDmSendGraftPacket(node,
                                            rowPtr->srcAddr,
                                            rowPtr->grpAddr);

                if (rowPtr->expirationTimer == 0)
                {
                    cacheTimer.srcAddr = rowPtr->srcAddr;
                    cacheTimer.grpAddr = rowPtr->grpAddr;
                    RoutingPimSetTimer(node,
                        inIntfIndex,
                        MSG_ROUTING_PimDmDataTimeOut,
                        (void*) &cacheTimer,
                        (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                }
                /*  Set DataTimeout timer */
                rowPtr->expirationTimer = node->getNodeTime() +
                    ROUTING_PIM_DM_DATA_TIMEOUT;
            }//end of if
        }//end of else if
    }
    else
    {
        RoutingPimDmAssertStateMachine(node,
                                       mcastSrcAddr,
                                       assertPkt->groupAddr.groupAddr,
                                       inIntfIndex,
                                       assertPkt->metricBitPref,
                                       assertPkt->metric,
                                       srcAddr);
    }
}

/*
*  FUNCTION     :RoutingPimDmSendAssertPacket()
*  PURPOSE      :Send Assert packet
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmSendAssertPacket(
        Node* node,
        NodeAddress srcAddr,
        NodeAddress grpAddr,
        int interfaceId)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    PimDmData* pimDmData = NULL;

    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }

    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    Message* assertMsg = NULL;
    RoutingPimDmAssertPacket assertPkt;
    memset(&assertPkt, 0, sizeof(RoutingPimDmAssertPacket));
    int metricPreference = 0;
    int metric = 0;

    NodeAddress nextHop = 0;
    int interfaceIndex = 0;

    rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node, srcAddr, grpAddr);
    ERROR_Assert(rowPtr, "Unable to send Assert, entry not found\n");

    NetworkGetInterfaceAndNextHopFromForwardingTable(node, srcAddr,
            &interfaceIndex, &nextHop);

    if (nextHop == (unsigned) NETWORK_UNREACHABLE)
    {
        metricPreference = ROUTING_ADMIN_DISTANCE_DEFAULT;
        metric = ROUTING_PIM_INFINITE_METRIC;
    }
    else
    {
        if (ip->interfaceInfo[interfaceId]->routingProtocolType
                == ROUTING_PROTOCOL_NONE)
        {
            metricPreference = 0;
        }
        else
        {
            metricPreference = NetworkRoutingGetAdminDistance(node,
                    ip->interfaceInfo[interfaceId]->routingProtocolType);
        }
        metric = NetworkGetMetricForDestAddress(node, srcAddr);
    }

    assertMsg = MESSAGE_Alloc(node,
                        NETWORK_LAYER,
                        MULTICAST_PROTOCOL_PIM,
                        MSG_ROUTING_PimPacket);
    MESSAGE_PacketAlloc(node,
                        assertMsg,
                        PIM_ASSERT_PACKET_SIZE,
                        TRACE_PIM);

    RoutingPimCreateCommonHeader(&assertPkt.commonHeader,
        ROUTING_PIM_ASSERT);

    /* set the group information in packet field*/
    assertPkt.groupAddr.addressFamily = 1;
    assertPkt.groupAddr.encodingType = 0;
    assertPkt.groupAddr.reserved = 0;
    assertPkt.groupAddr.maskLength = ROUTING_PIM_MASK_LENGTH;
    assertPkt.groupAddr.groupAddr = grpAddr;

    /* set the source address(encoded unicast form) in packet field*/
    assertPkt.sourceAddr.addressFamily = 1;
    assertPkt.sourceAddr.encodingType = 0;
    setNodeAddressInCharArray(assertPkt.sourceAddr.unicastAddr , srcAddr);

    /* Set RPT Bit 0 for (S, G) Assert Message */
    RoutingPimDmAssertPacketSetRPTBit(&(assertPkt.metricBitPref), 0);
    RoutingPimDmAssertPacketSetMetricPref(&(assertPkt.metricBitPref),
        metricPreference);
    assertPkt.metric = (unsigned int) metric;

    char* packet = MESSAGE_ReturnPacket(assertMsg);
    RoutingPimSetBufferFromAssertPacket(packet, &assertPkt);

    if (interfaceId >= 0)
    {
        /* Send packet */
        NetworkIpSendRawMessageToMacLayer(
                                      node,
                                      assertMsg,
                                      NetworkIpGetInterfaceAddress(
                                                node,
                                                interfaceId),
                                      ALL_PIM_ROUTER,
                                      IPTOS_PREC_INTERNETCONTROL,
                                      IPPROTO_PIM,
                                      1,
                                      interfaceId,
                                      ANY_DEST);
        pimDmData->stats.assertSent++;
#ifdef ADDON_DB
        pimDmData->dmSummaryStats.m_NumAssertSent++;
#endif

        if (DEBUG_ASSERT)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            char srcStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                        srcStr);
            char grpStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                        grpStr);
            char addrStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceAddress(node, interfaceId),
                addrStr);

            fprintf(stdout, "Node %u @ %s: Sending Assert for (%s , %s) "
                            "on interface %d(%s)\n",
                            node->nodeId, clockStr, srcStr, grpStr,
                            interfaceId, addrStr);
        }
    }
}

static
BOOL RoutingPimDmCompareMetric(
        int localPreference,
        int localMetric,
        NodeAddress localAddr,
        int remotePreference,
        int remoteMetric,
        NodeAddress remoteAddr)
{
    if (DEBUG_ASSERT)
    {
        char localAddrStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(localAddr,
                                    localAddrStr);
        char remoteAddrStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(remoteAddr,
                                    remoteAddrStr);

        fprintf(stdout, "Comparing My Assert metrics "
                        "(%d , %d , %s) with (%d , %d , %s)\n",
                        localPreference, localMetric,
                        localAddrStr, remotePreference, remoteMetric,
                        remoteAddrStr);
    }

    if (localPreference < remotePreference)
    {
        return TRUE;
    }
    else if (localPreference > remotePreference)
    {
        return FALSE;
    }
    else if (localMetric < remoteMetric)
    {
        return TRUE;
    }
    else if (localMetric > remoteMetric)
    {
        return FALSE;
    }
    else if (localAddr > remoteAddr)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*
*  FUNCTION     :RoutingPimDmHasLocalGroup()
*  PURPOSE      :Check whether the attached network has local member
*                for the specified group
*  ASSUMPTION   :None
*  RETURN VALUE :BOOL
*/
BOOL RoutingPimDmHasLocalGroup(Node* node, NodeAddress grpAddr,
        int interfaceIndex)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    BOOL localMember = FALSE;

    if (ip->isIgmpEnable == TRUE)
    {
        IgmpSearchLocalGroupDatabase(node,
                                     grpAddr,
                                     interfaceIndex,
                                     &localMember);
    }

    return localMember;
}

void
RoutingPimDmPrintNeibhborList(Node* node)
{
    ListItem* tempListItem = NULL;
    char nbrAddress[MAX_STRING_LENGTH];
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    int index = 1;
    char clockStr[MAX_STRING_LENGTH];
    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

    for (int i = 0 ; i < node->numberInterfaces ; i++)
    {
        tempListItem = pim->interface[i].dmInterface->neighborList->first;

        printf("\n\nNode %d @ %s : PIM NEIBHBOR LIST at Interface %d\n",
            node->nodeId, clockStr, i);
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
        index = 1;

        while (tempListItem)
        {
            RoutingPimNeighborListItem* neighbor =
                (RoutingPimNeighborListItem*)tempListItem->data;
            IO_ConvertIpAddressToString(neighbor->ipAddr,
                            nbrAddress);

            printf("%d \t %s\n",index,nbrAddress);
            index++;
            tempListItem = tempListItem->next;
    }
}
}

/*
*  FUNCTION     :RoutingPimDmPrintForwardingTable()
*  PURPOSE      :Print forwarding table of a node
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimDmPrintForwardingTable(Node* node)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }

    RoutingPimDmForwardingTableRow* rowPtr;

    FILE* fp;
    unsigned int i;
    char grpAddr[MAX_STRING_LENGTH];
    char srcAddr[MAX_STRING_LENGTH];
    char upstream[MAX_STRING_LENGTH];
    char downstream[MAX_STRING_LENGTH];
    char inIntf[MAX_STRING_LENGTH];
    char clockStr[MAX_CLOCK_STRING_LENGTH];
    clocktype timestamp;

    fp = fopen("FWDTable.txt", "a+");

    if (fp == NULL)
    {
        fprintf(stdout, "Couldn't open file FWDTable.txt\n");
        return;
    }

    timestamp = node->getNodeTime() / SECOND;
    ctoa(timestamp, clockStr);

    fprintf(fp, "        Forwarding Table for Node %u at time = %s\n",
                 node->nodeId, clockStr);
    fprintf(fp, "---------------------------------------------------------");
    fprintf(fp, "\n   Source       Group       Upstream     "
                "State        InIntf         Downstream Intf\n");

    rowPtr = (RoutingPimDmForwardingTableRow*)
                     BUFFER_GetData(&pimDmData->fwdTable.buffer);

    for (i = 0; i < pimDmData->fwdTable.numEntries; i++)
    {
        ListItem* listItem;

        listItem = rowPtr[i].downstream->first;
        IO_ConvertIpAddressToString(rowPtr[i].grpAddr, grpAddr);
        IO_ConvertIpAddressToString(rowPtr[i].srcAddr, srcAddr);
        IO_ConvertIpAddressToString(rowPtr[i].upstream, upstream);
        IO_ConvertIpAddressToString(rowPtr[i].inIntf, inIntf);

        fprintf(fp, "%10s    %10s  %10s    ", srcAddr, grpAddr, upstream);

        if (rowPtr[i].state == ROUTING_PIM_DM_PRUNE)
        {
            fprintf(fp, "Pruned         *  \n\n");
            continue;
        }
        fprintf(fp, " Active    %10s    ", inIntf);

        while (listItem)
        {
            RoutingPimDmDownstreamListItem* downstreamInfo;

            downstreamInfo =
                (RoutingPimDmDownstreamListItem*) listItem->data;

            if (downstreamInfo->isPruned == TRUE)
            {
                listItem = listItem->next;
                continue;
            }

            IO_ConvertIpAddressToString(
                downstreamInfo->intfAddr,
                downstream);

            fprintf(fp, "    %10s    \n", downstream);

            listItem = listItem->next;
            fprintf(fp, "                                        "
                        "                         ");
        }
        fprintf(fp, "\n\n");
    }

    fprintf(fp, "#########################################################");
    fprintf(fp, "\n\n");

    fclose(fp);
}


void
RoutingPimDmAdaptUnicastRouteChange(
    Node* node,
    NodeAddress destAddr,
    NodeAddress destAddrMask,
    NodeAddress nextHop,
    int interfaceId,
    int metric,
    NetworkRoutingAdminDistanceType adminDistance)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;

    RoutingPimDmForwardingTableRow* rowPtr = NULL;
    unsigned int i = 0;
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
    rowPtr = (RoutingPimDmForwardingTableRow*)
                     BUFFER_GetData(&pimDmData->fwdTable.buffer);

    for (i = 0; i < pimDmData->fwdTable.numEntries; i++)
    {
        /*  Check whether upstream have been changed */
#ifdef ADDON_BOEINGFCS
        //if (((rowPtr[i].srcAddr & destAddrMask) == destAddr)
        //    && (rowPtr[i].upstream != nextHop) && nextHop != 0)
        if ((((rowPtr[i].srcAddr & destAddrMask) == destAddr)
            && (rowPtr[i].upstream != nextHop) && nextHop != 0)
            && rowPtr[i].preference == adminDistance)
#else
        if (((rowPtr[i].srcAddr & destAddrMask) == destAddr)
            && (rowPtr[i].upstream != nextHop))
#endif
        {
            if (DEBUG_URC)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr[i].srcAddr,
                                            srcStr);

                fprintf(stdout, "Node %u @ %s: PIM got URC for "
                                "mcast source %s\n",
                                node->nodeId, clockStr,
                                srcStr);

                NetworkPrintForwardingTable(node);
            }

            /*  If this route is now unrechable, delete this entry */
            if (nextHop == (unsigned) NETWORK_UNREACHABLE)
            {
                if (DEBUG_URC)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr[i].srcAddr,
                                                srcStr);

                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr[i].grpAddr,
                                                grpStr);

                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout, "Node %u @ %s: Delete (%s , %s) entry "
                                    "from forwarding cache as there is no "
                                    "route back to source\n",
                                    node->nodeId, clockStr, srcStr, grpStr);
                }

#ifdef ADDON_DB
                //remove this entry from multicast_connectivity cache
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = rowPtr[i].grpAddr;
                strcpy(stats.rootNodeType,"Source");
                stats.rootNodeId= MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                          rowPtr[i].srcAddr);
                stats.upstreamNeighborId =
                                  MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                         rowPtr[i].upstream);
                stats.upstreamInterface =
                                  NetworkIpGetInterfaceIndexFromAddress(node,
                                                           rowPtr[i].inIntf);

                ListItem* listItem = rowPtr[i].downstream->first;

                while (listItem)
                {
                    RoutingPimDmDownstreamListItem* downstream;
                    downstream =
                            (RoutingPimDmDownstreamListItem*) listItem->data;

                    stats.outgoingInterface = downstream->interfaceIndex;

                    // delete entry from  multicast_connectivity cache
                    STATSDB_HandleMulticastConnDeletion(node, stats);

                    listItem = listItem->next;
                }
#endif

                ListFree(node, rowPtr[i].downstream, FALSE);

                BUFFER_RemoveDataFromDataBuffer(
                        &pimDmData->fwdTable.buffer,
                        (char*) &rowPtr[i],
                        sizeof(RoutingPimDmForwardingTableRow));

                // reset rowPtr since it now can potentially point to a new
                // place in memory after the call to RemoveDataFromDataBuffer
                rowPtr = (RoutingPimDmForwardingTableRow*)
                            BUFFER_GetData(&pimDmData->fwdTable.buffer);

                pimDmData->fwdTable.numEntries -= 1;
                i--;
                continue;
            }

            if (DEBUG_URC)
            {
                char upstreamStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr[i].upstream,
                                            upstreamStr);

                char nextHopStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(nextHop,
                                            nextHopStr);

                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr[i].srcAddr,
                                            srcStr);

                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(rowPtr[i].grpAddr,
                                            grpStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout, "Node %u @ %s: Change upstream from %s to %s "
                                "for (%s , %s)\n",
                                node->nodeId, clockStr, upstreamStr,
                                nextHopStr,
                                srcStr, grpStr);
            }

            /*  Make new RPF neighbor a upstream */
            rowPtr[i].upstream = nextHop;

          /*
           *   If this interface was previous downstream,
           *   delete from downstream list
           */
            if (RoutingPimDmIsDownstreamInterface(&rowPtr[i],
                        interfaceId))
            {
                int prevIntfIndex =
                    NetworkIpGetInterfaceIndexFromAddress(node,
                                        rowPtr[i].inIntf);
                RoutingPimDmRemoveDownstream(node, &rowPtr[i], interfaceId);

                /* Add previous upstream as new downstream */
                RoutingPimDmAddDownstream(node, &rowPtr[i], prevIntfIndex);
            }

            rowPtr[i].inIntf = NetworkIpGetInterfaceAddress(node,
                                       interfaceId);

            rowPtr[i].assertState = Pim_Assert_NoInfo;
            rowPtr[i].assertTime = 0;
            rowPtr[i].metric = metric;
            rowPtr[i].preference = adminDistance;

            if ((!RoutingPimDmAllDownstreamPruned(rowPtr) ||
                  NetworkIpIsPartOfMulticastGroup(node, rowPtr->grpAddr)))
            {
                rowPtr[i].state = ROUTING_PIM_DM_ACKPENDING;

                /*  Send Graft to new upstream */
                RoutingPimDmSendGraftPacket(node, rowPtr[i].srcAddr,
                        rowPtr[i].grpAddr);

                if (DEBUG_URC)
                {
                    RoutingPimDmPrintForwardingTable(node);
                }
            }
        }
    }
}
#ifdef ADDON_DB
BOOL RoutingPimDmIsMyMulticastPacket(Node *node,
                                     NodeAddress srcAddr,
                                     NodeAddress dstAddr,
                                     NodeAddress prevAddr,
                                     int incomingInterface)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    int interfaceId = 0;
    NodeAddress nextHopToSrc ;
    BOOL originateByMe = FALSE ;

    if (incomingInterface == -1)
    {
        return TRUE ;
    }

    for (int i = 0; i < node->numberInterfaces; i++)
    {
#ifdef ADDON_BOEINGFCS
        if ((pim->interface[incomingInterface].interfaceType ==
            MULTICAST_CES_RPIM_INTERFACE) &&
            (pim->modeType == ROUTING_PIM_MODE_DENSE))
        {
            if (NetworkIpIsMyIP(node, srcAddr))
            {
                return TRUE ;
            }
        }
        else
        {
            if (srcAddr == pim->interface[i].ipAddress)
            {
                originateByMe = TRUE;
                break ;
            }

        }
#else
         if (srcAddr == pim->interface[i].ipAddress)
         {
             originateByMe = TRUE ;
             break ;
         }
#endif
    }

    if (originateByMe)
    {
        nextHopToSrc = NEXT_HOP_ME;
    }
    else
    {
        // if not originated by me, see if we can reach the src
        NetworkGetInterfaceAndNextHopFromForwardingTable(
            node,
            srcAddr,
            &interfaceId,
            &nextHopToSrc);

#ifdef ADDON_BOEINGFCS
        if ((pim->interface[incomingInterface].interfaceType ==
            MULTICAST_CES_RPIM_INTERFACE) &&
            (pim->modeType == ROUTING_PIM_MODE_DENSE))
        {
            if (NetworkCesIncEplrsActiveOnNode(node))
            {
                if (prevAddr != ANY_IP)
                {
                    nextHopToSrc = prevAddr;
                }
                else
                {
                    nextHopToSrc = srcAddr;
                }

                interfaceId = incomingInterface;
            }

            /* If there is no route back to source discard packet silently */
            //if ((nextHop == (unsigned) NETWORK_UNREACHABLE) || nextHop == 0)
            if (nextHopToSrc == (unsigned) NETWORK_UNREACHABLE)
            {

                return FALSE;
            }

            if (nextHopToSrc != NEXT_HOP_ME && prevAddr != ANY_DEST)
            {
                // since ROSPF gives exit point rather then immediate next hop
                nextHopToSrc = prevAddr;
            }
        }
        else
        {
            if (nextHopToSrc == (unsigned) NETWORK_UNREACHABLE)
            {
                return FALSE;
            }

        }
    }
#else

        if (nextHopToSrc == (unsigned) NETWORK_UNREACHABLE)
        {
            return FALSE ;
        }
    }
#endif

    RoutingPimDmForwardingTableRow* rowPtr =
        RoutingPimDmGetFwdTableEntryForThisPair(node,
                 srcAddr, dstAddr);
    if (rowPtr == NULL)
    {
        // no cache entry here -
        /*  If packet didn't come from desired interface, discard it */
        if ((!originateByMe) &&
            (interfaceId != incomingInterface))
        {
            //printf("Node %u discard multicast packet "
            //                "as it does not came from the upstream "
            //                "interface %d\n",
            //                node->nodeId, interfaceId);
            return FALSE ;
        }

        // logic from buildnewcacheentry
        if (nextHopToSrc != NEXT_HOP_ME)
        {
            RoutingPimNeighborListItem* neighbor;

            /*  Search neighbor in neighbor list */

            RoutingPimSearchNeighborList(
                    pim->interface[incomingInterface].dmInterface->neighborList,
                    nextHopToSrc,
                    &neighbor);
#ifdef ADDON_BEOINGFCS
            if ((pim->interface[incomingInterface].interfaceType ==
                MULTICAST_CES_RPIM_INTERFACE) &&
                (pim->modeType == ROUTING_PIM_MODE_DENSE))
            {
                if (neighbor == NULL &&
                    !RoutingCesSdrActiveOnNode(node) &&
                    !NetworkCesIncEplrsActiveOnNode(node))
                {

                    return FALSE;
                }
            }
#else
            // If the upstream is not a router do not add it to fwding cache

            if (neighbor == NULL)
            {
                return FALSE ;
            }
#endif

        }
         /*
       *   Initially assume each interface other than the incoming
       *   one as downstream
       */
        int forwardToDownstream = 0;
        for (int i = 0; i < node->numberInterfaces; i++)
        {
#ifdef ADDON_BOEINGFCS
           if ((pim->interface[incomingInterface].interfaceType !=
                MULTICAST_CES_RPIM_INTERFACE) &&
                (pim->modeType == ROUTING_PIM_MODE_DENSE))
            {
                if (i == incomingInterface
                    && (srcAddr != pim->interface[i].ipAddress))
                {
                    continue;
                }
            }
#else
            /*  Skip incoming interface */
            if (i == incomingInterface
                && (srcAddr != pim->interface[i].ipAddress))
            {
                continue;
            }

#endif
            /*
           *   Consider an interface as downstream:
           *                    if 1) Not a broadcast interface
           *                 or if 2) Local member present for this group
           */

            if ((pim->interface[i].dmInterface->neighborCount >= 1)
                || (RoutingPimDmHasLocalGroup(node, dstAddr, i))
                || NetworkIpIsPartOfMulticastGroup(node, dstAddr))
            {
#ifdef ADDON_BOEINGFCS
                if ((pim->interface[incomingInterface].interfaceType ==
                     MULTICAST_CES_RPIM_INTERFACE) &&
                     (pim->modeType == ROUTING_PIM_MODE_DENSE))
                {
                    // only forward packets to non-SINCGARS and non-EPLRS interfaces.
                    // This is because they each have their own multicast forwarding
                    // mechanism that picks the packets up before arriving here. We
                    // do not want to introduce extra redundant traffic into the
                    // network.
                    if ((RoutingCesSdrActiveOnInterface(node, i) ||
                        NetworkCesIncEplrsActiveOnInterface(node, i)))
                    {
                        continue;
                    }
                }

#endif
                forwardToDownstream++ ;
                return TRUE ;
                //NodeAddress netAddr;

                //netAddr = NetworkIpGetInterfaceNetworkAddress(node, i);
                //printf("Node %u can forward multicast packet to "
                //        "network %u\n",
                //        node->nodeId, netAddr);
            }
        }

        if (forwardToDownstream == 0) {
            return FALSE ;
        }
        return TRUE ;
    }
    else {

#ifdef ADDON_BOEINGFCS
        if ((pim->interface[incomingInterface].interfaceType !=
            MULTICAST_CES_RPIM_INTERFACE) &&
            (pim->modeType == ROUTING_PIM_MODE_DENSE))
        {
            if (RoutingPimDmIsDownstreamInterface(rowPtr, incomingInterface)
                && (srcAddr != pim->interface[incomingInterface].ipAddress))
            {

                return FALSE ;
            }
        }
#else
        if (RoutingPimDmIsDownstreamInterface(rowPtr, incomingInterface)
            && (srcAddr != pim->interface[incomingInterface].ipAddress))
        {
            //printf("node %d receives packet from downstream interface %u \n",
            //    node->nodeId,
            //    incomingInterface ) ;

            return FALSE ;
        }
#endif
        /*  If packet didn't come from desired interface */
        if (rowPtr->inIntf != NetworkIpGetInterfaceAddress(node, incomingInterface))
        {
            // need input
            return FALSE ;
        }


        /*  So we've got the packet from correct upstream */
        if (rowPtr->state == ROUTING_PIM_DM_PRUNE)
        {
            return FALSE ;
        }

        return TRUE ;
    }

}

#endif




/*
*  FUNCTION     :RoutingPimDmSearchAssertSrcPair()
*  PURPOSE      :Get AssertSourcePair information
*  ASSUMPTION   :None
*  RETURN VALUE :Return desired information, return NULL if not found
*/
RoutingPimDmAssertSrcListItem*
RoutingPimDmSearchAssertSrcPair(Node* node,
                                   NodeAddress srcAddr,
                                   int interfaceId)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    ListItem* listItem =
        pim->interface[interfaceId].dmInterface->assertSourceList->first;

    while (listItem)
    {
        RoutingPimDmAssertSrcListItem* assertSrcListItem;

        assertSrcListItem =
            (RoutingPimDmAssertSrcListItem*) listItem->data;

        if (assertSrcListItem && assertSrcListItem->srcAddr == srcAddr)
        {
            return assertSrcListItem;
        }
        listItem = listItem->next;
    }

    return NULL;
}


/*
*  FUNCTION     :RoutingPimDmAddAssertSrcPair()
*  PURPOSE      :Add new assert source pair
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimDmAddAssertSrcPair(Node* node,
                                     NodeAddress sourceAddr,
                                     NodeAddress grpAddr,
                                     int interfaceId,
                                     BOOL isUpstream,
                                     NodeAddress upstreamAddr)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    RoutingPimDmAssertSrcListItem* assertSrcListItem
                        = RoutingPimDmSearchAssertSrcPair(node,
                                                             sourceAddr,
                                                             interfaceId);

    if (assertSrcListItem == NULL)
    {
        assertSrcListItem =
                (RoutingPimDmAssertSrcListItem*)
                MEM_malloc(sizeof(RoutingPimDmAssertSrcListItem));

        assertSrcListItem->srcAddr = sourceAddr;
        assertSrcListItem->assertState = Pim_Assert_NoInfo;
        assertSrcListItem->assertTime = node->getNodeTime();
        assertSrcListItem->winnerAssertMetric.metricPreference= 0x7FFFFFFF;
        assertSrcListItem->winnerAssertMetric.metric = 0xFFFFFFFF;
        assertSrcListItem->metric = NetworkGetMetricForDestAddress(
                                              node,
                                              sourceAddr,
                                              &(assertSrcListItem->preference));

        assertSrcListItem->interfaceId = interfaceId;
        assertSrcListItem->grpSet = new GroupSet;
        assertSrcListItem->isUpstream = isUpstream;
        assertSrcListItem->upstreamAddr = upstreamAddr;

        /* Insert item to assert source list */
        ListInsert(node,
                   pim->interface[interfaceId].dmInterface->assertSourceList,
                   node->getNodeTime(),
                   (void*) assertSrcListItem);
    }

    /*
    As STL set itself does not insert a duplicate entry so no need to
    check for duplicate entry
    */
    assertSrcListItem->grpSet->insert(grpAddr);

    if (DEBUG_WIRELESS)
    {
        printf("\nNode:%u::Entries in the group set %"TYPES_SIZEOFMFT"u\n",
                node->nodeId,
                assertSrcListItem->grpSet->size());
    }
}


/*
*  FUNCTION     :RoutingPimDmAssertStateMachine()
*  PURPOSE      :handles assert state machine if assert optimization is
*..              enabled
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimDmAssertStateMachine(Node* node,
                               NodeAddress mcastSrcAddr,
                               NodeAddress grpAddress,
                               int inIntfIndex,
                               UInt32 metricBitPref,
                               unsigned int metric,
                               NodeAddress assertPktSrcAddr)
{
    RoutingPimDmAssertSrcListItem* assertSrcListItem =
        RoutingPimDmSearchAssertSrcPair(node,
                                        mcastSrcAddr,
                                        inIntfIndex);

    if (assertSrcListItem == NULL)
    {
        return;
    }

    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    /*  Assert received on incoming interface */
    if (assertSrcListItem->isUpstream
        &&
        assertSrcListItem->interfaceId == inIntfIndex)
    {
        RoutingPimDmAssertTimerInfo timerInfo;
        BOOL localWins = FALSE;
        BOOL setAssertTimer = FALSE;
        GrpSetIterator grpSetIterator;

        if (assertSrcListItem->assertState == Pim_Assert_NoInfo)
        {
            //Assume that the sender is the assert winner

            assertSrcListItem->assertState = Pim_Assert_ILostAssert;

            RoutingPimDmForwardingTableRow* rowPtr = NULL;

            for (grpSetIterator = assertSrcListItem->grpSet->begin();
                 grpSetIterator != assertSrcListItem->grpSet->end();
                 grpSetIterator++)
            {
                rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                                            node,
                                            mcastSrcAddr,
                                            *grpSetIterator);

                if (rowPtr == NULL)
                {
                    if (DEBUG_WIRELESS)
                    {
                        ERROR_Assert(FALSE,"\nEntry must exist in database\n");
                    }

                    continue;
                }

                if (rowPtr->upstream != assertPktSrcAddr)
                {
                    if (NetworkIpIsPartOfMulticastGroup(node,
                                                       rowPtr->grpAddr)
                       ||
                       rowPtr->state != ROUTING_PIM_DM_PRUNE)
                    {
                        /*  Send prune to previous upstream */
                        RoutingPimDmSendJoinPrunePacket(
                                node,
                                rowPtr->srcAddr,
                                rowPtr->grpAddr,
                                UPSTREAM,
                                ROUTING_PIM_PRUNE_TREE,
                                ROUTING_PIM_DM_PRUNE_TIMEOUT);

                        /*  Make new RPF neighbor a upstream */
                        rowPtr->upstream = assertPktSrcAddr;
                        assertSrcListItem->upstreamAddr = assertPktSrcAddr;

                        rowPtr->state = ROUTING_PIM_DM_ACKPENDING;
                        /*  Send Graft to new upstream */
                        RoutingPimDmSendGraftPacket(node,
                                                    rowPtr->srcAddr,
                                                    rowPtr->grpAddr);
                   }
                   else
                   {
                       /*  Make new RPF neighbor a upstream */
                       rowPtr->upstream = assertPktSrcAddr;
                       assertSrcListItem->upstreamAddr = assertPktSrcAddr;
                   }
               }
            }

            assertSrcListItem->preference =
                        (NetworkRoutingAdminDistanceType)
                        RoutingPimDmAssertPacketGetMetricPref(
                                        metricBitPref);
            assertSrcListItem->metric = metric;
            setAssertTimer = TRUE;
        }
        else
        {
            localWins = RoutingPimDmCompareMetric(
                assertSrcListItem->preference,
                assertSrcListItem->metric,
                assertSrcListItem->upstreamAddr,
                RoutingPimDmAssertPacketGetMetricPref(metricBitPref),
                metric,
                assertPktSrcAddr);

            if (localWins)//Inferior
            {
                RoutingPimDmForwardingTableRow* rowPtr = NULL;

                for (grpSetIterator = assertSrcListItem->grpSet->begin();
                     grpSetIterator != assertSrcListItem->grpSet->end();
                     grpSetIterator++)
                {
                    rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                                            node,
                                            mcastSrcAddr,
                                            *grpSetIterator);

                    if (rowPtr == NULL)
                    {
                        if (DEBUG_WIRELESS)
                        {
                            ERROR_Assert(FALSE,"\nEntry must exist in database\n");
                        }

                        continue;
                    }

                    //Receive Inferior Assert from Current Winner
                    if (rowPtr->upstream == assertPktSrcAddr)
                    {
                        int outInterface = 0;
                        //The Assert state machine MUST
                        //transition to NoInfo (NI) state and cancel AT(S,G,I)
                        //The router MUST delete the previous Assert Winner’s
                        //address and metric and
                        //evaluate any possible transitions to its Upstream(S,G)
                        //state machine.
                        assertSrcListItem->assertState = Pim_Assert_NoInfo;
                        assertSrcListItem->assertTimerRunning = FALSE;
                        assertSrcListItem->assertTime = 0;
                        assertSrcListItem->preference = ROUTING_ADMIN_DISTANCE_DEFAULT;
                        assertSrcListItem->metric = ROUTING_PIM_INFINITE_METRIC;
                        setAssertTimer = FALSE;

                        /*  Make new RPF neighbor a upstream */
                        /*  Search routing table for a entry to the source */
                        NetworkGetInterfaceAndNextHopFromForwardingTable(
                            node,
                            rowPtr->srcAddr,
                            &outInterface,
                            &rowPtr->upstream);
                    }//end of if
                }//end of for
            }//end of if
            else//preferred
            {
                assertSrcListItem->assertState = Pim_Assert_ILostAssert;
                RoutingPimDmForwardingTableRow* rowPtr = NULL;

                for (grpSetIterator = assertSrcListItem->grpSet->begin();
                     grpSetIterator != assertSrcListItem->grpSet->end();
                     grpSetIterator++)
                {
                    rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                                            node,
                                            mcastSrcAddr,
                                            *grpSetIterator);

                    if (rowPtr == NULL)
                    {
                        if (DEBUG_WIRELESS)
                        {
                            ERROR_Assert(FALSE,"\nEntry must exist in database\n");
                        }

                        continue;
                    }

                    if (rowPtr->upstream != assertPktSrcAddr)
                    {
                        if (NetworkIpIsPartOfMulticastGroup(node,
                                                       rowPtr->grpAddr)
                           ||
                           rowPtr->state != ROUTING_PIM_DM_PRUNE)
                        {
                            /*  Send prune to previous upstream */
                            RoutingPimDmSendJoinPrunePacket(
                                    node,
                                    rowPtr->srcAddr,
                                    rowPtr->grpAddr,
                                    UPSTREAM,
                                    ROUTING_PIM_PRUNE_TREE,
                                    ROUTING_PIM_DM_PRUNE_TIMEOUT);

                            /*  Make new RPF neighbor a upstream */
                            rowPtr->upstream = assertPktSrcAddr;
                            assertSrcListItem->upstreamAddr =
                                                    assertPktSrcAddr;
                            rowPtr->state = ROUTING_PIM_DM_ACKPENDING;

                            /*  Send Graft to new upstream */
                            RoutingPimDmSendGraftPacket(node,
                                                        rowPtr->srcAddr,
                                                        rowPtr->grpAddr);
                        }
                        else
                        {
                            /*  Make new RPF neighbor a upstream */
                            rowPtr->upstream = assertPktSrcAddr;
                            assertSrcListItem->upstreamAddr =
                                                    assertPktSrcAddr;
                        }
                    }
                }

                assertSrcListItem->preference = 
                    (NetworkRoutingAdminDistanceType)
                    RoutingPimDmAssertPacketGetMetricPref(metricBitPref);
                assertSrcListItem->metric = metric;
                setAssertTimer = TRUE;
            }

            if (setAssertTimer)
            {
                if (assertSrcListItem->assertTime == 0)
                {
                    timerInfo.srcAddr = mcastSrcAddr;
                    timerInfo.grpAddr = grpAddress ;
                    RoutingPimSetTimer(
                        node,
                        inIntfIndex,
                        MSG_ROUTING_PimDmAssertTimeOut,
                        (void*) &timerInfo, ROUTING_PIM_DM_ASSERT_TIMEOUT);
                }

                assertSrcListItem->assertTimerRunning = TRUE;
                assertSrcListItem->assertTime = node->getNodeTime() +
                    ROUTING_PIM_DM_ASSERT_TIMEOUT;
            }//end of if
        }//end of else
    }//end of if
    /*  Else if it is downstream interface */
    else
    {
        BOOL localWins;
        BOOL resetAssertTimer = FALSE;

        if (assertSrcListItem->assertState == Pim_Assert_NoInfo)
        {
            //Assume that I am the assert winner

            NetworkDataIp* ip = (NetworkDataIp*)
                node->networkData.networkVar;

            assertSrcListItem->assertState = Pim_Assert_IWonAssert;

            assertSrcListItem->winnerAssertMetric.ipAddress
                = pim->interface[inIntfIndex].ipAddress;

            if (ip->interfaceInfo[inIntfIndex]->routingProtocolType
            == ROUTING_PROTOCOL_NONE)
            {
                assertSrcListItem->winnerAssertMetric.metricPreference = 0;
            }
            else
            {
                assertSrcListItem->winnerAssertMetric.metricPreference =
                        NetworkRoutingGetAdminDistance(node,
                            ip->interfaceInfo[inIntfIndex]
                                ->routingProtocolType);
            }

            assertSrcListItem->winnerAssertMetric.metric =
                NetworkGetMetricForDestAddress(
                    node, assertSrcListItem->srcAddr);
        }

        if (assertSrcListItem->assertState == Pim_Assert_IWonAssert)
        {
            localWins =
                RoutingPimDmCompareMetric(
                    assertSrcListItem->winnerAssertMetric.metricPreference,
                    assertSrcListItem->winnerAssertMetric.metric,
                    assertSrcListItem->winnerAssertMetric.ipAddress,
                    RoutingPimDmAssertPacketGetMetricPref(metricBitPref),
                    metric,
                    assertPktSrcAddr);

            if (localWins)
            {
                //RFC 3963 (Section 4.6.4.1 and 4.6.4.2) Received Inferior
                //assert if I am the winner or no info. Send Assert(S,G)
                //and reset Assert timer.
                RoutingPimDmForwardingTableRow* rowPtr =
                    RoutingPimDmGetFwdTableEntryForThisPair(node,
                                                            mcastSrcAddr,
                                                            grpAddress);

                if (rowPtr != NULL)
                {
                    RoutingPimDmSendAssertPacket(node,
                                                 mcastSrcAddr,
                                                 grpAddress,
                                                 inIntfIndex);

                    resetAssertTimer = TRUE;
                }
            }
            else
            {
                //RFC 3963 (Section 4.6.4.1 and 4.6.4.2) Received preffered
                //assert if I am the winner or no info.

                assertSrcListItem->assertState = Pim_Assert_ILostAssert;

                assertSrcListItem->winnerAssertMetric.ipAddress =
                                                assertPktSrcAddr;

                assertSrcListItem->winnerAssertMetric.metricPreference=
                    (NetworkRoutingAdminDistanceType)
                        RoutingPimDmAssertPacketGetMetricPref(
                            metricBitPref);

                assertSrcListItem->winnerAssertMetric.metric = metric;

                resetAssertTimer = TRUE;

                GrpSetIterator grpSetIterator;
                RoutingPimDmPruneTimerInfo timerInfo;

                RoutingPimDmForwardingTableRow* rowPtr = NULL;
                RoutingPimDmDownstreamListItem* downstreamInfo = NULL;

                for (grpSetIterator = assertSrcListItem->grpSet->begin();
                    grpSetIterator != assertSrcListItem->grpSet->end();
                    grpSetIterator++)
                {
                    rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                                            node,
                                            mcastSrcAddr,
                                            *grpSetIterator);

                    if (rowPtr == NULL)
                    {
                        if (DEBUG_WIRELESS)
                        {
                            ERROR_Assert(FALSE,"\nEntry must exist in database\n");
                        }

                        continue;
                    }

                    downstreamInfo = RoutingPimDmGetDownstreamInfo(
                                                rowPtr,
                                                inIntfIndex);

                    if (downstreamInfo == NULL)
                    {
                        continue;
                    }

                    /*  Prune this interface */
                    downstreamInfo->isPruned = TRUE;
                    downstreamInfo->lastPruneReceived = node->getNodeTime();
                    downstreamInfo->pruneTimer = ROUTING_PIM_DM_PRUNE_TIMEOUT;

#ifdef ADDON_DB
                    //remove this entry from multicast_connectivity cache
                    StatsDBConnTable::MulticastConnectivity stats;

                    stats.nodeId = node->nodeId;
                    stats.destAddr = rowPtr->grpAddr;
                    strcpy(stats.rootNodeType,"Source");
                    stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                    rowPtr->srcAddr);
                    stats.outgoingInterface = downstreamInfo->interfaceIndex;
                    stats.upstreamNeighborId =
                                          MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                   rowPtr->upstream);
                    stats.upstreamInterface =
                                          NetworkIpGetInterfaceIndexFromAddress(node,
                                                                     rowPtr->inIntf);

                    // delete entry from  multicast_connectivity cache
                    STATSDB_HandleMulticastConnDeletion(node, stats);
#endif

                    if (downstreamInfo->pruneTimer !=
                            ROUTING_PIM_INFINITE_HOLDTIME)
                    {
                        /* Set timer to time out prune on this interface */
                        timerInfo.srcAddr = rowPtr->srcAddr;
                        timerInfo.grpAddr = rowPtr->grpAddr;
                        timerInfo.downstreamIntf = downstreamInfo->intfAddr;

                        RoutingPimSetTimer(node,
                            inIntfIndex,
                            MSG_ROUTING_PimDmPruneTimeoutAlarm,
                            (void*) &timerInfo, ROUTING_PIM_DM_PRUNE_TIMEOUT);
                    }

                    if (rowPtr->state != ROUTING_PIM_DM_PRUNE &&
                            RoutingPimDmAllDownstreamPruned(rowPtr) &&
                            !(NetworkIpIsPartOfMulticastGroup(node,
                                rowPtr->grpAddr)))
                    {
                        RoutingPimDmDataTimeoutInfo cacheTimer;

                        rowPtr->state = ROUTING_PIM_DM_PRUNE;

                        /*  Send prune to upstream */
                        RoutingPimDmSendJoinPrunePacket(
                            node,
                            rowPtr->srcAddr,
                            rowPtr->grpAddr,
                            UPSTREAM,
                            ROUTING_PIM_PRUNE_TREE,
                            ROUTING_PIM_DM_PRUNE_TIMEOUT);

                        rowPtr->delayedJoinTimer = FALSE;
                        rowPtr->graftRxmtTimer = FALSE;


                        if (rowPtr->expirationTimer == 0)
                        {
                            cacheTimer.srcAddr = rowPtr->srcAddr;
                            cacheTimer.grpAddr = rowPtr->grpAddr;
                            RoutingPimSetTimer(node,
                                inIntfIndex,
                                MSG_ROUTING_PimDmDataTimeOut,
                                    (void*) &cacheTimer,
                                    (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                        }
                        /*  Set DataTimeout timer */
                        rowPtr->expirationTimer = node->getNodeTime() +
                            ROUTING_PIM_DM_DATA_TIMEOUT;
                    }

                    if (assertSrcListItem->assertState == Pim_Assert_ILostAssert)
                    {
                        //send prune to assert winner
                        RoutingPimDmSendJoinPrunePacket(
                            node,
                            rowPtr->srcAddr,
                            rowPtr->grpAddr,
                            assertSrcListItem->winnerAssertMetric.ipAddress,
                            ROUTING_PIM_PRUNE_TREE,
                            ROUTING_PIM_DM_ASSERT_TIMEOUT);
                    }
                }
            }
        }
        else if (assertSrcListItem->assertState
                                == Pim_Assert_ILostAssert)
        {
            if (assertSrcListItem->winnerAssertMetric.ipAddress ==
                                                            assertPktSrcAddr)
            {
                localWins =
                    RoutingPimDmCompareMetric(
                        NetworkRoutingGetAdminDistance(node,
                            ip->interfaceInfo[inIntfIndex]
                                ->routingProtocolType),
                        NetworkGetMetricForDestAddress(
                            node, mcastSrcAddr),
                        pim->interface[inIntfIndex].ipAddress,
                        RoutingPimDmAssertPacketGetMetricPref(
                            metricBitPref),
                        metric,
                        assertPktSrcAddr);

                if (localWins)
                {
                    //RFC 3963 (Section 4.6.4.3)Receive Inferior Assert
                    //from Current Winner
                    assertSrcListItem->assertState = Pim_Assert_NoInfo;
                    assertSrcListItem->assertTimerRunning = FALSE;
                    assertSrcListItem->assertTime = 0;
                    assertSrcListItem->winnerAssertMetric.metricPreference =
                            ROUTING_ADMIN_DISTANCE_DEFAULT;
                    assertSrcListItem->winnerAssertMetric.metric =
                            ROUTING_PIM_INFINITE_METRIC;
                }
                else
                {
                    resetAssertTimer = TRUE;
                }
            }
            else
            {
                localWins =
                    RoutingPimDmCompareMetric(
                        assertSrcListItem->winnerAssertMetric.metricPreference,
                        assertSrcListItem->winnerAssertMetric.metric,
                        assertSrcListItem->winnerAssertMetric.ipAddress,
                        RoutingPimDmAssertPacketGetMetricPref(metricBitPref),
                        metric,
                        assertPktSrcAddr);

                if (!localWins)
                {
                    assertSrcListItem->winnerAssertMetric.ipAddress =
                        assertPktSrcAddr;
                    assertSrcListItem->winnerAssertMetric.metricPreference=
                        (NetworkRoutingAdminDistanceType)
                            RoutingPimDmAssertPacketGetMetricPref(
                                metricBitPref);

                    assertSrcListItem->winnerAssertMetric.metric = metric;

                    GrpSetIterator grpSetIterator;
                    RoutingPimDmForwardingTableRow* rowPtr = NULL;
                    RoutingPimDmDownstreamListItem* downstreamInfo = NULL;

                    for (grpSetIterator = assertSrcListItem->grpSet->begin();
                        grpSetIterator != assertSrcListItem->grpSet->end();
                        grpSetIterator++)
                    {
                        rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                                                node,
                                                mcastSrcAddr,
                                                *grpSetIterator);

                        if (rowPtr == NULL)
                        {
                            if (DEBUG_WIRELESS)
                            {
                                ERROR_Assert(FALSE,"\nEntry must exist in"
                                                   " database\n");
                            }

                            continue;
                        }

                        downstreamInfo = RoutingPimDmGetDownstreamInfo(
                                                    rowPtr,
                                                    inIntfIndex);

                        if (downstreamInfo == NULL)
                        {
                            continue;
                        }

                        RoutingPimDmPruneTimerInfo timerInfo;

                        /* Prune this interface */
                        downstreamInfo->isPruned = TRUE;
                        downstreamInfo->lastPruneReceived = node->getNodeTime();
                        downstreamInfo->pruneTimer =
                            ROUTING_PIM_DM_PRUNE_TIMEOUT;

#ifdef ADDON_DB
                        //remove this entry from multicast_connectivity cache
                        StatsDBConnTable::MulticastConnectivity stats;

                        stats.nodeId = node->nodeId;
                        stats.destAddr = rowPtr->grpAddr;
                        strcpy(stats.rootNodeType,"Source");
                        stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                        rowPtr->srcAddr);
                        stats.outgoingInterface = downstreamInfo->interfaceIndex;
                        stats.upstreamNeighborId =
                                              MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                       rowPtr->upstream);
                        stats.upstreamInterface =
                                              NetworkIpGetInterfaceIndexFromAddress(node,
                                                                         rowPtr->inIntf);

                        // delete entry from  multicast_connectivity cache
                        STATSDB_HandleMulticastConnDeletion(node, stats);
#endif

                        if (downstreamInfo->pruneTimer !=
                                ROUTING_PIM_INFINITE_HOLDTIME)
                        {
                            /* Set timer to time out prune on this interface */
                            timerInfo.srcAddr = rowPtr->srcAddr;
                            timerInfo.grpAddr = rowPtr->grpAddr;
                            timerInfo.downstreamIntf = downstreamInfo->intfAddr;

                            RoutingPimSetTimer(node,
                                inIntfIndex,
                                MSG_ROUTING_PimDmPruneTimeoutAlarm,
                                (void*) &timerInfo, ROUTING_PIM_DM_PRUNE_TIMEOUT);
                        }

                        if (rowPtr->state != ROUTING_PIM_DM_PRUNE &&
                                RoutingPimDmAllDownstreamPruned(rowPtr) &&
                                !(NetworkIpIsPartOfMulticastGroup(node,
                                    rowPtr->grpAddr)))
                        {
                            RoutingPimDmDataTimeoutInfo cacheTimer;

                            rowPtr->state = ROUTING_PIM_DM_PRUNE;

                            /*  Send prune to upstream */
                            RoutingPimDmSendJoinPrunePacket(
                                node,
                                rowPtr->srcAddr,
                                rowPtr->grpAddr,
                                UPSTREAM,
                                ROUTING_PIM_PRUNE_TREE,
                                ROUTING_PIM_DM_PRUNE_TIMEOUT);

                            rowPtr->delayedJoinTimer = FALSE;
                            rowPtr->graftRxmtTimer = FALSE;

                            if (rowPtr->expirationTimer == 0)
                            {
                                cacheTimer.srcAddr = rowPtr->srcAddr;
                                cacheTimer.grpAddr = rowPtr->grpAddr;
                                RoutingPimSetTimer(node,
                                    inIntfIndex,
                                    MSG_ROUTING_PimDmDataTimeOut,
                                        (void*) &cacheTimer,
                                        (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                            }
                            /*  Set DataTimeout timer */
                            rowPtr->expirationTimer = node->getNodeTime() +
                                ROUTING_PIM_DM_DATA_TIMEOUT;
                        }

                        if (assertSrcListItem->assertState ==
                                            Pim_Assert_ILostAssert)
                        {
                            //send prune to assert winner
                            RoutingPimDmSendJoinPrunePacket(
                                node,
                                rowPtr->srcAddr,
                                rowPtr->grpAddr,
                                assertSrcListItem->winnerAssertMetric.ipAddress,
                                ROUTING_PIM_PRUNE_TREE,
                                ROUTING_PIM_DM_ASSERT_TIMEOUT);
                        }
                    }
                }

                resetAssertTimer = TRUE;
            }

            if (DEBUG_WIRELESS)
            {
                fprintf(stdout, "Node %u says Node %u is Assert Winner\n",
                        node->nodeId,
                        assertSrcListItem->winnerAssertMetric.ipAddress);
            }
        }

        if (resetAssertTimer)
        {
            if (assertSrcListItem->assertTime == 0)
            {
            RoutingPimDmAssertTimerInfo timerInfo;
            timerInfo.srcAddr = assertSrcListItem->srcAddr;
            timerInfo.grpAddr = grpAddress;
            RoutingPimSetTimer(
                node,
                inIntfIndex,
                MSG_ROUTING_PimDmAssertTimeOut,
                (void*) &timerInfo,
                ROUTING_PIM_DM_ASSERT_TIMEOUT);
            }

            assertSrcListItem->assertTimerRunning = TRUE;
            assertSrcListItem->assertTime = node->getNodeTime() +
                            ROUTING_PIM_DM_ASSERT_TIMEOUT;
        }

        GrpSetIterator grpSetIterator;
        RoutingPimDmDataTimeoutInfo cacheTimer;

        RoutingPimDmForwardingTableRow* rowPtr = NULL;

        for (grpSetIterator = assertSrcListItem->grpSet->begin();
             grpSetIterator != assertSrcListItem->grpSet->end();
             grpSetIterator++)
        {
            if (assertSrcListItem->assertState == Pim_Assert_NoInfo
                &&
                rowPtr->state == ROUTING_PIM_DM_PRUNE)
            {
                rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                                        node,
                                        assertSrcListItem->srcAddr,
                                        *grpSetIterator);

                if (rowPtr == NULL)
                {
                    if (DEBUG_WIRELESS)
                    {
                        ERROR_Assert(FALSE,"\nEntry must exist in database\n");
                    }

                    continue;
                }


                rowPtr->state = ROUTING_PIM_DM_ACKPENDING;
                RoutingPimDmSendGraftPacket(node,
                                            rowPtr->srcAddr,
                                            rowPtr->grpAddr);

                if (rowPtr->expirationTimer == 0)
                {
                    cacheTimer.srcAddr = rowPtr->srcAddr;
                    cacheTimer.grpAddr = rowPtr->grpAddr;
                    RoutingPimSetTimer(node,
                        inIntfIndex,
                        MSG_ROUTING_PimDmDataTimeOut,
                        (void*) &cacheTimer,
                        (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                }
                /*  Set DataTimeout timer */
                rowPtr->expirationTimer = node->getNodeTime() +
                    ROUTING_PIM_DM_DATA_TIMEOUT;
            }
        }
    }
}
