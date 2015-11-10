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
 * File: dvmrp.pc
 * PUPPOSE: To simulate Distance Vector Multicast Routing Protocol (DVMRP)
 *          as described in draft-ietf-idmr-dvmrp-v3-10. It resides at the
 *          network layer just above IP.
 *
 * COMMENTS: DVMRP supports source networks aggregation to reduce the size of
 *           routing table. This implementation doesn't support this feature.
 *
 * FEATURES LEFT TO BE DONE: Tunneling.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "network_dualip.h"

#include "api.h"
#include "network_ip.h"
#include "multicast_igmp.h"

#include "multicast_dvmrp.h"

#ifdef CYBER_CORE
#include "network_iahep.h"
#endif // CYBER_CORE

#define noDEBUG
#define noDEBUG_TABLE
#define noDEBUG_ERROR
#define noDEBUG_Route3
#define noDEBUG_MTF7


/* For binary exponential back-off */
#define DVMRP_MAX_GRAFT    16

#ifdef DEBUG_Route3

    #define DOWN_TIME    (15 * MINUTE)

#endif

#ifdef DEBUG_MTF7

    #define GROUP1    0xE1000000

#endif


/* Internal function declaration */

/*----------------------------------------------------------------------------*/
/*         PROTOTYPE FOR CREATING AND SENDING DIFFERENT PACKET TO IP          */
/*----------------------------------------------------------------------------*/
static void RoutingDvmrpSendProbePacket(Node *node);

static void RoutingDvmrpSendRouteReport(Node *node, NodeAddress route);

static BOOL RoutingDvmrpCreateRouteReportPacket(Node *node, int *rowIndex,
        Message *routeMsg, NodeAddress route, int interfaceIndex);

static void RoutingDvmrpSendPrune(Node *node, NodeAddress srcAddr,
        NodeAddress grpAddr, clocktype pruneLifetime);

static void RoutingDvmrpSendGraft(Node *node, NodeAddress srcAddr,
        NodeAddress grpAddr);

static void RoutingDvmrpSendGraftAck(Node *node, NodeAddress destAddr,
        NodeAddress srcAddr, NodeAddress grpAddr);

/*----------------------------------------------------------------------------*/
/*              PROTOTYPE FOR HANDLING DIFFERENT PROTOCOL PACKET              */
/*----------------------------------------------------------------------------*/
static void RoutingDvmrpHandleProbePacket(Node *node,
        RoutingDvmrpProbePacketType *probePacket, NodeAddress sourceAddr,
        int size, int interfaceIndex);

static void RoutingDvmrpHandleRouteReportPacket(Node *node,
        RoutingDvmrpRouteReportPacketType *routePacket, NodeAddress srcAddr,
        int size, int interfaceIndex);

static void RoutingDvmrpHandlePrunePacket(Node *node,
        RoutingDvmrpPrunePacketType *prunePacket, NodeAddress srcAddr, int intrefcaeIndex);

static void RoutingDvmrpHandleGraftPacket(Node *node,
        RoutingDvmrpGraftPacketType *graftPacket, NodeAddress srcAddr, int interfaceIndex);

static void RoutingDvmrpHandleGraftAckPacket(Node *node,
        RoutingDvmrpGraftAckPacketType *graftAckPacket, NodeAddress srcAddr);

/*----------------------------------------------------------------------------*/
/*                 PROTOTYPE FOR HANDLING DIFFERENT TIMEOUT                   */
/*----------------------------------------------------------------------------*/
static void RoutingDvmrpHandlePruneTimeoutAlarm(Node *node,
        RoutingDvmrpPruneTimerInfo timerInfo);

static void RoutingDvmrpHandleGraftRtmxtTimeOut(Node *node,
        RoutingDvmrpGraftTimerInfo timerInfo);

static void RoutingDvmrpHandleNeighborTimeOut(Node *node,
        RoutingDvmrpInterface *interface, NodeAddress nbrAddr);

static void RoutingDvmrpHandleRouteTimeOut(Node *node,
        RoutingDvmrpRoutingTableRow *rowPtr);


/*----------------------------------------------------------------------------*/
/*              PROTOTYPE FOR INITIALIZING DIFFERENT STRUCTURE                */
/*----------------------------------------------------------------------------*/
static void RoutingDvmrpInitForwardingTable(Node *node);

static void RoutingDvmrpInitRoutingTable(Node *node);

static void RoutingDvmrpInitStat(Node *node);

static void RoutingDvmrpInitChildList(Node *node, LinkedList **childList,
        unsigned char metric, int upstream);

static void RoutingDvmrpInitDependentNeighborList(Node *node, LinkedList **list,
        RoutingDvmrpChildListItem *child);

/*----------------------------------------------------------------------------*/
/*                PROTOTYPE FOR UPDATING DIFFERENT STRUCTURE                  */
/*----------------------------------------------------------------------------*/
static void RoutingDvmrpAddNewRoute(Node *node, NodeAddress destAddr,
        NodeAddress netmask, NodeAddress nextHop, unsigned char metric,
        int interfaceId);

static void RoutingDvmrpUpdateRoute(Node *node, NodeAddress destAddr,
        NodeAddress netmask, NodeAddress nextHop, unsigned char metric,
        int interfaceId);

static NodeAddress *RoutingDvmrpFillNeighborList(Node *node,
        int interfaceIndex);

static void RoutingDvmrpRemoveDependency(Node *node,
        RoutingDvmrpRoutingTableRow *rowPtr, NodeAddress neighborAddr,
        int interfaceId);

static void RoutingDvmrpAddDependentNeighbor(Node *node,
        RoutingDvmrpRoutingTableRow *rowPtr, NodeAddress neighborIp,
        int interfaceId);

static void RoutingDvmrpUpdateDsgFwdIfRequired(LinkedList* childList,
        unsigned char metric);

static void RoutingDvmrpSetDesignatedForwarder(
        RoutingDvmrpRoutingTableRow *rowPtr, int interfaceId);

static void RoutingDvmrpUpdateChildList(Node *node, LinkedList **childList,
        unsigned char metric, int upstream);

static void RoutingDvmrpUpdateDownstreamList(Node *node, LinkedList *list,
        NodeAddress srcAddr, int interface, RoutingDvmrpListActionType type);

static void RoutingDvmrpUpdateDependentNeighborList(Node *node, LinkedList *list,
        NodeAddress addr, RoutingDvmrpListActionType type);

static void RoutingDvmrpAddToSourceList(Node *node, LinkedList *srcList,
        NodeAddress srcAddr);

static void RoutingDvmrpRemoveFromSourceList(Node *node, LinkedList *srcList,
        ListItem *srcItem);

static void RoutingDvmrpAddEntryInForwardingCache(Node *node,
        NodeAddress srcAddr, NodeAddress grpAddr, int upstream);

/*----------------------------------------------------------------------------*/
/*                 PROTOTYPE FOR DIFFERENT SEARCH FUNCTIONS                   */
/*----------------------------------------------------------------------------*/
static void RoutingDvmrpFindRoute(Node *node, NodeAddress destAddr,
        RoutingDvmrpRoutingTableRow **rowPtr);

static BOOL RoutingDvmrpLookUpForwardingCache(Node *node, Message *msg,
        NodeAddress groupAddr, BOOL *packetWasRouted);

static void RoutingDvmrpSearchNeighborList(LinkedList *list, NodeAddress addr,
        RoutingDvmrpNeighborListItem **item);

static BOOL RoutingDvmrpIsDependentNeighbor(RoutingDvmrpRoutingTableRow *rowPtr,
        NodeAddress neighborAddr, int interfaceId);

static BOOL RoutingDvmrpIsChildInterface(RoutingDvmrpRoutingTableRow *rowPtr,
        int interfaceId);

static RoutingDvmrpChildListItem* RoutingDvmrpGetChild(LinkedList* childList,
        int InterfaceId);

static NodeAddress RoutingDvmrpGetDesignatedForwarder(
        RoutingDvmrpRoutingTableRow *rowPtr, int interfaceId);

static void RoutingDvmrpGetDownstreamListForThisSourceGroup(Node *node,
        NodeAddress srcAddr, NodeAddress grpAdrr, LinkedList** downstreamList);

static RoutingDvmrpSourceList* RoutingDvmrpGetSourceForThisGroup(
        Node *node, NodeAddress srcAddr, NodeAddress grpAddr);

/*----------------------------------------------------------------------------*/
/*                              OTHER FUNCTIONS                               */
/*----------------------------------------------------------------------------*/
static void RoutingDvmrpSetTimer(Node *node, int eventType, void *data,
        clocktype delay);

static void RoutingDvmrpSetPruneLifetime(RoutingDvmrpSourceList *srcPtr,
                                         clocktype theTime);

static void RoutingDvmrpCreateCommonHeader(
        RoutingDvmrpCommonHeaderType *commonHeader,
        RoutingDvmrpPacketCode pktCode);

static void RoutingDvmrpPrintRoutingTable(Node *node);

static void RoutingDvmrpPrintForwardingTable(Node *node);

static BOOL RoutingDvmrpIsDvmrpEnabledInterface(Node *node,
        int interfaceId);

/*----------------------------End of function declaration---------------------*/


/*
 * FUNCTION     RoutingDvmrpUpdateFwdTableForChangedDependency
 * PURPOSE      Update forwarding table as neighbor dependency were changed
 *
 * Parameters:
 *     node:        Node in which the operation done.
 *     nbrAddr:     Neighbor address whose dependency has been changed.
 *     interfaceId: Interface over which dependency were changed.
 *     flag:         1 stands for new dependency,
 *                  -1 stands for removed dependency.
 */
static
void RoutingDvmrpUpdateFwdTableForChangedDependency(
    Node *node,
    NodeAddress nbrAddr,
    int interfaceId,
    int flag)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpForwardingTable *fwdTable = &dvmrp->forwardingTable;
    ListItem *srcListItem = NULL;
    RoutingDvmrpSourceList *srcPtr;
    RoutingDvmrpRoutingTableRow *rowPtr;
    int i;

    for (i = 0; i < (signed)fwdTable->numEntries; i++)
    {
        srcListItem = fwdTable->rowPtr[i].srcList->first;

        while (srcListItem)
        {
            ListItem *dListItem;
            RoutingDvmrpDownstreamListItem *dInfo = NULL;

            srcPtr = (RoutingDvmrpSourceList *) srcListItem->data;

            RoutingDvmrpFindRoute(node, srcPtr->srcAddr, &rowPtr);
            ERROR_Assert(rowPtr != NULL, "Unknown source have forwarding "
                                         "table entry\n");

            /*
             * If the interface, over which the dependency changed
             * is not a child interface for this source, or if we are not
             * the designated forwarder for this source on this network;
             * then skip this source
             */
            if ((!RoutingDvmrpIsChildInterface(rowPtr, interfaceId)) ||
                (dvmrp->interface[interfaceId].ipAddress !=
                RoutingDvmrpGetDesignatedForwarder(rowPtr, interfaceId)))
            {
                srcListItem = srcListItem->next;
                continue;
            }

            dListItem = srcPtr->downstream->first;

            while (dListItem)
            {
                dInfo = (RoutingDvmrpDownstreamListItem *) dListItem->data;

                if (dInfo->interfaceIndex == interfaceId)
                {
                    break;
                }
                dListItem = dListItem->next;
            }

            if (flag == 1)
            {
                if (!dListItem)
                {
                    RoutingDvmrpUpdateDownstreamList(
                        node, srcPtr->downstream, srcPtr->srcAddr, interfaceId,
                        ROUTING_DVMRP_INSERT_TO_LIST);
                }
                else
                {
                    /* Add to dependent neighbor list */
                    RoutingDvmrpUpdateDependentNeighborList(
                        node, dInfo->dependentNeighbor, nbrAddr,
                        ROUTING_DVMRP_INSERT_TO_LIST);

                    dInfo->isPruned = FALSE;
                }

                if (srcPtr->pruneActive)
                {
                    RoutingDvmrpGraftTimerInfo timerInfo;

                    srcPtr->pruneActive = FALSE;

                    /* Send graft to upstream */
                    RoutingDvmrpSendGraft(
                        node, srcPtr->srcAddr, fwdTable->rowPtr[i].groupAddr);

                    /*
                     * Add the graft to the retransmission timer list
                     * awaiting an acknowledgment.
                     */
                    srcPtr->graftRtmxtTimer = TRUE;

                    timerInfo.count = 0;
                    timerInfo.srcAddr = srcPtr->srcAddr;
                    timerInfo.grpAddr = fwdTable->rowPtr[i].groupAddr;

                    RoutingDvmrpSetTimer(node,
                                         MSG_ROUTING_DvmrpGraftRtmxtTimeOut,
                                         (void *) &timerInfo,
                                         DVMRP_GRAFT_RTMXT_INTERVAL);
                }
            }
            else if (flag == -1)
            {
                if (!dListItem)
                {
                    /* Do nothing */
                }
                else
                {
                    NodeAddress dsgFwd =
                        RoutingDvmrpGetDesignatedForwarder(rowPtr, interfaceId);

                    /* Remove dependent neighbor, if exist */
                    RoutingDvmrpUpdateDependentNeighborList(
                        node, dInfo->dependentNeighbor, nbrAddr,
                        ROUTING_DVMRP_REMOVE_FROM_LIST);

                    /*
                     * If there is no more dependent neighbor over this
                     * interface and we are not the designated forwarder,
                     * remove this interface
                     */
                    if ((dInfo->dependentNeighbor->size == 0) &&
                            (dsgFwd != dvmrp->interface[interfaceId].ipAddress))
                    {
                        RoutingDvmrpUpdateDownstreamList(
                            node, srcPtr->downstream, srcPtr->srcAddr,
                            interfaceId, ROUTING_DVMRP_REMOVE_FROM_LIST);
                    }
                }
            }
            else
            {
                ERROR_Assert(FALSE, "Condition shouldn't happen\n");
            }

            srcListItem = srcListItem->next;
        }
    }
}


/*
 * FUNCTION     RoutingDvmrpUpdateFwdTableForDesignatedForwarder
 * PURPOSE      Update forwarding table as designated forwarder was changed.
 *
 * Parameters:
 *     node:        Node in which the operation done.
 *     interfaceId: Interface over which forwarder status was changed.
 *     flag:         1 --> This node become new forwarder,
 *                  -1 --> Some other node become forwarder.
 */
static
void RoutingDvmrpUpdateFwdTableForDesignatedForwarder(
    Node *node,
    int interfaceId,
    int flag)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpForwardingTable *fwdTable = &dvmrp->forwardingTable;
    ListItem *srcListItem = NULL;
    RoutingDvmrpSourceList *srcPtr;
    RoutingDvmrpRoutingTableRow *rowPtr;
    int i;

    for (i = 0; i < (signed)fwdTable->numEntries; i++)
    {
        srcListItem = fwdTable->rowPtr[i].srcList->first;

        while (srcListItem)
        {
            ListItem *dListItem;
            RoutingDvmrpDownstreamListItem *dInfo = NULL;

            srcPtr = (RoutingDvmrpSourceList *) srcListItem->data;

            RoutingDvmrpFindRoute(node, srcPtr->srcAddr, &rowPtr);
            ERROR_Assert(rowPtr != NULL, "Unknown source have forwarding "
                                         "table entry\n");

            dListItem = srcPtr->downstream->first;

            while (dListItem)
            {
                dInfo = (RoutingDvmrpDownstreamListItem *) dListItem->data;

                if (dInfo->interfaceIndex == interfaceId)
                {
                    break;
                }
                dListItem = dListItem->next;
            }

            if (flag == 1)
            {
                if (!dListItem)
                {
                    RoutingDvmrpUpdateDownstreamList(
                        node, srcPtr->downstream, srcPtr->srcAddr, interfaceId,
                        ROUTING_DVMRP_INSERT_TO_LIST);
                }
                else
                {
                    dInfo->isPruned = FALSE;
                }

                if (srcPtr->pruneActive)
                {
                    RoutingDvmrpGraftTimerInfo timerInfo;

                    srcPtr->pruneActive = FALSE;

                    /* Send graft to upstream */
                    RoutingDvmrpSendGraft(
                        node, srcPtr->srcAddr, fwdTable->rowPtr[i].groupAddr);

                    /*
                     * Add the graft to the retransmission timer list
                     * awaiting an acknowledgment.
                     */
                    srcPtr->graftRtmxtTimer = TRUE;

                    timerInfo.count = 0;
                    timerInfo.srcAddr = srcPtr->srcAddr;
                    timerInfo.grpAddr = fwdTable->rowPtr[i].groupAddr;

                    RoutingDvmrpSetTimer(node,
                                         MSG_ROUTING_DvmrpGraftRtmxtTimeOut,
                                         (void *) &timerInfo,
                                         DVMRP_GRAFT_RTMXT_INTERVAL);
                }
            }
            else if (flag == -1)
            {
                if (!dListItem)
                {
                    /* Do nothing */
                }
                else
                {
                    RoutingDvmrpUpdateDownstreamList(
                        node, srcPtr->downstream, srcPtr->srcAddr,
                        interfaceId, ROUTING_DVMRP_REMOVE_FROM_LIST);
                }
            }
            else
            {
                ERROR_Assert(FALSE, "Condition shouldn't happen\n");
            }

            srcListItem = srcListItem->next;
        }
    }
}


/*
 * FUNCTION     RoutingDvmrpInit
 * PURPOSE      Initialization function for DVMRP multicast protocol
 *
 * Parameters:
 *     node:       Node being initialized
 *     dvmrpPtr:   Reference to DVMRP data structure.
 *     nodeInput:  Reference to input file.
 */
void RoutingDvmrpInit(Node *node,
               const NodeInput *nodeInput,
               int interfaceIndex)
{
    Message    *newMsg;
    BOOL       retVal;
    clocktype  delay;
    char       buf[MAX_STRING_LENGTH];
    int i;

    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    /* Allocate DVMRP layer structure */
    DvmrpData* dvmrp = (DvmrpData *)
        MEM_malloc (sizeof(DvmrpData));

    RANDOM_SetSeed(dvmrp->seed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_DVMRP,
                   interfaceIndex);

    /* DVMRP router must support AllDvmrpRouter group */
    NetworkIpAddToMulticastGroupList(node, ALL_DVMRP_ROUTER);

#ifdef CYBER_CORE
    if (ip->isIgmpEnable == TRUE)
    {
        if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
        {
            IgmpJoinGroup(node,
                          IAHEPGetIAHEPRedInterfaceIndex(node),
                          ALL_DVMRP_ROUTER,
                          (clocktype)0);

            if (IAHEP_DEBUG)
            {
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(ALL_DVMRP_ROUTER, addrStr);
                printf("\nRed Node: %d", node->nodeId);
                printf("\tJoins Group: %s\n", addrStr);
            }
        }
    }
#endif

    #ifdef DEBUG_TABLE
    {
        FILE *fp;
        fp = fopen("FWDTable.txt", "w");

        if (fp == NULL)
        {
            fprintf(stdout, "Couldn't open file FWDTable.txt\n");
        }

        fp = fopen("RTTable.txt", "w");

        if (fp == NULL)
        {
            fprintf(stdout, "Couldn't open file RTTable.txt\n");
        }
    }
    #endif

    #ifdef DEBUG_MTF7
    {
        if (node->nodeId == 8 || node->nodeId == 9)
        {
            NetworkIpAddToMulticastGroupList(node, GROUP1);
        }
    }
    #endif

    NetworkIpSetMulticastRoutingProtocol(node, dvmrp, interfaceIndex);

    NetworkIpSetMulticastRouterFunction(node,
                                        &RoutingDvmrpRouterFunction,
                                        interfaceIndex);

    /* Inform IGMP about multicast routing protocol */
    if (ip->isIgmpEnable == TRUE
        && !TunnelIsVirtualInterface(node, interfaceIndex))
    {
        IgmpSetMulticastProtocolInfo(node, interfaceIndex,
                &RoutingDvmrpLocalMembersJoinOrLeave);
    }

    /* Determine whether or not to print the stats at the end of simulation. */
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTING-STATISTICS",
        &retVal,
        buf);

    if ((retVal == FALSE) || (strcmp (buf, "NO") == 0))
    {
        dvmrp->showStat = FALSE;
    }
    else if (strcmp (buf, "YES") == 0)
    {
        dvmrp->showStat = TRUE;
    }

    /* Initializes statistics structure */
    RoutingDvmrpInitStat(node);

    /* Allocate interface structure */
    dvmrp->interface = (RoutingDvmrpInterface *)
        MEM_malloc(node->numberInterfaces
            * sizeof(RoutingDvmrpInterface));

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (NetworkIpGetInterfaceType(node, interfaceIndex) == NETWORK_IPV6)
        {
            continue;
        }
        /* Currently all interfaces are treated as physical */
        dvmrp->interface[i].interfaceType = ROUTING_DVMRP_PHYSICAL;
        dvmrp->interface[i].ipAddress = NetworkIpGetInterfaceAddress(node, i);
        dvmrp->interface[i].subnetMask=
            NetworkIpGetInterfaceSubnetMask(node, i);
        dvmrp->interface[i].probeInterval = DVMRP_PROBE_MSG_INTERVAL;
        dvmrp->interface[i].neighborTimeoutInterval = DVMRP_NEIGHBOR_TIMEOUT;
        dvmrp->interface[i].probeGenerationId = 0;
        dvmrp->interface[i].neighborCount = 0;

        /* Initialize neighbor list */
        ListInit(node, &dvmrp->interface[i].neighborList);
    }

    /* Initialize routing table */
    RoutingDvmrpInitRoutingTable(node);

    /* Initialize forwarding table */
    RoutingDvmrpInitForwardingTable(node);

    /*
     * Every DVMRP_PROBE_MSG_INTERVAL (10) seconds a router should send out
     * Probe packet on all of its local interfaces containing the list of
     * neighbor DVMRP routers for which Probe message have been receive.
     * Initiate the messaging system at the start of simulation.
     */

    delay = (clocktype) (RANDOM_erand(dvmrp->seed) *
                         ROUTING_DVMRP_BROADCAST_JITTER);

    newMsg = MESSAGE_Alloc(node,
                            NETWORK_LAYER,
                            MULTICAST_PROTOCOL_DVMRP,
                            MSG_ROUTING_DvmrpScheduleProbe);
    MESSAGE_Send(node, newMsg, (clocktype) delay);

    /*
     * Every FULL_UPDATE_RATE seconds a router should send out DVMRP messages
     * with all of its routing information to all of its interfaces. To prevent
     * routers from synchronizing when they send updates, we jitter the time.
     */
    delay = (clocktype) (ROUTING_DVMRP_BROADCAST_JITTER +
                         RANDOM_erand(dvmrp->seed) *
                         ROUTING_DVMRP_BROADCAST_JITTER);

    dvmrp->routingTable.nextBroadcastTime = delay;
    newMsg = MESSAGE_Alloc(node,
                            NETWORK_LAYER,
                            MULTICAST_PROTOCOL_DVMRP,
                            MSG_ROUTING_DvmrpPeriodUpdateAlarm);
    MESSAGE_Send(node, newMsg, (clocktype) delay);

    // Dynamic address change
    // registering RoutingDvmrpHandleAddressChangeEvent function
    NetworkIpAddAddressChangedHandlerFunction(node,
                            &RoutingDvmrpHandleChangeAddressEvent);
    #ifdef DEBUG
    {
        RoutingDvmrpPrintRoutingTable(node);
    }
    #endif
} /* DvmrpInit */



