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
// PURPOSE : This is an implementation of the extensions to the OSPF
//           protocol to support QoS routes following the RFC 2676.
//
// ASSUMPTION: 1. The scope of QoS route computation is currently
//                limited to a single area.
//             2. All routers within the area are assumed to run a QoS
//                enabled version of OSPF and all interfaces on this
//                router are assumed to be QoS capable.
//             3. Extended Breadth First Search for single path algorithm
//                is used to accommodate more than one Qos metrics during
//                path calculation.
//             4. No resourced are reserved, all routers monitors and floods
//                it's link utilization. So some times the end-to-end may
//                not satisfy QoS requirements after granting connection.
//


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"
#include "qualnet_error.h"
#include "network_ip.h"
#include "routing_ospfv2.h"
#include "transport_udp.h"
#include "routing_qospf.h"

#define QOSPF_DEBUG               0
#define QOSPF_DEBUG_OUTPUT        0
#define QOSPF_DEBUG_FINALIZE      0
#define QOSPF_DEBUG_PATH_CAL      0
#define QOSPF_DEBUG_OUTPUT_FILE   0

//--------------------------------------------------------------------------
//                       Some utility functions
//--------------------------------------------------------------------------

//
// FUNCTION: QospfTrace
// PURPOSE:  Prints trace info in a trace file.
// PARAMETERS:  Node* node
//                  node data
//              QospfData* qospf
//                  qospf data
//              char* traceInfo
//                  trace information.
// RETURN: None.
//
void QospfTrace(
    Node* node, QospfData* qospf, char* traceInfo)
{
    FILE *fp;
    static UInt32 count = 0;
    char currentTime[MAX_STRING_LENGTH];
    TIME_PrintClockInSecond(node->getNodeTime(), currentTime);

    if (qospf->trace == FALSE)
    {
        return;
    }

    if (count == 0)
    {
        fp = fopen("qospfTrace.asc", "w");
        ERROR_Assert(fp != NULL,
            "QospfTrace: file initial open error.\n");
    }
    else
    {
        fp = fopen("qospfTrace.asc", "a");
        ERROR_Assert(fp != NULL,
            "QospfTrace: file open error.\n");
    }
    count++;

    fprintf(fp, " Node %3u  %15s    %s\n",
        node->nodeId, currentTime, traceInfo);
    fclose(fp);
}


//
// FUNCTION: PrintBufContains
// PURPOSE:  Prints a string collected in a buffer
// PARAMETERS:  char* buf
//                      Collected string
//              BOOL printLine
//                      Flag, if true print a dash line
// RETURN: None.
//
static
void PrintBufContains(const char* buf, BOOL printLine)
{
    int i;
    int count;

    printf("   |");
    if (printLine)
    {
        for (i = 4; i < 76; i++)
        {
            printf("-");
        }
    }
    else
    {
        count = (int)strlen(buf);
        printf("%s", buf);
        for (i = count + 4; i < 76; i++)
        {
            printf(" ");
        }
    }
    printf("|\n");
}


//
// FUNCTION: QospfPrintSessionInformation
// PURPOSE:  Prints the list of connection exist in a node
// PARAMETERS:  Node* node
//                      Pointer to node
// RETURN: None.
//
static
void QospfPrintSessionInformation(Node* node)
{
    ListItem* tempListItem;
    char srcAddr[MAX_ADDRESS_STRING_LENGTH];
    char destAddr[MAX_ADDRESS_STRING_LENGTH];
    QospfDifferentSessionInformation* sessionInfo;
    char getTimeInSecond[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int  count;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;


    tempListItem = qospf->pathList->first;

    if (qospf->pathList->size)
    {
        printf("\n\n");
        PrintBufContains(buf, TRUE);
        sprintf(buf,"          Status of different sessions "
            "originated from node %u", node->nodeId);
        PrintBufContains(buf, FALSE);

        while (tempListItem)
        {
            sessionInfo = (QospfDifferentSessionInformation*)
                    tempListItem->data;
            if (sessionInfo->isOriginator)
            {
                PrintBufContains(buf, TRUE);
                sprintf(buf, "Informations for Session %d",
                    sessionInfo->sessionId);
                PrintBufContains(buf, FALSE);

                TIME_PrintClockInSecond(tempListItem->timeStamp,
                                        getTimeInSecond);
                sprintf(buf,"    Session requested at %s second",
                    getTimeInSecond);
                PrintBufContains(buf, FALSE);

                IO_ConvertIpAddressToString(
                    sessionInfo->sourceAddress, srcAddr);
                IO_ConvertIpAddressToString(
                    sessionInfo->destAddress, destAddr);

                sprintf(buf,"    Source Addr %s Dest Addr %s",
                    srcAddr, destAddr);
                PrintBufContains(buf, FALSE);

                sprintf(buf,"    Source port %d Dest port %d",
                    sessionInfo->sourcePort, sessionInfo->destPort);
                PrintBufContains(buf, FALSE);
                PrintBufContains(" ", FALSE);

                sprintf(buf,"    QoS requirement.....");
                PrintBufContains(buf, FALSE);
                sprintf(buf,"       Bandwidth %u bps",
                    sessionInfo->requestedMetric.availableBandwidth);
                PrintBufContains(buf, FALSE);
                sprintf(buf,"       End to end delay %u micro seconds",
                    sessionInfo->requestedMetric.averageDelay);
                PrintBufContains(buf, FALSE);
                sprintf(buf,"       User priority %u.",
                    sessionInfo->requestedMetric.priority);
                PrintBufContains(buf, FALSE);
                PrintBufContains(" ", FALSE);

                sprintf(buf,"    Path information....");
                PrintBufContains(buf, FALSE);
                if (sessionInfo->numRouteAddresses)
                {
                    TIME_PrintClockInSecond(sessionInfo->sessionAdmittedAt,
                                            getTimeInSecond);
                    sprintf(buf,"       Session admitted in %u attempt(s)",
                        sessionInfo->numTimesRetried);
                    PrintBufContains(buf, FALSE);
                    sprintf(buf,"       And finally admitted at %s seconds",
                        getTimeInSecond);
                    PrintBufContains(buf, FALSE);
                    sprintf(buf,"       Num hop %d",
                        sessionInfo->numRouteAddresses);
                    PrintBufContains(buf, FALSE);
                    sprintf(buf,"       Path:  ");

                    for (count = 0;
                         count < sessionInfo->numRouteAddresses;
                         count++)
                    {
                        sprintf(buf + strlen(buf),"%x  ",
                            sessionInfo->routeAddresses[count]);
                    }
                    PrintBufContains(buf, FALSE);
                }
                else
                {
                    sprintf(buf, "       Session retried %u times but "
                        "not admitted", sessionInfo->numTimesRetried);
                    PrintBufContains(buf, FALSE);
                }

                PrintBufContains(" ", FALSE);
            }

            tempListItem = tempListItem->next;
        }
        PrintBufContains(" ", FALSE);
        PrintBufContains(buf, TRUE);
    }
}


//
// FUNCTION: ReturnSessionId
// PURPOSE:  Gives session Id of a session.
// PARAMETERS:  Node* node
//                      Pointer to node
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              AppQosToNetworkSend* info
//                      Pointer to that location where informations
//                      for that session available.
// RETURN: None.
//
static
int ReturnSessionId(
    Node* node, QospfData* qospf, AppQosToNetworkSend* info)
{
    ListItem* tempListItem;
    QospfDifferentSessionInformation* getPath;

    tempListItem = qospf->pathList->first;
    while (tempListItem)
    {
        getPath = (QospfDifferentSessionInformation*)
                tempListItem->data;

        if (((getPath->sourcePort == info->sourcePort)
              && (getPath->destPort == info->destPort))
            && ((getPath->sourceAddress == info->sourceAddress)
                 && (getPath->destAddress == info->destAddress)))
        {
            return getPath->sessionId;
        }

        tempListItem = tempListItem->next;
    }
    return 0;
}


//
// FUNCTION: QospfPrintSourceRoutedPath
// PURPOSE:  Prints total path for a connection
// PARAMETERS:  Node* node
//                      Pointer to node
//              NodeAddress nodeAddress
//                      Total route from source to destination
//              int numAdress
//                      Number of elements present in path.
// RETURN: None.
//
static
void QospfPrintSourceRoutedPath(
    Node* node,
    NodeAddress* nodeAddress,
    int numAdress)
{
   int count;
   printf ("Source route (according to nodeId): \t %d ->", node->nodeId);

   for (count = 0; count < numAdress; count++)
   {
       printf("%d  ", MAPPING_GetNodeIdFromInterfaceAddress(
                         node, *(nodeAddress + count)));
   }
   printf("\n");
}


//--------------------------------------------------------------------------
//                  Update and Modify the database
//--------------------------------------------------------------------------

//
// FUNCTION: QospfPrintVertexDatabase
// PURPOSE:  Prints the Vertex set present in a node
// PARAMETERS:  Node* node
//                      Pointer to node
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
// RETURN: None.
//
static
void QospfPrintVertexDatabase(Node* node, QospfData* qospf)
{
    ListItem* tempListItem;
    ListItem* tempListForInterfaceInfo;
    ListItem* tempListForNeighborAddr;
    QospfSetOfVertices* getVertex;
    QospfInterfaceInfo* getInterfaceInfo;
    NodeAddress* getConnectedNeighbor;

    ERROR_Assert(qospf->vertexSet, "Vertex set not initialized");
    tempListItem = qospf->vertexSet->first;

    printf("\n\n\t\t------------------------\n");
    printf("\t\t|Vertex set for node %u |\n", node->nodeId);
    printf("\t\t-------------------------\n");
    while (tempListItem)
    {
        getVertex = (QospfSetOfVertices*) tempListItem->data;

        ERROR_Assert(getVertex, "No vertex found");

        if (getVertex->vertexType == QOSPF_ROUTER)
        {
            printf("\n\nVertex Number =  %d", getVertex->vertexNumber);
            printf("\tVertex Addr.  =  %x", getVertex->vertexAddress);
            printf("\tVertex Type   =  %d", getVertex->vertexType);
            printf("\n--------------------------------------------------");

            printf("\nInterfaces Of this Vertex :");

            ERROR_Assert(getVertex->vertexInterfaceInfo,
                "Vertex Interface Info list not initialize");

            tempListForInterfaceInfo =
                getVertex->vertexInterfaceInfo->first;

            while (tempListForInterfaceInfo)
            {
                getInterfaceInfo = (QospfInterfaceInfo*)
                    tempListForInterfaceInfo->data;

                ERROR_Assert(getInterfaceInfo,
                    "Data not found in vertex interface info list");

                printf("\n");
                printf("\n\tInterface %d",
                    getInterfaceInfo->ownInterfaceIndex);
                printf("\n\tInterface IP address %x",
                    getInterfaceInfo->ownInterfaceIpAddr);

                printf("\n\tIP Address of neighbors conntd "
                    "to this interface");
                tempListForNeighborAddr =
                    getInterfaceInfo->connectedInterfaceIpAddr->first;

                while (tempListForNeighborAddr)
                {
                    getConnectedNeighbor = (NodeAddress*)
                        tempListForNeighborAddr->data;

                    printf("\n\t\tIP Address : %x", *getConnectedNeighbor);
                    tempListForNeighborAddr = tempListForNeighborAddr->next;
                }
                tempListForInterfaceInfo = tempListForInterfaceInfo->next;
            }
        }

        tempListItem = tempListItem->next;
        printf("\n");
    }
}


//
// FUNCTION: QospfCreateVertexDatabase
// PURPOSE:  Collects the set of routers present in the area
// PARAMETERS:  Node* node
//                      Pointer to node
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              Ospfv2LinkStateHeader* LSHeader
//                      Pointer to LSA header.
// RETURN: None.
//
static
void QospfCreateVertexDatabase(
    Node* node,
    QospfData* qospf,
    Ospfv2LinkStateHeader* LSHeader)
{
    ListItem* tempListItem;
    QospfInterfaceInfo* tempInterfaceInfo;
    QospfSetOfVertices* tempVertex;
    QospfSetOfVertices* getVertex;
    BOOL isVertexPresent = FALSE;
    int linkCount;
    int numTos;

    Ospfv2RouterLSA* routerLSA;
    Ospfv2LinkInfo*  linkList;
    QospfPerLinkQoSMetricInfo* qosInfo;

    ERROR_Assert(qospf->vertexSet, "No vertex set found");
    tempListItem = qospf->vertexSet->first;

    // Search entire list for the presence of this Vertex
    while (tempListItem)
    {
        getVertex = (QospfSetOfVertices*) tempListItem->data;

        ERROR_Assert(getVertex, "No vertex found");

        if ((getVertex->vertexAddress == LSHeader->linkStateID)
            && (getVertex->vertexType == LSHeader->linkStateType))
        {
            isVertexPresent = TRUE;
            break;
        }
        tempListItem = tempListItem->next;
    }

    // If the vertex is not present, then insert it
    if (!isVertexPresent)
    {
        tempVertex = (QospfSetOfVertices*)
                MEM_malloc(sizeof(QospfSetOfVertices));

        tempVertex->vertexNumber = qospf->vertexSet->size + 1;
        tempVertex->vertexAddress = LSHeader->linkStateID;

        // Find and collect all the informations of each interface,
        // described in the LSA.
        ListInit(node, &tempVertex->vertexInterfaceInfo);

        // Collects informations only for router LSA
        if (LSHeader->linkStateType == OSPFv2_ROUTER)
        {
            // Get vertex type
            tempVertex->vertexType = QOSPF_ROUTER;

            routerLSA = (Ospfv2RouterLSA*) LSHeader;
            linkList = (Ospfv2LinkInfo*) (routerLSA + 1);

            // For other interfaces
            for (linkCount = 0;
                 linkCount < routerLSA->numLinks;
                 linkCount++)
            {
                if (linkList->type == OSPFv2_POINT_TO_POINT ||
                    linkList->type == OSPFv2_TRANSIT)
                {
                    tempInterfaceInfo = (QospfInterfaceInfo*)
                        MEM_malloc(sizeof(QospfInterfaceInfo));

                    tempInterfaceInfo->ownInterfaceIpAddr =
                        linkList->linkData;

                    numTos = linkList->numTOS;
                    qosInfo = (QospfPerLinkQoSMetricInfo*)
                            (linkList + 1);

                    tempInterfaceInfo->ownInterfaceIndex =
                        qosInfo->interfaceIndex;

                    ListInit(node,
                             &tempInterfaceInfo->connectedInterfaceIpAddr);

                    ListInsert(node,
                               tempVertex->vertexInterfaceInfo,
                               0,
                               (void*)tempInterfaceInfo);

                    linkList = (Ospfv2LinkInfo*) (qosInfo + numTos);
                }
                else
                {
                    // Go for next link
                    numTos = linkList->numTOS;
                    qosInfo = (QospfPerLinkQoSMetricInfo*)
                            (linkList + 1);
                    linkList = (Ospfv2LinkInfo*) (qosInfo + numTos);
                }
            }
        }

        // Insert the item into Vertex set
        ListInsert(node, qospf->vertexSet, 0, (void*)tempVertex);
    }
}


//
// FUNCTION: QospfPrintLinkDatabase
// PURPOSE:  Prints the Link database present in a node
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
// RETURN: None.
//
static
void QospfPrintLinkDatabase(Node* node, QospfData* qospf)
{
    int qCount;
    ListItem* firstListItem;
    ListItem* secondListItem;

    QospfLinkedFrom* getLinkedFrom;
    QospfLinkedTo*   getLinkedTo;

    printf("\n\n\t\t-------------------------------------\n");
    printf("\t\t| Link database present in node %3u |\n", node->nodeId);
    printf("\t\t-------------------------------------\n");

    ERROR_Assert(qospf->topologyLinksList, "Topology list not initialized");

    firstListItem = qospf->topologyLinksList->first;

    while (firstListItem)
    {
        getLinkedFrom = (QospfLinkedFrom*)firstListItem->data;

        ERROR_Assert(getLinkedFrom,
            "Originating node for the link not found");

        printf("\nLink extend from the nodeId %x (vertex num %d)\n",
            getLinkedFrom->fromRouterId, getLinkedFrom->fromVertexNumber);

        ERROR_Assert(getLinkedFrom->linkToInfoList,
            "Error in topology list");

        secondListItem = getLinkedFrom->linkToInfoList->first;

        while (secondListItem)
        {
            getLinkedTo = (QospfLinkedTo*)secondListItem->data;

            ERROR_Assert(getLinkedTo, "Ending node for the link not found");

            printf("\tto nodeId %x (vertex num %d) at interface IP %x\n",
                getLinkedTo->toRouterId,
                getLinkedTo->toVertexNumber,
                getLinkedTo->neighborIpAddr);

            printf("\twhere network address %x and mask %x\n",
                getLinkedTo->networkAddr, getLinkedTo->networkMask);

            printf("\t\twhere own Interface index and Ip are %d and %x\n",
                    getLinkedTo->interfaceIndex, getLinkedTo->ownIpAddr);

            printf("\tType of the link %d\n", getLinkedTo->linkType);
            printf("\t-------------------------------------\n");
            printf("\t\t... and qos metrics of different "
                "queue at interface");
            printf(" %d are \n", getLinkedTo->interfaceIndex);

            for (qCount = 0;
                 qCount < getLinkedTo->numQueues;
                 qCount++)
            {
                printf("\t\t\tQueue No = %d, Bandwidth = %d, Delay = %d\n",
                    getLinkedTo->queueSpecificCost[qCount].queueNumber,
                    getLinkedTo->queueSpecificCost[qCount].
                            availableBandwidth,
                    getLinkedTo->queueSpecificCost[qCount].averageDelay);
            }
            printf("\n");

            secondListItem = secondListItem->next;
        }
        firstListItem = firstListItem->next;
    }
}


//
// FUNCTION: QospfInsertNewElementIntoLinkDatabase
// PURPOSE:  If no link database present for a router, it creates all
//           the lists and stores the database for that node.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              Ospfv2LinkStateHeader* LSHeader,
//                      Pointer to LSA header.
// RETURN: None.
//
static
void QospfInsertNewElementIntoLinkDatabase(
    Node* node,
    QospfData* qospf,
    Ospfv2LinkStateHeader* LSHeader)
{
    int linkCounter;
    int index;

    QospfLinkedFrom* tempLinkedFrom;
    QospfLinkedTo*   tempLinkedTo;
    Ospfv2RouterLSA* routerLSA;
    Ospfv2LinkInfo*  linkList;
    QospfPerLinkQoSMetricInfo* qosInfo;

    tempLinkedFrom = (QospfLinkedFrom*)
            MEM_malloc(sizeof(QospfLinkedFrom));

    memset(tempLinkedFrom, 0, sizeof(QospfLinkedFrom));

    tempLinkedFrom->fromRouterId = LSHeader->advertisingRouter;

     // Initialize linkToInfoList, which collects QospfLinkedTo
     // structure in it's data field.
    ListInit(node, &tempLinkedFrom->linkToInfoList);

    routerLSA = (Ospfv2RouterLSA*) LSHeader;
    linkList = (Ospfv2LinkInfo*) (routerLSA + 1);

    for (linkCounter = 0; linkCounter < routerLSA->numLinks; linkCounter++)
    {
        if (linkList->type == OSPFv2_POINT_TO_POINT)
        {
            tempLinkedTo = (QospfLinkedTo*)
                    MEM_malloc(sizeof(QospfLinkedTo));

            memset(tempLinkedTo, 0, sizeof(QospfLinkedTo));

            tempLinkedTo->toRouterId = linkList->linkID;

            // Get own and neighbor IP address
            tempLinkedTo->ownIpAddr = linkList->linkData;
            tempLinkedTo->neighborIpAddr =
                (NodeAddress) QOSPF_NEXT_HOP_UNREACHABLE;

            // Get type of the link
            tempLinkedTo->linkType = (Ospfv2LinkType) linkList->type;

            // Get all QoS metric informations
            qosInfo = (QospfPerLinkQoSMetricInfo*) (linkList + 1);

            tempLinkedTo->interfaceIndex = qosInfo->interfaceIndex;

            tempLinkedTo->numQueues =
                linkList->numTOS / QOSPF_NUM_TOS_METRIC;

            tempLinkedTo->queueSpecificCost =
                    (QospfQosConstraint*) MEM_malloc(
                        tempLinkedTo->numQueues
                                * sizeof(QospfQosConstraint));

            // Retrieve queue specific delay and bandwidth from Router LSA
            for (index = 0;
                 index < tempLinkedTo->numQueues;
                 index++)
            {
                tempLinkedTo->queueSpecificCost[index].queueNumber =
                        qosInfo->queueNumber;

                // Collects available bandwidth
                if (qosInfo->qos == QOSPF_BANDWIDTH)
                {
                    tempLinkedTo->
                        queueSpecificCost[index].availableBandwidth =
                            (unsigned) ((qosInfo->qosMetricMantissaPart *
                                pow((double)QOSPF_QOS_METRIC_BASE_VAL,
                                    qosInfo->qosMetricExponentPart)) * 8.0);

                    index--;
                }
                // Collects link propagation delay
                else
                {
                    tempLinkedTo->queueSpecificCost[index].averageDelay =
                            (unsigned) (qosInfo->qosMetricMantissaPart *
                                pow((double)QOSPF_QOS_METRIC_BASE_VAL,
                                    qosInfo->qosMetricExponentPart));
                }

                // Increment the pointer for next metric
                qosInfo = qosInfo + 1;
            }

            // Insert this link information to linkToInfoList
            ListInsert(node,
                       tempLinkedFrom->linkToInfoList,
                       0,
                       (void*)tempLinkedTo);

            // Place the linkList pointer to next link
            linkList = (Ospfv2LinkInfo*) qosInfo;
        }
        else
        {
            int numTos;

            // Place the linkList pointer properly
            numTos = linkList->numTOS;
            linkList = (Ospfv2LinkInfo*)
                ((QospfPerLinkQoSMetricInfo*)(linkList + 1) + numTos);
        }
    }

    // Insert all link informations for this router to topologyLinksList
    ListInsert(node, qospf->topologyLinksList, 0, (void*)tempLinkedFrom);
}


//
// FUNCTION: QospfCreateLinkDatabase
// PURPOSE:  Collects the topological database of the area in which the node
//           presents for Q-OSPF in a link by link manner. This function is
//           called when a new connection request arrives.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure.
//              Ospfv2LinkStateHeader* LSHeader
//                      Pointer to LSA header.
// RETURN: None.
//
static
void QospfCreateLinkDatabase(
    Node* node,
    QospfData* qospf,
    Ospfv2LinkStateHeader* LSHeader)
{
    ListItem* tempListItem;
    BOOL isElementPresent = FALSE;
    QospfLinkedFrom* getLinkedFrom;

    ERROR_Assert(qospf->topologyLinksList,
        "Topology link list not initialized");

    tempListItem = qospf->topologyLinksList->first;

   // Search entire list to check the existence of this LSA
    while (tempListItem)
    {
        getLinkedFrom = (QospfLinkedFrom*) tempListItem->data;

        ERROR_Assert(getLinkedFrom, "Topology not found");

        if (getLinkedFrom->fromRouterId == LSHeader->advertisingRouter)
        {
            isElementPresent = TRUE;
            break;
        }
        tempListItem = tempListItem->next;
    }

    // No entry for this element, so insert it
    if (!isElementPresent)
    {
        QospfInsertNewElementIntoLinkDatabase(node, qospf, LSHeader);
    }
}


//
// FUNCTION: QospfMapNeighborAddress
// PURPOSE:  Finds neighbor connected to each interface of a node.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              NodeAddress fromRouterId,
//                      routerId from where link originated.
//              NodeAddress toRouterId,
//                      routerId where link connected.
//              NodeAddress neighborIpAddress.
//                      Neighbor Ip address.
//              NodeAddress netAddress
//                      Network Address
//              NodeAddress netMask
//                      Network Mask
// RETURN: None.
//
static
void QospfMapNeighborAddress(
    Node* node,
    QospfData*  qospf,
    NodeAddress fromRouterId,
    NodeAddress toRouterId,
    NodeAddress neighborIpAddress,
    NodeAddress netAddress,
    NodeAddress netMask)
{
    ListItem* fromListItem;
    ListItem* toListItem;

    QospfLinkedFrom* getLinkedFrom;
    QospfLinkedTo*   getLinkedTo;

    BOOL isNeighborIPAssigned = FALSE;

    ERROR_Assert(qospf->topologyLinksList,
        "Topology link list not initialized");

    fromListItem = qospf->topologyLinksList->first;

   // Search entire topology list
    while (fromListItem)
    {
        getLinkedFrom = (QospfLinkedFrom*) fromListItem->data;

        ERROR_Assert(getLinkedFrom, "Topology not found");

        if (getLinkedFrom->fromRouterId == toRouterId)
        {
            toListItem = getLinkedFrom->linkToInfoList->first;

            while (toListItem)
            {
                getLinkedTo = (QospfLinkedTo*) toListItem->data;
                ERROR_Assert(getLinkedTo, "Topology not found");

                if ((getLinkedTo->toRouterId == fromRouterId))
                {
                    if (netAddress ==
                        (getLinkedTo->ownIpAddr & netMask))
                    {
                        getLinkedTo->neighborIpAddr = neighborIpAddress;
                        isNeighborIPAssigned = TRUE;
                    }
                }

                if (isNeighborIPAssigned)
                {
                    break;
                }

                toListItem = toListItem->next;
            }
        }

        if (isNeighborIPAssigned)
        {
            break;
        }

        fromListItem = fromListItem->next;
    }
}


//
// FUNCTION: QospfAssignVertexNumber
// PURPOSE:  Assign pseudo vertex number into topology link list
// PARAMETERS:  QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              NodeAddress vertexAddress
//                      Vertex router Id
// RETURN: None.
//
static
int QospfAssignVertexNumber(QospfData* qospf, NodeAddress vertexAddress)
{
    ListItem* vertexListItem;
    QospfSetOfVertices* getVertexSetData;

    ERROR_Assert(qospf->vertexSet, "Vertex set list not initialized");
    vertexListItem = qospf->vertexSet->first;

    while (vertexListItem)
    {
        getVertexSetData = (QospfSetOfVertices*)
                vertexListItem->data;

        if (getVertexSetData->vertexType == QOSPF_ROUTER &&
            getVertexSetData->vertexAddress == vertexAddress)
        {
            return getVertexSetData->vertexNumber;
        }
        vertexListItem = vertexListItem->next;
    }

    // Shouldn't get here. return is given to
    // avoid compilation message
    ERROR_Assert(FALSE, "Corrupted Database");
    return 0;
}


//
// FUNCTION: QospfGetNetworkAddrAndMask
// PURPOSE:  Assigns network address and network mask for a link
// PARAMETERS:  Node* node
//                      Pointer to node
//              NodeAddress fromRouterId
//                      router Id from where link originated
//              NodeAddress ownAddress
//                      IP address of that interface from
//                      where link originated
//              NodeAddress* networkAddress
//                      pointer collects network address
//              NodeAddress* networkMask
//                      pointer collects network mask
// RETURN: None.
//
static
void QospfGetNetworkAddrAndMask(
    Node* node,
    NodeAddress fromRouterId,
    NodeAddress ownAddress,
    NodeAddress* networkAddress,
    NodeAddress* networkMask)
{
    int numTos;
    int linkCounter;
    NodeAddress netAddr;
    Ospfv2ListItem* item;
    Ospfv2Area* area = NULL;
    Ospfv2LinkStateHeader* LSHeader = NULL;
    Ospfv2RouterLSA* routerLSA;
    Ospfv2LinkInfo*  linkList;

    area = Ospfv2GetArea(node, OSPFv2_BACKBONE_AREA);
    ERROR_Assert(area, "Area doesn't exist\n");

    item = area->routerLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;
        if (LSHeader->advertisingRouter == fromRouterId)
        {
            break;
        }
        item = item->next;
    }

    if (LSHeader == NULL)
    {
        char errMsg[MAX_STRING_LENGTH];

        sprintf(errMsg,
            "LSA not found for Router id %x\n at node %u\n",
            fromRouterId, node->nodeId);
        ERROR_Assert(FALSE, errMsg);
    }

    routerLSA = (Ospfv2RouterLSA*) LSHeader;
    linkList = (Ospfv2LinkInfo*) (routerLSA + 1);

    for (linkCounter = 0;
         linkCounter < routerLSA->numLinks;
         linkCounter++)
    {
        if (linkList->type == OSPFv2_STUB)
        {
            netAddr = linkList->linkData & ownAddress;

            if (netAddr == linkList->linkID)
            {
                *networkAddress = netAddr;
                *networkMask = linkList->linkData;
                break;
            }
        }

        // Place the linkList pointer to next link
        numTos = linkList->numTOS;
        linkList = (Ospfv2LinkInfo*)
            ((QospfPerLinkQoSMetricInfo*)(linkList + 1) + numTos);
    }
}


