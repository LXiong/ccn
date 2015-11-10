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
*  File: pim.c
*
*
*  PUPPOSE: To simulate Protocol Independent Multicast Routing
*           Protocol. It
*           resides at the network layer just above IP.
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "api.h"
#include "network_ip.h"
#include "list.h"
#include "buffer.h"
#include "multicast_igmp.h"

#include "network_dualip.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#ifdef ADDON_BOEINGFCS
#include "network_ces_inc.h"
#include "network_ces_inc_sincgars.h"
#include "network_ces_inc_eplrs.h"
#include "routing_ces_sdr.h"
#include "multicast_ces_rpim_sm.h"
#include "multicast_ces_rpim_dm.h"
#endif

#include "multicast_pim.h"

#include "partition.h"

#ifdef CYBER_CORE
#include "network_iahep.h"
#endif // CYBER_CORE

#define noDEBUG
#define noDEBUG_ERROR

#define DEBUG_WIRELESS 0
#define DEBUG_DOWN 0
#define DEBUG_BS 0
#define DEBUG_JP 0

#define DEBUG_CONFIG_PARAMS 0
#define DEBUG_TIMER 0
#define DEBUG_HELLO 0

static void RoutingPimRouterFunction(Node* node,
                                     Message* msg,
                                     NodeAddress groupAddr,
                                     int incomingInterface,
                                     BOOL* packetWasRouted,
                                     NodeAddress prevHop);

static void RoutingPimUnicastRouteChange(Node* node,
                                         NodeAddress destAddr,
                                         NodeAddress destAddrMask,
                                         NodeAddress nextHop,
                                         int interfaceId,
                                         int metric,
                              NetworkRoutingAdminDistanceType adminDistance);

static void RoutingPimSmInit(Node* node,
                             int interfaceIndex);

static BOOL RoutingPimIsRPPresent(Node* node,
                                  NodeAddress groupAddr);
/*
* FUNCTION     : getNodeAddressFromCharArray()
* PURPOSE      : Get the value of node address from the atext string
* ASSUMPTION   : None
* RETURN VALUE : NodeAddress
*/
NodeAddress getNodeAddressFromCharArray(char* address)
{
    NodeAddress addr = 0;
    unsigned char *tmpAddr = (unsigned char *) address;
    for (int i = 0; i < 4 ; i++)
    {
        addr = (addr << 8);
        addr += (NodeAddress)(*tmpAddr);
        tmpAddr++;
    }
    return addr;
}

/*
* FUNCTION     : setNodeAddressInCharArray()
* PURPOSE      : Set the node address in a atext string
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void setNodeAddressInCharArray(char* destinationAddr,
                                      NodeAddress srcAddr)
{
    *(destinationAddr) = (char)(srcAddr >> 24);
    *(++destinationAddr) = (char)(srcAddr >> 16);
    *(++destinationAddr) = (char)(srcAddr >> 8);
    *(++destinationAddr) = (char)(srcAddr);
}


/*
* FUNCTION     : RoutingPimCommonHeaderSetVar()
* PURPOSE      : Set the value of var for RoutingPimCommonHeaderType
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void RoutingPimCommonHeaderSetVar(unsigned char *rpChType,
                                         unsigned char var)
{
    //masks var within boundry range
    var = var & maskChar(5, 8);

    //clears first 4 bits
    *rpChType = *rpChType & maskChar(5, 8);

    //setting the value of var in rpChType
    *rpChType = *rpChType | LshiftChar(var, 4);
}

/* These functions are defined for completeness sake but not used */
#if 0
/*
* FUNCTION     : RoutingPimCommonHeaderGetVar()
* PURPOSE      : Returns the value of var for RoutingPimCommonHeaderType
* ASSUMPTION   : None
* RETURN VALUE : unsigned char
*/
static unsigned char RoutingPimCommonHeaderGetVar(unsigned char rpChType)
{
    unsigned char var = rpChType;

    //clears the last 4 bits
    var = var & maskChar(1, 4);

    //right shifts so that last 4 bits represent var
    var = RshiftChar(var, 4);

    return var;
}
#endif

/*
* FUNCTION     : RoutingPimCommonHeaderSetType()
* PURPOSE      : Set the value of type for RoutingPimCommonHeaderType
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void RoutingPimCommonHeaderSetType(unsigned char *rpChType,
                                          unsigned char type)
{
    //masks type within boundry range
    type = type & maskChar(5, 8);

    //clears last 4 bits
    *rpChType = *rpChType & maskChar(1, 4);

    //setting the value of type in rpChType
    *rpChType = *rpChType | type;
}

/*
* FUNCTION     : PimSmRegisterPacketGetNullReg()
* PURPOSE      : Returns the value of nullRegister for
*                RoutingPimSmRegisterPacket
* ASSUMPTION   : None
* RETURN VALUE : BOOL
*/
BOOL PimSmRegisterPacketGetNullReg(UInt32 rpSmRegPkt)
{
    UInt32 nullRegister = rpSmRegPkt;

    //clears all bits except second bit
    nullRegister = nullRegister & maskInt(2, 2);

    //right shift 30 places so that last bit represents nullRegister
    nullRegister = RshiftInt(nullRegister, 2);

    return (BOOL)nullRegister;
}


/*
* FUNCTION     : PimSmRegisterPacketGetReserved()
* PURPOSE      : Returns the value of reserved for RoutingPimSmRegisterPacket
* ASSUMPTION   : None
* RETURN VALUE : UInt32
*/
UInt32 PimSmRegisterPacketGetReserved(UInt32 rpSmRegPkt)
{
    UInt32 reserved = rpSmRegPkt;

    //clears first and second bit
    reserved = reserved & maskInt(3, 32);

    return reserved;
}

/*
* FUNCTION     : PimSmEncodedSourceAddrGetWildCard()
* PURPOSE      : Returns the value of reserved for
*                RoutingPimEncodedSourceAddr
* ASSUMPTION   : None
* RETURN VALUE : BOOL
*/
BOOL PimSmEncodedSourceAddrGetWildCard(unsigned char rpSimEsa)
{
    unsigned char wildCard = rpSimEsa;

    //clears all bits except seventh bit
    wildCard = wildCard & maskChar(7, 7);

    //right shift 1 place so that last bit represents wildCard
    wildCard = RshiftChar(wildCard, 7);

    return (BOOL)wildCard;
}


/*
* FUNCTION     : PimSmEncodedSourceAddrGetRPT()
* PURPOSE      : Returns the value of RPT for
*                RoutingPimEncodedSourceAddr
* ASSUMPTION   : None
* RETURN VALUE : BOOL
*/
BOOL PimSmEncodedSourceAddrGetRPT(unsigned char rpSimEsa)
{
    unsigned char RPT = rpSimEsa;

    //clears all bits except last bit
    RPT = RPT & maskChar(8, 8);

    return (BOOL)RPT;
}

/*
* FUNCTION     : PimSmAssertPacketGetRPTBit()
* PURPOSE      : Returns the value of RPTBit for RoutingPimSmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : BOOL
*/
BOOL PimSmAssertPacketGetRPTBit(UInt32 metricBitPrefSm)
{
    UInt32 RPTBit = metricBitPrefSm;

    //clears all bits except first bit
    RPTBit = RPTBit & maskInt(1, 1);

    //right shift 31 places so that last bit represents RPTBit
    RPTBit = RshiftInt(RPTBit, 1);

    return (BOOL)RPTBit;
}

/*
* FUNCTION     : PimSmEncodedSourceAddrSetReserved()
* PURPOSE      : Set the value of reserved for RoutingPimEncodedSourceAddr
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void PimSmEncodedSourceAddrSetReserved(unsigned char *rpSimEsa,
                                              unsigned char reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskChar(4, 8);

    //clears first 5 bits
    *rpSimEsa = *rpSimEsa & maskChar(6, 8);

    //setting value of reserved in rpSimEsa
    *rpSimEsa = *rpSimEsa | LshiftChar(reserved, 5);
}

/*
* FUNCTION     : PimSmEncodedSourceAddrGetSparseBit()
* PURPOSE      : Returns the value of sparseBit for
*                RoutingPimEncodedSourceAddr
* ASSUMPTION   : None
* RETURN VALUE : BOOL
*/
BOOL PimSmEncodedSourceAddrGetSparseBit(unsigned char rpSimEsa)
{
    unsigned char sparseBit = rpSimEsa;

    //clears all bits except sixth bit
    sparseBit = sparseBit & maskChar(6, 6);

    //right shift 2 places so that last bit represents sparseBit
    sparseBit = RshiftChar(sparseBit, 6);

    return (BOOL)sparseBit;
}
/*
* FUNCTION     : PimSmEncodedSourceAddrSetSparseBit()
* PURPOSE      : Set the value of sparseBit for RoutingPimEncodedSourceAddr
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void PimSmEncodedSourceAddrSetSparseBit(unsigned char *rpSimEsa,
                                               BOOL sparseBit)
{
    unsigned char sparseBit_char =(unsigned char) sparseBit;

    //masks sparseBit within boundry range
    sparseBit_char = sparseBit_char & maskChar(8, 8);

    //clears the sixth bit
    *rpSimEsa = *rpSimEsa & (~(maskChar(6, 6)));

    //setting value of x in rpSimEsa
    *rpSimEsa = *rpSimEsa | LshiftChar(sparseBit_char, 6);
}


/*
* FUNCTION     : PimSmEncodedSourceAddrSetWildCard()
* PURPOSE      : Set the value of wildCard for RoutingPimEncodedSourceAddr
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void PimSmEncodedSourceAddrSetWildCard(unsigned char *rpSimEsa,
                                              BOOL wildCard)
{
    unsigned char x =(unsigned char) wildCard;

    //masks wildCard within boundry range
    x = x & maskChar(8, 8);

    //clears the sixth bit
    *rpSimEsa = *rpSimEsa & (~(maskChar(7, 7)));

    //setting value of x in rpSimEsa
    *rpSimEsa = *rpSimEsa | LshiftChar(x, 7);
}


/*
* FUNCTION     : PimSmEncodedSourceAddrSetRPT()
* PURPOSE      : Set the value of RPT for RoutingPimEncodedSourceAddr
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void PimSmEncodedSourceAddrSetRPT(unsigned char *rpSimEsa, BOOL RPT)
{
    unsigned char x =(unsigned char) RPT;

    //masks RPT within boundry range
    x = x & maskChar(8, 8);

    //clears the sixth bit
    *rpSimEsa = *rpSimEsa & (~(maskChar(8, 8)));

    //setting value of x in rpSimEsa
    *rpSimEsa = *rpSimEsa | x;
}

/*
* FUNCTION     : PimSmRegisterPacketSetBorder()
* PURPOSE      : Set the value of border for RoutingPimSmRegisterPacket
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void PimSmRegisterPacketSetBorder(UInt32 *rpSmRegPkt, BOOL border)
{
    UInt32 x = border;

    //masks border within boundry range
    x = x & maskInt(32, 32);

    //clears first bit
    *rpSmRegPkt = *rpSmRegPkt & (~(maskInt(1, 1)));

    //setting value of x in rpSmRegPkt
    *rpSmRegPkt = *rpSmRegPkt | LshiftInt(x, 1);
}


/*
* FUNCTION     : PimSmRegisterPacketSetNullReg()
* PURPOSE      : Set the value of nullRegister for RoutingPimSmRegisterPacket
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void PimSmRegisterPacketSetNullReg(UInt32 *rpSmRegPkt,
                                          BOOL nullRegister)
{
    UInt32 x = nullRegister;

    //masks nullRegister within boundry range
    x = x & maskInt(32, 32);

    //clears second bit
    *rpSmRegPkt = *rpSmRegPkt & (~(maskInt(2, 2)));

    //setting value of x in rpSmRegPkt
    *rpSmRegPkt = *rpSmRegPkt | LshiftInt(x, 2);
}


/*
* FUNCTION     : PimSmRegisterPacketSetReserved()
* PURPOSE      : Set the value of reserved for RoutingPimSmRegisterPacket
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void PimSmRegisterPacketSetReserved(UInt32 *rpSmRegPkt,
                                           UInt32 reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskInt(3, 32);

    //clears all bits except first two bits
    *rpSmRegPkt = *rpSmRegPkt & maskInt(1, 2);

    //setting value of reserved in rpSmRegPkt
    *rpSmRegPkt = *rpSmRegPkt | reserved;
}

/*
* FUNCTION     : PimSmAssertPacketSetRPTBit()
* PURPOSE      : Set the value of RPTBit for RoutingPimSmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void PimSmAssertPacketSetRPTBit(UInt32 *metricBitPrefSm, BOOL RPTBit)
{
    UInt32 x = RPTBit;

    //masks RPTBit within boundry range
    x = x & maskInt(32, 32);

    //clears first bit
    *metricBitPrefSm = *metricBitPrefSm & (~(maskInt(1, 1)));

    //setting value of x in metricBitPrefSm
    *metricBitPrefSm = *metricBitPrefSm | LshiftInt(x, 1);
}


/*
* FUNCTION     : PimSmAssertPacketSetMetricPref()
* PURPOSE      : Set the value of metricPreference for
*                RoutingPimSmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : void
*/
void PimSmAssertPacketSetMetricPref(UInt32 *metricBitPrefSm,
                                           UInt32 metricPreference)
{
    //masks metricPreference within boundry range
    metricPreference = metricPreference & maskInt(2, 32);

    //clears all bits except first bit
    *metricBitPrefSm = *metricBitPrefSm & maskInt(1, 1);

    //setting value of metricPreference in metricBitPrefSm
    *metricBitPrefSm = *metricBitPrefSm | metricPreference;
}

/*
* FUNCTION     : PimSmAssertPacketGetMetricPref()
* PURPOSE      : Returns the value of metricPreference for
*                RoutingPimSmAssertPacket
* ASSUMPTION   : None
* RETURN VALUE : UInt32
*/
UInt32 PimSmAssertPacketGetMetricPref(UInt32 metricBitPrefSm)
{
    UInt32 metricPreference = metricBitPrefSm;

    //clears first bit
    metricPreference = metricPreference & maskInt(2, 32);

    return metricPreference;
}


/*
* FUNCTION     : RoutingPimCommonHeaderGetType()
* PURPOSE      : Returns the value of version for RoutingPimCommonHeaderType
* ASSUMPTION   : None
* RETURN VALUE : unsigned char
*/
unsigned char RoutingPimCommonHeaderGetType(unsigned char rpChType)
{
    unsigned char type= rpChType;

    //clears the first 4 bits
    type = type & maskChar(5, 8);

    return type;
}

ListItem*
RoutingPimSmIsRPMappingPresent(Node* node,
                               RoutingPimEncodedGroupAddr* grpInfo,
                               RoutingPimEncodedUnicastAddr* rpInfo);
/*-------------------------------------------------------------------------*/
/*           PROTOTYPE FOR INITIALIZING DIFFERENT STRUCTURE                */
/*-------------------------------------------------------------------------*/

static void RoutingPimInitStat(Node* node);

static void RoutingPimInitInterface(Node* node,
                                    const NodeInput* nodeInput,
                                    int interfaceIndex);

/*-------------------------End of function declaration---------------------*/


/*
*  FUNCTION     RoutingPimInit
*  PURPOSE      Initialization function for PIM multicast protocol
*
*  Parameters:
*      node:            Node being initialized
*      nodeInput:       Reference to input file.
*      interfaceIndex:  Interface over which PIM is running
*/
void RoutingPimInit(Node* node,
                    const NodeInput* nodeInput,
                    int interfaceIndex)
{
    /* Allocate PIM layer structure */
    PimData* pim = (PimData*)
            MEM_malloc (sizeof(PimData));
    PimDmData* pimDmData = NULL;
    PimDataSm* pimDataSm = NULL;
    PimDataSmDm* PimSmDmData = NULL;
    NodeAddress interfaceAddress;
    interfaceAddress = NetworkIpGetInterfaceAddress(node,interfaceIndex);

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    clocktype delay;
    Message* newMsg;

    /* Determine PIM routing mode */
    IO_ReadString(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "PIM-ROUTING-MODE",
        &retVal,
        buf);

    if (retVal == FALSE)
    {
        ERROR_ReportError("\"PIM-ROUTING-MODE\" needs to be either DENSE"
                          " or SPARSE or SPARSE-DENSE\n");
    }

    if (strcmp(buf, "DENSE") == 0)
    {
        pim->modeType = ROUTING_PIM_MODE_DENSE;
        pimDmData = (PimDmData*)
                    MEM_malloc (sizeof(PimDmData));

        pim->pimModePtr = (void*) pimDmData;
        RANDOM_SetSeed(pimDmData->seed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_PIM,
                   interfaceIndex);
    }
    else if (strcmp(buf, "SPARSE") == 0)
    {
        pim->modeType = ROUTING_PIM_MODE_SPARSE;
        pimDataSm = (PimDataSm*)
                    MEM_malloc (sizeof(PimDataSm));

        memset(pimDataSm, 0, sizeof(PimDataSm));
        pim->pimModePtr = (void*) pimDataSm;
        RANDOM_SetSeed(pimDataSm->seed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_PIM,
                   interfaceIndex);
    }
    else if (strcmp(buf, "SPARSE-DENSE") == 0)
    {
        /* Init PIM-SMDM structures*/
        pim->modeType = ROUTING_PIM_MODE_SPARSE_DENSE;
        PimSmDmData = (PimDataSmDm*)
                    MEM_malloc (sizeof(PimDataSmDm));
        memset(PimSmDmData, 0, sizeof(PimDataSmDm));

        PimSmDmData->pimSm = (PimDataSm*)
                    MEM_malloc (sizeof(PimDataSm));
        memset(PimSmDmData->pimSm, 0, sizeof(PimDataSm));

        PimSmDmData->pimDm = (PimDmData*)
                    MEM_malloc (sizeof(PimDmData));
        memset(PimSmDmData->pimSm, 0, sizeof(PimDmData));

        pim->pimModePtr = (void*) PimSmDmData;
        RANDOM_SetSeed(PimSmDmData->pimDm->seed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_PIM,
                   interfaceIndex);
        RANDOM_SetSeed(PimSmDmData->pimSm->seed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_PIM,
                   interfaceIndex);
    }
    else
    {
        ERROR_ReportError("\"PIM-ROUTING-MODE needs to be either DENSE"
                          " or SPARSE or SPARSE-DENSE\n");
    }

    RANDOM_SetSeed(pim->seed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_PIM,
                   interfaceIndex);

    if (ip->ipForwardingEnabled == TRUE)
    {
        NetworkIpAddToMulticastGroupList(node, ALL_PIM_ROUTER);
#ifdef CYBER_CORE
        if (ip->isIgmpEnable == TRUE)
        {
            if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
            {
                IgmpJoinGroup(node, IAHEPGetIAHEPRedInterfaceIndex(node),
                    ALL_PIM_ROUTER, (clocktype)0);

                if (IAHEP_DEBUG)
                {
                    char addrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(ALL_PIM_ROUTER, addrStr);
                    printf("\nRed Node: %d", node->nodeId);
                    printf("\tJoins Group: %s\n", addrStr);
                }
            }
        }
#endif
    }

    /* Set Multicast Routing Protocol */
    NetworkIpSetMulticastRoutingProtocol(node, pim, interfaceIndex);

    /* Inform IGMP about multicast routing protocol */
    if (ip->isIgmpEnable == TRUE
        && !TunnelIsVirtualInterface(node, interfaceIndex))
    {
        IgmpSetMulticastProtocolInfo(node, interfaceIndex,
                &RoutingPimLocalMembersJoinOrLeave);
    }


    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        /* Set Router function */
        NetworkIpSetMulticastRouterFunction(node,
            &RoutingPimDmRouterFunction, interfaceIndex);


        /* Set funtion pointer to get informed when route changed */
        NetworkIpSetRouteUpdateEventFunction(node,
                &RoutingPimDmAdaptUnicastRouteChange);

         /* Initialize forwarding table */
        RoutingPimDmInitForwardingTable(node);

        #ifdef DEBUG
        {
            FILE* fp;
            fp = fopen("FWDTable.txt", "w");

            if (fp == NULL)
            {
                fprintf(stdout, "Couldn't open file FWDTable.txt\n");
            }
        }
        #endif

    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
#ifdef DEBUG
        printf("Initing SM-DM router,URC& DM FW table\n");
#endif
        /* Set Router function */
        NetworkIpSetMulticastRouterFunction(node,
            &RoutingPimRouterFunction, interfaceIndex);

        /* Set funtion pointer to get informed when route changed */
        NetworkIpSetRouteUpdateEventFunction(node,
                &RoutingPimUnicastRouteChange);

         /* Initialize forwarding table */
        RoutingPimDmInitForwardingTable(node);

#ifdef DEBUG
        {
            FILE* fp;
            fp = fopen("FWDTable.txt", "w");

            if (fp == NULL)
            {
                fprintf(stdout, "Couldn't open file FWDTable.txt\n");
            }
        }
#endif
        /* SM INIT */
        /* Initialize forwarding table & treeInfo Base*/
#ifdef DEBUG
        printf("Initing SM router,URC&FW table\n");
#endif
        RoutingPimSmInitForwardingTable(node);
        RoutingPimSmInitTreeInfoBase(node);

#ifdef ADDON_BOEINGFCS
        /* Initialize Group Membership State Counters */
        RoutingPimSmInitGroupMemStateCounters(node);
#endif // ADDON_BOEINGFCS

        RoutingPimSmInit(node,interfaceIndex);
        /* Initializes statistics */
        RoutingPimInitStat(node);

        /* Initializes interface structure */
        RoutingPimInitInterface(node,
                                nodeInput,
                                interfaceIndex);

#ifdef ADDON_BOEINGFCS
        MulticastCesRpimReadConfigParams(node, interfaceIndex);
#endif

#ifdef DEBUG
        {
            unsigned int i;
            printf("Node %u: \n", node->nodeId);

            for (i = 0; i < (unsigned int)node->numberInterfaces; i++)
            {
                NodeAddress addr = NetworkIpGetInterfaceAddress(node, i);

                printf("IP of interface %d = %u \n", i, addr);
            }
            printf("\n");
        }
#endif
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        /* Set Router function */
        NetworkIpSetMulticastRouterFunction(node,
            &RoutingPimSmForwardingFunction,
            interfaceIndex);

        /* Set funtion pointer to get informed when route changed */
        NetworkIpSetRouteUpdateEventFunction(node,
                &RoutingPimSmProcessUnicastRouteChange);

        /* Initialize forwarding table & treeInfo Base*/
        RoutingPimSmInitForwardingTable(node);
        RoutingPimSmInitTreeInfoBase(node);

#ifdef ADDON_BOEINGFCS
        /* Initialize Group Membership State Counters */
        RoutingPimSmInitGroupMemStateCounters(node);
#endif // ADDON_BOEINGFCS

#ifdef DEBUG
        {
            FILE* fp;

            fp = fopen("TreeInfoBase.txt", "w");

            if (fp == NULL)
            {
                fprintf(stdout, "Couldn't open file TreeInfoBase.txt\n");
            }
        }
#endif
        RoutingPimSmInit(node,interfaceIndex);
        /* Initializes statistics */
        RoutingPimInitStat(node);

        /* Initializes interface structure */
        RoutingPimInitInterface(node,
                                nodeInput,
                                interfaceIndex);

#ifdef ADDON_BOEINGFCS
        MulticastCesRpimReadConfigParams(node, interfaceIndex);
#endif

#ifdef DEBUG
        {
            unsigned int i;
            printf("Node %u: \n", node->nodeId);

            for (i = 0; i < (unsigned int)node->numberInterfaces; i++)
            {
                NodeAddress addr = NetworkIpGetInterfaceAddress(node, i);

                printf("IP of interface %d = %u \n", i, addr);
            }
            printf("\n");
        }
#endif
    }


    /* Determine whether or not to print the stats at end of simulation. */
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTING-STATISTICS",
        &retVal,
        buf);

    if ((retVal == FALSE) || (strcmp (buf, "NO") == 0))
    {
        pim->showStat = FALSE;
    }
    else if (strcmp (buf, "YES") == 0)
    {
        pim->showStat = TRUE;
    }

    pim->statPrinted = FALSE;


    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
    /* Initializes statistics */
    RoutingPimInitStat(node);

    /* Initializes interface structure */
    RoutingPimInitInterface(node,
                            nodeInput,
                            interfaceIndex);
#ifdef ADDON_BOEINGFCS
    RPimDmInit(node,
               interfaceIndex,
               nodeInput);

    if ((pim->interface[interfaceIndex].interfaceType ==
            MULTICAST_CES_RPIM_INTERFACE) &&
            (pim->modeType == ROUTING_PIM_MODE_DENSE))
        {
            if ((NetworkCesIncSincgarsActiveOnInterface(node, interfaceIndex) &&
                RoutingCesSdrActiveOnInterface(node, interfaceIndex)))
            {
                char errBuf[MAX_STRING_LENGTH];
                sprintf(errBuf, "Warning: Node %d interface %d "
                    "is running SINCGARS, which provides its own "
                    "multicast capabilities. Using RPIM-DM may unnecessarily "
                    "increase the load on the network." ,
                    node->nodeId, interfaceIndex);

                ERROR_ReportWarning(errBuf);
            }

            if (NetworkCesIncEplrsActiveOnInterface(node, interfaceIndex))
            {
                char errBuf[MAX_STRING_LENGTH];
                sprintf(errBuf, "Warning: Node %d interface %d "
                    "is running EPLRS, which provides its own "
                    "multicast capabilities. Using RPIM-DM may unnecessarily "
                    "increase the load on the network." ,
                    node->nodeId, interfaceIndex);

                ERROR_ReportWarning(errBuf);
            }
        }
#endif
    }

    /*
    *  Every ROUTING_PIM_HELLO_PERIOD (30) seconds a router should send out
    *  Hello packet on all of its local interfaces. Initiate the messaging
    *  system at the start of simulation. At startup the timer should be set
    *  between 0 and ROUTING_PIM_TRIGGERED_HELLO_DELAY.
    */

    if (ip->ipForwardingEnabled == TRUE)
    {
        for (int i = 0; i < node->numberInterfaces; i++)
        {
            /* Skip those interfaces that are not PIM enabled */
            if (!RoutingPimIsPimEnabledInterface(node, i))
            {
                continue;
            }

            delay = (clocktype) (RANDOM_nrand(pim->seed)
                                        % ROUTING_PIM_TRIGGERED_HELLO_DELAY);
            newMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    MULTICAST_PROTOCOL_PIM,
                                    MSG_ROUTING_PimScheduleHello);

            MESSAGE_AddInfo(node, newMsg, sizeof(int), INFO_TYPE_PhyIndex);

            memcpy(MESSAGE_ReturnInfo(newMsg, INFO_TYPE_PhyIndex),
                   &i,
                   sizeof(int));

#ifdef DEBUG
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char delayStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(delay,
                                        delayStr);

                printf("Node %d @ %s: At Init, Scheduling "
                       "MSG_ROUTING_PimScheduleHello "
                       "for interface %d with delay %s\n",
                                 node->nodeId,
                                 clockStr,
                                 i,
                                 delayStr);
            }
#endif
            MESSAGE_Send(node, newMsg, delay);
        }
    }
}


