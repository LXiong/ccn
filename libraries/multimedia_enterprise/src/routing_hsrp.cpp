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


// This implementation follows RFC 2281.
// Features ommited: - Authentication of messages.
// Features to be implemented: - Multiple standby groups.

// Assumption: The same virtual IP address is used within the same group.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "network_ip.h"
#include "mac.h"
#include "app_util.h"
#include "routing_hsrp.h"

// The below Debug functions are solely used for debugging purposes.

static
BOOL HsrpDebug(Node* node)
{
    return FALSE;
}

static
BOOL HsrpDebugState(Node* node)
{
    return FALSE;
}

static
BOOL HsrpDebugTimer(Node* node)
{
    return FALSE;
}

static
BOOL HsrpDebugEvent(Node* node)
{
    return FALSE;
}


// -------------------------------------------------------------------------
// Name: HsrpGetStateString
// Purpose: Print the string representation of the state only.
// Return: None.
// -------------------------------------------------------------------------
char *HsrpGetStateString(Node* node, HsrpState state)
{
    switch (state)
    {
        case HSRP_INITIAL:
            return strdup("INITIAL");
            break;

        case HSRP_LEARN:
            return strdup("LEARN");
            break;

        case HSRP_LISTEN:
            return strdup("LISTEN");
            break;

        case HSRP_SPEAK:
            return strdup("SPEAK");
            break;

        case HSRP_STANDBY:
            return strdup("STANDBY");
            break;

        case HSRP_ACTIVE:
            return strdup("ACTIVE");
            break;

        default:
            assert(0);
            return NULL;
    }
}

// -------------------------------------------------------------------------
// Name: HsrpPrintState
// Purpose: Print the string representation of the state.
// Return: None.
// -------------------------------------------------------------------------
void HsrpPrintState(Node* node, HsrpState state)
{
    printf("    state is ");

    switch (state)
    {
        case HSRP_INITIAL:
            printf("INITIAL\n");
            break;

        case HSRP_LEARN:
            printf("LEARN\n");
            break;

        case HSRP_LISTEN:
            printf("LISTEN\n");
            break;

        case HSRP_SPEAK:
            printf("SPEAK\n");
            break;

        case HSRP_STANDBY:
            printf("STANDBY\n");
            break;

        case HSRP_ACTIVE:
            printf("ACTIVE\n");
            break;

        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpPrintPacket
// Purpose: Print the HSRP packet cotent.
// Return: None.
// -------------------------------------------------------------------------
void HsrpPrintPacket(Node* node, HsrpPacket* packet)
{
    char buf[MAX_STRING_LENGTH];

    printf("    packet content:\n");
    printf("        version = %d\n", packet->version);
    printf("        opCode = %d\n", packet->opCode);
    printf("        state = %d\n", packet->state);
    printf("        helloTime = %d\n", packet->helloTime);
    printf("        holdTime = %d\n", packet->holdTime);
    printf("        priority = %d\n", packet->priority);
    printf("        group = %d\n", packet->group);
    printf("        reserved = %d\n", packet->reserved);

    IO_ConvertIpAddressToString(packet->virtualIpAddress, buf);
    printf("        virtualIpAddress = %s\n", buf);
}


HsrpInterfaceInfo* HsrpGetInterfaceInfo(Node *node, int interfaceIndex)
{
    HsrpData* hsrp = (HsrpData*) node->appData.hsrp;
    HsrpInterfaceInfo* info;
    ListItem* item;

    item = hsrp->interfaceInfo->first;

    while (item)
    {
        info = (HsrpInterfaceInfo*) item->data;

        if (info->interfaceIndex == interfaceIndex ||
            interfaceIndex == ANY_INTERFACE)
        {
            return info;
        }

        item = item->next;
    }

    assert(0);
    return NULL;
}

// -------------------------------------------------------------------------
// Name: HsrpEnabledAndActiveRouter
// Purpose: Determined if Hsrp is enabled and whether or not the router
//          is an active router.
// Return: TRUE if HSRP is enabled and the router is an active router,
//         FALSE otherwise.
// Note: Called at the MAC layer to determine whether or not to
//       continue to process the packet/frame.
// -------------------------------------------------------------------------
BOOL HsrpEnabledAndActiveRouter(Node* node,
                                int interfaceIndex)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    // Check if we are running HSRP on this interface.
    if (!NetworkIpIsHsrpEnabled(node, interfaceIndex))
    {
        return FALSE;
    }
    else
    {
        assert(interfaceInfo != NULL);

        // Check if we are the active router.
        if (interfaceInfo->state == HSRP_ACTIVE)
        {
            return TRUE;
        }

        return FALSE;
    }
}


// -------------------------------------------------------------------------
// Name: HsrpHigherPriority
// Purpose: Determined which priority is higher.
// Return: TRUE the first priority is higher, FALSE otherwise.
// -------------------------------------------------------------------------
BOOL HsrpHigherPriority(int firstPriority,
                        NodeAddress firstNodeAddr,
                        int secondPriority,
                        NodeAddress secondNodeAddr)
{
    // When comparing priorities of two different routers,
    // the router with the numerically higher priority wins.
    // In the case of routers with equal priority the router
    // with the higher IP address wins.

    if (firstPriority > secondPriority)
    {
        return TRUE;
    }
    else if (firstPriority == secondPriority)
    {
        if (firstNodeAddr > secondNodeAddr)
        {
            return TRUE;
        }
    }

    return FALSE;
}


// -------------------------------------------------------------------------
// Name: HsrpLowerPriority
// Purpose: Determined which priority is lower.
// Return: TRUE the first priority is lower, FALSE otherwise.
// -------------------------------------------------------------------------
BOOL HsrpLowerPriority(int firstPriority,
                       NodeAddress firstNodeAddr,
                       int secondPriority,
                       NodeAddress secondNodeAddr)
{
    // When comparing priorities of two different routers,
    // the router with the numerically higher priority wins.
    // In the case of routers with equal priority the router
    // with the higher IP address wins.

    if (secondPriority > firstPriority)
    {
        return TRUE;
    }
    else if (secondPriority == firstPriority)
    {
        if (secondNodeAddr > firstNodeAddr)
        {
            return TRUE;
        }
    }

    return FALSE;
}


// -------------------------------------------------------------------------
// Name: HsrpSetState
// Purpose: Set the state of a node.
// Return: None.
// -------------------------------------------------------------------------
void HsrpSetState(Node* node, int interfaceIndex, HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    //if (HsrpDebugState(node))
    //{
    //  printf("    state before was %d\n", interfaceInfo->state);
    //}

    interfaceInfo->state = state;

    if (HsrpDebugState(node))
    {
        HsrpPrintState(node, interfaceInfo->state);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpStartHelloTimer
// Purpose: Start the hello timer, using jitters also.
// Return: None.
// -------------------------------------------------------------------------
void HsrpStartHelloTimer(Node* node, int interfaceIndex)
{
    HsrpInterfaceInfo* interfaceInfo;
    clocktype jitter;
    Message* timerMsg;
    HsrpTimerInfo* info;

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);
    interfaceInfo->timer.helloSeqno++;

    // Create the timer.
    timerMsg = MESSAGE_Alloc(node,
                             APP_LAYER,
                             APP_ROUTING_HSRP,
                             MSG_APP_HelloTimer);

    MESSAGE_InfoAlloc(node,
                      timerMsg,
                      sizeof(HsrpTimerInfo));

    info = (HsrpTimerInfo*) MESSAGE_ReturnInfo(timerMsg);

    info->seqno  = interfaceInfo->timer.helloSeqno;
    info->interfaceIndex= interfaceIndex;

    // Add jitter to the timer.
    jitter = RANDOM_nrand(interfaceInfo->jitterSeed) % HSRP_HELLO_JITTER;

    if (HsrpDebugTimer(node))
    {
        char buf[MAX_STRING_LENGTH];

        ctoa(interfaceInfo->helloTime, buf);
        printf("        to expire %s seconds from now\n", buf);
    }

    // Start the timer.
    MESSAGE_Send(node,
                 timerMsg,
                 (interfaceInfo->helloTime * SECOND) + jitter);
}


// -------------------------------------------------------------------------
// Name: HsrpStartActiveTimer
// Purpose: Start the active timer.
// Return: None.
// Note: Action A
// -------------------------------------------------------------------------
void HsrpStartActiveTimer(Node* node,
                          int interfaceIndex,
                          clocktype timerDelay)
{
    HsrpInterfaceInfo* interfaceInfo;
    Message* timerMsg;
    HsrpTimerInfo* info;

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);
    interfaceInfo->timer.activeSeqno++;

    // Create the timer.
    timerMsg = MESSAGE_Alloc(node,
                             APP_LAYER,
                             APP_ROUTING_HSRP,
                             MSG_APP_ActiveTimer);

    MESSAGE_InfoAlloc(node, timerMsg, sizeof(HsrpTimerInfo));
    info = (HsrpTimerInfo*) MESSAGE_ReturnInfo(timerMsg);

    info->seqno  = interfaceInfo->timer.activeSeqno;
    info->interfaceIndex = interfaceIndex;

    if (HsrpDebugTimer(node))
    {
        char buf[MAX_STRING_LENGTH];
        printf("    starting active timer with seqno %d\n",
               interfaceInfo->timer.activeSeqno);

        ctoa(timerDelay, buf);
        printf("        to expire %s seconds from now\n", buf);

        ctoa(timerDelay * SECOND + node->getNodeTime(), buf);
        printf("        fired at %s\n", buf);
    }

    // Start the timer.
    MESSAGE_Send(node, timerMsg, timerDelay * SECOND);
}


// -------------------------------------------------------------------------
// Name: HsrpStartStandbyTimer
// Purpose: Start the standby timer.
// Return: None.
// Note: Action B
// -------------------------------------------------------------------------
void HsrpStartStandbyTimer(Node* node,
                           int interfaceIndex,
                           clocktype timerDelay)
{
    HsrpInterfaceInfo* interfaceInfo;
    Message* timerMsg;
    HsrpTimerInfo* info;

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);
    interfaceInfo->timer.standbySeqno++;

    // Create the timer.
    timerMsg = MESSAGE_Alloc(node,
                             APP_LAYER,
                             APP_ROUTING_HSRP,
                             MSG_APP_StandbyTimer);

    MESSAGE_InfoAlloc(node, timerMsg, sizeof(HsrpTimerInfo));
    info = (HsrpTimerInfo*) MESSAGE_ReturnInfo(timerMsg);

    info->seqno  = interfaceInfo->timer.standbySeqno;
    info->interfaceIndex = interfaceIndex;

    if (HsrpDebugTimer(node))
    {
        char buf[MAX_STRING_LENGTH];
        printf("    starting standby timer with seqno %d\n",
               interfaceInfo->timer.standbySeqno);

        ctoa(timerDelay, buf);
        printf("        to expire %s seconds from now\n", buf);
    }

    // Start the timer.
    MESSAGE_Send(node, timerMsg, timerDelay * SECOND);
}