/* FUNCTION     :RoutingDvmrpHandleProtocolEvent()
 * PURPOSE      :Handle protocol event
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */

void RoutingDvmrpHandleProtocolEvent(Node *node, Message *msg)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    Message *newMsg;

    switch (msg->eventType)
    {
        /* Time to send Probe packet */
        case MSG_ROUTING_DvmrpScheduleProbe:
        {
            clocktype delay;
            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld sending PROBE at time %s\n",
                                 node->nodeId, clockStr);
            }
            #endif

            /* Send Probe packet on all of its interfaces */
            RoutingDvmrpSendProbePacket(node);

            newMsg = MESSAGE_Duplicate(node, msg);

            /* Reschedule Probe packet broadcast */
            delay = (clocktype) (RANDOM_erand(dvmrp->seed) *
                                 ROUTING_DVMRP_BROADCAST_JITTER);

            MESSAGE_Send(node, newMsg, DVMRP_PROBE_MSG_INTERVAL + delay);
            break;
        }

        /* Time to send Routing update */
        case MSG_ROUTING_DvmrpPeriodUpdateAlarm:
        {
            clocktype delay;

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld sending Route Report at time %s\n",
                                 node->nodeId, clockStr);
            }
            #endif

            /*
             * Send route update to all neighbors for all routes
             * in routing table
             */
            RoutingDvmrpSendRouteReport(node, ANY_IP);

            newMsg = MESSAGE_Duplicate(node, msg);

            /* Schedule time for next route report */
            delay = (clocktype) (RANDOM_erand(dvmrp->seed) *
                                 ROUTING_DVMRP_BROADCAST_JITTER);

            dvmrp->routingTable.nextBroadcastTime =
                                   node->getNodeTime() + DVMRP_FULL_UPDATE_RATE + delay;

            MESSAGE_Send(node, newMsg, DVMRP_FULL_UPDATE_RATE + delay);

            break;
        }

        /* Send triggered update */
        case MSG_ROUTING_DvmrpTriggeredUpdateAlarm:
        {
            NodeAddress route;

            /* Get route for which we have to send triggered update */
            memcpy(&route, MESSAGE_ReturnInfo(msg), sizeof(NodeAddress));

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld got TriggeredUpdateAlarm at "
                                "time %s\n", node->nodeId, clockStr);
                fprintf(stdout, "Node %ld sending Route Report at time %s\n",
                                 node->nodeId, clockStr);
            }
            #endif

            /* Send route update for this particular route */
            RoutingDvmrpSendRouteReport(node, route);

            break;
        }

        /* Handle route time out */
        case MSG_ROUTING_DvmrpRouteTimeoutAlarm:
        {
            NodeAddress route;
            RoutingDvmrpRoutingTableRow *rowPtr;

            /* Get route for which we have to check route timeout */
            memcpy(&route, MESSAGE_ReturnInfo(msg), sizeof(NodeAddress));

            RoutingDvmrpFindRoute(node, route, &rowPtr);

            if ((rowPtr) && (node->getNodeTime() - rowPtr->lastModified >
                        rowPtr->routeExpirationTimer))
            {
                #ifdef DEBUG
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                    fprintf(stdout, "Node %ld got RouteTimeoutAlarm at "
                                    "time %s for route %ld\n",
                                     node->nodeId, clockStr);
                }
                #endif

                RoutingDvmrpHandleRouteTimeOut(node, rowPtr);
            }
            else
            {
                #ifdef DEBUG_ERROR
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                    fprintf(stdout, "Node %ld got invalid RouteTimeoutAlarm at"
                                    " time %s for route %ld\n",
                                     node->nodeId, clockStr);
                }
                #endif
            }

            break;
        }

        case MSG_ROUTING_DvmrpGarbageCollectionAlarm:
        {
            NodeAddress route;
            RoutingDvmrpRoutingTableRow *rowPtr;

            /* Get route for which we have got alarm */
            memcpy(&route, MESSAGE_ReturnInfo(msg), sizeof(NodeAddress));

            RoutingDvmrpFindRoute(node, route, &rowPtr);

            if (rowPtr)
            {
                #ifdef DEBUG
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                    fprintf(stdout, "Node %ld delete route entry to %u at "
                                    "time %s\n",
                                     node->nodeId, route, clockStr);
                }
                #endif

                rowPtr->state = ROUTING_DVMRP_EXPIRE;
            }

            break;
        }

        case MSG_ROUTING_DvmrpNeighborTimeoutAlarm:
        {
            /* Get timer info */
            RoutingDvmrpTimerInfo *info = (RoutingDvmrpTimerInfo *)
                MESSAGE_ReturnInfo(msg);

            NodeAddress neighborAddr = info->neighborAddr;
            int interfaceIndex = info->interfaceIndex;

            RoutingDvmrpInterface *thisInterface;
            RoutingDvmrpNeighborListItem *neighbor;

            thisInterface = &dvmrp->interface[interfaceIndex];

            /* Search for the neighbor in list */
            RoutingDvmrpSearchNeighborList(thisInterface->neighborList,
                                           neighborAddr,
                                           &neighbor);

            ERROR_Assert(neighbor != NULL, "Invalid neighbor timeout alarm");
            ERROR_Assert(neighbor->ipAddr == neighborAddr,
                         "Invalid neighbor timeout alarm");

            if ((node->getNodeTime() - neighbor->lastProbeReceive) >=
                        thisInterface->neighborTimeoutInterval)
            {
                #ifdef DEBUG
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                    fprintf(stdout, "Node %ld got NeighborTimeoutAlarm at "
                                    "time %s\n", node->nodeId, clockStr);
                }
                #endif

                dvmrp->stats.numNeighbor--;
                RoutingDvmrpHandleNeighborTimeOut(node, thisInterface,
                        neighbor->ipAddr);
            }

            break;
        }

        case MSG_ROUTING_DvmrpPruneTimeoutAlarm:
        {
            RoutingDvmrpPruneTimerInfo timerInfo;

            memcpy(&timerInfo, MESSAGE_ReturnInfo(msg), sizeof(RoutingDvmrpPruneTimerInfo));
            RoutingDvmrpHandlePruneTimeoutAlarm(node, timerInfo);

            break;
        }

        case MSG_ROUTING_DvmrpGraftRtmxtTimeOut:
        {
            RoutingDvmrpGraftTimerInfo timerInfo;

            memcpy(&timerInfo, MESSAGE_ReturnInfo(msg), sizeof(RoutingDvmrpGraftTimerInfo));
            RoutingDvmrpHandleGraftRtmxtTimeOut(node, timerInfo);

            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Unknown protocol event in dvmrp\n");
        }
    }

    MESSAGE_Free(node, msg);
}



/* FUNCTION     :RoutingDvmrpHandleProtocolPacket()
 * PURPOSE      :Handle received protocol packet
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */

void RoutingDvmrpHandleProtocolPacket(Node *node, Message *msg,
        NodeAddress srcAddr, NodeAddress destAddr, int interfaceId)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);

    /* Get DVMRP common header */
    RoutingDvmrpCommonHeaderType *commonHeader =
        (RoutingDvmrpCommonHeaderType *) MESSAGE_ReturnPacket(msg);



    if (interfaceId == -1)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    if (!RoutingDvmrpIsDvmrpEnabledInterface(node, interfaceId))
    {
        MESSAGE_Free(node, msg);
        return;
    }

    /* For debugging purpose */
    #ifdef DEBUG_Route3
    {
        char clockStr[100];

        if ((node->getNodeTime() < DOWN_TIME) && (node->nodeId == 4))
        {
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld is down. Current time = %s\n",
                             node->nodeId, clockStr);
            MESSAGE_Free(node, msg);
            return;
        }
    }
    #endif

    switch (commonHeader->pktCode)
    {
        /* Probe Packet */
        case ROUTING_DVMRP_PROBE:
        {
            RoutingDvmrpProbePacketType *probePacket =
                (RoutingDvmrpProbePacketType *) MESSAGE_ReturnPacket(msg);

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld receive PROBE from node %ld at "
                                "time %s\n", node->nodeId, srcAddr, clockStr);
            }
            #endif

            dvmrp->stats.totalPacketReceive++;
            dvmrp->stats.probePacketReceive++;

            /* Process received Probe packet */
            RoutingDvmrpHandleProbePacket(node, probePacket, srcAddr,
                                          MESSAGE_ReturnPacketSize(msg), interfaceId);
            break;
        }

        /* Route Report Packet */
        case ROUTING_DVMRP_REPORT:
        {
            RoutingDvmrpRouteReportPacketType *routePacket =
                                          (RoutingDvmrpRouteReportPacketType *)
                                          MESSAGE_ReturnPacket(msg);

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld receive Route Report from node %x at"
                                " time %s\n", node->nodeId, srcAddr, clockStr);
            }
            #endif

            dvmrp->stats.totalPacketReceive++;
            dvmrp->stats.routingUpdateReceive++;

            /* Process received Route Update packet */
            RoutingDvmrpHandleRouteReportPacket(node, routePacket, srcAddr,
                                                MESSAGE_ReturnPacketSize(msg), interfaceId);

            break;
        }

        /* Prune packet */
        case ROUTING_DVMRP_PRUNE:
        {
            RoutingDvmrpPrunePacketType *prunePacket =
                (RoutingDvmrpPrunePacketType *) MESSAGE_ReturnPacket(msg);

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld receive Prune from node %ld at "
                                "time %s\n", node->nodeId, srcAddr, clockStr);
            }
            #endif

            dvmrp->stats.totalPacketReceive++;
            dvmrp->stats.pruneReceive++;

            /* Process received Prune packet */
            RoutingDvmrpHandlePrunePacket(node, prunePacket, srcAddr, interfaceId);

            break;
        }

        /* Graft packet */
        case ROUTING_DVMRP_GRAFT:
        {
            RoutingDvmrpGraftPacketType *graftPacket =
                (RoutingDvmrpGraftPacketType *) MESSAGE_ReturnPacket(msg);

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld receive Graft from node %ld at "
                                "time %s\n", node->nodeId, srcAddr, clockStr);
            }
            #endif

            dvmrp->stats.totalPacketReceive++;
            dvmrp->stats.graftReceive++;

            /* Process received Graft packet */
            RoutingDvmrpHandleGraftPacket(node, graftPacket, srcAddr, interfaceId);

            break;
        }

        /* Graft Ack packet */
        case ROUTING_DVMRP_GRAFT_ACK:
        {
            RoutingDvmrpGraftAckPacketType *graftAckPacket =
                (RoutingDvmrpGraftAckPacketType *) MESSAGE_ReturnPacket(msg);

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld receive Graft Ack from node %ld at "
                                "time %s\n", node->nodeId, srcAddr, clockStr);
            }
            #endif

            dvmrp->stats.totalPacketReceive++;
            dvmrp->stats.graftAckReceive++;

            /* Process received Graft Ack packet */
            RoutingDvmrpHandleGraftAckPacket(node, graftAckPacket, srcAddr);

            break;
        }

        default:
        {
            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld receive packet from node %ld with "
                                "unknown type at time %s\n",
                                 node->nodeId, srcAddr, clockStr);
            }
            #endif
        }
    }

    MESSAGE_Free(node, msg);
}



/* FUNCTION     :RoutingDvmrpInitStat()
 * PURPOSE      :Initializes statistics structure
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpInitStat(Node *node)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);

    dvmrp->stats.numNeighbor = 0;
    dvmrp->stats.totalPacketSent = 0;
    dvmrp->stats.probePacketSent = 0;
    dvmrp->stats.routingUpdateSent = 0;
    dvmrp->stats.pruneSent = 0;
    dvmrp->stats.graftSent = 0;
    dvmrp->stats.graftAckSent = 0;
    dvmrp->stats.totalPacketReceive = 0;
    dvmrp->stats.probePacketReceive = 0;
    dvmrp->stats.routingUpdateReceive = 0;
    dvmrp->stats.pruneReceive = 0;
    dvmrp->stats.graftReceive = 0;
    dvmrp->stats.graftAckReceive = 0;
    dvmrp->stats.numTriggerUpdate = 0;
    dvmrp->stats.numRTUpdate = 0;
    dvmrp->stats.numFTUpdate = 0;
    dvmrp->stats.numOfDataPacketOriginated = 0;
    dvmrp->stats.numOfDataPacketReceived = 0;
    dvmrp->stats.numOfDataPacketForward = 0;
    dvmrp->stats.numOfDataPacketDiscard = 0;

    dvmrp->stats.alreadyPrinted = FALSE;
}


/* FUNCTION     :RoutingDvmrpInitRoutingTable()
 * PURPOSE      :Initializes routing table
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpInitRoutingTable(Node *node)
{
    int size, i;
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpRoutingTable *routingTable = &dvmrp->routingTable;

    routingTable->numEntries = node->numberInterfaces;
    routingTable->nextBroadcastTime = 0;
    routingTable->numRowsAllocated =
        (node->numberInterfaces / DVMRP_INITIAL_TABLE_SIZE + 1) *
            DVMRP_INITIAL_TABLE_SIZE;

    size = sizeof(RoutingDvmrpRoutingTableRow) * routingTable->numRowsAllocated;

    /* Allocate rows */
    routingTable->rowPtr = (RoutingDvmrpRoutingTableRow *)
        MEM_malloc(size);

    for (i = 0; i < (signed)routingTable->numEntries; i++)
    {
        routingTable->rowPtr[i].destAddr =
            NetworkIpGetInterfaceNetworkAddress(node, i);
        routingTable->rowPtr[i].subnetMask =
            NetworkIpGetInterfaceSubnetMask(node, i);
        routingTable->rowPtr[i].nextHop =
            NetworkIpGetInterfaceAddress(node, i);
        routingTable->rowPtr[i].interfaceId = i;
        routingTable->rowPtr[i].state = ROUTING_DVMRP_VALID;
        routingTable->rowPtr[i].lastModified = node->getNodeTime();
        routingTable->rowPtr[i].metric = 0;
        routingTable->rowPtr[i].routeExpirationTimer = DVMRP_EXPIRATION_TIMEOUT;
        routingTable->rowPtr[i].triggeredUpdateScheduledFlag = FALSE;

        /* Initialize child list */
        RoutingDvmrpInitChildList(node, &routingTable->rowPtr[i].child, 0, i);
    }
}


/* FUNCTION     :RoutingDvmrpAddNewRoute()
 * PURPOSE      :Add new route to routing table
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpAddNewRoute(Node *node, NodeAddress destAddr,
        NodeAddress netmask, NodeAddress nextHop, unsigned char metric,
        int interfaceId)
{
    int i;
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpRoutingTable *routingTable = &dvmrp->routingTable;

    /* Check if the table has been filled */
    if (routingTable->numEntries == routingTable->numRowsAllocated)
    {
        RoutingDvmrpRoutingTableRow *oldRow = routingTable->rowPtr;

        /* Double route table size */
        routingTable->rowPtr = (RoutingDvmrpRoutingTableRow *)
            MEM_malloc(2 * routingTable->numRowsAllocated *
                sizeof(RoutingDvmrpRoutingTableRow));

        for (i = 0; i < (signed)routingTable->numEntries; i++)
        {
            routingTable->rowPtr[i] = oldRow[i];
        }
        MEM_free(oldRow);

        /* Update table size */
        routingTable->numRowsAllocated *= 2;
    }

    /* Insert route. */

    for (i = 0; i < (signed)routingTable->numEntries; i++)
    {
        if ((routingTable->rowPtr[i].destAddr == destAddr) &&
            (routingTable->rowPtr[i].subnetMask == netmask))
        {
            if (routingTable->rowPtr[i].state == ROUTING_DVMRP_EXPIRE)
            {
                routingTable->rowPtr[i].nextHop = nextHop;
                routingTable->rowPtr[i].metric = metric;
                routingTable->rowPtr[i].interfaceId = interfaceId;
                routingTable->rowPtr[i].state = ROUTING_DVMRP_VALID;
                routingTable->rowPtr[i].lastModified = node->getNodeTime();

                /* Update child list */
                RoutingDvmrpUpdateChildList(node,
                        &routingTable->rowPtr[i].child,
                        metric,
                        interfaceId);
            }

            return;
        }
    }

    /* New route, so add it to routing table. */
    routingTable->rowPtr[i].destAddr = destAddr;
    routingTable->rowPtr[i].subnetMask = netmask;
    routingTable->rowPtr[i].nextHop = nextHop;
    routingTable->rowPtr[i].metric = metric;
    routingTable->rowPtr[i].interfaceId = interfaceId;
    routingTable->rowPtr[i].state = ROUTING_DVMRP_VALID;
    routingTable->rowPtr[i].lastModified = node->getNodeTime();
    routingTable->rowPtr[i].routeExpirationTimer = DVMRP_EXPIRATION_TIMEOUT;
    routingTable->rowPtr[i].triggeredUpdateScheduledFlag = FALSE;

    /* Initialize child list */
    RoutingDvmrpInitChildList(node, &routingTable->rowPtr[i].child, metric,
                              interfaceId);

    routingTable->numEntries++;
}


