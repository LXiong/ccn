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
// PROTOCOL :: MAC-SATELLITE-BENTPIPE
// LAYER :: MAC
// REFERENCES :: DVS-S
// COMMENTS ::
// **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "api.h"
#include "antenna.h"

#include "network_ip.h"
#include "phy_satellite_rsv.h"

#include "if_queue.h"

#ifdef PARALLEL
#include "parallel.h"
#endif

#if defined(DEBUG)
#undef DEBUG
#endif

#define DEBUG 0

#include "mac_satellite_bentpipe.h"

// Debugging variable for transmission timing diagrams.
static int globalTransmissionCount = 0;

static
bool AddressIsNodeBroadcast(Node* node, Address& addr)
{
    bool done;
    int i;
    bool isNodeBroadcast(false);

    if (addr.networkType != NETWORK_IPV4)
    {
        return false;
    }

    for (done = false, i = 0;
         !done && i < node->numberInterfaces;
         i++)
    {
        if (addr.interfaceAddr.ipv4 ==
            NetworkIpGetInterfaceBroadcastAddress(node, i))
        {
            isNodeBroadcast = true;
            done = true;
        }
    }

    return isNodeBroadcast;
}

static
bool AddressIsBroadcast(Address& addr)
{
    bool isBroadcast(false);
    const unsigned char ipv6_anydest[] =
    {
        0xff, 0x02, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff
    } ;

    switch(addr.networkType)
    {
        case NETWORK_IPV4:
            if (addr.interfaceAddr.ipv4 == ANY_DEST)
            {
                isBroadcast = true;
            }
        break;

        case NETWORK_IPV6:
        {
            int k;

            isBroadcast = true;
            for (k = 0; k < 16; k++) // see main.h
            {
                if (addr.interfaceAddr.ipv6.s6_addr[k]
                    != ipv6_anydest[k])
                {
                    isBroadcast = false;
                    break;
                }
            }
        }
        break;

        case NETWORK_ATM:
        case NETWORK_INVALID:
        default:
            ERROR_ReportError("The Satellite model has identified"
                              " a misconfiguration or error.  The"
                              " model does not support unspecified or"
                              " ATM E.164 addresses within the"
                              " network header.  Please confirm"
                              " this to be the case in the"
                              " current configuration file.  If"
                              " you believe this is an error,"
                              " please contact QualNet technical"
                              " support.  The simulation cannot"
                              " continue.");
    }

    return isBroadcast;
}

static
string AddressToString(Address& addr)
{
    char addressString[BUFSIZ];

    memset(addressString,
           0,
           BUFSIZ);

    switch(addr.networkType)
    {
        case NETWORK_IPV4:
        {
            NodeAddress na(addr.interfaceAddr.ipv4);
            IO_ConvertIpAddressToString(na, addressString);
        }
            break;
        case NETWORK_IPV6:
        {
            in6_addr na(addr.interfaceAddr.ipv6);
            IO_ConvertIpAddressToString(&na, addressString);
        }
            break;
        case NETWORK_ATM:
        case NETWORK_INVALID:
        default:
            ERROR_ReportError("The Satellite model has identified"
                              " a misconfiguration or error.  The"
                              " model does not support unspecified or"
                              " ATM E.164 addresses within the"
                              " network header.  Please confirm"
                              " this to be the case in the"
                              " current configuration file.  If"
                              " you believe this is an error,"
                              " please contact QualNet technical"
                              " support.  The simulation cannot"
                              " continue.");
    }

    return string(addressString);
}

// /**
// FUNCTION :: MacSatelliteBentpipeHandlePromiscuousMode
// LAYER :: MAC
// PURPOSE :: Process a frame if the local interface is presently
//            configured to be in promiscuous mode.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure.
// +        state : MacSatelliteBentpipeState : The local data structure
//          for the interface MAC state machine.
// +        prevHop : Address : The address of the previous hop
//          of the packet.
// +        dstAddr : Address : The address of the ultimate
//          destination of the packet.
// RETURN :: void : NULL
// **/
static
void MacSatelliteBentpipeHandlePromiscuousMode(Node *node,
                                           MacSatelliteBentpipeState *state,
                                           Message *frame,
                                           Address* prevHop,
                                           Address* dstAddr)
{
    if (dstAddr->networkType == NETWORK_IPV4)
    {
        MESSAGE_RemoveHeader(node,
                             frame,
                             sizeof(MacSatelliteBentpipeData),
                             TRACE_SATELLITE_BENTPIPE);

        MAC_SneakPeekAtMacPacket(node,
                                 state->macData->interfaceIndex,
                                 frame,
                                 prevHop->interfaceAddr.ipv4,
                                 dstAddr->interfaceAddr.ipv4);

        MESSAGE_AddHeader(node, frame,
                          sizeof(MacSatelliteBentpipeData),
                          TRACE_SATELLITE_BENTPIPE);
    }
}


// /**
// FUNCTION :: MacSatelliteBentpipeGetData
// LAYER :: MAC
// PURPOSE :: Return the MAC (generic) machine data for the provided
//            <node, interfaceIndex> pair.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure.
// +        macNum : int : The index of the interface desired.
// RETURN :: MacData* : A pointer to the generic state machine data.
// **/

static
MacData *MacSatelliteBentpipeGetData(Node *node,
                                     int macNum)
{
    return (MacData*)(node->macData[macNum]);
}

// /**
// FUNCTION :: MacSatelliteBentpipeGetState
// LAYER :: MAC
// PURPOSE :: Return the detailed state machine data for the provided
//            <node, interfaceIndex> pair.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure.
// +        macNum : int : The index of the interface desired.
// RETURN :: MacSatelliteBentpipeState* : A pointer to the state machine
//           data.
// **/

static
MacSatelliteBentpipeState *MacSatelliteBentpipeGetState(Node *node,
                                                        int macNum)
{
    MacData *macData = MacSatelliteBentpipeGetData(node,
                                                   macNum);
    return (MacSatelliteBentpipeState*)macData->macVar;
}

// /**
// FUNCTION :: MacSatelliteBentpipeTimerIsSet
// LAYER :: MAC
// PURPOSE :: Queries the state data to see if a timer is currently set
//            in the state machine.
// PARAMETERS ::
// +        msg : Message* : A pointer to a timer message
// RETURN :: BOOL : If the pointer is non-NULL then the routine will
//           return TRUE else it returns FALSE
// **/

static
BOOL MacSatelliteBentpipeTimerIsSet(Message *msg)
{
    BOOL isTimerSet = FALSE;

    if (msg != 0) {
        isTimerSet = TRUE;
    }

    return isTimerSet;
}

// /**
// FUNCTION :: MacSatelliteBentpipeSetStatet
// LAYER :: MAC
// PURPOSE :: Set the MAC-protocol-specific data pointer
// PARAMETERS ::
// +        node : Node* : The local node data structure
// +        macNum : int : The integer indentifier of the local
//          interface
// +        state : MacSatelliteBentpipeState* : A pointer to the memory
//          region that stores the protocol-specific state for the
//          satellite MAC protocol.
// RETURN :: void : NULL
// **/

static
void MacSatelliteBentpipeSetState(Node *node,
                                  int macNum,
                                  MacSatelliteBentpipeState *state)
{
    MacData *macData = MacSatelliteBentpipeGetData(node,
                                                   macNum);
    macData->macVar = (void*)state;
}


// /**
// FUNCTION :: MacSatelliteBentpipeGetTransmissionTimeForPacket
// LAYER :: MAC
// PURPOSE :: Returns the transmission time for a packet.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface.
// +        msg : Message* : A pointer to the message that will be sent
//          and requires a transmission time.
// RETURN :: clocktype : The integer amount of time, in fundamental
//           time quanta (currently nanoseconds) that the transmission will
//           require when passed to the PHY layer for signalling.
// **/

#define BITS_PER_BYTE (8)

static
clocktype MacSatelliteBentpipeGetTransmissionTimeForPacket(Node *node,
                                                           int interfaceIndex,
                                                           Message *msg)
{
    MacSatelliteBentpipeState *state
        = MacSatelliteBentpipeGetState(node,
                                       interfaceIndex);

    int dataRate = PHY_GetTxDataRate(node,
                                     state->macData->phyNumber);
    int hdrSize = BITS_PER_BYTE * sizeof(MacSatelliteBentpipeHeader);
    int payloadSize = BITS_PER_BYTE * MESSAGE_ReturnPacketSize(msg);
    int totalSize = hdrSize + payloadSize;

    clocktype transmissionTime
        = ((clocktype)totalSize * SECOND) / (clocktype)dataRate;

    return transmissionTime;
}

