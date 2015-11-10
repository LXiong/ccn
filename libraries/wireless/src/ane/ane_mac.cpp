// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
// All Rights Reserved.
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
#define __ANE_ANE_MAC_C__
#include <iostream>
#include <string>

#include <stdlib.h>

#include <math.h>
#include <stdarg.h>

#include "api.h"
#include "partition.h"

#ifdef ENTERPRISE_LIB
#include "mpls.h"
#endif //ENTERPRISE_LIB
#include "clock.h"

#include "network_ip.h"
#include "mac.h"

#include "util_constants.h"
#include "util_nameservice.h"

#ifdef PARALLEL
#include "parallel.h"
#endif 

#define ANE_DEBUG false
#define ANE_TRACE false

#include "ane_mac.h"
#include "anesat_mac.h"

using namespace std;

void MacAneDefaultChannelModel::insertTransmission(Message* msg,
                                                   MacAneHeader* hdr)
{

}

void MacAneDefaultChannelModel::removeTransmission(Message* msg,
                                                   MacAneHeader* hdr)
{

}

void
MacAneDefaultChannelModel::getTransmissionTime(Message* msg,
                                               MacAneHeader* hdr,
                                               double& transmissionStartTime,
                                               double& transmissionDuration)
{
    transmissionStartTime = 0.0;

   int packetSizeBytes = hdr->actualSizeBytes + hdr->headerSizeBytes;

    transmissionDuration = 8.0 * packetSizeBytes / getBandwidth(mac);
}

void MacAneDefaultChannelModel::getDelayByNode(NodeId dstNodeId,
                                               bool& dropped,
                                               double& delay)
{
    dropped = false;
    delay = 0.040;

    string droppedString;

    if (dropped)
    {
        droppedString = "D";
    }
    else
    {
        droppedString = " ";
    }


    printf("Latency set to %0.3f ms [%s]\n",
           delay * 1000.0,
           droppedString.c_str());
}

void MacAneDefaultChannelModel::stationProcess(MacAneHeader* hdr,
                                               BOOL &keepPacket)
{
    keepPacket = TRUE;
}

void MacAneDefaultChannelModel::moduleAllocate()
{

}

void MacAneDefaultChannelModel::moduleFinalize()
{

}

void
MacAneDefaultChannelModel::moduleInitialize(const NodeInput* nodeInput,
                                            Address* interfaceAddress)
{

}

static
bool AddressIsNodeBroadcast(Node* node, Address* addr)
{
    bool done;
    int i;
    bool isNodeBroadcast(false);

    if (addr->networkType != NETWORK_IPV4)
    {
        return false;
    }

    for (done = false, i = 0;
         !done && i < node->numberInterfaces;
         i++)
    {
        if (addr->interfaceAddr.ipv4 ==
            NetworkIpGetInterfaceBroadcastAddress(node, i))
        {
            isNodeBroadcast = true;
            done = true;
        }
    }

    return isNodeBroadcast;
}