/*
*  FUNCTION     :RoutingPimLocalMembersJoinOrLeave()
*  PURPOSE      :Handle local group membership events
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimLocalMembersJoinOrLeave(Node* node,
                                       NodeAddress groupAddr,
                                       int interfaceId,
                                       LocalGroupMembershipEventType event)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        RoutingPimDmLocalMembersJoinOrLeave(node,
                                        groupAddr,
                                        interfaceId,
                                        event);
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        RoutingPimSmLocalMembersJoinOrLeave(node,
                                        groupAddr,
                                        interfaceId,
                                        event);

    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        RoutingPimSmLocalMembersJoinOrLeave(node,
                                                groupAddr,
                                                interfaceId,
                                                event);

        RoutingPimDmLocalMembersJoinOrLeave(node,
                                                groupAddr,
                                                interfaceId,
                                                event);
    }
}


/* FUNCTION     :RoutingPimHandleProtocolEvent()
*  PURPOSE      :Handle protocol event
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/

void RoutingPimHandleProtocolEvent(Node* node, Message* msg)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    RoutingPimDmInterface* dmInterface = NULL;
    RoutingPimSmInterface* smInterface = NULL;

    switch (msg->eventType)
    {
        /* Time to send Hello packet */
        case MSG_ROUTING_PimScheduleHello:
        {
            Message* newMsg;
            int interfaceIndex;
            PimDmData* pimDmData = NULL;

            memcpy(&interfaceIndex,
                   MESSAGE_ReturnInfo(msg, INFO_TYPE_PhyIndex),
                   sizeof(int));

            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                dmInterface = pim->interface[interfaceIndex].dmInterface;
                smInterface = pim->interface[interfaceIndex].smInterface;
                pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
                dmInterface = pim->interface[interfaceIndex].dmInterface;
                pimDmData = (PimDmData*)pim->pimModePtr;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                smInterface = pim->interface[interfaceIndex].smInterface;
            }

            /* Reschedule Hello packet broadcast */
            newMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    MULTICAST_PROTOCOL_PIM,
                                    MSG_ROUTING_PimScheduleHello);

            MESSAGE_AddInfo(node, newMsg, sizeof(int), INFO_TYPE_PhyIndex);

            memcpy(MESSAGE_ReturnInfo(newMsg, INFO_TYPE_PhyIndex),
                   &interfaceIndex,
                   sizeof(int));

            if (dmInterface
                &&
                pim->interface[interfaceIndex].helloSuppression == FALSE)
            {
                /* Send Hello packet on selected interfaces */
                RoutingPimSendHelloPacket(node,
                    interfaceIndex);

                if (DEBUG_HELLO)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char delayStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(dmInterface->helloInterval,
                                            delayStr);

                    printf("Node %d @ %s: Scheduling "
                           "MSG_ROUTING_PimScheduleHello "
                           "for interface %d with delay %s\n",
                            node->nodeId,
                            clockStr,
                            interfaceIndex,
                            delayStr);
                }

                MESSAGE_Send(node,
                    newMsg,
                    dmInterface->helloInterval);
            }
            else if (dmInterface
                    &&
                    pim->interface[interfaceIndex].helloSuppression == TRUE)
                 {
                     RoutingPimGetNeighborFromIPTable(node,
                                                       interfaceIndex);

                      MESSAGE_Send(node,
                                   newMsg,
                                   dmInterface->helloInterval);
                 }
                 else if (smInterface)
                 {
                     /* Send Hello packet on selected interfaces */
                     RoutingPimSendHelloPacket(node,
                                                interfaceIndex);

                     if (DEBUG_HELLO)
                     {
                         char clockStr[MAX_STRING_LENGTH];
                         TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                         char delayStr[MAX_STRING_LENGTH];
                         TIME_PrintClockInSecond(smInterface->helloInterval,
                                                 delayStr);
    
                         printf("Node %d @ %s: Scheduling "
                                "MSG_ROUTING_PimScheduleHello "
                                "for interface %d with delay %s\n",
                                 node->nodeId,
                                 clockStr,
                                 interfaceIndex,
                                 delayStr);
                     }

                     MESSAGE_Send(node,
                                  newMsg,
                                  smInterface->helloInterval);
                 }
            break;
        }
        case MSG_ROUTING_PimDmNeighborTimeOut:
        {
            RoutingPimDmNbrTimerInfo* nbrTimer;
            RoutingPimNeighborListItem* neighbor;
            RoutingPimInterface* thisInterface;

            nbrTimer = (RoutingPimDmNbrTimerInfo*) MESSAGE_ReturnInfo(msg);
            thisInterface = &pim->interface[nbrTimer->interfaceIndex];

            /* Search neighbor in neighbor list */
            RoutingPimSearchNeighborList(
                thisInterface->dmInterface->neighborList,
                                         nbrTimer->nbrAddr,
                                         &neighbor);

            if (!neighbor)
            {
                break;
            }

            if ((node->getNodeTime() >= neighbor->NLTTimer)
                     && (neighbor->holdTime != (unsigned short)
                     ROUTING_PIM_INFINITE_HOLDTIME))
            {
                if (DEBUG_TIMER)
                {
                    char neighborStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(nbrTimer->nbrAddr,
                                            neighborStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout,
                            "Node %u @ %s: Timer "
                                   "MSG_ROUTING_PimDmNeighborTimeOut expires "
                                   "for neighbor %s\n",
                                   node->nodeId,
                                   clockStr,
                                   neighborStr);
                }

                neighbor->NLTTimer = 0;
                /* Delete Pim Neighbor from list */
                RoutingPimDmDeleteNeighbor(node, nbrTimer->nbrAddr,
                        nbrTimer->interfaceIndex);
            }
            else
            {
                clocktype delay = neighbor->NLTTimer - node->getNodeTime();
                RoutingPimDmNbrTimerInfo newNbrTimer;
                newNbrTimer.interfaceIndex = nbrTimer->interfaceIndex;
                newNbrTimer.nbrAddr = nbrTimer->nbrAddr;

                RoutingPimSetTimer(node,
                                   newNbrTimer.interfaceIndex,
                                   MSG_ROUTING_PimDmNeighborTimeOut,
                                   (void*) &newNbrTimer,
                                   delay);

                neighbor->NLTTimer = node->getNodeTime() + delay;
            }
            break;
        }

        case MSG_ROUTING_PimDmPruneTimeoutAlarm:
        {
            RoutingPimDmPruneTimerInfo* pruneTimer;
            RoutingPimDmForwardingTableRow* rowPtr;
            RoutingPimDmDownstreamListItem* downstreamInfo;
            int interfaceId;

            pruneTimer = (RoutingPimDmPruneTimerInfo*)
                                 MESSAGE_ReturnInfo(msg);

            /* Search for matching entry */
            rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node,
                             pruneTimer->srcAddr, pruneTimer->grpAddr);

            if (rowPtr == NULL)
            {
                break;
            }

            interfaceId = NetworkIpGetInterfaceIndexFromAddress(node,
                                  pruneTimer->downstreamIntf);
            downstreamInfo = RoutingPimDmGetDownstreamInfo(rowPtr,
                interfaceId);
            if (downstreamInfo == NULL)
            {
                break;
            }

            if ((downstreamInfo->isPruned)
                && ((node->getNodeTime() - downstreamInfo->lastPruneReceived) >=
                    (downstreamInfo->pruneTimer)))
            {
                if (DEBUG_TIMER)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(pruneTimer->srcAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(pruneTimer->grpAddr,
                                                grpStr);
                    char intfAddrStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(downstreamInfo->intfAddr,
                                            intfAddrStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout,"Node %u @ %s: Timer "
                                   "MSG_ROUTING_PimDmPruneTimeoutAlarm "
                                   "expires for (%s , %s) for downstream "
                                   "interface %d(%s)\n",
                                   node->nodeId,
                                   clockStr,
                                   srcStr,
                                   grpStr,
                                   downstreamInfo->interfaceIndex,
                                   intfAddrStr);
                }

                /* Start sending data packet out this interface */
                downstreamInfo->isPruned = FALSE;

                if (rowPtr->state == ROUTING_PIM_DM_PRUNE)
                {
                    RoutingPimDmDataTimeoutInfo cacheTimer;

                    /* Send graft to upstream */
                    rowPtr->state = ROUTING_PIM_DM_ACKPENDING;
                    RoutingPimDmSendGraftPacket(node, rowPtr->srcAddr,
                            rowPtr->grpAddr);

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
                    /* Set DataTimeout timer */
                    rowPtr->expirationTimer = node->getNodeTime() +
                        ROUTING_PIM_DM_DATA_TIMEOUT;
                }
            }

            break;
        }

        case MSG_ROUTING_PimDmAssertTimeOut:
        {
            RoutingPimDmAssertTimerInfo* assertTimer = NULL;
            RoutingPimDmForwardingTableRow* rowPtr = NULL;
            RoutingPimDmDownstreamListItem* downstreamInfo = NULL;

            int interfaceIndex = 0;
            clocktype assertTime = 0;
            RoutingPimAssertState assertState = Pim_Assert_NoInfo;
            BOOL assertTimerRunning = FALSE;

            memcpy(&interfaceIndex,
               MESSAGE_ReturnInfo(msg, INFO_TYPE_PhyIndex),
               sizeof(int));

            assertTimer = (RoutingPimDmAssertTimerInfo*)
                                   MESSAGE_ReturnInfo(msg);

            if (pim->interface[interfaceIndex].assertOptimization == FALSE)
            {
                /* Search for matching entry */
                rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node,
                                 assertTimer->srcAddr, assertTimer->grpAddr);
                 if (rowPtr == NULL)
                {
                    break;
                }
    
                if (rowPtr->inIntf == NetworkIpGetInterfaceAddress(node,
                                        interfaceIndex))
                {
                    assertTime = rowPtr->assertTime;
                    assertState = rowPtr->assertState;
                    assertTimerRunning = rowPtr->assertTimerRunning;
                }
                else if (RoutingPimDmIsDownstreamInterface(rowPtr,
                                                          interfaceIndex))
                {
                    downstreamInfo = RoutingPimDmGetDownstreamInfo(
                                        rowPtr,
                                        interfaceIndex);
                    assertTime = downstreamInfo->assertTime;
                    assertState = downstreamInfo->assertState;
                    assertTimerRunning = downstreamInfo->assertTimerRunning;
                }
                else
                {
                    break;
                }
    
                if (assertTimerRunning == FALSE)
                {
                    break;
                }
    
                if (node->getNodeTime() >= assertTime)
                {
                    if (DEBUG_TIMER)
                    {
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(assertTimer->srcAddr,
                                                    srcStr);
                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(assertTimer->grpAddr,
                                                    grpStr);
                        char intfAddrStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(NetworkIpGetInterfaceAddress(
                                                node, interfaceIndex),
                                                intfAddrStr);
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
    
                        fprintf(stdout,"Node %u @ %s: Timer "
                                       "MSG_ROUTING_PimDmAssertTimeOut "
                                       "expires for (%s , %s) for "
                                       "interface %d(%s)\n",
                                       node->nodeId,
                                       clockStr,
                                       srcStr,
                                       grpStr,
                                       interfaceIndex,
                                       intfAddrStr);
                    }
    
                    RoutingPimAssertState oldState = assertState;
    
                    //RFC 3963 (Section 4.6.4.2 and Section 4.6.4.3):
                    //The Assert state machine MUST transition to NoInfo state
                    if (rowPtr->inIntf == NetworkIpGetInterfaceAddress(node,
                                            interfaceIndex))
                    {
                        rowPtr->assertTime = 0;
                        rowPtr->assertState = Pim_Assert_NoInfo;
    
                        rowPtr->preference = ROUTING_ADMIN_DISTANCE_DEFAULT;
                        rowPtr->metric = ROUTING_PIM_INFINITE_METRIC;
    
                        }
                    else
                    {
                        downstreamInfo->assertTime = 0;
                        downstreamInfo->assertState = Pim_Assert_NoInfo;
    
                        downstreamInfo->winnerAssertMetric.metricPreference =
                            ROUTING_ADMIN_DISTANCE_DEFAULT;
                        downstreamInfo->winnerAssertMetric.metric =
                            ROUTING_PIM_INFINITE_METRIC;
    
                        if (oldState == Pim_Assert_ILostAssert &&
                            rowPtr->state == ROUTING_PIM_DM_PRUNE)
                        {
                            //Change the upstream state so that data packets
                            //can arrive leading to change in the assert state
    
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
                                    interfaceIndex,
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
                else
                {
                    clocktype delay = assertTime - node->getNodeTime();
                    RoutingPimDmAssertTimerInfo newAssertTimer;
                    newAssertTimer.srcAddr = assertTimer->srcAddr;
                    newAssertTimer.grpAddr = assertTimer->grpAddr;
    
                    RoutingPimSetTimer(node,
                                       interfaceIndex,
                                       MSG_ROUTING_PimDmAssertTimeOut,
                                       (void*) &newAssertTimer,
                                       delay);
    
                    assertTime = node->getNodeTime() + delay;
    
                    if (rowPtr->inIntf ==
                        NetworkIpGetInterfaceAddress(node,
                                                     interfaceIndex))
                    {
                        rowPtr->assertTime = assertTime;
                        rowPtr->assertTimerRunning = TRUE;
                    }
                    else if (RoutingPimDmIsDownstreamInterface(rowPtr,
                                                              interfaceIndex))
                    {
                        downstreamInfo->assertTime = assertTime;
                        downstreamInfo->assertTimerRunning = TRUE;
                    }
                }
            }
            else
            {
                RoutingPimDmAssertSrcListItem* assertSrcListItem
                            = RoutingPimDmSearchAssertSrcPair(
                                            node,
                                            assertTimer->srcAddr,
                                            interfaceIndex);

                if (assertSrcListItem == NULL)
                {
                    if (DEBUG_WIRELESS)
                    {
                        ERROR_Assert(FALSE,"\nentry must be present\n");
                    }

                    break;
                }

                if (assertTimerRunning == FALSE)
                {
                    break;
                }

                assertTime = assertSrcListItem->assertTime;
                assertState = assertSrcListItem->assertState;

                if (node->getNodeTime() >= assertTime)
                {
                    RoutingPimAssertState oldState = assertState;
                    GrpSetIterator setIterator;

                    for (setIterator = assertSrcListItem->grpSet->begin();
                         setIterator != assertSrcListItem->grpSet->end();
                         setIterator++)
                    {
                        rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                                                        node,
                                                        assertTimer->srcAddr,
                                                        *setIterator);
                        if (rowPtr == NULL)
                        {
                            break;
                        }

                        if (assertSrcListItem->isUpstream
                            &&
                            (assertSrcListItem->interfaceId == interfaceIndex))
                        {
                            assertSrcListItem->assertTime = 0;
                            assertSrcListItem->assertState = Pim_Assert_NoInfo;
        
                            assertSrcListItem->preference = ROUTING_ADMIN_DISTANCE_DEFAULT;
                            assertSrcListItem->metric = ROUTING_PIM_INFINITE_METRIC;
                        }
                        else
                        {
                            assertSrcListItem->assertTime = 0;
                            assertSrcListItem->assertState = Pim_Assert_NoInfo;
        
                            assertSrcListItem->winnerAssertMetric.metricPreference =
                                ROUTING_ADMIN_DISTANCE_DEFAULT;
                            assertSrcListItem->winnerAssertMetric.metric =
                                ROUTING_PIM_INFINITE_METRIC;

                            if (oldState == Pim_Assert_ILostAssert &&
                                rowPtr->state == ROUTING_PIM_DM_PRUNE)
                            {
                                //Change the upstream state so that data packets
                                //can arrive leading to change in the assert state

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
                                        interfaceIndex,
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

            }

            break;
        }

        case MSG_ROUTING_PimDmDataTimeOut:
        {
            RoutingPimDmDataTimeoutInfo* exprTimer;
            RoutingPimDmForwardingTableRow* rowPtr;

            exprTimer = (RoutingPimDmDataTimeoutInfo*)
                                MESSAGE_ReturnInfo(msg);

            /* Search for matching entry */
            rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node,
                             exprTimer->srcAddr, exprTimer->grpAddr);

            if (rowPtr == NULL)
            {
                break;
            }

            if (node->getNodeTime() >= rowPtr->expirationTimer)
            {
                if (DEBUG_TIMER)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                                grpStr);
                    char intfAddrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->inIntf,
                                                intfAddrStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout,"Node %u @ %s: Timer "
                                   "MSG_ROUTING_PimDmDataTimeOut "
                                   "expires for (%s , %s) for "
                                   "interface %d(%s)\n",
                                   node->nodeId,
                                   clockStr,
                                   srcStr,
                                   grpStr,
                                   NetworkIpGetInterfaceIndexFromAddress(
                                                    node,
                                                    rowPtr->inIntf),
                                   intfAddrStr);
                }


                BOOL pruneExpire = FALSE;
                ListItem* listItem = rowPtr->downstream->first;
                rowPtr->expirationTimer = 0;
                /* Check if a prune timer expire at the same time */
                while (listItem)
                {
                    RoutingPimDmDownstreamListItem* downstream =
                        (RoutingPimDmDownstreamListItem*) listItem->data;

                    if ((downstream->isPruned)
                        && (node->getNodeTime()-downstream->lastPruneReceived >=
                                downstream->pruneTimer))
                    {
                        pruneExpire = TRUE;
                        break;
                    }
                    listItem = listItem->next;
                }

                if (pruneExpire == FALSE)
                {
                    #ifdef DEBUG
                    {
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(exprTimer->srcAddr,
                                                    srcStr);
                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(exprTimer->grpAddr,
                                                    grpStr);
                        fprintf(stdout, "Node %u @ %s: Delete forwarding "
                                        "cache entry (%s, %s)\n",
                                         node->nodeId, clockStr, srcStr,
                                         grpStr);
                    }
                    #endif

                    PimDmData* pimDmData = NULL;
                    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
                    {
                        pimDmData = (PimDmData*)pim->pimModePtr;
                    }
                    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
                    {
                        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
                    }
#ifdef ADDON_DB
                //remove this entry from multicast_connectivity cache
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = rowPtr->grpAddr;
                strcpy(stats.rootNodeType,"Source");
                    stats.rootNodeId= 
                        MAPPING_GetNodeIdFromInterfaceAddress(
                            node,
                                                          rowPtr->srcAddr);
                stats.upstreamNeighborId =
                        MAPPING_GetNodeIdFromInterfaceAddress(
                            node,
                                                         rowPtr->upstream);
                stats.upstreamInterface =
                        NetworkIpGetInterfaceIndexFromAddress(
                            node,
                                                           rowPtr->inIntf);

                ListItem* listItem = rowPtr->downstream->first;

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

                    /* Delete cache entry */
                    ListFree(node, rowPtr->downstream, FALSE);
                    BUFFER_RemoveDataFromDataBuffer(
                            &pimDmData->fwdTable.buffer,
                            (char*) rowPtr,
                            sizeof(RoutingPimDmForwardingTableRow));
                    pimDmData->fwdTable.numEntries -= 1;
                }
            }
            else
            {
                clocktype delay = rowPtr->expirationTimer - node->getNodeTime();
                RoutingPimDmDataTimeoutInfo cacheTimer;
                cacheTimer.srcAddr = exprTimer->srcAddr;
                cacheTimer.grpAddr = exprTimer->grpAddr;

                RoutingPimSetTimer(
                   node,
                   NetworkIpGetInterfaceIndexFromAddress(
                                            node,
                                                           rowPtr->inIntf),
                                   MSG_ROUTING_PimDmDataTimeOut,
                                   (void*) &cacheTimer,
                                   delay);

                rowPtr->expirationTimer = node->getNodeTime() + delay;
            }

            break;
        }

        case MSG_ROUTING_PimDmGraftRtmxtTimeOut:
        {
            RoutingPimDmGraftTimerInfo* graftTimer = NULL;
            RoutingPimDmForwardingTableRow* rowPtr = NULL;

            graftTimer =
                (RoutingPimDmGraftTimerInfo*) MESSAGE_ReturnInfo(msg);

            /* Search for matching entry */
            rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node,
                             graftTimer->srcAddr, graftTimer->grpAddr);

            /* If GraftAck is not received yet, send Graft again */
            if (rowPtr
                && rowPtr->graftRxmtTimer
                && (rowPtr->state == ROUTING_PIM_DM_ACKPENDING))
            {
                if (DEBUG_TIMER)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                                grpStr);
                    char intfAddrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->inIntf,
                                            intfAddrStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout,"Node %u @ %s: Timer "
                                   "MSG_ROUTING_PimDmGraftRtmxtTimeOut "
                                   "expires for (%s , %s) for "
                                   "interface %d(%s)\n",
                                   node->nodeId,
                                   clockStr,
                                   srcStr,
                                   grpStr,
                                   NetworkIpGetInterfaceIndexFromAddress(
                                                    node,
                                                    rowPtr->inIntf),
                                   intfAddrStr);
                }

                RoutingPimDmSendGraftPacket(node, graftTimer->srcAddr,
                        graftTimer->grpAddr);
            }

            break;
        }

        case MSG_ROUTING_PimDmJoinTimeOut:
        {
            RoutingPimDmDelayedPruneTimerInfo* pruneTimer = NULL;
            RoutingPimDmForwardingTableRow* rowPtr = NULL;
            RoutingPimDmDownstreamListItem* downstreamInfo = NULL;
            int interfaceId = 0;

            pruneTimer = (RoutingPimDmDelayedPruneTimerInfo*)
                                 MESSAGE_ReturnInfo(msg);

            /* Search for matching entry */
            rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(node,
                             pruneTimer->srcAddr, pruneTimer->grpAddr);

            if (rowPtr == NULL)
            {
                break;
            }

            interfaceId = NetworkIpGetInterfaceIndexFromAddress(node,
                                  pruneTimer->downstreamIntf);
            downstreamInfo = RoutingPimDmGetDownstreamInfo(rowPtr,
                interfaceId);
            if (downstreamInfo == NULL)
            {
                break;
            }

           /*
            *  If no join request received for this (S, G) entry, prune
            *  this downstream interface
            */
            if (downstreamInfo->delayedPruneActive)
            {
                if (DEBUG_TIMER)
                {
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                                srcStr);
                    char grpStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                                grpStr);
                    char intfAddrStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(downstreamInfo->intfAddr,
                                            intfAddrStr);
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                    fprintf(stdout,"Node %u @ %s: Timer "
                                   "MSG_ROUTING_PimDmJoinTimeOut "
                                   "expires for (%s , %s) for "
                                   "interface %d(%s)\n",
                                   node->nodeId,
                                   clockStr,
                                   srcStr,
                                   grpStr,
                                   downstreamInfo->interfaceIndex,
                                   intfAddrStr);
                }

                RoutingPimDmPruneTimerInfo timerInfo;
                clocktype delay = 0;

                downstreamInfo->delayedPruneActive = FALSE;

                /* Don't prune this interface if Local group member exist */
                if (RoutingPimDmHasLocalGroup(node, rowPtr->grpAddr,
                             interfaceId))
                {
                    break;
                }

                if ((pim->interface[interfaceId].dmInterface->neighborCount)
                    > 1)
                {
                    RoutingPimDmSendJoinPrunePacket(
                        node,
                        rowPtr->srcAddr,
                        rowPtr->grpAddr,
                        (pim->interface[interfaceId].ipAddress ),
                        ROUTING_PIM_PRUNE_TREE,
                        ROUTING_PIM_DM_PRUNE_TIMEOUT);
                }

                #ifdef DEBUG
                {
                    char intfStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(downstreamInfo->intfAddr,
                                                intfStr);
                    fprintf(stdout, "Node %u prune interface %s\n",
                                 node->nodeId, intfStr);
                }
                #endif

                /* Prune this interface */
                downstreamInfo->isPruned = TRUE;

#ifdef ADDON_DB
                //remove this entry from multicast_connectivity cache
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = rowPtr->grpAddr;
                strcpy(stats.rootNodeType,"Source");
                stats.rootNodeId= MAPPING_GetNodeIdFromInterfaceAddress(node,
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

                /* Set timer to time out prune on this interface */
                timerInfo.srcAddr = rowPtr->srcAddr;
                timerInfo.grpAddr = rowPtr->grpAddr;
                timerInfo.downstreamIntf = downstreamInfo->intfAddr;


                if (downstreamInfo->pruneTimer !=
                            ROUTING_PIM_INFINITE_HOLDTIME)
                {
                    delay = (clocktype)(
                        downstreamInfo->pruneTimer
                                    - ROUTING_PIM_DM_RANDOM_DELAY_JOIN_TIMEOUT
                                    - ROUTING_PIM_DM_LAN_PROPAGATION_DELAY);

                    RoutingPimSetTimer(node,
                        interfaceId,
                        MSG_ROUTING_PimDmPruneTimeoutAlarm,
                                           (void*) &timerInfo,
                                           delay);
                }

                /* If all downstreams have been pruned */
                if (RoutingPimDmAllDownstreamPruned(rowPtr) &&
                    !(NetworkIpIsPartOfMulticastGroup(node,
                            rowPtr->grpAddr)))
                {
                    RoutingPimDmDataTimeoutInfo cacheTimer;

                    rowPtr->state = ROUTING_PIM_DM_PRUNE;
                    /* Send prune to upstream */
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
                                interfaceId,
                                MSG_ROUTING_PimDmDataTimeOut,
                                (void*) &cacheTimer,
                                (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                    }
                    /* Set DataTimeout timer */
                    rowPtr->expirationTimer = node->getNodeTime() + 
                    ROUTING_PIM_DM_DATA_TIMEOUT;

#ifdef CYBER_CORE
                    NetworkDataIp* ip = (NetworkDataIp *)
                        node->networkData.networkVar;

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

            break;
        }

        case MSG_ROUTING_PimDmScheduleJoin:
        {
            RoutingPimDmDelayedJoinTimerInfo* joinTimer;
            RoutingPimDmForwardingTableRow* rowPtr;

            joinTimer = (RoutingPimDmDelayedJoinTimerInfo*)
                                MESSAGE_ReturnInfo(msg);

            /* Search for matching entry */
            rowPtr = RoutingPimDmGetFwdTableEntryForThisPair(
                                                node,
                                                joinTimer->srcAddr,
                                                joinTimer->grpAddr);

            /* Didn't receive a join for this (S, G) entry */
            Int32 interfaceId = joinTimer->interfaceIndex;
            if (rowPtr)
            {
                if (rowPtr->delayedJoinTimer)
                {

                    if (DEBUG_TIMER)
                    {
                        char srcStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                                    srcStr);
                        char grpStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                                    grpStr);
                        char intfAddrStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(rowPtr->inIntf,
                                                intfAddrStr);
                        char clockStr[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                        fprintf(stdout,"Node %u @ %s: Timer "
                                       "MSG_ROUTING_PimDmScheduleJoin "
                                       "expires for (%s , %s) for "
                                       "interface %d(%s)\n",
                                       node->nodeId,
                                       clockStr,
                                       srcStr,
                                       grpStr,
                                       NetworkIpGetInterfaceIndexFromAddress(
                                                        node,
                                                        rowPtr->inIntf),
                                       intfAddrStr);
                    }

                    if (pim->interface[interfaceId].joinPruneSuppression ==
                        FALSE)
                    {
                       /*
                        *  Send PIM JoinPrune packet to let the upstream know
                        *  that it still expect multicast datagram from the
                        *  expected upstream
                        */
                        RoutingPimDmSendJoinPrunePacket(
                                            node,
                                            joinTimer->srcAddr,
                                            joinTimer->grpAddr,
                                            UPSTREAM,
                                            ROUTING_PIM_JOIN_TREE,
                                            ROUTING_PIM_DM_PRUNE_TIMEOUT);
        
                        rowPtr->delayedJoinTimer = FALSE;
                        rowPtr->graftRxmtTimer = FALSE;

                    }
                    else
                    {
                        if (joinTimer->seqNo >=
                            rowPtr->delayedJoinTimerSeqNo)
                        {
                            RoutingPimDmSendJoinPrunePacket(
                                              node,
                                              joinTimer->srcAddr,
                                              joinTimer->grpAddr,
                                              UPSTREAM,
                                              ROUTING_PIM_JOIN_TREE,
                                              ROUTING_PIM_DM_PRUNE_TIMEOUT);

                            rowPtr->delayedJoinTimer = FALSE;
                            rowPtr->graftRxmtTimer = FALSE;

                        }
                    }
                }
                rowPtr->lastJoinTimerEnd = 0;
            }

            break;
        }
#ifdef ADDON_BOEINGFCS
        case MSG_NETWORK_FlushTables:
        {
            PimDmData* pimDmData;
            // This event is called at every RPIM_FLUSH_INTERVAL
            // to delete message cache.
            if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
                pimDmData = (PimDmData*)pim->pimModePtr;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
            }
            RPimCacheDeleteEntry* msgDelEntry =
                (RPimCacheDeleteEntry *) MESSAGE_ReturnInfo(msg);
            RPimDeleteMsgCache(pim,
                &pimDmData->messageCache,
                msgDelEntry);
            break;
        }
#endif
        case MSG_ROUTING_PimScheduleTriggeredHello :
        {
            int interfaceIndex = 0;

            memcpy(&interfaceIndex,
                   MESSAGE_ReturnInfo(msg, INFO_TYPE_PhyIndex),
                   sizeof(int));

            /* Send Hello packet on selected interfaces */
            RoutingPimSendHelloPacket(node,
                interfaceIndex);

            if (DEBUG_HELLO)
            {
                fprintf(stdout, "It was Triggered Hello.\n");
            }

            //updating triggered hello stats
            PimDataSm* pimDataSm = NULL;
            PimDmData* pimDataDm = NULL;

            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
                pimDataDm = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
                pimDataDm = (PimDmData*)pim->pimModePtr;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                pimDataSm = (PimDataSm*)pim->pimModePtr;
            }

            if (pimDataSm)
            {
                pimDataSm->stats.helloSent--;
                pimDataSm->stats.triggeredHelloSent++;
            }
            else
            {
                pimDataDm->stats.helloSent--;
                pimDataDm->stats.triggeredHelloSent++;
            }
            break ;
        }
        case MSG_ROUTING_PimSmNeighborTimeOut:
        {
            RoutingPimSmNbrTimerInfo* nbrTimer;
            RoutingPimNeighborListItem* neighbor;
            RoutingPimInterface* thisInterface;

            nbrTimer = (RoutingPimSmNbrTimerInfo*) MESSAGE_ReturnInfo(msg);
            thisInterface = &pim->interface[nbrTimer->interfaceIndex];

            /* Search neighbor in neighbor list */
            RoutingPimSearchNeighborList(
                thisInterface->smInterface->neighborList,
                                         nbrTimer->nbrAddr,
                                         &neighbor);

            if (!neighbor)
            {
                break;
            }
            if (node->getNodeTime() >= neighbor->NLTTimer)
            {
                neighbor->NLTTimer = 0;
                RoutingPimSmHandleNeighborTimeoutEvent(node,
                    nbrTimer->nbrAddr,nbrTimer->interfaceIndex);
            }
            else if (neighbor->holdTime != 0xFFFF)
            {
                clocktype delay =
                    neighbor->NLTTimer - node->getNodeTime();
                RoutingPimSmNbrTimerInfo newnbrTimer;
                newnbrTimer.nbrAddr = nbrTimer->nbrAddr;
                newnbrTimer.interfaceIndex = nbrTimer->interfaceIndex;

                RoutingPimSetTimer(node,
                                   nbrTimer->interfaceIndex,
                                   MSG_ROUTING_PimSmNeighborTimeOut,
                                   (void*) &newnbrTimer,
                                   delay);

                neighbor->NLTTimer =
                    node->getNodeTime() + delay;
            }
            else if (neighbor->holdTime == 0xFFFF)
            {
                neighbor->NLTTimer = 0;
            }
            break;
        }
        case MSG_ROUTING_PimSmScheduleCandidateRP:
        {
            BOOL sent = FALSE;
            RoutingPimSmCandidateRPTimerInfo* candidateRPTimer;
            RoutingPimSmCandidateRPTimerInfo rpTimerInfo;
            candidateRPTimer = (RoutingPimSmCandidateRPTimerInfo*)
                                    MESSAGE_ReturnInfo(msg);
            PimDataSm* pimSmData = NULL;
            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                pimSmData = (PimDataSm*) pim->pimModePtr;
            }