// -------------------------------------------------------------------------
// Name: HsrpStopActiveTimer
// Purpose: Cancel the active timer.
// Return: None.
// Note: Action C
// -------------------------------------------------------------------------
void HsrpStopActiveTimer(Node* node, int interfaceIndex)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    if (HsrpDebugTimer(node))
    {
        printf("    stopping active timer with seqno %d\n",
               interfaceInfo->timer.activeSeqno);
    }

    // Stop timer by increasing timer sequence number.
    interfaceInfo->timer.activeSeqno++;
}

// -------------------------------------------------------------------------
// Name: HsrpStopStandbyTimer
// Purpose: Cancel the standby timer.
// Return: None.
// Note: Action D
// -------------------------------------------------------------------------
void HsrpStopStandbyTimer(Node* node, int interfaceIndex)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    if (HsrpDebugTimer(node))
    {
        printf("    stopping standby timer with seqno %d\n",
               interfaceInfo->timer.standbySeqno);
    }

    // Stop timer by increasing timer sequence number.
    interfaceInfo->timer.standbySeqno++;
}

// -------------------------------------------------------------------------
// Name: HsrpLearnParameters
// Purpose: Learn about source node's HSRP parameters.
// Return: None.
// Note: Action E
// -------------------------------------------------------------------------
void HsrpLearnParameters(Node* node,
                         int interfaceIndex,
                         clocktype helloTime,
                         clocktype holdTime,
                         NodeAddress virtualIpAddress,
                         int incomingInterface)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    if (HsrpDebug(node))
    {
        printf("    learning parameters, if applicable\n");
    }

    // Only update parameters which were not user configured.

    if (!interfaceInfo->helloTimeConfigured)
    {
        interfaceInfo->helloTime = helloTime;
        printf("        helloTime learnt!\n");
    }

    if (!interfaceInfo->holdTimeConfigured)
    {
        interfaceInfo->holdTime = holdTime;
        printf("        holdTime learnt!\n");
    }

    if (!interfaceInfo->virtualIpAddressConfigured)
    {
        interfaceInfo->virtualIpAddress = virtualIpAddress;
        MAC_SetVirtualMacAddress(node, incomingInterface, virtualIpAddress);
        printf("        virtualIpAddress learnt!\n");
    }
}


// -------------------------------------------------------------------------
// Name: HsrpSendMessage
// Purpose: Send out an HSRP message with the appropriate fields.
// Return: None.
// -------------------------------------------------------------------------
void HsrpSendMessage(Node* node,
                     int interfaceIndex,
                     HsrpPacketType type,
                     HsrpState state,
                     clocktype helloTime,
                     clocktype holdTime,
                     int priority,
                     int group,
                     NodeAddress virtualIpAddress)
{
    //HsrpInterfaceInfo* interfaceInfo;
    HsrpPacket packet;
    NodeAddress sourceAddr;
    NodeAddress destAddr;

    //interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    // Fill in the packet fields.

    packet.version = (char) 0;
    packet.opCode = (char) type;
    packet.state = (char) state;
    packet.helloTime = (char) helloTime;
    packet.holdTime = (char) holdTime;
    packet.priority = (char) priority;
    packet.group = (char) group;
    packet.reserved = (char) 0;
    packet.authenticationData1 = HSRP_ATHENTICATION_DATA1;
    packet.authenticationData2 = HSRP_ATHENTICATION_DATA2;
    packet.virtualIpAddress = virtualIpAddress;

    if (HsrpDebug(node))
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);
        printf("Node %d sending packet at time %s\n", node->nodeId, buf);
        HsrpPrintPacket(node, &packet);
    }

    // Send message out to interfaces that supports HSRP only.

    sourceAddr = NetworkIpGetInterfaceAddress(node, interfaceIndex);
    destAddr = NetworkIpGetInterfaceBroadcastAddress(node, interfaceIndex);

    if (HsrpDebug(node))
    {
        char buf[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(destAddr, buf);
        printf("    to destAddr %s\n", buf);
    }

    Message *hsrpMsg = APP_UdpCreateMessage(
                       node,
                       sourceAddr,
                       APP_ROUTING_HSRP,
                       destAddr,
                       APP_ROUTING_HSRP,
                       TRACE_HSRP,
                       IPTOS_PREC_INTERNETCONTROL);

    APP_AddPayload(node, hsrpMsg, &packet, sizeof(packet));

    APP_UdpSend(node, hsrpMsg, PROCESS_IMMEDIATELY);
}



