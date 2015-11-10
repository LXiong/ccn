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
// PROTOCOL   :: LANMAR
// LAYER      :: NETWORK
// REFERENCES ::
// + IETF Draft : draft-ietf-manet-lanmar-04.txt
// + IETF Draft : raft-ietf-manet-fsr-03.txt
// COMMENTS   :: This is the implementation of Landmark Ad Hoc
//               Routing (LANMAR) protocol
// **/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"
#include "network_ip.h"
#include "network_dualip.h"
#include "routing_fsrl.h"


// Use hysteresis winner compete, otherwise, use the simple rule.
#define HYSTERESIS

#define DRIFTER



// /**
// FUNCTION   :: FsrlInitDebug
// LAYER      :: NETWORK
// PURPOSE    :: Used for debugging purposes for the configuration variables.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlInitDebug(Node* node)
{
    return FALSE;
}


// /**
// FUNCTION   :: FsrlDebug
// LAYER      :: NETWORK
// PURPOSE    :: Used for debugging purposes
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebug(Node* node)
{
   return FALSE;
}


// /**
// FUNCTION   :: FsrlDebugTopologyTable
// LAYER      :: NETWORK
// PURPOSE    :: Whether to print out topology table debug information
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugTopologyTable(Node* node)
{
    return FALSE;

}


// /**
// FUNCTION   :: FsrlDebugInDetail
// LAYER      :: NETWORK
// PURPOSE    :: Whether to print out more detailed debug information
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugInDetail(Node* node)
{
    return FALSE;

}


// /**
// FUNCTION   :: FsrlDebugNeighbor
// LAYER      :: NETWORK
// PURPOSE    :: Whether to print out neighbor list debug information
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugNeighbor(Node* node)
{
   return FALSE;

}


// /**
// FUNCTION   :: FsrlDebugShortestPath
// LAYER      :: NETWORK
// PURPOSE    :: Whether to debug the shortest path calculation
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugShortestPath(Node* node)
{
   return FALSE;
}


// /**
// FUNCTION   :: FsrlDebugForwarding
// LAYER      :: NETWORK
// PURPOSE    :: Whether to debug data packet forwarding
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugForwarding(Node* node)
{
   return FALSE;
}


// /**
// FUNCTION   :: FsrlDebugForwarding
// LAYER      :: NETWORK
// PURPOSE    :: Whether to debug data packet forwarding
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugTimers(Node* node)
{
   return FALSE;
}


// /**
// FUNCTION   :: FsrlDebugPackets
// LAYER      :: NETWORK
// PURPOSE    :: Whether to debug routing packet
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugPackets(Node* node)
{
   return FALSE;
}


// /**
// FUNCTION   :: FsrlDebugLandmark
// LAYER      :: NETWORK
// PURPOSE    :: Whether to debug landmark updates
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugLandmark(Node* node)
{
   return FALSE;
}


// /**
// FUNCTION   :: FsrlDebugLandmarkElection
// LAYER      :: NETWORK
// PURPOSE    :: Whether to debug landmark election
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugLandmarkElection(Node* node)
{
    return FALSE;
}


// /**
// FUNCTION   :: FsrlDebugDrifter
// LAYER      :: NETWORK
// PURPOSE    :: Whether to debug drifter
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL FsrlDebugDrifter(Node* node)
{
    return FALSE;
}


// /**
// FUNCTION   :: FsrlPrintRoutingTable
// LAYER      :: NETWORK
// PURPOSE    :: Print local scope routing table entries
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlPrintRoutingTable(Node* node, FsrlData* fisheye)
{
    FisheyeRoutingTable *routingTable = &(fisheye->routingTable);
    char addrString1[MAX_STRING_LENGTH];
    char addrString2[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];

    int i;

    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
    printf("    routing table content of node %d at %s:\n",
    node->nodeId, clockStr);
    printf("        size is %u\n", routingTable->rtSize);

    for (i = 0; i < routingTable->rtSize; i ++)
    {
        IO_ConvertIpAddressToString((routingTable->row)[i].destAddr,
                                    addrString1);
        IO_ConvertIpAddressToString((routingTable->row)[i].nextHop,
                                    addrString2);
        printf("destAddr = %s, nextHop = %s, Outgoing Interface = %d,"
                                                          "distance = %u\n",
           addrString1, addrString2,(routingTable->row)[i].outgoingInterface,
               (routingTable->row)[i].distance);
    }

    printf("\n");
}


// /**
// FUNCTION   :: FsrlPrintLandmarkTable
// LAYER      :: NETWORK
// PURPOSE    :: Print landmark table entries
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlPrintLandmarkTable(Node* node, FsrlData* fisheye)
{
    char addrString[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];
    FsrlLandmarkTableRow* row;

    char timeInSecond[MAX_STRING_LENGTH];
    TIME_PrintClockInSecond(node->getNodeTime(), timeInSecond);


    row = fisheye->landmarkTable.row;

    printf("    Landmark table content of node %d at %s:\n",
        node->nodeId, timeInSecond);
    printf("        size = %d\n", fisheye->landmarkTable.size);

    if (row == NULL)
    {
        printf("        empty\n\n");
    }

    while (row != NULL)
    {
        IO_ConvertIpAddressToString(row->destAddr, addrString);
        printf("        destAddr = %s\n", addrString);

        IO_ConvertIpAddressToString(row->nextHop, addrString);
        printf("        nextHop = %s\n", addrString);

        printf("        numLandmarkMember = %d\n",
               row->numLandmarkMember);

        printf("        distance = %d\n",
               row->distance);

        printf("        sequenceNumber = %d\n",
               row->sequenceNumber);

        ctoa(row->timestamp, clockStr);
        printf("        timestamp = %s\n\n", clockStr);

        row = row->next;
     }
     for (int i = 0; i < node->numberInterfaces; i++)
     {
        printf("        fisheye->isLandmark[%u] = %u\n",
            i, fisheye->isLandmark[i]);
     }
}


// /**
// FUNCTION   :: FsrlPrintDrifterTable
// LAYER      :: NETWORK
// PURPOSE    :: Print drifter table entries
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlPrintDrifterTable(Node* node, FsrlData* fisheye)
{
    char addrString[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];
    FsrlDrifterTableRow* row;

    row = fisheye->drifterTable.row;

    printf("    Drifter table content:\n");

    if (row == NULL)
    {
        printf("        empty\n\n");
    }

    while (row != NULL)
    {
        IO_ConvertIpAddressToString(row->destAddr, addrString);
        printf("        destAddr = %s\n", addrString);

        IO_ConvertIpAddressToString(row->nextHop, addrString);
        printf("        nextHop = %s\n", addrString);

        printf("        distance = %d\n",
               row->distance);

        printf("        sequenceNumber = %d\n",
               row->sequenceNumber);

        printf("        outgoingInterface = %d\n",
               row->outgoingInterface);

        ctoa(row->lastModified, clockStr);
        printf("        timestamp = %s\n\n", clockStr);

        row = row->next;
     }
}


// /**
// FUNCTION   :: FsrlPrintTopologyTable
// LAYER      :: NETWORK
// PURPOSE    :: Print local scope topology table entries
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlPrintTopologyTable(Node* node, FsrlData* fisheye)
{
    char addrString[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];
    FisheyeTopologyTableRow* ttRow;
    FsrlNeighborListItem* neighborList;

    ctoa(node->getNodeTime(), clockStr);

    printf("    topology table at time %s\n\n", clockStr);

    ttRow = fisheye->topologyTable.first;

    if (ttRow == NULL)
    {
        printf("        empty!\n");
    }

    // Get the number of members in the same subnet.
    while (ttRow != NULL)
    {
        IO_ConvertIpAddressToString(ttRow->linkStateNode, addrString);
        printf("        linkStateNode = %s\n", addrString);
        ctoa(ttRow->timestamp, clockStr);
        printf("        timestamp = %s\n", clockStr);
        ctoa(ttRow->removalTimestamp, clockStr);
        printf("        removalTimestamp = %s\n", clockStr);
        printf("        distance = %d\n", ttRow->distance);
        printf("        sequenceNumber = %d\n",
               ttRow->neighborInfo->sequenceNumber);
        printf("        numNeighbor = %d\n",
               ttRow->neighborInfo->numNeighbor);

        neighborList = ttRow->neighborInfo->list;

        while (neighborList != NULL)
        {
            IO_ConvertIpAddressToString(neighborList->nodeAddr,
                                        addrString);
            printf("            neighborAddr = %s\n", addrString);

            neighborList = neighborList->next;
        }

        IO_ConvertIpAddressToString(ttRow->fromAddr, addrString);
        printf("        fromAddr = %s\n", addrString);

        ttRow = ttRow->next;

        printf("\n");
    }
}


// /**
// FUNCTION   :: FsrlPrintFisheyeUpdatePacket
// LAYER      :: NETWORK
// PURPOSE    :: Print fisheye topology update packet
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to message structure of the packet
// RETURN     :: void : NULL
// **/

static
void FsrlPrintFisheyeUpdatePacket(Node* node, Message* msg)
{
    FsrlFisheyeHeaderField headerField;
    FsrlFisheyeDataField dataField;
    char addrString[MAX_STRING_LENGTH];
    NodeAddress nodeAddr;
    char* payload;
    int ttSize;
    int i;

    printf("    Fisheye Update packet content:\n");

    payload = MESSAGE_ReturnPacket(msg);

    // pointer to payload.
    ttSize = 0;

    memcpy(&headerField, &(payload[ttSize]), sizeof(headerField));
    ttSize += sizeof(headerField);
    ERROR_Assert(headerField.packetType == FSRL_UPDATE_TOPOLOGY_TABLE,
                 "Wrong packet type!");

    printf("        packetType = %u\n", headerField.packetType);
    printf("        packet length = %u\n", headerField.packetSize);

    // Start to update TopologyTable from the packet received.
    while (ttSize < MESSAGE_ReturnPacketSize(msg))
    {
        memcpy(&dataField,
               &(payload[ttSize]),
               sizeof(dataField));
        ttSize += sizeof(dataField);

        IO_ConvertIpAddressToString(dataField.destAddr, addrString);
        printf("            destAddr = %s\n", addrString);
        printf("            sequenceNumber = %u\n",
               FsrlFisheyeDataFieldGetSeqNum(dataField.dataField));
        printf("            numNeighbor = %u\n",
            FsrlFisheyeDataFieldGetNumNeighbour(dataField.dataField));

        for (i = 0; i < (signed)FsrlFisheyeDataFieldGetNumNeighbour
            (dataField.dataField); i++)
        {
            memcpy(&nodeAddr,
                   &(payload[ttSize]),
                   sizeof(nodeAddr));
            IO_ConvertIpAddressToString(nodeAddr, addrString);
            printf("                neighborAddr = %s\n", addrString);
            ttSize += sizeof(nodeAddr);

        }

        printf("\n");
    }
}


// /**
// FUNCTION   :: FsrlPrintLanmarUpdatePacket
// LAYER      :: NETWORK
// PURPOSE    :: Print landmark update packet
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to message structure of the packet
// RETURN     :: void : NULL
// **/

static
void FsrlPrintLanmarUpdatePacket(Node* node, Message* msg)
{
    int landmarkTableHeaderSize = sizeof(FsrlLanmarUpdateHeaderField);
    FsrlLanmarUpdateHeaderField headerField;
    FsrlLanmarUpdateLandmarkField landmarkField;
    FsrlLanmarUpdateDrifterField drifterField;

    char addrString[MAX_STRING_LENGTH];
    char* payload;
    int ttSize;
    int i;

    payload = MESSAGE_ReturnPacket(msg);

    // pointer to payload
    ttSize = 0;

    printf("    LANMAR Update packet content:\n");
    memcpy(&headerField, &(payload[ttSize]), sizeof(headerField));
    ttSize += sizeof(headerField);

    printf("    packet type = %d\n", headerField.packetType);

    printf("    landmark flag = %d\n", headerField.landmarkFlag);
    printf("    no. landmark entries = %d\n", headerField.numLandmark);
    printf("    no. drifter entries = %d\n", headerField.numDrifter);

    printf("    landmark entries:\n");

    if (headerField.numLandmark == 0)
    {
        printf("        empty\n\n");
    }

    // Loop through LM entries.
    for (i = 0; i < headerField.numLandmark; i ++)
    {
        memcpy(&landmarkField, &(payload[ttSize]), sizeof(landmarkField));
        ttSize += sizeof(landmarkField);

        IO_ConvertIpAddressToString(landmarkField.landmarkAddr, addrString);
        printf("        destAddr = %s\n", addrString);
        IO_ConvertIpAddressToString(landmarkField.nextHopAddr, addrString);
        printf("        nextHop = %s\n", addrString);
        printf("        distance = %d\n", landmarkField.distance);
        printf("        numMember = %d\n", landmarkField.numMember);
        printf("        sequenceNumber = %d\n\n",
               landmarkField.sequenceNumber);
    }

    printf("    drifter entries:\n");

    if (headerField.numDrifter == 0)
    {
        printf("        empty\n\n");
    }

    // Loop through drifter entries.
    for (i = 0; i < headerField.numDrifter; i ++)
    {
        memcpy(&(drifterField), &(payload[ttSize]), sizeof(drifterField));
        ttSize += sizeof(drifterField);

        IO_ConvertIpAddressToString(drifterField.drifterAddr, addrString);
        printf("        destAddr = %s\n", addrString);
        IO_ConvertIpAddressToString(drifterField.nextHopAddr, addrString);
        printf("        nextHop = %s\n", addrString);
        printf("        distance = %d\n", drifterField.distance);
        printf("        sequenceNumber = %d\n\n",
               drifterField.sequenceNumber);
    }

    ERROR_Assert ((unsigned)ttSize ==
                  (landmarkTableHeaderSize +
                   headerField.numLandmark * sizeof(landmarkField) +
                   headerField.numDrifter * sizeof(drifterField)),
                  "Packet corrupted!");
}

// /**
// FUNCTION   :: FsrlSameSubnet
// LAYER      :: NETWORK
// PURPOSE    :: See if two addresses are in the same subnet
// PARAMETERS ::
// + landmarkAddr : NodeAddress : The first node address
// + destAddr : NodeAddress : The second node address
// + numHostBits : int : Host bits for indicating address structure
// RETURN     :: BOOL : TRUE for in the same subnet, FALSE for not
// **/

static
BOOL FsrlSameSubnet(NodeAddress landmarkAddr,
                    NodeAddress destAddr,
                    int numHostBits)
{
    NodeAddress landmarkNetworkAddr;
    NodeAddress destNetworkAddr;

    landmarkNetworkAddr = MaskIpAddress(
                              landmarkAddr,
                              ConvertNumHostBitsToSubnetMask(numHostBits));

    destNetworkAddr = MaskIpAddress(
                          destAddr,
                          ConvertNumHostBitsToSubnetMask(numHostBits));

    return (landmarkNetworkAddr == destNetworkAddr);
}


// /**
// FUNCTION   :: FsrlIsDrifter
// LAYER      :: NETWORK
// PURPOSE    :: Determine if we are a drifter.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// + lower : short : lower bound of scope
// + upper : short : upper bound of scope
// RETURN     :: BOOL : TRUE for is drifter, FALSE for not drifter
// **/