/* FUNCTION     :RoutingDvmrpUpdateRoute()
 * PURPOSE      :Update a route in routing table
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpUpdateRoute(Node *node, NodeAddress destAddr,
        NodeAddress netmask, NodeAddress nextHop, unsigned char metric,
        int interfaceId)
{
    int i;
    BOOL flashUpdate = FALSE;
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpRoutingTable *routingTable = &dvmrp->routingTable;

    #ifdef DEBUG_ERROR
    {
        RoutingDvmrpPrintRoutingTable(node);
    }
    #endif

    for (i = 0; i < (signed)routingTable->numEntries; i++)
    {
        if ((routingTable->rowPtr[i].destAddr == destAddr) &&
            (routingTable->rowPtr[i].subnetMask == netmask))
        {

            /* If we need to send flash update */
            if ((metric != routingTable->rowPtr[i].metric) ||
                (nextHop != routingTable->rowPtr[i].nextHop) ||
                (interfaceId != routingTable->rowPtr[i].interfaceId))
            {
                flashUpdate = TRUE;
            }

            routingTable->rowPtr[i].state = ROUTING_DVMRP_VALID;
            routingTable->rowPtr[i].routeExpirationTimer =
                DVMRP_EXPIRATION_TIMEOUT;

            if ((metric < routingTable->rowPtr[i].metric) &&
                (interfaceId == routingTable->rowPtr[i].interfaceId))
            {

                /*
                 * We have got a better metric from the previous
                 * upstream, so check all the downstream interfaces if
                 * we can advertise a better metric than the existing
                 * designated forwarder advertise.
                 */
                RoutingDvmrpUpdateDsgFwdIfRequired(
                        routingTable->rowPtr[i].child, metric);
            }
            else if ((metric > routingTable->rowPtr[i].metric) &&
                     (nextHop == routingTable->rowPtr[i].nextHop))
            {

                /* Update advertize metric on downstream */
                ListItem *tempListItem;
                RoutingDvmrpChildListItem *childInfo;

                /*
                 * Change advertiseMetric on interfaces on which
                 * this router is designated forwarder
                 */
                tempListItem = routingTable->rowPtr[i].child->first;

                while (tempListItem)
                {
                    childInfo = (RoutingDvmrpChildListItem *)
                            tempListItem->data;
                    if (childInfo->designatedForwarder ==
                            childInfo->ipAddr)
                    {
                        // Fix Start
                        //childInfo->advertiseMetric =
                        //    routingTable->rowPtr[i].metric;
                        childInfo->advertiseMetric = metric;
                        // Fix End
                    }
                    tempListItem = tempListItem->next;
                }
            }

            routingTable->rowPtr[i].metric = metric;
            routingTable->rowPtr[i].nextHop = nextHop;

            if (interfaceId != routingTable->rowPtr[i].interfaceId)
            {

                /* We need to update child list */
                RoutingDvmrpUpdateChildList(node,
                        &routingTable->rowPtr[i].child, metric, interfaceId);
            }

            routingTable->rowPtr[i].interfaceId = interfaceId;

            /* Refresh timer and schedule next route timeout */
            routingTable->rowPtr[i].lastModified = node->getNodeTime();

            RoutingDvmrpSetTimer(node, MSG_ROUTING_DvmrpRouteTimeoutAlarm,
                    (void *) &routingTable->rowPtr[i].destAddr,
                    DVMRP_EXPIRATION_TIMEOUT);

            if (flashUpdate)
            {

                /* Schedule triggered update */
                RoutingDvmrpSetTimer(node,
                        MSG_ROUTING_DvmrpTriggeredUpdateAlarm,
                        (void *) &routingTable->rowPtr[i].destAddr,
                        DVMRP_TRIGGERED_UPDATE_RATE);
            }

            break;
        }
    }

    ERROR_Assert((unsigned)i != routingTable->numEntries, "Route not found\n");

    #ifdef DEBUG
    {
        char clockStr[100];

        ctoa(node->getNodeTime(), clockStr);

        fprintf(stdout, "Node %ld update route %u at time %s\n",
                         node->nodeId, destAddr, clockStr);
        RoutingDvmrpPrintRoutingTable(node);
    }
    #endif
}


/* FUNCTION     :RoutingDvmrpUpdateDsgFwdIfRequired()
 * PURPOSE      :If condition holds, update designated fowwarder on downstream.
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpUpdateDsgFwdIfRequired(LinkedList *childList,
        unsigned char metric)
{
    ListItem *tempListItem = childList->first;
    RoutingDvmrpChildListItem *childInfo;

    while (tempListItem)
    {
        childInfo = (RoutingDvmrpChildListItem *) tempListItem->data;

        if ((childInfo->advertiseMetric > metric) ||
            ((childInfo->advertiseMetric == metric) &&
                 (childInfo->designatedForwarder >
                  childInfo->ipAddr)))
        {

            childInfo->advertiseMetric = metric;
            childInfo->designatedForwarder = childInfo->ipAddr;
        }
        tempListItem = tempListItem->next;
    }
}


/* FUNCTION     :RoutingDvmrpInitChildList()
 * PURPOSE      :Initialize child list
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpInitChildList(Node *node, LinkedList **childList,
        unsigned char metric, int upstream)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpChildListItem *tempItem;
    int i;

    ListInit(node, childList);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        /* Skip upstream interface */
        if (i == upstream)
        {
            continue;
        }

        tempItem = (RoutingDvmrpChildListItem *)
            MEM_malloc(sizeof(RoutingDvmrpChildListItem));

        tempItem->interfaceIndex = i;
        tempItem->ipAddr = NetworkIpGetInterfaceAddress(node, i);

        /* Assume the role of designated forwarder initially */
        tempItem->designatedForwarder = NetworkIpGetInterfaceAddress(node, i);
        tempItem->advertiseMetric = metric;

        /* Initialize dependent neighbor list */
        ListInit(node, &tempItem->dependentNeighbor);
        if (dvmrp->interface[i].neighborCount != 0)
        {
            NodeAddress *addr;
            int count = 0;
            ListItem *tempListItem =
                dvmrp->interface[i].neighborList->first;

            while (count < dvmrp->interface[i].neighborCount)
            {
                RoutingDvmrpNeighborListItem *tempNBInfo;

                tempNBInfo = (RoutingDvmrpNeighborListItem *)
                    tempListItem->data;
                addr = (NodeAddress *) MEM_malloc(sizeof(NodeAddress));
                *addr = tempNBInfo->ipAddr;

                /* Insert to list */
                ListInsert(node, tempItem->dependentNeighbor, node->getNodeTime(),
                           (void *) addr);

                tempListItem = tempListItem->next;
                count++;
            }
            tempItem->isLeaf = FALSE;
        }
        else
        {
            tempItem->isLeaf = TRUE;
        }

        /* Now add to list */
        ListInsert(node, *childList, node->getNodeTime(), (void *) tempItem);
    }
}


/* FUNCTION     :RoutingDvmrpGetChild()
 * PURPOSE      :Get child information
 * ASSUMPTION   :None
 * RETURN VALUE :RoutingDvmrpChildListItem*
 */
static RoutingDvmrpChildListItem*
RoutingDvmrpGetChild(LinkedList *childList, int interfaceId)
{
    ListItem *tempListItem;
    RoutingDvmrpChildListItem *tempItem;

    tempListItem = childList->first;

    while (tempListItem)
    {
        tempItem = (RoutingDvmrpChildListItem *) tempListItem->data;

        if (tempItem->interfaceIndex == interfaceId)
        {
            return (tempItem);
        }
        tempListItem = tempListItem->next;
    }
    return NULL;
}

/* FUNCTION     :RoutingDvmrpUpdateChildList()
 * PURPOSE      :Update child list
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpUpdateChildList(Node *node, LinkedList **childList,
        unsigned char metric, int upstream)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    LinkedList *tempChildList;
    RoutingDvmrpChildListItem *tempItem;
    int i;

    ListInit(node, &tempChildList);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        RoutingDvmrpChildListItem *prevChild =
                RoutingDvmrpGetChild(*childList, i);

        /* Skip upstream interface */
        if (i == upstream)
        {
            continue;
        }

        tempItem = (RoutingDvmrpChildListItem *)
            MEM_malloc(sizeof(RoutingDvmrpChildListItem));

        tempItem->interfaceIndex = i;
        tempItem->ipAddr = NetworkIpGetInterfaceAddress(node, i);

        /* Determine designated forwarder on this network */
        if (prevChild != NULL)
        {
            if ((prevChild->advertiseMetric > metric) ||
                     ((prevChild->advertiseMetric == metric) &&
                          (prevChild->designatedForwarder > tempItem->ipAddr)))
            {
                tempItem->designatedForwarder = tempItem->ipAddr;
                tempItem->advertiseMetric = metric;
            }
            else
            {
                tempItem->designatedForwarder = prevChild->designatedForwarder;
                tempItem->advertiseMetric = prevChild->advertiseMetric;
            }
        }
        else
        {
            /*
             * If this interface was not in the child list, assume the role of
             * designated forwarder on this network
             */
            tempItem->designatedForwarder = tempItem->ipAddr;
            tempItem->advertiseMetric = metric;
        }

        /* Initialize dependent neighbor list */
        ListInit(node, &tempItem->dependentNeighbor);
        if (dvmrp->interface[i].neighborCount != 0)
        {
            NodeAddress *addr;
            int count = 0;
            ListItem *tempListItem =
                dvmrp->interface[i].neighborList->first;

            while (count < dvmrp->interface[i].neighborCount)
            {
                RoutingDvmrpNeighborListItem *tempNBInfo;

                tempNBInfo = (RoutingDvmrpNeighborListItem *)
                    tempListItem->data;
                addr = (NodeAddress *) MEM_malloc(sizeof(NodeAddress));
                *addr = tempNBInfo->ipAddr;

                /* Insert to list */
                ListInsert(node, tempItem->dependentNeighbor, node->getNodeTime(),
                           (void *) addr);

                tempListItem = tempListItem->next;
                count++;
            }
            tempItem->isLeaf = FALSE;
        }
        else
        {
            tempItem->isLeaf = TRUE;
        }

        /* Now add to list */
        ListInsert(node, tempChildList, node->getNodeTime(), (void *) tempItem);
    }

    /* Free old list */
    ListFree(node, *childList, FALSE);

    /* Set new list */
    *childList = tempChildList;
}


/* FUNCTION     :RoutingDvmrpIsChildInterface()
 * PURPOSE      :Check if the interface is in the child list for this route
 * ASSUMPTION   :None
 * RETURN VALUE :BOOL
 */
static BOOL RoutingDvmrpIsChildInterface(RoutingDvmrpRoutingTableRow *rowPtr,
        int interfaceId)
{
    ListItem *tempListItem;
    RoutingDvmrpChildListItem *tempChildInfo;

    tempListItem = rowPtr->child->first;

    /* Search entire child list */
    while (tempListItem)
    {
        tempChildInfo = (RoutingDvmrpChildListItem *) tempListItem->data;

        if (tempChildInfo->interfaceIndex == interfaceId)
        {
            return TRUE;
        }

        tempListItem = tempListItem->next;
    }
    return FALSE;
}



/* FUNCTION     :RoutingDvmrpIsDependentNeighbor()
 * PURPOSE      :Check if the node is dependent for this route
 * ASSUMPTION   :None
 * RETURN VALUE :BOOL
 */
static BOOL RoutingDvmrpIsDependentNeighbor(RoutingDvmrpRoutingTableRow *rowPtr,
        NodeAddress neighborAddr, int interfaceId)
{
    ListItem *tempChildItem;
    RoutingDvmrpChildListItem *tempChildInfo;

    tempChildItem = rowPtr->child->first;

    /* Search through entire child list */
    while (tempChildItem)
    {
        tempChildInfo = (RoutingDvmrpChildListItem *) tempChildItem->data;

        if (tempChildInfo->interfaceIndex == interfaceId)
        {
            ListItem *tempNBItem = tempChildInfo->dependentNeighbor->first;

            while (tempNBItem)
            {
                NodeAddress *addr = (NodeAddress *) tempNBItem->data;

                if (*addr == neighborAddr)
                {
                    return TRUE;
                }

                tempNBItem = tempNBItem->next;
            }
            break;
        }
        tempChildItem = tempChildItem->next;
    }
    return FALSE;
}


/* FUNCTION     :RoutingDvmrpRemoveDependency()
 * PURPOSE      :Remove neighbor from dependent neighbor list
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpRemoveDependency(Node *node,
        RoutingDvmrpRoutingTableRow *rowPtr, NodeAddress neighborAddr,
        int interfaceId)
{
    ListItem *tempChildItem;
    RoutingDvmrpChildListItem *tempChildInfo;

    tempChildItem = rowPtr->child->first;

    /* Search through entire child list */
    while (tempChildItem)
    {
        tempChildInfo = (RoutingDvmrpChildListItem *)
            tempChildItem->data;

        if (tempChildInfo->interfaceIndex == interfaceId)
        {
            ListItem *tempNBItem =
                tempChildInfo->dependentNeighbor->first;

            while (tempNBItem)
            {
                NodeAddress *addr =
                    (NodeAddress *) tempNBItem->data;

                if (*addr == neighborAddr)
                {
                    /* Remove from list */
                    ListGet(node,
                            tempChildInfo->dependentNeighbor,
                            tempNBItem, TRUE, FALSE);

                    /*
                     * Check if there is still dependent neighbor. Otherwise
                     * make this network as leaf network for this source
                     */
                    if (tempChildInfo->dependentNeighbor->size == 0)
                    {
                        tempChildInfo->isLeaf = TRUE;
                    }

                    return;
                }

                tempNBItem = tempNBItem->next;
            }

            ERROR_Assert(FALSE, "Attemt to remove Invalid Dependency");
        }

        tempChildItem = tempChildItem->next;
    }
}


/* FUNCTION     :RoutingDvmrpAddDependentNeighbor()
 * PURPOSE      :Search for a route
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpAddDependentNeighbor(Node *node,
        RoutingDvmrpRoutingTableRow *rowPtr, NodeAddress neighborIp,
        int interfaceId)
{
    ListItem *tempChildItem;
    RoutingDvmrpChildListItem *tempChildInfo;

    tempChildItem = rowPtr->child->first;

    /* Search through entire child list */
    while (tempChildItem)
    {
        tempChildInfo = (RoutingDvmrpChildListItem *)
            tempChildItem->data;

        if (tempChildInfo->interfaceIndex == interfaceId)
        {
            NodeAddress *addr = (NodeAddress *)
                MEM_malloc(sizeof(NodeAddress));

            *addr = neighborIp;

            /* Now insert item to list */
            ListInsert(node, tempChildInfo->dependentNeighbor,
                       node->getNodeTime(), (void *) addr);

            /* As we have dependent neighbor, this interface can not be leaf */
            tempChildInfo->isLeaf = FALSE;

            break;
        }
        tempChildItem = tempChildItem->next;
    }
}


/* FUNCTION     :RoutingDvmrpGetDesignatedForwarder()
 * PURPOSE      :Get designated forwarder on a network for a source
 * ASSUMPTION   :None
 * RETURN VALUE :NodeAddress
 */
static NodeAddress RoutingDvmrpGetDesignatedForwarder(
        RoutingDvmrpRoutingTableRow *rowPtr, int interfaceId)
{
    ListItem *tempChildItem;
    RoutingDvmrpChildListItem *tempChildInfo;

    tempChildItem = rowPtr->child->first;

    /* Search through entire child list */
    while (tempChildItem)
    {
        tempChildInfo = (RoutingDvmrpChildListItem *)
            tempChildItem->data;

        if (tempChildInfo->interfaceIndex == interfaceId)
        {
            return (tempChildInfo->designatedForwarder);
        }
        tempChildItem = tempChildItem->next;
    }
    return (unsigned) (-1);
}


/* FUNCTION     :RoutingDvmrpSetDesignatedForwarder()
 * PURPOSE      :Set the router as the  designated forwarder on a network
 *               for the source
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpSetDesignatedForwarder(
        RoutingDvmrpRoutingTableRow *rowPtr, int interfaceId)
{
    ListItem *tempChildItem;
    RoutingDvmrpChildListItem *tempChildInfo;

    tempChildItem = rowPtr->child->first;

    /* Search through entire child list */
    while (tempChildItem)
    {
        tempChildInfo = (RoutingDvmrpChildListItem *)
            tempChildItem->data;

        if (tempChildInfo->interfaceIndex == interfaceId)
        {
            tempChildInfo->designatedForwarder = tempChildInfo->ipAddr;
            tempChildInfo->advertiseMetric = rowPtr->metric;
            break;
        }
        tempChildItem = tempChildItem->next;
    }
}


/* FUNCTION     :RoutingDvmrpFindRoute()
 * PURPOSE      :Search for a route
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpFindRoute(Node *node, NodeAddress destAddr,
        RoutingDvmrpRoutingTableRow **rowPtr)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpRoutingTable *routingTable = &dvmrp->routingTable;
    int i;

    *rowPtr = NULL;

    for (i = 0; i < (signed)routingTable->numEntries; i++)
    {
        NodeAddress maskedDestAddr =
            MaskIpAddress(destAddr, routingTable->rowPtr[i].subnetMask);

        if (routingTable->rowPtr[i].destAddr == maskedDestAddr)
        {
            *rowPtr = &routingTable->rowPtr[i];
            break;
        }
    }
}


/* FUNCTION     :RoutingDvmrpInitForwardingTable()
 * PURPOSE      :Initializes forwarding table
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpInitForwardingTable(Node *node)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpForwardingTable *forwardingTable = &dvmrp->forwardingTable;
    int size = sizeof(RoutingDvmrpForwardingTableRow) *
        DVMRP_INITIAL_TABLE_SIZE;

    forwardingTable->numEntries = 0;
    forwardingTable->numRowsAllocated = 10;
    forwardingTable->rowPtr = (RoutingDvmrpForwardingTableRow *)
        MEM_malloc(size);
    memset(forwardingTable->rowPtr, 0, size);
}


/* FUNCTION     :RoutingDvmrpCreateCommonHeader()
 * PURPOSE      :Create common header for different dvrmp packet
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpCreateCommonHeader(
        RoutingDvmrpCommonHeaderType *commonHeader,
        RoutingDvmrpPacketCode pktCode)
{
    commonHeader->type = 0x13;
    commonHeader->pktCode = (unsigned char) pktCode;

    /* Not used in simulation */
    commonHeader->checksum = 0;

    commonHeader->reserved = 0;
    commonHeader->minorVersion = 0xFF;
    commonHeader->majorVersion = 0x3;
}



/* FUNCTION     :RoutingDvmrpSendProbePacket()
 * PURPOSE      :Send probe packet on each interface
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpSendProbePacket(Node *node)
{
    Message *probeMsg;
    RoutingDvmrpProbePacketType *probePacket;
    clocktype delay;
    NodeAddress *neighborList = NULL;
    int numNeighbor;
    int i;

    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);


    /* For debugging purpose */
    #ifdef DEBUG_Route3
    {
        char clockStr[100];

        if ((node->getNodeTime() < DOWN_TIME) && (node->nodeId == 4))
        {
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld is down. Current time = %s\n",
                             node->nodeId, clockStr);
            return;
        }
    }
    #endif

    /* Create packet for each interface */
    for (i = 0; i < node->numberInterfaces; i++)
    {

        /* Skip those interfaces that are not DVMRP enabled */
        if (!RoutingDvmrpIsDvmrpEnabledInterface(node, i))
        {
            continue;
        }

        numNeighbor = dvmrp->interface[i].neighborCount;

        /* Get all neighbor learned through this interface */
        neighborList = RoutingDvmrpFillNeighborList(node, i);

        /* Create Probe packet */
        probeMsg = MESSAGE_Alloc(node,
                                  NETWORK_LAYER,
                                  MULTICAST_PROTOCOL_DVMRP,
                                  MSG_ROUTING_DvmrpPacket);
        MESSAGE_PacketAlloc(node,
                             probeMsg,
                             sizeof(RoutingDvmrpProbePacketType) +
                                 (sizeof(NodeAddress) * numNeighbor),
                             TRACE_DVMRP);

        probePacket = (RoutingDvmrpProbePacketType *)
            MESSAGE_ReturnPacket(probeMsg);

        #ifdef DEBUG_ERROR
        {
            int size = MESSAGE_ReturnPacketSize(probeMsg);
            fprintf(stdout, "Node %ld  interface = %d :: Allocated packet "
                            "size = %d, numNeighbor = %d\n",
                             node->nodeId, i, size, numNeighbor);
            fprintf(stdout, "\t\tSize of Probe header = %d\n",
                             sizeof(RoutingDvmrpProbePacketType));
        }
        #endif

        /* Create common header */
        RoutingDvmrpCreateCommonHeader(&probePacket->commonHeader,
                                       ROUTING_DVMRP_PROBE);

        probePacket->generationId = dvmrp->interface[i].probeGenerationId++;

        if (neighborList)
        {
            memcpy(probePacket + 1, neighborList,
                numNeighbor * sizeof(NodeAddress));
        }
        else
        {
            #ifdef DEBUG_ERROR
            {
                fprintf(stdout, "Node %ld has no neighbor on interface %ld\n",
                                node->nodeId, i);
            }
            #endif
        }

        /* Used to jitter Probe packet broadcast */
        //delay = RANDOM_erand(dvmrp->seed) * ROUTING_DVMRP_BROADCAST_JITTER;
        delay = 0;

        /* Now send packet out through this interface */
        NetworkIpSendRawMessageToMacLayerWithDelay(
                                      node,
                                      MESSAGE_Duplicate(node, probeMsg),
                                      NetworkIpGetInterfaceAddress(node, i),
                                      ALL_DVMRP_ROUTER,
                                      IPTOS_PREC_INTERNETCONTROL,
                                      IPPROTO_IGMP,
                                      1,
                                      i,
                                      ANY_DEST,
                                      delay);

        dvmrp->stats.totalPacketSent++;
        dvmrp->stats.probePacketSent++;

        MEM_free(neighborList);
        MESSAGE_Free(node, probeMsg);
    }
}


