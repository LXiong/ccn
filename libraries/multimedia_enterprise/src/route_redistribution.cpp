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


#include "api.h"
#include "fileio.h"
#include "qualnet_error.h"
#include "network_ip.h"
#include "routing_ospfv2.h"
#include "routing_rip.h"
#include "routing_bgp.h"
#include "routing_igrp.h"
#include "routing_eigrp.h"
#include "route_parse_util.h"
#include "route_redistribution.h"
#include "route_map.h"

#define     DEBUG  0 
#define     DEBUG_ROUTE_MAP 0
#define     DEBUG_EIGRP   0


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeGetRoutingTableUpdateFunction
// PURPOSE: Gets the routing table update function to be used by route
//          redistribution
// ARGUMENTS:
//          node:: The current node
//          routingTableUpdateFunction:: Function pointer to update the
//          routing table by a routing protocol
//          interfaceIndex:: Interface index
// RETURN: routingTableUpdateFunction:: A function pointer of the routing
//         table update function for redistribution
// ------------------------------------------------------------------------

RoutingTableUpdateFunction RouteRedistributeGetRoutingTableUpdateFunction(
    Node* node,
    NetworkRoutingProtocolType routingProtocol)
{
    int i = 0;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ip->interfaceInfo[i]->routingProtocolType == routingProtocol)
        {
            return (ip->interfaceInfo[i]->routingTableUpdateFunction);
        }
    }

    return (NULL);
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeSetRoutingTableUpdateFunction
// PURPOSE: Sets the routing table update function to be used by route
//          redistribution
// ARGUMENTS:
//          node:: The current node
//          routingTableUpdateFunction:: Function pointer to update the
//          routing table by a routing protocol
//          interfaceIndex:: Interface index
// RETURN: None
// ------------------------------------------------------------------------

void RouteRedistributeSetRoutingTableUpdateFunction(
    Node* node,
    RoutingTableUpdateFunction routingTableUpdateFunction,
    int interfaceIndex)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    ERROR_Assert(
        ip->interfaceInfo[interfaceIndex]->routingTableUpdateFunction ==
        NULL, "Route redistribution routing table update function "
        "already set\n");

    ip->interfaceInfo[interfaceIndex]->routingTableUpdateFunction =
         routingTableUpdateFunction;
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributePrintProtocol
// PURPOSE: Prints the routing protocol name
// ARGUMENTS:
//          node:: Node type
//          routingProtocol:: Routing protocol type
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributePrintProtocol(
    Node* node,
    NetworkRoutingProtocolType routingProtocol)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    RouteRedistribute* info = (RouteRedistribute*)
        ip->rtRedistributeInfo;

    switch (routingProtocol)
    {
        case ROUTING_PROTOCOL_RIP:
        {
            if (info->versionReceiver == RIP_VERSION_1
                 || info->versionDistributor == RIP_VERSION_1)
            {
                printf("RIPv1\n");
            }
            else
            {
                printf("RIPv2\n");
            }

            break;
        }
        case ROUTING_PROTOCOL_OSPFv2:
        {
            printf("OSPFv2\n");
            break;
        }
        case ROUTING_PROTOCOL_IGRP:
        {
            printf("IGRP\n");
            break;
        }
        case ROUTING_PROTOCOL_EIGRP:
        {
            printf("EIGRP\n");
            break;
        }
        case EXTERIOR_GATEWAY_PROTOCOL_EBGPv4:
        {
            printf("BGPv4\n");
            break;
        }
        case ROUTING_PROTOCOL_BELLMANFORD:
        {
            printf("BELLMANFORD\n");
            break;
        }               

        default:
            ERROR_Assert(NULL, "Unknown Routing Protocol!!!\n");
    }
}


// ------------------------------------------------------------------------
//
// FUNCTION     : ConvertCharArrayToInt
// PURPOSE      : Converting the content of a charecter type array to
//                an unsigned integer
// ARGUMENTS    : arr:: The charecter type array
// RETURN VALUE : The unsigned int
// ASSUMPTION   : The input array is exactly 3-byte long.
// ------------------------------------------------------------------------