static
BOOL FsrlIsDrifter(Node* node, FsrlData* fisheye,
                   short lower, short upper,
                   int interfaceIndex)
{
    BOOL isDrifter = FALSE;
    BOOL found = FALSE;
    FsrlLandmarkTableRow* tmp;
    NodeAddress lmAddr;
    int dist = FISHEYE_INFINITY;

    if (FsrlDebugDrifter(node))
    {
        printf("    checking if we are a drifter\n");
    }

    // No Landmark yet
    if (fisheye->landmarkTable.row == NULL)
    {
        if (FsrlDebugDrifter(node))
        {
            printf("        landmark table empty, so not drifter\n");
        }

        return isDrifter;
    }
    else
    {
        tmp = fisheye->landmarkTable.row;

        while (!found && tmp != NULL)
        {
            if (FsrlSameSubnet(
                    tmp->destAddr,
                    NetworkIpGetInterfaceAddress(node, interfaceIndex),
                    NetworkIpGetInterfaceNumHostBits(
                    node, interfaceIndex)))
            {
                found = TRUE;
                lmAddr = tmp->destAddr;
                dist = tmp->distance;

                if (FsrlDebugDrifter(node))
                {
                    char addrString[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(tmp->destAddr, addrString);
                    printf("        landmark is %s\n", addrString);
                    printf("        scope is %d\n", upper);
                    printf("        distance is %d\n", dist);
                }

                break;
            }

            tmp = tmp->next;
        }
    }

    // If no landmark is found for our subnet, then do not consider
    // ourself as a drifter.
    if (!found)
    {
        if (FsrlDebugDrifter(node))
        {
            printf("        not in landmark table, so not drifter\n");
        }

        return isDrifter;
    }

    ERROR_Assert(dist <= FISHEYE_INFINITY,
                 "Invalid landmark table entry!");

    // We are a drifter only if we are not within the landmark scope
    // and distance is not INFINITY.

    if ((dist >= lower && dist <= (upper + 1)) ||
        (dist == FISHEYE_INFINITY))
    {
        if (FsrlDebugDrifter(node))
        {
            printf("        within scope or INFINITY, so not drifter\n");
        }

        isDrifter = FALSE;
    }
    else
    {
        if (FsrlDebugDrifter(node))
        {
            printf("        out of scope, so we are a drifter\n");
        }

        isDrifter = TRUE;
    }

    return isDrifter;
}


// /**
// FUNCTION   :: FsrlCheckDrifterTable
// LAYER      :: NETWORK
// PURPOSE    :: Check if a node exists in drifter table
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// + nodeAddr : NodeAddress : node address for checking
// RETURN     :: BOOL : TRUE for in drifter table, FALSE for not in table
// **/

static
BOOL FsrlCheckDrifterTable(Node*node,
                           FsrlData* fisheye,
                           NodeAddress nodeAddr)
{
    FsrlDrifterTableRow* tmp;

    if (FsrlDebugDrifter(node))
    {
        printf("    checking if we are in drifter table\n");
    }

    // No drifter yet.
    if (fisheye->drifterTable.row == NULL)
    {
        if (FsrlDebugDrifter(node))
        {
            printf("        table empty\n");
        }

        return FALSE;
    }
    else
    {
        tmp = fisheye->drifterTable.row;

        while (tmp != NULL)
        {
            if (tmp->destAddr == nodeAddr)
            {
                if (FsrlDebugDrifter(node))
                {
                    printf("        we are in drifter table\n");
                }

                return TRUE;
            }

            tmp = tmp->next;
        }
    }

    if (FsrlDebugDrifter(node))
    {
        printf("        we are not in drifter table\n");
    }

    return FALSE;
}


// /**
// FUNCTION   :: FsrlUpdateDrifterTable
// LAYER      :: NETWORK
// PURPOSE    :: Update or insert node into drifter table
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + destAddr : NodeAddress : Drifter node's address
// + nextHop : NodeAddress : Next hop to this drifter node
// + outgoingInterface : int : interface index
// + distance : int : distance to this drifter node
// + comingSeq : int : sequence number
// RETURN     :: void : NULL
// **/

static
void FsrlUpdateDrifterTable(Node* node,
                            NodeAddress destAddr,
                            NodeAddress nextHop,
                            int outgoingInterface,
                            int distance,
                            int comingSeq)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlDrifterTableRow* tmp;
    FsrlDrifterTableRow* prev = NULL;
    BOOL found = FALSE;

    if (FsrlDebugDrifter(node))
    {
        char addrString[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(destAddr, addrString);
        printf("Node %d updating drifter %s\n", node->nodeId, addrString);
        printf("    drifter table before updating...%s\n", addrString);

        FsrlPrintDrifterTable(node, fisheye);
    }

    // first one
    if (fisheye->drifterTable.row == NULL)
    {
        fisheye->drifterTable.row = (FsrlDrifterTableRow*)
            MEM_malloc(sizeof(FsrlDrifterTableRow));

        if (FsrlDebugDrifter(node))
        {
            printf("        table empty, so add\n");
        }

        fisheye->drifterTable.row->destAddr = destAddr;
        fisheye->drifterTable.row->nextHop = nextHop;
        fisheye->drifterTable.row->outgoingInterface = outgoingInterface;
        fisheye->drifterTable.row->distance = distance;
        fisheye->drifterTable.row->sequenceNumber = comingSeq;
        fisheye->drifterTable.row->lastModified = node->getNodeTime();
        fisheye->drifterTable.row->next = NULL;
        fisheye->drifterTable.size = 1;
    }
    else
    {
        tmp = fisheye->drifterTable.row;

        while (!found && tmp != NULL)
        {
            prev = tmp;

            if (tmp->destAddr == destAddr)
            {
                found = TRUE;

                if (FsrlDebugDrifter(node))
                {
                    printf("        entry exists, so update entry\n");
                }

                if (comingSeq > tmp->sequenceNumber)
                {
                    tmp->nextHop = nextHop;
                    tmp->outgoingInterface = outgoingInterface;
                    tmp->distance = distance;
                    tmp->sequenceNumber = comingSeq;
                    tmp->lastModified = node->getNodeTime();
                }
                // Need this in FindShortestPath
                else if (comingSeq == tmp->sequenceNumber &&
                         tmp->distance >= distance)
                {
                    tmp->nextHop = nextHop;
                    tmp->outgoingInterface = outgoingInterface;
                    tmp->distance = distance;
                    tmp->lastModified = node->getNodeTime();
                }
            }

            tmp = tmp->next;
        }

        if (!found)
        {
            tmp = (FsrlDrifterTableRow*)
                  MEM_malloc(sizeof(FsrlDrifterTableRow));

            if (FsrlDebugDrifter(node))
            {
                printf("        entry not found, so add entry\n");
            }

            prev->next = tmp;
            tmp->next = NULL;
            tmp->destAddr = destAddr;
            tmp->nextHop = nextHop;
            tmp->outgoingInterface = outgoingInterface;
            tmp->distance = distance;
            tmp->sequenceNumber = comingSeq;
            tmp->lastModified = node->getNodeTime();

            fisheye->drifterTable.size++;
        }
    }

    if (FsrlDebugDrifter(node))
    {
        printf("    drifter table after updating...\n");

        FsrlPrintDrifterTable(node, fisheye);
    }
}


// /**
// FUNCTION   :: FsrlDeleteFromDrifterTable
// LAYER      :: NETWORK
// PURPOSE    :: Delete node from drifter table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + destAddr : NodeAddress : Drifter node's address
// RETURN     :: void : NULL
// **/

static
void FsrlDeleteFromDrifterTable(Node* node, NodeAddress destAddr)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);

    FsrlDrifterTableRow* tmp;
    FsrlDrifterTableRow* toFree;
    BOOL found = FALSE;
    int dist;

    if (FsrlDebugDrifter(node))
    {
        char addrString[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(destAddr, addrString);
        printf("    wanting to delete drifter %s\n", addrString);
    }

    // empty
    if (fisheye->drifterTable.row == NULL)
    {
        if (FsrlDebugDrifter(node))
        {
            printf("        drifter table is empty, so no deleting\n");
        }

        return;
    }
    else if (fisheye->drifterTable.row->nextHop == destAddr)
    {
        if (FsrlDebugDrifter(node))
        {
            printf("        drifter found, so delete\n");
        }

        toFree = fisheye->drifterTable.row;
        fisheye->drifterTable.row = toFree->next;
        dist = toFree->distance;

        MEM_free(toFree);

        fisheye->drifterTable.size--;
    }
    else
    {
        tmp = fisheye->drifterTable.row;

        while (!found && tmp->next != NULL)
        {
            if (tmp->next->nextHop == destAddr)
            {
                found = TRUE;

                if (FsrlDebugDrifter(node))
                {
                    printf("        drifter found, so delete\n");
                }

                toFree = tmp->next;
                tmp->next = toFree->next;
                dist = toFree->distance;

                MEM_free(toFree);
                --fisheye->drifterTable.size;
            }

            tmp = tmp->next;
        }
    }
}