// -------------------------------------------------------------------------
// Name: HsrpSendHelloMessage
// Purpose: Send out an HSRP Hello message.
// Return: None.
// Note: Action F
// -------------------------------------------------------------------------
void HsrpSendHelloMessage(Node* node, int interfaceIndex)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    if (HsrpDebug(node))
    {
        printf("Node %d sending HELLO packet\n", node->nodeId);
    }

    HsrpSendMessage(node,
                    interfaceIndex,
                    HSRP_HELLO,
                    interfaceInfo->state,
                    interfaceInfo->helloTime,
                    interfaceInfo->holdTime,
                    interfaceInfo->priority,
                    interfaceInfo->group,
                    interfaceInfo->virtualIpAddress);

    interfaceInfo->stat.numHelloSent++;
}


// -------------------------------------------------------------------------
// Name: HsrpSendCoupMessage
// Purpose: Send out an COUP Hello message.
// Return: None.
// Note: Action G
// -------------------------------------------------------------------------
void HsrpSendCoupMessage(Node* node, int interfaceIndex)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    if (HsrpDebug(node))
    {
        printf("Node %d sending COUP packet\n", node->nodeId);
    }

    HsrpSendMessage(node,
                    interfaceIndex,
                    HSRP_COUP,
                    interfaceInfo->state,
                    interfaceInfo->helloTime,
                    interfaceInfo->holdTime,
                    interfaceInfo->priority,
                    interfaceInfo->group,
                    interfaceInfo->virtualIpAddress);

    interfaceInfo->stat.numCoupSent++;
}


// -------------------------------------------------------------------------
// Name: HsrpSendResignMessage
// Purpose: Send out an RESIGN Hello message.
// Return: None.
// Note: Action H
// -------------------------------------------------------------------------
void HsrpSendResignMessage(Node* node, int interfaceIndex)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    if (HsrpDebug(node))
    {
        printf("Node %d sending RESIGN packet\n", node->nodeId);
    }

    HsrpSendMessage(node,
                    interfaceIndex,
                    HSRP_RESIGN,
                    interfaceInfo->state,
                    interfaceInfo->helloTime,
                    interfaceInfo->holdTime,
                    interfaceInfo->priority,
                    interfaceInfo->group,
                    interfaceInfo->virtualIpAddress);

    interfaceInfo->stat.numResignSent++;
}


// -------------------------------------------------------------------------
// Name: HsrpSendGratuitousArpMessage
// Purpose: Send out an ARP message.  We don't have ARP in QualNet, so
//          don't do anything here.
// Return: None.
// Note: Action I
// -------------------------------------------------------------------------
void HsrpSendGratuitousArpMessage(Node* node, int interfaceIndex)
{
    if (HsrpDebug(node))
    {
        printf("Node %d sending GRATUITOUS ARP packet\n", node->nodeId);
        printf("    not implemented...\n");
    }
    // ARP not implemented in QualNet.
}


// -------------------------------------------------------------------------
// Name: HsrpProcessInterfaceUpEvent
// Purpose: Take appropriate actions when an interface up event occurs.
// Return: None.
// Note: Processing event a.
// -------------------------------------------------------------------------
void HsrpProcessInterfaceUpEvent(Node* node,
                                 int interfaceIndex,
                                 HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
        {
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 interfaceInfo->holdTime);

            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  interfaceInfo->holdTime);

            if (interfaceInfo->virtualIpAddressConfigured)
            {
                HsrpSetState(node, interfaceIndex, HSRP_LISTEN);
            }
            else
            {
                HsrpSetState(node, interfaceIndex, HSRP_LEARN);
            }

            break;
        }
        case HSRP_LEARN:
        case HSRP_LISTEN:
        case HSRP_SPEAK:
        case HSRP_STANDBY:
        case HSRP_ACTIVE:
            break;

        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessInterfaceDownEvent
// Purpose: Take appropriate actions when an interface down event occurs.
// Return: None.
// Note: Processing event b.
// -------------------------------------------------------------------------
void HsrpProcessInterfaceDownEvent(Node* node,
                                   int interfaceIndex,
                                   HsrpState state)
{
    //HsrpInterfaceInfo* interfaceInfo;
    //interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
            break;

        case HSRP_LEARN:
        case HSRP_LISTEN:
        case HSRP_SPEAK:
        case HSRP_STANDBY:
        {
            HsrpStopActiveTimer(node, interfaceIndex);
            HsrpStopStandbyTimer(node, interfaceIndex);
            HsrpSetState(node, interfaceIndex, HSRP_INITIAL);

            break;
        }
        case HSRP_ACTIVE:
        {
            HsrpStopActiveTimer(node, interfaceIndex);
            HsrpStopStandbyTimer(node, interfaceIndex);
            HsrpSendResignMessage(node, interfaceIndex);
            HsrpSetState(node, interfaceIndex, HSRP_INITIAL);

            break;
        }
        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessActiveTimerExpiryEvent
// Purpose: Take appropriate actions when an active timer expiry
//          event occurs.
// Return: None.
// Note: Processing event c.
// -------------------------------------------------------------------------
void HsrpProcessActiveTimerExpiryEvent(Node* node,
                                       int interfaceIndex,
                                       HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
        case HSRP_LEARN:
            break;

        case HSRP_LISTEN:
        {
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 interfaceInfo->holdTime);

            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  interfaceInfo->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_SPEAK);

            break;
        }
        case HSRP_SPEAK:
            break;

        case HSRP_STANDBY:
        {
            HsrpStopActiveTimer(node, interfaceIndex);
            HsrpStopStandbyTimer(node, interfaceIndex);
            HsrpSendHelloMessage(node, interfaceIndex);
            HsrpSendGratuitousArpMessage(node, interfaceIndex);

            HsrpSetState(node, interfaceIndex, HSRP_ACTIVE);

            break;
        }
        case HSRP_ACTIVE:
            break;

        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessStandbyTimerExpiry
// Purpose: Take appropriate actions when a standby timer expiry
//          event occurs.
// Return: None.
// Note: Processing event d.
// -------------------------------------------------------------------------
void HsrpProcessStandbyTimerExpiry(Node* node,
                                   int interfaceIndex,
                                   HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
        case HSRP_LEARN:
            break;

        case HSRP_LISTEN:
        {
            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  interfaceInfo->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_SPEAK);

            break;
        }
        case HSRP_SPEAK:
        {
            HsrpStopStandbyTimer(node, interfaceIndex);

            // TBD: The state diagram does not include this action.
            //      However, how will the router transition from
            //      STANDBY to ACTIVE if we do not include this???
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 interfaceInfo->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_STANDBY);
            break;
        }
        case HSRP_STANDBY:
        case HSRP_ACTIVE:
            break;

        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessHelloTimerExpiry
// Purpose: Take appropriate actions when a hello timer expiry
//          event occurs.
// Return: None.
// Note: Processing event e.
// -------------------------------------------------------------------------
void HsrpProcessHelloTimerExpiry(Node* node,
                                 int interfaceIndex,
                                 HsrpState state)
{
    //HsrpInterfaceInfo* interfaceInfo;
    //interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
        case HSRP_LEARN:
        case HSRP_LISTEN:
            break;

        case HSRP_SPEAK:
        case HSRP_STANDBY:
        case HSRP_ACTIVE:
        {
            HsrpSendHelloMessage(node, interfaceIndex);
            break;
        }
        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessReceiveHelloHigherPriorityInSpeakStateEvent
// Purpose: Take appropriate actions when receipt of a Hello message
//          of higher priority from a router in Speak state event occurs.
// Return: None.
// Note: Processing event f.
// -------------------------------------------------------------------------
void HsrpProcessReceiveHelloHigherPriorityInSpeakStateEvent(
                                                        Node* node,
                                                        int interfaceIndex,
                                                        Message* msg,
                                                        HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
        case HSRP_LEARN:
        case HSRP_LISTEN:
            break;

        case HSRP_SPEAK:
        case HSRP_STANDBY:
        {
            // Not from Standby router, so use own hold time.
            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  interfaceInfo->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_LISTEN);

            break;
        }
        case HSRP_ACTIVE:
            break;

        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessReceiveHelloHigherPriorityFromActiveEvent
// Purpose: Take appropriate actions when receipt of a Hello message
//          of higher priority from the active router event occurs.
// Return: None.
// Note: Processing event g.
// -------------------------------------------------------------------------
void HsrpProcessReceiveHelloHigherPriorityFromActiveEvent(Node* node,
                                                          int interfaceIndex,
                                                          Message* msg,
                                                          HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;
    HsrpPacket* packet = (HsrpPacket*) MESSAGE_ReturnPacket(msg);
    UdpToAppRecv* info = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);
    int incomingInterface = info->incomingInterfaceIndex;

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
            break;
        case HSRP_LEARN:
        {
            HsrpLearnParameters(node,
                                interfaceIndex,
                                packet->helloTime,
                                packet->holdTime,
                                packet->virtualIpAddress,
                                incomingInterface);

            // Need to use hold time from the Hello message.
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 packet->holdTime);

            // Hold time not from Hello since recevied from Active router.
            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  interfaceInfo->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_LISTEN);

            break;
        }
        case HSRP_LISTEN:
        case HSRP_SPEAK:
        case HSRP_STANDBY:
        {
            HsrpLearnParameters(node,
                                interfaceIndex,
                                packet->helloTime,
                                packet->holdTime,
                                packet->virtualIpAddress,
                                incomingInterface);

            // Need to use hold time from the Hello message.
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 packet->holdTime);

            break;
        }
        case HSRP_ACTIVE:
        {
            // Need to use hold time from the Hello message.
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 packet->holdTime);

            // Hold time not from Hello since recevied from Active router.
            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  interfaceInfo->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_SPEAK);
            break;
        }
        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessReceiveHelloLowerPriorityFromActiveEvent
// Purpose: Take appropriate actions when receipt of a Hello message
//           of lower priority from the active router event occurs.
// Return: None.
// Note: Processing event h.
// -------------------------------------------------------------------------
void HsrpProcessReceiveHelloLowerPriorityFromActiveEvent(Node* node,
                                                         int interfaceIndex,
                                                         Message* msg,
                                                         HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;
    HsrpPacket* packet = (HsrpPacket*) MESSAGE_ReturnPacket(msg);
    UdpToAppRecv* info = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);
    int incomingInterface = info->incomingInterfaceIndex;

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
            break;

        case HSRP_LEARN:
        {
            HsrpLearnParameters(node,
                                interfaceIndex,
                                packet->helloTime,
                                packet->holdTime,
                                packet->virtualIpAddress,
                                incomingInterface);

            // Need to use hold time from the Hello message.
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 packet->holdTime);

            // Hold time not from Hello since recevied from Active router.
            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  interfaceInfo->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_LISTEN);
            break;
        }
        case HSRP_LISTEN:
        case HSRP_SPEAK:
        case HSRP_STANDBY:
        {
            if (interfaceInfo->preemptionCapability)
            {
                if (HsrpDebug(node))
                {
                    printf("    preemption enabled\n");
                }

                // Hold time not from Hello since recevied from
                // Active router.
                HsrpStartStandbyTimer(node,
                                      interfaceIndex,
                                      interfaceInfo->holdTime);

                HsrpSendCoupMessage(node, interfaceIndex);
                HsrpSendHelloMessage(node, interfaceIndex);
                HsrpSendGratuitousArpMessage(node, interfaceIndex);
                HsrpSetState(node, interfaceIndex, HSRP_ACTIVE);
            }
            else
            {
                if (HsrpDebug(node))
                {
                    printf("    preemption disabled\n");
                }

                // Need to use hold time from the Hello message.
                HsrpStartActiveTimer(node,
                                     interfaceIndex,
                                     packet->holdTime);
            }
            break;
        }
        case HSRP_ACTIVE:
        {
            HsrpSendCoupMessage(node, interfaceIndex);
            break;
        }
        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessReceiveResignFromActiveEvent
// Purpose: Take appropriate actions when receipt of a Resign message
//          from the active router event occurs.
// Return: None.
// Note: Processing event i.
// -------------------------------------------------------------------------
void HsrpProcessReceiveResignFromActiveEvent(Node* node,
                                             int interfaceIndex,
                                             Message* msg,
                                             HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
        case HSRP_LEARN:
            break;
        case HSRP_LISTEN:
        {
            // Use own hold time since message is not Hello.
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 interfaceInfo->holdTime);

            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  interfaceInfo->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_SPEAK);
            break;
        }
        case HSRP_SPEAK:
        {
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 interfaceInfo->holdTime);
            break;
        }
        case HSRP_STANDBY:
        {
            // Use own hold time since message is not Hello.
            HsrpStopActiveTimer(node, interfaceIndex);

            HsrpSendHelloMessage(node, interfaceIndex);
            HsrpSendGratuitousArpMessage(node, interfaceIndex);
            HsrpSetState(node, interfaceIndex, HSRP_ACTIVE);

            break;
        }
        case HSRP_ACTIVE:
            break;

        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessReceiveCoupHigherPriorityEvent
// Purpose: Take appropriate actions when receipt of a Coup message
//          from a higher priority router event occurs.
// Return: None.
// Note: Processing event j.
// -------------------------------------------------------------------------
void HsrpProcessReceiveCoupHigherPriorityEvent(Node* node,
                                               int interfaceIndex,
                                               Message* msg,
                                               HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
        case HSRP_LEARN:
        case HSRP_LISTEN:
        case HSRP_SPEAK:
        case HSRP_STANDBY:
            break;

        case HSRP_ACTIVE:
        {
            // Use own hold time since message is not Hello.
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 interfaceInfo->holdTime);

            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  interfaceInfo->holdTime);

            HsrpSendResignMessage(node, interfaceIndex);
            HsrpSetState(node, interfaceIndex, HSRP_SPEAK);

            break;
        }
        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessReceiveHelloHigherPriorityFromStandbyEvent
