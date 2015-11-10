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
//------------------------------------------------------------------------
//
// file : igrp.c
// purpose : simulating the Interior Gateway Routing Protocol (IGRP)
// referance : Ref  http://www.cisco.com/warp/public/103/5.html
//
//
// ASSUMPTIONS AND OMITTED FEATURES
// --------------------------------
// 1) Currently following fields are not used in metric
//   calculation of IGRP.
//      mtu           (maximum transferable unit)
//      reliability   (channel reliability / rate of error)
//      load          (current load on the channel)
//
// 2) Currently we are assuming only one type of service called
//   DEFAULT_TOS
//
// 3) IGRP currently exchanges only systems routes and exterior
//   routes but no interior routes.
//
// 4) IGRP currently does not support applying offsets to the
//   routing metric.
//
// 5) IGRP currently does not support point to point routing
//   information exchange.
//
// 6) IGRP currently does not allow controlling traffic distribution.
//
// 7) This version of IGRP does not support validating source IP address
//   and unequal cost load balancing
//-------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "api.h"
#include "network_ip.h"

#include "network_dualip.h"

#include "main.h"
#include "routing_igrp.h"
#include "fileio.h"
#include "mapping.h"
#include "buffer.h"
#include "trace.h"

// Added for Route-Redistribution
#define ROUTE_REDISTRIBUTE

#define DEBUG_FAULT 0
#define IGRP_DEBUG  0

