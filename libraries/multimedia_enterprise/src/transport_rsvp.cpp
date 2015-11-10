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
// PURPOSE:
//
// File: rsvp.c
// Author: Debasish Ghosh
// Objective: RSVP-TE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "network_ip.h"
#include "transport_rsvp.h"
#include "qualnet_error.h"
#include "mpls.h"

#define RSVP_DEBUG_OUTPUT 0
#define RSVP_DEBUG        0
#define RSVP_DEBUG_HELLO  0

//-------------------------------------------------------------------------
// FUNCTION     RsvpStatInit
// PURPOSE      Initialization function for RSVP statistics variable.
//
// Return:          None
// Parameters:
//     node:        node being initialized.
//     nodeInput:   structure containing contents of input file
//-------------------------------------------------------------------------
static
void RsvpStatInit(
    Node* node,
    const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    // Read RSVP-STATISTICS from config file
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "RSVP-STATISTICS",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "YES") == 0)
        {
            rsvp->rsvpStatsEnabled = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            rsvp->rsvpStatsEnabled = FALSE;
        }
        else
        {
            // invalid entry in config
            char errorBuf[MAX_STRING_LENGTH];

            sprintf(errorBuf, "TRANSPORT unknown (%s) for"
                    " RSVP-STATISTICS\n", buf);
            ERROR_ReportError(errorBuf);
        }
    }
    else
    {
        // Assume FALSE if not specified
        rsvp->rsvpStatsEnabled = FALSE;
    }

    // allocate memory for Statistics element
    rsvp->rsvpStat = (RsvpStatistics*) MEM_malloc(sizeof(RsvpStatistics));
    memset(rsvp->rsvpStat, 0, sizeof(RsvpStatistics));
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpResvStyleInit
// PURPOSE      Initialization function for RSVP RESV style.
//              Possible values are FF -> Fixed Filter style,
//                                  SE -> Shared Explicit style
//                                  WF -> Wildcard Filter style
//
// Return:          None
// Parameters:
//     node:        node being initialized.
//     nodeInput:   structure containing contents of input file
//-------------------------------------------------------------------------
static
void RsvpResvStyleInit(
    Node* node,
    const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    // read reservation style from config
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "RSVP-RESERVATION-STYLE",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "FF") == 0)
        {
            rsvp->styleType = RSVP_FF_STYLE;
        }
        else if (strcmp(buf, "SE") == 0)
        {
            rsvp->styleType = RSVP_SE_STYLE;
        }
        else if (strcmp(buf, "WF") == 0)
        {
            char errorBuf[MAX_STRING_LENGTH];

            rsvp->styleType = RSVP_WF_STYLE;

            // WF style is not implemented for traffic engineering. Still the
            // code is written for WF style, it will be required for RSVP
            // implementation later.
            sprintf(errorBuf, "WF style is not implemented"
                    " for Traffic Engineering");
            ERROR_ReportError(errorBuf);
        }
        else
        {
            // invalid style in config
            char errorBuf[MAX_STRING_LENGTH];

            sprintf(errorBuf, "TRANSPORT unknown (%s) for"
                    " RSVP-RESERVATION-STYLE\n", buf);
            ERROR_ReportError(errorBuf);
        }
    }
    else
    {
        // default style is RSVP_FF_STYLE
        rsvp->styleType = RSVP_FF_STYLE;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpExplicitRouteInit
// PURPOSE      Initialization function for RSVP RSVP_EXPLICIT_ROUTE option.
//
// Return:          None
// Parameters:
//     node:        node being initialized.
//     nodeInput:   structure containing contents of input file
//-------------------------------------------------------------------------
static
void RsvpExplicitRouteInit(
    Node* node,
    const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    int index = 0;
    NodeInput routeInput;
    BOOL retVal = FALSE;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    IO_ReadCachedFile(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "RSVP-TE-EXPLICIT-ROUTE-FILE",
        &retVal,
        &routeInput);

    if (!retVal)
    {
        return;
    }

    for (index = 0; index < routeInput.numLines; index++)
    {
        // For IO_GetDelimitedToken
        char iotoken[MAX_STRING_LENGTH];
        char* next;
        char* token = NULL;
        char* p = NULL;
        char explicitRoute[MAX_STRING_LENGTH];
        const char* delims = "{,} \n";
        int num = 0;

        strcpy(explicitRoute, routeInput.inputStrings[index]);
        p = explicitRoute;

        token = IO_GetDelimitedToken(iotoken, p, delims, &next);

        if (!token)
        {
            sprintf(buf, "Can't find Explicit Route list in %s",
                    explicitRoute);
            ERROR_ReportError(buf);
        }
        else
        {
            num = atoi(token);

            token = IO_GetDelimitedToken(iotoken, next, delims, &next);

            if (num != (int) node->nodeId)
            {
                continue;
            }

            // get the explicit path
            rsvp->abstractNodeList = (NodeAddress*) MEM_malloc
                (sizeof(NodeAddress) * MAX_EXPLICIT_ADDRESS_COUNT);

            memset(rsvp->abstractNodeList, 0,
                sizeof(NodeAddress) * MAX_EXPLICIT_ADDRESS_COUNT);

            rsvp->abstractNodeCount = 0;

            // retriving token(s)
            while (token)
            {
                int numHostBits = 0;
                BOOL isNodeId = FALSE;

                if (rsvp->abstractNodeCount > MAX_EXPLICIT_ADDRESS_COUNT)
                {
                    rsvp->abstractNodeCount = 0;
                    MEM_free(rsvp->abstractNodeList);
                    rsvp->abstractNodeList = NULL;
                    return;
                }

                // parse and collect explicit path
                IO_ParseNodeIdHostOrNetworkAddress(
                    token,
                    rsvp->abstractNodeList + rsvp->abstractNodeCount,
                    &numHostBits,
                    &isNodeId);

                if (isNodeId == TRUE)
                {
                    // The given inputs should be Ip Address and
                    // node Id. If user specifies nodeId here - then
                    // it should be treated as an error.
                    ERROR_ReportError("Giving Node Id as explicit"
                                      "route is an illigal");
                }
                rsvp->abstractNodeCount++;
                token = IO_GetDelimitedToken(iotoken, next, delims, &next);
            }//while//
        }//else//
    }//for//
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpRecordRouteInit
// PURPOSE      Initialization function for RSVP RSVP_RECORD_ROUTE option.
//              Possible values are OFF, NORMAL and LABELED
//
// Return:          None
// Parameters:
//     node:        node being initialized.
//     nodeInput:   structure containing contents of input file
//-------------------------------------------------------------------------
static
void RsvpRecordRouteInit(
    Node* node,
    const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "RSVP-TE-RECORD-ROUTE",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "OFF") == 0)
        {
            rsvp->recordRouteType = RSVP_RECORD_ROUTE_NONE;
        }
        else if (strcmp(buf, "NORMAL") == 0)
        {
            rsvp->recordRouteType = RSVP_RECORD_ROUTE_NORMAL;
        }
        else if (strcmp(buf, "LABELED") == 0)
        {
            rsvp->recordRouteType = RSVP_RECORD_ROUTE_LABELED;
        }
        else
        {
            char errorBuf[MAX_STRING_LENGTH];

            sprintf(errorBuf, "TRANSPORT unknown (%s) for"
                    " RSVP-TE-RECORD-ROUTE\n",
                    buf);
            ERROR_ReportError(errorBuf);
        }
    }
    else
    {
        // Assume FALSE if not specified
        rsvp->recordRouteType = RSVP_RECORD_ROUTE_NONE;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpPrintPSB
// PURPOSE      Print PSB for the node
//
// Return:      None
// Parameters:
//      node:   node which received the message
//-------------------------------------------------------------------------
static
void RsvpPrintPSB(Node* node)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = rsvp->psb->first;

    if (item)
    {
        printf("----------------------------------------------------------"
               "----------------------\n"
               "            (node = %u) PSB\n"
               "----------------------------------------------------------"
               "----------------------\n"
               "%16s%4s%16s%7s%14s%3s%3s%-16s\n"
               "----------------------------------------------------------"
               "----------------------\n",
               node->nodeId,
               "Sender Address", "Lsp", "Receiver", "Tunnel", "PHOP",
               "II", "OI", "    RecordRoute");
    }
    else
    {
        printf("----------------------------------------------------------"
               "----------------------\n"
               "            (node = %u) PSB (EMPTY)\n",
               node->nodeId);
    }

    // process each PSB entry
    while (item)
    {
        RsvpPathStateBlock* psb = (RsvpPathStateBlock*) item->data;
        char sender[MAX_IP_STRING_LENGTH];
        char receiver[MAX_IP_STRING_LENGTH];
        char prevHop[MAX_IP_STRING_LENGTH];
        RsvpTeObjectRecordRoute* recordRouteNormal = NULL;

        IO_ConvertIpAddressToString(
            psb->senderTemplate.senderAddress,
            sender);

        IO_ConvertIpAddressToString(
            psb->session.receiverAddress,
            receiver);

        IO_ConvertIpAddressToString(
            psb->rsvpHop.nextPrevHop,
            prevHop);

        printf("%16s%4u%16s%5u%16s%3d",
               sender,
               psb->senderTemplate.lspId,
               receiver,
               psb->session.tunnelId,
               prevHop,
               psb->incomingInterface);

        if (psb->outInterfaceList && psb->outInterfaceList->size)
        {
            printf("%3d", psb->outInterfaceList->first->data);
        }

        if (psb->recordRoute)
        {
            // if RSVP_RECORD_ROUTE present
            int length = sizeof(RsvpObjectHeader);
            RsvpTeSubObject* subObject = NULL;

            // this variable is used for debug ptinting
            int numRecRoute = 0;

            recordRouteNormal = (RsvpTeObjectRecordRoute*) psb->recordRoute;

            subObject = &(recordRouteNormal->subObject);

            // print all the node address from subObject
            while (length < recordRouteNormal->objectHeader.objectLength)
            {
                NodeAddress ipAddress;
                char ipAddr[MAX_IP_STRING_LENGTH];

                ERROR_Assert(subObject, "Invalid Subobject"
                             " in RSVP_RECORD_ROUTE");

                numRecRoute++;

                // form 4 byte IP address from two parts in sub object
                ipAddress = (subObject->ipAddress1 << 16)
                             + subObject->ipAddress2;

                IO_ConvertIpAddressToString(ipAddress, ipAddr);

                if (numRecRoute == 1)
                {
                    printf("%16s", ipAddr);
                }
                else
                {
                    printf("\n%79s", ipAddr);
                }

                length += subObject->length;
                subObject += 1;
            }
        }
        printf("\n");
        item = item->next;
    }
    printf("----------------------------------------------------------"
           "----------------------\n\n\n\n");
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpPrintRSB
// PURPOSE      Print RSB for the node
//
// Return:      None
// Parameters:
//      node:   node which received the message
//-------------------------------------------------------------------------
static
void RsvpPrintRSB(Node* node)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* filtSSItem = NULL;
    ListItem* item = rsvp->rsb->first;

    if (item)
    {
        printf("----------------------------------------------------------"
               "----------------------\n"
               "            (node = %u) RSB\n"
               "----------------------------------------------------------"
               "----------------------\n"
               "%16s%7s%16s%3s%17s%9s%-16s\n"
               "----------------------------------------------------------"
               "----------------------\n",
               node->nodeId,
               "Recever_Address", "Tunnel", "NHOP", "OI", "Sender", "Label",
               " RecordRoute");
    }
    else
    {
        printf("----------------------------------------------------------"
               "----------------------\n"
               "            (node = %u) RSB (EMPTY)\n",
               node->nodeId);
    }

    // process each RSB entry
    while (item)
    {
        RsvpResvStateBlock* rsb = (RsvpResvStateBlock*) item->data;
        BOOL formatFlag = TRUE;
        char receiver[MAX_IP_STRING_LENGTH];
        char nextHop[MAX_IP_STRING_LENGTH];

        IO_ConvertIpAddressToString(rsb->session.receiverAddress, receiver);
        IO_ConvertIpAddressToString(rsb->nextHopAddress, nextHop);

        printf("%16s%7u%16s%3d",
               receiver,
               rsb->session.tunnelId,
               nextHop,
               rsb->outgoingLogicalInterface);

        // process all FiltSS in current RSB
        filtSSItem = rsb->filtSSList->first;

        while (filtSSItem)
        {
            RsvpFiltSS* filtSS = (RsvpFiltSS*) filtSSItem->data;
            char sender[MAX_IP_STRING_LENGTH];

            IO_ConvertIpAddressToString(
                filtSS->filterSpec->senderAddress,
                sender);

            if (formatFlag)
            {
                printf("%16s%9u", sender, filtSS->label->label);
            }
            else
            {
                printf("%42s%9u", sender, filtSS->label->label);
                formatFlag = FALSE;
            }

            if (filtSS->recordRoute)
            {
                // if RSVP_RECORD_ROUTE present
                int length = sizeof(RsvpObjectHeader);
                RsvpTeSubObject* subObject = NULL;

                if (rsvp->recordRouteType == RSVP_RECORD_ROUTE_LABELED)
                {
                    // RSVP_RECORD_ROUTE is of Labeld type
                    RsvpTeObjectRecordRouteLabel* recordRouteLabel =
                        (RsvpTeObjectRecordRouteLabel*)
                            filtSS->recordRoute;

                    // this variable is used for debug ptinting
                    int numRecRoute = 0;

                    subObject = &(recordRouteLabel->subObject);

                    // collect all node list from subObjects
                    while (length < recordRouteLabel->
                            objectHeader.objectLength)
                    {
                        NodeAddress ipAddress;
                        char ipAddressStr[MAX_STRING_LENGTH];

                        ERROR_Assert(subObject, "Invalid Sub object in"
                                     " RSVP_RECORD_ROUTE");

                        numRecRoute++;

                        // form 4 byte IP addr from two parts in sub object
                        ipAddress = (subObject->ipAddress1 << 16)
                                    + subObject->ipAddress2;

                        IO_ConvertIpAddressToString(
                            ipAddress,
                            ipAddressStr);

                        if (numRecRoute == 1)
                        {
                            printf("%16s", ipAddressStr);
                        }
                        else
                        {
                            printf("\n%83s", ipAddressStr);
                        }
                        length += (subObject->length
                                  + recordRouteLabel->recordLabel.length);

                        subObject = (RsvpTeSubObject*)
                            ((char*) subObject + (subObject->length
                                + recordRouteLabel->recordLabel.length));
                    }
                }
                else
                {
                    // this variable is used for debug ptinting
                    int numRecRoute = 0;

                    // normal RSVP_RECORD_ROUTE
                    RsvpTeObjectRecordRoute* recordRouteNormal =
                        (RsvpTeObjectRecordRoute*) filtSS->recordRoute;

                    subObject = &(recordRouteNormal->subObject);

                    // process all node address
                    while (length < recordRouteNormal->
                            objectHeader.objectLength)
                    {
                        NodeAddress ipAddress;
                        char ipAddressStr[MAX_STRING_LENGTH];

                        ERROR_Assert(subObject, "Invalid Sub object in "
                                     "RSVP_RECORD_ROUTE");

                        numRecRoute++;

                        // form 4 byte IP address from two parts
                        ipAddress = (subObject->ipAddress1 << 16)
                                     + subObject->ipAddress2;

                        IO_ConvertIpAddressToString(
                            ipAddress,
                            ipAddressStr);

                        if (numRecRoute == 1)
                        {
                            printf("%16s", ipAddressStr);
                        }
                        else
                        {
                            printf("\n%83s", ipAddressStr);
                        }

                        length += subObject->length;
                        subObject += 1;
                    }
                }
            }
            filtSSItem = filtSSItem->next;
        }
        printf("\n");
        item = item->next;
    }
    printf("----------------------------------------------------------"
           "----------------------\n\n\n\n");
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpPrintHelloList
// PURPOSE      Print hello list for the node
//
// Return:      None
// Parameters:
//      node:   node which received the message
//-------------------------------------------------------------------------
static void
RsvpPrintHelloList(Node* node)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = rsvp->helloList->first;
    char neighbor[MAX_IP_STRING_LENGTH];

    if (item)
    {
        printf("----------------------------------------------------------"
               "----------------------\n"
               "            (node = %u) RSVP_HELLO List\n"
               "----------------------------------------------------------"
               "----------------------\n"
               "%16s%4s%12s%12s%10s\n"
               "----------------------------------------------------------"
               "----------------------\n",
               node->nodeId,
               "Neighbor_Addr","OI", "srcInstance", "dstInstance",
               "Failure");
    }
    else
    {
        printf("----------------------------------------------------------"
               "----------------------\n"
               "            (node = %u) RSVP_HELLO List (EMPTY)\n",
               node->nodeId);
    }

    while (item)
    {
        RsvpTeHelloExtension* hello = (RsvpTeHelloExtension*) item->data;

        ERROR_Assert(hello, "Invalid Hello list entry");

        IO_ConvertIpAddressToString(hello->neighborAddr, neighbor);

        printf("%16s%4u%12u%12u%7s\n",
               neighbor,
               hello->interfaceIndex,
               hello->srcInstance,
               hello->dstInstance,
               hello->isFailureDetected ? "YES" : "NO");

        item = item->next;
    }
    printf("----------------------------------------------------------"
           "----------------------\n\n\n\n");
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpPrintFTNAndILM
// PURPOSE      Print ILM and FTN entries for the node
//
// Return:      None
// Parameters:
//       node:  node which is printing the structure
//-------------------------------------------------------------------------
void
RsvpPrintFTNAndILM(Node* node)
{
    int i = 0;
    MplsData* mpls = MplsReturnStateSpace(node);

    if (!mpls)
    {
       return;
    }

    printf("----------------------------------------------------------"
           "----------------------\n"
           "            (node = %d) ILM List\n"
           "----------------------------------------------------------"
           "----------------------\n"
           "%8s%10s%17s%12s%12s\n"
           "----------------------------------------------------------"
           "----------------------\n",
           node->nodeId,
           "Label", "Interface", "NextHop", "Operation", "Label_Stack");

    for (i = 0; i < mpls->numILMEntries; i++)
    {
        char nextHop[MAX_IP_STRING_LENGTH];

        IO_ConvertIpAddressToString(mpls->ILM[i].nhlfe->nextHop, nextHop);

        printf("%8u%10d%17s%12u",
               mpls->ILM[i].label,
               mpls->ILM[i].nhlfe->nextHop_OutgoingInterface,
               nextHop,
               mpls->ILM[i].nhlfe->Operation);

        if (mpls->ILM[i].nhlfe->labelStack)
        {
            printf("%12u\n" ,mpls->ILM[i].nhlfe->labelStack[0]);
        }
        else
        {
            printf("%12s\n", "(NULL)");
        }
    }

    printf("----------------------------------------------------------"
           "----------------------\n"
           "            (node = %d) FTN List\n"
           "----------------------------------------------------------"
           "----------------------\n"
           "%16s%10s%17s%12s%12s\n"
           "----------------------------------------------------------"
           "----------------------\n",
           node->nodeId,
           "Receiver_Addr", "Interface", "NextHop", "Operation",
           "Label_Stack");

    for (i = 0; i < mpls->numFTNEntries; i++)
    {
        char nextHop[MAX_IP_STRING_LENGTH];
        char receiver[MAX_IP_STRING_LENGTH];

        IO_ConvertIpAddressToString(mpls->FTN[i].nhlfe->nextHop, nextHop);
        IO_ConvertIpAddressToString(mpls->FTN[i].fec.ipAddress, receiver);

        printf("%16s%10u%17s%12u",
               receiver,
               mpls->FTN[i].nhlfe->nextHop_OutgoingInterface,
               nextHop,
               mpls->FTN[i].nhlfe->Operation);

        if (mpls->FTN[i].nhlfe->labelStack)
        {
            printf("%12u\n", (mpls->FTN[i].nhlfe->labelStack[0]));
        }
        else
        {
            printf("%12s\n", "(NULL)");
        }
    }

    printf("----------------------------------------------------------"
           "----------------------\n\n\n\n");
}


static
RsvpPathStateBlock* RsvpSearchTimerStoreList(
    Node* node,
    RsvpTimerStore* tStore)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = NULL;

    ERROR_Assert(rsvp->timerStoreList, "Invalid timer store list");

    item = rsvp->timerStoreList->first;

    // Go through the PSB list
    while (item)
    {
        // get a PSB from the list
        RsvpTimerStore* timerStore = (RsvpTimerStore*) item->data;

        ERROR_Assert(timerStore, "Invalid timer store list");

        if ((timerStore->messagePtr == tStore->messagePtr) &&
            (timerStore->psb == tStore->psb))
        {
            return tStore->psb;
        }
        item = item->next;
    }
    return NULL;
}