// /**
// FUNCTION :: MacSatelliteBentpipeDequeueAndEncapsulatePacket
// LAYER :: MAC
// PURPOSE :: This routine removes a packet from the network output queue
//            and encapsulates it for transmission.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface.
// RETURN :: Message* : The first available packet from the network layer
//           output queue is returned, already encapsulated with the
//           satellite MAC protocol headers.
// **/

static
Message *MacSatelliteBentpipeDequeueAndEncapsulatePacket(Node *node,
                                                         int interfaceIndex)
{
    MacSatelliteBentpipeState *state
        = MacSatelliteBentpipeGetState(node,
                                   interfaceIndex);
    Message* msg = NULL;
    // Address nextHopAddr;

    NodeAddress nextHopAddr;

    int networkType;
    TosType priority;

    MacSatelliteBentpipeHeader hdrs;
    MacSatelliteBentpipeHeader* hdr = &hdrs;

    MAC_OutputQueueDequeuePacket(node,
                                 interfaceIndex,
                                 &msg,
                                 &nextHopAddr,
                                 &networkType,
                                 &priority);

    Address ns_nextHopAddr;

    ns_nextHopAddr.networkType = NETWORK_IPV4;
    ns_nextHopAddr.interfaceAddr.ipv4 = nextHopAddr;

    if (msg == NULL)
    {
        ERROR_ReportError("Unexpected failure in "
                          "dequeuing output frame for transmission.");
    }

    int sduSize = MESSAGE_ReturnPacketSize(msg);

    MESSAGE_AddHeader(node,
                      msg,
                      sizeof(MacSatelliteBentpipeHeader),
                      TRACE_SATELLITE_BENTPIPE);

    hdr->type = MacSatelliteBentpipeDataPdu;

    // See notes in the corresponding structure definition.  To
    // properly accommodate transmissions on a bentpipe satellite, the
    // noise terms should add on the uplink and downlink.

    hdr->uplinkSnrValid = FALSE;
    hdr->uplinkSnr = -1.0;

    if (state->role == MacSatelliteBentpipeRoleSatellite)
    {
        hdr->isDownlinkPacket = TRUE;
    }
    else
    {
        hdr->isDownlinkPacket = FALSE;
    }

    hdr->size = sduSize + MacSatelliteBentpipeHeaderSize;

    memcpy((void*)&hdr->pdu.data.dstAddr,
           (void*)&ns_nextHopAddr,
           sizeof(Address));

    memcpy((void*)&hdr->pdu.data.srcAddr,
           (void*)&state->localAddr,
           sizeof(Address));

    hdr->pdu.data.prio = MacSatelliteBentpipeDefaultPriority;

    // This memcpy operation should occur whenever the byte alignment of
    // the operand returned from MESSAGE_ReturnPacket() is not
    // guaranteed.

    memcpy(MESSAGE_ReturnPacket(msg),
           hdr,
           sizeof(MacSatelliteBentpipeHeader));

    return msg;
}

// /**
// FUNCTION :: MacSatelliteBentpipeProcessUplinkPacket
// LAYER :: MAC
// PURPOSE :: This routine processes a frame that has been received on the
//            satellite uplink.  This routine is only invoked by the
//            node that is acting as the satellite.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface.
// +        msg : Message* : A pointer to the message data structure
//          representing the received packet from the uplink channel.
// RETURN :: void : NULL
// **/

static
void MacSatelliteBentpipeProcessUplinkPacket(Node *node,
                                             int interfaceIndex,
                                             Message *msg)
{
    MacSatelliteBentpipeState* state
    = MacSatelliteBentpipeGetState(node,
                                   interfaceIndex);
    MacSatelliteBentpipeHeader hdrs;
    MacSatelliteBentpipeHeader *hdr = &hdrs;
    int upLinkChannelIndex = 0;
    ListItem* listItem = NULL;
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr = NULL;
    clocktype now = node->getNodeTime();
    double simTime = (double)node->getNodeTime() / (double)SECOND;

    // See comments in the corresponding header file.  The use of
    // floats in the protocol transmission header (PCI) is preferable
    // because it reduces the risk of structure size inconsistencies among
    // platforms that QualNet supports.

    float fptrs;
    float* fptr = &fptrs;

    // This memcpy operation should occur at any time the operand, in this
    // case MESSAGE_ReturnPacket() does not guaranteed proper alignment of
    // its return value.

    memcpy(hdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacSatelliteBentpipeHeader));

    if (DEBUG) {
        string localAddrStr = AddressToString(state->localAddr);

        printf("%0.6e: Satellite[%d/%d:%s] receives uplink packet.\n",
               (double)now / (double)SECOND,
               node->nodeId,
               interfaceIndex,
               localAddrStr.c_str());
    }

    //this if block will not get executed as by default the variable
    //state->forwardToPayloadProcessor is set as FALSE
    if (state->forwardToPayloadProcessor == TRUE)
    {
        //
        // Payload processor processing code
        //
        Address lastHopAddr;

        memcpy((void*)&lastHopAddr,
               &hdr->pdu.data.srcAddr,
               sizeof(Address));
        upLinkChannelIndex = 0;

        memcpy(&upLinkChannelIndex,
               MESSAGE_ReturnInfo(msg, INFO_TYPE_PhyIndex),
               sizeof(int));

        listItem = state->upDownChannelInfoList->first;
        while (listItem)
        {
            upDownChannelPairPtr = (MacSatelliteBentpipeUpDownChannelInfo*)
                                   listItem->data;

            if (upDownChannelPairPtr->upChannelInfo->channelIndex ==
                upLinkChannelIndex)
            {
                Address dstAddr = hdr->pdu.data.dstAddr;

                if (AddressIsBroadcast(dstAddr))
                {
                    upDownChannelPairPtr->upChannelInfo->stats.broadcastPktsRecd++;
                }
                else
                {
                    upDownChannelPairPtr->upChannelInfo->stats.unicastPktsRecd++;
                }
                break;
            }
            else
            {
                listItem = listItem->next;
            }
        }
        MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(MacSatelliteBentpipeHeader),
                             TRACE_SATELLITE_BENTPIPE);

        MAC_HandOffSuccessfullyReceivedPacket(node,
                                              interfaceIndex,
                                              msg,
                                              lastHopAddr.interfaceAddr.ipv4);
    }
    else
    {
        //
        // Begin bent-pipe processing code
        //
        if (MESSAGE_ReturnInfo(msg) != 0) {

            // This memcpy operation should occur at any time the operand,
            // in this case MESSAGE_ReturnInfo() does not guaranteed proper
            // alignment of its return value. If alignment is not
            // guaranteed, then dereferencing the pointer directly will fail.

            memcpy(fptr, MESSAGE_ReturnInfo(msg), sizeof(float));

            hdr->uplinkSnr = *fptr;
            hdr->uplinkSnrValid = TRUE;
        }
        else {
            hdr->uplinkSnrValid = FALSE;
        }

        //
        // Prepare the packet for downlink transmission.
        //

        hdr->isDownlinkPacket = TRUE;
        memcpy(MESSAGE_ReturnPacket(msg),
               hdr,
               sizeof(MacSatelliteBentpipeHeader));

        PHY_SetTransmitPower(node,
                             state->macData->phyNumber,
                             state->transmitPowermW);
        if (DEBUG) {
            printf("ProcessUplink, %d, %d, %d, %lf\n",
                   globalTransmissionCount,
                   node->nodeId,
                   interfaceIndex,
                   simTime);

            globalTransmissionCount++;
        }

        memcpy(&upLinkChannelIndex,
               MESSAGE_ReturnInfo(msg, INFO_TYPE_PhyIndex),
               sizeof(int));

        //As it is a satellite node,hence the signal must be sent on every
        //downlink channel except the corresponding downlink channel of
        //“upLinkChannelIndex”.Therefore, add the message to the message
        //queue of required downlink channels using the function
        //“MacSatelliteBentpipeInsertInChannelQueue()”

        listItem = state->upDownChannelInfoList->first;
        while (listItem)
        {
            upDownChannelPairPtr = (MacSatelliteBentpipeUpDownChannelInfo*)
                listItem->data;

            if (upDownChannelPairPtr->upChannelInfo->channelIndex ==
                    upLinkChannelIndex)
            {
                Address dstAddr = hdr->pdu.data.dstAddr;

                if (AddressIsBroadcast(dstAddr))
                {
                    upDownChannelPairPtr->upChannelInfo->stats.broadcastPktsRecd++;
                }
                else
                {
                    upDownChannelPairPtr->upChannelInfo->stats.unicastPktsRecd++;
                }

                listItem = listItem->next;
                continue;
            }

            MacSatelliteBentpipeInsertInChannelQueue(
                node,
                state,
                upDownChannelPairPtr->downChannelInfo->channelIndex,
                MESSAGE_Duplicate(node, msg));

            MacSatelliteBentpipeChannelHasPacketToSend(
                node,
                state,
                upDownChannelPairPtr->downChannelInfo->channelIndex);

            listItem = listItem->next;
        }//end of while
        MESSAGE_Free(node, msg);
    }//end of else
}