static
unsigned int ConvertCharArrayToInt(unsigned char arr[])
{
    unsigned value = ((0xFF & arr[2]) << 16)
                     + ((0xFF & arr[1]) << 8)
                     + (0xFF & arr[0]);
    return value;
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributePrintRouteInfo
// PURPOSE: Prints the redistributing route information
// ARGUMENTS:
//          node:: The current node
//          destAddress:: IP address of destination network or host
//          destAddressMask:: Netmask
//          nextHopAddress:: Next hop IP address
//          interfaceIndex:: Interface to get hooked from
//          cost:: Cost metric associated with the route
//          info:: Pointer to redistribution structure
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributePrintRouteInfo(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    int cost,
    RouteRedistribute* info)
{
    char buf[MAX_STRING_LENGTH];

    printf("\n--------------------------------------------------\n");
    printf("Node Id\t\t\t\t= %u\n", node->nodeId);

    IO_ConvertIpAddressToString(destAddress, buf);
    printf("DestAddress\t\t\t= %s\n", buf);

    IO_ConvertIpAddressToString(nextHopAddress, buf);
    if (nextHopAddress == (unsigned) NETWORK_UNREACHABLE)
    {
        printf("NextHopAddress\t\t\t= %s (UNREACHABLE)\n", buf);
    }
    else
    {
        printf("NextHopAddress\t\t\t= %s\n", buf);
    }

    IO_ConvertIpAddressToString(destAddressMask, buf);
    printf("DestAddressMask\t\t\t= %s\n", buf);

    if (info->receiverProtocolType == ROUTING_PROTOCOL_IGRP)
    {
        IgrpRoutingInfo* rtInfo =
            (IgrpRoutingInfo*) info->genericMetricPtr;
        if (rtInfo!=NULL)
        {
            printf("Metric\t\t\t\t= %19u %13u %5u %8u %4u\n",
                ConvertCharArrayToInt(rtInfo->bandwidth),
                ConvertCharArrayToInt(rtInfo->delay),
                rtInfo->reliability,
                rtInfo->load,
                rtInfo->mtu);
        }
    }
    else
    if (info->receiverProtocolType == ROUTING_PROTOCOL_EIGRP)
    {
    //The statistics for route redistribution
    //of EIGRP METRIC
        EigrpMetric* rtInfo =
            (EigrpMetric*) info->genericMetricPtr;
        if (rtInfo!=NULL)
        {
            printf("Metric\t\t\t\t= %4u %4u %5hu %5u %5u\n",
            rtInfo->bandwidth,
            rtInfo->delay,
            rtInfo->reliability,
            rtInfo->load,
            ConvertCharArrayToInt(rtInfo->mtu));
        }
    }
    else
    {
        printf("Metric\t\t\t\t= %d\n", cost);
    }

    printf("InterfaceIndex\t\t\t= %d\n", interfaceIndex);

    printf("Distributing Protocol Name\t= ");
    RouteRedistributePrintProtocol(node, info->distributorProtocolType);

    printf("Receiving Protocol Name\t\t= ");
    RouteRedistributePrintProtocol(node, info->receiverProtocolType);

    TIME_PrintClockInSecond(info->startTime, buf);
    printf("Redistribution Start Time[s]\t= %s\n", buf);

    TIME_PrintClockInSecond(info->endTime, buf);
    printf("Redistribution End Time[s]\t= %s\n", buf);

    if (info->removalIsEnabled == TRUE)
    {
        TIME_PrintClockInSecond(info->removalStartTime, buf);
        printf("Redistribution Blocking Start Time[s] = %s\n", buf);

        TIME_PrintClockInSecond(info->removalEndTime, buf);
        printf("Redistribution Blocking End Time[s] = %s\n", buf);
    }

    TIME_PrintClockInSecond(node->getNodeTime(), buf);
    printf("Current Sim Time[s]\t\t= %s\n", buf);
    printf("--------------------------------------------------\n");

}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeCallHookFunction
// PURPOSE: Calls the proper hooking function depending on the receiver
//          protocol
// ARGUMENTS:
//          node:: The current node
//          destAddress:: IP address of destination network or host
//          destAddressMask:: Netmask
//          nextHopAddress:: Next hop IP address
//          interfaceIndex:: Interface Index
//          info:: Route redistribute structure pointer
//          cost:: Cost metric associated with the route
//          routingProtocolType:: Distributing protocol
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributeCallHookFunction(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    RouteRedistribute* info,
    int metric,
    NetworkRoutingProtocolType routingProtocolType)
{

    RoutingTableUpdateFunction rtTableUpdateFuncPtr = NULL;

    // Check the protocol type and call the appropriate hooking function.

    ERROR_Assert(info->distributorProtocolType == routingProtocolType,
        "Distributing protocol did not match.\n");


    rtTableUpdateFuncPtr = RouteRedistributeGetRoutingTableUpdateFunction
        ( node, info->receiverProtocolType);

    // In place of adding the below code, a better approach
    // would be to use redistribution of default routes. However since
    // redistribution of default routes is not supported we have to update
    // forwarding table in routing_rip.cpp RipInit(...) function. The
    // problem with this function is that, it might be possible that the
    // protocol that RIP is redistributing the routes too might not be
    // initialized leading to assert.This happens only with Bellmanford
    // routing protocol. So bypassing the assert in this case.
    if (!(((info->distributorProtocolType == ROUTING_PROTOCOL_RIP) &&
        (info->receiverProtocolType == ROUTING_PROTOCOL_BELLMANFORD)) ||
        ((info->receiverProtocolType == ROUTING_PROTOCOL_RIP) &&
        (info->distributorProtocolType == ROUTING_PROTOCOL_BELLMANFORD))))
    {
        ERROR_Assert(rtTableUpdateFuncPtr != NULL, "Routing Table update "
            "function has not been set.\n");
    }
    if (info->receiverProtocolType == ROUTING_PROTOCOL_OSPFv2)
    {
        // If Metric option has been specified then this will be
        // distributed unless default metric has been set
        (rtTableUpdateFuncPtr) (
            node,
            destAddress,
            destAddressMask,
            nextHopAddress,
            interfaceIndex,
            (int*) &info->metric);

        if (DEBUG)
        {
            RouteRedistributePrintRouteInfo(
                node,
                destAddress,
                destAddressMask,
                nextHopAddress,
                interfaceIndex,
                info->metric,
                info);

            printf("Above Route Is Delivered.\n");
        }
    }
    else
    if (info->receiverProtocolType == ROUTING_PROTOCOL_RIP)
    {
        int cost = info->metric;

        if (cost == ROUTE_COST)
        {
            cost = metric;
        }
        else
        {
            cost = info->metric;
        }
        if (rtTableUpdateFuncPtr)
        {
            (rtTableUpdateFuncPtr) (
                node,
                destAddress,
                destAddressMask,
                nextHopAddress,
                interfaceIndex,
                (int*) &cost);
        }

        if (DEBUG)
        {
            RouteRedistributePrintRouteInfo(
                node,
                destAddress,
                destAddressMask,
                nextHopAddress,
                interfaceIndex,
                cost,
                info);

            printf("Above Route Is Delivered.\n");
        }
    }
    else
    if (info->receiverProtocolType == ROUTING_PROTOCOL_BELLMANFORD)
    {
        int cost = info->metric;

        if (cost == ROUTE_COST)
        {
            cost = metric;
        }
        if (rtTableUpdateFuncPtr)
        {
            (rtTableUpdateFuncPtr) (
                node,
                destAddress,
                destAddressMask,
                nextHopAddress,
                interfaceIndex,
                (int*) &cost);
        }
        if (DEBUG)
        {
            RouteRedistributePrintRouteInfo(
                node,
                destAddress,
                destAddressMask,
                nextHopAddress,
                interfaceIndex,
                metric,
                info);

            printf("Above Route Is Delivered.\n");
        }
    }
    else
    if (info->receiverProtocolType == ROUTING_PROTOCOL_IGRP)
    {
        (rtTableUpdateFuncPtr) (
            node,
            destAddress,
            destAddressMask,
            nextHopAddress,
            interfaceIndex,
            (IgrpRoutingInfo*) info->genericMetricPtr);

        if (DEBUG)
        {
            RouteRedistributePrintRouteInfo(
                node,
                destAddress,
                destAddressMask,
                nextHopAddress,
                interfaceIndex,
                metric,
                info);

            printf("Above Route Is Delivered.\n");
        }
    }
    else
    if (info->receiverProtocolType == ROUTING_PROTOCOL_EIGRP)
    {
        (rtTableUpdateFuncPtr) (
            node,
            destAddress,
            destAddressMask,
            nextHopAddress,
            interfaceIndex,
            (EigrpMetric*) info->genericMetricPtr);

        if (DEBUG)
        {
            RouteRedistributePrintRouteInfo(
                node,
                destAddress,
                destAddressMask,
                nextHopAddress,
                interfaceIndex,
                metric,
                info);

            printf("Above Route Is Delivered.\n");
        }
    }        
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeValidateProtocols
// PURPOSE: This function returns TRUE if only both the routing protocols
//          passed as arguments are enabled on the node otherwise returns
//          FALSE
// ARGUMENTS:
//          node:: The current node
//          routingProtocolType1:: Type value of routing protocol
//          routingProtocolType2:: Type value of routing protocol
// RETURN: BOOL
// ------------------------------------------------------------------------

static
BOOL RouteRedistributeValidateProtocols(
    Node* node,
    NetworkRoutingProtocolType routingProtocolType1,
    NetworkRoutingProtocolType routingProtocolType2)
{
    int i = 0;
    BOOL flag1 = FALSE;
    BOOL flag2 = FALSE;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if ((ip->interfaceInfo[i]->routingProtocolType ==
            routingProtocolType1) && (flag1 == FALSE))
        {
            flag1 = TRUE;
            continue;
        }
        else
        if (ip->interfaceInfo[i]->routingProtocolType ==
            routingProtocolType2)
        {
            flag2 = TRUE;
        }


    }

    if ((flag1 == TRUE) && (flag2 == TRUE))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeAddHook
// PURPOSE: Plugging to the route redistribution
// ARGUMENTS:
//          node:: The current node
//          destAddress:: IP address of destination network or host
//          destAddressMask:: Netmask
//          nextHopAddress:: Next hop IP address
//          interfaceIndex:: Interface to get hooked from
//          cost:: Cost metric associated with the route
//          routingProtocolType:: Type value of routing protocol
// RETURN: None
// ------------------------------------------------------------------------

void RouteRedistributeAddHook(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    int cost,
    NetworkRoutingProtocolType routingProtocolType)

{

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    RouteRedistribute* info = (RouteRedistribute*)
        ip->rtRedistributeInfo;

    // in case it is a EXTERANL ROUTES FOR OSPFv2, we still want to redistributed
    if ((routingProtocolType == ROUTING_PROTOCOL_OSPFv2_TYPE1_EXTERNAL) ||
        (routingProtocolType == ROUTING_PROTOCOL_OSPFv2_TYPE2_EXTERNAL))
    {
        routingProtocolType = ROUTING_PROTOCOL_OSPFv2;
    }

    if (info->distributorProtocolType == ROUTING_PROTOCOL_RIP)
     {
        RipData* dataPtr = (RipData*) node->appData.routingVar;
        if (!((dataPtr->version ==(unsigned int) info->versionReceiver)
            || (dataPtr->version == (unsigned int) info->versionDistributor)))
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "Node: %u, Distributing protocol for route "
                " redistribution is not running in the node.\n",
                node->nodeId);
            ERROR_ReportError(errString);
        }
     }

    while (info != NULL)
    {
        if (info->distributorProtocolType == routingProtocolType)
        {

            RedistributeStats* stats = (RedistributeStats*)
                info->rtRedistributeStats;

            clocktype startTime = info->startTime;
            clocktype endTime = info->endTime;
            clocktype blockStart = info->removalStartTime;
            clocktype blockEnd = info->removalEndTime;
            clocktype currentTime = node->getNodeTime();
            BOOL proceed = FALSE;

            stats->totalRoutesFound++;
            stats->interfaceIndex = interfaceIndex;

            // Route blocking is not enabled
            if (info->removalIsEnabled == FALSE)
            {
                if (currentTime < startTime || currentTime > endTime)
                {
                    proceed = FALSE;
                }
                else
                {
                    proceed = TRUE;
                }
            }
            // 'NO REDISTRIBUTE' is enabled
            else
            {
                if ((currentTime >= blockStart && currentTime <= blockEnd)
                    || (currentTime < startTime || currentTime > endTime))
                {
                    proceed = FALSE;
                }
                else
                {
                    proceed = TRUE;
                }
            }

            if (proceed == TRUE)
            {

                if ((info->distributorProtocolType !=
                    EXTERIOR_GATEWAY_PROTOCOL_EBGPv4)
                    && (info->distributorProtocolType !=
                           ROUTING_PROTOCOL_STATIC)
                    && (info->distributorProtocolType !=
                           ROUTING_PROTOCOL_DEFAULT))
                {
                    BOOL flag = FALSE;

                    // Check whether both the protocols are
                    // enabled at the node.

                    flag = RouteRedistributeValidateProtocols(
                                node,
                                info->receiverProtocolType,
                                info->distributorProtocolType);

                    ERROR_Assert(flag != FALSE, "Either anyone or "
                        "both the protocols are not running at node "
                        "specified in the router-config file.\n");
                }

                // If route map is present
                if (info->rMap)
                {
                    BOOL rtMapPermit = FALSE; // Decision of route map
                    BOOL flag = FALSE;
                    RouteMap* routeMap;
                    RouteMapEntry* entry;
                    RouteMapValueSet* valueList = NULL;

                    routeMap = info->rMap;
                    entry = (RouteMapEntry*) routeMap->entryHead;

                    // Route map is enabled
                    // We init the value list
                    valueList = (RouteMapValueSet*)
                        RouteMapInitValueList();

                    if (DEBUG_ROUTE_MAP)
                    {
                        printf("**************************************"
                            "************\n");
                        printf("Route Information given to Route Map.");
                        RouteRedistributePrintRouteInfo(
                            node,
                            destAddress,
                            destAddressMask,
                            nextHopAddress,
                            interfaceIndex,
                            cost,
                            info);
                    }

                    // Fill the match values of the valueList before
                    // calling the action function
                    valueList->matchCost = cost;
                    valueList->matchSrcAddress = destAddress;
                    valueList->matchDestAddress = destAddressMask;
                    valueList->matchDestAddNextHop = nextHopAddress;

                    // Set the default value of external cost if
                    // Route Map criteria does not match. In that
                    // case default value should be taken.
                    valueList->setNextHop = nextHopAddress;
                    valueList->setMetric = cost;

                    // Check the clauses from each route map entry
                    while (entry)
                    {
                        flag = RouteMapAction(node, valueList, entry);

                        //permit
                        if ((entry->type == ROUTE_MAP_PERMIT) &&
                            (flag == TRUE))
                        {
                            // match
                            rtMapPermit = TRUE;
                            break;
                        }

                        entry = entry->next;
                    }
                    // Extract the set values of valueList.
                    nextHopAddress = valueList->setNextHop;
                    info->metric = valueList->setMetric;

                    // If work of valueList is over then we free it
                    MEM_free(valueList);

                    // 'match' clause has been satisfied by the
                    // Route Map so redistribribute the routes.
                    if (rtMapPermit == TRUE)
                    {
                        if (DEBUG_ROUTE_MAP)
                        {
                            RouteRedistributePrintRouteInfo(
                                node,
                                destAddress,
                                destAddressMask,
                                nextHopAddress,
                                interfaceIndex,
                                info->metric,
                                info);

                            printf("Route Map permitted the above "
                                "route.\n");
                            printf("*********************************"
                                "*****************\n");
                        }

                        if (nextHopAddress == (unsigned) NETWORK_UNREACHABLE)
                        {
                            // Route is become unreachable and
                            // should be reflected to the stat.
                            stats->numInvalidRoutesInformed++;
                        }
                        else
                        {
                            stats->numNewRoutesDistributed++;
                        }

                        RouteRedistributeCallHookFunction(
                            node,
                            destAddress,
                            destAddressMask,
                            nextHopAddress,
                            interfaceIndex,
                            info,
                            info->metric,
                            routingProtocolType);
                    }
                    // Route map did not permit to redistribute
                    // the route. So only stat is to be updated.
                    else
                    {
                        stats->numRoutesDiscarded++;

                        if (DEBUG_ROUTE_MAP)
                        {
                            printf("Route Map discarded the above "
                                "route.\n");
                            printf("*********************************"
                                "*****************\n");
                        }
                    }
                }
                // Route Map is not enabled
                else
                {
                    if (nextHopAddress == (unsigned) NETWORK_UNREACHABLE)
                    {
                        // Route is become unreachable and
                        // should be reflected to the stat.
                        stats->numInvalidRoutesInformed++;
                    }
                    else
                    {
                        stats->numNewRoutesDistributed++;
                    }

                    RouteRedistributeCallHookFunction(
                        node,
                        destAddress,
                        destAddressMask,
                        nextHopAddress,
                        interfaceIndex,
                        info,
                        cost,
                        routingProtocolType);
                }
            }
            // Current sim time does not permit to redistribute
            // the route.
            else
            {
                if (DEBUG)
                {
                    RouteRedistributePrintRouteInfo(
                        node,
                        destAddress,
                        destAddressMask,
                        nextHopAddress,
                        interfaceIndex,
                        info->metric,
                        info);

                    printf("Above Route Is Blocked\n");
                }

                if (info->removalIsEnabled == FALSE)
                {
                    stats->numRoutesDiscardedForSimTime++;
                }
                else
                {
                    if ((currentTime >= blockStart) &&
                        (currentTime <= blockEnd))
                    {
                        stats->numRoutesBlocked++;
                    }
                    else
                    {
                        stats->numRoutesDiscardedForSimTime++;
                    }
                }
            }
        }

        info = info->next;
    }
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributePrintCommonStat
// PURPOSE: Prints out the common(excluding protocol name) statistics at
//          the end of the simulation
// ARGUMENTS:
//          node:: The current node
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributePrintCommonStat(
    Node* node,
    RouteRedistribute* info)
{
    char buf[MAX_STRING_LENGTH];

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    RedistributeStats* stats = (RedistributeStats*)
        info->rtRedistributeStats;

    int interfaceIndex = stats->interfaceIndex;

    sprintf(buf, "Total Routes Found = %d",
            stats->totalRoutesFound);
    IO_PrintStat(node, "Network", "Rt Redist",
            ip->interfaceInfo[interfaceIndex]->ipAddress, -1, buf);

    sprintf(buf, "Routes Redistributed = %d",
            stats->numNewRoutesDistributed);
    IO_PrintStat(node, "Network", "Rt Redist",
            ip->interfaceInfo[interfaceIndex]->ipAddress, -1, buf);

    sprintf(buf, "Unreachable Routes Informed = %d",
            stats->numInvalidRoutesInformed);
    IO_PrintStat(node, "Network", "Rt Redist",
            ip->interfaceInfo[interfaceIndex]->ipAddress, -1, buf);

    sprintf(buf, "Routes Discarded for Time Range = %d",
            stats->numRoutesDiscardedForSimTime);
    IO_PrintStat(node, "Network", "Rt Redist",
            ip->interfaceInfo[interfaceIndex]->ipAddress, -1, buf);

    if (info->rMap)
    {
        sprintf(buf, "Routes Discarded by Route Map = %d",
                stats->numRoutesDiscarded);
        IO_PrintStat(node, "Network", "Rt Redist",
                ip->interfaceInfo[interfaceIndex]->ipAddress, -1, buf);
    }

    if (info->removalIsEnabled == TRUE)
    {
        sprintf(buf, "Routes Blocked for NO REDISTRIBUTE = %d",
            stats->numRoutesBlocked);
        IO_PrintStat(node, "Network", "Rt Redist",
                ip->interfaceInfo[interfaceIndex]->ipAddress, -1, buf);
    }
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeInitializeVariables
// PURPOSE: Initializes the variables of redistribution structure
// ARGUMENTS:
//          info:: Pointer to redistribution structure
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributeInitializeVariables(RouteRedistribute* info)
{
    info->distributorProtocolType = ROUTING_PROTOCOL_NONE;
    info->receiverProtocolType = ROUTING_PROTOCOL_NONE;
    info->removalIsEnabled = FALSE;
    // default is set to TRUE
    info->genericMetricPtr = NULL;
    info->next = NULL;
    info->metric = 0;
    info->startTime = 0;
    info->endTime = 0;
    info->removalStartTime = 0;
    info->removalEndTime = 0;
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeInitializeStatVariables
// PURPOSE: Initializes the statistics variables
// ARGUMENTS:
//          info:: Pointer to redistribution structure
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributeInitializeStatVariables(RouteRedistribute* info)
{
    info->rtRedistributeStats = (RedistributeStats*)
        RtParseMallocAndSet(sizeof(RedistributeStats));

    RedistributeStats* stats = (RedistributeStats*)
        info->rtRedistributeStats;

    stats->numNewRoutesDistributed = 0;
    stats->numInvalidRoutesInformed = 0;
    stats->numRoutesDiscarded = 0;
    stats->totalRoutesFound = 0;
    stats->numRoutesBlocked = 0;
    stats->numRoutesDiscardedForSimTime = 0;
    stats->interfaceIndex = DEFAULT_INTERFACE;
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeStatInit
// PURPOSE: Initialize statistics for the Route Redistribution
// ARGUMENTS:
//          node:: The current node
//          nodeInput:: Main configuration file
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributeStatInit(
    Node* node,
    const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTE-REDISTRIBUTION-STATISTICS",
        &retVal,
        buf);

    if (retVal == FALSE)
    {
        ip->isRtRedistributeStatOn = FALSE;
    }
    else
    {
        if (strcmp(buf, "YES") == 0)
        {
            ip->isRtRedistributeStatOn = TRUE;
        }
        else
        if (strcmp(buf, "NO") == 0)
        {
            ip->isRtRedistributeStatOn = FALSE;
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "Node %2u: Expecting YES or NO for "
                "ROUTE-REDISTRIBUTE-STATISTICS parameter in configuration "
                "file.\n", node->nodeId);
            ERROR_ReportError(errString);
        }
    }
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeParseSimTime
// PURPOSE: Parse the simulation start time end time
// ARGUMENTS:
//          node:: The current node
//          command:: Redistribution Command type
//          startTimeString:: Start of the simulation
//          endTimeString:: End of the simulation
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributeParseSimTime(
    Node* node,
    RedistributeCommandType command,
    char* startTimeString,
    char* endTimeString)
{

    clocktype startTime = 0;
    clocktype endTime = 0;
    clocktype simTime = 0;

    int lenStartTimeString = 0;
    int lenEndTimeString = 0;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    // Get the Simulation Time which is specified in
    // main configuration file

    simTime = TIME_ReturnMaxSimClock(node);

    lenStartTimeString = (int)strlen(startTimeString);
    lenEndTimeString = (int)strlen(endTimeString);

    if (lenStartTimeString != 0 && lenEndTimeString == 0)
    {
        ERROR_ReportError("<End Time> is missing in the REDISTRIBUTE "
            "command.\n");
    }

    // <Start Time> and <End Time> are not at all specified. Hence
    // REDISTRIBUTION will run from start to end of the Simulation.
    else
    if (lenStartTimeString == 0 && lenEndTimeString == 0)
    {
        if ((ip->rtRedistributeInfo->distributorProtocolType ==
            ROUTING_PROTOCOL_STATIC) ||
            (ip->rtRedistributeInfo->distributorProtocolType ==
            ROUTING_PROTOCOL_DEFAULT))
        {
           startTime = 1;
        }
        else
        {
        startTime = 0;
        }
        endTime = simTime;
    }

    // <Start Time> and <End Time> both are specified. So check the values
    // and assign them to the Redistribution structure.
    else
    {
        startTime = TIME_ConvertToClock(startTimeString);
        endTime = TIME_ConvertToClock(endTimeString);

        if (startTime < 0)
        {
            ERROR_ReportError("<Start Time> should not be less than 0\n");
        }

        // Specified <End Time> should not be greater than Simulation Time.
        else
        if (endTime > simTime && endTime != 0)
        {
            ERROR_ReportError("<End Time> should not be greater than "
                "Simulation Time\n");
        }
        else
        if (endTime < startTime && endTime != 0)
        {
            ERROR_ReportError("<End Time> should be greater than "
                "<Start Time>\n");
        }

        // Given <End Time> is equal to 0. So Redistribution will take
        // effect till end of the Simulation.
        if (endTime == 0)
        {
            endTime = simTime;
        }
    }

    if (command == ROUTE_REDISTRIBUTE)
    {
        ip->rtRedistributeInfo->startTime = startTime;
        ip->rtRedistributeInfo->endTime = endTime;

        Message* newMsg = NULL;

        newMsg = MESSAGE_Alloc(node,
                                NETWORK_LAYER,
                                NETWORK_ROUTE_REDISTRIBUTION,
                                MSG_NETWORK_RedistributeData);


        MESSAGE_Send(node, newMsg, startTime);
    }
    else
    if (command == ROUTE_NO_REDISTRIBUTE)
    {
        ip->rtRedistributeInfo->removalIsEnabled = TRUE;
        ip->rtRedistributeInfo->removalStartTime = startTime;
        ip->rtRedistributeInfo->removalEndTime = endTime;
    }
    else
    {
        ERROR_Assert(FALSE, "Unknown command type !!!\n");
    }
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeValidateReceiverProtocolType
// PURPOSE: Initializes and validates the receiver protocol
// ARGUMENTS:
//          node:: The current node
//          nodeInput:: Main configuration file
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributeValidateReceiverProtocolType(
    Node* node,
    char* protocolName,
    RouteRedistribute* info)
{

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    if (strlen(protocolName) == 0)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "You must specify the protocol name."
        "\nThe format is ROUTER <Protocol>.\n");
        ERROR_ReportError(errString);
    }

    if (RtParseStringNoCaseCompare(protocolName, "OSPFv2") == 0)
    {
        info->receiverProtocolType = ROUTING_PROTOCOL_OSPFv2;

        // set metric = 20 for the external routes
        // to be redistributed to OSPF netwrok.
        info->metric = OSPFV2_DEFAULT_EXTERNAL_COST;
    }
    else
    if (RtParseStringNoCaseCompare(protocolName, "IGRP") == 0)
    {
        info->receiverProtocolType = ROUTING_PROTOCOL_IGRP;
    }
    else
    if (RtParseStringNoCaseCompare(protocolName, "RIPv1") == 0
         || RtParseStringNoCaseCompare(protocolName, "RIPv2") == 0)
    {
        if (RtParseStringNoCaseCompare(protocolName, "RIPv1") == 0)
        {
            info->versionReceiver = RIP_VERSION_1;
        }
        else
        {
            info->versionReceiver = RIP_VERSION_2;
        }
        info->receiverProtocolType = ROUTING_PROTOCOL_RIP;
        info->metric = ROUTE_COST;
    }
    // Bellmanford as receiver protocol
    else
    if (RtParseStringNoCaseCompare(protocolName, "BELLMANFORD") == 0)
    {
        info->receiverProtocolType = ROUTING_PROTOCOL_BELLMANFORD;
        info->metric = ROUTE_COST;
    }
    else
    if (RtParseStringNoCaseCompare(protocolName, "EIGRP") == 0)
    {
        info->receiverProtocolType = ROUTING_PROTOCOL_EIGRP;

    }      
    else
    if (RtParseStringNoCaseCompare(protocolName, "BGPv4") == 0)
    {
        ERROR_ReportError("Need not redistribute the routes to BGPv4, "
            "this will in turn take care of that.\n");
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Route Redistribution to '%s' is "
        " not yet supported.\n", protocolName);
        ERROR_ReportError(errString);
    }

    if (ip->rtRedistributeInfo == NULL)
    {
        ip->rtRedistributeInfo = info;
    }
    else
    {
        info->next = ip->rtRedistributeInfo;
        ip->rtRedistributeInfo = info;
    }
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeValidateDistributorProtocolType
// PURPOSE: Validates and assigns the distributor protocol to
//          redistribution structure
// ARGUMENTS:
//          node:: The current node
//          protocolName:: Protocol name to be validated
//          originalString:: Entire command string for error display
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributeValidateDistributorProtocolType(
    Node* node,
    char* protocolName,
    char* originalString)
{

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    RouteRedistribute* info = (RouteRedistribute*) ip->rtRedistributeInfo;

    if (strlen(protocolName) == 0)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nProtocol name is missing in router-config "
        "file.\n", originalString);
        ERROR_ReportError(errString);
    }

    // Route redistribution of RIP is enabled
    if (RtParseStringNoCaseCompare(protocolName, "RIPv1") == 0
        ||RtParseStringNoCaseCompare(protocolName, "RIPv2") == 0)
    {
        if (RtParseStringNoCaseCompare(protocolName, "RIPv1") == 0)
        {
            info->versionDistributor = RIP_VERSION_1;
        }
        else
        {
            info->versionDistributor = RIP_VERSION_2;
        }
        info->distributorProtocolType = ROUTING_PROTOCOL_RIP;
    }
    // Route redistribution of BELLMANFORD has enabled
    else
    if (RtParseStringNoCaseCompare(protocolName, "BELLMANFORD") == 0)
    {
        info->distributorProtocolType = ROUTING_PROTOCOL_BELLMANFORD;
    }
    else
    if (RtParseStringNoCaseCompare(protocolName, "IGRP") == 0)
    {
        info->distributorProtocolType = ROUTING_PROTOCOL_IGRP;
    }
    else
    if (RtParseStringNoCaseCompare(protocolName, "EIGRP") == 0)
    {
        info->distributorProtocolType = ROUTING_PROTOCOL_EIGRP;
    }
    else
    if (RtParseStringNoCaseCompare(protocolName, "OSPFv2") == 0)
    {
        info->distributorProtocolType = ROUTING_PROTOCOL_OSPFv2;
    }
    else
    if (RtParseStringNoCaseCompare(protocolName, "BGPv4") == 0)
    {
        info->distributorProtocolType = EXTERIOR_GATEWAY_PROTOCOL_EBGPv4;
    }        
    else
    if (RtParseStringNoCaseCompare(protocolName, "STATIC") == 0)
    {
        info->distributorProtocolType = ROUTING_PROTOCOL_STATIC;
    }
    else
    if (RtParseStringNoCaseCompare(protocolName, "DEFAULT") == 0)
    {
        info->distributorProtocolType = ROUTING_PROTOCOL_DEFAULT;
    }
    else
    {
        char errString[MAX_STRING_LENGTH];

        if ((RtParseStringNoCaseCompare(protocolName, "METRIC") == 0) ||
            (RtParseStringNoCaseCompare(protocolName, "ROUTE-MAP") == 0))
        {
            sprintf(errString, "\n\"%s\"\nProtocol name is missing.\n",
                originalString);

            ERROR_ReportError(errString);
        }

        sprintf(errString, "\n\"%s\"\nRoute Redistribution of %s is not "
            "yet supported.\n", originalString, protocolName);

        ERROR_ReportError(errString);
    }

    ip->rtRedistributeIsEnabled = TRUE;
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeReadIgrpMetric
// PURPOSE: Reads and assigns the IGRP Metric values
// ARGUMENTS:
//          node:: The current node
//          token4:: bandWidth
//          token5:: delay
//          token6:: reliability
//          token7:: load
//          token8:: MTU
//          originalString:: Entire Command string for error display
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributeReadIgrpMetric(
    Node* node,
    char* token4,
    char* token5,
    char* token6,
    char* token7,
    char* token8,
    const char* originalString)
{

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    RouteRedistribute* info = (RouteRedistribute*) ip->rtRedistributeInfo;
    IgrpRoutingInfo* metricPtr = NULL;

    info->genericMetricPtr = (IgrpRoutingInfo*)
        RtParseMallocAndSet(sizeof(IgrpRoutingInfo));

    metricPtr = (IgrpRoutingInfo*) info->genericMetricPtr;

    if (IO_IsStringNonNegativeInteger(token4))
    {
        metricPtr->bandwidth[2] = (unsigned char)
            (0xFF & (atoi(token4) >> 16));
        metricPtr->bandwidth[1] = (unsigned char)
            (0xFF & (atoi(token4) >> 8));
        metricPtr->bandwidth[0] = (unsigned char)
            (0xFF & (atoi(token4)));
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nBandwith should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }

    if (IO_IsStringNonNegativeInteger(token5))
    {
        metricPtr->delay[2] = (unsigned char)
            (0xFF & (atoi(token5) >> 16));
        metricPtr->delay[1] = (unsigned char)
            (0xFF & (atoi(token5) >> 8));
        metricPtr->delay[0] = (unsigned char)
            (0xFF & (atoi(token5)));
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nDelay should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }

    if (IO_IsStringNonNegativeInteger(token6))
    {
        metricPtr->reliability = (unsigned char) atoi(token6);
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nReliability should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }

    if (IO_IsStringNonNegativeInteger(token7))
    {
        metricPtr->load = (unsigned char) atoi(token7);
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nLoad should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }

    if (IO_IsStringNonNegativeInteger(token8))
    {
        metricPtr->mtu = (short) atoi(token8);
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nMTU should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }
}

// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeReadEigrpMetric
// PURPOSE: Reads and assigns the EIGRP Metric values
// ARGUMENTS:
//          node:: The current node
//          token4:: bandWidth
//          token5:: delay
//          token6:: reliability
//          token7:: load
//          token8:: MTU
//          originalString:: Entire Command string for error display
// RETURN: None
// ------------------------------------------------------------------------

static
void RouteRedistributeReadEigrpMetric(
    Node* node,
    char* token4,
    char* token5,
    char* token6,
    char* token7,
    char* token8,
    const char* originalString)
{

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    RouteRedistribute* info = (RouteRedistribute*) ip->rtRedistributeInfo;
    EigrpMetric* metricPtr = NULL;

    if (strlen(token8) == 0)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nEIGRP takes 5 parameters as METRIC\n", originalString);
        ERROR_ReportError(errString);
    }

    info->genericMetricPtr = (EigrpMetric*)
        RtParseMallocAndSet(sizeof(EigrpMetric));

    metricPtr = (EigrpMetric*) info->genericMetricPtr;

    if (IO_IsStringNonNegativeInteger(token4))
    {
        metricPtr->bandwidth = (unsigned int)atoi(token4);
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nBandwith should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }

    if (IO_IsStringNonNegativeInteger(token5))
    {
        metricPtr->delay = (unsigned int)atoi(token5);
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nDelay should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }

    if (IO_IsStringNonNegativeInteger(token6))
    {
        metricPtr->reliability = (unsigned short) atoi(token6);
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nReliability should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }

    if (IO_IsStringNonNegativeInteger(token7))
    {
        metricPtr->load = (unsigned char) atoi(token7);
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nLoad should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }

    if (IO_IsStringNonNegativeInteger(token8))
    {
        metricPtr->mtu[2] = (unsigned char)
            (0xFF & (atoi(token8) >> 16));
        metricPtr->mtu[1] = (unsigned char)
            (0xFF & (atoi(token8) >> 8));
        metricPtr->mtu[0] = (unsigned char)
            (0xFF & (atoi(token8)));
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
        "\n\"%s\"\nMTU should have a nemerical "
        "value.\n", originalString);
        ERROR_ReportError(errString);
    }
    if (DEBUG_EIGRP)
    {
        printf("The metric of EIGRP \n");
        printf("The bandwidth:%d \n", atoi(token4));
        printf("The delay:%d \n", atoi(token5));
        printf("The reliability:%d\n", atoi(token6));
        printf("The load: %d\n",  atoi(token7));
        printf("The MTU:%d\n", atoi(token8));
    }
}




// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeParseRouteMap
// PURPOSE: Parse the metric command
// ARGUMENTS:
//          node:: The current node
//          nodeInput:: Main configuration file
// RETURN: None
// ------------------------------------------------------------------------
static
void RouteRedistributeParseRouteMap(Node* node, char** criteriaString,
                                 const char* originalString,
                                 RouteRedistribute* info)
{
    char token1[MAX_STRING_LENGTH];
    int position = 0;
    sscanf(*criteriaString, "%s", token1);
    //route-map map-tag: If the route-map keyword is
    //not entered, all routes are redistributed; if
    //no map-tag value is entered, no routes are
    //imported.
    if (strlen(token1) == 0)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,"\n\"Node: %u\t%s\"\nRoute map tag is missing in "
        "router-config file.\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // Hook to Route Map
    info->rMap = RouteMapAddHook(node, token1);

    position = RtParseGetPositionInString(
        *criteriaString, token1);
    *criteriaString = *criteriaString + position;
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeParseMetric
// PURPOSE: Parse the metric command
// ARGUMENTS:
//          node:: The current node
//          nodeInput:: Main configuration file
// RETURN: None
// ------------------------------------------------------------------------
static
void RouteRedistributeParseMetric(Node* node, char** criteriaString,
                                 const char* originalString)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    char token1[MAX_STRING_LENGTH];
    char token2[MAX_STRING_LENGTH];
    char token3[MAX_STRING_LENGTH];
    char token4[MAX_STRING_LENGTH];
    char token5[MAX_STRING_LENGTH];
    int position = 0;

    sscanf(*criteriaString, "%s %s %s %s %s", token1, token2, token3,
        token4, token5);

    if (ip->rtRedistributeInfo->receiverProtocolType ==
        ROUTING_PROTOCOL_IGRP)
    {
        RouteRedistributeReadIgrpMetric(
            node,
            token1,
            token2,
            token3,
            token4,
            token5,
            originalString);

        position = RtParseGetPositionInString(
            *criteriaString, token5);
        *criteriaString = *criteriaString + position;
    }
    else
    if (ip->rtRedistributeInfo->receiverProtocolType ==
        ROUTING_PROTOCOL_EIGRP)
    {
        //
        //EIGRP metric has fetched when the receiver protocol
        //is EIGRP.
        //
        RouteRedistributeReadEigrpMetric(
            node,
            token1,
            token2,
            token3,
            token4,
            token5,
            originalString);

        position = RtParseGetPositionInString(
            *criteriaString, token5);
        *criteriaString = *criteriaString + position;
    }
    else
    {
        // Check whether a numerical value is
        // missing after the 'METRIC' keyword
        if (strlen(token1) == 0)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
            "\n\"%s\"\nMetric value is missing in "
            "router-config file.\n", originalString);
            ERROR_ReportError(errString);
        }
        // Check whether this is a number or
        // character string. If this is a number then
        // assign the value to the structure else
        // display an ERROR Report.
        if (IO_IsStringNonNegativeInteger(token1))
        {
            ip->rtRedistributeInfo->metric =
                atoi(token1);

            position = RtParseGetPositionInString(
                *criteriaString, token1);
            *criteriaString = *criteriaString + position;
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
            "\n\"%s\"\nMetric should have a numerical "
            "value.\n", originalString);
            ERROR_ReportError(errString);
        }
    }
}