#ifdef ADDON_BOEINGFCS
            BOOL breakHere = FALSE;
            breakHere = MulticastCesRpimCheckRp(node, msg);

            if (breakHere)
                break;
#endif

            RoutingPimInterface* thisInterface =
                            &pim->interface[candidateRPTimer->intfIndex];

            #ifdef DEBUG_ERRORS
            {
                  char clockStr[100];
                  ctoa(node->getNodeTime(), clockStr);
                  printf("Node %u,Int %d: "
                      " MSG_ROUTING_PimSmScheduleCandidateRP\n",
                                    node->nodeId,
                                    candidateRPTimer->intfIndex);
                  ctoa(pimSmData->lastCandidateRPSend, clockStr);
                  printf("Node %u:pimsm->lastCandidateRPSend at %s\n",
                        node->nodeId, clockStr);
            }
            #endif

            // dynamic address change
            rpTimerInfo.srcAddr = thisInterface->ipAddress;
            rpTimerInfo.intfIndex = candidateRPTimer->intfIndex;
            rpTimerInfo.seqNo = thisInterface->candidateRPTimerSeq + 1;
            thisInterface->candidateRPTimerSeq++;

            if ((thisInterface->BSRFlg == TRUE)
                &&
                (thisInterface->cBSRState == ELECTED_BSR))
            {
                sent = RoutingPimSmSendCandidateRPPacket(node,
                                          (int)candidateRPTimer->intfIndex);
            }
#ifdef DEBUG
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("Scheduling MSG_ROUTING_PimSmCandidateRPTimeOut"
            " at time %s for int %d \n",clockStr, rpTimerInfo.intfIndex);
    }
#endif
           RoutingPimSetTimer(node,
                              rpTimerInfo.intfIndex,
                              MSG_ROUTING_PimSmCandidateRPTimeOut,
                              (void*) &rpTimerInfo,
                              (clocktype)0);
#if 0
#ifdef ADDON_BOEINGFCS
            if (!sent)
            {
                MulticastCesRpimRescheduleRpPacket(node, msg);
            }
#endif
#endif
            break;
        }
        case MSG_ROUTING_PimSmCandidateRPTimeOut:
        {
            clocktype delay = 0;
            BOOL sent = FALSE;
            RoutingPimSmCandidateRPTimerInfo* candidateRPTimer;
            RoutingPimSmCandidateRPTimerInfo rpTimerInfo;
            candidateRPTimer = (RoutingPimSmCandidateRPTimerInfo*)
                                    MESSAGE_ReturnInfo(msg);
            PimDataSm* pimSmData = NULL;
            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                pimSmData = (PimDataSm*) pim->pimModePtr;
            }
            RoutingPimInterface* thisInterface =
                           &pim->interface[candidateRPTimer->intfIndex];

            #ifdef DEBUG_ERRORS
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                  printf("Node %u, Int %d:"
                      " MSG_ROUTING_PimSmCandidateRPTimeOut\n",
                                    node->nodeId,
                                    candidateRPTimer->intfIndex);
                  ctoa(pimSmData->lastCandidateRPSend, clockStr);
                  printf("Node %u:pimsm->lastCandidateRPSend at %s\n",
                        node->nodeId, clockStr);
            }
            #endif


            if (candidateRPTimer->seqNo >=
                                         thisInterface->candidateRPTimerSeq)
            {

#ifdef ADDON_BOEINGFCS
                BOOL breakHere = FALSE;
                breakHere = MulticastCesRpimCheckRp(node, msg);

                if (breakHere)
                    break;
#endif
                // dynamic address change 
                rpTimerInfo.srcAddr = thisInterface->ipAddress;
                rpTimerInfo.intfIndex = candidateRPTimer->intfIndex;
                rpTimerInfo.seqNo = thisInterface->candidateRPTimerSeq + 1;
                thisInterface->candidateRPTimerSeq++;

                if (thisInterface->backOffCounter != 0)
                {
                    thisInterface->backOffCounter--;
                    /* Send CANDIDATE-RP packet towards BSR for the domain */

                    sent = RoutingPimSmSendCandidateRPPacket(node,
                                          (int)candidateRPTimer->intfIndex);

                    //Schedule C_RP_BACKOFF timer
                    delay = (clocktype) (RANDOM_nrand(pimSmData->seed)
                                 % ROUTING_PIMSM_CANDIDATE_RP_ADV_BACKOFF);
#ifdef DEBUG
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("Scheduling MSG_ROUTING_PimSmCandidateRPTimeOut"
            " at time %s for int %d with delay %d\n",
            clockStr, rpTimerInfo.intfIndex,delay);
    }
#endif
                    RoutingPimSetTimer(node,
                                       rpTimerInfo.intfIndex,
                                       MSG_ROUTING_PimSmCandidateRPTimeOut,
                                       (void*) &rpTimerInfo,
                                       delay);
                }
                else
                {
                    /* Send CANDIDATE-RP packet towards BSR for the domain */
                    sent = RoutingPimSmSendCandidateRPPacket(node,
                                           (int)candidateRPTimer->intfIndex);

                    //Schedule C_RP_TIMEOUT TIMER
                    delay = 
                    (clocktype)(pimSmData->routingPimSmCandidateRpTimeout);
#ifdef DEBUG
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("Scheduling MSG_ROUTING_PimSmCandidateRPTimeOut"
            " at time %s for int %d with delay %d\n",
            clockStr, rpTimerInfo.intfIndex,delay);
    }
#endif
                    RoutingPimSetTimer(node,
                                       rpTimerInfo.intfIndex,
                                       MSG_ROUTING_PimSmCandidateRPTimeOut,
                                       (void*) &rpTimerInfo,
                                       delay);
                }
#ifdef ADDON_BOEINGFCS
            if (!sent)
            {
                MulticastCesRpimRescheduleRpPacket(node, msg);
            }
#endif
            }
            break;
        }

        case MSG_ROUTING_PimSmBootstrapTimeOut:
        {
            RoutingPimSmBootstrapTimerInfo  * bootstrapTimer;
            bootstrapTimer = (RoutingPimSmBootstrapTimerInfo*)
                                    MESSAGE_ReturnInfo(msg);
            PimDataSm* pimSmData = NULL;
            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                pimSmData = (PimDataSm*) pim->pimModePtr;
            }
            RoutingPimInterface* thisInterface =
                &pim->interface[bootstrapTimer->intfIndex];

            #ifdef DEBUG_ERRORS
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("Node %u,Int %d: MSG_ROUTING_PimSmBootstrapTimeOut\n",
                            node->nodeId,bootstrapTimer->intfIndex);
                ctoa(pimSmData->lastBootstrapSend, clockStr);
                printf("Node %u:pimsm->lastBootstrapSend at %s\n",
                    node->nodeId, clockStr);
            }
            #endif

            if (bootstrapTimer->seqNo >= thisInterface->bootstrapTimerSeq)
            {
                if (thisInterface->BSRFlg == TRUE)
                {
                    RoutingPimSmCBSRStateMachine(node,
                                                 bootstrapTimer->intfIndex,
                                                 BOOTSTRAP_TIMER_EXP,
                                                 msg);
                }
                else
                {
                    RoutingPimSmNonCBSRStateMachine(node,
                                                    bootstrapTimer->intfIndex,
                                                    BOOTSTRAP_TIMER_EXP,
                                                    msg);
                }
            }
            break;
        }

        case MSG_ROUTING_PimSmKeepAliveTimeOut:
        {
            RoutingPimSmKeepAliveTimerInfo  * keepAliveTimer;
            RoutingPimSmTreeInfoBaseRow     * treeInfoBaseRowPtr;

            keepAliveTimer = (RoutingPimSmKeepAliveTimerInfo*)
                                MESSAGE_ReturnInfo(msg);

            #ifdef DEBUG_ERRORS
            {
                printf("Node %u HandleKeepAliveTimeOutEvent \n",
                        node->nodeId);
                printf("seq %d  srcAddr: %u, dest: %u, interface: %d \n",
                   keepAliveTimer->seqNo, keepAliveTimer->srcAddr,
                       keepAliveTimer->grpAddr, keepAliveTimer->intfIndex);
                printf(" keepAliveTimer->treeState is %d \n",
                    keepAliveTimer->treeState);

            }
            #endif

            treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                                  (node, keepAliveTimer->grpAddr,
                                      keepAliveTimer->srcAddr,
                                           keepAliveTimer->treeState);

            if (treeInfoBaseRowPtr != NULL)
            {
                #ifdef DEBUG_ERRORS
                {
                    printf(" current seqNo is %d \n",
                         treeInfoBaseRowPtr->keepAliveTimerSeq);
                }
                #endif

                if (node->getNodeTime() >= 
                        treeInfoBaseRowPtr->keepAliveExpiryTimer)
                {
                    treeInfoBaseRowPtr->keepAliveExpiryTimer = 0;
                }
                else
                {
                    clocktype delay = 
                        treeInfoBaseRowPtr->keepAliveExpiryTimer - 
                        node->getNodeTime();
                    RoutingPimSmKeepAliveTimerInfo  newKeepAliveTimer;
                    newKeepAliveTimer.grpAddr = keepAliveTimer->grpAddr;
                    newKeepAliveTimer.intfIndex = keepAliveTimer->intfIndex;
                    newKeepAliveTimer.srcAddr = keepAliveTimer->srcAddr;
                    newKeepAliveTimer.treeState = keepAliveTimer->treeState;

                    RoutingPimSetTimer(node,
                                       keepAliveTimer->intfIndex,
                                       MSG_ROUTING_PimSmKeepAliveTimeOut,
                                       (void*) &newKeepAliveTimer,
                                       delay);

                    treeInfoBaseRowPtr->keepAliveExpiryTimer = 
                        node->getNodeTime() + delay;
                }
            }

            break;
        }

        case MSG_ROUTING_PimSmExpiryTimerTimeout:
        {
            RoutingPimSmJoinPruneTimerInfo  * expiryTimer;
            RoutingPimSmTreeInfoBaseRow     * treeInfoBaseRowPtr;
            RoutingPimSmDownstream          * downStreamListPtr = NULL;

            expiryTimer = (RoutingPimSmJoinPruneTimerInfo*)
                                MESSAGE_ReturnInfo(msg);
            #ifdef DEBUG_ERRORS
            {
                char clockStr[100];

                ctoa(node->getNodeTime(), clockStr);
                printf("Node %u Get ExpiryTimerTimeOut at %s \n",
                        node->nodeId, clockStr);
                printf("    srcAddr: %u, dest: %u, interface: %d \n",
                    expiryTimer->srcAddr, expiryTimer->grpAddr,
                        expiryTimer->intfIndex);
                printf(" associated tree state: %d \n",
                    expiryTimer->treeState);
            }
            #endif

            treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                                  (node, expiryTimer->grpAddr,
                                    expiryTimer->srcAddr,
                                        expiryTimer->treeState);
            if (treeInfoBaseRowPtr == NULL)
            {
#ifdef ADDON_BOEINGFCS
                if (MulticastCesRpimActiveOnNode(node))
               {
                   break;
               }
#endif
#ifdef EXATA
                if (!node->partitionData->isEmulationMode)
                {
                    ERROR_Assert(FALSE, "entry must be in treeInfo Base\n");
                }
#else
                ERROR_Assert(FALSE, "entry must be in treeInfo Base\n");
#endif //EXATA

                break;
            }

            if (treeInfoBaseRowPtr != NULL)
            {
                downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, expiryTimer->intfIndex);
            }
            #ifdef DEBUG_ERRORS
            {
                printf(" Node %u: has expiry timer seq = %d \n",
                     node->nodeId, expiryTimer->seqNo);
                printf(" current avavilable seq %d \n",
                     downStreamListPtr->expiryTimerSeq);
            }
            #endif

            if (downStreamListPtr != NULL)
            {
                if (expiryTimer->seqNo >= downStreamListPtr->expiryTimerSeq)
                {
                    if (DEBUG_JP)
                    {
                        printf("Node %u HandleExpiryTimerTimeOutEvent \n",
                                node->nodeId);
                    }

                    if (expiryTimer->treeState == ROUTING_PIMSM_G)
                    {
                        expiryTimer->srcAddr = treeInfoBaseRowPtr->srcAddr;
                    }


                    RoutingPimSmHandleExpiryTimerExpiresEvent(node,
                        expiryTimer->srcAddr, expiryTimer->grpAddr,
                             expiryTimer->intfIndex, treeInfoBaseRowPtr);
                }
            }

            break;
        }

        case MSG_ROUTING_PimSmPrunePendingTimerTimeout:
        {
            RoutingPimSmJoinPruneTimerInfo  * prunePendingTimer;
            RoutingPimSmTreeInfoBaseRow     * treeInfoBaseRowPtr;
            RoutingPimSmDownstream          * downStreamListPtr = NULL;

            prunePendingTimer = (RoutingPimSmJoinPruneTimerInfo*)
                                MESSAGE_ReturnInfo(msg);

            treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                                (node, prunePendingTimer->grpAddr,
                                    prunePendingTimer->srcAddr,
                                        prunePendingTimer->treeState);

            if (treeInfoBaseRowPtr == NULL)
            {
#ifdef ADDON_BOEINGFCS
                if (MulticastCesRpimActiveOnNode(node))
                {
                    break;
                }
#endif
#ifdef EXATA
               if (!node->partitionData->isEmulationMode)
               {
                    ERROR_Assert(FALSE, "entry must be in treeInfo Base\n");
               }
#else
                ERROR_Assert(FALSE, "entry must be in treeInfo Base\n");
#endif //EXATA
               break;
            }

            if (treeInfoBaseRowPtr != NULL)
            {
                downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                           treeInfoBaseRowPtr, prunePendingTimer->intfIndex);
            }
#ifdef ADDON_BOEINGFCS
            if (downStreamListPtr == NULL)
            {
                break;
            }
#endif

            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);

                printf("Node %u Get PrunePendingTimerTimeOut at %s \n",
                        node->nodeId, clockStr);
                printf("Seq: %d srcAddr: %u, dest: %u, interface: %d \n",
                    prunePendingTimer->seqNo, prunePendingTimer->srcAddr,
                        prunePendingTimer->grpAddr,
                            prunePendingTimer->intfIndex);
                printf(" Current PPTimer Seq is %d \n",
                    downStreamListPtr->prunePendingTimerSeq);
                printf(" associated tree state: %d \n",
                    prunePendingTimer->treeState);
            }
            #endif


            if (downStreamListPtr->prunePendingTimerRunning)
            {
                if (DEBUG_JP)
                {
                    printf("Node %u HandlePrunePendingTimerTimeOutEvent \n",
                            node->nodeId);
                }
                RoutingPimSmHandlePrunePendingTimerExpiresEvent(node,
                    prunePendingTimer->srcAddr,
                        prunePendingTimer->grpAddr,
                            prunePendingTimer->intfIndex,
                                treeInfoBaseRowPtr);
            }
            else
            {
                #ifdef DEBUG
                {
                    printf(" timer is cancelled \n");
                }
                #endif
            }

            break;
        }

        case MSG_ROUTING_PimSmJoinTimerTimeout:
        {
            RoutingPimSmJoinPruneTimerInfo  * joinTimer;
            RoutingPimSmTreeInfoBaseRow     * treeInfoBaseRowPtr;

            joinTimer = (RoutingPimSmJoinPruneTimerInfo*)
                                MESSAGE_ReturnInfo(msg);
            if (DEBUG_JP)
            {
                char clockStr[100];

                ctoa(node->getNodeTime(), clockStr);
                printf("Node %u Get JoinTimerTimeOut at %s \n",
                        node->nodeId, clockStr);
                printf("seqNo:%d  srcAddr: %u, dest: %u, interface: %d \n",
                    joinTimer->seqNo, joinTimer->srcAddr, joinTimer->grpAddr,
                        joinTimer->intfIndex);
                printf(" associated tree state: %d \n",
                            joinTimer->treeState);
            }

            treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                                   (node, joinTimer->grpAddr,
                                         joinTimer->srcAddr,
                                               joinTimer->treeState);

            if (treeInfoBaseRowPtr == NULL)
            {
#ifdef EXATA
               if (!node->partitionData->isEmulationMode)
               {
                    ERROR_Assert(FALSE, "entry must be in treeInfo Base\n");
               }
#else
               ERROR_Assert(FALSE, "entry must be in treeInfo Base\n");
#endif //EXATA
                break;
            }

            if (DEBUG_JP)
            {
                char clockStr[100];

                ctoa(node->getNodeTime(), clockStr);
                printf("Node %u Get JoinTimerTimeOut at %s \n",
                        node->nodeId, clockStr);
                printf("seqNo:%d  srcAddr: %u, dest: %u, interface: %d \n",
                    joinTimer->seqNo, joinTimer->srcAddr, joinTimer->grpAddr,
                        joinTimer->intfIndex);
                printf("treeInfoBaseRowPtr->joinTimerSeq %d \n",
                            treeInfoBaseRowPtr->joinTimerSeq);
                printf(" associated tree state: %d \n",
                            joinTimer->treeState);
            }


            if (joinTimer->seqNo >= treeInfoBaseRowPtr->joinTimerSeq)
            {

                if (joinTimer->treeState == ROUTING_PIMSM_G)
                {
                    joinTimer->srcAddr = treeInfoBaseRowPtr->srcAddr;
                }


                RoutingPimSmHandleJoinTimerExpiresEvent(node,
                        joinTimer->srcAddr, joinTimer->grpAddr,
                            treeInfoBaseRowPtr);
            }
            break;
        }
        case MSG_ROUTING_PimSmAssertTimerTimeout:
        {
            RoutingPimSmAssertTimerInfo     * assertTimer;
            RoutingPimSmTreeInfoBaseRow     * treeInfoBaseRowPtr;
            RoutingPimSmDownstream          * downStreamListPtr = NULL;

            assertTimer = (RoutingPimSmAssertTimerInfo*)
                                MESSAGE_ReturnInfo(msg);

            treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                                   (node, assertTimer->grpAddr,
                                         assertTimer->srcAddr,
                                               assertTimer->treeState);
            if (treeInfoBaseRowPtr != NULL)
            {
                downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, assertTimer->intfIndex);
            }

            #ifdef DEBUG
            {
                char clockStr[100];

                ctoa(node->getNodeTime(), clockStr);
                printf("Node %u Get AssertTimerTimeOut at %s \n",
                        node->nodeId, clockStr);
                printf("seqNo:%d  srcAddr: %u, dest: %u, interface: %d \n",
                    assertTimer->seqNo, assertTimer->srcAddr,
                            assertTimer->grpAddr,
                        assertTimer->intfIndex);
                printf(" associated tree state: %d \n",
                            assertTimer->treeState);
            }
            #endif
            if (node->getNodeTime() >= downStreamListPtr->assertTime)
            {
                 RoutingPimSmHandleAssertTimeOutEvent(node,
                        assertTimer->srcAddr,
                      assertTimer->grpAddr, assertTimer->intfIndex,
                           treeInfoBaseRowPtr);
            }
            else
            {
                clocktype delay =
                   downStreamListPtr->assertTime - node->getNodeTime();
                RoutingPimSmAssertTimerInfo   newassertTimer;
                /* Set timer to assert_time */
                newassertTimer.srcAddr = assertTimer->srcAddr;
                newassertTimer.grpAddr = assertTimer->grpAddr;
                newassertTimer.intfIndex = assertTimer->intfIndex;
                newassertTimer.treeState = assertTimer->treeState;
                RoutingPimSetTimer(node,
                       assertTimer->intfIndex,
                       MSG_ROUTING_PimSmAssertTimerTimeout,
                       (void*) &newassertTimer, delay);
                downStreamListPtr->assertTime = node->getNodeTime() + delay;
            }
            break;
        }
        case MSG_ROUTING_PimSmRegisterStopTimeOut:
        {
            RoutingPimSmRegisterStopTimerInfo* registerStopTimer;
            RoutingPimSmTreeInfoBaseRow      * treeInfoBaseRowPtr;

            RoutingPimSmRegisterStopTimerInfo    timerInfo;
            clocktype delay;

            NodeAddress RPAddr;

            registerStopTimer = (RoutingPimSmRegisterStopTimerInfo*)
                                     MESSAGE_ReturnInfo(msg);

            RPAddr = RoutingPimSmFindRPForThisGroup(node,
                                     registerStopTimer->grpAddr);

            treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                                   (node, registerStopTimer->grpAddr,
                                         registerStopTimer->srcAddr,
                                            registerStopTimer->treeState);
             #ifdef DEBUG
            {
                char clockStr[100];

                ctoa(node->getNodeTime(), clockStr);
                printf("\nNode %u Get RegisterStopTimerTimeOut at %s \n",
                        node->nodeId, clockStr);
                printf("\nseqNo:%d  srcAddr: %u, dest: %u, interface: %d \n",
                    registerStopTimer->seqNo, registerStopTimer->srcAddr,
                         registerStopTimer->grpAddr,
                            registerStopTimer->intfIndex);

                printf(" associated tree state: %d \n",
                            registerStopTimer->treeState);
            }
            #endif
            if (registerStopTimer->seqNo >=
                   (unsigned int) treeInfoBaseRowPtr->registerStopTimerSeq)
            {
                if (treeInfoBaseRowPtr->registerState
                        == PimSm_Register_JoinPending)
                {
                    /* move to join state */
                    treeInfoBaseRowPtr->registerState = PimSm_Register_Join;
                    treeInfoBaseRowPtr->isTunnelPresent = TRUE;

                    #ifdef DEBUG
                        printf("Node %u: Time to add tunnel \n",
                            node->nodeId);
                    #endif
                }
                else
                if (treeInfoBaseRowPtr->registerState
                    == PimSm_Register_Prune)
                {
                    treeInfoBaseRowPtr->registerState =
                        PimSm_Register_JoinPending;
                    /* set register stop timer */
                    timerInfo.srcAddr = registerStopTimer->srcAddr;
                    timerInfo.grpAddr = registerStopTimer->grpAddr;
                    timerInfo.intfIndex = registerStopTimer->intfIndex;
                    timerInfo.seqNo =
                        treeInfoBaseRowPtr->registerStopTimerSeq + 1;

                    timerInfo.treeState = ROUTING_PIMSM_SG;

                    /* Note the current registerStopTimerSeq */
                    treeInfoBaseRowPtr->registerStopTimerSeq++;

                    PimDataSm* pimSmData = NULL;
                    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
                    {
                        pimSmData =
                            ((PimDataSmDm*) pim->pimModePtr)->pimSm;
                    }
                    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
                    {
                        pimSmData = (PimDataSm*) pim->pimModePtr;
                    }
                    //Delay should be Register-Probe timer.We are currently
                    // not supporting Register Probing hence we consider
                    // the delay 0
                    delay = 0;
                    RoutingPimSetTimer(node,
                        timerInfo.intfIndex,
                        MSG_ROUTING_PimSmRegisterStopTimeOut,
                            (void*) &timerInfo, delay);
                    #ifdef ERRORS
                        printf("Node %u: send null register\n",
                            node->nodeId);
                    #endif
                }
            }
            break;
        }
        case MSG_ROUTING_PimSmOverrideTimerTimeout:
        {
            RoutingPimSmJoinPruneTimerInfo    * OTTimer;
            RoutingPimSmTreeInfoBaseRow      * treeInfoBaseRowPtr;
            NodeAddress nextHop;

            int outIntfId;

            OTTimer = (RoutingPimSmJoinPruneTimerInfo*)
                                     MESSAGE_ReturnInfo(msg);
            #ifdef DEBUG
            {
                char clockStr[100];

                ctoa(node->getNodeTime(), clockStr);
                printf("Node %u Get OverrideTimerTimeOut at %s \n",
                        node->nodeId, clockStr);
                printf("seqNo:%d  srcAddr: %u, dest: %u, interface: %d \n",
                    OTTimer->seqNo, OTTimer->srcAddr,
                         OTTimer->grpAddr, OTTimer->intfIndex);

                printf(" associated tree state: %d \n",
                            OTTimer->treeState);
            }
            #endif

            NodeAddress RPAddress =
                RoutingPimSmFindRPForThisGroup(node, OTTimer->grpAddr);

            RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                RPAddress,
                    &outIntfId, &nextHop);
            treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                                   (node,OTTimer->grpAddr,
                                         OTTimer->srcAddr,
                                         OTTimer->treeState);

            if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
            {
                for (int i = 0; i < node->numberInterfaces; i++)
                {
                    if (RPAddress ==
                        NetworkIpGetInterfaceAddress(node, i))
                    {
                        nextHop = RPAddress;
                        outIntfId = i;
                    }
                }
            }
            if (outIntfId == NETWORK_UNREACHABLE)
            {
                break;
            }
            if (nextHop == 0)
            {
                nextHop = RPAddress ;
            }
            if (OTTimer->seqNo >= treeInfoBaseRowPtr->OTTimerSeq)
            {
                /* send S, G, rpt */
                RoutingPimSmSendJoinPrunePacket(node,
                    OTTimer->srcAddr,
                    nextHop,
                    OTTimer->grpAddr,
                    outIntfId,
                    ROUTING_PIMSM_SGrpt_JOIN_PRUNE,
                    ROUTING_PIM_JOIN_TREE,
                    treeInfoBaseRowPtr);
            }
            treeInfoBaseRowPtr->lastOTTimerStart = 0;
            break;
        }
        case MSG_ROUTING_PimSmRouterGrpToRPTimeout:
        {
            RoutingPimEncodedGroupAddr grpInfo;
            memset(&grpInfo, 0, sizeof(RoutingPimEncodedGroupAddr));

            RoutingPimEncodedUnicastAddr rpInfo;
            memset(&rpInfo, 0, sizeof(RoutingPimEncodedUnicastAddr));

            RoutingPimSmGrpToRPTimerInfo* routerGrpToRPTimer =
                (RoutingPimSmGrpToRPTimerInfo*) MESSAGE_ReturnInfo(msg);

            grpInfo.groupAddr = routerGrpToRPTimer->grpPrefix;
            grpInfo.maskLength = routerGrpToRPTimer->maskLength;

            setNodeAddressInCharArray(rpInfo.unicastAddr,
                                      routerGrpToRPTimer->RPAddress);

            ListItem* rpListItem = 
            RoutingPimSmIsRPMappingPresent(node,
                                           &grpInfo,
                                           &rpInfo);

            if (rpListItem == NULL)
            {
                if (DEBUG_BS)
                {
                    printf("\nEntry not present in the list\n");
                }
                break;
            }

            RoutingPimSmRPList* rpListPtr = 
                (RoutingPimSmRPList*)(rpListItem->data);

            if (routerGrpToRPTimer->seqNo >= rpListPtr->routerGrpToRPTimerSeq)
            {
                if (DEBUG_BS)
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                    printf("Node %d As Router Group To RP Timer expired.\n",
                                        node->nodeId);
                    printf("Hence,Remove Grp To RP Mapping From RP Set at" 
                                        " time %s.\n",clockStr);
                }

                RoutingPimSmRemoveGrpToRPMappingFromRPSet(
                        node,
                        rpListItem);
            }
            break;
        }
        case MSG_ROUTING_PimSmBSRGrpToRPTimeout:
        {
            BOOL IamCurrentBSR = FALSE;
            RoutingPimInterface* thisInterface = NULL;

            for (int i = 0; i < node->numberInterfaces; i++)
            {
                thisInterface = &pim->interface[i];

                if (thisInterface->cBSRState == ELECTED_BSR)
                {
                    IamCurrentBSR = TRUE;
                }
            }

            if (IamCurrentBSR == FALSE)
            {
                if (DEBUG_BS)
                {
                    printf("I am not elected BSR."
                           "Hence,not handling this event");
                }
                break;
            }

            RoutingPimEncodedGroupAddr grpInfo;
            memset(&grpInfo, 0, sizeof(RoutingPimEncodedGroupAddr));

            RoutingPimEncodedUnicastAddr rpInfo;
            memset(&rpInfo, 0, sizeof(RoutingPimEncodedUnicastAddr));

            RoutingPimSmGrpToRPTimerInfo* bsrGrpToRPTimer =
                (RoutingPimSmGrpToRPTimerInfo*) MESSAGE_ReturnInfo(msg);

            grpInfo.groupAddr = bsrGrpToRPTimer->grpPrefix;
            grpInfo.maskLength = bsrGrpToRPTimer->maskLength;

            setNodeAddressInCharArray(rpInfo.unicastAddr,
                          bsrGrpToRPTimer->RPAddress);

            ListItem* rpListItem =
            RoutingPimSmIsRPMappingPresent(node,
                                           &grpInfo,
                                           &rpInfo);

            if (rpListItem == NULL)
            {
                if (DEBUG_BS)
                {
                    printf("\nEntry not present in the list\n");
                }
                break;
            }

            RoutingPimSmRPList* rpListPtr = 
                (RoutingPimSmRPList*)(rpListItem->data);

            if (bsrGrpToRPTimer->seqNo >= rpListPtr->bsrGrpToRPTimerSeq)
            {
                if (DEBUG_BS)
                {
                    printf("As BSR Group To RP Timer expired.\n");
                    printf("Hence,Remove Grp To RP Mapping From RP Set.\n");
                }

                RoutingPimSmRemoveGrpToRPMappingFromRPSet(
                       node,
                       rpListItem);
            }
            break;
        }