// /**
// FUNCTION   :: FsrlCheckExpiredEntryInDrifterTable
// LAYER      :: NETWORK
// PURPOSE    :: Check for expired drifter entries in drifter table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlCheckExpiredEntryInDrifterTable(
         Node* node,
         FsrlData* fisheye)
{

    FsrlDrifterTableRow* tmp;
    FsrlDrifterTableRow* prev = NULL;
    FsrlDrifterTableRow* toFree;

    if (FsrlDebugDrifter(node))
    {
        printf("Node %d checking for drifter timeout\n", node->nodeId);
    }

    tmp = prev = fisheye->drifterTable.row;

    while (tmp != NULL)
    {
        toFree = NULL;

        if ((node->getNodeTime() - tmp->lastModified) >
            fisheye->parameter.drifterMaxAge)
        {
            if (FsrlDebugDrifter(node))
            {
                char addrString[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(tmp->destAddr, addrString);
                printf("    drifter %s expired, so remove from table\n",
                       addrString);
            }

            if (tmp == fisheye->drifterTable.row)
            {
                fisheye->drifterTable.row = tmp->next;
                prev = fisheye->drifterTable.row;
            }
            else
            {
                prev->next = tmp->next;
            }

            toFree = tmp;
            tmp = tmp->next;

            MEM_free(toFree);

            fisheye->drifterTable.size --;
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
        }
    }
}


// /**
// FUNCTION   :: FsrlFindRowInLandmarkTable
// LAYER      :: NETWORK
// PURPOSE    :: Find one entry in landmark table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// + nodeAddr : NodeAddress : landmark node address
// RETURN     :: FsrlLandmarkTableRow* : pointer to the entry.
// **/

static
FsrlLandmarkTableRow* FsrlFindRowInLandmarkTable(
                          FsrlData* fisheye,
                          NodeAddress nodeAddr)
{
    FsrlLandmarkTableRow* row = NULL;
    FsrlLandmarkTableRow* retRow = NULL;
    BOOL found;

    found = FALSE;
    row = fisheye->landmarkTable.row;

    while (row != NULL && !found)
    {
        if (nodeAddr == row->destAddr)
        {
            found = TRUE;
            retRow = row;
        }

        row = row->next;
    }

    return retRow;
}


// /**
// FUNCTION   :: FsrlUpdateLandmarkMemberCountInLandmarkTable
// LAYER      :: NETWORK
// PURPOSE    :: Update our own landmark member count in landmark table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// + numLandmarkMember : int : Number of members
// RETURN     :: void : NULL
// **/

static
void FsrlUpdateLandmarkMemberCountInLandmarkTable(
         Node* node,
         FsrlData* fisheye,
         int numLandmarkMember,
         int interfaceIndex)
{
    FsrlLandmarkTableRow* lmDvrow;

    lmDvrow = FsrlFindRowInLandmarkTable(fisheye,
                    NetworkIpGetInterfaceAddress(node, interfaceIndex));

    // If we are the current landmark (meaning that we exist in
    // the landmark table), then update our member count.
    // Otherwise, don't do anything.

    if (lmDvrow)
    {
        lmDvrow->numLandmarkMember = numLandmarkMember;
    }
}


// /**
// FUNCTION   :: FsrlLandmarkWinnerUpdated
// LAYER      :: NETWORK
// PURPOSE    :: See if we need to update who's the landmark winner.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + currentLandmark : FsrlLandmarkTableRow* : current landmark entry
// + destAddr : NodeAddress : nodes competing for landmark
// + memberNum : int : Number of members of the competing node
// + A : double : parameter for landmark election alg.
// + N : double : parameter for landmark election alg.
// RETURN     :: BOOL : TRUE, needs to update, FALSE, keep current landmark
// **/

static
BOOL FsrlLandmarkWinnerUpdated(
         Node* node,
         FsrlLandmarkTableRow* currentLandmark,
         NodeAddress destAddr,
         int memberNum,
         double A,
         int N)
{
    BOOL change = FALSE;

    if (FsrlDebugLandmarkElection(node))
    {
        char addrString[MAX_STRING_LENGTH];

        printf("    checking winner...\n");
        printf("        memberNum = %d\n", memberNum);
        printf("        currentLandmark->numLandmarkMember = %d\n",
               currentLandmark->numLandmarkMember);

        IO_ConvertIpAddressToString(destAddr, addrString);
        printf("        destAddr = %s\n", addrString);
        IO_ConvertIpAddressToString(currentLandmark->destAddr, addrString);
        printf("        currentLandmark->destAddr = %s\n", addrString);
    }

    #ifndef HYSTERESIS
    {
        char addrString1[MAX_STRING_LENGTH];
        char addrString2[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(currentLandmark->destAddr, addrString1);
        IO_ConvertIpAddressToString(destAddr, addrString2);

        if (memberNum > currentLandmark->numLandmarkMember)
        {
            if (FsrlDebugLandmarkElection(node))
            {
                printf("    replacing LM node %s with node %s due to "
                       "more members\n",
                       addrString1, addrString2);
            }

            change = TRUE;
        }
        else
        {
            if ((memberNum == currentLandmark->numLandmarkMember) &&
                (destAddr < currentLandmark->destAddr))
            {
                if (FsrlDebugLandmarkElection(node))
                {
                    printf("    replacing LM node %s with node %s\n"
                           "    due to tie breaker because of same number "
                           "of members\n",
                           addrString1, addrString2);
                }

                change = TRUE;
            }
            else
            {
                if (FsrlDebugLandmarkElection(node))
                {
                    printf("    keeping node %s as LM node due "
                           "to tie breaker\n", addrString1);
                }
            }
        }
    }
    #endif

    #ifdef HYSTERESIS
    {
        char addrString1[MAX_STRING_LENGTH];
        char addrString2[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(currentLandmark->destAddr, addrString1);
        IO_ConvertIpAddressToString(destAddr, addrString2);

        if ((memberNum >= A * currentLandmark->numLandmarkMember) ||
            (currentLandmark->numLandmarkMember < N))
        {
            if (FsrlDebugLandmarkElection(node))
            {
                printf("    replacing LM node %s with node %s due to "
                       "more members\n",
                       addrString1, addrString2);
            }

            change = TRUE;
        }
        else if (memberNum >= (1 / A) *
                 currentLandmark->numLandmarkMember &&
                 memberNum < A * currentLandmark->numLandmarkMember)
        {
            if (memberNum > currentLandmark->numLandmarkMember)
            {
                if (FsrlDebugLandmarkElection(node))
                {
                    printf("    replacing LM node %s with node %s due to "
                           "more members\n",
                           addrString1, addrString2);
                }

                change = TRUE;
            }
            else if (memberNum == currentLandmark->numLandmarkMember &&
                     destAddr < currentLandmark->destAddr)
            {
                if (FsrlDebugLandmarkElection(node))
                {
                    printf("    replacing LM node %s with node %s\n"
                           "    due to tie breaker because of same number\n"
                           "    of members\n",
                           addrString1, addrString2);
                }

                change = TRUE;
            }
            else
            {
                if (FsrlDebugLandmarkElection(node))
                {
                    printf("    keeping node %s as LM node due "
                           "to tie breaker\n", addrString1);
                }
            }
        }
        else
        {
            if (FsrlDebugLandmarkElection(node))
            {
                printf("    keeping node %s as LM node\n", addrString1);
            }
        }
    }
    #endif

    return change;
}


// /**
// FUNCTION   :: FsrlUpdateLandmarkTable
// LAYER      :: NETWORK
// PURPOSE    :: Update entry in landmark table via landmark election.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + destAddr : NodeAddress : landmark node address
// + nextHop : NodeAddress : next hoop to the landmark node
// + outgoingInterface : int : interface index
// + distance : int : distance to landmark node
// + sequenceNumber : int : sequence number
// RETURN     :: void : NULL
// **/

static
void FsrlUpdateLandmarkTable(Node* node,
                             NodeAddress destAddr,
                             NodeAddress nextHop,
                             int outgoingInterface,
                             int numLandmarkMember,
                             int distance,
                             int sequenceNumber,
                             int interfaceIndex)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlLandmarkTableRow* row;
    FsrlLandmarkTableRow* prev;
    BOOL change = FALSE;
    BOOL found = FALSE;
    double A;
    int N;
    int index;

    if (distance > FISHEYE_INFINITY)
    {
        printf("Node %d got bad distance of %d\n", node->nodeId, distance);
        ERROR_Assert(FALSE, "Invalid distance!");
    }

    ERROR_Assert(fisheye != NULL, "Memory Error!");

    A = fisheye->parameter.alpha;
    N = fisheye->parameter.minMemberThreshold;

    // If the node's same subnet neighbors are less than the threshold,
    // then the node cannot be a landmark.
    if (numLandmarkMember < N)
    {
        if (FsrlDebugLandmarkElection(node))
        {
            printf("        number of member (%d) is below "
                   "threshold (%d)\n",
                   numLandmarkMember, N);
        }

        return;
    }

    if (FsrlDebugLandmarkElection(node))
    {
        printf("    before landmark election\n");
        FsrlPrintLandmarkTable(node, fisheye);
    }

    ERROR_Assert(distance <= FISHEYE_INFINITY, "Wrong distance!");

    if (fisheye->landmarkTable.row == NULL && distance < FISHEYE_INFINITY)
    {
        if (FsrlDebugLandmarkElection(node))
        {
            printf("    landmark table is empty, so add entry\n");
        }

        change = TRUE;

        fisheye->landmarkTable.row = (FsrlLandmarkTableRow*)
            MEM_malloc(sizeof(FsrlLandmarkTableRow));

        fisheye->landmarkTable.row->destAddr = destAddr;
        fisheye->landmarkTable.row->nextHop = nextHop;
        fisheye->landmarkTable.row->outgoingInterface = outgoingInterface;
        fisheye->landmarkTable.row->numLandmarkMember = numLandmarkMember;
        fisheye->landmarkTable.row->distance = distance;
        fisheye->landmarkTable.row->sequenceNumber = sequenceNumber;
        fisheye->landmarkTable.row->timestamp = node->getNodeTime();

        fisheye->landmarkTable.row->prev = NULL;
        fisheye->landmarkTable.row->next = NULL;
        fisheye->landmarkTable.size = 1;

        // Update our landmark status.
        if (destAddr == NetworkIpGetInterfaceAddress(node, interfaceIndex))
        {
            fisheye->isLandmark[interfaceIndex] = TRUE;
        }
    }
    else
    {
        row = fisheye->landmarkTable.row;
        prev = row;
        while (row != NULL && !found)
        {
            prev = row;

            if (row->destAddr == destAddr)
            {
                found = TRUE;

                if (FsrlDebugLandmarkElection(node))
                {
                    char addrString[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(destAddr, addrString);

                    printf("    found entry %s\n", addrString);

                    if (row->sequenceNumber < sequenceNumber &&
                        distance < FISHEYE_INFINITY)
                    {
                        printf("        newer sequence number\n");
                    }
                    else if (row->sequenceNumber == sequenceNumber &&
                             row->distance > distance)
                    {
                        printf("        same sequence number, "
                               "but lower distance\n");
                    }
                    else if (row->sequenceNumber < sequenceNumber &&
                             distance == FISHEYE_INFINITY &&
                             row->nextHop == nextHop)
                    {
                        printf("        newer sequence number and "
                               "same next hop\n");
                    }
                    else
                    {
                        printf("        no need to update\n");
                    }
                }

                // Update if:
                // 1. New update has higher sequence number and
                //    valid distance.
                // 2. New update has same sequence number but smaller
                //    distance.
                // 3. New update has higher sequence number, distance
                //    is unreachable and next hop matches entry in table.

                if ((row->sequenceNumber < sequenceNumber &&
                     distance <  FISHEYE_INFINITY) ||
                    (row->sequenceNumber == sequenceNumber &&
                     row->distance > distance) ||
                    (row->sequenceNumber < sequenceNumber &&
                     distance ==  FISHEYE_INFINITY &&
                     row->nextHop == nextHop))
                {
                    if (FsrlDebugLandmarkElection(node))
                    {
                        char addrString[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(destAddr, addrString);

                        printf("    already landmark, so just "
                               "update info\n");
                    }

                    row->nextHop = nextHop;
                    row->outgoingInterface = outgoingInterface;
                    row->numLandmarkMember = numLandmarkMember;
                    row->distance = distance;
                    row->sequenceNumber = sequenceNumber;
                    row->timestamp = node->getNodeTime();
                }
            }
            // If landmark node is not the same but in the same subnet,
            // then we need to determine winner.
            else if (FsrlSameSubnet(row->destAddr,
                                    destAddr,
                                    NetworkIpGetInterfaceNumHostBits(
                                        node, interfaceIndex)))
            {
                found = TRUE;

                if (FsrlDebugLandmarkElection(node))
                {
                    printf("    not same landmark, but same subnet, so "
                           "need to check winner\n");
                }

                if ((distance < FISHEYE_INFINITY &&
                     FsrlLandmarkWinnerUpdated(node,
                                               row,
                                               destAddr,
                                               numLandmarkMember,
                                               A,
                                               N)) ||
                    (row->distance == FISHEYE_INFINITY))
                {
                    // Update our status if we are the landmark or not.

                    if (destAddr ==
                        NetworkIpGetInterfaceAddress(node, interfaceIndex))
                    {
                        // We are the landmark.
                        fisheye->isLandmark[interfaceIndex] = TRUE;
                    }
                    for (index = 0; index < node->numberInterfaces;
                            index++)
                    {
                        if (row->destAddr ==
                                NetworkIpGetInterfaceAddress(node, index))
                        {
                           // We are going to be replaced as the landmark,
                           fisheye->isLandmark[index] = FALSE;

                        }

                    }

                    change = TRUE;
                    row->destAddr = destAddr;
                    row->nextHop = nextHop;
                    row->outgoingInterface = outgoingInterface;
                    row->numLandmarkMember = numLandmarkMember;
                    row->distance = distance;
                    row->sequenceNumber  = sequenceNumber;
                    row->timestamp = node->getNodeTime();
                }
            }

            row = row->next;
        }

        if ((!found) && distance < FISHEYE_INFINITY)
        {
            // If we are here, then the landmark node must be of
            // a subnet that's not already claimed by other nodes.
            // Therefore, we must add this landmark.

            if (FsrlDebugLandmarkElection(node))
            {
                printf("    landmark part of subnet not yet claimed, "
                       "so add entry\n");
            }

            row = fisheye->landmarkTable.row;

            while (row != NULL)
            {
                prev = row;
                row = row->next;
            }

            change = TRUE;

            row = (FsrlLandmarkTableRow*)
                  MEM_malloc(sizeof(FsrlLandmarkTableRow));

            prev->next = row;
            row->destAddr = destAddr;
            row->nextHop = nextHop;
            row->outgoingInterface = outgoingInterface;
            row->numLandmarkMember = numLandmarkMember;
            row->distance = distance;
            row->sequenceNumber = sequenceNumber;
            row->timestamp = node->getNodeTime();

            row->prev = prev;
            row->next = NULL;

            fisheye->landmarkTable.size ++;

            // chk the numLandmarks within limit
            ERROR_Assert(fisheye->landmarkTable.size <= 255,
                         "Number of landmarks exceed limit!!!");

            if (destAddr == NetworkIpGetInterfaceAddress(
                            node, interfaceIndex))
            {
                fisheye->isLandmark[interfaceIndex] = TRUE;
            }
        }
    }

    if (FsrlDebugLandmarkElection(node))
    {
        printf("    node%u after landmark election\n", node->nodeId);

        FsrlPrintLandmarkTable(node, fisheye);
    }
}


// /**
// FUNCTION   :: FsrlCheckExpiredEntryInLandmarkTable
// LAYER      :: NETWORK
// PURPOSE    :: Check for expired entries in landmark table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + needDelete : BOOL : Whether need to delete the expired entry.
// RETURN     :: void : NULL
// **/

static
void FsrlCheckExpiredEntryInLandmarkTable(Node* node, BOOL needDelete)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlLandmarkTableRow* tmpRow;
    FsrlLandmarkTableRow* prevRow;
    FsrlLandmarkTableRow* nextRow;

    for (int i = 0; i < node->numberInterfaces; i++)
    {
        tmpRow = fisheye->landmarkTable.row;
        prevRow = fisheye->landmarkTable.row;

        if (FsrlDebugLandmark(node))
        {
            printf("Node %d checking for expired entries"
                "in landmark table\n",
                   node->nodeId);
        }

        while (tmpRow != NULL)
        {
            // Used to determine which is the next row to search after
            // deleting an entry.
            nextRow = tmpRow->next;

            if ((tmpRow->timestamp + fisheye->parameter.landmarkMaxAge) <
                node->getNodeTime() &&
                (tmpRow->destAddr !=
                 NetworkIpGetInterfaceAddress(node, i)))
            {
                if (FsrlDebugLandmark(node))
                {
                    char addrString[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(tmpRow->destAddr,
                        addrString);
                    printf("    node%u expires %s from landmark table\n",
                           node->nodeId,
                           addrString);
                }

                if (!needDelete)
                {
                    if (tmpRow->distance != FISHEYE_INFINITY)
                    {
                        tmpRow->distance = FISHEYE_INFINITY;
                        tmpRow->sequenceNumber ++;
                    }

                    prevRow = tmpRow;
                }
                else
                {
                    // Delete at front
                    if (tmpRow == fisheye->landmarkTable.row)
                    {
                        fisheye->landmarkTable.row = tmpRow->next;

                        // Case where only one entry in table.
                        if (tmpRow->next != NULL)
                        {
                            fisheye->landmarkTable.row->prev = NULL;
                        }
                    }
                    // Delete at middle
                    else if (tmpRow->next != NULL)
                    {
                        prevRow->next = tmpRow->next;
                        tmpRow->next->prev = prevRow;
                    }
                    // Delete at end
                    else if (tmpRow->next == NULL)
                    {
                        prevRow->next = tmpRow->next;
                    }
                    else
                    {
                        ERROR_Assert(0, "Wrong landmar table!");
                    }

                    MEM_free(tmpRow);
                    fisheye->landmarkTable.size --;

                    // Since we deleted a landmark entry, check to see if we can
                    // claim ourself as the landmark.

                    FsrlUpdateLandmarkTable(
                        node,
                        NetworkIpGetInterfaceAddress(node, i),
                        NetworkIpGetInterfaceAddress(node, i),
                        -1,
                        fisheye->numLandmarkMember[i],
                        0,
                        fisheye->landmarkTable.ownSequenceNumber,
                        i);

                }
            }
            else
            {
                // If we deleted an entry, then prevRow stays the same.
                // If entry is not deleted, then prevRow is tmpRow.
                prevRow = tmpRow;
            }

            tmpRow = nextRow;
        }
    }
}


// /**
// FUNCTION   :: FsrlInvalidateEntryInLandmarkTable
// LAYER      :: NETWORK
// PURPOSE    :: Set a landmark entry's distance metric to infinity.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + nodeAddr : NodeAddress : Node address of the landmark entry
// RETURN     :: void : NULL
// **/

static
void FsrlInvalidateEntryInLandmarkTable(Node* node,
                                        NodeAddress nodeAddr)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlLandmarkTableRow* tmpRow;

    tmpRow = fisheye->landmarkTable.row;

    while (tmpRow != NULL)
    {
        if (tmpRow->nextHop == nodeAddr)
        {
            if (FsrlDebugLandmark(node))
            {
                char addrString[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(tmpRow->destAddr, addrString);
                printf("    setting %s's metric to INFINITY\n", addrString);
            }

            tmpRow->distance = FISHEYE_INFINITY;
            tmpRow->sequenceNumber ++;
        }

        tmpRow = tmpRow->next;
    }
}


// /**
// FUNCTION   :: FsrlDeleteInfinityEntriesFromLandmarkTable
// LAYER      :: NETWORK
// PURPOSE    :: Delete entries from landmark table if distance is infinity.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: void : NULL
// **/

static
void FsrlDeleteInfinityEntriesFromLandmarkTable(Node* node)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlLandmarkTableRow* currentRow = NULL;
    FsrlLandmarkTableRow* tmpRow = NULL;
    FsrlLandmarkTableRow* prevRow = NULL;
    BOOL deleted = FALSE;

    currentRow = fisheye->landmarkTable.row;
    prevRow = currentRow;

    while (currentRow != NULL)
    {
        if ((currentRow->distance == FISHEYE_INFINITY) &&
            ((currentRow->timestamp +
            fisheye->parameter.landmarkMaxAge +
            fisheye->parameter.landmarkUpdateInterval) <
            node->getNodeTime()))
        {
            if (FsrlDebugLandmark(node))
            {
                char addrString[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(currentRow->destAddr,
                                            addrString);
                printf("    node%u removes %s from landmark table "
                       "(due to infinity distance)\n",
                       node->nodeId,
                       addrString);
            }

            deleted = TRUE;

            // Delete at front
            if (currentRow == fisheye->landmarkTable.row)
            {
                fisheye->landmarkTable.row = currentRow->next;

                if (fisheye->landmarkTable.row != NULL)
                {
                    fisheye->landmarkTable.row->prev = NULL;
                    prevRow = NULL;
                }
            }
            // Delete at middle
            else if (currentRow->next != NULL)
            {
                prevRow->next = currentRow->next;
                currentRow->next->prev = prevRow;
            }
            // Delete at end
            else if (currentRow->next == NULL)
            {
                prevRow->next = currentRow->next;
            }
            else
            {
                ERROR_Assert(0, "Bad landmark routing table!");
            }

            tmpRow = currentRow;
            currentRow = currentRow->next;

            MEM_free(tmpRow);

            fisheye->landmarkTable.size --;

            continue;
        }

        prevRow = currentRow;
        currentRow = currentRow->next;
    }

    if (deleted)
    {
        // Since we just deleted one or more landmarks, there's a
        // possibility that we can become the new landmark.  Thus, we need
        // to check...
        for (int i = 0; i < node->numberInterfaces; i++)
        {
            FsrlUpdateLandmarkTable(
                node,
                NetworkIpGetInterfaceAddress(node, i),
                NetworkIpGetInterfaceAddress(node, i),
                -1,
                fisheye->numLandmarkMember[i],
                0,
                fisheye->landmarkTable.ownSequenceNumber,
                i);
        }

    }
}


// /**
// FUNCTION   :: FsrlDeleteFromLandmarkTable
// LAYER      :: NETWORK
// PURPOSE    :: Delete node from landmark table if it exists in table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + nodeAddr : NodeAddress : Landmark node to be deleted
// RETURN     :: void : NULL
// **/

static
void FsrlDeleteFromLandmarkTable(Node* node, NodeAddress nodeAddr,
                                 int interfaceIndex)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlLandmarkTableRow* tmpRow;
    FsrlLandmarkTableRow* prevRow;

    tmpRow = fisheye->landmarkTable.row;
    prevRow = tmpRow;

    while (tmpRow != NULL)
    {
        if (tmpRow->destAddr == nodeAddr)
        {
            if (FsrlDebugLandmark(node))
            {
                char addrString[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(tmpRow->destAddr, addrString);
                printf("    node%u removing %s from landmark table\n",
                       node->nodeId,
                       addrString);
            }

            // Delete at front
            if (tmpRow == fisheye->landmarkTable.row)
            {
                fisheye->landmarkTable.row = tmpRow->next;

                if (fisheye->landmarkTable.row != NULL)
                {
                    fisheye->landmarkTable.row->prev = NULL;
                    prevRow = NULL;
                }
            }
            // Delete at middle
            else if (tmpRow->next != NULL)
            {
                prevRow->next = tmpRow->next;
                tmpRow->next->prev = prevRow;
            }
            // Delete at end
            else if (tmpRow->next == NULL)
            {
                prevRow->next = tmpRow->next;
            }
            else
            {
                ERROR_Assert(0, "Bad landmark routing table!");
            }

            if (FsrlDebugLandmark(node))
            {
                printf("    updating landmark table due to landmark "
                       "node timeout\n");
            }

            MEM_free(tmpRow);

            fisheye->landmarkTable.size --;

            // Since we just deleted a landmark, there's a possibility
            // that we can become the new landmark.  Thus, we need
            // to check...

            for (int i = 0; i < node->numberInterfaces; i++)
            {
                FsrlUpdateLandmarkTable(
                    node,
                    NetworkIpGetInterfaceAddress(node, i),
                    NetworkIpGetInterfaceAddress(node, i),
                    -1,
                    fisheye->numLandmarkMember[i],
                    0,
                    fisheye->landmarkTable.ownSequenceNumber,
                    i);
            }

            return;
        }

        prevRow = tmpRow;
        tmpRow = tmpRow->next;
    }
}


// /**
// FUNCTION   :: FsrlFindRowInTopologyTable
// LAYER      :: NETWORK
// PURPOSE    :: Get one entry in topology table.
// PARAMETERS ::
// + nodeAddr : NodeAddress : node address
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: FisheyeTopologyTableRow* : Pointer to the entry
// **/

static
FisheyeTopologyTableRow* FsrlFindRowInTopologyTable(
                             NodeAddress nodeAddr,
                             FsrlData* fisheye)
{
    FisheyeTopologyTableRow* iterator = fisheye->topologyTable.first;

    while (iterator != NULL)
    {
        if (nodeAddr == iterator->linkStateNode)
        {
            return iterator;
        }

        iterator = iterator->next;
    }

    return NULL;
}


// /**
// FUNCTION   :: FsrlFindNeighbourInTopologyTable
// LAYER      :: NETWORK
// PURPOSE    :: Return the Topology list row whose neighbour matches with
//              the nodes Ip address
// PARAMETERS ::
// + nodeAddr : NodeAddress : node address
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: FisheyeTopologyTableRow* : Pointer to the entry
// **/

static
FisheyeTopologyTableRow* FsrlFindNeighbourInTopologyTable(
                             NodeAddress nodeAddr,
                             FsrlData* fisheye, Node* node)
{
    FisheyeTopologyTableRow* iterator = fisheye->topologyTable.first;
    FsrlNeighborListItem* tmp = NULL;
    while (iterator != NULL)
    {
        if (nodeAddr == iterator->linkStateNode)
        {
            tmp = iterator->neighborInfo->list;
            while (tmp != NULL)
            {
                if (NetworkIpIsMyIP(node,tmp->nodeAddr))
                {
                    FisheyeTopologyTableRow* iterator2 = fisheye->
                                                    topologyTable.first;
                    while (iterator2 != NULL)
                    {
                      if (tmp->nodeAddr == iterator2->linkStateNode)
                      {
                          return iterator2;
                      }
                      iterator2 = iterator2->next;
                    }
                }
                tmp = tmp->next;
            }
        }
        iterator = iterator->next;
    }
    return NULL;
}

// /**
// FUNCTION   :: FsrlCountLandmarkMember
// LAYER      :: NETWORK
// PURPOSE    :: Count how many group members in scope we have
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: int : Number of members
// **/

static
int FsrlCountLandmarkMember(Node* node, FsrlData* fisheye,
                            int interfaceIndex)
{
    FisheyeTopologyTableRow* iterator = fisheye->topologyTable.first;
    int numMember = 0;

    while (iterator != NULL)
    {

        if (FsrlSameSubnet(iterator->linkStateNode,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                NetworkIpGetInterfaceNumHostBits(node, interfaceIndex)))
        {
            numMember ++;
        }

        iterator = iterator->next;
    }

    return numMember;
}


// /**
// FUNCTION   :: FsrlCheckExpiredEntryInTopologyTable
// LAYER      :: NETWORK
// PURPOSE    :: Check for expired entries in topology table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlCheckExpiredEntryInTopologyTable(Node* node,
                                          FsrlData* fisheye)
{
    FisheyeTopologyTableRow* iterator = fisheye->topologyTable.first;
    FisheyeTopologyTableRow* tmp;
    FisheyeTopologyTableRow* prev;

    FsrlNeighborListItem* current;
    FsrlNeighborListItem* currentNext;

    clocktype now = node->getNodeTime();

    prev = iterator;

    if (FsrlDebug(node))
    {
        printf("Node %d checking for expired entries in topology table\n",
               node->nodeId);
    }

    if (FsrlDebugInDetail(node))
    {
        printf("    before topology table checking for expired entries\n");

        FsrlPrintTopologyTable(node, fisheye);
    }

    while (iterator != NULL)
    {
        if (iterator->timestamp + fisheye->parameter.fisheyeMaxAge < now &&
            !NetworkIpIsMyIP(node, iterator->linkStateNode))
        {
            if (iterator->removalTimestamp == 0)
            {
                iterator->removalTimestamp = now;
                break;
            }
            // Not time to remove from table yet.  This time period should
            // be long enough so that this linkstate in the routing domain
            // is flushed out.
            else if (iterator->removalTimestamp +
                     fisheye->parameter.fisheyeMaxAge >= now)
            {
                break;
            }

            if (FsrlDebug(node))
            {
                char addrStr[MAX_STRING_LENGTH];
                char clockStr[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(iterator->linkStateNode,
                                            addrStr);
                ctoa(node->getNodeTime(), clockStr);
                printf("        node%u removing %s from topology table "
                       "at time %s\n",
                       node->nodeId, addrStr, clockStr);
            }

            tmp = iterator;

            if (iterator == fisheye->topologyTable.first)
            {
                fisheye->topologyTable.first = iterator->next;
                iterator = fisheye->topologyTable.first;
                prev = iterator;
            }
            else
            {
                prev->next = iterator->next;
                iterator = prev->next;
            }

            // Free entry that has just been replaced.
            current = tmp->neighborInfo->list;
            while (current != NULL)
            {
                currentNext = current->next;
                MEM_free(current);
                current = currentNext;
            }

            MEM_free(tmp->neighborInfo);
            MEM_free(tmp);
        }
        else
        {
            prev = iterator;
            iterator = iterator->next;
        }
    }

    for (int i = 0; i < node->numberInterfaces; i++)
    {
        fisheye->numLandmarkMember[i] =
            FsrlCountLandmarkMember(node, fisheye, i);

        // If we fall below the member threshold, then we need to
        // remove ourself from the landmark table.
        if (fisheye->numLandmarkMember[i] <
            fisheye->parameter.minMemberThreshold)
        {
            FsrlDeleteFromLandmarkTable(node,
                NetworkIpGetInterfaceAddress(node, i),
                i);
            fisheye->isLandmark[i] = FALSE;
        }
        else
        {
            FsrlUpdateLandmarkMemberCountInLandmarkTable(
                node,
                fisheye,
                fisheye->numLandmarkMember[i],
                i);

            FsrlUpdateLandmarkTable(
                node,
                NetworkIpGetInterfaceAddress(node, i),
                NetworkIpGetInterfaceAddress(node, i),
                -1,
                fisheye->numLandmarkMember[i],
                0,
                fisheye->landmarkTable.ownSequenceNumber,
                i);
        }
    }

    if (FsrlDebugInDetail(node))
    {
        printf("    after topology table checking for expired entries\n");

        FsrlPrintTopologyTable(node, fisheye);
    }
}


// /**
// FUNCTION   :: FsrlInitListToArray
// LAYER      :: NETWORK
// PURPOSE    :: Init. array that is used for shortest path calculation.
// PARAMETERS ::
// + list : FsrlListToArray* : pointer to the list
// RETURN     :: void : NULL
// **/

static
void FsrlInitListToArray(FsrlListToArray* list)
{
    list->size  = 0;
    list->first = NULL;
}


// /**
// FUNCTION   :: FsrlPrintListToArray
// LAYER      :: NETWORK
// PURPOSE    :: Print array that is used for shortest path calculation.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + list : FsrlListToArray* : Pointer to the list
// RETURN     :: void : NULL
// **/

static
void FsrlPrintListToArray(Node* node, FsrlListToArray* list)
{
    FsrlListToArrayItem* current = list->first;
    char clockStr[MAX_STRING_LENGTH];

    ctoa(node->getNodeTime(), clockStr);

    printf("nodeAddr = %d The listsize = %d time = %s\n",
           node->nodeId, list->size, clockStr);

    while (current != NULL)
    {
        printf("nodeAddr = %d, index = %d\n",
               current->nodeAddr, current->index);

        current=current->next;
    }

    printf("End of list\n");
    fflush(stdout);
}


// /**
// FUNCTION   :: FsrlFreeListToArray
// LAYER      :: NETWORK
// PURPOSE    :: Free array that is used for shortest path calculation.
// PARAMETERS ::
// + list : FsrlListToArray* : Pointer to the list
// RETURN     :: void : NULL
// **/

static
void FsrlFreeListToArray(FsrlListToArray* list)
{
    FsrlListToArrayItem* current = list->first;
    FsrlListToArrayItem* next;

    while (current != NULL)
    {
        next = current->next;
        MEM_free(current);
        current = next;
    }

    list->size = 0;
    list->first = NULL;
}


// /**
// FUNCTION   :: FsrlInsertListToArray
// LAYER      :: NETWORK
// PURPOSE    :: Insert to array used for shortest path calculation.
// PARAMETERS ::
// + list : FsrlListToArray* : Pointer to the list
// + nodeAddr : NodeAddress : Node address to be inserted
// + index : int** : Address of pointer to return index in the list
// RETURN     :: void : NULL
// **/

static
void FsrlInsertListToArray(FsrlListToArray* list,
                           NodeAddress nodeAddr,
                           int** index)
{
    // First element.
    if (list->size == 0)
    {
        FsrlListToArrayItem* temp = (FsrlListToArrayItem*)
            MEM_malloc(sizeof(FsrlListToArrayItem));

        ERROR_Assert(temp != NULL, "Memory error!");

        temp->prev = temp->next = NULL;
        temp->nodeAddr = nodeAddr;
        temp->index = 0;

        // Note all index in topology table will change automatically.
        *index = &(temp->index);

        (list->size) ++;
        list->first = temp;
    }
    else
    {
        // Organize the list in ascending order, bubble sort idea.
        FsrlListToArrayItem* temp = list->first;
        FsrlListToArrayItem* prev = temp;

        // Flag to indicate whether we got a position to insert.
        BOOL found = FALSE;

        while (temp != NULL && !found)
        {
            prev = temp;

            if (temp->nodeAddr > nodeAddr)
            {
                // Need to insert in front of temp.
                FsrlListToArrayItem* iterator;

                FsrlListToArrayItem* newItem = (FsrlListToArrayItem*)
                    MEM_malloc(sizeof(FsrlListToArrayItem));

                ERROR_Assert(newItem != NULL, "Memory allocation error!");

                found = TRUE;
                newItem->nodeAddr = nodeAddr;

                if (temp->prev == NULL)
                {
                    // temp is the first, now newItem will be the first
                    //  update list, temp and newItem pointers and index

                    newItem->next = temp;
                    newItem->prev = NULL;
                    temp->prev = newItem;

                    list->size ++;
                    list->first = newItem;
                    newItem->index = 0;

                    *index = &(newItem->index);
                    iterator = newItem->next;

                    while (iterator != NULL)
                    {
                        // this index may be pointed by all
                        // same nodeAddr from TopologyTable.
                        iterator->index ++;

                        iterator = iterator->next;
                    }
                }
                else /* temp is in the middle */
                {
                    newItem->next = temp;
                    newItem->prev = temp->prev;
                    temp->prev->next = newItem;
                    temp->prev = newItem;

                    list->size ++;

                    newItem->index = temp->index;
                    *index = &(newItem->index);
                    iterator = newItem->next;

                    while (iterator != NULL)
                    {
                        // this index may be pointed by all
                        // same nodeAddr from TopologyTable
                        iterator->index ++;

                        iterator = iterator->next;
                    }
                }/* end of middle else */
            }/* end of temp->nodeAddr > nodeAddr if */
            else if (temp->nodeAddr == nodeAddr)
            {
                found = TRUE;
                *index = &(temp->index);
            }

            temp = temp->next;
        }/* end of while temp loop */

        if (!found)
        {
            // The one we are trying to insert is the largest nodeAddr.
            FsrlListToArrayItem* lastItem = (FsrlListToArrayItem*)
                MEM_malloc(sizeof(FsrlListToArrayItem));

            ERROR_Assert(lastItem != NULL, "Memory error!");

            lastItem->nodeAddr = nodeAddr;
            lastItem->next = NULL;
            lastItem->prev = prev;
            prev->next = lastItem;

            lastItem->index = prev->index + 1;
            *index = &(lastItem->index);
            list->size ++;
        }
    }
}


// /**
// FUNCTION   :: FsrlFindShortestPath
// LAYER      :: NETWORK
// PURPOSE    :: Calculate shortest path base on the current linke state
//               table using Dijkstra's algorithm.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: void : NULL
// **/

static
void FsrlFindShortestPath(Node* node)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    int infIndex = 0;

    FisheyeRoutingTableRow* shorthestPathfromInterface[MAX_NUM_PHYS];
    int routingTableSize = 0;
    int intfRouteSize[MAX_NUM_PHYS];

    memset (intfRouteSize, 0, 10 * sizeof(int));
    memset (shorthestPathfromInterface, 0,
        10 * sizeof(FisheyeRoutingTableRow*));

    for (infIndex = 0; infIndex < node->numberInterfaces && !TunnelIsVirtualInterface(node, infIndex); infIndex++)
    {
        char addrString[MAX_STRING_LENGTH];
        FsrlListToArray* list;
        NodeAddress* array;
        int* prevHop;
        int* nextHop;
        int* distance;
        int** connectionTable;


        FisheyeTopologyTableRow* row;
        FsrlListToArrayItem* iterator;
        int i;
        int j;

        int NODES;
        int UNSEEN = 99999;
        int priority;
        int min;
        int iid;

        clocktype now = node->getNodeTime();

        FsrlCheckExpiredEntryInTopologyTable(node, fisheye);

        list = (FsrlListToArray*) MEM_malloc(sizeof(FsrlListToArray));

        FsrlInitListToArray(list);

        row = fisheye->topologyTable.first;

        if (FsrlDebugShortestPath(node))
        {
            printf("    before calculating shortest path...\n");
            FsrlPrintTopologyTable(node, fisheye);
            FsrlPrintRoutingTable(node, fisheye);
        }

        while (row != NULL)
        {
            FsrlNeighborListItem* tmp;

            // Only include not expired or self nodes.

            if ((row->incomingInterface == infIndex) &&
                (row->timestamp + fisheye->parameter.fisheyeMaxAge >= now ||
                 row->linkStateNode
                    == NetworkIpGetInterfaceAddress(node, infIndex)))
            {
                FsrlInsertListToArray(list,
                                      row->linkStateNode,
                                      &row->index);

                tmp = row->neighborInfo->list;

                while (tmp != NULL)
                {
                    FsrlInsertListToArray(list,
                                          tmp->nodeAddr,
                                          &tmp->index);
                    tmp = tmp->next;
                }
            }

            row = row->next;
        }

        array = (NodeAddress*) MEM_malloc(list->size * sizeof(NodeAddress));
        ERROR_Assert(array != NULL, "Memory error!");

        if (FsrlDebugShortestPath(node))
        {
            printf("    list content\n");
        }

        iterator = list->first;

        for (i = 0; i < list->size; i ++)
        {
            if (FsrlDebugShortestPath(node))
            {
                IO_ConvertIpAddressToString(iterator->nodeAddr, addrString);
                printf("        list[%d] = %s\n", i, addrString);
            }

            array[i] = iterator->nodeAddr;
            iterator = iterator->next;
        }

        // build connectionTable.
        connectionTable = (int**) MEM_malloc(sizeof(int*) * list->size);

        for (i = 0; i < list->size; i ++)
        {
            connectionTable[i] = (int*) MEM_malloc(sizeof(int) * list->size);
        }

        for (i = 0; i < list->size; i ++)
        {
            for (j = 0; j < list->size; j ++)
            {
                connectionTable[i][j] = 0 ;
            }
        }


        // Construct connectivity matrix.
        row = fisheye->topologyTable.first;
        while (row != NULL)
        {
            FsrlNeighborListItem* tmp = row->neighborInfo->list;


            // Only include not expired or self nodes.
            if ((row->incomingInterface == infIndex) &&
                 (row->timestamp + fisheye->parameter.fisheyeMaxAge >= now ||
                 row->linkStateNode
                    == NetworkIpGetInterfaceAddress(node, infIndex)))
            {
                // Get the link state node index
                i = *(row->index);

                // Then, for each of the link state node's neighbors,
                // update the connectivity matrix accordingly.
                while (tmp != NULL)
                {
                    if (!NetworkIpIsMyIP(node, tmp->nodeAddr))
                    {
                        j = *(tmp->index);
                        connectionTable[i][j] = 1;
                        connectionTable[j][i] = 1;
                    }

                    tmp = tmp->next;
                }
            }

            row = row->next;
        }

        if (FsrlDebugShortestPath(node))
        {
            printf("\n        ");

            for (i = 0; i < list->size; i ++)
            {
                printf("[%d] ", i);
            }

            printf("\n");

            for (i = 0; i < list->size; i ++)
            {
                printf("     [%d]", i);

                for (j = 0; j < list->size; j ++)
                {
                    printf(" %d  ", connectionTable[i][j]);
                }

                printf("\n");
            }

            printf("\n");
        }

        // Now we have the topology table.
        // So, run Dijkstra's algorithm to find the shortest path.

        NODES = list->size;

        distance = (int*) MEM_malloc((list->size + 1) * sizeof(int));
        prevHop = (int*) MEM_malloc((list->size + 1) * sizeof(int));
        nextHop = (int*) MEM_malloc((list->size + 1) * sizeof(int));

        for (i = 0; i < NODES; i ++)
        {
            distance[i] = -UNSEEN;
            prevHop[i]  = NODES;
            nextHop[i]  = -1;
        }

        distance[NODES] = -(UNSEEN + 1);
        min = NODES;

        //for (infIndex = 0; infIndex < node->numberInterfaces; infIndex++)
        {

            row = FsrlFindRowInTopologyTable(
                NetworkIpGetInterfaceAddress(node, infIndex), fisheye);

            ERROR_Assert(row != NULL, "bad topology table...");

            iid = *(row->index);
            nextHop[iid]  = iid;
            prevHop[iid]  = iid;
            distance[iid] = 0;

            if (FsrlDebugShortestPath(node))
            {
                printf("Dijkstra's algorithm:\n");
                printf("    iid = %d\n", iid);
                printf("    nextHop[iid] = %d\n", nextHop[iid]);
                printf("    distance[iid] = %d\n", distance[iid]);
            }

            for (i = iid; i != NODES; i = min, min = NODES)
            {
                distance[i] = -distance[i];

                if (FsrlDebugShortestPath(node))
                {
                    printf("      distance[i=%d] = %d\n", i, distance[i]);
                }

                for (j = 0; j < NODES; j ++)
                {
                    if (FsrlDebugShortestPath(node))
                    {
                        printf("        distance[j=%d] = %d\n",
                            j, distance[j]);
                    }

                    if (distance[j] < 0)
                    {
                        priority = distance[i] + connectionTable[i][j];

                        if (FsrlDebugShortestPath(node))
                        {
                            printf("        connectionTable[%d][%d] = %d\n",
                                   i, j, connectionTable[i][j]);
                            printf("        priority = %d\n", priority);
                        }

                        if (connectionTable[i][j] &&
                                (distance[j] < -priority))
                        {
                            distance[j] = -priority;
                            prevHop[j] = i;

                            if (FsrlDebugShortestPath(node))
                            {
                                printf("        distance[j = %d] = %d\n",
                                       j, distance[j]);
                                printf("        prevHop[j = %d] = %d\n",
                                       j, prevHop[j]);
                            }

                            if (prevHop[j] == iid)
                            {
                                nextHop[j] = j;

                                if (FsrlDebugShortestPath(node))
                                {
                                    printf("        j = nextHop[j = %d] "
                                        "= %d\n",
                                           j, nextHop[j]);
                                }
                            }
                            else
                            {
                                nextHop[j] = nextHop[i];

                                if (FsrlDebugShortestPath(node))
                                {
                                    printf("        nextHop[i]"
                                        " = nextHop[j = %d] = %d\n",
                                           j, nextHop[j]);
                                }
                            }
                        }

                        if (distance[j] > distance[min])
                        {
                            min = j;

                            if (FsrlDebugShortestPath(node))
                            {
                                printf("        min = %d\n", min);
                            }
                        }
                    }
                }
            }

        // Dijkstra's algorithm is finished and now we save the results.
        //MEM_free(fisheye->routingTable.row);

        shorthestPathfromInterface[infIndex] = (FisheyeRoutingTableRow*)
                    MEM_malloc(list->size * sizeof(FisheyeRoutingTableRow));

        //fisheye->routingTable.row = (FisheyeRoutingTableRow* )
        //    MEM_malloc(list->size * sizeof(FisheyeRoutingTableRow));

        //ERROR_Assert(fisheye->routingTable.row != NULL, "Memory error!");
        routingTableSize += list->size;
        intfRouteSize[infIndex] = list->size;

        //fisheye->routingTable.rtSize = list->size;

        iterator = list->first;

        if (FsrlDebugShortestPath(node))
        {
            printf("    updating routing table\n");
        }

        for (i = 0; i < list->size; i ++)
        {
            if (FsrlDebugShortestPath(node))
            {
                printf("        examining nextHop[%d] = %d\n", i,nextHop[i]);
            }

            //(fisheye->routingTable.row)[i].destAddr = array[i];
            shorthestPathfromInterface[infIndex][i].destAddr = array[i];
            if (nextHop[i] < 0)
            {

                //fisheye->routingTable.row[i].nextHop =
                //    (NodeAddress) NETWORK_UNREACHABLE;
                //fisheye->routingTable.row[i].distance = FISHEYE_INFINITY;
                //fisheye->routingTable.row[i].outgoingInterface = -1;

                shorthestPathfromInterface[infIndex][i].nextHop =
                                (NodeAddress) NETWORK_UNREACHABLE;
                shorthestPathfromInterface[infIndex][i].distance
                    = FISHEYE_INFINITY;
                shorthestPathfromInterface[infIndex][i].outgoingInterface
                    = -1;
            }
            else
            {
                ERROR_Assert(nextHop[i] < list->size, "Wrong value!");
                ERROR_Assert(prevHop[i] < list->size, "Wrong value!");
                ERROR_Assert(prevHop[i] < list->size, "Wrong value!");


                shorthestPathfromInterface[infIndex][i].nextHop
                    = array[nextHop[i]];

                shorthestPathfromInterface[infIndex][i].prevHop
                    = array[prevHop[i]];

                shorthestPathfromInterface[infIndex][i].distance
                    = distance[i];

                /*
                fisheye->routingTable.row[i].nextHop = array[nextHop[i]];

                fisheye->routingTable.row[i].prevHop = array[prevHop[i]];

                fisheye->routingTable.row[i].distance = distance[i];
                */

                if (NetworkIpIsMyIP(node,shorthestPathfromInterface[infIndex][i].nextHop))
                {
                    row = FsrlFindRowInTopologyTable(
                          shorthestPathfromInterface[infIndex][i].nextHop,
                          fisheye);
                }
                else
                {
                    row = FsrlFindNeighbourInTopologyTable(
                          shorthestPathfromInterface[infIndex][i].nextHop,
                          fisheye, node);
                }
                if (row == NULL)
                {
                    shorthestPathfromInterface[infIndex][i]
                        .outgoingInterface = DEFAULT_INTERFACE;
                }
                else
                {
                    shorthestPathfromInterface[infIndex][i]
                        .outgoingInterface = row->incomingInterface;
                }
            }

            ERROR_Assert(array[i] == iterator->nodeAddr, "Wrong value!");

            iterator = iterator->next;
        }


        row = fisheye->topologyTable.first;

        while (row != NULL)
        {

            if ((row->incomingInterface == infIndex) &&
                (row->timestamp + fisheye->parameter.fisheyeMaxAge >= now ||
                row->linkStateNode
                    == NetworkIpGetInterfaceAddress(node, infIndex)))
            {
                row->distance = distance[*(row->index)];
            }

            row = row->next;
        }

        // Note: This flag could be TRUE because of
        // FsrlUpdateMyNeighborInfo.
        fisheye->routingTable.changed = FALSE;


        } //End for all interface

        // Free all malloc memory.
        MEM_free(distance);
        MEM_free(prevHop);
        MEM_free(nextHop);

        distance = NULL;
        prevHop = NULL;
        nextHop = NULL;

        for (i = 0; i < list->size; i ++)
        {
            MEM_free(connectionTable[i]);
            connectionTable[i] = NULL;
        }

        MEM_free(connectionTable);
        connectionTable = NULL;

        MEM_free(array);
        array = NULL;

        FsrlFreeListToArray(list);
        MEM_free(list);

        list = NULL;

        if (FsrlDebugShortestPath(node))
        {
            printf("    after calculating shortest path...\n");
            FsrlPrintRoutingTable(node, fisheye);
        }

        if (FsrlDebug(node))
        {
            printf("Node %d after shortest path calculation\n",
                node->nodeId);
            FsrlPrintRoutingTable(node, fisheye);
        }
    }
    // Now done for all interfaces now build the combine topology...

    MEM_free(fisheye->routingTable.row);


    fisheye->routingTable.row = (FisheyeRoutingTableRow*)
            MEM_malloc(routingTableSize * sizeof(FisheyeRoutingTableRow));

    fisheye->routingTable.rtSize = routingTableSize;

    int i = 0;
    int k = 0;
    for (infIndex = 0; infIndex < node->numberInterfaces; infIndex++)
    {
        for (i = 0; i < intfRouteSize[infIndex]; i++)
        {
            fisheye->routingTable.row[k].destAddr =
                shorthestPathfromInterface[infIndex][i].destAddr ;
            fisheye->routingTable.row[k].nextHop =
                shorthestPathfromInterface[infIndex][i].nextHop;
            fisheye->routingTable.row[k].prevHop =
                shorthestPathfromInterface[infIndex][i].prevHop;
            fisheye->routingTable.row[k].distance =
                shorthestPathfromInterface[infIndex][i].distance;
            fisheye->routingTable.row[k].outgoingInterface =
                shorthestPathfromInterface[infIndex][i].outgoingInterface;
            k++;
        }
        MEM_free(shorthestPathfromInterface[infIndex]);
    }

     if (fisheye->routingTable.row[0].distance > FISHEYE_INFINITY)
        {
            char addrStr[MAX_STRING_LENGTH];

            printf("Node %d having problems\n", node->nodeId);

            IO_ConvertIpAddressToString(fisheye->routingTable.row[0].nextHop,
                                        addrStr);
            printf("nextHop is %s\n", addrStr);
            IO_ConvertIpAddressToString(fisheye->routingTable.row[0].prevHop,
                                        addrStr);
            printf("prevHop is %s\n", addrStr);
            printf("distance is %d\n",
                fisheye->routingTable.row[0].distance);

            FsrlPrintRoutingTable(node, fisheye);

            ERROR_Assert(0, "Wrong distance!");
        }


    ERROR_Assert(fisheye->routingTable.row != NULL, "Memory error!");
}

// /**
// FUNCTION   :: FsrlInitStats
// LAYER      :: NETWORK
// PURPOSE    :: Initialize statistics variables.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlInitStats(Node* node, FsrlData* fisheye)
{
    memset(&fisheye->stats, 0, sizeof(fisheye->stats));
}


// /**
// FUNCTION   :: FsrlGetNextHopToLandmark
// LAYER      :: NETWORK
// PURPOSE    :: Get next hop to a specified landmark.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + destAddr : NodeAddress : Destnation node address.
// + nextHop : NodeAddress* : For returning next hop to the landmark
// + distance : int* : For returning distance to the landmark
// RETURN     :: BOOL : TRUE means find landmark, FALSE means no
//                      correponding landmark is found.
// **/

static
BOOL FsrlGetNextHopToLandmark(Node* node,
                              NodeAddress destAddr,
                              NodeAddress* nextHop,
                              int* distance,
                              int interfaceIndex)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlLandmarkTableRow* tmp;
    NodeAddress lmAddr = (NodeAddress) NETWORK_UNREACHABLE;
    BOOL found = FALSE;

    *nextHop = (NodeAddress) NETWORK_UNREACHABLE;
    *distance = -1;

    tmp = fisheye->landmarkTable.row;

    while (!found && tmp != NULL)
    {

        if (FsrlSameSubnet(tmp->destAddr,
                           destAddr,
                           NetworkIpGetInterfaceNumHostBits(
                            node, interfaceIndex)))
        {
            found = TRUE;
            lmAddr = tmp->destAddr;
            *nextHop = tmp->nextHop;
            *distance = tmp->distance;
        }

        tmp = tmp->next;
    }

    // If we are the landmark of drifter, then no need to propagate
    // drifter information.

    if (lmAddr ==
        NetworkIpGetInterfaceAddress(node, interfaceIndex))
    {
        found = FALSE;
    }

    return found;
}


// /**
// FUNCTION   :: FsrlFindRouteByFisheye
// LAYER      :: NETWORK
// PURPOSE    :: Find route using local fisheye routing table
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + destAddr : NodeAddress : Destination node
// + nextHop : NodeAddress* : For returning next hop to destination
// + outgoingInterface : int* : For returning interface to destination
// + distance : int* : For returning distance to destination
// RETURN     :: BOOL : TRUE means route found, FALSE means no route found
// **/

static
BOOL FsrlFindRouteByFisheye(Node* node,
                            NodeAddress destAddr,
                            NodeAddress* nextHop,
                            int* outgoingInterface,
                            int* distance)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FisheyeRoutingTable* routingTable = &(fisheye->routingTable);

    BOOL found = FALSE;
    int i;


    *nextHop = (NodeAddress) NETWORK_UNREACHABLE;
    *distance = FISHEYE_INFINITY;
    *outgoingInterface = -1;

    if (FsrlDebugForwarding(node))
    {
        char addrString[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(destAddr, addrString);
        printf("    looking for route to %s\n", addrString);

        FsrlPrintRoutingTable(node, fisheye);
    }

    for (i = 0; (i < routingTable->rtSize) && (!found); i ++)
    {
        if ((routingTable->row)[i].destAddr == destAddr)
        {
            // find the route directly.
            found = TRUE;

            *nextHop = (routingTable->row)[i].nextHop;
            *distance = (routingTable->row)[i].distance;
            *outgoingInterface = (routingTable->row)[i].outgoingInterface;

            ERROR_Assert(routingTable->row[i].distance <= FISHEYE_INFINITY,
                         "Wrong fisheye routing table entry!");

            if (routingTable->row[i].distance == FISHEYE_INFINITY)
            {
                *nextHop = (NodeAddress) NETWORK_UNREACHABLE;
            }
        }
    }

    return found;
}


// /**
// FUNCTION   :: FsrlFindRouteByDrifter
// LAYER      :: NETWORK
// PURPOSE    :: Find route to destinaiton from drifter table
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + destAddr : NodeAddress : Destination node
// + nextHop : NodeAddress* : For returning next hop to destination
// + outgoingInterface : int* : For returning interface to destination
// + distance : int* : For returning distance to destination
// RETURN     :: BOOL : TRUE means route found, FALSE means no route found
// **/

static
BOOL FsrlFindRouteByDrifter(Node* node,
                            NodeAddress destAddr,
                            NodeAddress* nextHop,
                            int* outgoingInterface,
                            int* distance)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlDrifterTableRow* tmp;
    BOOL found = FALSE;

    *nextHop = (NodeAddress) NETWORK_UNREACHABLE;
    *distance = FISHEYE_INFINITY;
    *outgoingInterface = -1;

    FsrlCheckExpiredEntryInDrifterTable(node, fisheye);

    if (FsrlDebugForwarding(node))
    {
        FsrlPrintDrifterTable(node, fisheye);
    }

    tmp = fisheye->drifterTable.row;

    while (!found && tmp != NULL)
    {
        if (tmp->destAddr == destAddr)
        {
            found = TRUE;

            *nextHop = tmp->nextHop;
            *distance = tmp->distance;
            *outgoingInterface = tmp->outgoingInterface;

            ERROR_Assert(tmp->distance <= FISHEYE_INFINITY,
                         "Wrong drifter table entry!");

            if (tmp->distance == FISHEYE_INFINITY)
            {
                *nextHop = (NodeAddress) NETWORK_UNREACHABLE;
            }
        }

        tmp = tmp->next;
    }

    return found;
}


// /**
// FUNCTION   :: FsrlFindRouteByLandmark
// LAYER      :: NETWORK
// PURPOSE    :: Find route to destinaiton using landmark table
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + destAddr : NodeAddress : Destination node
// + nextHop : NodeAddress* : For returning next hop to destination
// + outgoingInterface : int* : For returning interface to destination
// + distance : int* : For returning distance to destination
// RETURN     :: BOOL : TRUE means route found, FALSE means no route found
// **/

static
BOOL FsrlFindRouteByLandmark(Node* node,
                             NodeAddress destAddr,
                             NodeAddress* nextHop,
                             int* outgoingInterface,
                             int* distance)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlLandmarkTableRow* tmp;
    BOOL found = FALSE;

    *nextHop = (NodeAddress) NETWORK_UNREACHABLE;
    *distance = FISHEYE_INFINITY;
    *outgoingInterface = -1;

    FsrlCheckExpiredEntryInLandmarkTable(node, FALSE);

    if (FsrlDebugForwarding(node))
    {
        FsrlPrintLandmarkTable(node, fisheye);
    }

    tmp = fisheye->landmarkTable.row;

    while (tmp != NULL && !found)
    {

        // Check all the interfaces
        BOOL isInSameSubnet = FALSE;
        for (int i = 0; i < node->numberInterfaces; i ++)
        {
            if (FsrlSameSubnet(tmp->destAddr,
                               destAddr,
                               NetworkIpGetInterfaceNumHostBits(
                                   node, i)))
            {
                isInSameSubnet = TRUE;
                break;
            }
        }
        if (isInSameSubnet)
        {
            found = TRUE;

            *nextHop = tmp->nextHop;
            *outgoingInterface = tmp->outgoingInterface;

            // Don't want to return distance of 0 since this means
            // we are using ourself as the next hop.
            if (tmp->distance != 0)
            {
                *distance = tmp->distance;
            }

            ERROR_Assert(tmp->distance <= FISHEYE_INFINITY,
                         "Wrong landmark table entry!");

            if (tmp->distance == FISHEYE_INFINITY)
            {
                *nextHop = (NodeAddress) NETWORK_UNREACHABLE;
            }
        }

        tmp = tmp->next;
    }

    // Chech this ip is mine.
    if (NetworkIpIsMyIP(node, *nextHop))
    {
        return FALSE;
    }

    return found;
}


