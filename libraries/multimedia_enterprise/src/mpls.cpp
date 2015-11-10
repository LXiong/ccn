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
// PURPOSE: Simulate MPLS rfc 3031
//
//
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
// The following file have been included for integrating IP with MPLS.
#include "transport_rsvp.h"
#include "network_ip.h"
#include "mpls.h"
#include "mpls_ldp.h"
// The following 2 files have been included for integrating IP with MPLS.
#include "mac_arp.h"
#include "mac_llc.h"
#include "if_queue.h"
#include "if_scheduler.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#ifdef EXATA
#ifdef PAS_INTERFACE
#include "packet_analyzer.h"
#endif
#endif //EXATA

#define MPLS_DEBUG 0

//-------------------------------------------------------------------------
// FUNCTION     : Print_FTN_And_ILM()
//
// PURPOSE      : printing ILM and FTN tables.
//
// PARAMETERS   : node - node which is printing the ILM and FTN table.
//
// RETURN VALUE : void
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void Print_FTN_And_ILM(Node* node)
{
    int i = 0;
    MplsData* mpls = NULL;

    char operation[MAX_STRING_LENGTH][MAX_STRING_LENGTH] = {"FIRST",
                                                            "REPLACE",
                                                            "POP",
                                                            "REPL_PUSH"};

    mpls = MplsReturnStateSpace(node);

    if (!mpls)
    {
       return;
    }

    printf("--------------------- node  = %-4d ------------------------"
           "------------------\n"
           "%4s%12s%15s%11s%12s\n"
           "-----------------------------------------------------------"
           "------------------\n",
           node->nodeId, "label", "interfaceId", "nexthop", "operation",
           "*labelStack");

    for (i = 0; i < mpls->numILMEntries; i++)
    {
        char nextHopIpAddress[MAX_STRING_LENGTH] = {0};

        IO_ConvertIpAddressToString(mpls->ILM[i].nhlfe->nextHop,
                                    nextHopIpAddress);

        printf("%4u%12d%16s%11s",
               mpls->ILM[i].label,
               mpls->ILM[i].nhlfe->nextHop_OutgoingInterface,
               nextHopIpAddress,
               operation[mpls->ILM[i].nhlfe->Operation]);

        if (mpls->ILM[i].nhlfe->labelStack)
        {
            printf("%12u" ,mpls->ILM[i].nhlfe->labelStack[0]);
        }
        else
        {
            printf("%12s", "[NULL]");
        }
        printf("\n");
    }

    printf("\n%9s%16s%12s%17s%11s%12s\n"
           "-----------------------------------------------------------"
           "------------------\n",
           "Num_Bits", "IP_Address", "interfaceId", "next_hop",
           "operation", "*labelStack");

    for (i = 0; i < mpls->numFTNEntries; i++)
    {
        char nextHopIpAddress[MAX_STRING_LENGTH] = {0};
        char fecIpAddress[MAX_STRING_LENGTH] = {0};

        IO_ConvertIpAddressToString(mpls->FTN[i].fec.ipAddress,
                                    fecIpAddress);

        IO_ConvertIpAddressToString(mpls->FTN[i].nhlfe->nextHop,
                                    nextHopIpAddress);

        printf("%8u%17s%12d%17s%11s",
               mpls->FTN[i].fec.numSignificantBits,
               fecIpAddress,
               mpls->FTN[i].nhlfe->nextHop_OutgoingInterface,
               nextHopIpAddress,
               operation[mpls->FTN[i].nhlfe->Operation]);

        if (mpls->FTN[i].nhlfe->labelStack)
        {
            printf("%12u", (mpls->FTN[i].nhlfe->labelStack[0]));
        }
        else
        {
            printf("%12s", "[NULL]");
        }
        printf("\n");
    }
    printf("\n\n");
}


//-------------------------------------------------------------------------
// FUNCTION    : MplsInitStats()
//
// PURPOSE     : Initialize Statistical variables.
//
// PARAMETERS  : stats - pointer to MplsStatsType structure.
//
// RETURN TYPE : void
//
// ASSUMPTION  : None
//-------------------------------------------------------------------------
static
void MplsInitStats(MplsStatsType* stats)
{
    stats->numPacketsSentToMac = 0;
    stats->numPacketsReceivedFromMac = 0;
    stats->numPacketHandoveredToIp = 0;
    stats->numPacketDroppedAtSource = 0;
    stats->numPendingPacketsCleared = 0;
    stats->numPacketsDroppedDueToExpiredTTL = 0;
    stats->numPacketsDroppedDueToBadLabel = 0;

    // The following stats have been added for
    // integrating IP with MPLS.

    stats->numPacketsSentToMPLSatIngress = 0;
    stats->numPacketsHandoveredToIPatIngressDueToNoFTN = 0;
    stats->numPacketsHandoveredToIPatLSRdueToBadLabel = 0;
    stats->numPacketsHandoveredToIPatLSRdueToMTU = 0;
    stats->numPacketsDroppedDueToLessMTU = 0;
    stats->numPacketsReceivedFromIP = 0;
}


//-------------------------------------------------------------------------
// FUNCTION     : ReturnMplsVar()
//
// PURPOSE      : Returning the pointer to the mpls structure (if exists).
//
// PARAMETERS   : node - Pointer to the node structure; the node which is
//                       performing the above operation.
//
// RETURN VALUE : pointer to the mpls structure if initialized and exists;
//                the return value is NULL otherwise
//
// ASSUMPTION   : The function will be used within the scope of this
//                file only.
//-------------------------------------------------------------------------
static
MplsData* ReturnMplsVar(Node* node)
{
    int i = 0;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (node->macData[i])
        {
            if (node->macData[i]->mplsVar)
            {
                return (MplsData*) node->macData[i]->mplsVar;
            }
        }
    }
    return NULL;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsReturnStateSpace()