#ifdef ADDON_BOEINGFCS
        case MSG_MULTICAST_CES_RescheduleJoinOrLeave:
        {
            MulticastCesRpimHandleRescheduleJoinOrLeave(node, msg);
            break;
        }
        case MSG_MULTICAST_CES_FlushCache:
        {
            MulticastCesRpimHandleFlushCache(node, msg);
            break;
        }
#endif

#ifdef ADDON_DB
        case MSG_STATS_PIM_SM_InsertStatus:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL || !db->statsPimTable ||
                !db->statsPimTable->createPimSmStatusTable)
            {
                break;
            }

            HandleStatsDBPimSmStatusTableInsertion(node);

            MESSAGE_Send(node,
                MESSAGE_Duplicate(node, msg),
                db->statsPimTable->pimStatusInterval);

            break;
        }
        case MSG_STATS_PIM_SM_InsertSummary:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL)
            {
                break;
            }

            if (!db->statsPimTable
                || !db->statsPimTable->createPimSmSummaryTable)
            {
                break;
            }

            HandleStatsDBPimSmSummaryTableInsertion(node);

            MESSAGE_Send(node,
                         MESSAGE_Duplicate(node, msg),
                         db->statsPimTable->pimSummaryInterval);

            break;
        }
        case MSG_STATS_PIM_DM_InsertSummary:
        {
            StatsDb* db = node->partitionData->statsDb;
            if (db == NULL)
            {
                break;
            }

            if (!db->statsPimTable
                || !db->statsPimTable->createPimDmSummaryTable)
            {
                break;
            }

            HandleStatsDBPimDmSummaryTableInsertion(node);

            MESSAGE_Send(node,
                         MESSAGE_Duplicate(node, msg),
                         db->statsPimTable->pimSummaryInterval);

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

           HandleStatsDBPimMulticastNetSummaryTableInsertion(node);

           MESSAGE_Send(node,
                        MESSAGE_Duplicate(node, msg),
                        db->statsSummaryTable->summaryInterval);

           break;
        }
#endif
        default:
        {

#ifdef EXATA
            if (node->partitionData->isEmulationMode)
            {
                char buf[MAX_STRING_LENGTH];
                sprintf(buf,
                  "Unknown protocol event in PIM");
                ERROR_ReportWarning(buf);
            }
            else
            {
                printf("    Event Type = %d\n", msg->eventType);
                ERROR_Assert(FALSE, "Unknown protocol event in PIM\n");
            }
#else
            printf("    Event Type = %d\n", msg->eventType);
            ERROR_Assert(FALSE, "Unknown protocol event in PIM\n");
#endif
            break;
        }
    }

    MESSAGE_Free(node, msg);
}


/*
*  FUNCTION     :RoutingPimHandleProtocolPacket()
*  PURPOSE      :Handle incoming control packet of PIM
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimHandleProtocolPacket(Node* node, Message* msg,
                                    NodeAddress srcAddr, int interfaceId)
{
    /* Make sure that PIM is running over this interface */
//  This check needs to occur before any other code is run to
//  avoid potential errors when PIM is not active on this interface.
    if (!RoutingPimIsPimEnabledInterface(node, interfaceId))
    {
        MESSAGE_Free(node, msg);
        return;
    }

    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    RoutingPimInterface* thisInterface = &pim->interface[interfaceId];

    /* Get PIM Common header */
    RoutingPimCommonHeaderType* commonHeader =
        (RoutingPimCommonHeaderType*) MESSAGE_ReturnPacket(msg);

    switch (RoutingPimCommonHeaderGetType(commonHeader->rpChType))
    {
        case ROUTING_PIM_HELLO:
        {
            RoutingPimHelloPacket* helloPkt =
                (RoutingPimHelloPacket*) MESSAGE_ReturnPacket(msg);
            int size = MESSAGE_ReturnPacketSize(msg);

            if (DEBUG_HELLO)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char srcAddrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(srcAddr, srcAddrStr);
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(
                          NetworkIpGetInterfaceAddress(node, interfaceId),
                          addrStr);

                fprintf(stdout, "Node %u @ %s: Received HELLO from %s "
                                "on interface %d(%s)\n",
                                node->nodeId, clockStr, srcAddrStr,
                                interfaceId, addrStr);
            }

            #ifdef DEBUG_ERROR
            {
                fprintf(stdout, "    PIM Hello packet length = %d\n", size);
            }
            #endif

            if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
                PimDmData* pimDmData = (PimDmData*) pim->pimModePtr;

                pimDmData->stats.helloReceived++;
#ifdef ADDON_DB
                pimDmData->dmSummaryStats.m_NumHelloRecvd++;
#endif
                RoutingPimDmHandleHelloPacket(node, srcAddr, helloPkt,
                    size, interfaceId);
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                PimDataSm* pimSmData = (PimDataSm*) pim->pimModePtr;

                pimSmData->stats.numOfHelloPacketReceived++;
#ifdef ADDON_DB
                pimSmData->smSummaryStats.m_NumHelloRecvd++;
#endif
                RoutingPimSmHandleHelloPacket(node,
                                              srcAddr,
                                              helloPkt,
                                              size,
                                              interfaceId);
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                PimDmData* pimDmData =
                    ((PimDataSmDm*) pim->pimModePtr)->pimDm;
                pimDmData->stats.helloReceived++;
#ifdef ADDON_DB
                pimDmData->dmSummaryStats.m_NumHelloRecvd++;
#endif
                RoutingPimDmHandleHelloPacket(node, srcAddr, helloPkt,
                    size, interfaceId);

                PimDataSm* pimSmData =
                    ((PimDataSmDm*) pim->pimModePtr)->pimSm;
                pimSmData->stats.numOfHelloPacketReceived++;
#ifdef ADDON_DB
                pimSmData->smSummaryStats.m_NumHelloRecvd++;
#endif
                RoutingPimSmHandleHelloPacket(node, srcAddr, helloPkt,
                    size, interfaceId);
            }

            break;
        }

        case ROUTING_PIM_JOIN_PRUNE:
        {
            if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
                PimDmData* pimDmData = (PimDmData*) pim->pimModePtr;
                RoutingPimDmJoinPrunePacket* joinPrunePkt =
                    (RoutingPimDmJoinPrunePacket*) MESSAGE_ReturnPacket(msg);

                pimDmData->stats.joinPruneReceived++;
#ifdef ADDON_DB
                pimDmData->dmSummaryStats.m_NumJoinPruneRecvd++;
#endif
                RoutingPimDmHandleJoinPrunePacket(node,
                    srcAddr, joinPrunePkt,interfaceId);
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                PimDataSm* pimSmData = (PimDataSm*) pim->pimModePtr;
                RoutingPimSmJoinPrunePacket* joinPrunePkt =
                    (RoutingPimSmJoinPrunePacket*) MESSAGE_ReturnPacket(msg);
                int i = 0;
                int j = 0;
                RoutingPimSmJoinPruneGroupInfo* grpInfo;

                RoutingPimEncodedSourceAddr * encodedSrcAddr = NULL;
                RoutingPimNeighborListItem* neighbor = NULL;
                /* a PIM Join/Prune message should only be accepted for
                processing if it comes from a known PIM neighbor.*/
                if (pim->interface[interfaceId].
                         smInterface->neighborList->size > 0)
                {
                    RoutingPimSearchNeighborList(
                        pim->interface[interfaceId].smInterface->neighborList,
                        srcAddr,
                        &neighbor);
                }
                if (neighbor == NULL)
                {
                    /* silently discard this msg */
                    break;
                }

                pimSmData->stats.numOfJoinPrunePacketReceived++;
#ifdef ADDON_DB
                pimSmData->smSummaryStats.m_NumJoinPruneRecvd++;
#endif
                grpInfo =
                    (RoutingPimSmJoinPruneGroupInfo*) (joinPrunePkt + 1);
                for (i = 0 ; i < joinPrunePkt->numGroups ; i++)
                {
                    encodedSrcAddr =
                        (RoutingPimEncodedSourceAddr*)(grpInfo + 1);
                    for (j = 0 ; j < grpInfo->numJoinedSrc ; j++)
                    {
                        if (DEBUG_JP)
                        {
                            char clockStr[100];
                            RoutingPimEncodedUnicastAddr* upstreamNbr =
                                                &(joinPrunePkt->upstreamNbr);
                            ctoa(node->getNodeTime(), clockStr);
                            printf("\n");
                            printf("Node %u receive JOIN_PRUNE from"
                             "node %x at "
                             "time %s\n", node->nodeId, srcAddr, clockStr);
                            printf(" the packet is for node 0x%x \n",
                                getNodeAddressFromCharArray(
                                                upstreamNbr->unicastAddr));
                            printf(" pktSrc: 0x%x  pktGrp 0x%x "
                                    "pktUpstream: 0x%x\n",
                                encodedSrcAddr->sourceAddr,
                                grpInfo->encodedGrpAddr.groupAddr,
                                getNodeAddressFromCharArray(
                                            upstreamNbr->unicastAddr));
                        }

                        RoutingPimSmHandleJoinPrunePacket(node,
                                interfaceId,
                                grpInfo,
                                encodedSrcAddr,
                                joinPrunePkt->upstreamNbr,
                                ROUTING_PIM_JOIN,
                                (clocktype)(joinPrunePkt->holdTime *SECOND),
                                srcAddr);

                        encodedSrcAddr = encodedSrcAddr + 1;
                    }
                    for (j = 0 ; j < grpInfo->numPrunedSrc ; j++)
                    {
                        if (DEBUG_JP)
                        {
                            char clockStr[100];
                            RoutingPimEncodedUnicastAddr* upstreamNbr =
                                               &(joinPrunePkt->upstreamNbr);
                            ctoa(node->getNodeTime(), clockStr);
                            printf("\n");
                            printf("Node %u receive JOIN_PRUNE from"
                             "node %x at "
                             "time %s\n", node->nodeId, srcAddr, clockStr);
                            printf(" the packet is for node 0x%x \n",
                                getNodeAddressFromCharArray(
                                                upstreamNbr->unicastAddr));
                            printf(" pktSrc: 0x%x  pktGrp 0x%x "
                                    "pktUpstream: 0x%x\n",
                                encodedSrcAddr->sourceAddr,
                                grpInfo->encodedGrpAddr.groupAddr,
                                getNodeAddressFromCharArray(
                                            upstreamNbr->unicastAddr));
                        }
                        RoutingPimSmHandleJoinPrunePacket(node,
                                interfaceId,
                                grpInfo,
                                encodedSrcAddr,
                                joinPrunePkt->upstreamNbr,
                                ROUTING_PIM_PRUNE,
                                (clocktype)(joinPrunePkt->holdTime *SECOND),
                                srcAddr);
                        encodedSrcAddr = encodedSrcAddr + 1;
                    }
                    grpInfo =
                        (RoutingPimSmJoinPruneGroupInfo*)encodedSrcAddr;
                }
                // end of message
                RoutingPimSmHandleJoinPrunePacket(node,
                                interfaceId,
                                grpInfo,
                                encodedSrcAddr,
                                joinPrunePkt->upstreamNbr,
                                ROUTING_PIM_END_OF_MESSAGE,
                                (clocktype)(joinPrunePkt->holdTime *SECOND),
                                srcAddr);
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                RoutingPimSmJoinPrunePacket* joinPrunePkt =
                    (RoutingPimSmJoinPrunePacket*) MESSAGE_ReturnPacket(msg);
                int i = 0;
                int j = 0;
                BOOL smHandled = FALSE;
                PimDataSm* pimSmData = NULL;
                PimDmData* pimDmData = NULL;
                RoutingPimSmJoinPruneGroupInfo* grpInfo;
                RoutingPimEncodedSourceAddr * encodedSrcAddr = NULL;
                RoutingPimNeighborListItem* neighbor = NULL;

                grpInfo =
                    (RoutingPimSmJoinPruneGroupInfo*) (joinPrunePkt + 1);
                for (i = 0 ; i < joinPrunePkt->numGroups ; i++)
                {
                    encodedSrcAddr =
                        (RoutingPimEncodedSourceAddr*)(grpInfo + 1);

                    if (PimSmEncodedSourceAddrGetSparseBit(
                            encodedSrcAddr->rpSimEsa))
                    {
                        pimSmData =
                            ((PimDataSmDm*) pim->pimModePtr)->pimSm;
                        /* a PIM Join/Prune message should only be accepted
                        * for processing if it comes from a known PIM
                        * neighbor.*/
                        if (pim->interface[interfaceId].
                                 smInterface->neighborList->size > 0)
                        {
                            RoutingPimSearchNeighborList(
                                pim->interface[interfaceId].
                                     smInterface->neighborList,
                                   srcAddr,
                                   &neighbor);
                        }
                        if (neighbor == NULL)
                        {
                            /* silently discard this msg */
                            break;
                        }
                        pimSmData->stats.numOfJoinPrunePacketReceived++;
                        smHandled = TRUE;
                    }
                    else
                    {
                        pimDmData =
                        ((PimDataSmDm*) pim->pimModePtr)->pimDm;
                        pimDmData->stats.joinPruneReceived++;
                    }

                    for (j = 0 ; j < grpInfo->numJoinedSrc ; j++)
                    {
                        if (DEBUG_JP)
                        {
                            char clockStr[100];
                            RoutingPimEncodedUnicastAddr* upstreamNbr =
                                       &(joinPrunePkt->upstreamNbr);

                            ctoa(node->getNodeTime(), clockStr);
                            printf("\n");
                            printf("Node %u receive JOIN_PRUNE from"
                              "node %x at "
                              "time %s\n", node->nodeId, srcAddr, clockStr);
                            printf(" the packet is for node 0x%x \n", 
                               getNodeAddressFromCharArray(
                                        upstreamNbr->unicastAddr));
                            printf(" pktSrc: 0x%x  pktGrp 0x%x pktUpstream:"
                                " 0x%x\n",
                                encodedSrcAddr->sourceAddr, 
                                grpInfo->encodedGrpAddr.groupAddr, 
                            getNodeAddressFromCharArray(
                                        upstreamNbr->unicastAddr));
                        }

                        if (PimSmEncodedSourceAddrGetSparseBit(
                            encodedSrcAddr->rpSimEsa))
                        {
                            RoutingPimSmHandleJoinPrunePacket(node,
                                       interfaceId,
                                       grpInfo,
                                       encodedSrcAddr,
                                       joinPrunePkt->upstreamNbr,
                                        ROUTING_PIM_JOIN,
                                (clocktype)(joinPrunePkt->holdTime *SECOND),
                                       srcAddr);
                        }
                        else
                        {
                            RoutingPimDmHandleCompoundJoinPrunePacket(node,
                                    interfaceId,
                                    (RoutingPimDmJoinPruneGroupInfo*)grpInfo,
                                    encodedSrcAddr,
                                    joinPrunePkt->upstreamNbr,
                                    ROUTING_PIM_JOIN,
                                    (clocktype)
                                    (joinPrunePkt->holdTime *SECOND));
                        }
                        encodedSrcAddr = encodedSrcAddr + 1;
                    }
                    for (j = 0 ; j < grpInfo->numPrunedSrc ; j++)
                    {
                        if (DEBUG_JP)
                        {
                            char clockStr[100];
                            RoutingPimEncodedUnicastAddr* upstreamNbr =
                                           &(joinPrunePkt->upstreamNbr);

                            ctoa(node->getNodeTime(), clockStr);
                            printf("\n");
                            printf("Node %u receive JOIN_PRUNE from"
                             "node %x at "
                             "time %s\n", node->nodeId, srcAddr, clockStr);
                            printf(" the packet is for node 0x%x \n", 
                                getNodeAddressFromCharArray(
                                            upstreamNbr->unicastAddr));
                            printf(" pktSrc: 0x%x  pktGrp 0x%x "
                                   "pktUpstream: 0x%x\n",
                                encodedSrcAddr->sourceAddr, 
                                grpInfo->encodedGrpAddr.groupAddr, 
                                getNodeAddressFromCharArray(
                                            upstreamNbr->unicastAddr));
                        }

                        if (PimSmEncodedSourceAddrGetSparseBit(
                            encodedSrcAddr->rpSimEsa))
                        {
                            RoutingPimSmHandleJoinPrunePacket(node,
                                   interfaceId,
                                   grpInfo,
                                   encodedSrcAddr,
                                   joinPrunePkt->upstreamNbr,
                                   ROUTING_PIM_PRUNE,
                            (clocktype)(joinPrunePkt->holdTime *SECOND), 
                                   srcAddr);
                        }
                        else
                        {
                            RoutingPimDmHandleCompoundJoinPrunePacket(node,
                                    interfaceId,
                                    (RoutingPimDmJoinPruneGroupInfo*)grpInfo,
                                    encodedSrcAddr,
                                    joinPrunePkt->upstreamNbr,
                                    ROUTING_PIM_PRUNE,
                                    (clocktype)
                                    (joinPrunePkt->holdTime *SECOND));
                        }
                        encodedSrcAddr = encodedSrcAddr + 1;
                    }
                    grpInfo =
                            (RoutingPimSmJoinPruneGroupInfo*)encodedSrcAddr;
                }
                if (smHandled)
                {
                    // end of message
                    RoutingPimSmHandleJoinPrunePacket(node,
                                interfaceId,
                                grpInfo,
                                encodedSrcAddr,
                                joinPrunePkt->upstreamNbr,
                                ROUTING_PIM_END_OF_MESSAGE,
                                (clocktype)(joinPrunePkt->holdTime *SECOND), 
                                srcAddr);
                }
            }

            break;
        }

        case ROUTING_PIM_ASSERT:
        {
            RoutingPimDmAssertPacket assertPkt;
            char* packet = MESSAGE_ReturnPacket(msg);
            RoutingPimGetAssertPacketFromBuffer(packet, &assertPkt);

            if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
                PimDmData* pimDmData = (PimDmData*) pim->pimModePtr;

                pimDmData->stats.assertReceived++;
#ifdef ADDON_DB
                pimDmData->dmSummaryStats.m_NumAssertRecvd++;
#endif
                RoutingPimDmHandleAssertPacket(
                    node,
                    srcAddr,
                    &assertPkt,
                    interfaceId);
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                PimDataSm* pimSmData = NULL;
                pimSmData = (PimDataSm*) pim->pimModePtr;

                RoutingPimSmAssertType       assertType;
                RoutingPimSmMulticastTreeState  treeState;

#ifdef ADDON_DB
                pimSmData->smSummaryStats.m_NumAssertRecvd++;
#endif
                #ifdef DEBUG
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                    printf("\n");
                    printf("Node %ld receive ASSERT from node %ld at "
                               "time %s\n", node->nodeId, srcAddr,
                               clockStr);
                    printf(" it has pkt_Grp %u & pkt_Src 0x%x \n",
                        assertPkt.groupAddr.groupAddr,
                        getNodeAddressFromCharArray(
                        assertPkt.sourceAddr.unicastAddr));
                    printf(" RPTbit %d & metricPref %d & metric %d \n",
                     PimSmAssertPacketGetRPTBit(assertPkt.metricBitPref),
                        PimSmAssertPacketGetMetricPref(
                        assertPkt.metricBitPref),
                        assertPkt.metric);
                }
                #endif
                /* a PIM Assert message should only be accepted for
                processing if it comes from a known PIM neighbor.
                A PIM router hears about PIM neighbors through PIM Hello
                messages. If a router receives an Assert message from a
                particular IP source address and it has not seen a PIM Hello
                message from that source address, then the Assert message
                SHOULD be discarded without further processing. */
                RoutingPimNeighborListItem* neighbor = NULL;

                if (pim->interface[interfaceId].
                        smInterface->neighborList->size > 0)
                {
                    RoutingPimSearchNeighborList(
                        pim->interface[interfaceId].smInterface->neighborList,
                        srcAddr,
                        &neighbor);
                }
                if (neighbor == NULL)
                {
                    break;
                }
                /* check the assert type from receieved message */
                if (PimSmAssertPacketGetRPTBit
                    (assertPkt.metricBitPref) == TRUE)
                {
                    pimSmData->stats.numOfGAssertPacketReceived++;
                    assertType = ROUTING_PIMSM_G_ASSERT;
                    treeState = ROUTING_PIMSM_G;

                    #ifdef DEBUG
                    {
                        printf(" Assert Type is %d \n", assertType);
                    }
                    #endif
                    RoutingPimSmHandleGAssertStateMachine(node, srcAddr,
                                &assertPkt, treeState, interfaceId);
                }
                else
                if (PimSmAssertPacketGetRPTBit
                    (assertPkt.metricBitPref) == FALSE)
                {
                    pimSmData->stats.numOfSGAssertPacketReceived++;
                    assertType = ROUTING_PIMSM_SG_ASSERT;
                    treeState = ROUTING_PIMSM_SG;
                    RoutingPimSmHandleSGAssertStateMachine(node, srcAddr,
                                &assertPkt, treeState, interfaceId);
                }
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                PimDataSm* pimSmData =
                        ((PimDataSmDm*) pim->pimModePtr)->pimSm;
                PimDmData* pimDmData =
                        ((PimDataSmDm*) pim->pimModePtr)->pimDm;
                RoutingPimSmAssertType       assertType;
                RoutingPimSmMulticastTreeState  treeState;

                /* check the assert type from receieved message */
                if (PimSmAssertPacketGetRPTBit
                    (assertPkt.metricBitPref) == TRUE)
                {
                    pimSmData->stats.numOfGAssertPacketReceived++;
                    assertType = ROUTING_PIMSM_G_ASSERT;
                    treeState = ROUTING_PIMSM_G;
#ifdef DEBUG
                    {
                        printf(" Assert Type is %d \n", assertType);
                    }
#endif
                    RoutingPimSmHandleGAssertStateMachine(node, srcAddr,
                                &assertPkt, treeState, interfaceId);
                }
                else
                {
                    BOOL is_sptbit_set = FALSE;
                    BOOL joinDesired = FALSE;
                    RoutingPimSmTreeInfoBaseRow  *treeInfoBaseRowPtr = NULL;
                    BOOL isRPPresent = RoutingPimIsRPPresent(node,
                                              assertPkt.groupAddr.groupAddr);

                    treeInfoBaseRowPtr =
                        RoutingPimSmSearchTreeInfoBaseForThisGroup
                                         (node,
                                          assertPkt.groupAddr.groupAddr,
                                          srcAddr,
                                          ROUTING_PIMSM_SG);
                    if (treeInfoBaseRowPtr)
                    {
                        is_sptbit_set = treeInfoBaseRowPtr->SPTbit;
                        joinDesired =
                            RoutingPimSmJoinDesiredSG(node,
                                               assertPkt.groupAddr.groupAddr,
                                               srcAddr);
                    }
                    if (isRPPresent == TRUE || joinDesired)
                    {
                        pimSmData->stats.numOfSGAssertPacketReceived++;
                        treeState = ROUTING_PIMSM_SG;
                        RoutingPimSmHandleSGAssertStateMachine(node, srcAddr,
                                        &assertPkt, treeState, interfaceId);
                    }
                    else
                    {
                        pimDmData->stats.assertReceived++;
                        RoutingPimDmHandleAssertPacket(
                            node,
                            srcAddr,
                            &assertPkt,
                            interfaceId);
                    }
                }
            }
            break;
        }

        case ROUTING_PIM_GRAFT:
        {
            RoutingPimDmGraftPacket* graftPkt =
                (RoutingPimDmGraftPacket*) MESSAGE_ReturnPacket(msg);
            int size = MESSAGE_ReturnPacketSize(msg);

            PimDmData* pimDmData = NULL;
            if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
                pimDmData = (PimDmData*) pim->pimModePtr;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimDmData = 
                    ((PimDataSmDm*) pim->pimModePtr)->pimDm;
            }
            if (pimDmData)
            {
                pimDmData->stats.graftReceived++;
#ifdef ADDON_DB
                pimDmData->dmSummaryStats.m_NumGraftRecvd++;
#endif
                RoutingPimDmHandleGraftPacket(node, srcAddr, graftPkt,
                     size, interfaceId);
            }
            break;
        }

        case ROUTING_PIM_GRAFT_ACK:
        {
            RoutingPimDmGraftAckPacket* graftAckPkt =
                (RoutingPimDmGraftAckPacket*) MESSAGE_ReturnPacket(msg);
            PimDmData* pimDmData = NULL;

            if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
                pimDmData = (PimDmData*) pim->pimModePtr;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimDmData = 
                    ((PimDataSmDm*) pim->pimModePtr)->pimDm;
            }
            if (pimDmData)
            {
                pimDmData->stats.graftAckReceived++;
#ifdef ADDON_DB
                pimDmData->dmSummaryStats.m_NumGraftAckRecvd++;
#endif
                RoutingPimDmHandleGraftAckPacket(
                    node,
                    srcAddr,
                    graftAckPkt);
            }

            break;
        }

        case ROUTING_PIM_REGISTER_STOP:
        {
            RoutingPimSmRegisterStopPacket* registerStopPkt =
                (RoutingPimSmRegisterStopPacket*) MESSAGE_ReturnPacket(msg);

            PimDataSm* pimSmData = NULL;
            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                pimSmData = (PimDataSm*) pim->pimModePtr;
            }
            if (pimSmData)
            {
                pimSmData->stats.numOfRegisterStopPacketReceived++;
#ifdef ADDON_DB
                pimSmData->smSummaryStats.m_NumRegisterStopRecvd++;
#endif
            #ifdef DEBUG
            {
                char clockStr[100];

                ctoa(node->getNodeTime(), clockStr);
                int size = MESSAGE_ReturnPacketSize(msg);
                printf("\n");
                printf("Node %u receive REGISTER_STOP from node %u at "
                           "time %s\n", node->nodeId, srcAddr, clockStr);
                printf(" it has pkt_Grp 0x%x & pkt_Src 0x%x \n",
                    registerStopPkt->encodedGroupAddr.groupAddr,
                    getNodeAddressFromCharArray(registerStopPkt->
                                                encodedSrcAddr.unicastAddr));
                printf("    PIMSM Assert packet length = %d\n", size);
            }
            #endif
            RoutingPimSmHandleRegisterStopPacket(node, srcAddr,
                    registerStopPkt);
            }
            break;
        }
        case ROUTING_PIM_REGISTER:
        {
            #ifdef DEBUG
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("\n");
                printf(" Node %u get registered packet from %u at %s\n",
                    node->nodeId, srcAddr, clockStr);
            }
            #endif
            PimDataSm* pimSmData = NULL;
            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                pimSmData = (PimDataSm*) pim->pimModePtr;
            }
            if (pimSmData)
            {
                pimSmData->stats.numOfRegisteredPacketReceived++;
#ifdef ADDON_DB
                pimSmData->smSummaryStats.m_NumRegisterRecvd++;
#endif
                RoutingPimSmHandleRegisteredPacket(node, msg, srcAddr,
                    interfaceId);
            }
            break;
        }
        case ROUTING_PIM_CANDIDATE_RP:
        {
            RoutingPimSmCandidateRPPacket* candidateRPPkt =
                (RoutingPimSmCandidateRPPacket*) MESSAGE_ReturnPacket(msg);

            PimDataSm* pimSmData = NULL;
            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                pimSmData = (PimDataSm*) pim->pimModePtr;
            }

            RoutingPimEncodedGroupAddr   * grpInfo;

            grpInfo = (RoutingPimEncodedGroupAddr*) (candidateRPPkt + 1);

            if (DEBUG_BS)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("\n");
                printf(" Node %u receive candidate_RP packet \
                       from %u at %s\n", node->nodeId, srcAddr, clockStr);
                printf(" associated RPAddr = %s \n",
                    candidateRPPkt->encodedUnicastRP.unicastAddr);
                printf("associated groupAddr = %u \n",
                            grpInfo->groupAddr);
            }
            if (pimSmData)
            {
#ifdef ADDON_DB
                pimSmData->smSummaryStats.m_NumCandidateRPRecvd++;
#endif
                RoutingPimSmHandleCandidateRPPacket(node, msg, interfaceId);
            }
            break;
        }
        case ROUTING_PIM_BOOTSTRAP:
        {
            // When PIM-SM bootstrap packets were sent from live systems 
            // JNE was only configured for PIM-DM, a seg fault 
            // occured because the pimSmData structure was accessed.
            // The below fix prints out a warning, and breaks for the 
            // case statement to not handle the packet.    
            if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
#ifdef EXATA
                if (node->partitionData->isEmulationMode)
                {
                    char buf[MAX_STRING_LENGTH];
                    sprintf(buf,
                        "Dropping bootstrap since we are in Dense Mode\n");
                    ERROR_ReportWarning(buf);
                }
#endif
                break;
            }

            if (DEBUG_BS)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("\n");
                printf(" Node %u receive bootstrap packet from %d at %s\n",
                    node->nodeId, interfaceId, clockStr);
            }

            PimDataSm* pimSmData = NULL;
            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                pimSmData = (PimDataSm*) pim->pimModePtr;
            }
            if (pimSmData)
            {
                pimSmData->stats.numOfBootstrapPacketReceived++;
#ifdef ADDON_DB
                pimSmData->smSummaryStats.m_NumBootstrapRecvd++;
#endif
                RoutingPimSmHandleBootstrapPacket(node, msg, srcAddr,
                    interfaceId);
            }
            break;
        }
        case ROUTING_PIM_STATE_REFRESH:
        {
            //The state refresh packet is not handled by QualNet
            //So silently discard the packet
            break;
        }
        default:
        {


#ifdef EXATA
            if (node->partitionData->isEmulationMode)
            {
                char buf[MAX_STRING_LENGTH];
                sprintf(buf,
                  "Unknown packet type\n");
                ERROR_ReportWarning(buf);
            }
            else
            {
                ERROR_Assert(FALSE, "Unknown packet type\n");
            }
#else
            ERROR_Assert(FALSE, "Unknown packet type\n");
#endif
            break;
        }
    }

    MESSAGE_Free(node, msg);
}