// Purpose: Take appropriate actions when receipt of a Hello message
//          of higher priority from the standby router event occurs.
// Return: None.
// Note: Processing event k.
// -------------------------------------------------------------------------
void HsrpProcessReceiveHelloHigherPriorityFromStandbyEvent(
                                                    Node* node,
                                                    int interfaceIndex,
                                                    Message* msg,
                                                    HsrpState state)
{
    //HsrpInterfaceInfo* interfaceInfo;
    HsrpPacket* packet = (HsrpPacket*) MESSAGE_ReturnPacket(msg);

    //interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
        case HSRP_LEARN:
            break;

        case HSRP_LISTEN:
        {
            // Need to use hold time from the Hello message.
            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  packet->holdTime);
            break;
        }
        case HSRP_SPEAK:
        case HSRP_STANDBY:
        {
            // Need to use hold time from the Hello message.
            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  packet->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_LISTEN);

            break;
        }
        case HSRP_ACTIVE:
        {
            // Need to use hold time from the Hello message.
            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  packet->holdTime);
            break;
        }
        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessReceiveHelloLowerPriorityFromStandbyEvent
// Purpose: Take appropriate actions when receipt of a Hello message
//          of lower priority from the standby router event occurs.
// Return: None.
// Note: Processing event l.
// -------------------------------------------------------------------------
void HsrpProcessReceiveHelloLowerPriorityFromStandbyEvent(Node* node,
                                                          int interfaceIndex,
                                                          Message* msg,
                                                          HsrpState state)
{
    HsrpInterfaceInfo* interfaceInfo;
    HsrpPacket* packet = (HsrpPacket*) MESSAGE_ReturnPacket(msg);

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (state)
    {
        case HSRP_INITIAL:
        case HSRP_LEARN:
            break;

        case HSRP_LISTEN:
        {
            // Need to use hold time from the Hello message.
            HsrpStartStandbyTimer(node, interfaceIndex, packet->holdTime);
            HsrpSetState(node, interfaceIndex, HSRP_SPEAK);
            break;
        }
        case HSRP_SPEAK:
        {
            HsrpStopStandbyTimer(node, interfaceIndex);

            // TBD: The state diagram does not include this action.
            //      However, how will the router transition from
            //      STANDBY to ACTIVE if we do not include this???
            HsrpStartActiveTimer(node,
                                 interfaceIndex,
                                 interfaceInfo->holdTime);

            HsrpSetState(node, interfaceIndex, HSRP_STANDBY);

            break;
        }
        case HSRP_STANDBY:
            break;

        case HSRP_ACTIVE:
        {
            // Need to use hold time from the Hello message.
            HsrpStartStandbyTimer(node,
                                  interfaceIndex,
                                  packet->holdTime);
            break;
        }
        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpProcessEvent
// Purpose: Calls the appropriate event functions based on the event
//          that has occurred.
// Return: None.
// -------------------------------------------------------------------------
void HsrpProcessEvent(Node* node,
                      int interfaceIndex,
                      Message* msg,
                      HsrpEvent event)
{
    HsrpInterfaceInfo* interfaceInfo;

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    switch (event)
    {
        case HSRP_INTERFACE_UP:
            if (HsrpDebugEvent(node))
            {
                printf("    event is HSRP_INTERFACE_UP\n");
            }

            HsrpProcessInterfaceUpEvent(node,
                                        interfaceIndex,
                                        interfaceInfo->state);
            break;

        case HSRP_INTERFACE_DOWN:
            if (HsrpDebugEvent(node))
            {
                printf("    event is HSRP_INTERFACE_DOWN\n");
            }

            HsrpProcessInterfaceDownEvent(node,
                                          interfaceIndex,
                                          interfaceInfo->state);
            break;

        case HSRP_ACTIVE_TIMER_EXPIRY:
            if (HsrpDebugEvent(node))
            {
                printf("    event is HSRP_ACTIVE_TIMER_EXPIRY\n");
            }

            HsrpProcessActiveTimerExpiryEvent(node,
                                              interfaceIndex,
                                              interfaceInfo->state);
            break;

        case HSRP_STANDBY_TIMER_EXPIRY:
            if (HsrpDebugEvent(node))
            {
                printf("    event is HSRP_STANDBY_TIMER_EXPIRY\n");
            }

            HsrpProcessStandbyTimerExpiry(node,
                                          interfaceIndex,
                                          interfaceInfo->state);
            break;

        case HSRP_HELLO_TIMER_EXPIRY:
            if (HsrpDebugEvent(node))
            {
                printf("    event is HSRP_HELLO_TIMER_EXPIRY\n");
            }

            HsrpProcessHelloTimerExpiry(node,
                                        interfaceIndex,
                                        interfaceInfo->state);
            break;

        case HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_IN_SPEAK_STATE:
            if (HsrpDebugEvent(node))
            {
                printf("    event is "
                   "HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_IN_SPEAK_STATE\n");
            }

            HsrpProcessReceiveHelloHigherPriorityInSpeakStateEvent(
                                                node,
                                                interfaceIndex,
                                                msg,
                                                interfaceInfo->state);
            break;

        case HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_FROM_ACTIVE:
            if (HsrpDebugEvent(node))
            {
                printf("    event is "
                       "HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_FROM_ACTIVE\n");
            }

            HsrpProcessReceiveHelloHigherPriorityFromActiveEvent(
                                                node,
                                                interfaceIndex,
                                                msg,
                                                interfaceInfo->state);
            break;

        case HSRP_RECEIVE_HELLO_LOWER_PRIORITY_FROM_ACTIVE:
            if (HsrpDebugEvent(node))
            {
                printf("    event is "
                       "HSRP_RECEIVE_HELLO_LOWER_PRIORITY_FROM_ACTIVE\n");
            }

            HsrpProcessReceiveHelloLowerPriorityFromActiveEvent(
                                                node,
                                                interfaceIndex,
                                                msg,
                                                interfaceInfo->state);
            break;

        case HSRP_RECEIVE_RESIGN_FROM_ACTIVE:
            if (HsrpDebugEvent(node))
            {
                printf("    event is "
                       "HSRP_RECEIVE_RESIGN_FROM_ACTIVE\n");
            }

            HsrpProcessReceiveResignFromActiveEvent(node,
                                                    interfaceIndex,
                                                    msg,
                                                    interfaceInfo->state);
            break;

        case HSRP_RECEIVE_COUP_HIGHER_PRIORITY:
            if (HsrpDebugEvent(node))
            {
                printf("    event is "
                       "HSRP_RECEIVE_COUP_HIGHER_PRIORITY\n");
            }

            HsrpProcessReceiveCoupHigherPriorityEvent(node,
                                                      interfaceIndex,
                                                      msg,
                                                      interfaceInfo->state);
            break;

        case HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_FROM_STANDBY:
            if (HsrpDebugEvent(node))
            {
                printf("    event is "
                       "HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_FROM_STANDBY\n");
            }

            HsrpProcessReceiveHelloHigherPriorityFromStandbyEvent(
                                                node,
                                                interfaceIndex,
                                                msg,
                                                interfaceInfo->state);
            break;

        case HSRP_RECEIVE_HELLO_LOWER_PRIORITY_FROM_STANDBY:
            if (HsrpDebugEvent(node))
            {
                printf("    event is "
                       "HSRP_RECEIVE_HELLO_LOWER_PRIORITY_FROM_STANDBY\n");
            }

            HsrpProcessReceiveHelloLowerPriorityFromStandbyEvent(
                                                node,
                                                interfaceIndex,
                                                msg,
                                                interfaceInfo->state);
            break;

        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: HsrpInterfaceStatusHandler
// Purpose: Takes care of situations where the interface goes up and down.
// Return: None.
// -------------------------------------------------------------------------
void HsrpInterfaceStatusHandler(Node *node,
                                int interfaceIndex,
                                MacInterfaceState state)
{
    HsrpInterfaceInfo* interfaceInfo;

    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    if (HsrpDebug(node))
    {
        char buf[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), buf);
        printf("Node %d got interface status update at time %s\n",
               node->nodeId, buf);
        HsrpPrintState(node, interfaceInfo->state);
    }

    switch (state)
    {
        // Interface just went down.
        case MAC_INTERFACE_DISABLE:
            HsrpProcessEvent(node,
                             interfaceIndex,
                             NULL,
                             HSRP_INTERFACE_DOWN);
            break;

        // Interface just came back up.
        case MAC_INTERFACE_ENABLE:
            HsrpProcessEvent(node,
                             interfaceIndex,
                             NULL,
                             HSRP_INTERFACE_UP);
            break;

        default:
            assert(0);
    }
}



// -------------------------------------------------------------------------
// Name: RoutingHsrpInit
// Purpose: Handling all initializations needed by static routing.
// Return: None.
// -------------------------------------------------------------------------
void
RoutingHsrpInit(
    Node* node,
    const NodeInput* nodeInput,
    const int interfaceIndex)
{
    HsrpData* hsrp = (HsrpData*) node->appData.hsrp;
    HsrpInterfaceInfo* interfaceInfo;
    int groupNumber;
    int priority;
    clocktype helloTime;
    clocktype holdTime;
    BOOL preemptionCapability = FALSE;
    NodeAddress virtualIpAddress;
    char preemptionCapabilityStr[MAX_STRING_LENGTH];
    char virtualIpAddressStr[MAX_STRING_LENGTH];
    BOOL retVal;

    if (hsrp == NULL)
    {
        hsrp = (HsrpData*) MEM_malloc(sizeof(HsrpData));
        node->appData.hsrp = (void*) hsrp;
        ListInit(node, &hsrp->interfaceInfo);
    }

    interfaceInfo = (HsrpInterfaceInfo*)
                    MEM_malloc(sizeof(HsrpInterfaceInfo));
    memset(interfaceInfo, 0, sizeof(HsrpInterfaceInfo));

    RANDOM_SetSeed(interfaceInfo->jitterSeed,
                   node->globalSeed,
                   node->nodeId,
                   APP_ROUTING_HSRP,
                   interfaceIndex);

    NetworkIpSetHsrpOnInterface(node, interfaceIndex);

    MAC_SetInterfaceStatusHandlerFunction(node,
                                          interfaceIndex,
                                          &HsrpInterfaceStatusHandler);

    // Get the HSRP configuration file...
    IO_ReadInt(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "HSRP-STANDBY-GROUP-NUMBER",
        &retVal,
        &groupNumber);

    if (!retVal)
    {
        groupNumber = HSRP_DEFAULT_GROUP_NUMBER;
    }

    if (groupNumber < HSRP_MIN_GROUP_NUMBER)
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "HSRP-STANDBY-GROUP-NUMBER (%d) must be >= %d.\n",
                     groupNumber, HSRP_MIN_GROUP_NUMBER);

        ERROR_ReportError(buf);
    }

    if (groupNumber > HSRP_MAX_GROUP_NUMBER)
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "HSRP-STANDBY-GROUP-NUMBER (%d) must be <= %d.\n",
                     groupNumber, HSRP_MAX_GROUP_NUMBER);

        ERROR_ReportError(buf);
    }

    IO_ReadInt(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "HSRP-PRIORITY",
        &retVal,
        &priority);

    if (!retVal)
    {
        priority = HSRP_DEFAULT_PRIORITY;
    }

    IO_ReadTime(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "HSRP-HELLO-TIME",
        &retVal,
        &helloTime);

    interfaceInfo->helloTimeConfigured = TRUE;

    if (!retVal)
    {
        interfaceInfo->helloTimeConfigured = FALSE;
        helloTime = HSRP_DEFAULT_HELLO_TIME;
    }

    // Get the HSRP configuration file...
    IO_ReadTime(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "HSRP-HOLD-TIME",
        &retVal,
        &holdTime);

    interfaceInfo->holdTimeConfigured = TRUE;

    if (!retVal)
    {
        interfaceInfo->holdTimeConfigured = FALSE;
        holdTime = HSRP_DEFAULT_HOLD_TIME;
    }

    if (holdTime <= helloTime)
    {
        char buf[MAX_STRING_LENGTH];
        char holdTimeString[MAX_STRING_LENGTH];
        char helloTimeString[MAX_STRING_LENGTH];

        ctoa(holdTime, holdTimeString);
        ctoa(helloTime, helloTimeString);

        sprintf(buf, "HSRP: HOLD-TIME (%s) must be greater than "
                     "HELLO-TIME (%s).\n", holdTimeString, helloTimeString);

        ERROR_ReportError(buf);
    }

    // Get the HSRP configuration file...
    IO_ReadString(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "HSRP-PREEMPTION-CAPABILITY",
        &retVal,
        preemptionCapabilityStr);

    if (!retVal)
    {
        preemptionCapability = HSRP_DEFAULT_PREEMPTION_CAPABILITY;
    }
    else
    {
        if (strcmp(preemptionCapabilityStr, "YES") == 0)
        {
            preemptionCapability = TRUE;
        }
        else if (strcmp(preemptionCapabilityStr, "NO") == 0)
        {
            preemptionCapability = FALSE;
        }
        else
        {
        char buf[MAX_STRING_LENGTH];

            sprintf(buf, "HSRP: Invalid "
                         "HSRP-PREEMPTION-CAPABILITY (%s)\n",
                          preemptionCapabilityStr);
        }
    }


    // Make sure that at least one router is configured with a
    // virtual IP address.

    // TBD: We cannot do this in QualNet at the moment, so we must
    //      make sure that every node is assigned a virtual IP address.

    //IO_ReadString(
    //    ANY_NODEID,
    //    ANY_ADDRESS,
    //    nodeInput,
    //    "HSRP-VIRTUAL-IP-ADDRESS",
    //    &retVal,
    //    virtualIpAddressStr);

    //ERROR_Assert(retVal,
    //             "HSRP: At least one router must be configured with "
    //             "a virtual IP address.\n");

    // Now, if we are configured with a virtual IP address, then
    // go ahead and find out what it is.

    IO_ReadString(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "HSRP-VIRTUAL-IP-ADDRESS",
        &retVal,
        virtualIpAddressStr);

    // TBD: For the moment, we reinforce that every node must have
    //      a virtual IP address assigned to them.

    ERROR_Assert(retVal,
                 "HSRP: Router must be configured with "
                 "a virtual IP address.\n");

    interfaceInfo->virtualIpAddressConfigured = TRUE;

    if (retVal)
    {
        NodeAddress nodeId;

        IO_AppParseSourceString(
                    node,
                    virtualIpAddressStr,
                    virtualIpAddressStr,
                    &nodeId,
                    &virtualIpAddress);
    }
    else
    {
        interfaceInfo->virtualIpAddressConfigured = FALSE;
        virtualIpAddress = ANY_IP;
    }

    interfaceInfo->interfaceIndex = interfaceIndex;

    interfaceInfo->group = groupNumber;
    interfaceInfo->priority = priority;

    // Store these values in seconds...
    interfaceInfo->helloTime = helloTime / SECOND;
    interfaceInfo->holdTime = holdTime / SECOND;

    interfaceInfo->virtualIpAddress = virtualIpAddress;

    // QualNet does not support MAC address, so use IP address instead.
    MAC_SetVirtualMacAddress(node, interfaceIndex, virtualIpAddress);

    interfaceInfo->preemptionCapability = preemptionCapability;

    // Assigned the IP address from interface 0 as the IP address
    // of this node.
    interfaceInfo->ipAddress = NetworkIpGetInterfaceAddress(node,
                                                            interfaceIndex);

    interfaceInfo->timer.activeSeqno = 0;
    interfaceInfo->timer.standbySeqno = 0;

    interfaceInfo->stat.numHelloSent = 0;
    interfaceInfo->stat.numCoupSent = 0;
    interfaceInfo->stat.numResignSent = 0;
    interfaceInfo->stat.numHelloReceived = 0;
    interfaceInfo->stat.numCoupReceived = 0;
    interfaceInfo->stat.numResignReceived = 0;

    if (HsrpDebug(node))
    {
        char buf[MAX_STRING_LENGTH];

        printf("Node %d intial configuration:\n", node->nodeId);
        printf("    group = %d\n",
               interfaceInfo->group);
        printf("    priority = %d\n", interfaceInfo->priority);

        ctoa(interfaceInfo->helloTime, buf);
        printf("    helloTime = %s\n", buf);
        ctoa(interfaceInfo->holdTime, buf);
        printf("    holdTime = %s\n", buf);
        IO_ConvertIpAddressToString(interfaceInfo->virtualIpAddress, buf);
        printf("    virtualIpAddress = %s\n", buf);
        printf("    preemptionCapability = %d\n",
               interfaceInfo->preemptionCapability);
        IO_ConvertIpAddressToString(interfaceInfo->ipAddress, buf);
        printf("    ipAddress = %s\n", buf);
    }


    ListInsert(node, hsrp->interfaceInfo, 0, interfaceInfo);

    // TBD: Application cannot yet send multicast packet without
    //      using multicast routing protocol.  This multicast address
    //      is really "limited broadcast."
    // All HSRP routers need to listen on this multicast address.
    //NetworkIpAddToMulticastGroupList(node, HSRP_ALL_ROUTERS);

    // Initial state.
    HsrpSetState(node, interfaceIndex, HSRP_INITIAL);

    HsrpStartHelloTimer(node, interfaceIndex);

    // Assume that the interface is up at the start of simulation
    HsrpProcessEvent(node, interfaceIndex, NULL, HSRP_INTERFACE_UP);
}


