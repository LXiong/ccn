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
#define __SATPHY_MAC_INTERFACE_C__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "sattsm.h"
#include "util_interface.h"

#if defined(ADDONS_TMOS)
# include "tmr.h"
#endif 

#if defined(ADDON_ANE) 
# include "ane_mac.h"
#endif

#include "util_memory.h"
DECLARE(SatTsmShim)
DECLARE(SatTsmInterfaceDecl)
DECLARE(SatTsmMacShimInterface)
DECLARE(SatTsmModuleInterface)

#include <stdarg.h>
#ifdef SATTSM_DEBUG
PRIVATE void printm(SatTsmShim shim, char *fmt, ...)
{
    va_list ap;
    char totalfmt[1024], *prefmt = "[%s:%d:%d]: ", *postfmt = "\n";

    va_start(ap, fmt);

    if (strlen(fmt) > 1024 - strlen(prefmt) - strlen(postfmt))
        return;

    if (shim->myNode == NULL)
        printf("[%s:<unspecified>:<unspecified>]: ", shim->shimName);
    else
        printf(prefmt, shim->shimName, shim->myNode->nodeId, shim->myIfIdx);

    vprintf(fmt, ap);

    printf(postfmt);
    fflush(stdout);
}
#else                           /* SATTSM_DEBUG */
static void printm(SatTsmShim shim, char *fmt, ...)
{
}
#endif                          /* SATTSM_DEBUG */

PUBLIC BOOL SatTsmNodeHasNetworkProcess(void *tsmdata)
{
    SatTsmShim s = (SatTsmShim) tsmdata;

    assert(s != NULL);

    printm(s, "Queried for network process instantiation (always TRUE).");

    return TRUE;
}

PRIVATE void SatTsmShimRegisterMac(SatTsmShim s, SatTsmShimMacInterface i,
                                   const char *tag)
{
    assert(s != NULL);
    assert(s->mac == NULL);
    assert(tag != NULL);

    printm(s, "SatTsmShimRegisterMac(tag=%s)", tag);

    s->mac = i;
    s->macTag = strdup(tag);
}

PRIVATE SatTsmShimMacInterface SatTsmShimGetMacInterface(Node * node,
                                                         int ifidx)
{
    register SatTsmShim s = (SatTsmShim) SatTsmShimGetSelf(node, ifidx);

    assert(s != NULL && s->mac != NULL);

    return s->mac;
}

PUBLIC void *SatTsmShimGetMac(Node * node, int ifidx)
{
    SatTsmShimMacInterface macif = SatTsmShimGetMacInterface(node, ifidx);

    assert(macif != NULL);

    return macif->macData;
}

PUBLIC Node *SatTsmShimGetNodeById(void *data, int id)
{
    SatTsmShim s = (SatTsmShim) data;
    register int i;

    for (i = 0; i < s->nodeListSize; i++) {
        Node *currNode = s->nodeList[i].node;
        int currNodeId = currNode->nodeId;

        if (id == currNodeId)
            return currNode;
    }
    return NULL;
}

PUBLIC int SatTsmShimGetIfIdx(void *data, Node * dstNode)
{
    SatTsmShim s = (SatTsmShim) data;
    register int i;

    for (i = 0; i < s->nodeListSize; i++) {
        register Node *currNode = s->nodeList[i].node;

        if (currNode == dstNode)
            return s->nodeList[i].interfaceIndex;
    }
    abort();
    return -1;
}

PUBLIC void *SatTsmShimGetSelf(Node * node, int ifidx)
{
    SatTsmShim s;

    assert(node != NULL && node->macData != NULL);

    s = (SatTsmShim) node->macData[ifidx]->macVar;
    assert(s != NULL);

    return s;
}

PRIVATE void SatTsmShimDeregisterMac(SatTsmShim s, const char *tag)
{
    assert(s != NULL);
    assert(s->mac != NULL);
    assert(tag != NULL);
    assert(strcmp(tag, s->macTag) == 0);

    printm(s, "SatTsmShimDeregisterMac(tag=%s)", tag);

    s->mac = NULL;
    (void) free(s->macTag);
}

PUBLIC void SatTsmShimNetworkLayerHasPacketToSend(void *tsmdata)
{
    SatTsmShim s = (SatTsmShim) tsmdata;

    assert(s != NULL);
    assert(s->mac != NULL);

    printm(s, "SatTsmShimNetworkLayerHasPacketToSend()");

    s->mac->ulPacketReadyIndication(s->myNode, s->mac->macData);
}