/*
*  FUNCTION     :RoutingPimDmFinalize()
*  PURPOSE      :Finalize funtion for PIM-DM
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimDmFinalize(Node* node,PimDmData* pimDmData)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Number of Hello Packets Sent = %d",
                                        pimDmData->stats.helloSent);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Triggered Hello Packets Sent = %d",
                                   pimDmData->stats.triggeredHelloSent);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Hello Packets Received = %d",
        pimDmData->stats.helloReceived);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Current Number of Neighbors = %d",
        pimDmData->stats.numNeighbor);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Join/Prune Packets Sent = %d",
        pimDmData->stats.joinPruneSent);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Join/Prune Packets Received = %d",
            pimDmData->stats.joinPruneReceived);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Join Packets Sent = %d",
        pimDmData->stats.joinSent);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Prune Packets Sent = %d",
        pimDmData->stats.pruneSent);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Join Packets Received = %d",
        pimDmData->stats.joinReceived);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Prune Packets Received = %d",
        pimDmData->stats.pruneReceived);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Graft Packets Sent = %d",
                                   pimDmData->stats.graftSent);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Graft Packets Received = %d",
        pimDmData->stats.graftReceived);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Graft ACK Packets Sent = %d",
        pimDmData->stats.graftAckSent);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Graft ACK Packets Received = %d",
            pimDmData->stats.graftAckReceived);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Assert Packets Sent = %d",
        pimDmData->stats.assertSent);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Assert Packets Received = %d",
        pimDmData->stats.assertReceived);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Data Packets Sent as Data Source = %d",
            pimDmData->stats.numDataPktOriginate);
    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Data Packets Received = %d",
        pimDmData->stats.numDataPktReceived);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Data Packets Forwarded = %d",
        pimDmData->stats.numDataPktForward);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Data Packets Discarded = %d",
        pimDmData->stats.numDataPktDiscard);

    IO_PrintStat(node, "Network", "PIM-DM", ANY_DEST, -1, buf);

    #ifdef DEBUG
    {
        fprintf(stdout, "Node %u: Number of entry in \
                        forwarding cache %d\n",
                         node->nodeId, pimDmData->fwdTable.numEntries);
        fprintf(stdout, "    Sizeof Buffer = %d\n",
            pimDmData->fwdTable.buffer.currentSize);

        RoutingPimDmPrintForwardingTable(node);
    }
    #endif
}

/*
*  FUNCTION     :RoutingPimSmFinalize()
*  PURPOSE      :Finalize funtion for PIM-DM
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimSmFinalize(Node* node,PimDataSm* pimSmData)
{
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Number of Neighbors = %d", pimSmData->stats.numNeighbor);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Receivers Joined = %d",
        pimSmData->stats.numGroupJoin);

    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Receivers Left = %d",
             pimSmData->stats.numGroupLeave);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Hello Packets Received = %d",
             pimSmData->stats.numOfHelloPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Hello Packets Sent = %d",
             pimSmData->stats.helloSent);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Triggered Hello Packets Sent = %d",
             pimSmData->stats.triggeredHelloSent);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Registered Packets Received = %d",
             pimSmData->stats.numOfRegisteredPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Registered Packets Sent = %d",
             pimSmData->stats.numOfRegisteredPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of encapsulated (Registered Packets) data "
                 " packets  Forwarded = %d",
                 pimSmData->stats.numOfRegisterDataPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Register-Stop Packets Received = %d",
             pimSmData->stats.numOfRegisterStopPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Register-Stop Packets Forwarded = %d",
             pimSmData->stats.numOfRegisterStopPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);


    sprintf(buf, "Number of Join/Prune Packets Received = %d",
             pimSmData->stats.numOfJoinPrunePacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Join/Prune Packets Forwarded = %d",
             pimSmData->stats.numOfJoinPrunePacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (*,G) Join Packets Received = %d",
        pimSmData->stats.numOfGJoinPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (*,G) Prune Packets Received = %d",
        pimSmData->stats.numOfGPrunePacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (*,G) Join Packets Forwarded = %d",
        pimSmData->stats.numOfGJoinPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (*,G) Prune Packets Forwarded = %d",
        pimSmData->stats.numOfGPrunePacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G) Join Packets Received = %d",
        pimSmData->stats.numOfSGJoinPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G) Prune Packets Received = %d",
        pimSmData->stats.numOfSGPrunePacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G) Join Packets Forwarded = %d",
        pimSmData->stats.numOfSGJoinPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G) Prune Packets Forwarded = %d",
        pimSmData->stats.numOfSGPrunePacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G,Rpt) Join Packets Received = %d",
        pimSmData->stats.numOfSGrptJoinPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G,Rpt) Prune Packets Received = %d",
        pimSmData->stats.numOfSGrptPrunePacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G,Rpt) Join Packets Forwarded = %d",
        pimSmData->stats.numOfSGrptJoinPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G,Rpt) Prune Packets Forwarded = %d",
        pimSmData->stats.numOfSGrptPrunePacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (*,G) Assert Packets Received = %d",
             pimSmData->stats.numOfGAssertPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (*,G) Assert Packets Forwarded = %d",
             pimSmData->stats.numOfGAssertPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G) Assert Packets Received = %d",
             pimSmData->stats.numOfSGAssertPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G) Assert Packets Forwarded = %d",
             pimSmData->stats.numOfSGAssertPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (*,G) Assert Cancel Packets Received = %d",
             pimSmData->stats.numOfGAssertCancelPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (*,G) Assert Cancel Packets Forwarded = %d",
             pimSmData->stats.numOfGAssertCancelPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G) Assert Cancel Packets Received = %d",
             pimSmData->stats.numOfSGAssertCancelPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of (S,G) Assert Cancel Packets Forwarded = %d",
             pimSmData->stats.numOfSGAssertCancelPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Bootstrap Packets Originated = %d",
             pimSmData->stats.numOfbootstrapPacketOriginated);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Bootstrap Packets Received = %d",
             pimSmData->stats.numOfBootstrapPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Bootstrap Packets Forwarded = %d",
             pimSmData->stats.numOfBootstrapPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Candidate-RP Packets Received = %d",
             pimSmData->stats.numOfCandidateRPPacketReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Candidate-RP Packets Forwarded = %d",
             pimSmData->stats.numOfCandidateRPPacketForwarded);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);
    sprintf(buf, "Number of Data Packets Sent As Data Source = %d",
            pimSmData->stats.numDataPktOriginate);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Data Packets Received = %d",
        pimSmData->stats.numDataPktReceived);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Data Packets Forwarded = %d",
        pimSmData->stats.numDataPktForward);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    sprintf(buf, "Number of Data Packets Discarded = %d",
        pimSmData->stats.numDataPktDiscard);
    IO_PrintStat(node, "Network", "PIM-SM", ANY_DEST, -1, buf);

    if (pimSmData->lastBSMRcvdFromCurrentBSR != NULL)
    {
        MESSAGE_Free(node,pimSmData->lastBSMRcvdFromCurrentBSR);
    }

    #ifdef DEBUG_ERRORS
        NetworkPrintForwardingTable(node);
        RoutingPimSmPrintRPList(node);
    #endif
    #ifdef DEBUG
        RoutingPimSmPrintTreeInfoBase(node);
        RoutingPimSmPrintForwardingTable(node);
    #endif
}
/*
*  FUNCTION     :RoutingPimFinalize()
*  PURPOSE      :Finalize funtion for PIM
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimFinalize(Node* node)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    /* Print out stats if user specifies it. */
    if (!pim->showStat)
    {
        return;
    }

    /* Only print statistics once per node. */
    if (pim->statPrinted== TRUE)
    {
        return;
    }

    pim->statPrinted= TRUE;


    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        PimDataSm* pimSmData = (PimDataSm*) pim->pimModePtr;
        RoutingPimSmFinalize(node,pimSmData);
    }
    else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        PimDmData* pimDmData = (PimDmData*) pim->pimModePtr;
        RoutingPimDmFinalize(node,pimDmData);
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        PimDataSm* pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
        PimDmData* pimDmData = ((PimDataSmDm*) pim->pimModePtr)->pimDm;

        RoutingPimSmFinalize(node,pimSmData);
        RoutingPimDmFinalize(node,pimDmData);
    }

#ifdef DEBUG
    {
        NetworkPrintForwardingTable(node);
    }
#endif
}
/*
*  FUNCTION     :RoutingPimDmInitStat()
*  PURPOSE      :Initializes statistics structure for Pim DM
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimDmInitStat(PimDmData* pimDmData)
    {
        pimDmData->stats.numDataPktDiscard = 0;
        pimDmData->stats.numDataPktOriginate = 0;
        pimDmData->stats.numDataPktReceived = 0;
        pimDmData->stats.numDataPktForward = 0;
        pimDmData->stats.helloSent = 0;
        pimDmData->stats.triggeredHelloSent = 0;
        pimDmData->stats.helloReceived = 0;
        pimDmData->stats.joinPruneReceived = 0;
        pimDmData->stats.joinPruneSent = 0;
        pimDmData->stats.graftSent = 0;
        pimDmData->stats.graftReceived = 0;
        pimDmData->stats.graftAckSent = 0;
        pimDmData->stats.graftAckReceived = 0;
        pimDmData->stats.assertSent = 0;
        pimDmData->stats.assertReceived = 0;
        pimDmData->stats.numNeighbor = 0;
        pimDmData->stats.joinSent = 0;
        pimDmData->stats.pruneSent = 0;
        pimDmData->stats.joinReceived = 0;
        pimDmData->stats.pruneReceived = 0;
#ifdef ADDON_DB
        pimDmData->dmSummaryStats.m_NumHelloSent = 0;
        pimDmData->dmSummaryStats.m_NumHelloRecvd = 0;
        pimDmData->dmSummaryStats.m_NumJoinPruneSent = 0;
        pimDmData->dmSummaryStats.m_NumJoinPruneRecvd = 0;
        pimDmData->dmSummaryStats.m_NumGraftSent = 0;
        pimDmData->dmSummaryStats.m_NumGraftRecvd = 0;
        pimDmData->dmSummaryStats.m_NumGraftAckSent = 0;
        pimDmData->dmSummaryStats.m_NumGraftAckRecvd = 0;
        pimDmData->dmSummaryStats.m_NumAssertSent = 0;
        pimDmData->dmSummaryStats.m_NumAssertRecvd = 0;
#endif
    }
/*
*  FUNCTION     :RoutingPimSmInitStat()
*  PURPOSE      :Initializes statistics structure for Pim SM
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimSmInitStat(PimDataSm* pimSmData)
    {
        pimSmData->stats.numDataPktDiscard = 0;
        pimSmData->stats.numDataPktOriginate = 0;
        pimSmData->stats.numDataPktReceived = 0;
        pimSmData->stats.numDataPktForward = 0;
        pimSmData->stats.helloSent = 0;
        pimSmData->stats.numNeighbor = 0;
        pimSmData->stats.numGroupJoin = 0;
        pimSmData->stats.numGroupLeave = 0;

        pimSmData->stats.numOfHelloPacketReceived = 0;

        pimSmData->stats.numOfCandidateRPPacketForwarded = 0;
        pimSmData->stats.numOfCandidateRPPacketReceived = 0;

        pimSmData->stats.numOfBootstrapPacketForwarded = 0;
        pimSmData->stats.numOfBootstrapPacketReceived = 0;

        pimSmData->stats.numOfRegisteredPacketForwarded = 0;
        pimSmData->stats.numOfRegisteredPacketReceived = 0;

        pimSmData->stats.numOfJoinPrunePacketForwarded = 0;
        pimSmData->stats.numOfJoinPrunePacketReceived = 0;

        pimSmData->stats.numOfGAssertPacketForwarded = 0;
        pimSmData->stats.numOfGAssertPacketReceived = 0;

        pimSmData->stats.numOfSGAssertPacketForwarded = 0;
        pimSmData->stats.numOfSGAssertPacketReceived = 0;

        pimSmData->stats.numOfGAssertCancelPacketForwarded = 0;
        pimSmData->stats.numOfGAssertCancelPacketReceived = 0;

        pimSmData->stats.numOfSGAssertCancelPacketForwarded = 0;
        pimSmData->stats.numOfSGAssertCancelPacketReceived = 0;

        pimSmData->stats.numOfRegisteredPacketForwarded = 0;
        pimSmData->stats.numOfRegisteredPacketReceived = 0;

        pimSmData->stats.numOfRegisterStopPacketForwarded = 0;
        pimSmData->stats.numOfRegisterStopPacketReceived = 0;
        pimSmData->stats.numOfGJoinPacketForwarded = 0;
        pimSmData->stats.numOfGPrunePacketForwarded = 0;
        pimSmData->stats.numOfGJoinPacketReceived = 0;
        pimSmData->stats.numOfGPrunePacketReceived = 0;
        pimSmData->stats.numOfSGJoinPacketForwarded = 0;
        pimSmData->stats.numOfSGPrunePacketForwarded = 0;
        pimSmData->stats.numOfSGJoinPacketReceived = 0;
        pimSmData->stats.numOfSGPrunePacketReceived = 0;
        pimSmData->stats.numOfSGrptJoinPacketForwarded = 0;
        pimSmData->stats.numOfSGrptPrunePacketForwarded = 0;
        pimSmData->stats.numOfSGrptJoinPacketReceived = 0;
        pimSmData->stats.numOfSGrptPrunePacketReceived = 0;
#ifdef ADDON_DB
        pimSmData->smSummaryStats.m_NumHelloSent = 0;
        pimSmData->smSummaryStats.m_NumHelloRecvd = 0;
        pimSmData->smSummaryStats.m_NumJoinPruneSent = 0;
        pimSmData->smSummaryStats.m_NumJoinPruneRecvd = 0;
        pimSmData->smSummaryStats.m_NumCandidateRPSent = 0;
        pimSmData->smSummaryStats.m_NumCandidateRPRecvd = 0;
        pimSmData->smSummaryStats.m_NumBootstrapSent = 0;
        pimSmData->smSummaryStats.m_NumBootstrapRecvd = 0;
        pimSmData->smSummaryStats.m_NumRegisterSent = 0;
        pimSmData->smSummaryStats.m_NumRegisterRecvd = 0;
        pimSmData->smSummaryStats.m_NumRegisterStopSent = 0;
        pimSmData->smSummaryStats.m_NumRegisterStopRecvd = 0;
        pimSmData->smSummaryStats.m_NumAssertSent = 0;
        pimSmData->smSummaryStats.m_NumAssertRecvd = 0;
#endif
    }
/*
*  FUNCTION     :RoutingPimInitStat()
*  PURPOSE      :Initializes statistics structure
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
static void RoutingPimInitStat(Node* node)
    {
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);


#ifdef ADDON_DB
    strcpy(pim->pimMulticastNetSummaryStats.m_ProtocolType,"");
    pim->pimMulticastNetSummaryStats.m_NumDataSent = 0;
    pim->pimMulticastNetSummaryStats.m_NumDataRecvd = 0;
    pim->pimMulticastNetSummaryStats.m_NumDataForwarded = 0;
    pim->pimMulticastNetSummaryStats.m_NumDataDiscarded = 0;
#endif

    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        PimDmData* pimDmData;
        pimDmData = (PimDmData*) pim->pimModePtr;

        RoutingPimDmInitStat(pimDmData);

    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        PimDataSm* pimSmData = (PimDataSm*) pim->pimModePtr;
        RoutingPimSmInitStat(pimSmData);
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        PimDataSm* pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
        PimDmData* pimDmData = ((PimDataSmDm*) pim->pimModePtr)->pimDm;
        RoutingPimDmInitStat(pimDmData);
        RoutingPimSmInitStat(pimSmData);

    }

}

/*
*  FUNCTION     :RoutingPimInitSmInterface()
*  PURPOSE      :Initializes interface structure
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
 */
void RoutingPimInitSmInterface(Node* node,
                          RoutingPimSmInterface* smInterface)
{
    PimData* pim = (PimData*)
    NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    /* Gen Id should be unique at startup */
    smInterface->helloGenerationId = RANDOM_nrand(pim->seed);
    smInterface->neighborCount = 0;

    ListInit(node, &smInterface->neighborList);
    return;
}

/*
*  FUNCTION     :RoutingPimInitDmInterface()
*  PURPOSE      :Initializes interface structure
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
 */
void RoutingPimInitDmInterface(Node* node,
                          RoutingPimDmInterface* dmInterface,
                          BOOL isDense,
                          RoutingPimSmInterface* smInterface = NULL)
{
    PimData* pim = (PimData*)
    NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    if (isDense)
    {
        dmInterface->helloGenerationId = RANDOM_nrand(pim->seed);
    }
    else
    {
        dmInterface->helloGenerationId = smInterface->helloGenerationId;
    }
    dmInterface->isLeaf = TRUE;
    dmInterface->neighborCount = 0;
    ListInit(node, &dmInterface->neighborList);
    ListInit(node, &dmInterface->assertSourceList);
    return;
}

/*
*  FUNCTION     :RoutingPimInitInterface()
*  PURPOSE      :Initializes interface structure
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
 */
static void RoutingPimInitInterface(Node* node,
                                    const NodeInput* nodeInput,
                                    int initInterface)
{
    BOOL retVal = FALSE;
    clocktype buf = 0;
    int drPriority = 0;
    int i = 0;
    char bufStr[MAX_STRING_LENGTH];
    char bufVal[MAX_STRING_LENGTH];
    BOOL switchToSpt = FALSE;

#ifdef CYBER_CORE
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
#endif //CYBER_CORE

    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimSmData = NULL;
    PimDmData* pimDmData = NULL;

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
        pimDmData = ((PimDataSmDm*) pim->pimModePtr)->pimDm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*) pim->pimModePtr;
    }
    BOOL staticRPPresent = FALSE;
    unsigned char highestPriority = 0;
    NodeAddress highestIPAddress =(unsigned int) NETWORK_UNREACHABLE;
    clocktype delay = 0;
    int interfaceIndex = 0;
    RoutingPimSmBootstrapTimerInfo timerInfo;
    memset(&timerInfo, 0,sizeof(RoutingPimSmBootstrapTimerInfo));

    if ((pim->modeType == ROUTING_PIM_MODE_SPARSE) ||
       (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE))
    {
        staticRPPresent = RoutingPimSmCheckForStaticRP(node,
                                                       nodeInput);
    }
#ifdef DEBUG
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("In function RoutingPimInitInterface at time %s\n",clockStr);
    }
#endif
    RoutingPimDmInterface* dmInterface = NULL;
    RoutingPimSmInterface* smInterface = NULL;

    pim->interface = (RoutingPimInterface*)
            MEM_malloc(node->numberInterfaces * sizeof(RoutingPimInterface));

    for (i = 0; i < node->numberInterfaces; i++)
    {
        memset(&pim->interface[i],
               0,
               sizeof(RoutingPimInterface));
        if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
        {
            pim->interface[i].dmInterface = (RoutingPimDmInterface*)
                MEM_malloc(sizeof(RoutingPimDmInterface));
            memset(pim->interface[i].dmInterface,
               0,
               sizeof(RoutingPimDmInterface));

            pim->interface[i].smInterface = (RoutingPimSmInterface*)
                MEM_malloc(sizeof(RoutingPimSmInterface));
            memset(pim->interface[i].smInterface,
               0,
               sizeof(RoutingPimSmInterface));

            dmInterface = pim->interface[i].dmInterface;
            smInterface = pim->interface[i].smInterface;
        }
        else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
        {
            pim->interface[i].dmInterface = (RoutingPimDmInterface*)
                MEM_malloc(sizeof(RoutingPimDmInterface));
            memset(pim->interface[i].dmInterface,
               0,
               sizeof(RoutingPimDmInterface));
            dmInterface = pim->interface[i].dmInterface;
        }
        else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
        {
            pim->interface[i].smInterface = (RoutingPimSmInterface*)
                MEM_malloc(sizeof(RoutingPimSmInterface));
            memset(pim->interface[i].smInterface,
               0,
               sizeof(RoutingPimSmInterface));
            smInterface = pim->interface[i].smInterface;


            /* Determine whether or not to switch direct to SPT */
            NodeAddress intfAddress = NetworkIpGetInterfaceAddress(node, i);
            retVal = FALSE;
            bufVal[0] = '\0';
            switchToSpt = FALSE;
            IO_ReadBool(node->nodeId,
                        intfAddress,
                        node->partitionData->nodeInput,
                        "ROUTING-PIMSM-DIRECT-SPT",
                        &retVal,
                        &switchToSpt);

            if (retVal)
            {
                pim->interface[i].switchDirectToSPT = switchToSpt;
                retVal = FALSE;
                bufVal[0] = '\0';
                pim->interface[i].srcGrpList = NULL;

                IO_ReadString(node->nodeId,
                              intfAddress,
                              node->partitionData->nodeInput,
                              "ROUTING-PIMSM-SOURCE-GROUP-PAIR",
                              &retVal,
                              bufVal);

                ListInit(node, &pim->interface[i].srcGrpList);
                if (retVal)
                {
                    pim->interface[i].srcGrpList =
                      RoutingPimParseSourceGroupStr(node,
                                            bufVal,
                                            pim->interface[i].srcGrpList,
                                            interfaceIndex);
                }
            }
            else
            {
                pim->interface[i].switchDirectToSPT = FALSE;
            }
        }
        /* Determine interface type */
        if (NetworkIpIsWiredBroadcastNetwork(node, i) ||
            ((!TunnelIsVirtualInterface(node, i)) &&
            MAC_IsOneHopBroadcastNetwork(node,i)))
        {
#ifdef HITL_INTERFACE
            if (i >= initInterface)
#endif
            {
                pim->interface[i].interfaceType =
                    ROUTING_PIM_BROADCAST_INTERFACE;
            }
            if (dmInterface)
            {
                dmInterface->designatedRouter =
                    NetworkIpGetInterfaceAddress(node, i);
            }
            if (smInterface)
            {
                smInterface->drInfo.ipAddr =
                    NetworkIpGetInterfaceAddress(node, i);
            }
        }
#ifdef CYBER_CORE
        else if ((ip->iahepEnabled) && (ip->iahepData->nodeType == RED_NODE) &&
                IsIAHEPRedSecureInterface(node, i))
        {
            pim->interface[i].interfaceType = ROUTING_PIM_BROADCAST_INTERFACE;

            if (dmInterface)
            {
                dmInterface->designatedRouter =
                    NetworkIpGetInterfaceAddress(node, i);
            }
            if (smInterface)
            {
                smInterface->drInfo.ipAddr =
                    NetworkIpGetInterfaceAddress(node, i);
            }
        }
#endif //CYBER_CORE
        else
        {
#ifdef HITL_INTERFACE
            if (i >= initInterface)
#endif
            {
                pim->interface[i].interfaceType =
                    ROUTING_PIM_POINT_TO_POINT_INTERFACE;
            }
            if (dmInterface)
            {
                dmInterface->designatedRouter =
                    NetworkIpGetInterfaceAddress(node, i);
            }
            if (smInterface)
            {
                smInterface->drInfo.ipAddr =
                    NetworkIpGetInterfaceAddress(node, i);
            }
        }

#ifdef ADDON_BOEINGFCS
            if (dmInterface)
            {
                MulticastCesRpimCheckRpimEnabled(node,
                i,
                node->partitionData->nodeInput,
                &dmInterface->designatedRouter);
            }
            if (smInterface)
            {
                MulticastCesRpimCheckRpimEnabled(node,
                i,
                node->partitionData->nodeInput,
                &smInterface->drInfo.ipAddr);
            }

#endif


        pim->interface[i].ipAddress = NetworkIpGetInterfaceAddress(node, i);
        pim->interface[i].subnetMask =
            NetworkIpGetInterfaceSubnetMask(node, i);

        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, i),
            node->partitionData->nodeInput,
            "ROUTING-PIM-HELLO-PERIOD",
            &retVal,
            &buf);

        if (retVal == FALSE)
        {
            if (dmInterface)
            {
                dmInterface->helloInterval =
                    ROUTING_PIM_HELLO_PERIOD;
            }
            if (smInterface)
            {
                smInterface->helloInterval =
                    ROUTING_PIM_HELLO_PERIOD;
            }
        }
        else
        {
            if (buf <= 0)
            {
                char warning[MAX_STRING_LENGTH];
                warning[0] = '\0';
                sprintf(warning,"Node : %d , ROUTING-PIM-HELLO-PERIOD "
                                "should be >= 0 at interface %d",
                                node->nodeId, i);
                ERROR_ReportWarning(warning);

                //continue with default values
                if (dmInterface)
                {
                    dmInterface->helloInterval =
                        ROUTING_PIM_HELLO_PERIOD;
                }
                if (smInterface)
                {
                    smInterface->helloInterval =
                        ROUTING_PIM_HELLO_PERIOD;
                }
            }
            else
            {
                if (dmInterface)
                {
                    dmInterface->helloInterval = buf;
                }
                if (smInterface)
                {
                    smInterface->helloInterval = buf;
                }
            }
        }

        if (pim->modeType != ROUTING_PIM_MODE_DENSE && smInterface)
        {
            IO_ReadInt(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, i),
                node->partitionData->nodeInput,
                "ROUTING-PIM-DR-PRIORITY",
                &retVal,
                &drPriority);

            if (retVal == FALSE)
            {
                smInterface->DRPriority = PIM_DR_PRIORITY;
            }
            else
            {
                if (drPriority < 0)
                {
                    char warning[MAX_STRING_LENGTH];
                    warning[0] = '\0';
                    sprintf(warning,"Node : %d , ROUTING-PIM-DR-PRIORITY "
                                    "should be > 0 at interface %d",
                                    node->nodeId, i);
                    ERROR_ReportWarning(warning);

                    //continue with default values
                    smInterface->DRPriority = PIM_DR_PRIORITY;
               }
               else
               {
                   smInterface->DRPriority = (unsigned int) drPriority;
               }
            }
        }

        if (pimDmData)
        {
            retVal = FALSE;
            bufStr[0] = '\0';
            IO_ReadString(node->nodeId,
                          NetworkIpGetInterfaceAddress(node, i),
                          node->partitionData->nodeInput,
                          "ROUTING-PIMDM-HELLO-SUPPRESSION",
                          &retVal,
                          bufStr);

            if (retVal)
            {
                if (strcmp (bufStr, "YES") == 0
                    ||
                    strcmp (bufStr, "yes") == 0)
                {
                    pim->interface[i].helloSuppression = TRUE;
                }
                else
                {
                    pim->interface[i].helloSuppression = FALSE;
                }
            }
            else
            {
                pim->interface[i].helloSuppression = FALSE;
            }

            retVal = FALSE;
            bufStr[0] = '\0';
            IO_ReadString(node->nodeId,
                          NetworkIpGetInterfaceAddress(node, i),
                          node->partitionData->nodeInput,
                          "ROUTING-PIMDM-BROADCAST-MODE",
                          &retVal,
                          bufStr);

            if (retVal)
            {
                if (strcmp (bufStr, "YES") == 0
                    ||
                    strcmp (bufStr, "yes") == 0)
                {
                    pim->interface[i].broadcastMode = TRUE;
                }
                else
                {
                    pim->interface[i].broadcastMode = FALSE;
                }
            }
            else
            {
                pim->interface[i].broadcastMode = FALSE;
            }

            retVal = FALSE;
            bufStr[0] = '\0';
            IO_ReadString(node->nodeId,
                          NetworkIpGetInterfaceAddress(node, i),
                          node->partitionData->nodeInput,
                          "ROUTING-PIMDM-JOIN-PRUNE-SUPPRESSION",
                          &retVal,
                          bufStr);

            if (retVal)
            {
                if (strcmp (bufStr, "YES") == 0
                    ||
                    strcmp (bufStr, "yes") == 0)
                {
                    pim->interface[i].joinPruneSuppression = TRUE;
                }
                else
                {
                    pim->interface[i].joinPruneSuppression = FALSE;
                }
            }
            else
            {
                pim->interface[i].joinPruneSuppression = FALSE;
            }

            retVal = FALSE;
            bufStr[0] = '\0';
            IO_ReadString(node->nodeId,
                          NetworkIpGetInterfaceAddress(node, i),
                          node->partitionData->nodeInput,
                          "ROUTING-PIMDM-ASSERT-OPTIMIZATION",
                          &retVal,
                          bufStr);

            if (retVal)
            {
                if (strcmp (bufStr, "YES") == 0
                    ||
                    strcmp (bufStr, "yes") == 0)
                {
                    pim->interface[i].assertOptimization = TRUE;
                }
                else
                {
                    pim->interface[i].assertOptimization = FALSE;
                }
            }
            else
            {
                pim->interface[i].assertOptimization = FALSE;
            }
        }

        #ifdef ADDON_BOEINGFCS
        IO_ReadBool(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, i),
            node->partitionData->nodeInput,
            "ROUTING-PIM-USE-MPR",
            &retVal,
            (int*) &pim->interface[i].useMpr);

        if (retVal == FALSE)
        {
            pim->interface[i].useMpr = FALSE;
        }