// /**
// FUNCTION   :: FsrlRoutePacket
// LAYER      :: NETWORK
// PURPOSE    :: Route a packet to destination.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to message structure of the packet
// RETURN     :: void : NULL
// **/

static
void FsrlRoutePacket(Node* node, Message* msg)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    NodeAddress nextHop = (NodeAddress) NETWORK_UNREACHABLE;
    int outgoingInterface = -1;
    int distance = FISHEYE_INFINITY;

    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    if (FsrlDebugPackets(node))
    {
        char addrString[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(ipHeader->ip_dst, addrString);
        printf("Node %d got DATA, need to find route to dest %s\n",
               node->nodeId, addrString);
    }

    if (fisheye->routingTable.changed == TRUE)
    {
        FsrlFindShortestPath(node);
    }

    // First look for route that is within scope.
    if (FsrlFindRouteByFisheye(node,
                               ipHeader->ip_dst,
                               &nextHop,
                               &outgoingInterface,
                               &distance))
    {
        if (FsrlDebugForwarding(node))
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(nextHop, addrString);
            printf("    fisheye route\n");
            printf("        nextHop = %s, outgoingInterface=%d\n",
                   addrString, outgoingInterface);
        }

        if (nextHop != (unsigned) NETWORK_UNREACHABLE)
        {
            NetworkIpSendPacketToMacLayer(node,
                                          msg,
                                          outgoingInterface,
                                          nextHop);

        }
        else
        {
            if (FsrlDebugForwarding(node))
            {
                printf("        dropped due to out of fisheye scope\n");
            }

            fisheye->stats.dropInScope++;

            MESSAGE_Free(node, msg);
        }
    }
    // If route not within scope, try to route by drifter.
    else if (FsrlFindRouteByDrifter(node,
                                    ipHeader->ip_dst,
                                    &nextHop,
                                    &outgoingInterface,
                                    &distance))
    {
        if (FsrlDebugForwarding(node))
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(nextHop, addrString);
            printf("    drifter route\n");
            printf("        nextHop = %s, outgoingInterface=%d\n",
                   addrString, outgoingInterface);
        }

        // Need route to drifter node directly.
        if (nextHop != (unsigned) NETWORK_UNREACHABLE)
        {
            NetworkIpSendPacketToMacLayer(node,
                                          msg,
                                          outgoingInterface,
                                          nextHop);
        }
        else
        {
            if (FsrlDebugForwarding(node))
            {
                printf("        dropped due to no drifter info\n");
            }

            fisheye->stats.dropNoDF ++;

            MESSAGE_Free(node, msg);
        }
    }
    // If route not within scope or not drifter, route by landmark.
    else if (FsrlFindRouteByLandmark(node,
                                     ipHeader->ip_dst,
                                     &nextHop,
                                     &outgoingInterface,
                                     &distance))
    {
        if (FsrlDebugForwarding(node))
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(nextHop, addrString);
            printf("    landmark route\n");
            printf("        nextHop = %s, outgoingInterface=%d\n",
                   addrString, outgoingInterface);
        }

        // need route to Landmark node using subnet ID in the address.
        if (nextHop != (unsigned) NETWORK_UNREACHABLE)
        {
            NetworkIpSendPacketToMacLayer(node,
                                          msg,
                                          outgoingInterface,
                                          nextHop);
        }
        else
        {
            if (FsrlDebugForwarding(node))
            {
                printf("        dropped due to no landmark info\n");
            }

            fisheye->stats.dropNoLM ++;

            MESSAGE_Free(node, msg);
        }
    }
    // If were get here, then no route exists for this destination, so drop.
    else
    {
        if (FsrlDebugForwarding(node))
        {
            printf("        dropped due to no route\n");
        }

        fisheye->stats.dropNoRoute ++;

        MESSAGE_Free(node, msg);
    }
}