//
// FUNCTION: QospfGetNeighborIpForLinkDatabase
// PURPOSE:  Finds neighbor connected to each interface of a
//           node in entire link database.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
// RETURN: None.
//
static
void QospfGetNeighborIpForLinkDatabase(Node* node, QospfData* qospf)
{
    ListItem* fromListItem;
    ListItem* toListItem;

    QospfLinkedFrom* getLinkedFrom;
    QospfLinkedTo*   getLinkedTo;

    ERROR_Assert(qospf->topologyLinksList,
        "Topology link list not initialized");

    fromListItem = qospf->topologyLinksList->first;

    // Search entire topology list and find
    // neighbor connected to each interface
    while (fromListItem)
    {
        getLinkedFrom = (QospfLinkedFrom*) fromListItem->data;

        ERROR_Assert(getLinkedFrom, "Topology not found");

        getLinkedFrom->fromVertexNumber =
            QospfAssignVertexNumber(qospf, getLinkedFrom->fromRouterId);

        toListItem = getLinkedFrom->linkToInfoList->first;
        while (toListItem)
        {
            getLinkedTo = (QospfLinkedTo*) toListItem->data;
            ERROR_Assert(getLinkedTo, "Topology not found");

            getLinkedTo->toVertexNumber =
                QospfAssignVertexNumber(qospf, getLinkedTo->toRouterId);

            QospfGetNetworkAddrAndMask(
                node,
                getLinkedFrom->fromRouterId,
                getLinkedTo->ownIpAddr,
                &getLinkedTo->networkAddr,
                &getLinkedTo->networkMask);

            QospfMapNeighborAddress(
                node,
                qospf,
                getLinkedFrom->fromRouterId,
                getLinkedTo->toRouterId,
                getLinkedTo->ownIpAddr,
                getLinkedTo->networkAddr,
                getLinkedTo->networkMask);

            toListItem = toListItem->next;
        }
        fromListItem = fromListItem->next;
    }
}


//
// FUNCTION: QospfGetNeighborIpForVertexDatabase
// PURPOSE:  Finds neighbor connected to each interface of a
//           node in entire vertex database.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
// RETURN: None.
//
static
void QospfGetNeighborIpForVertexDatabase(Node* node, QospfData* qospf)
{
    ListItem* vertexListItem;
    ListItem* fromListItem;
    ListItem* toListItem;
    ListItem* interfaceListItem;
    BOOL isNeighborIPAssigned;

    QospfLinkedFrom*    getLinkFromData;
    QospfLinkedTo*      getlinkToData;
    QospfSetOfVertices* getVertexSetData;
    QospfInterfaceInfo* getInterfaceInfo;
    NodeAddress* tempNodeAddr;

    // Collects neighbor connected to each interface of
    // a vertex in entire vertex database.
    ERROR_Assert(qospf->topologyLinksList,
        "Topology link list not initialized");
    fromListItem = qospf->topologyLinksList->first;

    while (fromListItem)
    {
        getLinkFromData = (QospfLinkedFrom*) fromListItem->data;
        ERROR_Assert(getLinkFromData , "Topology entry not found");

        isNeighborIPAssigned = FALSE;

        ERROR_Assert(qospf->vertexSet, "Vertex set list not initialized");
        vertexListItem = qospf->vertexSet->first;

        while (vertexListItem)
        {
            getVertexSetData = (QospfSetOfVertices*)
                    vertexListItem->data;

            // Vertex matched with from router Id. So get all interface
            // informations collected in QospfLinkedTo structure and
            // assign neighbor properly to every interface of the vertex.
            if (getVertexSetData->vertexAddress ==
                    getLinkFromData->fromRouterId)
            {
                // Retrieve connected link information from topology
                // database for this vertex.
                ERROR_Assert(getLinkFromData->linkToInfoList,
                    "LinkedTo list not initialized");
                toListItem = getLinkFromData->linkToInfoList->first;

                while (toListItem)
                {
                    getlinkToData = (QospfLinkedTo*) toListItem->data;
                    ERROR_Assert(getlinkToData,
                        "Topology entry not found");

                    // Retrieve interface information for this vertex
                    ERROR_Assert(getVertexSetData->vertexInterfaceInfo,
                        "Interface info list in vertex set "
                        "not initialized");

                    interfaceListItem =
                        getVertexSetData->vertexInterfaceInfo->first;

                    isNeighborIPAssigned = FALSE;

                    while (interfaceListItem)
                    {
                        getInterfaceInfo = (QospfInterfaceInfo*)
                                interfaceListItem->data;
                        ERROR_Assert(getInterfaceInfo,
                            "Interface information entry not found");

                        // Interface from linkTo in topology database
                        // matched with interface of this vertex. So
                        // collect neighbor from linkTo to vertex database
                        if (getlinkToData->interfaceIndex ==
                                getInterfaceInfo->ownInterfaceIndex)
                        {
                            tempNodeAddr = (NodeAddress*)
                                    MEM_malloc(sizeof(NodeAddress));

                            *tempNodeAddr = getlinkToData->neighborIpAddr;

                            ERROR_Assert(
                                getInterfaceInfo->connectedInterfaceIpAddr,
                                "Connected neighbor list not initialized");

                            ListInsert(
                                node,
                                getInterfaceInfo->connectedInterfaceIpAddr,
                                0,
                                (void*)tempNodeAddr);

                            isNeighborIPAssigned = TRUE;
                        }

                        // Neighbor for this interface assigned.
                        // So find next interface.
                        if (isNeighborIPAssigned)
                        {
                            break;
                        }
                        interfaceListItem = interfaceListItem->next;
                    }

                    toListItem = toListItem->next;
                }
            }

            // Neighbors of all interface for this vertex
            // assigned. Go for next vertex.
            if (isNeighborIPAssigned)
            {
                break;
            }

            vertexListItem = vertexListItem->next;
        }
        fromListItem = fromListItem->next;
    }
}


//
// FUNCTION: QospfCreateDatabase
// PURPOSE:  Creates database required for QoS path calculation
//           from Router LSA present in the OSPF Layer
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
// RETURN: None.
//
static
void QospfCreateDatabase(Node* node, QospfData* qospf)
{
    Ospfv2ListItem* item;
    Ospfv2Area* area = NULL;
    Ospfv2LinkStateHeader* LSHeader;

    ListInit(node, &qospf->vertexSet);
    ListInit(node, &qospf->topologyLinksList);

    area = Ospfv2GetArea(node, OSPFv2_BACKBONE_AREA);

    ERROR_Assert(area, "Area doesn't exist\n");

    if (QOSPF_DEBUG)
    {
        item = area->routerLSAList->first;
        while (item)
        {
            LSHeader = (Ospfv2LinkStateHeader*) item->data;
            Ospfv2PrintLSA((char*) LSHeader);
            item = item->next;
        }
    }

    // Get all the router LSA for this area and create
    // the data base require for Q-OSPF.
    item = area->routerLSAList->first;
    while (item)
    {
        LSHeader = (Ospfv2LinkStateHeader*) item->data;

        QospfCreateVertexDatabase(node, qospf, LSHeader);
        QospfCreateLinkDatabase(node, qospf, LSHeader);
        item = item->next;
    }

    // Now we have to rearrange the data base. We are collecting
    // neighbor address during this arrangement.
    // Note : Before vertex database arrangement link database
    // must be arranged first.

    if (QOSPF_DEBUG)
    {
        printf("\nLink Database before rearrangement....\n");
        QospfPrintLinkDatabase(node, qospf);

        printf("\n\nVertex database before rearrangement...\n");
        QospfPrintVertexDatabase(node, qospf);
    }

    QospfGetNeighborIpForLinkDatabase(node, qospf);

    QospfGetNeighborIpForVertexDatabase(node, qospf);

    if (QOSPF_DEBUG)
    {
        printf("\n\nVertex database after Rearrangement...\n");
        QospfPrintVertexDatabase(node, qospf);

        printf("\nLink Database after rearrangement....\n");
        QospfPrintLinkDatabase(node, qospf);
    }
}