static
bool AddressIsBroadcast(Address* addr)
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

    switch(addr->networkType)
    {
        case NETWORK_IPV4:
            if (addr->interfaceAddr.ipv4 == ANY_DEST)
            {
                isBroadcast = true;
            }
            break;

        case NETWORK_IPV6:
        {
            int k;

            isBroadcast = true;
            for (k = 0; k < 20; k++)
            {
                if (addr->interfaceAddr.ipv6.s6_addr[k]
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
string AddressToString(Address* addr)
{
    char addressString[BUFSIZ];
    memset((void*)addressString,
           0,
           BUFSIZ);

    switch(addr->networkType)
    {
        case NETWORK_IPV4:
        {
            NodeAddress na(addr->interfaceAddr.ipv4);
            IO_ConvertIpAddressToString(na, addressString);
        }
        break;
        case NETWORK_IPV6:
        {
            in6_addr na(addr->interfaceAddr.ipv6);
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


static
MacAneState* MacAneGetState(Node* node,
                            int interfaceIndex)
{
    MacAneState* state =
        (MacAneState*) node->macData[interfaceIndex]->macVar;

    ERROR_Assert(state != NULL,
                 "The ANE model has detected that the current node"
                 " data structure is incorrectly configured for this"
                 " protocol.  The MAC data pointer is NULL, indicating"
                 " that either the node data structure has become"
                 " corrupted or that the initialization logic has"
                 " incorrectly set up the MAC protocol.  If the"
                 " simulation has already executed a number of ANE"
                 " events, then the problem is likely to be corruption."
                 " If this is the first event, then review the "
                 " initialization routines to ensure this structure"
                 " is being properly initialized.  ");

    return state;
}

static 
void MacAneTrace(MacAneState* mac, string state)
{
    if (ANE_TRACE)
    {
        clocktype now = mac->myNode->getNodeTime();
        
        printf("TRANS, %" TYPES_64BITFMT "d, %d, %d, %s\n", 
               now,
               mac->myNode->nodeId,
               mac->myIfIdx,
               state.c_str());
    }
}

static
void MacAnePrint(MacAneState* mac,
                 const char *fmt,
                 ...)
{

    if (ANE_DEBUG)
    {
        va_list ap;
        bool hasNodeContext = mac->myNode != NULL;
        
        char clockbuf[BUFSIZ];
        char nodeIdBuf[BUFSIZ];
        char nodeInterfaceIdBuf[BUFSIZ];
        if (hasNodeContext)
        {
            clocktype now = mac->myNode->getNodeTime();
            double dnow = (double)(now) / (double)SECOND;
            
            sprintf(clockbuf, 
                    "%0.3le", 
                    dnow);
            
            sprintf(nodeIdBuf, 
                    "%d", 
                    mac->myNode->nodeId);
                    
            sprintf(nodeInterfaceIdBuf, 
                    "%d", 
                    mac->myIfIdx);
        }
        else
        {
            sprintf(clockbuf, "[Unspecified]");
            
            sprintf(nodeIdBuf, "[Unspecified]");
            sprintf(nodeInterfaceIdBuf, "[Unspecified]");
        }
            
        char vbuf[BUFSIZ];
        
        va_start(ap, fmt);

        vsprintf(vbuf, fmt, ap);
        
        const char* tfmt("[%s:%s:%s]@%s: %s\n");
        printf(tfmt, 
               mac->myModuleName.c_str(),
               nodeIdBuf,
               nodeInterfaceIdBuf,
               clockbuf,
               vbuf);
        
        va_end(ap);

        fflush(stdout);
    }
}


// /**
// FUNCTION :: MacAneIsInterestedInFrameQ
// LAYER :: MAC
// PURPOSE :: Execute logic to determine whether or not the local
//            interface is interested in accepting frames for
//            the indicated IPv4 address.
// PARAMETERS ::
// +        mac : MacAneState* : A pointer to a data structure representing
//          the local set of MAC-protocol-specific state machine variables.
// +        dstAddr : Address* : An abstract representation
//          of a QualNet address.  A frame has been received with this
//          address and the caller is requesting whether or not the local
//          MAC process is interested in receiving the frame.
// RETURN :: bool : A boolean quantity that indicates whether the local
//           MAC process is interested in this frame.  If the value is
//           true, then the frame is acceptible; false otherwise.
// **/

bool MacAneIsInterestedInFrameQ(MacAneState* mac,
                                Address* dstAddr)
{
    ERROR_Assert(dstAddr->networkType == NETWORK_IPV4,
                 "The ANE model has detected a programmatic"
                 " issue with the model.  To simplify conversion"
                 " to IPv6, all IPv6 interfaces are given an"
                 " equivalent link-level IPv4 address.  In this case,"
                 " an non-IPv4 address has entered the system.  The"
                 " cause of this should be reviewed by QualNet"
                 " technical support.  The simulation cannot"
                 " continue.");

    bool isMyUnicastFrame =
        MAC_IsMyUnicastFrame(mac->myNode,
                             dstAddr->interfaceAddr.ipv4) == TRUE;

    bool isNodeBroadcast = AddressIsNodeBroadcast(mac->myNode,
                                                  dstAddr);

    bool isGlobalBroadcast = AddressIsBroadcast(dstAddr);

    bool isRoutable = mac->isHeadend;

    bool acceptAddress = isMyUnicastFrame
        || isNodeBroadcast
        || isGlobalBroadcast 
        || isRoutable;

    return acceptAddress;
}

bool MacAneIsInterestedInFrameQ(MacAneState* mac,
                                NodeAddress dstAddr)
{
    Address addr;

    addr.networkType = NETWORK_IPV4;
    addr.interfaceAddr.ipv4 = dstAddr;

    return MacAneIsInterestedInFrameQ(mac, &addr);
}

static
void MacAneDlSetStatus(Node* node,
                       MacAneState* mac,
                       int status)
{
    mac->status = status;

    MacAnePrint(mac,
                "DlSetStatus status=%d.",
                status);
}

#if defined(_WIN32)
static clocktype llround(double x)
{
    double xx = floor(x + 0.5);
    
    clocktype xxx = (clocktype)xx;
    
    return xxx;
}
#endif /* _WIN32 */

static
void MacAneSend(MacAneState* mac,
                int evCode,
                NodeId dstNodeId,
                int dstIfIdx,
                Message* msg,
                double dt)
{
    double nsdt = dt * (double)SECOND;
    
    // Round to nearest 100 ns
    clocktype ctdelay = 100 * llround(nsdt / 100.0);

    if (msg == NULL)
    {
        msg = MESSAGE_Alloc(mac->myNode,
                            MAC_LAYER,
                            MAC_PROTOCOL_ANE,
                            evCode);
    }
    else
    {
        MESSAGE_SetEvent(msg, 
                         (short)evCode);
                         
        MESSAGE_SetLayer(msg, 
                         MAC_LAYER,
                         MAC_PROTOCOL_ANE);
    }

    MESSAGE_SetInstanceId(msg, 
                          (short)dstIfIdx);
                          
    bool sameNode(mac->myNode->nodeId == dstNodeId); 
    bool sameInterface(mac->myIfIdx == dstIfIdx);
    
    bool sameAutomaton(sameNode && sameInterface);
    bool zeroDelay(ctdelay == 0);
    
    bool cutThrough(sameAutomaton && zeroDelay);
    
    clocktype now = mac->myNode->getNodeTime();
    int size = MESSAGE_ReturnPacketSize(msg);
    
    if (cutThrough)
    {
        if (ANE_TRACE)
        {
            printf("SEND-L, %" TYPES_64BITFMT "d, %d, %d, %"
                   TYPES_64BITFMT "d, %d\n",
                   now,
                   mac->myNode->nodeId, 
                   mac->myNode->nodeId,
                   ctdelay,
                   size);
        }
        
        MacAneLayer(mac->myNode,
                    mac->myIfIdx,
                    msg);
    }
    else 
    {
        if (ANE_TRACE)
        {
            printf("SEND-R, %" TYPES_64BITFMT "d, %d, %d, %" 
                   TYPES_64BITFMT "d, %d\n",
                   now,
                   mac->myNode->nodeId, 
                   dstNodeId,
                   ctdelay,
                   size);
        }
        
        MESSAGE_RemoteSend(mac->myNode,
                           dstNodeId,
                           msg,
                           ctdelay);
    }
}

static
void MacAnePrintStat(MacAneState* mac,
                     const char *fmt,
                     ...)
{
    va_list ap;

    char buf[BUFSIZ];

    va_start(ap,
             fmt);

    vsprintf(buf,
             fmt,
             ap);

    va_end(ap);

    IO_PrintStat(mac->myNode,
                 "MAC",
                 "Ane",
                 ANY_DEST,
                 mac->myIfIdx,
                 buf);
}

static
void MacAneStartNextTransmission(MacAneState* mac)
{
    MacAneTrace(mac, "MacAneStartNextTransmission");

    BOOL networkQueueIsEmpty(MAC_OutputQueueIsEmpty(mac->myNode,
                                                    mac->myIfIdx));

    if (!networkQueueIsEmpty) {
        MacAnePrint(mac,
                    "There are more packets to send.");

        MacAneNetworkLayerHasPacketToSend(mac->myNode,
                                          mac);
    }
}

void MacAneInitialize(Node* node,
                      int ifidx,
                      const NodeInput* nodeInput,
                      NodeAddress interfaceAddress,
                      SubnetMemberData* subnetList,
                      int nodesInSubnet,
                      int subnetListIndex)
{
    MacAneState* mac = MacAneGetState(node, ifidx);

    MacAneTrace(mac, "MacAneInitialize");


    mac->nodeList = subnetList;
    mac->nodeListSize = nodesInSubnet;
    
    if ((mac->gestalt() & MAC_ANE_DISTRIBUTED_ACCESS)
        == MAC_ANE_DISTRIBUTED_ACCESS)
    {
        char keybuf[1024];
        sprintf(keybuf,
                "/QualNet/Addons/Satellite/Ane/"
                "StateData/AneMac/Node[%d]/Interface[%d]",
                node->nodeId,
                ifidx);
        UTIL_NameServicePutImmutable(keybuf, (void*)mac);
    }

    char arch[BUFSIZ];
    BOOL wasFound = FALSE;

    IO_ReadString(node,
                  node->nodeId,
                  ifidx, //interfaceAddress,
                  nodeInput,
                  "ANE-SUBNET-ARCHITECTURE",
                  &wasFound,
                  arch);

    if (wasFound == TRUE)
    {
        if (strcmp(arch, "Asymmetric") == 0
            || strcmp(arch, "ASYMMETRIC") == 0
            || strcmp(arch, "asymmetric") == 0)
        {
            int hen;

            mac->subnetType = MacAneState::ClientServer;
            IO_ReadInt(node, 
                       node->nodeId,
                       ifidx, //interfaceAddress,
                       nodeInput,
                       "ANE-HEADEND-NODE",
                       &wasFound,
                       &hen);

            ERROR_Assert(wasFound == TRUE,
                         "When the ANE network type"
                         " is specified as asymmetric,"
                         " the head-end nodeId must be"
                         " specified.  Please review the"
                         " configuration file for this"
                         " simulation and determine if"
                         " the proper line is there or if"
                         " it has been misspelled.");

            ERROR_Assert(hen > 0,
                         "When the ANE network type"
                         " is specified as asymmetric,"
                         " the head-end nodeId must be"
                         " specified and be valid.  The"
                         " nodeId provided is not valid, please"
                         " check the configuration file and"
                         " verify the ANE-HEADEND-NODE statement"
                         " for this node.");

            mac->csNodeId = (NodeAddress)hen;

            if (mac->csNodeId == mac->myNode->nodeId)
            {
                mac->isHeadend = true;
            }
        }
    }

    
    if ((mac->gestalt() & MAC_ANE_CENTRALIZED_ACCESS) 
        == MAC_ANE_CENTRALIZED_ACCESS)
    {
        int npNode;

        IO_ReadInt(node,
                   node->nodeId,
                   ifidx, //interfaceAddress,
                   nodeInput,
                   "ANE-PROCESSOR-NODE",
                   &wasFound,
                   &npNode);

        ERROR_Assert(wasFound == TRUE,
                     "ANE requires a network processor to be"
                     " defined when the simulation processing"
                     " architecture is centralized.");

        mac->npNodeId = npNode;

        int npNodeIndex;

        IO_ReadInt(node,
                   node->nodeId,
                   ifidx, //interfaceAddress,
                   nodeInput,
                   "ANE-PROCESSOR-NODE-INDEX",
                   &wasFound,
                   &npNodeIndex);

        ERROR_Assert(wasFound == TRUE,
                     "ANE requires a network processor to be"
                     " defined when the simulation processing"
                     " architecture is centralized.");

        mac->npNodeIfIdx = npNodeIndex;
    }

    char moduleName[BUFSIZ];
    IO_ReadString(node, 
                  node->nodeId, 
                  ifidx,//interfaceAddress, 
                  nodeInput, 
                  "ANE-EQUATION-DEFINITION", 
                  &wasFound, 
                  moduleName);
    
    
    if (wasFound == FALSE)
    {
        ERROR_ReportWarning("The configuration file for this"
                            " simulation has not specified an"
                            " equation for this abstract model."
                            " The default model is being assumed."
                            " If this is not correct, please review"
                            " the configuration file to ensure the"
                            " correct model is specified and that"
                            " no misspelling exists.");
        sprintf(moduleName, "%s", MAC_ANE_DEFAULT_MODEL_NAME);
    }
    
    if (strcmp(moduleName, 
               "ane_default_mac") == 0) 
    {
        mac->channelModel = new MacAneDefaultChannelModel(mac);
    }
    else if (strcmp(moduleName,
                    "anesat_mac") == 0)
    {
        mac->channelModel = new AneSatChannelModel(mac);
    }
    else
    {
        ERROR_ReportError("This executable does not support the"
                          " specified abstract channel model. "
                          " Please verify"
                          " you have included the correct library"
                          " routines in"
                          " this simulation executable and that you"
                          " have the"
                          " proper licensing rights to the code.");
    }

    mac->channelModel->moduleAllocate();

    Address ifaddr;
    ifaddr.networkType = NETWORK_IPV4;
    ifaddr.interfaceAddr.ipv4 = interfaceAddress;

    mac->channelModel->moduleInitialize(nodeInput,
                                        &ifaddr);

#ifdef PARALLEL // Parallel
    PARALLEL_SetProtocolIsNotEOTCapable(node);
    PARALLEL_SetMinimumLookaheadForInterface(node, 0);
#endif // endParallel

    mac->eventMap[MacAneEvent_StationDepart]
        = new MacAneEventStationDepart(mac);

    mac->eventMap[MacAneEvent_TransmitterIdle]
        = new MacAneEventTransmitterIdle(mac);

    mac->eventMap[MacAneEvent_ChannelArrive]
        = new MacAneEventChannelArrive(mac);

    mac->eventMap[MacAneEvent_ChannelDepart]
        = new MacAneEventChannelDepart(mac);

    mac->eventMap[MacAneEvent_StationArrive]
        = new MacAneEventStationArrive(mac);

    mac->eventMap[MacAneEvent_StationProcess]
        = new MacAneEventStationProcess(mac);

    mac->eventMap[MacAneEvent_GrantIndication]
        = new MacAneEventGrantIndication(mac);

    mac->eventMap[MacAneEvent_RequestIndication]
        = new MacAneEventRequestIndication(mac);

    mac->eventMap[MacAneEvent_StationRequest]
        = new MacAneEventStationRequest(mac);

    mac->eventMap[MacAneEvent_NotifyInterest]
        = new MacAneEventNotifyInterest(mac);

    mac->eventMap[MacAneEvent_PublishNotifications]
        = new MacAneEventPublishNotifications(mac);
    
    MacAnePrint(mac, 
                "All event handlers installed.");
    
    if ((mac->gestalt() & MAC_ANE_CENTRALIZED_ACCESS) ==
        MAC_ANE_CENTRALIZED_ACCESS) 

    {
        MacAneSend(mac,
                   MacAneEvent_PublishNotifications,
                   mac->myNode->nodeId,
                   mac->myIfIdx,
                   NULL,
                   0.0);
    }
}


void MacAneLayer(Node* node,
                 int interfaceIndex,
                 Message* msg)
{
    MacAneState* mac = MacAneGetState(node,
                                      interfaceIndex);
    
    MacAneTrace(mac, "MacAneLayer");

    int evCode = MESSAGE_GetEvent(msg);

    map<int, MacAneEventHandler*>::iterator pos =
        mac->eventMap.find(evCode);

    if (pos == mac->eventMap.end())
    {
        ERROR_ReportWarning("The ANE protocol module has"
                            " encountered an unexpected event."
                            " The simulation is continuing but"
                            " the results may be affected by"
                            " this abnormal program execution.  The"
                            " model should be reviewed for possible"
                            " logic issues.");

        MESSAGE_Free(mac->myNode, msg);
    }
    else
    {
        pos->second->onEvent(evCode, msg);
    }
}

void MacAneSetTcBandwidthExample(Node* node, 
                                 int ifidx, 
                                 double bwLimit)

{
    ManagementRequest req;
    ManagementResponse resp;

    req.type = ManagementRequestSetBandwidthLimit;
    req.data = &bwLimit;

    resp.data = NULL;

    MAC_ManagementRequest(node,
                          ifidx,
                          &req,
                          &resp);

    ERROR_Assert(resp.result == ManagementResponseOK,
                 "");
}


void MacAneManagementRequest(Node* node,
                             int ifidx,
                             ManagementRequest* req,
                             ManagementResponse* resp)
{
    MacAneState* mac = MacAneGetState(node,
                                      ifidx);
    
    MacAneTrace(mac, "MacAneManagementRequest");


#if defined(DEBUG_PR4785)
    printf("MacAneManagementRequest(node=%d,"
           " ifidx=%d, req=%p, resp=%p)\n",
           node->nodeId,
           ifidx,
           req,
           resp);
#endif

    mac->channelModel->managementRequest(req,
                                         resp);
}


void MacAneFinalize(Node* node,
                    int ifidx)
{
    MacAneState* mac = MacAneGetState(node,
                                      ifidx);

    MacData* macData = node->macData[ifidx];
    
    MacAneTrace(mac, "MacAneFinalize");

    if (macData->macStats == TRUE) {
        MacAneRunTimeStat(node, mac);
    }


    mac->channelModel->moduleFinalize();

    delete mac->channelModel;
}


void MacAneNetworkLayerHasPacketToSend(Node* node,
                                       MacAneState* mac)
{
    Message* msg(NULL);
    NodeAddress nextHop;
    Address ns_nextHop;

    int networkType;
    QueuePriorityType priority;
    
    MacAneTrace(mac, "MacAneNetworkLayerHasPacketToSend");

    if (mac->transmitterStatus == MacAne::Active)
    {
        return;
    }

    MAC_OutputQueueDequeuePacket(mac->myNode,
                                 mac->myIfIdx,
                                 &msg,
                                 &nextHop,
                                 &networkType,
                                 &priority);

    ns_nextHop.networkType = NETWORK_IPV4;
    ns_nextHop.interfaceAddr.ipv4 = nextHop;

    ERROR_Assert(msg != NULL,
                 "The network layer has indicated to the ANE"
                 " protocol that it has a packet to send.  However,"
                 " when the packet was requested, the network"
                 " layer could not return a packet.  It is likely"
                 " the simulation has experienced an uncaught fatal"
                 " error.  The results may be incorrect and the"
                 " simulation must be immediately aborted.");

    MacAneHeader hdr;

    hdr.srcAddr.networkType = NETWORK_IPV4;
    hdr.srcAddr.interfaceAddr.ipv4
        =  MAC_VariableHWAddressToFourByteMacAddress(
            mac->myNode,
            &mac->myNode->macData[mac->myIfIdx]->macHWAddr);

    hdr.dstAddr.networkType = NETWORK_IPV4;
    hdr.dstAddr.interfaceAddr.ipv4 = nextHop;

    hdr.actualSizeBytes = MESSAGE_ReturnPacketSize(msg);
    hdr.headerSizeBytes = mac->channelModel->getHeaderSize();

    hdr.sourceNodeId = mac->myNode->nodeId;
    hdr.sourceNodeIfIdx = mac->myIfIdx;

    MESSAGE_AddHeader(mac->myNode,
                      msg,
                      sizeof(MacAneHeader),
                      TRACE_ANE);

    memcpy(MESSAGE_ReturnPacket(msg),
           (void*) &hdr,
           sizeof(MacAneHeader));


    MacAnePrint(mac,
                "[%d] receives data to send"
                " {src=0x%x, dst=0x%x, sdu=%d, pci=%d [U]}\n",
                mac->myNode->nodeId, 
                hdr.srcAddr, 
                hdr.dstAddr, 
                hdr.actualSizeBytes,
                hdr.headerSizeBytes);
    
    MacAnePrint(mac, 
                "Dequeued packet, invoking event StationDepart.");
                
    MacAneSend(mac, 
              MacAneEvent_StationRequest, 
              mac->myNode->nodeId, 
              mac->myIfIdx, 
              msg, 
              0.0);
}

void MacAneRunTimeStat(Node* node, MacAneState* mac) 
{
    MacAneTrace(mac, "MacAneRunTimeStat");

    MacAnePrintStat(mac, 
                    "Frames sent = %d", 
                    mac->pktsSent);
                    
    MacAnePrintStat(mac, 
                    "Frames received = %d", 
                    mac->pktsRecd);
                    
    MacAnePrintStat(mac, 
                    "Frames relayed = %d", 
                    mac->pktsFwd);
}

void MacAneEventStationRequest::onEvent(int evCode,
                                        Message *msg)
{
    MacAneTrace(m_mac, "MacAneEventStationRequest");

    ERROR_Assert(m_mac->transmitterStatus == MacAne::Idle,
                 "The ANE MAC has requested that a"
                 " frame depart from the interface"
                 " output queue.  However, the system"
                 " is still transmitting, which would"
                 " cause a collision.  This is likely a"
                 " programmatic error in the simulation."
                 " The results should be reviewed to ensure"
                 " an accurate result.");

    MacAneHeader hdr;
    memcpy((void*)&hdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneHeader));

   int packetSizeBytes = hdr.actualSizeBytes + hdr.headerSizeBytes;
    
    if (ANE_TRACE)
    {
        clocktype now = m_mac->myNode->getNodeTime();
        
        printf("SRQ, %" TYPES_64BITFMT "d, %d, %d, %d\n", 
               now, 
               m_mac->myNode->nodeId,
               m_mac->myIfIdx,
               packetSizeBytes);
        
    }

    MacAneBandwidthRequest bwr(m_mac->myNode->nodeId,
                               m_mac->myIfIdx,
                               MacAneBandwidthRequest::Nominal,
                               8 * packetSizeBytes);

    MESSAGE_AddHeader(m_mac->myNode,
                      msg,
                      sizeof(MacAneBandwidthRequest),
                      TRACE_ANE);

    memcpy(MESSAGE_ReturnPacket(msg),
           (void*)&bwr,
           sizeof(MacAneBandwidthRequest));

    int reqHandlerNodeId(m_mac->myNode->nodeId);
    int reqHandlerNodeIfIdx(m_mac->myIfIdx);
    
    if ((m_mac->gestalt() & MAC_ANE_CENTRALIZED_ACCESS) 
        == MAC_ANE_CENTRALIZED_ACCESS)

    {
        reqHandlerNodeId = m_mac->npNodeId;
        reqHandlerNodeIfIdx = m_mac->npNodeIfIdx;
    }

    MacAneSend(m_mac,
               MacAneEvent_RequestIndication,
               reqHandlerNodeId,
               reqHandlerNodeIfIdx,
               msg,
               100 / (double)SECOND);
}

void MacAneEventRequestIndication::onEvent(int evcode,
                                           Message* msg)
{
    MacAneTrace(m_mac, "MacAneEventRequestIndication");

    MacAneBandwidthRequest req;

    memcpy((void*)&req,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneBandwidthRequest));

    MESSAGE_RemoveHeader(m_mac->myNode,
                         msg,
                         sizeof(MacAneBandwidthRequest),
                         TRACE_ANE);

    MacAneHeader machdr;

    memcpy((void*) &machdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneHeader));

    double transmissionStartTime;
    double transmissionDuration;

    m_mac->channelModel->getTransmissionTime(msg,
                                             &machdr,
                                             transmissionStartTime,
                                             transmissionDuration);

    req.acceptRequest(transmissionStartTime,
                      transmissionDuration);

    MESSAGE_AddHeader(m_mac->myNode,
                      msg,
                      sizeof(MacAneBandwidthRequest),
                      TRACE_ANE);

    memcpy(MESSAGE_ReturnPacket(msg),
           (void*)&req,
           sizeof(MacAneBandwidthRequest));

    MacAneSend(m_mac,
               MacAneEvent_GrantIndication,
               req.nodeId,
               req.ifidx,
               msg,
               0.0);
}

void MacAneEventGrantIndication::onEvent(int evcode,
                                         Message *msg)
{
    MacAneTrace(m_mac, "MacAneEventGrantIndication");

    MacAneBandwidthRequest req;

    memcpy((void*)&req,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneBandwidthRequest));

    ERROR_Assert(req.isGranted(),
                 "The simulation system has encountered an"
                 " error in MacAneEventGrantIndication.  This"
                 " usually indicates a logic or programmatic"
                 " issue in the program.  Please report this"
                 " condition to QualNet support.");
    
    MacAneSend(m_mac, 
               MacAneEvent_StationDepart, 
               m_mac->myNode->nodeId, 
               m_mac->myIfIdx, 
               msg, 

               req.transmissionStartTime);
}


void MacAneEventStationDepart::onEvent(int evCode,
                                       Message* msg)
{
    MacAneTrace(m_mac, "MacAneEventStationDepart");

    MacAneBandwidthRequest req;

    memcpy((void*)&req,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneBandwidthRequest));


    MESSAGE_RemoveHeader(m_mac->myNode,
                         msg,
                         sizeof(MacAneBandwidthRequest),
                         TRACE_ANE);

    MacAneHeader machdr;
    memcpy((void*)&machdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneHeader));

    double nodeReadyTime;

    m_mac->channelModel->getNodeReadyTime(msg,
                                          &machdr,
                                          nodeReadyTime);

    if (nodeReadyTime < req.transmissionDuration) {
        nodeReadyTime = req.transmissionDuration;
    }

    ++m_mac->pktsSent;

    MacAnePrint(m_mac,
                "Invoking ChannelArrive.");

    int channelHandlerNodeId(m_mac->myNode->nodeId);
    int channelHandlerIfIdx(m_mac->myIfIdx);
    
    if ((m_mac->gestalt() & MAC_ANE_CENTRALIZED_ACCESS)
        == MAC_ANE_CENTRALIZED_ACCESS)
    {
        channelHandlerNodeId = m_mac->npNodeId;
        channelHandlerIfIdx = m_mac->npNodeIfIdx;
    }

    MacAneSend(m_mac,
               MacAneEvent_ChannelArrive,
               channelHandlerNodeId,
               channelHandlerIfIdx,
               msg,
               0.0);

    m_mac->transmitterStatus = MacAne::Active;

    MacAneSend(m_mac,
               MacAneEvent_TransmitterIdle,
               m_mac->myNode->nodeId,
               m_mac->myIfIdx,
               NULL,
               nodeReadyTime);
}

void MacAneEventChannelArrive::onEvent(int evCode,
                                       Message * msg)
{
    MacAneTrace(m_mac, "MacAneEventChannelArrive");

    MacAneHeader machdr;

    memcpy((void*)&machdr,
           (MacAneHeader*) MESSAGE_ReturnPacket(msg),
           sizeof(MacAneHeader));

    m_mac->channelModel->insertTransmission(msg,
                                            &machdr);

    unsigned int senderNodeId = machdr.sourceNodeId;
    int senderNodeIfIdx = machdr.sourceNodeIfIdx;

    Address srcAddr = machdr.srcAddr;
    Address dstAddr = machdr.dstAddr;

    MacAnePrint(m_mac,
                "Packet arrived in downlink src=0x%x, dst=0x%x",
                AddressToString(&srcAddr).c_str(),
                AddressToString(&dstAddr).c_str());

    string subnetTypeString;

    if (m_mac->subnetType == MacAneState::PeerToPeer)
    {
        subnetTypeString = "Peer-To-Peer";
    }
    else
    {
        subnetTypeString = "Client-Server";
    }

    MacAnePrint(m_mac,
                "subnetType=%s",
                subnetTypeString.c_str());

    bool isHeadendNode(m_mac->csNodeId == senderNodeId);
    bool isHeadendInterfaceIndex(senderNodeIfIdx == m_mac->csIfidx);

    if (m_mac->subnetType == MacAneState::ClientServer) {
        if (m_mac->csIfidx == -1) {
            bool done(false);
            int k;

            for (k = 0; !done && k < m_mac->nodeListSize; k++)
            {
                if (m_mac->nodeList[k].nodeId == m_mac->csNodeId) {
                    m_mac->csIfidx = m_mac->nodeList[k].interfaceIndex;
                    done = true;
                }
            }
            ERROR_Assert(done == true,
                         "The ANE MAC cannot find the interface index"
                         " of the head-end control node.  This index"
                         " is necessary for the proper operation of"
                         " the state machine.  Please check the"
                         " initialization code or the simulation"
                         " kernel to identify the cause of this"
                         " problem.");
        }

        string isHeadendString;


        if (isHeadendNode && isHeadendInterfaceIndex)
        {
            isHeadendString = "true";
        }
        else
        {
            isHeadendString = "false";
        }

        MacAnePrint(m_mac,
                    "isHeadend=%s should be [%d.%d]\n",
                    isHeadendString.c_str(),
                    m_mac->csNodeId,
                    m_mac->csIfidx);
    }

    bool isPeerToPeer(m_mac->subnetType == MacAneState::PeerToPeer);
    bool isClientServer(m_mac->subnetType == MacAneState::ClientServer);
    bool isSubnetHeadend(isHeadendNode && isHeadendInterfaceIndex);

    if (isPeerToPeer || (isClientServer && isSubnetHeadend))
    {
        int k;
        int copyCount;

        for (k = 0, copyCount = 0; k < m_mac->nodeListSize; k++)
        {
            int currNodeId = m_mac->nodeList[k].nodeId;
            int currIfIdx = m_mac->nodeList[k].interfaceIndex;

            bool isMe((unsigned int)currNodeId == senderNodeId);
            bool isntMe(!isMe);

            bool isInterestedInFrame =
                m_mac->nodeIsInterestedInPacket(currNodeId,
                                                currIfIdx,
                                                &dstAddr);

            if (isntMe && isInterestedInFrame)
            {
                bool dropped(false);
                double dt;

                m_mac->channelModel->getDelayByNode(currNodeId,
                                                    dropped,
                                                    dt);

                if (dropped == false)
                {
                    Message* newmsg = MESSAGE_Duplicate(m_mac->myNode,
                                                        msg);

                    if (dt < 0.0) {
                        ERROR_ReportWarning("The ANE MAC requires that "
                                            " all packet"
                                            " propagations have a "
                                            " positive"
                                            " time value to be"
                                            " realistic."
                                            " This"
                                            " packet has a negative"
                                            " propagation delay, so it"
                                            " will be set to"
                                            " a small positive value."
                                            " This may"
                                            " not be realistic so"
                                            " the results"
                                            " of the simulation should"
                                            " be"
                                            " reviewed"
                                            " for accuracy.");
                        dt = MAC_ANE_DEFAULT_PROPAGATION_TIME;
                    }

                    MESSAGE_AddHeader(m_mac->myNode,
                                      newmsg,
                                      sizeof(MacAneChannelTerminus),
                                      TRACE_ANE);

                    MacAneChannelTerminus term;

                    memcpy((void*) &term,
                           MESSAGE_ReturnPacket(newmsg),
                           sizeof(MacAneChannelTerminus));

                    term.nodeId = currNodeId;
                    term.nodeIfIdx = currIfIdx;

                    bool isFirstCopy(copyCount == 0);
                    ++copyCount;

                    if (isFirstCopy)
                    {
                        term.firstCopy = TRUE;
                    }
                    else
                    {
                        term.firstCopy = FALSE;
                    }

                    memcpy(MESSAGE_ReturnPacket(newmsg),
                           (void*) &term,
                           sizeof(MacAneChannelTerminus));

                    MacAneSend(m_mac,
                               MacAneEvent_ChannelDepart,
                               m_mac->myNode->nodeId,
                               m_mac->myIfIdx,
                               newmsg,
                               dt);
                }
            }
        }
    }
    else
    {
        int currNodeId = m_mac->csNodeId;
        int currIfIdx = m_mac->csIfidx;

        Address currAddr;
        currAddr.networkType = NETWORK_IPV4;

        currAddr.interfaceAddr.ipv4
            = MAPPING_GetInterfaceAddressForInterface(m_mac->myNode,
                                                      currNodeId,
                                                      currIfIdx);

        bool isMyNode((unsigned int)currNodeId == senderNodeId);
        bool isntMyNode(!isMyNode);

        ERROR_Assert(currAddr.networkType == NETWORK_IPV4,
                     "This model only support IPv4 at the MAC"
                     " level.  Please contact technical support"
                     " if IPv6 at the MAC layer is required.  This"
                     " model should work with IPv6 networks, only"
                     " in this case the MAC has tried to embed IPv6"
                     " addressed into the MAC header.  This is not"
                     " supported.  The program cannot continue.");


        bool isMyAddress(currAddr.interfaceAddr.ipv4 ==
                         srcAddr.interfaceAddr.ipv4);

        bool isntMyAddress(!isMyAddress);

        if (isntMyNode && isntMyAddress)
        {
            bool dropped(false);
            double dt(0.0);

            m_mac->channelModel->getDelayByNode(currNodeId,
                                                dropped,
                                                dt);

            if (dropped == false) {
                Message *newmsg = MESSAGE_Duplicate(m_mac->myNode,
                                                    msg);

                if (dt < 0.0) {
                    ERROR_ReportWarning("The ANE MAC requires that "
                                        " all packet"
                                        " propagations have a "
                                        " positive"
                                        " time value to be"
                                        " §realistic."
                                        " This"
                                        " packet has a negative"
                                        " propagation delay, so it"
                                        " will be set to"
                                        " a small positive value."
                                        " This may"
                                        " not be realistic so"
                                        " the results"
                                        " of the simulation should"
                                        " be"
                                        " reviewed"
                                        " for accuracy.");

                    dt = MAC_ANE_DEFAULT_PROPAGATION_TIME;
                }

                MESSAGE_AddHeader(m_mac->myNode,
                                  newmsg,
                                  sizeof(MacAneChannelTerminus),
                                  TRACE_ANE);

                MacAneChannelTerminus term;

                memcpy((void*) &term,
                       MESSAGE_ReturnPacket(newmsg),
                       sizeof(MacAneChannelTerminus));

                term.nodeId = currNodeId;
                term.nodeIfIdx = currIfIdx;

                term.firstCopy = TRUE;

                memcpy(MESSAGE_ReturnPacket(newmsg),
                       (void*) &term,
                       sizeof(MacAneChannelTerminus));

                MacAneSend(m_mac,
                           MacAneEvent_ChannelDepart,
                           m_mac->myNode->nodeId,
                           m_mac->myIfIdx,
                           newmsg,
                           dt);
            }
        }
    }

    MESSAGE_Free(m_mac->myNode,
                 msg);
}

void MacAneEventChannelDepart::onEvent(int evCode,
                                       Message* msg)
{
    ERROR_Assert(msg != NULL,
                 "The ANE MAC requires a valid frame to be sent"
                 " over the channel.  The Message pointer supplied"
                 " with this return was invalid.  Please check"
                 " the calling routine logic to verify proper"
                 " protocol operation.");

    MacAneTrace(m_mac, "MacAneEventChannelDepart");

    MacAneChannelTerminus term;

    memcpy((void*) &term,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneChannelTerminus));

    MESSAGE_RemoveHeader(m_mac->myNode,
                         msg,
                         sizeof(MacAneChannelTerminus),
                         TRACE_ANE);

    MacAneHeader machdr;

    memcpy((void*) &machdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneHeader));

    m_mac->channelModel->removeTransmission(msg,
                                            &machdr);

    MacAneSend(m_mac,
               MacAneEvent_StationArrive,
               term.nodeId,
               term.nodeIfIdx,
               msg,
               0.0);
}

