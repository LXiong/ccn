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
//
//
//
// Purpose: Simulate MPLS LDP rfc 3036
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "mpls.h"
#include "network_ip.h"
#include "transport_tcp.h"
#include "app_util.h"

#include "mpls_ldp.h"
#include "mpls_shim.h"
#include "buffer.h"

#define MPLS_LDP_DEBUG 0

static
BOOL MplsLdpPerformLabelRequestProcedure(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* peer,
    Mpls_FEC fec,
    Attribute_Type* RAttribute,
    Attribute_Type* SAttribute,
    NodeAddress upstreamLSR_Id,
    LabelRequestStatus requestStatus);

static
BOOL MplsLdpPerformLsrLabelDistributionProcedure(
    Node* node,
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    MplsLdpHelloAdjacency* peer,
    Attribute_Type* SAttribute,
    Attribute_Type* RAttribute,
    NodeAddress msgSource);

static
BOOL IsNextHopExistForThisDest(
    Node* node,
    NodeAddress destAddr,
    NodeAddress* nextHopAddress);

//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpPrintDesiredSizeAndActualSizeOfStructures()
//
// PURPOSE      : printing size of different message structure
//                for debugging.
//
// PARAMETERS   : none.
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpPrintDesiredSizeAndActualSizeOfStructures()
{
    static int i = 0;

    // print the structues/sizeof structures only once
    if (i == 0)
    {
        printf("2^20-1 = %d  -- 2^3 = %d\n"
               "ALL THE SIZE PRINTED BELOW IS IN TERMS OF BITS\n"
               "size of LDP_Generic_Label_TLV_Value_Type RFC: 32\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of"
               " LDP_Common_Hello_Parameters_TLV_Value_Type RFC: 32\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_FEC_Prefix_Value_Type RFC: 24\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_Prefix_FEC_Element RFC: 32\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_FEC_HostAddress_Value_Type RFC: 24\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_ATM_TLV RFC: 32\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_FrameRelay_TLV RFC: 32\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_Address_List_TLV_Value_Header_Type RFC:16\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_HopCount_TLV_Value_Type RFC:8"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_Status_TLV_Value_Type RFC: 80\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_Common_Session_TLV_Value_Type RFC:112\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_ATM_Common_Session_TLV_value RFC:32\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_ATM_FrameRelay_TLV_value RFC:32\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of"
               " LDP_Label_Request_MessageId_TLV_Value_Type RFC:32\n"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n"
               "size of LDP_VandorSpecific_TLV_Value_Type RFC: 32"
               "using sizeof %" TYPES_SIZEOFMFT "u \n"
               "------------------------------------------------\n",
               (int) pow(2.0,20.0) - 1,
               (int) pow(2.0,3.0),
               (sizeof(LDP_Generic_Label_TLV_Value_Type) * 8),
               (sizeof(LDP_Common_Hello_Parameters_TLV_Value_Type) * 8),
               (sizeof(LDP_FEC_Prefix_Value_Type) * 8),
               (sizeof(LDP_Prefix_FEC_Element) * 8),
               (sizeof(LDP_FEC_HostAddress_Value_Type) * 8),
               (sizeof(LDP_HostAddress_FEC_Element) * 8),
               (sizeof(LDP_ATM_TLV) * 8),
               (sizeof(LDP_FrameRelay_TLV) * 8),
               (sizeof(LDP_Address_List_TLV_Value_Header_Type) * 8),
               (sizeof(LDP_HopCount_TLV_Value_Type) * 8),
               (sizeof(LDP_Status_TLV_Value_Type) * 8),
               (sizeof(LDP_Common_Session_TLV_Value_Type) * 8),
               (sizeof(LDP_ATM_Common_Session_TLV_value) * 8),
               (sizeof(LDP_ATM_FrameRelay_TLV_value) * 8),
               (sizeof(LDP_Label_Request_MessageId_TLV_Value_Type) * 8),
               (sizeof(LDP_VendorSpecific_TLV_Value_Type) * 8));
    }
    i++;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpPrintLib()
//
// PURPOSE      : printing LIB table for debugging purpose.
//
// PARAMETERS   : node - which is performing the above operation
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpPrintLib(Node* node)
{
    MplsLdpApp* ldp = NULL;
    unsigned int i = 0;

    ldp = MplsLdpAppGetStateSpace(node);

    printf("node --> %d\n"
           "----------------------------------------------------------"
           "------------------\n"
           "%6s %16s %16s %16s %16s\n"
           "----------------------------------------------------------"
           "------------------\n",
           node->nodeId,
           "Length",
           "IP_Address",
           "Next_hop",
           "LSR_ID",
           "Label_Value");

    for (i = 0; i < ldp->LIB_NumEntries; i++)
    {
        char prefix[MAX_IP_STRING_LENGTH];
        char nextHop[MAX_IP_STRING_LENGTH];
        char lsr[MAX_IP_STRING_LENGTH];

        IO_ConvertIpAddressToString(ldp->LIB_Entries[i].prefix, prefix);
        IO_ConvertIpAddressToString(ldp->LIB_Entries[i].nextHop, nextHop);
        IO_ConvertIpAddressToString(ldp->LIB_Entries[i].LSR_Id.LSR_Id, lsr);

        printf("%4u %16s %16s %16s %8u %4u\n",
               ldp->LIB_Entries[i].length,
               prefix,
               nextHop,
               lsr,
               ldp->LIB_Entries[i].LSR_Id.label_space_id,
               ldp->LIB_Entries[i].label);
    }
    printf("\n\n");
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpPrintPendingRequestList()
//
// PURPOSE      : printing entries in the "pendingRequest" cache.
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to MplsLdpApp structure.
//
// RETURN VALUE : none
//
//
//-------------------------------------------------------------------------
static
void MplsLdpPrintPendingRequestList(Node* node, MplsLdpApp* ldp)
{
    char nextHop[MAX_IP_STRING_LENGTH];
    char sourceAddr[MAX_IP_STRING_LENGTH];
    char ipAddress[MAX_IP_STRING_LENGTH];
    unsigned int i = 0;

    printf("----------------------------------------------------------\n"
           "node = %u (Pending label request list)\n"
           "----------------------------------------------------------\n"
           "%16s %16s  %16s"
           "----------------------------------------------------------\n",
           node->nodeId,
           "Next_Hop_LSR_Id",
           "Sourse_LSR_Id",
           "fec.Ip_Address\n");

    for (i = 0; i < ldp->pendingRequestNumEntries; i++)
    {
        IO_ConvertIpAddressToString(ldp->pendingRequest[i].nextHopLSR_Id,
            nextHop);

        IO_ConvertIpAddressToString(ldp->pendingRequest[i].sourceLSR_Id,
            sourceAddr);

        IO_ConvertIpAddressToString(ldp->pendingRequest[i].fec.ipAddress,
            ipAddress);

        printf("%16s %16s %16s\n", nextHop, sourceAddr, ipAddress);
    }
    printf("\n\n");
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpListHelloAdjacency()
//
// PURPOSE      : printing entries in the hello adjacency list.
//
// PARAMETERS   : node - which is performing the above operation
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpListHelloAdjacency(Node* node)
{
    MplsLdpApp* ldp = NULL;
    unsigned int i = 0;

    ldp = MplsLdpAppGetStateSpace(node);

    printf("---------  %u --------------------------\n"
           "%17s %17s\n"
           "---------------------------------------\n",
           node->nodeId,
           "LSR_Id_OF_The_LSR",
           "LinkAddr_of_LSR");

    for (i = 0; i < ldp->numHelloAdjacency; i++)
    {
        char lsr[MAX_IP_STRING_LENGTH];
        char linkAddr[MAX_IP_STRING_LENGTH];

        IO_ConvertIpAddressToString(ldp->helloAdjacency[i].LSR2.LSR_Id, lsr);

        IO_ConvertIpAddressToString(ldp->helloAdjacency[i].LinkAddress,
            linkAddr);

        printf("%17s %17s\n",lsr, linkAddr);
    }
    printf("\n\n");
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpPrintIncomingFecToLabelMappingRec()
//
// PURPOSE      : printing entries in the "incomingFecToLabelMappingRec"
//                cache.
//
// PARAMETERS   : node - which is performing the above operation
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpPrintIncomingFecToLabelMappingRec(Node* node)
{
    unsigned int i = 0;
    MplsLdpApp* ldp = NULL;

    ldp = MplsLdpAppGetStateSpace(node);

    if (!ldp)
    {
        return;
    }

    printf("------------------------------------------------------\n"
           "node = %u\n"
           "------------------------------------------------------\n"
           "%16s %18s %16s\n"
           "------------------------------------------------------\n",
           node->nodeId,
           "Fec/Ip_Address",
           "Peer_Address/LsrId",
           "Incoming_Label");

    for (i = 0; i < ldp->incomingFecToLabelMappingRecNumEntries; i++)
    {
        char ipAddress[MAX_IP_STRING_LENGTH];
        char peerAddr[MAX_IP_STRING_LENGTH];
        unsigned int label = ldp->incomingFecToLabelMappingRec[i].label;

        IO_ConvertIpAddressToString(
            ldp->incomingFecToLabelMappingRec[i].fec.ipAddress,
            ipAddress);

        IO_ConvertIpAddressToString(
            ldp->incomingFecToLabelMappingRec[i].peer,
            peerAddr);

        printf("%16s %16s %14u\n", ipAddress, peerAddr, label);
    }
    printf("\n\n");
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpShowFecToLabelMappingRec()
//
// PURPOSE      : printing entries in the "fecToLabelMappingRec" cache.
//
// PARAMETERS   : node - which is performing the above operation
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpShowFecToLabelMappingRec(Node* node)
{
    unsigned int i = 0;
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    if (!ldp)
    {
        return;
    }

    printf("------------------------------------------\n"
           "node = %u\n"
           "------------------------------------------\n"
           "%16s %15s %8s\n"
           "------------------------------------------\n",
           node->nodeId,
           "fec/IP_Address",
           "Peer",
           "Label");

    for (i = 0; i < ldp->fecToLabelMappingRecNumEntries; i++)
    {
        char ipAddress[MAX_IP_STRING_LENGTH];
        char peerAddr[MAX_IP_STRING_LENGTH];
        unsigned int label = ldp->fecToLabelMappingRec[i].label;

        IO_ConvertIpAddressToString(
            ldp->fecToLabelMappingRec[i].fec.ipAddress,
            ipAddress);

        IO_ConvertIpAddressToString(
            ldp->fecToLabelMappingRec[i].peer,
            peerAddr);

        printf("%16s %16s %4u", ipAddress, peerAddr, label);
    }
    printf("\n\n");
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpPrintLocalAddresses()
//
// PURPOSE      : printing entries in the local Addresses of an LSR.
//
// PARAMETERS   : node - which is performing the above operation
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpPrintLocalAddresses(Node* node)
{
    unsigned int i = 0;
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    printf("---------------------------------------\n"
           "node = %u\n"
           "---------------------------------------\n",
           node->nodeId);

    for (i = 0; i < ldp->numLocalAddr; i++)
    {
        char localAddr[MAX_IP_STRING_LENGTH];

        IO_ConvertIpAddressToString(ldp->localAddr[i], localAddr);
        printf("LocalAddress[%d] = %16s\n", i, localAddr);
    }
    printf("\n\n");
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpRemoveILMEntry()
//
// PURPOSE      : deleting a row from ILM table.
//
// PARAMETERS   : mpls - pointer to mpls var
//                nexthop - next hop to match
//                label - label to delete
//                inOut - either type INCOMING or OUTGOING
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpRemoveILMEntry(
    MplsData* mpls,
    NodeAddress nextHop,
    unsigned int label,
    unsigned int inOut)
{
    int i = 0;
    int index = 0;
    BOOL toBeDeleted = FALSE;

    for (i = 0; i < mpls->numILMEntries; i++)
    {
        unsigned int out_label = 0;

        if (mpls->ILM[i].nhlfe->labelStack)
        {
               out_label = mpls->ILM[i].nhlfe->labelStack[0];
        }

        if (inOut == OUTGOING)
        {
            if ((out_label == label) &&
                (nextHop == mpls->ILM[i].nhlfe->nextHop))
            {
               toBeDeleted = TRUE;
               index = i;
               break;
            }
        }
        else if (inOut == INCOMING)
        {
            if (mpls->ILM[i].label == label)
            {
                toBeDeleted = TRUE;
                index = i;
            }
        }
    }

    if (toBeDeleted)
    {
        MEM_free(mpls->ILM[index].nhlfe->labelStack);
        MEM_free(mpls->ILM[index].nhlfe);

        if (index == (mpls->numILMEntries - 1))
        {
            memset(&(mpls->ILM[index]), 0, sizeof(Mpls_ILM_entry));
        }
        else
        {
            int i = 0;

            // compress by shifting element of the array one place
            // left if any intermediate element is deleted
            for (i = (index + 1); i < mpls->numILMEntries; i++)
            {
                memcpy(&(mpls->ILM[i - 1]), &(mpls->ILM[i]),
                       sizeof(Mpls_ILM_entry));
            }

            memset(&(mpls->ILM[(mpls->numILMEntries) - 1]), 0,
                sizeof(Mpls_ILM_entry));
        }
        (mpls->numILMEntries)--;
    }
}


//-------------------------------------------------------------------------
//
// FUNCTION     :  MatchFec()
//
// PURPOSE      :  matching a fec "x" with a fec "y"
//
// PARAMETERS   :  x - fec "x"
//                 y - fec "y"
//
// RETURN VALUE : TRUE if it is a match or FALSE otherwise
//
//-------------------------------------------------------------------------
static
BOOL MatchFec(Mpls_FEC* x, Mpls_FEC* y)
{
    if (((x->ipAddress == ANY_IP) && (x->numSignificantBits == 0)) ||
        ((y->ipAddress == ANY_IP) && (y->numSignificantBits == 0)))
    {
        return TRUE;
    }
    else
    {
        // Changed for adding priority support for FEC classification.
        // While matching FEC, compare the priorities also.
        BOOL retVal = (((x)->ipAddress == (y)->ipAddress) &&
            ((x)->numSignificantBits == (y)->numSignificantBits)
            && ((x)->priority == (y)->priority)) ;

        return retVal;
    }
}


//-------------------------------------------------------------------------
//
// FUNCTION     :  MplsLdpRemoveHeader()
//
// PURPOSE      :  This function is called to remove a header from the
//                 packet enclosed in the message. The "packetSize"
//                 variable in the message will be decreased by "hdrSize".
//
// PARAMETERS   :  node - node which is removing the header
//                 msg -  message for which header is being removed
//                 hdrSize - size of the header being removed
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpSkipHeader(
    Node* node,
    Message* msg,
    int hdrSize)
{
    if (MPLS_LDP_DEBUG)
    {
       printf("node = %u is removing header\n", node->nodeId);
    }
    msg->packet += hdrSize;
    msg->packetSize -= hdrSize;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpAppGetStateSpace()
//
// PURPOSE      : returning pointer to the structure MplsLdpApp if
//                node is given as the argument to the function.
//
// PARAMETERS   : node - the node which will return the "pointer
//                       to the structure MplsLdpApp"
//
// RETURN VALUE : pointer to the structure MplsLdpApp if MPLS-LDP
//                structure is initialized, or NULL otherwise.
//
// ASSUMPTIONS  : none
//
//-------------------------------------------------------------------------
MplsLdpApp* MplsLdpAppGetStateSpace(Node* node)
{
    AppInfo* appList = node->appData.appPtr;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_MPLS_LDP)
        {
            return (MplsLdpApp*) appList->appDetail;
        }
    }
    return NULL;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpSetTcpConnectionClosed()
//
// PURPOSE      : indicates a tcp connection is closed for a
//                hello adjacency
//
// PARAMETERS   : ldp - pointer to MplsLdpApp structure
//                connId - connection id to match
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpSetTcpConnectionClosed(
    MplsLdpApp* ldp,
    int connId)
{
    unsigned int i = 0;

    while (i < ldp->numHelloAdjacency)
    {
        if (ldp->helloAdjacency[i].connectionId == connId)
        {
            ldp->helloAdjacency[i].connectionId = INVALID_TCP_CONNECTION_ID;
            break;
        }
        i++;
    }
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpCheckAndRemoveExpiredHelloAdjacencies()
//
// PURPOSE      : Remove hello adjacencies from hello adjacency list if
//                no message is received from the hello adjacency(s) for
//                a certain interval
//
// PARAMETERS   : node - which is removing hello adjacency
//                ldp - pointer to the MplsLdpApp structure.
//                helloPeer - peer to be removed
//                matchHelloPeer - should I match "helloPeer" or will
//                                 check for all hello adjacencies.
//
// RETURN VALUE : TRUE if a hello adjacency is removed, FALSE otherwise
//
//-------------------------------------------------------------------------
static
BOOL MplsLdpCheckAndRemoveExpiredHelloAdjacencies(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* helloPeer,
    BOOL matchHelloPeer)
{
    // Removes an hello adjacency from the list if no message is received
    // from the node for a certain interval

    BOOL needRemoval = FALSE;
    int index = -1;

    MplsLdpHelloAdjacency* helloAdjacency = ldp->helloAdjacency;

    if ((helloPeer == NULL) && (matchHelloPeer == TRUE))
    {
       ERROR_Assert(FALSE, "Error in"
           " MplsLdpCheckAndRemoveExpiredHelloAdjacencies()"
           " Cannot remove NULL peer !!!");
    }

    if (matchHelloPeer == TRUE)
    {
        unsigned int i = 0;

        while (i < ldp->numHelloAdjacency)
        {
            BOOL removalContition = (helloAdjacency[i].lastHeard <
                 (node->getNodeTime() - ldp->keepAliveTime));

            if ((helloPeer == (&helloAdjacency[i])) && (removalContition))
            {
                needRemoval = TRUE;
                index = i;
                break;
            }

            i++;
        }
    }
    else if (matchHelloPeer == FALSE)
    {
        unsigned int i = 0;

        while (i < ldp->numHelloAdjacency)
        {
            BOOL removalContition = (helloAdjacency[i].lastHeard <
                 (node->getNodeTime() - ldp->keepAliveTime));

            if (removalContition)
            {
                needRemoval = TRUE;
                index = i;
                break;
            }
            i++;
        }
    }

    if (needRemoval)
    {
        unsigned int i = 0;

        MEM_free(ldp->helloAdjacency[index].buffer);
        MEM_free(ldp->helloAdjacency[index].incomingCache);
        MEM_free(ldp->helloAdjacency[index].outboundCache);
        MEM_free(ldp->helloAdjacency[index].outboundLabelMapping);
        MEM_free(ldp->helloAdjacency[index].interfaceAddresses);

        BUFFER_ClearAssemblyBuffer(
            &(ldp->helloAdjacency[index].reassemblyBuffer), 0, FALSE);
        BUFFER_SetAnticipatedSizeForAssemblyBuffer(
                        &(ldp->helloAdjacency[index].reassemblyBuffer), 0);

        for (i = index + 1; i < ldp->numHelloAdjacency; i++)
        {
            memcpy(&helloAdjacency[i - 1],
                &helloAdjacency[i],
                sizeof(MplsLdpHelloAdjacency));

        }
        memset(&(helloAdjacency[ldp->numHelloAdjacency - 1]), 0,
              sizeof(MplsLdpHelloAdjacency));
        ldp->numHelloAdjacency--;
    }
    return needRemoval;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpStartKeepAliveTimerForThisHelloAdjacency()
//
// PURPOSE      : starts keep alive timer for a given hello adjacency
//
// PARAMETERS   : node - which is starting hello adjacency timer
//                ldp - pointer to the MplsLdpApp structure.
//                helloAdjacency - pointer to the hello adjacency
//
// RETURN VALUE : TRUE if a hello adjacency is removed, FALSE otherwise
//
//-------------------------------------------------------------------------
static
void MplsLdpStartKeepAliveTimerForThisHelloAdjacency(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* helloAdjacency)
{
    Message* msg = MESSAGE_Alloc(
                       node,
                       APP_LAYER,
                       APP_MPLS_LDP,
                       MSG_APP_MplsLdpKeepAliveTimerExpired);

    MESSAGE_InfoAlloc(node, msg, sizeof(NodeAddress));

    memcpy(MESSAGE_ReturnInfo(msg),
        &(helloAdjacency->LSR2.LSR_Id),
        sizeof(NodeAddress));

    MESSAGE_Send(node, msg, ldp->keepAliveTime);
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpReturnHelloAdjacency()
//
// PURPOSE      : searching and hello adjacency from the hello adjacency
//                list. Given : it's LDP identifier type or link address
//                or both.
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                linkAddress - link adderess to be matched
//                LSR - LSR_Id to be matched
//                matchLinkAddress - matches link address if value of this
//                                   parameter is set to TRUE.
//                matchLSR - matches LSR_Id if value of this parameter is
//                           set to TRUE.
//
// RETURN VALUE : pointer to the Mpls-Ldp hello adjacency neighbor in the
//                hello adjacency list if match occues. Or NULL otherwise.
//
//-------------------------------------------------------------------------
static
MplsLdpHelloAdjacency* MplsLdpReturnHelloAdjacency(
    MplsLdpApp* ldp,
    NodeAddress linkAddress,
    LDP_Identifier_Type LSR,
    BOOL matchLinkAddress,
    BOOL matchLSR)
{
    unsigned int i = 0;

    ERROR_Assert(matchLinkAddress || matchLSR, "Error in"
        " MplsLdpReturnHelloAdjacency()"
        " matchLinkAddress, matchLSR both FALSE !!!");

    ERROR_Assert (ldp, "Error in MplsLdpReturnHelloAdjacency()"
        " ldp is NULL !!!");

    if (MPLS_LDP_DEBUG)
    {
        printf("Num of hello adjacency %d\n",ldp->numHelloAdjacency);
    }

    for (i = 0; i < ldp->numHelloAdjacency; i++)
    {
        if ((ldp->helloAdjacency[i].LinkAddress == linkAddress) ||
            (!matchLinkAddress))  // either the link address matches
                                  // or you don't care if it matches
        {
            if (!matchLSR)  // all we wanted to was match the link address
            {
                if (MPLS_LDP_DEBUG)
                {
                    char mode[2][MAX_STRING_LENGTH] = {"ACTIVE",
                                                       "PASSIVE"};

                    printf("MplsLdpReturnHelloAdjacency"
                           " matches link address %d "
                           "with entry %d: %s\n",
                           linkAddress,
                           i,
                           mode[ldp->helloAdjacency[i].initMode]);
                }
                return &ldp->helloAdjacency[i];
            }
            else
            {
                if ((ldp->helloAdjacency[i].LSR2.LSR_Id == LSR.LSR_Id) &&
                    (ldp->helloAdjacency[i].LSR2.label_space_id ==
                     LSR.label_space_id))
                {
                    if (MPLS_LDP_DEBUG)
                    {
                        char mode[2][MAX_STRING_LENGTH] = {"ACTIVE",
                                                           "PASSIVE"};

                        printf("MplsLdpReturnHelloAdjacency"
                               " matches link address %d "
                               "with entry %d: %s\n",
                               linkAddress,
                               i,
                               mode[ldp->helloAdjacency[i].initMode]);
                    }
                    return &ldp->helloAdjacency[i];
                }
            } // end else
        } //end if on linkAddress
    } // end for (i = 0; i < ldp->numHelloAdjacency; i++)
    return NULL;
}

//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpReturnHelloAdjacencyForThisConnectionId()
//
// PURPOSE      : searching and hello adjacency from the hello adjacency
//                list. Given : it's connection id
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                Connection_Id - Connection_Id to be matched
//
// RETURN VALUE : pointer to the Mpls-Ldp hello adjacency neighbor in the
//                hello adjacency list if match occues. Or NULL otherwise.
//-------------------------------------------------------------------------
static
MplsLdpHelloAdjacency* MplsLdpReturnHelloAdjacencyForThisConnectionId(
    MplsLdpApp* ldp,
    int Connection_Id)
{
    unsigned int i = 0;
    BOOL found = FALSE;

    for (i = 0; i < ldp->numHelloAdjacency; i++)
    {
        if (ldp->helloAdjacency[i].connectionId == Connection_Id)
        {
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        return &(ldp->helloAdjacency[i]);
    }
    else
    {
        return NULL;
    }
}
//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpReturnHelloAdjacencyForThisLSR_Id()
//
// PURPOSE      : searching and hello adjacency from the hello adjacency
//                list. Given : it's LDP identifier type
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                LSR - LSR_Id to be matched
//
// RETURN VALUE : pointer to the Mpls-Ldp hello adjacency neighbor in the
//                hello adjacency list if match occues. Or NULL otherwise.
//-------------------------------------------------------------------------
static
MplsLdpHelloAdjacency* MplsLdpReturnHelloAdjacencyForThisLSR_Id(
    MplsLdpApp* ldp,
    NodeAddress peerLSR_Id)
{
    unsigned int i = 0;
    BOOL found = FALSE;

    for (i = 0; i < ldp->numHelloAdjacency; i++)
    {
        if (ldp->helloAdjacency[i].LSR2.LSR_Id == peerLSR_Id)
        {
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        return &(ldp->helloAdjacency[i]);
    }
    else
    {
        return NULL;
    }
}

//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpSwapAdderss()
//
// PURPOSE      : Swaping Ip address positions within the Loacl address
//                array. The address in the position index1 will be placed
//                in the index2 and nodeAddress2 will go in the index1
//
// PARAMETERS   : addrArray[] - local address array.
//                nodeAddress2 - address to be added.
//                index1 - position1
//                index2 - position2
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
static
void MplsLdpSwapAdderss(
    NodeAddress addrArray[],
    NodeAddress nodeAddress2,
    int index1,
    int index2)
{
    NodeAddress tempNodeAddress = addrArray[index1];
    addrArray[index1] = nodeAddress2;
    addrArray[index2] = tempNodeAddress;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddSourceAddr()
//
// PURPOSE      : Adding local addresses to LSR. Also to place lowest IP
//                address at the localAddress[0]. It not necesserly sorts
//                the address but only ensures that lowest address will
//                be in the 0'th position of the array.
//
// PARAMETERS   : node - pointer to the node.
//                sourceAddr - source address to add.
//
// RETURN VALUE : the pointer to the MPLS LDP data structure,
//                NULL if nothing found
//
// ASSUMPTION   : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddSourceAddr(MplsLdpApp* ldp, NodeAddress sourceAddr)
{
    if (ldp->numLocalAddr == ldp->maxLocalAddr) // Need to add more space
    {
        NodeAddress* nextLocalAddrPtr = NULL;

        nextLocalAddrPtr = (NodeAddress*)
            MEM_malloc(sizeof(NodeAddress)
                * (ldp->maxLocalAddr + MPLS_LDP_APP_INITIAL_NUM_LOCAL_ADDR));

        memcpy(nextLocalAddrPtr,
             ldp->localAddr,
             sizeof(NodeAddress) * ldp->maxLocalAddr);

        MEM_free(ldp->localAddr);

        ldp->localAddr = nextLocalAddrPtr;

        ldp->maxLocalAddr += MPLS_LDP_APP_INITIAL_NUM_LOCAL_ADDR;
    }

    if (sourceAddr < ldp->localAddr[0])
    {
        MplsLdpSwapAdderss(ldp->localAddr,
                           sourceAddr,
                           0,
                           ldp->numLocalAddr);
    }
    else
    {
        ldp->localAddr[ldp->numLocalAddr] = sourceAddr;
    }

    ldp->numLocalAddr++;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddPduHeader().
//
// PURPOSE:     : adding PDU header to message awating in the message
//                buffer before despatching it.
//
// PARAMETERS   : buffer - pointer to the message buffer.
//                LSR_ID - LSR_ID of the LSR which is adding the PDU header
//                label_space_id - mpls ldp label space id.
//
// RETURN VALUE : none
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
static
void MplsLdpAddPduHeader(
    PacketBuffer* buffer,
    NodeAddress LSR_Id,
    unsigned short label_space_id)
{
    char* headerPtr = NULL;
    LDP_Header actualHeader;

    unsigned short pdu_length = (unsigned short)
        (BUFFER_GetCurrentSize(buffer) + LDP_IDENTIFIER_SIZE);

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        LDP_HEADER_SIZE,
        (char**) &headerPtr);

    actualHeader.Version = LDP_RFC_3036_PROTOCOL_VERSION;

    actualHeader.PDU_Length = pdu_length; // total length of this PDU in
                                          // octets, excluding the Version
                                          // and PDU Length fields.

    actualHeader.LDP_Identifier.LSR_Id = LSR_Id;
    actualHeader.LDP_Identifier.label_space_id = label_space_id;

    memcpy(headerPtr, &actualHeader, LDP_HEADER_SIZE);
}


//-------------------------------------------------------------------------
// FUNCTION    : MplsLdpTransmitPduFromHelloAdjacencyBuffer().
//
// PURPOSE:    : despatching PDU from the message buffer of hello
//               adjacency list
//
// PARAMETERS  : node - node which is sending the message.
//               ldp - pointer to the MplsLdpApp structure.
//               helloAdjacency - pointer to hello adjacency list.
//
// RETURN VALUE : none
//
// ASSUMPTION  : none
//-------------------------------------------------------------------------
static void
MplsLdpTransmitPduFromHelloAdjacencyBuffer(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* helloAdjacency)
{
    if (BUFFER_GetCurrentSize(helloAdjacency->buffer) > 0)
    {
        MplsLdpAddPduHeader(
            helloAdjacency->buffer,
            ldp->LSR_Id,
            PLATFORM_WIDE_LABEL_SPACE_ID);

        if (MPLS_LDP_DEBUG)
        {
            printf("node %d sends %d bytes via tcp conn id %d\n"
                   "LSR IdentifierType = %d to %u\n",
                   node->nodeId,
                   BUFFER_GetCurrentSize(helloAdjacency->buffer),
                   helloAdjacency->connectionId,
                   ldp->LSR_Id,
                   helloAdjacency->LinkAddress);
        }

        UpdateLastMsgSent(helloAdjacency);

        ERROR_Assert(helloAdjacency->connectionId,
            "TCP connection id is ZERO !!!");

        Message *msg = APP_TcpCreateMessage(
            node,
            helloAdjacency->connectionId,
            TRACE_MPLS_LDP);

        APP_AddPayload(
            node,
            msg,
            BUFFER_GetData(helloAdjacency->buffer),
            BUFFER_GetCurrentSize(helloAdjacency->buffer));

        APP_TcpSend(node, msg);

        BUFFER_ClearPacketBufferData(helloAdjacency->buffer);
    }
}


//-------------------------------------------------------------------------
// FUNCTION    : MplsLdpSubmitMessageToHelloAdjacencyBuffer().
//
// PURPOSE:    : put the MPLS-LDP message(s) into the message buffer
//               of the hello adjacency list
//
// PARAMETERS  : node - node which is inputting the message.
//               ldp - pointer to the MplsLdpApp structure.
//               bufffer - pointer to the message buffer
//               helloAdjacency - pointer to hello adjacency list.
//
// RETURN VALUE : none
//
// ASSUMPTION  : none
//-------------------------------------------------------------------------
static
void MplsLdpSubmitMessageToHelloAdjacencyBuffer(
    Node* node,
    MplsLdpApp* ldp,
    const PacketBuffer* buffer,
    MplsLdpHelloAdjacency* helloAdjacency)
{
    if ((unsigned) (BUFFER_GetCurrentSize(buffer) + LDP_HEADER_SIZE) >
        (ldp->maxPDULength - ((unsigned)
            BUFFER_GetCurrentSize(helloAdjacency->buffer))))
    {
        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            helloAdjacency);
    }
    BUFFER_ConcatenatePacketBuffer(buffer, helloAdjacency->buffer);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddRowToOutboundLabelMappingCache()
//
// PURPOSE      : adding a row to out bound label mapping cache
//
// PARAMETERS   : node - node which is adding a record to
//                       outbound label mapping cache
//                helloAdjacency - pointer to hello adjacency list.
//                fec - fec for which label mapping is sent.
//                LSR_Id - LSR to which label mapping is sent.
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddRowToOutboundLabelMappingCache(
    Node* node,
    MplsLdpHelloAdjacency* helloAdjacency,
    Mpls_FEC fec,
    unsigned int label,
    NodeAddress LSR_Id)
{
    int index = helloAdjacency->outboundLabelMappingNumEntries;

    if (helloAdjacency->outboundLabelMappingNumEntries ==
            helloAdjacency->outboundLabelMappingMaxEntries)
    {
        OutboundLabelRequestCache* newTable;

        newTable = (OutboundLabelRequestCache*)
                    MEM_malloc(sizeof(OutboundLabelRequestCache) * 2
                        * helloAdjacency->outboundLabelMappingMaxEntries);

        memcpy(newTable, helloAdjacency->outboundLabelMapping,
               sizeof(OutboundLabelRequestCache));

        MEM_free(helloAdjacency->outboundLabelMapping);

        helloAdjacency->outboundLabelMapping = newTable;

        helloAdjacency->outboundLabelMappingMaxEntries =
            (2 * helloAdjacency->outboundLabelMappingMaxEntries);
    }

    memcpy(&(helloAdjacency->outboundLabelMapping[index].fec),
        &fec,
        sizeof(Mpls_FEC));

    helloAdjacency->outboundLabelMapping[index].LSR_Id = LSR_Id;
    helloAdjacency->outboundLabelMapping[index].label = label;

    helloAdjacency->outboundLabelMapping[index].exit_time =
        node->getNodeTime();

    helloAdjacency->outboundLabelMappingNumEntries ++;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpFreeAttributeType()
//
// PURPOSE      : Freeing the structure Attribute_Type
//
// PARAMETERS   : attr - pointer to the structure
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpFreeAttributeType(Attribute_Type* attr)
{
    MEM_free(attr->pathVectorValue.pathVector);
    attr->pathVectorValue.pathVector = NULL;
    memset(attr, 0, sizeof(Attribute_Type));
}


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpAddAttributeType();
//
//  PURPOSE      : adding source attribute to dest Attribute
//
//  PARAMETER    : dest - destination location
//                 source - source location
//
//  RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpAddAttributeType(Attribute_Type* dest, Attribute_Type* source)
{
    ERROR_Assert(dest, "Error in MplsLdpAddAttributeType()"
        " dest Attribute space is NULL !!!");

    if (source)
    {
        dest->hopCount = source->hopCount;
        dest->pathVector = source->pathVector;
        dest->numHop = source->numHop;

        // copy path vector of source
        if (source->pathVector == TRUE)
        {
            dest->pathVectorValue.length =
                source->pathVectorValue.length;

            dest->pathVectorValue.pathVector = (NodeAddress*)
                MEM_malloc((sizeof(NodeAddress)
                    * source->pathVectorValue.length));

            memcpy(dest->pathVectorValue.pathVector,
                source->pathVectorValue.pathVector,
                (source->pathVectorValue.length * sizeof(NodeAddress)));

        }
        else if (source->pathVector == FALSE)
        {
            dest->pathVectorValue.length = 0;
            dest->pathVectorValue.pathVector = NULL;
        }
    }
    else // if (source == NULL)
    {
        memset(dest, 0, sizeof(Attribute_Type));
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddRowToIncomingFecToLabelMappingRec()
//
// PURPOSE      : adding a row to incomingFecToLabelMappingRec cache
//
// PARAMETERS   : node - node which is adding a record to
//                       incomingFecToLabelMappingRec cache
//                ldp - pointer to the MplsLdpApp structure.
//                fec - fec entry label mapping is received.
//                peer - pointer to peer LSR's record in hello adjacency
//                       list from which label mapping is received.
//                label - label which is received.
//                RAttribute - received attribute value
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddRowToIncomingFecToLabelMappingRec(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    MplsLdpHelloAdjacency* peer,
    unsigned int label,
    Attribute_Type* RAttribute)
{
    int index = ldp->incomingFecToLabelMappingRecNumEntries;

    if (ldp->incomingFecToLabelMappingRecNumEntries ==
            ldp->incomingFecToLabelMappingRecMaxEntries)
    {
        FecToLabelMappingRec* newTable = NULL;

        newTable = (FecToLabelMappingRec*)
                   MEM_malloc(sizeof(FecToLabelMappingRec) * 2
                       * ldp->incomingFecToLabelMappingRecMaxEntries);

        memcpy(newTable,
            ldp->incomingFecToLabelMappingRec,
            (sizeof(FecToLabelMappingRec)
               * ldp->incomingFecToLabelMappingRecNumEntries));

        MEM_free(ldp->incomingFecToLabelMappingRec);

        ldp->incomingFecToLabelMappingRec = newTable;

        ldp->incomingFecToLabelMappingRecMaxEntries =
            (2 * ldp->incomingFecToLabelMappingRecMaxEntries);
    }

    memcpy(&(ldp->incomingFecToLabelMappingRec[index].fec),
        &fec,
        sizeof(Mpls_FEC));

    ldp->incomingFecToLabelMappingRec[index].label = label;
    ldp->incomingFecToLabelMappingRec[index].peer = peer->LSR2.LSR_Id;

    // copy RAttribute
    MplsLdpAddAttributeType(
        (&ldp->incomingFecToLabelMappingRec[index].Attribute),
        RAttribute);

    (ldp->incomingFecToLabelMappingRecNumEntries)++;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddRowToOutboundLabelRequestCache()
//
// PURPOSE      : adding a record to outboundCache of hello adjacency
//
// PARAMETERS   : node - node which is adding a record to
//                       incomingFecToLabelMappingRec cache
//                helloAdjacency - pointer to MplsLdpHelloAdjacency
//                                 structure.
//                fec - fec entry t
//                LSR_Id - LSR_Id of the hello adjacency
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddRowToOutboundLabelRequestCache(
    Node* node,
    MplsLdpHelloAdjacency* helloAdjacency,
    Mpls_FEC fec,
    NodeAddress LSR_Id)
{
    int index = helloAdjacency->outboundCacheNumEntries;

    if (helloAdjacency->outboundCacheNumEntries ==
            helloAdjacency->outboundCacheMaxEntries)
    {
        OutboundLabelRequestCache* newTable = NULL;

        newTable = (OutboundLabelRequestCache*)
                   MEM_malloc(sizeof(OutboundLabelRequestCache) * 2
                       * helloAdjacency->outboundCacheMaxEntries);

        memcpy(newTable, helloAdjacency->outboundCache,
            sizeof(OutboundLabelRequestCache)
               * helloAdjacency->outboundCacheNumEntries);

        MEM_free(helloAdjacency->outboundCache);

        helloAdjacency->outboundCache = newTable;

        helloAdjacency->outboundCacheMaxEntries =
            (2 * helloAdjacency->outboundCacheMaxEntries);
    }

    memcpy(&(helloAdjacency->outboundCache[index].fec),
           &fec,
           sizeof(Mpls_FEC));

    helloAdjacency->outboundCache[index].LSR_Id = LSR_Id;
    helloAdjacency->outboundCache[index].exit_time = node->getNodeTime();
    helloAdjacency->outboundCacheNumEntries++;
}


//-------------------------------------------------------------------------
// FUNCTION    : MplsLdpAddMessageHeader().
//
// PURPOSE:    : to put the MPLS-LDP message header to the LDP messages
//               before they put to the message buffer of
//               the hello adjacency list
//
// PARAMETERS  : buffer - pointer to the packet buffer.
//               U - Unknown message bit. See structure "LDP_Message_Header"
//               Message_Type - message type
//               Message_Length - length of the message
//               Message_Id - message identification number.
//
// RETURN VALUE : none
//
// ASSUMPTION  : none
//-------------------------------------------------------------------------
static
void MplsLdpAddMessageHeader(
    PacketBuffer* buffer,
    unsigned short U,
    unsigned short Message_Type,
    unsigned short Message_Length,
    unsigned int Message_Id)
{
    char* msgHeaderPtr;
    LDP_Message_Header actualMsgHeader;

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_Message_Header),
        (char**) &msgHeaderPtr);

    LDP_Message_HeaderSetU(&(actualMsgHeader.ldpMsg), U);
    LDP_Message_HeaderSetMsgType(&(actualMsgHeader.ldpMsg), Message_Type);

    actualMsgHeader.Message_Length = (unsigned short)
        (LDP_MESSAGE_ID_SIZE + Message_Length);

    actualMsgHeader.Message_Id = Message_Id;
    memcpy(msgHeaderPtr, &actualMsgHeader, sizeof(LDP_Message_Header));
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpReturnMyInterfaceAddresses()
//
// PURPOSE      : Returning Ip addresses of all interfaces of a given node
//
// PARAMETERS   : node - who's interfaces addresses to be returned.
//                myInterfaceAddresses - array of interface address that
//                                       will be returned from the function
//                numberOfAddresses - number of interfaces that will be
//                                    returned from the function.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpReturnMyInterfaceAddresses(
    Node*  node,
    MplsLdpApp* ldp,
    NodeAddress** myInterfaceAddresses,
    int* numberOfAddresses)
{
    unsigned int i = 0;
    *numberOfAddresses = ldp->numLocalAddr;

    *myInterfaceAddresses = (NodeAddress*)
        MEM_malloc(sizeof(NodeAddress) * ldp->numLocalAddr);

    for (i = 0 ; i < ldp->numLocalAddr; i++)
    {
        (*myInterfaceAddresses)[i] = ldp->localAddr[i];
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddPathVectorTLV()
//
// PURPOSE      : Adding Path Vector TLV on the top of the LDP message
//                existing in packet buffer.
//
// PARAMETERS   : buffer - pointer to the packet buffer
//                attribute - pointer to the Attribute_Type structure
//                            containing the attribute values
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddPathVectorTLV(
    PacketBuffer* buffer,
    Attribute_Type* attribute)
{
    // Allocate Path Vector TLV
    unsigned short length = (unsigned short) (attribute->pathVectorValue.
                                length * sizeof(NodeAddress));

    char* pathVector = NULL;
    LDP_TLV_Header tlvHeader;

    BUFFER_AddHeaderToPacketBuffer(buffer, length, (char**) &pathVector);

    memcpy(pathVector, attribute->pathVectorValue.pathVector, length);

    BUFFER_AddHeaderToPacketBuffer(buffer, sizeof(LDP_TLV_Header),
        (char**) &pathVector);

    LDP_TLV_HeaderSetU(&(tlvHeader.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(tlvHeader.tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(tlvHeader.tlvHdr), LDP_PATH_VECTOR_TLV);
    tlvHeader.Length = length;

    memcpy(pathVector, &tlvHeader, sizeof(LDP_TLV_Header));
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddHopCountTLV()
//
// PURPOSE      : Adding hop count TLV on the top of the LDP message
//                existing in packet buffer.
//
// PARAMETERS   : buffer - pointer to the packet buffer
//                attribute - pointer to the Attribute_Type structure
//                            containing the attribute values
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddHopCountTLV(
    PacketBuffer* buffer,
    Attribute_Type* attribute)
{
    // Allocate Hop Count TLV (if exists in attributes)

    LDP_HopCount_TLV_Value_Type hopCountTLV;
    char* pktPtr = NULL;
    LDP_TLV_Header tlvHeader;

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        SIZE_OF_HOP_COUNT_TLV,
        (char**) &pktPtr);

    hopCountTLV.hop_count_value = (char) attribute->numHop;

    memcpy(pktPtr, &hopCountTLV, SIZE_OF_HOP_COUNT_TLV);

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_TLV_Header),
        (char**) &pktPtr);

    LDP_TLV_HeaderSetU(&(tlvHeader.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(tlvHeader.tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(tlvHeader.tlvHdr), LDP_HOP_COUNT_TLV);
    tlvHeader.Length = SIZE_OF_HOP_COUNT_TLV;

    memcpy(pktPtr, &tlvHeader, sizeof(LDP_TLV_Header));
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddLabelTLV()
//
// PURPOSE      : Adding label TLV on the top of the LDP message
//                existing in packet buffer.
//
// PARAMETERS   : buffer - pointer to the packet buffer
//                label  - label value
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddLabelTLV(
    PacketBuffer* buffer,
    unsigned int label)
{
    char* pktPtr = NULL;
    LDP_TLV_Header tlvHeader;
    LDP_Generic_Label_TLV_Value_Type  incomingLabel;

    incomingLabel.Label = label;

    LDP_TLV_HeaderSetU(&(tlvHeader.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(tlvHeader.tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(tlvHeader.tlvHdr), LDP_GENERIC_LABEL_TLV);
    tlvHeader.Length = sizeof(LDP_Generic_Label_TLV_Value_Type);

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_Generic_Label_TLV_Value_Type),
        &pktPtr);

    memcpy(pktPtr, &incomingLabel, sizeof(LDP_Generic_Label_TLV_Value_Type));

    BUFFER_AddHeaderToPacketBuffer(buffer, sizeof(LDP_TLV_Header), &pktPtr);

    memcpy(pktPtr, &tlvHeader, sizeof(LDP_TLV_Header));
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsAddFecTLV()
//
// PURPOSE      : Adding fec TLV on the top of the LDP message
//                existing in packet buffer.
//
// PARAMETERS   : buffer - pointer to the packet buffer
//                tlvHeaderFec  - fec TLV header
//                fecTLVtype - fec TLV type
//                fec - the fec value.
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsAddFecTLV(
    PacketBuffer* buffer,
    LDP_TLV_Header tlvHeaderFec,
    LDP_Prefix_FEC_Element fecTLVtype,
    Mpls_FEC fec)
{

    char* fecData = NULL;
    char* bufferFec = NULL;
    char* headerFec = NULL;

    int varLength = (tlvHeaderFec.Length -  sizeof(LDP_Prefix_FEC_Element));

    // Address List
    BUFFER_AddHeaderToPacketBuffer(buffer, varLength, &fecData);
    memcpy(fecData, &(fec.ipAddress), varLength);


    // Add LDP_Prefix_FEC_Element
    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_Prefix_FEC_Element),
        (char**) &bufferFec);

    memcpy(bufferFec, &fecTLVtype, sizeof(LDP_Prefix_FEC_Element));

    // Add Fec TLV Header
    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_TLV_Header),
        (char**) &headerFec);

    memcpy(headerFec, &tlvHeaderFec, sizeof(LDP_TLV_Header));
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddExtendedStatusTLV()
//
// PURPOSE      : Adding ExtendedStatusTLV on the top of the LDP message
//                existing in packet buffer.
//
// PARAMETERS   : buffer - pointer to the packet buffer
//                status - status value
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddExtendedStatusTLV(
    PacketBuffer* buffer,
    unsigned char* status)
{
    char* extendedStatusPtr = NULL;
    char* headerPtr = NULL;
    LDP_TLV_Header headerTLV;

    LDP_TLV_HeaderSetU(&(headerTLV.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(headerTLV.tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(headerTLV.tlvHdr), LDP_EXTENDED_STATUS_TLV);
    headerTLV.Length = sizeof(LDP_Extended_Status_TLV_Type);

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_Extended_Status_TLV_Type),
        &extendedStatusPtr);

    memcpy(extendedStatusPtr, status, sizeof(LDP_Extended_Status_TLV_Type));

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_TLV_Header),
        &headerPtr);

    memcpy(headerPtr, &headerTLV, sizeof(LDP_TLV_Header));
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddReturnedPDUTLV()
//
// PURPOSE      : Adding Returned PDU TLV on the top of the LDP message
//                existing in packet buffer.
//
// PARAMETERS   : buffer - pointer to the packet buffer
//                sizeReturnedPDU - size of the Returned PDU TLV
//                returnedPDU - returned PDU TLV
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddReturnedPDUTLV(
    PacketBuffer* buffer,
    unsigned short sizeReturnedPDU,
    unsigned char* returnedPDU)
{
    char* returnPDUTLVptr = NULL;
    char* headerPtr = NULL;
    LDP_TLV_Header headerTLV;

    LDP_TLV_HeaderSetU(&(headerTLV.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(headerTLV.tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(headerTLV.tlvHdr), LDP_RETURNED_PDU_TLV);
    headerTLV.Length = sizeReturnedPDU;

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeReturnedPDU,
        &returnPDUTLVptr);

    memcpy(returnPDUTLVptr, returnedPDU, sizeReturnedPDU);

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_TLV_Header),
        &headerPtr);

    memcpy(headerPtr, &headerTLV, sizeof(LDP_TLV_Header));
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddReturnedMessageTLV()
//
// PURPOSE      : Adding Returned Message TLV on the top of the LDP message
//                existing in packet buffer.
//
// PARAMETERS   : buffer - pointer to the packet buffer
//                sizeReturnedMessage - size of the Returned Message TLV
//                returnedMessage - returned message TLV
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddReturnedMessageTLV(
    PacketBuffer* buffer,
    unsigned short sizeReturnedMessage,
    unsigned char* returnedMessage)
{
    char* returnedMessagePtr = NULL;
    char* headerPtr = NULL;
    LDP_TLV_Header headerTLV;

    LDP_TLV_HeaderSetU(&(headerTLV.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(headerTLV.tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(headerTLV.tlvHdr), LDP_RETURNED_MSG_TLV);
    headerTLV.Length = sizeReturnedMessage;

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeReturnedMessage,
        &returnedMessagePtr);
    memcpy(returnedMessagePtr, returnedMessage, sizeReturnedMessage);

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_TLV_Header),
        &headerPtr);

    memcpy(headerPtr, &headerTLV, sizeof(LDP_TLV_Header));
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddStatusTLV()
//
// PURPOSE      : Adding Status TLV on the top of the LDP message
//                existing in packet buffer.
//
// PARAMETERS   : buffer - pointer to the packet buffer
//                message Type - status TLV message type
//                referedMessage - unique id of the message to which this
//                                 status TLV message refers to
//                statusCode - status Code
//                notificationMsgType - notification message Type against
//                                      which this status TLV message is
//                                      generated.
//                forwardBit - is set to either TRUE or FALSE depending on
//                             whether this status TLV is to be forwarded
//                             or not.
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddStatusTLV(
    PacketBuffer* buffer,
    unsigned short messageType,
    unsigned int referedMessage,
    unsigned int statusCode,
    BOOL notificationMsgType,
    BOOL forwardBit)
{
    LDP_Status_TLV_Value_Type statusTLV;
    LDP_TLV_Header headerTLV;
    char* bufferPtr = NULL;
    char* headerPtr = NULL;

    LDP_Status_TLVSetE(&(statusTLV.statusValue), notificationMsgType);
    LDP_Status_TLVSetF(&(statusTLV.statusValue), forwardBit);
    LDP_Status_TLVSetStatus(&(statusTLV.statusValue), statusCode);
    statusTLV.message_id = referedMessage;
    statusTLV.message_type = messageType;

    LDP_TLV_HeaderSetU(&(headerTLV.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(headerTLV.tlvHdr), forwardBit);
    //earlier forwardBit typecasted to unsigned short
    LDP_TLV_HeaderSetType(&(headerTLV.tlvHdr), LDP_STATUS_TLV);
    headerTLV.Length = sizeof(LDP_Status_TLV_Value_Type);

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_Status_TLV_Value_Type),
        &bufferPtr);

    memcpy(bufferPtr, &statusTLV, sizeof(LDP_Status_TLV_Value_Type));

    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_TLV_Header),
        &headerPtr);

    memcpy(headerPtr, &headerTLV, sizeof(LDP_TLV_Header));
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpSendLinkHello()
//
// PURPOSE      : sending hello message to all the neighboring LSR's
//
// REFERENCE    : RFC 3036 Section 3.5.2
//
// PARAMETERS   : node - node which is sending the link hello
//                Message_Id - message identification number
//                LSR_Id - the LSR Id of the node which is sending the
//                         link hello message
//                holdTime - hold time interval.
//
// RETURN VALUE : none
//
// ASSUMPTION   : LSR's does not use Transport layer  optional
//                object (TLV) type, when it sends a hello
//                message to it's peer
//-------------------------------------------------------------------------
static
void MplsLdpSendLinkHello(
    Node* node,
    unsigned int Message_Id,
    NodeAddress LSR_Id,
    clocktype holdTime)
{
    int i = 0;
    LDP_Common_Hello_Parameters_TLV_Value_Type* helloParmValuePtr = NULL;
    LDP_TLV_Header* tlvHeader = NULL;
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    // Create a PacketBuffer with the Common Hello
    // Parameters TLV which is composed of the TLV
    // header + Hello Parameters value structure
    PacketBuffer* buffer =
    BUFFER_AllocatePacketBufferWithInitialHeader(
        sizeof(LDP_Common_Hello_Parameters_TLV_Value_Type),
        sizeof(LDP_TLV_Header),
        (ldp->maxPDULength
             - (sizeof(LDP_Common_Hello_Parameters_TLV_Value_Type)
             + sizeof(LDP_TLV_Header))),
        FALSE,
        (char**) &helloParmValuePtr,
        (char**) &tlvHeader);

    helloParmValuePtr->Hold_Time = ((unsigned short)(holdTime / SECOND));
    LDP_Common_Hello_Parameters_TLV_ValueSetT(
        &(helloParmValuePtr->lchpTlvValType),MPLS_LDP_LINK_HELLO);
    LDP_Common_Hello_Parameters_TLV_ValueSetR(
        &(helloParmValuePtr->lchpTlvValType),
        MPLS_LDP_NO_TARGETED_HELLO_REQUEST);
    LDP_Common_Hello_Parameters_TLV_ValueSetReserved(
        &(helloParmValuePtr->lchpTlvValType), 0);

    LDP_TLV_HeaderSetU(&(tlvHeader->tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(tlvHeader->tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(tlvHeader->tlvHdr),
        LDP_COMMON_HELLO_PARAMETERS_TLV);
    tlvHeader->Length = sizeof(LDP_Common_Hello_Parameters_TLV_Value_Type);

    // Add an LDP Message Header to this, to create
    // a complete LDP Hello Msg
    MplsLdpAddMessageHeader(
        buffer,
        (unsigned short) 0,
        (unsigned short) LDP_HELLO_MESSAGE_TYPE,
        (unsigned short) BUFFER_GetCurrentSize(buffer),
        Message_Id);

    MplsLdpAddPduHeader(buffer, LSR_Id, PLATFORM_WIDE_LABEL_SPACE_ID);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        NodeAddress destAddress =
            NetworkIpGetInterfaceBroadcastAddress(node, i);

        Message* msg = APP_UdpCreateMessage(
            node,
            NetworkIpGetInterfaceAddress(node, i),
            (short) APP_MPLS_LDP,
            destAddress,
            (short) APP_MPLS_LDP,
            TRACE_MPLS_LDP);

        APP_AddPayload(
            node,
            msg,
            BUFFER_GetData(buffer),
            BUFFER_GetCurrentSize(buffer));

        APP_UdpSend(node, msg);

        // increment number of UDP link hello messages send.
        INCREMENT_STAT_VAR(numUdpLinkHelloSend);

        if (MPLS_LDP_DEBUG)
        {
            printf("node %u is sending udp packet\n"
                   "source = %u  interface = %d dest = %u\n",
                   node->nodeId,
                   ldp->localAddr[i],
                   i,
                   destAddress);
        }
        ldp->currentMessageId++;
    }

    if (MPLS_LDP_DEBUG)
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);

        printf("#%d: MplsLdpSendLinkHello at %s\n"
               "\tCurrent Buffer Size for LDP Hello PDU is %u\n",
               node->nodeId,
               buf,
               BUFFER_GetCurrentSize(buffer));
    }
    BUFFER_FreePacketBuffer(buffer);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpSendInitializationMessage
//
// PURPOSE      : sending an initialization message to a peer LSR
//
// REFERENCE    : RFC 3036 Section 3.5.3
//
// PARAMETERS   : node - the sender node
//                LSR_Id - LSR Id of the node which is sending the message
//                keep alive time - the proposed keep alive time
//                message Id - unique message Id
//                helloAdjcency- pointer to the hello adjaceny to
//                               message is to be sent
//                tcpConnId - transport connection id
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpSendInitializationMessage(
    Node* node,
    clocktype keep_alive_time,
    unsigned int messageId,
    MplsLdpHelloAdjacency* helloAdjacency,
    unsigned int tcpConnId)
{
    LDP_Common_Session_TLV_Value_Type commonSessionValue;
    char* commonSessionValuePtr = NULL;
    LDP_TLV_Header* tlvHeader = NULL;
    PacketBuffer* buffer = NULL;

    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);
    LDP_Identifier_Type Receiver_Id = helloAdjacency->LSR2;

    // allocate buffer with space such that we have space for
    // (tlvHeader + commonSessionValue + an Initial header to be sent
    //  as a PDU)
    buffer = BUFFER_AllocatePacketBufferWithInitialHeader(
                 COMMON_SESSION_TLV_SIZE,
                 sizeof(LDP_TLV_Header),
                 (ldp->maxPDULength
                     - (COMMON_SESSION_TLV_SIZE + sizeof(LDP_TLV_Header))),
                  FALSE,
                 (char**) &commonSessionValuePtr,
                 (char**) &tlvHeader);

    // add LDP_Common_Session_TLV_Value_Type data
    commonSessionValue.protocol_version = 1;

    commonSessionValue.keepalive_time = (unsigned short)
        (keep_alive_time / SECOND);

    LDPCommonSessionTLVSetA(&(commonSessionValue.ldpCsTlvValType),
        ldp->labelAdvertisementMode) ;
    LDPCommonSessionTLVSetD(&(commonSessionValue.ldpCsTlvValType),
        ldp->loopDetection);
    LDPCommonSessionTLVSetReserved(&(commonSessionValue.ldpCsTlvValType),
        0);
    LDPCommonSessionTLVSetPVL(&(commonSessionValue.ldpCsTlvValType),
        (unsigned short)ldp->pathVectorLimit);

    commonSessionValue.max_pdu_length = (unsigned short) ldp->maxPDULength;

    // The two fields below are together receiver's Label Space Identifier.
    // RFC 3036 . Section 3.5.3.
    // The Initialization message carries both the LDP Identifier for the
    // sender's (active LSR's) label space and the LDP Identifier for the
    // receiver's (passive LSR's) label space. RFC 3036 .section 2.5.3
    commonSessionValue.ldp_identifier.LSR_Id = Receiver_Id.LSR_Id;

    commonSessionValue.ldp_identifier.label_space_id =
        Receiver_Id.label_space_id;

    // add LDP_TLV_Header data
    LDP_TLV_HeaderSetU(&(tlvHeader->tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(tlvHeader->tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(tlvHeader->tlvHdr),
        LDP_COMMON_SESSION_PARAMETER_TLV);
    tlvHeader->Length = COMMON_SESSION_TLV_SIZE;

    memcpy(commonSessionValuePtr,
        &commonSessionValue,
        COMMON_SESSION_TLV_SIZE);

    // add LDP header to it to complete a initialization message.
    MplsLdpAddMessageHeader(
        buffer,
        (unsigned short) 0,
        (unsigned short) LDP_INITIALIZATION_MESSAGE_TYPE,
        (unsigned short) BUFFER_GetCurrentSize(buffer),
        messageId);

    // submit it to hello adjacency buffer for sending it
    MplsLdpSubmitMessageToHelloAdjacencyBuffer(
        node,
        ldp,
        buffer,
        helloAdjacency);

    if (MPLS_LDP_DEBUG)
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);

        printf("#%d: MplsLdpSendInitializationMessage at %s\n"
               "through TCP connection Id %d\n"
               "Current Buffer Size for Initialization PDU is %d\n",
               node->nodeId,
               buf,
               tcpConnId,
               BUFFER_GetCurrentSize(buffer));
    }
    BUFFER_FreePacketBuffer(buffer);

    // increment the number of initializtion message send
    INCREMENT_STAT_VAR(numInitializationMsgSend);
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpSendKeepAliveMessage()
//
// PURPOSE      : Sending keep alive message.An LSR sends KeepAlive
//                Messages as part of a mechanism that monitors the
//                integrity of the LDP session transport connection.
//
// PARAMETERS   : node :- node which is sending keepalive message.
//                LSR_Id :- LSR Id of the node which is sending the message
//                messageId :- unique message Id
//                tcpConnId :- trnasport connection id.
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpSendKeepAliveMessage(
    Node*  node,
    unsigned int messageId,
    MplsLdpHelloAdjacency* helloAdjacency)
{
    PacketBuffer* buffer = NULL;
    char* notUsed = NULL;
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    // no optional or data part.Allocate space for header only
    buffer = BUFFER_AllocatePacketBuffer(
                 0,
                 ldp->maxPDULength,
                 FALSE,
                 &notUsed);

    // add LDP header to it to complete a keepalive message
    MplsLdpAddMessageHeader(
        buffer,
        (unsigned short) 0,
        (unsigned short) LDP_KEEP_ALIVE_MESSAGE_TYPE,
        (unsigned short) BUFFER_GetCurrentSize(buffer),
        messageId);

    MplsLdpSubmitMessageToHelloAdjacencyBuffer(
        node,
        ldp,
        buffer,
        helloAdjacency);

    if (MPLS_LDP_DEBUG)
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);

        printf("#%d: MplsLdpSendKeepAliveMessage at %s\n"
               "through connection Id %d\n"
               "Current Buffer Size of keepAlive PDU is %d\n",
               node->nodeId,
               buf,
               helloAdjacency->connectionId,
               BUFFER_GetCurrentSize(buffer));
    }
    BUFFER_FreePacketBuffer(buffer);

    // increment number of keep alive message sent.
    INCREMENT_STAT_VAR(numKeepAliveMsgSend);
}


//-------------------------------------------------------------------------
//
// FUNCTION     : MplsLdpSendAddressMessage()
//
// PURPOSE      : sending address messages.An LSR sends the Address
//                Message to an LDP peer to advertise its interface
//                addresses.Address List TLV :The list of interface
//                addresses being advertised by the sending LSR.
//                RFC 3036 Section : 3.5.5
//
// PARAMETERS   : node :- node which is sending keepalive message.
//                LSR_Id :- LSR Id of the node which is sending the message
//                messageId :- unique message Id
//                tcpConnId :- trnasport connection id.
//
// RETURN VALUE : none
//
//-------------------------------------------------------------------------
static
void MplsLdpSendAddressMessage(
    Node* node,
    unsigned int messageId,
    MplsLdpHelloAdjacency* helloAdjacency)
{
    int numberOfAddresses;
    unsigned short* TLV_mandatory_parameter = NULL;

    PacketBuffer* buffer = NULL;
    LDP_TLV_Header* tlvHeader = NULL;
    NodeAddress* myInterfaceAddresses = NULL;
    NodeAddress* addressList = NULL;

    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    // get all of your interface addresses to advertised.
    MplsLdpReturnMyInterfaceAddresses(
        node,
        ldp,
        &myInterfaceAddresses,
        &numberOfAddresses);

    // allocate buffer space for 'address list' and 'address family'.These
    // two together makes address list TLV type.RFC 3036 Section 3.4.3
    buffer = BUFFER_AllocatePacketBufferWithInitialHeader(
                 numberOfAddresses * sizeof(NodeAddress),
                 sizeof(unsigned short), 0, TRUE,
                 (char**) &addressList,
                 (char**) &TLV_mandatory_parameter);

    // copy the addresses
    memcpy(addressList,
        myInterfaceAddresses,
        (sizeof(NodeAddress) * numberOfAddresses));

    // assigne the value of mandatory fields
    *(TLV_mandatory_parameter) = IP_V4_ADDRESS_FAMILY;

    // add TLV_header complete the Address list TLV type.So
    // fragments of codes below alocates tlv header,and
    // assigns value to the header fields.
    BUFFER_AddHeaderToPacketBuffer(
        buffer,
        sizeof(LDP_TLV_Header),
        (char**) &tlvHeader);

    assert(tlvHeader);

    LDP_TLV_HeaderSetU(&(tlvHeader->tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(tlvHeader->tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(tlvHeader->tlvHdr), LDP_ADDRESS_LIST_TLV);

    tlvHeader->Length = (unsigned short)
        (sizeof(LDP_Address_List_TLV_Value_Header_Type)
         + sizeof(NodeAddress) * numberOfAddresses);

    // add LDP header to complete Address Message.
    MplsLdpAddMessageHeader(
        buffer,
        (unsigned short) 0,
        (unsigned short) LDP_ADDRESS_LIST_MESSAGE_TYPE,
        (unsigned short) BUFFER_GetCurrentSize(buffer),
        messageId);

    MplsLdpSubmitMessageToHelloAdjacencyBuffer(
        node,
        ldp,
        buffer,
        helloAdjacency);

    if (MPLS_LDP_DEBUG)
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);

        printf("#%u is sending addressMesage  no of interface is = %u\n"
               "#%u: MplsLdpSendAddressMessage at %s\n"
               "througt connectionId %d\n"
               "Current Buffer Size of Address Message PDU is %u\n"
               "tlvHeader->U %1d/tlvHeader->F %1d/tlvHeader->Type %d\n"
               "lvHeader->Length %d\n",
               node->nodeId,
               numberOfAddresses,
               node->nodeId,
               buf,
               helloAdjacency->connectionId,
               BUFFER_GetCurrentSize(buffer),
               LDP_TLV_HeaderGetU(tlvHeader->tlvHdr),
               LDP_TLV_HeaderGetF(tlvHeader->tlvHdr),
               LDP_TLV_HeaderGetType(tlvHeader->tlvHdr),
               tlvHeader->Length);
    }
    BUFFER_FreePacketBuffer(buffer);
    MEM_free(myInterfaceAddresses);

    // Increment number of address message send
    INCREMENT_STAT_VAR(numAddressMsgSend);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpSendLabelRequestMessage()
//
// PURPOSE      : sending a label request message to a peer
//
// PARAMETERS   : node - node which is sending the label request message
//                ldp - pointer to the MplsLdpApp structure.
//                peer - pointer to the peer-LSR's record in the hello
//                       adjacency list.
//                fec - fec for which label request message is to be sent
//                attribute - pointer to the label request message
//                            attribute values.
//                messageId -unique message Id.
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpSendLabelRequestMessage(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* peer,
    Mpls_FEC fec,
    Attribute_Type* attribute,
    unsigned int messageId)
{
    LDP_TLV_Header tlvHeaderFec;
    LDP_Prefix_FEC_Element fecTLVtype;
    PacketBuffer* buffer = NULL;
    char* notUsed = NULL;

    // Caluculate FEC Value field
    fecTLVtype.Element_Type = 3;
    fecTLVtype.Prefix.Address_Family = IP_V4_ADDRESS_FAMILY;
    fecTLVtype.Prefix.PreLen = (unsigned char) fec.numSignificantBits;

    // Calculate FEC header field
    LDP_TLV_HeaderSetU(&(tlvHeaderFec.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(tlvHeaderFec.tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(tlvHeaderFec.tlvHdr), LDP_FEC_TLV);
    tlvHeaderFec.Length = sizeof(LDP_Prefix_FEC_Element)
                          + sizeof(NodeAddress);
    //Allocate buffer space for packet manipulation
    buffer = BUFFER_AllocatePacketBuffer(
                 0,
                 sizeof(LDP_TLV_Header),
                 TRUE,
                 &notUsed);

    // if Loop detection enabled
    if ((ldp->loopDetection) && (attribute != NULL))
    {
        // Allocate Path Vector TLV (if exists in attributes)

        if (attribute->pathVector)
        {
            MplsLdpAddPathVectorTLV(buffer, attribute);
        }

        // Allocate Hop Count TLV (if exists in attributes)
        if (attribute->hopCount)
        {
            MplsLdpAddHopCountTLV(buffer, attribute);
        }
    }

    // Allocate FEC TLV
    MplsAddFecTLV(buffer, tlvHeaderFec, fecTLVtype, fec);

    MplsLdpAddMessageHeader(
        buffer,
        (unsigned short) 0,
        (unsigned short) LDP_LABEL_REQUEST_MESSAGE_TYPE,
        (unsigned short) BUFFER_GetCurrentSize(buffer),
        messageId);

    MplsLdpSubmitMessageToHelloAdjacencyBuffer(node, ldp, buffer, peer);

    if (MPLS_LDP_DEBUG)
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);

        printf("#%u:MplsLdpSendLabelRequestMessage at %s\n"
               "through TCP connection Id %u\n"
               "Current Buffer Size for Label Request PDU is %u\n",
               node->nodeId,
               buf,
               peer->connectionId,
               BUFFER_GetCurrentSize(buffer));
    }
    BUFFER_FreePacketBuffer(buffer);

    // record this out going request into out bound cache
    MplsLdpAddRowToOutboundLabelRequestCache(
        node,
        peer,
        fec,
        peer->LSR2.LSR_Id);

    // increment number of label request message send
    INCREMENT_STAT_VAR(numLabelRequestMsgSend);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpCopySAttibuteToRAttribute()
//
// PURPOSE      : copying SAttribute value to RAttributeValue
//
// PARAMETERS   : LSR_Id - LSR which is performing the above operation
//                RAttribute - received attribute value
//                SAttribute - attribute value which is to be sent
//
// RETURN VALUE :
//-------------------------------------------------------------------------
static
void MplsLdpCopySAttibuteToRAttribute(
    NodeAddress LSR_Id,
    Attribute_Type* RAttribute,
    Attribute_Type* SAttribute)
{
    // PMpA.19 Add LSR Id to beginning of Path Vector from RAttributes
    //         and copy the resulting Path Vector into SAttributes.
    //         Goto PMpA.21.
    int i = 0;
    NodeAddress* newPathVector = NULL;

    newPathVector = (NodeAddress*) MEM_malloc(sizeof(NodeAddress)
                        * (RAttribute->pathVectorValue.length + 1));

    newPathVector[0] = LSR_Id;

    for (i = 0; i < (signed int) RAttribute->pathVectorValue.length; i++)
    {
        newPathVector[1 + i] =
            RAttribute->pathVectorValue.pathVector[i];
    }

    MEM_free(SAttribute->pathVectorValue.pathVector);
    SAttribute->pathVectorValue.pathVector = NULL;

    SAttribute->pathVector = TRUE;

    SAttribute->pathVectorValue.length =
        (RAttribute->pathVectorValue.length + 1);

    SAttribute->pathVectorValue.pathVector = newPathVector;
}


//-------------------------------------------------------------------------
// FUNCTION     : IamEgressForThisFec()
//
// PURPOSE      : checks if a LSR is Egress LSR or not w.r.t
//                a given fec
//
// PARAMETERS   : node -which is performing the above operation
//                fec - fec to be matched
//                ldp - pointer to the MplsLdpApp structure
//
// RETURN VALUE : TRUE if the node is an Edge Route AND next hop is
//                not in the helloadjacency of the node.
//                OR the node is the destination itself.
//                FALSE otherwise
//-------------------------------------------------------------------------
static
BOOL IamEgressForThisFec(
    Node* node,
    Mpls_FEC fec,
    MplsLdpApp* ldp)
{
    // checkes whether I am egress router for this FEC

    MplsData* mpls = NULL;
    mpls = MplsReturnStateSpace(node);

    if (NetworkIpIsMyIP(node, fec.ipAddress))
    {
        return TRUE;
    }

    if (mpls->isEdgeRouter)
    {
             NodeAddress nextHopAddress = (NodeAddress) NETWORK_UNREACHABLE;

            if (IsNextHopExistForThisDest(node, fec.ipAddress,
                                          &nextHopAddress))
            {
                MplsLdpHelloAdjacency* peerSource = NULL;
                LDP_Identifier_Type notUsed;

                memset(&notUsed, 0, sizeof(LDP_Identifier_Type));
                peerSource =  MplsLdpReturnHelloAdjacency(
                                             ldp,
                                             nextHopAddress,
                                             notUsed,
                                             TRUE, //  match link address
                                             FALSE); // do not match LSR_Id

                // If the next hop from the IP forwarding table does NOT lie
                // in the hello adjacency of the node, then it will be Egress
                if (!peerSource)
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
                return FALSE;
            }

    }
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpRetrivePreviouslySentLabelMapping()
//
// PURPOSE      : Retriving a record to fecToLabelMappingRec cache
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec to match
//                LSR_Id - LSR_Id to match.                .
//                label - label which is sent.
//                hopCount - hop count entry
//
// RETURN VALUE : TRUE if label mapping has been sent or FALSE otherwise.
//-------------------------------------------------------------------------
static
BOOL MplsLdpRetrivePreviouslySentLabelMapping(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress LSR_Id,
    unsigned int* label,
    unsigned int* hopCount,
    Mpls_FEC *fecPtr,
    BOOL deleteRec)
{
    unsigned int i = 0;

    for (i = 0; i < ldp->fecToLabelMappingRecNumEntries; i ++)
    {
        if ((ldp->fecToLabelMappingRec[i].peer  == LSR_Id) &&
            (MatchFec(&(ldp->fecToLabelMappingRec[i].fec), &fec)))
        {
            *label = ldp->fecToLabelMappingRec[i].label;

            if (ldp->fecToLabelMappingRec[i].Attribute.hopCount)
            {
                *hopCount = ldp->fecToLabelMappingRec[i].Attribute.numHop;
            }
            else
            {
                *hopCount = 0;
            }

            // Copy and return the fec to the caller if needed.
            if (fecPtr != NULL)
            {
                memcpy(fecPtr, &ldp->fecToLabelMappingRec[i].fec,
                       sizeof(Mpls_FEC));
            }

            if (deleteRec)
            {
               // if user wants to deleate deleate it.
               if (i == (ldp->fecToLabelMappingRecNumEntries - 1))
               {
                   memset(&(ldp->fecToLabelMappingRec[i]), 0,
                          sizeof(FecToLabelMappingRec));
               }
               else
               {
                  int j = 0;
                  for (j = (i + 1);
                       j < (signed int) ldp->fecToLabelMappingRecNumEntries;
                       j++)
                   {
                      memcpy(&(ldp->fecToLabelMappingRec[j - 1]),
                             &(ldp->fecToLabelMappingRec[j]),
                             sizeof(FecToLabelMappingRec));
                   }
               }

               (ldp->fecToLabelMappingRecNumEntries)--;
            }
            return TRUE;
        }
    }

    if (fecPtr != NULL)
    {
        memset(fecPtr, 0, sizeof(Mpls_FEC));
    }
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpPrepareLabelMappingAttributes()
//
// PURPOSE      : prepair label mapping attributes before sending label
//                mapping message
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structure.
//                peer - pointer to peer LSR's record in the
//                       hello adjacency structure
//                fec - fec tobe send with the label mapping message
//                RAttribute - received attribute value
//                SAttribute - attribute to be sent with label
//                              mapping message
//                isPropagating - is label request prepaired to propagate
//                prevHopCount - prevoius hop count.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpPrepareLabelMappingAttributes(
    Node*  node,
    MplsLdpApp*  ldp,
    MplsLdpHelloAdjacency* peer,
    Mpls_FEC  fec,
    Attribute_Type* RAttribute,
    Attribute_Type* SAttribute,
    BOOL isPropagating,
    unsigned int prevHopCount)
{
    // A.2.8.  MplsLdpPrepareLabelMappingAttributes
    //
    // Summary:
    //
    // This procedure is used whenever a Label Mapping is to be sent to a
    // Peer to compute the Hop Count and Path Vector, if any, to include
    // in the message.
    //
    // Parameters:
    //
    // -  Peer.  The LDP peer to which the message is to be sent.
    //
    // -  FEC.  The FEC for which a label request is to be sent.
    //
    // -  RAttributes.  The attributes this LSR associates with the LSP
    //    for FEC.
    //
    // -  SAttributes.  The attributes to be included in the Label
    //    Mapping message.
    //
    // -  IsPropagating.  The LSR is sending the Label Mapping message to
    //    propagate one received from the FEC next hop.
    //
    // -  PrevHopCount.  The Hop Count, if any, this LSR associates with
    //    the LSP for the FEC.

    unsigned int labelNotUsed = 0;

    if ((peer->hopCountRequird) ||
        (ldp->loopDetection))
    {

        // PMpA.2  Is LSR egress for FEC?
        //         If not, goto PMpA.4.
        if (IamEgressForThisFec(node, fec, ldp))
        {
            //  PMpA.3  Include Hop Count of 1 in SAttributes.
            //          Goto PMpA.21.
            SAttribute->hopCount = TRUE;
            SAttribute->numHop = 1;
            // done PMpA.21
            return;
        }
    }

    ERROR_Assert(SAttribute && RAttribute, "Error in"
        " MplsLdpPrepareLabelMappingAttributes()"
        " SAttribute and/or RAttribute == NULL, and shouldn't be !!!");


    // PMpA.1  Is Hop Count required for this Peer (see Note 1.) ? OR
    //         Do RAttributes include a Hop Count? OR
    //         Is Loop Detection configured on LSR?
    //         If not, goto PMpA.21.
    if ((peer->hopCountRequird) ||
        (RAttribute->hopCount) ||
        (ldp->loopDetection))
    {
        // PMpA.4  Do RAttributes have a Hop Count?
        //          If not, goto PMpA.8.
        if (RAttribute->hopCount)
        {
            // PMpA.5  Is LSR member of edge set for an LSR domain
            //         whose LSRs do not perform TTL decrement AND
            //         Is Peer in that domain (See Note 2.).
            //         If not, goto PMpA.7
            if ((ldp->decrementsTTL) && (ldp->member_Dec_TTL_Domain))
            {
                // PMpA.6  Include Hop Count of 1 in SAttributes.
                //         Goto PMpA.9.
                SAttribute->hopCount = TRUE;
                SAttribute->numHop = 1;
            }
            else
            {
                // PMpA.7  Increment RAttributes Hop Count and
                //         copy the resulting Hop Count to SAttributes.
                //         See Note 2.  Goto PMpA.9.
                SAttribute->hopCount = TRUE;
                RAttribute->numHop = RAttribute->numHop + 1;
                SAttribute->numHop = RAttribute->numHop;
            }
        }
        else
        {
            // PMpA.8  Include Hop Count of unknown (0) in SAttributes
            SAttribute->numHop = MPLS_UNKNOWN_NUM_HOPS;
        }

        // PMpA.9  Is Loop Detection configured on LSR?
        //         If not, goto PMpA.21.
        if (!(ldp->loopDetection))
        {
            // done PMpA.21
            return;
        }

        // PMpA.10 Do RAttributes have a Path Vector?
        //         If so, goto PMpA.19.
        if (RAttribute->pathVector)
        {
            // MpA.19 Add LSR Id to beginning of Path Vector from
            //        RAttributes and copy the resulting Path Vector into
            //        SAttributes. Goto PMpA.21.
            MplsLdpCopySAttibuteToRAttribute(
                ldp->LSR_Id,
                RAttribute,
                SAttribute);

            return;
        }

        // PMpA.11 Is LSR propagating a received Label Mapping?
        //         If not, goto PMpA.20.
        if (isPropagating)
        {
           // PMpA.12 Does LSR support merging?
           //         If not, goto PMpA.14.
           if (ldp->supportLabelMerging)
           {
                unsigned int labelSent = 0;
                unsigned int hopCount = 0;

                // PMpA.13 Has LSR previously sent a Label Mapping for
                //         FEC to Peer? If not, goto PMpA.20.
                if (!MplsLdpRetrivePreviouslySentLabelMapping(
                         ldp,
                         fec,
                         peer->LSR2.LSR_Id,
                         &labelSent,
                         &hopCount,
                         NULL,
                         FALSE))
                {
                    // PMpA.20 Include Path Vector of length 1 containing
                    //         LSR Id in SAttributes.
                    SAttribute->pathVector = TRUE;
                    SAttribute->pathVectorValue.length = 1;

                    return;
                }
            }

            // PMpA.14 Do RAttributes include a Hop Count?
            //         If not, goto PMpA.21.
            if (!(RAttribute->hopCount))
            {
                //  PMpA.21 done
                return;
            }

            // PMpA.15 Is Hop Count in Rattributes unknown(0)?
            //        If so, goto PMpA.20.
            if (RAttribute->numHop == MPLS_UNKNOWN_NUM_HOPS)
            {
                // PMpA.20 Include Path Vector of length 1 containing
                //         LSR Id in SAttributes.
                SAttribute->pathVector = TRUE;
                SAttribute->pathVectorValue.length = 1;

                return;
            }

            // PMpA.16 Has LSR previously sent a Label Mapping for FEC to
            //         Peer? If not goto PMpA.21.
            if (!MplsLdpRetrivePreviouslySentLabelMapping(
                     ldp,
                     fec,
                     peer->LSR2.LSR_Id,
                     &labelNotUsed,
                     &prevHopCount,
                     NULL,
                     FALSE))
            {
                return;
            }

            // PMpA.17 Is Hop Count in RAttributes different from
            //         PrevHopCount ? If not goto PMpA.21.
            if (RAttribute->numHop != prevHopCount)
            {
                return;
            }

            // PMpA.18 Is the Hop Count in RAttributes > PrevHopCount?
            //         OR   Is PrevHopCount unknown(0)
            //         If not, goto PMpA.21.
            if ((RAttribute->numHop > prevHopCount) ||
                (prevHopCount == MPLS_UNKNOWN_NUM_HOPS))
            {
                return;
            }

            // PMpA.19 Add LSR Id to beginning of Path Vector from
            //        RAttributes and copy the resulting Path Vector
            //        into SAttributes. Goto PMpA.21.
            MplsLdpCopySAttibuteToRAttribute(
                ldp->LSR_Id,
                RAttribute,
                SAttribute);
        }
        else
        {
             // PMpA.20 Include Path Vector of length 1 containing LSR Id
             //         in SAttributes.
             SAttribute->pathVector = TRUE;
             SAttribute->pathVectorValue.length = 1;
        }
    }
    else
    {
        // done PMpA.21
        return;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpSendLabelMappingMessage()
//
// PURPOSE      : sending a label mapping message to a peer
//
// PARAMETERS   : node - node which is sending the label mapping message
//                ldp - pointer to the MplsLdpApp structure.
//                peer - pointer to the peer-LSR's record in the hello
//                       adjacency list.
//                fec - fec for which label mapping message is to be sent
//                label - label to be sent
//                SAttribute - pointer to the label mapping message
//                            attribute values.
//                messageId -unique message Id.
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpSendLabelMappingMessage(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* peer,
    Mpls_FEC fec,
    unsigned int label,
    Attribute_Type* SAttribute,
    unsigned int messageId)
{
    LDP_Prefix_FEC_Element fecTLVtype;
    LDP_TLV_Header tlvHeaderFec;
    PacketBuffer* buffer = NULL;
    char* notUsed = NULL;

    fecTLVtype.Element_Type = 3;
    fecTLVtype.Prefix.Address_Family = IP_V4_ADDRESS_FAMILY;
    fecTLVtype.Prefix.PreLen = (unsigned char) fec.numSignificantBits;

    // Calculate FEC header field
    LDP_TLV_HeaderSetU(&(tlvHeaderFec.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(tlvHeaderFec.tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(tlvHeaderFec.tlvHdr), LDP_FEC_TLV);
    tlvHeaderFec.Length = sizeof(LDP_Prefix_FEC_Element)
                          + sizeof(NodeAddress);

    buffer = BUFFER_AllocatePacketBuffer(
                 0,
                 sizeof(LDP_TLV_Header),
                 TRUE,
                 (char**) &notUsed);

    // build up the optional fields . The optional fields include :
    //
    //     Type                         length
    // ------------------------------------------------------------
    // 1) Label Request Message Id TLV   4         bytes
    // 2) Hop Count TLV                  1         bytes
    // 3) Path vector TLV              variable
    //
    // RFC 3036 Section 3.5.7 page no 66

    if ((SAttribute) && (ldp->loopDetection))
    {
        // Allocate Path Vector TLV (if exists in attributes)
        if (SAttribute->pathVector)
        {
            MplsLdpAddPathVectorTLV(buffer, SAttribute);
        }

        // Allocate Hop Count TLV (if exists in attributes)
        if (SAttribute->hopCount)
        {
            MplsLdpAddHopCountTLV(buffer, SAttribute);
        }
    }

    // Allocate Label TLV
    MplsLdpAddLabelTLV(buffer, label);

    // allocate FEC tlv on top
    MplsAddFecTLV(buffer, tlvHeaderFec, fecTLVtype, fec);

    MplsLdpAddMessageHeader(
        buffer,
        (unsigned short) 0,
        (unsigned short) LDP_LABEL_MAPPING_MESSAGE_TYPE,
        (unsigned short) BUFFER_GetCurrentSize(buffer),
        messageId);

    MplsLdpSubmitMessageToHelloAdjacencyBuffer(node, ldp, buffer, peer);

    if (MPLS_LDP_DEBUG)
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);

        printf("#%d: MplsLdpSendLabelMappingMessage at %s"
               "through TCP connection Id %d"
               "Current Buffer Size for Label Mapping PDU is %d"
               "Message Id is  %d\n",
               node->nodeId,
               buf,
               peer->connectionId,
               BUFFER_GetCurrentSize(buffer),
               messageId);
    }
    BUFFER_FreePacketBuffer(buffer);

    MplsLdpAddRowToOutboundLabelMappingCache(
        node,
        peer,
        fec,
        label,
        peer->LSR2.LSR_Id);

    // increment number of Label Mapping Send
    INCREMENT_STAT_VAR(numLabelMappingSend);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpSendLabelReleaseOrWithdrawMessage()
//
// PURPOSE      : sending a label release or withdraw message.
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structur
//                peerSource - pointer to peer LSR's record in the
//                             hello adjacency structure
//                label - label to be send with label release or
//                        withdraw message
//                fec - fec tobe send with the label request message
//                loopDetectionStatus -  loop Detection Status
//                addOptionalLabel -
//                messageId -  unique message Id
//                messageType - indicates if it is Label release or
//                              label withdraw message
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpSendLabelReleaseOrWithdrawMessage(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* peerSource,
    unsigned int label,
    Mpls_FEC fec,
    BOOL loopDetectionStatus,
    BOOL addOptionalLabel,
    unsigned int messageId,
    int messageType)
{
    PacketBuffer* buffer = NULL;
    char* notUsed = NULL;
    LDP_Prefix_FEC_Element fecTLVtype;
    LDP_TLV_Header tlvHeaderFec;

    buffer = BUFFER_AllocatePacketBuffer(
                 0,
                 sizeof(LDP_TLV_Header),
                 TRUE,
                 (char**) &notUsed);

    fecTLVtype.Element_Type = 3;
    fecTLVtype.Prefix.Address_Family = IP_V4_ADDRESS_FAMILY;
    fecTLVtype.Prefix.PreLen = (unsigned char) fec.numSignificantBits;

    // Calculate FEC header field
    LDP_TLV_HeaderSetU(&(tlvHeaderFec.tlvHdr), 0);
    LDP_TLV_HeaderSetF(&(tlvHeaderFec.tlvHdr), 0);
    LDP_TLV_HeaderSetType(&(tlvHeaderFec.tlvHdr), LDP_FEC_TLV);
    tlvHeaderFec.Length = sizeof(LDP_Prefix_FEC_Element)
                          + sizeof(NodeAddress);

    if (addOptionalLabel)
    {
       MplsLdpAddLabelTLV(buffer, label);
    }

    MplsAddFecTLV(buffer, tlvHeaderFec, fecTLVtype, fec);

    if (messageType == LDP_LABEL_RELEASE_MESSAGE_TYPE)
    {
        MplsLdpAddMessageHeader(
            buffer,
            (unsigned short) 0,
            (unsigned short) LDP_LABEL_RELEASE_MESSAGE_TYPE,
            (unsigned short) BUFFER_GetCurrentSize(buffer),
            messageId);

        // increment number of label withdraw message send
        INCREMENT_STAT_VAR(numLabelReleaseMsgSend);

    }
    else if (messageType == LDP_LABEL_WITHDRAW_MESSAGE_TYPE)
    {
        MplsLdpAddMessageHeader(
            buffer,
            (unsigned short) 0,
            (unsigned short) LDP_LABEL_WITHDRAW_MESSAGE_TYPE,
            (unsigned short) BUFFER_GetCurrentSize(buffer),
            messageId);

        // increment number of label release message send
        INCREMENT_STAT_VAR(numLabelWithdrawMsgSend);
    }
    else
    {
        ERROR_Assert(FALSE, "Error in"
            " MplsLdpSendLabelReleaseOrWithdrawMessage()"
            " not label release or withdraw message !!!");
    }

    MplsLdpSubmitMessageToHelloAdjacencyBuffer(
        node,
        ldp,
        buffer,
        peerSource);

    BUFFER_FreePacketBuffer(buffer);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpRemoveLabelForForwardingSwitchingUse()
//
// PURPOSE      : removing a label for forwarding and switching use.
//
// PARAMETERS   : node - node which is removing the label for
//                       forwarding and switching use
//                fec - the FEC to be matched
//                label - The label to be removed.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpRemoveLabelForForwardingSwitchingUse(
    Node* node,
    NodeAddress nextHop,
    unsigned int label,
    unsigned int inOut,
    Mpls_FEC fec)
{
    MplsData* mpls = NULL;
    MplsLdpApp* ldp = NULL;

    mpls = MplsReturnStateSpace(node);

    ERROR_Assert(mpls, "Error in"
        " MplsLdpRemoveLabelForForwardingSwitchingUse()"
        " MplsData is NULL !!!");

    ldp = MplsLdpAppGetStateSpace(node);

    ERROR_Assert(ldp, "Error in "
        " MplsLdpRemoveLabelForForwardingSwitchingUse()"
        " MplsLdpApp is NULL !!!");

    MplsLdpRemoveILMEntry(mpls, nextHop, label, inOut);
    MplsRemoveFTNEntry(mpls, fec.ipAddress, label);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpPerformLsrLabelReleaseProcedure()
//
// PURPOSE      : perfrom label release.
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec to be released
//                peer - pointer to peer LSR's record in the
//                       hello adjacency structure
//                label - label to be released
//                RAttribute - received attribute value
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpPerformLsrLabelReleaseProcedure(
    Node* node,
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    MplsLdpHelloAdjacency* peer,
    unsigned int label,
    Attribute_Type* RAttribute)
{
    if (ldp->labelRetentionMode == LABEL_RETENTION_MODE_LIBERAL)
    {
        MplsLdpAddRowToIncomingFecToLabelMappingRec(
            ldp,
            fec,
            peer,
            label,
            RAttribute);
    }
    else if (ldp->labelRetentionMode == LABEL_RETENTION_MODE_CONSERVATIVE)
    {
        MplsLdpSendLabelReleaseOrWithdrawMessage(
            node,
            ldp,
            peer,
            label,
            fec,
            LOOP_DETECTED,
            TRUE,
            ldp->currentMessageId,
            LDP_LABEL_RELEASE_MESSAGE_TYPE);

        // send the pdu

        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            peer);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpInstallLabelForForwardingAndSwitchingUse()
//
// PURPOSE      : Installing label for forwarding and switching use
//                in the ILM table
//
// PARAMETERS   : node - node which is performing the above operation
//                fec - fec to be installed orwarding and switching use
//                nextHopAdde - next hop from which this label mapping
//                              has been received
//                label - label to be installed orwarding and switching use
//                type - type of the label INCOMING or OUTGOING
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpInstallLabelForForwardingAndSwitchingUse(
    Node* node,
    Mpls_FEC fec,
    NodeAddress nextHopAddr,
    unsigned int label,
    int type)
{
    NodeAddress ipAddress = fec.ipAddress;
    int numSignificantBits = fec.numSignificantBits;
    unsigned int labelStack = label;
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    if (MPLS_LDP_DEBUG)
    {
        if (IamEgressForThisFec(node, fec, ldp))
        {
            printf("\tILM entry for ipAddress %d"
                   " / nextHop %d label %d\n",
                   ipAddress,
                   nextHopAddr,
                   label);
        }
        else
        {
            printf("\tFTN entry for ipAddress %d"
                   " / nextHop %d label %d\n",
                   ipAddress,
                   nextHopAddr,
                   label);
        }
    }

    if (IamEgressForThisFec(node, fec, ldp))
    {
        unsigned int outLabel = Implicit_NULL_Label;

        if (ldp->labelDistributionControlMode ==
                   MPLS_CONTROL_MODE_INDEPENDENT)
        {
            // for CONTROL MODE INDEPENDENT
            AssignLabelForControlModeIndependent(
                node,
                ipAddress,
                numSignificantBits,
                nextHopAddr,
                POP_LABEL_STACK,
                &labelStack,
                &outLabel,
                TRUE,
                FALSE,
                1); // 1 = number of labels
        }
        else // if (control mode is ORDERED)
        {
            // for CONTROL MODE ORDERED
            MplsCheckAndAssignLabelToILM(
                node,
                ipAddress,
                numSignificantBits,
                nextHopAddr,
                POP_LABEL_STACK,
                &labelStack,
                &outLabel,
                TRUE,
                TRUE,
                1); // 1 = number of labels

            MplsAssignFECtoOutgoingLabelAndInterface(
                node,
                ipAddress,
                numSignificantBits,
                nextHopAddr,
                FIRST_LABEL,
                &outLabel,
                1); // 1 = number of labels
        }
    }
    else // if (!IamEgressForThisFec(node, fec, ldp))
    {
        if (type == INCOMING)
        {
            unsigned int outLabel = Implicit_NULL_Label;

            // This assigns only in comming label
            // but not outgoing.
            if (ldp->labelDistributionControlMode ==
                   MPLS_CONTROL_MODE_INDEPENDENT)
            {
                // for CONTROL MODE INDEPENDENT
                AssignLabelForControlModeIndependent(
                    node,
                    ipAddress,
                    numSignificantBits,
                    nextHopAddr,
                    REPLACE_LABEL,
                    &labelStack,
                    &outLabel,
                    TRUE,
                    FALSE,
                    1); // 1 = number of labels

            }
            else  // if (control mode is ORDERED)
            {
                // for CONTROL MODE ORDERED
                MplsCheckAndAssignLabelToILM(
                    node,
                    ipAddress,
                    numSignificantBits,
                    nextHopAddr,
                    REPLACE_LABEL,
                    &labelStack,
                    &outLabel,
                    TRUE,
                    FALSE,
                    1); // 1 = number of labels
            }
        }
        else if (type == OUTGOING)
        {
            unsigned int inLabel =  Implicit_NULL_Label;

            if (ldp->labelDistributionControlMode ==
                   MPLS_CONTROL_MODE_INDEPENDENT)
            {
                // for CONTROL MODE INDEPENDENT
                AssignLabelForControlModeIndependent(
                    node,
                    ipAddress,
                    numSignificantBits,
                    nextHopAddr,
                    REPLACE_LABEL,
                    &inLabel,
                    &labelStack,
                    FALSE,
                    TRUE,
                    1); // 1 = number of labels
            }
            else // if (control mode is ORDERED)
            {
                // for CONTROL MODE ORDERED
                MplsCheckAndAssignLabelToILM(
                    node,
                    ipAddress,
                    numSignificantBits,
                    nextHopAddr,
                    REPLACE_LABEL,
                    &inLabel,
                    &labelStack,
                    FALSE,
                    TRUE,
                    1); // 1 = number of labels

                MplsAssignFECtoOutgoingLabelAndInterface(
                    node,
                    ipAddress,
                    numSignificantBits,
                    nextHopAddr,
                    FIRST_LABEL,
                    &labelStack,
                    1); // 1 = number of labels
            }
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpPerformLabelUseProcedure()
//
// PURPOSE      : installing label for ILM use
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structure.
//                fec - fec tobe send with the label request message
//                nextHopAddr - next hop from which a label mapping
//                              is received
//                labelSentToUpstreamPeer - label sent to upstream peer
//                labelFromNextHop - label received from next hop
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpPerformLabelUseProcedure(
    Node* node,
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress nextHopAddr,
    unsigned int labelSentToUpstreamPeer,
    unsigned int labelFromNextHop)
{
    unsigned int outLabel = labelFromNextHop;
    unsigned int inLabel = labelSentToUpstreamPeer;

    //  For Use Immediate OR
    //  Use If Loop Not Detected
    //  Install label sent to MsgSource and label from Next
    //  Hop if (LSR is not egress) for forwarding and switching use.

    Mpls_NHLFE_Operation opcode = (IamEgressForThisFec(node, fec, ldp)) ?
                         POP_LABEL_STACK : REPLACE_LABEL;

    // NOTE : The ERROR_Assert() statement can be un-commented
    //        below for debugging. Under normal circumstances,
    //        both "inLabel" and "outLabel" SHOULD NOT BE NULL.
    //
    // ERROR_Assert(inLabel && outLabel, "Error in "
    //              " MplsLdpPerformLabelUseProcedure()"
    //              " both in label and out label is NULL !!!");

    if (IamEgressForThisFec(node, fec, ldp) || !(inLabel && outLabel))
    {
        return;
    }

    // We deal the case when the node is not egress.
    // When node is an egress case is delt in
    // Install_Label ... function
    // This is the case when a LSR already has label for
    // a given fec;

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d : is performing label use procedure\n"
               "For the fec = %u And next hop == %d\n"
               "inLabel = %d / outLabel =%d\n",
               node->nodeId,
               fec.ipAddress,
               nextHopAddr,
               labelSentToUpstreamPeer,
               labelFromNextHop);
    }

    if (ldp->labelDistributionControlMode ==
            MPLS_CONTROL_MODE_INDEPENDENT)
    {
        AssignLabelForControlModeIndependent(
            node,
            fec.ipAddress,
            fec.numSignificantBits,
            nextHopAddr,
            opcode,
            &inLabel,
            &outLabel,
            TRUE,
            TRUE,
            1); // 1 = number of labels
    }
    else
    {
        MplsCheckAndAssignLabelToILM(
            node,
            fec.ipAddress,
            fec.numSignificantBits,
            nextHopAddr,
            opcode,
            &inLabel,
            &outLabel,
            TRUE,
            TRUE,
            1); // 1 = number of labels

        MplsAssignFECtoOutgoingLabelAndInterface(
            node,
            fec.ipAddress,
            fec.numSignificantBits,
            nextHopAddr,
            FIRST_LABEL,
            &outLabel,
            1); // 1 = number of labels
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : IsNextHopExistForThisDest()
//
// PURPOSE      : to check if a next hop exist for a given destination
//                address "destAddr". This next hop will be supplied from
//                Network Forwarding Table
//
// PARAMETERS   : node - the node which is checking the Network
//                       Forwarding Table.
//                destId - the destination address for which next hop
//                         is required.
//                nextHopAddress - The return value of nextHopAddress.
//
// RETURN VALUE : TRUE if a valid next hop exists, FALSE otherwise
//-------------------------------------------------------------------------
static
BOOL IsNextHopExistForThisDest(
    Node* node,
    NodeAddress destAddr,
    NodeAddress* nextHopAddress)
{
    int interfaceIndex = NETWORK_UNREACHABLE;
    BOOL found = FALSE;
    int i = 0;

    // match if it matches any of my interface address
    for (i = 0; i < node->numberInterfaces; i++)
    {
       if (NetworkIpGetInterfaceAddress(node, i) == destAddr)
       {
           *nextHopAddress = destAddr;
           return TRUE;
       }
    }

    // else if it is not matching any of my interface
    // adresses, then search network forwarding table
    NetworkGetInterfaceAndNextHopFromForwardingTable(
        node,
        destAddr,
        &interfaceIndex,
        nextHopAddress);

    if (*nextHopAddress == 0)
    {
        // next hop address "0" means next hop
        // itself is the destination address
        *nextHopAddress = destAddr;
    }

    if ((interfaceIndex == NETWORK_UNREACHABLE) ||
        (*nextHopAddress == (unsigned) NETWORK_UNREACHABLE))
    {
        found = FALSE;
    }
    else
    {
        found = TRUE;
    }
    return found;
}


//-------------------------------------------------------------------------
// FUNCTION     : PerfromLsrLabelUseProcedure()
//
// PURPOSE      : perfrom label distribution procedure.
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structure.
//                peer - pointer to peer LSR's record in the
//                       hello adjacency structure
//                fec - fec
//                RAttribute - received attribute value
//                labelSentToUpstreamPeer - label sent to upstream peer
//                label_received - label received
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void PerfromLsrLabelUseProcedure(
    Node* node,
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    unsigned int labelSentToUpstreamPeer,
    unsigned int label_received)
{
    //  For Use Immediate OR
    //  For Use If Loop Not Detected
    unsigned int i = 0;

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d in LSR_label use procedure:\n"
               "label received = %d / label sent = %d\n",
               node->nodeId,
               label_received,
               labelSentToUpstreamPeer);
    }

    // 1. Iterate through step 3 for each label mapping or
    //    FEC previously sent to Peer
    for (i = 0; i < ldp->fecToLabelMappingRecNumEntries; i++)
    {
        if (MatchFec(&(ldp->fecToLabelMappingRec[i].fec), &fec))
        {
            unsigned int nextHop;

            //  2. Install label received and label sent to peer for
            //     forwarding/switching use.
            ERROR_Assert(IsNextHopExistForThisDest(node,
                             fec.ipAddress,
                             &nextHop),
                "Error in PerfromLsrLabelUseProcedure()"
                " next hop do not exists !!!");

            MplsLdpPerformLabelUseProcedure(
                node,
                ldp,
                fec,
                nextHop,
                ldp->fecToLabelMappingRec[i].label,
                label_received);
        }
        // 3. End of Itaration from Step 1.
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : IsItDuplicateOrPendingRequest()
//
// PURPOSE      : checks whether a label request message is a pending
//                label request or not.if there is any pending request
//                upsteram from which label request message was received
//                returned back.
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec to be matched
//                LSR_Id - LSR_Id to match
//                upstream - upstream node address to be returned
//
// RETURN VALUE : TRUE if any matching pending request
//-------------------------------------------------------------------------
static
BOOL IsItDuplicateOrPendingRequest(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress LSR_Id,
    NodeAddress* upstream,
    BOOL matchFec)
{
    // check in pending Request structure.
    // will be replaced by general serch.
    unsigned int i = 0;
    BOOL found = FALSE;

    if (matchFec == FALSE)
    {
        for (i = 0; i < ldp->pendingRequestNumEntries; i++)
        {
            if (ldp->pendingRequest[i].nextHopLSR_Id == LSR_Id)
            {
               found = TRUE;
               *upstream = ldp->pendingRequest[i].sourceLSR_Id;
               break;
            }
        }
    }
    else if (matchFec == TRUE)
    {
        for (i = 0; i < ldp->pendingRequestNumEntries; i++)
        {
            if ((MatchFec(&(ldp->pendingRequest[i].fec), &fec)) &&
               (ldp->pendingRequest[i].nextHopLSR_Id == LSR_Id))
            {
                found = TRUE;
                *upstream = ldp->pendingRequest[i].sourceLSR_Id;
                break;
            }
        }
    }
    return found;
}


//-------------------------------------------------------------------------
// FUNCTION     : IsItOutstandingLabelRequestPreviouslySentForFEC()
//
// PURPOSE      : to check if a Label request message for the
//                FEC "fec", has been sent to the LSR with
//                LSR_Id "msgSource".
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec entry to be search for
//                msgSource - LSR_Id of peer LSR
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
BOOL IsItOutstandingLabelRequestPreviouslySentForFEC(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress msgSource)
{
    unsigned int i = 0;
    BOOL found = FALSE;

    for (i = 0; i < ldp->outGoingRequestNumEntries; i++)
    {
        if ((MatchFec(&ldp->outGoingRequest[i].fec, &fec)) &&
            (ldp->outGoingRequest[i].LSR_Id == msgSource))
        {
            found = TRUE;
            break;
        }
    }
    return found;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpDeleteRowFromOutstandingLabelMappingList()
//
// PURPOSE      : to delete record from the fecToLabelMappingRec
//                cache whoes FEC is equal to "fec"
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec entry to be search for
//                msgSource - LSR_Id of peer LSR
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
BOOL MplsLdpDeleteRowFromOutstandingLabelRequestList(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress msgSource)
{
    int i = 0;
    BOOL found = FALSE;
    int index = -1;

    for (i = 0; i < (signed) ldp->outGoingRequestNumEntries; i++)
    {
        if ((MatchFec(&(ldp->outGoingRequest[i].fec), &fec)) &&
            (ldp->outGoingRequest[i].LSR_Id == msgSource))
        {
            index = i;
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        int numEntries = ldp->outGoingRequestNumEntries;

        MplsLdpFreeAttributeType(&(ldp->outGoingRequest[index].Attribute));

        if (index == (numEntries - 1))
        {
            memset(&(ldp->outGoingRequest[index]), 0, sizeof(StatusRecord));
        }
        else
        {
            for (i = (index + 1); i < numEntries; i++)
            {
                memcpy(&(ldp->outGoingRequest[i - 1]),
                    &(ldp->outGoingRequest[i]),
                    sizeof(StatusRecord));
            }

            memset(&(ldp->outGoingRequest[numEntries - 1]),
                0,
                sizeof(StatusRecord));
        }
        (ldp->outGoingRequestNumEntries)--;
    }
    return found;
}


//-------------------------------------------------------------------------
// FUNCTION     : IfPathVectorIncludeLSRId()
//
// PURPOSE      : checking if the received attribute value contains the
//                given lsrId or not
//
// PARAMETERS   : nodeAddress - path vector.
//                pathVectorLength - path vector length
//                lsrId - lsrId to be searched for.
//
// RETURN VALUE : loop detection status
//-------------------------------------------------------------------------
static
BOOL IfPathVectorIncludeLSRId(
    NodeAddress nodeAddress[],
    int pathVectorLength,
    NodeAddress lsrId)
{
    int numEntries = pathVectorLength;
    int i = 0;

    for (i = 0; i < numEntries; i++)
    {
        if (nodeAddress[i] == lsrId)
        {
            return TRUE;
        }
    }
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpCheckReceivedAttributes()
//
// PURPOSE      : checking the received attribute value in a label mapping
//                or label request message.
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                msgSource - The LDP peer that sent the message
//                messageType - The type of message received.
//                RAttributes -  The attributes in the message
//
// RETURN VALUE : loop detection status
//-------------------------------------------------------------------------
static
int MplsLdpCheckReceivedAttributes(
    MplsLdpApp* ldp,
    NodeAddress MsgSource,
    int messageType,
    Attribute_Type* RAttribute)
{
    // A.2.6. MplsLdpCheckReceivedAttributes
    //
    // Summary:
    //
    // Check the attributes received in a Label Mapping or Label Request
    // message.  If the attributes include a Hop Count or Path Vector,
    // perform a loop detection check.  If a loop is detected, cause a
    // Loop Detected Notification message to be sent to MsgSource.
    //
    // Parameters:
    //
    // -  MsgSource.  The LDP peer that sent the message.
    //
    // -  MsgType.  The type of message received.
    //
    // -  RAttributes.  The attributes in the message.

    ERROR_Assert(RAttribute, "Error in"
        " MplsLdpCheckReceivedAttributes()"
        " RAttribute should not be NULL !!!");

    // CRa.1   Do RAttributes include Hop Count?
    //         If not, goto CRa.5.
    if (RAttribute->hopCount == TRUE)
    {
        // CRa.2   Does Hop Count exceed Max allowable hop count?
        //         If so, goto CRa.6.
        if (RAttribute->numHop > (unsigned int) ldp->maxAllowableHopCount)
        {
            // CRa.6   Is MsgType LabelMapping?
            //         If so, goto CRa.8.  (See Note 1.)
/*** Integration of MPLS with IP start ***/
            // Send loop detection error message in the case of
            // label request message also if the above case is true
            if ((messageType == LDP_LABEL_MAPPING_MESSAGE_TYPE) ||
                (messageType == LDP_LABEL_REQUEST_MESSAGE_TYPE))
/*** Integration of MPLS with IP end ***/
            {
                //  CRa.8   Return Loop Detected.
                //  CRa.9   DONE
                return LOOP_DETECTED;
            }
        }
    }
    else
    {
        // CRa.5   Return No Loop Detected.
        return NO_LOOP_DETECTED;
    }

    // CRa.3   Do RAttributes include Path Vector?
    //         If not, goto CRa.5.
    if (RAttribute->pathVector == TRUE)
    {
        // check does Path Vector Include LSR Id?
        BOOL ifLsrIdFound = IfPathVectorIncludeLSRId(
                                RAttribute->pathVectorValue.pathVector,
                                RAttribute->pathVectorValue.length,
                                ldp->localAddr[0]);

        // CRa.4   Does Path Vector Include LSR Id? OR
        //         Does length of Path Vector exceed Max allowable length?
        //         If so, goto CRa.6
        if ((ifLsrIdFound == TRUE) ||
           ((RAttribute->pathVectorValue.length) > ldp->pathVectorLimit))
        {
            // CRa.6   Is MsgType LabelMapping?
            //         If so, goto CRa.8.  (See Note 1.)
            if (messageType == LDP_LABEL_MAPPING_MESSAGE_TYPE)
            {
                //  CRa.8   Return Loop Detected.
                //  CRa.9   DONE
                return LOOP_DETECTED;
            }
        }
        else
        {
             // CRa.5   Return No Loop Detected.
             return NO_LOOP_DETECTED;
        }
    }
    else
    {
        // CRa.5   Return No Loop Detected.
        return NO_LOOP_DETECTED;
    }

    // CRa.7 Execute procedure Send_Notification (MsgSource, Loop
    //       Detected)

    ERROR_Assert(messageType == LDP_LABEL_REQUEST_MESSAGE_TYPE,
                "Message Reached This statement of the code"
                " should be LDP_LABEL_REQUEST_MESSAGE_TYPE");

    return LOOP_DETECTED;

    // NOTE : if "LOOP_DETECTED" is returnd by the above return
    //        statement, NotificationMessage will be sent from
    //        the caller function.
    //
    // NOTE : The above return statement will be reached only
    //        and only if Message type is
    //        "LDP_LABEL_REQUEST_MESSAGE_TYPE"
}


static
BOOL MplsLdpGetFecFromFecToLabelMappingRec(
    MplsLdpApp* ldp,
    NodeAddress LSR_Id,
    Mpls_FEC* fec)
{
   unsigned int i = 0;

    for (i = 0; i < ldp->incomingFecToLabelMappingRecNumEntries; i++)
    {
        if (ldp->incomingFecToLabelMappingRec[i].peer == LSR_Id)
        {
           memcpy(fec,
               &ldp->incomingFecToLabelMappingRec[i].fec,
               sizeof(Mpls_FEC));

           return TRUE;
        }
    }
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpPreviouslyReceivedAndRetainedAlabelMapping()
//
// PURPOSE      : checks if an LSR retains Label mapping of not
//                 for a given fec
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec to be matched
//                label - label value to be returned if a label
//                         mapping is retained.
//                RAttribute - restore the value of RAttribute Received
//
//
// RETURN VALUE : pointer to the matched record. or NULL otherwise.
//-------------------------------------------------------------------------
static
FecToLabelMappingRec* MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    unsigned int* label,
    NodeAddress LSR_Id,
    Attribute_Type** RAttribute)
{
    // checkes whether a previous label bound has been received.
    unsigned int i = 0;
    BOOL found = FALSE;
    *RAttribute = NULL;

    for (i = 0; i < ldp->incomingFecToLabelMappingRecNumEntries; i++)
    {
        if ((MatchFec(&ldp->incomingFecToLabelMappingRec[i].fec, &fec)) &&
            (ldp->incomingFecToLabelMappingRec[i].peer == LSR_Id))
        {
            found = TRUE;
            *label = ldp->incomingFecToLabelMappingRec[i].label;
            *RAttribute = &(ldp->incomingFecToLabelMappingRec[i].Attribute);
            break;
        }
    }

    if (found)
    {
       return &(ldp->incomingFecToLabelMappingRec[i]);
    }
    else
    {
       return NULL;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpDeleteEntriesFromIncomingFecToLabelMappingRec()
//
// PURPOSE      : delete entries from "incomingFecToLabelMappingRec"
//                cache
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec to be match
//                LSR_Id - LSR_Id to be matched
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
BOOL MplsLdpDeleteEntriesFromIncomingFecToLabelMappingRec(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress LSR_Id)
{
    unsigned int i = 0;
    unsigned int index = 0;
    BOOL found = FALSE;

    for (i = 0; i < ldp->incomingFecToLabelMappingRecNumEntries; i++)
    {
        if ((ldp->incomingFecToLabelMappingRec[i].peer == LSR_Id) &&
            (MatchFec(&ldp->incomingFecToLabelMappingRec[i].fec, &fec)))
        {
            index = i;
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        // Free Attribute type structure and reset it with all 0's
        MplsLdpFreeAttributeType(&(ldp->incomingFecToLabelMappingRec[index].
            Attribute));

        if (index == ldp->incomingFecToLabelMappingRecNumEntries - 1)
        {
            memset(&(ldp->incomingFecToLabelMappingRec[index]), 0,
                sizeof(FecToLabelMappingRec));
        }
        else
        {
            for (i = index + 1;
                 i < ldp->incomingFecToLabelMappingRecNumEntries; i++)
            {
                memcpy(&(ldp->incomingFecToLabelMappingRec[i - 1]),
                       &ldp->incomingFecToLabelMappingRec[i],
                       sizeof(FecToLabelMappingRec));
            }
            memset(&(ldp->incomingFecToLabelMappingRec[i]), 0,
                   sizeof(FecToLabelMappingRec));
        }
        (ldp->incomingFecToLabelMappingRecNumEntries)--;
    }
    return found;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpReceiveLabelMapping()
//
// PURPOSE      : receive and process a label mapping message.
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structur
//                msgSource - the peer from which label mapping message
//                            is received
//                label - label received
//                fec - fec for which the label is received
//                RAttribute - received attribute
//                isIngress - set to TRUE if it is ingress LSR.
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpReceiveLabelMapping(
    Node* node,
    MplsLdpApp* ldp,
    NodeAddress msgSource,
    unsigned int label,
    Mpls_FEC fec,
    Attribute_Type* RAttribute,
    BOOL isIngress)
{
    // A.1.2. Receive Label Mapping
    //
    // Summary:
    //
    // The response by an LSR to receipt of a FEC label mapping from an
    // LDP peer may involve one or more of the following actions:
    //
    // -  Transmission of a label release message for the FEC label to
    //    the LDP peer;
    //
    // -  Transmission of label mapping messages for the FEC to one or
    //    more LDP peers,
    //
    // -  Installation of the newly learned label for
    //    forwarding/switching use by the LSR.
    //
    // Context:
    //
    // -  LSR.  The LSR handling the event.
    //
    // -  MsgSource.  The LDP peer that sent the message.
    //
    // -  FEC.  The FEC specified in the message.
    //
    // -  Label.  The label specified in the message.
    //
    // -  PrevAdvLabel.  The label for FEC, if any, previously advertised
    //    to an upstream peer.
    //
    // -  StoredHopCount.  The hop count previously recorded for the FEC.
    //
    // -  RAttributes.  Attributes received with the message.  E.g., Hop
    //    Count, Path Vector.
    //
    // -  SAttributes to be included in Label Mapping message, if any,
    //    propagated to upstream peers.

    Attribute_Type* RAttributeNotUsed = NULL;
    MplsLdpHelloAdjacency* peerSource = NULL;
    MplsLdpHelloAdjacency* peerNext = NULL;
    unsigned int prevLabel = 0;
    unsigned int storedHopCount = 0;
    NodeAddress nextHopAddress = MPLS_LDP_INVALID_PEER_ADDRESS;
    unsigned int i = 0;
    NodeAddress upstream = MPLS_LDP_INVALID_PEER_ADDRESS;

    Attribute_Type SAttribute;                 // define and initialize
    SAttribute.hopCount = FALSE;               // SAttribute type
    SAttribute.pathVector = FALSE;
    SAttribute.numHop = MPLS_UNKNOWN_NUM_HOPS;
    SAttribute.pathVectorValue.length = 0;
    SAttribute.pathVectorValue.pathVector = NULL;

    if (MPLS_LDP_DEBUG)
    {
        printf("node = %d In the function MplsLdpReceiveLabelMapping()"
               " label = %d from %u\n"
               "fec.ipAddress = %u  fec.numsignificantbits %u\n",
               node->nodeId,
               label,
               msgSource,
               fec.ipAddress,
               fec.numSignificantBits);
    }

    peerSource = MplsLdpReturnHelloAdjacencyForThisLSR_Id(ldp, msgSource);

    if (!peerSource)
    {
        // NOTE : The ERROR_Assert() statement can be un-commented
        //        below for debugging. Under normal circumstances,
        //        i.e. without linl/node fault "peerSource"
        //        SHOULD NOT BE NULL.
        //
        // program should not reach this position.
        // printf("#%d No Match found For This LSR_Id  %u !!!\n",
        //       node->nodeId, msgSource);
        // ERROR_Assert(peerSource, "Error in MplsLdpReceiveLabelMapping()"
        //    " No Match found For This LSR_Id !!!");
        return;
    }

    // LMp.1   Does the received label mapping match an outstanding
    //         label request for FEC previously sent to MsgSource.
    //         If not, goto LMp.3.
    if (IsItOutstandingLabelRequestPreviouslySentForFEC(ldp, fec, msgSource))
    {
        //  LMp.2   Delete record of outstanding FEC label request.
        MplsLdpDeleteRowFromOutstandingLabelRequestList(
            ldp,
            fec,
            msgSource);

        if (MPLS_LDP_DEBUG)
        {
            printf("#%d: LMp.2 Delete record of"
                   "outstanding FEC label request.\n"
                   "label mapping is been sent to %u fec = %u\n",
                   node->nodeId,
                   msgSource,
                   fec.ipAddress);
        }
    }

    //  LMp.3   Execute procedure MplsLdpCheckReceivedAttributes (MsgSource,
    //          LabelMapping, RAttributes).
    //          If No Loop Detected, goto LMp.9.
    if (MplsLdpCheckReceivedAttributes(
            ldp,
            msgSource,
            LDP_LABEL_MAPPING_MESSAGE_TYPE,
            RAttribute) != NO_LOOP_DETECTED)
    {
        unsigned int prevLabel = 0;
        //   LMp.4   Does the LSR have a previously received label mapping
        //           for  FEC from MsgSource? (See Note 1.)
        //           If not, goto LMp.8.  (See Note 2.)
        if (MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
                ldp,
                fec,
                &prevLabel,
                peerSource->LSR2.LSR_Id,
                &RAttributeNotUsed))
        {
            // LMp.5   Does the label previously received from MsgSource
            //         match Label (i.e. the label received in the message)?
            //         (See Note 3.) If not, goto LMp.8.  (See Note 4.)
            if (label == prevLabel)
            {
                //  LMp.6   Delete matching label mapping for FEC
                //           previously received from MsgSource.
                MplsLdpDeleteEntriesFromIncomingFecToLabelMappingRec(
                    ldp,
                    fec,
                    msgSource);

                if (MPLS_LDP_DEBUG)
                {
                    printf("in function MplsLdpReceiveLabelMapping()\n"
                           "node = %d Label to be deleted %d \n",
                           node->nodeId,
                           label);

                    Print_FTN_And_ILM(node);
                }

                //  LMp.7   Remove Label from forwarding/switching use.
                //          (See Note 5.) Goto LMp.33.
                MplsLdpRemoveLabelForForwardingSwitchingUse(
                     node,
                     peerSource->LinkAddress,
                     label,
                     OUTGOING,
                     fec);

                //  LMp.33  Done
                return;
            }
        }

        if (MPLS_LDP_DEBUG)
        {
            printf("#%d is performing LABEL RELEASE PROCEDURE"
                   "for label %u\nLOOP DETECTED\n",
                   node->nodeId,
                   label);
        }

        // LMp.8   Execute procedure Send_Message (MsgSource, Label
        //          Release, FEC, Label, Loop Detected Status code).
        //          Goto LMp.33.
        MplsLdpSendLabelReleaseOrWithdrawMessage(
            node,
            ldp,
            peerSource,
            label,
            fec,
            LOOP_DETECTED,
            TRUE,
            ldp->currentMessageId,
            LDP_LABEL_RELEASE_MESSAGE_TYPE);

        // send the pdu

        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            peerSource);

        (ldp->currentMessageId)++;

        //  LMp.33  Done
        return;
    }

    //  LMp.9   Does LSR have a previously received label mapping for FEC
    //          from MsgSource for the LSP in question?  (See Note 6.)
    //          If not, goto LMp.11.
    if (MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
            ldp,
            fec,
            &prevLabel,
            peerSource->LSR2.LSR_Id,
            &RAttributeNotUsed))
    {
        //  LMp.10  Does the label previously received from MsgSource match
        //          Label (i.e., the label received in the message)?
        //          (See Note 3.)
        //          If not, goto LMp.32.  (See Note 4.)
        if (prevLabel != label)
        {
            if (MPLS_LDP_DEBUG)
            {
                printf("#%d PreviousLabel(%d) != LabelReceived (%d)"
                       " performing label release process for fec = %u\n",
                       node->nodeId,
                       prevLabel,
                       label,
                       fec.ipAddress);
            }

            //  LMp.32  Execute procedure Send_Message (MsgSource,
            //          Label Release, FEC, Label).
            MplsLdpSendLabelReleaseOrWithdrawMessage(
                node,
                ldp,
                peerSource,
                label,
                fec,
                NO_LOOP_DETECTED,
                TRUE,
                ldp->currentMessageId,
                LDP_LABEL_RELEASE_MESSAGE_TYPE);

            // send the pdu

            MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                node,
                ldp,
                peerSource);

            (ldp->currentMessageId) ++;

            //  LMp.33  DONE.
            return;
        }
    }

    //  LMp.11  Determine the Next Hop for FEC.
    IsNextHopExistForThisDest(node, fec.ipAddress, &nextHopAddress);

    //  LMp.12  Is MsgSource the Next Hop for FEC?
    //          If so, goto LMp.14.
    if ((nextHopAddress != msgSource) &&
        (nextHopAddress != peerSource->LinkAddress))
    {
        //  LMp.13  Perform LSR Label Release procedure:
        MplsLdpPerformLsrLabelReleaseProcedure(
            node,
            ldp,
            fec,
            peerSource,
            label,
            RAttribute);

        if (MPLS_LDP_DEBUG)
        {
            printf("nextHopAddress != msgSource nextHop = %u"
                   " msgSource = %u\n"
                   "#%d\nnextHopAddress != msgSource nextHop  = %u"
                   " peerSource->linkAddress = %u\n",
                   nextHopAddress,
                   msgSource,
                   node->nodeId,
                   nextHopAddress,
                   peerSource->LinkAddress);
        }
        //  LMp 33 Done
        return;
    }

    //  LMp.14  Is LSR an ingress for FEC?
    //          If not, goto LMp.16.
    if (isIngress)
    {
        //   LMp.15  Install Label for forwarding/switching use.
        MplsLdpInstallLabelForForwardingAndSwitchingUse(
            node,
            fec,
            nextHopAddress,
            label,
            OUTGOING);
    }

    //  LMp.16  Record label mapping for FEC with Label and RAttributes
    //           has been received from MsgSource.
    MplsLdpAddRowToIncomingFecToLabelMappingRec(
        ldp,
        fec,
        peerSource,
        label,
        RAttribute);

    //  LMp.17  Iterate through LMp.31 for each Peer.  (See Note 7).
    for (i = 0; i < ldp->numHelloAdjacency; i++)
    {
        //  LMp.18  Has LSR previously sent a label mapping for FEC to Peer
        //          for the LSP in question?  (See Note 8.)
        //          If so goto LMp.22
        if (MplsLdpRetrivePreviouslySentLabelMapping(
                ldp,
                fec,
                ldp->helloAdjacency[i].LSR2.LSR_Id,
                &prevLabel,
                &storedHopCount,
                NULL,
                FALSE))
        {
            int j = 0;
            int numRequest =
                ldp->helloAdjacency[i].outboundLabelMappingNumEntries;

            // LMp.22  Iterate through LMp.27 for each label mapping for FEC
            //         previously sent to Peer.
            for (j = 0; j < numRequest; j++)
            {
                if (MatchFec(&(ldp->helloAdjacency[i].
                                  outboundLabelMapping[j].fec),
                             &fec))
                {
                    continue;
                }
                else
                {
                    //  LMp.24  Execute procedure
                    //          MplsLdpPrepareLabelMappingAttributes(Peer,
                    //          FEC,RAttributes, SAttributes, IsPropagating,
                    //          StoredHopCount).
                    MplsLdpPrepareLabelMappingAttributes(
                        node,
                        ldp,
                        &(ldp->helloAdjacency[i]),
                        fec,
                        RAttribute,
                        &SAttribute,
                        TRUE,
                        storedHopCount);

                    if (MPLS_LDP_DEBUG)
                    {
                        printf("#%d is sending message to peer %u"
                               "label = %d / fec = %d\n",
                               node->nodeId,
                               ldp->helloAdjacency[i].LinkAddress,
                               prevLabel,
                               fec.ipAddress);
                    }

                    //  LMp.25  Execute procedure
                    //          Send_Message (Peer, Label Mapping, FEC,
                    //          PrevAdvLabel, SAttributes).  (See Note 10.)
                    MplsLdpSendLabelMappingMessage(
                        node,
                        ldp,
                        &(ldp->helloAdjacency[i]),
                        fec,
                        prevLabel,
                        &SAttribute,
                        (ldp->currentMessageId)++);

                    // send the pdu

                    MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                        node,
                        ldp,
                        &(ldp->helloAdjacency[i]));

                    //  LMp.26  Update record of label mapping for FEC
                    //          previously sent to Peer to include the new
                    //          attributes sent.
                }
                //  LMp.27  End iteration from LMp.22.
            }
        }

        // LMp.28  Does LSR have any label requests for FEC from Peer
        //         marked as pending?
        //         If not, goto LMp.30.
        peerSource = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                         ldp,
                         msgSource);

        ERROR_Assert(peerSource, "Error in MplsLdpReceiveLabelMapping()"
            " peer not found in hello adjacency list !!!");

        if (IsItDuplicateOrPendingRequest(
                ldp,
                fec,
                peerSource->LinkAddress,
                &upstream,
                TRUE))
        {
            unsigned int  label_sent_to_upstream = 0;

            peerNext = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                           ldp,
                           upstream);

            ERROR_Assert(peerNext,"Error in MplsLdpReceiveLabelMapping()"
                " peerNext should not be NULL !!!");

            //  LMp.29  Perform LSR Label Distribution procedure:
            label_sent_to_upstream =
            MplsLdpPerformLsrLabelDistributionProcedure(
                                         node,
                                         ldp,
                                         fec,
                                         peerNext,
                                         &SAttribute,
                                         RAttribute,
                                         peerSource->LinkAddress);

            if (!label_sent_to_upstream)
            {
                continue;
            }
            else
            {
                //  LMp.30  Perform LSR Label Use procedure:
                PerfromLsrLabelUseProcedure(
                    node,
                    ldp,
                    fec,
                    label_sent_to_upstream,
                    label);

                break;
            }
        }
        else
        {
            //  LMp.30  Perform LSR Label Use procedure:
            PerfromLsrLabelUseProcedure(
                node,
                ldp,
                fec,
                TRUE,
                label);

            break;
        }
        // LMp.31  End iteration from LMp.17.
        //         Go to LMp.33.
    }
    //  LMp 33 done
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessLabelMappingMessage()
//
// PURPOSE      : preparing for processing a label mapping message
//
// PARAMETERS   : node - node which is preparing to process the
//                       label mapping message
//                msg - the label request message.
//                msgHeader - message header.
//                LSR_Id - LSR_Id of peer LSR                .
//                tcpConnId - TCP connection Id
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpProcessLabelMappingMessage(
    Node* node,
    Message* msg,
    LDP_Message_Header* msgHeader,
    LDP_Identifier_Type LSR_Id,
    int tcpConnId)
{
    MplsLdpApp* ldp;
    int sizeOfPacket = 0;
    Attribute_Type RAttribute;

    LDP_TLV_Header tlvHeader;

    LDP_Prefix_FEC_Element prefixFecElement;    // These variables are
    NodeAddress* addressList = NULL;            // needed for extructing
    unsigned int lengthPrefix = 0;              // the value of FEC TLV
    Mpls_FEC fec;

    LDP_Generic_Label_TLV_Value_Type label_TLV; // For extracting label TLV

    char* packetPtr = MESSAGE_ReturnPacket(msg);
    sizeOfPacket = (msgHeader->Message_Length - LDP_MESSAGE_ID_SIZE);
    ldp = MplsLdpAppGetStateSpace(node);

    RAttribute.hopCount = FALSE;
    RAttribute.numHop = MPLS_UNKNOWN_NUM_HOPS;
    RAttribute.pathVector = FALSE;
    RAttribute.pathVectorValue.length = 0;

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d received a label Mapping Message\n"
               "Message Type = %x /message Id  == %d"
               "Total size of the packet : = %d\n"
               "The source LsrId is = %d\n"
               "Through Tcp ConnId %d\n",
               node->nodeId,
               LDP_Message_HeaderGetMsgType(msgHeader->ldpMsg),
               msgHeader->Message_Id,
               sizeOfPacket,
               LSR_Id.LSR_Id,
               tcpConnId);
    }

    // extract TLV_header from the packet into "tlvHeader"
    memcpy(&tlvHeader, packetPtr, sizeof(LDP_TLV_Header));
    packetPtr = packetPtr + sizeof(LDP_TLV_Header);
    sizeOfPacket = sizeOfPacket - sizeof(LDP_TLV_Header);

    if (MPLS_LDP_DEBUG)
    {
        printf("Removed TLV header Of FEC TLV"
               "U = %d , F = %d\n"
               "herder.Type  = %d\n"
               "header.length = %d\n",
               LDP_TLV_HeaderGetU(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetF(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetType(tlvHeader.tlvHdr),
               tlvHeader.Length);
    }

    // extract the LDP Prefix element value type into
    // "prefixFecElement"
    memcpy(&prefixFecElement, packetPtr , sizeof(LDP_Prefix_FEC_Element));
    packetPtr = packetPtr + sizeof(LDP_Prefix_FEC_Element);
    sizeOfPacket = sizeOfPacket - sizeof(LDP_Prefix_FEC_Element);

    if (MPLS_LDP_DEBUG)
    {
        printf("removed FEC TLV "
               "fec Element Type  %d"
               "fec Address family = %d , length = %d\n",
               prefixFecElement.Element_Type,
               prefixFecElement.Prefix.Address_Family,
               prefixFecElement.Prefix.PreLen);
    }

    // calculate the length of optional Prefix field.
    lengthPrefix = (tlvHeader.Length - sizeof(LDP_Prefix_FEC_Element));

    // allocate equivalant amount of memory space
    addressList = (NodeAddress*) MEM_malloc(lengthPrefix);

    // extract the addersses into "addressList"
    memcpy(addressList, packetPtr, lengthPrefix);

    packetPtr = packetPtr + lengthPrefix;
    sizeOfPacket = sizeOfPacket - lengthPrefix;

    if (MPLS_LDP_DEBUG)
    {
        int numPrefix = 0,i = 0;
        numPrefix = (lengthPrefix / sizeof(NodeAddress));

        printf("remove addresses from prefix field\n"
               "number of address received are : %d",
               numPrefix);

        for (i = 0; i < numPrefix; i++)
        {
            printf("%d, ", addressList[i]);
        }
    }

    // build fec data from Fec TLV
    fec.ipAddress = addressList[0];
    fec.numSignificantBits = prefixFecElement.Prefix.PreLen;
    // Changed for adding priority support for FEC classification.
    // By default the value of priority is zero.
    fec.priority = 0 ;

    // Remove Header of Label TLV field
    memcpy(&tlvHeader, packetPtr, sizeof(LDP_TLV_Header));
    packetPtr = packetPtr + sizeof(LDP_TLV_Header);
    sizeOfPacket = sizeOfPacket - sizeof(LDP_TLV_Header);

    if (MPLS_LDP_DEBUG)
    {
        printf("Removed TLV header Of FEC TLV\n"
               "U = %d , F = %d\n"
               "herder.Type  = %d\n"
               "header.length = %d\n",
               LDP_TLV_HeaderGetU(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetF(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetType(tlvHeader.tlvHdr),
               tlvHeader.Length);
    }

    // Remove label from Label TLV
    memcpy(&label_TLV, packetPtr, sizeof(LDP_Generic_Label_TLV_Value_Type));
    packetPtr = packetPtr + sizeof(LDP_Generic_Label_TLV_Value_Type);
    sizeOfPacket = sizeOfPacket - sizeof(LDP_Generic_Label_TLV_Value_Type);

    if (MPLS_LDP_DEBUG)
    {
        printf("Label TLV has been removed\n"
               "Value of label is  = %u \n"
               "size of optional field = %u\n",
               label_TLV.Label,
               sizeOfPacket);
    }

    while (sizeOfPacket > 0)
    {
        LDP_TLV_Header tlvHeader;

        // extract the next TLV header
        memcpy(&tlvHeader, packetPtr, sizeof(LDP_TLV_Header));
        packetPtr = packetPtr + sizeof(LDP_TLV_Header);
        sizeOfPacket = sizeOfPacket -  sizeof(LDP_TLV_Header);

        if (MPLS_LDP_DEBUG)
        {
            printf("received Optional field of type %d"
                   " of length %d\n",
                   LDP_TLV_HeaderGetType(tlvHeader.tlvHdr),
                   tlvHeader.Length);
        }

        switch (LDP_TLV_HeaderGetType(tlvHeader.tlvHdr))
        {
            case LDP_HOP_COUNT_TLV :
            {
                LDP_HopCount_TLV_Value_Type* hopCountValue;

                hopCountValue = (LDP_HopCount_TLV_Value_Type*)
                   MEM_malloc(sizeof(LDP_HopCount_TLV_Value_Type));

                memcpy(hopCountValue, packetPtr,
                    sizeof(LDP_HopCount_TLV_Value_Type));

                packetPtr = packetPtr + sizeof(LDP_HopCount_TLV_Value_Type);
                sizeOfPacket = sizeOfPacket
                               - sizeof(LDP_HopCount_TLV_Value_Type);

                if (MPLS_LDP_DEBUG)
                {
                    printf("received packet has hop count optional"
                           "and number of hop count is %u\n",
                           hopCountValue->hop_count_value);
                }

                // build RAttribute Values
                RAttribute.hopCount = TRUE;
                RAttribute.numHop = hopCountValue->hop_count_value;
                break;
            }
            case LDP_PATH_VECTOR_TLV :
            {
                int hopCountInPathVector = 0;
                NodeAddress* pathVectorTLV = NULL;

                hopCountInPathVector = tlvHeader.Length
                                       / sizeof(NodeAddress);

                pathVectorTLV = (NodeAddress*)
                    MEM_malloc(tlvHeader.Length);

                memcpy(pathVectorTLV, packetPtr, tlvHeader.Length);
                packetPtr = packetPtr + tlvHeader.Length;
                sizeOfPacket = sizeOfPacket - tlvHeader.Length;

                // build RAttribute Value
                RAttribute.pathVector = TRUE;
                RAttribute.pathVectorValue.length = hopCountInPathVector;
                RAttribute.pathVectorValue.pathVector = pathVectorTLV;

                if (MPLS_LDP_DEBUG)
                {
                    int i = 0;
                    printf("received packet has hop pathVector optional "
                           "\total size is %d\n"
                           "number of hop is %d they are:\n",
                           tlvHeader.Length,
                           hopCountInPathVector);

                    for (i = 0; i < hopCountInPathVector; i++)
                    {
                        printf("%u,",pathVectorTLV[i]);
                    }
                    printf("\n");
                }
                break;
            }
            default :
            {
                ERROR_Assert(FALSE, "Error in"
                    " MplsLdpProcessLabelMappingMessage()"
                    " TLV of unknown type has been received !!!");
            }
        }
    }

    MplsLdpReceiveLabelMapping(
        node,
        ldp,
        LSR_Id.LSR_Id,
        label_TLV.Label,
        fec,
        &RAttribute,
        TRUE);

    MEM_free(addressList);

    // increment number of label mapping received
    INCREMENT_STAT_VAR(numLabelMappingReceived);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddHelloAdjacencyToList()
//
// PURPOSE      : Establishing a hello adjacency.  Enters required
//                information about a distant neighbor into its
//                hello adjacency structure.
//
// PARAMETERS   : node - node which is adding to hello adjacency list
//                ldp - pointer to the MplsLdpApp structure.
//                sourceAddr - source address of the hello packet
//                              received by the node.
//                helloTimerInterval - hello Timer Interval
//                last Heard -  when last msg heard
//
// RETURN VALUE : pointer to MplsLdpHelloAdjacency - the hello adjacency
//                just added
//-------------------------------------------------------------------------
static
MplsLdpHelloAdjacency* MplsLdpAddHelloAdjacencyToList(
    Node* node,
    MplsLdpApp* ldp,
    NodeAddress sourceAddr,
    LDP_Identifier_Type LSR_Id,
    clocktype helloTimerInterval)
{
    int index = ldp->numHelloAdjacency;

    // Do we need to add more space?
    if (ldp->numHelloAdjacency == ldp->maxHelloAdjacency)
    {
        MplsLdpHelloAdjacency* newHelloAdjacency = (MplsLdpHelloAdjacency*)
            MEM_malloc(sizeof(MplsLdpHelloAdjacency)
                * ldp->maxHelloAdjacency * 2);
        // Initializing helloAdjacency to Zero
        memset(newHelloAdjacency, 0, sizeof(MplsLdpHelloAdjacency)
                * ldp->maxHelloAdjacency * 2);

        memcpy(newHelloAdjacency,
            ldp->helloAdjacency,
            sizeof(MplsLdpHelloAdjacency) * ldp->maxHelloAdjacency);

        MEM_free(ldp->helloAdjacency);
        ldp->helloAdjacency = newHelloAdjacency;
        ldp->maxHelloAdjacency *= 2;
    }

    // None of the sourceAddr or LSR_Id.LSR_Id should be NULL
    ERROR_Assert((sourceAddr && LSR_Id.LSR_Id),
                 "sourceAddr or LSR_Id.LSR_Id or both NULL !!!");

    ldp->helloAdjacency[index].LinkAddress = sourceAddr;
    ldp->helloAdjacency[index].LSR2.LSR_Id = LSR_Id.LSR_Id;
    ldp->helloAdjacency[index].LSR2.label_space_id = LSR_Id.label_space_id;
    ldp->helloAdjacency[index].helloTimerInterval = helloTimerInterval;
    ldp->helloAdjacency[index].lastHeard = node->getNodeTime();
/*** Code for MPLS with IP integration start ***/
    // Till the time initialization with client is done,
    // we should not send the label request message.
    ldp->helloAdjacency[index].OkToSendLabelRequests = FALSE;
/*** Code for MPLS with IP integration end***/
    ldp->helloAdjacency[index].buffer = BUFFER_AllocatePacketBuffer(
                                            0,
                                            ldp->maxPDULength,
                                            FALSE,
                                            NULL);

    InitializeReassemblyBuffer(&(ldp->helloAdjacency[index].reassemblyBuffer)
        , ldp->maxPDULength);

    ERROR_Assert(ldp->helloAdjacency[index].buffer,
                "Unable to allocate packet buffer !!!");

    // These are added for keep track of incoming request
    ldp->helloAdjacency[index].incomingCacheMaxEntries =
        MPLS_LDP_MAX_INCOMING_REQUEST_CACHE;

    ldp->helloAdjacency[index].incomingCacheNumEntries = 0;

    ldp->helloAdjacency[index].incomingCache = (IncomingRequestCache*)
        MEM_malloc(sizeof(IncomingRequestCache)
            * MPLS_LDP_MAX_INCOMING_REQUEST_CACHE);

    // For keeping track of outbond requests
    ldp->helloAdjacency[index].outboundCacheMaxEntries =
        MPLS_LDP_MAX_OUTBOUND_CACHE;

    ldp->helloAdjacency[index].outboundCacheNumEntries = 0;

    ldp->helloAdjacency[index].outboundCache = (OutboundLabelRequestCache*)
        MEM_malloc(sizeof(OutboundLabelRequestCache)
            * MPLS_LDP_MAX_OUTBOUND_CACHE);

    // For keeping track of out bound label mappings
    ldp->helloAdjacency[index].outboundLabelMappingMaxEntries =
        MPLS_LDP_MAX_OUTBOUND_CACHE;

    ldp->helloAdjacency[index].outboundLabelMappingNumEntries = 0;

    ldp->helloAdjacency[index].outboundLabelMapping =
        (OutboundLabelRequestCache*)
            MEM_malloc(sizeof(OutboundLabelRequestCache)
                * MPLS_LDP_MAX_OUTBOUND_CACHE);

    ldp->helloAdjacency[index].hopCountRequird = TRUE;
    ldp->helloAdjacency[index].numInterfaceAddress = 0;
    ldp->helloAdjacency[index].interfaceAddresses = NULL;

    ldp->helloAdjacency[index].connectionId = INVALID_TCP_CONNECTION_ID;
    ldp->numHelloAdjacency++;

    return &(ldp->helloAdjacency[ldp->numHelloAdjacency - 1]);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpCreateHelloAdjacency()
//
// PURPOSE      : creating a hello adjacency
//
// PARAMETERS   : node - which is creating hello adjacency
//                ldp - pointer to the MplsLdpApp structure.
//                sourceAddr - source address of the hello packet
//                              received by the node.
//                helloTimerInterval - hello Timer Interval
//
// RETURN VALUE : pointer to MplsLdpHelloAdjacency - the hello adjacency
//                just added
//-------------------------------------------------------------------------
static MplsLdpHelloAdjacency* MplsLdpCreateHelloAdjacency(
    Node* node,
    MplsLdpApp* ldp,
    NodeAddress sourceAddr,
    LDP_Identifier_Type LSR_Id,
    clocktype helloTimerInterval,
    int incomingInterfaceIndex)
{
    MplsLdpHelloAdjacency* helloAdjacency = NULL;
    unsigned int labelStack = IPv4_Explicit_NULL_Label;

    // The exchange of Hellos results in the creation of a Hello
    // adjacency at LSR1 that serves to bind the link (L) and
    // the label spaces LSR1:a and LSR2:b.

    ERROR_Assert(sourceAddr, "Error in MplsLdpCreateHelloAdjacency()"
        " source address is not valid !!!");

    helloAdjacency = MplsLdpAddHelloAdjacencyToList(
                         node,
                         ldp,
                         sourceAddr,
                         LSR_Id,
                         helloTimerInterval);

    MplsAssignFECtoOutgoingLabelAndInterface(
        node,
        sourceAddr,
        (sizeof(NodeAddress) * 8),
        sourceAddr,
        FIRST_LABEL,
        &labelStack,
        1); // 1 = Number of labels

    // If LSR1 does not already have an LDP session for the exchange
    // of label spaces LSR1:a and LSR2:b it attempts to open a TCP
    // connection for a new LDP session with LSR2.

    // LSR1 determines whether it will play the active or passive role
    // in session establishment by comparing addresses A1 and A2 as
    // unsigned integers.  If A1 > A2, LSR1 plays the active role;
    // otherwise it is passive.

    if ((unsigned int) ldp->localAddr[0] >
        (unsigned int) LSR_Id.LSR_Id)
    {
        // LSR is in active role.
        helloAdjacency->initMode = MPLS_LDP_ACTIVE_INITIALIZATION_MODE;

        // LSR is in active role. So it will try to open TCP
        // connection with "sourceAddr"
        ERROR_Assert(incomingInterfaceIndex != ANY_INTERFACE,
                    "Incoming Interface Index cannot be ANY_INTERFACE");

        APP_TcpOpenConnection(
            node,
            APP_MPLS_LDP,
            NetworkIpGetInterfaceAddress(
                node,
                incomingInterfaceIndex),
            node->appData.nextPortNum++,
            sourceAddr,
            (short) APP_MPLS_LDP,
            ANY_NODEID,
            PROCESS_IMMEDIATELY,
            IPTOS_PREC_INTERNETCONTROL);
    }
    else
    {
        // LSR is in passive role. So it will wight for
        // "sourceAddr" to open a TCP connection
        helloAdjacency->initMode = MPLS_LDP_PASSIVE_INITIALIZATION_MODE;
    }
    return helloAdjacency;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpUpdateOrEstablishHelloAdjacency()
//
// PURPOSE      : establishing or building hello adjacency list
//
// PARAMETERS   : node - node which is building hello adjacency.
//                sourceAddr - source address of the hello adjacency
//                             for which hello adjacency list has to
//                             be build or updated
//                LSR - LSR_Id of the hello adjacency for which hello
//                      adjacency list has to be build or updated
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpUpdateOrEstablishHelloAdjacency(
    Node* node,
    NodeAddress sourceAddr,
    LDP_Identifier_Type LSR,
    int incomingInterfaceIndex)
{
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);
    MplsLdpHelloAdjacency*  helloAdjacency;

    if (ldp)
    {
        helloAdjacency = MplsLdpReturnHelloAdjacency(
                             ldp,
                             sourceAddr,
                             LSR,
                             FALSE,    // do not match link address
                             TRUE);    // match LSR_Id

        // If the Hello is acceptable, the LSR checks whether it has a
        // Hello adjacency for the Hello source.  If so, it restarts the
        // hold timer for the Hello adjacency.  If not it creates a Hello
        // adjacency for the Hello source and starts its hold timer.

        if (helloAdjacency)
        {
            // "Reset" hold timer - actually, update lastHeard so that when
            // the existing hold timer expires, it will reset that, rather
            // than eliminating the hello adjacency
            UpdateLastHeard(helloAdjacency);
        }
        else
        {
            // Create a Hello Adjacency
            MplsLdpCreateHelloAdjacency(
                node,
                ldp,
                sourceAddr,
                LSR,
                ldp->targetedHelloInterval,
                incomingInterfaceIndex);
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddRowToIncomingRequestCache()
//
// PURPOSE      : adding a record to incoming label request cache
//
// PARAMETERS   : node - node which is adding a record to
//                       incoming label request cache
//                helloAdjacency - pointer to hello adjacency list.
//                fec - fec for which label request has came.
//                LSR_Id - from which label request has came.
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddRowToIncomingRequestCache(
    Node* node,
    MplsLdpHelloAdjacency* helloAdjacency,
    Mpls_FEC fec,
    NodeAddress LSR_Id)
{
    int index = helloAdjacency->incomingCacheNumEntries;

    if (helloAdjacency->incomingCacheNumEntries ==
          helloAdjacency->incomingCacheMaxEntries)
    {
        IncomingRequestCache* newTable = NULL;

        newTable = (IncomingRequestCache*)
            MEM_malloc((sizeof(IncomingRequestCache) * 2
                * helloAdjacency->incomingCacheMaxEntries));

        memcpy(newTable,
            helloAdjacency->incomingCache,
            (sizeof(IncomingRequestCache)
                * helloAdjacency->incomingCacheNumEntries));

        MEM_free(helloAdjacency->incomingCache);

        helloAdjacency->incomingCache = newTable;

        helloAdjacency->incomingCacheMaxEntries =
             (2 * helloAdjacency->incomingCacheMaxEntries);
    }

    memcpy(&(helloAdjacency->incomingCache[index].fec),
        &fec,
        sizeof(Mpls_FEC));

    helloAdjacency->incomingCache[index].LSR_Id = LSR_Id;
    helloAdjacency->incomingCache[index].entry_time = node->getNodeTime();
    helloAdjacency->incomingCacheNumEntries++;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddRowToLabelWithdrawnRec()
//
// PURPOSE      : to add records into the LabelWithdrawRec cache.
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                status - the status value.
//                fec - fec entry
//                label - label entry
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddRowToLabelWithdrawnRec(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress LSR_Id,
    int label)
{
    int index = ldp->withdrawnLabelRecNumEntries;

    if (ldp->withdrawnLabelRecNumEntries ==
            ldp->withdrawnLabelRecMaxEntries)
    {
        LabelWithdrawRec* newTable;

        newTable = (LabelWithdrawRec*)
            MEM_malloc((sizeof(LabelWithdrawRec) * 2
                * ldp->withdrawnLabelRecMaxEntries));

        memcpy(newTable,
            ldp->withdrawnLabelRec,
            sizeof(LabelWithdrawRec) * ldp->withdrawnLabelRecNumEntries);

        MEM_free(ldp->withdrawnLabelRec);

        ldp->withdrawnLabelRec = newTable;

        ldp->withdrawnLabelRecMaxEntries = 2
            * ldp->withdrawnLabelRecMaxEntries;
    }

    memcpy(&(ldp->withdrawnLabelRec[index].fec), &fec, sizeof(Mpls_FEC));
    ldp->withdrawnLabelRec[index].LSR_Id = LSR_Id;
    ldp->withdrawnLabelRec[index].labelWithdrawn = label;
    (ldp->withdrawnLabelRecNumEntries)++;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddRowToThePendingRequestList()
//
// PURPOSE      : to add records into the pending label request cache
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - the fec entry
//                nextHopLSR_Id - next hop LSR_Id
//                sourceLSR_Id - source LSR_Id
//
// RETURN VALUE : number of label mappings.
//-------------------------------------------------------------------------
static
void MplsLdpAddRowToThePendingRequestList(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress nextHopLSR_Id,
    NodeAddress sourceLSR_Id)
{
    if (ldp->pendingRequestNumEntries == ldp->pendingRequestMaxEntries)
    {
        PendingRequest* newTable;
        newTable = (PendingRequest*)
                   MEM_malloc(sizeof(PendingRequest) * 2
                       * ldp->pendingRequestMaxEntries);

        memcpy(newTable,
            ldp->pendingRequest,
            sizeof(PendingRequest) * ldp->pendingRequestNumEntries);

        MEM_free(ldp->pendingRequest);

        ldp->pendingRequest = newTable;

        ldp->pendingRequestMaxEntries = 2 * ldp->pendingRequestMaxEntries;
    }

    memcpy(&(ldp->pendingRequest[ldp->pendingRequestNumEntries].fec),
           &fec,
           sizeof(Mpls_FEC));

    ldp->pendingRequest[ldp->pendingRequestNumEntries].nextHopLSR_Id =
         nextHopLSR_Id;

    ldp->pendingRequest[ldp->pendingRequestNumEntries].sourceLSR_Id =
         sourceLSR_Id;

    (ldp->pendingRequestNumEntries)++;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpDeleteDuplicateOrPendingRequest()
//
// PURPOSE      : deletes a pending label request message
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec to be matched
//                LSR_Id - LSR_Id to be matched.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpDeleteDuplicateOrPendingRequest(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress LSR_Id)
{
    unsigned int i = 0;
    unsigned int index = 0;
    BOOL found = FALSE;

    for (i = 0; i < ldp->pendingRequestNumEntries; i++)
    {
        if ((MatchFec(&(ldp->pendingRequest[i].fec), &fec)) &&
            ((ldp->pendingRequest[i].nextHopLSR_Id == LSR_Id) ||
             (ldp->pendingRequest[i].sourceLSR_Id == LSR_Id)))
        {
            // remember the index of the entry to be deleted
            index = i;
            found = TRUE;
            break;
        }
        ERROR_Assert((ldp->pendingRequest[i].nextHopLSR_Id !=
                     ldp->pendingRequest[i].sourceLSR_Id),
                     "Next hop LSR_Id cannot be source LSR_Id!!!");
    }

    if (found)
    {
        if (index == (ldp->pendingRequestNumEntries - 1))
        {
            // if it is the last entry ....
            memset(&ldp->pendingRequest[index], 0, sizeof(PendingRequest));
        }
        else
        {
            // compress the empty space
            for (i = index + 1; i < ldp->pendingRequestNumEntries; i++)
            {
               memcpy(&ldp->pendingRequest[i - 1],
                   &ldp->pendingRequest[i],
                   sizeof(PendingRequest));
            }

            memset(&ldp->pendingRequest[ldp->pendingRequestNumEntries - 1],
                0,
                sizeof(PendingRequest));
        }
        (ldp->pendingRequestNumEntries)--;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddRowToOutgoingFecToLabelMappingRec()
//
// PURPOSE      : adding a record to fecToLabelMappingRec cache
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec entry
//                peer - pointer to peer LSR's record in hello adjacency
//                       list to which label mapping has been send.
//                label - label which is sent.
//                hopCount - hop count entry
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpAddRowToOutgoingFecToLabelMappingRec(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    MplsLdpHelloAdjacency* peer,
    unsigned int label,
    Attribute_Type* SAttribute)
{
    int index = ldp->fecToLabelMappingRecNumEntries;

    if (ldp->fecToLabelMappingRecNumEntries ==
           ldp->fecToLabelMappingRecMaxEntries)
    {
         FecToLabelMappingRec* newTable;
         newTable = (FecToLabelMappingRec*)
                    MEM_malloc(sizeof(FecToLabelMappingRec) * 2
                        * ldp->fecToLabelMappingRecMaxEntries);

         memcpy(newTable, ldp->fecToLabelMappingRec,
                sizeof(FecToLabelMappingRec)
                    * ldp->fecToLabelMappingRecNumEntries);

         MEM_free(ldp->fecToLabelMappingRec);

         ldp->fecToLabelMappingRec = newTable;

         ldp->fecToLabelMappingRecMaxEntries =
             2 * ldp->fecToLabelMappingRecMaxEntries;
    }

    memcpy(&(ldp->fecToLabelMappingRec[index].fec), &fec, sizeof(Mpls_FEC));
    ldp->fecToLabelMappingRec[index].peer = peer->LSR2.LSR_Id;
    ldp->fecToLabelMappingRec[index].label = label;

    // copy Atribute sent
    MplsLdpAddAttributeType(&(ldp->fecToLabelMappingRec[index].Attribute),
      SAttribute);

    (ldp->fecToLabelMappingRecNumEntries)++;
}


//-------------------------------------------------------------------------
// FUNCTION     : AnyRequestPreviouslyReceivedFromMsgSource()
//
// PURPOSE      : sending notification message to a peer in case
//                of an error occurs.
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec to be searched
//                LSR_Id - the to which notification message has to be sent.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
BOOL AnyRequestPreviouslyReceivedFromMsgSource(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress LSR_Id)
{
    unsigned int i = 0;
    BOOL found = FALSE;
    MplsLdpHelloAdjacency* helloAdjacency =
        MplsLdpReturnHelloAdjacencyForThisLSR_Id(ldp, LSR_Id);

    for (i = 0; i < helloAdjacency->incomingCacheNumEntries; i++)
    {
        if ((helloAdjacency->incomingCache[i].LSR_Id == LSR_Id) &&
            (MatchFec(&(helloAdjacency->incomingCache[i].fec), &fec)))
        {
            found = TRUE;
        }
    }
    return found;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpSendNotificationMessage()
//
// PURPOSE      : sending notification message to a peer in case
//                of an error occurs.
//
// PARAMETERS   : node - the node which is sending notification message
//                LSR_Id - the to which notification message has to be sent.
//                messageId - the unique message Id
//                statusCode - Notification Message Error Code
//                referedMessage - uniqueId of the message against against
//                                 which notification message is generated
//                message_type - message Type
//                notificationType - notification message type
//                forwardBit - set to either TRUE or FALSE depending on
//                             whether notification message is to be
//                             forwarded or not
//                ldp - pointer to the MplsLdpApp structure.
//                optional - optional parameters
//                helloAdjacency - pointer to peer LSR's record in the
//                                 hello adjacency structure to whom
//                                 notification message has to be sent
// RETURN VALUE : none
//----------------------------------------------------------------------
static
void MplsLdpSendNotificationMessage(
    Node* node,
    NodeAddress LSR_Id,
    unsigned int messageId,
    NotificationErrorCode statusCode,
    unsigned int referedMessage,
    unsigned int message_type,
    NotificationType notificationType,
    BOOL forwardBit,
    MplsLdpApp* ldp,
    OptionalParameters* optional,
    MplsLdpHelloAdjacency* helloAdjacency)
{
    PacketBuffer* buffer = NULL;
    char* notUsed = NULL;

    buffer = BUFFER_AllocatePacketBuffer(0,
                 sizeof(LDP_TLV_Header),
                 TRUE,
                 &notUsed);

    if (optional)
    {
        if (optional->extendedStatus)
        {
             MplsLdpAddExtendedStatusTLV(buffer, optional->extendedStatus);
        }

        if (optional->sizeReturnedPDU != 0)
        {
             MplsLdpAddReturnedPDUTLV(
                 buffer,
                 (unsigned short) optional->sizeReturnedPDU,
                 optional->returnedPDU);
        }

        if (optional->sizeReturnedMessage != 0)
        {
             MplsLdpAddReturnedMessageTLV(
                 buffer,
                 (unsigned short) optional->sizeReturnedMessage,
                 optional->returnedMessage);
        }

        switch (optional->otherTLVtype)
        {
            case LDP_FEC_TLV :
            {
                LDP_Prefix_FEC_Element fecTLVtype;
                LDP_TLV_Header tlvHeaderFec;

                fecTLVtype.Element_Type = 3;
                fecTLVtype.Prefix.Address_Family = IP_V4_ADDRESS_FAMILY;

                fecTLVtype.Prefix.PreLen = (char) ((Mpls_FEC*)
                    (optional->otherdata))->numSignificantBits;

                LDP_TLV_HeaderSetU(&(tlvHeaderFec.tlvHdr), 0);
                LDP_TLV_HeaderSetF(&(tlvHeaderFec.tlvHdr), 0);
                LDP_TLV_HeaderSetType(&(tlvHeaderFec.tlvHdr), LDP_FEC_TLV);
                tlvHeaderFec.Length = sizeof(LDP_Prefix_FEC_Element)
                                      + sizeof(NodeAddress);

                MplsAddFecTLV(
                    buffer,
                    tlvHeaderFec,
                    fecTLVtype,
                    *((Mpls_FEC*)(optional->otherdata)));

                break;
            }
            default :
            {
                ERROR_Assert(FALSE, "Error in "
                    "MplsLdpSendNotificationMessage() "
                    "Not FEC TLV !!!");
            }
        }
    }

    MplsLdpAddStatusTLV(
        buffer,
        (unsigned short) message_type,
        referedMessage,
        statusCode,
        notificationType,
        forwardBit);

    MplsLdpAddMessageHeader(
        buffer,
        (unsigned short) 0,
        (unsigned short) LDP_NOTIFICATION_MESSAGE_TYPE,
        (unsigned short) BUFFER_GetCurrentSize(buffer),
        messageId);

    MplsLdpSubmitMessageToHelloAdjacencyBuffer(
        node,
        ldp,
        buffer,
        helloAdjacency);

    BUFFER_FreePacketBuffer(buffer);

    if (MPLS_LDP_DEBUG)
    {
        printf("node %d  sending notification to LSR_Id %u\n"
               "error Type is %d refered message Id is %u\n"
               "message type is %d\n",
               node->nodeId,
               LSR_Id,
               statusCode,
               referedMessage,
               message_type);
    }

    // increment number of notification messages send
    INCREMENT_STAT_VAR(numNotificationMsgSend);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAllocateLabel()
//
// PURPOSE      : Allocate a Label
//
// PARAMETERS   : ldp - the LSR allocating the label
//
// RETURN VALUE : label
//-------------------------------------------------------------------------
unsigned
int MplsLdpAllocateLabel(MplsLdpApp* ldp)
{
    int label;

    if (ldp->nextLabel < MPLS_LDP_MAX_LABEL_VALUE)
    {
        return ((ldp->nextLabel)++);
    }
    else
    {
        // This block has to be coded yet
        label = 0;
        return label;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAddEntriesToLIB()
//
// PURPOSE      : to add records into the LIB (Label Information Base)
//                table
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                numSignificantBits - the number of significant bits in
//                                     IP address (prefix length)
//                prefix - dest IP address
//                nextHop - next hop LSR_Id
//                label - the label entry
//
// RETURN VALUE : number of label mappings.
//-------------------------------------------------------------------------
static
void MplsLdpAddEntriesToLIB(
    MplsLdpApp*  ldp,
    int numSignificantBits,
    NodeAddress prefix,
    NodeAddress nextHop,
    unsigned int label)
{
    LIB* newTable = NULL;

    if (ldp->LIB_NumEntries == ldp->LIB_MaxEntries)
    {
        newTable = (LIB*) MEM_malloc(sizeof(LIB) * ldp->LIB_MaxEntries * 2);
        memcpy(newTable, ldp->LIB_Entries,
            sizeof(LIB) * ldp->LIB_NumEntries);
        MEM_free(ldp->LIB_Entries);
        ldp->LIB_Entries = newTable;
        ldp->LIB_MaxEntries = (ldp->LIB_MaxEntries * 2);
    }

    ldp->LIB_Entries[ldp->LIB_NumEntries].length = numSignificantBits;
    ldp->LIB_Entries[ldp->LIB_NumEntries].prefix = prefix;
    ldp->LIB_Entries[ldp->LIB_NumEntries].nextHop = nextHop;
    ldp->LIB_Entries[ldp->LIB_NumEntries].LSR_Id.LSR_Id = nextHop;

    ldp->LIB_Entries[ldp->LIB_NumEntries].LSR_Id.label_space_id =
        PLATFORM_WIDE_LABEL_SPACE_ID;

    ldp->LIB_Entries[ldp->LIB_NumEntries].label = label;
    ldp->LIB_NumEntries++;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpSendLabel()
//
// PURPOSE      : sending label mapping message to the next hop peer
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structure.
//                peer - pointer to peer LSR's record in the
//                       hello adjacency structure
//                fec - fec tobe send with the label request message
//                SAttribute - attribute to be send with label mapping
//                             message
//                haveLabelToSend - Does LSR have a label to allocate?
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static unsigned int
MplsLdpSendLabel(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* peer,
    Mpls_FEC fec,
    Attribute_Type* SAttribute,
    BOOL haveLabelToSend)
{
    //  A.2.1. MplsLdpSendLabel
    //
    //  Summary:
    //
    //  The MplsLdpSendLabel procedure allocates a label for a FEC for an LDP
    //  peer, if possible, and sends a label mapping for the FEC to the
    //  peer.  If the LSR is unable to allocate the label and if it has a
    //  pending label request from the peer, it sends the LDP peer a No
    //  Label Resources notification.
    //
    //  Parameters:
    //
    //  -  Peer.  The LDP peer to which the label mapping is to be sent.
    //
    //  -  FEC.  The FEC for which a label mapping is to be sent.
    //
    //  -  Attributes.  The attributes to be included with the label
    //     mapping.
    NodeAddress upstream = MPLS_LDP_INVALID_PEER_ADDRESS;

    // SL.1   Does LSR have a label to allocate?
    //        If not, goto SL.9.
    if (haveLabelToSend)
    {
        unsigned int label = 0;
        NodeAddress nextHop = MPLS_LDP_INVALID_PEER_ADDRESS;
        unsigned int notUsed = 0;

        //  SL.2   Allocate Label and bind it to the FEC.
        label = MplsLdpAllocateLabel(ldp);
        nextHop = peer->LinkAddress;

        IsNextHopExistForThisDest(node, fec.ipAddress, &nextHop);

        if (!SearchEntriesInTheLib(ldp, fec, &notUsed))
        {
            // Label exists in LIB means this label already sent to
            // upstream. Avoid entering duplicate incoming label.
            MplsLdpAddEntriesToLIB(
                ldp,
                fec.numSignificantBits,
                fec.ipAddress,
                nextHop,
                label);

           //  SL.3   Install Label for forwarding/switching use.
           /* MplsLdpInstallLabelForForwardingAndSwitchingUse(
                node,
                fec,
                nextHop,
                label,
                INCOMING);*/
        }

        // Modified as part of IP-MPLS integration.
        BOOL installLabel = FALSE;
        for (unsigned int i = 0; i < ldp->fecToLabelMappingRecNumEntries;i++)
        {
            if ((ldp->fecToLabelMappingRec[i].fec.ipAddress == fec.ipAddress)
                &&(ldp->fecToLabelMappingRec[i].fec.numSignificantBits ==
                    fec.numSignificantBits)
                &&(ldp->fecToLabelMappingRec[i].fec.priority == fec.priority)
                &&(ldp->fecToLabelMappingRec[i].peer == peer->LSR2.LSR_Id))
             {
                  installLabel = TRUE;
                  break;
             }
        }
        if (!installLabel)
        {
            //  SL.3   Install Label for forwarding/switching use.
            MplsLdpInstallLabelForForwardingAndSwitchingUse(
                node,
                fec,
                nextHop,
                label,
                INCOMING);
        }
        //  SL.4   Execute procedure Send_Message (Peer, Label Mapping, FEC,
        //         Label, Attributes).
        MplsLdpSendLabelMappingMessage(
            node,
            ldp,
            peer,
            fec,
            label,
            SAttribute,
            (ldp->currentMessageId)++);

        // send the pdu
        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            peer);

        //  SL.5   Record label mapping for FEC with Label and Attributes
        //         has been sent to Peer.
        MplsLdpAddRowToOutgoingFecToLabelMappingRec(
            ldp,
            fec,
            peer,
            label,
            SAttribute);

        //  SL.6   Does LSR have a record of a FEC label request from Peer
        //         marked as pending?
        if (IsItDuplicateOrPendingRequest(
               ldp,
               fec,
               peer->LSR2.LSR_Id,
               &upstream,
               TRUE))
        {
            //  SL.7   Delete record of pending label request
            //         for FEC from Peer.
            MplsLdpDeleteDuplicateOrPendingRequest(
                ldp,
                fec,
                peer->LSR2.LSR_Id);

            return label;
        }
        else
        {
            //  SL.8   Return success.
            ERROR_Assert(label,"Error in MplsLdpSendLabel()"
                " label is 0 !!!");
            return label;
        }
    }

    // SL.9   Does LSR have a label request for FEC from Peer marked as
    //        pending?
    //        If not, goto SL.13.
    if (IsItDuplicateOrPendingRequest(
            ldp,
            fec,
            peer->LSR2.LSR_Id,
            &upstream,
            TRUE))
    {
        //  SL.10  Execute procedure Send_Notification (Peer, No Label
        //         Resources).
        MplsLdpSendNotificationMessage(
            node,
            peer->LSR2.LSR_Id,
            ldp->currentMessageId,
            NO_LABEL_RESOURCE_ERROR,
            0,
            LDP_LABEL_MAPPING_MESSAGE_TYPE,
            FATAL_ERROR_NOTIFICATION,
            FALSE,
            ldp,
            NULL,
            peer);

        // send the pdu
        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            peer);

        //  SL.11  Delete record of pending label request for FEC from Peer.
        MplsLdpDeleteDuplicateOrPendingRequest(ldp, fec, peer->LSR2.LSR_Id);

        //  SL.12  Record No Label Resources notification has been sent to
        //         Peer.
        //         Goto SL.14.
    }
    else
    {
        //  SL.13  Record label mapping needed for FEC and Attributes for
        //         Peer, but no-label-resources.  (See Note 1.)
    }
    //  SL.14  Return failure.
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpRecognizeNewFEC()
//
// PURPOSE      : if a new fec is recognized perform necessery actions.
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structure.
//                fec - fec tobe send with the label request message
//                nextHopAddr -  The next hop for the FEC
//                initAttribute - initial attribute
//                SAttribute - attribute to be send with label mapping
//                             message
//                storedHopCount - Hop count associated with FEC label
//                                 mapping, if any, previously received
//                                 from Next Hop.
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpRecognizeNewFEC(
    Node* node,
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress nextHopAddr,
    Attribute_Type* initAttribute,
    Attribute_Type* SAttribute,
    int storedHopCount)
{
    // Recognize New FEC
    //
    // Summary:
    //
    // The response by an LSR to learning a new FEC via the routing table
    // may involve one or more of the following actions:
    //
    //     -  Transmission of label mappings for the FEC to one or more LDP
    //        peers;
    //
    //     -  Transmission of a label request for the FEC to the FEC next
    //        hop;
    //
    //     -  Any of the actions that can occur when the LSR receives a label
    //        mapping for the FEC from the FEC next hop.
    //
    // Context:
    //
    //     -  LSR.  The LSR handling the event.
    //
    //     -  FEC. The newly recognized FEC.
    //
    //     -  Next Hop.  The next hop for the FEC.
    //
    //     -  InitAttributes.  Attributes to be associated with the new FEC.
    //        (See Note 1.)
    //
    //     -  SAttributes.  Attributes to be included in Label Mapping or
    //        Label Request messages, if any, sent to peers.
    //
    //     -  StoredHopCount.  Hop count associated with FEC label mapping,
    //        if any, previously received from Next Hop.
    unsigned int i = 0;
    int isPropagating = FALSE;
    MplsLdpHelloAdjacency* peer = NULL;
    unsigned int label = 0;
    FecToLabelMappingRec* hasLabel = NULL;
    LDP_Identifier_Type notUsed;

    memset(&notUsed, 0, sizeof(LDP_Identifier_Type));

    //  FEC.1   Perform LSR Label Distribution procedure:
    //
    //          For Downstream Unsolicited Independent Control
    //
    //           1. Iterate through 5 for each Peer.
    if ((ldp->labelAdvertisementMode == UNSOLICITED) &&
           (ldp->labelDistributionControlMode ==
               MPLS_CONTROL_MODE_INDEPENDENT))
    {

        if (MPLS_LDP_DEBUG)
        {
            printf("MPLS_LDP in MPLS_CONTROL_MODE_INDEPENDENT\n"
                   "#%d: MplsLdpRecognizeNewFEC(dest = %d / nextHop = %d)\n",
                   node->nodeId,
                   fec.ipAddress,
                   nextHopAddr);
        }

        for (i = 0; i < ldp->numHelloAdjacency; i++)
        {

            // 2.Has LSR previously received and retained a label
            //   mapping for FEC from Next Hop?
            //  If so, set Propagating to IsPropagating.
            //  If not, set Propagating to NotPropagating.
            if (MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
                    ldp,
                    fec,
                    &label,
                    ldp->helloAdjacency[i].LSR2.LSR_Id,
                    &initAttribute))
            {
                isPropagating = TRUE;
            }
            else
            {
                isPropagating = FALSE;
            }

            // 3. Execute procedure MplsLdpPrepareLabelMappingAttributes
            //   (Peer, FEC, InitAttributes, SAttributes, Propagating,
            //   Unknown hop count(0)).
            peer = &(ldp->helloAdjacency[i]);

            MplsLdpPrepareLabelMappingAttributes(
                node,
                ldp,
                peer,
                fec,
                initAttribute,
                SAttribute,
                isPropagating,
                MPLS_UNKNOWN_NUM_HOPS);

            if (MPLS_LDP_DEBUG)
            {
                printf("#%d is sending a label mapping to %u fec: %u\n",
                       node->nodeId,
                       peer->LSR2.LSR_Id,
                       fec.ipAddress);
            }

            //  4. Execute procedure MplsLdpSendLabel (Peer, FEC,
            //     SAttributes)
            MplsLdpSendLabel(node, ldp, peer, fec, SAttribute, TRUE);

            // 5. End iteration from 1.
            //   Goto FEC.2.
        }
    }
    else if ((ldp->labelAdvertisementMode == UNSOLICITED) &&
                (ldp->labelDistributionControlMode ==
                    MPLS_CONTROL_MODE_ORDERED))
    {
        if (MPLS_LDP_DEBUG)
        {
            printf("MPLS_LDP in MPLS_CONTROL_MODE_ORDERED\n"
                   "#%d: MplsLdpRecognizeNewFEC(dest = %d / nextHop = %d)\n",
                   node->nodeId,
                   fec.ipAddress,
                   nextHopAddr);
        }

        // For Downstream Unsolicited Ordered Control
        //
        //    1. Iterate through 5 for each Peer.
        for (i = 0; i < ldp->numHelloAdjacency; i++)
        {
            hasLabel = MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
                           ldp,
                           fec,
                           &label,
                           ldp->helloAdjacency[i].LSR2.LSR_Id,
                           &initAttribute);

            // 2.  Is LSR egress for the FEC? OR
            //     Has LSR previously received and retained a label
            //     mapping for FEC from Next Hop?
            //     If not, continue iteration for next Peer.
            if (IamEgressForThisFec(node, fec, ldp) || (hasLabel))
            {
                // 3. Execute procedure MplsLdpPrepareLabelMappingAttributes
                //    (Peer, FEC, InitAttributes, SAttributes, Propagating,
                //    StoredHopCount).
                peer = &(ldp->helloAdjacency[i]);

                MplsLdpPrepareLabelMappingAttributes(
                    node,
                    ldp,
                    peer,
                    fec,
                    initAttribute,
                    SAttribute,
                    TRUE,
                    storedHopCount);

                // 4. Execute procedure MplsLdpSendLabel (Peer, FEC,
                //                                        SAttributes)
                MplsLdpSendLabel(node, ldp, peer, fec, SAttribute, TRUE);
            }
            // 5. End iteration from 1.
            //     Goto FEC.2.
        }
    }
    //  For Downstream On Demand Independent Control OR
    //  For Downstream On Demand Ordered Control
    //
    //  1. Goto FEC.2.  (See Note 2.)
    //
    peer = MplsLdpReturnHelloAdjacency(
               ldp,
               nextHopAddr,
               notUsed,
               TRUE,
               FALSE);

    if (peer)
    {
        hasLabel = MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
                       ldp,
                       fec,
                       &label,
                       peer->LSR2.LSR_Id,
                       &initAttribute);
    }

    // FEC.2   Has LSR previously received and retained a label
    //         mapping for FEC from Next Hop?
    //         If so, goto FEC.5
    if (!hasLabel)
    {
        // FEC.3   Is Next Hop an LDP peer?
        //         If not, Goto FEC.6
        if (!peer)
        {
            if (MPLS_LDP_DEBUG)
            {
                printf("The nextHopAddr == %u is not the"
                       " current LSR's(%d) peer."
                       "Label request operation aborted\n",
                       nextHopAddr,
                       node->nodeId);
            }
            // FEC.6  done
            return;
        }

        if (MPLS_LDP_DEBUG)
        {
            printf("#%d start label Request procedure\n",
                   node->nodeId);
        }

        // FEC.4   Perform LSR Label Request procedure:
        MplsLdpPerformLabelRequestProcedure(
            node,
            ldp,
            peer,
            fec,
            initAttribute,
            SAttribute,
            ldp->localAddr[0],
            Request_When_Needed);
    }
    else
    {
        //  FEC.5   Generate Event: Received Label Mapping from Next Hop.
        //          (See Note 3.)
        NodeAddress nextHop = MPLS_LDP_INVALID_PEER_ADDRESS;
        int interfaceId = INVALID_INTERFACE_INDEX;
        Attribute_Type RAttribute;
        MplsLdpHelloAdjacency* ldpNext = NULL;
        LDP_Identifier_Type notUsed;

        ERROR_Assert(label, "Error in"
            " MplsLdpRecognizeNewFEC() label is 0 !!!");

        memset(&RAttribute, 0, sizeof(RAttribute));
        memset(&notUsed, 0, sizeof(LDP_Identifier_Type));

        NetworkGetInterfaceAndNextHopFromForwardingTable(
            node,
            fec.ipAddress,
            &interfaceId,
            &nextHop);

        if (nextHop == 0)
        {
            // next hop address "0" means next hop
            // itself is the destination address
            nextHop = fec.ipAddress;
        }

        ldpNext = MplsLdpReturnHelloAdjacency(
                      ldp,
                      nextHop,
                      notUsed,
                      TRUE,
                      FALSE);

        assert(ldpNext);

        //  FEC.5   Generate Event: Received Label Mapping from Next Hop.
        //          (See Note 3.)
        MplsLdpReceiveLabelMapping(
            node,
            ldp,
            ldpNext->LSR2.LSR_Id,
            label,
            fec,
            initAttribute,
            TRUE);
    }
    // FEC.6  done
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpSendLabelWithdraw()
//
// PURPOSE      : sending a label withdraw message.
//
// PARAMETERS   : node - node which is sending the label withdraw message
//                ldp - pointer to the MplsLdpApp structure.
//                peerSource - pointer to the peer LSR's record in the
//                             hello adjacency list to which label withdraw
//                             message is to be sent
//                fec - the FEC to be specified in the message
//                label - The label specified in the message.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpSendLabelWithdraw(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* peerSource,
    Mpls_FEC fec,
    unsigned int label)
{
    //  SWd.1  Execute procedure Send_Message (Peer, Label Withdraw, FEC,
    //         Label)
    MplsLdpSendLabelReleaseOrWithdrawMessage(
        node,
        ldp,
        peerSource,
        label,
        fec,
        NO_LOOP_DETECTED,
        TRUE,
        ldp->currentMessageId,
        LDP_LABEL_WITHDRAW_MESSAGE_TYPE);

    // send the pdu

    MplsLdpTransmitPduFromHelloAdjacencyBuffer(
        node,
        ldp,
        peerSource);

    ldp->currentMessageId++;

    //  SWd.2  Record label withdraw for FEC has been sent to Peer and
    //         mark it as outstanding.
    MplsLdpAddRowToLabelWithdrawnRec(
        ldp,
        fec,
        peerSource->LSR2.LSR_Id,
        label);
}


//--------------------------------------------------------------------------
//  FUNCTION     : MplsLdpAddRowToOutgoingLabelRequestCache()
//
//  PURPOSE      : to add records into the outgoing label request cache.
//
//  PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                 status - the status value.
//                 fec - fec entry
//                 LSR_Id - LSR_Id
//                 status - status type (TRUE or FALSE)
//
//  RETURN VALUE : pointer to the item just added.
//--------------------------------------------------------------------------
static
StatusRecord* MplsLdpAddRowToOutgoingLabelRequestCache(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress LSR_Id,
    BOOL status,
    Attribute_Type* SAttribute)
{
    int index = ldp->outGoingRequestNumEntries;

    if (index == (signed) ldp->outGoingRequestMaxEntries)
    {
        StatusRecord* newTable = NULL;

        newTable = (StatusRecord*) MEM_malloc(sizeof(StatusRecord) * 2
                       * ldp->outGoingRequestMaxEntries);

        memcpy(newTable,
            ldp->outGoingRequest,
            sizeof(StatusRecord) * index);

        MEM_free(ldp->outGoingRequest);

        ldp->outGoingRequest = newTable;
        ldp->outGoingRequestMaxEntries = 2 * index;
    }

    // Changed for adding priority support for FEC classification.
    // Initialise the memory assigned.
    memset(&(ldp->outGoingRequest[index].fec), 0, sizeof(Mpls_FEC));

    memcpy(&(ldp->outGoingRequest[index].fec), &fec, sizeof(Mpls_FEC));

    ldp->outGoingRequest[index].LSR_Id = LSR_Id;

    ldp->outGoingRequest[index].statusOutstanding = status;

    MplsLdpAddAttributeType(&(ldp->outGoingRequest[index].Attribute),
                            SAttribute);

    (ldp->outGoingRequestNumEntries)++;

    return &(ldp->outGoingRequest[index]);
}


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpReturnStatusRecordForThisFEC()
//
//  PURPOSE      : Returns status record for a given LSR. And if not found
//                 Adds it into status record (outGoingRequest cache).
//
//  PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                 fec - fec whose status record is to be searched for
//                 destLSR - peer LSR ID
//
//  RETURN VALUE : pointer to the entry added
//-------------------------------------------------------------------------
static
StatusRecord* MplsLdpReturnStatusRecordForThisFEC(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    NodeAddress destLSR,
    Attribute_Type* SAttribute)
{
    unsigned int i = 0;
    BOOL found = FALSE;

    for (i = 0; i < ldp->outGoingRequestNumEntries; i++)
    {
        if ((ldp->outGoingRequest[i].LSR_Id == destLSR) &&
            (MatchFec(&fec, &ldp->outGoingRequest[i].fec)))
        {
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        return &(ldp->outGoingRequest[i]);
    }
    else
    {
        return MplsLdpAddRowToOutgoingLabelRequestCache(
                    ldp,
                    fec,
                    destLSR,
                    FALSE,  // status Outstanding == FALSE
                    SAttribute);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpGetNotificationErrorCode()
//
// PURPOSE      : preparing for processing a label mapping message
//
// PARAMETERS   : msg - the label request message.
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static NotificationErrorCode MplsLdpGetNotificationErrorCode(
    const Message* msg)
{
    char* packetPtr = NULL;
    LDP_Status_TLV_Value_Type statusTlv;

    packetPtr = MESSAGE_ReturnPacket(msg);

    // skip the header part
    packetPtr = (packetPtr + sizeof(LDP_TLV_Header));

    //extract Status TLV Data
    memcpy(&statusTlv, packetPtr, sizeof(LDP_Status_TLV_Value_Type));

    //return errorCode
    return (NotificationErrorCode)
        LDP_Status_TLVGetStatus(statusTlv.statusValue);
}


//-------------------------------------------------------------------------
// FUNCTION    : MplsLdpPerformLsrLabelNoRouteProcedure
//
// PURPOSE     : to trigger a label request timeout timer
//
// PARAMETERS  : node - node which is starting the timer
//               ldp - pointer to MplsLdpApp structure
//               defReq - details of the defered label request inormation
//                        see the structure DefferedRequest
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpPerformLsrLabelNoRouteProcedure(
    Node* node,
    DefferedRequest defReq)
{

    Message* msg = NULL;

    //  1. Record deferred label request for FEC and Attributes
    //           to be sent to MsgSource.
    //

    // 2. Start timeout.  Goto NoNH.3.
    msg = MESSAGE_Alloc(
              node,
              APP_LAYER,
              APP_MPLS_LDP,
              MSG_APP_MplsLdpLabelRequestTimerExpired);

    MESSAGE_InfoAlloc(node, msg, sizeof(DefferedRequest));
    memcpy(MESSAGE_ReturnInfo(msg), &defReq, sizeof(DefferedRequest));

    MplsLdpAddAttributeType(
        &(((DefferedRequest*) MESSAGE_ReturnInfo(msg))->SAttribute),
        &(defReq.SAttribute));

    MESSAGE_Send(
        node,
        msg,
        LDP_DEFAULT_LABEL_REQUEST_TIME_OUT);
}


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpProcessLoopDetectedError()
//
//  PURPOSE      : processing loop detection error
//
//  PARAMETERS   : node - node which is processing loop detection error
//                 ldp - pointer to the MplsLdpApp structure.
//                 msg - the error message.
//                 msgHeader - message header.
//                 LSR_Id - LSR_Id of peer LSR                .
//-------------------------------------------------------------------------
static
void MplsLdpProcessLoopDetectedError(
    Node* node,
    MplsLdpApp* ldp,
    Message* msg,
    LDP_Message_Header* msgHeader,
    LDP_Identifier_Type LSR_Id)
{
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessNoRouteError()
//
// PURPOSE      : processing no route error
//
// PARAMETERS   : node - node which is processing no route error
//                ldp - pointer to the MplsLdpApp structure.
//                msg - the error message.
//                msgHeader - message header.
//                LSR_Id - LSR_Id of peer LSR                .
//-------------------------------------------------------------------------
static
void MplsLdpProcessNoRouteError(
    Node* node,
    MplsLdpApp* ldp,
    Message* msg,
    LDP_Message_Header* msgHeader,
    Mpls_FEC fec,
    LDP_Identifier_Type LSR_Id)
{
    // Summary:
    //
    // When an LSR receives a No Route notification from an LDP peer in
    // response to a Label Request message, the Label No Route procedure
    // in use dictates its response. The LSR either will take no further
    // action, or it will defer the label request by starting a timer and
    // send another Label Request message to the peer when the timer
    // later expires.

    StatusRecord* StatusRecPtr = NULL;
    DefferedRequest defReq;

    memset(&defReq, 0, sizeof(DefferedRequest));

    StatusRecPtr = MplsLdpReturnStatusRecordForThisFEC(
                        ldp,
                        fec,
                        LSR_Id.LSR_Id,
                        &(defReq.SAttribute));

    defReq.LSR_Id = LSR_Id.LSR_Id;
    memcpy(&defReq.fec, &fec, sizeof(Mpls_FEC));

    MplsLdpAddAttributeType(&(defReq.SAttribute),
        &(StatusRecPtr->Attribute));

    //  NoNH.1  Delete record of outstanding label request for FEC sent
    //          to MsgSource.
    MplsLdpDeleteRowFromOutstandingLabelRequestList(
        ldp,
        fec,
        LSR_Id.LSR_Id);

    //NoNH.2  Perform LSR Label No Route procedure.
    MplsLdpPerformLsrLabelNoRouteProcedure(node, defReq);

    MplsLdpFreeAttributeType(&(defReq.SAttribute));
    // NoNH.3  DONE.
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessNoLabelResourceError()
//
// PURPOSE      : processing no label resource error
//
// PARAMETERS   : node - node which is processing no label resource error
//                ldp - pointer to the MplsLdpApp structure.
//                msg - the error message.
//                msgHeader - message header.
//                LSR_Id - LSR_Id of peer LSR                .
//-------------------------------------------------------------------------
static
void MplsLdpProcessNoLabelResourceError(
    Node* node,
    MplsLdpApp* ldp,
    Message* msg,
    LDP_Message_Header* msgHeader,
    LDP_Identifier_Type LSR_Id)
{

    // NoRes.1 Delete record of outstanding label request for FEC sent
    //         to MsgSource.
    //
    // NoRes.2 Record label mapping for FEC from MsgSource is needed but
    //         that no label resources are available.
    //
    // NoRes.3 Set status record indicating it is not OK to send label
    //         requests to MsgSource.
    //
    // NoRes.4 DONE.
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessHelloMessage()
//
// PURPOSE      : processing Hello message
//
// PARAMETERS   : node - node which is receiving the hello message.
//                msg - the hello message
//                sourcePort - source port
//                sourceAddr - source address of the hello adjacency
//                             form which the hello message is received
//                LSR - LSR_Id of of the hello adjacency form which
//                      the hello message is received
//                msgHeader - pointer to ldp message header.
//
// RETURN VALUE : none.
//
// ASSUMPTION   : hello message only contains common hello parameter
//                tlv (which is mandatory), and no optional parameter
//-------------------------------------------------------------------------
static
void MplsLdpProcessHelloMessage(
    Node* node,
    Message* msg,
    NodeAddress sourceAddr,
    LDP_Identifier_Type LSR_Id,
    int incomingInterfaceIndex)
{
    LDP_TLV_Header tlvHeader;
    LDP_Common_Hello_Parameters_TLV_Value_Type helloParms;

    char* packetPtr = MESSAGE_ReturnPacket(msg);
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    memcpy(&tlvHeader, packetPtr, sizeof(LDP_TLV_Header));

    memcpy(&helloParms,
        (packetPtr + sizeof(LDP_TLV_Header)),
        sizeof(LDP_Common_Hello_Parameters_TLV_Value_Type));

    if (MPLS_LDP_DEBUG)
    {
        printf("\tTLV: U = %d / F = %d / Type = %d / Length = %d\n"
               "\t\tValue: Hold_Time = %d / T = %d / R = %d\n"
               "## node->nodeId %d\n",
               LDP_TLV_HeaderGetU(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetF(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetType(tlvHeader.tlvHdr),
               tlvHeader.Length,
               helloParms.Hold_Time,
               LDP_Common_Hello_Parameters_TLV_ValueGetT
               (helloParms.lchpTlvValType),
               LDP_Common_Hello_Parameters_TLV_ValueGetR
               (helloParms.lchpTlvValType),
               node->nodeId);
    }

    if (LDP_TLV_HeaderGetType(tlvHeader.tlvHdr) !=
        LDP_COMMON_HELLO_PARAMETERS_TLV)
    {
        if (MPLS_LDP_DEBUG)
        {
            printf("\t\tIncorrectly formatted Hello Message\n"
                   "\t\tCommon Hello Parameters s a mandatory"
                   " element so ignore\n");
        }

        // Incorrectly formatted Hello Message - Common Hello Parameters
        // is a mandatory element so ignore this Hello Message
        return;
    }

    // rfc 3036: A Link Hello is acceptable if the interface on which
    // it was received has been configured for label switching.
    //
    // If the Hello is acceptable, the LSR checks whether it has a
    // Hello adjacency for the Hello source.  If so, it restarts the
    // hold timer for the Hello adjacency.  If not it creates a Hello
    // adjacency for the Hello source and starts its hold timer.

    MplsLdpUpdateOrEstablishHelloAdjacency(
        node,
        sourceAddr,
        LSR_Id,
        incomingInterfaceIndex);

    if (tlvHeader.Length >
        sizeof(LDP_Common_Hello_Parameters_TLV_Value_Type))
    {
        // tlvHeader.Length is greater than
        // sizeof(LDP_Common_Hello_Parameters_TLV_Value_Type then some
        // Optional Parameters are present in this hello message

        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr," MPLS node %d received a Hello Message with"
            " optional parameters.  Support not yet coded.\n",
            node->nodeId);

        ERROR_ReportError(errorStr);
    }

    // increment number of link hello message received
    INCREMENT_STAT_VAR(numUdpLinkHelloReceived);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpChecksSessionParametersAreAcceptable()
//
// PURPOSE      : checking if session parameters are acceptable or not
//
// PARAMETERS   : commonSession - commonSessionTLV
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
BOOL MplsLdpChecksSessionParametersAreAcceptable(
    LDP_Common_Session_TLV_Value_Type  commonSession)
{
    // Currently we assume that the session parameters
    // are always acceptable
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessInitializationMessageInPassive()
//
// PURPOSE      : processing initialization message when LSR is in
//                passive mode.RFC 3036.Section 2.5.2
//
// PARAMETERS   : node - the node which is receiving the initialization
//                       message in the passive mode
//                commonSession - commonSessionTLV
//                sourceLSR_Id - source LSR Id
//                helloAdjacency - pointer to hello adjacency from
//                                 which the message is received.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpProcessInitializationMessageInPassive(
    Node* node,
    LDP_Common_Session_TLV_Value_Type commonSession,
    MplsLdpHelloAdjacency* helloAdjacency)
{
    LDP_Identifier_Type  ldp_identifier;
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    // If there is a matching Hello adjacency, the adjacency
    // specifies the local label space for the session.
    ldp_identifier.LSR_Id = helloAdjacency->LSR2.LSR_Id;
    ldp_identifier.label_space_id = helloAdjacency->LSR2.label_space_id;

    if (MplsLdpChecksSessionParametersAreAcceptable(commonSession))
    {
        // Next LSR1 checks whether the session parameters proposed in
        // the message are acceptable.  If they are, LSR1 replies with
        // an Initialization message of its own to propose the
        // parameters it wishes to use and a KeepAlive message to
        // signal acceptance of LSR2's parameters.
        MplsLdpSendInitializationMessage(
            node,
            ldp->keepAliveTime,
            ldp->currentMessageId,
            helloAdjacency,
            helloAdjacency->connectionId);

/*** Code for MPLS integration with IP start ***/
        // transmiting each message individually
        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            helloAdjacency);
/*** Code for MPLS integration with IP end ***/
        ldp->currentMessageId++;

        MplsLdpSendKeepAliveMessage(
            node,
            ldp->currentMessageId,
            helloAdjacency);

        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            helloAdjacency);

        ldp->currentMessageId++;
    }
    else
    {
        // If the parameters are not acceptable, LSR1 responds by
        // sending a Session Rejected/Parameters Error Notification
        // message and closing the TCP connection.
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "node %d received an Initialization Msg in "
                "passive mode with unacceptable params.\n"
                "  This is not currently supported.\n", node->nodeId);

        ERROR_ReportError(errorStr);
    }

    // Increment number of initialization message received in passive mode
    INCREMENT_STAT_VAR(numInitializationMsgReceivedAsPassive);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessInitializationMessageInActive()
//
// PURPOSE      : processing initialization message when LSR is in
//                active mode. RFC 3036.Section 2.5.2
//
// PARAMETERS   : node - the node which is receiving the initialization
//                       message in the active mode
//                commonSession - commonSessionTLV
//                sourceLSR_Id - source LSR Id
//                helloAdjacency - pointer to hello adjacency from
//                                 which the message is received.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpProcessInitializationMessageInActive(
    Node*  node,
    LDP_Common_Session_TLV_Value_Type commonSession,
    MplsLdpHelloAdjacency* helloAdjacency)
{

    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    // When LSR1 plays the active role:
    //
    // a. If LSR1 receives an Error Notification message, LSR2 has
    //    rejected its proposed session and LSR1 closes the TCP
    //    connection.
    //
    // b. If LSR1 receives an Initialization message, it checks
    //    whether the session parameters are acceptable.  If so, it
    //    replies with a KeepAlive message.  If the session parameters
    //    are unacceptable, LSR1 sends a Session Rejected/Parameters
    //    Error Notification message and closes the connection.
    //
    // c. If LSR1 receives a KeepAlive message, LSR2 has accepted its
    //    proposed session parameters.
    //
    // d. When LSR1 has received both an acceptable Initialization
    //    message and a KeepAlive message the session is operational
    //    from LSR1's point of view.

    if (MplsLdpChecksSessionParametersAreAcceptable(commonSession))
    {
        MplsLdpSendKeepAliveMessage(
            node,
            ldp->currentMessageId,
            helloAdjacency);

        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            helloAdjacency);
     }
    else
    {
        // send notification message
        // This portion is left to be done for
        // further extension
    }

    // Increment initialization message received in active mode
    INCREMENT_STAT_VAR(numInitializationMsgReceivedAsActive);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessInitializationMessage()
//
// PURPOSE      : processing initialization message
//
// PARAMETER    : node - the node which is receiving initialization message
//                msg - the initialization message
//                msgHeader - message Header of the incoming message
//                sourceLSR_Id - source LSR_Id
//                tcpConnId - transport connection id
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpProcessInitializationMessage(
    Node* node,
    Message* msg,
    LDP_Identifier_Type sourceLSR_Id,
    int tcpConnId)
{
    LDP_TLV_Header tlvHeader;
    LDP_Common_Session_TLV_Value_Type commonSession;
    MplsLdpHelloAdjacency*  helloAdjacency = NULL;

    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    // extract the packet from message
    char* packetPtr = MESSAGE_ReturnPacket(msg);

    ERROR_Assert(ldp, "Error in MplsLdpProcessInitializationMessage()"
        " MPLS_LDP is not Initialized !!!");

    // get the hello adjacency from which the initialization
    // messege is received
    helloAdjacency = MplsLdpReturnHelloAdjacency(
                         ldp,
                         ANY_IP,
                         sourceLSR_Id,
                         FALSE,
                         TRUE);

    // extract TLV_header from the packet into "tlvHeader"
    memcpy(&tlvHeader, packetPtr, sizeof(LDP_TLV_Header));

    // extract the LDP_Common_Session_TLV_Value_Type into "commonSession"
    memcpy(&commonSession,
        (packetPtr + sizeof(LDP_TLV_Header)),
        COMMON_SESSION_TLV_SIZE);

    if (MPLS_LDP_DEBUG)
    {
        printf("node %d processing initialization message\n"
               "TLV: U = %d / F = %d / Type = %d / Length = %d\n"
               "Value: protocol version = %d / keepalive_time = %d\n"
               "A = %d / D = %d / PVlim = %d / pdu_length = %d\n",
               node->nodeId,
               LDP_TLV_HeaderGetU(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetF(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetType(tlvHeader.tlvHdr),
               tlvHeader.Length,
               commonSession.protocol_version,
               commonSession.keepalive_time,
               LDPCommonSessionTLVGetA(commonSession.ldpCsTlvValType),
               LDPCommonSessionTLVGetD(commonSession.ldpCsTlvValType),
               LDPCommonSessionTLVGetPVL(commonSession.ldpCsTlvValType),
               commonSession.max_pdu_length);
    }


    if (LDP_TLV_HeaderGetType(tlvHeader.tlvHdr) !=
        LDP_COMMON_SESSION_PARAMETER_TLV)
    {
        // Incorrectly Initialization Message so return
        return;
    }

    if (tlvHeader.Length > COMMON_SESSION_TLV_SIZE)
    {
        // Optional Parameters are present in this hello message
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr," node %d received a Initialization message with "
                "optional parameters.  Support not yet coded.\n",
                node->nodeId);

        ERROR_ReportError(errorStr);
    }

    if (helloAdjacency)
    {
        helloAdjacency->connectionId = tcpConnId;
        if (helloAdjacency->initMode == MPLS_LDP_ACTIVE_INITIALIZATION_MODE)
        {
            assert(helloAdjacency->connectionId);
            // Mpls Ldp Process Initialization message in Active mode
            MplsLdpProcessInitializationMessageInActive(
                node,
                commonSession,
                helloAdjacency);
        }
        else
        {
            assert(helloAdjacency->connectionId);
            // Mpls Ldp Process Initailization in Passive mode
            MplsLdpProcessInitializationMessageInPassive(
                node,
                commonSession,
                helloAdjacency);
        }
    }
    else // if (no hello adjacency exists)
    {
        Message* timerMsg = NULL;

        helloAdjacency = MplsLdpCreateHelloAdjacency(
                             node,
                             ldp,
                             sourceLSR_Id.LSR_Id,
                             sourceLSR_Id,
                             ldp->targetedHelloInterval,
                             ANY_INTERFACE);

        timerMsg = MESSAGE_Alloc(node,
                       APP_LAYER,
                       APP_MPLS_LDP,
                       MSG_APP_MplsLdpSendKeepAliveDelayExpired);

        MESSAGE_InfoAlloc(
            node,
            timerMsg,
            sizeof(NodeAddress));

        memcpy(MESSAGE_ReturnInfo(timerMsg),
            &(helloAdjacency->LSR2.LSR_Id),
            sizeof(NodeAddress));

        MESSAGE_Send(node, timerMsg, KEEP_ALIVE_DELAY(ldp));

        helloAdjacency->connectionId = tcpConnId;

        ERROR_Assert(helloAdjacency->initMode ==
                     MPLS_LDP_PASSIVE_INITIALIZATION_MODE,
            "Error in MplsLdpProcessInitializationMessage()"
            " initialization mode should be passive !!!");

        // Mpls Ldp Process Initailization in Passive mode
        MplsLdpProcessInitializationMessageInPassive(
            node,
            commonSession,
            helloAdjacency);
    }

    // An LSR that receives an Address Message message uses the addresses
    // it learns to maintain a database for mapping between peer LDP
    // Identifiers and next hop addresses; .
    //
    // When a new LDP session is initialized and before sending Label
    // Mapping or Label Request messages an LSR should advertise its
    // interface addresses with one or more Address messages.
    //
    // Whenever an LSR "activates" a new interface address, it should
    // advertise the new address with an Address message.
    //
    // Ref RFC 3036 section 3.5.5.1
    MplsLdpSendAddressMessage(
        node,
        ldp->currentMessageId,
        helloAdjacency);

    // send the pdu

    MplsLdpTransmitPduFromHelloAdjacencyBuffer(
        node,
        ldp,
        helloAdjacency);

    ldp->currentMessageId ++;

    // A LDP session starts after exchanging initialization message.
    // Hence start a keep alive timer
    MplsLdpStartKeepAliveTimerForThisHelloAdjacency(
        node,
        ldp,
        helloAdjacency);

    // increment total number of initialization message received
    INCREMENT_STAT_VAR(numInitializationMsgReceived);
}

//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessKeepAliveMessage()
//
// PURPOSE      : process keep alive message
//
// PARAMETER    : node :- receiving node
//                msg :- incoming message header
//                source port :- source port address
//                msgHeader :- message Header of the incoming message
//                LSR_Id :- source LSR_Id
//                tcpConnId :- transport connection id
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpProcessKeepAliveMessage(
    Node* node,
    Message* msg,
    int tcpConnId)
{
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);
    if (MPLS_LDP_DEBUG)
    {
        unsigned int msgSize = MESSAGE_ReturnPacketSize(msg);
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);
        printf("#%d: Received keep alive Message at %s size = %u\n"
               "through TCP connection Id %d\n",
               node->nodeId,
               buf,
               msgSize,
               tcpConnId);
    }
    // increment number of keep alive message received.
    INCREMENT_STAT_VAR(numKeepAliveMsgReceived);
}


//-------------------------------------------------------------------------
//  FUNCTION   : MplsLdpProcessAddressMessage()
//
//  PURPOSE    : processing adderss message
//
//  PARAMETER  : node - node which is receiving the address message
//               msg - incoming address message
//               msgHeader - message Header of the incoming message
//               sourceLSR_Id - source LSR_Id
//               tcpConnId - transport connection id
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpProcessAddressMessage(
    Node* node,
    Message* msg,
    LDP_Identifier_Type sourceLSR_Id,
    int tcpConnId)
{
    LDP_TLV_Header tlvHeader;
    LDP_Address_List_TLV_Value_Header_Type addressListHeader;
    NodeAddress* addressList = NULL;
    MplsLdpHelloAdjacency* helloAdjacency = NULL;
    MplsLdpApp* ldp = NULL;
    unsigned int lengthOfOptionalField = 0;
    int notUsed = 0;

    // extract the packet from message
    char* packetPtr = MESSAGE_ReturnPacket(msg);
    memcpy(&tlvHeader, packetPtr, sizeof(LDP_TLV_Header));

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d has received amessage from: %u connId = %d\n"
               "sizeopf the packet is %d\n"
               "U = %d / F = %d / Type = %d Length = %d\n",
               node->nodeId,
               sourceLSR_Id.LSR_Id,
               tcpConnId,
               MESSAGE_ReturnPacketSize(msg),
               LDP_TLV_HeaderGetU(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetF(tlvHeader.tlvHdr),
               LDP_TLV_HeaderGetType(tlvHeader.tlvHdr),
               tlvHeader.Length);
    }

    // extract the LDP_Common_Session_TLV_Value_Type into
    // "addressListHeader"
    memcpy(&addressListHeader,
        packetPtr + sizeof(LDP_TLV_Header),
        LDP_ADDRESS_LIST_TLV_SIZE);

    // calculate the length of optional field.Optinnal fields is
    // supposed to carry only addresses.
    lengthOfOptionalField = (tlvHeader.Length - LDP_ADDRESS_LIST_TLV_SIZE);

    ERROR_Assert((lengthOfOptionalField % sizeof(NodeAddress)) == 0,
       "Error in MplsLdpProcessAddressMessage()"
       " length of  optional fields it should be multipul of 4 !!!");

    // allocate equivalant amount of memory space
    addressList = (NodeAddress*) MEM_malloc(lengthOfOptionalField);

    // extract the addersses into "addressList"
    memcpy(addressList,
        (packetPtr + sizeof(LDP_TLV_Header) + LDP_ADDRESS_LIST_TLV_SIZE),
        lengthOfOptionalField);

    ldp = MplsLdpAppGetStateSpace(node);

    ERROR_Assert(ldp,"Error in MplsLdpProcessAddressMessage()"
        " MplsLdpApp is NULL !!!");

    helloAdjacency = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                         ldp,
                         sourceLSR_Id.LSR_Id);

    ERROR_Assert(helloAdjacency,"Error in"
        " MplsLdpProcessAddressMessage()"
        " Hello Adjcency not found !!!");

    if (addressListHeader.Address_Family != IP_V4_ADDRESS_FAMILY)
    {
        // If an LSR does not support the Address Family specified
        // in the Address List TLV, it should send an "Unsupported
        // Address Family" Notification to its LDP signalling an error
        // and abort processing the message.
        MplsLdpSendNotificationMessage(
            node,
            sourceLSR_Id.LSR_Id,
            ldp->currentMessageId,
            UNSUPPORTED_ADDRESS_FAMILY,
            0,
            LDP_ADDRESS_LIST_MESSAGE_TYPE,
            FATAL_ERROR_NOTIFICATION,
            FALSE,
            ldp,
            NULL,
            helloAdjacency);

        // send the pdu

        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            helloAdjacency);

        return;
    }

    helloAdjacency->numInterfaceAddress = lengthOfOptionalField;

    helloAdjacency->interfaceAddresses = (NodeAddress*)
        MEM_malloc(lengthOfOptionalField);

    memcpy(helloAdjacency->interfaceAddresses,
       addressList,
       lengthOfOptionalField);

    if (MPLS_LDP_DEBUG)
    {
        int i = 0;
        int numInterface = (lengthOfOptionalField / sizeof(NodeAddress));

        printf("#%d received an  address message from %u\n"
               "The source LSR has %d interfaces\n"
               "They are :",
               node->nodeId,
               sourceLSR_Id.LSR_Id,
               numInterface);

        for (i = 0; i < numInterface ; i++)
        {
            printf("%d, ", addressList[i]);
        }

        printf("Link Address of This LSR to source"
               " is %u interfaceId = %d\n",
               helloAdjacency->LinkAddress,
               notUsed);
    }
/*** Code for MPLS integration with IP start ***/
    // This has been done so that label requests are
    // done after address messages are sent and processed
    helloAdjacency->OkToSendLabelRequests = TRUE;
/*** Code for MPLS integration with IP end ***/

    MEM_free(addressList);

    // increment number of address message received
    INCREMENT_STAT_VAR(numAddressMsgReceived);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpPerformLabelDistributionProcedure()
//
// PURPOSE      : executes label distribution procedure
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structure.
//                fec - fec tobe send with the label mapping message
//                peer - pointer to peer LSR's record in the
//                       hello adjacency structure
//                SAttribute - attribute to be sent with label
//                              mapping message
//                RAttribute - received attribute value
//                label_retained_if_any - return label retained if any
//                labelSentToUpstreamPeer - return label sent to
//                                              upstream peer
//                helloAdjacencyNext - pointer to peer LSR's record
//                                     in the hello adjacency structure
//                                     to be used as next hop.
//                sourceLSR_Id - node from which label request has been
//                               received
//
// RETURN VALUE :
//-------------------------------------------------------------------------
static
BOOL MplsLdpPerformLabelDistributionProcedure(
    Node* node,
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    MplsLdpHelloAdjacency* peer,
    Attribute_Type* SAttribute,
    Attribute_Type* RAttribute,
    unsigned int* label_retained_if_any,
    unsigned int* labelSentToUpstreamPeer,
    MplsLdpHelloAdjacency* helloAdjacencyNext,
    NodeAddress sourceLSR_Id)
{
    Attribute_Type* RAttributeNextHop = NULL;
    FecToLabelMappingRec* hasLabel = NULL;
    BOOL isEgress = FALSE;
    BOOL isPropagating = FALSE;
    int prevHopCount = 0;
    unsigned int label = 0;

    // Check is LSR egress for FEC ?
    isEgress = IamEgressForThisFec(node, fec, ldp);

    if (helloAdjacencyNext)
    {
        // check has lsr previously received and retained a label mapping
        // for this FEC from the next hop?
        hasLabel = MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
                       ldp,
                       fec,
                       &label,
                       helloAdjacencyNext->LSR2.LSR_Id,
                       &RAttributeNextHop);
    }
    else // if (helloAdjacencyNext == NULL)
    {
        if (!isEgress)
        {
            // Deleate entries from pending label request list.
            MplsLdpDeleteDuplicateOrPendingRequest(ldp, fec, sourceLSR_Id);

            // do not do rest of the processing
            // goto LRq.10.
            return FALSE;
        }
        helloAdjacencyNext = peer;
    }

    if (hasLabel)
    {
        *label_retained_if_any = label;
        prevHopCount = RAttributeNextHop->numHop;
    }
    else
    {
        *label_retained_if_any = 3;
        prevHopCount = 0;
    }

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d in perform label distribution procedure\n"
               "\tisEgress = %s / hasLabel = %s\n",
               node->nodeId,
               isEgress ? "TRUE" : "FALSE",
               hasLabel ? "TRUE" : "FALSE");

        if (hasLabel)
        {
            printf("#%d has retained a label for fec == %d\n"
                   "Value of the label is : %d\n",
                   node->nodeId,
                   fec.ipAddress,
                   label);
        }
    }

    // 1. Has LSR previously received and retained a label
    //    mapping for FEC from Next Hop?.Is so, set Propagating to
    //    IsPropagating.If not, set Propagating to NotPropagating.
    if (hasLabel)
    {
        isPropagating = TRUE;
    }
    else
    {
        isPropagating = FALSE;
    }

    //  For Downstream Unsolicited Independent Control OR
    //  For Downstream On Demand Independent Control
    if (ldp->labelDistributionControlMode == MPLS_CONTROL_MODE_INDEPENDENT)
    {
        if (MPLS_LDP_DEBUG)
        {
            printf("#%d labelDistributionControlMode == INDEPENDENT"
                   " fec = %d \n got label request message from %u\n"
                   "sending a label mapping message immediately to %u\n",
                   node->nodeId,
                   fec.ipAddress,
                   peer->LSR2.LSR_Id,
                   peer->LSR2.LSR_Id);
        }

        // 2.Execute procedure
        //   MplsLdpPrepareLabelMappingAttributes(MsgSource, FEC,
        //   RAttributes, SAttributes, Propagating, StoredHopCount).
        MplsLdpPrepareLabelMappingAttributes(
            node,
            ldp,
            peer,
            fec,
            RAttribute,
            SAttribute,
            isPropagating,
            prevHopCount);

        // 3. Execute procedure MplsLdpSendLabel (MsgSource, FEC,
        //         SAttributes).
        *labelSentToUpstreamPeer =
            MplsLdpSendLabel(node, ldp, peer, fec, SAttribute, TRUE);

        //  4. Is LSR egress for FEC? OR
        //     Has LSR previously received and retained a label
        //     mapping for FEC from Next Hop?
        //          If so, goto LRq.11.
        //          If not, goto LRq.10.
        if ((hasLabel) || (isEgress))     // hasLabel == 0 if FALSE
        {                                 // isEgress == 0 if FALSE
            // goto LRq.11.
            return TRUE;
        }
        else
        {   // goto LRq.10.
            return FALSE;
        }
    }
    else if (ldp->labelDistributionControlMode == MPLS_CONTROL_MODE_ORDERED)
    {
        //  For Downstream Unsolicited Ordered Control OR
        //  For Downstream On Demand Ordered Control

        //  1. Is LSR egress for FEC? OR
        //     Has LSR previously received and retained a label
        //     mapping for FEC from Next Hop?  (See Note 3.)
        //     If not, goto LRq.10.
        if ((!hasLabel) && (!isEgress))   //hasLabel == 0 if FALSE
        {                                 //isEgress == 0 if FALSE
            // goto LRq.10.
            return FALSE;
        }
        else
        {
            //  2. Execute procedure
            //     MplsLdpPrepareLabelMappingAttributes(MsgSource, FEC,
            //     RAttributes, SAttributes, IsPropagating,
            //     StoredHopCount)
            MplsLdpPrepareLabelMappingAttributes(
                node,
                ldp,
                peer,
                fec,
                RAttributeNextHop, //RAttribute ,
                SAttribute,
                isPropagating,
                prevHopCount);

            // 3. Execute procedure MplsLdpSendLabel (MsgSource, FEC,
            //      SAttributes).
            //      Goto LRq.11.
            *labelSentToUpstreamPeer =
                MplsLdpSendLabel(node, ldp, peer, fec, SAttribute, TRUE);

            // goto LRq.11.
            return TRUE;
        }
    }
    // control shouldnot reach this block of code in any sitition
    ERROR_Assert(FALSE, "Error in MplsLdpPerformLabelDistributionProcedure()"
        " Unknown LabelDistributionControlMode !!!");

    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessLabelRequestAndGenerateResponse()
//
// PURPOSE      : Generate response to a label request message.
//
// PARAMETERS   : node - node which is generating response to a label
//                       request message.
//                ldp - pointer to the MplsLdpApp structure.
//                sourceLSR_Id - LSR_Id of the LSR from which label request
//                               message is received
//                fec - The FEC specified in the message.
//                RAttribute - Attributes received with the message
//                ref_msg_Id - message Id of the received message
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpProcessLabelRequestAndGenerateResponse(
    Node* node,
    MplsLdpApp* ldp,
    NodeAddress sourceLSR_Id,
    Mpls_FEC fec,
    Attribute_Type* RAttribute,
    unsigned int ref_msg_Id)
{
    // A.1.1. Receive Label Request
    //
    // Summary:
    //
    // The response by an LSR to receipt of a FEC label request from an
    // LDP peer may involve one or more of the following actions:
    //
    // -  Transmission of a notification message to the requesting LSR
    //    indicating why a label mapping for the FEC cannot be provided;
    //
    // -  Transmission of a FEC label mapping to the requesting LSR;
    //
    // -  Transmission of a FEC label request to the FEC next hop;
    //
    // -  Installation of labels for forwarding/switching use by the LSR.
    //
    // Context:
    //
    // -  LSR.  The LSR handling the event.
    //
    // -  MsgSource.  The LDP peer that sent the message.
    //
    // -  FEC.  The FEC specified in the message.
    //
    // -  RAttributes.  Attributes received with the message.  E.g., Hop
    //    Count, Path Vector.
    //
    // -  SAttributes.  Attributes to be included in Label Request
    //    message, if any, propagated to FEC Next Hop.
    //
    // -  StoredHopCount.  The hop count, if any, previously recorded for
    //    the FEC.

    MplsLdpHelloAdjacency* helloAdjacencyNext = NULL;
    BOOL successfullySent = FALSE;
    NodeAddress nextHop = MPLS_LDP_INVALID_PEER_ADDRESS;
    BOOL egressOrHasLabel = FALSE;
    unsigned int label_retained_if_any = 0;
    unsigned int labelSentToUpstreamPeer = 0;
    NodeAddress upstream = MPLS_LDP_INVALID_PEER_ADDRESS;
    Attribute_Type SAttribute;
    LDP_Identifier_Type notUsed;

    memset(&notUsed, 0, sizeof(LDP_Identifier_Type));

    SAttribute.hopCount = FALSE;             // Initialize the SAttribute
    SAttribute.pathVector = FALSE;           // Structure before useing it,
    SAttribute.numHop = 0;                   // to prevent memory error.
    SAttribute.pathVectorValue.length = 0;
    SAttribute.pathVectorValue.pathVector = NULL;

    //  LRq.1  Execute procedure MplsLdpCheckReceivedAttributes (MsgSource,
    //         LabelRequest, RAttributes).
    //         If Loop Detected, goto LRq.13.
    if (MplsLdpCheckReceivedAttributes(
            ldp,
            sourceLSR_Id,
            LDP_LABEL_REQUEST_MESSAGE_TYPE,
            RAttribute) != LOOP_DETECTED)
    {
        MplsLdpHelloAdjacency* msgSource = NULL;

        if (MPLS_LDP_DEBUG)
        {
            printf("loop is not detected\n"
                   "#%d : received an label Request message from %u"
                   "for fec = %u\n",
                   node->nodeId,
                   sourceLSR_Id,
                   fec.ipAddress);
        }

        // LRq.2   Is there a Next Hop for FEC?
        //         If not, goto LRq.5.
        if (IsNextHopExistForThisDest(node, fec.ipAddress, &nextHop))
        {
            MplsLdpHelloAdjacency* helloAdjacency = NULL;

            helloAdjacency = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                                 ldp,
                                 sourceLSR_Id);

            // LRq.3   Is MsgSource the Next Hop?
            //         If not, goto LRq.6.
            if ((nextHop == sourceLSR_Id) ||
                (nextHop == helloAdjacency->LinkAddress))
            {
                // The following code has been added for integrating IP+MPLS.

                // if connection is established with the peer,
                // this means there is no route available.
                // hence send no route error
                OptionalParameters optionalParameter;

                // build optional parameter list for NO_ROUTE_ERROR
                // notification message. Only fec TLV is included
                // in the notification message
                optionalParameter.extendedStatus = NULL;
                optionalParameter.sizeReturnedPDU = 0;
                optionalParameter.returnedPDU = NULL;
                optionalParameter.sizeReturnedMessage = 0;
                optionalParameter.returnedMessage = NULL;
                optionalParameter.otherTLVtype = LDP_FEC_TLV;
                optionalParameter.otherdata = (unsigned char*) &fec;

                // LRq.4   Execute procedure Send_Notification (MsgSource,
                //         Loop Detected).Goto LRq.13

                // if sender of a label request message is itself the
                // next hop then there must be a loop. So
                // send LOOP_DETECTION_ERROR notification message

                MplsLdpSendNotificationMessage(
                    node,
                    sourceLSR_Id,
                    ldp->currentMessageId,
                    LOOP_DETECTED_ERROR,
                    ref_msg_Id,
                    LDP_LABEL_REQUEST_MESSAGE_TYPE,
                    FATAL_ERROR_NOTIFICATION,
                    FALSE,
                    ldp,
                    &optionalParameter,
                    helloAdjacency);

                // send the pdu

                MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                    node,
                    ldp,
                    helloAdjacency);

                if (MPLS_LDP_DEBUG)
                {
                    printf("node = %d nextHop = %u"
                           "LSR_Id = %u helloAdjacency->LinkAddr %u\n",
                           node->nodeId,
                           nextHop,
                           sourceLSR_Id,
                           helloAdjacency->LinkAddress);

                    MplsLdpListHelloAdjacency(node);
                }
                // LRq.13  DONE
                return;
            }
           //Added for IP-MPLS integration
            MplsLdpHelloAdjacency* helloAdjacency_nexthop = NULL;
            helloAdjacency_nexthop = MplsLdpReturnHelloAdjacency(
                                         ldp,
                                         nextHop,
                                         notUsed,
                                         TRUE,   // match link address
                                         FALSE); // do not match LSR_Id

            // Check that peer can be sent the label requests.
            // Check that next hop exists in hello adjacency.
            // Also to check that i am not the egress.
            //if (helloAdjacency_nexthop->connectionId == NULL)
            if (((helloAdjacency_nexthop == NULL) ||
                (helloAdjacency_nexthop->OkToSendLabelRequests == FALSE))
                && (!(IamEgressForThisFec(node,fec, ldp))))
            {
                // if connection is established with the peer,
                // this means there is no route available.
                // hence send no route error
                OptionalParameters optionalParameter;
                MplsLdpHelloAdjacency* helloAdjacency = NULL;

                helloAdjacency = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                                         ldp,
                                         sourceLSR_Id);

                // build optional parameter list for NO_ROUTE_ERROR
                // notification message. Only fec TLV is included
                // in the notification message
                optionalParameter.extendedStatus = NULL;
                optionalParameter.sizeReturnedPDU = 0;
                optionalParameter.returnedPDU = NULL;
                optionalParameter.sizeReturnedMessage = 0;
                optionalParameter.returnedMessage = NULL;
                optionalParameter.otherTLVtype = LDP_FEC_TLV;
                optionalParameter.otherdata = (unsigned char*) &fec;

                // LRq.5   Execute procedure Send_Notification (MsgSource,
                //         No Route).Goto LRq.13
                //         if next hop does not exists for the FEC then
                //         send NO_ROUTE_ERROR notification message
                MplsLdpSendNotificationMessage(
                    node,
                    sourceLSR_Id,
                    ldp->currentMessageId,
                    NO_ROUTE_ERROR,
                    ref_msg_Id,
                    LDP_LABEL_REQUEST_MESSAGE_TYPE,
                    FATAL_ERROR_NOTIFICATION,
                    FALSE,
                    ldp,
                    &optionalParameter,
                    helloAdjacency);

                // send the pdu

                MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                    node,
                    ldp,
                    helloAdjacency);

                // LRq.13  DONE
                return;
            }  // end of if helloAdjacency_nexthop->connectionId == NULL
        }
        else
        {
            OptionalParameters optionalParameter;
            MplsLdpHelloAdjacency* helloAdjacency = NULL;

            helloAdjacency = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                                     ldp,
                                     sourceLSR_Id);

            // build optional parameter list for NO_ROUTE_ERROR
            // notification message. Only fec TLV is included
            // in the notification message
            optionalParameter.extendedStatus = NULL;
            optionalParameter.sizeReturnedPDU = 0;
            optionalParameter.returnedPDU = NULL;
            optionalParameter.sizeReturnedMessage = 0;
            optionalParameter.returnedMessage = NULL;
            optionalParameter.otherTLVtype = LDP_FEC_TLV;
            optionalParameter.otherdata = (unsigned char*) &fec;

            // LRq.5   Execute procedure Send_Notification (MsgSource,
            //         No Route).Goto LRq.13
            //         if next hop do not exists for the FEC then
            //         send NO_ROUTE_ERROR notification message
            MplsLdpSendNotificationMessage(
                node,
                sourceLSR_Id,
                ldp->currentMessageId,
                NO_ROUTE_ERROR,
                ref_msg_Id,
                LDP_LABEL_REQUEST_MESSAGE_TYPE,
                FATAL_ERROR_NOTIFICATION,
                FALSE,
                ldp,
                &optionalParameter,
                helloAdjacency);

            // send the pdu

            MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                node,
                ldp,
                helloAdjacency);

            // LRq.13  DONE
            return;
        }

        // LRq.6  Has LSR previously received a label request for FEC from
        //        MsgSource? If not, goto LRq.8
        if (AnyRequestPreviouslyReceivedFromMsgSource(
                ldp,
                fec,
                sourceLSR_Id))
        {
            // LRq.7   Is the label request a duplicate request?
            //        If so, Goto LRq.13.  (See Note 2.)
            if (IsItDuplicateOrPendingRequest(
                    ldp,
                    fec,
                    sourceLSR_Id,
                    &upstream,
                    TRUE))
            {
                if (MPLS_LDP_DEBUG)
                {
                    printf("node %d has got a duplicate label request\n"
                           "from %u : fec.ipAddress = %u\n",
                           node->nodeId,
                           sourceLSR_Id,
                           fec.ipAddress);
                }
                // LRq.13  DONE
                return;
            }
        }

        //  LRq.8   Record label request for FEC received from MsgSource and
        //         mark it pending.
        MplsLdpAddRowToThePendingRequestList(
            ldp,
            fec,
            nextHop,
            sourceLSR_Id);

        // LRq.9   Perform LSR Label Distribution procedure
        msgSource = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                        ldp,
                        sourceLSR_Id);

        helloAdjacencyNext = MplsLdpReturnHelloAdjacency(
                                 ldp,
                                 nextHop,
                                 notUsed,
                                 TRUE,   // match link address
                                 FALSE); // do not match LSR_Id

        egressOrHasLabel = MplsLdpPerformLabelDistributionProcedure(
                               node,
                               ldp,
                               fec,
                               msgSource,
                               &SAttribute,
                               RAttribute,
                               &label_retained_if_any,
                               &labelSentToUpstreamPeer,
                               helloAdjacencyNext,
                               sourceLSR_Id);

        if (!egressOrHasLabel)
        {
            LabelRequestStatus requestStatus = Request_On_Request;

            if (MPLS_LDP_DEBUG)
            {
                if (!helloAdjacencyNext)
                {
                    printf("#%d nextHop == %u NOT FOUND  fec = %d"
                           "%d is not a LDP peer\n",
                           node->nodeId,
                           nextHop,
                           fec.ipAddress,
                           nextHop);
                }
                else
                {
                    printf("#%d executing MplsLdpPerformLabelRequest"
                           " Procedure nextHop == %u fec = %d\n",
                           node->nodeId,
                           nextHop,
                           fec.ipAddress);
                }
                MplsLdpListHelloAdjacency(node);
            }

            // NOTE : The ERROR_Assert() statement can be un-commented
            //        below for debugging. Under normal circumstances,
            //        i.e. without linl/node fault "helloAdjacencyNext"
            //        SHOULD NOT BE NULL.
            //
            // ERROR_Assert(helloAdjacencyNext, "Error in"
            //        " MplsLdpProcessLabelRequestAndGenerateResponse()"
            //        " helloAdjacencyNext is NULL !!!");

            if (!helloAdjacencyNext)
            {
                // If hello adjacency do not exists then
                // do not make label request. Change
                // requestStatus to Request_Never
                requestStatus = Request_Never;
            }

            // LRq.10  Perform LSR Label Request procedure:
            successfullySent = MplsLdpPerformLabelRequestProcedure(
                                   node,
                                   ldp,
                                   helloAdjacencyNext,
                                   fec,
                                   RAttribute,
                                   &SAttribute,
                                   sourceLSR_Id,
                                   requestStatus);
        }

        //  LRq.11 Has LSR successfully sent a label for FEC to MsgSource?
        //         If not, goto LRq.13.  (See Note 4.)
        if (successfullySent)
        {
            // LRq.13  DONE
            return;
        }
        else
        {
            if (MPLS_LDP_DEBUG)
            {
                printf("#%d is performing Lable Use Procedure\n",
                       node->nodeId);
            }

            // LRq.12  Perform Label Use procedure.
            MplsLdpPerformLabelUseProcedure(
                node,
                ldp,
                fec,
                nextHop,
                labelSentToUpstreamPeer,
                label_retained_if_any);
        }
    }
    else
    {
        MplsLdpHelloAdjacency* helloAdjacency = NULL;
        helloAdjacency = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                             ldp,
                             sourceLSR_Id);

        // The following code has been added for integrating IP + MPLS.

        // if connection is established with the peer,
        // this means there is no route available.
        // hence send no route error
        OptionalParameters optionalParameter;

        // build optional parameter list for NO_ROUTE_ERROR
        // notification message. Only fec TLV is included
        // in the notification message
        optionalParameter.extendedStatus = NULL;
        optionalParameter.sizeReturnedPDU = 0;
        optionalParameter.returnedPDU = NULL;
        optionalParameter.sizeReturnedMessage = 0;
        optionalParameter.returnedMessage = NULL;
        optionalParameter.otherTLVtype = LDP_FEC_TLV;
        optionalParameter.otherdata = (unsigned char*) &fec;

        // NOTE : Alogrithm for MplsLdpCheckReceivedAttributes()
        //        specifies that SendNotificationMessage() may be
        //        executed with in that function if loop detected.
        //        Well called it here. So it do not exactly follow
        //        the statement in LRq.1. Also see the NOTE in
        //        MplsLdpCheckReceivedAttributes().

        MplsLdpSendNotificationMessage(
            node,
            sourceLSR_Id,
            ldp->currentMessageId,
            LOOP_DETECTED_ERROR,
            ref_msg_Id,
            LDP_LABEL_REQUEST_MESSAGE_TYPE,
            FATAL_ERROR_NOTIFICATION,
            FALSE,
            ldp,
            &optionalParameter,
            helloAdjacency);

        // send the pdu
        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            helloAdjacency);
    }
    // LRq.13 DONE
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessLabelRequestMessage()
//
// PURPOSE      : processing label request message.
//
// PARAMETERS   : node - node which is processing the label request message
//                msg - the label request message
//                msgHeader - the message header
//                LSR_Id - LSR_Id of the LSR from which label request
//                         message is received
//                tcpConnId - TCP connection Id.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpProcessLabelRequestMessage(
    Node* node,
    Message* msg,
    LDP_Message_Header* msgHeader,
    LDP_Identifier_Type sourceLSR_Id,
    int tcpConnId)
{
    MplsLdpApp* ldp = NULL;

    char* receivedData = NULL;
    int receivedDataSize = 0;
    MplsLdpHelloAdjacency* helloAdjacency = NULL;
    LDP_TLV_Header tlvHeader;
    unsigned int numOptionalFields = 0;

    LDP_Prefix_FEC_Element fecTLV;
    NodeAddress* nodeAddressList = NULL;
    int numAddress = 0;

    LDP_HopCount_TLV_Value_Type* hopCountValue = NULL;

    NodeAddress* pathVectorTLV = NULL;
    int hopCountInPathVector = 0;

    Attribute_Type RAttribute;
    Mpls_FEC fec;

    ldp = MplsLdpAppGetStateSpace(node);

    ERROR_Assert(ldp, "Error in MplsLdpProcessLabelRequestMessage()"
        " MplsLdpApp is NULL !!!");

    receivedDataSize = (msgHeader->Message_Length - LDP_MESSAGE_ID_SIZE);
    receivedData = MESSAGE_ReturnPacket(msg);

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d MplsLdpReceive Label request message from %u\n"
               " size of the packet is %d"
               " messaage type == %x"
               " message length == %d"
               " Through Tcp connId = %d\n",
               node->nodeId,
               sourceLSR_Id.LSR_Id,
               receivedDataSize,
               LDP_Message_HeaderGetMsgType(msgHeader->ldpMsg),
               msgHeader->Message_Length,
               tcpConnId);
    }

    // Remove Fec TLV header
    memcpy(&tlvHeader, receivedData, sizeof(LDP_TLV_Header));
    receivedData = receivedData + sizeof(LDP_TLV_Header);
    receivedDataSize = (receivedDataSize - sizeof(LDP_TLV_Header));

    if (MPLS_LDP_DEBUG)
    {
        printf("fecTLV header removed\n"
               "tlvHeader.Length = %d"
               " tlvHeader.Type = %d\n",
               tlvHeader.Length,
               LDP_TLV_HeaderGetType(tlvHeader.tlvHdr));
    }

    // Get fec TLV value type
    memcpy(&fecTLV, receivedData, sizeof(LDP_Prefix_FEC_Element));
    receivedData = receivedData + sizeof(LDP_Prefix_FEC_Element);
    receivedDataSize = receivedDataSize - sizeof(LDP_Prefix_FEC_Element);

    if (MPLS_LDP_DEBUG)
    {
        printf("Address_Family = %d PreLen = %d Type = %d\n",
               fecTLV.Prefix.Address_Family,
               fecTLV.Prefix.PreLen,
               fecTLV.Element_Type);
    }

    // Rest of the value i.e
    // (tlvHeader.Length - sizeof(LDP_Prefix_FEC_Element) is set of
    // addresses. Extract ...
    numAddress = (tlvHeader.Length - sizeof(LDP_Prefix_FEC_Element))
                 / sizeof(NodeAddress);

    nodeAddressList = (NodeAddress*)
        MEM_malloc(tlvHeader.Length - sizeof(LDP_Prefix_FEC_Element));

    memcpy(nodeAddressList,
        receivedData,
        (tlvHeader.Length - sizeof(LDP_Prefix_FEC_Element)));

    receivedData = receivedData + (numAddress * sizeof(NodeAddress));
    receivedDataSize = receivedDataSize - (numAddress * sizeof(NodeAddress));

    if (MPLS_LDP_DEBUG)
    {
        int i = 0;
        printf("Number of addresses = %d printing addreses .....\n",
                numAddress);

        for (i = 0; i < numAddress; i++)
        {
            printf("nodeAdressList %d ",nodeAddressList[i]);
        }

        printf("This Label request message is for the fec = %u\n",
               nodeAddressList[0]);
    }

    // hunt for other optional field
    while (receivedDataSize > 0)
    {
        LDP_TLV_Header tlvHeader;

        // extract the next TLV header
        memcpy(&tlvHeader, receivedData, sizeof(LDP_TLV_Header));
        receivedData = receivedData + sizeof(LDP_TLV_Header);
        receivedDataSize = receivedDataSize -  sizeof(LDP_TLV_Header);

        if (MPLS_LDP_DEBUG)
        {
            printf("received optional field no %d\n"
                   "received Optional field of type %d"
                   " of length %d\n",
                   ++(numOptionalFields),
                   LDP_TLV_HeaderGetType(tlvHeader.tlvHdr),
                   tlvHeader.Length);
        }

        switch (LDP_TLV_HeaderGetType(tlvHeader.tlvHdr))
        {
            case LDP_HOP_COUNT_TLV :
            {
                hopCountValue = (LDP_HopCount_TLV_Value_Type*)
                    MEM_malloc(sizeof(LDP_HopCount_TLV_Value_Type));

                memcpy(hopCountValue,
                    receivedData,
                    sizeof(LDP_HopCount_TLV_Value_Type));

                receivedData = receivedData
                               + sizeof(LDP_HopCount_TLV_Value_Type);

                receivedDataSize = receivedDataSize
                               - sizeof(LDP_HopCount_TLV_Value_Type);

                if (MPLS_LDP_DEBUG)
                {
                    printf("received packet has hop count optional"
                           " and hop count is %d size = %" 
                           TYPES_SIZEOFMFT "u \n",
                           hopCountValue->hop_count_value,
                           sizeof(LDP_HopCount_TLV_Value_Type));
                }

                break;
            }
            case LDP_PATH_VECTOR_TLV :
            {
                hopCountInPathVector = tlvHeader.Length
                                       / sizeof(NodeAddress);

                pathVectorTLV = (NodeAddress*) MEM_malloc(tlvHeader.Length);

                memcpy(pathVectorTLV, receivedData, tlvHeader.Length);

                receivedData = receivedData + tlvHeader.Length;
                receivedDataSize = receivedDataSize - tlvHeader.Length;

                if (MPLS_LDP_DEBUG)
                {
                    int i = 0;

                    printf("received packet has hop pathVector optional\n"
                           "total size is %d "
                           "number of hop is %d they are \n:",
                           tlvHeader.Length,
                           hopCountInPathVector);

                    for (i = 0 ; i < hopCountInPathVector; i++)
                    {
                        printf("%4d,",pathVectorTLV[i]);
                    }
                    printf("\n");
                }
                break;
            }
            default :
            {
                ERROR_Assert(FALSE, "Error in"
                    " MplsLdpProcessLabelRequestMessage()"
                    " TLV of unknown type has been received !!!");
            }
        }
    } // end while (receivedDataSize > 0)

    // building RAttrribute type
    if (hopCountValue)
    {
        RAttribute.hopCount = TRUE;
        RAttribute.numHop = hopCountValue->hop_count_value;
    }
    else
    {
        RAttribute.hopCount = FALSE;
        RAttribute.numHop = MPLS_UNKNOWN_NUM_HOPS;
    }

    if (pathVectorTLV)
    {
        RAttribute.pathVector = TRUE;
        RAttribute.pathVectorValue.length = hopCountInPathVector;

        RAttribute.pathVectorValue.pathVector = (NodeAddress*)
            MEM_malloc(hopCountInPathVector * sizeof(NodeAddress));

        memcpy(RAttribute.pathVectorValue.pathVector,
            pathVectorTLV,
            hopCountInPathVector * sizeof(NodeAddress));
    }
    else
    {
        RAttribute.pathVector = FALSE;
        RAttribute.pathVectorValue.length = 0;
        RAttribute.pathVectorValue.pathVector = NULL;
    }

    // Buildind Fec data
    fec.numSignificantBits = fecTLV.Prefix.PreLen;
    fec.ipAddress = nodeAddressList[0];
    // Changed for adding priority support for FEC classification.
    // By default the priority value is zero.
    fec.priority = 0;

    MplsLdpProcessLabelRequestAndGenerateResponse(
        node,
        ldp,
        sourceLSR_Id.LSR_Id,
        fec,
        &RAttribute,
        msgHeader->Message_Id);

    // Record label request got in the incoming label request cache.
    helloAdjacency = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                         ldp,
                         sourceLSR_Id.LSR_Id);

    MplsLdpAddRowToIncomingRequestCache(
        node,
        helloAdjacency,
        fec,
        sourceLSR_Id.LSR_Id);


    MplsLdpFreeAttributeType(&RAttribute);

    MEM_free(nodeAddressList);
    MEM_free(pathVectorTLV);
    MEM_free(hopCountValue);

    // increment number of label request message received.
    INCREMENT_STAT_VAR(numLabelRequestMsgReceived);
}


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpDeleteOutstandingLabelWithdrawRecord()
//
//  PURPOSE      : to delete a record from LabelWithdrawRec cache
//
//  PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                 fec - fec to be matched for
//                 label - label to be matched for
//
//  RETURN VALUE : return TRUE if label is found in LabelWithdrawRec
//                 and deleted successfully, or FALSE otherwise.
//-------------------------------------------------------------------------
static
BOOL MplsLdpDeleteOutstandingLabelWithdrawRecord(
    MplsLdpApp* ldp,
    unsigned int label,
    Mpls_FEC fec)
{
    unsigned int i = 0;
    BOOL found = FALSE;
    unsigned int index = 0;

    for (i = 0; i < ldp->withdrawnLabelRecNumEntries; i++)
    {
       if ((MatchFec(&(ldp->withdrawnLabelRec[i].fec), &fec)) &&
          (ldp->withdrawnLabelRec[i].labelWithdrawn == label))
       {
           found = TRUE;
           break;
       }
    }

    if (found)
    {
        if (index == (ldp->withdrawnLabelRecNumEntries - 1))
        {
            memset(&(ldp->withdrawnLabelRec[index]), 0,
                   sizeof(LabelWithdrawRec));
        }
        else
        {
            for (i = index + 1; i < ldp->withdrawnLabelRecNumEntries; i++)
            {
                memcpy((&ldp->withdrawnLabelRec[i - i]),
                       &ldp->withdrawnLabelRec[i],
                       sizeof(LabelWithdrawRec));
            }
            memset(&(ldp->withdrawnLabelRec[index]), 0,
                   sizeof(LabelWithdrawRec));
        }
        (ldp->withdrawnLabelRecNumEntries)--;
    }
    return found;
}


//-------------------------------------------------------------------------
// FUNCTION     : IsConfiguredToPropagateLabelRelease()
//
// PURPOSE      : determining if LSR is configured to propagate
//                label release message or not
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//
// ASSUMPTION   : none.
//-------------------------------------------------------------------------
static
BOOL IsConfiguredToPropagateLabelRelease(MplsLdpApp* ldp)
{
    return ldp->propagateLabelRelease;
}


//-------------------------------------------------------------------------
// FUNCTION     : IsLsrMergingLabels()
//
// PURPOSE      : determining if LSR is configured to support
//                label Merging or not
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//
// ASSUMPTION   : none.
//----------------------------------------------------------------------
static
BOOL IsLsrMergingLabels(MplsLdpApp* ldp)
{
   return ldp->supportLabelMerging;
}


//-------------------------------------------------------------------------
// FUNCTION     : IsLabelMappedToWithdrawnLabel()
//
// PURPOSE      : to check whether a label is mapped to withdrawn
//                label or not.(For this search LabelWithdrawRec
//                cache).
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - fec to be matched for
//                label - label to be matched for
//
// RETURN VALUE : return TRUE if label is found in LabelWithdrawRec
//                or FALSE otherwise.
//-------------------------------------------------------------------------
static
BOOL IsLabelMappedToWithdrawnLabel(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    unsigned int label)
{
    unsigned int i = 0;
    BOOL found = FALSE;

    for (i = 0; i < ldp->withdrawnLabelRecNumEntries; i++)
    {
        if (!memcpy(&(ldp->withdrawnLabelRec[i].fec), &fec,
               sizeof(Mpls_FEC)) &&
                   (ldp->withdrawnLabelRec[i].labelWithdrawn == label))
        {
            found = TRUE;
            break;
        }
    }
    return found;
}


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpMatchAnOutstandingLabelWithdraw()
//
//  PURPOSE      : to check whether a label is in withdrawn label record
//                 cache or not.(For this search LabelWithdrawRec cache).
//
//  PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                 fec - fec to be matched for
//                 label - label to be matched for
//
//  RETURN VALUE : return TRUE if label is found in LabelWithdrawRec
//                 or FALSE otherwise.
//-------------------------------------------------------------------------
static
BOOL MplsLdpMatchAnOutstandingLabelWithdraw(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    unsigned int label)
{
    return IsLabelMappedToWithdrawnLabel(ldp, fec, label);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpDeleteOutgoingEntriesForPeer()
//
// PURPOSE      : to if any upstream peer hold a label mapping
//                with a given label
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                label - label to be matched
//
// RETURN VALUE : return TRUE if inLabel is found in ILM entries
//                or FALSE otherwise.
//-------------------------------------------------------------------------
static
BOOL MplsLdpDeleteOutgoingEntriesForPeer(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    MplsLdpHelloAdjacency* peerToMatch)
{
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int k = 0;

    for (j = 0; j < ldp->numHelloAdjacency; j++)
    {
        MplsLdpHelloAdjacency* peer = &(ldp->helloAdjacency[j]);

        for (i = 0; i < peer->outboundLabelMappingNumEntries;  i++)
        {
            if ((MatchFec(&(peer->outboundLabelMapping[i].fec), &fec)) &&
                (peer->LSR2.LSR_Id == peerToMatch->LSR2.LSR_Id))
            {
                if (i == (peer->outboundLabelMappingNumEntries - 1))
                {
                    memset(&(peer->outboundLabelMapping[i].fec), 0,
                           sizeof(Mpls_FEC));
                }
                else
                {
                    for (k = i + 1; k < peer->outboundLabelMappingNumEntries;
                         k++)
                    {
                        memcpy((&peer->outboundLabelMapping[k - i].fec),
                               &peer->outboundLabelMapping[k].fec,
                                sizeof(Mpls_FEC));
                    }
                    memset(&(peer->outboundLabelMapping[i].fec), 0,
                           sizeof(Mpls_FEC));
                }
                (peer->outboundLabelMappingNumEntries)--;
                return TRUE ;
            } // end of if MatchFec
        }  //end of for loop  i
    }  // end of for loop j
    return FALSE;
}   // end of function MplsLdpDeleteOutgoingEntriesForPeer



//-------------------------------------------------------------------------
// FUNCTION     : AnyOtherUpstreamHoldsMappingWithThisLabel()
//
// PURPOSE      : to if any upstream peer hold a label mapping
//                with a given label
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                label - label to be matched
//
// RETURN VALUE : return TRUE if inLabel is found in ILM entries
//                or FALSE otherwise.
//-------------------------------------------------------------------------
static
BOOL IsAnyOtherPeerHoldsMappingWithThisFEC(
    MplsLdpApp* ldp,
    Mpls_FEC fec)
{
    unsigned int i = 0;
    unsigned int j = 0;

    for (j = 0; j < ldp->numHelloAdjacency; j++)
    {
        MplsLdpHelloAdjacency* peer = &(ldp->helloAdjacency[j]);

        for (i = 0; i < peer->outboundLabelMappingNumEntries;  i++)
        {
            if (MatchFec(&(peer->outboundLabelMapping[i].fec), &fec))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpReceiveLabelReleaseMessage()
//
// PURPOSE      : processing a label release message.
//
// PARAMETERS   : node - node which is processing a label release message
//                ldp - pointer to the MplsLdpApp structure.
//                label - The label specified in the message.
//                fec - the FEC specified in the message
//                msgSource - The LDP peer that sent the message.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpReceiveLabelReleaseMessage(
    Node* node,
    MplsLdpApp* ldp,
    unsigned int label,
    Mpls_FEC fec,
    NodeAddress msgSource)
{
    NodeAddress nextHopAddress = MPLS_LDP_INVALID_PEER_ADDRESS;
    unsigned int labelReceived = 0;
    BOOL nextHopExists = FALSE;
    FecToLabelMappingRec* receivedLabelMapping = NULL;
    Attribute_Type* RAttribute = NULL;
    MplsLdpHelloAdjacency* peerSource = NULL;
    MplsLdpHelloAdjacency* peerNext = NULL;
    LDP_Identifier_Type LSR_Identifier;

    memset(&LSR_Identifier, 0, sizeof(LSR_Identifier));

    peerSource = MplsLdpReturnHelloAdjacencyForThisLSR_Id(ldp, msgSource);

    //  Summary :
    //  When an LSR receives a label release message for a FEC from a
    //  peer, it checks whether other peers hold the released label.  If
    //  none do, the LSR removes the label from forwarding/switching use,
    //  if it has not already done so, and if the LSR holds a label
    //  mapping from the FEC next hop, it releases the label mapping.
    //
    //  Context:
    //
    //  -  LSR.  The LSR handling the event.
    //
    //  -  MsgSource.  The LDP peer that sent the message.
    //
    //  -  Label.  The label specified in the message.
    //
    //  -  FEC.  The FEC specified in the message.

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d in MplsLdpReceiveLabelReleaseMessage()"
               "label = %d / fec = %d"
               "source lsr id = %u\n",
               node->nodeId,
               label,
               fec.ipAddress,
               LSR_Identifier.LSR_Id);
    }

    // LRl.1   Remove MsgSource from record of peers that hold Label for
    //         FEC.  (See Note 1.)
    MplsLdpDeleteEntriesFromIncomingFecToLabelMappingRec(
            ldp,
            fec,
            msgSource);

    // LRl.2   Does message match an outstanding label withdraw for FEC
    //         previously sent to MsgSource?
    //         If not, goto LRl.4
    if (MplsLdpMatchAnOutstandingLabelWithdraw(ldp, fec, label))
    {
        // LRl.3   Delete record of outstanding label withdraw for FEC
        //         previously sent to MsgSource.
        MplsLdpDeleteOutstandingLabelWithdrawRecord(ldp, label, fec);
    }

    // LRl.4   Is LSR merging labels for this FEC?
    //         If not, goto LRl.6.  (See Note 2.)
    if (IsLsrMergingLabels(ldp))
    {
        // LRl.5   Has LSR previously advertised a label for this FEC to
        //         other peers?
        //         If so, goto LRl.10.
        ERROR_Assert(FALSE, "label merging is not coded yet :(\n");
    }

    // LRl.6   Is LSR egress for the FEC?
    //         If so, goto LRl.10
    if (!IamEgressForThisFec(node, fec, ldp))
    {
        nextHopExists = IsNextHopExistForThisDest(
                            node,
                            fec.ipAddress,
                            &nextHopAddress);

        peerNext = MplsLdpReturnHelloAdjacency(
                       ldp,
                       nextHopAddress,
                       LSR_Identifier,
                       TRUE,
                       FALSE);

        if (peerNext)
        {
            receivedLabelMapping =
            MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
                ldp,
                fec,
                &labelReceived,
                peerNext->LSR2.LSR_Id,
                &RAttribute);
        }

        // LRl.7 Is there a Next Hop for FEC? AND
        //       Does LSR have a previously received label mapping for FEC
        //       from Next Hop?
        //       If not, goto LRl.10.
        if ((nextHopExists) && (receivedLabelMapping))
        {
            // LRl.8   Is LSR configured to propagate releases?
            //         If not, goto LRl.10.  (See Note 3.)
            if (IsConfiguredToPropagateLabelRelease(ldp))
            {
                if (MPLS_LDP_DEBUG)
                {
                    printf("#%d removing label %d for forwarding/switching"
                           " use\nThis is configured to propagate"
                           " label Release  message\nHence sending"
                           " label release to %d"
                           " for labelReceived  = %d\n",
                           node->nodeId,
                           label,
                           nextHopAddress,
                           labelReceived);
                }

                //  LRl.9   Execute procedure Send_Message (Next Hop, Label
                //          Release, FEC, Label from Next Hop).
                MplsLdpSendLabelReleaseOrWithdrawMessage(
                    node,
                    ldp,
                    peerNext,
                    labelReceived,
                    fec,
                    NO_LOOP_DETECTED,
                    TRUE,
                    ldp->currentMessageId,
                    LDP_LABEL_RELEASE_MESSAGE_TYPE);

                // send the pdu

                MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                    node,
                    ldp,
                    peerNext);

                (ldp->currentMessageId)++;
            }
        }
    }

    // LRl.10  Remove Label from forwarding/switching use for traffic
    //         from MsgSource.
    if (MPLS_LDP_DEBUG)
    {
        printf("in function MplsLdpReceiveLabelReleaseMessage()\n"
               "node = %d Label to be deleted %d \n source = %u\n",
               node->nodeId,
               label,
               peerSource->LinkAddress);

        Print_FTN_And_ILM(node);
    }

    MplsLdpRemoveLabelForForwardingSwitchingUse(
         node,
         peerSource->LinkAddress,
         label,
         INCOMING,
         fec);

    // Remove the label from the peer.
    // Since we have received the label release message so,
    // we should delete from the outgoing list of the peers.

    MplsLdpDeleteOutgoingEntriesForPeer(
        ldp,
        fec,
        peerSource) ;

    // LRl.11  Do any peers still hold Label for FEC?
    //         If so, goto LRl.13.
    if (!IsAnyOtherPeerHoldsMappingWithThisFEC(ldp, fec))
    {
        // LRl.12  Free the Label.

        unsigned int label = 0;
        unsigned int hopCount = 0;

        // Delete the corresponding entry from the table.
        MplsLdpRetrivePreviouslySentLabelMapping(
            ldp,
            fec,peerSource->LSR2.LSR_Id,
            &label,
            &hopCount,
            NULL,
            TRUE);
    }

    // Increment number of label release message received
    INCREMENT_STAT_VAR(numLabelReleaseMsgReceived);

    // LRl.13  DONE.
}


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpReceiveLabelWithdrawMessage()
//
//  PURPOSE      : Receiving a label withdraw message.
//
//  PARAMETERS   : node - node which is receiving the label withdraw message
//                 ldp - pointer to the MplsLdpApp structure.
//                 label - The label specified in the message.
//                 fec - the FEC to be specified in the message
//                 msgSource - the LDP peer that sent the message
//
//  RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpReceiveLabelWithdrawMessage(
    Node* node,
    MplsLdpApp* ldp,
    unsigned int label,
    Mpls_FEC fec,
    NodeAddress msgSource)
{
    //  A.1.5. Receive Label Withdraw
    //
    //  Summary:
    //
    //  When an LSR receives a label withdraw message for a FEC from an
    //  LDP peer, it responds with a label release message and it removes
    //  the label from any forwarding/switching use.  If ordered control
    //  is in use, the LSR sends a label withdraw message to each LDP peer
    //  to which it had previously sent a label mapping for the FEC.  If
    //  the LSR is using Downstream on Demand label advertisement with
    //  independent control, it then acts as if it had just recognized the
    //  FEC.
    //
    //  Context:
    //
    //  -  LSR.  The LSR handling the event.
    //
    //  -  MsgSource.  The LDP peer that sent the message.
    //
    //  -  Label.  The label specified in the message.
    //
    //  -  FEC.  The FEC specified in the message.
    //
    MplsLdpHelloAdjacency* peerSource = NULL;
    unsigned int labelRetained = 0;
    Attribute_Type* RAttribute;

    peerSource = MplsLdpReturnHelloAdjacencyForThisLSR_Id(ldp, msgSource);

    ERROR_Assert(peerSource, "Error in MplsLdpReceiveLabelReleaseMessage()"
        " peerSource is NULL !!!");

    if (MPLS_LDP_DEBUG)
    {
        printf("in function MplsLdpReceiveLabelWithdrawMessage()\n"
               "node = %d Label to be deleted %d \n",
               node->nodeId,
               label);

        Print_FTN_And_ILM(node);
    }

    //LWd.1   Remove Label from forwarding/switching use.
    MplsLdpRemoveLabelForForwardingSwitchingUse(
        node,
        peerSource->LinkAddress,
        label,
        OUTGOING,
        fec);

    // LWd.2   Execute procedure Send_Message (MsgSource, Label Release,
    //         FEC, Label)
    MplsLdpSendLabelReleaseOrWithdrawMessage(
        node,
        ldp,
        peerSource,
        label,
        fec,
        NO_LOOP_DETECTED,
        TRUE,
        ldp->currentMessageId++,
        LDP_LABEL_RELEASE_MESSAGE_TYPE);

    // send the pdu

    MplsLdpTransmitPduFromHelloAdjacencyBuffer(
        node,
        ldp,
        peerSource);

    // LWd.3   Has LSR previously received and retained a matching label
    //         mapping for FEC from MsgSource?
    //         If not, goto LWd.13.
    if (MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
            ldp,
            fec,
            &labelRetained,
            peerSource->LSR2.LSR_Id,
            &RAttribute))
    {
        // LWd.4   Delete matching label mapping for FEC previously received
        //         from MsgSource.
        MplsLdpDeleteEntriesFromIncomingFecToLabelMappingRec(
            ldp,
            fec,
            msgSource);

        // LWd.5   Is LSR using ordered control?
        //         If so, goto LWd.8.
        if (ldp->labelDistributionControlMode == MPLS_CONTROL_MODE_ORDERED)
        {
            unsigned int i = 0;

            // LWd.8   Iterate through LWd.12 for each Peer, other than
            //         MsgSource.
            for (i = 0; i < ldp->numHelloAdjacency; i++)
            {
                if (ldp->helloAdjacency[i].LSR2.LSR_Id != msgSource)
                {
                    unsigned int labelSent = 0;
                    unsigned int hopCount = 0;

                    //  LWd.9   Has LSR previously sent a label mapping for
                    //          FEC If not, continue iteration for next
                    //          Peer at LWd.8.
                    if (!MplsLdpRetrivePreviouslySentLabelMapping(
                             ldp,
                             fec,
                             ldp->helloAdjacency[i].LSR2.LSR_Id,
                             &labelSent,
                             &hopCount,
                             NULL,
                             FALSE))
                    {
                        continue;
                    }

                    //  LWd.10  Does the label previously sent to Peer "map"
                    //          to the   withdrawn Label? If not, continue
                    //          iteration for next Peer at LWd.8.
                    //          (See Note 3.)
                    if (IsLabelMappedToWithdrawnLabel(ldp, fec, label))
                    {
                        continue;
                    }

                    //  LWd.11  Execute procedure MplsLdpSendLabelWithdraw
                    //          (Peer,FEC, Label ,previously sent to Peer).
                    MplsLdpSendLabelWithdraw(
                        node,
                        ldp,
                        &(ldp->helloAdjacency[i]),
                        fec,
                        labelSent);
                }
                // LWd.12  End iteration from LWd.8.
            }
        }
        else if (ldp->labelDistributionControlMode ==
                 MPLS_CONTROL_MODE_INDEPENDENT)
        {
            // LWd.6   Is MsgSource using Downstream On Demand label
            //         advertisement?
            //         If not, goto LWd.13.
            if (ldp->labelAdvertisementMode == ON_DEMAND)
            {
                // This is the actually label advertisement mode
                // of current LSR, But we need to know Label
                // distribution mode of peer LSR. We assume all LSR
                // in a group are in same label distribution mode.
                // Hence Above check will suffice

                NodeAddress nextHopAddress = MPLS_LDP_INVALID_PEER_ADDRESS;
                Attribute_Type notUsed;
                int interfaceIndex = INVALID_INTERFACE_INDEX;

                memset(&notUsed, 0, sizeof(Attribute_Type));

                NetworkGetInterfaceAndNextHopFromForwardingTable(
                    node,
                    fec.ipAddress,
                    &interfaceIndex,
                    &nextHopAddress);

                if (nextHopAddress == 0)
                {
                    // next hop address "0" means next hop
                    // itself is the destination address
                    nextHopAddress = fec.ipAddress;
                }

                //  LWd.7   Generate Event: Recognize New FEC for FEC.
                //          Goto LWd.13.  (See Note 2.)
                MplsLdpRecognizeNewFEC(
                    node,
                    ldp,
                    fec,
                    nextHopAddress,
                    &notUsed,
                    (Attribute_Type*) &notUsed,
                    ((RAttribute->hopCount) ? RAttribute->numHop : 0));
            }
        }
        else
        {
            ERROR_Assert(FALSE, "Error in"
                " MplsLdpReceiveLabelWithdrawMessage()"
                " unknown label Advertisement Mode !!!");
        }
    }

    // Increment number of label label withdraw message
    INCREMENT_STAT_VAR(numLabelWithdrawMsgReceived);
    // LWd.13  DONE
}


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpProcessNotificationMessage()
//
//  PURPOSE      : processing notification message.
//
//  PARAMETERS   : node - node which is processing the notification message
//                 msg - the notification message
//                 msgHeader - the message header
//                 LSR_Id - LSR_Id of the LSR from which notification
//                          message is received
//                 tcpConnId - TCP connection Id.
//
//  RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpProcessNotificationMessage(
    Node* node,
    Message* msg,
    LDP_Message_Header* msgHeader,
    LDP_Identifier_Type LSR_Id,
    int tcpConnId)
{
    NotificationErrorCode errorCode = UNIDENTIFIED_ERROR;
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);
    int packetSize = MESSAGE_ReturnPacketSize(msg);
    char* packet = MESSAGE_ReturnPacket(msg);

    // extract error code
    errorCode = MplsLdpGetNotificationErrorCode(msg);

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d has recevied an error message through tcp conn %d\n"
               "the source LSR_Id is = %d"
               "the error code is = %d\n",
               node->nodeId,
               tcpConnId,
               LSR_Id.LSR_Id,
               errorCode);
    }

    // remove ldp tlv header header
    packet = packet + sizeof(LDP_TLV_Header);
    packetSize = packetSize - sizeof(LDP_TLV_Header);

    // Skip status TLV portion of notification message.
    packet = packet + sizeof(LDP_Status_TLV_Value_Type);
    packetSize = packetSize - sizeof(LDP_Status_TLV_Value_Type);

    switch (errorCode)
    {
        case UNIDENTIFIED_ERROR :
        {
             break;
        }
       // The following code has been added for integrating IP+MPLS
        // code as per RFC 3036 section A.1.11
        case LOOP_DETECTED_ERROR :
        case NO_ROUTE_ERROR :
        {
             LDP_Prefix_FEC_Element fecTlv;
             Mpls_FEC fec;

             // remove Ldp Tlv header
             packet = packet + sizeof(LDP_TLV_Header);
             packetSize = packetSize - sizeof(LDP_TLV_Header);

             ERROR_Assert((packetSize == (sizeof(LDP_Prefix_FEC_Element)
                 + sizeof(NodeAddress))),
                 " Error in MplsLdpProcessNotificationMessage()"
                 " NO_ROUTE_ERROR notification received a tlv which"
                 " is not FEC TLV");

             // copy Fec tlv,
             memcpy(&fecTlv, packet, sizeof(LDP_Prefix_FEC_Element));
             packet = packet + sizeof(LDP_Prefix_FEC_Element);
             packetSize = packetSize - sizeof(LDP_Prefix_FEC_Element);

             ERROR_Assert((packetSize == sizeof(NodeAddress)),
                 " Error in MplsLdpProcessNotificationMessage()"
                 " NO_ROUTE_ERROR notification received a tlv which"
                 " cotains something othe than node address in fecTlv");

             // copy ip address of fec
             memcpy(&(fec.ipAddress), packet, sizeof(NodeAddress));
             packet = packet + sizeof(NodeAddress);
             packetSize = packetSize - sizeof(NodeAddress);

             //get num significant bits of fec
             fec.numSignificantBits = fecTlv.Prefix.PreLen;

             // Changed for adding priority support for FEC classification.
             // By default the priority value is zero.
             fec.priority = 0;

             MplsLdpProcessNoRouteError(
                 node,
                 ldp,
                 msg,
                 msgHeader,
                 fec,
                 LSR_Id);

             break;
        }
        case NO_LABEL_RESOURCE_ERROR :
        {
             MplsLdpProcessNoLabelResourceError(
                 node,
                 ldp,
                 msg,
                 msgHeader,
                 LSR_Id);

             break;
        }
        default :
        {
             printf("#%dNotification Message Received with unknown\n",
                 node->nodeId);

             printf("error code %d ", errorCode);
             ERROR_Assert(FALSE, " undesirable situation!!!");
        }
    }
    // increment number of notification messages received
    INCREMENT_STAT_VAR(numNotificationMsgReceived);
}


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpProcessLabelReleaseOrWithdrawMessage()
//
//  PURPOSE      : processing label release or withdraw message
//
//  PARAMETERS   : node - node which is processing label
//                        release or withdraw message
//                 msg - the release or withdraw message message.
//                 msgHeader - message header.
//                 LSR_Id - LSR_Id of peer LSR from which the message is
//                          received.
//                 messageType - label-release or label-withdraw
//                 tcpConnId - TCP connection through which message is
//                             received.
//-------------------------------------------------------------------------
static
void MplsLdpProcessLabelReleaseOrWithdrawMessage(
    Node* node,
    Message* msg,
    LDP_Message_Header* msgHeader,
    LDP_Identifier_Type LSR_Id,
    int messageType,
    int tcpConnId)
{
    char* packetPtr;
    int packetSize = 0;
    MplsLdpApp* ldp;

    LDP_TLV_Header tlvHeader;
    LDP_TLV_Header tlvHeaderLabel;

    LDP_Prefix_FEC_Element fecTlv;
    Mpls_FEC fec;
    LDP_Generic_Label_TLV_Value_Type label;
    label.Label = 0;

    packetPtr = MESSAGE_ReturnPacket(msg);
    ldp = MplsLdpAppGetStateSpace(node);
    packetSize = (msgHeader->Message_Length - LDP_MESSAGE_ID_SIZE);

    if (MPLS_LDP_DEBUG)
    {
        if (messageType == LDP_LABEL_RELEASE_MESSAGE_TYPE)
        {
            printf("#%d received a label Release Message/n",
                 node->nodeId);
        }
        else if (messageType == LDP_LABEL_WITHDRAW_MESSAGE_TYPE)
        {
            printf("#%d received a label Withdraw Message/n",
                node->nodeId);
        }
        else
        {
            printf("not a label release or withdraw message\n");

            ERROR_Assert(FALSE, " Error in"
                " MplsLdpProcessLabelReleaseOrWithdrawMessage()"
                " not a label withdraw or release message !!!");
        }

        printf("Message Type = %x / essage Id  == %d\n"
               "Total size of the packet : = %d\n"
               "The source LsrId is = %d\n"
               "Through Tcp ConnId %d\n",
               LDP_Message_HeaderGetMsgType(msgHeader->ldpMsg),
               msgHeader->Message_Id,
               packetSize,
               LSR_Id.LSR_Id,
               tcpConnId);
    }

    memcpy(&tlvHeader, packetPtr, sizeof(LDP_TLV_Header));
    packetPtr = packetPtr + sizeof(LDP_TLV_Header);
    packetSize = packetSize - sizeof(LDP_TLV_Header);

    if (MPLS_LDP_DEBUG)
    {
        printf("Removed TLV header Of FEC TLV\n"
               "U = %d , F = %dherder.Type  = %d"
               "header.length = %d sizeof Fec Tlv = %" TYPES_SIZEOFMFT
               "u sizeof Fec = %" TYPES_SIZEOFMFT "u" ,
               LDP_TLV_HeaderGetU(tlvHeaderLabel.tlvHdr),
               LDP_TLV_HeaderGetF(tlvHeaderLabel.tlvHdr),
               LDP_TLV_HeaderGetType(tlvHeaderLabel.tlvHdr),
               tlvHeader.Length,
               sizeof(LDP_Prefix_FEC_Element),
               sizeof(Mpls_FEC));
    }

    ERROR_Assert(tlvHeader.Length == (sizeof(LDP_Prefix_FEC_Element)
           + sizeof(NodeAddress)),
           "Error in MplsLdpProcessLabelReleaseOrWithdrawMessage()"
           " header length mismatch !!!");

    // Extract Fec TLV......
    memcpy(&fecTlv, packetPtr, sizeof(LDP_Prefix_FEC_Element));
    packetPtr = packetPtr + sizeof(LDP_Prefix_FEC_Element);
    packetSize = packetSize - sizeof(LDP_Prefix_FEC_Element);

    if (MPLS_LDP_DEBUG)
    {
        printf("fecTlv.Element_Type = %d"
               "fecTlv.Prefix.Address_Family = %d"
               "fecTlv.Prefix.PreLen = %d\n",
               fecTlv.Element_Type,
               fecTlv.Prefix.Address_Family,
               fecTlv.Prefix.PreLen);
    }

    // Fec TLV should contain exactly one fec in the context
    // of label to be released or label to be withdrawn.
    memcpy(&fec.ipAddress, packetPtr, sizeof(NodeAddress));
    fec.numSignificantBits = fecTlv.Prefix.PreLen;
    // Changed for adding priority support for FEC classification.
    // By default the priority value is zero.
    fec.priority = 0 ;

    packetPtr = packetPtr + sizeof(NodeAddress);
    packetSize = packetSize - sizeof(NodeAddress);

    if (MPLS_LDP_DEBUG)
    {
        printf("FEC extracted from FEC TLV\n"
               "the Fec.ipAdress = %d / Fec.prelen = %d\n",
               fec.ipAddress,
               fec.numSignificantBits);
    }

    // if packet size still not zero then there is some optional
    // label field in the message (which is Label TLV and it is
    // equally possiable that label TLV may be present both in
    // label release or withdraw message...)
    if (packetSize)
    {
        // Extract Label from label tlv
        memcpy(&tlvHeaderLabel, packetPtr, sizeof(LDP_TLV_Header));
        packetPtr = packetPtr + sizeof(LDP_TLV_Header);
        packetSize = packetSize - sizeof(LDP_TLV_Header);

        if (MPLS_LDP_DEBUG)
        {
            printf("Removed TLV header Of Label TLV\n"
                   "U = %d,F = %d herder.Type = %d header.length = %d\n",
                   LDP_TLV_HeaderGetU(tlvHeaderLabel.tlvHdr),
                   LDP_TLV_HeaderGetF(tlvHeaderLabel.tlvHdr),
                   LDP_TLV_HeaderGetType(tlvHeaderLabel.tlvHdr),
                   tlvHeaderLabel.Length);
        }

        ERROR_Assert(packetSize >= (signed) sizeof(LDP_Generic_Label_TLV_Value_Type),
            "Error in MplsLdpProcessLabelReleaseOrWithdrawMessage()"
            " packet size mismatch !!!");

        memcpy(&label, packetPtr, sizeof(LDP_Generic_Label_TLV_Value_Type));
        packetPtr = packetPtr + sizeof(LDP_Generic_Label_TLV_Value_Type);
        packetSize = packetSize - sizeof(LDP_Generic_Label_TLV_Value_Type);

        if (MPLS_LDP_DEBUG)
        {
            printf("Label extracted from label TLV"
                   "the label is = %d\n",
                   label.Label);
        }
    }

    // since for the moment we do not allow more optional fields
    // than label Tlv at this point after extraction of all the
    // Tlv's packet size should be zero;

    ERROR_Assert(packetSize == 0, "Error in"
        " MplsLdpProcessLabelReleaseOrWithdrawMessage()"
        " packet size is zero !!!");

    if (messageType == LDP_LABEL_RELEASE_MESSAGE_TYPE)
    {
        MplsLdpReceiveLabelReleaseMessage(
            node,
            ldp,
            label.Label,
            fec,
            LSR_Id.LSR_Id);
    }
    else if (messageType == LDP_LABEL_WITHDRAW_MESSAGE_TYPE)
    {
        MplsLdpReceiveLabelWithdrawMessage(
            node,
            ldp,
            label.Label,
            fec,
            LSR_Id.LSR_Id);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpProcessPDU()
//
// PURPOSE      : processing Hello message
//
// PARAMETERS   : node - node which is receiving PDU.
//                msg - the PDU message
//                sourceAddr - source address of the hello adjacency
//                             form which PDU is received
//                sourcePort - source port
//                LSR - LSR_Id of of the hello adjacency form which
//                      PDU is received
//                msgHeader - pointer to ldp message header.
//                tcpConnId - TCP  connection id through which the message
//                            is received (if received through TCP)
//                packetType - indicates if the "msg" is a TCP packet or
//                             UDP packet
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpProcessPDU(
    Node* node,
    Message* msg,
    NodeAddress sourceAddr,
    int tcpConnId,
    Packet_Type packetType)
{
    LDP_Header actualHeader;
    LDP_Message_Header msgHeader;
    MplsLdpHelloAdjacency* helloAdjacency = NULL;

    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    // Reassembly Block :
    // From TCP data comes in chunks of size 512 bytes. That means,
    // if from senders end any data-chunk of size greater than 512 byte
    // is send it is broken into data-chunks of size at most 512 byte.
    // e.g If a data chunk of size 890 bytes is send from sender end,
    // recever will receive two data-chunks of size 512 and 378 bytes.
    // The whole purpose of the block below is reassembling the
    // fragmented packets.


    // New procedure for processing PDU and for reassmebly
    // Modified as part of IP-MPLS integration

    // get the hello adjacency or the peer that has sent the
    // PDU using tcpConnection Id. And update the last message heard.
    helloAdjacency = MplsLdpReturnHelloAdjacencyForThisConnectionId(
                         ldp,
                         tcpConnId);

    if (helloAdjacency != NULL
        && BUFFER_GetAnticipatedSize(&(helloAdjacency->reassemblyBuffer))!= 0
        && packetType == TCP_PACKET)
    {
        // This means that some remaining data was anticipated.
        // Add data to buffer
        BUFFER_AddDataToAssemblyBuffer(
            &(helloAdjacency->reassemblyBuffer),
            MESSAGE_ReturnPacket(msg),
            MESSAGE_ReturnPacketSize(msg),
            TRUE);

        if ((BUFFER_GetAnticipatedSize(&(helloAdjacency->reassemblyBuffer)) +
                LDP_MESSAGE_ID_SIZE) > (unsigned)
                                   BUFFER_GetCurrentSize(
                                        &(helloAdjacency->reassemblyBuffer)))
        {
            // This means some data-chunks is yet to be received.
            // so go-back and wait
            UpdateLastHeard(helloAdjacency);
            return;
        }
        else
        {
            int notUsed = 0;
            int currentSize =
                 BUFFER_GetCurrentSize(&(helloAdjacency->reassemblyBuffer));

            // Rebuild the message;
            MESSAGE_Free(node, msg);

            msg = MESSAGE_Alloc(node, notUsed, notUsed, notUsed);

            MESSAGE_PacketAlloc(
                node,
                msg,
                currentSize,
                TRACE_MPLS_LDP);

            memcpy(MESSAGE_ReturnPacket(msg),
                BUFFER_GetData(&(helloAdjacency->reassemblyBuffer)),
                currentSize);

            BUFFER_ClearAssemblyBuffer(
                        &(helloAdjacency->reassemblyBuffer), notUsed, FALSE);
            BUFFER_SetAnticipatedSizeForAssemblyBuffer(
                        &(helloAdjacency->reassemblyBuffer), 0);
        }        
    }
    // End of reassembly block
    BOOL morePackets = TRUE;
    while (morePackets)
    {
        memcpy(&actualHeader, MESSAGE_ReturnPacket(msg), LDP_HEADER_SIZE);

        if ((signed) (actualHeader.PDU_Length + LDP_MESSAGE_ID_SIZE)
            <= (MESSAGE_ReturnPacketSize(msg)))
        {
            // End of reassembly block
            // The TCP data which does not get fragmented directly
            // comes here

            // The following code has been changed for IP-MPLS integration.
            // In case of wireless network, I can receive my own message.
            // So ignoring the same.
            if (actualHeader.LDP_Identifier.LSR_Id == ldp->LSR_Id)
            {
                 //MESSAGE_Free(node, msg);
                 return;
            }
            if (MPLS_LDP_DEBUG)
            {
                printf("node = %d PDU Header: LSR_Id = %d / "
                       "label_space_id = %d / length = %d"
                       "packet = %s packet\n",
                       node->nodeId,
                       actualHeader.LDP_Identifier.LSR_Id,
                       actualHeader.LDP_Identifier.label_space_id,
                       actualHeader.PDU_Length,
                       (packetType == TCP_PACKET) ? "TCP" : "UDP");
            }

            ERROR_Assert(MESSAGE_ReturnPacketSize(msg) >= (signed)
                         sizeof(LDP_Message_Header),
                "Error in MplsLdpProcessPDU()"
                " packet size is smaller than the LDP Message Header !!!");

           // Each LDP PDU can carry one or more LDP messages.
           // keep on extracting messages from PDU
           // untill all LDP messages are extracted

           // Since all LDP messages are transmitted from the buffer
           // immediately, a header of size LDP_HEADER_SIZE is added
           // to each message, which needs to be removed for each packet
           // and then further procesed.

            MplsLdpSkipHeader(node, msg, LDP_HEADER_SIZE);
            memcpy(&msgHeader,
                MESSAGE_ReturnPacket(msg),
                sizeof(LDP_Message_Header));

            if ((signed) (sizeof(LDP_Message_Header)) >
                MESSAGE_ReturnPacketSize(msg))
            {
                if (MPLS_LDP_DEBUG)
                {
                    printf("sizeof ldp header = %" TYPES_SIZEOFMFT
                           "u size of packet = %d ",
                           sizeof(LDP_Message_Header),
                           MESSAGE_ReturnPacketSize(msg));
                }
                break;
            }

            MplsLdpSkipHeader(node, msg, sizeof(LDP_Message_Header));

            if (MPLS_LDP_DEBUG)
            {
                printf("Message Header: U = %d / Message_Type = %X"
                       "/ Length = %d ID = %d\n",
                   LDP_Message_HeaderGetU(msgHeader.ldpMsg),
                   LDP_Message_HeaderGetMsgType(msgHeader.ldpMsg),
                       msgHeader.Message_Length,
                       msgHeader.Message_Id);
            }

            // Process the Message Here
            switch (LDP_Message_HeaderGetMsgType(msgHeader.ldpMsg))
            {
                case LDP_HELLO_MESSAGE_TYPE:
                {
                    // NOTE : The tcpConnid we are passing is actually
                    //        incoming interface thru which this UDP
                    //        hello message is received.The hello message
                    //        is the only UDP message processed in this
                    //        function. All other message are received via
                    //        the TCP connection.
                    int incomingInterfaceIndex = tcpConnId;

                    // Process UDP hello message
                    MplsLdpProcessHelloMessage(
                        node,
                        msg,
                        sourceAddr,
                        actualHeader.LDP_Identifier,
                        incomingInterfaceIndex);

                    break;
                }
                case LDP_INITIALIZATION_MESSAGE_TYPE :
                {
                    // process initialization message
                    MplsLdpProcessInitializationMessage(
                        node,
                        msg,
                        actualHeader.LDP_Identifier,
                        tcpConnId);

                    break;
                }
                case LDP_KEEP_ALIVE_MESSAGE_TYPE :
                {
                    // process Keep Alive message
                    MplsLdpProcessKeepAliveMessage(
                        node,
                        msg,
                        tcpConnId);

                    break;
                }
                case LDP_ADDRESS_LIST_MESSAGE_TYPE:
                {
                    // process Address message
                    MplsLdpProcessAddressMessage(
                        node,
                        msg,
                        actualHeader.LDP_Identifier,
                        tcpConnId);

                    break;
                }
                case LDP_LABEL_REQUEST_MESSAGE_TYPE:
                {
                    // process Label request message

                    MplsLdpProcessLabelRequestMessage(
                        node,
                        msg,
                        &msgHeader,
                        actualHeader.LDP_Identifier,
                        tcpConnId);

                    break;
                }
                case LDP_LABEL_MAPPING_MESSAGE_TYPE :
                {
                    // process label mapping message
                     MplsLdpProcessLabelMappingMessage(
                        node,
                        msg,
                        &msgHeader,
                        actualHeader.LDP_Identifier,
                        tcpConnId);

                    break;
                }
                case LDP_NOTIFICATION_MESSAGE_TYPE :
                {
                    // process notification message
                    MplsLdpProcessNotificationMessage(
                        node,
                        msg,
                        &msgHeader,
                        actualHeader.LDP_Identifier,
                        tcpConnId);

                    break;
                }
                case LDP_LABEL_RELEASE_MESSAGE_TYPE :
                {
                    // process label release message
                    MplsLdpProcessLabelReleaseOrWithdrawMessage(
                        node,
                        msg,
                        &msgHeader,
                        actualHeader.LDP_Identifier,
                        LDP_LABEL_RELEASE_MESSAGE_TYPE,
                        tcpConnId);

                    break;
                }
                case LDP_LABEL_WITHDRAW_MESSAGE_TYPE :
                {
                    // process label withdraw message
                    MplsLdpProcessLabelReleaseOrWithdrawMessage(
                        node,
                        msg,
                        &msgHeader,
                        actualHeader.LDP_Identifier,
                        LDP_LABEL_WITHDRAW_MESSAGE_TYPE,
                        tcpConnId);

                    break;
                }
                default:
                {
                    char errorStr[MAX_STRING_LENGTH];

                    sprintf(errorStr, "MPLS node %d received unknown LDP "
                            "Message Type %x\npacket size = %d",
                            node->nodeId,
                        LDP_Message_HeaderGetMsgType(msgHeader.ldpMsg),
                            MESSAGE_ReturnPacketSize(msg));

                    ERROR_ReportError(errorStr);

                }
            } // end switch(msgHeader.Message_Type)


            if (MESSAGE_ReturnPacketSize(msg) >
                ((signed)(msgHeader.Message_Length - LDP_MESSAGE_ID_SIZE)))
            {
                // remove LDP mesage ID
                MplsLdpSkipHeader(
                    node,
                    msg,
                    (msgHeader.Message_Length - LDP_MESSAGE_ID_SIZE));
            }
            else
            {
                morePackets = FALSE;

                if (MPLS_LDP_DEBUG)
                {
                    printf("\t more messages in this pdu\n");
                }
                break;
            }

        } // end of main if stmt
        else if (packetType == TCP_PACKET)
        {
            // get the hello adjacency or the peer that has sent the
            // PDU.
            if (!helloAdjacency)
        {
                helloAdjacency = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                                     ldp,
                                     actualHeader.LDP_Identifier.LSR_Id);

                ERROR_Assert(helloAdjacency, "Error in MplsLdpProcessPDU()"
                    " helloAdjacency not found !!!");
            }

            if (BUFFER_NumberOfBlocks(
                                &(helloAdjacency->reassemblyBuffer)) == 0)
            {
                // This means it is the first chunk and header
                // contains total data length. So prepare yourself to
                // receive that amount of data.
                BUFFER_SetAnticipatedSizeForAssemblyBuffer(
                    &(helloAdjacency->reassemblyBuffer),
                    actualHeader.PDU_Length);
            }

            if (MPLS_LDP_DEBUG)
            {
                printf("node %d Got non-contiguous packet of length = %d\n"
                       "This is Chunck no %d And of length = %d",
                       node->nodeId,
                       BUFFER_GetAnticipatedSize(
                                        &(helloAdjacency->reassemblyBuffer)),
                       BUFFER_NumberOfBlocks(
                                        &(helloAdjacency->reassemblyBuffer)),
                       MESSAGE_ReturnPacketSize(msg));
            }

            // Add data to buffer
            BUFFER_AddDataToAssemblyBuffer(
                &(helloAdjacency->reassemblyBuffer),
                MESSAGE_ReturnPacket(msg),
                MESSAGE_ReturnPacketSize(msg),
                FALSE);

            break;
        }
    } // end of while (morePackets)

    // get the hello adjacency or the peer that has sent the
    // PDU. And update the last message heard.
    if (!helloAdjacency)
    {
    helloAdjacency = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                         ldp,
                         actualHeader.LDP_Identifier.LSR_Id);

    ERROR_Assert(helloAdjacency, "Error in MplsLdpProcessPDU()"
        " helloAdjacency not found !!!");
    }

    UpdateLastHeard(helloAdjacency);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpIntervalReadTime()
//
// PURPOSE      : Reading Timer vales from configuration file
//
// PARAMETERS   : nodeId - node which is reading the timer values.
//                nodeInput - the input configuration file,
//                searchString - the index string
//                resultValue - pointer to the variable where the
//                              return value to be stored
//                defaultValue - the default value of the timer
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpIntervalReadTime(
    NodeAddress nodeId,
    const NodeInput* nodeInput,
    const char searchString[],
    clocktype* resultTime,
    clocktype defaultValue)
{
    BOOL retVal = FALSE;

    IO_ReadTime(
        nodeId,
        ANY_IP,
        nodeInput,
        searchString,
        &retVal,
        resultTime);

    if (!retVal)
    {
        *resultTime = defaultValue;

        if (MPLS_LDP_DEBUG)
        {
            printf("MPLS node %d did not find %s in"
                   " the configuration file.\n",
                   nodeId,
                   searchString);
        }
    }

    if (MPLS_LDP_DEBUG)
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(*resultTime, buf);

        printf("MPLS node %d sets %s to %s\n",
               nodeId,
               searchString,
               buf);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpUpdateStatusRecordForThisFec()
//
// PURPOSE      : updating the status record corresponding to label
//                request message send in the StatusRecord cache.
//
// PARAMETERS   : statusRecPtr - pointer to the status record in the
//                               StatusRecord cache whose staus is to
//                               be updated.
//                status - the status value.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpUpdateStatusRecordForThisFec(
    StatusRecord* statusRecPtr,
    BOOL status,
    Attribute_Type* SAttribute)
{
    statusRecPtr->statusOutstanding = status;
    MplsLdpFreeAttributeType(&(statusRecPtr->Attribute));
    MplsLdpAddAttributeType(&(statusRecPtr->Attribute), SAttribute);
}


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpSendLabelRequest()
//
//  PURPOSE      : preparing for sending a label request message.An LSR
//                 uses the MplsLdpSendLabelRequest procedure to send a
//                 request
//                 for a label for a FEC to an LDP peer if currently
//                 permitted to do so.
//
//  PARAMETERS   : node - which is preparing for sending a label request
//                        message
//                 ldp - pointer to the MplsLdpApp structure.
//                 peer - The LDP peer to which the label request
//                        is to be sent.
//                 fec - The FEC for which a label request is to be sent.
//                 attributes - attributes to be included in the label
//                   request. E.g., Hop Count, Path Vector.
//
//  RETURN VALUE : none
//-------------------------------------------------------------------------
static
BOOL MplsLdpSendLabelRequest(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* peer,
    NodeAddress upstreamLSR_Id,
    Mpls_FEC fec,
    Attribute_Type* attribute)
{
    StatusRecord* statusRecPtr = NULL;
    if (MPLS_LDP_DEBUG)
    {
        printf("#%d: MplsLdpSendLabelRequest(peer = %d/fec.ip = %d)"
               "the LSR_Id is : %d\n",
               node->nodeId,
               peer->LSR2.LSR_Id,
               fec.ipAddress,
               peer->LSR2.LSR_Id);
    }

    // Has a label request for FEC previously been sent to Peer
    // and is it marked as outstanding? If so, Return success.
    statusRecPtr = MplsLdpReturnStatusRecordForThisFEC(
                       ldp,
                       fec,
                       peer->LSR2.LSR_Id,
                       attribute);

    if (statusRecPtr->statusOutstanding)
    {
       return TRUE;
    }
    else if (peer->OkToSendLabelRequests)
    {
        // Is status record indicating it is OK to send label
        // requests to Peer set?
        // Execute procedure Send_Message (Peer, Label Request, FEC,
        // Attributes).
        MplsLdpSendLabelRequestMessage(
            node,
            ldp,
            peer,
            fec,
            attribute,
            ldp->currentMessageId);

        // send the pdu
        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            peer);

        (ldp->currentMessageId)++;

        // Record label request for FEC has been sent to Peer and
        // mark it as outstanding.
        MplsLdpUpdateStatusRecordForThisFec(statusRecPtr, TRUE, attribute);
        return TRUE;
    }
    else
    {
        // Postpone the label request by recording label mapping for
        // FEC and Attributes from Peer is needed but that no label
        // resources are available.
        //
        // Return failure.
        ERROR_Assert(upstreamLSR_Id != ldp->localAddr[0],
            "Error in MplsLdpSendLabelRequest()"
            " upstream LSR_Id equal to local LSR_Id");

        MplsLdpAddRowToThePendingRequestList(
            ldp,
            fec,
            peer->LSR2.LSR_Id,
            upstreamLSR_Id);

        return FALSE;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : SearchEntriesInTheLib()
//
// PURPOSE      : searching for a given FEC in the LIB (Label Information
//                Base) table and finding and finding an outgoing label
//                for that fec.
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - the fec to be searched for
//                outLabel - the value of the outgoing Label
//
// RETURN VALUE : pointer to the matching record in LIB or NULL otherwise
//                if given FEC in not found in the LIB.
//-------------------------------------------------------------------------
LIB* SearchEntriesInTheLib(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    unsigned int* inLabel)
{
    unsigned int i = 0;
    BOOL found = FALSE;

    for (i = 0; i < ldp->LIB_NumEntries; i++)
    {
        if (ldp->LIB_Entries[i].prefix == fec.ipAddress)
        {
            found = TRUE;
            *inLabel = ldp->LIB_Entries[i].label;
            break;
        }
    }

    if (found)
    {
        return &(ldp->LIB_Entries[i]);
    }
    else
    {
        return NULL;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpDeleteEntriesFromLIB()
//
// PURPOSE      : searching for a given FEC in the LIB (Label Information
//                Base) table and deleting the matching record form the LIB.
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                fec - the fec to be searched for
//                label - the label value to be matched for
//
// RETURN VALUE : none.
//-------------------------------------------------------------------------
static
void MplsLdpDeleteEntriesFromLIB(
    MplsLdpApp* ldp,
    unsigned int label,
    Mpls_FEC fec)
{
    unsigned int i = 0;
    unsigned int index = 0;
    BOOL found = FALSE;

    for (i = 0; i < ldp->LIB_NumEntries; i++)
    {
        if ((ldp->LIB_Entries[i].label == label) &&
            ((ldp->LIB_Entries[i].prefix == fec.ipAddress)||
             (fec.ipAddress == ANY_IP)))
        {
            // remember the index of the entry to be deleted
            index = i;
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        if (index == (ldp->LIB_NumEntries - 1))
        {
            //if it is the last entry ....
            memset(&ldp->LIB_Entries[index], 0, sizeof(LIB));
        }
        else
        {
            //compress the empty space
            for (i = index + 1; i < ldp->LIB_NumEntries; i++)
            {
                memcpy(&ldp->LIB_Entries[i - 1],
                    &ldp->LIB_Entries[i],
                    sizeof(LIB));
            }

            memset(&ldp->LIB_Entries[ldp->LIB_NumEntries - 1],
                0,
                sizeof(LIB));
        }
        (ldp->LIB_NumEntries)--;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpPrepareLabelRequestAttributes()
//
// PURPOSE      : preparing label request attributes
//
// PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                peer - pointer to peer LSR's record in hello
//                       adjacency structure
//                fec - fec tobe send with the label request message
//                RAttribute - received attribute value
//                SAttribute - attribute to be sent with label
//                             request message
//                ingressForThisFEC - if LSR is ingress for this fec.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpPrepareLabelRequestAttributes(
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* peer,
    Mpls_FEC  fec,
    Attribute_Type* RAttribute,
    Attribute_Type* SAttribute,
    BOOL ingressForThisFEC)
{
    // A.2.7. MplsLdpPrepareLabelRequestAttributes
    //
    // Summary:
    //
    // This procedure is used whenever a Label Request is to be sent to a
    // Peer to compute the Hop Count and Path Vector, if any, to include
    // in the message.
    //
    // Parameters:
    //
    // -  Peer.  The LDP peer to which the message is to be sent.
    //
    // -  FEC.  The FEC for which a label request is to be sent.
    //
    // -  RAttributes.  The attributes this LSR associates with the LSP
    //    for FEC.
    //
    // -  SAttributes.  The attributes to be included in the Label
    //    Request message.

    BOOL hopCountRequiredForPeer = ldp->hopCountNeededInLabelRequest;

    // if a LSR is ingress RAttribute is NULL. If a LSR is not
    // ingress RAttribute must not be NULL
    if (!ingressForThisFEC)
    {
        ERROR_Assert(RAttribute && SAttribute, "Error in"
            " MplsLdpPrepareLabelRequestAttributes()"
            " LSR not Ingress but RAttribute is NULL");
    }

    // PRqA.1  Is Hop Count required for this Peer (see Note 1.) ? OR
    //         Do RAttributes include a Hop Count? OR
    //         Is Loop Detection configured on LSR?
    //         If not, goto PRqA.14.
    if ((hopCountRequiredForPeer) ||
        ((RAttribute) && (RAttribute->hopCount)) ||
        (ldp->loopDetection))
    {
        // PRqA.2 Is LSR ingress for FEC?
        //        If not, goto PRqA.6.
        if (ingressForThisFEC)
        {
            // PRqA.3  Include Hop Count of 1 in SAttributes
            SAttribute->hopCount = TRUE;
            SAttribute->numHop = 1;

            // PRqA.4  Is Loop Detection configured on LSR?
            //        If not, goto PRqA.14.
            if (!(ldp->loopDetection))
            {
                // PRqA.14 done
                return;
            }

            // PRqA.5 Is LSR merge-capable?
            //        If so, goto PRqA.14.
            //        If not, goto PRqA.13.
            if (!ldp->supportLabelMerging)
            {
                // PRqA.13 Include Path Vector of length 1 containing
                //         LSR Id in SAttributes.
                SAttribute->pathVector = TRUE;
                SAttribute->pathVectorValue.length = 1;
                SAttribute->pathVectorValue.pathVector =
                    (NodeAddress*)MEM_malloc(sizeof(NodeAddress));
                SAttribute->pathVectorValue.pathVector[0] = ldp->LSR_Id;

                // PRqA.14 do
                return;
            }
            else
            {
                // PRqA.14 done
                return;
            }
        }

        // PRqA.6  Do RAttributes include a Hop Count?
        //         If not, goto PRqA.8.
        if (RAttribute->hopCount)
        {
            // PRqA.7  Increment RAttributes Hop Count and copy the
            //      resulting Hop  Count to SAttributes.  (See Note 2.)
            //        Goto PRqA.9.
            SAttribute->hopCount = TRUE;

            RAttribute->numHop =
                MPLS_INCREMENT_HOP_COUNT_IF_KNOWN(RAttribute->numHop);

            SAttribute->numHop = RAttribute->numHop;
        }
        else
        {
            // PRqA.8  Include Hop Count of unknown (0) in SAttributes.
            SAttribute->hopCount = FALSE;
            SAttribute->numHop = MPLS_UNKNOWN_NUM_HOPS;
        }

        // PRqA.9  Is Loop Detection configured on LSR?
        //         If not, goto PRqA.14.
        if (ldp->loopDetection)
        {
            // PRqA.10 Do RAttributes have a Path Vector?
            //        If so, goto PRqA.12.
            if (RAttribute->pathVector)
            {
                //  PRqA.12 Add LSR Id to beginning of Path Vector from
                //          RAttributes and copy the resulting Path Vector
                //          into SAttributes.  Goto PRqA.14.

                MplsLdpCopySAttibuteToRAttribute(
                    ldp->LSR_Id,
                    RAttribute,
                    SAttribute);
            }
            else
            {
                // PRqA.11 Is LSR merge-capable?
                //         If so, goto PRqA.14.
                //         If not, goto PRqA.13.
                if (!(ldp->supportLabelMerging))
                {
                    // PRqA.13 Include Path Vector of length 1 containing
                    //         LSR Id in SAttributes.
                    SAttribute->pathVector = TRUE;
                    SAttribute->pathVectorValue.length = 1;

                    SAttribute->pathVectorValue.pathVector =
                        (NodeAddress*)MEM_malloc(sizeof(NodeAddress));

                    SAttribute->pathVectorValue.pathVector[0] = ldp->LSR_Id;
                }
            }
        }
    }
    else
    {
        // PRqA.14 done
        return;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : IsIngress()
//
// PURPOSE      : checks if LSR is ingress based on RAttribute and Request
//                status
//
// PARAMETER    : RAttribute - Received attribute
//                requestStatus - Label Request Status
//
// RETURN VALUE : TRUE if LSR is Ingress or FALSE otherwise
//-------------------------------------------------------------------------
static
BOOL IsIngress(Attribute_Type* RAttribute, LabelRequestStatus requestStatus)
{
    return ((RAttribute == NULL) && (requestStatus == Request_When_Needed));
}


//-------------------------------------------------------------------------
//  FUNCTION     : IsIngress()
//
//  PURPOSE      : checks if an LSR is Ingress LSR or not for an
//                 Incoming packet
//                 Code used in IP+MPLS integration
//
//  PARAMETERS   : node - the node to check if it is ingress
//                 msg - the message received by the node.
//                 ldp - pointer to the MplsLdpApp structure.
//                 isEdgeRouter - is the node configured as Edge Router
//
//  RETURN VALUE : TRUE if the node is an Edge Route OR node is source
//                 AND next hop is in the helloadjacency of the node,
//                 FALSE otherwise
//
//  ASSUMPTIONS  : none
//-------------------------------------------------------------------------

BOOL IsIngress(
    Node* node,
    Message* msg,
    BOOL isEdgeRouter)
{
    MplsLdpApp *ldp = NULL;
    ldp = MplsLdpAppGetStateSpace(node);

    BOOL srcOfPacket = FALSE;

    IpHeaderType* ipHeader = (IpHeaderType*)  MESSAGE_ReturnPacket(msg);
    int i;
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ipHeader->ip_src == NetworkIpGetInterfaceAddress(node, i))
        {
            srcOfPacket = TRUE;
            break;
        }
    }
    if ((isEdgeRouter) || (srcOfPacket))
    {
        NodeAddress nextHopAddress = (NodeAddress) NETWORK_UNREACHABLE;

        if (IsNextHopExistForThisDest(node, ipHeader->ip_dst,
                                      &nextHopAddress))
        {
            MplsLdpHelloAdjacency* peerSource = NULL;
            LDP_Identifier_Type notUsed;
            memset(&notUsed, 0, sizeof(LDP_Identifier_Type));
            peerSource =  MplsLdpReturnHelloAdjacency(
                                         ldp,
                                         nextHopAddress,
                                         notUsed,
                                         TRUE,    //  match link address
                                         FALSE);    // do not match LSR_Id

            // If the next hop from the IP forwarding table lies in the
            // hello adjacency of the node, then it will be Ingress.
            if (peerSource)
            {
                // Until the connection is established with peer source,
                // return False
                if (peerSource->OkToSendLabelRequests)
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
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpPerformLabelRequestProcedure()
//
// PURPOSE      : preparing label request attributes and sending label
//                request message.
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structure.
//                peer - pointer to peer LSR's record in the
//                       hello adjacency structure
//                fec - fec tobe send with the label request message
//                RAttribute - received attribute value
//                SAttribute - attribute to be sent with label
//                             request message
//                requestStatus - LabelRequestStatus
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
BOOL MplsLdpPerformLabelRequestProcedure(
    Node* node,
    MplsLdpApp* ldp,
    MplsLdpHelloAdjacency* peer,
    Mpls_FEC fec,
    Attribute_Type* RAttribute,
    Attribute_Type* SAttribute,
    NodeAddress upstreamLSR_Id,
    LabelRequestStatus requestStatus)
{
    switch (requestStatus)
    {
        case Request_Never :
        {
             // Goto LRq.13.
             return TRUE;
        }
        case Request_When_Needed :
        case Request_On_Request  :
        {
            BOOL success = FALSE;

            // 1. Execute procedure MplsLdpPrepareLabelRequestAttributes
            //    (Next Hop, FEC, RAttributes, SAttributes);
            MplsLdpPrepareLabelRequestAttributes(
                ldp,
                peer,
                fec,
                RAttribute,
                SAttribute,
                IsIngress(RAttribute, requestStatus));

            // 2. Execute procedure MplsLdpSendLabelRequest (Next Hop, FEC,
            //    SAttributes).
            //    Goto LRq.13.
            success = MplsLdpSendLabelRequest(
                          node,
                          ldp,
                          peer,
                          upstreamLSR_Id,
                          fec,
                          SAttribute);

            // Goto LRq.13.
            return success;
        }
        default :
        {
            ERROR_Assert(FALSE, "Error in "
                " MplsLdpPerformLabelRequestProcedure()"
                " Unknown LabelRequestStatus !!!");

            return FALSE;
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpNewFECRecognized()
//
// PURPOSE      : Perform Label Request for a newly detected FEC.
//
// PARAMETERS   : node - the node which has recognized the new fec.
//                mpls - the mpls structure (in the mac layer).
//                destAddr - destination address embedded in the fec.
//                interfaceIndex - incoming interface index.
//                nextHop - next hop peer
//
// RETURN VALUE : none.
//
// ASSUMPTIONS  : This function's parameter list must match
//                typedef void (*MplsNewFECCallbackFunctionType)
//                defined in "mpls.h"
//-------------------------------------------------------------------------
void
MplsLdpNewFECRecognized(
    Node* node,
    MplsData* mpls,
    NodeAddress destAddr,
    int interfaceIndex,
    NodeAddress nextHopAddr)
{
    MplsLdpApp* ldp;
    Mpls_FEC fec;
    Attribute_Type notUsed;

    ldp = MplsLdpAppGetStateSpace(node);

    memset(&notUsed, 0, sizeof(Attribute_Type));

    fec.ipAddress = destAddr;
    fec.numSignificantBits = (sizeof(NodeAddress) * 8);
    // Changed for adding priority support for FEC classification.
    // By default the priority value is zero.
    fec.priority = 0 ;

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d: MplsLdpNewFECRecognized(dest = %d /"
               "interfaceIndex = %d)\n",
               node->nodeId,
               destAddr,
               interfaceIndex);
    }

    MplsLdpRecognizeNewFEC(
        node,
        ldp,
        fec,
        nextHopAddr,
        NULL,
        (Attribute_Type*) &notUsed,
        0);
    if (notUsed.pathVectorValue.pathVector != NULL)
    {
        MEM_free(notUsed.pathVectorValue.pathVector);
        notUsed.pathVectorValue.pathVector = NULL;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpPerformLsrLabelDistributionProcedure()
//
// PURPOSE      : perfrom label distribution procedure.
//
// PARAMETERS   : node - which is performing the above operation
//                ldp - pointer to the MplsLdpApp structure.
//                fec - fec
//                peer - pointer to peer LSR's record in the
//                       hello adjacency structure
//                RAttribute - received attribute value
//                SAttribute - attribute to be sent with label
//                             mapping message
//                msgSource - node from which label mapping message has
//                            been received.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
BOOL MplsLdpPerformLsrLabelDistributionProcedure(
    Node* node,
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    MplsLdpHelloAdjacency* peer,
    Attribute_Type* SAttribute,
    Attribute_Type* RAttribute,
    NodeAddress msgSource)
{
    BOOL pendingReqExist = FALSE;
    NodeAddress upstream = MPLS_LDP_INVALID_PEER_ADDRESS;
    unsigned int labelSent = 0;

    // For Downstream Unsolicited Independent Control OR
    // For Downstream Unsolicited Ordered Control
    if (ldp->labelAdvertisementMode == UNSOLICITED)
    {
        // 1. Execute procedure MplsLdpPrepareLabelMappingAttributes(Peer,
        //    FEC,RAttributes, SAttributes, IsPropagating, UnknownHopCount).
        MplsLdpPrepareLabelMappingAttributes(
            node,
            ldp,
            peer,
            fec,
            RAttribute,
            SAttribute,
            TRUE,
            MPLS_UNKNOWN_NUM_HOPS);

        // 2. Execute procedure MplsLdpSendLabel (Peer, FEC, SAttributes).
        //    If the procedure fails, continue iteration for
        //    next Peer at LMp.17.
        labelSent = MplsLdpSendLabel(node, ldp, peer, fec, SAttribute, TRUE);

        if (labelSent)
        {
            return labelSent;
        }

        // If no pending request for peer goto LMP 30
        pendingReqExist = IsItDuplicateOrPendingRequest(
                              ldp,
                              fec,
                              msgSource,
                              &upstream,
                              TRUE);
        if (pendingReqExist)
        {
            return FALSE;
        }
        return FALSE;
    }

    // For Downstream On Demand Independent Control OR
    // For Downstream On Demand Ordered Control
    if (ldp->labelAdvertisementMode == ON_DEMAND)
    {
        NodeAddress upstream = MPLS_LDP_INVALID_PEER_ADDRESS;

        if (MPLS_LDP_DEBUG)
        {
            printf("For Downstream On Demand Independent Control OR\n"
                   "For Downstream On Demand Ordered Control\n"
                   "#%d in MplsLdpPerformLsrLabelDistributionProcedure()"
                   "fec = %d",
                   node->nodeId,
                   fec.ipAddress);
        }

        // 1. Iterate through Step 5 for each pending label
        //    request for FEC from Peer marked as pending.
        while (IsItDuplicateOrPendingRequest(
                   ldp,
                   fec,
                   msgSource,
                   &upstream,
                   TRUE))
        {
            // 2. Execute procedure
            //    MplsLdpPrepareLabelMappingAttributes(Peer, FEC,
            //    RAttributes, SAttributes, IsPropagating,
            //    UnknownHopCount)
            peer = MplsLdpReturnHelloAdjacencyForThisLSR_Id(ldp, upstream);

            ERROR_Assert(peer, "Error in"
                " MplsLdpPerformLsrLabelDistributionProcedure()"
                " peer Not found !!!");

            MplsLdpPrepareLabelMappingAttributes(
                node,
                ldp,
                peer,
                fec,
                RAttribute,
                SAttribute,
                TRUE,
                MPLS_UNKNOWN_NUM_HOPS);

            //  3. Execute procedure MplsLdpSendLabel (Peer, FEC,
            //     SAttributes).
            //     If the procedure fails, continue iteration for next
            //     Peer at LMp.17.
            // This code has been added as part of IP-MPLS integration
            // RFC 3036, Section 3.5.7.1.1
            // If an LSR is configured for independent control, a mapping
            // message is transmitted by the LSR.
            // 5. The receipt of a mapping from the downstream next hop  AND
            //        a) no upstream mapping has been created  OR
            //        b) loop detection is configured  OR
            //        c) the attributes of the mapping have changed.
            BOOL isLabelSent = FALSE;
            if (ldp->labelDistributionControlMode ==
                     MPLS_CONTROL_MODE_INDEPENDENT)
            {
                Mpls_FEC fec_1;
                unsigned int label_1 = 0;
                unsigned int hopCount = 0;
                isLabelSent = MplsLdpRetrivePreviouslySentLabelMapping(
                    ldp,
                    fec,
                    peer->LSR2.LSR_Id,
                    &label_1,
                    &hopCount,
                    &fec_1,
                    FALSE);
            }
            if (!isLabelSent)
            {
                labelSent =
                MplsLdpSendLabel(node, ldp, peer, fec, SAttribute, TRUE);
            }
            // 4. Delete record of pending request.
            MplsLdpDeleteDuplicateOrPendingRequest(ldp, fec, msgSource);

            // 5. End iteration from Step 1.
        }
        // Goto LMP.30.
        return labelSent;
    }

    // control should not reach this block of code under any condition
    ERROR_Assert(FALSE, "Error in"
        " MplsLdpPerformLsrLabelDistributionProcedure()"
        " Unknown Label Advertisement Mode !!!");

    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION   : MplsLdpDetectChangeInFecNextHop()
//
// PURPOSE    : taking necessery actions when next hop for a LSR
//              changes.
//
// PARAMETERS : node - node which is generating response to a label
//                       request message.
//              ldp - pointer to the MplsLdpApp structure.
//              oldLabel - old label to be cleared
//              fec - The FEC for which next hop has changed.
//              newNextHop - current next hop
//              oldNextHop - previous next hop
//              RAttribute - Attributes to be included in Label Label
//                           Request message if any, sent to New Next Hop.
//              SAttribute - The attributes, if any, currently associated
//                           with the FEC.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void MplsLdpDetectChangeInFecNextHop(
    Node* node,
    MplsLdpApp* ldp,
    unsigned int oldLabel,
    Mpls_FEC fec,
    NodeAddress newNextHop,
    NodeAddress oldNextHop,
    NodeAddress oldPeerLSR_Id,
    Attribute_Type* RAttribute)
{
    // A.1.7. Detect Change in FEC Next Hop
    //
    // Summary:
    //
    // The response by an LSR to a change in the next hop for a FEC may
    // involve one or more of the following actions:
    //
    // -  Removal of the label from the FEC's old next hop from
    //    forwarding/switching use;
    //
    // -  Transmission of label mapping messages for the FEC to one or
    //    more LDP peers;
    //
    // -  Transmission of a label request to the FEC's new next hop;
    //
    // -  Any of the actions that can occur when the LSR receives a label
    //    mapping from the FEC's new next hop.
    //
    // Context:
    //
    // -  LSR.  The LSR handling the event.
    //
    // -  FEC.  The FEC whose next hop changed.
    //
    // -  New Next Hop.  The current next hop for the FEC.
    //
    // -  Old Next Hop.  The previous next hop for the FEC.
    //
    // -  OldLabel.  Label, if any, previously received from Old Next
    //    Hop.
    //
    // -  CurAttributes.  The attributes, if any, currently associated
    //     with the FEC.
    //
    // -  SAttributes.  Attributes to be included in Label Label Request
    //    message, if any, sent to New Next Hop.
    unsigned int i = 0;
    NodeAddress nextHopAddress = MPLS_LDP_INVALID_PEER_ADDRESS;
    NodeAddress upstream = MPLS_LDP_INVALID_PEER_ADDRESS;
    LDP_Identifier_Type notUsed;
    MplsLdpHelloAdjacency* oldPeerSource = NULL;
    MplsLdpHelloAdjacency* newPeerSource = NULL;
    unsigned int labelRetained = 0;
    BOOL isNextHopExists = FALSE;

    memset(&notUsed, 0, sizeof(LDP_Identifier_Type));

    oldPeerSource = MplsLdpReturnHelloAdjacency(
                        ldp,
                        oldNextHop,
                        notUsed,
                        TRUE,
                        FALSE);

    // NH.1   Has LSR previously received and retained a label mapping
    //        for FEC from Old Next Hop?
    //        If not, goto NH.6.
    Attribute_Type* tmpRAttribute = NULL;
    if (MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
            ldp,
            fec,
            &labelRetained,
            ((oldPeerSource) ? oldPeerSource->LSR2.LSR_Id : oldPeerLSR_Id),
            &tmpRAttribute))
    {
        ERROR_Assert(labelRetained == oldLabel, "Error in"
            " MplsLdpDetectChangeInFecNextHop() !!!");

        if (MPLS_LDP_DEBUG)
        {
            printf("in function MplsLdpDetectChangeInFecNextHop()\n"
                   "node = %d Label to be deleted %d \n",
                   node->nodeId,
                   oldLabel);

            Print_FTN_And_ILM(node);
        }

        // NH.2   Remove label from forwarding/switching use.
        MplsLdpRemoveLabelForForwardingSwitchingUse(
             node,
             oldNextHop,
             oldLabel,
             OUTGOING,
             fec);

        // NH.3   Is LSR using Liberal label retention?
        //        If so, goto NH.6.
        if (ldp->labelRetentionMode != LABEL_RETENTION_MODE_LIBERAL)
        {
            // NH.4   Execute procedure Send_Message (Old Next Hop, Label
            //        Release, OldLabel).
            if (oldPeerSource)
            {
                MplsLdpSendLabelReleaseOrWithdrawMessage(
                    node,
                    ldp,
                    oldPeerSource,
                    oldLabel,
                    fec,
                    NO_LOOP_DETECTED,
                    TRUE,
                    ldp->currentMessageId,
                    LDP_LABEL_RELEASE_MESSAGE_TYPE);

                // send the pdu

                MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                    node,
                    ldp,
                    oldPeerSource);

                ldp->currentMessageId ++;
            }

            // NH.5   Delete label mapping for FEC previously received
            //        from Old Next Hop.
            MplsLdpDeleteEntriesFromIncomingFecToLabelMappingRec(
                ldp,
                fec,
                ((oldPeerSource) ? oldPeerSource->LSR2.LSR_Id :
                    oldPeerLSR_Id));
        }
    }

    // NH.6   Does LSR have a label request pending with Old Next Hop?
    //        If not, goto NH.10.
    if (IsItDuplicateOrPendingRequest(
            ldp,
            fec,
            ((oldPeerSource) ? oldPeerSource->LSR2.LSR_Id :
                oldPeerLSR_Id),
            &upstream,
            FALSE))
    {
        // NH.7   Is LSR using Conservative label retention?
        //        If not, goto NH.10.
        if (ldp->labelRetentionMode == LABEL_RETENTION_MODE_CONSERVATIVE)
        {
            //  NH.8   Execute procedure Send_Message (Old Next Hop,
            //         Label Abort Request, FEC, TLV), where TLV is a
            //         Label Request Message ID TLV that carries the
            //         message ID of the pending label request.
            //

            //  NH.9   Record a label abort request is pending for FEC
            //         with Old Next Hop.
        }
    }

    isNextHopExists = IsNextHopExistForThisDest(
                          node,
                          fec.ipAddress,
                          &nextHopAddress);

    newPeerSource = MplsLdpReturnHelloAdjacency(
                        ldp,
                        nextHopAddress,
                        notUsed,
                        TRUE,
                        FALSE);

    // NH.10  Is there a New Next Hop for the FEC?
    //        If not, goto NH.16.
    if (isNextHopExists  &&  newPeerSource)
    {
        // NH.11  Has LSR previously received and retained a label
        //        mapping for FEC from New Next Hop?
        //        If not, goto NH.13.
        if (MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
                ldp,
                fec,
                &labelRetained,
                newPeerSource->LSR2.LSR_Id,
                &tmpRAttribute))
        {
            // NH.12  Generate Event: Received Label Mapping from
            //        New Next Hop.
            //        Goto NH.20.  (See Note 2.)
            MplsLdpReceiveLabelMapping(
               node,
               ldp,
               ldp->localAddr[0],
               labelRetained,
               fec,
               tmpRAttribute,
               FALSE);

            // NH.20  DONE.
            return;
        }

        //  NH.13  Is LSR using Downstream on Demand advertisement? OR
        //         Is Next Hop using Downstream on Demand advertisement?
        //         OR Is LSR using Conservative label retention?
        //         (See Note 3.)
        //         If so, goto NH.14.
        //         If not, goto NH.20.
        if ((ldp->labelAdvertisementMode == ON_DEMAND) ||
            (ldp->labelRetentionMode ==
                 LABEL_RETENTION_MODE_CONSERVATIVE))

        {
            Attribute_Type SAttribute;

            //  NH.14  Execute procedure MplsLdpPrepareLabelRequestAttributes
            //         (Next Hop, FEC, CurAttributes, SAttributes)
            MplsLdpPrepareLabelRequestAttributes(
                ldp,
                newPeerSource,
                fec,
                RAttribute,
                &SAttribute,
                FALSE);

            //  NH.15  Execute procedure MplsLdpSendLabelRequest
            //         (New Next Hop, FEC, SAttributes).
            //         (See Note 4.)
            //         Goto NH.20.
            MplsLdpSendLabelRequest(
                node,
                ldp,
                newPeerSource,
                notUsed.LSR_Id,
                fec,
                &SAttribute);
            return;
        }
    }

    // NH.16  Iterate through NH.19 for each Peer.
    for (i = 0; i < ldp->numHelloAdjacency; i++)
    {
        unsigned int labelSent = 0;
        unsigned int hopCount = 0;

        //  NH.17  Has LSR previously sent a label mapping for
        //         FEC to Peer? If not, continue iteration for next
        //         Peer at NH.16
        if (!MplsLdpRetrivePreviouslySentLabelMapping(
                 ldp,
                 fec,
                 ldp->helloAdjacency[i].LSR2.LSR_Id,
                 &labelSent,
                 &hopCount,
                 NULL,
                 FALSE))
        {
            // continue iteration for next Peer at NH.16
            continue;
        }

        //  NH.18  Execute procedure MplsLdpSendLabelWithdraw (Peer,
        //         FEC, Label previously sent to Peer).
        MplsLdpSendLabelReleaseOrWithdrawMessage(
            node,
            ldp,
            &ldp->helloAdjacency[i],
            labelSent,
            fec,
            NO_LOOP_DETECTED,
            TRUE,
            ldp->currentMessageId,
            LDP_LABEL_WITHDRAW_MESSAGE_TYPE);

        // send the pdu

        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
            node,
            ldp,
            &ldp->helloAdjacency[i]);

        // NH.19  End iteration from NH.16.
        MplsLdpDeleteEntriesFromLIB(ldp, labelSent, fec);
    }
    // NH.20  DONE.
}

//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpInitStat()
//
//  PURPOSE      : initializing MPLS-LDP protocol's  statictical results.
//
//  PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//
//  RETURN VALUE : none
//
//  ASSUMPTIONS  : none
//-------------------------------------------------------------------------
static
void MplsLdpInitStat(MplsLdpApp* ldp)
{
    ldp->ldpStat.numLabelRequestMsgSend = 0;
    ldp->ldpStat.numLabelMappingSend = 0;
    ldp->ldpStat.numNotificationMsgSend = 0;
    ldp->ldpStat.numKeepAliveMsgSend = 0;
    ldp->ldpStat.numLabelWithdrawMsgSend = 0;
    ldp->ldpStat.numLabelReleaseMsgSend = 0;
    ldp->ldpStat.numAddressMsgSend = 0;
    ldp->ldpStat.numInitializationMsgSend = 0;
    ldp->ldpStat.numUdpLinkHelloSend = 0;
    ldp->ldpStat.numLabelRequestMsgReceived = 0;
    ldp->ldpStat.numLabelMappingReceived = 0;
    ldp->ldpStat.numNotificationMsgReceived = 0;
    ldp->ldpStat.numKeepAliveMsgReceived = 0;
    ldp->ldpStat.numLabelWithdrawMsgReceived = 0;
    ldp->ldpStat.numLabelReleaseMsgReceived = 0;
    ldp->ldpStat.numAddressMsgReceived = 0;
    ldp->ldpStat.numInitializationMsgReceived = 0;
    ldp->ldpStat.numInitializationMsgReceivedAsActive = 0;
    ldp->ldpStat.numInitializationMsgReceivedAsPassive = 0;
    ldp->ldpStat.numUdpLinkHelloReceived = 0;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpScheduleHelloMessageWithDelay()
//
// PURPOSE      : Scheduling hello message with a given elay
//
// PARAMETERS   : node - the node which is initializing
//                ldp - pointer to MplsLdpApp
//                delay - delay
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
static
void MplsLdpScheduleHelloMessageWithDelay(
    Node* node,
    MplsLdpApp* ldp,
    clocktype delay)
{
    Message* msg = MESSAGE_Alloc(
                      node,
                      APP_LAYER,
                      APP_MPLS_LDP,
                      MSG_APP_MplsLdpSendHelloMsgTimerExpired);

    MESSAGE_Send(node, msg, delay);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpInit()
//
// PURPOSE      : initializing MPLS-LDP protocol.
//
// PARAMETERS   : node - the node which is initializing
//                nodeInput - the input configuration file
//                sourceAddr - address of the node which is initializing
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsLdpInit(
    Node* node,
    const NodeInput* nodeInput,
    NodeAddress sourceAddr)
{
    char buffer[MAX_STRING_LENGTH];
    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    if (!MplsReturnStateSpace(node))
    {
        // if mac-layer MPLS engine is not initialized then
        // do not initialize MPLS-LDP protocol
        return;
    }

    if (!ldp)
    {
        BOOL found = FALSE;

        ldp = (MplsLdpApp*) MEM_malloc(sizeof(MplsLdpApp));

        // allocate space for holding local addrecess and
        // initialize it
        ldp->maxLocalAddr = MPLS_LDP_APP_INITIAL_NUM_LOCAL_ADDR;

        ldp->localAddr = (NodeAddress*)
                          MEM_malloc(sizeof(NodeAddress)
                              * MPLS_LDP_APP_INITIAL_NUM_LOCAL_ADDR);

        ldp->localAddr[0] = sourceAddr;
        ldp->numLocalAddr = 1;

        // initialize hello adjacency structure
        ldp->helloAdjacency = (MplsLdpHelloAdjacency*)
             MEM_malloc(sizeof(MplsLdpHelloAdjacency)
                 * MPLS_LDP_APP_INITIAL_NUM_HELLO_ADJACENCY);
        // Initializing helloAdjacency to Zero
        memset(ldp->helloAdjacency, 0, sizeof(MplsLdpHelloAdjacency)
                 * MPLS_LDP_APP_INITIAL_NUM_HELLO_ADJACENCY);

        ldp->maxHelloAdjacency = MPLS_LDP_APP_INITIAL_NUM_HELLO_ADJACENCY;
        ldp->numHelloAdjacency = 0;

        // initialize current message Id
        ldp->currentMessageId = 1;

        // initialize LSR_Id of this LSR
        ldp->LSR_Id = sourceAddr;

        // initialize next lable to be allocated
        ldp->nextLabel = (Last_Reserved + 1);

        MplsLdpIntervalReadTime(node->nodeId, nodeInput,
            "MPLS-LINK-HELLO-HOLD-TIME", &(ldp->linkHelloHoldTime),
            LDP_DEFAULT_LINK_HELLO_HOLD_TIME);

        MplsLdpIntervalReadTime(node->nodeId, nodeInput,
            "MPLS-LINK-HELLO-INTERVAL", &(ldp->linkHelloInterval),
            LDP_DEFAULT_LINK_HELLO_INTERVAL);

        MplsLdpIntervalReadTime(node->nodeId, nodeInput,
            "MPLS-TARGETED-HELLO-INTERVAL", &(ldp->targetedHelloInterval),
            LDP_DEFAULT_TARGETED_HELLO_INTERVAL);

        // Read default keepAlive time from user.
        MplsLdpIntervalReadTime(node->nodeId, nodeInput,
             "MLPS-LDP-DEFAULT-KEEP-ALIVE-TIME", &(ldp->keepAliveTime),
              LDP_DEFAULT_KEEP_ALIVE_TIME_INTERVAL);

        MplsLdpIntervalReadTime(node->nodeId, nodeInput,
            "MPLS-SEND-KEEP-ALIVE-DELAY", &(ldp->sendKeepAliveDelay),
             DEFAULT_MPLS_SEND_KEEP_ALIVE_DELAY);

        MplsLdpIntervalReadTime(node->nodeId, nodeInput,
            "LDP-DEFAULT-LABEL-REQUEST-TIME-OUT",
            &(ldp->requestRetryTimer),
            LDP_DEFAULT_LABEL_REQUEST_TIME_OUT);

        // query statistics to collected or not
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-LDP-STATISTICS",
            &found,
            buffer);

        if (found)
        {
            if (strcmp(buffer, "YES") == 0)
            {
                ldp->collectStat = TRUE;
            }
            else if (strcmp(buffer, "NO") == 0)
            {
                ldp->collectStat = FALSE;
            }
            else
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr, "\"MPLS-LDP-STATICS\""
                        " must be specified as \"YES\"\n"
                        "or \"NO\", not: %s\n", buffer);

                ERROR_ReportError(errorStr);
            }
        }
        else
        {
            // our default option of MPLS-LDP-STATISTICS is FALSE
            ldp->collectStat = FALSE;
        }

        // query label distribution mode from user
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-LDP-LABEL-ADVERTISEMENT-MODE",
            &found,
            buffer);

        if (found)
        {
            if (strcmp(buffer, "UNSOLICITED") == 0)
            {
                ldp->labelAdvertisementMode = UNSOLICITED;
            }
            else if (strcmp(buffer, "ON-DEMAND") == 0)
            {
                ldp->labelAdvertisementMode = ON_DEMAND;
            }
            else
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr, "\"MPLS-LDP-LABEL-ADVERTISEMENT-MODE\""
                        " must be specified as \"ON-DEMAND\"\n"
                        "or \"UNSOLICITED\", not: %s\n", buffer);

                ERROR_ReportError(errorStr);
            }
        }
        else
        {
            // our default option of label distribution
            // is "label distribution on demand."
            ldp->labelAdvertisementMode = ON_DEMAND;
        }

        if (ldp->labelAdvertisementMode == ON_DEMAND)
        {
            MplsSetNewFECCallbackFunction(node, &MplsLdpNewFECRecognized);
        }

        // query label distribution mode from user
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-LDP-LOOP-DETECTION",
            &found,
            buffer);

        if (found)
        {
            if (strcmp(buffer, "NO") == 0)
            {
                ldp->loopDetection = FALSE;

                // if LOOP-DETECTION is not enabled then
                // about path vector limit parameter is not needed
                ldp->pathVectorLimit = 0;
            }
            else if (strcmp(buffer, "YES") == 0)
            {
                int pathVectorLimit;
                ldp->loopDetection = TRUE;

                // if LOOP-DETECTION is enabled the query from user
                // about path vector limit
                IO_ReadInt(
                    node->nodeId,
                    ANY_ADDRESS,
                    nodeInput,
                    "MPLS-LDP-PATH-VECTOR-LIMIT",
                    &found,
                    &pathVectorLimit);

                if (found)
                {
                    ERROR_Assert(pathVectorLimit >
                                 MIN_PATH_VECTOR_LIMIT,
                        "path vector limit in less than"
                        " MIN_PATH_VECTOR_LIMIT");

                    ldp->pathVectorLimit = pathVectorLimit;
                }
                else
                {
                    ldp->pathVectorLimit = DEFAULT_PATH_VECTOR_LIMIT;
                }
            }
            else
            {
                ERROR_Assert(FALSE, "\nWrong specification in loop "
                     "detection\nJust give YES or NO ");
            }
        }
        else
        {
            // our default option for loop
            // detection is set to "no"
            ldp->loopDetection = FALSE;
            ldp->pathVectorLimit = 0;
        }

        // query label max PDU length from user.
        IO_ReadInt(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-LDP-MAX-PDU-LENGTH",
            &found,
            &(ldp->maxPDULength));

        if (found)
        {
            ERROR_Assert(ldp->maxPDULength >
                SMALLEST_ALLOWABLE_MAX_PDU_LENGTH,
                "PDU Length is less than "
                "SMALLEST_ALLOWABLE_MAX_PDU_LENGT");
        }
        else
        {
            ldp->maxPDULength = LDP_PRE_NEGOTIATION_MAX_PDU_LENGTH;
        }

        // Ask for LABEL DISTRIBUTION CONTROL MODE from user
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-LABEL-DISTRIBUTION-CONTROL-MODE",
            &found,
            buffer);

        if (found)
        {
            if (strcmp("ORDERED", buffer) == 0)
            {
                ldp->labelDistributionControlMode =
                    MPLS_CONTROL_MODE_ORDERED;
            }
            else if (strcmp("INDEPENDENT", buffer) == 0)
            {
                ldp->labelDistributionControlMode =
                    MPLS_CONTROL_MODE_INDEPENDENT;

            }
            else
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr, "\"MPLS-LABEL-DISTRIBUTION-CONTROL-MODE\" "
                        "must be either \"ORDERED\" or \"INDEPENDENT\"\n"
                        "%s is invalid",
                        buffer);

                ERROR_ReportError(errorStr);
            }
        }
        else
        {
            // The default label distribution control mode is "ORDERED"
            ldp->labelDistributionControlMode = MPLS_CONTROL_MODE_ORDERED;
        }

        // Inform the mac layer about label distribution
        // control mode...
        MplsSetLabelAdvertisementMode(node,
            ldp->labelDistributionControlMode);

        // Ask for LABEL RETENTION MODE form user
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-LABEL-RETENTION-MODE",
            &found,
            buffer);

        if (found)
        {
            if (strcmp("LIBERAL", buffer) == 0)
            {
                ldp->labelRetentionMode = LABEL_RETENTION_MODE_LIBERAL;
            }
            else if (strcmp("CONSERVATIVE", buffer) == 0)
            {
                ldp->labelRetentionMode = LABEL_RETENTION_MODE_CONSERVATIVE;
            }
            else
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr, "\"MPLS-LABEL-RETENTION-MODE\" must be "
                        "specified as either \"LIBERAL\"\n"
                        "or \"CONSERVATIVE\".  %s is invalid.",
                        buffer);

                ERROR_ReportError(errorStr);
            }

        }
        else
        {
             // The default label retention mode is "LIBERAL"
             ldp->labelRetentionMode = LABEL_RETENTION_MODE_LIBERAL;
        }

        // query from user if LSR is to configured for decrementing TTL
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-LDP-DECREMENTS-TTL",
            &found,
            buffer);

        if (found)
        {
            if (strcmp("YES", buffer) == 0)
            {
                ldp->decrementsTTL = TRUE;
            }
            else if (strcmp("NO", buffer) == 0)
            {
                ldp->decrementsTTL = FALSE;
            }
            else
            {
                ERROR_ReportError("\"MPLS-LDP-DECREMENTS-TTL\" should be "
                                  "either \"YES\" or \"NO\".");
            }
        }
        else
        {
            // defaule option for MPLS-LDP-DECREMENTS-TTL is FALSE
            ldp->decrementsTTL = FALSE;
        }

        // query if LSR belongs to member of TTL decrementing domain
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-MEMBER-OF-DECREMENT-TTL-DOMAIN",
            &found,
            buffer);

        if (found)
        {
            if (strcmp("YES", buffer) == 0)
            {
                ldp->member_Dec_TTL_Domain = TRUE;
            }
            else if (strcmp("NO", buffer) == 0)
            {
                ldp->member_Dec_TTL_Domain = FALSE;
            }
            else
            {
                ERROR_ReportError("\"MPLS-MEMBER-OF-DECREMENT-TTL-DOMAIN\" "
                                  "should be either \"YES\" or \"NO\".");
            }
        }
        else
        {
            // default optino for MEMBER-OF-DECREMENT-TTL-DOMAIN
            // is false.
            ldp->member_Dec_TTL_Domain = FALSE;
        }

        // query if LSR will support label merging
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SUPPORT-LABEL-MERGING",
            &found,
            buffer);

        if (found)
        {
            if (strcmp("YES", buffer) == 0)
            {
                ldp->supportLabelMerging = TRUE;
            }
            else if (strcmp("NO", buffer) == 0)
            {
                ldp->supportLabelMerging = FALSE;
            }
            else
            {
                ERROR_ReportError("\"SUPPORT-LABEL-MERGING\" "
                                  "should be either \"YES\" or \"NO\".");
            }
        }
        else
        {
            // default option for SUPPORT-LABEL-MERGING
            // is false
            ldp->supportLabelMerging = FALSE;
        }

        // query if LSR will support label release message
        // to propagate or not
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "CONFIGURED-FOR-LABEL-RELEASE-MESSAGE-PROPAGATE",
            &found,
            buffer);

        if (found)
        {
            if (strcmp("YES", buffer) == 0)
            {
                ldp->propagateLabelRelease = TRUE;
            }
            else if (strcmp("NO", buffer) == 0)
            {
                ldp->propagateLabelRelease = FALSE;
            }
            else
            {
                ERROR_ReportError(
                    "\"CONFIGURED-FOR-LABEL-RELEASE-MESSAGE-PROPAGATE\" "
                    "should be either \"YES\" or \"NO\".");
            }
        }
        else
        {
            // default option for
            // CONFIGURED-FOR-LABEL-RELEASE-MESSAGE-PROPAGATE
            // is false
            ldp->propagateLabelRelease = FALSE;
        }

        // query for maximum alowable hop count value
        IO_ReadInt(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MAX-ALLOWABLE-HOP-COUNT",
            &found,
            &(ldp->maxAllowableHopCount));

        if (found)
        {
            if (ldp->maxAllowableHopCount < MIN_ALLOWABLE_MAX_HOP_COUNT)
            {
                ERROR_ReportError("\"MAX-ALLOWABLE-HOP-COUNT\" should be "
                    "greater than MIN_ALLOWABLE_HOP_COUNT");
            }
        }
        else
        {
            // set a default max Allowable Hop Count value.
            ldp->maxAllowableHopCount = MAX_ALLOWABLE_HOP_COUNT;
        }

        // query if HOP COUNT REQUIRED IN LABEL REQUEST
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "HOP-COUNT-REQUIRED-IN-LABEL-REQUEST",
            &found,
            buffer);

        if (found)
        {
            if (strcmp("YES", buffer) == 0)
            {
                ldp->hopCountNeededInLabelRequest = TRUE;
            }
            else if (strcmp("NO", buffer) == 0)
            {
                ldp->hopCountNeededInLabelRequest = FALSE;
            }
            else
            {
                ERROR_ReportError(
                    "\"CONFIGURED-FOR-LABEL-RELEASE-MESSAGE-PROPAGATE\" "
                    "should be either \"YES\" or \"NO\".");
            }

        }
        else
        {
            // default option for HOP-COUNT-REQUIRED-IN-LABEL-REQUEST
            // is FALSE
            ldp->hopCountNeededInLabelRequest = FALSE;
        }

        // For Storing outgoing labels.
        ldp->outGoingRequestMaxEntries = MAX_OUT_GOING_REQUEST_SIZE;
        ldp->outGoingRequestNumEntries = 0;
        ldp->outGoingRequest = (StatusRecord*)
            MEM_malloc(sizeof(StatusRecord)
                * MAX_OUT_GOING_REQUEST_SIZE);

        // For keeping track of pending request
        ldp->pendingRequestMaxEntries = MAX_PENDING_REQUEST_SIZE;
        ldp->pendingRequestNumEntries = 0;
        ldp->pendingRequest = (PendingRequest*)
            MEM_malloc(sizeof(PendingRequest)
                * MAX_PENDING_REQUEST_SIZE);

        // For keeping track of (LIB)
        ldp->LIB_MaxEntries = MAX_LIB_SIZE;
        ldp->LIB_NumEntries = 0;
        ldp->LIB_Entries = (LIB*)
            MEM_malloc(sizeof(LIB) * MAX_LIB_SIZE);

        // For keeping track of outgoing FEC to Label mapping information
        ldp->fecToLabelMappingRecNumEntries = 0;
        ldp->fecToLabelMappingRecMaxEntries = MAX_LIB_SIZE;
        ldp->fecToLabelMappingRec = (FecToLabelMappingRec*)
            MEM_malloc(sizeof(FecToLabelMappingRec) * MAX_LIB_SIZE);

        // For keeping track of incoming FEC to Label mapping information
        ldp->incomingFecToLabelMappingRecNumEntries = 0;
        ldp->incomingFecToLabelMappingRecMaxEntries = MAX_LIB_SIZE;
        ldp->incomingFecToLabelMappingRec = (FecToLabelMappingRec*)
            MEM_malloc(sizeof(FecToLabelMappingRec) * MAX_LIB_SIZE);

        // For keeping track of Withdrawn fec/label records
        ldp->withdrawnLabelRecNumEntries = 0;
        ldp->withdrawnLabelRecMaxEntries = MAX_LIB_SIZE;
        ldp->withdrawnLabelRec = (LabelWithdrawRec*)
            MEM_malloc(sizeof(FecToLabelMappingRec) * MAX_LIB_SIZE);

        // register this ldp
        APP_RegisterNewApp(node, APP_MPLS_LDP, ldp);

        // initialize MPLS-LDP statistics
        MplsLdpInitStat(ldp);

        // schedule link hello with delay
        MplsLdpScheduleHelloMessageWithDelay(
            node,
            ldp,
            MPLS_LDP_STARTUP_DELAY);

        // set the interface handler function with MAC to handle interface
        // faults.
        for (int i = 0; i < node->numberInterfaces; i++)
        {
            MAC_SetInterfaceStatusHandlerFunction(
                 node,
                 i,
                 &LDPInterfaceStatusHandler);

        } // end of for loop
    }
    else
    {
        MplsLdpAddSourceAddr(ldp, sourceAddr);
    }

    ldp->LSR_Id = ldp->localAddr[0];

    APP_TcpServerListen(
        node,
        APP_MPLS_LDP,
        sourceAddr,
        (short)APP_MPLS_LDP);

    if (MPLS_LDP_DEBUG)
    {
        printf("#%d: Mpls_Ldp_App Init(sourceAddr %d)\n",
               node->nodeId,
               sourceAddr);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpLayer()
//
// PURPOSE      : handling MPLS-LPS's event types.
//
// PARAMETERS   : node - the node which handling the event type
//                msg  - the message received by the node.
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsLdpLayer(Node* node, Message* msg)
{
    if (!MplsLdpAppGetStateSpace(node))
    {
        // if MplsLdpApp is not initialized then return
        MESSAGE_Free(node, msg);
        return;
    }

    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_APP_FromTransListenResult :
        {
            TransportToAppListenResult* listenResult = NULL;

            listenResult = (TransportToAppListenResult*)
                               MESSAGE_ReturnInfo(msg);

            if (MPLS_LDP_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                ctoa(node->getNodeTime(), clockStr);

                printf("MPLS: Node %d at %s got listenResult\n",
                       node->nodeId,
                       clockStr);
            }

            if (listenResult->connectionId == INVALID_TCP_CONNECTION_ID)
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr, "MPLS: Node %d unable to listen to "
                        "TCP port.\n", node->nodeId);

                ERROR_ReportError(errorStr);
            }

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_APP_FromTransport :
        {
            // LSR has received an UDP message from
            // transport layer. Possibly a hello message
            // because only hello messages are send as UDP packet

            int incomingInterfaceIndex = ANY_INTERFACE;
            UdpToAppRecv* info = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

            if (MPLS_LDP_DEBUG)
            {
                char clockStr[MAX_STRING_LENGTH];
                ctoa(node->getNodeTime(), clockStr);

                printf("MPLS: Node %d at %s got data from UDP size %d\n"
                       "\tsourceAddr:Port = %d:%d\n",
                       node->nodeId,
                       clockStr,
                       MESSAGE_ReturnPacketSize(msg),
                       GetIPv4Address(info->sourceAddr),
                       info->sourcePort);
            }
            incomingInterfaceIndex = info->incomingInterfaceIndex;

            // Process MPLS-LDP PDU
            MplsLdpProcessPDU(
                node,
                msg,
                GetIPv4Address(info->sourceAddr),
                incomingInterfaceIndex,
                UDP_PACKET);

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_APP_FromTransOpenResult :
        {
            TransportToAppOpenResult* openResult = NULL;

            // get open result from transport layer
            openResult = (TransportToAppOpenResult*) MESSAGE_ReturnInfo(msg);

            if (openResult->connectionId > 0)
            {
                // LSR has successfully opened a connection
                // with a hello adjacency

                Message* timerMsg = NULL;
                LDP_Identifier_Type notUsed;
                MplsLdpHelloAdjacency* helloAdjacency = NULL;

                MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

                memset(&notUsed, 0, sizeof(LDP_Identifier_Type));

                helloAdjacency = MplsLdpReturnHelloAdjacency(
                                     ldp,
                                     GetIPv4Address(openResult->remoteAddr),
                                     notUsed,
                                     TRUE,      // Match link address
                                     FALSE);    // Do not match LSR_ID

                if (helloAdjacency)
                {
                    // LSR starts the Keep-Alive-Delay-Has-Expired
                    // timer for the hello the adjacency
                    timerMsg = MESSAGE_Alloc(node,
                                   APP_LAYER,
                                   APP_MPLS_LDP,
                                   MSG_APP_MplsLdpSendKeepAliveDelayExpired);

                    MESSAGE_InfoAlloc(
                        node,
                        timerMsg,
                        sizeof(NodeAddress));

                    memcpy(MESSAGE_ReturnInfo(timerMsg),
                         &(helloAdjacency->LSR2.LSR_Id),
                         sizeof(NodeAddress));

                    MESSAGE_Send(
                        node,
                        timerMsg,
                        KEEP_ALIVE_DELAY(ldp));

                    // remember connection Id of the hello adjacency
                    helloAdjacency->connectionId = openResult->connectionId;
                }
                else
                {
                    ERROR_Assert(openResult->type == TCP_CONN_PASSIVE_OPEN,
                        "Error Opening TCP connection");
                }

                if (openResult->type == TCP_CONN_ACTIVE_OPEN)
                {
                    MplsLdpSendInitializationMessage(
                        node,
                        ldp->keepAliveTime,
                        ldp->currentMessageId,
                        helloAdjacency,
                        openResult->connectionId);

                    MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                        node,
                        ldp,
                        helloAdjacency);

                    if (MPLS_LDP_DEBUG)
                    {
                        printf("*** node %d active connection open"
                               " SUCCESSFULL Connection id = %d\n",
                               node->nodeId,
                               openResult->connectionId);
                    }
                }
                else if (openResult->type == TCP_CONN_PASSIVE_OPEN)
                {
                    // if a passive connection is opened then do
                    // nothing, but wait for an initialization
                    // message to come from the active partner.

                    if (MPLS_LDP_DEBUG)
                    {
                        printf("*** node %d PASSIVE connection open\n"
                               " connection Id = %d\n",
                               node->nodeId,
                               openResult->connectionId);
                    }
                }
                else
                {
                    char errorStr[MAX_STRING_LENGTH];

                    sprintf(errorStr, "node %d received an open result that"
                            " was neither active nor passive.\n",
                            node->nodeId);

                    ERROR_ReportError(errorStr);
                }
            }
            else
            {
                char buff[MAX_STRING_LENGTH];
                sprintf(buff, "*** node %d active connection open"
                    " FAILED\n", node->nodeId);
                ERROR_ReportWarning(buff);
            }

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_APP_FromTransCloseResult :
        {
            MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

            TransportToAppCloseResult* closeResult =
                (TransportToAppCloseResult*) MESSAGE_ReturnInfo(msg);

            if (MPLS_LDP_DEBUG)
            {
                printf("node %d has closed the connection %d\n",
                       node->nodeId,
                       closeResult->connectionId);
            }

            MplsLdpSetTcpConnectionClosed(ldp, closeResult->connectionId);
            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_FromTransDataSent :
        {
            if (MPLS_LDP_DEBUG)
            {
                printf("*** node %d MSG_APP_FromTransDataSent\n",
                       node->nodeId);
            }

            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_FromTransDataReceived :
        {
            // Receive transport layer information
            TransportToAppDataReceived*  dataReceivedInfo =
                (TransportToAppDataReceived*) MESSAGE_ReturnInfo(msg);

            if (MPLS_LDP_DEBUG)
            {
                printf("*** node %d MSG_APP_FromTransDataReceived"
                       "%d bytes Connection Id = %d\n",
                       node->nodeId,
                       MESSAGE_ReturnPacketSize(msg),
                       dataReceivedInfo->connectionId);
            }

            MplsLdpProcessPDU(
                node,
                msg,
                ANY_IP,
                dataReceivedInfo->connectionId,
                TCP_PACKET);

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_APP_MplsLdpSendKeepAliveDelayExpired :
        {
            MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);
            NodeAddress LSR_Id = MPLS_LDP_INVALID_PEER_ADDRESS;
            MplsLdpHelloAdjacency* helloAdjacency = NULL;
            clocktype idealTime = 0;

            memcpy(&LSR_Id, MESSAGE_ReturnInfo(msg), sizeof(NodeAddress));

            helloAdjacency = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                                 ldp,
                                 LSR_Id);

            if (!helloAdjacency)
            {
                MESSAGE_Free(node, msg);
                break;
            }

            if (helloAdjacency->connectionId != INVALID_TCP_CONNECTION_ID)
            {
                idealTime = (node->getNodeTime()
                             - (helloAdjacency)->lastMsgSent);

                if (KEEP_ALIVE_DELAY(ldp) <= idealTime)
                {
                    if (BUFFER_GetCurrentSize((helloAdjacency)->buffer) > 0)
                    {
                        // send the pdu
                        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                            node,
                            ldp,
                            helloAdjacency);
                    }
                    else
                    {
                        // send keep alive;
                        MplsLdpSendKeepAliveMessage(
                            node,
                            ldp->currentMessageId,
                            helloAdjacency);

                        MplsLdpTransmitPduFromHelloAdjacencyBuffer(
                            node,
                            ldp,
                            helloAdjacency);

                        (ldp->currentMessageId)++;
                    }

                    MESSAGE_Send(node, msg, KEEP_ALIVE_DELAY(ldp));
                }
                else
                {
                    MESSAGE_Send(
                        node,
                        msg,
                        (KEEP_ALIVE_DELAY(ldp) - idealTime));
                }
            }
            else // if (helloAdjacency->connectionId == -1)
            {
                // else if TCP connection is closed
                // free the timer.
                MESSAGE_Free(node, msg);
            }

            break;
        }
        case MSG_APP_MplsLdpSendHelloMsgTimerExpired :
        {
            MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

            MplsLdpSendLinkHello(
                node,
                ldp->currentMessageId,
                ldp->LSR_Id,
                ldp->linkHelloHoldTime);

            MESSAGE_Send(node, msg, ldp->linkHelloInterval);

            break;
        }
        case MSG_APP_MplsLdpKeepAliveTimerExpired :
        {
            MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);
            BOOL needRemoval = FALSE;
            MplsLdpHelloAdjacency* helloPtr = NULL;
            NodeAddress LSR_Id = MPLS_LDP_INVALID_PEER_ADDRESS;
            NodeAddress oldNextHop = MPLS_LDP_INVALID_PEER_ADDRESS;
            int connectionId = - 1;

            memcpy(&LSR_Id, MESSAGE_ReturnInfo(msg), sizeof(NodeAddress));

            helloPtr = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                           ldp,
                           LSR_Id);

            // If hello adjacency has expired, then do not process it.
            if (helloPtr == NULL)
            {
                MESSAGE_Free(node, msg);
                break;
            }

            ERROR_Assert(helloPtr, "Error in MplsLdpLayer()"
                "Hello Adjacency Not found !!!");

            connectionId = helloPtr->connectionId;
            oldNextHop =  helloPtr->LinkAddress;

            needRemoval = MplsLdpCheckAndRemoveExpiredHelloAdjacencies(
                              node,
                              ldp,
                              helloPtr,
                              TRUE);

            if (needRemoval == FALSE)
            {
                // Hello adjacency is not removed/dead
                // Hence start keep alive timer again.
                MplsLdpStartKeepAliveTimerForThisHelloAdjacency(
                    node,
                    ldp,
                    helloPtr);
            }
            else // if (needRemoval == TRUE)
            {
                unsigned int oldLabel = 0;
                Attribute_Type* RAttribute = NULL;
                Mpls_FEC fec;
                unsigned int label = 0;
                unsigned int hopCount = 0;

                FecToLabelMappingRec* fecToLabelMappingRec = NULL;

                // Initialize fec with any fec to match ANY fec.
                fec.ipAddress = ANY_IP;
                fec.numSignificantBits = 0;
                // Changed for adding priority support for FEC classification.
                // By default the priority value is zero.
                fec.priority = 0 ;

                if (MPLS_LDP_DEBUG)
                {
                    printf("Node = %d Hello adjacency %u is removed\n",
                           node->nodeId,
                           LSR_Id);
                }

                // The following code has been changed while
                // integrating IP + MPLS.
                // If connection to peer is not established
                // or has already been closed, then we should not
                // try to close the connection.
                if (connectionId != INVALID_TCP_CONNECTION_ID)
                {
                    APP_TcpCloseConnection(node, connectionId);
                }

                MplsLdpRetrivePreviouslySentLabelMapping(
                    ldp,
                    fec,
                    LSR_Id,
                    &label,
                    &hopCount,
                    &fec,
                    FALSE);

                if (IamEgressForThisFec(node, fec, ldp))
                {
                    ERROR_Assert(fec.ipAddress != ANY_IP,
                         "Cannot Be Egress for ANY_IP !!!");

                    MplsLdpRetrivePreviouslySentLabelMapping(
                        ldp,
                        fec,
                        LSR_Id,
                        &label,
                        &hopCount,
                        NULL,
                        TRUE);

                    MplsLdpRemoveLabelForForwardingSwitchingUse(
                        node,
                        oldNextHop,   // not Used
                        label,        // in coming label to be deleated
                        INCOMING,
                        fec);         // not used.

                    MplsLdpDeleteEntriesFromLIB(ldp, label, fec);
                } // end if (IamEgressForThisFec(node, fec, ldp))

                // Try matching ANY fec.
                fec.ipAddress = ANY_IP;
                fec.numSignificantBits = 0;
                // Changed for adding priority support for FEC classification.
                // By default the priority value is zero.
                fec.priority = 0 ;

                while ((fecToLabelMappingRec =
                       MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
                           ldp,
                           fec,
                           &oldLabel,
                           LSR_Id,
                           &RAttribute)) != NULL)
                {
                    NodeAddress newNextHop = MPLS_LDP_INVALID_PEER_ADDRESS;

                    ERROR_Assert(fecToLabelMappingRec,
                                 "fecToLabelMappingRec is NULL!!!");

                    memcpy(&fec,
                           &(fecToLabelMappingRec->fec),
                           sizeof(Mpls_FEC));

                    IsNextHopExistForThisDest(
                        node,
                        fec.ipAddress,
                        &newNextHop);

                    MplsLdpGetFecFromFecToLabelMappingRec(
                        ldp,
                        LSR_Id,
                        &fec);

                    MplsLdpDetectChangeInFecNextHop(
                        node,
                        ldp,
                        oldLabel,
                        fec,
                        newNextHop,
                        oldNextHop,
                        LSR_Id,
                        RAttribute);

                    MplsLdpDeleteEntriesFromIncomingFecToLabelMappingRec(
                        ldp,
                        fec,
                        LSR_Id);

                    MplsLdpRemoveLabelForForwardingSwitchingUse(
                        node,
                        oldNextHop,  // match old next hop
                        oldLabel,    // match outgoing label
                        OUTGOING,
                        fec);        // match fec

                    // Try matching ANY fec.
                    fec.ipAddress = ANY_IP;
                    fec.numSignificantBits = 0;
                    // Changed for adding priority support for FEC classification.
                    // By default the priority value is zero.
                    fec.priority = 0 ;
                } // end while (...)
            }// end else (needRemoval == TRUE)
            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_MplsLdpLabelRequestTimerExpired :
        {
            MplsLdpHelloAdjacency* peer = NULL;
            MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

            DefferedRequest defReq;
            Attribute_Type SAttribute;

            // TO.1    Retrieve the record of the deferred label request.
            memcpy(&defReq,
                   MESSAGE_ReturnInfo(msg),
                   sizeof(DefferedRequest));

            peer = MplsLdpReturnHelloAdjacencyForThisLSR_Id(
                       ldp,
                       defReq.LSR_Id);

            MplsLdpAddAttributeType(&SAttribute, &defReq.SAttribute);

            // TO.2    Is Peer the next hop for FEC?
            //         If not, goto TO.4.
            if (peer)
            {
                if (MPLS_LDP_DEBUG)
                {
                    printf("MSG_APP_MplsLdpLabelRequestTimerExpired\n"
                           "#%d start label  deferred Request procedure\n"
                           "Fec.IpAddress = %u LsrId = %u\n",
                           node->nodeId,
                           defReq.fec.ipAddress,
                           defReq.LSR_Id);
                }

                // TO.3    Execute procedure MplsLdpSendLabelRequest
                //         (Peer, FEC).
                MplsLdpSendLabelRequest(
                    node,
                    ldp,
                    peer,
                    defReq.LSR_Id,
                    defReq.fec,
                    &SAttribute);
            }

            MplsLdpFreeAttributeType(&SAttribute);

            // TO.4    DONE.
            MplsLdpFreeAttributeType(&(((DefferedRequest*)
                MESSAGE_ReturnInfo(msg))->SAttribute));

            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_MplsLdpFaultMessageDOWN:
        {
            // Interface is Down
            int interfaceIndex;
            memcpy(&interfaceIndex, MESSAGE_ReturnInfo(msg),
                   sizeof(int));
            LDPHandleInterfaceEvent(node,
                                   interfaceIndex,
                                   LDP_InterfaceDown);
            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_MplsLdpFaultMessageUP:
        {
            // Interface is Up
            int interfaceIndex;
            memcpy(&interfaceIndex, MESSAGE_ReturnInfo(msg),
                   sizeof(int));
            LDPHandleInterfaceEvent(node,
                                   interfaceIndex,
                                   LDP_InterfaceUp);
            MESSAGE_Free(node, msg);
            break;

        }
        default:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr,
                "MPLS: Node %d received unknown event %d\n",
                node->nodeId,
                MESSAGE_GetEvent(msg));

            ERROR_ReportError(errorStr);
        }
    }// end switch (MESSAGE_GetEvent(msg))
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpPrintStat()
//
// PURPOSE      : printing MPLS-LDP statistical results.
//
// PARAMETERS   : node - the node which finalizing the statistical results
//                ldp - pointer to the MplsLdpApp structure.
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
static
void MplsLdpPrintStat(Node* node, MplsLdpApp* ldp)
{
    char buf[MAX_STRING_LENGTH];

    // print Number of Label Request Message Send
    sprintf(buf, "Number of Label Request Message Send = %u",
        ldp->ldpStat.numLabelRequestMsgSend);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Label Mapping Send
    sprintf(buf, "Number of Label Mapping Send = %u",
        ldp->ldpStat.numLabelMappingSend);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Notification Message Send
    sprintf(buf, "Number of Notification Message Send = %u",
        ldp->ldpStat.numNotificationMsgSend);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Keep Alive Message Send
    sprintf(buf, "Number of Keep Alive Message Send = %u",
        ldp->ldpStat.numKeepAliveMsgSend);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Label Withdraw Message Send
    sprintf(buf, "Number of Label Withdraw Message Send = %u",
        ldp->ldpStat.numLabelWithdrawMsgSend);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        -1, // instance Id
        buf);

    // print Number of Label Release Message Send
    sprintf(buf, "Number of Label Release Message Send = %u",
        ldp->ldpStat.numLabelReleaseMsgSend);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Address Message Send
    sprintf(buf, "Number of Address Message Send = %u",
        ldp->ldpStat.numAddressMsgSend);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Initialization Message Send
    sprintf(buf, "Number of Initialization Message Send = %u",
        ldp->ldpStat.numInitializationMsgSend);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of UDP link hello Message Send
    sprintf(buf, "Number of UDP link hello Message Send = %u",
        ldp->ldpStat.numUdpLinkHelloSend);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
         ANY_DEST,
         ANY_INTERFACE, // instance Id
         buf);

    // print Number of Label Request Message Received
    sprintf(buf, "Number of Label Request Message Received = %u",
        ldp->ldpStat.numLabelRequestMsgReceived);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Label Mapping Received
    sprintf(buf, "Number of Label Mapping Received = %u",
        ldp->ldpStat.numLabelMappingReceived);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Notification Mssage Received
    sprintf(buf, "Number of Notification Message Received = %u",
        ldp->ldpStat.numNotificationMsgReceived);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Keep Alive Message Received
    sprintf(buf, "Number of Keep Alive Message Received = %u",
        ldp->ldpStat.numKeepAliveMsgReceived);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Label Withdraw Message Received
    sprintf(buf, "Number of Label Withdraw Message Received = %u",
        ldp->ldpStat.numLabelWithdrawMsgReceived);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Label Release Message Received
    sprintf(buf, "Number of Label Release Message Received = %u",
        ldp->ldpStat.numLabelReleaseMsgReceived);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Address Message Received
    sprintf(buf, "Number of Address Message Received = %u",
        ldp->ldpStat.numAddressMsgReceived);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Total Number of Initialization Message Received
    sprintf(buf, "Number of Initialization Message Received = %u",
       ldp->ldpStat.numInitializationMsgReceived);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Initialization Message Received in active mode
    sprintf(buf, "Number of Initialization Message Received"
       " in active = %u",
       ldp->ldpStat.numInitializationMsgReceivedAsActive);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of Initialization Message Received in passive mode
    sprintf(buf, "Number of Initialization Message Received"
        " in passive = %u",
        ldp->ldpStat.numInitializationMsgReceivedAsPassive);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);

    // print Number of UDP Link Hello Received Message Received
    sprintf(buf, "Number of UDP Link Hello Message Received = %u",
        ldp->ldpStat.numUdpLinkHelloReceived);

    IO_PrintStat(
        node,
        "Application",
        "MPLS-LDP",
        ANY_DEST,
        ANY_INTERFACE, // instance Id
        buf);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpFinalize()
//
// PURPOSE      : finalizing and printing MPLS-LDP statistical results.
//
// PARAMETERS   : node - the node which finalizing the statistical results
//                AppInfo - pointer to the AppInfo structure.
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsLdpFinalize(Node* node, AppInfo* appInfo)
{

    MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);

    if (MPLS_LDP_DEBUG)
    {
        MplsLdpPrintLib(node);
        MplsLdpListHelloAdjacency(node);
        MplsLdpPrintLocalAddresses(node);
        NetworkPrintForwardingTable(node);
        MplsLdpPrintIncomingFecToLabelMappingRec(node);
        MplsLdpPrintIncomingFecToLabelMappingRec(node);
        MplsLdpPrintPendingRequestList(node, ldp);
    }

    if ((ldp != NULL) && (ldp->collectStat == TRUE))
    {
        MplsLdpPrintStat(node, ldp);
    }
}

//-------------------------------------------------------------------------
// NAME         : LDPHandleInterfaceEvent()
// PURPOSE      : Handle interface event and change interface state
//                accordingly
// PARAMETERS   : node - the node on which fault has occured
//                interfaceIndex - the index of interface
//                    at which fault has occured
//                eventType - Interface Up / Down
// ASSUMPTION   : None
// RETURN VALUE : Null
//-------------------------------------------------------------------------
void LDPHandleInterfaceEvent(
    Node* node,
    int interfaceIndex,
    LDPInterfaceEvent eventType)
{

    switch (eventType)
    {
        case LDP_InterfaceUp:
        {
             MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);
            MplsLdpSendLinkHello(
                node,
                ldp->currentMessageId,
                ldp->LSR_Id,
                ldp->linkHelloHoldTime);
            break;
        }
        case LDP_InterfaceDown:
        {
           MplsLdpApp* ldp = MplsLdpAppGetStateSpace(node);
           int connectionId = - 1;
           int index = -1;
           while (TRUE)
           {
                // Need to delete entries for all nodes connected
                // directly to the faulty interface.
                unsigned int i = 0;
                MplsLdpHelloAdjacency* helloPtr = NULL;
                //NodeAddress LSR_Id = MPLS_LDP_INVALID_PEER_ADDRESS;
                NodeAddress oldNextHop = MPLS_LDP_INVALID_PEER_ADDRESS;
                connectionId = - 1;

                NodeAddress linkAddress = NetworkIpGetInterfaceAddress(node,
                                                 interfaceIndex);
                NodeAddress subnetMask = NetworkIpGetInterfaceSubnetMask(
                                                 node,
                                                 interfaceIndex);
                index = -1;
                for (i = 0; i < ldp->numHelloAdjacency; i++)
                {
                    if ((ldp->helloAdjacency[i].LinkAddress & subnetMask) ==
                          (linkAddress & subnetMask))
                             // finding the hello Adjacency neighbor by
                             // matching link address
                    {
                        index = i;
                        break ;
                    }
                 }

                // If no hello adjacency is found, then do not do any
                // processing.
                if (index == -1)
                {
                    return ;
                }
                helloPtr= (MplsLdpHelloAdjacency*)MEM_malloc(sizeof
                              (MplsLdpHelloAdjacency));
                memcpy(helloPtr,&ldp->helloAdjacency[index],
                        sizeof(MplsLdpHelloAdjacency));

                ERROR_Assert(helloPtr, "Error in MplsLdpLayer()"
                    "Hello Adjacency Not found !!!");

                connectionId = helloPtr->connectionId;
                oldNextHop =  helloPtr->LinkAddress;

                MEM_free(ldp->helloAdjacency[index].buffer);
                MEM_free(ldp->helloAdjacency[index].incomingCache);
                MEM_free(ldp->helloAdjacency[index].outboundCache);
                MEM_free(ldp->helloAdjacency[index].outboundLabelMapping);
                MEM_free(ldp->helloAdjacency[index].interfaceAddresses);

                for (i = index + 1; i < ldp->numHelloAdjacency;
                    i++)
                {
                    memcpy(&ldp->helloAdjacency[i - 1],
                        &ldp->helloAdjacency[i],
                        sizeof(MplsLdpHelloAdjacency));
                }
                ldp->numHelloAdjacency--;

                // If connection to peer is not established
                // or has already been closed, then we should not
                // try to close the connection.
                if (connectionId != INVALID_TCP_CONNECTION_ID)
                {
                    APP_TcpCloseConnection(node, connectionId);
                }

                unsigned int oldLabel = 0;
                Attribute_Type* RAttribute = NULL;
                Mpls_FEC fec;
                unsigned int label = 0;
                unsigned int hopCount = 0;
                FecToLabelMappingRec* fecToLabelMappingRec = NULL;

                // Initialize fec with any fec to match ANY fec.
                fec.ipAddress = ANY_IP;
                fec.numSignificantBits = 0;

                while (MplsLdpRetrivePreviouslySentLabelMapping(
                        ldp,
                        fec,
                        helloPtr->LSR2.LSR_Id,
                        &label,
                        &hopCount,
                        &fec,
                        FALSE))
                {
                    if (IamEgressForThisFec(node, fec, ldp))
                    {
                        ERROR_Assert(fec.ipAddress != ANY_IP,
                             "Cannot Be Egress for ANY_IP !!!");

                        MplsLdpRetrivePreviouslySentLabelMapping(
                            ldp,
                            fec,
                            helloPtr->LSR2.LSR_Id,
                            &label,
                            &hopCount,
                            NULL,
                            TRUE);

                        MplsLdpRemoveLabelForForwardingSwitchingUse(
                            node,
                            oldNextHop,   // not Used
                            label,        // in coming label to be deleated
                            INCOMING,
                            fec);         // not used.

                        MplsLdpDeleteEntriesFromLIB(ldp, label, fec);
                    } // end if (IamEgressForThisFec(node, fec))
                    else
                    {
                        MplsLdpRetrivePreviouslySentLabelMapping(
                            ldp,
                            fec,
                            helloPtr->LSR2.LSR_Id,
                            &label,
                            &hopCount,
                            NULL,
                            TRUE);
                        // For deleting library entries.
                        MplsLdpDeleteEntriesFromLIB(ldp, label, fec);

                        MplsData* mpls = NULL;

                        mpls = MplsReturnStateSpace(node);
                        MplsLdpRemoveILMEntry(mpls, oldNextHop, label,
                                              INCOMING);

                        //// Release the label.
                        //MplsLdpReceiveLabelReleaseMessage(node,ldp, label,
                        //                               fec, fec.ipAddress);
                    }

                    fec.ipAddress = ANY_IP;
                    fec.numSignificantBits = 0;
                }   // end of while.

                // Try matching ANY fec.
                fec.ipAddress = ANY_IP;
                fec.numSignificantBits = 0;

                while ((fecToLabelMappingRec =
                       MplsLdpPreviouslyReceivedAndRetainedAlabelMapping(
                           ldp,
                           fec,
                           &oldLabel,
                           helloPtr->LSR2.LSR_Id,
                           &RAttribute)) != NULL)
                {
                    NodeAddress newNextHop = MPLS_LDP_INVALID_PEER_ADDRESS;

                    ERROR_Assert(fecToLabelMappingRec,
                                 "fecToLabelMappingRec is NULL!!!");

                    memcpy(&fec,
                           &(fecToLabelMappingRec->fec),
                           sizeof(Mpls_FEC));

                    IsNextHopExistForThisDest(
                        node,
                        fec.ipAddress,
                        &newNextHop);

                    MplsLdpGetFecFromFecToLabelMappingRec(
                        ldp,
                        helloPtr->LSR2.LSR_Id,
                        &fec);

                    MplsLdpDetectChangeInFecNextHop(
                        node,
                        ldp,
                        oldLabel,
                        fec,
                        newNextHop,
                        oldNextHop,
                        helloPtr->LSR2.LSR_Id,
                        RAttribute);

                    MplsLdpDeleteEntriesFromIncomingFecToLabelMappingRec(
                        ldp,
                        fec,
                        helloPtr->LSR2.LSR_Id);

                    MplsLdpRemoveLabelForForwardingSwitchingUse(
                        node,
                        oldNextHop,  // match old next hop
                        oldLabel,    // match outgoing label
                        OUTGOING,
                        fec);        // match fec

                    // Try matching ANY fec.
                    fec.ipAddress = ANY_IP;
                    fec.numSignificantBits = 0;
                } // end while (...)

                // Delete the entries from FTN for which the
                // interface is faulty.
                {
                    int i = 0;
                    int index = 0;
                    BOOL toBeDeleted = FALSE;

                    MplsData* mpls = NULL;
                    mpls = MplsReturnStateSpace(node);
                    for (i = 0; i < mpls->numFTNEntries; i++ )
                    {
                        NodeAddress ip_Address = mpls->FTN[i].nhlfe->nextHop;
                        if ((ip_Address == helloPtr->LinkAddress))
                        {
                            toBeDeleted = TRUE;
                            index = i;
                            break;
                        }
                    }
                    if (toBeDeleted)
                    {
                        MEM_free(mpls->FTN[index].nhlfe);

                        if (index == (mpls->numFTNEntries - 1))
                        {
                            memset(&(mpls->FTN[index]), 0,
                                sizeof(Mpls_FTN_entry));
                        }
                        else
                        {
                            for (i = (index + 1); i < mpls->numFTNEntries;
                                 i++)
                            {
                                memcpy(&(mpls->FTN[i - 1]), &(mpls->FTN[i]),
                                    sizeof(Mpls_FTN_entry));
                            }
                            memset(&(mpls->FTN[(mpls->numFTNEntries) - 1]),
                                    0, sizeof(Mpls_FTN_entry));
                        }
                        (mpls->numFTNEntries)--;
                    } // end of if (toBeDeleted)
                }
                MEM_free(helloPtr) ;
                helloPtr = NULL ;
            } // end of while
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Unknown interface event!\n");
        }
    }
}

//-------------------------------------------------------------------------
//  NAME         : LDPInterfaceStatusHandler()
//  PURPOSE      : Handle mac alert.
//  PARAMETERS   : node - the node on which fault has occured
//                 interfaceIndex - the index of interface
//                 at which fault has occured
//                 state - State of fault
//  ASSUMPTION   : None
//  RETURN VALUE : None
//-------------------------------------------------------------------------

void LDPInterfaceStatusHandler(
    Node* node,
    int interfaceIndex,
    MacInterfaceState state)
{
    switch (state)
    {
        case MAC_INTERFACE_DISABLE:
        {
            Message* msg = MESSAGE_Alloc(node,
                                         APP_LAYER,
                                         APP_MPLS_LDP,
                                         MSG_APP_MplsLdpFaultMessageDOWN);

            MESSAGE_InfoAlloc(node,
                              msg,
                              sizeof(NodeAddress));

            memcpy(MESSAGE_ReturnInfo(msg),
                            &interfaceIndex,
                            sizeof(int));

            MESSAGE_Send(node, msg, 0);
            break;
        }
        case MAC_INTERFACE_ENABLE:
        {

            Message* msg = MESSAGE_Alloc(node,
                                         APP_LAYER,
                                         APP_MPLS_LDP,
                                         MSG_APP_MplsLdpFaultMessageUP);

            MESSAGE_InfoAlloc(node,
                              msg,
                              sizeof(NodeAddress));

            memcpy(MESSAGE_ReturnInfo(msg),
                            &interfaceIndex,
                            sizeof(int));
            MESSAGE_Send(node, msg, 0);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Unknown MacInterfaceState\n");
        }
    }
}