//--------------------------------------------------------------------------
//                     Additional part of Router LSA
//--------------------------------------------------------------------------

//
// FUNCTION: QospfEncodeQosMetric
// PURPOSE:  Encode the metric value from linear representation to
//           exponential representation.
// PARAMETERS:  Ospfv2StatusOfQueue* queueStatus
//                      Current status of the queues present in each
//                      interface.
//              QospfPerLinkQoSMetricInfo** metricPointer
//                      Pointer to that location where the coded metric value
//                      will be inserted.
//              QospfTypeOfQos qosType
//                      Type of Quality of Service.
// RETURN: None.
//
static
void QospfEncodeQosMetric(
    Ospfv2StatusOfQueue* queueStatus,
    QospfPerLinkQoSMetricInfo** metricPointer,
    QospfTypeOfQos qosType,
    BOOL isQueueDelayConsidered)
{
    double metricValue = 0.0;
    double stepSize = 0.0;
    unsigned tempExponentPart = 0;

    // Get value of bandwidth to be encoded
    if (qosType == QOSPF_BANDWIDTH)
    {
        // Advertise bandwidth in Bytes/sec
        metricValue = (double) (queueStatus->availableBandwidth / 8.0);
    }
    // Get value of delay to be encoded
    else
    {
        // Queuing delay considered for path calculation, So advertise it
        if (isQueueDelayConsidered)
        {
            metricValue = (double) (queueStatus->propDelay +
                    queueStatus->qDelay);
        }
        // Queuing delay not considered. Advertise propagation delay only
        else
        {
            metricValue = (double) (queueStatus->propDelay);
        }
    }

    // Encode the linear value into exponential representation
    stepSize = (metricValue - 1) / QOSPF_QOS_METRIC_MANTISSA_MAX;

    if (stepSize < 1.0f)
    {
        tempExponentPart = 0;
    }
    else
    {
        tempExponentPart = (unsigned)(log10(stepSize)
                            / log10((double)QOSPF_QOS_METRIC_BASE_VAL));
        tempExponentPart++;
    }

    if (tempExponentPart >= QOSPF_QOS_METRIC_BASE_VAL)
    {
        ERROR_ReportError("\nOut of range metric value");
    }

    metricValue /= pow((double)QOSPF_QOS_METRIC_BASE_VAL, (int)tempExponentPart);

    if (QOSPF_DEBUG)
    {
        printf("Value before assignment %f\n", metricValue);
    }

    // Set exponent and mantissa part.
    (*metricPointer)->qosMetricExponentPart = tempExponentPart;
    (*metricPointer)->qosMetricMantissaPart = (int) metricValue;

    if (QOSPF_DEBUG)
    {
        printf("Value after assignment %d\n\n",
            (*metricPointer)->qosMetricMantissaPart);
    }
}


//
// FUNCTION: QospfGetQosInformationForTheLink
// PURPOSE:  Provides Qos related information for each link. As only
//           bandwidth and delay is considered, this function provides
//           these informations. This function is called from ospfv2.c
// PARAMETERS:  Node* node
//                      Pointer to node.
//              int interfaceIndex
//                      Interface index of that interface whose QoS metrics
//                      will be advertised.
//              Ospfv2LinkInfo** linkAddress
//                      Temporary pointer to the LinkInfo structure from
//                      OSPF after which the QoS information will be added.
// RETURN: None.
//
void QospfGetQosInformationForTheLink(
    Node* node,
    int interfaceIndex,
    Ospfv2LinkInfo** linkAddress)
{
    ListItem* queueListedItem;
    int utilizedLinkBw = 0;
    char traceInfo[MAX_STRING_LENGTH];

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;

    Ospfv2StatusOfQueue* getQueueStatus;

    QospfPerLinkQoSMetricInfo* getWorkingPointer =
            (QospfPerLinkQoSMetricInfo*) (*linkAddress);

    //
    // Figure shown below, is portion of router LSA used to advertise
    // additional TOS-specific information. This field is used by
    // Q-ospf to flood a Qos metric for the link.
    // +                                                               +
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |      TOS      |        0      |          TOS  metric          |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // +                                                               +
    // We are advertising current status of all the queues for each
    // interface. For this purpose we are using remaining 3 bits of TOS,
    // present before the 1 byte reserved field (according to the RFC2328),
    // to identify the particular Queue. The last five bits indicates type
    // of QoS advertised. Next one byte reserved field is used to identify
    // the interface index of the router from where the above advertised
    // link originates. Next 2 byte field TOS metric is used to advertise
    // Qos Metric. So the field becomes as below
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |queue| Qos Type| interfaceIndex|          QoS  metric          |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //

    ERROR_Assert(ospf->iface[interfaceIndex].presentStatusOfQueue,
        "presentStatusOfQueue list not initialized");

    queueListedItem =
        ospf->iface[interfaceIndex].presentStatusOfQueue->first;

    while (queueListedItem)
    {
        // Get each queue status
        getQueueStatus = (Ospfv2StatusOfQueue*) queueListedItem->data;

        ERROR_Assert(getQueueStatus, "Error in presentStatusOfQueue list");

        // Encode bandwidth
        getWorkingPointer->qos = QOSPF_BANDWIDTH;

        getWorkingPointer->queueNumber = getQueueStatus->queueNo;
        getWorkingPointer->interfaceIndex = (unsigned char) interfaceIndex;
        QospfEncodeQosMetric(getQueueStatus,
                             &getWorkingPointer,
                             QOSPF_BANDWIDTH,
                             qospf->isQueueDelayConsidered);

        // update the  pointer to insert Delay part
        getWorkingPointer = getWorkingPointer + 1;

        // Encode delay
        getWorkingPointer->qos = QOSPF_DELAY;

        getWorkingPointer->queueNumber = getQueueStatus->queueNo;
        getWorkingPointer->interfaceIndex = (unsigned char) interfaceIndex;
        QospfEncodeQosMetric(getQueueStatus,
                             &getWorkingPointer,
                             QOSPF_DELAY,
                             qospf->isQueueDelayConsidered);

        if (qospf->trace)
        {
            utilizedLinkBw += getQueueStatus->utilizedBandwidth;
            sprintf(traceInfo,
                "Interface[%u] status: QNo. %u  BW %u  Dlay %u",
                interfaceIndex, getQueueStatus->queueNo,
                getQueueStatus->availableBandwidth,
                (qospf->isQueueDelayConsidered ?
                    (getQueueStatus->propDelay + getQueueStatus->qDelay):
                    (getQueueStatus->propDelay)));

            QospfTrace(node, qospf, traceInfo);
        }

        // update the  pointer
        getWorkingPointer = getWorkingPointer + 1;

        // Point next queue structure
        queueListedItem = queueListedItem->next;
    }

    if (qospf->trace)
    {
        sprintf(traceInfo,
            "Interface[%u]:current utilized Bw %u prev utilized Bw %u ",
            interfaceIndex, utilizedLinkBw,
            ospf->iface[interfaceIndex].lastAdvtUtilizedBandwidth);

        QospfTrace(node, qospf, traceInfo);
    }

    // Change the linkAddress pointer to
    // point after QoS information field.
    *linkAddress = (Ospfv2LinkInfo*) getWorkingPointer;
}


//--------------------------------------------------------------------------
//      Functions related to  Q-ospf flooding and interface monitoring
//--------------------------------------------------------------------------

//
// FUNCTION: QospfGetLinkUtilization
// PURPOSE:  Returns total bits transmitted through the link in a unit time.
// PARAMETERS:  int transmitedUnit
//                      Total bits transmitted.
//              clocktype period
//                      Time during the bits transmitted.
// RETURN: int.
//
static
int QospfGetLinkUtilization(int transmitedUnit, clocktype period)
{
    int second;

    // Convert the period into Seconds
    second = (int) (period / SECOND);

    if (!second)
    {
        return 0;
    }

    transmitedUnit = ((transmitedUnit * 8) / second);
    return transmitedUnit;
}


//
// FUNCTION: QospfIsFloodingRequired
// PURPOSE:  Checks whether flooding is required to update
//           latest status of this node.
// PARAMETERS:  Node* node
//                      Pointer to node.
// RETURN: BOOL.
//
static
BOOL QospfIsFloodingRequired(Node* node)
{
    int   count;
    int   linkUtilization;
    int   currentUtilisedBw;
    int   previousUtilisedBw;
    float changeInUtilisedBw;
    float linkBandwidth = 0;
    ListItem* tempListItem;
    Ospfv2StatusOfQueue* queueStatus = NULL;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;

    // Check whether change in utilized bandwidth in any interface
    // exceeds flooding probability. If so, then allow flooding.
    for (count = 0; count < node->numberInterfaces; count++)
    {
        // Get particular scheduler for this interface
        Scheduler* scheduler = ip->interfaceInfo[count]->scheduler;
        linkUtilization = 0;

        // Get the presentStatusOfQueue structure from
        // interface structure present in OSPF structure.
        ERROR_Assert(ospf->iface[count].presentStatusOfQueue,
            "presentStatusOfQueue list not initialized");

        tempListItem = ospf->iface[count].presentStatusOfQueue->first;

        while (tempListItem)
        {
            int qDelayVal = -1;
            int totalTransmissionVal = -1;

            queueStatus = (Ospfv2StatusOfQueue*) tempListItem->data;

            (*scheduler).qosInformationUpdate(queueStatus->queueNo,
                                              &qDelayVal,
                                              &totalTransmissionVal,
                                              node->getNodeTime());

            linkUtilization += totalTransmissionVal;

            tempListItem = tempListItem->next;
        }

        // Get the change in bandwidth utilization
        currentUtilisedBw =
            QospfGetLinkUtilization(linkUtilization,
                                    qospf->interfaceMonitorInterval);

        previousUtilisedBw =
            ospf->iface[count].lastAdvtUtilizedBandwidth;

        changeInUtilisedBw = (float)
                abs(currentUtilisedBw - previousUtilisedBw);

        linkBandwidth = (float) queueStatus->linkBandwidth;

        // Check whether recent change exceeds flooding factor
        if ((changeInUtilisedBw / linkBandwidth) > qospf->floodingFactor)
        {
            return TRUE;
        }
    }

    return FALSE;
}


//
// FUNCTION: QospfPrintQueueStatus
// PURPOSE:  Prints the present queue status
// PARAMETERS:  Node* node
//                      Pointer to node.
// RETURN: None.
//
static
void QospfPrintQueueStatus(Node* node)
{
    int count;
    ListItem* tempListItem;
    Ospfv2StatusOfQueue* getQueueStatus;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    printf("\nSTATUS OF QUEUE at node %u\n", node->nodeId);

    for (count = 0; count < node->numberInterfaces; count++)
    {
        ERROR_Assert(ospf->iface[count].presentStatusOfQueue,
            "presentStatusOfQueue list not initialized");

        tempListItem = ospf->iface[count].presentStatusOfQueue->first;

        printf("\n  Queue info for interface %d", count);

        while (tempListItem)
        {
            getQueueStatus = (Ospfv2StatusOfQueue*)
                    tempListItem->data;

            ERROR_Assert(getQueueStatus,
                "Error in presentStatusOfQueue list");

            printf("\n\tQueue No. %d", getQueueStatus->queueNo);
            printf("\n\t\tTotal Link Bandwidth %d,  Utilized Bandwidth %d,",
                getQueueStatus->linkBandwidth,
                getQueueStatus->utilizedBandwidth);
            printf("\n\t\tPropagation Delay %d, Queuing Delay %d",
                getQueueStatus->propDelay,
                getQueueStatus->qDelay);

           tempListItem = tempListItem->next;
        }
        printf("\n");
    }
}


//
// FUNCTION: QospfUpdateQueueInformation
// PURPOSE:  Updates each queue's information in every interface of a node.
//           It returns a true value if a significant change in utilized
//           bandwidth occurred in any interface of the node.
// PARAMETERS:  Node* node
//                      Pointer to node.
// RETURN: BOOL.
//
static
BOOL QospfUpdateQueueInformation(Node* node)
{
    int count;
    int totalUtilisedBw;
    int utilisedBwForThisQueue;
    ListItem* tempListItem;
    Ospfv2StatusOfQueue* queueStatus;
    BOOL isFloodingNecessary = FALSE;
    char dataBuf[MAX_STRING_LENGTH] = {0};

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;

    if (QOSPF_DEBUG)
    {
        printf("\nNode %d updating Queuing Delay\n", node->nodeId);
    }

    isFloodingNecessary = QospfIsFloodingRequired(node);

    for (count = 0; count < node->numberInterfaces; count++)
    {
        // Get particular scheduler for this interface
        Scheduler* scheduler = ip->interfaceInfo[count]->scheduler;

        totalUtilisedBw = 0;

        // Get presentStatusOfQueue structure from interface
        // structure present in OSPF structure.
        ERROR_Assert(ospf->iface[count].presentStatusOfQueue,
            "presentStatusOfQueue list not initialized");

        tempListItem = ospf->iface[count].presentStatusOfQueue->first;

        if (QOSPF_DEBUG)
        {
            printf("\tQueueing Delay at interface %d\n", count);
        }

        while (tempListItem)
        {
            int qDelayVal = -1;
            int totalTransmissionVal = -1;

            queueStatus = (Ospfv2StatusOfQueue*) tempListItem->data;

            // This call includes Modification of average queuing delay,
            (*scheduler).qosInformationUpdate(queueStatus->queueNo,
                                              &qDelayVal,
                                              &totalTransmissionVal,
                                              node->getNodeTime(),
                                              true);

            utilisedBwForThisQueue =
                QospfGetLinkUtilization(totalTransmissionVal,
                                        qospf->interfaceMonitorInterval);

            totalUtilisedBw += utilisedBwForThisQueue;

            if (QOSPF_DEBUG)
            {
                printf("\t\tQueue %d : %d\n", queueStatus->queueNo,
                        queueStatus->qDelay);
                printf("\t\tUtilised bandwidth %d\n",
                        queueStatus->utilizedBandwidth);
            }

            // update queuing delay
            queueStatus->qDelay = qDelayVal;

            // update utilized bandwidth
            queueStatus->utilizedBandwidth = utilisedBwForThisQueue;

            // Update the available bandwidth. Available bandwidth for a
            // particular queue is total link bandwidth minus utilized
            // bandwidth of this queue and the previous queues (as we are
            // considering the STRICT-PRIORITY scheduling only).
            queueStatus->availableBandwidth =
                queueStatus->linkBandwidth - totalUtilisedBw;

            tempListItem = tempListItem->next;
        } // end of queue loop

        // Collect total bandwidth utilization for statistical purpose
        qospf->qospfStat.bandwidthUtilization[count] =
            MAX(qospf->qospfStat.bandwidthUtilization[count],
                totalUtilisedBw);

        if (isFloodingNecessary)
        {
            // Collect last advertised bandwidth
            ospf->iface[count].lastAdvtUtilizedBandwidth =
                totalUtilisedBw;
        }

        if (QOSPF_DEBUG_OUTPUT_FILE)
        {
            // Collects total bandwidth used for this interfaces
            sprintf(dataBuf + strlen(dataBuf), "%d\t",
                    (totalUtilisedBw / 1000));
        }

    } // end of interface index loop

    // Accumulates different statistics for a node to print it into
    // a data file. If QOSPF_DEBUG_OUTPUT_FILE is on, a datafile is opened
    // and stores the relevant information of the particular node.
    if (QOSPF_DEBUG_OUTPUT_FILE)
    {
        char fileName[MAX_STRING_LENGTH];
        FILE* fp;
        char dataBuf1[MAX_STRING_LENGTH];
        sprintf(fileName, "DataFile_%d", node->nodeId);
        fp = fopen(fileName, "a");
        if (fp)
        {
            sprintf(dataBuf1, "%d\t\t%d\t%d\t%s\n",
                    (int) (node->getNodeTime()/SECOND),
                    qospf->qospfStat.numActiveConnection,
                    qospf->qospfStat.numRejectedConnection,
                    dataBuf);
            if (fwrite(dataBuf1, strlen(dataBuf1), 1, fp)!= 1)
            {
               ERROR_ReportWarning("Unable to write in file \n");
            }
            fclose(fp);
            dataBuf[0] = '\0';
        }
    }

    if (QOSPF_DEBUG)
    {
        // Prints different queue status for every interface of a nodes
        QospfPrintQueueStatus(node);
    }

    return isFloodingNecessary;
}