static
void RsvpRemoveFromTimerStoreList(Node* node, RsvpPathStateBlock* cPSB)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = NULL;

    ERROR_Assert(rsvp->timerStoreList, "Invalid timer store list");

    item = rsvp->timerStoreList->first;

    // Go through the PSB list
    while (item)
    {
        // get a PSB from the list
        RsvpTimerStore* timerStore = (RsvpTimerStore*) item->data;

        ERROR_Assert(timerStore, "Invalid timer store list");

        if (timerStore->psb == cPSB)
        {
            ListItem* nextItem = item->next;
            MESSAGE_CancelSelfMsg(node, timerStore->messagePtr);
            ListGet(node, rsvp->timerStoreList, item, TRUE, FALSE);
            item = nextItem;
        }
        else
        {
            item = item->next;
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpInitApplication.
// PURPOSE      This function is called when TRANSPORT_RSVP_InitApplication
//              arrives. From here session is registered and initiates the
//              LSP creation.
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     msg:     message received by the layer
//-------------------------------------------------------------------------
static
void RsvpInitApplication(Node* node, Message* msg)
{
    AppToRsvpSend* info = (AppToRsvpSend*) MESSAGE_ReturnInfo(msg);
    NodeAddress sourceAddr = info->sourceAddr;
    NodeAddress destAddr = info->destAddr;

    RsvpUpcallFunctionType upcallFunctionPtr =
        (RsvpUpcallFunctionType) info->upcallFunctionPtr;

    RsvpTeObjectSenderTemplate senderTemplate;
    RsvpObjectSenderTSpec senderTSpec;
    RsvpObjectAdSpec adSpec;
    int sessionId;

    MplsData* mpls = MplsReturnStateSpace(node);

    // going to initiate RSVP message if MPLS traffic enginnering is enabled
    if (mpls)
    {
        // Create RSVP session for establishing LSP for current FEC
        sessionId = RsvpRegisterSession(
                        node,
                        destAddr,
                        upcallFunctionPtr);

        // Create senderTemplate, sendertSpec, adSpec objects
        senderTemplate = RsvpCreateSenderTemplate(
                             sourceAddr,
                             sessionId);

        senderTSpec = RsvpCreateSenderTSpec(node);
        adSpec = RsvpCreateAdSpec(node);

        // Initiate the RSVP message flow
        RsvpDefineSender(
            node,
            sessionId,
            &senderTemplate,
            &senderTSpec,
            &adSpec,
            IPDEFTTL);
    }
    else
    {
        // RSVP-TE cannot run without mpls so repot warning and exit.
        // So if user observes packets are reaching destination that
        // might be by any other process except RSVP-TE.
        ERROR_ReportWarning("MPLS Protocol in not enabled,"
                            " Make MPLS-PROTOCOL \"YES\" in the"
                            " configuration file \n");
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchPSBSessionSenderTemplatePair
// PURPOSE      Search in the PSB for the session and senderTemplate pair
//              If any such entry found return that PSB
//
// Return:              Matching PSB, NULL if no matching
// Parameters:
//     node:            node which received the message
//     session:         RSVP_SESSION_OBJECT object from the message
//     senderTemplate:  RSVP_SENDER_TEMPLATE object from the message
//-------------------------------------------------------------------------
static
RsvpPathStateBlock* RsvpSearchPSBSessionSenderTemplatePair(
    Node* node,
    RsvpTeObjectSession* session,
    RsvpTeObjectSenderTemplate* senderTemplate)
{
    RsvpPathStateBlock* psb = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = NULL;

    ERROR_Assert(rsvp->psb, "Invalid path state block");

    item = rsvp->psb->first;

    // Go through the PSB list
    while (item)
    {
        // get a PSB from the list
        psb = (RsvpPathStateBlock*) item->data;

        ERROR_Assert(psb, "Invalid path state block");

        // check the session, sender template pair
        if ((psb->session.receiverAddress == session->receiverAddress)
            && (psb->session.tunnelId == session->tunnelId)
            && (psb->senderTemplate.senderAddress ==
                senderTemplate->senderAddress)
            && (psb->senderTemplate.lspId == senderTemplate->lspId))
        {
            return psb;
        }
        item = item->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchPSB
// PURPOSE      Search in the PSB for the session and senderTemplate pair
//              and incoming interface matching. If any such entry found
//              return that PSB
//
// Return:              Matching PSB, NULL if no matching
// Parameters:
//     node:            node which received the message
//     msg:             message received by the layer
//     session:         RSVP_SESSION_OBJECT object from the message
//     senderTemplate:  RSVP_SENDER_TEMPLATE object from the message
//     fPSB:            return PSB if localFlag is OFF
//-------------------------------------------------------------------------
static
RsvpPathStateBlock* RsvpSearchPSB(
    Node* node,
    Message* msg,
    RsvpTeObjectSession* session,
    RsvpTeObjectSenderTemplate* senderTemplate,
    RsvpPathStateBlock** fPSB)
{
    RsvpPathStateBlock* cPSB = RsvpSearchPSBSessionSenderTemplatePair(
                                   node,
                                   session,
                                   senderTemplate);

    if (cPSB)
    {
        // In addition of that incoming interface must match with the cPSB.
        NetworkToTransportInfo* info = (NetworkToTransportInfo*)
            MESSAGE_ReturnInfo(msg);

        if (cPSB->incomingInterface != info->incomingInterfaceIndex)
        {
            cPSB = NULL;
        }
    }

    // for matching PSB if Local_Flag is off then set fPSB with matching PSB
    // otherwise set null. If any match found return cPSB otherwise
    // return NULL
    if ((cPSB != NULL) && (cPSB->localOnlyFlag == FALSE))
    {
        *fPSB = cPSB;
    }
    else
    {
        *fPSB = NULL;
    }
    return cPSB;
}

//-------------------------------------------------------------------------
// FUNCTION     RsvpCreatePSB
// PURPOSE      Create a new PSB for current session/senderTemplate pair
//
// Return:          Created PSB
// Parameters:
//     node:        node which received the message
//     allObjects:  all objects in the message
//     ipAddr:      IP address of incoming interface for current node
//-------------------------------------------------------------------------
static
RsvpPathStateBlock* RsvpCreatePSB(
    Node* node,
    RsvpAllObjects* allObjects)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    // Create a new PSB
    RsvpPathStateBlock* newPSB = (RsvpPathStateBlock*)
        MEM_malloc(sizeof(RsvpPathStateBlock));

    // initialize with 0
    memset(newPSB, 0, sizeof(RsvpPathStateBlock));

    // Save RSVP_SESSION_OBJECT, RSVP_SENDER_TEMPLATE. RSVP_SENDER_TSPEC,
    // PHOP objects in the PSB
    memcpy(&newPSB->session,
           allObjects->session,
           sizeof(RsvpTeObjectSession));

    memcpy(&newPSB->senderTemplate,
           allObjects->senderTemplate,
           sizeof(RsvpTeObjectSenderTemplate));

    memcpy(&newPSB->senderTSpec,
           allObjects->senderTSpec,
           sizeof(RsvpObjectSenderTSpec));

    if (allObjects->rsvpHop)
    {
        memcpy(&newPSB->rsvpHop,
               allObjects->rsvpHop,
               sizeof(RsvpObjectRsvpHop));
    }

    // Insert it to the PSB list and return created PSB
    ListInsert(node, rsvp->psb, node->getNodeTime(), newPSB);
    return newPSB;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchRSB
// PURPOSE      Search in the RSB for the current session and sender_template
//              matches with a RSVP_FILTER_SPEC from the filter_spec list and
//              outgoing interface of RSB must match an entry from outgoing
//              Interface list of PSB
//
// Return:      List of matching RSB, NULL if no matching
// Parameters:
//     node:    node which received the message
//     cPSB:    current PSB
//-------------------------------------------------------------------------
static
LinkedList* RsvpSearchRSB(
    Node* node,
    RsvpPathStateBlock* cPSB)
{
    LinkedList* rsbList = NULL;
    ListItem* item = NULL;
    RsvpResvStateBlock* rsb = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    ERROR_Assert(rsvp->rsb, "Invalid Resv state block");

    item = rsvp->rsb->first;
    ListInit(node, &rsbList);

    // Go through the RSB list
    while (item)
    {
        // get a RSB from the list
        rsb = (RsvpResvStateBlock*) item->data;

        ERROR_Assert(rsb, "Invalid Resv state block");

        // check the session object
        if ((rsb->session.receiverAddress == cPSB->session.receiverAddress)
            && (rsb->session.tunnelId == cPSB->session.tunnelId))
        {
            ListItem* itemSpec = rsb->filtSSList->first;
            RsvpTeObjectFilterSpec* filterSpec = NULL;

            // Now try to match RSVP_SENDER_TEMPLATE with a RSVP_FILTER_SPEC
            // in the RSVP_FILTER_SPEC list of RSB
            while (itemSpec)
            {
                filterSpec = ((RsvpFiltSS*) itemSpec->data)->filterSpec;

                ERROR_Assert(filterSpec, "Invalid Filter spec");

                if ((filterSpec->senderAddress ==
                        cPSB->senderTemplate.senderAddress)
                    && (filterSpec->lspId == cPSB->senderTemplate.lspId))
                {
                    // filter spec also matches. Now check Outgoing interface
                    // of RSB must match from outgoing interface list of PSB
                    IntListItem* itemInterface = cPSB->outInterfaceList->first;
                    int outInterface = ANY_INTERFACE;

                    while (itemInterface)
                    {
                        outInterface = itemInterface->data;

                        if (outInterface == rsb->outgoingLogicalInterface)
                        {
                            RsvpResvStateBlock* newRSB =
                                (RsvpResvStateBlock*)
                                MEM_malloc(sizeof(RsvpResvStateBlock));

                            memcpy(newRSB, rsb, sizeof(RsvpResvStateBlock));

                            // interface also matches, insert RSB in list
                            ListInsert(
                                node,
                                rsbList,
                                node->getNodeTime(),
                                newRSB);
                        }
                        itemInterface = itemInterface->next;
                    }
                }
                itemSpec = itemSpec->next;
            }
        }
        item = item->next;
    }
    return rsbList;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateObjectHeader
// PURPOSE      The function creates a object header
//
// Return:      None
// Parameters:
//     commonHeader:    Pointer to object header
//     objectSize:      size of the object
//     classNum:        object class Num
//     cType:           C_Type of the object class
//-------------------------------------------------------------------------
static
void RsvpCreateObjectHeader(
     RsvpObjectHeader* objectHeader,
     unsigned short objectSize,
     unsigned char classNum,
     unsigned char cType)
{
//    ERROR_Assert(objectHeader, "Invalid object header");

    objectHeader->objectLength = objectSize;
    objectHeader->classNum = classNum;
    objectHeader->cType = cType;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateStyle
// PURPOSE      The function creates a new RSVP_STYLE object.
//
// Return:      New RSVP_STYLE object
// Parameters:
//     node:    node which received the message
//-------------------------------------------------------------------------
static
RsvpObjectStyle RsvpCreateStyle(Node* node)
{
    RsvpObjectStyle style;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    RsvpCreateObjectHeader(
        &style.objectHeader,
        sizeof(RsvpObjectStyle),
        (unsigned char) RSVP_STYLE,
        1);

    style.flags = 0;        // not used
    style.reserved1 = 0;    // first 2 bytes of option header,not used
    // next 3 bits total 19 bits not used
    RsvpObjectStyleSetReserved2(&(style.rsvpStyleBits), 0);


    // style read from the config file
    RsvpObjectStyleSetStyleBits(&(style.rsvpStyleBits), rsvp->styleType);
    return style;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpObjectRsvpHop
// PURPOSE      The function creates a new RSVP_HOP object for previous
//              HOP or next HOP
//
// Return:      New RSVP_HOP object
// Parameters:
//     hopAddr:     IP address of the current interface
//     interfcaeId: current interface
//-------------------------------------------------------------------------
static
RsvpObjectRsvpHop RsvpCreateRsvpHop(
    NodeAddress hopAddr,
    int interfaceId)
{
    RsvpObjectRsvpHop rsvpHop;

    RsvpCreateObjectHeader(
        &rsvpHop.objectHeader,
        sizeof(RsvpObjectRsvpHop),
        (unsigned char) RSVP_HOP,
        1);

    rsvpHop.nextPrevHop = hopAddr;      // next or prev Hop IP address
    rsvpHop.lih = interfaceId;          // logical interface handle
    return rsvpHop;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateCommonHeader
// PURPOSE      The function creates a common header
//
// Return:      None
// Parameters:
//     commonHeader:    Pointer to common header
//     msgType:         RSVP message type
//     ttl:             IP TTL
//     packeSize:       size of total message
//-------------------------------------------------------------------------
static
void RsvpCreateCommonHeader(
    RsvpCommonHeader* commonHeader,
    RsvpMsgType msgType,
    unsigned char ttl,
    unsigned short packetSize)
{
    ERROR_Assert(commonHeader, "Invalid common header");
    // protocol version number
    RsvpCommonHeaderSetVersionNum(&(commonHeader->rsvpCh), RSVPv1);
    RsvpCommonHeaderSetFlags(&(commonHeader->rsvpCh), 0);//reserved right now
    commonHeader->msgType = (unsigned char) msgType;
    commonHeader->rsvpChecksum = 0;        // normally set to 0 in Qualnet
    commonHeader->sendTtl = ttl;           // TTL with which message is sent
    commonHeader->reserved = 0;
    commonHeader->rsvpLength = packetSize; // total length of the RSVP
                                           // message
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateFlowSpec
// PURPOSE      The function appends a new RSVP_FLOWSPEC at the end of a
//              message
//
// Return:          None
// Parameters:
//     resvMessage: Pointer to a pointer of Resv Message
//     packetSize:  pointer to the size of packet
//     flowSpec:    RSVP_FLOWSPEC to be added
//-------------------------------------------------------------------------
static
void RsvpCreateFlowSpec(
    void** resvMessage,
    unsigned short* packetSize,
    RsvpObjectFlowSpec* flowSpec)
{
    // new packet size is the existing plus the RSVP_FLOWSPEC size
    unsigned short newPacketSize = (short) (*packetSize
                                            + sizeof(RsvpObjectFlowSpec));

    // allocate memory for new size
    void* newResvMessage = (void*) MEM_malloc(newPacketSize);

    ERROR_Assert(*resvMessage, "Invalid Resv message");

    // copy old packet and RSVP_FLOWSPEC object in new area
    memcpy(newResvMessage, *resvMessage, *packetSize);

    memcpy(((char*) newResvMessage) + (*packetSize),
           flowSpec,
           sizeof(RsvpObjectFlowSpec));

    MEM_free(*resvMessage);

    // RSVP_FLOWSPEC object size is addded to common header
    ((RsvpCommonHeader*) newResvMessage)->rsvpLength = newPacketSize;

    // new packet and its size
    *resvMessage = newResvMessage;
    *packetSize = newPacketSize;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateSubObject
// PURPOSE      The function creates subobject of
//              "RSVP_RECORD_ROUTE/RSVP_EXPLICIT_ROUTE"
//
// Return:              None
// Parameters:
//     subObject        sub object pointer
//     ipAddr:          IP address of next hop to be added in subobject
//     classNum:        class Num type RSVP_RECORD_ROUTE or
//                      RSVP_EXPLICIT_ROUTE
//-------------------------------------------------------------------------
static
void RsvpCreateSubObject(
    RsvpTeSubObject* subObject,
    NodeAddress ipAddr,
    RsvpObjectClassNum classNum)
{
    if (classNum == RSVP_RECORD_ROUTE)
    {
        // assign record type
        subObject->TypeField.recordType = 0x01;
    }
    else if (classNum == RSVP_EXPLICIT_ROUTE)
    {
        // only consider strict route
        RsvpTeSubObjectSetLBit(&(subObject->TypeField.typeExpRoute),
            0);

        // assign subobject type
        RsvpTeSubObjectSetExpType(
            &(subObject->TypeField.typeExpRoute), 0x01);
    }
    // length of subobj
    subObject->length = sizeof(RsvpTeSubObject);

    // subobject1 is IP (upper order)
    subObject->ipAddress1 = (unsigned short) (ipAddr >> 16);

    // subobject2 is IP (lower order)
    subObject->ipAddress2 = (unsigned short) ipAddr;

    // length in bit of Ipv4 prefix
    subObject->prefixLength = 32;
    subObject->Padding.flags = 0;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateRecordRoute
// PURPOSE      The function creates a new RSVP_RECORD_ROUTE object or add
//              one sub-object in current object
//
// Return:               None
// Parameters:
//     recordRoute:      Current RSVP_RECORD_ROUTE object
//     flag:             label recording flag is desired or not
//     ipAddr:           IP address of the current interface
//     labelCType:       C_Type from RSVP_LABEL_REQUEST object
//     labelContent:     Content of label from RSVP_LABEL_REQUEST object
//     rsvpMessage:      pointer to poimter of RSVP message
//     returnPacketSize: size fo the current message
//-------------------------------------------------------------------------
static
void RsvpCreateRecordRoute(
    void* recordRoute,
    RsvpSessionAttributeFlags flag,
    NodeAddress ipAddr,
    unsigned char labelCType,
    unsigned labelContent,
    void** rsvpMessage,
    unsigned short* returnPacketSize)
{
    RsvpTeSubObject* subObject = NULL;
    unsigned short size = 0;
    unsigned short recordRouteSize = 0;
    RsvpTeObjectRecordRoute* newRecordRoute = NULL;
    void* newRsvpMessage = NULL;

    ERROR_Assert(*rsvpMessage, "Invalid RSVP message");

    if (recordRoute == NULL)
    {
        // new RSVP_RECORD_ROUTE object is going to create
        recordRouteSize = sizeof(RsvpTeObjectRecordRoute);
    }
    else
    {
        // already created add new sub object
        recordRouteSize = (short) (((RsvpTeObjectRecordRoute*)
            recordRoute)->objectHeader.objectLength
                + sizeof(RsvpTeSubObject));
    }

    if (flag == RSVP_LABEL_RECORDING_DESIRED)
    {
        // if label recording flag is on, then consider label record object
        recordRouteSize += sizeof(RsvpTeRecordLabel);
    }

    // allocate new message area for RSVP_RECORD_ROUTE object also
    newRsvpMessage = MEM_malloc(*returnPacketSize + recordRouteSize);
    memcpy(newRsvpMessage, *rsvpMessage, *returnPacketSize);

    // this is the new record route position
    newRecordRoute = (RsvpTeObjectRecordRoute*)
        (((char*) newRsvpMessage) + *returnPacketSize);

    // if object not created yet, going to create new one with object header
    if (recordRoute == NULL)
    {
        // prepare object header if not present
        size = sizeof(RsvpObjectHeader);

        RsvpCreateObjectHeader(
            &newRecordRoute->objectHeader,
            size,
            (unsigned char) RSVP_RECORD_ROUTE,
            1);
    }
    else
    {
        // copy previous RSVP_RECORD_ROUTE object to the new one
        size = ((RsvpTeObjectRecordRoute*)
                   recordRoute)->objectHeader.objectLength;

        memcpy(newRecordRoute, recordRoute, size);
    }

    // now new sub-object and label recording object will be added
    if (flag == RSVP_LABEL_RECORDING_DESIRED)
    {
        // going to add optional label recording object
        RsvpTeRecordLabel* recordLabel = (RsvpTeRecordLabel*)
            (((char*) newRecordRoute) + size);

        // type when label recording is on
        recordLabel->recordType = 0x03;

        recordLabel->length = sizeof(RsvpTeRecordLabel);

        // global label flag
        recordLabel->flags = 0x01;

        // same for the label object
        recordLabel->cType = labelCType;

        // content of label object
        recordLabel->labelContent = labelContent;

        // adjust length in the object header with the record label object
        newRecordRoute->objectHeader.objectLength = (short)
            (newRecordRoute->objectHeader.objectLength
             + recordLabel->length);

        // that much length is added in size to get next position
        size = (short) (size + recordLabel->length);
    }

    // sub object for current node is created
    subObject = (RsvpTeSubObject*) (((char*) newRecordRoute) + size);

    RsvpCreateSubObject(subObject, ipAddr, RSVP_RECORD_ROUTE);

    // adjust length in the object header with the subobject
    newRecordRoute->objectHeader.objectLength = (short)
        (newRecordRoute->objectHeader.objectLength + subObject->length);

    MEM_free(*rsvpMessage);

    // RSVP_RECORD_ROUTE object size is added. The message may be any RSVP
    // message. PathMsssage type casting done only to get commonHeader.
    ((RsvpPathMessage*) newRsvpMessage)->commonHeader.rsvpLength
        = (short) (((RsvpPathMessage*) newRsvpMessage)->commonHeader.
            rsvpLength + recordRouteSize);

    *rsvpMessage = newRsvpMessage;
    *returnPacketSize = (short) (*returnPacketSize + recordRouteSize);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpFindPhopForSenderTemplate
// PURPOSE      Search in the PSB for the current sender template to find
//              PHOP
//
// Return:          PHOP for current sender template, 0 if not found
// Parameters:
//     node:            node which received the message
//     senderTemplate:  RSVP_SENDER_TEMPLATE for which PHOP is required
//-------------------------------------------------------------------------
static
NodeAddress RsvpFindPhopForSenderTemplate(
    Node* node,
    RsvpTeObjectSenderTemplate* senderTemplate)
{
    RsvpPathStateBlock* psb = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = NULL;

    ERROR_Assert(rsvp->psb, "Invalid path state block");

    item = rsvp->psb->first;

    // Go through the PSB list
    while (item)
    {
        // get a PSB from the list
        psb = (RsvpPathStateBlock*) item->data;

        ERROR_Assert(psb, "Invalid path state block");

        // check the session, sender template pair
        if ((psb->senderTemplate.senderAddress ==
                senderTemplate->senderAddress)
            && (psb->senderTemplate.lspId == senderTemplate->lspId))
        {
            return psb->rsvpHop.nextPrevHop;
        }
        item = item->next;
    }
    return 0;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateFilterSpec
// PURPOSE      The function appends a new RSVP_FILTER_SPEC at the end of a
//              message
//
// Return:          None
// Parameters:
//     resvMessage: Pointer to a pointer of Resv Message
//     packetSize:  pointer to the size of packet
//     filterSpec:  RSVP_FILTER_SPEC to be added
//-------------------------------------------------------------------------
static
void RsvpCreateFilterSpec(
    void** resvMessage,
    unsigned short* packetSize,
    RsvpTeObjectFilterSpec* filterSpec)
{
    // new packet size is the existing plus the RSVP_FILTER_SPEC size
    unsigned short newPacketSize = (short)(*packetSize
                                           + sizeof(RsvpTeObjectFilterSpec));

    // allocate memory for new size
    void* newResvMessage = (void*) MEM_malloc(newPacketSize);

    ERROR_Assert(*resvMessage, "Invalid Resv message");

    // copy old packet and RSVP_FILTER_SPEC object in new area
    memcpy(newResvMessage, *resvMessage, *packetSize);

    memcpy(((char*) newResvMessage) + (*packetSize),
           filterSpec,
           sizeof(RsvpTeObjectFilterSpec));

    MEM_free(*resvMessage);

    // RSVP_FILTER_SPEC object size is addded to common header
    ((RsvpCommonHeader*) newResvMessage)->rsvpLength = newPacketSize;

    // new packet and its size
    *resvMessage = newResvMessage;
    *packetSize = newPacketSize;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchLSPForDuplicate
// PURPOSE      Search LSP to check whether there are more than one LSP. Then
//              return the old LSP
//
// Return:      Old LSP id, if found otherwise 0
// Parameters:
//     node:    node which received the message
//     psb:     current PSB for which a new LSP is found
//-------------------------------------------------------------------------
static
unsigned short RsvpSearchLSPForDuplicate(
    Node* node,
    RsvpTeObjectSession* session,
    RsvpTeObjectFilterSpec* filterSpec)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = rsvp->lsp->first;

    while (item)
    {
        RsvpLsp* lsp = (RsvpLsp*) item->data;

        if ((lsp->destAddr == session->receiverAddress)
            && (lsp->sourceAddr == filterSpec->senderAddress)
            && (lsp->lspId != filterSpec->lspId))
        {
            // old LSP return
            return lsp->lspId;
        }
        item = item->next;
    }

    // duplicate LSP not found, no tear down required
    return 0;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchLSP
// PURPOSE      The function searches a LSP entry
//
// Return:          In label for the current out label, 0 if not found
// Parameters:
//     node:        node which received the message
//     lspId:       LSP ID
//     outLabel:    out Label received from next HOP
//     destAddr:    destination address
//-------------------------------------------------------------------------
static
int RsvpSearchLSP(
    Node* node,
    unsigned short lspId,
    int outLabel,
    NodeAddress destAddr)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    MplsData* mpls = MplsReturnStateSpace(node);
    int i = 0;

    ListItem* item = rsvp->lsp->first;

    while (item)
    {
        RsvpLsp* lsp = (RsvpLsp*) item->data;

        // search in ILM entries
        for (i = 0; i < mpls->numILMEntries; i++)
        {
            int outIlmLabel = 0;

            if (mpls->ILM[i].nhlfe->labelStack)
            {
                   outIlmLabel = mpls->ILM[i].nhlfe->labelStack[0];
            }

            // if destination, lspId and out label matches return inLabel
            if (outIlmLabel == outLabel
                && lsp->destAddr == destAddr
                && lsp->lspId == lspId)
            {
                return lsp->inLabel;
            }
        }
        item = item->next;
    }
    return 0;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateLSP
// PURPOSE      The function creates a LSP entry
//
// Return:              Created label ID
// Parameters:
//     node:            node which received the message
//     senderAddress:   sender address
//     lspId:           LSP ID
//     outLabel:        out Label received from next HOP
//     receiverAddress: destination address
//     nextHop:         next HOP address for the destination
//-------------------------------------------------------------------------
static
int RsvpCreateLSP(
    Node* node,
    NodeAddress senderAddress,
    unsigned short lspId,
    unsigned int outLabel,
    NodeAddress receiverAddress,
    NodeAddress nextHop)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    RsvpLsp* lsp = (RsvpLsp*) MEM_malloc(sizeof(RsvpLsp));
    Mpls_NHLFE* mplsNhlfe = NULL;

    Mpls_FEC fec;
    MplsData* mpls = MplsReturnStateSpace(node);

    // create a new LSP and allocate new available label
    lsp->inLabel = rsvp->nextAvailableLabel++;
    lsp->lspId = lspId;
    lsp->sourceAddr = senderAddress;
    lsp->destAddr = receiverAddress;

    // insert new LSP in the list
    ListInsert(node, rsvp->lsp, node->getNodeTime(), lsp);

    // going to create ILM structure

    if (!nextHop)
    {
        // nextHop 0 means reached at the destination, set POP_LABEL_STACK
        mplsNhlfe = MplsAddNHLFEEntry(mpls,
                                      ANY_INTERFACE,
                                      nextHop,
                                      POP_LABEL_STACK,
                                      NULL,
                                      0); // numLabels
    }
    else
    {
        // in intermediate node, set REPLACE_LABEL
        mplsNhlfe = MplsAddNHLFEEntry(
                mpls,
                NetworkGetInterfaceIndexForDestAddress(
                    node,
                    nextHop),
                nextHop,
                REPLACE_LABEL,
                &outLabel,
                1); // numLabels
    }

    // create ILM entry
    MplsAddILMEntry(mpls, lsp->inLabel, mplsNhlfe);

    // going to create FTN entry
    mplsNhlfe = MplsAddNHLFEEntry(
                mpls,
                NetworkGetInterfaceIndexForDestAddress(
                    node, nextHop),
                nextHop,
                FIRST_LABEL,
                &outLabel,
                1); // numLabels

    fec.ipAddress = receiverAddress;
    fec.numSignificantBits = 32;

    // Changed for adding priority support for FEC classification.
    // By default the value of priority is zero.
    fec.priority = 0;

    // add in FTN entry
    MplsAddFTNEntry(mpls, fec, mplsNhlfe);

    if ((receiverAddress != nextHop) &&
        (outLabel != 0))
    {
           SetAllPendingReadyToSend(
               node,
               NetworkGetInterfaceIndexForDestAddress(node, nextHop),
               receiverAddress,
               mpls,
               mplsNhlfe);
    }

    // return allocated label
    return lsp->inLabel;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateLabel
// PURPOSE      The function appends a new RSVP_LABEL_OBJECT at the end of
//              a message
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     resvMessage: Pointer to a pointer of Resv Message
//     packetSize:  pointer to the size of packet
//     filtSS:      Filt SS
//     nextHop:     next HOP address
//-------------------------------------------------------------------------
static
void RsvpCreateLabel(
    Node* node,
    RsvpResvMessage** resvMessage,
    unsigned short* packetSize,
    RsvpFiltSS* filtSS,
    NodeAddress nextHop)
{
    RsvpTeObjectLabel newLabel;

    // new packet size is the existing plus the RSVP_FLOWSPEC size
    unsigned short newPacketSize = (short)
        (*packetSize + sizeof(RsvpTeObjectLabel));

    // allocate memory for new size
    RsvpResvMessage* newResvMessage = (RsvpResvMessage*)
        MEM_malloc(newPacketSize);

    // Now for each incoming and outgoing label pair make an entry in the LSP
    // list. The label coming with RSVP_RESV_MESSAGE is treated as the next
    // hop label identifier. The RSVP_LABEL_OBJECT created by this node for
    // creating another RSVP_RESV_MESSAGE for PHOP is incoming label.

    int inLabel = RsvpSearchLSP(
                      node,
                      filtSS->filterSpec->lspId,
                      filtSS->label->label,
                      (*resvMessage)->session.receiverAddress);

    if (!inLabel)
    {
        inLabel = RsvpCreateLSP(
                      node,
                      filtSS->filterSpec->senderAddress,
                      filtSS->filterSpec->lspId,
                      filtSS->label->label,
                      (*resvMessage)->session.receiverAddress,
                      nextHop);
    }

    // create new RSVP_LABEL_OBJECT object
    RsvpCreateObjectHeader(
        &newLabel.objectHeader,
        sizeof(RsvpTeObjectLabel),
        RSVP_LABEL_OBJECT,
        1);

    newLabel.label = inLabel;

    ERROR_Assert(*resvMessage, "Invalid Resv message");

    // copy old packet and RSVP_FLOWSPEC object in new area
    memcpy(newResvMessage, *resvMessage, *packetSize);

    memcpy(((char*)newResvMessage) + (*packetSize),
           &newLabel,
           sizeof(RsvpTeObjectLabel));

    MEM_free(*resvMessage);

    // RSVP_LABEL_OBJECT object size is addded to common header
    newResvMessage->commonHeader.rsvpLength = newPacketSize;

    // new packet and its size
    *resvMessage = newResvMessage;
    *packetSize = newPacketSize;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateFiltSS
// PURPOSE      The function appends a new RSVP_FILTER_SPEC,
//              RSVP_LABEL_OBJECT and RSVP_RECORD_ROUTE
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     resvMessage: Pointer to a pointer of Resv Message
//     packetSize:  pointer to the size of packet
//     filtSSList:  FiltSS to be added
//     ipAddr:      IP address of sending node
//     nextHop:     Next HOP address
//     phopAddr:    PHOP address
//-------------------------------------------------------------------------
static
void RsvpCreateFiltSS(
    Node* node,
    void** resvMessage,
    unsigned short* packetSize,
    LinkedList* filtSSList,
    RsvpSessionAttributeFlags flags,
    NodeAddress ipAddr,
    NodeAddress nextHop,
    NodeAddress phopAddr)
{
    ListItem* item = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    ERROR_Assert(filtSSList, "Invalid filter spec");
    ERROR_Assert(*resvMessage, "Invalid Resv message");

    item = filtSSList->first;
    while (item)
    {
        RsvpFiltSS* filtSS = (RsvpFiltSS*) item->data;

        ERROR_Assert(filtSS, "Invalid filter spec");

        // for Resv message add those filter spec that has the same input
        // PHOP For ResvErr message this check is not required
        if ((RsvpFindPhopForSenderTemplate(node, filtSS->filterSpec)
                == phopAddr)
            || (((RsvpCommonHeader*) *resvMessage)->msgType
                == RSVP_RESV_ERR_MESSAGE))
        {
            // add RSVP_FILTER_SPEC
            RsvpCreateFilterSpec(
                resvMessage,
                packetSize,
                filtSS->filterSpec);

            // RSVP_LABEL_OBJECT and RSVP_RECORD_ROUTE required only for
            // Resb message
            if (((RsvpCommonHeader*) *resvMessage)->msgType
                 == RSVP_RESV_MESSAGE)
            {
                // add RSVP_LABEL_OBJECT
                RsvpCreateLabel(
                    node,
                    (RsvpResvMessage**) resvMessage,
                    packetSize,
                    filtSS,
                    nextHop);

                // label recording is not required here make related value 0
                if (rsvp->recordRouteType != RSVP_RECORD_ROUTE_NONE)
                {
                    RsvpCreateRecordRoute(
                        filtSS->recordRoute,
                        (RsvpSessionAttributeFlags) flags,
                        ipAddr,
                        filtSS->label->objectHeader.cType,
                        filtSS->label->label,
                        resvMessage,
                        packetSize);
                }
            }
        }
        item = item->next;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateFlowDescriptor
// PURPOSE      The function appends a new flow descriptor at the end of a
//              message
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     resvMessage: Pointer to a pointer of Resv Message
//     packetSize:  pointer to the size of packet
//     flowSpec:    RSVP_FLOWSPEC to be added with the message
//     filtSSList:  RSVP_FILTER_SPEC, RSVP_LABEL_OBJECT and RSVP_RECORD_ROUTE
//                  list to be added
//     ipAddr:      IP address of the sending node
//-------------------------------------------------------------------------
static
void RsvpCreateFlowDescriptor(
    Node* node,
    void** resvMessage,
    unsigned short* packetSize,
    RsvpObjectFlowSpec* flowSpec,
    LinkedList* filtSSList,
    RsvpSessionAttributeFlags flags,
    NodeAddress ipAddr,
    NodeAddress nextHop,
    NodeAddress phopAddr)
{
    // create and add RSVP_FLOWSPEC at the end of the packet
    RsvpCreateFlowSpec(resvMessage, packetSize, flowSpec);

    // create and add FILTSS (RSVP_FILTER_SPEC, RSVP_LABEL_OBJECT,
    // RSVP_RECORD_ROUTE)
    RsvpCreateFiltSS(
        node,
        resvMessage,
        packetSize,
        filtSSList,
        (RsvpSessionAttributeFlags) flags,
        ipAddr,
        nextHop,
        phopAddr);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateErrorSpec
// PURPOSE      The function creates a RSVP_ERROR_SPEC object
//
// Return:              None
// Parameters:
//     hello:           RSVP_ERROR_SPEC object pointer
//     errorNodeAddr:   node address where error occurs
//     errorFlag:       error flag
//     errorCode:       error code
//     errorValue:      error value
//-------------------------------------------------------------------------
static
void RsvpCreateErrorSpec(
    RsvpObjectErrorSpec* errorSpec,
    NodeAddress errorNodeAddr,
    unsigned char errorFlag,
    unsigned char errorCode,
    unsigned short errorValue)
{
    RsvpCreateObjectHeader(
        &errorSpec->objectHeader,
        sizeof(RsvpObjectErrorSpec),
        RSVP_ERROR_SPEC,
        1);

    errorSpec->errorNodeAddress = errorNodeAddr;
    errorSpec->errorFlag = errorFlag;
    errorSpec->errorCode = errorCode;
    errorSpec->errorValue = errorValue;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateAndSendResvErrMessage
// PURPOSE      The function creates and send ResvErr message
//
// Return:              None
// Parameters:
//      node:           node which received the message
//      cPSB:           current PSB
//      errorCode:      error code for PathErr
//      errorValue:     error value
//-------------------------------------------------------------------------
static
void RsvpCreateAndSendResvErrMessage(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects,
    RsvpErrorCode errorCode,
    unsigned short errorValue)
{
    Message* resvErrMsg = NULL;
    RsvpResvErrMessage* resvErrMessage = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    unsigned short packetSize = sizeof(RsvpResvErrMessage);

    int interfaceId = ((NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg))->incomingInterfaceIndex;

    NodeAddress ipAddr = NetworkIpGetInterfaceAddress(node, interfaceId);

    // counter of ResvErr message sent incremented
    rsvp->rsvpStat->numResvErrMsgSent++;

    // consider RSVP_ADSPEC also if present in Path message
    if (allObjects->scope)
    {
        packetSize = (short) (packetSize +
            allObjects->scope->objectHeader.objectLength);
    }

    // create the ResvErr message
    resvErrMessage = (RsvpResvErrMessage*) MEM_malloc(packetSize);

    // crete common header
    RsvpCreateCommonHeader(
        (RsvpCommonHeader*) resvErrMessage,
        RSVP_RESV_ERR_MESSAGE,
        1,
        packetSize);

    // RSVP_SESSION_OBJECT object copied from all object structure
    memcpy(&resvErrMessage->session,
           allObjects->session,
           sizeof(RsvpTeObjectSession));

    // create RSVP_HOP object
    resvErrMessage->rsvpHop = RsvpCreateRsvpHop(ipAddr, interfaceId);

    // create RSVP_ERROR_SPEC object with the error
    RsvpCreateErrorSpec(
        &resvErrMessage->errorSpec,
        ipAddr,
        0,
        (unsigned char) errorCode,
        errorValue);

    memcpy(&resvErrMessage->style,
           allObjects->style,
           sizeof(RsvpObjectStyle));

    // copy RSVP_SCOPE object if available
    if (allObjects->scope)
    {
        memcpy(resvErrMessage + 1,
               allObjects->scope,
               sizeof(RsvpObjectScope));
    }

    switch
        (RsvpObjectStyleGetStyleBits(allObjects->style->rsvpStyleBits))
    {
        case RSVP_FF_STYLE:
        {
            // add each RSVP_FLOWSPEC and FiltSS
            LinkedList* filtSS = NULL;
            ListItem* item = NULL;
            RsvpFFDescriptor* ffDescriptor = NULL;

            ERROR_Assert(allObjects->FlowDescriptorList.ffDescriptor,
                         "Invalid flow descriptor");

            item = allObjects->FlowDescriptorList.ffDescriptor->first;

            while (item)
            {
                RsvpFiltSS* tempRsvpFiltSS = (RsvpFiltSS*)
                    MEM_malloc(sizeof(RsvpFiltSS));

                ffDescriptor = (RsvpFFDescriptor*) item->data;

                ERROR_Assert(ffDescriptor, "Invalid flow descriptor");

                memcpy(tempRsvpFiltSS, ffDescriptor->filtSS, sizeof(RsvpFiltSS));

                ListInit(node, &filtSS);

                ListInsert(node,
                    filtSS,
                    node->getNodeTime(),
                    tempRsvpFiltSS);

                RsvpCreateFlowDescriptor(
                    node,
                    (void**) &resvErrMessage,
                    &packetSize,
                    ffDescriptor->flowSpec,
                    filtSS,
                    (RsvpSessionAttributeFlags) 0,
                    ipAddr,
                    ipAddr,
                    0); // phopAddr not required

                ListFree(node, filtSS, FALSE);
                item = item->next;
            }
            break;
        }

        case RSVP_SE_STYLE:
        {
            // there should be one RSVP_FLOWSPEC and list of FiltSS

            RsvpSEDescriptor* seDescriptor = (RsvpSEDescriptor*)
                allObjects->FlowDescriptorList.seDescriptor;

            ERROR_Assert(seDescriptor, "Invalid Flow descriptor");

            RsvpCreateFlowDescriptor(
                node,
                (void**) &resvErrMessage,
                &packetSize,
                seDescriptor->flowSpec,
                seDescriptor->filtSS,
                (RsvpSessionAttributeFlags) 0,
                ipAddr,
                ipAddr,
                0); // phopAddr not required here

            break;
        }

        case RSVP_WF_STYLE:
        {
            // only one RSVP_FLOWSPEC should be there
            RsvpCreateFlowSpec(
                (void**) &resvErrMessage,
                &packetSize,
                allObjects->FlowDescriptorList.flowSpec);

            break;
        }

        default:
        {
            // invalid style
            ERROR_Assert(FALSE, "Invalid Style in Resv message");
            break;
        }
    }

    resvErrMsg = MESSAGE_Alloc(node,
                               TRANSPORT_LAYER,
                               TransportProtocol_RSVP,
                               MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, resvErrMsg, packetSize, TRACE_RSVP);
    memcpy(MESSAGE_ReturnPacket(resvErrMsg), resvErrMessage, packetSize);
    MEM_free(resvErrMessage);

    ERROR_Assert(interfaceId >= 0, "Invalid interface");

    // send the ResvErr message to the next HOP
    NetworkIpSendRawMessageToMacLayer(
        node,
        resvErrMsg,
        ipAddr,
        allObjects->rsvpHop->nextPrevHop,  // previous hop is destination
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_RSVP,
        1,                                 // TTL = 1, ResvErr goes one hop
        interfaceId,
        allObjects->rsvpHop->nextPrevHop); // next hop for destination
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessLoopDetection
// PURPOSE      The function processes the RSVP_RECORD_ROUTE object from the
//              Path or
//              Resv message and detect any loop in the path.
//
// Return:              TRUE if loop detected, else FALSE
// Parameters:
//     node:            node which received the message
//     recordRoute:     RSVP_RECORD_ROUTE object
//     interfaceList:   list of output interface
//-------------------------------------------------------------------------
static
BOOL RsvpProcessLoopDetection(
    Node* node,
    void* recordRoute)
{
    int length = sizeof(RsvpObjectHeader);
    RsvpStatistics* rsvpStat = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    rsvpStat = rsvp->rsvpStat;

    RsvpTeSubObject* subObject = NULL;

    // handle if label recording flag is set in RSVP_SESSION_ATTRIBUTE
    if (((RsvpTeObjectRecordRoute*) recordRoute)
        ->subObject.TypeField.recordType == 3)  // labeled record route
    {
        RsvpTeObjectRecordRouteLabel* recordRouteLabel =
            (RsvpTeObjectRecordRouteLabel*) recordRoute;

        ERROR_Assert(recordRouteLabel, "RSVP_RECORD_ROUTE object"
                     " not present");

        subObject = &(recordRouteLabel->subObject);

        while (length < recordRouteLabel->objectHeader.objectLength)
        {
            NodeAddress ipAddress;

            ERROR_Assert(subObject, "Invalid Sub object in"
                         " RSVP_RECORD_ROUTE");

            // form 4 byte IP address from two parts in sub object
            ipAddress = (subObject->ipAddress1 << 16)
                        + subObject->ipAddress2;

            // if current IP address matches with any one of the
            // interface then loop is detected
            if (NetworkIpIsMyIP(node, ipAddress))
            {
                // identify loop
                char errorBuf[MAX_STRING_LENGTH];

                sprintf(errorBuf, "Loop detected at node %d while"
                        " creating LSP\n", node->nodeId);

                ERROR_ReportWarning(errorBuf);
                return TRUE;
            }
            length += (subObject->length
                       + recordRouteLabel->recordLabel.length);

            subObject = (RsvpTeSubObject*) ((char*) subObject
                + (subObject->length
                + recordRouteLabel->recordLabel.length));
        }
    }
    else
    {
        RsvpTeObjectRecordRoute* recordRouteNormal =
            (RsvpTeObjectRecordRoute*) recordRoute;

        ERROR_Assert(recordRouteNormal, "RSVP_RECORD_ROUTE"
                     " object not present");

        subObject = &(recordRouteNormal->subObject);

        while (length < recordRouteNormal->objectHeader.objectLength)
        {
            NodeAddress ipAddress;

            ERROR_Assert(subObject, "Invalid Sub object in"
                         " RSVP_RECORD_ROUTE");

            // form 4 byte IP address from two parts in sub object
            ipAddress = (subObject->ipAddress1 << 16)
                        + subObject->ipAddress2;

            // if current IP address matches with any one of the
            // interface then loop is detected
            if (NetworkIpIsMyIP(node, ipAddress))
            {
                rsvpStat->numLoopsDetected++;
                if (RSVP_DEBUG)
                {
                // identify loop
                char errorBuf[MAX_STRING_LENGTH];

                sprintf(errorBuf, "Loop detected at node %d while "
                        "creating LSP\n", node->nodeId);

                ERROR_ReportWarning(errorBuf);

                }

                return TRUE;
            }
            length += subObject->length;

            // next sub object in the record route
            subObject += 1;
        }
    }
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCopyFilterSpec
// PURPOSE      Add FiltSS in the filter spec list of RSB
//
// Return:              None
// Parameters:
//     node:            node which received the message
//     filterSpecList:  list of filtSS
//     filtSS:          filtSS to be inserted in RSB
//-------------------------------------------------------------------------
static
BOOL RsvpCopyFilterSpec(
    Node* node,
    Message* msg,
    RsvpResvStateBlock* rsb,
    RsvpFiltSS* filtSS,
    RsvpAllObjects* allObjects)
{
    // allocate memory for new filtSS
    RsvpFiltSS* newFiltSS = (RsvpFiltSS*) MEM_malloc(sizeof(RsvpFiltSS));
    memset(newFiltSS, 0, sizeof(RsvpFiltSS));

    ERROR_Assert(allObjects, "RSVP message is not parsed");
    ERROR_Assert(filtSS, "Invalid Filter spec");

    newFiltSS->filterSpec = (RsvpTeObjectFilterSpec*)
        MEM_malloc(sizeof(RsvpTeObjectFilterSpec));

    memcpy(newFiltSS->filterSpec,
           filtSS->filterSpec,
           sizeof(RsvpTeObjectFilterSpec));

    if (filtSS->label)
    {
        newFiltSS->label = (RsvpTeObjectLabel*)
            MEM_malloc(sizeof(RsvpTeObjectLabel));

        memcpy(newFiltSS->label,
               filtSS->label,
               sizeof(RsvpTeObjectLabel));

        if (!RsvpSearchLSP(node,
                           filtSS->filterSpec->lspId,
                           filtSS->label->label,
                           rsb->session.receiverAddress))
        {
            RsvpCreateLSP(
                node,
                filtSS->filterSpec->senderAddress,
                filtSS->filterSpec->lspId,
                filtSS->label->label,
                rsb->session.receiverAddress,
                rsb->nextHopAddress);
        }
    }

    if (filtSS->recordRoute)
    {
        // Process loop detection
        if (RsvpProcessLoopDetection(
                node,
                filtSS->recordRoute))
        {
            // loop detected send ResvErr
            RsvpCreateAndSendResvErrMessage(
                node,
                msg,
                allObjects,
                RSVP_ROUTING_PROBLEM,
                RSVP_TE_RRO_INDICATED_ROUTING_LOOPS);

            return FALSE;
        }

        if ((allObjects->sessionAttr) &&
            (allObjects->sessionAttr->flags == RSVP_LABEL_RECORDING_DESIRED))
        {
            newFiltSS->recordRoute = (RsvpTeObjectRecordRouteLabel*)
                MEM_malloc(((RsvpObjectHeader*)
                            filtSS->recordRoute)->objectLength);

            memcpy(newFiltSS->recordRoute,
                   filtSS->recordRoute,
                   ((RsvpObjectHeader*) filtSS->recordRoute)->objectLength);
        }
        else
        {
            newFiltSS->recordRoute = (RsvpTeObjectRecordRoute*)
                MEM_malloc(((RsvpObjectHeader*)
                    filtSS->recordRoute)->objectLength);

            memcpy(newFiltSS->recordRoute,
                   filtSS->recordRoute,
                   ((RsvpObjectHeader*)
                       filtSS->recordRoute)->objectLength);
        }
    }

    // insert in RSB filter spec list
    ListInsert(node, rsb->filtSSList, node->getNodeTime(), newFiltSS);
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpAddFilterSpecList
// PURPOSE      Add FiltSS in the filter spec list of RSB, This list consists
//              of RSVP_FILTER_SPEC, RSVP_LABEL_OBJECT and RSVP_RECORD_ROUTE
//
// Return:              None
// Parameters:
//     node:            node which received the message
//     msg:             message received by the layer
//     newRSB:          new RSB block for the entry
//     filterSpecList:  list of filtSS
//-------------------------------------------------------------------------
static
BOOL RsvpAddFilterSpecList(
    Node* node,
    Message* msg,
    RsvpResvStateBlock* newRSB,
    LinkedList* filterSpecList,
    RsvpAllObjects* allObjects)
{
    ListItem* item = NULL;

    ERROR_Assert(filterSpecList, "Invalid Filter spec");
    ERROR_Assert(newRSB, "Invalid RSB");

    // for each FiltSS in the current message
    item = filterSpecList->first;

    while (item)
    {
        RsvpFiltSS* filtSS = (RsvpFiltSS*) item->data;

        ERROR_Assert(filtSS, "Invalid Filter spec");

        // Copy each filtSS in the RSB entry
        if (!RsvpCopyFilterSpec(
                 node,
                 msg,
                 newRSB,
                 filtSS,
                 allObjects))
        {
            // error in copying the filtSS
            return FALSE;
        }
        item = item->next;
    }
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateRSBForReceiver
// PURPOSE      Create a new RSB for receiver node
//
// Return:              None
// Parameters:
//     node:            node which received the message
//     msg:             message received by the layer
//     allObjects:      all objects in the message
//     cPSB:            current PSB
//-------------------------------------------------------------------------
static
void RsvpCreateRSBForReceiver(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects,
    RsvpPathStateBlock* cPSB)
{
    LinkedList* rsbList = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    ERROR_Assert(cPSB, "Invalid Path state block");

    rsbList = RsvpSearchRSB(node, cPSB);

    ERROR_Assert(rsbList, "Invalid Resv state list");

    if (!rsbList->size)
    {
        RsvpObjectStyle style;
        LinkedList* filterSpecList = NULL;
        RsvpFiltSS* filtSS = NULL;
        ListItem *listItm = NULL;

        // Create a new RSB and initialise
        RsvpResvStateBlock* newRSB = (RsvpResvStateBlock*)
                MEM_malloc(sizeof(RsvpResvStateBlock));
        memset(newRSB, 0, sizeof(RsvpResvStateBlock));

        // Set the session, NHOP, OI and style of the RSB from the message.
        memcpy(&newRSB->session,
               &cPSB->session,
               sizeof(RsvpTeObjectSession));

        newRSB->outgoingLogicalInterface = ANY_INTERFACE;

        style = RsvpCreateStyle(node);
        memcpy(&newRSB->style, &style, sizeof(RsvpObjectStyle));

        // Copy FiltSS list into the RSB.
        filtSS = (RsvpFiltSS*) MEM_malloc(sizeof(RsvpFiltSS));

        filtSS->filterSpec
            = (RsvpTeObjectFilterSpec *)
              MEM_malloc(sizeof(RsvpTeObjectFilterSpec));

        memcpy(filtSS->filterSpec,
               &cPSB->senderTemplate,
               sizeof(RsvpTeObjectFilterSpec));

        RsvpCreateObjectHeader(
            &filtSS->filterSpec->objectHeader,
            sizeof(RsvpTeObjectFilterSpec),
            (unsigned char) RSVP_FILTER_SPEC,
            1);

        filtSS->label = (RsvpTeObjectLabel*)
            MEM_malloc(sizeof(RsvpTeObjectLabel));

        RsvpCreateObjectHeader(
            &filtSS->label->objectHeader,
            sizeof(RsvpTeObjectLabel),
            (unsigned char) RSVP_LABEL_OBJECT,
            1);

        filtSS->label->label = 0;

        filtSS->recordRoute = NULL;

        ListInit(node, &filterSpecList);
        ListInsert(node, filterSpecList, node->getNodeTime(), filtSS);

        // Initialize the filter spec list entry
        ListInit(node, &newRSB->filtSSList);

        RsvpAddFilterSpecList(
            node,
            msg,
            newRSB,
            filterSpecList,
            allObjects);

        // this loop frees filtSS->label and filtSS->filterSpec
        listItm = filterSpecList->first;
        while (listItm)
        {
            MEM_free(((RsvpFiltSS*) listItm->data)->filterSpec);
            MEM_free(((RsvpFiltSS*) listItm->data)->label);
            listItm = listItm->next;
        }

        ListFree(node, filterSpecList, FALSE);

        // Copy RSVP_FLOWSPEC and any RSVP_SCOPE object from the message
        // into the RSB.
        memcpy(&newRSB->flowSpec,
               &cPSB->senderTSpec,
               sizeof(RsvpObjectSenderTSpec));

        RsvpCreateObjectHeader(
            &newRSB->flowSpec.objectHeader,
            sizeof(RsvpObjectFlowSpec),
            (unsigned char) RSVP_FLOWSPEC,
            1);

        // Insert it to the RSB list and return created RSB
        ListInsert(node, rsvp->rsb, node->getNodeTime(), newRSB);
    }
    ListFree(node, rsbList, FALSE);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateTimeValues
// PURPOSE      The function creates a new RSVP_TIME_VALUES object.
//
// Return:      New RSVP_TIME_VALUES object
// Parameters:
//     node:            node which received the message
//     lastTimeValue:   last time value sent
//-------------------------------------------------------------------------
static
RsvpObjectTimeValues RsvpCreateTimeValues(
    Node* node,
    unsigned lastTimeValue,
    RsvpObjectTimeValues* timeValuesPtr)
{
    RsvpObjectTimeValues timeValues;
    unsigned timeVal;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    RsvpCreateObjectHeader(
        &timeValues.objectHeader,
        sizeof(RsvpObjectTimeValues),
        (unsigned char) RSVP_TIME_VALUES,
        1);

    // prepare the new time value for forwarding message
    timeVal = (unsigned) (RANDOM_erand(rsvp->seed) * RSVP_STATE_REFRESH_DELAY);

    timeVal = (unsigned) ((float) timeVal * ((RSVP_MAX_STATE_REFRESH_DELAY)
                  / (RSVP_MIN_STATE_REFRESH_DELAY)));

    while (lastTimeValue && ((timeVal / lastTimeValue) >= 1.3))
    {
        timeVal /= 2;
    }
    timeValues.refreshPeriod = timeVal;
    memcpy(timeValuesPtr, &timeValues, sizeof(RsvpObjectTimeValues));
    return timeValues;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreatePathMessage
// PURPOSE      The function creates a Path message
//
// Return:              Pointer to Path message
// Parameters:
//      ttl:            TTL
//      session:        RSVP_SESSION_OBJECT object pointer
//      rsvpHop:        RSVP_HOP object pointer
//      timeValues:     RSVP_TIME_VALUES object pointer
//      explicitRoute:  RSVP_EXPLICIT_ROUTE object pointer
//      labelRequest:   RSVP_LABEL_REQUEST object pointer
//      sessionAttr:    SESSION_AATRIBUTE object pointer
//      senderTemplate: RSVP_SENDER_TEMPLATE object pointer
//      senderTSpec:    RSVP_SENDER_TSPEC object pointer
//      adSpec:         RSVP_ADSPEC object pointer
//      returnPacketSize:   returned packet size
//-------------------------------------------------------------------------
static
RsvpPathMessage* RsvpCreatePathMessage(
    unsigned char ttl,
    RsvpTeObjectSession* session,
    RsvpObjectRsvpHop* rsvpHop,
    RsvpObjectTimeValues* timeValues,
    RsvpTeObjectExplicitRoute* explicitRoute,
    RsvpTeObjectLabelRequest* labelRequest,
    RsvpTeObjectSessionAttribute* sessionAttr,
    RsvpTeObjectSenderTemplate* senderTemplate,
    RsvpObjectSenderTSpec* senderTSpec,
    RsvpObjectAdSpec* adSpec,
    unsigned short* returnPacketSize)
{
    RsvpPathMessage* pathMessage = NULL;
    RsvpTeObjectSession* sessionPtr = NULL;
    RsvpObjectRsvpHop* rsvpHopPtr = NULL;
    RsvpObjectTimeValues* timeValuesPtr = NULL;
    RsvpTeObjectExplicitRoute* explicitRoutePtr = NULL;
    RsvpTeObjectLabelRequest* labelRequestPtr = NULL;
    char* senderTemplatePtr = NULL;
    char* senderTSpecPtr = NULL;
    RsvpTeObjectSessionAttribute* sessionAttrPtr = NULL;
    RsvpObjectAdSpec* adSpecPtr = NULL;

    // size of compulsory object in path message
    unsigned short packetSize = sizeof(RsvpPathMessage);

    ERROR_Assert(session, "RSVP_SESSION_OBJECT"
                 " object must be present in PathMsg");

    ERROR_Assert(timeValues, "RSVP_TIME_VALUES"
                 " object must be present in PathMsg");

    ERROR_Assert(senderTemplate, "RSVP_SENDER_TEMPLATE object must be"
                 " present in PathMsg");

    ERROR_Assert(senderTSpec, "RSVP_SENDER_TSPEC object must"
                 " be present in PathMsg");

    // consider size fo optional objects also
    if (explicitRoute != NULL)
    {
        packetSize = (short) (packetSize
            + explicitRoute->objectHeader.objectLength);
    }

    if (labelRequest != NULL)
    {
        packetSize = (short) (packetSize
            + labelRequest->objectHeader.objectLength);
    }

    if (sessionAttr != NULL)
    {
        packetSize = (short) (packetSize
            + sessionAttr->objectHeader.objectLength);
    }

    if (adSpec != NULL)
    {
        packetSize = (short) (packetSize
            + adSpec->objectHeader.objectLength);
    }

    // allocate total Path message packet
    pathMessage = (RsvpPathMessage*) MEM_malloc(packetSize);

    // add common header
    RsvpCreateCommonHeader(
        (RsvpCommonHeader*) pathMessage,
        RSVP_PATH_MESSAGE,
        ttl,
        packetSize);

    // copy RSVP_SESSION_OBJECT object to the packet area
    sessionPtr = (RsvpTeObjectSession*)((RsvpCommonHeader*) pathMessage + 1);

    memcpy(sessionPtr, session, sizeof(RsvpTeObjectSession));

    // copy RSVP_HOP object to the packet area
    rsvpHopPtr = (RsvpObjectRsvpHop*) (sessionPtr + 1);

    if (rsvpHop != NULL)
    {
        memcpy(rsvpHopPtr, rsvpHop, sizeof(RsvpObjectRsvpHop));
    }

    // copy RSVP_TIME_VALUES object to the packet area
    timeValuesPtr = (RsvpObjectTimeValues*) (rsvpHopPtr + 1);
    memcpy(timeValuesPtr, timeValues, sizeof(RsvpObjectTimeValues));

    // copy RSVP_EXPLICIT_ROUTE object to the packet area
    explicitRoutePtr = (RsvpTeObjectExplicitRoute*) (timeValuesPtr + 1);

    if (explicitRoute != NULL)
    {
        memcpy(explicitRoutePtr,
               explicitRoute,
               explicitRoute->objectHeader.objectLength);

        labelRequestPtr = (RsvpTeObjectLabelRequest*)
                          (((char*) explicitRoutePtr)
                            + explicitRoute->objectHeader.objectLength);
    }
    else
    {
        labelRequestPtr = (RsvpTeObjectLabelRequest*) explicitRoutePtr;
    }

    // copy LSBEL_REQUEST object to the packet area
    if (labelRequest != NULL)
    {
        memcpy(labelRequestPtr,
               labelRequest,
               labelRequest->objectHeader.objectLength);

        sessionAttrPtr = (RsvpTeObjectSessionAttribute*)
            (labelRequestPtr + 1);
    }
    else
    {
        sessionAttrPtr = (RsvpTeObjectSessionAttribute*) labelRequestPtr;
    }

    // copy SESSION_ATTRIBUTES object to the packet area
    if ((sessionAttr != NULL) && sessionAttr->objectHeader.objectLength)
    {
        memcpy(sessionAttrPtr,
               sessionAttr,
               sessionAttr->objectHeader.objectLength);

        senderTemplatePtr = (char*)(((char*)sessionAttrPtr)
                            + sessionAttr->objectHeader.objectLength);
    }
    else
    {
        senderTemplatePtr = (char*) sessionAttrPtr;
    }

    // copy RSVP_SENDER_TEMPLATE object to the packet area
    memcpy((char*) senderTemplatePtr,
           (char*) senderTemplate,
           sizeof(RsvpTeObjectSenderTemplate));

    // copy RSVP_SENDER_TSPEC object to the packet area
    senderTSpecPtr = (char*) (senderTemplatePtr +
                        sizeof(RsvpTeObjectSenderTemplate));

    memcpy((char*) senderTSpecPtr,
           (char*) senderTSpec,
           sizeof(RsvpObjectSenderTSpec));

    // copy RSVP_ADSPEC object to the packet area
    adSpecPtr = (RsvpObjectAdSpec*) (senderTSpecPtr +
                    sizeof(RsvpObjectSenderTSpec));

    if (adSpec != NULL)
    {
        memcpy(adSpecPtr, adSpec, adSpec->objectHeader.objectLength);
    }

    // RSVP_RECORD_ROUTE object will be added later when sending the
    // Path message

    *returnPacketSize = packetSize;
    return pathMessage;
}

static
void RsvpGetInterfaceAndNextHopForExplicitRoute(
    Node* node,
    RsvpTeSubObject* subObject,
    int* interfaceIndex,
    NodeAddress* nextHop)
{
    *nextHop = (subObject->ipAddress1 << 16) + subObject->ipAddress2;
    *interfaceIndex = NetworkIpGetInterfaceIndexForNextHop(node, *nextHop);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSendPathMessage
// PURPOSE      The function sends the Path message
//
// Return:                      None
// Parameters:
//      node:                   node which received the message
//      pathMessage:            path message to be sent
//      sourceAddress:          source address
//      destninationAddress:    destination address
//      ttl:                    TTL
//-------------------------------------------------------------------------
static
void RsvpSendPathMessage(
    Node* node,
    RsvpPathMessage* pathMessage,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    RsvpTeObjectExplicitRoute* explicitRoute,
    unsigned char ttl)
{
    int packetLength = ((RsvpCommonHeader*) pathMessage)->rsvpLength;
    int interfaceIndex;
    NodeAddress nextHop;

    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    // create the path message
    Message* msg = MESSAGE_Alloc(node,
                                 TRANSPORT_LAYER,
                                 TransportProtocol_RSVP,
                                 MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, packetLength, TRACE_RSVP);

    memcpy(MESSAGE_ReturnPacket(msg), pathMessage, packetLength);

    // hop detection, special technique if RSVP_EXPLICIT_ROUTE is ON
    if (explicitRoute)
    {
        RsvpGetInterfaceAndNextHopForExplicitRoute(
            node,
            &(explicitRoute->subObject),
            &interfaceIndex,
            &nextHop);
    }
    else
    {
        NetworkGetInterfaceAndNextHopFromForwardingTable(
            node,
            destinationAddress,
            &interfaceIndex,
            &nextHop);

        if (RSVP_DEBUG)
        {
            char destAddress[MAX_IP_STRING_LENGTH] = {0};
            char nextHopAddr[MAX_IP_STRING_LENGTH] = {0};

            IO_ConvertIpAddressToString(destinationAddress, destAddress);
            IO_ConvertIpAddressToString(nextHop, nextHopAddr);

            printf("node %u Rsvp is sending path message:\n"
                   "destnation address = %s\n"
                   "next hop address   = %s\n"
                   "interface index = %d\n"
                   "sizeof path msg = %u\n\n",
                   node->nodeId,
                   destAddress,
                   nextHopAddr,
                   interfaceIndex,
                   MESSAGE_ReturnPacketSize(msg));
        }
    }

    if ((interfaceIndex == ANY_INTERFACE) ||
        (interfaceIndex == CPU_INTERFACE))
    {
        // routing table is still not ready.
        // Or routing protocol is still not stabilized.
        // So discard the path message.
        MESSAGE_Free(node, msg);
        return;
    }

    ERROR_Assert(interfaceIndex >= 0, "Invalid interface");

    rsvp->rsvpStat->numPathMsgSent++;

    if (nextHop == 0)
    {
        // next hop "0" means destination address itself
        // is the next hop
        nextHop = destinationAddress;
    }

    // send the path message to the next address
    NetworkIpSendRawMessageToMacLayer(
        node,
        msg,
        sourceAddress,
        nextHop,       // destination address is "nexthop" itself
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_RSVP,
        (int) ttl,
        interfaceIndex,
        nextHop);      // next hop address for destination
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpEnvokePathRefresh
// PURPOSE      The function processes the Path refresh event
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     msg:     message received by the layer
//     rPSB:    refreshing PSB
//-------------------------------------------------------------------------
static
void RsvpEnvokePathRefresh(
    Node* node,
    RsvpPathStateBlock* rPSB)
{
    Message* refreshPathMsg = NULL;
    Message* refreshHelloMsg = NULL;
    RsvpPathMessage* pathMessage = NULL;
    RsvpPathMessage* pathMessageToSend = NULL;
    RsvpObjectTimeValues timeValues;

    unsigned delayTime = 0;
    unsigned short returnPacketSize = 0;
    unsigned short packetSizeToSend = 0;
    IntListItem* item = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    RsvpTimerStore* timerStore;

    if (RSVP_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH] = {0};
        ctoa(node->getNodeTime(), clockStr);
        printf("PathRefresh Message received by %d sim-time %s\n",
            node->nodeId, clockStr);
    }

    if (!rPSB)
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr, "Invalid PSB in %d", node->nodeId);
        ERROR_ReportError(errorStr);
    }

    if (rPSB->remainingTtl == 1)
    {
        // For next forwarding TTL will be 0, return without sending Message
        return;
    }

    // create new RSVP_TIME_VALUES object for message forwarding
    RsvpCreateTimeValues(node, rPSB->lastTimeValues, &timeValues);

    rPSB->lastTimeValues = timeValues.refreshPeriod;

    // prepare all the objects and call for the creation of path message
    pathMessage = RsvpCreatePathMessage(
                      rPSB->remainingTtl,
                      &rPSB->session,
                      NULL,
                      &timeValues,
                      rPSB->modifiedExplicitRoute,
                      &rPSB->labelRequest,
                      rPSB->sessionAttr,
                      &rPSB->senderTemplate,
                      &rPSB->senderTSpec,
                      &rPSB->adSpec,
                      &returnPacketSize);

    ERROR_Assert(rPSB->outInterfaceList, "Invalid interface");

    // The message has to be sent for each interface
    item = rPSB->outInterfaceList->first;
    while (item)
    {
        int interfaceId = item->data;
        NodeAddress ipAddr = NetworkIpGetInterfaceAddress(node, interfaceId);

        // pathMessage copied to another area and add RSVP_RECORD_ROUTE
        // object
        packetSizeToSend = returnPacketSize;
        pathMessageToSend = (RsvpPathMessage*) MEM_malloc(packetSizeToSend);
        memcpy(pathMessageToSend, pathMessage, packetSizeToSend);

        // create RSVP_HOP
        pathMessageToSend->rsvpHop = RsvpCreateRsvpHop(
                                         ipAddr,
                                         interfaceId);

        if (rsvp->recordRouteType != RSVP_RECORD_ROUTE_NONE)
        {
            // rsvp->recordRouteType != RSVP_RECORD_ROUTE_NONE
            // so create RSVP_RECORD_ROUTE object
            RsvpCreateRecordRoute(
                rPSB->recordRoute,
                (RsvpSessionAttributeFlags) 0,
                ipAddr,
                0,
                0,
                (void**) &pathMessageToSend,
                &packetSizeToSend);
        }

        // send Path message
        RsvpSendPathMessage(
            node,
            pathMessageToSend,
            ipAddr,
            rPSB->session.receiverAddress,
            rPSB->modifiedExplicitRoute,
            (unsigned char) (rPSB->remainingTtl - 1));

        MEM_free(pathMessageToSend);
        item = item->next;
    }

    // path message is sent to all interfaces. now remove it
    MEM_free(pathMessage);

    RsvpRemoveFromTimerStoreList(node, rPSB);
    // Re-send PathRefresh with proper delay for next trigger
    delayTime = (unsigned) ((RSVP_REFRESH_TIME_CONSTANT_K + 0.5)
            * 1.5 * timeValues.refreshPeriod);

    refreshPathMsg = MESSAGE_Alloc(node,
                                   TRANSPORT_LAYER,
                                   TransportProtocol_RSVP,
                                   MSG_TRANSPORT_RSVP_PathRefresh);


    timerStore = (RsvpTimerStore*) MEM_malloc(sizeof(RsvpTimerStore));
    timerStore->messagePtr = refreshPathMsg;
    timerStore->psb = rPSB;

    ListInsert(node, rsvp->timerStoreList, node->getNodeTime(), timerStore);

    // send current PSB pointer in the info field
    MESSAGE_InfoAlloc(node, refreshPathMsg, sizeof(RsvpTimerStore));
    memcpy(MESSAGE_ReturnInfo(refreshPathMsg),
           timerStore,
           sizeof(RsvpTimerStore));

    MESSAGE_Send(node, refreshPathMsg, delayTime * MILLI_SECOND);

    if (RSVP_DEBUG)
    {
        printf("node %d scheduling path refresh with delay %u\n",
              node->nodeId,
              delayTime);
    }

    // Set the timer MSG_TRANSPORT_RSVP_HelloExtension for hello adjacency
    refreshHelloMsg = MESSAGE_Alloc(node,
                                    TRANSPORT_LAYER,
                                    TransportProtocol_RSVP,
                                    MSG_TRANSPORT_RSVP_HelloExtension);

    // send current PSB pointer in the info field
    MESSAGE_Send(node, refreshHelloMsg, RSVP_HELLO_TIMER_DELAY);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchRSBForSessionOnly
// PURPOSE      Search in the RSB for the current session only
//
// Return:          Matching RSB, NULL if no matching
// Parameters:
//     node:        node which received the message
//     allObejcts:  all objects from the message
//-------------------------------------------------------------------------
static
LinkedList* RsvpSearchRSBForSessionOnly(
    Node* node,
    RsvpTeObjectSession* session)
{
    RsvpResvStateBlock* rsb = NULL;
    ListItem* item = NULL;
    LinkedList* rsbList = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    ERROR_Assert(rsvp->rsb, "Invalid Resv state block");

    ListInit(node, &rsbList);
    item = rsvp->rsb->first;

    // Go through the PSB list
    while (item)
    {
        // get a PSB from the list
        rsb = (RsvpResvStateBlock*) item->data;

        ERROR_Assert(rsb, "Invalid Resv state block");

        // check the session object
        if ((rsb->session.receiverAddress == session->receiverAddress)
            && (rsb->session.tunnelId == session->tunnelId))
        {
            RsvpResvStateBlock* newRSB =
                (RsvpResvStateBlock*)
                MEM_malloc(sizeof(RsvpResvStateBlock));

            memcpy(newRSB, rsb, sizeof(RsvpResvStateBlock));
            ListInsert(node, rsbList, node->getNodeTime(), newRSB);
        }
        item = item->next;
    }
    return rsbList;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateResvMessage
// PURPOSE      The function creates a Resv message
//
// Return:              Pointer to Resv message
// Parameters:
//      node:           node which received the message
//      session:        RSVP_SESSION_OBJECT object pointer
//      rsvpHop:        RSVP_HOP object pointer
//      timeValues:     RSVP_TIME_VALUES object pointer
//      resvConf:       RESV_CONFIRM object pointer
//      scope:          RSVP_SCOPE object pointer
//      style:          RSVP_STYLE object pointer
//      returnPacketSize: returned packet size
//-------------------------------------------------------------------------
static
RsvpResvMessage* RsvpCreateResvMessage(
    RsvpTeObjectSession* session,
    RsvpObjectRsvpHop* rsvpHop,
    RsvpObjectTimeValues* timeValues,
    RsvpObjectResvConfirm* resvConf,
    RsvpObjectScope* scope,
    RsvpObjectStyle* style,
    unsigned short* returnPacketSize)
{
    RsvpResvMessage* resvMessage = NULL;
    RsvpTeObjectSession* sessionPtr = NULL;
    RsvpObjectRsvpHop* rsvpHopPtr = NULL;
    RsvpObjectTimeValues* timeValuesPtr = NULL;
    RsvpObjectResvConfirm* resvConfPtr = NULL;
    RsvpObjectScope* scopePtr = NULL;
    RsvpObjectStyle* stylePtr = NULL;

    // size of the compulsory object in Resv message
    unsigned short packetSize = sizeof(RsvpResvMessage);

    ERROR_Assert(session, "RSVP_SESSION_OBJECT object"
                 " must be present in ResvMsg");

    ERROR_Assert(timeValues, "RSVP_TIME_VALUES object"
                 " must be present in ResvMsg");

    ERROR_Assert(rsvpHop, "RSVP_HOP object must be present in ResvMsg");
    ERROR_Assert(style, "RSVP_STYLE object must be present in ResvMsg");

    // consider optional object size also
    if (resvConf != NULL)
    {
        packetSize = (short) (packetSize
            + resvConf->objectHeader.objectLength);
    }

    if (scope != NULL)
    {
        packetSize = (short) (packetSize
            + scope->objectHeader.objectLength);
    }

    // allocate total Resv message size
    resvMessage = (RsvpResvMessage*) MEM_malloc(packetSize);
    memset(resvMessage, 0, packetSize);

    // add common header
    RsvpCreateCommonHeader(
        (RsvpCommonHeader*) resvMessage,
        RSVP_RESV_MESSAGE,
        1,
        packetSize);

    // add RSVP_SESSION_OBJECT object
    sessionPtr = (RsvpTeObjectSession*)
            ((RsvpCommonHeader*) resvMessage + 1);
    memcpy(sessionPtr, session, sizeof(RsvpTeObjectSession));

    // add RSVP_HOP object
    rsvpHopPtr = (RsvpObjectRsvpHop*) (sessionPtr + 1);
    memcpy(rsvpHopPtr, rsvpHop, sizeof(RsvpObjectRsvpHop));

    // add RSVP_TIME_VALUES object
    timeValuesPtr = (RsvpObjectTimeValues*) (rsvpHopPtr + 1);
    memcpy(timeValuesPtr, timeValues, sizeof(RsvpObjectTimeValues));

    // add RESV_CONFIRM object
    resvConfPtr = (RsvpObjectResvConfirm*) (timeValuesPtr + 1);

    if (resvConf->objectHeader.objectLength > 0)
    {
        memcpy(resvConfPtr, resvConf, resvConf->objectHeader.objectLength);
        scopePtr = (RsvpObjectScope*) (resvConfPtr + 1);
    }
    else
    {
        scopePtr = (RsvpObjectScope*) resvConfPtr;
    }

    // add scope object
    if (scope->objectHeader.objectLength > 0)
    {
        memcpy(scopePtr, scope, scope->objectHeader.objectLength);
        stylePtr = (RsvpObjectStyle*) (scopePtr + 1);
    }
    else
    {
        stylePtr = (RsvpObjectStyle*) scopePtr;
    }

    // add style object
    memcpy(stylePtr, style, style->objectHeader.objectLength);

    *returnPacketSize = packetSize;
    return resvMessage;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSendResvMessage
// PURPOSE      The function sends the Resv message
//
// Return:                      None
// Parameters:
//      node:                   node which received the message
//      pathMessage:            resv message to be sent
//      sourceAddress:          source address
//      destninationAddress:    destination address
//      interfaceIndex:         interface id
//-------------------------------------------------------------------------
static
void RsvpSendResvMessage(
    Node* node,
    RsvpResvMessage* resvMessage,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceIndex)
{
    int packetLength = ((RsvpCommonHeader*) resvMessage)->rsvpLength;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    // create the path message
    Message* msg = MESSAGE_Alloc(node,
                                 TRANSPORT_LAYER,
                                 TransportProtocol_RSVP,
                                 MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, packetLength, TRACE_RSVP);
    memcpy(MESSAGE_ReturnPacket(msg), resvMessage, packetLength);
    rsvp->rsvpStat->numResvMsgSent++;

    ERROR_Assert(interfaceIndex >= 0, "Invalid interface");

    if (RSVP_DEBUG)
    {
        char destAddr[MAX_STRING_LENGTH] = {0};
        IO_ConvertIpAddressToString(destinationAddress, destAddr);

        printf("node %d is sending is resv message to %s\n",
            node->nodeId,
            destAddr);
    }

    // send the Resv message to the destination address
    NetworkIpSendRawMessageToMacLayer(
        node,
        msg,
        sourceAddress,
        destinationAddress,  // destination address of Resv message
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_RSVP,
        1,                   // 1 == TTL, resv message reaches nexthop only
        interfaceIndex,
        destinationAddress); // destination address is itself the nextHop
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpEnvokeResvRefresh
// PURPOSE      The function processes the Resv refresh event
// ASSUMPTION   This function is simplified without considering the merging
//              of RSVP_FLOWSPEC. So working with the structure TCSB has
//              been ignored here. Only proper RSBs are collected for the
//              previous HOP and a new RSVP_RESV_MESSAGE is send it to that
//              PHOP. For resource reservation in LSP the above part should
//              be implemented
//
// Return:      None
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     phopAddr:    refreshing PHOP
//-------------------------------------------------------------------------
static
void RsvpEnvokeResvRefresh(
    Node* node,
    NodeAddress phopAddr)
{
    Message* refreshResvMsg = NULL;
    Message* refreshHelloMsg = NULL;
    RsvpResvMessage* resvMessage = NULL;
    RsvpObjectTimeValues timeValues;
    RsvpStyleType resvStyle = (RsvpStyleType) 0;
    unsigned delayTime = 0;
    unsigned short packetSize = 0;
    ListItem* item = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    ERROR_Assert(rsvp->psb, "Invalid PSB");
    memset(&timeValues, 0, sizeof(RsvpObjectTimeValues));

    if (RSVP_DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH] = {0};
        ctoa(node->getNodeTime(), clockStr);
        printf("ResvRefresh Message received by %d sim-time %s\n",
            node->nodeId, clockStr);
    }

    item = rsvp->psb->first;

    // go for each PSB with matching PHOP
    while (item)
    {
        RsvpPathStateBlock* psb = (RsvpPathStateBlock*) item->data;

        ERROR_Assert(psb, "Invalid PSB");

        if (psb->rsvpHop.nextPrevHop == phopAddr)
        {
            // List of RSBs for this PSB
            NodeAddress ipAddr;
            RsvpObjectRsvpHop rsvpHop;
            ListItem* rsbItem = NULL;
            BOOL isFlowSpecProcessed;

            LinkedList* rsbList = RsvpSearchRSBForSessionOnly(
                                node,
                                &psb->session);

            ERROR_Assert(rsbList, "Invalid RSB list");

            ipAddr = NetworkIpGetInterfaceAddress(
                         node,
                         psb->incomingInterface);

            // create RSVP_HOP
            rsvpHop = RsvpCreateRsvpHop(
                          ipAddr,
                          psb->incomingInterface);

            rsbItem = rsbList->first;

            resvMessage = NULL;
            packetSize = 0;
            isFlowSpecProcessed = FALSE;

            // collect flow descriptor for each RSB for different style
            while (rsbItem)
            {
                RsvpResvStateBlock* rRSB = (RsvpResvStateBlock*)
                    rsbItem->data;

                ERROR_Assert(rRSB, "Invalid RSB");

                // create RSVP_TIME_VALUES
                RsvpCreateTimeValues(
                    node,
                    rRSB->lastTimeValues,
                    &timeValues);

                rRSB->lastTimeValues = timeValues.refreshPeriod;

                if (resvMessage == NULL)
                {
                    // First RSB, create Resv message with compulsary object
                    resvMessage = RsvpCreateResvMessage(
                                      &rRSB->session,
                                      &rsvpHop,
                                      &timeValues,
                                      &rRSB->resvConf,
                                      &rRSB->scope,
                                      &rRSB->style,
                                      &packetSize);

                    resvStyle =(RsvpStyleType)RsvpObjectStyleGetStyleBits(
                        rRSB->style.rsvpStyleBits);
                }

                // resv style must match for each matching RSBs
                if (resvStyle != RsvpObjectStyleGetStyleBits(
                    rRSB->style.rsvpStyleBits))
                {
                    rsbItem = rsbItem->next;    // next RSB
                    continue;
                }

                // add flow descriptor considering different style
                switch
                    (RsvpObjectStyleGetStyleBits(
                    rRSB->style.rsvpStyleBits))
                {
                    case RSVP_FF_STYLE:
                    {
                        // add each RSVP_FLOWSPEC and FiltSS
                        ListItem* item = rRSB->filtSSList->first;
                        RsvpFiltSS* filtSS = (RsvpFiltSS*) item->data;

                        if (RsvpFindPhopForSenderTemplate(
                                node,
                                filtSS->filterSpec) == phopAddr)
                        {
                            RsvpCreateFlowDescriptor(
                                node,
                                (void**) &resvMessage,
                                &packetSize,
                                &rRSB->flowSpec,
                                rRSB->filtSSList,
                                (RsvpSessionAttributeFlags)
                                    psb->sessionAttr->flags,
                                ipAddr,
                                rRSB->nextHopAddress,
                                phopAddr);

                            isFlowSpecProcessed = TRUE;
                        }
                        break;
                    }

                    case RSVP_SE_STYLE:
                    {
                        unsigned short oldPacketSize;

                        // there should be one RSVP_FLOWSPEC and list of
                        // FiltSS
                        if (!isFlowSpecProcessed)
                        {
                            // create and add RSVP_FLOWSPEC at the end of
                            // packet
                            RsvpCreateFlowSpec(
                                (void**) &resvMessage,
                                &packetSize,
                                &rRSB->flowSpec);

                            isFlowSpecProcessed = TRUE;
                        }

                        oldPacketSize = packetSize;

                        // create and add FILTSS
                        RsvpCreateFiltSS(
                            node,
                            (void**) &resvMessage,
                            &packetSize,
                            rRSB->filtSSList,
                            (RsvpSessionAttributeFlags)
                                psb->sessionAttr->flags,
                            ipAddr,
                            rRSB->nextHopAddress,
                            phopAddr);

                        // packet size equal to old packet size implies
                        // filtSS is not added with the message,
                        // message discarded
                        if (oldPacketSize == packetSize)
                        {
                            isFlowSpecProcessed = FALSE;
                        }

                        break;
                    }

                    case RSVP_WF_STYLE:
                    {
                        // only one RSVP_FLOWSPEC should be there
                        if (!isFlowSpecProcessed)
                        {
                            RsvpCreateFlowSpec(
                                (void**) &resvMessage,
                                 &packetSize,
                                 &rRSB->flowSpec);

                            isFlowSpecProcessed = TRUE;
                        }
                        break;
                    }

                    default:
                    {
                        // invalid style
                        ERROR_Assert(FALSE, "Unknown reservation style");
                        break;
                    }
                } // end switch (rRSB->style.styleBits)
                // next RSB
                rsbItem = rsbItem->next;
            }

            if (isFlowSpecProcessed)
            {
                // Create a new RSVP_RESV_MESSAGE and send it to the
                // previous HOP
                RsvpSendResvMessage(
                    node,
                    resvMessage,
                    ipAddr,
                    phopAddr,
                    psb->incomingInterface);
            }

            if (resvMessage)
            {
                MEM_free(resvMessage);
            }
            ListFree(node, rsbList, FALSE);
        }
        // next PSB
        item = item->next;
    }

    // Re-send ResvRefresh with proper delay for the next trigger
    if (timeValues.refreshPeriod == 0)
    {
        return;
    }
    else
    {
        delayTime = (unsigned) ((RSVP_REFRESH_TIME_CONSTANT_K + 0.5)
            * 1.5 * timeValues.refreshPeriod);
    }

    refreshResvMsg = MESSAGE_Alloc(node,
                                   TRANSPORT_LAYER,
                                   TransportProtocol_RSVP,
                                   MSG_TRANSPORT_RSVP_ResvRefresh);

    // send current PSB pointer in the info field
    MESSAGE_InfoAlloc(node, refreshResvMsg, sizeof(NodeAddress));
    memcpy(MESSAGE_ReturnInfo(refreshResvMsg),
           &phopAddr,
           sizeof(NodeAddress));

    MESSAGE_Send(node, refreshResvMsg, delayTime * MILLI_SECOND);

    // Set the timer MSG_TRANSPORT_RSVP_HelloExtension for hello adjacency
    refreshHelloMsg = MESSAGE_Alloc(node,
                                    TRANSPORT_LAYER,
                                    TransportProtocol_RSVP,
                                    MSG_TRANSPORT_RSVP_HelloExtension);

    // send current PSB pointer in the info field
    MESSAGE_Send(node, refreshHelloMsg, RSVP_HELLO_TIMER_DELAY);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpInsertOutInterfaceInPSB
// PURPOSE      Insert a new output interface if it not present in list
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     allObjects:  all objects in the message
//     cPSB:        current PSB
//     ipAddr:      IP address of incoming interface for current node
//-------------------------------------------------------------------------
static
void RsvpInsertOutInterfaceInPSB(
    Node* node,
    RsvpAllObjects* allObjects,
    RsvpPathStateBlock* cPSB,
    NodeAddress ipAddr)
{
    int interfaceId;
    BOOL isInterfaceFound = FALSE;
    IntListItem* item = NULL;

    // if reaches in the receiver node, output interface set to -1
    if (ipAddr == allObjects->session->receiverAddress)
    {
        interfaceId = ANY_INTERFACE;
    }
    else
    {
        if (allObjects->explicitRoute)
        {
            NodeAddress nextHop;

            // get interface index when explicit route is enabled
            RsvpGetInterfaceAndNextHopForExplicitRoute(
                node,
                &(allObjects->explicitRoute->subObject),
                &interfaceId,
                &nextHop);
        }
        else
        {
            // get interface index in normal case
            interfaceId = NetworkGetInterfaceIndexForDestAddress(
                              node,
                              allObjects->session->receiverAddress);
        }
    }

    ERROR_Assert(cPSB, "Invalid path state block");

    // out interface list created if not created before
    if (!cPSB->outInterfaceList)
    {
        IntListInit(node, &cPSB->outInterfaceList);
    }

    item = cPSB->outInterfaceList->first;

    while (item)
    {
        if (item->data == interfaceId)
        {
            isInterfaceFound = TRUE;
        }
        item = item->next;
    }

    // insert output interface if not inserted already
    if (!isInterfaceFound)
    {
        IntListInsert(
            node,
            cPSB->outInterfaceList,
            node->getNodeTime(),
            interfaceId);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateAndSendPathErrMessage
// PURPOSE      The function creates and send PathErr message
//
// Return:              None
// Parameters:
//      node:           node which received the message
//      cPSB:           current PSB
//      errorCode:      error code for PathErr
//      errorValue:     error value
//-------------------------------------------------------------------------
static
void RsvpCreateAndSendPathErrMessage(
    Node* node,
    RsvpPathStateBlock* cPSB,
    RsvpErrorCode errorCode,
    unsigned short errorValue)
{
    Message* msg = NULL;
    RsvpPathErrMessage* pathErrMessage = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    NodeAddress routerAddr = NetworkIpGetInterfaceAddress(
                                 node,
                                 cPSB->incomingInterface);

    unsigned short packetSize = sizeof(RsvpPathErrMessage);

    // counter of PathErr message sent incremented
    rsvp->rsvpStat->numPathErrMsgSent++;

    // consider RSVP_ADSPEC also if present in Path message
    if (cPSB->adSpec.objectHeader.objectLength)
    {
        packetSize = (short) (packetSize
            + cPSB->adSpec.objectHeader.objectLength);
    }

    // create the PathErr message
    msg = MESSAGE_Alloc(node,
                        TRANSPORT_LAYER,
                        TransportProtocol_RSVP,
                        MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node, msg, packetSize, TRACE_RSVP);

    // PathErr message pointer
    pathErrMessage = (RsvpPathErrMessage*) MESSAGE_ReturnPacket(msg);

    // crete common header
    RsvpCreateCommonHeader(
        (RsvpCommonHeader*) pathErrMessage,
        RSVP_PATH_ERR_MESSAGE,
        1,
        packetSize);

    // RSVP_SESSION_OBJECT object copied from current PSB
    memcpy(&pathErrMessage->session,
           &cPSB->session,
           sizeof(RsvpTeObjectSession));

    // create RSVP_ERROR_SPEC object with the error
    RsvpCreateErrorSpec(
        &pathErrMessage->errorSpec,
        routerAddr,
        0,
        (unsigned char) errorCode,
        errorValue);

    // copy SENTER_TEMPLATE, RSVP_SENDER_TSPEC and RSVP_ADSPEC from
    // current PSB
    memcpy(&pathErrMessage->senderDescriptor.senderTemplate,
           &cPSB->senderTemplate,
           sizeof(RsvpTeObjectSenderTemplate));

    memcpy(&pathErrMessage->senderDescriptor.senderTSpec,
           &cPSB->senderTSpec,
           sizeof(RsvpObjectSenderTSpec));

    if (cPSB->adSpec.objectHeader.objectLength)
    {
        memcpy(pathErrMessage + 1,
               &cPSB->adSpec,
               sizeof(RsvpObjectAdSpec));
    }

    ERROR_Assert(cPSB->incomingInterface >= 0, "Invalid interface");

    // send the PathErr message to the previous HOP
    NetworkIpSendRawMessageToMacLayer(
        node,
        msg,
        routerAddr,
        cPSB->rsvpHop.nextPrevHop,  // previous hop is the destination
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_RSVP,
        1,                          // 1 == TTL, PathErr goes one hop only
        cPSB->incomingInterface,
        cPSB->rsvpHop.nextPrevHop); // previous hop itself is the next hop
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpRemoveFirstSubObject
// PURPOSE      The function removes first subObject from
//              RSVP_EXPLICIT_ROUTE object
//
// Return:              None
// Parameters:
//     explicitRoute:   RSVP_EXPLICIT_ROUTE object
//-------------------------------------------------------------------------
static
void RsvpRemoveFirstSubObject(
    RsvpTeObjectExplicitRoute** explicitRoute)
{
    // new EXPLICIT ROUTE size original minus one subObject size
    unsigned short newSize = (short)
        ((*explicitRoute)->objectHeader.objectLength
            - sizeof(RsvpTeSubObject));

    // allocate new EXPLICIT ROUTE object
    RsvpTeObjectExplicitRoute* newExplicitRoute =
        (RsvpTeObjectExplicitRoute*) MEM_malloc(newSize);

    RsvpCreateObjectHeader(
        &newExplicitRoute->objectHeader,
        newSize,
        (unsigned char) RSVP_EXPLICIT_ROUTE,
        1);

    // copy all the subObject excpet the first one
    memcpy(&newExplicitRoute->subObject,
           &((*explicitRoute)->subObject) + 1,
           newSize - sizeof(RsvpObjectHeader));

    MEM_free(*explicitRoute);
    *explicitRoute = newExplicitRoute;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchAdjacency
// PURPOSE      The function searches any adjacency of the current node
//
// Return:          TRUE adjacency found, else FALSE
// Parameters:
//     node:        node which received the message
//     ipAddress:   IP address of second subObject
//-------------------------------------------------------------------------
static
BOOL RsvpSearchAdjacency(Node* node, NodeAddress ipAddress)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = rsvp->helloList->first;

    while (item)
    {
        RsvpTeHelloExtension* hello = (RsvpTeHelloExtension*) item->data;

        // if any neighbor of this node matches with second subObject
        if ((!hello->isFailureDetected) &&
            (hello->neighborAddr == ipAddress))
        {
            return TRUE;
        }
        item = item->next;
    }
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTeProcessExplicitObject
// PURPOSE      The function processes the RSVP_EXPLICIT_ROUTE object from
//              the Path message
// ASSUMPTION   All the nodes in EXPLICIT path will be treated as strict node
//
// Return:              TRUE if processing is ok, else FALSE
// Parameters:
//     node:            node which received the message
//     msg:             message received by the layer
//     cPSB:            current PSB to be updated
//     explicitRoute:   RSVP_EXPLICIT_ROUTE object
//-------------------------------------------------------------------------
static
BOOL RsvpTeProcessExplicitObject(
    Node* node,
    Message* msg,
    RsvpPathStateBlock* psb,
    RsvpTeObjectExplicitRoute** explicitRoute)
{
    NodeAddress ipAddress;
    int interfaceId = ((NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg))->incomingInterfaceIndex;

    // if RSVP_EXPLICIT_ROUTE present
    if (*explicitRoute)
    {
        RsvpTeSubObject* subObject = &((*explicitRoute)->subObject);

        // if no first sub object then error
        if (subObject->length == 0)
        {
            RsvpCreateAndSendPathErrMessage(
                node,
                psb,
                RSVP_ROUTING_PROBLEM,
                RSVP_TE_BAD_EXPLICIT_ROUTE);

            return FALSE;
        }

        // form 4 byte IP address from two parts in sub object
        ipAddress = (subObject->ipAddress1 << 16) + subObject->ipAddress2;

        // current node is not part of abstract node
        if (ipAddress != NetworkIpGetInterfaceAddress(node, interfaceId))
        {
            // lodge PathErr
            RsvpCreateAndSendPathErrMessage(
                node,
                psb,
                RSVP_ROUTING_PROBLEM,
                RSVP_TE_BAD_INITIAL_SUB_OBJECT);

            return FALSE;
        }

        // check for any second object present
        if ((*explicitRoute)->objectHeader.objectLength
            <= (subObject->length + sizeof(RsvpObjectHeader)))
        {
            // no second object present, remove current RSVP_EXPLICIT_ROUTE
            MEM_free(*explicitRoute);
            *explicitRoute = NULL;
        }
        else
        {
            // second sub object in the explicit route
            subObject += 1;

            // form 4 byte IP address from two parts in sub object
            ipAddress = (subObject->ipAddress1 << 16)
                         + subObject->ipAddress2;

            // current node is part of second subobject also
            if (ipAddress == NetworkIpGetInterfaceAddress(node, interfaceId))
            {
                // remove first sub object and start from beginning
                RsvpRemoveFirstSubObject(explicitRoute);

                if (!RsvpTeProcessExplicitObject(
                         node,
                         msg,
                         psb,
                         explicitRoute))
                {
                    return FALSE;
                }
            }
            else
            {
                // current node is not part of second sub object. Now check
                // for any adjacency
                if (!RsvpSearchAdjacency(node, ipAddress))
                {
                    // no adjacency found Path Err
                    RsvpCreateAndSendPathErrMessage(
                        node,
                        psb,
                        RSVP_ROUTING_PROBLEM,
                        RSVP_TE_BAD_STRICT_NODE);

                    return FALSE;
                }
                else
                {
                    // adjacency found, remove first sub object
                    RsvpRemoveFirstSubObject(explicitRoute);
                    return TRUE;
                }
            }
        }
    }
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpUpdateCurrentPSB
// PURPOSE      Update the current PSB with the objects in the
//              RSVP_PATH_MESSAGE. The PSB may the exisitng one or new one.
//
// Return:          TRUE if PSB properly updated, FALSE error found
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     cPSB:        current PSB to be updated
//     allObjects:  all objects in the message
//-------------------------------------------------------------------------
static
BOOL RsvpUpdateCurrentPSB(
    Node* node,
    Message* msg,
    RsvpPathStateBlock* cPSB,
    RsvpAllObjects* allObjects)
{

    int incInterface = ((NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg))->incomingInterfaceIndex;

    NodeAddress ipAddr = NetworkIpGetInterfaceAddress(node, incInterface);

    ERROR_Assert(allObjects, "RSVP message is not parsed");
    ERROR_Assert(cPSB, "Invalid path state block");

    if (allObjects->adSpec != NULL)
    {
        // Copy RSVP_ADSPEC to current PSB
        memcpy(&cPSB->adSpec,
               allObjects->adSpec,
               allObjects->adSpec->objectHeader.objectLength);
    }

    if (allObjects->labelRequest != NULL)
    {
        // Copy RSVP_LABEL_REQUEST to current PSB
        memcpy(&cPSB->labelRequest,
               allObjects->labelRequest,
               sizeof(RsvpTeObjectLabelRequest));
    }

    if (allObjects->explicitRoute != NULL)
    {
        // Copy receiving RSVP_EXPLICIT_ROUTE to current PSB
        cPSB->receivingExplicitRoute = (RsvpTeObjectExplicitRoute*)
            MEM_malloc( allObjects->explicitRoute->objectHeader.
                objectLength);

        memcpy(cPSB->receivingExplicitRoute,
               allObjects->explicitRoute,
               allObjects->explicitRoute->objectHeader.objectLength);

        if (!RsvpTeProcessExplicitObject(
                 node,
                 msg,
                 cPSB,
                 &allObjects->explicitRoute))
        {
            // log RSVP_EXPLICIT_ROUTE error
            return FALSE;
        }

        if (allObjects->explicitRoute != NULL)
        {
            // Copy modified RSVP_EXPLICIT_ROUTE to current PSB
            cPSB->modifiedExplicitRoute =
                (RsvpTeObjectExplicitRoute*) MEM_malloc(
                    allObjects->explicitRoute->objectHeader.objectLength);

            memcpy(cPSB->modifiedExplicitRoute,
                   allObjects->explicitRoute,
                   allObjects->explicitRoute->objectHeader.objectLength);
        }
    }

    // insert output interface if not present in list
    RsvpInsertOutInterfaceInPSB(node, allObjects, cPSB, ipAddr);

    if (allObjects->recordRoute != NULL)
    {

        // consider only for the first interface

        // if previous PSB is there remove it
        if (cPSB->recordRoute)
        {
            MEM_free(cPSB->recordRoute);
            cPSB->recordRoute = NULL;
        }

        // Process loop detection
        if (RsvpProcessLoopDetection(
                node,
                allObjects->recordRoute))
        {
            // loop detected send PathErr
            RsvpCreateAndSendPathErrMessage(
                node,
                cPSB,
                RSVP_ROUTING_PROBLEM,
                RSVP_TE_RRO_INDICATED_ROUTING_LOOPS);

            // loop detected
            return FALSE;
        }

        // store RSVP_RECORD_ROUTE object in PSB
        cPSB->recordRoute = (void*) MEM_malloc(
            ((RsvpTeObjectRecordRoute*)
                allObjects->recordRoute)->objectHeader.objectLength);

        memcpy(cPSB->recordRoute,
               allObjects->recordRoute,
               ((RsvpTeObjectRecordRoute*)
                   allObjects->recordRoute)->objectHeader.objectLength);
    }

    if (allObjects->sessionAttr != NULL)
    {

        if (!cPSB->sessionAttr)
        {
            // if already some memory space allocated three then do
            // not allocate it again.
            cPSB->sessionAttr = (RsvpTeObjectSessionAttribute*) MEM_malloc(
                allObjects->sessionAttr->objectHeader.objectLength);
        }
        // store RSVP_SESSION_ATTRIBUTE object in PSB
        memcpy(cPSB->sessionAttr,
               allObjects->sessionAttr,
               allObjects->sessionAttr->objectHeader.objectLength);
    }

    // Store the receiving TTL in PSB
    cPSB->remainingTtl = (unsigned char) ((NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg))->receivingTtl;

    // If the received TTL differs from Send_TTL in the RSVP common header
    if (cPSB->remainingTtl != allObjects->commonHeader->sendTtl)
    {
        cPSB->nonRsvpFlag = TRUE;
    }
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessPathMessage
// PURPOSE      This function processes the RSVP_PATH_MESSAGE when an
//              intermediate RSVP node or the receiver gets the
//              RSVP_PATH_MESSAGE
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     allObjects:  collection of all RSVP objects
//-------------------------------------------------------------------------
static
void RsvpProcessPathMessage(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    BOOL pathRefreshNeeded = FALSE;
    BOOL resvRefreshNeeded = FALSE;
    RsvpPathStateBlock* cPSB = NULL;
    RsvpPathStateBlock* fPSB = NULL;
    LinkedList* activeRSBs = NULL;

    // incoming interface at which RSVP_PATH_MESSAGE arrives
    int incInterface = ((NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg))->incomingInterfaceIndex;

    // IP address of the incoming interface
    NodeAddress ipAddr = NetworkIpGetInterfaceAddress(node, incInterface);

    ERROR_Assert(allObjects, "RSVP message is not parsed");

    // matching PSB returned in cPSB, and fPSB is set for matching
    // PSB if Local_Flag is OFF
    cPSB = RsvpSearchPSB(node,
                         msg,
                         allObjects->session,
                         allObjects->senderTemplate,
                         &fPSB);
    if (cPSB == NULL)
    {
        // no matching PSB found, going to create a new PSB
        cPSB = RsvpCreatePSB(node, allObjects);

        cPSB->incomingInterface = ((NetworkToTransportInfo*)
            MESSAGE_ReturnInfo(msg))->incomingInterfaceIndex;

        pathRefreshNeeded = TRUE;
    }
    else
    {
        // matching PSB found
        if ((cPSB->rsvpHop.nextPrevHop != allObjects->rsvpHop->nextPrevHop)
            || (cPSB->rsvpHop.lih != allObjects->rsvpHop->lih))
        {
            // copy PHOP IP address and LIH to the current PSB
            memcpy(&cPSB->rsvpHop,
                   allObjects->rsvpHop,
                   sizeof(RsvpObjectRsvpHop));

            resvRefreshNeeded = TRUE;
            pathRefreshNeeded = TRUE;
        }

        if (memcmp(&cPSB->senderTSpec, allObjects->senderTSpec,
            sizeof(RsvpObjectSenderTSpec)))
        {
            // copy RSVP_SENDER_TSPEC to the current PSB
            memcpy(&cPSB->senderTSpec,
                   allObjects->senderTSpec,
                   sizeof(RsvpObjectSenderTSpec));

            pathRefreshNeeded = TRUE;
        }
    }

    // Update current PSB (new one or existing one) with other objects
    if (!RsvpUpdateCurrentPSB(node, msg, cPSB, allObjects))
    {
        // return to RsvpHandlePacket()
        // to drop and free the message
        return;
    }

    if (pathRefreshNeeded == FALSE)
    {
        // return to RsvpHandlePacket()
        // to drop and free the message
        return;
    }
    else
    {
        // If this PATH message came from a network interface and not from a
        // local API, make a Path Event upcall for each local application for
        // this session. If PHOP is the sender, then message from local
        // interface
        if (allObjects->rsvpHop->nextPrevHop !=
            allObjects->senderTemplate->senderAddress)
        {
            // receive upcall function pointer
            RsvpUpcallFunctionType upcallFunctionPtr = NULL;

            // create info parameter
            RsvpUpcallInfoParameter infoParm;
            infoParm.Info.PathEvent.senderTSpec = &cPSB->senderTSpec;
            infoParm.Info.PathEvent.senderTemplate = &cPSB->senderTemplate;
            infoParm.Info.PathEvent.adSpec = &cPSB->adSpec;

            // upcall to application
            if (upcallFunctionPtr)
            {
                (upcallFunctionPtr) (cPSB->session.tunnelId,
                                     PATH_EVENT,
                                     infoParm);
            }
        }

        // if RSVP_PATH_MESSAGE is received by egress node, generate
        // RSVP_RESV_MESSAGE
        if (allObjects->session->receiverAddress == ipAddr)
        {
            RsvpCreateRSBForReceiver(node, msg, allObjects, cPSB);
        }
        else
        {
            ERROR_Assert(cPSB->outInterfaceList &&
                         cPSB->outInterfaceList->first,
                         "Invalid interface list");

            // output valid output interface exists
            if (cPSB->outInterfaceList->size
                && (cPSB->outInterfaceList->first->data != -1))
            {
                RsvpEnvokePathRefresh(node, cPSB);
            }
        }

        // activeRSBs = List of RSBs whose filterSpecList contains
        // RSVP_SENDER_TEMPLATE of cPSB and output interface appears in
        // output interface list of cPSB
        activeRSBs = RsvpSearchRSB(node, cPSB);

        ERROR_Assert(activeRSBs, "Invalid Resv state block");

        if (activeRSBs->size == 0)
        {
            // return to RsvpHandlePacket()
            // to drop and free the message
            ListFree(node, activeRSBs, FALSE);
            return;
        }
        else
        {
            // execute ResvRefresh event for PHOP in cPSB
            RsvpEnvokeResvRefresh(
                node,
                cPSB->rsvpHop.nextPrevHop);
        }

        // delete the memory allocated for RSB list
        ListFree(node, activeRSBs, FALSE);

        // TBD
        // execute UPDATE TRAFFIC CONTROL. Right now it is ignored

        // return to RsvpHandlePacket()
        // to drop and free the message
        return;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchPSBForSessionOnly
// PURPOSE      Search in the PSB for the current session only
//
// Return:          Matching PSB, NULL if no matching
// Parameters:
//     node:        node which received the message
//     allObejcts:  all objects from the message
//-------------------------------------------------------------------------
static
RsvpPathStateBlock* RsvpSearchPSBForSessionOnly(
    Node* node,
    RsvpTeObjectSession* session)
{
    RsvpPathStateBlock* psb = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = NULL;

    ERROR_Assert(rsvp->psb, "Invalid path state block");

    item = rsvp->psb->first;

    // Go through the PSB list
    while (item)
    {
        // get a PSB from the list
        psb = (RsvpPathStateBlock*) item->data;

        ERROR_Assert(psb, "Invalid path state block");

        // check the session object
        if ((psb->session.receiverAddress == session->receiverAddress)
            && (psb->session.tunnelId == session->tunnelId))
        {
            return psb;
        }
        item = item->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchRSBForSessionNhop
// PURPOSE      Search in the RSB for the current RSVP_SESSION_OBJECT, NHOP
//              pair and for distinct style the filter spec list
//
// Return:              Matching PSB, NULL if no matching
// Parameters:
//     node:            node which received the message
//     session:         RSVP_SESSION_OBJECT object in the message
//     rsvpHop:         RSVP_HOP object for the next HOP in the message
//     filterSpecList:  list of filter spec
//-------------------------------------------------------------------------
static
RsvpResvStateBlock* RsvpSearchRSBForSessionNhop(
    Node* node,
    RsvpTeObjectSession* session,
    RsvpObjectRsvpHop* rsvpHop,
    LinkedList* filterSpecList)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = rsvp->rsb->first;

    ERROR_Assert(rsvp->rsb, "Invalid Resv state block");

    // Go through the RSB list
    while (item)
    {
        RsvpResvStateBlock* rsb = (RsvpResvStateBlock*) item->data;

        ERROR_Assert(rsb, "Invalid Resv state block");

        // search for the SESSIOn and NHOP pair
        if ((rsb->session.receiverAddress == session->receiverAddress)
            && (rsb->session.tunnelId == session->tunnelId)
            && (rsb->nextHopAddress == rsvpHop->nextPrevHop))
        {
            if (filterSpecList == NULL)
            {
                // for WF style filter spec list not considered
                return rsb;
            }
            else
            {
                // for FF and SE style, try to match filter spec list. For
                // each RSVP_FILTER_SPEC in the list, search the list from
                // current RSB

                // this loop for filter spec list in current message
                ListItem* itemFilter = filterSpecList->first;
                while (itemFilter)
                {
                    ListItem* itemRSBFilter = NULL;
                    RsvpFiltSS* filtSS = (RsvpFiltSS*) itemFilter->data;

                    ERROR_Assert(filtSS, "Invalid Filter spec");
                    ERROR_Assert(rsb->filtSSList, "Invalid Filter spec");

                    itemRSBFilter = rsb->filtSSList->first;

                    // this loop for the filter spec list in RSB
                    while (itemRSBFilter)
                    {
                        RsvpFiltSS* filtSSRsb = (RsvpFiltSS*)
                            itemRSBFilter->data;

                        ERROR_Assert(filtSSRsb, "Invalid Filter spec");

                        if ((filtSS->filterSpec->senderAddress ==
                                filtSSRsb->filterSpec->senderAddress)
                            && (filtSS->filterSpec->lspId ==
                                filtSSRsb->filterSpec->lspId))
                        {
                            // matching implies searching ok
                            return rsb;
                        }
                        itemRSBFilter = itemRSBFilter->next;
                    }
                    itemFilter = itemFilter->next;
                }
            }
        }
        item = item->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateRSB
// PURPOSE      Create a new RSB for current session/NHOP pair
//
// Return:              Created RSB
// Parameters:
//     node:            node which received the message
//     msg:             message received by the layer
//     allObjects:      all objects in the message
//     flowSpec:        RSVP_FLOWSPEC object
//     filterSpecList:  list of RSVP_FILTER_SPEC
//-------------------------------------------------------------------------
static
RsvpResvStateBlock* RsvpCreateRSB(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects,
    RsvpObjectFlowSpec* flowSpec,
    LinkedList* filterSpecList)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    // Create a new RSB and initialise
    RsvpResvStateBlock* newRSB = (RsvpResvStateBlock*)
        MEM_malloc(sizeof(RsvpResvStateBlock));

    memset(newRSB, 0, sizeof(RsvpResvStateBlock));

    ERROR_Assert(flowSpec, "Invalid Flow spec");
    ERROR_Assert(filterSpecList, "Invalid Filter spec");

    // Set the session, NHOP, OI and style of the RSB from the message.
    memcpy(&newRSB->session,
           allObjects->session,
           sizeof(RsvpTeObjectSession));

    memcpy(&newRSB->nextHopAddress,
           &allObjects->rsvpHop->nextPrevHop,
           sizeof(NodeAddress));

    newRSB->outgoingLogicalInterface = ((NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg))->incomingInterfaceIndex;

    memcpy(&newRSB->style,
           allObjects->style,
           sizeof(RsvpObjectStyle));

    // Initialize the filter spec list entry
    ListInit(node, &newRSB->filtSSList);

    // Copy FiltSS (filter spec, label anf record route) list into the RSB.
    if (!RsvpAddFilterSpecList(
             node,
             msg,
             newRSB,
             filterSpecList,
             allObjects))
    {
        // error creating RSB
        return NULL;
    }

    // Copy RSVP_FLOWSPEC and any RSVP_SCOPE object from the message
    // into the RSB.
    memcpy(&newRSB->flowSpec, flowSpec, sizeof(RsvpObjectFlowSpec));

    if (allObjects->scope)
    {
        memcpy(&newRSB->scope, allObjects->scope, sizeof(RsvpObjectScope));
    }

    // Insert it to the RSB list and return created RSB
    ListInsert(node, rsvp->rsb, node->getNodeTime(), newRSB);
    return newRSB;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpAddFilterSpecIfRequired
// PURPOSE      Add FiltSS in the filter spec list of RSB if current filtSS
//              is not present in the list. This list consists of
//              RSVP_FILTER_SPEC, RSVP_LABEL_OBJECT and RSVP_RECORD_ROUTE
//
// Return:              TRUE if new or modified, else FALSE
// Parameters:
//     node:            node which received the message
//     rsb:             new RSB block for the entry
//     filterSpecList:  list of filtSS
//-------------------------------------------------------------------------
static
BOOL RsvpAddFilterSpecIfRequired(
    Node* node,
    Message* msg,
    RsvpResvStateBlock* rsb,
    LinkedList* filterSpecList,
    RsvpAllObjects* allObjects)
{
    BOOL newOrMod = FALSE;
    ListItem* item = NULL;

    ERROR_Assert(filterSpecList, "Invalid Filter spec");

    item = filterSpecList->first;

    // for each filtSS in the list collected from the message
    while (item)
    {
        BOOL isMatchingFound = FALSE;
        ListItem* itemRsb = NULL;
        RsvpFiltSS* filtSS = (RsvpFiltSS*) item->data;

        ERROR_Assert(filtSS, "Invalid Filter spec");
        ERROR_Assert(rsb->filtSSList, "Invalid Filter spec");

        // search in the RSB for that filtSS
        itemRsb = rsb->filtSSList->first;

        while (itemRsb)
        {
            RsvpFiltSS* filtSSRsb = (RsvpFiltSS*) itemRsb->data;

            ERROR_Assert(filtSSRsb, "Invalid Filter spec");

            // if current filtSS is not found in RSB, prepare to insert it
            if ((filtSS->filterSpec->senderAddress ==
                    filtSSRsb->filterSpec->senderAddress)
                && (filtSS->filterSpec->lspId ==
                    filtSSRsb->filterSpec->lspId))
            {
                isMatchingFound = TRUE;
            }
            itemRsb = itemRsb->next;
        }

        if (!isMatchingFound)
        {
            newOrMod = TRUE;

            // current filtSS not found in RSB, going to insert it
            RsvpCopyFilterSpec(
                node,
                msg,
                rsb,
                filtSS,
                allObjects);
        }
        item = item->next;
    }
    return newOrMod;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessResvFlowDescriptor
// PURPOSE      Process each Flow descriptor for the current Resv message.
//
// Return:                None
// Parameters:
//     node:              node which received the message
//     allObjects:        All Objects
//     flowSpec:          RSVP_FLOWSPEC for current set
//     filterSpecList:    list of Filter spec
//     resvRefreshNeeded: output flag resv refresh needed
//-------------------------------------------------------------------------
static
void RsvpProcessResvFlowDescriptor(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects,
    RsvpObjectFlowSpec* flowSpec,
    LinkedList* filterSpecList,
    BOOL* resvRefreshNeeded)
{
    BOOL newOrMod = FALSE;
    RsvpResvStateBlock* activeRSB = NULL;

    ERROR_Assert(flowSpec, "Invalid Flow spec");
    ERROR_Assert(filterSpecList, "Invalid Filter spec");

    // Find RSB for current RSVP_SESSION_OBJECT and NHOP pair, if reservation
    // style is distinct then filter_spec also must match for the selection.
    // Call it activeRSB
    activeRSB = RsvpSearchRSBForSessionNhop(
                    node,
                    allObjects->session,
                    allObjects->rsvpHop,
                    filterSpecList);

    if (activeRSB == NULL)
    {
        // search failed, going to create a new RSB
        activeRSB = RsvpCreateRSB(
                        node,
                        msg,
                        allObjects,
                        flowSpec,
                        filterSpecList);

        if (activeRSB == NULL)
        {
            // RSB cannot be created , return and free the message
            return;
        }
        newOrMod = TRUE;
    }
    else
    {
        // search criteria found a RSB
        // Add new RSVP_FILTER_SPEC if not present and set flag for that
        newOrMod = RsvpAddFilterSpecIfRequired(
                       node,
                       msg,
                       activeRSB,
                       filterSpecList,
                       allObjects);

        // copy RSVP_STYLE, RSVP_FLOWSPEC or RSVP_SCOPE objects from the
        // message to RSB if they are changed
        if (memcmp(&activeRSB->style,
                   allObjects->style,
                   sizeof(RsvpObjectStyle)))
        {
            memcpy(&activeRSB->style,
                   allObjects->style,
                   sizeof(RsvpObjectStyle));

            newOrMod = TRUE;
        }

        if (memcmp(&activeRSB->flowSpec,
                   flowSpec,
                   sizeof(RsvpObjectFlowSpec)))
        {
            memcpy(&activeRSB->flowSpec,
                   flowSpec,
                   sizeof(RsvpObjectFlowSpec));

            newOrMod = TRUE;
        }

        if ((allObjects->scope != NULL) &&
            (memcmp(&activeRSB->scope,
                    allObjects->scope,
                    sizeof(RsvpObjectScope)) != 0))
        {
            memcpy(&activeRSB->scope,
                   allObjects->scope,
                   sizeof(RsvpObjectScope));

            newOrMod = TRUE;
        }

        // copy RESV_CONFIRM object to current RSB and Set
        // newOrMod flag on.
        if (allObjects->resvConf != NULL)
        {
            memcpy(&activeRSB->resvConf, allObjects->resvConf,
                    sizeof(RsvpObjectResvConfirm));
            newOrMod = TRUE;
        }
    }

    // Start or restart the cleanup timer on active RSB
    // TBD
    if (newOrMod == TRUE)
    {
        // TBD
        // active RSB is new or modified
        // This part will be implemented later when UPDATE TRAFFIC CONTROL
        // will be handled

        // execute the UPDATE TRAFFIC CONTROL event. If the result is to
        // modify the traffic control state, this sequence will turn on the
        // Resv_Refresh_Needed flag and make a RESV_EVENT upcall to any local
        // application.

        // If the UPDATE TRAFFIC CONTROL sequence fails with an error, then
        // delete a new RSB but restore the original reservation in an
        // old RSB.

        // right now by default resvRefresh flag set to true
        *resvRefreshNeeded = TRUE;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpAddPhopInList
// PURPOSE      Add PHOP address in Phop list if current PHOP address is not
//              already in the list
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     phopList:    PHOP list
//     currentPhop: current PHOP to be inserted
//-------------------------------------------------------------------------
static
void RsvpAddPhopInList(
    Node* node,
    LinkedList* phopList,
    NodeAddress currentPhop)
{
    ListItem* item = NULL;
    BOOL isAlreadyExist = FALSE;

    ERROR_Assert(phopList, "Invalid PHOP list");

    if (currentPhop == 0)
    {
        // no PHOP to added
        return;
    }

    item = phopList->first;

    while (item)
    {
        NodeAddress *phop = (NodeAddress *) item->data;

        if (*phop == currentPhop)
        {
            isAlreadyExist = TRUE;
        }
        item = item->next;
    }

    // if not already in the list, insert it
    if (!isAlreadyExist)
    {
        NodeAddress* phopPtr = (NodeAddress*) MEM_malloc(
            sizeof(NodeAddress));
        *phopPtr = currentPhop;
        ListInsert(node, phopList, node->getNodeTime(), (void*) phopPtr);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpLocatePSBForOIAndFilterSpec
// PURPOSE      The function finds list of PSB in the current output
//              interface
//              and with one RSVP_FILTER_SPEC matches with the
//              RSVP_SENDER_TEMPLATE
//              object in the PSB
//
// Return:              None
// Parameters:
//     node:            node which received the message
//     msg:             receiving message in the node
//     filterSpecList:  list of Filter spec
//-------------------------------------------------------------------------
static
void RsvpLocatePSBForOIAndFilterSpec(
    Node* node,
    Message* msg,
    RsvpFiltSS* filtSS,
    LinkedList* psbList)
{
    // Search in PSB for the current interface with the outgoing interface
    // and RSVP_FILTER_SPEC object from filterSpec list matches with
    // RSVP_SENDER_TEMPLATE. Create a list with matching PSBs and return
    // the list

    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    int OI = ((NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg))->incomingInterfaceIndex;

    RsvpPathStateBlock* psb = NULL;
    ListItem* item = NULL;
    IntListItem* interfaceItem = NULL;

    ERROR_Assert(filtSS, "Invalid Filter spec");

    // for FF and SE style
    item = rsvp->psb->first;

    //  Go through the PSB list
    while (item)
    {
        // get a PSB from the list
        psb = (RsvpPathStateBlock*) item->data;

        ERROR_Assert(psb, "Invalid Path state block");

        // check sender template with one RSVP_FILTER_SPEC object
        if ((psb->senderTemplate.senderAddress ==
                filtSS->filterSpec->senderAddress)
            && (psb->senderTemplate.lspId ==
                filtSS->filterSpec->lspId))
        {
            ERROR_Assert(psb->outInterfaceList, "Invalid interface");

            interfaceItem = psb->outInterfaceList->first;

            // go through the out interface list of current psb
            while (interfaceItem)
            {
                int outInterface = interfaceItem->data;

                // check current interface matching with out interface
                // list of the PSB
                if (outInterface == OI)
                {
                    RsvpPathStateBlock* newPSB =
                        (RsvpPathStateBlock*)
                        MEM_malloc(sizeof(RsvpPathStateBlock));

                    memcpy(newPSB, psb, sizeof(RsvpPathStateBlock));
                    ListInsert(node, psbList, node->getNodeTime(), newPSB);
                }
                interfaceItem = interfaceItem->next;
            }
        }
        item = item->next;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpDeleteUnnecessaryFilterSpec
// PURPOSE      This function deletes unnecessary filter spec from the filter
//              spec list, if for that filter spec there is no PSB
//
// Return:              None
// Parameters:
//     node:            node which received the message
//     filterSpecList:  list of Filter spec
//-------------------------------------------------------------------------
static
void RsvpDeleteUnnecessaryFilterSpec(
     Node* node,
     LinkedList* filterSpecList)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    BOOL isMatchingFound = FALSE;
    ListItem* item = NULL;
    ListItem* psbItem = NULL;

    ERROR_Assert(filterSpecList, "Invalid filter spec");

    item = filterSpecList->first;

    // go through the filter spec list
    while (item)
    {
        RsvpFiltSS* filtSS = (RsvpFiltSS*) item->data;
        isMatchingFound = TRUE;

        ERROR_Assert(rsvp->psb, "Invalid PSB");

        // now search in PSB for each filter spec
        psbItem = rsvp->psb->first;
        while (psbItem && (!isMatchingFound))
        {
            RsvpPathStateBlock* psb = (RsvpPathStateBlock*) psbItem->data;

            ERROR_Assert(rsvp->psb, "Invalid Path state block");

            // if any matching found in PSB for the current filter spec
            if ((psb->senderTemplate.senderAddress ==
                    filtSS->filterSpec->senderAddress)
                && (psb->senderTemplate.lspId == filtSS->filterSpec->lspId))
            {
                isMatchingFound = TRUE;
            }
            else
            {
                isMatchingFound = FALSE;
            }
            psbItem = psbItem->next;
        }
        if (!isMatchingFound)
        {
            ListItem* nextItem = item->next;

            // no matching found in PSB for the current filter spec.
            ListGet(node, filterSpecList, item, TRUE, FALSE);
            item = nextItem;
        }
        else
        {
            item = item->next;
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessResvFlowDescriptorList
// PURPOSE      The function processes the flow descriptor list of Resv
//              message
// ASSUMPTION   WF style is not considered for traffic engineering as
//              described in the draft-ietf-mpls-rsvp-lsp-tunnel-08.txt.
//              Still codes are developed for WF style, that will be used
//              for RSVP only. So all the WF style related codes are not
//              tested.
//
// Return:                  None
// Parameters:
//     node:                node which received the message
//     msg:                 receiving message in the node
//     allObjects:          all objects in the message
//     phopList:            PHOP list
//     resvRefreshNeeded:   output flag whether resv refresh needed
//-------------------------------------------------------------------------
static void
RsvpProcessResvFlowDescriptorList(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects,
    LinkedList* phopList,
    BOOL* resvRefreshNeeded)
{
    LinkedList* cPSBs = NULL;
    RsvpPathStateBlock* psb = NULL;

    ERROR_Assert(allObjects, "RSVP message is not parsed");

    switch (RsvpObjectStyleGetStyleBits(allObjects->style->rsvpStyleBits))
    {
        case RSVP_FF_STYLE:
        {
            // processing FF style
            ListItem* item = allObjects->FlowDescriptorList.
                ffDescriptor->first;

            RsvpFFDescriptor* ffDescriptor = NULL;
            LinkedList* filterSpecList = NULL;
            RsvpFiltSS* filtSS = NULL;

            // for each flow descriptor
            while (item)
            {
                // For each flow descriptor in the list
                ffDescriptor = (RsvpFFDescriptor*) item->data;

                ERROR_Assert(ffDescriptor, "Invalid Flow descriptor");

                // send the filter spec in a list
                ListInit(node, &cPSBs);
                ListInit(node, &filterSpecList);

                // create filtSS with current filtSS
                filtSS = (RsvpFiltSS*) MEM_malloc(sizeof(RsvpFiltSS));

                memcpy(filtSS,
                       ffDescriptor->filtSS,
                       sizeof(RsvpFiltSS));

                ListInsert(node, filterSpecList, node->getNodeTime(), filtSS);

                // filtSpecs is the list of RSVP_FILTER_SPEC, here it is one
                // filterSpec for the current flowSpec. returns list of PSBs
                // matching
                RsvpLocatePSBForOIAndFilterSpec(
                    node,
                    msg,
                    ffDescriptor->filtSS,
                    cPSBs);

                ERROR_Assert(cPSBs, "Invalid Path state list");

                if (cPSBs->size == 0)
                {
                    RsvpCreateAndSendResvErrMessage(
                        node,
                        msg,
                        allObjects,
                        RSVP_NO_SENDER_INFO_FOR_RESV,
                        0);
                }
                else if (cPSBs->size > 1)
                {
                    RsvpCreateAndSendResvErrMessage(
                        node,
                        msg,
                        allObjects,
                        RSVP_CONFLICTING_SENDER_PORT,
                        0);
                }
                else
                {
                    psb = (RsvpPathStateBlock *)
                          ((ListItem*) cPSBs->first)->data;

                    ERROR_Assert(psb, "Invalid Path state block");

                    // Add PHOP from the PSB to the refreshPhopList
                    RsvpAddPhopInList(
                        node,
                        phopList,
                        psb->rsvpHop.nextPrevHop);

                    // process each flow descriptor
                    RsvpProcessResvFlowDescriptor(
                        node,
                        msg,
                        allObjects,
                        ffDescriptor->flowSpec,
                        filterSpecList,
                        resvRefreshNeeded);
                }
                ListFree(node, filterSpecList, FALSE);
                ListFree(node, cPSBs, FALSE);
                item = item->next;
            }
            break;
        }

        case RSVP_SE_STYLE:
        {
            // processing of SE style
            RsvpSEDescriptor* seDescriptor = (RsvpSEDescriptor*)
                allObjects->FlowDescriptorList.seDescriptor;

            ListItem* item = NULL;

            // filtSpecs is list of RSVP_FILTER_SPEC for current flowSpec.
            // returns list of PSBs matching
            item = seDescriptor->filtSS->first;

            while (item)
            {
                RsvpFiltSS* filtSS = (RsvpFiltSS*) item->data;

                ListInit(node, &cPSBs);

                RsvpLocatePSBForOIAndFilterSpec(
                    node,
                    msg,
                    filtSS,
                    cPSBs);

                ERROR_Assert(cPSBs, "Invalid Path state list");

                if (cPSBs->size == 0)
                {
                    RsvpCreateAndSendResvErrMessage(
                        node,
                        msg,
                        allObjects,
                        RSVP_NO_SENDER_INFO_FOR_RESV,
                        0);
                }
                else if (cPSBs->size > 1)
                {
                    RsvpCreateAndSendResvErrMessage(
                        node,
                        msg,
                        allObjects,
                        RSVP_CONFLICTING_SENDER_PORT,
                        0);
                }
                else
                {
                    psb = (RsvpPathStateBlock *)
                          ((ListItem*) cPSBs->first)->data;

                    ERROR_Assert(psb, "Invalid Path state block");

                    // Add PHOP from the PSB to the refreshPhopList
                    RsvpAddPhopInList(
                        node,
                        phopList,
                        psb->rsvpHop.nextPrevHop);
                }
                ListFree(node, cPSBs, FALSE);
                item = item->next;
            }

            // Delete the RSVP_FILTER_SPEC from the filter spec list for
            // which there is no PSB entry.
            RsvpDeleteUnnecessaryFilterSpec(node, seDescriptor->filtSS);

            // process flow descriptor now
            RsvpProcessResvFlowDescriptor(
                node,
                msg,
                allObjects,
                seDescriptor->flowSpec,
                seDescriptor->filtSS,
                resvRefreshNeeded);

            break;
        }

        case RSVP_WF_STYLE:
        {
            // filtSpecs is NULL here. It returns list of PSBs matching
            ListInit(node, &cPSBs);
            RsvpLocatePSBForOIAndFilterSpec(node, msg, NULL, cPSBs);

            ERROR_Assert(cPSBs, "Invalid Path state list");

            if (cPSBs->size == 0)
            {
                RsvpCreateAndSendResvErrMessage(
                    node,
                    msg,
                    allObjects,
                    RSVP_NO_SENDER_INFO_FOR_RESV,
                    0);
            }
            else
            {
                psb = (RsvpPathStateBlock *)
                      ((ListItem*) cPSBs->first)->data;

                ERROR_Assert(psb, "Invalid Path state block");

                // Add PHOP from the PSB to the refreshPhopList
                RsvpAddPhopInList(
                    node,
                    phopList,
                    psb->rsvpHop.nextPrevHop);

                // process flow descriptor with NULL filter spec
                RsvpProcessResvFlowDescriptor(
                    node,
                    msg,
                    allObjects,
                    allObjects->FlowDescriptorList.flowSpec,
                    NULL,
                    resvRefreshNeeded);
            }
            ListFree(node, cPSBs, FALSE);
            break;
        }

        default:
        {
            // invalid style
            ERROR_Assert(FALSE, "Unknown Reservation Style");
            break;
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchPSBForTearDown
// PURPOSE      Search PSB list to find the PSB to be teared down for old LSP
//
// Return:      PSB to be teared down, if not found then NULL
// Parameters:
//     node:    node which received the message
//     lspId:   old LSP
//     psb:     current PSB for which a new LSP is found
//-------------------------------------------------------------------------
static
RsvpPathStateBlock* RsvpSearchPSBForTearDown(
    Node* node,
    unsigned short lspId,
    RsvpTeObjectSession* session,
    RsvpTeObjectFilterSpec* filterSpec)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = rsvp->psb->first;

    while (item)
    {
        RsvpPathStateBlock* psb = (RsvpPathStateBlock*) item->data;

        if ((psb->session.receiverAddress == session->receiverAddress) &&
            (psb->senderTemplate.senderAddress ==
                filterSpec->senderAddress) &&
            (psb->senderTemplate.lspId == lspId))
        {
            return psb;
        }
        item = item->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpRemoveFilterSpecFromRSB
// PURPOSE      Remove current RSVP_FILTER_SPEC from the list of RSB, that
//              matches SENTER_TEMPLATE of current PathTear message.
//
// Return:              None
// Parameters:
//     node:            node which received the message
//     cRSB:            current matching RSB
//     senderTemplate:  RSVP_SENDER_TEMPLATE to be matched
//-------------------------------------------------------------------------
static
void RsvpRemoveFilterSpecFromRSB(
    Node* node,
    RsvpResvStateBlock* cRSB,
    RsvpTeObjectSenderTemplate senderTemplate)
{
    ListItem* item = NULL;
    item = cRSB->filtSSList->first;

    ERROR_Assert(cRSB->filtSSList, "Invalid filter spec");

    while (item)
    {
        RsvpFiltSS* filtSS = (RsvpFiltSS*) item->data;

        ERROR_Assert(filtSS, "Invalid filter spec");

        if ((filtSS->filterSpec->senderAddress ==
               senderTemplate.senderAddress)
            && (filtSS->filterSpec->lspId == senderTemplate.lspId))
        {
            ListItem* nextItem = item->next;

            // remove current item from list
            ListGet(node, cRSB->filtSSList, item, TRUE, FALSE);
            item = nextItem;
        }
        else
        {
            item = item->next;
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpRemoveCurrentRSB
// PURPOSE      Remove current RSB if the RSB does not contain any
//              RSVP_FILTER_SPEC
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     cRSB:    current matching RSB
//-------------------------------------------------------------------------
static
void RsvpRemoveCurrentRSB(
    Node* node,
    RsvpResvStateBlock* cRSB)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = NULL;

    ERROR_Assert(rsvp->rsb, "Invalid RSB");

    item = rsvp->rsb->first;
    while (item)
    {
        RsvpResvStateBlock* rsb = (RsvpResvStateBlock*) item->data;
        ERROR_Assert(rsb, "Invalid RSB");

        if ((rsb->session.receiverAddress == cRSB->session.receiverAddress)
            && (rsb->session.tunnelId == cRSB->session.tunnelId)
            && (rsb->nextHopAddress == cRSB->nextHopAddress)
            && (rsb->filtSSList->size == 0))
        {
            ListItem* nextItem = item->next;

            // remove current item from list
            ListGet(node, rsvp->rsb, item, TRUE, FALSE);
            item = nextItem;
        }
        else
        {
            item = item->next;
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpRemoveCurrentPSB
// PURPOSE      Remove current PSB after the processing of PathTear
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     cPSB:    PSB to be removed
//-------------------------------------------------------------------------
static
void RsvpRemoveCurrentPSB(
    Node* node,
    RsvpPathStateBlock* cPSB)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = NULL;

    ERROR_Assert(rsvp->psb, "Invalid PSB");

    item = rsvp->psb->first;

    // go through the PSB list
    while (item)
    {
        RsvpPathStateBlock* psb = (RsvpPathStateBlock*) item->data;

        ERROR_Assert(psb, "Invalid PSB");

        if ((psb->session.receiverAddress == cPSB->session.receiverAddress)
            && (psb->session.tunnelId == cPSB->session.tunnelId)
            && (psb->senderTemplate.senderAddress ==
                cPSB->senderTemplate.senderAddress)
            && (psb->senderTemplate.lspId == cPSB->senderTemplate.lspId))
        {
            RsvpRemoveFromTimerStoreList(node, cPSB);

            // first remove outInterface list
            IntListFree(node, psb->outInterfaceList, FALSE);

            // remove current item from list
            ListGet(node, rsvp->psb, item, TRUE, FALSE);
            return;
        }
        item = item->next;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpRemoveOldLsp
// PURPOSE      Remove Old LSP after the processing of PathTear
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     psb:     PSB for which LSP to be removed
//-------------------------------------------------------------------------
static
void RsvpRemoveOldLsp(
    Node* node,
    RsvpPathStateBlock* psb)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = NULL;
    MplsData* mpls = MplsReturnStateSpace(node);

    ERROR_Assert(rsvp->lsp, "Invalid LSP");

    item = rsvp->lsp->first;

    // go through the PSB list
    while (item)
    {
        RsvpLsp* lsp = (RsvpLsp*) item->data;
        unsigned int outLabel;

        ERROR_Assert(lsp, "Invalid LSP");

        if ((lsp->destAddr == psb->session.receiverAddress)
            && (lsp->sourceAddr == psb->senderTemplate.senderAddress)
            && (lsp->lspId == psb->senderTemplate.lspId))
        {
            MplsRemoveILMEntry(mpls, lsp->inLabel, &outLabel);
            MplsRemoveFTNEntry(mpls, lsp->destAddr, outLabel);

            // remove current item from LSP list
            ListGet(node, rsvp->lsp, item, TRUE, FALSE);
            return;
        }
        item = item->next;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpRemoveCurrentRSBForWFStyle
// PURPOSE      Remove current RSB if there is no other PSB matching with
//              that RSB. That means if that is the only one RSB for PSBs
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     cRSB:    RSB to be removed
//-------------------------------------------------------------------------
static
void RsvpRemoveCurrentRSBForWFStyle(
     Node* node,
     RsvpResvStateBlock* cRSB)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    // Search in PSB list to find how many occurance of this RSB
    int counter = 0;
    ListItem* item = NULL;

    ERROR_Assert(rsvp->psb, "Invalid PSB");

    item = rsvp->psb->first;
    while (item)
    {
        RsvpPathStateBlock* psb =
            (RsvpPathStateBlock*) item->data;

        ERROR_Assert(psb, "Invalid PSB");

        if ((psb->session.receiverAddress == cRSB->session.receiverAddress)
            && (psb->session.tunnelId == cRSB->session.tunnelId))
        {
            counter++;      // number of occurance
        }
        item = item->next;
    }

    // check whether there are more than one PSB for current RSB
    if (counter <= 1)
    {
        // if only 1 PSB, then remove RSB from the list
        ListItem* item = rsvp->rsb->first;

        while (item)
        {
            RsvpResvStateBlock* rsb = (RsvpResvStateBlock*) item->data;

            ERROR_Assert(rsb, "Invalid RSB");

            if ((rsb->session.receiverAddress ==
                   cRSB->session.receiverAddress)
                && (rsb->session.tunnelId == cRSB->session.tunnelId))
            {
                // remove current item from list
                ListGet(node, rsvp->rsb, item, TRUE, FALSE);
                return;
            }
            item = item->next;
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateAndSendPathTearMessage
// PURPOSE      The function creates and send Path tear message
//
// Return:           None
// Parameters:
//      node:        node which received the message
//      cPSB:        current PSB
//-------------------------------------------------------------------------
static
void RsvpCreateAndSendPathTearMessage(
    Node* node,
    RsvpPathStateBlock* psb)
{
    Message* msg = NULL;
    RsvpPathTearMessage* pathTearMessage = NULL;
    NodeAddress sourceAddr;
    NodeAddress nextHop = 0;
    int interfaceIndex = ANY_INTERFACE;
    RsvpResvStateBlock* rsb = NULL;
    LinkedList* rsbList = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    rsvp->rsvpStat->numPathTearMsgSent++;

    // interface index and next hop is determined from its RSB
    rsbList = RsvpSearchRSB(node, psb);

    if (rsbList && rsbList->size)
    {
        ERROR_Assert(rsbList->first, "Invalid RSB");

        rsb = (RsvpResvStateBlock*) rsbList->first->data;
        interfaceIndex = rsb->outgoingLogicalInterface;
        nextHop = rsb->nextHopAddress;
    }

    sourceAddr = NetworkIpGetInterfaceAddress(node, interfaceIndex);

    // create the PathTear message
    msg = MESSAGE_Alloc(node,
                        TRANSPORT_LAYER,
                        TransportProtocol_RSVP,
                        MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(RsvpPathTearMessage),
                        TRACE_RSVP);

    // PathErr message pointer
    pathTearMessage = (RsvpPathTearMessage*)
        MESSAGE_ReturnPacket(msg);

    // create common header
    RsvpCreateCommonHeader(
        (RsvpCommonHeader*) pathTearMessage,
        RSVP_PATH_TEAR_MESSAGE,
        1,
        sizeof(RsvpPathTearMessage));

    // RSVP_SESSION_OBJECT object copied from all object structure
    memcpy(&pathTearMessage->session,
           &psb->session,
           sizeof(RsvpTeObjectSession));

    // create RSVP_HOP object
    pathTearMessage->rsvpHop = RsvpCreateRsvpHop(
                                   sourceAddr,
                                   interfaceIndex);

    // copy SENTER_TEMPLATE, RSVP_SENDER_TSPEC and RSVP_ADSPEC from
    // current PSB
    memcpy(&pathTearMessage->senderDescriptor.senderTemplate,
           &psb->senderTemplate,
           sizeof(RsvpTeObjectSenderTemplate));

    memcpy(&pathTearMessage->senderDescriptor.senderTSpec,
           &psb->senderTSpec,
           sizeof(RsvpObjectSenderTSpec));

    ERROR_Assert(interfaceIndex >= 0, "Invalid interface");

    // send the Path tear message to the previous HOP
    NetworkIpSendRawMessageToMacLayer(
        node,
        msg,
        sourceAddr,
        nextHop,        // destination address is the next hop
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_RSVP,
        1,              // TTL = 1, Path tear just goes to nexthop
        interfaceIndex,
        nextHop);       // next hop to reach the destination
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpForwardPathTearAndRefreshState
// PURPOSE      The function processes the RSVP_PATH_TEAR_MESSAGE and
//              forward it to the next RSVP node. Also update the state
//              block accordingly
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     msg:     message received by the layer
//     psb:     current PSB
//-------------------------------------------------------------------------
static
void RsvpForwardPathTearAndRefreshState(
    Node* node,
    Message* msg,
    RsvpPathStateBlock* psb)
{
    LinkedList* rsbList = NULL;
    RsvpResvStateBlock* cRSB = NULL;
    ListItem* item = NULL;

    int incInterface = ((NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg))->incomingInterfaceIndex;

    ERROR_Assert(psb->outInterfaceList, "Invalid interface");

    // if destination address is not the receiver address, then forward
    // PathTear message to the next hop
    if (psb->session.receiverAddress !=
            NetworkIpGetInterfaceAddress(node, incInterface))
    {
        // forward the message
        RsvpCreateAndSendPathTearMessage(node, psb);
    }

    // return list of RSBs for current PSB
    rsbList = RsvpSearchRSB(node, psb);

    ERROR_Assert(rsbList, "Invalid RSB list");

    item = rsbList->first;
    while (item)
    {
        cRSB = (RsvpResvStateBlock*) item->data;

        ERROR_Assert(cRSB, "Invalid RSB");

        if (RsvpObjectStyleGetStyleBits(cRSB->style.rsvpStyleBits)
            == RSVP_FF_STYLE
            || RsvpObjectStyleGetStyleBits(cRSB->style.rsvpStyleBits)
            == RSVP_SE_STYLE)
        {
            // Remove the RSVP_FILTER_SPEC that matches senderTemplate of
            // current PSB from the FilterSpecList of RSB
            RsvpRemoveFilterSpecFromRSB(
                node,
                cRSB,
                psb->senderTemplate);

            // current RSB contains no filter spec
            if (cRSB->filtSSList->size == 0)
            {
                // remove cRSB from the RSB list
                RsvpRemoveCurrentRSB(node, cRSB);
            }
        }
        else if (RsvpObjectStyleGetStyleBits(cRSB->style.rsvpStyleBits)
            == RSVP_WF_STYLE)
        {
            // cRSB matches with no other PSB, remove cRSB
            RsvpRemoveCurrentRSBForWFStyle(node, cRSB);
        }
        item = item->next;
    }

    ListFree(node, rsbList, FALSE);

    // TBD
    // if a RSB was found then execute event sequence UPDATE TRAFFIC CONTROL
    // to update the traffic control state, Will be implemented later.

    // remove old LSP
    RsvpRemoveOldLsp(node, psb);

    // delete current psb
    RsvpRemoveCurrentPSB(node, psb);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTearDownLastLSP
// PURPOSE      Search PSB and LSP to check whether path tearing is required
//              If so initiate the PathTear
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     msg:         message received by the layer
//     psb:     current PSB for which a new LSP is found
//-------------------------------------------------------------------------
static
void RsvpTearDownLastLSP(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    unsigned short lspId;
    RsvpPathStateBlock* tearingPSB;

    // find LSP for which egress node is same but different LSP. That means
    // there are more than one LSP. Previous one should be teared down
    switch
        (RsvpObjectStyleGetStyleBits(allObjects->style->rsvpStyleBits))
    {
        case RSVP_FF_STYLE:
        {
            ListItem* item = allObjects->FlowDescriptorList.
                ffDescriptor->first;

            RsvpFFDescriptor* ffDescriptor = NULL;

            // for each flow descriptor
            while (item)
            {
                // For each flow descriptor in the list
                ffDescriptor = (RsvpFFDescriptor*) item->data;

                ERROR_Assert(ffDescriptor, "Invalid Flow descriptor");

                lspId = RsvpSearchLSPForDuplicate(
                            node,
                            allObjects->session,
                            ffDescriptor->filtSS->filterSpec);

                // Find the PSB that is going to be teared down
                if (lspId)
                {
                    tearingPSB = RsvpSearchPSBForTearDown(
                                     node,
                                     lspId,
                                     allObjects->session,
                                     ffDescriptor->filtSS->filterSpec);

                    // PSB to be teared down is found and going to tear it
                    if (tearingPSB)
                    {
                        RsvpForwardPathTearAndRefreshState(
                            node,
                            msg,
                            tearingPSB);
                    }
                }
                item = item->next;
            }
            break;
        }

        case RSVP_SE_STYLE:
        {
            RsvpSEDescriptor* seDescriptor = (RsvpSEDescriptor*)
                allObjects->FlowDescriptorList.seDescriptor;

            ListItem* filtSSItem = seDescriptor->filtSS->first;

            while (filtSSItem)
            {
                RsvpFiltSS* filtSS = (RsvpFiltSS*) filtSSItem->data;

                lspId = RsvpSearchLSPForDuplicate(
                            node,
                            allObjects->session,
                            filtSS->filterSpec);

                // Find the PSB that is going to be teared down
                if (lspId)
                {
                    tearingPSB = RsvpSearchPSBForTearDown(
                                     node,
                                     lspId,
                                     allObjects->session,
                                     filtSS->filterSpec);

                    // PSB to be teared down is found and going to tear it
                    if (tearingPSB)
                    {
                        RsvpForwardPathTearAndRefreshState(
                            node,
                            msg,
                            tearingPSB);
                    }
                }
                filtSSItem = filtSSItem->next;
            }
            break;
        }

        case RSVP_WF_STYLE:
        {
            // not implemented, for traffic enginnering
            // it is not required
            ERROR_ReportError("Unknown reservation style");
            break;
        }

        default:
        {
            // unknown reservation style
            ERROR_Assert(FALSE, "Unknown reservation style");
            break;
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchRSBForCompatibleStyle
// PURPOSE      Search RSB for current session and finds any imcompatibility
//              style with the current message
//
// Return:          matching style or not
// Parameters:
//     node:        node which received the message
//     allObjects:  all objects in the message
//-------------------------------------------------------------------------
static
BOOL RsvpSearchRSBForCompatibleStyle(
    Node* node,
    RsvpAllObjects* allObjects)
{
    LinkedList* rsbList = RsvpSearchRSBForSessionOnly(
                        node,
                        allObjects->session);

    ListItem* item = rsbList->first;

    while (item)
    {
        RsvpResvStateBlock* rsb = (RsvpResvStateBlock*) item->data;

        if (RsvpObjectStyleGetStyleBits(rsb->style.rsvpStyleBits) !=
            RsvpObjectStyleGetStyleBits(allObjects->style->rsvpStyleBits))
        {
            ListFree(node, rsbList, FALSE);
            return FALSE;
        }
        item = item->next;
    }
    ListFree(node, rsbList, FALSE);
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessResvMessage
// PURPOSE      This function processes the RSVP_RESV_MESSAGE when an
//              intermediate RSVP node or the sender gets the
//              RSVP_RESV_MESSAGE
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     allObjects:  all objects in the message
//-------------------------------------------------------------------------
static
void RsvpProcessResvMessage(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    LinkedList* refreshPhopList = NULL;
    BOOL resvRefreshNeeded = FALSE;
    RsvpPathStateBlock* cPSB = NULL;

    ERROR_Assert(allObjects, "RSVP message is not parsed");

    // if session of the message does not match with any PSB, send ResvErr
    cPSB = RsvpSearchPSBForSessionOnly(node, allObjects->session);
    if (cPSB == NULL)
    {
        // error, send ResvErr message
        RsvpCreateAndSendResvErrMessage(
            node,
            msg,
            allObjects,
            RSVP_NO_PATH_INFO_FOR_RESV,
            0);

        // return to RsvpHandlePacket()
        // to drop and free the message
        return;
    }

    // If existing RSB has different style than current message send ResvErr
    if (!RsvpSearchRSBForCompatibleStyle(node, allObjects))
    {
        // reservation style error, send ResvErr message
        RsvpCreateAndSendResvErrMessage(
            node,
            msg,
            allObjects,
            RSVP_CONFLICTING_RESERVATION_STYLE,
            0);

        // return to RsvpHandlePacket() ...
        // to drop and free the message
        return;
    }

    ListInit(node, &refreshPhopList);

    // process flow descriptor list depending on the different style
    RsvpProcessResvFlowDescriptorList(
        node,
        msg,
        allObjects,
        refreshPhopList,
        &resvRefreshNeeded);

    if (resvRefreshNeeded)
    {
        ListItem* item = NULL;

        ERROR_Assert(refreshPhopList, "Invalid PHOP list");

        item = refreshPhopList->first;
        while (item)
        {
            NodeAddress *phopAddr = (NodeAddress *) item->data;

            // execute ResvRefresh sequence for each PHOP in PHOP list
            RsvpEnvokeResvRefresh(node, *phopAddr);
            item = item->next;
        }
    }

    // if this is the sender node, then check whether PathTear is required.
    // It will be required when another LSP was already established for this
    // sneder/receiver pair.

    if (cPSB->rsvpHop.nextPrevHop == 0)
    {
        RsvpTearDownLastLSP(node, msg, allObjects);
    }

    ListFree(node, refreshPhopList, FALSE);

    // return to RsvpHandlePacket() ...
    // to drop and free the message
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessPathErrMessage
// PURPOSE      This function processes the RSVP_PATH_ERR_MESSAGE when an
//              intermediate RSVP node or the sender gets the
//              RSVP_PATH_ERR_MESSAGE
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     allObjects:  all objects in the message
//-------------------------------------------------------------------------
static
void RsvpProcessPathErrMessage(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    RsvpPathStateBlock* cPSB = NULL;

    ERROR_Assert(allObjects, "RSVP message is not parsed");

    cPSB = RsvpSearchPSBSessionSenderTemplatePair(
               node,
               allObjects->session,
               allObjects->senderTemplate);

    if (cPSB == NULL)
    {
        // return to RsvpHandlePacket()
        // to drop and free the message
        return;
    }
    else
    {
        if (cPSB->rsvpHop.nextPrevHop == 0)
        {
            // receive upcall function pointer
            RsvpUpcallFunctionType upcallFunctionPtr = NULL;

            // create info parameter
            RsvpUpcallInfoParameter infoParm;

            infoParm.Info.PathError.errorCode =
                allObjects->errorSpec->errorCode;

            infoParm.Info.PathError.errorValue =
                allObjects->errorSpec->errorValue;

            infoParm.Info.PathError.errorNode =
                allObjects->errorSpec->errorNodeAddress;

            infoParm.Info.PathError.senderTemplate =
                allObjects->senderTemplate;

            // upcall to application
            if (upcallFunctionPtr)
            {
                (upcallFunctionPtr) (cPSB->session.tunnelId,
                                     PATH_ERROR,
                                     infoParm);
            }
        }
        else
        {
            // send a copy of the PathErr message to the PHOP IP address
            Message* pathErrMsg = MESSAGE_Duplicate(node, msg);

            NodeAddress routerAddr = NetworkIpGetInterfaceAddress(
                                         node,
                                         cPSB->incomingInterface);

            RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

            // counter of PathErr message sent incremented
            rsvp->rsvpStat->numPathErrMsgSent++;

            ERROR_Assert(cPSB->incomingInterface >= 0, "Invalid interface");

            // send the PathErr message to the previous HOP
            NetworkIpSendRawMessageToMacLayer(
                node,
                pathErrMsg,
                routerAddr,
                cPSB->rsvpHop.nextPrevHop,  // destination is previous hop
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_RSVP,
                1,                          // TTL = 1, PathErr goes one hop
                cPSB->incomingInterface,
                cPSB->rsvpHop.nextPrevHop); // next hop for destination
        }
        // return to RsvpHandlePacket()
        // to drop and free the message
        return;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchRSBFilterSpec
// PURPOSE      Search in the RSB filter_spec list for a RSVP_FILTER_SPEC
//              that matches from the ResvErr message.
//
// Return:              TRUE - if one RSVP_FILTER_SPEC found, else FALSE
// Parameters:
//     node:            node which received the message
//     rsbFiltSSList:   RSVP_FILTER_SPEC list in the current RSB
//     msgFiltSSList:   RSVP_FILTER_SPEC list in the current message
//-------------------------------------------------------------------------
static
BOOL RsvpSearchRSBFilterSpec(
    LinkedList* rsbFiltSSList,
    LinkedList* msgFiltSSList)
{
    ListItem* msgItem = NULL;

    ERROR_Assert(rsbFiltSSList, "Invalid RSB list");
    ERROR_Assert(msgFiltSSList, "Invalid RSB list");

    msgItem = msgFiltSSList->first;

    // go for the RSVP_FILTER_SPEC item in the ResvErr message
    while (msgItem)
    {
        ListItem* rsbItem = NULL;
        RsvpFiltSS* msgFiltSS = (RsvpFiltSS*) msgItem->data;

        ERROR_Assert(msgFiltSS, "Invalid filter spec");

        rsbItem = rsbFiltSSList->first;

        // go for the RSVP_FILTER_SPEC item in the RSB
        while (rsbItem)
        {
            RsvpFiltSS* rsbFiltSS = (RsvpFiltSS*) rsbItem->data;

            ERROR_Assert(rsbFiltSS, "Invalid filter spec");

            if ((msgFiltSS->filterSpec->senderAddress ==
                    rsbFiltSS->filterSpec->senderAddress)
                && (msgFiltSS->filterSpec->lspId ==
                    rsbFiltSS->filterSpec->lspId))
            {
                // matching found
                return TRUE;
            }
            rsbItem = rsbItem->next;
        }
        msgItem = msgItem->next;
    }
    // no matching of RSVP_FILTER_SPEC
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSearchRSBForInterfaceNotMatching
// PURPOSE      Search in the RSB for the current session and interface not
//              matching with incmoing interface of the message and
//              RSVP_FILTER_SPEC matches from the RSVP_FILTER_SPEC list
// ASSUMPTION   WF style is not considered for traffic engineering as
//              described the draft draft-ietf-mpls-rsvp-lsp-tunnel-08.txt.
//              Still codes are developed for WF style, that will be used
//              for RSVP only. So all the WF style related codes are not
//              tested.
//
// Return:          List of RSBs matching above criteria
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     allObjects:  all objects
//-------------------------------------------------------------------------
static
LinkedList* RsvpSearchRSBForInterfaceNotMatching(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    LinkedList* rsbList = NULL;
    BOOL isRsbFound = FALSE;
    RsvpResvStateBlock* rsb = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    // incmoing interface of the message received from info field
    int incInterface = ((NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg))->incomingInterfaceIndex;

    ListItem* item = NULL;

    ERROR_Assert(rsvp->rsb, "Invalid RSB");

    item = rsvp->rsb->first;

    ListInit(node, &rsbList);

    // Go through the RSB list
    while (item)
    {
        isRsbFound = FALSE;

        // get a RSB from the list
        rsb = (RsvpResvStateBlock*) item->data;

        ERROR_Assert(rsb, "Invalid RSB");

        // check the session object and incming interface differs from OI
        if ((rsb->session.receiverAddress ==
                allObjects->session->receiverAddress)
            && (rsb->session.tunnelId == allObjects->session->tunnelId)
            && (incInterface != rsb->outgoingLogicalInterface))
        {
            // now check the RSVP_FILTER_SPEC, it varies of different style,
            // consider different styles
            switch (RsvpObjectStyleGetStyleBits(
                allObjects->style->rsvpStyleBits))
            {
                case RSVP_FF_STYLE:
                {
                    ListItem* itemFlow = NULL;
                    RsvpFFDescriptor* ffDescriptor = NULL;
                    LinkedList* filterSpecList = NULL;
                    ERROR_Assert(allObjects->FlowDescriptorList.ffDescriptor,
                            "Invalid Flow descriptor");

                    itemFlow =
                        allObjects->FlowDescriptorList.ffDescriptor->first;

                    while (itemFlow)
                    {
                        // For each flow descriptor in the list
                        RsvpFiltSS* tempFiltSS;

                        ffDescriptor = (RsvpFFDescriptor*) itemFlow->data;

                        ERROR_Assert(ffDescriptor,
                                     "Invalid Flow descriptor");

                        // send the filter spec in a list
                        ListInit(node, &filterSpecList);

                        tempFiltSS = (RsvpFiltSS*)
                                     MEM_malloc(sizeof(RsvpFiltSS));

                        memcpy(tempFiltSS, ffDescriptor->filtSS, sizeof(RsvpFiltSS));

                        ListInsert(node,
                            filterSpecList,
                            node->getNodeTime(),
                            tempFiltSS);

                        if (RsvpSearchRSBFilterSpec(
                                rsb->filtSSList,
                                filterSpecList))
                        {
                            // matching found, prepare to insert in rsbList
                            isRsbFound = TRUE;
                        }
                        ListFree(node, filterSpecList, FALSE);
                        itemFlow = itemFlow->next;
                    }
                    break;
                }

                case RSVP_SE_STYLE:
                {
                    RsvpSEDescriptor* seDescriptor = (RsvpSEDescriptor*)
                        allObjects->FlowDescriptorList.seDescriptor;

                    ERROR_Assert(seDescriptor, "Invalid Flow descriptor");

                    if (RsvpSearchRSBFilterSpec(
                            rsb->filtSSList,
                            seDescriptor->filtSS))
                    {
                        // matching found, prepare to insert in rsbList
                        isRsbFound = TRUE;
                    }
                    break;
                }

                case RSVP_WF_STYLE:
                {
                    // RSVP_FILTER_SPEC checking not required. matching
                    // found, prepare to insert in rsbList
                    isRsbFound = TRUE;
                }

                default:
                {
                    // invalid style
                    ERROR_Assert(FALSE, "Unknown Reservation Style");
                    break;
                }
            }
        }

        if (isRsbFound)
        {
            RsvpResvStateBlock* newRSB
                = (RsvpResvStateBlock*)
                  MEM_malloc(sizeof(RsvpResvStateBlock));

            memcpy(newRSB, rsb, sizeof(RsvpResvStateBlock));
            ListInsert(node, rsbList, node->getNodeTime(), newRSB);
        }
        item = item->next;
    }
    return rsbList;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessResvErrMessage
// PURPOSE      This function processes the RSVP_RESV_ERR_MESSAGE when an
//              intermediate RSVP node or the receiver gets the
//              RSVP_RESV_ERR_MESSAGE
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     allObjects:  all objects in the message
//-------------------------------------------------------------------------
static
void RsvpProcessResvErrMessage(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    // The resource reservation part is not considered. So the updation of
    // BSB is not considered now. It must be handled at the time of
    // resource reservation
    LinkedList* rsbList = NULL;
    ListItem* item = NULL;
    RsvpPathStateBlock* cPSB = NULL;

    ERROR_Assert(allObjects, "RSVP message is not parsed");

    // if session of the message does not match with any PSB, drop message
    cPSB = RsvpSearchPSBForSessionOnly(node, allObjects->session);

    if (cPSB == NULL)
    {
        // return to RsvpHandlePacket()
        // to drop and free the message
        return;
    }

    rsbList = RsvpSearchRSBForInterfaceNotMatching(
                  node,
                  msg,
                  allObjects);


    ERROR_Assert(rsbList, "Invalid RSB");

    item = rsbList->first;

    while (item)
    {
        RsvpResvStateBlock* cRSB = (RsvpResvStateBlock*) item->data;

        ERROR_Assert(cRSB, "Invalid RSB");

        // NHOP in the RSB is the local API
        if (cRSB->nextHopAddress == 0)
        {
            // receive upcall function pointer
            RsvpUpcallFunctionType upcallFunctionPtr = NULL;

            // create info parameter
            RsvpUpcallInfoParameter infoParm;

            infoParm.Info.ResvError.errorCode =
                allObjects->errorSpec->errorCode;

            infoParm.Info.ResvError.errorValue =
                allObjects->errorSpec->errorValue;

            infoParm.Info.ResvError.errorNode =
                allObjects->errorSpec->errorNodeAddress;

            infoParm.Info.ResvError.errorFlag =
                allObjects->errorSpec->errorFlag;

            infoParm.Info.ResvError.flowSpec = &cRSB->flowSpec;
            infoParm.Info.ResvError.filterSpecList = cRSB->filtSSList;

            // upcall to application
            if (upcallFunctionPtr)
            {
                (upcallFunctionPtr) (cRSB->session.tunnelId,
                                     RESV_ERROR,
                                     infoParm);
            }
        }
        else
        {
            // create ResvErr message and send to NHOP
            // send a copy of the PathErr message to the PHOP IP address
            Message* resvErrMsg = MESSAGE_Duplicate(node, msg);

            NodeAddress routerAddr = NetworkIpGetInterfaceAddress(
                                         node,
                                         cRSB->outgoingLogicalInterface);

            RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

            // counter of PathErr message sent incremented
            rsvp->rsvpStat->numResvErrMsgSent++;

            ERROR_Assert(cRSB->outgoingLogicalInterface >= 0,
                    "Invalid interface");

            // send the PathErr message to the previous HOP
            NetworkIpSendRawMessageToMacLayer(
                node,
                resvErrMsg,
                routerAddr,
                cRSB->nextHopAddress,  // next hop is the destination
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_RSVP,
                1,                     // TTL = 1, PathErr goes one hop
                cRSB->outgoingLogicalInterface,
                cRSB->nextHopAddress); // next hop for destination
        }
        item = item->next;
    }
    // return to RsvpHandlePacket()
    // to drop and free the message
    ListFree(node, rsbList, FALSE);
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessPathTearMessage
// PURPOSE      The function processes the RSVP_PATH_TEAR_MESSAGE when an
//              intermediate RSVP node or the receiver gets the
//              RSVP_PATH_TEAR_MESSAGE
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     allObjects:  all objects in the message
//-------------------------------------------------------------------------
static
void RsvpProcessPathTearMessage(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    RsvpPathStateBlock* cPSB = NULL;

    ERROR_Assert(allObjects, "RSVP message is not parsed");

    cPSB = RsvpSearchPSBSessionSenderTemplatePair(
               node,
               allObjects->session,
               allObjects->senderTemplate);

    // if matching found
    if (cPSB == NULL)
    {
        // return to RsvpHandlePacket()
        // to drop and free the message
        return;
    }
    else
    {
        // generate PathTear for this PSB
        RsvpForwardPathTearAndRefreshState(node, msg, cPSB);
    }
    // return to RsvpHandlePacket()
    // to drop and free the message
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpRemoveFilterSpecIfRequired
// PURPOSE      Remove FiltSS from the filter spec list of RSB if current
//              filtSS is present in the list. This list consists of
//              RSVP_FILTER_SPEC, RSVP_LABEL_OBJECT and RSVP_RECORD_ROUTE
//
// Return:              Matching PSB, NULL if no matching
// Parameters:
//     node:            node which received the message
//     rsb:             new RSB block for the entry
//     filterSpecList:  list of filtSS
//-------------------------------------------------------------------------
static
void RsvpRemoveFilterSpecIfRequired(
    Node* node,
    RsvpResvStateBlock* rsb,
    LinkedList* filterSpecList)
{
    ListItem* item = NULL;

    ERROR_Assert(filterSpecList, "Invalid filter spec");
    item = filterSpecList->first;

    // for each filtSS in the list collected from the message
    while (item)
    {
        ListItem* itemRsb = NULL;
        RsvpFiltSS* filtSS = (RsvpFiltSS*) item->data;

        ERROR_Assert(filtSS, "Invalid filter spec");

        // search in the RSB for that filtSS
        itemRsb = rsb->filtSSList->first;
        while (itemRsb)
        {
            RsvpFiltSS* filtSSRsb = (RsvpFiltSS*) itemRsb->data;

            ERROR_Assert(filtSSRsb, "Invalid filter spec");

            // if current filtSS is found in RSB, then prepare to remove it
            if ((filtSS->filterSpec->senderAddress ==
                    filtSSRsb->filterSpec->senderAddress)
                && (filtSS->filterSpec->lspId == filtSSRsb->filterSpec->lspId))
            {
                ListItem* nextItem = itemRsb->next;
                ListGet(node, rsb->filtSSList, itemRsb, TRUE, FALSE);
                itemRsb = nextItem;
            }
            else
            {
                itemRsb = itemRsb->next;
            }
        }
        item = item->next;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessResvTearFlowDescriptor
// PURPOSE      Process each Flow descriptor for the current ResvTear message
// ASSUMPTION   This function is written but not tested. After the
//              implementation of resource reservation, this function will be
//              required
//
// Return:              None
// Parameters:
//     node:            node which received the message
//     allObjects:      All Objects
//     flowSpec:        RSVP_FLOWSPEC for current set
//-------------------------------------------------------------------------
static
void RsvpProcessResvTearFlowDescriptor(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects,
    LinkedList* filterSpecList)
{
    // Find RSB for current RSVP_SESSION_OBJECT and NHOP pair, if reservation
    // style is distinct then filter_spec also must match for the selection
    // Call it activeRSB
    RsvpResvStateBlock* activeRSB = RsvpSearchRSBForSessionNhop(
                                        node,
                                        allObjects->session,
                                        allObjects->rsvpHop,
                                        filterSpecList);

    if (activeRSB != NULL)
    {
        // search criteria found a RSB
        // If existing RSB has different style than the current message
        if (RsvpObjectStyleGetStyleBits(activeRSB->style.rsvpStyleBits)
            != RsvpObjectStyleGetStyleBits(allObjects->style->rsvpStyleBits))
        {
            return;
        }

        // Remove RSVP_FILTER_SPEC from current RSB which are in the message
        RsvpRemoveFilterSpecIfRequired(
            node,
            activeRSB,
            filterSpecList);

        ERROR_Assert(activeRSB->filtSSList, "Invalid filter spec");

        if (activeRSB->filtSSList->size == 0)
        {
            // delete activeRSB
            RsvpRemoveCurrentRSB(node, activeRSB);

            // TBD
            // Execute the UPDATE TRAFFIC CONTROL event to update the traffic
            // control state to be consistent with the reservation state. If
            // the result is to modify the traffic control state, the
            // Resv_Refresh_Needed flag will be turned on and a RESV_EVENT
            // upcall will be made to any local application.
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessResvTearFlowDescriptorList
// PURPOSE      The function processes flow descriptor list of ResvTear
//              message
// ASSUMPTION   This function is written but not tested. After the
//              implementation of resource reservation, this function will be
//              required.
//              WF style is not considered for traffic engineering as
//              described in the draft-ietf-mpls-rsvp-lsp-tunnel-08.txt.
//              Still codes are developed for WF style, that will be used
//              for RSVP only. So all the WF style related codes are not
//              tested.
//
// Return:          List of PSBs
// Parameters:
//     node:        node which received the message
//     msg:         receiving message in the node
//     allObjects:  all objects in the message
//-------------------------------------------------------------------------
static
LinkedList* RsvpProcessResvTearFlowDescriptorList(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    LinkedList* cPSBs = NULL;
    ListInit(node, &cPSBs);

    ERROR_Assert(allObjects, "RSVP message is not parsed");

    switch
        (RsvpObjectStyleGetStyleBits(allObjects->style->rsvpStyleBits))
    {
        case RSVP_FF_STYLE:
        {
            ListItem* item = NULL;
            RsvpFFDescriptor* ffDescriptor = NULL;
            LinkedList* filterSpecList = NULL;

            ERROR_Assert(allObjects->FlowDescriptorList.ffDescriptor,
                         "Invalid Flow descriptor");

            item = allObjects->FlowDescriptorList.ffDescriptor->first;

            while (item)
            {
                RsvpFiltSS* filtSS = NULL;

                // For each flow descriptor in the list
                ffDescriptor = (RsvpFFDescriptor*) item->data;

                ERROR_Assert(ffDescriptor, "Invalid flow descriptor");

                // send the filter spec in a list
                ListInit(node, &filterSpecList);

                filtSS = (RsvpFiltSS *) MEM_malloc(sizeof(RsvpFiltSS));

                memcpy(filtSS,
                       ffDescriptor->filtSS,
                       sizeof(RsvpFiltSS));

                ListInsert(node, filterSpecList, node->getNodeTime(), filtSS);

                RsvpLocatePSBForOIAndFilterSpec(
                    node,
                    msg,
                    ffDescriptor->filtSS,
                    cPSBs);

                RsvpProcessResvTearFlowDescriptor(
                    node,
                    msg,
                    allObjects,
                    filterSpecList);

                ListFree(node, filterSpecList, FALSE);
                item = item->next;
            }
            break;
        }

        case RSVP_SE_STYLE:
        {
            RsvpSEDescriptor* seDescriptor = (RsvpSEDescriptor*)
                allObjects->FlowDescriptorList.seDescriptor;

            ListItem* item = NULL;

            ERROR_Assert(seDescriptor, "Invalid flow descriptor");

            item = seDescriptor->filtSS->first;

            while (item)
            {
                RsvpFiltSS* filtSS = (RsvpFiltSS*) item->data;

                RsvpLocatePSBForOIAndFilterSpec(
                    node,
                    msg,
                    filtSS,
                    cPSBs);

                item = item->next;
            }

            RsvpProcessResvTearFlowDescriptor(
                node,
                msg,
                allObjects,
                seDescriptor->filtSS);

            break;
        }

        case RSVP_WF_STYLE:
        {
            RsvpLocatePSBForOIAndFilterSpec(
                node,
                msg,
                NULL,
                cPSBs);

            RsvpProcessResvTearFlowDescriptor(
                node,
                msg,
                allObjects,
                NULL);

            break;
        }

        default:
        {
            //invalid style
            ERROR_Assert(FALSE, "Unknown reservation style");
            break;
        }
    }
    return cPSBs;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpSendResvTearMessage
// PURPOSE      The function creates and send ResvTear message
//
// Return:           None
// Parameters:
//      node:               node which received the message
//      resvTearMessage:    message received by the layer
//      interfaceIndex:
//      ipAddr:
//-------------------------------------------------------------------------
static
void RsvpSendResvTearMessage(
    Node* node,
    Message* resvTearMessage,
    int interfaceIndex,
    NodeAddress ipAddr)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    rsvp->rsvpStat->numResvTearMsgSent++;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpProcessResvTearMessage
// PURPOSE      The function processes the RSVP_RESV_TEAR_MESSAGE when an
//              intermediate RSVP node or the sender gets the
//              RSVP_RESV_TEAR_MESSAGE
// ASSUMPTION   This function is written but not tested. After the
//              implementation of resource reservation, this function will be
//              required
//
// Return:          Matching PSB, NULL if no matching
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     allObjects:  all objects in the message
//-------------------------------------------------------------------------
static
void RsvpProcessResvTearMessage(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    BOOL resvRefreshNeeded = FALSE;
    LinkedList* refreshPhopList = NULL;
    ListItem* item = NULL;

    NodeAddress phopAddr = 0;
    NodeAddress lastPhopAddr = 0;
    BOOL isRsbFound = TRUE;

    // Process flow descriptor and return PSB list for which current
    // interface matches with outInterfaceList of PSB and
    // RSVP_SENDER_TEMPLATE matches with
    // one RSVP_FILTER_SPEC list in ResvTear message
    LinkedList* psbList = RsvpProcessResvTearFlowDescriptorList(
                        node,
                        msg,
                        allObjects);

    ERROR_Assert(psbList, "Invalid PSB list");

    item = psbList->first;
    ListInit(node, &refreshPhopList);

    while (item)
    {
        LinkedList* rsbList = NULL;
        RsvpPathStateBlock* psb = (RsvpPathStateBlock*) item->data;

        ERROR_Assert(psb, "Invalid PSB");

        // search a RSB on the same outgoing interface and whose
        // filterSpecList includes RSVP_SENDER_TEMPLATE of PSB
        rsbList = RsvpSearchRSB(node, psb);

        ERROR_Assert(rsbList, "Invalid RSB list");

        if (rsbList->size)
        {
            NodeAddress* phopPtr = (NodeAddress*)
                    MEM_malloc(sizeof(NodeAddress));

            // Add PHOP of the PSB to refreshPhopList
            phopAddr = psb->rsvpHop.nextPrevHop;

            *phopPtr = phopAddr;

            ListInsert(node,
                       refreshPhopList,
                       node->getNodeTime(),
                       phopPtr);
        }
        else
        {
            // RSB not found
            isRsbFound = FALSE;
        }

        // delete memory for rsbList
        ListFree(node, rsbList, FALSE);

        if (phopAddr != lastPhopAddr)
        {
            // create and send newly created rsvpTearMsg
            NodeAddress ipAddr;
            Message* newResvTearMsg = MESSAGE_Duplicate(node, msg);

            if (!isRsbFound)
            {
                // TBD
                // should add current flow spec in the message. not clear
            }

            ipAddr = NetworkIpGetInterfaceAddress(
                         node,
                         psb->incomingInterface);

            RsvpSendResvTearMessage(
                node,
                newResvTearMsg,
                psb->incomingInterface,
                ipAddr);

            lastPhopAddr = phopAddr;
        }
    }

    if (resvRefreshNeeded == TRUE)
    {
        // execute RESV REFRESH sequence for each PHOP in refreshPhopList
        ListItem* item = refreshPhopList->first;

        while (item)
        {
            NodeAddress *phopAddr = (NodeAddress *) item->data;

            // execute ResvRefresh sequence for each PHOP in PHOP list
            RsvpEnvokeResvRefresh(node, *phopAddr);
            item = item->next;
        }
    }

    ListFree(node, refreshPhopList, FALSE);

    // return to RsvpHandlePacket()
    // to drop and free the message
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateHello
// PURPOSE      The function creates a RSVP_HELLO REQUEST or ACK object
//
// Return:              None
// Parameters:
//     hello:           RSVP_HELLO object pointer
//     srcInstance:     srcInstance for the sender
//     dstInstance:     dstInstance for the neighbor
//     helloObjectType: RSVP_HELLO object type
//-------------------------------------------------------------------------
static
void RsvpCreateHello(
    RsvpTeObjectHello* hello,
    unsigned int srcInstance,
    unsigned int dstInstance,
    RsvpTeHelloObjectType helloObjectType)
{
    RsvpCreateObjectHeader(
        &hello->objectHeader,
        sizeof(RsvpTeObjectHello),
        RSVP_HELLO,
        (unsigned char) helloObjectType);

    // assign instance of sender and destination
    hello->srcInstance = srcInstance;
    hello->dstInstance = dstInstance;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTeCreateAndSendHelloMessage
// PURPOSE      The function creates and send Hello message
//
// Return:              None
// Parameters:
//      node:           node which received the message
//      neighAddr:      neighbor address
//      interfaceId:    interface ID at which message will be sent
//      srcInstance:    srcInstance for the sender
//      dstInstance:    dstInstance for the neighbor
//      helloObjectType:RSVP_HELLO object type
//-------------------------------------------------------------------------
static
void RsvpTeCreateAndSendHelloMessage(
    Node* node,
    int interfaceId,
    unsigned int srcInstance,
    unsigned int dstInstance,
    RsvpTeHelloObjectType helloObjectType)
{
    RsvpTeHelloMessage* helloMessage = NULL;
    NodeAddress sourceAddr = NetworkIpGetInterfaceAddress(node, interfaceId);

    // create the hello message
    Message* msg = MESSAGE_Alloc(node,
                                 TRANSPORT_LAYER,
                                 TransportProtocol_RSVP,
                                 MSG_TRANSPORT_FromAppSend);

    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(RsvpTeHelloMessage),
                        TRACE_RSVP);

    helloMessage = (RsvpTeHelloMessage*) MESSAGE_ReturnPacket(msg);

    RsvpCreateCommonHeader(
        (RsvpCommonHeader*) helloMessage,
        RSVP_HELLO_MESSAGE,
        1,
        sizeof(RsvpTeHelloMessage));

    RsvpCreateHello(
        &helloMessage->hello,
        srcInstance,
        dstInstance,
        helloObjectType);

    ERROR_Assert(interfaceId >= 0, "Invalid interface");

    if (RSVP_DEBUG_HELLO)
    {
        printf("node %u is sending a hello message through interface %d\n",
               node->nodeId,
               interfaceId);
    }

    // send the hello message to the destination address
    NetworkIpSendRawMessageToMacLayer(
        node,
        msg,
        sourceAddr,
        ANY_IP,       // destination address
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_RSVP,
        1,            // TTL = 1 hello nessage will go 1 hop only.
        interfaceId,
        ANY_IP);      // next hop address
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateHelloList
// PURPOSE      The function creates a Hello list structure for current hello
//
// Return:              None
// Parameters:
//      node:           node which received the message
//      neighAddr:      Neighbor address from where RSVP_HELLO received
//      interfaceIndex: interface at which RSVP_HELLO received
//      thisSrcIns:     Receiving node srcInstance
//      neighSrcIns:    Sending node srcInstance
//-------------------------------------------------------------------------
static
void RsvpCreateHelloList(
    Node* node,
    NodeAddress neighAddr,
    int interfaceIndex,
    unsigned int thisSrcIns,
    unsigned int neighSrcIns)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    RsvpTeHelloExtension* hello
        = (RsvpTeHelloExtension *) MEM_malloc(sizeof(RsvpTeHelloExtension));

    hello->neighborAddr = neighAddr;
    hello->interfaceIndex = interfaceIndex;
    hello->srcInstance = thisSrcIns;
    hello->dstInstance = neighSrcIns;
    hello->nonMatchingCount = 0;
    hello->isFailureDetected = FALSE;

    // insert in the hello list
    ListInsert(node, rsvp->helloList, node->getNodeTime(), hello);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTeSearchHelloInstance
// PURPOSE      When a node receives Hello ack message from its neighbor
//
// Return:              TRUE normally, FALSE for failure detected
// Parameters:
//     node:            node which received the message
//     neighAddr:       neighbor address
//     interfaceIndex:  interface index at which message comes
//     neighSrcIns:     neighbor srcInstance
//     thisSrcIns:      this node srcInstance
//-------------------------------------------------------------------------
static
BOOL RsvpTeSearchHelloInstance(
    Node* node,
    NodeAddress neighAddr,
    int interfaceIndex,
    unsigned int* neighSrcIns,
    unsigned int* thisSrcIns)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = NULL;
    RsvpTeHelloExtension* hello = NULL;

    ERROR_Assert(rsvp->helloList, "Invalid hello list");

    item = rsvp->helloList->first;

    while (item)
    {
        hello = (RsvpTeHelloExtension*) item->data;

        if (hello->interfaceIndex == interfaceIndex)
        {
            hello->neighborAddr = neighAddr;

            // check for the dstInstance
            if (!hello->dstInstance)
            {
                // still dstInstance is 0
                if (*neighSrcIns == 0)
                {
                    // if neighSrcInstance is 0 then create a new instance
                    *neighSrcIns = rsvp->nextAvailableInstance++;
                }
                // neighSrcInstance set to the dstInstance
                hello->dstInstance = *neighSrcIns;
            }
            else
            {
                // dstInstance is non zero
                if (hello->dstInstance != *neighSrcIns)
                {
                    // not matching with neighSrcIns. failure detected
                    hello->isFailureDetected = TRUE;
                    return FALSE;
                }
            }

            // check for the srcInstance
            if (!hello->srcInstance)
            {
                // still srcInstance is 0
                if (*thisSrcIns == 0)
                {
                    // if SrcInstance is 0 then create a new instance
                    *thisSrcIns = rsvp->nextAvailableInstance++;
                }
                // set SrcInstance
                hello->srcInstance = *thisSrcIns;
            }
            else
            {
                // if current srcInstance is 0, set it from hello structure
                if (*thisSrcIns == 0)
                {
                    *thisSrcIns = hello->srcInstance;
                }

                // srcInstance is non zero
                if (hello->srcInstance != *thisSrcIns)
                {
                    // not matching with SrcInstance. failure detected
                    hello->nonMatchingCount++;

                    // after max interval of not matching, failure detected
                    if (hello->nonMatchingCount >= MAX_NON_MATCHING_COUNT)
                    {
                        hello->isFailureDetected = TRUE;
                        return FALSE;
                    }
                }
                else
                {
                    hello->nonMatchingCount = 0;
                }
            }

            // neigh entry already there and no error detected
            hello->isFailureDetected = FALSE;
            return TRUE;
        }
        item = item->next;
    }

    // still srcInstance is 0
    if (*thisSrcIns == 0)
    {
        // if SrcInstance is 0 then create a new instance
        *thisSrcIns = rsvp->nextAvailableInstance++;
    }

    // neighbor entry is not in the list, now going to create a new one
    RsvpCreateHelloList(
        node,
        neighAddr,
        interfaceIndex,
        *thisSrcIns,
        *neighSrcIns);

    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTeProcessHelloRequest
// PURPOSE      When a node receives Hello request message from its neighbor
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     helloRequest:pointer to the hello object
//-------------------------------------------------------------------------
static
void RsvpTeProcessHelloRequest(
    Node* node,
    Message* msg,
    RsvpTeObjectHello* helloRequest)
{
    Message* refreshHelloMsg = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    NetworkToTransportInfo* info =
        (NetworkToTransportInfo*) MESSAGE_ReturnInfo(msg);

    NodeAddress neighAddr = GetIPv4Address(info->sourceAddr);

    int interfaceIndex = info->incomingInterfaceIndex;

    unsigned int neighSrcIns = helloRequest->srcInstance;
    unsigned int thisSrcIns = helloRequest->dstInstance;

    // search in the hello List to find the entry for the current neightbor
    // If instance values are not yet given, add them properly. If any
    // mismatching occurs, return FALSE as the indication of failure
    // detected.

    if (RSVP_DEBUG_HELLO)
    {
        char neighborAddress[MAX_IP_STRING_LENGTH] = {0};
        NodeAddress nodeId2 = MAPPING_GetNodeIdFromInterfaceAddress(
                                  node,
                                  neighAddr);

        IO_ConvertIpAddressToString(neighAddr,neighborAddress);

        printf("node %u has reseived a hello message from node %d\n"
               " whose addres is %s\n",
               node->nodeId,
               nodeId2,
               neighborAddress);
    }

    if (RsvpTeSearchHelloInstance(
            node,
            neighAddr,
            interfaceIndex,
            &neighSrcIns,
            &thisSrcIns))
    {
        int interfaceId = NetworkIpGetInterfaceIndexForNextHop(
                              node,
                              neighAddr);

        rsvp->rsvpStat->numHelloAckMsgSent++;

        // create RSVP_HELLO_ACK message and send
        RsvpTeCreateAndSendHelloMessage(
            node,
            interfaceId,
            thisSrcIns,
            neighSrcIns,
            RSVP_HELLO_ACK);
    }

    // check whether RSVP_HELLO adjacency is already met
    if (node->numberInterfaces > rsvp->helloList->size)
    {
        // Set timer MSG_TRANSPORT_RSVP_HelloExtension for hello adjacency
        refreshHelloMsg = MESSAGE_Alloc(node,
                                        TRANSPORT_LAYER,
                                        TransportProtocol_RSVP,
                                        MSG_TRANSPORT_RSVP_HelloExtension);

        // send current PSB pointer in the info field
        MESSAGE_Send(node, refreshHelloMsg, RSVP_HELLO_TIMER_DELAY);
    }
    // return to RsvpHandlePacket()
    // to drop and free the message
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTeProcessHelloMessage
// PURPOSE      When a node receives Hello ack message from its neighbor
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     helloRequest:pointer to the hello object
//-------------------------------------------------------------------------
static
void RsvpTeProcessHelloAck(
    Node* node,
    Message* msg,
    RsvpTeObjectHello* helloAck)
{
    NetworkToTransportInfo* info =
        (NetworkToTransportInfo*) MESSAGE_ReturnInfo(msg);

    NodeAddress neighAddr = GetIPv4Address(info->sourceAddr);

    int interfaceIndex = info->incomingInterfaceIndex;

    unsigned int neighSrcIns = helloAck->srcInstance;
    unsigned int thisSrcIns = helloAck->dstInstance;

    // search in the hello List to find the entry for the current neightbor
    // If instance values are not yet given, add them properly. If any
    // mismatching occurs, return FALSE as the indication of failure
    // detected.

    RsvpTeSearchHelloInstance(
        node,
        neighAddr,
        interfaceIndex,
        &neighSrcIns,
        &thisSrcIns);

    // return to RsvpHandlePacket()
    // to drop and free the message
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTeProcessHelloMessage
// PURPOSE      This function processes the RSVP_HELLO_MESSAGE when a RSVP
//              node gets the RSVP_HELLO_MESSAGE
//
// Return:          None
// Parameters:
//     node:        node which received the message
//     msg:         message received by the layer
//     allObjects:  pointer to the all objects
//-------------------------------------------------------------------------
static
void RsvpTeProcessHelloMessage(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    ERROR_Assert(allObjects && allObjects->hello,
                 "Invalid RSVP_HELLO message");

    switch (allObjects->hello->objectHeader.cType)
    {
        case RSVP_HELLO_REQUEST:
        {
            rsvp->rsvpStat->numHelloRequestMsgReceived++;

            // process Hello message for REQUEST object
            RsvpTeProcessHelloRequest(node, msg, allObjects->hello);
            break;
        }

        case RSVP_HELLO_ACK:
        {
            rsvp->rsvpStat->numHelloAckMsgReceived++;
            // process Hello message for ACK object
            RsvpTeProcessHelloAck(node, msg, allObjects->hello);
            break;
        }

        default:
        {
            // invalid type
            ERROR_Assert(FALSE, "Invalid RSVP_HELLO message.");
            break;
        }
    } // end switch (allObjects->hello->objectHeader.cType)
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpIsCorrectDestAddress
// PURPOSE      This function is called from RsvpIsValidMessage to
//              check the destination address of the packet matches with any
//              one of the interface address of the current node
//
// Return:          TRUE if correct destination address, else FALSE
// Parameters:
//     node:        node which received the message
//     destAddress: destination address of the packet
//-------------------------------------------------------------------------
static
BOOL RsvpIsCorrectDestAddress(Node* node, NodeAddress destAddress)
{
    int index = -1;

    for (index = 0; index < node->numberInterfaces; index++)
    {
        if (NetworkIpGetInterfaceAddress(node, index) == destAddress)
        {
            // dest. address matches with any one of the interface address
            return TRUE;
        }
    }
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpIsValidMessage
// PURPOSE      This function is called from RsvpHandlePacket for
//              general validation of the message
//
// Return:      TRUE if valid message otherwise FALSE
// Parameters:
//     node:    node which received the message
//     msg:     message received by the layer
//-------------------------------------------------------------------------
static
BOOL RsvpIsValidMessage(Node* node, Message* msg)
{
    RsvpCommonHeader* commonHeader = (RsvpCommonHeader*)
        MESSAGE_ReturnPacket(msg);

    NetworkToTransportInfo* info = (NetworkToTransportInfo*)
        MESSAGE_ReturnInfo(msg);

    NodeAddress destAddress = GetIPv4Address(info->destinationAddr);

    ERROR_Assert(commonHeader, "Invalid message type");

    // version must be 1
    if (RsvpCommonHeaderGetVersionNum(commonHeader->rsvpCh) != 1)
    {
        return FALSE;
    }

    if ((commonHeader->msgType != RSVP_PATH_MESSAGE) &&
        (commonHeader->msgType != RSVP_PATH_TEAR_MESSAGE) &&
        (commonHeader->msgType != RSVP_RESV_CONF_MESSAGE) &&
        (commonHeader->msgType != RSVP_HELLO_MESSAGE))
    {
        if (!RsvpIsCorrectDestAddress(node, destAddress))
        {
            // send the message to the proper destination address
            NetworkIpReceivePacketFromTransportLayer(
                node,
                msg,
                GetIPv4Address(info->sourceAddr),
                destAddress,
                ANY_INTERFACE,
                info->priority,
                IPPROTO_RSVP,
                FALSE);

            return FALSE;
        }
    }
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpParseFiltSS
// PURPOSE      Function is called from RsvpParseFlowDescriptorList
//              Validate the flow descriptor structure in the messages and
//              collect them in a proper structure, which will be used later.
//
// Return:          Length of the FiltSS, 0 if parsing failed.
// Parameters:
//     node:        node which received the message
//     packetPtr:   pointer to pointer of the packet
//     filtSS:      one filter spec set
//     style:       reservation style
//-------------------------------------------------------------------------
static
unsigned short RsvpParseFiltSS(
    unsigned char** packetPtr,
    RsvpFiltSS** filtSS,
    RsvpAllObjects* allObjects,
    RsvpStyleType style)
{
    RsvpObjectHeader* objectHeader = ((RsvpObjectHeader*) *packetPtr);
    unsigned short objectLength = 0;

    ERROR_Assert(filtSS, "Wrong RSVP message format");

    // allocate memory for filt ss
    *filtSS = (RsvpFiltSS *) MEM_malloc(sizeof(RsvpFiltSS));

    // Initialise FiltSS, it is already allocated in the previous function
    memset(*filtSS, 0, sizeof(RsvpFiltSS));

    // parse FiltSS, consists of RSVP_FILTER_SPEC, RSVP_LABEL_OBJECT and
    // optional RECORDROUTE
    while ((objectHeader->classNum == RSVP_FILTER_SPEC)
        || (objectHeader->classNum == RSVP_LABEL_OBJECT)
        || (objectHeader->classNum == RSVP_RECORD_ROUTE))
    {
        switch (objectHeader->classNum)
        {
            case RSVP_FILTER_SPEC:
            {
                if ((*filtSS)->filterSpec != NULL)
                {
                    if (style == RSVP_SE_STYLE)
                    {
                        // For SE style: second RSVP_FILTER_SPEC indicates
                        // the next FiltSS. Further processing done.
                        return objectLength;
                    }
                    else
                    {
                        // For FF style: already filter spec processed,
                        // duplicate object, message discarded
                        return 0;
                    }
                }

                // collect RSVP_FILTER_SPEC in FiltSS
                (*filtSS)->filterSpec = (RsvpTeObjectFilterSpec*)
                    MEM_malloc(sizeof(RsvpTeObjectFilterSpec));

                memcpy((*filtSS)->filterSpec,
                       *packetPtr,
                       sizeof(RsvpTeObjectFilterSpec));
                break;
            }

            case RSVP_LABEL_OBJECT:
            {
                if ((*filtSS)->label != NULL)
                {
                    // already label processed, duplicate object
                    return 0;
                }

                // collect RSVP_LABEL_OBJECT in FiltSS
                (*filtSS)->label = (RsvpTeObjectLabel*)
                    MEM_malloc(sizeof(RsvpTeObjectLabel));

                memcpy((*filtSS)->label,
                        *packetPtr,
                        sizeof(RsvpTeObjectLabel));
                break;
            }

            case RSVP_RECORD_ROUTE:
            {
                // If RSVP_RECORD_ROUTE already found ignore the second one
                if ((*filtSS)->recordRoute == NULL)
                {
                    // collect RSVP_RECORD_ROUTE in FiltSS
                    (*filtSS)->recordRoute = MEM_malloc(objectHeader->objectLength);

                    memcpy((*filtSS)->recordRoute,
                            *packetPtr,
                            objectHeader->objectLength);
                }
                else
                {
                    return 0;
                }
                break;
            }

            default:
            {
                // invalid object appears in FiltSS , message discarded
                ERROR_Assert(FALSE, "invalid object appears in FiltSS");
                return 0;
            }
        }
        objectLength = (short) (objectLength + objectHeader->objectLength);
        *packetPtr += objectHeader->objectLength;
        objectHeader = ((RsvpObjectHeader*) *packetPtr);

        if ((*packetPtr - allObjects->packetInitialPtr)
            >= allObjects->commonHeader->rsvpLength)
        {
            break;
        }
    }
    return objectLength;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpParseFlowDescriptorList
// PURPOSE      This function is called from RsvpParseAndFindObjects.
//              It is assumed that RSVP_STYLE object is preceding the flow
//              descriptor. For different type of reservation style now the
//              packet is parsed and gets different objects in the flow
//              descriptor. If any error found while parsing returns 0.
//              Otherwise properly update flowDescriptor variable in
//              RsvpAllObjects structure. The length of the objects
//              are also maintained and it is returned to indicate the total
//              length of the flow descriptor
// ASSUMPTION   WF style is not considered for traffic engineering as
//              described in the draft-ietf-mpls-rsvp-lsp-tunnel-08.txt.
//              Still codes are developed for WF style, that will be used
//              for RSVP only. So all the WF style related codes are not
//              tested.
//
// Return:          Length of the FlowDescriptor, 0 if parsing failed.
// Parameters:
//     node:        node which received the message
//     packetPtr:   pointer to the packet
//     allObjects:  all RSVP objects collected from the message
//-------------------------------------------------------------------------
static
unsigned short RsvpParseFlowDescriptorList(
    Node *node,
    unsigned char** packetPtr,
    RsvpAllObjects* allObjects)
{
    RsvpObjectHeader* objectHeader = ((RsvpObjectHeader*) *packetPtr);
    unsigned short objectLength = 0;
    unsigned short returnLength = 0;

    ERROR_Assert(allObjects, "RSVP message is not parsed");
    ERROR_Assert(objectHeader, "Wrong RSVP object format");

    switch (RsvpObjectStyleGetStyleBits(allObjects->style->rsvpStyleBits))
    {
        case RSVP_FF_STYLE:
        {
            // <FF flow descriptor list> ::= <RSVP_FLOWSPEC>
            // <RSVP_FILTER_SPEC> <RSVP_LABEL_OBJECT> [<RSVP_RECORD_ROUTE>]
            // |<FF flow descriptor list> <FF flow descriptor>
            // <FF flow descriptor> ::= [<RSVP_FLOWSPEC>] <RSVP_FILTER_SPEC>
            // <RSVP_LABEL_OBJECT> [<RSVP_RECORD_ROUTE>]

            RsvpFFDescriptor* ffDescriptor = NULL;

            // For each RSVP_FLOWSPEC
            ffDescriptor = (RsvpFFDescriptor*)
                MEM_malloc(sizeof(RsvpFFDescriptor));

            memset(ffDescriptor, 0, sizeof(RsvpFFDescriptor));

            // collect RSVP_FLOWSPEC
            ffDescriptor->flowSpec = (RsvpObjectFlowSpec*)
                MEM_malloc(sizeof(RsvpObjectFlowSpec));

            memcpy(ffDescriptor->flowSpec, *packetPtr, sizeof(RsvpObjectFlowSpec));

            // go for the next object
            objectLength = (short) (objectLength
                + objectHeader->objectLength);

            *packetPtr += objectHeader->objectLength;
            objectHeader = (RsvpObjectHeader*) *packetPtr;

            // for each RSVP_FLOWSPEC collect FiltSS
            returnLength = RsvpParseFiltSS(
                               packetPtr,
                               &ffDescriptor->filtSS,
                               allObjects,
                               RSVP_FF_STYLE);

            if (returnLength)
            {
                objectLength = (short) (objectLength + returnLength);
                objectHeader = (RsvpObjectHeader*) *packetPtr;
            }
            else
            {
                return 0;
            }

            // Insert current RSVP_FLOWSPEC and FiltSS pair in current
            // allObject list

            ListInsert(
                node,
                allObjects->FlowDescriptorList.ffDescriptor,
                node->getNodeTime(),
                ffDescriptor);

            break;
        }

        case RSVP_SE_STYLE:
        {
            // <SE flow descriptor> ::= <RSVP_FLOWSPEC> <SE filter spec list>
            // <SE filter spec list> ::= <SE filter spec> |
            // <SE filter spec list> <SE filter spec>
            // <SE filter spec> ::= <RSVP_FILTER_SPEC> <RSVP_LABEL_OBJECT>
            // [ <RSVP_RECORD_ROUTE> ]

            RsvpSEDescriptor* seDescriptor = NULL;
            RsvpFiltSS* filtSS = NULL;

            seDescriptor = (RsvpSEDescriptor*)
                MEM_malloc(sizeof(RsvpSEDescriptor));

            memset(seDescriptor, 0, sizeof(RsvpSEDescriptor));

            // collect RSVP_FLOWSPEC for SE style
            seDescriptor->flowSpec = (RsvpObjectFlowSpec*)
                MEM_malloc(sizeof(RsvpObjectFlowSpec));

            memcpy(seDescriptor->flowSpec, *packetPtr, sizeof(RsvpObjectFlowSpec));

            // search for the next object
            objectLength = (short) (objectLength
                + objectHeader->objectLength);

            *packetPtr += objectHeader->objectLength;

            // init of filtSS list
            ListInit(node, &seDescriptor->filtSS);

            // search for the list of RSVP_FILTER_SPEC
            while ((*packetPtr - allObjects->packetInitialPtr)
                        < allObjects->commonHeader->rsvpLength &&
                   ((RsvpObjectHeader*)
                         *packetPtr)->classNum == RSVP_FILTER_SPEC)
            {
                // Collect one FiltSS and insert in the list of FiltSS
                returnLength = RsvpParseFiltSS(
                                   packetPtr,
                                   &filtSS,
                                   allObjects,
                                   RSVP_SE_STYLE);

                if (returnLength)
                {
                    objectLength = (short) (objectLength + returnLength);
                }
                else
                {
                    return 0;
                }

                ListInsert(
                    node,
                    seDescriptor->filtSS,
                    node->getNodeTime(),
                    filtSS);
            }
            allObjects->FlowDescriptorList.seDescriptor = seDescriptor;
            break;
        }

        case RSVP_WF_STYLE:
        {
            // <WF flow descriptor> ::= <RSVP_FLOWSPEC>
            allObjects->FlowDescriptorList.flowSpec = (RsvpObjectFlowSpec*)
                MEM_malloc(sizeof(RsvpObjectFlowSpec));

            memcpy(allObjects->FlowDescriptorList.flowSpec,
                   *packetPtr,
                   sizeof(RsvpObjectFlowSpec));

            objectLength = (short) (objectLength
                + objectHeader->objectLength);

            *packetPtr += objectHeader->objectLength;
            break;
        }

        default:
        {
            // invalid style
            ERROR_Assert(FALSE, "Unknown reservation style");
            return 0;
        }
    }
    return objectLength;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCheckRequiredObject
// PURPOSE      This function is called from RsvpParseAndFindObjects.
//              It validates the required objects in the message. If the
//              required object is missing, the function gives parsing error
//              by returning FALSE.
//
// Return:          None
// Parameters:
//     msgType:     type of RSVP message
//     allObjects:  all RSVP objects collected from the message
//-------------------------------------------------------------------------
static
BOOL RsvpCheckRequiredObject(
    RsvpMsgType msgType,
    RsvpAllObjects* allObjects)
{
    ERROR_Assert(allObjects, "RSVP message is not parsed");

    switch (msgType)
    {
        case RSVP_PATH_MESSAGE:
        {
            // <Path Message> ::= <Common Header> [ <RSVP_INTEGRITY> ]
            // <RSVP_SESSION_OBJECT> <RSVP_HOP> <RSVP_TIME_VALUES>
            // [ <RSVP_EXPLICIT_ROUTE> ] <RSVP_LABEL_REQUEST>
            // [ <RSVP_SESSION_ATTRIBUTE> ] <sender descriptor>
            // <sender descriptor> ::=  <RSVP_SENDER_TEMPLATE>
            // <RSVP_SENDER_TSPEC> [ <RSVP_ADSPEC> ] [ <RSVP_RECORD_ROUTE> ]

            if ((allObjects->session == NULL)
                || (allObjects->timeValues == NULL)
                || (allObjects->rsvpHop == NULL)
                || (allObjects->senderTemplate == NULL)
                || (allObjects->senderTSpec == NULL)
                || (allObjects->labelRequest == NULL))
            {
                return FALSE;
            }
            break;
        }

        case RSVP_RESV_MESSAGE:
        {
            // <Resv Message> ::=  <Common Header> [ <RSVP_INTEGRITY> ]
            // <RSVP_SESSION_OBJECT> <RSVP_HOP> <RSVP_TIME_VALUES>
            // [ <RESV_CONFIRM> ] [ <RSVP_SCOPE> ] <RSVP_STYLE>
            // <flow descriptor list>

            if ((allObjects->session == NULL)
                || (allObjects->timeValues == NULL)
                || (allObjects->rsvpHop == NULL)
                || (allObjects->style == NULL))
            {
                return FALSE;
            }
            break;
        }

        case RSVP_PATH_ERR_MESSAGE:
        {
            // <PathErr message> ::= <Common Header> [ <RSVP_INTEGRITY> ]
            // <RSVP_SESSION_OBJECT> <RSVP_ERROR_SPEC>
            // [ <sender descriptor> ] <sender descriptor> ::=
            // <RSVP_SENDER_TEMPLATE> <RSVP_SENDER_TSPEC> [ <RSVP_ADSPEC> ]

            if ((allObjects->session == NULL)
                || (allObjects->errorSpec == NULL))
            {
                return FALSE;
            }
            break;
        }

        case RSVP_RESV_ERR_MESSAGE:
        {
            // <ResvErr Message> ::= <Common Header> [ <RSVP_INTEGRITY> ]
            // <RSVP_SESSION_OBJECT> <RSVP_HOP> <RSVP_ERROR_SPEC>
            // [ <RSVP_SCOPE> ] <RSVP_STYLE> [ < flow descriptor list > ]

            if ((allObjects->session == NULL)
                || (allObjects->rsvpHop == NULL)
                || (allObjects->errorSpec == NULL)
                || (allObjects->style == NULL))
            {
                return FALSE;
            }

            break;
        }

        case RSVP_PATH_TEAR_MESSAGE:
        {
            // <PathTear Message> ::= <Common Header> [ <RSVP_INTEGRITY> ]
            // <RSVP_SESSION_OBJECT> <RSVP_HOP> [<sender descriptor>]
            // <sender descriptor> ::=  <RSVP_SENDER_TEMPLATE>
            // <RSVP_SENDER_TSPEC> [ <RSVP_ADSPEC> ]

            if ((allObjects->session == NULL) ||
                (allObjects->rsvpHop == NULL))
            {

                return FALSE;
            }
            break;
        }

        case RSVP_RESV_TEAR_MESSAGE:
        {
            // <ResvTear Message> ::= <Common Header> [<RSVP_INTEGRITY>]
            // <RSVP_SESSION_OBJECT> <RSVP_HOP> [ <RSVP_SCOPE> ]
            // <RSVP_STYLE> <flow descriptor list>

            if ((allObjects->session == NULL)
                || (allObjects->rsvpHop == NULL)
                || (allObjects->style == NULL))
            {
                return FALSE;
            }
            break;
        }

        case RSVP_RESV_CONF_MESSAGE:
        {
            // <ResvConf message> ::= <Common Header> [ <RSVP_INTEGRITY> ]
            // <RSVP_SESSION_OBJECT> <RSVP_ERROR_SPEC> <RESV_CONFIRM>
            // <RSVP_STYLE> <flow descriptor list>

            if ((allObjects->session == NULL)
                || (allObjects->errorSpec == NULL)
                || (allObjects->resvConf == NULL)
                || (allObjects->style == NULL))
            {
                return FALSE;
            }
            break;
        }

        case RSVP_HELLO_MESSAGE:
        {
            // <Hello Message> ::= <Common Header> [ <RSVP_INTEGRITY> ]
            // <RSVP_HELLO>
            if (allObjects->hello == NULL)
            {
                return FALSE;
            }
            break;
        }

        default:
        {
            // invalid message type
            ERROR_Assert(FALSE, "Invalid message type");
            return FALSE;
        }
    }
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpParseAndFindObjects
// PURPOSE      This function is called to parse the message and if the
//              parsing is successful then return the information of all
//              RSVP objects from the message for further processing
//
// Return:          TRUE if parsing is correct else FALSE
// Parameters:
//     node:        node which received the message
//     destAddress: destination address of the packet
//-------------------------------------------------------------------------
static
BOOL RsvpParseAndFindObjects(
    Node* node,
    Message* msg,
    RsvpAllObjects* allObjects)
{
    unsigned char* workingPtr = NULL;
    unsigned short objectLength = 0;
    unsigned short totalLength = sizeof(RsvpCommonHeader);

    unsigned short packetSize = (short) MESSAGE_ReturnPacketSize(msg);
    RsvpCommonHeader* commonHeader = NULL;

    memset(allObjects, 0, sizeof(RsvpAllObjects));

    // store initial packet pointer
    allObjects->packetInitialPtr
        = (unsigned char *) MESSAGE_ReturnPacket(msg);

    commonHeader = (RsvpCommonHeader*) allObjects->packetInitialPtr;
    ERROR_Assert(commonHeader, "Invalid message type");

    if ((commonHeader->msgType == RSVP_RESV_MESSAGE)
        || (commonHeader->msgType == RSVP_RESV_ERR_MESSAGE)
        || (commonHeader->msgType == RSVP_RESV_TEAR_MESSAGE))
    {
        ListInit(node, &(allObjects->FlowDescriptorList.ffDescriptor));
    }

    if (packetSize != commonHeader->rsvpLength)
    {
        // invalid message length, message discarded
        return FALSE;
    }

    // store common header
    allObjects->commonHeader
        = (RsvpCommonHeader *) MEM_malloc(sizeof(RsvpCommonHeader));

    memcpy(allObjects->commonHeader,
           commonHeader,
           sizeof(RsvpCommonHeader));

    if (RSVP_DEBUG)
    {
        char msgType = allObjects->commonHeader->msgType;
        printf("msg type = %d\n", msgType);
    }

    // working ptr is now set to the first object header in the message
    workingPtr = (unsigned char*) (commonHeader + 1);

    while (packetSize > totalLength)
    {
        ERROR_Assert(workingPtr, "Invalid message structure");

        switch (((RsvpObjectHeader*) workingPtr)->classNum)
        {
            case RSVP_NULL_OBJECT:
            {
                // Only handles working pointer, NULL objects are ignored
                objectLength = 4;
                break;
            }

            case RSVP_SESSION_OBJECT:
            {
                // handles RSVP_SESSION_OBJECT object, must be present at all message
                allObjects->session =
                    (RsvpTeObjectSession *)
                    MEM_malloc(sizeof(RsvpTeObjectSession));

                memcpy(allObjects->session,
                       workingPtr,
                       sizeof(RsvpTeObjectSession));

                objectLength = allObjects->session->objectHeader.
                    objectLength;

                break;
            }

            case RSVP_HOP:
            {
                allObjects->rsvpHop =
                    (RsvpObjectRsvpHop *)
                    MEM_malloc(sizeof(RsvpObjectRsvpHop));

                memcpy(allObjects->rsvpHop,
                       workingPtr,
                       sizeof(RsvpObjectRsvpHop));

                objectLength = allObjects->rsvpHop->objectHeader.
                    objectLength;

                break;
            }

            case RSVP_TIME_VALUES:
            {
                allObjects->timeValues =
                    (RsvpObjectTimeValues *)
                    MEM_malloc(sizeof(RsvpObjectTimeValues));

                memcpy(allObjects->timeValues,
                       workingPtr,
                       sizeof(RsvpObjectTimeValues));

                objectLength = allObjects->timeValues->objectHeader.
                    objectLength;

                break;
            }

            case RSVP_ERROR_SPEC:
            {
                allObjects->errorSpec =
                    (RsvpObjectErrorSpec *)
                    MEM_malloc(sizeof(RsvpObjectErrorSpec));

                memcpy(allObjects->errorSpec,
                       workingPtr,
                       sizeof(RsvpObjectErrorSpec));

                objectLength = allObjects->errorSpec->objectHeader.
                    objectLength;

                break;
            }

            case RSVP_SCOPE:
            {
                allObjects->scope =
                    (RsvpObjectScope *) MEM_malloc(sizeof(RsvpObjectScope));

                memcpy(allObjects->scope,
                       workingPtr,
                       sizeof(RsvpObjectScope));

                objectLength = allObjects->scope->objectHeader.objectLength;
                break;
            }

            case RSVP_STYLE:
            {
                allObjects->style =
                    (RsvpObjectStyle *) MEM_malloc(sizeof(RsvpObjectStyle));

                memcpy(allObjects->style,
                       workingPtr,
                       sizeof(RsvpObjectStyle));

                objectLength = allObjects->style->objectHeader.objectLength;
                break;
            }

            case RSVP_FLOWSPEC:
            {
                // process the flow descriptor list based on the current
                // style, return total length of the flow descriptor,
                // packetPtr is the current position of the packet where
                // RSVP_FLOWSPEC object is found. This function allocates
                // flowDescriptor variable in allObjects.
                objectLength = RsvpParseFlowDescriptorList(
                                   node,
                                   &workingPtr,
                                   allObjects);

                if (objectLength == 0)
                {
                    // return 0 indicates error in parsing descriptor list
                    return FALSE;
                }
                break;
            }

            case RSVP_SENDER_TEMPLATE:
            {
                allObjects->senderTemplate =
                    (RsvpTeObjectSenderTemplate *)
                    MEM_malloc(sizeof(RsvpTeObjectSenderTemplate));

                memcpy(allObjects->senderTemplate,
                       workingPtr,
                       sizeof(RsvpTeObjectSenderTemplate));

                objectLength = allObjects->senderTemplate->objectHeader.
                    objectLength;

                break;
            }

            case RSVP_SENDER_TSPEC:
            {
                allObjects->senderTSpec =
                    (RsvpObjectSenderTSpec *)
                    MEM_malloc(sizeof(RsvpObjectSenderTSpec));

                memcpy(allObjects->senderTSpec,
                       workingPtr,
                       sizeof(RsvpObjectSenderTSpec));

                objectLength = allObjects->senderTSpec->objectHeader.
                    objectLength;

                break;
            }

            case RSVP_ADSPEC:
            {
                allObjects->adSpec =
                    (RsvpObjectAdSpec *)
                    MEM_malloc(sizeof(RsvpObjectAdSpec));

                memcpy(allObjects->adSpec,
                       workingPtr,
                       sizeof(RsvpObjectAdSpec));

                objectLength = allObjects->adSpec->objectHeader.objectLength;
                break;
            }

            case RSVP_RESV_CONFIRM:
            {
                allObjects->resvConf =
                    (RsvpObjectResvConfirm *)
                    MEM_malloc(sizeof(RsvpObjectResvConfirm));

                memcpy(allObjects->resvConf,
                       workingPtr,
                       sizeof(RsvpObjectResvConfirm));

                objectLength = allObjects->resvConf->objectHeader.
                    objectLength;

                break;
            }

            case RSVP_LABEL_REQUEST:
            {
                allObjects->labelRequest =
                    (RsvpTeObjectLabelRequest *)
                    MEM_malloc(sizeof(RsvpTeObjectLabelRequest));

                memcpy(allObjects->labelRequest,
                       workingPtr,
                       sizeof(RsvpTeObjectLabelRequest));

                objectLength = allObjects->labelRequest->objectHeader.
                    objectLength;

                break;
            }

            case RSVP_EXPLICIT_ROUTE:
            {
                objectLength = ((RsvpObjectHeader*) workingPtr)->objectLength;

                // If already found RSVP_EXPLICIT_ROUTE ignore the second one
                if (!allObjects->explicitRoute)
                {
                    allObjects->explicitRoute
                        = (RsvpTeObjectExplicitRoute *)
                          MEM_malloc(objectLength);
                    memcpy(allObjects->explicitRoute, workingPtr, objectLength);
                }

                break;
            }

            case RSVP_RECORD_ROUTE:
            {
                objectLength = ((RsvpObjectHeader*) workingPtr)->objectLength;

                // If already found RSVP_RECORD_ROUTE ignore the second one
                if (!allObjects->recordRoute)
                {
                    allObjects->recordRoute = MEM_malloc(objectLength);
                    memcpy(allObjects->recordRoute, workingPtr, objectLength);
                }
                break;
            }

            case RSVP_POLICY_DATA:
            {
                // only considered for length validation
                objectLength = ((RsvpObjectHeader*)
                    workingPtr)->objectLength;

                break;
            }

            case RSVP_SESSION_ATTRIBUTE:
            {
                objectLength = ((RsvpObjectHeader*) workingPtr)->objectLength;

                allObjects->sessionAttr
                    = (RsvpTeObjectSessionAttribute*)
                      MEM_malloc(objectLength);
                memcpy(allObjects->sessionAttr,
                       workingPtr,
                       objectLength);
                break;
            }

            case RSVP_HELLO:
            {
                allObjects->hello
                    = (RsvpTeObjectHello *)
                      MEM_malloc(sizeof(RsvpTeObjectHello));
                memcpy(allObjects->hello,
                       workingPtr,
                       sizeof(RsvpTeObjectHello));

                objectLength = allObjects->hello->objectHeader.objectLength;
                break;
            }

            default:
            {
                // Invalid object type return FALSE, message is discarded
                ERROR_Assert(FALSE, "Invalid RSVP object in message");
                break;
            }
        }

        // consider the next RSVP object in the message
        totalLength = (short) (totalLength + objectLength);

        if (totalLength < commonHeader->rsvpLength)
        {
            // for RSVP_FLOWSPEC already the pointer is moved
            if (((RsvpObjectHeader*) workingPtr)->classNum != RSVP_FLOWSPEC)
            {
                workingPtr += objectLength;
            }
        }
    }

    if (totalLength != commonHeader->rsvpLength)
    {
        // invalid message length, message discarded.
        return FALSE;
    }

    // Now check whether all required RSVP objects present in the
    // corresponding message. If this condition satisfies then return TRUE
    // otherwise return FALSE.
    return RsvpCheckRequiredObject(
               (RsvpMsgType) commonHeader->msgType, allObjects);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpFreeFiltSS.
// PURPOSE      Freeing filtSS object
// Return:      None
// Parameters:
//     filtSS: pointer to filtSS to be deleted
//-------------------------------------------------------------------------
static
void RsvpFreeFiltSS(RsvpFiltSS* filtSS)
{
    if (filtSS)
    {
        if (filtSS->filterSpec)
        {
            MEM_free(filtSS->filterSpec);
        }
        if (filtSS->label)
        {
            MEM_free(filtSS->label);
        }
        if (filtSS->recordRoute)
        {
            MEM_free(filtSS->recordRoute);
        }

       // if filterspec exists free filter spec
       MEM_free(filtSS);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpFreeMiscObjects.
// PURPOSE      Freeing different RSVP object in the structure
// Return:      None
// Parameters:
//     allObjects: pointer to "RsvpAllObjects" Structure.
//-------------------------------------------------------------------------
static
void RsvpFreeMiscObjects(RsvpAllObjects* allObjects)
{
    if (allObjects->commonHeader != NULL)
    {
        MEM_free(allObjects->commonHeader);
    }
    if (allObjects->session != NULL)
    {
        MEM_free(allObjects->session);
    }
    if (allObjects->senderTemplate != NULL)
    {
        MEM_free(allObjects->senderTemplate);
    }
    if (allObjects->senderTSpec != NULL)
    {
        MEM_free(allObjects->senderTSpec);
    }
    if (allObjects->adSpec != NULL)
    {
        MEM_free(allObjects->adSpec);
    }
    if (allObjects->rsvpHop != NULL)
    {
        MEM_free(allObjects->rsvpHop);
    }
    if (allObjects->timeValues != NULL)
    {
        MEM_free(allObjects->timeValues);
    }
    if (allObjects->resvConf != NULL)
    {
        MEM_free(allObjects->resvConf);
    }
    if (allObjects->scope != NULL)
    {
        MEM_free(allObjects->scope);
    }
    if (allObjects->errorSpec != NULL)
    {
        MEM_free(allObjects->errorSpec);
    }
    if (allObjects->labelRequest != NULL)
    {
        MEM_free(allObjects->labelRequest);
    }
    if (allObjects->resvConf != NULL)
    {
        MEM_free(allObjects->resvConf);
    }
    if (allObjects->explicitRoute != NULL)
    {
        MEM_free(allObjects->explicitRoute);
    }
    if (allObjects->sessionAttr != NULL)
    {
        MEM_free(allObjects->sessionAttr);
    }
    if (allObjects->hello != NULL)
    {
        MEM_free(allObjects->hello);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpFreeAllObjects.
// PURPOSE      Freeing FlowSpec Lists in the "RsvpAllObjects" structure
// Return:      None
// Parameters:
//     node:    node which is freeing the "RsvpAllObjects" Structure
//     allObjects: pointer to "RsvpAllObjects" Structure.
//-------------------------------------------------------------------------
static
void RsvpFreeAllObjects(Node* node, RsvpAllObjects* allObjects)
{
    RsvpFreeMiscObjects(allObjects);

    if (!allObjects->style)
    {
       // if style not exists then return
       return;
    }

    switch (RsvpObjectStyleGetStyleBits(allObjects->style->rsvpStyleBits))
    {
        case RSVP_FF_STYLE:
        {
            // if FF Flow descriptor list exists then free it
            if (allObjects->FlowDescriptorList.ffDescriptor)
            {
                ListItem* listItm = allObjects->FlowDescriptorList.
                    ffDescriptor->first;

                while (listItm)
                {
                    RsvpFiltSS* filtSS = ((RsvpFFDescriptor*)
                        listItm->data)->filtSS;

                    RsvpObjectFlowSpec* flowSpec = ((RsvpFFDescriptor*)
                        listItm->data)->flowSpec;

                    if (flowSpec)
                    {
                        MEM_free(flowSpec);
                    }

                    // If filter spec object structure exists then ...
                    RsvpFreeFiltSS(filtSS);

                    listItm = listItm->next;
                }
                ListFree(node,
                         allObjects->FlowDescriptorList.ffDescriptor,
                         FALSE);
            }
            break;
        }

        case RSVP_SE_STYLE:
        {
            RsvpSEDescriptor* seDescriptor =
                allObjects->FlowDescriptorList.seDescriptor;

            // if SE Flow descriptor list exists then free it
            if (seDescriptor)
            {
                if (seDescriptor->flowSpec)
                {
                    MEM_free(seDescriptor->flowSpec);
                }
                if (seDescriptor->filtSS)
                {
                    ListItem* listItm = seDescriptor->filtSS->first;

                    while (listItm)
                    {
                        RsvpFiltSS* filtSS = (RsvpFiltSS*) listItm->data;

                        RsvpFreeFiltSS(filtSS);
                        listItm = listItm->next;
                    }

                }
                MEM_free(allObjects->FlowDescriptorList.seDescriptor);
            }
            break;
        }

        case RSVP_WF_STYLE:
        {
            MEM_free(allObjects->FlowDescriptorList.flowSpec);
            break;
        }

        default:
        {
            // invalid style
            ERROR_Assert(FALSE, "Invalid Style in Resv message");
            break;
        }
    }
    MEM_free(allObjects->style);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpHandlePacket.
// PURPOSE      This function handles all the RSVP messages received by a
//              node in the transport layer. This is normally called from
//              RsvpLayer function when event type is
//              MSG_TRANSPORT_FromNetwork.
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     msg:     message received by the layer
//-------------------------------------------------------------------------
static
void RsvpHandlePacket(Node* node, Message* msg, NodeAddress source)
{
    RsvpAllObjects allObjects;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    RsvpCommonHeader* commonHeader = (RsvpCommonHeader*)
        MESSAGE_ReturnPacket(msg);

    char clockStr[MAX_STRING_LENGTH] = {0};

    memset(&allObjects, 0, sizeof(RsvpAllObjects));
    ctoa(node->getNodeTime(), clockStr);

    if (RSVP_DEBUG)
    {
        char ipAddress[MAX_STRING_LENGTH] = {0};
        IO_ConvertIpAddressToString(source, ipAddress);

        printf("node %u Received message from %s\n",
               node->nodeId,
               ipAddress);
    }

    // check the validity of the message
    if (!RsvpIsValidMessage(node, msg))
    {
        // Invalid message. Free message and return
        // goto RsvpLayer() to free the message.
        return;
    }

    // Parse the message and check the validity of the all objects and their
    // length. This function returns all the RSVP objects in the output
    // parameter RsvpAllObjects. If the validity fails function
    // returns FALSE, then free the message and return
    if (!RsvpParseAndFindObjects(node, msg, &allObjects))
    {
        // Found any parsing error, then free message and return
        // goto RsvpLayer() to dropfree the message
        return;
    }

    switch (commonHeader->msgType)
    {
        case RSVP_PATH_MESSAGE:
        {
            if (RSVP_DEBUG)
            {
                printf("RSVP_PATH_MESSAGE received by %d with"
                       " packet size = %d\n",
                        node->nodeId,
                        commonHeader->rsvpLength);
            }

            rsvp->rsvpStat->numPathMsgReceived++;

            // RSVP RSVP_PATH_MESSAGE processing
            RsvpProcessPathMessage(node, msg, &allObjects);
            break;
        }

        case RSVP_RESV_MESSAGE:
        {
            if (RSVP_DEBUG)
            {
                printf("RSVP_RESV_MESSAGE received by %d with"
                       " packet size = %d sim-time %s\n",
                        node->nodeId,
                        commonHeader->rsvpLength,
                        clockStr);
            }

            rsvp->rsvpStat->numResvMsgReceived++;

            // RSVP RSVP_RESV_MESSAGE processing
            RsvpProcessResvMessage(node, msg, &allObjects);
            break;
        }

        case RSVP_PATH_ERR_MESSAGE:
        {
            if (RSVP_DEBUG)
            {
                printf("RSVP_PATH_ERR_MESSAGE received by %d with"
                       " packet size = %d sim-time %s\n",
                       node->nodeId,
                       commonHeader->rsvpLength,
                       clockStr);
            }
            rsvp->rsvpStat->numPathErrMsgReceived++;

            // RSVP RSVP_PATH_ERR_MESSAGE processing
            RsvpProcessPathErrMessage(node, msg, &allObjects);
            break;
        }

        case RSVP_RESV_ERR_MESSAGE:
        {
            if (RSVP_DEBUG)
            {
                printf("RSVP_RESV_ERR_MESSAGE received by %d with"
                       " packet size = %d sim-time %s\n",
                       node->nodeId,
                       commonHeader->rsvpLength,
                       clockStr);
            }

            rsvp->rsvpStat->numResvErrMsgReceived++;

            // RSVP RSVP_RESV_ERR_MESSAGE processing
            RsvpProcessResvErrMessage(node, msg, &allObjects);
            break;
        }

        case RSVP_PATH_TEAR_MESSAGE:
        {
            if (RSVP_DEBUG)
            {
                printf("RSVP_PATH_TEAR_MESSAGE received by %d with"
                       " packet size = %d sim-time = %s\n",
                       node->nodeId,
                       commonHeader->rsvpLength,
                       clockStr);
            }

            rsvp->rsvpStat->numPathTearMsgReceived++;

            // RSVP RSVP_PATH_TEAR_MESSAGE processing
            RsvpProcessPathTearMessage(node, msg, &allObjects);
            break;
        }

        case RSVP_RESV_TEAR_MESSAGE:
        {
            if (RSVP_DEBUG)
            {
                printf("RSVP_RESV_TEAR_MESSAGE received by %d with"
                       " packet size = %d sim-time %s\n",
                       node->nodeId,
                       commonHeader->rsvpLength,
                       clockStr);
            }
            rsvp->rsvpStat->numResvTearMsgReceived++;

            // RSVP RSVP_RESV_TEAR_MESSAGE processing
            RsvpProcessResvTearMessage(node, msg, &allObjects);
            break;
        }

        case RSVP_RESV_CONF_MESSAGE:
        {
            if (RSVP_DEBUG)
            {
                printf("RSVP_RESV_CONF_MESSAGE received by %d"
                       " with packet size = %d sim-time %s\n",
                       node->nodeId,
                       commonHeader->rsvpLength,
                       clockStr);
            }
            // RSVP RSVP_RESV_CONF_MESSAGE processing
            //TransportRsvpProcessResvConfMessage(node, msg, &allObjects);
            break;
        }

        case RSVP_HELLO_MESSAGE:
        {
            if (RSVP_DEBUG)
            {
                printf("RSVP_HELLO_MESSAGE received by %d"
                       " with packet size = %d sim-time %s\n",
                       node->nodeId,
                       commonHeader->rsvpLength,
                       clockStr);
            }
            // RSVP_HELLO_MESSAGE processing
            RsvpTeProcessHelloMessage(node, msg, &allObjects);
            break;
        }

        default:
        {
            // Invalid message
            ERROR_Assert(FALSE, "Invalid RSVP message.");
            break;
        }
    }
    RsvpFreeAllObjects(node, &allObjects);
    // Goto RsvpLayer() To drop and free the message
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTeEnvokeHelloExtension
// PURPOSE      The function processes the Hello refresh event
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     msg:     message received by the layer
//-------------------------------------------------------------------------
static void
RsvpTeEnvokeHelloExtension(Node* node, Message* msg)
{
    int index = -1;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    unsigned int neighSrcIns;
    unsigned int thisSrcIns;

    // Create and send RSVP_HELLO_MESSAGE for REQUEST object for all adjacent
    // neighbors. Maintain TransportRsvpHelloExtension structure properly.
    for (index = 0; index < node->numberInterfaces; index++)
    {
        ListItem* item = NULL;
        RsvpTeHelloExtension* hello = NULL;
        BOOL isFound = FALSE;

        ERROR_Assert(rsvp->helloList, "Invalid hello list");

        // search in hello list for an entry for this interface
        item = rsvp->helloList->first;
        isFound = FALSE;

        while (item && (!isFound))
        {
            hello = (RsvpTeHelloExtension*) item->data;
            if (hello->interfaceIndex == index)
            {
                // entry found take values from it for the RSVP_HELLO request
                isFound = TRUE;
            }
            item = item->next;
        }

        if (isFound)
        {
            // already in the hello list
            if (!hello->srcInstance)
            {
                // srcInstance set to next available if not set already
                hello->srcInstance = rsvp->nextAvailableInstance++;
            }

            // collect srcInstance and dstInstance for RSVP_HELLO REQUEST
            thisSrcIns = hello->srcInstance;
            neighSrcIns = hello->dstInstance;
        }
        else
        {
            // not in the hello list
            thisSrcIns = rsvp->nextAvailableInstance++;
            neighSrcIns = 0;

            // Create Hello list structure. Here neighbor address still not
            // received, set as 0. It is found when it receives
            // RSVP_HELLO_REQUEST from the neighbor
            RsvpCreateHelloList(
                node,
                0,
                index,
                thisSrcIns,
                neighSrcIns);
        }

        rsvp->rsvpStat->numHelloRequestMsgSent++;

        // create and send the RSVP_HELLO REQUEST for this interface
        RsvpTeCreateAndSendHelloMessage(
            node,
            index,
            thisSrcIns,
            neighSrcIns,
            RSVP_HELLO_REQUEST);
    }
    // goto RsvpLayer() to drop
    // and free the message.
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateExplicitRoute
// PURPOSE      The function creates RSVP_EXPLICIT_ROUTE object
//
// Return:                  None
// Parameters:
//     node:                node which received the message
//     abstractNode:        abstract node list
//     abstractNodeCount:   number of abs. node to be added in
//                          RSVP_EXPLICIT_ROUTE
//-------------------------------------------------------------------------
static
RsvpTeObjectExplicitRoute* RsvpCreateExplicitRoute(
    Node* node,
    NodeAddress* abstractNode,
    int abstractNodeCount)
{
    int i = 0;
    RsvpTeSubObject* subObject = NULL;
    RsvpTeObjectExplicitRoute* explicitRoute;

    // total size is the size of all ub objects and the object header
    unsigned short explicitRouteSize = (short) (sizeof(RsvpObjectHeader)
        + (sizeof(RsvpTeSubObject) * abstractNodeCount));

    // if no adjacency then RSVP_EXPLICIT_ROUTE not possible
    if (!RsvpSearchAdjacency(node, *abstractNode))
    {
        return NULL;
    }

    explicitRoute = (RsvpTeObjectExplicitRoute*)
        MEM_malloc(explicitRouteSize);

    // create object header
    RsvpCreateObjectHeader(
        &explicitRoute->objectHeader,
        explicitRouteSize,
        RSVP_EXPLICIT_ROUTE,
        1);

    subObject = &(explicitRoute->subObject);

    // create sub object for all abstract nodes
    for (i = 0; i < abstractNodeCount; i++)
    {
        RsvpCreateSubObject(
            subObject,
            *(abstractNode + i),
            RSVP_EXPLICIT_ROUTE);

        // next sub object pointer
        subObject++;
    }
    return explicitRoute;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTeProcessExplicitRoute
// PURPOSE      The function processes the RSVP_EXPLICIT_ROUTE path
//
// Return:              None
// Parameters:
//      node:           node which received the message
//      msg:            message received by the layer
//      psb:            current PSb
//-------------------------------------------------------------------------
static
void RsvpTeProcessExplicitRoute(
    Node* node,
    RsvpPathStateBlock* psb)
{
    RsvpPathStateBlock* cPSB = NULL;
    RsvpAllObjects allObjects;
    RsvpTeObjectExplicitRoute* explicitRoute = NULL;
    RsvpTeObjectSenderTemplate senderTemplate;
    RsvpObjectRsvpHop rsvpHop;
    RsvpObjectTimeValues timeValues;
    RsvpPathMessage* pathMessage = NULL;

    unsigned short returnPacketSize = 0;
    int interfaceId;
    NodeAddress ipAddr;

    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;

    if (RSVP_DEBUG)
    {
        printf("Explicit route self message received by %d\n", node->nodeId);
    }

    // If hello adjacency not formed, explicit path processing not possible
    // resend the timer for the next try
    if (!RsvpSearchAdjacency(node, *(rsvp->abstractNodeList)))
    {
        // Set the timer MSG_TRANSPORT_RSVP_InitiateExplicitRoute
        Message* explicitRouteMsg =
            MESSAGE_Alloc(node,
                          TRANSPORT_LAYER,
                          TransportProtocol_RSVP,
                          MSG_TRANSPORT_RSVP_InitiateExplicitRoute);

        // send current PSB pointer in the info field
        MESSAGE_InfoAlloc(node, explicitRouteMsg, sizeof(char*));
        memcpy(MESSAGE_ReturnInfo(explicitRouteMsg), &psb, sizeof(char*));

        MESSAGE_Send(node,
                     explicitRouteMsg,
                     RSVP_EXPLICIT_ROUTE_TIMER_DELAY);
        return;
    }

    // going to create RSVP_EXPLICIT_ROUTE path
    if (rsvp->abstractNodeCount)
    {
        explicitRoute = RsvpCreateExplicitRoute(
                             node,
                             rsvp->abstractNodeList,
                             rsvp->abstractNodeCount);
    }

    senderTemplate = RsvpCreateSenderTemplate(
                         psb->senderTemplate.senderAddress,
                         psb->senderTemplate.lspId + 1);

    interfaceId = NetworkIpGetInterfaceIndexForNextHop(
                      node,
                      *(rsvp->abstractNodeList));

    ipAddr = NetworkIpGetInterfaceAddress(node, interfaceId);

    // create RSVP_HOP
    rsvpHop = RsvpCreateRsvpHop(ipAddr, interfaceId);

    // create RSVP_TIME_VALUES
    RsvpCreateTimeValues(node, RSVP_STATE_REFRESH_DELAY, &timeValues);

    allObjects.session = &(psb->session);
    allObjects.senderTemplate = &senderTemplate;
    allObjects.senderTSpec = &(psb->senderTSpec);
    allObjects.rsvpHop = NULL;          // sender has no PHOP in PSB
    allObjects.timeValues = &timeValues;
    allObjects.explicitRoute = explicitRoute;

    cPSB = RsvpCreatePSB(node, &allObjects);

    IntListInit(node, &cPSB->outInterfaceList);

    IntListInsert(node,
               cPSB->outInterfaceList,
               node->getNodeTime(),
               interfaceId);

    if (psb->adSpec.objectHeader.objectLength != 0)
    {
        // Copy RSVP_ADSPEC to current PSB
        memcpy(&cPSB->adSpec,
               &psb->adSpec,
               psb->adSpec.objectHeader.objectLength);
    }

    // Copy RSVP_LABEL_REQUEST to current PSB
    memcpy(&cPSB->labelRequest,
           &psb->labelRequest,
           sizeof(RsvpTeObjectLabelRequest));

    if (explicitRoute != NULL)
    {
        // Copy receiving RSVP_EXPLICIT_ROUTE to current PSB
        cPSB->receivingExplicitRoute = (RsvpTeObjectExplicitRoute*)
                MEM_malloc(explicitRoute->objectHeader.objectLength);

        memcpy(cPSB->receivingExplicitRoute,
               explicitRoute,
               explicitRoute->objectHeader.objectLength);
    }

    if (psb->sessionAttr->objectHeader.objectLength != 0)
    {
        // store RSVP_SESSION_ATTRIBUTE object in PSB
        cPSB->sessionAttr = (RsvpTeObjectSessionAttribute*)
                MEM_malloc(psb->sessionAttr->objectHeader.objectLength);

        memcpy(cPSB->sessionAttr,
               psb->sessionAttr,
               psb->sessionAttr->objectHeader.objectLength);
    }

    // prepare the path message with explicit route path
    pathMessage = RsvpCreatePathMessage(
                      IPDEFTTL,
                      &cPSB->session,
                      &rsvpHop,
                      &timeValues,
                      explicitRoute,
                      &cPSB->labelRequest,
                      cPSB->sessionAttr,
                      &senderTemplate,
                      &cPSB->senderTSpec,
                      &cPSB->adSpec,
                      &returnPacketSize);

    // create RSVP_RECORD_ROUTE
    if (rsvp->recordRouteType != RSVP_RECORD_ROUTE_NONE)
    {
        RsvpCreateRecordRoute(
            NULL,
            (RsvpSessionAttributeFlags) 0,
            ipAddr,
            0,
            0,
            (void**) &pathMessage,
            &returnPacketSize);
    }

    RsvpSendPathMessage(
        node,
        pathMessage,
        ipAddr,
        cPSB->session.receiverAddress,
        explicitRoute,
        IPDEFTTL);

    if (explicitRoute)
    {
        MEM_free(explicitRoute);
    }
    MEM_free(pathMessage);
}


//-------------------------------------------------------------------------
// FUNCTION     TransportRsvpCreateSession
// PURPOSE      The function creates a new RSVP_SESSION_OBJECT object
//
// Return:      New RSVP_SESSION_OBJECT object
// Parameters:
//     receiverAddr:    IP address of receiver
//     tunnellId:       tunnell ID
//     upcallPtr:       upcall function pointer
//-------------------------------------------------------------------------
static RsvpTeObjectSession
TransportRsvpCreateSession(
    NodeAddress receiverAddr,
    int tunnellId,
    unsigned upcallPtr)
{
    RsvpTeObjectSession session;

    RsvpCreateObjectHeader(
        &session.objectHeader,
        sizeof(RsvpTeObjectSession),
        (unsigned char) RSVP_SESSION_OBJECT,
        7);

    session.receiverAddress = receiverAddr; // egress node IPv4 address
    session.mustBeZero = 0;                 // reserved 2 bytes, must be 0
    session.tunnelId = (unsigned short) tunnellId;   // remain constant in a tunnel
    session.extendedTunnelId = upcallPtr;   // upcall func. pointer passed
    return session;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpTeObjectLabelRequest
// PURPOSE      The function creates a new RSVP_LABEL_REQUEST object.
//
// Return:      New RSVP_LABEL_REQUEST object
// Parameters:
//     l3Pid:   layer 3 identifier
//-------------------------------------------------------------------------
static RsvpTeObjectLabelRequest
TransportRsvpCreateLabelRequest(unsigned short l3Pid)
{
    RsvpTeObjectLabelRequest labelRequest;

    RsvpCreateObjectHeader(
        &labelRequest.objectHeader,
        sizeof(RsvpTeObjectLabelRequest),
        (unsigned char) RSVP_LABEL_REQUEST,
        1);

    labelRequest.reserved = 0;       // must be set 0
    labelRequest.L3Pid = l3Pid;      // ident. of layer 3 protocol
    return labelRequest;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateSenderTemplate
// PURPOSE      The function creates a new RSVP_SENDER_TEMPLATE object.
//
// Return:          New RSVP_SENDER_TEMPLATE object
// Parameters:
//     node:        node which received the message
//     sourceAddr:  source address
//     lspId:       LSP id
//-------------------------------------------------------------------------
RsvpTeObjectSenderTemplate
RsvpCreateSenderTemplate(
    NodeAddress sourceAddr,
    int lspId)
{
    RsvpTeObjectSenderTemplate senderTemplate;

    RsvpCreateObjectHeader(
        &senderTemplate.objectHeader,
        sizeof(RsvpTeObjectSenderTemplate),
        (unsigned char) RSVP_SENDER_TEMPLATE,
        7);

    senderTemplate.senderAddress = sourceAddr;
    senderTemplate.mustBeZero = 0;
    senderTemplate.lspId = (unsigned short) lspId;

    return senderTemplate;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateSenderTSpec
// PURPOSE      The function creates a new RSVP_SENDER_TSPEC object.
//
// Return:      New RSVP_SENDER_TSPEC object
// Parameters:
//     node:            node which received the message
//-------------------------------------------------------------------------
RsvpObjectSenderTSpec
RsvpCreateSenderTSpec(Node* node)
{
    RsvpObjectSenderTSpec senderTSpec;

    memset(&senderTSpec, 0, sizeof(RsvpObjectSenderTSpec));

    RsvpCreateObjectHeader(
        &senderTSpec.objectHeader,
        sizeof(RsvpObjectSenderTSpec),
        (unsigned char) RSVP_SENDER_TSPEC,
        7);

    // object will be created at the time of resource reservation
    if (RSVP_DEBUG)
    {
        printf("Node %d is creating a new RSVP_SENDER_TSPEC object\n",
               node->nodeId);
    }
    return senderTSpec;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateAdSpec
// PURPOSE      The function creates a new RSVP_ADSPEC object.
//
// Return:      New RSVP_ADSPEC object
// Parameters:
//     node:            node which received the message
//-------------------------------------------------------------------------
RsvpObjectAdSpec
RsvpCreateAdSpec(Node* node)
{
    RsvpObjectAdSpec adSpec;

    RsvpCreateObjectHeader(
        &adSpec.objectHeader,
        sizeof(RsvpObjectAdSpec),
        (unsigned char) RSVP_ADSPEC,
        2);

    if (RSVP_DEBUG)
    {
        printf("Node %d is creating a new RSVP_ADSPEC object\n",
               node->nodeId);
    }
    // object will be created at the time of resource reservation
    return adSpec;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpCreateSessionAttr
// PURPOSE      The function creates a RSVP_SESSION_ATTRIBUTE at the end of
//              message
//
// Return:          RSVP_SESSION_ATTRIBUTE object pointer
// Parameters:
//     setupPrio:   setup priority
//     holdingPrio: holding priority
//     flags:       flags label recording desired etc.
//     sessionName: name of the session
//-------------------------------------------------------------------------
static
RsvpTeObjectSessionAttribute* RsvpCreateSessionAttr(
    RsvpSessionAttributeFlags flags,
    char* sessionName)
{
    RsvpTeObjectSessionAttribute* sessionAttr;
    unsigned char actualLength;
    unsigned char sessionLength = (unsigned char) strlen(sessionName);

    if (sessionLength % 2)
    {
        sessionLength++;
    }

    actualLength = (unsigned char ) (sizeof(RsvpTeObjectSessionAttribute) + sessionLength);

    sessionAttr = (RsvpTeObjectSessionAttribute*)
             MEM_malloc(actualLength);

    memset (sessionAttr, 0, actualLength);

    RsvpCreateObjectHeader(
        &sessionAttr->objectHeader,
        (unsigned short) actualLength,
        RSVP_SESSION_ATTRIBUTE,
        1);

    // sessionAttr->setupPriority ;   // priority for taking resource
    // sessionAttr->holdingPriority;  // priority for holding resource

    // local protection,label recording
    sessionAttr->flags = (unsigned char) flags;

    // length of display string
    sessionAttr->nameLength = sessionLength;

    // display string is variable length and allocated dynamically
    memcpy(((char*) sessionAttr) + sizeof(RsvpTeObjectSessionAttribute),
           sessionName,
           strlen(sessionName));

    return sessionAttr;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpRegisterSession
// PURPOSE      Any application receiving any data packet to be sent to a
//              destination by using LSP must call this function to create
//              a session
//
// Return:          session ID
// Parameters:
//      node:               node which received the message
//      destinationAddr:    destination address
//      upcallFunctionPtr:  function pointer for upcall
//-------------------------------------------------------------------------
int RsvpRegisterSession(
    Node* node,
    NodeAddress destinationAddr,
    RsvpUpcallFunctionType upcallFunctionPtr)
{
    // Create a new session, return a session ID to the application that
    // calls this function to register the session. The upcall function
    // pointer also received here and has been used for different upcalls.
    // All the information is stored in the sessionList structure of the
    // node. A session id is unique to one dest. addr. This list also store
    // the upcall function pointer coming from different sender.

    char sessionName[MAX_STRING_LENGTH] = "Session";
    RsvpSessionList* newEntry = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = rsvp->sessionList->first;

    while (item)
    {
        RsvpSessionList* sessionList = (RsvpSessionList*) item->data;

        // destination address same means in same session
        if (sessionList->destinationAddr == destinationAddr)
        {
            return sessionList->sessionId;
        }
        item = item->next;
    }

    // this session not found create an entry in the list and return the new
    // session id to the caller
    newEntry = (RsvpSessionList*) MEM_malloc(sizeof(RsvpSessionList));
    newEntry->destinationAddr = destinationAddr;
    newEntry->upcallFunctionPtr = upcallFunctionPtr;
    sprintf(sessionName, "%s%d", sessionName, rsvp->nextSessionId);
    newEntry->sessionId = rsvp->nextSessionId++;
    strcpy(newEntry->sessionName, sessionName);
    newEntry->isSessionReleased = FALSE;
    ListInsert(node, rsvp->sessionList, node->getNodeTime(), newEntry);
    return newEntry->sessionId;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpGetSessionInfo
// PURPOSE      Get session info from the list
//
// Return:                  None
// Parameters:
//      node:               node which received the message
//      sessionId:          ID for the current session
//      receiverAddr:       returned receiver address
//      upcallPtr:          returned upcall ptr
//      sessionName:        returned session name
//-------------------------------------------------------------------------
static
void RsvpGetSessionInfo(
    Node* node,
    int sessionId,
    NodeAddress* receiverAddr,
    RsvpUpcallFunctionType* upcallPtr,
    char* sessionName)
{
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    ListItem* item = rsvp->sessionList->first;

    while (item)
    {
        RsvpSessionList* sessionList = (RsvpSessionList*) item->data;

        if (sessionList->sessionId == sessionId)
        {
            *receiverAddr = sessionList->destinationAddr;
            *upcallPtr = sessionList->upcallFunctionPtr;
            strcpy(sessionName, sessionList->sessionName);
            return;
        }
        item = item->next;
    }
    return;
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpDefineSender
// PURPOSE      In the created session application requests for the creation
//              of LSP by using different RSVP messages
//
// Return:                  None
// Parameters:
//      node:               node which received the message
//      sessionId:          resgistered session ID
//      senderTemplate:     RSVP_SENDER_TEMPLATE object
//      senderTSpec:        NSEDER_TSPEC object
//      adSpec:             RSVP_ADSPEC object
//      ttl:                TTL
//-------------------------------------------------------------------------
void RsvpDefineSender(
    Node* node,
    int sessionId,
    RsvpTeObjectSenderTemplate* senderTemplate,
    RsvpObjectSenderTSpec* senderTSpec,
    RsvpObjectAdSpec* adSpec,
    unsigned char ttl)
{
    RsvpUpcallFunctionType upcallPtr = NULL;
    char sessionName[MAX_STRING_LENGTH];
    RsvpPathMessage* pathMessage = NULL;
    RsvpPathStateBlock* cPSB = NULL;
    Message* refreshHelloMsg = NULL;

    RsvpAllObjects allObjects;
    RsvpTeObjectSession session;
    RsvpObjectRsvpHop rsvpHop;
    RsvpObjectTimeValues timeValues;
    RsvpTeObjectLabelRequest labelRequest;
    RsvpTeObjectSessionAttribute* sessionAttr = NULL;
    RsvpTeObjectExplicitRoute* explicitRoute = NULL;

    NodeAddress ipAddr;
    NodeAddress receiverAddr = 0;
    unsigned short returnPacketSize = 0;
    int interfaceId;
    RsvpSessionAttributeFlags flags = (RsvpSessionAttributeFlags) 0;
    unsigned int delayTime = 0;
    Message* refreshPathMsg = NULL;
    RsvpData* rsvp = (RsvpData*) node->transportData.rsvpVariable;
    RsvpTimerStore* timerStore;

    // collect receiver addr. and upcall function ptr for current session
    RsvpGetSessionInfo(
        node,
        sessionId,
        &receiverAddr,
        &upcallPtr,
        sessionName);

    // create RSVP_SESSION_OBJECT object
    session = TransportRsvpCreateSession(
                  receiverAddr,
                  sessionId,
                  0);

    interfaceId = NetworkGetInterfaceIndexForDestAddress(
                      node,
                      receiverAddr);

    ipAddr = NetworkIpGetInterfaceAddress(node, interfaceId);

    // create RSVP_HOP
    rsvpHop = RsvpCreateRsvpHop(ipAddr, interfaceId);

    // create RSVP_TIME_VALUES
    RsvpCreateTimeValues(node, RSVP_STATE_REFRESH_DELAY, &timeValues);

    labelRequest = TransportRsvpCreateLabelRequest(L3PID);

    // create RSVP_SESSION_ATTRIBUTE object
    if (rsvp->recordRouteType == RSVP_RECORD_ROUTE_LABELED)
    {
        flags = RSVP_LABEL_RECORDING_DESIRED;
    }

    sessionAttr = RsvpCreateSessionAttr(flags, sessionName);

    // create RSVP_EXPLICIT_ROUTE object with path defined
    // in the configuration file
    if (rsvp->abstractNodeCount)
    {
        explicitRoute = RsvpCreateExplicitRoute(
                            node,
                            rsvp->abstractNodeList,
                            rsvp->abstractNodeCount);
    }

    pathMessage = RsvpCreatePathMessage(
                      ttl,
                      &session,
                      &rsvpHop,
                      &timeValues,
                      explicitRoute,
                      &labelRequest,
                      sessionAttr,
                      senderTemplate,
                      senderTSpec,
                      adSpec,
                      &returnPacketSize);

    if (rsvp->recordRouteType != RSVP_RECORD_ROUTE_NONE)
    {
        // rsvp->recordRouteType != RSVP_RECORD_ROUTE_NONE
        // so create RSVP_RECORD_ROUTE object
        RsvpCreateRecordRoute(
            NULL,
            (RsvpSessionAttributeFlags) 0,
            ipAddr,
            0,
            0,
            (void**) &pathMessage,
            &returnPacketSize);
    }

    if (!RsvpSearchPSBSessionSenderTemplatePair(
             node,
             &session,
             senderTemplate))
    {
        allObjects.session = &session;
        allObjects.senderTemplate = senderTemplate;
        allObjects.senderTSpec = senderTSpec;
        allObjects.rsvpHop = NULL;          // sender has no PHOP in PSB
        allObjects.timeValues = &timeValues;
        allObjects.explicitRoute = explicitRoute;

        cPSB = RsvpCreatePSB(node, &allObjects);
        IntListInit(node, &cPSB->outInterfaceList);

        IntListInsert(
            node,
            cPSB->outInterfaceList,
            node->getNodeTime(),
            interfaceId);

        if (adSpec != NULL)
        {
            // Copy RSVP_ADSPEC to current PSB
            memcpy(&cPSB->adSpec, adSpec, adSpec->objectHeader.objectLength);
        }

        // Copy RSVP_LABEL_REQUEST to current PSB
        memcpy(&cPSB->labelRequest,
               &labelRequest,
               sizeof(RsvpTeObjectLabelRequest));

        if (explicitRoute != NULL)
        {
            // Copy receiving RSVP_EXPLICIT_ROUTE to current PSB
            cPSB->receivingExplicitRoute =
                (RsvpTeObjectExplicitRoute*) MEM_malloc(
                    explicitRoute->objectHeader.objectLength);

            memcpy(cPSB->receivingExplicitRoute,
                   explicitRoute,
                   explicitRoute->objectHeader.objectLength);
        }

        if ((sessionAttr != NULL) && sessionAttr->objectHeader.objectLength)
        {
            // store RSVP_SESSION_ATTRIBUTE object in PSB
            cPSB->sessionAttr =
                (RsvpTeObjectSessionAttribute*) MEM_malloc(
                    sessionAttr->objectHeader.objectLength);

            memcpy(cPSB->sessionAttr,
                   sessionAttr,
                   sessionAttr->objectHeader.objectLength);
        }
    }

    RsvpSendPathMessage(
        node,
        pathMessage,
        ipAddr,
        receiverAddr,
        explicitRoute,
        ttl);

    // RSVP_EXPLICIT_ROUTE is present but not added with the Path message due
    // to the unavailability of the adjacent node. In that case send the
    // following timer message so that RSVP_EXPLICIT_ROUTE will be added
    // after a delay
    if (rsvp->abstractNodeCount && cPSB && !explicitRoute)
    {
        // Set the timer MSG_TRANSPORT_RSVP_InitiateExplicitRoute
        Message* explicitRouteMsg =
            MESSAGE_Alloc(node,
                          TRANSPORT_LAYER,
                          TransportProtocol_RSVP,
                          MSG_TRANSPORT_RSVP_InitiateExplicitRoute);

        // send current PSB pointer in the info field
        MESSAGE_InfoAlloc(node, explicitRouteMsg, sizeof(char*));
        memcpy(MESSAGE_ReturnInfo(explicitRouteMsg), &cPSB, sizeof(char*));

        MESSAGE_Send(node,
                     explicitRouteMsg,
                     RSVP_EXPLICIT_ROUTE_TIMER_DELAY);
    }

    if (explicitRoute)
    {
        MEM_free(explicitRoute);
    }

    MEM_free(sessionAttr);
    MEM_free(pathMessage);

    // Set the timer MSG_TRANSPORT_RSVP_HelloExtension
    // for hello adjacency
    refreshHelloMsg = MESSAGE_Alloc(node,
                                    TRANSPORT_LAYER,
                                    TransportProtocol_RSVP,
                                    MSG_TRANSPORT_RSVP_HelloExtension);

    MESSAGE_Send(node, refreshHelloMsg, RSVP_HELLO_TIMER_DELAY);

    if (cPSB)
    {
        // set the timer MSG_TRANSPORT_RSVP_PathRefresh
        // for periodic refracement of RSVP path message.
        delayTime = (unsigned) ((RSVP_REFRESH_TIME_CONSTANT_K + 0.5)
                    * 1.5 * timeValues.refreshPeriod);

        refreshPathMsg = MESSAGE_Alloc(node,
                                       TRANSPORT_LAYER,
                                       TransportProtocol_RSVP,
                                       MSG_TRANSPORT_RSVP_PathRefresh);

        timerStore = (RsvpTimerStore*) MEM_malloc(sizeof(RsvpTimerStore));
        timerStore->messagePtr = refreshPathMsg;
        timerStore->psb = cPSB;

        ListInsert(node, rsvp->timerStoreList, node->getNodeTime(), timerStore);

        // send current PSB pointer in the info field
        MESSAGE_InfoAlloc(node, refreshPathMsg, sizeof(RsvpTimerStore));
        memcpy(MESSAGE_ReturnInfo(refreshPathMsg),
               timerStore,
               sizeof(RsvpTimerStore));

        MESSAGE_Send(node, refreshPathMsg, delayTime * MILLI_SECOND);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpInit
// PURPOSE      Initialization function for RSVP.
//
// Parameters:
//     node:      node being initialized.
//     nodeInput: structure containing contents of input file
//-------------------------------------------------------------------------
void RsvpInit(Node* node, const NodeInput* nodeInput)
{

    RsvpData* rsvp = (RsvpData*) MEM_malloc(sizeof(RsvpData));

    ERROR_Assert(node->transportData.rsvpVariable == NULL,
                 "RSVP-TE already is initialized");

    node->transportData.rsvpVariable = (void*) rsvp;

    RANDOM_SetSeed(rsvp->seed,
                   node->globalSeed,
                   node->nodeId,
                   TransportProtocol_RSVP);

    // Initialize different data structure
    ListInit(node, &rsvp->sessionList);
    ListInit(node, &rsvp->psb);
    ListInit(node, &rsvp->rsb);
    ListInit(node, &rsvp->bsb);

    rsvp->nextSessionId = 1;
    rsvp->nextAvailableLspId = 1;
    rsvp->nextAvailableLabel = 16;
    rsvp->nextAvailableInstance = 1;
    rsvp->abstractNodeCount = 0;

    ListInit(node, &rsvp->helloList);
    ListInit(node, &rsvp->lsp);
    ListInit(node, &rsvp->timerStoreList);

    // Initialisation routine for different element
    RsvpExplicitRouteInit(node, nodeInput);
    RsvpRecordRouteInit(node, nodeInput);
    RsvpResvStyleInit(node, nodeInput);
    RsvpStatInit(node, nodeInput);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpLayer.
// PURPOSE      This is the layer function of RSVP, that handles all the RSVP
//              receiving messages. This function has been called from
//              TRANSPORT_ProcessEvent of transport.pc when protocol ID is
//              TransportProtocol_RSVP.
//
// Return:      None
// Parameters:
//     node:    node which received the message
//     msg:     message received by the layer
//-------------------------------------------------------------------------
void
RsvpLayer(Node* node, Message* msg)
{
    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_TRANSPORT_RSVP_InitApp:
        {
            // The message is sent from the RSVP aware application
            // to create the LSP
            RsvpInitApplication(node, msg);
            break;
        }

        case MSG_TRANSPORT_FromNetwork:
        {
            NetworkToTransportInfo* netToTransInfo =
                (NetworkToTransportInfo*) MESSAGE_ReturnInfo(msg);

            // When network layer receives a RSVP packet from MAC layer, it
            // is sent to the TRANSPORT layer with this message for
            // processing. In the following function the packets are
            // processed. All the RSVP messages are handled here
            RsvpHandlePacket(node, msg,
                             GetIPv4Address(netToTransInfo->sourceAddr));
            break;
        }

        case MSG_TRANSPORT_RSVP_PathRefresh:
        {
            // This is triggered when timer is expired for a PSB. Find
            // refreshingPSB from the info field for which timer is expired
            RsvpPathStateBlock* refreshingPSB = (RsvpPathStateBlock*)
                RsvpSearchTimerStoreList(
                    node, (RsvpTimerStore*) MESSAGE_ReturnInfo(msg));

            if (refreshingPSB)
            {
                RsvpEnvokePathRefresh(node, refreshingPSB);
            }
            break;
        }

        case MSG_TRANSPORT_RSVP_ResvRefresh:
        {
            // This is triggered when timer is expired for a RSB. Find
            // phopAddr from the info field for which the timer is expired
            NodeAddress phopAddr = 0;
            memcpy(&phopAddr, MESSAGE_ReturnInfo(msg), sizeof(NodeAddress));
            RsvpEnvokeResvRefresh(node, phopAddr);
            break;
        }

        case MSG_TRANSPORT_RSVP_HelloExtension:
        {
            // This self message is triggered at a fixed interval from a RSVP
            // node. From here the RSVP_HELLO request message is generated
            // and refresh the timer for the next triggering
            RsvpTeEnvokeHelloExtension(node, msg);
            break;
        }

        case MSG_TRANSPORT_RSVP_InitiateExplicitRoute:
        {
            // This message is envoked when a node needs explicit route.
            // Initially the node sends this message and do the normal
            // procedure for LSP creation. From here explicit route
            // path creation will be activated

            RsvpPathStateBlock* psb;
            memcpy(&psb, MESSAGE_ReturnInfo(msg), sizeof(char*));

            RsvpTeProcessExplicitRoute(node, psb);
            break;
        }

        default:
        {
            // invalid event type
            ERROR_Assert(FALSE, "Invalid RSVP message.");
            break;
        }
    }
    // Free all the message()
    MESSAGE_Free(node, msg);
}


//-------------------------------------------------------------------------
// FUNCTION     RsvpFinalize
// PURPOSE      Called at the end of simulation to collect the results of
//              the simulation of the RSVP protocol of Transport Layer.
//
// Return:      None
// Parameter:
//     node:    node for which results are to be collected.
//-------------------------------------------------------------------------
void
RsvpFinalize(Node* node)
{
    RsvpStatistics* rsvpStat = NULL;
    RsvpData* rsvp = NULL;
    char buf[2 * MAX_STRING_LENGTH] = {0};
    rsvp = (RsvpData*) node->transportData.rsvpVariable;
    rsvpStat = rsvp->rsvpStat;

    if (rsvp->rsvpStatsEnabled == TRUE)
    {
        sprintf(buf, "Path Messages Received = %u",
                rsvpStat->numPathMsgReceived);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id
            buf);

        sprintf(buf, "Path Messages Sent = %u",
                rsvpStat->numPathMsgSent);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "Resv Messages Received = %u",
                rsvpStat->numResvMsgReceived);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "Resv Messages Sent = %u",
                rsvpStat->numResvMsgSent);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "PathErr Messages Received = %u",
                rsvpStat->numPathErrMsgReceived);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "PathErr Messages Sent = %u",
                rsvpStat->numPathErrMsgSent);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "ResvErr Messages Received = %u",
                rsvpStat->numResvErrMsgReceived);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "ResvErr Messages Sent = %u",
                rsvpStat->numResvErrMsgSent);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "PathTear Messages Received = %u",
                rsvpStat->numPathTearMsgReceived);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "PathTear Messages Sent = %u",
                rsvpStat->numPathTearMsgSent);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "ResvTear Messages Received = %u",
                rsvpStat->numResvTearMsgReceived);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "ResvTear Messages Sent = %u",
                rsvpStat->numResvTearMsgSent);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "HelloRequest Messages Received = %u",
                rsvpStat->numHelloRequestMsgReceived);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "HelloRequest Messages Sent = %u",
                rsvpStat->numHelloRequestMsgSent);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "HelloAck Messages Received = %u",
                rsvpStat->numHelloAckMsgReceived);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "HelloAck Messages Sent = %u",
                rsvpStat->numHelloAckMsgSent);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);

        sprintf(buf, "Loops Detected = %u",
            rsvpStat->numLoopsDetected);
        IO_PrintStat(
            node,
            "Transport",
            "RSVP",
            ANY_DEST,
            ANY_INTERFACE, // instance Id ,
            buf);
    } // End if

    if (RSVP_DEBUG_OUTPUT)
    {
       RsvpPrintPSB(node);
       RsvpPrintRSB(node);
       RsvpPrintFTNAndILM(node);
       RsvpPrintHelloList(node);
    }
}

// The following code has been added for integrating
// IP + MPLS
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
    int interfaceIndex = 0; //INVALID_INTERFACE_INDEX;
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
// FUNCTION     : IsIngressRsvp()
//
// PURPOSE      : to check if the node is Ingress for the RSVP application
//
// PARAMETERS   : node - the node which is checking the Network
//                       Forwarding Table.
//                msg -  msg/packet to be routed.
//
// RETURN VALUE : TRUE if the node is an Ingress, FALSE otherwise
//-------------------------------------------------------------------------
BOOL IsIngressRsvp(Node *node, Message* msg)
{
    IpHeaderType* ipHeader = (IpHeaderType*)  MESSAGE_ReturnPacket(msg);
    int i;
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ipHeader->ip_src == NetworkIpGetInterfaceAddress(node, i))
        {
            break;
        }
    }

    NodeAddress nextHopAddress = (NodeAddress) NETWORK_UNREACHABLE;

    if (IsNextHopExistForThisDest(node, ipHeader->ip_dst, &nextHopAddress))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}