/* FUNCTION     :RoutingDvmrpHandleProbePacket()
 * PURPOSE      :Process received Probe packet
 * ASSUMPTION   :Router doesn't restarted during simulation
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpHandleProbePacket(Node *node,
        RoutingDvmrpProbePacketType *probePacket, NodeAddress sourceAddr,
        int size, int interfaceIndex)
{

    RoutingDvmrpInterface *thisInterface;
    int neighborCount;
    NodeAddress *tempNeighbor;
    RoutingDvmrpNeighborListItem *neighbor;
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpTimerInfo timerInfo;


    thisInterface = &dvmrp->interface[interfaceIndex];

    neighborCount =
        (size - sizeof(RoutingDvmrpProbePacketType)) / sizeof(NodeAddress);

    #ifdef DEBUG_ERROR
    {
        fprintf(stdout, "Node %ld :: Received packet size = %d "
                        "neighbor in packet = %d\n",
                        node->nodeId, size, neighborCount);
    }
    #endif

    /* Get the neighbor list from packet */
    if (neighborCount)
    {
        tempNeighbor = (NodeAddress *) (probePacket + 1);
    }
    else
    {
        tempNeighbor = NULL;
    }

    /* Search neighbor in neighbor list */
    RoutingDvmrpSearchNeighborList(thisInterface->neighborList,
                                   sourceAddr,
                                   &neighbor);

    /* New neighbor */
    if (!neighbor)
    {
        #ifdef DEBUG
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld found new neighbor %ld on interface %d "
                            "at time %s\n",
                            node->nodeId, sourceAddr, interfaceIndex, clockStr);
        }
        #endif

        /* Allocate space for new neighbor */
        neighbor = (RoutingDvmrpNeighborListItem *)
                   MEM_malloc(sizeof(RoutingDvmrpNeighborListItem));

        neighbor->ipAddr = sourceAddr;
        neighbor->lastGenIdReceive = probePacket->generationId;
        neighbor->lastProbeReceive = node->getNodeTime();
        neighbor->relation = ROUTING_DVMRP_ONE_WAY_NEIGHBOR;

        /* Add to neighbor list */
        ListInsert(node,
                   thisInterface->neighborList,
                   node->getNodeTime(),
                   (void *) neighbor);

        thisInterface->neighborCount++;
        dvmrp->stats.numNeighbor++;
    }
    else
    {
        if (++neighbor->lastGenIdReceive != probePacket->generationId)
        {
            neighbor->lastProbeReceive = node->getNodeTime();
            neighbor->lastGenIdReceive = probePacket->generationId;

            /* TBD: Neighbor has been restarted. We need to take some action */
        }
        else
        {
            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld refresh neighbor expiry timer for "
                                "%ld at time %s\n",
                                 node->nodeId, sourceAddr, clockStr);
            }
            #endif

            /* Refresh neighbor expiry timer */
            neighbor->lastProbeReceive = node->getNodeTime();
        }
    }

    /* Whether this neighbor include myself in probe message list */
    while (neighborCount > 0)
    {
        if (*tempNeighbor++ == thisInterface->ipAddress)
        {
            #ifdef DEBUG
            {
                fprintf(stdout, "Node %ld found himself in neighbor %ld's "
                                "probe packet\n",
                                node->nodeId, sourceAddr);
            }
            #endif

            /* Establish two way connection */
            if (neighbor->relation != ROUTING_DVMRP_TWO_WAY_NEIGHBOR)
            {
                Message *routeMsg;
                clocktype delay;
                int rowIndex = 0;

                neighbor->relation = ROUTING_DVMRP_TWO_WAY_NEIGHBOR;

                #ifdef DEBUG
                {
                    fprintf(stdout, "Node %ld establish Two-Way connection "
                                    "with node %ld\n",
                                    node->nodeId, sourceAddr);
                }
                #endif

                while (rowIndex < (signed) dvmrp->routingTable.numEntries)
                {
                    BOOL isPacket;

                    /* Allocate message for packet */
                    routeMsg = MESSAGE_Alloc(node, NETWORK_LAYER,
                                              MULTICAST_PROTOCOL_DVMRP,
                                              MSG_ROUTING_DvmrpPacket);

                    /* Create route report packet */
                    isPacket = RoutingDvmrpCreateRouteReportPacket(
                                                         node,
                                                         &rowIndex,
                                                         routeMsg,
                                                         ANY_IP,
                                                         interfaceIndex);
                    if (isPacket)
                    {
                        #ifdef DEBUG
                        {
                            fprintf(stdout, "Node %ld sending Route report to "
                                            "node %ld\n",
                                            node->nodeId, sourceAddr);
                        }
                        #endif

                        /* Used to jitter Route report packet broadcast */
                        //delay = RANDOM_erand(dvmrp->seed)
                        //            * ROUTING_DVMRP_BROADCAST_JITTER;
                        delay = 0;

                        /* Now send packet out through this interface */
                        NetworkIpSendRawMessageToMacLayerWithDelay(
                                          node,
                                          MESSAGE_Duplicate(node, routeMsg),
                                          thisInterface->ipAddress,
                                          sourceAddr,
                                          IPTOS_PREC_INTERNETCONTROL,
                                          IPPROTO_IGMP,
                                          1,
                                          interfaceIndex,
                                          sourceAddr,
                                          delay);

                        dvmrp->stats.totalPacketSent++;
                        dvmrp->stats.routingUpdateSent++;
                    }

                    MESSAGE_Free(node, routeMsg);
                }//end while
            }

            break;
        }

        neighborCount--;
    }

    if (neighborCount == 0)
    {
        neighbor->relation = ROUTING_DVMRP_ONE_WAY_NEIGHBOR;
    }

    /* Send neighbor expiry timer to self for this neighbor */
    timerInfo.neighborAddr = sourceAddr;
    timerInfo.interfaceIndex = interfaceIndex;

    RoutingDvmrpSetTimer(node, MSG_ROUTING_DvmrpNeighborTimeoutAlarm,
                         (void *) &timerInfo,
                         thisInterface->neighborTimeoutInterval);
}


/* FUNCTION     :RoutingDvmrpFillNeighborList()
 * PURPOSE      :Fill Probe packet neighbor list
 * ASSUMPTION   :None
 * RETURN VALUE :NodeAddress*
 */
static NodeAddress *RoutingDvmrpFillNeighborList(Node *node,
        int interfaceIndex)
{
    ListItem *tempListItem;
    RoutingDvmrpNeighborListItem *tempNBInfo;
    NodeAddress *neighborList;
    int neighborCount;
    int count = 0;
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);

    neighborCount = dvmrp->interface[interfaceIndex].neighborCount;

    #ifdef DEBUG_ERROR
    {
        fprintf(stdout, "Node %ld Interface %d :: Number of neighbor = %d\n",
                         node->nodeId, interfaceIndex, neighborCount);
    }
    #endif

    if (neighborCount == 0)
    {
        neighborList = NULL;
    }
    else
    {
        neighborList = (NodeAddress *)
            MEM_malloc(sizeof(NodeAddress) * neighborCount);
    }

    tempListItem = dvmrp->interface[interfaceIndex].neighborList->first;

    while (tempListItem)
    {
        tempNBInfo = (RoutingDvmrpNeighborListItem *) tempListItem->data;

        /* Add neighbor to list */
        neighborList[count] = tempNBInfo->ipAddr;

        tempListItem = tempListItem->next;
        count++;
    }
    ERROR_Assert(count == neighborCount, "Invalid neighborCount");

    return (neighborList);
}


/* FUNCTION     :RoutingDvmrpSearchNeighborList()
 * PURPOSE      :Search neighbor list for specific neighbor
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpSearchNeighborList(LinkedList *list, NodeAddress addr,
        RoutingDvmrpNeighborListItem **item)
{
    ListItem *listItem;

    ERROR_Assert(list != NULL, "Neighbor list is not initialized");

    listItem = list->first;

    while ((listItem != NULL) && (((RoutingDvmrpNeighborListItem *)
               listItem->data)->ipAddr != addr))
    {
        listItem = listItem->next;
    }

    *item = (RoutingDvmrpNeighborListItem *)
        ((!listItem) ? NULL : listItem->data);
}


/* FUNCTION     :RoutingDvmrpHandleNeighborTimeOut()
 * PURPOSE      :Take action if a neighbor timed out
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpHandleNeighborTimeOut(Node *node,
        RoutingDvmrpInterface *interface, NodeAddress nbrAddr)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node,  MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpRoutingTable *rtTable = &dvmrp->routingTable;
    RoutingDvmrpForwardingTable *fwdTable = &dvmrp->forwardingTable;
    RoutingDvmrpRoutingTableRow *rowPtr;
    int interfaceId = NetworkIpGetInterfaceIndexForNextHop(node, nbrAddr);
    int i;
    ListItem *listItem = interface->neighborList->first;

    /* Search the neighbor item */
    while (listItem)
    {
        RoutingDvmrpNeighborListItem *neighbor =
            (RoutingDvmrpNeighborListItem *) listItem->data;

        if (neighbor->ipAddr == nbrAddr)
        {
            break;
        }
        listItem = listItem->next;
    }

    interface->neighborCount--;

    /* Remove neighbor from neighbor list */
    ListGet(node, interface->neighborList, listItem, TRUE, FALSE);

    /*
     * All routes learned from this neighbor should be immediately
     * placed in hold-down.
     */
    for (i = 0; i < (signed)rtTable->numEntries; i++)
    {
        if (rtTable->rowPtr[i].nextHop == nbrAddr)
        {
            rtTable->rowPtr[i].state = ROUTING_DVMRP_HOLD_DOWN;
        }
    }

    /*
     * If this neighbor is considered to be the designated forwarder for
     * any of the routes it is advertising, a new designated forwarder
     * for each source network should be selected.
     */
    for (i = 0; i < (signed)rtTable->numEntries; i++)
    {
        if ((rtTable->rowPtr[i].state == ROUTING_DVMRP_VALID) &&
            (rtTable->rowPtr[i].interfaceId != interfaceId))
        {
            ListItem *childItem = rtTable->rowPtr[i].child->first;
            RoutingDvmrpChildListItem *childInfo;

            while (childItem)
            {
                childInfo = (RoutingDvmrpChildListItem *) childItem->data;

                if (childInfo->designatedForwarder == nbrAddr)
                {
                    childInfo->designatedForwarder = childInfo->ipAddr;
                    childInfo->advertiseMetric = rtTable->rowPtr[i].metric;
                }

                /*
                 * If this neighbor was considered as dependent, then remove
                 * from dependent neighbor list
                 */
                if (RoutingDvmrpIsDependentNeighbor(&rtTable->rowPtr[i],
                            nbrAddr, interfaceId))
                {
                    RoutingDvmrpRemoveDependency(node, &rtTable->rowPtr[i],
                            nbrAddr, interfaceId);
                }
                childItem = childItem->next;
            }
        }
    }

    /* Update forwarding table */
    for (i = 0; i < (signed)fwdTable->numEntries; i++)
    {
        ListItem *srcItem = fwdTable->rowPtr[i].srcList->first;

        while (srcItem)
        {
            RoutingDvmrpSourceList *srcInfo =
                (RoutingDvmrpSourceList *) srcItem->data;

            RoutingDvmrpFindRoute(node, srcInfo->srcAddr, &rowPtr);

            /*
             * Any forwarding cache entries based on this upstream neighbor
             * should be flushed.
             */
            if (srcInfo->prevHop == nbrAddr)
            {
                ListItem *item = srcItem;

                srcItem = srcItem->next;

                RoutingDvmrpRemoveFromSourceList(node,
                        fwdTable->rowPtr[i].srcList, item);
                continue;
            }
            else if (srcInfo->upstream != interfaceId)
            {
                ListItem *item = srcInfo->downstream->first;
                RoutingDvmrpDownstreamListItem *downstreamInfo;

                while (item)
                {
                    downstreamInfo =
                        (RoutingDvmrpDownstreamListItem *) item->data;

                    if (downstreamInfo->interfaceIndex == interfaceId)
                    {
                        NodeAddress dsgFwd = RoutingDvmrpGetDesignatedForwarder(
                                               rowPtr, interfaceId);

                        /* Remove dependent neighbor, if exist */
                        RoutingDvmrpUpdateDependentNeighborList(node,
                                downstreamInfo->dependentNeighbor, nbrAddr,
                                ROUTING_DVMRP_REMOVE_FROM_LIST);

                        /*
                         * If there is no more dependent neighbor over this
                         * interface and we are not the designated forwarder,
                         * remove this interface
                         */
                        if ((downstreamInfo->dependentNeighbor->size == 0) &&
                                (dsgFwd != interface->ipAddress))
                        {
                            RoutingDvmrpUpdateDownstreamList(node,
                                    srcInfo->downstream, srcInfo->srcAddr,
                                    interfaceId,
                                    ROUTING_DVMRP_REMOVE_FROM_LIST);
                        }
                        break;
                    }
                    item = item->next;
                }
            }
            srcItem = srcItem->next;
        }
    }
}




/* FUNCTION     :RoutingDvmrpSendRouteReport()
 * PURPOSE      :To send periodic routing update
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpSendRouteReport(Node *node, NodeAddress route)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);


    Message *routeMsg;
    clocktype delay;
    int rowIndex;
    int i;

    /* For debugging purpose */
    #ifdef DEBUG_Route3
    {
        char clockStr[100];

        if ((node->getNodeTime() < DOWN_TIME) && (node->nodeId == 4))
        {
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld is down. Current time = %s\n",
                             node->nodeId, clockStr);
            return;
        }
    }
    #endif

    /*
     * Send current node's entire route table, using multiple
     * packet if necessary
     */

    for (i = 0; i < node->numberInterfaces; i++)
    {
        rowIndex = 0;

        /* Skip those interfaces that are not DVMRP enabled */
        if (!RoutingDvmrpIsDvmrpEnabledInterface(node, i))
        {
            continue;
        }

        /* A router will not send route reports on an interface until it knows
         * (by means of received probes) that it has a neighboring multicast
         * router on that interface.
         */
        if (dvmrp->interface[i].neighborCount == 0)
        {
            continue;
        }
        else
        {
            ListItem *listItem;
            RoutingDvmrpNeighborListItem *nbrInfo;

            listItem = dvmrp->interface[i].neighborList->first;

            while (listItem)
            {
                nbrInfo = (RoutingDvmrpNeighborListItem *)listItem->data;

                if (nbrInfo->relation == ROUTING_DVMRP_TWO_WAY_NEIGHBOR)
                {
                    break;
                }
                listItem = listItem->next;
            }

            if (listItem == NULL)
            {
                continue;
            }
        }

        while (rowIndex < (signed)dvmrp->routingTable.numEntries)
        {
            BOOL isPacket;

            /* Allocate message for packet */
            routeMsg = MESSAGE_Alloc(node, NETWORK_LAYER,
                                      MULTICAST_PROTOCOL_DVMRP,
                                      MSG_ROUTING_DvmrpPacket);

            /* Create route report packet */
            isPacket = RoutingDvmrpCreateRouteReportPacket(node,
                                                           &rowIndex,
                                                           routeMsg,
                                                           route,
                                                           i);
            if (isPacket)
            {

                /* Used to jitter Route report packet broadcast */
                //delay = RANDOM_erand(dvmrp->seed) * ROUTING_DVMRP_BROADCAST_JITTER;
                delay = 0;

                /* Now send packet out through this interface */
                NetworkIpSendRawMessageToMacLayerWithDelay(
                                          node,
                                          MESSAGE_Duplicate(node, routeMsg),
                                          NetworkIpGetInterfaceAddress(node, i),
                                          ALL_DVMRP_ROUTER,
                                          IPTOS_PREC_INTERNETCONTROL,
                                          IPPROTO_IGMP,
                                          1,
                                          i,
                                          ANY_DEST,
                                          delay);

                dvmrp->stats.totalPacketSent++;
                dvmrp->stats.routingUpdateSent++;

                if (route != ANY_IP)
                {
                    dvmrp->stats.numTriggerUpdate++;
                }
            }

            MESSAGE_Free(node, routeMsg);
        }//end while
    }//end for
}


/* FUNCTION     :RoutingDvmrpCreateRouteReportPacket()
 * PURPOSE      :Create DVMRP Route report packet
 * ASSUMPTION   :None
 * RETURN VALUE :BOOL
 */
static BOOL RoutingDvmrpCreateRouteReportPacket(Node *node, int *rowIndex,
        Message *routeMsg, NodeAddress route, int interfaceIndex)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);

    RoutingDvmrpRouteReportPacketType *routePacket;
    int maxPayloadSize =
                      DVMRP_MAX_PACKET_LENGTH - DVMRP_FIXED_LENGTH_HEADER_SIZE;

    unsigned char *payloadBuff;
    int payloadRowIndex = 0;
    int payloadSize = 0;

    payloadBuff = (unsigned char *) MEM_malloc(maxPayloadSize);

    /*
     * Send current node's entire route table, using multiple
     * packet if necessary
     */

    while ((*rowIndex < (signed)dvmrp->routingTable.numEntries) &&
               (payloadSize < maxPayloadSize))
    {
        /* Check if the route has been expired */
        if (dvmrp->routingTable.rowPtr[*rowIndex].state == ROUTING_DVMRP_EXPIRE)
        {
            (*rowIndex)++;

            /* Skip to next route */
            continue;
        }

        if ((route == ANY_IP) ||
                (dvmrp->routingTable.rowPtr[*rowIndex].destAddr == route))
        {
            NodeAddress destAddr;
            NodeAddress netmask;
            unsigned char metric;
            unsigned char addr1, addr2, addr3, addr4;

            netmask = dvmrp->routingTable.rowPtr[*rowIndex].subnetMask;
            destAddr = dvmrp->routingTable.rowPtr[*rowIndex].destAddr;

            /* Check if the route is in hold down state */
            if (dvmrp->routingTable.rowPtr[*rowIndex].state ==
                    ROUTING_DVMRP_HOLD_DOWN)
            {
                metric = ROUTING_DVMRP_INFINITY;
            }
            else
            {
                /* Split horizon process */
                if (interfaceIndex ==
                    dvmrp->routingTable.rowPtr[*rowIndex].interfaceId)
                {
                    metric = (unsigned char) (dvmrp->routingTable.rowPtr[*rowIndex].metric +
                        ROUTING_DVMRP_INFINITY);
                }
                else
                {
                        metric = dvmrp->routingTable.rowPtr[*rowIndex].metric;
                }
            }

            addr1 = (unsigned char) (netmask >> 16);
            addr2 = (unsigned char) (netmask >> 8);
            addr3 = (unsigned char) netmask;

            payloadBuff[payloadRowIndex++] = addr1;
            payloadBuff[payloadRowIndex++] = addr2;
            payloadBuff[payloadRowIndex++] = addr3;

            /* Check for number of non zero byte in mask */
            if ((netmask % 0x1000000) == 0)
            {
                /* Number of non zero byte = 1 */
                addr1 = (unsigned char) (destAddr >> 24);
                payloadBuff[payloadRowIndex++] = addr1;

                payloadBuff[payloadRowIndex++] = metric;

                payloadSize += 5;
            }
            else if ((netmask % 0x10000) == 0)
            {

                /* Number of non zero byte in mask = 2 */
                addr1 = (unsigned char) (destAddr >> 24);
                addr2 = (unsigned char) (destAddr >> 16);
                payloadBuff[payloadRowIndex++] = addr1;
                payloadBuff[payloadRowIndex++] = addr2;

                payloadBuff[payloadRowIndex++] = metric;

                payloadSize += 6;
            }
            else if ((netmask %0x100) == 0)
            {

                /* Number of non zero byte in mask = 3 */
                addr1 = (unsigned char) (destAddr >> 24);
                addr2 = (unsigned char) (destAddr >> 16);
                addr3 = (unsigned char) (destAddr >> 8);
                payloadBuff[payloadRowIndex++] = addr1;
                payloadBuff[payloadRowIndex++] = addr2;
                payloadBuff[payloadRowIndex++] = addr3;

                payloadBuff[payloadRowIndex++] = metric;

                payloadSize += 7;
            }
            else
            {

                /* Number of non zero byte in mask = 4 */
                addr1 = (unsigned char) (destAddr >> 24);
                addr2 = (unsigned char) (destAddr >> 16);
                addr3 = (unsigned char) (destAddr >> 8);
                addr4 = (unsigned char) destAddr;
                payloadBuff[payloadRowIndex++] = addr1;
                payloadBuff[payloadRowIndex++] = addr2;
                payloadBuff[payloadRowIndex++] = addr3;
                payloadBuff[payloadRowIndex++] = addr4;

                payloadBuff[payloadRowIndex++] = metric;

                payloadSize += 8;
            }

            if (route != ANY_IP)
            {

                /* Reset flag to enable trigger update */
                dvmrp->routingTable.rowPtr[*rowIndex].
                        triggeredUpdateScheduledFlag = FALSE;

                (*rowIndex)++;
                break;
            }
        }
        (*rowIndex)++;
    }

    if (payloadSize > 0)
    {

        /* Start packet creation */
        MESSAGE_PacketAlloc(node,
                      routeMsg,
                      sizeof(RoutingDvmrpRouteReportPacketType) + payloadSize,
                      TRACE_DVMRP);

        routePacket = (RoutingDvmrpRouteReportPacketType *)
                      MESSAGE_ReturnPacket(routeMsg);

        /* Create common header */
        RoutingDvmrpCreateCommonHeader(&routePacket->commonHeader,
                               ROUTING_DVMRP_REPORT);

        memcpy(routePacket + 1, payloadBuff, payloadSize);

        MEM_free(payloadBuff);
        return (TRUE);
    }
    else
    {
        MEM_free(payloadBuff);
        return (FALSE);
    }
}