// /**
// FUNCTION :: MacSatelliteBentpipeProcessDownlinkPacket
// LAYER :: MAC
// PURPOSE :: This routine processes a frame that has been received on the
//            satellite downlink.  This routine is only invoked by the
//            nodes acting as ground stations.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface.
// +        msg : Message* : A pointer to the message data structure
//          representing the received packet from the downlink channel.
// RETURN :: void : NULL
// **/

static
void MacSatelliteBentpipeProcessDownlinkPacket(Node *node,
                                               int interfaceIndex,
                                               Message *msg)
{
    MacSatelliteBentpipeState* state
    = MacSatelliteBentpipeGetState(node,
                                   interfaceIndex);
    MacSatelliteBentpipeHeader hdrs;
    MacSatelliteBentpipeHeader* hdr = &hdrs;

    BOOL isUnicast = FALSE;
    BOOL isBroadcast = FALSE;


    // This memcpy operation should occur at any time the operand, in this
    // case MESSAGE_ReturnPacket() does not guaranteed proper alignment of
    // its return value.

    memcpy(hdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacSatelliteBentpipeHeader));

    int channelId = 0;

    memcpy(&channelId,
           MESSAGE_ReturnInfo(msg, INFO_TYPE_PhyIndex),
           sizeof(int));

    if (DEBUG) {
        clocktype now = node->getNodeTime();

        char addrstr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&(state->localAddr),
                                    addrstr);

        printf("%0.6e: Ground Station[%d/%d:%s] receives"
               " downlink packet.\n",
               (double)now / (double)SECOND,
               node->nodeId,
               interfaceIndex,
               addrstr);

    }

    Address dstAddr;
    memcpy((void*)&dstAddr,
           (void*)&hdr->pdu.data.dstAddr,
           sizeof(Address));

    Address lastHopAddr;
    memcpy((void*)&lastHopAddr,
           (void*)&hdr->pdu.data.srcAddr,
           sizeof(Address));

    MacSatelliteBentpipeChannel* channelInfo =
        MacSatelliteBentpipeGetChannelInfo(
            node,
            state,
            channelId);

    if (AddressIsBroadcast(dstAddr))
    {
        //
        // Is a broadcast packet
        //
        channelInfo->stats.broadcastPktsRecd++;
        isBroadcast = TRUE;
    }
    else if (MAC_IsMyUnicastFrame(node,
                                  dstAddr.interfaceAddr.ipv4) == TRUE)
    {
        //
        // Is a unicast packet destined for this node/interface pair
        //
        channelInfo->stats.unicastPktsRecd++;
        isUnicast = TRUE;

        if (DEBUG)
        {
            char addrstr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&(state->localAddr),
                                        addrstr);

            printf("Node[%d/%d:%s] accepts unicast packet.\n",
                   node->nodeId,
                   interfaceIndex,
                   addrstr);
        }
    }
    else
    {
        //
        // Is a broadcast packet destined for any interface on this node
        //

        bool isNodeBroadcast =
            AddressIsNodeBroadcast(node, dstAddr);

        if (isNodeBroadcast)
        {
            isBroadcast = TRUE;
            channelInfo->stats.broadcastPktsRecd++;
        }
    }

    if (isBroadcast || isUnicast) {
        //
        // This packet should be accepted by this interface.
        //

        MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(MacSatelliteBentpipeHeader),
                             TRACE_SATELLITE_BENTPIPE);

        ERROR_Assert(lastHopAddr.networkType == NETWORK_IPV4,
                     "The Satellite model has detected that"
                     " an IPv6 address has inadvertently become"
                     " active in the network.  QualNet presently"
                     " uses a IPv4/IPv6 mapping mechanism to"
                     " simplify model migration.  This should"
                     " ensure that no IPv6 addresses become"
                     " active in the simulation.  The programming"
                     " logic should be checked for errors.  Please"
                     " report this to QualNet technical support"
                     " for further action.  The simulation cannot"
                     " continue.");

        MAC_HandOffSuccessfullyReceivedPacket(node,
                                          interfaceIndex,
                                          msg,
                                          lastHopAddr.interfaceAddr.ipv4);

        // At this point, the protocol has finished processing the
        // packet.  However, it has been given to another layer
        // permanently and so should not be freed.

    }
    else
    {

        // If the packet is not destined for this node, it may be still
        // be of some use for a protocol residing on the node.  Query the
        // node state to see if the interface has been set in promiscuous
        // node.  If so, call the appropriate routine to handle processing
        // of promiscuous mode packet receptions.

        if (node->macData[interfaceIndex]->promiscuousMode == TRUE) {
            MacSatelliteBentpipeHandlePromiscuousMode(node,
                                                      state,
                                                      msg,
                                                      &lastHopAddr,
                                                      &dstAddr);
        }

        // At this point, the protocol has finished processing the
        // packet.  Likewise, it is assumed that any action taken by the
        // node completed synchronously with the previous function call.
        //
        // Therefore the packet can be safely freed without any unintended
        // effects.

        MESSAGE_Free(node, msg);
    }
}

// /**
// FUNCTION :: MacSatelliteBentpipeInit
// LAYER :: MAC
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            initialize a <node,interface> pair for use with the satellite
//            MAC protocol.  This routine is used by other routines outside
//            of this file.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface.
// +        interfaceAddress : NodeAddress : An IPv4 address representing
//          the address of the local interface
// +        nodeInput : NodeInput* : A pointer to the data structure that
//          contains the QualNet configuration file (and auxillary files)
//          information deserialized into memory.
// +        nodesInSubnet : int : The count of <node,interface> pairs that
//          are in the local subnet.
// RETURN :: void : NULL
// **/