// /**
// FUNCTION   :: FsrlUpdateMyNeighborInfo
// LAYER      :: NETWORK
// PURPOSE    :: Update neighbor info. based on control packets received.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + nodeAddr : NodeAddress : Neighbor node address
// + incomingInterface : int : Interface index the control packet from
// RETURN     :: void : NULL
// **/

static
void FsrlUpdateMyNeighborInfo(Node* node,
                              NodeAddress nodeAddr,
                              int incomingInterface)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FisheyeTopologyTableRow* row;
    FsrlNeighborListItem* neighbor;
    FsrlNeighborListItem* prev = NULL;
    BOOL change = FALSE;
    BOOL found = FALSE;


    row = FsrlFindRowInTopologyTable(NetworkIpGetInterfaceAddress(
        node, incomingInterface),
        fisheye);

    // ERROR_Assert(row != NULL, "Wrong topology table!");
    if (!row)
    {
        return;
    }

    // Neighbor list is empty.
    if (row->neighborInfo->list == NULL)
    {
        FsrlNeighborListItem* tmp = (FsrlNeighborListItem*)
            MEM_malloc(sizeof(FsrlNeighborListItem));

        tmp->nodeAddr = nodeAddr;
        tmp->incomingInterface = incomingInterface;
        tmp->timeLastHeard = node->getNodeTime();
        tmp->next = NULL;
        row->neighborInfo->numNeighbor = 1;

        row->neighborInfo->list = tmp;
        fisheye->routingTable.changed = TRUE;
        change = TRUE;
    }
    // Look for neighbor in the neighbor list since neighbor
    // list is not empty.
    else
    {
        neighbor = row->neighborInfo->list;

        while ((neighbor != NULL) && (!found))
        {
            if (neighbor->nodeAddr == nodeAddr)
            {
                found = TRUE;
                neighbor->timeLastHeard = node->getNodeTime();
                neighbor->incomingInterface = incomingInterface;
            }
            else
            {
                prev = neighbor;
                neighbor = neighbor->next;
            }
        }

        if (!found)
        {
            FsrlNeighborListItem* tmp = (FsrlNeighborListItem*)
                MEM_malloc(sizeof(FsrlNeighborListItem));

            tmp->nodeAddr = nodeAddr;
            tmp->incomingInterface = incomingInterface;
            tmp->timeLastHeard = node->getNodeTime();
            tmp->next = NULL;
            prev->next = tmp;

            ERROR_Assert(row->neighborInfo->numNeighbor > 0,
                             "Wrong neighbor information!");

            // chk numNeighbor within limit
            ERROR_Assert(row->neighborInfo->numNeighbor < 255,
                         "Number of neighbor exceed limit!!!");

            row->neighborInfo->numNeighbor ++;

            fisheye->routingTable.changed = TRUE;
            change = TRUE;
        }
    }
}


// /**
// FUNCTION   :: FsrlDeleteNeighbor
// LAYER      :: NETWORK
// PURPOSE    :: Delete a neighbor from topology table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + neighborAddr : NodeAddress : Neighbor node address
// RETURN     :: void : NULL
// **/

static
void FsrlDeleteNeighbor(Node* node, NodeAddress neighborAddr)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FisheyeTopologyTableRow* row;
    FsrlNeighborListItem* trash;
    FsrlNeighborListItem* prev;
    FsrlNeighborListItem* current;
    BOOL change;


    change = FALSE;
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        row = FsrlFindRowInTopologyTable(NetworkIpGetInterfaceAddress(
                                         node, i),
                                         fisheye);

        if (!row)
        {
            continue;
            // ERROR_Assert(row != NULL, "Wrong topology table!");
        }

        current = prev = row->neighborInfo->list;


        if (FsrlDebug(node))
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(neighborAddr, addrString);
            printf("Node %d deleting neighbor %s\n",
                node->nodeId, addrString);
        }

        while (current != NULL)
        {
            trash = NULL;

            if (current->nodeAddr == neighborAddr)
            {
                change = TRUE;

                // Remove the stale neighbor from neighborList
                if (current == row->neighborInfo->list)
                {
                    row->neighborInfo->list = row->neighborInfo->list->next;
                    prev = row->neighborInfo->list;
                }
                else
                {
                    prev->next = current->next;
                }

                trash = current;
                current = current->next;

                MEM_free(trash);

                ERROR_Assert(row->neighborInfo->numNeighbor> 0,
                             "Wrong neighbor information!");
                row->neighborInfo->numNeighbor --;

                trash = NULL;
            }
            else
            {
                prev = current;
                current = current->next;
            }
        }
    }
    if (change)
    {
        if (FsrlDebug(node))
        {
            printf("        found!\n");
        }

        fisheye->routingTable.changed = TRUE;
    }
    else
    {
        if (FsrlDebug(node))
        {
            printf("        not found!\n");
        }
    }
}


// /**
// FUNCTION   :: FsrlSendLanmarUpdatePacket
// LAYER      :: NETWORK
// PURPOSE    :: Send landmark update packet.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// + lower : int : Lower bound of the scope
// + upper : int : Upper bound of the scope
// RETURN     :: void : NULL
// **/