/* FUNCTION     :RoutingDvmrpHandleRouteReportPacket()
 * PURPOSE      :Process received Route Report packet
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpHandleRouteReportPacket(Node *node,
        RoutingDvmrpRouteReportPacketType *routePacket, NodeAddress srcAddr,
        int size, int interfaceIndex)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);


    RoutingDvmrpInterface *thisInterface = &dvmrp->interface[interfaceIndex];
    RoutingDvmrpNeighborListItem *tempNBInfo;
    int payloadSize = size - DVMRP_FIXED_LENGTH_HEADER_SIZE;
    unsigned char *payloadBuff;
    int payloadRowIndex = 0;

    /*
     * Check the neighbor list in appropriate interface to confirm the
     * neighbor is known. If a match is not found, ignore report.
     */
    RoutingDvmrpSearchNeighborList(thisInterface->neighborList, srcAddr,
                                   &tempNBInfo);

    if (!tempNBInfo)
    {
        #ifdef DEBUG
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld got route report from unknown neighbor "
                   "%ld on interface %d at time %s\n",
                    node->nodeId, srcAddr, interfaceIndex, clockStr);
        }
        #endif

        return;
    }

    /* Check for two-way neighbor relationship */
    if (tempNBInfo->relation != ROUTING_DVMRP_TWO_WAY_NEIGHBOR)
    {
        #ifdef DEBUG
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld got route report from neighbor %ld on "
                   "interface %d at time %s,\nwith whom a Two-Way relation "
                   "has not been established yet\n",
                    node->nodeId, srcAddr, interfaceIndex, clockStr);
        }
        #endif

        return;
    }

    payloadBuff = (unsigned char *) MEM_malloc(payloadSize);

    /* Get payload from route report packet */
    memcpy(payloadBuff, routePacket + 1, payloadSize);

    /* Get each route report from the packet and process it */
    while (payloadRowIndex < payloadSize)
    {
        RoutingDvmrpRoutingTableRow *rowPtr;
        NodeAddress netmask;
        NodeAddress destAddr;
        unsigned char metric;
        unsigned char adjustedMetric;
        unsigned char addr1, addr2, addr3, addr4;

        /* Get netmask */
        addr1 = 0xFF;
        addr2 = payloadBuff[payloadRowIndex++];
        addr3 = payloadBuff[payloadRowIndex++];
        addr4 = payloadBuff[payloadRowIndex++];

        netmask = (addr1 << 24) | (addr2 << 16) | (addr3 << 8) | (addr4);

        /* Get route */
        if ((netmask % 0x1000000) == 0)
        {

            /* Number of non zero byte = 1 */
            addr1 = payloadBuff[payloadRowIndex++];

            destAddr = (addr1 << 24);
        }
        else if ((netmask % 0x10000) == 0)
        {

            /* Number of non zero byte in mask = 2 */
            addr1 = payloadBuff[payloadRowIndex++];
            addr2 = payloadBuff[payloadRowIndex++];

            destAddr = (addr1 << 24) | (addr2 << 16);
        }
        else if ((netmask %0x100) == 0)
        {

            /* Number of non zero byte in mask = 3 */
            addr1 = payloadBuff[payloadRowIndex++];
            addr2 = payloadBuff[payloadRowIndex++];
            addr3 = payloadBuff[payloadRowIndex++];

            destAddr = (addr1 << 24) | (addr2 << 16) | (addr3 << 8);
        }
        else
        {

            /* Number of non zero byte in mask = 4 */
            addr1 = payloadBuff[payloadRowIndex++];
            addr2 = payloadBuff[payloadRowIndex++];
            addr3 = payloadBuff[payloadRowIndex++];
            addr4 = payloadBuff[payloadRowIndex++];

            destAddr = (addr1 << 24) | (addr2 << 16) | (addr3 << 8) | (addr4);
        }

        /* Get metric for this route */
        metric = payloadBuff[payloadRowIndex++];

        adjustedMetric = metric < ROUTING_DVMRP_INFINITY ? (unsigned char) (metric + 1) : metric;

        /* Try to get the route from routing table */
        RoutingDvmrpFindRoute(node, destAddr, &rowPtr);
        // Dynamic address change
        /* Check if the route is new or had been expired or the subnet mask
        is different from network mask. The handling is done at the node 
        which is receiving a route report packet. The subnet mask in route
        report might be different as along with address DHCP server also
        gives a subnet mask. This new subnet mask will be sent in updates
        sent. Thus at the receiving end the route should be considered new
        in case where a different subnet mask is received in update packet.*/
        if ((rowPtr == NULL) || (rowPtr->state == ROUTING_DVMRP_EXPIRE) ||
             rowPtr->subnetMask != netmask)
        {
            if (adjustedMetric <= ROUTING_DVMRP_INFINITY)
            {
                /* Add new route */

                #ifdef DEBUG
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                    fprintf(stdout, "Node %ld add new route to %u at time %s\n",
                            node->nodeId, destAddr, clockStr);
                }
                #endif

                RoutingDvmrpAddNewRoute(node, destAddr, netmask, srcAddr,
                                        adjustedMetric, interfaceIndex);

                /* Set route timeout alarm */
                RoutingDvmrpSetTimer(node, MSG_ROUTING_DvmrpRouteTimeoutAlarm,
                        (void *) &destAddr,
                        DVMRP_EXPIRATION_TIMEOUT);
            }

            /* Get next route, if any */
            continue;
        }

        /* If the route is in hold down state, make it valid */
        if ((rowPtr->state == ROUTING_DVMRP_HOLD_DOWN) &&
                (metric < ROUTING_DVMRP_INFINITY))
        {

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld take route %u out of hold-down ",
                                 node->nodeId, destAddr);
            }
            #endif

            /*
             * Take route out of hold down immediately if the route
             * is re-learned from the same router with the same metric
             */
            if ((rowPtr->nextHop == srcAddr) &&
                    (rowPtr->metric == adjustedMetric))
            {

                /* Schedule triggered update */
                RoutingDvmrpSetTimer(node,
                        MSG_ROUTING_DvmrpTriggeredUpdateAlarm,
                        (void *) &rowPtr->destAddr,
                        DVMRP_TRIGGERED_UPDATE_RATE);
            }

            /* Update route */
            RoutingDvmrpUpdateRoute(node, destAddr, netmask, srcAddr,
                    adjustedMetric, interfaceIndex);

            /* Skip to next route */
            continue;
        }

        /* Section 3.4.6.B.1 */
        if (metric < ROUTING_DVMRP_INFINITY)
        {

            /* If the neighbor was dependent, cancel dependency */
            if (RoutingDvmrpIsDependentNeighbor(rowPtr, srcAddr,
                        interfaceIndex))
            {

                #ifdef DEBUG
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                    fprintf(stdout, "Node %ld remove %u from dependent "
                                    "neighbor list at time %s\n",
                                     node->nodeId, srcAddr, clockStr);
                }
                #endif

                RoutingDvmrpRemoveDependency(node, rowPtr, srcAddr,
                        interfaceIndex);

                RoutingDvmrpUpdateFwdTableForChangedDependency(
                    node, srcAddr, interfaceIndex, -1);
            }

            if (((adjustedMetric > rowPtr->metric) &&
                     (srcAddr == rowPtr->nextHop)) ||
                (adjustedMetric < rowPtr->metric) ||
                ((adjustedMetric == rowPtr->metric) &&
                     (srcAddr <= rowPtr->nextHop)))
            {

                /* Update route */
                RoutingDvmrpUpdateRoute(node, destAddr, netmask, srcAddr,
                        adjustedMetric, interfaceIndex);
            }
            else if (interfaceIndex != rowPtr->interfaceId)
            {

                /* Get child info if this report has came from a downstream */
                RoutingDvmrpChildListItem *childInfo =
                    RoutingDvmrpGetChild(rowPtr->child, interfaceIndex);

                /* If required change designated forwarder on downstream */
                if (childInfo->designatedForwarder != srcAddr)
                {

                    /*
                     * Change designated forwarder if
                     * 1) received metric is better or
                     * 2) metric is same but sending node has lower ip
                     *    address than the existing designated forwarder
                     */
                    if ((childInfo->advertiseMetric > metric) ||
                            ((childInfo->advertiseMetric == metric) &&
                            (childInfo->designatedForwarder > srcAddr)))
                    {
                        if (childInfo->designatedForwarder == childInfo->ipAddr)
                        {
                            RoutingDvmrpUpdateFwdTableForDesignatedForwarder(
                                node, interfaceIndex, -1);
                        }

                        #ifdef DEBUG
                        {
                            char clockStr[100];
                            NodeAddress netAddr =
                                NetworkIpGetInterfaceNetworkAddress(
                                            node,
                                            interfaceIndex);
                            ctoa(node->getNodeTime(), clockStr);
                            fprintf(stdout, "Node %ld choose new Designated"
                                            " forwarder %u on network %u "
                                            "for route %u at time%s\n",
                                             node->nodeId, srcAddr, netAddr,
                                             destAddr, clockStr);
                        }
                        #endif

                        childInfo->advertiseMetric = metric;
                        childInfo->designatedForwarder = srcAddr;
                    }
                }
                else
                {
                    /* Check if this node can advertise a better metric */
                    if ((metric > rowPtr->metric) ||
                            ((metric == rowPtr->metric) &&
                            (childInfo->ipAddr < srcAddr)))
                    {

                        #ifdef DEBUG
                        {
                            char clockStr[100];
                            NodeAddress netAddr =
                                NetworkIpGetInterfaceNetworkAddress(
                                            node,
                                            interfaceIndex);
                            ctoa(node->getNodeTime(), clockStr);
                            fprintf(stdout, "Node %ld become new Designated"
                                            " forwarder on network %u for "
                                            "route %u at time%s\n",
                                             node->nodeId, netAddr,
                                             destAddr, clockStr);
                        }
                        #endif

                        childInfo->designatedForwarder =
                                childInfo->ipAddr;
                        childInfo->advertiseMetric = rowPtr->metric;

                        RoutingDvmrpUpdateFwdTableForDesignatedForwarder(
                            node, interfaceIndex, 1);
                    }
                    else
                    {
                        childInfo->designatedForwarder = srcAddr;
                        childInfo->advertiseMetric = metric;
                    }
                }
            }
        }
        else if ((srcAddr == rowPtr->nextHop) &&
                     ((metric >= ROUTING_DVMRP_INFINITY) &&
                      (metric < 2 * ROUTING_DVMRP_INFINITY)))
        {
            clocktype delay;
            int i;
            RoutingDvmrpForwardingTable *fwdTable = &dvmrp->forwardingTable;

            /* Make the route invalid */
            rowPtr->state = ROUTING_DVMRP_HOLD_DOWN;

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                fprintf(stdout, "Node %ld set hold down state for "
                                "route %u at time%s\n",
                                 node->nodeId, destAddr, clockStr);
            }
            #endif

            /* Update forwarding table */
            for (i = 0; i < (signed) fwdTable->numEntries; i++)
            {
                ListItem *srcItem = fwdTable->rowPtr[i].srcList->first;

                while (srcItem)
                {
                    RoutingDvmrpSourceList *srcInfo =
                        (RoutingDvmrpSourceList *) srcItem->data;

                    /*
                     * All forwarding cache entries based on this route
                     * should be flushed
                     */
                    if (srcInfo->srcAddr == rowPtr->destAddr)
                    {
                        ListItem *item = srcItem;

                        srcItem = srcItem->next;

                        RoutingDvmrpRemoveFromSourceList(node,
                                fwdTable->rowPtr[i].srcList, item);
                        continue;
                    }

                    srcItem = srcItem->next;
                }
            }

            /* Schedule triggered update */
            RoutingDvmrpSetTimer(node,
                    MSG_ROUTING_DvmrpTriggeredUpdateAlarm,
                    (void *) &rowPtr->destAddr,
                    DVMRP_TRIGGERED_UPDATE_RATE);

            /* Schedule garbage collection alarm */
            delay = DVMRP_GARBAGE_TIMEOUT - DVMRP_EXPIRATION_TIMEOUT;
            RoutingDvmrpSetTimer(node,
                    MSG_ROUTING_DvmrpGarbageCollectionAlarm,
                    (void *) &rowPtr->destAddr, delay);
        }
        else if (interfaceIndex != rowPtr->interfaceId)
        {
            NodeAddress dsgFwd = RoutingDvmrpGetDesignatedForwarder(rowPtr,
                                       interfaceIndex);
            BOOL isDependent = RoutingDvmrpIsDependentNeighbor(rowPtr, srcAddr,
                                       interfaceIndex);


            /*
             * If the neighbor was the designated forwarder on this
             * network, receiving router become new designated
             * forwarder
             */
            if (dsgFwd == srcAddr)
            {

                #ifdef DEBUG
                {
                    char clockStr[100];
                    NodeAddress netAddr = NetworkIpGetInterfaceNetworkAddress(
                                                node, interfaceIndex);
                    ctoa(node->getNodeTime(), clockStr);

                    fprintf(stdout, "Node %ld become new Designated forwarder "
                                    "on network %u for route %u at time %s\n",
                                     node->nodeId, netAddr, destAddr, clockStr);
                }
                #endif

                RoutingDvmrpSetDesignatedForwarder(rowPtr, interfaceIndex);

                RoutingDvmrpUpdateFwdTableForDesignatedForwarder(
                    node, interfaceIndex, 1);
            }

            /*
             * If the neighbor was dependent and received metric is infinity,
             * cancel dependency
             */
            if ((metric == ROUTING_DVMRP_INFINITY) && (isDependent))
            {

                #ifdef DEBUG
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                    fprintf(stdout, "Node %ld remove %u from "
                                    "dependent neighbor list at "
                                    "time %s\n",
                                     node->nodeId, srcAddr,
                                     clockStr);
                }
                #endif

                RoutingDvmrpRemoveDependency(node, rowPtr, srcAddr,
                        interfaceIndex);

                RoutingDvmrpUpdateFwdTableForChangedDependency(
                    node, srcAddr, interfaceIndex, -1);
            }
            else if (((ROUTING_DVMRP_INFINITY < metric) &&
                          (2 * ROUTING_DVMRP_INFINITY > metric)) &&
                     (!isDependent))

            {
                #ifdef DEBUG
                {
                    char clockStr[100];
                    NodeAddress netAddr = NetworkIpGetInterfaceNetworkAddress(
                                                node, interfaceIndex);
                    ctoa(node->getNodeTime(), clockStr);
                    fprintf(stdout, "Node %ld include node %u as dependent "
                                    "neighbor on network %u for route %u "
                                    "at time%s\n",
                                     node->nodeId, srcAddr, netAddr,
                                     destAddr, clockStr);
                }
                #endif

                RoutingDvmrpAddDependentNeighbor(node, rowPtr, srcAddr,
                        interfaceIndex);

                RoutingDvmrpUpdateFwdTableForChangedDependency(
                    node, srcAddr, interfaceIndex, 1);
            }
        }
        else
        {
            /* Ignore route */
        }
    }//end while

     MEM_free(payloadBuff);
}


/* FUNCTION     :RoutingDvmrpHandleRouteTimeOut()
 * PURPOSE      :Set timer
 * ASSUMPTION   :Router doesn't restarted during simulation
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpHandleRouteTimeOut(Node *node,
        RoutingDvmrpRoutingTableRow *rowPtr)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpForwardingTable *fwdTable = &dvmrp->forwardingTable;
    clocktype delay;
    int i;

    rowPtr->state = ROUTING_DVMRP_HOLD_DOWN;

    /* Set garbage collection alarm */
    delay = DVMRP_GARBAGE_TIMEOUT - DVMRP_EXPIRATION_TIMEOUT;
    RoutingDvmrpSetTimer(node, MSG_ROUTING_DvmrpGarbageCollectionAlarm,
            (void *) &rowPtr->destAddr, delay);

    /* Update forwarding table */
    for (i = 0; i < (signed)fwdTable->numEntries; i++)
    {
        ListItem *srcItem = fwdTable->rowPtr[i].srcList->first;

        while (srcItem)
        {
            RoutingDvmrpSourceList *srcInfo =
                (RoutingDvmrpSourceList *) srcItem->data;

            /*
             * All forwarding cache entries based on this route
             * should be flushed
             */
            if (srcInfo->srcAddr == rowPtr->destAddr)
            {
                ListItem *item = srcItem;

                srcItem = srcItem->next;

                RoutingDvmrpRemoveFromSourceList(node,
                        fwdTable->rowPtr[i].srcList, item);
                continue;
            }

            srcItem = srcItem->next;
        }
    }
}


/* FUNCTION     :RoutingDvmrpSetTimer()
 * PURPOSE      :Set timer
 * ASSUMPTION   :Router doesn't restarted during simulation
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpSetTimer(Node *node, int eventType, void *data,
        clocktype delay)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    Message *newMsg;

    newMsg = MESSAGE_Alloc(node, NETWORK_LAYER,
                            MULTICAST_PROTOCOL_DVMRP,
                            eventType);

    #ifdef DEBUG_ERROR
    {
        fprintf(stdout, "Node %ld set timer, Event Type = %d\n",
                        node->nodeId, eventType);
    }
    #endif

    switch (eventType)
    {
        case MSG_ROUTING_DvmrpNeighborTimeoutAlarm:
        {
            MESSAGE_InfoAlloc(node, newMsg, sizeof(RoutingDvmrpTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingDvmrpTimerInfo));

            break;
        }

        case MSG_ROUTING_DvmrpTriggeredUpdateAlarm:
        {
            RoutingDvmrpRoutingTable *routingTable = &dvmrp->routingTable;
            RoutingDvmrpRoutingTableRow *rowPtr;
            NodeAddress ipAddr;

            memcpy(&ipAddr, data, sizeof(NodeAddress));
            RoutingDvmrpFindRoute(node, ipAddr, &rowPtr);

            ERROR_Assert(rowPtr != NULL, "Attempt to set triggerUpdateTimer "
                                 "for an invalid route\n");

            /* Schedule a triggered update only if
             * a) A triggered update is not already scheduled for the node, or
             * b) Next periodic update is not very soon (> node->getNodeTime() + 5s)
             */
                if (rowPtr->triggeredUpdateScheduledFlag == FALSE
                    && routingTable->nextBroadcastTime
                       > node->getNodeTime() + DVMRP_TRIGGERED_UPDATE_RATE)
                {
                    MESSAGE_InfoAlloc(node, newMsg, sizeof(NodeAddress));
                    memcpy(MESSAGE_ReturnInfo(newMsg),
                           &ipAddr,
                           sizeof(NodeAddress));

                    rowPtr->triggeredUpdateScheduledFlag = TRUE;
                }
                else
                {
                    MESSAGE_Free(node, newMsg);
                    return;
                }

            break;
        }

        case MSG_ROUTING_DvmrpRouteTimeoutAlarm:
        {
            MESSAGE_InfoAlloc(node, newMsg, sizeof(NodeAddress));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(NodeAddress));

            break;
        }

        case MSG_ROUTING_DvmrpGarbageCollectionAlarm:
        {
            MESSAGE_InfoAlloc(node, newMsg, sizeof(NodeAddress));
            memcpy(MESSAGE_ReturnInfo(newMsg), data, sizeof(NodeAddress));

            break;
        }

        case MSG_ROUTING_DvmrpPruneTimeoutAlarm:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                               sizeof(RoutingDvmrpPruneTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingDvmrpPruneTimerInfo));

            break;
        }

        case MSG_ROUTING_DvmrpGraftRtmxtTimeOut:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                               sizeof(RoutingDvmrpGraftTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingDvmrpGraftTimerInfo));

            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Wrong event type within dvmrp.pc");
        }
    }

    /* Send self message */
    MESSAGE_Send(node, newMsg, delay);
}


/*----------------------------------------------------------------------------*/
/*      VARIOUS FUNCTIONS FOR HANDLING FORWARDING TABLE DATA STRUCTURE        */
/*----------------------------------------------------------------------------*/

/*
 * FUNCTION     :RoutingDvmrpInitDependentNeighborList()
 * PURPOSE      :Initalizes dependent neighbors data structure on this interface
 *               for this particular source group pair.
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpInitDependentNeighborList(Node *node, LinkedList **list,
        RoutingDvmrpChildListItem *child)
{
    ListItem *tempListItem = child->dependentNeighbor->first;
    RoutingDvmrpDependentNeighborListItem *tempNBItem;

    ListInit(node, list);

    while (tempListItem)
    {
        tempNBItem = (RoutingDvmrpDependentNeighborListItem *)
            MEM_malloc(sizeof(RoutingDvmrpDependentNeighborListItem));

        memcpy(&tempNBItem->addr, tempListItem->data, sizeof(NodeAddress));
        tempNBItem->pruneActive = FALSE;
        tempNBItem->lastPruneReceived = (clocktype) 0;
        tempNBItem->pruneLifetime = (clocktype) 0;

        /* Insert item to list */
        ListInsert(node, *list, node->getNodeTime(), (void *) tempNBItem);

        tempListItem = tempListItem->next;
    }
}