void MacSatelliteBentpipeInit(Node *node,
                              int interfaceIndex,
                              NodeAddress p_interfaceAddress,
                              NodeInput const *nodeInput,
                              int nodesInSubnet)
{
    int i = 0;
    int numChannels = PROP_NumberChannels(node);
    BOOL isListeningToChannel = FALSE;

    Address interfaceAddress;
    interfaceAddress.networkType = NETWORK_IPV4;
    interfaceAddress.interfaceAddr.ipv4 =
                    MAC_VariableHWAddressToFourByteMacAddress(node,
                               &node->macData[interfaceIndex]->macHWAddr);

    BOOL wasFound = FALSE;
    char buf[1024];

    MacSatelliteBentpipeState *state = (MacSatelliteBentpipeState*)
        MEM_malloc(sizeof(MacSatelliteBentpipeState));

    // Compiler and library treatment of malloc varies from platform to
    // platform.  Some platforms return zero'd memory at all times and
    // others do not.  Some do so depending on the state of the memory
    // page (i.e. if the page was previuosly considered zero mapped).  To
    // be safe, the programmer shhould clear the memory before using it.

    memset(state,
           0,
           sizeof(MacSatelliteBentpipeState));

    MacSatelliteBentpipeSetState(node,
                                 interfaceIndex,
                                 state);

    state->macData = MacSatelliteBentpipeGetData(node,
                                                 interfaceIndex);

    memcpy((void*)&(state->localAddr),
           (void*)&interfaceAddress,
           sizeof(Address));

#ifdef PARALLEL // Parallel

    // These two statements are required to obtain identical results on
    // parallel platforms as sequential.  The minimum lookahead is presently
    // set to zero, which is conservative.

    PARALLEL_SetProtocolIsNotEOTCapable(node);
    PARALLEL_SetMinimumLookaheadForInterface(node,
                                             0);
#endif // endParallel

    IO_ReadString(node, node->nodeId, interfaceIndex,
                  nodeInput,
                  "MAC-SATELLITE-BENTPIPE-ROLE",
                  &wasFound,
                  buf);

    if (wasFound == FALSE) {
        state->role = MacSatelliteBentpipeDefaultRole;
    }
    else if (strcmp(buf, "GROUND-STATION") == 0) {
        state->role = MacSatelliteBentpipeRoleGroundStation;
    }
    else if (strcmp(buf, "SATELLITE") == 0) {
        state->role = MacSatelliteBentpipeRoleSatellite;
    }
    else {
        ERROR_ReportError("Unknown MAC role.");
    }

    if (DEBUG) {
        const char* role = NULL;

        if (state->role == MacSatelliteBentpipeRoleGroundStation) {
            role = "ground station";
        }
        else {
            role = "satellite";
        }

        string addrStr = AddressToString(state->localAddr);

        printf("Node[%d.%d] [addr=%s] role=\"%s\"\n", node->nodeId,
               interfaceIndex,
               addrStr.c_str(),
               role);
    }

    int listeningChannelCount = 0;
    for (i = 0; i < numChannels; i++)
    {
        isListeningToChannel =
            PHY_IsListeningToChannel(node,
                                     state->macData->phyNumber,
                                     i);
   
        if (isListeningToChannel == TRUE)
        {
            listeningChannelCount++;
        }
    }

    if (state->role == MacSatelliteBentpipeRoleGroundStation)
    {
        ERROR_Assert(listeningChannelCount >= 1,
                     "At least one channel should be set to "
                     "listening at ground station. This configuration has changed,"
                     "Please refer to the upgrade directions in the user manual for"
                     "correct configuration in the documentation folder.");
    }
    else
    {
        ERROR_Assert(listeningChannelCount >= 2,
                     "At least two channels should be set to "
                     "listening at satellite. This configuration has changed,"
                     "Please refer to the upgrade directions in the user manual for"
                     "correct configuration  in the documentation folder.");
    }

    BOOL forwardToProcessor;
    IO_ReadBool(node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "MAC-SATELLITE-BENTPIPE-FORWARD-TO-PAYLOAD-PROCESSOR",
                &wasFound,
                &forwardToProcessor);
    if (wasFound == FALSE) 
    {
        state->forwardToPayloadProcessor 
            = MacSatelliteBentpipeDefaultForwardToPayloadProcessor;
    }
    else if (forwardToProcessor)
    {
        state->forwardToPayloadProcessor = TRUE;
    }
    else
    {
        state->forwardToPayloadProcessor = FALSE;
    }

    IO_ReadDouble(node, node->nodeId, interfaceIndex,
                  nodeInput,
                  "MAC-SATELLITE-BENTPIPE-TRANSMIT-POWER-MW",
                  &wasFound,
                  &state->transmitPowermW);

    if (wasFound == FALSE)
    {
        state->transmitPowermW
            = MacSatelliteBentpipeDefaultTransmitPowermW;
    }

    MacSatelliteBentpipeReadUpDownLinkInfo(
        node,
        interfaceIndex,
        nodeInput);
}

// /**
// FUNCTION :: MacSatelliteBentpipeNetworkLayerHasPacketToSend
// LAYER :: MAC
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            indicate that the local network layer output queue has
//            a packet ready to be transmitted on the local subnet.  This
//            routine is called by other routines outside of this file.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the memory
//          region containing satellite MAC protocol-specific data.
// RETURN :: void : NULL
// **/


void MacSatelliteBentpipeNetworkLayerHasPacketToSend(
    Node *node,
    MacSatelliteBentpipeState *state)
{
    int interfaceIndex = state->macData->interfaceIndex;
    double simTime = (double)node->getNodeTime() / (double)SECOND;
    Message *msg = NULL;

    ListItem* listItem = NULL;
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr = NULL;

    if (DEBUG) {
        printf("%0.6e: Node[%d] has packet to send on interface %d.\n",
               simTime, node->nodeId, state->macData->interfaceIndex);
    }

    // The operation of the PHY in this state is as follows:
    //
    // Present State : Future State : Action
    // PHY_IDLE : PHY_TRANSMITTING : Begin transmitting packet
    // immediately
    // Default : Unmodified : Take no further action
    //

    msg = MacSatelliteBentpipeDequeueAndEncapsulatePacket(
                node,
                interfaceIndex);

    if (msg == NULL)
    {
        ERROR_ReportError("Unexpected NULL packet in network dequeue");
    }

    MacSatelliteBentpipeHeader mac_hdr;

    memcpy((void*)&mac_hdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacSatelliteBentpipeHeader));

    PHY_SetTransmitPower(node,
                         state->macData->phyNumber,
                         state->transmitPowermW);

    if (DEBUG) {
        printf("PacketRts, %d, %d, %d, %d[%d], %lf\n",
               globalTransmissionCount,
               node->nodeId,
               interfaceIndex,
               mac_hdr.size,
               MESSAGE_ReturnPacketSize(msg),
               simTime);

        globalTransmissionCount++;
    }

    if (state->role == MacSatelliteBentpipeRoleGroundStation)
    {
        //As the packet is coming from the upper layer so it must be added
        //in the uplink channel queue

        MacSatelliteBentpipeChannel* upChannelInfo =
            MacSatelliteBentpipeGetUpLinkChannelInfoGS(
                node,
                state);

        //add the packet to the channel queue
        MacSatelliteBentpipeInsertInChannelQueue(
            node,
            state,
            upChannelInfo->channelIndex,
            MESSAGE_Duplicate(node, msg));

        MacSatelliteBentpipeChannelHasPacketToSend(
            node,
            state,
            upChannelInfo->channelIndex);
    }
    else // satellite
    {
        if (state->forwardToPayloadProcessor == FALSE)
        {
            //Asserting at this condition as the packet must not come from upper
            //layer at satellite node

            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,"Warning:Satellite node %d : It Should not receive packet"
                       "from network layer",node->nodeId);
            ERROR_ReportWarning(errStr);
        }
        else
        {
            // As it is a satellite node,hence the signal must be sent on every
            // downlink channel.Therefore, add the message to the message
            // queue of required downlink channels using the function
            // “MacSatelliteBentpipeInsertInChannelQueue()”

            listItem = state->upDownChannelInfoList->first;
            while (listItem)
            {
                upDownChannelPairPtr = (MacSatelliteBentpipeUpDownChannelInfo*)
                    listItem->data;

                MacSatelliteBentpipeInsertInChannelQueue(
                    node,
                    state,
                    upDownChannelPairPtr->downChannelInfo->channelIndex,
                    MESSAGE_Duplicate(node, msg));

                MacSatelliteBentpipeChannelHasPacketToSend(
                    node,
                    state,
                    upDownChannelPairPtr->downChannelInfo->channelIndex);

                listItem = listItem->next;
            }// end of while
        }// end of forwardToPayloadProcessor
    }//end of else

    MESSAGE_Free(node, msg);
}



// /**
// FUNCTION :: MacSatelliteBentpipeProcessEvent
// LAYER :: MAC
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            process an event in the control path of the protocol.
//            Presently this protocol does not create or accept any
//            such events. This routine is invoked by other modules outside
//            of this file.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface starting at 0.
// +        msg : Message* : A pointer to the message data structure
//          associated with the event.
// RETURN :: void : NULL
// **/

void MacSatelliteBentpipeProcessEvent(Node *node,
                                      int interfaceIndex,
                                      Message *msg)
{
    ERROR_ReportError("This MAC process does not generate any internal "
                      "events.");
}

// /**
// FUNCTION :: MacSatelliteBentpipeReceivePacketFromPhy
// LAYER :: MAC
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            process the reception of a packet in its entirety by
//            the physical layer.  This routine is invoked by other modules
//            outside of this file.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        msg : Message* : A pointer to the message data structure
//          representing the packet received by the PHY.
// RETURN :: void : NULL
// **/