// ------------------------------------------------------------------------
// FUNCTION: HandleRouteTableUpdate
// PURPOSE: Handler function for route table update
// ARGUMENTS:
//          node:: The current node
//          msg:: message structure
// RETURN: None
// ------------------------------------------------------------------------
static void HandleRouteTableUpdate(Node* node, Message* msg)
{
    int interfaceIndex = 0;
    NodeAddress destAddress;
    NodeAddress destAddressMask;
    NetworkRoutingProtocolType routingProtocolType,
    routingProtocolType2, distributorProtocolType, receiverProtocolType;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    distributorProtocolType =
        ip->rtRedistributeInfo->distributorProtocolType;
    receiverProtocolType =  ip->rtRedistributeInfo->receiverProtocolType;

    if ((distributorProtocolType == ROUTING_PROTOCOL_STATIC)
        || (distributorProtocolType == ROUTING_PROTOCOL_DEFAULT))
    {
        NetworkForwardingTable* forwardTable = &(ip->forwardTable);
        for (int j = 0; j < forwardTable->size; j++)
        {
           if (forwardTable->row[j].protocolType == distributorProtocolType)
           {
               for (int i = 0; i < node->numberInterfaces; i++)
                {
                    if ((NetworkIpGetInterfaceType(node, i) != NETWORK_IPV4)
                        && (NetworkIpGetInterfaceType(node, i) !=
                            NETWORK_DUAL))
                    {
                        continue;
                    }

                    routingProtocolType =
                        NetworkIpGetUnicastRoutingProtocolType(node,
                                                               i,
                                                               NETWORK_IPV4);

                    if (routingProtocolType == receiverProtocolType)
                    {
                        RouteRedistributeAddHook(
                            node,
                            forwardTable->row[j].destAddress,
                            forwardTable->row[j].destAddressMask,
                            forwardTable->row[j].nextHopAddress,
                            i,
                            0,
                            distributorProtocolType);
                    }
                }
           }
        }
        return;
    }

    for (interfaceIndex = 0; interfaceIndex < node->numberInterfaces;
         interfaceIndex++)
    {
        if (NetworkIpGetInterfaceType(node, interfaceIndex) != NETWORK_IPV4
            && NetworkIpGetInterfaceType(node, interfaceIndex) !=
               NETWORK_DUAL)
        {
            continue;
        }
        routingProtocolType = NetworkIpGetUnicastRoutingProtocolType(node,
                                  interfaceIndex, NETWORK_IPV4);
        if (routingProtocolType == distributorProtocolType)
        {
            destAddress = NetworkIpGetInterfaceNetworkAddress(node,
                              interfaceIndex);
            destAddressMask = NetworkIpGetInterfaceSubnetMask(node,
                                  interfaceIndex);
            int i = 0;
            for (i = 0; i < node->numberInterfaces; i++)
            {
                if (i != interfaceIndex
                  && NetworkIpGetInterfaceType(node, i) != NETWORK_IPV4
                  && NetworkIpGetInterfaceType(node, i) != NETWORK_DUAL)
                {
                    continue;
                }
                routingProtocolType2 =
                    NetworkIpGetUnicastRoutingProtocolType(node,
                                                           i,
                                                           NETWORK_IPV4);
                if (routingProtocolType2 == receiverProtocolType)
                {
                    RouteRedistributeAddHook(
                        node,
                        destAddress,
                        destAddressMask,
                        0,
                        interfaceIndex,
                        0,
                        routingProtocolType);
                }
            }
        }
    }
}
// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeInit
// PURPOSE: Initialize the Route redistribution
// ARGUMENTS:
//          node:: The current node
//          nodeInput:: Main configuration file
// RETURN: None
// ------------------------------------------------------------------------