//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpGetSubnetMaskFromIPAddressType()
//
// PURPOSE      : Getting subnet mask from IP address type. IP address
//                type can be CLASS A, CLASS B, CLASS C
//
// PARAMETERS   : Ip address type either of CLASS A, CLASS B, or CLASS C
//
// RETURN VALUE : subnet mask
//
// ASSUMPTION   : CLASS D should never appear in this function.
//
//-------------------------------------------------------------------------
static
unsigned int IgrpGetSubnetMaskFromIPAddressType(IpAddressType ipAddressType)
{
    switch (ipAddressType)
    {
        case ADDRESS_TYPE_CLASS_A :
        {
            return 0xFF000000;
        }
        case ADDRESS_TYPE_CLASS_B :
        {
            return 0xFFFF0000;
        }
        case ADDRESS_TYPE_CLASS_C :
        {
            return 0xFFFFFF00;
        }
        case ADDRESS_TYPE_CLASS_D :
        {
            return 0xFFFFFFFF;
        }
        default:
        {
            ERROR_Assert(FALSE, "Invalid IP Address class !!!");
        }
    }
    // control MUST NOT reach here.
    ERROR_Assert(FALSE, "Invalid IP Address class !!!");
    return 0x00000000;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpGetIpAddressTypeFromIpAddress()
//
// PURPOSE      : Getting Ip address type from IP address. IP address
//                type can be CLASS A, CLASS B, CLASS C
//
// PARAMETERS   : Ip address.
//
// RETURN VALUE : Ip address type (CLASS A, CLASS B or CLASS C).
//
// ASSUMPTION   : This Function should not never CLASS D. IP address
//                is given in form of a 4-byte integer.
//
//-------------------------------------------------------------------------
static
IpAddressType IgrpGetIpAddressTypeFromIpAddress(NodeAddress address)
{
    unsigned char shiftByBit = 32;
    do
    {
        unsigned int addressType = ((unsigned) address >> (--shiftByBit));
        switch (addressType)
        {
            case ADDRESS_TYPE_CLASS_A :
            {
                return ADDRESS_TYPE_CLASS_A;
            }
            case ADDRESS_TYPE_CLASS_B :
            {
                return ADDRESS_TYPE_CLASS_B;
            }
            case ADDRESS_TYPE_CLASS_C :
            {
                return ADDRESS_TYPE_CLASS_C;
            }
            case ADDRESS_TYPE_CLASS_D :
            {
                return ADDRESS_TYPE_CLASS_D;
            }
            default :
            {
                if (shiftByBit < 28)
                {
                    ERROR_Assert(FALSE, "INVALID IP ADDRESS !!!");
                }
            }
        }
    } while (TRUE);
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpReturnSubnetMaskFromIPAddress()
//
// PURPOSE      : getting subnet mask form ip address
//
// PARAMETERS   : ipAddress of the node.
//
// RETURN VALUE : subnet mask
//
// ASSUMPTION   : IP address is given in form of a 4-byte integer.
//
//-------------------------------------------------------------------------
static
unsigned int IgrpReturnSubnetMaskFromIPAddress(NodeAddress address)
{
    return IgrpGetSubnetMaskFromIPAddressType(
               IgrpGetIpAddressTypeFromIpAddress(address));
}

//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpGetPropDelay()
//
// PURPOSE      : getting the propagation delay information
//
// PARAMETERS   :  node - the node who's bandwidth is needed.
//                 interfaceIndex - interface Index
//
// RETURN VALUE : propagation delay
//
// ASSUMPTION   : Array is exactly 3-byte long.
//
//-------------------------------------------------------------------------
static
clocktype IgrpGetPropDelay(Node *node, int interfaceIndex)
{
    clocktype propdelay = NetworkIpGetPropDelay(node,interfaceIndex)/10;
    return propdelay;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpGetInvBandwidth()
//
// PURPOSE      : getting the inverted bandwidth information
//
// PARAMETERS   :  node - the node who's bandwidth is needed.
//                 interfaceIndex - interface Index
//
// RETURN VALUE : inverted bandwidth
//
// ASSUMPTION   : Bandwidth read from interface is in from of bps unit.
//                To invert the bandwidth we use the equation
//                10000000 / bandwidth. Where bandwidth is in Kbps unit.
//
//-------------------------------------------------------------------------
static
unsigned int IgrpGetInvBandwidth(Node *node, int interfaceIndex)
{
    unsigned int invBandwidth = 0;
    unsigned int bandwidth =
    (unsigned int)NetworkIpGetBandwidth(node,interfaceIndex);
    bandwidth = bandwidth / 1000;
    invBandwidth = 10000000 / bandwidth;
    return invBandwidth;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpConvertCharArrayToInt()
//
// PURPOSE      : Converting the content of a charecter type array to
//                an unsigned integer
//
// PARAMETERS   :  arr - The charecter type array
//
// RETURN VALUE : The unsigned int
//
// ASSUMPTION   : The input array is exactly 3-byte long.
//
//-------------------------------------------------------------------------
static
unsigned int IgrpConvertCharArrayToInt(unsigned char arr[])
{
    unsigned value = ((0xFF & arr[2]) << 16)
                     + ((0xFF & arr[1]) << 8)
                     + (0xFF & arr[0]);
    return value;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IsDestUnreachable()
//
// PURPOSE      : Check whether a destination is unreachable
//
// PARAMETERS   : delay - the 3-byte delay vaule
//
// RETURN VALUE : Returns true if delay if 0xFFFFFF, false otherwise
//
// ASSUMPTION   : Array delay is exactly 3-byte long.
//
//-------------------------------------------------------------------------
static
BOOL IsDestUnreachable(unsigned char delay[])
{
    BOOL isMatchingDelay = (IgrpConvertCharArrayToInt(delay) ==
                               DESTINATION_INACCESSABLE);
    return isMatchingDelay;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpPrintIgrpRoutingTable()
//
// PURPOSE      : Printing the IGRP routing table for debugging purpose
//
// PARAMETERS   : node - Node which is printing routing table
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpPrintIgrpRoutingTable(Node* node)
{
    int i = 0, j = 0;
    IgrpRoutingTable* igrpRoutingTable = NULL;
    int numIgrpRouteTableEntries = 0;

    RoutingIgrp* igrp = (RoutingIgrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IGRP);

    char clockStr[MAX_STRING_LENGTH] = {0};

    printf("----------------------------------------------------------"
           "----------------------------------------\n"
           "node : %u\n"
           "----------------------------------------------------------"
           "----------------------------------------\n",
           node->nodeId);

    igrpRoutingTable = (IgrpRoutingTable*)
        BUFFER_GetData(&(igrp->igrpRoutingTable));

    numIgrpRouteTableEntries = BUFFER_GetCurrentSize(
        &(igrp->igrpRoutingTable)) / sizeof (IgrpRoutingTable);

    for (i = 0; i < numIgrpRouteTableEntries; i++)
    {
        IgrpRoutingTableEntry* routeEntry = (IgrpRoutingTableEntry*)
            BUFFER_GetData(&(igrpRoutingTable[i].igrpRoutingTableEntry));

        int numRouteEntry =
            BUFFER_GetCurrentSize(&(igrpRoutingTable[i].
                igrpRoutingTableEntry)) / sizeof(IgrpRoutingTableEntry);

        printf("tos = %u, k1 = %d k2 = %d k3 = %d k4 = %d k5 = %d\n"
               "---------------------------------------------------------"
               "---------------------------------------------------------"
               "---------------------\n"
               "%18s %13s %19s %8s %18s %5s %17s %15s %10s\n"
               "---------------------------------------------------------"
               "---------------------------------------------------------"
               "---------------------\n",
               igrpRoutingTable[i].tos, // tos
               igrpRoutingTable[i].k1,  // k1
               igrpRoutingTable[i].k2,  // k2
               igrpRoutingTable[i].k3,  // k3
               igrpRoutingTable[i].k4,  // k4
               igrpRoutingTable[i].k5,  // k5
               "Destination_Addr",
               "Delay(10*US)",
               "InvBandWidth(Kbps)",
               "HopCount",
               "NextHopAddress",
               "Index",
               "LastUpdateTime(S)",
               "BandWidth(Kbps)",
               "Igrp-Metric");

        for (j = 0; j <numRouteEntry; j++)
        {
            char destAddr[MAX_STRING_LENGTH] = {0};
            char nextHop[MAX_STRING_LENGTH] = {0};
            IgrpMetricStruct *metric = &(routeEntry[j].vectorMetric);
            unsigned int invBandwidth = IgrpConvertCharArrayToInt(metric->bandwidth);
            unsigned int bandwidth = 0;

            if (invBandwidth != 0)
            {
                bandwidth = 10000000 / invBandwidth;
            }

            IO_ConvertIpAddressToString(
                (IgrpConvertCharArrayToInt(metric->number) << 8),
                destAddr);

            IO_ConvertIpAddressToString(routeEntry[j].nextHop, nextHop);
            ctoa(routeEntry[j].lastUpdateTime / SECOND , clockStr);

            printf("%18s %13u %19u %8u %18s %5u %17s %15u %14f\n",
                destAddr,                                     // destAddr
                IgrpConvertCharArrayToInt(metric->delay),     // delay
                IgrpConvertCharArrayToInt(metric->bandwidth), // bandwidth
                metric->hopCount,
                nextHop,
                routeEntry[j].outgoingInterface,
                clockStr,
                bandwidth,
                routeEntry[j].metric);

        } // end for (j = 0; j <numRouteEntry; j++)
        printf("---------------------------------------------------------"
               "---------------------------------------------------------"
               "---------------------\n");
    } // end for (i = 0; i < numIgrpRouteTableEntries; i++)
} // end IgrpPrintIgrpRoutingTable()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpPrintIgrpStructure()
//
// PURPOSE      : Printing Routing IGRP structure for debugging purpose
//
// PARAMETERS   : node - node who's routing structure has to be printed
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpPrintIgrpStructure(Node* node)
{
    RoutingIgrp* igrp = (RoutingIgrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IGRP);

    char clockString[MAX_STRING_LENGTH] = {0};

    memset(clockString, 0, MAX_STRING_LENGTH);

    ctoa(igrp->broadcastTime, &clockString[0]);  // broadcast time
    ctoa(igrp->invalidTime, &clockString[15]);   // invalid time
    ctoa(igrp->holdTime, &clockString[30]);      // hold time
    ctoa(igrp->flushTime, &clockString[45]);     // flash time
    ctoa(igrp->periodicTimer, &clockString[60]); // periodic time

    printf("---------------------------------------------------------"
           "-------------------\n"
           "node : %u \n"
           "---------------------------------------------------------"
           "-------------------\n"
           "Igrp Variance  : %f\n"
           "Igrp As System : %u\n"
           "Igrp maxHops   : %d\n"
           "Broadcast time : %s ns\n"
           "Invalid Time   : %s ns\n"
           "Hold Time      : %s ns\n"
           "Flush Time     : %s ns\n"
           "Periodic Timer : %s ns\n"
           "---------------------------------------------------------"
           "-------------------\n",
           node->nodeId,
           igrp->variance,
           igrp->asystem,
           igrp->maxHop,
           &clockString[0],   // broadcast time
           &clockString[15],  // invalid time
           &clockString[30],  // hold time
           &clockString[45],  // flash time
           &clockString[60]); // periodic time
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpPrintRouteEntriesInUpdateMessage()
//
// PURPOSE      : Printing the content of update message for debugging
//
// PARAMETERS   : node - the node which is geting/sending the update
//                       message
//                routeEntry - pointer to the routing table entries
//                size - size of the update message (without header)
//                interfaceIndex - interface index through which node is
//                                 sending / receiving update message
//
// RETURN VALUE : void
//
// ASSUMPTION   : header of the update message has been removed before
//                printing
//
//-------------------------------------------------------------------------
static
void IgrpPrintRouteEntriesInUpdateMessage(
    Node* node,
    IgrpRoutingInfo routeEntry[],
    int size,
    int interfaceIndex,
    BOOL isReceivedUpdateMsg)
{
    int j = 0;
    char now[MAX_STRING_LENGTH] = {0};
    int numEntries = size / sizeof(IgrpRoutingInfo);
    TIME_PrintClockInSecond(node->getNodeTime(), now);
    printf("---------------------------------------------------------"
           "-------------------\n"
           "node : %u interfaceIndex %u %s at %s sec.\n"
           "---------------------------------------------------------"
           "-------------------\n"
           "%18s %13s %19s %4s %5s %8s %9s\n"
           "---------------------------------------------------------"
           "-------------------\n",
           node->nodeId,
           interfaceIndex,
           isReceivedUpdateMsg ? "Recvd update":"Sending update",
           now,
           "Destination_Addr",
           "Delay(10*US)",
           "BandWidth(Kbps)",
           "MTU",
           "Relib",
           "load",
           "hopCount");

    for (j = 0; j < numEntries; j++)
    {
        char destinationAddress[MAX_STRING_LENGTH] = {0};

        IO_ConvertIpAddressToString(
            IgrpConvertCharArrayToInt(routeEntry[j].number),
            destinationAddress);

        printf("%16s %13u %19u %4u %5u %8u %8u\n",
            destinationAddress,
            IgrpConvertCharArrayToInt(routeEntry[j].delay),
            IgrpConvertCharArrayToInt(routeEntry[j].bandwidth),
            routeEntry[j].mtu,
            routeEntry[j].reliability,
            routeEntry[j].load,
            routeEntry[j].hopCount);
    }
    printf("-------------------------------------------------------"
           "--------------------\n\n");
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpPrintMessageHeader()
//
// PURPOSE      : print IGRP header for debugging purpose
//
// PARAMETERS   : node - node which is printing the header
//                igrpHeader - pointer to the igrp header
//                interfaceindex - interface index through which node is
//                                 sending/receiving message
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpPrintMessageHeader(IgrpHeader* igrpHeader)
{
    printf("---------------------------------------------------------"
           "-------------------\n"
           "version   : %u\n"
           "opcode    : %u\n"
           "edition   : %u\n"
           "asystem   : %u\n"
           "ninterior : %u\n"
           "nsystem   : %u\n"
           "nexterior : %u\n"
           "checksum  : %u\n"
           "---------------------------------------------------------"
           "-------------------\n",
           IgrpHeaderGetVersion(igrpHeader->IgrpHdr),
           IgrpHeaderGetOpCode(igrpHeader->IgrpHdr),
           igrpHeader->edition,
           igrpHeader->asystem,
           igrpHeader->ninterior,
           igrpHeader->nsystem,
           igrpHeader->nexterior,
           igrpHeader->checksum);
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpPrintPacketSendingStatics()
//
// PURPOSE      : printing number of packets sent through each interface
//
// PARAMETER    : node - node which is printing the above statistics
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpPrintPacketSendingStatics(Node* node)
{
    int i = 0, j = 0;
    IgrpRoutingTable* igrpRoutingTable = NULL;
    int numIgrpRouteTableEntries = 0;

    RoutingIgrp* igrp = (RoutingIgrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IGRP);

    printf("----------------------------------------------------------"
           "-----------------\n"
           "node : %u\n"
           "----------------------------------------------------------"
           "-----------------\n",
           node->nodeId);

    igrpRoutingTable = (IgrpRoutingTable*)
        BUFFER_GetData(&(igrp->igrpRoutingTable));

    numIgrpRouteTableEntries = BUFFER_GetCurrentSize(
        &(igrp->igrpRoutingTable)) / sizeof (IgrpRoutingTable);

    for (i = 0; i < numIgrpRouteTableEntries; i++)
    {
        IgrpRoutingTableEntry* routeEntry = (IgrpRoutingTableEntry*)
            BUFFER_GetData(&(igrpRoutingTable[i].igrpRoutingTableEntry));

        int numRouteEntry = BUFFER_GetCurrentSize(&(igrpRoutingTable[i].
            igrpRoutingTableEntry)) / sizeof(IgrpRoutingTableEntry);

        printf("tos = %u, k1 = %d k2 = %d k3 = %d k4 = %d k5 = %d\n"
               "---------------------------------------------------------"
               "-----------------\n"
               "%17s %8s %19s %8s %12s\n"
               "---------------------------------------------------------"
               "-----------------\n",
               igrpRoutingTable[i].tos,
               igrpRoutingTable[i].k1,
               igrpRoutingTable[i].k2,
               igrpRoutingTable[i].k3,
               igrpRoutingTable[i].k4,
               igrpRoutingTable[i].k5,
               "Dest_Address",
               "hopCount",
               "NextHopAddress",
               "interface",
               "numPacketSend");

        for (j = 0; j <numRouteEntry; j++)
        {
             char destAddress[MAX_STRING_LENGTH] = {0};
             char nextAddress[MAX_STRING_LENGTH] = {0};

             IO_ConvertIpAddressToString(IgrpConvertCharArrayToInt(
                        routeEntry[j].vectorMetric.number),
                 destAddress);

             IO_ConvertIpAddressToString(routeEntry[j].nextHop,
                 nextAddress);

             printf("%17s %8u %17s %8u %12u\n",
                    destAddress,
                    routeEntry[j].vectorMetric.hopCount,
                    nextAddress,
                    routeEntry[j].outgoingInterface,
                    routeEntry[j].numOfPacketSend);

        } // end for (j = 0; j <numRouteEntry; j++)
        printf("--------------------------------------------------------"
               "--------------------\n\n");
    } // end for (i = 0; i < numIgrpRouteTableEntries; i++)
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpAllocateRoutingSpace()
//
// PARAMETERS   : igrp - routing IGRP structure
//
// PURPOSE      : allocating space for IGRP routing structure
//                and initialize structure element with default values
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpAllocateRoutingSpace(Node* node, RoutingIgrp** igrp)
{
    // Initialize igrp with default values.
    // Then modify them as needed, if user specifies
    // something different in the configuration file.
    (*igrp) = (RoutingIgrp*) MEM_malloc(sizeof(RoutingIgrp));
    (*igrp)->edition = 0;
    (*igrp)->variance = 1.0;
    (*igrp)->maxHop = IGRP_DEFAULT_MAXIMUM_HOP_COUNT;
    (*igrp)->broadcastTime = IGRP_DEFAULT_BROADCAST_TIMER;
    (*igrp)->invalidTime = IGRP_DEFAULT_INVALID_TIMER;
    (*igrp)->holdTime = IGRP_DEFAULT_HOLD_TIMER;
    (*igrp)->flushTime = IGRP_DEFAULT_FLUSH_TIMER;
    (*igrp)->periodicTimer = IGRP_DEFAULT_PERIODIC_TIMER;
    (*igrp)->sleepTime = IGRP_TRIGGER_DELAY;
    (*igrp)->holdDownEnabled = TRUE;
    (*igrp)->splitHorizonEnabled = TRUE;
    (*igrp)->collectStat = FALSE;
    (*igrp)->lastRegularUpdateSend = 0;
    (*igrp)->recentlyTriggeredUpdateScheduled = FALSE;

    // initialize statiscal data
    (*igrp)->igrpStat.isAlreadyPrinted = FALSE;
    (*igrp)->igrpStat.numRegularUpdates = 0;
    (*igrp)->igrpStat.numTriggeredUpdates = 0;
    (*igrp)->igrpStat.numRouteTimeouts = 0;
    (*igrp)->igrpStat.numResponsesReceived = 0;

    (*igrp)->igrpStat.packetSentStat = (unsigned int *)
        MEM_malloc(sizeof(unsigned int) * node->numberInterfaces);
    memset((*igrp)->igrpStat.packetSentStat, 0,
        (sizeof(unsigned int) * node->numberInterfaces));

    // Allocate space for routing table entries.
    BUFFER_InitializeDataBuffer(&((*igrp)->igrpRoutingTable),
        sizeof(IgrpRoutingTable) * IGRP_MAX_NUM_OF_TOS);
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpReadFromConfigurationFile()
//
// PURPOSE      : 1.Reading configuration parameters from
//                  igrp configuration file
//                2.Determine if the node is an IGRP router or not
//
// PARAMETERS   : node - The node being initialized.
//                igrp - The igrp structure
//                igrpConfigFile - The igrp configuration file to be read.
//                found - is there any entry found in igrp file for the
//                        given node? found == TRUE when an entry
//                        is found for the given node, or FALES otherwise.
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpReadFromConfigurationFile(
    Node* node,
    RoutingIgrp** igrp,
    NodeInput* igrpConfigFile,
    BOOL* found)
{
    int i = 0;
    DataBuffer temp;

    while (i < igrpConfigFile->numLines)
    {
        char token[MAX_STRING_LENGTH] = {0};
        unsigned int nodeId = 0;
        unsigned int asId = IGRP_DEFAULT_AS_ID;
        char* currentLine = igrpConfigFile->inputStrings[i];
        BOOL tosSpecified = FALSE;
        BOOL networkSpecified = FALSE;
        sscanf(currentLine, "%s", token);

        // Skip if not a router statement
        if (strcmp(token, "ROUTER"))
        {
            i++;
            continue;
        }

        sscanf(currentLine, "%*s%u%s%u", &nodeId, token, &asId);

        if (strcmp(token, "IGRP") != 0)
        {
            char errStr[MAX_STRING_LENGTH] = {0};
            sprintf(errStr, "Node %u Not a IGRP router", node->nodeId);
            ERROR_ReportError(errStr);
        }

        // Skip if router statement doesn't belong to us.
        if (node->nodeId != nodeId)
        {
            i++;
            continue;
        }

        // Each Igrp router should reach this point of code
        // once and once only at the time of initialization
        IgrpAllocateRoutingSpace(node, igrp);

        *found = TRUE;

        i++;

        if (i == igrpConfigFile->numLines)
        {
            continue;
        }

        currentLine = igrpConfigFile->inputStrings[i];

        sscanf(currentLine, "%s", token);

        BUFFER_InitializeDataBuffer(&temp,
            (sizeof(IgrpRoutingTableEntry)
             * IGRP_ASSUMED_MAX_NUM_OF_ROUTE_ENTRIES));

        (*igrp)->asystem = (unsigned short) asId;

        while ((strcmp(token, "ROUTER")) &&
              (i < igrpConfigFile->numLines))
        {
            if (strcmp(token, "NETWORK") == 0)
            {
                char network[MAX_STRING_LENGTH] = {0};
                NodeAddress outputNodeAddress = 0;
                int numHostBits = 0;
                int interfaceIndex = -1;
                NodeAddress interfaceAddress = 0;
                unsigned int propDelay = 0;
                unsigned int bandwidth = 0;
                IgrpRoutingTableEntry igrpRoute;

                if (tosSpecified == TRUE)
                {
                    ERROR_Assert(FALSE, "All NETWORK statement should be "
                        "given before any METRIC-WEIGHTS statement");
                }

                networkSpecified = TRUE;

                sscanf(currentLine, "%*s%s", network);

                IO_ParseNetworkAddress(
                    network,
                    &outputNodeAddress,
                    &numHostBits);

                interfaceAddress = MAPPING_GetInterfaceAddressForSubnet(
                                       node,
                                       node->nodeId,
                                       outputNodeAddress,
                                       numHostBits);

                interfaceIndex = NetworkIpGetInterfaceIndexFromAddress(
                                     node,
                                     interfaceAddress);

                if (interfaceIndex < 0)
                {
                    char errorStr[MAX_STRING_LENGTH] = {0};

                    sprintf(errorStr,
                         "Network configuration for Router %u not proper\n"
                         "Corresponding network statement is %s\n",
                         node->nodeId,
                         network);
                    ERROR_ReportError(errorStr);
                }

                bandwidth = IgrpGetInvBandwidth(node, interfaceIndex);

                propDelay = (unsigned int) (IgrpGetPropDelay(node,
                                interfaceIndex) / MICRO_SECOND);

                // Build up an routing table entry.
                igrpRoute.vectorMetric.number[2] =
                    ((unsigned char) (0xFF & (outputNodeAddress >> 24)));
                igrpRoute.vectorMetric.number[1] =
                    ((unsigned char) (0xFF & (outputNodeAddress >> 16)));
                igrpRoute.vectorMetric.number[0] =
                    ((unsigned char) (0xFF & (outputNodeAddress >> 8)));

                igrpRoute.vectorMetric.delay[2] =
                    ((unsigned char) (0xFF & (propDelay >> 16)));
                igrpRoute.vectorMetric.delay[1] =
                    ((unsigned char) (0xFF & (propDelay >> 8)));
                igrpRoute.vectorMetric.delay[0] =
                    ((unsigned char) (0xFF & (propDelay)));

                igrpRoute.vectorMetric.bandwidth [2] =
                    ((unsigned char) (0xFF & (bandwidth >> 16)));
                igrpRoute.vectorMetric.bandwidth [1] =
                    ((unsigned char) (0xFF & (bandwidth >> 8)));
                igrpRoute.vectorMetric.bandwidth[0] =
                    ((unsigned char) (0xFF & (bandwidth)));

                igrpRoute.vectorMetric.mtu = 0;         // not used.
                igrpRoute.vectorMetric.reliability = 1; // not used.
                igrpRoute.vectorMetric.load = 0;        // not used.
                igrpRoute.vectorMetric.hopCount = 0;
                igrpRoute.metric = 0.0;

                igrpRoute.remoteCompositeMetric =
                    IGRP_INVALID_REMOTE_METRIC;

                igrpRoute.asystem = (unsigned short) asId;
                igrpRoute.routeType = IGRP_SYSTEM_ROUTE;
                igrpRoute.nextHop = 0;

                igrpRoute.outgoingInterface = interfaceIndex;
                igrpRoute.lastUpdateTime = 0;
                igrpRoute.isInHoldDownState = FALSE;
                igrpRoute.numOfPacketSend = 0;
                igrpRoute.isPermanent =  FALSE;

                BUFFER_AddDataToDataBuffer(&temp,
                    (char*)&igrpRoute,
                    sizeof(IgrpRoutingTableEntry));

                if (IGRP_DEBUG)
                {

                    printf("node: %4u "
                           "neighbor Network: %s "
                           "interfaceIndex : %u "
                           "bandWidth: %u "
                           "propagation-delay: %u\n",
                           node->nodeId,
                           network,
                           interfaceIndex,
                           bandwidth,
                           propDelay);
                }
            }
            else if (strcmp(token, "VARIANCE") == 0)
            {
                float variance = 1.0;
                sscanf(currentLine, "%*s%f", &variance);

                if (variance < 1.0F)
                {
                    char errorStr[MAX_STRING_LENGTH] = {0};
                    sprintf(errorStr, "Invalid variance value %f\n",
                            variance);
                    ERROR_ReportWarning(errorStr);
                    (*igrp)->variance = 1.0;
                }
                (*igrp)->variance = variance;
            }
            else if (strcmp(token, "METRIC-WEIGHTS") == 0)
            {
                char tos[MAX_STRING_LENGTH] = {0};
                IgrpRoutingTable igrpRoutingTable;
                int k1 = 1, k2 = 0, k3 = 1, k4 = 0, k5 = 0;

                igrpRoutingTable.tos = IGRP_DEFAULT_TOS;
                tosSpecified = TRUE;

                sscanf(currentLine, "%*s%s", tos);

                if (strcmp(tos, "DEFAULT-TOS") == 0)
                {
                    igrpRoutingTable.tos = IGRP_DEFAULT_TOS;
                }
                else
                {
                    ERROR_Assert(FALSE, "Not a tos type\n");
                }

                sscanf(currentLine, "%*s%*s%d%d%d%d%d",
                    &k1, &k2, &k3, &k4, &k5);

                igrpRoutingTable.k1 = k1;
                igrpRoutingTable.k2 = k2;
                igrpRoutingTable.k3 = k3;
                igrpRoutingTable.k4 = k4;
                igrpRoutingTable.k5 = k5;

                BUFFER_InitializeDataBuffer(
                    &(igrpRoutingTable.igrpRoutingTableEntry),
                    (sizeof(IgrpRoutingTableEntry)
                    * IGRP_ASSUMED_MAX_NUM_OF_ROUTE_ENTRIES));

                BUFFER_AddDataToDataBuffer(
                    &(igrpRoutingTable.igrpRoutingTableEntry),
                    BUFFER_GetData(&temp),
                    BUFFER_GetCurrentSize(&temp));

                BUFFER_AddDataToDataBuffer(
                    &((*igrp)->igrpRoutingTable),
                    (char*) (&igrpRoutingTable),
                    sizeof(IgrpRoutingTable));

                if (IGRP_DEBUG)
                {
                    printf("node: %4u tos: %d k1 = %d, k2 = %d, "
                           "k3 = %d, k4 = %d, k5 = %d\n",
                           node->nodeId,
                           igrpRoutingTable.tos,
                           igrpRoutingTable.k1,
                           igrpRoutingTable.k2,
                           igrpRoutingTable.k3,
                           igrpRoutingTable.k4,
                           igrpRoutingTable.k5);
                }
            }
            else if (strcmp(token, "METRIC-HOLDDOWN") == 0)
            {
                char isMetricHolddown[MAX_STRING_LENGTH] = {0};
                sscanf(currentLine, "%*s%s", isMetricHolddown);

                if (strcmp(isMetricHolddown, "YES") == 0)
                {
                    (*igrp)->holdDownEnabled = TRUE;
                }
                else if (strcmp(isMetricHolddown, "NO") == 0)
                {
                    (*igrp)->holdDownEnabled = FALSE;
                }
                else
                {
                    char errorStr[MAX_STRING_LENGTH] = {0};
                    sprintf(errorStr, "Error in configuration string"
                           "\"METRIC-HOLDDOWN\": %s", isMetricHolddown);
                    ERROR_ReportError(errorStr);
                }
            }
            else if (strcmp(token, "IP-SPLIT-HORIZON") == 0)
            {
                char isSplitHorizon[MAX_STRING_LENGTH] = {0};
                sscanf(currentLine, "%*s%s", isSplitHorizon);

                if (strcmp(isSplitHorizon, "YES") == 0)
                {
                    (*igrp)->splitHorizonEnabled = TRUE;
                }
                else if (strcmp(isSplitHorizon, "NO") == 0)
                {
                    (*igrp)->splitHorizonEnabled = FALSE;
                }
                else
                {
                    char errorStr[MAX_STRING_LENGTH] = {0};
                    sprintf(errorStr, "Error in configuration string"
                           "\"IP-SPLIT-HORIZON\": %s", isSplitHorizon);
                    ERROR_ReportError(errorStr);
                }
            }
            else
            {
                char errorMsg[MAX_STRING_LENGTH] = {0};
                sprintf(errorMsg, "Unknown string in IGRP configuration"
                       " File: %s\n", token);
                ERROR_ReportError(errorMsg);
            }

            i++;

            if (i < igrpConfigFile->numLines)
            {
                currentLine = igrpConfigFile->inputStrings[i];
                sscanf(currentLine, "%s", token);
            }
        } // end of reading for one node.

        if (networkSpecified == FALSE)
        {
            char errorMsg[MAX_STRING_LENGTH];

            sprintf(errorMsg, "No network statement for Router %d",
                node->nodeId);

            ERROR_ReportError(errorMsg);
        }

        if (tosSpecified == FALSE)
        {
            // If tos is not specified in the configuration
            // file then assume a default tos.
            IgrpRoutingTable igrpRoutingTable;

            igrpRoutingTable.tos = 1;
            igrpRoutingTable.k1 = 1;
            igrpRoutingTable.k2 = 0;
            igrpRoutingTable.k3 = 1;
            igrpRoutingTable.k4 = 0;
            igrpRoutingTable.k5 = 0;

            BUFFER_InitializeDataBuffer(
                &(igrpRoutingTable.igrpRoutingTableEntry),
                (sizeof(IgrpRoutingTableEntry)
                * IGRP_ASSUMED_MAX_NUM_OF_ROUTE_ENTRIES));

            BUFFER_AddDataToDataBuffer(
                &(igrpRoutingTable.igrpRoutingTableEntry),
                BUFFER_GetData(&temp),
                BUFFER_GetCurrentSize(&temp));

            BUFFER_AddDataToDataBuffer(
                &((*igrp)->igrpRoutingTable),
                (char*) (&igrpRoutingTable),
                sizeof(IgrpRoutingTable));
        }
        BUFFER_DestroyDataBuffer(&temp);
    } // end while (i < igrpConfigFile->numLines)
} // end IgrpReadFromConfigurationFile()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpCalculateMetric()
//
// PURPOSE      : Calculating the IGRP metric value.
//
// PARAMETERS   : routingInfo - IGRP vector metric structure
//                k1, k2, k3, k4, k5 - weight values of type of service
//
// RETURN VALUE : caluclated metric value (see the calculation in the
//                function defination)
//
//-------------------------------------------------------------------------
static
float IgrpCalculateMetric(IgrpRoutingInfo* routingInfo,
    int k1, int k2, int k3, int k4, int k5)
{
    // calculate metric as per CISCO specification
    unsigned char reliability = 0;

    float inverseBandwidth = (float)
        IgrpConvertCharArrayToInt(routingInfo->bandwidth);

    float metric = 0.0;

    if (k5 == 0)
    {
        reliability = 1;
    }
    else
    {
        reliability = (unsigned char)
            (k5 / (routingInfo->reliability + k4));
    }

    metric = ((k1 * inverseBandwidth
            + k2 * inverseBandwidth / (256 - routingInfo->load)
            + k3 * IgrpConvertCharArrayToInt(routingInfo->delay))
            * reliability);

    return metric;
}


//-------------------------------------------------------------------------
//
// FUNCTION  : IgrpFindMatchingDestinationInUpdateMessage()
//
// PURPOSE   : finding if an entry already exist in update message.
//
// PARAMETER : updateMessage : the update message.
//             numEntries    : number of entries currently in the
//                             update messagge
//             routeEntry    : pointer to the route entry
//             destAddress   : Array of destination address.
//             matchingEntry : pointer to the matching entry, if found
//                             NULL otherwise
// ASSUPTION : none.
//
//-------------------------------------------------------------------------
static
BOOL IgrpFindMatchingDestinationInUpdateMessage(
    IgrpRoutingInfo updateMessage[],
    int numEntries,
    IgrpRoutingTableEntry* routeEntry,
    unsigned char destAddress[],
    IgrpRoutingInfo** matchingEntry)
{
    int i = 0;
    BOOL found = FALSE;
    *matchingEntry = NULL;

    if (routeEntry->isPermanent)
    {
        return FALSE;
    }

    for (i = 0; i < numEntries; i++)
    {
        if (memcmp(updateMessage[i].number, destAddress, 3) == 0)
        {
            found = TRUE;
            *matchingEntry = &updateMessage[i];
            break;
        }
    }
    return found;
}


//-------------------------------------------------------------------------
//
// FUNCTION  : IgrpFindMatchingDestinationInRoutingTable()
//
// PURPOSE   : finding if an entry already exist in routing table
//             and returning a pointer to routing entry which has the
//             minimum metric value.
//
// PARAMETER : routingTable  : the routing table.
//             numEntries    : number of entries currently in the
//                             routing table.
//             destAddress   : destination address to search for.
//             matchingEntry : pointer to the matching entry if found,
//                             NULL otherwise
// ASSUPTION : Returns the path with minimal metric. when multiple entry
//             exists, NULL otherwise if no matching entry found.
//-------------------------------------------------------------------------
static
BOOL IgrpFindMatchingDestinationInRoutingTable(
    IgrpRoutingTableEntry routingTable[],
    int numEntries,
    unsigned char destAddress[],
    IgrpRoutingTableEntry** matchingEntry)
{
    int i = 0;
    float minMetric = 0;
    BOOL found = FALSE;
    *matchingEntry = NULL;

    for (i = 0; i < numEntries; i++)
    {
        if (!memcmp(routingTable[i].vectorMetric.number, destAddress, 3))
        {
            if (IsDestUnreachable(routingTable[i].vectorMetric.delay))
            {
                // Do not consider unreachable destinations
                continue;
            }

            if (!found)
            {
                minMetric = routingTable[i].metric;
                *matchingEntry = &routingTable[i];
                found = TRUE;
            }
            else if ((found) && (routingTable[i].metric <= minMetric))
            {
                minMetric = routingTable[i].metric;
                *matchingEntry = &routingTable[i];
            }
        }
    }
    return found;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpCalculateChecksum()
//
// PURPOSE      : Calculating the value of checksum
//
// PARAMETERS   : msg - the data-packet for which checksun to be
//                      calculated.
//                size - size of the message
//
// RETURN VALUE : The claculated value of the checksum
//
// ASSUMPTION   : currently all checksum values are zero
//
//-------------------------------------------------------------------------
static
unsigned short IgrpCalculateChecksum(
    char* msg,
    int size)
{
    return 0;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpSendPacketToMacLayer()
//
// PURPOSE      : 1) Adding IGRP header to the message and then
//                2) Sending a message to the neighbor
//
// PARAMETER    : node - node sending the message.
//                msg - message/packet to be send.
//                igrp - igrp routing structure
//                numInteriorRoute - number of interior route
//                numSystemRoute - number of system route
//                numExteriorRoute - number of exterior route
//                interfaceIndex - interface through the message
//                                 has to be send.
// RETURN VALUE : void
//
// ASSUPTION    : none.
//
//-------------------------------------------------------------------------
static
void IgrpSendPacketToMacLayer(
    Node* node,
    Message* msg,
    RoutingIgrp* igrp,
    short numInteriorRoute,
    short numSystemRoute,
    short numExteriorRoute,
    int interfaceIndex)
{
    IgrpHeader igrpHeader;
    MESSAGE_AddHeader(node, msg, sizeof(IgrpHeader), TRACE_IGRP);

    IgrpHeaderSetVersion(&(igrpHeader.IgrpHdr), 1);
    IgrpHeaderSetOpCode(&(igrpHeader.IgrpHdr), IGRP_UPDATE);

    // CISCO doesn't use this, so we don't either
    igrpHeader.edition = (igrp->edition)++;

    igrpHeader.asystem = igrp->asystem;
    igrpHeader.ninterior = numInteriorRoute;
    igrpHeader.nsystem = numSystemRoute;
    igrpHeader.nexterior = numExteriorRoute;

    // We don't handle checksum for now.
    igrpHeader.checksum = 0;

    memcpy(msg->packet, &igrpHeader, sizeof(IgrpHeader));

    ((IgrpHeader*) MESSAGE_ReturnPacket(msg))->checksum =
        IgrpCalculateChecksum(MESSAGE_ReturnPacket(msg),
            MESSAGE_ReturnPacketSize(msg));

    // Dynamic Address Change
    // A node should not send the message to the neighbor if 
    // this node itself has not acquired a valid address
    if (NetworkIpGetInterfaceAddress(node, interfaceIndex))
    {
        NetworkIpSendRawMessageWithDelay(
            node,
            msg,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            NetworkIpGetInterfaceBroadcastAddress(node, interfaceIndex),
            interfaceIndex,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_IGRP,
            1,  // TTL value = 1
            0); // delay = 0
    }
} // end IgrpSendPacketToMacLayer()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpCalculateNewPathMetricViaSource()
//
// PURPOSE      : calculating the vector metric of a new incoming path
//
// PAPAMETER    : node - node which is calculating the path metric
//                vectorMetric - vector metric of incoming path
//                interfaceIndex - interface index through which the new
//                                 path information has been received
//
// RETURN VALUE : pointer to the newly calculated vector metric
//
//-------------------------------------------------------------------------
static
IgrpRoutingInfo* IgrpCalculateNewPathMetricViaSource(
    Node* node,
    IgrpRoutingInfo* vectorMetric,
    int interfaceIndex)
{
    IgrpRoutingInfo* newVector = (IgrpRoutingInfo*) MEM_malloc(
                                      sizeof(IgrpRoutingInfo));
    unsigned int bandwidth = 0;
    clocktype propDelay = 0;

    memcpy(newVector->number, vectorMetric->number, 3);

    // get minimum bandwidth.
    bandwidth =
    MAX(((unsigned) IgrpGetInvBandwidth(node,interfaceIndex)),
        IgrpConvertCharArrayToInt(vectorMetric->bandwidth));

    newVector->bandwidth[2] = ((char) (0xFF & (bandwidth >> 16)));
    newVector->bandwidth[1] = ((char) (0xFF & (bandwidth >> 8)));
    newVector->bandwidth[0] = ((char) (0xFF & (bandwidth)));

    if (!IsDestUnreachable(vectorMetric->delay))
    {
        propDelay = ((IgrpGetPropDelay(node, interfaceIndex)
                          / MICRO_SECOND)
                    + IgrpConvertCharArrayToInt(
                          vectorMetric->delay));

        newVector->delay[2] = ((char) (0xFF & (propDelay >> 16)));
        newVector->delay[1] = ((char) (0xFF & (propDelay >> 8)));
        newVector->delay[0] = ((char) (0xFF & (propDelay)));
    }
    else
    {
        newVector->delay[2] = 0xFF;
        newVector->delay[1] = 0xFF;
        newVector->delay[0] = 0xFF;
    }

    newVector->reliability = (unsigned char)
        MIN(1, vectorMetric->reliability);

    // mtu is currently an unused field.
    newVector->mtu = (short) MIN(0, vectorMetric->mtu);

    // load is currently an unused field.
    newVector->load = (unsigned char) MAX(1, vectorMetric->load);

    newVector->hopCount = (unsigned char) (vectorMetric->hopCount + 1);
    return newVector;
}


//-------------------------------------------------------------------------
//
// FUNCTION  : IgrpAddNewRouteToRoutingTable()
//
// PURPOSE   : 1) update IGRP routing table.
//             2) update Network layer forwarding table
//
// PARAMETER : node - node updating the routing table
//             igrpRoutingTable - IGRP's routing table.
//             newPathMetric - the path metric VECTOR that comes
//                             with update.
//             metric - the metric
//             remoteCompositeMetric - composit metric of "destAddr",
//                                     calculated at source of the update
//                                     message.Where "destAddr" is the
//                                     destination address for which
//                                     update has been sent.
//             asystem - the AS Id
//             routeType - type of the route  IGRP_INTERIOR_ROUTE, or
//                         IGRP_SYSTEM_ROUTE, or IGRP_EXTERIOR_ROUTE ?
//             nextHop - next Hop
//             outgoingInterface - outgoing Interface
//             isBest - if isBest is set to TRUE network forwarding table
//                      is updated.
//             currentTime - time when this roue is entered.
//
// ASSUPTION : none.
//
//-------------------------------------------------------------------------
static
void IgrpAddNewRouteToRoutingTable(
    Node* node,
    DataBuffer* igrpRoutingTable,
    IgrpMetricStruct* newPathMetric,
    float metric,
    float remoteCompositeMetric,
    unsigned short asystem,
    int routeType,
    NodeAddress nextHop,
    int outgoingInterface,
    BOOL isBest,
    clocktype currentTime,
    BOOL hook)
{
    IgrpRoutingTableEntry routeEntry;

    memcpy(&routeEntry.vectorMetric, newPathMetric, sizeof(IgrpRoutingInfo));

    routeEntry.metric = metric;
    routeEntry.remoteCompositeMetric = remoteCompositeMetric;
    routeEntry.asystem = asystem;
    routeEntry.routeType = (unsigned char) routeType;
    routeEntry.nextHop = nextHop;
    routeEntry.outgoingInterface = outgoingInterface;
    routeEntry.isInHoldDownState = FALSE;
    routeEntry.numOfPacketSend = 0;
    routeEntry.lastUpdateTime = currentTime;

    routeEntry.isPermanent =  hook;

    if (routeType == IGRP_EXTERIOR_ROUTE)
    {
        BUFFER_AddDataToDataBuffer(
            igrpRoutingTable,
            (char*) &routeEntry,
            sizeof(IgrpRoutingTableEntry));
    }
    else if ((routeType == IGRP_INTERIOR_ROUTE) ||
             (routeType == IGRP_SYSTEM_ROUTE))
    {
            BUFFER_AddDataToDataBuffer(
                igrpRoutingTable,
                (char*) &routeEntry,
                sizeof(IgrpRoutingTableEntry));
    }

    if ((isBest == TRUE)&& !hook)
    {
      if (IGRP_DEBUG)
    {
      printf("Content of Forwarding Table Before Update\n:");
      NetworkPrintForwardingTable(node);
    }
        NetworkUpdateForwardingTable(
            node,
            (IgrpConvertCharArrayToInt(newPathMetric->number) << 8),
            IgrpReturnSubnetMaskFromIPAddress(
                (IgrpConvertCharArrayToInt(newPathMetric->number) << 8)),
            nextHop,
            outgoingInterface,
            newPathMetric->hopCount,
            ROUTING_PROTOCOL_IGRP);
    if (IGRP_DEBUG)
    {
      printf("Content of Forwarding Table After Update\n:");
      NetworkPrintForwardingTable(node);
    }
    }
} // end IgrpAddNewRouteToRoutingTable()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpFindRouteToUpdateOrDelete()
//
// PURPOSE      : deleting  a route from IGRP routing table
//
// PARAMETERS   : buffer - igrp routing table
//                vectorMetric - vector metric of route to be deleted.
//                outInterface - the outgoing interface index which
//                the route (to be deleted) corresponds to
//                matchDelayReliability - do you want to match delay and
//                                        reliability parameters? set it
//                                        TRUE if you want to. set it
//                                        FALSE otherwise.
//                isLastEntry - is the route gonig to be deleted last
//                              enrtry to the corresponding destination?
//                              isLastEntry == TRUE if answer to the
//                              question is YES. isLastEntry == FALSE
//                              otherwise
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpFindRouteToUpdateOrDelete(
    DataBuffer* buffer,
    IgrpRoutingInfo* vectorMetric,
    int outInterface,
    BOOL matchDelayReliability,
    BOOL* isLastEntry,
    BOOL* isHoldDown,
    IgrpRoutingTableEntry** routeToDelete,
    NodeAddress matchingNextHop)
{
    int i = 0, k = 0;

    IgrpRoutingTableEntry* igrpRouteTable = (IgrpRoutingTableEntry*)
                               BUFFER_GetData(buffer);

    int numEntry = BUFFER_GetCurrentSize(buffer)
                   / sizeof(IgrpRoutingTableEntry);

    *routeToDelete = NULL;

    for (i = 0; i < numEntry; i++)
    {
        BOOL isMatchingDelay = FALSE;
        BOOL isMatchingBandwidth = FALSE;

        // check if destination address is matching
        if (IgrpConvertCharArrayToInt(
                igrpRouteTable[i].vectorMetric.number) ==
            IgrpConvertCharArrayToInt(vectorMetric->number))
        {
            k++;

            if (matchDelayReliability == TRUE)
            {
                // set isMatchingDelay TRUE if delay matches
                isMatchingDelay =
                    (IgrpConvertCharArrayToInt(igrpRouteTable[i].
                         vectorMetric.delay) ==
                     IgrpConvertCharArrayToInt(vectorMetric->delay));

                // set isMatchingBandwidth TRUE if bandwidth matches
                isMatchingBandwidth =
                    (IgrpConvertCharArrayToInt(igrpRouteTable[i].
                         vectorMetric.bandwidth) ==
                    IgrpConvertCharArrayToInt(vectorMetric->bandwidth));
            }
            else
            {
                isMatchingDelay = TRUE;
                isMatchingBandwidth = TRUE;
            }

            if ((igrpRouteTable[i].outgoingInterface == outInterface) &&
                (isMatchingDelay) && (isMatchingBandwidth))
            {

                if (matchingNextHop == ANY_IP
                    || matchingNextHop == igrpRouteTable[i].nextHop
                    || matchingNextHop == (NodeAddress) NETWORK_UNREACHABLE)
                {
                   *routeToDelete = &igrpRouteTable[i];
                }
            }
            *isHoldDown = igrpRouteTable[i].isInHoldDownState;
        }
    }

    if (k == 1)
    {
        *isLastEntry = TRUE;
    }
    else
    {
        *isLastEntry = FALSE;
        ERROR_Assert(*isHoldDown == FALSE, "If there is more than"
            " one entry for a destination then in the routing table"
            " \"isHoldDown\" cannot be true");
    }
}


//-------------------------------------------------------------------------
//
//  FUNCTION     : IgrpScheduleTriggeredUpdate()
//
//  PURPOSE      : Scheduling triggered update message with little
//                 bit dealy. This is to keep updates from being issued
//                 too often, if the network is changing rapidly.
//
//  PARAMETERS   : node - node which is sending triggered update.
//
//  RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpScheduleTriggeredUpdate(Node* node)
{
    RoutingIgrp* igrp = (RoutingIgrp*) NetworkIpGetRoutingProtocol(node,
                            ROUTING_PROTOCOL_IGRP);

    // Calculate triggered update delay
    clocktype delay = igrp->sleepTime - (RANDOM_nrand(igrp->seed) %
                      ((clocktype) (igrp->sleepTime * IGRP_TRIGGER_JITTER)));

    // Calculate next regular update time
    clocktype nextRegularUpdateTime = igrp->lastRegularUpdateSend
                                      + igrp->broadcastTime;

    if ((igrp->recentlyTriggeredUpdateScheduled == FALSE) &&
        (nextRegularUpdateTime > node->getNodeTime() + delay))
    {
        // schedule a triggered update message
        Message* msg = MESSAGE_Alloc(
                           node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_IGRP,
                           MSG_ROUTING_IgrpTriggeredUpdateAlarm);

        igrp->recentlyTriggeredUpdateScheduled = TRUE;

        MESSAGE_Send(node, msg, delay);
    }
} // end IgrpScheduleTriggeredUpdate()


//------------------------------------------------------------------------
//
// FUNCTION     : IsAnyPathsToDestHaveNextHopThruThisInterface()
//
// PURPOSE      : Finding a routing table entry with matching outgoing
//                interface index.
//
// PARAMETERS   : igrpRoutingTableEntry - pointer to the routing
//                                        table entry.
//                interfaceIndex - the interface in dex to be matched.
//                startPos - position to start search,
//                destAddress[] - destination to match
//                numEntries - number of entries in the routing table.
//
// RETURN VALUE : TRUE if a routing entry with matching outgoing interface
//                index is is found; or FALSE otherwise.
//
// ASSUMPTION   : none
//
//------------------------------------------------------------------------
static
BOOL IsAnyPathsToDestHaveNextHopThruThisInterface(
    IgrpRoutingTableEntry* igrpRoutingTableEntry,
    int interfaceIndex,
    int startPos,
    unsigned char destAddress[],
    int numEntries)
{
    int i = 0, j = 0;

    for (i = 0, j = startPos;
         i < numEntries;
         i++, (j = (j + 1) % numEntries))
    {
        BOOL isMatchingDest = (IgrpConvertCharArrayToInt(destAddress) ==
            IgrpConvertCharArrayToInt(igrpRoutingTableEntry[j].
                vectorMetric.number));

        if ((igrpRoutingTableEntry[j].outgoingInterface ==
             interfaceIndex) && (isMatchingDest))
        {
           return TRUE;
        }
    }
    return FALSE;
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpSendUpdateMessage()
//
// PURPOSE      : sending new update message.
//
// PARAMETER    : node - Node sending the update message.
//
// RETURN VALUE : BOOL; TRUE is update message is sent.
//
// ASSUPTION    : none.
//
//-------------------------------------------------------------------------
static
BOOL IgrpSendUpdateMessage(Node* node)
{
    int i = 0, j = 0, k = 0;
    IgrpRoutingInfo* updateMessage = NULL;
    BOOL isUpdateSent = FALSE;

    RoutingIgrp* igrp = (RoutingIgrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IGRP);

    int numEntriesInUpdateMessage = 0;

    IgrpRoutingTable* igrpRoutingTable = (IgrpRoutingTable*)
        BUFFER_GetData(&igrp->igrpRoutingTable);

    int numOfTos = BUFFER_GetCurrentSize(&igrp->igrpRoutingTable)
                       / sizeof(IgrpRoutingTable);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        short numInteriorRoute = 0;
        short numSystemRoute = 0;
        short numExteriorRoute = 0;
        numEntriesInUpdateMessage = 0;

        if (!NetworkIpInterfaceIsEnabled(node, i)
            || (NetworkIpGetUnicastRoutingProtocolType(node, i)
                    != ROUTING_PROTOCOL_IGRP))
        {
           continue;
        }

        // create an empty message of maximum allowable size
        updateMessage = (IgrpRoutingInfo*) MEM_malloc(
            IGRP_MAX_ROUTE_ADVERTISED * sizeof(IgrpRoutingInfo));

        for (j = 0; j < numOfTos; j++)
        {
            IgrpRoutingTableEntry* igrpRoutingTableEntry =
                (IgrpRoutingTableEntry*) BUFFER_GetData(
                    &(igrpRoutingTable[j].igrpRoutingTableEntry));

            int numEntries = BUFFER_GetCurrentSize(&(igrpRoutingTable[j].
                igrpRoutingTableEntry)) / sizeof (IgrpRoutingTableEntry);

            // get tos weight parameters of IGRP
            int k1 = igrpRoutingTable[j].k1;
            int k2 = igrpRoutingTable[j].k2;
            int k3 = igrpRoutingTable[j].k3;
            int k4 = igrpRoutingTable[j].k4;
            int k5 = igrpRoutingTable[j].k5;

            for (k = 0; k < numEntries; k++)
            {
                IgrpRoutingInfo* matchingEntry = NULL;
                float metric = 0;

                if (igrp->splitHorizonEnabled == TRUE)
                {
                    // If any paths to destination have a next hop reached
                    // through this interface then continue with the
                    // next entry in the routing table.

                    // do not broadcast the route through the interface
                    // if the destination of the route is reachable via
                    // that interface.(This process is followed if split
                    // horizon is on)
                    if (IsAnyPathsToDestHaveNextHopThruThisInterface(
                           &igrpRoutingTableEntry[0],
                           i,   // interface index to match
                           0,   // start position
                           igrpRoutingTableEntry[k].vectorMetric.number,
                           numEntries))
                    {
                        continue;
                    }
                }

                // if an entry for the destination already exist in
                // the update message then...
                if (IgrpFindMatchingDestinationInUpdateMessage(
                        updateMessage,
                        numEntriesInUpdateMessage,
                        &igrpRoutingTableEntry[k],
                        igrpRoutingTableEntry[k].vectorMetric.number,
                        &matchingEntry))
                {
                    // get the igrp metric of that entry
                    metric = IgrpCalculateMetric(matchingEntry,
                                 k1, k2, k3, k4, k5);

                    if (metric <= igrpRoutingTableEntry[k].metric)
                    {
                        // If any path with minimul composite metric
                        // already exist in the update message the
                        // continue with the next entry in the
                        // routing table
                        continue;
                    }
                    else
                    {
                        // If more optimised entry is found in routing
                        // table ovewrite the previous entry in the
                        // update message.

                        // modify the entry of the update message
                        // with new metric information.
                        memcpy(matchingEntry,
                               &igrpRoutingTableEntry[k].vectorMetric,
                               sizeof(IgrpRoutingInfo));
                    }
                }
                else
                {
                    // if the destination address do not already
                    // exists in the update message, then add
                    // it in the update message.

                    if (igrpRoutingTableEntry[k].routeType ==
                           IGRP_INTERIOR_ROUTE)
                    {
                         numInteriorRoute++;
                    }
                    else if (igrpRoutingTableEntry[k].routeType ==
                             IGRP_SYSTEM_ROUTE)
                    {
                        numSystemRoute++;
                    }
                    else if (igrpRoutingTableEntry[k].routeType ==
                             IGRP_EXTERIOR_ROUTE)
                    {
                        numExteriorRoute++;
                    }

                    // add the entries to the update message
                    memcpy(&updateMessage[numEntriesInUpdateMessage],
                           &igrpRoutingTableEntry[k].vectorMetric,
                           sizeof(IgrpRoutingInfo));

                    numEntriesInUpdateMessage++;

                    // if packet size exceeds maximum packet size
                    // the send the update message and create a new
                    // blank update message of maximum allowable size
                    if (!(numEntriesInUpdateMessage %
                          IGRP_MAX_ROUTE_ADVERTISED))
                    {
                        Message* msg = NULL;

                        msg = MESSAGE_Alloc(
                                  node,
                                  MAC_LAYER,
                                  ROUTING_PROTOCOL_IGRP,
                                  0);

                        MESSAGE_PacketAlloc(node,
                            msg,
                            (IGRP_MAX_ROUTE_ADVERTISED
                                * sizeof(IgrpRoutingInfo)),
                            TRACE_IGRP);

                        memcpy(msg->packet,
                            updateMessage,
                            (IGRP_MAX_ROUTE_ADVERTISED
                                * sizeof(IgrpRoutingInfo)));

                        isUpdateSent = TRUE;
                        // Send the message
                        IgrpSendPacketToMacLayer(
                             node,
                             msg,
                             igrp,
                             numInteriorRoute,
                             numSystemRoute,
                             numExteriorRoute,
                             i); // i = interface Index

                        memset(updateMessage, 0,
                             IGRP_MAX_ROUTE_ADVERTISED
                             * sizeof(IgrpRoutingInfo));

                        numInteriorRoute = 0;
                        numSystemRoute = 0;
                        numExteriorRoute = 0;
                        numEntriesInUpdateMessage = 0;
                    }
                }
            }// end for (k = 0; k < numEntries; k++)
        }// end for (j = 0; j < numOfTos; j++)

        if ((numEntriesInUpdateMessage != 0) &&
            ((numEntriesInUpdateMessage % IGRP_MAX_ROUTE_ADVERTISED)))
        {
            Message* msg = MESSAGE_Alloc(
                               node,
                               MAC_LAYER,
                               ROUTING_PROTOCOL_IGRP,
                               0); // 0 = event type (notUsed)

            MESSAGE_PacketAlloc(node, msg,
                ((numEntriesInUpdateMessage % IGRP_MAX_ROUTE_ADVERTISED)
                    * sizeof(IgrpRoutingInfo)),
                TRACE_IGRP);

            memcpy(MESSAGE_ReturnPacket(msg),
                updateMessage,
                ((numEntriesInUpdateMessage % IGRP_MAX_ROUTE_ADVERTISED)
                    * sizeof(IgrpRoutingInfo)));

            isUpdateSent = TRUE;
            IgrpSendPacketToMacLayer(
                node,
                msg,
                igrp,
                numInteriorRoute,
                numSystemRoute,
                numExteriorRoute,
                i); // i = interface Index
        }
        MEM_free(updateMessage);
    } // end for (i = 0; i < node->numberInterfaces; i++)
    return isUpdateSent;
} // end IgrpSendUpdateMessage()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpUpdateForwardingTableWithAlternetPath()
//
// PURPOSE      : Finding an alternative best path for a deleted path to
//                update the network forwarding table.
//
// PARAMETERS   : node - node which is finding the alternative route.
//                igrp - pointer to igrp routing structure
//                destAddress[] - destination address for which alternate
//                                path is to be found.
//
// RETURN VALUE : none.
//
// ASSUMPTION   : destAddress[] is the first 3 bytes of the IP address
//                packed as a character array
//
//-------------------------------------------------------------------------
static
void IgrpUpdateForwardingTableWithAlternetPath(
    Node* node,
    RoutingIgrp* igrp,
    unsigned char destAddress[])
{
    int i = 0;
    IgrpRoutingTableEntry* matchingAndMinimalEntry = NULL;
    IgrpRoutingTable* igrpRoutingTable = NULL;
    int numOfTos = 0;

    igrpRoutingTable = (IgrpRoutingTable*) BUFFER_GetData(
                               &igrp->igrpRoutingTable);

    numOfTos = BUFFER_GetCurrentSize (&igrp->igrpRoutingTable)
                   / sizeof(IgrpRoutingTable);

    for (i = 0; i < numOfTos; i++)
    {
        IgrpRoutingTableEntry* igrpRoutingTableEntry =
            (IgrpRoutingTableEntry*)
            BUFFER_GetData(&(igrpRoutingTable[i].igrpRoutingTableEntry));

        int numEntries = BUFFER_GetCurrentSize(&(igrpRoutingTable[i].
            igrpRoutingTableEntry)) / sizeof (IgrpRoutingTableEntry);

        IgrpFindMatchingDestinationInRoutingTable(
            igrpRoutingTableEntry,
            numEntries,
            destAddress,
            &matchingAndMinimalEntry);

        if (matchingAndMinimalEntry)
        {
      if (IGRP_DEBUG)
        {
          printf("Content of Forwarding Table Before Update\n:");
          NetworkPrintForwardingTable(node);
        }
           NetworkUpdateForwardingTable(
               node,
               (IgrpConvertCharArrayToInt(
                   matchingAndMinimalEntry->vectorMetric.number) << 8),
               IgrpReturnSubnetMaskFromIPAddress(
                   (IgrpConvertCharArrayToInt(
                       matchingAndMinimalEntry->vectorMetric.number) << 8)),
               matchingAndMinimalEntry->nextHop,
               matchingAndMinimalEntry->outgoingInterface,
               matchingAndMinimalEntry->vectorMetric.hopCount,
               ROUTING_PROTOCOL_IGRP);
       if (IGRP_DEBUG)
         {
           printf("Content of Forwarding Table Before Update\n:");
           NetworkPrintForwardingTable(node);
         }
           break;
        }
    }
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpRemoveUnfeasiblePaths()
//
// PURPOSE      : Removing out of variance path from IGRP routing table
//
// PARAMETERS   : igrpRoutingTable - pointer to IGRP routing table
//                igrp - pointer to igrp routing structure
//                destAddr[] - destination address for which alternate
//                             path is to be found.
//                newMinMetric - minimum metric against which metric
//                               comparison to be done
//
// RETURN VALUE : none.
//
//-------------------------------------------------------------------------
static
void IgrpRemoveUnfeasiblePaths(
    IgrpRoutingTable* igrpRoutingTable,
    RoutingIgrp* igrp,
    unsigned char destAddr[],
    float newMinMetric)
{
    int i = 0;
    float newVarianceRange = 0, var =0;
    unsigned int destAddrToMatch = IgrpConvertCharArrayToInt(destAddr);

    IgrpRoutingTableEntry* igrpRoutingTableEntry =
        (IgrpRoutingTableEntry*)
            BUFFER_GetData(&(igrpRoutingTable->igrpRoutingTableEntry));

    int numEntries = BUFFER_GetCurrentSize(&(igrpRoutingTable->
        igrpRoutingTableEntry)) / sizeof (IgrpRoutingTableEntry);


    if (igrp->variance == 1)
    {
        var = 1.1f;
    }
    else
    {
        var = igrp->variance;

    }
    newVarianceRange = var * newMinMetric;

    while (i < numEntries)
    {
        unsigned int destAddrInRouteTable = IgrpConvertCharArrayToInt(
           igrpRoutingTableEntry[i].vectorMetric.number);
        float metric = igrpRoutingTableEntry[i].metric;
        int hopCount = igrpRoutingTableEntry[i].vectorMetric.hopCount;

        if ((destAddrToMatch == destAddrInRouteTable) &&
            (metric > newVarianceRange) &&
            (hopCount > 0))
        {
            BUFFER_ClearDataFromDataBuffer(
                &(igrpRoutingTable->igrpRoutingTableEntry),
                ((char*) &igrpRoutingTableEntry[i]),
                sizeof(IgrpRoutingTableEntry),
                FALSE);
            numEntries--;
            i--;
        }
        i++;
    }
}


//-------------------------------------------------------------------------
//
// FUNCTION  : RoutingIgrpProcessNewIncomingUpdate()
//
// PURPOSE   : processing a new incoming update message.
//
// PARAMETER : node - Node processing the update.
//             igrpUpdateMessage - the update message
//             sourceAddr - source address of the neighbor form
//                          whom the message has been received
// ASSUPTION : none.
//
//-------------------------------------------------------------------------
static
void RoutingIgrpProcessNewIncomingUpdate(
    Node* node,
    Message* msg,
    NodeAddress sourceAddr,
    int interfaceIndex,
    BOOL hook)
{
    RoutingIgrp* igrp = (RoutingIgrp*) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_IGRP);

    IgrpRoutingTable* igrpRoutingTable = NULL;
    int numOfTos = 0;
    IgrpHeader* igrpHeader = (IgrpHeader*) MESSAGE_ReturnPacket(msg);
    IgrpRoutingInfo* destAddressArray = NULL;
    short ninterior = 0;
    short nsystem = 0;
    short nexterior = 0;
    short asystem = 0;
    int numEntriesInUpdateMesage = 0;
    int i = 0, j = 0;

    if (!igrp)
    {
        // I am not a IGRP router,
        // then discard the message
        return;
    }

    if (igrpHeader->checksum != IgrpCalculateChecksum(
                                    MESSAGE_ReturnPacket(msg),
                                    MESSAGE_ReturnPacketSize(msg)))
    {
        // if checksum is wrong then
        // discard the message
        return;
    }

    ninterior = igrpHeader->ninterior;
    nsystem = igrpHeader->nsystem;
    nexterior = igrpHeader->nexterior;
    asystem = igrpHeader->asystem;

    if (IGRP_DEBUG)
    {
        char sourceAddress[MAX_STRING_LENGTH] = {0};
        IO_ConvertIpAddressToString(sourceAddr, sourceAddress);

        printf("node : %u has received a message from %s\n",
               node->nodeId, sourceAddress);

        IgrpPrintMessageHeader((IgrpHeader*)
            MESSAGE_ReturnPacket(msg));
    }

    // remove the igrp header
    MESSAGE_RemoveHeader(node, msg, sizeof(IgrpHeader), TRACE_IGRP);

    // get the number of route-entries in the update message
    numEntriesInUpdateMesage = (MESSAGE_ReturnPacketSize(msg)
                                    / sizeof(IgrpRoutingInfo));

    // extract the route entries
    destAddressArray = (IgrpRoutingInfo*) MESSAGE_ReturnPacket(msg);

    if (IGRP_DEBUG)
    {
        printf("Num Entries In Update Mesage %u\n"
               "New Update Came at %u th second **\n",
               numEntriesInUpdateMesage,
               (unsigned int) (node->getNodeTime() / SECOND));

        IgrpPrintRouteEntriesInUpdateMessage(
            node,
            destAddressArray,
            MESSAGE_ReturnPacketSize(msg),
            interfaceIndex,
            TRUE);

        IgrpPrintIgrpRoutingTable(node);
    }

    igrpRoutingTable = (IgrpRoutingTable*) BUFFER_GetData(
                           &igrp->igrpRoutingTable);

    numOfTos = BUFFER_GetCurrentSize (&igrp->igrpRoutingTable)
                   / sizeof(IgrpRoutingTable);

    for (i = 0; i < numOfTos; i++)
    {
        IgrpRoutingTableEntry* igrpRoutingTableEntry =
            (IgrpRoutingTableEntry*) BUFFER_GetData(
                &(igrpRoutingTable[i].igrpRoutingTableEntry));

        int numEntries = BUFFER_GetCurrentSize(&(igrpRoutingTable[i].
            igrpRoutingTableEntry)) / sizeof (IgrpRoutingTableEntry);

        // tos weight parameters of IGRP
        int k1 = igrpRoutingTable[i].k1;
        int k2 = igrpRoutingTable[i].k2;
        int k3 = igrpRoutingTable[i].k3;
        int k4 = igrpRoutingTable[i].k4;
        int k5 = igrpRoutingTable[i].k5;

        IgrpRoutingTableEntry* matchingAndMinimalEntry = NULL;

        for (j = 0; j <numEntriesInUpdateMesage; j++)
        {
            float var = 0;
            float newMetric = 0;
            float remoteMetric =  0;
            BOOL found = FALSE;
            BOOL isLastEntry = FALSE;
            BOOL isHoldDown = FALSE;
            BOOL isHopCountExceededForMatchingPath = FALSE;
            IgrpRoutingTableEntry* matchingPathEntry = NULL;
            IgrpRoutingInfo* newPathMetric = NULL;

            // Remove path from the routing table if present
            IgrpFindRouteToUpdateOrDelete(
                &(igrpRoutingTable[i].igrpRoutingTableEntry),
                &destAddressArray[j],
                interfaceIndex,
                FALSE,
                &isLastEntry,
                &isHoldDown,
                &matchingPathEntry,
                sourceAddr); // source Address TBD

            //External routes should get updated irrespective of
            //hold down timer state.
            if ((isLastEntry) && (isHoldDown)
                && (!hook))
            {
                // if route already exists in the routing table and
                // route is in hold down state then continue
                // with next routing entry
                continue;
            }

            // find if route already exists in the routing table or not
            found = IgrpFindMatchingDestinationInRoutingTable(
                        igrpRoutingTableEntry,
                        numEntries,
                        destAddressArray[j].number,
                        &matchingAndMinimalEntry);

            // calculate the vector metric to the destination
            // via the sender of the update message
            newPathMetric = IgrpCalculateNewPathMetricViaSource(
                                node,
                                &destAddressArray[j],
                                interfaceIndex);

            // calculate the new metric value
            newMetric = IgrpCalculateMetric(
                            newPathMetric, k1, k2, k3, k4, k5);

            // calculate the metric of the sender of the update message
            remoteMetric = IgrpCalculateMetric(
                               &destAddressArray[j], k1, k2, k3, k4, k5);

            if (!found)
            {
                // if the destination is not found in the routing
                // table add the destimation to the routing table.
                // And send an update message
                if (IGRP_DEBUG)
                {
                    char sourceAddress[MAX_STRING_LENGTH] = {0};
                    char destAddress[MAX_STRING_LENGTH] = {0};

                    IO_ConvertIpAddressToString(sourceAddr, sourceAddress);

                    IO_ConvertIpAddressToString(IgrpConvertCharArrayToInt(
                        destAddressArray[j].number), destAddress);

                    printf("Node = %u,  update from %s,  metric = %f"
                           " dest = %s\n",
                           node->nodeId,
                           sourceAddress,
                           newMetric,
                           destAddress);
                }

                if (IsDestUnreachable(destAddressArray[j].delay))
                {
                    MEM_free(newPathMetric);
                    newPathMetric = NULL;
                    continue;
                }

                // Add the route to the IGRP routing table
                IgrpAddNewRouteToRoutingTable(
                    node,
                    &(igrpRoutingTable[i].igrpRoutingTableEntry),
                    newPathMetric,
                    newMetric,
                    remoteMetric,
                    asystem,
                    ((asystem == igrp->asystem) ? IGRP_SYSTEM_ROUTE :
                        IGRP_EXTERIOR_ROUTE),
                    sourceAddr,
                    interfaceIndex,
                    TRUE,
                    node->getNodeTime(),
            hook);

                MEM_free(newPathMetric);
                newPathMetric = NULL;

                // trigger an update message
                IgrpScheduleTriggeredUpdate(node);
            }
            else
            {
                if (igrp->variance == 1)
                {
                    var = 1.1f;
                }
                else
                {
                    var = igrp->variance;
                }

                if (IGRP_DEBUG)
                {
                    char destAddress[MAX_STRING_LENGTH] = {0};
                    char sourceAddress[MAX_STRING_LENGTH] = {0};
                    NodeAddress dest = IgrpConvertCharArrayToInt(
                                           destAddressArray[j].number);

                    IO_ConvertIpAddressToString(dest, destAddress);
                    IO_ConvertIpAddressToString(sourceAddr, sourceAddress);

                    printf("node = %u,  update from %s,  metric = %f"
                           " pre metric = %f dest = %s\n",
                           node->nodeId,
                           sourceAddress,
                           newMetric,
                           matchingAndMinimalEntry->metric,
                           destAddress);
                }

                if (newMetric > matchingAndMinimalEntry->metric)
                {
                    if (matchingPathEntry)
                    {
                        isHopCountExceededForMatchingPath =
                        (matchingAndMinimalEntry->vectorMetric.hopCount <
                               destAddressArray[j].hopCount);
                    }

                    // if (destination is shown as unreachable in update)
                    // OR if (holddown is enabled AND newMetric < variance
                    // * existing metric) OR if (holddown is disabled AND
                    // new update has a hop count greater than old
                    // hop count)
                    var = var * matchingAndMinimalEntry->metric;

                    if (IsDestUnreachable(destAddressArray[j].delay)  ||
                       ((igrp->holdDownEnabled) && (newMetric > var)) ||
                       ((!igrp->holdDownEnabled) &&
                           isHopCountExceededForMatchingPath))
                    {
                        if (!matchingPathEntry)
                        {
                            // possibly a loop or or infeasible route so
                            // do not add the entry to the routing table
                            MEM_free(newPathMetric);
                            continue;
                        }

                        //hook is checked to propagate invalid external
                        //routes to other nodes in subnet
                        if (((isLastEntry) && (igrp->holdDownEnabled))
                              || hook)
                        {
                            Message* msg = NULL;
                            IgrpHoldDownInfoType holdDownInfo;

                            // do not delete the route but set
                            // delay to destination inaccessable
                            // and put it to holddown state
                            matchingPathEntry->vectorMetric.delay[0] = 0xFF;
                            matchingPathEntry->vectorMetric.delay[1] = 0xFF;
                            matchingPathEntry->vectorMetric.delay[2] = 0xFF;
                            matchingPathEntry->isInHoldDownState = TRUE;

                            matchingPathEntry->metric =
                                IgrpCalculateMetric(
                                    &matchingPathEntry->vectorMetric,
                                    k1, k2, k3, k4, k5);

                            // set and fire holddown timer
                            msg = MESSAGE_Alloc(
                                      node,
                                      NETWORK_LAYER,
                                      ROUTING_PROTOCOL_IGRP,
                                      MSG_ROUTING_IgrpHoldTimerExpired);

                            MESSAGE_InfoAlloc(node, msg,
                                sizeof(IgrpHoldDownInfoType));

                            memcpy(&holdDownInfo.routingInfo,
                                   &matchingPathEntry->vectorMetric,
                                   sizeof(IgrpRoutingInfo));

                            holdDownInfo.tos = i;
                            holdDownInfo.interfaceIndex =
                                matchingPathEntry->outgoingInterface;

                            memcpy(MESSAGE_ReturnInfo(msg),
                                &holdDownInfo,
                                sizeof(IgrpHoldDownInfoType));

                            MESSAGE_Send(node, msg, igrp->holdTime);
                        }
                        else
                        {
                            // ... or if path is not the last entry
                            // to the destination, of if hold down
                            // feature is disabled then delete the
                            // entry from the routing table
                            BUFFER_ClearDataFromDataBuffer(
                                &(igrpRoutingTable[i].
                                    igrpRoutingTableEntry),
                                (char*) matchingPathEntry,
                                sizeof(IgrpRoutingTableEntry),
                                FALSE);
                        }

                        // and also send an update message
                        IgrpScheduleTriggeredUpdate(node);

                        // update the network layer forwarding table
                        if (isLastEntry && !hook)
                        {
                            NetworkUpdateForwardingTable(
                                node,
                                (IgrpConvertCharArrayToInt(
                                    newPathMetric->number) << 8),
                                IgrpReturnSubnetMaskFromIPAddress(
                                    (IgrpConvertCharArrayToInt(
                                         newPathMetric->number) << 8)),
                                (NodeAddress) NETWORK_UNREACHABLE,
                                ANY_INTERFACE,
                                IGRP_COST_INFINITY,
                                ROUTING_PROTOCOL_IGRP);
                        }
                        else
                        {
                            IgrpUpdateForwardingTableWithAlternetPath(
                                node,
                                igrp,
                                newPathMetric->number);
                        }
                    }
                    else if (matchingPathEntry)
                    {
                        // update an existing entry with new metric value

                        memcpy(&matchingPathEntry->vectorMetric,
                            newPathMetric,
                            sizeof(IgrpRoutingInfo));

                        matchingPathEntry->metric = newMetric;

                        matchingPathEntry->remoteCompositeMetric =
                            remoteMetric;

                        matchingPathEntry->lastUpdateTime =
                            node->getNodeTime();
                    }
                    else
                    {
                        IgrpAddNewRouteToRoutingTable(
                            node,
                            &(igrpRoutingTable[i].igrpRoutingTableEntry),
                            newPathMetric,
                            newMetric,
                            remoteMetric,
                            asystem,
                            ((asystem == igrp->asystem) ? IGRP_SYSTEM_ROUTE :
                                IGRP_EXTERIOR_ROUTE),
                            sourceAddr,
                            interfaceIndex,
                            FALSE,
                            node->getNodeTime(),
                hook);
                    }
                    MEM_free(newPathMetric);
                }
                else if (newMetric <= matchingAndMinimalEntry->metric)
                {
                    BOOL notUsed = FALSE;
                    IgrpRoutingTableEntry* routeToUpdate = NULL;
                    NodeAddress destination = 0;

                    if (!IsDestUnreachable(newPathMetric->delay))
                    {
                        destination = sourceAddr;
                    }
                    else
                    {
                        destination = (NodeAddress) NETWORK_UNREACHABLE;
                    }

                    IgrpFindRouteToUpdateOrDelete(
                        &(igrpRoutingTable[i].igrpRoutingTableEntry),
                        newPathMetric,
                        interfaceIndex,
                        FALSE,
                        &notUsed,
                        &notUsed,
                        &routeToUpdate,
                        sourceAddr);

                    if (routeToUpdate)
                    {
                        // update an already existing route
                        memcpy(&routeToUpdate->vectorMetric,
                            newPathMetric,
                            sizeof(IgrpRoutingInfo));

                        routeToUpdate->metric = newMetric;

                        routeToUpdate->remoteCompositeMetric =
                            remoteMetric;

                        routeToUpdate->lastUpdateTime = node->getNodeTime();
                    }
                    else
                    {
                        // This is a route with a same metric value.
                        // But it is a differint path (i.e. uses
                        // different interface index for routing).
                        // Add this route to the routing table
                        IgrpAddNewRouteToRoutingTable(
                            node,
                            &(igrpRoutingTable[i].igrpRoutingTableEntry),
                            newPathMetric,
                            newMetric,
                            remoteMetric,
                            asystem,
                            ((asystem == igrp->asystem) ?
                                IGRP_SYSTEM_ROUTE : IGRP_EXTERIOR_ROUTE),
                            sourceAddr,
                            interfaceIndex,
                            (newMetric < matchingAndMinimalEntry->metric ?
                                TRUE : FALSE),
                            node->getNodeTime(),
                hook);
                    }

                    IgrpRemoveUnfeasiblePaths(
                        &igrpRoutingTable[i],
                        igrp,
                        newPathMetric->number,
                        newMetric);

                    MEM_free(newPathMetric);
                    newPathMetric = NULL;
                } // else if (newMetric <= matchingEntry->metric)
            } // end of else part of the "if (!found)"

            igrpRoutingTableEntry = (IgrpRoutingTableEntry*)
                BUFFER_GetData(&(igrpRoutingTable[i].igrpRoutingTableEntry));

            numEntries = BUFFER_GetCurrentSize(&(igrpRoutingTable[i].
                igrpRoutingTableEntry)) / sizeof (IgrpRoutingTableEntry);

        } // end for (j = 0; j <numEntriesInUpdateMesage; j++)
    } // end for (i = 0; i < numOfTos; i++)
} // end RoutingIgrpProcessNewIncomingUpdate()


// Added for Route Redistribution
#ifdef ROUTE_REDISTRIBUTE

void IgrpHookToRedistribute(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    void* igrpRoutingInfoStruct)
{
    Message* msg = MESSAGE_Alloc(
                        node,
                        MAC_LAYER,
                        ROUTING_PROTOCOL_IGRP,
                        0);

    IgrpHeader igrpHeader = {0};
    IgrpRoutingInfo igrpRoute = {{0}};
    IgrpRoutingInfo* igrpRoutingInfo = (IgrpRoutingInfo*)
        igrpRoutingInfoStruct;

    // Build a dummy IGRP Header with dummy header information
    IgrpHeaderSetVersion(&(igrpHeader.IgrpHdr), 1);// protocol version number
    IgrpHeaderSetOpCode(&(igrpHeader.IgrpHdr), IGRP_UPDATE);// opcode
    igrpHeader.edition = 0;                  // edition number
    igrpHeader.asystem = IGRP_DEFAULT_AS_ID; // autonomous system number
    igrpHeader.ninterior = 0;                // num of subnets in local net
    igrpHeader.nsystem = 0;                  // num of networks in AS
    igrpHeader.nexterior = 0;                // num of networks outside AS
    igrpHeader.checksum = 0;                 // checksum of IGRP hdr & data

    // Build a metric information.
    memset(
        &igrpRoute,
        0,
        sizeof(IgrpRoutingInfo));

    igrpRoute.number[2] = ((unsigned char) (0xFF & (destAddress >> 24)));
    igrpRoute.number[1] = ((unsigned char) (0xFF & (destAddress >> 16)));
    igrpRoute.number[0] = ((unsigned char) (0xFF & (destAddress >> 8)));

    // Explicitly METRIC Argument has been specified
    if (igrpRoutingInfo != NULL)
    {
        //If nexthop is unreachable then make the route inaccessible
        //by setting delay as unreachable
        if (nextHopAddress == (unsigned) NETWORK_UNREACHABLE)
        {
            igrpRoute.delay[0] = 0xFF;
            igrpRoute.delay[1] = 0xFF;
            igrpRoute.delay[2] = 0xFF;
        }
        else
        {
            memcpy(
            igrpRoute.delay,
            &igrpRoutingInfo->delay,
            sizeof(igrpRoutingInfo->delay));
        }
        igrpRoute.hopCount = igrpRoutingInfo->hopCount;
        memcpy(
            igrpRoute.bandwidth,
            &igrpRoutingInfo->bandwidth,
            sizeof(igrpRoutingInfo->bandwidth));

        igrpRoute.reliability = igrpRoutingInfo->reliability;
        igrpRoute.load = igrpRoutingInfo->load;
        igrpRoute.mtu = igrpRoutingInfo->mtu;
    }

    // Encapsulate the message in the packet.
    MESSAGE_PacketAlloc(
        node,
        msg,
        sizeof(IgrpRoutingInfo),
        TRACE_IGRP);

    memcpy(
        msg->packet,
        &igrpRoute,
        sizeof(IgrpRoutingInfo));

    // Add header.
    MESSAGE_AddHeader(
        node,
        msg,
        sizeof(IgrpHeader),
        TRACE_IGRP);

    memcpy(
        msg->packet,
        &igrpHeader,
        sizeof(IgrpHeader));

    RoutingIgrpProcessNewIncomingUpdate(
        node,
        msg,
        nextHopAddress,
        interfaceIndex,
        TRUE);
}

#endif  // ROUTE_REDISTRIBUTE


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpGetOpcode()
//
// PURPOSE      : Extracting IGRP opcode from the header
//                of the IGRP protocol packet
// PARAMETER    : packet - IGRP protocol packet
//
// RETURN VALUE : IGRP opcode
//
//-------------------------------------------------------------------------
static
char IgrpGetOpcode(char* packet)
{
    return (char)(IgrpHeaderGetOpCode(((IgrpHeader*)(packet))->IgrpHdr));
}


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpHandleProtocolPacket()
//
// PURPOSE      : Receiving and processing Igrp protocol-specific packets
//
// PARAMETERS   : node - the node which is receiving the packet
//                msg - the protocol packet received by the node
//                sourceAddr - neighbor address from whom the
//                            protocol packet has been received
//                interfaceIndex - interface index through which
//                                 the packet has been received.
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
void IgrpHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddr,
    int interfaceIndex)
{
    switch (IgrpGetOpcode(msg->packet))
    {
        #if 0
        // This portion is currently not being used.
        case IGRP_REQUEST :
        {
            IgrpSendUpdateMessage(node);
            break;
        }
        #endif
        case IGRP_UPDATE :
        {
            RoutingIgrpProcessNewIncomingUpdate(
                node,
                msg,
                sourceAddr,
                interfaceIndex,
                FALSE);

            break;
        }
        default :
        {
            ERROR_Assert(FALSE, "ERROR : unknown IGRP opcode\n");
            break;
        }
    } // end switch (IgrpGetOpcode(msg->packet))
    MESSAGE_Free(node, msg);
} // end NetworkIgrpHandleProtocolPacket()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpGetInterfaceInfo()
//
// PURPOSE      : To set/reset interface related parameters
//                useful for IGRP (delay). If the interface is
//                down delay is equal to DESTINATION_INACCESSABLE.
//
// PARAMETERS   : node - the node performing the operation
//                routingEntry - a single row of a routing table
//                isInterfaceEnabled - is interface enabled ?
//                TRUE or FALSE
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpGetInterfaceInfo(
    Node* node,
    IgrpRoutingTableEntry* routingEntry,
    BOOL isInterfaceEnabled,
    int metricInfo[])
{
    NodeAddress previousState = routingEntry->nextHop;
    NodeAddress currentState = previousState;
    // make destinations in accessable
    if (!isInterfaceEnabled)
    {
        // make destinations in accessable
        routingEntry->vectorMetric.delay[2] = 0xFF;
        routingEntry->vectorMetric.delay[1] = 0xFF;
        routingEntry->vectorMetric.delay[0] = 0xFF;
        routingEntry->nextHop = (NodeAddress) NETWORK_UNREACHABLE;
    }
    else
    {
        // update the delay information with interface data
        clocktype propDelay = IgrpGetPropDelay(node,
             routingEntry->outgoingInterface) / MICRO_SECOND ;

        routingEntry->vectorMetric.delay[2] =
            ((char) (0xFF & (propDelay >> 16)));
        routingEntry->vectorMetric.delay[1] =
            ((char) (0xFF & (propDelay >> 8)));
        routingEntry->vectorMetric.delay[0] =
            ((char) (0xFF & (propDelay)));
        routingEntry->nextHop = 0;
    }

    currentState = routingEntry->nextHop;
    routingEntry->metric = IgrpCalculateMetric(
                               &routingEntry->vectorMetric,
                               metricInfo[0],
                               metricInfo[1],
                               metricInfo[2],
                               metricInfo[3],
                               metricInfo[4]);

    if (previousState != currentState)
    {
        IgrpScheduleTriggeredUpdate(node);
    }
} // end IgrpGetInterfaceInfo()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpProcessInvalidTimeOut()
//
// PURPOSE      : processing routes whose invalid time has been expired
//
// PARAMETERS   : node - node which is processing the invalid
//                       timed-out routes
//                igrp - routing igrp structure
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpProcessInvalidTimeOut(
    Node* node,
    RoutingIgrp* igrp)
{
    int i = 0, j = 0;
    int k1 = 0, k2 = 0, k3 = 0, k4 = 0, k5 = 0;

    clocktype currentTime = node->getNodeTime();

    IgrpRoutingTable* igrpRoutingTable = (IgrpRoutingTable*)
        BUFFER_GetData(&igrp->igrpRoutingTable);

    int numOfTos = BUFFER_GetCurrentSize(&igrp->igrpRoutingTable)
        / sizeof(IgrpRoutingTable);

    // The code fragment below processes the IGRP routes
    // whose invalid timer has been expired
    for (i = 0; i < numOfTos; i++)
    {
        IgrpRoutingTableEntry* routingEntry = (IgrpRoutingTableEntry*)
            BUFFER_GetData(&igrpRoutingTable[i].igrpRoutingTableEntry);

        int numEntry = BUFFER_GetCurrentSize(&igrpRoutingTable[i].
            igrpRoutingTableEntry) / sizeof(IgrpRoutingTableEntry);

        k1 = igrpRoutingTable[i].k1;
        k2 = igrpRoutingTable[i].k2;
        k3 = igrpRoutingTable[i].k3;
        k4 = igrpRoutingTable[i].k4;
        k5 = igrpRoutingTable[i].k5;

        for (j = 0; j < numEntry; j++)
        {
            // if not directly connected to this router

            if (!routingEntry[j].isPermanent)
            {

                if (routingEntry[j].vectorMetric.hopCount != 0)
                {
                    if (((routingEntry[j].lastUpdateTime
                        + igrp->invalidTime) > currentTime) ||
                        IsDestUnreachable(routingEntry[j].vectorMetric.delay))
                    {
                        // If current time < Paths'S LAST UPDATE TIME +
                        // INVALID TIME then continue with the next path
                        continue;
                    }
                    else
                    {
                        BOOL isLastEntry = FALSE;
                        BOOL isHoldDown  = FALSE;
                        IgrpRoutingTableEntry* routeToDelete = NULL;

                        // Remove Path from routing table
                        IgrpFindRouteToUpdateOrDelete (
                            &igrpRoutingTable[i].igrpRoutingTableEntry,
                            &routingEntry[j].vectorMetric,
                            routingEntry[j].outgoingInterface,
                            TRUE,
                            &isLastEntry,
                            &isHoldDown,
                            &routeToDelete,
                            ANY_IP);

                        // If Path was the last route to Dest
                        if (isLastEntry)
                        {
                            // set the metric as in accessable
                            routeToDelete->vectorMetric.delay[0] = 0xFF;
                            routeToDelete->vectorMetric.delay[1] = 0xFF;
                            routeToDelete->vectorMetric.delay[2] = 0xFF;

                            routeToDelete->metric =
                                IgrpCalculateMetric(
                                &routeToDelete->vectorMetric,
                                k1, k2, k3, k4, k5);

                            // If hold down feature is enabled then..
                            if (igrp->holdDownEnabled)
                            {
                                // Set metric for D to inaccessible
                                // Unless holddowns are disabled,
                                // Start holddown timer for D

                                Message* msg = NULL;
                                IgrpHoldDownInfoType holdDownInfo;

                                routeToDelete->isInHoldDownState = TRUE;
                                msg = MESSAGE_Alloc(
                                    node,
                                    NETWORK_LAYER,
                                    ROUTING_PROTOCOL_IGRP,
                                    MSG_ROUTING_IgrpHoldTimerExpired);

                                MESSAGE_InfoAlloc(node, msg,
                                    sizeof(IgrpHoldDownInfoType));

                                memcpy(&holdDownInfo.routingInfo,
                                    &routeToDelete->vectorMetric,
                                    sizeof(IgrpRoutingInfo));

                                holdDownInfo.tos = i;

                                holdDownInfo.interfaceIndex =
                                    routeToDelete->outgoingInterface;

                                memcpy(MESSAGE_ReturnInfo(msg),
                                    &holdDownInfo,
                                    sizeof(IgrpHoldDownInfoType));

                                MESSAGE_Send(node, msg, igrp->holdTime);
                            } // end if (igrp->holdDownEnabled)

                            // Trigger an update message
                            IgrpScheduleTriggeredUpdate(node);

                            NetworkUpdateForwardingTable(
                                node,
                                (IgrpConvertCharArrayToInt(
                                routeToDelete->vectorMetric.number) << 8),
                                IgrpReturnSubnetMaskFromIPAddress(
                                IgrpConvertCharArrayToInt(
                                routeToDelete->vectorMetric.number) << 8),
                                (NodeAddress) NETWORK_UNREACHABLE,
                                ANY_INTERFACE,
                                IGRP_COST_INFINITY,
                                ROUTING_PROTOCOL_IGRP);
                        } // end if (isLastentry)
                        else
                        {
                            unsigned char ipaddrBytes[3];
                            memcpy(ipaddrBytes,
                                   routeToDelete->vectorMetric.number,
                                   3);

                            // delete the route from routing table
                            BUFFER_ClearDataFromDataBuffer(
                                &(igrpRoutingTable[i].
                                igrpRoutingTableEntry),
                                (char*) routeToDelete,
                                sizeof(IgrpRoutingTableEntry),
                                FALSE);

                            IgrpUpdateForwardingTableWithAlternetPath(
                                node,
                                igrp,
                                ipaddrBytes);

                        } // end else part of "if (isLastentry)"

                        // increment the number of triggerde update
                        igrp->igrpStat.numRouteTimeouts++;
                    }
                } // end if (routingEntry[j].vectorMetric.hopCount != 0)
                else
                {
                    // Check this interface is dead or not
                    int metricInfo[5] = {k1, k2, k3, k4, k5};
                    IgrpGetInterfaceInfo(
                        node,
                        &routingEntry[j],
                        NetworkIpInterfaceIsEnabled(node,
                        routingEntry[j].outgoingInterface),
                        metricInfo);
                }
            }
        } // end for (j = 0; j < numEntry; j++)
    } // end for (i = 0; i < numOfTos; i++)
} // end IgrpProcessInvalidTimeOut()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpProcessFlushTimeOut()
//
// PURPOSE      : processing routes whose flush time has been expired
//
// PARAMETERS   : node - node which is processing the flushing
//                       timed-out routes
//                igrp - routing igrp structure
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
static
void IgrpProcessFlushTimeOut(
    Node* node,
    RoutingIgrp* igrp)
{
    int i = 0, j = 0;

    clocktype currentTime = node->getNodeTime();

    IgrpRoutingTable* igrpRoutingTable = (IgrpRoutingTable*)
        BUFFER_GetData(&igrp->igrpRoutingTable);

    int numOfTos = BUFFER_GetCurrentSize(&igrp->igrpRoutingTable)
        / sizeof(IgrpRoutingTable);

    // The code fragment below processes the IGRP routes
    // whose flush timer has been expired
    for (i = 0; i < numOfTos; i++)
    {
        IgrpRoutingTableEntry* routingEntry = (IgrpRoutingTableEntry*)
            BUFFER_GetData(&igrpRoutingTable[i].igrpRoutingTableEntry);

        int numEntry = BUFFER_GetCurrentSize(&igrpRoutingTable[i].
            igrpRoutingTableEntry) / sizeof(IgrpRoutingTableEntry);

        for (j = 0; j < numEntry; j++)
        {
            // If Destination metric is inaccessible then
            if ((routingEntry[j].vectorMetric.hopCount != 0) &&
                IsDestUnreachable(routingEntry[j].vectorMetric.delay))
            {
                // If current time >= Destination "D" last
                // update time + flush time then
                if ((routingEntry[j].lastUpdateTime
                     + igrp->flushTime) <= currentTime)
                {
                    // Remove entry for D
                    BUFFER_ClearDataFromDataBuffer(
                        &(igrpRoutingTable[i].igrpRoutingTableEntry),
                        (char*) &routingEntry[j],
                        sizeof(IgrpRoutingTableEntry),
                        FALSE);

                    numEntry--;
                    j--;
                }
            }
        } // end for (j = 0; j < numEntry; j++)
    } // end for (i = 0; i < numOfTos; i++)
} // end IgrpProcessFlushTimeOut()


//-------------------------------------------------------------------------
//
// FUNCTION     IgrpGetNextHops()
//
// PURPOSE      Determine the next hop/hops for a perticular dest. If dest
//              address is directly connected to the node then the
//              interface index is al ways found in the position
//              decisionStruct->interfaceIndexArray[0]
//
// PARAMETERS   igrp            - the igrp struct
//              tos             - type of service
//              destAddr        - the destination
//              alternateRoutes - set of next hop along with the following
//                                information : interface index, number of
//                                packets send, metric, remote metric.
//              numAlternatinesRoutes - number of alternative routes in
//                                     structure alternateRoutes[],
//
// RETURN VALUE : BOOL decides if dest is directly connected
//                to node or not.
//
//-------------------------------------------------------------------------
static
BOOL IgrpGetNextHops(
    RoutingIgrp* igrp,
    char tos,
    NodeAddress destAddr,
    NodeAddress previouHopAddress,
    IgrpRoutingTableEntry* alternateRoutes[],
    int* numAlternatinesRoutes)
{
    // The field ipHeader->ip_tos of Ip header is currently not used. Igrp
    // is assumed to have separate routing table for different type of
    // service but currently only one type of service is supported.

    int i = 0, index = 0;

    IgrpRoutingTable* igrpRoutingTable = (IgrpRoutingTable*)
        BUFFER_GetData(&igrp->igrpRoutingTable);

    IgrpRoutingTableEntry* igrpRoutingTableEntry =
        (IgrpRoutingTableEntry*)
        BUFFER_GetData(&igrpRoutingTable[0].igrpRoutingTableEntry);

    int numEntry = BUFFER_GetCurrentSize(&igrpRoutingTable[0].
        igrpRoutingTableEntry) / sizeof(IgrpRoutingTableEntry);

    NodeAddress subnetMask = IgrpReturnSubnetMaskFromIPAddress(destAddr);

    *numAlternatinesRoutes = 0;

    for (i = 0; i < numEntry; i++)
    {
        if (((IgrpConvertCharArrayToInt(igrpRoutingTableEntry[i].
              vectorMetric.number) << 8) == (destAddr & subnetMask)) &&
            (!IsDestUnreachable(igrpRoutingTableEntry[i].
                vectorMetric.delay)))
        {
            /*Commented During ARP task, to remove previous hop dependency*/
            /*if (previouHopAddress == igrpRoutingTableEntry[i].nextHop)
            {
                // previous hop and next hop is in the same
                // network. Do not choose it to forward packet
                continue;
            }*/

            // is the destination itself the next hop ? ||
            // is the network directly connected to the node.
            if ((igrpRoutingTableEntry[i].nextHop ==
                destAddr) ||
                (igrpRoutingTableEntry[i].nextHop == 0))
            {
                *numAlternatinesRoutes = 1;
                alternateRoutes[0] = &igrpRoutingTableEntry[i];
                return TRUE;
            }
            else
            {
                (*numAlternatinesRoutes)++;
                alternateRoutes[index] = &igrpRoutingTableEntry[i];
                index++;
            }
        } // end if (igrpRoutingTableEntry.vectorMetric.number == dest)
    }// end for (i = 0; i < numEntries; i++)
    return FALSE;
}//end IgrpGetNextHops()


//-------------------------------------------------------------------------
//
// FUNCTION     RoutingIgrpGetRoute()
//
// PURPOSE      Choose best possible next hop among possibles
//
// PARAMETERS   decisionStruct  - set of next hop along with the following
//                                information : interface index, number of
//                                packets send, metric, remote metric.
//
// RETURN VALUE : the best interface index
//
//-------------------------------------------------------------------------
static
IgrpRoutingTableEntry* RoutingIgrpGetRoute(
    IgrpRoutingTableEntry* routeptr[],
    int numEntries)
{
    double minMetricValue = DBL_MAX;
    int index = 0, i = 0;

    // if there is only one path to the destination...
    if (numEntries == 1)
    {
        routeptr[0]->numOfPacketSend++;

        // return the path indexed by 0'th entry
        return(routeptr[0]);
    }

    for (i = 0; i < numEntries; i++)
    {
        double numOfPacketSend = routeptr[i]->numOfPacketSend;
        double metric = routeptr[i]->metric * numOfPacketSend;

        if (metric < minMetricValue)
        {
            // record index with minimal metric
            index = i;
            minMetricValue = metric;
        }
    }

    // otherwise return the path indexed by 0'th entry
    routeptr[index]->numOfPacketSend++;

    return routeptr[index];
} // end RoutingIgrpGetRoute()


//-------------------------------------------------------------------------
//
// FUNCTION     IgrpRouterFunction()
//
// PURPOSE      Determine the routing action to take for a the given data
//              packet, and set the PacketWasRouted variable to TRUE if no
//              further handling of this packet by IP is necessary.
// PARAMETERS   msg             - the data packet
//              destAddr        - the destination node of this data packet
//              previouHopAddress - notUsed
//              PacketWasRouted - variable that indicates to IP that it
//                               no longer needs to act on this
//                                data packet
//
//-------------------------------------------------------------------------
void IgrpRouterFunction(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previouHopAddress,
    BOOL* packetWasRouted)
{
    int i = 0;
    IgrpRoutingTableEntry* routePtr = NULL;
    IgrpRoutingTableEntry** alternateRoutes = NULL;
    int numRoutes = 0;

    RoutingIgrp* igrp = (RoutingIgrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IGRP);

    IpHeaderType* ipHeader =  (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        // match the destination address of the packet against
        // all the interface address to see if the packet intended
        // for this node or not. If the packet is intended for this
        // node then do not route it and return.
        if ((destAddr == NetworkIpGetInterfaceAddress(node, i)) ||
            (destAddr == NetworkIpGetInterfaceBroadcastAddress (node, i)))
        {
            *packetWasRouted = FALSE;
            return;
        }
    }

    // if this packet is not intended for this node then find
    // a route (or possibly a set of routes)from the routing
    // to route the packet table
    alternateRoutes = (IgrpRoutingTableEntry**)
        MEM_malloc((node->numberInterfaces)
            * sizeof(IgrpRoutingTableEntry*));

    if (IgrpGetNextHops(
            igrp,
            (char) IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len),
            destAddr,
            previouHopAddress,
            alternateRoutes,
            &numRoutes))
    {
        // If destination is on a directly-connected network
        // then send packet direct to the destination.
        NetworkIpSendPacketToMacLayer(
            node,
            msg,
            alternateRoutes[0]->outgoingInterface,
            destAddr);

        igrp->igrpStat.packetSentStat
            [alternateRoutes[0]->outgoingInterface]++;

        alternateRoutes[0]->numOfPacketSend ++;
        *packetWasRouted = TRUE;

        MEM_free(alternateRoutes);
        alternateRoutes = NULL;
        return;
    }

    if (numRoutes == 0)
    {
        // If there are no paths to the destination in the routing
        // table, or all paths are upstream then send
        // protocol-specific error message and discard the packet
        *packetWasRouted = FALSE;
        MEM_free(alternateRoutes);
        alternateRoutes = NULL;
        return;
    }

    // Choose the next path to use. If there are more than
    // alternate round-robin with frequency proportional
    // to inverse of composite metric.
    routePtr = RoutingIgrpGetRoute(alternateRoutes, numRoutes);

    NetworkIpSendPacketToMacLayer(
        node,
        msg,
        routePtr->outgoingInterface,
        routePtr->nextHop);

    ERROR_Assert((routePtr->outgoingInterface < node->numberInterfaces),
        "Attempt to send packet thru an in valid interface");
    igrp->igrpStat.packetSentStat[routePtr->outgoingInterface]++;

    if (IGRP_DEBUG)
    {
        char destAddress[MAX_STRING_LENGTH] = {0};

        IO_ConvertIpAddressToString(destAddr, destAddress);
        printf("node = %u destAddress == %s  interface  = %u \n",
               node->nodeId,
               destAddress,
               routePtr->outgoingInterface);
        IgrpPrintIgrpRoutingTable(node);
    }

    *packetWasRouted = TRUE;

    MEM_free(alternateRoutes);
    alternateRoutes = NULL;
} // IgrpRouterFunction()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpDoTimerValidityTest()
//
// PURPOSE      : Doing validity test for timers.
//
// PARAMETERS   : node - node which is reading the timer value.
//                timeString - name of the timer
//                lowestBoundary - lowest value of the timer (exclusive)
//                suggestedValue - suggested value of the timer
//                valueRead - actual value read
//
// RETURN VALUE : none
//
// ASSUMPTION   : none
//
//-------------------------------------------------------------------------
static
void IgrpDoTimerValidityTest(
    Node* node,
    const char* timerString,
    clocktype lowestBoundary,
    clocktype suggestedValue,
    clocktype valueRead)
{
    char errStr[MAX_STRING_LENGTH] = {0};
    char timer[MAX_STRING_LENGTH] = {0};

    clocktype tolerance = 10 * SECOND;
    ctoa(valueRead, timer);

    if (valueRead <= lowestBoundary)
    {
        sprintf(errStr, "node %u:Time-value for %s timer"
                " is wrongly given. Value = %s ns",
                node->nodeId,
                timerString,
                timer);
        ERROR_ReportError(errStr);
    }

    if (((valueRead > (suggestedValue + tolerance)) ||
        (valueRead < (suggestedValue - tolerance))))
    {
        sprintf(errStr, "node %u:Time-value for %s timer"
                " given is not suggested value . Value = %s ns",
                node->nodeId,
                timerString,
                timer);
        ERROR_ReportWarning(errStr);
    }
}


//-------------------------------------------------------------------------
//
// FUNCTION     : RoutingIgrpInit()
//
// PURPOSE      : Initializing the IGRP protocol structure
//
// PARAMETERS   : node - The node being initialized
//                igrp - The igrp routing structure
//                nodeInput - The input configuration file
//                interfaceIndex - The tinterface index
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
void IgrpInit(
    Node* node,
    RoutingIgrp** igrp,
    const NodeInput* nodeInput,
    int interfaceIndex)
{
    BOOL wasFound = FALSE;
    NodeInput igrpConfigFile;
    Message* msgPeriodic = NULL;
    Message* msgBroadcast = NULL;
    int numHops = 0;
    BOOL isIgrpRouter = FALSE;
    char buffer[MAX_STRING_LENGTH] = {0};
    NodeAddress interfaceAddress = 
                NetworkIpGetInterfaceAddress(node, interfaceIndex);

    // Read a configuration parameter that will
    // enable IGRP routing protocol
    IO_ReadCachedFile(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "IGRP-CONFIG-FILE",
        &wasFound,
        &igrpConfigFile);

    if (!wasFound)
    {
        ERROR_ReportError("IGRP Configuration file not Found");
    }

    IgrpReadFromConfigurationFile(
        node,
        igrp,
        &igrpConfigFile,
        &isIgrpRouter);

    if (isIgrpRouter == FALSE)
    {
        // I am not an IGRP router so abort IGRP initialization
        *igrp = NULL;
        return;
    }

    RANDOM_SetSeed((*igrp)->seed,
                   node->globalSeed,
                   node->nodeId,
                   ROUTING_PROTOCOL_IGRP,
                   interfaceIndex);

    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "IGRP-BROADCAST-TIME",
        &wasFound,
        &((*igrp)->broadcastTime));

    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "IGRP-INVALID-TIME",
        &wasFound,
        &((*igrp)->invalidTime));

    IgrpDoTimerValidityTest(
        node,
        "IGRP-INVALID-TIME",
        0,
        (3 * ((*igrp)->broadcastTime)),
        ((*igrp)->invalidTime));

    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "IGRP-HOLD-TIME",
        &wasFound,
        &((*igrp)->holdTime));

    IgrpDoTimerValidityTest(
        node,
        "IGRP-HOLD-TIME",
        0,
        (3 * ((*igrp)->broadcastTime) + 10 * SECOND),
        ((*igrp)->holdTime));

    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "IGRP-FLUSH-TIME",
        &wasFound,
        &((*igrp)->flushTime));

    IgrpDoTimerValidityTest(
        node,
        "IGRP-FLUSH-TIME",
        0,
        (7 * ((*igrp)->broadcastTime)),
        ((*igrp)->flushTime));

    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "IGRP-PERIODIC-TIMER",
        &wasFound,
        &((*igrp)->periodicTimer));

    IgrpDoTimerValidityTest(
        node,
        "IGRP-PERIODIC-TIMER",
        0,
        IGRP_DEFAULT_PERIODIC_TIMER,
        ((*igrp)->periodicTimer));

    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "IGRP-SLEEP-TIME",
        &wasFound,
        &((*igrp)->sleepTime));

    IgrpDoTimerValidityTest(
        node,
        "IGRP-SLEEP-TIME",
        0,
        IGRP_MIN_SLEEP_TIME,
        ((*igrp)->sleepTime));

    // Read maximum number of hop count supported.
    IO_ReadInt(node->nodeId, interfaceAddress, nodeInput,
        "IGRP-MAXIMUM-HOPS", &wasFound, &numHops);

    if (wasFound && (numHops < IGRP_MINIMUM_NUM_HOPS))
    {
        char errorStr[MAX_STRING_LENGTH] = {0};
        int igrpMinNumHops = IGRP_MINIMUM_NUM_HOPS;

        sprintf(errorStr, "Number of hops is less than %d\n",
            igrpMinNumHops);
        ERROR_ReportError(errorStr);

        (*igrp)->maxHop = (unsigned char) numHops;
    }

    IO_ReadString(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "ROUTING-STATISTICS",
        &wasFound,
        buffer);

    if ((wasFound) && (strcmp(buffer, "YES") == 0))
    {
        (*igrp)->collectStat = TRUE;
    }

    // Set IgrpRouter function
    NetworkIpSetRouterFunction(
        node,
        &IgrpRouterFunction,
        interfaceIndex);

    // Set routing table update function for route redistribution
    RouteRedistributeSetRoutingTableUpdateFunction(
        node,
        &IgrpHookToRedistribute,
        interfaceIndex);


    // Send route broadcast message through all interfaces
    msgBroadcast = MESSAGE_Alloc(
                       node,
                       NETWORK_LAYER,
                       ROUTING_PROTOCOL_IGRP,
                       MSG_ROUTING_IgrpBroadcastTimerExpired);

    // Fire first broadcast timer (and randomize it).
    MESSAGE_Send(node, msgBroadcast, IGRP_START_UP_MIN_DELAY +
                 RANDOM_nrand((*igrp)->seed) / SECOND);

    // Set periodic timer
    msgPeriodic = MESSAGE_Alloc(
                      node,
                      NETWORK_LAYER,
                      ROUTING_PROTOCOL_IGRP,
                      MSG_ROUTING_IgrpPeriodicTimerExpired);

    // Fire periodic timer
    MESSAGE_Send(node, msgPeriodic, PROCESS_IMMEDIATELY);

    if (DEBUG_FAULT)
    {
        Message* msgDebug1 = NULL;
        Message* msgDebug2 = NULL;

        msgDebug1 = MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        ROUTING_PROTOCOL_IGRP,
                        MSG_NETWORK_PrintRoutingTable);

        msgDebug2 = MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        ROUTING_PROTOCOL_IGRP,
                        MSG_NETWORK_PrintRoutingTable);

        // Fire debugging message
        MESSAGE_Send(node, msgDebug1, 170 * SECOND);
        MESSAGE_Send(node, msgDebug2, 7 * MINUTE);
    }

    if (IGRP_DEBUG)
    {
        IgrpPrintIgrpStructure(node);
        printf("node :%4u initialized.\n\n", node->nodeId);
    }
}// end RoutingIgrpInit();


//-------------------------------------------------------------------------
//
// FUNCTION     : RoutingIgrpHandleProtocolEvent()
//
// PURPOSE      : Handling the timer-related events of IGRP protocol
//
// PARAMETER    : node - the node which is handling the event
//                msg - the event
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
void IgrpHandleProtocolEvent(
    Node* node,
    Message* msg)
{
    RoutingIgrp* igrp = (RoutingIgrp*)
         NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IGRP);

    switch (msg->eventType)
    {
        case MSG_ROUTING_IgrpBroadcastTimerExpired:
        {
            BOOL isUpdateSend = FALSE;

            if (IGRP_DEBUG)
            {
                char clockStr[16];
                ctoa(node->getNodeTime(), clockStr);

                printf("broadcast timer expired. node : %u is "
                       " sending message simtime : %s \n",
                       node->nodeId, clockStr);
            }

            // Send/trigger an update message
            isUpdateSend = IgrpSendUpdateMessage(node);

            // Reschedule next update message.
            MESSAGE_Send(
                node,
                msg,
                (igrp->broadcastTime + RANDOM_nrand(igrp->seed)));

            if (isUpdateSend)
            {
                // Increment stat;
                igrp->igrpStat.numRegularUpdates++;

                // Record the time when last update sent
                igrp->lastRegularUpdateSend = node->getNodeTime();
            }

            break;
        } // end case MSG_ROUTING_IgrpBroadcastTimerExpired
        case MSG_ROUTING_IgrpPeriodicTimerExpired :
        {
            // Process invalid time out
            IgrpProcessInvalidTimeOut(node, igrp);

            // Process flush time out
            IgrpProcessFlushTimeOut(node, igrp);

            MESSAGE_Send(node, msg, igrp->periodicTimer);
            break;
        } // end case MSG_ROUTING_IgrpPeriodicTimerExpired
        case MSG_ROUTING_IgrpHoldTimerExpired:
        {
            BOOL notUsed = FALSE;
            IgrpRoutingTableEntry* routeToUpdate = NULL;

            IgrpHoldDownInfoType* holdDownInfo = (IgrpHoldDownInfoType*)
                MESSAGE_ReturnInfo(msg);

            IgrpRoutingTable* igrpRoutingTable = (IgrpRoutingTable*)
                BUFFER_GetData(&igrp->igrpRoutingTable);

            // Find the routing table entry who's
            // hold down timer has been expired
            IgrpFindRouteToUpdateOrDelete(
                &(igrpRoutingTable[holdDownInfo->tos].
                    igrpRoutingTableEntry),
                &(holdDownInfo->routingInfo),
                holdDownInfo->interfaceIndex,
                FALSE,
                &notUsed,
                &notUsed,
                &routeToUpdate,
                ANY_IP); // Match any next hop

            if (routeToUpdate)
            {
                // Release the route from hold down state
                routeToUpdate->isInHoldDownState = FALSE;
            }

            // Release the hold down timer
            MESSAGE_Free(node, msg);

            break;
        } // end case MSG_ROUTING_IgrpHoldTimerExpired
        case MSG_ROUTING_IgrpTriggeredUpdateAlarm :
        {
            BOOL isUpdateSend = FALSE;

            // send update message
            isUpdateSend = IgrpSendUpdateMessage(node);

            if (isUpdateSend)
            {
                // Increment number of triggered sent
                // (statistics collection)
                igrp->igrpStat.numTriggeredUpdates++;
            }

            igrp->recentlyTriggeredUpdateScheduled = FALSE;

            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_NETWORK_PrintRoutingTable :
        {
            IgrpPrintIgrpRoutingTable(node);
            MESSAGE_Free(node, msg);
            break;
        }
        default :
        {
            ERROR_Assert(FALSE, "Error : unknown event fired\n");
            break;
        }
    } // end switch(msg->event)
} // end IgrpHandleProtocolEvent()


//-------------------------------------------------------------------------
//
// FUNCTION     : IgrpFinalize()
//
// PURPOSE      : finalizing and printing IGRP statistical data
//
// PARAMETERS   : node - node which is printing the statistics
//
// RETURN VALUE : void
//
//-------------------------------------------------------------------------
void IgrpFinalize(Node* node)
{
    char buffer[MAX_STRING_LENGTH] = {0};
    int i = 0;

    RoutingIgrp* igrp = (RoutingIgrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IGRP);

    if ((igrp == NULL) ||
        (igrp->collectStat == FALSE) ||
        (igrp->igrpStat.isAlreadyPrinted))
    {
        return;
    }

    if (IGRP_DEBUG)
    {
        IgrpPrintIgrpRoutingTable(node);
        IgrpPrintPacketSendingStatics(node);
    }

    sprintf(buffer, "Number of Regular Updates : %u",
        igrp->igrpStat.numRegularUpdates);
    IO_PrintStat(node, "Network", "IGRP", ANY_DEST, ANY_IP, buffer);

    sprintf(buffer, "Number of Triggered Updates : %u",
        igrp->igrpStat.numTriggeredUpdates);
    IO_PrintStat(node, "Network", "IGRP", ANY_DEST, ANY_IP, buffer);

    sprintf(buffer, "Number of Route Timeouts : %u",
        igrp->igrpStat.numRouteTimeouts);
    IO_PrintStat(node, "Network", "IGRP", ANY_DEST, ANY_IP, buffer);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        sprintf(buffer, "Number of Packets Send Thru"
                " interface (%u) is %u",
                i, igrp->igrpStat.packetSentStat[i]);
        IO_PrintStat(node, "Network", "IGRP", ANY_DEST, ANY_IP, buffer);
    }
    igrp->igrpStat.isAlreadyPrinted = TRUE;
}