void MacSatelliteBentpipeReceivePacketFromPhy(Node *node,
                                              MacSatelliteBentpipeState *state,
                                              Message *msg)
{
    int interfaceIndex = state->macData->interfaceIndex;

    MacSatelliteBentpipeHeader hdrs;
    MacSatelliteBentpipeHeader* hdr = &hdrs;

    if (DEBUG) {
        string localAddrStr = AddressToString(state->localAddr);

        printf("%0.6e: Node[%d/%d:%s] receives message %p from PHY.\n",
               (double)node->getNodeTime() / (double)SECOND,
               node->nodeId,
               interfaceIndex,
               localAddrStr.c_str(),
               msg);
    }

    // This memcpy operation should occur at any time the operand, in this
    // case MESSAGE_ReturnPacket() does not guaranteed proper alignment of
    // its return value.

    memcpy(hdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacSatelliteBentpipeHeader));

    // The header will specify which this is an uplink or downlink packet.
    // There are other ways to ascertain this information that could be used
    // to verify that no programmatic error has occured. However, this is
    // suitable for normal operation in which the model has been verified
    // for proper operation during component and system test.

    if (hdr->isDownlinkPacket == FALSE) {
        if (state->role == MacSatelliteBentpipeRoleSatellite) {
            MacSatelliteBentpipeProcessUplinkPacket(node,
                                                    interfaceIndex,
                                                    msg);
        }
    }
    else {
        if (state->role != MacSatelliteBentpipeRoleSatellite) {
            MacSatelliteBentpipeProcessDownlinkPacket(node,
                                                      interfaceIndex,
                                                      msg);
        }
    }
}

// /**
// FUNCTION :: MacSatelliteBentpipePhyStatusChangeNotification
// LAYER :: MAC
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            inform the local MAC process that the PHY layer has reported
//            a change in its state.  This routine is invoked by other
//            modules outside of this file.  The MAC process can take
//            action as a result of the status change but cannot veto
//            the action directly.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        oldPhyStatus : PhyStatusType : An enumerated value holding the
//          status of the PHY process _before_ the change in state occured.
// +        newPhyStatus : PhyStatusType : An enumerated value holding the
//          status of the PHY process _after_ the change in state occured.
// RETURN :: void : NULL
// **/

void
MacSatelliteBentpipeReceivePhyStatusChangeNotification(
    Node *node,
    MacSatelliteBentpipeState *state,
    PhyStatusType oldPhyStatus,
    PhyStatusType newPhyStatus)
{
//    int interfaceIndex = state->macData->interfaceIndex;
//    double simTime = (double)node->getNodeTime() / (double)SECOND;
//
//    if (DEBUG)
//    {
//        printf("%0.6e: Node[%d] receives phy status change from %s"
//               " to %s.\n",
//               simTime,
//               node->nodeId,
//               PHY_PhyStatusTypeToString(oldPhyStatus),
//               PHY_PhyStatusTypeToString(newPhyStatus));
//    }
//
//    if (oldPhyStatus != newPhyStatus)
//    {
//        state->phyStatus = newPhyStatus;
//    }
//
//    // If the new status of the interface has become idle, then the
//    // MAC process immediately shifts to PHY_TRANSMITTING and sends out
//    // a packet removed from the head of the network layer output queue for
//    // this interface.
//
//    if (newPhyStatus == PHY_IDLE)
//    {
//        if (MAC_OutputQueueIsEmpty(node, interfaceIndex) == FALSE)
//        {
//            Message* msg =
//                MacSatelliteBentpipeDequeueAndEncapsulatePacket(node,
//                                                                interfaceIndex);
//
//            if (msg == NULL)
//            {
//                ERROR_ReportError("Unexpected NULL packet in network dequeue");
//            }
//
//            MacSatelliteBentpipeHeader mac_hdr;
//
//            memcpy((void*)&mac_hdr,
//                   MESSAGE_ReturnPacket(msg),
//                   sizeof(MacSatelliteBentpipeHeader));
//
//            PHY_SetTransmitPower(node,
//                                 state->macData->phyNumber,
//                                 state->transmitPowermW);
//            if (DEBUG) {
//                printf("PhyStatusChange, %d, %d, %d, %lf\n",
//                       globalTransmissionCount,
//                       node->nodeId,
//                       interfaceIndex,
//                       simTime);
//
//                globalTransmissionCount++;
//            }
//
//            PHY_StartTransmittingSignal(node,
//                                        state->macData->phyNumber,
//                                        msg,
//                                        FALSE,
//                                        0);
//        }
//    }
}

 /**
 FUNCTION :: MacSatelliteBentpipeRunTimeStat
 LAYER :: MAC
 PURPOSE :: This routine is called by the MAC layer dispatch code to
            report the current statistics of the layer via the
            IO_ utility library.  It is usually only called at the
            end of the simulation but may be called at any point during
            the simulation itself.  This routine is called by other
            functions outside of this file.
 PARAMETERS ::
 +        node : Node* : A pointer to the local node data structure
 +        interfaceIndex : int : The integer index of the interface
          associated with the local MAC process.
 RETURN :: void : NULL
 **/