void MacAneEventStationArrive::onEvent(int evCode,
                                       Message* msg)
{
    MacAneTrace(m_mac, "MacAneEventStationArrive");

    MacAneHeader machdr;

    memcpy((void*) &machdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneHeader));

    MacAneSend(m_mac,
               MacAneEvent_StationProcess,
               m_mac->myNode->nodeId,
               m_mac->myIfIdx,
               msg,
               0.0);
}

void MacAneEventStationProcess::onEvent(int evCode,
                                        Message* msg)
{
    MacAneTrace(m_mac, "MacAneEventStationProcess");

    BOOL keepPacket = FALSE;

    MacAneHeader machdr;

    memcpy((void*) &machdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacAneHeader));

    m_mac->channelModel->stationProcess(&machdr,
                                        keepPacket);

    if (keepPacket == FALSE)
    {
        MacAnePrint(m_mac,
                    "RESULT Dropping Packet");

    }
    else
    {
        MacAnePrint(m_mac,
                    "RESULT Keeping Packet");

        MESSAGE_RemoveHeader(m_mac->myNode,
                             msg,
                             sizeof(MacAneHeader), TRACE_ANE);

        bool acceptFrame(MacAneIsInterestedInFrameQ(m_mac,
                                                    &machdr.dstAddr));

        if (acceptFrame)
        {
            MacAnePrint(m_mac,
                        "Frame received src=%d dst=%d",
                        machdr.srcAddr,
                        machdr.dstAddr);

            ++m_mac->pktsRecd;

            MAC_HandOffSuccessfullyReceivedPacket(m_mac->myNode,
                                                  m_mac->myIfIdx,
                                                  msg,
                                                  machdr.srcAddr.interfaceAddr.ipv4);
        }
        else
        {
            if (m_mac->isPromiscuous == TRUE)
            {
                if (machdr.srcAddr.networkType == NETWORK_IPV4)
                {
                    MAC_SneakPeekAtMacPacket(m_mac->myNode,
                                             m_mac->myIfIdx,
                                             msg,
                                             machdr.srcAddr.interfaceAddr.ipv4,
                                             machdr.dstAddr.interfaceAddr.ipv4);
                }
            }

            MESSAGE_Free(m_mac->myNode, msg);
        }
    }
}