// -------------------------------------------------------------------------
// Name: HsrpIsAuthenticated
// Purpose: Authenticates the incoming messages.
// Return: TRUE if messages is authenticated, FALSE otherwise.
// -------------------------------------------------------------------------
BOOL HsrpIsAuthenticated(Node* node, HsrpPacket* packet)
{
    // We do not take care of authentication...
    return TRUE;
}


// -------------------------------------------------------------------------
// Name: HsrpHandleDataFromTransport
// Purpose: Handle data that just arrived and process them accordingly.
// Return: None.
// -------------------------------------------------------------------------
void HsrpHandleDataFromTransport(Node* node, Message* msg)
{
    HsrpPacket* packet;
    int priority;
    int state;
    UdpToAppRecv* info;
    NodeAddress fromAddr;
    int incomingInterface;
    HsrpInterfaceInfo* interfaceInfo;

    packet = (HsrpPacket*) MESSAGE_ReturnPacket(msg);
    priority = (int) packet->priority;
    state = (int) packet->state;
    info = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);
    fromAddr = GetIPv4Address(info->sourceAddr);
    incomingInterface = info->incomingInterfaceIndex;

    // Don't process message if we are not configured for HSRP on
    // this interface.
    if (!NetworkIpIsHsrpEnabled(node, incomingInterface))
    {
        return;
    }

    interfaceInfo = HsrpGetInterfaceInfo(node, incomingInterface);

    // Make sure that messages are authenticated before processing them.
    if (!HsrpIsAuthenticated(node, packet))
    {
        return;
    }

    if (HsrpDebug(node))
    {
        char buf[MAX_STRING_LENGTH];
        char addrStr[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), buf);
        IO_ConvertIpAddressToString(fromAddr, addrStr);

        printf("Node %d received packet from node %s at time %s\n",
               node->nodeId, addrStr, buf);
        HsrpPrintState(node, interfaceInfo->state);
    }

    switch (packet->opCode)
    {
        case HSRP_HELLO:
        {
            interfaceInfo->stat.numHelloReceived++;

            if (HsrpDebug(node))
            {
                printf("    it's a HELLO packet\n");
                HsrpPrintPacket(node,
                                (HsrpPacket*) MESSAGE_ReturnPacket(msg));
            }

            // Received Hello from higher priority router in Speak state.
            if (HsrpHigherPriority(priority,
                                   fromAddr,
                                   interfaceInfo->priority,
                                   interfaceInfo->ipAddress) &&
                state == HSRP_SPEAK)
            {
                HsrpProcessEvent(
                        node,
                        incomingInterface,
                        msg,
                        HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_IN_SPEAK_STATE);
            }
            // Received Hello from higher priority router in Active state.
            else if (HsrpHigherPriority(priority,
                                        fromAddr,
                                        interfaceInfo->priority,
                                        interfaceInfo->ipAddress) &&
                     state == HSRP_ACTIVE)
            {
                HsrpProcessEvent(
                        node,
                        incomingInterface,
                        msg,
                        HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_FROM_ACTIVE);
            }
            // Received Hello from lower priority router in Active state.
            else if (HsrpLowerPriority(priority,
                                       fromAddr,
                                       interfaceInfo->priority,
                                       interfaceInfo->ipAddress) &&
                     state == HSRP_ACTIVE)
            {
                HsrpProcessEvent(
                        node,
                        incomingInterface,
                        msg,
                        HSRP_RECEIVE_HELLO_LOWER_PRIORITY_FROM_ACTIVE);
            }
            // Received Hello from higher priority router in Standby state.
            else if (HsrpHigherPriority(priority,
                                        fromAddr,
                                        interfaceInfo->priority,
                                        interfaceInfo->ipAddress) &&
                     state == HSRP_STANDBY)
            {
                HsrpProcessEvent(
                        node,
                        incomingInterface,
                        msg,
                        HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_FROM_STANDBY);
            }
            // Received Hello from lower priority router in Standby state.
            else if (HsrpLowerPriority(priority,
                                       fromAddr,
                                       interfaceInfo->priority,
                                       interfaceInfo->ipAddress) &&
                     state == HSRP_STANDBY)
            {
                HsrpProcessEvent(
                        node,
                        incomingInterface,
                        msg,
                        HSRP_RECEIVE_HELLO_LOWER_PRIORITY_FROM_STANDBY);
            }
            else
            {
                if (HsrpDebug(node))
                {
                    printf("    sender priority = %d, state = %d\n",
                           priority, state);
                }
            }

            break;
        }
        case HSRP_COUP:
        {
            interfaceInfo->stat.numCoupReceived++;

            if (HsrpDebug(node))
            {
                printf("    it's a COUP packet\n");
                HsrpPrintPacket(node,
                                (HsrpPacket*) MESSAGE_ReturnPacket(msg));
            }

            // Received Coup from higher priority router.
            if (HsrpHigherPriority(priority,
                                   fromAddr,
                                   interfaceInfo->priority,
                                   interfaceInfo->ipAddress))
            {
                HsrpProcessEvent(
                        node,
                        incomingInterface,
                        msg,
                        HSRP_RECEIVE_COUP_HIGHER_PRIORITY);
            }
            else
            {
                if (HsrpDebug(node))
                {
                    printf("    sender priority = %d, state = %d\n",
                           priority, state);
                }
            }

            break;
        }
        case HSRP_RESIGN:
        {
            interfaceInfo->stat.numResignReceived++;

            if (HsrpDebug(node))
            {
                printf("    it's a RESIGN packet\n");
                HsrpPrintPacket(node,
                                (HsrpPacket*) MESSAGE_ReturnPacket(msg));
            }

            // Received Resign from Active router.
            if (packet->state == HSRP_ACTIVE)
            {
                HsrpProcessEvent(
                        node,
                        incomingInterface,
                        msg,
                        HSRP_RECEIVE_RESIGN_FROM_ACTIVE);
            }
            else
            {
                if (HsrpDebug(node))
                {
                    printf("    sender priority = %d, state = %d\n",
                           priority, state);
                }
            }

            break;
        }
        default:
            assert(0);
    }
}