void MacSatelliteBentpipeRunTimeStat(Node *node,
                                     int interfaceIndex)
{
    MacSatelliteBentpipeState* state
        = MacSatelliteBentpipeGetState(node,
                                       interfaceIndex);
    char buf[BUFSIZ];

    ListItem* listItem = state->upDownChannelInfoList->first;
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr = NULL;

    while (listItem)
    {
        upDownChannelPairPtr = (MacSatelliteBentpipeUpDownChannelInfo*)
            listItem->data;

        sprintf(buf,
            "UNICAST packets sent to the channel[up channel index: %d] = %d",
            upDownChannelPairPtr->upChannelInfo->channelIndex,
            upDownChannelPairPtr->upChannelInfo->stats.unicastPktsSent);
        IO_PrintStat(node,
                     "Mac",
                     "SATELLITE_BENTPIPE",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf,
            "UNICAST packets sent to the channel[down channel index: %d] = %d",
            upDownChannelPairPtr->downChannelInfo->channelIndex,
            upDownChannelPairPtr->downChannelInfo->stats.unicastPktsSent);
        IO_PrintStat(node,
                     "Mac",
                     "SATELLITE_BENTPIPE",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf,
            "BROADCAST packets sent to the channel[up channel index: %d] = %d",
            upDownChannelPairPtr->upChannelInfo->channelIndex,
            upDownChannelPairPtr->upChannelInfo->stats.broadcastPktsSent);
        IO_PrintStat(node,
                     "Mac",
                     "SATELLITE_BENTPIPE",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf,
            "BROADCAST packets sent to the channel[down channel index: %d] = %d",
            upDownChannelPairPtr->downChannelInfo->channelIndex,
            upDownChannelPairPtr->downChannelInfo->stats.broadcastPktsSent);
        IO_PrintStat(node,
                     "Mac",
                     "SATELLITE_BENTPIPE",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf,
            "UNICAST packets received from channel[up channel index: %d] = %d",
            upDownChannelPairPtr->upChannelInfo->channelIndex,
            upDownChannelPairPtr->upChannelInfo->stats.unicastPktsRecd);
        IO_PrintStat(node,
                     "Mac",
                     "SATELLITE_BENTPIPE",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf,
            "UNICAST packets received from channel[down channel index: %d] = %d",
            upDownChannelPairPtr->downChannelInfo->channelIndex,
            upDownChannelPairPtr->downChannelInfo->stats.unicastPktsRecd);
        IO_PrintStat(node,
                     "Mac",
                     "SATELLITE_BENTPIPE",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf,
            "BROADCAST packets received from channel[up channel index: %d] = %d",
            upDownChannelPairPtr->upChannelInfo->channelIndex,
            upDownChannelPairPtr->upChannelInfo->stats.broadcastPktsRecd);
        IO_PrintStat(node,
                     "Mac",
                     "SATELLITE_BENTPIPE",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf,
            "BROADCAST packets received from channel[down channel index: %d] = %d",
            upDownChannelPairPtr->downChannelInfo->channelIndex,
            upDownChannelPairPtr->downChannelInfo->stats.broadcastPktsRecd);
        IO_PrintStat(node,
                     "Mac",
                     "SATELLITE_BENTPIPE",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        listItem = listItem->next;
    }//end of while
}

// /**
// FUNCTION :: MacSatelliteBentpipeFinalize
// LAYER :: MAC
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            request that the current MAC process cease processing
//            and release any resources currently allocated to it.  This
//            normally occurs at the end of the simulation but in the
//            future
//            may be called whenever the current MAC operation is
//            dynamically terminated.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the interface
//          associated with the local MAC process.
// RETURN :: void : NULL
// **/

void MacSatelliteBentpipeFinalize(Node *node,
                                  int interfaceIndex)
{
    MacSatelliteBentpipeState* state
        = MacSatelliteBentpipeGetState(node,
                                       interfaceIndex);

    // Currently the only action taken in this function is to print
    // out the run-time statistics via the IO_ library.

    if (state->macData->macStats == TRUE) {
        MacSatelliteBentpipeRunTimeStat(node,
                                        interfaceIndex);
    }
}

// /**
// FUNCTION :: MacSatelliteBentpipeReadUpDownLinkInfo
// LAYER :: MAC
// PURPOSE :: This routine is called by the MAC-SATELLITE-BENTPIPE
//            init to read the uplink and downlink channel configurations
//            at ground station and satellite node
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface.
// +        nodeInput : NodeInput* : A pointer to the data structure that
//          contains the QualNet configuration file (and auxillary files)
//          information deserialized into memory.
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeReadUpDownLinkInfo(
    Node*node,
    int interfaceIndex,
    NodeInput const *nodeInput)
{
    MacSatelliteBentpipeState* state
        = MacSatelliteBentpipeGetState(node,
                                       interfaceIndex);

    Int32 upLinkChannelId = INVALID_CHANNEL_ID;
    Int32 downLinkChannelId = INVALID_CHANNEL_ID;
    BOOL upLinkFound = FALSE;
    BOOL downLinkFound = FALSE;
    char string[MAX_STRING_LENGTH];
    Int32 channelIndex = INVALID_CHANNEL_ID;

    if (state->role == MacSatelliteBentpipeRoleGroundStation)
    {
        IO_ReadString(node,
                      node->nodeId,
                      interfaceIndex,
                      nodeInput,
                      "MAC-SATELLITE-USE-UPLINK-CHANNEL",
                      &upLinkFound,
                      string);

        if (upLinkFound)
        {
            if (IO_IsStringNonNegativeInteger(string))
            {
                channelIndex = (Int32)strtoul(string, NULL, 10);

                if (channelIndex < node->numberChannels)
                {
                    upLinkChannelId = channelIndex;
                }
                else
                {
                    ERROR_ReportErrorArgs("MAC-SATELLITE-USE-UPLINK-CHANNEL "
                                          "parameter should be an integer"
                                          " between 0 and %d or a valid "
                                          "channel name.",
                                          node->numberChannels - 1);
                }
            }
            else if (isalpha(*string) && PHY_ChannelNameExists(node,
                                                               string))
            {
                upLinkChannelId = PHY_GetChannelIndexForChannelName(
                                        node,
                                        string);
            }
            else
            {
                ERROR_ReportErrorArgs("MAC-SATELLITE-USE-UPLINK-CHANNEL "
                                      "parameter should be an integer "
                                      "between 0 and %d or a valid "
                                      "channel name.",
                                      node->numberChannels - 1);
            }
        }

        IO_ReadString(node,
                      node->nodeId,
                      interfaceIndex,
                      nodeInput,
                      "MAC-SATELLITE-USE-DOWNLINK-CHANNEL",
                      &downLinkFound,
                      string);

        if (downLinkFound)
        {
            if (IO_IsStringNonNegativeInteger(string))
            {
                channelIndex = (Int32)strtoul(string, NULL, 10);

                if (channelIndex < node->numberChannels)
                {
                    downLinkChannelId = channelIndex;
                }
                else
                {
                    ERROR_ReportErrorArgs("MAC-SATELLITE-USE-DOWNLINK-CHANNEL "
                                          "parameter should be an integer"
                                          " between 0 and %d or a valid "
                                          "channel name.",
                                          node->numberChannels - 1);
                }
            }
            else if (isalpha(*string) && PHY_ChannelNameExists(node, string))
            {
                downLinkChannelId = PHY_GetChannelIndexForChannelName(
                                            node,
                                            string);
            }
            else
            {
                ERROR_ReportErrorArgs("MAC-SATELLITE-USE-DOWNLINK-CHANNEL "
                                      "parameter should be an integer "
                                      "between 0 and %d or a valid "
                                      "channel name.",
                                      node->numberChannels - 1);
            }
        }

        if (upLinkFound || downLinkFound)
        {
            MacSatelliteBentpipeAddUpDownLinkChannel(
                node,
                state->macData->phyNumber,
                state,
                interfaceIndex,
                upLinkChannelId,
                downLinkChannelId);
        }
    }
    else        //satellite
    {
        BOOL wasFound = FALSE;
        int channelCount = 0;

        IO_ReadInt(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-SATELLITE-CHANNEL-COUNT",
                  &wasFound,
                  &channelCount);

        if (wasFound && channelCount < 0)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr , "Node %u : MAC-SATELLITE-CHANNEL-COUNT "
                             "cannot be < 0", node->nodeId);
            ERROR_Assert(FALSE , errStr);
        }

        if (channelCount == 0)
        {
            IO_ReadString(node,
                          node->nodeId,
                          interfaceIndex,
                          nodeInput,
                          "MAC-SATELLITE-UPLINK-CHANNEL",
                          &upLinkFound,
                          string);

            if (upLinkFound)
            {
                if (IO_IsStringNonNegativeInteger(string))
                {
                    channelIndex = (Int32)strtoul(string, NULL, 10);

                    if (channelIndex < node->numberChannels)
                    {
                        upLinkChannelId = channelIndex;
                    }
                    else
                    {
                        ERROR_ReportErrorArgs("MAC-SATELLITE-UPLINK-CHANNEL "
                                              "parameter should be an integer"
                                              " between 0 and %d or a valid "
                                              "channel name.",
                                              node->numberChannels - 1);
                    }
                }
                else if (isalpha(*string) && PHY_ChannelNameExists(node,
                                                                   string))
                {
                    upLinkChannelId = PHY_GetChannelIndexForChannelName(
                                            node,
                                            string);
                }
                else
                {
                    ERROR_ReportErrorArgs("MAC-SATELLITE-UPLINK-CHANNEL "
                                          "parameter should be an integer "
                                          "between 0 and %d or a valid "
                                          "channel name.",
                                          node->numberChannels - 1);
                }
            }

            IO_ReadString(node,
                          node->nodeId,
                          interfaceIndex,
                          nodeInput,
                          "MAC-SATELLITE-DOWNLINK-CHANNEL",
                          &downLinkFound,
                          string);

            if (downLinkFound)
            {
                if (IO_IsStringNonNegativeInteger(string))
                {
                    channelIndex = (Int32)strtoul(string, NULL, 10);

                    if (channelIndex < node->numberChannels)
                    {
                        downLinkChannelId = channelIndex;
                    }
                    else
                    {
                        ERROR_ReportErrorArgs("MAC-SATELLITE-DOWNLINK-CHANNEL "
                                              "parameter should be an integer"
                                              " between 0 and %d or a valid "
                                              "channel name.",
                                              node->numberChannels - 1);
                    }
                }
                else if (isalpha(*string) && PHY_ChannelNameExists(node, string))
                {
                    downLinkChannelId = PHY_GetChannelIndexForChannelName(
                                                node,
                                                string);
                }
                else
                {
                    ERROR_ReportErrorArgs("MAC-SATELLITE-DOWNLINK-CHANNEL "
                                          "parameter should be an integer "
                                          "between 0 and %d or a valid "
                                          "channel name.",
                                          node->numberChannels - 1);
                }
            }

            if (upLinkFound && downLinkFound)
            {
                if (upLinkChannelId == downLinkChannelId)
                {
                    char errStr[MAX_STRING_LENGTH];
                    sprintf(errStr , "Node %u : Currently it is not "
                                     "supported that "
                                     "MAC-SATELLITE-UPLINK-CHANNEL is equal "
                                     "to MAC-SATELLITE-DOWNLINK-CHANNEL",
                                     node->nodeId);
                    ERROR_Assert(FALSE , errStr);
                }

                MacSatelliteBentpipeAddUpDownLinkChannel(
                    node,
                    state->macData->phyNumber,
                    state,
                    interfaceIndex,
                    upLinkChannelId,
                    downLinkChannelId);
            }

            if (upLinkFound == FALSE && downLinkFound == FALSE)
            {
                MacSatelliteBentpipeAddUpDownLinkChannel(
                    node,
                    state->macData->phyNumber,
                    state,
                    interfaceIndex,
                    0,//upLinkChannelId
                    1);//downLinkChannelId
            }
        }
        else
        {
            for (int i = 0 ; i < channelCount ; i++)
            {
                IO_ReadStringInstance(node->nodeId,
                                      NetworkIpGetInterfaceAddress(
                                                        node,
                                                        interfaceIndex),
                                      nodeInput,
                                      "MAC-SATELLITE-UPLINK-CHANNEL",
                                      i,
                                      FALSE,
                                      &upLinkFound,
                                      string);

                if (upLinkFound)
                {
                    if (IO_IsStringNonNegativeInteger(string))
                    {
                        channelIndex = (Int32)strtoul(string, NULL, 10);

                        if (channelIndex < node->numberChannels)
                        {
                            upLinkChannelId = channelIndex;
                        }
                        else
                        {
                            ERROR_ReportErrorArgs("MAC-SATELLITE-UPLINK-CHANNEL "
                                                  "parameter should be an integer"
                                                  " between 0 and %d or a valid "
                                                  "channel name.",
                                                  node->numberChannels - 1);
                        }
                    }
                    else if (isalpha(*string) && PHY_ChannelNameExists(node,
                                                                       string))
                    {
                        upLinkChannelId = PHY_GetChannelIndexForChannelName(
                                                node,
                                                string);
                    }
                    else
                    {
                        ERROR_ReportErrorArgs("MAC-SATELLITE-UPLINK-CHANNEL "
                                              "parameter should be an integer "
                                              "between 0 and %d or a valid "
                                              "channel name.",
                                              node->numberChannels - 1);
                    }
                }

                IO_ReadStringInstance(node->nodeId,
                                      NetworkIpGetInterfaceAddress(
                                                        node,
                                                        interfaceIndex),
                                      nodeInput,
                                      "MAC-SATELLITE-DOWNLINK-CHANNEL",
                                      i,
                                      FALSE,
                                      &downLinkFound,
                                      string);

                if (downLinkFound)
                {
                    if (IO_IsStringNonNegativeInteger(string))
                    {
                        channelIndex = (Int32)strtoul(string, NULL, 10);

                        if (channelIndex < node->numberChannels)
                        {
                            downLinkChannelId = channelIndex;
                        }
                        else
                        {
                            ERROR_ReportErrorArgs("MAC-SATELLITE-DOWNLINK-CHANNEL "
                                                  "parameter should be an integer"
                                                  " between 0 and %d or a valid "
                                                  "channel name.",
                                                  node->numberChannels - 1);
                        }
                    }
                    else if (isalpha(*string) && PHY_ChannelNameExists(node, string))
                    {
                        downLinkChannelId = PHY_GetChannelIndexForChannelName(
                                                    node,
                                                    string);
                    }
                    else
                    {
                        ERROR_ReportErrorArgs("MAC-SATELLITE-DOWNLINK-CHANNEL "
                                              "parameter should be an integer "
                                              "between 0 and %d or a valid "
                                              "channel name.",
                                              node->numberChannels - 1);
                    }
                }

                if (upLinkFound && downLinkFound)
                {
                    if (upLinkChannelId == downLinkChannelId)
                    {
                        char errStr[MAX_STRING_LENGTH];
                        sprintf(errStr , "Node %u : Currently it is not "
                                         "supported that "
                                         "MAC-SATELLITE-UPLINK-CHANNEL is equal "
                                         "to MAC-SATELLITE-DOWNLINK-CHANNEL",
                                         node->nodeId);
                        ERROR_Assert(FALSE , errStr);
                    }

                    MacSatelliteBentpipeAddUpDownLinkChannel(
                        node,
                        state->macData->phyNumber,
                        state,
                        interfaceIndex,
                        upLinkChannelId,
                        downLinkChannelId);
                }//end of if
            }//end of for
        }//end of else
    }//end of else //satellite
}