static
void FsrlSendLanmarUpdatePacket(Node* node,
                                FsrlData* fisheye,
                                int lower,
                                int upper)
{
    int landmarkTableEntrySize = sizeof(FsrlLanmarUpdateLandmarkField);
    int drifterTableEntrySize =  sizeof(FsrlLanmarUpdateDrifterField);

    FsrlLanmarUpdateHeaderField headerField;
    FsrlLanmarUpdateLandmarkField landmarkField;
    FsrlLanmarUpdateDrifterField drifterField;

    FsrlLandmarkTableRow *landmarkTableTmp;
    FsrlDrifterTableRow *drifterTableTmp;
    int landmarkTableSize;
    int drifterTableSize;

    BOOL landmarkTableDone;
    BOOL drifterTableDone;

    char payload[FISHEYE_MAX_TOPOLOGY_TABLE];
    FsrlPacketType packetType = FSRL_UPDATE_DV;
    int packetSize;
    int numDrifter;

    NodeAddress nextHop;
    int distance;
    BOOL infinityEntry = FALSE;
    int i;


    BOOL haveDoneAnyInfinityEntry = FALSE;
    // Send Landmark Updates for all interface
    for (i = 0; i < node->numberInterfaces; i ++)
    {
        infinityEntry = FALSE;
        landmarkTableDone = FALSE;
        drifterTableDone = FALSE;

        drifterTableTmp = fisheye->drifterTable.row;
        landmarkTableTmp = fisheye->landmarkTable.row;

        if (FsrlDebug(node))
        {
            printf("Node %u before sending LANMAR Update packet\n",
                   node->nodeId);

            FsrlPrintLandmarkTable(node, fisheye);
            FsrlPrintDrifterTable(node, fisheye);
        }


        // Note that we are still sending out LANMAR updates even
        // if no landmark node meets the election criteria.  In this
        // case, the update packet will solely function as hello packets.
        int maxTopologyTable = (GetNetworkIPFragUnit(node, i)  -
                                sizeof(IpHeaderType));
        while ((!landmarkTableDone) || (!drifterTableDone))
        {
            landmarkTableSize = 0;
            drifterTableSize = 0;
            numDrifter = 0;

            packetSize = 0;

            // First, start by filling the packet with header information.
            // Will be filled in later when the number of drifter entries
            // are known.
            packetSize += sizeof(headerField);

            if (FsrlDebugLandmark(node))
            {
                printf("    now filling in landmark entries\n");
            }

            // Then, fill the packet with LMDV entries until either
            // the packet is full or there is no more LMDV entries.

            while (((packetSize + landmarkTableEntrySize) <(signed)
                    maxTopologyTable) &&
                   (landmarkTableTmp != NULL))
            {
                // Only advertise if the member threshold is satisfied.
                if (landmarkTableTmp->numLandmarkMember <
                    fisheye->parameter.minMemberThreshold)
                {
                    FsrlPrintLandmarkTable(node, fisheye);
                    ERROR_Assert(0, "Wrong landmark information!");
                }

                landmarkField.landmarkAddr = landmarkTableTmp->destAddr;
                landmarkField.nextHopAddr = landmarkTableTmp->nextHop;

                ERROR_Assert(landmarkField.nextHopAddr != 0,
                             "Wrong next hop!");

                landmarkField.distance
                    = (unsigned char) landmarkTableTmp->distance;
                landmarkField.numMember
                    = (unsigned char) landmarkTableTmp->numLandmarkMember;
                landmarkField.sequenceNumber
                    = (unsigned short) landmarkTableTmp->sequenceNumber;

                if (landmarkField.distance > FISHEYE_INFINITY)
                {
                    printf("Node %d has landmarkField.distance of %d\n",
                           node->nodeId, landmarkField.distance);

                    FsrlPrintLandmarkTable(node, fisheye);
                    ERROR_Assert(0, "Wrong distance information!");
                }

                // Used later to determine if we need to remove infinity
                // entries from the landmark table.
                if (landmarkTableTmp->distance == FISHEYE_INFINITY)
                {
                    infinityEntry = TRUE;
                    haveDoneAnyInfinityEntry = TRUE;
                }

                memcpy(&(payload[packetSize]),
                       &landmarkField,
                       sizeof(landmarkField));

                packetSize += sizeof(landmarkField);

                landmarkTableSize += landmarkTableEntrySize;

                landmarkTableTmp = landmarkTableTmp->next;
            }

            if (landmarkTableTmp == NULL)
            {
                landmarkTableDone = TRUE;
            }

            if (FsrlDebugDrifter(node))
            {
                printf("    now filling in drifter entries\n");
            }

            // Then, fill the packet with drifter information until either
            // the packet is full or there is no more drifter left.

            while ((landmarkTableDone) &&
                   ((packetSize + drifterTableEntrySize) <(signed)
                    maxTopologyTable) &&
                   (drifterTableTmp != NULL))
            {
                if (FsrlGetNextHopToLandmark(node,
                                             drifterTableTmp->destAddr,
                                             &nextHop,
                                             &distance,
                                             i))
                {
                    if (FsrlDebugDrifter(node))
                    {
                        char addrString[MAX_STRING_LENGTH];

                        IO_ConvertIpAddressToString(nextHop, addrString);
                        printf("    next hop is %s\n", addrString);
                    }

                    drifterField.drifterAddr = drifterTableTmp->destAddr;
                    drifterField.nextHopAddr = nextHop;

                    ERROR_Assert(nextHop != 0, "Wrong next hop!");

                    drifterField.distance = (char) drifterTableTmp->distance;
                    drifterField.sequenceNumber =
                        (unsigned short) drifterTableTmp->sequenceNumber;

                    memcpy(&(payload[packetSize]),
                           &drifterField,
                           sizeof(drifterField));
                    packetSize += sizeof(drifterField);

                    drifterTableSize += drifterTableEntrySize;
                    numDrifter ++;

                    // chk numDrifers within limit
                    ERROR_Assert(numDrifter <= 255,
                                 "Number of drifters exceed limit!!!");
                }

                drifterTableTmp = drifterTableTmp->next;
            }

            if (drifterTableTmp == NULL)
            {
                drifterTableDone = TRUE;
            }

            // Now that we know the number of drifter entries,
            // we can now fill up the header entries.
            headerField.packetType
                = (unsigned char) packetType;
            headerField.landmarkFlag
                = (unsigned char) fisheye->isLandmark[i];
            headerField.numLandmark
                = (unsigned char) fisheye->landmarkTable.size;
            headerField.numDrifter
                = (unsigned char) numDrifter;


            memcpy(&(payload[0]),
                   &headerField,
                   sizeof(headerField));

            if (packetSize > 0)
            {
                Message* msg;
                NodeAddress sourceAddr;

                msg = MESSAGE_Alloc(node,
                                    MAC_LAYER, 0, MSG_MAC_FromNetwork);
                MESSAGE_PacketAlloc(node,
                                    msg, packetSize, TRACE_FSRL);

                memcpy(MESSAGE_ReturnPacket(msg), payload, packetSize);

                if (FsrlDebugPackets(node))
                {
                    char clockStr[MAX_STRING_LENGTH];

                    ctoa(node->getNodeTime(), clockStr);
                    printf("Node %u sending LANMAR"
                           "Update packet at time %s\n",
                           node->nodeId, clockStr);

                    FsrlPrintLanmarUpdatePacket(node, msg);
                }

                sourceAddr = NetworkIpGetInterfaceAddress(node, i);

                NetworkIpSendRawMessageWithDelay(
                    node,
                    msg, //MESSAGE_Duplicate(node, msg),
                    sourceAddr,
                    ANY_DEST,
                    i,
                    IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_FSRL,
                    1,
                    (clocktype) (RANDOM_nrand(fisheye->seed)
                                 % FISHEYE_RANDOM_JITTER));

                if (FsrlDebugPackets(node))
                {
                    char addrString[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(sourceAddr,
                                                addrString);
                    printf("    using interface %s\n", addrString);
                }

                fisheye->stats.controlOH += packetSize;
                fisheye->stats.controlPkts += 1;
            }
        }
    }
    // Some routes have been set to infinity so delete them.
    if (haveDoneAnyInfinityEntry)
    {
        FsrlDeleteInfinityEntriesFromLandmarkTable(node);
    }
}


// /**
// FUNCTION   :: FsrlHandleNeighborTimeout
// LAYER      :: NETWORK
// PURPOSE    :: Time out stale neighbors
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlHandleNeighborTimeout(Node* node, FsrlData* fisheye)
{
    FisheyeTopologyTableRow* row;
    Message* newMsg;
    clocktype randomTime;
    BOOL change;
    FsrlNeighborListItem* trash;
    FsrlNeighborListItem* prev;
    FsrlNeighborListItem* current;


    change = FALSE;
    for (int i = 0; i < node->numberInterfaces; i++)
    {

        row = FsrlFindRowInTopologyTable(
            NetworkIpGetInterfaceAddress(node, i), fisheye);


        // ERROR_Assert(row != NULL, "Wrong topology table!");
        if (!row)
        {
            continue;
        }

        if (FsrlDebugNeighbor(node))
        {
            printf("Node %d checking neighbor timeout\n", node->nodeId);
        }

        current = prev = row->neighborInfo->list;


        while (current != NULL)
        {
            trash = NULL;

            if ((node->getNodeTime() - current->timeLastHeard) >
                fisheye->parameter.neighborTOInterval)
            {
                change = TRUE;

                // Remove the stale neighbor from neighborList.

                if (current == row->neighborInfo->list)
                {
                    // Entry to remove is the first entry in the list.

                    row->neighborInfo->list = row->neighborInfo->list->next;

                    prev = row->neighborInfo->list;
                }
                else
                {
                    // Entry to remove is not the first entry in the list.

                    prev->next = current->next;
                }

                row->neighborInfo->numNeighbor --;
                //ERROR_Assert(row->neighborInfo->numNeighbor >= 0,
                //             "Wrong neighbor information!");

                if (FsrlDebugNeighbor(node))
                {
                    char addrString[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(current->nodeAddr,
                        addrString);
                    printf("    neighbor %s timed out!\n", addrString);
                }

                // We need to remove the neighbor that has timed out from
                // the drifter table if it's the next hop to a drifter.
                FsrlDeleteFromDrifterTable(node, current->nodeAddr);

                // We need to remove the neighbor that has timed out from
                // the landmark table if it's a landmark.

                FsrlDeleteFromLandmarkTable(node, current->nodeAddr, i);

                // Set distance metric to infinity and increment seqno by 1.
                FsrlInvalidateEntryInLandmarkTable(node,
                                                   current->nodeAddr);

                trash = current;
                current = current->next;

                MEM_free(trash);

                trash = NULL;
            }
            else
            {
                prev = current;
                current = current->next;
            }
        }
    }

    if (change)
    {
        fisheye->routingTable.changed = TRUE;
    }

    // schedule next neighbor timeout check.
    randomTime = (clocktype)(RANDOM_nrand(fisheye->seed) % FISHEYE_RANDOM_JITTER);

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_FSRL,
                           MSG_FsrlNeighborTimeout);

    MESSAGE_Send(node,
                 newMsg,
                 randomTime + fisheye->parameter.neighborTOInterval);
}


// /**
// FUNCTION   :: FsrlUpdateTopologyTable
// LAYER      :: NETWORK
// PURPOSE    :: Insert or update one entry in topology table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + tmp : FisheyeTopologyTableRow* : Pointer to the entry
// RETURN     :: BOOL : TRUE, entry is updated, FALSE, no update
// **/

static
BOOL FsrlUpdateTopologyTable(Node* node,
                             FisheyeTopologyTableRow* tmp,
                             int incomingInterface)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FisheyeTopologyTableRow *iterator = fisheye->topologyTable.first;
    FisheyeTopologyTableRow *prev;

    FsrlNeighborListItem *current;
    FsrlNeighborListItem *currentNext;

    BOOL update = FALSE;
    BOOL found = FALSE;

    prev = iterator;

    if (FsrlDebugTopologyTable(node))
    {
        printf("    before topology table update\n");
        FsrlPrintTopologyTable(node, fisheye);
    }

    while (iterator != NULL)
    {
        if (tmp->linkStateNode == iterator->linkStateNode)
        {
            found = TRUE;

            if (FsrlDebugTopologyTable(node))
            {
                char addrString[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(tmp->linkStateNode, addrString);
                printf("    found %s in table\n", addrString);
            }

            if (tmp->neighborInfo->sequenceNumber >
                iterator->neighborInfo->sequenceNumber)
            {
                update = TRUE;

                if (FsrlDebugTopologyTable(node))
                {
                    printf("        greater sequence number\n");
                }

                // Use tmp as newer topology table entry, not iterator.
                // Update prev->next and tmp->next

                // Replace old entry in list.
                if (prev != iterator)
                {
                    prev->next = tmp;
                }
                else
                {
                    fisheye->topologyTable.first = tmp;
                }

                tmp->next = iterator->next;


                // Free entry that has just been replaced.
                current = iterator->neighborInfo->list;
                while (current != NULL)
                {
                    currentNext = current->next;
                    MEM_free(current);
                    current = currentNext;
                }

                MEM_free(iterator->neighborInfo);
                MEM_free(iterator);

                // Exit loop since we already updated the table.
                break;
            }
            else
            {
                if (FsrlDebugTopologyTable(node))
                {
                    printf("        sequence number not greater\n");
                }
            }
        }

        prev = iterator;
        iterator = iterator->next;
    }

    if (!found)
    {
        update = TRUE;

        if (FsrlDebugTopologyTable(node))
        {
            printf("   not found, so add to table\n");
        }

        if (prev != NULL)
        {
            prev->next = tmp;
        }
        else
        {
            // Empty topology table case, may never occur,
            // since we initilized an entry at the very beginning
            fisheye->topologyTable.first = tmp;
        }
    }

    if (update)
    {

        fisheye->numLandmarkMember[incomingInterface] =
            FsrlCountLandmarkMember(node, fisheye, incomingInterface);

        // If we fall below the member threshold, then we need to
        // remove ourself from the landmark table.
        if (fisheye->numLandmarkMember[incomingInterface] <
            fisheye->parameter.minMemberThreshold)
        {

            FsrlDeleteFromLandmarkTable(node,
                          NetworkIpGetInterfaceAddress(node,
                                       incomingInterface),
                          incomingInterface);

            fisheye->isLandmark[incomingInterface] = FALSE;
        }
        else
        {

            FsrlUpdateLandmarkMemberCountInLandmarkTable(
                node,
                fisheye,
                fisheye->numLandmarkMember[incomingInterface],
                incomingInterface);



            FsrlUpdateLandmarkTable(
                node,
                NetworkIpGetInterfaceAddress(node, incomingInterface),
                NetworkIpGetInterfaceAddress(node, incomingInterface),
                -1,
                fisheye->numLandmarkMember[incomingInterface],
                0,
                fisheye->landmarkTable.ownSequenceNumber,
                incomingInterface);
        }
    }

    if (FsrlDebugTopologyTable(node))
    {
        //FsrlFindShortestPath(node);
        printf("    after topology table update\n");
        FsrlPrintTopologyTable(node, fisheye);
    }

    return update;
}


// /**
// FUNCTION   :: FsrlInitTopologyTable
// LAYER      :: NETWORK
// PURPOSE    :: Initialize topology table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlInitTopologyTable(Node* node, FsrlData* fisheye)
{
    FisheyeTopologyTableRow* row;

    FisheyeTopologyTableRow* prevRow = NULL;
    NodeAddress addr;
    int i;

    memset((void*)&(fisheye->topologyTable),
           0,
           sizeof(FisheyeTopologyTable));

    fisheye->topologyTable.first = NULL;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        addr = NetworkIpGetInterfaceAddress(node, i);

        row = (FisheyeTopologyTableRow*)
            MEM_malloc(sizeof(FisheyeTopologyTableRow));

        row->linkStateNode = addr;
        row->distance = 0;

        row->index = NULL;
        row->neighborInfo = (FisheyeNeighborInfo*)
            MEM_malloc(sizeof(FisheyeNeighborInfo));

        row->neighborInfo->sequenceNumber = -1;

        row->neighborInfo->numNeighbor = 0;
        row->neighborInfo->list = NULL;

        row->timestamp = node->getNodeTime();
        row->incomingInterface = i;

        row->fromAddr = NetworkIpGetInterfaceAddress(node, i);

        row->next = NULL;


        // Set the starting row
        if (i == 0)
        {
            fisheye->topologyTable.first = row;
            prevRow = row;
        } else {
            prevRow->next = row;
            prevRow = row;
        }


        fisheye->numLandmarkMember[i] = 1;
    }
}


// /**
// FUNCTION   :: FsrlInitRoutingTable
// LAYER      :: NETWORK
// PURPOSE    :: Initialize routing table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlInitRoutingTable(Node* node, FsrlData* fisheye)
{
    // Zero out the routing table.
    memset((void*)&(fisheye->routingTable),
           0,
           sizeof(FisheyeRoutingTable));

    fisheye->routingTable.rtSize = 0;
    fisheye->routingTable.changed = TRUE;

    // Will be assigned from TopologyTable.
    fisheye->routingTable.row = NULL;
}


// /**
// FUNCTION   :: FsrlSendFisheyeUpdatePacket
// LAYER      :: NETWORK
// PURPOSE    :: Send toplogy table update packets within scope
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// + lower : int : Lower bound of scope
// + upper : int : Upper bound of scope
// RETURN     :: void : NULL
// **/