PUBLIC void SatTsmShimFinalize(void *tsmdata)
{
    SatTsmShim s = (SatTsmShim) tsmdata;
    register int i;

    assert(s != NULL);
    assert(s->mac != NULL);

    printm(s, "SatTsmShimFinalize()");

    SatTsmMMgrFinalize(s->moduleMgr,
                       s->myNode->macData[s->myIfIdx]->macStats);

    delete s->eventMgr;

    s->mac = NULL;

    (void) free(s->macTag);
    s->macTag = NULL;

    (void) free(s->shimName);
    s->shimName = NULL;

    OBJ_V(SatTsmMacShimInterface);
    MEM_free(s->decl->macshim);

    OBJ_V(SatTsmModuleInterface);
    MEM_free(s->decl->module);

    OBJ_V(SatTsmInterfaceDecl);
    MEM_free(s->decl);
    s->decl = NULL;

    s->myNode = NULL;
    s->nodeList = NULL;

    OBJ_V(SatTsmShim);
    MEM_free(tsmdata);
}

PRIVATE void SatTsmShimInitialize(SatTsmShim s, const NodeInput * nodeInput, NodeAddress subnetAddress, NodeAddress interfaceAddress, int numHostBits, char *macProtocolName, SubnetMemberData * subnetList, int nodesInSubnet, int subnetListIndex) {
    register int i;

    assert(s != NULL);
    printm(s, "SatTsmShimInitialize()");
    SatTsmMMgrInitialize(s->moduleMgr, nodeInput, subnetAddress, interfaceAddress,
                         numHostBits, macProtocolName,
                         subnetList, nodesInSubnet, subnetListIndex);

    NetworkUpdateForwardingTable(s->myNode, subnetAddress, ConvertNumHostBitsToSubnetMask
                                 (numHostBits), 0, s->myIfIdx, 1, ROUTING_PROTOCOL_STATIC);
}

PUBLIC void SatTsmShimLayer(void *tsmdata, Message * msg)
{
    SatTsmShim s = (SatTsmShim) tsmdata;

    assert(s != NULL);
    assert(s->mac != NULL);

    printm(s, "SatTsmShimLayer()");
    string p = (char*) MESSAGE_ReturnPacket(msg);

    UTIL_Uei uei(p);

    MESSAGE_RemoveHeader(s->myNode, msg, p.length()+1, TRACE_SATTSM_SHIM);

    s->eventMgr->post(msg, uei, 0.0);
}

PUBLIC void SatTsmShimRunTimeStat(void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);
    printm(s, "SatTsmShimRunTimeStat()");
    SatTsmMMgrRunTimeStat(s->moduleMgr);
}

PRIVATE void SatTsmShimCreateModule(SatTsmShim s, const char *moduleTag)
{
    void *module = NULL;
    SatTsmInterfaceDecl id = NULL;

    if (strcmp(moduleTag, "SATTSM") == 0) {
        module = (void *) SatTsmTestMacNew();
        id = SatTsmTestMacGetInterfaces(module);
    }
#if defined(ADDON_TMOS)
    else if (strcmp(moduleTag, "TMOS-TMR") == 0) {
        module = (void *) TmosMacNew(s->myNode, s->myIfIdx, moduleTag);
        id = TmosGetInterfaces(module);
    }
#endif
#if defined(ADDON_ANE)
    else if (strcmp(moduleTag, "ANE") == 0) {
        module = (void*) AneMacNew(s->myNode, s->myIfIdx, moduleTag);
        id = AneMacGetInterfaces(module);
    } 
#endif 
    else {
        abort();
    }

    printm(s, "SatTsmShimCreateModule(tag=%s)", moduleTag);

    assert(id->module != NULL);
    printm(s, "Registering shim module");
    SatTsmMMgrRegisterModule(s->moduleMgr, id->module, moduleTag);
    
    id->module->setEventServer(s->eventMgr,module);
}

PRIVATE void SatTsmShimAssignLowerLayer(SatTsmShim s, const char *moduleTag)
{
    register int i, done;
    SatTsmInterfaceDecl id;

    assert(s != NULL);
    assert(s->mac == NULL);

    printm(s, "Assigning lower layer to module=%s", moduleTag);
    id = SatTsmMMgrGetModuleInterfaces(s->moduleMgr, moduleTag);
    assert(id && id->shimmac != NULL);
    id->shimmac->dlSetUlModule(s->decl->macshim, id->shimmac->macData);
    // subordinate registers self if desired
}

PRIVATE void RegisterMac(SatTsmShimMacInterface i, const char *tag,
                         void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);
    printm(s, "RegisterMac()");
    SatTsmShimRegisterMac(s, i, tag);
}

PRIVATE void UlSneakPeek(Message * msg, NodeAddress prevHop,
                         NodeAddress destAddr, void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);
    printm(s, "UlSneakPeek()");

    MAC_SneakPeekAtMacPacket(s->myNode, s->myIfIdx, msg, prevHop, destAddr);
}

PRIVATE BOOL UlOutputQueueEmptyQ(void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);
    printm(s, "UlOutputQueueEmptyQ()");

    return MAC_OutputQueueIsEmpty(s->myNode, s->myIfIdx);
}

PRIVATE void UlDequeuePacket(Message ** msgptr, NodeAddress * nextHop,
                             int *networkType, QueuePriorityType * priority,
                             void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);
    assert(msgptr != NULL);

    printm(s, "UlDequeuePacket()");

    *msgptr = NULL;
    MAC_OutputQueueDequeuePacket(s->myNode,
                                 s->myIfIdx,
                                 msgptr, nextHop, networkType, priority);

}