void RouteRedistributeInit(
    Node* node,
    const NodeInput* nodeInput)
{
    NodeInput rtConfig;
    BOOL isFound = FALSE;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    ip->rtRedistributeIsEnabled = FALSE;
    ip->rtRedistributeInfo = NULL;

    // Read configuration for router configuration file

    IO_ReadCachedFile(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTER-CONFIG-FILE",
        &isFound,
        &rtConfig);

    // file found

    if (isFound == TRUE)
    {
        int count = 0; // variables defined for counter.
        BOOL foundNodeIdentifier = TRUE;
        BOOL foundRouter = FALSE;
        BOOL routerInstance = FALSE;
        BOOL gotForThisNode = FALSE;

        for (count = 0; count < rtConfig.numLines; count++)
        {

            char* originalString = NULL;
            char token1[MAX_STRING_LENGTH] = {0}; // for command
            char token2[MAX_STRING_LENGTH] = {0}; // for protocol
            char token3[MAX_STRING_LENGTH] = {0}; // Metric/Route-Map
            char token4[MAX_STRING_LENGTH] = {0}; // Metric/Route-Map value
            char token5[MAX_STRING_LENGTH] = {0};
            char token6[MAX_STRING_LENGTH] = {0};
            char token7[MAX_STRING_LENGTH] = {0};
            char token8[MAX_STRING_LENGTH] = {0};
            char token9[MAX_STRING_LENGTH] = {0};
            char token10[MAX_STRING_LENGTH] = {0};
            char token11[MAX_STRING_LENGTH] = {0};
            char tokenDefaultMetric4[MAX_STRING_LENGTH] = {0}; //Default Metric
            char tokenDefaultMetric5[MAX_STRING_LENGTH] = {0};
            char tokenDefaultMetric6[MAX_STRING_LENGTH] = {0};
            char tokenDefaultMetric7[MAX_STRING_LENGTH] = {0};
            char tokenDefaultMetric8[MAX_STRING_LENGTH] = {0};
            RouteRedistribute* info = NULL;

            int  numOfTokens = 0;
            char* criteriaString = NULL;

            numOfTokens = sscanf(rtConfig.inputStrings[count],
                "%s %s %s %s %s %s %s %s %s %s %s", token1, token2, token3,
                token4, token5, token6, token7, token8, token9, token10,
                token11);

            // we make a copy of the original string for error reporting
            originalString = RtParseStringAllocAndCopy(
                rtConfig.inputStrings[count]);

            criteriaString = originalString;

            if (!RtParseStringNoCaseCompare("NODE-IDENTIFIER", token1))
            {
                BOOL isAnother = FALSE;
                foundNodeIdentifier = FALSE;
                routerInstance = FALSE;
                foundRouter = FALSE;

                RtParseIdStmt(
                    node,
                    rtConfig,
                    count,
                    &foundNodeIdentifier,
                    &gotForThisNode,
                    &isAnother);

                MEM_free(originalString);
                if (isAnother == TRUE)
                {
                    // Got another NODE-IDENTIFIER statement so this
                    // configuration is complete
                    break;
                }
                else
                {
                    continue;
                }
            }
            // ROUTER command would be as below.
            // ROUTER <Protocol Name>
            else
            if ((foundNodeIdentifier == TRUE) &&
                 RtParseStringNoCaseCompare("ROUTER", token1) == 0)
            {
                foundRouter = TRUE;
                routerInstance = TRUE;

                info = (RouteRedistribute*)
                    RtParseMallocAndSet(sizeof(RouteRedistribute));

                RouteRedistributeInitializeVariables(info);
                // Initializing the stat variables
                RouteRedistributeInitializeStatVariables(info);

                RouteRedistributeValidateReceiverProtocolType(
                    node,
                    token2,
                    info);

            }
            // REDISTRIBUTION command would be as below.
            // REDISTRIBUTE <Protocol> {METRIC|ROUTE-MAP} {VALUE}
            // {<Start Time> <End Time>}
            else
            if ((foundNodeIdentifier == TRUE) && (foundRouter == TRUE) &&
                (RtParseStringNoCaseCompare("REDISTRIBUTE", token1) == 0))
            {
                // Check whether the protocol name is missing after
                // the 'REDISTRIBUTE' command placed at router-config
                // file.
                // redistribute <protocol> <metric metric-values>
                //  <route-map map-name> <starttime endtime>

                if (routerInstance == FALSE)
                {
                    NetworkDataIp* ip =
                        (NetworkDataIp*) node->networkData.networkVar;
                    RouteRedistribute* lastEntry = (RouteRedistribute*)
                        ip->rtRedistributeInfo;

                    info = (RouteRedistribute*) RtParseMallocAndSet(
                        sizeof(RouteRedistribute));

                    RouteRedistributeInitializeVariables(info);
                    // Initializing the stat variables
                    RouteRedistributeInitializeStatVariables(info);

                    ERROR_Assert(lastEntry != NULL, "Last Entry is NULL.\n"
                        "It should not occur!!!\n");

                    info->receiverProtocolType =
                        lastEntry->receiverProtocolType;

                    info->next = ip->rtRedistributeInfo;
                    ip->rtRedistributeInfo = info;
                }
                else
                {
                    info = ip->rtRedistributeInfo;
                }

                // <protocol> <metric metric-values> <route-map map-name>
                //  <starttime endtime>
                RouteRedistributeValidateDistributorProtocolType(
                    node,
                    token2,
                    originalString);

                routerInstance = FALSE;

                // Check any other distribution parameters
                // <metric metric-values> <route-map map-name>
                //  <starttime endtime>
                if (strlen(token3) != 0)
                {
                    // <metric metric-values> <route-map map-name>
                    //  <starttime endtime>
                    char token1[MAX_STRING_LENGTH] = {0};
                    int position = RtParseGetPositionInString(
                        criteriaString, token2);
                    criteriaString = criteriaString + position;

                    position = 0;

                    if (*criteriaString != '\0')
                    {
                        if ((RtParseStringNoCaseCompare(
                            token3, "METRIC") == 0))
                        {
                            position = RtParseGetPositionInString(
                                criteriaString, token3);
                            criteriaString = criteriaString + position;
                            // only the metric value will be parsed
                            RouteRedistributeParseMetric(node,
                                    &criteriaString, originalString);
                        }

                        if (*criteriaString != '\0')
                        {
                            sscanf(criteriaString, "%s", token1);
                            // <route-map map-name> <starttime endtime>
                            // Found Route Map for this node and this is tagged
                            // with redistribution.
                            if ((RtParseStringNoCaseCompare(token1,
                                "ROUTE-MAP") == 0))
                            {
                                position = RtParseGetPositionInString(
                                    criteriaString, token1);
                                criteriaString = criteriaString + position;
                                RouteRedistributeParseRouteMap(node,
                                        &criteriaString, originalString,
                                        info);
                            }
                        }

                        memset(token1, 0, MAX_STRING_LENGTH);
                        memset(token2, 0, MAX_STRING_LENGTH);
                        // token1 and token2 if defined should be the start
                        //  time and the end time
                        if (*criteriaString != '\0')
                        {
                            int tempCount = sscanf(criteriaString,
                                    "%s %s %s", token1, token2, token3);
                            if (tempCount > 2)
                            {
                                char errString[MAX_STRING_LENGTH];
                                sprintf(errString, "Node: %u, Wrong syntax"
                                     " of route redistribution in"
                                    " router-config file.\n",node->nodeId);
                                ERROR_ReportError(errString);
                            }
                        }

                        RouteRedistributeParseSimTime(
                            node,
                            ROUTE_REDISTRIBUTE,
                            token1,
                            token2);
                    }
                    // If metric or route map is not defined
                    else
                    {
                        if ((ip->rtRedistributeInfo->receiverProtocolType ==
                                ROUTING_PROTOCOL_IGRP) ||
                            (ip->rtRedistributeInfo->receiverProtocolType ==
                                ROUTING_PROTOCOL_EIGRP))
                        {
                            //Set Default Metric
                            sscanf("10000 100 255 1 1500","%s %s %s %s %s",
                                tokenDefaultMetric4,tokenDefaultMetric5,
                                tokenDefaultMetric6,tokenDefaultMetric7,
                                tokenDefaultMetric8);

                            if (ip->rtRedistributeInfo->receiverProtocolType ==
                                ROUTING_PROTOCOL_IGRP)
                            {
                                RouteRedistributeReadIgrpMetric(
                                    node,
                                    tokenDefaultMetric4,
                                    tokenDefaultMetric5,
                                    tokenDefaultMetric6,
                                    tokenDefaultMetric7,
                                    tokenDefaultMetric8,
                                    originalString);
                            }

                            if (ip->rtRedistributeInfo->receiverProtocolType
                                == ROUTING_PROTOCOL_EIGRP)
                            {
                                //
                                //EIGRP metric has fetched when the receiver protocol
                                //is EIGRP.
                                //
                                RouteRedistributeReadEigrpMetric(
                                    node,
                                    tokenDefaultMetric4,
                                    tokenDefaultMetric5,
                                    tokenDefaultMetric6,
                                    tokenDefaultMetric7,
                                    tokenDefaultMetric8,
                                    originalString);
                            }
                        }

                        RouteRedistributeParseSimTime(
                            node,
                            ROUTE_REDISTRIBUTE,
                            token3,
                            token4);
                        }
                    }
                // <starttime endtime>
                // e.g. REDISTRIBUTE RIPv2
                // No parameters for REDISTRIBUTE is found.
                    else
                    {
                        if ((ip->rtRedistributeInfo->receiverProtocolType ==
                                ROUTING_PROTOCOL_IGRP) ||
                            (ip->rtRedistributeInfo->receiverProtocolType ==
                                ROUTING_PROTOCOL_EIGRP))
                        {
                        //Set Default Metric for EIGRP and IGRP
                        sscanf("10000 100 255 1 1500","%s %s %s %s %s",
                            tokenDefaultMetric4,tokenDefaultMetric5,
                            tokenDefaultMetric6,tokenDefaultMetric7,
                            tokenDefaultMetric8);

                        if (ip->rtRedistributeInfo->receiverProtocolType ==
                            ROUTING_PROTOCOL_IGRP)
                        {
                            RouteRedistributeReadIgrpMetric(
                            node,
                                tokenDefaultMetric4,
                                tokenDefaultMetric5,
                                tokenDefaultMetric6,
                                tokenDefaultMetric7,
                                tokenDefaultMetric8,
                                originalString);
                        }
                        else
                        if (ip->rtRedistributeInfo->receiverProtocolType ==
                            ROUTING_PROTOCOL_EIGRP)
                        {
                            //
                            //EIGRP metric has fetched when the receiver protocol
                            //is EIGRP.
                            //
                            RouteRedistributeReadEigrpMetric(
                                node,
                                tokenDefaultMetric4,
                                tokenDefaultMetric5,
                                tokenDefaultMetric6,
                                tokenDefaultMetric7,
                                tokenDefaultMetric8,
                                originalString);
                        }
                    }

                    RouteRedistributeParseSimTime(
                        node,
                        ROUTE_REDISTRIBUTE,
                        token3,
                        token4);
                }
            }
            // NO REDISTRIBUTE command would be as below.
            // NO REDISTRIBUTE <Protocol> {<Start Time> <End Time>}
            else
            if ((foundNodeIdentifier == TRUE) &&
                (RtParseStringNoCaseCompare("NO", token1) == 0))
            {
                if (strlen(token2) == 0)
                {
                    ERROR_ReportError("Syntax is NO REDISTRIBUTE "
                        "<Protocol> {<Start Time> <End Time>}\n");
                }

                if (RtParseStringNoCaseCompare(
                        "REDISTRIBUTE",
                        token2) == 0)
                {
                    if ((ip->rtRedistributeInfo->receiverProtocolType ==
                            ROUTING_PROTOCOL_IGRP) ||
                        (ip->rtRedistributeInfo->receiverProtocolType ==
                            ROUTING_PROTOCOL_EIGRP))
                    {
                        if (numOfTokens == 9 || numOfTokens == 11)
                        {
                            RouteRedistributeParseSimTime(
                                node,
                                ROUTE_NO_REDISTRIBUTE,
                                token10,
                                token11);
                        }
                        else
                        {
                            ERROR_ReportError("Check the number of "
                                "parameters in NO REDISTRIBUTE command\n");
                        }
                    }
                    else
                    {
                        if (strlen(token3) == 0)
                        {
                            char errString[MAX_STRING_LENGTH];
                            sprintf(errString, "Protocol name is missing in "
                                "router-config file.\n");

                            ERROR_ReportError(errString);
                        }

                        RouteRedistributeParseSimTime(
                            node,
                            ROUTE_NO_REDISTRIBUTE,
                            token4,
                            token5);
                    }
                }
                else
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "Unknown command !!!\n"
                        "NO REDISTRIBUTE <Protocol> {<Start Time> "
                        "<End Time>}\n");
                    ERROR_ReportError(errString);
                }
            }

            MEM_free(originalString);
        }

        gotForThisNode = FALSE;
        RouteRedistributeStatInit(node, nodeInput);
    }
}


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeFinalize
// PURPOSE: Prints out the statistics at the end of the simulation
// ARGUMENTS:
//          node:: The current node
// RETURN: None
// ------------------------------------------------------------------------