void MacAneEventTransmitterIdle::onEvent(int evcode,
                                         Message* msg)
{
    MacAneTrace(m_mac, "MacAneEventTransmitterIdle");

    m_mac->transmitterStatus = MacAne::Idle;

    MESSAGE_Free(m_mac->myNode,
                 msg);

    MacAneStartNextTransmission(m_mac);
}

void MacAneEventPublishNotifications::sendNotification(Address* addr)
{
    MacAneInterestNotification notify;

    notify.reportingNodeId = m_mac->myNode->nodeId;
    notify.reportingNodeIfIdx = m_mac->myIfIdx;

    memcpy((void*)&notify.requestedAddress,
           (void*)addr,
           sizeof(Address));

    notify.interestLevel = true;

    Message* msg = MESSAGE_Alloc(m_mac->myNode,
                                 MAC_LAYER,
                                 MAC_PROTOCOL_ANE,
                                 MacAneEvent_NotifyInterest);
    
    
    MESSAGE_PacketAlloc(m_mac->myNode,
                        msg,
                        sizeof(MacAneInterestNotification),
                        TRACE_ANE);
    

    memcpy(MESSAGE_ReturnPacket(msg),
           (void*)&notify,
           sizeof(MacAneInterestNotification));
    
    extern void MacAneSend(MacAneState*, 
                           int,
                           NodeAddress,
                           int,
                           Message*,
                           double);

    MacAneSend(m_mac,
               MacAneEvent_NotifyInterest,
               m_mac->npNodeId,
               m_mac->npNodeIfIdx,
               msg,
               0);
}