/*
 * FUNCTION     :RoutingDvmrpUpdateDependentNeighborList()
 * PURPOSE      :Update dependent neighbor list for this particular downstream
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpUpdateDependentNeighborList(Node *node, LinkedList *list,
        NodeAddress addr, RoutingDvmrpListActionType type)
{
    ListItem *tempNbItem = list->first;
    RoutingDvmrpDependentNeighborListItem *tempNbInfo;

    switch (type)
    {
        case ROUTING_DVMRP_INSERT_TO_LIST:
        {
            tempNbInfo = (RoutingDvmrpDependentNeighborListItem *)
                MEM_malloc(
                    sizeof(RoutingDvmrpDependentNeighborListItem));
            tempNbInfo->addr = addr;
            tempNbInfo->pruneActive = FALSE;
            tempNbInfo->lastPruneReceived = (clocktype) 0;
            tempNbInfo->pruneLifetime = (clocktype) 0;

            /* Add new neighbor to list */
            ListInsert(node, list, node->getNodeTime(), (void *) tempNbInfo);

            break;
        }

        case ROUTING_DVMRP_REMOVE_FROM_LIST:
        {
            if (list->size == 0)
            {
                return;
            }

            while (tempNbItem)
            {
                tempNbInfo = (RoutingDvmrpDependentNeighborListItem *)
                                 tempNbItem->data;

                if (tempNbInfo->addr == addr)
                {

                    /* Remove neighbor from list */
                    ListGet(node, list, tempNbItem, TRUE, FALSE);
                    break;
                }
                tempNbItem = tempNbItem->next;
            }

            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Invalid list operation");
        }
    }
}


/*
 * FUNCTION     :RoutingDvmrpUpdateDownstreamList()
 * PURPOSE      :Update downstream interface list for this particular
 *               source group pair.
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpUpdateDownstreamList(Node *node, LinkedList *list,
        NodeAddress srcAddr, int interface, RoutingDvmrpListActionType type)
{
    ListItem *tempListItem;
    RoutingDvmrpDownstreamListItem *tempItem;

    switch (type)
    {
        case ROUTING_DVMRP_INSERT_TO_LIST:
        {
            RoutingDvmrpRoutingTableRow *rowPtr;
            RoutingDvmrpChildListItem *childInfo;

            RoutingDvmrpFindRoute(node, srcAddr, &rowPtr);
            childInfo = RoutingDvmrpGetChild(rowPtr->child, interface);

            //Receiving interface does not match the downStream interface
            if (childInfo == NULL)
            {
                return;
            }
            tempItem = (RoutingDvmrpDownstreamListItem *)
                MEM_malloc(sizeof(RoutingDvmrpDownstreamListItem));
            tempItem->interfaceIndex = interface;
            tempItem->isPruned = FALSE;
            tempItem->whenPruned = (clocktype) 0;

            /* Initializes dependent neighbor list */
            RoutingDvmrpInitDependentNeighborList(node,
                                                  &tempItem->dependentNeighbor,
                                                  childInfo);

            /* Add item to list */
            ListInsert(node, list, node->getNodeTime(), (void *) tempItem);

            break;
        }
        case ROUTING_DVMRP_REMOVE_FROM_LIST:
        {
            ERROR_Assert(list->size != 0, "Attempt to remove from empty list");

            /* Search list for the item we have to remove */
            tempListItem = list->first;
            while (tempListItem)
            {
                tempItem = (RoutingDvmrpDownstreamListItem *)
                    tempListItem->data;
                if (tempItem->interfaceIndex == interface)
                {
                    /* Free dependent neighbor list */
                    ListFree(node, tempItem->dependentNeighbor, FALSE);

                    ListGet(node, list, tempListItem, TRUE, FALSE);
                    break;
                }
                tempListItem = tempListItem->next;
            }
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Invalid list operation");
        }
    }
}


/*
 * FUNCTION     :RoutingDvmrpAddToSourceList()
 * PURPOSE      :Add new source for this group
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpAddToSourceList(Node *node, LinkedList *srcList,
        NodeAddress srcAddr)
{
    RoutingDvmrpRoutingTableRow *rowPtr;
    RoutingDvmrpSourceList *tempListItem = (RoutingDvmrpSourceList *)
        MEM_malloc(sizeof(RoutingDvmrpSourceList));

    RoutingDvmrpFindRoute(node, srcAddr, &rowPtr);

    ERROR_Assert(rowPtr != NULL, "Unknown source\n");
    ERROR_Assert(srcList != NULL, "Attempt to add in non-initializes source"
                                  " list");

    tempListItem->srcAddr = srcAddr;
    tempListItem->upstream = rowPtr->interfaceId;
    tempListItem->prevHop = rowPtr->nextHop;
    tempListItem->graftRtmxtTimer = FALSE;
    tempListItem->pruneActive = FALSE;
    tempListItem->lastDownstreamPruned = (clocktype) 0;
    tempListItem->pruneLifetime = (clocktype) 0;

    /* Initialize downstream list */
    ListInit(node, &tempListItem->downstream);

    /* Insert to list */
    ListInsert(node, srcList, node->getNodeTime(), (void *) tempListItem);
}


/*
 * FUNCTION     :RoutingDvmrpRemoveFromSourceList()
 * PURPOSE      :Remove a source entry for this group
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpRemoveFromSourceList(Node *node, LinkedList *srcList,
        ListItem *srcItem)
{
    RoutingDvmrpSourceList *srcInfo = (RoutingDvmrpSourceList *) srcItem->data;
    ListItem *tempItem = srcInfo->downstream->first;

    while (tempItem)
    {
        RoutingDvmrpDownstreamListItem *tempInfo =
                (RoutingDvmrpDownstreamListItem *) tempItem->data;

        /* Free dependent neighbor list */
        ListFree(node, tempInfo->dependentNeighbor, FALSE);

        tempItem = tempItem->next;
    }

    /* Now free downstream list */
    ListFree(node, srcInfo->downstream, FALSE);

    /* Remove from source list */
    ListGet(node, srcList, srcItem, TRUE, FALSE);
}


/*
 * FUNCTION     :RoutingDvmrpGetSourceForThisGroup()
 * PURPOSE      :Get the desired source pointer for a specific group
 * ASSUMPTION   :None
 * RETURN VALUE :RoutingDvmrpSourceList*
 */
static RoutingDvmrpSourceList*
RoutingDvmrpGetSourceForThisGroup(Node *node, NodeAddress srcAddr,
        NodeAddress grpAddr)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpForwardingTable *fwdTable = &dvmrp->forwardingTable;
    int i;

    /* Search forwarding table for the source group pair */
    for (i = 0; i < (signed)fwdTable->numEntries; i++)
    {
        if (fwdTable->rowPtr[i].groupAddr == grpAddr)
        {
            ListItem *tempListItem = fwdTable->rowPtr[i].srcList->first;
            RoutingDvmrpSourceList *srcInfo;

            while (tempListItem)
            {
                srcInfo = (RoutingDvmrpSourceList *) tempListItem->data;
                if (srcInfo->srcAddr == srcAddr)
                {
                    return (srcInfo);
                }
                tempListItem = tempListItem->next;
            }
            break;
        }
    }
    return NULL;
}


/*
 * FUNCTION     :RoutingDvmrpAddEntryInForwardingCache()
 * PURPOSE      :Add new entry in forwarding cache
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpAddEntryInForwardingCache(Node *node,
        NodeAddress srcAddr, NodeAddress grpAddr, int upstream)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpForwardingTable *fwdTable = &dvmrp->forwardingTable;
    int i;

    /* Check if the table has been filled */
    if (fwdTable->numEntries == fwdTable->numRowsAllocated)
    {
        RoutingDvmrpForwardingTableRow *oldRow = fwdTable->rowPtr;

        /* Double forwarding table size */
        fwdTable->rowPtr = (RoutingDvmrpForwardingTableRow *)
            (2 * fwdTable->numRowsAllocated *
                sizeof(RoutingDvmrpForwardingTableRow));

        for (i = 0; i < (signed)fwdTable->numEntries; i++)
        {
            fwdTable->rowPtr[i] = oldRow[i];
        }
        MEM_free(oldRow);

        /* Update table size */
        fwdTable->numRowsAllocated *= 2;
    }

    /* Check if a entry for this group allready exist */
    for (i = 0; i < (signed)fwdTable->numEntries; i++)
    {
        if (fwdTable->rowPtr[i].groupAddr == grpAddr)
        {
            /* Add new source to list */
            RoutingDvmrpAddToSourceList(node, fwdTable->rowPtr[i].srcList,
                                        srcAddr);
            return;
        }
    }

    /* Insert new row */
    fwdTable->rowPtr[i].groupAddr = grpAddr;

    /* Initializes source list */
    ListInit(node, &fwdTable->rowPtr[i].srcList);
    RoutingDvmrpAddToSourceList(node, fwdTable->rowPtr[i].srcList,
                                srcAddr);

    fwdTable->numEntries++;
}


/*
 * FUNCTION     :RoutingDvmrpGetDownstreamListForThisSourceGroup()
 * PURPOSE      :Get downstream list pointer.
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpGetDownstreamListForThisSourceGroup(Node *node,
        NodeAddress srcAddr, NodeAddress grpAddr, LinkedList **downstreamList)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpForwardingTable *fwdTable = &dvmrp->forwardingTable;
    int i;

    /* Search forwarding table for the source group pair */
    for (i = 0; i < (signed)fwdTable->numEntries; i++)
    {
        if (fwdTable->rowPtr[i].groupAddr == grpAddr)
        {
            ListItem *tempListItem = fwdTable->rowPtr[i].srcList->first;
            RoutingDvmrpSourceList *srcInfo;

            while (tempListItem)
            {
                srcInfo = (RoutingDvmrpSourceList *) tempListItem->data;
                if (srcInfo->srcAddr == srcAddr)
                {
                    *downstreamList = srcInfo->downstream;
                    return;
                }
                tempListItem = tempListItem->next;
            }
            break;
        }
    }
    *downstreamList = NULL;
}


/*
 * FUNCTION     :RoutingDvmrpLookUpForwardingCache()
 * PURPOSE      :Forward packet using forwarding cache
 * ASSUMPTION   :None
 * RETURN VALUE :Return TRUE if matching entry found
 */
static BOOL RoutingDvmrpLookUpForwardingCache(Node *node, Message *msg,
        NodeAddress groupAddr, BOOL *packetWasRouted)
{

    LinkedList *downstreamList = NULL;
    RoutingDvmrpSourceList *srcPtr;
    IpHeaderType *ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

    *packetWasRouted = FALSE;

    /* Get source list pointer to this source group pair */
    srcPtr = RoutingDvmrpGetSourceForThisGroup(node, ipHeader->ip_src,
                                               groupAddr);

    if (srcPtr == NULL)
    {
        /* We don't have any entry for this source group pair */
        return FALSE;
    }

    /* Check if prune should be be still active */
    if ((srcPtr->pruneActive) &&
        ((node->getNodeTime() - srcPtr->lastDownstreamPruned) < srcPtr->pruneLifetime))
    {
        clocktype pruneTime = srcPtr->pruneLifetime -
                (node->getNodeTime() - srcPtr->lastDownstreamPruned);

        #ifdef DEBUG
        {
            fprintf(stdout, "Node %ld is still in pruned state for source %u "
                            "group %u\n",
                             node->nodeId, ipHeader->ip_src, groupAddr);
        }
        #endif

        /* Send prune to upstream */
        RoutingDvmrpSendPrune(node, ipHeader->ip_src, groupAddr, pruneTime);
    }
    else
    {
        ListItem *tempListItem;
        RoutingDvmrpDownstreamListItem *downstream;

        /* Get downstream interface list */
        RoutingDvmrpGetDownstreamListForThisSourceGroup(node,
                                                        ipHeader->ip_src,
                                                        groupAddr,
                                                        &downstreamList);
        tempListItem = downstreamList->first;

        while (tempListItem)
        {
            NodeAddress nextHop;
            downstream = (RoutingDvmrpDownstreamListItem *) tempListItem->data;

            /* Check if this interface is pruned */
            if (downstream->isPruned)
            {
                /* Skip this interface */
                tempListItem = tempListItem->next;
                continue;
            }

            #ifdef DEBUG
            {
                char clockStr[100];
                NodeAddress netAddr;

                ctoa(node->getNodeTime(), clockStr);
                netAddr = NetworkIpGetInterfaceNetworkAddress(node,
                                             downstream->interfaceIndex);

                fprintf(stdout, "Node %ld forward multicast packet to network "
                                "%u at time %s using forwarding cache\n",
                                node->nodeId, netAddr, clockStr);
            }
            #endif

            nextHop = NetworkIpGetInterfaceBroadcastAddress(node,
                                              downstream->interfaceIndex);

            NetworkIpSendPacketToMacLayer(node,
                                          MESSAGE_Duplicate(node, msg),
                                          downstream->interfaceIndex,
                                          nextHop);

            *packetWasRouted = TRUE;
            tempListItem = tempListItem->next;
        }

        // TBD: Currently we are assuming that a router will not join a group.
        //      We need to change IGMP in order to perform this function
//        if ((*packetWasRouted == FALSE)
//            && (!NetworkIpIsPartOfMulticastGroup(node, groupAddr)))
//        {
//            srcPtr->pruneActive = TRUE;

            /* Stop any pending grafts awaiting acknowledgement */
//            srcPtr->graftRtmxtTimer = FALSE;

            /* Determine prune lifetime */
//            RoutingDvmrpSetPruneLifetime(srcPtr);

            /* Send prune to upstream */
//            RoutingDvmrpSendPrune(node, ipHeader->ip_src, groupAddr,
//                                  srcPtr->pruneLifetime);
//        }
    }

    return TRUE;
}




/*
 * FUNCTION     :RoutingDvmrpRouterFunction()
 * PURPOSE      :Handle multicast forwarding of packet
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
void RoutingDvmrpRouterFunction(Node *node,
                                Message *msg,
                                NodeAddress grpAddr,
                                int interfaceIndex,
                                BOOL *packetWasRouted,
                                NodeAddress prevHop)
{
    DvmrpData* dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    RoutingDvmrpRoutingTableRow *rowPtr;
    IpHeaderType *ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
    BOOL retVal;
    int incomingInterface;
    int i;

    *packetWasRouted = FALSE;

    /* If packet is from transport layer, send out every interface */
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ipHeader->ip_src == dvmrp->interface[i].ipAddress)
        {
            // RFC 1112: Only one interface is used for initial transmission

            int j;

            for (j = 0; j < node->numberInterfaces; j++)
            {
                NodeAddress nextHop;

                #ifdef DEBUG
                {
                    char clockStr[100];
                    NodeAddress netAddr;

                    ctoa(node->getNodeTime(), clockStr);
                    netAddr = NetworkIpGetInterfaceNetworkAddress(node, j);

                    fprintf(stdout, "Node %ld sending multicast packet to "
                                    "network %u at time %s\n",
                                    node->nodeId, netAddr, clockStr);
                }
                #endif

                nextHop = NetworkIpGetInterfaceBroadcastAddress(node, j);

                NetworkIpSendPacketToMacLayer(
                                node,
                                MESSAGE_Duplicate(node, msg),
                                j,
                                nextHop);

                *packetWasRouted = TRUE;
            }

            dvmrp->stats.numOfDataPacketOriginated++;

            MESSAGE_Free(node, msg);

            return;
        }
    }

    /* Search routing table for a entry of the source */
    RoutingDvmrpFindRoute(node, ipHeader->ip_src, &rowPtr);

    /* Get incoming interface */
    incomingInterface = interfaceIndex;

    /* If there is no route back to source, discard packet scilently */
    if (!rowPtr)
    {

        #ifdef DEBUG
        {

            fprintf(stdout, "Node %ld discard multicast packet as it does not "
                            "have route back to source %u\n",
                            node->nodeId, ipHeader->ip_src);
        }
        #endif

        dvmrp->stats.numOfDataPacketDiscard++;

        return;
    }

    /* If TTL value is less than 2, discard packet scilently */
    if (ipHeader->ip_ttl < 2)
    {

        #ifdef DEBUG
        {

            fprintf(stdout, "Node %ld discard multicast packet as TTL of "
                            "packet is < 2\n",
                            node->nodeId);
        }
        #endif

        dvmrp->stats.numOfDataPacketDiscard++;
        return;
    }

    /* If packet didn't come from desired interface, discard it */
    if (rowPtr->interfaceId != incomingInterface)
    {
        #ifdef DEBUG
        {

            fprintf(stdout, "Node %ld discard multicast packet as it does not "
                            "came from the upstream interface %d\n",
                            node->nodeId, rowPtr->interfaceId);
        }
        #endif

        dvmrp->stats.numOfDataPacketDiscard++;
        return;
    }

    /* Try to forward with forwarding cache */
    retVal = RoutingDvmrpLookUpForwardingCache(node,
                                               msg,
                                               grpAddr,
                                               packetWasRouted);

    /* If forwarding cache does not contain the desired entry build one */
    if (!retVal)
    {
        LinkedList *downstreamList;
        ListItem *tempChildItem;
        BOOL anyDependent = FALSE;

        /* Built new entry in forwarding cache */
        RoutingDvmrpAddEntryInForwardingCache(node, ipHeader->ip_src,
                                              grpAddr, rowPtr->interfaceId);

        /* Get downstream list pointer */
        RoutingDvmrpGetDownstreamListForThisSourceGroup(node,
                                                        ipHeader->ip_src,
                                                        grpAddr,
                                                        &downstreamList);

        tempChildItem = rowPtr->child->first;

        /*
         * Check each child interface to determine whether the packet
         * should be forwarded over this interface or not
         */
        while (tempChildItem)
        {
            RoutingDvmrpChildListItem *childInfo =
                (RoutingDvmrpChildListItem *) tempChildItem->data;

            /* Check if this node is designated forwarder on this interface */
            if (childInfo->ipAddr == childInfo->designatedForwarder)
            {
                NodeAddress nextHop;

                /* If this is a leaf interface, check local group database */
                if (childInfo->isLeaf == TRUE)
                {
                    BOOL localMember = FALSE;

                    /*
                     * This is a leaf network for this source group pair,
                     * so search local group database to determine if there
                     * exist any host that seeks packet for this group
                     */
                    if (ip->isIgmpEnable == TRUE)
                    {
                        IgmpSearchLocalGroupDatabase(node,
                                                     grpAddr,
                                                     childInfo->interfaceIndex,
                                                     &localMember);
                    }
                    if (localMember)
                    {
                        anyDependent = TRUE;

                        /* Add this interface to downstream list */
                        RoutingDvmrpUpdateDownstreamList(
                                                  node,
                                                  downstreamList,
                                                  ipHeader->ip_src,
                                                  childInfo->interfaceIndex,
                                                  ROUTING_DVMRP_INSERT_TO_LIST);

                        #ifdef DEBUG
                        {
                            char clockStr[100];
                            NodeAddress netAddr;

                            ctoa(node->getNodeTime(), clockStr);
                            netAddr = NetworkIpGetInterfaceNetworkAddress(
                                              node, childInfo->interfaceIndex);

                            fprintf(stdout, "Node %ld forward multicast packet "
                                            "to network %u at time %s\n",
                                            node->nodeId, netAddr, clockStr);
                        }
                        #endif

                        /* Send packet out this interface */
                        nextHop = NetworkIpGetInterfaceBroadcastAddress(node,
                                                   childInfo->interfaceIndex);

                        NetworkIpSendPacketToMacLayer(
                                        node,
                                        MESSAGE_Duplicate(node, msg),
                                        childInfo->interfaceIndex,
                                        nextHop);

                        *packetWasRouted = TRUE;
                    }
                }
                else
                {
                    anyDependent = TRUE;
                    RoutingDvmrpUpdateDownstreamList(
                                              node,
                                              downstreamList,
                                              ipHeader->ip_src,
                                              childInfo->interfaceIndex,
                                              ROUTING_DVMRP_INSERT_TO_LIST);

                    #ifdef DEBUG
                    {
                        char clockStr[100];
                        NodeAddress netAddr;

                        ctoa(node->getNodeTime(), clockStr);
                        netAddr = NetworkIpGetInterfaceNetworkAddress(
                                          node, childInfo->interfaceIndex);

                        fprintf(stdout, "Node %ld forward multicast packet to "
                                        "network %u at time %s\n",
                                        node->nodeId, netAddr, clockStr);
                    }
                    #endif

                    /* Send packet out this interface */
                    nextHop = NetworkIpGetInterfaceBroadcastAddress(node,
                                               childInfo->interfaceIndex);

                    NetworkIpSendPacketToMacLayer(
                                    node,
                                    MESSAGE_Duplicate(node, msg),
                                    childInfo->interfaceIndex,
                                    nextHop);

                    *packetWasRouted = TRUE;
                }
            }

            /* Take next child interface */
            tempChildItem = tempChildItem->next;
        }

        if (!anyDependent)
        {
            RoutingDvmrpSourceList *srcPtr;

            /* Get source list pointer to this source group pair */
            srcPtr = RoutingDvmrpGetSourceForThisGroup(node,
                                                       ipHeader->ip_src,
                                                       grpAddr);

            srcPtr->lastDownstreamPruned = node->getNodeTime();

            // TBD: Currently, we are assuming that a router
            //      will not join a group.
            //      We need to change IGMP in order to perform this function
//            if (!NetworkIpIsPartOfMulticastGroup(node, grpAddr))
//            {
                srcPtr->pruneActive = TRUE;
                srcPtr->pruneLifetime = DVMRP_DEFAULT_PRUNE_LIFETIME;

                /* Send prune to upstream */
                RoutingDvmrpSendPrune(node, ipHeader->ip_src, grpAddr,
                                      DVMRP_DEFAULT_PRUNE_LIFETIME);
//            }
        }
    }

    if ((*packetWasRouted))
    {
        dvmrp->stats.numOfDataPacketForward++;

        MESSAGE_Free(node, msg);
    }
    else
    {
        dvmrp->stats.numOfDataPacketDiscard++;
    }

    #ifdef DEBUG
    {
        RoutingDvmrpPrintForwardingTable(node);
    }
    #endif
}