void RouteRedistributeFinalize(Node* node)
{

    char receiverProtocol[MAX_STRING_LENGTH];
    char distributorProtocol[MAX_STRING_LENGTH];

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    if (ip->rtRedistributeIsEnabled == TRUE)
    {
        RouteRedistribute* info = (RouteRedistribute*)
            ip->rtRedistributeInfo;

        while (info != NULL)
        {
            if (info->receiverProtocolType == ROUTING_PROTOCOL_OSPFv2)
            {
                sprintf(receiverProtocol, "OSPFv2");
            }
            else
            if (info->receiverProtocolType == ROUTING_PROTOCOL_RIP)
            {
                if (info->versionReceiver == RIP_VERSION_1)
                {
                sprintf(receiverProtocol, "RIPv1");
                }
                else
                {
                sprintf(receiverProtocol, "RIPv2");
            }
            }
            else
            if (info->receiverProtocolType == ROUTING_PROTOCOL_IGRP)
            {
                sprintf(receiverProtocol, "IGRP");
            }
            else
            if (info->receiverProtocolType == ROUTING_PROTOCOL_EIGRP)
            {
                sprintf(receiverProtocol, "EIGRP");
            }
            else
            if (info->receiverProtocolType == ROUTING_PROTOCOL_BELLMANFORD)
            {
                sprintf(receiverProtocol, "BELLMANFORD");
            }
            else
            if (info->receiverProtocolType == EXTERIOR_GATEWAY_PROTOCOL_EBGPv4)
            {
                sprintf(receiverProtocol, "BGPv4");
            }            
            
            else
            {
                ERROR_Assert(FALSE, "This protocol is not yet supported\n");
            }

            if (info->distributorProtocolType == ROUTING_PROTOCOL_RIP)
            {
                if (info->versionDistributor == RIP_VERSION_1)
                {
                    sprintf(distributorProtocol, "RIPv1");
                }
                else
                {
                sprintf(distributorProtocol, "RIPv2");
                }

            }
            else
            if (info->distributorProtocolType == ROUTING_PROTOCOL_OSPFv2)
            {
                sprintf(distributorProtocol, "OSPFv2");

            }
            else
            if (info->distributorProtocolType == ROUTING_PROTOCOL_IGRP)
            {
                sprintf(distributorProtocol, "IGRP");

            }
            else
            if (info->distributorProtocolType == ROUTING_PROTOCOL_EIGRP)
            {
                sprintf(distributorProtocol, "EIGRP");

            }
            else
            if (info->distributorProtocolType ==
                EXTERIOR_GATEWAY_PROTOCOL_EBGPv4)
            {
                sprintf(distributorProtocol, "BGPv4");

            }
            else
            if (info->distributorProtocolType == ROUTING_PROTOCOL_BELLMANFORD)
            {
                sprintf(distributorProtocol, "BELLMANFORD");

            }
            else
            if (info->distributorProtocolType == ROUTING_PROTOCOL_STATIC)
            {
                sprintf(distributorProtocol, "STATIC");

            }
            else
            if (info->distributorProtocolType == ROUTING_PROTOCOL_DEFAULT)
            {
                sprintf(distributorProtocol, "DEFAULT");

            }

            RouteRedistributePrintCommonStat(
                node,
                info);

            info = info->next;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        RouteRedistributionLayer.
//  PURPOSE:     Handle timers and layer messages.
//  PARAMETERS: 
//               Node* node:     Node handling the incoming messages
//               Message* msg: Message for node to interpret.
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void RouteRedistributionLayer(Node* node, Message* msg)
{
    switch (msg->eventType)
    {
        case MSG_NETWORK_RedistributeData:
        {
            HandleRouteTableUpdate(node, msg);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];

            sprintf(errStr, "Unknown Event-Type for node %u", node->nodeId);
            ERROR_Assert(FALSE, errStr);

            break;
        }
    }

    MESSAGE_Free(node, msg);
}
