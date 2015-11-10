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
*  File: pimsm.c
*
*
*  PUPPOSE: To simulate Protocol Independent Multicast (Sparse Mode) Routing
*           Protocol (PIM-SM). It resides at the network layer just above IP.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#include "qualnet_error.h"
#include "network_ip.h"

#include "multicast_igmp.h"
#include "list.h"
#include "multicast_pim.h"
#include "transport_udp.h"
#ifdef ADDON_BOEINGFCS
#include "multicast_ces_rpim_sm.h"
#include "mi_ces_multicast_mesh.h"
#endif
#ifdef CYBER_CORE
#include "multicast_igmp.h"
#endif
#include "partition.h"

#define DEBUG 0
#define DEBUG_BS 0
#define DEBUG_RP_CHANGE 0
#define DEBUG_JP 0
#define DEBUG_ERRORS 0 
#define DEBUG_FORWARD_PACKET 0
#define DEBUG_GROUP_MEMBERSHIP_COUNTERS 0

/**************************************************************************/
/*                      VARIOUS MACROS USED IN PIMSM                      */
/**************************************************************************/

void
RoutingPimSmPrintNghbrList(Node* node);

BOOL
RoutingPimSmCheckSwitchToSpt(Node* node, NodeAddress grpAddr,
         NodeAddress srcAddr,
         RoutingPimSmDataPacketsPerGroup* sptSwitchoverDataPacketsForGroup);

BOOL
RoutingPimSmJoinG(Node* node, NodeAddress groupAddr,
                  NodeAddress srcAddr, int interfaceId);
BOOL
RoutingPimSmJoinSG(Node* node, NodeAddress groupAddr,
                   NodeAddress srcAddr, int interfaceId);
BOOL
RoutingPimSmJoinRP(Node* node, NodeAddress groupAddr,
                   NodeAddress srcAddr, int interfaceId);

BOOL
RoutingPimSmPruneSGrpt(Node* node, NodeAddress groupAddr,
                       NodeAddress srcAddr, int interfaceId);

unsigned int
RoutingPimSmImmediateOutListRP(Node* node, NodeAddress grpAddr,
    NodeAddress srcAddr, RoutingPimSmTreeInfoBaseRow  * routingTableRowPtr);

unsigned int
RoutingPimSmImmediateOutListG(Node* node, NodeAddress grpAddr,
    NodeAddress srcAddr, RoutingPimSmTreeInfoBaseRow  * routingTableRowPtr,
    int incomingIntf);

unsigned int
RoutingPimSmImmediateOutListSG(Node* node, NodeAddress grpAddr,
    NodeAddress srcAddr, RoutingPimSmTreeInfoBaseRow * routingTableRowPtr);

unsigned int
RoutingPimSmInheritedOutListSGrpt(Node* node, NodeAddress grpAddr,
    NodeAddress srcAddr, RoutingPimSmTreeInfoBaseRow * routingTableRowPtr);

unsigned int
RoutingPimSmInheritedOutListSG(Node* node, NodeAddress grpAddr,
    NodeAddress srcAddr, RoutingPimSmTreeInfoBaseRow * routingTableRowPtr);

BOOL
RoutingPimSmIncludeG(Node* node, NodeAddress grpAddr,
                     NodeAddress srcAddr, int interfaceId);
BOOL
RoutingPimSmIncludeSG(Node* node, NodeAddress grpAddr,
                      NodeAddress srcAddr, int interfaceId);
BOOL
RoutingPimSmExcludeSG(Node* node, NodeAddress grpAddr,
                      NodeAddress srcAddr, int interfaceId);

BOOL
RoutingPimSmLostAssertG(Node* node, NodeAddress grpAddr,
                        NodeAddress srcAddr, int interfaceId);
BOOL
RoutingPimSmLostAssertSG(Node* node, NodeAddress grpAddr,
                         NodeAddress srcAddr, int interfaceId);
BOOL
RoutingPimSmLostAssertSGrpt(Node* node, NodeAddress grpAddr,
                NodeAddress srcAddr, int interfaceId);

BOOL
RoutingPimSmRPTJoinDesiredG(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId);

BOOL
RoutingPimSmJoinDesiredRP();

BOOL
RoutingPimSmPruneDesiredSGrpt(Node* node, NodeAddress grpAddr,
            NodeAddress srcAddr, int interfaceId);

NodeAddress
RoutingPimSmSelectNewRPFG(Node* node, NodeAddress groupAddr);

NodeAddress
RoutingPimSmSelectNewRPFSG(Node* node,
                           NodeAddress groupAddr,
                           NodeAddress srcAddr);
NodeAddress
RoutingPimSmSelectNewRPFSGrpt(Node* node, NodeAddress groupAddr,
                              NodeAddress srcAddr);
void
RoutingPimSmDeleteList(Node* node, LinkedList* list, BOOL type);
BOOL
IsInterfaceInThisPimSmList(LinkedList* list, int interfaceId);

/**************************************************************************/
/*    VARIOUS FUNCTIONS USED FOR BOOTSTRAP & RP SELECTION PROCESS         */
/**************************************************************************/




/**************************************************************************/
/*        VARIOUS FUNCTIONS USED FOR FORWARDING PROCESS                   */
/**************************************************************************/
BOOL
RoutingPimSmCheckDirectlyConnectedSource(Node* node,
        NodeAddress srcAddr, int interfaceId);

RoutingPimSmForwardingTableRow*
RoutingPimSmSearchForwardingTable(Node* node, int interfaceId,
                                  NodeAddress grpAddr, NodeAddress srcAddr);

RoutingPimSmForwardingTableRow*
RoutingPimSmSetMulticastForwardingTable(Node* node, int interfaceId,
                                NodeAddress groupAddr, NodeAddress srcAddr);

void
RoutingPimSmUpdateSPTbit(Node* node, NodeAddress srcAddr,
                         NodeAddress grpAddr, int interfaceId,
                         RoutingPimSmTreeInfoBaseRow* routTableRowPtr);


/**************************************************************************/
/*             VARIOUS FUNCTIONS USED FOR HELLO PROCESS                   */
/**************************************************************************/
void
RoutingPimSmElectDR(Node* node, int interfaceIndex);


/**************************************************************************/
/*           VARIOUS FUNCTIONS USED FOR JOIN_PRUNE PROCESS                */
/**************************************************************************/

void
RoutingPimSmHandleGUpstreamStateMachine(Node* node, NodeAddress srcAddr,
                    int interfaceId, NodeAddress groupAddr,
                    clocktype joinPruneHoldTime,
                    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr,
                    BOOL suppressJoinPrune);

void
RoutingPimSmHandleSGUpstreamStateMachine(Node* node, NodeAddress srcAddr,
        int interfaceId, NodeAddress groupAddr,
        clocktype joinPruneHoldTime,
        RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr,
        BOOL suppressJoinPrune);

void
RoutingPimSmHandleSGrptUpstreamStateMachine(Node* node, NodeAddress srcAddr,
        NodeAddress groupAddr,
        clocktype joinPruneHoldTime,
        RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr);

RoutingPimSmDownstream*
RoutingPimSmSetDownstreamList(Node* node, int interfaceId,
                            RoutingPimSmTreeInfoBaseRow* routingTableRowPtr);

LinkedList*
RoutingPimSmCheckForSGRptPeriodicMessage(Node* node, NodeAddress grpAddress);

/*********************/
void
RoutingPimSmSetMulticastTreeState(Node* node, int interfaceId,
                            NodeAddress groupAddr, NodeAddress upstreamNbr,
                            RoutingPimSmMulticastTreeState treeState);

void
RoutingPimSmHandleGDownstreamStateMachine(Node* node, NodeAddress srcAddr,
        int interfaceId, NodeAddress groupAddr, unsigned int numJoinedSrc,
        unsigned int numPrunedSrc);
/**************************************************************************/
/*           VARIOUS FUNCTIONS USED FOR REGISTER PROCESS                  */
/**************************************************************************/
void
RoutingPimSmSetRegisterState(Node* node,
                             RoutingPimSmRegisterState registerState);
BOOL
RoutingPimSmSendRegisterPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress);

BOOL
RoutingPimSmCouldRegisterSG(Node* node, NodeAddress srcAddr,
                            NodeAddress grpAddr, int incomingInterface);

void
RoutingPimSmSendRegisterStopMessage(Node* node, NodeAddress grpAddr,
            NodeAddress mcastSrc, NodeAddress registerPktSrc, int outIntf);

/**************************************************************************/
/*             VARIOUS FUNCTIONS USED FOR ASSERT PROCESS                  */
/**************************************************************************/
void
RoutingPimSmSendAssertPacket(Node* node, NodeAddress srcAddr,
                             NodeAddress grpAddr, int outIntfId,
                             RoutingPimSmAssertType  assertType);

BOOL
RoutingPimSmCouldAssertG(Node* node, NodeAddress grpAddr,
                         NodeAddress srcAddr, int interfaceId);
BOOL
RoutingPimSmCouldAssertSG(Node* node, NodeAddress grpAddr,
                          NodeAddress srcAddr, int interfaceId);
void
RoutingPimSmSetMyAssert(Node* node, NodeAddress groupAddr,
                        NodeAddress srcAddr, int interfaceId,
                        RoutingPimSmAssertMetric * myMetric);
BOOL
RoutingPimSmComparePktAssertMetric(BOOL pktRPTbit,
            unsigned int pktPreference, unsigned int pktMetric,
            NodeAddress pktAddr, BOOL myRPTbit, unsigned int myPreference,
            unsigned int myMetric, NodeAddress myAddr);
void
RoutingPimSmPerformActionA1(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId,
                            RoutingPimSmMulticastTreeState treeState);
void
RoutingPimSmPerformActionA2(Node* node, NodeAddress assertSender,
                            RoutingPimSmAssertPacket* assertPkt,
                            RoutingPimSmMulticastTreeState treeState,
                            unsigned int interfaceId);
void
RoutingPimSmPerformActionA3(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId,
                            RoutingPimSmMulticastTreeState treeState);
void
RoutingPimSmPerformActionA4(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId,
                            RoutingPimSmMulticastTreeState treeState);
void
RoutingPimSmPerformActionA5(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId,
                            RoutingPimSmMulticastTreeState treeState);
void
RoutingPimSmPerformActionA6(Node* node, NodeAddress assertSender,
                            RoutingPimSmAssertPacket* assertPkt,
                            RoutingPimSmMulticastTreeState treeState,
                            int interfaceId);
BOOL
RoutingPimSmAssertTrackingDesiredG(Node* node, NodeAddress grpAddr,
                                   NodeAddress srcAddr, int interfaceId);
BOOL
RoutingPimSmAssertTrackingDesiredSG(Node* node, NodeAddress grpAddr,
                                    NodeAddress srcAddr, int interfaceId);

/**************************************************************************/
/*             VARIOUS FUNCTIONS USED FOR BSR MECHANISM                  */
/**************************************************************************/
ListItem*
RoutingPimSmIsRPMappingPresent(Node* node,
                               RoutingPimEncodedGroupAddr* grpInfo,
                               RoutingPimEncodedUnicastAddr* rpInfo);


void
RoutingPimSmReadCandidateRPparameters(Node* node,
                                      int interfaceIndex,
                                      const NodeInput* nodeInput);

void
RoutingPimSmReadCandidateBSRparameters(Node* node,
                                      int interfaceIndex,
                                      const NodeInput* nodeInput);

void
RoutingPimSmCheckForCandidateBSR(Node* node,
                                 int interfaceIndex,
                                 const NodeInput* nodeInput);

BOOL
RoutingPimSmCheckForStaticRP(Node* node,
                             const NodeInput* nodeInput);

LinkedList*
RoutingPimSmParseGroupRangeStr(Node* node,
                               char* groupRangeStr,
                               LinkedList* rpAccessList);

void
RoutingPimSmForwardBSM(Node* node,
                       Message* bootstrapMsg,
                       int incomingIntf);

void
RoutingPimSmOriginateBSM(Node* node,
                         int interfaceIndex);

BOOL
RoutingPimSmIsEmptyCRPMsg(Message* candidateRPMsg);

clocktype
RoutingPimSmCalculateBSRandOverride(Node* node,
                                    int interfaceIndex);

BOOL
RoutingPimSmIsEmptyBSM(Message* bootstrapMsg);

char*
RoutingPimSmFindGrpInBSM(Node* node,
                         Message* bootstrapMsg,
                         NodeAddress grpPrefix,
                         unsigned char maskLength);

void
RoutingPimSmRemoveGrpToRPMappingFromRPSet(Node* node,
                                          ListItem* rpListItem);

void
RoutingPimSmRemoveUnreachableMappings(Node* node,
                                      Message* bootstrapMsg);

void
RoutingPimSmStoreRPSet(Node* node,
                       int interfaceIndex,
                       Message* bootstrapMsg);

void
RoutingPimSmRefreshRPSet(Node* node,
                         int interfaceIndex);

void
RoutingPimSmCBSRStateMachine(Node* node,
                             int interfaceIndex,
                             RoutingPimSmBSREvent eventType,
                             Message* bootstrapMsg);

void
RoutingPimSmNonCBSRStateMachine(Node* node,
                                int interfaceIndex,
                                RoutingPimSmBSREvent eventType,
                                Message* bootstrapMsg);

void
RoutingPimSmRemoveBSRState(Node* node,
                           int interfaceIndex);

BOOL
RoutingPimSmIsBSMPreferred(Node* node,
                           unsigned char pktBSRPriority,
                           NodeAddress pktBSRAddr,
                           int interfaceIndex);

BOOL
RoutingPimSmDoBSMProcessingChecks(Node* node,
                                  NodeAddress bsmSrcAddr,
                                  int interfaceIndex,
                                  Message* bootstrapMsg);


BOOL
RoutingPimSmCheckForStaticRP(Node* node,
                             const NodeInput* nodeInput);

void
RoutingPimSmPrintRPGroupAccessList(Node* node,
                              int interfaceIndex);

void
RoutingPimSmPrintBootStrapMessage(Node* node,
                                  Message* bootStrapMsg);

void
RoutingPimSmPrintCandidateRPAdvMessage(Node* node,
                                       Message* candidateRPMsg);

void
RoutingPimSmCheckForRPChangeG(Node* node);

void
RoutingPimSmCheckForRPChangeSG(Node* node);

/**************************************************************************/
/*     VARIOUS FUNCTIONS USED FOR GROUP MEMBERSHIP STATE COUNTERS         */
/**************************************************************************/

int RoutingPimSmGroupMemStateIncrementCounter(Node* node,
                                      NodeAddress groupAddr,
                                      NodeAddress upstream, int upInterface);

int RoutingPimSmGroupMemStateDecrementCounter(Node* node,
                                      NodeAddress groupAddr,
                                      NodeAddress upstream, int upInterface);

unsigned int RoutingPimSmCountValidOutgoingInterfaceCount(Node* node,
                                         RoutingPimSmTreeInfoBaseRow* entry);


/*-------------------------End of function declaration---------------------*/
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

/*
* FUNCTION     : PimSmEncapsulationHeaderSetEncapVer()
* PURPOSE      : Set the value of encapsulatedIpV for
*                RoutingPimSmEncapsulationHeader
* ASSUMPTION   : None
* RETURN VALUE : void
*/
static void PimSmEncapsulationHeaderSetEncapVer(unsigned char *rpsEncapVhl,
                                              unsigned char encapsulatedIpV)
{
    //masks encapsulated_ip_v within boundry range
    encapsulatedIpV = encapsulatedIpV & maskChar(6, 8);

    //clears the first 3 bits
    *rpsEncapVhl = *rpsEncapVhl & maskChar(4, 8);

    //setting value of encapsulatedIpV in rpsEncapVhl
    *rpsEncapVhl = *rpsEncapVhl | LshiftChar(encapsulatedIpV, 3);
}


/*
* FUNCTION     : PimSmEncapsulationHeaderSetEncapHL()
* PURPOSE      : Set the value of encapsulatedIpHl for
*                RoutingPimSmEncapsulationHeader
* ASSUMPTION   : None
* RETURN VALUE : void
*/
static void PimSmEncapsulationHeaderSetEncapHL(unsigned char *rpsEncapVhl,
                                            unsigned char encapsulatedIpHl)
{
    //masks encapsulated_ip_hl within boundry range
    encapsulatedIpHl = encapsulatedIpHl & maskChar(4, 8);

    //clears the first 3 bits
    *rpsEncapVhl = *rpsEncapVhl & maskChar(1, 3);

    //setting value of encapsulatedIpHl in rpsEncapVhl
    *rpsEncapVhl = *rpsEncapVhl | encapsulatedIpHl;
}


/*
* FUNCTION     : PimSmEncapsulationHeaderSetEncapFlags()
* PURPOSE      : Set the value of encapsulatedIpFlags for
*                RoutingPimSmEncapsulationHeader
* ASSUMPTION   : None
* RETURN VALUE : void
*/
static void PimSmEncapsulationHeaderSetEncapFlags(UInt16 *rpsEncapFlagOff,
                                               UInt16 encapsulatedIpFlags)
{
    //masks encapsulated_ip_flags within boundry range
    encapsulatedIpFlags = encapsulatedIpFlags & maskShort(14, 16);

    //clears the first 3 bits
    *rpsEncapFlagOff = *rpsEncapFlagOff & maskShort(4, 16);

    //setting value of encapsulatedIpFlags in rpsEncapFlagOff
    *rpsEncapFlagOff =
        *rpsEncapFlagOff | LshiftShort(encapsulatedIpFlags, 3);
}


/*
* FUNCTION     : PimSmEncapsulationHeaderSetEncapFragOffset()
* PURPOSE      : Set the value of encapsulatedIpFragmentOffset for
*                RoutingPimSmEncapsulationHeader
* ASSUMPTION   : None
* RETURN VALUE : void
*/
static void PimSmEncapsulationHeaderSetEncapFragOffset(
    UInt16 *rpsEncapFlagOff, UInt16 encapsulatedIpFragmentOffset)
{
    //masks encapsulated_ip_fragment_offset within boundry range
    encapsulatedIpFragmentOffset =
        encapsulatedIpFragmentOffset & maskShort(4, 16);

    //clears the last 13 bits
    *rpsEncapFlagOff = *rpsEncapFlagOff & maskShort(1, 3);

    //setting value of encapsulatedIpFragmentOffset in rpsEncapFlagOff
    *rpsEncapFlagOff = *rpsEncapFlagOff | encapsulatedIpFragmentOffset;
}


/*
* FUNCTION     : PimSmEncapsulationHeaderGetEncapVer()
* PURPOSE      : Returns the value of encapsulatedIpV for
*                RoutingPimSmEncapsulationHeader
* ASSUMPTION   : None
* RETURN VALUE : unsigned char
*/
static unsigned char PimSmEncapsulationHeaderGetEncapVer(
    unsigned char rpsEncapVhl)
{
    unsigned char encapsulatedIpV = rpsEncapVhl;

    //clears the last 13 bits
    encapsulatedIpV = encapsulatedIpV & maskChar(1, 3);

    //right shifts so that last 3 bits represent encapsulatedIpV
    encapsulatedIpV = RshiftChar(encapsulatedIpV, 3);

    return encapsulatedIpV;
}


/*
* FUNCTION     : PimSmEncapsulationHeaderGetEncapHL()
* PURPOSE      : Returns the value of encapsulatedIpHl for
*                RoutingPimSmEncapsulationHeader
* ASSUMPTION   : None
* RETURN VALUE : unsigned char
*/
static unsigned char PimSmEncapsulationHeaderGetEncapHL(
    unsigned char rpsEncapVhl)
{
    unsigned char encapsulatedIpHl = rpsEncapVhl;

    //clears the last 13 bits
    encapsulatedIpHl = encapsulatedIpHl & maskChar(4, 8);

    return encapsulatedIpHl;
}


/*
* FUNCTION     : PimSmEncapsulationHeaderGetEncapFlags()
* PURPOSE      : Returns the value of encapsulatedIpFlags for
*                RoutingPimSmEncapsulationHeader
* ASSUMPTION   : None
* RETURN VALUE : UInt16
*/
static UInt16 PimSmEncapsulationHeaderGetEncapFlags(UInt16 rpsEncapFlagOff)
{
    UInt16 encapsulatedIpFlags = rpsEncapFlagOff;

    //clears the last 13 bits
    encapsulatedIpFlags = encapsulatedIpFlags & maskShort(1, 3);

    //right shifts so that last 3 bits represent encapsulatedIpFlags
    encapsulatedIpFlags = RshiftShort(encapsulatedIpFlags, 3);

    return encapsulatedIpFlags;
}


/*
* FUNCTION     : PimSmEncapsulationHeaderGetEncapFragOffset()
* PURPOSE      : Returns the value of encapsulatedIpFragmentOffset for
*                RoutingPimSmEncapsulationHeader
* ASSUMPTION   : None
* RETURN VALUE : UInt16
*/
static UInt16 PimSmEncapsulationHeaderGetEncapFragOffset(
    UInt16 rpsEncapFlagOff)
{
    UInt16 encapsulated_ip_frag_offset = rpsEncapFlagOff;

    //clears the first 3 bits
    encapsulated_ip_frag_offset = encapsulated_ip_frag_offset &
                                  maskShort(4, 16);

    return encapsulated_ip_frag_offset;
}




/*
* FUNCTION     : PimSmEncodedSourceAddrGetReserved()
* PURPOSE      : Returns the value of reserved for
*                RoutingPimEncodedSourceAddr
* ASSUMPTION   : None
* RETURN VALUE : unsigned char
*/
static unsigned char PimSmEncodedSourceAddrGetReserved(
    unsigned char rpSimEsa)
{
    unsigned char reserved = rpSimEsa;

    //clears last three bits
    reserved = reserved & maskChar(1, 5);

    //setting value of reserved in rpSimEsa
    reserved = RshiftChar(reserved, 5);

    return reserved;
}


/*
* FUNCTION     : PimSmEncodedSourceAddrGetSparseBit()
* PURPOSE      : Returns the value of sparseBit for
*                RoutingPimEncodedSourceAddr
* ASSUMPTION   : None
* RETURN VALUE : BOOL
*/
static BOOL PimSmEncodedSourceAddrGetSparseBit(unsigned char rpSimEsa)
{
    unsigned char sparseBit = rpSimEsa;

    //clears all bits except sixth bit
    sparseBit = sparseBit & maskChar(6, 6);

    //right shift 2 places so that last bit represents sparseBit
    sparseBit = RshiftChar(sparseBit, 6);

    return (BOOL)sparseBit;
}

/*
* FUNCTION     : PimSmRegisterPacketGetBorder()
* PURPOSE      : Returns the value of border for RoutingPimSmRegisterPacket
* ASSUMPTION   : None
* RETURN VALUE : BOOL
*/
static BOOL PimSmRegisterPacketGetBorder(UInt32 rpSmRegPkt)
{
    UInt32 border = rpSmRegPkt;

    //clears all bits except first bit
    border = border & maskInt(1, 1);

    //right shift 31 places so that last bit represents border
    border = RshiftInt(border, 1);

    return (BOOL)border;
}
#endif

/*
*  FUNCTION    : RoutingPimSmInitTreeInfoBase()
*  PURPOSE     : Initializes treeInfo Base
*  ASSUMPTION  : None
*  RETURN VALUE: NULL
*/
void
RoutingPimSmInitTreeInfoBase(Node* node)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }

    RoutingPimSmTreeInformationBase* treeInfoBase = &pimDataSm->treeInfoBase;
    treeInfoBase->numEntries = 0;
    treeInfoBase->rowPtrMap = new TreeInfoRowMap;
}
#ifdef CYBER_CORE
/*!
 * \brief Check if the group address is a red group address for
 *        control message in red network
 *
 * \param node      Pointer to current node
 * \param groupAddr group address
 *
 * \return     TRUE if it is default red group address; otherwise FALSE
 *
 * Check if a group address is a group address in red network for
 * control message in red network. It is only called at red node.
 */
static
BOOL RoutingPimSmIsDefaultRedMulticastGroup(Node* node, NodeAddress groupAddr)
{
    if (groupAddr == MULTICAST_BROADCAST_ADDRESS ||
         groupAddr == ANY_ADDRESS ||
         groupAddr == ALL_PIM_ROUTER ||
         groupAddr == OSPFv2_ALL_SPF_ROUTERS ||
         groupAddr == OSPFv2_ALL_D_ROUTERS)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*!
 * \brief Check if the group address is a black group address for
 *        control message in red network
 *
 * \param node      Pointer to current node
 * \param groupAddr group address
 *
 * \return     TRUE if it is default black group address; otherwise FALSE
 *
 * Check if a group address is a group address in black network for
 * control message in red network. It is only called at black node.
 */
static
BOOL RoutingPimSmIsDefaultBlackMulticastGroup(Node* node, NodeAddress groupAddr)
{
    if (groupAddr == BROADCAST_MULTICAST_MAPPEDADDRESS ||
         groupAddr == ALL_PIM_ROUTER + DEFAULT_RED_BLACK_MULTICAST_MAPPING ||
         groupAddr == OSPFv2_ALL_SPF_ROUTERS + DEFAULT_RED_BLACK_MULTICAST_MAPPING ||
         groupAddr == OSPFv2_ALL_D_ROUTERS + DEFAULT_RED_BLACK_MULTICAST_MAPPING)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*!
 * \brief Initiate IGMP JOIN to black network
 *
 * \param node      Pointer to current node
 * \param redSrcAddr red source address
 * \param redGrpAddr red group  address
 * \param messageType IGMP message type
 *
 * \return     NONE
 *
 * Initiate IGMP JOIN/LEAVE message to black network, so (S,G) Join/Prune
 * message can be sent from the black node to setup or tear down the SPT
 */
static
void RoutingPimSmIAHEPInitIgmpMsgToBlackNetwork(Node* node,
                                                NodeAddress redSrcAddr,
                                                NodeAddress redGrpAddr,
                                                unsigned char messageType)
{
    // need to include the next red hop address now
    // as IAHEP node does run routing protocol
    NodeAddress nextHop;
    int outInterface;

    NetworkGetInterfaceAndNextHopFromForwardingTable(node,
                       redSrcAddr, &outInterface, &nextHop);
    if (nextHop == (unsigned) NETWORK_UNREACHABLE ||
       nextHop == (NodeAddress)0)
    {
        nextHop = redSrcAddr;
    }

    IAHEPCreateIgmpJoinLeaveMessage(
        node,
        redGrpAddr,
        messageType);

    if (IAHEP_DEBUG)
    {
       char addrStr[MAX_STRING_LENGTH];
       IO_ConvertIpAddressToString(redGrpAddr, addrStr);
       printf("\nRed Node: %d", node->nodeId);
       printf("\tJoins Group: %s\n", addrStr);
    }
}
#endif

/*
*  FUNCTION    : RoutingPimSmInitForwardingTable()
*  PURPOSE     : Initializes forwarding table
*  ASSUMPTION  : None
*  RETURN VALUE: NULL
*/
void
RoutingPimSmInitForwardingTable(Node* node)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }
    RoutingPimSmForwardingTable* fwdTable = &pimDataSm->forwardingTable;

    unsigned int size =
        sizeof(RoutingPimSmForwardingTableRow)*  PIM_INITIAL_TABLE_SIZE;

    fwdTable->numEntries = 0;
    fwdTable->numRowsAllocated = 10;
    fwdTable->rowPtr = (RoutingPimSmForwardingTableRow*)
                       MEM_malloc(size);
    memset(fwdTable->rowPtr, 0, size);
}

/*
*  FUNCTION    : RoutingPimSmGetNextHopOutInterface()
*  PURPOSE     : returns next hop and outgoing interface
*  ASSUMPTION  : None
*  RETURN VALUE: NULL
*/
void
RoutingPimSmGetNextHopOutInterface(Node* node,
                                   NodeAddress addr,
                                   NodeAddress* nextHop,
                                   int* outInterface)
{
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (addr ==
            NetworkIpGetInterfaceAddress(node, i))
        {
            *nextHop = addr;
            *outInterface = i;
        }
    }
}
/*
*  FUNCTION    : RoutingPimSmLocalMembersJoinOrLeave()
*  PURPOSE     : Handle local group membership Join & leave event
*  ASSUMPTION  : None
*  RETURN VALUE: NULL
*/
void
RoutingPimSmLocalMembersJoinOrLeave(Node* node,
                                    NodeAddress groupAddr,
                                    int interfaceId,
                                    LocalGroupMembershipEventType event)
{
    IgmpInterfaceInfoType* thisInterface = NULL;

    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    RoutingPimSmDataPacketsPerGroup *numDataPacketsPerGroup;
    IgmpData* igmp = IgmpGetDataPtr(node);
    ERROR_Assert(igmp, " IGMP data pointer not found");

    NetworkDataIp* ip = (NetworkDataIp*)node->networkData.networkVar;

    if (interfaceId >= 0)
    {
        thisInterface = igmp->igmpInterfaceInfo[interfaceId];
    }

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }
    NodeAddress RPAddr;
    NodeAddress nextHop;

    int outInterface = NETWORK_UNREACHABLE;
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr;

    bool initSGJoinorPrune = FALSE;
    if (ip->isSSMEnable &&
        Igmpv3IsSSMAddress(node, groupAddr) &&
        thisInterface->igmpVersion == IGMP_V3)
    {
        initSGJoinorPrune = TRUE;
    }

    /* check the RP For this group & find associated nextHop*/

    char groupAddress[MAX_STRING_LENGTH];
    IO_ConvertIpAddressToString(groupAddr,
                                groupAddress);

    if (DEBUG) {
        printf("Group Address is %s\n",
               groupAddress);
    }

    RPAddr = RoutingPimSmFindRPForThisGroup(node,
                                            groupAddr);
#ifdef CYBER_CORE
    BOOL defaultRedGrp = FALSE;
    BOOL redNodeNonRedIntf = FALSE;
    if (ip->iahepEnabled &&
        ip->iahepData->nodeType == RED_NODE /*&&
        !IsIAHEPRedSecureInterface(node, interfaceId)*/)
    {
        redNodeNonRedIntf = TRUE;
    }

    if (redNodeNonRedIntf == TRUE &&
        RoutingPimSmIsDefaultRedMulticastGroup(node, groupAddr))
    {
        defaultRedGrp = TRUE;
    }
    if (defaultRedGrp)
    {
        // let black node set up the (*.G) for these default red group
        IAHEPCreateIgmpJoinLeaveMessage(node,
                                        groupAddr,
                                        IGMP_MEMBERSHIP_REPORT_MSG);
    }
#endif
#ifdef ADDON_BOEINGFCS
    if (RPAddr == (NodeAddress)NETWORK_UNREACHABLE)
    {
        MulticastCesRpimRescheduleJoinOrLeave(node,
                                              groupAddr,
                                              interfaceId,
                                              event);
        return;
    }
#endif

    char rpAddress[MAX_STRING_LENGTH];
    IO_ConvertIpAddressToString(RPAddr,
                                rpAddress);

    if (DEBUG) {
        printf("RP Address is %s\n",
               rpAddress);
    }

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, RPAddr,
            &outInterface, &nextHop);

    if (nextHop == 0)
    {
        nextHop = RPAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHop,
                                           &outInterface);
    }

    if (outInterface == NETWORK_UNREACHABLE)
    {
        if (DEBUG)
        {
            printf("Node %u has invalid out interface towards RP for group"
                   "%s\n",node->nodeId,groupAddress);
        }
    }

#ifdef CYBER_CORE
    BOOL defaultBlkGrp = FALSE;
    BOOL blkNodeBlkIntf = FALSE;
    if (ip->iahepEnabled &&
        ip->iahepData->nodeType == BLACK_NODE &&
        IsIAHEPBlackSecureInterface(node, interfaceId))
    {
        blkNodeBlkIntf = TRUE;
    }

    if (blkNodeBlkIntf == TRUE &&
        RoutingPimSmIsDefaultBlackMulticastGroup(node, groupAddr))
    {
        defaultBlkGrp = TRUE;
    }
#endif

    switch (event)
    {
    case LOCAL_MEMBER_JOIN_GROUP:
    {
        if (DEBUG_JP)
        {
            char clockStr[100];

            ctoa(node->getNodeTime(), clockStr);
            printf("Node %d, Igmp notify about new membership "
                   "%u at interface %d at %s\n",
                   node->nodeId, groupAddr, interfaceId, clockStr);
        }


        pimDataSm->stats.numGroupJoin++;

#ifdef CYBER_CORE
        if ((ip->iahepEnabled &&
            ip->iahepData->nodeType == RED_NODE) &&
            !RoutingPimSmIsDefaultRedMulticastGroup(node, groupAddr))
        {
            // if RED node also is a member of the red group
            // but the group is not default red group
            // set up the (S' G') in black network, S' is
            // the black node mapped to the red RP
            RoutingPimSmIAHEPInitIgmpMsgToBlackNetwork(
                    node, RPAddr, groupAddr, IGMP_MEMBERSHIP_REPORT_MSG);
        }

#endif
        if (pimDataSm->sptSwitchThresholdInfo.threshold !=
                (unsigned int)ROUTING_PIMSM_SWITCH_SPT_THRESHOLD_INFINITE)
        {
            if (pimDataSm->sptSwitchThresholdInfo.
                 numDataPacketsReceivedPerGroup == NULL)
            {
                ListInit(node, &pimDataSm->sptSwitchThresholdInfo.
                               numDataPacketsReceivedPerGroup);
                    numDataPacketsPerGroup =
                        (RoutingPimSmDataPacketsPerGroup*)
                        MEM_malloc(sizeof(RoutingPimSmDataPacketsPerGroup));
                memset(numDataPacketsPerGroup, 0,
                        sizeof(RoutingPimSmDataPacketsPerGroup));
                numDataPacketsPerGroup->grpAddr = groupAddr;
                numDataPacketsPerGroup->numDataPacketsReceived = 0;
                ListInsert(node, pimDataSm->sptSwitchThresholdInfo.
                               numDataPacketsReceivedPerGroup,
                               node->getNodeTime(),
                           (void*)numDataPacketsPerGroup);
            }
            else
            {
                BOOL found = FALSE;
                ListItem* tempListItem = NULL;
                tempListItem = pimDataSm->sptSwitchThresholdInfo.
                               numDataPacketsReceivedPerGroup->first;
                while (tempListItem)
                {
                    numDataPacketsPerGroup =
                    (RoutingPimSmDataPacketsPerGroup*) tempListItem->data;
                    if (numDataPacketsPerGroup->grpAddr == groupAddr)
                    {
                        found = TRUE;
                        break;
                    }
                    tempListItem = tempListItem->next;
                }

                if (found == FALSE)
                {
                    numDataPacketsPerGroup =
                        (RoutingPimSmDataPacketsPerGroup*)
                        MEM_malloc(sizeof(RoutingPimSmDataPacketsPerGroup));
                    memset(numDataPacketsPerGroup, 0,
                            sizeof(RoutingPimSmDataPacketsPerGroup));
                    numDataPacketsPerGroup->grpAddr = groupAddr;
                    numDataPacketsPerGroup->numDataPacketsReceived = 0;
                    ListInsert(node, pimDataSm->sptSwitchThresholdInfo.
                                   numDataPacketsReceivedPerGroup,
                                   node->getNodeTime(),
                           (void*)numDataPacketsPerGroup);
                }
            }
        }

        if (DEBUG_JP)
        {
            printf("Node %u has D-Router of interface(%d) is %x \n",
                       node->nodeId, interfaceId,
                       pim->interface[interfaceId].
                            smInterface->drInfo.ipAddr);
            printf("this interface has address %x \n",
                       pim->interface[interfaceId].ipAddress);
        }


        if (pim->interface[interfaceId].ipAddress
            == pim->interface[interfaceId].smInterface->drInfo.ipAddr)
        {
            if (DEBUG_JP) {
                printf("\n Node %u: modify downstream \n",
                           node->nodeId);
            }

            if (DEBUG) {
                printf("\n Node %u: modify downstream \n",
                       node->nodeId);
            }

            if (pim->interface[interfaceId].switchDirectToSPT == TRUE
                || initSGJoinorPrune
#ifdef CYBER_CORE
               && defaultBlkGrp == FALSE
#endif
               )
            {
                ListItem* srcGrpListItem = NULL;
                srcGrpListItem = pim->interface[interfaceId].srcGrpList->first;
                RoutingPimSourceGroupList* srcGrpListPtr;

                while (srcGrpListItem)
                {
                    srcGrpListPtr = (RoutingPimSourceGroupList*)
                        srcGrpListItem->data;

                    if (srcGrpListPtr->groupAddr != groupAddr)
                    {
                        srcGrpListItem = srcGrpListItem->next;
                        continue;
                    }

                    treeInfoBaseRowPtr =
                        RoutingPimSmSearchTreeInfoBaseForThisGroup(
                            node,
                            srcGrpListPtr->groupAddr,
                            srcGrpListPtr->srcAddr,
                            ROUTING_PIMSM_SG);

                    if (treeInfoBaseRowPtr == NULL)
                    {
                        treeInfoBaseRowPtr =
                        RoutingPimSmSetMulticastTreeInfoBase(
                                    node,
                                    srcGrpListPtr->groupAddr,
                                    srcGrpListPtr->srcAddr,
                                    ROUTING_PIMSM_SG);
                    }

                    RoutingPimSmHandleDownstreamStateMachine(
                        node,
                        srcGrpListPtr->srcAddr,
                        interfaceId,
                        srcGrpListPtr->groupAddr,
                        treeInfoBaseRowPtr,
                        ROUTING_PIM_JOIN);

                    RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                        node,
                        srcGrpListPtr->srcAddr,
                        &outInterface,
                        &nextHop);


                    if (outInterface == NETWORK_UNREACHABLE)
                    {
                        srcGrpListItem = srcGrpListItem->next;
                        continue;
                    }

                    RoutingPimSmHandleUpstreamStateMachine(
                                node,
                                srcGrpListPtr->srcAddr,
                                interfaceId,
                                srcGrpListPtr->groupAddr,
                                treeInfoBaseRowPtr);

                    srcGrpListItem = srcGrpListItem->next;
                }
            }
            else
            {
                treeInfoBaseRowPtr =
                    RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                                    groupAddr,
                                    pim->interface[interfaceId].ipAddress,
                            ROUTING_PIMSM_G);

                if (treeInfoBaseRowPtr == NULL)
                {
                    if (DEBUG) {
                        printf("Time to built NewEntry in \
                               TreeInfoBase\n");
                    }

                    treeInfoBaseRowPtr =
                        RoutingPimSmSetMulticastTreeInfoBase(node,
                                    groupAddr,
                                    pim->interface[interfaceId].ipAddress,
                                    ROUTING_PIMSM_G);

#ifdef ADDON_DB
                    if (outInterface != NETWORK_UNREACHABLE)
                    {
                        if (RPAddr == pim->interface[outInterface].ipAddress)
                        {
                            StatsDBConnTable::MulticastConnectivity stats;

                            stats.nodeId = node->nodeId;
                            stats.destAddr = groupAddr;
                            strcpy(stats.rootNodeType,"RP");
                            stats.rootNodeId =
                                MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                     RPAddr);
                            stats.outgoingInterface = interfaceId;
                            stats.upstreamNeighborId =
                                  MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                    nextHop);
                            stats.upstreamInterface = outInterface;

                            //insert this new entry into
                            //multicast_connectivity cache
                            STATSDB_HandleMulticastConnCreation(node,stats);
                        }
                    }
#endif
                }
                /*
                 *  the (*, G) Join travels hop-by-hop towards RP for the
                 *  group, and in each router it passes through,
                 *  SET multicast tree state = group G
                 */
                RoutingPimSmHandleDownstreamStateMachine(node,
                        pim->interface[interfaceId].ipAddress,
                        interfaceId, groupAddr,
                        treeInfoBaseRowPtr,
                        ROUTING_PIM_JOIN);

                if (outInterface == NETWORK_UNREACHABLE)
                {
                    if (DEBUG_JP)
                    {
                        printf("Node %u has RP 0.0.0.0 for group"
                           "%x\n",node->nodeId,groupAddr);
                    }
                    return;
                }

                if (RPAddr != pim->interface[outInterface].ipAddress)
                {
                    if (DEBUG_JP) {
                        printf("Node %u: not RP,so also modify upstream\n",
                               node->nodeId);
                    }

                    RoutingPimSmHandleUpstreamStateMachine(node,
                                    RPAddr,
                                    interfaceId, groupAddr,
                                    treeInfoBaseRowPtr);
                }
            }//end of else
        }
        break;
    }

    case LOCAL_MEMBER_LEAVE_GROUP:
    {
        if (DEBUG_JP)
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
                printf("Node %d, Igmp notify about expired membership "
                   "%u at interface %d at time %s\n",
                   node->nodeId, groupAddr, interfaceId,
                   clockStr);
        }

        RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr = NULL;
        pimDataSm->stats.numGroupLeave++;
        if (pimDataSm->sptSwitchThresholdInfo.threshold !=
                (unsigned int)ROUTING_PIMSM_SWITCH_SPT_THRESHOLD_INFINITE &&
                pimDataSm->sptSwitchThresholdInfo.
                            numDataPacketsReceivedPerGroup)
        {
            ListItem* tempListItem = NULL;

            tempListItem = pimDataSm->sptSwitchThresholdInfo.
                       numDataPacketsReceivedPerGroup->first;
            while (tempListItem)
            {
                numDataPacketsPerGroup =
                    (RoutingPimSmDataPacketsPerGroup*) tempListItem->data;
                if (numDataPacketsPerGroup->grpAddr == groupAddr)
                {
                    ListGet(node,
                            pimDataSm->sptSwitchThresholdInfo.
                            numDataPacketsReceivedPerGroup,
                            tempListItem,
                            TRUE,
                            FALSE);
                    if (pimDataSm->sptSwitchThresholdInfo.
                            numDataPacketsReceivedPerGroup->size == 0)
                    {
                        ListFree(node,
                                 pimDataSm->sptSwitchThresholdInfo.
                                    numDataPacketsReceivedPerGroup,
                                 FALSE);
                        pimDataSm->sptSwitchThresholdInfo.
                                    numDataPacketsReceivedPerGroup = NULL;
                    }
                    break;
                }
                tempListItem = tempListItem->next;
            }
        }

        treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                             (node, groupAddr,
                             pim->interface[interfaceId].ipAddress,
                             ROUTING_PIMSM_G);

        if (treeInfoBaseRowPtr == NULL
            && (pim->interface[interfaceId].switchDirectToSPT == FALSE)
            && initSGJoinorPrune == FALSE)
        {
            char buf[MAX_STRING_LENGTH];
            sprintf(buf,
              "Node %d received Leave request before Joining the group %u\n",
                    node->nodeId, groupAddr);
            ERROR_ReportWarning(buf);
        }
        else
        {
            if (DEBUG_JP)
            {
                printf("Node %u has D_Router of interface(%d) is %d \n",
                    node->nodeId, interfaceId,
                    pim->interface[interfaceId].smInterface->drInfo.ipAddr);
            }

            if (pim->interface[interfaceId].ipAddress
                == pim->interface[interfaceId].smInterface->drInfo.ipAddr)
            {
                if (pim->interface[interfaceId].switchDirectToSPT == TRUE
                    || initSGJoinorPrune)
                {
                    ListItem* srcGrpListItem = NULL;
                    srcGrpListItem = pim->interface[interfaceId].srcGrpList->first;
                    RoutingPimSourceGroupList* srcGrpListPtr;

                    while (srcGrpListItem)
                    {
                        srcGrpListPtr = (RoutingPimSourceGroupList*)
                            srcGrpListItem->data;

                        if (srcGrpListPtr->groupAddr != groupAddr)
                        {
                            srcGrpListItem = srcGrpListItem->next;
                            continue;
                        }

                        treeInfoBaseRowPtr =
                        RoutingPimSmSearchTreeInfoBaseForThisGroup(
                                    node,
                                    srcGrpListPtr->groupAddr,
                                    srcGrpListPtr->srcAddr,
                                    ROUTING_PIMSM_SG);

                        if (treeInfoBaseRowPtr == NULL)
                        {
                            char buf[MAX_STRING_LENGTH];
                            sprintf(buf,
                              "Node %d received Leave"
                              "request before Joining the group %u\n",
                               node->nodeId, srcGrpListPtr->groupAddr);
                            ERROR_ReportWarning(buf);
                        }
                        else
                        {
                            RoutingPimSmHandleDownstreamStateMachine(
                                node,
                                srcGrpListPtr->srcAddr,
                                interfaceId,
                                srcGrpListPtr->groupAddr,
                                treeInfoBaseRowPtr,
                                ROUTING_PIM_PRUNE);

                            RoutingPimSmHandleUpstreamStateMachine(
                                node,
                                srcGrpListPtr->srcAddr,
                                interfaceId,
                                srcGrpListPtr->groupAddr,
                                treeInfoBaseRowPtr);
                        }//end of else

                        srcGrpListItem = srcGrpListItem->next;
                    }//end of while
                }//end of if
                else
                {
                    /*
                     *  the (*, G) Join travels hop-by-hop towards RP for the
                     *  group, and in each router it passes through,
                     *  SET multicast tree state = group G
                     */
                    RoutingPimSmHandleDownstreamStateMachine(node,
                            pim->interface[interfaceId].ipAddress,
                            interfaceId, groupAddr,
                            treeInfoBaseRowPtr,
                            ROUTING_PIM_PRUNE);

                    if (outInterface == NETWORK_UNREACHABLE)
                    {
                        if (DEBUG)
                        {
                            printf("Node %u has RP 0.0.0.0 for group"
                                   "%s\n",node->nodeId,groupAddress);
                        }
                        return;
                    }

#ifdef ADDON_DB
                    if (RPAddr == pim->interface[outInterface].ipAddress)
                    {
                        //remove this entry from multicast_connectivity cache
                        StatsDBConnTable::MulticastConnectivity stats;

                        stats.nodeId = node->nodeId;
                        stats.destAddr = groupAddr;
                        strcpy(stats.rootNodeType,"RP");
                        stats.rootNodeId =
                            MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                  RPAddr);
                        stats.outgoingInterface = interfaceId;
                        stats.upstreamNeighborId =
                                  MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                    nextHop);
                        stats.upstreamInterface = outInterface;

                        STATSDB_HandleMulticastConnDeletion(node, stats);
                    }
#endif

                    if (RPAddr != pim->interface[outInterface].ipAddress)
                    {
                        if (DEBUG) {
                            printf("Node %u: not RP,so also modify \
                                   upstream\n",node->nodeId);
                        }
                        RoutingPimSmHandleUpstreamStateMachine(node,
                                        RPAddr,
                                        interfaceId, groupAddr,
                                        treeInfoBaseRowPtr);
                    }
                }//end of else
            }   //if DR
        }//end of else
        break;
    }
    default:
    {
#ifdef EXATA
        if (node->partitionData->isEmulationMode)
        {
            char buf[MAX_STRING_LENGTH];
            sprintf(buf, "Node %d Unknown IGMP Event\n", node->nodeId);
            ERROR_ReportWarning(buf);
        }
        else
            ERROR_Assert(FALSE, "Unknown IGMP Event\n");
#else
        ERROR_Assert(FALSE, "Unknown IGMP Event\n");
#endif
        break;
    }
    }
}

/*
*  FUNCTION    : RoutingPimSmForwardingFunction()
*  PURPOSE     : Handle the forwarding process for PIMSM
*  ASSUMPTION  : None
*  RETURN VALUE: NULL
 */

void RoutingPimSmForwardingFunction(Node* node,
                                    Message* msg,
                                    NodeAddress grpAddr,
                                    int incomingInterface,
                                    BOOL* packetWasRouted,
                                    NodeAddress prevHop)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    NodeAddress srcAddr;
    int sourceInterface;
    NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;

    prevHop = prevHop;     //warnings
    NodeAddress RPAddr;
    int RPInterface;

    BOOL IAmRP = FALSE;
    BOOL originateByMe = FALSE;
    BOOL IAmDR = FALSE;
    BOOL isloopbackPacket = FALSE;
    int i;
    RoutingPimSmTreeInfoBaseRow       * treeInfoBaseRowPtr = NULL ;
    RoutingPimSmForwardingTableRow    * fwdTableRowPtr = NULL ;
    LinkedList* outIntfList;
    ListItem* listItem;
    BOOL sameInt = TRUE;
    BOOL registerSent = FALSE;
    RoutingPimSmDataPacketsPerGroup* sptSwitchoverDataPacketsForGroup = NULL;
    NodeAddress nextHopForSrc = 0;
    NodeAddress nextHopForRP = 0;
    int forward = 0;
    int originaliif = incomingInterface;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    bool initSGJoinorPrune = FALSE;
    if (ip->isSSMEnable && Igmpv3IsSSMAddress(node, grpAddr))
    {
        initSGJoinorPrune = TRUE;
    }
    srcAddr = ipHeader->ip_src;
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        srcAddr,
                                                        &sourceInterface,
                                                        &nextHopForSrc);

    if (nextHopForSrc == 0)
    {
        nextHopForSrc = srcAddr ;
    }
    else if (nextHopForSrc == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           srcAddr,
                                           &nextHopForSrc,
                                           &sourceInterface);
    }

    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (srcAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            sourceInterface = CPU_INTERFACE;
            break;
        }
    }
    if (DEBUG || DEBUG_FORWARD_PACKET)
    {
        char clockStr[100];
        char addrStr1[MAX_STRING_LENGTH];
        char addrStr2[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr, node);
        IO_ConvertIpAddressToString(
                            srcAddr, addrStr1);
        IO_ConvertIpAddressToString(
                            grpAddr, addrStr2);
        printf("\n");
        printf("\n Time %s : ",clockStr);
        printf("Node %u get M_castPkt(src = %s & dest = %s) from intf %d",
               node->nodeId, addrStr1, addrStr2, incomingInterface);
    }

    pimDataSm->stats.numDataPktReceived++;

    RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        RPAddr,
                                                        &RPInterface,
                                                        &nextHopForRP);


    if (DEBUG_FORWARD_PACKET)
    {
        printf("RPInterface for node %d is %d\n",
               node->nodeId,
               RPInterface);
        char rpAddress[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(RPAddr,
                                    rpAddress);

        printf("RP Address is %s\n",
               rpAddress);
    }
    if (nextHopForRP == 0)
    {
        nextHopForRP = RPAddr ;
    }
    else if (nextHopForRP == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHopForRP,
                                           &RPInterface);
    }

    /*check if it is the RP */
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (RPAddr == pim->interface[i].ipAddress)
        {
            IAmRP  = TRUE;
#ifdef CYBER_CORE
            if (!RoutingPimSmIsDefaultBlackMulticastGroup(node, grpAddr) &&
                !RoutingPimSmIsDefaultRedMulticastGroup(node, grpAddr))
            {
                MESSAGE_AddInfo(node, msg, sizeof(BOOL), INFO_TYPE_RPProcessed);
                memcpy(MESSAGE_ReturnInfo(msg, INFO_TYPE_RPProcessed),
                   &IAmRP,
                   sizeof(BOOL));
            }
#endif
            break;
        }
    }

    if (pimDataSm->sptSwitchThresholdInfo.threshold !=
        (unsigned int)ROUTING_PIMSM_SWITCH_SPT_THRESHOLD_INFINITE &&
        pimDataSm->sptSwitchThresholdInfo.numDataPacketsReceivedPerGroup
        != NULL)
        {
            ListItem* tempListItem = NULL;
            tempListItem = pimDataSm->sptSwitchThresholdInfo.
                           numDataPacketsReceivedPerGroup->first;
            BOOL grpEntryfound = FALSE;
            while (tempListItem)
            {
                sptSwitchoverDataPacketsForGroup =
                    (RoutingPimSmDataPacketsPerGroup*) tempListItem->data;
                if (sptSwitchoverDataPacketsForGroup->grpAddr == grpAddr
#ifdef CYBER_CORE
                    &&(((ip->iahepEnabled &&
                       ip->iahepData->nodeType == BLACK_NODE &&
                       !RoutingPimSmIsDefaultBlackMulticastGroup(node, grpAddr))
                       ||
                       (ip->iahepEnabled &&
                       ip->iahepData->nodeType == RED_NODE &&
                       !RoutingPimSmIsDefaultRedMulticastGroup(node, grpAddr)))
                       ||
                       !ip->iahepEnabled)
#endif
                   )
                {
                    sptSwitchoverDataPacketsForGroup->
                        numDataPacketsReceived++;
                    grpEntryfound = TRUE;
                    break;
                }
                tempListItem = tempListItem->next;
            }
            if (!grpEntryfound)
            {
                sptSwitchoverDataPacketsForGroup = NULL;
            }
        }
    if (DEBUG_FORWARD_PACKET)
    {
        printf(" it has ipHeader->ip_src  = %x \n", ipHeader->ip_src);
        printf(" it has ipHeader->ip_dst  = %x \n", ipHeader->ip_dst);
        printf(" it has ipHeader->ip_ttl  = %d \n", ipHeader->ip_ttl);
        printf(" it has ipHeader->ip_protocol  = %d \n", ipHeader->ip_p);
    }

    if (pim->interface[incomingInterface].ipAddress ==
            pim->interface[incomingInterface].smInterface->drInfo.ipAddr)
    {
        IAmDR = TRUE;
    }

    /* check for any matching entry in the current forwarding table */
    fwdTableRowPtr = RoutingPimSmSearchForwardingTable(node,
                     incomingInterface, grpAddr, srcAddr);
    if (fwdTableRowPtr == NULL)
    {
        if (DEBUG_JP) {
            printf("    Time to built new entry in fwd table \n");
        }
        fwdTableRowPtr = RoutingPimSmSetMulticastForwardingTable(node,
                         incomingInterface, grpAddr, srcAddr);
        fwdTableRowPtr->lastPktTTL = ipHeader->ip_ttl;
    }

    /* check if this is a new pkt & the node is the originator of this Pkt*/
    if ((prevHop == ANY_IP && NetworkIpIsMyIP(node, ipHeader->ip_src))
         &&
        (ipHeader->ip_src == pim->interface[incomingInterface].ipAddress))
    {
        originateByMe = TRUE;
        incomingInterface = CPU_INTERFACE;
        if (DEBUG || DEBUG_FORWARD_PACKET)
        {
            char clockStr[100];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr, node);
            printf("\n Time %s : ",clockStr);
            printf("I am the source of the packet");
            printf("This is a new Packet");
        }
        pimDataSm->stats.numDataPktOriginate++;
        pimDataSm->stats.numDataPktReceived--;

        treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                             (node, grpAddr, srcAddr, ROUTING_PIMSM_SG);

        if (treeInfoBaseRowPtr == NULL)
        {
            treeInfoBaseRowPtr = RoutingPimSmSetMulticastTreeInfoBase
                                 (node, grpAddr, srcAddr, ROUTING_PIMSM_SG);
        }

#ifdef ADDON_DB
        if (prevHop == ANY_IP && NetworkIpIsMyIP(node, ipHeader->ip_src))
        {
            pim->pimMulticastNetSummaryStats.m_NumDataSent++;
        }
#endif

        i = NetworkIpGetInterfaceIndexFromAddress(node,srcAddr);
        if (pim->interface[i].ipAddress !=
           pim->interface[i].smInterface->drInfo.ipAddr)
        {
            NodeAddress nextHop = ANY_IP;
            if (DEBUG || DEBUG_FORWARD_PACKET)
            {
                char clockStr[100];
                char addrStr1[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr,node);
                IO_ConvertIpAddressToString(
                                    nextHop, addrStr1);
                printf("\n Time %s : ",clockStr);
                printf("Source Node %d sending m_pkt to DR %s on int %d",
                    node->nodeId, addrStr1,i);
            }

#ifdef ADDON_DB
            StatsDBConnTable::MulticastConnectivity stats;
            stats.nodeId = node->nodeId;
            stats.destAddr = grpAddr;
            strcpy(stats.rootNodeType,"Source");
            stats.rootNodeId= MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                       ipHeader->ip_src);
            stats.outgoingInterface = i;

                if (prevHop == (unsigned int)ANY_INTERFACE)
            {
                //this is my packet
                stats.upstreamNeighborId = stats.rootNodeId;
                //adding interface index as CPU_INTERFACE
                stats.upstreamInterface = CPU_INTERFACE;
            }
            else
            {
                stats.upstreamNeighborId =
                     MAPPING_GetNodeIdFromInterfaceAddress(node,prevHop);
                stats.upstreamInterface = incomingInterface;
            }

            //if using database, update the multicast connection table
            STATSDB_HandleMulticastConnCreation(node,stats);
#endif

            NetworkIpSendPacketToMacLayer(node,
                                  MESSAGE_Duplicate(node, msg),
                                  i,
                                  nextHop);
            forward++;
        }

        if (forward != 0)
        {
            pimDataSm->stats.numDataPktForward++;
        }

    }

    if (prevHop != ANY_IP && NetworkIpIsMyIP(node, ipHeader->ip_src))
    {
        isloopbackPacket = TRUE;
    }

#ifdef ADDON_DB
    if (!originateByMe)
    {
        pim->pimMulticastNetSummaryStats.m_NumDataRecvd++;
    }
    else
    {
        if (isloopbackPacket &&
            NetworkIpIsPartOfMulticastGroup(node, ipHeader->ip_dst))
        {
            pim->pimMulticastNetSummaryStats.m_NumDataRecvd++;
        }
    }
#endif


    /* On receipt on a data from S to G on interface iif:
     * check if it is directly connected to source
     */
    /* if (DirectlyConnected(S) == TRUE AND iif == RPF_interface(S) ) {
       set KeepaliveTimer(S,G) to Keepalive_Period
        # Note: a register state transition or UpstreamJPState(S,G)
        # transition may happen as a result of restarting
        # KeepaliveTimer, and must be dealt with here.}*/
    if ((RoutingPimSmCheckDirectlyConnectedSource(node, srcAddr,
            incomingInterface) && (sourceInterface == incomingInterface)))
    {
#ifdef ADDON_DB
        if (IAmRP)
        {
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = grpAddr;
                strcpy(stats.rootNodeType,"Source");
                stats.rootNodeId= MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                           ipHeader->ip_src);
                stats.outgoingInterface = ANY_DEST;

                if (prevHop == (unsigned int)ANY_INTERFACE)
                {
                    //this is my packet
                    stats.upstreamNeighborId = stats.rootNodeId;
                    //adding interface index as CPU_INTERFACE
                    stats.upstreamInterface = CPU_INTERFACE;
                }
                else
                {
                    stats.upstreamNeighborId =
                         MAPPING_GetNodeIdFromInterfaceAddress(node,prevHop);
                    stats.upstreamInterface = incomingInterface;
                }

                //if using database, update the multicast connection table
                STATSDB_HandleMulticastConnCreation(node,stats);
        }
#endif

        RoutingPimSmKeepAliveTimerInfo    timerInfo;
        clocktype delay;

        if (!(treeInfoBaseRowPtr
            &&
            treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG))
        {
            treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                             (node, grpAddr, srcAddr, ROUTING_PIMSM_SG);

            if (treeInfoBaseRowPtr == NULL)
            {
                treeInfoBaseRowPtr = RoutingPimSmSetMulticastTreeInfoBase
                                 (node, grpAddr, srcAddr, ROUTING_PIMSM_SG);
                if (DEBUG || DEBUG_FORWARD_PACKET)
                {
                    printf("\n Created (S,G) for node %d : ",node->nodeId);
                }
            }
        }

        if (DEBUG || DEBUG_FORWARD_PACKET)
        {
            printf("\n STARTED (S,G) KA timer for node %d : ",node->nodeId);
        }
#ifdef CYBER_CORE
        BOOL blkSPTReady = FALSE;
        if (ip->iahepEnabled &&
            ip->iahepData->nodeType == BLACK_NODE &&
            IsIAHEPBlackSecureInterface(node, incomingInterface) &&
            treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join &&
            !RoutingPimSmIsDefaultBlackMulticastGroup(node, grpAddr))
        {
            // when blkSPT is ready, and get packets from src, start use
            // SPT instead of RPT
            blkSPTReady = TRUE;
        }
        //BOOL redSPTReady = FALSE;
        //if (ip->iahepData != NULL &&
        //    ip->iahepData->nodeType == RED_NODE &&
        //    !IsIAHEPRedSecureInterface(node, incomingInterface) &&
        //    treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join &&
        //    !RoutingPimSmIsDefaultRedMulticastGroup(node, grpAddr))
        //{
            // when redSPT is ready, and get packets from src, start use
            // SPT instead of RPT
            //redSPTReady = TRUE;
        //}
#endif
        if (treeInfoBaseRowPtr->keepAliveExpiryTimer == 0)
        {
            /* set KeepaliveTimer(S, G) to Keepalive_Period */
            timerInfo.srcAddr = srcAddr;
            timerInfo.grpAddr = grpAddr;
            timerInfo.intfIndex = (unsigned int)incomingInterface;
            timerInfo.treeState = ROUTING_PIMSM_SG;

            /* Note the current KeepAliveTimerSeq */

            delay = (clocktype) (pimDataSm->routingPimSmKeepaliveTimeout);


            RoutingPimSetTimer(node,
                               incomingInterface,
                               MSG_ROUTING_PimSmKeepAliveTimeOut,
                               (void*) &timerInfo, delay);

        }

        treeInfoBaseRowPtr->keepAliveExpiryTimer = node->getNodeTime() +
            (clocktype) (pimDataSm->routingPimSmKeepaliveTimeout);

        /*
         *  Routing PimSm Handle Register State and if necessary
         *   set the register state transition accordingly
         */

        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u PimSm Checking for Registering pkt \n",
                   node->nodeId);
        }
        BOOL  registerTransition = FALSE;

        //RFC:4601::Section:4.4.1
        if (treeInfoBaseRowPtr->oldRPatSrcDR != RPAddr)
        {
            if ((treeInfoBaseRowPtr->registerState ==
                                PimSm_Register_JoinPending)
                ||
                (treeInfoBaseRowPtr->registerState ==
                                PimSm_Register_Prune))
            {
                treeInfoBaseRowPtr->registerState = PimSm_Register_Join;
                treeInfoBaseRowPtr->isTunnelPresent = TRUE;
                treeInfoBaseRowPtr->registerStopTimerSeq++;
            }
            treeInfoBaseRowPtr->oldRPatSrcDR = RPAddr;
        }

        if ((forward == 0) && RoutingPimSmCouldRegisterSG(node,
                srcAddr, grpAddr, incomingInterface)
#ifdef CYBER_CORE
            && ((blkSPTReady == FALSE)/* ||
               (redSPTReady == FALSE )*/)
#endif
             )
        {
            registerTransition = TRUE;
            if (treeInfoBaseRowPtr->registerState == PimSm_Register_NoInfo)
            {
                treeInfoBaseRowPtr->isTunnelPresent = TRUE;
                treeInfoBaseRowPtr->registerState = PimSm_Register_Join;
            }

            if (IAmDR && IAmRP)
            {
                treeInfoBaseRowPtr->registerState = PimSm_Register_Join;
                if (treeInfoBaseRowPtr->isTunnelPresent)
                {
#ifdef ADDON_BOEINGFCS
                    if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface)
                        && !(node->networkData.networkVar->iahepEnabled
                             && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                        && (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G
                            || treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG)
                        && treeInfoBaseRowPtr->upstreamState != PimSm_JoinPrune_Join
                        && RoutingPimSmCountValidOutgoingInterfaceCount(node, treeInfoBaseRowPtr) > 0)
                    {
                        if (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
                            && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh)
                        {
                            int counter = RoutingPimSmGroupMemStateIncrementCounter(
                                            node,
                                            treeInfoBaseRowPtr->grpAddr,
                                            treeInfoBaseRowPtr->upstream,
                                            treeInfoBaseRowPtr->upInterface);
                            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                            {
                                printf("\nnode %d Increment counter in RoutingPimSmForwardingFunction. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                   node->nodeId, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, treeInfoBaseRowPtr->upInterface, counter);
                            }
                            if (counter == 1)
                            {
                                MiCesMulticastMeshInitiateMeshFormation(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, pimDataSm->miMulticastMeshTimeout, FALSE);
                            }
                        }
                    }
#endif // ADDON_BOEINGFCS
                    treeInfoBaseRowPtr->upstreamState = PimSm_JoinPrune_Join;
                    RoutingPimSmHandleUpstreamStateMachine(node,
                                               srcAddr,
                                               incomingInterface,
                                               grpAddr,
                                               treeInfoBaseRowPtr);

                    registerSent = FALSE;
                    if (DEBUG_FORWARD_PACKET)
                    {
                        char clockStr[100];
                        TIME_PrintClockInSecond(node->getNodeTime(),
                                        clockStr, node);
                        printf("\n Time %s : ",clockStr);
                        printf("Node %d is RP and DR hence no register"
                             "sent", node->nodeId);
                    }
                }
            }
            else if (IAmDR)
            {
#ifndef CYBER_CORE
                if (treeInfoBaseRowPtr->isTunnelPresent &&
                    pim->interface[originaliif].switchDirectToSPT == FALSE &&
                    initSGJoinorPrune == FALSE)
#else
                if (treeInfoBaseRowPtr->isTunnelPresent
                    &&
                    ((pim->interface[originaliif].switchDirectToSPT == FALSE  &&
                    initSGJoinorPrune == FALSE)
                     || RoutingPimSmIsDefaultBlackMulticastGroup(node, grpAddr)
                    ))
#endif
                {
                    /* create & send register packet */
                    Message* tempMsg = MESSAGE_Duplicate(node, msg);
                    BOOL forwardByRP = FALSE;
                    if (RPInterface >= 0)
                    {
                        forwardByRP = RoutingPimSmSendRegisterPacket(node,
                            tempMsg, pim->interface[RPInterface].ipAddress,
                            RPAddr);
                    }
                    MESSAGE_Free(node, tempMsg);
                    if (forwardByRP == TRUE)
                    {
                        registerSent = TRUE;
                        if (DEBUG_FORWARD_PACKET)
                        {
                            char clockStr[100];
                            TIME_PrintClockInSecond(node->getNodeTime(),
                                            clockStr, node);
                            printf("\n Time %s : ",clockStr);
                            printf("Node %d is DR hence register"
                                 "sent", node->nodeId);
                        }
                    }
#ifdef ADDON_DB
                    if (forwardByRP == FALSE)
                    {

                        HandleNetworkDBEvents(
                            node,
                            msg,
                            incomingInterface,
                            "NetworkPacketDrop",
                            "No Route to RP",
                            0,
                            0,
                            0,
                            0);
                    }
#endif
#ifdef ADDON_DB
                    if (!originateByMe)
                    {
                        pim->pimMulticastNetSummaryStats.
                                                    m_NumDataForwarded++;
                    }
#endif
                }
            }
            else
            {
                if (originateByMe == TRUE)
                {
                    if (treeInfoBaseRowPtr->isTunnelPresent)
                    {
                        if (treeInfoBaseRowPtr->SPTbit == FALSE)
                        {
                            *packetWasRouted = TRUE;
                            MESSAGE_Free(node, msg);
                            return;
                        }
                    }
                }   //only if source;
            }
        }
        else if (RoutingPimSmCouldRegisterSG(node,
                srcAddr, grpAddr, incomingInterface) == FALSE)
        {
            /* check the register state & update accordingly */
            if (treeInfoBaseRowPtr->registerState != PimSm_Register_NoInfo)
            {
                registerTransition = TRUE;
                /* RoutingPimSmSetRegisterState to PimSm_Register_NoInfo */
                if (treeInfoBaseRowPtr->registerState == PimSm_Register_Join)
                {
                    treeInfoBaseRowPtr->isTunnelPresent = FALSE;
                }

                treeInfoBaseRowPtr->registerState = PimSm_Register_NoInfo;
            }
        }
        /* # Note: a register state transition or UpstreamJPState(S,G)
           # transition may happen as a result of restarting
           # KeepaliveTimer, and must be dealt with here.*/
        if (registerTransition == FALSE)
        {
            Int32 incomingSrcInterface = CPU_INTERFACE;
            if (incomingInterface == CPU_INTERFACE)
            {
                // I am the source node and packet is from upper layer
                // Need to find the correct source interface to pass to
                // upstream state machine
                incomingSrcInterface =
                    NetworkIpGetInterfaceIndexFromAddress(node,srcAddr);
            }
            else
            {
                // I am not the source; incoming interface is the source
                // interface
                incomingSrcInterface = incomingInterface;
            }
            RoutingPimSmHandleUpstreamStateMachine(
                                    node,
                                    srcAddr,
                                    incomingSrcInterface,
                                    grpAddr,
                                    treeInfoBaseRowPtr);
        }
    }//if directly connected;

    if (!(treeInfoBaseRowPtr
        &&
        treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG))
    {
        treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, ROUTING_PIMSM_SG);
    }

    if (treeInfoBaseRowPtr != NULL )
    {

        if (DEBUG_FORWARD_PACKET)
        {
            printf("treeInfoBaseRowPtr not NULL\n");
        }

        /*if (iif == RPF_interface(S) AND UpstreamJPState(S,G) == Joined
        AND inherited_olist(S,G) != NULL ) {
                set KeepaliveTimer(S,G) to Keepalive_Period
        }*/
        if (incomingInterface == sourceInterface
            &&
            treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join &&
            RoutingPimSmInheritedOutListSG(
                                        node,
                                        grpAddr,
                                        srcAddr,
                                        treeInfoBaseRowPtr) != 0)
        {
            if (treeInfoBaseRowPtr->keepAliveExpiryTimer == 0)
            {
                RoutingPimSmKeepAliveTimerInfo    timerInfo;
                clocktype delay;
                /* set KeepaliveTimer(S, G) to Keepalive_Period */
                timerInfo.srcAddr = srcAddr;
                timerInfo.grpAddr = grpAddr;
                timerInfo.intfIndex = (unsigned int)incomingInterface;
                timerInfo.treeState = ROUTING_PIMSM_SG;

                /* Note the current KeepAliveTimerSeq */

                delay = (clocktype)(pimDataSm->routingPimSmKeepaliveTimeout);


                RoutingPimSetTimer(node,
                                   incomingInterface,
                                   MSG_ROUTING_PimSmKeepAliveTimeOut,
                                   (void*) &timerInfo, delay);

            }
            treeInfoBaseRowPtr->keepAliveExpiryTimer = node->getNodeTime() +
                (clocktype)(pimDataSm->routingPimSmKeepaliveTimeout);
            if (DEBUG_FORWARD_PACKET)
            {
                printf("Node %u A Setting KA \n",
                   node->nodeId);
            }
        }
        /*Update_SPTbit(S,G,iif)*/
        if (treeInfoBaseRowPtr->SPTbit == FALSE)
        {
            RoutingPimSmUpdateSPTbit(node,
                                     srcAddr,
                                     grpAddr,
                                     incomingInterface,
                                     treeInfoBaseRowPtr);
        }
    }
    else
    {
        if (!initSGJoinorPrune)
        {
            treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(
                                     node,
                                     grpAddr,
                                     srcAddr,
                                     ROUTING_PIMSM_G);
        }
    }

    if (treeInfoBaseRowPtr == NULL)
    {
#ifdef ADDON_DB
            HandleNetworkDBEvents(
                node,
                msg,
                incomingInterface,
                "NetworkPacketDrop",
                "Multicast Leaf Node, Does not Forward",
                0,
                0,
                0,
                0);

#endif
        *packetWasRouted = TRUE ;
        MESSAGE_Free(node, msg);

        pimDataSm->stats.numDataPktDiscard++;
        if (DEBUG_FORWARD_PACKET) {
            printf("\ntreeInfoBaseRowPtr NULL, returning discard"
                "%d \n",pimDataSm->stats.numDataPktDiscard);
        }
        return;
    }

    /*if (iif == RPF_interface(S) AND SPTbit(S,G) == TRUE ) {
            oiflist = inherited_olist(S,G)
        } else if (iif == RPF_interface(RP(G)) AND SPTbit(S,G) == FALSE) {
        oiflist = inherited_olist(S,G,rpt)
        CheckSwitchToSpt(S,G)*/
    ListInit(node, &outIntfList);

    if (DEBUG_FORWARD_PACKET)
    {
        printf("Node %u: has now outIntfList of size %d\n",
               node->nodeId, outIntfList->size);
        printf("Node %u: time to set outInterfaceList \n", node->nodeId);
        printf(" it has incoming Interface %u \n", incomingInterface);
        printf(" source interface %u \n", sourceInterface);
        printf(" RP Interface %u \n", RPInterface);
        printf("upstreamState %d \n",
               treeInfoBaseRowPtr->upstreamState);
    }

    if (DEBUG)
    {
        printf("For node %d FF: sameInt = %d, SPTbit = %d, "
                "upstreamState = %d\n",
               node->nodeId,
               sameInt,
               treeInfoBaseRowPtr->SPTbit,
               treeInfoBaseRowPtr->upstreamState);
        printf("source = %d, incoming = %d\n",
               sourceInterface, incomingInterface);
    }

    if ((incomingInterface == sourceInterface)
         && (treeInfoBaseRowPtr->SPTbit))
    {
        ListItem* tempListItem;
        if (DEBUG_FORWARD_PACKET) {
            printf(" Node %d receieve data on source Interface \n",
                   node->nodeId);
        }

        /* Time to make  oiflist = inherited_olist(S, G) */
        if (RoutingPimSmInheritedOutListSG
            (node, grpAddr, srcAddr, treeInfoBaseRowPtr)
            != 0)
        {
            tempListItem = treeInfoBaseRowPtr->inheritedOutIntfSG->first;

            while (tempListItem)
            {
                unsigned int* inheritedIntfPtr = (unsigned int*)
                                        MEM_malloc(sizeof(unsigned int));

                memcpy(inheritedIntfPtr, tempListItem->data, sizeof(int));

                if (DEBUG_FORWARD_PACKET) {
                    printf("Node %u: add intf %d from inheritedSGList \n",
                           node->nodeId,* inheritedIntfPtr);
                }

                if (IsInterfaceInThisPimSmList(outIntfList,
                                               * inheritedIntfPtr) == FALSE)
                {
                    ListInsert(node, outIntfList, 0,
                               (void*)inheritedIntfPtr);
                }
                else
                {
                    if (DEBUG_FORWARD_PACKET) {
                        printf("No need to add in this list"
                               "since already there \n");
                    }
                    MEM_free(inheritedIntfPtr);
                }
                tempListItem = tempListItem->next;
            }
        }
#ifdef ADDON_BOEINGFCS
        if (IAmDR &&
            sourceInterface == RPInterface &&
            //treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_NotJoin &&
            sptSwitchoverDataPacketsForGroup != NULL)
        {
            // in case RPintf is the same as sourceIntf
            // and this is the receiver's DR
            // check to switch to SPT
            BOOL sgJoinSent = FALSE;
            sgJoinSent = RoutingPimSmCheckSwitchToSpt(node, grpAddr, srcAddr,
                                         sptSwitchoverDataPacketsForGroup);
#ifdef CYBER_CORE
            if (sgJoinSent)
            {
                if (ip->iahepEnabled
                    && ip->iahepData->nodeType == RED_NODE
                    && IsIAHEPRedSecureInterface(node, incomingInterface))
                {
                    // and this is a red node & DR
                    // receive the first packet from G
                    // need to send a IGMP JOIN with (S,G) info to
                    // black side so black side can setup SPT for (S',G')
                    IgmpData* igmp = (IgmpData*)ip->igmpDataPtr;
                    int redIntf = IAHEPGetIAHEPRedInterfaceIndex(node);
                    if (redIntf >= 0)
                    {
                        if (igmp &&
                            (igmp->igmpInterfaceInfo[redIntf] != NULL) &&
                            (igmp->igmpInterfaceInfo[redIntf]->igmpRouter
                            != NULL))
                        {
                            RoutingPimSmIAHEPInitIgmpMsgToBlackNetwork(
                                node, srcAddr, grpAddr, IGMP_MEMBERSHIP_REPORT_MSG);
                        }
                    }
                }
            }
#endif
        }
#endif
    }
    else if ((RPInterface != NETWORK_UNREACHABLE) &&
            (incomingInterface == RPInterface) &&
            (treeInfoBaseRowPtr->SPTbit == FALSE))
    {
        ListItem* tempListItem;

        if (DEBUG_FORWARD_PACKET) {
            printf(" Node %d receieve data on RP_Interface \n",
                node->nodeId);
        }

        /*Time to make oiflist = inherited_olist(S, G, rpt)  */
        if (RoutingPimSmInheritedOutListSGrpt(node, grpAddr,
                    srcAddr, treeInfoBaseRowPtr) != 0)
        {
            tempListItem =
                treeInfoBaseRowPtr->inheritedOutIntfSGrpt->first;

            while (tempListItem)
            {
                unsigned int* inheritedIntfPtr = (unsigned int*)
                                    MEM_malloc(sizeof(unsigned int));

                memcpy(inheritedIntfPtr,
                       tempListItem->data, sizeof(int));

                if (DEBUG_FORWARD_PACKET) {
                    printf("Node %u: add intf %d from "
                           "inheritedSGrptList\n",
                           node->nodeId,* inheritedIntfPtr);
                }

                if (IsInterfaceInThisPimSmList(outIntfList,
                                         * inheritedIntfPtr) == FALSE)
                {
                    ListInsert(node, outIntfList, 0,
                               (void*) inheritedIntfPtr);
                }
                else
                {
                    if (DEBUG_FORWARD_PACKET) {
                        printf("No need to add in this list"
                               "since already there \n");
                    }
                    MEM_free(inheritedIntfPtr);
                }
                tempListItem = tempListItem->next;
            }
        }
        if (sptSwitchoverDataPacketsForGroup != NULL)
        {
            // check to switch to SPT
            BOOL sgJoinSent = FALSE;
            sgJoinSent = RoutingPimSmCheckSwitchToSpt(node, grpAddr, srcAddr,
                                         sptSwitchoverDataPacketsForGroup);
#ifdef CYBER_CORE
            if (sgJoinSent)
            {
                if (ip->iahepEnabled
                    && ip->iahepData->nodeType == RED_NODE && IAmDR
                    && IsIAHEPRedSecureInterface(node, incomingInterface))
                {
                    // and this is a red node & DR
                    // receive the first packet from G
                    // need to send a IGMP JOIN with (S,G) info to
                    // black side so black side can setup SPT for (S',G')
                    IgmpData* igmp = (IgmpData*)ip->igmpDataPtr;
                    int redIntf = IAHEPGetIAHEPRedInterfaceIndex(node);
                    if (redIntf >= 0)
                    {
                        if (igmp &&
                            (igmp->igmpInterfaceInfo[redIntf] != NULL) &&
                            (igmp->igmpInterfaceInfo[redIntf]->igmpRouter
                            != NULL))
                        {
                            RoutingPimSmIAHEPInitIgmpMsgToBlackNetwork(
                                node, srcAddr, grpAddr, IGMP_MEMBERSHIP_REPORT_MSG);
                        }
                    }
                }
            }
#endif
        }
    }
    /* Note: RPF check failed
    # A transition in an Assert FSM may cause an Assert(S,G)
    # or Assert(*,G) message to be sent out interface iif.
    # See section 4.6 for details.
    if (SPTbit(S,G) == TRUE AND iif is in inherited_olist(S,G) ) {
        send Assert(S,G) on iif
    } else if (SPTbit(S,G) == FALSE AND
        iif is in inherited_olist(S,G,rpt) {
            send Assert(*,G) on iif
    }*/
    else
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf(" Node %d receieve data not from source/RP "
                   "Interface \n",
                   node->nodeId);
        }

        /*Data Arrives From S To G On I And CouldAssert(GI) */
        if (RoutingPimSmCouldAssertSG(node, grpAddr, srcAddr,
                              incomingInterface))
        {
            if (DEBUG_FORWARD_PACKET)
            {
                printf(" Node %d inside RoutingPimSmCouldAssertSG \n",
                       node->nodeId);
            }
            RoutingPimSmDownstream* associatedDownstream = NULL;
            RoutingPimSmTreeInfoBaseRow
                *currentTreeInfoBaseRowPtr = NULL;
            /* it checks to see if S-G in its multicast treeInfo Base */

            if (treeInfoBaseRowPtr
                &&
                treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG)
            {
               currentTreeInfoBaseRowPtr = treeInfoBaseRowPtr;
            }
            else
            {
               currentTreeInfoBaseRowPtr =
                RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                                 grpAddr, srcAddr, ROUTING_PIMSM_SG);
            }

            if (currentTreeInfoBaseRowPtr == NULL)
            {
                ERROR_ReportError("SG_Pkt received so entry must "
                    "exist\n");
            }
            else /* check if the interface present in the
                 downstream list */
            {
                associatedDownstream = IsInterfaceInPimSmDownstreamList(
                                                node,
                                                currentTreeInfoBaseRowPtr,
                                                incomingInterface);
            }
            if ((associatedDownstream != NULL) &&
                (associatedDownstream->assertState == PimSm_Assert_NoInfo))
            {
                if (DEBUG_FORWARD_PACKET)
                {
                    printf("Node %u: interface not present in SG "
                        "Downstream \n",node->nodeId);
                    printf("Node %d: RoutingPimSmSetAssertState"
                           "(node, winner)\n",node->nodeId);
                    printf("Node %d: PerformsActionA1()\n",
                                node->nodeId);
                    printf("Node %u: move to winner state \n",
                                node->nodeId);
                }
                associatedDownstream->assertState =
                            PimSm_Assert_IWonAssert;

                RoutingPimSmPerformActionA1(node,
                                            grpAddr,
                                            srcAddr,
                                            incomingInterface,
                                            ROUTING_PIMSM_SG);
            }
        }
        /* Data Arrives From for G On I And CouldAssert(GI) */
        else if (RoutingPimSmCouldAssertG(node, grpAddr, srcAddr,
                                         incomingInterface))
        {
            if (DEBUG_FORWARD_PACKET)
            {
                printf(" Node %d inside RoutingPimSmCouldAssertG \n",
                       node->nodeId);
            }
            RoutingPimSmTreeInformationBase* treeInfoBase =
                &pimDataSm->treeInfoBase;
            RoutingPimSmTreeInfoBaseRow
                *currentTreeInfoBaseRowPtr = NULL;
            RoutingPimSmDownstream* associatedDownstream;

            TreeInfoRowMap* rowPtrMap = treeInfoBase->rowPtrMap;
            TreeInfoRowMapIterator mapIterator;
            RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;

            /*it checks for all group G entry in the treeInfo Base */
            /* Search treeInfo Base for the desired match */
            for (mapIterator = rowPtrMap->begin();
                 mapIterator != rowPtrMap->end();
                 mapIterator++)
            {
                rowPtr = mapIterator->second;
                if (DEBUG_ERRORS)
                {
                    printf("   checking the treeInfo Base entry %d\n", i);
                }

                /* check for group */
                if ((rowPtr->grpAddr == treeInfoBaseRowPtr->grpAddr)
                    &&
                    (rowPtr->treeState == treeInfoBaseRowPtr->treeState))
                {
                    currentTreeInfoBaseRowPtr = rowPtr;

                    if (currentTreeInfoBaseRowPtr != NULL)
                    {
                        associatedDownstream =
                            IsInterfaceInPimSmDownstreamList(node,
                                        currentTreeInfoBaseRowPtr,
                                        incomingInterface);
                        if ((associatedDownstream != NULL) &&
                            (associatedDownstream->assertState ==
                            PimSm_Assert_NoInfo))
                        {
                            /* this router won assert */
                            associatedDownstream->assertState
                            = PimSm_Assert_IWonAssert;
                            if (DEBUG_FORWARD_PACKET)
                            {
                                printf("Node %d: PerformsActionA1()\n",
                                            node->nodeId);
                                printf("Node %u: move to winner state \n",
                                            node->nodeId);
                            }
                            RoutingPimSmPerformActionA1(node,
                                                        grpAddr,
                                                        srcAddr,
                                                        incomingInterface,
                                                        ROUTING_PIMSM_G);
                        }
                    }
                }
            }   //end of for;
        }    //if could assert G true;
    }
    /*Time to create the fresh out Interface List for forwarding packet*/
    /* RoutingPimSm Forward packet on all interfaces in oiflist */
    /* oiflist = oiflist - iif
        forward packet on all interfaces in oiflist*/
    if (DEBUG_FORWARD_PACKET) {
        printf("originateByMe = %d \n", originateByMe);
        printf("Node %u: has final outIntfList of size %d\n",
               node->nodeId, outIntfList->size);
        RoutingPimSmPrintTreeInfoBase(node);
    }
    BOOL packetForward = FALSE;
    if (outIntfList->size == 0 && registerSent == FALSE && forward == 0)
    {
        if (DEBUG_FORWARD_PACKET) {
            printf("Node %u: does not forward this pkt \n",
                   node->nodeId);
        }
#ifdef ADDON_DB
#ifdef ADDON_BOEINGFCS

        if (pim->interface[incomingInterface].interfaceType ==
                MULTICAST_CES_RPIM_INTERFACE)
        {
            HandleNetworkDBEvents(
                node,
                msg,
                incomingInterface,
                "NetworkPacketDrop",
                "No Route, RPIM-SM",
                0,
                0,
                0,
                0);
        }
        else
#endif
        {
            HandleNetworkDBEvents(
                node,
                msg,
                incomingInterface,
                "NetworkPacketDrop",
                "No Route, PIM-SM",
                0,
                0,
                0,
                0);
        }
#endif
        pimDataSm->stats.numDataPktDiscard++;
        if (DEBUG_FORWARD_PACKET)
        {
            char clockStr[100];
            TIME_PrintClockInSecond(node->getNodeTime(),
                            clockStr, node);
            printf("\n Time %s : ",clockStr);
            printf("Node %d is discarding packet as no outgoing interface"
                 " present.,numDataPktDiscard %d", node->nodeId,
                 pimDataSm->stats.numDataPktDiscard);
        }
#ifdef ADDON_DB
        if (!originateByMe &&
            !(NetworkIpIsPartOfMulticastGroup(node, ipHeader->ip_dst)))
        {
            pim->pimMulticastNetSummaryStats.m_NumDataDiscarded++;
        }
#endif
    }
    else
#ifdef CYBER_CORE
    if (!(ip->iahepEnabled &&
        ip->iahepData->nodeType == BLACK_NODE &&
        registerSent == TRUE))
     // in case the packet has been sent via register packet in blakc network, do not
     // forward again
#endif
    {
        /* Forward Packet */
        listItem = outIntfList->first;
        if (DEBUG_FORWARD_PACKET)
        {
            printf("outIntfList size = %d\n", outIntfList->size);
        }

        while (listItem)
        {
            unsigned int* inheritedIntfPtr = (unsigned int*)listItem->data;
            int inheritedIntf =(int)(*inheritedIntfPtr);

            if (DEBUG)
            {
                printf("\tintf = %d\n", inheritedIntf);
            }

            /* Remove Incoming Interface From outIntfList */
            BOOL sameInt = TRUE;

            /* The packet is originated by this node and has been forwarded
             * to the DR on the LAN.The check also excludes
             * the incoming interface from the list of oif entries*/
            BOOL cond1 = TRUE;
            cond1 = ((originateByMe) && (inheritedIntf != originaliif) &&
                    (forward != 0));

            /* The packet is originated by this node and has not been
             * forwarded so far on any oif list.This is possible only when
             * DR originates the pkt*/
            BOOL cond2 = TRUE;
            cond2 = ((originateByMe) && (forward == 0));

            /* The packet is not originated by this node and outgoing
             * int is not same as the incoming int(as the incoming int
             * should always be excluded from the oif list).
             */
            BOOL cond3 = TRUE;
            cond3 = ((!originateByMe) &&
                    (inheritedIntf != incomingInterface));

            BOOL cond4 = FALSE;
#ifdef ADDON_BOEINGFCS
            // This condition is required to allow the RP to forward packets
            // back out on the incoming interface, for RPIM, to support the cases:
            // (i) When the source is sending out packets faster than it
            //     can receive and handle S,G joins from the receivers, or,
            //     if the receivers do not send out S,G joins due to the
            //     SPT Switch threshold being high
            // (ii)When the receiver joins the group after the RP has joined
            //     the S,G group and as a result may only start receiving packets
            //     from the RP, despite the RP being on the SPT tree
            //  In cases when neither of these conditions are satisified,
            //  the multicast mesh will drop the packet due to non-existence
            //  of mesh owing to no downstream receivers.
            cond4 = ((MulticastCesRpimActiveOnInterface(node, inheritedIntf)
                      &&(IAmRP)) &&
                     (treeInfoBaseRowPtr != NULL) &&
                     (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG) &&
                     (treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join));
#endif

            if (cond1 || cond2 || cond3 || cond4)
            {
                sameInt = FALSE;
            }

            if (DEBUG_FORWARD_PACKET)
            {
                printf("Node %d: RP=%d, sameInt=%d, originateByMe=%d "
                       "incoming=%d inherited=%d\n",
                       node->nodeId,
                       IAmRP,
                       sameInt,
                       originateByMe,
                       incomingInterface,
                       inheritedIntf);
            }

            if (!sameInt)
            {
                NodeAddress nextNode =
                    NetworkIpGetInterfaceBroadcastAddress(node,
                                                          inheritedIntf);
                if (DEBUG_FORWARD_PACKET)
                {
                    printf("Node %u: Forward packet through intf %d\n",
                           node->nodeId, inheritedIntf);
                    printf("    send packet to %x \n", nextNode);
                }

                Message* tempMsg = MESSAGE_Duplicate(node, msg);

                ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

                NetworkIpSendPacketToMacLayer(
                    node,
                    tempMsg,
                    inheritedIntf,
                    nextNode);

                if (DEBUG || DEBUG_FORWARD_PACKET)
                {
                    char clockStr[100];
                    TIME_PrintClockInSecond(node->getNodeTime(),
                            clockStr, node);
                    printf("\n Time %s : ",clockStr);
                    printf("Node %d is forwarding packet through interface "
                           " %d", node->nodeId, inheritedIntf);
                }
                fwdTableRowPtr->pktRouted = TRUE;
                fwdTableRowPtr->inInterface = incomingInterface;
                packetForward = TRUE;
            }
            else
            {
                if (DEBUG_FORWARD_PACKET) {
                    printf("Node %u: Not forwarding packet through "
                           "interface %d\n",
                           node->nodeId, inheritedIntf);
                }
            }

            listItem = listItem->next;
        }
        if (packetForward)
        {
            pimDataSm->stats.numDataPktForward++;
#ifdef ADDON_DB
            if (!originateByMe)
            {
                pim->pimMulticastNetSummaryStats.m_NumDataForwarded++;
            }
            else
            {
                if (isloopbackPacket)
                {
                    pim->pimMulticastNetSummaryStats.m_NumDataForwarded++;
                }
            }
#endif
        }
        else if (!packetForward && registerSent == FALSE && forward == 0)
        {
            pimDataSm->stats.numDataPktDiscard++;
            if (DEBUG_FORWARD_PACKET)
            {
                char clockStr[100];
                TIME_PrintClockInSecond(node->getNodeTime(),
                            clockStr, node);
                printf("\n Time %s : ",clockStr);
                printf("Node %d is discarding packet as incoming interface "
                           " is the onlu outgoing interface "
                           " discard %d", node->nodeId,
                           pimDataSm->stats.numDataPktDiscard);
            }
#ifdef ADDON_DB
#ifdef ADDON_BOEINGFCS

        if (pim->interface[incomingInterface].interfaceType ==
                MULTICAST_CES_RPIM_INTERFACE)
        {
            HandleNetworkDBEvents(
                node,
                msg,
                incomingInterface,
                "NetworkPacketDrop",
                "No Route, RPIM-SM",
                0,
                0,
                0,
                0);
        }
        else
#endif
        {
            HandleNetworkDBEvents(
                node,
                msg,
                incomingInterface,
                "NetworkPacketDrop",
                "No Route, PIM-SM",
                0,
                0,
                0,
                0);
        }
#endif
        }
    }
    if (DEBUG_ERRORS) {
        RoutingPimSmPrintForwardingTable(node);
    }

    *packetWasRouted = TRUE;
    MESSAGE_Free(node, msg);
    ListFree(node, outIntfList, FALSE);
}
/*
*  FUNCTION     :RoutingPimSmDeleteNeighbor()
*  PURPOSE      :Delete neighbor information
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimSmDeleteNeighbor(Node* node, NodeAddress nbrAddr,
        int interfaceId)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimSmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimNeighborListItem* neighbor;
    RoutingPimInterface* thisInterface;
    ListItem* listItem;

    thisInterface = &pim->interface[interfaceId];
    listItem = thisInterface->smInterface->neighborList->first;

    if (!listItem)
    {
        return;
    }
    while (listItem)
    {
        neighbor = (RoutingPimNeighborListItem*) listItem->data;

        if (neighbor->ipAddr == nbrAddr)
        {
            /*  Got the neighbor, so delete it */
            ListGet(
                node,
                thisInterface->smInterface->neighborList,
                listItem,
                TRUE,
                FALSE);

            thisInterface->smInterface->neighborCount--;
            pimSmData->stats.numNeighbor--;

            if (DEBUG)
            {
                fprintf(stdout, "Node %d remove neighbor %x from "
                                "neighbor list\n",
                                node->nodeId, nbrAddr);
            }
            if (DEBUG)
            {
                RoutingPimSmPrintNghbrList(node);
            }

            break;
        }
        listItem = listItem->next;
    }
    /*  Select a new DR if this neighbor was preious DR */
    if (thisInterface->smInterface->drInfo.ipAddr == nbrAddr)
    {
        /*  Elect new DR */
        RoutingPimSmElectDR(node, interfaceId);

        if (DEBUG)
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
            fprintf(stdout, "Node %d says new DR on interface %d is %x "
                            "at time %s\n",
                            node->nodeId, interfaceId,
                            thisInterface->smInterface->drInfo.ipAddr,
                            clockStr);
        }

    }
}
/*
*  FUNCTION     :RoutingPimSmHandleNeighborTimeoutEvent()
*  PURPOSE      :Handle NLT expiry
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimSmHandleNeighborTimeoutEvent(Node* node, NodeAddress nbrAddr,
        int interfaceId)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmTreeInformationBase* treeInfoBase =
                                            &pimDataSm->treeInfoBase;

    TreeInfoRowMap* rowPtrMap = treeInfoBase->rowPtrMap;
    TreeInfoRowMapIterator mapIterator;

    RoutingPimSmTreeInfoBaseRow * rowPtr = NULL;
    RoutingPimSmDownstream* downStreamListPtr = NULL;
    if (DEBUG)
    {
        printf("Node %d: Neighbor %x says it's going down "
               "as NLT expires\n",
               node->nodeId, nbrAddr);
    }
    /* Delete Pim Neighbor from list */
    RoutingPimSmDeleteNeighbor(node, nbrAddr,
                        interfaceId);

    /* Handle (S,G) asser state machine */
    /*The Neighbor Liveness Timer associated with the current
    winner expires or we receive a Hello message from the current
    winner reporting a different GenID from the one it previously
    reported. This indicates that the current winner’s interface
    or router has gone down (and may have come back up), and so
    we must assume it no longer knows it was the winner. We
    transition to the NoInfo state,deleting this (S,G) assert
    information (Actions A5 below).*/

    for (mapIterator = rowPtrMap->begin();
         mapIterator != rowPtrMap->end();
         mapIterator++)
    {
        rowPtr = mapIterator->second;
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(
                                node,
                                rowPtr,
                                interfaceId);
        if (downStreamListPtr && downStreamListPtr->assertWinner ==
                nbrAddr &&
                downStreamListPtr->assertState ==
                PimSm_Assert_ILostAssert)
        {
            downStreamListPtr->assertState =
                            PimSm_Assert_NoInfo;
            RoutingPimSmPerformActionA5(node,
                            rowPtr->grpAddr,
                            rowPtr->srcAddr,
                            interfaceId,
                            rowPtr->treeState);
            break;
        }
    }

}

/*
*  FUNCTION    : RoutingPimSmHandleHelloPacket()
*  PURPOSE     : Process received hello packet
*  ASSUMPTION  : None
*  RETURN VALUE: NULL
*/
void
RoutingPimSmHandleHelloPacket(Node* node, NodeAddress srcAddr,
                              RoutingPimHelloPacket* helloPkt,
                              unsigned int pktSize, int interfaceIndex)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    RoutingPimInterface* thisInterface;
    RoutingPimNeighborListItem* neighbor = NULL;
    unsigned short holdTime;
    unsigned  int genId = 0, drPriority = PIM_DR_PRIORITY;
    RoutingPimSmNbrTimerInfo timerInfo;
    BOOL isDRPrioritypresent = FALSE;

    if (!RoutingPimProcessHelloPacket(helloPkt, pktSize, &holdTime, &genId,
                                      &drPriority, &isDRPrioritypresent))
    {
        /* There should be some error in processing option */
        return;
    }


#ifdef ADDON_BOEINGFCS
    if (interfaceIndex < 0)
    {
        return;
    }
#endif
    thisInterface = &pim->interface[interfaceIndex];

    /* Search neighbor in neighbor list */
    if (thisInterface->smInterface->neighborList->size > 0) {
        RoutingPimSearchNeighborList(thisInterface->smInterface->neighborList,
                                     srcAddr,
                                     &neighbor);
    }

    /* If we allready have an entry for this neighbor */
    if (neighbor)
    {
        if (holdTime == 0)
        {
            if (DEBUG)
            {
                printf("Node %d: Neighbor %x says it's going down "
                       "by sending \"holdtime = 0\"\n",
                       node->nodeId, srcAddr);
            }

            /*
             *  Neighbor says it's going down by sending "holdtime = 0"
             *  Remove this neighbor from neighbor list
             */
            RoutingPimSmDeleteNeighbor(node, srcAddr, interfaceIndex);
            return;
        }
        BOOL start_dr_election = FALSE;
        if (neighbor->lastGenIdReceive != genId)
        {
            if (DEBUG_JP)
            {
                printf("Node %d observe neighbor %x has been "
                       "restarted\n",
                       node->nodeId, neighbor->ipAddr);
            }
            Message* newMsg;
            clocktype delay ;
            //  Send a PIM Hello message to let it know this routers
            // existence
            // This should happen between 0 and triggered_hello_delay(5 s)
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
            start_dr_election = TRUE;

            // /* The Generation ID of the router that is RPF’(*,G) changes
            /* The upstream (*,G) state machine remains in Joined state.
            If the Join Timer is set to expire in more than t_override
            seconds, reset it so that it expires after t_override seconds.*/
            RoutingPimSmTreeInformationBase* treeInfoBase =
                                            &pimDataSm->treeInfoBase;
            RoutingPimSmTreeInfoBaseRow * rowPtr = NULL;
            RoutingPimSmJoinPruneTimerInfo joinTimer;
            RoutingPimSmDownstream* downStreamListPtr = NULL;

            TreeInfoRowMap* rowPtrMap = treeInfoBase->rowPtrMap;
            TreeInfoRowMapIterator mapIterator;

            for (mapIterator = rowPtrMap->begin();
                 mapIterator != rowPtrMap->end();
                 mapIterator++)
            {
                if (DEBUG_ERRORS)
                {
                    printf(" checking the treeInfo Base entry \n");
                }

                rowPtr = mapIterator->second;

                if ((rowPtr->treeState == ROUTING_PIMSM_G ||
                     rowPtr->treeState == ROUTING_PIMSM_SG )
                     &&
                    rowPtr->nextHopForRP == neighbor->ipAddr)
                {
                    /* check for multicast tree state */
                    if (rowPtr->upstreamState == PimSm_JoinPrune_Join)
                    {
                        /* The upstream (*,G) state machine remains in Joined
                        state. If the Join Timer is set to expire in more
                        than t_override seconds, reset it so that it expires
                        after t_override seconds.*/
                        delay = (clocktype)(ROUTING_PIMSM_OVERRIDE_INTERVAL
                                    *  RANDOM_erand(pimDataSm->seed));
                        if ((node->getNodeTime() + delay) <
                                    rowPtr->lastJoinTimerEnd)
                         {
                            joinTimer.srcAddr = rowPtr->srcAddr;
                            joinTimer.grpAddr = rowPtr->grpAddr;
                            joinTimer.intfIndex = (unsigned int)interfaceIndex;
                            joinTimer.seqNo = rowPtr->joinTimerSeq + 1;
                            joinTimer.treeState =
                                    rowPtr->treeState ;
                            /* note the latest join timer Seq no */
                            rowPtr->joinTimerSeq++;
                            rowPtr->lastJoinTimerEnd = node->getNodeTime() +
                                                       delay;

                            RoutingPimSetTimer(node,
                                   interfaceIndex,
                                   MSG_ROUTING_PimSmJoinTimerTimeout,
                                   (void*) &joinTimer, delay);
                        }
                    }
                    /* Call assert state/mc*/
                    /*The Neighbor Liveness Timer associated with the current
                    winner expires or we receive a Hello message from the
                    current winner reporting a different GenID from the one
                    it previously reported. This indicates that the current
                    winner’s interface or router has gone down
                    (and may have come back up), and so we must assume it no
                    longer knows it was the winner.
                    We transition to the NoInfo state, deleting this (S,G)
                    assert information (Actions A5 below).*/

                    downStreamListPtr = IsInterfaceInPimSmDownstreamList(
                                            node,
                                            rowPtr,
                                            interfaceIndex);
                    if (downStreamListPtr &&
                        downStreamListPtr->assertWinner ==
                        neighbor->ipAddr &&
                        downStreamListPtr->assertState ==
                        PimSm_Assert_ILostAssert)
                    {
                        downStreamListPtr->assertState =
                                        PimSm_Assert_NoInfo;
                        RoutingPimSmPerformActionA5(node,
                                        rowPtr->grpAddr,
                                        rowPtr->srcAddr,
                                        interfaceIndex,
                                        rowPtr->treeState);
                    }
                }
            }
        }

        /* Else, refresh neighbor timeout timer */
        neighbor->ipAddr = srcAddr;
        neighbor->lastGenIdReceive = genId;
        neighbor->lastHelloReceive = node->getNodeTime();
        neighbor->lastDRPriorityReceive = drPriority;
        neighbor->isDRPrioritypresent = isDRPrioritypresent;
        neighbor->holdTime = holdTime;

        if (neighbor->NLTTimer == 0)
        {
            /*  Send neighbor expiry timer to self for this neighbor */
            timerInfo.nbrAddr = srcAddr;
            timerInfo.interfaceIndex = interfaceIndex;

            if (holdTime != 0xFFFF)
            {
                RoutingPimSetTimer(node,
                interfaceIndex,
                MSG_ROUTING_PimSmNeighborTimeOut,
                &timerInfo,
                (clocktype) holdTime*SECOND);
            }
        }
        if (holdTime != 0xFFFF)
        {
            neighbor->NLTTimer = node->getNodeTime() +
                            (clocktype) holdTime*SECOND;
        }
        if ((!start_dr_election) && (isDRPrioritypresent) &&
            (neighbor->lastDRPriorityReceive != drPriority))
        {
            start_dr_election = TRUE;
        }
        if (start_dr_election)
        {
            /* Elect new DR on all interfaces, LAN or otherwise*/
            RoutingPimSmElectDR(node, interfaceIndex);
        }
        return;
    }
    else // new neighbor
    {
        if ((holdTime==0))
        {
            if (DEBUG)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("Node %d found new neighbor %x on interface %d "
                   "at time %s with holdtime 0. Hence a NoOp and return.\n",
                   node->nodeId, srcAddr, interfaceIndex, clockStr);
            }
            return ;
        }
        /* Neighbor doesn't have an entry, so build one */
        if (DEBUG)
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %d found new neighbor %x on interface %d "
                   "at time %s\n",
                   node->nodeId, srcAddr, interfaceIndex, clockStr);
        }

        /* Allocate space for new neighbor */
        neighbor = (RoutingPimNeighborListItem*)
                   MEM_malloc(sizeof(RoutingPimNeighborListItem));

        neighbor->ipAddr = srcAddr;
        neighbor->lastGenIdReceive = genId;
        neighbor->lastHelloReceive = node->getNodeTime();
        neighbor->lastDRPriorityReceive = drPriority;
        neighbor->isDRPrioritypresent = isDRPrioritypresent;
        neighbor->NLTTimer = 0;
        /* Add to neighbor list */
        ListInsert(node,
            thisInterface->smInterface->neighborList,
                   node->getNodeTime(),
                   (void*) neighbor);

        /* Since we've found a neighbor, this network is not Leaf */
        thisInterface->smInterface->neighborCount++;
        pimDataSm->stats.numNeighbor++;

            /* Elect new DR on all interfaces, LAN or otherwise*/
        RoutingPimSmElectDR(node, interfaceIndex);
        if (DEBUG)
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %d says new DR on interface %d is %x "
                   "at time %s\n",
                   node->nodeId, interfaceIndex,
                   thisInterface->smInterface->drInfo.ipAddr, clockStr);
        }
        if (DEBUG)
        {
            RoutingPimSmPrintNghbrList(node);
        }

        Message* newMsg;
        clocktype delay ;
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

    /*  Send neighbor expiry timer to self for this neighbor */
    timerInfo.nbrAddr = srcAddr;
    timerInfo.interfaceIndex = interfaceIndex;
    if (holdTime != 0xFFFF)
    {
        RoutingPimSetTimer(node,
            interfaceIndex,
            MSG_ROUTING_PimSmNeighborTimeOut,
            &timerInfo,
            (clocktype) holdTime*SECOND);
        neighbor->NLTTimer = node->getNodeTime() +
                            (clocktype) holdTime*SECOND;

    }
}
/*
*  FUNCTION    : RoutingPimSmDRIsBetter()
*  PURPOSE     : Elect new DR over this interface
*  ASSUMPTION  : All PIM Neighbor is capable in participating DR election
*  RETURN VALUE: NULL
*/
bool
RoutingPimSmDRIsBetter(RoutingPimNeighborListItem* tempNbInfo,
                       RoutingPimInterface* dr_interface,
                       BOOL dr_priority_present)
{
    if (dr_priority_present == FALSE )
    {
        if (tempNbInfo->ipAddr > dr_interface->smInterface->drInfo.ipAddr)
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
        if (tempNbInfo->lastDRPriorityReceive >
                dr_interface->smInterface->drInfo.lastDRPriorityReceive )
        {
            return TRUE;
        }
        else
        {
            if ((tempNbInfo->lastDRPriorityReceive ==
                 dr_interface->smInterface->drInfo.lastDRPriorityReceive ) &&
                (tempNbInfo->ipAddr >
                dr_interface->smInterface->drInfo.ipAddr ))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }
}


/*
*  FUNCTION    : RoutingPimSmElectDR()
*  PURPOSE     : Elect new DR over this interface
*  ASSUMPTION  : All PIM Neighbor is capable in participating DR election
*  RETURN VALUE: NULL
*/
void
RoutingPimSmElectDR(Node* node, int interfaceIndex)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];
    ListItem* tempNbItem = thisInterface->smInterface->neighborList->first;

    thisInterface->smInterface->drInfo.ipAddr = thisInterface->ipAddress;
    thisInterface->smInterface->drInfo.lastDRPriorityReceive =
        thisInterface->smInterface->DRPriority;

    BOOL dr_priority_present = TRUE;

    while (tempNbItem)
    {
        RoutingPimNeighborListItem* tempNbInfo;

        tempNbInfo = (RoutingPimNeighborListItem*) tempNbItem->data;
        if (!tempNbInfo->isDRPrioritypresent)
        {
            dr_priority_present = FALSE;
            break;
        }
        tempNbItem = tempNbItem->next;
    }
    tempNbItem = thisInterface->smInterface->neighborList->first;
    while (tempNbItem)
    {
        RoutingPimNeighborListItem* tempNbInfo;

        tempNbInfo = (RoutingPimNeighborListItem*) tempNbItem->data;
        if (RoutingPimSmDRIsBetter( tempNbInfo,
                                     thisInterface,
                                     dr_priority_present) == TRUE )
        {
            thisInterface->smInterface->drInfo = *tempNbInfo;
        }
        tempNbItem = tempNbItem->next;
    }

}


/*
*  FUNCTION    : RoutingPimSmCheckDirectlyConnectedSource()
*  PURPOSE     : Routing PimSm Check if Source is on any subnet that is
*                directly connected to this router
*               (or the originator of the multicast packets).
*  RETURN VALUE: If Found TRUE
*/

BOOL
RoutingPimSmCheckDirectlyConnectedSource(Node* node,
        NodeAddress srcAddr, int interfaceId)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    NodeAddress netAddr;
    NodeAddress subnetMask;
    NodeAddress maskedSrcAddress;
    int i;
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    if (CPU_INTERFACE == interfaceId)
    {
        if (DEBUG) {
            printf(" Node %u: I am the source \n", node->nodeId);
        }
        return TRUE;
    }
    /* check if I am the source i.e If packet is from transport layer, */
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (srcAddr == pim->interface[interfaceId].ipAddress)
        {
            if (DEBUG) {
                printf(" Node %u: I am the source \n", node->nodeId);
            }
            return TRUE;
        }
    }   //end of for;

    /* Get incoming interface index */
    netAddr = NetworkIpGetInterfaceNetworkAddress(node, interfaceId);
    subnetMask = NetworkIpGetInterfaceSubnetMask(node, interfaceId);
    maskedSrcAddress = MaskIpAddress(srcAddr, subnetMask);

    if (DEBUG)
    {
        printf(" netAddr %x \n", netAddr);
        printf("subnetMask %u \n", subnetMask);
        printf(" maskedSrcAddress %u \n", maskedSrcAddress);
    }

    /* then check if I am directly connected to source */
    if (netAddr == maskedSrcAddress)
    {
/*
#ifdef ADDON_BOEINGFCS
        BOOL retVal = FALSE;
        retVal = MulticastCesRpimIsDirectlyConnected(node,
                srcAddr, interfaceId);

        if (retVal && DEBUG)
        {
            printf(" Node %u: I am directly connected to source %x \n",
                   node->nodeId, srcAddr);
        }

        return retVal;
#endif
*/
        if (DEBUG)
        {
            printf(" Node %u: I am directly connected to source %x \n",
                   node->nodeId, srcAddr);
        }
        return TRUE;
    }
/*Red node can be place in different subnets, if we dont have this, they will
never be connected*/
#ifdef CYBER_CORE
    else if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE
            && !IsIAHEPRedSecureInterface(node, interfaceId))
    {
        if (DEBUG)
        {
            printf(" Node %u: I am directly connected to source %x \n",
                   node->nodeId, srcAddr);
        }
        return TRUE;
    }
#endif
    else
    {
        if (DEBUG) {
            printf(" Node %u: I am not directly connected to source %x \n",
                   node->nodeId, srcAddr);
        }
        return FALSE;
    }
}

/*
*  FUNCTION    : RoutingPimSmCouldRegisterSG()
*  PURPOSE     : Check if router can Register a mcast packet
*  RETURN VALUE: TRUE If condition satisfies;
*/
BOOL
RoutingPimSmCouldRegisterSG(Node* node, NodeAddress srcAddr,
                            NodeAddress grpAddr, int incomingInterface)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    int i = incomingInterface;
    RoutingPimInterface* thisInterface;
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr;
    BOOL isDrOnIif = FALSE;

    if (DEBUG_ERRORS)
    {
        printf(" Node %u: check for inInterface %d \n",
               node->nodeId, i);
    }

    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                         (node, grpAddr, srcAddr, ROUTING_PIMSM_SG);

    if (treeInfoBaseRowPtr == NULL)
    {
        treeInfoBaseRowPtr = RoutingPimSmSetMulticastTreeInfoBase
                             (node, grpAddr, srcAddr, ROUTING_PIMSM_SG);
    }

    /* check if I am the DR on RPF_interface(S) */
    /* check if the keepAlive Timer Running */
    /* if I am directly connected with source */
    if (incomingInterface == CPU_INTERFACE)
    {
        isDrOnIif = TRUE;
    }
    else
    {
        thisInterface = &pim->interface[i];
    if (
#ifdef ADDON_BOEINGFCS
        ((MulticastCesRpimActiveOnInterface(node, i))
            && (thisInterface->ipAddress == srcAddr)) ||
         (!MulticastCesRpimActiveOnInterface(node, i) &&
#endif
        (thisInterface->ipAddress ==
        thisInterface->smInterface->drInfo.ipAddr))
#ifdef ADDON_BOEINGFCS
       )
#endif
        {
            isDrOnIif = TRUE;
        }
    }
    if (isDrOnIif
        && treeInfoBaseRowPtr->keepAliveExpiryTimer
        && (RoutingPimSmCheckDirectlyConnectedSource(node, srcAddr, i)))
    {
        if (DEBUG) {
            printf("Node %u:PimSm Could Register is TRUE\n",
                   node->nodeId);
        }
        return TRUE;
    }

    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmReadCandidateRPparameters()
*  PURPOSE     : Routing PIMSM read Candidate RP parameters
*  RETURN VALUE: NULL
*/
void
RoutingPimSmReadCandidateRPparameters(Node* node,
                                      int interfaceIndex,
                                      const NodeInput* nodeInput)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];

    char buf[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;

    //Read other parameters for candidate RP
    int rpPriority = 0;
    IO_ReadInt(node->nodeId,
                  NetworkIpGetInterfaceAddress(node,interfaceIndex),
                  nodeInput,
                  "PIM-SM-CANDIDATE-RP-PRIORITY",
                  &retVal,
                  &rpPriority);

    if (retVal)
    {
        if ((0 <= rpPriority) && (rpPriority <= ROUTING_PIM_MAX_RP_PRIORITY))
        {
            thisInterface->rpData.rpPriority = (unsigned char)rpPriority;

            if (DEBUG_BS)
            {
                printf("\nNode %u is Candidate-RP with priority %d \n",
                        node->nodeId,thisInterface->rpData.rpPriority);
                printf("It's address is 0x%x\n",
                NetworkIpGetInterfaceAddress(node, interfaceIndex));
            }
        }
        else
        {
            ERROR_ReportError("RP-PRIORITY should lie in the range 0-255\n");
        }
    }
    else
    {
        thisInterface->rpData.rpPriority = (unsigned char)
                                           ROUTING_PIM_CANDIDATE_RP_PRIORITY;
    }

    retVal = FALSE;
    IO_ReadString(node->nodeId,
                  NetworkIpGetInterfaceAddress(node,interfaceIndex),
                  nodeInput,
                  "PIM-SM-RP-GROUP-RANGE",
                  &retVal,
                  buf);

    if (retVal)
    {
        thisInterface->rpData.rpAccessList =
            RoutingPimSmParseGroupRangeStr(node,
                                         buf,
                                         thisInterface->rpData.rpAccessList);
    }
    else
    {
        //supporting the range 224.0.0.0 with grpPrefix 4
        RoutingPimSmRPAccessList* accessListPtr =
            (RoutingPimSmRPAccessList*)
            MEM_malloc(sizeof(RoutingPimSmRPAccessList));

        accessListPtr->groupAddr = 0xE0000000;/*224.0.0.0*/
        accessListPtr->groupMask = 0x0FFFFFFF;/*15.255.255.255*/
        ListInsert(node,
                   thisInterface->rpData.rpAccessList,
                   (clocktype)0,
                   (void*)accessListPtr);
    }

    if (DEBUG_BS)
    {
       RoutingPimSmPrintRPGroupAccessList(node,
                                          interfaceIndex);
    }
}

LinkedList*
RoutingPimSmParseGroupRangeStr(Node* node,
                               char* groupRangeStr,
                               LinkedList* rpAccessList)
{
    if (rpAccessList == NULL)
    {
        ListInit(node, &rpAccessList);
    }

    char* tmpBuf = NULL;
    char* tmpStr1 = NULL;
    char* tmpStr2 = NULL;

    char grpAddrStr[MAX_STRING_LENGTH];
    char grpMaskStr[MAX_STRING_LENGTH];

    int isNodeId = 0;
    int numHostBits = 0;
    int qualifierLen = 0;

    if ((groupRangeStr[0] == '\0') || (groupRangeStr[0] != '<'))
    {
       ERROR_ReportError("\nPIM-SM-RP-GROUP-RANGE is a Bad String!!!\n");
    }
    else
    {
        RoutingPimSmRPAccessList* accessListPtr = NULL;

        tmpBuf = groupRangeStr;
        while (tmpBuf)
        {
            accessListPtr = (RoutingPimSmRPAccessList*)MEM_malloc(
                            sizeof(RoutingPimSmRPAccessList));

            tmpBuf++;//skipping '<'

            // skip white space
            while (isspace(*tmpBuf))
            {
                tmpBuf++;
            }

            tmpStr1 = strchr(tmpBuf, ',');
            if (tmpStr1 == NULL)
            {
                ERROR_ReportError("\n\nPIM-SM-RP-GROUP-RANGE : "
                    "The specified Group Range is a Bad string.\n");
            }

            qualifierLen = tmpStr1 - tmpBuf;
            strncpy(grpAddrStr, &tmpBuf[0],qualifierLen);
            grpAddrStr[qualifierLen] = '\0';

            IO_TrimLeft(grpAddrStr);
            IO_TrimRight(grpAddrStr);

            IO_ParseNodeIdHostOrNetworkAddress(grpAddrStr,
                                               &accessListPtr->groupAddr,
                                               &numHostBits,
                                               &isNodeId);

            if (NetworkIpIsMulticastAddress(node,
                                         accessListPtr->groupAddr) == FALSE)
            {
                ERROR_ReportError("\n\nPIM-SM-RP-GROUP-RANGE : "
                    "The specified Group Range is a Bad string.\n");
            }

            tmpStr1++;
            // skip white space
            while (isspace(*tmpStr1))
            {
                tmpStr1++;
            }

            tmpStr2 = strchr(tmpBuf, '>');
            if (tmpStr2 == NULL)
            {
                ERROR_ReportError("\n\nPIM-SM-RP-GROUP-RANGE : "
                    "The specified Group Range is a Bad string.\n");
            }

            qualifierLen = tmpStr2 - tmpStr1;
            strncpy(grpMaskStr, &tmpStr1[0],qualifierLen);
            grpMaskStr[qualifierLen] = '\0';

            IO_TrimLeft(grpMaskStr);
            IO_TrimRight(grpMaskStr);

            IO_ParseNodeIdHostOrNetworkAddress(grpMaskStr,
                                               &accessListPtr->groupMask,
                                               &numHostBits,
                                               &isNodeId);

            if (RoutingPimCheckInverseSubnetMask(accessListPtr->groupMask)
                == FALSE)
            {
                ERROR_ReportError("\nPIM-SM : The specified inverse mask in"
                                    " access-list is Invalid.\n");
            }

            //check whether the given group-address and group mask are
            //compatible or not

            if (ConvertSubnetMaskToNumHostBits(accessListPtr->groupAddr)
                < ConvertSubnetMaskToNumHostBits(~accessListPtr->groupMask))
            {
                 ERROR_ReportError("\n\nPIM-SM : The specified mask "
                                    "parameter is Invalid.\n"
                                    "(GroupAddress & GroupMask) != "
                                    "GroupAddress.\n");
            }

            ListInsert(node,
                       rpAccessList,
                      (clocktype)0,
                      (void*)accessListPtr);

            tmpBuf = strchr(tmpBuf, '<');
        }//end of while
    }//end of else

    return rpAccessList;
}

BOOL
RoutingPimSmCheckForStaticRP(Node* node,
                             const NodeInput* nodeInput)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    BOOL isStaticRPPresent = FALSE;

    char parameterValue[MAX_STRING_LENGTH];

    BOOL numStaticRPFound = FALSE;

    int i = 0;
    int j = 0;
    int numHostBits = 0;
    int isNodeId = 0;
    int numStaticRP = 0;
    NodeAddress RPAddr = (unsigned int) NETWORK_UNREACHABLE;
    RoutingPimSmRPList* RPListPtr = NULL;
    LinkedList* rpAccessList = NULL;
    RoutingPimSmRPAccessList* accessListPtr = NULL;
    ListItem* rpAccessListItem = NULL;
    BOOL processed = FALSE;
    BOOL found = FALSE;

    IO_ReadInt(node->nodeId,
               ANY_ADDRESS,
               nodeInput,
               "PIM-SM-STATIC-NUMBER-OF-RP",
               &numStaticRPFound,
               &numStaticRP);

    /*
    if PIM-SM-STATIC-NUMBER-OF-RP not present then search for global level
    or node level PIM-SM-STATIC-RP-ADDRESS else search for instance level
    PIM-SM-STATIC-RP-ADDRESS

    The following check "(numStaticRPFound && numStaticRP == 1)" is required
    to handle cases in which the number of static RPs is configured as 1 but the
    further parameters are not configured on instance level.Instead they
    are configured at global level which is not a misconfiguration
    */
    if ((!numStaticRPFound) || numStaticRP == 1)
    {
        IO_ReadString(node->nodeId,
                      ANY_ADDRESS,
                      nodeInput,
                      "PIM-SM-STATIC-RP-ADDRESS",
                      &found,
                      parameterValue);

        if (found)
        {
            processed = TRUE;
            isStaticRPPresent = TRUE;
            IO_ParseNodeIdHostOrNetworkAddress(parameterValue,
                                               &RPAddr,
                                               &numHostBits,
                                               &isNodeId);


            //Read RP Priority
            found = FALSE;
            int rpPriority = 0;
            IO_ReadInt(node->nodeId,
                       ANY_ADDRESS,
                       nodeInput,
                       "PIM-SM-STATIC-RP-PRIORITY",
                       &found,
                       &rpPriority);

            if (found)
            {
                if ((0 <= rpPriority) &&
                   (rpPriority <= ROUTING_PIM_MAX_RP_PRIORITY))
                {
                    if (DEBUG_BS)
                    {
                        printf("\nNode %u has Static-RP with priority %d \n",
                                node->nodeId,rpPriority);
                    }
                }
                else
                {
                    ERROR_ReportError("\nRP-PRIORITY should lie in "
                                      "the range 0-255\n");
                }
            }
            else
            {
                rpPriority = (unsigned char)
                             ROUTING_PIM_CANDIDATE_RP_PRIORITY;
            }

            //Read RP supported group ranges
            found = FALSE;
            parameterValue[0] = '\0';
            IO_ReadString(node->nodeId,
                          ANY_ADDRESS,
                          nodeInput,
                          "PIM-SM-STATIC-RP-GROUP-RANGE",
                          &found,
                          parameterValue);

            if (found)
            {
                rpAccessList = RoutingPimSmParseGroupRangeStr(node,
                                               parameterValue,
                                               NULL);

                rpAccessListItem = rpAccessList->first;
                for (i = 0; i < rpAccessList->size; i++)
                {
                    //Create a new RP Set entry
                    RPListPtr = (RoutingPimSmRPList*)
                                  MEM_malloc(sizeof(RoutingPimSmRPList));

                    accessListPtr =
                        (RoutingPimSmRPAccessList*)rpAccessListItem->data;

                    RPListPtr->grpPrefix = accessListPtr->groupAddr;

                    RPListPtr->holdtime =
                           ROUTING_PIMSM_CANDIDATE_RP_TIMEOUT_DEFAULT/SECOND;

                    RPListPtr->maskLength = (unsigned char)
                    RoutingPimGetMaskLengthFromSubnetMask(
                                                   accessListPtr->groupMask);

                    RPListPtr->priority = (unsigned char) rpPriority;
                    RPListPtr->RPAddress = RPAddr;

                    ListInsert(node, pimDataSm->RPList,
                               0, (void*) RPListPtr);

                    rpAccessListItem = rpAccessListItem->next;
                }//end of for
                if ((rpAccessList != NULL) && (rpAccessList->size != 0))
                {
                    ListFree(node, rpAccessList, FALSE);
                }
            }
            else
            {
                //Create a new RP Set entry with a grp addr = 0xe0000000/4
                RPListPtr = (RoutingPimSmRPList*)
                              MEM_malloc(sizeof(RoutingPimSmRPList));

                RPListPtr->grpPrefix = 0xE0000000;

                RPListPtr->holdtime =
                       ROUTING_PIMSM_CANDIDATE_RP_TIMEOUT_DEFAULT/SECOND;

                RPListPtr->maskLength = (unsigned char)4;

                RPListPtr->priority = (unsigned char)rpPriority;
                RPListPtr->RPAddress = RPAddr;
                ListInsert(node, pimDataSm->RPList, 0, (void*) RPListPtr);
            }
        }
        else
        {
            processed = FALSE;
   }
    }

    if (numStaticRPFound && processed == FALSE)   //instance level searching
    {
        if (numStaticRP <= 0)
        {
            ERROR_ReportError("PIM-SM-STATIC-NUMBER-OF-RP should be > 0 \n");
        }

        BOOL retVal = FALSE;
        for (i = 0 ; i < numStaticRP ; i++)
        {
            IO_ReadStringInstance(node->nodeId,
                                  ANY_ADDRESS,
                                  nodeInput,
                                  "PIM-SM-STATIC-RP-ADDRESS",
                                  i,
                                  FALSE,
                                  &retVal,
                                  parameterValue);

            if (retVal)
            {
                isStaticRPPresent = TRUE;
                IO_ParseNodeIdHostOrNetworkAddress(parameterValue,
                                                   &RPAddr,
                                                   &numHostBits,
                                                   &isNodeId);

                //Read RP Priority
                int rpPriority = 0;

                IO_ReadStringInstance(node->nodeId,
                                      ANY_ADDRESS,
                                      nodeInput,
                                      "PIM-SM-STATIC-RP-PRIORITY",
                                      i,
                                      FALSE,
                                      &retVal,
                                      parameterValue);

                if (retVal)
                {
                    if ((0 <= rpPriority)
                        &&
                        (rpPriority <= ROUTING_PIM_MAX_RP_PRIORITY))
                    {
                        if (DEBUG_BS)
                        {
                            printf("\nNode %u has Static-RP with priority "
                                "%d \n", node->nodeId,rpPriority);
                        }
                    }
                    else
                    {
                        ERROR_ReportError("\nRP-PRIORITY should lie in the"
                                          " range 0-255\n");
                    }
                }
                else
                {
                    rpPriority = (unsigned char)
                                 ROUTING_PIM_CANDIDATE_RP_PRIORITY;
                }

                //Read RP supported group ranges
                retVal = FALSE;
                parameterValue[0] = '\0';

                IO_ReadStringInstance(node->nodeId,
                                      ANY_ADDRESS,
                                      nodeInput,
                                      "PIM-SM-STATIC-RP-GROUP-RANGE",
                                      i,
                                      FALSE,
                                      &retVal,
                                      parameterValue);

                if (retVal)
                {
                    rpAccessList = RoutingPimSmParseGroupRangeStr(node,
                                                              parameterValue,
                                                              NULL);

                    rpAccessListItem = rpAccessList->first;
                    for (j = 0; j < rpAccessList->size; j++)
                    {
                        //Create a new RP Set entry
                        RPListPtr = (RoutingPimSmRPList*)
                                      MEM_malloc(sizeof(RoutingPimSmRPList));

                        accessListPtr =
                           (RoutingPimSmRPAccessList*)rpAccessListItem->data;

                        RPListPtr->grpPrefix = accessListPtr->groupAddr;

                        RPListPtr->holdtime =
                           ROUTING_PIMSM_CANDIDATE_RP_TIMEOUT_DEFAULT/SECOND;

                        RPListPtr->maskLength = (unsigned char)
                        RoutingPimGetMaskLengthFromSubnetMask(
                                                    accessListPtr->groupMask);

                        RPListPtr->priority = (unsigned char)rpPriority;
                        RPListPtr->RPAddress = RPAddr;

                        ListInsert(node,
                                   pimDataSm->RPList,
                                   0,
                                   (void*) RPListPtr);

                        rpAccessListItem = rpAccessListItem->next;
                    }//end of for

                    if ((rpAccessList != NULL) && (rpAccessList->size != 0))
                    {
                        ListFree(node, rpAccessList, FALSE);
                    }
                }
                else
                {
                   //Create a new RP Set entry with a grp addr = 0xe0000000/4
                    RPListPtr = (RoutingPimSmRPList*)
                                  MEM_malloc(sizeof(RoutingPimSmRPList));

                    RPListPtr->grpPrefix = 0xE0000000;

                    RPListPtr->holdtime =
                           ROUTING_PIMSM_CANDIDATE_RP_TIMEOUT_DEFAULT/SECOND;

                    RPListPtr->maskLength = (unsigned char)4;

                    RPListPtr->priority = (unsigned char)rpPriority;
                    RPListPtr->RPAddress = RPAddr;
                    ListInsert(node, pimDataSm->RPList,
                               0,
                               (void*) RPListPtr);
                }//end of else
            }
        }//end of for
    }

    return isStaticRPPresent;
}

/*
*  FUNCTION    : RoutingPimSmCheckForCandidateRP()
*  PURPOSE     : Routing PIMSM search for Candidate RP
*  RETURN VALUE: NULL
*/
void
RoutingPimSmCheckForCandidateRP(Node* node,
                                int interfaceIndex,
                                const NodeInput* nodeInput)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];

    char buf[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;

    IO_ReadString(node->nodeId,
                  NetworkIpGetInterfaceAddress(node,interfaceIndex),
                  nodeInput,
                  "PIM-SM-CANDIDATE-RP",
                  &retVal,
                  buf);

    /* if there exist some Candidate RP read them */
    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (DEBUG_BS)
            {
                printf("Node %u is Candidate-RP at interface %d\n",
                       node->nodeId ,
                       interfaceIndex);
                printf("It's address is 0x%x\n",
                       NetworkIpGetInterfaceAddress(node, interfaceIndex));
            }

            thisInterface->RPFlag = TRUE;

            RoutingPimSmReadCandidateRPparameters(node,
                                                  interfaceIndex,
                                                  nodeInput);
        }
        else
        {
            if (strcmp(buf, "NO") == 0)
            {
                thisInterface->RPFlag = FALSE;
            }
            else
            {
                ERROR_ReportError("PIM-SM-CANDIDATE-RP should be \
                                  either \"YES\" or \"" "NO\".\n");
            }
        }
    }
    else
    {
        thisInterface->RPFlag = FALSE;
#ifdef ADDON_BOEINGFCS
        RoutingRPimSmSetCandidateRP(node, interfaceIndex);
#endif
    }
}

/*
*  FUNCTION    : RoutingPimSmReadCandidateBSRparameters()
*  PURPOSE     : Routing PIMSM read Candidate BSR parameters
*  RETURN VALUE: NULL
*/
void
RoutingPimSmReadCandidateBSRparameters(Node* node,
                                      int interfaceIndex,
                                      const NodeInput* nodeInput)
{
    PimData* pim = (PimData*)
         NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];
    BOOL retVal = FALSE;

    int bsrPriority = 0;
    IO_ReadInt(node->nodeId,
                  NetworkIpGetInterfaceAddress(node, interfaceIndex),
                  nodeInput,
                  "PIM-SM-CANDIDATE-BSR-PRIORITY",
                  &retVal,
                  &bsrPriority);

    if (retVal)
    {
        if ((0 <= bsrPriority)
            &&
            (bsrPriority <= ROUTING_PIM_MAX_BSR_PRIORITY))
        {
            thisInterface->bsrData.bsrPriority =
                                (unsigned char)bsrPriority;

            if (DEBUG_BS)
            {
                printf("Node %u is Candidate-BSR with priority %d\n",
                       node->nodeId,
                       thisInterface->bsrData.bsrPriority);
                printf("It's address is 0x%x\n",
                NetworkIpGetInterfaceAddress(node, interfaceIndex));
            }
        }
        else
        {
            ERROR_ReportError("BSR-PRIORITY should lie in the"
                              " range 0-255\n");
        }
    }
    else
    {
        thisInterface->bsrData.bsrPriority =
            (unsigned char)ROUTING_PIM_CANDIDATE_BSR_PRIORITY;
    }
}

/*
*  FUNCTION    : RoutingPimSmCheckForCandidateBSR()
*  PURPOSE     : Routing PIMSM search for Candidate BSR
*  RETURN VALUE: NULL
*/
void
RoutingPimSmCheckForCandidateBSR(Node* node,
                                 int interfaceIndex,
                                 const NodeInput* nodeInput)
{
    PimData* pim = (PimData*)
         NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];

    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    IO_ReadString(node->nodeId,
                  NetworkIpGetInterfaceAddress(node, interfaceIndex),
                  nodeInput,
                  "PIM-SM-CANDIDATE-BSR",
                  &retVal,
                  buf);

    thisInterface->BSRFlg = FALSE;
    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (DEBUG_BS)
            {
                printf("Node %u is Candidate-BSR at interface %d\n",
                       node->nodeId ,
                       interfaceIndex);
                printf("It's address is 0x%x\n",
                       NetworkIpGetInterfaceAddress(node, interfaceIndex));
            }

            thisInterface->BSRFlg = TRUE;
            RoutingPimSmReadCandidateBSRparameters(node,
                                                   interfaceIndex,
                                                   nodeInput);
        }
        else
        {
            if (strcmp(buf, "NO") == 0)
            {
                thisInterface->BSRFlg = FALSE;
            }
            else
            {
                ERROR_ReportError("PIM-SM-CANDIDATE-BSR should be "
                                  "either \"YES\" or \"" "NO\".\n");
            }
        }
    }
    else
    {
        if (thisInterface->RPFlag == TRUE)
        {
            thisInterface->BSRFlg = TRUE;
            thisInterface->bsrData.bsrPriority =
                                    thisInterface->rpData.rpPriority;
        }
        else
        {
            thisInterface->BSRFlg = FALSE;
#ifdef ADDON_BOEINGFCS
            RoutingRPimSmSetCandidateBSR(node, interfaceIndex);
#endif
        }
    }
}

/*
*  FUNCTION    : RoutingPimSmFindRPForThisGroup()
*  PURPOSE     : Routing PimSm Find RP For This Group.
*  RETURN VALUE: RP Address
*/
NodeAddress
RoutingPimSmFindRPForThisGroup(Node* node,
                               NodeAddress groupAddrToSearch)
{
    PimData* pim = (PimData*)
         NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    NodeAddress RPAddr = (unsigned int)NETWORK_UNREACHABLE;

    if (pimDataSm->RPList->size == 0)
    {
        return RPAddr;
    }

    RoutingPimSmRPList* RPListPtr = NULL;
    unsigned char maxMaskLen = 0;
    unsigned char maxPriority = ROUTING_PIM_MAX_RP_PRIORITY;

    LinkedList* longestMatchRPList = NULL;
    RoutingPimSmRPList* longestMatchRPListPtr = NULL;
    ListItem* longestMatchRPListItem = NULL;

    LinkedList* highestPriorityRPList = NULL;
    RoutingPimSmRPList* highestPriorityRPListPtr = NULL;

    ListItem* tempListItem = NULL;
    NodeAddress grpMask = (unsigned int)NETWORK_UNREACHABLE;
    NodeAddress maskedGroupAddressToSearch = 0;

#if 0
    #ifdef ADDON_BOEINGFCS
        if (MulticastCesRpimActiveOnNode(node))
        {

            RPAddr = MulticastCesRpimElectRPForThisGroup(node,
                     groupAddrToSearch);

            if (DEBUG_ERRORS)
            {
                printf("Node %u: \n", node->nodeId);
                printf("actual RP List size %d \n", pimDataSm->RPList->size);
                printf(" Resultant RP = %x \n", RPAddr);
            }

            return RPAddr;
        }
    #endif
#endif

//1. Perform longest match on group-range to obtain a list of RPs.
    ListInit(node, &longestMatchRPList);

    if (DEBUG_BS)
    {
        RoutingPimSmPrintRPList(node);
    }

    tempListItem = pimDataSm->RPList->first;
    while (tempListItem)
    {
        RPListPtr = (RoutingPimSmRPList*) tempListItem->data;
        grpMask = ConvertNumHostBitsToSubnetMask(32 - RPListPtr->maskLength);

        maskedGroupAddressToSearch = MaskIpAddress(groupAddrToSearch,
                                                   grpMask);

        if (RPListPtr->grpPrefix == maskedGroupAddressToSearch)
        {
            if (RPListPtr->maskLength > maxMaskLen)
            {
                if (longestMatchRPList != NULL)
                {
                    ListFree(node,
                             longestMatchRPList,
                             FALSE);
                 }

                ListInit(node, &longestMatchRPList);

                longestMatchRPListPtr = (RoutingPimSmRPList*)
                    MEM_malloc(sizeof(RoutingPimSmRPList));

                /* copy the available info from RP list for this RP */
                longestMatchRPListPtr->RPAddress = RPListPtr->RPAddress;
                longestMatchRPListPtr->holdtime = RPListPtr->holdtime;
                longestMatchRPListPtr->priority = RPListPtr->priority;
                longestMatchRPListPtr->grpPrefix = RPListPtr->grpPrefix;
                longestMatchRPListPtr->maskLength = RPListPtr->maskLength;

                ListInsert(node,
                           longestMatchRPList,
                           (clocktype) 0,
                           longestMatchRPListPtr);

                //save the maximum mask length
                maxMaskLen = longestMatchRPListPtr->maskLength;
            }
            else if (RPListPtr->maskLength == maxMaskLen)
            {
                longestMatchRPListPtr = (RoutingPimSmRPList*)
                    MEM_malloc(sizeof(RoutingPimSmRPList));

                /* copy the available info from RP list for this RP */
                longestMatchRPListPtr->RPAddress = RPListPtr->RPAddress;
                longestMatchRPListPtr->holdtime = RPListPtr->holdtime;
                longestMatchRPListPtr->priority = RPListPtr->priority;
                longestMatchRPListPtr->grpPrefix = RPListPtr->grpPrefix;
                longestMatchRPListPtr->maskLength = RPListPtr->maskLength;

                ListInsert(node,
                           longestMatchRPList,
                           (clocktype) 0,
                           longestMatchRPListPtr);
            }
        }

        tempListItem = tempListItem->next;
    }//end of while

//2. From this list of matching RPs, find the one with highest priority.
//   Eliminate any RPs from the list that have lower priorities.

    if (longestMatchRPList->size == 1)
    {
        longestMatchRPListPtr =
            (RoutingPimSmRPList*)longestMatchRPList->first->data;

        RPAddr = longestMatchRPListPtr->RPAddress;

        if (longestMatchRPList != NULL)
        {
            ListFree(node,
                     longestMatchRPList,
                     FALSE);
        }

        if (DEBUG_BS)
        {
            printf("\nRP Address found is 0x%x\n",RPAddr);
        }

        return RPAddr;
    }
    else //multiple RPs
    {
        if (longestMatchRPList->size == 0)
        {
            if (longestMatchRPList != NULL)
            {
                ListFree(node,
                         longestMatchRPList,
                         FALSE);
            }

            if (DEBUG_BS)
            {
                printf("\nRP is UNREACHABLE!!!\n");
            }

            return ((unsigned) NETWORK_UNREACHABLE);
        }
        ListInit(node, &highestPriorityRPList);

        longestMatchRPListItem = longestMatchRPList->first;
        while (longestMatchRPListItem)
        {
            RPListPtr = (RoutingPimSmRPList*) longestMatchRPListItem->data;

            /*
            Note that lower values of this field indicate higher
            priorities, so that a value of zero is the highest possible
            priority
            */
            if (RPListPtr->priority < maxPriority)
            {
                if (highestPriorityRPList != NULL)
                {
                    ListFree(node,
                             highestPriorityRPList,
                             FALSE);
                 }
                ListInit(node, &highestPriorityRPList);

                highestPriorityRPListPtr = (RoutingPimSmRPList*)
                    MEM_malloc(sizeof(RoutingPimSmRPList));

                /* copy the available info from RP list for this RP */
                highestPriorityRPListPtr->RPAddress = RPListPtr->RPAddress;
                highestPriorityRPListPtr->holdtime = RPListPtr->holdtime;
                highestPriorityRPListPtr->priority = RPListPtr->priority;
                highestPriorityRPListPtr->grpPrefix = RPListPtr->grpPrefix;
                highestPriorityRPListPtr->maskLength = RPListPtr->maskLength;

                ListInsert(node,
                           highestPriorityRPList,
                           (clocktype) 0,
                           highestPriorityRPListPtr);

                //save the maximum mask length
                maxPriority = highestPriorityRPListPtr->priority;
            } //end of if
            else if (RPListPtr->priority == maxPriority)
            {
                highestPriorityRPListPtr = (RoutingPimSmRPList*)
                    MEM_malloc(sizeof(RoutingPimSmRPList));

                /* copy the available info from RP list for this RP */
                highestPriorityRPListPtr->RPAddress = RPListPtr->RPAddress;
                highestPriorityRPListPtr->holdtime = RPListPtr->holdtime;
                highestPriorityRPListPtr->priority = RPListPtr->priority;
                highestPriorityRPListPtr->grpPrefix = RPListPtr->grpPrefix;
                highestPriorityRPListPtr->maskLength = RPListPtr->maskLength;

                ListInsert(node,
                           highestPriorityRPList,
                           (clocktype) 0,
                           highestPriorityRPListPtr);
            }//end of else

            longestMatchRPListItem = longestMatchRPListItem->next;
        }//end of while (longestMatchRPListItem)
    }//end of else


//3. If only one RP remains in the list, use that RP.

    if (highestPriorityRPList->size == 1)
    {
        highestPriorityRPListPtr =
            (RoutingPimSmRPList*)highestPriorityRPList->first->data;

        RPAddr = highestPriorityRPListPtr->RPAddress;
    }

    /*4. If multiple RPs are in the list, use the PIM hash function to
     choose one.*/
    else
    {
        if (highestPriorityRPList->size == 0)
        {
            RPAddr = (unsigned) NETWORK_UNREACHABLE;
        }
        else
        {
            if (DEBUG_BS)
            {
                printf(" Node %u: use the PIM hash function to choose RP\n",
                       node->nodeId);
            }

            unsigned int x = 1103515245;
            unsigned int y = 12345;
            unsigned int z = (unsigned int) pow(2.0, 31);
            double currentHashValue = 0;
            NodeAddress maskedGrpAddr = (unsigned int)NETWORK_UNREACHABLE;

            unsigned int value = 0;
            RoutingPimSmRPList* listPtr = NULL;
            NodeAddress currentRPAddr = (unsigned int)NETWORK_UNREACHABLE;
            NodeAddress localRPAddress = (unsigned int)NETWORK_UNREACHABLE;

            ListItem* tempItem = highestPriorityRPList->first;
            maskedGrpAddr = MaskIpAddress(groupAddrToSearch,
                                          PIMSM_HASH_MASK);

            while (tempItem)
            {
                listPtr = (RoutingPimSmRPList*)tempItem->data;

                localRPAddress = listPtr->RPAddress;

                /* calculate hash function value */
                value = (x* ((x* maskedGrpAddr + y) ^ (localRPAddress)) + y);
                value = value % z;

                if (value > currentHashValue)
                {
                    currentHashValue = value;
                    /* set the RPAddress as current RPAddress */
                    currentRPAddr = localRPAddress;
                }
                else
                {
                    /*
                     If more than one RP has the same highest hash value, the
                     RP with the highest IP address is chosen.
                    */
                    if (value == currentHashValue)
                    {
                        if (currentRPAddr < localRPAddress)
                        {
                            currentRPAddr = localRPAddress;
                        }
                    }
               }

               tempItem = tempItem->next;
            }//End while


            RPAddr = currentRPAddr;
        }
    }

    if (DEBUG_BS)
    {
        printf("\n\nNode %u: \n", node->nodeId);
        printf("actual RP List size %d \n", pimDataSm->RPList->size);
        printf("Available RPList size = %d \n", highestPriorityRPList->size);
        printf("Resultant RP = 0x%x \n", RPAddr);
    }

    if (longestMatchRPList)
    {
        ListFree(node,
                 longestMatchRPList,
                 FALSE);
     }

    if (highestPriorityRPList != NULL)
    {
        ListFree(node,
                 highestPriorityRPList,
                 FALSE);
    }

    if (DEBUG_BS)
    {
        printf("\nNode %d : RP Address found is 0x%x for "
            "group 0x%x\n",node->nodeId,RPAddr,groupAddrToSearch);
    }

    return RPAddr;
}


/*
*  FUNCTION    : RoutingPimSmSendJoinPrunePacket()
*  PURPOSE     : RoutingPimSm Send Joining or Pruning information of groups
*  RETURN VALUE: NULL;
*/
void
RoutingPimSmSendJoinPrunePacket(Node* node,
                                NodeAddress srcAddr,
                                NodeAddress upstreamNbr,
                                NodeAddress grpAddr,
                                int outIntfId,
                                RoutingPimSmJoinPruneType  joinPruneType,
                                RoutingPimJoinPruneActionType actionType,
                          RoutingPimSmTreeInfoBaseRow * treeInfoBaseRowPtr)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    bool ignoreProcessing = FALSE;
    NetworkDataIp* ip = (NetworkDataIp*)node->networkData.networkVar;
    
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    if (pimDataSm && ip->isSSMEnable)
    {
        if (Igmpv3IsSSMAddress(node, grpAddr))
        {
            ignoreProcessing = TRUE;
        }
    }

    RoutingPimSmMulticastTreeState treeState = ROUTING_PIMSM_G;
    LinkedList* periodicSGRptInfoList = NULL;

    if (joinPruneType == ROUTING_PIMSM_G_JOIN_PRUNE)
    {
        treeState = ROUTING_PIMSM_G;
    }
    else if (joinPruneType == ROUTING_PIMSM_SG_JOIN_PRUNE)
    {
        treeState = ROUTING_PIMSM_SG;
    }
    else if (joinPruneType == ROUTING_PIMSM_SGrpt_JOIN_PRUNE)
    {
        treeState = ROUTING_PIMSM_SGrpt;
    }
    if ((ignoreProcessing) && (treeState != ROUTING_PIMSM_SG))
    {
        // If the grp Addr is in ssm range, then dont do any (*,G)
        // or (S,G,Rpt) processing.
        return;
    }
    /* if does not exist create one for SGrpt */
    if (treeState == ROUTING_PIMSM_SGrpt && treeInfoBaseRowPtr == NULL)
    {
        treeInfoBaseRowPtr =
            RoutingPimSmSetMulticastTreeInfoBase(node,
                             grpAddr,
                             srcAddr,
                             treeState);
    }
    else if (treeInfoBaseRowPtr == NULL)
    {
#ifdef EXATA
        if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE,
                "Join/Prune packet need to be send so entry \
                                     must exist in Tree Infor base \n");
        }
#else
        ERROR_Assert(FALSE,
        "Join/Prune packet need to be send so entry \
                             must exist in Tree Infor base \n");
#endif
        return;
    }
    // If we are currently suppressed exit without sending
    if (treeInfoBaseRowPtr->isSuppressed)
    {
        clocktype currTime = node->getNodeTime();

        if (treeInfoBaseRowPtr->suppressionEnds >
            currTime)
        {
            return;
        }
        else
        {
            treeInfoBaseRowPtr->isSuppressed = FALSE;
            RoutingPimJoinPruneMessageType isJoinOrPrune;
            // Recalculate upstream router
            unsigned short numJoinedSrc;
            unsigned short numPrunedSrc;
            if (actionType == ROUTING_PIM_JOIN_TREE)
            {
                numJoinedSrc = 1;
                numPrunedSrc = 0;
                isJoinOrPrune = ROUTING_PIM_JOIN;
            }
            else
            {
                numJoinedSrc = 0;
                numPrunedSrc = 1;
                isJoinOrPrune = ROUTING_PIM_PRUNE;
            }

            RoutingPimSmHandleUpstreamStateMachine(
                node,
                srcAddr,
                outIntfId,
                grpAddr,
                treeInfoBaseRowPtr);
        }
    }

    Message* joinPruneMsg;
    RoutingPimSmJoinPrunePacket* joinPrunePkt;
    RoutingPimSmJoinPruneGroupInfo grpInfo;
    memset(&grpInfo, 0, sizeof(RoutingPimSmJoinPruneGroupInfo));

    RoutingPimEncodedSourceAddr  encodedSrcAddr;
    /* check the action type  & switch accordingly */
    switch (actionType)
    {
        case ROUTING_PIM_JOIN_TREE:
        {
            /* set the group information */
            grpInfo.encodedGrpAddr.addressFamily = 1;
            grpInfo.encodedGrpAddr.encodingType = 0;
            grpInfo.encodedGrpAddr.reserved = 0;
            grpInfo.encodedGrpAddr.maskLength = 32;
            grpInfo.encodedGrpAddr.groupAddr = grpAddr;

            grpInfo.numJoinedSrc = 1;
            grpInfo.numPrunedSrc = 0;

            /* Set the encoded join source field */
            encodedSrcAddr.addressFamily = 1;
            encodedSrcAddr.encodingType = 0;
            PimSmEncodedSourceAddrSetReserved(&(encodedSrcAddr.rpSimEsa), 0);
            PimSmEncodedSourceAddrSetSparseBit(&(encodedSrcAddr.rpSimEsa),
                                               1);

            /*
             *  Each group specific set may contain (*, G), (S, G, rpt) and
             *  (S, G) source list entries in the Join or Prune lists.
             */
            if (joinPruneType == ROUTING_PIMSM_G_JOIN_PRUNE)
            {
                if ((outIntfId >= 0)
                && ((unsigned) outIntfId != (unsigned) NETWORK_UNREACHABLE))
                {
                    pimDataSm->stats.numOfGJoinPacketForwarded++;
                }
                PimSmEncodedSourceAddrSetWildCard(&(encodedSrcAddr.rpSimEsa),
                                                  1);
                PimSmEncodedSourceAddrSetRPT(&(encodedSrcAddr.rpSimEsa), 1);
                treeState = ROUTING_PIMSM_G;

                /* When a router is going to send a Join(*, G), it checks
                for each (S, G) for which it has state, to decide whether to
                include a Prune(S, G, rpt) in the compound Join/Prune message
                */

                periodicSGRptInfoList =
                    RoutingPimSmCheckForSGRptPeriodicMessage(node, grpAddr);
            }
            else
                if (joinPruneType == ROUTING_PIMSM_SG_JOIN_PRUNE)
                {
                    if ((outIntfId >= 0)
                    && ((unsigned) outIntfId != (unsigned)
                    NETWORK_UNREACHABLE))
                    {
                        pimDataSm->stats.numOfSGJoinPacketForwarded++;
                    }
                    PimSmEncodedSourceAddrSetWildCard(
                                &(encodedSrcAddr.rpSimEsa), 0);
                    PimSmEncodedSourceAddrSetRPT(
                                &(encodedSrcAddr.rpSimEsa), 0);
                    treeState = ROUTING_PIMSM_SG;
                }
                else
                    if (joinPruneType == ROUTING_PIMSM_SGrpt_JOIN_PRUNE)
                    {
                        if ((outIntfId >= 0)
                            && ((unsigned) outIntfId !=
                            (unsigned) NETWORK_UNREACHABLE))
                        {
                            pimDataSm->stats.numOfSGrptJoinPacketForwarded++;
                        }
                        PimSmEncodedSourceAddrSetWildCard(
                                &(encodedSrcAddr.rpSimEsa), 0);
                        PimSmEncodedSourceAddrSetRPT(
                                &(encodedSrcAddr.rpSimEsa), 1);
                        treeState = ROUTING_PIMSM_SGrpt;
                    }
            encodedSrcAddr.maskLength = 32;
            encodedSrcAddr.sourceAddr = srcAddr;
            break;
        }
        case ROUTING_PIM_PRUNE_TREE:
        {
            /* set the group info */
            grpInfo.encodedGrpAddr.addressFamily = 1;
            grpInfo.encodedGrpAddr.encodingType = 0;
            grpInfo.encodedGrpAddr.reserved = 0;
            grpInfo.encodedGrpAddr.maskLength = 32;
            grpInfo.encodedGrpAddr.groupAddr = grpAddr;

            grpInfo.numJoinedSrc = 0;
            grpInfo.numPrunedSrc = 1;

            /* Set the encoded prune source field */
            encodedSrcAddr.addressFamily = 1;
            encodedSrcAddr.encodingType = 0;
            PimSmEncodedSourceAddrSetReserved(
                    &(encodedSrcAddr.rpSimEsa), 0);
            PimSmEncodedSourceAddrSetSparseBit(
                    &(encodedSrcAddr.rpSimEsa), 1);
            /*
             *  Each group specific set may contain (*, G), (S, G, rpt) and
             *  (S, G) source list entries in the Join or Prune lists.
             */
            if (joinPruneType == ROUTING_PIMSM_G_JOIN_PRUNE)
            {
                if ((outIntfId >= 0)
                && ((unsigned) outIntfId != (unsigned) NETWORK_UNREACHABLE))
                {
                    pimDataSm->stats.numOfGPrunePacketForwarded++;
                }
                PimSmEncodedSourceAddrSetWildCard(&(encodedSrcAddr.rpSimEsa),
                                                  1);
                PimSmEncodedSourceAddrSetRPT(&(encodedSrcAddr.rpSimEsa), 1);
                treeState = ROUTING_PIMSM_G;
            }
            else
                if (joinPruneType == ROUTING_PIMSM_SG_JOIN_PRUNE)
                {
                    if ((outIntfId >= 0)
                    && ((unsigned) outIntfId != (unsigned)
                    NETWORK_UNREACHABLE))
                    {
                        pimDataSm->stats.numOfSGPrunePacketForwarded++;
                    }
                    PimSmEncodedSourceAddrSetWildCard(
                        &(encodedSrcAddr.rpSimEsa), 0);
                    PimSmEncodedSourceAddrSetRPT(
                        &(encodedSrcAddr.rpSimEsa), 0);
                    treeState = ROUTING_PIMSM_SG;
                }
                else
                    if (joinPruneType == ROUTING_PIMSM_SGrpt_JOIN_PRUNE)
                    {
                        if ((outIntfId >= 0)
                        && ((unsigned) outIntfId != (unsigned)
                        NETWORK_UNREACHABLE))
                        {
                            pimDataSm->stats.
                                numOfSGrptPrunePacketForwarded++;
                        }
                        PimSmEncodedSourceAddrSetWildCard(&
                            (encodedSrcAddr.rpSimEsa), 0);
                        PimSmEncodedSourceAddrSetRPT(&
                            (encodedSrcAddr.rpSimEsa), 1);
                        treeState = ROUTING_PIMSM_SGrpt;
                    }
            encodedSrcAddr.maskLength = 32;
            encodedSrcAddr.sourceAddr = srcAddr;

            break;
        }
        default:
        {
#ifdef EXATA
            if (!node->partitionData->isEmulationMode)
            {
                ERROR_Assert(FALSE, "Unknown JOIN PRUNE  Event\n");
            }
#else
            ERROR_Assert(FALSE, "Unknown JOIN PRUNE  Event\n");
#endif
            return;
        }
    }   //end of switch;

    char* dataPtr;
    unsigned int size;

    /* Send Join or Prune for anly one (S, G) pair */
    joinPruneMsg = MESSAGE_Alloc(node,
                                 NETWORK_LAYER,
                                 MULTICAST_PROTOCOL_PIM,
                                 MSG_ROUTING_PimPacket);

     size = sizeof(RoutingPimSmJoinPrunePacket)
           + sizeof(RoutingPimSmJoinPruneGroupInfo) +
           sizeof(RoutingPimEncodedSourceAddr);

    // Incerase the number of Prune message if SGRpt Prune included
    if (periodicSGRptInfoList && ListGetSize(node, periodicSGRptInfoList))
    {
        grpInfo.numPrunedSrc = grpInfo.numPrunedSrc + (unsigned short)
                    ListGetSize(node, periodicSGRptInfoList);

        //increase the size
        size = size + ((ListGetSize(node, periodicSGRptInfoList))*
                            sizeof(RoutingPimEncodedSourceAddr));
    }
    MESSAGE_PacketAlloc(node, joinPruneMsg, size, TRACE_PIM);

    joinPrunePkt = (RoutingPimSmJoinPrunePacket*)
                   MESSAGE_ReturnPacket(joinPruneMsg);

    /* Set the Header */
    RoutingPimCreateCommonHeader(&joinPrunePkt->commonHeader,
                                 ROUTING_PIM_JOIN_PRUNE);
    /* set the packet field */
    joinPrunePkt->upstreamNbr.addressFamily = 1;/*for IP4 */
    joinPrunePkt->upstreamNbr.encodingType = 0;
    setNodeAddressInCharArray(joinPrunePkt->upstreamNbr.unicastAddr,
                              upstreamNbr);

    joinPrunePkt->reserved = 0;

    /* Only one group per packet */
    joinPrunePkt->numGroups = 1;
    joinPrunePkt->holdTime = (unsigned short)
        (pimDataSm->routingPimSmJoinPruneHoldTimeout / SECOND);

    dataPtr = (char*) joinPrunePkt + sizeof(RoutingPimSmJoinPrunePacket);
    memcpy(dataPtr, &grpInfo, sizeof(RoutingPimSmJoinPruneGroupInfo));

    dataPtr += sizeof(RoutingPimSmJoinPruneGroupInfo);

    /* Put source address */
    memcpy(dataPtr, &encodedSrcAddr, sizeof(RoutingPimEncodedSourceAddr));
    if ((encodedSrcAddr.sourceAddr == 0)
            || (grpInfo.encodedGrpAddr.groupAddr == 0)
            || (getNodeAddressFromCharArray(
                    joinPrunePkt->upstreamNbr.unicastAddr) == 0))
    {
#ifdef EXATA
        if (node->partitionData->isEmulationMode)
        {
            char buf[MAX_STRING_LENGTH];
            sprintf(buf,
              "Node %d received malformed Join\n",
                    node->nodeId);
            ERROR_ReportWarning(buf);
        }
        else
            ERROR_Assert(FALSE, "wrong input\n");
#else
        ERROR_Assert(FALSE, "wrong input\n");
#endif
        return;
    }
    if (periodicSGRptInfoList)
    {
        ListItem* tempListItem;
        tempListItem = periodicSGRptInfoList->first;
        while (tempListItem)
        {
            NodeAddress* SGRptsrcAddress =
                    (NodeAddress*)tempListItem->data;

            dataPtr += sizeof(RoutingPimEncodedSourceAddr);
            PimSmEncodedSourceAddrSetWildCard(&
                                (encodedSrcAddr.rpSimEsa), 0);
            PimSmEncodedSourceAddrSetRPT(&
                    (encodedSrcAddr.rpSimEsa), 1);
            encodedSrcAddr.maskLength = 32;
            encodedSrcAddr.sourceAddr = *SGRptsrcAddress;
            /* Put source address */
            memcpy(dataPtr, &encodedSrcAddr,
                sizeof(RoutingPimEncodedSourceAddr));
            tempListItem = tempListItem->next;
            if ((outIntfId >= 0)
            && ((unsigned) outIntfId != (unsigned) NETWORK_UNREACHABLE))
            {
                pimDataSm->stats.numOfSGrptPrunePacketForwarded++;
            }
        }
    }
    if ((outIntfId >= 0)
            && ((unsigned) outIntfId != (unsigned) NETWORK_UNREACHABLE))
    {

        TosType priority = IPTOS_PREC_INTERNETCONTROL;

#ifdef ADDON_BOEINGFCS
        if (MulticastCesRpimActiveOnInterface(node, outIntfId))
        {
            priority = IPTOS_PREC_PRIORITY;
        }
#endif

        /* Now send packet out through this interface */
        NetworkIpSendRawMessageToMacLayer(
            node,
            MESSAGE_Duplicate(node, joinPruneMsg),
            NetworkIpGetInterfaceAddress(node, outIntfId),
            ALL_PIM_ROUTER,
            priority,
            IPPROTO_PIM,
            1,
            outIntfId,
            ANY_DEST);

        pimDataSm->stats.numOfJoinPrunePacketForwarded++;
#ifdef ADDON_DB
        pimDataSm->smSummaryStats.m_NumJoinPruneSent++;
#endif

        if (DEBUG)
        {

            char clockStr[100];
            char debugBuff[100];
            ctoa(node->getNodeTime(), clockStr);
            strcpy(debugBuff,
                   (actionType == ROUTING_PIM_JOIN_TREE ? "JOIN": "PRUNE"));

            printf("Node %d send %s to intf %d at time %s\n",
                   node->nodeId, debugBuff, outIntfId, clockStr);
            printf(" the joinPruneType is %d \n", joinPruneType);
            printf(" pktSrc: %x  pktGrp: %x pktUpstream: %x \n",
                   encodedSrcAddr.sourceAddr,
                   grpInfo.encodedGrpAddr.groupAddr,
                   getNodeAddressFromCharArray(
                   joinPrunePkt->upstreamNbr.unicastAddr));
        }


        treeInfoBaseRowPtr->lastJoinPruneSend = node->getNodeTime();
    }
    if ((periodicSGRptInfoList != NULL) &&
            (periodicSGRptInfoList->size != 0))
    {
        ListFree(node, periodicSGRptInfoList, FALSE);
    }
    MESSAGE_Free(node, joinPruneMsg);
}


/*
*  FUNCTION    : RoutingPimSmSetMulticastTreeInfoBase
*  PURPOSE     : Roting PimSm Set Multicast Tree Information Base Table
*  RETURN VALUE: NULL
*/
RoutingPimSmTreeInfoBaseRow*
RoutingPimSmSetMulticastTreeInfoBase(Node* node,
                                     NodeAddress groupAddr,
                                     NodeAddress srcAddr,
                                    RoutingPimSmMulticastTreeState treeState)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    RoutingPimSmTreeInformationBase* treeInfo = &pimDataSm->treeInfoBase;

    TreeInfoRowMap* rowPtrMap = treeInfo->rowPtrMap;
    TreeInfoRowKey rowKey;
    memset(&rowKey, 0, sizeof(TreeInfoRowKey));

    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr = NULL;
    treeInfoBaseRowPtr = (RoutingPimSmTreeInfoBaseRow*)
        MEM_malloc(sizeof(RoutingPimSmTreeInfoBaseRow));
    memset(treeInfoBaseRowPtr, 0, sizeof(RoutingPimSmTreeInfoBaseRow));

    TreeInfoRowInsertSucceeded insertSucceeded;
    RoutingPimSmTreeInfoBaseRow* newtreeInfoBaseRowPtr = NULL;
    NodeAddress RPAddr;
    NodeAddress nextHop;
    int outInterface;

    /* check the RP For this group & find associated nextHop*/
    RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);

    if (DEBUG) {
        printf("Node %u: row RP = %x \n", node->nodeId, RPAddr);
        printf("    Num Rows (before) = %d\n", treeInfo->numEntries);
    }

    if (DEBUG_ERRORS) {
        printf("Node %u has the new entries: \n", node->nodeId);
        printf("    grpAddr = %d \n", groupAddr);
        printf("    RPAddr = %x \n", RPAddr);
        printf("    Num Rows (before) = %d\n", treeInfo->numEntries);
    }

    /* Check if the table has been filled */
    /* Insert new entry in map */
    treeInfoBaseRowPtr->grpAddr = groupAddr;
    treeInfoBaseRowPtr->treeState = treeState;
    treeInfoBaseRowPtr->SPTbit = FALSE;
    treeInfoBaseRowPtr->keepAliveExpiryTimer = 0;
    treeInfoBaseRowPtr->OTTimerSeq = 0;
    treeInfoBaseRowPtr->registerState = PimSm_Register_NoInfo;
    treeInfoBaseRowPtr->registerStopTimerSeq = 0;
    treeInfoBaseRowPtr->isTunnelPresent = TRUE;
    treeInfoBaseRowPtr->lastOTTimerStart = 0;
    treeInfoBaseRowPtr->lastJoinTimerEnd = 0;
    treeInfoBaseRowPtr->lastJoinPruneSend = 0;
    treeInfoBaseRowPtr->lastOTTimerEnd = 0;
    treeInfoBaseRowPtr->RPointAddr = RPAddr;
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, RPAddr,
            &outInterface, &nextHop);

    if (nextHop == 0)
    {
        nextHop = RPAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHop,
                                           &outInterface);
   }

    treeInfoBaseRowPtr->nextHopForRP = nextHop;
    treeInfoBaseRowPtr->nextIntfForRP = outInterface;

    treeInfoBaseRowPtr->joinSeenInUpIntf = FALSE;
    treeInfoBaseRowPtr->pruneSeenInUpIntf = FALSE;

    /* check the tree state & set accordingly */
    if (treeState == ROUTING_PIMSM_G)
    {
        treeInfoBaseRowPtr->srcAddr = RPAddr;
        RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                            RPAddr,
                                                            &outInterface,
                                                            &nextHop);

        if (nextHop == 0)
        {
            nextHop = RPAddr ;
        }
        else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
        {
            RoutingPimSmGetNextHopOutInterface(node,
                                               RPAddr,
                                               &nextHop,
                                               &outInterface);
        }

        treeInfoBaseRowPtr->oldRPatSrcDR = RPAddr;
        treeInfoBaseRowPtr->upstream = nextHop;
        treeInfoBaseRowPtr->upInterface = outInterface;

        treeInfoBaseRowPtr->nextHopForSrc = nextHop;
        treeInfoBaseRowPtr->nextIntfForSrc = outInterface;
    }
    else
        if (treeState == ROUTING_PIMSM_SG ||
            treeState == ROUTING_PIMSM_SGrpt)
        {
            treeInfoBaseRowPtr->srcAddr = srcAddr;
            RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                                srcAddr,
                                                                &outInterface,
                                                                &nextHop);

            if (nextHop == 0)
            {
                nextHop = srcAddr ;
            }
            else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
            {
                RoutingPimSmGetNextHopOutInterface(node,
                                                   srcAddr,
                                                   &nextHop,
                                                   &outInterface);
            }

            treeInfoBaseRowPtr->oldRPatSrcDR = RPAddr;
            treeInfoBaseRowPtr->upstream = nextHop;
            treeInfoBaseRowPtr->upInterface = outInterface;

            treeInfoBaseRowPtr->nextHopForSrc = nextHop;
            treeInfoBaseRowPtr->nextIntfForSrc = outInterface;
        }
    if (treeState == ROUTING_PIMSM_SG || treeState == ROUTING_PIMSM_G
        || treeState == ROUTING_PIMSM_RP)
    {
        treeInfoBaseRowPtr->upstreamState = PimSm_JoinPrune_NotJoin;
    }
    else if (treeState == ROUTING_PIMSM_SGrpt)
    {
        newtreeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                             (node, groupAddr, srcAddr,
                              ROUTING_PIMSM_G);
        if (((newtreeInfoBaseRowPtr != NULL)
            && (newtreeInfoBaseRowPtr->upstreamState !=
            PimSm_JoinPrune_Join)) || (newtreeInfoBaseRowPtr == NULL))
        {
            treeInfoBaseRowPtr->upstreamState = PimSm_SGrpt_NotJoined;
        }
        else
        {
            treeInfoBaseRowPtr->upstreamState = PimSm_SGrpt_NotPruned;
        }
    }

    treeInfoBaseRowPtr->upAssertWinner = (unsigned) NETWORK_UNREACHABLE;
    treeInfoBaseRowPtr->metric.RPTbit = 1;
    treeInfoBaseRowPtr->metric.metricPreference =
                    ROUTING_ADMIN_DISTANCE_DEFAULT;
    treeInfoBaseRowPtr->metric.metric = 0xFFFFFFFF;
    treeInfoBaseRowPtr->metric.ipAddress = (unsigned)NETWORK_UNREACHABLE;
    treeInfoBaseRowPtr->joinTimerSeq = 0;

    /* set for assert state */
    treeInfoBaseRowPtr->upAssertState = PimSm_Assert_NoInfo;

    ListInit(node, &treeInfoBaseRowPtr->immediateOutIntfRP);
    ListInit(node, &treeInfoBaseRowPtr->immediateOutIntfG);
    ListInit(node, &treeInfoBaseRowPtr->immediateOutIntfSG);
    ListInit(node, &treeInfoBaseRowPtr->inheritedOutIntfSGrpt);
    ListInit(node, &treeInfoBaseRowPtr->inheritedOutIntfSG);

    /* Check downStream */
    ListInit(node, &treeInfoBaseRowPtr->downstream);

    /* Join/Prune Suppression */
    treeInfoBaseRowPtr->isSuppressed = FALSE;

    if (DEBUG_JP)
    {
        printf(" corresponding downstream List has size %d \n",
               treeInfoBaseRowPtr->downstream->size);
    }

    if (treeState == ROUTING_PIMSM_G)
    {
        rowKey.srcAddr = 0;
        rowKey.grpAddr = treeInfoBaseRowPtr->grpAddr;
        rowKey.treeState = treeInfoBaseRowPtr->treeState;
    }
    else //SG state or SG-RPT state
    {
        rowKey.srcAddr = treeInfoBaseRowPtr->srcAddr;
        rowKey.grpAddr = treeInfoBaseRowPtr->grpAddr;
        rowKey.treeState = treeInfoBaseRowPtr->treeState;
    }

    insertSucceeded = rowPtrMap->insert(make_pair(rowKey,
                                                  treeInfoBaseRowPtr));

    if (insertSucceeded.second)
    {
        if (DEBUG)
        {
            printf("Insert succeeded!\n");
        }

        treeInfo->numEntries++;
        return treeInfoBaseRowPtr;
    }
    else
    {
        printf("Insert failed!\n");
        return NULL;
    }
}


/*
*  FUNCTION    : RoutingPimSmSetMulticastForwardingTable()
*  PURPOSE     : Roting PimSm Set Multicast Forwarding Table
*  RETURN VALUE: NULL
*/
RoutingPimSmForwardingTableRow*
RoutingPimSmSetMulticastForwardingTable(Node* node, int interfaceId,
                                NodeAddress groupAddr, NodeAddress srcAddr)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmForwardingTable* fwdTable = &pimDataSm->forwardingTable;
    RoutingPimSmForwardingTableRow* fwdTableRowPtr;

    unsigned  int i;

    i = fwdTable->numEntries;

    if (DEBUG_ERRORS) {
        printf("Node %u has the new entries: \n", node->nodeId);
        printf("    grpAddr = %d \n", groupAddr);
        printf("    pkt received from = %d \n", interfaceId);
        printf("    Num Rows (before) = %d\n", i);
    }

    /* Check if the table has been filled */
    if (fwdTable->numEntries == fwdTable->numRowsAllocated)
    {
        RoutingPimSmForwardingTableRow* oldRow = fwdTable->rowPtr;

        /* Double forwarding table size */
        fwdTable->rowPtr = (RoutingPimSmForwardingTableRow*)
                           MEM_malloc(2*  fwdTable->numRowsAllocated*
                           sizeof(RoutingPimSmForwardingTableRow));
        for (i = 0; i < fwdTable->numEntries; i++)
        {
            fwdTable->rowPtr[i] = oldRow[i];
        }

        MEM_free(oldRow);

        /* Update table size */
        fwdTable->numRowsAllocated *= 2;
    }   // if table is filled

    /* Insert new row */
    fwdTable->rowPtr[i].grpAddr = groupAddr;
    fwdTable->rowPtr[i].srcAddr = srcAddr;
    fwdTable->rowPtr[i].inInterface = interfaceId;
    fwdTable->rowPtr[i].SPTbit = FALSE;
    fwdTable->rowPtr[i].pktRouted = FALSE;
    fwdTable->rowPtr[i].lastPktTTL = 64;

    fwdTableRowPtr = &fwdTable->rowPtr[i];

    fwdTable->numEntries++;
    return fwdTableRowPtr;
}


/*
*  FUNCTION    : RoutingPimSmSetDownstreamList
*  PURPOSE     : Roting PimSm Incorporate New Interface in Downstream List
*  RETURN VALUE: RoutingPimSmDownstream*
*/

RoutingPimSmDownstream*
RoutingPimSmSetDownstreamList(Node* node,int interfaceId,
                           RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr)

{
    RoutingPimSmDownstream* downStreamListPtr;

    downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                        treeInfoBaseRowPtr, interfaceId);

    if (downStreamListPtr == NULL)
    {
        RoutingPimSmDownstream* newdownStreamList =
            (RoutingPimSmDownstream*)
            MEM_malloc(sizeof(RoutingPimSmDownstream));

        // zero out all fields of struct so results are consistent
        // accross platforms.
        memset(newdownStreamList, 0, sizeof(RoutingPimSmDownstream));

        if (DEBUG_JP) {
            printf(" Time to add new interface in downstream \n");
        }

        newdownStreamList->interfaceId = interfaceId;

        newdownStreamList->intfAddr =
            NetworkIpGetInterfaceAddress (node, interfaceId);

        newdownStreamList->ttl = 1;
        newdownStreamList->joinPruneState = PimSm_JoinPrune_NoInfo;
        newdownStreamList->expiryTimerSeq = 0;
        newdownStreamList->prunePendingTimerSeq = 0;
        newdownStreamList->assertState = PimSm_Assert_NoInfo;
        newdownStreamList->assertTime = (clocktype)0;
        newdownStreamList->assertWinner = (unsigned) NETWORK_UNREACHABLE;

        /* AssertWinner defaults to Null and AssertWinnerMetric
            defaults to Infinity when in the NoInfo state. */

        newdownStreamList->assertWinnerMetric.RPTbit = 1;
        newdownStreamList->assertWinnerMetric.metricPreference =
            ROUTING_ADMIN_DISTANCE_DEFAULT;
        newdownStreamList->assertWinnerMetric.metric = 0xFFFFFFFF;
        newdownStreamList->assertWinnerMetric.ipAddress =
            (unsigned)NETWORK_UNREACHABLE;

        ListInsert(node, treeInfoBaseRowPtr->downstream, 0,
                   (void*) newdownStreamList);
        downStreamListPtr = newdownStreamList;
    }   // interface is in downstream list;

    if (DEBUG_JP) {
        printf("   corresponding final downstream List has size %d \n",
               treeInfoBaseRowPtr->downstream->size);

        RoutingPimSmPrintTreeInfoBase(node);
    }
    return downStreamListPtr;
}


/*
*  FUNCTION      : RoutingPimSmSearchTreeInfoBaseForThisGroup()
*  PURPOSE       : Search the cache for the specified TreeState & group
*  RETURN VALUE  : RoutingPimSmTreeInfoBaseRow*
*/

RoutingPimSmTreeInfoBaseRow*
RoutingPimSmSearchTreeInfoBaseForThisGroup(Node* node,
                                           NodeAddress grpAddr,
                                           NodeAddress srcAddr,
                                    RoutingPimSmMulticastTreeState treeState)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    RoutingPimSmTreeInformationBase* treeInfoBase = &pimDataSm->treeInfoBase;
    RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;
    TreeInfoRowKey rowKey;
    memset(&rowKey, 0, sizeof(TreeInfoRowKey));

    TreeInfoRowMap* rowPtrMap = treeInfoBase->rowPtrMap;
    TreeInfoRowMapIterator findIterator;

    if (treeState == ROUTING_PIMSM_G)
    {
        rowKey.srcAddr = 0 ;
    }
    else //SG state or SG-RPT state
    {
        rowKey.srcAddr = srcAddr;
    }

    rowKey.grpAddr = grpAddr;
    rowKey.treeState = treeState;

    findIterator = rowPtrMap->find(rowKey);
    if (findIterator == rowPtrMap->end())
    {
        if (DEBUG)
        {
            printf("\nTree Info not found in database");
        }
        return NULL;
    }
    else
    {
        rowPtr = findIterator->second;
        return rowPtr;
    }
}

/*
*  FUNCTION     : RoutingPimSmSearchForwardingTable()
*  PURPOSE      : Search the cache for the specified group & source
*  RETURN VALUE : RoutingPimSmTreeInfoBaseRow*
*/

RoutingPimSmForwardingTableRow*
RoutingPimSmSearchForwardingTable(Node* node, int interfaceId,
                                  NodeAddress grpAddr, NodeAddress srcAddr)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmForwardingTable* fwdTable = &pimDataSm->forwardingTable;
    RoutingPimSmForwardingTableRow * rowPtr;

    unsigned  int i;
    rowPtr = NULL;

    if (DEBUG_ERRORS) {
        printf("Node %u search the group from F-Table containing %d entry\n",
               node->nodeId, fwdTable->numEntries);
        printf(" grpAddr = %u & srcAddr = %x \n", grpAddr, srcAddr);
    }

    /* Search treeInfo Base for the desired match */
    for (i = 0; i < fwdTable->numEntries; i++)
    {
        if (DEBUG_ERRORS) {
            printf("   checking the fwd-table entry %d\n", i);
        }

        if (fwdTable->rowPtr[i].inInterface == interfaceId)
        {
            /* check for group */
            if (fwdTable->rowPtr[i].grpAddr == grpAddr)
            {
                if (DEBUG_ERRORS) {
                    printf(" group found in fwd Table\n");
                }

                /* check for corresponding source Addr */
                if (fwdTable->rowPtr[i].srcAddr == srcAddr)
                {
                    if (DEBUG) {
                        printf(" source-group found in fwd Table\n");
                    }

                    rowPtr = &fwdTable->rowPtr[i];
                    return rowPtr;
                }   //source matched;
            }   //group found;
        }
    }//end of for;
    return rowPtr;
}

/*
*  FUNCTION    : RoutingPimSmHandleDownstreamStateMachine()
*  PURPOSE     : PIM Join/Prune message modifies the downstream state machine
*                for all associated Joined and Pruned sources for each group.
*                Also modify downstream assert State;
*  RETURN VALUE: NULL
*/
void
RoutingPimSmHandleDownstreamStateMachine(Node* node, NodeAddress srcAddr,
        int interfaceId, NodeAddress groupAddr,
        RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr,
        RoutingPimJoinPruneMessageType isJoinOrPrune,
        clocktype joinPruneHoldTime)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    RoutingPimSmDownstream* downstreamListPtr;

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    if (treeInfoBaseRowPtr == NULL)
    {
#ifdef EXATA
       if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
        }
#else
        ERROR_Assert(FALSE,
            "JoinPrune_Pkt received so entry must exist\n");
#endif
        return;
    }
    /* Insert the corresponding downstream Interface */
    downstreamListPtr = RoutingPimSmSetDownstreamList (node, interfaceId,
                        treeInfoBaseRowPtr);

    if (DEBUG_JP) {
        printf(" This is %d join prune message \n",
                                 treeInfoBaseRowPtr->treeState);
    }

    /* If state already exists, then the Join message has reached
    the shared tree and the interface from which the message was received is
    entered in the outgoing interface list. If no state exists, a (*,G) entry
    is created, the interface is entered in the outgoing interface list, and
    the Join message is again sent towards the RP. */

    if (treeInfoBaseRowPtr->treeState != ROUTING_PIMSM_SGrpt)
    {
        if (isJoinOrPrune == ROUTING_PIM_JOIN)
        {
            RoutingPimSmJoinPruneTimerInfo    expiryTimer;
            RoutingPimSmJoinPruneTimerInfo    prunePendingTimer;
            clocktype delay;

            if (downstreamListPtr->joinPruneState
                    == PimSm_JoinPrune_PrunePending)
            {
                /* RoutingPimSmCancelPrunePendingTimer */
                prunePendingTimer.srcAddr = srcAddr;
                prunePendingTimer.grpAddr = groupAddr;
                prunePendingTimer.intfIndex = (unsigned int)interfaceId;
                prunePendingTimer.seqNo =
                    downstreamListPtr->prunePendingTimerSeq;
                prunePendingTimer.treeState = treeInfoBaseRowPtr->treeState;

                delay = 0;

                RoutingPimSetTimer(node,
                                   interfaceId,
                                   MSG_ROUTING_PimSmPrunePendingTimerTimeout,
                                   (void*) &prunePendingTimer, delay);

                downstreamListPtr->prunePendingTimerRunning = FALSE;
            }

            /* set the corresponding multicast state in downstream list */
            if (downstreamListPtr->joinPruneState == PimSm_JoinPrune_NoInfo
                && joinPruneHoldTime != -1)
            {
                downstreamListPtr->ETCurrentTime = joinPruneHoldTime;
            }
            else if (joinPruneHoldTime != -1)
            {
                    downstreamListPtr->ETCurrentTime =
                        ((unsigned short)downstreamListPtr->ETCurrentTime >=
                        joinPruneHoldTime) ?
                        downstreamListPtr->ETCurrentTime : joinPruneHoldTime;
            }

#ifdef ADDON_BOEINGFCS
            if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface)
                && !(node->networkData.networkVar->iahepEnabled
                     && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                && (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G
                    || treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG)
                && downstreamListPtr->joinPruneState == PimSm_JoinPrune_NoInfo
                && treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join
                && RoutingPimSmCountValidOutgoingInterfaceCount(node, treeInfoBaseRowPtr) == 0) // a new one is about to be added
            {
                if (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
                    && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh)
                {
                    int counter = RoutingPimSmGroupMemStateIncrementCounter(node,
                                               treeInfoBaseRowPtr->grpAddr,
                                               treeInfoBaseRowPtr->upstream,
                                               treeInfoBaseRowPtr->upInterface);
                    if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                    {
                        printf("\nnode %d Increment counter in RoutingPimSmHandleDownstreamStateMachine. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                           node->nodeId, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, treeInfoBaseRowPtr->upInterface, counter);
                    }
                    if (counter == 1)
                    {
                        MiCesMulticastMeshInitiateMeshFormation(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, pimDataSm->miMulticastMeshTimeout, FALSE);
                    }
                }
            }
#endif // ADDON_BOEINGFCS
            downstreamListPtr->joinPruneState = PimSm_JoinPrune_Join;
            downstreamListPtr->lastJoinReceived = node->getNodeTime();

            if (DEBUG_JP)
            {
                char clockStr[100];
                printf(" setting tree state for downstreamIntf %d \n",
                       downstreamListPtr->interfaceId);
                ctoa(node->getNodeTime(),clockStr);
                printf("Node %u last join received at %s for intf %d\n",
                       node->nodeId, clockStr,
                       downstreamListPtr->interfaceId);

                printf(" Set expiry timer \n");
            }
            if (joinPruneHoldTime != -1)
            {
                /* RoutingPimSmStartsExpiryTimer */
                expiryTimer.srcAddr = srcAddr;
                expiryTimer.grpAddr = groupAddr;
                expiryTimer.intfIndex = interfaceId;
                expiryTimer.treeState = treeInfoBaseRowPtr->treeState;
                expiryTimer.seqNo = downstreamListPtr->expiryTimerSeq + 1;

                /* Note the current expiryTimerSeq */
                downstreamListPtr->expiryTimerSeq++;

                // Update current time to store current time for expiry
                delay = (clocktype) (downstreamListPtr->ETCurrentTime);

                RoutingPimSetTimer(node,
                                   interfaceId,
                                   MSG_ROUTING_PimSmExpiryTimerTimeout,
                                   (void*) &expiryTimer, delay);

            }
            /* check the current assert state & do accordingly */
            if (DEBUG_JP) {
                printf(" associatedSrc %x associatedGrp %x \n",
                       srcAddr, groupAddr);
            }
            if (downstreamListPtr->assertState == PimSm_Assert_ILostAssert)
            {
                downstreamListPtr->assertState = PimSm_Assert_NoInfo;
                RoutingPimSmPerformActionA5(node, groupAddr, srcAddr,
                                downstreamListPtr->interfaceId,
                                treeInfoBaseRowPtr->treeState);
            }

        }   //end of if;
    }   // end of if;
    if (isJoinOrPrune == ROUTING_PIM_PRUNE)
    {
        RoutingPimSmJoinPruneTimerInfo    prunePendingTimer;
        clocktype delay;

        if (DEBUG_JP) {
            printf(" Node %u has join prune state = %d \n",
                   node->nodeId, downstreamListPtr->joinPruneState);
        }

        if ((downstreamListPtr->joinPruneState == PimSm_JoinPrune_Join)
                && (treeInfoBaseRowPtr->treeState != ROUTING_PIMSM_SGrpt))
        {
            downstreamListPtr->joinPruneState = PimSm_JoinPrune_PrunePending;
            downstreamListPtr->lastPruneReceived = node->getNodeTime();
            if (DEBUG_JP)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf(" Node %u :\n", node->nodeId);
                printf(" lastPruneReceived & set PPTimer at %s in intf %d\n",
                       clockStr, downstreamListPtr->interfaceId);
            }


            /* RoutingPimSmStartsPrunePendingTimer */
            downstreamListPtr->prunePendingTimerRunning = TRUE;

            prunePendingTimer.srcAddr =
                srcAddr;
            prunePendingTimer.grpAddr = groupAddr;
            prunePendingTimer.intfIndex = (unsigned int)interfaceId;
            prunePendingTimer.seqNo =
                downstreamListPtr->prunePendingTimerSeq + 1;
            prunePendingTimer.treeState = treeInfoBaseRowPtr->treeState;

            /* Note the current prunePendingTimerSeq */
            downstreamListPtr->prunePendingTimerSeq++;

            /* The PrunePending timer is set to the J/P_Override_Interval if
            router has more than one neighbor on that interface; otherwise
            it is set to zero causing it to expire immediately */

            /* if I hav only one neighbor associated with this interface */
            if (pim->interface[interfaceId].smInterface->neighborList->size == 1)
            {
                delay = 0;
            }
            else
            {
                delay = (clocktype) (ROUTING_PIMSM_OVERRIDE_INTERVAL +
                                      ROUTING_PIMSM_PROPAGATION_DELAY);
            }
            RoutingPimSetTimer(node,
                               interfaceId,
                               MSG_ROUTING_PimSmPrunePendingTimerTimeout,
                               (void*) &prunePendingTimer, delay);
        }

#ifdef CYBER_CORE
        NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;
        if (ip->iahepEnabled && ip->iahepData->nodeType == RED_NODE)
        {
            IAHEPCreateIgmpJoinLeaveMessage(
                node,
                groupAddr,
                IGMP_LEAVE_GROUP_MSG);

            if (IAHEP_DEBUG)
            {
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(groupAddr, addrStr);
                printf("\nRed Node: %d", node->nodeId);
                printf("\tLeaves Group: %s\n", addrStr);
            }
        }
#endif
    }
    if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SGrpt)
    {
        RoutingPimSmJoinPruneTimerInfo    expiryTimer;
        RoutingPimSmJoinPruneTimerInfo    prunePendingTimer;
        clocktype delay;
        switch (downstreamListPtr->joinPruneState)
        {
            case PimSm_JoinPrune_NoInfo:
            {
                // Receive Prune(S,G,rpt)
                if (isJoinOrPrune == ROUTING_PIM_PRUNE)
                {
                    /* The (S,G,rpt) downstream state machine on
                    interface I transitions to the Prune-Pending state.
                    The Expiry Timer (ET) is started and set to the
                    HoldTime from the triggering Join/Prune message.
                    The Prune-Pending Timer is started. It is set to the
                    J/P_Override_Interval (I) if the router has more
                    than one neighbor on that interface; otherwise, it
                    is set to zero, causing it to expire immediately.*/
                    downstreamListPtr->joinPruneState =
                        PimSm_JoinPrune_PrunePending;
                    downstreamListPtr->lastPruneReceived =
                        node->getNodeTime();
                    if (DEBUG_JP)
                    {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);
                        printf(" Node %u :\n", node->nodeId);
                        printf(" lastPruneReceived & set PPTimer at %s"
                            " in intf %d\n",
                           clockStr, downstreamListPtr->interfaceId);
                    }


                    /* RoutingPimSmStartsPrunePendingTimer */
                    downstreamListPtr->prunePendingTimerRunning = TRUE;

                    prunePendingTimer.srcAddr = srcAddr ;
                    prunePendingTimer.grpAddr = groupAddr;
                    prunePendingTimer.intfIndex = (unsigned int)interfaceId;
                    prunePendingTimer.seqNo =
                        downstreamListPtr->prunePendingTimerSeq + 1;

                    prunePendingTimer.treeState = ROUTING_PIMSM_SGrpt ;

                    /* Note the current prunePendingTimerSeq */
                    downstreamListPtr->prunePendingTimerSeq++;

                    /* The PrunePending timer is set to the
                    J/P_Override_Interval if router has more than one
                    neighbor on that interface; otherwise it is set to
                    zero causing it to expire immediately */

                    /* if I hav only one neighbor associated
                    with this interface */
                    if (pim->interface[interfaceId].
                        smInterface->neighborList->size == 1)
                    {
                        delay = 0;
                    }
                    else
                    {
                        delay = (clocktype)
                            (ROUTING_PIMSM_OVERRIDE_INTERVAL +
                                    ROUTING_PIMSM_PROPAGATION_DELAY);
                    }
                    RoutingPimSetTimer(
                                node,
                                interfaceId,
                                MSG_ROUTING_PimSmPrunePendingTimerTimeout,
                                (void*) &prunePendingTimer, delay);

                    /* RoutingPimSmStartsExpiryTimer */
                    expiryTimer.srcAddr = srcAddr;
                    expiryTimer.grpAddr = groupAddr;
                    expiryTimer.intfIndex = (unsigned int)interfaceId;
                    expiryTimer.treeState = treeInfoBaseRowPtr->treeState;
                    expiryTimer.seqNo =
                        downstreamListPtr->expiryTimerSeq + 1;

                    /* Note the current expiryTimerSeq */
                    downstreamListPtr->expiryTimerSeq++;

                    /* The Expiry Timer (ET) is started and set to the
                    HoldTime from the triggering Join/Prune message.*/
                    if (joinPruneHoldTime != -1)
                    {
                        downstreamListPtr->ETCurrentTime =
                            joinPruneHoldTime;
                    }
                    if (joinPruneHoldTime != -1)
                    {
                        delay = (clocktype)
                            (downstreamListPtr->ETCurrentTime);
                        RoutingPimSetTimer(node,
                                       interfaceId,
                                       MSG_ROUTING_PimSmExpiryTimerTimeout,
                                       (void*) &expiryTimer, delay);
                    }
                }
                break;
            }
            case PimSm_JoinPrune_Pruned:
            {
                // Receive Join(S,G,rpt)
                if (isJoinOrPrune == ROUTING_PIM_JOIN)
                {
                    /*
                    The (S,G,rpt) downstream state machine on interface
                    I transitions to NoInfo state.*/
                    downstreamListPtr->joinPruneState =
                                        PimSm_JoinPrune_NoInfo;
                    downstreamListPtr->lastJoinReceived =
                        node->getNodeTime();
                    if (DEBUG_JP)
                    {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);
                        printf(" Node %u :\n", node->nodeId);
                        printf(" lastJoinReceived & cancel PPTimer at %s"
                        " in intf %d\n",
                           clockStr, downstreamListPtr->interfaceId);
                    }
                    // ET and PPT are canceled
                    downstreamListPtr->prunePendingTimerRunning = FALSE;
                    // Cancel expiry timer
                    downstreamListPtr->expiryTimerSeq++;
                }
                // Receive Prune(S,G,rpt)
                if (isJoinOrPrune == ROUTING_PIM_PRUNE)
                {
                     /* The (S,G,rpt) downstream state machine on
                     interface I remains in Prune state.
                     The Expiry Timer (ET) is restarted, set to maximum
                     of its current value and the HoldTime from the
                     triggering Join/Prune message.*/
                     downstreamListPtr->lastPruneReceived =
                        node->getNodeTime();
                     if (DEBUG_JP)
                     {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);
                        printf(" Node %u :\n", node->nodeId);
                        printf("lastPruneReceived & restart expiry timer"
                        "at %s in intf %d\n",
                           clockStr, downstreamListPtr->interfaceId);
                     }
                     // Update current time to store current time for
                     // expiry
                    if (joinPruneHoldTime != -1)
                    {
                        downstreamListPtr->ETCurrentTime =
                        (downstreamListPtr->ETCurrentTime >
                        joinPruneHoldTime) ?
                        downstreamListPtr->ETCurrentTime :
                        joinPruneHoldTime;
                    }
                    delay = (clocktype)
                        (downstreamListPtr->ETCurrentTime);
                    /* RoutingPimSmStartsExpiryTimer */
                    expiryTimer.srcAddr = srcAddr;
                    expiryTimer.grpAddr = groupAddr;
                    expiryTimer.intfIndex = (unsigned int)interfaceId;
                    expiryTimer.treeState = treeInfoBaseRowPtr->treeState;
                    expiryTimer.seqNo =
                        downstreamListPtr->expiryTimerSeq + 1;
                    if (joinPruneHoldTime != -1)
                    {
                        /* Note the current expiryTimerSeq */
                        downstreamListPtr->expiryTimerSeq++;
                        RoutingPimSetTimer(node,
                                   interfaceId,
                                   MSG_ROUTING_PimSmExpiryTimerTimeout,
                                   (void*) &expiryTimer, delay);
                    }
                }
                break;
            }
            case PimSm_JoinPrune_PrunePending:
            {
                //Receive Join(S,G,rpt)
                if (isJoinOrPrune == ROUTING_PIM_JOIN)
                {
                    // The (S,G,rpt) downstream state machine on
                    // interface I transitions to NoInfo state.
                    // ET and PPT are canceled.
                    downstreamListPtr->joinPruneState =
                                            PimSm_JoinPrune_NoInfo;
                    downstreamListPtr->lastJoinReceived =
                            node->getNodeTime();
                    if (DEBUG_JP)
                    {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);
                        printf(" Node %u :\n", node->nodeId);
                        printf(" lastJoinReceived & cancel PPTimer at %s"
                        " in intf %d\n",
                           clockStr, downstreamListPtr->interfaceId);
                    }
                    // ET and PPT are canceled
                    downstreamListPtr->prunePendingTimerRunning = FALSE;
                    // Cancel Expiry timer
                    downstreamListPtr->expiryTimerSeq++;
                }
                break;
            }
            case PimSm_JoinPrune_Temp_Pruned:
            {
                // Receive Prune(S,G,rpt)
                if (isJoinOrPrune == ROUTING_PIM_PRUNE)
                {
                     /* The (S,G,rpt) downstream state machine on
                     interface I transitions back to the
                     Prune state.The Expiry Timer (ET) is
                     restarted, set to maximum
                     of its current value and the HoldTime from the
                     triggering Join/Prune message.*/
                    downstreamListPtr->joinPruneState =
                                        PimSm_JoinPrune_Pruned;
                    downstreamListPtr->lastPruneReceived =
                        node->getNodeTime();
                     if (DEBUG_JP)
                     {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);
                        printf(" Node %u :\n", node->nodeId);
                        printf("lastPruneReceived & restart expiry timer"
                        "at %s in intf %d\n",
                           clockStr, downstreamListPtr->interfaceId);
                     }
                    /* RoutingPimSmStartsExpiryTimer */
                    expiryTimer.srcAddr = srcAddr;
                    expiryTimer.grpAddr = groupAddr;
                    expiryTimer.intfIndex = (unsigned int)interfaceId;
                    expiryTimer.treeState = treeInfoBaseRowPtr->treeState;
                    expiryTimer.seqNo =
                        downstreamListPtr->expiryTimerSeq + 1;

                    /* Note the current expiryTimerSeq */
                    downstreamListPtr->expiryTimerSeq++;
                     // Update current time to store current time for
                     // expiry
                    if (joinPruneHoldTime != -1)
                    {
                        downstreamListPtr->ETCurrentTime =
                        (downstreamListPtr->ETCurrentTime >
                        joinPruneHoldTime) ?
                        downstreamListPtr->ETCurrentTime :
                        joinPruneHoldTime;
                    }
                    if (joinPruneHoldTime != -1)
                    {
                        delay = (clocktype)
                                (downstreamListPtr->ETCurrentTime);
                        RoutingPimSetTimer(node,
                                   interfaceId,
                                   MSG_ROUTING_PimSmExpiryTimerTimeout,
                                   (void*) &expiryTimer, delay);
                    }
                }
                // End of Message
                if (isJoinOrPrune == ROUTING_PIM_END_OF_MESSAGE)
                {
                    /* The (S,G,rpt) downstream state machine on
                    interface I transitions to the NoInfo
                    state. ET is canceled.*/
                    downstreamListPtr->joinPruneState =
                                        PimSm_JoinPrune_NoInfo;
                    downstreamListPtr->expiryTimerSeq++;
                }
                break;
            }
            case PimSm_JoinPrune_Temp_PrunePending:
            {
                // Receive Prune(S,G,rpt)
                if (isJoinOrPrune == ROUTING_PIM_PRUNE)
                {
                     /* The (S,G,rpt) downstream state machine on
                     interface I transitions back to the
                     Prune-Pending state.The Expiry Timer (ET) is
                     restarted, set to maximum
                     of its current value and the HoldTime from the
                     triggering Join/Prune message.*/
                    downstreamListPtr->joinPruneState =
                                        PimSm_JoinPrune_PrunePending;
                    downstreamListPtr->lastPruneReceived =
                        node->getNodeTime();
                     if (DEBUG_JP)
                     {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);
                        printf(" Node %u :\n", node->nodeId);
                        printf("lastPruneReceived & restart expiry timer"
                        "at %s in intf %d\n",
                           clockStr, downstreamListPtr->interfaceId);
                     }
                     /* RoutingPimSmStartsExpiryTimer */
                    expiryTimer.srcAddr = srcAddr;
                    expiryTimer.grpAddr = groupAddr;
                    expiryTimer.intfIndex = (unsigned int)interfaceId;
                    expiryTimer.treeState = treeInfoBaseRowPtr->treeState;
                    expiryTimer.seqNo =
                        downstreamListPtr->expiryTimerSeq + 1;

                    /* Note the current expiryTimerSeq */
                    downstreamListPtr->expiryTimerSeq++;
                     // Update current time to store current time for
                     // expiry
                    if (joinPruneHoldTime != -1)
                    {
                        downstreamListPtr->ETCurrentTime =
                        (downstreamListPtr->ETCurrentTime >
                        joinPruneHoldTime) ?
                        downstreamListPtr->ETCurrentTime :
                        joinPruneHoldTime;
                    }
                    if (joinPruneHoldTime != -1)
                    {
                        delay = (clocktype)
                                (downstreamListPtr->ETCurrentTime);
                        RoutingPimSetTimer(node,
                                   interfaceId,
                                   MSG_ROUTING_PimSmExpiryTimerTimeout,
                                   (void*) &expiryTimer, delay);
                    }
                }
                // End of Message
                if (isJoinOrPrune == ROUTING_PIM_END_OF_MESSAGE)
                {
                    /* The (S,G,rpt) downstream state machine on
                    interface I transitions to the NoInfo state. ET and
                    PPT are canceled.*/
                    downstreamListPtr->joinPruneState =
                                        PimSm_JoinPrune_NoInfo;
                    downstreamListPtr->prunePendingTimerRunning = FALSE;
                    /*downstreamListPtr->lastJoinReceived =
                            node->getNodeTime();*/
                    downstreamListPtr->expiryTimerSeq++;
                }
                break;
            }
            default:
            {
#ifdef EXATA
                if (!node->partitionData->isEmulationMode)
                {
                    ERROR_Assert(FALSE, "Unknown downstream JOIN PRUNE"
                        "state \n");
                }
#else
                ERROR_Assert(FALSE, "Unknown downstream JOIN PRUNE"
                    "state \n");
#endif
                return;
            }
        }
    }

    if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G)
    {
        // Receive Join(*,G). Check if any Prune state or Prune Pending
        // state
        if (isJoinOrPrune == ROUTING_PIM_JOIN)
        {
            /* it checks to see if any SGrpt state with same group
            address */
            RoutingPimSmTreeInformationBase* treeInfoBase =
                &pimDataSm->treeInfoBase;
            RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;
            TreeInfoRowMap* rowPtrMap = treeInfoBase->rowPtrMap;
            TreeInfoRowMapIterator mapIterator;

            /* Search treeInfo Base for the desired match */
            for (mapIterator = rowPtrMap->begin();
                 mapIterator != rowPtrMap->end();
                 mapIterator++)
            {
                rowPtr = mapIterator->second;

                if (DEBUG_ERRORS) {
                    printf("   checking the treeInfo Base entry \n");
                }

                /* check for tree state and grp address */
                if (rowPtr->treeState == ROUTING_PIMSM_SGrpt
                    && rowPtr->grpAddr == groupAddr)
                {
                    downstreamListPtr = RoutingPimSmSetDownstreamList(
                                            node,
                                            interfaceId,
                                            rowPtr);

                    switch (downstreamListPtr->joinPruneState)
                    {
                        case PimSm_JoinPrune_Pruned:
                        {
                            /* The (S,G,rpt) downstream state machine on
                            interface I transitions to PruneTmp state whilst
                            the remainder of the compound Join/Prune message
                            containing the Join(*,G) is processed.*/
                            downstreamListPtr->joinPruneState =
                                                PimSm_JoinPrune_Temp_Pruned;
                            break;
                        }
                        case PimSm_JoinPrune_PrunePending:
                        {
                            /* The (S,G,rpt) downstream state machine on
                            interface I transitions to Prune-Pending-Tmp
                            state
                            whilst the remainder of the compound Join/Prune
                            message containing the Join(*,G) is processed.*/
                            downstreamListPtr->joinPruneState =
                                        PimSm_JoinPrune_Temp_PrunePending;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
            }
        }
    }
}

/*
*   FUNCTION  : IsInterfaceInDownstreamList
*   PURPOSE   : Check if the interfaceId is already included in the
*                downstreamList of PimSm treeInfo Base
*   RETURN    : RotingPimSmDownstream* .
*/
RoutingPimSmDownstream*
IsInterfaceInPimSmDownstreamList(Node* node,
                                 RoutingPimSmTreeInfoBaseRow* entry,
                                 int interfaceIndex)
{
    ListItem* tempListItem;

    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmTreeInformationBase* treeInfo = &pimDataSm->treeInfoBase;

    if (DEBUG_ERRORS) {
        printf ("treeInfo Base for node %u has %d entry\n",
                node->nodeId, treeInfo->numEntries);
    }

    tempListItem = entry->downstream->first;

    while (tempListItem)
    {
        RoutingPimSmDownstream* downStream = (RoutingPimSmDownstream*)
                                             tempListItem->data;

        if (downStream->interfaceId == interfaceIndex)
        {
            if (DEBUG_JP) {
                printf(" the interfaceId %d is already present \n",
                       downStream->interfaceId);
            }
            return downStream;
        }

        tempListItem = tempListItem->next;
    }//End while

    if (DEBUG_ERRORS) {
        printf("    No such interface In Downstream List\n");
    }

    return NULL;
}

/*
*  FUNCTION    : RoutingPimSmSendRegisterPacket()
*  PURPOSE     : Encapsulating data packets in the Register Tunnel
*  RETURN VALUE: NULL
*/
BOOL
RoutingPimSmSendRegisterPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmRegisterPacket* registerPkt;
    TosType priority;
    NodeAddress nextHop;
    int outInterface;

    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(
        node,
        destinationAddress,
        &outInterface,
        &nextHop);

    if (nextHop == (unsigned)NETWORK_UNREACHABLE)
    {
#ifdef ADDON_BOEINGFCS
        if (MulticastCesRpimActiveOnNode(node))
        {
            if (NetworkIpIsMyIP(node, destinationAddress))
            {
                nextHop = destinationAddress;
                outInterface = NetworkIpGetInterfaceIndexFromAddress(node,
                    destinationAddress);
            }
            else
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);

                if (DEBUG_ERRORS) {
                    printf(" Node %u: has no route to RP  %x at %s \n",
                        node->nodeId, destinationAddress, clockStr);

                    NetworkPrintForwardingTable(node);
                }
                pimDataSm->stats.numOfRegisteredPacketDropped++;
                return FALSE;
            }
        }
        else
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);

            if (DEBUG_ERRORS) {
                printf(" Node %u: has no route to RP  %x at %s \n",
                    node->nodeId, destinationAddress, clockStr);

                NetworkPrintForwardingTable(node);
            }
            pimDataSm->stats.numOfRegisteredPacketDropped++;
            return FALSE;
        }
#endif
    }

    if (nextHop == 0)
    {
        nextHop = destinationAddress ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           destinationAddress,
                                           &nextHop,
                                           &outInterface);
    }

    if (outInterface == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,destinationAddress);
        }
        pimDataSm->stats.numDataPktDiscard++;
        return FALSE;
    }

    /* Set the Register Tunnel is an interface with a smaller MTU
    than the underlying IP interface towards the RP. The encapsulating
    DR may perform Path-MTU Discovery to the RP to determine the effective
    MTU of the tunnel.This smaller MTU takes both the outer IP header and
    the PIM register header overhead into account.If a multicast packet is
    fragmented on the way into the Register Tunnel, each fragment is
    encapsulated individually so contains IP, PIM, and inner IP headers.  */

    /* the TTL of the original data packet is decremented before
     * it is encapsulated in the Register Tunnel. */
    /* The IP ECN bits should be copied from the original packet to the IP

    header of the encapsulating packet.  */
    /* The Diffserv Code Point (DSCP) should be copied from the original
    packet to the IP header of the encapsulating packet.It MAY be set
    independently by the encapsulating router, based upon static
    configuration or traffic classification. */

    // Add protocol specific header to the data packet
    MESSAGE_AddHeader(node, msg,
                      sizeof(RoutingPimSmRegisterPacket), TRACE_PIM);


    registerPkt = (RoutingPimSmRegisterPacket*)
                  MESSAGE_ReturnPacket(msg);

    /* set the common header */
    RoutingPimCreateCommonHeader(&registerPkt->commonHeader,
                                 ROUTING_PIM_REGISTER);


    /*
     The Border bit: If the router is a DR for a source that it is
     directly connected to, it sets the B bit to 0. If the router is a
     PMBR for a source in a directly connected cloud, it sets the B bit
     to 1.
    */
    PimSmRegisterPacketSetBorder(&(registerPkt->rpSmRegPkt), 0);

    /* The Null-Register bit. Set to 1 by a DR that is probing the RP
     before expiring its local Register-Suppression timer. Set to 0
     otherwise.
    */
    PimSmRegisterPacketSetNullReg(&(registerPkt->rpSmRegPkt), 0);
    PimSmRegisterPacketSetReserved(&(registerPkt->rpSmRegPkt), 0);

    priority = IPTOS_PREC_INTERNETCONTROL;

#ifdef ADDON_BOEINGFCS
    if (MulticastCesRpimActiveOnInterface(node, outInterface))
    {
        priority = IPTOS_PREC_PRIORITY;
    }
#endif

    if (NetworkIpIsMyIP(node, destinationAddress))
    {
        RoutingPimSmHandleRegisteredPacket(node,
                                           msg,
                                           sourceAddress,
                                           outInterface);
    }
    else
    {
        /* Set the ip header */
        NetworkIpSendRawMessageToMacLayer(
        node,
        MESSAGE_Duplicate(node, msg),
        sourceAddress,
        destinationAddress,
        priority,
        IPPROTO_PIM,
        ipHeader->ip_ttl - 1,
        outInterface,
        nextHop);
    }

    pimDataSm->stats.numOfRegisteredPacketForwarded++;
#ifdef ADDON_DB
    pimDataSm->smSummaryStats.m_NumRegisterSent++;
#endif
    return TRUE ;
}


/*
*  FUNCTION         : RoutingPimSmHandleRegisteredPacket()
*  PURPOSE          : Check if this Registered Pkt need to be processed
*  RETURN VALUE     : NULL
*/
void
RoutingPimSmHandleRegisteredPacket(Node* node, Message* msg,
                                   NodeAddress srcAddr, int incomingIntf)
{
    NodeAddress mcastSource;
    NodeAddress groupAddr;
    NodeAddress RPAddr;
    int i;
    int mcastSrcIntf;
    NodeAddress nextHop;
    clocktype keepaliveperiod,RP_keepaliveperiod;

    BOOL IamRegPktSrc = FALSE;
    NetworkDataIp* ip = (NetworkDataIp*)node->networkData.networkVar;

    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    if ((msg->numberOfHeaders) &&
        (msg->headerProtocols[msg->numberOfHeaders-1] == TRACE_FORWARD))
    {
        MESSAGE_RemoveHeader(node, msg, 0, TRACE_FORWARD);
        /*Recreate the headers as they would have been lost
          if received from external entity*/
        msg->packet += sizeof(RoutingPimSmRegisterPacket);
        msg->packet += sizeof(IpHeaderType);
        msg->packet += sizeof(TransportUdpHeader);
        msg->packetSize = msg->packetSize
                         - sizeof(RoutingPimSmRegisterPacket)
                         - sizeof(IpHeaderType)
                         - sizeof(TransportUdpHeader);
        MESSAGE_AddHeader(node, msg, sizeof(TransportUdpHeader), TRACE_UDP);
        MESSAGE_AddHeader(node, msg, sizeof(IpHeaderType), TRACE_IP);
        MESSAGE_AddHeader(node, msg,
                      sizeof(RoutingPimSmRegisterPacket), TRACE_PIM);
    }
    RoutingPimSmRegisterPacket* registerPkt =
        (RoutingPimSmRegisterPacket*) MESSAGE_ReturnPacket(msg);
    if (((unsigned int)msg->packetSize) < sizeof(IpHeaderType) +
          sizeof (RoutingPimSmRegisterPacket))
    {
        if (DEBUG_ERRORS)
        {
            printf("\n Incorrect Register packet received");
        }
        return;
    }
    IpHeaderType* ipHeader = (IpHeaderType*)((char*)registerPkt
                             + sizeof(RoutingPimSmRegisterPacket));
    if (ipHeader->ip_sum <= 0 && PimSmRegisterPacketGetNullReg
        (registerPkt->rpSmRegPkt))
    {
        if (DEBUG_ERRORS)
        {
            printf("\n Incorrect checksum received discarding Register");
        }
        return;
    }
    if (DEBUG_ERRORS)
    {
        printf(" it has ipHeader->ip_src  = %x \n", ipHeader->ip_src);
        printf(" it has ipHeader->ip_dst  = %x \n", ipHeader->ip_dst);
        printf(" it has ipHeader->ip_ttl  = %d \n", ipHeader->ip_ttl);
        printf(" it has ipHeader->ip_protocol  = %d \n", ipHeader->ip_p);
        printf(" it has ipHeader->ip_length  = %d \n",
            IpHeaderGetIpLength(ipHeader->ip_v_hl_tos_len));
    }

    mcastSource = ipHeader->ip_src;
    groupAddr = ipHeader->ip_dst;
    bool inSSMRange = FALSE;
    if (ip->isSSMEnable && Igmpv3IsSSMAddress(node, groupAddr))
    {
        inSSMRange = TRUE;
    }

    if (DEBUG_ERRORS) {
        NetworkPrintForwardingTable(node);
    }

    /* select the outInterface & nextHop to reach mcastSource */
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, mcastSource,
            &mcastSrcIntf, &nextHop);

#ifdef ADDON_BOEINGFCS
    if (nextHop == (unsigned)NETWORK_UNREACHABLE)
    {
        if (MulticastCesRpimActiveOnNode(node))
        {
            if (NetworkIpIsMyIP(node, mcastSource))
            {
                nextHop = mcastSource;
                mcastSrcIntf = NetworkIpGetInterfaceIndexFromAddress(node,
                    mcastSource);
            }
        }
    }
#endif

    if (nextHop == 0)
    {
        nextHop = mcastSource ;
        //mcastSrcIntf = incomingIntf;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           mcastSource,
                                           &nextHop,
                                           &mcastSrcIntf);
    }

    if (mcastSrcIntf == NETWORK_UNREACHABLE)
    {
        if (DEBUG)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,mcastSource);
        }
        return;
    }

    /*
     *  if outer.dst (encapsulated_Hdr_Dest) is not one of my addresses drop
     *  packet silently. This should not happen if the lower layer is working
     */

    /* check if I am the source of the register pkt */
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (pim->interface[i].ipAddress == srcAddr)
        {
            IamRegPktSrc = TRUE;
        }
    }

    RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);
    BOOL IamRP = FALSE;
    // Check if any of its interface address is same as RP
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (pim->interface[i].ipAddress == RPAddr)
        {
            IamRP = TRUE;

#ifdef CYBER_CORE
            if (!RoutingPimSmIsDefaultBlackMulticastGroup(node, groupAddr) &&
                !RoutingPimSmIsDefaultRedMulticastGroup(node, groupAddr))
            {
                MESSAGE_AddInfo(node, msg, sizeof(BOOL), INFO_TYPE_RPProcessed);
                memcpy(MESSAGE_ReturnInfo(msg, INFO_TYPE_RPProcessed),
                   &IamRP,
                   sizeof(BOOL));
            }
#endif

            break;
        }
    }
    if (IamRP)
    {
        RoutingPimSmKeepAliveTimerInfo    timerInfo;
        BOOL sentRegisterStop = FALSE;
        clocktype delay;
        RoutingPimSmTreeInfoBaseRow   * treeInfoBaseRowPtr = NULL;

        /* PRESERVE THE S, G STATE AT RP */
        /* search for this group in multicast treeInfo Base */
        treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                             (node, groupAddr, mcastSource,
                              ROUTING_PIMSM_SG);


        /* if does not exist create one */

        if (treeInfoBaseRowPtr == NULL)
        {
            treeInfoBaseRowPtr = RoutingPimSmSetMulticastTreeInfoBase(node,
                                 groupAddr,
                                 mcastSource,
                                 ROUTING_PIMSM_SG);
            if (DEBUG) {
                printf("\nNode %u:it is the RP, so modify upstream\n",
                       node->nodeId);
            }
        }
        /*if (inherited_olist(S, G) is NULL) or SPTbit(S, G) set */
        if (DEBUG_FORWARD_PACKET) {
            printf("\nNode %u: checking for registerStop or forwarding pkt"
                "\n", node->nodeId);
            printf("SPTbit is %d \n", treeInfoBaseRowPtr->SPTbit);
        }
        if ((inSSMRange) ||
            ( (RoutingPimSmInheritedOutListSG(node, groupAddr,
                        mcastSource, treeInfoBaseRowPtr) == 0)
                || (treeInfoBaseRowPtr->SPTbit == TRUE ) )
                && (!IamRegPktSrc)
#ifdef CYBER_CORE
                // for default black group keep using register packet
                && (!RoutingPimSmIsDefaultBlackMulticastGroup(node, groupAddr))
#endif
           )
        {
            RoutingPimSmSendRegisterStopMessage(node, groupAddr,
                                    mcastSource, srcAddr, incomingIntf);
            sentRegisterStop = TRUE;
#ifdef ADDON_DB
            HandleNetworkDBEventsForPimSm(
                node,
                msg,
                incomingIntf,
                "NetworkPacketDrop",
                "Register Stop Sent",
                0,
                0,
                0,
                0);

#endif
        }
        if (inSSMRange)
        {
            // if grp addr is in ssm range do no further register processing
            return;
        }
        /* RP should preserve (S, G)state that was created in response to
            a Register message for at least (3*  Register_Suppression_Time),
            otherwise the RP may stop joining (S, G) before the DR for S has
            restarted sending registers.Traffic would then be interrupted
            until the Register-Stop timer expires at the DR.

            However, at the RP, the keepalive period must be at least the
            Register_Suppression_Time, or the RP may timeout the (S,G)
            state before the next Null-Register arrives. Thus,the KAT(S,G)
            is set to  max(Keepalive_Period, RP_Keepalive_Period)
            when a Register-Stop is sent.*/

        if (sentRegisterStop)
        {
            RP_keepaliveperiod =
                (clocktype) (ROUTING_PIM_REGISTER_SUPP_CONSTANT *
                       pimDataSm->routingPimSmRegisterSupressionTime ) +
                       (clocktype) pimDataSm->routingPimSmRegisterProbeTime;
            keepaliveperiod = (clocktype)
                              (pimDataSm->routingPimSmKeepaliveTimeout);
            /* Set KAT(S,G) to max(Keepalive_Period, RP_Keepalive_Period)
             * when a Register-Stop is sent.*/
            delay = ((keepaliveperiod >= RP_keepaliveperiod)
                     ? keepaliveperiod : RP_keepaliveperiod);

        }
        else
        {
            delay = (clocktype) (pimDataSm->routingPimSmKeepaliveTimeout);
        }

        if (treeInfoBaseRowPtr->keepAliveExpiryTimer == 0)
        {
            timerInfo.srcAddr = mcastSource;
            timerInfo.grpAddr = groupAddr;
            timerInfo.intfIndex = (unsigned int)mcastSrcIntf;
            timerInfo.treeState = ROUTING_PIMSM_SG;

            /* Note the current KeepAliveTimerSeq */

            RoutingPimSetTimer(node,
                               incomingIntf,
                               MSG_ROUTING_PimSmKeepAliveTimeOut,
                               (void*) &timerInfo,
                               delay);

        }

        treeInfoBaseRowPtr->keepAliveExpiryTimer = node->getNodeTime() + delay;
        /* it will initiate an (S, G) source-specificJoin towards S */
        if (DEBUG_JP)
        {
            printf("Node %u (RP) send JOIN to intf %d for McastSrc %x to %x "
                  "\n",node->nodeId, mcastSrcIntf, mcastSource, nextHop);
        }
        RoutingPimSmHandleUpstreamStateMachine(node,
                                               mcastSource,
                                               incomingIntf, groupAddr,
                                               treeInfoBaseRowPtr);
#ifdef CYBER_CORE
         NetworkDataIp *ip = (NetworkDataIp *)
                                    node->networkData.networkVar;
        if (ip->iahepEnabled &&
           ip->iahepData->nodeType == RED_NODE
           )
        {
            // if the RP send (S,G) JOIN,
            // and this is a red node,
            // need to send a IGMP JOIN with (S,G) info to black side
            // so black side can setup SPT for (S',G')

            RoutingPimSmIAHEPInitIgmpMsgToBlackNetwork(
                node, mcastSource,
                groupAddr,
                IGMP_MEMBERSHIP_REPORT_MSG);
        }

#endif

        if (treeInfoBaseRowPtr->SPTbit == FALSE &&
            !(PimSmRegisterPacketGetNullReg(registerPkt->rpSmRegPkt)))
        {
            BOOL packetWasRouted = FALSE;
            LinkedList* outIntfList = NULL;
            ListItem* listItem;
            /* retrive the original mcastDataPkt from registerPkt */
            MESSAGE_RemoveHeader(node, msg,
                                 sizeof(RoutingPimSmRegisterPacket),
                                 TRACE_PIM);

            /* decrease the value os ip Header by 1 */
            ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
            ipHeader->ip_ttl = (unsigned char) (ipHeader->ip_ttl - 1);
            /*
             *  pass the inner mcastPacket to the normal
             *  forwarding path for forwarding on the (*, G) tree.
             */
            if (DEBUG_FORWARD_PACKET)
            {
                printf("Node %u:ready to forward decapsulatetPkt as if",
                       node->nodeId);
            }
            ListInit(node, &outIntfList);
            ListItem* tempListItem;
            /*Time to make oiflist = inherited_olist(S, G, rpt)  */
            if (RoutingPimSmInheritedOutListSGrpt(node, groupAddr,
                        mcastSource, treeInfoBaseRowPtr) != 0)
            {
                tempListItem =
                    treeInfoBaseRowPtr->inheritedOutIntfSGrpt->first;

                while (tempListItem)
                {
                    unsigned int* inheritedIntfPtr = (unsigned int*)
                                    MEM_malloc(sizeof(unsigned int));

                    memcpy(inheritedIntfPtr,
                        tempListItem->data, sizeof(int));

                    if (DEBUG) {
                     printf("Node %u: add intf %d from "
                           "inheritedSGrptList\n",
                           node->nodeId,* inheritedIntfPtr);
                    }

                    if (IsInterfaceInThisPimSmList(outIntfList,
                                         *inheritedIntfPtr) == FALSE)
                    {
                        ListInsert(node, outIntfList, 0,
                               (void*) inheritedIntfPtr);
                    }
                    else
                    {
                        if (DEBUG) {
                            printf("No need to add in this list"
                               "since already there \n");
                        }
                        MEM_free(inheritedIntfPtr);
                    }
                    tempListItem = tempListItem->next;
                }
            }
            if (outIntfList->size == 0)
            {
                pimDataSm->stats.numDataPktDiscard++;
                if (DEBUG_FORWARD_PACKET)
                {
                    char clockStr[100];
                    TIME_PrintClockInSecond(node->getNodeTime(),
                                            clockStr,
                                            node);
                    printf("\n");
                    printf("\n Time %s : ",clockStr);
                    printf("Node %d received Register packet and no "
                            "outgoing inetrface present. "
                            " pktdiscard = %d\n", node->nodeId,
                            pimDataSm->stats.numDataPktDiscard);
                    printf("Data packet discarded");
                }

#ifdef ADDON_DB
                HandleNetworkDBEventsForPimSm(
                    node,
                    msg,
                    incomingIntf,
                    "NetworkPacketDrop",
                    "Register Stop Sent",
                    0,
                    0,
                    0,
                    0);
#endif
            }
            else
            {
                /* Forward Packet */
                listItem = outIntfList->first;
                while (listItem)
                {
                    unsigned int* inheritedIntfPtr =
                            (unsigned int*)listItem->data;
                    int inheritedIntf = (int)(*inheritedIntfPtr);

                    NodeAddress nextNode =
                        NetworkIpGetInterfaceBroadcastAddress(node,
                                                      inheritedIntf);
                    Message* tempMsg = MESSAGE_Duplicate(node, msg);

                    ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

                    if (NetworkIpIsMyIP(node, groupAddr))
                    {
                        DeliverPacket(node,
                                      MESSAGE_Duplicate(node, tempMsg),
                                      inheritedIntf,
                                      RPAddr);
                    }
                    NetworkIpSendPacketToMacLayer(
                                                    node,
                                                    tempMsg,
                                                    inheritedIntf,
                                                    nextNode);
                    if (DEBUG_FORWARD_PACKET)
                    {
                        char clockStr[100];
                        TIME_PrintClockInSecond(node->getNodeTime(),
                                clockStr, node);
                        printf("\n Time %s : ",clockStr);
                        printf("Node %d received Register packet and"
                            " forwarding packet through interface %d",
                                node->nodeId,
                               inheritedIntf);
                    }
                    listItem = listItem->next;
                }
                pimDataSm->stats.numOfRegisterDataPacketForwarded++;
            }
            packetWasRouted = TRUE;
            ListFree(node, outIntfList, FALSE);
        }//pkt.NullRegisterBit
    }
}


/*
*  FUNCTION         : RoutingPimSmPrintTreeInfoBase()
*  PURPOSE          : Routing PimSm Print treeInfo Base
*  RETURN VALUE     : NULL
*/
void
RoutingPimSmPrintTreeInfoBase(Node* node)
{
    FILE* fp;
    char clockTime[100];
    char srcNetAddr[100];
    char groupAddr[100];
    char upStream[100];

    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    RoutingPimSmTreeInformationBase* treeInfo = &pimDataSm->treeInfoBase;
    TreeInfoRowMap* rowPtrMap = treeInfo->rowPtrMap;
    RoutingPimSmTreeInfoBaseRow     *rowPtr;
    TreeInfoRowMapIterator mapIterator;

    ctoa(node->getNodeTime(), clockTime);

    fp = fopen("TreeInfoBase.txt", "a+");

    if (fp == NULL)
    {
        fprintf(stdout, "Couldn't open file TreeInfoBase.txt\n");
        return;
    }
    if (DEBUG_ERRORS) {
        printf ("treeInfo Base for node %u has %d entry at %s\n",
                node->nodeId, treeInfo->numEntries, clockTime);
    }

    fprintf (fp, "treeInfo Base for node %u has %d entry at %s\n",
             node->nodeId, treeInfo->numEntries, clockTime);

    fprintf(fp, "\n    Group    Source  upStream  treSt  upJPSt  SPT  "
            "Intf   JPSt   asrtSt\n");
    fprintf(fp, "* **********************************************"
            "*******************************\n");

    if (treeInfo->numEntries != 0)
    {
        for (mapIterator = rowPtrMap->begin();
             mapIterator != rowPtrMap->end();
             mapIterator++)
        {
            rowPtr = mapIterator->second;
            ListItem* tempListItem;

            IO_ConvertIpAddressToString(rowPtr->grpAddr,
                                        groupAddr);
            IO_ConvertIpAddressToString(rowPtr->srcAddr,
                                        srcNetAddr);
            IO_ConvertIpAddressToString(rowPtr->upstream,
                                        upStream);

            fprintf (fp, "%10s %9s %9s  %2d     %2d    %2d\n",
                     groupAddr, srcNetAddr, upStream,
                     rowPtr->treeState,
                     rowPtr->upstreamState,
                     rowPtr->SPTbit);

            tempListItem = rowPtr->downstream->first;

            while (tempListItem)
            {
                RoutingPimSmDownstream* downstream =
                    (RoutingPimSmDownstream*) tempListItem->data;

                char interfaceAddress[100];

                IO_ConvertIpAddressToString(downstream->intfAddr,
                                            interfaceAddress);

                fprintf(fp, "   %50d   %3d    %3d\n",
                        downstream->interfaceId,
                        downstream->joinPruneState,
                        downstream->assertState);

                tempListItem = tempListItem->next;
            }//End while

        }
    }
    else
    {
        fprintf(fp, "    No Item In R_Table\n");
    }

    fprintf (fp, "\n\n");
    fclose(fp);
}

/*
*  FUNCTION    : RoutingPimSmHandleUpstreamStateMachine()
*  PURPOSE     : A PIM Join/Prune message modifies the upstream state machine
*                for all associated Joined and Pruned sources for each group.
*  RETURN VALUE: NULL
*/

void
RoutingPimSmHandleUpstreamStateMachine(Node* node, NodeAddress srcAddr,
        int interfaceId, NodeAddress groupAddr,
        RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr,
        clocktype joinPruneHoldTime,
        BOOL suppressJoinPrune)
{
    PimData* pim = (PimData*)NetworkIpGetMulticastRoutingProtocol(node,
                                                     MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    bool ignoreProcessing = FALSE;
    NetworkDataIp* ip = (NetworkDataIp*)node->networkData.networkVar;
    if (pim)
    {
        if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
        {
            pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
        }
        else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
        {
            pimDataSm = (PimDataSm*) pim->pimModePtr;
        }
        if (pimDataSm && ip->isSSMEnable)
        {
            if (Igmpv3IsSSMAddress(node, groupAddr))
            {
                if (DEBUG_JP)
                {
                    ERROR_ReportWarning("Ignoring upstream JOIN(*,G) process"
                        " as SSM router received SSM group range join request."
                        " For SSM range group address, only JOIN(S,G) will be"
                        " created.\n");
                }
                ignoreProcessing = TRUE;
            }
        }
    }
    switch (treeInfoBaseRowPtr->treeState)
    {
        case ROUTING_PIMSM_G:
        {
            if (!ignoreProcessing)
            {
                RoutingPimSmHandleGUpstreamStateMachine(node,
                                                        srcAddr,
                                                        interfaceId,
                                                        groupAddr,
                                                        joinPruneHoldTime,
                                                        treeInfoBaseRowPtr,
                                                        suppressJoinPrune);
            }
            break;
        }
        case ROUTING_PIMSM_SG:
        {
            RoutingPimSmHandleSGUpstreamStateMachine(node,
                                                     srcAddr,
                                                     interfaceId,
                                                     groupAddr,
                                                     joinPruneHoldTime,
                                                     treeInfoBaseRowPtr,
                                                     suppressJoinPrune);
            break;
        }
        case ROUTING_PIMSM_SGrpt:
        {
            if ((!ignoreProcessing) && (!suppressJoinPrune))
            {
                RoutingPimSmHandleSGrptUpstreamStateMachine(node,
                                                            srcAddr,
                                                            groupAddr,
                                                            joinPruneHoldTime,
                                                            treeInfoBaseRowPtr);
            }
            break;
        }

        default:
        {
#ifdef EXATA
            if (!node->partitionData->isEmulationMode)
            {
                ERROR_Assert(FALSE, "Unknown tree State in PIMSM JOIN_PRUNE\n");
            }
#else
            ERROR_Assert(FALSE, "Unknown tree State in PIMSM JOIN_PRUNE\n");
#endif
            break;
        }
    }
}


/*
*  FUNCTION    : RoutingPimSmHandleGUpstreamStateMachine()
*  PURPOSE     : PIM G Join/Prune message modifies the upstream state machine
*                for all associated Joined and Pruned sources for each group.
*  RETURN VALUE: NULL
*/

void
RoutingPimSmHandleGUpstreamStateMachine(Node* node, NodeAddress srcAddr,
                    int interfaceId, NodeAddress groupAddr,
                    clocktype joinPruneHoldTime,
                    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr,
                    BOOL suppressJoinPrune)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    NodeAddress RPAddr;
    NodeAddress nextHop;
    int outInterface;
    RoutingPimSmJoinPruneType  joinPruneType = ROUTING_PIMSM_G_JOIN_PRUNE;
    RoutingPimSmMulticastTreeState  treeState = ROUTING_PIMSM_G;

    if (treeInfoBaseRowPtr == NULL)
    {
#ifdef EXATA
       if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
        }
#else
        ERROR_Assert(FALSE,
            "JoinPrune_Pkt received so entry must exist\n");
#endif
        return;
    }

    /* check the RP For this group & find associated nextHop*/
    RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, RPAddr,
            &outInterface, &nextHop);

    if (nextHop == 0)
    {
        nextHop = RPAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHop,
                                           &outInterface);
    }

    if (outInterface == NETWORK_UNREACHABLE)
    {
        if (DEBUG_JP)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,RPAddr);
        }
        return;
    }

#ifdef ADDON_BOEINGFCS
    if (outInterface < 0)
    {
        return;
    }
#endif

    /* JoinDesired is true when the router has forwarding state
    that would cause it to forward traffic for G using shared tree
    state. Although JoinDesired is true, the router's sending of a
    Join message may be suppressed by another router sending
    a Join onto the upstream interface. */

    if (DEBUG_JP) {
        printf("Node %u: This is G join prune message \n",
               node->nodeId);
        printf(" Node %u handle upstremm Interface %d \n",
               node->nodeId, outInterface);
        printf(" it has upstream join prune state %d \n",
               treeInfoBaseRowPtr->upstreamState);
    }

    if (treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_NotJoin)
    {
        /* if JoinDesired(*, G) in that RPF interface is true */
        if (RoutingPimSmJoinDesiredG(node, groupAddr, srcAddr, outInterface))
        {
            RoutingPimSmJoinPruneTimerInfo    joinTimer;
            clocktype delay;

            if (DEBUG_JP)
            {
                printf(" Set Join-Prune State to join \n");
                printf(" Routing PimSm Send %d Join Packet \n",
                       joinPruneType);
                printf(" Routing PimSm Set Timer to t_periodic \n");
                printf("Node %u:Set UpstreamState for %d treeState \
                       to join \n", node->nodeId, treeState);
                printf("Node %u: set upatream join \n", node->nodeId);
            }

#ifdef ADDON_BOEINGFCS
            if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface)
                && !(node->networkData.networkVar->iahepEnabled
                     && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                && RoutingPimSmCountValidOutgoingInterfaceCount(node, treeInfoBaseRowPtr) > 0)
            {
                if (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
                    && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh)
                {
                    int counter = RoutingPimSmGroupMemStateIncrementCounter(node,
                                                                            treeInfoBaseRowPtr->grpAddr,
                                                                            treeInfoBaseRowPtr->upstream,
                                                                            treeInfoBaseRowPtr->upInterface);
                    if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                    {
                        printf("\nnode %d Increment counter in RoutingPimSmHandleGUpstreamStateMachine. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                           node->nodeId, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, treeInfoBaseRowPtr->upInterface, counter);
                    }
                    if (counter == 1)
                    {
                        MiCesMulticastMeshInitiateMeshFormation(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, pimDataSm->miMulticastMeshTimeout, FALSE);
                    }
                }
            }
#endif // ADDON_BOEINGFCS
            if (!suppressJoinPrune)
            {
                treeInfoBaseRowPtr->upstreamState = PimSm_JoinPrune_Join;

#ifdef ADDON_DB
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = groupAddr;
                strcpy(stats.rootNodeType,"RP");
                stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                 RPAddr);
                stats.outgoingInterface = interfaceId;
                stats.upstreamNeighborId =
                            MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                nextHop);
                stats.upstreamInterface = outInterface;

                //insert this new entry into multicast_connectivity cache
                STATSDB_HandleMulticastConnCreation(node,stats);
#endif

                /* Routing PimSm Send (*, G)Join Packet */
                RoutingPimSmSendJoinPrunePacket(node,
                                                srcAddr,
                                                nextHop,
                                                groupAddr,
                                                outInterface,
                                                joinPruneType,
                                                ROUTING_PIM_JOIN_TREE,
                                                treeInfoBaseRowPtr);

                /* Routing PimSm Set Join Timer to t_periodic*/
                joinTimer.srcAddr = srcAddr;
                joinTimer.grpAddr = groupAddr;
                joinTimer.intfIndex = (unsigned int)outInterface;
                joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
                joinTimer.treeState = treeState;

                /* Note the latest joinTimerSeq in treeInfo Base */
                treeInfoBaseRowPtr->joinTimerSeq++;
                delay = (clocktype) (pimDataSm->routingPimSmTPeriodicInterval);
                treeInfoBaseRowPtr->lastJoinTimerEnd = node->getNodeTime() + delay;
                RoutingPimSetTimer(node,
                                   outInterface,
                                   MSG_ROUTING_PimSmJoinTimerTimeout,
                                   (void*) &joinTimer, delay);
            }
        }
    }
    else /* inJoin state */
    {
        if (!suppressJoinPrune)
        {

            /* If RPF_interface(RP(G)) is a shared medium. This router sees
              another router on RPF_interface(RP(G)) send a Join(*, G) to
              RPF'(*, G).  This causes this router to suppress its own Join.
              */
            if ((interfaceId >= 0) && 
                (pim->interface[interfaceId].interfaceType ==
                    ROUTING_PIM_BROADCAST_INTERFACE))
            {
                /* check if join seen in this up interface is  true */
                if (treeInfoBaseRowPtr->joinSeenInUpIntf)
                {
                    RoutingPimSmJoinPruneTimerInfo    joinTimer;
                    clocktype delay;

                    if (DEBUG_JP) {
                        printf("Node %u Check join seen \n", node->nodeId);
                        printf(" corresponding join seen is %d \n",
                               treeInfoBaseRowPtr->joinSeenInUpIntf);
                        printf(" corresponding prune seen is %d \n",
                               treeInfoBaseRowPtr->pruneSeenInUpIntf);
                    }

                    /* Routing PimSm increase Timer to t_supressed*/
                    joinTimer.srcAddr = srcAddr;
                    joinTimer.grpAddr = groupAddr;
                    joinTimer.intfIndex = (unsigned int)outInterface;
                    joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
                    joinTimer.treeState = treeState;

                    /* Let t_joinsuppress be the minimum of t_suppressed
                    and the HoldTime from the Join/Prune message triggering this
                    event. If the Join Timer is set to expire in less than
                    t_joinsuppress seconds, reset it so that it expires after
                    t_joinsuppress seconds. If the Join Timer is set to expire
                    in more than t_joinsuppress seconds,leave it unchanged*/

                    /* Modification required as RFC suggests t_suppressed as
                        rand(1.1*pimDataSm->routingPimSmTPeriodicInterval,
                             1.4*pimDataSm->routingPimSmTPeriodicInterval) and
                             also consideration of suppression enabled or not*/
                    clocktype t_suppressed =
                        (clocktype)
                        (1.4* pimDataSm->routingPimSmTPeriodicInterval);
                    clocktype t_joinsuppress = 0;
                    if (joinPruneHoldTime != 0xffff)
                    {
                        t_joinsuppress =
                        t_suppressed <= joinPruneHoldTime ? t_suppressed:
                        joinPruneHoldTime;
                    }
                    else
                        t_joinsuppress = t_suppressed;
                    if ((node->getNodeTime() + t_joinsuppress) >
                        treeInfoBaseRowPtr->lastJoinTimerEnd)
                    {
                        delay = t_joinsuppress;
                        /* note the latest join timer Seq no */
                        treeInfoBaseRowPtr->joinTimerSeq++;
                        treeInfoBaseRowPtr->lastJoinTimerEnd =
                                        node->getNodeTime() + delay;
                        RoutingPimSetTimer(node,
                                       outInterface,
                                       MSG_ROUTING_PimSmJoinTimerTimeout,
                                       (void*) &joinTimer, delay);
                    }
                    treeInfoBaseRowPtr->joinSeenInUpIntf = FALSE;
                }

                /* check if prune seen in this up Interface is true */
                if (treeInfoBaseRowPtr->pruneSeenInUpIntf)
                {
                    RoutingPimSmJoinPruneTimerInfo    joinTimer;
                    clocktype delay;

                if (DEBUG_JP)
                    {
                        printf("causes this rtr to lower Timer to Override\n");
                    }

                    /* Routing PimSm decrease Timer to t_override */
                    joinTimer.srcAddr = srcAddr;
                    joinTimer.grpAddr = groupAddr;
                    joinTimer.intfIndex = (unsigned int)outInterface;
                    joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
                    joinTimer.treeState = treeState;

                    /* If the Join Timer is set to expire in more than t_override
                    seconds, reset it so that it expires after t_override
                    seconds. If the Join Timer is set to expire in less than
                    t_override seconds, leave it unchanged.*/
                    delay = (clocktype)(ROUTING_PIMSM_OVERRIDE_INTERVAL
                                          * RANDOM_erand(pimDataSm->seed));
                    if ((node->getNodeTime() + delay) <
                        treeInfoBaseRowPtr->lastJoinTimerEnd)
                    {
                        /* note the latest join timer Seq no */
                        treeInfoBaseRowPtr->joinTimerSeq++;
                        treeInfoBaseRowPtr->lastJoinTimerEnd =
                                    node->getNodeTime() + delay;

                        RoutingPimSetTimer(node,
                                       outInterface,
                                       MSG_ROUTING_PimSmJoinTimerTimeout,
                                       (void*) &joinTimer, delay);
                    }
                    treeInfoBaseRowPtr->pruneSeenInUpIntf = FALSE;
                }
            }
        }
        /* if JoinDesired(*, G) in that RPF interface is false */
        if (RoutingPimSmJoinDesiredG(node, groupAddr, srcAddr, outInterface)
                == FALSE )
        {
            if (DEBUG_JP)
            {
                printf(" Set Join-Prune State to Prune \n");
                printf(" Routing PimSm Send %d Prune Packet \n",
                       joinPruneType);
            }

#ifdef ADDON_BOEINGFCS
            if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface)
                && !(node->networkData.networkVar->iahepEnabled
                     && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                && treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join
                && RoutingPimSmCountValidOutgoingInterfaceCount(node, treeInfoBaseRowPtr) > 0)
            {
                if (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
                    && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh)
                {
                    int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                            treeInfoBaseRowPtr->grpAddr,
                                                                            treeInfoBaseRowPtr->upstream,
                                                                            treeInfoBaseRowPtr->upInterface);
                    if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                    {
                        printf("\nnode %d Decrement counter in RoutingPimSmHandleGUpstreamStateMachine. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                           node->nodeId, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, treeInfoBaseRowPtr->upInterface, counter);
                    }
                    if (counter == 0)
                    {
                        MiCesMulticastMeshInitiateMeshDeletion(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream);
                    }
                }
            }
#endif // ADDON_BOEINGFCS
            if (!suppressJoinPrune)
            {
                /* Set Join-Prune State to not joined */
                treeInfoBaseRowPtr->upstreamState = PimSm_JoinPrune_NotJoin;

    #ifdef ADDON_DB
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = groupAddr;
                strcpy(stats.rootNodeType,"RP");
                stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                 RPAddr);
                stats.outgoingInterface = interfaceId;
                stats.upstreamNeighborId =
                            MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                nextHop);
                stats.upstreamInterface = outInterface;

                //delete this entry into multicast connectivity cache
                STATSDB_HandleMulticastConnDeletion(node,stats);
    #endif

                /* Routing PimSm Send (*, G)Prune Packet */
                RoutingPimSmSendJoinPrunePacket(node,
                                                srcAddr,
                                                nextHop,
                                                groupAddr,
                                                outInterface,
                                                joinPruneType,
                                                ROUTING_PIM_PRUNE_TREE,
                                                treeInfoBaseRowPtr);

                /* Note the latest joinTimerSeq in treeInfo Base */
                treeInfoBaseRowPtr->joinTimerSeq++;
            }
        }
        /* current next hop towards the RP changes  */
        else if ((RoutingPimSmSelectNewRPFG(node, groupAddr) != nextHop)
             && (!suppressJoinPrune))

        {
            /* decrease timer to t_override */
            RoutingPimSmJoinPruneTimerInfo    joinTimer;
            clocktype delay;


            /* Routing PimSm decrease Timer to t_override */
            joinTimer.srcAddr = srcAddr;
            joinTimer.grpAddr = groupAddr;
            joinTimer.intfIndex = (unsigned int)outInterface;
            joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
            joinTimer.treeState = treeState;
            /* If the Join Timer is set to expire in more than t_override
            seconds, reset it so that it expires after t_override seconds.
            If the Join Timer is set to expire in less than t_override
            seconds, leave it unchanged.*/
            delay = (clocktype)(ROUTING_PIMSM_OVERRIDE_INTERVAL
                                  * RANDOM_erand(pimDataSm->seed));
            if ((node->getNodeTime() + delay) <
                 treeInfoBaseRowPtr->lastJoinTimerEnd)
            {
                /* note the latest join timer Seq no */
                treeInfoBaseRowPtr->joinTimerSeq++;
                treeInfoBaseRowPtr->lastJoinTimerEnd = node->getNodeTime() +
                                                        delay;

                RoutingPimSetTimer(node,
                               outInterface,
                               MSG_ROUTING_PimSmJoinTimerTimeout,
                               (void*) &joinTimer, delay);
            }
        }
    }   // in join state;
}

/*
*  FUNCTION    : RoutingPimSmHandleSGUpstreamStateMachine()
*  PURPOSE     : PIM SG Join/Prune message modifies upstream state machine
*                for all associated Joined and Pruned sources for each entry.
*  RETURN VALUE: NULL
*/

void
RoutingPimSmHandleSGUpstreamStateMachine(Node* node, NodeAddress srcAddr,
        int interfaceId, NodeAddress groupAddr,
        clocktype joinPruneHoldTime,
        RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr,
        BOOL suppressJoinPrune)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    NodeAddress nextHop;
    int  outInterface;
    RoutingPimSmJoinPruneType joinPruneType = ROUTING_PIMSM_SG_JOIN_PRUNE;
    RoutingPimSmMulticastTreeState treeState = ROUTING_PIMSM_SG;
    BOOL IAmRP = FALSE;

    if (treeInfoBaseRowPtr == NULL)
    {
#ifdef EXATA
        if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
        }
#else
        ERROR_Assert(FALSE,
            "JoinPrune_Pkt received so entry must exist\n");
#endif
        return;
    }

    /*find the correct RPF(S) interface & associated nextHop */
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, srcAddr,
            &outInterface, &nextHop);

    if (nextHop == 0)
    {
        nextHop = srcAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           srcAddr,
                                           &nextHop,
                                           &outInterface);
    }

    if (outInterface == NETWORK_UNREACHABLE)
    {
        if (DEBUG_JP)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,srcAddr);
        }
        return;
    }

#ifdef ADDON_BOEINGFCS
    /* RFC 2362 - 3.2.2.3:

    For each address, Sj, in the join list of the Join/Prune
    message, for which there is an existing (Sj,G) route entry,

       1 If the RPT-bit is not set for Sj listed in the
         Join/Prune message, but the RPT-bit flag is set on the
         existing (Sj,G) entry, the router clears the RPT-bit flag
         on the (Sj,G) entry, sets the incoming interface to point
         towards Sj for that (Sj,G) entry, and sends a Join/Prune
         message corresponding to that entry through the new
         incoming interface; and ...

    */

    if (treeState == ROUTING_PIMSM_SG &&
        treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SGrpt)
    {
        // 'clears the RPT-bit flag on (Sj,G)'
        treeInfoBaseRowPtr->treeState = ROUTING_PIMSM_SG;
    }
#endif

    if (DEBUG_JP) {
        printf(" \nNode %u handle upstremm Interface %d \n",
               node->nodeId, outInterface);
        printf(" it has upstream join prune state %d \n",
               treeInfoBaseRowPtr->upstreamState);
    }

    if (treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_NotJoin)
    {
        if (DEBUG_JP) {
            printf("\nNode %u: set upstream join \n", node->nodeId);
        }

        /* if JoinDesired(*, S, G) in that RPF interface is true */
        if (RoutingPimSmJoinDesiredSG(
                    node,
                    groupAddr,
                    srcAddr))
        {
            RoutingPimSmJoinPruneTimerInfo    joinTimer;
            clocktype delay;
#ifdef ADDON_BOEINGFCS
            if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface)
                && !(node->networkData.networkVar->iahepEnabled
                     && node->networkData.networkVar->iahepData->nodeType == RED_NODE))
//                && RoutingPimSmCountValidOutgoingInterfaceCount(node, treeInfoBaseRowPtr) > 0)
            {
                if (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
                    && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh)
                {
                    int counter = RoutingPimSmGroupMemStateIncrementCounter(node,
                                                                            treeInfoBaseRowPtr->grpAddr,
                                                                            treeInfoBaseRowPtr->upstream,
                                                                            treeInfoBaseRowPtr->upInterface);
                    if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                    {
                        printf("\nnode %d Increment counter in RoutingPimSmHandleSGUpstreamStateMachine. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                           node->nodeId, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, treeInfoBaseRowPtr->upInterface, counter);
                    }
                    if (counter == 1)
                    {
                        MiCesMulticastMeshInitiateMeshFormation(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, pimDataSm->miMulticastMeshTimeout, FALSE);
                    }
                }
            }
#endif // ADDON_BOEINGFCS
            if (!suppressJoinPrune)
            {
                treeInfoBaseRowPtr->upstreamState = PimSm_JoinPrune_Join;
                if (DEBUG_JP)
                {
                    printf(" \nSet Join-Prune State to join \n");
                    printf(" Routing PimSm Send %d Join Packet \n",
                           joinPruneType);
                    printf(" Routing PimSm Set Timer to t_periodic \n");
                    printf("Node %u:Set UpstreamState for %d treeState \
                           to join \n", node->nodeId, treeState);
                }

#ifdef ADDON_DB
                NodeAddress RPAddr = RoutingPimSmFindRPForThisGroup(node,
                                                                groupAddr);

                /*check if I am the RP */
                for (int i = 0; i < node->numberInterfaces; i++)
                {
                    if (RPAddr == pim->interface[i].ipAddress)
                    {
                        IAmRP  = TRUE;
                        break;
                    }
                }

                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = groupAddr;
                strcpy(stats.rootNodeType,"Source");
                stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                   srcAddr);
                if (IAmRP)
                {
                    stats.outgoingInterface = ANY_DEST;
                }
                else
                {
                    stats.outgoingInterface = interfaceId;
                }

                stats.upstreamNeighborId =
                            MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                nextHop);
                stats.upstreamInterface = outInterface;

                //insert this new entry into multicast_connectivity cache
                STATSDB_HandleMulticastConnCreation(node,stats);
#endif

                /* Routing PimSm Send (S, G)Join Packet */
                RoutingPimSmSendJoinPrunePacket(node,
                                                srcAddr,
                                                nextHop,
                                                groupAddr,
                                                outInterface,
                                                joinPruneType,
                                                ROUTING_PIM_JOIN_TREE,
                                                treeInfoBaseRowPtr);

                /* Routing PimSm Set Join Timer to t_periodic*/
                joinTimer.srcAddr = srcAddr;
                joinTimer.grpAddr = groupAddr;
                joinTimer.intfIndex = (unsigned int)outInterface;
                joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
                joinTimer.treeState = treeState;

                /* Note the latest joinTimerSeq in treeInfo Base */
                treeInfoBaseRowPtr->joinTimerSeq++;
                delay = (clocktype) (pimDataSm->routingPimSmTPeriodicInterval);
                treeInfoBaseRowPtr->lastJoinTimerEnd = node->getNodeTime() + delay;
                RoutingPimSetTimer(node,
                                   outInterface,
                                   MSG_ROUTING_PimSmJoinTimerTimeout,
                                   (void*) &joinTimer, delay);
            }
        }
    }
    else /* inJoin state */
    {

        /* If RPF_interface(RP(S)) is a shared medium. This router sees
          another router on RPF_interface(RP(S)) send a Join(*, S, G) to
          RPF'(*, S, G).  This causes this router to suppress its own Join.
          */
        if (!suppressJoinPrune)
        {
            if ((interfaceId >= 0) &&
                (pim->interface[interfaceId].interfaceType ==
                    ROUTING_PIM_BROADCAST_INTERFACE))
            {
                /* check if join seen in this up interface is  true */
                if (treeInfoBaseRowPtr->joinSeenInUpIntf)
                {
                    RoutingPimSmJoinPruneTimerInfo    joinTimer;
                    clocktype delay;

                    if (DEBUG_JP) {
                        printf("Node %u Check join seen \n", node->nodeId);
                        printf(" corresponding join seen is %d \n",
                               treeInfoBaseRowPtr->joinSeenInUpIntf);
                        printf(" corresponding prune seen is %d \n",
                               treeInfoBaseRowPtr->pruneSeenInUpIntf);
                        printf("causes this router to suppress its own Join \n");
                    }

                    /* Routing PimSm increase Timer to t_supressed*/
                    joinTimer.srcAddr = srcAddr;
                    joinTimer.grpAddr = groupAddr;
                    joinTimer.intfIndex = (unsigned int)outInterface;
                    joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
                    joinTimer.treeState = treeState;
                    /* Let t_joinsuppress be the minimum of t_suppressed and the
                    HoldTime from the Join/Prune message triggering this event.
                    If the Join Timer is set to expire in less than
                    t_joinsuppress seconds, reset it so that it expires
                    after t_joinsuppress seconds. If the Join Timer is set to
                    expire in more than t_joinsuppress seconds,leave it
                    unchanged.*/
                    clocktype t_suppressed =
                        (clocktype)
                        (1.4* pimDataSm->routingPimSmTPeriodicInterval);
                    clocktype t_joinsuppress = 0;
                    if (joinPruneHoldTime != 0xffff)
                    {
                        t_joinsuppress =
                        t_suppressed <= joinPruneHoldTime ? t_suppressed:
                        joinPruneHoldTime;
                    }
                    else
                        t_joinsuppress = t_suppressed;
                    if ((node->getNodeTime() + t_joinsuppress) >
                        treeInfoBaseRowPtr->lastJoinTimerEnd)
                    {
                        /* note the latest join timer Seq no */
                        delay = t_joinsuppress;
                        treeInfoBaseRowPtr->joinTimerSeq++;
                        treeInfoBaseRowPtr->lastJoinTimerEnd =
                                            node->getNodeTime() + delay;
                        RoutingPimSetTimer(node,
                                       outInterface,
                                       MSG_ROUTING_PimSmJoinTimerTimeout,
                                       (void*) &joinTimer, delay);
                    }
                    treeInfoBaseRowPtr->joinSeenInUpIntf = FALSE;
                }

                /* check if prune seen in this up Interface is true */
                if (treeInfoBaseRowPtr->pruneSeenInUpIntf)
                {
                    RoutingPimSmJoinPruneTimerInfo   joinTimer;
                    clocktype delay;

                    /* Routing PimSm decrease Timer to t_override */
                    joinTimer.srcAddr = srcAddr;
                    joinTimer.grpAddr = groupAddr;
                    joinTimer.intfIndex = (unsigned int)outInterface;
                    joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
                    joinTimer.treeState = treeState;
                    delay = (clocktype)(ROUTING_PIMSM_OVERRIDE_INTERVAL
                                        *  RANDOM_erand(pimDataSm->seed));
                    /* If the Join Timer is set to expire in more than
                    t_override seconds, reset it so that it expires after
                    t_override seconds.*/
                    if ((node->getNodeTime() + delay) <
                        treeInfoBaseRowPtr->lastJoinTimerEnd)
                    {
                        /* note the latest join timer Seq no */
                        treeInfoBaseRowPtr->joinTimerSeq++;
                        treeInfoBaseRowPtr->lastJoinTimerEnd =
                            node->getNodeTime() + delay;
                        RoutingPimSetTimer(node,
                                       outInterface,
                                       MSG_ROUTING_PimSmJoinTimerTimeout,
                                       (void*) &joinTimer, delay);
                    }
                    treeInfoBaseRowPtr->pruneSeenInUpIntf = FALSE;
                }
            }
        }
        /* if JoinDesired(*, G) in that RPF interface is false */
        if (RoutingPimSmJoinDesiredSG(node, groupAddr, srcAddr)
                == FALSE)
        {
            if (DEBUG_JP)
            {
                printf(" \nNode %d Set Join-Prune State to NotJoin \n",
                        node->nodeId);
                printf(" Routing PimSm Send %d Prune Packet \n",
                       joinPruneType);
            }

#ifdef ADDON_BOEINGFCS
            if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface)
                && !(node->networkData.networkVar->iahepEnabled
                     && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                && treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join
                && RoutingPimSmCountValidOutgoingInterfaceCount(node, treeInfoBaseRowPtr) > 0)
            {
                if (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
                    && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh)
                {
                    int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                            treeInfoBaseRowPtr->grpAddr,
                                                                            treeInfoBaseRowPtr->upstream,
                                                                            treeInfoBaseRowPtr->upInterface);
                    if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                    {
                        printf("\nnode %d Decrement counter in RoutingPimSmHandleSGUpstreamStateMachine. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                           node->nodeId, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, treeInfoBaseRowPtr->upInterface, counter);
                    }
                    if (counter == 0)
                    {
                        MiCesMulticastMeshInitiateMeshDeletion(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream);
                    }
                }
            }
#endif // ADDON_BOEINGFCS
            if (!suppressJoinPrune)
            {

                /* Set Join-Prune State to not joined */
                treeInfoBaseRowPtr->upstreamState = PimSm_JoinPrune_NotJoin;

#ifdef ADDON_DB
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = groupAddr;
                strcpy(stats.rootNodeType,"Source");
                stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                    srcAddr);
                stats.outgoingInterface = interfaceId;
                stats.upstreamNeighborId =
                                    MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                        nextHop);
                if (IAmRP)
                {
                    stats.outgoingInterface = ANY_DEST;
                }
                else
                {
                    stats.outgoingInterface = interfaceId;
                }

                //delete this entry into multicast connectivity cache
                STATSDB_HandleMulticastConnDeletion(node,stats);
#endif

                /* Routing PimSm Send (*, G)Prune Packet */
                RoutingPimSmSendJoinPrunePacket(node,
                                                srcAddr,
                                                nextHop,
                                                groupAddr,
                                                outInterface,
                                                joinPruneType,
                                                ROUTING_PIM_PRUNE_TREE,
                                                treeInfoBaseRowPtr);

                /* cancel the last join timer */
               /* Note the latest joinTimerSeq in treeInfo Base */
                treeInfoBaseRowPtr->joinTimerSeq++;
                /* Set SPTbit(S, G) False */
                if (DEBUG)
                {
                    printf("Node %u: set SPTbit to False for src %x grp"
                           " %x \n",node->nodeId, srcAddr, groupAddr);
                }

                treeInfoBaseRowPtr->SPTbit = FALSE;
            }
        }
        /* current next hop towards the RP changes  */
        else if ((RoutingPimSmSelectNewRPFSG(node,
                 groupAddr, srcAddr) != nextHop)
                 && (!suppressJoinPrune))
        {
            /* decrease timer to t_override */
            RoutingPimSmJoinPruneTimerInfo    joinTimer;
            clocktype delay;
            if (DEBUG)
            {
                printf("causes this router to lower Timer to t_override \n");
            }
            /* Routing PimSm decrease Timer to t_override */
            joinTimer.srcAddr = srcAddr;
            joinTimer.grpAddr = groupAddr;
            joinTimer.intfIndex = (unsigned int)outInterface;
            joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
            joinTimer.treeState = treeState;
            /* If the Join Timer is set to expire in more than
            t_override seconds, reset it so that it expires after
            t_override seconds. If the Join Timer is set to expire in
            less than t_override seconds, leave it unchanged.*/

            delay = (clocktype)(ROUTING_PIMSM_OVERRIDE_INTERVAL
                                *  RANDOM_erand(pimDataSm->seed));
            if ((node->getNodeTime() + delay) <
                 treeInfoBaseRowPtr->lastJoinTimerEnd)
            {
                /* note the latest join timer Seq no */
                treeInfoBaseRowPtr->joinTimerSeq++;
                treeInfoBaseRowPtr->lastJoinTimerEnd =
                                    node->getNodeTime() + delay;
                RoutingPimSetTimer(node,
                               outInterface,
                               MSG_ROUTING_PimSmJoinTimerTimeout,
                               (void*) &joinTimer, delay);
            }
        }
    }   // in join state;
}

/*
*  FUNCTION    : RoutingPimSmHandleSGrptUpstreamStateMachine()
*  PURPOSE     : A PIM SGrpt Join/Prune message modifies the upstream
*               state machine for all associated Joined and Pruned sources
*               for each such entry.
*  RETURN VALUE: NULL
*/

void
RoutingPimSmHandleSGrptUpstreamStateMachine(Node* node, NodeAddress srcAddr,
        NodeAddress groupAddr,
        clocktype joinPruneHoldTime,
        RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimSmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }

    joinPruneHoldTime = joinPruneHoldTime;

    NodeAddress nextHop;
    int  outInterface;

    RoutingPimSmJoinPruneTimerInfo    OTTimer;
    NodeAddress RPAddr;

    RoutingPimSmJoinPruneType  joinPruneType =
        ROUTING_PIMSM_SGrpt_JOIN_PRUNE;

    RoutingPimSmMulticastTreeState  treeState = ROUTING_PIMSM_SGrpt;

    if (treeInfoBaseRowPtr == NULL)
    {
#ifdef EXATA
        if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
        }
#else
        ERROR_Assert(FALSE,
            "JoinPrune_Pkt received so entry must exist\n");
#endif
        return;
    }

    /* check the RP For this group & find associated nextHop*/
    RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, RPAddr,
            &outInterface, &nextHop);

    if (nextHop == 0)
    {
        nextHop = RPAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHop,
                                           &outInterface);
    }

    if (outInterface == NETWORK_UNREACHABLE)
    {
        if (DEBUG_JP)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,RPAddr);
        }
        return;
    }

    /* JoinDesired is true when the router has forwarding state
    that would cause it to forward traffic for G using shared tree
    state. Although JoinDesired is true, the router's sending of a
    Join message may be suppressed by another router sending
    a Join onto the upstream interface. */

    if (DEBUG_JP) {
        printf(" Node %u handle upstremm Interface %d \n",
               node->nodeId, outInterface);
        printf(" it has upstream join prune state %d \n",
               treeInfoBaseRowPtr->upstreamState);
    }
    switch (treeInfoBaseRowPtr->upstreamState)
    {
        case PimSm_SGrpt_NotPruned:
        {
            // See Prune(S,G,rpt) to RPF’(S,G,rpt)
            if (treeInfoBaseRowPtr->pruneSeenInUpIntf)
            {

                /* The action is to set the (S,G,rpt) Override Timer
                to the randomized prune-override interval,
                t_override. However,if the Override Timer is already
                running, we only set the timer if doing so would set
                it to a lower value. */

                BOOL scheduleTimer = FALSE;
                clocktype delay = (clocktype)
                        (ROUTING_PIMSM_OVERRIDE_INTERVAL
                                 *  RANDOM_erand(pimSmData->seed));
                if (treeInfoBaseRowPtr->lastOTTimerStart != 0)
                {
                    if ((treeInfoBaseRowPtr->lastOTTimerEnd
                         - node->getNodeTime()) > delay )
                    {
                        // Schedule new override timer
                        scheduleTimer = TRUE;
                    }
                }
                else
                {
                         scheduleTimer = TRUE;
                }
                if (scheduleTimer)
                {
                    OTTimer.srcAddr = srcAddr;
                    OTTimer.grpAddr = groupAddr;
                    OTTimer.intfIndex = (unsigned int)outInterface;
                    OTTimer.seqNo = treeInfoBaseRowPtr->OTTimerSeq + 1;
                    OTTimer.treeState = treeState;
                    treeInfoBaseRowPtr->lastOTTimerStart =
                        node->getNodeTime();
                    treeInfoBaseRowPtr->lastOTTimerEnd =
                        delay + node->getNodeTime();
                    /* Note the latest joinTimerSeq in treeInfo Base */
                    treeInfoBaseRowPtr->OTTimerSeq++;

                    /*clocktype delay = (clocktype)
                        (ROUTING_PIMSM_OVERRIDE_INTERVAL
                                 *  RANDOM_erand(pimSmData->seed));*/
                    RoutingPimSetTimer(node,
                               outInterface,
                               MSG_ROUTING_PimSmOverrideTimerTimeout,
                               (void*) &OTTimer, delay);
                }
                treeInfoBaseRowPtr->pruneSeenInUpIntf = FALSE;
            }
            // See Join(S,G,rpt) to RPF’(S,G,rpt)
            if (treeInfoBaseRowPtr->joinSeenInUpIntf)
            {
                /* The router sees a Join(S,G,rpt) from someone else
                to RPF’(S,G,rpt), which is the correct upstream
                neighbor. If we’re in "NotPruned" state and the
                (S,G,rpt) Override Timer is running,
                then this is because we have been triggered to send
                our own Join(S,G,rpt) to RPF’(S,G,rpt).
                Someone else beat us to it, so there’s no need to send
                our own Join.The action is to cancel
                the Override Timer. */
                treeInfoBaseRowPtr->OTTimerSeq++;
                treeInfoBaseRowPtr->joinSeenInUpIntf = FALSE;
            }
            // PruneDesired(S,G,rpt)->TRUE
            else if (RoutingPimSmPruneDesiredSGrpt(node, groupAddr, srcAddr,
                                      outInterface) == TRUE)
            {
                /* If the router was previously in NotPruned state,
                then the action is to send a
                Prune(S,G,rpt) to RPF’(S,G,rpt), and to cancel the
                Override Timer.*/

                if (DEBUG_JP)
                {
                    printf(" Set Join-Prune State to prune \n");
                    printf(" Routing PimSm Send %d prune Packet \n",
                           joinPruneType);
                    printf(" Routing PimSm cancel OT Timer \n");
                }

                treeInfoBaseRowPtr->upstreamState =
                                        PimSm_SGrpt_Pruned;

                /* Routing PimSm Send (*, S, G, rpt)Prune Packet */
                RoutingPimSmSendJoinPrunePacket(node,
                                            srcAddr,
                                            nextHop,
                                            groupAddr,
                                            outInterface,
                                            joinPruneType,
                                            ROUTING_PIM_PRUNE_TREE,
                                            treeInfoBaseRowPtr);

                /* Routing PimSm Cancel override Timer(the last one stored)*/
                treeInfoBaseRowPtr->OTTimerSeq++;
            }
            // RPTJoinDesired(G)->FALSE
            else if (RoutingPimSmRPTJoinDesiredG( node,
                                             groupAddr,
                                             srcAddr,
                                             outInterface) == FALSE)
            {
                /* The router no longer wishes to receive any traffic
                   destined for G on the RP Tree. This causes a
                   transition to the RPTNotJoined(G) state,
                   and the Override Timer is canceled if it was running.
                   */
                    if (DEBUG_JP)
                    {
                        printf(" Set Join-Prune State to RPTNotJoined \n");
                        printf(" Routing PimSm cancel OT Timer \n");
                    }
                    treeInfoBaseRowPtr->upstreamState =
                                        PimSm_SGrpt_NotJoined;
                    /* Routing PimSm Cancel override Timer
                    (the last one stored)*/
                    treeInfoBaseRowPtr->OTTimerSeq++;
            }

            // RPF’(S,G,rpt) changes to become equal to RPF’(*,G)
            break;
        }
        case PimSm_SGrpt_Pruned:
        {
            // PruneDesired(S,G,rpt)->FALSE
            if ((RoutingPimSmPruneDesiredSGrpt(node, groupAddr, srcAddr,
                                          outInterface) ==
                                          FALSE) &&
                    (RoutingPimSmRPTJoinDesiredG( node,
                                                 groupAddr,
                                                 srcAddr,
                                                 outInterface) == TRUE))
            {
                /* If the router is in the Pruned(S,G,rpt) state,
                and PruneDesired(S,G,rpt) changes to FALSE,
                this could be because the router no longer
                has RPTJoinDesired(G) true, or it now wishes to receive
                traffic from S again. If it is the former, then this
                transition should not happen, but instead the
                "RPTJoinDesired(G)->FALSE" transition should happen.
                Thus, this transition should be interpreted as
                "PruneDesired(S,G,rpt)->FALSE AND
                RPTJoinDesired(G)==TRUE".The action is to send a
                Join(S,G,rpt) to RPF’(S,G,rpt)*/
                if (DEBUG_JP)
                {
                    printf(" Set Join-Prune State to Not Prune \n");
                    printf(" Routing PimSm Send %d Join Packet \n",
                       joinPruneType);
                }
                RoutingPimSmSendJoinPrunePacket(node,
                                        srcAddr,
                                        nextHop,
                                        groupAddr,
                                        outInterface,
                                        joinPruneType,
                                        ROUTING_PIM_JOIN_TREE,
                                        treeInfoBaseRowPtr);
                treeInfoBaseRowPtr->upstreamState =
                                        PimSm_SGrpt_NotPruned;
            }
            // RPTJoinDesired(G)->FALSE
            else if (RoutingPimSmRPTJoinDesiredG( node,
                                             groupAddr,
                                             srcAddr,
                                             outInterface) == FALSE)
            {
                /* The router no longer wishes to receive any traffic
                destined for G on the RP Tree. This causes a
                transition to the RPTNotJoined(G) state,
                and the Override Timer is canceled if it was running.
                */
               if (DEBUG_JP)
               {
                    printf(" Set Join-Prune State to RPTNotJoined \n");
                    printf(" Routing PimSm cancel OT Timer \n");
                }
                treeInfoBaseRowPtr->upstreamState =
                                            PimSm_SGrpt_NotJoined;
                /* Routing PimSm Cancel override Timer
                            (the last one stored)*/
                treeInfoBaseRowPtr->OTTimerSeq++;
            }
           break;
        }
        case PimSm_SGrpt_NotJoined:
        {
            // PruneDesired(S,G,rpt)->TRUE
            if (RoutingPimSmPruneDesiredSGrpt(node, groupAddr, srcAddr,
                                      outInterface) == TRUE)
            {
                /* If the router was previously in RPTNotJoined(G) state,
                then there is no need to trigger an action in this state
                machine because sending a Prune(S,G,rpt)
                is handled by the rules for sending the Join(*,G)
                or Join(*,*,RP)*/
                if (DEBUG_JP)
                {
                    printf(" Set Join-Prune State to prune \n");
                }
                treeInfoBaseRowPtr->upstreamState =
                                        PimSm_SGrpt_Pruned;
            }
            // inherited_olist(S,G,rpt) becomes non-NULL
            else if (RoutingPimSmInheritedOutListSGrpt(node,
                                                      groupAddr,
                                                      srcAddr,
                                                  treeInfoBaseRowPtr) != 0)
            {
                /* This transition is only relevant in the
                RPTNotJoined(G) state.The router has joined the RP tree
                (handled by the (*,G) or (*,*,RP) upstream state machine
                as appropriate) and wants to receive traffic from S.
                This does not trigger any events in this state machine,
                but causes a transition to the NotPruned(S,G,rpt) state.*/
                if (DEBUG_JP)
                {
                    printf(" Set Join-Prune State to not prune \n");
                }
                treeInfoBaseRowPtr->upstreamState =
                                         PimSm_SGrpt_NotPruned;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}


/*
*  FUNCTION    : RoutingPimSmHandlePrunePendingTimerExpiresEvent()
*  PURPOSE     : Routing PimSm Handle Prune Pending Timer Expires Event
*  RETURN VALUE: NULL
*/
void
RoutingPimSmHandlePrunePendingTimerExpiresEvent(Node* node,
        NodeAddress srcAddr, NodeAddress groupAddr, int interfaceId,
        RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    RoutingPimSmDownstream* downstreamListPtr;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmTreeInformationBase* treeInfoBase = &pimDataSm->treeInfoBase;

    TreeInfoRowMap* rowPtrMap = treeInfoBase->rowPtrMap;
    TreeInfoRowMapIterator mapIterator;
    RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;

    RoutingPimSmJoinPruneType joinPruneType =
         ROUTING_PIMSM_NOINFO_JOIN_PRUNE;

    /* If state already exists, then the Join message has reached
    the shared tree and the interface from which the message was received is
    entered in the outgoing interface list. If no state exists, a (*,G) entry
    is created, the interface is entered in the outgoing interface list, and
    the Join message is again sent towards the RP. */

    if (treeInfoBaseRowPtr == NULL)
    {
#ifdef EXATA
        if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
        }
#else
        ERROR_Assert(FALSE,
            "JoinPrune_Pkt received so entry must exist\n");
#endif
        return;
    }

    downstreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                        treeInfoBaseRowPtr, interfaceId);

    if (downstreamListPtr == NULL)
    {
#ifdef EXATA
        if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
        }
#else
        ERROR_Assert(FALSE,
            "JoinPrune_Pkt received so entry must exist\n");
#endif
        return;
    }

    /* check the protocolState & switch accrdingly */
    switch (treeInfoBaseRowPtr->treeState)
    {
        case ROUTING_PIMSM_G:
        {
            joinPruneType = ROUTING_PIMSM_G_JOIN_PRUNE;
            break;
        }
        case ROUTING_PIMSM_SG:
        {
            joinPruneType = ROUTING_PIMSM_SG_JOIN_PRUNE;
            break;
        }
        case ROUTING_PIMSM_SGrpt:
        {
            joinPruneType = ROUTING_PIMSM_SGrpt_JOIN_PRUNE;
            break;
        }
        default:
        {
#ifdef EXATA
            if (!node->partitionData->isEmulationMode)
            {
                ERROR_Assert(FALSE, "Unknown multicast tree State"
                            " in PIMSM\n");
            }
#else
            ERROR_Assert(FALSE, "Unknown multicast tree State in PIMSM\n");
#endif
            return;
        }
    }

    NodeAddress RPAddr = RoutingPimSmFindRPForThisGroup(node,
                                                        groupAddr);
    BOOL IamRP = FALSE;
    // Check if any of its interface address is same as RP
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (pim->interface[i].ipAddress == RPAddr)
        {
            IamRP = TRUE;
            break;
        }
    }

    /* check the interface specific state & switch accordingly */
    if (downstreamListPtr->joinPruneState
            == PimSm_JoinPrune_PrunePending)
    {
#ifdef ADDON_BOEINGFCS
        if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface)
            && !(node->networkData.networkVar->iahepEnabled
                 && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
            && (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G
                || treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG)
            && treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join
            && RoutingPimSmCountValidOutgoingInterfaceCount(node, treeInfoBaseRowPtr) == 1) // we are about to invalidate the last one
        {
            if (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
                && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh)
            {
                int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                        treeInfoBaseRowPtr->grpAddr,
                                                                        treeInfoBaseRowPtr->upstream,
                                                                        treeInfoBaseRowPtr->upInterface);
                if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                {
                    printf("\nnode %d Decrement counter in RoutingPimSmHandlePrunePendingTimerExpiresEvent. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                       node->nodeId, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, treeInfoBaseRowPtr->upInterface, counter);
                }
                if (counter == 0)
                {
                    MiCesMulticastMeshInitiateMeshDeletion(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream);
                }
            }
        }
#endif // ADDON_BOEINGFCS
        if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SGrpt)
            downstreamListPtr->joinPruneState = PimSm_JoinPrune_Pruned;
        else
            downstreamListPtr->joinPruneState = PimSm_JoinPrune_NoInfo;
        /* A PruneEcho(*, G) need not be sent on a point-to-point
        interface.*/
        if (pim->interface[interfaceId].interfaceType !=
                ROUTING_PIM_POINT_TO_POINT_INTERFACE &&
                treeInfoBaseRowPtr->treeState != ROUTING_PIMSM_SGrpt)
        {

            downstreamListPtr->joinPruneState = PimSm_JoinPrune_NoInfo;
            /* A PruneEcho(*,G) need not be sent on an interface that
            contains only a single PIM neighbor during the time this state
            machine was in Prune-Pending state.*/
            if (pim->interface[interfaceId].
                    smInterface->neighborList->size > 1)
            {
                if (DEBUG_JP) {
                    printf(" Node %u send prune on Interface %d \
                           joinPruneType %d\n",
                           node->nodeId, interfaceId,joinPruneType);
                    printf(" it has downstream join prune state pruned as \
                           prune pending timer expired\n");
                }
                /* Routing PimSm Send (*, G)Prune Packet */
                RoutingPimSmSendJoinPrunePacket(node,
                                    srcAddr,
                                    pim->interface[interfaceId].ipAddress,
                                    groupAddr,
                                    //RPInterface,
                                    interfaceId,
                                    joinPruneType,
                                    ROUTING_PIM_PRUNE_TREE,
                                    treeInfoBaseRowPtr);
            }
        }
        if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G && IamRP &&
                RoutingPimSmJoinDesiredG(node,
                                         groupAddr,
                                         srcAddr,
                                         interfaceId) == FALSE )
        {
            /* If tree state is G,Search for all those entries that
               correspond to this group.
            */
            for (mapIterator = rowPtrMap->begin();
                 mapIterator != rowPtrMap->end();
                 mapIterator++)
            {
                rowPtr = mapIterator->second;
                /* check for group */
                if (rowPtr->grpAddr == groupAddr)
                {
                    /* check for multicast tree state */
                    if ((rowPtr->treeState == ROUTING_PIMSM_G) ||
                        (rowPtr->treeState == ROUTING_PIMSM_SGrpt))
                    {
                        continue;
                    }
                    else if ((rowPtr->treeState == ROUTING_PIMSM_SG) &&
                             (rowPtr->upstreamState == PimSm_JoinPrune_Join))
                    {
#ifdef ADDON_BOEINGFCS
                        if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface)
                            && !(node->networkData.networkVar->iahepEnabled
                                 && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                            && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
                        {
                            if (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                                && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh)
                            {
                                int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                        rowPtr->grpAddr,
                                                                        rowPtr->upstream,
                                                                        rowPtr->upInterface);
                                if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                                {
                                    printf("\nnode %d Decrement counter in RoutingPimSmHandlePrunePendingTimerExpiresEvent (pruning source tree at RP). grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                           node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                                }
                                if (counter == 0)
                                {
                                    MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr, rowPtr->upstream);
                                }
                            }
                        }
#endif // ADDON_BOEINGFCS
                        rowPtr->upstreamState = PimSm_JoinPrune_NotJoin;
                        if (DEBUG_JP) {
                            printf(" Node %u send SG prune on Interface %d \n",
                                   node->nodeId, rowPtr->nextIntfForSrc);
                            printf(" it has upstream join prune state"
                                " NotJOIN as prune pending timer expired\n");
                        }
                        /* Send PRUNE (S,G) upstream*/
                        RoutingPimSmSendJoinPrunePacket(node,
                                rowPtr->srcAddr,
                                rowPtr->nextHopForSrc,
                                groupAddr,
                                rowPtr->nextIntfForSrc,
                                ROUTING_PIMSM_SG_JOIN_PRUNE,
                                ROUTING_PIM_PRUNE_TREE,
                                treeInfoBaseRowPtr);

                        /* cancel the last join timer */
                        /* Cancel the joinTimerSeq in treeInfo Base */
                        rowPtr->joinTimerSeq++;
                        /* Set SPTbit(S, G) False */
                        rowPtr->SPTbit = FALSE;
                    }
                }
            }//end of for
        }
        else if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SGrpt &&
                IamRP &&
                RoutingPimSmJoinDesiredSG(node,
                                         groupAddr,
                                         srcAddr) == FALSE)
        {
        /* If tree state is (S,G,Rpt), Search for all those entries that
           correspond to this (group,source) and snd PRUNE.
        */
            for (mapIterator = rowPtrMap->begin();
                 mapIterator != rowPtrMap->end();
                 mapIterator++)
            {
                rowPtr = mapIterator->second;
                /* check for group */
                if (rowPtr->grpAddr == groupAddr)
                {
                    /* check for multicast tree state */
                    if ((rowPtr->treeState == ROUTING_PIMSM_G) ||
                        (rowPtr->treeState == ROUTING_PIMSM_SGrpt))
                    {
                        continue;
                    }
                    else if ((rowPtr->treeState == ROUTING_PIMSM_SG)
                              &&
                              (rowPtr->upstreamState == PimSm_JoinPrune_Join)
                              &&
                              (rowPtr->srcAddr == treeInfoBaseRowPtr->srcAddr))
                    {
#ifdef ADDON_BOEINGFCS
                        if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface)
                            && !(node->networkData.networkVar->iahepEnabled
                                 && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                            && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
                        {
                            if (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                                && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh)
                            {
                                int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                                        rowPtr->grpAddr,
                                                                                        rowPtr->upstream,
                                                                                        rowPtr->upInterface);
                                if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                                {
                                    printf("\nnode %d Decrement counter in RoutingPimSmHandlePrunePendingTimerExpiresEvent (from RP). grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                       node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                                }
                                if (counter == 0)
                                {
                                    MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr, rowPtr->upstream);
                                }
                            }
                        }
#endif // ADDON_BOEINGFCS
                        rowPtr->upstreamState = PimSm_JoinPrune_NotJoin;
                        if (DEBUG_JP) {
                            printf(" Node %u send SG prune on Interface %d \n",
                                   node->nodeId, rowPtr->nextIntfForSrc);
                            printf(" it has upstream join prune state"
                                " NotJOIN as prune pending timer expired\n");
                        }
                        /* Send PRUNE (S,G) upstream*/
                        RoutingPimSmSendJoinPrunePacket(node,
                                rowPtr->srcAddr,
                                rowPtr->nextHopForSrc,
                                groupAddr,
                                rowPtr->nextIntfForSrc,
                                ROUTING_PIMSM_SG_JOIN_PRUNE,
                                ROUTING_PIM_PRUNE_TREE,
                                treeInfoBaseRowPtr);

                        /* Cancel the joinTimerSeq in treeInfo Base */
                        rowPtr->joinTimerSeq++;
                        /* Set SPTbit(S, G) False */
                        rowPtr->SPTbit = FALSE;
                    }
                }
            }
        }
        else
        {
            RoutingPimSmHandleUpstreamStateMachine(node,
                                            srcAddr,
                                            interfaceId,
                                            groupAddr,
                                            treeInfoBaseRowPtr);
        }
    }
}

/*
*  FUNCTION    : RoutingPimSmHandleExpiryTimerExpiresEvent()
*  PURPOSE     : Routing PimSm Handle Expiry Timer Expires Event
*  RETURN VALUE: NULL
*/
void
RoutingPimSmHandleExpiryTimerExpiresEvent(Node* node,
        NodeAddress srcAddr, NodeAddress groupAddr, int interfaceId,
        RoutingPimSmTreeInfoBaseRow * treeInfoBaseRowPtr)
{
    RoutingPimSmDownstream* downstreamListPtr;
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmTreeInformationBase* treeInfoBase = &pimDataSm->treeInfoBase;

    TreeInfoRowMap* rowPtrMap = treeInfoBase->rowPtrMap;
    TreeInfoRowMapIterator mapIterator;
    RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;

    /* it checks to see if group G in its multicast treeInfo Base */

    if (treeInfoBaseRowPtr == NULL)
    {
#ifdef EXATA
        if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
        }
#else
        ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
#endif
        return;
    }

    /* it checks to see if interface is in corresponding downstream list */
    downstreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                        treeInfoBaseRowPtr, interfaceId);

    if (downstreamListPtr == NULL)
    {
        printf(" No such interface present in downstream List \n");
        return;
    }

#ifdef ADDON_BOEINGFCS
    if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface)
        && !(node->networkData.networkVar->iahepEnabled
             && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
        && (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G
            || treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG)
        && downstreamListPtr->joinPruneState != PimSm_JoinPrune_NoInfo
        && treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join
        && RoutingPimSmCountValidOutgoingInterfaceCount(node, treeInfoBaseRowPtr) == 1) // we are about to invalidate the last one
    {
        if (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
            && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh)
        {
            int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                    treeInfoBaseRowPtr->grpAddr,
                                                                    treeInfoBaseRowPtr->upstream,
                                                                    treeInfoBaseRowPtr->upInterface);
            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
            {
                printf("\nnode %d Decrement counter in RoutingPimSmHandleExpiryTimerExpiresEvent. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                       node->nodeId, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, treeInfoBaseRowPtr->upInterface, counter);
            }
            if (counter == 0)
            {
                MiCesMulticastMeshInitiateMeshDeletion(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream);
            }
        }
    }
#endif // ADDON_BOEINGFCS
    /* check the protocolState & switch accrdingly */

    switch (treeInfoBaseRowPtr->treeState)
    {
        case ROUTING_PIMSM_G:
        {
            /* Set the interface specific state */
            downstreamListPtr->joinPruneState = PimSm_JoinPrune_NoInfo;
            break;
        }
        case ROUTING_PIMSM_SGrpt:
        case ROUTING_PIMSM_SG:
        {
            /* Set the interface specific state */
            downstreamListPtr->joinPruneState = PimSm_JoinPrune_NoInfo;
            break;
        }
        default:
        {
#ifdef EXATA
            if (!node->partitionData->isEmulationMode)
            {
                ERROR_Assert(FALSE, "Unknown multicast tree State"
                    " in PIMSM\n");
            }
#else
            ERROR_Assert(FALSE, "Unknown multicast tree State in PIMSM\n");
#endif
            break;
        }
    }   //end of switch;

#ifdef ADDON_DB
    //remove this entry from multicast_connectivity cache
    BOOL IAmRP = FALSE;

    StatsDBConnTable::MulticastConnectivity stats;

    stats.nodeId = node->nodeId;
    stats.destAddr = treeInfoBaseRowPtr->grpAddr;

    stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                        treeInfoBaseRowPtr->srcAddr);
    stats.upstreamNeighborId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                       treeInfoBaseRowPtr->upstream);

    stats.upstreamInterface = treeInfoBaseRowPtr->upInterface;

    if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G)
    {
        strcpy(stats.rootNodeType,"RP");
        stats.outgoingInterface = downstreamListPtr->interfaceId;

        // delete entry from  multicast_connectivity cache
        STATSDB_HandleMulticastConnDeletion(node, stats);
    }
    else if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG)
    {
        strcpy(stats.rootNodeType,"Source");

        /*check if it is the RP */
        for (int j = 0; j < node->numberInterfaces; j++)
        {
            if (treeInfoBaseRowPtr->RPointAddr ==
                                                pim->interface[j].ipAddress)
            {
                IAmRP  = TRUE;
                break;
            }
        }

        if (IAmRP)
        {
            stats.outgoingInterface = ANY_DEST;
        }
        else
        {
            stats.outgoingInterface = downstreamListPtr->interfaceId;
        }

        // delete entry from  multicast_connectivity cache
        STATSDB_HandleMulticastConnDeletion(node, stats);
    }
#endif

    NodeAddress RPAddr = RoutingPimSmFindRPForThisGroup(
                                     node,
                                     groupAddr);
    BOOL IamRP = FALSE;
    // Check if any of its interface address is same as RP
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (pim->interface[i].ipAddress == RPAddr)
        {
            IamRP = TRUE;
            break;
        }
    }

    if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G && IamRP &&
        RoutingPimSmJoinDesiredG(node,
                                 groupAddr,
                                 srcAddr,
                                 interfaceId) == FALSE )
    {
        /* If tree state is G,Search for all those entries that
           correspond to this group.
        */
        for (mapIterator = rowPtrMap->begin();
             mapIterator != rowPtrMap->end();
             mapIterator++)
        {
            rowPtr = mapIterator->second;
            /* check for group */
            if (rowPtr->grpAddr == groupAddr)
            {
                /* check for multicast tree state */
                if ((rowPtr->treeState == ROUTING_PIMSM_G) ||
                    (rowPtr->treeState == ROUTING_PIMSM_SGrpt))
                {
                    continue;
                }
                else if ((rowPtr->treeState == ROUTING_PIMSM_SG) &&
                        (rowPtr->upstreamState == PimSm_JoinPrune_Join))
                {
#ifdef ADDON_BOEINGFCS
                    if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface)
                        && !(node->networkData.networkVar->iahepEnabled
                             && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                        && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
                    {
                        if (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                            && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh)
                        {
                            int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                                    rowPtr->grpAddr,
                                                                                    rowPtr->upstream,
                                                                                    rowPtr->upInterface);
                            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                            {
                                printf("\nnode %d Decrement counter in RoutingPimSmHandleExpiryTimerExpiresEvent (pruning source tree at RP). grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                        node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                            }
                            if (counter == 0)
                            {
                                MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr,rowPtr->upstream);
                            }
                        }
                    }
#endif // ADDON_BOEINGFCS
                    rowPtr->upstreamState = PimSm_JoinPrune_NotJoin;
                    if (DEBUG_JP) {
                        printf(" Node %u send SG prune on Interface %d \n",
                               node->nodeId, rowPtr->nextIntfForSrc);
                        printf(" it has upstream join prune state NotJOIN as"
                               " expiry timer expired\n");
                    }
                    /* Send PRUNE (S,G) upstream*/
                    RoutingPimSmSendJoinPrunePacket(
                        node,
                        rowPtr->srcAddr,
                        rowPtr->nextHopForSrc,
                        groupAddr,
                        rowPtr->nextIntfForSrc,
                        ROUTING_PIMSM_SG_JOIN_PRUNE,
                        ROUTING_PIM_PRUNE_TREE,
                        rowPtr);

                     /* cancel the last join timer */
                    /* Cancel the joinTimerSeq in treeInfo Base */
                    rowPtr->joinTimerSeq++;
                    /* Set SPTbit(S, G) False */
                    rowPtr->SPTbit = FALSE;
                }
            }
        }
    }
    else if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SGrpt && IamRP &&
        RoutingPimSmJoinDesiredSG(node,
                                 groupAddr,
                                 srcAddr) == FALSE )
    {
    /* If tree state is (S,G,Rpt), Search for all those entries that
       correspond to this (group,source) and snd PRUNE.
    */
        for (mapIterator = rowPtrMap->begin();
             mapIterator != rowPtrMap->end();
             mapIterator++)
        {
            rowPtr = mapIterator->second;
            /* check for group */
            if (rowPtr->grpAddr == groupAddr)
            {
                /* check for multicast tree state */
                if ((rowPtr->treeState == ROUTING_PIMSM_G) ||
                    (rowPtr->treeState == ROUTING_PIMSM_SGrpt))
                {
                    continue;
                }
                else if ((rowPtr->treeState == ROUTING_PIMSM_SG) &&
                        (rowPtr->upstreamState == PimSm_JoinPrune_Join) &&
                        (rowPtr->srcAddr == srcAddr))
                {
#ifdef ADDON_BOEINGFCS
                    if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface)
                        && !(node->networkData.networkVar->iahepEnabled
                             && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                        && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
                    {
                        if (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                            && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh)
                        {
                            int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                                    rowPtr->grpAddr,
                                                                                    rowPtr->upstream,
                                                                                    rowPtr->upInterface);
                            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                            {
                                printf("\nnode %d Decrement counter in RoutingPimSmHandleExpiryTimerExpiresEvent (from RP). grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                       node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                            }
                            if (counter == 0)
                            {
                                MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr, rowPtr->upstream);
                            }
                        }
                    }
#endif // ADDON_BOEINGFCS
                    rowPtr->upstreamState = PimSm_JoinPrune_NotJoin;
                    if (DEBUG_JP) {
                        printf(" Node %u send SG prune on Interface %d \n",
                               node->nodeId, rowPtr->nextIntfForSrc);
                        printf(" it has upstream join prune state NotJOIN as \
                               expiry timer expired\n");
                    }
                    /* Send PRUNE (S,G) upstream*/
                    RoutingPimSmSendJoinPrunePacket(
                            node,
                            rowPtr->srcAddr,
                            rowPtr->nextHopForSrc,
                            groupAddr,
                            rowPtr->nextIntfForSrc,
                            ROUTING_PIMSM_SG_JOIN_PRUNE,
                            ROUTING_PIM_PRUNE_TREE,
                            rowPtr);

                    /* Cancel the joinTimerSeq in treeInfo Base */
                    rowPtr->joinTimerSeq++;
                    /* Set SPTbit(S, G) False */
                    rowPtr->SPTbit = FALSE;
                }
            }
        }
    }
    else
    {
        RoutingPimSmHandleUpstreamStateMachine(node,
                                               srcAddr,
                                               interfaceId,
                                               groupAddr,
                                               treeInfoBaseRowPtr);
    }
}

/*
*  FUNCTION    : RoutingPimSmHandleJoinTimerExpiresEvent()
*  PURPOSE     : Routing PimSm Handle Join Timer Expires Event
*  RETURN VALUE: NULL
*/
void
RoutingPimSmHandleJoinTimerExpiresEvent(Node* node,
                                    NodeAddress srcAddr,
                                    NodeAddress groupAddr,
                            RoutingPimSmTreeInfoBaseRow * treeInfoBaseRowPtr)
{
    NodeAddress RPAddr;
    NodeAddress nextHop = 0;
    int outInterface = 0;
    clocktype delay;

    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmTreeInformationBase* treeInfo = &pimDataSm->treeInfoBase;

    TreeInfoRowMap* rowPtrMap = treeInfo->rowPtrMap;
    TreeInfoRowMapIterator mapIterator;
    RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;

    RoutingPimSmJoinPruneTimerInfo   joinTimer;
    RoutingPimSmJoinPruneType  joinPruneType =
        ROUTING_PIMSM_NOINFO_JOIN_PRUNE;

    unsigned int i;
    BOOL isSPT = FALSE ;
    BOOL IAmRP  = FALSE;

    if (treeInfo->numEntries != 0)
    {
        for (mapIterator = rowPtrMap->begin();
             mapIterator != rowPtrMap->end();
             mapIterator++)
        {
            rowPtr = mapIterator->second;
            if (rowPtr->treeState == ROUTING_PIMSM_SG)
            {
                isSPT = TRUE ;
                break;
            }
        }
    }
    else
    {
#ifdef EXATA
        if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE, "No Entry Found.\n");
        }
#else
        ERROR_Assert(FALSE, "No Entry Found.\n");
#endif
        return;
    }

    /* it checks to see if group G in its multicast treeInfo Base */

    if (treeInfoBaseRowPtr == NULL)
    {
#ifdef EXATA
        if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
        }
#else
        ERROR_Assert(FALSE,
                "JoinPrune_Pkt received so entry must exist\n");
#endif
        return;
    }

    /* check the protocolState & switch accrdingly */
    switch (treeInfoBaseRowPtr->treeState)
    {
        case ROUTING_PIMSM_G:
        {
            RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);
            for (i = 0; i < (unsigned int) node->numberInterfaces; i++)
            {
                if (RPAddr == pim->interface[i].ipAddress)
                {
                    IAmRP  = TRUE;
                    break ;
                }
            }

            if (IAmRP  == TRUE)
                return;
            // Get the next hop to RP
            RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, RPAddr,
            &outInterface, &nextHop);
            nextHop = RoutingPimSmSelectNewRPFG(node, groupAddr);

            if (outInterface == NETWORK_UNREACHABLE)
            {
                if (DEBUG)
                {
                    printf("Node %u has invalid outinterface towards"
                        " RP for group 0x%x\n",node->nodeId,groupAddr);
                }
                return;
            }

            joinPruneType = ROUTING_PIMSM_G_JOIN_PRUNE;
            break;
        }
        case ROUTING_PIMSM_SGrpt:
        {
            RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);
            for (i = 0; i < (unsigned int) node->numberInterfaces; i++)
            {
                if (RPAddr == pim->interface[i].ipAddress)
                {
                    IAmRP  = TRUE;
                    break ;
                }
            }

            if (IAmRP  == TRUE)
                return;
            // Get the next hop to RP
            RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                                RPAddr,
                                                                &outInterface,
                                                                &nextHop);

            nextHop = RoutingPimSmSelectNewRPFSGrpt(node,
                                                    groupAddr,
                                                    srcAddr);

            if (nextHop == 0)
            {
                nextHop = RPAddr ;
            }
            else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
            {
                RoutingPimSmGetNextHopOutInterface(node,
                                                   srcAddr,
                                                   &nextHop,
                                                   &outInterface);
            }

            if (outInterface == NETWORK_UNREACHABLE)
            {
                if (DEBUG)
                {
                    printf("Node %u has invalid outinterface towards"
                        " RP for group 0x%x\n",node->nodeId,groupAddr);
                }
                return;
            }

            joinPruneType = ROUTING_PIMSM_SGrpt_JOIN_PRUNE;
            break;

        }
        case ROUTING_PIMSM_SG:
        {
            RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                    srcAddr, &outInterface, &nextHop);
            nextHop = RoutingPimSmSelectNewRPFSG(node, groupAddr,srcAddr);

            if (nextHop == 0)
            {
                nextHop = srcAddr ;
                outInterface = NetworkIpGetInterfaceIndexForNextHop(
                                   node,
                                   srcAddr);
            }
            else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
            {
                RoutingPimSmGetNextHopOutInterface(node,
                                                   srcAddr,
                                                   &nextHop,
                                                   &outInterface);
            }
            if (outInterface == NETWORK_UNREACHABLE)
            {
                if (DEBUG)
                {
                    printf("Node %u has invalid outinterface towards"
                        " src for group 0x%x\n",node->nodeId,groupAddr);
                }
                return;
            }

            joinPruneType = ROUTING_PIMSM_SG_JOIN_PRUNE;
            break;
        }
        default:
        {
#ifdef EXATA
            if (!node->partitionData->isEmulationMode)
            {
                ERROR_Assert(FALSE, "Unknown multicast tree State"
                            " in PIMSM\n");
            }
#else
            ERROR_Assert(FALSE, "Unknown multicast tree State in PIMSM\n");
#endif
            return;
        }
    }   //end of switch;
    if (DEBUG_JP) {
        printf(" Node %u send joinPruneType %d join on Interface %d \n",
               node->nodeId, joinPruneType,outInterface);
        printf(" it has upstream join prune state JOIN as \
               join timer expired\n");
    }
    /* Routing PimSm Send Join Packet */
    RoutingPimSmSendJoinPrunePacket(node,
                                    srcAddr,
                                    nextHop,
                                    groupAddr,
                                    outInterface,
                                    joinPruneType,
                                    ROUTING_PIM_JOIN_TREE,
                                    treeInfoBaseRowPtr);

    joinTimer.srcAddr = srcAddr;
    joinTimer.grpAddr = groupAddr;
    joinTimer.intfIndex = (unsigned int)outInterface;
    joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
    joinTimer.treeState = treeInfoBaseRowPtr->treeState;

    /* Note the latest joinTimerSeq in treeInfo Base */
    treeInfoBaseRowPtr->joinTimerSeq++;

    delay = (clocktype) (pimDataSm->routingPimSmTPeriodicInterval);
    treeInfoBaseRowPtr->lastJoinTimerEnd = node->getNodeTime() + delay;
    RoutingPimSetTimer(node,
                       outInterface,
                       MSG_ROUTING_PimSmJoinTimerTimeout,
                       (void*) &joinTimer, delay);
}

/*
*  FUNCTION    : RoutingPimSmHandleAssertTimeOutEvent()
*  PURPOSE     : RoutingPimSmHandleAssertTimeOutEvent
*  RETURN VALUE: NULL
*/
void
RoutingPimSmHandleAssertTimeOutEvent(Node* node,
                                     NodeAddress srcAddr,
                                     NodeAddress groupAddr,
                                     int interfaceId,
                             RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr)
{
    RoutingPimSmDownstream* downStreamListPtr;

    if (DEBUG_ERRORS) {
        printf("Node %u handle AssertTimerTimeOut  \n",
               node->nodeId);
        printf("srcAddr: %x, dest: %x, interface: %d \n",
               srcAddr, groupAddr, interfaceId);
        printf(" associated tree state: %d \n",
               treeInfoBaseRowPtr->treeState);
    }

    if (treeInfoBaseRowPtr == NULL)
    {
#ifdef EXATA
        if (!node->partitionData->isEmulationMode)
        {
            ERROR_Assert(FALSE, "Assert_Pkt received so entry must exist\n");
        }
#else
        ERROR_Assert(FALSE, "Assert_Pkt received so entry must exist\n");
#endif
        return;
    }
    downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                        treeInfoBaseRowPtr, interfaceId);

    if (downStreamListPtr != NULL)
    {
        if (downStreamListPtr->assertState == PimSm_Assert_IWonAssert)
        {
            if (DEBUG_ERRORS) {
                printf("Node %u: move to winner state \n", node->nodeId);
                printf("Node %u: Winner PerformActionA3() \n",
                       node->nodeId);
            }
            RoutingPimSmPerformActionA3(node, groupAddr, srcAddr,
                                            interfaceId,
                                            treeInfoBaseRowPtr->treeState);
        }
        else if (downStreamListPtr->assertState == PimSm_Assert_ILostAssert)
        {
            downStreamListPtr->assertState = PimSm_Assert_NoInfo;
            RoutingPimSmPerformActionA5(node, groupAddr, srcAddr,
                                        interfaceId,
                                        treeInfoBaseRowPtr->treeState);
        }
    }
}




/*
*  FUNCTION    : RoutingPimSmSendAssertPacket()
*  PURPOSE     : RoutingPimSm Send Assert information through out Interface
*  RETURN VALUE: NULL;
*/

void
RoutingPimSmSendAssertPacket(Node* node, NodeAddress srcAddr,
                             NodeAddress grpAddr, int outIntfId,
                             RoutingPimSmAssertType assertType)
{

#ifdef ADDON_BOEINGFCS
    if (MulticastCesRpimActiveOnNode(node)) {
        return;
    }
#endif

    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    Message* assertMsg;
    RoutingPimSmAssertPacket* assertPkt;
    unsigned int size;

    /* Send assert for only one (*, S, G) pair */
    assertMsg = MESSAGE_Alloc(node,
                              NETWORK_LAYER,
                              MULTICAST_PROTOCOL_PIM,
                              MSG_ROUTING_PimPacket);

    size = sizeof(RoutingPimSmAssertPacket);

    MESSAGE_PacketAlloc(node, assertMsg, size, TRACE_PIM);

    assertPkt = (RoutingPimSmAssertPacket*)
                MESSAGE_ReturnPacket(assertMsg);

    /* Set the Header */
    RoutingPimCreateCommonHeader(&assertPkt->commonHeader,
                                 ROUTING_PIM_ASSERT);

    /* set the group information in packet field*/
    assertPkt->groupAddr.addressFamily = 1;
    assertPkt->groupAddr.encodingType = 0;
    assertPkt->groupAddr.reserved = 0;
    assertPkt->groupAddr.maskLength = 32;
    assertPkt->groupAddr.groupAddr = grpAddr;

    /* set the sorce information in packet field*/
    assertPkt->sourceAddr.addressFamily = 1;
    assertPkt->sourceAddr.encodingType = 0;

    /*
     *  Each group specific set may contain (*, G) and
     *  (S, G) source list entries in the Assert lists.
     */
    if (assertType == ROUTING_PIMSM_G_ASSERT)
    {
        NodeAddress RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);

        /* check the RP For this group & find associated nextHop*/
        int RPIntf = 0;
        NodeAddress nextHopForRP = 0;
        RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                            RPAddr,
                                                            &RPIntf,
                                                            &nextHopForRP);
        if (nextHopForRP == 0)
        {
            nextHopForRP = RPAddr ;
        }
        else if (nextHopForRP == (unsigned int)NETWORK_UNREACHABLE)
        {
            RoutingPimSmGetNextHopOutInterface(node,
                                               RPAddr,
                                               &nextHopForRP,
                                               &RPIntf);
        }

        if (RPIntf == NETWORK_UNREACHABLE)
        {
            if (DEBUG_FORWARD_PACKET)
            {
                printf("Node %u has invalid out interface towards 0x%x"
                       ,node->nodeId,RPAddr);
            }
            return;
        }
        setNodeAddressInCharArray(
            assertPkt->sourceAddr.unicastAddr, RPAddr);

        /* set metric in assert Pkt */
        PimSmAssertPacketSetRPTBit(&(assertPkt->metricBitPref), 1);
        NetworkRoutingProtocolType routingProtocolType =
            NetworkIpGetUnicastRoutingProtocolType(node, RPIntf);

        if (routingProtocolType == ROUTING_PROTOCOL_NONE)
        {
            PimSmAssertPacketSetMetricPref(&(assertPkt->metricBitPref),0);
        }
        else
        {
            PimSmAssertPacketSetMetricPref(&(assertPkt->metricBitPref),
                NetworkRoutingGetAdminDistance(node,
                NetworkIpGetUnicastRoutingProtocolType(node, RPIntf)));
        }

        assertPkt->metric = NetworkGetMetricForDestAddress(node, RPAddr);
    }
    else if (assertType == ROUTING_PIMSM_SG_ASSERT)
    {
        unsigned int srcIntf = outIntfId;
        setNodeAddressInCharArray(
        assertPkt->sourceAddr.unicastAddr, srcAddr);

        /* set metric in assert Pkt */
        PimSmAssertPacketSetRPTBit(&(assertPkt->metricBitPref), 0);
        NetworkRoutingProtocolType routingProtocolType =
            NetworkIpGetUnicastRoutingProtocolType(node, srcIntf);

        if (routingProtocolType == ROUTING_PROTOCOL_NONE)
        {
            PimSmAssertPacketSetMetricPref(&(assertPkt->metricBitPref),0);
        }
        else
        {
            PimSmAssertPacketSetMetricPref(&(assertPkt->metricBitPref),
                NetworkRoutingGetAdminDistance(node,
                NetworkIpGetUnicastRoutingProtocolType(node, srcIntf)));
        }

        assertPkt->metric = NetworkGetMetricForDestAddress(node, srcAddr);
    }
    else if (assertType == ROUTING_PIMSM_G_ASSERT_CANCEL)
    {
        setNodeAddressInCharArray(
        assertPkt->sourceAddr.unicastAddr, 0);

        /* set metric in assert Pkt as infinity */
        PimSmAssertPacketSetRPTBit(&(assertPkt->metricBitPref), 1);
        PimSmAssertPacketSetMetricPref(&(assertPkt->metricBitPref),
                        ROUTING_ADMIN_DISTANCE_DEFAULT);
        assertPkt->metric = 0xFFFFFFFF;
    }
    else if (assertType == ROUTING_PIMSM_SG_ASSERT_CANCEL)
    {
        setNodeAddressInCharArray(
            assertPkt->sourceAddr.unicastAddr, srcAddr);

        /* set metric in assert Pkt as infinity */
        PimSmAssertPacketSetRPTBit(&(assertPkt->metricBitPref),
                                                        0);
        PimSmAssertPacketSetMetricPref(&
                            (assertPkt->metricBitPref),
                             ROUTING_ADMIN_DISTANCE_DEFAULT);
        assertPkt->metric = 0xFFFFFFFF;
    }

    char* packet = MESSAGE_ReturnPacket(assertMsg);
    RoutingPimSetBufferFromAssertPacket(packet, assertPkt);

    /* assert pkt is for broadcast network only */
    if (((outIntfId >= 0) && ((unsigned)outIntfId != (unsigned)NETWORK_UNREACHABLE))
        && (pim->interface[outIntfId].interfaceType ==
            ROUTING_PIM_BROADCAST_INTERFACE))
    {
        TosType priority = IPTOS_PREC_INTERNETCONTROL;

#ifdef ADDON_BOEINGFCS
        if (MulticastCesRpimActiveOnInterface(node, outIntfId))
        {
            priority = IPTOS_PREC_PRIORITY;
        }
#endif

        /* Now send packet out through this interface */
        NetworkIpSendRawMessageToMacLayer(
            node,
            MESSAGE_Duplicate(node, assertMsg),
            NetworkIpGetInterfaceAddress(node,
                                         outIntfId),
            ALL_PIM_ROUTER,
            priority,
            IPPROTO_PIM,
            1,
            outIntfId,
            ANY_DEST);

        if (DEBUG)
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);

            printf("Node %d send Assert to intf %d at time %s\n",
                   node->nodeId, outIntfId, clockStr);
            printf(" the assertType is %d \n", assertType);
            printf(" pktSrc: %x  pktGrp: %u  \n",
                getNodeAddressFromCharArray
                (assertPkt->sourceAddr.unicastAddr),
                   assertPkt->groupAddr.groupAddr);
        }
        if (assertType == ROUTING_PIMSM_G_ASSERT)
        {
            pimDataSm->stats.numOfGAssertPacketForwarded++;
        }
        else if (assertType == ROUTING_PIMSM_SG_ASSERT)
        {
            pimDataSm->stats.numOfSGAssertPacketForwarded++;
        }
        else if (assertType == ROUTING_PIMSM_G_ASSERT_CANCEL)
        {
            pimDataSm->stats.numOfGAssertCancelPacketForwarded++;
        }
        else if (assertType == ROUTING_PIMSM_SG_ASSERT_CANCEL)
        {
            pimDataSm->stats.numOfSGAssertCancelPacketForwarded++;
        }
#ifdef ADDON_DB
        pimDataSm->smSummaryStats.m_NumAssertSent++;
#endif
    }   //if broadcast;
    MESSAGE_Free(node, assertMsg);
}

/*
*  FUNCTION    : RoutingPimSmHandleGAssertStateMachine()
*  PURPOSE     : After receiving a Assert Packet PimSm Handle
*                (*, G) Assert State Machine
*  RETURN VALUE: NULL
*/
void
RoutingPimSmHandleGAssertStateMachine(Node* node, NodeAddress srcAddr,
                                    RoutingPimSmAssertPacket* assertPkt,
                                    RoutingPimSmMulticastTreeState treeState,
                                    unsigned int interfaceId)
{
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr;
    RoutingPimSmDownstream* downStreamListPtr = NULL;
    RoutingPimSmAssertStrength  assertStrength;

    RoutingPimSmAssertMetric  myMetric;
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    if (DEBUG_ERRORS) {
        printf("Node %u: handle G assert from %u \n", node->nodeId, srcAddr);
        printf(" pktSrc %x pktGrp %u\n",
                getNodeAddressFromCharArray(
                assertPkt->sourceAddr.unicastAddr),
               assertPkt->groupAddr.groupAddr);
    }

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         assertPkt->groupAddr.groupAddr,
                         getNodeAddressFromCharArray
                         (assertPkt->sourceAddr.unicastAddr), treeState);

    if (treeInfoBaseRowPtr == NULL)
    {
        if (DEBUG_ERRORS) {
            printf("G_Assert_Pkt received so entry must exist\n");
        }
        return;
    }

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        /* check the assert state */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr == NULL)
    {
        if (interfaceId == (unsigned int)treeInfoBaseRowPtr->upInterface)
        {
            treeInfoBaseRowPtr->upAssertState = PimSm_Assert_ILostAssert;
            if ((RoutingPimSmComparePktAssertMetric(
                    PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref),
                    PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref),
                    assertPkt->metric, srcAddr,
                    treeInfoBaseRowPtr->metric.RPTbit,
                    treeInfoBaseRowPtr->metric.metricPreference,
                    treeInfoBaseRowPtr->metric.metric,
                    treeInfoBaseRowPtr->metric.ipAddress)))
            {
                treeInfoBaseRowPtr->metric.ipAddress = srcAddr;
                treeInfoBaseRowPtr->metric.metric = assertPkt->metric;
                treeInfoBaseRowPtr->metric.metricPreference =
                PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref);
                treeInfoBaseRowPtr->metric.RPTbit =
                    PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref);
                treeInfoBaseRowPtr->upAssertWinner = srcAddr;

                /*RPF’(*,G) changes due to an Assert*/
                treeInfoBaseRowPtr->nextHopForSrc = srcAddr;
                treeInfoBaseRowPtr->nextHopForRP = srcAddr;
                treeInfoBaseRowPtr->upstream = srcAddr;

                /* decrease timer to t_override */
                RoutingPimSmJoinPruneTimerInfo    joinTimer;
                clocktype delay;

                /* Routing PimSm decrease Timer to t_override */
                joinTimer.srcAddr = RoutingPimSmFindRPForThisGroup(node,
                                     assertPkt->groupAddr.groupAddr);
                joinTimer.grpAddr = assertPkt->groupAddr.groupAddr;
                joinTimer.intfIndex = (unsigned int)interfaceId;
                joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
                joinTimer.treeState = treeInfoBaseRowPtr->treeState;
                /* If the Join Timer is set to expire in more than t_override
                seconds, reset it so that it expires after t_override seconds.
                If the Join Timer is set to expire in less than t_override
                seconds, leave it unchanged.*/
                delay = (clocktype)(ROUTING_PIMSM_OVERRIDE_INTERVAL
                                      * RANDOM_erand(pimDataSm->seed));
                if ((node->getNodeTime() + delay) <
                     treeInfoBaseRowPtr->lastJoinTimerEnd)
                {
                    /* note the latest join timer Seq no */
                    treeInfoBaseRowPtr->joinTimerSeq++;
                    treeInfoBaseRowPtr->lastJoinTimerEnd = node->getNodeTime() +
                                                            delay;

                    RoutingPimSetTimer(node,
                                   interfaceId,
                                   MSG_ROUTING_PimSmJoinTimerTimeout,
                                   (void*) &joinTimer, delay);
                }
            }
        }
        return;
    }

    /* set my_assert_metric */
    RoutingPimSmSetMyAssert(node, assertPkt->groupAddr.groupAddr,
                            getNodeAddressFromCharArray (
                            assertPkt->sourceAddr.unicastAddr),
                            interfaceId, &myMetric);

    if (DEBUG_ERRORS) {
        printf(" Node %u: has assertPkt metric \n", node->nodeId);
        printf(" RPTbit %d \n", PimSmAssertPacketGetRPTBit
            (assertPkt->metricBitPref));
        printf(" metricPreference %d \n",
            PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref));
        printf(" metric %d \n", assertPkt->metric);
        printf(" metricIpAddress %x \n", srcAddr);
    }

    if (DEBUG_ERRORS) {
        printf(" Node %u: has it's my metric \n", node->nodeId);
        printf(" RPTbit %d \n", myMetric.RPTbit);
        printf(" metricPreference %d \n", myMetric.metricPreference);
        printf(" metric %d \n", myMetric.metric);
        printf(" metricIpAddress %x \n", myMetric.ipAddress);
    }

    if (DEBUG_ERRORS) {
        printf(" Node %u: has current assert winner metric \n",
               node->nodeId);
        printf(" RPTbit %d \n",
               downStreamListPtr->assertWinnerMetric.RPTbit);
        printf(" metricPreference %d \n",
               downStreamListPtr->assertWinnerMetric.metricPreference);
        printf(" metric %d \n",
               downStreamListPtr->assertWinnerMetric.metric);
        printf(" metricIpAddress %x \n",
               downStreamListPtr->assertWinnerMetric.ipAddress);
    }

    /* set the assert strength */
    if (RoutingPimSmComparePktAssertMetric(
        PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref),
        PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref),
        assertPkt->metric, srcAddr,
        myMetric.RPTbit, myMetric.metricPreference, myMetric.metric,
        myMetric.ipAddress) == FALSE)
    {
        if (DEBUG) {
            printf("Node %u: pktmetric is worse than myMetric \n",
                node->nodeId);
            /* An "inferior assert" is one with a worse packet-metric than
            my_assert_metric(*, G).  */
            printf(" it is inferior assert \n");
        }
        assertStrength = PIMSM_ASSERT_INFERIOR;
    }
    else if (RoutingPimSmComparePktAssertMetric(
                PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref),
                PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref),
                assertPkt->metric, srcAddr,
                downStreamListPtr->assertWinnerMetric.RPTbit,
                downStreamListPtr->assertWinnerMetric.metricPreference,
                downStreamListPtr->assertWinnerMetric.metric,
                downStreamListPtr->assertWinnerMetric.ipAddress))
    {
        if (DEBUG) {
            printf("Node %u: pktmetric is better than assertWinnerMetric"
                   "\n",node->nodeId);
            printf(" it is preffered assert \n");
        }
        /* An "preffered assert" is one with a better packet-metric than
        current_assert_winner_metric(*, G).  */
        assertStrength = PIMSM_ASSERT_PREFFERED;
    }
    else
    {
        if (DEBUG_ERRORS) {
            printf(" it is ordinary assert \n");
        }
        assertStrength = PIMSM_ASSERT_ORDINARY;
    }

    /* check the assert state & handle accordingly */
    if (DEBUG_ERRORS) {
        printf("Node %u has downstream assert state %d \n", node->nodeId,
               downStreamListPtr->assertState);
    }

    if (downStreamListPtr->assertState == PimSm_Assert_NoInfo)
    {
        /* Receieve InferiorAssert With RPTBit Set & CouldAssert(GI) */
        if ((assertStrength == PIMSM_ASSERT_INFERIOR)
            && (PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref) == 1)
                && (RoutingPimSmCouldAssertG(
                                     node,
                                     assertPkt->groupAddr.groupAddr,
                                     getNodeAddressFromCharArray(
                                     assertPkt->sourceAddr.unicastAddr),
                                     interfaceId)))
        {
            if (DEBUG) {
                printf("Node %u: move to winner state \n", node->nodeId);
                printf("Node %u: NoInfo PerformActionA1()\n", node->nodeId);
            }
            downStreamListPtr->assertState = PimSm_Assert_IWonAssert;
            RoutingPimSmPerformActionA1(node,
                                    assertPkt->groupAddr.groupAddr,
                                    getNodeAddressFromCharArray (
                                    assertPkt->sourceAddr.unicastAddr),
                                    interfaceId,
                                    treeState);

        }
          /* Receieve PrefferedAssert With RPTBit Set & AssTrDes(GI)*/
        else if (((assertStrength == PIMSM_ASSERT_PREFFERED)
                && (PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref)
                == 1)) &&
                (RoutingPimSmAssertTrackingDesiredG(node,
                                assertPkt->groupAddr.groupAddr,
                                getNodeAddressFromCharArray (
                                assertPkt->sourceAddr.unicastAddr),
                                interfaceId)))
        {
            downStreamListPtr->assertState = PimSm_Assert_ILostAssert;
            RoutingPimSmPerformActionA2(node, srcAddr, assertPkt,
                                        treeState, interfaceId);
        }
    }
    else if (downStreamListPtr->assertState == PimSm_Assert_IWonAssert)
    {
        /* receieve inferior assert */
        if (assertStrength == PIMSM_ASSERT_INFERIOR)
        {
            if (DEBUG) {
                printf("Node %u: move to winner state \n", node->nodeId);
                printf("Node %u: Winner PerformActionA3() \n",
                   node->nodeId);
            }
            downStreamListPtr->assertState = PimSm_Assert_IWonAssert;
            RoutingPimSmPerformActionA3(node,
                                assertPkt->groupAddr.groupAddr,
                                getNodeAddressFromCharArray (
                                assertPkt->sourceAddr.unicastAddr),
                                interfaceId, treeState);
        }
           /* receieve preffered assert */
        else if (assertStrength == PIMSM_ASSERT_PREFFERED)
        {
            downStreamListPtr->assertState = PimSm_Assert_ILostAssert;
            RoutingPimSmPerformActionA2(node, srcAddr, assertPkt,
                                        treeState, interfaceId);
        }
        else if (RoutingPimSmCouldAssertG(
                        node,
                        assertPkt->groupAddr.groupAddr,
                        getNodeAddressFromCharArray (
                        assertPkt->sourceAddr.unicastAddr),
                        interfaceId) == FALSE)
        {
            downStreamListPtr->assertState = PimSm_Assert_NoInfo;

            RoutingPimSmPerformActionA4(node,
                            assertPkt->groupAddr.groupAddr,
                            getNodeAddressFromCharArray (
                            assertPkt->sourceAddr.unicastAddr),
                            interfaceId, treeState);
        }
    }
    else if (downStreamListPtr->assertState == PimSm_Assert_ILostAssert)
    {
        /* receieve preffered assert */
        // with rpt bit set
        if (assertStrength == PIMSM_ASSERT_PREFFERED &&
                    PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref)
                        == 1)
        {
            downStreamListPtr->assertState =
                        PimSm_Assert_ILostAssert;
            RoutingPimSmPerformActionA2(node, srcAddr, assertPkt,
                                    treeState, interfaceId);
        }
         /* Receive Acceptable Assert from Current Winner
                     with RPTbit set */
        else if (assertStrength == PIMSM_ASSERT_ORDINARY
                         && (srcAddr == downStreamListPtr->assertWinner)
                         && (PimSmAssertPacketGetRPTBit
                            (assertPkt->metricBitPref) == 1))
         {
            /* We stay in Loser state and perform Actions
            A2 below.*/
            downStreamListPtr->assertState =
                PimSm_Assert_ILostAssert;
            RoutingPimSmPerformActionA2(node,
                                        srcAddr,
                                        assertPkt,
                                        treeState,
                                        interfaceId);
         }
         /* receieve inferior assert from current winner */
        else if ((assertStrength == PIMSM_ASSERT_INFERIOR)
                            && (srcAddr == downStreamListPtr->assertWinner))
        {
            downStreamListPtr->assertState = PimSm_Assert_NoInfo;
            RoutingPimSmPerformActionA5(node,
                        assertPkt->groupAddr.groupAddr,
                        getNodeAddressFromCharArray (
                        assertPkt->sourceAddr.unicastAddr),
                        interfaceId, treeState);
        }
         /* checking for assertTrackingDesiredG */
        else if (RoutingPimSmAssertTrackingDesiredG(node,
                       assertPkt->groupAddr.groupAddr,
                       getNodeAddressFromCharArray (
                       assertPkt->sourceAddr.unicastAddr), interfaceId)
                        == FALSE)
        {
            downStreamListPtr->assertState = PimSm_Assert_NoInfo;
            RoutingPimSmPerformActionA5(node,
                        assertPkt->groupAddr.groupAddr,
                        getNodeAddressFromCharArray (
                        assertPkt->sourceAddr.unicastAddr),
                        interfaceId, treeState);
        }
         /* my metric is better than winner's metric  */
        else if (RoutingPimSmComparePktAssertMetric(myMetric.RPTbit,
                       myMetric.metricPreference, myMetric.metric,
                       myMetric.ipAddress,
                       downStreamListPtr->assertWinnerMetric.RPTbit,
                       downStreamListPtr->assertWinnerMetric.metricPreference,
                       downStreamListPtr->assertWinnerMetric.metric,
                       downStreamListPtr->assertWinnerMetric.ipAddress))
        {
            if (DEBUG_ERRORS) {
                printf("Node %u: mymetric better than"
                    " assertWinnerMetric \n",node->nodeId);
                printf("Node %u: move to NoInfo state \n",
                        node->nodeId);
            }
            downStreamListPtr->assertState = PimSm_Assert_NoInfo;
            RoutingPimSmPerformActionA5(node,
                        assertPkt->groupAddr.groupAddr,
                        getNodeAddressFromCharArray (
                        assertPkt->sourceAddr.unicastAddr),
                        interfaceId, treeState);
        }
    }
    /* Note that some of these actions may cause the value of
     * JoinDesired(*,G) or RPF’(*,G)) to change, which could
     * cause further transitions in other state machines.*/
    RoutingPimSmHandleUpstreamStateMachine(
        node,
        getNodeAddressFromCharArray(assertPkt->sourceAddr.unicastAddr),
        interfaceId,
        assertPkt->groupAddr.groupAddr,
        treeInfoBaseRowPtr);

}

/*
*  FUNCTION    : RoutingPimSmHandleSGAssertStateMachine()
*  PURPOSE     : After receiving a Assert Packet PimSm Handle
*                (*, S, G) Assert State Machine
*  RETURN VALUE: NULL
*/
void
RoutingPimSmHandleSGAssertStateMachine(Node* node, NodeAddress srcAddr,
                                    RoutingPimSmAssertPacket* assertPkt,
                                    RoutingPimSmMulticastTreeState treeState,
                                    unsigned int interfaceId)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimSmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream* downStreamListPtr = NULL;
    RoutingPimSmAssertStrength  assertStrength;

    RoutingPimSmAssertMetric  myMetric;


    if (DEBUG_ERRORS) {
        printf("Node %u:handle SG assert from %x \n", node->nodeId, srcAddr);
        printf("pktSrc %x pktGrp %u\n", getNodeAddressFromCharArray (
            assertPkt->sourceAddr.unicastAddr),
               assertPkt->groupAddr.groupAddr);
    }

    /* it checks to see if group SG in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         assertPkt->groupAddr.groupAddr,
                         getNodeAddressFromCharArray (
                         assertPkt->sourceAddr.unicastAddr), treeState);
    if (treeInfoBaseRowPtr == NULL)
    {
        if (DEBUG_ERRORS) {
            printf("SG_Assert_Pkt received so entry must exist\n");
        }
        return;
    }
    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr == NULL)
    {
        if ((treeInfoBaseRowPtr != NULL) &&
            (interfaceId == (unsigned int)treeInfoBaseRowPtr->upInterface))
        {
            treeInfoBaseRowPtr->upAssertState = PimSm_Assert_ILostAssert;
            if ((RoutingPimSmComparePktAssertMetric(
                    PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref),
                    PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref),
                    assertPkt->metric, srcAddr,
                    treeInfoBaseRowPtr->metric.RPTbit,
                    treeInfoBaseRowPtr->metric.metricPreference,
                    treeInfoBaseRowPtr->metric.metric,
                    treeInfoBaseRowPtr->metric.ipAddress)))
            {
                treeInfoBaseRowPtr->metric.ipAddress = srcAddr;
                treeInfoBaseRowPtr->metric.metric = assertPkt->metric;
                treeInfoBaseRowPtr->metric.metricPreference =
                PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref);
                treeInfoBaseRowPtr->metric.RPTbit =
                    PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref);
                treeInfoBaseRowPtr->upAssertWinner = srcAddr;

                /*RPF’(S,G) changes due to an Assert*/
                treeInfoBaseRowPtr->nextHopForSrc = srcAddr;
                treeInfoBaseRowPtr->upstream = srcAddr;

                /* decrease timer to t_override */
                RoutingPimSmJoinPruneTimerInfo    joinTimer;
                clocktype delay;

                /* Routing PimSm decrease Timer to t_override */
                joinTimer.srcAddr = getNodeAddressFromCharArray (
                         assertPkt->sourceAddr.unicastAddr);
                joinTimer.grpAddr = assertPkt->groupAddr.groupAddr;
                joinTimer.intfIndex = (unsigned int)interfaceId;
                joinTimer.seqNo = treeInfoBaseRowPtr->joinTimerSeq + 1;
                joinTimer.treeState = treeInfoBaseRowPtr->treeState;
                /* If the Join Timer is set to expire in more than
                t_override seconds, reset it so that it expires after
                t_override seconds. If the Join Timer is set to expire in
                less than t_override seconds, leave it unchanged.*/

                delay = (clocktype)(ROUTING_PIMSM_OVERRIDE_INTERVAL
                                    *  RANDOM_erand(pimSmData->seed));
                if ((node->getNodeTime() + delay) <
                     treeInfoBaseRowPtr->lastJoinTimerEnd)
                {
                    /* note the latest join timer Seq no */
                    treeInfoBaseRowPtr->joinTimerSeq++;
                    treeInfoBaseRowPtr->lastJoinTimerEnd =
                                        node->getNodeTime() + delay;
                    RoutingPimSetTimer(node,
                                   interfaceId,
                                   MSG_ROUTING_PimSmJoinTimerTimeout,
                                   (void*) &joinTimer, delay);
                }
            }
        }
        return;
    }

    /* set my_assert_metric */
    RoutingPimSmSetMyAssert(node, assertPkt->groupAddr.groupAddr,
                            getNodeAddressFromCharArray (
                            assertPkt->sourceAddr.unicastAddr),
                            interfaceId, &myMetric);

    if (DEBUG) {
        printf(" Node %u: has assertPkt metric \n", node->nodeId);
        printf(" RPTbit %d \n",PimSmAssertPacketGetRPTBit
            (assertPkt->metricBitPref));
        printf(" metricPreference %d \n",
            PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref));
        printf(" metric %d \n", assertPkt->metric);
        printf(" metricIpAddress %x \n", srcAddr);

        printf(" Node %u: has it's my metric \n", node->nodeId);
        printf(" RPTbit %d \n", myMetric.RPTbit);
        printf(" metricPreference %d \n", myMetric.metricPreference);
        printf(" metric %d \n", myMetric.metric);
        printf(" metricIpAddress %x \n", myMetric.ipAddress);

        printf(" Node %u: has current assert winner metric \n",
               node->nodeId);
        printf(" RPTbit %d \n",
               downStreamListPtr->assertWinnerMetric.RPTbit);
        printf(" metricPreference %d \n",
               downStreamListPtr->assertWinnerMetric.metricPreference);
        printf(" metric %d \n",
               downStreamListPtr->assertWinnerMetric.metric);
        printf(" metricIpAddress %x \n",
               downStreamListPtr->assertWinnerMetric.ipAddress);
    }

    /* set the assert strength */
    if (RoutingPimSmComparePktAssertMetric
        (PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref),
         PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref),
         assertPkt->metric, srcAddr,
         myMetric.RPTbit, myMetric.metricPreference, myMetric.metric,
         myMetric.ipAddress) == FALSE)
    {
        if (DEBUG_ERRORS) {
            printf("Node %u: pktmetric is worse than myMetric \n",
                   node->nodeId);

            /* An "inferior assert" is one with a worse packet-metric than
             my_assert_metric(*, G).  */
            printf(" it is inferior assert \n");
        }
        assertStrength = PIMSM_ASSERT_INFERIOR;
    }
    else if (RoutingPimSmComparePktAssertMetric
               (PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref),
                   PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref),
                   assertPkt->metric, srcAddr,
                   downStreamListPtr->assertWinnerMetric.RPTbit,
                   downStreamListPtr->assertWinnerMetric.metricPreference,
                   downStreamListPtr->assertWinnerMetric.metric,
                   downStreamListPtr->assertWinnerMetric.ipAddress))
    {
        if (DEBUG) {
            printf("Node %u: pktmetric is better than assertWinnerMetric"
                "\n",node->nodeId);
            printf(" it is preffered assert \n");
        }

        /* An "preffered assert" is one with a better packet-metric than
         current_assert_winner_metric(*, G).  */
        assertStrength = PIMSM_ASSERT_PREFFERED;
    }
    else
    {
        if (DEBUG_ERRORS) {
            printf(" it is ordinary assert \n");
        }
        assertStrength = PIMSM_ASSERT_ORDINARY;
    }

    /* check the assert state & handle accordingly */
    if (DEBUG_ERRORS) {
        printf("Node %u has downstream assert state %d \n", node->nodeId,
               downStreamListPtr->assertState);
    }

    if (downStreamListPtr->assertState == PimSm_Assert_NoInfo)
    {
        /* Receieve InferiorAssert With RPTBit clear & CouldAssert(SGI) */
        if (((assertStrength == PIMSM_ASSERT_INFERIOR)
            && (PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref) == 0))
                && (RoutingPimSmCouldAssertSG(node,
                                    assertPkt->groupAddr.groupAddr,
                                    getNodeAddressFromCharArray(
                                    assertPkt->sourceAddr.unicastAddr),
                                    interfaceId)))
        {
            if (DEBUG_ERRORS) {
                printf("Node %u: move to winner state \n", node->nodeId);
                printf("Node %u: NoInfo PerformActionA1()\n", node->nodeId);
            }
            downStreamListPtr->assertState = PimSm_Assert_IWonAssert;
            RoutingPimSmPerformActionA1(
                node,
                assertPkt->groupAddr.groupAddr,
                getNodeAddressFromCharArray(
                assertPkt->sourceAddr.unicastAddr),
                interfaceId,
                treeState);

        }
          /* Receieve PrefferedAssert With RPTBit clear & AssTrDes(SGI)*/
        else if (((assertStrength == PIMSM_ASSERT_PREFFERED)
                && (PimSmAssertPacketGetRPTBit(
                assertPkt->metricBitPref) == 0)) && (
                RoutingPimSmAssertTrackingDesiredSG(
                node,
                assertPkt->groupAddr.groupAddr,
                getNodeAddressFromCharArray (
                assertPkt->sourceAddr.unicastAddr),
                interfaceId)))
        {
            downStreamListPtr->assertState = PimSm_Assert_ILostAssert;
            RoutingPimSmPerformActionA6(node, srcAddr, assertPkt,
                                        treeState, interfaceId);
        }
         /*Receive Assert with rpt bit set&could assert(S, G) true */
        else if ((PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref)
                == 1)&& (
                RoutingPimSmCouldAssertSG(
                    node,
                    assertPkt->groupAddr.groupAddr,
                    getNodeAddressFromCharArray(
                    assertPkt->sourceAddr.unicastAddr),
                    interfaceId)))
        {
            if (DEBUG_ERRORS) {
                printf("Node %u: move to winner state \n", node->nodeId);
                printf("Node %u: NoInfo PerformActionA1()\n",node->nodeId);
            }
            downStreamListPtr->assertState = PimSm_Assert_IWonAssert;
            RoutingPimSmPerformActionA1(
                node,
                assertPkt->groupAddr.groupAddr,
                getNodeAddressFromCharArray(
                assertPkt->sourceAddr.unicastAddr),
                interfaceId,
                treeState);
        }
    }
    else if (downStreamListPtr->assertState == PimSm_Assert_IWonAssert)
    {
        /* receieve inferior assert */
        if (assertStrength == PIMSM_ASSERT_INFERIOR)
        {
            if (DEBUG_ERRORS)
            {
                printf("Node %u: move to winner state \n", node->nodeId);
                printf("Node %u: Winner PerformActionA3() \n",
                   node->nodeId);
            }
            downStreamListPtr->assertState = PimSm_Assert_IWonAssert;
            RoutingPimSmPerformActionA3(node,
                                assertPkt->groupAddr.groupAddr,
                                getNodeAddressFromCharArray(
                                assertPkt->sourceAddr.unicastAddr),
                                interfaceId, treeState);
        }
           /* receieve preffered assert */
        else if (assertStrength == PIMSM_ASSERT_PREFFERED)
        {
            downStreamListPtr->assertState = PimSm_Assert_ILostAssert;
            RoutingPimSmPerformActionA2(node, srcAddr, assertPkt,
                                        treeState, interfaceId);
        }
        //else       /* checking for could assert SGI  */
        else if (RoutingPimSmCouldAssertSG(
                            node,
                            assertPkt->groupAddr.groupAddr,
                            getNodeAddressFromCharArray (
                            assertPkt->sourceAddr.unicastAddr),
                            interfaceId) == FALSE)
        {
            downStreamListPtr->assertState = PimSm_Assert_NoInfo;
            RoutingPimSmPerformActionA4(node,
                                assertPkt->groupAddr.groupAddr,
                                getNodeAddressFromCharArray (
                                assertPkt->sourceAddr.unicastAddr),
                                interfaceId, treeState);
        }
    }
    else if (downStreamListPtr->assertState == PimSm_Assert_ILostAssert)
    {
        /* receieve preffered assert */
        if (assertStrength == PIMSM_ASSERT_PREFFERED)
        {
            downStreamListPtr->assertState = PimSm_Assert_ILostAssert;
            RoutingPimSmPerformActionA2(node, srcAddr, assertPkt,
                                        treeState, interfaceId);
        }
         /* receieve inferior assert from current winner */
        else if ((assertStrength == PIMSM_ASSERT_INFERIOR)
                    && (srcAddr == downStreamListPtr->assertWinner))
        {
            downStreamListPtr->assertState = PimSm_Assert_NoInfo;
            RoutingPimSmPerformActionA5(node,
                        assertPkt->groupAddr.groupAddr,
                        getNodeAddressFromCharArray (
                        assertPkt->sourceAddr.unicastAddr),
                        interfaceId, treeState);
        }
        /* Receive Preffered Assert with RPTbit clear from
        Current Winner*/
        else if ((assertStrength == PIMSM_ASSERT_PREFFERED)
             && (PimSmAssertPacketGetRPTBit(
             assertPkt->metricBitPref) == 0) &&
             (srcAddr == downStreamListPtr->assertWinner))
        {
            /*We stay in Loser state and perform Actions A2 below.*/
            downStreamListPtr->assertState = PimSm_Assert_ILostAssert;
            RoutingPimSmPerformActionA2(node, srcAddr, assertPkt,
                            treeState, interfaceId);
        }
        /* checking for assertTrackingDesiredSG */
        else if (RoutingPimSmAssertTrackingDesiredSG(
                    node,
                    assertPkt->groupAddr.groupAddr,
                    getNodeAddressFromCharArray (
                    assertPkt->sourceAddr.unicastAddr),
                    interfaceId) == FALSE)
        {
            downStreamListPtr->assertState = PimSm_Assert_NoInfo;
            RoutingPimSmPerformActionA5(node,
                            assertPkt->groupAddr.groupAddr,
                            getNodeAddressFromCharArray (
                            assertPkt->sourceAddr.unicastAddr),
                            interfaceId, treeState);
        }
        /* my metric is better than winner's metric  */
        else if (RoutingPimSmComparePktAssertMetric(
           myMetric.RPTbit,
           myMetric.metricPreference,
           myMetric.metric,
           myMetric.ipAddress,
           downStreamListPtr->assertWinnerMetric.RPTbit,
           downStreamListPtr->assertWinnerMetric.metricPreference,
           downStreamListPtr->assertWinnerMetric.metric,
           downStreamListPtr->assertWinnerMetric.ipAddress))
        {
            if (DEBUG_ERRORS) {
                printf("Node %u: mymetric better than "
                    "assertWinnerMetric \n",node->nodeId);
            }
            downStreamListPtr->assertState = PimSm_Assert_NoInfo;
            RoutingPimSmPerformActionA5(node,
                            assertPkt->groupAddr.groupAddr,
                            getNodeAddressFromCharArray (
                            assertPkt->sourceAddr.unicastAddr),
                            interfaceId, treeState);
        }
    }
    /* Note that some of these actions may cause the
     * value of JoinDesired(S,G),PruneDesired(S,G,rpt), or RPF’(S,G)
     * to change, which could cause further transitions in
     * other state machines.*/
    RoutingPimSmHandleUpstreamStateMachine(
       node,
       getNodeAddressFromCharArray(assertPkt->sourceAddr.unicastAddr),
       interfaceId,
       assertPkt->groupAddr.groupAddr,
       treeInfoBaseRowPtr);

}


/*
*  FUNCTION     : RoutingPimSmCouldAssertG()
*  PURPOSE      : Check if the router will send assert for this group;
*  RETURN VALUE : TRUE if condition satisfies
*/
BOOL
RoutingPimSmCouldAssertG(Node* node, NodeAddress grpAddr,
                         NodeAddress srcAddr, int interfaceId)
{
    NodeAddress RPAddr;
    int RPInterface;
    NodeAddress nextHopForRP = 0;

    /* check the RP For this group & find associated nextHop*/
    RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        RPAddr,
                                                        &RPInterface,
                                                        &nextHopForRP);
    if (nextHopForRP == 0)
    {
        nextHopForRP = RPAddr ;
    }
    else if (nextHopForRP == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHopForRP,
                                           &RPInterface);
    }

    if (RPInterface == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,RPAddr);
        }
        return FALSE;
    }
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (RPAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            RPInterface = CPU_INTERFACE;
            break;
        }
    }
    /*if ((I in (joins(*,* , RP(G)) (+) joins(*, G) (+) pim_include(*, G)))
     *      && (RPInterface != interfaceId)
     */

    BOOL sameInt = TRUE;
    if (interfaceId != RPInterface)
    {
        sameInt = FALSE;
    }

    if (((RoutingPimSmJoinRP(node, grpAddr, srcAddr, interfaceId))
            || (RoutingPimSmJoinG(node, grpAddr, srcAddr, interfaceId))
            || (RoutingPimSmIncludeG(node, grpAddr, srcAddr, interfaceId)))
            && (!sameInt))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
*  FUNCTION     : RoutingPimSmCouldAssertSG()
*  PURPOSE      : Check if the router will send assert for this group.
*  RETURN VALUE : TRUE if condition satisfies
*/
BOOL
RoutingPimSmCouldAssertSG(Node* node, NodeAddress grpAddr,
                          NodeAddress srcAddr, int interfaceId)
{
    RoutingPimSmTreeInfoBaseRow* routTableRowPtr;

    int srcInterface = 0;
    NodeAddress nextHopForSrc = 0;
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        srcAddr,
                                                        &srcInterface,
                                                        &nextHopForSrc);
    if (nextHopForSrc == 0)
    {
        nextHopForSrc = srcAddr ;
    }
    else if (nextHopForSrc == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           srcAddr,
                                           &nextHopForSrc,
                                           &srcInterface);
    }

    if (srcInterface == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,srcAddr);
        }
        return FALSE;
    }

    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (srcAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            srcInterface = CPU_INTERFACE;
            break;
        }
    }
    /* it checks to see if S-G in its multicast treeInfo Base */
    routTableRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(
                          node,
                          grpAddr,
                          srcAddr,
                          ROUTING_PIMSM_SG);

    if (routTableRowPtr == NULL)
    {
        if (DEBUG_ERRORS) {
            printf("SG_Pkt received so entry must exist\n");
        }
    }
    /* check if the interface present in the downstream list */
    if (routTableRowPtr != NULL)
    {
        BOOL sameInt = TRUE;

        if (srcInterface != interfaceId)
        {
            sameInt = FALSE;
        }

        if ((routTableRowPtr->SPTbit == TRUE)
                && (!sameInt)
                && ((RoutingPimSmJoinRP(node, grpAddr, srcAddr, interfaceId))
                  || (RoutingPimSmJoinG(node, grpAddr, srcAddr, interfaceId))
                    && (!RoutingPimSmPruneSGrpt(node, grpAddr, srcAddr,
                                                interfaceId))
                    || (RoutingPimSmIncludeG(node,
                                             grpAddr, srcAddr, interfaceId))
                    && (!RoutingPimSmExcludeSG(node,
                                               grpAddr,
                                               srcAddr, interfaceId))
                    && (!RoutingPimSmLostAssertG(node,
                                               grpAddr,srcAddr, interfaceId))
                    || (RoutingPimSmJoinSG(node,
                                           grpAddr, srcAddr, interfaceId))
                    || (RoutingPimSmIncludeSG(node,
                                              grpAddr, srcAddr, interfaceId)
                                              )))
        {
            if (DEBUG_ERRORS) {
                printf(" Node %u:could assert is TRUE\n", node->nodeId);
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmJoinG()
*  PURPOSE     : Check if the router wants to Join For This Group
*  RETURN VALUE: TRUE if condition satisfies
*/
BOOL
RoutingPimSmJoinG(Node* node, NodeAddress groupAddr,
                  NodeAddress srcAddr, int interfaceId)
{
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr;
    RoutingPimSmDownstream* downStreamListPtr;

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         groupAddr, srcAddr, ROUTING_PIMSM_G);

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        /* check the downstream state */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
        if (downStreamListPtr != NULL)
        {
            if (DEBUG_ERRORS) {
                printf(" downstreamState is %d\n",
                       downStreamListPtr->joinPruneState);
            }
            /* check the associated state */
            if ((downStreamListPtr->joinPruneState == PimSm_JoinPrune_Join)
                    || (downStreamListPtr->joinPruneState ==
                        PimSm_JoinPrune_PrunePending))
            {
                if (DEBUG_ERRORS) {
                    printf("Node %u: intf %d return true:joinG \n",
                           node->nodeId, interfaceId);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmJoinSG()
*  PURPOSE     : Check if the router wants to Join For This SorceGroup Pair
*  RETURN VALUE: TRUE if condition satisfies
*/

BOOL
RoutingPimSmJoinSG(Node* node, NodeAddress groupAddr,
                   NodeAddress srcAddr, int interfaceId)
{
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr;
    RoutingPimSmDownstream* downStreamListPtr;

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         groupAddr, srcAddr, ROUTING_PIMSM_SG);

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        /* check the downstream state */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
        if (downStreamListPtr != NULL)
        {
            /* check the associated state */
            if ((downStreamListPtr->joinPruneState == PimSm_JoinPrune_Join)
                    || (downStreamListPtr->joinPruneState ==
                        PimSm_JoinPrune_PrunePending))
            {
                if (DEBUG_ERRORS) {
                    printf("Node %u: intf %d return true:joinSG \n",
                           node->nodeId, interfaceId);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmJoinRP()
*  PURPOSE     : Check if the router wants to Join For This RP
*  RETURN VALUE: TRUE if condition satisfies
*/

BOOL
RoutingPimSmJoinRP(Node* node, NodeAddress groupAddr,
                   NodeAddress srcAddr, int interfaceId)
{
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr;
    RoutingPimSmDownstream* downStreamListPtr;

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         groupAddr, srcAddr, ROUTING_PIMSM_RP);

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        /* check the downstream state */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
        if (downStreamListPtr != NULL)
        {
            /* check the associated state */
            if ((downStreamListPtr->joinPruneState == PimSm_JoinPrune_Join)
                    || (downStreamListPtr->joinPruneState ==
                        PimSm_JoinPrune_PrunePending))
            {
                if (DEBUG_ERRORS) {
                    printf("Node %u: intf %d return true:joinRP \n",
                           node->nodeId, interfaceId);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmPruneSGrpt()
*  PURPOSE     : Check if the router wants to Join For This SGrpt entry
*  RETURN VALUE: TRUE if condition satisfies
*/
BOOL
RoutingPimSmPruneSGrpt(Node* node, NodeAddress groupAddr,
                       NodeAddress srcAddr, int interfaceId)
{
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr;
    RoutingPimSmDownstream* downStreamListPtr;

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         groupAddr, srcAddr, ROUTING_PIMSM_SGrpt);

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        /* check the downstream state */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
        if (downStreamListPtr != NULL)
        {
            /* check the associated state */
            if ((downStreamListPtr->joinPruneState == PimSm_JoinPrune_Pruned)
                    || (downStreamListPtr->joinPruneState ==
                        PimSm_JoinPrune_Temp_Pruned))
            {
                if (DEBUG_ERRORS) {
                    printf("Node %u: intf %d return true:pruneSGrpt \n",
                           node->nodeId, interfaceId);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}


/*
*  FUNCTION    : RoutingPimSmSetMyAssert()
*  PURPOSE     : RoutingPimSmSetMyAssert Parameter
*  RETURN VALUE: TRUE if condition satisfies
*/
void
RoutingPimSmSetMyAssert(Node* node, NodeAddress groupAddr,
                        NodeAddress srcAddr, int interfaceId,
                        RoutingPimSmAssertMetric * myMetric)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    /* check the RP For this group & find associated nextHop*/
    if (DEBUG) {
        printf("Node %u: setting my metric for srcAddr %u & grpAddr %u \n",
               node->nodeId, srcAddr, groupAddr);
    }

    if (RoutingPimSmCouldAssertSG(node, groupAddr,
                                  srcAddr, interfaceId))
    {
        unsigned int srcIntf = interfaceId;
        if (DEBUG) {
            printf("Node %u: return spt_assert_metric(S, G, I)\n",
                   node->nodeId);
        }

        /* set my assert metric */
        myMetric->RPTbit = 0;
        NetworkRoutingProtocolType routingProtocolType =
            NetworkIpGetUnicastRoutingProtocolType(node, srcIntf);

        if (routingProtocolType == ROUTING_PROTOCOL_NONE)
        {
            myMetric->metricPreference = 0;
        }
        else
        {
            myMetric->metricPreference = NetworkRoutingGetAdminDistance(node,
                                     NetworkIpGetUnicastRoutingProtocolType
                                     (node, srcIntf));
        }

        myMetric->metric = NetworkGetMetricForDestAddress(node, srcAddr);
        myMetric->ipAddress = pim->interface[interfaceId].ipAddress;
    }
    else if (RoutingPimSmCouldAssertG(node, groupAddr,
                                     srcAddr, interfaceId))
    {
        NodeAddress RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);
        int RPIntf = 0;
        NodeAddress nextHopForRP = 0;
        RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                            RPAddr,
                                                            &RPIntf,
                                                            &nextHopForRP);
        if (nextHopForRP == 0)
        {
            nextHopForRP = RPAddr ;
        }
        else if (nextHopForRP == (unsigned int)NETWORK_UNREACHABLE)
        {
            RoutingPimSmGetNextHopOutInterface(node,
                                               RPAddr,
                                               &nextHopForRP,
                                               &RPIntf);
        }

        if (RPIntf == NETWORK_UNREACHABLE)
        {
            if (DEBUG_FORWARD_PACKET)
            {
                printf("Node %u has invalid out interface towards 0x%x"
                       ,node->nodeId,RPAddr);
            }
            return;
        }

        if (DEBUG) {
            printf("Node %u:return rpt_assert_metric(G,I)\n",node->nodeId);
        }

        /* set my assert metric */
        myMetric->RPTbit = 1;

        NetworkRoutingProtocolType routingProtocolType =
            NetworkIpGetUnicastRoutingProtocolType(node, RPIntf);

        if (routingProtocolType == ROUTING_PROTOCOL_NONE)
        {
            myMetric->metricPreference = 0;
        }
        else
        {
            myMetric->metricPreference = NetworkRoutingGetAdminDistance(node,
                                  NetworkIpGetUnicastRoutingProtocolType(
                                  node, RPIntf));
        }

        myMetric->metric = NetworkGetMetricForDestAddress(node, RPAddr);
        myMetric->ipAddress = pim->interface[interfaceId].ipAddress;
    }
    else
    {
        if (DEBUG) {
            printf("Node %u: return infinite_assert_metric()\n",
                   node->nodeId);
        }

        /* set my assert metric */
        myMetric->RPTbit = 1;
        myMetric->metricPreference = ROUTING_ADMIN_DISTANCE_DEFAULT;
        myMetric->metric = 0xFFFFFFFF;
        myMetric->ipAddress = (unsigned)NETWORK_UNREACHABLE;

    }
}

/*
*  FUNCTION    : RoutingPimSmComparePktAssertMetric()
*  PURPOSE     : RoutingPimSm Compare Between pktMetric & othermetric
*  RETURN VALUE: TRUE if condition satisfies
*/
BOOL
RoutingPimSmComparePktAssertMetric(BOOL pktRPTbit,
                                   unsigned int pktPreference,
                                   unsigned int pktMetric,
                                   NodeAddress pktAddr,
                                   BOOL myRPTbit,
                                   unsigned int myPreference,
                                   unsigned int myMetric,
                                   NodeAddress myAddr)
{
    if (pktRPTbit < myRPTbit)
    {
        return TRUE;
    }
    else if (pktRPTbit > myRPTbit)
    {
        return FALSE;
    }
    else if (pktPreference < myPreference)
    {
        return TRUE;
    }
    else if (pktPreference > myPreference)
    {
        return FALSE;
    }
    else if (pktMetric < myMetric)
    {
        return TRUE;
    }
    else if (pktMetric > myMetric)
    {
        return FALSE;
    }
    else if (pktAddr > myAddr)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
*  FUNCTION    : RoutingPimSmPerformActionA1()
*  PURPOSE     : Routing PimSm Perform Action
*                  Send Assert
*                  Set timer to (Assert_Time - Assert_Override_Interval)
*                  Store self as AssertWinner.
*                  Store rpt_assert_metric as AssertWinnerMetric.
*  RETURN VALUE: NONE
*/
void
RoutingPimSmPerformActionA1(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId,
                            RoutingPimSmMulticastTreeState treeState)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmAssertTimerInfo   timerInfo;
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;
    clocktype delay;

    NodeAddress RPAddr;
    int RPIntf = 0;
    NodeAddress nextHopForRP = 0;
    unsigned int srcIntf;

    srcIntf = interfaceId;

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, treeState);

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        /* check the assert state */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr == NULL)
    {
        return;
    }
    if (treeState == ROUTING_PIMSM_G)
    {
        /* Send (*.G) Assert */
        RoutingPimSmSendAssertPacket(node, srcAddr, grpAddr,
                                     interfaceId, ROUTING_PIMSM_G_ASSERT);

        /*  Store self as AssertWinner */
        if (DEBUG) {
            printf(" Node %u: Store self as AssertWinner\n", node->nodeId);
        }

        downStreamListPtr->assertWinner =
            pim->interface[interfaceId].ipAddress;

        /* Store rpt_assert_metric as AssertWinnerMetric(*, G, I). */
        downStreamListPtr->assertWinnerMetric.RPTbit = 1;
        RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);
        RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                            RPAddr,
                                                            &RPIntf,
                                                            &nextHopForRP);
        if (nextHopForRP == 0)
        {
            nextHopForRP = RPAddr ;
        }
        else if (nextHopForRP == (unsigned int)NETWORK_UNREACHABLE)
        {
            RoutingPimSmGetNextHopOutInterface(node,
                                               RPAddr,
                                               &nextHopForRP,
                                               &RPIntf);
        }

        if (RPIntf == NETWORK_UNREACHABLE)
        {
            if (DEBUG_FORWARD_PACKET)
            {
                printf("Node %u has invalid out interface towards 0x%x"
                       ,node->nodeId,RPAddr);
            }
            return;
        }
        NetworkRoutingProtocolType routingProtocolType =
            NetworkIpGetUnicastRoutingProtocolType(node, RPIntf);

        if (routingProtocolType == ROUTING_PROTOCOL_NONE)
        {
            downStreamListPtr->assertWinnerMetric.metricPreference = 0;
        }
        else
        {
            downStreamListPtr->assertWinnerMetric.metricPreference =
            NetworkRoutingGetAdminDistance(node,
                                      NetworkIpGetUnicastRoutingProtocolType(
                                      node, RPIntf));
        }

        downStreamListPtr->assertWinnerMetric.metric =
            NetworkGetMetricForDestAddress(node, RPAddr);
        downStreamListPtr->assertWinnerMetric.ipAddress =
            pim->interface[interfaceId].ipAddress;
    }
    else if (treeState == ROUTING_PIMSM_SG)
    {
        /* Send (*, S.G) Assert */
        RoutingPimSmSendAssertPacket(node, srcAddr, grpAddr,
                                     interfaceId,
                                     ROUTING_PIMSM_SG_ASSERT);

        /*  Store self as AssertWinner */
        if (DEBUG_ERRORS) {
            printf("Node %u: Store self as AssertWinner\n",node->nodeId);
        }
        downStreamListPtr->assertWinner =
            pim->interface[interfaceId].ipAddress;

        /* Store spt_assert_metric as AssertWinnerMetric(*, G, I). */
        downStreamListPtr->assertWinnerMetric.RPTbit = 0;

        NetworkRoutingProtocolType routingProtocolType =
            NetworkIpGetUnicastRoutingProtocolType(node, srcIntf);

        if (routingProtocolType == ROUTING_PROTOCOL_NONE)
        {
            downStreamListPtr->assertWinnerMetric.metricPreference = 0;
        }
        else
        {
            downStreamListPtr->assertWinnerMetric.metricPreference =
            NetworkRoutingGetAdminDistance(node,
                                  NetworkIpGetUnicastRoutingProtocolType(
                                  node,
                                  srcIntf));
        }

        downStreamListPtr->assertWinnerMetric.metric =
            NetworkGetMetricForDestAddress(node, srcAddr);
        downStreamListPtr->assertWinnerMetric.ipAddress =
            pim->interface[interfaceId].ipAddress;
    }

    delay = (clocktype)
            (pimDataSm->routingPimSmAssertTimeout
              - ROUTING_PIMSM_ASSERT_OVERRIDE_INTERVAL);

    if (downStreamListPtr->assertTime == 0)
    {
        /* Set timer to (Assert_Time - Assert_Override_Interval) */
        timerInfo.srcAddr = srcAddr;
        timerInfo.grpAddr = grpAddr;
        timerInfo.intfIndex = interfaceId;
        timerInfo.treeState = treeState;
        RoutingPimSetTimer(node,
                       interfaceId,
                       MSG_ROUTING_PimSmAssertTimerTimeout,
                       (void*) &timerInfo, delay);
    }

    downStreamListPtr->assertTime = node->getNodeTime() + delay;
}

/*
*  FUNCTION    : RoutingPimSmPerformActionA2()
*  PURPOSE     : Routing PimSm Perform Action:
*                Store new assert winner as AssertWinner(*, G, I) and assert
*                winner metric as AssertWinnerMetric(*, G, I).
*                Set timer to assert_time
*  RETURN VALUE: NONE
*/
void
RoutingPimSmPerformActionA2(Node* node, NodeAddress assertSender,
                            RoutingPimSmAssertPacket* assertPkt,
                            RoutingPimSmMulticastTreeState treeState,
                            unsigned int interfaceId)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmAssertTimerInfo   timerInfo;
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;
    clocktype delay;


    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         assertPkt->groupAddr.groupAddr,
                         getNodeAddressFromCharArray (
                         assertPkt->sourceAddr.unicastAddr), treeState);

    if (treeInfoBaseRowPtr != NULL)
    {
        /* check if the interface present in the downstream list */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                        treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr == NULL)
    {
        return;
    }

    /*  Store new assert winner as AssertWinner */
    if (DEBUG_ERRORS) {
        printf(" Node %u: Store new assert winner as AssertWinner\n",
               node->nodeId);
    }
    downStreamListPtr->assertWinner = assertSender;
    if (DEBUG) {
        printf(" assert_winner is %u \n", downStreamListPtr->assertWinner);
    }

    /* assert winner metric as AssertWinnerMetric(*, G, I) */
    downStreamListPtr->assertWinnerMetric.RPTbit =
         PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref);
    downStreamListPtr->assertWinnerMetric.metricPreference =
         PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref);
    downStreamListPtr->assertWinnerMetric.metric = assertPkt->metric;
    downStreamListPtr->assertWinnerMetric.ipAddress = assertSender;

    delay = (clocktype)
            (pimDataSm->routingPimSmAssertTimeout);

    if (downStreamListPtr->assertTime == 0)
    {
        /* Set timer to assert_time */
        timerInfo.srcAddr = getNodeAddressFromCharArray (
                            assertPkt->sourceAddr.unicastAddr);
        timerInfo.grpAddr = assertPkt->groupAddr.groupAddr;
        timerInfo.intfIndex = interfaceId;
        timerInfo.treeState = treeState;
        RoutingPimSetTimer(node,
                       interfaceId,
                       MSG_ROUTING_PimSmAssertTimerTimeout,
                       (void*) &timerInfo, delay);
    }
    downStreamListPtr->assertTime = node->getNodeTime() + delay;
}

/*
*  FUNCTION    : RoutingPimSmPerformActionA3()
*  PURPOSE     : Routing PimSm Perform Action:
*                Send Assert(*, G)
*                Set timer to (Assert_Time - Assert_Override_Interval)
*  RETURN VALUE: NONE
*/
void
RoutingPimSmPerformActionA3(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId,
                            RoutingPimSmMulticastTreeState treeState)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmAssertTimerInfo   timerInfo;
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;
    clocktype delay;

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, treeState);

    if (treeInfoBaseRowPtr != NULL)
    {
        /* check if the interface present in the downstream list */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                        treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr == NULL)
    {
        return;
    }
    if (treeState == ROUTING_PIMSM_G)
    {
        /* Send (*.G) Assert */
        RoutingPimSmSendAssertPacket(node, srcAddr, grpAddr,
                                     interfaceId, ROUTING_PIMSM_G_ASSERT);
    }
    else if (treeState == ROUTING_PIMSM_SG)
    {
        /* Send (*, S.G) Assert */
        RoutingPimSmSendAssertPacket(node, srcAddr, grpAddr,
                                     interfaceId,
                                     ROUTING_PIMSM_SG_ASSERT);
    }


    delay = (clocktype)
            (pimDataSm->routingPimSmAssertTimeout
              - ROUTING_PIMSM_ASSERT_OVERRIDE_INTERVAL);

    if (downStreamListPtr->assertTime == 0)
    {
        /*  Set timer to (Assert_Time - Assert_Override_Interval) */
        timerInfo.srcAddr = srcAddr;
        timerInfo.grpAddr = grpAddr;
        timerInfo.intfIndex = interfaceId;
        timerInfo.treeState = treeState;
        RoutingPimSetTimer(node,
                       interfaceId,
                       MSG_ROUTING_PimSmAssertTimerTimeout,
                       (void*) &timerInfo, delay);
    }

    downStreamListPtr->assertTime = node->getNodeTime() + delay;

}

/*
*  FUNCTION    : RoutingPimSmPerformActionA4()
*  PURPOSE     : Routing PimSm Perform Action:
*               Send AssertCancel(*, G)
*               Delete assert info (AssertWinner(*, G, I) and
*               AssertWinnerMetric(*, G, I) assume default values).
*  RETURN VALUE: NONE
*/
void
RoutingPimSmPerformActionA4(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId,
                            RoutingPimSmMulticastTreeState treeState)
{
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, treeState);

    if (treeInfoBaseRowPtr != NULL)
    {
        /* check if the interface present in the downstream list */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                        treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr == NULL)
    {
        return;
    }
    if (treeState == ROUTING_PIMSM_G)
    {
        NodeAddress RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);
        if (DEBUG)
        {
            printf("Node %u: Send (*.G) Assert Cancel through intf %d\n",
               node->nodeId, interfaceId);
        }
        /* An AssertCancel(*, G) is an infinite metric assert with the
        RPT bit set, and typically will name RP(G) as the source as it
        cannot name an appropriate S.*/

        RoutingPimSmSendAssertPacket(node, RPAddr, grpAddr,
                                     interfaceId,
                                     ROUTING_PIMSM_G_ASSERT_CANCEL);
    }
    else if (treeState == ROUTING_PIMSM_SG)
    {
        if (DEBUG)
        {
            printf("Node %u: Send (*.S, G) Assert Cancel through intf %d\n",
               node->nodeId, interfaceId);
        }
        /*An AssertCancel(S, G) is an infinite metric assert with the
        RPT bit set that names S as the source.*/

        RoutingPimSmSendAssertPacket(node, srcAddr, grpAddr,
                                     interfaceId,
                                     ROUTING_PIMSM_SG_ASSERT_CANCEL);
    }
    /* Delete assert info (AssertWinner() */
    if (DEBUG_ERRORS) {
        printf(" Node %u: Set default AssertWinner\n", node->nodeId);
    }
    downStreamListPtr->assertWinner = (unsigned)NETWORK_UNREACHABLE;

    /* AssertWinnerMetric() assume default values). */
    downStreamListPtr->assertWinnerMetric.RPTbit = 1;
    downStreamListPtr->assertWinnerMetric.metricPreference =
        ROUTING_ADMIN_DISTANCE_DEFAULT;
    downStreamListPtr->assertWinnerMetric.metric = 0xFFFFFFFF;
    downStreamListPtr->assertWinnerMetric.ipAddress =
        (unsigned)NETWORK_UNREACHABLE;
}

/*
*  FUNCTION    : RoutingPimSmPerformActionA5()
*  PURPOSE     : Routing PimSm Perform Action:
*                Delete assert info (AssertWinner(*, G, I) and
*                AssertWinnerMetric(*, G, I) assume default values).
*  RETURN VALUE: NONE
*/
void
RoutingPimSmPerformActionA5(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId,
                            RoutingPimSmMulticastTreeState treeState)
{
    //RoutingPimSmAssertTimerInfo   timerInfo;
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;
    int outInterface = 0;
    NodeAddress nextHop = 0;

    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(
                                node,
                                grpAddr,
                                srcAddr,
                                treeState);

    if (treeInfoBaseRowPtr != NULL)
    {
        /* check if the interface present in the downstream list */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                        treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr == NULL)
    {
        if (treeInfoBaseRowPtr
            &&
            interfaceId == treeInfoBaseRowPtr->upInterface)
        {
            treeInfoBaseRowPtr->metric.ipAddress =
                                        (unsigned)NETWORK_UNREACHABLE;
            treeInfoBaseRowPtr->metric.metric = 0xFFFFFFFF;
            treeInfoBaseRowPtr->metric.metricPreference =
                    ROUTING_ADMIN_DISTANCE_DEFAULT;
            treeInfoBaseRowPtr->metric.RPTbit = 1;
            treeInfoBaseRowPtr->upAssertWinner =
                                        (unsigned) NETWORK_UNREACHABLE;

            /*Store the original RPF*/
            RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                node,
                treeInfoBaseRowPtr->srcAddr,
                &outInterface,
                &nextHop);

            treeInfoBaseRowPtr->nextHopForSrc = nextHop;
            treeInfoBaseRowPtr->upstream = nextHop;

            if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G)
            {
                treeInfoBaseRowPtr->nextHopForRP = nextHop;
            }
        }

        return;
    }
    /* Delete assert info (AssertWinner(*) */
    if (DEBUG_ERRORS) {
        printf(" Node %u: Set default AssertWinner\n", node->nodeId);
    }
    downStreamListPtr->assertWinner = (unsigned)NETWORK_UNREACHABLE;

    /* AssertWinnerMetric(*) assume default values). */
    downStreamListPtr->assertWinnerMetric.RPTbit = 1;
    downStreamListPtr->assertWinnerMetric.metricPreference =
        ROUTING_ADMIN_DISTANCE_DEFAULT;
    downStreamListPtr->assertWinnerMetric.metric = 0xFFFFFFFF;
    downStreamListPtr->assertWinnerMetric.ipAddress =
        (unsigned)NETWORK_UNREACHABLE;
}

/*
*  FUNCTION    : RoutingPimSmPerformActionA6()
*  PURPOSE     : Routing PimSm Perform Action:
*                Store new assert winner as AssertWinner(*, S, G, I)
*                and assert winner metric as AssertWinnerMetric(*, S, G, I).
*                Set timer to assert_time
*                if I is the RPF_Interface(S) set SPTbit(S, G) to true
*  RETURN VALUE: NONE
*/
void
RoutingPimSmPerformActionA6(Node* node, NodeAddress assertSender,
                            RoutingPimSmAssertPacket* assertPkt,
                            RoutingPimSmMulticastTreeState treeState,
                            int interfaceId)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmAssertTimerInfo   timerInfo;
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;
    clocktype delay;

    int srcIntf = 0;
    NodeAddress nextHopForSrc = 0;
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(
            node,
            getNodeAddressFromCharArray(assertPkt->sourceAddr.unicastAddr),
            &srcIntf,
            &nextHopForSrc);

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         assertPkt->groupAddr.groupAddr,
                         getNodeAddressFromCharArray (
                         assertPkt->sourceAddr.unicastAddr), treeState);

    if (treeInfoBaseRowPtr != NULL)
    {
        /* check if the interface present in the downstream list */
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                        treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr == NULL)
    {
        return;
    }

    /*  Store new assert winner as AssertWinner */
    if (DEBUG) {
        printf(" Node %u: change current AssertWinner to assertSender %u\n",
               node->nodeId, assertSender);
    }
    downStreamListPtr->assertWinner = assertSender;

    /* assert winner metric as AssertWinnerMetric(*, S, G, I) */
    downStreamListPtr->assertWinnerMetric.RPTbit =
        PimSmAssertPacketGetRPTBit(assertPkt->metricBitPref);
    downStreamListPtr->assertWinnerMetric.metricPreference =
            PimSmAssertPacketGetMetricPref(assertPkt->metricBitPref);
    downStreamListPtr->assertWinnerMetric.metric = assertPkt->metric;
    downStreamListPtr->assertWinnerMetric.ipAddress = assertSender;

    delay = (clocktype)
            (pimDataSm->routingPimSmAssertTimeout);

    if (downStreamListPtr->assertTime == 0)
    {
        /* Set timer to assert_time */
        timerInfo.srcAddr = getNodeAddressFromCharArray (
                            assertPkt->sourceAddr.unicastAddr);
        timerInfo.grpAddr = assertPkt->groupAddr.groupAddr;
        timerInfo.intfIndex = interfaceId;
        timerInfo.treeState = treeState;
        RoutingPimSetTimer(node,
                       interfaceId,
                       MSG_ROUTING_PimSmAssertTimerTimeout,
                       (void*) &timerInfo, delay);
    }

    downStreamListPtr->assertTime = node->getNodeTime() + delay;
    /* if I is the RPF_Interface(S) set SPTbit(S, G) to true */
    if ((interfaceId == srcIntf) &&
        (treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join))
    {
        treeInfoBaseRowPtr->SPTbit = TRUE;
    }
}

/*
*  FUNCTION    : RoutingPimSmAssertTrackingDesiredG()
*  PURPOSE     : RoutingPimSm checks if AssertTrackingDesired is true
*  RETURN VALUE: TRUE if conditionsatisfies
*/
BOOL
RoutingPimSmAssertTrackingDesiredG(Node* node, NodeAddress grpAddr,
                                   NodeAddress srcAddr, int interfaceId)
{
    int RPIntf = 0;
    NodeAddress nextHopForRP = 0;
    NodeAddress RPAddr = 0;
    RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                node,
                RPAddr,
                &RPIntf,
                &nextHopForRP);
    if (nextHopForRP == 0)
    {
        nextHopForRP = RPAddr ;
    }
    else if (nextHopForRP == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHopForRP,
                                           &RPIntf);
    }

    if (RPIntf == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,RPAddr);
        }
        return FALSE;
    }
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (RPAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            RPIntf = CPU_INTERFACE;
            break;
        }
    }
    if ((RoutingPimSmCouldAssertG(node, grpAddr, srcAddr, interfaceId))
            || ((RPIntf == interfaceId)
                && (RoutingPimSmRPTJoinDesiredG(node, grpAddr,
                                                srcAddr, interfaceId))))
    {
        return TRUE;
    }
    return FALSE;
}

//done
/*
*  FUNCTION    : RoutingPimSmAssertTrackingDesiredSG()
*  PURPOSE     : RoutingPimSm checks if AssertTrackingDesired is true
*              (I in ((joins(*,* , RP(G)) (+) joins(*, G) (-) prunes(SGrpt))
*                        (+) (pim_include(*, G) (-) pim_exclude(S, G))
*                       (-) lost_assert(*, G)
*                        (+) joins(S, G) (+) pim_include(S, G)))
*                OR (RPF_interface(S) == I AND JoinDesired(S, G) == TRUE)
*                OR (RPF_interface(RP) == I AND JoinDesired(*, G) == TRUE
*                    AND SPTbit(S, G) == FALSE)
*  RETURN VALUE: TRUE if conditionsatisfies
*/
BOOL
RoutingPimSmAssertTrackingDesiredSG(Node* node, NodeAddress grpAddr,
                                    NodeAddress srcAddr, int interfaceId)
{
    int srcIntf;
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr;
    int i = interfaceId;
    NodeAddress RPAddr = 0;
    RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);

    int RPIntf = 0;
    NodeAddress nextHopForRP = 0, nextHopForSrc = 0;

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(
            node,
            srcAddr,
            &srcIntf,
            &nextHopForSrc);
    if (nextHopForSrc == 0)
    {
        nextHopForSrc = srcAddr ;
    }
    else if (nextHopForSrc == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           srcAddr,
                                           &nextHopForSrc,
                                           &srcIntf);
    }

    if (srcIntf == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,srcAddr);
        }
        return FALSE;
    }

    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (srcAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            srcIntf = CPU_INTERFACE;
            break;
        }
    }
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(
            node,
            RPAddr,
            &RPIntf,
            &nextHopForRP);
    if (nextHopForRP == 0)
    {
        nextHopForRP = RPAddr ;
    }
    else if (nextHopForRP == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHopForRP,
                                           &RPIntf);
    }

    if (RPIntf == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,RPAddr);
        }
        return FALSE;
    }
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (RPAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            RPIntf = CPU_INTERFACE;
            break;
        }
    }
    treeInfoBaseRowPtr =
                     RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, ROUTING_PIMSM_SG);
    if ((RoutingPimSmJoinRP(node, grpAddr, srcAddr, i)
            || RoutingPimSmJoinG(node, grpAddr, srcAddr, i)
            && (!RoutingPimSmPruneSGrpt(node, grpAddr, srcAddr, i))
            || RoutingPimSmIncludeG(node, grpAddr, srcAddr, i)
            && (!RoutingPimSmExcludeSG(node, grpAddr, srcAddr, i))
            && (!RoutingPimSmLostAssertG(node, grpAddr, srcAddr, i))
            || RoutingPimSmJoinSG(node, grpAddr, srcAddr, i)
            || RoutingPimSmIncludeSG(node, grpAddr, srcAddr, i))
            || ((srcIntf == i) && (RoutingPimSmJoinDesiredSG(node, grpAddr,
                                   srcAddr)))
            || ((RPIntf == i) && (RoutingPimSmJoinDesiredG(node, grpAddr,
                                  srcAddr, i))
                && (treeInfoBaseRowPtr &&
                   treeInfoBaseRowPtr->SPTbit == FALSE)))
    {
        return TRUE;
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmInheritedOutListSGrpt
*  PURPOSE     : RoutingPimSmInheritedOutListSGrpt
*               inherited_olist(S, G, rpt) =
*                (joins(*,* , RP(G)) (+) joins(*, G) (-) prunes(S, G, rpt))
*                (+) (pim_include(*, G) (-) pim_exclude(S, G))
*                (-) (lost_assert(*, G) (+) lost_assert(S, G, rpt))
*  RETURN VALUE: The size of InheriredOutListSGrpt
*/
unsigned int
RoutingPimSmInheritedOutListSGrpt(Node* node, NodeAddress grpAddr,
                             NodeAddress srcAddr,
                             RoutingPimSmTreeInfoBaseRow *treeInfoBaseRowPtr)
{
    int i;
    PimData* pim = (PimData*)NetworkIpGetMulticastRoutingProtocol(node,
                                                     MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    bool ignoreProcessing = FALSE;
    NetworkDataIp* ip = (NetworkDataIp*)node->networkData.networkVar;
    if (pim)
    {
        if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
        {
            pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
        }
        else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
        {
            pimDataSm = (PimDataSm*) pim->pimModePtr;
        }
        if (pimDataSm && ip->isSSMEnable)
        {
            if (Igmpv3IsSSMAddress(node, grpAddr))
            {
                ignoreProcessing = TRUE;
            }
        }
    }
    RoutingPimSmDeleteList(node,
                           treeInfoBaseRowPtr->inheritedOutIntfSGrpt, FALSE);
    if (ignoreProcessing == FALSE)
    {
        /* check for each interface */
        for (i = 0; i < node->numberInterfaces; i++)
        {
            if ((RoutingPimSmJoinRP(node, grpAddr, srcAddr, i) ||
                    (RoutingPimSmJoinG(node, grpAddr, srcAddr, i) &&
                    (!RoutingPimSmPruneSGrpt(node, grpAddr, srcAddr, i)))) ||
                    (RoutingPimSmIncludeG(node, grpAddr, srcAddr, i) &&
                     (!RoutingPimSmExcludeSG(node, grpAddr, srcAddr, i))) &&
                    (!(RoutingPimSmLostAssertG(node, grpAddr, srcAddr, i) ||
                       RoutingPimSmLostAssertSGrpt(node, grpAddr,
                                                   srcAddr, i))))
            {

                unsigned int* outIntfListPtr =
                    (unsigned int*)MEM_malloc(sizeof(unsigned int));

                * outIntfListPtr = (unsigned int)i;
                ListInsert(
                    node,
                    treeInfoBaseRowPtr->inheritedOutIntfSGrpt,
                    0,
                    (void*) outIntfListPtr);

            }
        }   // end of for;
    }
    if (DEBUG) {
        printf("inheritedOutIntfSGrpt->size %d\n",
               treeInfoBaseRowPtr->inheritedOutIntfSGrpt->size);
    }

    return (unsigned int)(treeInfoBaseRowPtr->inheritedOutIntfSGrpt->size);
}

/*
*  FUNCTION    : RoutingPimSmInheritedOutListSG
*  PURPOSE     : RoutingPimSmInheritedOutListSG
*               inherited_olist(S, G) =
*                   inherited_olist(S, G, rpt) (+) immediate_olist(S, G)
*  RETURN VALUE: The size of InheriredOutListSG;
*/
unsigned int
RoutingPimSmInheritedOutListSG(Node* node, NodeAddress grpAddr,
                             NodeAddress srcAddr,
                             RoutingPimSmTreeInfoBaseRow *treeInfoBaseRowPtr)
{
    ListItem* tempListItem;
    unsigned int i;

    RoutingPimSmDeleteList(node,
                           treeInfoBaseRowPtr->inheritedOutIntfSG, FALSE);

    if (RoutingPimSmInheritedOutListSGrpt(node, grpAddr,
                                          srcAddr, treeInfoBaseRowPtr) != 0)
    {
        tempListItem = treeInfoBaseRowPtr->inheritedOutIntfSGrpt->first;
        while (tempListItem)
        {
            unsigned int* inheritedSGrpt = (unsigned int*)tempListItem->data;
            i =* inheritedSGrpt;

            if (IsInterfaceInThisPimSmList(
                        treeInfoBaseRowPtr->inheritedOutIntfSG, i) == FALSE)
            {
                unsigned int* outIntfListPtr =
                    (unsigned int*)MEM_malloc(sizeof(unsigned int));

                * outIntfListPtr = i;
                ListInsert(node, treeInfoBaseRowPtr->inheritedOutIntfSG, 0,
                           (void*) outIntfListPtr);
            }
            tempListItem = tempListItem->next;
        }
    }

    if (RoutingPimSmImmediateOutListSG(node, grpAddr, srcAddr,
                                       treeInfoBaseRowPtr) != 0)
    {
        tempListItem = treeInfoBaseRowPtr->immediateOutIntfSG->first;

        while (tempListItem)
        {
            unsigned int* inheritedSG = (unsigned int*)tempListItem->data;

            i =* inheritedSG;

            if (IsInterfaceInThisPimSmList(
                                      treeInfoBaseRowPtr->inheritedOutIntfSG,
                                      i) == FALSE)
            {
                unsigned int* outIntfListPtr =
                    (unsigned int*)MEM_malloc(sizeof(unsigned int));
                * outIntfListPtr = i;

                ListInsert(node, treeInfoBaseRowPtr->inheritedOutIntfSG, 0,
                           (void*) outIntfListPtr);
            }
            tempListItem = tempListItem->next;
        }
    }

    if (DEBUG) {
        printf("inheritedOutIntfSG->size %d\n",
               treeInfoBaseRowPtr->inheritedOutIntfSG->size);
    }
    return treeInfoBaseRowPtr->inheritedOutIntfSG->size;
}


/*
*  FUNCTION    : RoutingPimSmImmediateOutlistG
*  PURPOSE     : RoutingPimSmImmediate_olistG
*               immediate_olist (*, G)  =
*                   joins(*, G) (+) pim_include(*, G) (-) lost_assert(*, G)
*  RETURN VALUE: The Size of ImmediateOutlistG;
*/
unsigned int
RoutingPimSmImmediateOutListG(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr,
                            RoutingPimSmTreeInfoBaseRow * treeInfoBaseRowPtr)
{
    int i;

    RoutingPimSmDeleteList(node,
                           treeInfoBaseRowPtr->immediateOutIntfG, FALSE);

    /* check for each interface */
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (RoutingPimSmJoinG(node, grpAddr, srcAddr, i)
                || RoutingPimSmIncludeG(node, grpAddr,
                                        srcAddr, i)
                && (!RoutingPimSmLostAssertG(node, grpAddr, srcAddr, i)))
        {
            unsigned int* outIntfListPtr =
                (unsigned int*)MEM_malloc(sizeof(unsigned int));
            * outIntfListPtr = i;
            ListInsert(node, treeInfoBaseRowPtr->immediateOutIntfG, 0,
                       (void*) outIntfListPtr);
        }
    }   // end of for;
    if (DEBUG) {
        printf("immediateOutIntfG->size %d\n",
               treeInfoBaseRowPtr->immediateOutIntfG->size);
    }
    return treeInfoBaseRowPtr->immediateOutIntfG->size;
}

/*
*  FUNCTION    : RoutingPimSmImmediateOutlistSG
*  PURPOSE     : RoutingPimSmImmediate_olistG
*               immediate_olist (*, S, G)  =
*                   joins(*, S, G) (+) pim_include(*, S, G)
*                       (-) lost_assert(*, S, G)
*  RETURN VALUE: The size of ImmediateOutlistSG;
*/
unsigned int
RoutingPimSmImmediateOutListSG(Node* node, NodeAddress grpAddr,
                NodeAddress srcAddr,
                RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr)
{
    int i;
    RoutingPimSmDeleteList(node,
                           treeInfoBaseRowPtr->immediateOutIntfSG, FALSE);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (RoutingPimSmJoinSG(node, grpAddr, srcAddr, i)
                || RoutingPimSmIncludeSG(node, grpAddr,
                                         srcAddr, i)
                && (!RoutingPimSmLostAssertSG(node, grpAddr, srcAddr, i)))
        {
            unsigned int* outIntfListPtr =
                (unsigned int*)MEM_malloc(sizeof(unsigned int));
            * outIntfListPtr = i;
            ListInsert(node, treeInfoBaseRowPtr->immediateOutIntfSG, 0,
                       (void*) outIntfListPtr);

        }
    }   // end of for;

    if (DEBUG_ERRORS) {
        printf("immediateOutIntfSG->size %d\n",
               treeInfoBaseRowPtr->immediateOutIntfSG->size);
    }

    return treeInfoBaseRowPtr->immediateOutIntfSG->size;
}

/*
*  FUNCTION    : IsInterfaceInThisPimSmList()
*  PURPOSE     : IsInterfaceInThisPimSmList
*  RETURN VALUE: TRUE if it ispresent in the list;
*/

BOOL
IsInterfaceInThisPimSmList(LinkedList* list, int interfaceId)
{
    ListItem* tempListItem;
    unsigned int size;

    size = list->size;

    if (list->size == 0)
    {
        return FALSE;
    }
    tempListItem = list->first;

    while (tempListItem)
    {
        int* outIntf = (int*)tempListItem->data;

        BOOL sameInt = TRUE;
//#ifdef ADDON_BOEINGFCS
//        sameInt =
//            MulticastCesRpimSameInterface(node,
//                interfaceId,
//                *outIntf);
//#else
        if (*outIntf != interfaceId)
        {
            sameInt = FALSE;
        }
//#endif

        if (sameInt)
        {
            if (DEBUG) {
                printf(" the interfaceId %d is already present\n",* outIntf);
            }
            return TRUE;
        }
        tempListItem = tempListItem->next;
        size--;
    }//End while

    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmIncludeG()
*  PURPOSE     : RoutingPimSmImmediateOutlistSG
*               { all interfaces I such that:
*                ((I_am_DR(I) AND lost_assert(*, G, I) == FALSE)
*                   OR AssertWinner(*, G, I) == me)
*                       AND  local_receiver_include(*, G, I) }
*  RETURN VALUE: TRUE if condition satisfies;
*/
BOOL
RoutingPimSmIncludeG(Node* node, NodeAddress grpAddr,
                     NodeAddress srcAddr, int interfaceId)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;
    BOOL localMember = FALSE;
    BOOL isDR = FALSE;
    NetworkDataIp *ipLayer = (NetworkDataIp *) node->networkData.networkVar;
    IgmpInterfaceInfoType *thisInterface = NULL;
    if (ipLayer->isIgmpEnable && interfaceId >= 0)
    {
        IgmpData* igmp = IgmpGetDataPtr(node);
        ERROR_Assert(igmp, " IGMP interface not found");
        thisInterface = igmp->igmpInterfaceInfo[interfaceId];
        ERROR_Assert(thisInterface, " IGMP interface not found");
        if (thisInterface->igmpVersion == IGMP_V2)
        {
            IgmpSearchLocalGroupDatabase(node, grpAddr,
                                 interfaceId, &localMember);
        }
        else
        {
            Igmpv3SearchLocalGroupDatabase(node, grpAddr,
                                    interfaceId, &localMember,srcAddr);
        }
    }


    if (DEBUG_ERRORS) {
        printf("Node %u: intf %d return local member %d \n",
               node->nodeId, interfaceId, localMember);
    }

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, ROUTING_PIMSM_G);

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr != NULL)
    {
        if (((pim->interface[interfaceId].smInterface->drInfo.ipAddr ==
                pim->interface[interfaceId].ipAddress)
                && (RoutingPimSmLostAssertG(node, grpAddr,
                                            srcAddr,
                                            interfaceId) == FALSE))
                || (downStreamListPtr->assertWinner ==
                    pim->interface[interfaceId].ipAddress))
        {
            isDR = TRUE;

        }
    }
    if ((isDR) && (localMember))
    {
        if (DEBUG_ERRORS) {
            printf(" Node %u include G:return TRUE \n", node->nodeId);
        }
        return TRUE;
    }

    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmIncludeSG()
*  PURPOSE     : RoutingPimSmImmediateOutlistSG
*               { all interfaces I such that:
*                ((I_am_DR(I) AND lost_assert(*, S, G, I) == FALSE)
*                   OR AssertWinner(*, S, G, I) == me)
*                       AND  local_receiver_include(*, S, G, I) }
*  RETURN VALUE: TRUE if condition satisfies;
*/
BOOL
RoutingPimSmIncludeSG(Node* node, NodeAddress grpAddr,
                      NodeAddress srcAddr, int interfaceId)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr;
    RoutingPimSmDownstream       * downStreamListPtr;
    BOOL localMember = FALSE;
    BOOL isDR = FALSE;

    NetworkDataIp *ipLayer = (NetworkDataIp *) node->networkData.networkVar;
    IgmpInterfaceInfoType *thisInterface = NULL;
    if ((ipLayer->isIgmpEnable) && (interfaceId >= 0))
    {
        IgmpData* igmp = IgmpGetDataPtr(node);
        ERROR_Assert(igmp, " IGMP interface not found");
        thisInterface = igmp->igmpInterfaceInfo[interfaceId];
        ERROR_Assert(thisInterface, " IGMP interface not found");
        if (thisInterface->igmpVersion == IGMP_V2)
        {
            IgmpSearchLocalGroupDatabase(node, grpAddr,
                                 interfaceId, &localMember);
        }
        else
        {
            Igmpv3SearchLocalGroupDatabase(node, grpAddr,
                                    interfaceId, &localMember, srcAddr);
        }
    }

    if (DEBUG_ERRORS) {
        printf("Node %u: intf %d return local memberSG %d \n",
               node->nodeId, interfaceId, localMember);
    }

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, ROUTING_PIMSM_SG);

    if (treeInfoBaseRowPtr != NULL)
    {
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);

        if (downStreamListPtr != NULL)
        {
            if (((pim->interface[interfaceId].smInterface->drInfo.ipAddr ==
                    pim->interface[interfaceId].ipAddress)
                    && (RoutingPimSmLostAssertSG(node, grpAddr,
                                                 srcAddr, interfaceId)
                                                 == FALSE))
                    || (downStreamListPtr->assertWinner ==
                        pim->interface[interfaceId].ipAddress))
            {
                isDR = TRUE;

            }
        }
    }
    if ((isDR) && (localMember))
    {

        return TRUE;
    }

    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmExcludeSG()
*  PURPOSE     : RoutingPimSmExcludeSG
*               all interfaces I such that:
*                   all interfaces I such that:
*               ( (I_am_DR( I ) AND lost_assert(*,G,I) == FALSE )
*               OR AssertWinner(*,G,I) == me )
*               AND local_receiver_exclude(S,G,I) }
*  RETURN VALUE: TRUE if condition satisfies;
*/
BOOL
RoutingPimSmExcludeSG(Node* node, NodeAddress grpAddr,
                      NodeAddress srcAddr, int interfaceId)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;
    BOOL localMember = FALSE;
    BOOL localMemberExclude = FALSE;
    BOOL isDR = FALSE;

    NetworkDataIp *ipLayer = (NetworkDataIp *) node->networkData.networkVar;
    IgmpInterfaceInfoType *thisInterface = NULL;
    if (ipLayer->isIgmpEnable && interfaceId >= 0)
    {
        IgmpData* igmp = IgmpGetDataPtr(node);
        ERROR_Assert(igmp, " IGMP interface not found");
        thisInterface = igmp->igmpInterfaceInfo[interfaceId];
        ERROR_Assert(thisInterface, " IGMP interface not found");
        if (thisInterface->igmpVersion == IGMP_V2)
        {
            IgmpSearchLocalGroupDatabase(node, grpAddr,
                                 interfaceId, &localMember);
        }
        else
        {
            Igmpv3SearchLocalGroupDatabase(node, grpAddr,
                                    interfaceId, &localMember, srcAddr);
        }
    }

    if (DEBUG_ERRORS) {
        printf("Node %u: intf %d return local member %d \n",
               node->nodeId, interfaceId, localMember);
    }
    if (localMember)
    {
        /* local_receiver_exclude(S, G, I) is true if
        local_receiver_include(*, G, I) is true but none of the local members
        desire to receive traffic from S.

        In IGMP, there is no mechanism to trigger whether a host wants to
        exclude receiving data from a particular S for a grp G,
        hence there is no trigger to set localMemberExclude to TRUE.
        Hence we always keep it as FALSE */
        localMemberExclude = FALSE;
    }

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, ROUTING_PIMSM_G);

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
    }
    if (downStreamListPtr != NULL)
    {
        if (((pim->interface[interfaceId].smInterface->drInfo.ipAddr ==
                pim->interface[interfaceId].ipAddress)
                && (RoutingPimSmLostAssertG(node, grpAddr,
                                             srcAddr, interfaceId) == FALSE))
                || (downStreamListPtr->assertWinner ==
                    pim->interface[interfaceId].ipAddress))
        {
            isDR = TRUE;
        }
    }

    if ((isDR) && (localMemberExclude))
    {
        return TRUE;
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmLostAssertSGrpt()
*  PURPOSE     : RoutingPimSmLostAssertG
*               { all interfaces I such that
*                   lost_assert(*, S, G, rpt, I)}
*  RETURN VALUE: TRUE if conditionsatisfies
*/

BOOL
RoutingPimSmLostAssertSGrpt(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr,
                            int interfaceId)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;
    NodeAddress RPAddr;
    int RPIntf;
    NodeAddress nextHopForRP = 0;
    int srcIntf;
    NodeAddress nextHopForSrc = 0;

    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, ROUTING_PIMSM_SG);

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
    }
    else
    {
        return FALSE;
    }

    RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                    RPAddr,
                                                    &RPIntf,
                                                    &nextHopForRP);
    if (nextHopForRP == 0)
    {
        nextHopForRP = RPAddr ;
    }
    else if (nextHopForRP == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHopForRP,
                                           &RPIntf);
    }

    if (RPIntf == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,RPAddr);
        }
        return FALSE;
    }
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (RPAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            RPIntf = CPU_INTERFACE;
            break;
        }
    }

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        srcAddr,
                                                        &srcIntf,
                                                        &nextHopForSrc);
    if (nextHopForSrc == 0)
    {
        nextHopForSrc = srcAddr ;
    }
    else if (nextHopForSrc == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           srcAddr,
                                           &nextHopForSrc,
                                           &srcIntf);
    }

    if (srcIntf == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,srcAddr);
        }
        return FALSE;
    }
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (srcAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            srcIntf = CPU_INTERFACE;
            break;
        }
    }
    if ((interfaceId == RPIntf)
            || ((interfaceId == srcIntf) && (treeInfoBaseRowPtr->SPTbit)))
    {
        return FALSE;
    }
    else
    {
        if (downStreamListPtr != NULL)
        {
            if ((downStreamListPtr->assertWinner !=
                    (unsigned)NETWORK_UNREACHABLE)
                    && (downStreamListPtr->assertWinner !=
                        pim->interface[interfaceId].ipAddress))
            {
                if (DEBUG) {
                    printf(" Node %u lostAssertSGrpt:return TRUE \n",
                           node->nodeId);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}


/*
*  FUNCTION    : RoutingPimSmLostAssertG()
*  PURPOSE     : RoutingPimSmLostAssertG
*               { all interfaces I such that
*                   lost_assert(*, G, I)}
*  RETURN VALUE: TRUE if conditionsatisfies
*/

BOOL
RoutingPimSmLostAssertG(Node* node, NodeAddress grpAddr,
                        NodeAddress srcAddr, int interfaceId)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;
    /* it checks to see if group G in its multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, ROUTING_PIMSM_G);

    /* check if the interface present in the downstream list */
    if (treeInfoBaseRowPtr != NULL)
    {
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, interfaceId);
    }
    else
    {
        return FALSE;
    }

    NodeAddress RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);
    int RPIntf = 0;
    NodeAddress nextHopForRP = 0;
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        RPAddr,
                                                        &RPIntf,
                                                        &nextHopForRP);
    if (nextHopForRP == 0)
    {
        nextHopForRP = RPAddr ;
    }
    else if (nextHopForRP == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHopForRP,
                                           &RPIntf);
    }

    if (RPIntf == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,RPAddr);
        }
        return FALSE;
    }
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (RPAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            RPIntf = CPU_INTERFACE;
            break;
        }
    }
    if (interfaceId == RPIntf)
    {
        return FALSE;
    }
    else
    {
        if (downStreamListPtr != NULL)
        {
            if (downStreamListPtr->assertWinner !=
                (unsigned)NETWORK_UNREACHABLE
                && downStreamListPtr->assertWinner !=
                    pim->interface[interfaceId].ipAddress)
            {
                if (DEBUG_ERRORS) {
                    printf("Node %u lostAssertG:return TRUE\n",node->nodeId);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmLostAssertSG()
*  PURPOSE     : RoutingPimSmLostAssertG
*               { all interfaces I such that
*                   lost_assert(*, S, G, I)}
*  RETURN VALUE: TRUE if conditionsatisfies
*/
BOOL
RoutingPimSmLostAssertSG(Node* node, NodeAddress grpAddr,
                         NodeAddress srcAddr, int interfaceId)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    RoutingPimSmDownstream       * downStreamListPtr = NULL;

    if (DEBUG_ERRORS) {
        printf("Node %u: intf %dchecking for lostAssertSG \n",
               node->nodeId, interfaceId);
    }
    /* if (RPF_interface(S) == I ) return FALSE */
    int srcIntf = 0;
    NodeAddress nextHopForSrc = 0;
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        srcAddr,
                                                        &srcIntf,
                                                        &nextHopForSrc);
    if (nextHopForSrc == 0)
    {
        nextHopForSrc = srcAddr ;
    }
    else if (nextHopForSrc == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           srcAddr,
                                           &nextHopForSrc,
                                           &srcIntf);
    }

    if (srcIntf == NETWORK_UNREACHABLE)
    {
        if (DEBUG_FORWARD_PACKET)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,srcAddr);
        }
        return FALSE;
    }
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (srcAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            srcIntf = CPU_INTERFACE;
            break;
        }
    }
    if (interfaceId == srcIntf)
    {
        return FALSE;
    }
    else
    {
        /* it checks to see if group G in its multicast treeInfo Base */
        treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         grpAddr, srcAddr, ROUTING_PIMSM_SG);

        /* check if the interface present in the downstream list */
        if (treeInfoBaseRowPtr != NULL)
        {
            downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                                treeInfoBaseRowPtr, interfaceId);
        }
        if (downStreamListPtr != NULL)
        {
            if ((downStreamListPtr->assertWinner !=
                (unsigned)NETWORK_UNREACHABLE) &&
                (downStreamListPtr->assertWinner !=
                pim->interface[interfaceId].ipAddress) &&
                (downStreamListPtr->assertWinnerMetric.metric >
                (unsigned int)(NetworkGetMetricForDestAddress(node, srcAddr)
                )))
            {
                if (DEBUG_ERRORS) {
                    printf("Node %u: intf %d return true:lostAssertSG \n",
                           node->nodeId, interfaceId);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmJoinDesiredSG()
*  PURPOSE     : RoutingPimSmJoinDesiredSG
*               bool JoinDesired(S, G)
*                {
*                   return(immediate_olist(S, G) != NULL
*                       OR (KeepaliveTimer(S, G) is running
*                            AND inherited_olist(S, G) != NULL))
*                }
*  RETURN VALUE: TRUE if conditionsatisfies
*/
BOOL
RoutingPimSmJoinDesiredSG(Node* node, NodeAddress grpAddr,
                          NodeAddress srcAddr)
{
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr;

    /* search for this (S, G) entry in multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                         (node, grpAddr, srcAddr, ROUTING_PIMSM_SG);

    if (treeInfoBaseRowPtr != NULL)
    {
        if ((RoutingPimSmImmediateOutListSG(node, grpAddr,
                                        srcAddr, treeInfoBaseRowPtr) != 0)
                || (treeInfoBaseRowPtr->keepAliveExpiryTimer
                    && (RoutingPimSmInheritedOutListSG(node, grpAddr,
                                                       srcAddr,
                                               treeInfoBaseRowPtr) != 0)))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmJoinDesiredG()
*  PURPOSE     : RoutingPimSmJoinDesiredG
*               if (immediate_olist(*, G) != NULL ||
*                   (JoinDesired(*,* , RP(G)) &&
*                   AssertWinner(*, G, RPF_interface(RP(G))) != NULL))
*                    return TRUE
*               else
*                   return FALSE
*  RETURN VALUE: TRUE if conditionsatisfies
*/
BOOL
RoutingPimSmJoinDesiredG(Node* node, NodeAddress grpAddr,
                         NodeAddress srcAddr, int interfaceId)
{
    interfaceId = interfaceId;

    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr;

    /* search for this (S, G) entry in multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                         (node, grpAddr, srcAddr, ROUTING_PIMSM_G);
    if (treeInfoBaseRowPtr == NULL)
    {
        return FALSE;
    }

    if ((RoutingPimSmImmediateOutListG(node, grpAddr,
                                       srcAddr, treeInfoBaseRowPtr) != 0)
            || ((RoutingPimSmJoinDesiredRP())
                && (treeInfoBaseRowPtr->upAssertWinner !=
                    (unsigned)NETWORK_UNREACHABLE)))
    {
        return TRUE;
    }
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmJoinDesiredRP()
*  PURPOSE     : RoutingPimSmJoinDesiredRP
*               if immediate_olist(*,* , RP) != NULL
*                   return TRUE
*               else
*                   return FALSE
*  RETURN VALUE: TRUE if condition satisfies
*/
BOOL
RoutingPimSmJoinDesiredRP()
{
    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmRPTJoinDesiredG()
*  PURPOSE     : RoutingPimSmRPTJoinDesiredG
*                   (JoinDesired(*, G) || JoinDesired(*,* , RP(G)))
*  RETURN VALUE: TRUE if condition satisfies
*/
BOOL
RoutingPimSmRPTJoinDesiredG(Node* node, NodeAddress groupAddr,
                            NodeAddress srcAddr, int interfaceId)
{
    /* JoinDesired(*, G) || JoinDesired(*,* , RP(G)))*/
    if ((RoutingPimSmJoinDesiredG(node, groupAddr, srcAddr, interfaceId)
            || RoutingPimSmJoinDesiredRP()))
    {
        return TRUE;
    }
    return FALSE;
}




/*
*  FUNCTION    : RoutingPimSmPrintCandidateRPAdvMessage()
*  PURPOSE     : Print Candidate RP Adv Message
*  RETURN VALUE: NULL
*/
void
RoutingPimSmPrintCandidateRPAdvMessage(Node* node,
                                       Message* candidateRPMsg)
{
    RoutingPimSmCandidateRPPacket* candidateRPPkt =
       (RoutingPimSmCandidateRPPacket*) MESSAGE_ReturnPacket(candidateRPMsg);

    BOOL isEmptyCRPMsg = FALSE;
    int index = 0;
    char* dataPtr = NULL;
    dataPtr = (char*)candidateRPPkt + sizeof(RoutingPimSmCandidateRPPacket);

    unsigned int numGroupInfo = 0;
    RoutingPimEncodedGroupAddr grpInfo;
    memset(&grpInfo, 0, sizeof(RoutingPimEncodedGroupAddr));

    printf("\n\n\tCANDIDATE RP ADV PACKET at Node %d",node->nodeId);
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");


    printf("\n*******Common header******\n");
    printf("\nPIM Ver + Type = 0x%x", candidateRPPkt->commonHeader.rpChType);
    printf("\nReserved       = 0x%x", candidateRPPkt->commonHeader.reserved);
    printf("\nChecksum       = 0x%x", candidateRPPkt->commonHeader.checksum);

    printf("\nPrefix Count   = %d", candidateRPPkt->prefixCount);
    printf("\nPriority       = %d", candidateRPPkt->priority);
    printf("\nHold Time      = %d", candidateRPPkt->holdtime);

    printf("\n\n*******Candidate RP Address*******\n");
    RoutingPimPrintEncodedUnicastFormat(candidateRPPkt->encodedUnicastRP);

    isEmptyCRPMsg = RoutingPimSmIsEmptyCRPMsg(candidateRPMsg);

    if (isEmptyCRPMsg)
    {
        numGroupInfo = 1;
    }
    else
    {
        numGroupInfo = candidateRPPkt->prefixCount;
    }

    while (numGroupInfo)
    {
        if (isEmptyCRPMsg)
        {
            grpInfo.groupAddr = 0xE0000000;
            grpInfo.maskLength = 4;
        }
        else
        {
            memcpy(&grpInfo,
                   dataPtr,
                   sizeof(RoutingPimEncodedGroupAddr));

            dataPtr = dataPtr + sizeof(RoutingPimEncodedGroupAddr);
        }

        printf("\n Group %d",++index);
        RoutingPimPrintEncodedGroupFormat(grpInfo);

        numGroupInfo--;
    }//end of while

    printf("\n\n");
}

/*
*  FUNCTION    : RoutingPimSmPrintBootStrapMessage()
*  PURPOSE     : Print BootStrap Message
*  RETURN VALUE: NULL
*/
void
RoutingPimSmPrintBootStrapMessage(Node* node,
                                  Message* bootStrapMsg)
{
    RoutingPimSmBootstrapPacket* bootstrapPkt =
        (RoutingPimSmBootstrapPacket*) MESSAGE_ReturnPacket(bootStrapMsg);

    RoutingPimSmEncodedGroupBSRInfo   * grpInfo = NULL;
    RoutingPimSmEncodedUnicastRPInfo  * RPInfo = NULL;
    unsigned int bsmSize = 0;
    char* dataPtr = NULL;
    unsigned int rpCount = 0;
    int grpCount = 0;

    printf("\n\n\tBOOT STRAP PACKET at Node %d",node->nodeId);
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    printf("\n*******Common header******\n");
    printf("\nPIM Ver + Type = 0x%x", bootstrapPkt->commonHeader.rpChType);
    printf("\nReserved       = 0x%x", bootstrapPkt->commonHeader.reserved);
    printf("\nChecksum       = 0x%x", bootstrapPkt->commonHeader.checksum);

    printf("\n\nFragmentTag    = %d", bootstrapPkt->fragmentTag);
    printf("\nHashMaskLength = %d", bootstrapPkt->pimsmHashMaskLength);
    printf("\nBSR Priority   = %d", bootstrapPkt->BSRPriority);

    printf("\n\n*******Encoded Unicast BSR Address******\n");
    RoutingPimPrintEncodedUnicastFormat(bootstrapPkt->encodedUnicastBSR);

    //Check for empty BSM
    if (RoutingPimSmIsEmptyBSM(bootStrapMsg) == TRUE)
    {
        return;
    }

    bsmSize = bootStrapMsg->packetSize;

    //reaching the grpInfp part of the packet
    dataPtr = (char*)bootstrapPkt + sizeof(RoutingPimSmBootstrapPacket);
    bsmSize = bsmSize - sizeof(RoutingPimSmBootstrapPacket);

    //for each group
    while (bsmSize > 0)
    {
        printf("\n\n-------------Group Info : %d-------------\n",++grpCount);

        grpInfo = (RoutingPimSmEncodedGroupBSRInfo*) (dataPtr);

        bsmSize = bsmSize - sizeof(RoutingPimSmEncodedGroupBSRInfo);
        if (bsmSize > 0)
        {
            dataPtr = dataPtr + sizeof(RoutingPimSmEncodedGroupBSRInfo);
        }

        RoutingPimPrintEncodedGroupFormat(grpInfo->encodedGrpAddr);
        printf("\n\nRP Count    = %d",grpInfo->rpCount);
        printf("\nFrag RP Count = %d",grpInfo->fragmentRPCount);
        printf("\nReserved      = %d",grpInfo->reserved);

        //for each RP present for this group range
        rpCount = grpInfo->rpCount;
        while (rpCount)
        {
            printf("\n~~~~~~~~~~~~~RP Info~~~~~~~~~~~~~\n");
            RPInfo = (RoutingPimSmEncodedUnicastRPInfo*) (dataPtr);

            bsmSize = bsmSize - sizeof(RoutingPimSmEncodedUnicastRPInfo);
            if (bsmSize > 0)
            {
                dataPtr = dataPtr + sizeof(RoutingPimSmEncodedUnicastRPInfo);
            }

            RoutingPimPrintEncodedUnicastFormat(RPInfo->encodedUnicastRP);
            printf("\n\nRP HoldTime   = %d",RPInfo->RPHoldTime);
            printf("\nRP Priority   = %d",RPInfo->RPPriority);
            printf("\nReserved      = %d",RPInfo->reserved);
            rpCount--;
        }//end of while

    }//end of while

    printf("\n\n");
}

/*
*  FUNCTION    : RoutingPimSmPrintRPGroupAccessList()
*  PURPOSE     : Print Group Access list at Candidate
*  RETURN VALUE: NULL
*/
void
RoutingPimSmPrintRPGroupAccessList(Node* node,
                                   int interfaceIndex)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    ListItem* tempListItem = NULL;

    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];

    RoutingPimSmRPAccessList* accessListInfo = NULL;

    printf("\n\tAccess List of RP : 0x%x",NetworkIpGetInterfaceAddress(
                                      node,interfaceIndex));
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    tempListItem = thisInterface->rpData.rpAccessList->first;

    while (tempListItem)
    {
        accessListInfo = (RoutingPimSmRPAccessList*)(tempListItem->data);

        printf("Group Address = 0x%x\t", accessListInfo->groupAddr);
        printf("Group Mask = 0x%x\n", accessListInfo->groupMask);

        tempListItem = tempListItem->next;
    }//End while

}

/*
*  FUNCTION    : RoutingPimSmPrintNghbrList()
*  PURPOSE     : RoutingPimSm Print NbrList for this router
*  RETURN VALUE: NULL
*/

void
RoutingPimSmPrintNghbrList(Node* node)
{
    ListItem* tempListItem;
    int i;
    char nbrAddress[MAX_STRING_LENGTH];
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    for (i=0; i< node->numberInterfaces; i++)
    {
        printf ("\nNeighborList for node 0x%x ,interface %d, "
                "has %d entry\n",
                node->nodeId,
                i,
                pim->interface[i].smInterface->neighborCount);
        tempListItem = pim->interface[i].smInterface->neighborList->first;
        printf("\n\n\tSTORED NBR LIST at Node %d, int %d\n\n",
               node->nodeId,i);
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");

        while (tempListItem)
        {
            RoutingPimNeighborListItem* neighbor =
                (RoutingPimNeighborListItem*)tempListItem->data;
            IO_ConvertIpAddressToString(neighbor->ipAddr,
                            nbrAddress);
#ifdef DEBUG
    {
        printf("NBR Address is %s\n",
               nbrAddress);
    }
#endif
            tempListItem = tempListItem->next;
        }//End while
    }
}



/*
*  FUNCTION    : RoutingPimSmPrintRPList()
*  PURPOSE     : RoutingPimSm Print RPList for this router
*  RETURN VALUE: NULL
*/

void
RoutingPimSmPrintRPList(Node* node)
{
    ListItem* tempListItem;

    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    if (DEBUG) {
        printf ("RPList for node 0x%x has %d entry\n",
                node->nodeId, pimDataSm->RPList->size);
    }

    tempListItem = pimDataSm->RPList->first;
    printf("\n\n\tSTORED RP LIST at Node %d\n\n",node->nodeId);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");

    while (tempListItem)
    {
        RoutingPimSmRPList* RPListPtr =
            (RoutingPimSmRPList*)tempListItem->data;

        printf(" RP = %x\t", RPListPtr->RPAddress);
        printf(" grpPrefix = 0x%x\t", RPListPtr->grpPrefix);
        printf(" maskLength = %u\n", RPListPtr->maskLength);

        tempListItem = tempListItem->next;
    }//End while
}


/*
*  FUNCTION    : RoutingPimSmPrintForwardingTable()
*  PURPOSE     : RoutingPimSmPrintForwardingTable
*  RETURN VALUE: NULL
*/
void
RoutingPimSmPrintForwardingTable(Node* node)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    char clockTime[100];
    char grpAddr[100];
    char srcAddr[100];
    char RPAddr[100];

    unsigned int i;
    NodeAddress RPAddress;

    RoutingPimSmForwardingTable* fwdTable = &pimDataSm->forwardingTable;
    ctoa(node->getNodeTime(), clockTime);

    printf ("\n FWD Table for node %u has %d entry at %s\n",
            node->nodeId, fwdTable->numEntries, clockTime);

    if (fwdTable->numEntries == 0)
    {
        printf("\tdoesnt get any Multicast_Pkt so No Item In Fwd_Table \n");
        return;
    }

    /* print the forwarding table  */
    printf("\t -----------------------------------------------------\n");
    printf("\t  Group        Source      RPAddr     inIntf        \n");
    printf(" \t------------------------------------------------------\n");

    for (i = 0; i < fwdTable->numEntries; i++)
    {
        //ListItem* tempListItem;
        RPAddress = RoutingPimSmFindRPForThisGroup(node,
                    fwdTable->rowPtr[i].grpAddr);

        IO_ConvertIpAddressToString(fwdTable->rowPtr[i].grpAddr, grpAddr);
        IO_ConvertIpAddressToString(fwdTable->rowPtr[i].srcAddr, srcAddr);
        IO_ConvertIpAddressToString(RPAddress, RPAddr);

        printf ("\t %10s %10s  %10s   %3d \n", grpAddr, srcAddr, RPAddr,
                fwdTable->rowPtr[i].inInterface);
    }
    printf ("\n");
}

/*
*  FUNCTION    : RoutingPimSmCheckForSGRptPeriodicMessage()
*  PURPOSE     : Router Checks if it send the periodic SGRptMessage
*  RETURN VALUE: NULL
*/

LinkedList*
RoutingPimSmCheckForSGRptPeriodicMessage(Node* node, NodeAddress grpAddress)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmTreeInformationBase* treeInfo = &pimDataSm->treeInfoBase;

    TreeInfoRowMap* rowPtrMap = treeInfo->rowPtrMap;
    TreeInfoRowMapIterator mapIterator;
    RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;

    LinkedList* periodicSGRptInfoList = NULL;
    NodeAddress* srcAddress;
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr = NULL;
    /*
    When a router is going to send a Join(*, G), for each (S, G) for which
    it has state, to decide whether to include a Prune(S, G, rpt) in the
    compound Join/Prune message
    */

    if (treeInfo->numEntries != 0)
    {
        for (mapIterator = rowPtrMap->begin();
             mapIterator != rowPtrMap->end();
             mapIterator++)
        {
            NodeAddress nextHop;
            int srcIntf;

            NodeAddress nextHopToRP;
            int  RPInterface ;

            rowPtr = mapIterator->second;
            if (rowPtr->treeState == ROUTING_PIMSM_G)
            {
                continue;
            }

            RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                    node,
                    rowPtr->RPointAddr,
                    &RPInterface,
                    &nextHopToRP);

            if (nextHopToRP == 0)
            {
                nextHopToRP = rowPtr->RPointAddr ;
                RPInterface = NetworkIpGetInterfaceIndexForNextHop(
                                  node,
                                  nextHopToRP);
            }

            RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                    node,
                    rowPtr->srcAddr,
                    &srcIntf,
                    &nextHop);

            if (nextHop == 0)
            {
                nextHop = rowPtr->srcAddr ;
            }
            else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
            {
                RoutingPimSmGetNextHopOutInterface(node,
                                                   rowPtr->srcAddr,
                                                   &nextHop,
                                                   &srcIntf);
            }

            if (srcIntf == NETWORK_UNREACHABLE)
            {
                if (DEBUG)
                {
                    printf("Node %u has invalid out interface towards 0x%x"
                            ,node->nodeId,rowPtr->srcAddr);
                }
                continue;
            }

            if (rowPtr->SPTbit &&
                rowPtr->grpAddr == grpAddress)
            {
                /* If receiving (S, G) on the SPT, we only prune off the
                   shared tree if the rpf neighbors differ. */
                if (RoutingPimSmSelectNewRPFG(node,
                                              rowPtr->grpAddr)
                        !=
                        RoutingPimSmSelectNewRPFSG(node,
                                                   rowPtr->grpAddr,
                                                   rowPtr->srcAddr))
                {
                    if (DEBUG)
                    {
                        printf("Node %u:1 add Prune(S,G, rpt) \n",
                            node->nodeId);
                    }
                    srcAddress = (NodeAddress*)
                        MEM_malloc(sizeof(NodeAddress));
                    if (srcAddress == NULL)
                    {
#ifdef EXATA
                        if (!node->partitionData->isEmulationMode)
                        {
                            ERROR_Assert(srcAddress,
                            "Cannot allocate memory for SGRpt info\n");
                        }
                        else
                        {
                            ERROR_ReportWarning("Cannot allocate memory for"
                                " SGRpt info\n");
                            return NULL;
                        }
#else
                        ERROR_Assert(srcAddress,
                            "Cannot allocate memory for SGRpt info\n");
#endif
                    }
                    memset(srcAddress, 0, sizeof(NodeAddress));

                    *srcAddress = rowPtr->srcAddr;
                    if (periodicSGRptInfoList == NULL)
                    {
                        ListInit(node, &periodicSGRptInfoList);
                    }
                    ListInsert(node, periodicSGRptInfoList, 0,
                                    (void*) srcAddress);

                    treeInfoBaseRowPtr =
                        RoutingPimSmSearchTreeInfoBaseForThisGroup(
                                    node,
                                    rowPtr->grpAddr,
                                    rowPtr->srcAddr,
                                    ROUTING_PIMSM_SGrpt);

                    /* if does not exist create one */
                    if (treeInfoBaseRowPtr == NULL)
                    {
                        //Create a (S,G,Rpt) state
                        treeInfoBaseRowPtr =
                            RoutingPimSmSetMulticastTreeInfoBase(
                                node,
                                rowPtr->grpAddr,
                                rowPtr->srcAddr,
                                ROUTING_PIMSM_SGrpt);

                        treeInfoBaseRowPtr->upstreamState =
                                        PimSm_SGrpt_Pruned;
                    }
                }
            }
              /* inherited_olist(S, G, rpt) == NULL) */
            else if ((RoutingPimSmInheritedOutListSGrpt(node,
                                  rowPtr->grpAddr,
                                  rowPtr->srcAddr,
                                  rowPtr) == 0) &&
                     (rowPtr->grpAddr == grpAddress))
            {
                /* all (*, G) olist interfaces sent rpt prunes for
                (S, G)*/
                if (DEBUG)
                {
                    printf(" Node %u: 2 add Prune(SGrpt) "
                           " to compound message\n",
                           node->nodeId);
                }
                srcAddress = (NodeAddress*)
                    MEM_malloc(sizeof(NodeAddress));
                if (srcAddress == NULL)
                {
#ifdef EXATA
                    if (!node->partitionData->isEmulationMode)
                    {
                        ERROR_Assert(srcAddress,
                        "Cannot allocate memory for SGRpt info\n");
                    }
                    else
                    {
                        ERROR_ReportWarning("Cannot allocate memory for"
                            " SGRpt info\n");
                        return NULL;
                    }
#else
                    ERROR_Assert(srcAddress,
                        "Cannot allocate memory for SGRpt info\n");
#endif
                }
                memset(srcAddress, 0, sizeof(NodeAddress));

                *srcAddress = rowPtr->srcAddr;
                if (periodicSGRptInfoList == NULL)
                {
                    ListInit(node, &periodicSGRptInfoList);
                }

                ListInsert(node, periodicSGRptInfoList, 0,
                                (void*) srcAddress);

                treeInfoBaseRowPtr =
                    RoutingPimSmSearchTreeInfoBaseForThisGroup(
                                node,
                                rowPtr->grpAddr,
                                rowPtr->srcAddr,
                                ROUTING_PIMSM_SGrpt);

                /* if does not exist create one */
                if (treeInfoBaseRowPtr == NULL)
                {
                    //Create a (S,G,Rpt) state
                    treeInfoBaseRowPtr =
                        RoutingPimSmSetMulticastTreeInfoBase(node,
                            rowPtr->grpAddr,
                            rowPtr->srcAddr,
                            ROUTING_PIMSM_SGrpt);

                    treeInfoBaseRowPtr->upstreamState =
                                    PimSm_SGrpt_Pruned;
                }
            }
            /* (RPF'(*, G) != RPF'(S, G, rpt) */
            else if ((RoutingPimSmSelectNewRPFG(node,
                                        rowPtr->grpAddr)
                            !=
                            RoutingPimSmSelectNewRPFSGrpt(node,
                                            rowPtr->grpAddr,
                                            rowPtr->srcAddr)) &&
                            (rowPtr->grpAddr == grpAddress))
            {
                // Note: we joined the shared tree,
                //      but there was an (S, G) assert and
                //      the source tree RPF neighbor is different.
                if (DEBUG)
                {
                    printf(" Node %u:3 add Prune(S,G,rpt) to "
                        "compound message\n",
                           node->nodeId);
                }
                srcAddress = (NodeAddress*)
                MEM_malloc(sizeof(NodeAddress));
                if (srcAddress == NULL)
                {
#ifdef EXATA
                    if (!node->partitionData->isEmulationMode)
                    {
                        ERROR_Assert(srcAddress,
                    "Cannot allocate memory for SGRpt info\n");
                    }
                    else
                    {
                        ERROR_ReportWarning("Cannot allocate memory for"
                            " SGRpt info\n");
                        return NULL;
                    }
#else
                    ERROR_Assert(srcAddress,
                        "Cannot allocate memory for SGRpt info\n");
#endif
                }
                memset(srcAddress, 0, sizeof(NodeAddress));

                *srcAddress = rowPtr->srcAddr;
                if (periodicSGRptInfoList == NULL)
                {
                    ListInit(node, &periodicSGRptInfoList);
                }
                ListInsert(node, periodicSGRptInfoList, 0,
                            (void*) srcAddress);

                treeInfoBaseRowPtr =
                RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                            rowPtr->grpAddr,
                            rowPtr->srcAddr,
                            ROUTING_PIMSM_SGrpt);
                /* if does not exist create one */
                if (treeInfoBaseRowPtr == NULL)
                {
                    //Create a (S,G,Rpt) state
                    treeInfoBaseRowPtr =
                        RoutingPimSmSetMulticastTreeInfoBase(node,
                            rowPtr->grpAddr,
                            rowPtr->srcAddr,
                            ROUTING_PIMSM_SGrpt);

                    treeInfoBaseRowPtr->upstreamState =
                                PimSm_SGrpt_Pruned;
                }
            }
        }   //end of for;
    }   // if matching entry;
    return (periodicSGRptInfoList);
}


/*
*  FUNCTION    : RoutingPimSmSelectNewRPFG()
*  PURPOSE     : RoutingPimSmSelectNewRPFG
*  RETURN VALUE: New RPF Neighbor
*/

NodeAddress
RoutingPimSmSelectNewRPFG(Node* node, NodeAddress groupAddr)
{
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr;
    NodeAddress RPAddr;
    NodeAddress nextHop;
    int RpIntf;

    /* search for this (G) entry in multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                         (node, groupAddr, 0, ROUTING_PIMSM_G);

    if (treeInfoBaseRowPtr != NULL)
    {
        if (treeInfoBaseRowPtr->upAssertState == PimSm_Assert_ILostAssert)
        {
            return (treeInfoBaseRowPtr->upAssertWinner);
        }
    }

    RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, RPAddr,
            &RpIntf, &nextHop);

    if (nextHop == 0)
    {
        nextHop = RPAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHop,
                                           &RpIntf);
    }

    return nextHop;
}


/*
*  FUNCTION    : RoutingPimSmSelectNewRPFSGrpt()
*  PURPOSE     : RoutingPimSmSelectNewRPFSGrpt
*  RETURN VALUE: New RPF Neighbor
*/

NodeAddress
RoutingPimSmSelectNewRPFSGrpt(Node* node, NodeAddress groupAddr,
                              NodeAddress srcAddr)
{
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr;
    NodeAddress RPAddr = 0;
    NodeAddress nextHop = 0;
    int RpIntf = 0;

    RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, RPAddr,
            &RpIntf, &nextHop);

    if (nextHop == 0)
    {
        nextHop = RPAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           RPAddr,
                                           &nextHop,
                                           &RpIntf);
    }

    /* search for this (S, G,Rpt) entry in multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                         (node, groupAddr, srcAddr, ROUTING_PIMSM_SGrpt);

    RoutingPimSmDownstream* downstreamListPtr = NULL;

    if (treeInfoBaseRowPtr != NULL)
    {
        downstreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            treeInfoBaseRowPtr, RpIntf);
    }

    if (downstreamListPtr == NULL)
    {
        if (treeInfoBaseRowPtr
            &&
            treeInfoBaseRowPtr->upInterface == RpIntf)
        {
            if (treeInfoBaseRowPtr->upAssertState ==
                    PimSm_Assert_ILostAssert)
            {
                return treeInfoBaseRowPtr->upAssertWinner;
            }
        }
    }
    else
    {
        if (downstreamListPtr->assertState == PimSm_Assert_ILostAssert)
        {
            return downstreamListPtr->assertWinner;
        }
    }

    return nextHop;
}

/*
*  FUNCTION    : RoutingPimSmSelectNewRPFSG()
*  PURPOSE     : RoutingPimSmSelectNewRPFSG
*  RETURN VALUE: New RPF Neighbor
*/

NodeAddress
RoutingPimSmSelectNewRPFSG(Node* node, NodeAddress groupAddr,
                           NodeAddress srcAddr)
{
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr;

    NodeAddress nextHop;
    int srcIntf;

    /* search for this (S, G) entry in multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                         (node,
                          groupAddr,
                          srcAddr,
                          ROUTING_PIMSM_SG);

    if (treeInfoBaseRowPtr != NULL)
    {
        if (treeInfoBaseRowPtr->upAssertState == PimSm_Assert_ILostAssert)
        {
            return treeInfoBaseRowPtr->upAssertWinner;
        }
    }

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        srcAddr,
                                                        &srcIntf,
                                                        &nextHop);

    if (nextHop == 0)
    {
        nextHop = srcAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           srcAddr,
                                           &nextHop,
                                           &srcIntf);
    }

    return nextHop;
}

/*
*  FUNCTION    : RoutingPimSmSendRegisterStopMessage()
*  PURPOSE     : RoutingPimSm Send registerStop information
*  RETURN VALUE: NULL;
*/
void
RoutingPimSmSendRegisterStopMessage(Node* node, NodeAddress grpAddr,
                                    NodeAddress mcastSrc,
                                    NodeAddress registerPktSrc,
                                    int outIntf)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    NodeAddress RPAddr = 0;

    Message* registerStopMsg;
    NodeAddress registerPktSrcNextHop = ANY_DEST;
    Int32 registerPktSrcInterface;
    RoutingPimSmRegisterStopPacket* registerStopPkt;
    unsigned int size;

    /* Send Join or Prune for anly one (S, G) pair */
    registerStopMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    MULTICAST_PROTOCOL_PIM,
                                    MSG_ROUTING_PimPacket);

    size = sizeof(RoutingPimSmRegisterStopPacket);

    MESSAGE_PacketAlloc(node, registerStopMsg, size, TRACE_PIM);

    registerStopPkt = (RoutingPimSmRegisterStopPacket*)
                      MESSAGE_ReturnPacket(registerStopMsg);

    /* Set the Header */
    RoutingPimCreateCommonHeader(&registerStopPkt->commonHeader,
                                 ROUTING_PIM_REGISTER_STOP);

    /* set the group information in packet field*/
    registerStopPkt->encodedGroupAddr.addressFamily = 1;
    registerStopPkt->encodedGroupAddr.encodingType = 0;
    registerStopPkt->encodedGroupAddr.reserved = 0;
    registerStopPkt->encodedGroupAddr.maskLength = 32;
    registerStopPkt->encodedGroupAddr.groupAddr = grpAddr;

    /* set the sorce information in packet field*/
    registerStopPkt->encodedSrcAddr.addressFamily = 1;
    registerStopPkt->encodedSrcAddr.encodingType = 0;
    setNodeAddressInCharArray(registerStopPkt->encodedSrcAddr.unicastAddr,
                              mcastSrc);

    /* set ipheader Source % ip header destination */
    /* The IP source address is the address to which the register
       was addressed. The IP destination address is the source
       address of the register message. */



    if ((outIntf >= 0)
            && ((unsigned)outIntf != (unsigned)NETWORK_UNREACHABLE))
    {

        TosType priority = IPTOS_PREC_INTERNETCONTROL;

#ifdef ADDON_BOEINGFCS
        if (MulticastCesRpimActiveOnInterface(node, outIntf))
        {
            priority = IPTOS_PREC_PRIORITY;
        }
#endif
        // Send the packet to next hop of source
        RoutingPimGetInterfaceAndNextHopFromForwardingTable(
            node,
            registerPktSrc,
            &registerPktSrcInterface,
            &registerPktSrcNextHop);
        if (registerPktSrcNextHop == 0)
        {
            registerPktSrcNextHop = registerPktSrc ;
        }
        else if (registerPktSrcNextHop == (unsigned)NETWORK_UNREACHABLE)
        {
            RoutingPimSmGetNextHopOutInterface(node,
                                               registerPktSrc,
                                               &registerPktSrcNextHop,
                                               &registerPktSrcInterface);
        }

        if (registerPktSrcInterface == NETWORK_UNREACHABLE)
        {
            if (DEBUG)
            {
                printf("Node %u has invalid out interface towards 0x%x"
                            ,node->nodeId,registerPktSrc);
            }
            return;
        }

        RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);
        /* Now send packet out through this interface */
        NetworkIpSendRawMessageToMacLayer(
            node,
            MESSAGE_Duplicate(node, registerStopMsg),
            RPAddr,
            registerPktSrc,
            priority,
            IPPROTO_PIM,
            IPDEFTTL,
            registerPktSrcInterface,
            registerPktSrcNextHop);

        if (DEBUG_FORWARD_PACKET)
        {
            char clockStr[100];

            ctoa(node->getNodeTime(), clockStr);

            printf("Node %d send registerStop to intf %d at time %s\n",
                   node->nodeId, outIntf, clockStr);
            printf(" The Pkt is for %x \n", registerPktSrc);
            printf(" pktGrp: %x  \n",
                   registerStopPkt->encodedGroupAddr.groupAddr);
        }

#ifdef ADDON_BOEINGFCS
        if (NetworkIpIsMyIP(node, registerPktSrc))
        {
            RoutingPimSmRegisterStopPacket* tempPkt =
                (RoutingPimSmRegisterStopPacket*) MESSAGE_ReturnPacket(
                                                            registerStopMsg);

            RoutingPimSmHandleRegisterStopPacket(
                                  node,
                                  NetworkIpGetInterfaceAddress(node,outIntf),
                                  tempPkt);
        }
#endif

        pimDataSm->stats.numOfRegisterStopPacketForwarded++;
#ifdef ADDON_DB
        pimDataSm->smSummaryStats.m_NumRegisterStopSent++;
#endif
        MESSAGE_Free(node, registerStopMsg);
    }
}

/*
*  FUNCTION    : RoutingPimSmHandleRegisterStopPacket()
*  PURPOSE     : RoutingPimSm Handle RegisterStop Packet
*  RETURN VALUE: NULL;
*/
void
RoutingPimSmHandleRegisterStopPacket(
                            Node* node,
                            NodeAddress srcAddr,
                            RoutingPimSmRegisterStopPacket * registerStopPkt)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr;
    RoutingPimSmRegisterStopTimerInfo    timerInfo;
    clocktype delay;

    NodeAddress nextHop;
    int srcIntf;
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node, srcAddr,
            &srcIntf, &nextHop);

    if (nextHop == 0)
    {
        nextHop = srcAddr ;
    }
    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
    {
        RoutingPimSmGetNextHopOutInterface(node,
                                           srcAddr,
                                           &nextHop,
                                           &srcIntf);
    }

    if (srcIntf == NETWORK_UNREACHABLE)
    {
        if (DEBUG)
        {
            printf("Node %u has invalid out interface towards 0x%x"
                   ,node->nodeId,srcAddr);
        }
        return;
    }

    /* search for this (S, G) entry in multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                          (node,
                          registerStopPkt->encodedGroupAddr.groupAddr,
                          getNodeAddressFromCharArray(
                          registerStopPkt->encodedSrcAddr.unicastAddr),
                          ROUTING_PIMSM_SG);

    if (treeInfoBaseRowPtr != NULL)
    {
        if (DEBUG_ERRORS) {
            printf("the associated register state %d \n",
                   treeInfoBaseRowPtr->registerState);
            printf("Node %u: need to send data to DR %u without \
                   registering \n", node->nodeId, srcAddr);
        }

        /* check the associated register state */
        if (treeInfoBaseRowPtr->registerState == PimSm_Register_Join)
        {
            /* move to prune state */
            treeInfoBaseRowPtr->registerState = PimSm_Register_Prune;

            /* remove tunnel */

            if (DEBUG_ERRORS) {
                printf(" Node %u: RemoveTunnel\n", node->nodeId);
            }

            treeInfoBaseRowPtr->isTunnelPresent = FALSE;

            /* set register Stop Timer */
            timerInfo.srcAddr = getNodeAddressFromCharArray(
                            registerStopPkt->encodedSrcAddr.unicastAddr);
            timerInfo.grpAddr = registerStopPkt->encodedGroupAddr.groupAddr;
            timerInfo.intfIndex = srcIntf;
            timerInfo.seqNo =
                treeInfoBaseRowPtr->registerStopTimerSeq + 1;

            timerInfo.treeState = ROUTING_PIMSM_SG;

            /* Note the current KeepAliveTimerSeq */
            treeInfoBaseRowPtr->registerStopTimerSeq++;

            delay = (clocktype) (
            pimDataSm->routingPimSmRegisterSupressionTime
                     *  (RANDOM_erand(pimDataSm->seed) + 0.5));
            if (DEBUG)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("Node %u interface %d scheduling "
                    "MSG_ROUTING_PimSmRegisterStopTimeOut at time %s \n",
                                 node->nodeId,
                                 srcIntf,
                                 clockStr);
            }

            RoutingPimSetTimer(node,
                               srcIntf,
                               MSG_ROUTING_PimSmRegisterStopTimeOut,
                               (void*) &timerInfo, delay);
        }
        else if (treeInfoBaseRowPtr->registerState ==
                        PimSm_Register_JoinPending)
        {
            /* move to prune state */
            treeInfoBaseRowPtr->registerState = PimSm_Register_Prune;

            /* set register Stop Timer */
            timerInfo.srcAddr = getNodeAddressFromCharArray(
                            registerStopPkt->encodedSrcAddr.unicastAddr);
            timerInfo.grpAddr =
                    registerStopPkt->encodedGroupAddr.groupAddr;
            timerInfo.intfIndex = srcIntf;
            timerInfo.seqNo =
                treeInfoBaseRowPtr->registerStopTimerSeq + 1;

            timerInfo.treeState = ROUTING_PIMSM_SG;

            /* Note the current KeepAliveTimerSeq */
            treeInfoBaseRowPtr->registerStopTimerSeq++;
            delay = (clocktype) (
            pimDataSm->routingPimSmRegisterSupressionTime
                     *  (RANDOM_erand(pimDataSm->seed) + 0.5));
            if (DEBUG)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("Node %u interface %d scheduling "
                    "MSG_ROUTING_PimSmRegisterStopTimeOut at time %s \n",
                                 node->nodeId,
                                 srcIntf,
                                 clockStr);
            }

            RoutingPimSetTimer(node,
                               srcIntf,
                               MSG_ROUTING_PimSmRegisterStopTimeOut,
                               (void*) &timerInfo, delay);
        }
    }
}

/*
*  FUNCTION    : RoutingPimSmPruneDesiredSGrpt()
*  PURPOSE     : RoutingPimSmPruneDesiredSGrpt
*               bool PruneDesired(S, G, rpt)
*               {
*                   return (RPTJoinDesired(G) AND
*                       (inherited_olist(S, G, rpt) == NULL
*                       OR (SPTbit(S, G) == TRUE
*                       AND (RPF'(*, G) != RPF'(S, G)))))
*               }
*  RETURN VALUE: TRUE if condition satisfies;
*/
BOOL
RoutingPimSmPruneDesiredSGrpt(Node* node, NodeAddress grpAddr,
                              NodeAddress srcAddr, int interfaceId)
{
    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr;

    /* search for this (S, G) entry in multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                         (node, grpAddr, srcAddr,
                          ROUTING_PIMSM_SG);

    if (treeInfoBaseRowPtr == NULL)
    {
        treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                             (node, grpAddr, srcAddr,
                              ROUTING_PIMSM_SGrpt);
    }

    if (treeInfoBaseRowPtr == NULL)
    {
        return FALSE;
    }

    if ((RoutingPimSmRPTJoinDesiredG(node, grpAddr, srcAddr, interfaceId))
            && ((RoutingPimSmInheritedOutListSGrpt(node, grpAddr, srcAddr,
                              treeInfoBaseRowPtr) == 0)
                || ((treeInfoBaseRowPtr->SPTbit)
                    && (RoutingPimSmSelectNewRPFG(node, grpAddr)
                    != RoutingPimSmSelectNewRPFSG(node, grpAddr, srcAddr)))))
    {
        return TRUE;
    }
    return FALSE;
}


/*
*  FUNCTION    : RoutingPimSmSendCandidateRPPacket()
*  PURPOSE     : RoutingPimSm Send Candidate RP information
*                through out Interface
*  RETURN VALUE: NULL;
*/
BOOL
RoutingPimSmSendCandidateRPPacket(Node* node,
                                  int interfaceIndex)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    Message                             * candidateRPMsg;
    RoutingPimSmCandidateRPPacket       * candidateRPPkt;
    RoutingPimEncodedGroupAddr          grpInfo;
    memset(&grpInfo, 0, sizeof(RoutingPimEncodedGroupAddr));

    char* dataPtr = NULL;
    unsigned int size = 0;;
    unsigned int numGroup = 0;
    int outIntf = 0;;
    NodeAddress nextHop = (unsigned) NETWORK_UNREACHABLE;
    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];
    ListItem* tmpListItem = NULL;
    RoutingPimSmRPAccessList* accessListInfo = NULL;

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                                                   node,
                                                   thisInterface->currentBSR,
                                                   &outIntf,
                                                   &nextHop);

#ifdef ADDON_BOEINGFCS
    if (MulticastCesRpimActiveOnNode(node) &&
        outIntf == NETWORK_UNREACHABLE)
    {
        if (NetworkIpIsMyIP(node, thisInterface->currentBSR))
        {
            nextHop = thisInterface->currentBSR;
            outIntf = NetworkIpGetInterfaceIndexFromAddress(node,
                thisInterface->currentBSR);
        }
        else
        {
            return FALSE;
        }
    }

    if (MulticastCesRpimActiveOnInterface(node, outIntf)) {
        BOOL reachable = MulticastCesRpimIsReachable(nextHop);
        if (!reachable)
            return FALSE;
    }
#endif

    if (nextHop == 0)
    {
        nextHop = thisInterface->currentBSR;
    }

    if (DEBUG_ERRORS)
    {
        char BSAddr[100];
        IO_ConvertIpAddressToString(thisInterface->currentBSR, BSAddr);
        printf("Node %u: is ready for sending candidateRP Msg\n",
               node->nodeId);
        printf("available BSRAddr = %s \n", BSAddr);
        printf("available Intf = %d \n", outIntf);
        printf("available nextHop = %u \n", nextHop);
    }

    candidateRPMsg = MESSAGE_Alloc(node,
                                   NETWORK_LAYER,
                                   MULTICAST_PROTOCOL_PIM,
                                   MSG_ROUTING_PimPacket);

    numGroup = thisInterface->rpData.rpAccessList->size;
    size = sizeof(RoutingPimSmCandidateRPPacket)
           + (sizeof(RoutingPimEncodedGroupAddr)* numGroup);

    MESSAGE_PacketAlloc(node, candidateRPMsg, size, TRACE_PIM);

    candidateRPPkt = (RoutingPimSmCandidateRPPacket*)
                     MESSAGE_ReturnPacket(candidateRPMsg);

    /* Set the Header */
    RoutingPimCreateCommonHeader(&candidateRPPkt->commonHeader,
                                 ROUTING_PIM_CANDIDATE_RP);

    /* set the prefix Count, priority  & holdtime*/
    candidateRPPkt->prefixCount = (unsigned char)
                                    thisInterface->rpData.rpAccessList->size;
    candidateRPPkt->priority = thisInterface->rpData.rpPriority;
    candidateRPPkt->holdtime = (unsigned short)
                ((pimDataSm->routingPimSmCandidateRpTimeout * 2.5) / SECOND);

    /* set the unicast RP ADDRESS */
    candidateRPPkt->encodedUnicastRP.addressFamily = 1;
    candidateRPPkt->encodedUnicastRP.encodingType = 0;
    setNodeAddressInCharArray(candidateRPPkt->encodedUnicastRP.unicastAddr,
                              thisInterface->ipAddress);

    dataPtr = (char*) candidateRPPkt + sizeof(RoutingPimSmCandidateRPPacket);

    if (DEBUG_BS)
    {
       RoutingPimSmPrintRPGroupAccessList(node,
                                          interfaceIndex);
    }

    tmpListItem = thisInterface->rpData.rpAccessList->first;

    if (tmpListItem == NULL) //no groups present
    {
        grpInfo.addressFamily = 1;
        grpInfo.encodingType = 0;
        grpInfo.reserved = 0;

        grpInfo.maskLength = (unsigned char)4;
        grpInfo.groupAddr = 0xE0000000; //224.0.0.0
        memcpy(dataPtr, &grpInfo, sizeof(RoutingPimEncodedGroupAddr));
    }
    else
    {
        while (numGroup)
        {
            accessListInfo = (RoutingPimSmRPAccessList*)(tmpListItem->data);

            grpInfo.addressFamily = 1;
            grpInfo.encodingType = 0;
            grpInfo.reserved = 0;

            grpInfo.maskLength = (unsigned char)
            RoutingPimGetMaskLengthFromSubnetMask(accessListInfo->groupMask);
            grpInfo.groupAddr = accessListInfo->groupAddr;

            memcpy(dataPtr, &grpInfo, sizeof(RoutingPimEncodedGroupAddr));
            dataPtr = dataPtr + sizeof(RoutingPimEncodedGroupAddr);

            numGroup--;
            tmpListItem = tmpListItem->next;
        }//end of while
    }//end of else

    if (DEBUG_ERRORS)
    {
        printf("c_RP outInterface %d \n", outIntf);
    }
    if ((outIntf >= 0)
        &&
        ((unsigned)outIntf != (unsigned)NETWORK_UNREACHABLE))
    {

        TosType priority = IPTOS_PREC_INTERNETCONTROL;
        BOOL IamCurrentBSR = FALSE;

#ifdef ADDON_BOEINGFCS
        if (MulticastCesRpimActiveOnInterface(node, outIntf))
        {
            priority = IPTOS_PREC_PRIORITY;
        }
#endif

        for (int i = 0; i < node->numberInterfaces; i++)
        {
            if (pim->interface[i].ipAddress == thisInterface->currentBSR)
            {
                if (DEBUG_BS)
                {
                    printf("Node %u: I am the Current BSR \n", node->nodeId);
                }
                IamCurrentBSR = TRUE;
                break;
            }
        }

        if (IamCurrentBSR == TRUE)
        {
            RoutingPimSmHandleCandidateRPPacket(
                node,
                candidateRPMsg,
                NetworkIpGetInterfaceIndexFromAddress(
                                                 node,
                                                 thisInterface->currentBSR));
        }
        else
        {
            /* Now send packet out through this interface */
            NetworkIpSendRawMessageToMacLayer(
            node,
            MESSAGE_Duplicate(node, candidateRPMsg),
            NetworkIpGetInterfaceAddress(node,outIntf),
            thisInterface->currentBSR,
            priority,
            IPPROTO_PIM,
            IPDEFTTL,
            outIntf,
            ANY_DEST);
        }

        if (DEBUG_BS)
        {
            char clockStr[100];
            ctoa(node->getNodeTime(), clockStr);
            printf("\n");
            printf("Node %u send candidateRP to intf %d at time %s\n",
                   node->nodeId, outIntf, clockStr);
            printf(" associated RPAddr = 0x%x \n",
                   getNodeAddressFromCharArray(
                   candidateRPPkt->encodedUnicastRP.unicastAddr));
            printf("associated Grp = 0x%x \n", grpInfo.groupAddr);

            RoutingPimSmPrintCandidateRPAdvMessage(node,
                                                   candidateRPMsg);
        }

#ifdef ADDON_BOEINGFCS
        if (NetworkIpIsMyIP(node, thisInterface->currentBSR))
        {
            RoutingPimSmHandleCandidateRPPacket(node,
                                                candidateRPMsg,
                                                outIntf);
        }
#endif

        pimDataSm->stats.numOfCandidateRPPacketForwarded++;
        pimDataSm ->lastCandidateRPSend = node->getNodeTime();
#ifdef ADDON_DB
        pimDataSm->smSummaryStats.m_NumCandidateRPSent++;
#endif
    }
    else
    {
        MESSAGE_Free(node, candidateRPMsg);
        return FALSE;
    }

    MESSAGE_Free(node, candidateRPMsg);
    return TRUE;
}

/* FUNCTION     :RoutingPimSmForwardBSM()
*  PURPOSE      :Forward Bootstrap message
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimSmForwardBSM(Node* node,
                       Message* bootstrapMsg,
                       int incomingIntf)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    PimDataSm* pimSmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimInterface* thisInterface = NULL;
    clocktype delay = 0;
    BOOL bootstrapSent = FALSE;
    RoutingPimSmBootstrapTimerInfo timerInfo;
    memset(&timerInfo, 0, sizeof(RoutingPimSmBootstrapTimerInfo));

    int i = 0;

    for (i = 0 ; i < node->numberInterfaces ; i++)
    {
        if (incomingIntf == i) //its the incoming interface
        {
            continue;
        }

        thisInterface = &pim->interface[i];

        //updating the current BSR related info on other interfaces as it
        //might happen that these interfaces will not receive this BSM as the
        //other party will not forward this BSM back on incoming interface
        thisInterface->currentBSR = pim->interface[incomingIntf].currentBSR;

        thisInterface->currentBSRPriority =
                         pim->interface[incomingIntf].currentBSRPriority;

        thisInterface->cBSRState = CANDIDATE_BSR;

        /*
            according to the C-BSR state machine when the bsr state gets
            converted into C_BSR then the following needs to be done
            1.Forward BSM         //no need to do this here
            2.Store RP Set
            3.Set Bootstrap timer to BS_TIMEOUT
        */

        //storing the RP Set
        if (DEBUG_BS)
        {
            printf("\nInvoking storerpset from forwardbsm\n");
        }
        RoutingPimSmStoreRPSet(node,
                               i,
                               bootstrapMsg);


        //Setting Bootstrap timer to BS_TIMEOUT
        timerInfo.srcAddr = NetworkIpGetInterfaceAddress(node, i);
        timerInfo.intfIndex = i;

        timerInfo.seqNo = pim->interface[i].bootstrapTimerSeq + 1;
        /* Note the current bootstrapTimerSeq */
        pim->interface[i].bootstrapTimerSeq++;

        delay = ((2 * pimSmData->routingPimSmBootstrapTimeout) +
                (10 * SECOND));

        RoutingPimSetTimer(node,
                           timerInfo.intfIndex,
                           MSG_ROUTING_PimSmBootstrapTimeOut,
                           (void*) &timerInfo,
                           delay);

        //If PIM neighbors exist on this interface,forward the packet
        // REMOVE THIS?
        if (thisInterface->smInterface->neighborList->size > 0)
        {
            /* Used to jitter bootstrap packet broadcast */
            delay = (clocktype) RANDOM_erand(pimSmData->seed)*
                    ROUTING_PIM_BROADCAST_JITTER;

            TosType priority = IPTOS_PREC_INTERNETCONTROL;

#ifdef ADDON_BOEINGFCS
            if (MulticastCesRpimActiveOnInterface(node, i))
            {
                priority = IPTOS_PREC_PRIORITY;
            }
#endif

            /* Now send packet out through this interface */
           if (DEBUG_BS)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("\nNode %d : Forwarding BSM on interface %d at %s\n",
                        node->nodeId, i, clockStr);
            }

            NetworkIpSendRawMessageToMacLayer(
                node,
                MESSAGE_Duplicate(node, bootstrapMsg),
                NetworkIpGetInterfaceAddress(node, i),
                ALL_PIM_ROUTER,
                priority,
                IPPROTO_PIM,
                1,
                i,
                ANY_DEST);

            bootstrapSent = TRUE;
        }
    }

    if (bootstrapSent == TRUE)
    {
        pimSmData->stats.numOfBootstrapPacketForwarded++;
        pimSmData->lastBootstrapSend = node->getNodeTime();
    }
}

/*
*  FUNCTION    : RoutingPimSmOriginateBSM()
*  PURPOSE     : RoutingPimSm Send Bootstrap information thru out Interface
*  RETURN VALUE: NULL;
*/
void
RoutingPimSmOriginateBSM(Node* node,
                         int interfaceIndex)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];

    Message                           * bootstrapMsg;
    RoutingPimSmBootstrapPacket       * bootstrapPkt;
    RoutingPimSmEncodedGroupBSRInfo    grpInfo;
    memset(&grpInfo, 0, sizeof(RoutingPimSmEncodedGroupBSRInfo));

    LinkedList* grpHashList = NULL;

    ListItem* rpSetItem = NULL;
    ListItem* rpInfoListItem = NULL;

    ListItem* grpHashListItem = NULL;

    BOOL grpNotAlreadyPresent = FALSE;
    RoutingPimSmGroupHashList* smGroupHashList = NULL;
    RoutingPimSmEncodedUnicastRPInfo*   rpInfoListPtr = NULL;
    RoutingPimSmEncodedUnicastRPInfo*   newRPInfoListPtr = NULL;
    RoutingPimSmRPList* RPListPtr = NULL;
    RoutingPimSmGroupHashList* newGroupHashListPtr = NULL;

    char* dataPtr = NULL;
    unsigned int numRP = 0;
    unsigned int numGroups = 0;
    unsigned int size = 0;
    int i = 0;
    BOOL bootstrapOriginated = FALSE;

    if (DEBUG_BS)
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);

        printf("Node %u:Create Bootstrap Msg at %s\n",node->nodeId,clockStr);
        printf("current RPList size %d \n", pimDataSm->RPList->size);
        RoutingPimSmPrintRPList(node);
    }

    ListInit(node, &grpHashList);
    grpHashListItem = grpHashList->first;
    rpSetItem = pimDataSm->RPList->first;

    while (rpSetItem)
    {
        RPListPtr = (RoutingPimSmRPList*)rpSetItem->data;
        //search group address of rpSetItem in the hash table

        grpNotAlreadyPresent = FALSE;
        grpHashListItem = grpHashList->first;
        do
        {
            if (grpHashListItem == NULL)
            {
                grpNotAlreadyPresent = TRUE;
                break;
            }
            else
            {
                smGroupHashList = (RoutingPimSmGroupHashList*)
                                                    (grpHashListItem->data);

                if ((smGroupHashList->grpInfo.encodedGrpAddr.groupAddr ==
                            RPListPtr->grpPrefix)
                    &&
                    (smGroupHashList->grpInfo.encodedGrpAddr.maskLength ==
                            RPListPtr->maskLength))
                {
                    newRPInfoListPtr = (RoutingPimSmEncodedUnicastRPInfo*)
                        MEM_malloc(sizeof(RoutingPimSmEncodedUnicastRPInfo));

                    newRPInfoListPtr->encodedUnicastRP.addressFamily = 1;
                    newRPInfoListPtr->encodedUnicastRP.encodingType  = 0;

                    setNodeAddressInCharArray(
                              newRPInfoListPtr->encodedUnicastRP.unicastAddr,
                              RPListPtr->RPAddress);

                    //RFC:5059::Section:3.3
                    /*
                    For each RP-address, the "RP-Holdtime" field is
                    set to the Holdtime from the C-RP-Set, subject
                    to the constraint that it MUST be larger than
                    BS_Period and SHOULD be larger than 2.5 times
                    BS_Period to allow for some Bootstrap messages
                    getting lost.If some holdtimes from the C-RP-Sets
                    do not satisfy this constraint,the BSR MUST replace
                    those holdtimes with a value satisfying the constraint.
                    An exception to this is the holdtime of zero,which
                    is used to immediately withdraw mappings.
                    */

                    if ((RPListPtr->holdtime != 0)
                        &&
                        (RPListPtr->holdtime <=
                        (pimDataSm->routingPimSmBootstrapTimeout / SECOND)))
                    {
                        newRPInfoListPtr->RPHoldTime =
                         RPListPtr->holdtime +
                         (unsigned short)(2.5 *
                         (pimDataSm->routingPimSmBootstrapTimeout / SECOND));
                    }
                    else
                    {
                        newRPInfoListPtr->RPHoldTime = RPListPtr->holdtime;
                    }

                    newRPInfoListPtr->RPPriority = RPListPtr->priority;
                    newRPInfoListPtr->reserved   = 0;

                    ListInsert(node,
                               smGroupHashList->encodedUnicastRPInfoList,
                               (clocktype)0,
                               (void*) newRPInfoListPtr);

                    smGroupHashList->grpInfo.rpCount++;
                    smGroupHashList->grpInfo.fragmentRPCount++;

                    break;  //break from the inner do-while
                }
            }//end of else

            grpHashListItem = grpHashListItem->next;

            if (grpHashListItem == NULL)
            {
                grpNotAlreadyPresent = TRUE;
            }
        } while (grpHashListItem);

        if (grpNotAlreadyPresent == TRUE)
        {
            newGroupHashListPtr = (RoutingPimSmGroupHashList*)
                               MEM_malloc(sizeof(RoutingPimSmGroupHashList));
            memset(newGroupHashListPtr,0,sizeof(RoutingPimSmGroupHashList));

            grpInfo.encodedGrpAddr.addressFamily = 1;
            grpInfo.encodedGrpAddr.encodingType = 0;
            grpInfo.encodedGrpAddr.reserved = 0;
            grpInfo.encodedGrpAddr.maskLength = RPListPtr->maskLength;
            grpInfo.encodedGrpAddr.groupAddr = RPListPtr->grpPrefix;

            grpInfo.rpCount = 1;
            grpInfo.fragmentRPCount = 1;
            grpInfo.reserved = 0;

            memcpy(&newGroupHashListPtr->grpInfo,
                   &grpInfo,
                   sizeof(RoutingPimSmEncodedGroupBSRInfo));


            newRPInfoListPtr = (RoutingPimSmEncodedUnicastRPInfo*)
                        MEM_malloc(sizeof(RoutingPimSmEncodedUnicastRPInfo));
            memset(newRPInfoListPtr,
                   0,
                   sizeof(RoutingPimSmEncodedUnicastRPInfo));

            newRPInfoListPtr->encodedUnicastRP.addressFamily = 1;
            newRPInfoListPtr->encodedUnicastRP.encodingType  = 0;

            setNodeAddressInCharArray(
                newRPInfoListPtr->encodedUnicastRP.unicastAddr,
                RPListPtr->RPAddress);

            newRPInfoListPtr->RPHoldTime = RPListPtr->holdtime;
            newRPInfoListPtr->RPPriority = RPListPtr->priority;
            newRPInfoListPtr->reserved   = 0;

            ListInit(node, &newGroupHashListPtr->encodedUnicastRPInfoList);

            ListInsert(node,
                       newGroupHashListPtr->encodedUnicastRPInfoList,
                       (clocktype)0,
                       (void*) newRPInfoListPtr);

            ListInsert(node,
                       grpHashList,
                       (clocktype)0,
                       (void*) newGroupHashListPtr);

            grpHashListItem = grpHashList->first;
        }

        rpSetItem = rpSetItem->next;
    }//End while

    numRP = pimDataSm->RPList->size;

    bootstrapMsg = MESSAGE_Alloc(node,
                                 NETWORK_LAYER,
                                 MULTICAST_PROTOCOL_PIM,
                                 MSG_ROUTING_PimPacket);

    size = sizeof(RoutingPimSmBootstrapPacket)
           + (sizeof(RoutingPimSmEncodedGroupBSRInfo) * grpHashList->size)
           + (sizeof(RoutingPimSmEncodedUnicastRPInfo) * numRP);

    MESSAGE_PacketAlloc(node, bootstrapMsg, size, TRACE_PIM);
    bootstrapPkt = (RoutingPimSmBootstrapPacket*)
                   MESSAGE_ReturnPacket(bootstrapMsg);

    /* Set the Header */
    RoutingPimCreateCommonHeader(&bootstrapPkt->commonHeader,
                                 ROUTING_PIM_BOOTSTRAP);

    /* set the fragmenttag, hashMasklength & BSR priority */
    bootstrapPkt->fragmentTag = 1;
    bootstrapPkt->pimsmHashMaskLength = PIMSM_HASH_MASK_LENGTH;
    bootstrapPkt->BSRPriority = thisInterface->bsrData.bsrPriority;

    /* set the unicast BSR Address */
    bootstrapPkt->encodedUnicastBSR.addressFamily = 1;
    bootstrapPkt->encodedUnicastBSR.encodingType = 0;
    setNodeAddressInCharArray(bootstrapPkt->encodedUnicastBSR.unicastAddr,
                              thisInterface->currentBSR);

    /* put the group info in the pkt */
    dataPtr = (char*) bootstrapPkt + sizeof(RoutingPimSmBootstrapPacket);

    //add each group address and its corresponding RPs
    numGroups = grpHashList->size;
    grpHashListItem = grpHashList->first;
    while (numGroups)
    {
        smGroupHashList =
            (RoutingPimSmGroupHashList*)(grpHashListItem->data);

        memcpy(dataPtr,
               &smGroupHashList->grpInfo,
               sizeof(RoutingPimSmEncodedGroupBSRInfo));

        dataPtr += sizeof(RoutingPimSmEncodedGroupBSRInfo);

        rpInfoListItem = smGroupHashList->encodedUnicastRPInfoList->first;

        while (rpInfoListItem)
        {
            rpInfoListPtr =
                (RoutingPimSmEncodedUnicastRPInfo*)rpInfoListItem->data;

            memcpy(dataPtr,
                   rpInfoListPtr,
                   sizeof(RoutingPimSmEncodedUnicastRPInfo));

            if (DEBUG_ERRORS)
            {
                printf("include RP %s in bootstrap Msg \n",
                rpInfoListPtr->encodedUnicastRP.unicastAddr);
            }

            /* increase dataPtr to point next RP Info */
            dataPtr += sizeof(RoutingPimSmEncodedUnicastRPInfo);
            rpInfoListItem = rpInfoListItem->next;
        }

        grpHashListItem = grpHashListItem->next;
        numGroups--;
    }

    if (DEBUG_BS)
    {
        RoutingPimSmPrintBootStrapMessage(node,
                                          bootstrapMsg);
    }
    for (i = 0 ; i < node->numberInterfaces ; i++)
    {
        thisInterface = &pim->interface[i];

        //If PIM neighbors exist on this interface,forward the packet
        if (thisInterface->smInterface->neighborList->size > 0)
        {
            TosType priority = IPTOS_PREC_INTERNETCONTROL;

#ifdef ADDON_BOEINGFCS
            if (MulticastCesRpimActiveOnInterface(node, i))
            {
                priority = IPTOS_PREC_PRIORITY;
            }
#endif


            /* Now send packet out through this interface */
            NetworkIpSendRawMessageToMacLayer(
                node,
                MESSAGE_Duplicate(node, bootstrapMsg),
                NetworkIpGetInterfaceAddress(node, i),
                ALL_PIM_ROUTER,
                priority,
                IPPROTO_PIM,
                1,
                i,
                ANY_DEST);

            bootstrapOriginated = TRUE;
        }
    }

    if (bootstrapOriginated == TRUE)
    {
        pimDataSm->stats.numOfbootstrapPacketOriginated++;
        pimDataSm->lastBootstrapSend = node->getNodeTime();
#ifdef ADDON_DB
        pimDataSm->smSummaryStats.m_NumBootstrapSent++;
#endif
    }

    //Freeing the List
    grpHashListItem = grpHashList->first;
    while (grpHashListItem)
    {
        smGroupHashList =(RoutingPimSmGroupHashList*)(grpHashListItem->data);

        ListFree(node, smGroupHashList->encodedUnicastRPInfoList,FALSE);
        grpHashListItem = grpHashListItem->next;
    }

    ListFree(node, grpHashList, FALSE);

    MESSAGE_Free(node, bootstrapMsg);
}

/*
*  FUNCTION    : RoutingPimSmIsEmptyCRPMsg ()
*  PURPOSE     : Check for empty candidate RP message
*  RETURN VALUE: BOOL
*/
BOOL
RoutingPimSmIsEmptyCRPMsg(Message* candidateRPMsg)
{
    if (candidateRPMsg->packetSize == sizeof(RoutingPimSmCandidateRPPacket))
    {
        return TRUE;
    }

    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmHandleCandidateRPPacket()
*  PURPOSE     : RoutingPimSm handle Candidate RP Message
*  RETURN VALUE: NULL;
*/
void
RoutingPimSmHandleCandidateRPPacket(Node* node,
                                    Message* candidateRPMsg,
                                    int incomingIntf)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    RoutingPimSmCandidateRPPacket* candidateRPPkt =
       (RoutingPimSmCandidateRPPacket*) MESSAGE_ReturnPacket(candidateRPMsg);

    RoutingPimEncodedGroupAddr grpInfo;
    memset(&grpInfo, 0, sizeof(RoutingPimEncodedGroupAddr));

    RoutingPimSmBootstrapTimerInfo    bsrTimerInfo;
    memset(&bsrTimerInfo, 0, sizeof(RoutingPimSmBootstrapTimerInfo));

    RoutingPimSmGrpToRPTimerInfo      cgetTimerInfo;
    memset(&cgetTimerInfo, 0, sizeof(RoutingPimSmGrpToRPTimerInfo));

    RoutingPimInterface* thisInterface = NULL;

    clocktype delay = 0;

    RoutingPimSmRPList* existingRPListPtr = NULL;

    int numGroupInfo = 0;
    BOOL isEmptyCRPMsg = FALSE;
    int i = 0;

    RoutingPimSmRPList* RPListPtr = NULL;

    BOOL isRPSetChanged = FALSE;
    ListItem* rpListItem = NULL;

    pimDataSm->stats.numOfCandidateRPPacketReceived++;
#ifdef ADDON_DB
    pimDataSm->smSummaryStats.m_NumCandidateRPRecvd++;
#endif
    char* dataPtr = NULL;
    dataPtr = (char*)candidateRPPkt + sizeof(RoutingPimSmCandidateRPPacket);

    if (DEBUG_BS)
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);

        printf("\nNode %d : Candidate RP Advertisement received at %s"
               "on interface %d\n",node->nodeId, clockStr, incomingIntf);

        RoutingPimSmPrintCandidateRPAdvMessage(node,
                                               candidateRPMsg);
    }

    for (i = 0; i < node->numberInterfaces; i++)
    {
        thisInterface = &pim->interface[i];

        if (thisInterface->cBSRState != ELECTED_BSR)
        {
            //Discard the packet
            if (DEBUG_ERRORS)
            {
                printf(" Node %u: I am not the Current BSR \n",node->nodeId);
            }
            continue;
        }

        isEmptyCRPMsg = RoutingPimSmIsEmptyCRPMsg(candidateRPMsg);

        if (isEmptyCRPMsg)
        {
            if (DEBUG_BS)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(),clockStr);

                printf("Node %d :Empty Candidate RP Advertisement"
                       " received at %s on interface %d\n",
                       node->nodeId, clockStr, incomingIntf);
            }
            numGroupInfo = 1;
        }
        else
        {
            numGroupInfo = candidateRPPkt->prefixCount;
            if (DEBUG_BS)
            {
                printf("\nNode %d : Candidate RP Advertisement contains %d "
                       "number of groups \n",node->nodeId,numGroupInfo);
            }
        }

        while (numGroupInfo)
        {
            if (isEmptyCRPMsg)
            {
                grpInfo.groupAddr = 0xE0000000;
                grpInfo.maskLength = 4;
            }
            else
            {
                memcpy(&grpInfo,
                       dataPtr,
                       sizeof(RoutingPimEncodedGroupAddr));

                dataPtr = dataPtr + sizeof(RoutingPimEncodedGroupAddr);
            }

            rpListItem = RoutingPimSmIsRPMappingPresent(
                                          node,
                                          &grpInfo,
                                          &candidateRPPkt->encodedUnicastRP);


            if (rpListItem != NULL)
            {
                existingRPListPtr = (RoutingPimSmRPList*)rpListItem->data;
            }
            else
            {
                existingRPListPtr = NULL;
            }
            //If mapping does not exist
            if (existingRPListPtr == NULL)
            {
                isRPSetChanged = TRUE;

                //Create a new entry
                RPListPtr = (RoutingPimSmRPList*)
                                MEM_malloc(sizeof(RoutingPimSmRPList));
                memset(RPListPtr, 0, sizeof(RoutingPimSmRPList));

                RPListPtr->RPAddress = getNodeAddressFromCharArray(
                           candidateRPPkt->encodedUnicastRP.unicastAddr);
                RPListPtr->holdtime = candidateRPPkt->holdtime;
                RPListPtr->priority = candidateRPPkt->priority;
                RPListPtr->grpPrefix = grpInfo.groupAddr;
                RPListPtr->maskLength = grpInfo.maskLength;

                //add the entry to the RP set
                ListInsert(node, pimDataSm->RPList, 0, (void*) RPListPtr);
                if (DEBUG)
                {
                    printf("Node %u: add RPADDR 0x%x in RPList from "
                           " C_RP Pkt\n",node->nodeId, RPListPtr->RPAddress);
                    printf("current list size = %d \n",
                           pimDataSm->RPList->size);
                }

                //Schedule a CGET timer
                delay = (clocktype) (candidateRPPkt->holdtime * SECOND);

                cgetTimerInfo.RPAddress = RPListPtr->RPAddress;
                cgetTimerInfo.grpPrefix = RPListPtr->grpPrefix;
                cgetTimerInfo.maskLength = RPListPtr->maskLength;

                cgetTimerInfo.seqNo = RPListPtr->bsrGrpToRPTimerSeq + 1;
                RPListPtr->bsrGrpToRPTimerSeq++;

                RoutingPimSetTimer(node,
                                   i,
                                   MSG_ROUTING_PimSmBSRGrpToRPTimeout,
                                   (void*) &cgetTimerInfo,
                                   delay);

                //clearing timerInfo for next entry
                memset(&cgetTimerInfo,
                       0,
                       sizeof(RoutingPimSmGrpToRPTimerInfo));
            }
            else   //If mapping already exists
            {
                if (candidateRPPkt->holdtime != 0)
                {
                    //Schedule a CGET timer
                    delay = (clocktype) (candidateRPPkt->holdtime * SECOND);

                    cgetTimerInfo.RPAddress = existingRPListPtr->RPAddress;
                    cgetTimerInfo.grpPrefix = existingRPListPtr->grpPrefix;
                    cgetTimerInfo.maskLength = existingRPListPtr->maskLength;

                    cgetTimerInfo.seqNo =
                        existingRPListPtr->bsrGrpToRPTimerSeq + 1;
                    existingRPListPtr->bsrGrpToRPTimerSeq++;

                    RoutingPimSetTimer(node,
                                       i,
                                       MSG_ROUTING_PimSmBSRGrpToRPTimeout,
                                       (void*) &cgetTimerInfo,
                                       delay);

                    //Update the holdtime and priorty in the existing mapping
                    existingRPListPtr->holdtime = candidateRPPkt->holdtime;
                    existingRPListPtr->priority = candidateRPPkt->priority;
                }
                else  //holdtime is 0
                {
                    if (DEBUG_BS)
                    {
                        printf("\nAs holdtime is zero in candidate RP pkt."
                           "\nHence,Remove Grp To RP Mapping From RP Set\n");
                    }
                    RoutingPimSmRemoveGrpToRPMappingFromRPSet(node,
                                                              rpListItem);
                }
            }//end of else

            numGroupInfo--;
        }//end of while

        if (isRPSetChanged == TRUE)
        {
            bsrTimerInfo.srcAddr = NetworkIpGetInterfaceAddress(node,
                                                                i);

            bsrTimerInfo.intfIndex = i;
            bsrTimerInfo.seqNo = thisInterface->bootstrapTimerSeq + 1;
            /* Note the current bootstrapTimerSeq */
            thisInterface->bootstrapTimerSeq++;

            delay =(clocktype)(ROUTING_PIMSM_BOOTSTRAP_MIN_INTERVAL_DEFAULT);

            RoutingPimSetTimer(node,
                               i,
                               MSG_ROUTING_PimSmBootstrapTimeOut,
                               (void*) &bsrTimerInfo,
                               delay);
            if (DEBUG_BS)
            {
                printf("\nInvoking RP change from HandleC-RP Packet\n");
            }
            RoutingPimSmCheckForRPChangeG(node);
            RoutingPimSmCheckForRPChangeSG(node);
        }

        break; //from for loop
    }//end of for ()
}

/*
*  FUNCTION    : RoutingPimSmCalculateBSRandOverride()
*  PURPOSE     : RoutingPimSm Calculate BS_RAND_OVERRIDE TIMER VALUE
*  RETURN VALUE: clocktype
*/
clocktype
RoutingPimSmCalculateBSRandOverride(Node* node,
                                    int interfaceIndex)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];
    clocktype bsRandOverride = 0;
    double priorityDelay = 0.0;
    double addrDelay = 0.0;
    unsigned char bestPriority = 0;
    unsigned char storedPriority = 0;
    unsigned char myPriority = 0;
    NodeAddress bestAddr;
    NodeAddress storedAddr;
    NodeAddress myAddr;

    storedPriority = thisInterface->currentBSRPriority;
    myPriority = thisInterface->bsrData.bsrPriority;
    storedAddr = thisInterface->currentBSR;
    myAddr = NetworkIpGetInterfaceAddress(node,
                                          interfaceIndex);

    bestPriority =
        (storedPriority > myPriority) ? storedPriority : myPriority;

    bestAddr = (storedAddr > myAddr) ? storedAddr : myAddr;

    priorityDelay =
              2 * ((double)((log((double)(1 + bestPriority - myPriority)))) /
              log((double)2));

    if (bestPriority == myPriority)
    {
        addrDelay =
      ((double)(log((double)(1 + bestAddr - myAddr))) / log((double)2)) / 16;
    }
    else
    {
        addrDelay = 2 - ( myAddr / pow(2.0, 31));
    }

    bsRandOverride = (clocktype)((5 + priorityDelay + addrDelay) * SECOND);

    if (DEBUG_BS)
    {
        char clockStr[100];
        ctoa(bsRandOverride,clockStr);
        printf ("\n\nBS_RAND_OVERRIDE Timer value is %s\n", clockStr);
    }

    return bsRandOverride;
}


/*
*  FUNCTION    : RoutingPimSmIsEmptyBSM ()
*  PURPOSE     : Check for empty BSM
*  RETURN VALUE: BOOL
*/
BOOL
RoutingPimSmIsEmptyBSM(Message* bootStrapMsg)
{
    if (bootStrapMsg->packetSize == sizeof(RoutingPimSmBootstrapPacket))
    {
        return TRUE;
    }

    return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmIsRPMappingPresent ()
*  PURPOSE     : Check for RP mapping present in the stored RP set
*  RETURN VALUE: pointer to RoutingPimSmRPList if mapping found else NULL
*/
ListItem*
RoutingPimSmIsRPMappingPresent(Node* node,
                               RoutingPimEncodedGroupAddr* grpInfo,
                               RoutingPimEncodedUnicastAddr* rpInfo)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    ListItem *rpListItem = NULL;
    RoutingPimSmRPList* rpListPtr = NULL;
    rpListItem = pimDataSm->RPList->first;

    while (rpListItem)
    {
        rpListPtr = (RoutingPimSmRPList*)(rpListItem->data);

        if ((rpListPtr->grpPrefix == grpInfo->groupAddr)
            &&
            (rpListPtr->maskLength == grpInfo->maskLength)
            &&
            (rpListPtr->RPAddress ==
                getNodeAddressFromCharArray(rpInfo->unicastAddr)))
        {
            return rpListItem;
        }

        rpListItem = rpListItem->next;
    }

    return NULL;
}

/*
*  FUNCTION    : RoutingPimSmFindGrpInBSM()
*  PURPOSE     : get the pointer from BSM matching the grpPrefix and
*                maskLength
*  RETURN VALUE: char pointer
*/
char*
RoutingPimSmFindGrpInBSM(Node* node,
                         Message* bootstrapMsg,
                         NodeAddress grpPrefix,
                         unsigned char maskLength)
{
    char* dataPtr = NULL;
    RoutingPimSmEncodedGroupBSRInfo* grpInfo = NULL;
    RoutingPimSmBootstrapPacket* bootstrapPkt =(RoutingPimSmBootstrapPacket*)
                                          MESSAGE_ReturnPacket(bootstrapMsg);

    if (DEBUG_BS)
    {
        printf("\n\nFind the following group in the BSM\n");
        printf("\nGroup Address = 0x%x\n",grpPrefix);
        printf("\nGroup Mask Length = %d\n",maskLength);

        printf("\nBSM TO BE SEARCHED IN\n");
        RoutingPimSmPrintBootStrapMessage(node,
                                          bootstrapMsg);
    }

    unsigned int bsmSize = bootstrapMsg->packetSize;

    dataPtr = (char*)(bootstrapPkt) + sizeof(RoutingPimSmBootstrapPacket);
    bsmSize = bsmSize - sizeof(RoutingPimSmBootstrapPacket);

    while (bsmSize > 0)
    {
        grpInfo = (RoutingPimSmEncodedGroupBSRInfo*)dataPtr;

        bsmSize = bsmSize
                  - sizeof(RoutingPimSmEncodedGroupBSRInfo)
                  - (sizeof(RoutingPimSmEncodedUnicastRPInfo) *
                  grpInfo->rpCount);

        if (bsmSize > 0)
        {
            dataPtr = dataPtr
                      + sizeof(RoutingPimSmEncodedGroupBSRInfo)
                      +(sizeof(RoutingPimSmEncodedUnicastRPInfo) *
                      grpInfo->rpCount);
        }

        if ((grpInfo->encodedGrpAddr.groupAddr == grpPrefix)
            &&
            (grpInfo->encodedGrpAddr.maskLength == maskLength))
        {
            return (char*)grpInfo;
        }
    }

    return NULL;
}

/*
*  FUNCTION    : RoutingPimSmRemoveGrpToRPMappingFromRPSet()
*  PURPOSE     : Remove Group to RP Mapping From stored RP Set
*  RETURN VALUE: char pointer
*/
void
RoutingPimSmRemoveGrpToRPMappingFromRPSet(Node* node,
                                          ListItem *rpListItem)
{
    PimData* pim = (PimData*)
         NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    RoutingPimSmTreeInformationBase* treeInfo = &pimDataSm->treeInfoBase;
    TreeInfoRowMap* rowPtrMap = treeInfo->rowPtrMap;
    TreeInfoRowMapIterator mapIterator;
    RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;
    NodeAddress grpMask;
    NodeAddress maskedGroupAddress;
    RoutingPimSmRPList* rpListPtr = (RoutingPimSmRPList*)(rpListItem->data);

    if (DEBUG_BS)
    {
        char clockStr[100];

        ctoa(node->getNodeTime(),clockStr);
        printf("\nNode %d : Removing the Group to RP Mapping from the RP set"
               " at %s.", node->nodeId,clockStr);

        printf("\nPriority   : %d",rpListPtr->priority);
        printf("\nHoldtime   : %d",rpListPtr->holdtime);
        printf("\nRPAddress  : 0x%x",rpListPtr->RPAddress);
        printf("\nGrp Prefix : 0x%x",rpListPtr->grpPrefix);
        printf("\nMaskLength : %d",rpListPtr->maskLength);
    }

    for (mapIterator = rowPtrMap->begin();
         mapIterator != rowPtrMap->end();
         mapIterator++)
    {
        rowPtr = mapIterator->second;

        grpMask = ConvertNumHostBitsToSubnetMask(32 - rpListPtr->maskLength);
        maskedGroupAddress = MaskIpAddress(rowPtr->grpAddr, grpMask);

        if ((rowPtr->RPointAddr == rpListPtr->RPAddress)
            &&
            (maskedGroupAddress == rpListPtr->grpPrefix))
        {
            rowPtr->nextHopForRP   = (unsigned int)NETWORK_UNREACHABLE;
            rowPtr->nextIntfForRP  = (unsigned int)NETWORK_UNREACHABLE;
            rowPtr->RPointAddr     = (unsigned int)NETWORK_UNREACHABLE;

            if (rowPtr->treeState == ROUTING_PIMSM_G)
            {
#ifdef ADDON_BOEINGFCS
                if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface)
                    && !(node->networkData.networkVar->iahepEnabled
                         && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                    && rowPtr->upstreamState == PimSm_JoinPrune_Join
                    && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
                {
                    if (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                        && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh)
                    {
                        int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                                rowPtr->grpAddr,
                                                                                rowPtr->upstream,
                                                                                rowPtr->upInterface);
                        if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                        {
                            printf("\nnode %d Decrement counter in RoutingPimSmRemoveGrpToRPMappingFromRPSet. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                               node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                        }
                        if (counter == 0)
                        {
                            MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr, rowPtr->upstream);
                        }
                    }
                }
#endif // ADDON_BOEINGFCS
                rowPtr->nextHopForSrc  = (unsigned int)NETWORK_UNREACHABLE;
                rowPtr->nextIntfForSrc = (unsigned int)NETWORK_UNREACHABLE;
                rowPtr->srcAddr        = (unsigned int)NETWORK_UNREACHABLE;
                rowPtr->upInterface    = (unsigned int)NETWORK_UNREACHABLE;
                rowPtr->upstream       = (unsigned int)NETWORK_UNREACHABLE;
            }
        }
    }

    ListGet(node,
            pimDataSm->RPList,
            rpListItem,
            TRUE,
            FALSE);//Not message
    if (DEBUG_BS)
    {
        printf("\nAfter RemoveGrpToRPMapping\n");
        RoutingPimSmPrintRPList(node);
    }
}

/*
*  FUNCTION    : RoutingPimSmRemoveUnreachableMappings()
*  PURPOSE     : Remove unreachable group to RP mappings from the
*                stored RP set
*  RETURN VALUE: NULL
*/
void
RoutingPimSmRemoveUnreachableMappings(Node* node,
                                      Message* bootstrapMsg)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    ListItem *rpListItem = NULL;
    ListItem *rpListItemNext = NULL;

    RoutingPimSmRPList* rpListPtr = NULL;
    rpListItem = pimDataSm->RPList->first;

    char* matchedGroupPtrInBSM = NULL;

    RoutingPimSmEncodedGroupBSRInfo* grpInfo = NULL;
    RoutingPimSmEncodedUnicastRPInfo* rpInfo = NULL;
    BOOL rpFound = FALSE;
    unsigned char rpCount;

    while (rpListItem)
    {
        rpListPtr = (RoutingPimSmRPList*)(rpListItem->data);

        matchedGroupPtrInBSM = RoutingPimSmFindGrpInBSM(node,
                                                      bootstrapMsg,
                                                      rpListPtr->grpPrefix,
                                                      rpListPtr->maskLength);

        if (matchedGroupPtrInBSM == NULL)
        {
            rpListItemNext = rpListItem->next;

            if (DEBUG_BS)
            {
                printf("\nIn RoutingPimSmRemoveUnreachableMappings():\n");
                printf("As Grp To RP mapping not found in BSM.\n");
                printf("Hence,Remove Grp To RP Mapping From RP Set.\n");
            }
            RoutingPimSmRemoveGrpToRPMappingFromRPSet(node,
                                                      rpListItem);

            rpListItem = rpListItemNext;
            continue; //consider next entry
        }
        else  //Group found,now search for RP
        {
            grpInfo = (RoutingPimSmEncodedGroupBSRInfo*)matchedGroupPtrInBSM;

            rpCount = grpInfo->rpCount;
            matchedGroupPtrInBSM = matchedGroupPtrInBSM
                                   + sizeof(RoutingPimSmEncodedGroupBSRInfo);

            while (rpCount)
            {
                rpInfo = (RoutingPimSmEncodedUnicastRPInfo*)
                                                matchedGroupPtrInBSM;

                if (getNodeAddressFromCharArray(
                                rpInfo->encodedUnicastRP.unicastAddr)
                    == rpListPtr->RPAddress)
                {
                    rpFound = TRUE;
                    break;
                }

                matchedGroupPtrInBSM = matchedGroupPtrInBSM
                                  + sizeof(RoutingPimSmEncodedUnicastRPInfo);

                rpCount--;
            }//end of inner while

            if (rpFound == FALSE)
            {
                if (DEBUG_BS)
                {
                    RoutingPimSmPrintRPList(node);
                    RoutingPimSmPrintBootStrapMessage(node,
                                                      bootstrapMsg);
                }

                rpListItemNext = rpListItem->next;

                if (DEBUG_BS)
                {
                    printf("\nIn RoutingPimSmRemoveUnreachableMappings()"
                        " :\n");
                    printf("As RP not found in BSM.\n");
                    printf("Hence,Remove Grp To RP Mapping From RP Set.\n");
                }

                RoutingPimSmRemoveGrpToRPMappingFromRPSet(node,
                                                          rpListItem);
                rpListItem = rpListItemNext;
                continue; //consider next entry
            }
        }//end of else

        rpListItem = rpListItem->next;
    }//end of outer while
    if (DEBUG_BS)
    {
        printf("From Unreachablemappings\n");
        RoutingPimSmPrintRPList(node);
    }
}

/*
*  FUNCTION    : RoutingPimSmStoreRPSet()
*  PURPOSE     : RoutingPimSm populate RP Set from the bootstrap
*                message
*  RETURN VALUE: NULL;
*/
void
RoutingPimSmStoreRPSet(Node* node,
                       int interfaceIndex,
                       Message* bootstrapMsg)
{
    PimData* pim = (PimData*)
       NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }
    char* dataPtr = NULL;
    RoutingPimSmEncodedGroupBSRInfo   * grpInfo = NULL;
    RoutingPimSmEncodedUnicastRPInfo  * RPInfo = NULL;
    RoutingPimSmRPList* RPListPtr = NULL;
    RoutingPimSmRPList* existingRPListPtr = NULL;
    ListItem* rpListItem = NULL;

    unsigned int bsmSize = 0;

    //It might happen that RefreshRPSet call StoreRPSet and till now
    //there is no last BSM stored
    if (bootstrapMsg == NULL)
    {
        return;
    }

    RoutingPimSmBootstrapPacket* bootstrapPkt =
        (RoutingPimSmBootstrapPacket*)
            MESSAGE_ReturnPacket(bootstrapMsg);

    clocktype delay = 0;
    int rpCount = 0;
    RoutingPimSmGrpToRPTimerInfo timerInfo;
    memset(&timerInfo, 0, sizeof(RoutingPimSmGrpToRPTimerInfo));

    //Check for empty BSM
    if (RoutingPimSmIsEmptyBSM(bootstrapMsg) == TRUE)
    {
        return;
    }

    bsmSize = bootstrapMsg->packetSize;

    //reaching the grpInfp part of the packet
    dataPtr = (char*)bootstrapPkt + sizeof(RoutingPimSmBootstrapPacket);
    bsmSize = bsmSize - sizeof(RoutingPimSmBootstrapPacket);
    if (DEBUG_BS)
    {
        printf("Node %d : \nBefore Storing the RP-Set is :\n",node->nodeId);
        RoutingPimSmPrintRPList(node);
    }
    //for each group
    while (bsmSize > 0)
    {
        grpInfo = (RoutingPimSmEncodedGroupBSRInfo*) (dataPtr);

        bsmSize = bsmSize - sizeof(RoutingPimSmEncodedGroupBSRInfo);
        if (bsmSize > 0)
        {
            dataPtr = dataPtr + sizeof(RoutingPimSmEncodedGroupBSRInfo);
        }

        //for each RP present for this group range
        rpCount = grpInfo->rpCount;
        while (rpCount)
        {
            RPInfo = (RoutingPimSmEncodedUnicastRPInfo*) (dataPtr);

            bsmSize = bsmSize - sizeof(RoutingPimSmEncodedUnicastRPInfo);
            if (bsmSize > 0)
            {
                dataPtr = dataPtr + sizeof(RoutingPimSmEncodedUnicastRPInfo);
            }

            rpListItem  = RoutingPimSmIsRPMappingPresent(node,
                                                  &grpInfo->encodedGrpAddr,
                                                  &RPInfo->encodedUnicastRP);

            if (rpListItem != NULL)
            {
                existingRPListPtr = (RoutingPimSmRPList*)(rpListItem->data);
            }
            else
            {
                existingRPListPtr = NULL;
            }

            //If mapping does not exist
            if (existingRPListPtr == NULL)
            {
               //Create a new entry
                RPListPtr = (RoutingPimSmRPList*)
                            MEM_malloc(sizeof(RoutingPimSmRPList));
                memset(RPListPtr, 0, sizeof(RoutingPimSmRPList));

                /* set the RP Info */
                RPListPtr->RPAddress = getNodeAddressFromCharArray(
                                       RPInfo->encodedUnicastRP.unicastAddr);
                RPListPtr->holdtime = RPInfo->RPHoldTime;
                RPListPtr->priority = RPInfo->RPPriority;
                RPListPtr->grpPrefix = grpInfo->encodedGrpAddr.groupAddr;
                RPListPtr->maskLength = grpInfo->encodedGrpAddr.maskLength;

                ListInsert(node, pimDataSm->RPList, 0, (void*) RPListPtr);

                if (DEBUG_BS)
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(),clockStr);
                    printf("Node %u: Add the following Group to RP mapping "
                           "in RP-Set at %s\n",
                           node->nodeId , clockStr);

                    printf("\nRP Address  = 0x%x",RPListPtr->RPAddress);
                    printf("\nHoldtime    = %d",RPListPtr->holdtime);
                    printf("\nPriority    = %d",RPListPtr->priority);
                    printf("\nGrp Prefix  = 0x%x",RPListPtr->grpPrefix);
                    printf("\nMask Length = %d",RPListPtr->maskLength);

                    printf("\nNow the list is : ");
                    RoutingPimSmPrintRPList(node);
                }

                //Schedule a GET timer
                delay = (clocktype) (RPInfo->RPHoldTime * SECOND);

                timerInfo.RPAddress = RPListPtr->RPAddress;
                timerInfo.grpPrefix = RPListPtr->grpPrefix;
                timerInfo.maskLength = RPListPtr->maskLength;

                timerInfo.seqNo = RPListPtr->routerGrpToRPTimerSeq + 1;
                RPListPtr->routerGrpToRPTimerSeq++;
                if (DEBUG_BS)
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);
                                            char clockStr1[100];
                        ctoa(delay, clockStr1);
                    printf("\nNode %d Starting "
                        "MSG_ROUTING_PimSmRouterGrpToRPTimeout"
                        " at time %s on interface %d with delay %s\n",
                        node->nodeId,
                        clockStr,interfaceIndex,clockStr1);
                }
                RoutingPimSetTimer(node,
                                   interfaceIndex,
                                   MSG_ROUTING_PimSmRouterGrpToRPTimeout,
                                   (void*) &timerInfo,
                                   delay);

                //clearing timerInfo for next entry
                memset(&timerInfo, 0, sizeof(RoutingPimSmGrpToRPTimerInfo));
            }//end of if
            else   //If mapping already exists
            {
                if (RPInfo->RPHoldTime != 0)
                {
                    //Schedule a GET timer
                    delay = (clocktype) (RPInfo->RPHoldTime * SECOND);

                    timerInfo.RPAddress = existingRPListPtr->RPAddress;
                    timerInfo.grpPrefix = existingRPListPtr->grpPrefix;
                    timerInfo.maskLength = existingRPListPtr->maskLength;

                    timerInfo.seqNo =
                        existingRPListPtr->routerGrpToRPTimerSeq + 1;
                    existingRPListPtr->routerGrpToRPTimerSeq++;
                    if (DEBUG_BS)
                    {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);
                        char clockStr1[100];
                        ctoa(delay, clockStr1);
                        printf("\nNode %d Starting "
                            "MSG_ROUTING_PimSmRouterGrpToRPTimeout"
                            " at time %s on interface %d with delay %s\n",
                            node->nodeId,
                            clockStr,interfaceIndex,clockStr1);
                    }
                    RoutingPimSetTimer(node,
                                       interfaceIndex,
                                       MSG_ROUTING_PimSmRouterGrpToRPTimeout,
                                       (void*) &timerInfo,
                                       delay);

                    //Update the holdtime and priorty in the existing mapping
                    existingRPListPtr->holdtime = RPInfo->RPHoldTime;
                    existingRPListPtr->priority = RPInfo->RPPriority;
                }
                else  //holdtime is 0
                {
                    if (DEBUG_BS)
                    {
                        printf("\nIn RoutingPimSmStoreRPSet():\n");
                        printf("As Holdtime is zero.\n");
                        printf("Hence,Remove Grp To RP Mapping From"
                               " RP Set.\n");
                    }

                    RoutingPimSmRemoveGrpToRPMappingFromRPSet(node,
                                                              rpListItem);
                }
            }//end of else
            rpCount--;
        }//end of inner while
    }//end of outer while

    RoutingPimSmRemoveUnreachableMappings(node,
                                          bootstrapMsg);

    if (DEBUG_BS)
    {
        printf("Node %d : \nAfter Storing the RP-Set is :\n",node->nodeId);
        RoutingPimSmPrintRPList(node);
    }

    //RFC:5059::Section 3.1.5
    //the entire BSM is stored for use in the action Refresh RP-Set
    //and to prime a new PIM neighbor as described below
    Message* bootstrapMsgCopy = MESSAGE_Duplicate(node,
                                                  bootstrapMsg);

    if (pimDataSm->lastBSMRcvdFromCurrentBSR != NULL)
    {
        MESSAGE_Free(node,pimDataSm->lastBSMRcvdFromCurrentBSR);
    }

    pimDataSm->lastBSMRcvdFromCurrentBSR = MESSAGE_Duplicate(node,
                                                           bootstrapMsgCopy);

    if (DEBUG_BS)
    {
        printf("\nIn RoutingPimSmStoreRPSet() : storing BSM received as last"
               "BSM received\n");
        RoutingPimSmPrintBootStrapMessage(node,
                                       pimDataSm->lastBSMRcvdFromCurrentBSR);
    }

    MESSAGE_Free(node, bootstrapMsgCopy);
    if (DEBUG_BS)
    {
        printf("\nInvoking RP change from Store RP set\n");
    }
    RoutingPimSmCheckForRPChangeG(node);
    RoutingPimSmCheckForRPChangeSG(node);
}

/*
*  FUNCTION    : RoutingPimSmCheckForRPChangeG()
*  PURPOSE     : This Function checks whether the RP changes or next hop to
                 RP changes for any existing (*,G) entry
*  RETURN VALUE: void
*/

void
RoutingPimSmCheckForRPChangeG(Node* node)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }
    RoutingPimSmTreeInformationBase* treeInfo = &pimDataSm->treeInfoBase;

    TreeInfoRowMap* rowPtrMap = treeInfo->rowPtrMap;
    TreeInfoRowMapIterator mapIterator;
    RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;

    NodeAddress RPOld = 0;
    NodeAddress RPNew = 0;
    int nextIntfForRPNew = 0;
    NodeAddress nextHopForRPNew = 0;
    clocktype delay = 0;

    RoutingPimSmJoinPruneTimerInfo joinTimer;
    memset(&joinTimer,0,sizeof(RoutingPimSmJoinPruneTimerInfo));

    if (treeInfo->numEntries == 0)
    {
        if (DEBUG_ERRORS)
        {
            printf("Node %u:no entry in tree info base \n",
                   node->nodeId);
        }
        return;
    }

    /* check for each (*,G) entry that RP changes and nexthop
    to RP changes*/
    for (mapIterator = rowPtrMap->begin();
         mapIterator != rowPtrMap->end();
         mapIterator++)
    {
        rowPtr = mapIterator->second;
        if (rowPtr->treeState == ROUTING_PIMSM_G)
        {
            RPOld = rowPtr->RPointAddr;
            RPNew = RoutingPimSmFindRPForThisGroup(
                        node,
                        rowPtr->grpAddr);

            if (RPOld != RPNew)
            {
                RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                            node,
                            RPNew,
                            &nextIntfForRPNew,
                            &nextHopForRPNew);

                if (nextHopForRPNew == 0)
                {
                    nextHopForRPNew = RPNew ;
                }
                else if (nextHopForRPNew ==
                                    (unsigned int)NETWORK_UNREACHABLE)
                {
                    RoutingPimSmGetNextHopOutInterface(node,
                                                       RPNew,
                                                       &nextHopForRPNew,
                                                       &nextIntfForRPNew);
               }

               if (nextIntfForRPNew == NETWORK_UNREACHABLE)
               {
                   if (DEBUG)
                   {
                       printf("Node %u has invalid out interface"
                              " towards 0x%x",node->nodeId,RPNew);
                   }
                   continue;//to for loop
               }

               if (rowPtr->nextHopForRP != nextHopForRPNew)
               {
                   if (DEBUG_RP_CHANGE)
                   {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);
                        printf("\nNode %u: Check RP Change for (*,G) at time"
                               " %s",
                               node->nodeId,clockStr);
                        printf("\nGroup Address = 0x%x",
                                rowPtr->grpAddr);
                        printf("\nOld RP Address = 0x%x", RPOld);
                        printf("\nNext Hop for old RP = 0x%x",
                               rowPtr->nextHopForRP);
                        printf("\nSending Prune(*,G) towards 0x%x",RPOld);

                        printf("\nNew RP Address = 0x%x", RPNew);
                        printf("\nNext Hop for new RP = 0x%x",
                               nextHopForRPNew);
                        printf("\nSending Join(*,G) towards 0x%x\n\n",RPNew);
                   }

                   /* Send (*, G)Prune Packet towards old RP*/
                   if (NetworkIpIsMyIP(
                                node,
                                rowPtr->RPointAddr) == FALSE)
                   {
                       RoutingPimSmSendJoinPrunePacket(
                               node,
                               rowPtr->srcAddr,
                               rowPtr->upstream,
                               rowPtr->grpAddr,
                               rowPtr->nextIntfForSrc,
                               ROUTING_PIMSM_G_JOIN_PRUNE,
                               ROUTING_PIM_PRUNE_TREE,
                               rowPtr);
                   }


                   /* Send (*, G)Join Packet towards new RP*/
                   RoutingPimSmSendJoinPrunePacket(
                               node,
                               RPNew,
                               nextHopForRPNew,
                               rowPtr->grpAddr,
                               nextIntfForRPNew,
                               ROUTING_PIMSM_G_JOIN_PRUNE,
                               ROUTING_PIM_JOIN_TREE,
                               rowPtr);

#ifdef ADDON_BOEINGFCS
                    if (!(rowPtr->upstream == nextHopForRPNew && rowPtr->upInterface == nextIntfForRPNew)
                        && !(node->networkData.networkVar->iahepEnabled
                             && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                        && rowPtr->upstreamState == PimSm_JoinPrune_Join
                        && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
                    {
                        int counter;
                        if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface) &&
                            (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                             && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh))
                        {
                            counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                                rowPtr->grpAddr,
                                                                                rowPtr->upstream,
                                                                                rowPtr->upInterface);
                            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                            {
                                printf("\nnode %d Decrement counter in RoutingPimSmCheckForRPChangeG. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                       node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                            }
                            if (counter == 0)
                            {
                                MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr, rowPtr->upstream);
                            }
                        }
                        if (MulticastCesRpimActiveOnInterface(node, nextIntfForRPNew) &&
                            (nextHopForRPNew != NETWORK_UNREACHABLE && nextIntfForRPNew != NETWORK_UNREACHABLE
                             && node->networkData.networkVar->interfaceInfo[nextIntfForRPNew]->useMiMulticastMesh))
                        {
                            counter = RoutingPimSmGroupMemStateIncrementCounter(node,
                                                                                rowPtr->grpAddr,
                                                                                nextHopForRPNew,
                                                                                nextIntfForRPNew);
                            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                            {
                                printf("\nnode %d Increment counter in RoutingPimSmCheckForRPChangeG. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                   node->nodeId, rowPtr->grpAddr, nextHopForRPNew, nextIntfForRPNew, counter);
                            }
                            if (counter == 1)
                            {
                                MiCesMulticastMeshInitiateMeshFormation(node, nextIntfForRPNew, rowPtr->grpAddr, nextHopForRPNew, pimDataSm->miMulticastMeshTimeout, FALSE);
                            }
                        }
                    }
#endif // ADDON_BOEINGFCS
                   rowPtr->nextHopForRP = nextHopForRPNew;
                   rowPtr->nextIntfForRP = nextIntfForRPNew;
                   rowPtr->nextHopForSrc = nextHopForRPNew;
                   rowPtr->nextIntfForSrc = nextIntfForRPNew;
                   rowPtr->srcAddr = RPNew;
                   rowPtr->upInterface = nextIntfForRPNew;
                   rowPtr->upstream = nextHopForRPNew;
                   rowPtr->RPointAddr = RPNew;

                   /* Routing PimSm Set Join Timer to t_periodic*/
                   joinTimer.srcAddr = rowPtr->srcAddr;
                   joinTimer.grpAddr = rowPtr->grpAddr;
                   joinTimer.intfIndex = rowPtr->upInterface;
                   joinTimer.seqNo = rowPtr->joinTimerSeq + 1;
                   joinTimer.treeState = rowPtr->treeState;

                   /* Note the latest joinTimerSeq in treeInfo Base */
                   rowPtr->joinTimerSeq++;
                   delay = (clocktype)
                       (pimDataSm->routingPimSmTPeriodicInterval);
                   rowPtr->lastJoinTimerEnd = node->getNodeTime() + delay;

                   RoutingPimSetTimer(node,
                                      rowPtr->upInterface,
                                      MSG_ROUTING_PimSmJoinTimerTimeout,
                                      (void*) &joinTimer,
                                      delay);
               }
               else
               {
                   // Check for case where RP hasnt changed, and neither has
                  // next hop for RP, but the nexthop interface may have changed
#ifdef ADDON_BOEINGFCS
                    if (!(rowPtr->upstream == nextHopForRPNew && rowPtr->upInterface == nextIntfForRPNew)
                        && !(node->networkData.networkVar->iahepEnabled
                             && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                        && rowPtr->upstreamState == PimSm_JoinPrune_Join
                        && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
                    {
                        int counter;
                        if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface) &&
                            (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                             && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh))
                        {
                            counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                                rowPtr->grpAddr,
                                                                                rowPtr->upstream,
                                                                                rowPtr->upInterface);
                            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                            {
                                printf("\nnode %d Decrement counter in RoutingPimSmCheckForRPChangeG. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                       node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                            }
                            if (counter == 0)
                            {
                                MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr, rowPtr->upstream);
                            }
                        }
                        if (MulticastCesRpimActiveOnInterface(node, nextIntfForRPNew) &&
                            (nextHopForRPNew != NETWORK_UNREACHABLE && nextIntfForRPNew != NETWORK_UNREACHABLE
                             && node->networkData.networkVar->interfaceInfo[nextIntfForRPNew]->useMiMulticastMesh))
                        {
                            counter = RoutingPimSmGroupMemStateIncrementCounter(node,
                                                                                rowPtr->grpAddr,
                                                                                nextHopForRPNew,
                                                                                nextIntfForRPNew);
                            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                            {
                                printf("\nnode %d Increment counter in RoutingPimSmCheckForRPChangeG. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                   node->nodeId, rowPtr->grpAddr, nextHopForRPNew, nextIntfForRPNew, counter);
                            }
                            if (counter == 1)
                            {
                                MiCesMulticastMeshInitiateMeshFormation(node, nextIntfForRPNew, rowPtr->grpAddr, nextHopForRPNew, pimDataSm->miMulticastMeshTimeout, FALSE);
                            }
                        }
                    }
#endif // ADDON_BOEINGFCS
                   rowPtr->nextHopForRP = nextHopForRPNew;
                   rowPtr->nextIntfForRP = nextIntfForRPNew;
                   rowPtr->nextHopForSrc = nextHopForRPNew;
                   rowPtr->nextIntfForSrc = nextIntfForRPNew;
                   rowPtr->srcAddr = RPNew;
                   rowPtr->upInterface = nextIntfForRPNew;
                   rowPtr->upstream = nextHopForRPNew;
                   rowPtr->RPointAddr = RPNew;
               }
            }
        }
    }
}

/*
*  FUNCTION    : RoutingPimSmCheckForRPChangeSG()
*  PURPOSE     : This Function checks whether the RP changes or next hop to
                 RP changes for any existing (S,G) entry
*  RETURN VALUE: None
*/

void
RoutingPimSmCheckForRPChangeSG(Node* node)
{
    PimData* pim = (PimData*)
    NetworkIpGetMulticastRoutingProtocol(node,MULTICAST_PROTOCOL_PIM);

    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }
    RoutingPimSmTreeInformationBase* treeInfo = &pimDataSm->treeInfoBase;

    TreeInfoRowMap* rowPtrMap = treeInfo->rowPtrMap;
    TreeInfoRowMapIterator mapIterator;
    RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;

    NodeAddress RPOld = 0;
    NodeAddress RPNew = 0;
    int nextIntfForRPNew = 0;
    NodeAddress nextHopForRPNew = 0;
    BOOL retVal = FALSE;

    RoutingPimSmJoinPruneTimerInfo joinTimer;
    memset(&joinTimer,0,sizeof(RoutingPimSmJoinPruneTimerInfo));

    if (treeInfo->numEntries == 0)
    {
        if (DEBUG_ERRORS)
        {
            printf("Node %u:no entry in tree info base \n",
                   node->nodeId);
        }
        return;
    }

    /* check for each (*,G) entry that RP changes and nexthop
    to RP changes*/
    for (mapIterator = rowPtrMap->begin();
         mapIterator != rowPtrMap->end();
         mapIterator++)
    {
        rowPtr = mapIterator->second;
        if (rowPtr->treeState == ROUTING_PIMSM_SG)
        {
            RPOld = rowPtr->RPointAddr;
            RPNew = RoutingPimSmFindRPForThisGroup(
                        node,
                        rowPtr->grpAddr);

            if (RPOld != RPNew)
            {
                RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                            node,
                            RPNew,
                            &nextIntfForRPNew,
                            &nextHopForRPNew);

                if (nextHopForRPNew == 0)
                {
                    nextHopForRPNew = RPNew ;
                }
                else if (nextHopForRPNew ==
                                    (unsigned int)NETWORK_UNREACHABLE)
                {
                    RoutingPimSmGetNextHopOutInterface(node,
                                                       RPNew,
                                                       &nextHopForRPNew,
                                                       &nextIntfForRPNew);
               }

               if (nextIntfForRPNew == NETWORK_UNREACHABLE)
               {
                   if (DEBUG)
                   {
                       printf("Node %u has invalid out interface towards 0x%x"
                                ,node->nodeId,RPNew);
                   }
                   continue;//to for loop
               }

               if (rowPtr->nextHopForRP != nextHopForRPNew)
               {
                   /* Send (*, G)Prune Packet towards old RP*/
                   if ((NetworkIpIsMyIP(
                                node,
                                rowPtr->RPointAddr) == TRUE)
                        &&
                        (rowPtr->upstreamState == PimSm_JoinPrune_Join))
                   {
#ifdef ADDON_BOEINGFCS
                        if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface)
                            && !(node->networkData.networkVar->iahepEnabled
                                 && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                            && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
                        {
                            if (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                                && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh)
                            {
                                int counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                                        rowPtr->grpAddr,
                                                                                        rowPtr->upstream,
                                                                                        rowPtr->upInterface);
                                if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                                {
                                    printf("\nnode %d Decrement counter in RoutingPimSmCheckForRPChangeSG. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                           node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                                }
                                if (counter == 0)
                                {
                                    MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr, rowPtr->upstream);
                                }
                            }
                        }
#endif // ADDON_BOEINGFCS
                       rowPtr->upstreamState = PimSm_JoinPrune_NotJoin;

                       if (DEBUG_RP_CHANGE)
                       {
                           char clockStr[100];
                           ctoa(node->getNodeTime(), clockStr);
                           printf("\nNode %u: Check RP Change For (S,G) at "
                                  "time %s",
                                  node->nodeId,clockStr);
                           printf("\nGroup Address = 0x%x",
                                   rowPtr->grpAddr);
                           printf("\nOld RP Address = 0x%x", RPOld);
                           printf("\nNext Hop for old RP = 0x%x",
                                   rowPtr->nextHopForRP);
                           printf("\nSending Prune(S,G) towards 0x%x",
                                    rowPtr->srcAddr);

                           printf("\nNew RP Address = 0x%x", RPNew);
                           printf("\nNext Hop for new RP = 0x%x",
                                   nextHopForRPNew);
                       }

                       /* Send PRUNE (S,G) upstream*/
                       RoutingPimSmSendJoinPrunePacket(node,
                                   rowPtr->srcAddr,
                                   rowPtr->nextHopForSrc,
                                   rowPtr->grpAddr,
                                   rowPtr->nextIntfForSrc,
                                   ROUTING_PIMSM_SG_JOIN_PRUNE,
                                   ROUTING_PIM_PRUNE_TREE,
                                   rowPtr);

                       /* cancel the last join timer */
                       /* Cancel the joinTimerSeq in treeInfo Base */
                       rowPtr->joinTimerSeq++;
                       /* Set SPTbit(S, G) False */
                       rowPtr->SPTbit = FALSE;

                       retVal = TRUE;
                   }

                   //treeInfo->rowPtr[i].nextHopForRP = nextHopForRPNew;
                   //treeInfo->rowPtr[i].nextIntfForRP = nextIntfForRPNew;
                   //treeInfo->rowPtr[i].srcAddr = RPNew;
                   //treeInfo->rowPtr[i].upInterface = nextIntfForRPNew;
                   //treeInfo->rowPtr[i].upstream = nextHopForRPNew;
                   rowPtr->RPointAddr = RPNew;
               }
           }
       }
    }
}

/*
*  FUNCTION    : RoutingPimSmRefreshRPSet()
*  PURPOSE     : Refresh RP Set
*  RETURN VALUE: NULL
*/
void
RoutingPimSmRefreshRPSet(Node* node,
                         int interfaceIndex)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }
    if (pimDataSm->lastBSMRcvdFromCurrentBSR != NULL)
    {
        if (DEBUG_BS)
        {
            char clockStr[100];
            ctoa(node->getNodeTime(),clockStr);
            printf("\nNode %d : Refreshing RP set at %s\n",
                    node->nodeId,
                    clockStr);

            printf("Last BSM received is the following :\n");
            RoutingPimSmPrintBootStrapMessage(node,
                                       pimDataSm->lastBSMRcvdFromCurrentBSR);
        }
        if (DEBUG_BS)
        {
            printf("\nInvoking storerpset from refershre set\n");
        }
        RoutingPimSmStoreRPSet(node,
                               interfaceIndex,
                               pimDataSm->lastBSMRcvdFromCurrentBSR);
    }
}

/* FUNCTION     :RoutingPimSmCBSRStateMachine()
*  PURPOSE      :Handle Candidate BSR state machine
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void
RoutingPimSmCBSRStateMachine(Node* node,
                             int interfaceIndex,
                             RoutingPimSmBSREvent eventType,
                             Message* bootstrapMsg)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    PimDataSm* pimSmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmBootstrapTimerInfo    timerInfo;
    clocktype delay;

    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];

    timerInfo.srcAddr = NetworkIpGetInterfaceAddress(node,
                                                     interfaceIndex);

    timerInfo.intfIndex = interfaceIndex;
    timerInfo.seqNo = thisInterface->bootstrapTimerSeq + 1;
    /* Note the current bootstrapTimerSeq */
    thisInterface->bootstrapTimerSeq++;

    switch (eventType)
    {
        case RCVD_PREF_BSM :
        {
            thisInterface->cBSRState = CANDIDATE_BSR;
            RoutingPimSmForwardBSM(node,
                                   bootstrapMsg,
                                   interfaceIndex);
            if (DEBUG_BS)
            {
                printf("\nInvoking storerpset after forwardbsm"
                    " in RCVD_PREF_BSM-CBSR\n");
            }
            RoutingPimSmStoreRPSet(node,
                                   interfaceIndex,
                                   bootstrapMsg);

            delay = (clocktype)
                    ((2 * pimSmData->routingPimSmBootstrapTimeout) +
                    (10 * SECOND));

            RoutingPimSetTimer(node,
                               interfaceIndex,
                               MSG_ROUTING_PimSmBootstrapTimeOut,
                               (void*) &timerInfo,
                               delay);
            break;
        }
        case BOOTSTRAP_TIMER_EXP :
        {
            if (thisInterface->cBSRState == CANDIDATE_BSR)   //CANDIDATE_BSR
            {
                thisInterface->cBSRState = PENDING_BSR;

                delay = (clocktype)(RoutingPimSmCalculateBSRandOverride(
                                        node,
                                        interfaceIndex));
            }
            else      //PENDING_BSR OR ELECTED_BSR
            {
                thisInterface->cBSRState = ELECTED_BSR;

                //if it is the elected BSR then it must store the current BSR
                //as its own address
                thisInterface->currentBSR =
                    NetworkIpGetInterfaceAddress(node,
                                                 interfaceIndex);
                thisInterface->currentBSRPriority =
                                thisInterface->bsrData.bsrPriority;

                RoutingPimSmOriginateBSM(node,
                                         interfaceIndex);

                delay = (clocktype)(pimSmData->routingPimSmBootstrapTimeout);
            }

            RoutingPimSetTimer(node,
                               interfaceIndex,
                               MSG_ROUTING_PimSmBootstrapTimeOut,
                               (void*) &timerInfo,
                               delay);
            break;
        }
        case RCVD_NON_PREF_BSM :
        {
            switch (thisInterface->cBSRState)
            {
                case CANDIDATE_BSR:
                {
                    thisInterface->cBSRState = PENDING_BSR;
                    RoutingPimSmForwardBSM(node,
                                           bootstrapMsg,
                                           interfaceIndex);


                    delay = (clocktype)(RoutingPimSmCalculateBSRandOverride(
                                            node,
                                            interfaceIndex));

                    RoutingPimSetTimer(node,
                                       interfaceIndex,
                                       MSG_ROUTING_PimSmBootstrapTimeOut,
                                       (void*) &timerInfo,
                                       delay);
                    break;
                }
                case PENDING_BSR:
                {
                    RoutingPimSmForwardBSM(node,
                                           bootstrapMsg,
                                           interfaceIndex);
                    break;
                }
                case ELECTED_BSR:
                {
                    RoutingPimSmOriginateBSM(node,
                                             interfaceIndex);

                    delay = (clocktype)
                            (pimSmData->routingPimSmBootstrapTimeout);

                    RoutingPimSetTimer(node,
                                       interfaceIndex,
                                       MSG_ROUTING_PimSmBootstrapTimeOut,
                                       (void*) &timerInfo,
                                       delay);
                    break;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

/*
*  FUNCTION    : RoutingPimSmNonCBSRStateMachine()
*  PURPOSE     : Handle Non CBSR State machine
*  RETURN VALUE: NULL
*/
void
RoutingPimSmNonCBSRStateMachine(Node* node,
                                int interfaceIndex,
                                RoutingPimSmBSREvent eventType,
                                Message* bootstrapMsg)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    PimDataSm* pimSmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmBootstrapTimerInfo    timerInfo;
    clocktype delay = 0;

    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];

    timerInfo.srcAddr = NetworkIpGetInterfaceAddress(node,
                                                     interfaceIndex);

    timerInfo.intfIndex = interfaceIndex;
    timerInfo.seqNo = thisInterface->bootstrapTimerSeq + 1;
    /* Note the current bootystrapTimerSeq */
    thisInterface->bootstrapTimerSeq++;

    switch (eventType)
    {
        case RCVD_PREF_BSM :
        {
            if (thisInterface->ncBSRState == ACCEPT_PREFERRED)
            {
                //remain in accept preferred
                RoutingPimSmForwardBSM(node,
                                       bootstrapMsg,
                                       interfaceIndex);
                if (DEBUG_BS)
                {
                    printf("\nInvoking storerpset after forwardbsm"
                           " in RCVD_PREF_BSM-nCBSR\n");
                }
                RoutingPimSmStoreRPSet(node,
                                       interfaceIndex,
                                       bootstrapMsg);

                delay = (clocktype)
                        ((2 * pimSmData->routingPimSmBootstrapTimeout)
                        + (10 * SECOND));

                RoutingPimSetTimer(node,
                                   interfaceIndex,
                                   MSG_ROUTING_PimSmBootstrapTimeOut,
                                   (void*) &timerInfo,
                                   delay);
            }
            break;
        }
        case BOOTSTRAP_TIMER_EXP :
        {
            if (thisInterface->ncBSRState == ACCEPT_PREFERRED)
            {
                thisInterface->ncBSRState = ACCEPT_ANY;
                RoutingPimSmRefreshRPSet(node,
                                         interfaceIndex);
                RoutingPimSmRemoveBSRState(node,
                                           interfaceIndex);

                /*
                RFC:5059::Section 3.1.2
                No SZT is maintained.Hence, the event "Scope-Zone
                Expiry Timer Expires" does not exist and no actions
                with regard to this timer are executed.
                */

                //not setting the SZT to SZ_Timeout
            }
            break;
        }
        case RCVD_NON_PREF_BSM :
        {
            //remain in accept-preferred
            break;
        }
        case RCVD_BSM :
        {
            if ((thisInterface->ncBSRState == NO_INFO)
                ||
                (thisInterface->ncBSRState == ACCEPT_ANY))
            {
                thisInterface->ncBSRState = ACCEPT_PREFERRED;
                RoutingPimSmForwardBSM(node,
                                       bootstrapMsg,
                                       interfaceIndex);
            if (DEBUG_BS)
            {
                printf("\nInvoking storerpset after forwardbsm in "
                       "RCVD_BSM-CBSR\n");
            }
                RoutingPimSmStoreRPSet(node,
                                       interfaceIndex,
                                       bootstrapMsg);

                delay = (clocktype)
                        ((2 * pimSmData->routingPimSmBootstrapTimeout)
                        + (10 * SECOND));

                RoutingPimSetTimer(node,
                                   interfaceIndex,
                                   MSG_ROUTING_PimSmBootstrapTimeOut,
                                   (void*) &timerInfo,
                                   delay);
            }
            break;
        }
    }
}

/*
*  FUNCTION    : RoutingPimSmRemoveBSRState()
*  PURPOSE     : Remove BSR state
*  RETURN VALUE: NULL
*/
void
RoutingPimSmRemoveBSRState(Node* node,
                           int interfaceIndex)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    PimDataSm* pimSmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];

    thisInterface->currentBSR = (NodeAddress)NETWORK_UNREACHABLE;
    thisInterface->currentBSRPriority = 0;
    if (pimSmData->lastBSMRcvdFromCurrentBSR != NULL)
    {
        MESSAGE_Free(node, pimSmData->lastBSMRcvdFromCurrentBSR);
        pimSmData->lastBSMRcvdFromCurrentBSR = NULL;
    }
}

/*
*  FUNCTION    : RoutingPimSmIsBSMPreferred()
*  PURPOSE     : Check if the received BSM is preferred or not
*  RETURN VALUE: BOOL
*/
BOOL
RoutingPimSmIsBSMPreferred(Node* node,
                           unsigned char pktBSRPriority,
                           NodeAddress pktBSRAddr,
                           int interfaceIndex)
{
    PimData* pim = (PimData*)
         NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    RoutingPimInterface* thisInterface = &pim->interface[interfaceIndex];
    unsigned char currentBSRPriority = thisInterface->currentBSRPriority;
    NodeAddress currentBSRAddr = 0;

    currentBSRAddr = thisInterface->currentBSR;

    if (pktBSRPriority > currentBSRPriority)
    {
        return TRUE;
    }

    if (pktBSRPriority < currentBSRPriority)
    {
        return FALSE;
    }

   if (pktBSRAddr >= currentBSRAddr)
   {
       return TRUE;
   }

   return FALSE;
}

/*
*  FUNCTION    : RoutingPimSmDoBSMProcessingChecks()
*  PURPOSE     : Do the processing checks for the received BSM
*  RETURN VALUE: BOOL
*/
BOOL
RoutingPimSmDoBSMProcessingChecks(Node* node,
                                  NodeAddress bsmSrcAddr,
                                  int interfaceIndex,
                                  Message* bootstrapMsg)
{
    PimData* pim = (PimData*)
         NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    RoutingPimSmBootstrapPacket* bootstrapPkt =
        (RoutingPimSmBootstrapPacket*) MESSAGE_ReturnPacket(bootstrapMsg);

    RoutingPimInterface* thisInterface = NULL;
    RoutingPimNeighborListItem* neighbor = NULL;
    NodeAddress rpfNeighborBSMSrc = 0;
    NodeAddress rpfNeighborBSR = 0;
    NodeAddress bsrIPAddress = 0;

    BOOL forwardForRpim = FALSE;
    int outInterface = 0;

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        bsmSrcAddr,
                                                        &outInterface,
                                                        &rpfNeighborBSMSrc);

    thisInterface = &pim->interface[interfaceIndex];

    /* Search neighbor in neighbor list */
    if (thisInterface->smInterface->neighborList->size > 0)
    {
        RoutingPimSearchNeighborList(thisInterface->smInterface->neighborList,
                                     bsmSrcAddr,
                                     &neighbor);
    }
    if (((!(rpfNeighborBSMSrc == NEXT_HOP_ME
        || bsmSrcAddr == rpfNeighborBSMSrc)))
        ||
        (neighbor == NULL)
        )
    {
        //drop the packet silently
        /*printf("Node %d: Failed neighbor! %d && %d\n",
                node->nodeId, directlyConnected, neighbor);*/
        return FALSE;
    }

    //Qualnet is not supporting unicast BSM so assuming
    //the dest address would be ALL_PIM_ROUTERS
    bsrIPAddress = getNodeAddressFromCharArray(
                    bootstrapPkt->encodedUnicastBSR.unicastAddr);

    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        bsrIPAddress,
                                                        &outInterface,
                                                        &rpfNeighborBSR);
    if (RoutingPimGetNoForwardCommonHeader(bootstrapPkt->commonHeader)
            == FALSE)
    {
        if ((rpfNeighborBSR != NEXT_HOP_ME)
            &&
            (bsmSrcAddr != rpfNeighborBSR))
        {
            //drop the packet silently
            return FALSE;
        }
    }

    return TRUE;
}


/*
*  FUNCTION    : RoutingPimSmHandleBootstrapPacket()
*  PURPOSE     : Check if the received Bootstrap packet
*                should be further processed
*  RETURN VALUE: NULL;
*/
void
RoutingPimSmHandleBootstrapPacket(Node* node,
                                  Message* bootStrapMsg,
                                  NodeAddress srcAddr,
                                  int incomingIntf)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimSmData = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }
    RoutingPimSmBootstrapPacket* bootstrapPkt =
        (RoutingPimSmBootstrapPacket*) MESSAGE_ReturnPacket(bootStrapMsg);

    RoutingPimInterface* thisInterface = &pim->interface[incomingIntf];

    RoutingPimSmCandidateRPTimerInfo rpTimerInfo;
    memset(&rpTimerInfo, 0, sizeof(RoutingPimSmCandidateRPTimerInfo));

    clocktype delay = 0;

    BOOL isPreferredBSM = FALSE;

    NodeAddress prevHop = (unsigned int)NETWORK_UNREACHABLE;
    int prevIntf = 0;

    if (DEBUG_BS)
    {
       char clockStr[100];
       ctoa(node->getNodeTime(),clockStr);
       printf("Node %d received Bootstrap Packet from BSR Addr"
              " 0x%x at %s\n", node->nodeId,
                             getNodeAddressFromCharArray(
                             bootstrapPkt->encodedUnicastBSR.unicastAddr),
                             clockStr);

       RoutingPimSmPrintBootStrapMessage(node,
                                         bootStrapMsg);
    }

/*
// We may need to update this code since entire code structure
// has changed! Will need to do testing...
#ifdef ADDON_BOEINGFCS
    if (MulticastCesRpimActiveOnInterface(node, incomingIntf))
    {
        // process bootstrap packet regardless... there is important
        // information in these packets.
        RoutingPimSmProcessBootstrapMsg(node, msg, srcAddr, incomingIntf);
        return;
        // information in these packets.
    }
#endif
*/

    if (RoutingPimSmDoBSMProcessingChecks(node,
                                          srcAddr,
                                          incomingIntf,
                                          bootStrapMsg) == FALSE)
    {
        //Discard the BSM
        if (DEBUG_BS)
        {
            char clockStr[100];
            ctoa(node->getNodeTime(),clockStr);
            printf("\nNode %d : BootStrap processing checks failed at %s"
                   "\nHence,Discarding the Bootstrap message.\n",node->nodeId,
                   clockStr);
        }
        return;
    }

    //RFC:5059::Section 3.2
    if (thisInterface->RPFlag == TRUE) //Its a C-RP
    {
        if (RoutingPimSmIsEmptyBSM(bootStrapMsg) == TRUE)
        {
            if (DEBUG_BS)
            {
                printf("\nNode %d : Empty Bootstrap message is received.\n",
                        node->nodeId);
            }
            //As C-RP has received the empty BSM i.e. it will assume that
            //the BSR election has taken place

            /*
             if the received BSM dont have the higher IP address and priority
             as compared to the stored information then no need to update the
             current as it may happen that its own other interface which is a
             BSR itself and has the higher IP address.
             Also,not calling the isPreferredBSM for this as it might happen
             that this interface is in PENDING_BSR so it will use its own IP
             address for comparison
            */

            if (thisInterface->currentBSRPriority <
                        bootstrapPkt->BSRPriority)
            {
                thisInterface->currentBSR = getNodeAddressFromCharArray(
                                bootstrapPkt->encodedUnicastBSR.unicastAddr);
                thisInterface->currentBSRPriority =
                                  (unsigned char)(bootstrapPkt->BSRPriority);
            }
            else if (thisInterface->currentBSRPriority ==
                     bootstrapPkt->BSRPriority)
            {
                if (thisInterface->currentBSR < getNodeAddressFromCharArray(
                                bootstrapPkt->encodedUnicastBSR.unicastAddr))
                {
                    thisInterface->currentBSR = getNodeAddressFromCharArray(
                                bootstrapPkt->encodedUnicastBSR.unicastAddr);
                    thisInterface->currentBSRPriority =
                                  (unsigned char)(bootstrapPkt->BSRPriority);
                }
            }

            //this is the first C-RP-Adv
            RoutingPimSmSendCandidateRPPacket(node,
                                              incomingIntf);

            //Schedule a timer to C_RP_ADV_BACKOFF
            rpTimerInfo.srcAddr = srcAddr;
            rpTimerInfo.intfIndex = incomingIntf;
            rpTimerInfo.seqNo = thisInterface->candidateRPTimerSeq + 1;
            thisInterface->candidateRPTimerSeq++;
            thisInterface->backOffCounter = 2;

            delay = (clocktype) (RANDOM_nrand(pimSmData->seed)
                                 % ROUTING_PIMSM_CANDIDATE_RP_ADV_BACKOFF);

            RoutingPimSetTimer(node,
                               incomingIntf,
                               MSG_ROUTING_PimSmCandidateRPTimeOut,
                               (void*) &rpTimerInfo,
                               delay);
        }
    }

    isPreferredBSM = RoutingPimSmIsBSMPreferred(
                        node,
                        bootstrapPkt->BSRPriority,
                        getNodeAddressFromCharArray(
                        bootstrapPkt->encodedUnicastBSR.unicastAddr),
                        incomingIntf);

    if (isPreferredBSM == TRUE)
    {
        if (DEBUG_BS)
        {
            printf("\nNode %d : Preferred Bootstrap message is received.\n",
                    node->nodeId);
        }
        /* the receiving router stores the BSR address and priority */
        thisInterface->currentBSR = getNodeAddressFromCharArray(
                                bootstrapPkt->encodedUnicastBSR.unicastAddr);
        thisInterface->currentBSRPriority =
                               (unsigned char)(bootstrapPkt->BSRPriority);
    }

    /* find the actual prevHop & interface to reach BSR */
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                   thisInterface->currentBSR,
                                                   &prevIntf,
                                                   &prevHop);

    if (prevHop == 0)
    {
        prevHop = thisInterface->currentBSR ;
    }

    if (DEBUG_BS)
    {
        printf("\n\nIt has BSR Addr 0x%x \n", thisInterface->currentBSR);
        printf("prevHop = %x \n", prevHop);
        printf("prevIntf = %d \n", prevIntf);
        printf("incomingIntf = %d \n", incomingIntf);
        NetworkPrintForwardingTable(node);
    }

    BOOL sameInt = TRUE;

    if (prevIntf != incomingIntf)
    {
        sameInt = FALSE;
    }

    if (!sameInt)
    {
        if (DEBUG_BS) {
            printf("Node %u:get pkt form intf %d instead of BSR-Intf %d \n",
                   node->nodeId, incomingIntf, prevIntf);
            printf("No need to Forward this packet  \n");
        }
        return;
    }

    if (thisInterface->BSRFlg)   //For BSR
    {
        //Check for Preferred BSM
        if (isPreferredBSM == TRUE)
        {
            if (DEBUG_BS)
            {
                printf("\nNode %d : Calling Candidate BSR State Machine "
                       "for RCVD_PREF_BSM\n",node->nodeId);
            }

            RoutingPimSmCBSRStateMachine(node,
                                         incomingIntf,
                                         RCVD_PREF_BSM,
                                         bootStrapMsg);
        }
        else  //non-preferred BSM
        {
            if (DEBUG_BS)
            {
                printf("\nNode %d : Calling Candidate BSR State Machine "
                       "for RCVD_NON_PREF_BSM\n",node->nodeId);
            }
            RoutingPimSmCBSRStateMachine(node,
                                            incomingIntf,
                                            RCVD_NON_PREF_BSM,
                                            bootStrapMsg);
        }
    }
    else   //For non-BSR
    {
        if ((thisInterface->ncBSRState == NO_INFO)
            ||
            (thisInterface->ncBSRState == ACCEPT_ANY))
        {
            if (DEBUG_BS)
            {
                printf("\nNode %d : Calling Non-Candidate BSR State Machine "
                       "for RCVD_BSM\n",node->nodeId);
            }

            RoutingPimSmNonCBSRStateMachine(node,
                                            incomingIntf,
                                            RCVD_BSM,
                                            bootStrapMsg);
        }
        else
        {
            //Check for Preferred BSM
            if (isPreferredBSM == TRUE)
            {
                if (DEBUG_BS)
                {
                    printf("\nNode %d : Calling Non-Candidate BSR State "
                        "Machine for RCVD_PREF_BSM\n",node->nodeId);
                }

                RoutingPimSmNonCBSRStateMachine(node,
                                             incomingIntf,
                                             RCVD_PREF_BSM,
                                             bootStrapMsg);
            }
            else  //non-preferred BSM
            {
                if (DEBUG_BS)
                {
                    printf("\nNode %d : Calling Non-Candidate BSR State "
                        "Machine for RCVD_NON_PREF_BSM\n",node->nodeId);
                }

                RoutingPimSmNonCBSRStateMachine(node,
                                                incomingIntf,
                                                RCVD_NON_PREF_BSM,
                                                bootStrapMsg);
            }
        }
    }
}



/*
*  FUNCTION    : RoutingPimSmProcessUnicastRouteChange()
*  PURPOSE     : Routing PimSm Process Unicast Route Change
*  RETURN VALUE: NULL
*/

void
RoutingPimSmProcessUnicastRouteChange(Node* node,
                                      NodeAddress destAddr,
                                      NodeAddress destAddrMask,
                                      NodeAddress nextHop,
                                      int interfaceId,
                                      int metric,
                              NetworkRoutingAdminDistanceType adminDistance)

{
    PimData* pim = (PimData*)
                   NetworkIpGetMulticastRoutingProtocol(node,
                                                     MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }
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
    int nextIntfForDest = 0;
    NodeAddress nextHopForDest = 0;
    BOOL IAmRP = FALSE;

    if (treeInfo->numEntries == 0)
    {
        if (DEBUG_ERRORS) {
            printf("Node %u:no entry in tree info base \n",
                   node->nodeId);
        }
        return;
    }

    if (DEBUG_JP)
    {
        char clockStr[100];
        ctoa(node->getNodeTime(), clockStr);
        printf("Node %u: receieve root change information for intf %d\n",
               node->nodeId, interfaceId);
        printf("for dest %x \n", destAddr);
        printf("doing modification at %s \n", clockStr);
    }

    /* check for sending join prune packet */
    for (mapIterator = rowPtrMap->begin();
         mapIterator != rowPtrMap->end();
         mapIterator++)
    {
        rowPtr = mapIterator->second;
        groupAddr = rowPtr->grpAddr;

        RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
            rowPtr->srcAddr,
            &nextIntfForDest, &nextHopForDest);

        if (nextHopForDest == 0)
        {
            nextHopForDest = rowPtr->RPointAddr;
            nextIntfForDest = NetworkIpGetInterfaceIndexForNextHop(
                              node,
                              nextHopForDest);
        }

        if (nextHopForDest == (unsigned)NETWORK_UNREACHABLE)
        {
            if (DEBUG) {
                printf(" Node %u: inform RP unreachable \n",node->nodeId);
                printf("Hence no further packet transfer \n");
            }

#ifdef ADDON_DB
            //remove this entry from multicast_connectivity cache
            StatsDBConnTable::MulticastConnectivity stats;

            stats.nodeId = node->nodeId;
            stats.destAddr = rowPtr->grpAddr;

            stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                rowPtr->srcAddr);
            stats.upstreamNeighborId =
                              MAPPING_GetNodeIdFromInterfaceAddress(node,
                                               rowPtr->upstream);
            stats.upstreamInterface = rowPtr->upInterface;

            if (rowPtr->treeState == ROUTING_PIMSM_G)
            {
                strcpy(stats.rootNodeType,"RP");

                ListItem* listItem = rowPtr->downstream->first;

                while (listItem)
                {
                    RoutingPimSmDownstream* downstream;
                    downstream = (RoutingPimSmDownstream*) listItem->data;

                    stats.outgoingInterface = downstream->interfaceId;

                    // delete entry from  multicast_connectivity cache
                    STATSDB_HandleMulticastConnDeletion(node, stats);

                    listItem = listItem->next;
                }
            }
            else if (rowPtr->treeState == ROUTING_PIMSM_SG)
            {
                strcpy(stats.rootNodeType,"Source");

                /*check if it is the RP */
                for (int j = 0; j < node->numberInterfaces; j++)
                {
                    if (rowPtr->RPointAddr == pim->interface[j].ipAddress)
                    {
                        IAmRP  = TRUE;
                        break ;
                    }
                }

                if (IAmRP)
                {
                    stats.outgoingInterface = ANY_DEST;
                    // delete entry from  multicast_connectivity cache
                    STATSDB_HandleMulticastConnDeletion(node, stats);
                }
                else
                {
                    ListItem* listItem = rowPtr->downstream->first;

                    while (listItem)
                    {
                        RoutingPimSmDownstream* downstream;
                        downstream = (RoutingPimSmDownstream*)listItem->data;

                        stats.outgoingInterface = downstream->interfaceId;

                        // delete entry from  multicast_connectivity cache
                        STATSDB_HandleMulticastConnDeletion(node, stats);

                        listItem = listItem->next;
                    }
                }
            }
#endif
        }

        if (((rowPtr->srcAddr & destAddrMask) == destAddr)
                && (rowPtr->upstream != nextHop &&
                    rowPtr->srcAddr !=
                    rowPtr->upstream) &&
                (rowPtr->treeState == ROUTING_PIMSM_G ||
                 rowPtr->treeState == ROUTING_PIMSM_SG))
        {
            if (nextHop == (unsigned)NETWORK_UNREACHABLE
#ifdef ADDON_BOEINGFCS
                    || (MulticastCesRpimActiveOnInterface(node, interfaceId)
                        && nextHop == 0)
#endif
               )
            {
                if (DEBUG) {
                    printf("Node %u: no nextHop to reach destination \n",
                           node->nodeId);
                }
                return;
            }

            if (DEBUG_JP)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);

                printf("doing modification at %s \n", clockStr);
                printf(" new route change \n");
                printf("asociated srcAddr = 0x%x \n",
                       rowPtr->srcAddr);
                printf("old upstream %x\n", rowPtr->upstream);
                printf("old nextIntf %u\n", rowPtr->nextIntfForSrc);
                printf(" new interfaceId %d \n", interfaceId);
            }

            if (rowPtr->upstreamState == PimSm_JoinPrune_Join)
            {
                RoutingPimSmJoinPruneTimerInfo    joinTimer;
                clocktype delay;

                if (DEBUG_JP) {
                    printf("associated upstream state is join \n");
                }

                /* set the join prune Type */
                if (rowPtr->treeState == ROUTING_PIMSM_G)
                {
                    joinPruneType = ROUTING_PIMSM_G_JOIN_PRUNE;
                }
                else if (rowPtr->treeState == ROUTING_PIMSM_SG)
                {
                    joinPruneType = ROUTING_PIMSM_SG_JOIN_PRUNE;
                }

                if (DEBUG_JP) {
                    printf("Node %u :\n", node->nodeId);
                    printf(" send prune to old next Hop through intf %d\n",
                           rowPtr->nextIntfForRP);
                    printf(" send Join to new next Hop through intf %d\n",
                           interfaceId);
                }

                RPAddr = RoutingPimSmFindRPForThisGroup(node, groupAddr);
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

                if (joinPruneType == ROUTING_PIMSM_SG_JOIN_PRUNE &&
                    sgRptRowPtr != NULL &&
                    sgRptRowPtr->upstreamState == PimSm_SGrpt_Pruned &&
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

#ifdef ADDON_DB
                //found new and better next hop so update upstream info
                if (rowPtr->upstream != nextHop)
                {
                    //update this entry in multicast_connectivity cache
                    IAmRP = FALSE;

                    StatsDBConnTable::MulticastConnectivity stats;

                    stats.nodeId = node->nodeId;
                    stats.destAddr = rowPtr->grpAddr;

                    stats.rootNodeId =
                                MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                rowPtr->srcAddr);
                    stats.upstreamNeighborId =
                              MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                    nextHop);
                    stats.upstreamInterface = interfaceId;

                    if (rowPtr->treeState == ROUTING_PIMSM_G)
                    {
                        strcpy(stats.rootNodeType,"RP");

                        ListItem* listItem =
                                       rowPtr->downstream->first;

                        while (listItem)
                        {
                            RoutingPimSmDownstream* downstream;
                            downstream = (RoutingPimSmDownstream*)
                                                              listItem->data;

                            stats.outgoingInterface= downstream->interfaceId;

                            // update entry in  multicast_connectivity cache
                            STATSDB_HandleMulticastConnUpdateUpstreamInfo(
                                                          node, stats, TRUE);

                            listItem = listItem->next;
                        }
                    }
                    else if (rowPtr->treeState == ROUTING_PIMSM_SG)
                    {
                        strcpy(stats.rootNodeType,"Source");

                        /*check if it is the RP */
                        for (int j = 0; j < node->numberInterfaces; j++)
                        {
                            if (rowPtr->RPointAddr ==
                                                 pim->interface[j].ipAddress)
                            {
                                IAmRP  = TRUE;
                                break ;
                            }
                        }

                        if (IAmRP)
                        {
                            stats.outgoingInterface = ANY_DEST;
                            // update entry in  multicast_connectivity cache
                            STATSDB_HandleMulticastConnUpdateUpstreamInfo(
                                                          node, stats, TRUE);
                        }
                        else
                        {
                            ListItem* listItem = rowPtr->downstream->first;

                            while (listItem)
                            {
                                RoutingPimSmDownstream* downstream;
                                downstream = (RoutingPimSmDownstream*)
                                                              listItem->data;

                                stats.outgoingInterface =
                                                     downstream->interfaceId;

                               //update entry in multicast_connectivity cache
                               STATSDB_HandleMulticastConnUpdateUpstreamInfo(
                                                           node,stats, TRUE);

                                listItem = listItem->next;
                            }
                        }
                    }
                }
#endif
#ifdef ADDON_BOEINGFCS
                    if (!(node->networkData.networkVar->iahepEnabled
                             && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                        && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
                    {
                        int counter;
                        if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface) &&
                            (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                             && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh))
                        {
                            counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                                rowPtr->grpAddr,
                                                                                rowPtr->upstream,
                                                                                rowPtr->upInterface);
                            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                            {
                                printf("\nnode %d Decrement counter in RoutingPimSmProcessUnicastRouteChange (upstream router change). grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                       node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                            }
                            if (counter == 0)
                            {
                                MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr, rowPtr->upstream);
                            }
                        }
                        if (MulticastCesRpimActiveOnInterface(node, interfaceId) &&
                            (nextHop != NETWORK_UNREACHABLE && interfaceId != NETWORK_UNREACHABLE
                             && node->networkData.networkVar->interfaceInfo[interfaceId]->useMiMulticastMesh))
                        {
                            counter = RoutingPimSmGroupMemStateIncrementCounter(node,
                                                                                rowPtr->grpAddr,
                                                                                nextHop,
                                                                                interfaceId);
                            if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                            {
                                printf("\nnode %d Increment counter in RoutingPimSmProcessUnicastRouteChange (upstream router change). grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                   node->nodeId, rowPtr->grpAddr, nextHop, interfaceId, counter);
                            }
                            if (counter == 1)
                            {
                                MiCesMulticastMeshInitiateMeshFormation(node, interfaceId, rowPtr->grpAddr, nextHop, pimDataSm->miMulticastMeshTimeout, FALSE);
                            }
                        }
                    }
#endif // ADDON_BOEINGFCS
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
            else if (((rowPtr->upstreamState == PimSm_JoinPrune_NotJoin) &&
                     (rowPtr->treeState == ROUTING_PIMSM_G))
                      ||
                      ((pim->interface[interfaceId].switchDirectToSPT == TRUE) &&
                      (rowPtr->upstreamState == PimSm_JoinPrune_NotJoin) &&
                      (rowPtr->treeState == ROUTING_PIMSM_SG)))
            {
                RoutingPimSmHandleUpstreamStateMachine(node,
                                           rowPtr->srcAddr,
                                           rowPtr->nextIntfForSrc,
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
                if ((rowPtr->srcAddr & destAddrMask) == destAddr &&
                     downStreamListPtr->assertState ==
                     PimSm_Assert_ILostAssert)
                {
                    if (rowPtr->nextIntfForSrc != interfaceId)
                    {
                        downStreamListPtr->assertState = PimSm_Assert_NoInfo;
                        RoutingPimSmPerformActionA5(
                                            node,
                                            rowPtr->grpAddr,
                                            rowPtr->srcAddr,
                                            rowPtr->nextIntfForSrc,
                                            rowPtr->treeState);
                    }
                }
            }
        }//end of if
#ifdef ADDON_BOEINGFCS
        // upstream router stays same but upstream interface changes
        else if ((rowPtr->srcAddr & destAddrMask) == destAddr
                     && rowPtr->upstream == nextHop
                     && rowPtr->srcAddr != rowPtr->upstream
                    && rowPtr->upInterface != interfaceId
                     && !(node->networkData.networkVar->iahepEnabled
                          && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                     && (rowPtr->treeState == ROUTING_PIMSM_G
                         || rowPtr->treeState == ROUTING_PIMSM_SG)
                     && rowPtr->upstreamState == PimSm_JoinPrune_Join
                     && RoutingPimSmCountValidOutgoingInterfaceCount(node, rowPtr) > 0)
        {
            int counter;
            if (MulticastCesRpimActiveOnInterface(node, rowPtr->upInterface) &&
                (rowPtr->upstream != NETWORK_UNREACHABLE && rowPtr->upInterface != NETWORK_UNREACHABLE
                 && node->networkData.networkVar->interfaceInfo[rowPtr->upInterface]->useMiMulticastMesh))
            {
                counter = RoutingPimSmGroupMemStateDecrementCounter(node,
                                                                    rowPtr->grpAddr,
                                                                    rowPtr->upstream,
                                                                    rowPtr->upInterface);
                if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                {
                    printf("\nnode %d Decrement counter in RoutingPimSmProcessUnicastRouteChange (upstream interface change). grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                           node->nodeId, rowPtr->grpAddr, rowPtr->upstream, rowPtr->upInterface, counter);
                }
                if (counter == 0)
                {
                    MiCesMulticastMeshInitiateMeshDeletion(node, rowPtr->upInterface, rowPtr->grpAddr, rowPtr->upstream);
                }
            }
            if (MulticastCesRpimActiveOnInterface(node, interfaceId) &&
                (nextHop != NETWORK_UNREACHABLE && interfaceId != NETWORK_UNREACHABLE
                 && node->networkData.networkVar->interfaceInfo[interfaceId]->useMiMulticastMesh))
            {
                counter = RoutingPimSmGroupMemStateIncrementCounter(node,
                                                                    rowPtr->grpAddr,
                                                                    nextHop,
                                                                    interfaceId);
                if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                {
                    printf("\nnode %d Increment counter in RoutingPimSmProcessUnicastRouteChange (upstream interface change). grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                           node->nodeId, rowPtr->grpAddr, nextHop, interfaceId, counter);
                }
                if (counter == 1)
                {
                    MiCesMulticastMeshInitiateMeshFormation(node, interfaceId, rowPtr->grpAddr, nextHop, pimDataSm->miMulticastMeshTimeout, FALSE);
                }
            }
        }
#endif // ADDON_BOEINGFCS
    }//end of for
}

/*
*  FUNCTION    : RoutingPimSmDeleteList
*  PURPOSE     : Free all items of list but doesn't free list."type" is used
*                to indicate whether the list contains a Message structure
*                or not. If so, call MESSAGE_Free() instead of MEM_free().
*  RETURN: None.
*/
void RoutingPimSmDeleteList(Node* node, LinkedList* list, BOOL type)
{
    ListItem* item;
    ListItem* tempItem;

    if (list->size == 0)
    {
        return;
    }
    item = list->first;

    while (item)
    {
        tempItem = item;
        item = item->next;

        if (type == FALSE)
        {
            MEM_free(tempItem->data);
        }
        else
        {
            MESSAGE_Free(node, (Message*) tempItem->data);
        }
        MEM_free(tempItem);
        list->size--;
    }
}
/*
*  FUNCTION    : RoutingPimSmHandleJoinPrunePacket
*  PURPOSE     : Handled incoming Join/Prune or end of message
*  RETURN: None.
*/
void
RoutingPimSmHandleJoinPrunePacket(Node *node,
                    int interfaceId,
                    RoutingPimSmJoinPruneGroupInfo* grpInfo,
                    RoutingPimEncodedSourceAddr* encodedSrcAddr,
                    RoutingPimEncodedUnicastAddr upstreamNbr,
                    RoutingPimJoinPruneMessageType isJoinOrPrune,
                    clocktype joinPruneHoldTime,
                    NodeAddress srcAddr)
{
    RoutingPimSmJoinPruneType       joinPruneType =
                    (RoutingPimSmJoinPruneType)0;
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr;
    RoutingPimSmMulticastTreeState treeState =
        (RoutingPimSmMulticastTreeState)0;

    NodeAddress RPAddr = 0;
    NodeAddress nextHop;
    int outInterface;
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimSmData = NULL;

    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimSmData = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimSmData = (PimDataSm*) pim->pimModePtr;
    }

    RoutingPimSmTreeInformationBase* treeInfoBase = &pimSmData->treeInfoBase;
    TreeInfoRowMap* rowPtrMap = treeInfoBase->rowPtrMap;
    TreeInfoRowMapIterator mapIterator;
    RoutingPimSmTreeInfoBaseRow * rowPtr = NULL;

    // isJoinOrPrune = 1 : Join
    // isJoinOrPrune = 2 : Prune
    if (isJoinOrPrune == ROUTING_PIM_END_OF_MESSAGE)
    {
        if (DEBUG)
        {
            printf(" Node %u: Handle SGrptEnd of message\n",
                    node->nodeId);
        }

        // if (S,G,rpt) entry present then call downsteam and upstream with
        // group address and encoded source address from tree info base

        /* Search treeInfo Base for the desired match */
        for (mapIterator = rowPtrMap->begin();
             mapIterator != rowPtrMap->end();
             mapIterator++)
        {
            rowPtr = mapIterator->second;
            if (DEBUG_ERRORS) {
                printf("   checking the treeInfo Base entry\n");
            }

            /* check for tree state */
            if (rowPtr->treeState == ROUTING_PIMSM_SGrpt)
            {
                /* Call (S,G,rpt) state machine to pass end of message
                All routers enroute to RP(including RP) that receive
                Join/Prune(S,G,Rpt) must process them*/
                RoutingPimSmHandleDownstreamStateMachine(node,
                            rowPtr->srcAddr,
                            interfaceId,
                            rowPtr->grpAddr,
                            rowPtr,
                            isJoinOrPrune,
                            joinPruneHoldTime);
            }
        }
        return;
    }
    /* check the join prune type from receieved message */
    if ((PimSmEncodedSourceAddrGetWildCard(encodedSrcAddr->rpSimEsa) == 1)
        &&
        (PimSmEncodedSourceAddrGetRPT(encodedSrcAddr->rpSimEsa) == 1))
    {
        joinPruneType = ROUTING_PIMSM_G_JOIN_PRUNE;
        treeState = ROUTING_PIMSM_G;
        if (isJoinOrPrune == ROUTING_PIM_JOIN)
        {
            pimSmData->stats.numOfGJoinPacketReceived++;
        }
        else if (isJoinOrPrune == ROUTING_PIM_PRUNE)
        {
            pimSmData->stats.numOfGPrunePacketReceived++;
        }
    }
    else if ((PimSmEncodedSourceAddrGetWildCard(encodedSrcAddr->rpSimEsa) == 0)
              &&
             (PimSmEncodedSourceAddrGetRPT(encodedSrcAddr->rpSimEsa) == 0))
    {
        joinPruneType = ROUTING_PIMSM_SG_JOIN_PRUNE;
        treeState = ROUTING_PIMSM_SG;
        if (isJoinOrPrune == ROUTING_PIM_JOIN)
        {
            pimSmData->stats.numOfSGJoinPacketReceived++;
        }
        else if (isJoinOrPrune == ROUTING_PIM_PRUNE)
        {
            pimSmData->stats.numOfSGPrunePacketReceived++;
        }
    }
    else if ((PimSmEncodedSourceAddrGetWildCard(encodedSrcAddr->rpSimEsa) == 0)
             &&
             (PimSmEncodedSourceAddrGetRPT(encodedSrcAddr->rpSimEsa) == 1))
    {
        joinPruneType = ROUTING_PIMSM_SGrpt_JOIN_PRUNE;
        treeState = ROUTING_PIMSM_SGrpt;
        if (isJoinOrPrune == ROUTING_PIM_JOIN)
        {
            pimSmData->stats.numOfSGrptJoinPacketReceived++;
        }
        else if (isJoinOrPrune == ROUTING_PIM_PRUNE)
        {
            pimSmData->stats.numOfSGrptPrunePacketReceived++;
        }
    }

    /*
    RFC:4601::Section 4.5.2
    When a router receives a Join(*,G), it must first check to see
    whether the RP in the message matches RP(G) (the router’s idea of who
    the RP is). If the RP in the message does not match RP(G), the
    Join(*,G) should be silently dropped.
    */

    RPAddr = RoutingPimSmFindRPForThisGroup(node,
                                grpInfo->encodedGrpAddr.groupAddr);

    if ((joinPruneType == ROUTING_PIMSM_G_JOIN_PRUNE)
        &&
        (isJoinOrPrune == ROUTING_PIM_JOIN)
        &&
        (RPAddr != encodedSrcAddr->sourceAddr))
    {
        //Discard the Join (*,G) silently;
        if (DEBUG)
        {
            printf("Node %u: received Join (*,G) but "
                   "the RP in the message does not matches with RP(G)\n",
                         node->nodeId);
        }

        return;
    }

    BOOL newEntry = FALSE;
    /* search for this group in multicast treeInfo Base */
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                            grpInfo->encodedGrpAddr.groupAddr,
                            encodedSrcAddr->sourceAddr,
                            treeState);
    /* if does not exist create one */
    if (treeInfoBaseRowPtr == NULL)
    {
        if (DEBUG_JP)
            printf("Time to built new entry in treeInfoBase \n");


        newEntry = TRUE;

        treeInfoBaseRowPtr =
            RoutingPimSmSetMulticastTreeInfoBase(node,
                                grpInfo->encodedGrpAddr.groupAddr,
                                encodedSrcAddr->sourceAddr,
                                treeState);
    }
    /* When considering a Join/Prune message whose
    * PIM Destination field addresses another router, most Join
    * or Prune messages could affect each upstream state machine
    */

    /* JoinPrune Packet sent to this router */
    if (getNodeAddressFromCharArray
        (upstreamNbr.unicastAddr) ==
        pim->interface[interfaceId].ipAddress)
    {
        if (joinPruneType == ROUTING_PIMSM_G_JOIN_PRUNE)
        {
            RoutingPimGetInterfaceAndNextHopFromForwardingTable
                (node, RPAddr, &outInterface, &nextHop);

            if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
            {
                RoutingPimSmGetNextHopOutInterface(node,
                                                   RPAddr,
                                                   &nextHop,
                                                   &outInterface);
            }
            if (outInterface == NETWORK_UNREACHABLE)
            {
                return;
            }
            if (nextHop == 0)
            {
                nextHop = RPAddr ;
            }

            /*
           * the (*, G) Join travels hop-by-hop towards the RP
           * for the group, and in each router it passes through,
           * SET multicast
           * tree state = group G
           */

            RoutingPimSmHandleDownstreamStateMachine(node,
                            encodedSrcAddr->sourceAddr,
                            interfaceId,
                            grpInfo->encodedGrpAddr.groupAddr,
                            treeInfoBaseRowPtr,
                            isJoinOrPrune,
                            joinPruneHoldTime);

            if (RPAddr != pim->interface[outInterface].ipAddress)
            {
                if (DEBUG_JP)
                    printf("Node %u: not RP, modify upstream\n",
                         node->nodeId);

                RoutingPimSmHandleUpstreamStateMachine(node,
                      encodedSrcAddr->sourceAddr, interfaceId,
                      grpInfo->encodedGrpAddr.groupAddr,
                      treeInfoBaseRowPtr,
                      joinPruneHoldTime);
            }
            else
            {
                NodeAddress srcAddr;
                for (mapIterator = rowPtrMap->begin();
                     mapIterator != rowPtrMap->end();
                     mapIterator++)
                {
                    rowPtr = mapIterator->second;

                    if (rowPtr->treeState != ROUTING_PIMSM_SG)
                    {
                        continue;
                    }

                    srcAddr = rowPtr->srcAddr ;
                    RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                        node,
                        srcAddr,
                        &outInterface,
                        &nextHop);

                    if (nextHop == 0)
                    {
                        nextHop = srcAddr ;
                    }
                    else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
                    {
                        RoutingPimSmGetNextHopOutInterface(node,
                                                          srcAddr,
                                                          &nextHop,
                                                          &outInterface);
                    }

                    if (outInterface == NETWORK_UNREACHABLE)
                    {
                        if (DEBUG)
                        {
                            printf("Node %u has invalid out interface"
                                   "towards 0x%x",node->nodeId,
                                   srcAddr);
                        }
                        continue;//to for loop
                    }

                    treeInfoBaseRowPtr =
                        RoutingPimSmSearchTreeInfoBaseForThisGroup
                                (node,
                                grpInfo->encodedGrpAddr.groupAddr,
                                srcAddr,
                                ROUTING_PIMSM_SG);
                    if (treeInfoBaseRowPtr == NULL)
                    {
                        continue;//to for loop
                    }
                    if (DEBUG_JP)
                    {
                        printf("Node %u: is RP, modify upstream\n",
                            node->nodeId);
                    }

                    RoutingPimSmHandleUpstreamStateMachine(node,
                            srcAddr, interfaceId,
                            grpInfo->encodedGrpAddr.groupAddr,
                            treeInfoBaseRowPtr,
                            joinPruneHoldTime);
                }
            }
        }
        else if (joinPruneType == ROUTING_PIMSM_SG_JOIN_PRUNE)
        {
            RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                node,
                encodedSrcAddr->sourceAddr,
                &outInterface,
                &nextHop);

            if (nextHop == 0)
            {
                nextHop = encodedSrcAddr->sourceAddr ;
            }
            else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
            {
                RoutingPimSmGetNextHopOutInterface(node,
                                                  encodedSrcAddr->sourceAddr,
                                                  &nextHop,
                                                  &outInterface);
            }

#ifdef ADDON_DB
           //for SourceTree
            StatsDBConnTable::MulticastConnectivity stats;

            stats.nodeId = node->nodeId;
            stats.destAddr = grpInfo->encodedGrpAddr.groupAddr;
            strcpy(stats.rootNodeType,"Source");
            stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                 encodedSrcAddr->sourceAddr);
            stats.outgoingInterface = interfaceId;
            stats.upstreamNeighborId =
                        MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                            nextHop);
            stats.upstreamInterface = outInterface;

            if (isJoinOrPrune == ROUTING_PIM_JOIN)
            {
                //insert this new entry into multicast_connectivity cache
                STATSDB_HandleMulticastConnCreation(node,stats);
            }
            else
            {
                STATSDB_HandleMulticastConnDeletion(node, stats);
            }
#endif

            if (outInterface == NETWORK_UNREACHABLE)
            {
                if (DEBUG)
                {
                    printf("Node %u has invalid out interface towards 0x%x"
                           ,node->nodeId,encodedSrcAddr->sourceAddr);
                }
                return;
            }

            RoutingPimSmHandleDownstreamStateMachine(node,
                               encodedSrcAddr->sourceAddr,
                               interfaceId,
                               grpInfo->encodedGrpAddr.groupAddr,
                               treeInfoBaseRowPtr,
                               isJoinOrPrune,
                               joinPruneHoldTime);

            if (encodedSrcAddr->sourceAddr ==
                    pim->interface[outInterface].ipAddress)
            {
                if (DEBUG_JP)
                    printf("Node %u: set upstrm join\n",
                        node->nodeId);

                treeInfoBaseRowPtr->upstreamState = PimSm_JoinPrune_Join;
            }
            if (encodedSrcAddr->sourceAddr !=
                    pim->interface[outInterface].ipAddress)
            {
                if (DEBUG_JP)
                    printf("Node %u:not McastSrc,so modify \
                           upstream\n", node->nodeId);

                RoutingPimSmHandleUpstreamStateMachine(node,
                      encodedSrcAddr->sourceAddr,
                      interfaceId,
                      grpInfo->encodedGrpAddr.groupAddr,
                      treeInfoBaseRowPtr,
                      joinPruneHoldTime);
#ifdef CYBER_CORE
                NetworkDataIp *ip = (NetworkDataIp *)
                                    node->networkData.networkVar;
                ERROR_Assert(ip != NULL, "networkData.networkVar is NULL");
                if (newEntry
                   && isJoinOrPrune == ROUTING_PIM_JOIN
                   && ip->iahepEnabled
                   && ip->iahepData->nodeType == RED_NODE
                   && !IsIAHEPRedSecureInterface(node, interfaceId))
                {
                    // if first time see this (S,G) join
                    // and this is a red node, join from non red intf
                    // need to send a IGMP JOIN with (S,G) info to black side
                    // so black side can setup SPT for (S',G')

                    RoutingPimSmIAHEPInitIgmpMsgToBlackNetwork(
                        node, encodedSrcAddr->sourceAddr,
                        grpInfo->encodedGrpAddr.groupAddr,
                        IGMP_MEMBERSHIP_REPORT_MSG);
                }
#endif
            }
        }
        else if (joinPruneType == ROUTING_PIMSM_SGrpt_JOIN_PRUNE)
        {
            if (DEBUG_JP)
            {
                printf(" Node %u: Handle SGrptJOINPRUNE\n",
                                node->nodeId);
            }

            RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                node,
                RPAddr,
                &outInterface,
                &nextHop);

            if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
            {
                RoutingPimSmGetNextHopOutInterface(node,
                                                   RPAddr,
                                                   &nextHop,
                                                   &outInterface);
            }
            if (outInterface == NETWORK_UNREACHABLE)
            {
                return;
            }
            if (nextHop == 0)
            {
                nextHop = RPAddr ;
            }
            /* All routers enroute to RP(including RP) that receive
            Join/Prune(S,G,Rpt) must process them */
            RoutingPimSmHandleDownstreamStateMachine(node,
                            encodedSrcAddr->sourceAddr,
                            interfaceId,
                            grpInfo->encodedGrpAddr.groupAddr,
                            treeInfoBaseRowPtr,
                            isJoinOrPrune,
                            joinPruneHoldTime);
            if (RPAddr != pim->interface[outInterface].ipAddress)
            {
                if (DEBUG_JP)
                    printf("Node %u: not RP, modify upstream\n",
                         node->nodeId);

                RoutingPimSmHandleUpstreamStateMachine(node,
                                  encodedSrcAddr->sourceAddr, interfaceId,
                                  grpInfo->encodedGrpAddr.groupAddr,
                                  treeInfoBaseRowPtr,
                                  joinPruneHoldTime);

            }
            else
            {
                PimData* pim = (PimData*)
                        NetworkIpGetMulticastRoutingProtocol(node,
                        MULTICAST_PROTOCOL_PIM);

                PimDataSm* pimDataSm = NULL;
                if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
                {
                    pimDataSm = (PimDataSm*)pim->pimModePtr;
                }
                else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
                {
                    pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
                }
                RoutingPimSmTreeInformationBase* treeInfo =
                              &pimDataSm->treeInfoBase;

                NodeAddress grpAddr =
                    grpInfo->encodedGrpAddr.groupAddr ;

                NodeAddress srcAddr;

                if (treeInfo->numEntries == 0)
                {
                    if (DEBUG_ERRORS)
                    {
                        ERROR_Assert(FALSE, "No Entry Found\n");
                    }
                    return;
                }

#ifdef ADDON_DB
                //for RPTree
                StatsDBConnTable::MulticastConnectivity stats;

                stats.nodeId = node->nodeId;
                stats.destAddr = grpInfo->encodedGrpAddr.groupAddr;
                strcpy(stats.rootNodeType,"RP");
                stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                            RPAddr);
                stats.outgoingInterface = interfaceId;
                stats.upstreamNeighborId =
                            MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                                  nextHop);

                if (node->nodeId == stats.upstreamNeighborId)
                {
                    //adding interface index as ANY_DEST
                    stats.upstreamInterface = ANY_DEST;
                }
                else
                {
                    stats.upstreamInterface = outInterface;
                }

                if (isJoinOrPrune == ROUTING_PIM_JOIN)
                {
                    //insert this new entry into multicast_connectivity cache
                    STATSDB_HandleMulticastConnCreation(node,stats);
                }
                else
                {
                    STATSDB_HandleMulticastConnDeletion(node, stats);
                }
#endif

                for (mapIterator = rowPtrMap->begin();
                     mapIterator != rowPtrMap->end();
                     mapIterator++)
                {
                    rowPtr = mapIterator->second;

                    if ((rowPtr->treeState != ROUTING_PIMSM_SG) ||
                        ((rowPtr->treeState == ROUTING_PIMSM_SG) &&
                        (encodedSrcAddr->sourceAddr != rowPtr->srcAddr)))
                    {
                        continue;
                    }

                    srcAddr = rowPtr->srcAddr ;
                    if (isJoinOrPrune == ROUTING_PIM_JOIN)
                    {
                        RoutingPimGetInterfaceAndNextHopFromForwardingTable(
                            node,
                            srcAddr,
                            &outInterface,
                            &nextHop);

                        if (nextHop == 0)
                        {
                            nextHop = srcAddr ;
                        }
                        else if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
                        {
                            RoutingPimSmGetNextHopOutInterface(node,
                                                              srcAddr,
                                                              &nextHop,
                                                              &outInterface);
                        }

                        if (outInterface == NETWORK_UNREACHABLE)
                        {
                            if (DEBUG)
                            {
                                printf("Node %u has invalid out interface"
                                       "towards 0x%x",node->nodeId,
                                       srcAddr);
                            }
                            continue;//to for loop
                        }

                        treeInfoBaseRowPtr =
                            RoutingPimSmSearchTreeInfoBaseForThisGroup
                                    (node,
                                    grpAddr,
                                    srcAddr,
                                    ROUTING_PIMSM_SG);

                        if (treeInfoBaseRowPtr != NULL
                            && treeInfoBaseRowPtr->upstreamState
                            == PimSm_JoinPrune_NotJoin)
                        {

                            RoutingPimSmJoinPruneTimerInfo
                                joinTimer;

                            clocktype delay;

                            if (DEBUG_JP)
                            {
                                printf(" Set Join-Prune State \
                                       to Prune \n");
                                printf(" Routing PimSm Send SG \
                                       %d Prune Packet \n",
                                       joinPruneType);
                            }

#ifdef ADDON_BOEINGFCS
                            if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface)
                                && !(node->networkData.networkVar->iahepEnabled
                                     && node->networkData.networkVar->iahepData->nodeType == RED_NODE)
                                && RoutingPimSmCountValidOutgoingInterfaceCount(node, treeInfoBaseRowPtr) > 0)
                            {
                                if (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
                                    && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh)
                                {
                                    int counter = RoutingPimSmGroupMemStateIncrementCounter(node,
                                                                                            treeInfoBaseRowPtr->grpAddr,
                                                                                            treeInfoBaseRowPtr->upstream,
                                                                                            treeInfoBaseRowPtr->upInterface);
                                    if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                                    {
                                        printf("\nnode %d Increment counter in RoutingPimSmHandleJoinPrunePacket (S,G)rpt. grpAddr %x upstream router: %x upstream interface: %d counter %d\n",
                                               node->nodeId, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, treeInfoBaseRowPtr->upInterface, counter);
                                    }
                                    if (counter == 1)
                                    {
                                        MiCesMulticastMeshInitiateMeshFormation(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, pimSmData->miMulticastMeshTimeout, FALSE);
                                    }
                                }
                            }
#endif // ADDON_BOEINGFCS
                            /* Set Join-Prune State to joined */
                            treeInfoBaseRowPtr->upstreamState =
                                PimSm_JoinPrune_Join;

                            /* Routing PimSm Send (S, G) Join
                             * Packet
                             */
                            RoutingPimSmSendJoinPrunePacket(node,
                                srcAddr,
                                nextHop,
                                grpAddr,
                                outInterface,
                                ROUTING_PIMSM_SG_JOIN_PRUNE,
                                ROUTING_PIM_JOIN_TREE,
                                treeInfoBaseRowPtr);

                                /* Routing PimSm Set Join Timer
                                 * to t_periodic
                                 */
                                joinTimer.srcAddr = srcAddr;
                                joinTimer.grpAddr = grpAddr;
                                joinTimer.intfIndex
                                    = outInterface;
                                joinTimer.seqNo =
                                    treeInfoBaseRowPtr->joinTimerSeq + 1;
                                joinTimer.treeState =
                                    ROUTING_PIMSM_SG;

                                // Note the latest joinTimerSeq
                                // in treeInfo Base

                                treeInfoBaseRowPtr->joinTimerSeq++;
                                delay = (clocktype)
                                  (pimDataSm->routingPimSmTPeriodicInterval);
                                treeInfoBaseRowPtr->lastJoinTimerEnd =
                                            node->getNodeTime() + delay;
                                RoutingPimSetTimer(node,
                                    outInterface,
                                    MSG_ROUTING_PimSmJoinTimerTimeout,
                                    (void*) &joinTimer,
                                    delay);
                        }
                    }
                }
            }
        }
    }
    else
    {
        if (DEBUG_JP)
        {
        printf("Node %u:not the targetRtr for \
            this Pkt(Src %u)\n",
            node->nodeId, encodedSrcAddr->sourceAddr);
        }
        outInterface = NETWORK_UNREACHABLE;
        nextHop = (unsigned int) NETWORK_UNREACHABLE;
        if (isJoinOrPrune == ROUTING_PIM_JOIN)
        {
#ifdef ADDON_BOEINGFCS
            // Set join seen in upstream intf as we would like
            // to change upstream nbrs regardless of whether
            // they are the actual next hop to the src before
            // suppressing join
            if (MulticastCesRpimActiveOnInterface(node, interfaceId))
            {
                treeInfoBaseRowPtr->joinSeenInUpIntf = TRUE;
            }
#endif
            if ((joinPruneType == ROUTING_PIMSM_G_JOIN_PRUNE) ||
                (joinPruneType == ROUTING_PIMSM_SGrpt_JOIN_PRUNE))
            {
                RoutingPimGetInterfaceAndNextHopFromForwardingTable
                    (node, RPAddr, &outInterface, &nextHop);

                if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
                {
                    RoutingPimSmGetNextHopOutInterface(node,
                                                       RPAddr,
                                                       &nextHop,
                                                       &outInterface);
                }

                if (nextHop == 0)
                {
                    nextHop = RPAddr ;
                }
            }
            else if (joinPruneType == ROUTING_PIMSM_SG_JOIN_PRUNE)
            {
                RoutingPimGetInterfaceAndNextHopFromForwardingTable
                    (node,
                    encodedSrcAddr->sourceAddr,
                    &outInterface,
                    &nextHop);

                if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
                {
                    RoutingPimSmGetNextHopOutInterface(node,
                                                  encodedSrcAddr->sourceAddr,
                                                  &nextHop,
                                                  &outInterface);
                }

                if (nextHop == 0)
                {
                    nextHop = encodedSrcAddr->sourceAddr ;
                }
            }
            if ((outInterface == interfaceId) &&
                (((pim->interface[interfaceId].interfaceType ==
                ROUTING_PIM_BROADCAST_INTERFACE)) &&
                    (nextHop == getNodeAddressFromCharArray
                     (upstreamNbr.unicastAddr))))
            {
                if (treeInfoBaseRowPtr)
                {
                    treeInfoBaseRowPtr->joinSeenInUpIntf = TRUE;
                }
            }
        }
        outInterface = NETWORK_UNREACHABLE;
        nextHop = (unsigned int)NETWORK_UNREACHABLE;

        if (isJoinOrPrune == ROUTING_PIM_PRUNE)
        {
#ifdef ADDON_BOEINGFCS
            // Set prune seen in upstream intf as we would like
            // to change upstream nbrs regardless of whether
            // they are the actual next hop to the src before
            // suppressing prune
            if (MulticastCesRpimActiveOnInterface(node, interfaceId))
            {
                treeInfoBaseRowPtr->pruneSeenInUpIntf = TRUE;
            }
#endif
            if ((joinPruneType == ROUTING_PIMSM_G_JOIN_PRUNE) ||
                (joinPruneType == ROUTING_PIMSM_SGrpt_JOIN_PRUNE))
            {

                RoutingPimGetInterfaceAndNextHopFromForwardingTable
                    (node, RPAddr, &outInterface, &nextHop);

                if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
                {
                    RoutingPimSmGetNextHopOutInterface(node,
                                                       RPAddr,
                                                       &nextHop,
                                                       &outInterface);
                }

                if (nextHop == 0)
                {
                    nextHop = RPAddr ;
                }
            }
            else if (joinPruneType == ROUTING_PIMSM_SG_JOIN_PRUNE)
            {

                RoutingPimGetInterfaceAndNextHopFromForwardingTable
                    (node,
                    encodedSrcAddr->sourceAddr,
                    &outInterface,
                    &nextHop);

                if (nextHop == (unsigned int)NETWORK_UNREACHABLE)
                {
                    RoutingPimSmGetNextHopOutInterface(node,
                                                  encodedSrcAddr->sourceAddr,
                                                  &nextHop,
                                                  &outInterface);
                }

                if (nextHop == 0)
                {
                    nextHop = encodedSrcAddr->sourceAddr ;
                }
            }
            if ((outInterface == interfaceId) &&
                (((pim->interface[interfaceId].interfaceType ==
                ROUTING_PIM_BROADCAST_INTERFACE)) &&
                (nextHop == getNodeAddressFromCharArray
                (upstreamNbr.unicastAddr))))
            {
                if (treeInfoBaseRowPtr)
                {
                    treeInfoBaseRowPtr->pruneSeenInUpIntf = TRUE;
                }
            }
        }
        BOOL suppressJoinPrune = FALSE;
#ifdef ADDON_BOEINGFCS
        NodeAddress origUpstream = treeInfoBaseRowPtr->upstream;
        {
            suppressJoinPrune = MulticastCesRpimSetSuppression(
                node,
                interfaceId,
                newEntry,
                joinPruneHoldTime,
                encodedSrcAddr,
                upstreamNbr,
                &treeInfoBaseRowPtr,
                srcAddr);
        }
        if (treeInfoBaseRowPtr->upstream != origUpstream)
        {
            if (!(node->networkData.networkVar->iahepEnabled
                     && node->networkData.networkVar->iahepData->nodeType ==
                                                                   RED_NODE)
                && ((treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G
                     && RoutingPimSmCountValidOutgoingInterfaceCount(node,
                                                    treeInfoBaseRowPtr) > 0)
                    || treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_SG)
                &&
                    (treeInfoBaseRowPtr->upstreamState ==
                      PimSm_JoinPrune_NotJoin
                      || treeInfoBaseRowPtr->upstreamState ==
                      PimSm_JoinPrune_Join
                      || treeInfoBaseRowPtr->upstreamState ==
                      PimSm_JoinPrune_PrunePending))

            {
                int counter;
                if (MulticastCesRpimActiveOnInterface(node,
                                         treeInfoBaseRowPtr->upInterface) &&
                    (origUpstream != NETWORK_UNREACHABLE &&
                                         treeInfoBaseRowPtr->upInterface !=
                                                        NETWORK_UNREACHABLE
                     && node->networkData.networkVar->
                                          interfaceInfo[treeInfoBaseRowPtr->
                                          upInterface]->useMiMulticastMesh))
                {
                    counter = RoutingPimSmGroupMemStateDecrementCounter(
                                            node,
                                            treeInfoBaseRowPtr->grpAddr,
                                            origUpstream,
                                            treeInfoBaseRowPtr->upInterface);
                    if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                    {
                        printf("\nnode %d Decrement counter in"
                         " RoutingPimSmProcessUnicastRouteChange"
                         " (upstream router change). grpAddr %x upstream"
                         " router: %x upstream interface: %d counter %d\n",
                              node->nodeId, treeInfoBaseRowPtr->grpAddr,
                              origUpstream, treeInfoBaseRowPtr->upInterface,
                              counter);
                    }
                    if (counter == 0)
                    {
                        MiCesMulticastMeshInitiateMeshDeletion(node,
                                         treeInfoBaseRowPtr->upInterface,
                                         treeInfoBaseRowPtr->grpAddr,
                                         origUpstream);
                    }
                }
                if (MulticastCesRpimActiveOnInterface(node, treeInfoBaseRowPtr->upInterface) &&
                    (treeInfoBaseRowPtr->upstream != NETWORK_UNREACHABLE && treeInfoBaseRowPtr->upInterface != NETWORK_UNREACHABLE
                     && node->networkData.networkVar->interfaceInfo[treeInfoBaseRowPtr->upInterface]->useMiMulticastMesh))
                {
                    counter = RoutingPimSmGroupMemStateIncrementCounter(node,
                                            treeInfoBaseRowPtr->grpAddr,
                                            treeInfoBaseRowPtr->upstream,
                                            treeInfoBaseRowPtr->upInterface);
                    if (DEBUG_GROUP_MEMBERSHIP_COUNTERS)
                    {
                        printf("\nnode %d Increment counter in "
                         "RoutingPimSmProcessUnicastRouteChange (upstream"
                         " router change). grpAddr %x upstream router: %x"
                         " upstream interface: %d counter %d\n",
                           node->nodeId, treeInfoBaseRowPtr->grpAddr,
                           treeInfoBaseRowPtr->upstream,
                           treeInfoBaseRowPtr->upInterface, counter);
                    }
                    if (counter == 1)
                    {
                        MiCesMulticastMeshInitiateMeshFormation(node, treeInfoBaseRowPtr->upInterface, treeInfoBaseRowPtr->grpAddr, treeInfoBaseRowPtr->upstream, pimSmData->miMulticastMeshTimeout, FALSE);
                    }
                }
            }
        }

#endif

        if (treeInfoBaseRowPtr &&
            ((treeInfoBaseRowPtr->joinSeenInUpIntf) ||
             (treeInfoBaseRowPtr->pruneSeenInUpIntf)))
        {
            RoutingPimSmHandleUpstreamStateMachine(node,
                                  encodedSrcAddr->sourceAddr,
                                  interfaceId,
                                  grpInfo->encodedGrpAddr.groupAddr,
                                  treeInfoBaseRowPtr,
                                  joinPruneHoldTime,
                                  suppressJoinPrune);
        }
    }
}

/*
*  FUNCTION    : RoutingPimSmCheckSwitchToSpt(S, G)
*  PURPOSE     : Routing PimSm Check Switch To Spt(S, G)
*                   if ((pim_include(*, G) (-) pim_exclude(S, G)
*                           (+) pim_include(S, G) != NULL)
*                       AND SwitchToSptDesired(S, G))
*                       {
*                            restart KeepAliveTimer(S, G);
*                       }
*  RETURN VALUE: BOOL
*/
BOOL
RoutingPimSmCheckSwitchToSpt(Node* node, NodeAddress grpAddr,
        NodeAddress srcAddr,
        RoutingPimSmDataPacketsPerGroup* sptSwitchoverDataPacketsForGroup)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }
    int i;
    /*
    *  In Sparse-Mode PIM, last-hop routers join the shared tree towards the
    *  RP. Once traffic from sources to joined groups arrives at a last-hop
    *  router it has the option of switching to receive the traffic on a
    *  shortest path tree (SPT).

    An "infinite threshhold" policy can be implemented making
    SwitchToSptDesired(S, G) return false all the time.  A "switch on first
    packet" policy can be implemented by making SwitchToSptDesired(S, G)
    return true once a single packet has been received for the source and
    group.
    */
    /* if (( pim_include(*,G) - pim_exclude(S,G)
        + pim_include(S,G) != NULL )
        AND SwitchToSptDesired(S,G) ) {
        # Note: Restarting the KAT will result in the SPT switch
        set KeepaliveTimer(S,G) to Keepalive_Period*/

    RoutingPimSmTreeInfoBaseRow  * treeInfoBaseRowPtr = NULL;
    BOOL sgJoinSent = FALSE;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if ((RoutingPimSmIncludeG(node, grpAddr, srcAddr, i)
                && (!RoutingPimSmExcludeSG(node, grpAddr, srcAddr, i))
                || RoutingPimSmIncludeSG(node, grpAddr, srcAddr, i)) &&
                (pimDataSm->sptSwitchThresholdInfo.threshold !=
                (unsigned int)ROUTING_PIMSM_SWITCH_SPT_THRESHOLD_INFINITE &&
               (unsigned int)
                sptSwitchoverDataPacketsForGroup->numDataPacketsReceived >=
                pimDataSm->sptSwitchThresholdInfo.threshold))
        {
            RoutingPimSmKeepAliveTimerInfo    timerInfo;
            clocktype delay;
            /* Check for group address */
            ListItem* tempListItem = NULL;
            RoutingPimSmRPAccessList* groupListPtr = NULL;
            NodeAddress grpMask;
            NodeAddress maskedGroupAddressToSearch;
            BOOL groupAddrFound = FALSE;
            unsigned int maskLength = 0;
            if (pimDataSm->sptSwitchThresholdInfo.grpAddrList->size == 0)
            {
                groupAddrFound = TRUE;
            }
            else
            {
                tempListItem =
                    pimDataSm->sptSwitchThresholdInfo.grpAddrList->first;
                while (tempListItem)
                {
                    groupListPtr =
                        (RoutingPimSmRPAccessList*)tempListItem->data;
                    maskLength = RoutingPimGetMaskLengthFromSubnetMask(
                                     groupListPtr->groupMask);

                    grpMask = ConvertNumHostBitsToSubnetMask
                        (32 - maskLength);

                    maskedGroupAddressToSearch = MaskIpAddress(grpAddr,
                                                           grpMask);
                    if (groupListPtr->groupAddr ==
                            maskedGroupAddressToSearch)
                    {
                        groupAddrFound = TRUE;
                        break;
                    }
                    tempListItem = tempListItem->next;
                }
            }
            if (DEBUG_FORWARD_PACKET) {
                printf("Node %u want to switch to SPT for intf %d\n",
                       node->nodeId, i);
            }

            if (groupAddrFound)
            {
                if (treeInfoBaseRowPtr == NULL)
                {
                    /* search for this (S, G) entry in multicast treeInfo
                    Base */
                    treeInfoBaseRowPtr =
                        RoutingPimSmSearchTreeInfoBaseForThisGroup
                                     (node, grpAddr, srcAddr,
                                     ROUTING_PIMSM_SG);
                    if (treeInfoBaseRowPtr == NULL)
                    {
                        treeInfoBaseRowPtr =
                            RoutingPimSmSetMulticastTreeInfoBase(node,
                                             grpAddr,
                                             srcAddr,
                                             ROUTING_PIMSM_SG);
                    }
                }

                if (treeInfoBaseRowPtr->keepAliveExpiryTimer == 0)
                {
                    /* set KeepaliveTimer(S, G) to Keepalive_Period */
                    timerInfo.srcAddr = srcAddr;
                    timerInfo.grpAddr = grpAddr;
                    timerInfo.intfIndex = i;

                    timerInfo.treeState = ROUTING_PIMSM_SG;

                    /* Note the current KeepAliveTimerSeq */


                    delay = (clocktype)
                        (pimDataSm->routingPimSmKeepaliveTimeout);

                    RoutingPimSetTimer(node,
                                       timerInfo.intfIndex,
                                       MSG_ROUTING_PimSmKeepAliveTimeOut,
                                       (void*) &timerInfo, delay);
                }

                treeInfoBaseRowPtr->keepAliveExpiryTimer = node->getNodeTime() +
                    (clocktype) (pimDataSm->routingPimSmKeepaliveTimeout);

                // Call Upstream state machine
                RoutingPimSmHandleUpstreamStateMachine(
                                        node,
                                        srcAddr,
                                        i,
                                        grpAddr,
                                        treeInfoBaseRowPtr);
                sgJoinSent = TRUE;
            }
        }
    }

    return sgJoinSent;
}

/*
*  FUNCTION    : RoutingPimSmUpdateSPTbit()
*  PURPOSE     : Routing PimSm Update_SPTbit of TreeInformationBase Table
*  RETURN VALUE: NULL
*/

void
RoutingPimSmUpdateSPTbit(Node* node, NodeAddress srcAddr,
                         NodeAddress grpAddr, int interfaceId,
                         RoutingPimSmTreeInfoBaseRow* routTableRowPtr)
{
    RoutingPimSmDownstream* downStreamListPtr = NULL;

    if (routTableRowPtr == NULL)
    {
        routTableRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                          (node, grpAddr, srcAddr, ROUTING_PIMSM_SGrpt);

    }
    if (routTableRowPtr != NULL)
    {
        if (routTableRowPtr->SPTbit)
        {
            return;
        }
        downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                            routTableRowPtr, interfaceId);
    }
    /* check if it is time to update SPT bit */

    NodeAddress RPAddr = RoutingPimSmFindRPForThisGroup(node, grpAddr);
    int srcInterface = 0;
    NodeAddress nextHopForSrc = 0;
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        srcAddr,
                                                        &srcInterface,
                                                        &nextHopForSrc);
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (srcAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            srcInterface = CPU_INTERFACE;
            break;
        }
    }
    int RPInterface = 0;
    NodeAddress nextHopForRP = 0;
    RoutingPimGetInterfaceAndNextHopFromForwardingTable(node,
                                                        RPAddr,
                                                        &RPInterface,
                                                        &nextHopForRP);
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (RPAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            RPInterface = CPU_INTERFACE;
            break;
        }
    }
    if (DEBUG_ERRORS)
    {
        printf("Node %u: check if it is time to update SPT bit \n",
               node->nodeId);
        printf("srcIntf = %d and inIntf = %d \n", srcInterface, interfaceId);
        printf("RPIntf %d \n", RPInterface);
        printf("SIntf %u\n", RoutingPimSmSelectNewRPFSG(node, grpAddr,
                srcAddr));
        printf("Gintf %u\n", RoutingPimSmSelectNewRPFG(node, grpAddr));
    }
#ifdef CYBER_CORE
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    if (RPInterface == CPU_INTERFACE &&
        ip->iahepEnabled &&
        ip->iahepData->nodeType == BLACK_NODE &&
        RoutingPimSmIsDefaultBlackMulticastGroup(node, grpAddr)
       )
    {
        // this is Black RP
        // for default black group keep using RPT for black RP node
        return;
    }
#endif
    BOOL sameInt = TRUE;
    BOOL sameRPInt = TRUE;
    if (srcInterface != interfaceId)
    {
        sameInt = FALSE;
    }

    if (srcInterface != RPInterface)
    {
        sameRPInt = FALSE;
    }
    /*if (iif == RPF_interface(S)
        AND JoinDesired(S,G) == TRUE
        AND ( DirectlyConnected(S) == TRUE
        OR RPF_interface(S) != RPF_interface(RP(G))
        OR inherited_olist(S,G,rpt) == NULL
        OR ( ( RPF’(S,G) == RPF’(*,G) ) AND
        ( RPF’(S,G) != NULL ) )
        OR ( I_Am_Assert_Loser(S,G,iif) ) {
            Set SPTbit(S,G) to TRUE
        }
        }*/

    if (interfaceId == srcInterface
        && (RoutingPimSmJoinDesiredSG(node, grpAddr, srcAddr))
        && ((RoutingPimSmCheckDirectlyConnectedSource(node, srcAddr,
                                                      interfaceId))
             || (!sameRPInt)
             || (RoutingPimSmInheritedOutListSGrpt(node, grpAddr, srcAddr,
                            routTableRowPtr) == 0)
             || ((nextHopForSrc == nextHopForRP) &&
                (nextHopForSrc != 0))
             || (downStreamListPtr && downStreamListPtr->assertState
                 == PimSm_Assert_ILostAssert)))
    {
            routTableRowPtr->SPTbit = TRUE;
            if (DEBUG_FORWARD_PACKET)
            {
                printf("Node %u A Setting spt bit \n",
                   node->nodeId);
            }
    }
    if (DEBUG_FORWARD_PACKET) {
        printf("Node %u: Set SPTbit(S, G) to %d \n", node->nodeId,
               routTableRowPtr->SPTbit);
    }
}

/*
*  FUNCTION    : RoutingPimSmGetSptThresholdInfo
*  PURPOSE     : Gets the SPT threshold information
*  RETURN: None.
*/
void RoutingPimSmGetSptThresholdInfo(
                Node *node,
                char *sptSwitchThresholdInfoStr,
                RoutingPimSmSptThreshold *sptSwitchThresholdInfo)
{
    char iotoken[MAX_STRING_LENGTH];
    char *next = NULL;
    char *token = IO_GetToken(iotoken, sptSwitchThresholdInfoStr, &next);
    sptSwitchThresholdInfo->threshold = (unsigned int)atoi(token);
    token = NULL;
    while (next)
    {
        token = IO_GetToken(iotoken, next, &next);
        if (token && strcmp(token,"GROUP-RANGE") == 0)
        {
            IO_TrimLeft(next);
            IO_TrimRight(next);
            sptSwitchThresholdInfo->grpAddrList =
                RoutingPimSmParseGroupRangeStr(
                                    node,
                                    next,
                                    sptSwitchThresholdInfo->grpAddrList);
            break;
        }
    }
    if (!token)
    {
        ListInit(node, &sptSwitchThresholdInfo->grpAddrList);
    }

}

#ifdef ADDON_DB


void RoutingPimSmDeleteMulticastTreeInfoBase(Node *node,
    RoutingPimSmTreeInfoBaseRow * treeInfoBaseRowPtr)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);

    PimDataSm* pimSmData = (PimDataSm*) pim->pimModePtr;
    RoutingPimSmTreeInformationBase* treeInfoBase = &pimSmData->treeInfoBase;
    //RoutingPimSmTreeInfoBaseRow* rowPtr = NULL;
    TreeInfoRowKey rowKey;
    memset(&rowKey, 0, sizeof(TreeInfoRowKey));

    TreeInfoRowMap* rowPtrMap = treeInfoBase->rowPtrMap;

    if (treeInfoBaseRowPtr->treeState == ROUTING_PIMSM_G)
    {
        rowKey.srcAddr = 0;

    }
    else //SG state or SG-RPT state
    {
        rowKey.srcAddr = treeInfoBaseRowPtr->srcAddr;
    }

    rowKey.grpAddr = treeInfoBaseRowPtr->grpAddr;
    rowKey.treeState = treeInfoBaseRowPtr->treeState;

    TreeInfoRowMapIterator findIterator = rowPtrMap->find(rowKey);
    ERROR_Assert(findIterator != rowPtrMap->end(),
        "Error in RoutingPimSmDeleteMulticastTreeInfoBase") ;

    free(findIterator->second) ;
    rowPtrMap->erase(findIterator) ;
    --treeInfoBase->numEntries;

}

BOOL RoutingPimSmIsMyMulticastPacket(Node *node,
                                     NodeAddress srcAddr,
                                     NodeAddress dstAddr,
                                     NodeAddress prevAddr,
                                     int incomingInterface)
{

    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    BOOL originateByMe = FALSE ;

    if (incomingInterface == -1)
    {
        return TRUE;
    }
    /*check if it is the RP */
    BOOL IAmRP = FALSE ;
    NodeAddress RPAddr = RoutingPimSmFindRPForThisGroup(node, dstAddr);
    int sourceInterface = NetworkGetInterfaceIndexForDestAddress
                                    (node, srcAddr);
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (RPAddr == pim->interface[i].ipAddress)
        {
            // treeinfobase state S_G

            IAmRP  = TRUE;
            //RPAddr = pim->interface[sourceInterface].ipAddress ;
            break ;
        }
    }

    BOOL IAmDR = FALSE ;
    if (pim->interface[incomingInterface].ipAddress ==
            pim->interface[incomingInterface].smInterface->drInfo.ipAddr)
    {
        IAmDR = TRUE;
    }

    int RPInterface = NetworkGetInterfaceIndexForDestAddress(node, RPAddr);

    BOOL SPTbit = FALSE ;
    BOOL createTreeInfoBaseRowSG = FALSE ;
    RoutingPimSmTreeInfoBaseRow *createdTreeInfoBaseRowPtr = NULL ;
    RoutingPimSmTreeInfoBaseRow *treeInfoBaseRowPtr = NULL ;

    /* check if this is a new pkt & the node is the originator of this Pkt */
    if (/*(ipHeader->ip_ttl == 64)
        && */(srcAddr ==
            pim->interface[incomingInterface].ipAddress))
    {
        originateByMe = TRUE;
        treeInfoBaseRowPtr
            = RoutingPimSmSearchTreeInfoBaseForThisGroup
                         (node, dstAddr, srcAddr, ROUTING_PIMSM_SG);

        if (treeInfoBaseRowPtr == NULL)
        {
            treeInfoBaseRowPtr = RoutingPimSmSetMulticastTreeInfoBase
                                 (node, dstAddr, srcAddr, ROUTING_PIMSM_SG);
            createTreeInfoBaseRowSG = TRUE ;
            createdTreeInfoBaseRowPtr = treeInfoBaseRowPtr ;
        }

//-

        // S_G state
        if (treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join)
        {
            SPTbit = TRUE;
        }

        // below is using updated code
        for (int i = 0; i < node->numberInterfaces; i++)
        {
            if (!IAmDR)
            {
                // forward to the designated router

                if (createTreeInfoBaseRowSG && createdTreeInfoBaseRowPtr)
                {

                    RoutingPimSmDeleteMulticastTreeInfoBase
                                 (node, createdTreeInfoBaseRowPtr);
                }
                return TRUE ;
            }
        }
    }// orignated by me
//-

    RoutingPimSmJoinPruneState upstreamState = PimSm_JoinPrune_NoInfo ;

    /* On receipt on a data from S to G on interface iif:
    * check if it is directly connected to source
    */
    if (RoutingPimSmCheckDirectlyConnectedSource(node, srcAddr,
        incomingInterface) && IAmDR)
    {
        // S_G state
        treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup
                         (node, dstAddr, srcAddr, ROUTING_PIMSM_SG);

        if (treeInfoBaseRowPtr == NULL)
        {
            treeInfoBaseRowPtr = RoutingPimSmSetMulticastTreeInfoBase
                                 (node, dstAddr, srcAddr, ROUTING_PIMSM_SG);
            createTreeInfoBaseRowSG = TRUE ;

            createdTreeInfoBaseRowPtr = treeInfoBaseRowPtr ;
        }

        BOOL sameInt = TRUE;
        if (incomingInterface != RPInterface)
        {
            sameInt = FALSE;
        }

        // below is using the logic of updated
        int srcInterface = NetworkGetInterfaceIndexForDestAddress
                                    (node, srcAddr);
        if (treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join
                && (!sameInt
                    || pim->interface[incomingInterface].interfaceType
                    == ROUTING_PIM_BROADCAST_INTERFACE &&
                    srcInterface == incomingInterface))
        {
            SPTbit = TRUE ;
        }

        BOOL couldRegisterSG = FALSE ;
        RoutingPimInterface* thisInterface =
                                          &pim->interface[incomingInterface];

        if (thisInterface->interfaceType == ROUTING_PIM_BROADCAST_INTERFACE)
        {
            if ((thisInterface->ipAddress ==
                    thisInterface->smInterface->drInfo.ipAddr)
                    && (RoutingPimSmCheckDirectlyConnectedSource(node,
                                               srcAddr, incomingInterface)))
            {
                couldRegisterSG = TRUE ;
            }
        }
        else
        {
            if ((RoutingPimSmCheckDirectlyConnectedSource(node,
                                                         srcAddr,
                                                         incomingInterface)))
            {

                couldRegisterSG = TRUE ;
            }
        }

        if (couldRegisterSG)
        {
            if (IAmDR && IAmRP)
            {
                if (treeInfoBaseRowPtr->isTunnelPresent ||
                    treeInfoBaseRowPtr->registerState ==
                        PimSm_Register_NoInfo)
                {
                    upstreamState = PimSm_JoinPrune_Join;
                }
            }
            else if (IAmDR)
            {
                if (treeInfoBaseRowPtr->isTunnelPresent ||
                    treeInfoBaseRowPtr->registerState ==
                        PimSm_Register_NoInfo)
                {
                    /* create & send register packet */

                    if (createTreeInfoBaseRowSG && createdTreeInfoBaseRowPtr)
                    {
                        RoutingPimSmDeleteMulticastTreeInfoBase(node,
                                                  createdTreeInfoBaseRowPtr);
                    }

                    return TRUE ;

                }
            }
            else
            {
                if (originateByMe == TRUE)
                {
                    if (treeInfoBaseRowPtr->isTunnelPresent ||
                        treeInfoBaseRowPtr->registerState ==
                                                      PimSm_Register_NoInfo)
                    {
                        if (treeInfoBaseRowPtr->SPTbit == FALSE)
                        {
                            if (createTreeInfoBaseRowSG &&
                                                   createdTreeInfoBaseRowPtr)
                            {

                                RoutingPimSmDeleteMulticastTreeInfoBase
                                    (node, createdTreeInfoBaseRowPtr);
                            }
                            return FALSE;
                        }
                    }
                }
            }

        }// could register SG

    } // if directly connected
    treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                         dstAddr, srcAddr, ROUTING_PIMSM_SG);

    if (treeInfoBaseRowPtr )
    {
        BOOL sameInt = TRUE;
        if (incomingInterface != RPInterface)
        {
            sameInt = FALSE;
        }
        int srcInterface = NetworkGetInterfaceIndexForDestAddress
                                    (node, srcAddr);
        if ((treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join
            || upstreamState == PimSm_JoinPrune_Join)
                    && (!sameInt ||
                        (pim->interface[incomingInterface].interfaceType
                        == ROUTING_PIM_BROADCAST_INTERFACE && !IAmRP &&
                        srcInterface == incomingInterface)
                        || (incomingInterface == RPInterface
                            && IAmRP
                            && incomingInterface ==
                            treeInfoBaseRowPtr->upInterface)))
        {
            //treeInfoBaseRowPtr->SPTbit = TRUE ;
            SPTbit = TRUE ;
        }
    }
    else treeInfoBaseRowPtr = RoutingPimSmSearchTreeInfoBaseForThisGroup(
                                 node,
                                 dstAddr,
                                 srcAddr,
                                 ROUTING_PIMSM_G);

    if (treeInfoBaseRowPtr == NULL)
    {

        return FALSE;
    }

    BOOL routingPimSmCouldAssertSG = FALSE ;

    {
        // pls do not remove these brackets
        int srcInterface =
            NetworkGetInterfaceIndexForDestAddress(node, srcAddr);
        /* it checks to see if S-G in its multicast treeInfo Base */
        RoutingPimSmTreeInfoBaseRow* routTableRowPtr =
            RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                          dstAddr,
                          srcAddr,
                          ROUTING_PIMSM_SG);

        if (routTableRowPtr)
        {
            /* check if the interface present in the downstream list */
            BOOL sameInt = TRUE;

            if (srcInterface != incomingInterface)
            {
                sameInt = FALSE;
            }

            if ((routTableRowPtr->SPTbit == TRUE || SPTbit)
                && (!sameInt)
                && ((RoutingPimSmJoinRP(node,
                                        dstAddr,
                                        srcAddr,
                                        incomingInterface))
                  || (RoutingPimSmJoinG(node,
                                        dstAddr,
                                        srcAddr,
                                        incomingInterface))
                    && (!RoutingPimSmPruneSGrpt(node,
                                                dstAddr,
                                                srcAddr,
                                                incomingInterface))
                    || (RoutingPimSmIncludeG(node,
                                             dstAddr,
                                             srcAddr,
                                             incomingInterface))
                    && (!RoutingPimSmExcludeSG(node,
                                               dstAddr,
                                               srcAddr,
                                               incomingInterface))
                    && (!RoutingPimSmLostAssertG(node,
                                                 dstAddr,
                                                 srcAddr,
                                                 incomingInterface))
                    || (RoutingPimSmJoinSG(node,
                                           dstAddr,
                                           srcAddr,
                                           incomingInterface))
                    || (RoutingPimSmIncludeSG(node,
                                              dstAddr,
                                              srcAddr,
                                              incomingInterface))))
            {
                // could assert
                routingPimSmCouldAssertSG = TRUE ;
            }
        }
    }


    /*Data Arrives From S To G On I And CouldAssert(GI) */
//    if (RoutingPimSmCouldAssertSG(node, dstAddr, srcAddr,
//                              incomingInterface)
    if (routingPimSmCouldAssertSG
        && (pim->interface[incomingInterface].interfaceType ==
            ROUTING_PIM_BROADCAST_INTERFACE))
    {
        RoutingPimSmDownstream* associatedDownstream = NULL;
        RoutingPimSmTreeInfoBaseRow       *currentTreeInfoBaseRowPtr = NULL;
        /* it checks to see if S-G in its multicast treeInfo Base */
        currentTreeInfoBaseRowPtr =
            RoutingPimSmSearchTreeInfoBaseForThisGroup(node,
                             dstAddr, srcAddr, ROUTING_PIMSM_SG);

        if (currentTreeInfoBaseRowPtr == NULL)
        {
            // error state, but we do not want to make assert
            return TRUE ;
        }
        else /* check if the interface present in the downstream list */
        {
            /* update the SPT bit in treeInfo Base */
            SPTbit = TRUE;

            associatedDownstream = IsInterfaceInPimSmDownstreamList(node,
                                   treeInfoBaseRowPtr, incomingInterface);
        }

        if (associatedDownstream != NULL)
        {

            if (currentTreeInfoBaseRowPtr->SPTbit == FALSE)
            {
                SPTbit = TRUE ;
            }
        }
    }

    /*Time to create the fresh out Interface List for forwarding packet*/
    BOOL foundOutgoingInterface = FALSE ;
    LinkedList* outIntfList;
    ListInit(node, &outIntfList);

    /* if incominginterface is the source interface */
    BOOL sameInt = TRUE;

    if (sourceInterface != incomingInterface && !originateByMe)
    {
        sameInt = FALSE;
    }

    if ((sameInt)
        && ((treeInfoBaseRowPtr->SPTbit || SPTbit )
            && (treeInfoBaseRowPtr->upstreamState == PimSm_JoinPrune_Join||
            upstreamState == PimSm_JoinPrune_Join)))
    {
        ListItem* tempListItem;
        if (RoutingPimSmInheritedOutListSG
            (node, dstAddr, srcAddr, treeInfoBaseRowPtr) != 0)
        {
            tempListItem = treeInfoBaseRowPtr->inheritedOutIntfSG->first;

            while (tempListItem)
            {
                int* intIdPtr = (int*) tempListItem->data ;


                RoutingPimSmDownstream* downStreamListPtr;
                downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                                    treeInfoBaseRowPtr,* intIdPtr);


                if (downStreamListPtr == NULL
#ifdef ADDON_BOEINGFCS
                        // may have a valid downstream interface for G
                        || downStreamListPtr->joinPruneState != PimSm_JoinPrune_Join
#endif
                   )
                {
                    RoutingPimSmTreeInfoBaseRow     * treeInfoBaseRowPtrG;
                    treeInfoBaseRowPtrG =
                        RoutingPimSmSearchTreeInfoBaseForThisGroup(
                            node,
                            dstAddr,
                            srcAddr,
                            ROUTING_PIMSM_G);

#ifdef ADDON_BOEINGFCS
                    if (downStreamListPtr == NULL)
                    {
#endif
                        if (treeInfoBaseRowPtrG == NULL)
                        {
                            if (createTreeInfoBaseRowSG &&
                                                  createdTreeInfoBaseRowPtr)
                            {
                                RoutingPimSmDeleteMulticastTreeInfoBase
                                                 (node,
                                                 createdTreeInfoBaseRowPtr);
                            }
                            return FALSE;
                        }
#ifdef ADDON_BOEINGFCS
                    }

                    if (treeInfoBaseRowPtrG)
                    {
#endif

                        downStreamListPtr = IsInterfaceInPimSmDownstreamList(
                                                node,
                                                treeInfoBaseRowPtrG,
                                                * intIdPtr);
#ifdef ADDON_BOEINGFCS
                    }
#endif
                }

#ifdef ADDON_BOEINGFCS
                //downStreamListPtr can be NULL here...
                if (!downStreamListPtr)
                {
                    tempListItem = tempListItem->next;
                    continue ;
                }
#endif

                if (downStreamListPtr->joinPruneState !=
                        PimSm_JoinPrune_Join)
                {
                    tempListItem = tempListItem->next;
                    continue ;
                }

                foundOutgoingInterface = TRUE ;
                break ;

                tempListItem = tempListItem->next;
            }
        }
        if (createTreeInfoBaseRowSG && createdTreeInfoBaseRowPtr)
        {
            RoutingPimSmDeleteMulticastTreeInfoBase
                             (node, createdTreeInfoBaseRowPtr);
        }
        if (foundOutgoingInterface != 0)
        {
            return TRUE ;
        }else return FALSE ;
    }
    else if ((incomingInterface == RPInterface)
            && (treeInfoBaseRowPtr->SPTbit == FALSE))
    {
        //RoutingPimSmTreeInfoBaseRow * inheritedRow;
        ListItem* tempListItem;

        /*Time to make oiflist = inherited_olist(S, G, rpt)  */
        if (RoutingPimSmInheritedOutListSGrpt(node,
                                               dstAddr,
                                               srcAddr,
                                               treeInfoBaseRowPtr) != 0)
        {
            tempListItem = treeInfoBaseRowPtr->inheritedOutIntfSGrpt->first;

            while (tempListItem)
            {
                unsigned int* inheritedIntIdPtr =
                    (unsigned int*) tempListItem->data ;

                RoutingPimSmDownstream* downStreamListPtr = NULL;
                downStreamListPtr = IsInterfaceInPimSmDownstreamList(node,
                                    treeInfoBaseRowPtr,* inheritedIntIdPtr);

                if (downStreamListPtr != NULL
                        && (downStreamListPtr->joinPruneState !=
                            PimSm_JoinPrune_Join))
                {
                    tempListItem = tempListItem->next;
                    continue ;
                }

                foundOutgoingInterface = TRUE ;
                break ;
                tempListItem = tempListItem->next;
            }

            if (createTreeInfoBaseRowSG && createdTreeInfoBaseRowPtr)
            {
                RoutingPimSmDeleteMulticastTreeInfoBase
                             (node, createdTreeInfoBaseRowPtr);
            }
            if (foundOutgoingInterface != 0)
            {
                return TRUE ;
            }else return FALSE ;
        }

    }

    if (createTreeInfoBaseRowSG && createdTreeInfoBaseRowPtr)
    {
        RoutingPimSmDeleteMulticastTreeInfoBase
                     (node, createdTreeInfoBaseRowPtr);
    }
    return TRUE;
}
#endif
#ifdef ADDON_BOEINGFCS
int RoutingPimSmGroupMemStateIncrementCounter(Node* node, NodeAddress groupAddr, NodeAddress upstream, int upInterface)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    RoutingPimSmGroupMemStateCounters* counterMap = pimDataSm->groupMemStateCounters;

    RoutingPimSmGroupMemStateCounterKey key;

    key.groupAddr = groupAddr;
    key.upstream = upstream;
    key.upInterface = upInterface;

    RoutingPimSmGroupMemStateCounters::iterator itr = counterMap->find(key);

    if (itr == counterMap->end())
    {
        counterMap->insert(make_pair(key,1));
        return 1;
    }
    else
    {
        return ++itr->second;
    }
}

int RoutingPimSmGroupMemStateDecrementCounter(Node*
                                         node,
                                         NodeAddress groupAddr,
                                         NodeAddress upstream,
                                         int upInterface)
{
    PimData* pim = (PimData*)
          NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*) pim->pimModePtr)->pimSm;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*) pim->pimModePtr;
    }

    RoutingPimSmGroupMemStateCounters* counterMap =
                                            pimDataSm->groupMemStateCounters;

    RoutingPimSmGroupMemStateCounterKey key;

    key.groupAddr = groupAddr;
    key.upstream = upstream;
    key.upInterface = upInterface;

    RoutingPimSmGroupMemStateCounters::iterator itr = counterMap->find(key);

    if (itr == counterMap->end())
    {
        counterMap->insert(make_pair(key,-1));
        //printf("Error: decrement of new counter\n");
        return -1;
    }
    else
    {
        return --itr->second;
    }
}

unsigned int RoutingPimSmCountValidOutgoingInterfaceCount(Node* node,
                                          RoutingPimSmTreeInfoBaseRow* entry)
{
    ListItem* tempListItem;

    tempListItem = entry->downstream->first;

    int validInterfaceCount = 0;

    while (tempListItem)
    {
        RoutingPimSmDownstream* downStream = (RoutingPimSmDownstream*)
                                             tempListItem->data;

        if (downStream->joinPruneState == PimSm_JoinPrune_Join
            || downStream->joinPruneState == PimSm_JoinPrune_PrunePending)
        {
            validInterfaceCount++;
        }

        tempListItem = tempListItem->next;
    }//End while

    return validInterfaceCount;
    /*
    if (entry->treeState == ROUTING_PIMSM_G)
    {
        return RoutingPimSmImmediateOutListG(node, entry->grpAddr,
                                                     entry->srcAddr, entry);
    }
    else if (entry->treeState == ROUTING_PIMSM_SG)
    {
        return RoutingPimSmImmediateOutListSG(node, entry->grpAddr,
                                                    entry->srcAddr, entry);
    }
    */
}
#endif // ADDON_BOEINGFCS