#endif // ADDON_BOEINGFCS
        if (dmInterface && smInterface)
        {
            RoutingPimInitSmInterface(node,smInterface);
            RoutingPimInitDmInterface(node,dmInterface,FALSE,smInterface);
        }
        else if (smInterface)
        {
            RoutingPimInitSmInterface(node,smInterface);
        }
        else if (dmInterface)
        {
            RoutingPimInitDmInterface(node,dmInterface,TRUE);
        }
        if (pim->modeType == ROUTING_PIM_MODE_DENSE)
        {
            continue;
        }

        pim->interface[i].currentBSR = 0;

        pim->interface[i].currentBSRPriority = 0;

        pim->interface[i].bootstrapTimerSeq = 0;
        pim->interface[i].candidateRPTimerSeq = 0;

        if (staticRPPresent == TRUE )
        {
            continue;  //to for loop
        }

        //otherwise read for candidate RP
        ListInit(node, &pim->interface[i].rpData.rpAccessList);

        if (RoutingPimIsPimEnabledInterface(node, i))
            RoutingPimSmCheckForCandidateRP(node, i, nodeInput);
        if (RoutingPimIsPimEnabledInterface(node, i))
            RoutingPimSmCheckForCandidateBSR(node, i, nodeInput);

        /* we consider each CRP is a CBSR */
        if (pim->interface[i].RPFlag
#ifdef ADDON_BOEINGFCS
            || (MulticastCesRpimIsRouter(node, i) && RoutingPimIsPimEnabledInterface(node, i))
#endif
            )
        {
            RoutingPimSmCandidateRPTimerInfo rpTimerInfo;
            clocktype delay = 0;

            /*
            scheduling timer for sending candidate rp packet because it might
            happen that the C-RP is the only BSR and when it will originate
            the BSM,its directly connected nodes will not send that BSM on
            iif so this C-RP(which is also a BSR)never get the BSM and hence
            will never create the C-RP adv.So to avoid that scheduling a
            timer
            */
            PimDataSm* pimDataSm = NULL;
            if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                pimDataSm = (PimDataSm*) pim->pimModePtr;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
            }
            delay = (clocktype) (RANDOM_nrand(pimDataSm->seed)
                                 % pimDataSm->routingPimSmTriggeredDelay);

            rpTimerInfo.srcAddr = NetworkIpGetInterfaceAddress(node,
                                                               i);

            rpTimerInfo.intfIndex = i;
            rpTimerInfo.seqNo = pim->interface[i].candidateRPTimerSeq + 1;
            pim->interface[i].candidateRPTimerSeq++;
#ifdef DEBUG
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("Scheduling MSG_ROUTING_PimSmScheduleCandidateRP"
            " at time %s for int %d with delay %ld\n",clockStr, i,
            delay);
    }
#endif
#ifdef ADDON_BOEINGFCS
            MulticastCesRpimUpdateRpDelay(node,
                i,
                &delay);
#endif
            RoutingPimSetTimer(node,
                               rpTimerInfo.intfIndex,
                               MSG_ROUTING_PimSmScheduleCandidateRP,
                               (void*) &rpTimerInfo,
                               delay);

        }

        /*
        RFC:5059::Section 3.1.1
        The initial state for this configured scope zone is "Pending-BSR";
        the Bootstrap Timer is initialized to BS_Rand_Override.
        */
        if (pim->interface[i].BSRFlg == TRUE)//For Candidate BSR
        {
            pim->interface[i].cBSRState = PENDING_BSR;

            /*
            RFC:5059::Section 3.1.4
            If a router is in P-BSR state, then it uses its
            own weight as that of the current BSR.
            */
            pim->interface[i].currentBSR = NetworkIpGetInterfaceAddress(node,
                                                                        i);
            pim->interface[i].currentBSRPriority =
                pim->interface[i].bsrData.bsrPriority;
        }
        else  //For Non-Candidate BSR
        {
            /*
            RFC:5059::Section 3.1.2
            The initial state for this state machine is "Accept Any".
            */
            pim->interface[i].ncBSRState = ACCEPT_ANY;
        }

        //Among all the BSR enabled interfaces,internally first select which
        //interface can be the elected one
        if (pim->interface[i].BSRFlg == TRUE)
        {
            if (pim->interface[i].bsrData.bsrPriority > highestPriority)
            {
                highestPriority = pim->interface[i].bsrData.bsrPriority;
                highestIPAddress = pim->interface[i].ipAddress;
                continue;
            }

            if (pim->interface[i].bsrData.bsrPriority < highestPriority)
            {
                continue;
            }

            if (pim->interface[i].ipAddress > highestIPAddress)
            {
                highestPriority = pim->interface[i].bsrData.bsrPriority;
                highestIPAddress = pim->interface[i].ipAddress;
                continue;
            }
        }
    }
    // dynamic address change
    // registering RoutingPimHandleAddressChangeEvent function
     NetworkIpAddAddressChangedHandlerFunction(node,
                            &RoutingPimHandleChangeAddressEvent);
    
    if ((staticRPPresent == TRUE )
        || (pim->modeType == ROUTING_PIM_MODE_DENSE)
        || (highestIPAddress == (unsigned int)NETWORK_UNREACHABLE))
    {
        return;
    }

    interfaceIndex = NetworkIpGetInterfaceIndexFromAddress(node,
                                                           highestIPAddress);

    if (pim->interface[interfaceIndex].BSRFlg == TRUE)
    {
        timerInfo.srcAddr = highestIPAddress;
        timerInfo.intfIndex = interfaceIndex;

        timerInfo.seqNo =
            pim->interface[interfaceIndex].bootstrapTimerSeq + 1;
        /* Note the current bootstrapTimerSeq */
        pim->interface[interfaceIndex].bootstrapTimerSeq++;

        delay = (clocktype)(RoutingPimSmCalculateBSRandOverride(node,
                                                      timerInfo.intfIndex));
#ifdef DEBUG
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("Scheduling MSG_ROUTING_PimSmBootstrapTimeOut"
            " at time %s for int %d \n",clockStr, interfaceIndex);
    }
#endif
        RoutingPimSetTimer(node,
                           timerInfo.intfIndex,
                           MSG_ROUTING_PimSmBootstrapTimeOut,
                           (void*) &timerInfo,
                           delay);
    }

    /*
        If the router is the RP then its RP interfaces will also be BSR
        so setting their current BSR as the highest IP address of its own
        interfaces
    */
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if ((pim->interface[i].RPFlag == TRUE)
            ||
            pim->interface[i].BSRFlg == TRUE)
        {
            pim->interface[i].currentBSR = highestIPAddress;
            pim->interface[i].currentBSRPriority = highestPriority;
        }
    }//end of for
}


/**************************************************************************/
/*                            PIM HELLO                                   */
/**************************************************************************/

/*
*  FUNCTION     :RoutingPimProcessHelloPacket()
*  PURPOSE      :Process received hello packet's option field
*  ASSUMPTION   :None
*  RETURN VALUE :BOOL
*/
BOOL RoutingPimProcessHelloPacket(RoutingPimHelloPacket* helloPkt,
        int pktSize, unsigned short* holdTime, unsigned int* genId,
        unsigned int*  DRpriority, BOOL *isDRPrioritypresent)
{
    RoutingPimHelloPacketOption* option;
    int dataLen;
    char* dataPtr;

    dataLen = pktSize - sizeof(RoutingPimCommonHeaderType);
    dataPtr = ((char*) helloPkt) + sizeof(RoutingPimCommonHeaderType);

    /* Ignore any data shorter than PIM Hello header */
    while (dataLen >= (signed) sizeof(RoutingPimHelloPacket))
    {
        option = (RoutingPimHelloPacketOption*) dataPtr;

        switch (option->optionType)
        {
            case ROUTING_PIM_HOLD_TIME:
            {
                if (ROUTING_PIM_HOLDTIME_LENGTH != option->optionLength)
                {
                    #ifdef DEBUG
                    {
                        fprintf(stdout, "Invalid option length in \
                                        Hello packet."
                                        " Stop processing packet\n");
                    }
                    #endif

                    return FALSE;
                }

                dataPtr += sizeof(RoutingPimHelloPacketOption);
                memcpy(holdTime, dataPtr, option->optionLength);
                break;
            }

            case ROUTING_PIM_DR_PRIORITY:
            {
                if (ROUTING_PIM_DR_PRIORITY_LENGTH != option->optionLength)
                {
                    #ifdef DEBUG
                    {
                        fprintf(stdout, "Invalid option length "
                                        "in DR priority "
                                        " of Hello packet."
                                        " Stop processing packet\n");
                    }
                    #endif

                    return FALSE;
                }

                dataPtr += sizeof(RoutingPimHelloPacketOption);
                memcpy(DRpriority, dataPtr, option->optionLength);
                *isDRPrioritypresent = TRUE;
                break;
            }

            case ROUTING_PIM_GEN_ID:
            {
                if (ROUTING_PIM_GEN_ID_LENGTH != option->optionLength)
                {
                    #ifdef DEBUG
                    {
                        fprintf(stdout, "Invalid option length in \
                                        Hello packet."
                                        " Stop processing packet\n");
                    }
                    #endif

                    return FALSE;
                }

                dataPtr += sizeof(RoutingPimHelloPacketOption);
                memcpy(genId, dataPtr, option->optionLength);

                break;
            }

            default:
            {
                /* Ignore any other option */
                dataPtr += sizeof(RoutingPimHelloPacketOption);
            }
        }

        dataPtr += option->optionLength;
        dataLen -= (sizeof(RoutingPimHelloPacketOption)
                           + option->optionLength);
    }

    return TRUE;
}


/*
*  FUNCTION     :RoutingPimSendHelloPacket()
*  PURPOSE      :Send hello packet on each interface
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimSendHelloPacket(Node* node,
                               int interfaceIndex,
                               unsigned short holdTime,
                               NodeAddress oldAddress)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    Message* helloMsg = NULL;
    RoutingPimHelloPacket* helloPkt = NULL;
    RoutingPimHelloPacketOption* option = NULL;
    clocktype delay = 0;
    int i = 0;
    RoutingPimDmInterface* dmInterface = NULL;
    RoutingPimSmInterface* smInterface = NULL;
    PimDataSm* pimDataSm = NULL;
    PimDmData* pimDataDm = NULL;

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        dmInterface = pim->interface[interfaceIndex].dmInterface;
        smInterface = pim->interface[interfaceIndex].smInterface;
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
        pimDataDm = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        dmInterface = pim->interface[interfaceIndex].dmInterface;
        pimDataDm = (PimDmData*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        smInterface = pim->interface[interfaceIndex].smInterface;
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }

    i = interfaceIndex;
    //for (i = 0; i < node->numberInterfaces; i++)
    //{
    char buf[MAX_STRING_LENGTH];
    char* dataPtr;
    int dataLen;
    unsigned short tempVal;

    /* Skip those interfaces that are not PIM enabled */
    if (!RoutingPimIsPimEnabledInterface(node, i))
    {
        return;
    }

#ifdef ADDON_BOEINGFCS
    if (pim->modeType != ROUTING_PIM_MODE_DENSE)
    {
        BOOL returnHere = FALSE;
        returnHere = MulticastCesRpimUpdateNeighbors(node, i);
        if (returnHere)
        {
            return;
        }
    }
#endif

    /* Create PIM Common header */
    RoutingPimCreateCommonHeader((RoutingPimCommonHeaderType*) buf,
                                 ROUTING_PIM_HELLO);

    dataPtr = buf + sizeof(RoutingPimCommonHeaderType);

    /* Hold Time option */
    option = (RoutingPimHelloPacketOption*) dataPtr;
    option->optionType = (unsigned short) ROUTING_PIM_HOLD_TIME;
    option->optionLength = ROUTING_PIM_HOLDTIME_LENGTH;
    dataPtr += sizeof(RoutingPimHelloPacketOption);
    if (smInterface)
    {
        tempVal = (unsigned short)
            ((smInterface->helloInterval * 3.5) / SECOND);
    }
    else
    {
        tempVal = (unsigned short) 
            ((dmInterface->helloInterval * 3.5) / SECOND);
    }
    if (oldAddress != 0)
    {
        tempVal = 0;
    }
    memcpy(dataPtr, &tempVal, sizeof(unsigned short));
    dataPtr += sizeof(unsigned short);

    /* Priority option */
    if (pim->modeType != ROUTING_PIM_MODE_DENSE && smInterface)
    {
        option = (RoutingPimHelloPacketOption*) dataPtr;
        option->optionType = (unsigned short) ROUTING_PIM_DR_PRIORITY;
        option->optionLength = ROUTING_PIM_DR_PRIORITY_LENGTH;
        dataPtr += sizeof(RoutingPimHelloPacketOption);
    
        memcpy(dataPtr, &smInterface->DRPriority, sizeof(int));
        dataPtr += sizeof(int);
    }

    /* Generation Id option */
    option = (RoutingPimHelloPacketOption*) dataPtr;
    option->optionType = (unsigned short) ROUTING_PIM_GEN_ID;
    option->optionLength = ROUTING_PIM_GEN_ID_LENGTH;
    dataPtr += sizeof(RoutingPimHelloPacketOption);
    if (smInterface)
    {
        memcpy(dataPtr, &smInterface->helloGenerationId, sizeof(int));
    }
    else
    {
        memcpy(dataPtr, &dmInterface->helloGenerationId, sizeof(int));
    }
    dataPtr += sizeof(int);

    /* Length of PIM Packet */
    dataLen = (int) (dataPtr - buf);

    #ifdef DEBUG_ERROR
    {
        fprintf(stdout, "    PIM Hello packet length = %d\n", dataLen);
    }
    #endif
    /* Create Hello packet */
    helloMsg = MESSAGE_Alloc(node,
                              NETWORK_LAYER,
                              MULTICAST_PROTOCOL_PIM,
                              MSG_ROUTING_PimPacket);

    MESSAGE_PacketAlloc(node, helloMsg, dataLen, TRACE_PIM);
    helloPkt = (RoutingPimHelloPacket*) MESSAGE_ReturnPacket(helloMsg);
    memcpy(helloPkt, buf, dataLen);

    /* Used to jitter Hello packet broadcast */
    delay = (clocktype)
    RANDOM_erand(pim->seed)*  ROUTING_PIM_BROADCAST_JITTER;
    NodeAddress srcAddr = 0; // dynamic address change
    /* Now send packet out through this interface */
    if (oldAddress != 0)
    {
        srcAddr = oldAddress;
    }
    else
    {
        srcAddr = NetworkIpGetInterfaceAddress(node, i);
    }
    NetworkIpSendRawMessageToMacLayerWithDelay(
                    node,
                    MESSAGE_Duplicate(node, helloMsg),
                    srcAddr,
                    ALL_PIM_ROUTER,
                    IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_PIM,
                    1,
                    i,
                    ANY_DEST,
                    delay);

    if (pimDataSm)
    {
        pimDataSm->stats.helloSent++;
    }
    if (pimDataDm)
    {
        pimDataDm->stats.helloSent++;
    }
#ifdef ADDON_DB
    if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        PimDmData* pimDmData = (PimDmData*) pim->pimModePtr;
        pimDmData->dmSummaryStats.m_NumHelloSent++;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        PimDataSm* pimSmData = (PimDataSm*) pim->pimModePtr;
        pimSmData->smSummaryStats.m_NumHelloSent++;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        PimDataSm* pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
        PimDmData* pimDataDm = ((PimDataSmDm*)pim->pimModePtr)->pimDm;

        pimDataSm->smSummaryStats.m_NumHelloSent++;
        pimDataDm->dmSummaryStats.m_NumHelloSent++;
    }
#endif
    MESSAGE_Free(node, helloMsg);
//}

    if (DEBUG_HELLO)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        fprintf(stdout, "Node %u @ %s: Sending HELLO on interface %d.\n",
                         node->nodeId,
                         clockStr,
                         interfaceIndex);
    }
}


/*
*  FUNCTION     :RoutingPimCreateCommonHeader()
*  PURPOSE      :Create PIM common header
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimCreateCommonHeader(RoutingPimCommonHeaderType* header,
                        unsigned char pktCode)
{
    RoutingPimCommonHeaderSetVar(&(header->rpChType), PIM_VERSION);
    RoutingPimCommonHeaderSetType(&(header->rpChType), pktCode);
    header->reserved = 0;
    header->checksum = 0;
}

/*
*  FUNCTION     :RoutingPimIsPimEnabledInterface()
*  PURPOSE      :Determine whether a specific interface is pim enabled or not
*  ASSUMPTION   :None
*  RETURN VALUE :BOOL
 */
BOOL RoutingPimIsPimEnabledInterface(Node* node,
    int interfaceId)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;

    if ((ipLayer->interfaceInfo[interfaceId]->multicastEnabled == FALSE) ||
        (ipLayer->interfaceInfo[interfaceId]->multicastProtocolType !=
             MULTICAST_PROTOCOL_PIM))
    {
        return (FALSE);
    }

    return (TRUE);
}

/*
*  FUNCTION     :RoutingPimSearchNeighborList()
*  PURPOSE      :Search neighbor list for specific neighbor
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimSearchNeighborList(LinkedList* list, NodeAddress addr,
    RoutingPimNeighborListItem* *item)
{
    ListItem* listItem = NULL;

#ifndef EXATA
    ERROR_Assert(list != NULL, "Neighbor list is not initialized");
#endif
    listItem = list->first;

    while ((listItem != NULL) && (((RoutingPimNeighborListItem*)
               listItem->data)->ipAddr != addr))
    {
        listItem = listItem->next;
    }

    * item = (RoutingPimNeighborListItem*)
    ((!listItem) ? NULL : listItem->data);
}

/*
*  FUNCTION     :RoutingPimSetTimer()
*  PURPOSE      :Set timer
*  ASSUMPTION   :Router doesn't restarted during simulation
*  RETURN VALUE :NULL
 */
void RoutingPimSetTimer(Node* node,
                        int interfaceIndex,
                        int eventType,
                        void* data,
                        clocktype delay)
{
    Message* newMsg = NULL;

    newMsg = MESSAGE_Alloc(node, NETWORK_LAYER,
                            MULTICAST_PROTOCOL_PIM,
                            eventType);

    MESSAGE_AddInfo(node,
        newMsg,
        sizeof(int),
        INFO_TYPE_PhyIndex);

    memcpy(MESSAGE_ReturnInfo(newMsg, INFO_TYPE_PhyIndex),
           &interfaceIndex,
           sizeof(int));

    ERROR_Assert(delay >= 0, "Delay can't be negative in Set Timer");

    switch (eventType)
    {
        case MSG_ROUTING_PimDmNeighborTimeOut:
        {
            MESSAGE_InfoAlloc(node, newMsg, sizeof(RoutingPimDmNbrTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimDmNbrTimerInfo));

            if (DEBUG_TIMER)
            {
                RoutingPimDmNbrTimerInfo* timerInfo =
                    (RoutingPimDmNbrTimerInfo*) data;

                char neighborStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->nbrAddr,
                                            neighborStr);
                char delayStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(delay, delayStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout,"Node %u @ %s: Setting timer "
                               "MSG_ROUTING_PimDmNeighborTimeOut for "
                               "Neighbor %s with delay %s\n",
                               node->nodeId,
                               clockStr,
                               neighborStr,
                               delayStr);
            }

            break;
        }

        case MSG_ROUTING_PimDmDataTimeOut:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                sizeof(RoutingPimDmDataTimeoutInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimDmDataTimeoutInfo));

            if (DEBUG_TIMER)
            {
                RoutingPimDmDataTimeoutInfo* timerInfo =
                    (RoutingPimDmDataTimeoutInfo*) data;

                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->grpAddr,
                                            grpStr);
                char delayStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(delay, delayStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout,"Node %u @ %s: Setting timer "
                               "MSG_ROUTING_PimDmDataTimeOut for "
                               "(%s , %s) with delay %s\n",
                               node->nodeId,
                               clockStr,
                               srcStr,
                               grpStr,
                               delayStr);
            }

            break;
        }

        case MSG_ROUTING_PimDmScheduleJoin:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                    sizeof(RoutingPimDmDelayedJoinTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimDmDelayedJoinTimerInfo));

            if (DEBUG_TIMER)
            {
                RoutingPimDmDelayedJoinTimerInfo* timerInfo = 
                    (RoutingPimDmDelayedJoinTimerInfo*) data;

                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->grpAddr,
                                            grpStr);
                char delayStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(delay, delayStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout,"Node %u @ %s: Setting timer "
                               "MSG_ROUTING_PimDmScheduleJoin for "
                               "(%s , %s) with delay %s\n",
                               node->nodeId,
                               clockStr,
                               srcStr,
                               grpStr,
                               delayStr);
            }
            break;
        }

        case MSG_ROUTING_PimDmJoinTimeOut:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                    sizeof(RoutingPimDmDelayedPruneTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimDmDelayedPruneTimerInfo));

            if (DEBUG_TIMER)
            {
                RoutingPimDmDelayedPruneTimerInfo* timerInfo =
                    (RoutingPimDmDelayedPruneTimerInfo*) data;

                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->grpAddr,
                                            grpStr);
                char delayStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(delay, delayStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout,"Node %u @ %s: Setting timer "
                               "MSG_ROUTING_PimDmJoinTimeOut for "
                               "(%s , %s) with delay %s\n",
                               node->nodeId,
                               clockStr,
                               srcStr,
                               grpStr,
                               delayStr);
            }

            break;
        }

        case MSG_ROUTING_PimDmPruneTimeoutAlarm:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                sizeof(RoutingPimDmPruneTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimDmPruneTimerInfo));

            if (DEBUG_TIMER)
            {
                RoutingPimDmPruneTimerInfo* timerInfo =
                    (RoutingPimDmPruneTimerInfo*) data;

                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->grpAddr,
                                            grpStr);
                char delayStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(delay, delayStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout,"Node %u @ %s: Setting timer "
                               "MSG_ROUTING_PimDmPruneTimeoutAlarm for "
                               "(%s , %s) with delay %s\n",
                               node->nodeId,
                               clockStr,
                               srcStr,
                               grpStr,
                               delayStr);
            }
            break;
        }

        case MSG_ROUTING_PimDmGraftRtmxtTimeOut:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                sizeof(RoutingPimDmGraftTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimDmGraftTimerInfo));

            if (DEBUG_TIMER)
            {
                RoutingPimDmGraftTimerInfo* timerInfo =
                    (RoutingPimDmGraftTimerInfo*) data;
                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->grpAddr,
                                            grpStr);
                char delayStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(delay, delayStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout,"Node %u @ %s: Setting timer "
                               "MSG_ROUTING_PimDmGraftRtmxtTimeOut for "
                               "(%s , %s) with delay %s\n",
                               node->nodeId,
                               clockStr,
                               srcStr,
                               grpStr,
                               delayStr);
            }

            break;
        }

        case MSG_ROUTING_PimDmAssertTimeOut:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                    sizeof(RoutingPimDmAssertTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimDmAssertTimerInfo));

            if (DEBUG_TIMER)
            {
                RoutingPimDmAssertTimerInfo* timerInfo =
                    (RoutingPimDmAssertTimerInfo*) data;

                char srcStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->srcAddr,
                                            srcStr);
                char grpStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(timerInfo->grpAddr,
                                            grpStr);
                char delayStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(delay, delayStr);
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

                fprintf(stdout,"Node %u @ %s: Setting timer "
                               "MSG_ROUTING_PimDmAssertTimeOut for "
                               "(%s , %s) with delay %s\n",
                               node->nodeId,
                               clockStr,
                               srcStr,
                               grpStr,
                               delayStr);
            }

            break;
        }
        case MSG_ROUTING_PimSmKeepAliveTimeOut:
        {
            MESSAGE_InfoAlloc(
                node,
                newMsg,
                sizeof(RoutingPimSmKeepAliveTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmKeepAliveTimerInfo));

            break;
        }
        case MSG_ROUTING_PimSmNeighborTimeOut:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                sizeof(RoutingPimDmNbrTimerInfo));
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmNbrTimerInfo));

            break;
        }
        case MSG_ROUTING_PimSmBootstrapTimeOut:
        {
            MESSAGE_InfoAlloc(
                node,
                newMsg,
                sizeof(RoutingPimSmBootstrapTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmBootstrapTimerInfo));

            break;
        }
        case MSG_ROUTING_PimSmCandidateRPTimeOut:
        case MSG_ROUTING_PimSmScheduleCandidateRP:
        {
            MESSAGE_InfoAlloc(
                node,
                newMsg,
                sizeof(RoutingPimSmCandidateRPTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmCandidateRPTimerInfo));

            break;
        }
        case MSG_ROUTING_PimSmExpiryTimerTimeout:
        {
            MESSAGE_InfoAlloc(
                node,
                newMsg,
                sizeof(RoutingPimSmJoinPruneTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmJoinPruneTimerInfo));

            break;
        }

        case MSG_ROUTING_PimSmPrunePendingTimerTimeout:
        {
            MESSAGE_InfoAlloc(
                node,
                newMsg,
                sizeof(RoutingPimSmJoinPruneTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmJoinPruneTimerInfo));

            break;
        }
        case MSG_ROUTING_PimSmJoinTimerTimeout:
        {

            MESSAGE_InfoAlloc(node, newMsg,
                sizeof(RoutingPimSmJoinPruneTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmJoinPruneTimerInfo));

            break;
        }
        case MSG_ROUTING_PimSmAssertTimerTimeout:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                 sizeof(RoutingPimSmAssertTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmAssertTimerInfo));
            break;
        }
        case MSG_ROUTING_PimSmRegisterStopTimeOut:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                 sizeof(RoutingPimSmRegisterStopTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmRegisterStopTimerInfo));
            break;
        }

        case MSG_ROUTING_PimSmOverrideTimerTimeout:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                 sizeof(RoutingPimSmJoinPruneTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmJoinPruneTimerInfo));

            break;
        }
        case MSG_ROUTING_PimSmRouterGrpToRPTimeout:
        case MSG_ROUTING_PimSmBSRGrpToRPTimeout:
        {
            MESSAGE_InfoAlloc(node, newMsg,
                 sizeof(RoutingPimSmGrpToRPTimerInfo));

            memcpy(MESSAGE_ReturnInfo(newMsg),
                   data,
                   sizeof(RoutingPimSmGrpToRPTimerInfo));

            break;
        }

        default:
        {
#ifdef EXATA
            if (!node->partitionData->isEmulationMode)
                ERROR_Assert(FALSE, "Unknown event encounter while attempt "
                                "to set timer\n");
#else
                ERROR_Assert(FALSE, "Unknown event encounter while attempt "
                                "to set timer\n");
#endif
            if (newMsg)
            {
                MESSAGE_Free(node, newMsg);
            }
            return;
        }
    }

    /* Send self message */
    MESSAGE_Send(node, newMsg, delay);
}