//
// FUNCTION: QospfOriginateLSA
// PURPOSE:  If required, floods Router LSA to update other nodes
//           for current condition of available bandwidth and delay
//           of different queues of this node.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              BOOL isPeriodicUpdate
//                      Checks if function is called for periodic update
//                      or not. If it periodic update node must flood LSA.
// RETURN: None.
//
void QospfOriginateLSA(Node* node, QospfData* qospf, BOOL isPeriodicUpdate)
{
    BOOL isFloodingNecessary = FALSE;
    char traceInfo[MAX_STRING_LENGTH];

    if (isPeriodicUpdate)
    {
        if (qospf->trace)
        {
            sprintf(traceInfo,
                "Periodic flooding by originating router LSA");
            QospfTrace(node, qospf, traceInfo);
        }

        qospf->qospfStat.numPeriodicUpdate++;

        // Periodic update message. So originate LSA
        Ospfv2OriginateRouterLSAForThisArea(node,
                OSPFv2_BACKBONE_AREA, FALSE);
    }
    else
    {
        // Update different Queue informations before originating Router
        // LSA, as most recent information is required to advertise.
        isFloodingNecessary = QospfUpdateQueueInformation(node);

        if (qospf->trace)
        {
            sprintf(traceInfo,
                "Monitoring interface status %s",
                (isFloodingNecessary ?
                    " and Router LSA origination required":" "));
            QospfTrace(node, qospf, traceInfo);
        }

        // Originate router LSA if significant change occurred
        // in any interface.
        if (isFloodingNecessary)
        {
            qospf->qospfStat.numTriggeredUpdate++;
            Ospfv2OriginateRouterLSAForThisArea(node,
                OSPFv2_BACKBONE_AREA, FALSE);
        }
    }
}


//--------------------------------------------------------------------------
//                  Functions to free different Lists
//--------------------------------------------------------------------------

//
// FUNCTION: QospfFreeTopologyLinksList
// PURPOSE:  Free the topology Link List
// PARAMETERS:  Node* node
//                      Pointer to the node.
//              LinkedList* list
//                      Pointer to the List to be freed
// RETURN: NONE
//
static
void QospfFreeTopologyLinksList(Node* node, LinkedList* list)
{
    ListItem* tempList;
    ListItem* tempListItem;

    QospfLinkedFrom* getLinkedFrom;
    QospfLinkedTo*   getLinkedTo;

    // Retrieve Topology Links List
    ERROR_Assert(list, "Topology Links List not found");

    tempList = list->first;
    while (tempList)
    {
        getLinkedFrom = (QospfLinkedFrom*) tempList->data;

        // Get each QospfLinkedTo structure
        ERROR_Assert(getLinkedFrom->linkToInfoList,
                "QospfLinkedTo structure List not found");

        tempListItem = getLinkedFrom->linkToInfoList->first;
        while (tempListItem)
        {
            getLinkedTo = (QospfLinkedTo*) tempListItem->data;

            // Free array element present in QospfLinkedTo structure
            MEM_free(getLinkedTo->queueSpecificCost);

            tempListItem = tempListItem->next;
        }

        // Free the QospfLinkedTo structure
        ListFree(node, getLinkedFrom->linkToInfoList, FALSE);

        tempList = tempList->next;
    }

    // Free the base list
    ListFree(node, list, FALSE);
}


//
// FUNCTION: QospfFreeVertexSet
// PURPOSE:  Free the vertex set of this node
// PARAMETERS:  Node* node
//                      Pointer to the node.
//              LinkedList* list
//                      Pointer to the List to be freed
// RETURN: NONE
//
static
void QospfFreeVertexSet(Node* node, LinkedList* list)
{
    ListItem* tempItemVertexSet;
    ListItem* tempItemInterfaceInfo;

    QospfSetOfVertices* getVertexSet;
    QospfInterfaceInfo* getInterfaceInfo;

    // Retrieve vertex set list
    ERROR_Assert(list, "VertexSet List not found");

    tempItemVertexSet = list->first;
    while (tempItemVertexSet)
    {
        getVertexSet = (QospfSetOfVertices*) tempItemVertexSet->data;

        // Retrieve the vertex's interface info list
        ERROR_Assert(getVertexSet->vertexInterfaceInfo,
            "Vertex's interface info list not found");

        tempItemInterfaceInfo = getVertexSet->vertexInterfaceInfo->first;
        while (tempItemInterfaceInfo)
        {
            getInterfaceInfo = (QospfInterfaceInfo*)
                    tempItemInterfaceInfo->data;

            // Free the member list which contain NodeAddress type element
            ListFree(node,
                     getInterfaceInfo->connectedInterfaceIpAddr,
                     FALSE);

            tempItemInterfaceInfo = tempItemInterfaceInfo->next;
        }

        // Free Interface info list of this vertex
        ListFree(node, getVertexSet->vertexInterfaceInfo, FALSE);

        tempItemVertexSet = tempItemVertexSet->next;
    }

    // Free the base list contains Vertex Set
    ListFree(node, list, FALSE);
}


//
// FUNCTION: QospfFreeDescriptorStructureList
// PURPOSE:  Free the Descriptor Structure
// PARAMETERS:  Node* node
//                      Pointer to the node.
//              LinkedList* list
//                      Pointer to the List to be freed
// RETURN: NONE
//
static
void QospfFreeDescriptorStructureList(Node* node, LinkedList* list)
{
    ListItem* tempDescriptorList;
    ListItem* tempLinkDescriptor;

    QospfDescriptorStructure* getDescriptorStructure;
    QospfLinkDescriptor*      getLinkDescriptor;

    ERROR_Assert(list, "Descriptor List not found");

    tempDescriptorList = list->first;

    while (tempDescriptorList)
    {
        getDescriptorStructure = (QospfDescriptorStructure*)
                tempDescriptorList->data;

        // Free the list of neighbor vertex
        ListFree(node, getDescriptorStructure->neighborVertex, FALSE);

        // Get the link descriptors of this vertex
        ERROR_Assert(getDescriptorStructure->linkDescriptorsFromThisNode,
            "Link descriptors list not found");

        tempLinkDescriptor =
            getDescriptorStructure->linkDescriptorsFromThisNode->first;
        while (tempLinkDescriptor)
        {
            getLinkDescriptor =
                    (QospfLinkDescriptor*) tempLinkDescriptor->data;

            // Free the list containing Qos Metric of different queue
            ListFree(node,
                     getLinkDescriptor->queueSpecificLinkDescriptors,
                     FALSE);

            tempLinkDescriptor = tempLinkDescriptor->next;
        }

        // Free the link descriptors of this vertex
        ListFree(node,
                 getDescriptorStructure->linkDescriptorsFromThisNode,
                 FALSE);

        tempDescriptorList = tempDescriptorList->next;
    }

    // Free the base list of descriptor structure
    ListFree(node, list, FALSE);
}


//
// FUNCTION: QospfReleasePathList
// PURPOSE:  Free path list maintained by Q-ospf.
// PARAMETERS:  Node* node
//                      Pointer to the node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
// RETURN: NONE
//
static
void QospfReleasePathList(Node* node, QospfData* qospf)
{
    ListItem* tempListItem;
    QospfDifferentSessionInformation* getPath;

    tempListItem = qospf->pathList->first;
    while (tempListItem)
    {
        getPath =  (QospfDifferentSessionInformation*)
                tempListItem->data;

        if (getPath->numRouteAddresses)
        {
            // Free total path collected in a array
            MEM_free(getPath->routeAddresses);
        }

        tempListItem = tempListItem->next;
    }

    // Free path list
    ListFree(node, qospf->pathList, FALSE);
}


//--------------------------------------------------------------------------
//                Different functions for path calculation
//--------------------------------------------------------------------------

//
// FUNCTION: QospfExtractRouteFromDescriptorSet
// PURPOSE:  Checks whether the extended descriptor satisfies the Qos
//           constraints for a connection request. At the same time it
//           extracts path from the descriptor set
// PARAMETERS:  Node* node
//                      Pointer to the node
//              LinkedList* descriptorSet
//                      Pointer to the descriptor list created during path
//                      computation in the source node.
//              int sourceVertexId
//                      Source vertex number.
//              int destinationVertexId
//                      Destination vertex number.
//              NodeAddress** routeAddresses
//                      Pointer to that location where the total
//                      path will be stored.
//              int* numRouteAddresses
//                      Number of address stored.
// RETURN: BOOL
//
static
BOOL QospfExtractRouteFromDescriptorSet(
    Node* node,
    LinkedList* descriptorSet,
    int sourceVertexId,
    int destinationVertexId,
    NodeAddress** routeAddresses,
    int* numRouteAddresses)
{
    ListItem* tempListItem;
    QospfDescriptorStructure* getDescriptor;
    int tempVertex;
    BOOL isDestFoundFirst = FALSE;

    LinkedList* tempAddrList;
    NodeAddress* AddrList;
    NodeAddress* tempAddr;
    ListItem* item;

    ListInit(node, &tempAddrList);

    // Starting from the destination vertex id, search the entire
    // descriptor list until the source vertex id found.
    tempVertex = destinationVertexId;
    while (tempVertex != sourceVertexId)
    {
        ERROR_Assert(descriptorSet, "Descriptor set not found");
        tempListItem = descriptorSet->first;
        while (tempListItem)
        {
            getDescriptor = (QospfDescriptorStructure*)
                    tempListItem->data;

            // find the destination vertex first
            if (getDescriptor->fromVertexId == tempVertex)
            {
                tempAddr = (NodeAddress*) MEM_malloc(sizeof(NodeAddress));

                *tempAddr = getDescriptor->nodeDpValue.nextHop;
                ListInsert(node, tempAddrList, 0, (void*)tempAddr);

                tempVertex = getDescriptor->nodeDpValue.previousVertexId;
                isDestFoundFirst = TRUE;
                break;
            }

            if (((getDescriptor->fromVertexId == tempVertex)
                 && (getDescriptor->fromVertexId != sourceVertexId))
                    && (isDestFoundFirst))
            {
                tempAddr = (NodeAddress*) MEM_malloc(sizeof(NodeAddress));

                *tempAddr = getDescriptor->nodeDpValue.nextHop;
                ListInsert(node, tempAddrList, 0, (void*)tempAddr);

                tempVertex = getDescriptor->nodeDpValue.previousVertexId;
                break;
            }
            tempListItem = tempListItem->next;
        }
    }

    if (QOSPF_DEBUG)
    {
        printf("\nADDRESS LIST......\n");
        item = tempAddrList->last;
        while (item)
        {
            tempAddr = (NodeAddress*) item->data;
            printf("\t%u", *tempAddr);
            item = item->prev;
        }
        printf("\n\n");
    }

    // Insert the total path
    AddrList = (NodeAddress*) MEM_malloc(
            sizeof(NodeAddress) * tempAddrList->size);

    *numRouteAddresses = tempAddrList->size;
    *routeAddresses = AddrList;

    item = tempAddrList->last;
    while (item)
    {
        tempAddr = (NodeAddress*) item->data;

        // Next hop not found during database creation
        if ((*tempAddr) == (unsigned) QOSPF_NEXT_HOP_UNREACHABLE)
        {
            return TRUE;
        }

        *AddrList = *tempAddr;
        AddrList++;
        item = item->prev;
    }

    // Free the temporary Address list
    ListFree(node, tempAddrList, FALSE);
    return FALSE;
}


//
// FUNCTION: QospfConvertIpAddressIntoVertexId
// PURPOSE:  Converts an IP address of an interface into it's vertex
//           number as stored in vertex set at Q-OSPF Layer.
// PARAMETERS:  QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              NodeAddress ipAddress
//                      IP address.
// RETURN: int
//
static
int QospfConvertIpAddressIntoVertexId(
    QospfData* qospf,
    NodeAddress ipAddress)
{
    ListItem* tempListItemForVertex;
    ListItem* tempListItemForInterfaceAddr;
    QospfSetOfVertices* getVertex;
    QospfInterfaceInfo* getInterfaceAddr;

    ERROR_Assert(qospf->vertexSet, "No vertex set found");

    tempListItemForVertex = qospf->vertexSet->first;
    while (tempListItemForVertex)
    {
        getVertex = (QospfSetOfVertices*) tempListItemForVertex->data;

        // Check  the interfaces of this vertex. If the address
        // match with any of the source or destination address,
        // then collect the vertex number.
        tempListItemForInterfaceAddr =
            getVertex->vertexInterfaceInfo->first;

        while (tempListItemForInterfaceAddr)
        {
            getInterfaceAddr = (QospfInterfaceInfo*)
                    tempListItemForInterfaceAddr->data;

            // Check whether the is source address matched with the
            // interface IP address.
            if (ipAddress == getInterfaceAddr->ownInterfaceIpAddr)
            {
                return getVertex->vertexNumber;
            }
            tempListItemForInterfaceAddr =
                tempListItemForInterfaceAddr->next;
        }
        tempListItemForVertex = tempListItemForVertex->next;
    }

    // No vertex match, return 0
    return 0;
}


//
// FUNCTION: QospfIsLinkRedundant
// PURPOSE:  Collects the available QoS metrics for vertex from topology
//           links list and insert it in link descriptor structure.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              QospfDescriptorStructure* tempDescriptorStruct
//                      Pointer to the descriptor structure for that vertex
//                      whose link descriptors are checking
//              int vertexNumber
//                      Vertex number of that node descriptor whose
//                      link descriptors are checking.
//              QospfLinkedTo* getLinkedTo
//                      Link information as collected in Topology link list
//              QospfQosConstraint constraint
//                      Requested QoS Constraint.
// RETURN: NONE
//
static
BOOL QospfIsLinkRedundant(
    Node* node,
    QospfData* qospf,
    QospfDescriptorStructure* tempDescriptorStruct,
    int vertexnumber,
    QospfLinkedTo* getLinkedTo,
    QospfQosConstraint constraint)
{
    BOOL isAnotherLinkPresent = FALSE;
    BOOL isThisLinkBetter = FALSE;

    ListItem* linkDescriptorItem;
    QospfLinkDescriptor* getLinkDescriptor = NULL;
    ListItem* queueListItem;
    QospfQSpecificDescriptor* getQDescriptor;


    // Check if multiple link present between two nodes.
    linkDescriptorItem =
           tempDescriptorStruct->linkDescriptorsFromThisNode->first;

    while (linkDescriptorItem)
    {
        getLinkDescriptor = (QospfLinkDescriptor*) linkDescriptorItem->data;

        if (getLinkDescriptor->toVertexId == vertexnumber)
        {
            isAnotherLinkPresent = TRUE;
            break;
        }

        linkDescriptorItem = linkDescriptorItem->next;
    }

    // This is the only link. So it not a redundant Link
    if (!isAnotherLinkPresent)
    {
        return FALSE;
    }

    // So more than one link present. Choose the better Link.
    queueListItem = getLinkDescriptor->queueSpecificLinkDescriptors->first;
    while (queueListItem)
    {
        getQDescriptor =
                (QospfQSpecificDescriptor*) queueListItem->data;

        if (getQDescriptor->queueNumber == constraint.queueNumber)
        {
            if ((getLinkedTo->queueSpecificCost[constraint.queueNumber].
                    availableBandwidth >= constraint.availableBandwidth) &&
                (getLinkedTo->queueSpecificCost[constraint.queueNumber].
                    averageDelay < getQDescriptor->metricValue.delay))
            {
                isThisLinkBetter = TRUE;
                break;
            }
        }
        queueListItem = queueListItem->next;
    }

    if (isThisLinkBetter)
    {
        // This is a better one. So remove pervious link
        ListFree(node,
                 getLinkDescriptor->queueSpecificLinkDescriptors,
                 FALSE);
        ListGet(node,
                tempDescriptorStruct->linkDescriptorsFromThisNode,
                linkDescriptorItem,
                TRUE,
                FALSE);


        return FALSE;
    }

    // this is not a better link than previous one. So this link
    // is redundant link.
    return TRUE;
}


//
// FUNCTION: QospfInitialiseLinkDescriptorForThisVertex
// PURPOSE:  Collects the available QoS metrics for vertex from topology
//           links list and insert it in link descriptor structure.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              QospfDescriptorStructure* tempDescriptorStruct
//                      Pointer to the descriptor structure for that vertex
//                      whose link descriptor are collecting
//              int vertexNumber
//                      Vertex number of that descriptor whose
//                      link descriptorare collecting.
//              QospfQosConstraint constraint
//                      Requested QoS Constraint.
// RETURN: NONE
//
static
void QospfInitialiseLinkDescriptorForThisVertex(
    Node* node,
    QospfData* qospf,
    QospfDescriptorStructure* tempDescriptorStruct,
    int vertexNumber,
    QospfQosConstraint qosConstraint)
{
    ListItem* tempItemLinkedFrom;
    ListItem* tempItemLinkedTo;

    QospfLinkedFrom* getLinkedFrom;
    QospfLinkedTo*   getLinkedTo;

    int i;
    QospfLinkDescriptor* tempLinkDescriptor;
    QospfQSpecificDescriptor* tempLinkDes;

    // Initialize link descriptor's for this node
    ListInit(node, &tempDescriptorStruct->linkDescriptorsFromThisNode);

    // Find proper from_router_Id which matches with
    // the requested vertexNumber.
    tempItemLinkedFrom = qospf->topologyLinksList->first;
    while (tempItemLinkedFrom)
    {
        getLinkedFrom = (QospfLinkedFrom*) tempItemLinkedFrom->data;

        if (getLinkedFrom->fromVertexNumber == vertexNumber)
        {
            tempItemLinkedTo = getLinkedFrom->linkToInfoList->first;

            while (tempItemLinkedTo)
            {
                getLinkedTo = (QospfLinkedTo*)
                        tempItemLinkedTo->data;
                if (!QospfIsLinkRedundant(
                        node,
                        qospf,
                        tempDescriptorStruct,
                        getLinkedTo->toVertexNumber,
                        getLinkedTo,
                        qosConstraint))
                {

                    // Allocate and insert this link's descriptor
                    tempLinkDescriptor = (QospfLinkDescriptor*)
                        MEM_malloc(sizeof(QospfLinkDescriptor));

                    tempLinkDescriptor->toVertexId =
                        getLinkedTo->toVertexNumber;

                    tempLinkDescriptor->interfaceIndex =
                        getLinkedTo->interfaceIndex;

                    tempLinkDescriptor->interfaceIpAddr =
                        getLinkedTo->neighborIpAddr;

                    // Initialize and collects different queue information
                    // for this link descriptor.
                    ListInit(node,
                             &tempLinkDescriptor->
                                 queueSpecificLinkDescriptors);

                    for (i = 0;
                         i < getLinkedTo->numQueues;
                         i++)
                    {
                        tempLinkDes = (QospfQSpecificDescriptor*)
                            MEM_malloc(
                                sizeof(QospfQSpecificDescriptor));

                        tempLinkDes->queueNumber = getLinkedTo->
                                queueSpecificCost[i].queueNumber;

                        tempLinkDes->metricValue.bandwidth =
                            getLinkedTo->queueSpecificCost[i].
                                    availableBandwidth;

                        tempLinkDes->metricValue.delay = getLinkedTo->
                                queueSpecificCost[i].averageDelay;

                        ListInsert(
                            node,
                            tempLinkDescriptor->
                                queueSpecificLinkDescriptors,
                            0,
                            (void*) tempLinkDes);
                    }

                    // Insert the item into link descriptor list
                    ListInsert(node,
                               tempDescriptorStruct->
                                    linkDescriptorsFromThisNode,
                               0,
                               (void*) tempLinkDescriptor);
                }

                // Allocation over. So find next link
                tempItemLinkedTo = tempItemLinkedTo->next;
            }

            break;
        }

        tempItemLinkedFrom = tempItemLinkedFrom->next;
    }
}