// -------------------------------------------------------------------------
// Name: RoutingHsrpLayer
// Purpose: Handles all timers and packets from transport layer.
// Return: None.
// -------------------------------------------------------------------------
void
RoutingHsrpLayer(Node* node, Message* msg)
{
    HsrpInterfaceInfo* interfaceInfo;
    HsrpTimerInfo* info;
    char clockStr[MAX_STRING_LENGTH];

    ctoa(node->getNodeTime(), clockStr);

    switch (msg->eventType)
    {
        case MSG_APP_HelloTimer:
        {
            info = (HsrpTimerInfo*) MESSAGE_ReturnInfo(msg);
            interfaceInfo = HsrpGetInterfaceInfo(node, info->interfaceIndex);

            if (HsrpDebugTimer(node))
            {
                printf("Node %d got HelloTimer expiry at %s\n",
                       node->nodeId, clockStr);
                HsrpPrintState(node, interfaceInfo->state);
            }

            if (info->seqno == interfaceInfo->timer.helloSeqno)
            {
                HsrpProcessEvent(node,
                                 info->interfaceIndex,
                                 NULL, HSRP_HELLO_TIMER_EXPIRY);
                HsrpStartHelloTimer(node, info->interfaceIndex);
            }
            else
            {
                if (HsrpDebugTimer(node))
                {
                    printf("    expired!\n");
                }
            }

            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_ActiveTimer:
        {
            info = (HsrpTimerInfo*) MESSAGE_ReturnInfo(msg);
            interfaceInfo = HsrpGetInterfaceInfo(node, info->interfaceIndex);

            if (HsrpDebugTimer(node))
            {
                printf("Node %d got ActiveTimer expiry at %s\n",
                       node->nodeId, clockStr);
                HsrpPrintState(node, interfaceInfo->state);
            }

            if (info->seqno == interfaceInfo->timer.activeSeqno)
            {
                HsrpProcessEvent(node,
                                 info->interfaceIndex,
                                 NULL,
                                 HSRP_ACTIVE_TIMER_EXPIRY);
            }
            else
            {
                if (HsrpDebugTimer(node))
                {
                    printf("    expired!\n");
                }
            }

            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_StandbyTimer:
        {
            info = (HsrpTimerInfo*) MESSAGE_ReturnInfo(msg);
            interfaceInfo = HsrpGetInterfaceInfo(node, info->interfaceIndex);

            if (HsrpDebugTimer(node))
            {
                printf("Node %d got StandbyTimer expiry at %s\n",
                       node->nodeId, clockStr);
                HsrpPrintState(node, interfaceInfo->state);
            }

            if (info->seqno == interfaceInfo->timer.standbySeqno)
            {
                HsrpProcessEvent(node,
                                 info->interfaceIndex,
                                 NULL,
                                 HSRP_STANDBY_TIMER_EXPIRY);
            }
            else
            {
                if (HsrpDebugTimer(node))
                {
                    printf("    expired!\n");
                }
            }

            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_FromTransport:
        {
            HsrpHandleDataFromTransport(node, msg);
            MESSAGE_Free(node, msg);
            break;
        }
        default:
            printf("HSRP: Got unknown event type %d\n", msg->eventType);
            assert(FALSE);
    }
}


// -------------------------------------------------------------------------
// Name: RoutingHsrpLayer
// Purpose: Handling all finalization needed by HSRP.
// Return: None.
// -------------------------------------------------------------------------
void
RoutingHsrpFinalize(Node* node, int interfaceIndex)
{
    HsrpInterfaceInfo* interfaceInfo;
    NodeAddress interfaceAddress;
    char buf[MAX_STRING_LENGTH];

    if (!NetworkIpIsHsrpEnabled(node, interfaceIndex))
    {
        return;
    }

    interfaceAddress = NetworkIpGetInterfaceAddress(node, interfaceIndex);
    interfaceInfo = HsrpGetInterfaceInfo(node, interfaceIndex);

    sprintf(buf, "Number of Hello messages sent = %d",
            interfaceInfo->stat.numHelloSent);
    IO_PrintStat(node, "Application", "HSRP", interfaceAddress, -1, buf);

    sprintf(buf, "Number of Coup messages sent = %d",
            interfaceInfo->stat.numCoupSent);
    IO_PrintStat(node, "Application", "HSRP", interfaceAddress, -1, buf);

    sprintf(buf, "Number of Resign messages sent = %d",
            interfaceInfo->stat.numResignSent);
    IO_PrintStat(node, "Application", "HSRP", interfaceAddress, -1, buf);

    sprintf(buf, "Number of Hello messages received = %d",
            interfaceInfo->stat.numHelloReceived);
    IO_PrintStat(node, "Application", "HSRP", interfaceAddress, -1, buf);

    sprintf(buf, "Number of Coup messages received = %d",
            interfaceInfo->stat.numCoupReceived);
    IO_PrintStat(node, "Application", "HSRP", interfaceAddress, -1, buf);

    sprintf(buf, "Number of Resign messages received = %d",
            interfaceInfo->stat.numResignReceived);
    IO_PrintStat(node, "Application", "HSRP", interfaceAddress, -1, buf);

    sprintf(buf, "Last state = %s",
            HsrpGetStateString(node, interfaceInfo->state));
    IO_PrintStat(node, "Application", "HSRP", interfaceAddress, -1, buf);
}