void RoutingPimGetInterfaceAndNextHopFromForwardingTable(
    Node *node,
    NodeAddress destinationAddress,
    int *interfaceIndex,
    NodeAddress *nextHopAddress)
{
    BOOL found = FALSE;
    //if it is one of my interface address
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (destinationAddress == NetworkIpGetInterfaceAddress(node, i))
        {
            *interfaceIndex = i;
            *nextHopAddress = destinationAddress ;
            found = TRUE;
            break;
        }
    }

    if (found == TRUE)
    {
        return;
    }

    NetworkGetInterfaceAndNextHopFromForwardingTable(node,
                                destinationAddress,
                                interfaceIndex,
                                nextHopAddress);
    if (*nextHopAddress == 0)
    {
        *nextHopAddress = destinationAddress;
        *interfaceIndex = NetworkIpGetInterfaceIndexForNextHop(
                        node,
                        *nextHopAddress);
    }
    else if (*nextHopAddress == (unsigned int)NETWORK_UNREACHABLE)
    {
        for (int i = 0; i < node->numberInterfaces; i++)
        {
            if (destinationAddress ==
            NetworkIpGetInterfaceAddress(node, i))
            {
                *nextHopAddress = destinationAddress;
                *interfaceIndex = i;
            }
        }
    }
}
//Get the Assert packet from the message
void RoutingPimGetAssertPacketFromBuffer(char* packet,
                                         RoutingPimDmAssertPacket* assertPkt)
{
    memcpy(&assertPkt->commonHeader, packet,
        sizeof(RoutingPimCommonHeaderType));
    packet = packet + sizeof(RoutingPimCommonHeaderType);

    memcpy(&assertPkt->groupAddr.addressFamily, packet,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(&assertPkt->groupAddr.encodingType, packet,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(&assertPkt->groupAddr.reserved, packet,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(&assertPkt->groupAddr.maskLength, packet,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(&assertPkt->groupAddr.groupAddr, packet, sizeof(NodeAddress));
    packet = packet + sizeof(NodeAddress);

    memcpy(&assertPkt->sourceAddr.addressFamily, packet,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(&assertPkt->sourceAddr.encodingType, packet,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(&assertPkt->sourceAddr.unicastAddr, packet, sizeof(NodeAddress));
    packet = packet + sizeof(NodeAddress);

    memcpy(&assertPkt->metricBitPref, packet, sizeof(UInt32));
    packet = packet + sizeof(UInt32);

    memcpy(&assertPkt->metric, packet, sizeof(unsigned int));
}

//Set the Buffer from Assert packet
void RoutingPimSetBufferFromAssertPacket(char* packet,
                                         RoutingPimDmAssertPacket* assertPkt)
{
    memcpy(packet, &assertPkt->commonHeader,
        sizeof(RoutingPimCommonHeaderType));
    packet = packet + sizeof(RoutingPimCommonHeaderType);

    memcpy(packet, &assertPkt->groupAddr.addressFamily,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(packet, &assertPkt->groupAddr.encodingType,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(packet, &assertPkt->groupAddr.reserved, sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(packet, &assertPkt->groupAddr.maskLength, sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(packet, &assertPkt->groupAddr.groupAddr, sizeof(NodeAddress));
    packet = packet + sizeof(NodeAddress);

    memcpy(packet, &assertPkt->sourceAddr.addressFamily,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(packet, &assertPkt->sourceAddr.encodingType,
        sizeof(unsigned char));
    packet = packet + sizeof(unsigned char);

    memcpy(packet, &assertPkt->sourceAddr.unicastAddr, sizeof(NodeAddress));
    packet = packet + sizeof(NodeAddress);

    memcpy(packet, &assertPkt->metricBitPref, sizeof(UInt32));
    packet = packet + sizeof(UInt32);

    memcpy(packet, &assertPkt->metric, sizeof(unsigned int));
}

/*
*  FUNCTION    : RoutingPimGetNoForwardCommonHeader()
*  PURPOSE     : get No forward bit from the common header
*  RETURN VALUE: BOOL
*/
BOOL RoutingPimGetNoForwardCommonHeader(
    RoutingPimCommonHeaderType commonHeader)
{
    if (commonHeader.reserved & 0x80)
    {
        return TRUE;
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimGetMaskLengthFromSubnetMask()
*  PURPOSE     : get the mask length from the given subnet mask
*  i.e. if the subnet mask is 0.0.0.255 then return 24
*  RETURN VALUE: unsigned int
*/
unsigned int
RoutingPimGetMaskLengthFromSubnetMask(NodeAddress grpMask)
{
    unsigned int count = 0;
    while (grpMask)
    {
        if ((grpMask & 0x80000000) == 0)
        {
            count++;
        }
        else
        {
            return count;
        }

        grpMask = grpMask << 1;
    }

    return (sizeof(grpMask) * 8);
}

/*
*  FUNCTION    : RoutingPimCheckInverseSubnetMask()
*  PURPOSE     : Check the validity of Inverse Subnet Mask
*  RETURN VALUE: BOOL
*/
BOOL
RoutingPimCheckInverseSubnetMask(NodeAddress grpMask)
{
    unsigned int maskLen = RoutingPimGetMaskLengthFromSubnetMask(grpMask);
    NodeAddress mask;

    if (maskLen == 32)
    {
        mask = 0;
    }
    else
    {
        mask = ((unsigned int)(~0)) >> maskLen;
    }

    if ((mask ^ grpMask) == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*
*  FUNCTION    : RoutingPimPrintEncodedUnicastFormat()
*  PURPOSE     : To print encoded unicast format
*  RETURN VALUE: NULL
*/
void
RoutingPimPrintEncodedUnicastFormat(RoutingPimEncodedUnicastAddr unicastAddr)
{
    printf("\n~~~~~~~~Encoded Unicast Address~~~~~~~~\n");
    printf("\nAddr Family = 0x%x",unicastAddr.addressFamily);
    printf("\nEncoding Type = 0x%x",unicastAddr.encodingType);
    printf("\nUnicast Address = 0x%x",
                        getNodeAddressFromCharArray(unicastAddr.unicastAddr));
}

/*
*  FUNCTION    : RoutingPimPrintEncodedUnicastFormat()
*  PURPOSE     : To print encoded unicast format
*  RETURN VALUE: NULL
*/
void
RoutingPimPrintEncodedGroupFormat(RoutingPimEncodedGroupAddr grpAddr)
{
    printf("\n~~~~~~~~Encoded Group Address~~~~~~~~\n");
    printf("\nAddr Family = 0x%x",grpAddr.addressFamily);
    printf("\nEncoding Type = 0x%x",grpAddr.encodingType);
    printf("\nReserved = 0x%x",grpAddr.reserved);
    printf("\nMask Length = 0x%x",grpAddr.maskLength);

    printf("\nGroup Address = 0x%x",grpAddr.groupAddr);
}
/*
*  FUNCTION     :RoutingPimIsRPPresent()
*  PURPOSE      :Check at runtime if RP is present for the group
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
BOOL RoutingPimIsRPPresent(Node* node,NodeAddress groupAddr)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    NodeAddress RPAddr = (NodeAddress) NETWORK_UNREACHABLE;
    PimDataSm* pimDataSm = NULL;

    char rpAddress[MAX_STRING_LENGTH];
    NodeAddress nextHop = (NodeAddress) NETWORK_UNREACHABLE;
    int outInterface = NETWORK_UNREACHABLE;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RPAddr = RoutingPimSmFindRPForThisGroup(node,
                                        groupAddr);
    if (RPAddr == (unsigned)NETWORK_UNREACHABLE)
    {
        return FALSE;
    }
    IO_ConvertIpAddressToString(RPAddr,
                            rpAddress);
#ifdef DEBUG
    {
        printf("RP Address is %s\n",
               rpAddress);
    }
#endif
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, RPAddr,
            &outInterface, &nextHop);
    if (nextHop == 0)
    {
        nextHop = RPAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        for (int i = 0; i < node->numberInterfaces; i++)
        {
            if (RPAddr ==
                NetworkIpGetInterfaceAddress(node, i))
            {
                nextHop = RPAddr ;
                outInterface = i;
            }
        }
    }
    return TRUE;
}

/*
*  FUNCTION    : RoutingPimRouterFunction()
*  PURPOSE     : Handle the forwarding process for PIM
*  ASSUMPTION  : None
*  RETURN VALUE: NULL
 */

void RoutingPimRouterFunction(Node* node,
                            Message* msg,
                            NodeAddress groupAddr,
                            int incomingInterface,
                            BOOL* packetWasRouted,
                            NodeAddress prevHop)
{
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    int sourceInterface;
    NodeAddress srcAddr;
    BOOL is_sptbit_set = FALSE;
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL ;

    srcAddr = ipHeader->ip_src;
    sourceInterface = NetworkGetInterfaceIndexForDestAddress(node, srcAddr);
    BOOL isRPPresent = RoutingPimIsRPPresent(node,groupAddr);
    BOOL joinDesired = FALSE;

    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                             (node, groupAddr, srcAddr, ROUTING_PIMSM_SG);
    if (treeInfoBaseRowPtr)
    {
        is_sptbit_set = treeInfoBaseRowPtr->SPTbit;
        joinDesired =
            RoutingPimSmJoinDesiredSG(node, groupAddr, srcAddr);
    }
    //if ((isRPPresent == TRUE) || ((joinDesired && is_sptbit_set == TRUE)))
    //if (isRPPresent == TRUE || joinDesired || is_sptbit_set == TRUE)
    //if (isRPPresent == TRUE)
    if (isRPPresent == TRUE || joinDesired)
    {
        RoutingPimSmForwardingFunction(node,
                                        msg,
                                        groupAddr,
                                        incomingInterface,
                                        packetWasRouted,
                                        prevHop);
    }
    else
    {
        RoutingPimDmRouterFunction(node,
                                  msg,
                                  groupAddr,
                                  incomingInterface,
                                  packetWasRouted,
                                  prevHop);
    }
}

/*
*  FUNCTION    : RoutingPimUnicastRouteChange()
*  PURPOSE     : Handle the Unicast routing change for PIM
*  ASSUMPTION  : None
*  RETURN VALUE: NULL
 */

void RoutingPimUnicastRouteChange(Node* node,
                                  NodeAddress destAddr,
                                  NodeAddress destAddrMask,
                                  NodeAddress nextHop,
                                  int interfaceId,
                                  int metric,
                              NetworkRoutingAdminDistanceType adminDistance)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    BOOL processing_done = FALSE;
    BOOL is_sptbit_set = FALSE;
    BOOL joinDesired = FALSE;
    pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;

    RoutingPimSmTreeInformationBase* treeInfo = &pimDataSm->treeInfoBase;

    TreeInfoRowMap* rowPtrMap = treeInfo->rowPtrMap;

    TreeInfoRowMapIterator mapIterator;
    RoutingPimSmTreeInfoBaseRow * rowPtr = NULL;

    RoutingPimSmJoinPruneType joinPruneType =
        ROUTING_PIMSM_NOINFO_JOIN_PRUNE;

    metric = metric;
    adminDistance = adminDistance;

    NodeAddress RPAddr;
    NodeAddress groupAddr;
    BOOL invoke_Sparse = FALSE;
    BOOL isRPPresent;
    if (treeInfo->numEntries == 0)
    {
#ifdef DEBUG
        {
            printf("Node %u:no entry in tree info base \n",
                   node->nodeId);
        }
#endif
        /* Perform only Dense processing */
        RoutingPimDmAdaptUnicastRouteChange(node,
                                            destAddr,
                                            destAddrMask,
                                            nextHop,
                                            interfaceId,
                                            metric,
                                            adminDistance);
        return;
    }


    /* check for sending join prune packet */
    for (mapIterator = rowPtrMap->begin();
         mapIterator != rowPtrMap->end();
         mapIterator++)
    {
        rowPtr = mapIterator->second;
        invoke_Sparse = FALSE;

        groupAddr = rowPtr->grpAddr;
        /* check the RP For this group & find associated nextHop*/
        RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);

        /* SM case */
        if (((rowPtr->srcAddr & destAddrMask) == destAddr)
              && (rowPtr->upstream != nextHop &&
                  rowPtr->srcAddr != rowPtr->upstream) 
              &&
                (rowPtr->treeState == ROUTING_PIMSM_G ||
                 rowPtr->treeState == ROUTING_PIMSM_SG))
        {
            if (rowPtr->treeState == ROUTING_PIMSM_G)
            {
                invoke_Sparse = TRUE;
            }
            else if (rowPtr->treeState == ROUTING_PIMSM_SG)
            {
                invoke_Sparse = FALSE;
                is_sptbit_set = FALSE;
                joinDesired = FALSE;
                isRPPresent = RoutingPimIsRPPresent(node,
                                          rowPtr->grpAddr);

                if (rowPtr)
                {
                    is_sptbit_set = rowPtr->SPTbit;
                    joinDesired =
                        RoutingPimSmJoinDesiredSG(node,
                                           rowPtr->grpAddr,
                                           rowPtr->srcAddr);
                }
                if (isRPPresent == TRUE || joinDesired
                    || is_sptbit_set == TRUE)
                //if (isRPPresent == TRUE || joinDesired)
                {
                    invoke_Sparse = TRUE;
                }
            }

            if (invoke_Sparse)
            {
                if (nextHop == (unsigned)NETWORK_UNREACHABLE
#ifdef ADDON_BOEINGFCS
                    || (MulticastCesRpimActiveOnInterface(node, interfaceId)
                            && nextHop == 0)
#endif
                   )
                {
#ifdef DEBUG
                    {
                        printf("Node %u: no nextHop to reach destination \n",
                               node->nodeId);
                    }
#endif
                    return;
                }
#ifdef DEBUG
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);

                    printf("at node %d doing modification at %s \n",
                        node->nodeId,clockStr);
                    printf(" new route change \n");
                    printf("asociated srcAddr = 0x%x \n",
                           rowPtr->srcAddr);
                    printf("old upstream %u\n",rowPtr->upstream);
                    printf("old nextIntf %u\n", rowPtr->nextIntfForSrc);

                    printf(" new interfaceId %d \n", interfaceId);
                }
#endif
                if (rowPtr->upstreamState == PimSm_JoinPrune_Join)
                {
                    RoutingPimSmJoinPruneTimerInfo    joinTimer;
                    clocktype delay;

#ifdef DEBUG_ERRORS
                    {
                        printf("associated upstream state is join \n");
                    }
#endif
                    /* set the join prune Type */
                    if (rowPtr->treeState == ROUTING_PIMSM_G)
                    {
                        joinPruneType = ROUTING_PIMSM_G_JOIN_PRUNE;
                    }
                    else if (rowPtr->treeState == ROUTING_PIMSM_SG)
                    {
                        joinPruneType = ROUTING_PIMSM_SG_JOIN_PRUNE;
                    }
#ifdef DEBUG
                    {
                        printf("Node %u :\n", node->nodeId);
                        printf("send prune to old next Hop thru intf %d\n",
                               rowPtr->nextIntfForRP);
                        printf(" send Join to new next Hop thru intf %d\n",
                               interfaceId);
                    }
#endif
                    if (joinPruneType == ROUTING_PIMSM_SG_JOIN_PRUNE)
                    {
                        RoutingPimSmSendJoinPrunePacket(node,
                                          rowPtr->srcAddr,
                                          rowPtr->upstream,
                                          rowPtr->grpAddr,
                                          rowPtr->nextIntfForSrc,
                                          joinPruneType,
                                          ROUTING_PIM_PRUNE_TREE,
                                          rowPtr);

                        if (nextHop == 0)
                        {
                            nextHop = rowPtr->srcAddr ;
                        }
                    }
                    else
                    {
                        RoutingPimSmSendJoinPrunePacket(node,
                                          rowPtr->srcAddr,
                                          rowPtr->upstream,
                                          rowPtr->grpAddr,
                                          rowPtr->nextIntfForRP,
                                          joinPruneType,
                                          ROUTING_PIM_PRUNE_TREE,
                                          rowPtr);
                        if (nextHop == 0)
                        {
                            nextHop = RPAddr;
                        }
                    }

                    RoutingPimSmTreeInfoBaseRow* sgRptRowPtr = NULL;
                    sgRptRowPtr =
                        RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                                        rowPtr->grpAddr,
                                        rowPtr->srcAddr,
                                        ROUTING_PIMSM_SGrpt);

                    if (joinPruneType == ROUTING_PIMSM_SG_JOIN_PRUNE
                        &&
                        sgRptRowPtr != NULL
                        &&
                        sgRptRowPtr->upstreamState == PimSm_SGrpt_Pruned
                        &&
                        rowPtr->nextHopForRP == nextHop)
                    {
                        RoutingPimSmHandleUpstreamStateMachine(node,
                                           rowPtr->srcAddr,
                                           rowPtr->nextIntfForRP,
                                           rowPtr->grpAddr,
                                           sgRptRowPtr);
                    }
                    else
                    {
                        /* Routing PimSm Send (*, G)Join Packet */
                        RoutingPimSmSendJoinPrunePacket(node,
                                                rowPtr->srcAddr,
                                                nextHop,
                                                rowPtr->grpAddr,
                                                interfaceId,
                                                joinPruneType,
                                                ROUTING_PIM_JOIN_TREE,
                                                rowPtr);

                        /* Routing PimSm Set Join Timer to t_periodic*/
                        joinTimer.srcAddr = rowPtr->srcAddr;
                        joinTimer.grpAddr = rowPtr->grpAddr;
                        joinTimer.intfIndex = interfaceId;
                        joinTimer.seqNo = rowPtr->joinTimerSeq + 1;
                        joinTimer.treeState = rowPtr->treeState;

                        /* Note the latest joinTimerSeq in treeInfo Base */
                        rowPtr->joinTimerSeq++;
                        delay = (clocktype)
                            (pimDataSm->routingPimSmTPeriodicInterval);
                        rowPtr->lastJoinTimerEnd = node->getNodeTime() + delay;

                        RoutingPimSetTimer(node,
                                           interfaceId,
                                           MSG_ROUTING_PimSmJoinTimerTimeout,
                                           (void*) &joinTimer, delay);

                        rowPtr->nextHopForSrc = nextHop;
                        rowPtr->nextIntfForSrc = interfaceId;
                        rowPtr->upstream = nextHop;
                        rowPtr->upInterface = interfaceId;

                        if (joinPruneType == ROUTING_PIMSM_G_JOIN_PRUNE)
                        {
                            rowPtr->nextHopForRP = nextHop;
                            rowPtr->nextIntfForRP = interfaceId;
                        }
                    }
                }
                else if (rowPtr->upstreamState == PimSm_JoinPrune_NotJoin
                         &&
                         rowPtr->treeState == ROUTING_PIMSM_G)
                {
                    RoutingPimSmHandleUpstreamStateMachine(node,
                                           rowPtr->srcAddr,
                                           rowPtr->nextIntfForRP,
                                           rowPtr->grpAddr,
                                           rowPtr);
                }
                //*assert state machine
                /*RPF_interface(S) stops being interface I
                Interface I used to be the RPF interface for S, and now it is
                not. We transition to NoInfo state, deleting this (S,G) assert
                state (Actions A5 below).*/
                RoutingPimSmDownstream* downStreamListPtr = NULL;
                downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                                            rowPtr,
                                            interfaceId);

                if (downStreamListPtr != NULL)
                {
                    if ((rowPtr->srcAddr & destAddrMask) == destAddr
                        &&
                        downStreamListPtr->assertState ==
                        PimSm_Assert_ILostAssert)
                    {
                        if (rowPtr->nextIntfForSrc != interfaceId)
                        {
                            downStreamListPtr->assertState =
                                                        PimSm_Assert_NoInfo;

                            RoutingPimSmPerformActionA5(node,
                                          rowPtr->grpAddr,
                                          rowPtr->srcAddr,
                                          rowPtr->nextIntfForSrc,
                                          rowPtr->treeState);
                        }
                    }
                }
            }
        }//end of if
        if (!invoke_Sparse)
        {
            PimDmData* pimDmData = NULL;

            RoutingPimDmForwardingTableRow* rowPtr = NULL;
                    pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;

            rowPtr = (RoutingPimDmForwardingTableRow*)
                             BUFFER_GetData(&pimDmData->fwdTable.buffer);
                    /*  Check whether upstream have been changed */

            for (unsigned int i = 0 ;
                i < pimDmData->fwdTable.numEntries ;
                i++)
            {
#ifdef ADDON_BOEINGFCS
                if (((rowPtr[i].srcAddr & destAddrMask) == destAddr)
                    && (rowPtr[i].upstream != nextHop) && nextHop != 0)
#else
                if (((rowPtr[i].srcAddr & destAddrMask) == destAddr)
                    && (rowPtr[i].upstream != nextHop))
#endif
                {
                    processing_done = TRUE;
#ifdef DEBUG
                    {
                        fprintf(stdout, "Node %u: PIM got route update alert"
                        " from underlying unicast routing protocol\n",
                        node->nodeId);
                    }
#endif

                    /*  If this route is now unrechable, delete this entry */
                    if (nextHop == (unsigned) NETWORK_UNREACHABLE)
                    {
#ifdef DEBUG
                        {
                            fprintf(stdout,"Node %u delete (%u, %u) entry from "
                                     "forwarding cache as there is no route "
                                        "back to source\n",
                                        node->nodeId, rowPtr[i].srcAddr,
                                        rowPtr[i].grpAddr);
                        }
#endif
                        ListFree(node, rowPtr[i].downstream, FALSE);

                        BUFFER_RemoveDataFromDataBuffer(
                                &pimDmData->fwdTable.buffer,
                                (char*) &rowPtr[i],
                                sizeof(RoutingPimDmForwardingTableRow));
                        pimDmData->fwdTable.numEntries -= 1;
                        i--;
                        continue;
                    }

#ifdef DEBUG
                    {
                        fprintf(stdout, "Node %u change upstream from %u to %u "
                                        "for (%u, %u) cache entry\n",
                                       node->nodeId, rowPtr[i].upstream, nextHop,
                                        rowPtr[i].srcAddr, rowPtr[i].grpAddr);
                    }
#endif

                    /*  Make new RPF neighbor a upstream */
                    rowPtr[i].upstream = nextHop;

                    /*
                     *   If this interface was previous downstream,
                     *   delete from downstream list
                     */
                    if (RoutingPimDmIsDownstreamInterface(&rowPtr[i],
                                interfaceId))
                    {
                        int prevIntfIndex;

                        prevIntfIndex =
                            NetworkIpGetInterfaceIndexFromAddress(
                            node,
                                                               rowPtr[i].inIntf);
                        RoutingPimDmRemoveDownstream(node,
                                                     &rowPtr[i],
                                                     interfaceId);

                        /*  Add previous upstream as new downstream */
                        RoutingPimDmAddDownstream(node,
                                                  &rowPtr[i],
                                                  prevIntfIndex);

                        //Change the assert status
                    }

                    rowPtr[i].inIntf = NetworkIpGetInterfaceAddress(node,
                                               interfaceId);
                    rowPtr[i].assertState = Pim_Assert_NoInfo;
                    rowPtr[i].assertTime = node->getNodeTime();
                    rowPtr[i].metric = metric;
                    rowPtr[i].preference = adminDistance;

                    if ((!RoutingPimDmAllDownstreamPruned(rowPtr) ||
                          NetworkIpIsPartOfMulticastGroup(node, rowPtr->grpAddr))
                        && rowPtr[i].state == ROUTING_PIM_DM_PRUNE)
                    {
                        rowPtr[i].state = ROUTING_PIM_DM_ACKPENDING;

                        /*  Send Graft to new upstream */
                        RoutingPimDmSendGraftPacket(node, rowPtr[i].srcAddr,
                                rowPtr[i].grpAddr);

#ifdef DEBUG
                        {
                            RoutingPimDmPrintForwardingTable(node);
                        }
#endif
                    }
                }
                if (processing_done)
                    break;
            }
        }// end of if !invoke_Sparse
    }//end of for
}

/*
*  FUNCTION     :RoutingPimSmInit()
*  PURPOSE      :Handle Pim SM Init
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimSmInit(Node* node, int interfaceIndex)
{
    PimData* pim = (PimData*)
                   NetworkIpGetMulticastRoutingProtocol(node,
                                                     MULTICAST_PROTOCOL_PIM);
    NodeAddress interfaceAddress;

    interfaceAddress = NetworkIpGetInterfaceAddress(node,interfaceIndex);
    BOOL retVal = FALSE;
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }
#ifdef DEBUG
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("In function RoutingPimSmInit at time %s\n",clockStr);
    }
#endif
    /* Initialize the groupSpecificRPList & other parameters; */
    ListInit(node, &pimDataSm->RPList);

    pimDataSm->lastBootstrapSend = 0;
    pimDataSm->lastCandidateRPSend = 0;

    pimDataSm->lastBSMRcvdFromCurrentBSR = NULL;
    pimDataSm->routingPimSmRouterGrpToRPTimeout = 0;
    pimDataSm->routingPimSmBSRGrpToRPTimeout = 0;

    clocktype triggeredDelay;
    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        node->partitionData->nodeInput,
        "ROUTING-PIMSM-TRIGGERED-DELAY",
        &retVal,
        &triggeredDelay);

    if (retVal) {
        pimDataSm->routingPimSmTriggeredDelay =
            triggeredDelay;
    }
    else {
        pimDataSm->routingPimSmTriggeredDelay =
            ROUTING_PIMSM_TRIGGERED_DELAY_DEFAULT;
    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->routingPimSmTriggeredDelay,
            paramValue);

        printf("Value for ROUTING-PIMSM-TRIGGERED-DELAY is %s\n",
            paramValue);
    }

    clocktype bootstrapTimeout;
    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        node->partitionData->nodeInput,
        "ROUTING-PIMSM-BOOTSTRAP-PERIOD",
        &retVal,
        &bootstrapTimeout);

    if (retVal) {
        pimDataSm->routingPimSmBootstrapTimeout =
            bootstrapTimeout;
    }
    else {
        pimDataSm->routingPimSmBootstrapTimeout =
                     ROUTING_PIMSM_BOOTSTRAP_PERIOD_DEFAULT;

    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->routingPimSmBootstrapTimeout,
            paramValue);

        printf("Value for ROUTING-PIMSM-BOOTSTRAP-TIMEOUT is %s\n",
            paramValue);
    }

    clocktype candidateRpTimeout;
    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        node->partitionData->nodeInput,
        "ROUTING-PIMSM-CANDIDATE-RP-TIMEOUT",
        &retVal,
        &candidateRpTimeout);

    if (retVal) {
        pimDataSm->routingPimSmCandidateRpTimeout =
            candidateRpTimeout;
    }
    else {
        pimDataSm->routingPimSmCandidateRpTimeout =
            ROUTING_PIMSM_CANDIDATE_RP_TIMEOUT_DEFAULT;
    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->routingPimSmCandidateRpTimeout,
            paramValue);

        printf("Value for ROUTING-PIMSM-CANDIDATE-RP-TIMEOUT is %s\n",
            paramValue);
    }

    clocktype keepaliveTimeout;
    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        node->partitionData->nodeInput,
        "ROUTING-PIMSM-KEEPALIVE-TIMEOUT",
        &retVal,
        &keepaliveTimeout);

    if (retVal) {
        pimDataSm->routingPimSmKeepaliveTimeout =
            keepaliveTimeout;
    }
    else {
        pimDataSm->routingPimSmKeepaliveTimeout =
            ROUTING_PIMSM_KEEPALIVE_TIMEOUT_DEFAULT;
    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->routingPimSmKeepaliveTimeout,
            paramValue);

        printf("Value for ROUTING-PIMSM-KEEPALIVE-TIMEOUT is %s\n",
            paramValue);
    }

    clocktype joinpruneHoldTimeout;
    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        node->partitionData->nodeInput,
        "ROUTING-PIMSM-JOINPRUNE-HOLD-TIMEOUT",
        &retVal,
        &joinpruneHoldTimeout);

    if (retVal) {
        pimDataSm->routingPimSmJoinPruneHoldTimeout =
            joinpruneHoldTimeout;
    }
    else {
        pimDataSm->routingPimSmJoinPruneHoldTimeout =
            ROUTING_PIMSM_JOINPRUNE_HOLD_TIMEOUT_DEFAULT;
    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->routingPimSmJoinPruneHoldTimeout,
            paramValue);

        printf("Value for ROUTING-PIMSM-JOINPRUNE-HOLD-TIMEOUT is %s\n",
            paramValue);
    }

    clocktype tPeriodicInterval;
    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        node->partitionData->nodeInput,
        "ROUTING-PIMSM-T-PERIODIC-INTERVAL",
        &retVal,
        &tPeriodicInterval);

    if (retVal) {
        pimDataSm->routingPimSmTPeriodicInterval=
            tPeriodicInterval;
    }
    else {
        pimDataSm->routingPimSmTPeriodicInterval=
            ROUTING_PIMSM_T_PERIODIC_INTERVAL_DEFAULT;
    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->routingPimSmTPeriodicInterval,
            paramValue);

        printf("Value for ROUTING-PIMSM-T-PERIODIC-INTERVAL is %s\n",
            paramValue);
    }

    clocktype assertTimeout;
    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        node->partitionData->nodeInput,
        "ROUTING-PIMSM-ASSERT-TIMEOUT",
        &retVal,
        &assertTimeout);

    if (retVal) {
        pimDataSm->routingPimSmAssertTimeout=
            assertTimeout;
    }
    else {
        pimDataSm->routingPimSmAssertTimeout=
            ROUTING_PIMSM_ASSERT_TIMEOUT_DEFAULT;
    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->routingPimSmAssertTimeout,
            paramValue);

        printf("Value for ROUTING-PIMSM-ASSERT-TIMEOUT is %s\n",
            paramValue);
    }

    clocktype registerSupressionTime;
    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        node->partitionData->nodeInput,
        "ROUTING-PIMSM-REGISTER-SUPRESSION-TIME",
        &retVal,
        &registerSupressionTime);

    if (retVal) {
        pimDataSm->routingPimSmRegisterSupressionTime =
            registerSupressionTime;
    }
    else {
        pimDataSm->routingPimSmRegisterSupressionTime=
            ROUTING_PIMSM_REGISTER_SUPRESSION_TIME_DEFAULT;
    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->routingPimSmRegisterSupressionTime,
            paramValue);

        printf("ROUTING-PIMSM-REGISTER-SUPRESSION-TIME is %s\n",
            paramValue);
    }

    clocktype registerProbeTime;
    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        node->partitionData->nodeInput,
        "ROUTING-PIMSM-REGISTER-PROBE-TIME",
        &retVal,
        &registerProbeTime);

    if (retVal) {
        pimDataSm->routingPimSmRegisterProbeTime =
            registerProbeTime;
    }
    else {
        pimDataSm->routingPimSmRegisterProbeTime=
            ROUTING_PIMSM_REGISTER_PROBE_TIME_DEFAULT;
    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->routingPimSmRegisterProbeTime,
            paramValue);

        printf("ROUTING-PIMSM-REGISTER-PROBE-TIME is %s\n",
            paramValue);
    }

    if (pimDataSm->routingPimSmRegisterProbeTime >=
        ROUTING_PIMSM_HALF_REGISTER_SUPPRESSION_TIME(
                pimDataSm->routingPimSmRegisterSupressionTime))
    {
        ERROR_ReportError("\nRegister_Probe_Time should be less than half "
                          "the value of the Register_Suppression_Time to "
                          "prevent a possible negative value in the "
                          "setting of the Register-Stop Timer\n");
    }
    retVal = FALSE;
    char buf[MAX_STRING_LENGTH];
    IO_ReadString(node->nodeId,
                  interfaceAddress,
                  node->partitionData->nodeInput,
                  "ROUTING-PIMSM-SWITCH-SPT-THRESHOLD",
                  &retVal,
                  buf);

    if (retVal) {
        RoutingPimSmGetSptThresholdInfo(node,
                                        buf,
                                        &pimDataSm->sptSwitchThresholdInfo);
    }
    else {
        pimDataSm->sptSwitchThresholdInfo.threshold =
            (unsigned int) ROUTING_PIMSM_SWITCH_SPT_THRESHOLD_INFINITE;
        pimDataSm->sptSwitchThresholdInfo.grpAddrList = NULL;
        pimDataSm->sptSwitchThresholdInfo.numDataPacketsReceivedPerGroup = 
                                                        NULL;
    }

    if (DEBUG_CONFIG_PARAMS)
    {
        char paramValue[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (
            pimDataSm->sptSwitchThresholdInfo.threshold,
            paramValue);

        printf("PIMSM-SWITCH-SPT-THRESHOLD is %s\n",
            paramValue);
    }
}