static
void FsrlSendFisheyeUpdatePacket(Node* node,
                                 FsrlData* fisheye,
                                 int lower,
                                 int upper)
{
    FsrlFisheyeHeaderField headerField;
    FsrlFisheyeDataField dataField;
    unsigned short broadcastTopologyTableSize = 0;
    int neighborEntrySize;
    int headerPartSize;
    short neighborNum;

    FsrlNeighborListItem* tmp;
    FisheyeTopologyTableRow* row;

    char payload[FISHEYE_MAX_TOPOLOGY_TABLE];
    BOOL packetEmpty = TRUE;
    int i;


    BOOL isSent = FALSE;

    FsrlCheckExpiredEntryInTopologyTable(node, fisheye);


     for (i = 0; i < node->numberInterfaces; i++)
     {
        packetEmpty = TRUE;
        row = fisheye->topologyTable.first;

        // There should be at least one valid update packet, which describes
        // ourself.
        ERROR_Assert(row != NULL, "Wrong topology table!");

        while (row != NULL)
        {
            headerPartSize = 0;
            broadcastTopologyTableSize = 0;

            // Header fields will be filled later when packet size is known.
            broadcastTopologyTableSize += sizeof(headerField);

            // Get fixed header size.
            headerPartSize = broadcastTopologyTableSize;
            int maxTopologyTable = (GetNetworkIPFragUnit(node, i)  -
                                        sizeof(IpHeaderType));

            while (row != NULL)
            {

                if ((row->distance >= lower && row->distance < upper &&
                     row->timestamp + fisheye->parameter.fisheyeMaxAge >=
                     node->getNodeTime()) ||
                     NetworkIpIsMyIP(node, row->linkStateNode))
                     //row->linkStateNode ==
                     //NetworkIpGetInterfaceAddress(node, i))

                {

                    //neighborEntrySize = sizeof(dataField) +
                    //    (sizeof(NodeAddress) * row->neighborInfo->numNeighbor);

                    if (row->linkStateNode ==
                        NetworkIpGetInterfaceAddress(node, i))
                    {
                        // Treating the other interface as neighbor to this
                        // interface..
                        neighborEntrySize = sizeof(dataField) +
                            (sizeof(NodeAddress)
                            * (row->neighborInfo->numNeighbor +
                             node->numberInterfaces - 1));
                    } else {
                         neighborEntrySize = sizeof(dataField) +
                            (sizeof(NodeAddress)
                            * row->neighborInfo->numNeighbor);
                    }

                    // If current packet size + next neighbor
                    // information set size is within the max packet
                    // size limit, then go ahead and put the neighbor
                    // information set into the packet.  If not, send it
                    // out and create a new packet.

                    if ((signed)(broadcastTopologyTableSize
                        + neighborEntrySize) >
                        (signed)(maxTopologyTable - headerPartSize))
                    {
                        break;
                    }

                    // Mark packet to send out.
                    packetEmpty = FALSE;

                    // Destination address.
                    dataField.destAddr = row->linkStateNode;

                    dataField.dataField = 0 ;
                    // Destination sequence number.
                    FsrlFisheyeDataFieldSetSeqNum
                        (&(dataField.dataField),
                        row->neighborInfo->sequenceNumber);

                    // N_neighbors.
                    if (row->linkStateNode ==
                                NetworkIpGetInterfaceAddress(node, i))
                    {
                       FsrlFisheyeDataFieldSetNumNeighbour(
                           &(dataField.dataField),
                           row->neighborInfo->numNeighbor +
                                node->numberInterfaces - 1);
                    } else
                    {
                        FsrlFisheyeDataFieldSetNumNeighbour(
                            &(dataField.dataField),
                            row->neighborInfo->numNeighbor);
                    }

                    // Copy field into packet.
                    memcpy(&(payload[broadcastTopologyTableSize]),
                           &dataField,
                           sizeof(dataField));
                    broadcastTopologyTableSize += sizeof(dataField);

                    // Get the neighbor list.
                    tmp = row->neighborInfo->list;

                    neighborNum = 0;

                    // if address is of this interface then add others
                    // interface as neighbor information..
                    if (row->linkStateNode
                            == NetworkIpGetInterfaceAddress(node, i))
                    {
                        for (int j = 0 ; j < node->numberInterfaces; j++)
                        {
                            if (j != i)
                            {
                                NodeAddress addr =
                                    NetworkIpGetInterfaceAddress(node, j);

                                memcpy(&(payload[broadcastTopologyTableSize])
                                    ,&(addr)
                                    ,sizeof(NodeAddress));

                                broadcastTopologyTableSize
                                    += sizeof(NodeAddress);

                                neighborNum++;
                             }
                        }

                    }
                    // Get all the neighbors of this link state node.
                    while (tmp != NULL)
                    {
                        // Neighbor address.
                        memcpy(&(payload[broadcastTopologyTableSize]),
                               &(tmp->nodeAddr),
                               sizeof(tmp->nodeAddr));
                        broadcastTopologyTableSize += sizeof(tmp->nodeAddr);

                        tmp = tmp->next;
                        neighborNum++;
                    }

                    if (row->linkStateNode ==
                                NetworkIpGetInterfaceAddress(node, i))
                    {
                        ERROR_Assert(neighborNum ==
                            (row->neighborInfo->numNeighbor
                                + node->numberInterfaces - 1),
                                     "Wrong neighbor list!");
                    } else {

                        ERROR_Assert(neighborNum
                            == row->neighborInfo->numNeighbor,
                                     "Wrong neighbor list!");
                    }
                }

                row = row->next;
            }

            // Now that we know the packet length, update the packet length
            // field of the header.

            headerField.packetType = FSRL_UPDATE_TOPOLOGY_TABLE;
            headerField.packetSize = broadcastTopologyTableSize;

            memcpy(payload, &headerField, sizeof(headerField));

            if (!packetEmpty)
            {
                Message* msg;

                msg = MESSAGE_Alloc(node, MAC_LAYER, 0, MSG_MAC_FromNetwork);
                MESSAGE_PacketAlloc(node,
                                    msg,
                                    broadcastTopologyTableSize,
                                    TRACE_FSRL);

                memcpy(MESSAGE_ReturnPacket(msg),
                       payload,
                       broadcastTopologyTableSize);


                    if (FsrlDebugPackets(node))
                    {
                        char clockStr[MAX_STRING_LENGTH];

                        ctoa(node->getNodeTime(), clockStr);
                        printf("Node %u sending FISHEYE Update packet "
                            "at time %s\n",
                               node->nodeId, clockStr);

                        FsrlPrintTopologyTable(node, fisheye);
                        FsrlPrintFisheyeUpdatePacket(node, msg);
                    }


                    NodeAddress fromAddr = NetworkIpGetInterfaceAddress(
                                               node,
                                               i);

                    if (FsrlDebugDrifter(node))
                    {
                        char addrString[MAX_STRING_LENGTH];

                        IO_ConvertIpAddressToString(fromAddr, addrString);
                        printf("    sending from %s\n", addrString);
                    }

                    /*
                    NetworkIpSendRawMessage(node,
                                msg, //MESSAGE_Duplicate(node, msg),
                                fromAddr,
                                ANY_DEST,
                                i,
                                IPTOS_PREC_INTERNETCONTROL,
                                IPPROTO_FSRL,
                                1);
                   */
                   NetworkIpSendRawMessageWithDelay(node,
                                msg, //MESSAGE_Duplicate(node, msg),
                                fromAddr,
                                ANY_DEST,
                                i,
                                IPTOS_PREC_INTERNETCONTROL,
                                IPPROTO_FSRL,
                                1,
                                (clocktype) (RANDOM_nrand(fisheye->seed)
                                % FISHEYE_RANDOM_JITTER));

                   //MESSAGE_Free(node, msg);

                    isSent = TRUE;

            }
        }

    }
    if (isSent)
    {
        fisheye->stats.controlOH += broadcastTopologyTableSize;
        fisheye->stats.controlPkts += 1;
    }
}


// /**
// FUNCTION   :: FsrlHandleFisheyeUpdateTimeout
// LAYER      :: NETWORK
// PURPOSE    :: Handle timer to start sending out Fisheye update pkts.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlHandleFisheyeUpdateTimeout(Node* node, FsrlData* fisheye)
{
    FisheyeTopologyTableRow* row;
    clocktype randomTime;
    Message* newMsg;


    for (int i = 0; i < node->numberInterfaces; i++)
    {
        row = FsrlFindRowInTopologyTable(
            NetworkIpGetInterfaceAddress(node, i), fisheye);

        if (row)
        {
            // ERROR_Assert(row != NULL, "Wrong topology table!");

            // Increase the sequence number of its own neighbor list
            row->neighborInfo->sequenceNumber++;
        }
    }

    if (fisheye->routingTable.changed == TRUE)
    {
        FsrlFindShortestPath(node);
    }

    FsrlSendFisheyeUpdatePacket(node,
                                fisheye,
                                0,
                                fisheye->parameter.scope);

    // Invoke next intra update.
    randomTime = (clocktype) (RANDOM_nrand(fisheye->seed) % FISHEYE_RANDOM_JITTER);

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_FSRL,
                           MSG_FsrlIntraUpdate);

    MESSAGE_Send(node,
                 newMsg,
                 randomTime + fisheye->parameter.fisheyeUpdateInterval);
}


// /**
// FUNCTION   :: FsrlHandleFisheyeUpdatePacket
// LAYER      :: NETWORK
// PURPOSE    :: Handle fisheye update that has just been received.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to the message strucutre of the packet
// + srcAddr : NodeAddress : Source node of the packet
// + destAddr : NodeAddress : Destination node of the packet
// + incomingInterface : int : Interface index pkt is recved from
// + ttl : int : TTL value from IP header of the packet
// RETURN     :: void : NULL
// **/

static
void FsrlHandleFisheyeUpdatePacket(Node* node,
                                   Message* msg,
                                   NodeAddress srcAddr,
                                   NodeAddress destAddr,
                                   int incomingInterface,
                                   int ttl)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    FsrlFisheyeHeaderField headerField;
    FsrlFisheyeDataField dataField;

    FisheyeTopologyTableRow* tmp;
    FsrlNeighborListItem* prev;
    FsrlNeighborListItem* current;
    FsrlNeighborListItem* currentNext;

    char *payload;
    int packetSize;
    int ttSize;
    int i;




    payload = (char*)MESSAGE_ReturnPacket(msg);
    packetSize = MESSAGE_ReturnPacketSize(msg);

    ttSize = 0;

    // Get header fields.
    memcpy(&headerField, &(payload[ttSize]), sizeof(headerField));
    ttSize += sizeof(headerField);

    // Start to update topology table from the packet received for each
    // link state entry.
    while (ttSize < packetSize)
    {
        tmp = (FisheyeTopologyTableRow*)
            MEM_malloc(sizeof(FisheyeTopologyTableRow));

        tmp->neighborInfo = (FisheyeNeighborInfo*)
            MEM_malloc(sizeof(FisheyeNeighborInfo));

        ERROR_Assert(tmp != NULL, "Memory error!");

        tmp->index = NULL;

        memcpy(&dataField, &(payload[ttSize]), sizeof(dataField));
        ttSize += sizeof(dataField);

        tmp->linkStateNode = dataField.destAddr;

        tmp->neighborInfo->sequenceNumber =
            FsrlFisheyeDataFieldGetSeqNum(dataField.dataField);
        tmp->neighborInfo->numNeighbor=
            FsrlFisheyeDataFieldGetNumNeighbour(dataField.dataField);

        tmp->neighborInfo->list = NULL;
        current = NULL;
        prev = NULL;

        for (i = 0; i < tmp->neighborInfo->numNeighbor; i ++)
        {
            current = (FsrlNeighborListItem*)
                MEM_malloc(sizeof(FsrlNeighborListItem));

            ERROR_Assert(current != NULL, "Memory error!");

            // Neighbor address.
            memcpy(&(current->nodeAddr),
                   &(payload[ttSize]),
                   sizeof(current->nodeAddr));

            ttSize += sizeof(current->nodeAddr);

            current->next = NULL;

            if (prev != NULL)
            {
                prev->next = current;
                prev = current;
            }
            else
            {
                tmp->neighborInfo->list = current;
                prev = current;
            }
        }

        tmp->timestamp = node->getNodeTime();
        tmp->removalTimestamp = 0;
        tmp->incomingInterface = incomingInterface;
        tmp->fromAddr = srcAddr;
        tmp->next = NULL;

        // At this point, the destination information set should have
        // already been assigned.

        if (FsrlDebugTopologyTable(node))
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(tmp->linkStateNode, addrString);
            printf("    examining %s\n", addrString);
        }

        // Split horizon
        //if (!(tmp->linkStateNode == fisheye->ipAddress))
        if (!NetworkIpIsMyIP(node, tmp->linkStateNode))
        {

            if (FsrlUpdateTopologyTable(node, tmp, incomingInterface))
            {
                // There is an update.
                fisheye->routingTable.changed = TRUE;
            }
            else
            {
                // No update, so free tmp.
                current = tmp->neighborInfo->list;

                while (current != NULL)
                {
                    currentNext = current->next;
                    MEM_free(current);
                    current = currentNext;
                }

                MEM_free(tmp->neighborInfo);
                MEM_free(tmp);
            }
        }
        else // Sender's neighbor entry is this node, free tmp.
        {
            if (FsrlDebugTopologyTable(node))
            {
                printf("    split horizon\n");
            }

            current = tmp->neighborInfo->list;

            while (current != NULL)
            {
                currentNext = current->next;
                MEM_free(current);
                current = currentNext;
            }

            MEM_free(tmp->neighborInfo);
            MEM_free(tmp);
        }
    }
}


// /**
// FUNCTION   :: FsrlInitLandmarkTable
// LAYER      :: NETWORK
// PURPOSE    :: Initialize the landmark table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlInitLandmarkTable(Node* node, FsrlData* fisheye)
{
    memset(&fisheye->landmarkTable, 0, sizeof(FsrlLandmarkTable));

    fisheye->landmarkTable.row = NULL;
    fisheye->landmarkTable.ownSequenceNumber = 0;
    fisheye->landmarkTable.size = 0;
}


// /**
// FUNCTION   :: FsrlInitDrifterTable
// LAYER      :: NETWORK
// PURPOSE    :: Initialize the drifter table.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlInitDrifterTable(Node* node, FsrlData* fisheye)
{
    // Zero drifter table at start of simulation.
    memset(&(fisheye->drifterTable), 0, sizeof(FsrlDrifterTable));

    fisheye->drifterTable.row  = NULL;
    fisheye->drifterTable.size = 0;
    fisheye->drifterTable.ownSequenceNumber = 0;
}



// /**
// FUNCTION   :: FsrlHandleLandmarkUpdateTimeout
// LAYER      :: NETWORK
// PURPOSE    :: Handle timer to send out LANMAR update packet.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fisheye : FsrlData* : Pointer to LANMAR routing data structure
// RETURN     :: void : NULL
// **/

static
void FsrlHandleLandmarkUpdateTimeout(Node* node, FsrlData* fisheye)
{
    FsrlLandmarkTableRow* lmDvrow;
    clocktype randomTime;
    clocktype delay;
    Message* newMsg;


    int i = 0;

    #ifdef DRIFTER

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (FsrlIsDrifter(node,
                          fisheye,
                          0,
                          fisheye->parameter.scope,
                          i))
        {
            fisheye->drifterTable.ownSequenceNumber +=
                FISHEYE_SEQUENCE_NO_INCREMENT;

            FsrlUpdateDrifterTable(
                node,
                NetworkIpGetInterfaceAddress(node, i),
                NetworkIpGetInterfaceAddress(node, i),
                -1,
                0,
                fisheye->drifterTable.ownSequenceNumber);
        }
        else
        {
            // If we are not a drifter, then we need to check to make
            // sure that we are not in the drifter table.  If so, we
            // need to remove ourself from the list.
            if (FsrlCheckDrifterTable(node,
                                      fisheye,
                                      NetworkIpGetInterfaceAddress(
                                          node, i)))
            {
                FsrlDeleteFromDrifterTable(node,
                        NetworkIpGetInterfaceAddress(node, i));
            }

        }
    }

    FsrlCheckExpiredEntryInDrifterTable(node, fisheye);
    #endif


    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (fisheye->isLandmark[i])
        {
            fisheye->landmarkTable.ownSequenceNumber +=
                FISHEYE_SEQUENCE_NO_INCREMENT;
            lmDvrow = FsrlFindRowInLandmarkTable(
                                        fisheye,
                                        NetworkIpGetInterfaceAddress(
                                        node, i));
            ERROR_Assert(lmDvrow != NULL, "Wrong landmark table!");

            lmDvrow->sequenceNumber
                = fisheye->landmarkTable.ownSequenceNumber;
        }
    }

    FsrlSendLanmarUpdatePacket(node,
                               fisheye,
                               0,
                               fisheye->parameter.scope);

    // Invoke next landmark DV update.
    randomTime = (clocktype)(RANDOM_nrand(fisheye->seed) % FISHEYE_RANDOM_JITTER);

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_FSRL,
                           MSG_FsrlLMUpdate);

    delay = randomTime + fisheye->parameter.landmarkUpdateInterval;

    MESSAGE_Send(node, newMsg, delay);
}


// /**
// FUNCTION   :: FsrlHandleLanmarUpdatePacket
// LAYER      :: NETWORK
// PURPOSE    :: Handle landmark update packet just received.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to the message strucutre of the packet
// + srcAddr : NodeAddress : Source node of the packet
// + destAddr : NodeAddress : Destination node of the packet
// + incomingInterface : int : Interface index pkt is recved from
// + ttl : int : TTL value from IP header of the packet
// RETURN     :: void : NULL
// **/

static
void FsrlHandleLanmarUpdatePacket(Node* node,
                                  Message* msg,
                                  NodeAddress srcAddr,
                                  NodeAddress destAddr,
                                  int incomingInterface,
                                  int ttl)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);

    int landmarkTableHeaderSize = sizeof(FsrlLanmarUpdateHeaderField);
    FsrlLanmarUpdateHeaderField headerField;
    FsrlLanmarUpdateLandmarkField landmarkField;
    FsrlLanmarUpdateDrifterField drifterField;

    char *payload;
    int ttSize;
    int i;

    payload = (char*)MESSAGE_ReturnPacket(msg);

    // pointer to payload
    ttSize = 0;

    // Get header field.
    memcpy(&headerField, &(payload[ttSize]), sizeof(headerField));
    ttSize += sizeof(headerField);

    // Loop through LM entries.
    for (i = 0; i < headerField.numLandmark; i++)
    {
        // Landmark address.
        memcpy(&(landmarkField), &(payload[ttSize]), sizeof(landmarkField));
        ttSize += sizeof(landmarkField);

        // split horizon

        if (!(NetworkIpGetInterfaceAddress(node, incomingInterface) ==
              landmarkField.nextHopAddr))
        {
            int distance = landmarkField.distance;

            if (FsrlDebugLandmark(node))
            {
                printf("    node%u updating landmark table due to "
                       "update pkt\n",
                       node->nodeId);
            }

            ERROR_Assert(distance <= FISHEYE_INFINITY, "Wrong distance!");

            if (distance < FISHEYE_INFINITY)
            {
                distance ++;
            }

            FsrlUpdateLandmarkTable(
                node,
                landmarkField.landmarkAddr,
                srcAddr,
                incomingInterface,
                landmarkField.numMember,
                distance,
                landmarkField.sequenceNumber,
                incomingInterface);

            // If the advertise landmark have less than or equal to the
            // number of members we currently have, then we may be the
            // new landmark.  Thus, we need to possibly update our
            // landmark table.

            if (landmarkField.numMember <=
                    fisheye->numLandmarkMember[incomingInterface])
            {
                if (FsrlDebugLandmark(node))
                {
                    printf("    advertised landmark has <= # of members"
                           " we have,\n      we could be new landmark\n");
                }


               FsrlUpdateLandmarkTable(
                   node,
                   NetworkIpGetInterfaceAddress(node, incomingInterface),
                   NetworkIpGetInterfaceAddress(node, incomingInterface),
                   -1,
                   fisheye->numLandmarkMember[incomingInterface],
                   0,
                   fisheye->landmarkTable.ownSequenceNumber,
                   incomingInterface);
            }
        }
        else
        {
            if (FsrlDebugLandmark(node))
            {
                printf("    skip entry due to split horizon\n");
            }
        }
    }

    // Loop through drifter entries.
    for (i = 0; i < headerField.numDrifter; i++)

    {
        // Drifter address.
        memcpy(&(drifterField), &(payload[ttSize]), sizeof(drifterField));
        ttSize += sizeof(drifterField);


        if (drifterField.nextHopAddr ==
            NetworkIpGetInterfaceAddress(node, incomingInterface))
        {
            int distance = drifterField.distance;

            ERROR_Assert(distance <= FISHEYE_INFINITY, "Wrong distance!");

            if (distance < FISHEYE_INFINITY)
            {
                distance++;
            }

            if (FsrlDebugLandmark(node))
            {
                printf("    next hop to drifter!\n");
            }

            FsrlUpdateDrifterTable(
                node,
                drifterField.drifterAddr,
                srcAddr,
                incomingInterface,
                distance,
                drifterField.sequenceNumber);
        }
        else
        {
            if (FsrlDebugLandmark(node))
            {
                printf("    not the next hop to drifter\n");
            }
        }
    }

    ERROR_Assert ((unsigned)ttSize ==
                  (landmarkTableHeaderSize +
                   headerField.numLandmark * sizeof(landmarkField) +
                   headerField.numDrifter * sizeof(drifterField)),
                  "Wrong packet size!");
}



// /**
// FUNCTION   :: FsrlPrintRoutingStats
// LAYER      :: NETWORK
// PURPOSE    :: Print out statistics at end of simulation.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: void : NULL
// **/