PRIVATE BOOL SubnetBroadcastQ(SatTsmShim s, NodeAddress destAddr)
{
    assert(s != NULL);
    printm(s, "SubnetBroadcastQ()");

    if (SatTsmNodeHasNetworkProcess((void *) s) == TRUE) {
        register int i;

        for (i = 0; i < s->myNode->numberInterfaces; i++) {
            if (destAddr ==
                NetworkIpGetInterfaceBroadcastAddress(s->myNode, i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

PRIVATE BOOL UlAcceptAddressQ(NodeAddress destAddr, void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);
    assert(s->mac != NULL);

    printm(s, "UlAcceptAddressQ()");

    return MAC_IsMyUnicastFrame(s->myNode, destAddr)
        || SubnetBroadcastQ(s, destAddr)
        || destAddr == ANY_DEST;
}

PRIVATE void UlDeliver(Message * msg, NodeAddress srcAddr, void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);

    printm(s, "UlDeliver()");

    MAC_HandOffSuccessfullyReceivedPacket(s->myNode,
                                          s->myIfIdx, msg, srcAddr);
}

PRIVATE void DlStatusIndication(SatTsmMacStatus status, void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);

    printm(s, "DlStatusIndication()");
}

PRIVATE SatTsmInterfaceDecl GetInterfaces(void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);

    printm(s, "GetInterfaces()");

    return s->decl;
}

PRIVATE NodeAddress UlGetInterfaceAddress(void *data)
{
    SatTsmShim s = (SatTsmShim) data;

    assert(s != NULL);

    printm(s, "UlGetInterfaceAddress()");
    return NetworkIpGetInterfaceAddress(s->myNode, s->myIfIdx);
}

#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif                          /* _WIN32 */

PUBLIC void *SatTsmShimNew(Node * node, int ifidx, const NodeInput * nodeInput, NodeAddress subnetAddress, NodeAddress interfaceAddress, int numHostBits, char *macProtocolName, SubnetMemberData * subnetList, int nodesInSubnet, int subnetListIndex) {
    register int i;
    OBJ_P(SatTsmShim);
    SatTsmShim s = (SatTsmShim) MEM_malloc(sizeof(SatTsmShimS));

    s->mac = NULL;
    s->macTag = NULL;
    s->eventMgr = new UTIL_EventService(node, ifidx, subnetList, nodesInSubnet);
    s->moduleMgr = SatTsmMMgrNew(node, ifidx, "Shim");

    OBJ_P(SatTsmInterfaceDecl);
    s->decl = (SatTsmInterfaceDecl) MEM_malloc(sizeof(SatTsmInterfaceDeclS));

    s->decl->phymac = NULL;
    s->decl->macphy = NULL;
    s->decl->shimmac = NULL;

    OBJ_P(SatTsmMacShimInterface);
    s->decl->macshim = (SatTsmMacShimInterface) MEM_malloc(sizeof(SatTsmMacShimInterfaceS));

    s->decl->macshim->shimData = (void *) s;
    s->decl->macshim->registerMac = RegisterMac;
    s->decl->macshim->ulSneakPeek = UlSneakPeek;
    s->decl->macshim->ulOutputQueueEmptyQ = UlOutputQueueEmptyQ;
    s->decl->macshim->ulDequeuePacket = UlDequeuePacket;
    s->decl->macshim->ulAcceptAddressQ = UlAcceptAddressQ;
    s->decl->macshim->ulDeliver = UlDeliver;
    s->decl->macshim->dlStatusIndication = DlStatusIndication;
    s->decl->macshim->ulGetInterfaceAddress = UlGetInterfaceAddress;

    OBJ_P(SatTsmModuleInterface);
    s->decl->module = (SatTsmModuleInterface) MEM_malloc(sizeof(SatTsmModuleInterfaceS));;

    s->decl->module->moduleData = (void *) s;
    s->decl->module->onInitialize = NULL;
    s->decl->module->onFinalize = NULL;
    s->decl->module->onRuntimeStat = NULL;
    s->decl->module->getInterfaces = GetInterfaces;
    s->decl->module->setEventServer = NULL;

    s->myNode = node;
    s->myIfIdx = ifidx;
    s->shimName = strdup(macProtocolName);
    s->nodeList = subnetList;
    s->nodeListSize = nodesInSubnet;

    printm(s, "Shim contructed.");
    node->macData[ifidx]->macVar = (void *) s;
    // all initialized, time to construct module(s)
    SatTsmShimCreateModule(s, macProtocolName);

    SatTsmShimAssignLowerLayer(s, macProtocolName);

    SatTsmShimInitialize(s, nodeInput, subnetAddress, interfaceAddress,
        numHostBits, macProtocolName, subnetList, nodesInSubnet, subnetListIndex);

    return (void *) s;
}