/*
 * FUNCTION     :RoutingDvmrpSendPrune()
 * PURPOSE      :Send prune message to upstream
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpSendPrune(Node *node, NodeAddress srcAddr,
        NodeAddress grpAddr, clocktype pruneLifetime)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    Message *pruneMsg;
    RoutingDvmrpPrunePacketType *prunePacket;
    RoutingDvmrpRoutingTableRow *rowPtr;

    /* For debugging purpose */
    #ifdef DEBUG_Route3
    {
        char clockStr[100];

        if ((node->getNodeTime() < DOWN_TIME) && (node->nodeId == 4))
        {
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld is down. Current time = %s\n",
                             node->nodeId, clockStr);
            return;
        }
    }
    #endif

    RoutingDvmrpFindRoute(node, srcAddr, &rowPtr);

    /* If this node directly connected to source net, don't send prune */
    if (rowPtr->metric == 0)
    {
        return;
    }

    /* Allocate new message space */
    pruneMsg = MESSAGE_Alloc(node,
                              NETWORK_LAYER,
                              MULTICAST_PROTOCOL_DVMRP,
                              MSG_ROUTING_DvmrpPacket);

    /* Allocate space for packet */
    MESSAGE_PacketAlloc(node,
                        pruneMsg,
                        sizeof(RoutingDvmrpPrunePacketType),
                        TRACE_DVMRP);

    prunePacket = (RoutingDvmrpPrunePacketType *)
        MESSAGE_ReturnPacket(pruneMsg);

    /* Create common header */
    RoutingDvmrpCreateCommonHeader(&prunePacket->commonHeader,
                                   ROUTING_DVMRP_PRUNE);

    /* Set group and source field */
    prunePacket->sourceAddr = srcAddr;
    prunePacket->groupAddr = grpAddr;

    prunePacket->pruneLifetime = (unsigned int) (pruneLifetime / SECOND);

    #ifdef DEBUG
    {
        char clockStr[100];

        ctoa(node->getNodeTime(), clockStr);

        fprintf(stdout, "Node %ld send Prune to node %u at time %s\n",
                         node->nodeId, rowPtr->nextHop, clockStr);
    }
    #endif

    /* Now send packet out to upstream */
    NetworkIpSendRawMessageToMacLayer(
                                  node,
                                  MESSAGE_Duplicate(node, pruneMsg),
                                  NetworkIpGetInterfaceAddress(node,
                                          rowPtr->interfaceId),
                                  rowPtr->nextHop,
                                  IPTOS_PREC_INTERNETCONTROL,
                                  IPPROTO_IGMP,
                                  1,
                                  rowPtr->interfaceId,
                                  rowPtr->nextHop);

    dvmrp->stats.pruneSent++;
    dvmrp->stats.totalPacketSent++;

    MESSAGE_Free(node, pruneMsg);
}


/*
 * FUNCTION     :RoutingDvmrpHandlePrunePacket()
 * PURPOSE      :Handle received prune packet
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpHandlePrunePacket(Node *node,
        RoutingDvmrpPrunePacketType *prunePacket, NodeAddress srcAddr, int interfaceIndex)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;


    RoutingDvmrpInterface *thisInterface = &dvmrp->interface[interfaceIndex];
    RoutingDvmrpNeighborListItem *tempNBInfo;
    RoutingDvmrpSourceList *srcPtr;
    LinkedList *downstreamList;
    ListItem *tempItem;
    RoutingDvmrpDownstreamListItem *downstreamInfo = NULL;
    RoutingDvmrpDependentNeighborListItem *dependentNbInfo;

    /*
     * Check the neighbor list in appropriate interface to confirm the
     * neighbor is known. If a match is not found, ignore prune.
     */
    RoutingDvmrpSearchNeighborList(thisInterface->neighborList, srcAddr,
                                   &tempNBInfo);

    if (!tempNBInfo)
    {
        #ifdef DEBUG
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld got route Prune from unknown neighbor "
                   "%ld on interface %d at time %s\n",
                    node->nodeId, srcAddr, interfaceIndex, clockStr);
        }
        #endif

        return;
    }

    /* Get source list pointer to this source group pair */
    srcPtr = RoutingDvmrpGetSourceForThisGroup(node, prunePacket->sourceAddr,
                                               prunePacket->groupAddr);

    if (srcPtr == NULL)
    {
        /* We don't have any entry for this source group pair */
        return;
    }

    /* Get downstream list pointer */
    RoutingDvmrpGetDownstreamListForThisSourceGroup(node,
                                                    prunePacket->sourceAddr,
                                                    prunePacket->groupAddr,
                                                    &downstreamList);

    tempItem = downstreamList->first;

    while (tempItem)
    {
        downstreamInfo = (RoutingDvmrpDownstreamListItem *) tempItem->data;

        if (downstreamInfo->interfaceIndex == interfaceIndex)
        {
            break;
        }

        tempItem = tempItem->next;
    }

    if (tempItem == NULL)
    {
        /* Message didn't come from a valid downstream dependent */
        return;
    }

    /* Get dependent neighbor list */
    tempItem = downstreamInfo->dependentNeighbor->first;

    while (tempItem)
    {
        dependentNbInfo = (RoutingDvmrpDependentNeighborListItem *)
                              tempItem->data;

        if (dependentNbInfo->addr == srcAddr)
        {
            RoutingDvmrpPruneTimerInfo timerInfo;

            dependentNbInfo->pruneActive = TRUE;
            dependentNbInfo->lastPruneReceived = node->getNodeTime();
            dependentNbInfo->pruneLifetime =
                prunePacket->pruneLifetime * SECOND;

            /* Set timer */
            timerInfo.srcAddr = prunePacket->sourceAddr;
            timerInfo.grpAddr = prunePacket->groupAddr;
            timerInfo.nbrAddr = dependentNbInfo->addr;

            RoutingDvmrpSetTimer(node,
                                 MSG_ROUTING_DvmrpPruneTimeoutAlarm,
                                 (void *) &timerInfo,
                                 prunePacket->pruneLifetime * SECOND);
            break;
        }

        tempItem = tempItem->next;
    }

    if (tempItem == NULL)
    {
        /* Message didn't come from a valid downstream dependent */
        return;
    }

    /* Check if all dependent neighbor over this interface have sent prune */
    tempItem = downstreamInfo->dependentNeighbor->first;

    while (tempItem)
    {
        dependentNbInfo = (RoutingDvmrpDependentNeighborListItem *)
                              tempItem->data;

        if (dependentNbInfo->pruneActive == FALSE)
        {
            break;
        }

        tempItem = tempItem->next;
    }

    if (tempItem == NULL)
    {
        BOOL localMember = FALSE;

        /* All downstream dependent have sent prune already. So prune this
         * interface if there is no local group member for this group
         */
        if (ip->isIgmpEnable == TRUE)
        {
            IgmpSearchLocalGroupDatabase(node,
                                                prunePacket->groupAddr,
                                                interfaceIndex,
                                                &localMember);
        }
        if (!localMember)
        {
            downstreamInfo->isPruned = TRUE;
            downstreamInfo->whenPruned = node->getNodeTime();

            /* Check if all downstream interfaces have been pruned */
            tempItem = downstreamList->first;

            while (tempItem)
            {
                downstreamInfo = (RoutingDvmrpDownstreamListItem *) tempItem->data;

                if (downstreamInfo->isPruned == FALSE)
                {
                    break;
                }

                tempItem = tempItem->next;
            }

            if (tempItem == NULL)
            {
                /* All downstream interfaces have been pruned */
                srcPtr->lastDownstreamPruned = node->getNodeTime();

//TBD: Currently we are assuming that a router will not join a group
//     We need to change IGMP in order to perform this function
//                if (!NetworkIpIsPartOfMulticastGroup(node,
//                         prunePacket->groupAddr))
//                {
                    srcPtr->pruneActive = TRUE;

                    /* Stop any pending grafts awaiting acknowledgement */
                    srcPtr->graftRtmxtTimer = FALSE;

                    /* Determine prune lifetime */
                    RoutingDvmrpSetPruneLifetime(srcPtr, node->getNodeTime());

/* FIXME: At this point we can send prune upstream. Below is reference from draft.
 *
 * When a forwarding cache is being used, there is a trade-off that should
 * be considered when deciding when Prune messages should be sent upstream.
 * In all cases, when a data packet arrives and the downstream interface
 * list is empty, a prune is sent upstream. However, when a forwarding
 * cache entry transistions to an empty downstream interface list it is
 * possible as an optimization to send a prune at this time as well.  This
 * prune will possibly stop unwanted traffic sooner at the expense of
 * sending extra prune traffic for sources that are no longer sending.
 */
//                    RoutingDvmrpSendPrune(node, prunePacket->sourceAddr,
//                        prunePacket->groupAddr, srcPtr->pruneLifetime);
//                }
            }
        }
    }

}


/*
 * FUNCTION     :RoutingDvmrpSetPruneLifetime()
 * PURPOSE      :Set prune lifetime to the minimum value of the remaining
 *               prune lifetime of the dependent downstrem.
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpSetPruneLifetime(RoutingDvmrpSourceList *srcPtr,
                                         clocktype theTime)
{
    ListItem *tempDownstreamItem;
    ListItem *tempNbItem;

    srcPtr->pruneLifetime = DVMRP_DEFAULT_PRUNE_LIFETIME;

    tempDownstreamItem = srcPtr->downstream->first;

    while (tempDownstreamItem)
    {
        tempNbItem = ((RoutingDvmrpDownstreamListItem *)
            tempDownstreamItem->data)->dependentNeighbor->first;

        while (tempNbItem)
        {
            RoutingDvmrpDependentNeighborListItem *tempNbInfo =
                (RoutingDvmrpDependentNeighborListItem *) tempNbItem->data;

            clocktype remainingPruneLifetime = tempNbInfo->pruneLifetime -
                    (theTime - tempNbInfo->lastPruneReceived);

            if (srcPtr->pruneLifetime > remainingPruneLifetime)
            {
                srcPtr->pruneLifetime = remainingPruneLifetime;
            }
            tempNbItem = tempNbItem->next;
        }
        tempDownstreamItem = tempDownstreamItem->next;
    }

    ERROR_Assert(srcPtr->pruneLifetime >= 0, "Invalid prune lifetime");
}


/*
 * FUNCTION     :RoutingDvmrpHandlePruneTimeoutAlarm()
 * PURPOSE      :To handle prune time out
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpHandlePruneTimeoutAlarm(Node *node,
        RoutingDvmrpPruneTimerInfo timerInfo)
{

    RoutingDvmrpSourceList *srcPtr;
    ListItem *tempDownstreamItem;
    ListItem *tempNbItem;
    RoutingDvmrpDownstreamListItem *downstreamInfo;

    /* Get source list pointer to this source group pair */
    srcPtr = RoutingDvmrpGetSourceForThisGroup(node, timerInfo.srcAddr,
                                               timerInfo.grpAddr);

    tempDownstreamItem = srcPtr->downstream->first;

    while (tempDownstreamItem)
    {
        downstreamInfo = (RoutingDvmrpDownstreamListItem *)
                tempDownstreamItem->data;
        tempNbItem = downstreamInfo->dependentNeighbor->first;

        while (tempNbItem)
        {
            RoutingDvmrpDependentNeighborListItem *tempNbInfo =
                (RoutingDvmrpDependentNeighborListItem *) tempNbItem->data;

            if (tempNbInfo->addr == timerInfo.nbrAddr)
            {
                tempNbInfo->pruneActive = FALSE;
                downstreamInfo->isPruned = FALSE;
                srcPtr->pruneActive = FALSE;
                break;
            }

            tempNbItem = tempNbItem->next;
        }

        if (tempNbItem)
        {
            break;
        }
        tempDownstreamItem = tempDownstreamItem->next;
    }
}


/*
 * FUNCTION     :RoutingDvmrpSendGraft()
 * PURPOSE      :Send graft to upstream
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpSendGraft(Node *node, NodeAddress srcAddr,
        NodeAddress grpAddr)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    Message *graftMsg;
    RoutingDvmrpGraftPacketType *graftPacket;
    RoutingDvmrpRoutingTableRow *rowPtr;

    /* For debugging purpose */
    #ifdef DEBUG_Route3
    {
        char clockStr[100];

        if ((node->getNodeTime() < DOWN_TIME) && (node->nodeId == 4))
        {
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld is down. Current time = %s\n",
                             node->nodeId, clockStr);
            return;
        }
    }
    #endif

    RoutingDvmrpFindRoute(node, srcAddr, &rowPtr);

    /* If this node directly connected to source net, don't send graft */
    if (rowPtr->metric == 0)
    {
        return;
    }

    /* Allocate new message space */
    graftMsg = MESSAGE_Alloc(node,
                              NETWORK_LAYER,
                              MULTICAST_PROTOCOL_DVMRP,
                              MSG_ROUTING_DvmrpPacket);

    /* Allocate space for packet */
    MESSAGE_PacketAlloc(node,
                        graftMsg,
                        sizeof(RoutingDvmrpGraftPacketType),
                        TRACE_DVMRP);

    graftPacket = (RoutingDvmrpGraftPacketType *)
        MESSAGE_ReturnPacket(graftMsg);

    /* Create common header */
    RoutingDvmrpCreateCommonHeader(&graftPacket->commonHeader,
                                   ROUTING_DVMRP_GRAFT);

    /* Set group and source field */
    graftPacket->sourceAddr = srcAddr;
    graftPacket->groupAddr = grpAddr;

    #ifdef DEBUG
    {
        char clockStr[100];

        ctoa(node->getNodeTime(), clockStr);

        fprintf(stdout, "Node %ld send Graft to node %u at time %s\n",
                         node->nodeId, rowPtr->nextHop, clockStr);
    }
    #endif

    /* Now send packet out to upstream */
    NetworkIpSendRawMessageToMacLayer(
                                  node,
                                  MESSAGE_Duplicate(node, graftMsg),
                                  NetworkIpGetInterfaceAddress(node,
                                          rowPtr->interfaceId),
                                  rowPtr->nextHop,
                                  IPTOS_PREC_INTERNETCONTROL,
                                  IPPROTO_IGMP,
                                  1,
                                  rowPtr->interfaceId,
                                  rowPtr->nextHop);

    dvmrp->stats.graftSent++;
    dvmrp->stats.totalPacketSent++;

    MESSAGE_Free(node, graftMsg);
}


/*
 * FUNCTION     :RoutingDvmrpHandleGraftPacket()
 * PURPOSE      :Handle received graft packet
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpHandleGraftPacket(Node *node,
        RoutingDvmrpGraftPacketType *graftPacket, NodeAddress srcAddr, int interfaceIndex)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);


    RoutingDvmrpInterface *thisInterface = &dvmrp->interface[interfaceIndex];
    RoutingDvmrpNeighborListItem *tempNBInfo;
    RoutingDvmrpSourceList *srcPtr;
    LinkedList *downstreamList;
    ListItem *tempItem;
    RoutingDvmrpDownstreamListItem *downstreamInfo = NULL;
    RoutingDvmrpDependentNeighborListItem *dependentNbInfo;

    /*
     * Check the neighbor list in appropriate interface to confirm the
     * neighbor is known. If a match is not found, ignore graft.
     */
    RoutingDvmrpSearchNeighborList(thisInterface->neighborList, srcAddr,
                                   &tempNBInfo);

    if (!tempNBInfo)
    {
        #ifdef DEBUG
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %ld got route Graft from unknown neighbor "
                   "%ld on interface %d at time %s\n",
                    node->nodeId, srcAddr, interfaceIndex, clockStr);
        }
        #endif

        return;
    }

    /* Get source list pointer to this source group pair */
    srcPtr = RoutingDvmrpGetSourceForThisGroup(node, graftPacket->sourceAddr,
                                               graftPacket->groupAddr);

    if (srcPtr == NULL)
    {
        /* We don't have any entry for this source group pair */
        return;
    }

    /* Get downstream list pointer */
    RoutingDvmrpGetDownstreamListForThisSourceGroup(node,
                                                    graftPacket->sourceAddr,
                                                    graftPacket->groupAddr,
                                                    &downstreamList);

    tempItem = downstreamList->first;

    while (tempItem)
    {
        downstreamInfo = (RoutingDvmrpDownstreamListItem *) tempItem->data;

        if (downstreamInfo->interfaceIndex == interfaceIndex)
        {
            break;
        }

        tempItem = tempItem->next;
    }

    if (tempItem == NULL)
    {
        /* Message didn't come from a valid downstream dependent */
        return;
    }

    /* Get dependent neighbor list */
    tempItem = downstreamInfo->dependentNeighbor->first;

    while (tempItem)
    {
        dependentNbInfo = (RoutingDvmrpDependentNeighborListItem *)
                              tempItem->data;

        if (dependentNbInfo->addr == srcAddr)
        {
            dependentNbInfo->pruneActive = FALSE;
            downstreamInfo->isPruned = FALSE;

            /* Send graft acknowledgement to the sender */
            RoutingDvmrpSendGraftAck(node, srcAddr, srcPtr->srcAddr,
                                     graftPacket->groupAddr);

            /* Send graft if a prune had been sent upstream */
            if (srcPtr->pruneActive)
            {
                RoutingDvmrpGraftTimerInfo timerInfo;

                srcPtr->pruneActive = FALSE;
                RoutingDvmrpSendGraft(node, srcPtr->srcAddr,
                                      graftPacket->groupAddr);

                /*
                 * Add the graft to the retransmission timer list
                 * awaiting an acknowledgment.
                 */
                srcPtr->graftRtmxtTimer = TRUE;

                timerInfo.count = 0;
                timerInfo.srcAddr = srcPtr->srcAddr;
                timerInfo.grpAddr = graftPacket->groupAddr;

                RoutingDvmrpSetTimer(node,
                                     MSG_ROUTING_DvmrpGraftRtmxtTimeOut,
                                     (void *) &timerInfo,
                                     DVMRP_GRAFT_RTMXT_INTERVAL);
            }

            break;
        }

        tempItem = tempItem->next;
    }
}


/*
 * FUNCTION     :RoutingDvmrpHandleGraftRtmxtTimeOut()
 * PURPOSE      :Retransmit graft if graft ack yet not received
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpHandleGraftRtmxtTimeOut(Node *node,
        RoutingDvmrpGraftTimerInfo timerInfo)
{
    RoutingDvmrpSourceList *srcPtr;
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);

    /* Get source list pointer to this source group pair */
    srcPtr = RoutingDvmrpGetSourceForThisGroup(node, timerInfo.srcAddr,
                                               timerInfo.grpAddr);

    if (srcPtr == NULL)
    {
        /* perhaps this neighbor has been expired */
        return;
    }

    if (srcPtr->graftRtmxtTimer)
    {
        clocktype delay;
        int count;

        #ifdef DEBUG
        {
            fprintf(stdout, "Node %ld didn't got Graft Ack for source = %u, "
                            "group = %u\n",
                            node->nodeId, timerInfo.srcAddr, timerInfo.grpAddr);
        }
        #endif

        /*
         * Use binary exponential back-off policy for subsequent retransmission
         */
        if (timerInfo.count <= DVMRP_MAX_GRAFT)
        {
            count = ++timerInfo.count;
            count = (int)pow(2.0, count) - 1;
            count = RANDOM_nrand(dvmrp->seed) % count;
            delay = count * DVMRP_GRAFT_RTMXT_INTERVAL;

            RoutingDvmrpSendGraft(node, srcPtr->srcAddr, timerInfo.grpAddr);

            RoutingDvmrpSetTimer(node,
                                 MSG_ROUTING_DvmrpGraftRtmxtTimeOut,
                                 (void *) &timerInfo,
                                 delay);
        }
    }
}