//
// FUNCTION: QospfGetNeighborVertex
// PURPOSE:  Collects the neighbor vertex exists for this vertex.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              QospfDescriptorStructure* tempDescriptorStruct
//                      Pointer to descriptor structure for that vertex
//                      whose link descriptor are collecting
//              int vertexNumber
//                      Vertex number of that descriptor whose
//                      link descriptor are collecting.
// RETURN: NONE
//
static
void QospfGetNeighborVertex(
    Node* node,
    QospfData* qospf,
    QospfDescriptorStructure* descripStruct,
    int vertexNumber)
{
    ListItem* tempListItem;
    ListItem* tempListForInterface;
    ListItem* tempListForNeighbor;

    QospfSetOfVertices* getVertex;
    QospfInterfaceInfo* getInterfaceInfo;
    NodeAddress* getNeighbor;

    int neighVertexId = 0;
    int* tempNeighVrtx;

    ListInit(node, &descripStruct->neighborVertex);

    ERROR_Assert(qospf->vertexSet, "No vertex set found");

    tempListItem = qospf->vertexSet->first;

    // Search the entire list for the presence of Vertex
    while (tempListItem)
    {
        getVertex = (QospfSetOfVertices*) tempListItem->data;

        ERROR_Assert(getVertex, "No vertex found");

        if (getVertex->vertexNumber == vertexNumber)
        {
            // Vertex found. Now map the neighbors of each interface of
            // this vertex into its vertex number.
            tempListForInterface = getVertex->vertexInterfaceInfo->first;

            while (tempListForInterface)
            {
                getInterfaceInfo = (QospfInterfaceInfo*)
                        tempListForInterface->data;

                // Find the neighbor for this interface.
                tempListForNeighbor =
                    getInterfaceInfo->connectedInterfaceIpAddr->first;
                while (tempListForNeighbor)
                {
                    getNeighbor = (NodeAddress*) tempListForNeighbor->data;

                    neighVertexId = QospfConvertIpAddressIntoVertexId(
                                qospf, *getNeighbor);

                    if (neighVertexId)
                    {
                        // Neighbor matched. So consider this vertex
                        // as a neighbor vertex;

                        tempNeighVrtx = (int*) MEM_malloc
                                (sizeof(int));

                        *tempNeighVrtx = neighVertexId;

                        ListInsert(node,
                                   descripStruct->neighborVertex,
                                   0,
                                   (void*) tempNeighVrtx);
                    }

                    tempListForNeighbor = tempListForNeighbor->next;
                }// end of neighbor finding

                tempListForInterface = tempListForInterface->next;
            }// end of interface finding loop
            break;
        }
        tempListItem = tempListItem->next;
    }// end of vertex finding loop
}


//
// FUNCTION: QospfPrintDescriptorSet
// PURPOSE:  Prints the contents of the descriptor set.
// PARAMETERS:  LinkedList* descriptorSet
//                      Pointer to the descriptor list created during path
//                      computation in the source node.
// RETURN: NONE
//
static
void QospfPrintDescriptorSet(LinkedList* descriptorSet)
{
    ListItem* tempDescriptorItem;
    ListItem* tempLinkDescriptorItem;
    ListItem* tempNeighborItem;
    ListItem* tempQueueSpecificLink;

    QospfDescriptorStructure* getDescriptor;
    QospfLinkDescriptor* getLinkDescriptor;
    QospfQSpecificDescriptor* getQueueSpecificLink;
    NodeAddress* getNeighbor;

    ERROR_Assert(descriptorSet, "Descriptor set not found");
    tempDescriptorItem = descriptorSet->first;
    while (tempDescriptorItem)
    {
        getDescriptor = (QospfDescriptorStructure*)
                tempDescriptorItem->data;

        printf("\n\nThe Descriptor Item For Vertex %d\n",
            getDescriptor->fromVertexId);
        printf("---------------------------------\n");
        printf("\nVertex's OSPF router address 0x%x",
            getDescriptor->vertexAddress);

        // node descriptor
        printf("\n\nNode Descriptor :");
        printf("\n\tBandwidth = %d",
            getDescriptor->nodeDescriptor.bandwidth);
        printf("\n\tDelay = %d", getDescriptor->nodeDescriptor.delay);

        //Dp value
        printf("\n\nDp Value :");
        printf("\n\tPrev vertex = %d",
            getDescriptor->nodeDpValue.previousVertexId);

        printf("\n\tPrev outgoing interface index = %d",
            getDescriptor->nodeDpValue.previousVertexInterfaceId);

        printf("\n\tNext hop = 0x%x", getDescriptor->nodeDpValue.nextHop);

        // neighbor vertex
        printf("\n\nNeighbor Vertex of this vertex :");

        tempNeighborItem = getDescriptor->neighborVertex->first;

        while (tempNeighborItem)
        {
            getNeighbor = (NodeAddress*) tempNeighborItem->data;
            printf("\n\tNeighbor vertex = %d", *getNeighbor);
            tempNeighborItem = tempNeighborItem->next;
        }

        // Link descriptor
        printf("\n\nLink descriptors :");
        printf("\nLink extended from vertex %d",
            getDescriptor->fromVertexId);
        tempLinkDescriptorItem =
                getDescriptor->linkDescriptorsFromThisNode->first;

        while (tempLinkDescriptorItem)
        {
            getLinkDescriptor = (QospfLinkDescriptor*)
                tempLinkDescriptorItem->data;

            printf("\n\tto vertex %d", getLinkDescriptor->toVertexId);
            printf("\n\tInterface index of this vertex %d",
                getLinkDescriptor->interfaceIndex);

            printf("\n\tInterface IP addr. of neighbor vertex is 0x%x",
                getLinkDescriptor->interfaceIpAddr);

            tempQueueSpecificLink =
                getLinkDescriptor->queueSpecificLinkDescriptors->first;

            while (tempQueueSpecificLink)
            {
                getQueueSpecificLink = (QospfQSpecificDescriptor*)
                    tempQueueSpecificLink->data;

                printf("\n\t\tQueue number %d",
                    getQueueSpecificLink->queueNumber);

                printf("\n\t\t  Available Bandwidth %d",
                    getQueueSpecificLink->metricValue.bandwidth);

                printf("\n\t\t  Average Delay %d",
                    getQueueSpecificLink->metricValue.delay);

                tempQueueSpecificLink = tempQueueSpecificLink->next;
            }

            tempLinkDescriptorItem = tempLinkDescriptorItem->next;
        }

        printf("\n");
        tempDescriptorItem = tempDescriptorItem->next;
    }
}


//
// FUNCTION: QospfCreateDescriptorSet
// PURPOSE:  Create the descriptor set during path computation at source
//           node. After path computation, this set will not exists.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              LinkedList* descriptorSet
//                      Pointer to the descriptor list created during path
//                      computation in the source node.
//              QospfQosConstraint constraint
//                      Requested QoS Constraint.
// RETURN: NONE
//
static
void QospfCreateDescriptorSet(
    Node* node,
    QospfData* qospf,
    LinkedList* descriptorSet,
    QospfQosConstraint qosConstraint)
{
    ListItem* tempListItem;
    QospfSetOfVertices* getVertex;
    QospfDescriptorStructure* tempDescriptorStruct;

    ERROR_Assert(qospf->vertexSet, "No vertex set found");

    tempListItem = qospf->vertexSet->first;

    // Search the entire list for the presence of Vertex
    while (tempListItem)
    {
        getVertex = (QospfSetOfVertices*) tempListItem->data;

        ERROR_Assert(getVertex, "No vertex found");

        // Consider only point-to-point networks
        if (getVertex->vertexType == QOSPF_ROUTER)
        {
            // Vertex found. So create space to collects the data
            tempDescriptorStruct = (QospfDescriptorStructure*)
                    MEM_malloc(sizeof(QospfDescriptorStructure));

            tempDescriptorStruct->fromVertexId = getVertex->vertexNumber;
            tempDescriptorStruct->vertexAddress = getVertex->vertexAddress;

            // Initial value of bandwidth variable is maximum and delay
            // variable is zero. As during concatenation these value will
            // give the expected results.
            tempDescriptorStruct->nodeDescriptor.bandwidth =
                QOSPF_MAX_BANDWIDTH;

            tempDescriptorStruct->nodeDescriptor.delay =
                QOSPF_MIN_DELAY;

            // Put initial value to diff. fields of QospfDpStructure
            tempDescriptorStruct->nodeDpValue.previousVertexId = -1;
            tempDescriptorStruct->
                    nodeDpValue.previousVertexInterfaceId = -1;
            tempDescriptorStruct->nodeDpValue.nextHop =
                (NodeAddress) QOSPF_NEXT_HOP_UNREACHABLE;

            // Initialize the different link descriptors originates
            // from this vertex.
            QospfInitialiseLinkDescriptorForThisVertex(
                node,
                qospf,
                tempDescriptorStruct,
                getVertex->vertexNumber,
                qosConstraint);

            // Collects the neighbor vertex present for this vertex
            QospfGetNeighborVertex(
                node,
                qospf,
                tempDescriptorStruct,
                getVertex->vertexNumber);

            ListInsert(
                node,
                descriptorSet,
                0,
                (void*)tempDescriptorStruct);
        }

        tempListItem = tempListItem->next;
    }

    if (QOSPF_DEBUG)
    {
        QospfPrintDescriptorSet(descriptorSet);
    }
}


//
// FUNCTION: QospfIsDescriptorSatisfyQosConstraint
// PURPOSE:  Checks whether the extended descriptor satisfies the Qos
//           constraints for a data request.
// PARAMETERS:  int descriptor
//                      Descriptor number which is going to test
//              LinkedList* descriptorSet
//                      Pointer to the descriptor list created during path
//                      computation in the source node.
//              QospfQosConstraint constraint
//                      Value of QoS metrics requested by a data traffic.
// RETURN: BOOL
//
static
BOOL QospfIsDescriptorSatisfyQosConstraint(
    int descriptor,
    LinkedList* descriptorSet,
    QospfQosConstraint constraint)
{
    QospfDescriptorStructure* getDescriptor;
    ListItem* tempListItem;

    ERROR_Assert(descriptorSet, "Descriptor set not found");

    tempListItem = descriptorSet->first;
    while (tempListItem)
    {
        getDescriptor = (QospfDescriptorStructure*)
                tempListItem->data;

        // Find the proper descriptor from descriptor set
        if (getDescriptor->fromVertexId == descriptor)
        {
            // Check whether the descriptor satisfy the request
            if ((getDescriptor->nodeDescriptor.bandwidth >=
                    constraint.availableBandwidth)
                && (getDescriptor->nodeDescriptor.delay <=
                        constraint.averageDelay))
            {
                return TRUE;
            }
        }
        tempListItem = tempListItem->next;
    }
    return FALSE;
}


//
// FUNCTION: QospfIsDescriptorPresentInDescriptorList
// PURPOSE:  Finds whether a descriptor present in a descriptor list.
// PARAMETERS:  int descriptor
//                      Descriptor to find
//              LinkedList* list
//                      List of that type of descriptor that will be found
// RETURN: TRUE/FALSE
//
static
BOOL QospfIsDescriptorPresentInDescriptorList(int  descriptor, LinkedList* list)
{
    ListItem* tempListItem;
    int* getData;

    // Search the entire descriptor list
    ERROR_Assert(list, "Corrupted descriptor List");
    tempListItem = list->first;

    while (tempListItem)
    {
        getData = (int*) tempListItem->data;

        // descriptor found, so return TRUE
        if (*getData == descriptor)
        {
            return TRUE;
        }
        tempListItem = tempListItem->next;
    }
    return FALSE;
}


//
// FUNCTION: QospfEmptyDescriptorList
// PURPOSE:  To empty the different descriptor list.
// PARAMETERS:  Node* node
//                      pointer to the node.
//              LinkedList* list
//                      List of that type of descriptor that will be found
// RETURN: NONE
//
static
void QospfEmptyDescriptorList(Node* node, LinkedList* list)
{
    ListItem* listItem;

    if (list->size)
    {
        // Collects the first element and delete it
        ERROR_Assert(list, "Corrupted descriptor List");
        listItem = list->first;

        while (listItem)
        {
            ListGet(node, list, listItem, TRUE, FALSE);
            listItem = list->first;
        }
    }
}


//
// FUNCTION: QospfConcatenateDescriptors
// PURPOSE:  Concatenate the metrics value of two descriptors
// PARAMETERS:  QospfQosMetric* descriptor
//                      Metric value of the descriptor from where
//                      concatenation will be started.
//              QospfQosMetric* descriptorToBeModified
//                      Metric value of the descriptor upto which
//                      concatenation will be done.
// RETURN: NONE
//
static
void QospfConcatenateDescriptors(
    QospfQosMetric* descriptor,
    QospfQosMetric* descriptorToBeModified)
{
    // concatenate bandwidth
    if (descriptorToBeModified->bandwidth < descriptor->bandwidth)
    {
        descriptorToBeModified->bandwidth =
            descriptorToBeModified->bandwidth;
    }
    else
    {
        descriptorToBeModified->bandwidth = descriptor->bandwidth;
    }

    // concatenate delay
    descriptorToBeModified->delay = descriptorToBeModified->delay +
                                    descriptor->delay;
}


//
// FUNCTION: QospfFindLinkDescriptorFromDescriptorSet
// PURPOSE:  Finds link descriptors for a vertex.
// PARAMETERS:  int sourceVertexId,
//                      Source vertex
//              LinkedList* descriptorSet
//                      Pointer to the descriptor set created for path
//                      calculation.
//              int queueNumber
//                      Number of the queue whose Qos metric will be
//                      considered for path calculation.
//              int** elementSet
//                      Pointer to that location where the descriptors
//                      will be stored for further processing.
//              int* numElement
//                      Number of element found.
// RETURN: NONE
//
static
void QospfFindLinkDescriptorFromDescriptorSet(
    int   sourceVertexId,
    LinkedList* descriptorSet,
    QospfQosConstraint* constraint,
    int** elementSet,
    int*  numElement)
{
    ListItem* tempItem;
    ListItem* tempListItem;
    ListItem* tempItemToModify;
    ListItem* tempItemQueueInfo;

    QospfQSpecificDescriptor* getQueue;
    QospfLinkDescriptor*      getLinkDescriptor;
    QospfDescriptorStructure* getNodeDescriptor;
    QospfDescriptorStructure* getNodeDescriptorToModify;

    int* tempElementSet;
    int queueToSelect;

    ERROR_Assert(descriptorSet, "Descriptor set not found");
    tempListItem = descriptorSet->first;

    while (tempListItem)
    {
        getNodeDescriptor = (QospfDescriptorStructure*)
                             tempListItem->data;

        if (getNodeDescriptor->fromVertexId == sourceVertexId)
        {
            *numElement =
                getNodeDescriptor->linkDescriptorsFromThisNode->size;

            if (*numElement > 0)
            {
                tempElementSet = (int*) MEM_malloc(sizeof(int)
                    * getNodeDescriptor->linkDescriptorsFromThisNode->size);
            }
            else
            {
                tempElementSet = NULL;
            }

            *elementSet = tempElementSet;

            tempItem =
                getNodeDescriptor->linkDescriptorsFromThisNode->first;

            while (tempItem)
            {
                getLinkDescriptor = (QospfLinkDescriptor*)
                        tempItem->data;

                *tempElementSet = getLinkDescriptor->toVertexId;

                // Modify node descriptor by concatenating the link
                // descriptors of source node to proper node descriptor.

                // Find the proper queue
                tempItemQueueInfo =
                    getLinkDescriptor->queueSpecificLinkDescriptors->first;

                // Find the queue through which a packet with this
                // priority will forwarded.
                // Size of queueSpecificLinkDescriptors gives number of
                // queues present in this interface.
                queueToSelect = GetQueueNumberFromPriority(
                                    constraint->priority,
                                    getLinkDescriptor->
                                        queueSpecificLinkDescriptors->size); // Num Q


                while (tempItemQueueInfo)
                {
                    getQueue = (QospfQSpecificDescriptor*)
                            tempItemQueueInfo->data;

                    if (getQueue->queueNumber == queueToSelect)
                    {
                        // Find node to which descriptor will be modified.
                        ERROR_Assert(descriptorSet,
                            "Descriptor set not found");
                        tempItemToModify = descriptorSet->first;

                        while (tempItemToModify)
                        {
                            getNodeDescriptorToModify =
                                (QospfDescriptorStructure*)
                                            tempItemToModify->data;

                            // Concatenate the descriptors
                            if (getNodeDescriptorToModify->fromVertexId ==
                                getLinkDescriptor->toVertexId)
                            {
                                // Modify node descriptor
                                QospfConcatenateDescriptors(
                                   &getQueue->metricValue,
                                   &getNodeDescriptorToModify->
                                            nodeDescriptor);

                                // Modify Dp structure
                                getNodeDescriptorToModify->nodeDpValue.
                                    previousVertexId =
                                        getNodeDescriptor->fromVertexId;

                                getNodeDescriptorToModify->nodeDpValue.
                                    previousVertexInterfaceId =
                                        getLinkDescriptor->interfaceIndex;

                                getNodeDescriptorToModify->nodeDpValue.
                                    nextHop =
                                        getLinkDescriptor->interfaceIpAddr;
                                break;
                            }
                            tempItemToModify = tempItemToModify->next;
                        }
                        break;
                    }
                    tempItemQueueInfo = tempItemQueueInfo->next;
                } //end of concatenation

                // Increment pointers
                tempElementSet++;
                tempItem = tempItem->next;
            }
            break;
        }
        tempListItem = tempListItem->next;
    }
}