LinkedList*
RoutingPimParseSourceGroupStr(Node* node,
                              char* srcGroupStr,
                              LinkedList* srcGroupList,
                              Int32 interfaceIndex)
{
    if (srcGroupList == NULL)
    {
        ListInit(node, &srcGroupList);
    }

    char* tmpBuf = NULL;
    char* tmpStr1 = NULL;
    char* tmpStr2 = NULL;

    char srcStr[MAX_STRING_LENGTH];
    char grpStr[MAX_STRING_LENGTH];

    int isNodeId = 0;
    int numHostBits = 0;
    int qualifierLen = 0;

    if ((srcGroupStr[0] == '\0') || (srcGroupStr[0] != '<'))
    {
       ERROR_ReportError("\nRP GROUP RANGE is a Bad String!!!\n");
    }
    else
    {
        RoutingPimSourceGroupList* srcGroupListPtr = NULL;

        tmpBuf = srcGroupStr;
        while (tmpBuf)
        {
            srcGroupListPtr = (RoutingPimSourceGroupList*)MEM_malloc(
                            sizeof(RoutingPimSourceGroupList));

            tmpBuf++;//skipping '<'

            // skip white space
            while (isspace(*tmpBuf))
            {
                tmpBuf++;
            }

            tmpStr1 = strchr(tmpBuf, ',');
            if (tmpStr1 == NULL)
            {
                ERROR_ReportError("\n\nPIM-SM : The specified Group Range "
                                  "is a Bad string.\n");
            }

            qualifierLen = tmpStr1 - tmpBuf;
            strncpy(srcStr, &tmpBuf[0],qualifierLen);
            srcStr[qualifierLen] = '\0';

            IO_TrimLeft(srcStr);
            IO_TrimRight(srcStr);

            IO_ParseNodeIdHostOrNetworkAddress(srcStr,
                                               &srcGroupListPtr->srcAddr,
                                               &numHostBits,
                                               &isNodeId);

            tmpStr1++;
            // skip white space
            while (isspace(*tmpStr1))
            {
                tmpStr1++;
            }

            tmpStr2 = strchr(tmpBuf, '>');
            if (tmpStr2 == NULL)
            {
                ERROR_ReportError("\n\nPIM-SM : The specified Group Range "
                                  "is a Bad string.\n");
            }

            qualifierLen = tmpStr2 - tmpStr1;
            strncpy(grpStr, &tmpStr1[0],qualifierLen);
            grpStr[qualifierLen] = '\0';

            IO_TrimLeft(srcStr);
            IO_TrimRight(grpStr);

            IO_ParseNodeIdHostOrNetworkAddress(grpStr,
                                               &srcGroupListPtr->groupAddr,
                                               &numHostBits,
                                               &isNodeId);

            ListInsert(node,
                       srcGroupList,
                      (clocktype)0,
                      (void*)srcGroupListPtr);

            tmpBuf = strchr(tmpBuf, '<');
        }//end of while
    }//end of else

    return srcGroupList;
}


void RoutingPimGetNeighborFromIPTable(Node* node,
                                      int interfaceId)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDmData* pimDmData = NULL;

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDmData = ((PimDataSmDm*)pim->pimModePtr)->pimDm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
    {
        pimDmData = (PimDmData*)pim->pimModePtr;
    }

    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    NetworkForwardingTable *rt = &ip->forwardTable;
    RoutingPimNeighborListItem* neighbor = NULL;

    RoutingPimInterface* thisInterface = &pim->interface[interfaceId];

    int i = 0;
#ifdef DEBUG
    {
        NetworkPrintForwardingTable(node);
    }
#endif

    for (i = 0; i < rt->size; i++)
    {
        if (rt->row[i].protocolType == ROUTING_PROTOCOL_OSPFv2
            &&
            rt->row[i].destAddressMask == ANY_IP
            &&
            (rt->row[i].nextHopAddress == NEXT_HOP_ME
             ||
             rt->row[i].nextHopAddress == rt->row[i].destAddress))
        {
            RoutingPimSearchNeighborList(
                     thisInterface->dmInterface->neighborList,
                     rt->row[i].destAddress,
                     &neighbor);

            /*  If we already have an entry for this neighbor */
            if (neighbor)
            {
                /*  Else, refresh neighbor timeout timer */
                neighbor->lastHelloReceive = node->getNodeTime();
            }
            else
            {
                RoutingPimDmForwardingTableRow* rowPtr;

                /*  Neighbor doesn't have an entry, so build one */

                if (DEBUG_WIRELESS)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char srcStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(neighbor->ipAddr,
                                                srcStr);
                    char addrStr[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(
                        NetworkIpGetInterfaceAddress(node, interfaceId),
                        addrStr);
                    fprintf(stdout, "Node %u @ %s: Found new neighbor %s on "
                                    "interface %d(%s)\n",
                                    node->nodeId,
                                    clockStr,
                                    srcStr,
                                    interfaceId,
                                    addrStr);

                    RoutingPimDmPrintNeibhborList(node);
                }

                /*  Allocate space for new neighbor */
                neighbor = (RoutingPimNeighborListItem*)
                           MEM_malloc(sizeof(RoutingPimNeighborListItem));

                neighbor->ipAddr = rt->row[i].destAddress;
                //neighbor->lastGenIdReceive = genId;
                neighbor->lastHelloReceive = node->getNodeTime();
                //neighbor->holdTime = holdTime;

                /*  Add to neighbor list */
                ListInsert(node,
                           thisInterface->dmInterface->neighborList,
                           node->getNodeTime(),
                           (void*) neighbor);

                /*  Since we've found a neighbor, this network is not Leaf */
                thisInterface->dmInterface->isLeaf = FALSE;
                thisInterface->dmInterface->neighborCount++;
                pimDmData->stats.numNeighbor++;

                /*  Select DR if this is a broadcast interface */
                if (thisInterface->dmInterface->neighborCount > 1)
                {
                    /*  Elect new DR */
                    //RoutingPimDmElectDR(node, interfaceIndex);

                    #ifdef DEBUG
                    {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);
                        fprintf(stdout, "Node %ld says new DR on interface %d is %u "
                                "at time %s\n",
                                node->nodeId,
                                interfaceId,
                                thisInterface->dmInterface->designatedRouter,
                                clockStr);
                    }
                    #endif

                }

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
               if ((pim->interface[interfaceId].interfaceType!=
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
                        && (!RoutingPimDmIsDownstreamInterface(
                                &rowPtr[i],
                                interfaceId)))
                    {
                        RoutingPimDmAddDownstream(node, &rowPtr[i], interfaceId);

                        if (rowPtr[i].state == ROUTING_PIM_DM_PRUNE)
                        {
                            RoutingPimDmDataTimeoutInfo cacheTimer;

                            /*  Change to forward state */
                            rowPtr[i].state = ROUTING_PIM_DM_FORWARD;

                            /*  Send graft to upstream */
                            RoutingPimDmSendGraftPacket(node, rowPtr[i].srcAddr,
                                    rowPtr[i].grpAddr);

                            /*  Set DataTimeout timer */
                            rowPtr[i].expirationTimer = node->getNodeTime();
                            cacheTimer.srcAddr = rowPtr[i].srcAddr;
                            cacheTimer.grpAddr = rowPtr[i].grpAddr;
                            RoutingPimSetTimer(node,
                                interfaceId,
                                MSG_ROUTING_PimDmDataTimeOut,
                                    (void*) &cacheTimer,
                                    (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
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
                                rowPtr[i].state = ROUTING_PIM_DM_FORWARD;

                                /*  Send graft to upstream */
                                RoutingPimDmSendGraftPacket(node, rowPtr[i].srcAddr,
                                        rowPtr[i].grpAddr);

                                /*  Set DataTimeout timer */
                                rowPtr[i].expirationTimer = node->getNodeTime();
                                cacheTimer.srcAddr = rowPtr[i].srcAddr;
                                cacheTimer.grpAddr = rowPtr[i].grpAddr;
                                RoutingPimSetTimer(
                                    node,
                                    interfaceId,
                                    MSG_ROUTING_PimDmDataTimeOut,
                                    (void*) &cacheTimer,
                                    (clocktype) ROUTING_PIM_DM_DATA_TIMEOUT);
                            }//end of if
                        }//end of if
                    }//end of for
                }//end of for
            }//end of else
        }//end of if
    }//end of for
}

#ifdef ADDON_DB
/**
* To get the first node on each partition that is actually running PIM.
*
* @param partition Data structure containing partition information
* @return First node on the partition that is running PIM
*/
Node* StatsDBPimGetInitialNodePointer(PartitionData* partition)
{
    Node* traverseNode = partition->firstNode;
    PimData* pim = NULL;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        pim = (PimData*) NetworkIpGetMulticastRoutingProtocol(traverseNode,
                                                     MULTICAST_PROTOCOL_PIM);
        if (pim == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        // we've found a node that has PIM on it!
        return traverseNode;
    }

    return NULL;
}


void HandleStatsDBPimSmStatusTableInsertion(Node* node)
{
    StatsDb* db = node->partitionData->statsDb;
    PartitionData* partition = node->partitionData;

    // Check if the Table exists.
    if (!db ||!db->statsPimTable ||
        !db->statsPimTable->createPimSmStatusTable)
    {
        // Table does not exist
        return;
    }

    Node* traverseNode = node;
    int i;
    unsigned int j;
    PimData* pim = NULL;
    RoutingPimInterface* thisPimIntf = NULL;
    BOOL alreadyExist = FALSE;
    ListItem* tmpListItem = NULL;
    RoutingPimDmInterface* dmInterface = NULL;
    RoutingPimSmInterface* smInterface = NULL;
    NodeAddress DR = 0;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        pim = (PimData*) NetworkIpGetMulticastRoutingProtocol(traverseNode,
                                                    MULTICAST_PROTOCOL_PIM);

        if (pim == NULL || pim->modeType == ROUTING_PIM_MODE_DENSE)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        PimDataSm* pimSmData = (PimDataSm*) pim->pimModePtr;

        if (pimSmData == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        StatsDBPimSmStatus stats;

        //for BSR
        for (i = 0; i < traverseNode->numberInterfaces; i++)
        {
            thisPimIntf = &pim->interface[i];

            if (thisPimIntf != NULL)
            {
                if (thisPimIntf->BSRFlg)
                {
                    if (thisPimIntf->cBSRState == ELECTED_BSR)
                    {
                        stats.m_BsrType = "BSR";
                        break;
                    }
                    else
                    {
                        stats.m_BsrType = "Candidate";
                    }
                }
            }//if (thisPimIntf != NULL)
        }//for (i = 0; i < traverseNode->numberInterfaces; i++)

        //for DR
        for (i = 0; i < traverseNode->numberInterfaces; i++)
        {
            thisPimIntf = &pim->interface[i];
            if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
            {
                dmInterface = pim->interface[i].dmInterface;
                smInterface = pim->interface[i].smInterface;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
            {
                dmInterface = pim->interface[i].dmInterface;
            }
            else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
            {
                smInterface = pim->interface[i].smInterface;
            }
            if (dmInterface)
            {
                DR = dmInterface->designatedRouter;
            }
            else if (smInterface)
            {
                DR = smInterface->drInfo.ipAddr;
            }
            if (thisPimIntf != NULL)
            {
                if (thisPimIntf->ipAddress == DR)
                {
                    stats.m_IsDR = TRUE;

                    char netAddrString[MAX_STRING_LENGTH];

                    NodeAddress networkAddr =
                        NetworkIpGetInterfaceNetworkAddress(traverseNode, i);

                    IO_ConvertIpAddressToString(networkAddr, netAddrString);
                    stats.m_DrNetworkAddr.push_back(netAddrString);
                }
            }//if (thisPimIntf != NULL)
        }//for (i = 0; i < traverseNode->numberInterfaces; i++)


        //for Candidate RP information
        for (i = 0; i < traverseNode->numberInterfaces; i++)
        {
            thisPimIntf = &pim->interface[i];

            if (thisPimIntf != NULL)
            {
                //for RP
                if (thisPimIntf->RPFlag)
                {
                    //node is a candidate RP
                    stats.m_RpType = "Candidate";

                    //searching for group address range for candidate RP
                    RoutingPimSmRPAccessList* accessListInfo = NULL;
                    std::string groupRange;
                    groupRange.clear();

                    char groupAddrString[MAX_STRING_LENGTH];
                    char groupMaskString[MAX_STRING_LENGTH];

                    tmpListItem = thisPimIntf->rpData.rpAccessList->first;

                    if (tmpListItem != NULL)
                    {
                        unsigned int numGroup =
                                      thisPimIntf->rpData.rpAccessList->size;

                        while (numGroup)
                        {
                            accessListInfo =
                              (RoutingPimSmRPAccessList*)(tmpListItem->data);

                            strcpy(groupAddrString,"");
                            strcpy(groupMaskString,"");

                            IO_ConvertIpAddressToString(
                                  accessListInfo->groupAddr,groupAddrString);

                            IO_ConvertIpAddressToString(
                                  accessListInfo->groupMask,groupMaskString);

                            groupRange.append(groupAddrString);
                            groupRange += "/";
                            groupRange.append(groupMaskString);

                            //below is done for having unique entries
                            if (!stats.m_GroupAddrRange.size())
                            {
                                stats.m_GroupAddrRange.push_back(
                                                                 groupRange);
                            }
                            else
                            {
                                for (j=0;j<stats.m_GroupAddrRange.size();j++)
                                {
                                    if (!stats.m_GroupAddrRange[j].compare(
                                                                 groupRange))
                                    {
                                        alreadyExist = TRUE;
                                        break;
                                    }
                                }

                                if (!alreadyExist)
                                {
                                    stats.m_GroupAddrRange.push_back(
                                                                 groupRange);
                                }
                            }

                            groupRange.clear();

                            numGroup--;
                            tmpListItem = tmpListItem->next;
                        }
                    }//if (tmpListItem == NULL)
                }//if (thisPimIntf->RPFlag)
            }//if (thisPimIntf != NULL)
        }

        // Now we have the stats for the node with BSR, DR and Candidate RP
        // with GroupAddressRange info.
        // Insert into the database now.
        STATSDB_HandlePimSmStatusTableInsert(traverseNode, stats);

        //now working for RPs
        //now check (*,G) tree for RP status
        RoutingPimSmTreeInformationBase* treeInfo = &pimSmData->treeInfoBase;
        RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;
        TreeInfoRowMap* rowPtrMap = treeInfo->rowPtrMap;
        TreeInfoRowMapIterator mapIterator;
        char groupAddrString[MAX_STRING_LENGTH];

        if (treeInfo->numEntries != 0)
        {
            for (mapIterator = rowPtrMap->begin();
                 mapIterator != rowPtrMap->end();
                 mapIterator++)
            {
                rowPtr = mapIterator->second;
                if (rowPtr->treeState == ROUTING_PIMSM_G)
                {
                    NodeAddress groupAddress = rowPtr->grpAddr;
                    NodeAddress RPAddr = rowPtr->RPointAddr;

                    // Check if any of my interface address is same as RPAddr
                    for (j = 0;
                         j < (unsigned int) traverseNode->numberInterfaces;
                         j++)
                    {

                        thisPimIntf = &pim->interface[j];

                        if (thisPimIntf != NULL)
                        {
                            if (thisPimIntf->ipAddress == RPAddr)
                            {
                                //I am RP node

                                //Init the stats variable again for RP entry
                                stats.m_BsrType.clear();
                                stats.m_RpType.clear();
                                stats.m_GroupAddr.clear();
                                stats.m_GroupAddrRange.clear();
                                stats.m_IsDR = FALSE;
                                stats.m_DrNetworkAddr.clear();

                                //set the status as RP
                                stats.m_RpType = "RP";

                                IO_ConvertIpAddressToString(groupAddress,
                                                            groupAddrString);

                                stats.m_GroupAddr.append(groupAddrString);

                                // Now we have the RP with GroupAddress
                                // Insert additional rows into the database.
                                STATSDB_HandlePimSmStatusTableInsert(
                                                        traverseNode, stats);
                            }
                        }
                    }//for (j = 0; j < traverseNode->numberInterfaces; j++)
                }
            }
        }//if (treeInfo->numEntries != 0)

        //Init the stats variable again
        stats.m_BsrType.clear();
        stats.m_RpType.clear();
        stats.m_GroupAddr.clear();
        stats.m_GroupAddrRange.clear();
        stats.m_IsDR = FALSE;
        stats.m_DrNetworkAddr.clear();

        //next node
        traverseNode = traverseNode->nextNodeData;
    }
}


void HandleStatsDBPimSmSummaryTableInsertion(Node* node)
{
    StatsDb* db = node->partitionData->statsDb;
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;
    PimData* pim = NULL;

    // Check if the Table exists.
    if (!db ||!db->statsPimTable ||
        !db->statsPimTable->createPimSmSummaryTable)
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

        pim = (PimData*) NetworkIpGetMulticastRoutingProtocol(traverseNode,
                                                    MULTICAST_PROTOCOL_PIM);

        if (pim == NULL || pim->modeType == ROUTING_PIM_MODE_DENSE)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        PimDataSm* pimSmData = (PimDataSm*) pim->pimModePtr;

        if (pimSmData == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        StatsDBPimSmSummary stats;

        stats.m_NumHelloSent = pimSmData->smSummaryStats.m_NumHelloSent;
        stats.m_NumHelloRecvd = pimSmData->smSummaryStats.m_NumHelloRecvd;
        stats.m_NumJoinPruneSent =
                                pimSmData->smSummaryStats.m_NumJoinPruneSent;
        stats.m_NumJoinPruneRecvd =
                               pimSmData->smSummaryStats.m_NumJoinPruneRecvd;
        stats.m_NumCandidateRPSent =
                              pimSmData->smSummaryStats.m_NumCandidateRPSent;
        stats.m_NumCandidateRPRecvd =
                             pimSmData->smSummaryStats.m_NumCandidateRPRecvd;
        stats.m_NumBootstrapSent =
                                pimSmData->smSummaryStats.m_NumBootstrapSent;
        stats.m_NumBootstrapRecvd =
                               pimSmData->smSummaryStats.m_NumBootstrapRecvd;
        stats.m_NumRegisterSent =
                                 pimSmData->smSummaryStats.m_NumRegisterSent;
        stats.m_NumRegisterRecvd =
                                pimSmData->smSummaryStats.m_NumRegisterRecvd;
        stats.m_NumRegisterStopSent =
                             pimSmData->smSummaryStats.m_NumRegisterStopSent;
        stats.m_NumRegisterStopRecvd =
                            pimSmData->smSummaryStats.m_NumRegisterStopRecvd;
        stats.m_NumAssertSent = pimSmData->smSummaryStats.m_NumAssertSent;
        stats.m_NumAssertRecvd = pimSmData->smSummaryStats.m_NumAssertRecvd;

        // At this point we have the overall stats per node. Insert into the
        // database now.
        STATSDB_HandlePimSmSummaryTableInsert(traverseNode, stats);

        //Init the stats variables again for peg count over time period
        pimSmData->smSummaryStats.m_NumHelloSent = 0;
        pimSmData->smSummaryStats.m_NumHelloRecvd = 0;
        pimSmData->smSummaryStats.m_NumJoinPruneSent = 0;
        pimSmData->smSummaryStats.m_NumJoinPruneRecvd = 0;
        pimSmData->smSummaryStats.m_NumCandidateRPSent = 0;
        pimSmData->smSummaryStats.m_NumCandidateRPRecvd = 0;
        pimSmData->smSummaryStats.m_NumBootstrapSent = 0;
        pimSmData->smSummaryStats.m_NumBootstrapRecvd = 0;
        pimSmData->smSummaryStats.m_NumRegisterSent = 0;
        pimSmData->smSummaryStats.m_NumRegisterRecvd = 0;
        pimSmData->smSummaryStats.m_NumRegisterStopSent = 0;
        pimSmData->smSummaryStats.m_NumRegisterStopRecvd = 0;
        pimSmData->smSummaryStats.m_NumAssertSent = 0;
        pimSmData->smSummaryStats.m_NumAssertRecvd = 0;

        //next node
        traverseNode = traverseNode->nextNodeData;
    }
}


void HandleStatsDBPimDmSummaryTableInsertion(Node* node)
{
    StatsDb* db = node->partitionData->statsDb;
    PartitionData* partition = node->partitionData;
    Node* traverseNode = node;

    // Check if the Table exists.
    if (!db ||!db->statsPimTable ||
        !db->statsPimTable->createPimDmSummaryTable)
    {
        // Table does not exist
        return;
    }

    PimData* pim = NULL;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        pim = (PimData*) NetworkIpGetMulticastRoutingProtocol(traverseNode,
                                                    MULTICAST_PROTOCOL_PIM);

        if (pim == NULL || pim->modeType == ROUTING_PIM_MODE_SPARSE)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        PimDmData* pimDmData = (PimDmData*) pim->pimModePtr;

        if (pimDmData == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        StatsDBPimDmSummary stats;

        stats.m_NumHelloSent = pimDmData->dmSummaryStats.m_NumHelloSent;
        stats.m_NumHelloRecvd = pimDmData->dmSummaryStats.m_NumHelloRecvd;
        stats.m_NumJoinPruneSent =
                                pimDmData->dmSummaryStats.m_NumJoinPruneSent;
        stats.m_NumJoinPruneRecvd =
                               pimDmData->dmSummaryStats.m_NumJoinPruneRecvd;
        stats.m_NumGraftSent = pimDmData->dmSummaryStats.m_NumGraftSent;
        stats.m_NumGraftRecvd = pimDmData->dmSummaryStats.m_NumGraftRecvd;
        stats.m_NumGraftAckSent =pimDmData->dmSummaryStats.m_NumGraftAckSent;
        stats.m_NumGraftAckRecvd =
                                pimDmData->dmSummaryStats.m_NumGraftAckRecvd;
        stats.m_NumAssertSent = pimDmData->dmSummaryStats.m_NumAssertSent;
        stats.m_NumAssertRecvd = pimDmData->dmSummaryStats.m_NumAssertRecvd;

        // At this point we have the overall stats per node. Insert into the
        // database now.
        STATSDB_HandlePimDmSummaryTableInsert(traverseNode, stats);

        //Init the stats variables again for peg count over time period
        pimDmData->dmSummaryStats.m_NumHelloSent = 0;
        pimDmData->dmSummaryStats.m_NumHelloRecvd = 0;
        pimDmData->dmSummaryStats.m_NumJoinPruneSent = 0;
        pimDmData->dmSummaryStats.m_NumJoinPruneRecvd = 0;
        pimDmData->dmSummaryStats.m_NumGraftSent = 0;
        pimDmData->dmSummaryStats.m_NumGraftRecvd = 0;
        pimDmData->dmSummaryStats.m_NumGraftAckSent = 0;
        pimDmData->dmSummaryStats.m_NumGraftAckRecvd = 0;
        pimDmData->dmSummaryStats.m_NumAssertSent = 0;
        pimDmData->dmSummaryStats.m_NumAssertRecvd = 0;

        //next node
        traverseNode = traverseNode->nextNodeData;
    }
}


void HandleStatsDBPimMulticastNetSummaryTableInsertion(Node* node)
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

    PimData* pim = NULL;

    while (traverseNode != NULL)
    {
        if (traverseNode->partitionId != partition->partitionId)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        pim = (PimData*) NetworkIpGetMulticastRoutingProtocol(traverseNode,
                                                    MULTICAST_PROTOCOL_PIM);

        if (pim == NULL)
        {
            traverseNode = traverseNode->nextNodeData;
            continue;
        }

        StatsDBMulticastNetworkSummaryContent stats;

        if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
        {
            strcpy(stats.m_ProtocolType,"PIM-SM");
        }
        else if (pim->modeType == ROUTING_PIM_MODE_DENSE)
        {
            strcpy(stats.m_ProtocolType,"PIM-DM");
        }

        stats.m_NumDataSent = pim->pimMulticastNetSummaryStats.m_NumDataSent;
        stats.m_NumDataRecvd =
                             pim->pimMulticastNetSummaryStats.m_NumDataRecvd;
        stats.m_NumDataForwarded =
                         pim->pimMulticastNetSummaryStats.m_NumDataForwarded;
        stats.m_NumDataDiscarded =
                         pim->pimMulticastNetSummaryStats.m_NumDataDiscarded;

        // At this point we have the overall stats per node. Insert into the
        // database now.
        STATSDB_HandleMulticastNetSummaryTableInsert(traverseNode, stats);

        //Init the stats variables again for peg count over time period
        //strcpy(pim->pimMulticastNetSummaryStats.m_ProtocolType,"");
        pim->pimMulticastNetSummaryStats.m_NumDataSent = 0;
        pim->pimMulticastNetSummaryStats.m_NumDataRecvd = 0;
        pim->pimMulticastNetSummaryStats.m_NumDataForwarded = 0;
        pim->pimMulticastNetSummaryStats.m_NumDataDiscarded = 0;

        //next node
        traverseNode = traverseNode->nextNodeData;
    }
}
#endif

//---------------------------------------------------------------------------
// FUNCTION   :: RoutingPimHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address for Pim-SM
// and Pim-DM
// PARAMETERS ::
// + node               : Node* : Pointer to Node structure
// + interfaceIndex     : Int32 : interface index
// + oldAddress         : Address* : current address
// + subnetMask         : NodeAddress : subnetMask
// + networkType        : NetworkType : type of network protocol
// RETURN :: void : NULL
//---------------------------------------------------------------------------

void RoutingPimHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType)
{
    // Get the PIM data structure
    PimData* pim = (PimData*)
    NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    NetworkDataIp* ip = node->networkData.networkVar;
    IpInterfaceInfoType* interfaceInfo =
                      ip->interfaceInfo[interfaceIndex];

    // PIM-SM is only supported for IPv4
    if (networkType == NETWORK_IPV6)
    {
        return;
    }

    // currently interface address state is invalid, hence no need to update
    // tables as no communication can occur from this interface while the 
    // interface is in invalid address state
    if (interfaceInfo->addressState  == INVALID)
    {
        return;
    }

    pim->interface[interfaceIndex].subnetMask =
        NetworkIpGetInterfaceSubnetMask(node, interfaceIndex);

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        if (pim->interface[interfaceIndex].smInterface)
        {
            pim->interface[interfaceIndex].smInterface->drInfo.ipAddr =
                NetworkIpGetInterfaceAddress(node, interfaceIndex);
        }
    }

    // We do the following when PIM_SM acquires the new address
    // Before an interface goes down or changes primary IP address,
    // a Hello message with a zero HoldTime should be sent
    // immediately (with the old IP address if the IP address
    // changed).  This will cause PIM neighbors to remove this
    // neighbor (or its old IP address) immediately.


    RoutingPimSendHelloPacket(node,
                              interfaceIndex,
                              0,
                              pim->interface[interfaceIndex].ipAddress);

    if (DEBUG_HELLO)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %d @ %s: Sent "
               "Hello for interface %d with old address"
               " and holdtime 0\n",
                node->nodeId,
                clockStr,
                interfaceIndex);
    }

    // After an interface has changed its IP address, it MUST send
    // a Hello message with its new IP address. 
    RoutingPimSendHelloPacket(node,
                  interfaceIndex);

    if (DEBUG_HELLO)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %d @ %s: Sent "
               "Hello for interface %d with new address\n",
                node->nodeId,
                clockStr,
                interfaceIndex);
    }

    pim->interface[interfaceIndex].ipAddress =
        NetworkIpGetInterfaceAddress(node, interfaceIndex);

}

