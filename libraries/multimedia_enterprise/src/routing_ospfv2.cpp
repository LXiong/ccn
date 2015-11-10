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

// PURPOSE: This is an implementation of the intra-domain routing protocol
//          OSPF version 2 as described in the latest RFC 2328. It resides
//          at the network layer just above IP. This implementation also
//          follows the OSPF NSSA option as described in the RFC 1587.
//
// ASSUMPTIONS: 1. All nodes are considered routers.
//              2. All external path considered as Type2 external path.
//              3. ASBR will not calculate AS-External routes.
//                 BGP will be responsible to inject AS-External
//                 routes in IP Forwarding table.
//
// FEATURES LEFT TO BE DONE: 1. Virtual Link.
//                           2. Equal cost multipath.
//                           3. Incremental LSA update
//                           4. NBMA

// NOTE: POINT_TO_MULTIPOINT network type assumes a broadcast-capable
//       medium; Hellos, (some) LSUs, and delayed LS Acknowledgments
//       are sent to AllSPFRouters address, and neighbors do not need
//       to be pre-configured.
//
//       When running OSPFv2 over broadcast wireless medium it is
//       suggested to use POINT_TO_MULTIPOINT mode. If user still
//       wants to run in BROADCAST mode, configure the network as
//       single hop broadcast network.


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

#include "api.h"

#ifdef ADDON_DB
#include "dbapi.h"
#include "db_multimedia.h"
#endif

#include "partition.h"
#include "network_ip.h"
#include "routing_ospfv2.h"
#include "multicast_mospf.h"
#include "routing_qospf.h"
#include "external_socket.h"

#ifdef IPNE_INTERFACE
#include "ospf_interface.h"
#endif

#ifdef ADDON_STATS_MANAGER
#include "stats_manager.h"
#endif

#ifdef CYBER_CORE
#include "network_iahep.h"
#endif // CYBER_CORE

#ifdef ADDON_MA
#include "ma_interface.h"
#endif

#include "network_dualip.h"

#ifdef JNE_LIB
#include "audit_logger.h"
#endif

#ifdef JNE_GUI
    #include "jne.h"
    #include "vis_visual.h"

    static inline
    void JNEGUI_SetDesignatedRouterState(Node* node, int interfaceIndex, bool enableDR)
    {
        if (JNE_IsVisualizationEffectEnabled(node) == false)
        {
            return;
        }

        VISUAL_JneWnwDesignatedRouter(node, interfaceIndex, enableDR);
    }

    static inline
    void JNEGUI_EnableDesignatedRouterState(Node* node, int interfaceIndex)
    {
        JNEGUI_SetDesignatedRouterState(node, interfaceIndex, true);
    }

    static inline
    void JNEGUI_DisableDesignatedRouterState(Node* node, int interfaceIndex)
    {
        JNEGUI_SetDesignatedRouterState(node, interfaceIndex, false);
    }
#endif /* JNE_GUI */


typedef std::pair<int, NodeAddress> nbrAdvanceParams;
// Added for Route-Redistribution
#define ROUTE_REDISTRIBUTE


// Enabling this might reduce some duplicate LSA transmission in a network.
// When enable this, a node wait for time [0, OSPFv2_FLOOD_TIMER] before
// sending Update in response to a Request. Thus at synchronization time
// a node send less number of Update packet in response to several duplicate
// Request from different node in the network. This intern also reduce
// number of Ack packet transmitted in the network.
#define OSPFv2_SEND_DELAYED_UPDATE 1

// Enable this if you want ABR to examine transit areas summary LSAs also
// other than examining backbone area only. This might be helpfull for
// finding routes when link to backbone faults.
#ifndef ADDON_BOEINGFCS
#define EXAMINE_TRANSIT_AREA 0
#else
#define EXAMINE_TRANSIT_AREA 1
#endif

#define OSPFv2_DEBUG_TABLE 0
#define OSPFv2_DEBUG_TABLEErr 0

#define OSPFv2_DEBUG 0
#define OSPFv2_DEBUG_SPT 0
#define OSPFv2_DEBUG_LSDB 0
#define OSPFv2_DEBUG_ISM 0
#define OSPFv2_DEBUG_SYNC 0
#define OSPFv2_DEBUG_FLOOD 0
#define OSPFv2_DEBUG_HELLO 0
#define OSPFv2_DEBUG_PACKET 0

#define OSPFv2_DEBUG_DEMANDCIRCUIT 0

#ifdef ADDON_BOEINGFCS
#include "routing_ces_rospf.h"
#include "network_security_ces_haipe.h"
#include "mode_ces_wnw_receive_only.h"

#define OSPFv2_DEBUG_ROSPF 0
#define OSPFv2_MAX_NUM_RETX -1
#endif

#ifdef ADDON_NGCNMS
#define OSPFv2_MAX_RETX_LIST_SIZE 30
#endif

#define OSPFv2_DEBUG_ERRORS 0
#define OSPFv2_DEBUG_SPTErr 0
#define OSPFv2_DEBUG_LSDBErr 0
#define OSPFv2_DEBUG_SYNCErr 0
#define OSPFv2_DEBUG_FLOODErr 0
#define OSPFv2_DEBUG_HELLOErr 0

// to collect detailed information about protocol control load.
// ONLY ACCURATE IN 1 PROCESSOR SEQUENTIAL MODE!
#define COLLECT_DETAILS 0

#if COLLECT_DETAILS
#define OSPFv2_PRINT_INTERVAL 100 * SECOND

clocktype lastHelloPrint = 0;
clocktype lastDDPrint = 0;
clocktype lastLSReqPrint = 0;
clocktype lastRetxDDPrint = 0;
clocktype lastLSUpdatePrint = 0;
clocktype lastRetxLSUpdatePrint = 0;
clocktype lastRetxLSReqPrint = 0;
clocktype lastAckPrint = 0;
long numHelloBytes = 0;
long numDDBytesSent = 0;
long numLSReqBytesSent = 0;
long numRetxLSReqBytes = 0;
long numRetxLSUpdateBytes = 0;
long numRetxDDBytesSent = 0;
long numLSUpdateBytes = 0;
long numAckBytes = 0;
#endif

static
BOOL Ospfv2DebugSync(Node* node)
{
    return FALSE;
}

static
BOOL Ospfv2DebugFlood(Node* node)
{
    return FALSE;
}

static
BOOL Ospfv2DebugISM(Node* node)
{
    return FALSE;
}

static
BOOL Ospfv2DebugSPT(Node* node)
{
    return FALSE;
}


void Ospfv2OptionsSetQOS(unsigned char *options, BOOL qos)
{
    unsigned char x = (unsigned char)qos;

    //masks qos within boundry range
    x = x & maskChar(8, 8);

    //clears the 8th bit
    *options = *options & (~(maskChar(8, 8)));

    //setting the value of x in ospfLsh
    *options = *options | LshiftChar(x, 8);
}

void Ospfv2OptionsSetExt(unsigned char *options, BOOL external)
{
    unsigned char x = (unsigned char)external;

    //masks external within boundry range
    x = x & maskChar(8, 8);

    //clears the 7th bit
    *options = *options & (~(maskChar(7, 7)));

    //setting the value of x in ospfLsh
    *options = *options | LshiftChar(x, 7);
}

void Ospfv2OptionsSetMulticast(unsigned char *options, BOOL multicast)
{
    unsigned char x = (unsigned char)multicast;

    //masks multicast within boundry range
    x = x & maskChar(8, 8);

    //clears the 6th bit
    *options = *options & (~(maskChar(6, 6)));

    //setting the value of x in ospfLsh
    *options = *options | LshiftChar(x, 6);
}

void Ospfv2OptionsSetNSSACapability(unsigned char *options,
                                           BOOL np)
{
    unsigned char x = (unsigned char)np;

    //masks multicast within boundry range
    x = x & maskChar(8, 8);

    //clears the 5th bit
    *options = *options & (~(maskChar(5, 5)));

    //setting the value of reserved in ospfLsh
    *options = *options | LshiftChar(x, 5);
}

void Ospfv2OptionsSetDC(unsigned char *options, BOOL dc)
{
    unsigned char x = (unsigned char)dc;

    //masks multicast within boundry range
    x = x & maskChar(8, 8);

    //clears the 3rd bit
    *options = *options & (~(maskChar(3, 3)));

    //setting the value of reserved in ospfLsh
    *options = *options | LshiftChar(x, 3);
}

void Ospfv2OptionsSetOpaque(unsigned char *options, BOOL opaque)
{
    unsigned char x = (unsigned char)opaque;

    //masks multicast within boundry range
    x = x & maskChar(8, 8);

    //clears the 2nd bit
    *options = *options & (~(maskChar(2, 2)));

    //setting the value of reserved in ospfLsh
    *options = *options | LshiftChar(x, 2);
}

BOOL Ospfv2OptionsGetQOS(unsigned char options)
{
    unsigned char qos = options;

    //clears all the bits except 8th bit
    qos = qos & maskChar(8, 8);

    //Right shifts so that last bit represents qos
    qos = RshiftChar(qos, 8);

    return (BOOL)qos;
}

BOOL Ospfv2OptionsGetExt(unsigned char options)
{
    unsigned char external = options;

    //clears all the bits except 7th bit
    external = external & maskChar(7, 7);

    //Right shifts so that last bit represents qos
    external = RshiftChar(external, 7);

    return (BOOL)external;
}

BOOL Ospfv2OptionsGetMulticast(unsigned char options)
{
    unsigned char multicast = options;

    //clears all the bits except 6th bit
    multicast = multicast & maskChar(6, 6);

    //Right shifts so that last bit represents qos
    multicast = RshiftChar(multicast, 6);

    return (BOOL)multicast;
}

BOOL Ospfv2OptionsGetNSSACapability(unsigned char options)
{
    unsigned char np = options;

    //clears all the bits except 5th bit
    np = np & maskChar(5, 5);

    //Right shifts so that last bit represents qos
    np = RshiftChar(np, 5);

    return (BOOL)np;
}

BOOL Ospfv2OptionsGetDC(unsigned char options)
{
    unsigned char dc = options;

    //clears all the bits except 3rd bit
    dc = dc & maskChar(3, 3);

    //Right shifts so that last bit represents qos
    dc = RshiftChar(dc, 3);

    return (BOOL)dc;
}

BOOL Ospfv2OptionsGetOpaque(unsigned char options)
{
    unsigned char opaque = options;

    //clears all the bits except 2nd bit
    opaque = opaque & maskChar(2, 2);

    //Right shifts so that last bit represents qos
    opaque = RshiftChar(opaque, 2);

    return (BOOL)opaque;
}

void Ospfv2RouterLSASetNSSATranslation(unsigned char *ospfRouterLsa,
                                       BOOL Nt)
{
    unsigned char x = (unsigned char)Nt;

    //masks abr within boundry range
    x = x & maskChar(8, 8);

    //clears the 4th bit
    *ospfRouterLsa = *ospfRouterLsa & (~(maskChar(4, 4)));

    //setting the value of x in ospfRouterLsa
    *ospfRouterLsa = *ospfRouterLsa | LshiftChar(x, 4);
}

BOOL Ospfv2RouterLSAGetNSSATranslation(unsigned char ospfRouterLsa)
{
    unsigned char nt = ospfRouterLsa;

    //clears all the bits except eighth bit
    nt = nt & maskChar(4, 4);

    //Right shifts so that last bit represents areaBorderRouter
    nt = RshiftChar(nt, 4);

    return (BOOL)nt;
}

///////////////////////////////////////////////////////////////////
//                 Prototype declaration
///////////////////////////////////////////////////////////////////
void Ospfv2SendLSRequestPacket(Node* node,
                               NodeAddress nbrAddr,
                               int interfaceIndex,
                               BOOL retx);



static
void Ospfv2HandleInterfaceEvent(Node* node,
                                int interfaceIndex,
                                Ospfv2InterfaceEvent eventType);

void Ospfv2AddToLSRetxList(Node* node,
                           int interfaceIndex,
                           Ospfv2Neighbor* nbrInfo,
                           char* LSA,
                           int* referenceCount);


void Ospfv2SendLSUpdate(Node* node, NodeAddress nbrAddr, int interfaceId);

void Ospfv2SendUpdatePacket(Node* node,
                            NodeAddress dstAddr,
                            int interfaceId,
                            char* payload,
                            int payloadSize,
                            int LSACount);

static
BOOL Ospfv2HaveLinkWithNetworkVertex(Node* node, Ospfv2Vertex* parent);

static
BOOL Ospfv2LSABodyChanged(
    Ospfv2LinkStateHeader* newLSHeader,
    Ospfv2LinkStateHeader* oldLSHeader);

static
Ospfv2RoutingTableRow* Ospfv2GetRoute(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType);

static
BOOL Ospfv2CompDestType(
    Ospfv2DestType destType1,
    Ospfv2DestType destType2);


/***** Start: OPAQUE-LSA *****/
static
void Ospfv2OriginateType11OpaqueLSA(Node* node);
/***** End: OPAQUE-LSA *****/

// Grabber function for ROSPF
#ifdef ADDON_BOEINGFCS

BOOL Ospfv2RospfDisplayIcons(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    return ospf->displayRospfIcons;

}

#endif

//-------------------------------------------------------------------------//
//                              PRINT FUNCTIONS                            //
//-------------------------------------------------------------------------//


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintCommonHeader
// PURPOSE      :Print OSPFv2 Packet's common Header
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

//ospfHeader = (Ospfv2CommonHeader*) MESSAGE_ReturnPacket(msg);
void Ospfv2PrintCommonHeader(Ospfv2CommonHeader* ospfHeader)
{
    char addrStr[MAX_ADDRESS_STRING_LENGTH];

    printf("\n\n");
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |   Version = %d  |   Type = %d    |          Packet Length = %-7d|\n",
        ospfHeader->version, ospfHeader->packetType, ospfHeader->packetLength);
    printf("    -----------------------------------------------"
           "---------------------\n");
    IO_ConvertIpAddressToString(ospfHeader->routerID, addrStr);
    printf("    |                      Router ID = %-32s|\n",
            addrStr);
    printf("    -----------------------------------------------"
           "---------------------\n");
    IO_ConvertIpAddressToString(ospfHeader->areaID, addrStr);
    printf("    |                        Area ID = %-32s|\n",
            addrStr);
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |        Checksum = %-13d|          AuType = %-14d|\n",
            ospfHeader->checkSum, ospfHeader->authenticationType);
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |                         Authentication       "
           "                    |\n");
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |                         Authentication       "
           "                    |\n");
    printf("    -----------------------------------------------"
           "---------------------\n");
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintLSHeader
// PURPOSE      :Print Link State Header
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

void Ospfv2PrintLSHeader(Ospfv2LinkStateHeader* LSHeader)
{
    char addrStr[MAX_ADDRESS_STRING_LENGTH];

    printf("\n\n");
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |        Age = %-18d|     0    %2d%2d%2d|    Type = %d    |"
        "\n", LSHeader->linkStateAge,
        Ospfv2OptionsGetMulticast(LSHeader->options),
        Ospfv2OptionsGetExt(LSHeader->options),
        Ospfv2OptionsGetQOS(LSHeader->options), LSHeader->linkStateType);
    printf("    -----------------------------------------------"
           "---------------------\n");
    IO_ConvertIpAddressToString(LSHeader->linkStateID, addrStr);
    printf("    |                    Link State ID = %-30s|\n",
            addrStr);
    printf("    -----------------------------------------------"
           "---------------------\n");
    IO_ConvertIpAddressToString(LSHeader->advertisingRouter, addrStr);
    printf("    |                       AdvrRtr = %-33s|\n",
            addrStr);
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |                     Seq Number = %-32x|\n",
            LSHeader->linkStateSequenceNumber);
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |        Checksum = %-13d|          length = %-14d|\n",
            LSHeader->linkStateCheckSum, LSHeader->length);
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("\n\n");
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintLSA
// PURPOSE      :Print out the content of an LSA.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

void Ospfv2PrintLSA(char* LSA)
{
    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;

    printf("    -----------------------------------------------"
           "---------------------\n");
    Ospfv2PrintLSHeader(LSHeader);
    char advRt[20];
    int i;

    IO_ConvertIpAddressToString(LSHeader->advertisingRouter, advRt);

    // LSA is a router LSA.
    if (LSHeader->linkStateType == OSPFv2_ROUTER)
    {
        Ospfv2RouterLSA* routerLSA = (Ospfv2RouterLSA*) LSA;
        Ospfv2LinkInfo* linkList = (Ospfv2LinkInfo*) (routerLSA + 1);
        printf("    |   0  %2d%2d%2d%2d%2d|       0       |          "
            "   Links = %-12d|\n",
            Ospfv2RouterLSAGetNSSATranslation(routerLSA->ospfRouterLsa),
            Ospfv2RouterLSAGetWCMCReceiver(routerLSA->ospfRouterLsa),
            Ospfv2RouterLSAGetVirtLnkEndPt(routerLSA->ospfRouterLsa),
            Ospfv2RouterLSAGetASBRouter(routerLSA->ospfRouterLsa),
            Ospfv2RouterLSAGetABRouter(routerLSA->ospfRouterLsa),
            routerLSA->numLinks);
        printf("    -----------------------------------------------"
               "---------------------\n");

        for (i = 0; i < routerLSA->numLinks; i++)
        {
            int numTos;
            char linkID[20];
            char linkData[20];

            IO_ConvertIpAddressToString(linkList->linkID, linkID);
            IO_ConvertIpAddressToString(linkList->linkData, linkData);

            printf("    |                          Link ID = %-30s|\n",
                   linkID);
            printf("    -----------------------------------------------"
                   "---------------------\n");
            printf("    |                        Link Data = %-30s|\n",
                   linkData);
            printf("    -----------------------------------------------"
                   "---------------------\n");
            printf("    |    Type = %d   |  No of TOS = %d  |             "
                   "Metric = %-10d|\n",
                   linkList->type, linkList->numTOS, linkList->metric);
            printf("    -----------------------------------------------"
                   "---------------------\n");

            // Place the linkList pointer properly
            numTos = linkList->numTOS;
            linkList = (Ospfv2LinkInfo*)
                ((QospfPerLinkQoSMetricInfo*)(linkList + 1) + numTos);
        }
    }
    // LSA is a network LSA.
    else if (LSHeader->linkStateType == OSPFv2_NETWORK)
    {
        int numLink;
        NodeAddress netMask;
        char netMaskStr[MAX_ADDRESS_STRING_LENGTH];
        char rtrAddrStr[MAX_ADDRESS_STRING_LENGTH];

        Ospfv2NetworkLSA* networkLSA = (Ospfv2NetworkLSA*) LSA;

        NodeAddress* rtrAddr = ((NodeAddress*) (networkLSA + 1)) + 1;

        memcpy(&netMask, networkLSA + 1, sizeof(NodeAddress));
        IO_ConvertIpAddressToString(netMask, netMaskStr);

        printf("    |                     Network Mask = %-30s|\n",
                netMaskStr);
        printf("    -----------------------------------------------"
               "---------------------\n");

        numLink = (networkLSA->LSHeader.length
                  - sizeof(Ospfv2NetworkLSA) - 4)
                  / (sizeof(NodeAddress));

        for (i = 0; i < numLink; i++)
        {
            IO_ConvertIpAddressToString(rtrAddr[i], rtrAddrStr);
            printf("    |                  Attached Router = %-30s|\n",
                    rtrAddrStr);
            printf("    -----------------------------------------------"
                   "---------------------\n");
        }
    }
    else if ((LSHeader->linkStateType == OSPFv2_NETWORK_SUMMARY)
            || (LSHeader->linkStateType == OSPFv2_ROUTER_SUMMARY))
    {
        int index = 0;
        NodeAddress netMask;
        char netMaskStr[20];
        Int32 metric = 0;

        char destAddr[20];
        char destMask[20];

        index += sizeof(Ospfv2LinkStateHeader);
        memcpy(&netMask, &LSA[index], sizeof(NodeAddress));
        IO_ConvertIpAddressToString(netMask, netMaskStr);
        printf("    |                     Network Mask = %-30s|\n",
               netMaskStr);
        printf("    -----------------------------------------------"
               "---------------------\n");

        index += sizeof(NodeAddress);

        memcpy(&metric, &LSA[index], sizeof(Int32));
        index += sizeof(Int32);
        metric = metric & 0xFFFFFF;

        printf("    |      0      |                  Metric = %-25d|\n",
               metric);
        printf("    -----------------------------------------------"
               "---------------------\n");
        IO_ConvertIpAddressToString(LSHeader->linkStateID, destAddr);
        IO_ConvertIpAddressToString(*(NodeAddress*)(LSHeader+1), destMask);

        printf("    LSA is Type %d Summary LSA\n", LSHeader->linkStateType);
        printf("    Advertising Router = %s\n", advRt);
        printf("    Dest = %s\n", destAddr);
        printf("    Mask = %s\n", destMask);
        printf("    linkStateAge %d\n", LSHeader->linkStateAge);
    }
    // BGP-OSPF Patch Start
    else if (LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
    {
        Ospfv2ExternalLinkInfo* linkInfo;
        char addrStr[100];
        char netStr[100];
        char maskStr[100];
        char forwardAddr[20];

        linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
        IO_ConvertIpAddressToString(linkInfo->networkMask, addrStr);

        printf("    |                     Network Mask = %-30s|\n",
                addrStr);
        printf("    -----------------------------------------------"
               "---------------------\n");
        printf("    |%d|      0      |                Metric = %-25d|\n",
               Ospfv2ExternalLinkGetEBit(linkInfo->ospfMetric),
               Ospfv2ExternalLinkGetMetric(linkInfo->ospfMetric));
        printf("    -----------------------------------------------"
               "---------------------\n");
        IO_ConvertIpAddressToString(linkInfo->forwardingAddress, addrStr);
        printf("    |               Forwarding Address = %-30s|\n",
               addrStr);
        printf("    -----------------------------------------------"
               "---------------------\n");
        IO_ConvertIpAddressToString(linkInfo->externalRouterTag, addrStr);
        printf("    |               External Route Tag = %-30s|\n",
               addrStr);
        printf("    -----------------------------------------------"
               "---------------------\n");
        IO_ConvertIpAddressToString(LSHeader->linkStateID, netStr);
        IO_ConvertIpAddressToString(linkInfo->networkMask, maskStr);
        IO_ConvertIpAddressToString(linkInfo->forwardingAddress, forwardAddr);
        printf("    LSA is AS External LSA\n");
        printf("    Advertising Router = %15s\n", advRt);
        printf("    Dest = %15s\n", netStr);
        printf("    DestMask = %15s\n", maskStr);
        printf("    Forwarding Address = %15s\n", forwardAddr);
        printf("    linkStateAge %d\n", LSHeader->linkStateAge);
    }
    // BGP-OSPF Patch End
    else if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
    {
        Ospfv2ExternalLinkInfo* linkInfo;
        char netStr[100];
        char maskStr[100];

        linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
        IO_ConvertIpAddressToString(LSHeader->linkStateID, netStr);
        IO_ConvertIpAddressToString(linkInfo->networkMask, maskStr);
        printf("    LSA is NSSA External LSA\n");
        printf("    Advertising Router = %s\n", advRt);
        printf("    Dest = %s\n", netStr);
        printf("    DestMask = %s\n", maskStr);
        printf("    linkStateAge %d\n", LSHeader->linkStateAge);
    }
    // M-OSPF Patch Start
    else if (LSHeader->linkStateType == OSPFv2_GROUP_MEMBERSHIP)
    {
        char linkStateID[100];
        char vertexID[100];

        MospfGroupMembershipLSA* groupMembershipLSA =
                (MospfGroupMembershipLSA*)LSA;

        MospfGroupMembershipInfo* grpMemberList =
                (MospfGroupMembershipInfo*)(groupMembershipLSA + 1);

        int numLinks = (groupMembershipLSA->LSHeader.length
                        -  (sizeof(MospfGroupMembershipLSA)))
                        / (sizeof(MospfGroupMembershipInfo));

        IO_ConvertIpAddressToString(LSHeader->linkStateID,
                                    linkStateID);

        printf ("   %8s  %10s   %4d     %2d    %2d \n",
            advRt, linkStateID, LSHeader->linkStateAge,
            LSHeader->linkStateSequenceNumber, numLinks);

        for (i = 0; i < numLinks; i++)
        {
            IO_ConvertIpAddressToString(grpMemberList[i].vertexID,
                                        vertexID);

            printf("    |                  Vertex Tpye = %-34d|\n",
                   grpMemberList[i].vertexType);
            printf("    -----------------------------------------------"
                   "---------------------\n");

            printf("    |                   Vertex Id = %-35s|\n",
                            vertexID);
            printf("    -----------------------------------------------"
                   "---------------------\n");
        }
    }
    // M-OSPF Patch End

    /***** Start: OPAQUE-LSA *****/
    // LSA is a Opaque LSA.
    else if (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE)
    {
        UInt32 opaqueID;

        Ospfv2ASOpaqueLSA* opaqueLSA = (Ospfv2ASOpaqueLSA*) LSA;

        opaqueID = Ospfv2OpaqueGetOpaqueId(opaqueLSA->LSHeader.linkStateID);

        printf("    LSA from node %s (%d) contains:\n", advRt, opaqueID);
        printf("    LSA age is %d \n", opaqueLSA->LSHeader.linkStateAge);

#ifdef CYBER_CORE
#ifdef ADDON_BOEINGFCS
            if (Ospfv2OpaqueGetOpaqueType(opaqueLSA->LSHeader.linkStateID)
                    == (unsigned char)HAIPE_ADDRESS_ADV)
            {
                RoutingCesRospfPrintHaipeAddressAdvLSA(opaqueLSA);
            }
#endif
#endif

    }
    /***** End: OPAQUE-LSA *****/
    else
    {
        if (OSPFv2_DEBUG_LSDB)
        {
            printf("    No valid LSA \n");
        }
    }
    printf("\n\n");
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintRouterLSAListForThisArea()
// PURPOSE      :Print the content of the Router LSA list for this area
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintRouterLSAListForThisArea(
    Node* node,
    Ospfv2Area* thisArea)
{
    Ospfv2ListItem* item = NULL;
    char areaIDStr[MAX_ADDRESS_STRING_LENGTH];

    IO_ConvertIpAddressToString(thisArea->areaID,
                                areaIDStr);
    printf("    Router LSAs for node %u, area %s\n",
                node->nodeId, areaIDStr);
    printf("    size is %d\n", thisArea->routerLSAList->size);

    item = thisArea->routerLSAList->first;

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv2PrintLSA((char*) item->data);

        item = item->next;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintNetworkLSAListForThisArea()
// PURPOSE      :Print the content of the Network LSA list for this area.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintNetworkLSAListForThisArea(
    Node* node,
    Ospfv2Area* thisArea)
{
    Ospfv2ListItem* item = NULL;
    char areaIDStr[MAX_ADDRESS_STRING_LENGTH];

    IO_ConvertIpAddressToString(thisArea->areaID,
                                areaIDStr);
    printf("    Network LSAs for node %u, area %s\n",
                node->nodeId, areaIDStr);
    printf("    size is %d\n", thisArea->networkLSAList->size);

    item = thisArea->networkLSAList->first;

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv2PrintLSA((char*) item->data);
        item = item->next;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintSummaryLSAListForThisArea()
// PURPOSE      :Print the content of the Router and Network Summary
//               LSA list for this area
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintSummaryLSAListForThisArea(
    Node* node,
    Ospfv2Area* thisArea)
{
    Ospfv2ListItem* item = NULL;
    char areaIDStr[MAX_ADDRESS_STRING_LENGTH];

    IO_ConvertIpAddressToString(thisArea->areaID,
                                areaIDStr);
    printf("    Router Summary LSAs for node %u, area %s\n",
                node->nodeId, areaIDStr);
    printf("    size is %d\n", thisArea->routerSummaryLSAList->size);

    item = thisArea->routerSummaryLSAList->first;

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv2PrintLSA((char*) item->data);
        item = item->next;
    }

    printf("    Network Summary LSAs for node %u, area %s\n",
                node->nodeId, areaIDStr);
    printf("    size is %d\n", thisArea->networkSummaryLSAList->size);

    item = thisArea->networkSummaryLSAList->first;

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv2PrintLSA((char*) item->data);
        item = item->next;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintRouterLSAList()
// PURPOSE      :Print the content of the Router LSA list.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintRouterLSAList(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2ListItem* item = NULL;

    printf("Router LSAs for node %u\n", node->nodeId);

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv2Area* thisArea = (Ospfv2Area*) item->data;

        Ospfv2PrintRouterLSAListForThisArea(node, thisArea);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintNetworkLSAList
// PURPOSE      :Print the content of the Network LSA list.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

void Ospfv2PrintNetworkLSAList(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2ListItem* item = NULL;

    printf("Network LSAs for node %u\n", node->nodeId);

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv2Area* thisArea = (Ospfv2Area*) item->data;

        Ospfv2PrintNetworkLSAListForThisArea(node, thisArea);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintSummaryLSAList()
// PURPOSE      :Print the content of the Router and Network Summary
//               LSA list.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintSummaryLSAList(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2ListItem* item = NULL;

    printf("Summary LSAs for node %u\n", node->nodeId);

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv2Area* thisArea = (Ospfv2Area*) item->data;

        Ospfv2PrintSummaryLSAListForThisArea(node, thisArea);
    }
}


// BGP-OSPF Patch Start
//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintASExternalLSA()
// PURPOSE      :Print the content of the AS External LSA list for this node.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintASExternalLSA(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* item;

    if (!ospf->asExternalLSAList)
    {
        return;
    }
    item = ospf->asExternalLSAList->first;

    printf("AS External LSA list for node %u\n", node->nodeId);
    printf("Size is = %d\n", ospf->asExternalLSAList->size);

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv2PrintLSA((char*) item->data);
        item = item->next;
    }
    printf("\n");
}
// BGP-OSPF Patch End

//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintNSSAExternalLSA()
// PURPOSE      :Print the content of NSSA External LSA list for this node.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintNSSAExternalLSA(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* item;

    if (!ospf->nssaExternalLSAList)
    {
        return;
    }
    item = ospf->nssaExternalLSAList->first;

    printf("NSSA External LSA list for node %u\n", node->nodeId);
    printf("Size is = %d\n", ospf->nssaExternalLSAList->size);

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv2PrintLSA((char*) item->data);
        item = item->next;
    }
    printf("\n");
}

/***** Start: OPAQUE-LSA *****/
//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintASOpaqueLSAList()
// PURPOSE      :Print the contents of the AS Opaque LSA list for this node.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//
void Ospfv2PrintASOpaqueLSAList(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2ListItem* item = NULL;

    if (!ospf->ASOpaqueLSAList)
    {
        return;
    }
    item = ospf->ASOpaqueLSAList->first;

    printf("Type-11 Opaque LSAs for node %u\n", node->nodeId);
    printf("Size is = %d\n", ospf->ASOpaqueLSAList->size);

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv2PrintLSA((char*) item->data);
        item = item->next;
    }
    printf("\n");
}
/***** End: OPAQUE-LSA *****/

//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintLSDB()
// PURPOSE      :Print the content of entire LSDB of a node.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintLSDB(Node* node)
{
    char timeStringInSecond[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime()+getSimStartTime(node),
        timeStringInSecond);

    printf("---------------------------------------------------\n");
    printf("        LSDB OF NODE %u at time %s\n",
            node->nodeId, timeStringInSecond);
    Ospfv2PrintRouterLSAList(node);
    Ospfv2PrintNetworkLSAList(node);
    Ospfv2PrintSummaryLSAList(node);
    Ospfv2PrintASExternalLSA(node);
    Ospfv2PrintNSSAExternalLSA(node);
    /***** Start: OPAQUE-LSA *****/
    Ospfv2PrintASOpaqueLSAList(node);
    /***** End: OPAQUE-LSA *****/
    printf("---------------------------------------------------\n");
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2PrintHelloPacket
// PURPOSE: Print out the content of the Hello packet.
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2PrintHelloPacket(Ospfv2HelloPacket* helloPkt)
{
    int numNeighbor;
    numNeighbor = (helloPkt->commonHeader.packetLength
                  - sizeof(Ospfv2HelloPacket)) / sizeof(NodeAddress);

    char addrStr[MAX_ADDRESS_STRING_LENGTH];

    IO_ConvertIpAddressToString(helloPkt->networkMask, addrStr);
    printf("    |                    Network Mask = %-31s|\n", addrStr);
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |        Hello Interval = %-7d|     0    %2d%2d%2d|"
           "  Rtr Pri = %-4d|\n",
           helloPkt->helloInterval,
           Ospfv2OptionsGetMulticast(helloPkt->options),
           Ospfv2OptionsGetExt(helloPkt->options),
           Ospfv2OptionsGetQOS(helloPkt->options),
           helloPkt->rtrPri);
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |                Router Dead Interval = %-27d|\n",
            helloPkt->routerDeadInterval);
    printf("    -----------------------------------------------"
           "---------------------\n");
    IO_ConvertIpAddressToString(helloPkt->designatedRouter, addrStr);
    printf("    |                             DR = %-32s|\n",
            addrStr);
    printf("    -----------------------------------------------"
           "---------------------\n");
    IO_ConvertIpAddressToString(helloPkt->backupDesignatedRouter, addrStr);
    printf("    |                            BDR = %-32s|\n",
            addrStr);
    printf("    -----------------------------------------------"
           "---------------------\n");

    NodeAddress* tempNeighbor = NULL;
    tempNeighbor = (NodeAddress*)(helloPkt + 1);
    for (int i = 0; i < numNeighbor; i++)
    {
        IO_ConvertIpAddressToString(tempNeighbor[i], addrStr);
        printf("    |                            Nbr = %-32s|\n",
            addrStr);
        printf("    -----------------------------------------------"
               "---------------------\n");
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintDatabaseDescriptionPacket()
// PURPOSE      :Print content of database description packet.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2PrintDatabaseDescriptionPacket(
    Ospfv2DatabaseDescriptionPacket* dbDscrpPkt)
{
    Ospfv2LinkStateHeader* LSHeader = NULL;
    int size;

    printf("    |        Interface MTUl = %-7d|     0    %2d%2d%2d|"
           "0 0 0 0 0 %2d%2d%2d|\n",
           dbDscrpPkt->interfaceMtu,
           Ospfv2OptionsGetMulticast(dbDscrpPkt->options),
           Ospfv2OptionsGetExt(dbDscrpPkt->options),
           Ospfv2OptionsGetQOS(dbDscrpPkt->options),
           Ospfv2DatabaseDescriptionPacketGetInit(dbDscrpPkt->ospfDbDp),
           Ospfv2DatabaseDescriptionPacketGetMore(dbDscrpPkt->ospfDbDp),
           Ospfv2DatabaseDescriptionPacketGetMS(dbDscrpPkt->ospfDbDp));
    printf("    -----------------------------------------------"
           "---------------------\n");
    printf("    |                  DD sequence number = %-27d|\n",
           dbDscrpPkt->ddSequenceNumber);
    printf("    -----------------------------------------------"
           "---------------------\n");

    LSHeader = (Ospfv2LinkStateHeader*) (dbDscrpPkt + 1);
    size = dbDscrpPkt->commonHeader.packetLength;

    // For each LSA in the packet
    for (size -= sizeof(Ospfv2DatabaseDescriptionPacket); size > 0;
            size -= sizeof(Ospfv2LinkStateHeader))
    {
        Ospfv2PrintLSHeader(LSHeader);
        LSHeader += 1;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintLSRequestPacket()
// PURPOSE      :Print content of Link State Request packet.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2PrintLSRequestPacket(Ospfv2LinkStateRequestPacket* reqPkt)
{
    char addrStr[MAX_ADDRESS_STRING_LENGTH];

    Ospfv2LSRequestInfo* LSReqObject = NULL;
    int numLSReqObject;
    numLSReqObject = (reqPkt->commonHeader.packetLength
                         - sizeof(Ospfv2LinkStateRequestPacket))
                             / sizeof(Ospfv2LSRequestInfo);

    LSReqObject = (Ospfv2LSRequestInfo*) (reqPkt + 1);

    for (int i = 0; i < numLSReqObject; i++)
    {
        printf("    |                       LS type = %-33d|\n",
            LSReqObject->linkStateType);
        printf("    -----------------------------------------------"
               "---------------------\n");
        IO_ConvertIpAddressToString(LSReqObject->linkStateID, addrStr);
        printf("    |                         LS ID = %-33s|\n",
            addrStr);
        printf("    -----------------------------------------------"
               "---------------------\n");
        IO_ConvertIpAddressToString(LSReqObject->advertisingRouter, addrStr);
        printf("    |                       AdvrRtr = %-33s|\n",
            addrStr);
        printf("    -----------------------------------------------"
               "---------------------\n");
        LSReqObject += 1;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintLSUpdatePacket()
// PURPOSE      :Print content of Link State Update packet.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2PrintLSUpdatePacket(Ospfv2LinkStateUpdatePacket* updatePkt)
{
    Ospfv2LinkStateHeader* LSHeader = NULL;

    printf("    |                      Num LSAs = %-33d|\n",
        updatePkt->numLSA);

    LSHeader = (Ospfv2LinkStateHeader*) (updatePkt + 1);
    for (int count = 0; count < updatePkt->numLSA; count++,
            LSHeader = (Ospfv2LinkStateHeader*)
                (((char*) LSHeader) + LSHeader->length))
    {
        Ospfv2PrintLSA((char*)LSHeader);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintLSAckPacket()
// PURPOSE      :Print content of Link State Acknowledgement packet.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2PrintLSAckPacket(Ospfv2LinkStateAckPacket* ackPkt)
{
    Ospfv2LinkStateHeader* LSHeader = NULL;

    int numAck = (ackPkt->commonHeader.packetLength
                  - sizeof(Ospfv2LinkStateAckPacket))
                      / sizeof(Ospfv2LinkStateHeader);

    LSHeader = (Ospfv2LinkStateHeader*) (ackPkt + 1);

    for (int count = 0; count < numAck; count++, LSHeader = LSHeader + 1)
    {
        Ospfv2PrintLSHeader(LSHeader);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintProtocolPacket
// PURPOSE      :Print OSPFv2 Protocol packet i.e. Hello, Request, DD
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

//ospfHeader = (Ospfv2CommonHeader*) MESSAGE_ReturnPacket(msg);
void Ospfv2PrintProtocolPacket(Ospfv2CommonHeader* ospfHeader)
{
    switch (ospfHeader->packetType)
    {
        case OSPFv2_HELLO:
            Ospfv2PrintCommonHeader(ospfHeader);
            Ospfv2PrintHelloPacket((Ospfv2HelloPacket*)ospfHeader);
            break;
        case OSPFv2_DATABASE_DESCRIPTION:
            Ospfv2PrintCommonHeader(ospfHeader);
            Ospfv2PrintDatabaseDescriptionPacket(
                (Ospfv2DatabaseDescriptionPacket*)ospfHeader);
            break;
        case OSPFv2_LINK_STATE_REQUEST:
            Ospfv2PrintCommonHeader(ospfHeader);
            Ospfv2PrintLSRequestPacket(
                (Ospfv2LinkStateRequestPacket*)ospfHeader);
            break;
        case OSPFv2_LINK_STATE_UPDATE:
            Ospfv2PrintCommonHeader(ospfHeader);
            Ospfv2PrintLSUpdatePacket(
                (Ospfv2LinkStateUpdatePacket*)ospfHeader);
            break;
        case OSPFv2_LINK_STATE_ACK:
            Ospfv2PrintCommonHeader(ospfHeader);
            Ospfv2PrintLSAckPacket((Ospfv2LinkStateAckPacket*)ospfHeader);
            break;
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2PrintVertexList
// PURPOSE: Print each vertex item from the given list. This function is
//          called from Ospfv2PrintCandidateList() and
//          Ospfv2PrintShortestPathList()
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintVertexList(Ospfv2List* list)
{
    Ospfv2ListItem* listItem = list->first;

    while (listItem)
    {
        int i = 1;
        Ospfv2Vertex* entry;
        Ospfv2ListItem* nextHopItem;
        char vertexIdStr[20];
        char vertexTypeStr[25];

        entry = (Ospfv2Vertex*) listItem->data;
        IO_ConvertIpAddressToString(entry->vertexId, vertexIdStr);

        if (entry->vertexType == OSPFv2_VERTEX_ROUTER) {
            strcpy(vertexTypeStr, "OSPFv2_VERTEX_ROUTER");
        }
        else if (entry->vertexType == OSPFv2_VERTEX_NETWORK) {
            strcpy(vertexTypeStr, "OSPFv2_VERTEX_NETWORK");
        }
        else {
            strcpy(vertexTypeStr, "Unknown Vertex Type");
            ERROR_Assert(FALSE, "Unknown Vertex Type\n");
        }

        printf("    Vertex ID = %15s\n", vertexIdStr);
        printf("    Vertex Type = %s\n", vertexTypeStr);
        printf("    metric = %d\n", entry->distance);

        nextHopItem = entry->nextHopList->first;

        while (nextHopItem)
        {
            Ospfv2NextHopListItem* nextHopInfo = NULL;
            char nextHopStr[MAX_ADDRESS_STRING_LENGTH];

            nextHopInfo = (Ospfv2NextHopListItem*) nextHopItem->data;
            IO_ConvertIpAddressToString(nextHopInfo->nextHop, nextHopStr);

            printf("    NextHop[%d] = %s\n", i, nextHopStr);
            nextHopItem = nextHopItem->next;
            i += 1;
        }

        listItem = listItem->next;
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2PrintCandidateList
// PURPOSE: Print the content of the candidate list.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintCandidateList(
    Node* node,
    Ospfv2List* candidateList)
{

    printf("Candidate list for node %u\n", node->nodeId);
    printf("    size = %d\n", candidateList->size);

    Ospfv2PrintVertexList(candidateList);
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2PrintShortestPathList
// PURPOSE: Print the content of the shortest path list.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintShortestPathList(
    Node* node,
    Ospfv2List* shortestPathList)
{

    printf("Shortest path list for node %u\n", node->nodeId);
    printf("    size = %d\n", shortestPathList->size);

    Ospfv2PrintVertexList(shortestPathList);
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2PrintNeighborList
// PURPOSE:  Print out this node's neighbor list.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2PrintNeighborList(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2Neighbor* tempNeighborInfo = NULL;
    Ospfv2ListItem* tempListItem = NULL;

    printf("Neighbor list for node %u\n", node->nodeId);
    printf("    number of interfaces = %d\n", node->numberInterfaces);
    printf("    size is %d\n", ospf->neighborCount);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        int j = 1;
        char addrStr[MAX_ADDRESS_STRING_LENGTH];
        char nbrStr[MAX_ADDRESS_STRING_LENGTH];

        if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            continue;
        }

        IO_ConvertIpAddressToString(ospf->iface[i].address, addrStr);
        printf("    neighbors on interface %15s\n", addrStr);

        tempListItem = ospf->iface[i].neighborList->first;

        // Print out each of our neighbors in our neighbor list.
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;
#ifdef JNE_LIB
            if (!tempNeighborInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error","Neighbor data not found",
                                  JNE::CRITICAL);
            }
#endif
            ERROR_Assert(tempNeighborInfo, "Neighbor data not found\n");
            IO_ConvertIpAddressToString(
                tempNeighborInfo->neighborID, nbrStr);

            printf("        neighbor[%d] = %15s\n", j, nbrStr);

            tempListItem = tempListItem->next;
            j += 1;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2GetNeighborStateString()
// PURPOSE      :Based on eighbor state value get correct string.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2GetNeighborStateString(
    Ospfv2NeighborState state,
    char* str)
{
    if (state == S_NeighborDown)
    {
        strcpy(str, "S_NeighborDown");
    }
    else if (state == S_Attempt)
    {
        strcpy(str, "S_Attempt");
    }
    else if (state == S_Init)
    {
        strcpy(str, "S_Init");
    }
    else if (state == S_TwoWay)
    {
        strcpy(str, "S_TwoWay");
    }
    else if (state == S_ExStart)
    {
        strcpy(str, "S_ExStart");
    }
    else if (state == S_Exchange)
    {
        strcpy(str, "S_Exchange");
    }
    else if (state == S_Loading)
    {
        strcpy(str, "S_Loading");
    }
    else if (state == S_Full)
    {
        strcpy(str, "S_Full");
    }
    else
    {
        strcpy(str, "Unknown");
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintNeighborState()
// PURPOSE      :Print the current state of each neighbor.
// ASSUMPTION   :None
// RETURN VALUE :NULL
//-------------------------------------------------------------------------//

void Ospfv2PrintNeighborState(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    int i;

    printf("\n\n      Neighbor state for Node %u:\n", node->nodeId);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        Ospfv2ListItem* listItem = NULL;
        Ospfv2Neighbor* nbrInfo = NULL;

        if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            continue;
        }

        listItem = ospf->iface[i].neighborList->first;

        while (listItem)
        {
            char destStr[MAX_ADDRESS_STRING_LENGTH];
            char stateStr[20];
            nbrInfo = (Ospfv2Neighbor*) listItem->data;

            IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress, destStr);
            Ospfv2GetNeighborStateString(nbrInfo->state, stateStr);

            printf("\nState of neighbor %15s : %15s\n", destStr, stateStr);

            listItem = listItem->next;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintDBSummaryList()
// PURPOSE      :Print content of database summary list.
// ASSUMPTION   :None
// RETURN VALUE :NULL
//-------------------------------------------------------------------------//

static
void Ospfv2PrintDBSummaryList(
    Node* node,
    Ospfv2List* DBSummaryList)
{
    Ospfv2ListItem* item = DBSummaryList->first;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    char linkStateIDStr[MAX_ADDRESS_STRING_LENGTH];
    char advRt[MAX_ADDRESS_STRING_LENGTH];

    printf("    Database Summary list of node %u:\n", node->nodeId);

    while (item)
    {
        // Remember, we consider only one LSA per Update packet
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        IO_ConvertIpAddressToString(LSHeader->linkStateID, linkStateIDStr);
        IO_ConvertIpAddressToString(LSHeader->advertisingRouter, advRt);

        printf("        linkStateID = %15s\n", linkStateIDStr);
        printf("        advertisingRouter = %15s\n", advRt);
        printf("        linkStateSequenceNumber = 0x%x\n\n",
                LSHeader->linkStateSequenceNumber);

        item = item->next;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintDatabaseDescriptionPacket()
// PURPOSE      :Print content of database description packet.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2PrintDatabaseDescriptionPacket(Node* node, Message* msg)
{
    Ospfv2DatabaseDescriptionPacket* dbDscrpPkt = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    int size;
    char linkStateIDStr[MAX_ADDRESS_STRING_LENGTH];
    char linkStateTypeStr[MAX_ADDRESS_STRING_LENGTH];
    char advRt[MAX_ADDRESS_STRING_LENGTH];

    dbDscrpPkt = (Ospfv2DatabaseDescriptionPacket*)
                    MESSAGE_ReturnPacket(msg);

    LSHeader = (Ospfv2LinkStateHeader*) (dbDscrpPkt + 1);

    size = dbDscrpPkt->commonHeader.packetLength;

    printf("    Content of DATABASE:\n");

    // For each LSA in the packet
    for (size -= sizeof(Ospfv2DatabaseDescriptionPacket); size > 0;
            size -= sizeof(Ospfv2LinkStateHeader))
    {
        IO_ConvertIpAddressToString(
            LSHeader->linkStateType, linkStateTypeStr);
        IO_ConvertIpAddressToString(LSHeader->linkStateID, linkStateIDStr);
        IO_ConvertIpAddressToString(LSHeader->advertisingRouter, advRt);

        printf("        linkStateType = %25s\n", linkStateTypeStr);
        printf("        linkStateID = %15s\n", linkStateIDStr);
        printf("        advertisingRouter = %15s\n", advRt);
        printf("        linkStateSequenceNumber = 0x%x\n\n",
               LSHeader->linkStateSequenceNumber);

        LSHeader += 1;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2PrintRoutingTable
// PURPOSE      :Prints routing table to the screen
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

void Ospfv2PrintRoutingTable(Node* node)
{

    int i;

    Ospfv2Data* ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                         node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2RoutingTableRow* rowPtr = (Ospfv2RoutingTableRow*)
            BUFFER_GetData(&ospf->routingTable.buffer);
    char timeStringInSecond[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime()+getSimStartTime(node),
        timeStringInSecond);

    printf("\n OSPFv2 Routing table for node %u at time %15s\n",
           node->nodeId, timeStringInSecond);
    printf("   ------------------------------------------------------"
        "---------------------------------------------------\n");
    printf("   destination           mask            nextHop     metric   "
        "destType       pathType    areaId  type2Metric\n");
    printf("   ------------------------------------------------------"
        "---------------------------------------------------\n");

    for (i = 0; i < ospf->routingTable.numRows; i++)
    {
        char destStr[MAX_ADDRESS_STRING_LENGTH];
        char maskStr[MAX_ADDRESS_STRING_LENGTH];
        char nextHopStr[MAX_ADDRESS_STRING_LENGTH];
        char areaIdStr[MAX_ADDRESS_STRING_LENGTH];
        char destTypeStr[100];
        char pathTypeStr[100];

        if (rowPtr[i].flag == OSPFv2_ROUTE_INVALID)
        {
            continue;
        }

        IO_ConvertIpAddressToString(rowPtr[i].destAddr, destStr);
        IO_ConvertIpAddressToString(rowPtr[i].addrMask, maskStr);
        IO_ConvertIpAddressToString(rowPtr[i].nextHop, nextHopStr);
        IO_ConvertIpAddressToString(rowPtr[i].areaId, areaIdStr);

        if (rowPtr[i].destType == OSPFv2_DESTINATION_NETWORK)
        {
            strcpy(destTypeStr, "NETWORK");
        }
        else
        {
            strcpy(destTypeStr, "ROUTER");
        }

        if (rowPtr[i].pathType == OSPFv2_INTRA_AREA)
        {
            strcpy(pathTypeStr, "INTRA-AREA");
        }
        else if (rowPtr[i].pathType == OSPFv2_INTER_AREA)
        {
            strcpy(pathTypeStr, "INTER-AREA");
        }
        else if (rowPtr[i].pathType == OSPFv2_TYPE1_EXTERNAL)
        {
            strcpy(pathTypeStr, "TYPE1-EXTERNAL");
        }
        else if (rowPtr[i].pathType == OSPFv2_TYPE2_EXTERNAL)
        {
            strcpy(pathTypeStr, "TYPE2-EXTERNAL");
        }
        else
        {
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Unknown path type in routing table",
                              JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, "Unknown path type in routing table\n");
        }

        printf("  %15s  %15s  %15s  %4d  %10s  %15s",
                destStr,
                maskStr,
                nextHopStr,
                rowPtr[i].metric,
                destTypeStr,
                pathTypeStr);

        if (rowPtr[i].pathType == OSPFv2_TYPE2_EXTERNAL
            || rowPtr[i].pathType == OSPFv2_TYPE1_EXTERNAL)
        {
            printf("    ***   %4d\n", rowPtr[i].type2Metric);
        }
        else
        {
            printf("   %s\n", areaIdStr);
        }
    }

    printf("\n");

}


// /**
// FUNCTION   :: Ospfv2PrintTraceXMLCommonHeader
// LAYER      :: NETWORK
// PURPOSE    :: Print out packet common header information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + header  : Ospfv2CommonHeader* : Pointer to header
// RETURN ::  void : NULL
// **/

void Ospfv2PrintTraceXMLCommonHeader(Node* node, Ospfv2CommonHeader* header)
{
    char buf[MAX_STRING_LENGTH];
    char addr1[20];
    char addr2[20];

    IO_ConvertIpAddressToString(header->routerID, addr1);
    IO_ConvertIpAddressToString(header->areaID, addr2);

    sprintf(buf, " <commonHdr> %hu %hu %hd %s %s %hd %hd %d </commonHdr>",
            header->version,
            header->packetType,
            header->packetLength,
            addr1,
            addr2,
            header->checkSum,
            header->authenticationType,
            0);

    TRACE_WriteToBufferXML(node, buf);
}


// /**
// FUNCTION   :: Ospfv2PrintTraceXMLPacketOptions
// LAYER      :: NETWORK
// PURPOSE    :: Print out packet common header information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + options  : unsigned char : Packet options
// RETURN ::  void : NULL
// **/

void Ospfv2PrintTraceXMLPacketOptions(Node* node, unsigned char options)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, " <options> %d %d %d %d %d %d %d %d </options>",
        0, //reserved
        0, //opaque
        Ospfv2OptionsGetDC(options),
        0, //external-Attribute
        Ospfv2OptionsGetNSSACapability(options),
        Ospfv2OptionsGetMulticast(options),
        Ospfv2OptionsGetExt(options),
        Ospfv2OptionsGetQOS(options));
    TRACE_WriteToBufferXML(node, buf);
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2PrintHelloPacket
// PURPOSE: Print out the content of the Hello packet.
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2PrintHelloPacket(
    Node* node,
    Ospfv2HelloPacket* helloPkt)
{
    int i;
    int numNeighbor;
    NodeAddress* tempNeighbor = NULL;
    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];

    numNeighbor = (helloPkt->commonHeader.packetLength
                  - sizeof(Ospfv2HelloPacket)) / sizeof(NodeAddress);

    tempNeighbor = (NodeAddress*)(helloPkt + 1);
#ifdef JNE_LIB
    if (!tempNeighbor)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Neighbor not found in the Neighbor list",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(tempNeighbor,
        "Neighbor not found in the Neighbor list\n");

    printf("Node %u: Hello packet content:\n", node->nodeId);

    printf("    number of neighbor = %d\n", numNeighbor);

    for (i = 0; i < numNeighbor; i++)
    {
        IO_ConvertIpAddressToString(tempNeighbor[i], nbrAddrStr);
        printf("    neighbor[%d] = %15s\n", i + 1, nbrAddrStr);
    }
}


// /**
// FUNCTION   :: Ospfv2PrintTraceXMLLsaHeader
// LAYER      :: NETWORK
// PURPOSE    :: Print LSA header trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + lsaHdr    : Ospfv2LinkStateHeader* : Pointer to Ospfv2LinkStateHeader
// RETURN ::  void : NULL
// **/

void Ospfv2PrintTraceXMLLsaHeader(
    Node* node,
    Ospfv2LinkStateHeader* lsaHdr)
{
    char buf[MAX_STRING_LENGTH];
    char addr1[20];
    char addr2[20];
    sprintf(buf, " <lsaHdr>%hd",
        lsaHdr->linkStateAge);
    TRACE_WriteToBufferXML(node, buf);

    Ospfv2PrintTraceXMLPacketOptions(node, lsaHdr->options);

    IO_ConvertIpAddressToString(lsaHdr->linkStateID, addr1);
    IO_ConvertIpAddressToString(lsaHdr->advertisingRouter, addr2);

    sprintf(buf, "%hu %s %s %d %hd %hd",
        lsaHdr->linkStateType,
        addr1,
        addr2,
        lsaHdr->linkStateSequenceNumber,
        lsaHdr->linkStateCheckSum,
        lsaHdr->length);
    TRACE_WriteToBufferXML(node, buf);

    sprintf(buf, "</lsaHdr>");
    TRACE_WriteToBufferXML(node, buf);
}

// /**
// FUNCTION   :: Ospfv2PrintTraceXMLLsa
// LAYER      :: NETWORK
// PURPOSE    :: Print LSA body trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + lsaHdr    : Ospfv2LinkStateHeader* : Pointer to Ospfv2LinkStateHeader
// RETURN ::  void : NULL
// **/

static
void Ospfv2PrintTraceXMLLsa(Node* node, Ospfv2LinkStateHeader* lsaHdr)
{
    int i = 0;
    char buf[MAX_STRING_LENGTH];
    char addr1[20];
    char addr2[20];
    char addr3[20];

    if (lsaHdr->linkStateType == OSPFv2_ROUTER)
    {
        Ospfv2RouterLSA* routerLSA = (Ospfv2RouterLSA*) lsaHdr;
        Ospfv2LinkInfo* linkList = (Ospfv2LinkInfo*) (routerLSA + 1);

        sprintf(buf, " <routerLsaBody>");
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLLsaHeader(node, lsaHdr);

        //router LSA flags
        sprintf(buf, " <routerLsaFlags> %d %d %d %d %d %d %d %d </routerLsaFlags>",
            0, //reserved
            0, //reserved
            0, //reserved
            Ospfv2RouterLSAGetNSSATranslation(routerLSA->ospfRouterLsa),
            Ospfv2RouterLSAGetWCMCReceiver(routerLSA->ospfRouterLsa),
            Ospfv2RouterLSAGetVirtLnkEndPt(routerLSA->ospfRouterLsa),
            Ospfv2RouterLSAGetASBRouter(routerLSA->ospfRouterLsa),
            Ospfv2RouterLSAGetABRouter(routerLSA->ospfRouterLsa));
        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, " %hu %hd",
            0, //routerLSA->reserved,
            routerLSA->numLinks);
        TRACE_WriteToBufferXML(node, buf);

        for (i = 0; i < routerLSA->numLinks; i++)
        {
            int numTos = 0;
            IO_ConvertIpAddressToString(linkList->linkID, addr1);
            IO_ConvertIpAddressToString(linkList->linkData, addr2);

            sprintf(buf, " <linkInfo>%s %s %hu %hu %hd</linkInfo>",
                addr1,
                addr2,
                linkList->type,
                linkList->numTOS,
                linkList->metric);
            TRACE_WriteToBufferXML(node, buf);

            // Place the linkList pointer properly
            numTos = linkList->numTOS;
            linkList = (Ospfv2LinkInfo*)
                ((QospfPerLinkQoSMetricInfo*)(linkList + 1) + numTos);
        }

        sprintf(buf, " </routerLsaBody>");
        TRACE_WriteToBufferXML(node, buf);

    }
    else if (lsaHdr->linkStateType == OSPFv2_NETWORK)
    {
        Ospfv2NetworkLSA* networkLSA = (Ospfv2NetworkLSA*) lsaHdr;
        NodeAddress netMask = *((NodeAddress*) (networkLSA + 1));
        NodeAddress* rtrAddr = ((NodeAddress*) (networkLSA + 1)) + 1;
        int numLink = (networkLSA->LSHeader.length
                  - sizeof(Ospfv2NetworkLSA) - 4)
                  / (sizeof(NodeAddress));

        sprintf(buf, " <networkLsaBody>");
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLLsaHeader(node, lsaHdr);

        IO_ConvertIpAddressToString(netMask, addr1);

        sprintf(buf, " %s <routerList> %u", addr1, netMask);
        TRACE_WriteToBufferXML(node, buf);

        for (i = 0; i < numLink; i++)
        {
            IO_ConvertIpAddressToString(rtrAddr[i], addr1);
            sprintf(buf, " %s", addr1);
            TRACE_WriteToBufferXML(node, buf);
        }
        sprintf(buf, "</routerList></networkLsaBody>");
        TRACE_WriteToBufferXML(node, buf);
    }
    else if ((lsaHdr->linkStateType == OSPFv2_NETWORK_SUMMARY)
            || (lsaHdr->linkStateType == OSPFv2_ROUTER_SUMMARY))
    {
        int i = 0;
        Ospfv2SummaryLSA* summaryLSA = (Ospfv2SummaryLSA*) lsaHdr;
        NodeAddress netMask ;

        memcpy(&netMask, summaryLSA + 1, sizeof(NodeAddress));
        IO_ConvertIpAddressToString(netMask, addr1);

        sprintf(buf, " <summaryLsaBody>");
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLLsaHeader(node, lsaHdr);

        sprintf(buf, " %s %d %u </summaryLsaBody>",
            addr1,
            i,
            ((netMask + 1)) & 0x00FF);
        TRACE_WriteToBufferXML(node, buf);

    }
    else if ((lsaHdr->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
            || (lsaHdr->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL))
    {

        Ospfv2ExternalLinkInfo* linkInfo =
                    (Ospfv2ExternalLinkInfo*) lsaHdr ;

        IO_ConvertIpAddressToString(linkInfo->networkMask, addr1);
        IO_ConvertIpAddressToString(linkInfo->forwardingAddress, addr2);
        IO_ConvertIpAddressToString(linkInfo->externalRouterTag, addr3);

        sprintf(buf, " <externalLsaBody>");
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLLsaHeader(node, lsaHdr);

        sprintf(buf,
                " %s %d %hu %hu %s %s </externalLsaBody>",
                addr1,
                Ospfv2ExternalLinkGetEBit(linkInfo->ospfMetric),
                0, //linkInfo->reserved1,
                Ospfv2ExternalLinkGetMetric(linkInfo->ospfMetric),
                addr2,
                addr3);
        TRACE_WriteToBufferXML(node, buf);
    }
}


// /**
// FUNCTION   :: Ospfv2PrintTraceXML
// LAYER      :: NETWORK
// PURPOSE    :: Print out packet trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + msg  : Message* : Pointer to Message
// RETURN ::  void : NULL
// **/

void Ospfv2PrintTraceXML(Node* node, Message* msg)
{
    char buf[MAX_STRING_LENGTH];
    char addr1[20];
    char addr2[20];

    Ospfv2CommonHeader* header = (Ospfv2CommonHeader*)
                                         MESSAGE_ReturnPacket(msg);

    sprintf(buf, "<ospfv2>");
    TRACE_WriteToBufferXML(node, buf);

    if (header->packetType == OSPFv2_HELLO)
    {
        int i = 0;
        Ospfv2HelloPacket* helloPkt = (Ospfv2HelloPacket*)
                                            MESSAGE_ReturnPacket(msg);
        NodeAddress* tempNeighbor = (NodeAddress*)(helloPkt + 1);

        int numNeighbor = (helloPkt->commonHeader.packetLength
                      - sizeof(Ospfv2HelloPacket)) / sizeof(NodeAddress);

        sprintf(buf, " <helloPkt>");
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLCommonHeader(node, header);

        IO_ConvertIpAddressToString(helloPkt->networkMask, addr1);

        sprintf(buf, "%s %hd",
                addr1,
                helloPkt->helloInterval);
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLPacketOptions(node, helloPkt->options);

        IO_ConvertIpAddressToString(helloPkt->designatedRouter, addr1);
        IO_ConvertIpAddressToString(helloPkt->backupDesignatedRouter,
                                    addr2);
        sprintf(buf, " %hu %d %s %s",
                helloPkt->rtrPri,
                helloPkt->routerDeadInterval,
                addr1,
                addr2);
        TRACE_WriteToBufferXML(node, buf);

        if (numNeighbor)
        {
            sprintf(buf, " <neighborList>");
            TRACE_WriteToBufferXML(node, buf);

            for (i = 0; i < numNeighbor; i++)
            {
                IO_ConvertIpAddressToString(tempNeighbor[i], addr1);
                sprintf(buf, "%s ", addr1);
                TRACE_WriteToBufferXML(node, buf);
            }
            sprintf(buf, "</neighborList>");
            TRACE_WriteToBufferXML(node, buf);
        }

        sprintf(buf, "</helloPkt>");
        TRACE_WriteToBufferXML(node, buf);
    }
    else if (header->packetType == OSPFv2_DATABASE_DESCRIPTION)
    {

        Ospfv2DatabaseDescriptionPacket* dbDscrpPkt =
            (Ospfv2DatabaseDescriptionPacket*) MESSAGE_ReturnPacket(msg);
        int size = dbDscrpPkt->commonHeader.packetLength;
        Ospfv2LinkStateHeader* lsaHdr =
            (Ospfv2LinkStateHeader*) (dbDscrpPkt + 1);

        sprintf(buf, " <ddPkt>");
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLCommonHeader(node, header);

        sprintf(buf, " %hu", dbDscrpPkt->interfaceMtu);
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLPacketOptions(node, dbDscrpPkt->options);

        //DD flags
        sprintf(buf, " <ddFlags> %d %d %d %d %d %d %d %d </ddFlags>",
            0, //reserved
            0, //reserved
            0, //reserved
            0, //reserved
            0, //reserved
            Ospfv2DatabaseDescriptionPacketGetInit(dbDscrpPkt->ospfDbDp),
            Ospfv2DatabaseDescriptionPacketGetMore(dbDscrpPkt->ospfDbDp),
            Ospfv2DatabaseDescriptionPacketGetMS(dbDscrpPkt->ospfDbDp));
        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, " %hu", dbDscrpPkt->ddSequenceNumber);
        TRACE_WriteToBufferXML(node, buf);

        if (size -= sizeof(Ospfv2DatabaseDescriptionPacket))
        {
            for (; size > 0; size -= sizeof(Ospfv2LinkStateHeader))
            {
                Ospfv2PrintTraceXMLLsaHeader(node, lsaHdr);
                lsaHdr += 1;
            }
        }

        sprintf(buf, "</ddPkt>");
        TRACE_WriteToBufferXML(node, buf);
    }
    else if (header->packetType == OSPFv2_LINK_STATE_REQUEST)
    {
        Ospfv2LinkStateRequestPacket* reqPkt =
            (Ospfv2LinkStateRequestPacket*) MESSAGE_ReturnPacket(msg);

        int numLSReqObject = (reqPkt->commonHeader.packetLength
                         - sizeof(Ospfv2LinkStateRequestPacket))
                             / sizeof(Ospfv2LSRequestInfo);

        Ospfv2LSRequestInfo* lsaReqObject =
                (Ospfv2LSRequestInfo*) (reqPkt + 1);
        int i;

        sprintf(buf, "<requestPkt>");
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLCommonHeader(node, header);

        for (i = 0; i < numLSReqObject; i++)
        {
            IO_ConvertIpAddressToString(lsaReqObject->linkStateID, addr1);
            IO_ConvertIpAddressToString(lsaReqObject->advertisingRouter,
                                        addr2);

            sprintf(buf, " <reqInfo>%u %s %s</reqInfo>",
                    lsaReqObject->linkStateType,
                    addr1,
                    addr2);
            TRACE_WriteToBufferXML(node, buf);
        }
        sprintf(buf, "</requestPkt>");
        TRACE_WriteToBufferXML(node, buf);
    }
    else if (header->packetType == OSPFv2_LINK_STATE_UPDATE)
    {
        int count = 0;
        Ospfv2LinkStateUpdatePacket* updatePkt =
            (Ospfv2LinkStateUpdatePacket*) MESSAGE_ReturnPacket(msg);

        Ospfv2LinkStateHeader* LSHeader =
            (Ospfv2LinkStateHeader*) (updatePkt + 1);

        sprintf(buf, " <updatePkt>");
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLCommonHeader(node, header);

        sprintf(buf, " %d", updatePkt->numLSA);
        TRACE_WriteToBufferXML(node, buf);

        for (count = 0; count < updatePkt->numLSA; count++,
                LSHeader = (Ospfv2LinkStateHeader*)
                    (((char*) LSHeader) + LSHeader->length))
        {
            if (LSHeader->linkStateType == OSPFv2_GROUP_MEMBERSHIP)
            {
                MospfPrintTraceXMLLsa(node, LSHeader);
            }
            else
            {
                Ospfv2PrintTraceXMLLsa(node, LSHeader);
            }
        }
        sprintf(buf, " </updatePkt>");
        TRACE_WriteToBufferXML(node, buf);
    }
    else if (header->packetType == OSPFv2_LINK_STATE_ACK)
    {
        int count = 0;
        Ospfv2LinkStateAckPacket* ackPkt =
            (Ospfv2LinkStateAckPacket*) MESSAGE_ReturnPacket(msg);

        Ospfv2LinkStateHeader* LSHeader =
            (Ospfv2LinkStateHeader*) (ackPkt + 1);

        int numAck = (ackPkt->commonHeader.packetLength
                          - sizeof(Ospfv2LinkStateAckPacket))
                              / sizeof(Ospfv2LinkStateHeader);

        sprintf(buf, "<ackPkt>");
        TRACE_WriteToBufferXML(node, buf);

        Ospfv2PrintTraceXMLCommonHeader(node, header);

        for (count = 0; count < numAck; count++)
        {
            Ospfv2PrintTraceXMLLsaHeader(node, LSHeader);
            LSHeader = LSHeader + 1;
        }
        sprintf(buf, "</ackPkt>");
        TRACE_WriteToBufferXML(node, buf);
    }
    sprintf(buf, "</ospfv2>");
    TRACE_WriteToBufferXML(node, buf);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2InitTrace
// PURPOSE      :Initialize trace from user configuration.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//
static
void Ospfv2InitTrace(Node* node, const NodeInput* nodeInput)
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
        "TRACE-OSPFv2",
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
                "TRACE-OSPFv2 should be either \"YES\" or \"NO\".\n");
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
            TRACE_EnableTraceXML(node, TRACE_OSPFv2,
                "OSPFv2", Ospfv2PrintTraceXML, writeMap);
    }
    else
    {
            TRACE_DisableTraceXML(node, TRACE_OSPFv2,
                "OSPFv2", writeMap);
    }
    writeMap = FALSE;
}

//-------------------------------------------------------------------------//
//                             HANDLING LIST                               //
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// NAME: Ospfv2InitList
// PURPOSE: Initialize a list structure.
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2InitList(Ospfv2List** list)
{
    Ospfv2List* tmpList = (Ospfv2List*) MEM_malloc (sizeof(Ospfv2List));

    tmpList->size = 0;
    tmpList->first = tmpList->last = NULL;
    *list = tmpList;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2InitNonBroadcastNeighborList
// PURPOSE: Initialize Non Broadacast  list structure.
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2InitNonBroadcastNeighborList(
    Ospfv2NonBroadcastNeighborList** list)
{
    Ospfv2NonBroadcastNeighborList* tmpList =
        (Ospfv2NonBroadcastNeighborList*)
        MEM_malloc (sizeof(Ospfv2NonBroadcastNeighborList));

    tmpList->size = 0;
    tmpList->first = tmpList->last = NULL;
    *list = tmpList;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2InsertToList
// PURPOSE: Inserts an item to a list
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2InsertToList(
    Ospfv2List* list,
    clocktype timeStamp,
    void* data)
{
    Ospfv2ListItem* listItem = (Ospfv2ListItem*)
        MEM_malloc(sizeof(Ospfv2ListItem));

    listItem->data = data;
    listItem->timeStamp = timeStamp;
    listItem->unreachableTimeStamp = (clocktype)0;
    listItem->next = NULL;

#ifdef ADDON_BOEINGFCS
    listItem->referenceCount = NULL;
    listItem->numReferences = 0;
#endif

    if (list->size == 0)
    {
        // Only item in the list.
        listItem->prev = NULL;
        list->last = listItem;
        list->first = list->last;
    }
    else
    {
        // Insert at end of list.
        listItem->prev = list->last;
        list->last->next = listItem;
        list->last = listItem;
    }

    list->size++;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2InsertToNonBroadcastNeighborList
// PURPOSE: Inserts an item to Non Broadcast neighbor list
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2InsertToNonBroadcastNeighborList(
    Ospfv2NonBroadcastNeighborList* list,
    NodeAddress addr)
{
    Ospfv2NonBroadcastNeighborListItem* listItem =
        (Ospfv2NonBroadcastNeighborListItem*)
        MEM_malloc(sizeof(Ospfv2NonBroadcastNeighborListItem));
    listItem->neighborAddr = addr;
    listItem->next = NULL;
    if (list->size == 0)
    {
        listItem->prev = NULL;
        list->last = listItem;
        list->first = list->last;
    }
    else
    {
        // Insert at end of list.
        listItem->prev = list->last;
        list->last->next = listItem;
        list->last = listItem;
    }
    list->size++;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2InsertToListReferenceCount
// PURPOSE: Inserts an item to neighbor retransmission list and
//          increments the reference count
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2InsertToListReferenceCount(Ospfv2List* list,
                                      clocktype timeStamp,
                                      void* data,
                                      int* referenceCount)
{
    Ospfv2ListItem* listItem = (Ospfv2ListItem*)
        MEM_malloc(sizeof(Ospfv2ListItem));

    listItem->data = data;
    listItem->timeStamp = timeStamp;
    listItem->unreachableTimeStamp = (clocktype)0;
    listItem->next = NULL;

#ifdef ADDON_BOEINGFCS
    listItem->referenceCount = referenceCount;
    if (referenceCount != NULL)
    {
        (*referenceCount)++;
    }
    listItem->numReferences = 0;
#endif

    if (list->size == 0)
    {
        // Only item in the list.
        listItem->prev = NULL;
        list->last = listItem;
        list->first = list->last;
    }
    else
    {
        // Insert at end of list.
        listItem->prev = list->last;
        list->last->next = listItem;
        list->last = listItem;
    }

    list->size++;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2InsertToNeighborSendList
// PURPOSE: Inserts an item to neighbor send list
// RETURN: None.
//-------------------------------------------------------------------------//
void Ospfv2InsertToNeighborSendList(Ospfv2List* list,
                                    clocktype timeStamp,
                                    Ospfv2LinkStateHeader* LSHeader)
{
    Ospfv2SendLSAInfo* sendLSAInfo =
        (Ospfv2SendLSAInfo *) MEM_malloc(sizeof(Ospfv2SendLSAInfo));

    sendLSAInfo->advertisingRouter = LSHeader->advertisingRouter;
    sendLSAInfo->linkStateID = LSHeader->linkStateID;
    sendLSAInfo->linkStateType = LSHeader->linkStateType;
    Ospfv2InsertToList(list, timeStamp, (void*) sendLSAInfo);
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2RemoveFromList
// PURPOSE: This a general list handling function. Various lists are used in
//          the OSPF structure. Removes an item from the list. "type" is used
//          to indicate whether the list item contains a Message
//          structure or not. If so, call MESSAGE_Free() instead of
//          MEM_free().
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2RemoveFromList(
    Node* node,
    Ospfv2List* list,
    Ospfv2ListItem* listItem,
    BOOL type)
{
    Ospfv2ListItem* nextListItem = NULL;

    if (list == NULL || listItem == NULL)
    {
        return;
    }

    if (list->size == 0)
    {
        return;
    }

    nextListItem = listItem->next;

    if (list->size == 1)
    {
        list->first = list->last = NULL;
    }
    else if (list->first == listItem)
    {
        list->first = listItem->next;
        list->first->prev = NULL;
    }
    else if (list->last == listItem)
    {
        list->last = listItem->prev;
        list->last->next = NULL;
    }
    else
    {
        listItem->prev->next = listItem->next;
        if (listItem->prev->next != NULL)

        {
            listItem->next->prev = listItem->prev;
        }
    }

    list->size--;

#ifdef ADDON_BOEINGFCS
    // If using reference count, decrement and free
    if (listItem->referenceCount != NULL)
    {
        (*listItem->referenceCount)--;
        if (*listItem->referenceCount <= 0)
        {
            // All references gone, free memory
            MEM_free(listItem->referenceCount);
            if (type == FALSE)
            {
                MEM_free(listItem->data);
            }
            else
            {
                MESSAGE_Free(node, (Message*)listItem->data);
            }
        }
    }
    else if (listItem->data != NULL)
    {
        // No reference count, free as usual
        if (type == FALSE)
        {
            MEM_free(listItem->data);
        }
        else
        {
            MESSAGE_Free(node, (Message*)listItem->data);
        }
    }
#else
    if (listItem->data != NULL)
    {
        if (type == FALSE)
        {
            MEM_free(listItem->data);
        }
        else
        {
            MESSAGE_Free(node, (Message*)listItem->data);
        }
    }
#endif

    MEM_free (listItem);
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2FreeList
// PURPOSE: Free list and its allocated memory.  "type" is used
//          to indicate whether the list contains a Message
//          structure or not.  If so, call MESSAGE_Free() instead of
//          MEM_free().
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2FreeList(Node* node, Ospfv2List* list, BOOL type)
{
    Ospfv2ListItem* item = NULL;
    Ospfv2ListItem* tempItem = NULL;

    item = list->first;

    while (item)
    {
        tempItem = item;
        item = item->next;

#ifdef ADDON_BOEINGFCS
        // If using reference count, decrement and free
        if (tempItem->referenceCount != NULL)
        {
            (*tempItem->referenceCount)--;
            if (*tempItem->referenceCount <= 0)
            {
                // All references gone, free memory
                MEM_free(tempItem->referenceCount);
                if (type == FALSE)
                {
                    MEM_free(tempItem->data);
                }
                else
                {
                    MESSAGE_Free(node, (Message*)tempItem->data);
                }
            }
        }
        else if (tempItem->data != NULL)
        {
            // No reference count, free as usual
            if (type == FALSE)
            {
                MEM_free(tempItem->data);
            }
            else
            {
                MESSAGE_Free(node, (Message*)tempItem->data);
            }
        }
#else
        if (tempItem->data != NULL)
        {
            if (type == FALSE)
            {
                MEM_free(tempItem->data);
            }
            else
            {
                MESSAGE_Free(node, (Message*)tempItem->data);
            }
        }
#endif
        MEM_free(tempItem);
    }

    MEM_free(list);
}

//-------------------------------------------------------------------------//
// NAME     : Ospfv2DeleteList
// PURPOSE  : Free all items of list but doesn't free list.  "type" is used
//            to indicate whether the list contains a Message
//            structure or not.  If so, call MESSAGE_Free() instead of
//            MEM_free().
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2DeleteList(Node* node, Ospfv2List* list, BOOL type)
{
    Ospfv2ListItem* item = NULL;
    Ospfv2ListItem* tempItem = NULL;

    item = list->first;

    while (item)
    {
        tempItem = item;
        item = item->next;

#ifdef ADDON_BOEINGFCS
        // If using reference count, decrement and free
        if (tempItem->referenceCount != NULL)
        {
            (*tempItem->referenceCount)--;
            MEM_free(tempItem->referenceCount);
            if (*tempItem->referenceCount <= 0)
            {
                // All references gone, free memory
                if (type == FALSE)
                {
                    MEM_free(tempItem->data);
                }
                else
                {
                    MESSAGE_Free(node, (Message*)tempItem->data);
                }
            }
        }
        else if (tempItem->data != NULL)
        {
            // No reference count, free as usual
            if (type == FALSE)
            {
                MEM_free(tempItem->data);
            }
            else
            {
                MESSAGE_Free(node, (Message*)tempItem->data);
            }
        }
#else
        if (tempItem->data != NULL)
        {
            if (type == FALSE)
            {
                MEM_free(tempItem->data);
            }
            else
            {
                MESSAGE_Free(node, (Message*)tempItem->data);
            }
        }
#endif
        MEM_free(tempItem);
        list->size--;
    }
#ifdef JNE_LIB
    if (list->size != 0)
    {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "List is being deleted!! Size must be 0",
                              JNE::CRITICAL);
    }
#endif

    ERROR_Assert(list->size == 0,
        "List is being deleted!! Size must be 0\n");
    memset(list, 0, sizeof(Ospfv2List));

}


//-------------------------------------------------------------------------//
// NAME: Ospfv2ListIsEmpty()
// PURPOSE: Check whether a list is empty.
// RETURN: BOOL
//-------------------------------------------------------------------------//

BOOL Ospfv2ListIsEmpty(Ospfv2List* list)
{
    return ((list->size == 0) ? TRUE : FALSE);
}


//-------------------------------------------------------------------------//
//                            COMMON HEADER                                //
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// NAME: Ospfv2CalcCommonHeaderCheckSum
// PURPOSE: Calculate checksum for packet.
// RETURN: 0 for now.
//-------------------------------------------------------------------------//

static
short int Ospfv2CalcCommonHeaderCheckSum(
    Ospfv2CommonHeader* cmnHdr)
{
    return 0;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2CreateCommonHeader
// PURPOSE: Create common OSPF packet header for all packets.
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2CreateCommonHeader(
    Node* node,
    Ospfv2CommonHeader* commonHdr,
    Ospfv2PacketType pktType)
{
    Ospfv2Data* ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                     node, ROUTING_PROTOCOL_OSPFv2);

    commonHdr->version = OSPFv2_CURRENT_VERSION;
    commonHdr->packetType = (unsigned char) pktType;
    commonHdr->routerID = ospf->routerID;
    commonHdr->areaID = 0x0;
    commonHdr->checkSum = Ospfv2CalcCommonHeaderCheckSum(commonHdr);

    // Authentication not simulated.
    commonHdr->authenticationType = 0x0;
    commonHdr->authenticationData[0] = 0;
}


//-------------------------------------------------------------------------//
//                           OTHER FUNCTIONS                               //
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// NAME: Ospfv2ScheduleNeighborEvent
// PURPOSE: Schedule a neighbor event to be executed later. Primary
//          objective is to call Ospfv2HandleNeighborEvent() asynchronously
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2ScheduleNeighborEvent(
    Node* node,
    int interfaceId,
    NodeAddress nbrAddr,
    Ospfv2NeighborEvent nbrEvent)
{
    clocktype delay;
    Ospfv2NSMTimerInfo* NSMTimerInfo = NULL;

    Message* newMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    ROUTING_PROTOCOL_OSPFv2,
                                    MSG_ROUTING_OspfNeighborEvent);

#ifdef ADDON_NGCNMS
    MESSAGE_SetInstanceId(newMsg, interfaceId);
#endif

    MESSAGE_InfoAlloc(node, newMsg,  sizeof(Ospfv2NSMTimerInfo));

    NSMTimerInfo = (Ospfv2NSMTimerInfo*) MESSAGE_ReturnInfo(newMsg);

    NSMTimerInfo->interfaceId = interfaceId;
    NSMTimerInfo->nbrAddr = nbrAddr;
    NSMTimerInfo->nbrEvent = nbrEvent;

    delay = OSPFv2_EVENT_SCHEDULING_DELAY;
    MESSAGE_Send(node, newMsg, (clocktype) delay);
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2ScheduleInterfaceEvent
// PURPOSE: Schedule an interface event to be executed later. Primary
//          objective is to call Ospfv2HandleInterfaceEvent() asynchronously
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2ScheduleInterfaceEvent(
        Node* node,
        int interfaceId,
        Ospfv2InterfaceEvent intfEvent)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Message* newMsg = NULL;
    clocktype delay;
    Ospfv2ISMTimerInfo* ISMTimerInfo = NULL;

    // Timer is already scheduled
    if (ospf->iface[interfaceId].ISMTimer)
    {
        return;
    }

    newMsg = MESSAGE_Alloc(node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_ROUTING_OspfInterfaceEvent);

#ifdef ADDON_NGCNMS
    MESSAGE_SetInstanceId(newMsg, interfaceId);
#endif

    MESSAGE_InfoAlloc(node, newMsg,  sizeof(Ospfv2ISMTimerInfo));

    ISMTimerInfo = (Ospfv2ISMTimerInfo*) MESSAGE_ReturnInfo(newMsg);

    ISMTimerInfo->interfaceId = interfaceId;
    ISMTimerInfo->intfEvent = intfEvent;

    ospf->iface[interfaceId].ISMTimer = TRUE;
    delay = OSPFv2_EVENT_SCHEDULING_DELAY;

    MESSAGE_Send(node, newMsg, (clocktype) delay);
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2GetNeighborInfoByIPAddress
// PURPOSE:  Get the neighbor structure from the neighbor list using the
//           neighbor's IP address.
// RETURN: The neighbor structure we are looking for, or NULL if not found.
//-------------------------------------------------------------------------//

Ospfv2Neighbor* Ospfv2GetNeighborInfoByIPAddress(
    Node* node,
    int interfaceIndex,
    NodeAddress neighborAddr)
{
    Ospfv2ListItem* tempListItem = NULL;
    Ospfv2Neighbor* tempNeighborInfo = NULL;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    //we should search for only this interface neighbors
    //therefore this code is been commented during virtual link support
    //for (i = 0; i < node->numberInterfaces; i++)
    //{
    //    if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
    //    {
    //        continue;
    //    }
    //    tempListItem = ospf->iface[i].neighborList->first;

       tempListItem = ospf->iface[interfaceIndex].neighborList->first;

        // Search for the neighbor in my neighbor list.
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;
#ifdef JNE_LIB
            if (!tempNeighborInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Neighbor not found in the Neighbor list",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(tempNeighborInfo,
                "Neighbor not found in the Neighbor list");

            if (tempNeighborInfo->neighborIPAddress == neighborAddr)
            {
                // Found the neighbor we are looking for.
                return tempNeighborInfo;
            }

            tempListItem = tempListItem->next;
        }
 //   }

    return NULL;
}


//-------------------------------------------------------------------------//
// NAME     : Ospfv2GetInterfaceForThisNeighbor
// PURPOSE  : Get interface on which this neighbor is attached.
// RETURN   : The interface index we are looking for, or -1 if not found.
//-------------------------------------------------------------------------//

int Ospfv2GetInterfaceForThisNeighbor(
    Node* node,
    NodeAddress neighborAddr)
{
    int i;
    Ospfv2ListItem* tempListItem = NULL;
    Ospfv2Neighbor* tempNeighborInfo = NULL;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            continue;
        }

        tempListItem = ospf->iface[i].neighborList->first;

        // Search for the neighbor in my neighbor list.
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;
#ifdef JNE_LIB
            if (!tempNeighborInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Neighbor not found in the Neighbor list",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(tempNeighborInfo,
                "Neighbor not found in the Neighbor list");

            if (tempNeighborInfo->neighborIPAddress == neighborAddr)
            {
                // Found the neighbor we are looking for.
                return i;
            }

            tempListItem = tempListItem->next;
        }
    }

    return -1;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2FreeRequestList
// PURPOSE      :Free Link State Request list for this neighbor.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2FreeRequestList(Node* node, Ospfv2Neighbor* nbrInfo)
{
    Ospfv2ListItem* listItem = nbrInfo->linkStateRequestList->first;

    while (listItem)
    {
        Ospfv2LSReqItem* itemInfo = (Ospfv2LSReqItem*) listItem->data;

        // Remove item from list
        MEM_free(itemInfo->LSHeader);

        listItem = listItem->next;
    }

    Ospfv2FreeList(node, nbrInfo->linkStateRequestList, FALSE);
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2RemoveNeighborFromNeighborList
// PURPOSE:  Remove a neighbor from the neighbor list.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2RemoveNeighborFromNeighborList(
    Node* node,
    int interfaceIndex,
    NodeAddress neighborAddr)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2ListItem* tempListItem =
        ospf->iface[interfaceIndex].neighborList->first;
    Ospfv2Neighbor* tempNeighborInfo = NULL;

    // Search for the neighbor in my neighbor list.
    while (tempListItem)
    {
        tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;
#ifdef JNE_LIB
            if (!tempNeighborInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Neighbor not found in the Neighbor list",
                    JNE::CRITICAL);
            }
#endif
        ERROR_Assert(tempNeighborInfo,
            "Neighbor not found in the Neighbor list");

        // Found and delete.
        if (tempNeighborInfo->neighborIPAddress == neighborAddr)
        {
            if (OSPFv2_DEBUG_ERRORS)
            {
                char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(neighborAddr, nbrAddrStr);
                printf("Node %u removing %s from neighbor list\n",
                        node->nodeId, nbrAddrStr);
            }

            // Clear lists.
            Ospfv2FreeList(node,
                           tempNeighborInfo->linkStateRetxList,
                           FALSE);
            Ospfv2FreeList(node,
                           tempNeighborInfo->linkStateSendList,
                           FALSE);

            Ospfv2FreeList(node,
                           tempNeighborInfo->DBSummaryList,
                           FALSE);

            Ospfv2FreeRequestList(node, tempNeighborInfo);

            if (tempNeighborInfo->lastSentDDPkt != NULL)
            {
                MESSAGE_Free(node, tempNeighborInfo->lastSentDDPkt);
            }

            if (tempNeighborInfo->lastReceivedDDPacket != NULL)
            {
                MESSAGE_Free(node, tempNeighborInfo->lastReceivedDDPacket);
            }

#ifdef ADDON_BOEINGFCS
            if (! ROSPF_DEBUG_DBDG_VERIFICATION)
            {
                delete tempNeighborInfo->dbdgNbr;
            }
            else if (tempNeighborInfo->dbdgNbr != NULL)
            {
                tempNeighborInfo->dbdgNbr->cancelTimers();
            }
#endif

            // Then remove neighbor from neighbor list.
            Ospfv2RemoveFromList(
                                node,
                                ospf->iface[interfaceIndex].neighborList,
                                tempListItem,
                                FALSE);
            ospf->neighborCount--;
            return;
        }

        tempListItem = tempListItem->next;
    }

#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Neighbor not found in the Neighbor list",
                              JNE::CRITICAL);
#endif
    // Neighbor info should have been found.
    ERROR_Assert(FALSE, "Neighbor not found in the Neighbor list");
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2IsNSSA_ABR
// PURPOSE: Determine if an advertising address is a NSSA ABR.
// RETURN: TRUE if it is NSSA ABR, FALSE otherwise.
//-------------------------------------------------------------------------//
static
BOOL Ospfv2IsNSSA_ABR(Node* node, NodeAddress advtAddr)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* itemSummaryLSA = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    int i = 0;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        Ospfv2Interface* thisInterface = &ospf->iface[i];
        Ospfv2Area* thisArea = Ospfv2GetArea(node, thisInterface->areaId);

        if (thisArea && !thisArea->isNSSAEnabled)
        {
            continue;
        }

        //For Router Summary LSA
        itemSummaryLSA = thisArea->routerSummaryLSAList->first;

        while (itemSummaryLSA)
        {
            LSHeader = ((Ospfv2LinkStateHeader*) itemSummaryLSA->data);

            if (advtAddr == LSHeader->advertisingRouter)
            {
                return TRUE;
            }
            itemSummaryLSA = itemSummaryLSA->next;
        }

        //For Network Summary LSA
        itemSummaryLSA = thisArea->networkSummaryLSAList->first;

        while (itemSummaryLSA)
        {
            LSHeader = ((Ospfv2LinkStateHeader*) itemSummaryLSA->data);

            if (advtAddr == LSHeader->advertisingRouter)
            {
                return TRUE;
            }
            itemSummaryLSA = itemSummaryLSA->next;
        }
    }
    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2IsHighestRouterID
// PURPOSE: Determine if my router ID is highest in all reachable NSSA ABR.
// RETURN: TRUE if it is highest, FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2IsHighestRouterID(Node* node, NodeAddress routerID)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    NodeAddress highestRouterId = routerID;
    Ospfv2ListItem* itemSummaryLSA = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    int i = 0;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        Ospfv2Interface* thisInterface = &ospf->iface[i];
        Ospfv2Area* thisArea = Ospfv2GetArea(node, thisInterface->areaId);

        if (thisArea && !thisArea->isNSSAEnabled)
        {
            continue;
        }

        //For Router Summary LSA
        itemSummaryLSA = thisArea->routerSummaryLSAList->first;

        while (itemSummaryLSA)
        {
            LSHeader = ((Ospfv2LinkStateHeader*) itemSummaryLSA->data);

            if (highestRouterId < LSHeader->advertisingRouter)
            {
                highestRouterId = LSHeader->advertisingRouter;
            }
            itemSummaryLSA = itemSummaryLSA->next;
        }

        //For Network Summary LSA
        itemSummaryLSA = thisArea->networkSummaryLSAList->first;

        while (itemSummaryLSA)
        {
            LSHeader = ((Ospfv2LinkStateHeader*) itemSummaryLSA->data);

            if (highestRouterId < LSHeader->advertisingRouter)
            {
                highestRouterId = LSHeader->advertisingRouter;
            }
            itemSummaryLSA = itemSummaryLSA->next;
        }
    }

    if (highestRouterId == routerID)
    {
        return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2IsMyAddress
// PURPOSE: Determine if an IP address is my own.
// RETURN: TRUE if it's my IP address, FALSE otherwise.
//-------------------------------------------------------------------------//

BOOL Ospfv2IsMyAddress(Node* node, NodeAddress addr)
{
    int i;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (addr == NetworkIpGetInterfaceAddress(node, i))
        {
            return TRUE;
        }
    }

    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2SetTimer
// PURPOSE: Set timers of various types.
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2SetTimer(
    Node* node,
    int interfaceIndex,
    int eventType,
    NodeAddress neighborAddr,
    clocktype timerDelay,
    unsigned int sequenceNumber,
    NodeAddress advertisingRouter,
    Ospfv2PacketType type)
{
    Ospfv2TimerInfo* info = NULL;
    Message* newMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    ROUTING_PROTOCOL_OSPFv2,
                                    eventType);

#ifdef ADDON_NGCNMS
    MESSAGE_SetInstanceId(newMsg, interfaceIndex);
#endif

    MESSAGE_InfoAlloc(node, newMsg, sizeof(Ospfv2TimerInfo));
    info = (Ospfv2TimerInfo*) MESSAGE_ReturnInfo(newMsg);
    info->neighborAddr = neighborAddr;
    info->interfaceIndex = interfaceIndex;

    if (OSPFv2_DEBUG_ERRORS)
    {
        char clockStr[100];
        TIME_PrintClockInSecond(node->getNodeTime()+getSimStartTime(node),
            clockStr);
        printf("    setting timer at time %s\n", clockStr);
        printf("        type is ");
    }

    switch (eventType)
    {
        case MSG_ROUTING_OspfInactivityTimer:
        {
            Ospfv2Neighbor* neighborInfo =
                Ospfv2GetNeighborInfoByIPAddress(node,
                                                 interfaceIndex,
                                                 neighborAddr);
#ifdef JNE_LIB
            if (!neighborInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Neighbor Info not found",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(neighborInfo, "Neighbor Info not found");

            info->sequenceNumber =
                    ++neighborInfo->inactivityTimerSequenceNumber;

            if (OSPFv2_DEBUG_ERRORS)
            {
                printf("InactivityTimer with seqno %u\n",
                    info->sequenceNumber);
            }

            break;
        }
        case MSG_ROUTING_OspfRxmtTimer:
        {
            info->sequenceNumber = sequenceNumber;
            info->advertisingRouter = advertisingRouter;
            info->type = type;

            if (OSPFv2_DEBUG_ERRORS)
            {
                printf("RxmtTimer with seqno %u\n", info->sequenceNumber);
            }

            break;
        }
        default:
            // Shouldn't get here.
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Invalid Timer EventType",
                              JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, "Invalid Timer EventType\n");
    }

    if (OSPFv2_DEBUG_ERRORS)
    {
        char clockStr[100];
        TIME_PrintClockInSecond(node->getNodeTime() + getSimStartTime(node) +
                                timerDelay, clockStr);
        printf("        sequence number is %d\n", info->sequenceNumber);
        printf("        to expired at %s\n", clockStr);
    }

    MESSAGE_Send(node, newMsg, timerDelay);
}


// BGP-OSPF Patch Start
//-------------------------------------------------------------------------//
// NAME: Ospfv2NeighborIsInStubArea
// PURPOSE:  Check Neighbor is in STUB area or not.
// RETURN: If the interface belong into STUB area return TRUE otherwise FALSE
//-------------------------------------------------------------------------//

BOOL Ospfv2NeighborIsInStubArea(
    Node* node,
    int interfaceId,
    Ospfv2Neighbor* nbrInfo)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);


    Ospfv2Area* thisArea;

    thisArea = Ospfv2GetArea(node, ospf->iface[interfaceId].areaId);

    if (thisArea->externalRoutingCapability == FALSE)
    {
        return TRUE;
    }

    return  FALSE;
}
// BGP-OSPF Patch End

//-------------------------------------------------------------------------//
// NAME: Ospfv2NeighborIsInNSSAArea
// PURPOSE:  Check Neighbor is in NSSA area or not.
// RETURN: If the interface belong into NSSA area return TRUE otherwise FALSE
//-------------------------------------------------------------------------//

BOOL Ospfv2NeighborIsInNSSAArea(
    Node* node,
    int interfaceId,
    Ospfv2Neighbor* nbrInfo)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Area* thisArea;
    thisArea = Ospfv2GetArea(node, ospf->iface[interfaceId].areaId);

    return (thisArea && thisArea->isNSSAEnabled);
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2UpdateDBSummaryList()
// PURPOSE      :Create database summary list for this neighbor
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------//

static
void Ospfv2UpdateDBSummaryList(
    Node* node,
    int interfaceIndex,
    Ospfv2List* list,
    Ospfv2Neighbor* nbrInfo)
{

    Ospfv2ListItem* item = list->first;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    while (item)
    {
        Ospfv2LinkStateHeader* LSHeader = NULL;

        // Get LSA...
        Ospfv2LinkStateHeader* listLSHeader =
            (Ospfv2LinkStateHeader*) item->data;

        // Add to retx list if LSAge is MaxAge
        if (Ospfv2MaskDoNotAge(ospf, listLSHeader->linkStateAge) ==
            (OSPFv2_LSA_MAX_AGE / SECOND))
        {
            Ospfv2AddToLSRetxList(node,
                                  interfaceIndex,
                                  nbrInfo,
                                  (char*) listLSHeader,
                                  NULL);
            item = item->next;
            continue;
        }

        // Create new item to insert to list
        LSHeader = (Ospfv2LinkStateHeader*)
                MEM_malloc(sizeof(Ospfv2LinkStateHeader));

        memcpy(LSHeader, listLSHeader, sizeof(Ospfv2LinkStateHeader));

        Ospfv2InsertToList(nbrInfo->DBSummaryList,
                           0,
                           (void*) LSHeader);

        item = item->next;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2CreateDBSummaryList()
// PURPOSE      :Create database summary list for this neighbor
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------//

static
void Ospfv2CreateDBSummaryList(
    Node* node,
    int interfaceIndex,
    Ospfv2Neighbor* nbrInfo)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Area* thisArea =
        Ospfv2GetArea(node, ospf->iface[interfaceIndex].areaId);

#ifdef JNE_LIB
    if (interfaceIndex == -1)
    {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Unknown Neighbor",
                              JNE::CRITICAL);
    }
#endif
    ERROR_Assert(interfaceIndex != -1, "Unknown Neighbor\n");
#ifdef JNE_LIB
    if (!thisArea)
    {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Area doesn't exist",
                              JNE::CRITICAL);
    }
#endif
    ERROR_Assert(thisArea, "Area doesn't exist\n");

    if (Ospfv2DebugSync(node))
    {
        printf("    DB summary list before update\n");
        Ospfv2PrintDBSummaryList(node, nbrInfo->DBSummaryList);
    }

    // Add router LSA Sumary
    Ospfv2UpdateDBSummaryList(node,
                              interfaceIndex,
                              thisArea->routerLSAList,
                              nbrInfo);

    // Add network LSA Summary
    Ospfv2UpdateDBSummaryList(node,
                              interfaceIndex,
                              thisArea->networkLSAList,
                              nbrInfo);

    // Add summary LSA Summary
    Ospfv2UpdateDBSummaryList(node,
                              interfaceIndex,
                              thisArea->routerSummaryLSAList,
                              nbrInfo);
    Ospfv2UpdateDBSummaryList(node,
                              interfaceIndex,
                              thisArea->networkSummaryLSAList,
                              nbrInfo);

    //TBD: For virtual neighbor, AS-Extarnal LSA should not be included.
    // BGP-OSPF Patch Start
    if (!Ospfv2NeighborIsInStubArea(node, interfaceIndex, nbrInfo))
    {
        Ospfv2UpdateDBSummaryList(node,
                                  interfaceIndex,
                                  ospf->asExternalLSAList,
                                  nbrInfo);

        /***** Start: OPAQUE-LSA *****/
        if (ospf->opaqueCapable &&
            Ospfv2DatabaseDescriptionPacketGetO(nbrInfo->dbDescriptionNbrOptions))
        {
            Ospfv2UpdateDBSummaryList(node,
                                      interfaceIndex,
                                      ospf->ASOpaqueLSAList,
                                      nbrInfo);
        }
        /***** End: OPAQUE-LSA *****/
    }
    // BGP-OSPF Patch End
    if (Ospfv2NeighborIsInNSSAArea(node, interfaceIndex, nbrInfo))
    {
        Ospfv2UpdateDBSummaryList(node,
                                  interfaceIndex,
                                  ospf->nssaExternalLSAList,
                                  nbrInfo);
    }

    if (ospf->isMospfEnable == TRUE)
    {
        // Add Group Membership LSA Summary
        Ospfv2UpdateDBSummaryList(
            node, interfaceIndex, thisArea->groupMembershipLSAList, nbrInfo);
    }

    if (Ospfv2DebugSync(node))
    {
        printf("    DB summary list after update\n");

        Ospfv2PrintDBSummaryList(node, nbrInfo->DBSummaryList);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2AdjacencyRequire()
// PURPOSE      :Determine whether adjacency is required with this neighbor.
// ASSUMPTION   :Network type Point-to-Multipoint and
//              :Virtual link are not considered.
// RETURN VALUE :BOOL
//-------------------------------------------------------------------------//

static
BOOL Ospfv2AdjacencyRequire(
    Node* node,
    int interfaceId,
    Ospfv2Neighbor* nbrInfo)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
#ifdef ADDON_BOEINGFCS
    if (ospf->iface[interfaceId].type ==
        OSPFv2_ROSPF_INTERFACE)
    {
        return RoutingCesRospfAdjacencyRequired(node,
                                                interfaceId,
                                                nbrInfo->neighborIPAddress,
                                                nbrInfo->neighborNodeId);
    }
    else
#endif
    if ((ospf->iface[interfaceId].type ==
            OSPFv2_POINT_TO_POINT_INTERFACE)
        ||  (ospf->iface[interfaceId].type ==
                OSPFv2_POINT_TO_MULTIPOINT_INTERFACE)
        || (ospf->iface[interfaceId].designatedRouter ==
                ospf->routerID)
        || (ospf->iface[interfaceId].backupDesignatedRouter ==
                ospf->routerID)
        || (ospf->iface[interfaceId].designatedRouter ==
                nbrInfo->neighborID)
        || (ospf->iface[interfaceId].backupDesignatedRouter ==
                nbrInfo->neighborID))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2DestroyAdjacency()
// PURPOSE      :Destroy possibly formed adjacency.
// ASSUMPTION   :None.
// RETURN VALUE :None
//-------------------------------------------------------------------------//
void Ospfv2DestroyAdjacency(
    Node* node,
    Ospfv2Neighbor* nbrInfo,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    if (Ospfv2DebugSync(node))
    {
        char addrStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress, addrStr);
        printf("Node %d TEARING DOWN adjacency with nbr %s\n",
               node->nodeId, addrStr);
    }

    // Clear lists.

    Ospfv2FreeList(node, nbrInfo->linkStateRetxList, FALSE);
    Ospfv2FreeList(node, nbrInfo->linkStateSendList, FALSE);
    Ospfv2FreeList(node, nbrInfo->DBSummaryList, FALSE);
    Ospfv2FreeRequestList(node, nbrInfo);

    Ospfv2InitList(&nbrInfo->linkStateRetxList);
    Ospfv2InitList(&nbrInfo->linkStateSendList);
    Ospfv2InitList(&nbrInfo->DBSummaryList);
    Ospfv2InitList(&nbrInfo->linkStateRequestList);

    nbrInfo->LSReqTimerSequenceNumber += 1;

    if (nbrInfo->lastSentDDPkt != NULL)
    {
        MESSAGE_Free(node, nbrInfo->lastSentDDPkt);
        nbrInfo->lastSentDDPkt = NULL;
    }

#ifdef ADDON_BOEINGFCS
    if (! ROSPF_DEBUG_DBDG_VERIFICATION)
    {
        delete nbrInfo->dbdgNbr;
    }
    else if (nbrInfo->dbdgNbr != NULL)
    {
        nbrInfo->dbdgNbr->cancelTimers();
    }
    nbrInfo->dbdgNbr = NULL;
#endif
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleNeighborEvent()
// PURPOSE      :Handle neighbor event and change neighbor state accordingly
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------//

void Ospfv2HandleNeighborEvent(
    Node* node,
    int interfaceIndex,
    NodeAddress nbrAddr,
    Ospfv2NeighborEvent eventType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    // Find neighbor structure
    Ospfv2Neighbor* tempNeighborInfo =
        Ospfv2GetNeighborInfoByIPAddress(node, interfaceIndex, nbrAddr);

#ifdef ADDON_BOEINGFCS
    if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE &&
        !tempNeighborInfo)
    {
        // do nothing, but is possible due to mobility that is not
        // accounted for in standard OSPF
        return;
    }
    else
    {
#ifdef JNE_LIB
        if (!tempNeighborInfo)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Neighbor not found in the Neighbor list",
                              JNE::CRITICAL);
        }
#endif
        ERROR_Assert(tempNeighborInfo,
            "Neighbor not found in the Neighbor list");
    }
#else
#ifdef JNE_LIB
        if (!tempNeighborInfo)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Neighbor not found in the Neighbor list",
                              JNE::CRITICAL);
        }
#endif
    ERROR_Assert(tempNeighborInfo,
        "Neighbor not found in the Neighbor list");
#endif

    switch (eventType)
    {
        case E_HelloReceived:
        {
            if (tempNeighborInfo->state < S_Init)
            {
                if (Ospfv2DebugSync(node))
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    NSM receive event E_HelloReceived\n"
                        "    neighbor state (%s) move up to S_Init\n",
                        nbrAddrStr);
                }

                Ospfv2SetNeighborState(node,
                                       interfaceIndex,
                                       tempNeighborInfo,
                                       S_Init);
            }
            else
            {
                if (Ospfv2DebugSync(node))
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    NSM receive event E_HelloReceived\n"
                        "    No change in neighbor state (%s)\n",
                        nbrAddrStr);
                }
            }

#ifdef ADDON_BOEINGFCS
            if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
            {
                RoutingCesRospfHandleDRTimer(node, interfaceIndex, false);
            }
#endif

            // Reschedule inactivity timer.
            Ospfv2SetTimer(
                node,
                interfaceIndex,
                MSG_ROUTING_OspfInactivityTimer,
                nbrAddr,
                ospf->iface[interfaceIndex].routerDeadInterval,
                0,
                0,
                (Ospfv2PacketType) 0);
            break;
        }

        case E_Start:
        {
            // This event will be generated for NBMA networks only.
            // Avoid this case for now
            break;
        }

        case E_TwoWayReceived:
        {
            if (tempNeighborInfo->state != S_Init)
            {
                break;
            }
            if (tempNeighborInfo->state == S_Init)
            {
                // Determine whether adjacency is required
                if (Ospfv2AdjacencyRequire(node,
                                           interfaceIndex,
                                           tempNeighborInfo))
                {
                    if (Ospfv2DebugSync(node))
                    {
                        char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                        IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                        fprintf(stdout, "    NSM receive event "
                                        "E_TwoWayReceived.\n"
                                        "    neighbor state (%s) "
                                        "move up to S_ExStart\n",
                                         nbrAddrStr);
                    }

                    Ospfv2SetNeighborState(node,
                                           interfaceIndex,
                                           tempNeighborInfo,
                                           S_ExStart);
                }
                else
                {
                    if (Ospfv2DebugSync(node))
                    {
                        char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                        IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                        fprintf(stdout, "    NSM receive event "
                                        "E_TwoWayReceived\n"
                                        "    neighbor state (%s) "
                                        "move up to S_TwoWay\n",
                                         nbrAddrStr);
                    }

                    Ospfv2SetNeighborState(node,
                                           interfaceIndex,
                                           tempNeighborInfo,
                                           S_TwoWay);
                }
            }
            // No state change for neighbor state higher than S-Init
            break;
        }

        case E_NegotiationDone:
        {
            if (tempNeighborInfo->state != S_ExStart)
            {
                break;
            }

            Ospfv2SetNeighborState(node,
                                   interfaceIndex,
                                   tempNeighborInfo,
                                   S_Exchange);

            if (Ospfv2DebugSync(node))
            {
                char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                fprintf(stdout, "    NSM receive event E_NegotiationDone\n"
                                "    neighbor state (%s) move up to "
                                "S_Exchange\n",
                                nbrAddrStr);
            }

#ifdef ADDON_BOEINGFCS
            if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE &&
                RoutingCesRospfDatabaseDigestsEnabled(node, interfaceIndex))
            {
                if (! ROSPF_DEBUG_DBDG_VERIFICATION)
                {
                    delete tempNeighborInfo->dbdgNbr;
                }
                else if (tempNeighborInfo->dbdgNbr != NULL)
                {
                    tempNeighborInfo->dbdgNbr->cancelTimers();
                }
                tempNeighborInfo->dbdgNbr =
                    new RoutingCesRospfDbdgNeighbor(node,
                                                    interfaceIndex,
                                                    nbrAddr);
                tempNeighborInfo->dbdgNbr->negotiationDoneHook();
                break;
            }
#endif

            // List the contents of its entire area link state database
            // in the neighbor Database summary list.
            Ospfv2CreateDBSummaryList(node, interfaceIndex, tempNeighborInfo);
            break;
        }

        case E_ExchangeDone:
        {
            if (tempNeighborInfo->state != S_Exchange)
            {
                break;
            }

            if (Ospfv2DebugSync(node))
            {
                fprintf(stdout, "    NSM receive event S_ExchangeDone\n");
            }

#ifdef ADDON_BOEINGFCS
            if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE &&
                RoutingCesRospfDatabaseDigestsEnabled(node, interfaceIndex))
            {
                assert (tempNeighborInfo->dbdgNbr != NULL);
                tempNeighborInfo->dbdgNbr->exchangeDoneHook();
                if (Ospfv2DebugSync(node))
                {
                    fprintf(stdout, "    database digest exchange complete. "
                        "Neighbor state (0x%x) move up to S_Full\n",
                        nbrAddr);
                }
                Ospfv2SetNeighborState(node,
                                       interfaceIndex,
                                       tempNeighborInfo,
                                       S_Full);
                break;
            }
#endif

            if (Ospfv2ListIsEmpty(
                        tempNeighborInfo->linkStateRequestList))
            {
                if (Ospfv2DebugSync(node))
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    request list is empty. Neighbor"
                        " state (%s) move up to S_Full\n", nbrAddrStr);
                }

                Ospfv2SetNeighborState(node,
                                       interfaceIndex,
                                       tempNeighborInfo,
                                       S_Full);

                if (OSPFv2_DEBUG)
                {
                    NetworkPrintForwardingTable(node);
                }
            }
            else
            {
                if (Ospfv2DebugSync(node))
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    neighbor state (%s) move up "
                                    "to S_Loading\n",
                                     nbrAddrStr);
                }

                Ospfv2SetNeighborState(node,
                                       interfaceIndex,
                                       tempNeighborInfo,
                                       S_Loading);

                // Start (or continue) sending Link State Request packet
                Ospfv2SendLSRequestPacket(node,
                                          nbrAddr,
                                          interfaceIndex,
                                          FALSE);
            }
            break;
        }

        case E_LoadingDone:
        {
            if (tempNeighborInfo->state != S_Loading)
            {
                break;
            }

            if (Ospfv2DebugSync(node))
            {
                if (tempNeighborInfo->state != S_Full)
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    NSM receive event E_LoadingDone\n"
                                    "    neighbor state (%s) move up "
                                    "to S_Full\n",
                                    nbrAddrStr);
                }
            }

            Ospfv2SetNeighborState(node,
                                   interfaceIndex,
                                   tempNeighborInfo,
                                   S_Full);

            if (OSPFv2_DEBUG)
            {
                NetworkPrintForwardingTable(node);
            }

            break;
        }

        case E_AdjOk:
        {
            if ((Ospfv2AdjacencyRequire(node,
                                        interfaceIndex,
                                        tempNeighborInfo))
                && (tempNeighborInfo->state == S_TwoWay))
            {
                if (Ospfv2DebugSync(node))
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    NSM receive event E_AdjOk\n"
                                    "    neighbor state (%s) "
                                    "move up to S_ExStart\n",
                                     nbrAddrStr);
                }

                Ospfv2SetNeighborState(node,
                                       interfaceIndex,
                                       tempNeighborInfo,
                                       S_ExStart);
            }
            else if ((!Ospfv2AdjacencyRequire(node,
                                              interfaceIndex,
                                              tempNeighborInfo))
                    && (tempNeighborInfo->state > S_TwoWay))
            {
                if (Ospfv2DebugSync(node))
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    NSM receive event E_AdjOk\n"
                                    "    neighbor state (%s) "
                                    "move down to S_TwoWay\n",
                                     nbrAddrStr);
                }

                // Break possibly fromed adjacency
                Ospfv2DestroyAdjacency(node,
                                       tempNeighborInfo,
                                       interfaceIndex);
                Ospfv2SetNeighborState(node,
                                       interfaceIndex,
                                       tempNeighborInfo,
                                       S_TwoWay);
            }
            else
            {
                // No action required, no state change
                if (Ospfv2DebugSync(node))
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    NSM receive event E_AdjOk\n"
                                    "    No change in neighbor state (%s)\n",
                                     nbrAddrStr);
                }
            }

            break;
        }

        case E_BadLSReq:
        {
            if (tempNeighborInfo->state < S_Exchange)
            {
                break;
            }
            if (Ospfv2DebugSync(node))
            {
                char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                fprintf(stdout, "    NSM receive event E_BadLSReq\n"
                                "    neighbor state (%s) move down "
                                "to S_ExStart\n",
                                nbrAddrStr);
            }
        }
        case E_SeqNumberMismatch:
        {
            if (tempNeighborInfo->state < S_Exchange)
            {
                break;
            }
            if (Ospfv2DebugSync(node))
            {
                if (eventType == E_SeqNumberMismatch)
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    NSM receive event "
                                    "E_SeqNumberMismatch\n"
                                    "    neighbor state (%s) "
                                    "move down to S_ExStart\n",
                                     nbrAddrStr);
                }
            }

            // Torn down possibly formed adjacency
            Ospfv2DestroyAdjacency(node,
                                   tempNeighborInfo,
                                   interfaceIndex);

            Ospfv2SetNeighborState(node,
                                   interfaceIndex,
                                   tempNeighborInfo,
                                   S_ExStart
#ifdef ADDON_BOEINGFCS
                                   , TRUE
#endif
                                   );

            if (Ospfv2DebugSync(node))
            {
                fprintf(stdout, "Attempt to reestablished adjacency\n");
            }

            break;
        }

        case E_OneWay:
        {
            if (tempNeighborInfo->state >= S_TwoWay)
            {
                if (Ospfv2DebugSync(node))
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                    fprintf(stdout, "    NSM receive event E_OneWay\n"
                                    "    neighbor state (%s) move down "
                                    "to S_Init\n",
                                    nbrAddrStr);
                }

                Ospfv2DestroyAdjacency(node,
                                       tempNeighborInfo,
                                       interfaceIndex);

                Ospfv2SetNeighborState(node,
                                       interfaceIndex,
                                       tempNeighborInfo,
                                       S_Init);
            }
            break;
        }

        case E_InactivityTimer:
        //RFC:1793::SECTION:3.2.2. NEIGHBOR STATE MACHINE MODIFICATIONS
        //The InactivityTimer event has no effect on demand circuit neighbors
        //in state "Loading" or "Full".
        {
            //If the interface is a demand circuit and hello suppression is
            //already done and the state of the neighbor is Full or Loading
            //then there is no need of inactivity timer so not removing the
            //neighbor because expiry of this timer is just because of
            //suppression of hello
            //messages over demand circuits
            if ((ospf->supportDC == TRUE)
                &&
                (ospf->iface[interfaceIndex].ospfIfDemand == TRUE)
                &&
                (ospf->iface[interfaceIndex].type ==
                OSPFv2_POINT_TO_POINT_INTERFACE))
            {
                if (ospf->iface[interfaceIndex].helloSuppressionSuccess)
                {
                    if ((tempNeighborInfo->state == S_Loading)
                        ||
                        (tempNeighborInfo->state == S_Full))
                    {
                        //Do Nothing

                        if (OSPFv2_DEBUG_DEMANDCIRCUIT)
                        {
                            printf("\nNode %d\n", node->nodeId);
                            printf("\nInactivityTimer: As the neighbor"
                                   "is in state"
                                   "S_Loading/S_Full so not killing the"
                                   "neighbor\n");
                        }
                        break;
                    }
                 }
            }
        }
        case E_KillNbr:
        case E_LLDown:
        {
            if (Ospfv2DebugSync(node))
            {
                char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
                printf("    Node %d\n", node->nodeId);
                printf("    NSM receive event %d\n"
                    "    neighbor state (%s) move down to "
                    "S_NeighborDown\n",
                            eventType, nbrAddrStr);
            }

            Ospfv2SetNeighborState(node,
                                   interfaceIndex,
                                   tempNeighborInfo,
                                   S_NeighborDown);

            Ospfv2RemoveNeighborFromNeighborList(node,
                                                 interfaceIndex, nbrAddr);


#ifdef ADDON_BOEINGFCS
            RoutingCesRospfHandleNeighborDown(node, interfaceIndex, nbrAddr);
#endif

            break;
        }

        default:
        {
            char errorStr[MAX_STRING_LENGTH];
            char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];

            IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
            sprintf(errorStr, "Node %u : Unknown neighbor event %d for "
                              "neighbor (%s).\n",
                               node->nodeId, eventType, nbrAddrStr);

#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              errorStr,
                              JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, errorStr);
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2GetTopLSAFromList()
// PURPOSE      :Get top LSA Header from DB Summary list
// ASSUMPTION   :None
// RETURN VALUE :BOOL
//-------------------------------------------------------------------------//

static
BOOL Ospfv2GetTopLSAFromList(
    Node* node,
    Ospfv2List* list,
    Ospfv2LinkStateHeader* LSHeader)
{
    Ospfv2ListItem* listItem = NULL;

    if (list->first == NULL)
    {
        return FALSE;
    }

    listItem = list->first;
    memcpy(LSHeader, listItem->data, sizeof(Ospfv2LinkStateHeader));

    // Remove item from list
    Ospfv2RemoveFromList(node, list, listItem, FALSE);

    return TRUE;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2CreateLSReqObject()
// PURPOSE      :Create a LS Request object from link state request list
// ASSUMPTION   :None
// RETURN VALUE :BOOL
//-------------------------------------------------------------------------//

static
BOOL Ospfv2CreateLSReqObject(
    Node* node,
    Ospfv2Neighbor* nbrInfo,
    Ospfv2LSRequestInfo* LSReqObject,
    BOOL retx)
{
    Ospfv2ListItem* listItem = nbrInfo->linkStateRequestList->first;
    Ospfv2LSReqItem* itemInfo = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;

    if (retx)
    {
        while (listItem)
        {
            itemInfo = (Ospfv2LSReqItem*) listItem->data;

            if (itemInfo->seqNumber == nbrInfo->LSReqTimerSequenceNumber)
            {
                LSHeader = itemInfo->LSHeader;
                itemInfo->seqNumber = nbrInfo->LSReqTimerSequenceNumber + 1;
                break;
            }
            listItem = listItem->next;
        }
    }
    else
    {
        while (listItem)
        {
            itemInfo = (Ospfv2LSReqItem*) listItem->data;

            // service older requests before new ones... otherwise there may
            // not be any new ones, and we will send out an empty packet,
            // throwing off the LSReqTimerSequenceNumber for all of the old
            // requests
            if (itemInfo->seqNumber == nbrInfo->LSReqTimerSequenceNumber)
            {
                LSHeader = itemInfo->LSHeader;
                itemInfo->seqNumber = nbrInfo->LSReqTimerSequenceNumber + 1;
                break;
            }
            else if (itemInfo->seqNumber == 0)
            {
                LSHeader = itemInfo->LSHeader;
                itemInfo->seqNumber = nbrInfo->LSReqTimerSequenceNumber + 1;
                break;
            }
            listItem = listItem->next;
        }
    }

    if (LSHeader == NULL)
    {
        return FALSE;
    }

    if (OSPFv2_DEBUG)
    {
        char clkStr[100];
        char linkIDStr[20];
        char advRt[20];

        TIME_PrintClockInSecond(node->getNodeTime()+getSimStartTime(node),
                                clkStr);
        IO_ConvertIpAddressToString(LSHeader->linkStateID, linkIDStr);
        IO_ConvertIpAddressToString(LSHeader->advertisingRouter, advRt);
        printf("Node %u creating ReqObj at %s\n",
            node->nodeId, clkStr);
        printf("    requesting:\n");
        printf("        linkStateID = %s\n", linkIDStr);
        printf("        advertisingRouter = %s\n", advRt);
        printf("        linkStateSequenceNumber = 0x%x\n",
               LSHeader->linkStateSequenceNumber);
    }

    LSReqObject->linkStateType
        = (Ospfv2LinkStateType) LSHeader->linkStateType;
    LSReqObject->linkStateID = LSHeader->linkStateID;
    LSReqObject->advertisingRouter = LSHeader->advertisingRouter;
#ifdef ADDON_MA
    LSReqObject->mask = LSHeader->mask;
#endif

    return TRUE;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2LookupLSDB()
// PURPOSE      :Search for the LSA in LSDB
// ASSUMPTION   :None
// RETURN VALUE :Ospfv2LinkStateHeader*
//-------------------------------------------------------------------------//

Ospfv2LinkStateHeader* Ospfv2LookupLSDB(
    Node* node,
    char linkStateType,
    NodeAddress advertisingRouter,
    NodeAddress linkStateID,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2LinkStateHeader* LSHeader = NULL;

    Ospfv2Area* thisArea = NULL;

    // BGP-OSPF Patch Start
    if (linkStateType != OSPFv2_ROUTER_AS_EXTERNAL &&
        linkStateType != OSPFv2_ROUTER_NSSA_EXTERNAL &&
        /***** Start: OPAQUE-LSA *****/
        linkStateType != OSPFv2_AS_SCOPE_OPAQUE)
        /***** End: OPAQUE-LSA *****/
    {
        thisArea = Ospfv2GetArea(node,
                                 ospf->iface[interfaceIndex].areaId);
#ifdef JNE_LIB
        if (!thisArea)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Area doesn't exist",
                              JNE::CRITICAL);
        }
#endif
        ERROR_Assert(thisArea, "Area doesn't exist\n");
    }
    // BGP-OSPF Patch End

    if (linkStateType == OSPFv2_ROUTER)
    {
        LSHeader =  Ospfv2LookupLSAList(thisArea->routerLSAList,
                                        advertisingRouter,
                                        linkStateID);
    }
    else if (linkStateType == OSPFv2_NETWORK)
    {
        LSHeader = Ospfv2LookupLSAList(thisArea->networkLSAList,
                                       advertisingRouter,
                                       linkStateID);
    }
    else if (linkStateType == OSPFv2_NETWORK_SUMMARY)
    {
        LSHeader = Ospfv2LookupLSAList(thisArea->networkSummaryLSAList,
                                       advertisingRouter,
                                       linkStateID);
    }
    else if (linkStateType == OSPFv2_ROUTER_SUMMARY)
    {
        LSHeader = Ospfv2LookupLSAList(thisArea->routerSummaryLSAList,
                                       advertisingRouter,
                                       linkStateID);
    }
    // BGP-OSPF Patch Start
    else if (linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
    {
        LSHeader = Ospfv2LookupLSAList(ospf->asExternalLSAList,
                                       advertisingRouter,
                                       linkStateID);
    }
    // BGP-OSPF Patch End
    else if (linkStateType == OSPFv2_GROUP_MEMBERSHIP)
    {
        LSHeader = Ospfv2LookupLSAList(thisArea->groupMembershipLSAList,
                                       advertisingRouter,
                                       linkStateID);
    }
    else if (linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
    {
        LSHeader = Ospfv2LookupLSAList(ospf->nssaExternalLSAList,
                                       advertisingRouter,
                                       linkStateID);
    }
    /***** Start: OPAQUE-LSA *****/
    else if (linkStateType == OSPFv2_AS_SCOPE_OPAQUE)
    {
        LSHeader = Ospfv2LookupLSAList(ospf->ASOpaqueLSAList,
                                       advertisingRouter,
                                       linkStateID);
    }
    /***** End: OPAQUE-LSA *****/
    else
    {
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Unknown LS Type",
                              JNE::CRITICAL);
#endif
        ERROR_Assert(FALSE, "Unknown LS Type\n");
    }

    return LSHeader;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2LookupLSAList()
// PURPOSE      :Search for the LSA in list
// ASSUMPTION   :None
// RETURN VALUE :Ospfv2LinkStateHeader*
//-------------------------------------------------------------------------//

Ospfv2LinkStateHeader* Ospfv2LookupLSAList(
    Ospfv2List* list,
    NodeAddress advertisingRouter,
    NodeAddress linkStateID)
{
    Ospfv2LinkStateHeader* listLSHeader = NULL;

    Ospfv2ListItem* item = list->first;

    while (item)
    {
        // Get LS Header
        listLSHeader = (Ospfv2LinkStateHeader*) item->data;

        if (listLSHeader->advertisingRouter == advertisingRouter
            && listLSHeader->linkStateID == linkStateID)
        {
            return listLSHeader;
        }

        item = item->next;
    }
    return NULL;
}
#ifdef ADDON_MA
//-------------------------------------------------------------------------//
// NAME         :Ospfv2LookupLSAList_Extended()
// PURPOSE      :Search for the LSA in list
// ASSUMPTION   :None
// RETURN VALUE :Ospfv2LinkStateHeader*
//-------------------------------------------------------------------------//

Ospfv2LinkStateHeader* Ospfv2LookupLSAList_Extended(
    Node* node,
    Ospfv2List* list,
    NodeAddress advertisingRouter,
    NodeAddress linkStateID,
    NodeAddress mask)
{
    Ospfv2LinkStateHeader* listLSHeader = NULL;
    Ospfv2ExternalLinkInfo* linkInfo;
    Ospfv2ListItem* item = list->first;

    while (item)
    {
        // Get LS Header
        listLSHeader = (Ospfv2LinkStateHeader*) item->data;
        linkInfo = (Ospfv2ExternalLinkInfo*) (listLSHeader + 1);
        if (listLSHeader->advertisingRouter == advertisingRouter
            && listLSHeader->linkStateID == linkStateID
            && mask == linkInfo->networkMask)
        {
            return listLSHeader;
        }

        item = item->next;
    }
    return NULL;
}
#endif
//-------------------------------------------------------------------------//
// NAME         :Ospfv2GetLSAListItem()
// PURPOSE      :Search for the LSA in list and get the item.
// ASSUMPTION   :None
// RETURN VALUE :Ospfv2ListItem*
//-------------------------------------------------------------------------//

Ospfv2ListItem* Ospfv2GetLSAListItem(
    Ospfv2List* list,
    NodeAddress advertisingRouter,
    NodeAddress linkStateID)
{
    Ospfv2LinkStateHeader* listLSHeader = NULL;

    Ospfv2ListItem* item = list->first;

    while (item)
    {
        // Get LS Header
        listLSHeader = (Ospfv2LinkStateHeader*) item->data;

        if (listLSHeader->advertisingRouter == advertisingRouter
            && listLSHeader->linkStateID == linkStateID)
        {
            return item;
        }

        item = item->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2LookupLSAListByID()
// PURPOSE      :Search for the LSA in list by Link State ID
// ASSUMPTION   :None
// RETURN VALUE :Ospfv2LinkStateHeader*
//-------------------------------------------------------------------------//

Ospfv2LinkStateHeader* Ospfv2LookupLSAListByID(
    Ospfv2List* list,
    NodeAddress linkStateID)
{

#ifdef nADDON_BOEINGFCS
    if (RoutingCesRospfActiveOnAnyInterface(node))
    {
        return RoutingCesRospfLookupLSAListByID(node, list, linkStateID);
    }
#endif

    Ospfv2LinkStateHeader* listLSHeader = NULL;

    Ospfv2ListItem* item = list->first;

    while (item)
    {
        // Get LS Header
        listLSHeader = (Ospfv2LinkStateHeader*) item->data;

        if (listLSHeader->linkStateID == linkStateID)
        {
            return listLSHeader;
        }

        item = item->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2CompareLSA()
// PURPOSE      :Check whether the received LSA is more recent
// ASSUMPTION   :None
// RETURN VALUE : 1 if the first LSA is more recent,
//              :-1 if the second LSA is more recent,
//              : 0 if the two LSAs are same,
//-------------------------------------------------------------------------//

int Ospfv2CompareLSA(
    Node* node,
    Ospfv2LinkStateHeader* LSHeader,
    Ospfv2LinkStateHeader* oldLSHeader)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    // If LSA have newer sequence number
    if (LSHeader->linkStateSequenceNumber >
         oldLSHeader->linkStateSequenceNumber)
    {
        if (Ospfv2DebugSync(node))
        {
            fprintf(stdout, "Sequence number is newer\n");
        }

        return 1;
    }
    else if (oldLSHeader->linkStateSequenceNumber >
             LSHeader->linkStateSequenceNumber)
    {
        if (Ospfv2DebugSync(node))
        {
            fprintf(stdout, "Sequence number is older\n");
        }

        return -1;
    }
    else if (LSHeader->linkStateCheckSum > oldLSHeader->linkStateCheckSum)
    {
        return 1;
    }
    else if (LSHeader->linkStateCheckSum < oldLSHeader->linkStateCheckSum)
    {
        return -1;
    }
    // If only the new LSA have age equal to LSMaxAge
    else if ((Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) >=
             (OSPFv2_LSA_MAX_AGE / SECOND))
             && (Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge) <
                (OSPFv2_LSA_MAX_AGE / SECOND)))
    {
        if (Ospfv2DebugSync(node))
        {
            fprintf(stdout, "Sequence number is the same and new LSA's"
                " age is equal to LSAMaxAge\n");
        }

        return 1;
    }
    else if ((Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge) >=
               (OSPFv2_LSA_MAX_AGE / SECOND))
        && (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) <
               (OSPFv2_LSA_MAX_AGE / SECOND)))
    {
        if (Ospfv2DebugSync(node))
        {
            fprintf(stdout, "Sequence number is the same and old LSA's"
                " age is equal to LSAMaxAge\n");
        }

        return -1;
    }
    // If the LS age fields of two instances differ by more than MaxAgeDiff,
    // the instance having the smaller LS age is considered to be more
    // recent.
    else if ((abs(Ospfv2DifferenceOfTwoLSAAge(ospf,
                                             LSHeader->linkStateAge,
                                             oldLSHeader->linkStateAge))
                                         > (OSPFv2_MAX_AGE_DIFF / SECOND))
              &&
              (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge)
                < Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge)))
    {
        if (Ospfv2DebugSync(node))
        {
            fprintf(stdout, "Sequence number is equal, but new LSA has"
                " smaller LS age\n");
        }

        return 1;
    }
    else if (((abs(Ospfv2DifferenceOfTwoLSAAge(ospf,
                                             LSHeader->linkStateAge,
                                             oldLSHeader->linkStateAge)))
                                          > (OSPFv2_MAX_AGE_DIFF / SECOND))
             &&
             (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge)
               > Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge)))
    {
        if (Ospfv2DebugSync(node))
        {
            fprintf(stdout, "Sequence number is equal, but old LSA"
                " has smaller LS age\n");
        }

        return -1;
    }

    return 0;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2AddToRequestList()
// PURPOSE      :Add this LSA in Link State Request list
// ASSUMPTION   :None
// RETURN VALUE :NULL
//-------------------------------------------------------------------------//

static
void Ospfv2AddToRequestList(
    Node* node,
    Ospfv2List* linkStateRequestList,
    Ospfv2LinkStateHeader* LSHeader)
{

    Ospfv2LSReqItem* newItem = NULL;

    if (Ospfv2DebugSync(node))
    {
        char lsIDStr[MAX_ADDRESS_STRING_LENGTH];
        char advertRtrStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(LSHeader->linkStateID, lsIDStr);
        IO_ConvertIpAddressToString(LSHeader->advertisingRouter,
                                    advertRtrStr);
        fprintf(stdout, "    Adding new LSA to request list\n"
                        "        LS Type = %d, "
                        "Link State ID = %s, Advertising Router = %s\n",
                        LSHeader->linkStateType, lsIDStr, advertRtrStr);
    }

    // Search before adding LSHeader in request list,
    // whether it exists in list already or not.
    if (Ospfv2SearchRequestList(linkStateRequestList, LSHeader))
    {
        return;
    }

    // Create new item to insert to list
    newItem = (Ospfv2LSReqItem *) MEM_malloc(sizeof(Ospfv2LSReqItem));

    newItem->LSHeader = (Ospfv2LinkStateHeader*)
            MEM_malloc(sizeof(Ospfv2LinkStateHeader));
    memcpy(newItem->LSHeader, LSHeader, sizeof(Ospfv2LinkStateHeader));

    newItem->seqNumber = 0;
#ifdef ADDON_MA
    if (LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL ||
        LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
    {

        newItem->mask = LSHeader->mask;
    }
#endif

    Ospfv2InsertToList(linkStateRequestList,0, (void*) newItem);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2SearchRequestList()
// PURPOSE      :Search for a LSA in Link State Request list
// ASSUMPTION   :None
// RETURN VALUE :Return LSA if found, otherwise return NULL
//-------------------------------------------------------------------------//

Ospfv2LinkStateHeader* Ospfv2SearchRequestList(
    Ospfv2List* list,
    Ospfv2LinkStateHeader* LSHeader)
{
    Ospfv2ListItem* listItem = list->first;

    while (listItem)
    {
        Ospfv2LSReqItem* itemInfo = (Ospfv2LSReqItem*) listItem->data;
        Ospfv2LinkStateHeader* listLSHeader = itemInfo->LSHeader;
#ifdef ADDON_MA
        BOOL  isMatched;
        if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL ||
            LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)&&
            (listLSHeader->linkStateType == LSHeader->linkStateType))
        {
            /*
            Ospfv2ExternalLinkInfo* linkInfo;
            //Ospfv2ExternalLinkInfo* listInfo;
            linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
            //listInfo = (Ospfv2ExternalLinkInfo*) (listLSHeader + 1);

            if (linkInfo->networkMask == itemInfo->mask)
            */
            if (LSHeader->mask == listLSHeader->mask)
            {
                isMatched = TRUE;
            }else
            {
                isMatched = FALSE;
            }
        }else
        {
           isMatched = TRUE;
        }

        if (LSHeader->linkStateType == listLSHeader->linkStateType &&
            LSHeader->linkStateID == listLSHeader->linkStateID &&
            LSHeader->advertisingRouter == listLSHeader->advertisingRouter &&
            isMatched)
#else
        if ((LSHeader->linkStateType == listLSHeader->linkStateType)  &&
            (LSHeader->linkStateID == listLSHeader->linkStateID) &&
            (LSHeader->advertisingRouter == listLSHeader->advertisingRouter))
#endif
        {
            return listLSHeader;
        }
        listItem = listItem->next;
    }

    return NULL;
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2SearchSendList
// PURPOSE      :Search for a LSA in Link State Send list
// ASSUMPTION   :None
// RETURN VALUE :Return LSA if found, otherwise return NULL
//-------------------------------------------------------------------------//

Ospfv2ListItem* Ospfv2SearchSendList(
    Ospfv2List* list,
    Ospfv2LinkStateHeader* LSHeader)
{
    Ospfv2ListItem* listItem = list->first;

    while (listItem)
    {
        Ospfv2SendLSAInfo* sendInfo = (Ospfv2SendLSAInfo*) listItem->data;

        if ((sendInfo->linkStateType == LSHeader->linkStateType) &&
            (sendInfo->linkStateID == LSHeader->linkStateID) &&
            (sendInfo->advertisingRouter == LSHeader->advertisingRouter))
        {
            return listItem;
        }
        listItem = listItem->next;
    }

    return NULL;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2RemoveFromRequestList()
// PURPOSE      :Remove this LSA from Link State Request list
// ASSUMPTION   :None
// RETURN VALUE :NULL
//-------------------------------------------------------------------------//

void Ospfv2RemoveFromRequestList(
    Node* node,
    Ospfv2List* list,
    Ospfv2LinkStateHeader* LSHeader)
{
    Ospfv2ListItem* listItem = list->first;

#ifdef JNE_LIB
    if (list->size == 0)
    {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Request list is empty",
                              JNE::CRITICAL);
    }
#endif
    ERROR_Assert(list->size != 0, "Request list is empty\n");

    while (listItem)
    {
        Ospfv2LSReqItem* itemInfo = (Ospfv2LSReqItem*) listItem->data;
        Ospfv2LinkStateHeader* listLSHeader = itemInfo->LSHeader;
#ifdef ADDON_MA
        BOOL  isMatched;
         if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL ||
            LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)&&
            (listLSHeader->linkStateType == LSHeader->linkStateType))
        {
            /*
            Ospfv2ExternalLinkInfo* linkInfo;
            //Ospfv2ExternalLinkInfo* listInfo;
            linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
            //listInfo = (Ospfv2ExternalLinkInfo*) (listLSHeader + 1);

            if (linkInfo->networkMask == itemInfo->mask)
                */
            if (LSHeader->mask == listLSHeader->mask)
            {
                isMatched = TRUE;
            }else
            {
                isMatched = FALSE;
            }
        }else
        {
           isMatched = TRUE;
        }
        if (LSHeader->linkStateType == listLSHeader->linkStateType &&
            LSHeader->linkStateID == listLSHeader->linkStateID &&
            LSHeader->advertisingRouter == listLSHeader->advertisingRouter &&
            isMatched)
#else
        if ((LSHeader->linkStateType == listLSHeader->linkStateType) &&
            (LSHeader->linkStateID == listLSHeader->linkStateID) &&
            (LSHeader->advertisingRouter == listLSHeader->advertisingRouter))
#endif
        {
            // Remove item from list
            MEM_free(itemInfo->LSHeader);
            Ospfv2RemoveFromList(node, list, listItem, FALSE);

            break;
        }

        listItem = listItem->next;
    }
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2RemoveFromSendList()
// PURPOSE      :Remove this LSA from Link State Send list
// ASSUMPTION   :None
// RETURN VALUE :NULL
//-------------------------------------------------------------------------//

static
void Ospfv2RemoveFromSendList(
    Node* node,
    char linkStateType,
    NodeAddress linkStateID,
    NodeAddress advertisingRouter,
    unsigned int areaId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2ListItem* tempListItem = NULL;
    Ospfv2ListItem* sendItem = NULL;
    Ospfv2Neighbor* nbrInfo = NULL;

    Ospfv2SendLSAInfo* sendLSAInfo = NULL;

    // Look at each of my attached interface.
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            continue;
        }

        // Skip the interface if it doesn't belong to specified area
        if ((ospf->iface[i].areaId != areaId) &&
            (areaId != OSPFv2_INVALID_AREA_ID))
        {
            continue;
        }

        tempListItem = ospf->iface[i].neighborList->first;

        // Get the neighbors for each interface.
        while (tempListItem)
        {
            nbrInfo = (Ospfv2Neighbor*) tempListItem->data;
#ifdef JNE_LIB
            if (!nbrInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Neighbor not found in the Neighbor list",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(nbrInfo,
                "Neighbor not found in the Neighbor list\n");

            sendItem = nbrInfo->linkStateSendList->first;

            // Look for LSA in retransmission list of each neighbor.
            while (sendItem)
            {
                sendLSAInfo = (Ospfv2SendLSAInfo *) sendItem->data;

                // If LSA exists in retransmission list, remove it.

                if ((sendLSAInfo->linkStateType == linkStateType)
                    && (sendLSAInfo->linkStateID == linkStateID)
                    && (sendLSAInfo->advertisingRouter == advertisingRouter))
                {
                    Ospfv2ListItem* deleteItem = sendItem;
                    sendItem = sendItem->next;

                    Ospfv2RemoveFromList(node,
                                         nbrInfo->linkStateSendList,
                                         deleteItem,
                                         FALSE);
                }
                else
                {
                    sendItem = sendItem->next;
                }
            }

            tempListItem = tempListItem->next;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2RequestedLSAReceived()
// PURPOSE      :Check whether requested LSA(s) has been received
// ASSUMPTION   :None
// RETURN VALUE :TRUE if received, FALSE otherwise
//-------------------------------------------------------------------------//

BOOL Ospfv2RequestedLSAReceived(Ospfv2Neighbor* nbrInfo)
{
    // Requests are sent from top of the list. So it's sufficient
    // to check only first element of the list
    Ospfv2LSReqItem* itemInfo = (Ospfv2LSReqItem*)
                                nbrInfo->linkStateRequestList->first->data;

//#ifdef JNE_LIB
//    if (itemInfo->seqNumber > nbrInfo->LSReqTimerSequenceNumber)
//    {
//            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
//                              "LS Request not handled properly",
//                             JNE::CRITICAL);
//    }
//#endif
    ERROR_Assert(itemInfo->seqNumber <= nbrInfo->LSReqTimerSequenceNumber,
                    "LS Request not handled properly\n");

    if (itemInfo->seqNumber == nbrInfo->LSReqTimerSequenceNumber)
    {
        return FALSE;
    }

    return TRUE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2GetNbrInfoOnInterface
// PURPOSE:  Get the neighbor structure from the neighbor list of specific
//           interface using the neighbor's router ID.
// RETURN: The neighbor structure we are looking for, or NULL if not found.
//-------------------------------------------------------------------------//

static
Ospfv2Neighbor* Ospfv2GetNbrInfoOnInterface(
    Node* node,
    NodeAddress nbrId,
    int interfaceId)
{
    Ospfv2ListItem* listItem = NULL;
    Ospfv2Neighbor* nbrInfo = NULL;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    listItem = ospf->iface[interfaceId].neighborList->first;

    // Search for the neighbor in my neighbor list.
    while (listItem)
    {
        nbrInfo = (Ospfv2Neighbor*) listItem->data;

#ifdef JNE_LIB
        if (!nbrInfo)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Neighbor not found in the Neighbor list",
                              JNE::CRITICAL);
        }
#endif
        ERROR_Assert(nbrInfo, "Neighbor not found in the Neighbor list");

        if (nbrInfo->neighborID == nbrId)
        {
            // Found the neighbor we are looking for.
            return nbrInfo;
        }

        listItem = listItem->next;
    }

    return NULL;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2RouterFullyAdjacentWithDR()
// PURPOSE      :Verify if the router is fully adjacent with DR.
// ASSUMPTION   :None.
// RETURN VALUE :BOOL
//-------------------------------------------------------------------------//

static
BOOL Ospfv2RouterFullyAdjacentWithDR(Node* node, int interfaceId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Neighbor* neighborInfo = NULL;
    Ospfv2ListItem* listItem =
        ospf->iface[interfaceId].neighborList->first;

    while (listItem)
    {
        neighborInfo = (Ospfv2Neighbor*) listItem->data;

        if (neighborInfo->neighborID ==
                ospf->iface[interfaceId].designatedRouter
            && neighborInfo->state == S_Full)
        {
            return TRUE;
        }
        listItem = listItem->next;
    }

    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2DRFullyAdjacentWithAnyRouter()
// PURPOSE      :Verify if router is fully adjacent with any of it's
//               neighbor.
// ASSUMPTION   :None.
// RETURN VALUE :BOOL
//-------------------------------------------------------------------------//

BOOL Ospfv2DRFullyAdjacentWithAnyRouter(Node* node, int interfaceId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Neighbor* neighborInfo = NULL;
    Ospfv2ListItem* listItem =
        ospf->iface[interfaceId].neighborList->first;

    while (listItem)
    {
        neighborInfo = (Ospfv2Neighbor*) listItem->data;

        if (neighborInfo->state == S_Full)
        {
            return TRUE;
        }
        listItem = listItem->next;
    }

    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2NoNbrInStateExhangeOrLoading()
// PURPOSE      :Verify if any neighbor is in state Exchange or Loading.
// ASSUMPTION   :None.
// RETURN VALUE :BOOL
//-------------------------------------------------------------------------//

BOOL Ospfv2NoNbrInStateExhangeOrLoading(
    Node* node,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Neighbor* neighborInfo = NULL;
    Ospfv2ListItem* listItem =
        ospf->iface[interfaceIndex].neighborList->first;

    while (listItem)
    {
        neighborInfo = (Ospfv2Neighbor*) listItem->data;

        if (neighborInfo->state == S_Exchange
            || neighborInfo->state == S_Loading)
        {
            return FALSE;
        }
        listItem = listItem->next;
    }

    return TRUE;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2CopyLSA()
// PURPOSE      :Copy LSA.
// ASSUMPTION   :None.
// RETURN VALUE :char*
//-------------------------------------------------------------------------//

char* Ospfv2CopyLSA(char* LSA)
{
    Ospfv2LinkStateHeader* LSHeader =
            (Ospfv2LinkStateHeader*) LSA;
    char* newLSA = (char *) MEM_malloc(LSHeader->length);

    memcpy(newLSA, LSA, LSHeader->length);
    return newLSA;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2NextHopBelongsToThisArea()
// PURPOSE      :Check whether the next hop belongs to this area
// ASSUMPTION   :Consider only one next hop now
// RETURN VALUE :TRUE if next hop belongs to this area, FALSE otherwise
//-------------------------------------------------------------------------//

static
BOOL Ospfv2NextHopBelongsToThisArea(
    Node* node,
    Ospfv2RoutingTableRow* thisRoute,
    Ospfv2Area* area)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int intfIndex = thisRoute->outIntf;
#ifdef nADDON_BOEINGFCS
    if (intfIndex == -1)
    {
        return FALSE;
    }
#endif
#ifdef JNE_LIB
    if (intfIndex == -1)
    {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Neighbor on unknown interface",
                              JNE::CRITICAL);
    }
#endif

    ERROR_Assert(intfIndex != -1, "Neighbor on unknown interface\n");

    if (ospf->iface[intfIndex].type == OSPFv2_NON_OSPF_INTERFACE)
    {
        if (thisRoute->areaId == area->areaID)
            return TRUE;
    }
    else if (ospf->iface[intfIndex].areaId == area->areaID)
    {
        return TRUE;
    }

    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2ScheduleSPFCalculation()
// PURPOSE      :Schedule SPF calculation.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2ScheduleSPFCalculation(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Message* newMsg = NULL;
    clocktype delay;

    if (ospf->SPFTimer > node->getNodeTime())
    {
        return;
    }

    delay = (clocktype) (ospf->spfCalcDelay * RANDOM_erand(ospf->seed));

    ospf->SPFTimer = node->getNodeTime() + delay;

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfScheduleSPF);

#ifdef ADDON_NGCNMS
    MESSAGE_SetInstanceId(newMsg, -1);
#endif

    MESSAGE_Send(node, newMsg, (clocktype) delay);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2ScheduleRouterLSA()
// PURPOSE      :Schedule router LSA origination
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2ScheduleRouterLSA(
    Node* node,
    unsigned int areaId,
    BOOL flush)
{
    Message* newMsg = NULL;
    clocktype delay;
    char* msgInfo = NULL;
    Ospfv2LinkStateType lsType;

    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);
#ifdef JNE_LIB
    if (!thisArea)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Area doesn't exist",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(thisArea, "Area doesn't exist\n");

    if (thisArea->routerLSTimer == TRUE)
    {
        return;
    }
    else if (thisArea->routerLSAOriginateTime == 0)
    {
        delay = 1;

        delay += ospf->staggerStartTime;
    }
    else if ((node->getNodeTime() - thisArea->routerLSAOriginateTime) >=
                     OSPFv2_MIN_LS_INTERVAL)
    {
        delay = (clocktype)
                (RANDOM_erand(ospf->seed) * OSPFv2_BROADCAST_JITTER);
    }
    else
    {
        delay = (clocktype) (OSPFv2_MIN_LS_INTERVAL
                - (node->getNodeTime() - thisArea->routerLSAOriginateTime));
    }

    thisArea->routerLSTimer = TRUE;

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfScheduleLSDB);

#ifdef ADDON_NGCNMS
    // newly added for interface reset functionlaity.

    int i;
    int interfaceIndex = -1;
    for (i=0; i<node->numberInterfaces; i++)
    {
        if (ospf->iface[i].areaId == areaId)
        {
            interfaceIndex = i;
            break;
        }
    }
#ifdef JNE_LIB
    if (interfaceIndex == -1)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Problem determining interfaceIndex",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(interfaceIndex != -1,
        "Problem determining interfaceIndex\n");

    MESSAGE_SetInstanceId(newMsg, interfaceIndex);

    // end newly added
#endif

    MESSAGE_InfoAlloc(node,
                      newMsg,
                      sizeof(Ospfv2LinkStateType)
                      + sizeof(unsigned int)
                      + sizeof(BOOL));

    lsType = OSPFv2_ROUTER;
    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv2LinkStateType));
    msgInfo += sizeof(Ospfv2LinkStateType);
    memcpy(msgInfo, &areaId, sizeof(unsigned int));
    msgInfo += sizeof(unsigned int);
    memcpy(msgInfo, &flush, sizeof(BOOL));

    MESSAGE_Send(node, newMsg, (clocktype) delay);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2ScheduleNetworkLSA()
// PURPOSE      :Schedule network LSA origination
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2ScheduleNetworkLSA(Node* node, int interfaceId, BOOL flush)
{
    Message* newMsg = NULL;
    clocktype delay;
    char* msgInfo = NULL;
    Ospfv2LinkStateType lsType;
    Ospfv2LinkStateHeader* oldLSHeader = NULL;
    BOOL haveAdjNbr = FALSE;

    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Interface* thisInterface = &ospf->iface[interfaceId];
    Ospfv2Area* thisArea =
            Ospfv2GetArea(node, thisInterface->areaId);
#ifdef JNE_LIB
    if (!thisArea)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Area not found",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(thisArea, "Area not fount\n");

    // Get old instance, if any..
    oldLSHeader = Ospfv2LookupLSAList(
                        thisArea->networkLSAList,
                        ospf->routerID,
                        ospf->iface[interfaceId].address);

    haveAdjNbr = Ospfv2DRFullyAdjacentWithAnyRouter(node, interfaceId);

    if (haveAdjNbr && !flush)
    {
        // Originate LSA
    }
    else if (oldLSHeader && (!haveAdjNbr || flush))
    {
        // Flush LSA
        flush = TRUE;
    }
    else
    {
        // Do nothing
        return;
    }

#ifndef ADDON_BOEINGFCS
    if (thisInterface->networkLSTimer
        && !flush)
    {
        return;
    }
#else
    if (thisInterface->type == OSPFv2_ROSPF_INTERFACE)
    {
        if (thisInterface->networkLSTimer)
        {
            if (thisInterface->networkLSFlushTimer &&
               !flush)
            {
                // dont return.
            }
            else if (!thisInterface->networkLSFlushTimer &&
                     flush)
            {
                // dont return.
            }
            else
            {
                return;
            }
        }
    }
    else
    {
        if (thisInterface->networkLSTimer
        && !flush)
        {
            return;
        }
    }
#endif

    // We need to cancel previous timer if any
    thisInterface->networkLSTimerSeqNumber++;

    if ((thisInterface->networkLSAOriginateTime == 0)
        || ((node->getNodeTime() - thisInterface->networkLSAOriginateTime) >=
                     OSPFv2_MIN_LS_INTERVAL))
    {
        delay = (clocktype)
                (RANDOM_erand(ospf->seed) * OSPFv2_BROADCAST_JITTER);
    }
    else
    {
        delay = (clocktype) (OSPFv2_MIN_LS_INTERVAL
                - (node->getNodeTime()
                - thisInterface->networkLSAOriginateTime));
    }

    thisInterface->networkLSTimer = TRUE;
#ifdef ADDON_BOEINGFCS
    thisInterface->networkLSFlushTimer = flush;
#endif

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfScheduleLSDB);

#ifdef ADDON_NGCNMS
    MESSAGE_SetInstanceId(newMsg, interfaceId);
#endif

    MESSAGE_InfoAlloc(node,
                      newMsg,
                      sizeof(Ospfv2LinkStateType) + sizeof(int)
                      + sizeof(BOOL) + sizeof(int));

    lsType = OSPFv2_NETWORK;
    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv2LinkStateType));
    msgInfo += sizeof(Ospfv2LinkStateType);
    memcpy(msgInfo, &interfaceId, sizeof(int));
    msgInfo += sizeof(int);
    memcpy(msgInfo, &flush, sizeof(BOOL));
    msgInfo += sizeof(BOOL);
    memcpy(msgInfo, &thisInterface->networkLSTimerSeqNumber, sizeof(BOOL));
    MESSAGE_Send(node, newMsg, (clocktype) delay);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2ScheduleSummaryLSA()
// PURPOSE      :Schedule summary LSA origination
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2ScheduleSummaryLSA(
    Node* node,
    NodeAddress destAddr,
    NodeAddress addrMask,
    Ospfv2DestType destType,
    unsigned int areaId,
    BOOL flush)
{
    Message* newMsg = NULL;
    char* msgInfo = NULL;
    Ospfv2LinkStateType lsType;

    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);
#ifdef JNE_LIB
    if (!thisArea)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Area doesn't exist",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(thisArea, "Area doesn't exist\n");

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfScheduleLSDB);

#ifdef ADDON_NGCNMS
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    // newly added for interface reset functionlaity.

    int i;
    int interfaceIndex = -1;
    for (i=0; i<node->numberInterfaces; i++)
    {
        if (ospf->iface[i].areaId == areaId)
        {
            interfaceIndex = i;
            break;
        }
    }
#ifdef JNE_LIB
    if (interfaceIndex == -1)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Problem determining interfaceIndex",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(interfaceIndex != -1,
        "Problem determining interfaceIndex\n");

    MESSAGE_SetInstanceId(newMsg, interfaceIndex);

    // end newly added
#endif

    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Ospfv2LinkStateType) + sizeof(unsigned int)
            + sizeof(BOOL) + sizeof(NodeAddress)
            + sizeof(NodeAddress) + sizeof(Ospfv2DestType));

    if (destType == OSPFv2_DESTINATION_NETWORK)
    {
        lsType = OSPFv2_NETWORK_SUMMARY;
    }
    else
    {
        lsType = OSPFv2_ROUTER_SUMMARY;
    }

    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv2LinkStateType));
    msgInfo += sizeof(Ospfv2LinkStateType);
    memcpy(msgInfo, &areaId, sizeof(unsigned int));
    msgInfo += sizeof(unsigned int);
    memcpy(msgInfo, &flush, sizeof(BOOL));
    msgInfo += sizeof(BOOL);
    memcpy(msgInfo, &destAddr, sizeof(NodeAddress));
    msgInfo += sizeof(NodeAddress);
    memcpy(msgInfo, &addrMask, sizeof(NodeAddress));
    msgInfo += sizeof(NodeAddress);
    memcpy(msgInfo, &destType, sizeof(Ospfv2DestType));

    MESSAGE_Send(node, newMsg, (clocktype) OSPFv2_MIN_LS_INTERVAL);
}


// BGP-OSPF Patch Start
//-------------------------------------------------------------------------//
// NAME         :Ospfv2ScheduleASExternalLSA()
// PURPOSE      :Schedule AS and NSSA external LSA origination
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2ScheduleASExternalLSA(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    BOOL flush,
    Ospfv2LinkStateType lsType)
{
    char* msgInfo = NULL;

    Message* newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfScheduleLSDB);
    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Ospfv2LinkStateType) + (sizeof(NodeAddress) * 3)
            + sizeof(int) + sizeof(BOOL));

#ifdef ADDON_NGCNMS
    MESSAGE_SetInstanceId(newMsg, -1);
#endif

    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv2LinkStateType));
    msgInfo += sizeof(Ospfv2LinkStateType);
    memcpy(msgInfo, &destAddress, sizeof(NodeAddress));
    msgInfo += sizeof(NodeAddress);
    memcpy(msgInfo, &destAddressMask, sizeof(NodeAddress));
    msgInfo += sizeof(NodeAddress);
    memcpy(msgInfo, &nextHopAddress, sizeof(NodeAddress));
    msgInfo += sizeof(NodeAddress);
    memcpy(msgInfo, &external2Cost, sizeof(int));
    msgInfo += sizeof(int);
    memcpy(msgInfo, &flush, sizeof(BOOL));

    MESSAGE_Send(node, newMsg, (clocktype) OSPFv2_MIN_LS_INTERVAL);
}
// BGP-OSPF Patch End
// NPDR Temp fix

#ifdef ADDON_MA
void Ospfv2ScheduleASExternalLSA_ForMA(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    BOOL flush,
    clocktype delay)

{
    char* msgInfo = NULL;
    Ospfv2LinkStateType lsType;


    Message* newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfScheduleLSDB);
    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Ospfv2LinkStateType) + (sizeof(NodeAddress) * 3)
            + sizeof(int) + sizeof(BOOL));




    lsType = OSPFv2_ROUTER_AS_EXTERNAL;

    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv2LinkStateType));
    msgInfo += sizeof(Ospfv2LinkStateType);
    memcpy(msgInfo, &destAddress, sizeof(NodeAddress));
    msgInfo += sizeof(NodeAddress);
    memcpy(msgInfo, &destAddressMask, sizeof(NodeAddress));
    msgInfo += sizeof(NodeAddress);
    memcpy(msgInfo, &nextHopAddress, sizeof(NodeAddress));
    msgInfo += sizeof(NodeAddress);
    memcpy(msgInfo, &external2Cost, sizeof(int));
    msgInfo += sizeof(int);
    memcpy(msgInfo, &flush, sizeof(BOOL));

    MESSAGE_Send(node, newMsg, delay);
}
#endif
//-------------------------------------------------------------------------//
// NAME         :Ospfv2ScheduleLSA()
// PURPOSE      :Schedule LSA origination
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2ScheduleLSA(
    Node* node,
    Ospfv2LinkStateHeader* LSHeader,
    unsigned int areaId)
{
    if (LSHeader->linkStateType == OSPFv2_ROUTER)
    {
        Ospfv2ScheduleRouterLSA(node, areaId, FALSE);
    }
    else if (LSHeader->linkStateType == OSPFv2_NETWORK)
    {
        int interfaceId = NetworkIpGetInterfaceIndexFromAddress(
                                node,
                                LSHeader->linkStateID);
        Ospfv2ScheduleNetworkLSA(node, interfaceId, FALSE);
    }
    else if (LSHeader->linkStateType == OSPFv2_NETWORK_SUMMARY)
    {
        NodeAddress addrMask;

        memcpy(&addrMask, LSHeader + 1, sizeof(NodeAddress));
        Ospfv2ScheduleSummaryLSA(
            node, LSHeader->linkStateID, addrMask,
            OSPFv2_DESTINATION_NETWORK, areaId, FALSE);
    }
    else if (LSHeader->linkStateType == OSPFv2_ROUTER_SUMMARY)
    {
        NodeAddress addrMask = 0xFFFFFFFF;

        Ospfv2ScheduleSummaryLSA(
            node, LSHeader->linkStateID, addrMask,
            OSPFv2_DESTINATION_ASBR, areaId, FALSE);
    }
    else if (LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
    {
        Ospfv2ExternalLinkInfo* ospfv2ExternalLinkInfo;

        ospfv2ExternalLinkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);

        Ospfv2ScheduleASExternalLSA(
            node, LSHeader->linkStateID, ospfv2ExternalLinkInfo->networkMask,
            0,
            Ospfv2ExternalLinkGetMetric(ospfv2ExternalLinkInfo->ospfMetric),
            FALSE,
            OSPFv2_ROUTER_AS_EXTERNAL);
    }
    else if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
    {
        Ospfv2ExternalLinkInfo* ospfv2ExternalLinkInfo;

        ospfv2ExternalLinkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);

        Ospfv2ScheduleASExternalLSA(
            node, LSHeader->linkStateID, ospfv2ExternalLinkInfo->networkMask,
            0,
            Ospfv2ExternalLinkGetMetric(ospfv2ExternalLinkInfo->ospfMetric),
            FALSE,
            OSPFv2_ROUTER_NSSA_EXTERNAL);
    }
    /***** Start: OPAQUE-LSA *****/
    else if (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE)
    {
        Ospfv2ScheduleOpaqueLSA(node, OSPFv2_AS_SCOPE_OPAQUE);
    }
    /***** End: OPAQUE-LSA *****/
    else
    {
        // Shouldn't reach here
#ifdef JNE_LIB
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                          "Wrong LS type",
                          JNE::CRITICAL);
#endif
        ERROR_Assert(FALSE, "Wrong LS type\n");
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2RemoveLSAFromList()
// PURPOSE      :Remove LSA from list
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//


void Ospfv2RemoveLSAFromList(
    Node* node,
    Ospfv2List* list,
    char* LSA)
{
    if (list == NULL)
    {
        return;
    }

    Ospfv2ListItem* listItem = list->first;
    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;
    Ospfv2LinkStateHeader* listLSHeader = NULL;

    while (listItem)
    {
        listLSHeader = (Ospfv2LinkStateHeader*) listItem->data;
#ifdef ADDON_MA
        BOOL  isMatched;
         if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL ||
            LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)&&
            (listLSHeader->linkStateType == LSHeader->linkStateType))
        {
            /*
            Ospfv2ExternalLinkInfo* linkInfo;
            Ospfv2ExternalLinkInfo* listInfo;
            linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
            listInfo = (Ospfv2ExternalLinkInfo*) (listLSHeader + 1);

            if (linkInfo->networkMask == listInfo->networkMask)
            */
            if (LSHeader->mask == listLSHeader->mask)
            {
                isMatched = TRUE;
            }else
            {
                isMatched = FALSE;
            }
        }else
        {
           isMatched = TRUE;
        }

        if (LSHeader->advertisingRouter == listLSHeader->advertisingRouter &&
            LSHeader->linkStateID == listLSHeader->linkStateID &&
            isMatched)
#else
        if (listLSHeader->advertisingRouter == LSHeader->advertisingRouter
            && listLSHeader->linkStateID == LSHeader->linkStateID)
#endif
        {
            Ospfv2RemoveFromList(node, list, listItem, FALSE);
            break;
        }

        listItem = listItem->next;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2IsAreaNbrStateExchangeOrLoading
// PURPOSE      :Checks whether any neighbor in the area is in state Exchange
//               or Loading
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2IsAreaNbrStateExchangeOrLoading(
    Node* node,
    unsigned int areaId)
{
    BOOL stateFound = FALSE;
    Ospfv2ListItem* tempListItem = NULL;
    Ospfv2Neighbor* tempNeighborInfo = NULL;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int interfaceIndex;

    for (interfaceIndex = 0; interfaceIndex < node->numberInterfaces;
                                                           interfaceIndex++)
    {
        if (NetworkIpGetUnicastRoutingProtocolType(node, interfaceIndex) !=
            ROUTING_PROTOCOL_OSPFv2)
        {
            continue;
        }

        if (areaId != OSPFv2_INVALID_AREA_ID &&
            ospf->iface[interfaceIndex].areaId != areaId)
        {
            continue;
        }

        tempListItem = ospf->iface[interfaceIndex].neighborList->first;

        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;
#ifdef JNE_LIB
            if (!tempNeighborInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                                  "Neighbor not found in the Neighbor list",
                                  JNE::CRITICAL);
            }
#endif
            ERROR_Assert(tempNeighborInfo,
                "Neighbor not found in the Neighbor list");

            // check if the router's neighbors are in state Exchange/Loading
            if ((tempNeighborInfo->state == S_Exchange) ||
                      (tempNeighborInfo->state == S_Loading))
            {
                stateFound = TRUE;
                return stateFound;
            }
            tempListItem = tempListItem->next;
        }//end of while
    }//end of for

    return stateFound;
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2IsLSAInNbrRetxList()
// PURPOSE      :Check for LSA in neibhbor retransmission list
// ASSUMPTION   :None
// RETURN VALUE :BOOL
//-------------------------------------------------------------------------//
static
BOOL Ospfv2IsLSAInNbrRetxList(
    Node* node,
    char* LSA,
    unsigned int areaId)
{
    BOOL wasFound = FALSE;
    Ospfv2ListItem* tempListItem = NULL;
    Ospfv2Neighbor* tempNeighborInfo = NULL;
    Ospfv2ListItem* listItem = NULL;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;
    int interfaceIndex;

    for (interfaceIndex = 0; interfaceIndex < node->numberInterfaces;
                                                           interfaceIndex++)
    {
        if (NetworkIpGetUnicastRoutingProtocolType(node, interfaceIndex) !=
            ROUTING_PROTOCOL_OSPFv2)
        {
            continue;
        }

        if (areaId != OSPFv2_INVALID_AREA_ID &&
            ospf->iface[interfaceIndex].areaId != areaId)
        {
            continue;
        }

        tempListItem = ospf->iface[interfaceIndex].neighborList->first;

        // Search for the neighbor in my neighbor list.
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;

#ifdef JNE_LIB
            if (!tempNeighborInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                                  "Neighbor not found in the Neighbor list",
                                  JNE::CRITICAL);
            }
#endif
            ERROR_Assert(tempNeighborInfo,
                "Neighbor not found in the Neighbor list");

            // check whether it is in any neighbors retransmission list
            listItem = tempNeighborInfo->linkStateRetxList->first;

            while (listItem)
            {
                // If LSA exists in retransmission list
                if (Ospfv2CompareLSAToListItem(
                        node,
                        LSHeader,
                        (Ospfv2LinkStateHeader*) listItem->data))
                {
                    wasFound = TRUE;
                    return wasFound;
                }
                listItem = listItem->next;
            }

            tempListItem = tempListItem->next;
        }//end of while
    }//end of for

    return wasFound;
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2RemoveLSAFromLSDB()
// PURPOSE      :Remove LSA from LSDB
// ASSUMPTION   :None
// RETURN VALUE :None
// NOTE         :To remove AS-External LSA from LSDB call this function with
//               areaId argument as OSPFv2_INVALID_AREA_ID.
//-------------------------------------------------------------------------//

void Ospfv2RemoveLSAFromLSDB(
    Node* node,
    char* LSA,
    unsigned int areaId)
{
    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;
    char linkStateType = LSHeader->linkStateType;
    NodeAddress linkStateID = LSHeader->linkStateID;
    NodeAddress advertisingRouter = LSHeader->advertisingRouter;
    Ospfv2Area*  thisArea = NULL;
    BOOL removeFromSend = TRUE;

    // BGP-OSPF Patch Start
    if (LSHeader->linkStateType != OSPFv2_ROUTER_AS_EXTERNAL &&
        LSHeader->linkStateType != OSPFv2_ROUTER_NSSA_EXTERNAL &&
        /***** Start: OPAQUE-LSA *****/
        LSHeader->linkStateType != OSPFv2_AS_SCOPE_OPAQUE)
        /***** End: OPAQUE-LSA *****/
    {
        thisArea = Ospfv2GetArea(node, areaId);
#ifdef JNE_LIB
        if (!thisArea)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Area doesn't exist",
                              JNE::CRITICAL);
        }
#endif
        ERROR_Assert(thisArea, "Area doesn't exist\n");
    }
    // BGP-OSPF Patch End

    switch (LSHeader->linkStateType)
    {
        case OSPFv2_ROUTER:
        {
            Ospfv2RemoveLSAFromList(node, thisArea->routerLSAList, LSA);
            break;
        }

        case OSPFv2_NETWORK:
        {
            Ospfv2RemoveLSAFromList(node, thisArea->networkLSAList, LSA);
            break;
        }

        case OSPFv2_NETWORK_SUMMARY:
        {
            Ospfv2RemoveLSAFromList(
                node, thisArea->networkSummaryLSAList, LSA);
            break;
        }
        // BGP-OSPF Patch Start
        case OSPFv2_ROUTER_SUMMARY:
        {
            Ospfv2RemoveLSAFromList(
                node, thisArea->routerSummaryLSAList, LSA);
            break;
        }
        case OSPFv2_ROUTER_AS_EXTERNAL:
        {
            Ospfv2Data* ospf = (Ospfv2Data*)
                NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
#ifdef JNE_LIB
            if (areaId != OSPFv2_INVALID_AREA_ID)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Wrong function call",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(areaId == OSPFv2_INVALID_AREA_ID,
                "Wrong function call\n");
            Ospfv2RemoveLSAFromList(node, ospf->asExternalLSAList, LSA);
            break;
        }
        // BGP-OSPF Patch End
        case OSPFv2_GROUP_MEMBERSHIP:
        {
            Ospfv2RemoveLSAFromList(
                node, thisArea->groupMembershipLSAList, LSA);
            removeFromSend = FALSE;
            break;
        }
        case OSPFv2_ROUTER_NSSA_EXTERNAL:
        {
            Ospfv2Data* ospf = (Ospfv2Data*)
                NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
#ifdef JNE_LIB
            if (areaId != OSPFv2_INVALID_AREA_ID)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Wrong function call",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(areaId == OSPFv2_INVALID_AREA_ID,
                "Wrong function call\n");
            Ospfv2RemoveLSAFromList(node, ospf->nssaExternalLSAList, LSA);
            break;
        }
        /***** Start: OPAQUE-LSA *****/
        case OSPFv2_AS_SCOPE_OPAQUE:
        {
            Ospfv2Data* ospf = (Ospfv2Data*)
                NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
            Ospfv2RemoveLSAFromList(node, ospf->ASOpaqueLSAList, LSA);
            break;
        }
        /***** End: OPAQUE-LSA *****/
        default:
        {
            removeFromSend = FALSE;
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Unknown linkStateType",
                              JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, "Unknow linkStateType\n");
        }
    }

    if (removeFromSend)
    {
        Ospfv2RemoveFromSendList(node,
                                 linkStateType,
                                 linkStateID,
                                 advertisingRouter,
                                 areaId);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2FlushLSA()
// PURPOSE      :Flush LSA from routing domain
// ASSUMPTION   :None
// RETURN VALUE :None
// NOTE         :To flush AS-External LSA call this function with areaId
//               argument as OSPFv2_INVALID_AREA_ID. This was just because
//               AS-External LSAs are particularly associated with entire
//               domain and not to any specific are. So we need to flood
//               this LSA in each attached area.
//-------------------------------------------------------------------------//

void Ospfv2FlushLSA(Node* node, char* LSA, unsigned int areaId)
{
    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;

    //RFC:1793 Section 2.5
    //Both changes pertain only to DoNotAge LSAs, and in both cases a flushed
    //LSA’s LS age field should be set to MaxAge and not DoNotAge+MaxAge.

    //RFC:1793 Section 2.2
    //In particular, DoNotAge+MaxAge is equivalent to MaxAge; however for
    //backward-compatibility the MaxAge form should always be used when
    //flushing LSAs from the routing domain
    if (LSHeader->linkStateAge == (OSPFv2_LSA_MAX_AGE/SECOND))
    {
        return;
    }

    LSHeader->linkStateAge = (OSPFv2_LSA_MAX_AGE / SECOND);

    if (Ospfv2DebugFlood(node))
    {
        char timeStr[100];
        TIME_PrintClockInSecond(node->getNodeTime() + getSimStartTime(node),
                                timeStr);
        printf("Node %u flush LSA in area %u at time %s\n",
            node->nodeId, areaId, timeStr);
        char lsIDStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(LSHeader->linkStateID, lsIDStr);
        printf("    Type = %d, ID = %s\n",
            LSHeader->linkStateType, lsIDStr);
    }

    if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL) ||
        /***** Start: OPAQUE-LSA *****/
        (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
        /***** End: OPAQUE-LSA *****/
    {
        Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
        Ospfv2ListItem* listItem = ospf->area->first;
#ifdef JNE_LIB
        if (areaId != OSPFv2_INVALID_AREA_ID)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Wrong function call",
                              JNE::CRITICAL);
        }
#endif
        ERROR_Assert(areaId == OSPFv2_INVALID_AREA_ID,
            "Wrong function call\n");

        while (listItem)
        {
            Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;
#ifdef JNE_LIB
            if (!thisArea)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                                  "Area structure not initialized",
                                  JNE::CRITICAL);
            }
#endif
            ERROR_Assert(thisArea, "Area structure not initialized");
            if (thisArea->externalRoutingCapability)
            {
                Ospfv2FloodLSA(node,ANY_INTERFACE, LSA, ANY_DEST,
                    thisArea->areaID);
                Ospfv2AddLsaToMaxAgeLsaList(node, LSA, thisArea->areaID);
            }
            listItem = listItem->next;
        }
    }
    else if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
    {
        Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
        Ospfv2ListItem* listItem = ospf->area->first;

#ifdef JNE_LIB
        if (areaId != OSPFv2_INVALID_AREA_ID)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Wrong function call",
                              JNE::CRITICAL);
        }
#endif
        ERROR_Assert(areaId == OSPFv2_INVALID_AREA_ID,
            "Wrong function call\n");

        while (listItem)
        {
            Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;
#ifdef JNE_LIB
            if (!thisArea)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Area structure not initialized",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(thisArea, "Area structure not initialized");

            if (thisArea->isNSSAEnabled)
            {
               Ospfv2FloodLSA(node,ANY_INTERFACE, LSA,
                    ANY_DEST, thisArea->areaID);
               Ospfv2AddLsaToMaxAgeLsaList(node, LSA, thisArea->areaID);
            }
            listItem = listItem->next;
        }
    }
    else
    {
        // Flood LSA
        Ospfv2FloodLSA(node,ANY_INTERFACE, LSA, ANY_DEST, areaId);
        Ospfv2AddLsaToMaxAgeLsaList(node, LSA, areaId);
    }

    Ospfv2ScheduleSPFCalculation(node);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2FloodThroughAS
// PURPOSE      :Flood LSA throughout the autonomous system.
// ASSUMPTION   :None.
// RETURN VALUE :TRUE if LSA flooded back to receiving interface,
//               FALSE otherwise.
//-------------------------------------------------------------------------//

BOOL Ospfv2FloodThroughAS(
    Node* node,
    int interfaceIndex,
    char* LSA,
    NodeAddress srcAddr)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Area* thisArea = NULL;
    Ospfv2ListItem* listItem = NULL;
    BOOL retVal = FALSE;

    listItem = ospf->area->first;

    while (listItem)
    {
        thisArea = (Ospfv2Area*) listItem->data;

        // If this is not a Stub area, flood lsa through this area
        if (thisArea->externalRoutingCapability)
        {
            if (Ospfv2FloodLSA(node,
                               interfaceIndex,
                               LSA,
                               srcAddr,
                               thisArea->areaID))
            {
                retVal = TRUE;
            }
        }

        listItem = listItem->next;
    }

    return retVal;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2FloodThroughNSSA
// PURPOSE      :Flood LSA throughout the autonomous system.
// ASSUMPTION   :None.
// RETURN VALUE :TRUE if LSA flooded back to receiving interface,
//               FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2FloodThroughNSSA(
    Node* node,
    int interfaceIndex,
    char* LSA,
    NodeAddress srcAddr,
    unsigned int areaID)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Area* thisArea = NULL;
    Ospfv2ListItem* listItem = NULL;
    BOOL retVal = FALSE;

    listItem = ospf->area->first;

    while (listItem)
    {
        thisArea = (Ospfv2Area*) listItem->data;

        // If this is not a Stub area, flood lsa through this area
        if (thisArea &&
            thisArea->isNSSAEnabled == TRUE &&
            thisArea->areaID == areaID)
        {
            if (Ospfv2FloodLSA(node,
                               interfaceIndex,
                               LSA,
                               srcAddr,
                               thisArea->areaID))
            {
                retVal = TRUE;
            }
            break;
        }

        listItem = listItem->next;
    }

    return retVal;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2RefreshLSA()
// PURPOSE      :Refreshing LSA. The function simply increment sequence
//               number of the LSA by 1 and set the age to 0.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2RefreshLSA(
    Node* node,
    Ospfv2ListItem* LSAItem,
    unsigned int areaId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSAItem->data;

    if (LSHeader->linkStateSequenceNumber ==
            (signed int) OSPFv2_MAX_SEQUENCE_NUMBER)
    {
        // Sequence number reaches the maximum value. We need to
        // flush this instance first before originating any instance.
        if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
            /***** Start: OPAQUE-LSA *****/
            || (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
            /***** End: OPAQUE-LSA *****/
        {
            Ospfv2FlushLSA(node,
                          (char*) LSHeader,
                          OSPFv2_INVALID_AREA_ID);
        }
        else
        {
            Ospfv2FlushLSA(node, (char*) LSHeader, areaId);
        }
        Ospfv2ScheduleLSA(node, LSHeader, areaId);
    }
    else
    {
        LSHeader->linkStateSequenceNumber += 1;
        LSHeader->linkStateAge = 0;
        LSAItem->timeStamp = node->getNodeTime();

        LSHeader->linkStateCheckSum = OspfFindLinkStateChecksum(LSHeader);

        if (OSPFv2_DEBUG_LSDBErr)
        {
            printf("    Node %u: Refreshing LSA. LSType = %d\n",
                    node->nodeId, LSHeader->linkStateType);
        }

#ifdef IPNE_INTERFACE
        // Calculate LS Checksum here
        //CalculateLinkStateChecksum(node, LSHeader);
#endif

        // Flood LSA
        if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
            /***** Start: OPAQUE-LSA *****/
            || (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
            /***** End: OPAQUE-LSA *****/
        {
            Ospfv2FloodThroughAS(node,
                                 ANY_INTERFACE,
                                 (char*) LSHeader,
                                 ANY_DEST);
        }
        else if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
        {
            Ospfv2FloodThroughNSSA(node,
                                   ANY_INTERFACE,
                                   (char*) LSHeader,
                                   ANY_DEST,
                                   areaId);
        }
        else
        {
            Ospfv2FloodLSA(node,
                           ANY_INTERFACE,
                           (char*) LSHeader,
                           ANY_DEST,
                           areaId);
        }

        ospf->stats.numLSARefreshed++;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2InterfaceStatusHandler()
// PURPOSE      :Handle mac alert.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2InterfaceStatusHandler(
    Node* node,
    int interfaceIndex,
    MacInterfaceState state)
{
    switch (state)
    {
        case MAC_INTERFACE_DISABLE:
        {
            Ospfv2HandleInterfaceEvent(node,
                                       interfaceIndex,
                                       E_InterfaceDown);
            break;
        }

        case MAC_INTERFACE_ENABLE:
        {
            Ospfv2HandleInterfaceEvent(node,
                                       interfaceIndex,
                                       E_InterfaceUp);
            break;
        }

        default:
        {
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Unknown MacInterfaceState",
                              JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, "Unknown MacInterfaceState\n");
        }
    }
}


//-------------------------------------------------------------------------//
//                          OSPF HELLO PROTOCOL                            //
//-------------------------------------------------------------------------//
#ifdef ADDON_BOEINGFCS
// ******* Start NEW for ROSPF *********

clocktype Ospfv2GetHelloInterval(Node* node, int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
    NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    if (ospf)
    {
        return ospf->iface[interfaceIndex].helloInterval;
    }
    else
    {
        return OSPFv2_HELLO_INTERVAL;
    }
}

clocktype Ospfv2GetDeadInterval(Node* node, int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
    NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    if (ospf)
    {
        return ospf->iface[interfaceIndex].routerDeadInterval;
    }
    else
    {
        return OSPFv2_ROUTER_DEAD_INTERVAL;
    }
}

// ******** End NEW for ROSPF *********
#endif
//-------------------------------------------------------------------------//
// NAME         :Ospfv2ListDREligibleRouters()
// PURPOSE      :Make list of DR eligible routers.
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------//

static
void Ospfv2ListDREligibleRouters(
    Node* node,
    Ospfv2List* nbrList,
    Ospfv2List* eligibleRoutersList,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem = NULL;
    Ospfv2Neighbor* nbrInfo = NULL;
    Ospfv2DREligibleRouter* newRouter = NULL;

    // Calculating router itself is considered to be on the list
    if (ospf->iface[interfaceIndex].routerPriority > 0)
    {
        newRouter
            = (Ospfv2DREligibleRouter *)
              MEM_malloc(sizeof(Ospfv2DREligibleRouter));

        newRouter->routerID = ospf->routerID;
        newRouter->routerPriority =
            ospf->iface[interfaceIndex].routerPriority;
        newRouter->routerIPAddress = ospf->iface[interfaceIndex].address;
        newRouter->DesignatedRouter =
                ospf->iface[interfaceIndex].designatedRouterIPAddress;
        newRouter->BackupDesignatedRouter =
            ospf->iface[interfaceIndex].backupDesignatedRouterIPAddress;

        Ospfv2InsertToList(eligibleRoutersList,
                           0,
                           (void*) newRouter);
    }

    listItem = nbrList->first;

    while (listItem)
    {
        nbrInfo = (Ospfv2Neighbor*) listItem->data;

        // Consider only neighbors having established bidirectional
        // communication with this router and have rtrPriority > 0

        if (nbrInfo->state >= S_TwoWay && nbrInfo->neighborPriority > 0)
        {
            newRouter
            = (Ospfv2DREligibleRouter *)
              MEM_malloc(sizeof(Ospfv2DREligibleRouter));

            newRouter->routerID = nbrInfo->neighborID;
            newRouter->routerPriority = nbrInfo->neighborPriority;
            newRouter->routerIPAddress = nbrInfo->neighborIPAddress;
            newRouter->DesignatedRouter = nbrInfo->neighborDesignatedRouter;
            newRouter->BackupDesignatedRouter =
                    nbrInfo->neighborBackupDesignatedRouter;

            Ospfv2InsertToList(eligibleRoutersList,
                               0,
                               (void*) newRouter);
        }

        listItem = listItem->next;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2ElectBDR()
// PURPOSE      :Elect BDR for desired interface.
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------//

static
void Ospfv2ElectBDR(
    Node* node,
    Ospfv2List* eligibleRoutersList,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem = NULL;
    Ospfv2DREligibleRouter* routerInfo = NULL;
    Ospfv2DREligibleRouter* tempBDR = NULL;
    BOOL flag = FALSE;

    for (listItem = eligibleRoutersList->first; listItem;
            listItem = listItem->next)
    {
        routerInfo = (Ospfv2DREligibleRouter*) listItem->data;

        // If the router declared itself to be DR
        // it is not eligible to become BDR

        if (routerInfo->routerIPAddress == routerInfo->DesignatedRouter)
        {
            continue;
        }

        // If neighbor declared itself to be BDR
        if (routerInfo->routerIPAddress ==
            routerInfo->BackupDesignatedRouter)
        {
            if ((flag) && (tempBDR) &&
                ((tempBDR->routerPriority > routerInfo->routerPriority) ||
                ((tempBDR->routerPriority == routerInfo->routerPriority) &&
                (tempBDR->routerID > routerInfo->routerID))))
            {
                // do nothing
            }
            else
            {
                tempBDR = routerInfo;
                flag = TRUE;
            }
        }
        else if (!flag)
        {
            if ((tempBDR) &&
                ((tempBDR->routerPriority > routerInfo->routerPriority) ||
                ((tempBDR->routerPriority == routerInfo->routerPriority) &&
                (tempBDR->routerID > routerInfo->routerID))))
            {
                // do nothing
            }
            else
            {
                tempBDR = routerInfo;
            }
        }
    }

    // Set BDR to this iface
    if (tempBDR)
    {
        ospf->iface[interfaceIndex].backupDesignatedRouter =
                                                   tempBDR->routerID;
        ospf->iface[interfaceIndex].backupDesignatedRouterIPAddress =
                                                   tempBDR->routerIPAddress;
    }
    else
    {
        ospf->iface[interfaceIndex].backupDesignatedRouter = 0;
        ospf->iface[interfaceIndex].backupDesignatedRouterIPAddress = 0;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2ElectDR()
// PURPOSE      :Elect DR for desired interface.
// ASSUMPTION   :None
// RETURN VALUE :Ospfv2InterfaceState
//-------------------------------------------------------------------------//

static
Ospfv2InterfaceState Ospfv2ElectDR(
    Node* node,
    Ospfv2List* eligibleRoutersList,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem = NULL;
    Ospfv2DREligibleRouter* routerInfo = NULL;
    Ospfv2DREligibleRouter* tempDR = NULL;

    for (listItem = eligibleRoutersList->first; listItem;
            listItem = listItem->next)
    {
        routerInfo = (Ospfv2DREligibleRouter*) listItem->data;

        // If router declared itself to be DR
        if (routerInfo->routerIPAddress == routerInfo->DesignatedRouter)
        {
            if ((tempDR) &&
                ((tempDR->routerPriority > routerInfo->routerPriority) ||
                ((tempDR->routerPriority == routerInfo->routerPriority) &&
                (tempDR->routerID > routerInfo->routerID))))
            {
                // do nothing
            }
            else
            {
                tempDR = routerInfo;
            }
        }
    }

    if (tempDR)
    {
        ospf->iface[interfaceIndex].designatedRouter = tempDR->routerID;
        ospf->iface[interfaceIndex].designatedRouterIPAddress =
                                                     tempDR->routerIPAddress;
    }
    else
    {
        ospf->iface[interfaceIndex].designatedRouter =
            ospf->iface[interfaceIndex].backupDesignatedRouter;
        ospf->iface[interfaceIndex].designatedRouterIPAddress =
            ospf->iface[interfaceIndex].backupDesignatedRouterIPAddress;
    }

    // Return new interface state
    if (ospf->iface[interfaceIndex].designatedRouter == ospf->routerID)
    {
        return S_DR;
    }
    else if (ospf->iface[interfaceIndex].backupDesignatedRouter ==
                     ospf->routerID)
    {
        return S_Backup;
    }
    else
    {
        return S_DROther;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2DRElection()
// PURPOSE      :Elect DR and BDR for desired network.
// ASSUMPTION   :None
// RETURN VALUE :Ospfv2InterfaceState
//-------------------------------------------------------------------------//

static
Ospfv2InterfaceState Ospfv2DRElection(
    Node* node,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2List* eligibleRoutersList = NULL;
    NodeAddress oldDR;
    NodeAddress oldBDR;
    Ospfv2InterfaceState oldState;
    Ospfv2InterfaceState newState;

    //Make a list of eligible routers that will participate in DR election
    Ospfv2InitList(&eligibleRoutersList);

    Ospfv2ListDREligibleRouters(
        node,
        ospf->iface[interfaceIndex].neighborList,
        eligibleRoutersList,
        interfaceIndex);

    // RFC-2328, Section: 9.4.1
    // Note current values for the network's DR and BDR

    oldDR = ospf->iface[interfaceIndex].designatedRouter;
    oldBDR = ospf->iface[interfaceIndex].backupDesignatedRouter;

    // Note interface state
    oldState = ospf->iface[interfaceIndex].state;

    // RFC-2328, Section: 9.4.2 & 9.4.3
    // First election of DR and BDR
    Ospfv2ElectBDR(node, eligibleRoutersList, interfaceIndex);
    newState = Ospfv2ElectDR(node,
                             eligibleRoutersList,
                             interfaceIndex);
    Ospfv2FreeList(node, eligibleRoutersList, FALSE);

    // RFC-2328, Section: 9.4.4
    // If Router X is now newly the Designated Router or newly the Backup
    // Designated Router, or is now no longer the Designated Router or no
    // longer the Backup Designated Router, repeat steps 2 and 3, and then
    // proceed to step 5.
    if ((newState != oldState) &&
        ((newState != S_DROther) || (oldState > S_DROther)))
    {
        Ospfv2InitList(&eligibleRoutersList);
        Ospfv2ListDREligibleRouters(
            node,
            ospf->iface[interfaceIndex].neighborList,
            eligibleRoutersList,
            interfaceIndex);

        Ospfv2ElectBDR(node, eligibleRoutersList, interfaceIndex);
        newState = Ospfv2ElectDR(node,
                                 eligibleRoutersList,
                                 interfaceIndex);
        Ospfv2FreeList(node, eligibleRoutersList, FALSE);
    }

    if (Ospfv2DebugISM(node))
    {
        char clockStr[100];
        char drStr[100];
        char bdrStr[100];

        TIME_PrintClockInSecond(node->getNodeTime()+getSimStartTime(node),
            clockStr);
        IO_ConvertIpAddressToString(
            ospf->iface[interfaceIndex].designatedRouter, drStr);
        IO_ConvertIpAddressToString(
            ospf->iface[interfaceIndex].backupDesignatedRouter, bdrStr);
        printf("    Node %d declare DR = %s and BDR = %s\n"
               "    on interface %d at time %s\n",
                node->nodeId, drStr, bdrStr, interfaceIndex, clockStr);
    }

    if (((newState == S_DR) || (newState == S_Backup))
        && ((oldState != S_DR) && (oldState != S_Backup)))
    {
        NetworkIpAddToMulticastGroupList(node, OSPFv2_ALL_D_ROUTERS);
#ifdef CYBER_CORE
        NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
        if (ip->isIgmpEnable == TRUE)
        {
            if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
            {
                IgmpJoinGroup(node, IAHEPGetIAHEPRedInterfaceIndex(node),
                                OSPFv2_ALL_D_ROUTERS, (clocktype)0);

                if (IAHEP_DEBUG)
                {
                    char addrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(OSPFv2_ALL_D_ROUTERS,
                        addrStr);
                    printf("\nRed Node: %d", node->nodeId);
                    printf("\tJoins Group: %s\n", addrStr);
                }
            }
        }
#endif
    }
    else if (((newState != S_DR) && (newState != S_Backup))
        && ((oldState == S_DR) || (oldState == S_Backup)))
    {
        NetworkIpRemoveFromMulticastGroupList(node,
                                              OSPFv2_ALL_D_ROUTERS);
#ifdef CYBER_CORE
        NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
        if (ip->isIgmpEnable == TRUE)
        {
            if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
            {
                IgmpLeaveGroup(node, IAHEPGetIAHEPRedInterfaceIndex(node),
                    OSPFv2_ALL_D_ROUTERS, (clocktype)0);
            }
        }
#endif
    }

    // RFC-2328, Section: 9.4.7
    // If the above calculations have caused the identity of either the
    // Designated Router or Backup Designated Router to change, the set
    // of adjacencies associated with this interface will need to be
    // modified.

    if ((oldDR != ospf->iface[interfaceIndex].designatedRouter) ||
        (oldBDR != ospf->iface[interfaceIndex].backupDesignatedRouter))
    {
        Ospfv2ListItem* listItem = NULL;
        Ospfv2Neighbor* nbrInfo = NULL;

        for (listItem = ospf->iface[interfaceIndex].neighborList->first;
                listItem; listItem = listItem->next)
        {
            nbrInfo = (Ospfv2Neighbor*) listItem->data;

            if (nbrInfo->state >= S_TwoWay)
            {
                Ospfv2ScheduleNeighborEvent(
                    node,
                    interfaceIndex,
                    nbrInfo->neighborIPAddress,
                    E_AdjOk);
            }
        }
    }
    return newState;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2FillNeighborField
// PURPOSE: Update Hello packet neighbor list for this node
// RETURN: Number of eligible neighbor
//-------------------------------------------------------------------------//

static
int Ospfv2FillNeighborField(
    Node* node,
    NodeAddress** nbrList,
    int interfaceId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2ListItem* tmpListItem = NULL;
    Ospfv2Neighbor* tmpNbInfo = NULL;
    Ospfv2Interface* thisIntf = NULL;
    int count = 0;
    int neighborCount;

    thisIntf = &ospf->iface[interfaceId];
    neighborCount = thisIntf->neighborList->size;

    if (neighborCount <= 0)
    {
        *nbrList = NULL;
        return 0;
    }

    *nbrList = (NodeAddress*) MEM_malloc(sizeof(NodeAddress)
        * neighborCount);

    tmpListItem = thisIntf->neighborList->first;

    // Get a list of all my neighbors from this interface.
    while (tmpListItem)
    {
        tmpNbInfo = (Ospfv2Neighbor*) tmpListItem->data;

        // Consider only neighbors from which Hello packet
        // have been seen recently

        if (tmpNbInfo->state >= S_Init)
        {
            // Add neighbor to the list.
            (*nbrList)[count] = tmpNbInfo->neighborID;

            if (OSPFv2_DEBUG_HELLOErr)
            {
                char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(
                    tmpNbInfo->neighborID, nbrAddrStr);
                printf("    neighborList[%d] = %s\n",
                        count, nbrAddrStr);
            }

            count++;
        }
        tmpListItem = tmpListItem->next;
    }
    return count;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleHelloPacket
// PURPOSE      :Process received hello packet
// ASSUMPTION   :None
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

void Ospfv2HandleHelloPacket(
    Node* node,
    Ospfv2HelloPacket* helloPkt,
    NodeAddress sourceAddr,
    NodeId sourceId,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Interface* thisIntf = NULL;
    Ospfv2Area* thisArea = NULL;
    Ospfv2Neighbor* tempNeighborInfo = NULL;
    Ospfv2ListItem* tempListItem = NULL;
    NodeAddress* tempNeighbor = NULL;
    BOOL neighborFound;
    int numNeighbor;
    int count;

    NodeAddress nbrPrevDR = 0;
    NodeAddress nbrPrevBDR = 0;
    int nbrPrevPriority = 0;
    BOOL routerIdInHelloPktBody = FALSE;

    thisIntf = &ospf->iface[interfaceIndex];

    thisArea = Ospfv2GetArea(node, thisIntf->areaId);
#ifdef JNE_LIB
    if (!thisArea)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                          "Didn't find specified area",
                          JNE::CRITICAL);
    }
#endif

    ERROR_Assert(thisArea, "Didn't find specified area\n");

    if (((thisIntf->helloInterval / SECOND) != helloPkt->helloInterval) ||
        ((thisIntf->routerDeadInterval / SECOND) !=
                                        helloPkt->routerDeadInterval))
    {
        if (OSPFv2_DEBUG_HELLO)
        {
            printf("    Mismatch in helloInterval or routerDeadInterval."
                   " Drop hello packet\n");
        }
        return;
    }
    if (thisIntf->type != OSPFv2_POINT_TO_POINT_INTERFACE &&
        !(thisIntf->isVirtualInterface) &&
        thisIntf->subnetMask != helloPkt->networkMask)
    {
        if (OSPFv2_DEBUG_HELLO)
        {
            printf("    Mismatch in subnetMask. Drop hello packet\n");
        }
        return;
    }

    // The setting of the E-bit found in the Hello Packet's Options
    // field must match this area's ExternalRoutingCapability.

    if (((Ospfv2OptionsGetExt(helloPkt->options))
            && (thisArea->externalRoutingCapability == FALSE))
        || (!(Ospfv2OptionsGetExt(helloPkt->options))
            && (thisArea->externalRoutingCapability == TRUE)))
    {
        if (OSPFv2_DEBUG_HELLO)
        {
            printf("    Option field mismatch. Drop hello packet\n");
        }

        return;
    }

    if (((Ospfv2OptionsGetNSSACapability(helloPkt->options))
       && (thisArea->isNSSAEnabled == FALSE)) ||
       (!(Ospfv2OptionsGetNSSACapability(helloPkt->options))
       && (thisArea->isNSSAEnabled == TRUE)))
   {
        if (OSPFv2_DEBUG_HELLO)
        {
            printf(" N bit mismatch. Drop hello packet\n");
        }

        return;
   }

    numNeighbor = (helloPkt->commonHeader.packetLength
                  - sizeof(Ospfv2HelloPacket)) / sizeof(NodeAddress);

#ifdef JNE_LIB
    if (!tempNeighbor)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                          "Neighbor part of the Hello packet is not present",
                          JNE::CRITICAL);
    }
#endif
    if (OSPFv2_DEBUG_HELLOErr)
    {
        Ospfv2PrintHelloPacket(helloPkt);
    }

    // Update my records of my neighbor routers.
    tempListItem = thisIntf->neighborList->first;
    neighborFound = FALSE;

    while (tempListItem)
    {
        tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;
#ifdef JNE_LIB
        if (!tempNeighborInfo)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Neighbor not found in the Neighbor list",
                JNE::CRITICAL);
        }
#endif
        ERROR_Assert(tempNeighborInfo,
            "Neighbor not found in the Neighbor list\n");

        if (thisIntf->type == OSPFv2_BROADCAST_INTERFACE
            || thisIntf->type == OSPFv2_POINT_TO_MULTIPOINT_INTERFACE
#ifdef ADDON_BOEINGFCS
            || thisIntf->type == OSPFv2_ROSPF_INTERFACE
#endif
            || thisIntf->type == OSPFv2_NBMA_INTERFACE)
        {
            if (tempNeighborInfo->neighborIPAddress == sourceAddr)
            {
                // The neighbor exists in our neighbor list, so no need to
                // add this neighbor to our neighbor list.
                neighborFound = TRUE;

                if (OSPFv2_DEBUG_HELLO)
                {
                    char srcAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(sourceAddr, srcAddrStr);
                    printf("    already know about neighbor %s\n",
                        srcAddrStr);
                }

                break;
            }
        }
        else
        {
            if (tempNeighborInfo->neighborID ==
                            helloPkt->commonHeader.routerID)
            {
                // If a point-to-point interface receives packests from
                // two diffrent interfaces of same node which have difrent
                // IP addresses, it should process only one of them.
                // For unnumbered interfaces they would have same IP address
                // so it should treat the packet as if they are coming from
                // same interface.
                if (tempNeighborInfo->neighborIPAddress != sourceAddr)
                {
                    return;
                }

                // The neighbor exists in our neighbor list, so no need to
                // add this neighbor to our neighbor list.
                neighborFound = TRUE;

                if (OSPFv2_DEBUG_HELLO)
                {
                    char srcAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(sourceAddr, srcAddrStr);
                    printf("    already know about neighbor %s\n",
                        srcAddrStr);
                }

                break;
            }
        }

        tempListItem = tempListItem->next;
    }

    if (neighborFound == FALSE)
    {
        // Neighbor does not exist in my neighbor list,
        // so add the neighbor into my neighbor list.
        tempNeighborInfo = (Ospfv2Neighbor*)
                                MEM_malloc(sizeof(Ospfv2Neighbor));

        memset(tempNeighborInfo, 0, sizeof(Ospfv2Neighbor));

        tempNeighborInfo->neighborPriority = helloPkt->rtrPri;
        tempNeighborInfo->neighborID = helloPkt->commonHeader.routerID;
        tempNeighborInfo->neighborIPAddress = sourceAddr;
#ifdef ADDON_BOEINGFCS
        tempNeighborInfo->neighborNodeId = sourceId;
#endif

        // Set neighbor's view of DR and BDR
        tempNeighborInfo->neighborDesignatedRouter =
            helloPkt->designatedRouter;
        tempNeighborInfo->neighborBackupDesignatedRouter =
            helloPkt->backupDesignatedRouter;

        // Initializes different lists
        Ospfv2InitList(&tempNeighborInfo->linkStateRetxList);
        Ospfv2InitList(&tempNeighborInfo->linkStateSendList);
        Ospfv2InitList(&tempNeighborInfo->DBSummaryList);
        Ospfv2InitList(&tempNeighborInfo->linkStateRequestList);

        Ospfv2InsertToList(
            thisIntf->neighborList, 0, (void*) tempNeighborInfo);

        ospf->neighborCount++;

        if (OSPFv2_DEBUG_HELLO)
        {
            char sourceAddrStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(sourceAddr, sourceAddrStr);
            printf("    adding neighbor %s to neighbor list\n",
                    sourceAddrStr);
        }
    }
#ifdef ADDON_NGCNMS
    else
    {
        // can happen for interface restarts...
        if (tempNeighborInfo->neighborID !=
           helloPkt->commonHeader.routerID)
        {
            tempNeighborInfo->neighborID = helloPkt->commonHeader.routerID;
        }
    }
#endif

    // NBMA mode is not cosidered yet
    if (thisIntf->type == OSPFv2_BROADCAST_INTERFACE
#ifdef ADDON_BOEINGFCS
        || thisIntf->type == OSPFv2_ROSPF_INTERFACE
#endif
        || thisIntf->type == OSPFv2_POINT_TO_MULTIPOINT_INTERFACE)
    {
        // Note changes in field DR, BDR, router priority for
        // possible use in later step.
        nbrPrevDR = tempNeighborInfo->neighborDesignatedRouter;
        nbrPrevBDR = tempNeighborInfo->neighborBackupDesignatedRouter;
        nbrPrevPriority = tempNeighborInfo->neighborPriority;
        tempNeighborInfo->neighborDesignatedRouter
            = helloPkt->designatedRouter;
        tempNeighborInfo->neighborBackupDesignatedRouter =
                helloPkt->backupDesignatedRouter;
        tempNeighborInfo->neighborPriority = helloPkt->rtrPri;
    }
    else if (thisIntf->type == OSPFv2_POINT_TO_POINT_INTERFACE)
    {
        nbrPrevPriority = tempNeighborInfo->neighborPriority;
        tempNeighborInfo->neighborIPAddress = sourceAddr;
        tempNeighborInfo->neighborPriority = helloPkt->rtrPri;
    }
    else
    {
        nbrPrevPriority = tempNeighborInfo->neighborPriority;
        tempNeighborInfo->neighborPriority = helloPkt->rtrPri;
    }

#ifdef ADDON_BOEINGFCS
    if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
    {
        RoutingCesRospfHandleOspfv2HelloPacket(node,
                                     interfaceIndex,
                                     sourceAddr,
                                     sourceId);

    }
    else
    {
#endif
    // Handle neighbor event
    Ospfv2HandleNeighborEvent(node,
                              interfaceIndex,
                              sourceAddr,
                              E_HelloReceived);

    // Check whether this router itself appear in the
    // list of neighbor contained in Hello packet.

    count = 0;

    if (numNeighbor > 0)
    {
        // Get the neighbor part of the Hello packet.
        tempNeighbor = (NodeAddress*)(helloPkt + 1);
        ERROR_Assert(tempNeighbor,
            "Neighbor part of the Hello packet is not present\n");
    }
    while (count < numNeighbor)
    {
        NodeAddress ipAddr;

        memcpy(&ipAddr, (tempNeighbor + count), sizeof(NodeAddress));

        if (ipAddr == ospf->routerID)
        {
            // Handle neighbor event
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      sourceAddr,
                                      E_TwoWayReceived);

            routerIdInHelloPktBody = TRUE;

            break;
        }
        count++;
    }

    if (ospf->supportDC && routerIdInHelloPktBody)//Two-Way
    {
        if (Ospfv2OptionsGetDC(helloPkt->options))
        {
            if (ospf->iface[interfaceIndex].ospfIfDemand == FALSE)
            {
                ospf->iface[interfaceIndex].helloSuppressionSuccess = FALSE;
            }
            else
            {
                ospf->iface[interfaceIndex].helloSuppressionSuccess = TRUE;
            }
        }
        else
        {
            ospf->iface[interfaceIndex].helloSuppressionSuccess = FALSE;
        }
    }

    if (count == numNeighbor)
    {
        // Handle neighbor event : S_OneWay
        Ospfv2HandleNeighborEvent(node,
                                  interfaceIndex,
                                  sourceAddr,
                                  E_OneWay);

        // Stop processing packet further
        return;
    }
#ifdef ADDON_BOEINGFCS
    }
#endif

    // If a change in the neighbor's Router Priority field was noted,
    // the receiving interface's state machine is scheduled with
    // the event NeighborChange.
    if (nbrPrevPriority != tempNeighborInfo->neighborPriority)
    {
        Ospfv2ScheduleInterfaceEvent(node,
                                     interfaceIndex,
                                     E_NeighborChange);
    }

    // If the neighbor is both declaring itself to be Designated Router and
    // the Backup Designated Router field in the packet is equal to 0.0.0.0
    // and the receiving interface is in state Waiting, the receiving
    // interface's state machine is scheduled with the event BackupSeen.
    // Otherwise, if the neighbor is declaring itself to be Designated Router
    // and it had not previously, or the neighbor is not declaring itself
    // Designated Router where it had previously, the receiving interface's
    // state machine is scheduled with the event NeighborChange.

    if ((helloPkt->designatedRouter == tempNeighborInfo->neighborIPAddress)
        && (helloPkt->backupDesignatedRouter == 0)
        && (thisIntf->state == S_Waiting))
    {
        Ospfv2ScheduleInterfaceEvent(node,
                                     interfaceIndex,
                                     E_BackupSeen);
    }
    else if (((helloPkt->designatedRouter ==
        tempNeighborInfo->neighborIPAddress) &&
        (nbrPrevDR != tempNeighborInfo->neighborIPAddress)) ||
        ((helloPkt->designatedRouter != tempNeighborInfo->neighborIPAddress)
        && (nbrPrevDR == tempNeighborInfo->neighborIPAddress)))
    {
        Ospfv2ScheduleInterfaceEvent(node,
                                     interfaceIndex,
                                     E_NeighborChange);
    }

    // If the neighbor is declaring itself to be Backup Designated Router and
    // the receiving interface is in state Waiting, the receiving interface's
    // state machine is scheduled with the event BackupSeen. Otherwise, if
    // neighbor is declaring itself to be Backup Designated Router and it had
    // not previously, or the neighbor is not declaring itself Backup
    // Designated Router where it had previously, the receiving interface's
    // state machine is scheduled with the event NeighborChange.

    if ((helloPkt->backupDesignatedRouter ==
        tempNeighborInfo->neighborIPAddress) &&
        (thisIntf->state == S_Waiting))
    {
        Ospfv2ScheduleInterfaceEvent(node,
                                     interfaceIndex,
                                     E_BackupSeen);
    }
    else if (((helloPkt->backupDesignatedRouter ==
              tempNeighborInfo->neighborIPAddress)
        && (nbrPrevBDR != tempNeighborInfo->neighborIPAddress))
        || ((helloPkt->backupDesignatedRouter !=
                 tempNeighborInfo->neighborIPAddress)
        && (nbrPrevBDR == tempNeighborInfo->neighborIPAddress)))
    {
        Ospfv2ScheduleInterfaceEvent(node,
                                     interfaceIndex,
                                     E_NeighborChange);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendHelloPacket
// PURPOSE      :Creating new Hello packet and send it to all neighbors.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2SendHelloPacket(Node* node, Int32 interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    clocktype delay;
    int numNeighbor;
    NodeAddress* neighborList = NULL;

    Message* helloMsg = NULL;
    Ospfv2HelloPacket* helloPacket = NULL;
    Int32 i = interfaceIndex;

#ifdef ADDON_BOEINGFCS
        if (ospf && ospf->iface[i].type == OSPFv2_ROSPF_INTERFACE)
        {
            return;
        }
#endif
        if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            return;
        }

        // If interface state is down, no hello should be sent
        //RFC:1793::SECTION 3.1::INTERFACE STATE MACHINE MODIFICATIONS
        //Hellos are sent out such an interface when it is in "Down" state,
        //at the reduced interval of PollInterval
        //if (ospf->iface[i].state == S_Down)
        //{
        //    return;
        //}
        if ((ospf->supportDC == FALSE)
            ||
            ((ospf->supportDC == TRUE) &&
                (ospf->iface[i].ospfIfDemand == FALSE)))
        {
            if (ospf->iface[i].state == S_Down)
            {
                return;
            }
        }

        Ospfv2Area* thisArea = Ospfv2GetArea(
                                            node,
                                            ospf->iface[i].areaId);

#ifdef JNE_LIB
        if (!thisArea)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Area not identified",
                JNE::CRITICAL);
        }
#endif
        ERROR_Assert(thisArea, "Area not identified\n");

        // Get all my neighbors that I've learned.
        numNeighbor = Ospfv2FillNeighborField(node,
                                              &neighborList,
                                              i);

        //RFC:1793::SECTION 3.1::INTERFACE STATE MACHINE MODIFICATIONS
        //An OSPF point-to-point interface connecting to a demand circuit is
        //considered to be in state "Point-to-point" iff its associated
        //neighbor is in state "1-Way" or greater; otherwise the interface
        //is considered to be in state "Down".If the hello negotiation
        //succeeds,Hellos will cease to be sent out the interface
        // whenever the associated neighbor reaches state "Full".*/
        if ((ospf->supportDC == TRUE)
            &&
            (ospf->iface[i].type == OSPFv2_POINT_TO_POINT_INTERFACE)
            &&
            (ospf->iface[i].ospfIfDemand == TRUE))
        {
            //there will be only one neighbor
            if (numNeighbor)
            {
                Ospfv2ListItem* tmpListItem = NULL;
                Ospfv2Neighbor* tmpNbInfo = NULL;
                Ospfv2Interface* thisIntf = NULL;

                thisIntf = &ospf->iface[interfaceIndex];
                tmpListItem = thisIntf->neighborList->first;

                tmpNbInfo = (Ospfv2Neighbor*) tmpListItem->data;

                if (tmpNbInfo->state >= S_Init)
                {
                    ospf->iface[i].state = S_PointToPoint;
                }
                else
                {
                    ospf->iface[i].state = S_Down;
                }

                if ((ospf->iface[i].helloSuppressionSuccess == TRUE)
                    &&
                    tmpNbInfo->state >= S_Full)
                {
                    //Do Not Send Hello Packet
                    if (OSPFv2_DEBUG_DEMANDCIRCUIT)
                    {
                        printf("\nNode %d\n", node->nodeId);
                        printf("\nNode %d\n", node->nodeId);
                        printf("\nSend Hello Packet:: As the neighbor "
                               "is in state >= Full and hello suppression "
                               "done so not sending "
                               "Hello Packets\n");
                    }
                    return;
                }

            }
        }

        // Now, create the Hello packet.
        helloMsg = MESSAGE_Alloc(node,
                                 NETWORK_LAYER,
                                 ROUTING_PROTOCOL_OSPFv2,
                                 MSG_ROUTING_OspfPacket);

        MESSAGE_PacketAlloc(node,
                            helloMsg,
                            sizeof(Ospfv2HelloPacket)
                            + (sizeof(NodeAddress) * numNeighbor),
                            TRACE_OSPFv2);

        helloPacket = (Ospfv2HelloPacket*)
                        MESSAGE_ReturnPacket(helloMsg);

        memset(helloPacket, 0, sizeof(Ospfv2HelloPacket)
                               + (sizeof(NodeAddress) * numNeighbor));

        //RFC:1793:Section:3.2
        //For OSPF broadcast and NBMA networks that have been configured as
        //demand circuits, there is no change to the sending and receiving
        //of Hellos,
        //RFC:1793:Section:3.2.1
        //Even if the above negotiation fails, the router should continue
        //setting the DC-bit in its Hellos and Database Descriptions (the
        //neighbor will just ignore the bit).

        //if this interface of router is attached to DC then it
        //will continue setting the DC bit in hello packet
        if ((ospf->supportDC == TRUE)
            &&
            (ospf->iface[i].ospfIfDemand == TRUE)
            &&
            (ospf->iface[i].type == OSPFv2_POINT_TO_POINT_INTERFACE))
        {
            Ospfv2OptionsSetDC(&(helloPacket->options), 1);
        }

        // Fill in the header fields for the Hello packet.
        Ospfv2CreateCommonHeader(node,
                                 &helloPacket->commonHeader,
                                 OSPFv2_HELLO);

        // Keep track of the Hello packet length so my neighbors can
        // determine how many neighbors are in the Hello packet.

        helloPacket->commonHeader.packetLength =
                (short int) (sizeof(Ospfv2HelloPacket)
                + (sizeof(NodeAddress) *  numNeighbor));

        // Add our neighbor list at the end of the Hello packet.

        if (numNeighbor > 0)
        {
            memcpy(helloPacket + 1,
               neighborList,
               sizeof(NodeAddress) * numNeighbor);
        }

        if (OSPFv2_DEBUG_HELLOErr)
        {
            Ospfv2PrintHelloPacket(node, helloPacket);
        }

        // Fill area ID field of header
        helloPacket->commonHeader.areaID = ospf->iface[i].areaId;

        // Set E-bit if not a stub area
        if (thisArea->externalRoutingCapability == TRUE)
        {
            Ospfv2OptionsSetExt(&(helloPacket->options), 1);
        }

        if (thisArea->isNSSAEnabled == TRUE)
        {
            Ospfv2OptionsSetNSSACapability(&(helloPacket->options), 1);
            Ospfv2OptionsSetExt(&(helloPacket->options), 0);
        }

        if ((ospf->supportDC == TRUE)
            &&
            (ospf->iface[i].ospfIfDemand == TRUE)
            &&
            (ospf->iface[i].type == OSPFv2_POINT_TO_POINT_INTERFACE))
        {
            if (ospf->iface[i].state == S_Down)
            {
                //Hello interval must be poll interval
                helloPacket->helloInterval = (short int)
                                 (OSPFv2_POLL_INTERVAL / SECOND);
            }
            else
            {
                helloPacket->helloInterval = (short int)
                                 (ospf->iface[i].helloInterval / SECOND);
            }
        }
        else
        {
                helloPacket->helloInterval = (short int)
                                 (ospf->iface[i].helloInterval / SECOND);
        }

        helloPacket->rtrPri =
            (unsigned char) ospf->iface[i].routerPriority;
        helloPacket->routerDeadInterval =
                (int) (ospf->iface[i].routerDeadInterval / SECOND);

        // Designated router and backup Designated router is identified
        // by it's IP interface address on the network

        helloPacket->designatedRouter =
                ospf->iface[i].designatedRouterIPAddress;
        helloPacket->backupDesignatedRouter =
                ospf->iface[i].backupDesignatedRouterIPAddress;

        if (ospf->iface[i].isVirtualInterface)
        {
            helloPacket->networkMask = 0;
        }
        else
        {
            helloPacket->networkMask = ospf->iface[i].subnetMask;
        }

        if (OSPFv2_DEBUG_HELLOErr)
        {
            Ospfv2PrintHelloPacket(helloPacket);
            printf("    HELLO packet length = %d\n",
                    helloPacket->commonHeader.packetLength);
        }

        //Trace sending pkt
        ActionData acnData;
        acnData.actionType = SEND;
        acnData.actionComment = NO_COMMENT;
        TRACE_PrintTrace(node, helloMsg, TRACE_NETWORK_LAYER,
                PACKET_OUT, &acnData);


        // Used to jitter the Hello packet broadcast.
        delay = (clocktype) (RANDOM_erand (ospf->seed)
            * OSPFv2_BROADCAST_JITTER);

    // Determine destination address
        NodeAddress dstAddr;
        if ((ospf->iface[i].type ==
                OSPFv2_BROADCAST_INTERFACE) ||
            (ospf->iface[i].type ==
                OSPFv2_POINT_TO_POINT_INTERFACE))
        {
            dstAddr = OSPFv2_ALL_SPF_ROUTERS;
            if (OSPFv2_DEBUG_HELLOErr)
            {
                char dstAddrStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(dstAddr, dstAddrStr);
                printf("        sending to %s\n", dstAddrStr);
            }
            NetworkIpSendRawMessageToMacLayerWithDelay(
                node,
                MESSAGE_Duplicate(node, helloMsg),
                NetworkIpGetInterfaceAddress(node, i),
                dstAddr,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_OSPF,
                1,
                i,
                ANY_DEST,
                delay);
        }
        else if (ospf->iface[i].type ==
                    OSPFv2_POINT_TO_MULTIPOINT_INTERFACE)
        {
            if (ospf->iface[i].NonBroadcastNeighborList->size != 0)
            {
                Ospfv2NonBroadcastNeighborListItem* tempListItem =
                    ospf->iface[i].NonBroadcastNeighborList->first;
                while (tempListItem)
                {
                    dstAddr = tempListItem->neighborAddr;
                    if (OSPFv2_DEBUG_HELLOErr)
                    {
                        char dstAddrStr[MAX_ADDRESS_STRING_LENGTH];
                        IO_ConvertIpAddressToString(dstAddr, dstAddrStr);
                        printf("        sending to %s\n", dstAddrStr);
                    }
                    NetworkIpSendRawMessageToMacLayerWithDelay(
                        node,
                        MESSAGE_Duplicate(node, helloMsg),
                        NetworkIpGetInterfaceAddress(node, i),
                        dstAddr,
                        IPTOS_PREC_INTERNETCONTROL,
                        IPPROTO_OSPF,
                        1,
                        i,
                        ANY_DEST,
                        delay);
                    tempListItem = tempListItem->next;
                }
            }
            else
            {
                dstAddr  = OSPFv2_ALL_SPF_ROUTERS;
                if (OSPFv2_DEBUG_HELLOErr)
                {
                    char dstAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(dstAddr, dstAddrStr);
                    printf("        sending to %s\n", dstAddrStr);
                }
                NetworkIpSendRawMessageToMacLayerWithDelay(
                    node,
                    MESSAGE_Duplicate(node, helloMsg),
                    NetworkIpGetInterfaceAddress(node, i),
                    dstAddr,
                    IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_OSPF,
                    1,
                    i,
                    ANY_DEST,
                    delay);
            }
        }

        MESSAGE_Free(node, helloMsg);
        if (numNeighbor > 0)
        {
            MEM_free(neighborList);
        }
        ospf->stats.numHelloSent++;


#if COLLECT_DETAILS
        // instrumented code
        numHelloBytes += MESSAGE_ReturnPacketSize(helloMsg);

        if (node->getNodeTime() - lastHelloPrint >= OSPFv2_PRINT_INTERVAL)
        {
            lastHelloPrint = node->getNodeTime();
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            printf("%s Number of Hello bytes sent = %ld\n",
                   clockStr, numHelloBytes);
        }
        // end instrumented code

#endif
}


//-------------------------------------------------------------------------//
//                     OSPF DATABASE SYNCHRONIZATION                       //
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendDatabaseDescriptionPacket()
// PURPOSE      :Send database description packet
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2SendDatabaseDescriptionPacket(
    Node* node,
    NodeAddress nbrAddr,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Area* thisArea =
        Ospfv2GetArea(node, ospf->iface[interfaceIndex].areaId);

    Message* dbDscrpMsg = NULL;
    Ospfv2DatabaseDescriptionPacket* dbDscrpPkt = NULL;
    Ospfv2Neighbor* nbrInfo = NULL;
    NodeAddress dstAddr;
    clocktype delay;

    // Find neighbor structure
    nbrInfo = Ospfv2GetNeighborInfoByIPAddress(node,
                                               interfaceIndex,
                                               nbrAddr);

#ifdef JNE_LIB
    if (!nbrInfo)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Neighbor not found in the Neighbor list",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(nbrInfo != NULL, "Neighbor not found in the Neighbor list");

    // Create Database Description packet.
    dbDscrpMsg = MESSAGE_Alloc(node,
                               NETWORK_LAYER,
                               ROUTING_PROTOCOL_OSPFv2,
                               MSG_ROUTING_OspfPacket);
    if (OSPFv2_DEBUG)
    {
        if (nbrInfo->masterSlave == T_Master)
        {
            printf("    is Master\n");
        }
        else
        {
            printf("    is Slave\n");
        }
    }

    if (nbrInfo->state == S_ExStart)
    {
        // Create empty packet
        MESSAGE_PacketAlloc(node,
                            dbDscrpMsg,
                            sizeof(Ospfv2DatabaseDescriptionPacket),
                            TRACE_OSPFv2);

        dbDscrpPkt = (Ospfv2DatabaseDescriptionPacket*)
                          MESSAGE_ReturnPacket(dbDscrpMsg);

        memset(dbDscrpPkt,
               0,
               sizeof(Ospfv2DatabaseDescriptionPacket));

        // Set Init, More and master-slave bit
        Ospfv2DatabaseDescriptionPacketSetInit(&(dbDscrpPkt->ospfDbDp), 0x1);
        Ospfv2DatabaseDescriptionPacketSetMore(&(dbDscrpPkt->ospfDbDp), 0x1);
        Ospfv2DatabaseDescriptionPacketSetMS(&(dbDscrpPkt->ospfDbDp), 0x1);
        /***** Start: OPAQUE-LSA *****/
        if (ospf->opaqueCapable)
        {
            Ospfv2DatabaseDescriptionPacketSetO(&(dbDscrpPkt->options), TRUE);
        }
        else
        {
            Ospfv2DatabaseDescriptionPacketSetO(&(dbDscrpPkt->options), FALSE);
        }
        /***** End: OPAQUE-LSA *****/

        dbDscrpPkt->ddSequenceNumber = nbrInfo->DDSequenceNumber;
        dbDscrpPkt->commonHeader.packetLength =
                sizeof(Ospfv2DatabaseDescriptionPacket);
        nbrInfo->optionsBits = dbDscrpPkt->ospfDbDp;
        if (Ospfv2DebugSync(node))
        {
            fprintf(stdout, "    sending empty DD packet with seqno = %u\n",
                             nbrInfo->DDSequenceNumber);
        }
    }
    else if (nbrInfo->state == S_Exchange)
    {
        Ospfv2LinkStateHeader* payloadBuff;
        int maxLSAHeader =
            (GetNetworkIPFragUnit(node, interfaceIndex)
            - sizeof(IpHeaderType)
            - sizeof(Ospfv2DatabaseDescriptionPacket))
            / sizeof(Ospfv2LinkStateHeader);

        int numLSAHeader = 0;

        payloadBuff = (Ospfv2LinkStateHeader*)
            MEM_malloc(maxLSAHeader * sizeof(Ospfv2LinkStateHeader));

        if (Ospfv2DebugSync(node))
        {
            fprintf(stdout, "    preparing DATABASE...\n");
            fprintf(stdout, "        maxLSAHeader = %d\n", maxLSAHeader);
        }

        // Get LSA Header list
        while ((numLSAHeader < maxLSAHeader)  &&
            Ospfv2GetTopLSAFromList(node,
                                    nbrInfo->DBSummaryList,
                                    &payloadBuff[numLSAHeader]))
        {
            numLSAHeader++;
        }

        // Create packet
        MESSAGE_PacketAlloc(
            node,
            dbDscrpMsg,
            sizeof(Ospfv2DatabaseDescriptionPacket)
                + numLSAHeader * sizeof(Ospfv2LinkStateHeader),
            TRACE_OSPFv2);

        dbDscrpPkt = (Ospfv2DatabaseDescriptionPacket*)
                     MESSAGE_ReturnPacket(dbDscrpMsg);

        memset(dbDscrpPkt,
               0,
               sizeof(Ospfv2DatabaseDescriptionPacket)
               + numLSAHeader * sizeof(Ospfv2LinkStateHeader));

        Ospfv2DatabaseDescriptionPacketSetInit(&(dbDscrpPkt->ospfDbDp), 0x0);
#ifdef ADDON_BOEINGFCS
        if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE &&
            RoutingCesRospfDatabaseDigestsEnabled(node, interfaceIndex))
        {
            // when using database digests, force MORE bit on in
            // negotiation DB desc packets even though we haven't generated
            // a DB summary list
            Ospfv2DatabaseDescriptionPacketSetMore(&(dbDscrpPkt->ospfDbDp),
                                                   1);
        }
        else
#endif
        if (Ospfv2ListIsEmpty(nbrInfo->DBSummaryList))
        {
            Ospfv2DatabaseDescriptionPacketSetMore(&(dbDscrpPkt->ospfDbDp),
                0);
        }
        else
        {
            if (Ospfv2DebugSync(node))
            {
                Ospfv2PrintDBSummaryList(node, nbrInfo->DBSummaryList);
            }

            Ospfv2DatabaseDescriptionPacketSetMore(&(dbDscrpPkt->ospfDbDp),
                1);
        }
        if (nbrInfo->masterSlave == T_Master)
        {
            Ospfv2DatabaseDescriptionPacketSetMS(&(dbDscrpPkt->ospfDbDp), 1);
        }
        else
        {
            Ospfv2DatabaseDescriptionPacketSetMS(&(dbDscrpPkt->ospfDbDp), 0);
        }
        /***** Start: OPAQUE-LSA *****/
        if (ospf->opaqueCapable)
        {
            Ospfv2DatabaseDescriptionPacketSetO(&(dbDscrpPkt->options), TRUE);
        }
        else
        {
            Ospfv2DatabaseDescriptionPacketSetO(&(dbDscrpPkt->options), FALSE);
        }
        /***** End: OPAQUE-LSA *****/
        dbDscrpPkt->ddSequenceNumber = nbrInfo->DDSequenceNumber;
        nbrInfo->optionsBits = dbDscrpPkt->ospfDbDp;
        dbDscrpPkt->commonHeader.packetLength = (short)
                (sizeof(Ospfv2DatabaseDescriptionPacket)
                    + numLSAHeader * sizeof(Ospfv2LinkStateHeader));

        memcpy(dbDscrpPkt + 1,
               payloadBuff,
               numLSAHeader * sizeof(Ospfv2LinkStateHeader));

        MEM_free(payloadBuff);

        if (Ospfv2DebugSync(node))
        {
            char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
            fprintf(stdout, "    send DD packet to neighbor %s\n"
                            "        seqno = %u, numLSA = %d intf %d\n",
                            nbrAddrStr,
                            nbrInfo->DDSequenceNumber, numLSAHeader,
                            interfaceIndex);
        }
    }

    // Fill in the header fields for the Database Description packet
    Ospfv2CreateCommonHeader(node,
                             &dbDscrpPkt->commonHeader,
                             OSPFv2_DATABASE_DESCRIPTION);
    dbDscrpPkt->commonHeader.areaID = ospf->iface[interfaceIndex].areaId;

    dbDscrpPkt->interfaceMtu = (unsigned short)
                               GetNetworkIPFragUnit(node, interfaceIndex);

    // Set E-bit if not a stub area
    if (thisArea->externalRoutingCapability == TRUE)
    {
        Ospfv2OptionsSetExt(&(dbDscrpPkt->options), 1);
    }
    //if this interface of router is attached to DC then it
    //will continue setting the DC bit in database description packet
    if ((ospf->supportDC == TRUE)
        &&
        (ospf->iface[interfaceIndex].ospfIfDemand == TRUE)
        &&
        (ospf->iface[interfaceIndex].type == OSPFv2_POINT_TO_POINT_INTERFACE))
    {
        Ospfv2OptionsSetDC(&(dbDscrpPkt->options), 1);
    }
    else
    {
        Ospfv2OptionsSetDC(&(dbDscrpPkt->options), 0);
    }

#ifdef ADDON_BOEINGFCS
    if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
    {
        RoutingCesRospfAdjustDDHeader(node,
                                      interfaceIndex,
                                      nbrAddr,
                                      dbDscrpPkt);
    }
#endif

    // Add to retransmission list
    if (nbrInfo->lastSentDDPkt != NULL)
    {
        MESSAGE_Free(node, nbrInfo->lastSentDDPkt);
    }

    nbrInfo->lastSentDDPkt = MESSAGE_Duplicate(node, dbDscrpMsg);

    // Determine destination address
    if (ospf->iface[interfaceIndex].type ==
            OSPFv2_POINT_TO_POINT_INTERFACE)
    {
        dstAddr = OSPFv2_ALL_SPF_ROUTERS;
    }
    else
    {
        dstAddr = nbrAddr;
    }

    // Now, send packet
    delay = 0;

    if (OSPFv2_DEBUG)
    {
        Ospfv2PrintDatabaseDescriptionPacket(dbDscrpPkt);
    }

    //Trace sending pkt
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, dbDscrpMsg, TRACE_NETWORK_LAYER,
                     PACKET_OUT, &acnData);

    int dbDscrpPktSize = MESSAGE_ReturnPacketSize(dbDscrpMsg);

    NetworkIpSendRawMessageToMacLayerWithDelay(
                            node,
                            dbDscrpMsg,
                            NetworkIpGetInterfaceAddress(node,
                                                 interfaceIndex),
                            dstAddr,
                            IPTOS_PREC_INTERNETCONTROL,
                            IPPROTO_OSPF,
                            1,
                            interfaceIndex,
                            nbrAddr,
                            delay);
//#ifdef CYBER_CORE
//    interfaceIndex = oldInterface;
//#endif // CYBER_CORE
#if COLLECT_DETAILS
    // instrumented code
    numDDBytesSent += MESSAGE_ReturnPacketSize(dbDscrpMsg);

    if (node->getNodeTime() - lastDDPrint >= OSPFv2_PRINT_INTERVAL)
    {
        lastDDPrint = node->getNodeTime();
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("%s Number of DD bytes sent = %ld\n",
               clockStr, numDDBytesSent);
    }
    // end instrumented code
#endif

    nbrInfo->lastDDPktSent = node->getNodeTime();
    ospf->stats.numDDPktSent++;
    ospf->stats.numDDPktBytesSent += dbDscrpPktSize;

    // Update interface based stats
    Ospfv2Interface* thisIntf = NULL;
    thisIntf = &ospf->iface[interfaceIndex];
    if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
    {
        ospf->iface[interfaceIndex].interfaceStats.numDDPktSent++;
        ospf->iface[interfaceIndex].interfaceStats.numDDPktBytesSent +=
            dbDscrpPktSize;
    }

    // Set retransmission timer if this is Master
#ifndef nADDON_BOEINGFCS
    if (nbrInfo->masterSlave == T_Master)
#else
    if (nbrInfo->masterSlave == T_Master
        || ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
#endif
    {

        delay = (clocktype) (ospf->iface[interfaceIndex].rxmtInterval
                + (RANDOM_erand(ospf->seed) * OSPFv2_BROADCAST_JITTER));

        Ospfv2SetTimer(node,
                       interfaceIndex,
                       MSG_ROUTING_OspfRxmtTimer,
                       nbrInfo->neighborIPAddress,
                       delay,
                       dbDscrpPkt->ddSequenceNumber,
                       0,
                       OSPFv2_DATABASE_DESCRIPTION);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendLSRequestPacket()
// PURPOSE      :Send Link State Request. Parameter retx indicates whether we
//               are retransmissing the requests or seding fresh list.
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------//

void Ospfv2SendLSRequestPacket(
    Node* node,
    NodeAddress nbrAddr,
    int interfaceIndex,
    BOOL retx)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Message* reqMsg = NULL;
    Ospfv2LinkStateRequestPacket* reqPkt = NULL;
    Ospfv2Neighbor* nbrInfo = NULL;
    NodeAddress dstAddr;
    clocktype delay;

    Ospfv2LSRequestInfo* payloadBuff = NULL;

    int maxLSReqObject =
                (GetNetworkIPFragUnit(node, interfaceIndex)
                          - sizeof(IpHeaderType)
                             - sizeof(Ospfv2LinkStateRequestPacket))
                                 / sizeof(Ospfv2LSRequestInfo);
    int numLSReqObject;

    // Find neighbor structure
    nbrInfo = Ospfv2GetNeighborInfoByIPAddress(node,
                                               interfaceIndex,
                                               nbrAddr);
#ifdef JNE_LIB
    if (!nbrInfo)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Neighbor not found in the Neighbor list",
            JNE::CRITICAL);
    }
#endif

    ERROR_Assert(nbrInfo != NULL, "Neighbor not found in the Neighbor list");

    payloadBuff = (Ospfv2LSRequestInfo*)
                    MEM_malloc(maxLSReqObject
                    * sizeof(Ospfv2LSRequestInfo));

    numLSReqObject = 0;

    // Prepare LS Request packet payload
    while ((numLSReqObject < maxLSReqObject)
            && Ospfv2CreateLSReqObject(node,
                                       nbrInfo,
                                       &payloadBuff[numLSReqObject],
                                       retx))
    {
        numLSReqObject++;
    }

    if (numLSReqObject == 0)
    {
        ERROR_ReportWarning(
          "Sending EMPTY LS Request object");
    }

    // Create LS Request packet
    reqMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfPacket);

    MESSAGE_PacketAlloc(node,
                        reqMsg,
                        sizeof(Ospfv2LinkStateRequestPacket)
                        + numLSReqObject
                        * sizeof(Ospfv2LSRequestInfo),
                        TRACE_OSPFv2);

    reqPkt = (Ospfv2LinkStateRequestPacket*)
                MESSAGE_ReturnPacket(reqMsg);

    memset(reqPkt, 0, sizeof(Ospfv2LinkStateRequestPacket)
                            + numLSReqObject
                            * sizeof(Ospfv2LSRequestInfo));

    // Fill in the header fields for the LS Request packet
    Ospfv2CreateCommonHeader(node,
                             &reqPkt->commonHeader,
                             OSPFv2_LINK_STATE_REQUEST);

    reqPkt->commonHeader.packetLength = (short)
            (sizeof(Ospfv2LinkStateRequestPacket)
                + numLSReqObject * sizeof(Ospfv2LSRequestInfo));
    reqPkt->commonHeader.areaID = ospf->iface[interfaceIndex].areaId;

    // Add payload
    memcpy(reqPkt + 1,
           payloadBuff,
           numLSReqObject * sizeof(Ospfv2LSRequestInfo));

    // Free payload Buffer
    MEM_free(payloadBuff);

    // Determine destination address
    if (ospf->iface[interfaceIndex].type ==
            OSPFv2_POINT_TO_POINT_INTERFACE)
    {
        dstAddr = OSPFv2_ALL_SPF_ROUTERS;
    }
    else
    {
        dstAddr = nbrAddr;
    }
#ifdef CYBER_CORE
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    if ((ip->iahepEnabled)
        && (ip->interfaceInfo[interfaceIndex]->isVirtualInterface))
    {
        dstAddr = nbrAddr;
    }
#endif // CYBER_CORE

    if (Ospfv2DebugSync(node))
    {
        char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(nbrAddr, nbrAddrStr);
        fprintf(stdout, "    send REQUEST to neighbor %s interface %d\n",
                        nbrAddrStr, interfaceIndex);
    }

    // Set retransmission timer
    Ospfv2SetTimer(node,
                   interfaceIndex,
                   MSG_ROUTING_OspfRxmtTimer,
                   nbrInfo->neighborIPAddress,
                   ospf->iface[interfaceIndex].rxmtInterval,
                   ++nbrInfo->LSReqTimerSequenceNumber,
                   0,
                   OSPFv2_LINK_STATE_REQUEST);

    //Trace sending pkt
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, reqMsg, TRACE_NETWORK_LAYER,
                     PACKET_OUT, &acnData);


    // Now, send packet
    delay = 0;

    int reqPktSize = MESSAGE_ReturnPacketSize(reqMsg);

    NetworkIpSendRawMessageToMacLayerWithDelay(
                            node,
                            reqMsg,
                            NetworkIpGetInterfaceAddress(node,
                                                         interfaceIndex),
                            dstAddr,
                            IPTOS_PREC_INTERNETCONTROL,
                            IPPROTO_OSPF,
                            1,
                            interfaceIndex,
                            nbrAddr,
                            delay);


    ospf->stats.numLSReqSent++;

    // instrumented code
    ospf->stats.numLSReqBytesSent += reqPktSize;

    // Update interface based stats
    Ospfv2Interface* thisIntf = NULL;
    thisIntf = &ospf->iface[interfaceIndex];
    if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
    {
        ospf->iface[interfaceIndex].interfaceStats.numLSReqSent++;
        ospf->iface[interfaceIndex].interfaceStats.numLSReqBytesSent +=
            reqPktSize;
    }

#if COLLECT_DETAILS
    if (node->getNodeTime() - lastLSReqPrint >= OSPFv2_PRINT_INTERVAL)
    {
        lastLSReqPrint = node->getNodeTime();
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("%s Number of LS Request bytes sent = %ld\n",
               clockStr, numLSReqBytesSent);
    }
    // end instrumented code
#endif

}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2ProcessDatabaseDescriptionPacket()
// PURPOSE      :Process rest of the database description packet
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------//

static
void Ospfv2ProcessDatabaseDescriptionPacket(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Area* thisArea = Ospfv2GetArea(
                                node,
                                ospf->iface[interfaceIndex].areaId);
    Ospfv2Neighbor* nbrInfo = NULL;
    Ospfv2DatabaseDescriptionPacket* dbDscrpPkt = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    int size;

    // Find neighbor structure
    nbrInfo = Ospfv2GetNeighborInfoByIPAddress(node,
                                               interfaceIndex,
                                               srcAddr);
#ifdef JNE_LIB
    if (!nbrInfo)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Neighbor not found in the Neighbor list",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(nbrInfo != NULL, "Neighbor not found in the Neighbor list");

    dbDscrpPkt = (Ospfv2DatabaseDescriptionPacket*)
                    MESSAGE_ReturnPacket(msg);

    /***** Start: OPAQUE-LSA *****/
    nbrInfo->dbDescriptionNbrOptions = dbDscrpPkt->options;
    /***** End: OPAQUE-LSA *****/

    LSHeader = (Ospfv2LinkStateHeader*) (dbDscrpPkt + 1);
    size = dbDscrpPkt->commonHeader.packetLength;

    // For each LSA in the packet
    for (size -= sizeof(Ospfv2DatabaseDescriptionPacket); size > 0;
            size -= sizeof(Ospfv2LinkStateHeader))
    {
        Ospfv2LinkStateHeader* oldLSHeader = NULL;

        // Check for the validity of LSA
        if ((LSHeader->linkStateType < OSPFv2_ROUTER
             || LSHeader->linkStateType > OSPFv2_ROUTER_NSSA_EXTERNAL)
            /***** Start: OPAQUE-LSA *****/
            && LSHeader->linkStateType != OSPFv2_AS_SCOPE_OPAQUE
            && LSHeader->linkStateType != OSPFv2_LINK_LOCAL_SCOPE_OPAQUE
        )
            /***** End: OPAQUE-LSA *****/
        {
            if (Ospfv2DebugSync(node))
            {
                fprintf(stdout, "Unknown LS type (%d). Packet discarded\n",
                                 LSHeader->linkStateType);
            }

            // Handle neighbor event : Sequence Number Mismatch
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_SeqNumberMismatch);
            return;
        }

        // Stop processing packet if the neighbor is associated with stub
        // area and the LSA is AS-EXTERNAL-LAS

        if (LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL
            && thisArea && thisArea->externalRoutingCapability == FALSE)
        {
            if (Ospfv2DebugSync(node))
            {
                fprintf(stdout, "Receive AS_EXTERNAL_LSA from stub area. "
                                "Packet discarded\n");
            }

            // Handle neighbor event : Sequence Number Mismatch
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_SeqNumberMismatch);
            return;
        }

        // Stop processing packet if the neighbor is not associated with
        // NSSA area and the LSA is type 7 LSA

        if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL
            && thisArea && thisArea->isNSSAEnabled == FALSE)
        {
            if (Ospfv2DebugSync(node))
            {
                fprintf(stdout, "Receive NSSA_EXTERNAL_LSA from non NSSA."
                                " area.Packet discarded\n");
            }

            // Handle neighbor event : Sequence Number Mismatch
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_SeqNumberMismatch);
            return;
        }

        /***** Start: OPAQUE-LSA *****/
        if (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE
            && thisArea && thisArea->externalRoutingCapability == FALSE)
        {
            if (Ospfv2DebugSync(node))
            {
                fprintf(stdout, "Receive AS_SCOPE_OPAQUE from stub area. "
                                "Packet discarded\n");
            }

            // Handle neighbor event : Sequence Number Mismatch
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_SeqNumberMismatch);
            return;
        }
        if (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE
            && !Ospfv2DatabaseDescriptionPacketGetO(nbrInfo->dbDescriptionNbrOptions))
        {
            if (Ospfv2DebugSync(node))
            {
                fprintf(stdout, "Receive AS_SCOPE_OPAQUE from node that does "
                                "not support them. Packet discarded\n");
            }

            // Handle neighbor event : Sequence Number Mismatch
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_SeqNumberMismatch);
            return;
        }

        if (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE
            && ospf->opaqueCapable == FALSE)
        {
            // Skip LSA as node does not support opaque capability
            LSHeader += 1;
            continue;
        }
        /***** End: OPAQUE-LSA *****/

        if (LSHeader->linkStateType == OSPFv2_LINK_LOCAL_SCOPE_OPAQUE)
        {
            // Skip type 9 opaque LSA that we can't handle
            LSHeader += 1;
            continue;
        }

        // Looks up for the LSA in its own Database
        oldLSHeader = Ospfv2LookupLSDB(node,
                                       LSHeader->linkStateType,
                                       LSHeader->advertisingRouter,
                                       LSHeader->linkStateID,
                                       interfaceIndex);

#ifdef nADDON_BOEINGFCS
        // certain LSHeaders should be left out of request.
        if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE &&
            !RospfIncludeInRequest(node, LSHeader, ospf->routerID))
        {
            LSHeader+=1;
            continue;
        }
#endif

        // Add to request list if the LSA doesn't exist or if
        // this instance is more recent one

        if (!oldLSHeader
            || (Ospfv2CompareLSA(node, LSHeader, oldLSHeader) > 0))
        {
            Ospfv2AddToRequestList(node,
                                   nbrInfo->linkStateRequestList,
                                   LSHeader);
        }
        LSHeader += 1;
    }

    // Save received DD Packet
    if (!nbrInfo->lastReceivedDDPacket)
    {
        nbrInfo->lastReceivedDDPacket = MESSAGE_Duplicate(node, msg);
    }
    else
    {
        MESSAGE_Free(node, nbrInfo->lastReceivedDDPacket);
        nbrInfo->lastReceivedDDPacket = MESSAGE_Duplicate(node, msg);
    }

    // Master
    if (nbrInfo->masterSlave == T_Master)
    {
        nbrInfo->DDSequenceNumber++;
#ifdef ADDON_BOEINGFCS
        nbrInfo->ddReTxCount = 0;
#endif

        // If the router has already sent its entire sequence of DD packets,
        // and the just accepted packet has the more bit (M) set to 0, the
        // neighbor event ExchangeDone is generated.

        if ((!Ospfv2DatabaseDescriptionPacketGetMore(nbrInfo->optionsBits))
            && (!Ospfv2DatabaseDescriptionPacketGetMore(
            dbDscrpPkt->ospfDbDp)))
        {
            Ospfv2ScheduleNeighborEvent(node,
                                        interfaceIndex,
                                        srcAddr,
                                        E_ExchangeDone);
        }
#ifdef ADDON_BOEINGFCS
        else if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE &&
                 RoutingCesRospfDatabaseDigestsEnabled(node, interfaceIndex))
        {
            assert (nbrInfo->dbdgNbr != NULL);
            nbrInfo->dbdgNbr->startDigestExchange();
        }
#endif
        // Else send new DD packet.
        else
        {
            if (Ospfv2DebugSync(node))
            {
                if (Ospfv2DatabaseDescriptionPacketGetMore(
                        dbDscrpPkt->ospfDbDp))
                {
                    printf("Received DD packet has MORE bit set to 1\n");
                }

                if (!Ospfv2ListIsEmpty(nbrInfo->DBSummaryList))
                {
                    Ospfv2PrintDBSummaryList(node, nbrInfo->DBSummaryList);
                }
            }

            Ospfv2SendDatabaseDescriptionPacket(node,
                                                srcAddr,
                                                interfaceIndex);
        }
    }
    // Slave
    else
    {
        nbrInfo->DDSequenceNumber = dbDscrpPkt->ddSequenceNumber;

        // Send DD packet
        Ospfv2SendDatabaseDescriptionPacket(node,
                                            srcAddr,
                                            interfaceIndex);

       if (!(Ospfv2DatabaseDescriptionPacketGetMore(dbDscrpPkt->ospfDbDp))
            && Ospfv2ListIsEmpty(nbrInfo->DBSummaryList))
        {

            Ospfv2ScheduleNeighborEvent(node,
                                        interfaceIndex,
                                        srcAddr,
                                        E_ExchangeDone);
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleDatabaseDescriptionPacket()
// PURPOSE      :Handle database description packet
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2HandleDatabaseDescriptionPacket(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Neighbor* nbrInfo = NULL;
    Ospfv2DatabaseDescriptionPacket* dbDscrpPkt = NULL;
    BOOL isDuplicate = FALSE;

    Ospfv2DatabaseDescriptionPacket* oldPkt = NULL;

    // Find neighbor structure
    nbrInfo = Ospfv2GetNeighborInfoByIPAddress(node,
                                               interfaceIndex,
                                               srcAddr);

    if (!nbrInfo)
    {
        if (Ospfv2DebugSync(node))
        {
            char srcAddrStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(srcAddr, srcAddrStr);
            fprintf(stdout, "    receive DATABASE from unknown neighbor %s "
                            "on interface %d, packet discarded\n",
                             srcAddrStr, interfaceIndex);
        }
        return;
    }

    dbDscrpPkt = (Ospfv2DatabaseDescriptionPacket*)
                    MESSAGE_ReturnPacket(msg);

    // Check if two consecutive DD packets are same
    if (nbrInfo->lastReceivedDDPacket != NULL)
    {
        oldPkt = (Ospfv2DatabaseDescriptionPacket*)
                    MESSAGE_ReturnPacket(nbrInfo->lastReceivedDDPacket);

        if ((oldPkt->ospfDbDp == dbDscrpPkt->ospfDbDp)
            && (oldPkt->ddSequenceNumber == dbDscrpPkt->ddSequenceNumber)
            && (oldPkt->options == dbDscrpPkt->options))
        {
            isDuplicate = TRUE;
        }

        if (ospf->supportDC && Ospfv2OptionsGetDC(dbDscrpPkt->options))
        {
            if (ospf->iface[interfaceIndex].ospfIfDemand == FALSE)
            {
                ospf->iface[interfaceIndex].helloSuppressionSuccess = FALSE ;
            }
            else
            {
                ospf->iface[interfaceIndex].helloSuppressionSuccess = TRUE ;
            }
        }
        else //hello packet has DC bit as 0
        {
            ospf->iface[interfaceIndex].helloSuppressionSuccess = FALSE ;
        }
    }

    if (dbDscrpPkt->interfaceMtu > GetNetworkIPFragUnit(node, interfaceIndex))
    {
        if (OSPFv2_DEBUG_SYNC)
        {
            fprintf(stdout, " Interface MTU field indicates an IP"
                      " datagram size larger than router can accept.\n");
        }
        return;
    }
    if (Ospfv2DebugSync(node))
    {
        char srcAddrStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(srcAddr, srcAddrStr);
        printf("Node %d (routerId = %x) Received DatabaseDescription packet "
            "from srcAddr %s\n"
            "Seq = %d, I = %d, M = %d, MS = %d, nbrSequenceNumber = %d\n",
            node->nodeId,
            ospf->routerID,
            srcAddrStr,
            dbDscrpPkt->ddSequenceNumber,
            Ospfv2DatabaseDescriptionPacketGetInit(dbDscrpPkt->ospfDbDp),
            Ospfv2DatabaseDescriptionPacketGetMore(dbDscrpPkt->ospfDbDp),
            Ospfv2DatabaseDescriptionPacketGetMS(dbDscrpPkt->ospfDbDp),
            nbrInfo->DDSequenceNumber);
    }

    switch (nbrInfo->state)
    {
        // Reject packet if neighbor state is Down or Attempt or 2-Way
        case S_NeighborDown:
        case S_Attempt:
        case S_TwoWay:
        {
            if (Ospfv2DebugSync(node))
            {
                fprintf(stdout, "    neighbor state is below S_ExStart. "
                                  "Packet discarded\n");
            }

            break;
        }

        case S_Init:
        {
#ifdef nADDON_BOEINGFCS
            if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
            {
                if (RoutingCesRospfAdjacencyAccept(node, interfaceIndex,
                    srcAddr, nbrInfo->neighborNodeId))
                {
                   Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_TwoWayReceived);
                }
            }
            else
            {
#endif
            // Handle neighbor event : 2-Way receive
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_TwoWayReceived);
#ifdef nADDON_BOEINGFCS
            }
#endif
            if (nbrInfo->state != S_ExStart)
            {
                break;
            }
            else
            {
                // Otherwise, falls through to case S_ExStart below
            }
        }

        case S_ExStart:
        {
            // Slave
            if ((Ospfv2DatabaseDescriptionPacketGetInit
                (dbDscrpPkt->ospfDbDp))
                && (Ospfv2DatabaseDescriptionPacketGetMore
                (dbDscrpPkt->ospfDbDp))
                && (Ospfv2DatabaseDescriptionPacketGetMS
                (dbDscrpPkt->ospfDbDp))
                && (dbDscrpPkt->commonHeader.packetLength ==
                    sizeof(Ospfv2DatabaseDescriptionPacket))
                && (dbDscrpPkt->commonHeader.routerID > ospf->routerID))
            {
                nbrInfo->masterSlave = T_Slave;
                nbrInfo->DDSequenceNumber = dbDscrpPkt->ddSequenceNumber;

                if (Ospfv2DebugSync(node))
                {
                    printf("    now a Slave\n");
                }
            }
            // Master
            else if ((!(Ospfv2DatabaseDescriptionPacketGetInit
                (dbDscrpPkt->ospfDbDp)))
                && (!(Ospfv2DatabaseDescriptionPacketGetMS
                (dbDscrpPkt->ospfDbDp)))
                && (dbDscrpPkt->ddSequenceNumber ==
                    nbrInfo->DDSequenceNumber)
                && (dbDscrpPkt->commonHeader.routerID < ospf->routerID))
            {
                nbrInfo->masterSlave = T_Master;

                if (Ospfv2DebugSync(node))
                {
                    printf("    now a Master\n");
                }
            }
            // Cannot determine Master or Slave, so discard packet
                else
                {
                    if (Ospfv2DebugSync(node))
                    {
                        fprintf(stdout, "    cannot determine Master or Slave"
                            " from neighbor 0x%x, so discard packet\n",
                                         srcAddr);
                        printf("srcAddr %x init %d masterSlave"
                            "%d ddSequenceNumber %d nbrSequenceNumber"
                            "%d headerRouterId %x ospfRouterId %x\n",
                            srcAddr,
                            Ospfv2DatabaseDescriptionPacketGetInit
                            (dbDscrpPkt->ospfDbDp),
                            Ospfv2DatabaseDescriptionPacketGetMS
                            (dbDscrpPkt->ospfDbDp),
                            dbDscrpPkt->ddSequenceNumber,
                            nbrInfo->DDSequenceNumber,
                            dbDscrpPkt->commonHeader.routerID,
                            ospf->routerID);
                    }

                    break;
            }
            /***** Start: OPAQUE-LSA *****/
            nbrInfo->dbDescriptionNbrOptions = dbDscrpPkt->options;
            /***** End: OPAQUE-LSA *****/

            // Handle neighbor event : Negotiation Done
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_NegotiationDone);

            nbrInfo->neighborOptions = dbDscrpPkt->options;

            // Process rest of the DD packet
            Ospfv2ProcessDatabaseDescriptionPacket(node,
                                                   msg,
                                                   srcAddr,
                                                   interfaceIndex);

            break;
        }

        case S_Exchange:
        {
            // Check if the packet is duplicate
            if (isDuplicate)
            {
                // Master
                if (nbrInfo->masterSlave == T_Master)
                {
                    // Discard packet
                    if (Ospfv2DebugSync(node))
                    {
                        fprintf(stdout, "    duplicate DD packet. Neighbor "
                                "state is T_Master. Packet discarded\n");
                    }
                }

                // Slave
                else
                {
#ifdef JNE_LIB
                    if (!nbrInfo->lastSentDDPkt)
                    {
                        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                            "No packet to retx",
                            JNE::CRITICAL);
                    }
#endif
                    ERROR_Assert(nbrInfo->lastSentDDPkt,
                        "No packet to retx\n");

                    //Trace sending pkt
                    ActionData acnData;
                    acnData.actionType = SEND;
                    acnData.actionComment = NO_COMMENT;
                    TRACE_PrintTrace(node, nbrInfo->lastSentDDPkt,
                        TRACE_NETWORK_LAYER, PACKET_OUT, &acnData);

                    if (Ospfv2DebugSync(node))
                    {
                        fprintf(stdout, "    duplicate DD packet."
                            " Neighbor state is T_Slave. Retransmit "
                            "previous packet\n");
                    }

                    // This packet needs to be retransmitted,
                    // so send it out again.
                    NetworkIpSendRawMessageToMacLayer(
                        node,
                        MESSAGE_Duplicate(node, nbrInfo->lastSentDDPkt),
                        NetworkIpGetInterfaceAddress(node, interfaceIndex),
                        nbrInfo->neighborIPAddress,
                        IPTOS_PREC_INTERNETCONTROL,
                        IPPROTO_OSPF,
                        1,
                        interfaceIndex,
                        nbrInfo->neighborIPAddress);

#ifdef ADDON_BOEINGFCS
                    nbrInfo->ddReTxCount++;
#endif

#if COLLECT_DETAILS
                    // instrumented code
                    numRetxDDBytesSent += MESSAGE_ReturnPacketSize
                        (nbrInfo->lastSentDDPkt);

                    if (node->getNodeTime() - lastRetxDDPrint >=
                        OSPFv2_PRINT_INTERVAL)
                    {
                        lastRetxDDPrint = node->getNodeTime();
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                        printf("%s Number of ReTx DD bytes sent = %ld\n",
                               clockStr, numRetxDDBytesSent);
                    }
                    // end instrumented code
#endif

                }

                break;
            }

            // Check for Master-Slave mismatch
            if (((nbrInfo->masterSlave == T_Master)
                && (Ospfv2DatabaseDescriptionPacketGetMS(
                    dbDscrpPkt->ospfDbDp)))
                || ((nbrInfo->masterSlave == T_Slave)
                && (!(Ospfv2DatabaseDescriptionPacketGetMS(
                        dbDscrpPkt->ospfDbDp)))))
            {
                // Handle neighbor event : Sequence Number Mismatch
                Ospfv2HandleNeighborEvent(node,
                                          interfaceIndex,
                                          srcAddr,
                                          E_SeqNumberMismatch);

                // Stop processing packet
                break;
            }

            // Check if initialization bit is set
            if ((Ospfv2DatabaseDescriptionPacketGetInit(
                    dbDscrpPkt->ospfDbDp)))
            {
                // Handle neighbor event : Sequence Number Mismatch
                Ospfv2HandleNeighborEvent(node,
                                          interfaceIndex,
                                          srcAddr,
                                          E_SeqNumberMismatch);

                // Stop processing packet
                break;
            }

            // If neighbor option have been changed.
            if ((oldPkt) && (dbDscrpPkt->options != oldPkt->options))
            {
                // Handle neighbor event : Sequence Number Mismatch
                Ospfv2HandleNeighborEvent(node,
                                          interfaceIndex,
                                          srcAddr,
                                          E_SeqNumberMismatch);

                // Stop processing packet
                break;
            }

            // Check DD Sequence number
            if (((nbrInfo->masterSlave == T_Master)
                && (dbDscrpPkt->ddSequenceNumber !=
                    nbrInfo->DDSequenceNumber))
                || ((nbrInfo->masterSlave == T_Slave)
                && (dbDscrpPkt->ddSequenceNumber !=
                    nbrInfo->DDSequenceNumber + 1)))
            {
                // Handle neighbor event : Sequence Number Mismatch
                Ospfv2HandleNeighborEvent(node,
                                          interfaceIndex,
                                          srcAddr,
                                          E_SeqNumberMismatch);

                // Stop processing packet
                break;
            }

            // Process rest of the packet
            Ospfv2ProcessDatabaseDescriptionPacket(node,
                                                   msg,
                                                   srcAddr,
                                                   interfaceIndex);

            break;
        }

        case S_Loading:
        case S_Full:
        {
            if (isDuplicate)
            {
                // Master discards duplicate packet
                if (nbrInfo->masterSlave == T_Master)
                {
                    if (Ospfv2DebugSync(node))
                    {
                        fprintf(stdout, "    duplicate DD packet. Neighbor "
                                "state is T_Master. Packet discarded\n");
                    }

                    // Discard packet
                    break;
                }
                else
                {
                    // In states Loading and Full the slave must resend its
                    // last Database Description packet in response to
                    // duplicate Database Description packets received from
                    // the master.For this reason the slave must wait
                    // RouterDeadInterval seconds before freeing the last
                    // Database Description packet. Reception of a Database
                    // Description packet from the master after this interval
                    // will generate a SeqNumberMismatch neighbor event.

                    if (node->getNodeTime() - nbrInfo->lastDDPktSent <
                        ospf->iface[interfaceIndex].routerDeadInterval)
                    {
                        if (Ospfv2DebugSync(node))
                        {
                            fprintf(stdout, "    duplicate DD packet."
                                " Neighbor state is T_Slave. Retransmit "
                                "previous packet\n");
                        }

#ifdef JNE_LIB
                        if (!nbrInfo->lastSentDDPkt)
                        {
                            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                                "No packet to retx",
                                JNE::CRITICAL);
                        }
#endif
                        ERROR_Assert(nbrInfo->lastSentDDPkt,
                            "No packet to retx\n");


                        //Trace sending pkt
                        ActionData acnData;
                        acnData.actionType = SEND;
                        acnData.actionComment = NO_COMMENT;
                        TRACE_PrintTrace(node, nbrInfo->lastSentDDPkt,
                            TRACE_NETWORK_LAYER, PACKET_OUT, &acnData);

                        // Retransmit packet
                        NetworkIpSendRawMessageToMacLayer(
                            node,
                            MESSAGE_Duplicate(node, nbrInfo->lastSentDDPkt),
                            NetworkIpGetInterfaceAddress(node,
                                                         interfaceIndex),
                            nbrInfo->neighborIPAddress,
                            IPTOS_PREC_INTERNETCONTROL,
                            IPPROTO_OSPF,
                            1,
                            interfaceIndex,
                            nbrInfo->neighborIPAddress);

#ifdef ADDON_BOEINGFCS
                      nbrInfo->ddReTxCount++;
#endif

#if COLLECT_DETAILS
                        // instrumented code
                        numRetxDDBytesSent += MESSAGE_ReturnPacketSize
                            (nbrInfo->lastSentDDPkt);

                        if (node->getNodeTime() - lastRetxDDPrint >=
                            OSPFv2_PRINT_INTERVAL)
                        {
                            lastRetxDDPrint = node->getNodeTime();
                            char clockStr[MAX_STRING_LENGTH];
                            TIME_PrintClockInSecond(node->getNodeTime(),
                                                    clockStr);
                            printf("%s Number of ReTx DD bytes sent = %ld\n",
                                   clockStr, numRetxDDBytesSent);
                        }
                        // end instrumented code
#endif

                        break;
                    }
                }
            }

            // Handle neighbor event : Sequence Number Mismatch
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_SeqNumberMismatch);
            break;
        }

        default:
        {
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Unknown neighbor state",
                JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, "Unknown neighbor state\n");
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2QueueLSAToFlood()
// PURPOSE      :Add LSA to update LSA list for flooding
// ASSUMPTION   :None.
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2QueueLSAToFlood(
    Node* node,
    Ospfv2Interface* thisInterface,
    char* LSA,
    BOOL setDoNotAge
    )
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    char* newLSA = NULL;
    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;
    Ospfv2ListItem* listItem = thisInterface->updateLSAList->first;

    while (listItem)
    {
        Ospfv2LinkStateHeader* listLSHeader =
            (Ospfv2LinkStateHeader*) listItem->data;

        // If this LSA present in list
#ifdef ADDON_MA
        BOOL  isMatched;
         if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL ||
            LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)&&
            (listLSHeader->linkStateType == LSHeader->linkStateType))
        {
            /*
            Ospfv2ExternalLinkInfo* linkInfo;
            Ospfv2ExternalLinkInfo* listInfo;
            linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
            listInfo = (Ospfv2ExternalLinkInfo*) (listLSHeader + 1);

            if (linkInfo->networkMask == listInfo->networkMask)
            */
            if (LSHeader->mask == listLSHeader->mask)
            {
                isMatched = TRUE;
            }else
            {
                isMatched = FALSE;
            }
        }else
        {
           isMatched = TRUE;
        }

        if (LSHeader->linkStateType == listLSHeader->linkStateType &&
            LSHeader->linkStateID == listLSHeader->linkStateID &&
            LSHeader->advertisingRouter == listLSHeader->advertisingRouter &&
            isMatched)
#else
        if (LSHeader->linkStateType == listLSHeader->linkStateType &&
            LSHeader->linkStateID == listLSHeader->linkStateID &&
            LSHeader->advertisingRouter == listLSHeader->advertisingRouter)
#endif
        {
            // If this is newer instance add this instance replacing old one
            if (Ospfv2CompareLSA(node, LSHeader, listLSHeader) > 0)
            {
                MEM_free(listItem->data);
                listItem->data = (void*) Ospfv2CopyLSA(LSA);
                listItem->timeStamp = node->getNodeTime();

                LSHeader = (Ospfv2LinkStateHeader*) listItem->data;

                if (setDoNotAge == TRUE)
                {
                    Ospfv2LinkStateHeaderSetDoNotAge(LSHeader->linkStateAge);
                }
            }

            return;
        }

        listItem = listItem->next;
    }

    newLSA = Ospfv2CopyLSA(LSA);

    LSHeader = (Ospfv2LinkStateHeader*) newLSA;

    //Although is LSA already contains the DoNotAge set then it does not
    //matter whether setDoNotAge is set or not so where it is not required
    //to set a DoNotAge
    //explicitly, 0 may be passed for setDoNotAge parameter
    if (setDoNotAge == TRUE)
    {
        Ospfv2LinkStateHeaderSetDoNotAge(LSHeader->linkStateAge);
        ospf->stats.numDoNotAgeLSASent++;
    }

    // Add to update LSA list.
    Ospfv2InsertToList(thisInterface->updateLSAList,
                       node->getNodeTime(),
                       (void*) newLSA);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleLSRequestPacket()
// PURPOSE      :Handle Link State Request packet
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------//
static
void Ospfv2HandleLSRequestPacket(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Interface* thisInterface = &ospf->iface[interfaceIndex];
    Ospfv2LinkStateRequestPacket* reqPkt = NULL;
    int numLSReqObject;
    Ospfv2LSRequestInfo* LSReqObject = NULL;
    int i;

    BOOL setDoNotAge = FALSE;

    // Find neighbor structure

    Ospfv2Neighbor* nbrInfo =
        Ospfv2GetNeighborInfoByIPAddress(node, interfaceIndex, srcAddr);

    if (!nbrInfo)
    {
        if (Ospfv2DebugSync(node))
        {
            char srcStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(srcAddr,srcStr);

            fprintf(stdout, "Node %u receive LS Request from unknown"
                " neighbor %s. Discard LS Request packet\n",
                             node->nodeId, srcStr);
        }

        return;
    }

    // Neighbor state should be exchange or later
    if (nbrInfo->state < S_Exchange)
    {
        if (Ospfv2DebugSync(node))
        {
            char srcStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(srcAddr,srcStr);

            fprintf(stdout, "Node %u : neighbor (%s) state is below"
                " Exchange. Discard LS Request packet\n",
                               node->nodeId, srcStr);
        }

        return;
    }

    reqPkt = (Ospfv2LinkStateRequestPacket*)
                     MESSAGE_ReturnPacket(msg);

    numLSReqObject = (reqPkt->commonHeader.packetLength
                         - sizeof(Ospfv2LinkStateRequestPacket))
                             / sizeof(Ospfv2LSRequestInfo);

    LSReqObject = (Ospfv2LSRequestInfo*) (reqPkt + 1);

    for (i = 0; i < numLSReqObject; i++)
    {
        Ospfv2LinkStateHeader* LSHeader = NULL;

        // Stop processing packet if requested LSA type is not identified
        if ((LSReqObject->linkStateType < OSPFv2_ROUTER
            || LSReqObject->linkStateType > OSPFv2_ROUTER_NSSA_EXTERNAL)
           /***** Start: OPAQUE-LSA *****/
           && LSReqObject->linkStateType != OSPFv2_AS_SCOPE_OPAQUE)
           /***** End: OPAQUE-LSA *****/
        {
            if (Ospfv2DebugSync(node))
            {
                char srcStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(srcAddr,srcStr);

                fprintf(stdout, "Node %u : Receive bad LS Request from"
                    " neighbor (%s). Discard LS Request packet\n",
                                 node->nodeId, srcStr);
            }

            // Handle neighbor event : Bad LS Request
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_BadLSReq);
            return;
        }

        // Find LSA in my own LSDB
        LSHeader = Ospfv2LookupLSDB(node,
                                    (char) LSReqObject->linkStateType,
                                    LSReqObject->advertisingRouter,
                                    LSReqObject->linkStateID,
                                    interfaceIndex);

        // Stop processing packet if LSA is not found in my database
        if (LSHeader == NULL)
        {
            if (Ospfv2DebugSync(node))
            {
                char srcStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(srcAddr,srcStr);

                fprintf(stdout, "Node %u : Receive bad LS Request from"
                    " neighbor (%s). LSA not in LSDB."
                    " Discard LS Request packet\n",
                                 node->nodeId, srcStr);

                char idStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(LSReqObject->advertisingRouter ,
                    srcStr);
                IO_ConvertIpAddressToString(LSReqObject->linkStateID ,idStr);

                printf("Object being requested:\n"
                       "LS Type = %d\n"
                       "LS Advertising Router = %s\n"
                       "LS LinkState ID = %s\n",
                       LSReqObject->linkStateType,
                       srcStr,
                       idStr);
            }

            // Handle neighbor event : Bad LS Request
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      srcAddr,
                                      E_BadLSReq);
            return;
        }

        if (Ospfv2DebugSync(node))
        {
            printf("Node %d: Process REQUEST for LSHeader:\n", node->nodeId);
            Ospfv2PrintLSHeader(LSHeader);
        }

        //// Add LSA to send this neighbor
        //RFC:1793::Section:3.3. Flooding over demand circuits
        //The previous paragraph also pertains to LSAs flooded over
        //demand circuits in response to Link State Requests.

        //Set DoNotAge For this LSA if age is not MaxAge
        setDoNotAge = FALSE;
        if ((ospf->supportDC == TRUE) &&
            (ospf->iface[interfaceIndex].ospfIfDemand == TRUE))
        {
            if (!Ospfv2SearchRouterDBforDC(ospf,
                                           Ospfv2GetArea(node,
                                         ospf->iface[interfaceIndex].areaId),
                                           0))     //all LSAs have DC bit 1
            {
                if (!Ospfv2LSAHasMaxAge(ospf, (char*)LSHeader))
                {
                    setDoNotAge = TRUE;
                }
            }
        }
        Ospfv2QueueLSAToFlood(node,
                              thisInterface,
                              (char*) LSHeader ,
                               setDoNotAge
                               );
        LSReqObject += 1;

        if (OSPFv2_SEND_DELAYED_UPDATE)
        {

            if (!thisInterface->floodTimerSet)
            {
                Message* floodMsg = NULL;
                clocktype delay;

                thisInterface->floodTimerSet = TRUE;

                // Set Timer
                floodMsg = MESSAGE_Alloc(node,
                                         NETWORK_LAYER,
                                         ROUTING_PROTOCOL_OSPFv2,
                                         MSG_ROUTING_OspfFloodTimer);

#ifdef ADDON_NGCNMS
                MESSAGE_SetInstanceId(floodMsg, interfaceIndex);
#endif

                MESSAGE_InfoAlloc(node, floodMsg, sizeof(int));
                memcpy(MESSAGE_ReturnInfo(floodMsg),
                       &interfaceIndex,
                       sizeof(int));
#ifdef ADDON_BOEINGFCS
                memcpy(MESSAGE_ReturnInfo(floodMsg)+sizeof(int),
                       &nbrInfo->neighborIPAddress,
                       sizeof(NodeAddress));
#endif
                delay = (clocktype)
                            (RANDOM_erand(ospf->seed) * ospf->floodTimer);
                MESSAGE_Send(node, floodMsg, delay);
            }
        }
    }

    if (!OSPFv2_SEND_DELAYED_UPDATE)
    {
        Ospfv2SendLSUpdateToNeighbor(node, nbrInfo->neighborIPAddress,
            interfaceIndex);
    }
}


//-------------------------------------------------------------------------//
//                          FLOODING MECHANISM                             //
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// NAME         :Ospfv2AddToLSRetxList()
// PURPOSE      :Add LSA to retransmission list.
// ASSUMPTION   :None.
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2AddToLSRetxList(
    Node* node,
    int interfaceIndex,
    Ospfv2Neighbor* nbrInfo,
    char* LSA,
    int* referenceCount)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

#ifdef ADDON_NGCNMS
    // If over 50  remove something from the end
    if (nbrInfo->linkStateRetxList->size >= OSPFv2_MAX_RETX_LIST_SIZE)
    {
        Ospfv2ListItem* retxItem;
        retxItem = nbrInfo->linkStateRetxList->first;

        while (retxItem->next != NULL)
        {
            retxItem = retxItem->next;
        }

        Ospfv2RemoveFromList(node,
                             nbrInfo->linkStateRetxList,
                             retxItem,
                             FALSE);

        Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*)
                                                              retxItem->data;

        Ospfv2LinkStateHeader* copyLSA = (Ospfv2LinkStateHeader*)
                                               Ospfv2CopyLSA(LSHeader);

        if (Ospfv2LSAHasMaxAge(ospf, (char*)copyLSA))
        {
            /* We need not loop through in this case as we are passing the
             * areaId of the interface on which this neighbor exists. Hence
             * this areaId will not be Invalid.*/
            Ospfv2AddLsaToMaxAgeLsaList(node,
                                        (char*) copyLSA,
                                        ospf->iface[interfaceIndex].areaId);
        }
        MEM_free(copyLSA);
    }
#endif

    // If not using reference count make a copy
    if (referenceCount == NULL)
    {
        LSA = Ospfv2CopyLSA(LSA);
    }

    Ospfv2LinkStateHeader* newLSHeader = (Ospfv2LinkStateHeader*) LSA;

    // Add to link state retransmission list.
    Ospfv2InsertToListReferenceCount(nbrInfo->linkStateRetxList,
                                     node->getNodeTime(),
                                     (void*) LSA,
                                     referenceCount);

    Ospfv2InsertToNeighborSendList(nbrInfo->linkStateSendList,
                                   node->getNodeTime(),
                                   newLSHeader);

    if (Ospfv2DebugFlood(node))
    {
        char neighborIPStr[MAX_ADDRESS_STRING_LENGTH];
        char linkStateIDStr[MAX_ADDRESS_STRING_LENGTH];
        char advertisingRouterStr[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress,
                                    neighborIPStr);

        printf("    Node %u: Add LSA with seqno 0x%x to rext"
            " list of nbr %s\n", node->nodeId,
                newLSHeader->linkStateSequenceNumber,
                neighborIPStr);

        IO_ConvertIpAddressToString(newLSHeader->linkStateID,
                                    linkStateIDStr);
        IO_ConvertIpAddressToString(newLSHeader->advertisingRouter,
                                    advertisingRouterStr);

        printf("        Type = %d, ID = %s, advRtr = %s age = %d\n",
                newLSHeader->linkStateType,
                linkStateIDStr,
                advertisingRouterStr,
                newLSHeader->linkStateAge);
    }

    // Set timer for possible retransmission.
    if (nbrInfo->LSRetxTimer == FALSE)
    {
        nbrInfo->rxmtTimerSequenceNumber++;
        Ospfv2SetTimer(node,
                       interfaceIndex,
                       MSG_ROUTING_OspfRxmtTimer,
                       nbrInfo->neighborIPAddress,
                       ospf->iface[interfaceIndex].rxmtInterval,
                       nbrInfo->rxmtTimerSequenceNumber,
                       newLSHeader->advertisingRouter,
                       OSPFv2_LINK_STATE_UPDATE);

        nbrInfo->LSRetxTimer = TRUE;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2RemoveLSAFromRetxList
// PURPOSE      :Removes all LSA that corresponds to the specified
//               LSA from the retransmission list of all neighbors.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2RemoveLSAFromRetxList(
    Node* node,
    char linkStateType,
    NodeAddress linkStateID,
    NodeAddress advertisingRouter,
#ifdef ADDON_MA
    NodeAddress mask,
#endif
    unsigned int areaId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2ListItem* tempListItem = NULL;
    Ospfv2ListItem* retxItem = NULL;
    Ospfv2Neighbor* nbrInfo = NULL;

    Ospfv2LinkStateHeader* LSHeader = NULL;

    // Look at each of my attached interface.
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            continue;
        }

        // Skip the interface if it doesn't belong to specified area
        if (ospf->iface[i].areaId != areaId)
        {
            continue;
        }

        tempListItem = ospf->iface[i].neighborList->first;

        // Get the neighbors for each interface.
        while (tempListItem)
        {
            nbrInfo = (Ospfv2Neighbor*) tempListItem->data;
#ifdef JNE_LIB
            if (!nbrInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Neighbor not found in the Neighbor list",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(nbrInfo,
                "Neighbor not found in the Neighbor list\n");

            retxItem = nbrInfo->linkStateRetxList->first;

            // Look for LSA in retransmission list of each neighbor.
            while (retxItem)
            {
                LSHeader = (Ospfv2LinkStateHeader*) retxItem->data;
#ifdef ADDON_MA
                BOOL isMatched;
                 if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL ||
                    LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)&&
                    (linkStateType == LSHeader->linkStateType))
                {
                    /*
                    Ospfv2ExternalLinkInfo* linkInfo;
                    linkInfo = (Ospfv2ExternalLinkInfo*) (Header + 1);
                    Ospfv2ExternalLinkInfo* listInfo;
                    listInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);

                    if (linkInfo->networkMask == listInfo->networkMask)
                    */
                    if (LSHeader->mask == mask)
                    {
                        isMatched = TRUE;
                    }
                    else
                    {
                        isMatched = FALSE;
                    }
                }else
                {
                    isMatched = TRUE;
                }

                if ((LSHeader->linkStateType == linkStateType)
                        && (LSHeader->linkStateID == linkStateID)
                        && (LSHeader->advertisingRouter == advertisingRouter)
                        && (isMatched))

#else
                // If LSA exists in retransmission list, remove it.
                if ((LSHeader->linkStateType == linkStateType)
                    && (LSHeader->linkStateID == linkStateID)
                    && (LSHeader->advertisingRouter == advertisingRouter))
#endif
                {
                    Ospfv2ListItem* deleteItem = retxItem;
                    retxItem = retxItem->next;

                    if (Ospfv2DebugFlood(node))
                    {
                        char neighborIPAddressStr[MAX_ADDRESS_STRING_LENGTH];
                        char linkStateIDStr[MAX_ADDRESS_STRING_LENGTH];
                        char advertisingRouterStr[MAX_ADDRESS_STRING_LENGTH];

                        IO_ConvertIpAddressToString(
                            nbrInfo->neighborIPAddress,
                            neighborIPAddressStr);

                        printf("    Node %u: Removing LSA with seqno 0x%x"
                            " from rext list of nbr %s\n",
                                node->nodeId,
                                LSHeader->linkStateSequenceNumber,
                                neighborIPAddressStr);

                        IO_ConvertIpAddressToString(LSHeader->linkStateID,
                                                    linkStateIDStr);
                        IO_ConvertIpAddressToString(
                                LSHeader->advertisingRouter,
                                advertisingRouterStr);

                        printf("    Type = %d, ID = %s,"
                               " advRtr = %s age = %d\n",
                               LSHeader->linkStateType,
                               linkStateIDStr,
                               advertisingRouterStr,
                               LSHeader->linkStateAge);
                    }

                    // Remove acknowledged LSA.
                    Ospfv2RemoveFromList(node,
                                         nbrInfo->linkStateRetxList,
                                         deleteItem,
                                         FALSE);
                }
                else
                {
                    retxItem = retxItem->next;
                }
            }

            if ((Ospfv2ListIsEmpty(nbrInfo->linkStateRetxList))
                && (nbrInfo->LSRetxTimer == TRUE))
            {
                nbrInfo->LSRetxTimer = FALSE;
                nbrInfo->rxmtTimerSequenceNumber++;
            }

            tempListItem = tempListItem->next;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2RetransmitLSA()
// PURPOSE      :Retransmit LSAs
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2RetransmitLSA(
    Node* node,
    Ospfv2Neighbor* nbrInfo,
    int interfaceId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2ListItem* listItem = NULL;
    Ospfv2ListItem* sendListItem = NULL;

    Ospfv2LinkStateHeader* LSHeader = NULL;

    char* payloadBuff = NULL;
    int maxPayloadSize;
    int payloadSize;
    int count;

    if (Ospfv2ListIsEmpty(nbrInfo->linkStateRetxList))
    {
        nbrInfo->LSRetxTimer = FALSE;
        nbrInfo->rxmtTimerSequenceNumber++;
        return;
    }

    // Determine maximum payload size of update packet
    maxPayloadSize =
        (GetNetworkIPFragUnit(node, interfaceId)
         - sizeof(IpHeaderType)
         - sizeof(Ospfv2LinkStateUpdatePacket));

    payloadBuff = (char*) MEM_malloc(maxPayloadSize);

    payloadSize = 0;
    count = 0;

    listItem = nbrInfo->linkStateRetxList->first;

    // Prepare LS Update packet payload
    while (listItem)
    {
#ifdef ADDON_BOEINGFCS
        if (ospf->maxNumRetx > 0)
        {
            if (listItem->numReferences > ospf->maxNumRetx)
            {
                if (Ospfv2DebugSync(node))
                {
                    char neighborIPStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress,
                                                neighborIPStr);

                    printf("Node %d Destroying Adjacency to nbr %s due to"
                           " too many Retransmissions\n",
                           node->nodeId, neighborIPStr);
                }

                Ospfv2DestroyAdjacency(node,
                                       nbrInfo,
                                       interfaceId);
                Ospfv2SetNeighborState(node,
                                       interfaceId,
                                       nbrInfo,
                                       S_Init);
                return;
            }

            listItem->numReferences++;
        }
#endif

        // Check if this LSA have been in the list for at least rxmtInterval
        if ((node->getNodeTime()-listItem->timeStamp) <
            ospf->iface[interfaceId].rxmtInterval)
        {
            listItem = listItem->next;
            continue;
        }

        LSHeader = (Ospfv2LinkStateHeader*) listItem->data;

        if (payloadSize + LSHeader->length > maxPayloadSize)
        {
            break;
        }

        sendListItem = Ospfv2SearchSendList(nbrInfo->linkStateSendList,
                                            LSHeader);

        if (sendListItem)
        {
            sendListItem->timeStamp = node->getNodeTime();
        }
        else
        {
            Ospfv2InsertToNeighborSendList(nbrInfo->linkStateSendList,
                                           node->getNodeTime(),
                                           LSHeader);
        }
        listItem->timeStamp = node->getNodeTime();
        // Incrememnt LS age transmission delay (in seconds).
        LSHeader->linkStateAge = (short int) (LSHeader->linkStateAge +
             ((ospf->iface[interfaceId].infTransDelay
            / SECOND)));

        // LS age has a maximum age limit.
        if (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) >
           (OSPFv2_LSA_MAX_AGE / SECOND))
        {
            Ospfv2AssignNewAgeToLSAAge(ospf,
                                       LSHeader->linkStateAge,
                                       (OSPFv2_LSA_MAX_AGE / SECOND));
        }

        if (Ospfv2DebugFlood(node))
        {
            char clockStr[100];
            TIME_PrintClockInSecond(node->getNodeTime() + getSimStartTime(node),
                    clockStr);

            char neighborIPStr[MAX_ADDRESS_STRING_LENGTH];
            char linkStateIDStr[MAX_ADDRESS_STRING_LENGTH];
            char advertisingRouterStr[MAX_ADDRESS_STRING_LENGTH];

            IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress,
                                        neighborIPStr);
            IO_ConvertIpAddressToString(LSHeader->linkStateID,
                                        linkStateIDStr);
            IO_ConvertIpAddressToString(LSHeader->advertisingRouter,
                                        advertisingRouterStr);

            printf("    Node %u Retransmitting LSA to node %s at %s\n"
                   "    Type = %d, ID = %s, AdvRtr = %s, SeqNo = 0x%x\n",
                   node->nodeId, neighborIPStr, clockStr,
                   LSHeader->linkStateType,
                   linkStateIDStr,
                   advertisingRouterStr,
                   LSHeader->linkStateSequenceNumber);
        }

        //RFC:1793::Section:3.3. Flooding over demand circuits
        //It also pertains to LSAs that are retransmitted over demand
        //circuits.
        //RFC:1793::Section 3.2.1
        //Also, even if the negotiation to suppress Hellos fails, the
        //flooding modifications described in Section 3.3 are still
        //performed over the link.

        //RFC:1793::Section:3.3. Flooding over demand circuits
        //(2) When it has been decided to flood an LSA over a demand
        //circuit, DoNotAge should be set in the copy of the LSA that
        //is flooded out the demand interface. (There is one
        //exception: DoNotAge should not be set if the LSA’s LS age is
        //equal to MaxAge.)

        //Set DoNotAge For this LSA if age is not MaxAge
        if ((ospf->supportDC == TRUE) &&
            (ospf->iface[interfaceId].ospfIfDemand == TRUE))
        {
            if (!Ospfv2SearchRouterDBforDC(ospf,
                                          Ospfv2GetArea(node,
                                        ospf->iface[interfaceId].areaId),
                                          0))     //all LSAs have DC bit 1
            {
                if (!Ospfv2LSAHasMaxAge(ospf, (char*)LSHeader))
                {
                    //not incrementing LS Age as it gets incremented in
                    //the above portion of the code

                    Ospfv2LinkStateHeaderSetDoNotAge(LSHeader->linkStateAge);
                    ospf->stats.numDoNotAgeLSASent++;
                }
            }
        }

        memcpy(&payloadBuff[payloadSize], listItem->data, LSHeader->length);
        payloadSize += LSHeader->length;
        count++;

        listItem = listItem->next;
    }

#if COLLECT_DETAILS
    // instrumented code
    numRetxLSUpdateBytes += payloadSize;
    if (node->getNodeTime() - lastRetxLSUpdatePrint >= OSPFv2_PRINT_INTERVAL)
    {
        lastRetxLSUpdatePrint = node->getNodeTime();
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("%s Number of ReTx LS Update bytes sent = %ld\n",
               clockStr, numRetxLSUpdateBytes);
    }
#endif

    if (count > 0)
    {
        Ospfv2SendUpdatePacket(node,
                               nbrInfo->neighborIPAddress,
                               interfaceId,
                               payloadBuff,
                               payloadSize,
                               count);
        ospf->stats.numLSUpdateRxmt++;
    }
    nbrInfo->rxmtTimerSequenceNumber++;

    // If LSA stil exist in retx list, start timer again
    if (!Ospfv2ListIsEmpty(nbrInfo->linkStateRetxList))
    {
        Ospfv2SetTimer(node,
                       interfaceId,
                       MSG_ROUTING_OspfRxmtTimer,
                       nbrInfo->neighborIPAddress,
                       ospf->iface[interfaceId].rxmtInterval,
                       nbrInfo->rxmtTimerSequenceNumber,
                       0,
                       OSPFv2_LINK_STATE_UPDATE);

        nbrInfo->LSRetxTimer = TRUE;
    }
    else
    {
        nbrInfo->LSRetxTimer = FALSE;
    }

    MEM_free(payloadBuff);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendLSUpdate()
// PURPOSE      :Send Updated LSA to neighbor
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2SendLSUpdate(Node* node, NodeAddress nbrAddr, int interfaceId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Interface* thisInterface = &ospf->iface[interfaceId];

    Ospfv2ListItem* listItem = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    NodeAddress dstAddr;

    char* payloadBuff = NULL;
    int maxPayloadSize;
    int payloadSize;
    int count;

    if (Ospfv2ListIsEmpty(thisInterface->updateLSAList))
    {
        return;
    }

    // Determine Destination Address
#ifdef ADDON_BOEINGFCS
    if (thisInterface->type == OSPFv2_ROSPF_INTERFACE)
    {
        dstAddr = nbrAddr;
    }
    else
#endif
    if ((thisInterface->state == S_DR) || (thisInterface->state == S_Backup))
    {
        dstAddr = OSPFv2_ALL_SPF_ROUTERS;
    }
    else if (thisInterface->type == OSPFv2_BROADCAST_INTERFACE)
    {
        dstAddr = OSPFv2_ALL_D_ROUTERS;
    }
    else if (thisInterface->type == OSPFv2_POINT_TO_POINT_INTERFACE)
    {
        dstAddr = OSPFv2_ALL_SPF_ROUTERS;
    }
    else
    {
        dstAddr = nbrAddr;
    }

    // Determine maximum payload size of update packet
    maxPayloadSize =
        (GetNetworkIPFragUnit(node, interfaceId)
         - sizeof(IpHeaderType)
         - sizeof(Ospfv2LinkStateUpdatePacket));

    payloadBuff = (char*) MEM_malloc(maxPayloadSize);

    payloadSize = 0;
    count = 0;

    // Prepare LS Update packet payload
    while (!Ospfv2ListIsEmpty(thisInterface->updateLSAList))
    {
        listItem = thisInterface->updateLSAList->first;
        LSHeader = (Ospfv2LinkStateHeader*) listItem->data;

        if (payloadSize == 0 && LSHeader->length > maxPayloadSize)
        {
            MEM_free(payloadBuff);
            payloadBuff = (char*) MEM_malloc(LSHeader->length);
        }
        else if (payloadSize + LSHeader->length > maxPayloadSize)
        {
            Ospfv2SendUpdatePacket(node,
                                   dstAddr,
                                   interfaceId,
                                   payloadBuff,
                                   payloadSize,
                                   count);

            // Reset variables
            payloadSize = 0;
            count = 0;
            continue;
       }

       // Incrememnt LS age transmission delay (in seconds).
        LSHeader->linkStateAge = (short int) (LSHeader->linkStateAge +
                ((thisInterface->infTransDelay / SECOND)));

        // LS age has a maximum age limit.
        if (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) >
            (OSPFv2_LSA_MAX_AGE / SECOND))
        {
            Ospfv2AssignNewAgeToLSAAge(ospf,
                                       LSHeader->linkStateAge,
                                       (OSPFv2_LSA_MAX_AGE / SECOND));
        }

        memcpy(&payloadBuff[payloadSize], listItem->data, LSHeader->length);
        payloadSize += LSHeader->length;
        count++;

        // Remove item from list
        Ospfv2RemoveFromList(node,
                             thisInterface->updateLSAList,
                             listItem,
                             FALSE);
    }

    if (count > 0)
    {
        Ospfv2SendUpdatePacket(node,
                               dstAddr,
                               interfaceId,
                               payloadBuff,
                               payloadSize,
                               count);
    }

    MEM_free(payloadBuff);
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendLSUpdateToNeighbor()
// PURPOSE      :Send Updated LSA to neighbor
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2SendLSUpdateToNeighbor(Node* node,
                                  NodeAddress nbrAddr,
                                  int interfaceId,
                                  BOOL deleteLSAsFromUpdateList)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Interface* thisInterface = &ospf->iface[interfaceId];

    Ospfv2ListItem* listItem = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    NodeAddress dstAddr;

    char* payloadBuff = NULL;
    int maxPayloadSize;
    int payloadSize;
    int count;

    if (Ospfv2ListIsEmpty(thisInterface->updateLSAList))
    {
        return;
    }

    // Determine Destination Address
#ifdef ADDON_BOEINGFCS
    if (thisInterface->type == OSPFv2_ROSPF_INTERFACE)
    {
        dstAddr = nbrAddr;
    }
    else
#endif
    if ((thisInterface->state == S_DR) || (thisInterface->state == S_Backup))
    {
        dstAddr = OSPFv2_ALL_SPF_ROUTERS;
    }
    else if (thisInterface->type == OSPFv2_BROADCAST_INTERFACE)
    {
        dstAddr = OSPFv2_ALL_D_ROUTERS;
    }
    else
    {
        dstAddr = OSPFv2_ALL_SPF_ROUTERS;
    }

    // Determine maximum payload size of update packet
    maxPayloadSize =
        (GetNetworkIPFragUnit(node, interfaceId)
         - sizeof(IpHeaderType)
         - sizeof(Ospfv2LinkStateUpdatePacket));

    payloadBuff = (char*) MEM_malloc(maxPayloadSize);

    payloadSize = 0;
    count = 0;

    listItem = thisInterface->updateLSAList->first;
    // Prepare LS Update packet payload
    while (listItem != NULL)
    {
        LSHeader = (Ospfv2LinkStateHeader*) listItem->data;

        if (payloadSize == 0 && LSHeader->length > maxPayloadSize)
        {
            MEM_free(payloadBuff);
            payloadBuff = (char*) MEM_malloc(LSHeader->length);
        }
        else if (payloadSize + LSHeader->length > maxPayloadSize)
        {
            Ospfv2SendUpdatePacket(node,
                                   nbrAddr,
                                   interfaceId,
                                   payloadBuff,
                                   payloadSize,
                                   count);

            // Reset variables
            payloadSize = 0;
            count = 0;
            continue;
       }

       // Incrememnt LS age transmission delay (in seconds).
        LSHeader->linkStateAge = (short int) (LSHeader->linkStateAge +
                ((thisInterface->infTransDelay / SECOND)));

        // LS age has a maximum age limit.
        if (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) >
            (OSPFv2_LSA_MAX_AGE / SECOND))
        {
            Ospfv2AssignNewAgeToLSAAge(ospf,
                                       LSHeader->linkStateAge,
                                       (OSPFv2_LSA_MAX_AGE / SECOND));
        }

        memcpy(&payloadBuff[payloadSize], listItem->data, LSHeader->length);
        payloadSize += LSHeader->length;
        count++;

        if (deleteLSAsFromUpdateList == TRUE)
        {
            // Remove item from list
            Ospfv2RemoveFromList(node,
                                 thisInterface->updateLSAList,
                                 listItem,
                                 FALSE);
            listItem = thisInterface->updateLSAList->first;
        }
        else
        {
            listItem = listItem->next;
        }
    }

    if (count > 0)
    {
        Ospfv2SendUpdatePacket(node,
                               nbrAddr,
                               interfaceId,
                               payloadBuff,
                               payloadSize,
                               count);
    }

    MEM_free(payloadBuff);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendUpdatePacket()
// PURPOSE      :Send LS Updated packet to neighbor
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2SendUpdatePacket(
    Node* node,
    NodeAddress dstAddr,
    int interfaceId,
    char* payload,
    int payloadSize,
    int LSACount)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2LinkStateUpdatePacket* updatePkt = NULL;
    NodeAddress nextHop;

    Message* updateMsg = MESSAGE_Alloc(node,
                                       NETWORK_LAYER,
                                       ROUTING_PROTOCOL_OSPFv2,
                                       MSG_ROUTING_OspfPacket);

    MESSAGE_PacketAlloc(node,
                        updateMsg,
                        sizeof(Ospfv2LinkStateUpdatePacket)
                        + payloadSize,
                        TRACE_OSPFv2);

    updatePkt = (Ospfv2LinkStateUpdatePacket*)
                        MESSAGE_ReturnPacket(updateMsg);

    memset(updatePkt, 0, sizeof(Ospfv2LinkStateUpdatePacket)
                            + payloadSize);

    // Fill in the header fields for the Update packet
    Ospfv2CreateCommonHeader(node,
                             &updatePkt->commonHeader,
                             OSPFv2_LINK_STATE_UPDATE);

    updatePkt->commonHeader.packetLength = (short)
            (sizeof(Ospfv2LinkStateUpdatePacket)
                + payloadSize);

    updatePkt->commonHeader.areaID = ospf->iface[interfaceId].areaId;

    updatePkt->numLSA = LSACount;

    // Add payload
    memcpy(updatePkt + 1, payload, payloadSize);

    if (Ospfv2DebugFlood(node))
    {
        char dstStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(dstAddr,dstStr);

        fprintf(stdout, "    Node %u sending UPDATE to dest %s\n",
                         node->nodeId, dstStr);

    }

    //Trace sending pkt
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, updateMsg,TRACE_NETWORK_LAYER,
                     PACKET_OUT, &acnData);


    if (dstAddr == OSPFv2_ALL_SPF_ROUTERS
        || dstAddr == OSPFv2_ALL_D_ROUTERS)
    {
        nextHop = ANY_DEST;
    }
    else
    {
        // Sanity checking
#ifdef JNE_LIB
        if (NetworkIpIsMulticastAddress(node, dstAddr))
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Destination address is not valid",
                JNE::CRITICAL);
        }
#endif
        ERROR_Assert(!NetworkIpIsMulticastAddress(node, dstAddr),
            "Destination address is not valid\n");
        nextHop = dstAddr;
    }

    //  Need to get the packet size prior to sending to lower layers as lower
    //  this message might be freed after NetworkIpSendRawMessageToMacLayer
    //  returns.
    int updatePktSize = MESSAGE_ReturnPacketSize(updateMsg);

    // Send Update packet
    NetworkIpSendRawMessageToMacLayer(
                        node,
                        updateMsg,
                        NetworkIpGetInterfaceAddress(node, interfaceId),
                        dstAddr,
                        IPTOS_PREC_INTERNETCONTROL,
                        IPPROTO_OSPF,
                        1,
                        interfaceId,
                        nextHop);

#if COLLECT_DETAILS
    // instrumented code
    numLSUpdateBytes += updatePktSize;

    if (node->getNodeTime() - lastLSUpdatePrint >= OSPFv2_PRINT_INTERVAL)
    {
        lastLSUpdatePrint = node->getNodeTime();
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("%s Number of LS Update bytes sent = %ld\n",
               clockStr, numLSUpdateBytes);
    }
    // end instrumented code
#endif

    ospf->stats.numLSUpdateSent++;
    ospf->stats.numLSUpdateBytesSent += updatePktSize;

    // Update the interface based stats
    Ospfv2Interface* thisIntf = NULL;
    thisIntf = &ospf->iface[interfaceId];
    if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
    {
        ospf->iface[interfaceId].interfaceStats.numLSUpdateBytesSent +=
            updatePktSize;
        ospf->iface[interfaceId].interfaceStats.numLSUpdateSent++;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendDelayedAck()
// PURPOSE      :Send Delayed Ack
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2SendDelayedAck(
    Node* node,
    int interfaceId,
    char* LSA,
                          NodeAddress nbrAddr)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*)
                                MEM_malloc(sizeof(Ospfv2LinkStateHeader));

    memcpy(LSHeader, LSA, sizeof(Ospfv2LinkStateHeader));

    Ospfv2InsertToList(ospf->iface[interfaceId].delayedAckList,
                       node->getNodeTime(),
                       (void*) LSHeader);

    // Set Delayed Ack timer, if not set yet
    if (ospf->iface[interfaceId].delayedAckTimer == FALSE)
    {
        Message* delayedAckMsg = NULL;
        clocktype delay;

        ospf->iface[interfaceId].delayedAckTimer = TRUE;

        // Interval between delayed transmission must be less than
        // RxmtInterval

        delay = (clocktype)
                ((RANDOM_erand(ospf->seed)
                    * ospf->iface[interfaceId].rxmtInterval) / 2);

        // Set Timer
        delayedAckMsg = MESSAGE_Alloc(node,
                                      NETWORK_LAYER,
                                      ROUTING_PROTOCOL_OSPFv2,
                                      MSG_ROUTING_OspfDelayedAckTimer);

#ifdef ADDON_NGCNMS
        MESSAGE_SetInstanceId(delayedAckMsg, interfaceId);
#endif

        MESSAGE_InfoAlloc(node, delayedAckMsg, sizeof(int));
        memcpy(MESSAGE_ReturnInfo(delayedAckMsg), &interfaceId, sizeof(int));
#ifdef ADDON_BOEINGFCS
        memcpy(MESSAGE_ReturnInfo(delayedAckMsg) + sizeof(int),
               &nbrAddr, sizeof(NodeAddress));
#endif
        MESSAGE_Send(node, delayedAckMsg, delay);
    }
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendAckPacket
// PURPOSE      :Creates acknowledgement packet and send back to source.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2SendAckPacket(
    Node*  node,
    char* payload,
    int count,
    NodeAddress dstAddr,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    clocktype delay;
    Message* ackMsg = NULL;
    Ospfv2LinkStateAckPacket* ackPkt = NULL;
    int size;
    NodeAddress nextHop = ANY_DEST;

    if (count == 0)
    {
        return;
    }

    size = sizeof(Ospfv2LinkStateAckPacket)
                   + sizeof(Ospfv2LinkStateHeader)*  count;

    ackMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfPacket);

    MESSAGE_PacketAlloc(node, ackMsg, size, TRACE_OSPFv2);

    ackPkt = (Ospfv2LinkStateAckPacket*)
        MESSAGE_ReturnPacket(ackMsg);

    memset(ackPkt,
        0,
        sizeof(Ospfv2LinkStateAckPacket)
        + sizeof(Ospfv2LinkStateHeader) * count);

    // Copy LSA header to acknowledge packet.
    // This is how we acknowledged the LSA.

    memcpy(ackPkt + 1,
           payload,
           sizeof(Ospfv2LinkStateHeader) * count);

    Ospfv2CreateCommonHeader(node,
                             &(ackPkt->commonHeader),
                             OSPFv2_LINK_STATE_ACK);

    ackPkt->commonHeader.packetLength = (short) size;
    ackPkt->commonHeader.areaID = ospf->iface[interfaceIndex].areaId;

    if (Ospfv2DebugFlood(node))
    {
        char clockStr[100];
        TIME_PrintClockInSecond(node->getNodeTime() + getSimStartTime(node),
                    clockStr);

        char dstStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(dstAddr,dstStr);

        printf("    Node %u sending ACK to dest %s at time %s\n",
                node->nodeId,
                dstStr,
                clockStr);
    }

    //ospf->stats.numAckSent++;

    //Trace sending pkt
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, ackMsg,TRACE_NETWORK_LAYER,
                     PACKET_OUT, &acnData);

    delay = 0;

    if (dstAddr == OSPFv2_ALL_SPF_ROUTERS
        || dstAddr == OSPFv2_ALL_D_ROUTERS)
    {
        nextHop = ANY_DEST;
    }
    else
    {
        // Sanity checking
#ifdef JNE_LIB
        if (NetworkIpIsMulticastAddress(node, dstAddr))
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Destination address is not valid",
                JNE::CRITICAL);
        }
#endif
        ERROR_Assert(!NetworkIpIsMulticastAddress(node, dstAddr),
            "Destination address is not valid\n");
        nextHop = dstAddr;
    }

    // Send out the ACK

    int ackPktSize = MESSAGE_ReturnPacketSize(ackMsg);

    NetworkIpSendRawMessageToMacLayerWithDelay(
                            node,
                            ackMsg,
                            NetworkIpGetInterfaceAddress(node,
                                                         interfaceIndex),
                            dstAddr,
                            IPTOS_PREC_INTERNETCONTROL,
                            IPPROTO_OSPF,
                            1,
                            interfaceIndex,
                            nextHop,
                            delay);


    ospf->stats.numAckSent++;

    // instrumented code
    ospf->stats.numAckBytesSent += ackPktSize;

    // Update interface based stats
    Ospfv2Interface* thisIntf = NULL;
    thisIntf = &ospf->iface[interfaceIndex];
    if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
    {
        ospf->iface[interfaceIndex].interfaceStats.numAckSent++;
        ospf->iface[interfaceIndex].interfaceStats.numAckBytesSent +=
            ackPktSize;
    }

#if COLLECT_DETAILS
    if (node->getNodeTime() - lastAckPrint >= OSPFv2_PRINT_INTERVAL)
    {
        lastAckPrint = node->getNodeTime();
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("%s Number of ACK bytes sent = %ld\n",
               clockStr, numAckBytes);
    }
    // end instrumented code
#endif
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendDirectAck()
// PURPOSE      :Send Direct Ack to a neighbor
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//
void Ospfv2SendDirectAck(
        Node* node,
        int interfaceId,
        Ospfv2List* ackList,
        NodeAddress nbrAddr)
{

    Ospfv2ListItem* listItem = NULL;
    int count = 0;
    Ospfv2LinkStateHeader* payload = NULL;
    int maxCount;

//  ERROR_Assert(interfaceId != -1, "No Interface for Neighbor!!\n");

    maxCount =
        (GetNetworkIPFragUnit(node, interfaceId)
         - sizeof(IpHeaderType)
         - sizeof(Ospfv2LinkStateAckPacket)) / sizeof(Ospfv2LinkStateHeader);

    payload = (Ospfv2LinkStateHeader*)
                MEM_malloc(maxCount * sizeof(Ospfv2LinkStateHeader));

    while (!Ospfv2ListIsEmpty(ackList))
    {
        listItem = ackList->first;

        if (count == maxCount)
        {
            Ospfv2SendAckPacket(
                node, (char*) payload, count, nbrAddr, interfaceId);

            // Reset variables
            count = 0;
            continue;
        }

        memcpy(&payload[count],
               listItem->data,
               sizeof(Ospfv2LinkStateHeader));

        count++;

        // Remove item from list
        Ospfv2RemoveFromList(node, ackList, listItem, FALSE);
    }

    Ospfv2SendAckPacket(node,
                        (char*) payload,
                        count,
                        nbrAddr,
                        interfaceId);

    MEM_free(payload);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2InterfaceBelongToThisArea()
// PURPOSE      :Verify if the interface belong to this area.
// ASSUMPTION   :None.
// RETURN VALUE :BOOL
//-------------------------------------------------------------------------//

BOOL Ospfv2InterfaceBelongToThisArea(
    Node* node,
    unsigned int areaId,
    int interfaceIndex)
{
    Ospfv2ListItem* listItem = NULL;
    Ospfv2Interface* thisInterface = NULL;
    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);

#ifdef JNE_LIB
    if (!thisArea)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Area doesn't exist",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(thisArea, "Area doesn't exist\n");

    listItem = thisArea->connectedInterface->first;

    while (listItem)
    {
        thisInterface = (Ospfv2Interface*) listItem->data;

        if (thisInterface->address ==
                NetworkIpGetInterfaceAddress(node, interfaceIndex))
        {
            return TRUE;
        }
        listItem = listItem->next;
    }
    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2GetAddrRangeInfo()
// PURPOSE      :Get this address range information
// ASSUMPTION   :None
// RETURN VALUE :Ospfv2AreaAddressRange*
//-------------------------------------------------------------------------//

Ospfv2AreaAddressRange*  Ospfv2GetAddrRangeInfo(
    Node* node,
    unsigned int areaId,
    NodeAddress destAddr)
{
    Ospfv2AreaAddressRange* addrRangeInfo = NULL;
    Ospfv2Area* area = Ospfv2GetArea(node, areaId);
    Ospfv2ListItem* listItem = area->areaAddrRange->first;

    while (listItem)
    {
        addrRangeInfo = (Ospfv2AreaAddressRange*) listItem->data;

        if ((addrRangeInfo->address & addrRangeInfo->mask) ==
            (destAddr & addrRangeInfo->mask))
        {
            return addrRangeInfo;
        }

        listItem = listItem->next;
    }

    return NULL;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2GetAddrRangeInfoForAllArea()
// PURPOSE      :Get this address range information
// ASSUMPTION   :None
// RETURN VALUE :Ospfv2AreaAddressRange*
//-------------------------------------------------------------------------//

static
Ospfv2AreaAddressRange* Ospfv2GetAddrRangeInfoForAllArea(
    Node* node,
    NodeAddress destAddr)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Area* area = NULL;
    Ospfv2AreaAddressRange* addrRangeInfo = NULL;
    Ospfv2ListItem* listItem = ospf->area->first;

    while (listItem)
    {
        area = (Ospfv2Area*) listItem->data;
        addrRangeInfo = Ospfv2GetAddrRangeInfo(node,
                                               area->areaID,
                                               destAddr);

        if (addrRangeInfo != NULL)
        {
            return addrRangeInfo;
        }

        listItem = listItem->next;
    }

    return NULL;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2CheckForSummaryLSAValidity()
//
// PURPOSE      :Check the validity of summary information. This function is
//               used to check wheather this node's routing table has valid
//               entry for the destination we want to advertise or had
//               advertised at earlier time. The areaId is the id of
//               associated area the summary information will be or had been
//               advertised.
//
// ASSUMPTION   :Default destination is always valid advertisement.
//
// RETURN VALUE :1 if we find a perfect match in routing table.
//               -1 if we find an entry aggregated within this range.
//               0 if we didn't find any matching entry.
//-------------------------------------------------------------------------//

static
int Ospfv2CheckForSummaryLSAValidity(
    Node* node,
    NodeAddress destAddr,
    NodeAddress addrMask,
    Ospfv2DestType destType,
    unsigned int areaId,
    Ospfv2RoutingTableRow** matchPtr)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);
    Ospfv2AreaAddressRange* addrInfo = NULL;

    *matchPtr = NULL;

    // XXX: Assume information is valid for default destination.
    if (destAddr == OSPFv2_DEFAULT_DESTINATION
        && addrMask == OSPFv2_DEFAULT_MASK)
    {
        return 1;
    }

    for (i = 0; i < rtTable->numRows; i++)
    {
        // Do we have a perfect match?
        if ((rowPtr[i].destAddr & rowPtr[i].addrMask)
                    == (destAddr & addrMask)
            && addrMask == rowPtr[i].addrMask
            && Ospfv2CompDestType(rowPtr[i].destType, destType)
            && rowPtr[i].areaId != areaId
            && rowPtr[i].flag != OSPFv2_ROUTE_INVALID)
        {
            *matchPtr = &rowPtr[i];
            return 1;
        }
    }

    // We didn't get a perfect match. So it might be an aggregated
    // advertisement which must be one of our configured area address
    // range. So search for an intra area entry contained within this range.
    addrInfo = Ospfv2GetAddrRangeInfoForAllArea(node, destAddr);
    if (addrInfo)
    {
        for (i = 0; i < rtTable->numRows; i++)
        {
            if ((rowPtr[i].destAddr & addrMask) == (destAddr & addrMask)
                && addrMask < rowPtr[i].addrMask
                && rowPtr[i].pathType == OSPFv2_INTRA_AREA
                && Ospfv2CompDestType(rowPtr[i].destType, destType)
                && rowPtr[i].areaId != areaId
                && rowPtr[i].flag != OSPFv2_ROUTE_INVALID)
            {
                // FIXME: Not sure wheather we require assert here
                /*addrInfo = Ospfv2GetAddrRangeInfoForAllArea(node, destAddr);

                ERROR_Assert(addrInfo != NULL,
                       //&& addrInfo->address == destAddr,
                       // && addrInfo->mask == addrMask,
                        "Something wrong with route suppression\n");
                 */
                *matchPtr = &rowPtr[i];
                return -1;
            }
        }
    }

    return 0;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2FloodLSA
// PURPOSE      :Flood LSA throughout the area.
// ASSUMPTION   :None.
// RETURN VALUE :TRUE if LSA flooded back to receiving interface,
//               FALSE otherwise.
//-------------------------------------------------------------------------//

BOOL Ospfv2FloodLSA(
    Node* node,
    int inIntf,
    char* LSA,
    NodeAddress sourceAddr,
    unsigned int areaId)
{

    /***** Start: OPAQUE-LSA *****/
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    /***** End: OPAQUE-LSA *****/
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    BOOL setDoNotAge = FALSE;
    Ospfv2Neighbor* tempNeighborInfo = NULL;
    Ospfv2ListItem* tempListItem = NULL;

    Ospfv2LinkStateHeader* LSHeader =
            (Ospfv2LinkStateHeader*) LSA;

    BOOL floodedBackToRecvIntf = FALSE;

    Ospfv2LinkStateHeader* listLSHeader = NULL;
    std::list<nbrAdvanceParams> nbrAdvanceList;
    std::list<nbrAdvanceParams>::iterator nbrAdvanceListIt;

#ifdef ADDON_BOEINGFCS
    int* referenceCount = NULL;
    char* lsaCopy = NULL;
#endif

    // Need to first remove LSA from all neigbhor's
    // retx list before flooding it.
    // This is because we want to use the most
    // up-to-date LSA for retransmission.

    Ospfv2RemoveLSAFromRetxList(node,
                                LSHeader->linkStateType,
                                LSHeader->linkStateID,
                                LSHeader->advertisingRouter,
#ifdef ADDON_MA
                                LSHeader->mask,
#endif
                                areaId);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        BOOL addIntoRetxList = FALSE;

        //Since the interface is down so we cannot send any
        // Update regarding the same.
        if ((NetworkIpGetUnicastRoutingProtocolType(node, i) ==
            ROUTING_PROTOCOL_OSPFv2) && ospf->iface[i].state == S_Down)
        {
            continue;
        }

        // If this is not eligible interface, examine next interface
       if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE
            || (!Ospfv2InterfaceBelongToThisArea(node, areaId, i)))
        {
            continue;
        }

        //Ospfv2QueueLSAToFlood(node, &ospf->iface[i], LSA);
        //RFC:1793::Section:3.3. Flooding over demand circuits
        //If one or more LSAs have the DC-bit clear, flooding over demand
        //circuits is unchanged. Otherwise, flooding is changed as follows:
        //(1)Only truly changed LSAs are flooded over demand circuits.
        //   When a router receives a new LSA instance, it checks first to
        //   see whether the contents have changed. If not, the new LSA is
        //   simply a periodic refresh and it is not flooded out on attached
        //   demand circuits (it is still flooded out on other interfaces
        //    however).
        //   When comparing an LSA to its previous instance, the following
        //   are all considered to be changes in contents:
        //   The LSA’s Options field has changed.
        //   One or both of the LSA instances has LS age set to MaxAge
        //   (or DoNotAge + MaxAge).
        //   The length field in the LSA header has changed.
        //   The contents of the LSA, excluding the 20-byte link state
       //    header,have changed. (This excludes changes in LS Sequence
       //    Number and LS Checksum).
        //
        //(2)When it has been decided to flood an LSA over a demand circuit,
        //   DoNotAge should be set in the copy of the LSA that is flooded
        //   out on the demand interface (DoNotAge should not be set if
        //   the LSA’s LS
        //   age is equal to MaxAge.). It is perfectly possible that DoNotAge
        //   will already be set. This simply indicates that the LSA has
        //   already been flooded over demand circuits. In any case, the
        //   flooded copy’s LS age field must also be incremented by
        //   InfTransDelay.

        //RFC:1793::Section 3.2.1
        //Also, even if the negotiation to suppress Hellos fails, the
        //flooding modifications described in Section 3.3 are still
        //performed over the link.

        setDoNotAge = FALSE;
        if ((ospf->supportDC == TRUE) &&
            (ospf->iface[i].ospfIfDemand == TRUE))
        //&&
        //(ospf->iface[i].helloSuppressionSuccess == TRUE))
        {
            //Search area for LSA having DC bit = 0
            if (Ospfv2SearchRouterDBforDC(ospf,
                                          Ospfv2GetArea(node,
                                                        ospf->iface[i].areaId),
                                          0))
            {
                //Flooding is unchanged
                setDoNotAge = FALSE;    //setDoNotAge = 0
            }
            else  //All LSAs have DC = 1
            {
                if (Ospfv2CheckLSAToFloodOverDC(node,
                                                i,
                                                LSHeader))
                {
                    //Now it has been decided to flood
                    //Set DoNotAge For this LSA if age is not MaxAge
                    if ((Ospfv2LSAHasMaxAge(ospf, (char*)LSHeader) == FALSE)
                         &&
                         (Ospfv2CheckForIndicationLSA((Ospfv2RouterLSA*)LSA))
                         == FALSE)
                    {
                        setDoNotAge = TRUE;
                    }
                    else
                    {
                        setDoNotAge = FALSE;
                    }
                }
                else
                {
                    continue;
                }
            }
        }


        tempListItem = ospf->iface[i].neighborList->first;

        // Check each neighbor on this interface.
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;
#ifdef JNE_LIB
            if (!tempNeighborInfo)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Neighbor not found in the Neighbor list",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(tempNeighborInfo,
                "Neighbor not found in the Neighbor list\n");

            // RFC2328, Sec-13.3.1.a
            // If neighbor is in a lesser state than exchange,
            // it does not participate in flooding
            if ((tempNeighborInfo->state < S_Exchange)
/***** Start: OPAQUE-LSA *****/
#ifdef ADDON_BOEINGFCS
                // Must deal with mobile leaf nbrs that will not be
                // in full state, for opaque LSAs
                && ((ospf->iface[i].type != OSPFv2_ROSPF_INTERFACE) ||
                    // The nbr may be in exchange state if it is a mobile
                    // leaf node, I am a mobile leaf parent. In this case,
                    // if the LSA is an AS Scoped Opaque LSA, it is acceptable
                    !((RoutingCesRospfIsMobileLeafParent(node, i)
                         || ip->rospfData->iface[i].isThisMobileLeafRouterActingAsParent
                         )
                      && RoutingCesRospfInMobileLeafList(node,
                                                          tempNeighborInfo->neighborIPAddress,
                                                          tempNeighborInfo->neighborNodeId,
                                                          i)
                      && (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE)
                     )
                   )
#endif
/***** End: OPAQUE-LSA *****/
                 )
            {
                tempListItem = tempListItem->next;
                continue;
            }

            /***** Start: OPAQUE-LSA *****/
            // Skip neighbor if it is not opaque capable
            if ((LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE) &&
                (!Ospfv2DatabaseDescriptionPacketGetO(tempNeighborInfo->dbDescriptionNbrOptions)))
            {
                tempListItem = tempListItem->next;
                continue;
            }
            /***** End: OPAQUE-LSA *****/

            // RFC2328, Sec-13.3.1.b
            // If neighbor state is Exchange or Loading
            else if (tempNeighborInfo->state != S_Full)
            {
                listLSHeader =
                    Ospfv2SearchRequestList(
                        tempNeighborInfo->linkStateRequestList, LSHeader);

                if (listLSHeader)
                {
                    int moreRecentIndicator =
                        Ospfv2CompareLSA(node, LSHeader, listLSHeader);

                    // If the new LSA is less recent
                    if (moreRecentIndicator < 0)
                    {
                        tempListItem = tempListItem->next;
                        continue;
                    }
                    // If the two LSAs are same instances
                    else if (moreRecentIndicator == 0)
                    {
                        Ospfv2RemoveFromRequestList(
                            node,
                            tempNeighborInfo->linkStateRequestList,
                            listLSHeader);

                        if ((Ospfv2ListIsEmpty(
                                tempNeighborInfo->linkStateRequestList))
                            && (tempNeighborInfo->state == S_Loading))
                        {
                            // Cancel retransmission timer
                            tempNeighborInfo->LSReqTimerSequenceNumber += 1;

                            Ospfv2HandleNeighborEvent(node,
                                i,
                                tempNeighborInfo->neighborIPAddress,
                                E_LoadingDone);
                        }

                        tempListItem = tempListItem->next;
                        continue;
                    }
                    // New LSA is more recent
                    else
                    {
                        Ospfv2RemoveFromRequestList(
                            node,
                            tempNeighborInfo->linkStateRequestList,
                            listLSHeader);

                        if ((Ospfv2ListIsEmpty(
                                tempNeighborInfo->linkStateRequestList))
                            && (tempNeighborInfo->state == S_Loading))
                        {
                            // Cancel retransmission timer
                            tempNeighborInfo->LSReqTimerSequenceNumber += 1;

                            nbrAdvanceList.push_back(
                                nbrAdvanceParams
                                    (i,
                                     tempNeighborInfo->neighborIPAddress));
                        }
                    }
                }
            }

            // RFC2328, Sec-13.3.1.c
            // If LSA received from this neighbor, examine next neighbor

            if (tempNeighborInfo->neighborIPAddress == sourceAddr)
            {
                tempListItem = tempListItem->next;
                continue;
            }

            // RFC2328, Sec-13.3.1.d
            // Add to Retransmission list

#ifdef ADDON_BOEINGFCS
            // First time we add this LSA to the list we create one copy and
            // one reference count int.  For the remaining LSAs the copy is
            // reused.
            if (referenceCount == NULL)
            {
                referenceCount = (int*) MEM_malloc(sizeof(int));
                *referenceCount = 0;
                lsaCopy = Ospfv2CopyLSA(LSA);
            }
            Ospfv2AddToLSRetxList(node, i, tempNeighborInfo, lsaCopy,
                referenceCount);
#else
            Ospfv2AddToLSRetxList(node, i, tempNeighborInfo, LSA, NULL);
#endif

            addIntoRetxList = TRUE;
            tempListItem = tempListItem->next;
        }

        // RFC2328, Sec-13.3.2
        if (!addIntoRetxList)
        {
            continue;
        }

        if ((ospf->iface[i].type == OSPFv2_BROADCAST_INTERFACE)
            && (sourceAddr != ANY_DEST) && (i == inIntf))
        {
            // RFC2328, Sec-13.3.3
            // If the new LSA was received on this interface, and it was
            // received from either the Designated Router or the Backup
            // Designated Router, chances are that all the neighbors have
            // received the LSA already. Therefore, examine the next
            // interface.

            if ((ospf->iface[i].designatedRouterIPAddress == sourceAddr)
                || (ospf->iface[i].backupDesignatedRouterIPAddress
                            == sourceAddr))
            {
                continue;
            }

            // RFC2328, Sec-13.3.4
            // If the new LSA was received on this interface, and the
            // interface state is Backup, examine the next interface. The
            // Designated Router will do the flooding on this interface.

            if (ospf->iface[i].state == S_Backup)
            {
                continue;
            }
        }

        if (Ospfv2DebugFlood(node))
        {
            char clockStr[100];
            TIME_PrintClockInSecond(node->getNodeTime() + getSimStartTime(node),
                    clockStr);
            printf("    Node %u: flood UPDATE on interface %d at %s\n",
                    node->nodeId, i, clockStr);
        }

        // Note if we flood the LSA back to receiving interface
        // then this will be used later for sending Ack.

        if ((sourceAddr != ANY_DEST) && (i == inIntf))
        {
            floodedBackToRecvIntf = TRUE;
        }

        Ospfv2QueueLSAToFlood(node, &ospf->iface[i], LSA, setDoNotAge);

        if (ospf->iface[i].floodTimerSet == FALSE)
        {
            Message* floodMsg = NULL;
            clocktype delay;
#ifdef ADDON_BOEINGFCS
            NodeAddress temp = ANY_DEST;
#endif
            ospf->iface[i].floodTimerSet = TRUE;

            // Set Timer
            floodMsg = MESSAGE_Alloc(node,
                                     NETWORK_LAYER,
                                     ROUTING_PROTOCOL_OSPFv2,
                                     MSG_ROUTING_OspfFloodTimer);

#ifdef ADDON_NGCNMS
            MESSAGE_SetInstanceId(floodMsg, i);
#endif

            MESSAGE_InfoAlloc(node, floodMsg, sizeof(int));
            memcpy(MESSAGE_ReturnInfo(floodMsg), &i, sizeof(int));
#ifdef ADDON_BOEINGFCS
            memcpy(MESSAGE_ReturnInfo(floodMsg)+sizeof(int),
                       &temp,
                       sizeof(NodeAddress));
#endif
            //delay = (clocktype) (RANDOM_erand(ospf->seed)
            //                        *  OSPFv2_FLOOD_TIMER);
            delay = (clocktype) (RANDOM_erand(ospf->seed)
                                    *  ospf->floodTimer);
            MESSAGE_Send(node, floodMsg, delay);
        }
    }


    for (nbrAdvanceListIt = nbrAdvanceList.begin();
         nbrAdvanceListIt != nbrAdvanceList.end();
         nbrAdvanceListIt ++)
    {
        Ospfv2HandleNeighborEvent(node,
        (*nbrAdvanceListIt).first,
        (*nbrAdvanceListIt).second,
        E_LoadingDone);
    }

    return floodedBackToRecvIntf;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2FloodLSAKeptInAdvertiseList
// PURPOSE      :Flood pending Type 5 LSAs kept in the Advertise List.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

void Ospfv2FloodLSAKeptInAdvertiseList(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    unsigned int j;

    for (j = 0;j < ospf->nssaAdvertiseList.size();j++)
    {
        if (ospf->nssaAdvertiseList[j].LSA != NULL)
        {
            char* asExtLSA = ospf->nssaAdvertiseList[j].LSA;

            //flooding Type 5 LSA to all attached Type 5 capable areas
            int k = 0;

            for (k = 0; k < node->numberInterfaces; k++)
            {
                Ospfv2Interface* thisInterface = &ospf->iface[k];
                Ospfv2Area* thisArea = Ospfv2GetArea(node,
                                                     thisInterface->areaId);

                if (thisArea && thisArea->isNSSAEnabled)
                {
                    continue;
                }

                Ospfv2FloodLSA(node,
                               k,
                               asExtLSA,
                               ANY_DEST,
                               thisArea->areaID);
            }
            //Entry served.
            if (asExtLSA)
            {
                MEM_free(asExtLSA);
            }
            ospf->nssaAdvertiseList[j].LSA = NULL;
        }// if (ospf->nssaAdvertiseList[j].LSA)
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2UpdateLSAList
// PURPOSE      :Receive a LSA via flooding and LS Type has been determined.
//               Update this node's link state database, either router or
//               network or summary LSA. (determined by list that is being
//               passed in in function Ospfv2UpdateLSDB).
// ASSUMPTION   :None
// RETURN VALUE :TRUE if there was an update, FALSE otherwise.
//-------------------------------------------------------------------------//

static
int Ospfv2UpdateLSAList(
    Node* node,
    int interfaceIndex,
    Ospfv2List* list,
    char* LSA,
    NodeAddress srcAddr,
    unsigned int areaId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2LinkStateHeader* listLSHeader = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    Ospfv2ListItem* listItem = NULL;
    int retVal = 0;
    BOOL floodedBackToRecvIntf = FALSE;

    LSHeader = (Ospfv2LinkStateHeader*) LSA;

    listItem = list->first;

    if (Ospfv2DebugFlood(node))
    {
        printf("    Node %u updating LSDB\n", node->nodeId);
    }

    while (listItem)
    {
        listLSHeader = (Ospfv2LinkStateHeader*) listItem->data;
#ifdef ADDON_MA
        BOOL  isMatched;
         if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL ||
            LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)&&
            (listLSHeader->linkStateType == LSHeader->linkStateType))
        {
            /*
            Ospfv2ExternalLinkInfo* linkInfo;
            Ospfv2ExternalLinkInfo* listInfo;
            linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
            listInfo = (Ospfv2ExternalLinkInfo*) (listLSHeader + 1);
            // NPDR Temp fix
            //if (linkInfo->networkMask <= listInfo->networkMask)
            if (linkInfo->networkMask == listInfo->networkMask)
          */
            if (LSHeader->mask == listLSHeader->mask)
            {
                isMatched = TRUE;
            }else
            {
                isMatched = FALSE;
            }
        }else
        {
           isMatched = TRUE;
        }

        if (LSHeader->advertisingRouter == listLSHeader->advertisingRouter &&
            LSHeader->linkStateID == listLSHeader->linkStateID &&
            isMatched)
#else
        if ((LSHeader->advertisingRouter == listLSHeader->advertisingRouter)
                && (LSHeader->linkStateID == listLSHeader->linkStateID))
#endif
        {
            if (Ospfv2DebugFlood(node))
            {
                printf("    LSA found in LSDB\n"
                       "    advertisingRouter %x linkStateID %x\n",
                       LSHeader->advertisingRouter, LSHeader->linkStateID);
            }

            // RFC2328, Sec-13 (5.a)
            // If there is already a database copy, and if the database copy
            // was received via flooding (i.e. not self originated) and
            // installed less than MinLSArrival seconds ago, discard new LSA
            // (without acknowledging it) and examine the next LSA (if any).
            if ((!(Ospfv2LsaIsSelfOriginated(node, LSA))) &&
                (node->getNodeTime() - listItem->timeStamp) <
                                                    (OSPFv2_MIN_LS_ARRIVAL))

            {
                if (Ospfv2DebugFlood(node))
                {
                    printf("Node %u:  Received LSA is more recent, but"
                        " installed < MinLSArrival ago, so don't "
                        "update LSDB\n", node->nodeId);
                }

                // Examine next LSA
                return -1;
            }

            break;
        }

        listItem = listItem->next;
    }
    BOOL dontFlood = FALSE;

#ifdef ADDON_BOEINGFCS
    if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
    {
        if (RoutingCesRospfIsMobileLeafNode(node, interfaceIndex))
        {
            dontFlood = TRUE;
        }
    }
#endif

    if (!dontFlood)
    {

        // RFC2328, Sec-13 (Bullet - 5.b) & (Bullet - 5.e)
        // Immediately Flood LSA

        if (Ospfv2DebugFlood(node))
        {
            printf("    Flood received LSA\n");
        }

        if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL) ||
            /***** Start: OPAQUE-LSA *****/
            (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
            /***** End: OPAQUE-LSA *****/
        {
            floodedBackToRecvIntf = Ospfv2FloodThroughAS(
                                        node,interfaceIndex, LSA, srcAddr);
        }
        else if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
        {
            floodedBackToRecvIntf = Ospfv2FloodThroughNSSA(node,
                                       interfaceIndex,
                                       LSA,
                                       srcAddr,
                                       areaId);
        }
        else
        {
            floodedBackToRecvIntf = Ospfv2FloodLSA(node,
                                                   interfaceIndex,
                                                   LSA,
                                                   srcAddr,
                                                   areaId);
        }

        if (!floodedBackToRecvIntf)
        {
            if (ospf->iface[interfaceIndex].state == S_Backup)
            {
                if (ospf->iface[interfaceIndex].designatedRouterIPAddress ==
                        srcAddr)
                {
                    // Send Delayed Ack
                    Ospfv2SendDelayedAck(node, interfaceIndex, LSA, srcAddr);
                }
            }
            else
            {
                // Send Delayed Ack
                Ospfv2SendDelayedAck(node, interfaceIndex, LSA, srcAddr);
            }
        }
    }

    // RFC2328, Sec-13 (5.d)
    // Install LSA in LSDB. this may schedule Routing table calculation.
    if (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge)
        >= (OSPFv2_LSA_MAX_AGE / SECOND))
    {
#ifdef ADDON_MA
        if (node->mAEnabled == TRUE && ospf->sendLSAtoMA == TRUE)
        {
            int maRetVal = MAConstructSendAndRecieveCommunication(LSA,node);
        }
#endif
    }

    // RFC2328, Sec-13 (5.d)
    // Install LSA in LSDB. this may schedule Routing table calculation.

    retVal = Ospfv2InstallLSAInLSDB(node, list, LSA, areaId);
    return retVal;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2UpdateLSDB
// PURPOSE      :Receive a LSA via flooding, update this node's LSDB.
// ASSUMPTION   :None.
// RETURN VALUE :TRUE if there was an update, FALSE otherwise.
//-------------------------------------------------------------------------//

BOOL Ospfv2UpdateLSDB(
    Node* node,
    int interfaceIndex,
    char* LSA,
    NodeAddress srcAddr,
    unsigned int areaId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;
    Ospfv2Area* thisArea = NULL;
    BOOL retVal = FALSE;

    // BGP-OSPF Patch Start
    if (LSHeader->linkStateType != OSPFv2_ROUTER_AS_EXTERNAL &&
        LSHeader->linkStateType != OSPFv2_ROUTER_NSSA_EXTERNAL &&
        /***** Start: OPAQUE-LSA *****/
        LSHeader->linkStateType != OSPFv2_AS_SCOPE_OPAQUE)
        /***** End: OPAQUE-LSA *****/

    {
        thisArea = Ospfv2GetArea(node, areaId);
#ifdef JNE_LIB
        if (!thisArea)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Doesn't find the area",
                JNE::CRITICAL);
        }
#endif
        ERROR_Assert(thisArea, "Doesn't find the area\n");
    }
    // BGP-OSPF Patch End

    if (Ospfv2DebugFlood(node))
    {
        Ospfv2PrintLSA(LSA);
    }

    if (LSHeader->linkStateType == OSPFv2_ROUTER)
    {
        if (Ospfv2DebugFlood(node))
        {
            printf("Node %u updating Router LSDB\n", node->nodeId);
        }

        retVal = Ospfv2UpdateLSAList(node,
                                     interfaceIndex,
                                     thisArea->routerLSAList,
                                     LSA,
                                     srcAddr,
                                     areaId);
    }
    else if (LSHeader->linkStateType == OSPFv2_NETWORK)
    {
        if (Ospfv2DebugFlood(node))
        {
            printf("Node %u updating Network LSDB\n", node->nodeId);
        }

        retVal = Ospfv2UpdateLSAList(node,
                                     interfaceIndex,
                                     thisArea->networkLSAList,
                                     LSA,
                                     srcAddr,
                                     areaId);
    }
    else if (LSHeader->linkStateType == OSPFv2_NETWORK_SUMMARY)
    {
        if (Ospfv2DebugFlood(node))
        {
            printf("Node %u updating Network Summary LSDB\n", node->nodeId);
        }

        retVal = Ospfv2UpdateLSAList(node,
                                     interfaceIndex,
                                     thisArea->networkSummaryLSAList,
                                     LSA,
                                     srcAddr,
                                     areaId);
    }
    else if (LSHeader->linkStateType == OSPFv2_ROUTER_SUMMARY)
    {
        if (Ospfv2DebugFlood(node))
        {
            printf("Node %u updating Router Summary LSDB\n", node->nodeId);
        }

        retVal = Ospfv2UpdateLSAList(node,
                                     interfaceIndex,
                                     thisArea->routerSummaryLSAList,
                                     LSA,
                                     srcAddr,
                                     areaId);
    }
    // BGP-OSPF Patch Start
    else if (LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
    {
        if (Ospfv2DebugFlood(node))
        {
            printf("Node %u updating OSPFv2_ROUTER_AS_EXTERNAL LSDB\n",
                node->nodeId);
        }
        retVal =  Ospfv2UpdateLSAList(
                        node,
                        interfaceIndex,
                        ospf->asExternalLSAList,
                        LSA,
                        srcAddr,
                        areaId);
    }
    // BGP-OSPF Patch End
    else if (LSHeader->linkStateType == OSPFv2_GROUP_MEMBERSHIP)
    {
        if (Ospfv2DebugFlood(node))
        {
            printf("Node %u updating GroupMembership LSDB\n", node->nodeId);
        }
        retVal =  Ospfv2UpdateLSAList(
                        node,
                        interfaceIndex,
                        thisArea->groupMembershipLSAList,
                        LSA,
                        srcAddr,
                        areaId);
    }
    else if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
    {
        if (Ospfv2DebugFlood(node))
        {
            printf("Node %u updating OSPFv2_ROUTER_NSSA_EXTERNAL LSDB\n",
                node->nodeId);
        }
        retVal =  Ospfv2UpdateLSAList(
                        node,
                        interfaceIndex,
                        ospf->nssaExternalLSAList,
                        LSA,
                        srcAddr,
                        areaId);
    }
    /***** Start: OPAQUE-LSA *****/
    else if (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE)
    {
        if (Ospfv2DebugFlood(node))
        {
            printf("Node %u updating OSPFv2_AS_SCOPE_OPAQUE LSDB\n",
                node->nodeId);
        }
        retVal =  Ospfv2UpdateLSAList(
                        node,
                        interfaceIndex,
                        ospf->ASOpaqueLSAList,
                        LSA,
                        srcAddr,
                        areaId);
    }
    /***** End: OPAQUE-LSA *****/
    else
    {
        ERROR_Assert(FALSE, "LS Type not known. Can't update LSDB\n");
    }

    // RFC 2328 - Bullet 13 (5).(a)
    // Examine next LSA since older DB copy was installed less than
    // OSPFv2_MIN_LS_ARRIVAL seconds ago.
    if (retVal == -1)
    {
        return FALSE;
    }

    // RFC2328 (Sec-13, bullet-5.f)
    // Should take special action if this node is originator.
    if (Ospfv2LsaIsSelfOriginated(node, LSA))

    {
        // RFC 2328, Sec - 13.4
        // Only check the following case for now:
        // 1) LSA is a summary-LSA and the router no longer has an
        //    (advertisable) route to the destination,
        // 2) LSA is a network-LSA but the router is no longer DR
        //    for the network
        BOOL flush = FALSE;

        switch (LSHeader->linkStateType)
        {
            case OSPFv2_NETWORK:
            {
                int interfaceId = NetworkIpGetInterfaceIndexFromAddress(
                                        node, LSHeader->linkStateID);

                if ((ospf->iface[interfaceId].state != S_DR) ||
                    ((Ospfv2IsMyAddress(node, LSHeader->linkStateID)) &&
                     (LSHeader->advertisingRouter != ospf->routerID)))
                {
                    flush = TRUE;
                }
                break;
            }
            case OSPFv2_NETWORK_SUMMARY:
            {
                NodeAddress destAddr;
                NodeAddress addrMask;
                Ospfv2RoutingTableRow* rowPtr;
                int matchType;

                destAddr = LSHeader->linkStateID;
                memcpy(&addrMask, LSHeader + 1, sizeof(NodeAddress));

                matchType = Ospfv2CheckForSummaryLSAValidity(
                                node, destAddr, addrMask,
                                OSPFv2_DESTINATION_NETWORK,
                                areaId, &rowPtr);

                if (!matchType)
                {
                    flush = TRUE;
                }

                break;
            }
            case OSPFv2_ROUTER_SUMMARY:
            {

                NodeAddress destAddr;
                NodeAddress addrMask = 0xFFFFFFFF;
                Ospfv2RoutingTableRow* rowPtr;
                int matchType;

                destAddr = LSHeader->linkStateID;

                matchType = Ospfv2CheckForSummaryLSAValidity(
                                node, destAddr, addrMask,
                                OSPFv2_DESTINATION_ASBR,
                                areaId, &rowPtr);

                if (!matchType)
                {
                    flush = TRUE;
                }

                break;

            }
            case OSPFv2_ROUTER_AS_EXTERNAL:
            {
                NodeAddress destAddr;
                destAddr = LSHeader->linkStateID;

                if (!(Ospfv2GetValidRoute(node, destAddr,
                        OSPFv2_DESTINATION_NETWORK))
                   && (destAddr != OSPFv2_DEFAULT_DESTINATION))
                {
                    flush = TRUE;
                }
                break;
            }
            /***** Start: OPAQUE-LSA *****/
            case OSPFv2_AS_SCOPE_OPAQUE:
            {
#ifdef CYBER_CORE
#ifdef ADDON_BOEINGFCS
                if (Ospfv2OpaqueGetOpaqueType(LSHeader->linkStateID)
                        == (unsigned char)HAIPE_ADDRESS_ADV)
                {
                    if (RoutingCesRospfIsValidHaipeAdvLSA(node,
                           (Ospfv2ASOpaqueLSA*)LSA) == FALSE)
                    {
                        flush = TRUE;
                    }
                }
#endif
#endif
                break;
            }
            /***** End: OPAQUE-LSA *****/
            default:
            {
                break;
            }
        }

        if (flush)
        {
            if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL) ||
                /***** Start: OPAQUE-LSA *****/
               (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
               /***** End: OPAQUE-LSA *****/
            {
                Ospfv2FlushLSA(node, LSA, OSPFv2_INVALID_AREA_ID);
            }
            else
            {
                Ospfv2FlushLSA(node, LSA, areaId);
            }
        }
        else
        {
            Ospfv2LinkStateHeader* oldLSHeader =
                                    Ospfv2LookupLSDB(node,
                                    LSHeader->linkStateType,
                                    LSHeader->advertisingRouter,
                                    LSHeader->linkStateID,
                                    interfaceIndex);
            // In this function i.e. 'Ospfv2UpdateLSDB',
            // received LSA will be installed into LSDB,
            // so oldLSHeader will not be NULL.
            oldLSHeader->linkStateSequenceNumber =
                            LSHeader->linkStateSequenceNumber + 1;
            oldLSHeader->linkStateCheckSum =
                    OspfFindLinkStateChecksum(oldLSHeader);

            Ospfv2ScheduleLSA(node, LSHeader, areaId);
        }
    }

    return retVal;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleUpdatePacket
// PURPOSE      :Just received Update packet, so handle appropriately.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2HandleUpdatePacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddr,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Area* thisArea = Ospfv2GetArea(
                                    node,
                                    ospf->iface[interfaceIndex].areaId);
    Ospfv2LinkStateHeader* LSHeader = NULL;
    Ospfv2LinkStateUpdatePacket* updatePkt = NULL;
    Ospfv2Neighbor* nbrInfo = NULL;
    int count;
    BOOL isLSDBChanged = FALSE;
    int moreRecentIndicator = 0;
    BOOL lsaWithDCzeroFound = FALSE;

#ifdef ADDON_BOEINGFCS
    if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
    {
        RoutingCesRospfHandleUpdatePacket(
                                    node, msg, sourceAddr, interfaceIndex);
        return;
    }
#endif
    Ospfv2List* directAckList = NULL;

    nbrInfo = Ospfv2GetNeighborInfoByIPAddress(
                                    node, interfaceIndex, sourceAddr);

    if (nbrInfo == NULL)
    {
        if (Ospfv2DebugFlood(node))
        {
            char sourceStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(sourceAddr,sourceStr);

            fprintf(stdout, "Node %u receive LS Update from unknown"
                " neighbor %s. Discard LS Update packet\n",
                             node->nodeId, sourceStr);
        }

        return;
    }

    if (nbrInfo->state < S_Exchange)
    {
        if (Ospfv2DebugFlood(node))
        {
            char sourceStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(sourceAddr,sourceStr);

            printf("Node %u : Neighbor (%s) state is below S_Exchange. "
                   "Discard LS Update packet\n",
                   node->nodeId, sourceStr);
        }

        return;
    }

    if (Ospfv2DebugFlood(node))
    {
        char sourceStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(sourceAddr,sourceStr);

        printf("Node %u: OSPFv2 Process UPDATE from node %s\n",
                node->nodeId, sourceStr);
    }

    updatePkt = (Ospfv2LinkStateUpdatePacket*) msg->packet;
    LSHeader = (Ospfv2LinkStateHeader*) (updatePkt + 1);

    Ospfv2InitList(&directAckList);

    // Execute following steps for each LSA
    for (count = 0; count < updatePkt->numLSA; count++,
            LSHeader = (Ospfv2LinkStateHeader*)
                (((char*) LSHeader) + LSHeader->length))
    {
        Ospfv2LinkStateHeader* oldLSHeader;

        if (Ospfv2DebugFlood(node))
        {
            printf("    Process %dth LSA (Type = %d) in update pkt\n",
                        count + 1, LSHeader->linkStateType);
            Ospfv2PrintLSHeader(LSHeader);
        }

        if (ospf->supportDC == TRUE)
        {
            if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge)
                == TRUE)
            {
                ospf->stats.numDoNotAgeLSARecv++;
            }
        }

        // RFC 2328 Sec-13 (2): Validate LSA's LS Type
        if (LSHeader->linkStateType < OSPFv2_ROUTER
            || LSHeader->linkStateType > OSPFv2_ROUTER_NSSA_EXTERNAL)
        {
            if (Ospfv2DebugFlood(node))
            {
                char sourceStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(sourceAddr,sourceStr);

                fprintf(stdout, "Node %u : Receive bad LSA (Type = %d) from"
                                " neighbor (%s). Discard LSA and examine"
                                " next one\n",
                                 node->nodeId, LSHeader->linkStateType,
                                 sourceStr);
            }

            continue;
        }

        // RFC 2328 Sec-13 (3): AS-External LSA shouldn't flooded throughout
        // stub area
        if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
            && thisArea && (thisArea->externalRoutingCapability == FALSE))
        {
            if (Ospfv2DebugFlood(node))
            {
                char ifaceAddrStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(
                    ospf->iface[interfaceIndex].address, ifaceAddrStr);
                fprintf(stdout, "    Received Interface %s belongs to stub"
                                " area. Shouldn't process ASExternal LSA.\n",
                                 ifaceAddrStr);
            }

            continue;
        }

        if ((LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
            && thisArea && (thisArea->isNSSAEnabled == FALSE))
        {
            if (Ospfv2DebugFlood(node))
            {
                char ifaceAddrStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(
                    ospf->iface[interfaceIndex].address, ifaceAddrStr);
                fprintf(stdout, "    Received Interface %s belongs to NSSA "
                                " area. Shouldn't process NSSAExternal LSA.\n",
                         ifaceAddrStr);
            }

            continue;
        }

#ifdef ADDON_DB
        if (ospf->isMospfEnable == TRUE)
        {
            MospfData* mospf =
                (MospfData*)ospf->multicastRoutingProtocol;

            if (mospf)
            {

                if (LSHeader->linkStateType == OSPFv2_GROUP_MEMBERSHIP)
                {
                    mospf->mospfSummaryStats.m_NumGroupLSAReceived++;
                }

                if (LSHeader->linkStateType == OSPFv2_ROUTER)
                {
                    Ospfv2RouterLSA* routerLSA = (Ospfv2RouterLSA*) LSHeader;

                    if (Ospfv2RouterLSAGetWCMCReceiver(
                                                   routerLSA->ospfRouterLsa))
                    {
                        mospf->mospfSummaryStats.
                                               m_NumRouterLSA_WCMRReceived++;
                    }

                    if (Ospfv2RouterLSAGetVirtLnkEndPt(
                                                   routerLSA->ospfRouterLsa))
                    {
                        mospf->mospfSummaryStats.
                                               m_NumRouterLSA_VLEPReceived++;
                    }

                    if (Ospfv2RouterLSAGetASBRouter(routerLSA->ospfRouterLsa))
                    {
                        mospf->mospfSummaryStats.
                                               m_NumRouterLSA_ASBRReceived++;
                    }

                    if (Ospfv2RouterLSAGetABRouter(routerLSA->ospfRouterLsa))
                    {
                       mospf->mospfSummaryStats.m_NumRouterLSA_ABRReceived++;
                    }
                }
            }
        }
#endif

        // Find instance of this LSA contained in local database
        oldLSHeader = Ospfv2LookupLSDB(node,
                                       LSHeader->linkStateType,
                                       LSHeader->advertisingRouter,
                                       LSHeader->linkStateID,
                                       interfaceIndex);
#ifdef ADDON_MA
        if (LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL &&
            oldLSHeader)
        {
            Ospfv2ExternalLinkInfo* linkInfo;
            linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
            oldLSHeader = Ospfv2LookupLSAList_Extended(node,
                                                      ospf->asExternalLSAList,
                                                      LSHeader->advertisingRouter,
                                                      LSHeader->linkStateID,
                                                      linkInfo->networkMask);
        }else if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL &&
            oldLSHeader)
        {
            Ospfv2ExternalLinkInfo* linkInfo;
            linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
            oldLSHeader = Ospfv2LookupLSAList_Extended(node,
                                                      ospf->nssaExternalLSAList,
                                                      LSHeader->advertisingRouter,
                                                      LSHeader->linkStateID,
                                                      linkInfo->networkMask);
        }
#endif
        // RFC 2328 Sec-13 (4)
        if (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) ==
            (OSPFv2_LSA_MAX_AGE / SECOND)
            && (!oldLSHeader)
            && (Ospfv2NoNbrInStateExhangeOrLoading(node,
                                                   interfaceIndex)))
        {
            void* ackLSHeader = NULL;

            ackLSHeader = MEM_malloc(sizeof(Ospfv2LinkStateHeader));
            memcpy(ackLSHeader, LSHeader, sizeof(Ospfv2LinkStateHeader));
            Ospfv2InsertToList(directAckList, 0, ackLSHeader);

            if (Ospfv2DebugFlood(node))
            {
                printf("    LSA's LSAge is equal to MaxAge and there is "
                       "current no instance of the LSA in the LSDB,\n"
                       "    and none of the routers neighbor are in state "
                       "Exchange or Loading. Send direct Ack\n");
            }

            continue;
        }

        if (oldLSHeader)
        {
            moreRecentIndicator = Ospfv2CompareLSA(node, LSHeader,
                                                   oldLSHeader);
        }
        // RFC 2328 Sec-13 (5)
        if ((!oldLSHeader)
            || (moreRecentIndicator > 0))
        {
            if (Ospfv2DebugFlood(node))
            {
                printf("    Received LSA is more recent or no current "
                       "Database copy exist\n");
            }

            // Update our link state database, if applicable.
            if (Ospfv2UpdateLSDB(node,
                                 interfaceIndex,
                                 (char*) LSHeader,
                                 sourceAddr,
                                 thisArea->areaID))
            {
                isLSDBChanged = TRUE;

                //RFC:1793::SECTION:2.5::INTEROPERABILITY WITH UNMODIFIED
                //OSPF ROUTERS
                //If any LSA is found not having its DC-bit set
                //(regardless of reachability), then the router should flush
                //(i.e.,prematurely age; see Section 14.1 of [1]) from the
                //area all DoNotAge LSAs.

                //If LSA with DC bit clear has come then no need to check
                //the database for LSA with DC bit 0 as it is for sure one
                //is there as this new LSA with DC bit 0 has been updated in
                //the database
                if (ospf->supportDC == TRUE)
                {
                    if (!Ospfv2OptionsGetDC(LSHeader->options))
                    {
                        //flush from the area all DoNotAge LSAs
                        Ospfv2FlushAllDoNotAgeFromArea(node, ospf, thisArea);
                    }
                    else
                    {
                        //if the LSA with DC bit 1 has come so search the
                        //whole router database for
                        //LSAs with DC bit 0 and then flush all DoNotAge LSAs
                        lsaWithDCzeroFound = Ospfv2SearchRouterDBforDC(ospf,
                            thisArea, 0);

                        if (lsaWithDCzeroFound)
                        {
                            Ospfv2FlushAllDoNotAgeFromArea(node, ospf,
                                thisArea);
                        }
                    }
                }
            }
            continue;
        }

        // RFC 2328 Sec-13 (6)
        if (Ospfv2SearchRequestList(nbrInfo->linkStateRequestList, LSHeader))
        {
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      nbrInfo->neighborIPAddress,
                                      E_BadLSReq);

            Ospfv2FreeList(node, directAckList, FALSE);

            return;
        }

        // RFC 2328 Sec-13 (7)
        if (moreRecentIndicator == 0)
        {
            Ospfv2ListItem* listItem;

            if (Ospfv2DebugFlood(node))
            {
                printf("    LSA is duplicate\n");
            }

            listItem = nbrInfo->linkStateRetxList->first;

            while (listItem)
            {
                Ospfv2LinkStateHeader* retxLSHeader;

                retxLSHeader = (Ospfv2LinkStateHeader*)
                    listItem->data;

                // If LSA exists in retransmission list
                if (Ospfv2CompareLSAToListItem(
                        node,
                        LSHeader,
                        retxLSHeader))
                {
                    break;
                }

                listItem = listItem->next;
            }

            if (listItem)
            {
                // Treat received LSA as implied Ack and
                // remove LSA From LSRetx List
                Ospfv2RemoveFromList(node,
                                     nbrInfo->linkStateRetxList,
                                     listItem,
                                     FALSE);
                if (Ospfv2DebugFlood(node))
                {
                    char neighborIPStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress,
                                                neighborIPStr);

                    printf("    Received LSA treated as implied ack\n");
                    printf("    Node %u: Removing LSA with seqno 0x%x from "
                           "rext list of nbr %s\n",
                            node->nodeId, LSHeader->linkStateSequenceNumber,
                            neighborIPStr);

                    char linkStateIDStr[MAX_ADDRESS_STRING_LENGTH];
                    char advertisingRouterStr[MAX_ADDRESS_STRING_LENGTH];

                    IO_ConvertIpAddressToString(LSHeader->linkStateID,
                                                linkStateIDStr);
                    IO_ConvertIpAddressToString(LSHeader->advertisingRouter,
                                                advertisingRouterStr);

                    printf("        Type = %d, ID = %s,"
                            " advRtr = %s age = %d \n",
                            LSHeader->linkStateType,
                            linkStateIDStr,
                            advertisingRouterStr,
                            LSHeader->linkStateAge);
                }

                if (Ospfv2ListIsEmpty(nbrInfo->linkStateRetxList))
                {
                    nbrInfo->LSRetxTimer = FALSE;
                    nbrInfo->rxmtTimerSequenceNumber++;
                }

                if ((ospf->iface[interfaceIndex].state == S_Backup)
                    && (ospf->iface[interfaceIndex].
                            designatedRouterIPAddress == sourceAddr))
                {
                    if (Ospfv2DebugFlood(node))
                    {
                        printf("    I'm BDR. Send delayed Ack to DR\n");
                    }

                    // Send Delayed ack
                    Ospfv2SendDelayedAck(node,
                                         interfaceIndex,
                                         (char*) LSHeader,
                                         sourceAddr);
                }
            }
            else
            {
                void* ackLSHeader = NULL;

                ackLSHeader =
                    MEM_malloc(sizeof(Ospfv2LinkStateHeader));
                memcpy(ackLSHeader,
                       LSHeader,
                       sizeof(Ospfv2LinkStateHeader));

                Ospfv2InsertToList(directAckList,
                                   0,
                                   ackLSHeader);

                if (Ospfv2DebugFlood(node))
                {
                    char sourceStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(sourceAddr,sourceStr);

                    printf("    Received LSA was not treated as "
                        "implied Ack. Send direct ack to nbr %s\n",
                            sourceStr);
                }

            }
            continue;
        }

        // RFC 2328 Sec-13 (8)
        if (moreRecentIndicator < 0)
        {
            if ((Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge) ==
                                             (OSPFv2_LSA_MAX_AGE / SECOND))
                && (oldLSHeader->linkStateSequenceNumber ==
                        (signed int) OSPFv2_MAX_SEQUENCE_NUMBER))
            {
                if (Ospfv2DebugFlood(node))
                {
                    printf("Discarding LSA without ACK\n");
                }
                // Discard LSA without acknowledging
            }
            else
            {
                Ospfv2ListItem* sendListItem =
                    Ospfv2SearchSendList(nbrInfo->linkStateSendList,
                                         LSHeader);

                if (ospf->supportDC == TRUE)
                {
                    //RFC:1793::SECTION 2.4 BULLET 2
                    //#2.4  A CHANGE TO THE FLOODING ALGORITHM
                    //(1)If the database copy has LS age equal to MaxAge
                    //   and LS
                    //   sequence number equal to MaxSequenceNumber, simply
                    //   discard the received LSA without acknowledging it.
                    //   (In this case,
                    //   the LSA’s sequence number is wrapping, and the
                    //   MaxSequenceNumber LSA must be completely flushed
                    //   before any
                    //   new LSAs can be introduced). This is identical to
                    //   the behavior specified by Step 8 of Section 13 in
                    //   RFC1583.
                    //(2)Otherwise, send the database copy back to the
                    //   sending neighbor, encapsulated within a Link State
                    //   Update Packet. In
                    //   so doing, do not put the database copy of the LSA
                    //   on the neighbor’s link state retransmission list,
                    //   and do not
                    //   acknowledge the received (less recent) LSA instance.
                    if (ospf->supportDC == TRUE)
                    {
                       //just send the database copy back to the sender
                        Ospfv2SendUpdatePacket(node,
                                               sourceAddr,
                                               interfaceIndex,
                                               (char*) oldLSHeader,
                                               oldLSHeader->length,
                                               1);
                    }
                }
                else
                {
                    if ((sendListItem)
                        && (node->getNodeTime() - sendListItem->timeStamp >
                            OSPFv2_MIN_LS_ARRIVAL))
                    {
                        if (Ospfv2DebugFlood(node))
                        {
                            printf("Sending NEWER LSA to neighbor. "
                                "Not inserting into retx list.\n");
                        }

                        Ospfv2SendUpdatePacket(node,
                                               sourceAddr,
                                               interfaceIndex,
                                               (char*) oldLSHeader,
                                               oldLSHeader->length,
                                               1);
                        sendListItem->timeStamp = node->getNodeTime();
                    }
                    else if (sendListItem == NULL)
                    {
                        Ospfv2SendUpdatePacket(node,
                                               sourceAddr,
                                               interfaceIndex,
                                               (char*) oldLSHeader,
                                               oldLSHeader->length,
                                               1);

                        Ospfv2InsertToNeighborSendList(
                            nbrInfo->linkStateSendList,
                            node->getNodeTime(),
                            LSHeader);
                    }
                }
            }
        }
    }

    if (!Ospfv2ListIsEmpty(directAckList))
    {
        Ospfv2SendDirectAck(node,interfaceIndex, directAckList, sourceAddr);
    }

    Ospfv2FreeList(node, directAckList, FALSE);

    if (isLSDBChanged == TRUE)
    {

#ifdef ADDON_BOEINGFCS
        if (RoutingCesRospfActiveOnAnyInterface(node))
        {
            RoutingCesRospfHandleLSDBChange(node);
        }
#endif

        // Calculate shortest path as contents of LSDB has changed.
        Ospfv2ScheduleSPFCalculation(node);

        if (Ospfv2DebugFlood(node))
        {
            NetworkPrintForwardingTable(node);
        }
    }


    if (!Ospfv2ListIsEmpty(nbrInfo->linkStateRequestList))
    {
        // If we've got the desired LSA(s), send next request
        if (Ospfv2RequestedLSAReceived(nbrInfo))
        {
            // Send next LS request
            Ospfv2SendLSRequestPacket(node,
                                      sourceAddr,
                                      interfaceIndex,
                                      FALSE);
        }
    }
    else
    {
        // Cancel retransmission timer
        nbrInfo->LSReqTimerSequenceNumber += 1;

        Ospfv2HandleNeighborEvent(node,
                                  interfaceIndex,
                                  sourceAddr,
                                  E_LoadingDone);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleAckPacket
// PURPOSE      :Removes the LSA being acknowledged from the retx list.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2HandleAckPacket(
    Node*  node,
    Ospfv2LinkStateAckPacket* ackPkt,
    NodeAddress sourceAddr,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

#ifdef ADDON_BOEINGFCS
    if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
    {
        RoutingCesRospfHandleAckPacket(
                                node, ackPkt, sourceAddr, interfaceIndex);
        return;
    }
#endif
    Ospfv2ListItem* listItem = NULL;
    Ospfv2Neighbor* neighborInfo =
        Ospfv2GetNeighborInfoByIPAddress(node, interfaceIndex, sourceAddr);
    Ospfv2LinkStateHeader* LSHeader = NULL;
    int numAck;
    int count;

    // Neighbor no longer exists, so do nothing.
    if (neighborInfo == NULL)
    {
        return;
    }

    if (neighborInfo->state < S_Exchange)
    {
        return;
    }

    numAck = (ackPkt->commonHeader.packetLength
                      - sizeof(Ospfv2LinkStateAckPacket))
                          / sizeof(Ospfv2LinkStateHeader);

    LSHeader = (Ospfv2LinkStateHeader*) (ackPkt + 1);

    for (count = 0; count < numAck; count++, LSHeader = LSHeader + 1)
    {
        listItem = neighborInfo->linkStateRetxList->first;

        while (listItem)
        {
            // If LSA exists in retransmission list
            if (Ospfv2CompareLSAToListItem(
                    node,
                    LSHeader,
                    (Ospfv2LinkStateHeader*) listItem->data))
            {
                Ospfv2RemoveFromList(node,
                                     neighborInfo->linkStateRetxList,
                                     listItem,
                                     FALSE);
                if (Ospfv2DebugFlood(node))
                {
                    char neighborIPStr[MAX_ADDRESS_STRING_LENGTH];
                    char linkStateIDStr[MAX_ADDRESS_STRING_LENGTH];
                    char advertisingRouterStr[MAX_ADDRESS_STRING_LENGTH];

                    IO_ConvertIpAddressToString(
                                neighborInfo->neighborIPAddress,
                                neighborIPStr);

                    printf("    Node %u: Removing LSA with seqno 0x%x from "
                           "rext list of nbr %s\n",
                            node->nodeId, LSHeader->linkStateSequenceNumber,
                            neighborIPStr);

                    IO_ConvertIpAddressToString(LSHeader->linkStateID,
                                                linkStateIDStr);
                    IO_ConvertIpAddressToString(LSHeader->advertisingRouter,
                                                advertisingRouterStr);

                    printf("        Type = %d, ID = %s,"
                            " advRtr = %s, age = %d\n",
                            LSHeader->linkStateType,
                            linkStateIDStr,
                            advertisingRouterStr,
                            LSHeader->linkStateAge);
                }

                break;
            }

            listItem = listItem->next;
        }
    }

    if ((Ospfv2ListIsEmpty(neighborInfo->linkStateRetxList))
        && (neighborInfo->LSRetxTimer == TRUE))
    {
        neighborInfo->LSRetxTimer = FALSE;
        neighborInfo->rxmtTimerSequenceNumber++;
    }
}


//-------------------------------------------------------------------------//
//           INTERFACE AND AREA STRUCTURE INITIALIZATION                   //
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// NAME         :Ospfv2GetAreaId()
// PURPOSE      :Get are id from config file for this interface
// ASSUMPTION   :None
// RETURN VALUE :NONE.
//-------------------------------------------------------------------------//

static
void Ospfv2GetAreaId(
    Node* node,
    int interfaceId,
    const NodeInput* ospfConfigFile,
    NodeAddress* areaID)
{
    //fix for #2894 and #3506 start
    char* areaIdString = NULL;
    //fix for #2894 and #3506 end
    char* qualifier = NULL;
    int qualifierLen;
    int i;
    if (!ospfConfigFile)
    {
        // Setting default area id and return
        areaID = OSPFv2_BACKBONE_AREA;
        return;
    }
    qualifier = new char[ospfConfigFile->maxLineLen];

    for (i = 0; i < ospfConfigFile->numLines; i++)
    {
        char* currentLine = ospfConfigFile->inputStrings[i];
        char* aStr = NULL;
        BOOL isNodeId;
        int matchType;

        // Skip lines that are not begining with "["
        if (strncmp(currentLine, "[", 1) != 0)
        {
            continue;
        }

        aStr = strchr(currentLine, ']');
        if (aStr == NULL)
        {
            char* errStr = new char[MAX_STRING_LENGTH + strlen(currentLine)];
            sprintf(errStr, "Unterminated qualifier:\n %s\n", currentLine);
            ERROR_ReportError(errStr);
        }

        qualifierLen = (int)(aStr - currentLine - 1);
        strncpy(qualifier, &currentLine[1], qualifierLen);
        qualifier[qualifierLen] = '\0';

        aStr++;
        if (!isspace(*aStr))
        {
            char* errStr = new char[MAX_STRING_LENGTH + strlen(currentLine)];
            sprintf(errStr, "White space required after qualifier:\n %s\n",
                             currentLine);
            ERROR_ReportError(errStr);
        }
        // skip white space
        while (isspace(*aStr))
        {
            aStr++;
        }

        if (!QualifierMatches(node->nodeId,
                              NetworkIpGetInterfaceAddress(node,
                                                           interfaceId),
                              qualifier,
                              &matchType))
        {
            continue;
        }

        //fix for #2894 and #3506 start
        std::string idString;
        size_t strLen;
        idString.assign(aStr);
        strLen = idString.find("AREA-ID");
        if ((strLen == std::string::npos) || ((int)strLen != 0))
        {
            continue;
        }
        idString = idString.substr(strlen("AREA-ID"));
        areaIdString = (char*) MEM_malloc(idString.length() + 1);
        memset(areaIdString, 0, idString.length());
        strcpy(areaIdString, idString.c_str());
        IO_TrimLeft(areaIdString);
        //fix for #2894 and #3506 end

        IO_ParseNodeIdOrHostAddress(areaIdString, areaID, &isNodeId);

        if (isNodeId)
        {
            ERROR_ReportError("The format is AREA-ID  "
                              "<Area ID in IP address format>\n");
        }
        MEM_free(areaIdString);
        break;
    }
    if (i == ospfConfigFile->numLines)
    {
        char* errStr = new char[ospfConfigFile->maxLineLen];
        char addrStr[MAX_ADDRESS_STRING_LENGTH];
        NodeAddress addr;

        addr = NetworkIpGetInterfaceAddress(node, interfaceId);
        IO_ConvertIpAddressToString(addr, addrStr);

        sprintf(errStr, "Node %u: AreaId not specified on interface %s\n",
            node->nodeId, addrStr);

        ERROR_ReportError(errStr);
    }
    delete[] qualifier;
}


// /**
// FUNCTION   :: Ospfv2InitHostRoutes
// LAYER      :: NETWORK
// PURPOSE    :: To read the host routes from ospfv2 configuration file.
// PARAMETERS ::
// + node : Node* : Pointer to node running OSPFv2 which may have a host as
// its neighbor
// + ospfConfigFile  : NodeInput* : Pointer to OSPF configuration file that
// contains information about hosts connected to OSPF routers
// RETURN ::  void : NULL
// **/

void Ospfv2InitHostRoutes(
     Node *node,
    const NodeInput* ospfConfigFile)
{
    int i;
    int qualifierLen;
    Ospfv2HostRoutes hostRoute;
    NodeAddress address, tempAreaId;
    BOOL isNodeId;
    unsigned int cost;
    char* qualifier = NULL;
    char* parameterName = NULL;
    char* ipAddressString = NULL;
    char* areaIdString = NULL;

    if (!ospfConfigFile)
    {
        return;
    }

    qualifier         = new char[ospfConfigFile->maxLineLen];
    parameterName     = new char[ospfConfigFile->maxLineLen];
    ipAddressString   = new char[ospfConfigFile->maxLineLen];
    areaIdString      = new char[ospfConfigFile->maxLineLen];

    Ospfv2Data* ospf = (Ospfv2Data*)
          NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    for (i = 0; i < ospfConfigFile->numLines; i++)
    {
        char* currentLine = ospfConfigFile->inputStrings[i];
        char* aStr = NULL;

        // Skip lines that are not begining with "["
        if (strncmp(currentLine, "[", 1) != 0)
        {
            continue;
        }

        aStr = strchr(currentLine,']');
        if (aStr == NULL)
        {
            ERROR_ReportErrorArgs("Unterminated qualifier:\n %s\n",
                                  currentLine);
        }

        qualifierLen = (int)(aStr - currentLine - 1);
        strncpy(qualifier, &currentLine[1], qualifierLen);
        qualifier[qualifierLen] = '\0';

        aStr++;
        if (!isspace(*aStr))
        {
            ERROR_ReportErrorArgs("White space required after qualifier:\n"
                                  "%s\n", currentLine);
        }
        // skip white space
        while (isspace(*aStr))
        {
            aStr++;
        }

        if ((unsigned)atoi(qualifier) == node->nodeId)
        {
            sscanf(aStr, "%s", parameterName);
            if (strcmp(parameterName, "HOST-IP") == 0)
            {
                if (sscanf(aStr, "%s %s %u %s", parameterName,
                    ipAddressString, &cost, areaIdString) != 4)
                {
                    ERROR_ReportError("OSPFv2: The format for host routes"
                      " configuration is [<node-Id>] HOST-IP"
                      " <host-ip-address> <cost> <area-Id> \n");
                }

                IO_ConvertStringToNodeAddress(ipAddressString, &address);

                //Note that Area Id for OSPF is specified in
                //dot decimal notation
                IO_ParseNodeIdOrHostAddress(
                            areaIdString,
                            &tempAreaId,
                            &isNodeId);

                if (isNodeId)
                {
                    ERROR_ReportError("OSPFv2: The format for host routes"
                      " configuration is [<node-Id>] [<host-ip-address>]"
                      " [<cost>] [<area-Id>] \n");
                }
                hostRoute.hostIpAddress = address;
                hostRoute.areaId = tempAreaId;
                hostRoute.cost = cost;
                hostRoute.nodeId = node->nodeId;

                ospf->hostRoutes.push_back(hostRoute);
            }
        }
    }
    delete[] qualifier;
    delete[] parameterName;
    delete[] ipAddressString;
    delete[] areaIdString;
}



//-------------------------------------------------------------------------//
// NAME         :Ospfv2InitArea()
// PURPOSE      :Initializes area data structure
// ASSUMPTION   :None.
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2InitArea(
    Node* node,
    const NodeInput* ospfConfigFile,
    int interfaceIndex,
    unsigned int areaID)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Area* newArea = (Ospfv2Area*) MEM_malloc(sizeof(Ospfv2Area));

    if (ospf->area->size == OSPFv2_MAX_ATTACHED_AREA)
    {
        char errStr[MAX_STRING_LENGTH];

        sprintf(errStr, "Node %u attached to more than %d area.\n"
            "Please increase the value of \"OSPFv2_MAX_ATTACHED_AREA\" "
            "in ospfv2.h\n",
            node->nodeId, OSPFv2_MAX_ATTACHED_AREA);

        ERROR_ReportError(errStr);
    }

    newArea->areaID = areaID;
    Ospfv2InitList(&newArea->areaAddrRange);
    Ospfv2InitList(&newArea->connectedInterface);
    Ospfv2InitList(&newArea->routerLSAList);
    Ospfv2InitList(&newArea->networkLSAList);
    Ospfv2InitList(&newArea->routerSummaryLSAList);
    Ospfv2InitList(&newArea->networkSummaryLSAList);
    Ospfv2InitList(&newArea->groupMembershipLSAList);
    Ospfv2InitList(&newArea->maxAgeLSAList);

    newArea->routerLSTimer = FALSE;
    newArea->routerLSAOriginateTime = (clocktype) 0;
    newArea->groupMembershipLSTimer = FALSE;
    newArea->groupMembershipLSAOriginateTime = (clocktype) 0;
    newArea->transitCapability = FALSE;
    newArea->externalRoutingCapability = TRUE;
    newArea->stubDefaultCost = 0;

    // Mtree Fix 13 April Start
    newArea->isNSSAEnabled = FALSE;
    newArea->isNSSANoSummary = FALSE;
    newArea->isNSSANoRedistribution = FALSE;
    // Mtree Fix 13 April End

    // If entire domain defined as single area
    if (!ospfConfigFile)
    {
        ospf->areaBorderRouter = FALSE;

        // Make a link to connected interface structure
        Ospfv2InsertToList(newArea->connectedInterface,
                           0,
                           &ospf->iface[interfaceIndex]);

        // Insert area structure to list
        Ospfv2InsertToList(ospf->area, 0, newArea);

        return;
    }
    else
    {
        // Process each line of input file
        for (int i = 0; i < ospfConfigFile->numLines; i++)
        {
            char* currentLine = ospfConfigFile->inputStrings[i];
            char* areaIdString = new char[ospfConfigFile->maxLineLen];
            BOOL isNodeId;
            unsigned int currentAreaID;
            char* parameterName = new char[ospfConfigFile->maxLineLen];

            // Skip lines that are not begining with "AREA" keyword
            if (strncmp(currentLine, "AREA ", 5) != 0)
            {
                delete[] parameterName;
                delete[] areaIdString;
                continue;
            }

            sscanf(currentLine, "%*s %s %s", areaIdString, parameterName);

            IO_ParseNodeIdOrHostAddress(
                areaIdString,
                &currentAreaID,
                &isNodeId);

            // Skip lines that are not for this area
            if (currentAreaID != areaID)
            {
                delete[] parameterName;
                delete[] areaIdString;
                continue;
            }

            // Get address range
            if (strcmp(parameterName, "RANGE") == 0)
            {
                // For IO_GetDelimitedToken
                char* next;
                char* ptr = NULL;
                const char* delims = "{,} \t\n";
                char* token = NULL;
                char* iotoken = new char[ospfConfigFile->maxLineLen];

                char* areaRangeString = NULL;
                char* asIdStr;

                ptr = strchr(currentLine,'{');
                if (ptr == NULL)
                {
                    areaRangeString =
                        (char*) MEM_malloc(sizeof(char) *
                                           (ospfConfigFile->maxLineLen +
                                            MAX_STRING_LENGTH));

                    sprintf(areaRangeString,
                        "Could not find '{' character:\n"
                        "%s\n", currentLine);
                    ERROR_ReportError(areaRangeString);
                }

                asIdStr = strchr(currentLine, '}');
                if (asIdStr == NULL)
                {
                    areaRangeString =
                        (char*) MEM_malloc(sizeof(char) *
                                           (ospfConfigFile->maxLineLen +
                                            MAX_STRING_LENGTH));
                    sprintf(areaRangeString,
                        "Could not find '}' character:\n"
                        "%s\n", currentLine);
                    ERROR_ReportError(areaRangeString);
                }
                else
                {
                    unsigned int asID = 0;
                    int retVal = 0;

                    asIdStr += 1;

                    retVal = sscanf(asIdStr, "%u", &asID);

                    if (retVal && asID != ospf->asID)
                    {
                        delete[] parameterName;
                        delete[] areaIdString;
                        delete[] iotoken;
                        continue;
                    }
                }

                areaRangeString = (char*) MEM_malloc(asIdStr - ptr + 1);
                memset(areaRangeString, 0, asIdStr - ptr);

                strncpy(areaRangeString, ptr, asIdStr - ptr);
                areaRangeString[asIdStr - ptr] = '\0';

                token = IO_GetDelimitedToken(
                            iotoken, areaRangeString, delims, &next);

                if (!token)
                {
                    sprintf(areaRangeString,
                        "Cann't find subnet list, e.g. {N8-1.0,"
                        " N2-2.0, .. }:\n%s\n", currentLine);
                    ERROR_ReportError(areaRangeString);
                }

                while (TRUE)
                {
                    Ospfv2AreaAddressRange* newData = NULL;
                    NodeAddress networkAddr;
                    int numHostBits;
                    int k;

                    IO_ParseNetworkAddress(
                        token, &networkAddr, &numHostBits);

                    newData = (Ospfv2AreaAddressRange*)
                        MEM_malloc(sizeof(Ospfv2AreaAddressRange));

                    newData->address = networkAddr;
                    newData->mask = ConvertNumHostBitsToSubnetMask(
                                        numHostBits);
                    newData->advertise = TRUE;

                    // Link back to main area structure
                    newData->area = (void*) newArea;

                    for (k = 0; k < OSPFv2_MAX_ATTACHED_AREA; k++)
                    {
                        newData->advrtToArea[k] = FALSE;
                    }

                    Ospfv2InsertToList(newArea->areaAddrRange,0, newData);

                    // Retrieve next token.
                    token = IO_GetDelimitedToken(
                                            iotoken, next, delims, &next);

                    if (!token)
                    {
                        break;
                    }
                } //while
                delete[] iotoken;
                MEM_free(areaRangeString);
            }
            // Check whether this area has been configured as stub area
            else if (strcmp(parameterName, "STUB") == 0)
            {
                Int32 stubDefaultCost;
                unsigned int asID;
                int numParameters= 0;

                if (newArea->areaID == OSPFv2_BACKBONE_AREA)
                {
                    ERROR_Assert(FALSE, "Backbone area can not be "
                                        "configured as STUB\n");
                }
                // Get stub default cost
                numParameters = sscanf(currentLine, "%*s %*s %*s %d %u",
                                        &stubDefaultCost, &asID);

                if (numParameters == 1 || asID == ospf->asID)
                {
                    newArea->externalRoutingCapability = FALSE;
                    newArea->stubDefaultCost = stubDefaultCost;
                }
            }
            // Check whether this area has been configured as NSSA stub area
            else if (strcmp(parameterName, "NSSA") == 0)
            {
                Int32 stubDefaultCost;
                unsigned int asID;
                int numParameters = 0;
                char* nssaParameter = new char[ospfConfigFile->maxLineLen];

                if (!(newArea->isNSSAEnabled))
                {
                    numParameters = sscanf(currentLine, "%*s %*s %*s %d %u",
                                            &stubDefaultCost, &asID);
                    if (numParameters == 1 || asID == ospf->asID)
                    {
                        newArea->externalRoutingCapability = FALSE;
                        newArea->stubDefaultCost = stubDefaultCost;
                        newArea->isNSSAEnabled = TRUE;
                        ospf->nssaCandidateNode = TRUE;
                    }
                }
                else
                {
                    numParameters = sscanf(currentLine, "%*s %*s %*s %s",
                                            nssaParameter);
                    if (strcmp(nssaParameter, "NO-SUMMARY") == 0)
                    {
                        newArea->isNSSANoSummary = TRUE;
                    }
                    else if (strcmp(nssaParameter, "NO-REDISTRIBUTION") == 0)
                    {
                        newArea->isNSSANoRedistribution = TRUE;
                    }
                    else
                    {
#ifdef JNE_LIB
                        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                            "Invalid NSSA paramter.",
                            JNE::CRITICAL);
#endif
                        ERROR_Assert(FALSE, "Invalid NSSA paramter.");
                    }
                }
                delete[] nssaParameter;
            }
            delete[] parameterName;
            delete[] areaIdString;

        } //for

        // Make a link to connected interface structure
        Ospfv2InsertToList(newArea->connectedInterface,
                           0,
                           &ospf->iface[interfaceIndex]);

        // Insert area structure to list
        Ospfv2InsertToList(ospf->area, 0, newArea);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2AddInterfaceToArea()
// PURPOSE      :Add new interface to area structure.
// ASSUMPTION   :None.
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2AddInterfaceToArea(
    Node* node,
    unsigned int areaID,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem = NULL;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv2Area* areaInfo = (Ospfv2Area*) listItem->data;

        if (areaInfo->areaID == areaID)
        {
            // Make a link to connected interface structure
            Ospfv2InsertToList(areaInfo->connectedInterface,
                               0,
                               &ospf->iface[interfaceIndex]);

            break;
        }
    }
#ifdef JNE_LIB
    JneWriteToLogFile(node, JNE::M_OSPF,"Error",
        "Unable to add interface. Area doesn't exist.",
        JNE::CRITICAL);
#endif
    ERROR_Assert(listItem, "Unable to add interface. Area doesn't exist.\n");
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2ReadRouterInterfaceParameters()
// PURPOSE      :Read user specified interfacerelated parameters from file.
// ASSUMPTION   :None.
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2ReadRouterInterfaceParameters(
    Node* node,
    Ospfv2Interface* iface,
    int interfaceIndex,
    const NodeInput* ospfConfigFile,
    const NodeInput* nodeInput)
{
    if (!ospfConfigFile)
    {
        return;
    }
    int qualifierLen;
    int i;
    char* errStr = new char[MAX_STRING_LENGTH + ospfConfigFile->maxLineLen];
    char* valStr = new char[ospfConfigFile->maxLineLen];
    char* qualifier = new char[ospfConfigFile->maxLineLen];
    char* parameterName = new char[ospfConfigFile->maxLineLen];


    // Process each line of input file
    for (i = 0; i < ospfConfigFile->numLines; i++)
    {
        char* currentLine = ospfConfigFile->inputStrings[i];
        char* aStr = NULL;
        int matchType;

        // Skip lines that are not begining with "["
        if (strncmp(currentLine, "[", 1) != 0)
        {
            continue;
        }

        aStr = strchr(currentLine, ']');
        if (aStr == NULL)
        {
            sprintf(errStr, "Unterminated qualifier:\n %s\n", currentLine);
            ERROR_ReportError(errStr);
        }

        qualifierLen = (int)(aStr - currentLine - 1);
        strncpy(qualifier, &currentLine[1], qualifierLen);
        qualifier[qualifierLen] = '\0';

        aStr++;
        if (!isspace(*aStr))
        {
            sprintf(errStr, "White space required after qualifier:\n %s\n",
                             currentLine);
            ERROR_ReportError(errStr);
        }
        // skip white space
        while (isspace(*aStr))
        {
            aStr++;
        }

        if (!QualifierMatches(node->nodeId,
                              iface->address,
                              qualifier,
                              &matchType))
        {
            continue;
        }

        sscanf(aStr, "%s %s", parameterName, valStr);

        if (!strcmp(parameterName, "INTERFACE-COST"))
        {
            int outputCost = atoi(valStr);

            if (outputCost <= 0 || outputCost > 65535)
            {
               sprintf(errStr, "OSPF interface Init:\n     "
               "Interface output cost should be between 0 and 65536:\n %s\n",
                    currentLine);
                ERROR_ReportError(errStr);
            }
            iface->outputCost = outputCost;
        }
        else if (!strcmp(parameterName, "RXMT-INTERVAL"))
        {
            iface->rxmtInterval = TIME_ConvertToClock(valStr);

            if (iface->rxmtInterval <= 0)
            {
                sprintf(errStr, "OSPF interface Init:\n     "
                    "Retransmission interval should be greater than 0:\n "
                    "%s\n",
                    currentLine);
                ERROR_ReportError(errStr);
            }
        }
        else if (!strcmp(parameterName, "INF-TRANS-DELAY"))
        {
            iface->infTransDelay = TIME_ConvertToClock(valStr);

            if (iface->infTransDelay <= 0)
            {
                sprintf(errStr, "OSPF interface Init:\n     "
                    "InfTransDelay should be greater than 0:\n %s\n",
                    currentLine);
                ERROR_ReportError(errStr);
            }
        }
        else if (!strcmp(parameterName, "ROUTER-PRIORITY"))
        {
            iface->routerPriority = atoi(valStr);

            if (iface->routerPriority < 0)
            {
                sprintf(errStr, "OSPF interface Init:\n     "
                    "Router priority should be >= 0:\n %s\n",
                    currentLine);
                ERROR_ReportError(errStr);
            }
        }
        else if (!strcmp(parameterName, "HELLO-INTERVAL"))
        {
            iface->helloInterval = TIME_ConvertToClock(valStr);

            if (iface->helloInterval <= 0)
            {
                sprintf(errStr, "OSPF interface Init:\n     "
                    "Hello interval should be greater than 0:\n %s\n",
                    currentLine);
                ERROR_ReportError(errStr);
            }
            if (iface->helloInterval < 1 * SECOND
                            || iface->helloInterval > 0xffff * SECOND)
            {

                sprintf(errStr, "Permissible values for OSPF"
                              " Interface Hello Interval are "
                                      "in the range of 1-65535s.");
                ERROR_ReportWarning(errStr);

                sprintf(errStr, "Invalid Interface Hello Interval"
                               " Parameter specified, hence continuing"
                               " with the default"
                               " value: %" TYPES_64BITFMT "d\n.",
                                 OSPFv2_HELLO_INTERVAL / SECOND);
               ERROR_ReportWarning(errStr);

               iface->helloInterval = OSPFv2_HELLO_INTERVAL;
            }
        }
        else if (!strcmp(parameterName, "ROUTER-DEAD-INTERVAL"))
        {
            iface->routerDeadInterval = TIME_ConvertToClock(valStr);

            if (iface->routerDeadInterval <= 0)
            {
                sprintf(errStr, "OSPF interface Init:\n     "
                    "Router dead interval should be greater than 0:\n %s\n",
                    currentLine);
                ERROR_ReportError(errStr);
            }

            if (iface->routerDeadInterval < 1 * SECOND
                            || iface->routerDeadInterval > 0xffff * SECOND)
            {
                sprintf(errStr, "Permissible values for OSPF"
                                    " Interface Router Dead Interval are "
                                    "in the range of 1-65535s.");
                ERROR_ReportWarning(errStr);

                sprintf(errStr, "Invalid Interface Router Dead "
                                    "Interval Parameter specified, "
                                    "hence continuing with the default value"
                                    " (4 * hello interval)"
                                    " : %" TYPES_64BITFMT "d.",
                                (4 * iface->helloInterval) / SECOND);
                ERROR_ReportWarning(errStr);

                iface->routerDeadInterval = 4 * iface->helloInterval;
            }
        }
        else if (!strcmp(parameterName, "INTERFACE-TYPE"))
        {
            if (!strcmp(valStr, "BROADCAST"))
            {
                iface->type = OSPFv2_BROADCAST_INTERFACE;
            }
            else if (!strcmp(valStr, "POINT-TO-POINT"))
            {
                iface->type = OSPFv2_POINT_TO_POINT_INTERFACE;
            }
            else if (!strcmp(valStr, "POINT-TO-MULTIPOINT"))
            {
                iface->type = OSPFv2_POINT_TO_MULTIPOINT_INTERFACE;
            }
#ifdef ADDON_BOEINGFCS
            else if (!strcmp(valStr, "ROUTING-CES-ROSPF"))
            {
                iface->type = OSPFv2_ROSPF_INTERFACE;
                RoutingCesRospfInit(node, ospfConfigFile, interfaceIndex);
#ifdef CYBER_CORE
                NetworkSecurityCesHaipeInit(node, nodeInput);
#endif // CYBER_CORE

                iface->drCost = 0;
            }
#endif
            else
            {
                sprintf(errStr, "OSPF interface Init:\n     "
                    "Specified interface type is not supported or unknown:\n"
                    " %s\n",
                    currentLine);
                ERROR_ReportError(errStr);
            }
        }
        else if (!strcmp(parameterName, "NEIGHBOR"))
        {
            char* endOfList;
            char* next;
            char* token;
            char delims[] = "{,} \n\t";
            char* iotoken = new char[ospfConfigFile->maxLineLen];
            NodeAddress nodeAddr;
            aStr = strchr(currentLine, '{');
            if (aStr == NULL)
            {
                sprintf(errStr, "No neighbors defined:\n %s\n", currentLine);
                ERROR_ReportError(errStr);
            }
            endOfList = strchr(aStr, '}');
            if (endOfList != NULL)
                endOfList[0] = '\0';
            token = IO_GetDelimitedToken(iotoken, aStr, delims, &next);
            while (token)
            {
                int numHostBits = 0;
                BOOL isNodeId = FALSE;
                IO_ParseNodeIdHostOrNetworkAddress(
                                token, &nodeAddr, &numHostBits, &isNodeId);
                char* errStr = new char[MAX_STRING_LENGTH +
                                        ospfConfigFile->maxLineLen];
                sprintf(errStr, "Invalid NEIGHBOR %s", token);
#ifdef JNE_LIB
                if (numHostBits || isNodeId)
                {
                    JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                                      errStr, JNE::CRITICAL);
                }
#endif
                ERROR_Assert(!numHostBits && !isNodeId, errStr);
                Ospfv2InsertToNonBroadcastNeighborList(
                    iface->NonBroadcastNeighborList,
                    nodeAddr);
                token = IO_GetDelimitedToken(iotoken, next, delims, &next);
                delete[] errStr;
            }
            delete[] iotoken;
        }
        else if (!strcmp(parameterName, "OSPFv2-DEMAND-CIRCUIT-INTERFACE"))
        {
            if (strcmp(valStr, "YES") == 0)
            {
                iface->ospfIfDemand = TRUE;
            }
            else if (strcmp(valStr, "NO") == 0)
            {
                iface->ospfIfDemand = FALSE;
            }
            else
            {
                sprintf(errStr, "OSPF interface Init:\n     "
                    "Unknown parameter value:\n"
                    " %s\n",
                    currentLine);
                ERROR_ReportError(errStr);
            }
        }
        else
        {
            continue;
        }
    }

    if (iface->routerDeadInterval < iface->helloInterval)
    {
        sprintf(errStr, "Node %d, OSPF interface Init:\n     "
            "Router dead interval should some multiple of hello interval\n"
            "     Hence continuing with the default value "
            "(4 * hello interval): %" TYPES_64BITFMT "d\n",
            node->nodeId,
            (4 * iface->helloInterval) / SECOND);
        ERROR_ReportWarning(errStr);

        iface->routerDeadInterval = 4 * iface->helloInterval;
    }
    delete[] errStr;
    delete[] valStr;
    delete[] qualifier;
    delete[] parameterName;
}

#ifdef ADDON_NGCNMS
static
void Ospfv2ReInitInterface(
    Node* node,
    const NodeInput* ospfConfigFile,
    const NodeInput* nodeInput,
    int interfaceIndex,
    BOOL rospf)
{

    Ospfv2Data* ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                        node, ROUTING_PROTOCOL_OSPFv2);
#ifdef JNE_LIB
    if (NetworkIpGetInterfaceType(node, interfaceIndex) != NETWORK_IPV4)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Currently, OSPFv2 can only run on IPv4 only node", JNE::CRITICAL);
    }
#endif

    ERROR_Assert(
        NetworkIpGetInterfaceType(node, interfaceIndex) == NETWORK_IPV4,
            "Currenly, OSPFv2 can only run on IPv4 only node\n");

    // Used to print statistics only once during finalization.  In
    // the future, stats should be tied to each interface.
    ospf->stats.alreadyPrinted = FALSE;

    BOOL retVal;
    char protocolString[MAX_STRING_LENGTH];

    IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "ROUTING-PROTOCOL",
            &retVal,
            protocolString);
#ifdef ADDON_BOEINGFCS
        if (!retVal || (strcmp(protocolString, "OSPFv2")
                    && strcmp(protocolString, "ROUTING-CES-ROSPF")))
#else
        if (!retVal || (strcmp(protocolString, "OSPFv2")))
#endif
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "MULTICAST-PROTOCOL",
                &retVal,
                protocolString);

            if (!retVal || strcmp(protocolString, "MOSPF"))
            {
                ospf->iface[interfaceIndex].type = OSPFv2_NON_OSPF_INTERFACE;

                /***** Start: OPAQUE-LSA *****/
                ospf->iface[i].interfaceIndex = i;
                ospf->iface[i].address = NetworkIpGetInterfaceAddress(node, i);
                ospf->iface[i].subnetMask =
                                        NetworkIpGetInterfaceSubnetMask(node, i);
                /***** End: OPAQUE-LSA *****/
                return;
            }
        }

        // Determine network type.
        // The default operation mode of OSPF is OSPFv2_BROADCAST_INTERFACE
        // or OSPFv2_POINT_TO_POINT_INTERFACE depending on underlying
        // media type.
        if (MAC_IsOneHopBroadcastNetwork(node, interfaceIndex))
        {
            ospf->iface[interfaceIndex].type = OSPFv2_BROADCAST_INTERFACE;
        }
        else
        {
            ospf->iface[interfaceIndex].type =
                                        OSPFv2_POINT_TO_POINT_INTERFACE;
        }

        ospf->iface[interfaceIndex].address = NetworkIpGetInterfaceAddress
            (node, interfaceIndex);
        ospf->iface[interfaceIndex].subnetMask =
                                    NetworkIpGetInterfaceSubnetMask
                                    (node, interfaceIndex);

        ospf->iface[interfaceIndex].helloInterval = OSPFv2_HELLO_INTERVAL;
        ospf->iface[interfaceIndex].routerDeadInterval =
                                    OSPFv2_ROUTER_DEAD_INTERVAL;

        ospf->iface[interfaceIndex].infTransDelay = OSPFv2_INF_TRANS_DELAY;

        // Set default router priority.
        ospf->iface[interfaceIndex].routerPriority = 1;

        ospf->iface[interfaceIndex].outputCost = 1;
        ospf->iface[interfaceIndex].ospfIfDemand = FALSE;
        ospf->iface[interfaceIndex].helloSuppressionSuccess = FALSE;
        ospf->iface[interfaceIndex].rxmtInterval = OSPFv2_RXMT_INTERVAL;

        ospf->iface[interfaceIndex].includeSubnetRts = TRUE;

        IO_ReadBool(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OSPFv2-INCLUDE-SUBNET-ROUTES",
            &retVal,
            &(ospf->iface[interfaceIndex].includeSubnetRts));
#ifdef ADDON_BOEINGFCS

// special cost case when OSPF in HNW network
NetworkCesUpdateOspfCostInHnw(node, interfaceIndex);

#endif

        //Check for interface configurable parameter
        if (ospfConfigFile)
        {

            Ospfv2ReadRouterInterfaceParameters(
                node, &ospf->iface[interfaceIndex], interfaceIndex,
                ospfConfigFile, nodeInput);

            if (!strcmp(protocolString, "ROUTING-CES-ROSPF"))
            {
                char* errStr = new char[MAX_STRING_LENGTH +
                                       ospfConfigFile->maxLineLen];
                sprintf(errStr, "Interface %x should be ROSPF interface\n",
                        ospf->iface[interfaceIndex].address);
#ifdef JNE_LIB
                if (ospf->iface[interfaceIndex].type !=
                    OSPFv2_ROSPF_INTERFACE)
                {
                    JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                                      errStr, JNE::CRITICAL);
                }
#endif
                ERROR_Assert(ospf->iface[interfaceIndex].type ==
                    OSPFv2_ROSPF_INTERFACE,
                             errStr);
                delete[] errStr;
            }
        }

#ifdef ADDON_BOEINGFCS
        if (!ospfConfigFile)
        {
            if (!strcmp(protocolString, "ROUTING-CES-ROSPF"))
            {
                ospf->iface[interfaceIndex].type = OSPFv2_ROSPF_INTERFACE;
                RoutingCesRospfInit(node, ospfConfigFile, interfaceIndex);
#ifdef CYBER_CORE
                NetworkSecurityCesHaipeInit(node, nodeInput);
#endif // CYBER_CORE

                ospf->partitionedIntoArea = FALSE;
            }
        }

        RospfCheckDeadInterval(node,
                           interfaceIndex,
                           ospf->iface[interfaceIndex].routerDeadInterval);
#endif



        // Initializes area data structure, if area is enabled
        if (ospf->partitionedIntoArea == TRUE)
        {
            NodeAddress areaID;

            // Get Area ID associated with this interface
            Ospfv2GetAreaId(node, interfaceIndex, ospfConfigFile, &areaID);

            ospf->iface[interfaceIndex].areaId = areaID;

            if (!Ospfv2GetArea(node, areaID))
            {
                Ospfv2InitArea(node, ospfConfigFile, interfaceIndex, areaID);
            }
            else
            {
                Ospfv2AddInterfaceToArea(node, areaID, interfaceIndex);
            }
        }
        else
        {
            // Consider the routing domain as single area
            ospf->iface[interfaceIndex].areaId = OSPFv2_BACKBONE_AREA;

            if (!Ospfv2GetArea(node, OSPFv2_BACKBONE_AREA))
            {
                Ospfv2InitArea(node,
                               ospfConfigFile,
                               interfaceIndex,
                               OSPFv2_BACKBONE_AREA);
            }
            else
            {
                Ospfv2AddInterfaceToArea(
                                node, OSPFv2_BACKBONE_AREA, interfaceIndex);
            }
        }

        // Set initial interface state
        if (MAC_InterfaceIsEnabled(node, interfaceIndex))
        {
            if (ospf->iface[interfaceIndex].type ==
                    OSPFv2_BROADCAST_INTERFACE ||
                ospf->iface[interfaceIndex].type ==
                    OSPFv2_ROSPF_INTERFACE)
            {
                Message* waitTimerMsg;
                clocktype delay;

                ospf->iface[interfaceIndex].state = S_Waiting;

                // Send wait timer to self
                waitTimerMsg = MESSAGE_Alloc(node,
                                             NETWORK_LAYER,
                                             ROUTING_PROTOCOL_OSPFv2,
                                             MSG_ROUTING_OspfWaitTimer);

                MESSAGE_SetInstanceId(waitTimerMsg, interfaceIndex);

                MESSAGE_InfoAlloc(node, waitTimerMsg, sizeof(int));

                memcpy(MESSAGE_ReturnInfo(waitTimerMsg), &interfaceIndex,
                    sizeof(int));

                // Use jitter to avoid synchrinisation
                delay = (clocktype) (RANDOM_erand (ospf->seed)
                            * OSPFv2_BROADCAST_JITTER
                        + ospf->iface[interfaceIndex].routerDeadInterval);

                ospf->iface[interfaceIndex].waitTimerMsg = waitTimerMsg;

                MESSAGE_Send(
                    node, waitTimerMsg, delay + ospf->staggerStartTime);
            }
            else
            {
                ospf->iface[interfaceIndex].state = S_PointToPoint;
            }

            // We may need to produce a new instance of router LSA
            Ospfv2ScheduleRouterLSA(node, ospf->iface[interfaceIndex].areaId,
                FALSE);
        }
        else
        {
            ospf->iface[interfaceIndex].state = S_Down;
        }

        Ospfv2InitList(&ospf->iface[interfaceIndex].updateLSAList);
        Ospfv2InitList(&ospf->iface[interfaceIndex].delayedAckList);
        Ospfv2InitList(&ospf->iface[interfaceIndex].neighborList);

        // Q-OSPF Patch Start
        // Initialize queue status and last advertised bandwidth for Q-OSPF
        ListInit(node, &ospf->iface[interfaceIndex].presentStatusOfQueue);
        ospf->iface[interfaceIndex].lastAdvtUtilizedBandwidth = 0;
        // Q-OSPF Patch End

    if (ospf->partitionedIntoArea && ospf->areaBorderRouter
        && !ospf->backboneArea)
    {
        ERROR_ReportError("Node connected to multiple area must have "
                            "an interface to backbone\n");
    }
}
#endif

//-------------------------------------------------------------------------//
// NAME: Ospfv2InitInterface
// PURPOSE: Initialize the available network interface.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2InitInterface(
    Node* node,
    const NodeInput* ospfConfigFile,
    const NodeInput* nodeInput,
    BOOL rospf)
{
    Ospfv2Data* ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                        node, ROUTING_PROTOCOL_OSPFv2);
    BOOL retVal;
    int i;

    ospf->iface = (Ospfv2Interface*)
                          MEM_malloc(sizeof(Ospfv2Interface)
                          * node->numberInterfaces);
    memset(ospf->iface,
           0,
           sizeof(Ospfv2Interface) * node->numberInterfaces);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (NetworkIpGetInterfaceType(node, i) == NETWORK_IPV6)
        {
            continue;
        }

        NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
        if (ip->interfaceInfo[i]->routingProtocolType !=
                            ROUTING_PROTOCOL_OSPFv2)
        {
            ospf->iface[i].type = OSPFv2_NON_OSPF_INTERFACE;

            /***** Start: OPAQUE-LSA *****/
            ospf->iface[i].interfaceIndex = i;
            ospf->iface[i].address = NetworkIpGetInterfaceAddress(node, i);
            ospf->iface[i].subnetMask =
                                    NetworkIpGetInterfaceSubnetMask(node, i);
            /***** End: OPAQUE-LSA *****/

            continue;
        }

#ifdef ADDON_BOEINGFCS

        char* protocolString;
        if (ospfConfigFile)
        {
            protocolString = new char[ospfConfigFile->maxLineLen];
        }
        else
        {
            protocolString = new char[MAX_STRING_LENGTH];
        }

        if (NetworkIpGetInterfaceType(node, i) == NETWORK_IPV4
            || NetworkIpGetInterfaceType(node, i) == NETWORK_DUAL)
        {
            BOOL retVal;

            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, i),
                nodeInput,
                "ROUTING-PROTOCOL",
                &retVal,
                protocolString);
        }

#endif

        // Set interface based stats to 0.
        Ospfv2Interface* thisIntf = NULL;
        thisIntf = &ospf->iface[i];
        memset(&ospf->iface[i].interfaceStats,
            0,
            sizeof(Ospfv2Stats));

        if (ip->interfaceInfo[i]->isVirtualInterface
            || ip->interfaceInfo[i]->isUnnumbered)
        {
            ospf->iface[i].isVirtualInterface = TRUE;
        }

        // Determine network type.
        // The default operation mode of OSPF is OSPFv2_BROADCAST_INTERFACE
        // or OSPFv2_POINT_TO_POINT_INTERFACE depending on underlying
        // media type.
        if (ospf->iface[i].isVirtualInterface)
        {
#ifdef ADDON_BOEINGFCS
#ifdef CYBER_CORE
            // If we are a red node with a grey interface running ROSPF, do not
            // set this to OSPF.  Set it to non OSPF.
            RoutingCesRospfData* rospfData = ip->rospfData;
            ospf->iface[i].type = OSPFv2_POINT_TO_POINT_INTERFACE;
            if (rospfData != NULL && ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
            {
                int j;
                for (j = 0; j < node->numberInterfaces; j++)
                {
                    if (j != i && IsIAHEPRedSecureInterface(node, j))
                    {
                        if (RoutingCesRospfActiveOnInterface(node,j))
                        {
                            ospf->iface[i].type = OSPFv2_NON_OSPF_INTERFACE;
                        }
                        break;
                    }
                }
            }
#endif // CYBER_CORE
#else // ADDON_BOEINGFCS
            ospf->iface[i].type = OSPFv2_POINT_TO_POINT_INTERFACE;
#endif // ADDON_BOEINGFCS
        }
        else if (MAC_IsOneHopBroadcastNetwork(node, i))
        {
            ospf->iface[i].type = OSPFv2_BROADCAST_INTERFACE;
        }
        else if (MAC_IsPointToMultiPointNetwork(node, i))
        {
            ospf->iface[i].type = OSPFv2_POINT_TO_MULTIPOINT_INTERFACE;
        }
        else
        {
            ospf->iface[i].type = OSPFv2_POINT_TO_POINT_INTERFACE;
        }

        ospf->iface[i].interfaceIndex = i;
        ospf->iface[i].address = NetworkIpGetInterfaceAddress(node, i);
        ospf->iface[i].subnetMask =
                                    NetworkIpGetInterfaceSubnetMask(node, i);

        ospf->iface[i].helloInterval = OSPFv2_HELLO_INTERVAL;
        ospf->iface[i].routerDeadInterval =
                                    OSPFv2_ROUTER_DEAD_INTERVAL;

        ospf->iface[i].infTransDelay = OSPFv2_INF_TRANS_DELAY;

        // Set default router priority.
        ospf->iface[i].routerPriority = 1;

        ospf->iface[i].outputCost = 1;
        ospf->iface[i].ospfIfDemand = FALSE;
        ospf->iface[i].helloSuppressionSuccess = FALSE;
        ospf->iface[i].rxmtInterval = OSPFv2_RXMT_INTERVAL;
        Ospfv2InitNonBroadcastNeighborList(
                        &ospf->iface[i].NonBroadcastNeighborList);
        ospf->iface[i].includeSubnetRts = TRUE;

        IO_ReadBool(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, i),
            nodeInput,
            "OSPFv2-INCLUDE-SUBNET-ROUTES",
            &retVal,
            &(ospf->iface[i].includeSubnetRts));

        //Check for interface configurable parameter
        if (ospfConfigFile)
        {
            Ospfv2ReadRouterInterfaceParameters(
                node, &ospf->iface[i], i, ospfConfigFile, nodeInput);

#ifdef ADDON_BOEINGFCS

            if (NetworkIpGetInterfaceType(node, i) == NETWORK_IPV4
                || NetworkIpGetInterfaceType(node, i) == NETWORK_DUAL)
            {
                if (!strcmp(protocolString, "ROUTING-CES-ROSPF"))
                {
                    char* errStr = new char[MAX_STRING_LENGTH +
                                            ospfConfigFile->maxLineLen];
                    sprintf(errStr,
                            "Interface %x should be ROSPF interface"
                            " but instead was %d\n",
                            ospf->iface[i].address,
                            ospf->iface[i].type);
#ifdef JNE_LIB
                    if (ospf->iface[i].type !=
                        OSPFv2_ROSPF_INTERFACE)
                    {
                        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                            errStr, JNE::CRITICAL);
                    }
#endif

                    ERROR_Assert(
                        ospf->iface[i].type == OSPFv2_ROSPF_INTERFACE,
                                 errStr);
                    delete[] errStr;
                }
            }
#endif
        }
#ifdef ADDON_BOEINGFCS
        if (!ospfConfigFile)
        {
            if (NetworkIpGetInterfaceType(node, i) == NETWORK_IPV4
                || NetworkIpGetInterfaceType(node, i) == NETWORK_DUAL)
            {
                if (!strcmp(protocolString, "ROUTING-CES-ROSPF"))
                {
                    ospf->iface[i].type = OSPFv2_ROSPF_INTERFACE;
                    RoutingCesRospfInit(node, ospfConfigFile, i);
                    ospf->partitionedIntoArea = FALSE;
#ifdef CYBER_CORE
                    NetworkSecurityCesHaipeInit(node, nodeInput);
#endif // CYBER_CORE
                }
            }
        }
        delete[] protocolString;

        RospfCheckDeadInterval(node,
                           i,
                           ospf->iface[i].routerDeadInterval);
#endif

        // Initializes area data structure, if area is enabled
        if (ospf->partitionedIntoArea == TRUE)
        {
            NodeAddress areaID;

            // Get Area ID associated with this interface
            Ospfv2GetAreaId(node, i, ospfConfigFile, &areaID);

            ospf->iface[i].areaId = areaID;

            if (!Ospfv2GetArea(node, areaID))
            {
                Ospfv2InitArea(node, ospfConfigFile, i, areaID);
            }
            else
            {
                Ospfv2AddInterfaceToArea(node, areaID, i);
            }
        }
        else
        {
            // Consider the routing domain as single area
            ospf->iface[i].areaId = OSPFv2_BACKBONE_AREA;

            if (!Ospfv2GetArea(node, OSPFv2_BACKBONE_AREA))
            {
                Ospfv2InitArea(node,
                               ospfConfigFile,
                               i,
                               OSPFv2_BACKBONE_AREA);
            }
            else
            {
                Ospfv2AddInterfaceToArea(node, OSPFv2_BACKBONE_AREA, i);
            }
        }

        // Set initial interface state
        if (NetworkIpInterfaceIsEnabled(node, i))
        {
#ifndef ADDON_BOEINGFCS
            if (ospf->iface[i].type == OSPFv2_BROADCAST_INTERFACE)
#else
            if (ospf->iface[i].type == OSPFv2_BROADCAST_INTERFACE
                || ospf->iface[i].type == OSPFv2_ROSPF_INTERFACE)
#endif
            {
                Message* waitTimerMsg;
                clocktype delay;

                if (ospf->iface[i].routerPriority == 0)
                    ospf->iface[i].state = S_DROther;
                else
                {
                    ospf->iface[i].state = S_Waiting;

                    // Send wait timer to self
                    waitTimerMsg = MESSAGE_Alloc(node,
                                                 NETWORK_LAYER,
                                                 ROUTING_PROTOCOL_OSPFv2,
                                                 MSG_ROUTING_OspfWaitTimer);

#ifdef ADDON_NGCNMS
                    MESSAGE_SetInstanceId(waitTimerMsg, i);
#endif

                    MESSAGE_InfoAlloc(node, waitTimerMsg, sizeof(int));

                    memcpy(MESSAGE_ReturnInfo(waitTimerMsg),
                           &i,
                           sizeof(int));

                    // Use jitter to avoid synchrinisation
                    delay = (clocktype) (RANDOM_erand (ospf->seed)
                                * OSPFv2_BROADCAST_JITTER
                                + ospf->iface[i].routerDeadInterval);

                    ospf->iface[i].waitTimerMsg = waitTimerMsg;

                    MESSAGE_Send(
                        node, waitTimerMsg, delay + ospf->staggerStartTime);
                }
            }
            else
            {
                ospf->iface[i].state = S_PointToPoint;
            }

            // We may need to produce a new instance of router LSA
            Ospfv2ScheduleRouterLSA(node, ospf->iface[i].areaId, FALSE);
        }
        else
        {
            ospf->iface[i].state = S_Down;
        }

#ifdef JNE_GUI
        /*
         * EVENT: initialize DR state to disabled
         */
        JNEGUI_DisableDesignatedRouterState(node, i);
#endif /* JNE_GUI */


        Ospfv2InitList(&ospf->iface[i].updateLSAList);
        Ospfv2InitList(&ospf->iface[i].delayedAckList);
        Ospfv2InitList(&ospf->iface[i].neighborList);

        // Q-OSPF Patch Start
        // Initialize queue status and last advertised bandwidth for Q-OSPF
        ListInit(node, &ospf->iface[i].presentStatusOfQueue);
        ospf->iface[i].lastAdvtUtilizedBandwidth = 0;
        // Q-OSPF Patch End
    }
    if (ospf->area->size > 1)
    {
        ospf->areaBorderRouter = TRUE;
    }
    else
    {
        ospf->areaBorderRouter = FALSE;
    }

    Ospfv2Area* thisArea = NULL;
    for (i = 0; i < node->numberInterfaces; i++)
    {
        thisArea = Ospfv2GetArea(node, ospf->iface[i].areaId);
        if (thisArea && thisArea->isNSSAEnabled)
        {
            ospf->routerID = NetworkIpGetInterfaceAddress(node, i);
            break;
        }
    }

    Ospfv2ListItem* listItem = NULL;
    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        if ((ospf->backboneArea) && (ospf->nssaAreaBorderRouter))
        {
           break;
        }
        Ospfv2Area* areaInfo = (Ospfv2Area*) listItem->data;

        if ((areaInfo->areaID == OSPFv2_BACKBONE_AREA)
            && (ospf->backboneArea == NULL))
        {
            ospf->backboneArea = areaInfo;
        }
        if ((ospf->areaBorderRouter == TRUE) &&
           (areaInfo->isNSSAEnabled == TRUE) &&
            (ospf->nssaAreaBorderRouter == FALSE))
        {
            ospf->nssaAreaBorderRouter = TRUE;
        }
    }

#ifdef ADDON_BOEINGFCS
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    if (ip->rospfData)
    {
    // When running rospf a non-backbone router is allowed to connect to
    // multiple areas if all but one of the non-backbone areas are mobile
    // leaf areas. We don't have a good way to tell if an area is a mobile
    // leaf area, so don't check.
    }
    else
#endif
    if (ospf->partitionedIntoArea && ospf->areaBorderRouter
        && !ospf->backboneArea)
    {
        char buf[MAX_STRING_LENGTH];
        sprintf(buf, "Node %d connected to multiple areas, must have"
                     " an interface to backbone\n", node->nodeId);

        ERROR_ReportError(buf);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2GetArea()
// PURPOSE      :Get area structure pointer by area ID.
// ASSUMPTION   :None.
// RETURN VALUE :Ospfv2Area*
//-------------------------------------------------------------------------//

Ospfv2Area* Ospfv2GetArea(Node* node, unsigned int areaID)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem = NULL;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv2Area* areaInfo = (Ospfv2Area*) listItem->data;

        if (areaInfo->areaID == areaID)
        {
            return areaInfo;
        }
    }

    return NULL;
}


//-------------------------------------------------------------------------//
//                   ORIGINATE LSA AND MAINTAIN LSDB                       //
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// NAME     :Ospfv2CreateLSHeader()
// PURPOSE  :Create LS Header for newly created LSA.
// RETURN   :BOOL.
//-------------------------------------------------------------------------//

BOOL Ospfv2CreateLSHeader(
    Node* node,
    unsigned int areaId,
    Ospfv2LinkStateType LSType,
    Ospfv2LinkStateHeader* LSHeader,
    Ospfv2LinkStateHeader* oldLSHeader)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Area* thisArea = NULL;

    // BGP-OSPF Patch Start
    if ((LSType != OSPFv2_ROUTER_AS_EXTERNAL) &&
        (LSType != OSPFv2_ROUTER_NSSA_EXTERNAL) &&
        /***** Start: OPAQUE-LSA *****/
        (LSType != OSPFv2_AS_SCOPE_OPAQUE))
        /***** End: OPAQUE-LSA *****/
    {
        thisArea = Ospfv2GetArea(node, areaId);
#ifdef JNE_LIB
        if (!thisArea)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Area doesn't exist", JNE::CRITICAL);
        }
#endif
        ERROR_Assert(thisArea, "Area doesn't exist\n");
    }

    if ((LSType == OSPFv2_ROUTER_AS_EXTERNAL) ||
        (LSType == OSPFv2_ROUTER_NSSA_EXTERNAL) ||
        /***** Start: OPAQUE-LSA *****/
        ((LSType != OSPFv2_AS_SCOPE_OPAQUE) &&
        /***** End: OPAQUE-LSA *****/
        (thisArea->externalRoutingCapability == TRUE)))
    {
        Ospfv2OptionsSetExt(&(LSHeader->options), 1);
    }
    // BGP-OSPF Patch End

    if (ospf->isQosEnabled)
    {
        Ospfv2OptionsSetQOS(&(LSHeader->options), 1);
    }

    if (ospf->isMospfEnable == TRUE)
    {
        Ospfv2OptionsSetMulticast(&(LSHeader->options), 1);
    }

    //RFC:1793::SECTION 2.1. THE OSPF OPTIONS FIELD
    //A router implementing the functionality of demand circuit
    //sets the DC-bit
    //in the Options field of all LSAs that it originates.
    //This is regardless of
    //the LSAs’ LS type, and also regardless of whether
    //the router implements the
    //more substantialmodifications required of demand circuit endpoints
    if (ospf->supportDC == TRUE)
    {
        Ospfv2OptionsSetDC(&(LSHeader->options), 1);
    }
    else
    {
        Ospfv2OptionsSetDC(&(LSHeader->options), 0);
    }

    LSHeader->linkStateType = (char) LSType;
    LSHeader->advertisingRouter = ospf->routerID;

    if (oldLSHeader)
    {
        if ((oldLSHeader->linkStateSequenceNumber ==
                    (signed int) OSPFv2_MAX_SEQUENCE_NUMBER)
            && (Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge)
                   < (OSPFv2_LSA_MAX_AGE / SECOND)))
        {
            // Sequence number reaches the maximum value. We need to
            // flush this instance first before originating any instance.
            if ((oldLSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL) ||
                /***** Start: OPAQUE-LSA *****/
                (oldLSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
                /***** End: OPAQUE-LSA *****/
            {
                Ospfv2FlushLSA(node,
                               (char*) oldLSHeader,
                               OSPFv2_INVALID_AREA_ID);
            }
            else
            {
                Ospfv2FlushLSA(node, (char*) oldLSHeader, areaId);
            }
            return FALSE;
        }
        else if (Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge) ==
                   (OSPFv2_LSA_MAX_AGE / SECOND))
        {
           // Max Age LSA is still in the Retransmission List.
           // New LSA will be originated after removal from list.
            return FALSE;
        }
        LSHeader->linkStateSequenceNumber =
                oldLSHeader->linkStateSequenceNumber + 1;
    }
    else
    {
        LSHeader->linkStateSequenceNumber =
                (signed int) OSPFv2_INITIAL_SEQUENCE_NUMBER;
    }

    LSHeader->linkStateCheckSum = 0x0;

    return TRUE;
}


//-------------------------------------------------------------------------//
// NAME     :Ospfv2AddType1Link()
// PURPOSE  :Add Point-to-Point link.
// RETURN   :None.
//-------------------------------------------------------------------------//

void Ospfv2AddType1Link(
    Node* node,
    int interfaceId,
    Ospfv2Neighbor* tempNbInfo,
    Ospfv2LinkInfo** linkInfo
#ifdef ADDON_BOEINGFCS
    ,NodeAddress nbrAddr,
    int cost
#endif
)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

#ifdef ADDON_BOEINGFCS
    if (tempNbInfo != NULL)
    {
        (*linkInfo)->linkID = tempNbInfo->neighborID;
    }
    else
    {
        (*linkInfo)->linkID = nbrAddr;
    }
#else
    (*linkInfo)->linkID = tempNbInfo->neighborID;
#endif

    if (ospf->iface[interfaceId].isVirtualInterface == TRUE)
    {
        (*linkInfo)->linkData = interfaceId;
    }
    else
    {
        (*linkInfo)->linkData =
                    NetworkIpGetInterfaceAddress(node, interfaceId);
    }

    (*linkInfo)->type = OSPFv2_POINT_TO_POINT;

#ifdef ADDON_BOEINGFCS
    if (RoutingCesRospfActiveOnInterface(node, interfaceId))
    {
        int outputCost = 0;

        if (cost > 0)
        {
            outputCost = cost;
        }
        else
        {
            if (tempNbInfo != NULL)
            {
                outputCost =
                    RoutingCesRospfGetType1LinkCost(
                        node, interfaceId, tempNbInfo->neighborIPAddress);
            }
            else
            {
                outputCost = RoutingCesRospfGetType1LinkCost(node,
                                                             interfaceId,
                                                             nbrAddr);
            }
        }

        if (outputCost <= 0 || outputCost > 65535)
        {
           ERROR_ReportError("Interface output cost should be between 0 and "
               "65535\n");
        }
        (*linkInfo)->metric = (short) outputCost;

    }
    else
    {
#endif
        (*linkInfo)->metric = (short) ospf->iface[interfaceId].outputCost;

#ifdef ADDON_BOEINGFCS
    }
#endif

    if (OSPFv2_DEBUG_LSDBErr)
    {
        char linkIDStr[MAX_ADDRESS_STRING_LENGTH];
        char linkDataStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString((*linkInfo)->linkID,linkIDStr);
        IO_ConvertIpAddressToString((*linkInfo)->linkData,linkDataStr);

        printf("Node %d Adding type 1 link. linkID = %s, linkData %s\n",
                    node->nodeId, linkIDStr,linkDataStr);
    }

    if (ospf->isQosEnabled == TRUE)
    {
        (*linkInfo)->numTOS = (unsigned char)
            (2*  ospf->numQueueAdvertisedForQos);

        // Update the position of the working pointer
        (*linkInfo) = (*linkInfo) + 1;

        // To advertise the QoS related information for the Link
        QospfGetQosInformationForTheLink(node, interfaceId, linkInfo);
    }
    else
    {
        // As Q-OSPF is not enabled, numTOS must be zero
         (*linkInfo)->numTOS = 0;

        // Update the position of the working pointer
        (*linkInfo) = (*linkInfo) + 1;
    }
}


//-------------------------------------------------------------------------//
// NAME     :Ospfv2AddType2Link()
// PURPOSE  :Add link to transit network.
// RETURN   :None.
//-------------------------------------------------------------------------//

void Ospfv2AddType2Link(
    Node* node,
    int interfaceId,
    Ospfv2LinkInfo** linkInfo)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    // This broadcast network is represented by the designated router.
     (*linkInfo)->linkID =
             ospf->iface[interfaceId].designatedRouterIPAddress;
#ifdef ADDON_BOEINGFCS
    if (ospf->iface[interfaceId].type == OSPFv2_ROSPF_INTERFACE)
    {
        if (RoutingCesRospfIsMobileLeafNode(node, interfaceId))
        {
            (*linkInfo)->linkID = NetworkIpGetInterfaceAddress(node,
                interfaceId);
        }
    }
#endif

     (*linkInfo)->linkData = NetworkIpGetInterfaceAddress(node, interfaceId);
     (*linkInfo)->type = OSPFv2_TRANSIT;

#ifdef ADDON_BOEINGFCS
    if (RoutingCesRospfActiveOnInterface(node, interfaceId))
    {
        int outputCost = RoutingCesRospfGetType2LinkCost(node, interfaceId);
        if (outputCost <= 0 || outputCost > 65535)
        {
            char err[MAX_STRING_LENGTH];
           sprintf(err, "Interface output cost should be between 0 and "
                   "65535 but is %d\n", outputCost);
           ERROR_ReportError(err);
        }
        (*linkInfo)->metric = (short) outputCost;

    }
    else
    {
#endif

     (*linkInfo)->metric = (short) ospf->iface[interfaceId].outputCost;

#ifdef ADDON_BOEINGFCS
    }
#endif

    if (OSPFv2_DEBUG_LSDBErr)
    {
        char linkIDStr[MAX_ADDRESS_STRING_LENGTH];
        char linkDataStr[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString((*linkInfo)->linkID,linkIDStr);
        IO_ConvertIpAddressToString((*linkInfo)->linkData,linkDataStr);

        printf("    Adding type 2 link. linkID = %s, linkData %s\n",
                    linkIDStr,linkDataStr);
    }

    if (ospf->isQosEnabled == TRUE)
    {
        // numTOS field is the number of QoS Metrics advertised. In our
        // consideration, we are advertising available bandwidth and
        // average delay of all queue of each interface.
        (*linkInfo)->numTOS = (unsigned char)
            (2 * ospf->numQueueAdvertisedForQos);

        // Update the position of the working pointer
        *linkInfo = *linkInfo + 1;

        // To advertise the QoS related information for the Link
        QospfGetQosInformationForTheLink(node, interfaceId, linkInfo);
    }
    else
    {
        // As Q-OSPF is not enabled, numTOS must be zero
        (*linkInfo)->numTOS = 0;

        // Update the position of the working pointer
        *linkInfo = *linkInfo + 1;
    }
}


//-------------------------------------------------------------------------//
// NAME     :Ospfv2AddType3Link()
// PURPOSE  :Add link to stub network.
// RETURN   :None.
//-------------------------------------------------------------------------//

void Ospfv2AddType3Link(
    Node* node,
    int interfaceId,
    Ospfv2LinkInfo** linkInfo)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    (*linkInfo)->type = OSPFv2_STUB;

    // Changed to follow section 12.4.1, rfc 2328
    switch (ospf->iface[interfaceId].type)
    {
        case OSPFv2_NBMA_INTERFACE:
        case OSPFv2_BROADCAST_INTERFACE:
            (*linkInfo)->linkID =
                NetworkIpGetInterfaceNetworkAddress(node, interfaceId);
            (*linkInfo)->linkData =
                NetworkIpGetInterfaceSubnetMask(node, interfaceId);
            (*linkInfo)->metric = (short) ospf->iface[interfaceId].outputCost;
            break;

        case OSPFv2_POINT_TO_MULTIPOINT_INTERFACE:
            (*linkInfo)->linkID =
                NetworkIpGetInterfaceAddress(node, interfaceId);
            (*linkInfo)->linkData =  ANY_DEST;
            (*linkInfo)->metric = 0;
            break;

#ifdef ADDON_BOEINGFCS
        case OSPFv2_ROSPF_INTERFACE:
            (*linkInfo)->linkID = NetworkIpGetInterfaceAddress(node, interfaceId);
            (*linkInfo)->linkData =  ANY_DEST;
            (*linkInfo)->metric = 0;
            if (RoutingCesRospfActiveOnInterface(node, interfaceId))
            {
                int outputCost =
                    RoutingCesRospfGetType2LinkCost(node, interfaceId);
                if (outputCost <= 0 || outputCost > 65535)
                {
                    ERROR_ReportError("Interface output cost should be "
                        "between 0 and 65535\n");
                }
                (*linkInfo)->metric = (short) outputCost;
            }
            break;
#endif

        default:
            // OSPFv2_POINT_TO_POINT_INTERFACE
            // Need to implement accrding to section 12.4.1.1.
            (*linkInfo)->linkID =
                NetworkIpGetInterfaceNetworkAddress(node, interfaceId);
            (*linkInfo)->linkData =
                NetworkIpGetInterfaceSubnetMask(node, interfaceId);
            (*linkInfo)->metric =
                (short) ospf->iface[interfaceId].outputCost;
            break;
     }

    if (OSPFv2_DEBUG_LSDBErr)
    {
        char linkIDStr[MAX_ADDRESS_STRING_LENGTH];
        char linkDataStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString((*linkInfo)->linkID,linkIDStr);
        IO_ConvertIpAddressToString((*linkInfo)->linkData,linkDataStr);

        printf("    Adding type 3 link. linkID = %s, linkData %s\n",
                    linkIDStr, linkDataStr);
    }

    if (ospf->isQosEnabled == TRUE)
    {
        // numTOS field is the number of QoS Metrics advertised. In our
        // consideration, we are advertising available bandwidth and
        // average delay of all queue of each interface.

        (*linkInfo)->numTOS = (unsigned char)
            (2 *  ospf->numQueueAdvertisedForQos);

        // Update the position of the working pointer
        *linkInfo = *linkInfo + 1;

        // To advertise the QoS related information for the Link
        QospfGetQosInformationForTheLink(node, interfaceId, linkInfo);
    }
    else
    {
        // As Q-OSPF is not enabled, numTOS must be zero
        (*linkInfo)->numTOS = 0;

        // Update the position of the working pointer
        *linkInfo = *linkInfo + 1;
    }
}


//-------------------------------------------------------------------------//
// NAME     :Ospfv2AddHostRoute()
// PURPOSE  :Add single host route.
// RETURN   :None.
//-------------------------------------------------------------------------//

static
void Ospfv2AddHostRoute(
    Node* node,
    int interfaceId,
    Ospfv2Neighbor* tempNbInfo,
    Ospfv2LinkInfo** linkInfo)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    (*linkInfo)->linkID = tempNbInfo->neighborIPAddress;
    (*linkInfo)->linkData = 0xFFFFFFFF;

    (*linkInfo)->type = OSPFv2_STUB;

    (*linkInfo)->metric = (short) ospf->iface[interfaceId].outputCost;

    if (OSPFv2_DEBUG_LSDBErr)
    {
        char linkIDStr[MAX_ADDRESS_STRING_LENGTH];
        char linkDataStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString((*linkInfo)->linkID,linkIDStr);
        IO_ConvertIpAddressToString((*linkInfo)->linkData,linkDataStr);

        printf("    Adding host route. linkID = %s, linkData %s\n",
                    linkIDStr,linkDataStr);
    }

    if (ospf->isQosEnabled == TRUE)
    {
        (*linkInfo)->numTOS = (unsigned char)
            (2 *  ospf->numQueueAdvertisedForQos);

        // Update the position of the working pointer
        (*linkInfo) = (*linkInfo) + 1;

        // To advertise the QoS related information for the Link
        QospfGetQosInformationForTheLink(node, interfaceId, linkInfo);
    }
    else
    {
        // As Q-OSPF is not enabled, numTOS must be zero
         (*linkInfo)->numTOS = 0;

        // Update the position of the working pointer
        (*linkInfo) = (*linkInfo) + 1;
    }
}



// /**
// FUNCTION   :: Ospfv2AddHost()
// LAYER      :: NETWORK
// PURPOSE    :: To add a single host information to router LSA
// PARAMETERS ::
// + node : Node* : Pointer to node running OSPFv2 which has a host as its
// neighbor
// + cost: unsigned int: The cost of the link between the host and the
// corresponding router node
// + hostAddress: NodeAddress: IP address of the host node
// + interfaceId: int: Node's interface index on which OSPF is configured
// + linkInfo: Ospfv2LinkInfo **: Pointer to the list of links for the node's
// router LSA
// + ospfConfigFile  : NodeInput* : Pointer to OSPF configuration file that
// contains information about hosts connected to OSPF routers
// RETURN ::  void : NULL
// **/

static
void Ospfv2AddHost(
    Node* node,
    unsigned int cost,
    NodeAddress hostAddress,
    int interfaceId,
    Ospfv2LinkInfo** linkInfo)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    (*linkInfo)->linkID = hostAddress;
    (*linkInfo)->linkData = 0xFFFFFFFF;

    (*linkInfo)->type = OSPFv2_STUB;

    (*linkInfo)->metric = (short) cost;

    if ((*linkInfo)->metric != (short) cost)
    {
        ERROR_ReportWarning("Warning: Conversion from unsigned int to "
            "short int data. Possible loss of data\n");
    }

    if (OSPFv2_DEBUG_LSDBErr)
    {
        char linkIDStr[MAX_ADDRESS_STRING_LENGTH];
        char linkDataStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString((*linkInfo)->linkID,linkIDStr);
        IO_ConvertIpAddressToString((*linkInfo)->linkData,linkDataStr);

        printf("    Adding host route. linkID = %s, linkData %s\n",
                    linkIDStr,linkDataStr);
    }

    if (ospf->isQosEnabled == TRUE)
    {
        (*linkInfo)->numTOS = (unsigned char)
                  (2 *  ospf->numQueueAdvertisedForQos);

        // Update the position of the working pointer
        (*linkInfo) = (*linkInfo) + 1;

        // To advertise the QoS related information for the Link
        QospfGetQosInformationForTheLink(node, interfaceId, linkInfo);
    }
    else
    {
        // As Q-OSPF is not enabled, numTOS must be zero
         (*linkInfo)->numTOS = 0;

        // Update the position of the working pointer
        (*linkInfo) = (*linkInfo) + 1;
    }
}


//-------------------------------------------------------------------------//
// NAME     :Ospfv2AddSelfRoute()
// PURPOSE  :Add self interface as type3 link.
// RETURN   :None.
//-------------------------------------------------------------------------//

void Ospfv2AddSelfRoute(
    Node* node,
    int interfaceId,
    Ospfv2LinkInfo** linkInfo)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    (*linkInfo)->linkID = ospf->iface[interfaceId].address;
    (*linkInfo)->linkData = 0xFFFFFFFF;

    (*linkInfo)->type = OSPFv2_STUB;

    (*linkInfo)->metric = 0;

    if (OSPFv2_DEBUG_LSDBErr)
    {
        char linkIDStr[MAX_ADDRESS_STRING_LENGTH];
        char linkDataStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString((*linkInfo)->linkID,linkIDStr);
        IO_ConvertIpAddressToString((*linkInfo)->linkData,linkDataStr);

        printf("    Adding self route. linkID = %s, linkData %s\n",
                    linkIDStr,linkDataStr);
    }

    if (ospf->isQosEnabled == TRUE)
    {
        (*linkInfo)->numTOS = (unsigned char)
            (2 *  ospf->numQueueAdvertisedForQos);

        // Update the position of the working pointer
        (*linkInfo) = (*linkInfo) + 1;

        // To advertise the QoS related information for the Link
        QospfGetQosInformationForTheLink(node, interfaceId, linkInfo);
    }
    else
    {
        // As Q-OSPF is not enabled, numTOS must be zero
         (*linkInfo)->numTOS = 0;

        // Update the position of the working pointer
        (*linkInfo) = (*linkInfo) + 1;
    }
}

//-------------------------------------------------------------------------//
// API       :: ComputeLsaChecksum
// PURPOSE   :: Computes the checksum for a single LSA
// PARAMETERS::
// + lsa : UInt8* : The lsa to compute the checksum for.  Assumes
//                  checksum field is 0
// + length : int : the length of the lsa
// RETURN :: UInt16 : The checksum
//-------------------------------------------------------------------------//
UInt16 ComputeLsaChecksum(
    UInt8* lsa,
    int length)
{
    UInt8* sp;
    UInt8* ep;
    UInt8* p;
    UInt8* q;
    int c0 = 0, c1 = 0;
    int x, y;

    // Compute checksum

    length -= 2;
    sp = lsa + 2;

    for (ep = sp + length; sp < ep; sp = q)
    {
        q = sp + OSPFv2_LSA_CHECKSUM_MODX;
        if (q > ep)
        {
            q = ep;
        }
        for (p = sp; p < q; p++)
        {
            c0 += *p;
            c1 += c0;
        }
        c0 %= 255;
        c1 %= 255;
    }

    x = ((length - OSPFv2_LSA_CHECKSUM_OFFSET) * c0 - c1) % 255;
    if (x <= 0)
    {
        x += 255;
    }
    y = 510 - c0 - x;
    if (y > 255)
    {
        y -= 255;
    }

    // NOTE: This does not take care of endian issue
    return ((x << 8) + y);
}

//-------------------------------------------------------------------------//
// API       :: OspfFindLinkStateChecksum
// PURPOSE   :: Finds link state checksum for a state header and its contents
// PARAMETERS::
// + lsh : Ospfv2LinkStateHeader* : The start of the link state header
// RETURN :: short unsigned int : Checksum calculated
//-------------------------------------------------------------------------//

short unsigned int OspfFindLinkStateChecksum(
    Ospfv2LinkStateHeader* LSA)
{
    int i;
    int j;
    short int lshLength;

    //copy the LSA to another LSA i.e. lsh
    Ospfv2LinkStateHeader* lsh =
                (Ospfv2LinkStateHeader *) MEM_malloc(LSA->length);
    memcpy(lsh, LSA, LSA->length);

    EXTERNAL_ntoh(&lsh->linkStateAge, sizeof(lsh->linkStateAge));

    EXTERNAL_ntoh(&lsh->linkStateID, sizeof(lsh->linkStateID));

    EXTERNAL_ntoh(&lsh->advertisingRouter, sizeof(lsh->advertisingRouter));
    EXTERNAL_ntoh(&lsh->linkStateSequenceNumber,
        sizeof(lsh->linkStateSequenceNumber));

    lshLength = lsh->length;
    EXTERNAL_ntoh(&lsh->length, sizeof(lsh->length));

    switch (lsh->linkStateType)
    {
        case OSPFv2_ROUTER:
        {
            Ospfv2RouterLSA* router = (Ospfv2RouterLSA*) lsh;
            Ospfv2LinkInfo* links = (Ospfv2LinkInfo*) (router + 1);
            QospfPerLinkQoSMetricInfo* qos;

            for (i = 0; i < router->numLinks; i++)
            {
                EXTERNAL_ntoh(&links->linkID, sizeof(links->linkID));
                EXTERNAL_ntoh(&links->linkData, sizeof(links->linkData));
                EXTERNAL_ntoh(&links->metric, sizeof(links->metric));

                qos = (QospfPerLinkQoSMetricInfo*)(links + 1);
                for (j = 0; j < links->numTOS; j++)
                {
                    EXTERNAL_ntoh(
                        &qos->interfaceIndex + 1,
                        sizeof(UInt16));
                    qos++;
                }
                links = (Ospfv2LinkInfo*) qos;
            }

            EXTERNAL_ntoh(
                &router->numLinks,
                sizeof(router->numLinks));
            break;
        }

        case OSPFv2_NETWORK:
        {
            Ospfv2NetworkLSA* network = (Ospfv2NetworkLSA*) lsh;
            int numAddrs;

            numAddrs = (lshLength - sizeof(Ospfv2NetworkLSA))
                           / sizeof(NodeAddress);
            NodeAddress* addr = ((NodeAddress*) (network + 1));

            for (i = 0; i < numAddrs; i++)
            {
                EXTERNAL_ntoh(addr, sizeof(NodeAddress));
                addr++;
            }

            break;
        }

        case OSPFv2_NETWORK_SUMMARY:
        case OSPFv2_ROUTER_SUMMARY:
        {
            Ospfv2SummaryLSA* summary = (Ospfv2SummaryLSA*) lsh;
            NodeAddress* networkMask;
            UInt32* ospfMetric;
            networkMask = (NodeAddress*) (summary + 1);

            // swap netmask, metric
            EXTERNAL_ntoh(networkMask, sizeof(NodeAddress));
            ospfMetric = (UInt32 *)(networkMask + 1);
            EXTERNAL_ntoh(ospfMetric, sizeof(UInt32));
            break;
        }

        case OSPFv2_ROUTER_AS_EXTERNAL:
        {
            Ospfv2ASexternalLSA* external = (Ospfv2ASexternalLSA*) lsh;
            Ospfv2ExternalLinkInfo* info =
                (Ospfv2ExternalLinkInfo*) (external + 1);

            EXTERNAL_ntoh(&info->networkMask, sizeof(info->networkMask));
            EXTERNAL_ntoh(&info->ospfMetric, sizeof(UInt32));
            EXTERNAL_ntoh(
                &info->forwardingAddress,
                sizeof(info->forwardingAddress));
            EXTERNAL_ntoh(
                &info->externalRouterTag,
                sizeof(info->externalRouterTag));
            break;
        }

        case OSPFv2_GROUP_MEMBERSHIP:
        {
            ERROR_ReportWarning("OSPF group membership not supported");
            return 0;
            break;
        }

        case OSPFv2_ROUTER_NSSA_EXTERNAL:
        {
            ERROR_ReportWarning("OSPF NSSA External not supported");
            return 0;
            break;
        }

        /***** Start: OPAQUE-LSA *****/
        case OSPFv2_AS_SCOPE_OPAQUE:
        {
            Ospfv2ASOpaqueLSA* opaqueLSA = (Ospfv2ASOpaqueLSA*) lsh;

#ifdef CYBER_CORE
#ifdef ADDON_BOEINGFCS
            if (Ospfv2OpaqueGetOpaqueType(opaqueLSA->LSHeader.linkStateID)
                    == (unsigned char)HAIPE_ADDRESS_ADV)
            {
                RoutingCesRospfConvertNtoHHaipeAdvLSA(opaqueLSA);
            }
#endif
#endif
            break;
        }
        /***** End: OPAQUE-LSA *****/

        default:
        {
            ERROR_ReportWarning("Unknown LSA type");
            return 0;
            break;
        }
    }

    memset(&lsh->linkStateCheckSum,0, sizeof(lsh->linkStateCheckSum));

    short unsigned int LSCheckSum = 0;
    LSCheckSum = ComputeLsaChecksum(
                                (UInt8*) lsh,
                                lshLength);

    MEM_free(lsh);
    return LSCheckSum;
}

//-------------------------------------------------------------------------//
// NAME     :Ospfv2OriginateRouterLSAForThisArea()
// PURPOSE  :Originate router LSA for the specified area. If areaId passed
//          :as OSPFv2_SINGLE_AREA_ID, then consider total domain as
//          :single area, and include all functional interfaces.
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2OriginateRouterLSAForThisArea(
    Node* node,
    unsigned int areaId,
    BOOL flush,
    BOOL flood)
{
    int i;
    int count = 0;

    Ospfv2ListItem* tempListItem = NULL;
    Ospfv2Neighbor* tempNeighborInfo = NULL;

    Ospfv2RouterLSA* LSA = NULL;
    Ospfv2LinkStateHeader* oldLSHeader = NULL;

    Ospfv2LinkInfo* linkList = NULL;
    Ospfv2LinkInfo* tempWorkingPointer = NULL;
#ifdef ADDON_BOEINGFCS
    int interfaceIndex;
    BOOL isRospfIntf = FALSE;
#endif
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);


    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);

    int listSize;

#ifdef ADDON_BOEINGFCS
    // mobile leaf router does not send a router LSA here
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->iface[i].areaId == areaId &&
            RoutingCesRospfActiveOnInterface(node, i) &&
            RoutingCesRospfIsMobileLeafNode(node, i) &&
            (RoutingCesRospfGetMobileLeafParentRouter(node, i) ==
                (NodeAddress)NETWORK_UNREACHABLE))
        {
            return;
        }
    }
#endif


#ifdef ADDON_BOEINGFCS

    if (RoutingCesRospfActiveOnAnyInterface(node))
    {
        listSize =
            2 * (ROUTING_CES_ROSPF_MAX_NEIGHBORS + node->numberInterfaces);
    }
    else
    {
#endif
        listSize = 2 * (ospf->neighborCount + node->numberInterfaces);
#ifdef ADDON_BOEINGFCS
    }
#endif


    int hostListSize = 0;
    for (i = 0;i < ospf->hostRoutes.size();i++)
    {
        if (ospf->hostRoutes[i].areaId == areaId)
        {
            hostListSize++;
        }
    }

    listSize = listSize + hostListSize;


    int qosMetricSize;

    if (ospf->isQosEnabled == TRUE)
    {
        qosMetricSize = listSize * 2 * ospf->numQueueAdvertisedForQos;
    }
    else
    {
        qosMetricSize = 0;
    }

    linkList = (Ospfv2LinkInfo*)
                MEM_malloc(sizeof(Ospfv2LinkInfo) * listSize
                + sizeof(QospfPerLinkQoSMetricInfo)
                * qosMetricSize);

    memset(linkList,
           0,
           sizeof(Ospfv2LinkInfo) * listSize
           + sizeof(QospfPerLinkQoSMetricInfo) * qosMetricSize);

    tempWorkingPointer = linkList;

    if (OSPFv2_DEBUG_LSDB)
    {
        char clockStr[100];
        TIME_PrintClockInSecond(node->getNodeTime()+getSimStartTime(node),
            clockStr);
        printf("    Node %u originating Router LSA for area %d at"
            " time %s\n", node->nodeId, areaId, clockStr);
    }

    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    // Look at each of my interface...
    for (i = 0; i < node->numberInterfaces; i++)
    {

        if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            continue;
        }

        // If the attached network does not belong to this area, no links are
        // added to the LSA, and the next interface should be examined.
        if (!Ospfv2InterfaceBelongToThisArea(node, areaId, i))
        {
            continue;
        }
#ifdef ADDON_BOEINGFCS
        if (ospf->iface[i].type == OSPFv2_ROSPF_INTERFACE)
        {
            interfaceIndex = i;
            isRospfIntf = TRUE;
        }
#endif
        // If interface state is down, no link should be added
        if (ospf->iface[i].state == S_Down)
        {
            continue;
        }

        //No Loopbac state in QUALNET.
        if (ospf->isAdvertSelfIntf)
        {
            Ospfv2AddSelfRoute(node, i, &tempWorkingPointer);
            count++;
        }

        // If interface is a broadcast network.
        if (ospf->iface[i].type == OSPFv2_BROADCAST_INTERFACE)
        {
            if (OSPFv2_DEBUG_LSDBErr)
            {
                printf("    interface %d is a broadcast network\n", i);
            }

            // If interface state is waiting add type 3 link (stub network)
            if (ospf->iface[i].state == S_Waiting)
            {
                Ospfv2AddType3Link(node, i, &tempWorkingPointer);
            }
            else if (((ospf->iface[i].state != S_DR)
                && (Ospfv2RouterFullyAdjacentWithDR(node, i)))
                || ((ospf->iface[i].state == S_DR)
                && (Ospfv2DRFullyAdjacentWithAnyRouter(node, i))))
            {
                Ospfv2AddType2Link(node, i, &tempWorkingPointer);
            }
            else
            {
                Ospfv2AddType3Link(node, i, &tempWorkingPointer);
            }
            count++;
        }

        // If interface is Point-to-Multipoint
        else if (ospf->iface[i].type ==
                    OSPFv2_POINT_TO_MULTIPOINT_INTERFACE)
        {
            if (OSPFv2_DEBUG_LSDBErr)
            {
                printf("    interface %d is a point-to-multipoint network\n",
                            i);
            }

            // Add a single type3 link with linkID set to router's own
            // IP interface address, linkData set to mask 0xFFFFFFFF.
            Ospfv2AddSelfRoute(node, i, &tempWorkingPointer);
            count++;

            // For each fully adjacent neighbor add an type1 link.
            tempListItem = ospf->iface[i].neighborList->first;

            while (tempListItem)
            {
                tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;

#ifdef JNE_LIB
                if (!tempNeighborInfo)
                {
                    JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                        "Neighbor not found in the Neighbor list",
                        JNE::CRITICAL);
                }
#endif
                ERROR_Assert(tempNeighborInfo,
                    "Neighbor not found in the Neighbor list\n");

                // If neighbor is fully adjacent, add a type1 link
                if (tempNeighborInfo->state == S_Full)
                {
                    Ospfv2AddType1Link(
                        node, i, tempNeighborInfo, &tempWorkingPointer);
                    count++;
                }

                tempListItem = tempListItem->next;
            }
        }

#ifdef ADDON_BOEINGFCS
        else if (ospf->iface[i].type == OSPFv2_ROSPF_INTERFACE)
        {
            count = RoutingCesRospfAddToRouterLSA(node,
                                        i,
                                        &tempWorkingPointer,
                                        thisArea,
                                        count,
                                        flood);
        }
#endif

        // If interface is a point-to-point network.
        else
        {
            if (OSPFv2_DEBUG_LSDBErr)
            {
                printf("    interface %d is a point-to-point network\n", i);
            }

            tempListItem = ospf->iface[i].neighborList->first;

            if ((!tempListItem) &&
                (ospf->iface[i].state == S_PointToPoint))
            {
                if (!(ip->interfaceInfo[i]->isVirtualInterface
                        || ip->interfaceInfo[i]->isUnnumbered)
                    && MAC_IsPointToPointNetwork(node, i))
                {
                    // We have a subnet address for link.
                    Ospfv2AddType3Link(node, i, &tempWorkingPointer);
                    count++;
                }
            }

            // Get all my neighbors information from my neighbor list.
            while (tempListItem)
            {
                tempNeighborInfo = (Ospfv2Neighbor*)tempListItem->data;
#ifdef JNE_LIB
                if (!tempNeighborInfo)
                {
                    JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                        "Neighbor not found in the Neighbor list",
                        JNE::CRITICAL);
                }
#endif
                ERROR_Assert(tempNeighborInfo,
                    "Neighbor not found in the Neighbor list\n");

                if (OSPFv2_DEBUG_LSDBErr)
                {
                    char neighborIPStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(
                        tempNeighborInfo->neighborIPAddress,
                        neighborIPStr);

                    printf("    Neighbor %s, State = %d\n",
                                neighborIPStr,
                                tempNeighborInfo->state);
                }

                // If neighbor is fully adjacent, add a type1 link
                if (tempNeighborInfo->state == S_Full)
                {
                    Ospfv2AddType1Link(node,
                                       i,
                                       tempNeighborInfo,
                                       &tempWorkingPointer);
                    count++;
                }

                // In addition, as long as the state of the interface is
                // Point-to-Point (and regardless of the neighboring router
                // state), a Type 3 link (stub network) should be added.
                if (ospf->iface[i].state == S_PointToPoint)
                {
                    // Virtual link does't have subnet address
                    if (!(ip->interfaceInfo[i]->isVirtualInterface
                        || ip->interfaceInfo[i]->isUnnumbered)
                        && MAC_IsPointToPointNetwork(node, i))
                    {
                        // We have a subnet address for link.
                        Ospfv2AddType3Link(node, i, &tempWorkingPointer);
                    }
                    else
                    {
                        // This is for wireless interface
                        Ospfv2AddHostRoute(
                            node, i, tempNeighborInfo, &tempWorkingPointer);
                    }
                    count++;
                }

                tempListItem = tempListItem->next;
            }
        }
    }


    for (unsigned int i = 0;i<ospf->hostRoutes.size();i++)
    {
        if (ospf->hostRoutes[i].areaId == areaId)
        {
            int interfaceId = NetworkIpGetInterfaceIndexForNextHop(
                       node, ospf->hostRoutes[i].hostIpAddress);

            if ((interfaceId != -1) &&
                 (NetworkIpInterfaceIsEnabled(node, interfaceId)))
            {
                Ospfv2AddHost(node,
                              ospf->hostRoutes[i].cost,
                              ospf->hostRoutes[i].hostIpAddress,
                              interfaceId,
                              &tempWorkingPointer);
                count++;
            }
        }
    }


    if (OSPFv2_DEBUG_LSDBErr)
    {
        printf("    total entries are %d\n", count);
    }

#ifdef JNE_LIB
    if (count > listSize)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Calculation of listSize is wrong",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(count <= listSize, "Calculation of listSize is wrong\n");

    // Get old instance, if any..
    oldLSHeader = Ospfv2LookupLSAList(thisArea->routerLSAList,
                                      ospf->routerID,
                                      ospf->routerID);

    // Start constructing the Router LSA.
    LSA = (Ospfv2RouterLSA*)
          MEM_malloc(sizeof(Ospfv2RouterLSA)
          + (sizeof(Ospfv2LinkInfo) * count)
          + (sizeof(QospfPerLinkQoSMetricInfo)
          * 2 * ospf->numQueueAdvertisedForQos * count));

    memset(LSA,
        0,
        sizeof(Ospfv2RouterLSA)
        + (sizeof(Ospfv2LinkInfo) * count)
        + (sizeof(QospfPerLinkQoSMetricInfo)
        * 2 * ospf->numQueueAdvertisedForQos * count));

    if (!Ospfv2CreateLSHeader(
            node, areaId, OSPFv2_ROUTER, &LSA->LSHeader, oldLSHeader))
    {
        MEM_free(linkList);
        MEM_free(LSA);
        Ospfv2ScheduleRouterLSA(node, areaId, FALSE);
        return;
    }

    if (flush)
    {
        //RFC:1793 Section 2.5
        //Both changes pertain only to DoNotAge LSAs, and
        //in both cases a flushed
        //LSA’s LS age field should be set to MaxAge and
        //not DoNotAge+MaxAge.

        //RFC:1793 Section 2.2
        //In particular, DoNotAge+MaxAge is equivalent to MaxAge;
        //however for backward-compatibility the MaxAge form
        //should always be used when
        //flushing LSAs from the routing domain

        LSA->LSHeader.linkStateAge = (OSPFv2_LSA_MAX_AGE / SECOND);
    }

    LSA->LSHeader.linkStateID = ospf->routerID;

    LSA->LSHeader.length = (short) (sizeof(Ospfv2RouterLSA)
                            + (sizeof(Ospfv2LinkInfo) * count)
                            + (sizeof(QospfPerLinkQoSMetricInfo)
                            * 2 * ospf->numQueueAdvertisedForQos * count));

    if (ospf->nssaAreaBorderRouter == TRUE)
    {
        Ospfv2OptionsSetExt(&(LSA->LSHeader.options), 1);
    }

    // M-OSPF Patch Start
    // modify this parameter for Inter area multicast forwarder
    if (ospf->isMospfEnable == TRUE)
    {
         MospfData* mospf;
         mospf = (MospfData*)ospf->multicastRoutingProtocol;

        if (OSPFv2_DEBUG_ERRORS)
        {
             printf("Node %u has IAMF = %d \n", node->nodeId,
                 mospf->interAreaMulticastFwder);
        }

        if (mospf->interAreaMulticastFwder == TRUE)
        {
            Ospfv2RouterLSASetWCMCReceiver(&(LSA->ospfRouterLsa), 1);
        }
    }
    // M-OSPF Patch End

    // BGP-OSPF Patch Start
    if (ospf->asBoundaryRouter == TRUE)
    {
        Ospfv2RouterLSASetASBRouter(&(LSA->ospfRouterLsa), 1);
    }
    // BGP-OSPF Patch End

    if (ospf->areaBorderRouter == TRUE)
    {
        Ospfv2RouterLSASetABRouter(&(LSA->ospfRouterLsa), 1);
    }

    LSA->numLinks = (short) count;

    // Copy my link information into the LSA.
    memcpy(LSA + 1,
           linkList,
           ((sizeof(Ospfv2LinkInfo) * count)
           + (sizeof(QospfPerLinkQoSMetricInfo)
           * 2 * ospf->numQueueAdvertisedForQos * count)));

    LSA->LSHeader.linkStateCheckSum = OspfFindLinkStateChecksum(
                                                (Ospfv2LinkStateHeader*)LSA);

    // Note LSA Origination time
    thisArea->routerLSAOriginateTime = node->getNodeTime();
    thisArea->routerLSTimer = FALSE;

#ifdef IPNE_INTERFACE
    // Calculate LSA checksum
    //CalculateLinkStateChecksum(node, &LSA->LSHeader);
#endif

#ifdef ADDON_DB
    if (ospf->isMospfEnable == TRUE)
    {
        MospfData* mospf =
            (MospfData*)ospf->multicastRoutingProtocol;

        if (mospf)
        {
            if (Ospfv2RouterLSAGetWCMCReceiver(LSA->ospfRouterLsa))
            {
                mospf->mospfSummaryStats.m_NumRouterLSA_WCMRSent++;
            }

            if (Ospfv2RouterLSAGetVirtLnkEndPt(LSA->ospfRouterLsa))
            {
                mospf->mospfSummaryStats.m_NumRouterLSA_VLEPSent++;
            }

            if (Ospfv2RouterLSAGetASBRouter(LSA->ospfRouterLsa))
            {
                mospf->mospfSummaryStats.m_NumRouterLSA_ASBRSent++;
            }

            if (Ospfv2RouterLSAGetABRouter(LSA->ospfRouterLsa))
            {
                mospf->mospfSummaryStats.m_NumRouterLSA_ABRSent++;
            }
        }
    }
#endif

    if (OSPFv2_DEBUG_LSDBErr)
    {
        Ospfv2PrintLSA((char*) LSA);
    }

#ifdef ADDON_BOEINGFCS

    if (isRospfIntf &&
        !RoutingCesRospfShouldFloodLSA(node,
                             interfaceIndex,
                             thisArea,
                             ospf->routerID,
                             (char*)LSA,
                             OSPFv2_ROUTER))
    {
        MEM_free(linkList);
        MEM_free(LSA);
        return;
    }

#endif
    if (OSPFv2_DEBUG_LSDB)
    {
        printf("    now adding originated LSA to own LSDB\n");
    }

    if (flood)
    {
        // Flood LSA
        Ospfv2FloodLSA(node,
                       ANY_INTERFACE,
                       (char*) LSA,
                        ANY_DEST,
                        areaId);
    }

    if (Ospfv2InstallLSAInLSDB(
            node, thisArea->routerLSAList, (char*) LSA, areaId))
    {
        // I need to recalculate shortest path since my LSDB changed
        Ospfv2ScheduleSPFCalculation(node);
    }

    if (Ospfv2DebugSync(node))
    {
        if (flood)
        {
            fprintf(stdout, "FLOOD Node %u Flood self originated LSA\n",
                 node->nodeId);
        }
        else
        {
             fprintf(stdout, "Dont FLOOD Node %u Flood self originated LSA\n",
                 node->nodeId);
        }
    }

    ospf->stats.numRtrLSAOriginate++;

    MEM_free(linkList);
    MEM_free(LSA);
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2OriginateNetworkLSA
// PURPOSE: Originate network LSAs.
// RETURN: None.
//-------------------------------------------------------------------------//

void Ospfv2OriginateNetworkLSA(
    Node* node,
    int interfaceIndex,
    BOOL flush)
{
    int count = 0;
    Ospfv2ListItem* tempListItem = NULL;
    Ospfv2Neighbor* tempNeighborInfo = NULL;

    Ospfv2NetworkLSA* LSA = NULL;
    Ospfv2LinkStateHeader* oldLSHeader = NULL;

    NodeAddress* linkList = NULL;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Area* thisArea =
        Ospfv2GetArea(node, ospf->iface[interfaceIndex].areaId);

    int numNbr = ospf->iface[interfaceIndex].neighborList->size;

#ifdef ADDON_BOEINGFCS
    NodeAddress* neighbors = NULL;
    if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
    {
        neighbors = (NodeAddress*)
           MEM_malloc(sizeof(NodeAddress) * OSPFv2_ROSPF_MAX_NEIGHBORS);
        // use neighbors array instead of linkList for now
    }
    else
    {
#endif
    linkList = (NodeAddress*) MEM_malloc(sizeof(NodeAddress) * (numNbr + 1)
                                       + sizeof(NodeAddress));
#ifdef ADDON_BOEINGFCS
    }
#endif
    if (OSPFv2_DEBUG_LSDB)
    {
        char clockStr[100];
        NodeAddress netAddr;

        TIME_PrintClockInSecond(node->getNodeTime() + getSimStartTime(node),
                    clockStr);
        netAddr = NetworkIpGetInterfaceNetworkAddress(node, interfaceIndex);
        char netStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(netAddr,netStr);

        printf("    Node %u originating Network LSA for network %s "
               "at time %s\n",
               node->nodeId, netStr, clockStr);
    }

    // Include network mask
    // Include the designated router also.
#ifdef ADDON_BOEINGFCS
    if (ospf->iface[interfaceIndex].type != OSPFv2_ROSPF_INTERFACE)
    {
#endif

    linkList[count++] =
        NetworkIpGetInterfaceSubnetMask(node, interfaceIndex);

    // Include the designated router also.
    linkList[count++] = ospf->routerID;
#ifdef ADDON_BOEINGFCS
    }
    else
    {
        neighbors[count++] =
            NetworkIpGetInterfaceSubnetMask(node, interfaceIndex);
        neighbors[count++] = ospf->routerID;
    }
    if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
    {
        int j;

        count = RoutingCesRospfAddToNetworkLSA(node,
                                     interfaceIndex,
                                     thisArea,
                                     &neighbors,
                                     count);

         // since it is hard to tell how many links will be in our
         // network link state, copy all links into linkList once
         // once we know the correct number
         linkList = (NodeAddress*) MEM_malloc(sizeof(NodeAddress) * (count)
                                       + sizeof(NodeAddress));
         for (j=0; j<count; j++)
         {
             linkList[j] = neighbors[j];
         }

         MEM_free(neighbors);

    }
    else
    {
#endif
    tempListItem = ospf->iface[interfaceIndex].neighborList->first;

    // Get all my neighbors information from my neighbor list.
    while (tempListItem)
    {
        tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;

#ifdef JNE_LIB
        if (!tempNeighborInfo)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Neighbor not present into the Neighbor list",
                JNE::CRITICAL);
        }
#endif
        ERROR_Assert(tempNeighborInfo,
            "Neighbor not present into the Neighbor list\n");

        // List those router that area fully adjacent to DR
        if (tempNeighborInfo->state == S_Full)
        {
            linkList[count++] = tempNeighborInfo->neighborID;
        }

        tempListItem = tempListItem->next;
#ifdef ADDON_BOEINGFCS
        }
#endif
    }

    // Get old instance, if any..
    oldLSHeader = Ospfv2LookupLSAList(
                thisArea->networkLSAList,
                ospf->routerID,
                ospf->iface[interfaceIndex].address);

    // Start constructing the LSA
    LSA = (Ospfv2NetworkLSA*)
          MEM_malloc(sizeof(Ospfv2NetworkLSA)
          + (sizeof(NodeAddress) *  count));

    memset(LSA, 0, sizeof(Ospfv2NetworkLSA) + (sizeof(NodeAddress) * count));

    if (!Ospfv2CreateLSHeader(node,
                              ospf->iface[interfaceIndex].areaId,
                              OSPFv2_NETWORK,
                              &LSA->LSHeader,
                              oldLSHeader))
    {
        MEM_free(linkList);
        MEM_free(LSA);
        ospf->iface[interfaceIndex].networkLSTimer = FALSE;
        Ospfv2ScheduleNetworkLSA(node, interfaceIndex, FALSE);
        return;
    }

    if (flush)
    {
        //RFC:1793 Section 2.5
        //Both changes pertain only to DoNotAge LSAs, and in both cases a flushed
        //LSA’s LS age field should be set to MaxAge and not DoNotAge+MaxAge.

        //RFC:1793 Section 2.2
        //In particular, DoNotAge+MaxAge is equivalent to MaxAge; however for
        //backward-compatibility the MaxAge form should always be used when
        //flushing LSAs from the routing domain

        LSA->LSHeader.linkStateAge = (OSPFv2_LSA_MAX_AGE / SECOND);
    }

    LSA->LSHeader.length = (short) (sizeof(Ospfv2NetworkLSA)
                         + (sizeof(NodeAddress) * count));

    // LSA->LSHeader.linkStateID =
    LSA->LSHeader.linkStateID = ospf->iface[interfaceIndex].address;

    // Copy my link information to the LSA.
    memcpy(LSA + 1,
           linkList,
           sizeof(NodeAddress) * count);

    LSA->LSHeader.linkStateCheckSum = OspfFindLinkStateChecksum(
                                                (Ospfv2LinkStateHeader*)LSA);

#ifdef IPNE_INTERFACE
    // Calculate LSA checksum
    //CalculateLinkStateChecksum(node, &LSA->LSHeader);
#endif

#ifdef ADDON_BOEINGFCS
#if 1

    if (!RoutingCesRospfShouldFloodLSA(node,
                             interfaceIndex,
                             thisArea,
                             ospf->routerID,
                             (char*)LSA,
                             OSPFv2_NETWORK))
    {
        MEM_free(linkList);
        MEM_free(LSA);
        // make sure to reset LSTimer, otherwise another network
        // LS will never be sent again.
        ospf->iface[interfaceIndex].networkLSTimer = FALSE;
        ospf->iface[interfaceIndex].networkLSFlushTimer = FALSE;
        return;
    }

#endif
#endif

    // Note LSA Origination time
    ospf->iface[interfaceIndex].networkLSAOriginateTime =
            node->getNodeTime();

    ospf->iface[interfaceIndex].networkLSTimer = FALSE;

#ifdef ADDON_BOEINGFCS
    ospf->iface[interfaceIndex].networkLSFlushTimer = FALSE;
#endif

    if (OSPFv2_DEBUG_LSDBErr)
    {
        Ospfv2PrintLSA((char*) LSA);
    }

    if (OSPFv2_DEBUG_LSDB)
    {
        printf("    now adding originated LSA to own LSDB\n");
    }

    Ospfv2FloodLSA(node,
                   ANY_INTERFACE,
                   (char*) LSA,
                    ANY_DEST,
                    thisArea->areaID);

    if (Ospfv2InstallLSAInLSDB(
        node, thisArea->networkLSAList, (char*) LSA,
        ospf->iface[interfaceIndex].areaId))
    {
        // I need to recalculate shortest path since my LSDB changed
        Ospfv2ScheduleSPFCalculation(node);
    }

    if (Ospfv2DebugSync(node))
    {
        fprintf(stdout, "Flood self originated LSA\n");
    }

    ospf->stats.numNetLSAOriginate++;

    MEM_free(linkList);
    MEM_free(LSA);
}


//-------------------------------------------------------------------------//
// NAME     :Ospfv2OriginateSummaryLSA()
// PURPOSE  :Originate summary LSA for the specified route.
// RETURN   :None.
//-------------------------------------------------------------------------//

void Ospfv2OriginateSummaryLSA(
    Node* node,
    Ospfv2RoutingTableRow* advRt,
    Ospfv2Area* thisArea,
    BOOL flush)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2LinkStateHeader* oldLSHeader = NULL;
    Ospfv2ListItem* LSAItem = NULL;
    int index = 0;
    Int32 metric;
    BOOL retVal = FALSE;

    char* LSA = (char*) MEM_malloc(sizeof(Ospfv2LinkStateHeader)
                  + sizeof(NodeAddress) + sizeof(int));

    memset(LSA, 0, sizeof(Ospfv2LinkStateHeader)
                    + sizeof(NodeAddress) + sizeof(int));

    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;

    // Get metric
    metric = advRt->metric & 0xFFFFFF;

    if (advRt->destType == OSPFv2_DESTINATION_NETWORK)
    {
        unsigned int i;
        if ((thisArea->externalRoutingCapability == FALSE)
            || (thisArea->isNSSAEnabled))
        {
            for (i = 0;i < ospf->networkSummaryLSANotAdvertiseList.size();
                             i++)
            {
                if ((thisArea->areaID ==
                        ospf->networkSummaryLSANotAdvertiseList[i].areaId)
                  && (advRt->destAddr ==
                        ospf->networkSummaryLSANotAdvertiseList[i].address)
                  && (advRt->addrMask ==
                         ospf->networkSummaryLSANotAdvertiseList[i].mask))
                 {
                     return;
                 }
            }

            for (i = 0;i < ospf->networkSummaryLSAAdvertiseList.size();i++)
            {
                if ((thisArea->areaID ==
                         ospf->networkSummaryLSAAdvertiseList[i].areaId)
                && (advRt->destAddr ==
                         ospf->networkSummaryLSAAdvertiseList[i].address)
                && (advRt->addrMask ==
                          ospf->networkSummaryLSAAdvertiseList[i].mask))
                {
                    break;
                }
            }

            if ((ospf->networkSummaryLSAAdvertiseList.size() > 0) &&
                (i == ospf->networkSummaryLSAAdvertiseList.size()))
            {
                return;
            }
        }

        // Get old instance, if any..
        LSAItem = Ospfv2GetLSAListItem(thisArea->networkSummaryLSAList,
                                       ospf->routerID,
                                       advRt->destAddr);

        if (LSAItem)
        {
            oldLSHeader = (Ospfv2LinkStateHeader*) LSAItem->data;
        }

        retVal = Ospfv2CreateLSHeader(node,
                                      thisArea->areaID,
                                      OSPFv2_NETWORK_SUMMARY,
                                      LSHeader,
                                      oldLSHeader);

        index += sizeof(Ospfv2LinkStateHeader);

        memcpy(&LSA[index], &advRt->addrMask, sizeof(NodeAddress));
    }
    else if (advRt->destType == OSPFv2_DESTINATION_ASBR
        || advRt->destType == OSPFv2_DESTINATION_ABR_ASBR)
    {
        if (thisArea->externalRoutingCapability == FALSE)
        {
            return;
        }
        // Get old instance, if any..
        LSAItem = Ospfv2GetLSAListItem(thisArea->routerSummaryLSAList,
                                       ospf->routerID,
                                       advRt->destAddr);

        if (LSAItem)
        {
            oldLSHeader = (Ospfv2LinkStateHeader*) LSAItem->data;
        }

        retVal = Ospfv2CreateLSHeader(node,
                                      thisArea->areaID,
                                      OSPFv2_ROUTER_SUMMARY,
                                      LSHeader,
                                      oldLSHeader);

        index += sizeof(Ospfv2LinkStateHeader);

        memset(&LSA[index], 0, sizeof(NodeAddress));
    }
    else
    {
        // Shouldn't be here
#ifdef JNE_LIB
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Destination type is not a Network or ASBR,"
            " so it is not possible to create summary LSA",
            JNE::CRITICAL);
#endif
        ERROR_Assert(FALSE, "Destination type is not a Network or ASBR,"
            " so it is not possible to create summary LSA\n");

    }

    if ((LSAItem && (node->getNodeTime() - LSAItem->timeStamp
                        < OSPFv2_MIN_LS_INTERVAL))
        || (!retVal))
    {
        MEM_free(LSA);
        Ospfv2ScheduleSummaryLSA(
            node, advRt->destAddr, advRt->addrMask, advRt->destType,
            thisArea->areaID, FALSE);
        return;
    }

    index += sizeof(NodeAddress);

    memcpy(&LSA[index], &metric, sizeof(Int32));
    index += sizeof(Int32);

    LSHeader->length = sizeof(Ospfv2LinkStateHeader)
                     + sizeof(NodeAddress) + sizeof(Int32);

    LSHeader->linkStateID = advRt->destAddr;

    // M-OSPF Patch Start
    if (ospf->isMospfEnable)
    {
        MospfData* mospf = NULL;

        mospf = (MospfData*) ospf->multicastRoutingProtocol;

        if (!mospf->interAreaMulticastFwder)
        {
            Ospfv2OptionsSetMulticast(&(LSHeader->options), 0);
        }
    }
    // M-OSPF Patch End

    if (advRt->flag == OSPFv2_ROUTE_INVALID)
    {
        Ospfv2AssignNewAgeToLSAAge(ospf,
                                   LSHeader->linkStateAge,
                                   (OSPFv2_LSA_MAX_AGE / SECOND));
    }
#ifdef JNE_LIB
    if (index != LSHeader->length)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Wrong entry into the LSHeader->linkStateAge field.",
            JNE::CRITICAL);
    }
#endif

    ERROR_Assert(index == LSHeader->length,
        "Wrong entry into the LSHeader->linkStateAge field.");

    LSHeader->linkStateCheckSum = OspfFindLinkStateChecksum(
                                                (Ospfv2LinkStateHeader*)LSA);

#ifdef IPNE_INTERFACE
    // Calculate LSA checksum here
    //CalculateLinkStateChecksum(node, LSHeader);
#endif

    // Flood LSA
    Ospfv2FloodLSA(node,
                   ANY_INTERFACE,
                   LSA,
                   ANY_DEST,
                   thisArea->areaID);

    if (advRt->destType == OSPFv2_DESTINATION_NETWORK)
    {
        if (Ospfv2InstallLSAInLSDB(node,
                                   thisArea->networkSummaryLSAList,
                                   LSA,
                                   thisArea->areaID))
        {
            // I need to recalculate shortest path since my LSDB changed
            Ospfv2ScheduleSPFCalculation(node);
        }
    }
    else
    {
        if (Ospfv2InstallLSAInLSDB(node,
                                   thisArea->routerSummaryLSAList,
                                   LSA,
                                   thisArea->areaID))
        {
            // I need to recalculate shortest path since my LSDB changed
            //Ospfv2FindShortestPath(node);
            Ospfv2ScheduleSPFCalculation(node);
        }
    }

    if (Ospfv2DebugSync(node))
    {
        fprintf(stdout, "Node %d Flood self originated summary LSA\n",
            node->nodeId);
    }

    ospf->stats.numSumLSAOriginate++;

    MEM_free(LSA);
}


// BGP-OSPF Patch Start
//-------------------------------------------------------------------------//
// NAME     :Ospfv2OriginateDefaultSummaryLSA()
// PURPOSE  :Originate Default summary LSA for the attached STUB area.
//           This function is called from Ospfv2HandleAbrTask() when
//           routing table contains type1 or type2 external routes.
// RETURN   :None.
//-------------------------------------------------------------------------//

static
void Ospfv2OriginateDefaultSummaryLSA(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2RoutingTableRow defaultRt;
    Ospfv2ListItem* listItem;

    memset(&defaultRt, 0, sizeof(Ospfv2RoutingTableRow));

    defaultRt.destType = OSPFv2_DESTINATION_NETWORK;
    defaultRt.destAddr = OSPFv2_DEFAULT_DESTINATION;
    defaultRt.addrMask = OSPFv2_DEFAULT_MASK;
    defaultRt.flag = OSPFv2_ROUTE_NO_CHANGE;

    listItem = ospf->area->first;

    while (listItem)
    {
        Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;

        if (thisArea && (thisArea->isNSSAEnabled == TRUE)
            && (thisArea->isNSSANoSummary == FALSE))
        {
            listItem = listItem->next;
            continue;
        }

        if (thisArea->externalRoutingCapability == FALSE)
        {
            defaultRt.metric = thisArea->stubDefaultCost;
            Ospfv2OriginateSummaryLSA(node, &defaultRt, thisArea, FALSE);
        }

        listItem = listItem->next;
    }
}
// BGP-OSPF Patch End

//-------------------------------------------------------------------------//
// NAME     :Ospfv2CreateASExternalLSA()
// PURPOSE  : Create LSA for external AS.
// RETURN   : char*.
//-------------------------------------------------------------------------//

static
char *Ospfv2CreateASExternalLSA(Node* node)
{
    char* LSA = (char*) MEM_malloc(sizeof(Ospfv2LinkStateHeader)
                  + sizeof(Ospfv2ExternalLinkInfo));
    memset(LSA, 0, (sizeof(Ospfv2LinkStateHeader)
            + sizeof(Ospfv2ExternalLinkInfo)));

    return LSA;
}

//-------------------------------------------------------------------------//
// NAME     :Ospfv2FillASExternalLSA()
// PURPOSE  : Fill LSA fields.
// RETURN   : BOOL.
//-------------------------------------------------------------------------//
static
BOOL Ospfv2FillASExternalLSA(Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    Ospfv2LinkStateType lsType,
    char* LSA,
    Ospfv2LinkStateHeader* oldLSHeader,
    int iIndex)
{

    BOOL retVal;
    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;

    Ospfv2ExternalLinkInfo* ospfv2ExternalLinkInfo =
        (Ospfv2ExternalLinkInfo*) (LSA + sizeof(Ospfv2LinkStateHeader));

    retVal = Ospfv2CreateLSHeader(node,
                                  OSPFv2_INVALID_AREA_ID,
                                  lsType,
                                  LSHeader,
                                  oldLSHeader);

    if (!retVal)
    {
        MEM_free(LSA);
        Ospfv2ScheduleASExternalLSA(node,
                                    destAddress,
                                    destAddressMask,
                                    nextHopAddress,
                                    external2Cost,
                                    FALSE,
                                    lsType);
        return retVal;
    }

    ospfv2ExternalLinkInfo->networkMask = destAddressMask;
#ifdef ADDON_MA
    if (external2Cost == OSPFv2_LS_INFINITY)
    {
        //LSHeader->linkStateAge = (OSPFv2_LSA_MAX_AGE/SECOND);
        Ospfv2AssignNewAgeToLSAAge(ospf,
                                   LSHeader->linkStateAge,
                                   (OSPFv2_LSA_MAX_AGE / SECOND));
    }
    LSHeader->mask = destAddressMask;
#endif
    Ospfv2ExternalLinkSetEBit(&(ospfv2ExternalLinkInfo->ospfMetric), 1);
    Ospfv2ExternalLinkSetMetric(&(ospfv2ExternalLinkInfo->ospfMetric),
        external2Cost);

    if (lsType == OSPFv2_ROUTER_NSSA_EXTERNAL)
    {
        ospfv2ExternalLinkInfo->forwardingAddress =
                             NetworkIpGetInterfaceAddress(node, iIndex);
    }
    else
    {
       ospfv2ExternalLinkInfo->forwardingAddress = 0;
    }

    LSHeader->length = sizeof(Ospfv2LinkStateHeader)
                     + sizeof(Ospfv2ExternalLinkInfo);

    LSHeader->linkStateID = destAddress;
    Ospfv2OptionsSetExt(&(LSHeader->options), 1);

#ifdef IPNE_INTERFACE
    // Calculate LSA checksum
    //CalculateLinkStateChecksum(node, LSHeader);
#endif

    return retVal;
}

// BGP-OSPF Patch Start
//-------------------------------------------------------------------------//
// NAME     :Ospfv2OriginateASexternalLSA()
// PURPOSE  :Originate AS external LSA for the specified external route.
// RETURN   :None.
//-------------------------------------------------------------------------//

void Ospfv2OriginateASExternalLSA(
    Node *node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    BOOL flush)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Area* thisArea = NULL;
    Ospfv2ListItem* listItem = NULL;
    char* LSA = NULL;

#ifdef ADDON_BOEINGFCS
    if (!RoutingCesRospfActiveOnAnyInterface(node))
#endif
    {
#ifdef JNE_LIB
        if (!ospf->asBoundaryRouter)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Router is not an ASBR.",
                JNE::CRITICAL);
        }
#endif
        ERROR_Assert(ospf->asBoundaryRouter, "Router is not an ASBR.\n");
    }

    listItem = ospf->area->first;

    while (listItem)
    {
        thisArea = (Ospfv2Area*) listItem->data;

        // If this is not a Stub area, flood lsa through this area
        if (thisArea->externalRoutingCapability)
        {
            if (LSA == NULL)
            {
                // Get old instance, if any..
                Ospfv2LinkStateHeader* oldLSHeader = NULL;
                BOOL retVal;
#ifdef ADDON_MA
                oldLSHeader = Ospfv2LookupLSAList_Extended(node,
                                                ospf->asExternalLSAList,
                                                ospf->routerID,
                                                destAddress,
                                                destAddressMask);


#else
                oldLSHeader = Ospfv2LookupLSAList(ospf->asExternalLSAList,
                                                  ospf->routerID,
                                                  destAddress);
#endif
                if (nextHopAddress == (unsigned) (NETWORK_UNREACHABLE))
                {
#ifdef ADDON_MA
                    // This could happen if node has multiple MA-RMA connections
                    if (oldLSHeader == NULL)
                    {
                        return;
                    }
#else
                    if (oldLSHeader == NULL)
                    {
                        return;
                    }
#endif
                    Ospfv2FlushLSA(node, (char*) oldLSHeader,
                                              OSPFv2_INVALID_AREA_ID);
                    return;
                }

                LSA = (char*) MEM_malloc(sizeof(Ospfv2LinkStateHeader)
                                    + sizeof(Ospfv2ExternalLinkInfo));
                memset(LSA, 0, (sizeof(Ospfv2LinkStateHeader)
                                    + sizeof(Ospfv2ExternalLinkInfo)));
                 retVal = Ospfv2FillASExternalLSA(node,
                     destAddress,
                     destAddressMask,
                     nextHopAddress,
                     external2Cost,
                     OSPFv2_ROUTER_AS_EXTERNAL,
                     LSA,
                     oldLSHeader,
                     -1);
                if (!retVal)
                {
                    return;
                }

                Ospfv2LinkStateHeader* LSHeader =(Ospfv2LinkStateHeader*)LSA;

                LSHeader->linkStateCheckSum = OspfFindLinkStateChecksum(
                                                (Ospfv2LinkStateHeader*)LSA);

                Ospfv2FloodLSA(node,
                               ANY_INTERFACE,
                               LSA,
                               ANY_DEST,
                               thisArea->areaID);

                if (Ospfv2InstallLSAInLSDB(node,
                                           ospf->asExternalLSAList,
                                           LSA,
                                           OSPFv2_INVALID_AREA_ID))
                {
                    // I need to recalculate shortest path since LSDB changed
                    Ospfv2ScheduleSPFCalculation(node);
                }

                if (Ospfv2DebugSync(node))
                {
                    fprintf(stdout, "Flood self originated LSA\n");
                }

                listItem = listItem->next;
                continue;
            }
            Ospfv2FloodLSA(node,
                               ANY_INTERFACE,
                               LSA,
                               ANY_DEST,
                               thisArea->areaID);
            ospf->stats.numASExtLSAOriginate++;
        }

        listItem = listItem->next;
    }

    if (LSA)
        MEM_free(LSA);

}

//-------------------------------------------------------------------------//
// NAME     :Ospfv2FindInterfaceIndexForArea()
// PURPOSE  :Retereving interface index for the gived Area ID.
// RETURN   :interface index.
//-------------------------------------------------------------------------//
static
int Ospfv2FindInterfaceIndexForArea(
      Node* node,
      Ospfv2Data* ospf,
      unsigned int areaID)
{
    Ospfv2ListItem* listItem = NULL;
    listItem = ospf->area->first;

     for (int i = 0;i < node->numberInterfaces;i++)
     {
         if ((NetworkIpGetUnicastRoutingProtocolType(node, i, NETWORK_IPV4)
                                                == ROUTING_PROTOCOL_OSPFv2)
             && (ospf->iface[i].areaId == areaID))
         {
             return i;
         }
     }

     return -1;
}

//-------------------------------------------------------------------------//
// NAME     :Ospfv2OriginateNSSAexternalLSA()
// PURPOSE  :Originate NSSA external LSA for the specified external route.
// RETURN   :None.
//-------------------------------------------------------------------------//

void Ospfv2OriginateNSSAExternalLSA(
    Node *node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    BOOL flush)
{

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Area* thisArea = NULL;
    Ospfv2ListItem* listItem = NULL;
    char* LSA = NULL;
#ifdef JNE_LIB
    if (!ospf->asBoundaryRouter && !ospf->nssaAreaBorderRouter)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Router is neither an NSSA ASBR nor an ABR",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(ospf->asBoundaryRouter || ospf->nssaAreaBorderRouter,
                                "Router is not an NSSA ASBR.nor ABR\n");

    listItem = ospf->area->first;

    while (listItem)
    {
        thisArea = (Ospfv2Area*) listItem->data;

        // If this is NSSA area, flood lsa through this area
        if (thisArea && thisArea->isNSSAEnabled &&
            !(thisArea->isNSSANoRedistribution))
        {
            if (LSA == NULL)
            {
                // Get old instance, if any..
                Ospfv2LinkStateHeader* oldLSHeader = NULL;
                BOOL retVal;
#ifdef ADDON_MA
                oldLSHeader = Ospfv2LookupLSAList_Extended(node,
                                                ospf->nssaExternalLSAList,
                                                ospf->routerID,
                                                destAddress,
                                                destAddressMask);

#else
                oldLSHeader = Ospfv2LookupLSAList(ospf->nssaExternalLSAList,
                                                  ospf->routerID,
                                                  destAddress);
#endif

                if (nextHopAddress == (unsigned) (NETWORK_UNREACHABLE))
                {
#ifdef ADDON_MA
                    // This could happen if node has multiple MA-RMA connections
                    if (oldLSHeader == NULL)
                    {
                        return;
                    }
#else
                    if (oldLSHeader == NULL)
                    {
#ifdef JNE_LIB
                        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                            "This situation should not occur.",
                            JNE::CRITICAL);
#endif

                        ERROR_ReportWarning(
                            "OSPFv2 trying to flush non-existent LSA!");
                        return;
                    }
#endif
                    Ospfv2FlushLSA(node, (char*) oldLSHeader,
                                  OSPFv2_INVALID_AREA_ID);
                    return;
                }

                int iIndex = Ospfv2FindInterfaceIndexForArea(
                                    node,
                                    ospf,
                                    thisArea->areaID);

                LSA = (char*) MEM_malloc(sizeof(Ospfv2LinkStateHeader)
                                        + sizeof(Ospfv2ExternalLinkInfo));
                memset(LSA, 0, (sizeof(Ospfv2LinkStateHeader)
                                        + sizeof(Ospfv2ExternalLinkInfo)));

                retVal = Ospfv2FillASExternalLSA(node,
                    destAddress,
                    destAddressMask,
                    nextHopAddress,
                    external2Cost,
                    OSPFv2_ROUTER_NSSA_EXTERNAL,
                    LSA,
                    oldLSHeader,
                    iIndex);

                if (!retVal)
                {
                    return;
                }

                Ospfv2LinkStateHeader* LSHeader =(Ospfv2LinkStateHeader*)LSA;

                if (ospf->nssaAreaBorderRouter == FALSE)
                {
                    Ospfv2OptionsSetNSSACapability(&(LSHeader->options), 1);
                }

               for (unsigned int i = 0;
                            i < ospf->nssaNotAdvertiseList.size();
                            i++)
               {
                  if ((destAddress == ospf->nssaNotAdvertiseList[i].address)
                  && (destAddressMask == ospf->nssaNotAdvertiseList[i].mask))
                  {
                     Ospfv2OptionsSetNSSACapability(
                                        &(LSHeader->options), 0);
                     break;
                  }
               }

                LSHeader->linkStateCheckSum = OspfFindLinkStateChecksum(
                                                (Ospfv2LinkStateHeader*)LSA);

                Ospfv2FloodLSA(node,
                               ANY_INTERFACE,
                               LSA,
                               ANY_DEST,
                               thisArea->areaID);

                if (Ospfv2InstallLSAInLSDB(node,
                                           ospf->nssaExternalLSAList,
                                           LSA,
                                           OSPFv2_INVALID_AREA_ID))
                {
                    // I need to recalculate shortest path since LSDB changed
                    Ospfv2ScheduleSPFCalculation(node);
                }

                if (Ospfv2DebugSync(node))
                {
                    fprintf(stdout, "Flood self originated LSA\n");
                }

                listItem = listItem->next;
                continue;
            }
            Ospfv2FloodLSA(node,
                           ANY_INTERFACE,
                           LSA,
                           ANY_DEST,
                           thisArea->areaID);
        }

        listItem = listItem->next;
    }

    if (LSA)
        MEM_free(LSA);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2TranslateType7ToType5()
// PURPOSE      :Handle any translation occur from Type 7 to Type 5 on ABR
// ASSUMPTION   :None
// RETURN VALUE :BOOL
//-------------------------------------------------------------------------//
static
BOOL Ospfv2TranslateType7ToType5(
                Node* node,
                Ospfv2RoutingTableRow* routePtr)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
                  NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2RoutingTableRow* rowPtr = routePtr;

    Ospfv2LinkStateHeader* rowPtrLSHeader =
                                   (Ospfv2LinkStateHeader*) rowPtr->LSOrigin;
    Ospfv2ExternalLinkInfo* rowPtrLSABody =
                               (Ospfv2ExternalLinkInfo*)(rowPtrLSHeader + 1);
    NodeAddress forwardingAdd = rowPtrLSABody->forwardingAddress;

    unsigned int j, k = 0;
    BOOL wasExactMatch = FALSE;
    BOOL subsumedEntryFound = FALSE;
    BOOL oldEntryFound = FALSE;
    int rowPtrNumHostBits;
    int rangeNumHostBits;

    NodeAddress linkStateIDValue = rowPtrLSHeader->linkStateID;
    NodeAddress maskValue = rowPtrLSABody->networkMask;
    NodeAddress forwardingAddressValue = rowPtrLSABody->forwardingAddress;
    UInt32 metricValue = Ospfv2ExternalLinkGetMetric(
                                                  rowPtrLSABody->ospfMetric);

    //reset P-bit of Type 7 LSA
    Ospfv2OptionsSetNSSACapability(&(rowPtrLSHeader->options), 0);

    // Get old instance, if any..
    Ospfv2LinkStateHeader* oldAsExtLSAHeader = NULL;

    oldAsExtLSAHeader = Ospfv2LookupLSAList(ospf->asExternalLSAList,
                                          ospf->routerID,
                                          rowPtr->destAddr);

    if (oldAsExtLSAHeader)
    {
        Ospfv2ExternalLinkInfo* oldAsExtLSABody =
                            (Ospfv2ExternalLinkInfo*)(oldAsExtLSAHeader + 1);

        UInt32 oldAsExtMetricValue = Ospfv2ExternalLinkGetMetric(
                                                oldAsExtLSABody->ospfMetric);

        if ((oldAsExtLSAHeader->linkStateID == rowPtrLSHeader->linkStateID)
            && (oldAsExtMetricValue == metricValue)
            && (oldAsExtLSABody->forwardingAddress == forwardingAdd))
        {
            return FALSE;
        }
    }

    if (ospf->nssaAdvertiseList.size() < 1) //for default case
    {
        wasExactMatch = TRUE;
    }

    //check for exact match in Advertise status
    for (j = 0;j < ospf->nssaAdvertiseList.size();j++)
    {
        rowPtrNumHostBits = ConvertSubnetMaskToNumHostBits(
                                                 rowPtrLSABody->networkMask);
        rangeNumHostBits = ConvertSubnetMaskToNumHostBits(
                                            ospf->nssaAdvertiseList[j].mask);

        if (rowPtr->destAddr == ospf->nssaAdvertiseList[j].address
            && rowPtrNumHostBits == rangeNumHostBits)
        {
            //exact match
            wasExactMatch = TRUE;
            ospf->nssaAdvertiseList[j].wasExactMatched = TRUE;
            break;
        }
    }

    if (!wasExactMatch)
    {
        //check for more specific instance in Advertise status
        for (k = 0;k < ospf->nssaAdvertiseList.size();k++)
        {

            if (ospf->nssaAdvertiseList[k].wasExactMatched)
            {
                continue;
            }

            if (MaskIpAddress(rowPtr->destAddr,
                              ospf->nssaAdvertiseList[k].mask)
                == ospf->nssaAdvertiseList[k].address)
            {

                UInt32 rowPtrMetricValue = Ospfv2ExternalLinkGetMetric(
                                                  rowPtrLSABody->ospfMetric);
                UInt32 asExtMetricValue = 0;

                if (ospf->nssaAdvertiseList[k].LSA)
                {
                    oldEntryFound = TRUE;

                    Ospfv2LinkStateHeader* storedAsExtLSAHeader =
                      (Ospfv2LinkStateHeader*)ospf->nssaAdvertiseList[k].LSA;

                    Ospfv2ExternalLinkInfo* storedAsExtLSABody =
                           (Ospfv2ExternalLinkInfo*)(storedAsExtLSAHeader+1);

                    asExtMetricValue = Ospfv2ExternalLinkGetMetric(
                                             storedAsExtLSABody->ospfMetric);

                    if (rowPtr->pathType == OSPFv2_TYPE1_EXTERNAL
                        && ospf->nssaAdvertiseList[k].pathType ==
                                                      OSPFv2_TYPE1_EXTERNAL)
                    {
                        if (rowPtrMetricValue > asExtMetricValue)
                        {
                            Ospfv2ExternalLinkSetMetric(
                                           &(storedAsExtLSABody->ospfMetric),
                                           rowPtrMetricValue);
                        }
                    }
                    else if (rowPtr->pathType == OSPFv2_TYPE2_EXTERNAL
                             && ospf->nssaAdvertiseList[k].pathType ==
                                                       OSPFv2_TYPE1_EXTERNAL)
                    {
                        ospf->nssaAdvertiseList[k].pathType =
                                                       OSPFv2_TYPE2_EXTERNAL;

                        Ospfv2ExternalLinkSetMetric(
                                           &(storedAsExtLSABody->ospfMetric),
                                           rowPtrMetricValue + 1);
                    }
                    else if (rowPtr->pathType == OSPFv2_TYPE2_EXTERNAL
                             && ospf->nssaAdvertiseList[k].pathType ==
                                                       OSPFv2_TYPE2_EXTERNAL)
                    {
                        if ((rowPtrMetricValue + 1) > asExtMetricValue)
                        {
                            Ospfv2ExternalLinkSetMetric(
                                           &(storedAsExtLSABody->ospfMetric),
                                           rowPtrMetricValue + 1);
                        }
                    }
                }
                else
                {
                    subsumedEntryFound = TRUE;

                    linkStateIDValue = ospf->nssaAdvertiseList[k].address;
                    maskValue = ospf->nssaAdvertiseList[k].mask;
                    forwardingAddressValue = 0;

                    ospf->nssaAdvertiseList[k].pathType = rowPtr->pathType;

                    if (rowPtr->pathType == OSPFv2_TYPE1_EXTERNAL)
                    {
                        metricValue = rowPtrMetricValue;
                    }
                    else if (rowPtr->pathType == OSPFv2_TYPE2_EXTERNAL)
                    {
                        metricValue = rowPtrMetricValue + 1;
                    }
                }

                wasExactMatch = FALSE;
                break;
            }
        }
    }// if (!wasExactMatch)

    //for new Type 5 LSA
    char* asExtLSA = NULL;

    if (!wasExactMatch
        && oldEntryFound
        && ospf->nssaAdvertiseList[k].LSA != NULL)
    {
        asExtLSA = ospf->nssaAdvertiseList[k].LSA;
    }
    else
    {
        //creating Type 5 LSA
        asExtLSA = (char*) MEM_malloc(sizeof(Ospfv2LinkStateHeader)
                                    + sizeof(Ospfv2ExternalLinkInfo));
        memset(asExtLSA, 0, (sizeof(Ospfv2LinkStateHeader)
                                    + sizeof(Ospfv2ExternalLinkInfo)));

        if (!wasExactMatch
            && !oldEntryFound
            && (ospf->nssaAdvertiseList.size() > 0)
            && (k < ospf->nssaAdvertiseList.size())
            && (ospf->nssaAdvertiseList[k].LSA == NULL))
        {
            ospf->nssaAdvertiseList[k].LSA = asExtLSA;
        }
    }

    Ospfv2LinkStateHeader* asExtLSAHeader = (Ospfv2LinkStateHeader*)asExtLSA;

    Ospfv2ExternalLinkInfo* asExtLSABody =
                            (Ospfv2ExternalLinkInfo*) (asExtLSAHeader + 1);

    if (!oldEntryFound)
    {
        //setting E bit for Type 5
        Ospfv2OptionsSetExt(&(asExtLSAHeader->options), 1);
        Ospfv2ExternalLinkSetEBit(&(asExtLSABody->ospfMetric), 1);

        Ospfv2AssignNewAgeToLSAAge(ospf,
                                   asExtLSAHeader->linkStateAge,
                                   rowPtrLSHeader->linkStateAge);
        asExtLSAHeader->options = rowPtrLSHeader->options;
        asExtLSAHeader->linkStateID = linkStateIDValue;
        asExtLSAHeader->linkStateSequenceNumber =
                                     rowPtrLSHeader->linkStateSequenceNumber;
        asExtLSABody->networkMask = maskValue;

        Ospfv2ExternalLinkSetMetric(&(asExtLSABody->ospfMetric),metricValue);

        asExtLSABody->forwardingAddress = forwardingAddressValue;
        asExtLSABody->externalRouterTag = rowPtrLSABody->externalRouterTag;

        asExtLSAHeader->linkStateType = OSPFv2_ROUTER_AS_EXTERNAL;
        asExtLSAHeader->advertisingRouter = ospf->routerID;

        asExtLSAHeader->length = sizeof(Ospfv2LinkStateHeader)
                                 + sizeof(Ospfv2ExternalLinkInfo);
    }

    asExtLSAHeader->linkStateCheckSum = 0x0;

#ifdef IPNE_INTERFACE
    // Calculate LSA checksum
    CalculateLinkStateChecksum(node, asExtLSAHeader);
#endif

    if ((wasExactMatch && !oldEntryFound)
        || ospf->nssaAdvertiseList.size() < 1)
    {
        //flooding Type 5 LSA to all attached Type 5 capable areas
        int i = 0;

        for (i = 0; i < node->numberInterfaces; i++)
        {
            Ospfv2Interface* thisInterface = &ospf->iface[i];
            Ospfv2Area* thisArea = Ospfv2GetArea(node, thisInterface->areaId);

            if (thisArea && thisArea->isNSSAEnabled)
            {
                continue;
            }

            Ospfv2FloodLSA(node,
                           i,
                           asExtLSA,
                           ANY_DEST,
                           thisArea->areaID);
        }

        if (Ospfv2InstallLSAInLSDB(node,
                                   ospf->asExternalLSAList,
                                   asExtLSA,
                                   OSPFv2_INVALID_AREA_ID))
        {
            // I need to recalculate shortest path since my LSDB changed
            Ospfv2ScheduleSPFCalculation(node);
        }

        //Entry served.
        if (asExtLSA)
        {
            MEM_free(asExtLSA);
        }
    }
    else if (!wasExactMatch && !oldEntryFound && !subsumedEntryFound
            && (ospf->nssaAdvertiseList.size() > 0))
    {
        if (asExtLSA != NULL)
        {
            MEM_free(asExtLSA);
        }
    }

    return TRUE;
}


// Added for Route Redistribution
#ifdef ROUTE_REDISTRIBUTE

void Ospfv2HookToRedistribute(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    void* routeCost)
{
    BOOL flush = FALSE;
    Ospfv2LinkStateHeader* oldLSHeader = NULL;
    Int32 cost =  *(Int32*)routeCost;
    Int32 i;

    // Below for loop is to ignore redistribution of ospf interface
    // address in to ospf network.
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (NetworkIpGetUnicastRoutingProtocolType(
                    node,
                    i,
                    NETWORK_IPV4) == ROUTING_PROTOCOL_OSPFv2)
        {
            if (NetworkIpGetInterfaceAddress(node, i) == destAddress)
            {
                return;
            }
        }
    }

    Ospfv2Data* ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv2);

#ifdef JNE_LIB
    if (!ospf->asBoundaryRouter)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Router is not an ASBR.",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(ospf->asBoundaryRouter, "Router is not an ASBR.\n");

    oldLSHeader = Ospfv2LookupLSAList(
                      ospf->asExternalLSAList,
                      ospf->routerID,
                      destAddress);

    if (oldLSHeader != NULL)
    {
        oldLSHeader = Ospfv2LookupLSAList(
                      ospf->nssaExternalLSAList,
                      ospf->routerID,
                      destAddress);
    }

    if (oldLSHeader != NULL)
    {
        return;
    }

    if (nextHopAddress == (unsigned) NETWORK_UNREACHABLE)
    {
        flush = TRUE;
    }

      Ospfv2OriginateASExternalLSA(
           node,
           destAddress,
           destAddressMask,
           nextHopAddress,
           cost,
           flush);

      Ospfv2OriginateNSSAExternalLSA(
            node,
            destAddress,
            destAddressMask,
            nextHopAddress,
            cost,
            flush);
}

#endif  // ROUTE_REDISTRIBUTE


//-------------------------------------------------------------------------//
// NAME     :Ospfv2IsEnabled()
// PURPOSE  :Check IGP protocol is OSPF or NOT.
// RETURN   :Return TRUE if runnning OSPF Otherwise FALSE.
//-------------------------------------------------------------------------//

BOOL Ospfv2IsEnabled(
    Node *node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    BOOL flush)
{
    if (NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2))
    {
        Ospfv2OriginateASExternalLSA(node,
                                 destAddress,
                                 destAddressMask,
                                 nextHopAddress,
                                 external2Cost,
                                 flush);
        return TRUE;
    }
    return FALSE;
}
// BGP-OSPF Patch End


//-------------------------------------------------------------------------//
// NAME         :Ospfv2LSAsContentsChanged()
// PURPOSE      :Check contents of two LSA for any mismatch. This is used
//               to dtermine whether we need to recalculate routing table.
// ASSUMPTION   :The body of the LSA is not checked. Always assume they
//               differ.
// RETURN       :TRUE if contents have been changed, FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2LSAsContentsChanged(Ospfv2Data* ospf,
    Ospfv2LinkStateHeader* newLSHeader,
    Ospfv2LinkStateHeader* oldLSHeader)
{
    // If LSA's option field has changed
    if (newLSHeader->options != oldLSHeader->options)
    {
        return TRUE;
    }
    // Else if one instance has LSA age MaxAge and other does not
    else if (((Ospfv2MaskDoNotAge(ospf, newLSHeader->linkStateAge) ==
                  (OSPFv2_LSA_MAX_AGE / SECOND))
           && (Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge) !=
                      (OSPFv2_LSA_MAX_AGE / SECOND)))
       || ((Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge) ==
                   (OSPFv2_LSA_MAX_AGE / SECOND))
           && (Ospfv2MaskDoNotAge(ospf, newLSHeader->linkStateAge) !=
                      (OSPFv2_LSA_MAX_AGE / SECOND))))
    {
        return TRUE;
    }
    // Else if length field has changed
    else if (newLSHeader->length != oldLSHeader->length)
    {
        return TRUE;
    }
    // The body of the LSA has changed
    else if (Ospfv2LSABodyChanged(newLSHeader, oldLSHeader))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2AssignNewLSAIntoLSOrigin()
// PURPOSE: Assign newLSA address into the LSOrigin which previously holding
//          the old LSA address.
// RETURN: None
//-------------------------------------------------------------------------//

static
void Ospfv2AssignNewLSAIntoLSOrigin(Node* node,
                                    char* LSA,
                                    char* newLSA)
{
    int i;
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        if (rowPtr[i].LSOrigin == LSA)
        {
            rowPtr[i].LSOrigin = newLSA;
            //break;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2InstallLSAInLSDB()
// PURPOSE      :Installing LSAs in the database (RFC 2328, Sec-13.2).
// RETURN       :TRUE if new LSA is not same as old LSA, FALSE otherwise.
//-------------------------------------------------------------------------//

BOOL Ospfv2InstallLSAInLSDB(
    Node* node,
    Ospfv2List* list,
    char* LSA,
    unsigned int areaId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2LinkStateHeader* listLSHeader = NULL;
    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;
    Ospfv2ListItem* item = list->first;
    char* newLSA = NULL;
    BOOL retVal = FALSE;

    while (item)
    {
        // Get LS Header
        listLSHeader = (Ospfv2LinkStateHeader*) item->data;
#ifdef ADDON_MA
        BOOL  isMatched;
         if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL ||
            LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)&&
            (listLSHeader->linkStateType == LSHeader->linkStateType))
        {
            /*
            Ospfv2ExternalLinkInfo* linkInfo;
            Ospfv2ExternalLinkInfo* listInfo;
            linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
            listInfo = (Ospfv2ExternalLinkInfo*) (listLSHeader + 1);

            if (linkInfo->networkMask == listInfo->networkMask)
            */
            if (LSHeader->mask == listLSHeader->mask)
            {
                isMatched = TRUE;
            }else
            {
                isMatched = FALSE;
            }
        }else
        {
           isMatched = TRUE;
        }

        if (LSHeader->advertisingRouter == listLSHeader->advertisingRouter &&
            LSHeader->linkStateID == listLSHeader->linkStateID &&
            isMatched)
#else
        if (listLSHeader->advertisingRouter == LSHeader->advertisingRouter
            && listLSHeader->linkStateID == LSHeader->linkStateID)
#endif
        {
            break;
        }

        item = item->next;
    }

    if (OSPFv2_DEBUG_LSDB)
    {
        printf(" %d Install LSA in LSDB\n", node->nodeId);
    }

    // LSA found in list
    if (item != NULL)
    {
        // Check for difference between old and new instance of LSA
        retVal = Ospfv2LSAsContentsChanged(ospf,
                                           LSHeader, listLSHeader);
        item->timeStamp = node->getNodeTime();

        if (retVal)
        {
            newLSA = Ospfv2CopyLSA(LSA);
            // Update LSA in LSDB.
            item->data = (void*) newLSA;

            if ((LSHeader->linkStateType == OSPFv2_ROUTER) ||
                (LSHeader->linkStateType == OSPFv2_NETWORK))
            {
                // Assign newLSA address into the LSOrigin.
                Ospfv2AssignNewLSAIntoLSOrigin(
                        node, (char*)listLSHeader, newLSA);
            }
            MEM_free(listLSHeader);
        }
        else
        {
            memcpy(listLSHeader, LSHeader, LSHeader->length);
        }
    }
    // LSA not found in list
    else
    {
        newLSA = Ospfv2CopyLSA(LSA);
        Ospfv2InsertToList(list, node->getNodeTime(), (void*) newLSA);
        retVal = TRUE;
    }

#ifdef ADDON_MA
    if (node->mAEnabled == TRUE)
    {
        Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
        if (ospf->sendLSAtoMA == TRUE)
        {
            int maRetVal = MAConstructSendAndRecieveCommunication(LSA,node);
        }
    }
#endif

    if (Ospfv2LSAHasMaxAge(ospf, LSA))
    {
        if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL) ||
            /***** Start: OPAQUE-LSA *****/
            (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
            /***** End: OPAQUE-LSA *****/
        {
            Ospfv2ListItem* listItem = ospf->area->first;
            while (listItem)
            {
                Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;
#ifdef JNE_LIB
                if (!thisArea)
                {
                    JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                                  "Area structure not initialized",
                                  JNE::CRITICAL);
                }
#endif
                ERROR_Assert(thisArea, "Area structure not initialized");
                if (thisArea->externalRoutingCapability)
                {
                    Ospfv2AddLsaToMaxAgeLsaList(node, LSA, thisArea->areaID);
                }
                listItem = listItem->next;
            }
        }
        else if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
        {
            Ospfv2ListItem* listItem = ospf->area->first;

            while (listItem)
            {
                Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;
#ifdef JNE_LIB
                if (!thisArea)
                {
                    JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                        "Area structure not initialized",
                        JNE::CRITICAL);
                }
#endif
                ERROR_Assert(thisArea, "Area structure not initialized");

                if (thisArea->isNSSAEnabled)
                {
                    Ospfv2AddLsaToMaxAgeLsaList(node, LSA, thisArea->areaID);
                }
                listItem = listItem->next;
            }
        }
        else
        {
            Ospfv2AddLsaToMaxAgeLsaList(node, LSA, areaId);
        }
    }
    return retVal;
}


//-------------------------------------------------------------------------//
// FUNCTION    : Ospfv2CheckRouterLSABody()
// PURPOSE     : Compare old & new LSA
// ASSUMPTION  : TOS Field remains unaltered
// RETURN VALUE: TRUE  if LSA changed
//-------------------------------------------------------------------------//

static
BOOL Ospfv2CheckRouterLSABody(
    Ospfv2RouterLSA* newRouterLSA,
    Ospfv2RouterLSA* oldRouterLSA)
{
    int i;
    int j;
    BOOL* sameLinkInfo = NULL;
    int size = oldRouterLSA->numLinks;
    Ospfv2LinkInfo* newLinkList = (Ospfv2LinkInfo*) (newRouterLSA + 1);
    Ospfv2LinkInfo* oldLinkList = (Ospfv2LinkInfo*) (oldRouterLSA + 1);

    if ((newRouterLSA->numLinks != oldRouterLSA->numLinks)
      || (newRouterLSA->ospfRouterLsa != oldRouterLSA->ospfRouterLsa))
    {
        return TRUE;
    }

    // Make all flags False
    sameLinkInfo = (BOOL*) MEM_malloc(sizeof(BOOL) * size);
    memset(sameLinkInfo, 0, (sizeof(BOOL) * size));

    for (i = 0; i < newRouterLSA->numLinks; i++)
    {
        int numTosNew = newLinkList->numTOS;

        oldLinkList = (Ospfv2LinkInfo*) (oldRouterLSA + 1);

        for (j = 0; j < oldRouterLSA->numLinks; j++)
        {
            int numTosOld = oldLinkList->numTOS;

            if (sameLinkInfo[j] != TRUE)
            {
                if ((newLinkList->linkID == oldLinkList->linkID)
                    && (newLinkList->type == oldLinkList->type))
                {
                    if (newLinkList->linkData == oldLinkList->linkData)
                    {
                        if (newLinkList->metric == oldLinkList->metric)
                        {
                            sameLinkInfo[j] = TRUE;
                            break;
                        }   //metric
                    }   //data
                }   //id
            }   //already matched

            oldLinkList = (Ospfv2LinkInfo*)
                ((QospfPerLinkQoSMetricInfo*)(oldLinkList + 1)
                      + numTosOld);
        }   // end of for;

        newLinkList = (Ospfv2LinkInfo*)
            ((QospfPerLinkQoSMetricInfo*)(newLinkList + 1)
                  + numTosNew);
    }   //end of for;

    // check if any link info in old LSA changed
    for (j = 0; j < oldRouterLSA->numLinks; j++)
    {
        if (sameLinkInfo[j] != TRUE)
        {
            MEM_free(sameLinkInfo);
            return TRUE;
        }
    }

    MEM_free(sameLinkInfo);
    return FALSE;
}


//-------------------------------------------------------------------------//
// FUNCTION    : Ospfv2CheckNetworkLSABody()
// PURPOSE     : Compare old & new LSA
// ASSUMPTION  : TOS Field remains unaltered
// RETURN VALUE: TRUE  if LSA changed
//-------------------------------------------------------------------------//

static
BOOL Ospfv2CheckNetworkLSABody(
    Ospfv2NetworkLSA* newNetworkLSA,
    Ospfv2NetworkLSA* oldNetworkLSA)
{
    int i;
    int j;
    BOOL* sameLinkInfo = NULL;

    NodeAddress* newAttachedRtr;
    NodeAddress* oldAttachedRtr;

    int newNumRtr = (newNetworkLSA->LSHeader.length
                            -  sizeof(Ospfv2NetworkLSA) - 4)
                            / (sizeof(NodeAddress));

    int oldNumRtr = (oldNetworkLSA->LSHeader.length
                            -  sizeof(Ospfv2NetworkLSA) - 4)
                            / (sizeof(NodeAddress));

    // If network mask or attached router has changed
    if ((memcmp(
        newNetworkLSA + 1, oldNetworkLSA + 1, sizeof(NodeAddress)) != 0)
        || (newNumRtr != oldNumRtr))
    {
        return TRUE;
    }

    // Make all flags False
    sameLinkInfo = (BOOL*) MEM_malloc(sizeof(BOOL) * oldNumRtr);
    memset(sameLinkInfo, 0, sizeof(BOOL) * oldNumRtr);

    newAttachedRtr = (NodeAddress*) (newNetworkLSA + 1) + 1;
    oldAttachedRtr = (NodeAddress*) (oldNetworkLSA + 1) + 1;

    for (i = 0; i < newNumRtr; i++)
    {
        for (j = 0; j < oldNumRtr; j++)
        {
            if (sameLinkInfo[j] != TRUE)
            {
                if (newAttachedRtr[i] == oldAttachedRtr[j])
                {
                    sameLinkInfo[j] = TRUE ;
                    break;
                }   //attached router
            }   //already matched ;
        }   // end of for;
    }   //end of for;

    // check if any link info in old LSA changed
    for (j = 0; j < oldNumRtr; j++)
    {
        if (sameLinkInfo[j] != TRUE)
        {
            MEM_free(sameLinkInfo);
            return TRUE;
        }
    }

    MEM_free(sameLinkInfo);
    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2CheckSummaryLSABody()
// PURPOSE      :Check body of two Summary LSA for any mismatch.
// ASSUMPTION   :None
// RETURN       :TRUE if contents have been changed, FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2CheckSummaryLSABody(
    Ospfv2LinkStateHeader* newLSHeader,
    Ospfv2LinkStateHeader* oldLSHeader)
{
    if (memcmp(newLSHeader + 1, oldLSHeader + 1, 8) != 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


// BGP-OSPF Patch Start
//-------------------------------------------------------------------------//
// NAME         :Ospfv2CheckASExternalLSABody()
// PURPOSE      :Check body of two AS External LSA for any mismatch.
// ASSUMPTION   :None
// RETURN       :TRUE if contents have been changed, FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2CheckASExternalLSABody(
    Ospfv2LinkStateHeader* newLSHeader,
    Ospfv2LinkStateHeader* oldLSHeader)
{
    if (memcmp(newLSHeader + 1,
               oldLSHeader + 1,
               sizeof(Ospfv2ExternalLinkInfo)) != 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
// BGP-OSPF Patch End

/***** Start: OPAQUE-LSA *****/
//-------------------------------------------------------------------------//
// NAME         :Ospfv2CheckASScopeOpaqueLSABody()
// PURPOSE      :Compare bodies of Type-11 LSAs for mismatch
// ASSUMPTION   :None
// PARAMETERS   :newLSHeader - Pointer to new LSA
//               oldLSHeader - Pointer to existing LSA
// RETURN VALUE :BOOL - TRUE if mismatch, FALSE otherwise
//-------------------------------------------------------------------------//
static
BOOL Ospfv2CheckASScopeOpaqueLSABody(
    Ospfv2ASOpaqueLSA* newLSHeader,
    Ospfv2ASOpaqueLSA* oldLSHeader)
{
#ifdef CYBER_CORE
#ifdef ADDON_BOEINGFCS
    if ((Ospfv2OpaqueGetOpaqueType(newLSHeader->LSHeader.linkStateID)
            == (unsigned char)HAIPE_ADDRESS_ADV) &&
        (Ospfv2OpaqueGetOpaqueType(oldLSHeader->LSHeader.linkStateID)
            == (unsigned char)HAIPE_ADDRESS_ADV))
    {
        return RoutingCesRospfCheckHaipeAddrAdvLSABody(newLSHeader,
            oldLSHeader);
    }
    else
#endif
#endif
    {
        ERROR_ReportError("Unknown Type-11 LSA!");
    }

    return FALSE;
}
/***** End: OPAQUE-LSA *****/

//-------------------------------------------------------------------------//
// NAME         :Ospfv2LSABodyChanged()
// PURPOSE      :Check body of two LSA for any mismatch.
// ASSUMPTION   :None
// RETURN       :TRUE if contents have been changed, FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2LSABodyChanged(
    Ospfv2LinkStateHeader* newLSHeader,
    Ospfv2LinkStateHeader* oldLSHeader)
{
    BOOL retVal = FALSE;

    switch (newLSHeader->linkStateType)
    {
        case OSPFv2_ROUTER:
        {
            retVal = Ospfv2CheckRouterLSABody(
                            (Ospfv2RouterLSA*)newLSHeader,
                            (Ospfv2RouterLSA*) oldLSHeader);
            break;
        }

        case OSPFv2_NETWORK:
        {
            retVal = Ospfv2CheckNetworkLSABody(
                            (Ospfv2NetworkLSA*) newLSHeader,
                            (Ospfv2NetworkLSA*) oldLSHeader);
            break;
        }

        case OSPFv2_NETWORK_SUMMARY:
        case OSPFv2_ROUTER_SUMMARY:
        {
            retVal = Ospfv2CheckSummaryLSABody(newLSHeader,
                                               oldLSHeader);
            break;
        }

        // BGP-OSPF Patch Start
        case OSPFv2_ROUTER_AS_EXTERNAL:
        case OSPFv2_ROUTER_NSSA_EXTERNAL:
        {
            retVal = Ospfv2CheckASExternalLSABody(newLSHeader,
                                               oldLSHeader);
            break;
        }
        // BGP-OSPF Patch End

        case OSPFv2_GROUP_MEMBERSHIP:
        {
            return MospfCheckGroupMembershipLSABody(
                           (MospfGroupMembershipLSA*)newLSHeader,
                           (MospfGroupMembershipLSA*) oldLSHeader);
        }

        /***** Start: OPAQUE-LSA *****/
        case OSPFv2_AS_SCOPE_OPAQUE:
        {
            return Ospfv2CheckASScopeOpaqueLSABody(
                           (Ospfv2ASOpaqueLSA*) newLSHeader,
                           (Ospfv2ASOpaqueLSA*) oldLSHeader);
        }
        /***** End: OPAQUE-LSA *****/

        default:
        {
//#ifdef JNE_LIB
//            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
//                "Not a valid linkStateType",
//                JNE::CRITICAL);
//#endif
            ERROR_Assert(FALSE, "Not a valid linkStateType\n");
        }
    }

    return retVal;
}

static
BOOL Ospfv2DemandCircuitEnabledForThisArea(Node* node,
                                           unsigned int areaId,
                                           Ospfv2Data* ospf)
{
    int i;
    int demandCount = 0;
    int intfCount = 0;



    for (i=0; i<node->numberInterfaces; i++)
    {
        if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            continue;
        }

        if (areaId == OSPFv2_SINGLE_AREA_ID ||
            areaId == OSPFv2_INVALID_AREA_ID)
        {
            intfCount++;
            if (ospf->iface[i].useDemandCircuit)
            {
                demandCount++;
            }
        }
        else if (Ospfv2InterfaceBelongToThisArea(node, areaId, i))
        {
            intfCount++;
            if (ospf->iface[i].useDemandCircuit)
            {
                demandCount++;
            }
        }
    }

    if (demandCount > 0)
    {
        // RFC 1793: LSAs in regular OSPF areas are allowed to
        // have DoNotAge set if and only if every router in the OSPF
        // domain (excepting those in stub areas and NSSAs) is capable of
        // DoNotAge processing.

#ifdef JNE_LIB
        if (intfCount != demandCount)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "No support for different interfaces "
                "connected to same area with different"
                " demand circuit configurations...",
                JNE::CRITICAL);
        }
#endif
        ERROR_Assert(intfCount == demandCount,
            "No support for different interfaces "
            "connected to same area with different"
            " demand circuit configurations...");

        return TRUE;
    }

    return FALSE;

}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2IncrementLSAgeInLSAList
// PURPOSE      :Increment the link state age field of LSAs stored in the
//               LSDB and handle appropriately if this age field passes
//               a maximum age limit.
// ASSUMPTION   :None.
//-------------------------------------------------------------------------//

static
void Ospfv2IncrementLSAgeInLSAList(
    Node* node,
    Ospfv2List* list,
    unsigned int areaId)
{
    Ospfv2ListItem* item = list->first;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    while (item)
    {
        short int tempAge;

        Ospfv2LinkStateHeader* LSHeader =
            (Ospfv2LinkStateHeader*) item->data;

        if (OSPFv2_DEBUG_LSDBErr)
        {
            char routerStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(LSHeader->advertisingRouter,
                                        routerStr);

            printf("    LSA for node %s age was %d seconds type %d\n",
                    routerStr,
                    LSHeader->linkStateAge,
                    LSHeader->linkStateType);
        }

        if (Ospfv2CheckLSAForDoNotAge(ospf,
                                      LSHeader->linkStateAge))
        {
            tempAge = Ospfv2MaskDoNotAge(ospf,
                (short int) LSHeader->linkStateAge);
        }
        else
        {
            tempAge = (Ospfv2MaskDoNotAge(ospf, (short int) LSHeader->linkStateAge))
                      + (short int)(ospf->incrementTime / SECOND);
        }

        //As tempAge does not have the DoNotAge bit set so no need to change
        //this checking
        if (tempAge > (OSPFv2_LSA_MAX_AGE / SECOND))
        {
            tempAge = OSPFv2_LSA_MAX_AGE / SECOND;
        }

        if (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) ==
                                            (OSPFv2_LSA_MAX_AGE / SECOND))
        {
            item = item->next;
            continue;
        }

        // Increment LS age.
        Ospfv2AssignNewAgeToLSAAge(ospf,
                                   LSHeader->linkStateAge,
                                   tempAge);

        if (OSPFv2_DEBUG_LSDBErr)
        {
            char routerStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(LSHeader->advertisingRouter,
                                        routerStr);

            printf("    LSA for node %s now has age %d seconds type %d\n",
                    routerStr,
                    LSHeader->linkStateAge,
                    LSHeader->linkStateType);
        }


        // LS Age field of Self originated LSA reaches LSRefreshTime
        if ((LSHeader->advertisingRouter == ospf->routerID)
            && ((Ospfv2MaskDoNotAge(ospf, tempAge) ==
            (OSPFv2_LS_REFRESH_TIME / SECOND))))
        {
            Ospfv2ListItem* tempItem;

            tempItem = item;
            item = item->next;

            if ((LSHeader->linkStateType >= OSPFv2_ROUTER
                && LSHeader->linkStateType <= OSPFv2_ROUTER_AS_EXTERNAL)
                || (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
                /***** Start: OPAQUE-LSA *****/
                || (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
                /***** End: OPAQUE-LSA *****/
            {
                Ospfv2RefreshLSA(node, tempItem, areaId);
            }

            // M-OSPF Patch Start
            else if (LSHeader->linkStateType == OSPFv2_GROUP_MEMBERSHIP)
            {
                if (OSPFv2_DEBUG_LSDBErr)
                {
                    printf("Node %d: Refreshing Grm_LSA for area %d\n",
                            node->nodeId, areaId);
                }

                MospfScheduleGroupMembershipLSA(node, areaId,
                    (unsigned) ANY_INTERFACE, LSHeader->linkStateID, FALSE);
            }
            // M-OSPF Patch End
        }
        // Expired, so remove from LSDB and flood.
        else if (Ospfv2MaskDoNotAge(ospf, tempAge) ==
                (OSPFv2_LSA_MAX_AGE / SECOND))
        {
            item = item->next;

            if (Ospfv2DebugFlood(node) || OSPFv2_DEBUG_LSDBErr)
            {
                printf("    LSA for node %u deleted from LSDB\n",
                        LSHeader->advertisingRouter);
            }

            ospf->stats.numExpiredLSAge++;

            // Flood deleted LSA to quickly inform others of the invalid LSA.
            if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
                /***** Start: OPAQUE-LSA *****/
                || (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
                /***** End: OPAQUE-LSA *****/
            {
                Ospfv2FloodThroughAS(node, ANY_INTERFACE, (char*) LSHeader,
                    ANY_DEST);
                Ospfv2ListItem* listItem = ospf->area->first;

                while (listItem)
                {
                    Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;
#ifdef JNE_LIB
                    if (!thisArea)
                    {
                        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                              "Area structure not initialized",
                              JNE::CRITICAL);
                    }
#endif
                    ERROR_Assert(thisArea, "Area structure not initialized");
                    if (thisArea->externalRoutingCapability)
                    {
                        Ospfv2AddLsaToMaxAgeLsaList(node,
                                                    (Int8*) LSHeader,
                                                    thisArea->areaID);
                    }

                    listItem = listItem->next;
                }
            }
            else if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
            {
                Ospfv2FloodThroughNSSA(node,
                                       ANY_INTERFACE,
                                       (char*) LSHeader,
                                       ANY_DEST,
                                       areaId);
                Ospfv2ListItem* listItem = ospf->area->first;

                while (listItem)
                {
                    Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;
#ifdef JNE_LIB
                    if (!thisArea)
                    {
                        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                        "Area structure not initialized",
                        JNE::CRITICAL);
                    }
#endif
                    ERROR_Assert(thisArea, "Area structure not initialized");

                    if (thisArea->isNSSAEnabled)
                    {
                        Ospfv2AddLsaToMaxAgeLsaList(node,
                                                    (Int8*) LSHeader,
                                                    thisArea->areaID);
                    }

                    listItem = listItem->next;
                }
            }
            else
            {
                Ospfv2FloodLSA(node,
                               ANY_INTERFACE,
                               (char*)LSHeader,
                               ANY_DEST,
                               areaId);
                Ospfv2AddLsaToMaxAgeLsaList(node, (char*) LSHeader, areaId);
            }
            Ospfv2ScheduleSPFCalculation(node);
        }
        else
        {
            item = item->next;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2IncrementLSAge
// PURPOSE      :Increment the link state age field of LSAs stored in LSDB.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2IncrementLSAge(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;

        Ospfv2IncrementLSAgeInLSAList(node,
                                      thisArea->networkLSAList,
                                      thisArea->areaID);
        Ospfv2IncrementLSAgeInLSAList(node,
                                      thisArea->routerLSAList,
                                      thisArea->areaID);
        Ospfv2IncrementLSAgeInLSAList(node,
                                      thisArea->routerSummaryLSAList,
                                      thisArea->areaID);
        Ospfv2IncrementLSAgeInLSAList(node,
                                      thisArea->networkSummaryLSAList,
                                      thisArea->areaID);

        if (ospf->isMospfEnable == TRUE)
        {
            Ospfv2IncrementLSAgeInLSAList(
                node,
                thisArea->groupMembershipLSAList,
                thisArea->areaID);
        }
    }

    // BGP-OSPF Patch Start
    //Ospfv2IncrementLSAgeInLSAList(node, ospf->asExternalLSAList, 0);
    Ospfv2IncrementLSAgeInLSAList(
        node, ospf->asExternalLSAList, OSPFv2_INVALID_AREA_ID);
    // BGP-OSPF Patch End

    Ospfv2IncrementLSAgeInLSAList(
        node, ospf->nssaExternalLSAList, OSPFv2_INVALID_AREA_ID);

    //RFC:1793::Section 2.3
    //As increment LSA is called after periodic intervals so
    //removal of stale DoNotAge LSAs can be done here
    //(1) The LSA has been in the router’s database
    //for at least MaxAge seconds.
    //(2) The originator of the LSA has been unreachable (according to
    //the routing calculations specified by Section 16 of [1]) for
    //at least MaxAge seconds
    if (ospf->supportDC == TRUE)
    {
        Ospfv2RemoveStaleDoNotAgeLSA(node, ospf);
    }

    /***** Start: OPAQUE-LSA *****/
    Ospfv2IncrementLSAgeInLSAList(node,
                                  ospf->ASOpaqueLSAList,
                                  OSPFv2_INVALID_AREA_ID);
    /***** End: OPAQUE-LSA *****/
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2ResetAdvrtToAreaFlag()
// PURPOSE      :Reset advrtToArea flag within Ospfv2AreaAddressRange struct
//               for each area.
// ASSUMPTION   :None
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2ResetAdvrtToAreaFlag(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;
        Ospfv2ListItem* addrListItem = thisArea->areaAddrRange->first;

        while (addrListItem)
        {
            int i;
            Ospfv2AreaAddressRange* addrInfo
                = (Ospfv2AreaAddressRange*) addrListItem->data;

            for (i = 0; i < OSPFv2_MAX_ATTACHED_AREA; i++)
            {
                addrInfo->advrtToArea[i] = FALSE;
            }

            addrListItem = addrListItem->next;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2GetMaxMetricForThisRange()
// PURPOSE      :Get largest cost of any of the component networks within
//               the specified address range.
// ASSUMPTION   :None
// RETURN VALUE :Maximum cost.
//-------------------------------------------------------------------------//

Int32 Ospfv2GetMaxMetricForThisRange(
    Ospfv2RoutingTable* rtTable,
    Ospfv2AreaAddressRange* addrInfo)
{
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);
    int i;
    Int32 maxMetric = 0;
    Ospfv2Area* thisArea = (Ospfv2Area*) addrInfo->area;

    for (i = 0; i < rtTable->numRows; i++)
    {
        if (((addrInfo->address & addrInfo->mask) ==
            (rowPtr[i].destAddr & addrInfo->mask))
            && (rowPtr[i].areaId == thisArea->areaID)
            && (rowPtr[i].pathType == OSPFv2_INTRA_AREA)
            && (rowPtr[i].metric > maxMetric)
            && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID))
        {
            maxMetric = rowPtr[i].metric;
        }
    }

    return maxMetric;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2AdvertiseRoute()
// PURPOSE      :Advertise this route
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
BOOL Ospfv2AdvertiseRoute(
    Node* node,
    Ospfv2RoutingTableRow* thisRoute)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem = NULL;
    Ospfv2Area* area = NULL;
    int count = 0;
    BOOL originateDefaultSummary = FALSE;
    BOOL generatedASExternalLSA = FALSE;
    BOOL isMobileLeaf = FALSE;

    /***** Start: ROSPF Redirect *****/
#ifdef ADDON_BOEINGFCS
    generatedASExternalLSA =
        RoutingCesRospfAdvertiseASExtRouteToMobileLeafArea(node,
                                                           thisRoute);
    if (ospf->iface[thisRoute->outIntf].type == OSPFv2_ROSPF_INTERFACE)
    {
        isMobileLeaf = RoutingCesRospfIsMobileLeafNode(node, thisRoute->outIntf);
    }
#endif
    /***** End: ROSPF Redirect *****/

    for (listItem = ospf->area->first; listItem;
            listItem = listItem->next, count++)
    {
        area = (Ospfv2Area*) listItem->data;

        // Inter-Area routes are never advertised to backbone
        if ((thisRoute->pathType == OSPFv2_INTER_AREA)
            && (area->areaID == OSPFv2_BACKBONE_AREA))
        {
            continue;
        }

        // Do not generate Summary LSA if the area associated
        // with this set of path is the area itself
        if (thisRoute->areaId == area->areaID)
        {
            continue;
        }

        // If next hop belongs to this area ...
        if (Ospfv2NextHopBelongsToThisArea(node, thisRoute, area))
        {
            continue;
        }

        if (area->externalRoutingCapability == FALSE)
            originateDefaultSummary = TRUE;

        //No summary configured for NSSA area
        if ((thisRoute->pathType == OSPFv2_INTER_AREA)
            && (area->isNSSANoSummary == TRUE))
        {
            originateDefaultSummary = TRUE;
            continue;
        }

        if ((thisRoute->destType == OSPFv2_DESTINATION_ASBR
            || thisRoute->destType == OSPFv2_DESTINATION_ABR_ASBR))
        {
            Ospfv2RoutingTableRow* preferRoute;
            preferRoute = Ospfv2GetPreferredPath(node,
                                                 thisRoute->destAddr,
                                                 thisRoute->destType);
            if (Ospfv2RoutesMatchSame(thisRoute, preferRoute) &&
                (area->externalRoutingCapability == TRUE) &&
                (generatedASExternalLSA == FALSE))
            {
                Ospfv2OriginateSummaryLSA(node, thisRoute, area, FALSE);
            }
       }
        else if (thisRoute->pathType == OSPFv2_INTER_AREA)
        {
            Ospfv2OriginateSummaryLSA(node, thisRoute, area, FALSE);
        }
        else
        {
            // This is an Intra area route to a network which is contained
            // in one of the router's directly attached network
            Ospfv2AreaAddressRange* addrInfo = Ospfv2GetAddrRangeInfo(
                                                node,
                                                thisRoute->areaId,
                                                thisRoute->destAddr);

            // 1) By default, if a network is not contained in any explicitly
            //    configured address range, a Type 3 summary-LSA is generated
            //
            // 2) The backbone's configured ranges should be ignored when
            //    originating summary-LSAs into transit areas.
            //
            // 3) When range's status indicates DoNotAdvertise, the Type 3
            //    summary-LSA is suppressed and the component networks remain
            //    hidden from other areas.
            //
            // 4) At most a single type3 Summary-LSA is
            //    generated for each range

            if (((!addrInfo && !isMobileLeaf)
                || ((thisRoute->areaId == OSPFv2_BACKBONE_AREA)
                    && (area->transitCapability == TRUE)))
                && (generatedASExternalLSA == FALSE))
            {
                Ospfv2OriginateSummaryLSA(node, thisRoute, area, FALSE);
            }
            else if (addrInfo && addrInfo->advertise == TRUE
                && addrInfo->advrtToArea[count] == FALSE)
            {
                //No summary configured for NSSA area
                if (area->isNSSANoSummary == TRUE)
                {
                    originateDefaultSummary = TRUE;
                    continue;
                }

                if (generatedASExternalLSA == FALSE)
                {
                    Ospfv2RoutingTableRow advRt;

                    memset(&advRt, 0, sizeof(Ospfv2RoutingTableRow));

                    advRt.destType = OSPFv2_DESTINATION_NETWORK;
                    advRt.destAddr = addrInfo->address;
                    advRt.addrMask = addrInfo->mask;
                    advRt.flag = OSPFv2_ROUTE_NO_CHANGE;

                    advRt.metric = Ospfv2GetMaxMetricForThisRange(
                                        &ospf->routingTable, addrInfo);

                    Ospfv2OriginateSummaryLSA(node, &advRt, area, FALSE);
                    addrInfo->advrtToArea[count] = TRUE;
                }
            }
            else
            {
                // DO nothing.
            }
        }
    }

    return originateDefaultSummary;
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2CheckOldSummaryLSA()
// PURPOSE      :Supporting function for Ospfv2HandleRemovedOrChangedRoutes.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2CheckOldSummaryLSA(
    Node* node,
    Ospfv2Area* thisArea,
    Ospfv2LinkStateType lsType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2RoutingTableRow* rowPtr = NULL;
    Ospfv2List* list = NULL;
    Ospfv2ListItem* listItem = NULL;

    if (lsType == OSPFv2_NETWORK_SUMMARY)
    {
        list = thisArea->networkSummaryLSAList;
    }
    else if (lsType == OSPFv2_ROUTER_SUMMARY)
    {
        list = thisArea->routerSummaryLSAList;
    }
    else
    {
#ifdef JNE_LIB
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Invalid Summary LSA type.",
            JNE::CRITICAL);
#endif
        ERROR_Assert(FALSE, "Invalid Summary LSA type.");
    }

    listItem = list->first;

    while (listItem)
    {
        Ospfv2LinkStateHeader* LSHeader = NULL;
        char* LSABody = NULL;
        NodeAddress destAddr;
        NodeAddress addrMask;
        Ospfv2DestType destType;
        int retVal = 0;

        LSHeader = (Ospfv2LinkStateHeader*) listItem->data;
        LSABody = (char*) (LSHeader + 1);

        // Process only self originated LSA
        if (LSHeader->advertisingRouter != ospf->routerID)
        {
            listItem = listItem->next;
            continue;
        }

        destAddr = LSHeader->linkStateID;
        memcpy(&addrMask, LSABody, sizeof(NodeAddress));

        if (lsType == OSPFv2_NETWORK_SUMMARY)
        {
            destType = OSPFv2_DESTINATION_NETWORK;
        }
        else
        {
            destType = OSPFv2_DESTINATION_ASBR;
            addrMask = 0xFFFFFFFF;
        }

        // Lookup the routing table entry for an entry to this route
        retVal = Ospfv2CheckForSummaryLSAValidity(
                    node, destAddr, addrMask, destType,
                    thisArea->areaID, &rowPtr);

        if (!retVal)
        {
            listItem = listItem->next;
            Ospfv2FlushLSA(node, (char*) LSHeader, thisArea->areaID);
            continue;
        }

        listItem = listItem->next;
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleRemovedOrChangedRoutes()
// PURPOSE      :Some old route may not be flushed by the normal procedure
//               desribed in section 12.4.3 (RFC2328). Take extra care for
//               those routes.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2HandleRemovedOrChangedRoutes(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2ListItem* listItem = NULL;
    Ospfv2Area* area = NULL;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        area = (Ospfv2Area*) listItem->data;

        // Examine all summary LSAs originated by me for this area.
        Ospfv2CheckOldSummaryLSA(node, area, OSPFv2_NETWORK_SUMMARY);
        Ospfv2CheckOldSummaryLSA(node, area, OSPFv2_ROUTER_SUMMARY);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleABRTask()
// PURPOSE      :Handle any responsibility occur due to become an ABR
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void Ospfv2HandleABRTask(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    int i;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&ospf->routingTable.buffer);

    BOOL originateDefaultForAttachedStubArea = FALSE;
    BOOL isHighestId = FALSE;
    BOOL wasFound = FALSE;

    // Check entire routing table for added, deleted or changed
    // Intra-Area or Inter-Area routes that need to be advertised

    Ospfv2ResetAdvrtToAreaFlag(node);

    if (ospf->nssaAreaBorderRouter)
    {
        //check for highest router ID
        isHighestId = Ospfv2IsHighestRouterID(node, ospf->routerID);
    }

    for (i = 0; i < ospf->routingTable.numRows; i++)
    {
        // Examine next route if this have not been changed
        if (rowPtr[i].flag == OSPFv2_ROUTE_NO_CHANGE)
        {
            continue;
        }

        if (rowPtr[i].flag == OSPFv2_ROUTE_INVALID)
        {
            continue;
        }

        // Only destination of type network and ASBR needs to be advertised
        if (rowPtr[i].destType == OSPFv2_DESTINATION_ABR)
        {
            continue;
        }

        // External routes area never advertised in Summary LSA
        if ((rowPtr[i].pathType == OSPFv2_TYPE1_EXTERNAL)
            || (rowPtr[i].pathType == OSPFv2_TYPE2_EXTERNAL))
        {
            originateDefaultForAttachedStubArea = TRUE;

            //Searching Type 7 LSA for translating into Type 5
            if (ospf->nssaAreaBorderRouter)
            {
                //check for highest router ID
                if (!isHighestId)
                {
                    continue;
                }

                Ospfv2LinkStateHeader* LSHeader =
                                (Ospfv2LinkStateHeader*) rowPtr[i].LSOrigin;

                if (LSHeader->linkStateType != OSPFv2_ROUTER_NSSA_EXTERNAL)
                {
                    continue;
                }

                Ospfv2ExternalLinkInfo* LSABody =
                                   (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
                NodeAddress forwardingAdd = LSABody->forwardingAddress;

                if (!(Ospfv2OptionsGetNSSACapability(LSHeader->options)))
                {
                    continue;
                }
                if (!forwardingAdd)
                {
                    continue;
                }

                wasFound = FALSE;

                //check for DoNotAdvertise status
                for (unsigned int j = 0;
                                 j < ospf->nssaNotAdvertiseList.size();
                                 j++)
                {
                   if ((rowPtr[i].destAddr ==
                                       ospf->nssaNotAdvertiseList[j].address)
                      && (rowPtr[i].addrMask ==
                                         ospf->nssaNotAdvertiseList[j].mask))
                   {
                      Ospfv2OptionsSetNSSACapability(&(LSHeader->options),0);
                      wasFound = TRUE;
                      break;
                   }
                }

                if (wasFound)
                {
                    continue;
                }

                Ospfv2TranslateType7ToType5(node, &rowPtr[i]);
            }//if (ospf->nssaAreaBorderRouter)

            continue;
        }

        // Summary LSA shouldn't be generated for a route whose
        // cost equal equal or exceed the value LSInfinity

        if (rowPtr[i].metric >= OSPFv2_LS_INFINITY)
        {
            continue;
        }

        // Advertise this route
        BOOL originateDefaultSummary = Ospfv2AdvertiseRoute(node,&rowPtr[i]);
        if (originateDefaultSummary)
        {
            originateDefaultForAttachedStubArea = TRUE;
        }

    }

    //advertise Type 5 LSAs pointed through nssaAdvertiseList, if any kept
    // pending in function Ospfv2TranslateType7ToType5() for Advertise list
    Ospfv2FloodLSAKeptInAdvertiseList(node);


    // If the destination is still reachable, yet can no longer be
    // advertised according to the above procedure (e.g., it is now
    // an inter-area route, when it used to be an intra-area route
    // associated with some non-backbone area; it would thus no longer
    // be advertisable to the backbone), the LSA should also be flushed
    // from the routing domain.
    Ospfv2HandleRemovedOrChangedRoutes(node);

    if (originateDefaultForAttachedStubArea)
    {
        Ospfv2OriginateDefaultSummaryLSA(node);
    }

    if (ospf->supportDC == TRUE)
    {
        Ospfv2IndicatingAcrossAreasForDoNotAge(node);
    }
}


//-------------------------------------------------------------------------//
//                        HANDLING ROUTING TABLE                           //
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// NAME:    Ospfv2RouterFunction
// PURPOSE: Address of this function is assigned in the IP structure
//          during initialization. IP forwards packet by getting the nextHop
//          from this function. This fuction will be called via a pointer,
//          RouterFunction, in the IP structure, from function RoutePacket
//          AndSendToMac() in nwip.pc.
// RETURN:  None.
//-------------------------------------------------------------------------//

void Ospfv2RouterFunction(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress prevHopAddr,
    BOOL* packetWasRouted)
{

    // Let IP do the forwarding.
    *packetWasRouted = FALSE;

    return;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2InitRoutingTable
// PURPOSE: Allocate memory for initial routing table.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2InitRoutingTable(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int size = sizeof(Ospfv2RoutingTableRow) * OSPFv2_INITIAL_TABLE_SIZE;

    BUFFER_InitializeDataBuffer(&ospf->routingTable.buffer, size);
    ospf->routingTable.numRows = 0;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2InvalidateRoutingTable
// PURPOSE: Invalidate present routing table
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2InvalidateRoutingTable(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&ospf->routingTable.buffer);

    for (i = 0; i < ospf->routingTable.numRows; i++)
    {
#ifdef ADDON_MA
        // If ADDON_MA on, but not runing with MA
        if (!ospf->maMode)
        {
            if (ospf->asBoundaryRouter &&
                (rowPtr[i].pathType == OSPFv2_TYPE2_EXTERNAL))
            {
                continue;
            }
        }
#else
        if ((node->appData.exteriorGatewayProtocol ==
                APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4) &&
            (ospf->asBoundaryRouter) &&
#ifdef ADDON_BOEINGFCS
            !ModeCesWnwReceiveOnlyReturnCurrentState(node,0) &&
#endif
            (rowPtr[i].pathType == OSPFv2_TYPE2_EXTERNAL))
        {
            continue;
        }
#endif


        rowPtr[i].flag = OSPFv2_ROUTE_INVALID;
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2CompDestType()
// PURPOSE:  Compare two Dest type are same or not.
// RETURN: Return TRUE if the dest types are same otherwise FALSE.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2CompDestType(
    Ospfv2DestType destType1,
    Ospfv2DestType destType2)
{
    if (destType1 == destType2)
    {
        return TRUE;
    }
    else if (destType1 == OSPFv2_DESTINATION_ABR_ASBR
        && ((destType2 == OSPFv2_DESTINATION_ABR)
            || (destType2 == OSPFv2_DESTINATION_ASBR)))
    {
        return TRUE;
    }
    else if (destType2 == OSPFv2_DESTINATION_ABR_ASBR
        && ((destType1 == OSPFv2_DESTINATION_ABR)
            || (destType1 == OSPFv2_DESTINATION_ASBR)))
    {
        return TRUE;
    }

    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2GetRoute()
// PURPOSE:  Get desired route from routing table
// RETURN: Route entry. NULL if route not found.
//-------------------------------------------------------------------------//

static
Ospfv2RoutingTableRow* Ospfv2GetRoute(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);
    Ospfv2RoutingTableRow* longestMatchingEntry = NULL;

    for (i = 0; i < rtTable->numRows; i++)
    {
        // BGP-OSPF Patch Start
        if ((rowPtr[i].destAddr == 0) && destAddr)
        {
            continue;
        }
        // BGP-OSPF Patch End
        if (ospf->isAdvertSelfIntf)
        {
            if ((rowPtr[i].destAddr == destAddr)
                && Ospfv2CompDestType(rowPtr[i].destType, destType))
            {
                return &rowPtr[i];
            }
        }
        else
        {
            if ((rowPtr[i].destAddr & rowPtr[i].addrMask) ==
                    (destAddr & rowPtr[i].addrMask)
                && Ospfv2CompDestType(rowPtr[i].destType, destType))
            {
                if (!longestMatchingEntry
                    || rowPtr[i].addrMask > longestMatchingEntry->addrMask)
                {
                    longestMatchingEntry = &rowPtr[i];
                }
            }
        }
    }
    return longestMatchingEntry;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2GetIntraAreaRoute()
// PURPOSE:  Get desired route from routing table
// RETURN: Route entry. NULL if route not found.
//-------------------------------------------------------------------------//

static
Ospfv2RoutingTableRow* Ospfv2GetIntraAreaRoute(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType,
    NodeAddress areaId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);
    Ospfv2RoutingTableRow* longestMatchingEntry = NULL;
    for (i = 0; i < rtTable->numRows; i++)
    {
        if (ospf->isAdvertSelfIntf)
        {
            if ((rowPtr[i].destAddr == destAddr)
                && Ospfv2CompDestType(rowPtr[i].destType, destType)
                && (rowPtr[i].pathType == OSPFv2_INTRA_AREA)
                && (rowPtr[i].areaId == areaId))
            {
                if (!longestMatchingEntry
                    || rowPtr[i].addrMask > longestMatchingEntry->addrMask)
                {
                    longestMatchingEntry = &rowPtr[i];
                }
            }
        }
        else
        {
            if ((rowPtr[i].destAddr & rowPtr[i].addrMask) ==
                    (destAddr & rowPtr[i].addrMask)
                && Ospfv2CompDestType(rowPtr[i].destType, destType)
                && (rowPtr[i].pathType == OSPFv2_INTRA_AREA)
                && (rowPtr[i].areaId == areaId))
            {
                if (!longestMatchingEntry
                    || rowPtr[i].addrMask > longestMatchingEntry->addrMask)
                {
                    longestMatchingEntry = &rowPtr[i];
                }
            }
        }
    }
    return longestMatchingEntry;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2GetValidHostRoute()
// PURPOSE:  Get desired route from routing table
// RETURN: Route entry. NULL if route not found.
//-------------------------------------------------------------------------//

Ospfv2RoutingTableRow* Ospfv2GetValidHostRoute(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        // BGP-OSPF Patch Start
        if ((rowPtr[i].destAddr == 0) && destAddr)
        {
            continue;
        }
        // BGP-OSPF Patch End

        if ((rowPtr[i].destAddr == destAddr)
            && Ospfv2CompDestType(rowPtr[i].destType, destType)
            && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID))
        {
            return &rowPtr[i];
        }
    }
    return NULL;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2GetHostRoute()
// PURPOSE:  Get desired route from routing table
// RETURN: Route entry. NULL if route not found.
//-------------------------------------------------------------------------//

Ospfv2RoutingTableRow* Ospfv2GetHostRoute(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        // BGP-OSPF Patch Start
        if ((rowPtr[i].destAddr == 0) && destAddr)
        {
            continue;
        }
        // BGP-OSPF Patch End

        if ((rowPtr[i].destAddr == destAddr)
            && Ospfv2CompDestType(rowPtr[i].destType, destType))
        {
            return &rowPtr[i];
        }
    }
    return NULL;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2GetValidRoute()
// PURPOSE:  Get desired route from routing table
// RETURN: Route entry. NULL if route not found.
//-------------------------------------------------------------------------//

Ospfv2RoutingTableRow* Ospfv2GetValidRoute(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);
    Ospfv2RoutingTableRow* longestMatchingEntry = NULL;
    for (i = 0; i < rtTable->numRows; i++)
    {
        // BGP-OSPF Patch Start
        if ((rowPtr[i].destAddr == 0) && destAddr)
        {
            continue;
        }
        // BGP-OSPF Patch End

        if ((interfaceIndex != ANY_INTERFACE) &&
            (rowPtr[i].outIntf != interfaceIndex))
        {
            continue;
        }

        if (ospf->isAdvertSelfIntf)
        {
            if ((rowPtr[i].destAddr == destAddr)
                && Ospfv2CompDestType(rowPtr[i].destType, destType)
                && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID))
            {
                return &rowPtr[i];
            }
        }
        else
        {
            if ((rowPtr[i].destAddr & rowPtr[i].addrMask) ==
                    (destAddr & rowPtr[i].addrMask)
                && Ospfv2CompDestType(rowPtr[i].destType, destType)
                && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID))
            {
                if (!longestMatchingEntry
                    || rowPtr[i].addrMask > longestMatchingEntry->addrMask)
                {
                    longestMatchingEntry = &rowPtr[i];
                }
            }
        }
    }
    return longestMatchingEntry;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2GetRouteToSrcNet()
// PURPOSE:  Get desired route from routing table to destination.
//           This function search for closest match to a destination.
// RETURN: Route entry. NULL if route not found.
//-------------------------------------------------------------------------//

Ospfv2RoutingTableRow* Ospfv2GetRouteToSrcNet(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);
    Ospfv2RoutingTableRow* longestMatchingEntry = NULL;

    for (i = 0; i < rtTable->numRows; i++)
    {
        // BGP-OSPF Patch Start
        if ((rowPtr[i].destAddr == 0) && destAddr)
        {
            continue;
        }
        // BGP-OSPF Patch End

        if ((rowPtr[i].destAddr & rowPtr[i].addrMask) ==
                (destAddr & rowPtr[i].addrMask)
            && Ospfv2CompDestType(rowPtr[i].destType, destType)
            && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID))
        {
            if (!longestMatchingEntry
                || rowPtr[i].addrMask > longestMatchingEntry->addrMask)
            {
                longestMatchingEntry = &rowPtr[i];
            }
        }

    }

    return longestMatchingEntry;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2GetBackboneValidRoute()
// PURPOSE:  Get desired route from routing table for Backbone area.
// RETURN: Route entry. NULL if route not found.
//-------------------------------------------------------------------------//

Ospfv2RoutingTableRow* Ospfv2GetBackboneValidRoute(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTableRow* longestMatchingEntry = NULL;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        // BGP-OSPF Patch Start
        if ((rowPtr[i].destAddr == 0) && destAddr)
        {
            continue;
        }
        // BGP-OSPF Patch End

        if (ospf->isAdvertSelfIntf)
        {
            if ((rowPtr[i].destAddr == destAddr)
                && Ospfv2CompDestType(rowPtr[i].destType, destType)
                && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID)
                && (rowPtr[i].areaId == OSPFv2_BACKBONE_AREA))
            {
                if (!longestMatchingEntry
                    || rowPtr[i].addrMask > longestMatchingEntry->addrMask)
                {
                    longestMatchingEntry = &rowPtr[i];
                }
             //   return &rowPtr[i];
            }
        }
        else
        {
            if ((rowPtr[i].destAddr & rowPtr[i].addrMask) ==
                    (destAddr & rowPtr[i].addrMask)
                && Ospfv2CompDestType(rowPtr[i].destType, destType)
                && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID)
                && (rowPtr[i].areaId == OSPFv2_BACKBONE_AREA))
            {
                if (!longestMatchingEntry
                    || rowPtr[i].addrMask > longestMatchingEntry->addrMask)
                {
                    longestMatchingEntry = &rowPtr[i];
                }
            }
        }
    }
    return longestMatchingEntry;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2GetPreferredPath()
// PURPOSE:  Get desired route from routing table
// RETURN: Route entry. NULL if route not found.
//-------------------------------------------------------------------------//

Ospfv2RoutingTableRow* Ospfv2GetPreferredPath(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);
    Ospfv2RoutingTableRow* longestMatchingEntry = NULL;

    for (i = 0; i < rtTable->numRows; i++)
    {
        // BGP-OSPF Patch Start
        if ((rowPtr[i].destAddr == 0) && destAddr)
        {
            continue;
        }
        // BGP-OSPF Patch End

        if ((rowPtr[i].destAddr & rowPtr[i].addrMask) ==
                (destAddr & rowPtr[i].addrMask)
            && Ospfv2CompDestType(rowPtr[i].destType, destType)
            && (rowPtr[i].pathType == OSPFv2_INTRA_AREA)
            && (rowPtr[i].areaId != OSPFv2_BACKBONE_AREA)
            && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID))
        {
            if (longestMatchingEntry == NULL)
            {
                longestMatchingEntry = &rowPtr[i];
            }
            else if (rowPtr[i].addrMask == longestMatchingEntry->addrMask)
            {
                if (rowPtr[i].metric == longestMatchingEntry->metric)
                {
                    if (rowPtr[i].areaId > longestMatchingEntry->areaId)
                    {
                        longestMatchingEntry = &rowPtr[i];
                    }
                }
                else if (rowPtr[i].metric > longestMatchingEntry->metric)
                {
                    longestMatchingEntry = &rowPtr[i];
                }
            }
            else if (rowPtr[i].addrMask > longestMatchingEntry->addrMask)
            {
                longestMatchingEntry = &rowPtr[i];
            }
        }
    }

    if (longestMatchingEntry == NULL)
    {
        for (i = 0; i < rtTable->numRows; i++)
        {
            // BGP-OSPF Patch Start
            if ((rowPtr[i].destAddr == 0) && destAddr)
            {
                continue;
            }
            // BGP-OSPF Patch End

            if ((rowPtr[i].destAddr & rowPtr[i].addrMask) ==
                    (destAddr & rowPtr[i].addrMask)
                && Ospfv2CompDestType(rowPtr[i].destType, destType)
                && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID))
            {
                if (longestMatchingEntry == NULL)
                {
                    longestMatchingEntry = &rowPtr[i];
                }
                else if (rowPtr[i].addrMask == longestMatchingEntry->addrMask)
                {
                    if (rowPtr[i].metric == longestMatchingEntry->metric)
                    {
                        if (rowPtr[i].areaId > longestMatchingEntry->areaId)
                        {
                            longestMatchingEntry = &rowPtr[i];
                        }
                    }
                    else if (rowPtr[i].metric > longestMatchingEntry->metric)
                    {
                        longestMatchingEntry = &rowPtr[i];
                    }
                }
                else if (rowPtr[i].addrMask > longestMatchingEntry->addrMask)
                {
                    longestMatchingEntry = &rowPtr[i];
                }
            }
        }
    }

    return longestMatchingEntry;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2FreeRoute()
// PURPOSE:  Free desired route from routing table
// RETURN: None
//-------------------------------------------------------------------------//

static
void Ospfv2FreeRoute(Node* node, Ospfv2RoutingTableRow* rowPtr)
{

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2RoutingTable* rtTable = &ospf->routingTable;

#ifdef JNE_LIB
    if (!rowPtr)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "this variable rowPtr is NULL",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(rowPtr, "this variable rowPtr is NULL\n");

    BUFFER_ClearDataFromDataBuffer(&rtTable->buffer,
                                   (char*) rowPtr,
                                   sizeof(Ospfv2RoutingTableRow),
                                   FALSE);
    rtTable->numRows--;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2FreeInvalidRoute()
// PURPOSE:  Free invalid route from routing table
// RETURN: None
//-------------------------------------------------------------------------//

static
void Ospfv2FreeInvalidRoute(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTable* rtTable = &ospf->routingTable;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        if (rowPtr[i].flag == OSPFv2_ROUTE_INVALID)
        {
            Ospfv2FreeRoute(node, &rowPtr[i]);
            i--;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2RoutesMatchSame()
// PURPOSE:  Check if two routes are identical
// RETURN: TRUE if routes are same, FALSE otherwise.
//-------------------------------------------------------------------------//

BOOL Ospfv2RoutesMatchSame(
    Ospfv2RoutingTableRow* newRoute,
    Ospfv2RoutingTableRow* oldRoute)
{
    //TBD: Need to check all next hop in case of equal cost multipath
    if ((newRoute->metric == oldRoute->metric)
        && (newRoute->nextHop == oldRoute->nextHop))
    {
        return TRUE;
    }

    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2AddRoute()
// PURPOSE:  Add route to routing table
// RETURN: None
//-------------------------------------------------------------------------//

void Ospfv2AddRoute(
    Node* node,
    Ospfv2RoutingTableRow* newRoute)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2RoutingTable* rtTable = &ospf->routingTable;

    Ospfv2RoutingTableRow* rowPtr;

    // Get old route if any ..
    if (newRoute->destType != OSPFv2_DESTINATION_NETWORK)
    {
        rowPtr = Ospfv2GetIntraAreaRoute(node,
                                         newRoute->destAddr,
                                         newRoute->destType,
                                         newRoute->areaId);
    }
    else
    {
        rowPtr = Ospfv2GetRoute(node,
                                newRoute->destAddr,
                                newRoute->destType);
    }

    if (rowPtr)
    {
        if (Ospfv2RoutesMatchSame(newRoute, rowPtr))
        {
            newRoute->flag = OSPFv2_ROUTE_NO_CHANGE;
        }
        else
        {
            newRoute->flag = OSPFv2_ROUTE_CHANGED;
        }

        memcpy(rowPtr, newRoute, sizeof(Ospfv2RoutingTableRow));
    }
    else
    {
        newRoute->flag = OSPFv2_ROUTE_NEW;
        rtTable->numRows++;

        BUFFER_AddDataToDataBuffer(&rtTable->buffer,
                                   (char*) newRoute,
                                   sizeof(Ospfv2RoutingTableRow));
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2AddInterAreaRoute()
// PURPOSE:  Add Inter Area route to routing table
// RETURN: None
//-------------------------------------------------------------------------//

static
void Ospfv2AddInterAreaRoute(
    Node* node,
    Ospfv2RoutingTableRow* newRoute)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2RoutingTable* rtTable = &ospf->routingTable;

    // Get old route if any ..
    Ospfv2RoutingTableRow* rowPtr = Ospfv2GetRoute(node,
                                                   newRoute->destAddr,
                                                   newRoute->destType);

    if (rowPtr)
    {
        if (Ospfv2RoutesMatchSame(newRoute, rowPtr))
        {
            newRoute->flag = OSPFv2_ROUTE_NO_CHANGE;
        }
        else
        {
            newRoute->flag = OSPFv2_ROUTE_CHANGED;
        }

        memcpy(rowPtr, newRoute, sizeof(Ospfv2RoutingTableRow));
    }
    else
    {
        newRoute->flag = OSPFv2_ROUTE_NEW;
        rtTable->numRows++;

        BUFFER_AddDataToDataBuffer(&rtTable->buffer,
                                   (char*) newRoute,
                                   sizeof(Ospfv2RoutingTableRow));
    }
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2UpdateIntraAreaRoute()
// PURPOSE:  Update an Intra Area Route
// RETURN: None
//-------------------------------------------------------------------------//

static
void Ospfv2UpdateIntraAreaRoute(
    Node* node,
    Ospfv2Area* thisArea,
    Ospfv2Vertex* v)
{
    Ospfv2RoutingTableRow* rowPtr = NULL;
    Ospfv2RoutingTableRow newRow;

    //Ospfv2ListItem* listItem;
    Ospfv2NextHopListItem* nextHopItem = NULL;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    if (v->nextHopList->size > 0)
    {
        nextHopItem = (Ospfv2NextHopListItem*)
                          v->nextHopList->first->data;
    }

    // If vertex is router
    // TBD: Need to keep all route.
    if (v->vertexType == OSPFv2_VERTEX_ROUTER)
    {
        Ospfv2RouterLSA* routerLSA = (Ospfv2RouterLSA*) v->LSA;

        if ((Ospfv2RouterLSAGetASBRouter(routerLSA->ospfRouterLsa) == 1)
            || (Ospfv2RouterLSAGetABRouter(routerLSA->ospfRouterLsa) == 1))
        {
            // Add entry of vertex to routing table

            if (Ospfv2RouterLSAGetABRouter(routerLSA->ospfRouterLsa) == 1
                && Ospfv2RouterLSAGetASBRouter(routerLSA->ospfRouterLsa) == 1)
            {
                newRow.destType = OSPFv2_DESTINATION_ABR_ASBR;
            }
            else if
                (Ospfv2RouterLSAGetABRouter(routerLSA->ospfRouterLsa) == 1)
            {
                newRow.destType = OSPFv2_DESTINATION_ABR;
            }
            else
            {
                newRow.destType = OSPFv2_DESTINATION_ASBR;
            }

            newRow.destAddr = v->vertexId;
            newRow.addrMask = 0xFFFFFFFF;

            //TBD:
            newRow.option = routerLSA->LSHeader.options;

            newRow.areaId = thisArea->areaID;
            newRow.pathType = OSPFv2_INTRA_AREA;
            newRow.metric = v->distance;
            newRow.type2Metric = 0;
            newRow.LSOrigin = (void*) v->LSA;
            newRow.advertisingRouter = routerLSA->LSHeader.advertisingRouter;

            // TBD: consider one next hop for now
#ifdef ADDON_BOEINGFCS
            if (v->nextHopList->size > 0)
            {
#endif
            nextHopItem = (Ospfv2NextHopListItem*)
                                  v->nextHopList->first->data;
            newRow.nextHop = nextHopItem->nextHop;
            newRow.outIntf = nextHopItem->outIntf;

            Ospfv2AddRoute(node, &newRow);

#ifdef ADDON_BOEINGFCS
            }
#endif
        }
#ifdef ADDON_BOEINGFCS
        // only add route if there is a next hop to it.
        else if (v->nextHopList->size > 0)
        {
            newRow.destType = OSPFv2_DESTINATION_ABR;
            newRow.destAddr = v->vertexId;
            newRow.addrMask = 0xFFFFFFFF;

            //TBD:
            newRow.option = 0;

            newRow.areaId = thisArea->areaID;
            newRow.pathType = OSPFv2_INTRA_AREA;
            newRow.metric = v->distance;
            newRow.type2Metric = 0;
            newRow.LSOrigin = (void*) v->LSA;
            newRow.advertisingRouter = routerLSA->LSHeader.advertisingRouter;

            // TBD: consider one next hop for now
            nextHopItem = (Ospfv2NextHopListItem*)
                                  v->nextHopList->first->data;
            newRow.nextHop = nextHopItem->nextHop;
            newRow.outIntf = nextHopItem->outIntf;

            Ospfv2AddRoute(node, &newRow);
        }
#endif
    }

    // Else it must be a transit network vertex
    else if (nextHopItem != NULL &&
             ospf->iface[nextHopItem->outIntf].includeSubnetRts == TRUE)
    {
        NodeAddress netAddr;
        NodeAddress netMask;
        Ospfv2NetworkLSA* networkLSA = (Ospfv2NetworkLSA*) v->LSA;

        memcpy(&netMask, networkLSA + 1, sizeof(NodeAddress));
        netAddr = v->vertexId & netMask;

        rowPtr = Ospfv2GetValidRoute(node,
                                     netAddr,
                                     OSPFv2_DESTINATION_NETWORK);

        // If the routing table entry already exists, current routing table
        // entry should be overwritten if and only if the newly found path is
        // just as short and current routing table entry's Link State Origin
        // has a smaller Link State ID than the newly added vertex' LSA.
        if (rowPtr != NULL)
        {
            NodeAddress linkStateID;

            linkStateID = ((Ospfv2LinkStateHeader*) rowPtr->LSOrigin)
                                      ->linkStateID;

            if ((v->distance > (unsigned int) rowPtr->metric)
                || (networkLSA->LSHeader.linkStateID < linkStateID))
            {
                return;
            }

            Ospfv2FreeRoute(node, rowPtr);
        }

        // Add route to routing table
        newRow.destType = OSPFv2_DESTINATION_NETWORK;
        newRow.destAddr = netAddr;
        newRow.addrMask = netMask;
        newRow.option = 0;
        newRow.areaId = thisArea->areaID;
        newRow.pathType = OSPFv2_INTRA_AREA;
        newRow.metric = v->distance;
        newRow.type2Metric = 0;
        newRow.LSOrigin = (void*) v->LSA;
        newRow.advertisingRouter = networkLSA->LSHeader.advertisingRouter;

        // TBD: consider one next hop for now
        nextHopItem = (Ospfv2NextHopListItem*)
                              v->nextHopList->first->data;
        newRow.nextHop = nextHopItem->nextHop;
        newRow.outIntf = nextHopItem->outIntf;

        Ospfv2AddRoute(node, &newRow);
    }
}

//-------------------------------------------------------------------------//
// NAME     : Ospfv2GetInterfaceIndexFromlinkData
// PURPOSE  : to get interface index from linkData
// RETURN   : interface index.
//-------------------------------------------------------------------------//
int Ospfv2GetInterfaceIndexFromlinkData(Node* node, NodeAddress linkData)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    unsigned int i;

    for (i = 0; i < (UInt32) node->numberInterfaces; i++)
    {
        if (ip->interfaceInfo[i]->isVirtualInterface
            || ip->interfaceInfo[i]->isUnnumbered)
        {
            if ((UInt32)i == linkData)
            {
                return i;
            }
        }
        else  if (ip->interfaceInfo[i]->ipAddress == linkData)
        {
            return i;
        }
    }

    return -1;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2UpdateIpForwardingTable
// PURPOSE: Update IP's forwarding table so that IP knows how to route
//          packets.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2UpdateIpForwardingTable(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    int i;
    Ospfv2RoutingTableRow* rowPtr =
        (Ospfv2RoutingTableRow*) BUFFER_GetData(&ospf->routingTable.buffer);

    if (OSPFv2_DEBUG_ERRORS)
    {
        Ospfv2PrintRoutingTable(node);
    }

    NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OSPFv2);
    NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OSPFv2_TYPE1_EXTERNAL);
    NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OSPFv2_TYPE2_EXTERNAL);



    for (i = 0; i < ospf->routingTable.numRows; i++)
    {
        // Avoid updating IP forwarding table for route to self and router
        if ((!Ospfv2IsMyAddress(node, rowPtr[i].destAddr))
//#ifndef ADDON_BOEINGFCS
            && (rowPtr[i].destType == OSPFv2_DESTINATION_NETWORK)
//#endif
            && (rowPtr[i].flag != OSPFv2_ROUTE_INVALID))
        {
                if (rowPtr[i].pathType == OSPFv2_TYPE1_EXTERNAL)
                {
                    NetworkUpdateForwardingTable(
                        node,
                        rowPtr[i].destAddr,
                        rowPtr[i].addrMask,
                        rowPtr[i].nextHop,
                        rowPtr[i].outIntf,
                        rowPtr[i].metric,
                        ROUTING_PROTOCOL_OSPFv2_TYPE1_EXTERNAL);
                }
                else if (rowPtr[i].pathType == OSPFv2_TYPE2_EXTERNAL)
                {
                    NetworkUpdateForwardingTable(
                        node,
                        rowPtr[i].destAddr,
                        rowPtr[i].addrMask,
                        rowPtr[i].nextHop,
                        rowPtr[i].outIntf,
                        rowPtr[i].metric,
                        ROUTING_PROTOCOL_OSPFv2_TYPE2_EXTERNAL);
                }
                else
                {
                     NetworkUpdateForwardingTable(
                                node,
                                rowPtr[i].destAddr,
                                rowPtr[i].addrMask,
                                rowPtr[i].nextHop,
                                rowPtr[i].outIntf,
                                rowPtr[i].metric,
                                ROUTING_PROTOCOL_OSPFv2);
                }
        }
    }

#ifdef ADDON_STATS_MANAGER
    StatsManager_UpdateNode(node);
#endif
}


static
void Ospfv2AttemptAdjacency(
        Node* node,
        Ospfv2Neighbor* nbrInfo,
        int interfaceId)
{

    // If this is the first time that the adjacency has been attempted,
    // the DD Sequence number should be assigned to some unique value.
    if (nbrInfo->DDSequenceNumber == 0)
    {
        nbrInfo->DDSequenceNumber = (unsigned int) (node->getNodeTime()
                                        / SECOND);
    }
    else
    {
        nbrInfo->DDSequenceNumber++;
    }

#ifdef ADDON_BOEINGFCS
        nbrInfo->ddReTxCount = 0;
#endif

    nbrInfo->masterSlave = T_Master;

    if (Ospfv2DebugSync(node))
    {
        char clockStr[100];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        char neighborIPStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress,
                                    neighborIPStr);

        fprintf(stdout, "    attempt adjacency with neighbor %s at %s\n",
                        neighborIPStr, clockStr);

        fprintf(stdout, "        initial DD seqno = %u\n",
                        nbrInfo->DDSequenceNumber);
    }

    // Send Empty DD Packet
    Ospfv2SendDatabaseDescriptionPacket(node,
                                        nbrInfo->neighborIPAddress,
                                        interfaceId);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2SetNeighborState()
// PURPOSE      :Set the state of a neighbor.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2SetNeighborState(
    Node* node,
    int interfaceIndex,
    Ospfv2Neighbor* neighborInfo,
    Ospfv2NeighborState state
#ifdef ADDON_BOEINGFCS
    , BOOL reFormAdj
#endif
)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2NeighborState oldState;

#ifdef JNE_LIB
    if (interfaceIndex == -1)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Neighbor on unknown interface",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(interfaceIndex != -1, "Neighbor on unknown interface\n");
    Ospfv2Area* thisArea = Ospfv2GetArea(node, ospf->iface[interfaceIndex].areaId);

    oldState = neighborInfo->state;
    neighborInfo->state = state;

    // Attempt to form adjacency if new state is S_ExStart
    if ((oldState != state) && (state == S_ExStart))
    {
#ifdef ADDON_BOEINGFCS
        if (RoutingCesRospfActiveOnInterface(node, interfaceIndex))
        {
            if (reFormAdj ||
                (RoutingCesRospfAdjacencyRequired(node,
                                        interfaceIndex,
                                        neighborInfo->neighborIPAddress,
                                        neighborInfo->neighborNodeId)))
            {
                Ospfv2AttemptAdjacency(node, neighborInfo, interfaceIndex);
            }
        }
        else
#endif
        {
            Ospfv2AttemptAdjacency(node, neighborInfo, interfaceIndex);
        }
    }

    // Need to originate LSA when transitioning from/to S_Full state.
    // This is to inform other neighbors of new link.
    if ((oldState == S_Full && state != S_Full)
        || (oldState != S_Full && state == S_Full))
    {
        if (Ospfv2DebugSync(node))
        {
            char clockStr[100];

            TIME_PrintClockInSecond(node->getNodeTime() + getSimStartTime(node),
                    clockStr);
            char neighborIPStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(neighborInfo->neighborIPAddress,
                                        neighborIPStr);
            if (state == S_Full)
                printf("Node %u: neighbor %s goes to FULL state at"
                    " time %s\n", node->nodeId,
                    neighborIPStr, clockStr);
        }

#ifdef ADDON_BOEINGFCS
        if (ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE)
        {
            if (node->guiOption && Ospfv2RospfDisplayIcons(node))
            {
                NodeId nbrId = MAPPING_GetNodeIdFromInterfaceAddress(
                    node,
                    neighborInfo->neighborID);

                if (oldState != S_Full && state == S_Full)
                {
                    GUI_AddLink(node->nodeId,
                                nbrId,
                                GUI_NETWORK_LAYER,
                                0,
                                ANY_DEST,
                                255,
                                node->getNodeTime() + getSimStartTime(node));
                }

                else if (oldState == S_Full && state != S_Full)
                {
                    GUI_DeleteLink(node->nodeId,
                                   nbrId,
                                   GUI_NETWORK_LAYER,
                                   0,
                                   node->getNodeTime() + getSimStartTime(node));
                }
            }
        }
#endif
        Ospfv2ScheduleRouterLSA(node,
                                ospf->iface[interfaceIndex].areaId,
                                FALSE);

        if (ospf->iface[interfaceIndex].state == S_DR)
        {
            Ospfv2ScheduleNetworkLSA(node, interfaceIndex, FALSE);
        }
    }

    // It may be necessary to return to DR election algorithm
    if (((oldState < S_TwoWay) && (state >= S_TwoWay))
        || ((oldState >= S_TwoWay) && (state < S_TwoWay)))
    {
        Ospfv2ScheduleInterfaceEvent(node, interfaceIndex, E_NeighborChange);
    }

    /***** Start: OPAQUE-LSA *****/
    // Originate opaque LSA when in full state with a neighbor.
    if ((oldState != S_Full && state == S_Full) &&
        (ospf->opaqueCapable == TRUE) &&
        Ospfv2DatabaseDescriptionPacketGetO(neighborInfo->dbDescriptionNbrOptions) &&
        (thisArea->externalRoutingCapability == TRUE))
    {
        Ospfv2ScheduleOpaqueLSA(node,
                                OSPFv2_AS_SCOPE_OPAQUE);
    }

    /***** End: OPAQUE-LSA *****/
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2SetInterfaceState()
// PURPOSE      :Set the state of a interface.
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2SetInterfaceState(
    Node* node,
    int interfaceIndex,
    Ospfv2InterfaceState newState)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2InterfaceState oldState = ospf->iface[interfaceIndex].state;

    ospf->iface[interfaceIndex].state = newState;

    // We may need to produce a new instance of router LSA
    Ospfv2ScheduleRouterLSA(
        node, ospf->iface[interfaceIndex].areaId, FALSE);

    // If I'm new DR the produce network LSA for this network
    if (oldState != S_DR && newState == S_DR)
    {
        Ospfv2ScheduleNetworkLSA(node, interfaceIndex, FALSE);

        // M-OSPF Patch start
        if (ospf->isMospfEnable)
        {
            // Generate GroupMembershipLSA for group present in this network
            MospfScheduleGroupMembershipLSA(node,
                ospf->iface[interfaceIndex].areaId,
                    (unsigned) interfaceIndex, (unsigned) ANY_GROUP, FALSE);
        }
        // M-Ospf Patch end

#ifdef JNE_GUI
        /*
         * EVENT: promoted to Designated Router
         */
        JNEGUI_EnableDesignatedRouterState(node, interfaceIndex);
#endif /* JNE_GUI */

    }

    // Else if I'm no longer the DR, flush previously
    // originated network LSA
    else if (oldState == S_DR && newState != S_DR)
    {
        // Flush previously originated network LSA from routing domain
        Ospfv2ScheduleNetworkLSA(node, interfaceIndex, TRUE);

        // M-OSPF Patch start
        if (ospf->isMospfEnable)
        {
            // Flush previously originated GroupMembershipLSA for this area
            MospfScheduleGroupMembershipLSA(node,
                ospf->iface[interfaceIndex].areaId,
                    (unsigned) interfaceIndex, (unsigned) ANY_GROUP, TRUE);
        }
        // M-Ospf Patch end

#ifdef JNE_GUI
        /*
         * EVENT: resigned as Designated Router
         */
        JNEGUI_DisableDesignatedRouterState(node, interfaceIndex);
#endif /* JNE_GUI */

    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleInterfaceEvent()
// PURPOSE      :Handle interface event and change interface state
//               accordingly
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------//

static
void Ospfv2HandleInterfaceEvent(
    Node* node,
    int interfaceIndex,
    Ospfv2InterfaceEvent eventType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2InterfaceState newState = S_Down;
    // Copy old state information
    Ospfv2InterfaceState oldState = ospf->iface[interfaceIndex].state;

    switch (eventType)
    {
        case E_InterfaceUp:
        {
            if (ospf->iface[interfaceIndex].state != S_Down)
            {
                newState = oldState;
                break;
            }

            // Start Hello timer & enabling periodic sending of Hello packet
            if ((ospf->iface[interfaceIndex].type ==
                    OSPFv2_POINT_TO_POINT_INTERFACE)
               || (ospf->iface[interfaceIndex].type ==
                    OSPFv2_POINT_TO_MULTIPOINT_INTERFACE)
               || (ospf->iface[interfaceIndex].type ==
                    OSPFv2_VIRTUAL_LINK_INTERFACE))
            {
                newState = S_PointToPoint;
            }
            else if (ospf->iface[interfaceIndex].routerPriority == 0
#ifdef ADDON_BOEINGFCS
                     || ospf->iface[interfaceIndex].type ==
                     OSPFv2_ROSPF_INTERFACE
#endif
                    )
            {
                newState = S_DROther;
            }
            else
#ifdef ADDON_BOEINGFCS
            if (ospf->iface[interfaceIndex].type != OSPFv2_ROSPF_INTERFACE)
#endif
            {
                Message* waitTimerMsg = NULL;

                newState = S_Waiting;

                // Send wait timer to self
                waitTimerMsg = MESSAGE_Alloc(node,
                                             NETWORK_LAYER,
                                             ROUTING_PROTOCOL_OSPFv2,
                                             MSG_ROUTING_OspfWaitTimer);

#ifdef ADDON_NGCNMS
                MESSAGE_SetInstanceId(waitTimerMsg, interfaceIndex);
#endif

                MESSAGE_InfoAlloc(node, waitTimerMsg, sizeof(int));

                memcpy(MESSAGE_ReturnInfo(waitTimerMsg),
                       &interfaceIndex,
                       sizeof(int));

                ospf->iface[interfaceIndex].waitTimerMsg = waitTimerMsg;

                MESSAGE_Send(node, waitTimerMsg,
                    ospf->iface[interfaceIndex].routerDeadInterval);
            }

            break;
        }

        case E_WaitTimer:
        case E_BackupSeen:
        case E_NeighborChange:
        {
            if (((eventType == E_BackupSeen || eventType == E_WaitTimer)
                    && (ospf->iface[interfaceIndex].state != S_Waiting))
                || ((eventType == E_NeighborChange)
                    && (ospf->iface[interfaceIndex].state != S_DROther
                        && ospf->iface[interfaceIndex].state != S_Backup
                        && ospf->iface[interfaceIndex].state != S_DR))
                 )
            {
                newState = oldState;
            }
#ifdef ADDON_BOEINGFCS
            else if (ospf->iface[interfaceIndex].type ==
                     OSPFv2_ROSPF_INTERFACE)
            {
                if (eventType == E_WaitTimer &&
                    ospf->iface[interfaceIndex].state == S_Waiting)
                {
                    RoutingCesRospfHandleDRTimer(node, interfaceIndex, true);
                }
                RoutingCesRospfAdjDetermination(node, interfaceIndex);
                if (eventType == E_NeighborChange)
                {
                    newState = oldState;
                }
                else
                {
                    newState = S_DROther;
                }
            }
#endif
            else
            {
                newState = Ospfv2DRElection(node, interfaceIndex);
            }
            break;
        }

        case E_LoopInd:
        case E_UnloopInd:
        {
            // These 2 events are not used in simulation
            break;
        }

        case E_InterfaceDown:
        {
            // 1) Reset all interface variables
            // 2) Disable timers associated with this interface
            // 3) Generate S_KillNbr event to destry all neighbor
            //      associated with this interface
            Ospfv2ListItem* listItem =
                ospf->iface[interfaceIndex].neighborList->first;

            while (listItem)
            {
                Ospfv2Neighbor* nbrInfo =
                        (Ospfv2Neighbor*) listItem->data;

                listItem = listItem->next;

                //FIXME: Schedule the event or execute from here?
                Ospfv2HandleNeighborEvent(node,
                                          interfaceIndex,
                                          nbrInfo->neighborIPAddress,
                                          E_KillNbr);
            }

            if (ospf->iface[interfaceIndex].waitTimerMsg)
            {
                MESSAGE_CancelSelfMsg(node,
                                ospf->iface[interfaceIndex].waitTimerMsg);

                ospf->iface[interfaceIndex].waitTimerMsg = NULL;
            }

            ospf->iface[interfaceIndex].designatedRouter = 0;
            ospf->iface[interfaceIndex].designatedRouterIPAddress = 0;

            ospf->iface[interfaceIndex].backupDesignatedRouter = 0;
            ospf->iface[interfaceIndex].backupDesignatedRouterIPAddress
                = 0;

            //ospf->iface[interfaceIndex].networkLSTimer = FALSE;
            //ospf->iface[interfaceIndex].networkLSAOriginateTime = 0;
            ospf->iface[interfaceIndex].floodTimerSet = FALSE;
            ospf->iface[interfaceIndex].delayedAckTimer = FALSE;
            Ospfv2FreeList(
                node, ospf->iface[interfaceIndex].updateLSAList, FALSE);
            Ospfv2FreeList(
                node, ospf->iface[interfaceIndex].delayedAckList, FALSE);

            Ospfv2InitList(&ospf->iface[interfaceIndex].updateLSAList);
            Ospfv2InitList(&ospf->iface[interfaceIndex].delayedAckList);

            newState = S_Down;
            break;
        }

        default:
        {
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Unknown interface event!",
                JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, "Unknown interface event!\n");
        }
    }

    if (oldState != newState)
    {
        Ospfv2SetInterfaceState(node, interfaceIndex, newState);
    }
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleRetransmitTimer
// PURPOSE      :Retransmission timer expired, so may need
//               to retransmit packet.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2HandleRetransmitTimer(
    Node* node,
    int interfaceIndex,
    unsigned int sequenceNumber,
    NodeAddress neighborAddr,
    Ospfv2PacketType type)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Neighbor* nbrInfo =
        Ospfv2GetNeighborInfoByIPAddress(node, interfaceIndex, neighborAddr);
    Message* msg = NULL;
    BOOL foundPacket = FALSE;
    clocktype delay;

#ifdef JNE_LIB
    if (!nbrInfo)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Neighbor not found in the Neighbor list",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(nbrInfo, "Neighbor not found in the Neighbor list");
#ifdef JNE_LIB
    if (interfaceIndex == -1)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Not found any interface on which this neighbor is attached",
            JNE::CRITICAL);
    }
#endif

    ERROR_Assert(interfaceIndex != -1,
        "Not found any interface on which this neighbor is attached");

    switch (type)
    {
        case OSPFv2_LINK_STATE_UPDATE:
        {
            // Timer expired.
            if (sequenceNumber != nbrInfo->rxmtTimerSequenceNumber)
            {
                if (Ospfv2DebugFlood(node))
                {
                    printf("    Old timer\n");
                }

                // Timer is an old one, so just ignore.
                break;
            }

            Ospfv2RetransmitLSA(node, nbrInfo, interfaceIndex);

            break;
        }

        case OSPFv2_DATABASE_DESCRIPTION:
        {
            Ospfv2DatabaseDescriptionPacket* dbDscrpPkt = NULL;

            msg = nbrInfo->lastSentDDPkt;

            // Old Timer
            if (nbrInfo->masterSlave != T_Master
#ifdef ADDON_BOEINGFCS
                && ospf->iface[interfaceIndex].type != OSPFv2_ROSPF_INTERFACE
#endif
                )
            {
                break;
            }
            if (msg == NULL)
            {
                // No packet to retransmit
                break;
            }

#ifdef ADDON_BOEINGFCS
        if (ospf->maxNumRetx > 0)
        {
            if (nbrInfo->ddReTxCount >= ospf->maxNumRetx)
            {
                MESSAGE_Free(node, nbrInfo->lastSentDDPkt);
                nbrInfo->lastSentDDPkt = NULL;
                nbrInfo->ddReTxCount = 0;

                if (Ospfv2DebugSync(node))
                {
                    char neighborIPStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress,
                                                neighborIPStr);
                    printf("Node %d quit sending DD retransmission to %s."
                           " Too many retransmission without response\n",
                           node->nodeId, neighborIPStr);
                }


                // reset neighbor state to basically start from
                // the beginning.
                /*Ospfv2SetNeighborState(node,
                                       interfaceIndex,
                                       nbrInfo,
                                       S_Init);*/

                break;
            }
        }
#endif

            dbDscrpPkt = (Ospfv2DatabaseDescriptionPacket*)
                            MESSAGE_ReturnPacket(msg);

            // Send only database description packets that the
            // rmxtTimer indicates.
            if ((dbDscrpPkt->ddSequenceNumber == nbrInfo->DDSequenceNumber)
                && (dbDscrpPkt->ddSequenceNumber == sequenceNumber))
            {
                if (OSPFv2_DEBUG || Ospfv2DebugSync(node))
                {
                    char clockStr[100];
                    TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);
                    char neighborIPStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress,
                                                neighborIPStr);

                    fprintf(stdout, "Node %u retransmitting DD packet "
                                "(seqno %u) to node %s at time %s\n",
                                 node->nodeId, sequenceNumber,
                                 neighborIPStr,
                                 clockStr);
                }

                foundPacket = TRUE;

#ifdef ADDON_BOEINGFCS
                nbrInfo->ddReTxCount++;
#endif

                // Packet is retransmitted at the end of function.
                ospf->stats.numDDPktRxmt++;
                ospf->stats.numDDPktBytesRxmt +=
                                            MESSAGE_ReturnPacketSize(msg);

                // Update interface based stats
                Ospfv2Interface* thisIntf = NULL;
                thisIntf = &ospf->iface[interfaceIndex];
                if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
                {
                    ospf->iface[interfaceIndex].interfaceStats.numDDPktRxmt++;
                    ospf->iface[interfaceIndex].interfaceStats.numDDPktBytesRxmt +=
                        MESSAGE_ReturnPacketSize(msg);
                }

                // Set retransmission timer again if Master.
                if (nbrInfo->masterSlave == T_Master
#ifdef nADDON_BOEINGFCS
                    || (ospf->iface[interfaceIndex].type ==
                                                   OSPFv2_ROSPF_INTERFACE
                        && nbrInfo->state < S_Full)
#endif
                    )
                {
                    clocktype delay;

                    delay = (clocktype)
                            (ospf->iface[interfaceIndex].rxmtInterval
                            + (RANDOM_erand(ospf->seed)
                            * OSPFv2_BROADCAST_JITTER));

                    Ospfv2SetTimer(
                          node,
                          interfaceIndex,
                          MSG_ROUTING_OspfRxmtTimer,
                          nbrInfo->neighborIPAddress,
                          delay,
                          dbDscrpPkt->ddSequenceNumber,
                          0,
                          OSPFv2_DATABASE_DESCRIPTION);
                }
            }
            break;
        }

        case OSPFv2_LINK_STATE_REQUEST:
        {
            if (nbrInfo->LSReqTimerSequenceNumber != sequenceNumber)
            {
                // No packet to retransmit
                break;
            }

            if (Ospfv2ListIsEmpty(nbrInfo->linkStateRequestList))
            {
                // No packet to retransmit
                if (Ospfv2DebugSync(node))
                {
                    printf("Node %d dont retransmit LS Req because "
                        "LS Req List is EMPTY!!\n",
                           node->nodeId);
                }
                break;
            }

            if (Ospfv2DebugSync(node))
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);
                char neighborIPStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress,
                                            neighborIPStr);
                fprintf(stdout, "Node %u retransmitting LS Request packet "
                                "to node %s at time %s\n",
                                 node->nodeId,
                                 neighborIPStr,
                                 clockStr);
            }

            Ospfv2ListItem *listItem = nbrInfo->linkStateRequestList->first;
            Ospfv2LSReqItem *itemInfo;
            while (listItem)
            {
                itemInfo = (Ospfv2LSReqItem*)listItem->data;

                if (itemInfo->seqNumber == sequenceNumber)
                {
                    Ospfv2SendLSRequestPacket(node,
                                              neighborAddr,
                                              interfaceIndex,
                                              TRUE);

                    ospf->stats.numLSReqRxmt++;
                    // Update interface based stats
                    Ospfv2Interface* thisIntf = NULL;
                    thisIntf = &ospf->iface[interfaceIndex];
                    if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
                    {
                        ospf->iface[interfaceIndex].interfaceStats.numLSReqRxmt++;
                    }
                    break;
                }
                listItem = listItem->next;
            }

            break;
        }
        default:
        {
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Unknown packet type",
                JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, "Unknown packet type");
        }
    }

    if (foundPacket)
    {

        //Trace sending pkt
        ActionData acnData;
        acnData.actionType = SEND;
        acnData.actionComment = NO_COMMENT;
        TRACE_PrintTrace(node, msg,TRACE_NETWORK_LAYER,
                         PACKET_OUT, &acnData);

        // Found packet to retransmit, so done...
        delay = 0;


        // This packet needs to be retransmitted, so send it out again.
        NetworkIpSendRawMessageToMacLayerWithDelay(
                        node,
                        MESSAGE_Duplicate(node, msg),
                        NetworkIpGetInterfaceAddress(node, interfaceIndex),
                        nbrInfo->neighborIPAddress,
                        IPTOS_PREC_INTERNETCONTROL,
                        IPPROTO_OSPF,
                        1,
                        interfaceIndex,
                        nbrInfo->neighborIPAddress,
                        delay);


#if COLLECT_DETAILS
        // instrumented code
        if (type == OSPFv2_LINK_STATE_UPDATE)
        {
            numRetxLSUpdateBytes += MESSAGE_ReturnPacketSize(msg);
            if (node->getNodeTime() - lastRetxLSUpdatePrint >=
                                                    OSPFv2_PRINT_INTERVAL)
            {
                lastRetxLSUpdatePrint = node->getNodeTime();
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("%s Number of ReTx LS Update bytes sent = %ld\n",
                       clockStr, numRetxLSUpdateBytes);
            }
        }
        else if (type == OSPFv2_DATABASE_DESCRIPTION)
        {
            numRetxDDBytesSent += MESSAGE_ReturnPacketSize(msg);

            if (node->getNodeTime() - lastRetxDDPrint >= OSPFv2_PRINT_INTERVAL)
            {
                lastRetxDDPrint = node->getNodeTime();
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("%s Number of ReTx DD bytes sent = %ld\n",
                       clockStr, numRetxDDBytesSent);
            }
        }
        else if (type == OSPFv2_LINK_STATE_REQUEST)
        {
            numRetxLSReqBytes += MESSAGE_ReturnPacketSize(msg);
            if (node->getNodeTime() - lastRetxLSReqPrint >=
                                                    OSPFv2_PRINT_INTERVAL)
            {
                lastRetxLSReqPrint = node->getNodeTime();
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("%s Number of ReTx LS Request bytes sent = %ld\n",
                       clockStr, numRetxLSReqBytes);
            }
        }
        else
        {
            ERROR_ReportError("Problem with Instrumented code!");
        }
        // end instrumented code
#endif

    }
}


//-------------------------------------------------------------------------//
//                       SHORTEST PATH CALCULATION                         //
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// NAME: Ospfv2LSAHasMaxAge()
// PURPOSE: Check whether the given LSA has MaxAge
// RETURN: TRUE if age is MaxAge, FALSE otherwise
//-------------------------------------------------------------------------//
BOOL Ospfv2LSAHasMaxAge(Ospfv2Data* ospf, char* LSA)
{
    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;

    if (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) ==
                (OSPFv2_LSA_MAX_AGE / SECOND))
    {
        return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2RemoveLSAFromShortestPathList
// PURPOSE:
// RETURN:
//-------------------------------------------------------------------------//

static
BOOL Ospfv2RemoveLSAFromShortestPathList(
    Node* node,
    Ospfv2List* shortestPathList,
    char* LSA)
{
    Ospfv2ListItem* listItem  = shortestPathList->first;
    Ospfv2Vertex* listVertex = NULL;

    // Search through the shortest path list for the node.
    while (listItem)
    {
        listVertex = (Ospfv2Vertex*) listItem->data;
        if (LSA == listVertex->LSA)
        {
            if (listVertex->nextHopList)
            {
                Ospfv2FreeList(node, listVertex->nextHopList, FALSE);
            }
            Ospfv2RemoveFromList(node, shortestPathList, listItem, FALSE);
            return TRUE;
        }
        listItem = listItem->next;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2InShortestPathList
// PURPOSE: Determine if a node is already in the shortest path list.
// RETURN: TRUE if node is in the shortest path list, FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2InShortestPathList(
    Node* node,
    Ospfv2List* shortestPathList,
    Ospfv2VertexType vertexType,
    NodeAddress vertexId)
{
    Ospfv2ListItem* listItem  = shortestPathList->first;
    Ospfv2Vertex* listVertex = NULL;

#ifdef nADDON_BOEINGFCS
    // keep disabled for now (notice nADDON_BOEINGFCS)
    int vNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node, vertexId);
#endif

    // Search through the shortest path list for the node.
    while (listItem)
    {
        listVertex = (Ospfv2Vertex*) listItem->data;

        // Found it.
        if ((listVertex->vertexType == vertexType)
            && (listVertex->vertexId == vertexId))
        {
            if (Ospfv2DebugSPT(node) || OSPFv2_DEBUG_ERRORS)
            {
                printf("    already in shortest path list\n");
            }

            return TRUE;
        }

#ifdef nADDON_BOEINGFCS
    // keep disabled for now (notice nADDON_BOEINGFCS)
        if ((listVertex->vertexType == vertexType &&
             RoutingCesRospfHaveLink(node, vNodeId, listVertex->vertexId)))
    {
            return TRUE;
    }
#endif
        listItem = listItem->next;
    }

    if (Ospfv2DebugSPT(node))
    {
        char vertexStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(vertexId,vertexStr);
        printf("    Vertex = %s of type %d not in shortest path list\n",
                    vertexStr, vertexType);
    }

    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2GetLinkDataForThisVertex()
// PURPOSE: Get link data from the associated link for this vertex.
// RETURN: None
//-------------------------------------------------------------------------//

static
void Ospfv2GetLinkDataForThisVertex(
    Node* node,
    Ospfv2Vertex* vertex,
    Ospfv2Vertex* parent,
    NodeAddress* linkData)
{

    int i = 0;
    char assertStr[MAX_STRING_LENGTH];
    Ospfv2RouterLSA* rtrLSA = (Ospfv2RouterLSA*) parent->LSA;
    Ospfv2LinkInfo* linkList = (Ospfv2LinkInfo*) (rtrLSA + 1);

    for (i = 0; i < rtrLSA->numLinks; i++)
    {
        int numTos;

        if (linkList->linkID == vertex->vertexId
            && linkList->type != OSPFv2_STUB)
        {
            *linkData = linkList->linkData;

            return;
        }

        // Place the linkList pointer properly
        numTos = linkList->numTOS;
        linkList = (Ospfv2LinkInfo*)
            ((QospfPerLinkQoSMetricInfo*)(linkList + 1)
                  + numTos);
    }

    // Shouldn't get here
    char vertexAddrStr[MAX_ADDRESS_STRING_LENGTH];
    IO_ConvertIpAddressToString(vertex->vertexId, vertexAddrStr);
    sprintf(assertStr, "Not get link data from the associated link "
        "for this vertex %s\n", vertexAddrStr);
#ifdef JNE_LIB
    JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                      assertStr, JNE::CRITICAL);
#endif
    ERROR_Assert(FALSE, assertStr);
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2HaveLinkWithNetworkVertex()
// PURPOSE: Check whether the node has link with this network vertex
// RETURN: TRUE if node have link. FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2HaveLinkWithNetworkVertex(Node* node, Ospfv2Vertex* v)
{
#ifdef ADDON_BOEINGFCS
    Ospfv2NetworkLSA* vLSA = (Ospfv2NetworkLSA*)v->LSA;
    NodeAddress* attachedRouter = NULL;
    int numLink;
    int i;

    if (Ospfv2DebugSPT(node))
    {
        char vertexStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(v->vertexId,vertexStr);
        printf("    Do I have link with this Network? %s\n",
                    vertexStr);
    }

    vLSA = (Ospfv2NetworkLSA*) v->LSA;
    attachedRouter = ((NodeAddress*) (vLSA + 1)) + 1;

    numLink = (vLSA->LSHeader.length - sizeof(Ospfv2NetworkLSA) - 4)
                      / (sizeof(NodeAddress));

    for (i = 0; i < numLink; i++)
    {
        if (NetworkIpIsMyIP(node, attachedRouter[i]))
        {
            if (Ospfv2DebugSPT(node))
            {
                printf("    Link to NETWORK found\n");
            }
            return TRUE;
        }
    }

    if (Ospfv2DebugSPT(node))
    {
        printf("    Link to NETWORK NOT found\n");
    }

    return FALSE;

#else
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    int i;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        NodeAddress subnetMask = NetworkIpGetInterfaceSubnetMask(node, i);
        NodeAddress netAddr = NetworkIpGetInterfaceNetworkAddress(node, i);

        if ((v->vertexId & subnetMask) == netAddr
            && ospf->iface[i].state != S_Down)
        {
            return TRUE;
        }
    }
    return FALSE;
#endif
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2CheckRouterLSASetThisNetworkTransit()
// PURPOSE: Check whether routerLSA treated this netAddr as Transit Network.
// RETURN: TRUE if netAddr as 'Transit Network'. FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL Ospfv2CheckRouterLSASetThisNetworkTransit(
    Ospfv2RouterLSA* routerLSA,
    NodeAddress netAddr,
    NodeAddress subnetMask)
{
    int j;
    Ospfv2LinkInfo* linkList = (Ospfv2LinkInfo*) (routerLSA + 1);

    for (j = 0; j < routerLSA->numLinks; j++)
    {
        int numTos;

        if ((linkList->linkID & subnetMask) == netAddr
            && linkList->type == OSPFv2_TRANSIT)
        {
            return TRUE;
        }

        // Place the linkList pointer properly
        numTos = linkList->numTOS;
        linkList = (Ospfv2LinkInfo*)
            ((QospfPerLinkQoSMetricInfo*)(linkList + 1) + numTos);
    }
    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2IsTransitWithNetworkVertex()
// PURPOSE: Check whether the Network of this vertexId is 'Transit Network'
//          perspective to the area of this node. This means this router
//          should view the network as a 'Transit Network'.
// RETURN: TRUE if node have a Transit link. FALSE otherwise.
//-------------------------------------------------------------------------//

BOOL Ospfv2IsTransitWithNetworkVertex(Node* node,
                                         Ospfv2Area* thisArea,
                                         NodeAddress vertexId)
{
    int i;
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2RouterLSA* ownRouterLSA = NULL;

    ownRouterLSA = (Ospfv2RouterLSA*) Ospfv2LookupLSAList(
                    thisArea->routerLSAList, ospf->routerID, ospf->routerID);

    if (ownRouterLSA == NULL)
    {
        return FALSE;
    }

    for (i = 0; i < node->numberInterfaces; i++)
    {
        NodeAddress subnetMask = NetworkIpGetInterfaceSubnetMask(node, i);
        NodeAddress netAddr = NetworkIpGetInterfaceNetworkAddress(node, i);

        // vertexId comes from network vertex...
        // check to see if we are claiming this network
        // as our transit network

        if ((vertexId & subnetMask) != netAddr ||
            ospf->iface[i].state == S_Down)
        {
            // he is not in our subnet or our interface to him is down
            continue;
        }

        if (Ospfv2DebugSPT(node))
        {
            char vertexStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(vertexId, vertexStr);
            printf("Checking for transit network with vertex %s\n",
                   vertexStr);
        }

        if (Ospfv2CheckRouterLSASetThisNetworkTransit(
                ownRouterLSA, vertexId, subnetMask))
        {
            return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2IsPresentInNextHopList()
// PURPOSE: Find if nextHopListItem already present in nextHopList
// RETURN: TRUE on success, FALSE otherwise
//-------------------------------------------------------------------------//

BOOL Ospfv2IsPresentInNextHopList(
    Ospfv2List* nextHopList,
    Ospfv2NextHopListItem* nextHopListItem)
{
    Ospfv2ListItem* item = NULL;

    item = nextHopList->first;

    while (item)
    {
        Ospfv2NextHopListItem* tmpItem = (Ospfv2NextHopListItem*) item->data;
        if (tmpItem->nextHop == nextHopListItem->nextHop)
        {
            return TRUE;
        }
        item = item->next;
    }
    return FALSE;
}


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2IsPresentInMaxAgeLSAList()
// LAYER        :: NETWORK
// PURPOSE      :: Check whether maxAgeItem is present in maxAgeList
// ASSUMPTION   :: None.
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// + maxAgeList : Ospfv2List* : list containing all the MaxAge LSAs
// + MaxAgeLSAItem : Ospfv2LinkStateHeader* : item to be search in maxAgeList
// RETURN VALUE :: TRUE if present, FALSE otherwise
//-------------------------------------------------------------------------//
BOOL Ospfv2IsPresentInMaxAgeLSAList(
    Node* node,
    Ospfv2List* maxAgeList,
    Ospfv2LinkStateHeader* MaxAgeLSAItem)
{
    Ospfv2ListItem* item = NULL;

    if (maxAgeList == NULL)
    {
        return FALSE;
    }

    item = maxAgeList->first;

    while (item)
    {
        Ospfv2LinkStateHeader* tmpItem = (Ospfv2LinkStateHeader*) item->data;
        if (Ospfv2CompareLSAToListItem(node, MaxAgeLSAItem, tmpItem))
        {
            return TRUE;
        }
        item = item->next;
    }
    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2SetNextHopForThisVertex()
// PURPOSE: Set next hop for the vertex
// RETURN: TRUE on success, FALSE otherwise
//-------------------------------------------------------------------------//

static
BOOL Ospfv2SetNextHopForThisVertex(
    Node* node,
    Ospfv2Vertex* vertex,
    Ospfv2Vertex* parent)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2NextHopListItem* nextHopItem = NULL;

    // If parent is root, vertex is either directly connected router
    // or network
    if ((parent->vertexType == OSPFv2_VERTEX_ROUTER)
        && (parent->vertexId == ospf->routerID))
    {
        if (Ospfv2DebugSPT(node))
        {
            printf("Parent is ROOT\n");
        }

        NodeAddress linkData = 0;
        int interfaceId;

        nextHopItem = (Ospfv2NextHopListItem*)
                       MEM_malloc(sizeof(Ospfv2NextHopListItem));

            Ospfv2GetLinkDataForThisVertex(node, vertex, parent, &linkData);

        interfaceId = Ospfv2GetInterfaceIndexFromlinkData(node, linkData);
        nextHopItem->outIntf = interfaceId;

        if (ospf->iface[interfaceId].state == S_Down)
        {
            MEM_free(nextHopItem);
            return FALSE;
        }

        // Ideally this field is not required for an directly connected
        // network or router which is connected via a point-to-point link.
        // We doesn't considering the case of router connecting through a
        // poin-to-multipoint network. Still adding this field for wireless
        // network.
        if (vertex->vertexType == OSPFv2_VERTEX_ROUTER)
        {
            int interfaceId;
            Ospfv2Neighbor* nbrInfo = NULL;
            NodeAddress nbrIPAddress = (NodeAddress)NETWORK_UNREACHABLE;

            interfaceId = Ospfv2GetInterfaceIndexFromlinkData(
                                node,
                                linkData);
            nbrInfo = Ospfv2GetNbrInfoOnInterface(
                                node,
                                vertex->vertexId,
                                interfaceId);

#ifdef ADDON_BOEINGFCS
            if (nbrInfo == NULL &&
                ospf->iface[interfaceId].type == OSPFv2_ROSPF_INTERFACE)
            {
                nbrIPAddress = RoutingCesRospfGetNbrInfoOnInterface(
                                       node,
                                       vertex->vertexId,
                                       interfaceId);
            }
#endif

            if (nbrInfo == NULL &&
                nbrIPAddress == (NodeAddress)NETWORK_UNREACHABLE)
            {
                // Neighbor probably removed
                MEM_free(nextHopItem);
                return FALSE;
            }
            else if (nbrInfo != NULL)
            {
                nbrIPAddress = nbrInfo->neighborIPAddress;
            }

            nextHopItem->nextHop = nbrIPAddress;
        }
        else
        {
            // Vertex is directly connected network
            nextHopItem->nextHop = 0;
        }

        if (!Ospfv2IsPresentInNextHopList(vertex->nextHopList, nextHopItem))
        {
            Ospfv2InsertToList(vertex->nextHopList, 0, (void*) nextHopItem);
        }
        else
        {
            MEM_free(nextHopItem);
            return FALSE;
        }
    }
    // Else if parent is network that directly connects with root
    else if ((parent->vertexType == OSPFv2_VERTEX_NETWORK) &&
        (Ospfv2HaveLinkWithNetworkVertex(node, parent)))
    {

        if (Ospfv2DebugSPT(node))
        {
            printf("parent is NETWORK vertex "
                   "and directly connects with root\n");
        }

        NodeAddress linkData = 0;
        int interfaceId;

        nextHopItem = (Ospfv2NextHopListItem*)
                       MEM_malloc(sizeof(Ospfv2NextHopListItem));
            Ospfv2GetLinkDataForThisVertex(node, parent, vertex, &linkData);

        nextHopItem->nextHop = linkData;
        interfaceId = Ospfv2GetInterfaceForThisNeighbor(node,
                                                        linkData);

#ifdef ADDON_BOEINGFCS
        if (interfaceId == -1)
        {
            interfaceId = RoutingCesRospfGetInterfaceForThisNeighbor(node,
                                                        linkData);

            if (interfaceId == -1)
            {
                if (Ospfv2DebugSPT(node))
                {
                    char linkStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(linkData, linkStr);
                    printf("InterfaceIndex for linkData %s not found!!\n",
                           linkStr);
                }
                MEM_free(nextHopItem);
                return FALSE;
            }
        }
#endif

        if (interfaceId == -1)
        {
            MEM_free(nextHopItem);
            return FALSE;
        }

        if (ospf->iface[interfaceId].state == S_Down)
        {
            MEM_free(nextHopItem);
            return FALSE;
        }

        nextHopItem->outIntf = interfaceId;

        if (!Ospfv2IsPresentInNextHopList(vertex->nextHopList,nextHopItem))
        {
            Ospfv2InsertToList(vertex->nextHopList, 0, (void*) nextHopItem);
        }
        else
        {
            MEM_free(nextHopItem);
            return FALSE;
        }
    }
    else
    {
        // There should be an intervening router. So inherits the set
        // of next hops from parent
        Ospfv2ListItem* listItem = parent->nextHopList->first;
        BOOL inserted = FALSE;

        if (Ospfv2DebugSPT(node))
        {
            printf("This is an intervening router\n");
        }

#ifdef ADDON_BOEINGFCS
        NodeAddress linkData = 0;
        int interfaceId;

        if (vertex->vertexType == OSPFv2_VERTEX_ROUTER)
        {
            Ospfv2GetLinkDataForThisVertex(node, parent, vertex, &linkData);
        }
        else
        {
            Ospfv2GetLinkDataForThisVertex(node, vertex, parent, &linkData);
        }

        interfaceId = Ospfv2GetInterfaceForThisNeighbor(node, linkData);
        //ERROR_Assert(interfaceId >= 0, "Problem getting interfaceId from linkData");

        // we know that interfaceId is in one of our connected subnets...
        // so just check if the subnet for which this next hop is in WNW or not.
        // if it is, then we apply the ROSPF rule. If not, we apply the standard
        // calculation.
        if (interfaceId >= 0 &&
            RoutingCesRospfActiveOnInterface(node, interfaceId))
        {
            nextHopItem = (Ospfv2NextHopListItem*)
                           MEM_malloc(sizeof(Ospfv2NextHopListItem));

            nextHopItem->nextHop = linkData;
            nextHopItem->outIntf = interfaceId;

            if (!Ospfv2IsPresentInNextHopList(vertex->nextHopList,
                nextHopItem))
            {
                Ospfv2InsertToList(vertex->nextHopList,
                                   0,
                                   (void*) nextHopItem);

                inserted = TRUE;
            }
            else
            {
                MEM_free(nextHopItem);
            }
        }
        else
        {
            while (listItem)
            {
                nextHopItem = (Ospfv2NextHopListItem*)
                               MEM_malloc(sizeof(Ospfv2NextHopListItem));

                memcpy(nextHopItem,
                       listItem->data,
                       sizeof(Ospfv2NextHopListItem));

                if (!Ospfv2IsPresentInNextHopList(vertex->nextHopList,
                    nextHopItem))
                {
                    Ospfv2InsertToList(
                        vertex->nextHopList,
                        0,
                        (void*) nextHopItem);

                    inserted = TRUE;
                }
                else
                {
                    MEM_free(nextHopItem);
                }
                listItem = listItem->next;
            }
        }
#else
        while (listItem)
        {
            nextHopItem = (Ospfv2NextHopListItem*)
                           MEM_malloc(sizeof(Ospfv2NextHopListItem));

            memcpy(nextHopItem,
                   listItem->data,
                   sizeof(Ospfv2NextHopListItem));

            if (!Ospfv2IsPresentInNextHopList(vertex->nextHopList,
                nextHopItem))
            {
                Ospfv2InsertToList(
                    vertex->nextHopList,
                    0,
                    (void*) nextHopItem);

                inserted = TRUE;
            }
            else
            {
                MEM_free(nextHopItem);
            }
            listItem = listItem->next;
        }
#endif

        return inserted;
    }
    return TRUE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2LSAHasLink()
// PURPOSE: Check whether the given LSA's have link
// RETURN: TRUE if a link is found, FALSE otherwise
//-------------------------------------------------------------------------//

static
BOOL Ospfv2LSAHasLink(Node* node, char*  wLSA, char*  vLSA)
{
#ifdef nADDON_BOEINGFCS
    int interfaceIndex = NetworkIpGetInterfaceIndexFromAddress(node,
        ospf->routerID);
    BOOL rospf = ospf->iface[interfaceIndex].type == OSPFv2_ROSPF_INTERFACE;

    if (NetworkCesSubnetIsTrueGateway(node, interfaceIndex))
    {
        rospf = RoutingCesRospfActiveOnAnyInterface(node);
    }
#endif
    Ospfv2LinkStateHeader* wLSHeader = (Ospfv2LinkStateHeader*) wLSA;
    Ospfv2LinkStateHeader* vLSHeader = (Ospfv2LinkStateHeader*) vLSA;

#ifdef nADDON_BOEINGFCS
    int vNodeId =
        MAPPING_GetNodeIdFromInterfaceAddress(node,
                                              vLSHeader->linkStateID);
#endif

    if (Ospfv2DebugSPT(node))
    {
        printf("    Checking for common link\n");
        printf("    Child LSA:\n");
        Ospfv2PrintLSA(wLSA);
        printf("    Parent LSA:\n");
        Ospfv2PrintLSA(vLSA);
    }

    if (wLSHeader->linkStateType == OSPFv2_ROUTER)
    {
        short int numLinks = ((Ospfv2RouterLSA*) wLSA)->numLinks;
        short int i;

        Ospfv2LinkInfo* link =
            (Ospfv2LinkInfo*) (wLSA + sizeof(Ospfv2RouterLSA));

        for (i = 0; i < numLinks; i++)
        {
            short int numTos;

            if (link->type == OSPFv2_POINT_TO_POINT)
            {
                if ((vLSHeader->linkStateType == OSPFv2_ROUTER)
                    && (link->linkID == vLSHeader->advertisingRouter)
#ifdef nADDON_BOEINGFCS
                    || (rospf &&
                        RoutingCesRospfHaveLink(node, vNodeId, link->linkID))
#endif
                    )
                {
                    return TRUE;
                }
            }
            else if (link->type == OSPFv2_TRANSIT)
            {
                if ((vLSHeader->linkStateType == OSPFv2_NETWORK)
                    && (link->linkID == vLSHeader->linkStateID)
#ifdef nADDON_BOEINGFCS
                    || (rospf &&
                        RoutingCesRospfHaveLink(node, vNodeId, link->linkID))
#endif
            )
                {
                    return TRUE;
                }
            }
            numTos = link->numTOS;
            link = (Ospfv2LinkInfo*)
                ((QospfPerLinkQoSMetricInfo*)(link + 1)
                      + numTos);
        }
    }
    else if (wLSHeader->linkStateType == OSPFv2_NETWORK)
    {
        short int numLink = (short) ((wLSHeader->length
                - sizeof(Ospfv2LinkStateHeader) - sizeof(NodeAddress))
                    / sizeof(NodeAddress));
        short int i;
        NodeAddress* attachedRouter = (NodeAddress*) (wLSA
                + sizeof(Ospfv2LinkStateHeader)
                + sizeof(NodeAddress));

        if (vLSHeader->linkStateType == OSPFv2_NETWORK)
        {
            return FALSE;
        }

        for (i = 0; i < numLink; i++)
        {
            if (attachedRouter[i] == vLSHeader->linkStateID
#ifdef nADDON_BOEINGFCS
                || (rospf &&
                    RoutingCesRospfHaveLink(node, vNodeId,
                                  attachedRouter[i]))
#endif
               )
            {
                return TRUE;
            }
        }
    }

    if (Ospfv2DebugSPT(node))
    {
        char vStr[MAX_ADDRESS_STRING_LENGTH];
        char wStr[MAX_ADDRESS_STRING_LENGTH];

        IO_ConvertIpAddressToString(vLSHeader->linkStateID,vStr);
        IO_ConvertIpAddressToString(wLSHeader->linkStateID,wStr);

        printf("    LSA %s and %s have no link\n",
                    vStr,wStr);
    }

    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2FindCandidate()
// PURPOSE: Find candidate in list
// RETURN: Pointer to candidate, NULL otherwise.
//-------------------------------------------------------------------------//

static
Ospfv2Vertex*  Ospfv2FindCandidate(
    Node* node,
    Ospfv2List* candidateList,
    Ospfv2VertexType vertexType,
    NodeAddress vertexId)
{
    Ospfv2ListItem* tempItem = candidateList->first;

#ifdef nADDON_BOEINGFCS
    // keep disabled for now (notice nADDON_BOEINGFCS)
    int vNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node, vertexId);
#endif

    if (Ospfv2DebugSPT(node))
    {
        char vertexStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(vertexId,vertexStr);

        printf("        Vertex ID = %s\n", vertexStr);
        printf("        Vertex Type = %d\n", vertexType);
    }

    // Search entire candidate list
    while (tempItem)
    {
        Ospfv2Vertex* tempEntry = (Ospfv2Vertex*) tempItem->data;

        if (Ospfv2DebugSPT(node))
        {
            char vertexStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(tempEntry->vertexId,vertexStr);
            printf("        tempEntry->vertexId = %s\n",
                    vertexStr);
            printf("        tempEntry->vertexType = %d\n",
                    tempEntry->vertexType);
            printf("        tempEntry->distance = %d\n",
                    tempEntry->distance);
        }

        // Candidate found.
        if ((tempEntry->vertexType == vertexType)
            && (tempEntry->vertexId == vertexId))
        {
            return tempEntry;
        }
#ifdef nADDON_BOEINGFCS
    // keep disabled for now (notice nADDON_BOEINGFCS)
        if (tempEntry->vertexType == vertexType &&
            RoutingCesRospfHaveLink(node, vNodeId, tempEntry->vertexId))
    {
            return tempEntry;
    }
#endif

        tempItem = tempItem->next;
    }

    if (Ospfv2DebugSPT(node))
    {
        printf("Candidate NOT Found\n");
    }
    return NULL;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2UpdateCandidateListUsingNetworkLSA
// PURPOSE: Update the candidate list using only the network LSAs.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2UpdateCandidateListUsingNetworkLSA(
    Node* node,
    Ospfv2Area* thisArea,
    Ospfv2List* candidateList,
    Ospfv2Vertex* v)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2NetworkLSA* vLSA = NULL;
    NodeAddress* attachedRouter = NULL;
    int numLink;
    char* wLSA = NULL;
    int i;

    if (Ospfv2DebugSPT(node))
    {
        char vertexStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(v->vertexId,vertexStr);
        printf("    Looking for vertex from network LSA of vertex %s\n",
                    vertexStr);
    }

    vLSA = (Ospfv2NetworkLSA*) v->LSA;
    attachedRouter = ((NodeAddress*) (vLSA + 1)) + 1;

    numLink = (vLSA->LSHeader.length - sizeof(Ospfv2NetworkLSA) - 4)
                      / (sizeof(NodeAddress));

    for (i = 0; i < numLink; i++)
    {
        Ospfv2Vertex* candidateListItem = NULL;
        NodeAddress newVertexId;
        Ospfv2VertexType newVertexType;
        unsigned int newVertexDistance;
        Ospfv2LinkStateHeader* wLSHeader = NULL;

        if (Ospfv2DebugSPT(node))
        {
            char attachedRouterStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(attachedRouter[i],attachedRouterStr);
            printf("        examining router %s\n", attachedRouterStr);
        }

        wLSA = (char*) Ospfv2LookupLSAListByID(thisArea->routerLSAList,
                                               attachedRouter[i]);

        // RFC2328, Sec-16.1 (2.b & 2.c)
        if ((wLSA == NULL) || (Ospfv2LSAHasMaxAge(ospf, wLSA))
            || (!Ospfv2LSAHasLink(node, wLSA, v->LSA))
            || (Ospfv2InShortestPathList(node,
                                         thisArea->shortestPathList,
                                         OSPFv2_VERTEX_ROUTER,
                                         attachedRouter[i])))
        {
            if (Ospfv2DebugSPT(node))
            {
                if (wLSA == NULL)
                {
                    printf("wLSA is NULL\n");
                }
                else if (Ospfv2LSAHasMaxAge(ospf, wLSA))
                {
                    printf("LSA has MAX AGE\n");
                }
                else if (!Ospfv2LSAHasLink(node, wLSA, v->LSA))
                {
                    printf("No Link between wLSA and v->LSA\n");
                }
            }
            continue;
        }

        // If it's directly connected network.
        if (Ospfv2IsTransitWithNetworkVertex(node, thisArea, v->vertexId))
        {
            // Consider only routers that are directly reachable
            NodeAddress linkData = 0;
            int interfaceId;
            Ospfv2Neighbor* nbrInfo = NULL;
            Ospfv2Vertex* root = (Ospfv2Vertex*)
                thisArea->shortestPathList->first->data;

               Ospfv2GetLinkDataForThisVertex(node, v, root, &linkData);

            interfaceId = Ospfv2GetInterfaceIndexFromlinkData(
                                node, linkData);

            nbrInfo = Ospfv2GetNbrInfoOnInterface(
                            node, attachedRouter[i], interfaceId);

            if (!nbrInfo)
            {
#ifdef ADDON_BOEINGFCS

                Ospfv2Data* ospf = (Ospfv2Data*)
                            NetworkIpGetRoutingProtocol(node,
                            ROUTING_PROTOCOL_OSPFv2);

                NodeAddress nbrIPAddress = (NodeAddress)NETWORK_UNREACHABLE;

                if (ospf->iface[interfaceId].type == OSPFv2_ROSPF_INTERFACE)
                {
                    nbrIPAddress = RoutingCesRospfGetNbrInfoOnInterface(
                                           node,
                                           attachedRouter[i],
                                           interfaceId);
                }

                if (nbrIPAddress == (NodeAddress)NETWORK_UNREACHABLE)
                {
                    continue;
                }
#else
                continue;
#endif
            }
        }

        wLSHeader = (Ospfv2LinkStateHeader*) wLSA;

        newVertexType = OSPFv2_VERTEX_ROUTER;
        newVertexId = wLSHeader->linkStateID;
        newVertexDistance = v->distance;

        candidateListItem = Ospfv2FindCandidate(node,
                                                candidateList,
                                                newVertexType,
                                                newVertexId);

        if (candidateListItem == NULL)
        {
            // Insert new candidate
            candidateListItem = (Ospfv2Vertex*)
                     MEM_malloc(sizeof(Ospfv2Vertex));

            candidateListItem->vertexId = newVertexId;
            candidateListItem->vertexType = newVertexType;
            candidateListItem->LSA = wLSA;
            candidateListItem->distance = newVertexDistance;

            Ospfv2InitList(&candidateListItem->nextHopList);

            if (Ospfv2SetNextHopForThisVertex(node, candidateListItem, v))
            {
                if (Ospfv2DebugSPT(node))
                {
                    char newVertexStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(newVertexId,newVertexStr);
                    printf("    Inserting new vertex %s\n", newVertexStr);
                }

                Ospfv2InsertToList(candidateList,
                                   0,
                                   (void*) candidateListItem);
            }
            else
            {
                Ospfv2FreeList(node,
                               candidateListItem->nextHopList,
                               FALSE);
                MEM_free(candidateListItem);
            }
        }
        else if (candidateListItem->distance > newVertexDistance)
        {
            // update
            candidateListItem->distance = newVertexDistance;

            if (Ospfv2DebugSPT(node))
            {
                char newVertexStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(newVertexId,newVertexStr);
                printf("    Update distance and next hop for vertex %s\n",
                            newVertexStr);
            }

            Ospfv2DeleteList(node,
                             candidateListItem->nextHopList,
                             FALSE);
            Ospfv2SetNextHopForThisVertex(node, candidateListItem, v);
        }
        else if (candidateListItem->distance == newVertexDistance)
        {
            if (Ospfv2DebugSPT(node))
            {
                char newVertexStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(newVertexId,newVertexStr);
                printf("    Add new set of next hop for vertex %s\n",
                            newVertexStr);
            }

            // Add new set of next hop values
            Ospfv2SetNextHopForThisVertex(node, candidateListItem, v);
        }
        else
        {
            // Examine next link
            continue;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2UpdateCandidateListUsingRouterLSA
// PURPOSE: Update the candidate list using only the router LSAs.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2UpdateCandidateListUsingRouterLSA(
    Node* node,
    Ospfv2Area* thisArea,
    Ospfv2List* candidateList,
    Ospfv2Vertex* v)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
           NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2RouterLSA* vLSA = (Ospfv2RouterLSA*) v->LSA;
    char* wLSA = NULL;
    Ospfv2LinkInfo* linkList = NULL;
    Ospfv2LinkInfo* nextLink = NULL;
    int i;


    if (Ospfv2DebugSPT(node))
    {
        char vertexStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(v->vertexId,vertexStr);
        printf("    Looking for vertex from router LSA = %s\n",
            vertexStr);
    }

    nextLink = (Ospfv2LinkInfo*) (vLSA + 1);

    // Examine vertex v's links.
    for (i = 0; i < vLSA->numLinks; i++)
    {
        int numTos;
        Ospfv2Vertex* candidateListItem = NULL;
        NodeAddress newVertexId;
        Ospfv2VertexType newVertexType;
        unsigned int newVertexDistance;
        Ospfv2LinkStateHeader* wLSHeader = NULL;

        linkList = nextLink;

        // Place the linkList pointer properly
        numTos = linkList->numTOS;
        nextLink = (Ospfv2LinkInfo*)
            ((QospfPerLinkQoSMetricInfo*)(nextLink + 1)
                  + numTos);

        if (Ospfv2DebugSPT(node))
        {
            char linkStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(linkList->linkID,linkStr);
            printf("        examining link %s,    linkType = %d\n",
                            linkStr, linkList->type);
        }

        if (linkList->type == OSPFv2_POINT_TO_POINT)
        {
            wLSA = (char*) Ospfv2LookupLSAList(thisArea->routerLSAList,
                                               linkList->linkID,
                                               linkList->linkID);

            newVertexType = OSPFv2_VERTEX_ROUTER;
        }
        else if (linkList->type == OSPFv2_TRANSIT)
        {
            wLSA = (char*) Ospfv2LookupLSAListByID(thisArea->networkLSAList,
                                                   linkList->linkID);

            newVertexType = OSPFv2_VERTEX_NETWORK;
        }
        else
        {
            // TBD: Virtual link is not considered yet
            // RFC2328, Sec-16.1 (2.a)
            continue;
        }

        // RFC2328, Sec-16.1 (2.b & 2.c)
        if ((wLSA == NULL) || (Ospfv2LSAHasMaxAge(ospf, wLSA))
            || (!Ospfv2LSAHasLink(node, wLSA, v->LSA))
            || (Ospfv2InShortestPathList(node,
                                         thisArea->shortestPathList,
                                         newVertexType,
                                         linkList->linkID)))
        {
            if (Ospfv2DebugSPT(node))
            {
                if (wLSA == NULL)
                {
                    printf("wLSA is NULL\n");
                }
                else if (Ospfv2LSAHasMaxAge(ospf, wLSA))
                {
                    printf("LSA has MAX AGE\n");
                }
                else if (!Ospfv2LSAHasLink(node, wLSA, v->LSA))
                {
                    printf("No Link between wLSA and v->LSA\n");
                }
            }
            continue;
        }

        wLSHeader = (Ospfv2LinkStateHeader*) wLSA;

        newVertexId = wLSHeader->linkStateID;
        // linkList->metric is a SIGNED short, while newVertexDistance and
        // v->distance are UNSIGNED.
        newVertexDistance = v->distance + (unsigned short)linkList->metric;

        candidateListItem = Ospfv2FindCandidate(node,
                                                candidateList,
                                                newVertexType,
                                                newVertexId);

        if (candidateListItem == NULL)
        {
            // Insert new candidate
            candidateListItem = (Ospfv2Vertex*)
                     MEM_malloc(sizeof(Ospfv2Vertex));

            candidateListItem->vertexId = newVertexId;
            candidateListItem->vertexType = newVertexType;
            candidateListItem->LSA = wLSA;
            candidateListItem->distance = newVertexDistance;

            Ospfv2InitList(&candidateListItem->nextHopList);

            if (Ospfv2SetNextHopForThisVertex(node, candidateListItem, v))
            {
                if (Ospfv2DebugSPT(node))
                {
                    char newVertexStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(newVertexId,newVertexStr);
                    printf("    Inserting new vertex %s\n", newVertexStr);
                }

                Ospfv2InsertToList(candidateList,
                                   0,
                                   (void*) candidateListItem);
            }
            else
            {
                Ospfv2FreeList(node, candidateListItem->nextHopList, FALSE);
                MEM_free(candidateListItem);
            }
        }
        else if (candidateListItem->distance > newVertexDistance)
        {
            // update
            candidateListItem->distance = newVertexDistance;

            if (Ospfv2DebugSPT(node))
            {
                char newVertexStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(newVertexId,newVertexStr);
                printf("    Update distance and next hop for vertex %s\n",
                            newVertexStr);
            }

            // Free old next hop items

            Ospfv2DeleteList(node, candidateListItem->nextHopList, FALSE);
            Ospfv2SetNextHopForThisVertex(node, candidateListItem, v);
        }
        else if (candidateListItem->distance == newVertexDistance)
        {
            if (Ospfv2DebugSPT(node))
            {
                char newVertexStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(newVertexId,newVertexStr);
                printf("    Add new set of next hop for vertex %s\n",
                            newVertexStr);
            }

            // Add new set of next hop values
            Ospfv2SetNextHopForThisVertex(node, candidateListItem, v);
        }
        else
        {
            // Examine next link
            continue;
        }
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2UpdateCandidateList
// PURPOSE: Update the candidate list using LSDB (router and network LSAs).
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2UpdateCandidateList(
    Node* node,
    Ospfv2Area* thisArea,
    Ospfv2List* candidateList,
    Ospfv2Vertex* v)
{

    if (v->vertexType == OSPFv2_VERTEX_NETWORK)
    {
        Ospfv2UpdateCandidateListUsingNetworkLSA(node,
                                                 thisArea,
                                                 candidateList,
                                                 v);
    }
    else
    {
        Ospfv2UpdateCandidateListUsingRouterLSA(node,
                                                thisArea,
                                                candidateList,
                                                v);
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2UpdateShortestPathList
// PURPOSE: Add new entry to the shortest path list.
// RETURN: The just added entry.
//-------------------------------------------------------------------------//

static
Ospfv2Vertex* Ospfv2UpdateShortestPathList(
    Node* node,
    Ospfv2Area* thisArea,
    Ospfv2List* candidateList)
{
    Ospfv2ListItem* candidateListItem = NULL;
    Ospfv2ListItem* listItem = NULL;

    Ospfv2Vertex* closestCandidate = NULL;
    Ospfv2Vertex* listEntry = NULL;

    Ospfv2Vertex* shortestPathListItem =
        (Ospfv2Vertex*) MEM_malloc(sizeof(Ospfv2Vertex));

    int lowestMetric = OSPFv2_LS_INFINITY;

    listItem = candidateList->first;

    // Get the vertex with the smallest metric from the candidate list...
    while (listItem)
    {
        listEntry = (Ospfv2Vertex*) listItem->data;

        if (listEntry->distance < (unsigned int) lowestMetric)
        {
            lowestMetric = listEntry->distance;

            // Keep track of closest vertex
            candidateListItem = listItem;
            closestCandidate = listEntry;
        }
        // Network vertex get preference over router vertex
        else if ((listEntry->distance == (unsigned int) lowestMetric)
                && (candidateListItem)
                && (closestCandidate->vertexType
                            == OSPFv2_VERTEX_ROUTER)
                && (listEntry->vertexType == OSPFv2_VERTEX_NETWORK))
        {
            // Keep track of closest vertex
            candidateListItem = listItem;
            closestCandidate = listEntry;
        }

        listItem = listItem->next;
    }

#ifdef JNE_LIB
    if (!candidateListItem)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Candidate list is not exists.",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(candidateListItem, "Candidate list is not exists.\n");

    memcpy(shortestPathListItem, closestCandidate, sizeof(Ospfv2Vertex));

    if (Ospfv2DebugSPT(node))
    {
        char vertexStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(shortestPathListItem->vertexId,
                                    vertexStr);

        printf("    added Vertex %s of type %d to shortest path list\n",
                    vertexStr,
                    shortestPathListItem->vertexType);
        printf("    metric is %d\n", shortestPathListItem->distance);
    }

    // Now insert it into the shortest path list...
    Ospfv2InsertToList(thisArea->shortestPathList,
                       0,
                       (void*) shortestPathListItem);

    if (Ospfv2DebugSPT(node))
    {
        char vertexStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(closestCandidate->vertexId,vertexStr);
        printf("    removing Vertex %s from candidate list\n",
                    vertexStr);

        printf("    size was %d\n", candidateList->size);
    }

    // And remove it from the candidate list since no longer available.
    Ospfv2RemoveFromList(node, candidateList, candidateListItem, FALSE);

    if (Ospfv2DebugSPT(node))
    {
        printf("    C_List size now %d\n", candidateList->size);
        Ospfv2PrintShortestPathList(node, thisArea->shortestPathList);
    }

    // Update my routing table to reflect the new shortest path list.
    Ospfv2UpdateIntraAreaRoute(node, thisArea, shortestPathListItem);

    return shortestPathListItem;
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2AddStubRouteToShortestPath
// PURPOSE: Add stub routes (links) to shortest path.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2AddStubRouteToShortestPath(Node* node, Ospfv2Area* thisArea)
{
    int i;
    Ospfv2ListItem* listItem = NULL;
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Vertex* tempV = NULL;
    Ospfv2RouterLSA* listLSA = NULL;
    Ospfv2LinkInfo* linkList = NULL;
    Ospfv2LinkInfo* nextLink = NULL;

    //NodeAddress nextHopAddr;
    unsigned int distance;

    if (Ospfv2DebugSPT(node))
    {
        printf("    Adding stub route to routing table...\n");
    }

    // Check each router vertex
    listItem = thisArea->shortestPathList->first;

    while (listItem)
    {
        tempV = (Ospfv2Vertex*) listItem->data;

        // Examine router vertex only
        if (tempV->vertexType != OSPFv2_VERTEX_ROUTER)
        {
            listItem = listItem->next;
            continue;
        }

        listLSA = (Ospfv2RouterLSA*) tempV->LSA;

        if (Ospfv2DebugSPT(node))
        {
            char nodeStr[MAX_ADDRESS_STRING_LENGTH];
            IO_ConvertIpAddressToString(listLSA->LSHeader.advertisingRouter,
                                        nodeStr);
            printf("    LSA from node %s\n", nodeStr);
        }

        // Skip LSA if max age is reached and examine the next LSA.
        if (Ospfv2MaskDoNotAge(ospf, listLSA->LSHeader.linkStateAge) >=
             (OSPFv2_LSA_MAX_AGE / SECOND))
        {
            listItem = listItem->next;
            continue;
        }

        nextLink = (Ospfv2LinkInfo*) (listLSA + 1);

        for (i = 0; i < listLSA->numLinks; i++)
        {
            int numTos;

            linkList = nextLink;

            // Place the nextLink pointer properly
            numTos = nextLink->numTOS;
            nextLink = (Ospfv2LinkInfo*)
                ((QospfPerLinkQoSMetricInfo*)(nextLink + 1)
                + numTos);

            // Examine stub network only
            if (linkList->type == OSPFv2_STUB)
            {
                Ospfv2RoutingTableRow* rowPtr;
                Ospfv2RoutingTableRow newRow;

                // Don't process if this indicates to my address
                if (Ospfv2IsMyAddress(node, linkList->linkID))
                {
                    continue;
                }

                // Calculate distance from root

                // linkList->metric is a SIGNED short, while distance and
                // tempV->distance are UNSIGNED.
                distance =
                        (unsigned short)linkList->metric + tempV->distance;

                // Get this route from routing table
                // handling for host route
                if ((unsigned int)linkList->linkData == 0xffffffff)
                {
                    rowPtr = Ospfv2GetValidHostRoute(node,
                                                 linkList->linkID,
                                                 OSPFv2_DESTINATION_NETWORK);
                }
                else
                {
                    rowPtr = Ospfv2GetValidRoute(node,
                                                 linkList->linkID,
                                                 OSPFv2_DESTINATION_NETWORK);
                }

                if (rowPtr != NULL)
                {
                    // If the calculated distance is larger than previous,
                    // examine next stub link
                    //
                    // TBD: If metrics are equal just add the next hop in
                    //      list.
                    // Do that once we able to support equal cost multipath.

                    if (distance >= (unsigned int) rowPtr->metric)
                    {
                        continue;
                    }

                    Ospfv2FreeRoute(node, rowPtr);
                }

                // Add new route
                newRow.destType = OSPFv2_DESTINATION_NETWORK;
                newRow.destAddr = linkList->linkID;
                newRow.addrMask = linkList->linkData;
                newRow.option = 0;
                newRow.areaId = thisArea->areaID;
                newRow.pathType = OSPFv2_INTRA_AREA;
                newRow.metric = distance;
                newRow.type2Metric = 0;
                newRow.LSOrigin = (void*) tempV->LSA;
                newRow.advertisingRouter =
                    listLSA->LSHeader.advertisingRouter;

                // TBD: consider one next hop for now
                // If stub network is part of the node's interfaces
                if (tempV->vertexId == ospf->routerID)
                {
                    int intfId;

                    // handling for host route
                    if ((unsigned int)linkList->linkData != 0xffffffff)
                    {
                        intfId = Ospfv2GetInterfaceForThisNeighbor(
                                    node,
                                    linkList->linkID);
                    }
                    else
                    {
                        intfId = NetworkIpGetInterfaceIndexForNextHop(
                                    node,
                                    linkList->linkID);
                    }

                    if (intfId == -1)
                    {
                        Ospfv2LinkStateHeader* LSHeader = NULL;
                        LSHeader = (Ospfv2LinkStateHeader*)
                            Ospfv2LookupLSAList(thisArea->routerLSAList,
                                                linkList->linkID,
                                                     linkList->linkID);
                        if (LSHeader != NULL)
                        {
                            Ospfv2RemoveLSAFromLSDB(node,
                                                    (char*)LSHeader,
                                                    thisArea->areaID);
                            Ospfv2RemoveLSAFromShortestPathList(node,
                                thisArea->shortestPathList,
                                (char*)LSHeader);
                            if (OSPFv2_DEBUG_TABLEErr)
                            {
                                Ospfv2PrintLSDB(node);
                            }
                        }
                        continue;
                    }

                    newRow.outIntf = intfId;
                    newRow.nextHop = 0;
                }
                else
                {
                    if (tempV->nextHopList == NULL)
                    {
                        continue;
                    }
                    if (tempV->nextHopList->first == NULL)
                    {
                        continue;
                    }
                    if (tempV->nextHopList->first->data == NULL)
                    {
                        continue;
                    }

                    Ospfv2NextHopListItem* nextHopItem =
                                        (Ospfv2NextHopListItem*)
                                        tempV->nextHopList->first->data;

                    newRow.nextHop = nextHopItem->nextHop;
                    newRow.outIntf = nextHopItem->outIntf;

                }

                Ospfv2AddRoute(node, &newRow);
            }
        }

        listItem = listItem->next;
    }
}


//-------------------------------------------------------------------------//
// NAME: Ospfv2FreeVertexList()
// PURPOSE: Free vertex list.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2FreeVertexList(Node* node, Ospfv2List* vertexList)
{

    Ospfv2ListItem* listItem = vertexList->first;

    while (listItem)
    {
        Ospfv2Vertex* vertex = (Ospfv2Vertex*) listItem->data;

        Ospfv2FreeList(node, vertex->nextHopList, FALSE);
        listItem = listItem->next;
    }

    Ospfv2FreeList(node, vertexList, FALSE);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2FindShortestPathForThisArea
// PURPOSE      :Calculate the shortest path to all node in an area
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2FindShortestPathForThisArea(
    Node* node,
    unsigned int areaId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2List* candidateList = NULL;
    Ospfv2Vertex* tempV = NULL;

    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);

#ifdef JNE_LIB
    if (!thisArea)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Specified Area is not found",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(thisArea, "Specified Area is not found");

    Ospfv2InitList(&thisArea->shortestPathList);
    Ospfv2InitList(&candidateList);

    tempV = (Ospfv2Vertex*) MEM_malloc(sizeof(Ospfv2Vertex));

    if (Ospfv2DebugSPT(node))
    {
        printf("    Node %u Calculating shortest path for area %d\n",
                    node->nodeId, areaId);
        Ospfv2PrintLSDB(node);
    }

    // The shortest path starts with myself as the root.
    tempV->vertexId = ospf->routerID;
    tempV->vertexType = OSPFv2_VERTEX_ROUTER;
    tempV->LSA = (char*) Ospfv2LookupLSAList(thisArea->routerLSAList,
                                             ospf->routerID,
                                             ospf->routerID);

    if ((tempV->LSA == NULL) || (Ospfv2LSAHasMaxAge(ospf, tempV->LSA)))
    {
        MEM_free(tempV);
        Ospfv2FreeList(node, thisArea->shortestPathList, FALSE);
        Ospfv2FreeList(node, candidateList, FALSE);
        return;
    }

    Ospfv2InitList(&tempV->nextHopList);
    tempV->distance = 0;

    // Insert myself (root) to the shortest path list.
    Ospfv2InsertToList(thisArea->shortestPathList, 0, (void*) tempV);

    // Find candidates to be considered for the shortest path list.
    Ospfv2UpdateCandidateList(node, thisArea, candidateList, tempV);

    if (Ospfv2DebugSPT(node))
    {
        Ospfv2PrintCandidateList(node, candidateList);
    }

    // Keep calculating shortest path until the candidate list is empty.
    while (candidateList->size > 0)
    {
        // Select the next best node in the candidate list into
        // the shortest path list.  That node is tempV.
        tempV = Ospfv2UpdateShortestPathList(node,
                                             thisArea,
                                             candidateList);

        if (Ospfv2DebugSPT(node))
        {
            Ospfv2PrintShortestPathList(node, thisArea->shortestPathList);
        }

        // Find more candidates to be considered for the shortest path list.
        Ospfv2UpdateCandidateList(node, thisArea, candidateList, tempV);

        if (Ospfv2DebugSPT(node))
        {
            Ospfv2PrintCandidateList(node, candidateList);
        }
    }

    // Add stub routes to the shortest path list.
    Ospfv2AddStubRouteToShortestPath(node, thisArea);

    Ospfv2FreeVertexList(node, thisArea->shortestPathList);
    Ospfv2FreeVertexList(node, candidateList);
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2ExamineSummaryLSA
// PURPOSE: Examine this area's Summary LSA for inter area routes.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2ExamineSummaryLSA(
    Node* node,
    Ospfv2Area* thisArea,
    Ospfv2LinkStateType summaryLSAType,
    BOOL checkForRospfMobileLeaf = FALSE)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2RoutingTableRow* rowPtr = NULL;
    Ospfv2RoutingTableRow* oldRoute = NULL;
    Ospfv2List* list = NULL;
    Ospfv2ListItem* listItem = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    char* LSABody = NULL;

    if (summaryLSAType == OSPFv2_NETWORK_SUMMARY)
    {
        list = thisArea->networkSummaryLSAList;
    }
    else if (summaryLSAType == OSPFv2_ROUTER_SUMMARY)
    {
        list = thisArea->routerSummaryLSAList;
    }
    else
    {
#ifdef JNE_LIB
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Invalid Summary LSA type.",
            JNE::CRITICAL);
#endif
        ERROR_Assert(FALSE, "Invalid Summary LSA type.");
    }

    listItem = list->first;

    while (listItem)
    {
        Ospfv2RoutingTableRow newRow;
        NodeAddress destAddr;
        NodeAddress addrMask;
        NodeAddress borderRt;
        Int32 metric;

        LSHeader = (Ospfv2LinkStateHeader*) listItem->data;
        LSABody = (char*) (LSHeader + 1);

        destAddr = LSHeader->linkStateID;
        memcpy(&addrMask, LSABody, sizeof(NodeAddress));
        LSABody += sizeof(NodeAddress);

        if (summaryLSAType == OSPFv2_NETWORK_SUMMARY)
        {
            destAddr = destAddr & addrMask;
        }

        borderRt = LSHeader->advertisingRouter;

        memcpy(&metric, LSABody, sizeof(Int32));
        LSABody += sizeof(Int32);
#ifdef ADDON_BOEINGFCS

        // ignore LSA if the border router is not a mobile leaf
        if (checkForRospfMobileLeaf &&
            !RoutingCesRospfInMobileLeafList(node, thisArea, borderRt))
        {
            listItem = listItem->next;
            continue;
        }
#endif

        metric = metric & 0xFFFFFF;

        // If cost equal to LSInfinity or age equal to MxAge, examine next
        if ((metric == OSPFv2_LS_INFINITY)
            || (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) ==
                                   (OSPFv2_LSA_MAX_AGE / SECOND)))
        {
            listItem = listItem->next;
            continue;
        }

        // Don't process self originated LSA
        if (LSHeader->advertisingRouter == ospf->routerID)
        {
            listItem = listItem->next;
            continue;
        }

        // If it is a Type 3 summary-LSA, and the collection of destinations
        // described by the summary-LSA equals one of the router's configured
        // area address ranges and there are one or more reachable networks
        // contained in the area range then the summary-LSA should be ignored
        if (summaryLSAType == OSPFv2_NETWORK_SUMMARY)
        {
            Ospfv2AreaAddressRange* addrInfo =
                Ospfv2GetAddrRangeInfoForAllArea(node, destAddr);

            if (addrInfo != NULL)
            {
                rowPtr = Ospfv2GetValidRoute(node,
                                             destAddr,
                                             OSPFv2_DESTINATION_NETWORK);

                if (rowPtr != NULL)
                {
                    listItem = listItem->next;
                    continue;
                }
            }
        }

        // Lookup the routing table entry for border router having
        // this area as associated area
        rowPtr = Ospfv2GetIntraAreaRoute(node,
                                         borderRt,
                                         OSPFv2_DESTINATION_ABR,
                                         thisArea->areaID);

        if ((rowPtr == NULL) ||
            (rowPtr->flag == OSPFv2_ROUTE_INVALID))
        {
            listItem = listItem->next;
            continue;
        }
        newRow.destAddr = destAddr;
        newRow.option = 0;
        newRow.areaId = thisArea->areaID;
        newRow.pathType = OSPFv2_INTER_AREA;
        newRow.metric = rowPtr->metric + metric;

        // BGP-OSPF Patch Start
        if (!destAddr)
        {
            newRow.metric = metric;
        }
        // BGP-OSPF Patch End

        newRow.type2Metric = 0;
        newRow.LSOrigin = (void*) listItem->data;
        newRow.advertisingRouter = borderRt;
        newRow.nextHop = rowPtr->nextHop;
        newRow.outIntf = rowPtr->outIntf;

        if (summaryLSAType == OSPFv2_NETWORK_SUMMARY)
        {
            newRow.destType = OSPFv2_DESTINATION_NETWORK;
            newRow.addrMask = addrMask;

            oldRoute = Ospfv2GetValidRoute(node,
                                           destAddr,
                                           OSPFv2_DESTINATION_NETWORK);
        }
        else
        {
            newRow.destType = OSPFv2_DESTINATION_ASBR;
            newRow.addrMask = 0xFFFFFFFF;

            oldRoute = Ospfv2GetValidRoute(node,
                                           destAddr,
                                           OSPFv2_DESTINATION_ASBR);
        }

        if ((oldRoute == NULL)
            || (oldRoute->pathType == OSPFv2_TYPE1_EXTERNAL)
            || (oldRoute->pathType == OSPFv2_TYPE2_EXTERNAL))
        {
            Ospfv2AddInterAreaRoute(node, &newRow);
        }
        else if (oldRoute->pathType == OSPFv2_INTRA_AREA)
        {
            // Do nothing
        }
        else
        {
            // Install if new path is cheaper
            //TBD: If cost area equal, add path. Do that after enabling
            //     equal cost multipath
            if (newRow.metric < oldRoute->metric)
            {
                if (summaryLSAType != OSPFv2_NETWORK_SUMMARY
                    && rowPtr->destType == OSPFv2_DESTINATION_ABR_ASBR)
                {
                    newRow.destType = OSPFv2_DESTINATION_ABR_ASBR;
                }

                Ospfv2AddInterAreaRoute(node, &newRow);
            }
        }

        listItem = listItem->next;

    }
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2ExamineTransitAreaSummaryLSA
// PURPOSE: Examine this area's Summary LSA for inter area routes.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2ExamineTransitAreaSummaryLSA(
    Node* node,
    Ospfv2Area* thisArea,
    Ospfv2LinkStateType summaryLSAType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2RoutingTableRow* rowPtr = NULL;
    Ospfv2RoutingTableRow* oldRoute = NULL;
    Ospfv2List* list = NULL;
    Ospfv2ListItem* listItem = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    char* LSABody = NULL;

    if (summaryLSAType == OSPFv2_NETWORK_SUMMARY)
    {
        list = thisArea->networkSummaryLSAList;
    }
    else if (summaryLSAType == OSPFv2_ROUTER_SUMMARY)
    {
        list = thisArea->routerSummaryLSAList;
    }
    else
    {
#ifdef JNE_LIB
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Invalid Summary LSA type.",
            JNE::CRITICAL);
#endif
        ERROR_Assert(FALSE, "Invalid Summary LSA type.");
    }

    listItem = list->first;

    while (listItem)
    {
        Ospfv2RoutingTableRow newRow;
        NodeAddress destAddr;
        NodeAddress addrMask;
        NodeAddress borderRt;
        Int32 metric;
        Int32 newCost = 0;

        LSHeader = (Ospfv2LinkStateHeader*) listItem->data;
        LSABody = (char*) (LSHeader + 1);

        destAddr = LSHeader->linkStateID;
        memcpy(&addrMask, LSABody, sizeof(NodeAddress));
        LSABody += sizeof(NodeAddress);

        if (summaryLSAType == OSPFv2_NETWORK_SUMMARY)
        {
            destAddr = destAddr & addrMask;
        }

        borderRt = LSHeader->advertisingRouter;

        memcpy(&metric, LSABody, sizeof(Int32));
        LSABody += sizeof(Int32);

        metric = metric & 0xFFFFFF;

        // If cost equal to LSInfinity or age equal to MxAge, examine next
        if ((metric == OSPFv2_LS_INFINITY)
            || (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) ==
                       (OSPFv2_LSA_MAX_AGE / SECOND)))
        {
            listItem = listItem->next;
            continue;
        }

        // Don't process self originated LSA
        if (LSHeader->advertisingRouter == ospf->routerID)
        {
            listItem = listItem->next;
            continue;
        }

        if (summaryLSAType == OSPFv2_NETWORK_SUMMARY)
        {
            oldRoute = Ospfv2GetValidRoute(node,
                                           destAddr,
                                           OSPFv2_DESTINATION_NETWORK);
        }
        else
        {
            oldRoute = Ospfv2GetBackboneValidRoute(node,
                                           destAddr,
                                           OSPFv2_DESTINATION_ASBR);
        }

        if ((oldRoute == NULL)
            || ((oldRoute->pathType != OSPFv2_INTER_AREA)
             && (oldRoute->pathType != OSPFv2_INTRA_AREA))
            || (oldRoute->areaId != OSPFv2_BACKBONE_AREA))
        {
            listItem = listItem->next;
            continue;
        }

        rowPtr = Ospfv2GetIntraAreaRoute(node,
                                         borderRt,
                                         OSPFv2_DESTINATION_ABR,
                                         thisArea->areaID);

        if ((rowPtr == NULL) ||
            (rowPtr->flag == OSPFv2_ROUTE_INVALID))
        {
            listItem = listItem->next;
            continue;
        }
        else
        {
            newCost = rowPtr->metric + metric;
        }

        newRow.addrMask = oldRoute->addrMask;
        newRow.advertisingRouter = oldRoute->advertisingRouter;
        newRow.areaId = oldRoute->areaId;
        newRow.destAddr = oldRoute->destAddr;
        newRow.destType = oldRoute->destType;
        newRow.flag = oldRoute->flag;
        newRow.LSOrigin = oldRoute->LSOrigin;
        newRow.metric = oldRoute->metric;
        newRow.nextHop = oldRoute->nextHop;
        newRow.option = oldRoute->option;
        newRow.outIntf = oldRoute->outIntf;
        newRow.pathType = oldRoute->pathType;
        newRow.type2Metric = oldRoute->type2Metric;

        if (newCost <= oldRoute->metric)
        {
            newRow.nextHop = rowPtr->nextHop;
            newRow.metric = newCost;
            newRow.outIntf = rowPtr->outIntf;
            newRow.areaId = rowPtr->areaId;
            Ospfv2AddInterAreaRoute(node, &newRow);
        }

        listItem = listItem->next;
    }
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2FindInterAreaPath
// PURPOSE: Calculate Inter Area paths.
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2FindInterAreaPath(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    bool rospfOnAnyInterface = false;
#ifdef ADDON_BOEINGFCS
    rospfOnAnyInterface = RoutingCesRospfActiveOnAnyInterface(node);
#endif

    // Examine only Backbone Summary LSA if the router
    // has active attachment to multiple areas.

    if (!ospf->backboneArea && !rospfOnAnyInterface &&
            (ospf->areaBorderRouter == TRUE ||
             ospf->nssaAreaBorderRouter == TRUE))
    {
        ERROR_ReportError("Node is an ABR but not any "
            "interface belong into the backbone area\n");
    }
    else if (ospf->backboneArea &&
            (ospf->areaBorderRouter == TRUE ||
             ospf->nssaAreaBorderRouter == TRUE))
    {

        Ospfv2ExamineSummaryLSA(node,
                                ospf->backboneArea,
                                OSPFv2_NETWORK_SUMMARY);
        Ospfv2ExamineSummaryLSA(node,
                                ospf->backboneArea,
                                OSPFv2_ROUTER_SUMMARY);

        Ospfv2ListItem* listItem = ospf->area->first;

        while (listItem)
        {
            Ospfv2Area* thisArea = (Ospfv2Area*) listItem->data;

            if ((thisArea->areaID != OSPFv2_BACKBONE_AREA)
                && (thisArea->transitCapability == TRUE))
            {
                Ospfv2ExamineTransitAreaSummaryLSA(node,
                                        thisArea,
                                        OSPFv2_NETWORK_SUMMARY);
                Ospfv2ExamineTransitAreaSummaryLSA(node,
                                        thisArea,
                                        OSPFv2_ROUTER_SUMMARY);
            }

#ifdef ADDON_BOEINGFCS
            // this is for adding paths to mobile leaf areas
            if (rospfOnAnyInterface && (thisArea->areaID != OSPFv2_BACKBONE_AREA))
            {
                Ospfv2ExamineSummaryLSA(node,
                                        thisArea,
                                        OSPFv2_NETWORK_SUMMARY,
                                        TRUE);
                Ospfv2ExamineSummaryLSA(node,
                                        thisArea,
                                        OSPFv2_ROUTER_SUMMARY,
                                        TRUE);
            }
#endif
            listItem = listItem->next;
        }
    }
    else
    {
        Ospfv2Area* thisArea =
                (Ospfv2Area*) ospf->area->first->data;

        if (!rospfOnAnyInterface)
        {
#ifdef JNE_LIB
            if (ospf->area->size != 1)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Node is an interior router, "
                    "so it should belong to only one area.",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(ospf->area->size == 1, "Node is an interior router, "
                "so it should belong to only one area.\n");
        }

        Ospfv2ExamineSummaryLSA(node,
                                thisArea,
                                OSPFv2_NETWORK_SUMMARY);

        Ospfv2ExamineSummaryLSA(node,
                                thisArea,
                                OSPFv2_ROUTER_SUMMARY);

    }
}


// BGP-OSPF Patch Start
//-------------------------------------------------------------------------//
// NAME         :Ospfv2FindASExternalPath
// PURPOSE      :Calculate shortest path for the external route.
//               (As specified in Section 16 of RFC 2328)
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2FindASExternalPath(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2List* list = ospf->asExternalLSAList;
    Ospfv2ListItem* listItem = list->first;

    while (listItem)
    {
        Ospfv2RoutingTableRow* rowPtr = NULL;
        Ospfv2RoutingTableRow newRow;
        Int32 cost = 0;

        Ospfv2LinkStateHeader* LSHeader =
            (Ospfv2LinkStateHeader*) listItem->data;
        NodeAddress destAddr = LSHeader->linkStateID;
        NodeAddress asExternalRt = LSHeader->advertisingRouter;

        Ospfv2ExternalLinkInfo* LSABody =
                                    (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
        NodeAddress addrMask = LSABody->networkMask;
        NodeAddress forwardingAdd = LSABody->forwardingAddress;

        destAddr = destAddr & addrMask;

        cost = Ospfv2ExternalLinkGetMetric(LSABody->ospfMetric);

        if ((cost == OSPFv2_LS_INFINITY)
            || (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) ==
                                    (OSPFv2_LSA_MAX_AGE / SECOND)))
        {
            listItem = listItem->next;
            continue;
        }

        // Don't process self originated LSA
        if (asExternalRt == ospf->routerID)
        {
            listItem = listItem->next;
            continue;
        }

        rowPtr = Ospfv2GetValidRoute(
                    node, asExternalRt, OSPFv2_DESTINATION_NETWORK);
        if (!rowPtr)
        {
            rowPtr = Ospfv2GetValidRoute(
                    node, asExternalRt, OSPFv2_DESTINATION_ASBR);
        }

        // If no entry exist for advertising Router (ASBR), do nothing.
        if (!rowPtr)
        {
            listItem = listItem->next;
            continue;
        }

        newRow.destAddr = destAddr;
        newRow.option = 0;
        newRow.areaId = 0;
        newRow.pathType = OSPFv2_TYPE2_EXTERNAL;
        newRow.type2Metric = cost;
        newRow.LSOrigin = (void*) listItem->data;
        newRow.advertisingRouter = asExternalRt;
        newRow.destType = OSPFv2_DESTINATION_NETWORK;
        newRow.addrMask = addrMask;

        if (!forwardingAdd)
        {
            newRow.metric = rowPtr->metric;
            newRow.nextHop = rowPtr->nextHop;
            newRow.outIntf = rowPtr->outIntf;
        }
        else
        {
            rowPtr = Ospfv2GetValidRoute(
                        node, forwardingAdd, OSPFv2_DESTINATION_NETWORK);

            //Intra-area or Inter-area path must exist.
            //If no path exist, do nothing
            if (!rowPtr
                || (rowPtr->pathType != OSPFv2_INTRA_AREA
                    && rowPtr->pathType != OSPFv2_INTER_AREA))
            {
                listItem = listItem->next;
                continue;
            }
            newRow.metric = rowPtr->metric;
            newRow.nextHop = rowPtr->nextHop;
            newRow.outIntf = rowPtr->outIntf;
        }
#ifdef ADDON_MA
        rowPtr = Ospfv2GetRoute_Extended(node,
                                        newRow.destAddr,
                                        newRow.destType,
                                        newRow.addrMask,
                                        newRow.pathType);
#else
        rowPtr = Ospfv2GetValidRoute(node, newRow.destAddr, newRow.destType);
#endif

        //Route exist into the Routing Table.
        if (rowPtr)
        {
            //Intra-area and inter-area paths are always preferred
            //over AS external paths.
            if ((rowPtr->pathType == OSPFv2_INTRA_AREA) ||
                (rowPtr->pathType == OSPFv2_INTER_AREA))
            {
                rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
            }
            else if (rowPtr->pathType == OSPFv2_TYPE1_EXTERNAL)
            {
                //Type 1 external paths are compared by looking at the sum
                //of the distance to the forwarding address and the
                //advertised type 1 metric (X+Y).
                if (newRow.pathType == OSPFv2_TYPE1_EXTERNAL)
                {
                    if (newRow.metric >= rowPtr->metric)
                    {
                        rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                    }
                    else
                    {
                        newRow.flag = OSPFv2_ROUTE_CHANGED;
                        memcpy(rowPtr,
                               &newRow,
                               sizeof(Ospfv2RoutingTableRow));
                    }
                }
                //Type 1 external paths are always preferred over
                //type 2 external paths
                else
                {
                    rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                }
            }
            else
            {
                //Type 1 external paths are always preferred over
                //type 2 external paths
                if (newRow.pathType == OSPFv2_TYPE1_EXTERNAL)
                {
                    newRow.flag = OSPFv2_ROUTE_CHANGED;
                    memcpy(rowPtr, &newRow, sizeof(Ospfv2RoutingTableRow));
                }
                else if (newRow.type2Metric > rowPtr->type2Metric)
                {
                    rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                }
                else if (newRow.type2Metric == rowPtr->type2Metric)
                {
                    if (rowPtr->metric <= newRow.metric)
                    {
                        rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                    }
                    else
                    {
                        newRow.flag = OSPFv2_ROUTE_CHANGED;
                        memcpy(rowPtr,
                               &newRow,
                               sizeof(Ospfv2RoutingTableRow));
                    }
                }
                else
                {
                    newRow.flag = OSPFv2_ROUTE_CHANGED;
                    memcpy(rowPtr, &newRow, sizeof(Ospfv2RoutingTableRow));
                }
            }
        }
        else
        {
            Ospfv2RoutingTable* rtTable = &ospf->routingTable;

            newRow.flag = OSPFv2_ROUTE_NEW;
            rtTable->numRows++;
            BUFFER_AddDataToDataBuffer(&rtTable->buffer,
                                       (char*) (&newRow),
                                       sizeof(Ospfv2RoutingTableRow));
        }

        listItem = listItem->next;
    }
}
// BGP-OSPF Patch End


//-------------------------------------------------------------------------//
// NAME         :Ospfv2FindNSSAExternalPath
// PURPOSE      :Calculate shortest path for the NSSA external route.
//               (As specified in Section 3.5 of RFC 1587)
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2FindNSSAExternalPath(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2List* list = ospf->nssaExternalLSAList;
    Ospfv2ListItem* listItem = list->first;

    while (listItem)
    {
        Ospfv2RoutingTableRow* rowPtr = NULL;
        Ospfv2RoutingTableRow newRow;
        Int32 metric = 0;

        Ospfv2LinkStateHeader* LSHeader =
            (Ospfv2LinkStateHeader*) listItem->data;
        NodeAddress destAddr = LSHeader->linkStateID;
        NodeAddress nssaExternalRt = LSHeader->advertisingRouter;

        Ospfv2ExternalLinkInfo* LSABody =
                                    (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
        NodeAddress addrMask = LSABody->networkMask;
        NodeAddress forwardingAdd = LSABody->forwardingAddress;

        destAddr = destAddr & addrMask;

        metric = Ospfv2ExternalLinkGetMetric(LSABody->ospfMetric);

        // If metric cost equal to LSInfinity, or age equal to MaxAge,
        // or if it is a self originated LSA, then examine next
        if ((metric == OSPFv2_LS_INFINITY)
            || (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) ==
                                         (OSPFv2_LSA_MAX_AGE / SECOND))
            || (nssaExternalRt == ospf->routerID))
        {
            listItem = listItem->next;
            continue;
        }

        rowPtr = Ospfv2GetValidRoute(node,
                                     nssaExternalRt,
                                     OSPFv2_DESTINATION_ASBR);

        // If no entry exist for advertising Router (ASBR), do nothing.
        if (!rowPtr)
        {
            rowPtr = Ospfv2GetValidRoute(node,
                                         nssaExternalRt,
                                         OSPFv2_DESTINATION_ABR_ASBR);
            if (!rowPtr)
            {
               listItem = listItem->next;
               continue;
             }
        }

        // if destination is default destination, and originator of LSA and
        // calculating router are both NSSA area broder routers, do nothing
        if (destAddr == OSPFv2_DEFAULT_DESTINATION
            && (ospf->nssaAreaBorderRouter
                && Ospfv2IsNSSA_ABR(node, nssaExternalRt)))
        {
            listItem = listItem->next;
            continue;
        }

        newRow.destAddr = destAddr;
        newRow.option = LSHeader->options;
        newRow.areaId = 0;
        newRow.pathType = OSPFv2_TYPE2_EXTERNAL;
        newRow.type2Metric = metric;
        newRow.LSOrigin = (void*) listItem->data;
        newRow.advertisingRouter = nssaExternalRt;
        newRow.destType = OSPFv2_DESTINATION_NETWORK;
        newRow.addrMask = addrMask;

        if (!forwardingAdd)
        {
            newRow.metric = rowPtr->metric;
            newRow.nextHop = rowPtr->nextHop;
            newRow.outIntf = rowPtr->outIntf;
        }
        else
        {
            Ospfv2RoutingTableRow* routerRowPtr1 = NULL;
            Ospfv2RoutingTableRow* routerRowPtr2 = NULL;
            BOOL wasFound = FALSE;

            routerRowPtr1 = Ospfv2GetValidRoute(node,
                                         forwardingAdd,
                                         OSPFv2_DESTINATION_ASBR);
            if (routerRowPtr1)
            {
                if (routerRowPtr1->pathType == OSPFv2_INTRA_AREA)
                {
                    rowPtr = routerRowPtr1;
                    wasFound = TRUE;
                }
            }

            if (!routerRowPtr1 || !wasFound)
            {
                routerRowPtr2 = Ospfv2GetValidRoute(node,
                                            forwardingAdd,
                                            OSPFv2_DESTINATION_ABR_ASBR);
                if (routerRowPtr2)
                {
                    if (routerRowPtr2->pathType == OSPFv2_INTRA_AREA)
                    {
                        rowPtr = routerRowPtr2;
                        wasFound = TRUE;
                    }
                }
            }

            if (!wasFound)
            {
                //Intra-area path must exist. If no path exist, do nothing
                listItem = listItem->next;
                continue;
            }

            newRow.metric = rowPtr->metric;
            newRow.nextHop = rowPtr->nextHop;
            newRow.outIntf = rowPtr->outIntf;
        }
 #ifdef ADDON_MA
        rowPtr = Ospfv2GetRoute_Extended(node,
                                        newRow.destAddr,
                                        newRow.destType,
                                        newRow.addrMask,
                                        newRow.pathType);
#else
        rowPtr = Ospfv2GetValidRoute(node, newRow.destAddr, newRow.destType);
#endif

        //Route exist into the Routing Table.
        if (rowPtr)
        {
            //If path type is not Type-1 or Type-2 external path, do nothing
            if (rowPtr->pathType != OSPFv2_TYPE1_EXTERNAL
                && rowPtr->pathType != OSPFv2_TYPE2_EXTERNAL)
            {
                listItem = listItem->next;
                continue;
            }

            if (rowPtr->pathType == OSPFv2_TYPE1_EXTERNAL)
            {
                //Type 1 external paths are compared by looking at the sum
                //of the distance to the forwarding address and the
                //advertised type 1 metric (X+Y).
                if (newRow.pathType == OSPFv2_TYPE1_EXTERNAL)
                {
                    if (newRow.metric >= rowPtr->metric)
                    {
                        rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                    }
                    else if (newRow.metric == rowPtr->metric)
                    {
                        //rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                        Ospfv2LinkStateHeader* rowPtrLSHeader =
                                 (Ospfv2LinkStateHeader*) rowPtr->LSOrigin;
                        Ospfv2ExternalLinkInfo* rowPtrLSABody =
                            (Ospfv2ExternalLinkInfo*) (rowPtrLSHeader + 1);
                        NodeAddress rowPtrForwardingAdd =
                                           rowPtrLSABody->forwardingAddress;

                        //When a type-5 LSA and a type-7 LSA are found to
                        //have the same type and an equal distance, the
                        //following priorities apply (listed from highest to
                        //lowest) for breaking the tie:
                        //   a. Any type 5 LSA.
                        //   b. A type-7 LSA with the P-bit set and the
                        //      forwarding address non-zero.
                        //   c. Any other type-7 LSA.
                        if ((rowPtrLSHeader->linkStateType ==
                                                     LSHeader->linkStateType)
                             && (LSHeader->linkStateType ==
                                                  OSPFv2_ROUTER_AS_EXTERNAL))
                        {
                            rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                        }
                        else if ((rowPtrLSHeader->linkStateType ==
                                                     LSHeader->linkStateType)
                             && (LSHeader->linkStateType ==
                                                OSPFv2_ROUTER_NSSA_EXTERNAL))
                        {
                            if ((Ospfv2OptionsGetNSSACapability(
                                                    rowPtrLSHeader->options))
                                 && rowPtrForwardingAdd)
                            {
                                rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                            }
                            else if ((Ospfv2OptionsGetNSSACapability(
                                                          LSHeader->options))
                                    && forwardingAdd)
                            {
                                newRow.flag = OSPFv2_ROUTE_CHANGED;
                                memcpy(rowPtr,
                                       &newRow,
                                       sizeof(Ospfv2RoutingTableRow));
                            }
                            else
                            {
                                rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                            }
                        }
                        else
                        {
                            rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                        }
                    }
                    else
                    {
                        newRow.flag = OSPFv2_ROUTE_CHANGED;
                        memcpy(rowPtr,
                               &newRow,
                               sizeof(Ospfv2RoutingTableRow));
                    }
                }
                //Type 1 external paths are always preferred over
                //type 2 external paths
                else
                {
                    rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                }
            }
            else if (rowPtr->pathType == OSPFv2_TYPE2_EXTERNAL)
            {
                //Type 1 external paths are always preferred over
                //type 2 external paths
                if (newRow.pathType == OSPFv2_TYPE1_EXTERNAL)
                {
                    newRow.flag = OSPFv2_ROUTE_CHANGED;
                    memcpy(rowPtr, &newRow, sizeof(Ospfv2RoutingTableRow));
                }
                else if (newRow.type2Metric > rowPtr->type2Metric)
                {
                    rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                }
                else if (newRow.type2Metric == rowPtr->type2Metric)
                {
                    if (rowPtr->metric < newRow.metric)
                    {
                        rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                    }
                    else if (rowPtr->metric == newRow.metric)
                    {
                        //rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                        Ospfv2LinkStateHeader* rowPtrLSHeader =
                                 (Ospfv2LinkStateHeader*) rowPtr->LSOrigin;
                        Ospfv2ExternalLinkInfo* rowPtrLSABody =
                            (Ospfv2ExternalLinkInfo*) (rowPtrLSHeader + 1);
                        NodeAddress rowPtrForwardingAdd =
                                            rowPtrLSABody->forwardingAddress;

                        //When a type-5 LSA and a type-7 LSA are found to
                        //have the same type and an equal distance, the
                        //following priorities apply (listed from highest to
                        //lowest) for breaking the tie:
                        //   a. Any type 5 LSA.
                        //   b. A type-7 LSA with the P-bit set and the
                        //      forwarding address non-zero.
                        //   c. Any other type-7 LSA.
                        if ((rowPtrLSHeader->linkStateType ==
                                                     LSHeader->linkStateType)
                             && (LSHeader->linkStateType ==
                                                  OSPFv2_ROUTER_AS_EXTERNAL))
                        {
                            rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                        }
                        else if ((rowPtrLSHeader->linkStateType ==
                                                     LSHeader->linkStateType)
                             && (LSHeader->linkStateType ==
                                                OSPFv2_ROUTER_NSSA_EXTERNAL))
                        {
                            if ((Ospfv2OptionsGetNSSACapability(
                                                    rowPtrLSHeader->options))
                                 && rowPtrForwardingAdd)
                            {
                                rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                            }
                            else if ((Ospfv2OptionsGetNSSACapability(
                                                          LSHeader->options))
                                    && forwardingAdd)
                            {
                                newRow.flag = OSPFv2_ROUTE_CHANGED;
                                memcpy(rowPtr,
                                       &newRow,
                                       sizeof(Ospfv2RoutingTableRow));
                            }
                            else
                            {
                                rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                            }
                        }
                        else
                        {
                            rowPtr->flag = OSPFv2_ROUTE_NO_CHANGE;
                        }
                    }
                    else
                    {
                        newRow.flag = OSPFv2_ROUTE_CHANGED;
                        memcpy(rowPtr,&newRow,sizeof(Ospfv2RoutingTableRow));
                    }
                }
                else
                {
                    newRow.flag = OSPFv2_ROUTE_CHANGED;
                    memcpy(rowPtr, &newRow, sizeof(Ospfv2RoutingTableRow));
                }
            }
        }
        else
        {
            Ospfv2RoutingTable* rtTable = &ospf->routingTable;

            newRow.flag = OSPFv2_ROUTE_NEW;
            rtTable->numRows++;
            BUFFER_AddDataToDataBuffer(&rtTable->buffer,
                                       (char*) (&newRow),
                                       sizeof(Ospfv2RoutingTableRow));
        }

        listItem = listItem->next;
    }
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2FindShortestPath
// PURPOSE      :Calculate shortest path to all other nodes from this node.
//               (As specified in Section 16 of RFC 2328)
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

static
void Ospfv2FindShortestPath(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem = NULL;
    Ospfv2Area* thisArea = NULL;

    // Invalidate present routing table and save it so that
    // changes in routing table entries can be identified.
    Ospfv2InvalidateRoutingTable(node);

    if (OSPFv2_DEBUG_TABLEErr)
    {
        Ospfv2PrintLSDB(node);
    }

    // Find Intra Area route for each attached area
    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        thisArea = (Ospfv2Area*) listItem->data;

        Ospfv2FindShortestPathForThisArea(node, thisArea->areaID);
    }

    // Calculate Inter Area routes
    if (ospf->partitionedIntoArea == TRUE)
    {
        Ospfv2FindInterAreaPath(node);
    }

    // BGP-OSPF Patch Start
    // NOTE: For CES purposes, this must occur at ALL
    // routers, whether they are sBoundaryRouters or not.

    if ((!ospf->nssaCandidateNode) || ospf->nssaAreaBorderRouter)
    {
        // As per RFC 1587, section 3.5 mentions that an NSSA border router
        // should also examine both Type 7 and Type 5 LSA's.
        // "An NSSA area border router should examine both type-5 LSAs and
        // type-7 LSAs if either type-5 or type-7 routes need to be updated.
        // Type-7 LSAs should be examined after type-5 LSAs. An NSSA internal
        // router should examine type-7 LSAs when type-7 routes need to be
        // recalculated."
        Ospfv2FindASExternalPath(node);
    }

    // BGP-OSPF Patch End

    //for NSSA
    if (ospf->nssaAreaBorderRouter
        || ospf->nssaCandidateNode)
    {
        Ospfv2FindNSSAExternalPath(node);
    }

/***** Start: ROSPF Redirect *****/
#ifdef ADDON_BOEINGFCS
    RoutingCesRospfUpdateWithRedirectRoutes(node);
#endif
/***** End: ROSPF Redirect *****/

    // Update IP forwarding table using new shortest path.
    Ospfv2UpdateIpForwardingTable(node);

    //Store the timestamp of becoming unreachable of an advertising router
    //from the information contained in IP forwarding table
    if (ospf->supportDC == TRUE)
    {
        Ospfv2StoreUnreachableTimeStampForAllLSA(node, ospf);
    }

    if ((ospf->partitionedIntoArea == TRUE)
        && (ospf->areaBorderRouter == TRUE
            || ospf->nssaAreaBorderRouter == TRUE))
    {
        Ospfv2HandleABRTask(node);
    }

    Ospfv2FreeInvalidRoute(node);

    // Update IP forwarding table using new shortest path.
    Ospfv2UpdateIpForwardingTable(node);

    if (Ospfv2DebugSPT(node))
    {
        Ospfv2PrintRoutingTable(node);
    }

    if (OSPFv2_DEBUG_TABLE)
    {
        Ospfv2PrintRoutingTable(node);
        Ospfv2PrintNeighborState(node);
    }

}


// BGP-OSPF Patch Start
//-------------------------------------------------------------------------//
// NAME: Ospfv2ExternalRouteInit
// PURPOSE: Inject "External Route" into Routing Table for AS Boundary Router
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2ExternalRouteInit(
    Node* node,
    int interfaceIndex,
    const NodeInput* ospfExternalRouteInput)
{
    int i;
    BOOL defaultOriginated = FALSE;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    if (ospf->defaultInfo.defaultInfoOriginate
                && (ospf->defaultInfo.always == TRUE))
    {
        defaultOriginated = TRUE;
    }

    for (i = 0; i < ospfExternalRouteInput->numLines; i++)
    {

        char destAddressString[MAX_STRING_LENGTH];
        char nextHopString[MAX_STRING_LENGTH];
        NodeAddress sourceAddress;
        int numParameters;
        int cost;

        numParameters = sscanf(ospfExternalRouteInput->inputStrings[i],
                               "%u %s %s %d",
                               &sourceAddress,
                               destAddressString,
                               nextHopString,
                               &cost);

        if ((numParameters < 3) || (numParameters > 4))
        {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage, "OspfExternalRouting: Wrong number "
                    "of addresses in external route entry.\n  %s\n",
                    ospfExternalRouteInput->inputStrings[i]);

            ERROR_ReportError(errorMessage);
        }

        if (sourceAddress == node->nodeId)
        {
            NodeAddress destAddress;
            int destNumHostBits;
            NodeAddress nextHopAddress;
            int nextHopHostBits;
            BOOL isNodeId;
            Ospfv2RoutingTableRow newRow;
            int intfId;
            clocktype delay;
            Message* newMsg = NULL;
            char* msgInfo = NULL;
            BOOL flush = FALSE;
            Ospfv2LinkStateType lsType;
            NodeAddress addrMask;

            IO_ParseNodeIdHostOrNetworkAddress(
                destAddressString,
                &destAddress,
                &destNumHostBits,
                &isNodeId);

            IO_ParseNodeIdHostOrNetworkAddress(
                nextHopString,
                &nextHopAddress,
                &nextHopHostBits,
                &isNodeId);

            if (nextHopHostBits != 0)
            {
                char errorMessage[MAX_STRING_LENGTH];

                sprintf(errorMessage, "OspfExternalRouting: Next hop "
                        "address can't be a subnet\n  %s\n",
                        ospfExternalRouteInput->inputStrings[i]);

                ERROR_ReportError(errorMessage);
            }

            if (numParameters == 3)
            {
                cost = 1;
            }
            // Add new route
            newRow.destType = OSPFv2_DESTINATION_NETWORK;
            newRow.destAddr = destAddress;
            newRow.addrMask = ConvertNumHostBitsToSubnetMask(
                                                    destNumHostBits);
            newRow.option = 0;
            newRow.areaId = 0;
            newRow.pathType = OSPFv2_TYPE2_EXTERNAL;
            newRow.metric = 1;
            newRow.type2Metric = cost;
            newRow.LSOrigin = NULL;
            newRow.advertisingRouter = ospf->routerID;

            intfId = NetworkIpGetInterfaceIndexForNextHop(
                                node,
                                nextHopAddress);

            newRow.outIntf =  intfId;

            NodeAddress outIntf = NetworkIpGetInterfaceAddress(node, intfId);
            if (outIntf == nextHopAddress)
            {
                nextHopAddress = 0;
            }
            newRow.nextHop = nextHopAddress;

            Ospfv2AddRoute(node, &newRow);

            //build AS-EXTERNAL LSA
            if ((destAddress == 0) &&
               (ospf->defaultInfo.defaultInfoOriginate == FALSE))
            {
                continue;
            }

            lsType = OSPFv2_ROUTER_AS_EXTERNAL;
            addrMask = ConvertNumHostBitsToSubnetMask(destNumHostBits);
            flush = FALSE;

            if (ospf->defaultInfo.defaultInfoOriginate)
            {
               if (ospf->defaultInfo.routeMap)
                {
                    BOOL rtMapPermit = FALSE; // Decision of route map
                    BOOL flag = FALSE;
                    RouteMap* routeMap;
                    RouteMapEntry* entry;
                    RouteMapValueSet* valueList = NULL;

                    routeMap = ospf->defaultInfo.routeMap;
                    entry = (RouteMapEntry*) routeMap->entryHead;

                    // Route map is enabled
                    // We init the value list
                    valueList = (RouteMapValueSet*)RouteMapInitValueList();

                    // Fill the match values of the valueList before
                    // calling the action function
                    valueList->matchCost = cost;
                    valueList->matchDestAddress = destAddress;
                    valueList->matchDestAddressMask = addrMask;
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
                    cost = valueList->setMetric;

                    // If work of valueList is over then we free it
                    MEM_free(valueList);

                    // 'match' clause has been satisfied by the
                    // Route Map so redistribribute the routes.
                    if (rtMapPermit == TRUE)
                    {
                        destAddress = 0;
                        addrMask = 0;
                    }
                }
                else if (destAddress == 0)
                {
                    destAddress = 0;
                    addrMask = 0;
                }
            }

            if (!(defaultOriginated && destAddress == 0))
            {
                newMsg = MESSAGE_Alloc(
                            node, NETWORK_LAYER, ROUTING_PROTOCOL_OSPFv2,
                            MSG_ROUTING_OspfScheduleLSDB);

                    if (destAddress == 0)
                    {
                       defaultOriginated = TRUE;
                    }
    #ifdef ADDON_NGCNMS
                MESSAGE_SetInstanceId(newMsg, interfaceIndex);
    #endif

                MESSAGE_InfoAlloc(
                    node, newMsg, sizeof(Ospfv2LinkStateType)
                    + (sizeof(NodeAddress) * 3) + sizeof(int)
                    + sizeof(BOOL));
                msgInfo = MESSAGE_ReturnInfo(newMsg);

                memcpy(msgInfo, &lsType, sizeof(Ospfv2LinkStateType));
                msgInfo += sizeof(Ospfv2LinkStateType);
                memcpy(msgInfo, &destAddress, sizeof(NodeAddress));
                msgInfo += sizeof(NodeAddress);
                memcpy(msgInfo, &addrMask, sizeof(NodeAddress));
                msgInfo += sizeof(NodeAddress);
                memcpy(msgInfo, &nextHopAddress, sizeof(NodeAddress));
                msgInfo += sizeof(NodeAddress);
                memcpy(msgInfo, &cost, sizeof(int));
                msgInfo += sizeof(int);
                memcpy(msgInfo, &flush, sizeof(BOOL));

                // Schedule for originate AS External LSA
                delay = (clocktype) (OSPFv2_AS_EXTERNAL_LSA_ORIGINATION_DELAY
                    + (RANDOM_erand(ospf->seed) * OSPFv2_BROADCAST_JITTER));

                MESSAGE_Send(node, newMsg, delay + ospf->staggerStartTime);
            }

            // Mtree Start
            if (ospf->nssaCandidateNode)
            {
                    // Rather than using defaultAddress, nextHopAddress,
                    // cost and addrMask
                    // we are using newRow variables during
                    // Ospfv2OriginateNSSAExternalLSA()
                    // function call because above variables can be modified
                    // during RouteMap functionality above.
                if (newRow.destAddr > 0)
                {
                    Ospfv2OriginateNSSAExternalLSA(
                        node,
                        newRow.destAddr,
                        newRow.addrMask,
                        newRow.nextHop,
                        newRow.type2Metric,
                        flush);
                }
                else
                {
                    if (ospf->nssaDefInfoOrg)
                    {
                        Ospfv2OriginateNSSAExternalLSA(
                            node,
                            newRow.destAddr,
                            newRow.addrMask,
                            newRow.nextHop,
                            newRow.type2Metric,
                            flush);
                    }
                }
            }
          // Mtree End
       }
    }//for//
}

#ifdef ADDON_MA
void Ospfv2SendAllLSAtoMA(Node* node, Ospfv2Data* ospf)
{
    Ospfv2ListItem* item = NULL;
    Ospfv2ListItem* listItem = NULL;
    int maRetVal;

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv2Area* thisArea = (Ospfv2Area*) item->data;

        //Send RouterLSA for thisArea
        listItem = thisArea->routerLSAList->first;
        while (listItem)
        {
            maRetVal = MAConstructSendAndRecieveCommunication
                ((char*) listItem->data,node);
            listItem = listItem->next;
        }

        //Send NetworkLSA for thisArea;
        listItem = thisArea->networkLSAList->first;
        while (listItem)
        {
            maRetVal = MAConstructSendAndRecieveCommunication
                ((char*) listItem->data,node);
            listItem = listItem->next;
        }

        //Send RouterSummaryLSA for thisArea;
        listItem = thisArea->routerSummaryLSAList->first;
        while (listItem)
        {
            maRetVal = MAConstructSendAndRecieveCommunication
                ((char*) listItem->data,node);
            listItem = listItem->next;
        }

        //Send NetworkSummaryLSA for thisArea;
        listItem = thisArea->networkSummaryLSAList->first;
        while (listItem)
        {
            maRetVal = MAConstructSendAndRecieveCommunication
                ((char*) listItem->data,node);
            listItem = listItem->next;
        }

    }

    //Send ExternalLSA for thisArea;
    listItem = ospf->asExternalLSAList->first;
     while (listItem)
    {
        maRetVal = MAConstructSendAndRecieveCommunication
            ((char*) listItem->data,node);
        listItem = listItem->next;
    }

}

int Ospfv2FindCostFromPolicy(Node* node, NodeAddress destAddr)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2ListItem* listItem = NULL;

    Ospfv2ExternalCostPolicy* costEntry;

    listItem =  ospf->costPolicyList->first;

    while (listItem)
    {
        costEntry = (Ospfv2ExternalCostPolicy*) listItem->data;
        if (costEntry->externalAddr == (destAddr & costEntry->mask))
        {
            return costEntry->cost ;
        }

        listItem = listItem->next;
    }

    return -1;

}


//-------------------------------------------------------------------------//
// NAME: Ospfv2ExternalCostPolicyInit
// PURPOSE:
// RETURN: None.
//-------------------------------------------------------------------------//

static
void Ospfv2ExternalCostPolicyInit(
    Node* node,
    int interfaceIndex,
    const NodeInput* costPolicyInput)
{
    int i;
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    for (i = 0; i < costPolicyInput->numLines; i++)
    {

        char destAddressString[MAX_STRING_LENGTH];
        NodeAddress sourceAddress;
        int numParameters;
        int cost;
        Ospfv2ExternalCostPolicy* externalCost;

        numParameters = sscanf(costPolicyInput->inputStrings[i],
                               "%u %s %d",
                               &sourceAddress,
                               destAddressString,
                               &cost);

        if ((numParameters < 2) || (numParameters > 3))
        {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage, "Ospfv2ExternalCostPolicyInit: "
                    "Wrong format of cost policy entry.\n  %s\n",
                    costPolicyInput->inputStrings[i]);

            ERROR_ReportError(errorMessage);
        }

        if (cost < 0)
        {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage, "Ospfv2ExternalCostPolicyInit: "
                "cost is less than 0 \n  %s\n",
                    costPolicyInput->inputStrings[i]);

            ERROR_ReportError(errorMessage);
        }

        if (sourceAddress == node->nodeId)
        {
            NodeAddress destAddress;
            int destNumHostBits;
            BOOL isNodeId;
            clocktype delay;
            //NodeAddress addrMask;

            IO_ParseNodeIdHostOrNetworkAddress(
                destAddressString,
                &destAddress,
                &destNumHostBits,
                &isNodeId);


            if (numParameters == 2)
            {
                cost = 1;
            }

            //Insert
            externalCost = (Ospfv2ExternalCostPolicy*)
                MEM_malloc(sizeof(Ospfv2ExternalCostPolicy));

            externalCost->externalAddr = destAddress;
            externalCost->cost = cost;
            externalCost->mask =
                            ConvertNumHostBitsToSubnetMask(destNumHostBits);

            Ospfv2InsertToList(ospf->costPolicyList,0, (void*) externalCost);

        }
    }// End of for (i = 0; i < costPolicyInput->numLines; i++)
}
#endif


#ifdef ADDON_NGCNMS
static
void Ospfv2ReInit(
    Node* node,
    const NodeInput* nodeInput,
    BOOL rospf,
    int interfaceIndex)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];
    NodeInput ospfConfigFile;
    NodeInput* inputFile;
    clocktype delay;
    Message* newMsg;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    // Router ID is chosen to be the IP address
    // associated with the first interface.
    // first interface may be reset...
    ospf->routerID = NetworkIpGetInterfaceAddress(node, 0);

    // Read config file
    IO_ReadCachedFile(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "OSPFv2-CONFIG-FILE",
        &retVal,
        &ospfConfigFile);

    if (!retVal)
    {
        if (ospf->partitionedIntoArea && !rospf)
        {
            ERROR_ReportError("The OSPFv2-CONFIG-FILE file is required "
                "to configure area\n");
        }

        inputFile = NULL;
    }
    else
    {
        inputFile = &ospfConfigFile;
    }

    Ospfv2ReInitInterface(node, inputFile, nodeInput, interfaceIndex, rospf);

    delay = (clocktype) (RANDOM_erand(ospf->seed) * OSPFv2_BROADCAST_JITTER);

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfScheduleHello);

    MESSAGE_SetInstanceId(newMsg, interfaceIndex);

    MESSAGE_Send(node, newMsg, delay + ospf->staggerStartTime);

    // Timer to age out LSAs in LSDB.
    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfIncrementLSAge);

    MESSAGE_SetInstanceId(newMsg, interfaceIndex);

    MESSAGE_Send(node,
                 newMsg,
                 (clocktype) ospf->incrementTime
                 + ospf->staggerStartTime);

    NetworkIpSetRouterFunction(node,
                               &Ospfv2RouterFunction,
                               interfaceIndex);

    // Set routing table update function for route redistribution
    RouteRedistributeSetRoutingTableUpdateFunction(
            node,
            &Ospfv2HookToRedistribute,
            interfaceIndex);

    // BGP-OSPF Patch Start
    if (ospf->asBoundaryRouter)
    {
        NodeInput ospfExternalRouteInput;

        // Determine whether the user inject External Route or not.
        // Format is:[node] OSPF-INJECT-ROUTE
        // Here specify only the Node NOT the Network.

        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OSPFv2-INJECT-EXTERNAL-ROUTE",
            &retVal,
            buf);

        if (retVal)
        {
            if (strcmp(buf, "YES") == 0)
            {
                IO_ReadCachedFile(
                    ANY_NODEID,
                    NetworkIpGetInterfaceAddress(node, interfaceIndex),
                    nodeInput,
                    "OSPFv2-INJECT-ROUTE-FILE",
                    &retVal,
                    &ospfExternalRouteInput);

                Ospfv2ExternalRouteInit(node, interfaceIndex,
                    &ospfExternalRouteInput);

                if (!retVal)
                {
                    ERROR_ReportError("The OSPFv2-INJECT-ROUTE-FILE "
                                      "file is not specified\n");
                }
            }
        }
    }
    // BGP-OSPF Patch End

}
#endif

//-------------------------------------------------------------------------//
// NAME: Ospfv2InitSummaryAddress
// PURPOSE: To read the ADVERTISE and NOT-ADVERTISE addresses from input file
// RETURN: NONE.
//-------------------------------------------------------------------------//

static
void Ospfv2InitSummaryAddress(
    Node* node,
    Ospfv2Data* ospf,
    const NodeInput* ospfConfigFile)
{
    if (!ospfConfigFile)
    {
        return;
    }
    int i;
    int qualifierLen;

    char* qualifier       = new char[ospfConfigFile->maxLineLen];
    char* parameterName   = new char[ospfConfigFile->maxLineLen];
    char* addressString   = new char[ospfConfigFile->maxLineLen];
    char* maskString      = new char[ospfConfigFile->maxLineLen];
    char* advertiseString = new char[ospfConfigFile->maxLineLen];
    char* areaIdString    = new char[ospfConfigFile->maxLineLen];



    ospf->defaultInfo.defaultInfoOriginate = FALSE;
    ospf->defaultInfo.always = FALSE;
    ospf->defaultInfo.metricValue = 1;
    ospf->defaultInfo.metricType = 2;
    ospf->defaultInfo.routeMap = NULL;
    // Mtree Start
    ospf->nssaDefInfoOrg = FALSE;
    // Mtree End

    for (i = 0; i < ospfConfigFile->numLines; i++)
     {
        char* currentLine = ospfConfigFile->inputStrings[i];
        char* aStr = NULL;

        // Skip lines that are not begining with "["
        if (strncmp(currentLine, "[", 1) != 0)
        {
            continue;
        }

        aStr = strchr(currentLine, ']');
        if (aStr == NULL)
        {
            char* errStr = new char[strlen(currentLine) + MAX_STRING_LENGTH];
            sprintf(errStr, "Unterminated qualifier:\n %s\n", currentLine);
            ERROR_ReportError(errStr);
        }

        qualifierLen = (int)(aStr - currentLine - 1);
        strncpy(qualifier, &currentLine[1], qualifierLen);
        qualifier[qualifierLen] = '\0';

        aStr++;
        if (!isspace(*aStr))
        {
            char* errStr = new char[strlen(currentLine) + MAX_STRING_LENGTH];
            sprintf(errStr, "White space required after qualifier:\n %s\n",
                             currentLine);
            ERROR_ReportError(errStr);
        }
        // skip white space
        while (isspace(*aStr))
        {
            aStr++;
        }

       if ((unsigned)atoi(qualifier) == node->nodeId)
       {
          sscanf(aStr, "%s", parameterName);

            if (strcmp(parameterName, "SUMMARY-ADDRESS") == 0)
            {
                NSSASummaryAddress summaryAddress;
                NodeAddress networkAddr;

            // Mtree start - 25/06/09
                if (sscanf(aStr, "%s %s %s %s", parameterName,
                    addressString, maskString, advertiseString) != 4)
                {
                    ERROR_ReportError("OSPFv2: The format for SUMMARY-"
                                 "ADDRESS configuration is "
                                 "[<node-Id>] SUMMARY-ADDRESS "
                                 " [<subnet-address>] [<mask>] \n");
                }
          // Mtree end - 25/06/09
                 IO_ConvertStringToNodeAddress(addressString, &networkAddr);
                 summaryAddress.address = networkAddr;

                 IO_ConvertStringToNodeAddress(maskString, &networkAddr);
                 summaryAddress.mask = networkAddr;

                 //below values are init here. They are used for Advertise
                 //case only during Type 7 to Type 5 conversion.
                 summaryAddress.LSA = NULL;
                 summaryAddress.pathType = OSPFv2_TYPE1_EXTERNAL;
                 summaryAddress.wasExactMatched = FALSE;

                 if (strcmp(advertiseString, "ADVERTISE") == 0)
                 {
                    ospf->nssaAdvertiseList.push_back(summaryAddress);
                 }
                 else if (strcmp(advertiseString, "NOT-ADVERTISE") == 0)
                 {
                    ospf->nssaNotAdvertiseList.push_back(summaryAddress);
                 }
                 else
                 {
#ifdef JNE_LIB
                     JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                         "Not a valid SUMMARY-ADDRESS"
                         " status indictaion type",
                         JNE::CRITICAL);
#endif
                     ERROR_Assert(FALSE, " Not a valid SUMMARY-ADDRESS"
                                         " status indictaion type\n");
                 }
            }
      // Mtree start - 25/06/09
            else if (strcmp(parameterName, "NETWORK-SUMMARY-LSA") == 0)
            {
                NetworkSummaryLSA networkSummaryLSA;
                NodeAddress networkAddr;
                BOOL isNodeId;
                unsigned int areaId;

                if (sscanf(aStr, "%s %s %s %s %s", parameterName,
                       addressString, maskString, advertiseString,
                       areaIdString) != 5)
                {
                    ERROR_ReportError("OSPFv2: The format for network "
                                 "summary LSA configuration is "
                                 "[<node-Id>] NETWORK-SUMMARY-LSA "
                                 " [<subnet-address>] [<mask>] \n");
                }

                IO_ConvertStringToNodeAddress(addressString, &networkAddr);
                networkSummaryLSA.address = networkAddr;

                IO_ConvertStringToNodeAddress(maskString, &networkAddr);
                networkSummaryLSA.mask = networkAddr;

                IO_ParseNodeIdOrHostAddress(
                        areaIdString,
                        &areaId,
                        &isNodeId);
                if (isNodeId)
                {
                    ERROR_ReportError("OSPFv2: The format for network "
                                 "summary LSA configuration is "
                                 "[<node-Id>] NETWORK-SUMMARY-LSA "
                                 " [<subnet-address>] [<mask>] \n");
                }
                networkSummaryLSA.areaId = areaId;

                Ospfv2Area* thisArea = NULL;
                thisArea = Ospfv2GetArea(node, areaId);

                if (!thisArea)
                {
                    char* warning = new char[MAX_STRING_LENGTH +
                                             ospfConfigFile->maxLineLen];
                    sprintf(warning, "Node %u is not a member of "
                           "Area %s", node->nodeId, areaIdString);
                    ERROR_ReportWarning(warning);
                    delete[] warning;
                }
                else
                {
                    if ((thisArea->externalRoutingCapability) &&
                        (thisArea->isNSSAEnabled == FALSE))
                    {
                        char* warning = new char[MAX_STRING_LENGTH +
                                                 ospfConfigFile->maxLineLen];
                        sprintf(warning, "OSPFv2: Area %s is neither"
                              " a stub nor a NSSA area", areaIdString);
                        ERROR_ReportWarning(warning);
                        delete[] warning;
                    }
                    else
                    {
                        if (strcmp(advertiseString, "ADVERTISE") == 0)
                        {
                            ospf->networkSummaryLSAAdvertiseList.push_back
                                                      (networkSummaryLSA);
                        }
                        else if (strcmp(advertiseString,
                                            "NOT-ADVERTISE") == 0)
                        {
                            ospf->networkSummaryLSANotAdvertiseList.
                                              push_back(networkSummaryLSA);
                        }
                        else
                        {
#ifdef JNE_LIB
                            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                                "OSPFv2: Not a valid"
                                " NETWORK-SUMMARY-LSA status indictaion"
                                " type",
                                JNE::CRITICAL);
#endif
                            ERROR_Assert(FALSE, "OSPFv2: Not a valid"
                                 " NETWORK-SUMMARY-LSA status indictaion"
                                 " type\n");
                        }
                    }
                }
            }
      // Mtree end - 25/06/09
            else if (strcmp(parameterName, "DEFAULT-INFORMATION") == 0)
            {
                char* token = new char[ospfConfigFile->maxLineLen];
                char* originate;
                char* always;
                char* routeMap;
                char* rest;
                char* metric;
                char* metricType;

                aStr = aStr + strlen(parameterName) + 1;
                originate = IO_GetToken(token, aStr, &rest);
                if (strcmp(originate, "ORIGINATE") == 0)
                {
                    ospf->defaultInfo.defaultInfoOriginate = TRUE;
                    aStr = rest;

                    always = IO_GetToken(token, aStr, &rest);
                    if (always && strcmp(always, "ALWAYS") == 0)
                    {
                        ospf->defaultInfo.always = TRUE;
                        aStr = rest;
                     }

                    metric = IO_GetToken(token, aStr, &rest);
                    if (metric && strcmp(metric, "METRIC") == 0)
                    {
                       sscanf(rest, "%s", metric);
                       ospf->defaultInfo.metricValue = atoi(metric);
                       aStr = rest + strlen(metric) + 1;
                    }

                    metricType = IO_GetToken(token, aStr, &rest);
                    if (metricType && strcmp(metric, "METRIC-TYPE") == 0)
                    {
                       sscanf(rest, "%s", metricType);
                       ospf->defaultInfo.metricType = atoi(metricType);
                       aStr = rest + strlen(metricType) + 1;
                     }

                    routeMap = IO_GetToken(token, aStr, &rest);
                    if (routeMap && strcmp(routeMap, "ROUTE-MAP") == 0)
                    {
                        sscanf(rest, "%s", routeMap);
                        ospf->defaultInfo.routeMap = RouteMapAddHook(node,
                            routeMap);
                    }
                }
                else
                {
                    ospf->defaultInfo.defaultInfoOriginate = FALSE;
                    ospf->defaultInfo.routeMap = NULL;
                }
                delete[] token;
            }
            // Mtree Start
            else if (strcmp(parameterName, "NSSA") == 0)
            {
                char* defaultInfoString;
                defaultInfoString = new char[MAX_STRING_LENGTH +
                                             ospfConfigFile->maxLineLen];
                sscanf(aStr, "%s %s" , parameterName, defaultInfoString);
                if (strcmp(defaultInfoString,
                                    "DEFAULT-INFORMATION-ORIGINATE") == 0)
                {
                   ospf->nssaDefInfoOrg = TRUE;
                }
                delete[] defaultInfoString;
            }
          // Mtree End
        }
    }
    delete[] qualifier;
    delete[] parameterName;
    delete[] addressString;
    delete[] maskString;
    delete[] advertiseString;
    delete[] areaIdString;
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2Init
// PURPOSE: Initialize this routing protocol
// RETURN: NONE.
//-------------------------------------------------------------------------//

void Ospfv2Init(
    Node* node,
    Ospfv2Data** ospfLayerPtr,
    const NodeInput* nodeInput,
    BOOL rospf,
#ifdef ADDON_NGCNMS
    BOOL ifaceReset,
#endif
    int interfaceIndex)
{

#ifdef DISTRIBUTED_OPT
    if (node->partitionId != node->partitionData->partitionId)
    {
        return;
    }
#endif

    Message* newMsg;
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];
    NodeInput ospfConfigFile;
    NodeInput* inputFile;

    clocktype delay;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

#ifdef ADDON_NGCNMS
    if (ospf != NULL)
    {
#ifdef JNE_LIB
        if (!ifaceReset)
        {
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "ospfv2 data structure already allocated"
                " and not an interface reset!!",
                JNE::CRITICAL);
        }
#endif
        ERROR_Assert(ifaceReset, "ospfv2 data structure already allocated"
                                 " and not an interface reset!!\n");
        // must be an interface reset.

        (*ospfLayerPtr) = ospf;

        Ospfv2ReInit(node,
                     nodeInput,
                     rospf,
                     interfaceIndex);

        return;

    }
#endif

#ifdef JNE_LIB
    if (ospf != NULL)
    {
        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
            "Problem with OSPFv2 Initialization",
            JNE::CRITICAL);
    }
#endif
    ERROR_Assert(ospf == NULL, "Problem with OSPFv2 Initialization\n");
    ospf = (Ospfv2Data*) MEM_malloc (sizeof (Ospfv2Data));

    // BGP-OSPF Patch Start
    int asVal = 0;
    // BGP-OSPF Patch End

    memset(ospf, 0, sizeof (Ospfv2Data));

    (*ospfLayerPtr) = ospf;

    RANDOM_SetSeed(ospf->seed,
                   node->globalSeed,
                   node->nodeId,
                   ROUTING_PROTOCOL_OSPFv2,
                   interfaceIndex);

    Ospfv2InitTrace(node, nodeInput);

    // BGP-OSPF Patch Start
    // Read the Node's Autonomous System id.
    // Format is: [node] AS-NUMBER <AS ID>
    IO_ReadInt(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "AS-NUMBER",
        &retVal,
        &asVal);

    if (retVal)
    {
        if (asVal > OSPFv2_MAX_AS_NUMBER || asVal < OSPFv2_MIN_AS_NUMBER)
        {
            ERROR_ReportError("Autonomous System id must be less than "
                "equal to 65535 and greater than 0");
        }
        ospf->asID = asVal;
    }
    else
    {
        ospf->asID = 1;
    }

    // j.o. - runtime optimizations that shouldnt effect network
    // performance much.

    ospf->spfCalcDelay = OSPFv2_BROADCAST_JITTER;
    IO_ReadTime(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "OSPFv2-SPF-CALCULATION-DELAY",
        &retVal,
        &ospf->spfCalcDelay);

    ospf->floodTimer = OSPFv2_FLOOD_TIMER;
    IO_ReadTime(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "OSPFv2-LSA-FLOOD-DELAY",
        &retVal,
        &ospf->floodTimer);

    ospf->incrementTime = OSPFv2_LSA_INCREMENT_AGE_INTERVAL;
    IO_ReadTime(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "OSPFv2-LSA-AGE-INCREMENT-INTERVAL",
        &retVal,
        &ospf->incrementTime);

    if (ospf->incrementTime < SECOND)
    {
        ERROR_ReportError("OSPFv2-LSA-AGE-INCREMENT-INTERVAL "
                          "must be greater than or equal to 1 second.\n");
    }

    // end of runtime optimizations

#ifdef ADDON_BOEINGFCS
    ospf->maxNumRetx = OSPFv2_MAX_NUM_RETX;
    IO_ReadInt(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "OSPFv2-MAX-NUM-RETX",
        &retVal,
        &ospf->maxNumRetx);
#endif

    IO_ReadString(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "OSPFv2-ADVRT-SELF-INTF",
        &retVal,
        buf);

    if (!retVal || !strcmp (buf, "NO"))
    {
        ospf->isAdvertSelfIntf = FALSE;
    }
    else if (!strcmp (buf, "YES"))
    {
        ospf->isAdvertSelfIntf = TRUE;
    }
    else
    {
        ERROR_ReportError(
          "OSPFv2-ADVRT-SELF-INTF: Unknown value in configuration file.\n");
    }

    // Determine whether the node is AS BOUNDARY ROUTER or not.
    // Format is: [node] AS-BOUNDARY-ROUTER <YES/NO>
    // If not specified, node is not a AS BOUNDARY ROUTER.
    // Here specify only the Node NOT the Network.
    IO_ReadString(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "AS-BOUNDARY-ROUTER",
        &retVal,
        buf);

    if (!retVal || !strcmp (buf, "NO"))
    {
        ospf->asBoundaryRouter = FALSE;
    }
    else if (!strcmp (buf, "YES"))
    {
        ospf->asBoundaryRouter = TRUE;
    }
    else
    {
        ERROR_ReportError(
            "AS-BOUNDARY-ROUTER: Unknown value in configuration file.\n");
    }

    Ospfv2InitList(&ospf->asExternalLSAList);
    // BGP-OSPF Patch End

    /***** Start: OPAQUE-LSA *****/
    Ospfv2InitList(&(ospf->ASOpaqueLSAList));
    ospf->ASOpaqueLSTimer = FALSE;
    ospf->ASOpaqueLSAOriginateTime = (clocktype)0;

    BOOL configureOpaqueCapVal;
    ospf->opaqueCapable = TRUE;

    IO_ReadBool(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "OSPFv2-OPAQUE-LSA-CAPABLE",
            &retVal,
            &configureOpaqueCapVal);

    if (retVal)
    {
        ospf->opaqueCapable = configureOpaqueCapVal;
    }

    /***** End: OPAQUE-LSA *****/

    Ospfv2InitList(&ospf->nssaExternalLSAList);

    ospf->maxAgeLSARemovalTimerSet = FALSE;

    // Determine whether or not to print the stats at the end of simulation.
    IO_ReadString(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "ROUTING-STATISTICS",
        &retVal,
        buf);

    if (!retVal || !strcmp (buf, "NO"))
    {
        ospf->collectStat = FALSE;
    }
    else if (!strcmp (buf, "YES"))
    {
        ospf->collectStat = TRUE;
    }
    else
    {
        ERROR_ReportError(
            "ROUTING-STATISTICS: Unknown value in configuration file.\n");
    }

    // Used to print statistics only once during finalization.  In
    // the future, stats should be tied to each interface.
    ospf->stats.alreadyPrinted = FALSE;

    ospf->isMospfEnable = FALSE;

    // Determine whether the domain is partitioned into several areas.
    IO_ReadString(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "OSPFv2-DEFINE-AREA",
        &retVal,
        buf);

    if (!retVal || !strcmp (buf, "NO"))
    {
        ospf->partitionedIntoArea = FALSE;
        Ospfv2InitList(&ospf->area);
        ospf->backboneArea = NULL;
    }
    else if (!strcmp (buf, "YES"))
    {
        ospf->partitionedIntoArea = TRUE;
        Ospfv2InitList(&ospf->area);
        ospf->backboneArea = NULL;
    }
    else
    {
        ERROR_ReportError(
            "OSPFv2-DEFINE-AREA: Unknown value in configuration file.\n");
    }

#ifdef ADDON_BOEINGFCS
    ospf->displayRospfIcons = FALSE;

    IO_ReadBool(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "DISPLAY-ROSPF-ICONS",
        &retVal,
        &ospf->displayRospfIcons);
#endif
    // Read config file
    IO_ReadCachedFile(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "OSPFv2-CONFIG-FILE",
        &retVal,
        &ospfConfigFile);

    if (!retVal)
    {
#ifndef ADDON_BOEINGFCS
        if (ospf->partitionedIntoArea)
        {
            ERROR_ReportError("The OSPFv2-CONFIG-FILE file is required "
                "to configure area\n");
        }
#else
        if (ospf->partitionedIntoArea && !rospf)
        {
            ERROR_ReportError("The OSPFv2-CONFIG-FILE file is required "
                "to configure area\n");
        }
#endif

        inputFile = NULL;
    }
    else
    {
        inputFile = &ospfConfigFile;
    }

    // If we need to desynchronize routers start up time.
    //IO_ReadString(
    //    node->nodeId,
    //    NetworkIpGetInterfaceAddress(node, interfaceIndex),
    //    nodeInput,
    //    "OSPFv2-STAGGER-START",
    //    &retVal,
    //    buf);

    //if (!retVal || !strcmp (buf, "NO"))
    //{
    //    ospf->staggerStartTime = (clocktype) 0;
    //}
    //else if (!strcmp (buf, "YES"))
    //{
        //clocktype maxDelay = TIME_ReturnMaxSimClock(node);

        //if (maxDelay > OSPFv2_LS_REFRESH_TIME)
        //{
        //    maxDelay = OSPFv2_LS_REFRESH_TIME;
        //}

    clocktype maxDelay;

    IO_ReadTime(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "OSPFv2-STAGGER-START-TIME",
        &retVal,
        &maxDelay);

    if (retVal)
    {
        ospf->staggerStartTime = (clocktype)
                    ((RANDOM_erand(ospf->seed) * OSPFv2_BROADCAST_JITTER)
                    + maxDelay);
    }
    else
    {
        ospf->staggerStartTime = (clocktype) 0;
        //    (RANDOM_erand(ospf->seed) * maxDelay);
        maxDelay = (clocktype) 0;
    }
    //}
    //else
    //{
    //    ERROR_ReportError(
    //        "OSPFv2-STAGGER-START: Unknown value in configuration file.\n");
    //}

    // Initialize IP forwarding table.
    NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OSPFv2);
    Ospfv2InitRoutingTable(node);

    if (OSPFv2_DEBUG_ERRORS)
    {
        NetworkPrintForwardingTable(node);
    }

    // All OSPF Routers should join group AllSPFRouters
    NetworkIpAddToMulticastGroupList(node, OSPFv2_ALL_SPF_ROUTERS);

#ifdef CYBER_CORE
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    if (ip->isIgmpEnable == TRUE)
    {
        if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
        {
            IgmpJoinGroup(node, IAHEPGetIAHEPRedInterfaceIndex(node),
                                OSPFv2_ALL_SPF_ROUTERS, (clocktype)0);
            if (IAHEP_DEBUG)
            {
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(OSPFv2_ALL_SPF_ROUTERS, addrStr);
                printf("\nRed Node: %d", node->nodeId);
                printf("\tJoins Group: %s\n", addrStr);
            }

            IgmpJoinGroup(node, IAHEPGetIAHEPRedInterfaceIndex(node),
                                OSPFv2_ALL_D_ROUTERS, (clocktype)0);
            if (IAHEP_DEBUG)
            {
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(OSPFv2_ALL_D_ROUTERS, addrStr);
                printf("\nRed Node: %d", node->nodeId);
                printf("\tJoins Group: %s\n", addrStr);
            }

        }
    }
#endif

    ospf->neighborCount = 0;
    ospf->SPFTimer = (clocktype) 0;

    // Router ID is chosen to be the IP address
    // associated with the first interface.
    ospf->routerID = GetDefaultIPv4InterfaceAddress(node);
    ospf->supportDC = FALSE;

    IO_ReadString(node->nodeId,
                  NetworkIpGetInterfaceAddress(node, interfaceIndex),
                  nodeInput,
                  "OSPFv2-DEMAND-CIRCUIT-EXTENSION-ENABLED",
                  &retVal,
                  buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            ospf->supportDC = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            ospf->supportDC = FALSE;
        }
        else
        {
            char errStr[MAX_STRING_LENGTH];

            sprintf(errStr, "OSPF Init : "
                   "Unknown parameter value for "
                   "OSPFv2-DEMAND-CIRCUIT-EXTENSION-ENABLED");

            ERROR_ReportError(errStr);
        }
    }

    if (inputFile)
    {
       Ospfv2InitHostRoutes(node, inputFile);
    }

    Ospfv2InitInterface(node, inputFile, nodeInput, rospf);

    int i;
    clocktype staggerCheck;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, i),
            nodeInput,
            "OSPFv2-STAGGER-START-TIME",
            &retVal,
            &staggerCheck);

        if ((ospf->iface[i].type != OSPFv2_NON_OSPF_INTERFACE) &&
            ((!retVal && maxDelay != 0) ||
            (retVal && maxDelay != staggerCheck)))
        {
            char warning[256];
            sprintf(warning,
                    "OSPFv2-STAGGER-START-TIME for interfaces %d and %d of "
                    "node %d do not match",
                    interfaceIndex, i, node->nodeId);
            ERROR_ReportWarning(warning);
        }
    }

    Ospfv2InitSummaryAddress(node, ospf, inputFile);

    // Q-OSPF Patch Start
    // Determine whether or not Qos is enabled in simulation.
    // If Qos is not enabled, so no queue will be advertised for Qos.
    // If it was enabled, then this initialization will be done
    // by Q-OSPF.
    IO_ReadString(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "QUALITY-OF-SERVICE",
        &retVal,
        buf);

    if (!retVal || !strcmp (buf, "NONE"))
    {
        ospf->isQosEnabled = FALSE;
        ospf->numQueueAdvertisedForQos = 0;
    }
    else
    {
        ospf->isQosEnabled = TRUE;

        // Check, whether it is Q-OSPF
        if (!strcmp(buf, "Q-OSPF"))
        {
            // As Qos is enabled, we should pass the ospf->qosRoutingProtocol
            // pointer to point the Qospf Layer structure.
            QospfInit(
                node, (QospfData**) &ospf->qosRoutingProtocol, nodeInput);
            ospf->numQueueAdvertisedForQos = ((QospfData*)
                ospf->qosRoutingProtocol)->maxQueueInEachInterface;
        }
        else
        {
            ERROR_ReportError("\nInvalid QUALITY-OF-SERVICE parameter");
        }
    }
    // Q-OSPF Patch End

    // Schedule for sending Hello packets
    delay = (clocktype) (RANDOM_erand(ospf->seed) * OSPFv2_BROADCAST_JITTER);
    for (int index = 0; index < node->numberInterfaces; index++)
    {
         if (NetworkIpGetInterfaceType(node, index) == NETWORK_IPV6)
        {
            continue;
        }
        if (ospf->iface[index].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            continue;
        }
        newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfScheduleHello);

        MESSAGE_SetInstanceId(newMsg, (short)index);

        MESSAGE_Send(node, newMsg, delay + ospf->staggerStartTime);
    }

    // Timer to age out LSAs in LSDB.
    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfIncrementLSAge);

#ifdef ADDON_NGCNMS
    MESSAGE_SetInstanceId(newMsg, interfaceIndex);
#endif

    MESSAGE_Send(node,
                 newMsg,
                 (clocktype) ospf->incrementTime
                 + ospf->staggerStartTime);

    NetworkIpSetRouterFunction(node,
                               &Ospfv2RouterFunction,
                               interfaceIndex);

    // Set routing table update function for route redistribution
    RouteRedistributeSetRoutingTableUpdateFunction(
        node,
        &Ospfv2HookToRedistribute,
        interfaceIndex);

    // Initialized the stat variables.
    memset(&ospf->stats, 0, sizeof(Ospfv2Stats));

    if (OSPFv2_DEBUG_ERRORS)
    {
        int i;
        char netAddrStr[MAX_ADDRESS_STRING_LENGTH];
        char subnetAddrStr[MAX_ADDRESS_STRING_LENGTH];
        char ipAddrStr[MAX_ADDRESS_STRING_LENGTH];

        printf("Node %u network address info\n", node->nodeId);

        for (i = 0; i < node->numberInterfaces; i++)
        {
            printf("    interface = %d\n", i);

            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceNetworkAddress(node, i),
                netAddrStr);
            printf("    network address = %s\n", netAddrStr);

            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceSubnetMask(node, i),
                subnetAddrStr);
            printf("    subnet mask = %s\n", subnetAddrStr);

            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceAddress(node, i),
                ipAddrStr);
            printf("    ip address = %s\n\n", ipAddrStr);
        }
    }

    // BGP-OSPF Patch Start
    if (ospf->asBoundaryRouter)
    {
        NodeInput ospfExternalRouteInput;

        // Determine whether the user inject External Route or not.
        // Format is:[node] OSPF-INJECT-ROUTE
        // Here specify only the Node NOT the Network.

        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OSPFv2-INJECT-EXTERNAL-ROUTE",
            &retVal,
            buf);

        if (retVal)
        {
            if (strcmp(buf, "YES") == 0)
            {
                IO_ReadCachedFile(
                    node->nodeId,
                    NetworkIpGetInterfaceAddress(node, interfaceIndex),
                    nodeInput,
                    "OSPFv2-INJECT-ROUTE-FILE",
                    &retVal,
                    &ospfExternalRouteInput);

                    if (!retVal)
                    {
                      ERROR_ReportError("The OSPFv2-INJECT-ROUTE-FILE "
                                      "file is not specified\n");
                    }

                    // Mtree Start
                    Ospfv2ExternalRouteInit(node,
                                            interfaceIndex,
                                            &ospfExternalRouteInput);
                    // Mtree End
                 }
            }
            if (ospf->defaultInfo.defaultInfoOriginate
                && ospf->defaultInfo.always)
            {
                  Ospfv2OriginateASExternalLSA(
                        node,
                        OSPFv2_DEFAULT_DESTINATION,
                        OSPFv2_DEFAULT_MASK,
                        ospf->iface[interfaceIndex].address,
                        1,
                        FALSE);
            }
    }


    // Mtree Start
    if (ospf->nssaAreaBorderRouter)
    // Mtree End
    {
        Ospfv2Area* thisArea = NULL;

        for (int k = 0;k < node->numberInterfaces; k++)
        {
            thisArea = Ospfv2GetArea(node,
                                  ospf->iface[k].areaId);

            if (thisArea
                && thisArea->isNSSAEnabled
                && ospf->nssaDefInfoOrg)
            {
                Ospfv2OriginateNSSAExternalLSA(
                        node,
                        OSPFv2_DEFAULT_DESTINATION,
                        OSPFv2_DEFAULT_MASK,
                        ospf->iface[k].address,
                        1,
                        FALSE);
            }
        }
    }
    // BGP-OSPF Patch End

    // registering RoutingOspfv2HandleAddressChangeEvent function
    NetworkIpAddAddressChangedHandlerFunction(node,
                            &RoutingOspfv2HandleChangeAddressEvent);

#ifdef ADDON_MA
    clocktype startTime = 0;
    IO_ReadTime(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "OSPFv2-MA-START-TIME",
        &retVal,
        &startTime);

    if (!retVal)
    {
        ospf->sendLSAtoMA = TRUE;
    }else
    {
       if (startTime <0)
       {
           ERROR_ReportError("OSPFv2-MA-START-TIME should not be negative");
       }

        newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_OspfStartTimeMA);

        MESSAGE_SetInstanceId(newMsg, interfaceIndex);

        MESSAGE_Send(node, newMsg, startTime);
    }

    ospf->maMode = FALSE;
    IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "MA-INTERFACE",
            &retVal,
            buf);
    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            ospf->maMode = TRUE;
        }
    }

    Ospfv2InitList(&ospf->costPolicyList);
    if (ospf->asBoundaryRouter)
    {
        NodeInput costPolicyInput;

        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OSPFv2-EXTERNAL-COST-POLICY",
            &retVal,
            buf);

        if (retVal)
        {
            if (strcmp(buf, "YES") == 0)
            {
                IO_ReadCachedFile(
                    node->nodeId,
                    NetworkIpGetInterfaceAddress(node, interfaceIndex),
                    nodeInput,
                    "OSPFv2-EXTERNAL-COST-POLICY-FILE",
                    &retVal,
                    &costPolicyInput);

                if (!retVal)
                {
                    ERROR_ReportError("The OSPFv2-EXTERNAL-COST-POLICY-FILE "
                                      "file is not specified\n");
                }

                Ospfv2ExternalCostPolicyInit(node, interfaceIndex,
                    &costPolicyInput);
            }

        }
    }

#endif

}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleRoutingProtocolPacket
// PURPOSE      :Process OSPF-specific packets.
// ASSUMPTION   :None
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

void Ospfv2HandleRoutingProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddr,
    NodeAddress destinationAddress,
    NodeId sourceId,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

#ifdef CYBER_CORE

    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE
            && ip->interfaceInfo[interfaceIndex]->isVirtualInterface
            && ospf->iface[interfaceIndex].type == OSPFv2_NON_OSPF_INTERFACE)
    {
        interfaceIndex = IAHEPGetIAHEPRedInterfaceIndex(node);
    }
#endif // CYBER_CORE

    Ospfv2Interface* thisIntf = NULL;
    Ospfv2CommonHeader* ospfHeader = NULL;

    // Make sure that ospf is running on the incoming interface.
    // If ospf is not running on the incoming interface,
    // then drop this packet.

    if ((NetworkIpGetUnicastRoutingProtocolType(node, interfaceIndex) !=
        ROUTING_PROTOCOL_OSPFv2)
        || (node->getNodeTime() < ospf->staggerStartTime))
    {
        //Trace drop
        ActionData acnData;
        acnData.actionType = DROP;
        acnData.actionComment = DROP_PROTOCOL_UNAVAILABLE_AT_INTERFACE;
        TRACE_PrintTrace(node,
                         msg,
                         TRACE_NETWORK_LAYER,
                         PACKET_IN,
                         &acnData);

        MESSAGE_Free(node, msg);
        return;
    }

    // Locally originated packets should not be passed on to OSPF.
    if (Ospfv2IsMyAddress(node, sourceAddr))
    {
        MESSAGE_Free(node, msg);
        return;
    }

    thisIntf = &ospf->iface[interfaceIndex];

    // Packets with destination to AllDRouters and receiving interface
    // state not as Designated or Back up Designated should be discarded
    if ((destinationAddress == OSPFv2_ALL_D_ROUTERS)
        && (thisIntf->state < S_Backup))
    {
       MESSAGE_Free(node, msg);
       return;
    }

#ifdef ADDON_BOEINGFCS
    if (thisIntf->type == OSPFv2_ROSPF_INTERFACE)
    {
        if (msg->headerProtocols[msg->numberOfHeaders-1] ==
            TRACE_ROUTING_CES_ROSPF)
        {
            Ospfv2HandleRoutingCesRospfProtocolPacket(node, msg, sourceAddr,
                interfaceIndex);
            return;
        }

        if (! RoutingCesRospfAdjCheck(node,
                                      interfaceIndex,
                                      sourceAddr,
                                      msg))
        {
            // If RospfAdjCheck returns false, this means the packet was
            // a special ROSPF notification packet.  RospfAdjCheck has
            // done all the processing needed on the packet, and OSPF
            // should not process the packet further (as it is actually a
            // Database Description packet with flags which are invalid for
            // standard OSPFv2).
            if (Ospfv2DebugSync(node))
            {
                printf("Node %d intf %d dropping ROSPF notification packet\n",
                       node->nodeId, interfaceIndex);
            }
            MESSAGE_Free(node, msg);
            return;
        }
    }
#endif
    // Get OSPF header
    ospfHeader = (Ospfv2CommonHeader*) MESSAGE_ReturnPacket(msg);

    // Area ID specified in the header matches the
    // area ID of receiving interface
    if (ospfHeader->areaID == thisIntf->areaId)
    {
        // Make sure the packet has been sent over a single hop
        // According to rfc-2328 section 8.2 bullet 6 point (1)
        // the comparison should not be performed on point to point network.
        // but on wireless ad-hoc network (point-to-multipoint),
        // nodes may have different interface addresses. so the comparison
        // should be made only for broadcast networks.
        if (thisIntf->type == OSPFv2_BROADCAST_INTERFACE &&
            //added for virtual link support
            (sourceAddr & thisIntf->subnetMask) !=
            (NetworkIpGetInterfaceNetworkAddress(node, interfaceIndex)))
        {
            // NOTE:
            // Although this checking should not be performed over
            // Point-to-Point networks as the interface address of each
            // end of the link may assigned independently. However,
            // current QUALNET assign a network number on link, so we can
            // ignore this restriction unless QUALNET being capable of
            //doing so.
#ifndef ADDON_BOEINGFCS
            MESSAGE_Free(node, msg);
            return;
#endif
        }
    }

    // Area ID in packet indicate backbone
    else if (ospfHeader->areaID == 0)
    {
        // Packet has been sent over virtual link
        //TBD: Receiving router must be an ABR. Source must be
        //     other end of virtual link.
    }
    else
    {
        // Should be discarded as it comes from other area
        MESSAGE_Free(node, msg);
        return;
    }

    // trace recd pkt
    ActionData acnData;
    acnData.actionType = RECV;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_NETWORK_LAYER, PACKET_IN, &acnData);


    if (OSPFv2_DEBUG_PACKET)
    {
        char clockStr[100];
        TIME_PrintClockInSecond(node->getNodeTime() +
                                    getSimStartTime(node), clockStr);

        char sourceStr[MAX_ADDRESS_STRING_LENGTH];
        IO_ConvertIpAddressToString(sourceAddr, sourceStr);

        printf("Node %u receive OSPF Packet from node %s at time %s\n",
                node->nodeId, sourceStr, clockStr);
        Ospfv2PrintProtocolPacket(ospfHeader);
    }

    // What type of packet did we get?
    switch (ospfHeader->packetType)
    {
        // It's a Hello packet.
        case OSPFv2_HELLO:
        {
            Ospfv2HelloPacket* helloPkt = NULL;

            if (OSPFv2_DEBUG_HELLO)
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);

                char sourceStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(sourceAddr,sourceStr);

                printf("Node %u receive HELLO from node %s at time %s\n",
                        node->nodeId, sourceStr, clockStr);
            }

            ospf->stats.numHelloRecv++;

            // Get the Hello packet.
            helloPkt = (Ospfv2HelloPacket*)
                MESSAGE_ReturnPacket(msg);

            // Process the received Hello packet.
            Ospfv2HandleHelloPacket(node,
                                    helloPkt,
                                    sourceAddr,
                                    sourceId,
                                    interfaceIndex);

            break;
        }

        // It's a link state update packet.
        case OSPFv2_LINK_STATE_UPDATE:
        {
            Ospfv2LinkStateUpdatePacket* updatePkt = NULL;

            if (Ospfv2DebugFlood(node))
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);
                char sourceStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(sourceAddr,sourceStr);
                printf("Node %u receive UPDATE from node %s at time %s\n",
                        node->nodeId, sourceStr, clockStr);
            }

            ospf->stats.numLSUpdateRecv++;

            // update the interface based stats
            if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
            {
                ospf->iface[interfaceIndex].interfaceStats.numLSUpdateRecv++;
                ospf->iface[interfaceIndex].interfaceStats.numLSUpdateBytesRecv +=
                    MESSAGE_ReturnPacketSize(msg);
            }

            // Get update packet
            updatePkt = (Ospfv2LinkStateUpdatePacket*)
                            MESSAGE_ReturnPacket(msg);

            // Process the Update packet.
            Ospfv2HandleUpdatePacket(
                node, msg, sourceAddr, interfaceIndex);

            break;
        }

        // It's a link state acknowledgement packet.
        case OSPFv2_LINK_STATE_ACK:
        {
            Ospfv2LinkStateAckPacket* ackPkt = NULL;

            if (Ospfv2DebugFlood(node))
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);

                char sourceStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(sourceAddr,sourceStr);
                printf("Node %u receive ACK from node %s at time %s\n",
                        node->nodeId, sourceStr, clockStr);
            }

            ospf->stats.numAckRecv++;

            // Update interface based stats
            if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
            {
                ospf->iface[interfaceIndex].interfaceStats.numAckRecv++;
                ospf->iface[interfaceIndex].interfaceStats.numAckBytesRecv +=
                    MESSAGE_ReturnPacketSize(msg);
            }

            // Get LS Ack packet
            ackPkt = (Ospfv2LinkStateAckPacket*)
                            MESSAGE_ReturnPacket(msg);

            // Process the ACK packet.
            Ospfv2HandleAckPacket(
                node, ackPkt, sourceAddr, interfaceIndex);

            break;
        }
        // It's a database description packet.
        case OSPFv2_DATABASE_DESCRIPTION:
        {
            Ospfv2DatabaseDescriptionPacket* dbDscrpPkt = NULL;

            dbDscrpPkt = (Ospfv2DatabaseDescriptionPacket*)
                            MESSAGE_ReturnPacket(msg);

            if (Ospfv2DebugSync(node))
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);
                char sourceStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(sourceAddr,sourceStr);
                printf("Node %u receive DATABASE with seqno %u"
                        " from node %s at time %s\n",
                        node->nodeId,
                        dbDscrpPkt->ddSequenceNumber,
                        sourceStr,
                        clockStr);


                Ospfv2PrintDatabaseDescriptionPacket(dbDscrpPkt);
            }

            ospf->stats.numDDPktRecv++;

            // Update interface based stats
            if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
            {
                ospf->iface[interfaceIndex].interfaceStats.numDDPktRecv++;
                ospf->iface[interfaceIndex].interfaceStats.numDDPktBytesRecv +=
                    MESSAGE_ReturnPacketSize(msg);
            }

            // Process the Database Description packet.
            Ospfv2HandleDatabaseDescriptionPacket(node,
                                                  msg,
                                                  sourceAddr,
                                                  interfaceIndex);

            break;
        }

        // It's a Link State Request packet.
        case OSPFv2_LINK_STATE_REQUEST:
        {
            Ospfv2LinkStateRequestPacket* reqPkt = NULL;

            if (Ospfv2DebugSync(node))
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);
                char sourceStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(sourceAddr,sourceStr);

                printf("Node %u receive REQUEST from node %s "
                        "at time %s\n",
                        node->nodeId,
                        sourceStr,
                        clockStr);
            }

            ospf->stats.numLSReqRecv++;

            // Update interface based stats
            if (thisIntf->type != OSPFv2_NON_OSPF_INTERFACE)
            {
                ospf->iface[interfaceIndex].interfaceStats.numLSReqRecv++;
                ospf->iface[interfaceIndex].interfaceStats.numLSReqBytesRecv +=
                    MESSAGE_ReturnPacketSize(msg);
            }

            // get Request packet
            reqPkt = (Ospfv2LinkStateRequestPacket*)
                            MESSAGE_ReturnPacket(msg);

            // Process the Link State Request packet.
            Ospfv2HandleLSRequestPacket(node,
                                        msg,
                                        sourceAddr,
                                        interfaceIndex);

            break;
        }
        /***** Start: ROSPF Redirect *****/
#ifdef ADDON_BOEINGFCS
        case OSPFv2_ROSPF_DBDG:
        {
            if (thisIntf->type != OSPFv2_ROSPF_INTERFACE ||
                ! RoutingCesRospfDatabaseDigestsEnabled(node, interfaceIndex))
            {
                // got a database digest message where one wasn't expected,
                // drop the packet and ignore it
                ERROR_ReportWarning("Got a database digest packet on a "
                                    "non-ROSPF interface (or one not "
                                    "configured for database digests)");
                break;
            }

            if (Ospfv2DebugSync(node))
            {
                char clockStr[100];
                ctoa(node->getNodeTime()+getSimStartTime(node), clockStr);
                char sourceStr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(sourceAddr,sourceStr);
                printf("Node %u receive DATABASE DIGEST"
                        " from node %s at time %s\n",
                        node->nodeId,
                        sourceStr,
                        clockStr);
            }
            RoutingCesRospfProcessDbdgMsg(node,
                                          interfaceIndex,
                                          sourceAddr,
                                          msg);
            break;
        }

        /***** Start: ROSPF Redirect *****/
        // It's an ROSPF Redirect packet.
        case OSPFv2_ROSPF_REDIRECT:
        {
            if (RoutingCesRospfActiveOnInterface(node, interfaceIndex) == FALSE)
            {
                ERROR_ReportWarning("Received ROSPF redirect on non-ROSPF interface\n");
            }
            else
            {
                RoutingCesRospfHandleRedirectPacket(node,
                                                    msg,
                                                    sourceAddr,
                                                    interfaceIndex);
            }
            break;
        }
#endif
        /***** End: ROSPF Redirect *****/
        default:
        {
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                "Packet type not indentified",
                JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, "Packet type not indentified\n");
        }
    }

    MESSAGE_Free (node, msg);
}


//-------------------------------------------------------------------------//
// NAME         :Ospfv2HandleRoutingProtocolEvent
// PURPOSE      :Self-sent messages are handled in this function. Generally,
//               this function handles the timers required for the
//               implementation of OSPF. This function is called from the
//               funtion NetworkIpLayer() in ip.c.
// ASSUMPTION   :None
// RETURN VALUE :None.
//-------------------------------------------------------------------------//

void Ospfv2HandleRoutingProtocolEvent(Node* node, Message* msg)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Message* newMsg = NULL;

    // Sanity check
    if (!ospf)
    {
        if (!((msg->eventType == MSG_STATS_OSPF_InsertNeighborState) ||
              (msg->eventType == MSG_STATS_OSPF_InsertRouterLsa) ||
              (msg->eventType == MSG_STATS_OSPF_InsertNetworkLsa) ||
              (msg->eventType == MSG_STATS_OSPF_InsertSummaryLsa) ||
              (msg->eventType == MSG_STATS_OSPF_InsertExternalLsa) ||
              (msg->eventType == MSG_STATS_OSPF_InsertInterfaceState) ||
              (msg->eventType == MSG_STATS_OSPF_InsertAggregate) ||
              (msg->eventType == MSG_STATS_OSPF_InsertSummary) ||
              (msg->eventType == MSG_STATS_MOSPF_InsertSummary) ||
              (msg->eventType == MSG_STATS_MULTICAST_InsertSummary)))
        {
            if (msg->eventType == MSG_ROUTING_QospfSetNewConnection)
            {
                ERROR_ReportErrorArgs("Node %u: Q-OSPF works only with OSPF "
                                      "routing protocol\n", node->nodeId);
            }
            else
            {
                ERROR_ReportErrorArgs("Node %u got invalid (ospf) timer\n",
                                      node->nodeId);
            }
        }

        MESSAGE_Free(node, msg);
        return;
    }

    switch (msg->eventType)
    {
        // Time to increment the age of each LSA.
        case MSG_ROUTING_OspfIncrementLSAge:
        {
            if (OSPFv2_DEBUG_LSDBErr)
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);
                printf("Node %u got IncrementLSAge expired at time %s\n",
                        node->nodeId, clockStr);
            }

            // Increment age of each LSA.
            Ospfv2IncrementLSAge(node);

            // Reschedule LSA increment interval.
            newMsg = MESSAGE_Duplicate(node, msg);
            MESSAGE_Send(node,
                         newMsg,
                         ospf->incrementTime);

            break;
        }

        // Check to make sure that my neighbor is still there.
        case MSG_ROUTING_OspfInactivityTimer:
        {
            Ospfv2TimerInfo* info =
                (Ospfv2TimerInfo*) MESSAGE_ReturnInfo(msg);
            Ospfv2Neighbor* neighborInfo = NULL;
            int interfaceIndex;

            if (OSPFv2_DEBUG_HELLOErr)
            {
                char clockStr[100];
                char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);
                IO_ConvertIpAddressToString(info->neighborAddr, nbrAddrStr);
                printf("Node %u got InactivityTimer expired for "
                       "neighbor %s at time %s\n",
                        node->nodeId, nbrAddrStr, clockStr);
                printf("    sequence number is %u\n", info->sequenceNumber);
            }

            // Get the neighbor's information from my neighbor list.
            neighborInfo = Ospfv2GetNeighborInfoByIPAddress(
                                node,
                                info->interfaceIndex,
                                info->neighborAddr);
            interfaceIndex = info->interfaceIndex;

            // If neighbor doesn't exist (possibly removed from
            // neighbor list).
            if (neighborInfo == NULL || interfaceIndex == -1)
            {
                if (OSPFv2_DEBUG_HELLOErr)
                {
                    printf("    neighbor NO LONGER EXISTS\n");
                }

                break;
            }

#ifdef JNE_LIB
            if (interfaceIndex == -1)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "No Interface for Neighbor!!",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(interfaceIndex != -1,
                                "No Interface for Neighbor!!\n");

            if (OSPFv2_DEBUG_HELLOErr)
            {
                printf("Current sequence number is %u\n",
                        neighborInfo->inactivityTimerSequenceNumber);
            }

            // Timer expired.
            if (info->sequenceNumber !=
                neighborInfo->inactivityTimerSequenceNumber)
            {
                if (OSPFv2_DEBUG_HELLOErr)
                {
                    printf("    Old timer\n");
                }

                // Timer is an old one, so just ignore.
                break;
            }

            // If this timer has not expired already.
            Ospfv2HandleNeighborEvent(node,
                                      interfaceIndex,
                                      neighborInfo->neighborIPAddress,
                                      E_InactivityTimer);

            if (OSPFv2_DEBUG_HELLOErr)
            {
                Ospfv2PrintNeighborList(node);
            }

            break;
        }

        // Check to see if I need to retransmit any Packet.
        case MSG_ROUTING_OspfRxmtTimer:
        {
            Ospfv2TimerInfo* info =
                (Ospfv2TimerInfo*) MESSAGE_ReturnInfo(msg);
            Ospfv2Neighbor* neighborInfo = NULL;

            if (OSPFv2_DEBUG_ERRORS)
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);
                printf("Node %u got RxmtTimer expired at time %s\n",
                        node->nodeId, clockStr);
                printf("    sequence number is %u\n", info->sequenceNumber);
            }

            neighborInfo = Ospfv2GetNeighborInfoByIPAddress(
                                node,
                                info->interfaceIndex,
                                info->neighborAddr);

            // If neighbor doesn't exist (possibly removed from
            // neighbor list).
            if (neighborInfo == NULL)
            {
                if (OSPFv2_DEBUG_ERRORS)
                {
                    char nbrAddrStr[MAX_ADDRESS_STRING_LENGTH];
                    IO_ConvertIpAddressToString(info->neighborAddr,
                                                                nbrAddrStr);
                    printf("Node %d: Got Rxtx timer for neighbor %s."
                           "  Neighbor NO LONGER EXISTS\n",
                           node->nodeId, nbrAddrStr);
                }

                break;
            }

            // Retransmit packet to specified neighbor.
            Ospfv2HandleRetransmitTimer(node,
                                        info->interfaceIndex,
                                        info->sequenceNumber,
                                        info->neighborAddr,
                                        info->type);

            break;
        }

        // Time to send out Hello packets.
        case MSG_ROUTING_OspfScheduleHello:
        {
            clocktype delay;
            int i;
            i = (int)MESSAGE_GetInstanceId(msg);
            if (OSPFv2_DEBUG_HELLO)
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);
                printf("Node %u sending HELLO on interface %d at time %s\n",
                        node->nodeId, i, clockStr);
            }


            // Broadcast Hello packets to neighbors. on each interface
            Ospfv2SendHelloPacket(node, i);

            delay = (clocktype) (ospf->iface[i].helloInterval
                + (RANDOM_erand(ospf->seed) *  OSPFv2_BROADCAST_JITTER));

            // Reschedule Hello packet broadcast.
            newMsg = MESSAGE_Duplicate(node, msg);
            MESSAGE_Send(node, newMsg, delay);

            break;
        }

        // Time to originate LSAs.
        case MSG_ROUTING_OspfScheduleLSDB:
        {
            Ospfv2LinkStateType lsType;
            int interfaceId;
            unsigned int areaId;
            BOOL flush = FALSE;
            char* info = NULL;

            if (MESSAGE_ReturnInfoSize(msg) == 0)
            {
                break;
            }

            info = MESSAGE_ReturnInfo(msg);
            memcpy(&lsType, info, sizeof(Ospfv2LinkStateType));
            info += sizeof(Ospfv2LinkStateType);

            if (lsType == OSPFv2_ROUTER)
            {
                memcpy(&areaId, info, sizeof(unsigned int));
                info += sizeof(unsigned int);
                memcpy(&flush, info, sizeof(BOOL));

                Ospfv2OriginateRouterLSAForThisArea(
                    node, areaId, flush);
            }
            else if (lsType == OSPFv2_NETWORK)
            {
                int seqNum;

                memcpy(&interfaceId, info, sizeof(int));
                info += sizeof(int);
                memcpy(&flush, info, sizeof(BOOL));
                info += sizeof(BOOL);
                memcpy(&seqNum, info, sizeof(int));

                // If this timer has not been cancelled yet
                if (ospf->iface[interfaceId].networkLSTimerSeqNumber
                    == seqNum)
                {
                    Ospfv2OriginateNetworkLSA(node, interfaceId, flush);
                }
            }
            else if (lsType == OSPFv2_NETWORK_SUMMARY
                || lsType == OSPFv2_ROUTER_SUMMARY)
            {
                Ospfv2RoutingTableRow advRt;
                Ospfv2RoutingTableRow* rowPtr = NULL;
                Ospfv2Area* thisArea = NULL;
                int retVal = 0;

                memset(&advRt, 0, sizeof(Ospfv2RoutingTableRow));

                memcpy(&areaId, info, sizeof(unsigned int));
                info += sizeof(unsigned int);
                memcpy(&flush, info, sizeof(BOOL));
                info += sizeof(BOOL);
                memcpy(&advRt.destAddr, info, sizeof(NodeAddress));
                info += sizeof(NodeAddress);
                memcpy(&advRt.addrMask, info, sizeof(NodeAddress));
                info += sizeof(NodeAddress);
                memcpy(&advRt.destType, info, sizeof(Ospfv2DestType));

                thisArea = Ospfv2GetArea(node, areaId);

                retVal = Ospfv2CheckForSummaryLSAValidity(
                            node, advRt.destAddr, advRt.addrMask,
                            advRt.destType, thisArea->areaID, &rowPtr);

                if (retVal == 1)
                {
                    if (advRt.destAddr == OSPFv2_DEFAULT_DESTINATION)
                    {
#ifdef JNE_LIB
                        if (thisArea->transitCapability)
                        {
                            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                                "Default route should be advertised in "
                                "stub area",
                                JNE::CRITICAL);
                        }
#endif
                        ERROR_Assert(!thisArea->externalRoutingCapability,
                            "Default route should be advertised in "
                            "stub area\n");
                        advRt.metric = thisArea->stubDefaultCost;
                        advRt.flag = OSPFv2_ROUTE_NO_CHANGE;
                    }
                    else
                    {
                        advRt.metric = rowPtr->metric;
                        advRt.flag = rowPtr->flag;
                    }
                }
                else if (retVal == -1)
                {
                    Ospfv2AreaAddressRange* addrInfo;

                    addrInfo = Ospfv2GetAddrRangeInfoForAllArea(
                                    node, advRt.destAddr);
#ifdef JNE_LIB
                    if (!addrInfo ||
                        addrInfo->address != advRt.destAddr)
                    {
                        JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                            "Scheduled for wrong summary information",
                            JNE::CRITICAL);
                    }
#endif
                    ERROR_Assert(addrInfo &&
                        addrInfo->address == advRt.destAddr,
                        "Scheduled for wrong summary information\n");

                    advRt.metric = Ospfv2GetMaxMetricForThisRange(
                                        &ospf->routingTable, addrInfo);
                    advRt.flag = OSPFv2_ROUTE_NO_CHANGE;
                }
                else
                {
                    break;
                }

                Ospfv2OriginateSummaryLSA(node, &advRt, thisArea, flush);
            }
            // BGP-OSPF Patch Start
            else if (lsType == OSPFv2_ROUTER_AS_EXTERNAL)
            {
                NodeAddress destAddress;
                NodeAddress destAddressMask;
                NodeAddress nextHopAddress;
                int external2Cost;

                memcpy(&destAddress, info, sizeof(NodeAddress));
                info += sizeof(NodeAddress);
                memcpy(&destAddressMask, info, sizeof(NodeAddress));
                info += sizeof(NodeAddress);
                memcpy(&nextHopAddress, info, sizeof(NodeAddress));
                info += sizeof(NodeAddress);
                memcpy(&external2Cost, info, sizeof(int));
                info += sizeof(int);
                memcpy(&flush, info, sizeof(BOOL));

                Ospfv2OriginateASExternalLSA(node,
                                             destAddress,
                                             destAddressMask,
                                             nextHopAddress,
                                             external2Cost,
                                             flush);
            }
            // BGP-OSPF Patch End

            // M-OSPF Patch Start
            else if (lsType == OSPFv2_GROUP_MEMBERSHIP)
            {
                NodeAddress groupId;

                memcpy(&areaId, info, sizeof(unsigned int));
                info += sizeof(unsigned int);
                memcpy(&flush, info, sizeof(BOOL));
                info += sizeof(BOOL);
                memcpy(&interfaceId, info, sizeof(int));
                info += sizeof(int);
                memcpy(&groupId, info, sizeof(NodeAddress));

                MospfCheckLocalGroupDatabaseToOriginateGroupMembershipLSA(
                    node, areaId, interfaceId, groupId, flush);
            }
            // M-OSPF Patch End
            // NSSA support start
            else if (lsType == OSPFv2_ROUTER_NSSA_EXTERNAL)
            {
                NodeAddress destAddress;
                NodeAddress destAddressMask;
                NodeAddress nextHopAddress;
                int external2Cost;

                memcpy(&destAddress, info, sizeof(NodeAddress));
                info += sizeof(NodeAddress);
                memcpy(&destAddressMask, info, sizeof(NodeAddress));
                info += sizeof(NodeAddress);
                memcpy(&nextHopAddress, info, sizeof(NodeAddress));
                info += sizeof(NodeAddress);
                memcpy(&external2Cost, info, sizeof(int));
                info += sizeof(int);
                memcpy(&flush, info, sizeof(BOOL));

                Ospfv2OriginateNSSAExternalLSA(node,
                                             destAddress,
                                             destAddressMask,
                                             nextHopAddress,
                                             external2Cost,
                                             flush);
            }
            // NSSA support end

            /***** Start: OPAQUE-LSA *****/
            else if (lsType == OSPFv2_AS_SCOPE_OPAQUE)
            {
                Ospfv2OriginateType11OpaqueLSA(node);
            }
            /***** End: OPAQUE-LSA *****/

            break;
        }

        // Q-OSPF Patch Start
        // Q-OSPF periodic update LSA message
        case MSG_ROUTING_QospfScheduleLSDB:
        {
            QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;

            // Originate router LSA to inform the change of bandwidth
            QospfOriginateLSA(node, qospf, TRUE);

            // Reschedule LSA broadcast.
            newMsg = MESSAGE_Duplicate(node, msg);
            MESSAGE_Send(node, newMsg, qospf->floodingInterval);
            break;
        }

        // Interface status monitor message
        case MSG_ROUTING_QospfInterfaceStatusMonitor:
        {
            QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;

            // Originate router LSA to inform the change of bandwidth
            QospfOriginateLSA(node, qospf, FALSE);

            // Reschedule LSA broadcast.
            newMsg = MESSAGE_Duplicate(node, msg);
            MESSAGE_Send(node, newMsg, qospf->interfaceMonitorInterval);
            break;
        }

        // message for Qos application to initiate a new connection
        case MSG_ROUTING_QospfSetNewConnection:
        {
#ifdef JNE_LIB
            if (ospf->isQosEnabled != TRUE)
            {
                JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                    "Q-OSPF not enabled."
                    " Enabled it before QoS service request",
                    JNE::CRITICAL);
            }
#endif
            ERROR_Assert(ospf->isQosEnabled == TRUE, "\nQ-OSPF not enabled."
            " Enabled it before QoS service request");

            // Qos application is requested, so find Qos path for
            // this newly arrived connection
            QospfCreateDatabaseAndFindQosPath(node, msg);
            break;
        }
        // Q-OSPF Patch End

        // Single Shot Wait Timer
        case MSG_ROUTING_OspfWaitTimer:
        {
            int interfaceId;

            memcpy(&interfaceId, MESSAGE_ReturnInfo(msg), sizeof(int));

            if (Ospfv2DebugISM(node))
            {
                char clockStr[100];

                TIME_PrintClockInSecond(node->getNodeTime() +
                                            getSimStartTime(node), clockStr);

                fprintf(stdout, "Node %u, Interface %d: Wait Timer Expire "
                                "at time %s\n",
                                 node->nodeId, interfaceId, clockStr);
            }

            ospf->iface[interfaceId].waitTimerMsg = NULL;

            Ospfv2HandleInterfaceEvent(node, interfaceId, E_WaitTimer);
            break;
        }

        case MSG_ROUTING_OspfFloodTimer:
        {
            int interfaceId;
            NodeAddress nbrAddr = ANY_DEST;
            memcpy(&interfaceId, MESSAGE_ReturnInfo(msg), sizeof(int));
#ifdef ADDON_BOEINGFCS
            memcpy(&nbrAddr, MESSAGE_ReturnInfo(msg)+
                             sizeof(int), sizeof(NodeAddress));
#endif
            if (ospf->iface[interfaceId].floodTimerSet == FALSE)
            {
                break;
            }

#ifdef ADDON_BOEINGFCS
            // flood configuration information
            if (ospf->iface[interfaceId].type == OSPFv2_ROSPF_INTERFACE)
            {
                RoutingCesRospfHandleFloodTimer(node, interfaceId, nbrAddr);
            }
            else
            {
#endif
            Ospfv2SendLSUpdate(node, nbrAddr, interfaceId);

#ifdef ADDON_BOEINGFCS
            }
#endif
            ospf->iface[interfaceId].floodTimerSet = FALSE;

            break;
        }

        case MSG_ROUTING_OspfDelayedAckTimer:
        {
            int interfaceId;
            Ospfv2ListItem* listItem = NULL;
            int count;
            Ospfv2LinkStateHeader* payload = NULL;
            int maxCount;
#ifdef ADDON_BOEINGFCS
            NodeAddress nbrAddr;
#endif
            NodeAddress dstAddr;

            memcpy(&interfaceId, MESSAGE_ReturnInfo(msg), sizeof(int));
#ifdef ADDON_BOEINGFCS
            memcpy(&nbrAddr, MESSAGE_ReturnInfo(msg)+sizeof(int)
                           , sizeof(NodeAddress));
#endif
            if (ospf->iface[interfaceId].delayedAckTimer == FALSE)
            {
                break;
            }

            // Determine Destination Address
#ifdef ADDON_BOEINGFCS
            if (ospf->iface[interfaceId].type ==
                            OSPFv2_ROSPF_INTERFACE)
            {
                dstAddr = nbrAddr;
            }
            else
#endif
            if ((ospf->iface[interfaceId].state == S_DR)
                || (ospf->iface[interfaceId].state == S_Backup))
            {
                dstAddr = OSPFv2_ALL_SPF_ROUTERS;
            }
            else if (ospf->iface[interfaceId].type ==
                            OSPFv2_BROADCAST_INTERFACE)
            {
                dstAddr = OSPFv2_ALL_D_ROUTERS;
            }
            else
            {
                dstAddr = OSPFv2_ALL_SPF_ROUTERS;
            }

            maxCount =
                (GetNetworkIPFragUnit(node, interfaceId)
                 - sizeof(IpHeaderType)
                 - sizeof(Ospfv2LinkStateAckPacket))
                / sizeof(Ospfv2LinkStateHeader);

            payload = (Ospfv2LinkStateHeader*)
                        MEM_malloc(maxCount
                        *  sizeof(Ospfv2LinkStateHeader));
            count = 0;

            while (!Ospfv2ListIsEmpty(
                ospf->iface[interfaceId].delayedAckList))
            {
                listItem =
                    ospf->iface[interfaceId].delayedAckList->first;

                if (count == maxCount)
                {
                    Ospfv2SendAckPacket(node,
                                        (char*) payload,
                                        count,
                                        dstAddr,
                                        interfaceId);

                    // Reset variables
                    count = 0;
                    continue;
                }

                memcpy(&payload[count],
                       listItem->data,
                       sizeof(Ospfv2LinkStateHeader));

                count++;

                // Remove item from list
                Ospfv2RemoveFromList(
                                node,
                                ospf->iface[interfaceId].delayedAckList,
                                listItem,
                                FALSE);
            }

            Ospfv2SendAckPacket(node,
                                (char*)payload,
                                count,
                                dstAddr,
                                interfaceId);

            ospf->iface[interfaceId].delayedAckTimer = FALSE;
            MEM_free(payload);
            break;
        }

        case MSG_ROUTING_OspfScheduleSPF:
        {
            ospf->SPFTimer = node->getNodeTime();
            Ospfv2FindShortestPath(node);

            // M-OSPF Patch Start
            if (ospf->isMospfEnable == TRUE)
            {
                MospfModifyShortestPathAndForwardingTable(node) ;
            }
            // M-OSPF Patch End

            break;
        }

        case MSG_ROUTING_OspfNeighborEvent:
        {
            Ospfv2NSMTimerInfo* NSMTimerInfo = (Ospfv2NSMTimerInfo*)
                                MESSAGE_ReturnInfo(msg);

            Ospfv2HandleNeighborEvent(node,
                                      NSMTimerInfo->interfaceId,
                                      NSMTimerInfo->nbrAddr,
                                      NSMTimerInfo->nbrEvent);
            break;
        }

        case MSG_ROUTING_OspfInterfaceEvent:
        {
            Ospfv2ISMTimerInfo* ISMTimerInfo = (Ospfv2ISMTimerInfo*)
                                MESSAGE_ReturnInfo(msg);

            Ospfv2HandleInterfaceEvent(node,
                                       ISMTimerInfo->interfaceId,
                                       ISMTimerInfo->intfEvent);

            ospf->iface[ISMTimerInfo->interfaceId].ISMTimer = FALSE;
            break;
        }
#ifdef ADDON_DB
        case MSG_STATS_OSPF_InsertNeighborState:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL ||
                !db->statsOspfTable->createOspfNeighborStateTable)
            {
                break;
            }

            HandleStatsDBOspfNeighborStateTableInsertion(node);
            MESSAGE_Send(node,
                MESSAGE_Duplicate(node, msg),
                db->statsOspfTable->ospfNeighborStateTableInterval);
            break;
        }
        case MSG_STATS_OSPF_InsertRouterLsa:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL || !db->statsOspfTable->createOspfRouterLsaTable)
            {
                break;
            }

            HandleStatsDBOspfRouterLsaTableInsertion(node);
            MESSAGE_Send(node,
                MESSAGE_Duplicate(node, msg),
                db->statsOspfTable->ospfRouterLsaTableInterval);
            break;
        }
        case MSG_STATS_OSPF_InsertNetworkLsa:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL || !db->statsOspfTable->createOspfNetworkLsaTable)
            {
                break;
            }

            HandleStatsDBOspfNetworkLsaTableInsertion(node);
            MESSAGE_Send(node,
                MESSAGE_Duplicate(node, msg),
                db->statsOspfTable->ospfNetworkLsaTableInterval);
            break;
        }
        case MSG_STATS_OSPF_InsertSummaryLsa:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL || !db->statsOspfTable->createOspfSummaryLsaTable)
            {
                break;
            }

            HandleStatsDBOspfSummaryLsaTableInsertion(node);
            MESSAGE_Send(node,
                MESSAGE_Duplicate(node, msg),
                db->statsOspfTable->ospfSummaryLsaTableInterval);
            break;
        }
        case MSG_STATS_OSPF_InsertExternalLsa:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL ||
                !db->statsOspfTable->createOspfExternalLsaTable)
            {
                break;
            }

            HandleStatsDBOspfExternalLsaTableInsertion(node);
            MESSAGE_Send(node,
                MESSAGE_Duplicate(node, msg),
                db->statsOspfTable->ospfExternalLsaTableInterval);
            break;
        }
        case MSG_STATS_OSPF_InsertInterfaceState:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL ||
                !db->statsOspfTable->createOspfInterfaceStateTable)
            {
                break;
            }

            HandleStatsDBOspfInterfaceStateTableInsertion(node);
            MESSAGE_Send(node,
                MESSAGE_Duplicate(node, msg),
                db->statsOspfTable->ospfInterfaceStateTableInterval);
            break;
        }
        case MSG_STATS_OSPF_InsertAggregate:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL || !db->statsOspfTable->createOspfAggregateTable)
            {
                break;
            }

            HandleStatsDBOspfAggregateTableInsertion(node);
            MESSAGE_Send(node,
                MESSAGE_Duplicate(node, msg),
                db->statsOspfTable->ospfAggregateTableInterval);
            break;
        }
        case MSG_STATS_OSPF_InsertSummary:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL || !db->statsOspfTable->createOspfSummaryTable)
            {
                break;
            }

            HandleStatsDBOspfSummaryTableInsertion(node);
            MESSAGE_Send(node,
                MESSAGE_Duplicate(node, msg),
                db->statsOspfTable->ospfSummaryTableInterval);
            break;
        }
        case MSG_STATS_MOSPF_InsertSummary:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL)
            {
                break;
            }

            if (!db->statsOspfTable
                || !db->statsOspfTable->createMospfSummaryTable)
                //|| ospf->isMospfEnable)
            {
                break;
            }

            HandleStatsDBMospfSummaryTableInsertion(node);

            MESSAGE_Send(node,
                         MESSAGE_Duplicate(node, msg),
                         db->statsOspfTable->mospfSummaryInterval);

            break;
        }
        case MSG_STATS_MULTICAST_InsertSummary:
        {
            StatsDb* db = node->partitionData->statsDb;

            //multicast network summary handling
            // Check if the Table exists.
            if (!db || !db->statsSummaryTable
                || !db->statsSummaryTable->createMulticastNetSummaryTable)
            {
                break;
            }

           HandleStatsDBMospfMulticastNetSummaryTableInsertion(node);

           MESSAGE_Send(node,
                        MESSAGE_Duplicate(node, msg),
                        db->statsSummaryTable->summaryInterval);

           break;
        }
#endif
#ifdef ADDON_MA
        case MSG_ROUTING_OspfStartTimeMA:
        {
            if (node->mAEnabled && !ospf->sendLSAtoMA)
            {
                ospf->sendLSAtoMA = TRUE;
                Ospfv2SendAllLSAtoMA(node, ospf);
            }
            break;

        }
#endif
        case MSG_ROUTING_OspfMaxAgeRemovalTimer:
        {
            Ospfv2RemoveMaxAgeLSAListFromLSDB(node);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];

            // Shouldn't get here.
            sprintf(errStr, "Node %u: Got invalid ospf timer\n",
                node->nodeId);
#ifdef JNE_LIB
            JneWriteToLogFile(node, JNE::M_OSPF,"Error",
                errStr,
                JNE::CRITICAL);
#endif
            ERROR_Assert(FALSE, errStr);
        }
    }// end switch

    MESSAGE_Free(node, msg);
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2Finalize
// PURPOSE: Prints out the required statistics for this protocol during
//          termination of the simulation. This function is called from
//          nwip.pc
// RETURN: None
//-------------------------------------------------------------------------//

void Ospfv2Finalize(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)NetworkIpGetRoutingProtocol(
                                        node, ROUTING_PROTOCOL_OSPFv2);

    // Used to print statistics only once during finalization.  In
    // the future, stats should be tied to each interface.
    if (ospf->stats.alreadyPrinted == TRUE)
    {
        return;
    }

    ospf->stats.alreadyPrinted = TRUE;

    if (OSPFv2_DEBUG_TABLE)
    {
        printf("\n");
        Ospfv2PrintLSDB(node);
        Ospfv2PrintRoutingTable(node);
    }

    if (Ospfv2DebugSync(node))
    {
        Ospfv2PrintNeighborState(node);
    }

    if (OSPFv2_DEBUG_ERRORS)
    {
        int i;

        printf("Node %u address info\n", node->nodeId);

        for (i = 0; i < node->numberInterfaces; i++)
        {
            printf("    interface = %d\n", i);

            char netStr[MAX_ADDRESS_STRING_LENGTH];
            char subnetStr[MAX_ADDRESS_STRING_LENGTH];
            char ipStr[MAX_ADDRESS_STRING_LENGTH];

            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceNetworkAddress(node, i),
                netStr);
            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceSubnetMask(node, i),
                subnetStr);
            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceAddress(node, i),
                ipStr);

            printf("    network address = %s\n", netStr);
            printf("    subnet mask = %s\n", subnetStr);
            printf("    ip address = %s\n\n", ipStr);
        }

        NetworkPrintForwardingTable(node);
    }

    if (Ospfv2DebugISM(node))
    {
        int i;

        for (i = 0; i < node->numberInterfaces; i++)
        {
            char stateStr[20];

            if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
            {
                continue;
            }

            if (ospf->iface[i].state == S_Down) {
                strcpy(stateStr, "S_Down");
            }
            else if (ospf->iface[i].state == S_Loopback) {
                strcpy(stateStr, "S_Loopback");
            }
            else if (ospf->iface[i].state == S_Waiting) {
                strcpy(stateStr, "S_Waiting");
            }
            else if (ospf->iface[i].state == S_PointToPoint) {
                strcpy(stateStr, "S_PointToPoint");
            }
            else if (ospf->iface[i].state == S_DROther) {
                strcpy(stateStr, "S_DROther");
            }
            else if (ospf->iface[i].state == S_Backup) {
                strcpy(stateStr, "S_Backup");
            }
            else if (ospf->iface[i].state == S_DR) {
                strcpy(stateStr, "S_DR");
            }
            else {
                strcpy(stateStr, "Unknown");
            }

            printf("    [Node %u] Interface[%d] state %20s\n",
                    node->nodeId, i, stateStr);
        }
    }

    // Print out stats if user specifies it.
    if (ospf->collectStat == TRUE)
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "Hello Packets Sent = %d", ospf->stats.numHelloSent);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Hello Packets Received = %d",
            ospf->stats.numHelloRecv);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State Update Packets Sent = %d",
                 ospf->stats.numLSUpdateSent);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State Update Bytes Sent = %d",
                 ospf->stats.numLSUpdateBytesSent);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State Update Packets Received = %d",
                 ospf->stats.numLSUpdateRecv);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State ACK Packets Sent = %d",
                 ospf->stats.numAckSent);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State ACK Bytes Sent = %d",
                 ospf->stats.numAckBytesSent);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State ACK Packets Received = %d",
                 ospf->stats.numAckRecv);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State Update Packets Retransmitted = %d",
                 ospf->stats.numLSUpdateRxmt);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link State Advertisements Expired = %d",
                 ospf->stats.numExpiredLSAge);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Database Description Packets Sent = %d",
                 ospf->stats.numDDPktSent);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Database Description BytesSent = %d",
                 ospf->stats.numDDPktBytesSent);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Database Description Packets Received = %d",
                 ospf->stats.numDDPktRecv);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Database Description Packets Retransmitted = %d",
                 ospf->stats.numDDPktRxmt);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Database Description Bytes Retransmitted = %d",
                 ospf->stats.numDDPktBytesRxmt);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link State Request Packets Sent = %d",
                 ospf->stats.numLSReqSent);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link State Request Bytes Sent = %d",
                 ospf->stats.numLSReqBytesSent);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link State Request Packets Received = %d",
                 ospf->stats.numLSReqRecv);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link State Request Packets Retransmitted = %d",
                 ospf->stats.numLSReqRxmt);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Router LSA Originated = %d",
                 ospf->stats.numRtrLSAOriginate);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Network LSA Originated = %d",
                 ospf->stats.numNetLSAOriginate);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Summary LSA Originated = %d",
                 ospf->stats.numSumLSAOriginate);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "AS-External LSA Originated = %d",
                 ospf->stats.numASExtLSAOriginate);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        /***** Start: OPAQUE-LSA *****/
        if (ospf->opaqueCapable)
        {
            sprintf(buf, "Type 11 Opaque LSA Originated = %d",
                     ospf->stats.numASOpaqueOriginate);
            IO_PrintStat(
                node,
                "Network",
                "OSPFv2",
                ANY_DEST,
                -1,// instance Id,
                buf);
        }
        /***** End: OPAQUE-LSA *****/

        sprintf(buf, "Number of LSA Refreshed = %d",
                 ospf->stats.numLSARefreshed);
        IO_PrintStat(
            node,
            "Network",
            "OSPFv2",
            ANY_DEST,
            -1,// instance Id,
            buf);

        if (ospf->supportDC == TRUE)
        {
            sprintf(buf, "Number of DoNotAge LSA Sent = %d",
                     ospf->stats.numDoNotAgeLSASent);
            IO_PrintStat(
                node,
                "Network",
                "OSPFv2",
                ANY_DEST,
                -1,// instance Id,
                buf);

            sprintf(buf, "Number of DoNotAge LSA Received = %d",
                     ospf->stats.numDoNotAgeLSARecv);
            IO_PrintStat(
                node,
                "Network",
                "OSPFv2",
                ANY_DEST,
                -1,// instance Id,
                buf);
        }
    }

    if (ospf->isQosEnabled)
    {
    // Q-OSPF Patch Start
        QospfFinalize(node, (QospfData*) ospf->qosRoutingProtocol);
    // Q-OSPF Patch End
    }
#ifdef ADDON_BOEINGFCS
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    RoutingCesRospfData* rospf = ip->rospfData;

    if (rospf)
    {
        RoutingCesRospfFinalize(node);
    }
#endif

    Ospfv2Neighbor* tempNeighborInfo = NULL;
    Ospfv2ListItem* tempListItem = NULL;
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
        {
            continue;
        }

        tempListItem = ospf->iface[i].neighborList->first;

        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;
            ERROR_Assert(tempNeighborInfo, "Neighbor data not found\n");

            Ospfv2FreeList(node,
                           tempNeighborInfo->linkStateRetxList,
                           FALSE);

            Ospfv2FreeList(node,
                           tempNeighborInfo->linkStateSendList,
                           FALSE);

            Ospfv2FreeList(node,
                           tempNeighborInfo->DBSummaryList,
                           FALSE);

            Ospfv2FreeList(node,
                           tempNeighborInfo->linkStateRequestList,
                           FALSE);
            if (tempNeighborInfo->lastReceivedDDPacket != NULL)
            {
                MESSAGE_Free(node, tempNeighborInfo->lastReceivedDDPacket);
                tempNeighborInfo->lastReceivedDDPacket = NULL;
            }
            if (tempNeighborInfo->lastSentDDPkt != NULL)
            {
                MESSAGE_Free(node, tempNeighborInfo->lastSentDDPkt);
                tempNeighborInfo->lastSentDDPkt = NULL;
            }

            tempListItem = tempListItem->next;
        }

        Ospfv2FreeList(node,
                       ospf->iface[i].neighborList,
                       FALSE);
    }
}

//
// NAME         : Ospfv2FlushAllDoNotAgeFromArea
// PURPOSE      : Flush All DoNotAge From Area
// PARAMETERS   ::
// + node  :  Node pointer:contains the node pointer of an ospf
//            router
// + ospf  :  Ospfv2Data pointer:contains ospf data for a router
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2FlushAllDoNotAgeFromArea(Node* node,
                                    Ospfv2Data* ospf,
                                    Ospfv2Area* area)
{
    Ospfv2ListItem* item = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;

    //Check in Network List
    item = area->networkLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DoNotAge LSA
        if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
        {
            Ospfv2FlushLSA(node, (char*) LSHeader, area->areaID);
            item = item->next;

            Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader, area->areaID);
            continue;
        }

        item = item->next;
    }

    //Check in router List
    item = area->routerLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DoNotAge LSA
        if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
        {
            Ospfv2FlushLSA(node, (char*) LSHeader, area->areaID);
            item = item->next;

            Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader, area->areaID);
            continue;
        }

        item = item->next;
    }

    //Check in router summary LSA list
    item = area->routerSummaryLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DoNotAge LSA
        if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
        {
            Ospfv2FlushLSA(node, (char*) LSHeader, area->areaID);
            item = item->next;

            Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader, area->areaID);
            continue;
        }

        item = item->next;
    }

    //Check in network Summary LSA List
    item = area->networkSummaryLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DoNotAge LSA
        if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
        {
            Ospfv2FlushLSA(node, (char*) LSHeader, area->areaID);
            item = item->next;

            Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader, area->areaID);
            continue;
        }

        item = item->next;
    }

    //Check in AS-external list
    item = ospf->asExternalLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DoNotAge LSA
        if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
        {
            Ospfv2FlushLSA(node, (char*) LSHeader, OSPFv2_INVALID_AREA_ID);
            item = item->next;

            Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                OSPFv2_INVALID_AREA_ID);
            continue;
        }

        item = item->next;
    }

    //Check in Group-membership list
    item = area->groupMembershipLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DoNotAge LSA
        if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
        {
            Ospfv2FlushLSA(node, (char*) LSHeader, area->areaID);
            item = item->next;

            Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader, area->areaID);
            continue;
        }

        item = item->next;
    }

    //Check in nssa-external list
    item = ospf->nssaExternalLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DoNotAge LSA
        if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
        {
            Ospfv2FlushLSA(node, (char*) LSHeader, OSPFv2_INVALID_AREA_ID);
            item = item->next;

            Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                OSPFv2_INVALID_AREA_ID);
            continue;
        }

        item = item->next;
    }
    /***** Start: OPAQUE-LSA *****/
    if (ospf->ASOpaqueLSAList)
    {
        item = ospf->ASOpaqueLSAList->first;
        LSHeader = NULL;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //Check for DoNotAge LSA
            if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
            {
                Ospfv2FlushLSA(node, (char*) LSHeader, OSPFv2_INVALID_AREA_ID);
                item = item->next;

                Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                    OSPFv2_INVALID_AREA_ID);
                continue;
            }

            item = item->next;
        }
    }
    /***** End: OPAQUE-LSA *****/

}

//
// NAME         : Ospfv2SearchRouterDBforDC
// PURPOSE      : Search Router Database for LSA for Dc bit 0 or 1
//                  depending on the last parameter
// PARAMETERS   ::
// + ospf  :  Ospf Data pointer:contains the ospf data for an ospf
//            router
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// + dc    :  BOOL:contains 0 to search LSA with DC bit 0 and 1 to
//            search LSA with DC bit 1
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2SearchRouterDBforDC(Ospfv2Data* ospf,
                               Ospfv2Area* area,
                               BOOL dc)
{
    Ospfv2ListItem* item = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;

    //Check in Network List
    item = area->networkLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }

    //Check in router List
    item = area->routerLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }

    //Check in router summary LSA list
    item = area->routerSummaryLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }

    //Check in network Summary LSA List
    item = area->networkSummaryLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }

     //Check in AS-external list
    item = ospf->asExternalLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }

    //Check in Group-membership list
    item = area->groupMembershipLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }

    //Check in nssa-external list
    item = ospf->nssaExternalLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }
    /***** Start: OPAQUE-LSA *****/
    if (ospf->ASOpaqueLSAList)
    {
        item = ospf->ASOpaqueLSAList->first;
        LSHeader = NULL;

        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;
           //Check for DC bit as dc(given in parameter)
           if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
           {
               return TRUE;
           }

           item = item->next;
        }
    }
    /***** End: OPAQUE-LSA *****/

    return FALSE;
}

//
// NAME         : Ospfv2StoreUnreachableTimeStampForAllLSA
// PURPOSE      : Store the Unreachable timestamp for each LSA
// RFC:1793::SECTION 2.3::REMOVING STALE DONOTAGE LSAS
// The originator of the LSA has been unreachable (according to
// the routing calculations
// PARAMETERS   ::
// + node  :  Node pointer:contains the node pointer of an ospf
//            router
// + ospf  :  Ospfv2Data pointer:contains ospf data for a router
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2StoreUnreachableTimeStampForAllLSA(Node* node, Ospfv2Data* ospf)
{
    Ospfv2Area* area = NULL;
    Ospfv2ListItem* item = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    Ospfv2ListItem* listItem = NULL;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        area = (Ospfv2Area*) listItem->data;

        //Check in Network List
        item = area->networkLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //if previously it was reachable so now checking for its reachability
            if (item->unreachableTimeStamp == (clocktype)0)
            {
                //if not found i.e now it becomes unreachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                                   LSHeader->advertisingRouter) == FALSE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = node->getNodeTime();
                }
            }
            else //if it is already unreachable
            {
                //if found i.e now it becomes reachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                                   LSHeader->advertisingRouter) == TRUE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = (clocktype)0;
                }
            }

            item = item->next;
        }

        //Check in router List
        item = area->routerLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //if previously it was reachable so now checking for its reachability
            if (item->unreachableTimeStamp == (clocktype)0)
            {
                //if not found i.e now it becomes unreachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                                   LSHeader->advertisingRouter) == FALSE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = node->getNodeTime();
                }
            }
            else //if it is already unreachable
            {
                //if found i.e now it becomes reachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                                   LSHeader->advertisingRouter) == TRUE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = (clocktype)0;
                }
            }

            item = item->next;
        }

        //Check in router summary LSA list
        item = area->routerSummaryLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //if previously it was reachable so now checking for its reachability
            if (item->unreachableTimeStamp == (clocktype)0)
            {
                //if not found i.e now it becomes unreachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                               LSHeader->advertisingRouter) == FALSE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = node->getNodeTime();
                }
            }
            else //if it is already unreachable
            {
                //if found i.e now it becomes reachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                                   LSHeader->advertisingRouter) == TRUE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = (clocktype)0;
                }
            }

            item = item->next;
        }

        //Check in network Summary LSA List
        item = area->networkSummaryLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //if previously it was reachable so now checking for its reachability
            if (item->unreachableTimeStamp == (clocktype)0)
            {
                //if not found i.e now it becomes unreachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                                   LSHeader->advertisingRouter) == FALSE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = node->getNodeTime();
                }
            }
            else //if it is already unreachable
            {
                //if found i.e now it becomes reachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                                   LSHeader->advertisingRouter) == TRUE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = (clocktype)0;
                }
            }

            item = item->next;
        }

        //added on 18 sep 2009 start
        //Check in Group-membership list
        item = area->groupMembershipLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //if previously it was reachable so now checking for its reachability
            if (item->unreachableTimeStamp == (clocktype)0)
            {
                //if not found i.e now it becomes unreachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                               LSHeader->advertisingRouter) == FALSE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = node->getNodeTime();
                }
            }
            else //if it is already unreachable
            {
                //if found i.e now it becomes reachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                                   LSHeader->advertisingRouter) == TRUE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = (clocktype)0;
                }
            }

            item = item->next;
        }
    //added on 18 sep 2009 end
    }

    //added on 18 sep 2009 start
    //Check in AS-external list
    item = ospf->asExternalLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //if previously it was reachable so now checking for its reachability
        if (item->unreachableTimeStamp == (clocktype)0)
        {
            //if not found i.e now it becomes unreachable
            if (Ospfv2SearchAddressInForwardingTable(node,
                               LSHeader->advertisingRouter) == FALSE)
            {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = node->getNodeTime();
                }
            }
            else //if it is already unreachable
            {
                //if found i.e now it becomes reachable
                if (Ospfv2SearchAddressInForwardingTable(node,
                               LSHeader->advertisingRouter) == TRUE)
                {
                    //store the unreachable time stamp
                    item->unreachableTimeStamp = (clocktype)0;
                }
            }

            item = item->next;
        }



    //Check in nssa-external list
    item = ospf->nssaExternalLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //if previously it was reachable so now checking for its reachability
        if (item->unreachableTimeStamp == (clocktype)0)
        {
            //if not found i.e now it becomes unreachable
            if (Ospfv2SearchAddressInForwardingTable(node,
                               LSHeader->advertisingRouter) == FALSE)
            {
                //store the unreachable time stamp
                item->unreachableTimeStamp = node->getNodeTime();
            }
        }
        else //if it is already unreachable
        {
            //if found i.e now it becomes reachable
            if (Ospfv2SearchAddressInForwardingTable(node,
                               LSHeader->advertisingRouter) == TRUE)
            {
                //store the unreachable time stamp
                item->unreachableTimeStamp = (clocktype)0;
            }
        }

        item = item->next;
    }
    //added on 18 sep 2009 end
}

//
// NAME         : Ospfv2SearchAddressInForwardingTable
// PURPOSE      : Search Advertising router of the LSA in network IP
//                Forwarding Table.If found then return True else
//                False
// RFC:1793::SECTION 2.3::REMOVING STALE DONOTAGE LSAS
// The originator of the LSA has been unreachable (according to
// the routing calculations
// PARAMETERS   ::
// + node       :  Node pointer:contains the node pointer of an ospf
//                 router
// + advRouter  :  NodeAddress:contains the interface address of the
//                 advertising router of the LSA
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2SearchAddressInForwardingTable(Node* node,
                                            NodeAddress advRouter)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    NetworkForwardingTable *rt = &ip->forwardTable;
    int i = 0;
    NodeAddress maskedIpAddress;
    memset(&maskedIpAddress, 0, sizeof(NodeAddress));

    for (i = 0; i < rt->size; i++)
    {
        maskedIpAddress = MaskIpAddress(advRouter, rt->row[i].destAddressMask);
        if (rt->row[i].destAddress == maskedIpAddress)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//
// NAME         : Ospfv2RemoveStaleDoNotAgeLSA
// PURPOSE      : Remove stale Do Not Age
// RFC:1793::SECTION 2.3::REMOVING STALE DONOTAGE LSAS
// As LSAs with the DoNotAge bit set are never aged.
// Routers are required to flush a DoNotAge LSA if
// both of the following conditions are met:
// (1) The LSA has been in the router’s database for
//     at least MaxAge seconds.
// (2) The originator of the LSA has been unreachable (according to the
//     routing calculations) for at least MaxAge seconds.
// PARAMETERS   ::
// + node  :  Node pointer:contains the node pointer of an ospf router
// + ospf  :  Ospfv2Data pointer:contains ospf data for a router
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2RemoveStaleDoNotAgeLSA(Node* node, Ospfv2Data* ospf)
{
    Ospfv2Area* thisArea = NULL;
    Ospfv2ListItem* item = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    Ospfv2ListItem* listItem = NULL;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        thisArea = (Ospfv2Area*) listItem->data;

        //Check in Network List
        item = thisArea->networkLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //Check for DoNotAge LSA
            if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
            {
                //Check for atleast MaxAge Seconds
                if (((node->getNodeTime() - item->timeStamp) >=
                        OSPFv2_LSA_MAX_AGE)
                     &&
                    (item->unreachableTimeStamp!=0)
                     &&
                     ((node->getNodeTime() - item->unreachableTimeStamp) >=
                            OSPFv2_LSA_MAX_AGE))
                {
                   //Flush this LSA
                    Ospfv2FlushLSA(node, (char*) LSHeader, thisArea->areaID);
                    item = item->next;

                    Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                        thisArea->areaID);
                    continue;
                }
            }

            item = item->next;
        }

        //Check in router List
        item = thisArea->routerLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //Check for DoNotAge LSA
            if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
            {
                //Check for atleast MaxAge Seconds
                if (((node->getNodeTime() - item->timeStamp) >=
                            OSPFv2_LSA_MAX_AGE)
                     &&
                    (item->unreachableTimeStamp!=0)
                     &&
                     ((node->getNodeTime() - item->unreachableTimeStamp) >=
                            OSPFv2_LSA_MAX_AGE))
                {
                   //Flush this LSA
                    Ospfv2FlushLSA(node, (char*) LSHeader, thisArea->areaID);
                    item = item->next;

                    Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                        thisArea->areaID);
                    continue;
                }
            }

            item = item->next;
        }


        //Check in router summary LSA list
        item = thisArea->routerSummaryLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //Check for DoNotAge LSA
            if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
            {
                //Check for atleast MaxAge Seconds
                if (((node->getNodeTime() - item->timeStamp) >=
                            OSPFv2_LSA_MAX_AGE)
                     &&
                    (item->unreachableTimeStamp!=0)
                     &&
                     ((node->getNodeTime() - item->unreachableTimeStamp) >=
                            OSPFv2_LSA_MAX_AGE))
                {
                   //Flush this LSA
                    Ospfv2FlushLSA(node, (char*) LSHeader, thisArea->areaID);
                    item = item->next;

                    Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                                thisArea->areaID);
                    continue;
                }
            }

            item = item->next;
        }


        //Check in network Summary LSA List
        item = thisArea->networkSummaryLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //Check for DoNotAge LSA
            if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
            {
                //Check for atleast MaxAge Seconds
                if (((node->getNodeTime() - item->timeStamp) >=
                            OSPFv2_LSA_MAX_AGE)
                     &&
                    (item->unreachableTimeStamp!=0)
                     &&
                     ((node->getNodeTime() - item->unreachableTimeStamp) >=
                            OSPFv2_LSA_MAX_AGE))
                {
                   //Flush this LSA
                    Ospfv2FlushLSA(node, (char*) LSHeader, thisArea->areaID);
                    item = item->next;

                    Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                                thisArea->areaID);
                    continue;
                }
            }

            item = item->next;
        }

        //added on 18 sep 2009 start
        //Check in Group-membership list
        item = thisArea->groupMembershipLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;

            //Check for DoNotAge LSA
            if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
            {
                //Check for atleast MaxAge Seconds
                if (((node->getNodeTime() - item->timeStamp) >=
                            OSPFv2_LSA_MAX_AGE)
                     &&
                    (item->unreachableTimeStamp!=0)
                     &&
                     ((node->getNodeTime() - item->unreachableTimeStamp) >=
                            OSPFv2_LSA_MAX_AGE))
                {
                   //Flush this LSA
                    Ospfv2FlushLSA(node, (char*) LSHeader, thisArea->areaID);
                    item = item->next;

                    Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                                thisArea->areaID);
                    continue;
                }
            }

            item = item->next;
        }
        //added on 18 sep 2009 end
    }

    //added on 18 sep 2009 start
    //Check in AS-external list
    item = ospf->asExternalLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DoNotAge LSA
        if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
        {
            //Check for atleast MaxAge Seconds
            if (((node->getNodeTime() - item->timeStamp) >= OSPFv2_LSA_MAX_AGE)
                 &&
                (item->unreachableTimeStamp!=0)
                 &&
                 ((node->getNodeTime() - item->unreachableTimeStamp) >=
                        OSPFv2_LSA_MAX_AGE))
            {
               //Flush this LSA
                Ospfv2FlushLSA(node, (char*) LSHeader, thisArea->areaID);
                item = item->next;

                Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                            thisArea->areaID);
                continue;
            }
        }

        item = item->next;
    }

    //Check in nssa-external list
    item = ospf->nssaExternalLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DoNotAge LSA
        if (Ospfv2CheckLSAForDoNotAge(ospf, LSHeader->linkStateAge))
        {
            //Check for atleast MaxAge Seconds
            if (((node->getNodeTime() - item->timeStamp) >= OSPFv2_LSA_MAX_AGE)
                 &&
                (item->unreachableTimeStamp!=0)
                 &&
                 ((node->getNodeTime() - item->unreachableTimeStamp) >=
                        OSPFv2_LSA_MAX_AGE))
            {
               //Flush this LSA
                Ospfv2FlushLSA(node, (char*) LSHeader, thisArea->areaID);
                item = item->next;

                Ospfv2RemoveLSAFromLSDB(node, (char*) LSHeader,
                            thisArea->areaID);
                continue;
            }
        }

        item = item->next;
    }
    //added on 18 sep 2009 end
}

//
// NAME         : Ospfv2CheckLSAToFloodOverDC ()
// PURPOSE      : Check whether the LSA should be flooded over demand
//                circuit or notHandle Link State Request packet
// PARAMETERS   ::
// +node           :Node pointer
// +interfaceId    :interface id on which LSA needs to be flooded
// +LSHeader       :LS Header of the LSA to be flooded
// ASSUMPTION   :None.
// RETURN VALUE :BOOL
//
BOOL Ospfv2CheckLSAToFloodOverDC(Node* node,
                                 int interfaceId,
                                 Ospfv2LinkStateHeader* LSHeader)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2LinkStateHeader* oldLSHeader = NULL;

    // Find instance of this LSA contained in local database
    oldLSHeader = Ospfv2LookupLSDB(node,
                                   LSHeader->linkStateType,
                                   LSHeader->advertisingRouter,
                                   LSHeader->linkStateID,
                                   interfaceId);

    if (oldLSHeader)
    {
        if (Ospfv2LSAsContentsChangedForDC(ospf, LSHeader, oldLSHeader))
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
        //if LSA is not already present then returning true
        return TRUE;
    }
}

//
// NAME         : Ospfv2SearchAreaForLSAExceptIndLSAWithDC
// PURPOSE      : Search Area For LSA Except Indication LSA with DC
//                bit as given by dc (parameter)
// PARAMETERS   ::
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// + dc    :  BOOL:contains 0 to search LSA with DC bit 0 and 1 to
//            search LSA with DC bit 1
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
//
BOOL Ospfv2SearchAreaForLSAExceptIndLSAWithDC(Ospfv2Area* area,
                                              BOOL dc)
{
    Ospfv2ListItem* item = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;

    //Check in Network List
    item = area->networkLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }

    //Check in router List
    item = area->routerLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }

    //Check in router summary LSA list
    item = area->routerSummaryLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //as router summary list may contain indication LSA
        //so consider indication LSA
        if (Ospfv2CheckForIndicationLSA((Ospfv2RouterLSA*)item->data) ==
                    FALSE)
        {
            //Check for DC bit as dc(given in parameter)
            if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
            {
                return TRUE;
            }
        }

        item = item->next;
    }

    //Check in network Summary LSA List
    item = area->networkSummaryLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        //Check for DC bit as dc(given in parameter)
        if (Ospfv2OptionsGetDC(LSHeader->options) == dc)
        {
            return TRUE;
        }

        item = item->next;
    }

    return FALSE;
}

//
// NAME         : Ospfv2SearchAreaForIndLSAAndCmpRouterIds()
// PURPOSE      : Search Area For Indication LSA and compare the router Id of
//                its originator with the current router
//                and if an indication LSA
//                is found with greater router id then return TRUE
// PARAMETERS   ::
// + ospf  :  Ospf Data pointer:contains the ospf data for an ospf
//            router
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2SearchAreaForIndLSAAndCmpRouterIds(Ospfv2Data* ospf,
                                              Ospfv2Area* area)
{
    Ospfv2ListItem* item = NULL;
    Ospfv2RouterLSA* LSA = NULL;

    //As indication LSA are ROUTER_SUMMARY LSA so checking in
    //router summary LSA list
    item = area->routerSummaryLSAList->first;
    while (item)
    {
        LSA = (Ospfv2RouterLSA*) item->data;

        //Consider only indication LSA
        if (Ospfv2CheckForIndicationLSA((Ospfv2RouterLSA*)item->data) ==
                    TRUE)
        {
            //Compare the routerId of the originator of this indication LSA
            //with the current routerId and if the former one
            //is greater then return FALSE
            if (LSA->LSHeader.advertisingRouter > ospf->routerID)
            {
                return TRUE;
            }
        }

        item = item->next;
    }

    return FALSE;
}

//
// NAME         : Ospfv2SearchAreaForLSAWithDCOrgByOthers()
// PURPOSE      : Search Area For LSA with DC bit as set by
//                the "dc" parameter
//                and if their are LSA originated by other
//                routers then return TRUE
//                to originate indication LSA
// PARAMETERS   ::
// + ospf  :  Ospf Data pointer:contains the ospf data for an ospf
//            router
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// + dc    :  BOOL:contains 0 to search LSA with DC bit 0 and 1 to
//            search LSA with DC bit 1
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
//
BOOL Ospfv2SearchAreaForLSAWithDCOrgByOthers(Ospfv2Data* ospf,
                                             Ospfv2Area* area,
                                             BOOL dc)
{
    Ospfv2ListItem* item = NULL;
    Ospfv2RouterLSA* LSA = NULL;

    //Check in Network List
    item = area->networkLSAList->first;
    while (item)
    {
        LSA = (Ospfv2RouterLSA*) item->data;

        if (Ospfv2OptionsGetDC(LSA->LSHeader.options) == dc)
        {
            if (LSA->LSHeader.advertisingRouter!= ospf->routerID)
            {
                return TRUE;
            }
        }

        item = item->next;
    }

    //Check in router List
    item = area->routerLSAList->first;
    while (item)
    {
        LSA = (Ospfv2RouterLSA*) item->data;

        if (Ospfv2OptionsGetDC(LSA->LSHeader.options) == dc)
        {
            if (LSA->LSHeader.advertisingRouter != ospf->routerID)
            {
                return TRUE;
            }
        }

        item = item->next;
    }

    //Check in router summary LSA list
    item = area->routerSummaryLSAList->first;
    while (item)
    {
        LSA = (Ospfv2RouterLSA*) item->data;

        if (Ospfv2OptionsGetDC(LSA->LSHeader.options) == dc)
        {
            if (LSA->LSHeader.advertisingRouter != ospf->routerID)
            {
                return TRUE;
            }
        }

        item = item->next;
    }

    item = area->networkSummaryLSAList->first;
    while (item)
    {
        LSA = (Ospfv2RouterLSA*) item->data;

        if (Ospfv2OptionsGetDC(LSA->LSHeader.options) == dc)
        {
            if (LSA->LSHeader.advertisingRouter != ospf->routerID)
            {
                return TRUE;
            }
        }

        item = item->next;
    }

    return FALSE;
}

//
// NAME         : Ospfv2CheckForIndicationLSA()
// PURPOSE      : Check LSA for indication LSA
// PARAMETERS   ::
// + Ospfv2RouterLSA  :  Router LSA
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2CheckForIndicationLSA(Ospfv2RouterLSA* LSA)
{
    char *tmp = (char*)LSA;
    char *metric1;
    char *metric2;
    char *metric3;
    Int32 metric;

    tmp = tmp
          + sizeof(Ospfv2LinkStateHeader)
          + sizeof(NodeAddress)
          + sizeof(char);

    metric1 = (char*)tmp;
    metric2 = (char*)(tmp + sizeof(char));
    metric3 = (char*)(tmp + 2 * sizeof(char));

    metric = ((*metric1 << 16) | (*metric2 << 8) | (*metric3)) & 0xFFFFFF;

    //check for type 4 summary LSA
    if ((LSA->LSHeader.linkStateType == OSPFv2_ROUTER_SUMMARY)
        &&
        (Ospfv2OptionsGetDC(LSA->LSHeader.options) == 0)
        &&
        (metric == OSPFv2_LS_INFINITY)
        &&
        (LSA->LSHeader.linkStateID == LSA->LSHeader.advertisingRouter))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//
// NAME         : Ospfv2OriginateIndicationLSA()
// PURPOSE      : Originate indication LSAs
// PARAMETERS   ::
// +node  :Node pointer
// +thisArea  :Area Pointer in which LSA are to be flooded
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2OriginateIndicationLSA(Node* node,
                                  Ospfv2Area* thisArea)
{
    Int32 metric = 0;
    char metric1;
    char metric2;
    char metric3;
    int index = 0;

    char* LSA = (char*) MEM_malloc(sizeof(Ospfv2LinkStateHeader)
                + sizeof(NodeAddress) + sizeof(int));

    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;

    //Set metric to Infinity
    metric = OSPFv2_LS_INFINITY;

    metric1 = (char) (metric >> 16);
    metric2 = (char) (metric >> 8);
    metric3 = (char) metric;

    Ospfv2CreateIndicationLSHeader(node,
                                   LSHeader);
    //type of the LSA:OSPFv2_ROUTER_SUMMARY,

    index += sizeof(Ospfv2LinkStateHeader);

    memset(&LSA[index], 0, sizeof(NodeAddress));

    index += sizeof(NodeAddress);

    LSA[index++] = 0;

    LSA[index++] = metric1;
    LSA[index++] = metric2;
    LSA[index++] = metric3;

    // Flood LSA
    Ospfv2FloodLSA(node,
                   ANY_INTERFACE,
                   LSA,
                   ANY_DEST,
                   thisArea->areaID);

    MEM_free(LSA);
}


//
// NAME         : Ospfv2CreateIndicationLSHeader()
// PURPOSE      : Create LSHeader to originate indication LSAs
// RFC:1793::SECTION:2.5.2 INDICATING ACROSS AREA BOUNDARIES
// Indication-LSAs are type-4-summary LSAs , listing the area border
// router itself as the described ASBR, with the LSA’s cost set to
// LSInfinity and the DC-bit clear.

// PARAMETERS   ::
// +node        :Node pointer
// +LSHeader    :LSHeader pointer
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2CreateIndicationLSHeader(Node* node,
                                    Ospfv2LinkStateHeader* LSHeader)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    memset(LSHeader, 0, sizeof(Ospfv2LinkStateHeader));
    LSHeader->linkStateAge = 0;
    LSHeader->options = 0x0;

    LSHeader->linkStateType = (char)OSPFv2_ROUTER_SUMMARY;
    LSHeader->linkStateID = ospf->routerID;
    LSHeader->advertisingRouter = ospf->routerID;

    LSHeader->linkStateSequenceNumber =
                (signed int) OSPFv2_INITIAL_SEQUENCE_NUMBER;

    LSHeader->linkStateCheckSum = 0x0;

    LSHeader->length = sizeof(Ospfv2LinkStateHeader)
                       + sizeof(NodeAddress) + sizeof(int);
}

//
// NAME         : Ospfv2IndicatingAcrossAreasForDoNotAge
// PURPOSE      : indicate across area boundaries about DoNotAge
//                incapable routers
// RFC:1793::2.5.1. INDICATING ACROSS AREA BOUNDARIES
// The following two cases summarize the requirements for an
// area border router to originate indication-LSAs:
// (1) Suppose an area border router (Router X) is connected to
//     a regular non-backbone OSPF area (Area A). Furthermore,
//     assume that Area A has LSAs with the DC-bit clear, other
//     than indication-LSAs. Then Router X should originate
//     indication-LSAs into all other directly-connected
//     "regular" areas, including the backbone area, keeping
//     the guidelines of Section 2.5.1.1 in mind.
// (2) Suppose an area border router (Router X) is connected to
//     the backbone OSPF area (Area 0.0.0.0). Furthermore,
//     assume that the backbone has LSAs with the DC-bit clear
//     that are either a) not indication-LSAs or b)
//     indication-LSAs that have been originated by routers
//     other than Router X itself. Then Router X should
//     originate indication-LSAs into all other directlyconnected
//     "regular" non-backbone areas, keeping the
//     guidelines of Section 2.5.1.1 in mind.

// RFC:1793::2.5.1.1. LIMITING INDICATION-LSA ORIGINATION
// To limit the number of indication-LSAs originated, the following
// should be observed by an area border router (router X) when originating
// indication-LSAs.
// (1)Indication-LSAs are not originated into an Area A when A already has
//    LSAs with DC-bit clear other than indication-LSAs.
// (2)If another area border router has originated a indication-LSA into
//    Area A, and that area border router has a higher OSPF Router ID than
//    Router X, then Router X should not originate an
//    indication-LSA into Area A.

// PARAMETERS   ::
// + node       :  Node pointer:contains the node pointer of an ospf
//                 router
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2IndicatingAcrossAreasForDoNotAge(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
            NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    int i = 0;
    int j = 0;
    Ospfv2Area* currentArea = NULL;
    Ospfv2Area* nextArea = NULL;
    BOOL lsaFoundWithDCZero = FALSE;
    BOOL dontGenerateIndLSA = FALSE;

    for (i = 0; i < node->numberInterfaces;)
    {
        currentArea = Ospfv2GetArea(node,
                                    ospf->iface[i].areaId);

        lsaFoundWithDCZero = Ospfv2CheckABRForLSAwithDC(ospf,
                                                        currentArea);

        if (lsaFoundWithDCZero == FALSE)
        {
            //Consider the next interface for next area
            i++;
            continue;
        }
        else //if LSA other than indication LSA found with DC = 0
        {
            //Consider the next area other than of interfcae i
            //for originating indication LSA
            for (j = 0 ; j < node->numberInterfaces ; ++j)
            {
                if (j == i)
                {
                    continue;
                }

                nextArea = Ospfv2GetArea(node,
                                         ospf->iface[j].areaId);

                //If an area is NSSA or stub then don't do anything
                if ((nextArea->isNSSAEnabled == TRUE)     //for NSSA
                    ||
                    (nextArea->transitCapability == FALSE))    //for STUB
                {
                    continue;
                }

                dontGenerateIndLSA =
                    Ospfv2SearchAreaForLSAExceptIndLSAWithDC(
                                        nextArea,
                                        0);  //DC = 0

                if (dontGenerateIndLSA == TRUE)
                {
                    //Do not originate indication LSA for this area
                    continue;
                }

                dontGenerateIndLSA =
                    Ospfv2SearchAreaForIndLSAAndCmpRouterIds(ospf,
                                                             nextArea);

                if (dontGenerateIndLSA == TRUE)
                {
                    //Do not originate indication LSA for this area
                     continue;
                }

                //As the both the above conditions are false
                //so now generate indication LSA
                Ospfv2OriginateIndicationLSA(node,
                                             nextArea);

            }
        }

        break; //from for loop of "i"
    }
}

//
// NAME         : Ospfv2CheckABRForLSAwithDC
// PURPOSE      : Area border router checks the area for LSA with DC
//                bit clear
// PARAMETERS   ::
// + ospf       :  Ospf data pointer:contains the node pointer of an ospf
//                 router
// + currentArea: Area in which LSAs with DC bit clear needs to be searched
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2CheckABRForLSAwithDC(Ospfv2Data* ospf,
                                Ospfv2Area* currentArea)
{
    BOOL lsaFoundWithDCZero = FALSE;

    if (currentArea->areaID != OSPFv2_BACKBONE_AREA)  //Non-Backbone area
    {
        lsaFoundWithDCZero = Ospfv2SearchAreaForLSAExceptIndLSAWithDC(
                                currentArea,
                                0);  //DC = 0
    }
    else  //BackBone Area
    {
        lsaFoundWithDCZero = Ospfv2SearchAreaForLSAWithDCOrgByOthers(
                                 ospf,
                                 currentArea,
                                 0);  //DC = 0
    }

    return lsaFoundWithDCZero;
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2LSAsContentsChanged()
// PURPOSE      :Check contents of two LSA for any mismatch. This is used
//               to dtermine whether we need to recalculate routing table.
// ASSUMPTION   :The body of the LSA is not checked. Always assume they
//               differ.
// RETURN       :TRUE if contents have been changed, FALSE otherwise.
//-------------------------------------------------------------------------//

BOOL Ospfv2LSAsContentsChangedForDC(Ospfv2Data* ospf,
    Ospfv2LinkStateHeader* newLSHeader,
    Ospfv2LinkStateHeader* oldLSHeader)
{
    // If LSA's option field has changed
    if ((Ospfv2OptionsGetDC(newLSHeader->options)!=
        Ospfv2OptionsGetDC(oldLSHeader->options))
        || (Ospfv2OptionsGetExt(newLSHeader->options) !=
        Ospfv2OptionsGetExt(oldLSHeader->options))
        || (Ospfv2OptionsGetMulticast(newLSHeader->options) !=
        Ospfv2OptionsGetMulticast(oldLSHeader->options))
        || (Ospfv2OptionsGetQOS(newLSHeader->options) !=
        Ospfv2OptionsGetQOS(oldLSHeader->options)))
    {
        return TRUE;
    }

    //One or both of the LSA instances has LS age set to
    //MaxAge (or DoNotAge+MaxAge).
    else if ((Ospfv2MaskDoNotAge(ospf, newLSHeader->linkStateAge) ==
                  (OSPFv2_LSA_MAX_AGE / SECOND))
              ||
              (Ospfv2MaskDoNotAge(ospf, oldLSHeader->linkStateAge) ==
                  (OSPFv2_LSA_MAX_AGE / SECOND)))
    {
        return TRUE;
    }
    // Else if length field has changed
    else if (newLSHeader->length != oldLSHeader->length)
    {
        return TRUE;
    }
    // The body of the LSA has changed
    else if (Ospfv2LSABodyChanged(newLSHeader, oldLSHeader))
    {
        return TRUE;
    }
    // vgoudar: change to get newly originated opaque LSAs flooded to
    //          new fully adjacent neighbors
    else if ((newLSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE) &&
             (newLSHeader->linkStateSequenceNumber > oldLSHeader->linkStateSequenceNumber))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#ifdef ADDON_DB
/**
* To get the first node on each partition that is actually running OSPF.
*
* @param partition Data structure containing partition information
* @return First node on the partition that is running OSPF
*/
Node* StatsDBOspfGetInitialNodePointer(PartitionData* partition)
{
    Node* traverseNode = partition->firstNode;
    Ospfv2Data* ospf = NULL;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                    traverseNode, ROUTING_PROTOCOL_OSPFv2);
        if (ospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        // we've found a node that has OSPF on it!
        return traverseNode;
    }

    return NULL;
}


void HandleStatsDBOspfNeighborStateTableInsertion(Node* node)
{
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;
    Ospfv2Data* ospf = NULL;
    StatsDbOspfState stats;
    char ipAddr[MAX_ADDRESS_STRING_LENGTH];
    char destStr[MAX_ADDRESS_STRING_LENGTH];
    char stateStr[MAX_ADDRESS_STRING_LENGTH];
    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                    traverseNode, ROUTING_PROTOCOL_OSPFv2);
        if (ospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        int i;

        // We have to insert the Neighbor State in the database.
        for (i = 0; i < traverseNode->numberInterfaces; i++)
        {
            Ospfv2ListItem* listItem = NULL;
            Ospfv2Neighbor* nbrInfo = NULL;

            if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
            {
                continue;
            }

            listItem = ospf->iface[i].neighborList->first;
            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceAddress(traverseNode, i),
                ipAddr);
            while (listItem)
            {
                nbrInfo = (Ospfv2Neighbor*) listItem->data;

                IO_ConvertIpAddressToString(nbrInfo->neighborIPAddress,
                    destStr);
                Ospfv2GetNeighborStateString(nbrInfo->state, stateStr);
                stats.ipAddr = ipAddr;
                stats.state = stateStr ;
                stats.neighborAddr = destStr;

                // Now we insert the information into the database.
                STATSDB_HandleOspfNeighborStateTableInsert(
                    node,
                    stats);

                listItem = listItem->next;
            }
        }
        traverseNode = traverseNode->nextNodeData;
    }
}

void HandleStatsDBOspfRouterLsaTableInsertion(Node* node)
{
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;
    Ospfv2Data* ospf = NULL;
    Ospfv2ListItem* item = NULL;
    Ospfv2ListItem* areaItem = NULL;
    Ospfv2Area* thisArea = NULL;
    StatsDbOspfLsa routerLsaStats;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    Ospfv2RouterLSA* routerLSA = NULL;
    Ospfv2LinkInfo* linkList = NULL;
    char tempAddr[MAX_STRING_LENGTH];

    int i;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                    traverseNode, ROUTING_PROTOCOL_OSPFv2);
        if (ospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        for (item = ospf->area->first; item; item = item->next)
        {
            thisArea = (Ospfv2Area*) item->data;
            areaItem = thisArea->routerLSAList->first;
            while (areaItem)
            {
                LSHeader = (Ospfv2LinkStateHeader*) areaItem->data;
                IO_ConvertIpAddressToString(
                    LSHeader->advertisingRouter,
                    tempAddr);
                routerLsaStats.advRouterAddr = tempAddr;

                if (LSHeader->linkStateType == OSPFv2_ROUTER)
                {
                    routerLSA = (Ospfv2RouterLSA*) areaItem->data;
                    linkList = (Ospfv2LinkInfo*) (routerLSA + 1);

                    for (i = 0; i < routerLSA->numLinks; i++)
                    {
                        int numTos;
                        char linkID[MAX_ADDRESS_STRING_LENGTH];
                        char linkData[MAX_ADDRESS_STRING_LENGTH];

                        IO_ConvertIpAddressToString(linkList->linkID,
                                                    linkID);
                        IO_ConvertIpAddressToString(linkList->linkData,
                                                    linkData);
                        routerLsaStats.linkId = linkID;
                        routerLsaStats.linkData = linkData;
                        routerLsaStats.cost = (short)linkList->metric;
                        routerLsaStats.areaId = thisArea->areaID;;
                        routerLsaStats.linkType =
                            Ospfv2GetLinkTypeString(linkList->type);

                        //Not changing in stats
                        routerLsaStats.linkStateAge = LSHeader->linkStateAge;
                        STATSDB_HandleOspfRouterLsaTableInsert(
                            traverseNode,
                            routerLsaStats);

                        // Place the linkList pointer properly
                        numTos = linkList->numTOS;
                        linkList = (Ospfv2LinkInfo*)
                            ((QospfPerLinkQoSMetricInfo*)(linkList + 1) +
                            numTos);
                    }
                }
                areaItem = areaItem->next;
            }
        }
        traverseNode = traverseNode->nextNodeData;
    }
}

void HandleStatsDBOspfNetworkLsaTableInsertion(Node* node)
{
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;
    Ospfv2Data* ospf = NULL;
    Ospfv2ListItem* item = NULL;
    Ospfv2ListItem* areaItem = NULL;
    Ospfv2Area* thisArea = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    Ospfv2NetworkLSA* networkLSA = NULL;

    int i;
    char tempAddr[MAX_STRING_LENGTH];
    StatsDbOspfLsa lsaStats;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                    traverseNode, ROUTING_PROTOCOL_OSPFv2);
        if (ospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        for (item = ospf->area->first; item; item = item->next)
        {
            thisArea = (Ospfv2Area*) item->data;
            areaItem = thisArea->networkLSAList->first;
            while (areaItem)
            {
                LSHeader = (Ospfv2LinkStateHeader*) areaItem->data;
                IO_ConvertIpAddressToString(
                    LSHeader->advertisingRouter,
                    tempAddr);

                if (LSHeader->linkStateType == OSPFv2_NETWORK)
                {
                    int numLink;
                    NodeAddress netMask;
                    char netMaskStr[MAX_ADDRESS_STRING_LENGTH];
                    char rtrAddrStr[MAX_ADDRESS_STRING_LENGTH];

                    networkLSA = (Ospfv2NetworkLSA*) areaItem->data;

                    NodeAddress* rtrAddr =
                        ((NodeAddress*) (networkLSA + 1)) + 1;

                    memcpy(&netMask, networkLSA + 1, sizeof(NodeAddress));
                    IO_ConvertIpAddressToString(netMask, netMaskStr);

                    lsaStats.advRouterAddr = tempAddr;
                    lsaStats.areaId = thisArea->areaID;

                    //Not changing in stats
                    lsaStats.linkStateAge = LSHeader->linkStateAge;

                    numLink = (networkLSA->LSHeader.length
                        - sizeof(Ospfv2NetworkLSA) - 4)
                        / (sizeof(NodeAddress));

                    for (i = 0; i < numLink; i++)
                    {
                        IO_ConvertIpAddressToString(rtrAddr[i], rtrAddrStr);
                        lsaStats.routerAddr = rtrAddrStr;
                        STATSDB_HandleOspfNetworkLsaTableInsert(
                            traverseNode,
                            lsaStats);
                    }
                }
                areaItem = areaItem->next;
            }
        }
        traverseNode = traverseNode->nextNodeData;
    }
}

void HandleStatsDBOspfSummaryLsaTableInsertion(Node* node)
{
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;
    Ospfv2Data* ospf = NULL;
    Ospfv2ListItem* item = NULL;
    Ospfv2ListItem* areaItem = NULL;
    Ospfv2Area* thisArea = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;

    char tempAddr[MAX_STRING_LENGTH];
    StatsDbOspfLsa lsaStats;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                    traverseNode, ROUTING_PROTOCOL_OSPFv2);
        if (ospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        for (item = ospf->area->first; item; item = item->next)
        {
            thisArea = (Ospfv2Area*) item->data;
            areaItem = thisArea->routerSummaryLSAList->first;
            while (areaItem)
            {
                LSHeader = (Ospfv2LinkStateHeader*) areaItem->data;
                IO_ConvertIpAddressToString(
                    LSHeader->advertisingRouter,
                    tempAddr);

                if (LSHeader->linkStateType == OSPFv2_ROUTER_SUMMARY)
                {
                    char destAddr[MAX_ADDRESS_STRING_LENGTH];

                    IO_ConvertIpAddressToString(LSHeader->linkStateID,
                        destAddr);

                    lsaStats.linkType = "ROUTER_SUMMARY (4)";
                    lsaStats.advRouterAddr = tempAddr;
                    lsaStats.areaId = thisArea->areaID;
                    //Not changing in stats
                    lsaStats.linkStateAge = LSHeader->linkStateAge;
                    lsaStats.routerAddr = destAddr;
                    STATSDB_HandleOspfSummaryLsaTableInsert(
                            traverseNode,
                            lsaStats);

                }
                areaItem = areaItem->next;
            }
            areaItem = thisArea->networkSummaryLSAList->first;
            while (areaItem)
            {
                LSHeader = (Ospfv2LinkStateHeader*) areaItem->data;
                IO_ConvertIpAddressToString(
                    LSHeader->advertisingRouter,
                    tempAddr);

                if (LSHeader->linkStateType == OSPFv2_NETWORK_SUMMARY)

                {
                    char destAddr[MAX_ADDRESS_STRING_LENGTH];

                    IO_ConvertIpAddressToString(LSHeader->linkStateID,
                        destAddr);

                    lsaStats.linkType = "NETWORK_SUMMARY (3)";
                    lsaStats.advRouterAddr = tempAddr;
                    lsaStats.areaId = thisArea->areaID;
                    lsaStats.linkStateAge = LSHeader->linkStateAge;
                    lsaStats.routerAddr = destAddr;
                    STATSDB_HandleOspfSummaryLsaTableInsert(
                            traverseNode,
                            lsaStats);

                }
                areaItem = areaItem->next;
            }
        }
        traverseNode = traverseNode->nextNodeData;
    }
}

void HandleStatsDBOspfExternalLsaTableInsertion(Node* node)
{
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;
    Ospfv2Data* ospf = NULL;
    Ospfv2ListItem* item = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    Ospfv2ExternalLinkInfo* linkInfo;

    char tempAddr[MAX_STRING_LENGTH];
    StatsDbOspfLsa lsaStats;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                    traverseNode, ROUTING_PROTOCOL_OSPFv2);
        if (ospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        item = ospf->asExternalLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;
            IO_ConvertIpAddressToString(
                LSHeader->advertisingRouter,
                tempAddr);

            if (LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
            {
                char netStr[MAX_STRING_LENGTH];

                linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
                IO_ConvertIpAddressToString(LSHeader->linkStateID, netStr);

                lsaStats.linkType = "AS-EXTERNAL (5)";
                lsaStats.advRouterAddr = tempAddr;

                //Not changing in stats
                lsaStats.linkStateAge = LSHeader->linkStateAge;
                lsaStats.routerAddr = netStr;
                lsaStats.cost = Ospfv2ExternalLinkGetMetric
                    (linkInfo->ospfMetric);

                STATSDB_HandleOspfExternalLsaTableInsert(
                    traverseNode,
                    lsaStats);
            }
            item = item->next;
        }
        // Inserting OSPFv2_ROUTER_NSSA_EXTERNAL into ExternalLSA table
        item = ospf->nssaExternalLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;
            IO_ConvertIpAddressToString(
                LSHeader->advertisingRouter,
                tempAddr);

            if (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
            {
                char netStr[MAX_STRING_LENGTH];

                linkInfo = (Ospfv2ExternalLinkInfo*) (LSHeader + 1);
                IO_ConvertIpAddressToString(LSHeader->linkStateID, netStr);

                lsaStats.linkType = "NSSA-EXTERNAL (7)";
                lsaStats.advRouterAddr = tempAddr;
                lsaStats.linkStateAge = LSHeader->linkStateAge;
                lsaStats.routerAddr = netStr;
                lsaStats.cost = Ospfv2ExternalLinkGetMetric
                    (linkInfo->ospfMetric);

                STATSDB_HandleOspfExternalLsaTableInsert(
                    traverseNode,
                    lsaStats);
            }
            item = item->next;
        }

        traverseNode = traverseNode->nextNodeData;
    }
}

//-------------------------------------------------------------------------//
// NAME: Ospfv2GetLinkTypeString(unsigned char type)
// PURPOSE: Converts the link type for a enum to a string.
// RETURN: string for link type.
//-------------------------------------------------------------------------//

std::string Ospfv2GetLinkTypeString(unsigned char type)
{
    if (type == OSPFv2_POINT_TO_POINT)
    {
        return "OSPFv2_POINT_TO_POINT";
    }
    else if (type == OSPFv2_TRANSIT)
    {
        return "OSPFv2_TRANSIT";
    }
    else if (type == OSPFv2_STUB)
    {
        return "OSPFv2_STUB";
    }
    else if (type == OSPFv2_VIRTUAL)
    {
        return "OSPFv2_VIRTUAL";
    }
    else
    {
        return "UNKNOWN";
    }
}

void HandleStatsDBOspfInterfaceStateTableInsertion(Node* node)
{
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;

    Ospfv2Data* ospf = NULL;

    char netStr[MAX_ADDRESS_STRING_LENGTH];

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
                                    traverseNode, ROUTING_PROTOCOL_OSPFv2);
        if (ospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        int i;

        // We have to insert the Neighbor State in the database.
        for (i = 0; i < traverseNode->numberInterfaces; i++)
        {
            StatsDbOspfState stats;
            if (ospf->iface[i].type == OSPFv2_NON_OSPF_INTERFACE)
            {
                continue;
            }
            IO_ConvertIpAddressToString(
                NetworkIpGetInterfaceAddress(traverseNode, i),
                netStr);

            if (ospf->iface[i].state == S_Down)
            {
                stats.state =  "S_Down";
            }
            else if (ospf->iface[i].state == S_Loopback)
            {
                 stats.state =  "S_Loopback";
            }
            else if (ospf->iface[i].state == S_Waiting)
            {
                 stats.state =  "S_Waiting";
            }
            else if (ospf->iface[i].state == S_PointToPoint)
            {
                 stats.state =  "S_PointToPoint";
            }

            else if (ospf->iface[i].state == S_DROther)
            {
                 stats.state =  "S_DROther";
            }
            else if (ospf->iface[i].state == S_Backup)
            {
                 stats.state =  "S_Backup";
            }
            else if (ospf->iface[i].state == S_DR)
            {
                 stats.state =  "S_DR";
            }
            else
            {
                 stats.state =  "UNKNOWN";
            }


#ifdef ADDON_BOEINGFCS
            if (ospf->iface[i].type == OSPFv2_ROSPF_INTERFACE &&
                RoutingCesRospfIsMobileLeafNode(traverseNode, i))
            {
                stats.mobileLeaf = "Yes";
            }
            else
            {
                stats.mobileLeaf = "No";
            }
#endif
            stats.areaId = ospf->iface[i].areaId;
            stats.ipAddr = netStr;
            stats.neighborAddr = "";

            // Now we insert the information into the database.
            STATSDB_HandleOspfInterfaceStateTableInsert(
                node,
                stats);
        }
        traverseNode = traverseNode->nextNodeData;
    }
}

void HandleStatsDBOspfSummaryTableInsertion(Node* node)
{
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;
    int i;
    Ospfv2Interface* thisOspfIntf = NULL;
    Ospfv2Data* ospf = NULL;

    char ipAddr[MAX_ADDRESS_STRING_LENGTH];
    Int64 bytesSent = 0;
    double period = (double) node->getNodeTime() / SECOND;
    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
            traverseNode, ROUTING_PROTOCOL_OSPFv2);
        if (ospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        StatsDbOspfStats stats;
        bytesSent = 0;
        for (i = 0; i < traverseNode->numberInterfaces; i++)
        {
            IO_ConvertIpAddressToString(
                    NetworkIpGetInterfaceAddress(traverseNode, i),
                    ipAddr);
            thisOspfIntf = &ospf->iface[i];
            if (thisOspfIntf != NULL &&
                thisOspfIntf->type != OSPFv2_NON_OSPF_INTERFACE)
            {
                stats.ipAddr = ipAddr;
                stats.numLSUpdateSent =
                    thisOspfIntf->interfaceStats.numLSUpdateSent;
                stats.numLSUpdateBytesSent =
                    thisOspfIntf->interfaceStats.numLSUpdateBytesSent;
                stats.numLSUpdateRecv =
                    thisOspfIntf->interfaceStats.numLSUpdateRecv;
                stats.numLSUpdateBytesRecv =
                    thisOspfIntf->interfaceStats.numLSUpdateBytesRecv;

                stats.numAckSent =
                    thisOspfIntf->interfaceStats.numAckSent;
                stats.numAckBytesSent =
                    thisOspfIntf->interfaceStats.numAckBytesSent;
                stats.numAckRecv =
                    thisOspfIntf->interfaceStats.numAckRecv;
                stats.numAckBytesRecv =
                    thisOspfIntf->interfaceStats.numAckBytesRecv;

                stats.numDDPktSent =
                    thisOspfIntf->interfaceStats.numDDPktSent;
                stats.numDDPktBytesSent =
                    thisOspfIntf->interfaceStats.numDDPktBytesSent;
                stats.numDDPktRecv =
                    thisOspfIntf->interfaceStats.numDDPktRecv;
                stats.numDDPktRxmt =
                    thisOspfIntf->interfaceStats.numDDPktRxmt;
                stats.numDDPktBytesRxmt =
                    thisOspfIntf->interfaceStats.numDDPktBytesRxmt;
                stats.numDDPktBytesRecv =
                    thisOspfIntf->interfaceStats.numDDPktBytesRecv;

                stats.numLSReqSent =
                    thisOspfIntf->interfaceStats.numLSReqSent;
                stats.numLSReqBytesSent =
                    thisOspfIntf->interfaceStats.numLSReqBytesSent;
                stats.numLSReqRecv =
                    thisOspfIntf->interfaceStats.numLSReqRecv;
                stats.numLSReqRxmt =
                    thisOspfIntf->interfaceStats.numLSReqRxmt;
                stats.numLSReqBytesRecv =
                    thisOspfIntf->interfaceStats.numLSReqBytesRecv;

                // Now calculate the offered load and insert in the table
                bytesSent = stats.numLSUpdateBytesSent +
                    stats.numAckBytesSent + stats.numLSReqBytesSent +
                    stats.numDDPktBytesSent;
                stats.offeredLoad = ((double) bytesSent / period) * 8;
                STATSDB_HandleOspfSummaryTableInsert(traverseNode, stats);
            }
        }
        // At this point we have the overall stats per node. Insert into the
        // database now.
        /*bytesSent = stats.numLSUpdateBytesSent +
            stats.numAckBytesSent + stats.numLSReqBytesSent +
            stats.numDDPktBytesSent;
        stats.offeredLoad = ((double) bytesSent / period) * 8;
        STATSDB_HandleOspfSummaryTableInsert(traverseNode, stats);*/
        traverseNode = traverseNode->nextNodeData;
    }
}

static
void HandleStatsDBOspfAggregateStatsCalculation(Node* node,
                                                       StatsDbOspfStats* stats)
{
    Node* traverseNode = node;
    PartitionData* partition = node->partitionData;
    int i;
    Ospfv2Interface* thisOspfIntf = NULL;
    Ospfv2Data* ospf = NULL;

    Int64 bytesSent = 0;
    double period = (double) node->getNodeTime() / SECOND;
    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        ospf = (Ospfv2Data*) NetworkIpGetRoutingProtocol(
            traverseNode, ROUTING_PROTOCOL_OSPFv2);
        if (ospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }
        for (i = 0; i < traverseNode->numberInterfaces; i++)
        {
            thisOspfIntf = &ospf->iface[i];
            if (thisOspfIntf != NULL &&
                thisOspfIntf->type != OSPFv2_NON_OSPF_INTERFACE)
            {
                stats->numLSUpdateSent +=
                    thisOspfIntf->interfaceStats.numLSUpdateSent;
                stats->numLSUpdateBytesSent +=
                    thisOspfIntf->interfaceStats.numLSUpdateBytesSent;
                stats->numLSUpdateRecv +=
                    thisOspfIntf->interfaceStats.numLSUpdateRecv;
                stats->numLSUpdateBytesRecv +=
                    thisOspfIntf->interfaceStats.numLSUpdateBytesRecv;

                stats->numAckSent +=
                    thisOspfIntf->interfaceStats.numAckSent;
                stats->numAckBytesSent +=
                    thisOspfIntf->interfaceStats.numAckBytesSent;
                stats->numAckRecv +=
                    thisOspfIntf->interfaceStats.numAckRecv;
                stats->numAckBytesRecv +=
                    thisOspfIntf->interfaceStats.numAckBytesRecv;

                stats->numDDPktSent +=
                    thisOspfIntf->interfaceStats.numDDPktSent;
                stats->numDDPktBytesSent +=
                    thisOspfIntf->interfaceStats.numDDPktBytesSent;
                stats->numDDPktRecv +=
                    thisOspfIntf->interfaceStats.numDDPktRecv;
                stats->numDDPktRxmt +=
                    thisOspfIntf->interfaceStats.numDDPktRxmt;
                stats->numDDPktBytesRxmt +=
                    thisOspfIntf->interfaceStats.numDDPktBytesRxmt;
                stats->numDDPktBytesRecv +=
                    thisOspfIntf->interfaceStats.numDDPktBytesRecv;

                stats->numLSReqSent +=
                    thisOspfIntf->interfaceStats.numLSReqSent;
                stats->numLSReqBytesSent +=
                    thisOspfIntf->interfaceStats.numLSReqBytesSent;
                stats->numLSReqRecv +=
                    thisOspfIntf->interfaceStats.numLSReqRecv;
                stats->numLSReqRxmt +=
                    thisOspfIntf->interfaceStats.numLSReqRxmt;
                stats->numLSReqBytesRecv +=
                    thisOspfIntf->interfaceStats.numLSReqBytesRecv;
            }
        }
        traverseNode = traverseNode->nextNodeData;
    }
    bytesSent += stats->numLSUpdateBytesSent +
                    stats->numAckBytesSent + stats->numLSReqBytesSent +
                    stats->numDDPktBytesSent + stats->numDDPktBytesRxmt;
    stats->offeredLoad = ((double) bytesSent / period) * 8;
}

void HandleStatsDBOspfAggregateTableInsertion(Node* node)
{
    StatsDb* db = node->partitionData->statsDb;
    StatsDbOspfStats stats;
    HandleStatsDBOspfAggregateStatsCalculation(node, &stats);

#ifdef PARALLEL
    // If we are p0 then insert our stats
    if (node->partitionData->partitionId == 0)
    {
        STATSDB_HandleOspfAggregateTableInsert(node, stats);

        FlushQueryBufferStatsDb(db); // TODO: make this work with interlock
    }

    PARALLEL_SynchronizePartitions(node->partitionData,
                                   BARRIER_TYPE_HandleStatsDBOspfStart);

    // In parallel each partition must take turns updating the table
    for (int i = 1; i < node->partitionData->getNumPartitions(); i++)
    {
        if (i == node->partitionData->partitionId)
        {
            // Update our stats with the existing stats
            STATSDB_HandleOspfAggregateParallelRetreiveExisting(
                                                            node, &stats);

            // Update DB with new params
            STATSDB_HandleOspfAggregateTableUpdate(node, stats);
            
            FlushQueryBufferStatsDb(db); // TODO: make this work with interlock
        }

        PARALLEL_SynchronizePartitions(node->partitionData,
                                       BARRIER_TYPE_HandleStatsDBOspfEnd);
    }
#else
    STATSDB_HandleOspfAggregateTableInsert(node, stats);
#endif
}

void HandleStatsDBMospfSummaryTableInsertion(Node* node)
{
    StatsDb* db = node->partitionData->statsDb;
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;
    MospfData* mospf = NULL;

    // Check if the Table exists.
    if (!db || !db->statsOspfTable ||
        !db->statsOspfTable->createMospfSummaryTable)
    {
        // Table does not exist
        return;
    }

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        mospf = (MospfData*)NetworkIpGetMulticastRoutingProtocol(
                                    traverseNode, MULTICAST_PROTOCOL_MOSPF);

        if (mospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        StatsDBMospfSummary stats;

        stats.m_NumGroupLSAGenerated =
                        mospf->mospfSummaryStats.m_NumGroupLSAGenerated;
        stats.m_NumGroupLSAFlushed =
                        mospf->mospfSummaryStats.m_NumGroupLSAFlushed;
        stats.m_NumGroupLSAReceived =
                        mospf->mospfSummaryStats.m_NumGroupLSAReceived;
        stats.m_NumRouterLSA_WCMRSent =
                        mospf->mospfSummaryStats.m_NumRouterLSA_WCMRSent;
        stats.m_NumRouterLSA_WCMRReceived =
                        mospf->mospfSummaryStats.m_NumRouterLSA_WCMRReceived;
        stats.m_NumRouterLSA_VLEPSent =
                        mospf->mospfSummaryStats.m_NumRouterLSA_VLEPSent;
        stats.m_NumRouterLSA_VLEPReceived =
                        mospf->mospfSummaryStats.m_NumRouterLSA_VLEPReceived;
        stats.m_NumRouterLSA_ASBRSent =
                        mospf->mospfSummaryStats.m_NumRouterLSA_ASBRSent;
        stats.m_NumRouterLSA_ASBRReceived =
                        mospf->mospfSummaryStats.m_NumRouterLSA_ASBRReceived;
        stats.m_NumRouterLSA_ABRSent =
                        mospf->mospfSummaryStats.m_NumRouterLSA_ABRSent;
        stats.m_NumRouterLSA_ABRReceived =
                        mospf->mospfSummaryStats.m_NumRouterLSA_ABRReceived;

        // At this point we have the overall stats per node. Insert into the
        // database now.
        STATSDB_HandleMospfSummaryTableInsert(traverseNode, stats);

        //Init the stats variables again for peg count over time period
        mospf->mospfSummaryStats.m_NumGroupLSAGenerated = 0;
        mospf->mospfSummaryStats.m_NumGroupLSAFlushed = 0;
        mospf->mospfSummaryStats.m_NumGroupLSAReceived = 0;
        mospf->mospfSummaryStats.m_NumRouterLSA_WCMRSent = 0;
        mospf->mospfSummaryStats.m_NumRouterLSA_WCMRReceived = 0;
        mospf->mospfSummaryStats.m_NumRouterLSA_VLEPSent = 0;
        mospf->mospfSummaryStats.m_NumRouterLSA_VLEPReceived = 0;
        mospf->mospfSummaryStats.m_NumRouterLSA_ASBRSent = 0;
        mospf->mospfSummaryStats.m_NumRouterLSA_ASBRReceived = 0;
        mospf->mospfSummaryStats.m_NumRouterLSA_ABRSent = 0;
        mospf->mospfSummaryStats.m_NumRouterLSA_ABRReceived = 0;

        //next node
        traverseNode = traverseNode->nextNodeData;
    }
}

void HandleStatsDBMospfMulticastNetSummaryTableInsertion(Node* node)
{
    StatsDb* db = node->partitionData->statsDb;
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;

    // Check if the Table exists.
    if (!db || !db->statsSummaryTable ||
        !db->statsSummaryTable->createMulticastNetSummaryTable)
    {
        // Table does not exist
        return;
    }

    MospfData* mospf = NULL;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        mospf = (MospfData*)NetworkIpGetMulticastRoutingProtocol(
                                    traverseNode, MULTICAST_PROTOCOL_MOSPF);

        if (mospf == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        StatsDBMulticastNetworkSummaryContent stats;

        strcpy(stats.m_ProtocolType,"MOSPF");

        stats.m_NumDataSent =
                          mospf->mospfMulticastNetSummaryStats.m_NumDataSent;
        stats.m_NumDataRecvd =
                         mospf->mospfMulticastNetSummaryStats.m_NumDataRecvd;
        stats.m_NumDataForwarded =
                     mospf->mospfMulticastNetSummaryStats.m_NumDataForwarded;
        stats.m_NumDataDiscarded =
                     mospf->mospfMulticastNetSummaryStats.m_NumDataDiscarded;

        // At this point we have the overall stats per node. Insert into the
        // database now.
        STATSDB_HandleMulticastNetSummaryTableInsert(traverseNode, stats);

        //Init the stats variables again for peg count over time period
        //strcpy(mospf->mospfMulticastNetSummaryStats.m_ProtocolType,"");
        mospf->mospfMulticastNetSummaryStats.m_NumDataSent = 0;
        mospf->mospfMulticastNetSummaryStats.m_NumDataRecvd = 0;
        mospf->mospfMulticastNetSummaryStats.m_NumDataForwarded = 0;
        mospf->mospfMulticastNetSummaryStats.m_NumDataDiscarded = 0;

        //next node
        traverseNode = traverseNode->nextNodeData;
    }
}
#endif

/***** Start: OPAQUE-LSA *****/
//-------------------------------------------------------------------------//
// NAME         :Ospfv2ScheduleOpaqueLSA()
// PURPOSE      :Schedule Opaque LSA origination
// ASSUMPTION   :None
// PARAMETERS   :node - Node pointer to current node doing the processing
//              :LSALevelStruct - Pointer to structure containing LSDB
//              :LSType - The type of (Opaque) LSA
// RETURN VALUE :None
//-------------------------------------------------------------------------//
void
Ospfv2ScheduleOpaqueLSA(Node* node,
                        Ospfv2LinkStateType LSType)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    Message* newMsg = NULL;
    char* msgInfo = NULL;
    Ospfv2LinkStateType *tmpLSType;
    clocktype delay;

    if (ospf->opaqueCapable == FALSE)
    {
        return;
    }

    if (LSType == OSPFv2_AS_SCOPE_OPAQUE)
    {
        if (ospf->ASOpaqueLSTimer == TRUE)
        {
            // AS-scoped Opaque LSA origination currently in progress
            return;
        }
        // Compute appropriate delay
        else if (ospf->ASOpaqueLSAOriginateTime == 0)
        {
            delay = OSPFv2_MIN_LS_INTERVAL;

            //delay += ospf->staggerStartTime;
        }
        else if ((node->getNodeTime() - ospf->ASOpaqueLSAOriginateTime) >=
                         OSPFv2_MIN_LS_INTERVAL)
        {
            delay = (clocktype)
                    (RANDOM_erand(ospf->seed) * OSPFv2_BROADCAST_JITTER);
        }
        else
        {
            delay = (clocktype) (OSPFv2_MIN_LS_INTERVAL
                    - (node->getNodeTime() - ospf->ASOpaqueLSAOriginateTime));
        }

        // Set origination indicator
        ospf->ASOpaqueLSTimer = TRUE;

        // Construct and send event message
        newMsg = MESSAGE_Alloc(node,
                               NETWORK_LAYER,
                               ROUTING_PROTOCOL_OSPFv2,
                               MSG_ROUTING_OspfScheduleLSDB);

        MESSAGE_InfoAlloc(node,
                          newMsg,
                          sizeof(Ospfv2LinkStateType));
        msgInfo = MESSAGE_ReturnInfo(newMsg);

        tmpLSType = (Ospfv2LinkStateType *)msgInfo;
        *tmpLSType = LSType;

        MESSAGE_Send(node, newMsg, (clocktype) delay);
    }
}

//-------------------------------------------------------------------------//
// NAME         :Ospfv2OriginateType11OpaqueLSA()
// PURPOSE      :Originate Type-11 Opaque LSA
// ASSUMPTION   :None
// PARAMETERS   :node - Node pointer to current node doing the processing
// RETURN VALUE :None
//-------------------------------------------------------------------------//
static void
Ospfv2OriginateType11OpaqueLSA(Node* node)
{
#ifdef CYBER_CORE
#ifdef ADDON_BOEINGFCS
    RoutingCesRospfOriginateHaipeAdvLSA(node);
#endif
#endif
    return;
}
/***** End: OPAQUE-LSA *****/


//-------------------------------------------------------------------------//
// NAME         :Ospfv2IsNbrStateExchangeOrLoading
// PURPOSE      :Checks whether any neighbor is in state Exchange or Loading
// ASSUMPTION   :None.
// PARAMETERS   :node - Node pointer to current node doing the processing
// RETURN VALUE :BOOL.
//-------------------------------------------------------------------------//

BOOL Ospfv2IsNbrStateExchangeOrLoading(Node* node)
{
    BOOL stateFound = FALSE;
    Ospfv2ListItem* tempListItem = NULL;
    Ospfv2Neighbor* tempNeighborInfo = NULL;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    for (int interfaceIndex = 0; interfaceIndex < node->numberInterfaces;
                                                           interfaceIndex++)
    {
        if (NetworkIpGetUnicastRoutingProtocolType(node, interfaceIndex) !=
            ROUTING_PROTOCOL_OSPFv2)
        {
            continue;
        }

        tempListItem = ospf->iface[interfaceIndex].neighborList->first;

        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv2Neighbor*) tempListItem->data;

            ERROR_Assert(tempNeighborInfo,
                "Neighbor not found in the Neighbor list");

            // check if the router's neighbors are in state Exchange/Loading
            if ((tempNeighborInfo->state == S_Exchange) ||
                      (tempNeighborInfo->state == S_Loading))
            {
                stateFound = TRUE;
                return stateFound;
            }
            tempListItem = tempListItem->next;
        }//end of while
    }//end of for

    return stateFound;
}


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2RemoveMaxAgeLSAListFromLSDB()
// LAYER        :: NETWORK
// PURPOSE      :: Remove MaxAge LSAs from LSDB
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// RETURN VALUE :void
//-------------------------------------------------------------------------//
void Ospfv2RemoveMaxAgeLSAListFromLSDB(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    bool scheduledTimer = false;

    if (Ospfv2IsNbrStateExchangeOrLoading(node))
    {
        // reschedule MaxAgeLSA removal timer
        Ospfv2ScheduleMaxAgeLSARemoveTimer(node);
        return;
    }
    else
    {
        ospf->maxAgeLSARemovalTimerSet = FALSE;

        Ospfv2ListItem* item = NULL;
        Ospfv2Area* thisArea = NULL;

        for (int i = 0; i < node->numberInterfaces; i++)
        {
            thisArea = Ospfv2GetArea(node, ospf->iface[i].areaId);

            if (!thisArea)
            {
                continue;
            }

            item = thisArea->maxAgeLSAList->first;

            Ospfv2ListItem* tempItem = NULL;
            while (item)
            {
                unsigned int areaId = 0;
                Ospfv2LinkStateHeader* LSHeader =
                                        (Ospfv2LinkStateHeader*) item->data;

                tempItem = item;
                item = item->next;

                if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL)
                   || (LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL)
                   /***** Start: OPAQUE-LSA *****/
                   || (LSHeader->linkStateType == OSPFv2_AS_SCOPE_OPAQUE))
                   /***** End: OPAQUE-LSA *****/
                {
                    areaId = OSPFv2_INVALID_AREA_ID;
                }
                else
                {
                    areaId = ospf->iface[i].areaId;
                }

                if (Ospfv2IsLSAInNbrRetxList(node, (char*)LSHeader, areaId))
                {
                    // reshedule timer
                    if (scheduledTimer == false)
                    {
                        Ospfv2ScheduleMaxAgeLSARemoveTimer(node);
                        scheduledTimer = true;
                    }
                    continue;
                }

                Ospfv2RemoveLSAFromLSDB(node,
                                        (char*)LSHeader,
                                        areaId);

                Ospfv2RemoveFromList(node,
                                     thisArea->maxAgeLSAList,
                                     tempItem,
                                     FALSE);
            }
            if (thisArea->maxAgeLSAList->size > 0 &&
                ospf->maxAgeLSARemovalTimerSet == FALSE &&
                scheduledTimer == false)
            {
                Ospfv2ScheduleMaxAgeLSARemoveTimer(node);
                scheduledTimer = true;
            }
        }
    }
}


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2LsaIsSelfOriginated()
// LAYER        :: NETWORK
// PURPOSE      :: Check whether LSA is originated by node
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// + LSA        : char* : LSA to be checked if it is originated by node
// RETURN VALUE :: BOOL
//-------------------------------------------------------------------------//
BOOL Ospfv2LsaIsSelfOriginated(Node* node, char* LSA)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;

    if ((LSHeader->advertisingRouter == ospf->routerID) ||
        ((Ospfv2IsMyAddress(node, LSHeader->linkStateID)) &&
         (LSHeader->linkStateType == OSPFv2_NETWORK)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2AddLsaToMaxAgeLsaList()
// LAYER        :: NETWORK
// PURPOSE      :: To add the LSA to maxageLSA list and
//                 call function to schedule MaxAge LSA removal timer
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// + LSA        : char* : LSA to be added in maxAgeLSAList
// + areaId     : unsigned int : area whose maxAge LSAs need to be removed
// RETURN VALUE :: void
//-------------------------------------------------------------------------//
void Ospfv2AddLsaToMaxAgeLsaList(
    Node* node,
    char* LSA,
    unsigned int areaId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);

    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;

    ERROR_Assert(thisArea, "Area doesn't exist\n");
    ERROR_Assert(LSHeader, "LSHeader doesn't exist\n");

    if (!Ospfv2IsPresentInMaxAgeLSAList(
                    node, thisArea->maxAgeLSAList, LSHeader))
    {
        char* data = (char *) MEM_malloc(sizeof(Ospfv2LinkStateHeader));
        memcpy(data, LSHeader, sizeof(Ospfv2LinkStateHeader));

        Ospfv2InsertToList(thisArea->maxAgeLSAList,
                           node->getNodeTime(),
                           (void*) data);

        if (ospf->maxAgeLSARemovalTimerSet == FALSE)
        {
            Ospfv2ScheduleMaxAgeLSARemoveTimer(node);
        }
    }
}


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2ScheduleMaxAgeLSARemoveTimer()
// LAYER        :: NETWORK
// PURPOSE      :: To initiate MaxAge LSA removal timer
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// RETURN VALUE :: void
//-------------------------------------------------------------------------//
void Ospfv2ScheduleMaxAgeLSARemoveTimer(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Message* maxAgeLSARmvTimerMsg = NULL;

    maxAgeLSARmvTimerMsg = MESSAGE_Alloc(node,
                                         NETWORK_LAYER,
                                         ROUTING_PROTOCOL_OSPFv2,
                                         MSG_ROUTING_OspfMaxAgeRemovalTimer);

    ospf->maxAgeLSARemovalTimerSet = TRUE;

    MESSAGE_Send(node,
                 maxAgeLSARmvTimerMsg,
                 OSPFv2_MAXAGE_LSA_REMOVAL_TIME);
}


//-------------------------------------------------------------------------//
// FUNCTION      :: Ospfv2CompareLSAToListItem()
// LAYER         :: NETWORK
// PURPOSE       :: To add the LSA to maxageLSA list and
//                  call function to schedule MaxAge LSA removal timer
// PARAMETERS    ::
// + node        : Node* : Node pointer to current node doing the processing
// + LSHeader    : Ospfv2LinkStateHeader* : LSA to be compared
// + listLSHeader: Ospfv2LinkStateHeader* : LSA in list
// RETURN VALUE  :: BOOL
//-------------------------------------------------------------------------//
BOOL Ospfv2CompareLSAToListItem(
    Node* node,
    Ospfv2LinkStateHeader* LSHeader,
    Ospfv2LinkStateHeader* listLSHeader)
{
#ifdef ADDON_MA
    BOOL  isMatched;
    if ((LSHeader->linkStateType == OSPFv2_ROUTER_AS_EXTERNAL ||
        LSHeader->linkStateType == OSPFv2_ROUTER_NSSA_EXTERNAL) &&
        (listLSHeader->linkStateType == LSHeader->linkStateType))
    {
        if (LSHeader->mask == listLSHeader->mask)
        {
            isMatched = TRUE;
        }
        else
        {
            isMatched = FALSE;
        }
    }
    else
    {
       isMatched = TRUE;
    }

    if ((LSHeader->linkStateType == listLSHeader->linkStateType)
        && (LSHeader->advertisingRouter == listLSHeader->advertisingRouter)
        && (LSHeader->linkStateID == listLSHeader->linkStateID)
        && (!Ospfv2CompareLSA(node, LSHeader, listLSHeader))
        && isMatched)

#else
    if ((LSHeader->linkStateType == listLSHeader->linkStateType)
        && (LSHeader->advertisingRouter == listLSHeader->advertisingRouter)
        && (LSHeader->linkStateID == listLSHeader->linkStateID)
        && (!Ospfv2CompareLSA(node, LSHeader, listLSHeader)))
#endif
    {
        return TRUE;
    }

    return FALSE;
}

//--------------------------------------------------------------------------
// FUNCTION   :: RoutingOspfv2HandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
// PARAMETERS ::
// + node                    : Node*    : Pointer to Node structure
// + interfaceIndex          : Int32    : interface index
// + oldAddress              : Address* : old address
// + NetworkType networkType : type of network protocol
// RETURN     ::             : void     : NULL
//--------------------------------------------------------------------------
void RoutingOspfv2HandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType)
{
    NetworkDataIp* ip = (NetworkDataIp*)node->networkData.networkVar;
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    if (ospf == NULL)
    {
        return;
    }
    IpInterfaceInfoType* interfaceInfo =
        ip->interfaceInfo[interfaceIndex];

    // currently interface address state is invalid, hence no need to update
    // tables as no communication can occur from this interface while the 
    // interface is in invalid address state
    if (interfaceInfo->addressState  == INVALID)
    {
        return;
    }
    if (networkType == NETWORK_IPV6)
    {
        return;
    }

    ospf->iface[interfaceIndex].address = NetworkIpGetInterfaceAddress(node,
                                             interfaceIndex);
    ospf->iface[interfaceIndex].subnetMask =
        NetworkIpGetInterfaceSubnetMask(node, interfaceIndex);

    Ospfv2OriginateRouterLSAForThisArea(node,
                                        ospf->iface[interfaceIndex].areaId,
                                        FALSE);

    Ospfv2ListItem* listItem = NULL;
    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv2Area* thisArea = (Ospfv2Area*)listItem->data;
        Ospfv2List* list = thisArea->networkLSAList;
        Ospfv2ListItem* item = list->first;
        while (item)
        {
            short int tempAge;
            Ospfv2LinkStateHeader* LSHeader =
                (Ospfv2LinkStateHeader*)item->data;
            if (LSHeader->advertisingRouter == ospf->routerID)
            {
                Ospfv2ListItem* tempItem;
                tempItem = item;
                if (interfaceIndex == DEFAULT_INTERFACE)
                {
                    LSHeader->advertisingRouter = 
                        NetworkIpGetInterfaceAddress(
                                                    node,
                                                    interfaceIndex);
                }
                if (ospf->iface[interfaceIndex].state == S_DR)
                {
                    Ospfv2FlushLSA(node, (char*)tempItem, thisArea->areaID);
                    Ospfv2ScheduleNetworkLSA(node, interfaceIndex, FALSE);
                }
            }
            item = item->next;
        }
    }
    if (interfaceIndex == DEFAULT_INTERFACE)
    {
        ospf->routerID = NetworkIpGetInterfaceAddress(
                                                node,
                                                interfaceIndex);
    }
}