static
void FsrlPrintRoutingStats(Node* node)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);

    if (!fisheye->statsPrinted)
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "The number of intra-scope updates = %d",
                fisheye->stats.intraScopeUpdate);
        IO_PrintStat(node,
                     "Network",
                     "LANMAR",
                     ANY_DEST,
                     -1,
                     buf);

        sprintf(buf, "The number of landmark updates = %d",
                fisheye->stats.LMUpdate);
        IO_PrintStat(node,
                     "Network",
                     "LANMAR",
                     ANY_DEST,
                     -1,
                     buf);

        sprintf(buf, "Control overhead in bytes = %d",
                fisheye->stats.controlOH);
        IO_PrintStat(node,
                     "Network",
                     "LANMAR",
                     ANY_DEST,
                     -1,
                     buf);

        sprintf(buf, "Number of control packets = %d",
                fisheye->stats.controlPkts);
        IO_PrintStat(node,
                     "Network",
                     "LANMAR",
                     ANY_DEST,
                     -1,
                     buf);
        sprintf(buf, "Packets dropped within the scope = %d",
                fisheye->stats.dropInScope);
        IO_PrintStat(node,
                     "Network",
                     "LANMAR",
                     ANY_DEST,
                     -1,
                     buf);
        sprintf(buf, "Packets dropped due to no landmark matching = %d",
                fisheye->stats.dropNoLM);
        IO_PrintStat(node,
                     "Network",
                     "LANMAR",
                     ANY_DEST,
                     -1,
                     buf);
        sprintf(buf, "Packets dropped due to no drifter matching = %d",
                fisheye->stats.dropNoDF);
        IO_PrintStat(node,
                     "Network",
                     "LANMAR",
                     ANY_DEST,
                     -1,
                     buf);
        sprintf(buf, "Packets dropped due to no route= %d",
                fisheye->stats.dropNoRoute);
        IO_PrintStat(node,
                     "Network",
                     "LANMAR",
                     ANY_DEST,
                     -1,
                     buf);
        fisheye->statsPrinted = TRUE;

       }
}


// /**
// FUNCTION   :: FsrlMacLayerStatusHandler
// LAYER      :: NETWORK
// PURPOSE    :: Handle event that MAC layer dropped packets.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : const Message* : Pointer to message structure of the event
// + nextHop : const NodeAddress : Next hop node of dropped packet
// + interfaceIndex : const int : Interface index the pkt is sent on
// RETURN     :: void : NULL
// **/

void FsrlMacLayerStatusHandler(Node* node,
                               const Message* msg,
                               const NodeAddress nextHop,
                               const int interfaceIndex)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);

    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_NETWORK_PacketDropped:
        {
            IpHeaderType* ipHeader;

            ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

            if (FsrlDebug(node))
            {
                printf("Node %d got packet drop status handler\n",
                       node->nodeId);
            }

            if (ipHeader->ip_p == IPPROTO_FSRL)
            {
                return;
            }
            else
            {
                // Delete neigbor that is no longer reachable.
                FsrlDeleteNeighbor(node, nextHop);

                // We need to make sure the neighbor that is no longer
                // reachable is not the next hop to a drifter.
                // If neighbor is the next hop, remove neighbor from the
                // drifter table.
                FsrlDeleteFromDrifterTable(node, nextHop);

                // We need to make sure the neighbor that is no longer
                // reachable is not a landmark.  If neighbor is a landmark,
                // remove neighbor from the landmark table.

                FsrlDeleteFromLandmarkTable(node, nextHop, interfaceIndex);

                // Set distance metric to infinity and increment seqno by 1.
                FsrlInvalidateEntryInLandmarkTable(node, nextHop);

                if (FsrlDebugLandmark(node))
                {
                    FsrlPrintLandmarkTable(node, fisheye);
                }
            }

            break;
        }

        default:
            ERROR_ReportErrorArgs("FSRL: Unknown MSG type %d!\n",
                                  msg->eventType);
    }
}


// /**
// FUNCTION   :: FsrlHandleProtocolEvent
// LAYER      :: NETWORK
// PURPOSE    :: Handle self-scheduled messages such as timers
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to message structure of the event
// RETURN     :: void : NULL
// **/

void FsrlHandleProtocolEvent(Node* node, Message* msg)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    char clockStr[MAX_STRING_LENGTH];

    ctoa(node->getNodeTime(), clockStr);

    ERROR_Assert(fisheye != NULL, "Memory error!");

    if (FsrlDebugTimers(node))
    {
        printf("Node %u got protocol event timer at %s\n",
               node->nodeId, clockStr);
    }

    switch (msg->eventType)
    {
        case MSG_FsrlNeighborTimeout:
        {
            if (FsrlDebugTimers(node))
            {
                printf("    got MSG_FsrlNeighborTimeout\n");
            }

            FsrlHandleNeighborTimeout(node, fisheye);

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_FsrlIntraUpdate:
        {
            if (FsrlDebugTimers(node))
            {
                printf("    got MSG_FsrlIntraUpdate\n");
            }

            fisheye->stats.intraScopeUpdate++;
            FsrlHandleFisheyeUpdateTimeout(node, fisheye);

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_FsrlLMUpdate:
        {
            if (FsrlDebugTimers(node))
            {
                printf("    got MSG_FsrlLMUpdate\n");
            }

            FsrlCheckExpiredEntryInLandmarkTable(node, FALSE);

            FsrlHandleLandmarkUpdateTimeout(node, fisheye);

            MESSAGE_Free(node, msg);

            fisheye->stats.LMUpdate++;

            break;
        }
        default:
            ERROR_Assert(0, "Unknown MSG type!");
    }
}


// /**
// FUNCTION   :: FsrlHandleProtocolPacket
// LAYER      :: NETWORK
// PURPOSE    :: Handle routing packets
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to message structure of the packet
// + srcAddr : NodeAddress : Source node of the packet
// + destAddr : NodeAddress : Destination node of the packet
// + incomingInterface : int : From which interface the packet is received
// + ttl : int : TTL value in the IP header
// RETURN     :: void : NULL
// **/

void FsrlHandleProtocolPacket(Node* node,
                              Message* msg,
                              NodeAddress srcAddr,
                              NodeAddress destAddr,
                              int incomingInterface,
                              int ttl)
{
    FsrlData* fisheye = (FsrlData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_FSRL);
    unsigned char *packetType = (unsigned char*)MESSAGE_ReturnPacket(msg);
    char clockStr[MAX_STRING_LENGTH];

    ctoa(node->getNodeTime(), clockStr);

    switch (*packetType)
    {
        case FSRL_UPDATE_TOPOLOGY_TABLE:
        {
            if (FsrlDebugPackets(node))
            {
                char addrString[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);
                IO_ConvertIpAddressToString(srcAddr, addrString);

                printf("Node %u got FISHEYE Update packet from "
                       "node %s at time %s interface %d\n",
                       node->nodeId, addrString, clockStr
                        , incomingInterface);

                FsrlPrintFisheyeUpdatePacket(node, msg);
            }

            FsrlUpdateMyNeighborInfo(node,
                                     srcAddr,
                                     incomingInterface);

            FsrlHandleFisheyeUpdatePacket(node,
                                          msg,
                                          srcAddr,
                                          destAddr,
                                          incomingInterface,
                                          ttl);

            MESSAGE_Free(node, msg);

            break;
        }
        case FSRL_UPDATE_DV:
        {
            if (FsrlDebugPackets(node))
            {
                char addrString[MAX_STRING_LENGTH];
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                IO_ConvertIpAddressToString(srcAddr, addrString);

                printf("Node %u got LANMAR Update packet from "
                       "node %s at time %s\n",
                       node->nodeId, addrString, clockStr);

                FsrlPrintTopologyTable(node, fisheye);
                FsrlPrintLanmarUpdatePacket(node, msg);
                FsrlPrintLandmarkTable(node, fisheye);
            }

            FsrlUpdateMyNeighborInfo(node,
                                     srcAddr,
                                     incomingInterface);

            FsrlHandleLanmarUpdatePacket(node,
                                         msg,
                                         srcAddr,
                                         destAddr,
                                         incomingInterface,
                                         ttl);

            if (FsrlDebugPackets(node))
            {
                printf("    after LANMAR packet processing\n");
                FsrlPrintTopologyTable(node, fisheye);
                FsrlPrintLandmarkTable(node, fisheye);
            }

            MESSAGE_Free(node, msg);

            break;
        }
        default:
            ERROR_Assert(0, "Unknown packet type!");
    }
}


// /**
// FUNCTION   :: FsrlRouterFunction
// LAYER      :: NETWORK
// PURPOSE    :: Is called when network layer needs to route a packet
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to message structure of the packet
// + destAddr : NodeAddress : Destination node of the packet
// + previousHopAddress : NodeAddress : Previous hop node
// + PacketWasRouted : BOOL* : For indicating whehter the packet is routed
//                             in this function
// RETURN     :: void : NULL
// **/

void FsrlRouterFunction(Node* node,
                        Message* msg,
                        NodeAddress destAddr,
                        NodeAddress previousHopAddress,
                        BOOL* PacketWasRouted)
{
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    // Control are to be handled by LANMAR
    if (ipHeader->ip_p == IPPROTO_FSRL)
    {
        return;
    }

    // Data packets are forwarded here...

    if (NetworkIpIsMyIP(node, destAddr))
    {
        if (FsrlDebugPackets(node))
        {
            printf("Node %d got DATA intended for it\n", node->nodeId);
        }

        *PacketWasRouted = FALSE;
    }
    else
    {
        *PacketWasRouted = TRUE;

        FsrlRoutePacket(node, msg);
    }
}


// /**
// FUNCTION   :: FsrlInit
// LAYER      :: NETWORK
// PURPOSE    :: Initialization Function for LANMAR Protocol
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fsrl : FsrlData** : Address of data structure pointer to be filled in
// + nodeInput : const NodeInput* : Pointer to NodeInput
// + interfaceIndex : int : Interface index
// RETURN     :: void : NULL
// **/

void FsrlInit(Node* node,
              FsrlData** fsrlPtr,
              const NodeInput* nodeInput,
              int interfaceIndex)
{
    int minMemberThreshold;
    double alpha;
    int scope;

    clocktype neighborTOIntervalClk;
    clocktype fisheyeUpdateIntervalClk;
    clocktype landmarkUpdateIntervalClk;
    clocktype fisheyeMaxAgeClk;
    clocktype landmarkMaxAgeClk;
    clocktype drifterMaxAgeClk;

    BOOL retVal;

    clocktype randomTime;
    Message* newMsg;

    FsrlData* fisheye = (FsrlData*) MEM_malloc(sizeof(FsrlData));

    *fsrlPtr = fisheye;

    RANDOM_SetSeed(fisheye->seed,
                   node->globalSeed,
                   node->nodeId,
                   ROUTING_PROTOCOL_FSRL,
                   interfaceIndex);

    // Read config file for the LANMAR
    IO_ReadInt(node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
               nodeInput,
               "LANMAR-MIN-MEMBER-THRESHOLD",
               &retVal,
               &minMemberThreshold);

    if (retVal == FALSE)
    {
        minMemberThreshold = LANMAR_DEFAULT_MIN_MEMBER_THRESHOLD;
    }

    if (FsrlInitDebug(node))
    {
        printf("retVal = %d LANMAR-MIN-MEMBER-THRESHOLD = %d\n",
            retVal, minMemberThreshold);
    }

    IO_ReadDouble(node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
                  nodeInput,
                  "LANMAR-ALPHA",
                  &retVal,
                  &alpha);

    if (retVal == FALSE)
    {
        alpha = FISHEYE_DEFAULT_ALPHA;
    }

    if (FsrlInitDebug(node))
    {
        printf("retVal = %d LANMAR-ALPHA = %lf\n",retVal, alpha);
    }

    IO_ReadInt(node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
               nodeInput,
               "LANMAR-FISHEYE-SCOPE",
               &retVal,
               &scope);

    if (retVal == FALSE)
    {
        scope = FISHEYE_DEFAULT_SCOPE;
    }

    if (FsrlInitDebug(node))
    {
        printf("retVal = %d LANMAR-FISHEYE-SCOPE = %d\n",retVal, scope);
    }

    IO_ReadTime(node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "LANMAR-NEIGHBOR-TIMEOUT-INTERVAL",
                &retVal,
                &neighborTOIntervalClk);

    if (retVal == FALSE)
    {
        neighborTOIntervalClk = LANMAR_DEFAULT_NEIGHBOR_TIMEOUT_INTERVAL;
    }

    if (FsrlInitDebug(node))
    {
        char timeInSecond[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(neighborTOIntervalClk, timeInSecond);
        printf("retVal = %d LANMAR-NEIGHBOR-TIMEOUT-INTERVAL = %s\n",
            retVal, timeInSecond);
    }

    IO_ReadTime(node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "LANMAR-FISHEYE-UPDATE-INTERVAL",
                &retVal,
                &fisheyeUpdateIntervalClk);

    if (retVal == FALSE)
    {
        fisheyeUpdateIntervalClk = LANMAR_DEFAULT_FISHEYE_UPDATE_INTERVAL;
    }

    if (FsrlInitDebug(node))
    {
        char timeInSecond[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(fisheyeUpdateIntervalClk, timeInSecond);
        printf("retVal = %d LANMAR-FISHEYE-UPDATE-INTERVAL = %s\n",
            retVal, timeInSecond);
    }

   IO_ReadTime(node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
               nodeInput,
               "LANMAR-LANDMARK-UPDATE-INTERVAL",
               &retVal,
               &landmarkUpdateIntervalClk);

    if (retVal == FALSE)
    {
        landmarkUpdateIntervalClk = LANMAR_DEFAULT_LANDMARK_UPDATE_INTERVAL;
    }

    if (FsrlInitDebug(node))
    {
        char timeInSecond[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(landmarkUpdateIntervalClk, timeInSecond);
        printf("retVal = %d LANMAR-LANDMARK-UPDATE-INTERVAL = %s\n",
            retVal, timeInSecond);
    }

    IO_ReadTime(node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "LANMAR-FISHEYE-MAX-AGE",
                &retVal,
                &fisheyeMaxAgeClk);

    if (retVal == FALSE)
    {
        fisheyeMaxAgeClk = LANMAR_DEFAULT_FISHEYE_MAX_AGE;
    }

    if (FsrlInitDebug(node))
    {
        char timeInSecond[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(fisheyeMaxAgeClk, timeInSecond);
        printf("retVal = %d LANMAR-FISHEYE-MAX-AGE = %s\n",
            retVal, timeInSecond);
    }

    IO_ReadTime(node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "LANMAR-LANDMARK-MAX-AGE",
                &retVal,
                &landmarkMaxAgeClk);

    if (retVal == FALSE)
    {
        landmarkMaxAgeClk = LANMAR_DEFAULT_LANDMARK_MAX_AGE;
    }

    if (FsrlInitDebug(node))
    {
        char timeInSecond[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(landmarkMaxAgeClk, timeInSecond);
        printf("retVal = %d LANMAR-LANDMARK-MAX-AGE = %s\n",
            retVal, timeInSecond);
    }

    IO_ReadTime(node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "LANMAR-DRIFTER-MAX-AGE",
                &retVal,
                &drifterMaxAgeClk);

    if (retVal == FALSE)
    {
        drifterMaxAgeClk = LANMAR_DEFAULT_DRIFTER_MAX_AGE;
    }

    if (FsrlInitDebug(node))
    {
        char timeInSecond[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(drifterMaxAgeClk, timeInSecond);
        printf("retVal = %d LANMAR-DRIFTER-MAX-AGE = %s\n",
            retVal, timeInSecond);
    }

    fisheye->parameter.minMemberThreshold = minMemberThreshold;
    fisheye->parameter.alpha = alpha;
    fisheye->parameter.scope = (short) scope;

    fisheye->parameter.neighborTOInterval = neighborTOIntervalClk;
    fisheye->parameter.fisheyeUpdateInterval = fisheyeUpdateIntervalClk;
    fisheye->parameter.landmarkUpdateInterval = landmarkUpdateIntervalClk;
    fisheye->parameter.fisheyeMaxAge = fisheyeMaxAgeClk;
    fisheye->parameter.landmarkMaxAge = landmarkMaxAgeClk;
    fisheye->parameter.drifterMaxAge = drifterMaxAgeClk;

    fisheye->ipAddress = NetworkIpGetInterfaceAddress(node, interfaceIndex);
    fisheye->numHostBits = NetworkIpGetInterfaceNumHostBits(node,
                                                            interfaceIndex);
    fisheye->isLandmark[interfaceIndex] = FALSE;
    fisheye->numLandmarkMember[interfaceIndex] = 0;


    fisheye->statsPrinted = FALSE;

    FsrlInitTopologyTable(node, fisheye);
    FsrlInitRoutingTable(node, fisheye);
    FsrlInitLandmarkTable(node, fisheye);
    FsrlInitDrifterTable(node, fisheye);

    // Update landmark table in the case that we want nodes with
    // no neighbors detected to also be the landmark.

   for (int index = 0 ; index < node->numberInterfaces; index++)
   {
        FsrlUpdateLandmarkTable(
            node,
            NetworkIpGetInterfaceAddress(node, index),
            NetworkIpGetInterfaceAddress(node, index),
            -1,
            fisheye->numLandmarkMember[index],
            0,
            fisheye->landmarkTable.ownSequenceNumber,
            index);
    }
    if (FsrlDebugInDetail(node))
    {
        printf("Node %d after initializing...\n", node->nodeId);
        FsrlPrintTopologyTable(node, fisheye);
    }

    // Initialize variables
    FsrlInitStats(node, fisheye);

    NetworkIpSetRouterFunction(
        node,
        &FsrlRouterFunction,
        interfaceIndex);

    NetworkIpSetMacLayerStatusEventHandlerFunction(
        node,
        &FsrlMacLayerStatusHandler,
        interfaceIndex);

    // Start topology table exchange
    randomTime = (clocktype) (RANDOM_nrand(fisheye->seed) % FISHEYE_RANDOM_JITTER);

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_FSRL,
                           MSG_FsrlIntraUpdate);

    MESSAGE_Send(node, newMsg, randomTime);

    // Initial time for neighbor TO.
    randomTime = fisheye->parameter.fisheyeUpdateInterval +
                 (clocktype) (RANDOM_nrand(fisheye->seed) % FISHEYE_RANDOM_JITTER);

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_FSRL,
                           MSG_FsrlNeighborTimeout);

    MESSAGE_Send(node, newMsg, randomTime);

    // Initial time for landmark update.
    randomTime = (clocktype) (RANDOM_nrand(fisheye->seed) % FISHEYE_RANDOM_JITTER);
    randomTime = randomTime +
                 scope * fisheye->parameter.fisheyeUpdateInterval;

    newMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_FSRL,
                           MSG_FsrlLMUpdate);

    MESSAGE_Send(node, newMsg, randomTime);
}


// /**
// FUNCTION   :: FsrlFinalize
// LAYER      :: NETWORK
// PURPOSE    :: Finalization function, print out statistics
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: void : NULL
// **/

void FsrlFinalize(Node* node)
{
    if (node->networkData.networkStats == TRUE)
    {
        FsrlPrintRoutingStats(node);
    }
}