//
// FUNCTION: QospfInsertDescriptorIntoDescriptorList
// PURPOSE:  Insert the descriptor to it's descriptor list.
// PARAMETERS:  Node* node
//                      Pointer to the node.
//              LinkedList* tempList
//                      Pointer to the descriptor list.
//              int descriptor
//                      Descriptor to be inserted.
// RETURN: NONE
//
static
void QospfInsertDescriptorIntoDescriptorList(
    Node* node,
    LinkedList* tempList,
    int   descriptor)
{
    ListItem* tempListItem;
    int* getData;
    BOOL isDataAbsent = TRUE;

    int* tempDescriptor;

    // Search the list for the presence of the descriptor
    ERROR_Assert(tempList, "Corrupted descriptor List");

    tempListItem = tempList->first;
    while (tempListItem)
    {
        getData = (int*) tempListItem->data;

        if (descriptor == *getData)
        {
            isDataAbsent = FALSE;
            break;
        }

        tempListItem = tempListItem->next;
    }

    // Descriptor absent, include it
    if (isDataAbsent)
    {
        tempDescriptor = (int*) MEM_malloc(sizeof(int));
        *tempDescriptor = descriptor;

        ListInsert(node, tempList, 0, (void*) tempDescriptor);
    }
}


//
// FUNCTION: QospfPrintDescriptorList
// PURPOSE:  Print the content of the descriptor list.
// PARAMRTERS:  LinkedList* list.
//                      Pointer to the descriptor list.
// RETURN: NONE
//
static
void QospfPrintDescriptorList(LinkedList* list)
{
    ListItem* tempItem;
    int* listContent;

    ERROR_Assert(list, "Corrupted descriptor List");
    tempItem = list->first;
    printf("\nContents of the List..\n");
    while (tempItem)
    {
        listContent = (int*) tempItem->data;

        printf("\tDescriptor of Vertex %d\n", *listContent);
        tempItem = tempItem->next;
    }

    if (!list->size)
    {
        printf("\n\tList is empty.");
    }
    printf("\n\n");
}


//
// FUNCTION: QospfSearchDescriptorInDescriptorList
// PURPOSE:  Collects the descriptors present in descriptor list.
// PARAMETERS : LinkedList* list
//                      Pointer to the list whose descriptors will
//                      be collected.
//              int** descriptor
//                      Pointer to that location where the descriptors
//                      will be collected.
//              int*  numDescriptor
//                      Numbers of descriptors collected.
// RETURN: NONE
//
static
void QospfSearchDescriptorInDescriptorList(
    LinkedList* list,
    int** descriptor,
    int*  numDescriptor)
{
    ListItem* tempItem;
    int* tempElementSet;
    int* getData;

    // Allocate enough memory space for descriptors
    *numDescriptor = list->size;

    if (list->size > 0)
    {
        tempElementSet = (int*) MEM_malloc(
            sizeof (int) * list->size);
    }
    else
    {
        tempElementSet = NULL;
    }

    *descriptor = tempElementSet;

    // Get all the descriptors into allocated memory space
    ERROR_Assert(list, "Corrupted descriptor List");
    tempItem = list->first;
    while (tempItem)
    {
        getData = (int*)tempItem->data;

        *tempElementSet = *getData;

        tempElementSet++;
        tempItem = tempItem->next;
    }
}


//
// FUNCTION: QospfDeleteDescriptorFromDescriptorList
// PURPOSE:  Delete a descriptor from it's list.
// PARAMETERS:  Node* node
//                      Pointer to the node.
//              LinkedList* list
//                      Pointer the list whose descriptor
//                      will be deleted.
//              int descriptor
//                      Descriptor that will be deleted.
// RETURN: NONE
//
static
void QospfDeleteDescriptorFromDescriptorList(
    Node* node,
    LinkedList* list,
    int   descriptor)
{
    ListItem* tempItem;
    ListItem* tempListItem;
    int* getData;

    ERROR_Assert(list, "Corrupted descriptor List");
    tempItem = list->first;
    while (tempItem)
    {
        getData = (int*)tempItem->data;

        // particular descriptor found so delete it
        if (descriptor == *getData)
        {
            tempListItem = tempItem->next;
            ListGet(node, list, tempItem, TRUE, FALSE);
            tempItem = tempListItem;
            break;
        }
        else
        {
            tempItem = tempItem->next;
        }
    }
}


//
// FUNCTION: QospfReturnDpValue
// PURPOSE:  Returns the Dp value of a descriptor.
// PARAMETERS:  int descriptor
//                      Descriptor whose Dp value will be collected.
//              LinkedList* descriptorSet
//                      Pointer to the descriptor set.
// RETURN: int
//
static
int QospfReturnDpValue(int descriptor, LinkedList* descriptorSet)
{
    ListItem* tempListItem;
    QospfDescriptorStructure* getDescriptor;

    ERROR_Assert(descriptorSet, "Descriptor set not found");
    tempListItem = descriptorSet->first;
    while (tempListItem)
    {
        getDescriptor = (QospfDescriptorStructure*)
                        tempListItem->data;

        // Particular descriptor found from the descriptor set
        if (getDescriptor->fromVertexId == descriptor)
        {
            return (getDescriptor->nodeDpValue.previousVertexId);
        }
        tempListItem = tempListItem->next;
    }
    return (-1);
}


//
// FUNCTION: QospfExtendDescriptorFromProjectedVertexToNeighbor
// PURPOSE:  Extends the descriptor from Projected vertex it it's neighbor.
// PARAMETERS:  LinkedList* descriptorSet
//                      Pointer to the descriptor set.
//              int  descriptor
//                      Projected vertex.
//              int  neighborId
//                      Neighbor of the projected vertex.
//              int  advtQueue
//                      Queue to be considered.
//              LinkedList* projectedDescriptorList
//                      Pointer to projected descriptor list
// RETURN: NONE
//
static
void QospfExtendDescriptorFromProjectedVertexToNeighbor(
    LinkedList* descriptorSet,
    int   descriptor,
    int   neighborId,
    QospfQosConstraint* constraint,
    LinkedList* projectedDescriptorList)
{
    QospfQosMetric tempDescriptor;

    ListItem* tempListItemForDescriptor;
    ListItem* tempListItemForNeighbor;
    ListItem* tempListItemForQueue;

    QospfDescriptorStructure* getProjectedNode;
    QospfDescriptorStructure* getNeighborNode;
    QospfLinkDescriptor* getLinkDescriptor;
    QospfQSpecificDescriptor* getQueueInfo;
    int interfaceIndex = 0;
    int queueToSelect;
    NodeAddress nextHop = (unsigned) QOSPF_NEXT_HOP_UNREACHABLE;

    tempDescriptor.bandwidth = QOSPF_MAX_BANDWIDTH;
    tempDescriptor.delay = QOSPF_MIN_DELAY;

    // Concatenate the two descriptor and store the concatenated
    // value into a temporary local descriptor variable.
    ERROR_Assert(descriptorSet, "Descriptor set not found");
    tempListItemForDescriptor = descriptorSet->first;
    while (tempListItemForDescriptor)
    {
        getProjectedNode = (QospfDescriptorStructure*)
                            tempListItemForDescriptor->data;

        if (getProjectedNode->fromVertexId == descriptor)
        {
            QospfConcatenateDescriptors(
                &getProjectedNode->nodeDescriptor,
                &tempDescriptor);

            tempListItemForNeighbor =
                getProjectedNode->linkDescriptorsFromThisNode->first;
            while (tempListItemForNeighbor)
            {
                getLinkDescriptor = (QospfLinkDescriptor*)
                                     tempListItemForNeighbor->data;

                if (getLinkDescriptor->toVertexId == neighborId)
                {
                    interfaceIndex = getLinkDescriptor->interfaceIndex;
                    nextHop = getLinkDescriptor->interfaceIpAddr;

                    // find the queue for concatenate the metric value
                    tempListItemForQueue = getLinkDescriptor->
                            queueSpecificLinkDescriptors->first;

                    queueToSelect = GetQueueNumberFromPriority(
                                constraint->priority,
                                getLinkDescriptor->
                                    queueSpecificLinkDescriptors->size);

                    while (tempListItemForQueue)
                    {
                        getQueueInfo =
                            (QospfQSpecificDescriptor*)
                             tempListItemForQueue->data;

                        if (getQueueInfo->queueNumber == queueToSelect)
                        {
                            QospfConcatenateDescriptors(
                                &getQueueInfo->metricValue,
                                &tempDescriptor);

                            break;
                        }
                        tempListItemForQueue = tempListItemForQueue->next;
                    }
                    break;
                }
                tempListItemForNeighbor = tempListItemForNeighbor->next;
            }
            break;
        }
        tempListItemForDescriptor = tempListItemForDescriptor->next;
    }

    // Copy the content of the local temp descriptor value into neighbor
    // descriptor of projected descriptor in Descriptor Structure.
    tempListItemForNeighbor = descriptorSet->first;
    while (tempListItemForNeighbor)
    {
        getNeighborNode = (QospfDescriptorStructure*)
                tempListItemForNeighbor->data;

        if (getNeighborNode->fromVertexId == neighborId)
        {
            // If the descriptor (i.e. the descriptor of neighborId) not
            // present in Projected descriptor list, extend the descriptor
            if (!QospfIsDescriptorPresentInDescriptorList(
                    neighborId,
                    projectedDescriptorList))
            {
                getNeighborNode->nodeDescriptor.bandwidth =
                    tempDescriptor.bandwidth;
                getNeighborNode->nodeDescriptor.delay =
                    tempDescriptor.delay;
                getNeighborNode->nodeDpValue.previousVertexId = descriptor;
                getNeighborNode->nodeDpValue.previousVertexInterfaceId =
                    interfaceIndex;
                getNeighborNode->nodeDpValue.nextHop = nextHop;
                break;
            }
        }
        tempListItemForNeighbor = tempListItemForNeighbor->next;
    }
}


//
// FUNCTION: QospfFindNeighborVertexForCurrentProjectedVertex
// PURPOSE:  Collects the neighbors of the Projected vertex.
// PARAMETERS:  int projectedVertex
//                      Projected vertex.
//              LinkedList* descriptorSet
//                      Pointer to the descriptor set.
//              int** neighborVertexId
//                      Pointer to that location where the neighbors
//                      will be collected.
//              int* numNeighbor
//                      Number of neighbors collected.
// RETURN: NONE
//
static
void QospfFindNeighborVertexForCurrentProjectedVertex(
    int   projectedVertex,
    LinkedList* descriptorSet,
    int** neighborVertexId,
    int*  numNeighbor)
{
    ListItem* tempListItem;
    ListItem* tempItem;
    QospfDescriptorStructure* getDescriptor;

    int* getNeighborSet = NULL;
    int* getData;

    ERROR_Assert(descriptorSet, "Descriptor set not found");
    tempListItem = descriptorSet->first;

    while (tempListItem)
    {
        getDescriptor = (QospfDescriptorStructure*) tempListItem->data;

        if (getDescriptor->fromVertexId == projectedVertex)
        {
            if (getDescriptor->neighborVertex->size != 0)
            {
                getNeighborSet = (int*) MEM_malloc
                    (getDescriptor->neighborVertex->size * sizeof (int));

                *numNeighbor = getDescriptor->neighborVertex->size;
                *neighborVertexId = getNeighborSet;
            }
            else
            {
                *numNeighbor = getDescriptor->neighborVertex->size;
                *neighborVertexId = NULL;
            }

            // Insert the neighbors
            tempItem = getDescriptor->neighborVertex->first;
            while (tempItem)
            {
                getData = (int*)tempItem->data;
                *getNeighborSet = *getData;

                getNeighborSet++;
                tempItem = tempItem->next;
            }
            break;
        }
        tempListItem = tempListItem->next;
    }
}