//
// PURPOSE      : Returning the pointer to the mpls structure (if exists).
//
// PARAMETERS   : node - Pointer to the node structure; the node which is
//                       performing the above operation.
//
// RETURN VALUE : pointer to the mpls structure if initialized and exists;
//                the return value is NULL otherwise
//
// ASSUMPTION   : The function will be used as an interface function to
//                other modules, outside the scope of this file.
//                E.g MPLS-LDP, RSVP-TE etc.
//-------------------------------------------------------------------------
MplsData* MplsReturnStateSpace(Node* node)
{
    return ReturnMplsVar(node);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsParseAddressString()
//
// PURPOSE      : Takes an IP address string (dotted format) -
//                returns nodeId and Ip address in "unsigned int" format.
//
// PARAMETERS   : node - node which is performing the above operation.
//                sourceString - the input IP address string in dotted
//                               fromat.
//                sourceNodeId - the returned nodeId value.
//                sourceAddr - ip address in "unsigned int" format.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
static
void MplsParseAddressString(
    Node* node,
    const char* sourceString,
    NodeAddress* sourceNodeId,
    NodeAddress* sourceAddr)
{
    BOOL isNodeId = FALSE;

    IO_ParseNodeIdOrHostAddress(sourceString, sourceNodeId, &isNodeId);

    if (!isNodeId)
    {
        *sourceAddr = *sourceNodeId;

        *sourceNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                            node,
                            *sourceAddr);

        if (*sourceNodeId == INVALID_MAPPING)
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr,
                    "MPLS static label assignment file contains a "
                    "Source address %s which is not bound to a node\n",
                    sourceString);

            ERROR_ReportError(errorStr);
        }
    }
    else
    {
        ERROR_ReportError("MPLS static label assignment file contains an"
                          " entry for a nodeIdentifier \n"
                          "instead of an IP address.\n");
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsParseSourceDestAndNextHopStrings()
//
// PURPOSE      : 1) Extracting source NodeId and source Address
//                   (in unsigned int format) from "sourceString".
//                2) Extracting destination (in unsigned int format)
//                   Address and number of subnet bits in destination
//                   address from "destString".
//                3) Extracting nodeId and IP address of next hop
//                   (in unsigned int format) from "nextHopString".
//
// PARAMETERS   : node - the node which is performing the above operations
//                inputString - pointer to the input string ("sourceString"
//                              "destString", "nextHopString" are the
//                               sub-string of inputString).
//                sourceString - source address in dotted decimal format.
//                sourceNodeId - source NodeId
//                sourceAddr - source Address (in unsigned int format)
//                destString - destination address in dotted decimal format
//                destAddr - destination Address (in unsigned int format)
//                destNumSubnetBits - number of subnet bits in destination
//                                    address
//                nextHopString - next hop address in dotted decimal format
//                nextHopNodeId - nodeId of next hop.
//                nextHopAddr - next hop address (in unsigned int format)
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
static
void MplsParseSourceDestAndNextHopStrings(
    Node* node,
    const char* inputString,
    const char* sourceString,
    NodeAddress* sourceNodeId,
    NodeAddress* sourceAddr,
    const char* destString,
    NodeAddress* destAddr,
    int* destNumSubnetBits,
    const char* nextHopString,
    NodeAddress* nextHopNodeId,
    NodeAddress* nextHopAddr)
{
    BOOL destIsNodeId = FALSE;

    if (MPLS_DEBUG)
    {
        printf("MplsParseSourceDestAndNextHopStrings(%d)\n",
               node->nodeId);
    }

    MplsParseAddressString(node, sourceString, sourceNodeId, sourceAddr);

    IO_ParseNodeIdHostOrNetworkAddress(
        destString,
        destAddr,
        destNumSubnetBits,
        &destIsNodeId);

    if (destIsNodeId == TRUE)
    {
        ERROR_ReportError("The destination entries in the MPLS static route"
                          " file should be a subnet or IP address.\n");
    }

    MplsParseAddressString(node, nextHopString, nextHopNodeId, nextHopAddr);

    if (sourceNodeId == nextHopNodeId)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr,
                "Source node must be distinct from both destination"
                " and nextHop nodes in the MPLS static route file.\n\t%s\n",
                inputString);

        ERROR_ReportError(errorStr);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsParseSourceAndNextHopStrings()
//
// PURPOSE      : 1) Extracting source NodeId and source Address
//                   (in unsigned int format) from "sourceString".
//                2) Extracting nodeId and IP address of next hop
//                   (in unsigned int format) from "nextHopString".
//
// PARAMETERS   : node - the node which is performing the above operations
//                inputString - pointer to the input string ("sourceString"
//                              and "nextHopString" are the sub-string of
//                              inputString).
//                sourceString - source address in dotted decimal format.
//                sourceNodeId - source NodeId
//                sourceAddr - source Address (in unsigned int format)
//                nextHopString - next hop address in dotted decimal format
//                nextHopNodeId - nodeId of next hop.
//                nextHopAddr - next hop address (in unsigned int format)
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
static void
MplsParseSourceAndNextHopStrings(
    Node* node,
    const char* inputString,
    const char* sourceString,
    NodeAddress* sourceNodeId,
    NodeAddress* sourceAddr,
    const char* nextHopString,
    NodeAddress* nextHopNodeId,
    NodeAddress *nextHopAddr)
{
    if (MPLS_DEBUG)
    {
        printf("MplsParseSourceAndNextHopStrings(%d)\nInput string : %s\n",
               node->nodeId, inputString);
    }

    MplsParseAddressString(node, sourceString, sourceNodeId, sourceAddr);
    MplsParseAddressString(node, nextHopString, nextHopNodeId, nextHopAddr);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsProcessSpecialLabelForILM()
//
// PURPOSE      : checks for an outgoing label, if the outgoing label is
//                Implicit_NULL_Label then adds it into NHLFE entry and
//                returns TRUE.This function is called only from MplsInit().
//
// PARAMETERS   : node - The node which is performing the above operation
//                mpls - pointer to mpls structure.
//                incomingLabel - incoming label entry
//                outgoingLabel - outgoing label entry
//                nextHopNodeId - nodeId of the next hop.Where next hop is
//                                the node such that "outgoingLabel" can be
//                                used to switch-and-forward traffic to
//                                that node.
//                nextHopAddr - IP address of the next hop.
//
// RETURN VALUE : BOOL. TRUE if the label is Implicit_NULL_Label, or
//                FALSE otherwise
//
// ASSUMPTION   : use of only following two special labels is considered:
//                a) Implicit_NULL_Label
//                b) IPv4_Explicit_NULL_Label.
//                Use of other special labels such as Router_Alert_Label
//                or Router_Alert_Label will give an error.
//-------------------------------------------------------------------------
static
BOOL MplsProcessSpecialLabelForILM(
    Node* node,
    MplsData* mpls,
    unsigned int incomingLabel,
    unsigned int outgoingLabel,
    NodeAddress nextHopNodeId,
    NodeAddress nextHopAddr)
{
    switch(mpls->labelEncoding)
    {
        case MPLS_SHIM:
        {
            if (outgoingLabel < Last_Reserved)
            {
                switch (outgoingLabel)
                {
                    case Implicit_NULL_Label:
                    {
                        Mpls_NHLFE* nhlfe = NULL;

                        nhlfe = MplsAddNHLFEEntry(
                                    mpls,
                                    NetworkGetInterfaceIndexForDestAddress(
                                        node,
                                        nextHopAddr),
                                    nextHopAddr,
                                    POP_LABEL_STACK,
                                    NULL,
                                    0); // 0 == numLabels

                        MplsAddILMEntry(mpls, incomingLabel, nhlfe);

                        return TRUE;
                        break;
                    }
                    case IPv4_Explicit_NULL_Label:
                    {
                        return FALSE;
                        break;
                    }
                    case Router_Alert_Label:
                    case IPv6_Explicit_NULL_Label:
                    {
                        ERROR_ReportError("MPLS: Router_Alert_Label and "
                                          "IPv6_Explicit_NULL_Label are "
                                          "unsupported.\n");
                        break;
                    }
                } // switch (outgoingLabel)
            } // end if (outgoingLabel < Last_Reserved)
            break;
        }
        default:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr,
                    "Unknown MPLS Label Encoding Type %d\n",
                    mpls->labelEncoding);

            ERROR_ReportError(errorStr);
            break;
        }
    } // end switch(mpls->labelEncoding)
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsUpdateInfoField()
//
// PURPOSE      : setting/updating "staus" of message. The status can be
//                "MPLS_LABEL_REQUEST_PENDING" or "MPLS_READY_TO_SEND".
//
// PARAMETERS   : node - node which is updating the message's/packet's
//                       status.
//                msg - The message/packet who's status is to be updated.
//                nextHopAddress - next hop where the message/packet is
//                                 to be forwarded
//                status - "staus" of message
//
// RETURN VALUE : void.
//
// ASSUMPTION   : status is set to MPLS_LABEL_REQUEST_PENDING if no FTN
//                entry found for the packet's FEC; in all other cases
//                status is set to MPLS_READY_TO_SEND.
//-------------------------------------------------------------------------
static
void MplsUpdateInfoField(
    Node* node,
    unsigned int outgoinginterface,
    Message* msg,
    NodeAddress nextHopAddress,
    MplsStatus status,
    MacHWAddress& hwAddr)
{

    TackedOnInfoWhileInMplsQueueType* infoPtr = NULL;
    MESSAGE_InfoAlloc(node, msg, sizeof(TackedOnInfoWhileInMplsQueueType));

    infoPtr = (TackedOnInfoWhileInMplsQueueType *) MESSAGE_ReturnInfo(msg);

    memset(infoPtr, 0, sizeof(TackedOnInfoWhileInMplsQueueType));

    // Copying next Hop Address
    infoPtr->nextHopAddress = nextHopAddress;

    // Copying mac Address byte, type, length.
    if (hwAddr.byte)
    {
        memcpy(infoPtr->macAddress,hwAddr.byte,hwAddr.hwLength);
    }
    infoPtr->hwLength = hwAddr.hwLength;
    infoPtr->hwType = hwAddr.hwType;

    infoPtr->status = status;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsAddNHLFEEntry()
//
// PURPOSE      : Adding an entry to mpls NHLFE structure.
//
// PARAMETERS   : mpls - pointer to mpls structure
//                nextHop_OutgoingInterface - outgoing interface to reach
//                                           the next hop.
//                nextHop - Where next hop is the node such that label on the
//                          top of the label stack can be used to switch and
//                          forward traffic to that node.
//                Operation - label operation to be performed on label stack
//                labelStack - pointer to the label to be put on the top
//                             of the label stack
//
// RETURN VALUE : pointer to the newly added NHLFE entry in the NHLFE table
//
// ASSUMPTION   : currently supported label stack depth is only one.
//-------------------------------------------------------------------------
Mpls_NHLFE* MplsAddNHLFEEntry(
    MplsData* mpls,
    int nextHop_OutgoingInterface,
    NodeAddress nextHop,
    Mpls_NHLFE_Operation Operation,
    unsigned int* labelStack,
    int numLabels)
{
    Mpls_NHLFE* newNHLFE = (Mpls_NHLFE*) MEM_malloc(sizeof(Mpls_NHLFE));
    ERROR_Assert(mpls, "MPLS protocol is not running");

    newNHLFE->nextHop_OutgoingInterface = nextHop_OutgoingInterface;
    newNHLFE->nextHop = nextHop;
    newNHLFE->Operation = Operation;

    if (numLabels)
    {
        newNHLFE->labelStack = (unsigned int*)
            MEM_malloc(numLabels * (sizeof(unsigned int)));

        memcpy(newNHLFE->labelStack, labelStack,
            numLabels * sizeof(unsigned int));

        newNHLFE->numLabels = numLabels;
    }
    else
    {
        newNHLFE->labelStack = NULL;
    }
    return newNHLFE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsAddFTNEntry()
//
// PURPOSE      : Adding an entry to FTN table.
//
// PARAMETERS   : mpls - pointer to mpls structure
//                fec - FEC to be added in the FTN table.
//                nhlfe - corresponding nhlfe entry to be added.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void MplsAddFTNEntry(
    MplsData* mpls,
    Mpls_FEC fec,
    Mpls_NHLFE* nhlfe)
{

    // it is confirmed that a new row has to be made.
    if (mpls->numFTNEntries == mpls->maxFTNEntries)
    {
        // Need to add more space for FTN Entries
        Mpls_FTN_entry* newFTN = (Mpls_FTN_entry*)
            MEM_malloc(sizeof(Mpls_FTN_entry) *
                (mpls->maxFTNEntries + DEFAULT_MPLS_TABLE_SIZE));

        memcpy(newFTN,
               mpls->FTN,
               sizeof(Mpls_FTN_entry) * mpls->maxFTNEntries);

        MEM_free(mpls->FTN);
        mpls->FTN = newFTN;
        mpls->maxFTNEntries += DEFAULT_MPLS_TABLE_SIZE;
    }

    // Added for priority support in FEC classification in static MPLS.
    // Now check the position at which the FTN entry has to be inserted
    // this will be done by finding the longest prefix match by comparing
    // the fec.ipaddress, fec.numsignificantBits and fec.priority.

    int i = mpls->numFTNEntries;

    while (i > 0 && ((fec.ipAddress > mpls->FTN[i - 1].fec.ipAddress)
           || ((fec.ipAddress == mpls->FTN[i - 1].fec.ipAddress)
           && (fec.numSignificantBits >
                   mpls->FTN[i - 1].fec.numSignificantBits)
           && (fec.priority > mpls->FTN[i - 1].fec.priority))))
    {
        mpls->FTN[i] = mpls->FTN[i - 1];
        i--;
    }

    mpls->FTN[i].fec = fec;
    mpls->FTN[i].nhlfe = nhlfe;

    mpls->numFTNEntries++;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsMatchFTN()
//
// PURPOSE      : finding an matching FTN-entry from FTN table against
//                a given Fec->ipAddrss.
//
// PARAMETERS   : mpls - pointer to mpls structure
//                ipAddress - ipAddress (i.e. Fec->ipAddress) to be searched
//                            in the FTN table.
//
// RETURN VALUE : pointer to the matching entry in the FTN table (if found)
//                or NULL otherwise.
//
// ASSUMPTIONS  : this functions finds an exact match.It do not considers
//                something like "nearest" match.
//-------------------------------------------------------------------------

// Changed for adding priority support for FEC classification.
static
Mpls_FTN_entry* MplsMatchFTN(
                Node* node, NodeAddress ipAddress, unsigned int priority)
{
    MplsData* mpls = NULL;
    mpls = MplsReturnStateSpace(node);

    int currentIndex = -1;
    int currentNumBits = -1;
    int candidateIndex = 0;

    for (candidateIndex = 0; candidateIndex < mpls->numFTNEntries;
         candidateIndex++)
    {
        NodeAddress maskedIpAddress =
            MaskIpAddress(
                ipAddress,
                ConvertNumHostBitsToSubnetMask(
                    (sizeof(NodeAddress) * 8)
                     - mpls->FTN[candidateIndex].fec.numSignificantBits));

        if (MPLS_DEBUG)
        {
            printf("MatchFTN numSigBits = %u ANY_IP = %u\n"
                   "MatchFTN ip %u masked %u fec %u\n",
                   mpls->FTN[candidateIndex].fec.numSignificantBits,
                   ANY_IP,
                   ipAddress,
                   maskedIpAddress,
                   mpls->FTN[candidateIndex].fec.ipAddress);
        }

        // Changed for adding priority support for FEC classification.

        if (MplsLdpAppGetStateSpace(node))
        {
            // We are here it means Label Distribution Style
            // is Dynamic
            if (maskedIpAddress == mpls->FTN[candidateIndex].fec.ipAddress)
            {
                if (mpls->FTN[candidateIndex].fec.numSignificantBits >
                       currentNumBits)
                {
                    currentIndex = candidateIndex;
                    currentNumBits =
                        mpls->FTN[candidateIndex].fec.numSignificantBits;
                }
            }
        }
        else
        {
            // We are here it means MPLS is statically configured.
            // Changed for adding priority support for FEC classification.

            if ((maskedIpAddress == mpls->FTN[candidateIndex].fec.ipAddress)
                && (priority == mpls->FTN[candidateIndex].fec.priority))
            {
                if (mpls->FTN[candidateIndex].fec.numSignificantBits >
                       currentNumBits)
                {
                    currentIndex = candidateIndex;
                    currentNumBits =
                        mpls->FTN[candidateIndex].fec.numSignificantBits;
                }
            }
        }
    }

    if (currentIndex > -1)
    {
       // The following code has been added for IP + MPLS integration.
       // If the FTN entry has Implicit NULL label then
       // it means that node is egress node.
       //if (mpls->FTN[currentIndex].nhlfe->Operation == Implicit_NULL_Label)
       //{
       //    return NULL ;
       //}
        return &(mpls->FTN[currentIndex]);
    }
    else
    {
        return NULL;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsAddILMEntry()
//
// PURPOSE      : adding an entry to mpls ILM table.
//
// PARAMETERS   : mpls - pointer to mpls structure.
//                incomingLabel - label to be inserted in the ILM table
//                nhlfe - corresponding nhlfe entry to be added
//
// RETURN VALUE : void
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsAddILMEntry(
    MplsData* mpls,
    unsigned int incomingLabel,
    Mpls_NHLFE* nhlfe)
{
    if (mpls->numILMEntries == mpls->maxILMEntries)
    {
        // Need to add more space for ILM Entries
        Mpls_ILM_entry* newILM = (Mpls_ILM_entry*)
            MEM_malloc(sizeof(Mpls_ILM_entry) *
                (mpls->maxILMEntries + DEFAULT_MPLS_TABLE_SIZE));

        memcpy(newILM,
               mpls->ILM,
               sizeof(Mpls_ILM_entry) * mpls->maxILMEntries);

        MEM_free(mpls->ILM);
        mpls->ILM = newILM;
        mpls->maxILMEntries += DEFAULT_MPLS_TABLE_SIZE;
    }

    mpls->ILM[mpls->numILMEntries].label = incomingLabel;
    mpls->ILM[mpls->numILMEntries].nhlfe = nhlfe;
    mpls->numILMEntries++;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsMatchILM()
//
// PURPOSE      : finding an ILM entry in the ILM table against a given
//                label.
//
// PARAMETERS   : mpls - pointer to mpls structure.
//                incomingLabel - label to be searched.
//
// RETURN VALUE : pointer to the ILM entry in the ILM table if found, or
//                a NULL pointer otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
static
Mpls_ILM_entry* MplsMatchILM(MplsData* mpls, unsigned int incomingLabel)
{
    int i = 0;

    for (i = 0; i < mpls->numILMEntries; i++)
    {
        if (mpls->ILM[i].label == incomingLabel)
        {
            return &(mpls->ILM[i]);
        }
    }
    return NULL;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsAddLabel()
//
// PURPOSE      : Adding a label on the top of a packet
//
// PARAMETERS   : node - which is processing the packet
//                mpls - pointer to mpls structure.
//                msg - message/packet
//                labelStack - pointer to label stack in the NHLFE entry.
//                numLabels - number of labels to be pushed in the label
//                            stack.
//                TTL - TTL of the packet.
//
// ASSUMPTION   : currently supported label stack depth is only one.
//-------------------------------------------------------------------------
static
void MplsAddLabel(
    Node* node,
    MplsData* mpls,
    Message* msg,
    unsigned int* labelStack,
    int numLabels,
    unsigned int TTL)
{
    ERROR_Assert(labelStack, "Label stack is NULL!!!");

    switch (mpls->labelEncoding)
    {
        case MPLS_SHIM:
        {
            int i = 0;

            for (i = numLabels - 1; i >= 0; i--)
            {
                MplsShimAddLabel(
                    node,
                    msg,
                    labelStack[i],
                    (i == (numLabels - 1)), // is bottom Of Stack?
                    TTL);
            }
            break;
        }
        default:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr,
                    "Unknown MPLS Label Encoding Type %d\n",
                    mpls->labelEncoding);

            ERROR_ReportError(errorStr);
            break;
        }
    } // end switch(mpls->labelEncoding)
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsReplaceLabel()
//
// PURPOSE      : Replacing the label of an incoming "labeled packet" with
//                a new label
//
// PARAMETERS   : node - which is processing the packet
//                mpls - pointer to mpls structure.
//                msg - message/packet
//                labelStack - pointer to label stack in NHLFE entry.
//                numLabels - number of labels to be pushed in the label
//                            stack.
//                TTL - TTL of the packet.
//
// RETURN VALUE : void
//
// ASSUMPTION   : currently supported label stack depth is only one.
//-------------------------------------------------------------------------
static
void MplsReplaceLabel(
    Node* node,
    MplsData* mpls,
    Message* msg,
    unsigned int* labelStack,
    int numLabels,
    unsigned int TTL)
{
    ERROR_Assert(labelStack, "Label stack is NULL!!!");

    switch (mpls->labelEncoding)
    {
        case MPLS_SHIM:
        {
            int i = 0;
            Mpls_Shim_LabelStackEntry shim_label;
            BOOL bottomOfStack = FALSE;

            ERROR_Assert((numLabels > 0),
                         "Number of label is ReplaceLabel operation"
                         " is less than or equals to zero !!!");

            MplsShimReturnTopLabel(msg, &shim_label);

            bottomOfStack = MplsShimBottomOfStack(&shim_label);

            MplsShimReplaceLabel(node,
                                 msg,
                                 labelStack[numLabels - 1],
                                 bottomOfStack,
                                 TTL);

            for (i = numLabels - 2; i >= 0; i--)
            {
                MplsShimAddLabel(
                    node,
                    msg,
                    labelStack[i],
                    FALSE,   //is bottom Of Stack?
                    TTL);
            }

            break;
        }
        default:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "Unknown MPLS Label Encoding Type %d\n",
                    mpls->labelEncoding);

            ERROR_ReportError(errorStr);
            break;
        }
    } // end switch switch (mpls->labelEncoding)
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsPopLabelStack()
//
// PURPOSE      : poping a label from label stack on the top of the message.
//
// PARAMETERS   : node - which is processing the packet
//                mpls - pointer to mpls structure.
//                msg - message/packet
//                lastHopAddress - previous hop.
//                TTL - TTL of the packet.
//                interfaceIndex - interface index through which packet is
//                                 received
//
// RETURN VALUE : BOOL. returns TRUE if bottom of the label stack is found
//
// ASSUMPTION   : currently supported label stack depth is only one.
//-------------------------------------------------------------------------
static
BOOL MplsPopLabelStack(
    Node* node,
    MplsData* mpls,
    Message* msg,
    NodeAddress lastHopAddress,
    int TTL, // Changed during IP-MPLS integration
    int interfaceIndex)
{
    ERROR_Assert(mpls, "MPLS structure is NULL !!!");

    switch(mpls->labelEncoding)
    {
        case MPLS_SHIM:
        {
            BOOL bottomOfStack = FALSE;
            Mpls_Shim_LabelStackEntry shim_label;

            MplsShimPopTopLabel(node, msg, &bottomOfStack, &shim_label);

            if (bottomOfStack)
            {
                // This code has been added as part of IP-MPLS
                // Replace TTL value in ipheader with the current
                // TTL at Egress
                IpHeaderType *ipHeader = (IpHeaderType*) msg->packet;
                ipHeader->ip_ttl = (unsigned char)TTL;
                NetworkIpReceivePacketFromMacLayer(node,
                                                   msg,
                                                   lastHopAddress,
                                                   interfaceIndex);

                mpls->stats->numPacketHandoveredToIp++;
                return TRUE;
            }
            break;
        }
        default:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr,
                    "Unknown MPLS Label Encoding Type %d\n",
                    mpls->labelEncoding);

            ERROR_ReportError(errorStr);
            break;
        }
    } // end switch
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLabelOperation()
//
// PURPOSE      : performing a given label operation on the label stack.
//
// PARAMETERS   : node - which is processing the packet
//                mpls - pointer to mpls structure.
//                msg - message/packet
//                labelStack - pointer to the labelStack in the NHLFE.
//                numLabels - number of labels
//                lastHopAddress - previous hop.
//                TTL - TTL of the packet.
//                interfaceIndex - interface index through which packet is
//                                 received
//
// RETURN VALUE : BOOL. returns TRUE if pop label stack operation is
//                executed and bottom of the label stack is found
//
// ASSUMPTION   : currently supported label stack depth is only one.
//-------------------------------------------------------------------------
static
BOOL MplsLabelOperation(
    Node* node,
    MplsData* mpls,
    Message* msg,
    unsigned int* labelStack,
    int numLabels,
    NodeAddress lastHopAddress,
    unsigned int TTL,
    Mpls_NHLFE_Operation operation,
    int interfaceIndex)
{
    switch (operation)
    {
        case FIRST_LABEL:
        {
            MplsAddLabel(node, mpls, msg, labelStack, numLabels, TTL);
            return FALSE;
        }
        case REPLACE_LABEL:
        {
            MplsReplaceLabel(node, mpls, msg, labelStack, numLabels, TTL);
            return FALSE;
        }
        case POP_LABEL_STACK:
        {
            return MplsPopLabelStack(
                       node,
                       mpls,
                       msg,
                       lastHopAddress,
                       TTL,
                       interfaceIndex);
        }
        case REPLACE_LABEL_AND_PUSH:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "#%d: NHLFE Operation %d is not yet "
                    "supported\n", node->nodeId, operation);

            ERROR_ReportError(errorStr);
        }
        default:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "Unknown NHLFE Operation %d\n",
                    operation);

            ERROR_ReportError(errorStr);
        }
    } // end switch
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsProcessPacketWithNHLFE()
//
// PURPOSE      : processing a incoming packet and performing various
//                label operation on it. Calls MplsLabelOperation(). If TTL
//                expires drops the packet.
//
// PARAMETERS   : node - which is processing the packet
//                mpls - pointer to mpls structure.
//                msg - message/packet
//                nhlfe - pointer to the NHLFE structure in ILM/FTN.
//                lastHopAddress - previous hop.
//                TTL - TTL of the packet.
//                interfaceIndex - interface index through which packet is
//                                 received
// RETURN VALUE : as returned be MplsLabelOperation()
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
static
BOOL MplsProcessPacketWithNHLFE(
    Node* node,
    MplsData* mpls,
    Message* msg,
    Mpls_NHLFE* nhlfe,
    NodeAddress lastHopAddress,
    int TTL, // Changed during IP-MPLS integration
    //unsigned int TTL,
    int interfaceIndex)
{
    ERROR_Assert(nhlfe, "NHLFE structure is NULL !!!");

    if (TTL <= 0)
    {
        // If TTL is less than 0, then packet will be dropped,
        // or will be sent to IP based on the user specified option.
        if (mpls->routeToIPOnError)
        {
           MplsPopLabelStack( node,
                              mpls,
                              msg,
                              lastHopAddress,
                              TTL,
                              interfaceIndex);
            return TRUE;

        }
        else
        {
            // The message should be dropped due to expired ttl
            mpls->stats->numPacketsDroppedDueToExpiredTTL++;
            MESSAGE_Free(node, msg);
            return TRUE;
        }
    }

    return MplsLabelOperation(node, mpls, msg, nhlfe->labelStack,
                       nhlfe->numLabels, lastHopAddress, TTL,
                       nhlfe->Operation, interfaceIndex);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsAddBroadcastLabel()
//
// PURPOSE      : adding a broadcast label on the top of a message
//
// PARAMETERS   : node - which is processing the packet
//                mpls - pointer to mpls structure.
//                msg - message/packet
//                TTL - TTL of the packet.
//
// RETURN VALUE : none.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
static
void MplsAddBroadcastLabel(
    Node* node,
    MplsData* mpls,
    Message* msg,
    unsigned int TTL)
{
    switch (mpls->labelEncoding)
    {
        case MPLS_SHIM:
        {
            MplsShimAddLabel(node, msg, IPv4_Explicit_NULL_Label,
                             TRUE, // bottomOfStack?
                             TTL); // the TTL value
            break;
        }
        default:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "Unknown MPLS Label Encoding Type %d\n",
                    mpls->labelEncoding);

            ERROR_ReportError(errorStr);
            break;
        }
    } // end switch
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsRemoveILMEntry()
//
// PURPOSE      : Removing an ILM entry from ILM table. and returning the
//                corresponding outgoing label.
//
// PARAMETERS   : mpls - pointer to mpls structure.
//                label - incoming label to be deleted
//                outLabel - corresponding outgoing label
// RETURN VALUE : none.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsRemoveILMEntry(
    MplsData* mpls,
    unsigned int label,
    unsigned int* outLabel)
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

        if (mpls->ILM[i].label == label)
        {
           toBeDeleted = TRUE;
           index = i;
           *outLabel = out_label;
           break;
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
// FUNCTION     : MplsRemoveFTNEntry()
//
// PURPOSE      : Removing and FTN entry from FTN table. Given
//                Fec->ipAddress and corresponding outgoing Label
//
// PARAMETERS   : mpls - pointer to mpls structure.
//                ipAddress - ipAddress to be matched
//                outLabel - outgoing label to be matched
//
// RETURN VALUE : void
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsRemoveFTNEntry(
    MplsData* mpls,
    NodeAddress ipAddress,
    unsigned int outLabel)
{
    int i = 0;
    int index = 0;
    BOOL toBeDeleted = FALSE;

    for (i = 0; i < mpls->numFTNEntries; i++ )
    {
        NodeAddress ip_Address = mpls->FTN[i].fec.ipAddress;
        unsigned int label = 0;

        if (mpls->FTN[i].nhlfe->labelStack)
        {
            label = mpls->FTN[i].nhlfe->labelStack[0];
        }

        if ((ip_Address == ipAddress) && (label == outLabel))
        {
            toBeDeleted = TRUE;
            index = i;
            break;
        }
    }

    if (toBeDeleted)
    {

        MEM_free(mpls->FTN[index].nhlfe->labelStack);
        MEM_free(mpls->FTN[index].nhlfe);

        if (index == (mpls->numFTNEntries - 1))
        {
            memset(&(mpls->FTN[index]), 0, sizeof(Mpls_FTN_entry));
        }
        else
        {
            for (i = (index + 1); i < mpls->numFTNEntries; i++)
            {
                memcpy(&(mpls->FTN[i - 1]), &(mpls->FTN[i]),
                    sizeof(Mpls_FTN_entry));
            }
            memset(&(mpls->FTN[(mpls->numFTNEntries) - 1]), 0,
                sizeof(Mpls_FTN_entry));
        }
        (mpls->numFTNEntries)--;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : SameNHLFE()
//
// PURPOSE      : checks whether two NHLFEs are same or not.
//
// PARAMETERS   : nhlfe1 - first NHLFE,
//                nhlfe2 - second NHLFE,
//
// RETURN VALUE : BOOL, returns TRUE if NHLFEs are same.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
static
BOOL SameNHLFE(Mpls_NHLFE* nhlfe1, Mpls_NHLFE* nhlfe2)
{
    int found = FALSE;

    if (!(nhlfe1 && nhlfe2))
    {
       // if one of them is NULL then found = FALSE.
       return found;
    }
    else if ((nhlfe1->nextHop_OutgoingInterface ==
                nhlfe2->nextHop_OutgoingInterface) &&
                   (nhlfe1->nextHop == nhlfe2->nextHop) &&
                      (nhlfe1->Operation == nhlfe2->Operation) &&
                         (nhlfe1->numLabels == nhlfe2->numLabels))
    {
        if ((nhlfe1->numLabels == 0) && (nhlfe2->numLabels == 0))
        {
            found = TRUE;
        }
        else
        {
            if (!memcmp(nhlfe1->labelStack, nhlfe2->labelStack,
                   sizeof(unsigned int) * nhlfe1->numLabels))
            {
                found = TRUE;
            }
        }
    }
    return found;
}


//-------------------------------------------------------------------------
// FUNCTION     : Mpls_Match_ILM_For_This_Label()
//
// PURPOSE      : matching an outgoing label such that corresponding
//                in coming label is  Implicit_NULL_Label.
//
// PARAMETERS   : mpls - pointer to mpls structure.
//                label - outgoing label to be matched.
//                nextHopAddress - next hop address to be matched
//
// RETURN VALUE : pointer to the matching entry in the ILM table
//                (if found) or NULL otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
static
Mpls_ILM_entry* Mpls_Match_ILM_For_This_Label(
    MplsData* mpls,
    NodeAddress label,
    unsigned int nextHopAddr)
{
    int i = 0;
    int found = FALSE;
    unsigned int outLabel;
    NodeAddress nextHop;

    for (i = 0; i < mpls->numILMEntries; i++)
    {
        if (!(mpls->ILM[i].nhlfe->labelStack))
        {
            continue;
        }
        outLabel = (mpls->ILM[i].nhlfe->labelStack[0]);
        nextHop = mpls->ILM[i].nhlfe->nextHop;

        if ((outLabel == label) &&
            (mpls->ILM[i].label == Implicit_NULL_Label) &&
            (nextHop == nextHopAddr))
        {
             found = TRUE;
             break;
        }
    }

    if (found)
    {
        return &(mpls->ILM[i]);
    }
    else
    {
        return NULL;
    }
}

//
// MPLS Label Assignment API Functions. These functions will be used
// by label dietribution protocols like RSVP-TE MPLS-LDP etc.
//

//-------------------------------------------------------------------------
// FUNCTION     : MplsAssignFECtoOutgoingLabelAndInterface()
//
// PURPOSE      : creating an FTN entry, and also to change status of all
//                the pending packets in the queue from
//                MPLS_LABEL_REQUEST_PENDING, to MPLS_READY_TO_SEND.This
//                function will be used by an egress router.
//
// PARAMETERS   : node - the node which is performing the above operation
//                ipAddress - ipaddress (Ftn->ipAddress) to be added.
//                numSignificantBits - number of significant bits in IP
//                                     address.
//                nextHopAddr - next hop address to reach the destination
//                              (ipAddress is actually the destination
//                               address)
//                operation - Operation to be performed on the nhlfe entry.
//                labelStack - pointer to the label-stack to be
//                             added in FTN entry
//                numLabels - number of Labels to pushed in the label stack
//
// RETURN VALUE : none.
//
// ASSUMPTION   : label stack depth is 1.
//-------------------------------------------------------------------------
void MplsAssignFECtoOutgoingLabelAndInterface(
    Node* node,
    NodeAddress ipAddress,
    int numSignificantBits,
    NodeAddress nextHopAddr,
    Mpls_NHLFE_Operation Operation,
    unsigned int* labelStack,
    int numLabels)
{
    Mpls_FEC fec;
    Mpls_NHLFE* nhlfe = NULL;
    MplsData* mpls = ReturnMplsVar(node);
    Mpls_FTN_entry* ftn = NULL;

    ERROR_Assert(mpls, "MPLS structure is NULL!!!");

    if (MPLS_DEBUG)
    {
        printf("#%d: MplsAssignFECtoOutgoingLabelAndInterface()\n"
               "\tipAddress = %u / numSigBits = %u / nextHopAddr"
               " = %u Op = %u\n"
               "\nnumLabels = %u\tlabel = %u",
               node->nodeId,
               ipAddress,
               numSignificantBits,
               nextHopAddr,
               Operation,
               numLabels,
               *labelStack);
    }

    // Changed for adding priority support for FEC classification.
    // Here the priority (3rd param) has been passed as 0 because this
    // function (MplsMatchFTN) is  always directly or indirectly called from
    // mpls_ldp and in ldp, the priority is always zero.

    ftn = MplsMatchFTN(node, ipAddress, 0);
    if (ftn == NULL)
    {
         nhlfe = MplsAddNHLFEEntry(
                     mpls,
                     NetworkGetInterfaceIndexForDestAddress(
                         node,
                         nextHopAddr),
                     nextHopAddr,
                     Operation,
                     labelStack,
                     numLabels);

         fec.ipAddress = ipAddress;
         fec.numSignificantBits = numSignificantBits;

         // Changed for adding priority support for FEC classification.
         fec.priority = 0 ;

         MplsAddFTNEntry(mpls, fec, nhlfe);
    }
    else
    {
        int num_Labels = ftn->nhlfe->numLabels;

        if (!memcmp(labelStack, ftn->nhlfe->labelStack,
               sizeof(NodeAddress) * num_Labels))
        {
              return;
        }
        else
        {
             //deallocate previous nhlfe entry
             MEM_free(ftn->nhlfe->labelStack);
             MEM_free(ftn->nhlfe);

             //create new nhlfe entry;
             nhlfe = MplsAddNHLFEEntry(
                         mpls,
                         NetworkGetInterfaceIndexForDestAddress(
                             node,
                             nextHopAddr),
                         nextHopAddr,
                         Operation,
                         labelStack,
                         numLabels);

             ftn->nhlfe = nhlfe;
        }
    }

    if ((ipAddress != nextHopAddr) &&
        ((*labelStack != 0)|| (*labelStack != 3)))
    {
        SetAllPendingReadyToSend(
            node,
            NetworkGetInterfaceIndexForDestAddress(node, nextHopAddr),
            ipAddress,
            mpls,
            nhlfe);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsSetNewFECCallbackFunction()
//
// PURPOSE      : seting a callback function to invoke label distibution
//                procedure
//
// PARAMETERS   : node - node which is setting call back function.
//                NewFECCallbackFunctionPtr - pointer to callback function
//
// RETURN VALUE : none.
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void MplsSetNewFECCallbackFunction(
    Node* node,
    MplsNewFECCallbackFunctionType NewFECCallbackFunctionPtr)
{
    MplsData* mpls = ReturnMplsVar(node);

    ERROR_Assert(mpls, "MPLS structure is NULL!!!");

    ERROR_Assert(mpls->ReportNewFECCallback == NULL,
                 "Label distribution callback function is NULL!!!");

    mpls->ReportNewFECCallback = NewFECCallbackFunctionPtr;
}


//-------------------------------------------------------------------------
// FUNCTION      : Get_Out_Label()
//
// PURPOSE       : searching and retriving a output label from FTN entry.
//                 Given : fec and next hop address for that fec.
//
// PARAMETERS    : mpls - pointer to mpls structure.
//                 fec - the fec to be matched.
//                 outLabel - the outgoing label retrived.
//                 nextHopAddr - the next hop address to be matched.
//
// RETURN VALUE  : BOOL, returns TRUE if matching found, FALSE otherwise.
//
// ASSUMPTIONS   : none
//-------------------------------------------------------------------------
static
BOOL Get_Out_Label(
    MplsData* mpls,
    Mpls_FEC fec,
    unsigned int* outLabel,
    unsigned int nextHopAddr)
{
    int i = 0;
    BOOL found = FALSE;
    unsigned int label;
    unsigned int nextHop;

    for (i = 0; i < mpls->numFTNEntries; i++)
    {
        label = mpls->FTN[i].nhlfe->labelStack[0];
        nextHop = mpls->FTN[i].nhlfe->nextHop;

        if (!memcmp(&(mpls->FTN[i].fec), &fec, sizeof(Mpls_FEC)) &&
               (nextHopAddr == nextHop))
        {
            found = TRUE;
            *outLabel = label;
            break;
        }
    }
    return (found);
}


//-------------------------------------------------------------------------
// FUNCTION     : DuplicateEntryInILM()
//
// PURPOSE      : To check for a duplicate entry in ILM.
//
// PARAMETERS   : mpls - pointer to mpls structure.
//                inLabel - incoming label to be searched for
//                nhlfe - NHLFE entry to be matched
//
// RETURN VALUE : BOOL, returns TRUE if search is successsfull , FALSE
//                otherwise.
//
// ASSUMPTIONS  : none.
//-------------------------------------------------------------------------
static
BOOL DuplicateEntryInILM(
    MplsData* mpls,
    unsigned int inLabel,
    Mpls_NHLFE* nhlfe)
{
    int i = 0;
    BOOL found = FALSE;

    for (i = 0; i < mpls->numILMEntries; i++)
    {
        if ((mpls->ILM[i].label == inLabel) &&
            (SameNHLFE(mpls->ILM[i].nhlfe, nhlfe)))
        {
            found = TRUE;
            break;
        }
    }
    return found;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsCheckAndAssignLabelToILM()
//
// PURPOSE      : To install a label for forwarding and switching use.
//                This will be used for control mode ORDERED only.
//
// PARAMETERS   : node - node which is installing label.
//                ipAddress - Fec->ipAddress
//                numSignificantBits - number of significant bits in
//                                     Fec->ipAddress
//                nextHopAddr - next hop address to reach the destination
//                              (ipAddress is actually the destination
//                               address)
//                operation - Operation to be performed on the nhlfe entry.
//                incomingLabel - pointer to incoming label
//                outgoingLabel - pointer to outgoing label stack.
//                putInLabel - set this parameter TRUE if incoming label
//                             to put.
//                outgoingLabel - set this parameter TRUE if outgoing
//                                label to put.
//                numLabels - number of labels
//
// RETURN VALUE : none.
//
// ASSUMPTIONS  : This will be used for control mode ORDERED only.
//-------------------------------------------------------------------------
void MplsCheckAndAssignLabelToILM(
    Node* node,
    NodeAddress ipAddress,
    int numSignificantBits,
    NodeAddress nextHopAddr,
    Mpls_NHLFE_Operation Operation,
    unsigned int* incomingLabel,
    unsigned int* outgoingLabel,
    BOOL putInLabel,
    BOOL putOutLabel,
    int numLabels)
{
    Mpls_FEC fec;
    Mpls_NHLFE* nhlfe = NULL;
    MplsData* mpls = ReturnMplsVar(node);

    if (putInLabel && putOutLabel)
    {
        nhlfe = MplsAddNHLFEEntry(
                    mpls,
                    NetworkGetInterfaceIndexForDestAddress(
                        node,
                        nextHopAddr),
                    nextHopAddr,
                    Operation,
                    outgoingLabel,
                    numLabels);

        fec.ipAddress = ipAddress;
        fec.numSignificantBits = numSignificantBits;

        // Changed for adding priority support for FEC classification.
        // For LDP, the priority value is zero.
        fec.priority = 0 ;

        if (MPLS_DEBUG)
        {
            printf("#%u Assign in if (putInLabel && putOutLabel)\n"
                   "outLabel = %u and  inlabel = %u\n",
                   node->nodeId,
                   *outgoingLabel,
                   *incomingLabel);
        }

        if (!DuplicateEntryInILM(mpls, *incomingLabel, nhlfe))
        {
            MplsAddILMEntry(mpls, *incomingLabel, nhlfe);
        }
        else
        {
            MEM_free(nhlfe->labelStack);
            MEM_free(nhlfe);
        }
    }
    else if (putOutLabel)
    {
        unsigned int inLabel;

        nhlfe = MplsAddNHLFEEntry(
                    mpls,
                    NetworkGetInterfaceIndexForDestAddress(
                        node,
                        nextHopAddr),
                    nextHopAddr,
                    Operation,
                    outgoingLabel,
                    numLabels);

        inLabel = Implicit_NULL_Label;

        if (!DuplicateEntryInILM(mpls, inLabel, nhlfe))
        {
            if (MPLS_DEBUG)
            {
                printf("#%d Assign in if (putOutLabel)\n"
                       "outLabel = %d and  inlabel = %d\n",
                       node->nodeId,
                       *outgoingLabel,
                       inLabel);
            }
            MplsAddILMEntry(mpls, inLabel, nhlfe);
        }
        else
        {
            MEM_free(nhlfe->labelStack);
            MEM_free(nhlfe);
        }
    }
    else if (putInLabel)
    {
        Mpls_ILM_entry* ilm = NULL;
        MplsLdpApp* ldp = NULL;
        unsigned int outLabel = 0;
        Mpls_FEC fec;
        BOOL ftn_entry_found = FALSE;

        // build fec for search in LIB
        fec.ipAddress = ipAddress;
        fec.numSignificantBits = numSignificantBits;

        // Changed for adding priority support for FEC classification.
        // For LDP, the priority value is zero.
        fec.priority = 0 ;

        ldp = MplsLdpAppGetStateSpace(node);
        ERROR_Assert(ldp, "MPLS-LDP structure is NULL!!!");

        ftn_entry_found = Get_Out_Label(mpls,
                                        fec,
                                        &outLabel,
                                        nextHopAddr);
        ERROR_Assert(ftn_entry_found, "FTN Entry not found !!!");

        if (MPLS_DEBUG)
        {
            printf("#%d Assign in if (putInLabel if-part)\n"
                   "outLabel = %d and  inlabel = %d\n",
                   node->nodeId,
                   outLabel,
                   *incomingLabel);
        }

        ilm = Mpls_Match_ILM_For_This_Label(mpls,outLabel,nextHopAddr);
        if (ilm)
        {
            ERROR_Assert(ilm->label == Implicit_NULL_Label,
                         "Incoming Label is not Implicit_NULL_Label");

            memcpy(&ilm->label, incomingLabel, sizeof(unsigned int));
        }
        else
        {
            unsigned int inLabel;

            nhlfe = MplsAddNHLFEEntry(
                        mpls,
                        NetworkGetInterfaceIndexForDestAddress(
                            node,
                            nextHopAddr),
                        nextHopAddr,
                        Operation,
                        &outLabel,
                        numLabels);

            inLabel = *incomingLabel;
            MplsAddILMEntry(mpls, inLabel, nhlfe);

            if (MPLS_DEBUG)
            {
                printf("#%d Assign in if (putInLabel else-part)\n"
                       "outLabel = %d and  inlabel = %d\n",
                       node->nodeId,
                       *outgoingLabel,
                       *incomingLabel);
            }
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : AssignLabelForControlModeIndependent()
//
// PURPOSE      : To install a label for forwarding and switching use.
//                This will be used for control mode INDEPEDENT only.
//
// PARAMETERS   : node - node which is installing label.
//                ipAddress - Fec->ipAddress
//                numSignificantBits - number of significant bits in
//                                     Fec->ipAddress
//                nextHopAddr - next hop address to reach the destination
//                              (ipAddress is actually the destination
//                               address)
//                operation - Operation to be performed on the nhlfe entry.
//                incomingLabel - pointer to incoming label
//                outgoingLabel - pointer to outgoing label stack.
//                putInLabel - set this parameter TRUE if incoming label
//                             to put.
//                outgoingLabel - set this parameter TRUE if outgoing
//                                label to put.
//                numLabels - number of labels
//
// RETURN VALUE : none.
//
// ASSUMPTIONS  : This will be used for control mode INDEPEDENT only.
//-------------------------------------------------------------------------
void AssignLabelForControlModeIndependent(
    Node* node,
    NodeAddress ipAddress,
    int numSignificantBits,
    NodeAddress nextHopAddr,
    Mpls_NHLFE_Operation Operation,
    unsigned int* inLabel,
    unsigned int* outLabel,
    BOOL putInLabel,
    BOOL putOutLabel,
    int numLabels)

{
    MplsLdpApp* ldp = NULL;
    LIB* lib = NULL;
    unsigned int in_label = 0;
    Mpls_ILM_entry* ilm = NULL;
    Mpls_NHLFE* nhlfe = NULL;
    Mpls_FEC fec;
    MplsData* mpls = NULL;

    fec.ipAddress = ipAddress;
    fec.numSignificantBits = numSignificantBits;

    // Changed for adding priority support for FEC classification.
    // For LDP, the priority value is zero.
    fec.priority = 0 ;

    mpls = ReturnMplsVar(node);
    ldp = MplsLdpAppGetStateSpace(node);

    ERROR_Assert(mpls, "MPLS structure is NULL !!!");
    ERROR_Assert(ldp, "MPLS-LDP structure is NULL !!!");
    ERROR_Assert((putOutLabel) || (putInLabel),
                " both \"putOutLabel\" and \"putInLabel\" parameters"
                " are set to FALSE");

    if ((putOutLabel) && (!putInLabel))
    {
        lib = SearchEntriesInTheLib(ldp, fec, &in_label);

        if (!lib)
        {
            Mpls_NHLFE* nhlfe1 = NULL;
            Mpls_NHLFE* nhlfe2 = NULL;

            in_label = Implicit_NULL_Label;

            nhlfe1 = MplsAddNHLFEEntry(
                        mpls,
                        NetworkGetInterfaceIndexForDestAddress(node,
                            nextHopAddr),
                        nextHopAddr,
                        Operation,
                        outLabel,
                        numLabels);

            if (!DuplicateEntryInILM(mpls, *inLabel, nhlfe1))
            {
                MplsAddILMEntry(mpls, in_label, nhlfe1);
            }

            nhlfe2 = MplsAddNHLFEEntry(
                        mpls,
                        NetworkGetInterfaceIndexForDestAddress(node,
                            nextHopAddr),
                        nextHopAddr,
                        FIRST_LABEL,
                        outLabel,
                        numLabels);

            // MplsAddFTNEntry(mpls, fec, nhlfe2);
            MplsAssignFECtoOutgoingLabelAndInterface(node, ipAddress,
                numSignificantBits, nextHopAddr, FIRST_LABEL,
                outLabel, 1);
        }
        else
        {
            ERROR_Assert(((in_label)&&(in_label > 15)),
                         "in_label is invalid");

            nhlfe = MplsAddNHLFEEntry(
                        mpls,
                        NetworkGetInterfaceIndexForDestAddress(
                            node,
                            nextHopAddr),
                        nextHopAddr,
                        Operation,
                        outLabel,
                        numLabels);

            ilm = MplsMatchILM(mpls, in_label);

            if (!ilm)
            {
                MplsAddILMEntry(mpls, in_label, nhlfe);
            }
            else if (ilm->nhlfe->labelStack[0] == Implicit_NULL_Label)
            {
                MEM_free(ilm->nhlfe->labelStack);
                MEM_free(ilm->nhlfe);
                ilm->nhlfe = nhlfe;
            }

            MplsAssignFECtoOutgoingLabelAndInterface(node, ipAddress,
                numSignificantBits, nextHopAddr, FIRST_LABEL,
                outLabel, 1);
        }
    }
    else if ((!putOutLabel) && (putInLabel))
    {
        unsigned int outLabel = Implicit_NULL_Label;
        ERROR_Assert(*inLabel, "incoming label is Implicit_NULL_Label");

        nhlfe = MplsAddNHLFEEntry(
                    mpls,
                    NetworkGetInterfaceIndexForDestAddress(node,nextHopAddr),
                    nextHopAddr,
                    Operation,
                    &outLabel,
                    numLabels);

        if (!DuplicateEntryInILM(mpls, *inLabel, nhlfe))
        {
            MplsAddILMEntry(mpls, *inLabel, nhlfe);
        }
    }
    else if ((putOutLabel) && (putInLabel))
    {
        ilm = MplsMatchILM(mpls, *inLabel);

        if (ilm)
        {
            if (ilm->nhlfe->labelStack[0] == Implicit_NULL_Label)
            {
                ilm->nhlfe->labelStack[0] = *outLabel;
            }
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsSetLabelAdvertisementMode
//
// PURPOSE      : Informing MPLS-LDP label advertisement to mac layer mpls
//
// PARAMETERS   : node - the node which is performing the above operation.
//                controlMode - label advertisement control mode st by
//                              MPLS-LDP
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsSetLabelAdvertisementMode(Node* node, int controlMode)
{
    MplsData* mpls = ReturnMplsVar(node);
    mpls->labelDistributionControlMode = controlMode;
}


//
//  MPLS Queue Management Functions
//
//
//-------------------------------------------------------------------------
// FUNCTION     : MplsRoutePacket()
//
// PURPOSE      : Routing a packet to the next hop is corresponding FTN
//                entry for that dest is found in the FTN table. Otherwise
//                it invokes a callback function to start label
//                distribution operation.
//
// PARAMETERS   : node - the node which is tring to route the packet.
//                mpls - pointer to mpls structure.
//                msg - msg/packet to be routed.
//                destAddr - destination address of the msg/packet.
//                packetWasRouted - MplsRoutePacket() will set this
//                                  parameterto TRUE if the packet is
//                                  successfully routed.
//                priority - priority of the msg/packet.
//                incomingInterface - Index of interface on which packet
//                                    arrived.
//                outgoingInterface - Used only when the application
//                                    specifies a specific interface to use
//                                    to transmit packet.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void MplsRoutePacket(
    Node* node,
    MplsData* mpls,
    Message* msg,
    NodeAddress destAddr,
    BOOL* packetWasRouted,
     // Parameter added for priority support for FEC classification.
    unsigned int priority,
    int incomingInterface,
    int outgoingInterface)
{
    IpHeaderType *ipHeader = (IpHeaderType *) msg->packet;
    int interfaceIndex;

    //fragmentation variables for MPLS fragmentation
    ipFragmetedMsg* fragHead = NULL;
    ipFragmetedMsg* tempFH = NULL;
    BOOL fragmentedByMpls = FALSE;

    if (incomingInterface == CPU_INTERFACE)
    {
        // If sent by this node, then routing protocol should be associated
        // with the outgoing interface.
        interfaceIndex = outgoingInterface;
    }
    else
    {
        // If packet is being forwarded, then routing protocol should be
        // associated with the incoming interface.
        interfaceIndex = incomingInterface;
    }
   // Changed for adding priority support for FEC classification.
    // Third parameter priority is been passed.
    Mpls_FTN_entry* ftn = MplsMatchFTN(node, destAddr , priority);
    BOOL isFTN = TRUE ;

    // The following code has been added for IP + MPLS integration.
    // If the FTN entry has Implicit NULL label then
    // it means that node it egress node.
    if (ftn)
    {
        if (*(ftn->nhlfe->labelStack) == Implicit_NULL_Label)
        {
            isFTN = FALSE ;
        }
    }
    else
    {
        isFTN = FALSE ;
    }

    // The following has been done for integrating MPLS with IP.
    // Before triggering FTN or checking for FTN, check if the node
    // is Ingress for this FEC

    //Code added for rsvp integration with new code of IP-MPLS
    BOOL isIngress = FALSE;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    MplsLdpApp *ldp = MplsLdpAppGetStateSpace(node);

    if (rsvp)
    {
        isIngress = IsIngressRsvp(node, msg);
    }
    else if (ldp)
    {
        isIngress = IsIngress(node, msg, mpls->isEdgeRouter);
    }
    else if (!ldp)
    {
        // For Static MPLS checking connection with the next hop.
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
        if ((mpls->isEdgeRouter) || (srcOfPacket))
        {
            NodeAddress nextHopAddress = (NodeAddress) NETWORK_UNREACHABLE;
            int interfaceIndex = NETWORK_UNREACHABLE;
            if (ftn)
            {
                NetworkGetInterfaceAndNextHopFromForwardingTable(
                    node, ftn->nhlfe->nextHop,
                    &interfaceIndex, &nextHopAddress);

                if ((interfaceIndex == NETWORK_UNREACHABLE) ||
                    (nextHopAddress == (unsigned) NETWORK_UNREACHABLE))
                {
                    isIngress = FALSE;
                }
                else
                {
                    isIngress = TRUE;
                }
            }
            else
            {
                // We need to drop the packet, as ftn entry will not be
                // generated at later point of time in static mpls.
                // Optional field of MPLS integration with IP (drop or send
                // to ip on error) is not valid  here, as packets will be
                // routed via IP when FTN entry is not found.

                mpls->stats->numPacketsHandoveredToIPatIngressDueToNoFTN++;

                //This stat contains the total number of packets sent to IP
                mpls->stats->numPacketHandoveredToIp++;
                *packetWasRouted = FALSE;
                if (MPLS_DEBUG)
                {
                    printf("At node %d, No route exist in MPLS for FEC:"
                            "Dest=%x, Priority = %d\n",
                        node->nodeId, destAddr, priority) ;
                }
            }
        }
    }

    if (isIngress)
    {
        mpls->stats->numPacketsReceivedFromIP++;
        if (isFTN)
        {
            Mpls_NHLFE* nhlfe = NULL;
            nhlfe = ftn->nhlfe;

            if (MPLS_DEBUG)
            {
                char dest[MAX_STRING_LENGTH];
                char next[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(destAddr, dest);
                IO_ConvertIpAddressToString(nhlfe->nextHop, next);

                printf("#%d: MplsRoutePacket finds FTN match- "
                       "outgoing interface = %d"
                       "destAddr = %s next hop = %s\n",
                       node->nodeId,
                       MplsReturnOutgoingInterfaceFromNHLFE(nhlfe),
                       dest,
                       next);
            }
            // Fragmentation done here if required.
            if ((MESSAGE_ReturnPacketSize(msg) + (int)
                 sizeof(Mpls_Shim_LabelStackEntry)) >
                 GetNetworkIPFragUnit(node, nhlfe->nextHop_OutgoingInterface))
            {
                if (!(IpHeaderGetIpDontFrag(ipHeader->ipFragment)))
                {
                    if (IpFragmentPacket(node,
                            msg,
                            GetNetworkIPFragUnit(node,
                                    nhlfe->nextHop_OutgoingInterface),
                            &fragHead,
                            TRUE))//This means fragmentaion for MPLS
                    {
                        fragmentedByMpls = TRUE;
                    }
                    else
                    {
                        ERROR_ReportError("IP header with option header,"
                            " is not allowed for IP fragmentation"
                            " in this version at MPLS\n");
                    }
                }
                else
                {
                    // return if DF bit is set and packet size
                    // is greater than fragment
                    *packetWasRouted = FALSE;
                    return;
                }
            } // End fragmentation code.

            BOOL queueIsFull = FALSE;

            if (!fragmentedByMpls)
            {
                MplsIpOutputQueueInsert(node,
                                        mpls,
                                        nhlfe->nextHop_OutgoingInterface,
                                        msg,
                                        nhlfe->nextHop,
                                        &queueIsFull,
                                        TRUE); //is Packet from IP == TRUE

                if (queueIsFull)
                {
                   MESSAGE_Free(node, msg);
                }
            }
            else
            {
                if (fragHead)
                {
                    int fragId = 0;
                    while (fragHead)
                    {
#ifdef EXATA
#ifdef PAS_INTERFACE
                        IpHeaderType *ipH = (IpHeaderType *)
                                         MESSAGE_ReturnPacket(fragHead->msg);
                        int ifaceIndex;

                        if (incomingInterface == CPU_INTERFACE)
                        {
                            ifaceIndex = outgoingInterface;
                        }
                        else
                        {
                            ifaceIndex = incomingInterface;
                        }

                        if (PAS_LayerCheck(node,
                                           ifaceIndex,
                                           PACKET_SNIFFER_ETHERNET))
                        {
                            PAS_IPv4(node,
                                    ipH,
                                    0,
                                    MESSAGE_ReturnPacketSize(fragHead->msg));
                        }
#endif
#endif
                        // STATS DB CODE
#ifdef ADDON_DB
                        // Here we add the packet to the Network database.
                        // Gather as much information we can for the database.

                        StatsDBAppendMessageNetworkMsgId(node, fragHead->msg, 
                            fragId++) ;                
#endif

                        MplsIpOutputQueueInsert(node,
                                            mpls,
                                            nhlfe->nextHop_OutgoingInterface,
                                            fragHead->msg,
                                            nhlfe->nextHop,
                                            &queueIsFull,
                                            TRUE); //is Packet from IP ==TRUE

                        if (queueIsFull)
                        {
                           MESSAGE_Free(node, fragHead->msg);
                        }

                        tempFH = fragHead;
                        fragHead = fragHead->next;
                        MEM_free(tempFH);
                    } // end of while loop
                    MESSAGE_Free(node, msg);
                } // end of if block
            }
            *packetWasRouted = TRUE;
        }
        else
        {   // Changed for adding priority support for FEC classification.
            if (!MplsLdpAppGetStateSpace(node) && (!rsvp))
            {
                // This confirms that neither LDP nor RSVP is running
                // Optional field of MPLS integration with IP (drop or send
                // to ip on error) is not valid  here, as packets will be
                // routed via IP when FTN entry is not found.

                mpls->stats->numPacketsHandoveredToIPatIngressDueToNoFTN++;
                //This stat contains the total number of packets sent to IP
                mpls->stats->numPacketHandoveredToIp++;
                *packetWasRouted = FALSE;
                if (MPLS_DEBUG)
                {
                    printf("At node %d, No route exist in MPLS for FEC:"
                            "Dest=%x, Priority = %d\n",
                        node->nodeId, destAddr, priority) ;
                }
            }
            else
            {

                NodeAddress nextHop = (NodeAddress) NETWORK_UNREACHABLE;
                int interfaceIndex = ANY_INTERFACE;

                NetworkGetInterfaceAndNextHopFromForwardingTable(
                node,
                destAddr,
                &interfaceIndex,
                &nextHop);

                if (nextHop != (unsigned) NETWORK_UNREACHABLE)
                {
                    MplsData* checkMplsActive =
                        (MplsData*)  node->macData[interfaceIndex]->mplsVar;

                    if (checkMplsActive)
                    {
                        if (checkMplsActive->ReportNewFECCallback)
                        {
                            MplsNewFECCallbackFunctionType newFecCallback =
                                checkMplsActive->ReportNewFECCallback;

                            // call appropriate label distribution protocol
                            (newFecCallback)(node,
                                             checkMplsActive,
                                             destAddr,
                                             interfaceIndex,
                                             nextHop);
                        }
                    }
                }
                // Optional field of MPLS integration with IP (drop or send
                // to ip on error)

                if (mpls->routeToIPOnError)
                {
                  mpls->stats->numPacketsHandoveredToIPatIngressDueToNoFTN++;
                  //This stat contains the total number of packets sent to IP
                  mpls->stats->numPacketHandoveredToIp++;
                  *packetWasRouted = FALSE;
                }
                else
                {
                    // Fragmentation done here if required.
                    if ((MESSAGE_ReturnPacketSize(msg) +
                         (int) sizeof(Mpls_Shim_LabelStackEntry)) >
                                                  GetNetworkIPFragUnit(node, interfaceIndex))
                    {
                        if (!(IpHeaderGetIpDontFrag(ipHeader->ipFragment)))
                        {
                            if (IpFragmentPacket(node,
                                    msg,
                                    GetNetworkIPFragUnit(node, interfaceIndex),
                                    &fragHead,
                                    TRUE))//This means fragmentaion for MPLS
                            {
                                fragmentedByMpls = TRUE;
                            }
                            else
                            {
                                ERROR_ReportError("IP header with option"
                                    " header, is not allowed for IP"
                                    " fragmentation in this version"
                                    " at MPLS\n");
                            }
                        }
                        else
                        {
                            // return if DF bit is set and packet size
                            // is greater than fragment.*/
                            *packetWasRouted = FALSE;
                            return;
                        }
                    } // End fragmentation code.

                    BOOL queueIsFull = FALSE;

                    if (!fragmentedByMpls)
                    {
                        MplsIpOutputQueueInsert(node,
                                mpls,
                                interfaceIndex,
                                msg,
                                nextHop,
                                &queueIsFull,
                                TRUE); //is Packet from IP == TRUE

                        if (queueIsFull)
                        {
                           MESSAGE_Free(node, msg);
                        }
                    }
                    else
                    {
                        if (fragHead)
                        {
                            int fragId = 0 ;
                            while (fragHead)
                            {
#ifdef EXATA
#ifdef PAS_INTERFACE
                                IpHeaderType *ipH = (IpHeaderType *)
                                         MESSAGE_ReturnPacket(fragHead->msg);

                                int ifaceIndex;
                                if (incomingInterface == CPU_INTERFACE)
                                {
                                    ifaceIndex = outgoingInterface;
                                }
                                else
                                {
                                    ifaceIndex = incomingInterface;
                                }

                                if (PAS_LayerCheck(node,
                                                   ifaceIndex,
                                                   PACKET_SNIFFER_ETHERNET))
                                {
                                    PAS_IPv4(node,
                                             ipH,
                                             0,
                                             MESSAGE_ReturnPacketSize(
                                                             fragHead->msg));
                                }
#endif
#endif
                                // STATS DB CODE
#ifdef ADDON_DB
                                // Here we add the packet to the Network database.
                                // Gather as much information we can for the database.

                                StatsDBAppendMessageNetworkMsgId(node, fragHead->msg, 
                                    fragId++) ;                
#endif

                                MplsIpOutputQueueInsert(node,
                                        mpls,
                                        interfaceIndex,
                                        fragHead->msg,
                                        nextHop,
                                        &queueIsFull,
                                        TRUE); //is Packet from IP == TRUE

                                if (queueIsFull)
                                {
                                   MESSAGE_Free(node, fragHead->msg);
                                }

                                tempFH = fragHead;
                                fragHead = fragHead->next;
                                MEM_free(tempFH);
                            } // end of while loop
                            MESSAGE_Free(node, msg);
                        } // end of if block
                    }
                    *packetWasRouted = TRUE;
                }
             } // end of else static route
        } // end of else ftn not found
    } // end of isIngress
    else
    {
        // Node is not Ingress, hence packet will be routed by IP if
        // This can be case of Edge router sending packet to IP via IP.

        *packetWasRouted = FALSE;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : SetAllPendingReadyToSend()
//
// PURPOSE      : to change status of all the pending packets in the queue
//                from MPLS_LABEL_REQUEST_PENDING, to MPLS_READY_TO_SEND.
//                This operation is performed for a given dest address and
//                a given interface index.
//
// PARAMETERS   : node - node which is performing the above operation.
//                interfaceIndex - interface index
//                destIp - destination address.
//                mpls - pointer to mpls structure.
//                nhlfe - pointer to NHLFE structure in the FTN table for
//                        the given destIp.
//
// RETURN VALUE : Integer, number of packet who's status is changed.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
int SetAllPendingReadyToSend(
    Node* node,
    int interfaceIndex,
    NodeAddress destIp,
    MplsData* mpls,
    Mpls_NHLFE* nhlfe)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    Scheduler* scheduler = NULL;
    int i = DEQUEUE_NEXT_PACKET;
    Message* msg = NULL;
    int numPacketInQueue = 0;
    int hasPacketPending = 0;

    ERROR_Assert(interfaceIndex >= 0, "interfaceIndex less than zero");
    ERROR_Assert(interfaceIndex < node->numberInterfaces,
                 "interface index greater than number of interfaces");

    scheduler = ipLayer->interfaceInfo[interfaceIndex]->scheduler;

    numPacketInQueue = (*scheduler).numberInQueue(ALL_PRIORITIES);

    while (numPacketInQueue > i)
    {
        IpHeaderType* ipHeader = NULL;
        QueuePriorityType queuePriority = ALL_PRIORITIES;

        // check the number of packets in the queue
        (*scheduler).retrieve(ALL_PRIORITIES,
                              i,
                              &msg,
                              &queuePriority,
                              PEEK_AT_NEXT_PACKET,
                              node->getNodeTime());

        if (msg)
        {
            // The following has been done for integrating MPLS with IP.
            LlcHeader* llc = NULL;
            llc = (LlcHeader*) MESSAGE_ReturnPacket(msg);

            if (llc->etherType == PROTOCOL_TYPE_MPLS)
            {
                // The following code has been written for integrating
                // MPLS with IP.
                // Remove LLC header before extracting ipHeader

                LlcRemoveHeader(node, msg);
                if (MESSAGE_ReturnInfo(msg))
                {
                    int status = MplsReturnStatus(msg);
                    if (status == MPLS_LABEL_REQUEST_PENDING)
                    {
                        ipHeader = (IpHeaderType*)MESSAGE_ReturnPacket
                                                     (msg);
                        if (ipHeader->ip_dst == destIp)
                        {
                            //removing dummy virtual pay load
                            MESSAGE_RemoveVirtualPayload(node,
                                msg,
                            sizeof(Mpls_Shim_LabelStackEntry));

                            MplsReturnStatus(msg) = MPLS_READY_TO_SEND;

                            MplsProcessPacketWithNHLFE(node, mpls, msg,
                                nhlfe, node->nodeId, ipHeader->ip_ttl,
                                interfaceIndex);
                            // Code added for MPLS and IP integration
                            // Add LLC header after adding MPLS Shim
                            LlcAddHeader(node, msg, PROTOCOL_TYPE_MPLS);

                            hasPacketPending++;
                            MAC_NetworkLayerHasPacketToSend(node,
                                                        interfaceIndex);
                            // check number of packets in the queue again
                            numPacketInQueue =
                              (*scheduler).numberInQueue(ALL_PRIORITIES);

                            i = (DEQUEUE_NEXT_PACKET - 1);
                        }
                        else
                        {
                            //Add LLC header back after adding MPLS Shim
                            LlcAddHeader(node, msg, PROTOCOL_TYPE_MPLS);
                        }
                    }
                    else
                    {
                        //Add LLC header back after adding MPLS Shim
                        LlcAddHeader(node, msg, PROTOCOL_TYPE_MPLS);
                     }

                    } // end if (MESSAGE_ReturnInfo(msg))
                    else
                    {
                        //Add LLC header back after adding MPLS Shim
                        LlcAddHeader(node, msg, PROTOCOL_TYPE_MPLS);
                    }
                } // end of if etherType == PROTOCOL_TYPE_MPLS
                llc = NULL;
        } // end if (msg)
        i++;
    } // end while (numPacketInQueue > i)
    mpls->stats->numPendingPacketsCleared += hasPacketPending;
    return hasPacketPending;
}



//-------------------------------------------------------------------------
// FUNCTION     : MplsIpQueueInsert()
//
// PURPOSE      : Inserting a packet into the mpls queue.If the packet is
//                from ip and destination address is ANY_DEST then add a
//                broadcast label to the packet and then insert into the
//                queue.
//
// PARAMETERS   : node - node which is inserting the packet in the queue
//                mpls - pointer to mpls structure.
//                msg - pointer to Message structure.
//                interfaceIndex - interface index of the queue.
//                nextHopAddress - next hop address of the packet/message.
//                status - Mpls Status
//                tos - Type of Service
//                packetFromIP - set this parameter to TRUE if the packet
//                               is received from IP.
//                QueueIsFull - MplsIpOutputQueueInsert() will set this
//                              parameter to TRUE if queue is full.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------

void MplsIpQueueInsert(Node *node,
                       MplsData* mpls,
                       Message *msg,
                       int interfaceIndex,
                       NodeAddress nextHopAddress,
                       MplsStatus status,
                       TosType* tos,
                       BOOL packetFromIP,
                       BOOL* QueueIsFull)
{

    int queueIndex = ALL_PRIORITIES;
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    MacHWAddress hwAddr;
    BOOL isResolved = FALSE;

    if (ArpIsEnable(node,interfaceIndex))
    {
        if (node->macData[interfaceIndex]->macProtocol !=
                    MAC_PROTOCOL_802_3)
        {
            ERROR_Assert(LlcIsEnabled(node,interfaceIndex),
                "LLC Should be enabled, when ARP is used"
                "in protocol other than 802.3");
        }

        isResolved = ArpTTableLookup(node,
                                     interfaceIndex,
                                     PROTOCOL_TYPE_IP,
                                     *tos,
                                     nextHopAddress,
                                     &hwAddr,
                                     &msg,
                                     ANY_INTERFACE,
                                     PROTOCOL_TYPE_MPLS);
        if (!isResolved)
        {
            *QueueIsFull = FALSE;
            return;
        }
    }
    else
    {
        isResolved = IPv4AddressToHWAddress(node,
                                            interfaceIndex,
                                            msg,
                                            nextHopAddress,
                                            &hwAddr);
    }


    MplsUpdateInfoField(node, interfaceIndex,msg,
                        nextHopAddress, status,hwAddr);

    if (LlcIsEnabled(node, interfaceIndex))
    {
         LlcAddHeader(node,msg,PROTOCOL_TYPE_MPLS);
    }

    // Call the "insertFunction" via a ugly function pointer.
    queueIndex = GenericPacketClassifier(
            ipLayer->interfaceInfo[interfaceIndex]->scheduler,
            (int) ReturnPriorityForPHB(node,*tos));

    // Handle IPv4 packets.  IPv6 is not currently supported.
    if (node->networkData.networkProtocol != IPV6_ONLY)
    {
        NetworkIpAddPacketSentToMacDataPoints(
            node,
            msg,
            interfaceIndex);
    }

    (*ipLayer->interfaceInfo[interfaceIndex]->scheduler).insert(
                                    msg,
                                    QueueIsFull,
                                    queueIndex,
                                    NULL, //const void* infoField,
                                    node->getNodeTime());
    BOOL queueWasEmpty = MplsOutputQueueIsEmpty(node, interfaceIndex);

    if (packetFromIP)
    {

        if (*QueueIsFull)
        {
            // number of packet packet dropped at source increases.
            mpls->stats->numPacketDroppedAtSource++;

            // NOTE: This situation can occur if packets are sent by
            //       the application at very high rate. That way the
            //       the queues get filled-up by the packets before
            //       mpls could create the LSP. Recovery of the dropped
            //       packets are the responsibility of the corresponding
            //       higher lavel protocols.
        }
        else
        {
            // Put the packet in the queue. Once the packet is in the
            // queue, one can be sure that it will sent to the mac layer
            // for transmission once LSP is created (remember it is the
            // source end). So one may increament the statistics
            // here safely.
            mpls->stats->numPacketsSentToMac++;
            if (status != MPLS_LABEL_REQUEST_PENDING)
            {
                // The following has been done for integrating MPLS with IP.
                // Status is pending, hence no need to indicate MAC
                // to dequeue the packet

                MAC_NetworkLayerHasPacketToSend(node, interfaceIndex);
            }
        }
    }
    else
    {
        if (!queueWasEmpty)
        {
            if (MPLS_DEBUG)
            {
                printf("MPLS queue #%d/%d was empty\n",
                       node->nodeId,
                       interfaceIndex);
            }

            mpls->stats->numPacketsSentToMac++;
            MAC_NetworkLayerHasPacketToSend(node, interfaceIndex);
        }
    }
}

//-------------------------------------------------------------------------
// FUNCTION     : MplsIpOutputQueueInsert()
//
// PURPOSE      : Inserting a packet into the mpls queue.If the packet is
//                from ip and destination address is ANY_DEST then add a
//                broadcast label to the packet and then insert into the
//                queue.
//
// PARAMETERS   : node - node which is inserting the packet in the queue
//                mpls - pointer to mpls structure.
//                interfaceIndex - interface index of the queue.
//                msg - message/packet to be inserted in the queue.
//                nextHopAddress - next hop address of the packet/message.
//                QueueIsFull - MplsIpOutputQueueInsert() will set this
//                              parameter to TRUE if queue is full.
//                packetFromIP - set this parameter to TRUE if the packet
//                               is received from IP.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void
MplsIpOutputQueueInsert(
    Node* node,
    MplsData* mpls,
    int interfaceIndex,
    Message* msg,
    NodeAddress nextHopAddress,
    BOOL* QueueIsFull,
    BOOL packetFromIP)
{
    if (packetFromIP)
    {
        IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
        Mpls_FTN_entry* ftn = NULL;
        MplsStatus status = MPLS_LABEL_REQUEST_PENDING;

        if ((ipHeader->ip_dst == ANY_DEST) ||
            (nextHopAddress == ANY_DEST)   ||
            NetworkIpIsMulticastAddress(node, ipHeader->ip_dst))
        {
            if (MPLS_DEBUG)
            {
                printf("#%d: FTN - ANY_DEST\n", node->nodeId);
            }

            MplsAddBroadcastLabel(node, mpls, msg, ipHeader->ip_ttl);

            status = MPLS_READY_TO_SEND;
        }
        else
        {
            // Changed for adding priority support for FEC classification.
            // Third parameter i.e. priority of data packet has been passed.

            ftn = MplsMatchFTN(node, ipHeader->ip_dst,
                IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len));

            if (ftn)
            {
                Mpls_NHLFE* nhlfe = NULL;
                nhlfe = ftn->nhlfe;

                if (MPLS_DEBUG)
                {
                    printf("#%d: FTN match- outgoing label = %d  dest  %d\n",
                           node->nodeId,
                           nhlfe->labelStack[0],
                           ipHeader->ip_dst);
                }

                MplsProcessPacketWithNHLFE(node, mpls, msg, nhlfe,
                    node->nodeId, ipHeader->ip_ttl, interfaceIndex);

                status = MPLS_READY_TO_SEND;
            }
            else if ((ipHeader->ip_dst == nextHopAddress) ||
                     (nextHopAddress == 0))
            {
                // NOTE : nextHopAddress can be zero if the destination
                //        is directly connected.(i.e. destination node
                //        is itself the next hop)
                if (MPLS_DEBUG)
                {
                    printf("ipHeader->ip_dst %u / nextHopAddress = %u\n",
                           ipHeader->ip_dst,
                           nextHopAddress);
                }

                MplsAddBroadcastLabel(node, mpls, msg, ipHeader->ip_ttl);
                status = MPLS_READY_TO_SEND;
            }
            else
            {
                if (MPLS_DEBUG)
                {
                    printf("#%d: No FTN match for %d\n",
                           node->nodeId,
                           ipHeader->ip_dst);
                }
                status = MPLS_LABEL_REQUEST_PENDING;

                 //adding dummy virtual pay load
                 MESSAGE_AddVirtualPayload(node,
                     msg,
                     sizeof(Mpls_Shim_LabelStackEntry));

            }
        }

        TosType tos = IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len) ;
        MplsIpQueueInsert(node,
                          mpls,
                          msg,
                          interfaceIndex,
                          nextHopAddress,
                          status,
                          &tos,
                          packetFromIP,
                          QueueIsFull);

    }
    else
    {
        TosType tos = (TosType) IPTOS_PREC_ROUTINE;

        MplsIpQueueInsert(node,
                          mpls,
                          msg,
                          interfaceIndex,
                          nextHopAddress,
                          MPLS_READY_TO_SEND,
                          &tos,
                          FALSE,
                          QueueIsFull);
    }
}

//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueIsEmpty()
//
// PURPOSE      : checking whether a queue associated with a perticular
//                interface index is empty or not.
//
// PARAMETERS   : node - node which is checking the queue
//                interfaceIndex - interface index of the queue.
//
// RETURN VALUE : BOOL, Returns TRUE is queue is empty or FALSE otherwise
//                Queue will be said to be empty if there is NO IP packet
//                (inserted in queue via IP) and NO packet (inserted in
//                queue via MPLS)
//                with status MPLS_READY_TO_SEND.
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueIsEmpty(Node* node, int interfaceIndex)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*)  node->networkData.networkVar;
    Scheduler* scheduler = NULL;

    ERROR_Assert(interfaceIndex >= 0, "interfaceIndex less than zero");
    ERROR_Assert(interfaceIndex < node->numberInterfaces,
                 "interface index greater than number of interfaces");

    scheduler = ipLayer->interfaceInfo[interfaceIndex]->scheduler;

    // Call the "isEmptyFunction" via a ugly function pointer.
    if (!((*scheduler).isEmpty(ALL_PRIORITIES)))
    {
        int i = 0;
        Message* msg = NULL;
        QueuePriorityType queuePriority = ALL_PRIORITIES;
        int numPkts = (*scheduler).numberInQueue(ALL_PRIORITIES);

        for (i = 0; i < numPkts; i++)
        {
            // check the packets in the queue
            (*scheduler).retrieve(ALL_PRIORITIES,
                                  i,
                                  &msg,
                                  &queuePriority,
                                  PEEK_AT_NEXT_PACKET,
                                  node->getNodeTime());
            // The following has been done for integrating MPLS with IP.
            // Look for both IP and MPLS packets in the queue.

            LlcHeader* llc;
            llc = (LlcHeader*) MESSAGE_ReturnPacket((msg));
            if (llc->etherType == PROTOCOL_TYPE_MPLS)
            {
                if (MplsReturnStatus(msg) == MPLS_READY_TO_SEND)
                {
                    return FALSE;
                }
            }
            else if (llc->etherType == PROTOCOL_TYPE_IP)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsNotificationOfPacketDrop()
//
// PURPOSE      : To Implement something to indicate that the MPLS label
//                is incorrect
//
// PARAMETERS   : node - node which is performing the above operation
//                msg - msg/packet dropped.
//                nextHopAddress - next hop address of the packet.
//                interfaceIndex - interface index of the queue.
//
// RETURN VALUE : void
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsNotificationOfPacketDrop(
    Node* node,
    Message* msg,
    NodeAddress nextHopAddress,
    int interfaceIndex)
{
    // keep removing the MPLS Shim header 1-by-1 untill
    // bottom of stack is reached
    BOOL bottomOfStack = FALSE;
    Mpls_Shim_LabelStackEntry shim_label;
    while (1)
    {
        if (MplsReturnStatus(msg) == MPLS_LABEL_REQUEST_PENDING)
        {
            // removing dummy virtual pay load
            MESSAGE_RemoveVirtualPayload(node,
                                         msg,
                                         sizeof(Mpls_Shim_LabelStackEntry));
            break;
        }
        else
        {
            MplsShimPopTopLabel(node, msg, &bottomOfStack, &shim_label);

            if (bottomOfStack)
            {
                break;
            }
        }
    }

    NetworkIpNotificationOfPacketDrop(
        node,
        msg,
        nextHopAddress,
        interfaceIndex);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueTopPacketForAPriority()
//
// PURPOSE      : Look at the packet at the front of the specified priority
//                output queue
//
// PARAMETERS   : node - the node which is performing the above operation.
//                interfaceIndex - interface index of the queue
//                priority - Network queueing priority
//                msg - the message/packet to be looked for.
//                nextHopAddress - next hop address of the packet.
//                posInQueue - the packet's position in the queue.
//
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueTopPacketForAPriority(
    Node* node,
    int interfaceIndex,
    TosType priority,
    Message** msg,
    NodeAddress* nextHopAddress,
    MacHWAddress* nextHopMacAddr,
    int posInQueue)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    Scheduler* scheduler = NULL;
    TosType notUsed = ALL_PRIORITIES;

    // This field has been added for integrating MPLS with IP
    int i = posInQueue;

    ERROR_Assert(interfaceIndex >= 0, "interfaceIndex less than zero");
    ERROR_Assert(interfaceIndex < node->numberInterfaces,
                 "interface index greater than number of interfaces");

    scheduler = ipLayer->interfaceInfo[interfaceIndex]->scheduler;

    (*scheduler).retrieve(priority,
                          i,
                          msg,
                          &notUsed,
                          PEEK_AT_NEXT_PACKET,
                          node->getNodeTime());

    if (*msg == NULL)
    {
        return FALSE;
    }
    if (MplsReturnStatus((*msg)) == MPLS_LABEL_REQUEST_PENDING)
    {
       return FALSE;
    }

    TackedOnInfoWhileInMplsQueueType* infoPtr = NULL;
    infoPtr = (TackedOnInfoWhileInMplsQueueType*)(MESSAGE_ReturnInfo(*msg));

    // Copying nextHopAddress
    *nextHopAddress = infoPtr->nextHopAddress;

    // Copying next Hop Mac Address byte, type, length.
    if (nextHopMacAddr->byte == NULL)
    {
        nextHopMacAddr->byte = 
                              (unsigned char*) MEM_malloc(infoPtr->hwLength);
    }
    memcpy(nextHopMacAddr->byte,infoPtr->macAddress, infoPtr->hwLength);
    nextHopMacAddr->hwLength = infoPtr->hwLength;
    nextHopMacAddr->hwType = infoPtr->hwType;
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueDequeuePacketForAPriority()
//
// PURPOSE      : Remove the packet at the front of the specified priority
//                output queue.
//
// PARAMETERS   : node - the node which is performing the dequeue operation.
//                interfaceIndex - interface index of the queue
//                priority - Network queueing priority
//                msg - the message/packet to be dequeued.
//                nextHopAddress - next hop address of the packet.
//
// RETURN VALUE : void
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueDequeuePacketForAPriority(
    Node* node,
    int interfaceIndex,
    TosType priority,
    Message** msg,
    NodeAddress* nextHopAddress,
    MacHWAddress* nextHopMacAddr,
    int posInQueue)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    Scheduler* scheduler = NULL;
    TosType NotUsed = 0;

    // This field has been added for integrating MPLS with IP
    int i = posInQueue;
    ERROR_Assert(interfaceIndex >= 0, "interfaceIndex less than zero");
    ERROR_Assert(interfaceIndex < node->numberInterfaces,
                 "interface index greater than number of interfaces");

    scheduler = ipLayer->interfaceInfo[interfaceIndex]->scheduler;

    // retrieve next packets in the queue
    (*scheduler).retrieve(
        priority,
        i,
        msg,
        &NotUsed,
        PEEK_AT_NEXT_PACKET,
        node->getNodeTime());

    if (*msg == NULL) {
        return FALSE;
    }
    // check packet in the queue
    (*scheduler).retrieve(
            priority,
            i,
            msg,
            &NotUsed,
            PEEK_AT_NEXT_PACKET,
            node->getNodeTime());
    if (MplsReturnStatus((*msg)) == MPLS_LABEL_REQUEST_PENDING)
    {
       return FALSE;
    }

    // check the packet in the queue
    (*scheduler).retrieve(
        priority,
        i,
        msg,
        &NotUsed,
        DEQUEUE_PACKET,
        node->getNodeTime());

    ERROR_Assert(*msg != NULL, "Dequeued Message in NULL!!!");
    TackedOnInfoWhileInMplsQueueType* infoPtr = NULL;
    infoPtr = (TackedOnInfoWhileInMplsQueueType*)(MESSAGE_ReturnInfo(*msg));

    // Copying Next Hop Address
    *nextHopAddress = infoPtr->nextHopAddress;

    // Copying Next Hop Mac Address
    if (nextHopMacAddr->byte == NULL)
    {
        nextHopMacAddr->byte = 
                              (unsigned char*) MEM_malloc(infoPtr->hwLength);
    }
    memcpy(nextHopMacAddr->byte,infoPtr->macAddress, infoPtr->hwLength);
    nextHopMacAddr->hwLength = infoPtr->hwLength;
    nextHopMacAddr->hwType = infoPtr->hwType;
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsReceivePacketFromMacLayer()
//
// PURPOSE      : Receive a packet from mac Layer. If it is node's own
//                packet the put it thru to the IP layer (see the function
//                call MplsProcessPacketWithNHLFE()), or puts it into the
//                respactive output queue.
//
// PARAMETERS   : node - node which is receiving the packet from the Mac
//                       layer
//                interfaceIndex - interface index thru which the packet
//                                 is received.
//                msg - the message/packet which is received from mac
//                lastHopAddress - previous hop
//
// RETURN VALUE : void
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsReceivePacketFromMacLayer(
    Node* node,
    int interfaceIndex,
    Message* msg,
    NodeAddress lastHopAddress)
{
    MplsData* mpls = (MplsData*) node->macData[interfaceIndex]->mplsVar;
    MplsLabelEntry mpls_label;
    Mpls_ILM_entry* ilm = NULL;

    ERROR_Assert(mpls, "MPLS Structure  is NULL");

    switch(mpls->labelEncoding)
    {
        case MPLS_SHIM:
        {
            MplsShimReturnTopLabel(msg, &(mpls_label.shim));
            ilm = MplsMatchILM(mpls,
               Mpls_Shim_LabelStackEntryGetLabel(mpls_label.shim.shimStack));

            if (ilm)
            {
                Mpls_NHLFE* nhlfe = NULL;
                nhlfe = ilm->nhlfe;

                if (MPLS_DEBUG)
                {
                    printf("#%d: ILM match- outgoing label = %d\n",
                           node->nodeId,
                           (nhlfe->labelStack ? (int) nhlfe->labelStack[0] :
                                                -1));
                }
                mpls->stats->numPacketsReceivedFromMac++;

                // The following has been done for integrating MPLS with IP.
                if (MESSAGE_ReturnPacketSize(msg) > GetNetworkIPFragUnit
                                                       (node, interfaceIndex))
                {
                    // if packet size > MTU of the outgoing interface then we
                    // need to send packet to IP if routeToIPOnError is true,
                    // else drop.
                    if (mpls->routeToIPOnError)
                    {
                        mpls->stats->numPacketsHandoveredToIPatLSRdueToMTU++;
                        // pop label and send to IP for forwarding
                        MplsPopLabelStack(node,
                                          mpls,
                                          msg,
                                          lastHopAddress,
                                          Mpls_Shim_LabelStackEntryGetTTL
                                          (mpls_label.shim.shimStack),
                                          interfaceIndex);
                    }
                    else
                    {
                        // Drop and free the message/packet
                        mpls->stats->numPacketsDroppedDueToLessMTU++;
                        MESSAGE_Free(node, msg);
                    }
                 }

                // At egress node, TTL will not be decremented here
                // This will be decremented at IP
                if (nhlfe->Operation != POP_LABEL_STACK)
                {

                    if (!MplsProcessPacketWithNHLFE(
                        node, mpls, msg, nhlfe,lastHopAddress,
                        (Mpls_Shim_LabelStackEntryGetTTL(
                        mpls_label.shim.shimStack) - 1),
                                    interfaceIndex))
                    {
                        BOOL queueIsFull = FALSE;

                        if (MPLS_DEBUG)
                        {
                            printf("#%d: ILM routes packet to %d\n",
                                   node->nodeId,
                                   nhlfe->nextHop_OutgoingInterface);
                        }

                        MplsIpOutputQueueInsert(
                            node,
                            mpls,
                            nhlfe->nextHop_OutgoingInterface,
                            msg,
                            nhlfe->nextHop,
                            &queueIsFull,
                            FALSE); // is Packet from IP == FALSE

                        if (queueIsFull)
                        {
                            ((NetworkDataIp*)node->
                             networkData.networkVar)->stats.ipOutDiscards++;

                            MESSAGE_Free(node, msg);
                        }
                    }
                }
                else
                {
                    if (!MplsProcessPacketWithNHLFE(
                        node, mpls, msg, nhlfe,lastHopAddress,
                        Mpls_Shim_LabelStackEntryGetTTL(
                        mpls_label.shim.shimStack),interfaceIndex))
                    {
                        BOOL queueIsFull = FALSE;
                        if (MPLS_DEBUG)
                        {
                            printf("#%d: ILM routes packet to %d\n",
                                   node->nodeId,
                                   nhlfe->nextHop_OutgoingInterface);
                        }

                        MplsIpOutputQueueInsert(
                            node,
                            mpls,
                            nhlfe->nextHop_OutgoingInterface,
                            msg,
                            nhlfe->nextHop,
                            &queueIsFull,
                            FALSE); // is Packet from IP == FALSE

                        if (queueIsFull)
                        {
                            ((NetworkDataIp*)node->
                            networkData.networkVar)->stats.ipOutDiscards++;

                            MESSAGE_Free(node, msg);
                        }
                    }
                }

            }
            else // if (ilm == NULL)
            {
                // code enters this block means :- ILM entry do not
                // exists for an incoming labeled packet.

                if (mpls->labelDistributionControlMode ==
                       MPLS_CONTROL_MODE_INDEPENDENT)
                {
                    if (MPLS_DEBUG)
                    {
                        if (Mpls_Shim_LabelStackEntryGetLabel(
                            mpls_label.shim.shimStack)!= 3)
                        {
                            printf("#%d: No ILM match for incoming"
                                   " label %d\n",
                                   node->nodeId,
                                   Mpls_Shim_LabelStackEntryGetLabel(
                                   mpls_label.shim.shimStack));

                            Print_FTN_And_ILM(node);
                        }
                    }

                    // ERROR_Assert(mpls_label.shim.Label == 3,
                    //            "Incoming packets label is not"
                    //            "Implicit_NULL_Label");
                }
                else
                {
                    // NOTE: This ERROR_ReportError() statement can be
                    //       un-commented for de bugging purpose.
                    //       If labelDistributionControlMode is ORDERED
                    //       and no fault is exercised control MUST NOT
                    //       reach here.
                    //
                    // sprintf(errorStr,
                    //      "#%d: No ILM match for incoming label %d\n",
                    //       node->nodeId,
                    //       mpls_label.shim.Label);
                    // ERROR_ReportError(errorStr);
                }
                // If ILM is not found then packet will be
                // either sent to IP or dropped based on the
                // user configuration.

                if (mpls->routeToIPOnError)
                {
                   mpls->stats->numPacketsHandoveredToIPatLSRdueToBadLabel++;
                   MplsPopLabelStack( node,
                                      mpls,
                                      msg,
                                      lastHopAddress,
                                      Mpls_Shim_LabelStackEntryGetTTL(
                                      mpls_label.shim.shimStack),
                                      interfaceIndex);

                }
                else
                {
                    // Drop and free the message/packet
                    mpls->stats->numPacketsDroppedDueToBadLabel++;
                    MESSAGE_Free(node, msg);
                }
            }
            break;
        } // end case MPLS_SHIM
        default :
        {
            ERROR_ReportError("MPLS Error : "
                              "Unknown label encoding scheme");
            break;
        }
    } // end switch(mpls->labelEncoding)
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueTopPacket()
//
// PURPOSE      : Look at the packet at the front of the output queue
//
// PARAMETERS   : node - the node which is performing the above operation.
//                interfaceIndex - interface index of the queue
//                msg - the message/packet to be looked for.
//                nextHopAddress - next hop address of the packet.
//                priority - Network queueing priority
//                posInQueue - the packet's position in the queue.
//
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueTopPacket(
    Node* node,
    int interfaceIndex,
    Message**  msg,
    NodeAddress* nextHopAddress,
    MacHWAddress* nextHopMacAddr,
    TosType* priority,
    int posInQueue)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    Scheduler* scheduler = NULL;
    TosType NotUsed = ALL_PRIORITIES;

    // This field has been added for integrating MPLS with IP
    int i = posInQueue;

    ERROR_Assert(interfaceIndex >= 0, "interfaceIndex less than zero");
    ERROR_Assert(interfaceIndex < node->numberInterfaces,
                 "interface index greater than number of interfaces");

    scheduler = ipLayer->interfaceInfo[interfaceIndex]->scheduler;

    // check the next packets in the queue
    (*scheduler).retrieve(NotUsed,
                          i,
                          msg,
                          priority,
                          PEEK_AT_NEXT_PACKET,
                          node->getNodeTime());

    if (*msg == NULL) {
        return FALSE;
    }

    if (MplsReturnStatus((*msg)) == MPLS_LABEL_REQUEST_PENDING)
    {
       return FALSE;
    }

    TackedOnInfoWhileInMplsQueueType* infoPtr = NULL;
    infoPtr = (TackedOnInfoWhileInMplsQueueType*)(MESSAGE_ReturnInfo(*msg));

    // Copying Next Hop Mac Address
    *nextHopAddress = infoPtr->nextHopAddress;

    // Copying Next Hop Mac Address byte, type, length.
    if (nextHopMacAddr->byte == NULL)
    {
        nextHopMacAddr->byte = 
                              (unsigned char*) MEM_malloc(infoPtr->hwLength);
    }
    memcpy(nextHopMacAddr->byte,infoPtr->macAddress, infoPtr->hwLength);
    nextHopMacAddr->hwLength = infoPtr->hwLength;
    nextHopMacAddr->hwType = infoPtr->hwType;
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueDequeuePacket()
//
// PURPOSE      : Remove the packet at the front of output queue.
//
// PARAMETERS   : node - the node which is performing the dequeue operation.
//                interfaceIndex - interface index of the queue
//                priority - Network queueing priority
//                msg - the message/packet to be dequeued.
//                nextHopAddress - next hop address of the packet.
//                posInQueue - position of packet in queue
//
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueDequeuePacket(
    Node*  node,
    int interfaceIndex,
    Message** msg,
    NodeAddress* nextHopAddress,
    MacHWAddress* nextHopMacAddr,
    TosType* priority,
    int posInQueue)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    Scheduler* scheduler = NULL;
    TosType NotUsed = ALL_PRIORITIES;
    int numPacketInQueue = 0;

    ERROR_Assert(interfaceIndex >= 0, "interfaceIndex less than zero");
    ERROR_Assert(interfaceIndex < node->numberInterfaces,
                 "interface index greater than number of interfaces");

    scheduler = ipLayer->interfaceInfo[interfaceIndex]->scheduler;

    // check the next packet in the queue
    (*scheduler).retrieve(NotUsed,
                          posInQueue,
                          msg,
                          priority,
                          PEEK_AT_NEXT_PACKET,
                          node->getNodeTime());

    if (*msg == NULL)
    {
        return TRUE;
    }

    numPacketInQueue = (*scheduler).numberInQueue(NotUsed);
    (*scheduler).retrieve(NotUsed,
                          posInQueue,
                          msg,
                          priority,
                          PEEK_AT_NEXT_PACKET,
                          node->getNodeTime());
    if (MplsReturnStatus((*msg)) == MPLS_LABEL_REQUEST_PENDING)
    {
       return FALSE;
    }

    (*scheduler).retrieve(NotUsed,
                          posInQueue,
                          msg,
                          priority,
                          DEQUEUE_PACKET,
                          node->getNodeTime());

    ERROR_Assert(*msg != NULL, "Dequeued Message in NULL!!!");

    TackedOnInfoWhileInMplsQueueType* infoPtr = NULL;
    infoPtr = (TackedOnInfoWhileInMplsQueueType*)(MESSAGE_ReturnInfo(*msg));

    // Copying Next Hop Mac Address
    *nextHopAddress = infoPtr->nextHopAddress;

    // Copying Next Hop Mac Address byte, type, length.
    if (nextHopMacAddr->byte == NULL)
    {
        nextHopMacAddr->byte = 
                              (unsigned char*) MEM_malloc(infoPtr->hwLength);
    }
    memcpy(nextHopMacAddr->byte,infoPtr->macAddress, infoPtr->hwLength);
    nextHopMacAddr->hwLength = infoPtr->hwLength;
    nextHopMacAddr->hwType = infoPtr->hwType;
    return TRUE;
}

//
// End MPLS Queue Management Functions
//

//
// Define MplsInit(), MplsLayer() and MplsFinalize() function
//
//-------------------------------------------------------------------------
// FUNCTION     : MplsInit()
//
// PURPOSE      : Initialize variables, being called once for each node
//                at the beginning.
//
// PARAMETERS   : node - The node to be initialized.
//                nodeInput - the input configuration file
//                interfaceIndex - interface index
//                mplsVar - pointer to pointer to "MplsData" structure.
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
void MplsInit(
    Node* node,
    const NodeInput* nodeInput,
    int interfaceIndex,
    MplsData** mplsVar)
{
    if (MPLS_DEBUG)
    {
        printf("#%d: MplsInit()\n", node->nodeId);
    }
    // For integration of MPLS with IP, LLC must be enabled
    BOOL isLlcEnabled = FALSE;
    isLlcEnabled = LlcIsEnabled(node, interfaceIndex);
    if (!isLlcEnabled)
    {
        ERROR_ReportError("MPLS requires LLC to be enabled."
            " Please enable LLC!");
    }

    *mplsVar = ReturnMplsVar(node);

    if (*mplsVar)
    {
        // multiple interfaces running MPLS, previously initialized
        return;
    }
    else
    {
        BOOL wasFound = FALSE;
        int retVal = FALSE;
        char buf[MAX_STRING_LENGTH] = {0};
        Mpls_NHLFE* nhlfe = NULL;
        int i = 0;

        NodeInput mplsStaticRouteInput;

        *mplsVar = (MplsData*)  MEM_malloc(sizeof(MplsData));
        memset(*mplsVar, 0, sizeof(MplsData));

        IO_ReadCachedFile(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "MPLS-STATIC-ROUTE-FILE",
            &wasFound,
            &mplsStaticRouteInput);

        if (!wasFound)
        {
            mplsStaticRouteInput.numLines = 0;
        }

        // Allocate memory space for MPLS statistical variables.
        // Assumption is that - mpls statistics structure is always
        // initialized and statistical variables are always collected
        // properly. But printed only if MPLS-STATISTICS is YES.
        (*mplsVar)->stats = (MplsStatsType*)
                     MEM_malloc(sizeof(MplsStatsType));

        MplsInitStats((*mplsVar)->stats);

        // Collect statistics?
        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "MPLS-STATISTICS",
            &wasFound,
            buf);

        if (wasFound)
        {
            if (strcmp(buf, "YES") == 0)
            {
                (*mplsVar)->collectStats = TRUE;
            }
            else if (strcmp(buf, "NO") == 0)
            {
                (*mplsVar)->collectStats = FALSE;
            }
            else
            {
                ERROR_ReportError("MPLS-STATISTICS should be"
                                  " set to \"YES\"\" or \"NO\".\n");
            }
        }
        else
        {
            // Default option is do not collect statistis.
            (*mplsVar)->collectStats = FALSE;
        }
        // Check whether the node is an Edge Router or not
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-EDGE-ROUTER",
            &wasFound,
            buf);

        if (wasFound)
        {
            if (strcmp("YES", buf) == 0)
            {
                (*mplsVar)->isEdgeRouter = TRUE;
            }
            else if (strcmp("NO", buf) == 0)
            {
                (*mplsVar)->isEdgeRouter = FALSE;

            }
            else
            {
                ERROR_ReportError(
                       "\"MPLS-EDGE-ROUTER\" "
                    "should be either \"YES\" or \"NO\".");
            }
        }
        else
        {
            // The default option for MPLS-EDGE-ROUTER is "NO"
            (*mplsVar)->isEdgeRouter = FALSE;
        }

        // Route via IP on Error
        IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MPLS-ROUTE-TO-IP-ON-ERROR",
            &wasFound,
            buf);

        if (wasFound)
        {
            if (strcmp("YES", buf) == 0)
            {
                (*mplsVar)->routeToIPOnError = TRUE;
            }
            else if (strcmp("NO", buf) == 0)
            {
                (*mplsVar)->routeToIPOnError = FALSE;

            }
            else
            {
                ERROR_ReportError(
                       "\"MPLS-ROUTE-TO-IP-ON-ERROR\" "
                    "should be either \"YES\" or \"NO\".");
            }
        }
        else
        {
            // The default option for MPLS-EDGE-ROUTER is "NO"
            (*mplsVar)->routeToIPOnError = FALSE;
        }

        (*mplsVar)->FTN = (Mpls_FTN_entry*)
                          MEM_malloc(sizeof(Mpls_FTN_entry) *
                                            DEFAULT_MPLS_TABLE_SIZE);
        (*mplsVar)->numFTNEntries = 0;
        (*mplsVar)->maxFTNEntries = DEFAULT_MPLS_TABLE_SIZE;

        (*mplsVar)->ILM = (Mpls_ILM_entry*)
                          MEM_malloc(sizeof(Mpls_ILM_entry) *
                                            DEFAULT_MPLS_TABLE_SIZE);

        (*mplsVar)->numILMEntries = 0;
        (*mplsVar)->maxILMEntries = DEFAULT_MPLS_TABLE_SIZE;

        // Modify this if we implement a label encoding scheme
        // other than SHIM
        (*mplsVar)->labelEncoding = MPLS_SHIM;

        (*mplsVar)->ReportNewFECCallback = NULL;

        nhlfe = MplsAddNHLFEEntry(
                    (*mplsVar),
                    DEFAULT_INTERFACE,
                    node->nodeId,
                    POP_LABEL_STACK,
                    NULL,
                    0); // 0 == numLabels

        MplsAddILMEntry(
            (*mplsVar),
            IPv4_Explicit_NULL_Label,
            nhlfe);

        for (i = 0; i < mplsStaticRouteInput.numLines; i++)
        {
            char entryTypeStr[MAX_STRING_LENGTH] = {0};

            sscanf(mplsStaticRouteInput.inputStrings[i], "%s",
                   entryTypeStr);

            if (strcmp(entryTypeStr, "FTN") == 0)
            {
                char sourceString[MAX_STRING_LENGTH] = {0};
                char destString[MAX_STRING_LENGTH] = {0};
                char nextHopString[MAX_STRING_LENGTH] = {0};
                unsigned int label;
                unsigned int* labelStack = NULL;
                NodeAddress sourceNodeId;
                NodeAddress sourceAddr;
                NodeAddress destAddr;
                int destNumSubnetBits;
                NodeAddress nextHopNodeId;
                NodeAddress nextHopAddr;
                Mpls_FEC fec;
                // Changed for adding priority support for FEC classification
                // Default value has been initialised to zero.
                unsigned int priority = 0;

                // Changed for adding priority support for FEC classification
                retVal = sscanf(mplsStaticRouteInput.inputStrings[i],
                                "%*s %s %s %d %s %d",
                                sourceString,
                                destString,
                                &label,
                                nextHopString,
                                &priority);

                // Changed for adding priority support for FEC classification
                if ((retVal < 4) || (retVal > 5))
                {
                    char errorStr[MAX_STRING_LENGTH];

                    sprintf(errorStr,
                            "MPLS static label assignment specified"
                            " incorrectly.\n"
                            "\tFTN <src IP> <dest IP> <outgoing label>"
                            " <next hop IP> [priority]\nor\n"
                            "\tILM <src IP> <incoming label>"
                            " <outgoing label>"
                            " <next hop IP>\n%s\n",
                            mplsStaticRouteInput.inputStrings[i]);

                    ERROR_ReportError(errorStr);
                }

                MplsParseSourceDestAndNextHopStrings(
                    node,
                    mplsStaticRouteInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destAddr,
                    &destNumSubnetBits,
                    nextHopString,
                    &nextHopNodeId,
                    &nextHopAddr);

                if (node->nodeId == sourceNodeId)
                {
                    if (MPLS_DEBUG)
                    {
                        printf("Label %d from node %d:%d to node/subnet %d:"
                               " (bits %d) via node %d:%d\n",
                               label,
                               sourceNodeId,
                               sourceAddr,
                               destAddr,
                               destNumSubnetBits,
                               nextHopNodeId, nextHopAddr);
                    }

                    labelStack = (unsigned int*)
                         MEM_malloc(sizeof(unsigned int));

                    *labelStack = label;

                    nhlfe = MplsAddNHLFEEntry(
                                (*mplsVar),
                                NetworkGetInterfaceIndexForDestAddress(
                                    node,
                                    nextHopAddr),
                                nextHopAddr,
                                FIRST_LABEL,
                                labelStack,
                                1); // 1 == Number of Labels

                    fec.ipAddress = destAddr;
                    fec.numSignificantBits = (sizeof(NodeAddress) * 8)
                                              - destNumSubnetBits;

                    // Changed for adding priority support for FEC
                    // classification.
                    // If priroty is less then zero then it is an error.
                    ERROR_Assert((priority >= 0),
                        "Priority is less than Zero");

                    fec.priority = priority ;
                    MplsAddFTNEntry((*mplsVar), fec, nhlfe);
                 }
            }
            else if (strcmp(entryTypeStr, "ILM") == 0)
            {
                unsigned int incomingLabel,
                             outgoingLabel;

                char sourceString[MAX_STRING_LENGTH];
                char nextHopString[MAX_STRING_LENGTH];
                NodeAddress sourceNodeId;
                NodeAddress sourceAddr;
                NodeAddress nextHopNodeId;
                NodeAddress nextHopAddr = 0;
                unsigned int* labelStack = NULL;
                Mpls_NHLFE* nhlfe = NULL;

                retVal = sscanf(mplsStaticRouteInput.inputStrings[i],
                                "%*s %s %d %d %s",
                                sourceString,
                                &incomingLabel,
                                &outgoingLabel,
                                nextHopString);

                if (retVal != 4)
                {
                    char errorStr[MAX_STRING_LENGTH];

                    sprintf(errorStr,
                            "MPLS static label assignment specified"
                            " incorrectly.\n"
                            "\tFTN <src IP> <dest IP> <outgoing label>"
                            " <next hop IP> [priority] \nor\n"
                            "\tILM <src IP> <incoming label>"
                            "<outgoing label>"
                            " <next hop IP>\n%s\n",
                            mplsStaticRouteInput.inputStrings[i]);

                    ERROR_ReportError(errorStr);
                }

                MplsParseSourceAndNextHopStrings(
                    node,
                    mplsStaticRouteInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    nextHopString,
                    &nextHopNodeId,
                    &nextHopAddr);

                if (node->nodeId == sourceNodeId)
                {
                    if (MPLS_DEBUG)
                    {
                        printf("Incoming Label %d on node %d:%d maps to"
                               " outgoing label %d, nextHop node %d:%d\n",
                               incomingLabel,
                               sourceNodeId,
                               sourceAddr,
                               outgoingLabel,
                               nextHopNodeId,
                               nextHopAddr);
                    }

                    if (!MplsProcessSpecialLabelForILM(
                             node,
                             (*mplsVar),
                             incomingLabel,
                             outgoingLabel,
                             nextHopNodeId,
                             nextHopAddr))
                    {
                        labelStack = (unsigned int*)
                             MEM_malloc(sizeof(unsigned int));

                        *labelStack = outgoingLabel;

                        nhlfe = MplsAddNHLFEEntry(
                                    (*mplsVar),
                                    NetworkGetInterfaceIndexForDestAddress(
                                        node,
                                        nextHopAddr),
                                    nextHopAddr,
                                    REPLACE_LABEL,
                                    labelStack,
                                    1); // 1 == numLabels

                        MplsAddILMEntry((*mplsVar), incomingLabel, nhlfe);
                    }
                }
            }
            else
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr,
                        "MPLS static label assignment specified"
                        " incorrectly.\n"
                        "\tFTN <src IP> <dest IP> <outgoing label>"
                        " <next hop IP>\nor\n"
                        "\tILM <src IP> <incoming label> <outgoing label>"
                        " <next hop IP>\n%s\n",
                        mplsStaticRouteInput.inputStrings[i]);

                ERROR_ReportError(errorStr);
            }
        }
    } // end else
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsLayer()
//
// PURPOSE      : Process MPLS Messages
//
// PARAMETERS   : node - the node which is receiving the message
//                interfaceIndex - interface index via which the message
//                                 is received
//                msg - the event/message
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
void MplsLayer(Node* node, int interfaceIndex, Message* msg)
{
    if (MPLS_DEBUG)
    {
        printf("#%u: MPLSLayer() Received a message interface %d\n",
               node->nodeId,
               interfaceIndex);
    }
    MESSAGE_Free(node, msg);
    ERROR_ReportError("There are currently no MPLS messages in the layer");
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsPrintStats()
//
// PURPOSE      : Print out MPLS network statistics.
//
// PARAMETERS   : node - node who's statistics is to be printed.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void MplsPrintStats(Node* node)
{
    char buf[MAX_STRING_LENGTH];
    MplsData* mplsVar = ReturnMplsVar(node);

    if (mplsVar)
    {
        sprintf(buf, "Number of packets received from IP Layer = %u",
            mplsVar->stats->numPacketsReceivedFromIP);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Number of packets sent to Mac Layer = %u",
                mplsVar->stats->numPacketsSentToMac);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Number of packets received from Mac Layer = %u",
                mplsVar->stats->numPacketsReceivedFromMac);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Total number of packets sent to IP Layer = %u",
                mplsVar->stats->numPacketHandoveredToIp);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Number of pending packets cleared = %u",
                mplsVar->stats->numPendingPacketsCleared);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Number of packets dropped at source due to Buffer"
            " full = %u", mplsVar->stats->numPacketDroppedAtSource);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Number of packets dropped due to Bad Label = %u",
                mplsVar->stats->numPacketsDroppedDueToBadLabel);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Number of packets dropped due to Packet size bigger"
            " than IP Fragmentation Unit = %u",
        mplsVar->stats->numPacketsDroppedDueToLessMTU);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);



        sprintf(buf, "Number of packets dropped due to expired TTL = %u",
                mplsVar->stats->numPacketsDroppedDueToExpiredTTL);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Number of packets sent to IP due to No FTN at"
                     " Ingress = %u",
        mplsVar->stats->numPacketsHandoveredToIPatIngressDueToNoFTN);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Number of packets sent to IP due to Bad Label = %u",
                mplsVar->stats->numPacketsHandoveredToIPatLSRdueToBadLabel);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);

        sprintf(buf, "Number of packets sent to IP due to Packet size"
            " bigger than IP Fragmentation Unit = %u",
            mplsVar->stats->numPacketsHandoveredToIPatLSRdueToMTU);
        IO_PrintStat(
            node,
            "MAC",
            "MPLS",
            ANY_DEST,
            ANY_INTERFACE, // instance ID
            buf);
    }
    else
    {
        // one should not reach here...
        ERROR_ReportError("MPLS protocol is not Running"
                          "Can not print statistics !!!");
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsFinalize()
//
// PURPOSE      : Print out statistics, being called at the end. Calls
//                MplsPrintStats()
//
// PARAMETERS   : node - node who's statistics is to be printed.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void
MplsFinalize(Node* node)
{
    int interfaceIndex = 0;
    BOOL isMacPrintStatEnabled = FALSE;
    MplsData* mplsVar = ReturnMplsVar(node);

    if (!mplsVar)
    {
        // if MPLS protocol is not running.Do noting
        // and return quitely.
        return;
    }

    // Check if in ANY of the MAC interfaces has macStat enabled.
    // If macStat is enabled in at least one of the MAC interfaces
    // then set isMacPrintStatEnabled to TRUE.
    for (interfaceIndex = 0;
         interfaceIndex < node->numberInterfaces;
         interfaceIndex++)
    {
        if (node->macData[interfaceIndex]->macStats == TRUE)
        {
            isMacPrintStatEnabled = TRUE;
            break;
        }
    }

    if ((isMacPrintStatEnabled == TRUE) &&
        (mplsVar->collectStats == TRUE))
    {
        // if isMacPrintStatEnabled is TRUE and mpls stat
        // is enabled, then print mpls statics.
        MplsPrintStats(node);
    }

    if (MPLS_DEBUG)
    {
        Print_FTN_And_ILM(node);
    }
}

//-----------------------------------------------------------------------------
// FUNCTION     MplsExtractInfoField
// PURPOSE      Extract the information from mpls packet info field.
// PARAMETERS   Node *node
//                  Pointer to node.
//              int interfaceIndex
//                  Index of interface.
//              Message **msg
//                  Mpls Packet.
// RETURN       Next hop of mpls packet.
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
NodeAddress
MplsExtractInfoField(Node* node,
                     int interfaceIndex,
                     Message **msg,
                     MacHWAddress* nexthopmacAddr)
{
    NodeAddress nextHopAddress;

    nextHopAddress = ((TackedOnInfoWhileInMplsQueueType *)
                     MESSAGE_ReturnInfo((*msg)))->nextHopAddress;


    nexthopmacAddr->hwLength = ((TackedOnInfoWhileInMplsQueueType *)
                                    MESSAGE_ReturnInfo((*msg)))->hwLength;
    nexthopmacAddr->hwType = ((TackedOnInfoWhileInMplsQueueType *)
                                    MESSAGE_ReturnInfo((*msg)))->hwType;
    //Added to avoid double memory allocation and hence memory leak
    if (nexthopmacAddr->byte == NULL)
    {
        nexthopmacAddr->byte = (unsigned char*) MEM_malloc(
                             sizeof(unsigned char)*nexthopmacAddr->hwLength);
    }
    memcpy(nexthopmacAddr->byte,
        ((TackedOnInfoWhileInMplsQueueType *)MESSAGE_ReturnInfo((*msg)))->macAddress,
        ((TackedOnInfoWhileInMplsQueueType *)MESSAGE_ReturnInfo((*msg)))->hwLength);

    return(nextHopAddress);
}