// /**
// FUNCTION :: MacSatelliteBentpipeSetChannelInfo
// LAYER :: MAC
// PURPOSE :: This routine is to set the channel information
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        upDownChannelPairPtr : MacSatelliteBentpipeUpDownChannelInfo*
//          : A pointer to the data structure the up down channel info
//            parameters
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeSetChannelInfo(
    Node* node,
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr)
{
    upDownChannelPairPtr->upChannelInfo->msgQueue = new Queue;
    upDownChannelPairPtr->upChannelInfo->msgQueue->SetupQueue
        (node,
         "FIFO",
         MacSatelliteBentpipeChannelQueueSize);

    upDownChannelPairPtr->upChannelInfo->phyStatus = PHY_IDLE;
    memset(&upDownChannelPairPtr->upChannelInfo->stats,
           0,
           sizeof(PhySatelliteRsvStatistics));

    upDownChannelPairPtr->downChannelInfo->msgQueue = new Queue;
    upDownChannelPairPtr->downChannelInfo->msgQueue->SetupQueue
        (node,
         "FIFO",
         MacSatelliteBentpipeChannelQueueSize);

    upDownChannelPairPtr->downChannelInfo->phyStatus = PHY_IDLE;
    memset(&upDownChannelPairPtr->downChannelInfo->stats,
           0,
           sizeof(PhySatelliteRsvStatistics));
}

// /**
// FUNCTION :: MacSatelliteBentpipeAddUpDownLinkChannel
// LAYER    :: MAC
// PURPOSE ::  This routine is to return the channel information of the
//             uplink channel
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : phy index of this MAC
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        interfaceIndex : int : interfaceIndex of the node
// +        upLinkChannelId : int : upLinkChannelId for which the
//          information needs to be added
// +        downLinkChannelId : int : downLinkChannelId for which the
//          information needs to be added
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeAddUpDownLinkChannel(
    Node* node,
    int phyIndex,
    MacSatelliteBentpipeState* state,
    int interfaceIndex,
    int upLinkChannelId,
    int downLinkChannelId)
{
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr =
        (MacSatelliteBentpipeUpDownChannelInfo*)
        MEM_malloc(sizeof(MacSatelliteBentpipeUpDownChannelInfo));

    upDownChannelPairPtr->upChannelInfo =
        (MacSatelliteBentpipeChannel*)
        MEM_malloc(sizeof(MacSatelliteBentpipeChannel));

    upDownChannelPairPtr->downChannelInfo =
        (MacSatelliteBentpipeChannel*)
        MEM_malloc(sizeof(MacSatelliteBentpipeChannel));

    upDownChannelPairPtr->upChannelInfo->channelIndex = upLinkChannelId;
    upDownChannelPairPtr->downChannelInfo->channelIndex =
        downLinkChannelId;

    MacSatelliteBentpipeSetChannelInfo(
        node,
        upDownChannelPairPtr);

    if (state->upDownChannelInfoList == NULL)
    {
        ListInit(node,
                 &state->upDownChannelInfoList);
    }

    ListInsert(node,
               state->upDownChannelInfoList,
               node->getNodeTime(),
               (void*)upDownChannelPairPtr);

    if (state->role == MacSatelliteBentpipeRoleGroundStation)
    {
        PhySatelliteRsvAddUpDownLinkChannel(
            node,
            phyIndex,
            downLinkChannelId,
            upLinkChannelId,
            "GROUND-STATION");
    }//end of if
    else // SATELLITE
    {
        PhySatelliteRsvAddUpDownLinkChannel(
            node,
            phyIndex,
            upLinkChannelId,
            downLinkChannelId,
            "SATELLITE");
    }
}

// /**
// FUNCTION :: MacSatelliteBentpipeGetUpLinkChannelInfo
// LAYER    :: MAC
// PURPOSE ::  This routine is to return the channel information of the
//             uplink channel
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        upLinkChannelId : int : upLinkChannelId for which the
//          information is needed
// RETURN :: MacSatelliteBentpipeChannel* : uplink channel info pointer
// **/
MacSatelliteBentpipeChannel*
MacSatelliteBentpipeGetUpLinkChannelInfo(
    Node* node,
    MacSatelliteBentpipeState* state,
    int upLinkChannelId)
{
    ListItem* listItem = state->upDownChannelInfoList->first;
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr = NULL;

    while (listItem)
    {
        upDownChannelPairPtr = (MacSatelliteBentpipeUpDownChannelInfo*)
            listItem->data;

        if (upDownChannelPairPtr->upChannelInfo->channelIndex ==
                upLinkChannelId)
        {
            return upDownChannelPairPtr->upChannelInfo;
        }

        listItem = listItem->next;
    }

    char errStr[MAX_STRING_LENGTH];
    sprintf(errStr , "Node %d : MAC-SATELLITE-USE-UPLINK-CHANNEL "
                     "must be configured",node->nodeId);
    ERROR_Assert(FALSE , errStr);

    return NULL;
}