//
// FUNCTION: QospfComputeSinglePathUsingExtendedBreadthAlgorithm
// PURPOSE:  Modify the descriptor Set using the path calculation algorithm.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              NodeAddress sourceAddress
//                      Source IP address.
//              NodeAddress destinationAddress
//                      Destination IP address.
//              QospfQosConstraint qosConstraint
//                      Qos requirements of the data traffic.
//              LinkedList*    descriptorSet
//                      Pointer to the descriptor set created for path
//                      calculation.
// RETURN: TRUE/FALSE
//
static
BOOL QospfComputeSinglePathUsingExtendedBreadthAlgorithm(
    Node* node,
    int   sourceVertexId,
    int   destinationVertexId,
    QospfQosConstraint qosConstraint,
    LinkedList* descriptorSet)
{
    LinkedList* intermediateDescriptorlist;   // TT
    LinkedList* tempDescriptorlist;           // T
    LinkedList* projectedDescriptorList;      // P
    int*  descriptor = 0;
    int   numDescriptor = 0;
    int*  neighborVertexId = NULL;
    int   numNeighbor = 0;
    int   elementCount;
    int   count;
    int   hopCount;

    ListInit(node, &intermediateDescriptorlist);
    ListInit(node, &tempDescriptorlist);
    ListInit(node, &projectedDescriptorList);

    // Collects Link descriptors for source node. If it finds
    // descriptor variable will be that value else it will be
    // zero.
    QospfFindLinkDescriptorFromDescriptorSet(sourceVertexId,
                                             descriptorSet,
                                             &qosConstraint,
                                             &descriptor,
                                             &numDescriptor);

    for (elementCount = 0;
         elementCount <= numDescriptor - 1;
         elementCount++)
    {
        // Insert this descriptor into the descriptor list
        QospfInsertDescriptorIntoDescriptorList(
            node,
            tempDescriptorlist,
            descriptor[elementCount]);
    }

    if (numDescriptor > 0)
    {
        MEM_free(descriptor);
        descriptor = NULL;
    }

    // Make the projected descriptor list empty
    QospfEmptyDescriptorList(node, projectedDescriptorList);

    if (QOSPF_DEBUG_PATH_CAL)
    {
        printf("\nStatus of Temporary Descriptor List");
        QospfPrintDescriptorList(tempDescriptorlist);
        printf("\nStatus of Projected Descriptor List");
        QospfPrintDescriptorList(projectedDescriptorList);
    }

    hopCount = 0;

    if (QOSPF_DEBUG_PATH_CAL)
    {
        printf("\n\n{------------------------------------------------");
        printf("\nBefore Path calculation, Descriptor set becomes.....\n");
        QospfPrintDescriptorSet(descriptorSet);
        printf("\n}------------------------------------------------\n\n");
    }

    while ((!QospfIsDescriptorPresentInDescriptorList(
                destinationVertexId, projectedDescriptorList))
        && (tempDescriptorlist->size))
    {
        //------------------------ STEP 1 ------------------------------

        // Make the intermediate descriptor list empty
        QospfEmptyDescriptorList(node, intermediateDescriptorlist);

        if (QOSPF_DEBUG_PATH_CAL)
        {
            printf("\nStep 1");
            printf("\nStatus of Intermediate Descriptor List");
            QospfPrintDescriptorList(intermediateDescriptorlist);
        }

        // for (all the descriptor D(i), which belongs to the set T
        // of this testing node )
        QospfSearchDescriptorInDescriptorList(tempDescriptorlist,
                                              &descriptor,
                                              &numDescriptor);

        for (elementCount = 0; elementCount <= numDescriptor - 1;
             elementCount++)
        {
            // if the descriptor satisfy the qosConstraint, put the
            // descriptor in the set TT;
            if (QospfIsDescriptorSatisfyQosConstraint(
                    descriptor[elementCount],
                    descriptorSet,
                    qosConstraint))
            {
                // Insert this descriptor into the descriptor list
                QospfInsertDescriptorIntoDescriptorList(
                    node,
                    intermediateDescriptorlist,
                    descriptor[elementCount]);
            }
        }

        if (QOSPF_DEBUG_PATH_CAL)
        {
            printf("\nStatus of Intermediate Descriptor List");
            QospfPrintDescriptorList(intermediateDescriptorlist);
        }

        if (numDescriptor > 0)
        {
            MEM_free(descriptor);
            descriptor = NULL;
        }

        //---------------------- STEP 2 -----------------------------------

        // for (all the descriptor D(i), which belongs to the set TT of
        // this testing node)
        QospfSearchDescriptorInDescriptorList(
            intermediateDescriptorlist,
            &descriptor,
            &numDescriptor);

        for (elementCount = 0; elementCount <= numDescriptor - 1;
             elementCount++)
        {
            // if the descriptor not present in P, include it include in P
            if (!QospfIsDescriptorPresentInDescriptorList(
                    descriptor[elementCount],
                    projectedDescriptorList))
            {
                // Insert this descriptor into the descriptor list
                QospfInsertDescriptorIntoDescriptorList(
                    node,
                    projectedDescriptorList,
                    descriptor[elementCount]);
            }
            // else discard the descriptor from TT.
            else
            {
                QospfDeleteDescriptorFromDescriptorList(
                    node,
                    intermediateDescriptorlist,
                    descriptor[elementCount]);
            }
        }

        if (numDescriptor > 0)
        {
            MEM_free(descriptor);
            descriptor = NULL;
        }

        if (QOSPF_DEBUG_PATH_CAL)
        {
            printf("\nStep 2");
            printf("\nStatus of Projected Descriptor List");
            QospfPrintDescriptorList(projectedDescriptorList);
            printf("\nStatus of Intermediate Descriptor List");
            QospfPrintDescriptorList(intermediateDescriptorlist);
        }

        //---------------------- STEP 3 ------------------------

        // Make the temporary descriptor list empty
        QospfEmptyDescriptorList(node, tempDescriptorlist);

        if (QOSPF_DEBUG_PATH_CAL)
        {
            printf("\nStep 3");
            printf("\nStatus of Temporary Descriptor List");
            QospfPrintDescriptorList(tempDescriptorlist);
        }

        // for (each descriptor of the projected node in TT)
        QospfSearchDescriptorInDescriptorList(
            intermediateDescriptorlist,
            &descriptor,
            &numDescriptor);

        for (elementCount = 0; elementCount <= numDescriptor - 1;
             elementCount++)
        {
            if (QOSPF_DEBUG_PATH_CAL)
            {
                printf("\n----------------------------");
                printf("\nCurrent Descriptor %d\n",
                    descriptor[elementCount]);
            }

            // for (all the neighbor of the projected node)
            QospfFindNeighborVertexForCurrentProjectedVertex(
                descriptor[elementCount],
                descriptorSet,
                &neighborVertexId,
                &numNeighbor);

            for (count = 0; count <= numNeighbor - 1; count++)
            {
                // if (neighbor != Dp(projected node))
                if (neighborVertexId[count] !=
                    QospfReturnDpValue(descriptor[elementCount],
                                       descriptorSet))
                {
                    // concatenate the projector and include it into TT
                    QospfExtendDescriptorFromProjectedVertexToNeighbor(
                        descriptorSet,
                        descriptor[elementCount],
                        neighborVertexId[count],
                        &qosConstraint,
                        projectedDescriptorList);

                    // Insert this descriptor into the descriptor list
                    QospfInsertDescriptorIntoDescriptorList(
                        node,
                        tempDescriptorlist,
                        neighborVertexId[count]);

                    if (QOSPF_DEBUG_PATH_CAL)
                    {
                        printf("\n\tExtended upto its neighbor vertex %d\n",
                            neighborVertexId[count]);
                    }
                }
                else
                {
                    // skip the neighbor;
                    continue;
                }
            }

            if (numNeighbor > 0)
            {
                MEM_free(neighborVertexId);
            }

            if (QOSPF_DEBUG_PATH_CAL)
            {
                printf("\n----------------------------");
            }
        }

        if (numDescriptor > 0)
        {
            MEM_free(descriptor);
            descriptor = NULL;
        }

        if (QOSPF_DEBUG_PATH_CAL)
        {
            printf("\nStatus of Temporary Descriptor List");
            QospfPrintDescriptorList(tempDescriptorlist);
        }

        // increase the hop count
        hopCount++;

        if (QOSPF_DEBUG_PATH_CAL)
        {
            printf("\nNumber of Hop %d\n", hopCount);
        }

        if (QOSPF_DEBUG_PATH_CAL)
        {
            printf("\n\n{~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
            printf("\nPresent descriptor set .....\n");
            QospfPrintDescriptorSet(descriptorSet);
            printf("\n}~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
        }
    }

    if (QospfIsDescriptorPresentInDescriptorList(
           destinationVertexId,
           projectedDescriptorList))
    {
        // Free the different descriptor lists created for path calculation
        ListFree(node, intermediateDescriptorlist, FALSE);
        ListFree(node, tempDescriptorlist, FALSE);
        ListFree(node, projectedDescriptorList, FALSE);
        return TRUE;
    }
    else
    {
        // Free the different descriptor lists created for path calculation
        ListFree(node, intermediateDescriptorlist, FALSE);
        ListFree(node, tempDescriptorlist, FALSE);
        ListFree(node, projectedDescriptorList, FALSE);
        return FALSE;
    }
}


//--------------------------------------------------------------------------
//          Qos application path calculation and forwarding routines
//--------------------------------------------------------------------------

//
// FUNCTION: QospfIsPacketDemandedQos
// PURPOSE:  Checks whether the application packet coming from the
//           Quality of Service Application. This function is called
//           from RoutePacketAndSendToMac of ip.c
// PARAMETERS:  Node* node
//                      Pointer to node.
//              Message* msg
//                      Pointer to the message which contains
//                      data packet
// RETURN: TRUE/FALSE.
//
BOOL QospfIsPacketDemandedQos(Node* node, Message* msg)
{
    ListItem* tempListItem;
    BOOL isPacketArrivedPreviously = FALSE;
    QospfDifferentSessionInformation* getPath;
    QospfDifferentSessionInformation* tempPath;

    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    TransportUdpHeader* udpHeader = (TransportUdpHeader*)
            ((char*) ipHeader + IpHeaderGetHLen(ipHeader->ip_v_hl_tos_len)
            * 4);

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;

    if (!ospf->isQosEnabled)
    {
        return FALSE;
    }


    // Presently we are considering QOS only for TRAFFIC-GEN and
    // TRAFFIC-TRACE. They both use UDP. So first filtering is done
    // by checking these.
    if (ipHeader->ip_p == IPPROTO_UDP &&
        (udpHeader->destPort == APP_TRAFFIC_GEN_SERVER ||
         udpHeader->destPort == APP_TRAFFIC_TRACE_SERVER))
    {
        // Check whether this packet demanded quality of service.
        tempListItem = qospf->pathList->first;
        while (tempListItem)
        {
            getPath = (QospfDifferentSessionInformation*)
                    tempListItem->data;

            if (((getPath->sourcePort == udpHeader->sourcePort)
                && (getPath->destPort == udpHeader->destPort))
                && ((getPath->sourceAddress == ipHeader->ip_src)
                && (getPath->destAddress == ipHeader->ip_dst)))
            {
                isPacketArrivedPreviously = TRUE;
                return TRUE;
            }
            tempListItem = tempListItem->next;
        }

        // Record the arrival of this type of packet assuming it is
        // coming from a new connection. Intermediate nodes uses this
        // informations to forward packets properly taking paths from
        // IP option field.
        if (!isPacketArrivedPreviously)
        {
            //qospf->qospfStat.numActiveConnection++;

            tempPath = (QospfDifferentSessionInformation*)
                    MEM_malloc(sizeof(
                            QospfDifferentSessionInformation));
            memset(tempPath, 0, sizeof(QospfDifferentSessionInformation));

            tempPath->sessionId = qospf->pathList->size + 1;
            tempPath->isOriginator = FALSE;

            tempPath->sourcePort = udpHeader->sourcePort;
            tempPath->destPort = udpHeader->destPort;
            tempPath->sourceAddress = ipHeader->ip_src;
            tempPath->destAddress = ipHeader->ip_dst;
            tempPath->numRouteAddresses = 0;

            // Insert the new calculated path into pathList
            ListInsert(node, qospf->pathList, 0, (void*)tempPath);

            return TRUE;
        }
    }

    return FALSE;
}


//
// FUNCTION: QospfForwardQosApplicationPacket
// PURPOSE:  Forwards the application packets, by retrieve the route from
//           pathList, if the route is previously found. If the path list
//           not found then free the message. This function is called
//           from RoutePacketAndSendToMac of ip.c
// PARAMETERS:  Node* node
//                      Pointer to node.
//              Message* msg
//                      Pointer to the message which contains
//                      data packet
//              BOOL *PacketWasRouted
//                      Pointer to PacketWasRouted flag
// RETURN: NONE.
//
void QospfForwardQosApplicationPacket(
    Node* node,
    Message* msg,
    BOOL *PacketWasRouted)
{
    ListItem* tempListItem;
    QospfDifferentSessionInformation* getPath = NULL;

    BOOL isPathFound = FALSE;

    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    TransportUdpHeader* udpHeader = (TransportUdpHeader*)
            ((char*) ipHeader + IpHeaderGetHLen(ipHeader->ip_v_hl_tos_len)
            * 4);

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;

    tempListItem = qospf->pathList->first;
    while (tempListItem)
    {
        getPath = (QospfDifferentSessionInformation*)
                tempListItem->data;

        if (((getPath->sourcePort == udpHeader->sourcePort)
              && (getPath->destPort == udpHeader->destPort))
            && ((getPath->sourceAddress == ipHeader->ip_src)
                 && (getPath->destAddress == ipHeader->ip_dst)))
        {
            if (QOSPF_DEBUG)
            {
                char srcAddr[MAX_ADDRESS_STRING_LENGTH];
                char destAddr[MAX_ADDRESS_STRING_LENGTH];
                IO_ConvertIpAddressToString(
                    getPath->sourceAddress, srcAddr);
                IO_ConvertIpAddressToString(
                    getPath->destAddress, destAddr);
                printf("A datapacket arrived from ......\n");
                printf("\tSource Addr %s Dest Addr %s\n",
                    srcAddr, destAddr);
                printf("\tSource port %d Dest port %d\n\n",
                    getPath->sourcePort, getPath->destPort);

                QospfPrintSessionInformation(node);
            }

                isPathFound = TRUE;
            break;
        }
        tempListItem = tempListItem->next;
    }


    // Forwarding of packets by Q-OSPF.

    if (isPathFound && getPath->numRouteAddresses >= 1)
    {
        int outgoingInterface = -1;
        NodeAddress nextHop;

        if (getPath->numRouteAddresses == 1)
        {
            // calculate outgoing interface to route the packet
            memcpy((char *)&nextHop,
                   (char *)&ipHeader->ip_dst,
                   sizeof(NodeAddress));
            outgoingInterface = NetworkIpGetInterfaceIndexForNextHop(node,
                                                                     nextHop);
        }

        if (outgoingInterface != -1)
        {
            NetworkIpSendPacketOnInterface(node,
                                           msg,
                                           CPU_INTERFACE,
                                           outgoingInterface,
                                           nextHop);
            *PacketWasRouted = TRUE;
        }
        else if (getPath->numRouteAddresses > 1 ||
                 (getPath->numRouteAddresses == 1 && 
                  outgoingInterface == -1))
        {
            IpHeaderType *ipHeader = (IpHeaderType *) msg->packet;
            memcpy((char *)&ipHeader->ip_dst, (char *)getPath->routeAddresses,
                                                           sizeof(NodeAddress));

            NodeAddress* routeAddresses = getPath->routeAddresses;
            int numRouteAddresses = getPath->numRouteAddresses - 1;
            routeAddresses++;

            // Forward the packet using source route technique
            NetworkIpSendPacketToMacLayerWithNewStrictSourceRoute(
                node,
                msg,
                routeAddresses,
                numRouteAddresses,
                FALSE);

            if (QOSPF_DEBUG)
            {
                QospfPrintSourceRoutedPath(
                    node,
                    getPath->routeAddresses,
                    getPath->numRouteAddresses);
            }

            // Packet properly forwarded. So make the flag TRUE
            *PacketWasRouted = TRUE;
        }
    }
    else
    {
        // Path not found so packet is not routed by Q-OSPF. This packet
        // may come from a appplication not interested in Quality of
        //  Service. So leave it for IP for proper routing.

        *PacketWasRouted = FALSE;
    }

}


//
// FUNCTION: QospfIsPathAdmissionPosssible
// PURPOSE:  Checks whether a Qos path is possible this new session.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
//              int sessionId
//                      Session Id for which path calculation is
//                      going to invoke
// RETURN: TRUE/FALSE.
//
static
BOOL QospfIsPathAdmissionPosssible(
    Node* node,
    QospfData* qospf,
    int sessionId)
{
    LinkedList* descriptorSet = NULL;
    ListItem* listItem;
    NodeAddress* routeAddresses;
    int numRouteAddresses;

    BOOL isPathFound = FALSE;
    BOOL isErrorInPath = TRUE;

    NodeAddress sourceAddress;
    NodeAddress destinationAddress;
    int  sourceVertexId = 0;
    int  destinationVertexId = 0;
    QospfQosConstraint qosConstraint;

    QospfDifferentSessionInformation* getPath = NULL;

    // Find the information collected in lath list
    listItem = qospf->pathList->first;
    while (listItem)
    {
        getPath = (QospfDifferentSessionInformation*)
            listItem->data;
        if (getPath->sessionId == sessionId)
        {
            break;
        }
        listItem = listItem->next;
    }

    // 'getPath' is the location where info about this session is stored.
    // So use this pointer if modification or any info required from
    // this session
    getPath->sessionAdmittedAt = node->getNodeTime();
    getPath->numTimesRetried++;

    qosConstraint.priority = getPath->requestedMetric.priority;
    qosConstraint.availableBandwidth =
        getPath->requestedMetric.availableBandwidth;
    qosConstraint.averageDelay = getPath->requestedMetric.averageDelay;

    // Start path calculation by calling proper algorithm
    // as indicating in config. file

    switch (qospf->pathCalculationAlgorithm)
    {
        case EXTENDED_BREADTH_FIRST_SEARCH_SINGLE_PATH:
        {
            // The data field of list descriptor set holds
            // QospfDescriptorStructure structure. The total
            // list is created for path calculation purpose. So when
            // path calculation required we will create the list.
            ListInit(node, &descriptorSet);

            // Collect the source and destination address from info field
            sourceAddress = getPath->sourceAddress;
            destinationAddress = getPath->destAddress;

            // Convert the IP address into Vertex number. Because the path
            // calculation algorithm works on vertex number.
            sourceVertexId = QospfConvertIpAddressIntoVertexId(
                    qospf, sourceAddress);

            destinationVertexId = QospfConvertIpAddressIntoVertexId(
                    qospf, destinationAddress);

            // Source or destination not found in present condition. So path
            // must not be found. Path calculation algorithm must be skipped
            if (sourceVertexId == 0 || destinationVertexId == 0)
            {
                isPathFound = FALSE;
                break;
            }

            if (QOSPF_DEBUG)
            {
                printf("\n\n    -------------------------------");
                printf("\n\tFor source Addr. = 0x%x\n\tSource Vertex ID "
                    "= %d\n", sourceAddress, sourceVertexId);
                printf("\n\tFor Destin Addr. = 0x%x\n\tDestin Vertex ID "
                    "= %d", destinationAddress, destinationVertexId);
                printf("\n    -------------------------------\n\n");
            }

            // Initialize descriptor set
            QospfCreateDescriptorSet(
                node, qospf, descriptorSet, qosConstraint);

            if (QOSPF_DEBUG)
            {
                printf("\n\nBefore Path calculation, Descriptor set is..");
                QospfPrintDescriptorSet(descriptorSet);
            }

            isPathFound =
                QospfComputeSinglePathUsingExtendedBreadthAlgorithm(
                    node,
                    sourceVertexId,
                    destinationVertexId,
                    qosConstraint,
                    descriptorSet);
            break;
        }
        default:
        {
            // Shouldn't get here
            ERROR_ReportError("\nBad QOSPF-COMPUTATION-ALGORITHM ");
        }
    }



    if (QOSPF_DEBUG)
    {
        printf("\n\n");
        printf("After Path calculation, Descriptor set becomes.....");
        QospfPrintDescriptorSet(descriptorSet);
    }

    // Path found
    if (isPathFound)
    {
        // Extract the path created in Descriptor Set
        isErrorInPath = QospfExtractRouteFromDescriptorSet(
                            node,
                            descriptorSet,
                            sourceVertexId,
                            destinationVertexId,
                            &routeAddresses,
                            &numRouteAddresses);
        if (!isErrorInPath)
        {
            if (QOSPF_DEBUG_OUTPUT)
            {
                int count;
                printf("\n\n--------------------------------------------");
                printf("-------------------------------");
                printf("\nTotal QoS path for the (0x%x, 0x%x) pair: ",
                    getPath->sourceAddress, getPath->destAddress);

                for (count = 0; count < numRouteAddresses; count++)
                {
                    printf("%x  ", routeAddresses[count]);
                }
                printf("\n----------------------------------------------");
                printf("-----------------------------\n");
            }

            // Store the path in pathList for forwarding the packets
            getPath->routeAddresses = (NodeAddress*) MEM_malloc(
                    sizeof(NodeAddress) * numRouteAddresses);

            memcpy(getPath->routeAddresses,
                   routeAddresses,
                   sizeof(NodeAddress) * numRouteAddresses);
            getPath->numRouteAddresses = numRouteAddresses;

            // Path calculation called to forward a packet. So increase
            // statistic value of total number active session.
            qospf->qospfStat.numActiveConnection++;
        }

        // Free temporarily collected path
        MEM_free(routeAddresses);
    }
    else
    {
        if (QOSPF_DEBUG)
        {
            printf("\n--------------------------------------------------");
            printf("-------------------------");
            printf("\n\tQoS path not found for the (%x, %x) pair",
                (UInt32) getPath->sourceAddress, (UInt32) getPath->destAddress);
            printf("\n--------------------------------------------------");
            printf("-------------------------\n");
        }
    }

    if (qospf->pathCalculationAlgorithm ==
        EXTENDED_BREADTH_FIRST_SEARCH_SINGLE_PATH)
    {
        // Descriptor set is not required, as all the desired information
        // was retrieved. So free the list of descriptor structure.
        QospfFreeDescriptorStructureList(node, descriptorSet);
    }

    // Remove database created for path calculation
    QospfFreeVertexSet(node, qospf->vertexSet);
    QospfFreeTopologyLinksList(node, qospf->topologyLinksList);

    return (!isErrorInPath);
}


//
// FUNCTION: QospfCreateDatabaseAndFindQosPath
// PURPOSE:  Creates database when a request (from application) of a
//           new session session comes and after that it calculates a
//           Qos path if possible.
// PARAMETERS:  Node* node
//                      Pointer to node.
//              Message*   msgForConnectionRequest
//                      Pointer to the message which contains
//                      connection request
// RETURN: None.
//
void QospfCreateDatabaseAndFindQosPath(
    Node*      node,
    Message*   msgForConnectionRequest)
{
    BOOL isPathFound = FALSE;
    QospfDifferentSessionInformation* tempPath;

    AppQosToNetworkSend* info;

    Message* newMsg = NULL;
    NetworkToAppQosConnectionStatus* statusInfo;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;


    info = (AppQosToNetworkSend*)MESSAGE_ReturnInfo(msgForConnectionRequest);

    // Register the session if it is new
    if (!ReturnSessionId(node, qospf, info))
    {
        // Get all identifications of this new session from,
        // info field of the message sent by the application.
        tempPath = (QospfDifferentSessionInformation*)
            MEM_malloc(sizeof(QospfDifferentSessionInformation));

        tempPath->sessionId = qospf->pathList->size + 1;
        tempPath->sourcePort = info->sourcePort;
        tempPath->destPort = info->destPort;
        tempPath->sourceAddress = info->sourceAddress;
        tempPath->destAddress = info->destAddress;

        tempPath->isOriginator = TRUE;
        tempPath->sessionAdmittedAt = node->getNodeTime();
        tempPath->numTimesRetried = 0;

        tempPath->requestedMetric.priority = info->priority;
        tempPath->requestedMetric.availableBandwidth =
                info->bandwidthRequirement;
        tempPath->requestedMetric.averageDelay = info->delayRequirement;

        tempPath->routeAddresses = NULL;
        tempPath->numRouteAddresses = 0;

        ListInsert(node,
                   qospf->pathList,
                   node->getNodeTime(),
                   (void*) tempPath);
    }

    if (QOSPF_DEBUG)
    {
        char addrStr[MAX_ADDRESS_STRING_LENGTH];

        printf("Qos requirements of the connection request are ..\n");

        IO_ConvertIpAddressToString(info->sourceAddress, addrStr);
        printf("\tSource Addr %s\n", addrStr);

        IO_ConvertIpAddressToString(info->destAddress, addrStr);
        printf("\tDest Addr %s\n", addrStr);

        printf("\tPriority :> %u\n", info->priority);
        printf("\tBandwidth requirement :> %d(bps)\n",
            info->bandwidthRequirement);
        printf("\tDelay requirement :> %d(ns)\n", info->delayRequirement);
    }

    // Create the data base for Q-OSPF before path calculation
    QospfCreateDatabase(node, qospf);

    // Calculate path for this session.
    isPathFound = QospfIsPathAdmissionPosssible(
            node, qospf, ReturnSessionId(node, qospf, info));


    // Return status of connection to application layer
    if (info->clientType == APP_TRAFFIC_GEN_CLIENT)
    {
        newMsg = MESSAGE_Alloc(
                      node,
                      APP_LAYER,
                      APP_TRAFFIC_GEN_CLIENT,
                      MSG_APP_SessionStatus);
    }
    else if (info->clientType == APP_TRAFFIC_TRACE_CLIENT)
    {
        newMsg = MESSAGE_Alloc(
                      node,
                      APP_LAYER,
                      APP_TRAFFIC_TRACE_CLIENT,
                      MSG_APP_SessionStatus);
    }
    else
    {
        ERROR_Assert(FALSE,
            "Q-OSPF got connection request from unknown client");
    }

    MESSAGE_InfoAlloc(
        node, newMsg, sizeof(NetworkToAppQosConnectionStatus));

    statusInfo = (NetworkToAppQosConnectionStatus*)
            MESSAGE_ReturnInfo(newMsg);
    statusInfo->sourcePort = info->sourcePort;
    statusInfo->isSessionAdmitted = isPathFound;

    MESSAGE_Send(node, newMsg, 0);
}


//--------------------------------------------------------------------------
//                      Init and Finalize functions
//--------------------------------------------------------------------------

//
// FUNCTION:    QospfQueueInfoInit
// PURPOSE:     Update the present queue status of this node.
// PARAMETERS:  Node* node
//                      Pointer to node.
// RETURN: None.
//
static
void QospfQueueInfoInit(Node* node)
{
    int count;
    int qCount;
    Ospfv2StatusOfQueue* queueStatus;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    QospfData* qospf = (QospfData*) ospf->qosRoutingProtocol;

    // Initialize proper value for different queues
    for (count = 0; count < node->numberInterfaces; count++)
    {
        ERROR_Assert(ospf->iface[count].presentStatusOfQueue,
            "presentStatusOfQueue list not initialized");

        if (ospf->iface[count].presentStatusOfQueue->size == 0)
        {
            // Informations for all queues at an interface are
            // maintained in a list. Primary consideration of this
            // list is that all queues will be inserted in decreasing
            // order of priority. This assumption helps to calculate
            // available bandwidth of a queue dynamically.
            for (qCount = qospf->maxQueueInEachInterface - 1;
                 qCount >= 0;
                 qCount--)
            {
                queueStatus = (Ospfv2StatusOfQueue*)
                    MEM_malloc(sizeof(Ospfv2StatusOfQueue));

                // get Queue number
                queueStatus->queueNo = (unsigned char) qCount;

                // get link bandwidth
                queueStatus->linkBandwidth =
                   (unsigned int) node->macData[count]->bandwidth;

                // get link's utilized bandwidth
                queueStatus->utilizedBandwidth = 0;

                // get link's available bandwidth
                queueStatus->availableBandwidth =
                    queueStatus->linkBandwidth;

                // get link's propagation delay
                queueStatus->propDelay = (int)
                    (node->macData[count]->propDelay / MICRO_SECOND);

                // Get queuing delay of this queue
                queueStatus->qDelay = 0;

                // qospf->qospfStat.linkBandwidth collects maximum link
                // bandwidth of every interface of a node.
                qospf->qospfStat.linkBandwidth[count] =
                    queueStatus->linkBandwidth;

                ListInsert(node,
                           ospf->iface[count].presentStatusOfQueue,
                           0,
                           (void*)queueStatus);
            }
        }
    }

    if (QOSPF_DEBUG)
    {
        // Prints different queue status for every
        // interface of the nodes
        QospfPrintQueueStatus(node);
    }
}


//
// FUNCTION: QospfInit
// PURPOSE:  Initialize Q-OSPF protocol
// PARAMETER:   Node* node
//                      Pointer to node.
//              QospfData** qospfLayerPtr
//                      Pointer to Q-OSPF layer structure
//              const NodeInput* nodeInput
//                      Pointer to node input.
// RETURN: NONE.
//
void QospfInit(
    Node* node,
    QospfData** qospfLayerPtr,
    const NodeInput* nodeInput)
{
    BOOL retVal;
    Message* newMsg;
    char buf[MAX_STRING_LENGTH];

    QospfData* qospf = (QospfData*)
            MEM_malloc(sizeof(QospfData));

    (*qospfLayerPtr) = qospf;

    // Check whether domain is partitioned into several areas.
    // If so, provide error message. Because, currently Q-ospf
    // only supports single area.
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "OSPFv2-DEFINE-AREA",
        &retVal,
        buf);

    if ((retVal == TRUE) && (strcmp (buf, "YES") == 0))
    {
        ERROR_ReportError("\nCurrently Q-ospf supports single area only.");
    }

    // Determine if stats required at end of simulation
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "QOSPF-STATISTICS",
        &retVal,
        buf);

    if ((retVal == FALSE) || (strcmp (buf, "NO") == 0))
    {
        qospf->collectStat = FALSE;
    }
    else if (strcmp (buf, "YES") == 0)
    {
        qospf->collectStat = TRUE;
    }

    // For setting the path calculation algorithm that
    // will be used for path calculation.
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "QOSPF-COMPUTATION-ALGORITHM",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp (buf, "EXTENDED_BREADTH_FIRST_SEARCH_SINGLE_PATH") == 0)
        {
            qospf->pathCalculationAlgorithm =
                EXTENDED_BREADTH_FIRST_SEARCH_SINGLE_PATH;
        }
        else
        {
            ERROR_ReportError("\nInvalid Path Calculation algorithm");
        }
    }
    else
    {
        ERROR_ReportWarning("QOSPF-COMPUTATION-ALGORITHM not found.\n"
            "Default algorithm considered.");

        // Default path calculation algorithm is Extended Breadth
        // First Search algorithm for Single Path
        qospf->pathCalculationAlgorithm =
            EXTENDED_BREADTH_FIRST_SEARCH_SINGLE_PATH;

        if (QOSPF_DEBUG)
        {
            printf("\nDefault Path Computation Algorithm is assigned");
            printf("\nPath computation ....%d",
                    qospf->pathCalculationAlgorithm);
        }
    }

    // Initialization of this database will take place
    // during path calculation.
    qospf->topologyLinksList = NULL;
    qospf->vertexSet = NULL;

    // Get number of queue in each interface
    IO_ReadInt(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "IP-QUEUE-NUM-PRIORITIES",
        &retVal,
        &(qospf->maxQueueInEachInterface));

    if (retVal)
    {
        if (QOSPF_DEBUG)
        {
            printf("At node %u maximun queues in each interface is %d\n",
                node->nodeId, qospf->maxQueueInEachInterface);
        }

        if (qospf->maxQueueInEachInterface <= 0 ||
            qospf->maxQueueInEachInterface > 8)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                "Value of parameter IP-QUEUE-NUM-PRIORITIES not proper"
                " for node %u.\n"
                "Nunber of queues for Q-ospf will be within 1 to 8.\n",
                node->nodeId);
            ERROR_ReportError(errStr);
        }
        }
    else
    {
        char errStr[MAX_STRING_LENGTH];

        sprintf(errStr,
            "IP-QUEUE-NUM-PRIORITIES not set for node %u.\n"
            "In Q-ospf, all interfaces of a given node should have "
            "the same number of queues.\n",
            node->nodeId);
        ERROR_ReportError(errStr);
    }

    // Initialize the path list, which contains the calculated
    // path for every connection originated by this node.
    ListInit(node, &qospf->pathList);

    // statistics collection
    qospf->qospfStat.numActiveConnection = 0;
    qospf->qospfStat.numRejectedConnection = 0;
    qospf->qospfStat.numPeriodicUpdate = 0;
    qospf->qospfStat.numTriggeredUpdate = 0;

    qospf->qospfStat.linkBandwidth
        = (int *) MEM_malloc(sizeof (int) * node->numberInterfaces);

    qospf->qospfStat.bandwidthUtilization
        = (int *) MEM_malloc(sizeof(int) * node->numberInterfaces);
    memset(qospf->qospfStat.bandwidthUtilization, 0,
           sizeof(int) * node->numberInterfaces);

    // Collect interval of periodic Link State update.
    // If period not defined, then Q-OSPF will depends on OSPF
    // to originate periodic update.
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "QOSPF-FLOODING-INTERVAL",
        &retVal,
        buf);

    if (retVal)
    {
        // get user specified value
        qospf->floodingInterval = TIME_ConvertToClock(buf);

        if (qospf->floodingInterval < 0)
        {
            char string[MAX_STRING_LENGTH];
            sprintf(string,
                "QOSPF-FLOODING-INTERVAL for node %u "
                "should be greater than or equal to zero\n",
                node->nodeId);
            ERROR_ReportError(string);
        }

        // For floodingInterval = 0 ,no periodic message is required to
        // send. As Q-ospf depends upon
        // ospf for periodic flooding.

        if (qospf->floodingInterval > 0)
        {
            newMsg = MESSAGE_Alloc(node,
                               NETWORK_LAYER,
                               ROUTING_PROTOCOL_OSPFv2,
                               MSG_ROUTING_QospfScheduleLSDB);

            MESSAGE_Send(node, newMsg, qospf->floodingInterval);
        }
    }
    else
    {
        // No periodic message is required to send. As Q-ospf depends upon
        // ospf for periodic flooding. So initialise this variable.
        qospf->floodingInterval = 0;
    }

    if (QOSPF_DEBUG)
    {
        printf("\nSelected interval for periodic flooding = %s\n", buf);
    }

    // Get interval for interface status observation
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "QOSPF-INTERFACE-OBSERVATION-INTERVAL",
        &retVal,
        buf);

    if (retVal)
    {
        // Get user specified value
        qospf->interfaceMonitorInterval = TIME_ConvertToClock(buf);

        if (qospf->interfaceMonitorInterval <= 0)
        {
            char string[MAX_STRING_LENGTH];
            sprintf(string,
                "QOSPF-INTERFACE-OBSERVATION-INTERVAL for node %u."
                "should be greater than zero\n", node->nodeId);
            ERROR_ReportError(string);
        }
    }
    else
    {
        // Default value of interface observation interval is 2 second
        qospf->interfaceMonitorInterval =
                QOSPF_DEFAULT_INTERFACE_OBSERVATION_INTERVAL;
    }

    if (QOSPF_DEBUG)
    {
        printf("\nSelected interval for interface "
            "observation = %s\n", buf);
    }

    // Allocate and send a message to trigger own interface status
    // monitoring process.
    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_OSPFv2,
                           MSG_ROUTING_QospfInterfaceStatusMonitor);

    MESSAGE_Send(node, newMsg, qospf->interfaceMonitorInterval);

    // Get flooding factor
    IO_ReadDouble(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "QOSPF-FLOODING-FACTOR",
        &retVal,
        &qospf->floodingFactor);

    if (!retVal)
    {
        qospf->floodingFactor = QOSPF_DEFAULT_FLOODING_FACTOR;
    }
    else if (qospf->floodingFactor >= 1.0)
    {
        ERROR_ReportWarning("QOSPF-FLOODING-FACTOR not proper.\n"
            "Default value considered.");
        qospf->floodingFactor = QOSPF_DEFAULT_FLOODING_FACTOR;
    }

    if (QOSPF_DEBUG)
    {
        printf("\nSelected flooding factor = %f\n",
            qospf->floodingFactor);
    }

    // Determines whether queuing delay will be considered
    // for path calculation or not.
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "QUEUEING-DELAY-FOR-QOS-PATH-CALCULATION",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp (buf, "YES") == 0)
        {
            qospf->isQueueDelayConsidered = TRUE;
        }
        else
        {
            qospf->isQueueDelayConsidered = FALSE;
        }
    }
    else
    {
        // By default queuing delay will not be considered
        // for Qos path calculation.
        qospf->isQueueDelayConsidered = FALSE;
    }

    // Initialize the bandwidth and delay information of each
    // queue of every interface of this node.
    QospfQueueInfoInit(node);

    // Dynamic simulation results are stored in a text
    // file for analysis the result graphically.
    if (QOSPF_DEBUG_OUTPUT_FILE)
    {
        char fileName[MAX_STRING_LENGTH];
        FILE* fp;
        char dataBuf[MAX_STRING_LENGTH];
        int count;

        sprintf(fileName, "DataFile_%d", node->nodeId);

        // Open a data file to accumulate different statistics
        // informations of this node.
        fp = fopen(fileName, "w");
        if (fp)
        {
            sprintf(dataBuf, "%s\n",
                "       Conn.    Conn.     Utilized Bandwidth(in Kbps) ");
            sprintf(dataBuf + strlen(dataBuf), "%s\t",
                    "Time  Granted  Rejected");
            for (count = 0; count < node->numberInterfaces; count++)
            {
                sprintf(dataBuf + strlen(dataBuf), "%s%d\t", "I", count);
            }
            sprintf(dataBuf + strlen(dataBuf), "\n");
            if (fwrite(dataBuf, strlen(dataBuf), 1, fp)!= 1)
            {
                 ERROR_ReportWarning("Unable to write in file \n");
            }
            fclose(fp);
            dataBuf[0] = '\0';
        }
    }

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "QOSPF-TRACE",
        &retVal,
        buf);
    qospf->trace = FALSE;
    if (retVal)
    {
        if (strcmp (buf, "YES") == 0)
        {
            qospf->trace = TRUE;
        }
        else
        {
            qospf->trace = FALSE;
        }
    }
}