/*
 * FUNCTION     :RoutingDvmrpLocalMembersJoinOrLeave()
 * PURPOSE      :Handle local group membership events
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
void RoutingDvmrpLocalMembersJoinOrLeave(Node *node,
                                         NodeAddress groupAddr,
                                         int interfaceId,
                                         LocalGroupMembershipEventType event)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    RoutingDvmrpForwardingTable *fwdTable = &dvmrp->forwardingTable;
    ListItem *srcListItem = NULL;
    RoutingDvmrpSourceList *srcPtr;
    RoutingDvmrpRoutingTableRow *rowPtr;
    int i;

    /* Try to find if there is any active entry in forwarding table */
    for (i = 0; i < (signed)fwdTable->numEntries; i++)
    {
        if (fwdTable->rowPtr[i].groupAddr == groupAddr)
        {
            srcListItem = fwdTable->rowPtr[i].srcList->first;
            break;
        }
    }

    switch (event)
    {

        /* A local member joins a group */
        case LOCAL_MEMBER_JOIN_GROUP:
        {
            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);

                fprintf(stdout, "IGMP Notify Node %ld got new member %ld on "
                                "interface %d at time %s\n",
                                 node->nodeId, groupAddr,
                                 interfaceId, clockStr);
            }
            #endif

            while (srcListItem)
            {
                ListItem *dListItem;

                srcPtr = (RoutingDvmrpSourceList *) srcListItem->data;

                RoutingDvmrpFindRoute(node, srcPtr->srcAddr, &rowPtr);
                ERROR_Assert(rowPtr != NULL, "Unknown source have forwarding "
                                             "table entry\n");

                /*
                 * If the interface, over which the new member joins the group
                 * is not a child interface for this source, or if we are not
                 * the designated forwarder for this source on this network;
                 * then skip this source
                 */
                if ((!RoutingDvmrpIsChildInterface(rowPtr, interfaceId)) ||
                    (dvmrp->interface[interfaceId].ipAddress !=
                    RoutingDvmrpGetDesignatedForwarder(rowPtr, interfaceId)))
                {
                    srcListItem = srcListItem->next;
                    continue;
                }

                /* Verify that a Prune exist for this source group pair */
                dListItem = srcPtr->downstream->first;

                while (dListItem)
                {
                    RoutingDvmrpDownstreamListItem *dInfo =
                        (RoutingDvmrpDownstreamListItem *) dListItem->data;

                    if (dInfo->interfaceIndex == interfaceId)
                    {
                        dInfo->isPruned = FALSE;
                        break;
                    }
                    dListItem = dListItem->next;
                }

                if (srcPtr->pruneActive)
                {
                    RoutingDvmrpGraftTimerInfo timerInfo;

                    srcPtr->pruneActive = FALSE;

                    /* Send graft to upstream */
                    RoutingDvmrpSendGraft(node, srcPtr->srcAddr, groupAddr);

                    /*
                     * Add the graft to the retransmission timer list
                     * awaiting an acknowledgment.
                     */
                    srcPtr->graftRtmxtTimer = TRUE;

                    timerInfo.count = 0;
                    timerInfo.srcAddr = srcPtr->srcAddr;
                    timerInfo.grpAddr = groupAddr;

                    RoutingDvmrpSetTimer(node,
                                         MSG_ROUTING_DvmrpGraftRtmxtTimeOut,
                                         (void *) &timerInfo,
                                         DVMRP_GRAFT_RTMXT_INTERVAL);
                }

                srcListItem = srcListItem->next;
            }

            break;
        }

        case LOCAL_MEMBER_LEAVE_GROUP:
        {
            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);

                fprintf(stdout, "IGMP Notify Node %ld lost  member %ld on "
                                "interface %d at time %s\n",
                                 node->nodeId, groupAddr,
                                 interfaceId, clockStr);
            }
            #endif

            while (srcListItem)
            {
                ListItem *tempItem;
                RoutingDvmrpDownstreamListItem *downstreamInfo;
                RoutingDvmrpDependentNeighborListItem *dependentNbInfo;

                srcPtr = (RoutingDvmrpSourceList *) srcListItem->data;

                RoutingDvmrpFindRoute(node, srcPtr->srcAddr, &rowPtr);
                ERROR_Assert(rowPtr != NULL, "Unknown source have forwarding "
                                             "table entry\n");

                /*
                 * If the interface, over which the member leaves the group
                 * is not a child interface for this source, or if we are not
                 * the designated forwarder for this source on this network;
                 * then skip this source
                 */
                if ((!RoutingDvmrpIsChildInterface(rowPtr, interfaceId)) ||
                    (dvmrp->interface[interfaceId].ipAddress !=
                    RoutingDvmrpGetDesignatedForwarder(rowPtr, interfaceId)))
                {
                    srcListItem = srcListItem->next;
                    continue;
                }

                /*
                 * Check if all dependent neighbor over this interface
                 * have sent prune
                 */
                tempItem = srcPtr->downstream->first;

                while (tempItem)
                {
                    downstreamInfo = (RoutingDvmrpDownstreamListItem *)
                            tempItem->data;

                    if (downstreamInfo->interfaceIndex == interfaceId)
                    {
                        ListItem *dependentNbItem =
                            downstreamInfo->dependentNeighbor->first;

                        while (dependentNbItem)
                        {
                            dependentNbInfo =
                                (RoutingDvmrpDependentNeighborListItem *)
                                    dependentNbItem->data;

                            if (dependentNbInfo->pruneActive == FALSE)
                            {
                                break;
                            }

                            dependentNbItem = dependentNbItem->next;
                        }

                        if (dependentNbItem == NULL)
                        {
                            /*
                             * That means all dependent neighbor had been
                             * pruned, so we need to make this interface
                             * as pruned interface
                             */
                            downstreamInfo->isPruned = TRUE;
                            downstreamInfo->whenPruned = node->getNodeTime();
                        }

                        break;
                    }

                    tempItem = tempItem->next;
                }

                /* Check if all downstream interfaces have been pruned */
                tempItem = srcPtr->downstream->first;

                while (tempItem)
                {
                    downstreamInfo = (RoutingDvmrpDownstreamListItem *)
                            tempItem->data;

                    if (downstreamInfo->isPruned == FALSE)
                    {
                        break;
                    }

                    tempItem = tempItem->next;
                }

                if (tempItem == NULL)
                {
                    /* All downstream interfaces have been pruned */
                    srcPtr->lastDownstreamPruned = node->getNodeTime();

//TBD: Currently we are assuming that a router will not join a group
//     We need to change IGMP in order to perform this function
//                    if (!NetworkIpIsPartOfMulticastGroup(node, groupAddr))
//                    {
                        srcPtr->pruneActive = TRUE;

                        /* Stop any pending grafts awaiting acknowledgement */
                        srcPtr->graftRtmxtTimer = FALSE;

                        /* Determine prune lifetime */
                        RoutingDvmrpSetPruneLifetime(srcPtr, node->getNodeTime());

/* FIXME: At this point we can send prune upstream. Below is reference from draft.
 *
 * When a forwarding cache is being used, there is a trade-off that should
 * be considered when deciding when Prune messages should be sent upstream.
 * In all cases, when a data packet arrives and the downstream interface
 * list is empty, a prune is sent upstream. However, when a forwarding
 * cache entry transistions to an empty downstream interface list it is
 * possible as an optimization to send a prune at this time as well.  This
 * prune will possibly stop unwanted traffic sooner at the expense of
 * sending extra prune traffic for sources that are no longer sending.
 */
//                        RoutingDvmrpSendPrune(node, prunePacket->sourceAddr,
//                            prunePacket->groupAddr, srcPtr->pruneLifetime);
//                    }
                }

                srcListItem = srcListItem->next;
            }
            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Unknown Igmp operation");
        }
    }
}


/*
 * FUNCTION     :RoutingDvmrpSendGraftAck()
 * PURPOSE      :Send graft ack to dependent neighbor
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpSendGraftAck(Node *node, NodeAddress destAddr,
        NodeAddress srcAddr, NodeAddress grpAddr)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);
    Message *graftAckMsg;
    RoutingDvmrpGraftAckPacketType *graftAckPacket;
    RoutingDvmrpRoutingTableRow *rowPtr;

    RoutingDvmrpFindRoute(node, destAddr, &rowPtr);

    /* Allocate new message space */
    graftAckMsg = MESSAGE_Alloc(node,
                                 NETWORK_LAYER,
                                 MULTICAST_PROTOCOL_DVMRP,
                                 MSG_ROUTING_DvmrpPacket);

    /* Allocate space for packet */
    MESSAGE_PacketAlloc(node,
                        graftAckMsg,
                        sizeof(RoutingDvmrpGraftAckPacketType),
                        TRACE_DVMRP);

    graftAckPacket = (RoutingDvmrpGraftAckPacketType *)
        MESSAGE_ReturnPacket(graftAckMsg);

    /* Create common header */
    RoutingDvmrpCreateCommonHeader(&graftAckPacket->commonHeader,
                                   ROUTING_DVMRP_GRAFT_ACK);

    /* Set group and source field */
    graftAckPacket->sourceAddr = srcAddr;
    graftAckPacket->groupAddr = grpAddr;

    #ifdef DEBUG
    {
        char clockStr[100];

        ctoa(node->getNodeTime(), clockStr);

        fprintf(stdout, "Node %ld send Graft Ack to node %u at time %s\n",
                         node->nodeId, destAddr, clockStr);
    }
    #endif

    /* Now send packet out to downstream neighbor */
    NetworkIpSendRawMessageToMacLayer(
                                  node,
                                  MESSAGE_Duplicate(node, graftAckMsg),
                                  NetworkIpGetInterfaceAddress(node,
                                          rowPtr->interfaceId),
                                  destAddr,
                                  IPTOS_PREC_INTERNETCONTROL,
                                  IPPROTO_IGMP,
                                  1,
                                  rowPtr->interfaceId,
                                  destAddr);

    dvmrp->stats.graftAckSent++;
    dvmrp->stats.totalPacketSent++;

    MESSAGE_Free(node, graftAckMsg);
}


/*
 * FUNCTION     :RoutingDvmrpHandleGraftAckPacket()
 * PURPOSE      :Handle Graft Ack packet
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpHandleGraftAckPacket(Node *node,
        RoutingDvmrpGraftAckPacketType *graftAckPacket, NodeAddress srcAddr)
{
    RoutingDvmrpSourceList *srcPtr;
    RoutingDvmrpRoutingTableRow *rowPtr;

    /* Get source list pointer to this source group pair */
    srcPtr = RoutingDvmrpGetSourceForThisGroup(node,
                                               graftAckPacket->sourceAddr,
                                               graftAckPacket->groupAddr);

    if (srcPtr == NULL)
    {
        /* We don't have any entry for this source group pair */
        return;
    }

    RoutingDvmrpFindRoute(node, graftAckPacket->sourceAddr, &rowPtr);

    if ((srcPtr->graftRtmxtTimer) && (rowPtr->nextHop == srcAddr))
    {
        /* Stop retransmission timer */
        srcPtr->graftRtmxtTimer = FALSE;
    }
}


/*
 * FUNCTION     RoutingDvmrpFinalize
 * PURPOSE      Called at the end of simulation to collect the results of
 *              the simulation of DVMRP protocol of the NETWORK layer.
 *
 * Parameter:
 *     node:    node for which results are to be collected
 */
void RoutingDvmrpFinalize(Node *node)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);

    char buf[MAX_STRING_LENGTH];

    // Only print statistics once per node.
    if (dvmrp->stats.alreadyPrinted == TRUE)
    {
        return;
    }

    dvmrp->stats.alreadyPrinted = TRUE;

    #ifdef DEBUG_TABLE
    {
        RoutingDvmrpPrintRoutingTable(node);
        RoutingDvmrpPrintForwardingTable(node);
    }
    #endif

    sprintf(buf, "Packets Sent = %d",
                 dvmrp->stats.totalPacketSent);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Packets Received = %d",
                 dvmrp->stats.totalPacketReceive);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Probe Packets Sent = %d",
                 dvmrp->stats.probePacketSent);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Probe Packets Received = %d",
                 dvmrp->stats.probePacketReceive);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Neighbors = %d",
                 dvmrp->stats.numNeighbor);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Routing Updates Sent = %d",
                 dvmrp->stats.routingUpdateSent);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Routing Updates Received = %d",
                 dvmrp->stats.routingUpdateReceive);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Triggered Updates Sent = %d",
                 dvmrp->stats.numTriggerUpdate);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Prunes Sent = %d",
                 dvmrp->stats.pruneSent);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Prunes Received = %d",
                 dvmrp->stats.pruneReceive);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Grafts Sent = %d",
                 dvmrp->stats.graftSent);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Grafts Received = %d",
                 dvmrp->stats.graftReceive);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Graft ACKs Sent = %d",
                 dvmrp->stats.graftAckSent);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Graft ACKs Received = %d",
                 dvmrp->stats.graftAckReceive);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Multicast Packets Sent as Data Source = %d",
                 dvmrp->stats.numOfDataPacketOriginated);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Multicast Packets Forwarded = %d",
                 dvmrp->stats.numOfDataPacketForward);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Multicast Packets Discarded = %d",
                 dvmrp->stats.numOfDataPacketDiscard);
    IO_PrintStat(
        node,
        "Network",
        "DVMRP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);
} /* DvmrpFinalize */


/*
 * FUNCTION     :RoutingDvmrpIsDvmrpEnabledInterface()
 * PURPOSE      :Determine whether a specific interface is dvmrp enabled or not
 * ASSUMPTION   :None
 * RETURN VALUE :BOOL
 */
static BOOL RoutingDvmrpIsDvmrpEnabledInterface(Node *node,
        int interfaceId)
{
    NetworkDataIp *ipLayer = (NetworkDataIp *) node->networkData.networkVar;

    if ((ipLayer->interfaceInfo[interfaceId]->multicastEnabled == FALSE) ||
        (ipLayer->interfaceInfo[interfaceId]->multicastProtocolType !=
             MULTICAST_PROTOCOL_DVMRP))
    {
        return (FALSE);
    }

    return (TRUE);
}


/*
 * FUNCTION     :RoutingDvmrpPrintRoutingTable()
 * PURPOSE      :Print routing table of a node
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpPrintRoutingTable(Node *node)
{

    int i;
    clocktype timestamp;
    char destAddr[100];
    char subnetMask[100];
    char nextHop[100];
    char clockStr[MAX_CLOCK_STRING_LENGTH];
    FILE *fp;

    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);

    fp = fopen("RTTable.txt", "a+");

    if (fp == NULL)
    {
        fprintf(stdout, "Couldn't open file RTTable.txt\n");
        return;
    }

    timestamp = node->getNodeTime() / SECOND;
    ctoa(timestamp, clockStr);

    fprintf(fp, "\n        Routing table for node %d at time %s\n",
                 node->nodeId, clockStr);
    fprintf(fp, "        ------------------------\n");
    fprintf(fp, "     destination          mask                  nextHop   metric   "
                " upstream\n");

    for (i = 0; i < (signed)dvmrp->routingTable.numEntries; i++)
    {
        IO_ConvertIpAddressToString(dvmrp->routingTable.rowPtr[i].destAddr,
                                 destAddr);
        IO_ConvertIpAddressToString(dvmrp->routingTable.rowPtr[i].subnetMask,
                                 subnetMask);
        IO_ConvertIpAddressToString(dvmrp->routingTable.rowPtr[i].nextHop,
                                 nextHop);

        fprintf(fp, "%15s     %15s     %15s     %2d         %d\n",
                     destAddr,
                     subnetMask,
                     nextHop,
                     dvmrp->routingTable.rowPtr[i].metric,
                     dvmrp->routingTable.rowPtr[i].interfaceId);
    }

    fprintf(fp, "\n\n");

    fclose(fp);
}


/*
 * FUNCTION     :RoutingDvmrpPrintForwardingTable()
 * PURPOSE      :Print forwarding table of a node
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
static void RoutingDvmrpPrintForwardingTable(Node *node)
{
    DvmrpData *dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);

    FILE *fp;
    int i;
    char groupAddr[100];
    char srcAddr[100];
    char upstream[100];
    char downstream[100];
    char dependent[100];
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

    fprintf(fp, "        Forwarding Table for Node %d at time = %s\n",
                 node->nodeId, clockStr);
    fprintf(fp, "------------------------------------------------------------");
    fprintf(fp, "\n   Source       Group       Upstream     "
                "State    Downstream Intf    Dependent Nbr\n");

    for (i = 0; i < (signed)dvmrp->forwardingTable.numEntries; i++)
    {
        RoutingDvmrpForwardingTableRow *rowPtr;
        ListItem *srcListItem;

        rowPtr = &dvmrp->forwardingTable.rowPtr[i];
        srcListItem = rowPtr->srcList->first;
        IO_ConvertIpAddressToString(rowPtr->groupAddr, groupAddr);

        while (srcListItem)
        {
            RoutingDvmrpSourceList *srcInfo;
            ListItem *downstreamListItem;

            srcInfo = (RoutingDvmrpSourceList *) srcListItem->data;
            IO_ConvertIpAddressToString(srcInfo->srcAddr, srcAddr);
            IO_ConvertIpAddressToString(srcInfo->prevHop, upstream);

            fprintf(fp, "%10s    %10s  %10s    ",
                         srcAddr, groupAddr, upstream);

            if (srcInfo->pruneActive)
            {
                fprintf(fp, "Pruned           *\n\n");
                srcListItem = srcListItem->next;
                continue;
            }
            fprintf(fp, " Active    ");

            downstreamListItem = srcInfo->downstream->first;

            while (downstreamListItem)
            {
                RoutingDvmrpDownstreamListItem *downstreamInfo;
                ListItem *tempNbList;
                NodeAddress addr;

                downstreamInfo = (RoutingDvmrpDownstreamListItem *)
                                         downstreamListItem->data;

                if (downstreamInfo->isPruned == TRUE)
                {
                    downstreamListItem = downstreamListItem->next;
                    continue;
                }

                addr = NetworkIpGetInterfaceAddress(node,
                               downstreamInfo->interfaceIndex);
                IO_ConvertIpAddressToString(addr, downstream);
                fprintf(fp, "%10s    ", downstream);

                tempNbList = downstreamInfo->dependentNeighbor->first;

                while (tempNbList)
                {
                    RoutingDvmrpDependentNeighborListItem *tempNbInfo =
                            (RoutingDvmrpDependentNeighborListItem*)
                                    tempNbList->data;

                    if (tempNbInfo->pruneActive)
                    {
                        tempNbList = tempNbList->next;
                        continue;
                    }

                    IO_ConvertIpAddressToString(tempNbInfo->addr, dependent);
                    fprintf(fp, "   %10s\n                                     "
                                "                                ",
                                dependent);
                    tempNbList = tempNbList->next;
                }
                downstreamListItem = downstreamListItem->next;
                fprintf(fp, "\n                                        "
                            "           ");
            }
            fprintf(fp, "\n\n");
            srcListItem = srcListItem->next;
        }
    }

    fprintf(fp, "############################################################");
    fprintf(fp, "\n\n");

    fclose(fp);
}
// Dynamic address change
//---------------------------------------------------------------------------
// FUNCTION   :: RoutingDvmrpHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address for DVMRP
// PARAMETERS ::
// + node               : Node* : Pointer to Node structure
// + interfaceIndex     : Int32 : interface index
// + oldAddress         : Address* : current address
// + subnetMask         : NodeAddress : subnetMask
// + networkType        : NetworkType : type of network protocol
// RETURN :: NONE
//---------------------------------------------------------------------------
void RoutingDvmrpHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType)
{
    DvmrpData* dvmrp = (DvmrpData *)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_DVMRP);

    NetworkDataIp* ip = node->networkData.networkVar;

    IpInterfaceInfoType* interfaceInfo =
                      ip->interfaceInfo[interfaceIndex];

    if (interfaceInfo->addressState  == INVALID )
    {
        return;
    }

    // Determine interface type if IPv6
    if (NetworkIpGetInterfaceType(node, interfaceIndex) == NETWORK_IPV6)
    {
        return;
    }

    NodeAddress oldInterfaceAddress = 
                                dvmrp->interface[interfaceIndex].ipAddress;
    dvmrp->interface[interfaceIndex].ipAddress = 
                        NetworkIpGetInterfaceAddress(node, interfaceIndex);
    dvmrp->interface[interfaceIndex].subnetMask =
                    NetworkIpGetInterfaceSubnetMask(node, interfaceIndex);

    RoutingDvmrpAddNewRoute(
        node,
        NetworkIpGetInterfaceNetworkAddress(node, interfaceIndex),
        subnetMask,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        0,
        interfaceIndex);

    RoutingDvmrpUpdateRoute(
        node,
        NetworkIpGetInterfaceNetworkAddress(node, interfaceIndex),
        subnetMask,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        0,
        interfaceIndex);
}