// /**
// FUNCTION :: MacSatelliteBentpipeGetDownLinkChannelInfo
// LAYER    :: MAC
// PURPOSE ::  This routine is to return the channel information of the
//             downlink channel
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        downLinkChannelId : int : downLinkChannelId for which the
//          information is needed
// RETURN :: MacSatelliteBentpipeChannel* : downlink channel info pointer
// **/
MacSatelliteBentpipeChannel*
MacSatelliteBentpipeGetDownLinkChannelInfo(
    Node* node,
    MacSatelliteBentpipeState* state,
    int downLinkChannelId)
{
    ListItem* listItem = state->upDownChannelInfoList->first;
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr = NULL;

    while (listItem)
    {
        upDownChannelPairPtr = (MacSatelliteBentpipeUpDownChannelInfo*)
            listItem->data;

        if (upDownChannelPairPtr->downChannelInfo->channelIndex ==
                downLinkChannelId)
        {
            return upDownChannelPairPtr->downChannelInfo;
        }

        listItem = listItem->next;
    }

    char errStr[MAX_STRING_LENGTH];
    sprintf(errStr , "Node %d : MAC-SATELLITE-USE-DOWNLINK-CHANNEL "
                     "must be configured",node->nodeId);
    ERROR_Assert(FALSE , errStr);

    return NULL;
}

// /**
// FUNCTION :: MacSatelliteBentpipeGetChannelInfo
// LAYER    :: MAC
// PURPOSE :: This routine is to get the channel info of a particular
//            channel
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        channelIndex : the messages is added to this channel queue
// RETURN :: MacSatelliteBentpipeChannel* : pointer to the channel info
// **/
MacSatelliteBentpipeChannel*
MacSatelliteBentpipeGetChannelInfo(
    Node* node,
    MacSatelliteBentpipeState *state,
    int channelIndex)
{
    ListItem* listItem = state->upDownChannelInfoList->first;
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr = NULL;

    while (listItem)
    {
        upDownChannelPairPtr = (MacSatelliteBentpipeUpDownChannelInfo*)
            listItem->data;

        if (upDownChannelPairPtr->upChannelInfo->channelIndex ==
                channelIndex)
        {
            return upDownChannelPairPtr->upChannelInfo;
        }

        if (upDownChannelPairPtr->downChannelInfo->channelIndex ==
                channelIndex)
        {
            return upDownChannelPairPtr->downChannelInfo;
        }

        listItem = listItem->next;
    }

    char errStr[MAX_STRING_LENGTH];
    sprintf(errStr , "Node %d : MAC-SATELLITE-USE-UPLINK-CHANNEL or "
                     "MAC-SATELLITE-USE-DOWNLINK-CHANNEL "
                     "must be configured",node->nodeId);
    ERROR_Assert(FALSE , errStr);

    return NULL;
}

// /**
// FUNCTION :: MacSatelliteBentpipeGetUpLinkChannelInfoGS
// LAYER :: MAC
// PURPOSE :: This routine is to return the uplink channel information
//            of ground station
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// RETURN :: MacSatelliteBentpipeChannel* : channel info pointer
// **/
MacSatelliteBentpipeChannel*
MacSatelliteBentpipeGetUpLinkChannelInfoGS(
    Node* node,
    MacSatelliteBentpipeState* state)
{
    ListItem* listItem = state->upDownChannelInfoList->first;
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr = NULL;

    if (listItem)
    {
        upDownChannelPairPtr = (MacSatelliteBentpipeUpDownChannelInfo*)
            listItem->data;

        return upDownChannelPairPtr->upChannelInfo;
    }

    char errStr[MAX_STRING_LENGTH];
    sprintf(errStr , "Node %d : MAC-SATELLITE-USE-UPLINK-CHANNEL "
                     "must be configured",node->nodeId);
    ERROR_Assert(FALSE , errStr);

    return NULL;
}

// /**
// FUNCTION :: MacSatelliteBentpipePhyStatusChangeNotification
// LAYER :: MAC
// PURPOSE :: This routine is called by the PHY layer to
//            inform the local MAC process that the PHY layer has reported
//            a change in the channel state.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        channelIndex : channel whose status has changed
// +        interfaceIndex : interfaceIndex of the node
// +        oldPhyStatus : PhyStatusType : An enumerated value holding the
//          status of the PHY process _before_ the change in state occured.
// +        newPhyStatus : PhyStatusType : An enumerated value holding the
//          status of the PHY process _after_ the change in state occured.
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeReceivePhyStatusChangeNotification(
    Node *node,
    int channelIndex,
    int interfaceIndex,
    PhyStatusType oldPhyStatus,
    PhyStatusType newPhyStatus)
{
    MacSatelliteBentpipeState* state
        = MacSatelliteBentpipeGetState(node, interfaceIndex);

    double simTime = (double)node->getNodeTime() / (double)SECOND;

    if (DEBUG)
    {
        printf("%0.6e: Node[%d] receives phy status change from %s"
               " to %s.\n",
               simTime,
               node->nodeId,
               PHY_PhyStatusTypeToString(oldPhyStatus),
               PHY_PhyStatusTypeToString(newPhyStatus));
    }

    if (oldPhyStatus != newPhyStatus)
    {
        MacSatelliteBentpipeChannel* channelInfo =
            MacSatelliteBentpipeGetChannelInfo(
                node,
                state,
                channelIndex);

        channelInfo->phyStatus = newPhyStatus;
    }

    // If the new status of the interface has become idle, then the
    // MAC process immediately shifts to PHY_TRANSMITTING and sends out
    // a packet removed from the head of the network layer output queue for
    // this interface.

    if (newPhyStatus == PHY_IDLE)
    {
        MacSatelliteBentpipeSendNxtPktFromChannelQueue(
            node,
            state,
            channelIndex);
    }
}


// /**
// FUNCTION :: MacSatelliteBentpipeInsertInChannelQueue
// LAYER    :: MAC
// PURPOSE :: This routine is to insert the message in channel queue
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        channelIndex : the message is inserted to this channel queue
// +        msg : message pointer to insert in the queue
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeInsertInChannelQueue(
    Node* node,
    MacSatelliteBentpipeState *state,
    int channelIndex,
    Message* msg)
{
    MacSatelliteBentpipeChannel* channelInfo =
        MacSatelliteBentpipeGetChannelInfo(
            node,
            state,
            channelIndex);

    BOOL queueIsFull = FALSE;
    channelInfo->msgQueue->insert(
        msg,
        NULL,
        &queueIsFull,
        node->getNodeTime());
}

// /**
// FUNCTION :: MacSatelliteBentpipeChannelHasPacketToSend
// LAYER    :: MAC
// PURPOSE :: This routine is to send next packet from the queue
//            whenever a new message has been added to the queue
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        channelIndex : the messages is added to this channel queue
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeChannelHasPacketToSend(
    Node* node,
    MacSatelliteBentpipeState *state,
    int channelIndex)
{
    MacSatelliteBentpipeSendNxtPktFromChannelQueue(
        node,
        state,
        channelIndex);
}

// /**
// FUNCTION :: MacSatelliteBentpipeSendNxtPktFromChannelQueue
// LAYER    :: MAC
// PURPOSE :: This routine dequeues msg from channel queue and
//            transmit the signal.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        channelIndex : the messages will be dequeued from this
//          channelIndex queue
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeSendNxtPktFromChannelQueue(
    Node* node,
    MacSatelliteBentpipeState* state,
    int channelIndex)
{
    MacSatelliteBentpipeChannel* channelInfo =
        MacSatelliteBentpipeGetChannelInfo(
            node,
            state,
            channelIndex);

    if (channelInfo->msgQueue->isEmpty() == FALSE
        &&
        channelInfo->phyStatus == PHY_IDLE)
    {
        //dequeue msg from channel queue and transmit the signal
        Message* msg = NULL;
        BOOL dequeued = channelInfo->msgQueue->retrieve(
                            &msg,
                            0,
                            DEQUEUE_PACKET,
                            node->getNodeTime());

        MacSatelliteBentpipeHeader* hdr =
            (MacSatelliteBentpipeHeader*) MESSAGE_ReturnPacket(msg);

        Address dstAddr = hdr->pdu.data.dstAddr;

        if (AddressIsBroadcast(dstAddr))
        {
            channelInfo->stats.broadcastPktsSent++;
        }
        else
        {
            channelInfo->stats.unicastPktsSent++;
        }

        MESSAGE_AddInfo(node, msg, sizeof(int), INFO_TYPE_PhyIndex);
        memcpy(MESSAGE_ReturnInfo(msg, INFO_TYPE_PhyIndex),
               &channelIndex,
               sizeof(int));

        PHY_StartTransmittingSignal(node,
                                state->macData->phyNumber,
                                msg,
                                FALSE,
                                0);
    }//end of if
}