//
// FUNCTION: QospfFinalize
// PURPOSE:  Prints out the required statistics for this protocol during
//           termination of the simulation. This function is called from
//           ospfv2.c
// PARAMETERS:  Node* node
//                      Pointer to node.
//              QospfData* qospf
//                      Pointer to Q-OSPF layer structure
// RETURN: None
//
void QospfFinalize(Node* node, QospfData* qospf)
{
    int count;

    if (qospf->collectStat)
    {
        char buf[MAX_STRING_LENGTH];

        ListItem* tempListItem;
        QospfDifferentSessionInformation* sessionInfo;

        // Collect connection statistics
        tempListItem = qospf->pathList->first;
        while (tempListItem)
        {
            sessionInfo = (QospfDifferentSessionInformation*)
                tempListItem->data;
            if (sessionInfo->isOriginator &&
                (!sessionInfo->numRouteAddresses))
            {
                qospf->qospfStat.numRejectedConnection++;
            }
            tempListItem = tempListItem->next;
        }

        sprintf(buf, "Active Connections = %d",
            qospf->qospfStat.numActiveConnection);
        IO_PrintStat(
            node,
            "Network",
            "Q-OSPF",
            ANY_DEST,
            -1, // instance Id
            buf);

        sprintf(buf, "Rejected Connections = %d",
            qospf->qospfStat.numRejectedConnection);
        IO_PrintStat(
            node,
            "Network",
            "Q-OSPF",
            ANY_DEST,
            -1, // instance Id
            buf);

        sprintf(buf, "Periodic Update send = %d",
            qospf->qospfStat.numPeriodicUpdate);
        IO_PrintStat(
            node,
            "Network",
            "Q-OSPF",
            ANY_DEST,
            -1, // instance Id
            buf);

        sprintf(buf, "Triggered Update send = %d",
            qospf->qospfStat.numTriggeredUpdate);
        IO_PrintStat(
            node,
            "Network",
            "Q-OSPF",
            ANY_DEST,
            -1, // instance Id
            buf);

        for (count = 0; count < node->numberInterfaces; count++)
        {
            sprintf(buf, "Link Bandwidth = %d",
                qospf->qospfStat.linkBandwidth[count]);
            IO_PrintStat(
                node,
                "Network",
                "Q-OSPF",
                ANY_DEST,
                count, // instance Id
                buf);

            sprintf(buf, "Maximum Utilization of Bandwidth = %d",
                qospf->qospfStat.bandwidthUtilization[count]);
            IO_PrintStat(
                node,
                "Network",
                "Q-OSPF",
                ANY_DEST,
                count, // instance Id
                buf);
        }
    }

    if (QOSPF_DEBUG_FINALIZE)
    {
        // Print Path list
        QospfPrintSessionInformation(node);
    }

    // Free path list maintained by Q-ospf
    QospfReleasePathList(node, qospf);
}
