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
//--------------------------------------------------------------------------
//
// ASSUMPTIONS AND OMITTED FEATURES:
//
//
// 1) EXTERNAL ROUTE AND ROUTE-TAGGING IS NOT IMPLEMENTED.
//
// 2) EIGRP RELIABLY SENDS MULICAST UPDATE MESSAGE THRU RELIABLE TRANSFER
//    PROTOCOL (RTP). RTP SERVICE IS NOT AVAILBLE.
//
// 3) EIGRP IS INTENDED TO BE USED IN WIRED SCENARIOS ONLY
//
//--------------------------------------------------------------------------

#include <limits.h>
#include "api.h"
#include "network_ip.h"
#include "mac.h"
#include "routing_eigrp.h"

#include "network_dualip.h"

#define EIGRP_DEBUG          0
#define EIGRP_DEBUG_TABLE    0
#define DEBUG_EIGRP_QUERY    0
#define DEBUG_EIGRP_REPLY    0
#define DEBUG_UNICAST_UPDATE 0
#define DEBUG_SHOW_UPDATE    0
#define EIGRP_DEBUG_SUBNETS  0
#define DEBUG_EIGRP_ROUTING  0
#define EIGRP_FILTER_DEBUG   0

//--------------------------------------------------------------------------
//
// FUNCTION     : EgrpConvertCharArrayToInt()
//
// PURPOSE      : Converting the content of a character type array to
//                an unsigned integer
//
// PARAMETERS   : charArray[] - The character type array
//
// RETURN VALUE : The unsigned int
//
// ASSUMPTION   : The input array is exactly 3-byte long.
//
//--------------------------------------------------------------------------
static
unsigned int EigrpConvertCharArrayToInt(unsigned char arr[])
{
    unsigned value = ((0xFF & arr[2]) << 16)
                     + ((0xFF & arr[1]) << 8)
                     + (0xFF & arr[0]);
    return value;
}


//--------------------------------------------------------------------------
// FUNCTION     : PrintNeighborTable().
//
// PURPOSE      : Printing the eigrp neighbor table.
//
// PARAMETERS   : node - node which is printing the table
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void PrintNeighborTable(Node* node)
{
    int i = 0;
    RoutingEigrp* eigrp = (RoutingEigrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_EIGRP);

    printf("-----------------------------------------------------------"
           "----------\n");
    printf(" Neighbor Table of node %u\n", node->nodeId);
    printf("-----------------------------------------------------------"
           "----------\n");
    printf("Neighbor_Address interfaceIndex Neighbor'sHoldTime"
           "     helloTime state\n");
    printf("-----------------------------------------------------------"
           "----------\n");

    for (i = 0; i < node->numberInterfaces; i++)
    {
        char holdTime[MAX_STRING_LENGTH] = {0};
        char helloTime[MAX_STRING_LENGTH] = {0};

        if (eigrp->neighborTable[i].first)
        {
            EigrpNeighborInfo* neighborInfo = eigrp->neighborTable[i].first;

            ctoa(eigrp->neighborTable[i].holdTime, holdTime);
            ctoa(eigrp->neighborTable[i].helloInterval, helloTime);

            while (neighborInfo)
            {
                char neighborAddressStr[MAX_STRING_LENGTH] = {0};
                NodeAddress neighborAddress = neighborInfo->neighborAddr;

                IO_ConvertIpAddressToString(neighborAddress,
                    neighborAddressStr);

                printf("%16s %8u %21s %17s %2u\n",
                       neighborAddressStr,
                       i,
                       holdTime,
                       helloTime,
                       neighborInfo->state);
                neighborInfo = neighborInfo->next;
            } // while (neighborInfo)
        } // end if (eigrp->neighborTable[i].first)
    }// for (i = 0; i < node->numberInterfaces; i++)
    printf("-----------------------------------------------------------"
           "----------\n");
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpPrintExternRouteInfo()
//
// PURPOSE      : Printing External route information for debugging purpose
//
// PARAMETERS   : topologyTable - pointer to topology table
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpPrintExternRouteInfo(EigrpTopologyTableEntry* topologyTable)
{
    char originatingRouter[MAX_STRING_LENGTH] = {0};
    EigrpExtraTopologyMetric* eigrpExtraTopologyMetric =
        topologyTable->eigrpExtraTopologyMetric;

    IO_ConvertIpAddressToString(eigrpExtraTopologyMetric->originatingRouter,
         originatingRouter);

    printf("---------------\n"
           "EXTERNAL ROUTE!\n"
           "---------------\n"
           "Originating Router: %s\n"
           "Originiting As    : %u\n"
           "arbitraryTag      : %u\n"
           "External Metric   : %u\n"
           "Extern Reserved   : %u\n"
           "Extern ProtocolId : %u\n"
           "flags             : %u\n",
           originatingRouter,
           eigrpExtraTopologyMetric->originitingAs,
           eigrpExtraTopologyMetric->arbitraryTag,
           eigrpExtraTopologyMetric->externalProtocolMetric,
           eigrpExtraTopologyMetric->externReserved,
           eigrpExtraTopologyMetric->externProtocolId,
           eigrpExtraTopologyMetric->flags);
}


//--------------------------------------------------------------------------
// FUNCTION     : PrintTopologyTable().
//
// PURPOSE      : Printing the eigrp topology table.
//
// PARAMETERS   : node - node which is printing the table
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void PrintTopologyTable(Node* node)
{
    int count = 0;
    char timeStr[MAX_STRING_LENGTH] = {0};
    RoutingEigrp* eigrp = (RoutingEigrp*) NetworkIpGetRoutingProtocol(
                                               node,
                                               ROUTING_PROTOCOL_EIGRP);

    ListItem* temp = eigrp->topologyTable->first;
    ctoa(node->getNodeTime() / SECOND, timeStr);

    printf("-------------------------------------------------\n");
    printf(" Topology Table of node %u %s\n", node->nodeId, timeStr);
    printf("-------------------------------------------------\n");

    while (temp)
    {
        EigrpTopologyTableEntry* topologyTable =
            (EigrpTopologyTableEntry*)temp->data;

        char destAddress[MAX_STRING_LENGTH] = {0};
        char nextHopAddress[MAX_STRING_LENGTH] = {0};

        NodeAddress destination = (EigrpConvertCharArrayToInt(
            topologyTable->metric.destination) << 8);

        IO_ConvertIpAddressToString(destination, destAddress);
        IO_ConvertIpAddressToString(topologyTable->metric.nextHop,
            nextHopAddress);

        printf("dest address      : %s\n", destAddress);
        printf("nextHop(adv.)     : %s\n", nextHopAddress);
        printf("Enry type         : %d\n", topologyTable->entryType);
        if (topologyTable->metric.delay == EIGRP_DESTINATION_INACCESSABLE)
        {
            if (topologyTable->outgoingInterface == EIGRP_NULL0)
            {
                printf("delay             : %s\n", "DUMMY-ENTRY");
            }
            else
            {
                printf("delay             : %s\n", "UNREACHABLE-ENTRY");
            }
        }
        else
        {
            printf("delay             : %u\n", topologyTable->metric.delay);
        }
        printf("bandwidth         : %u\n", topologyTable->metric.bandwidth);
        printf("reported distance : %f\n", topologyTable->reportedDistance);
        printf("feasible distance : %f\n", topologyTable->feasibleDistance);
        printf("isOptimum         : %u\n", topologyTable->isOptimum);
        printf("ipPrefix          : %u\n", topologyTable->metric.ipPrefix);

        if (topologyTable->outgoingInterface == EIGRP_NULL0)
        {
            printf("outInterface      : %s\n",  "NULL-0");
        }
        else
        {
             printf("outInterface      : %u\n",
                 topologyTable->outgoingInterface);
        }

        printf("Is default route  : %d\n", topologyTable->isDefaultRoute);

        if (topologyTable->eigrpExtraTopologyMetric != NULL)
        {
            EigrpPrintExternRouteInfo(topologyTable);
        }

        printf("Is summary route  : %u\n", topologyTable->isSummary);
        printf("Is defined EIGRP  : %u\n",
            topologyTable->isDefinedInEigrpProcess);

        temp = temp->next;
        count++;
        printf("-------------------------------------------------\n");
    } // while (temp)
    printf("count == %u\n", count);
    printf("-------------------------------------------------\n");
}


//--------------------------------------------------------------------------
// FUNCTION     : PrintRoutingTable().
//
// PURPOSE      : Printing the eigrp routing table.
//
// PARAMETERS   : node - node which is printing the table
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void PrintRoutingTable(Node* node)
{
    int i = 0, j = 0;
    RoutingEigrp* eigrp = (RoutingEigrp*) NetworkIpGetRoutingProtocol(node,
                              ROUTING_PROTOCOL_EIGRP);

    EigrpRoutingTable* eigrpRoutingTable = (EigrpRoutingTable*)
        BUFFER_GetData(&eigrp->eigrpRoutingTable);

    int numTos = BUFFER_GetCurrentSize(&eigrp->eigrpRoutingTable) /
        sizeof(EigrpRoutingTable);

    for (i = 0; i < numTos; i++)
    {
        EigrpRoutingTableEntry* eigrpRoutingEntry = (EigrpRoutingTableEntry*)
            BUFFER_GetData(&eigrpRoutingTable[i].eigrpRoutingTableEntry);

        int numEntries = BUFFER_GetCurrentSize(&eigrpRoutingTable[i].
            eigrpRoutingTableEntry) / sizeof(EigrpRoutingTableEntry);

        printf("------------------------------------------------------"
               "---------------------\n"
               " Routing Table of node %u\n"
               "------------------------------------------------------"
               "---------------------\n"
               "%16s %10s %12s %9s %16s %5s %16s\n"
               "------------------------------------------------------"
               "---------------------\n",
               node->nodeId,
               "Destination-Addr",
               "Prop-Delay",
               "InvBandwidth",
               "Interface",
               "NextHop-Address",
               "Prefix",
               "LastUpdateTime");

        for (j = 0; j < numEntries; j++)
        {
            EigrpTopologyTableEntry* feasibleRoute =
                ((EigrpTopologyTableEntry*)
                    eigrpRoutingEntry[j].feasibleRoute->data);

            char clockstr[MAX_STRING_LENGTH] = {0};
            char destinationAddr[MAX_STRING_LENGTH] = {0};
            char nextHopAddr[MAX_STRING_LENGTH] = {0};
            char interfaceStr[MAX_STRING_LENGTH] = {0};

            NodeAddress destination = EigrpConvertCharArrayToInt(
                feasibleRoute->metric.destination) << 8;

            IO_ConvertIpAddressToString(destination, destinationAddr);
            IO_ConvertIpAddressToString(feasibleRoute->metric.nextHop,
                nextHopAddr);
            ctoa(eigrpRoutingEntry[j].lastUpdateTime, clockstr);

            if (eigrpRoutingEntry[j].outgoingInterface >= 0)
            {
                sprintf(interfaceStr, "%u",
                    eigrpRoutingEntry[j].outgoingInterface);
            }
            else
            {
                sprintf(interfaceStr, "%s", "NULL-0");
            }

            printf("%16s %10u %12u %9s %16s %6u %16s\n",
                   destinationAddr,
                   feasibleRoute->metric.delay,
                   feasibleRoute->metric.bandwidth,
                   interfaceStr,
                   nextHopAddr,
                   feasibleRoute->metric.ipPrefix,
                   clockstr);
        }
        printf("------------------------------------------------------"
               "---------------------\n");
    }
}


//--------------------------------------------------------------------------
// FUNCTION     : PrintEigrpHeader().
//
// PURPOSE      : Printing the eigrp message header.
//
// PARAMETERS   : node - node which is printing the header
//                eigrpHeader  - eigrp header.
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void PrintEigrpHeader(Node* node, EigrpHeader* eigrpHeader)
{
    printf("-------------------------------------------\n"
           "Node = %u\n"
           "-------------------------------------------\n"
           "version                 : %u\n"
           "opcode                  : %u\n"
           "checksum                : %u\n"
           "sequence number         : %u\n"
           "autonomous system       : %u\n"
           "-------------------------------------------\n",
           node->nodeId,
           eigrpHeader->version,
           eigrpHeader->opcode,
           eigrpHeader->checksum,
           eigrpHeader->sequenceNumber,
           eigrpHeader->asNum);
}


//--------------------------------------------------------------------------
// FUNCTION     : PrintAnyMessage().
//
// PURPOSE      : Printing the update/query/reply packet.
//
// PARAMETERS   : node - node which is printing the header
//                msg  - the message which is sent/received by the node
//                numEntries - number of entries in the packet
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : Message header TLV etc. are removed before printing
//--------------------------------------------------------------------------
static
void PrintAnyMessage(Node* node, char* msg, int numEntries, const char* type)
{
    int i = 0;
    EigrpUpdateMessage* updateMsg = (EigrpUpdateMessage*) msg;
    char timeStr[MAX_STRING_LENGTH] = {0};
    ctoa(node->getNodeTime(), timeStr);

    printf("-----------------------------------------------------------\n"
           "%s MESSAGE  node : %u %s\n"
           "-----------------------------------------------------------\n",
           type,
           node->nodeId,
           timeStr);

    for (i = 0; i < numEntries; i++)
    {
        NodeAddress destination = EigrpConvertCharArrayToInt(
            updateMsg[i].metric.destination);
        char destAddress[MAX_STRING_LENGTH] = {0};
        char nextHopAddr[MAX_STRING_LENGTH] = {0};

        IO_ConvertIpAddressToString(destination, destAddress);
        IO_ConvertIpAddressToString(updateMsg[i].metric.nextHop,
            nextHopAddr);

        printf("Dest Address : %s\n"
               "Delay        : %u\n"
               "bandWidth    : %u\n"
               "IpPrefix     : %u\n"
               "Next hop     : %s\n"
               "------------------------------\n",
               destAddress,
               updateMsg[i].metric.delay,
               updateMsg[i].metric.bandwidth,
               updateMsg[i].metric.ipPrefix,
               nextHopAddr);
    }
    printf("\n\n");
}


//--------------------------------------------------------------------------
// FUNCTION     : PrintUpdateMessage().
//
// PURPOSE      : Printing the update packet.
//
// PARAMETERS   : node - node which is printing the header
//                msg  - the message which is sent/received by the node
//                numEntries - number of entries in the packet
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : Message header TLV etc. are removed before printing
//--------------------------------------------------------------------------
static
void PrintUpdateMessage(Node* node, char* msg, int numEntries)
{
    PrintAnyMessage(node, msg, numEntries, "UPDATE");
}


//--------------------------------------------------------------------------
// FUNCTION     : PrintReplyMessage().
//
// PURPOSE      : Printing the eigrp reply.
//
// PARAMETERS   : node - node which is printing the header
//                msg  - the message which is sent/received by the node
//                numEntries - number of entries in the packet
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : Message header TLV etc. are removed before printing
//--------------------------------------------------------------------------
static
void PrintReplyMessage(Node* node, char* msg, int numEntries)
{
    PrintAnyMessage(node, msg, numEntries, "REPLY");
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpLPM()
//
// PURPOSE      : It is an utility function used in Longest Prefix Match
//
// PARAMETERS   : refAddress - address to be matched
//                tableAddress - address in the routing table with which
//                "refAddress" is tobe matched.
//                prefixlen - length of the prefix to be matched.
//
// RETURN VALUE : length of the prefix if matched or -1 otherwise.
//--------------------------------------------------------------------------
static
int EigrpLPM(unsigned refAddress, unsigned int tableAddress, int prefixlen)
{
    NodeAddress M_refAddress = refAddress >> (32 - prefixlen);
    NodeAddress M_tableAddress = tableAddress >> (32 - prefixlen);

    if (M_refAddress == M_tableAddress)
    {
        return prefixlen;
    }
    return -1;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpInitFilterListTable()
//
// PURPOSE      : Initializing Eigrp filter list table
//
// PARAMETERS   : eigrp - pointer to EIGRP data
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpInitFilterListTable(RoutingEigrp* eigrp)
{
    // eigrp->eigrpFilterListTable.numList = 0;
    // eigrp->eigrpFilterListTable.first   = NULL;
    // eigrp->eigrpFilterListTable.last    = NULL;
    memset(&(eigrp->eigrpFilterListTable), 0, sizeof(EigrpFilterListTable));
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpAddNodeToFilterTable()
//
// PURPOSE      : Initializing Eigrp filter list table
//
// PARAMETERS   : filterTable - pointer to filter Table
//                filterList - filter list entry to be added
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpAddNodeToFilterTable(
    EigrpFilterListTable* filterTable,
    EigrpFilterList* filterList)
{
    ERROR_Assert((filterTable && filterList),
        "Either filterTable or filterList or both NULL");

    if (filterTable->first == NULL)
    {
        filterTable->first = filterList;
        filterTable->last = filterTable->first;
        filterTable->first->prev = NULL;
        filterTable->first->next = NULL;
    }
    else
    {
        filterList->prev = filterTable->last;
        filterTable->last->next = filterList;
        filterTable->last = filterList;
        filterTable->last->next = NULL;
    }
    filterTable->numList++;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpAddToFilterList()
//
// PURPOSE      : Initializing Eigrp filter list table
//
// PARAMETERS   : filterTable - pointer to filter Table
//                eigrpAccessListType - filter list entry to be added
//                ACLparameter - The ACL parameter to be added
//                aclInterface - interface type
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpAddToFilterList(
    EigrpFilterListTable* filterTable,
    EigrpAccessListType eigrpAccessListType,
    char* ACLparameter,
    AccessListInterfaceType aclInterface,
    int interfaceIndex)
{
    EigrpFilterList* eigrpFilterList = (EigrpFilterList*)
        MEM_malloc(sizeof(EigrpFilterList));

    eigrpFilterList->type = eigrpAccessListType;
    eigrpFilterList->aclInterface = aclInterface;
    eigrpFilterList->interfaceIndex = interfaceIndex;

    if (eigrpFilterList->type == EIGRP_ACCESS_LIST_NUMBERED)
    {
        eigrpFilterList->ACLnumber = atoi(ACLparameter);
    }
    else if (eigrpFilterList->type == EIGRP_ACCESS_LIST_NAMED)
    {
        unsigned int stringLen = (unsigned int)strlen(ACLparameter);

        eigrpFilterList->ACLname = (char*) MEM_malloc(stringLen + 1);
        memset(eigrpFilterList->ACLname, 0, (stringLen + 1));
        strcpy(eigrpFilterList->ACLname, ACLparameter);
    }

    eigrpFilterList->prev = NULL;
    eigrpFilterList->next = NULL;

    // Add the node in the filterlist table
    EigrpAddNodeToFilterTable(filterTable, eigrpFilterList);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpReturnACLlist()
//
// PURPOSE      : Returning ACL attached with a given IP address. Provided
//                Access List Interface Type and interface index.
//
// PARAMETERS   : filterTable - pointer to eigrp filter table.
//                ipAddress - The ip address to be searched in ACL.
//                aclInterface - The Access List Interface Type
//                interfaceIndex - the interface index
//
// RETURN VALUE : pointer to access list attached with the "ipAddress"
//--------------------------------------------------------------------------
static
AccessList* EigrpReturnACLlist(
    Node* node,
    EigrpFilterListTable* filterTable,
    NodeAddress ipAddress,
    AccessListInterfaceType aclInterface,
    int interfaceIndex)
{
    EigrpFilterList* tempList = filterTable->first;

    while (tempList)
    {
        AccessListPointer* aclPtr = NULL;

        switch (aclInterface)
        {
            case ACCESS_LIST_IN:
            {
                // Search ACL of incoming Interface
                aclPtr = GET_IP_LAYER(node)->
                    interfaceInfo[interfaceIndex]->accessListInPointer;
                break;
            } // end case ACCESS_LIST_IN:

            case ACCESS_LIST_OUT:
            {
                 // Search ACL of outgoing Interface
                aclPtr = GET_IP_LAYER(node)->
                    interfaceInfo[interfaceIndex]->accessListOutPointer;
                break;
            } // end case ACCESS_LIST_OUT:

            default:
            {
                ERROR_Assert(FALSE, "Unknown Access"
                    "ListInterfaceType");
                break;
            }
        } // end switch (aclInterface)

        // search in per interface Access-list
        while (aclPtr)
        {
            AccessList* acl = *(aclPtr->accessList);
            EigrpAccessListType eigrpAccessListType = tempList->type;

            switch (eigrpAccessListType)
            {
                case EIGRP_ACCESS_LIST_NUMBERED:
                {
                    if (acl->accessListNumber == (signed)tempList->ACLnumber)
                    {
                        AccessList* temp = NULL;

                        for (temp = acl; temp != NULL; temp = temp->next)
                        {
                            // Match found return ACL pointer
                            if ((temp->srcAddr & (~(temp->srcMaskAddr))) ==
                                (ipAddress & (~(temp->srcMaskAddr))))
                            {
                                return temp;
                            }
                        } // end for ()
                    }
                    // Match not found search next ACL
                    break;
                } // end case EIGRP_ACCESS_LIST_NUMBERED:

                case EIGRP_ACCESS_LIST_NAMED:
                {
                    if (acl->accessListName != NULL)
                    {
                        if (strcmp(acl->accessListName,
                                tempList->ACLname) == 0)
                        {
                            // Match found return ACL pointer
                            AccessList* temp = NULL;
                            for (temp = acl; temp != NULL; temp = temp->next)
                            {
                                // Match found return ACL pointer
                                if ((temp->srcAddr & (~(temp->srcMaskAddr)))
                                    == (ipAddress & (~(temp->srcMaskAddr))))
                                {
                                    return temp;
                                }
                            } // end for ()
                        }
                    }
                    // Match not found search next ACL
                    break;
                } // end case EIGRP_ACCESS_LIST_NAMED:

                default:
                {
                    ERROR_Assert(FALSE, "Unknown Access"
                         "ListInterfaceType");
                    break;
                }
            } // end switch (aclInterface)
            aclPtr = aclPtr->next;
        } // end  while (aclPtr)
        tempList = tempList->next;
    } //  while (temp)
    // No matching ACL found
    return NULL;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpReturnACLPermission()
//
// PURPOSE      : Returning ACL permission attached with a given IP address.
//                Provided Access List Interface Type and interface index.
//
// PARAMETERS   : filterTable - pointer to eigrp filter table.
//                ipAddress - The ip address to be searched in ACL.
//                aclInterface - The Access List Interface Type
//                interfaceIndex - the interface index
//
// RETURN VALUE : pointer to access list permission attached with
//                the "ipAddress"
//--------------------------------------------------------------------------
AccessListFilterType EigrpReturnACLPermission(
    Node* node,
    EigrpFilterListTable* filterTable,
    NodeAddress ipAddress,
    AccessListInterfaceType aclInterface,
    int interfaceIndex)
{
    AccessList* acl = NULL;

    // Return ACCESS_LIST_PERMIT if
    // no distribute-list command is given.
    if (filterTable->numList == 0)
    {
        return ACCESS_LIST_PERMIT;
    }

    // search for the ACL attached with the
    // given ip Address (if any)
    acl = EigrpReturnACLlist(
              node,
              filterTable,
              ipAddress,
              aclInterface,
              interfaceIndex);

    // if ACL found return ACL permission.
    if (acl != NULL)
    {
        return acl->filterType;
    }

    // else if aclList == NILL (i.e if ACL not found)
    return ACCESS_LIST_PERMIT;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpReturnAclType();
//
// PURPOSE      : determining whether a <ACL>-string is a number or a name
//
// PARAMETRES   : aclType - string read from configuration file.
//
// RETURN VALUE : Eigrp Access List Type
//--------------------------------------------------------------------------
static
EigrpAccessListType EigrpReturnAclType(char* aclType)
{
    if (isdigit((unsigned) *aclType))
    {
        return EIGRP_ACCESS_LIST_NUMBERED;
    }
    else if (isalpha((unsigned) *aclType))
    {
        return EIGRP_ACCESS_LIST_NAMED;
    }
    else
    {
        ERROR_ReportError("Invalid name in <ACL>");
    }
    return EIGRP_ACCESS_LIST_NONE;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpReturnAclInterfaceType();
//
// PURPOSE      : Determining the interface type from the string. The given
//                string is read from configuration file.
//
// PARAMETRES   : interfaceType - string read from configuration file.
//
// RETURN VALUE : Access List Interface Type
//--------------------------------------------------------------------------
static
AccessListInterfaceType EigrpReturnAclInterfaceType(char* interfaceType)
{
    if (interfaceType != NULL)
    {
        if (strcmp(interfaceType, "IN") == 0)
        {
            return ACCESS_LIST_IN;
        }
        else if (strcmp(interfaceType, "OUT") == 0)
        {
            return ACCESS_LIST_OUT;
        }
        else
        {
            // Do Nothing
        }
    }
    ERROR_ReportError("Invalid Interface string");
    return ACCESS_LIST_IN;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpGetInterfaceIndexFromName()
//
// PURPOSE      : Extracting interface index from the interface name.
//
// PARAMETRES   : node - The node that is performing the above operation
//                name - Name of the interface.
//
// RETURN VALUE : interface index
//--------------------------------------------------------------------------
static
int EigrpGetInterfaceIndexFromName(Node* node, char* name)
{
    int i = 0;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        char* interfaceName = GET_IP_LAYER(node)->
           interfaceInfo[i]->interfaceName;

        if (strcmp(name, interfaceName) == 0)
        {
            return i;
        }
    }
    return ANY_INTERFACE;
}


//--------------------------------------------------------------------------
// FUNCTION     : IsDigitString()
//
// PURPOSE      : Checking if a string contains digits only
//
// PARAMETERS   : the string to be parsed
//
// RETURN VALUE : TRUE if the string contains all digits or FALSE otherwise
//--------------------------------------------------------------------------
static
BOOL IsDigitString(char* string)
{
    int i = 0;
    while (string[i] != '\0')
    {
        if (!isdigit(string[i]))
        {
            return FALSE;
        }
        i++;
    }
    return TRUE;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpCheckAndReturnInterfaceType();
//
// PURPOSE      : Determining the interface index from the string. The given
//                string is read from configuration file.
//
// PARAMETRES   : node - node which is reading configuration string
//                interfaceStr - string read from configuration file.
//
// RETURN VALUE : Access List Interface Type
//--------------------------------------------------------------------------
static
int EigrpCheckAndReturnInterfaceType(Node* node, char* interfaceStr)
{
    if (IsDigitString(interfaceStr))
    {
        int interfaceIndex = atoi(interfaceStr);

        if ((interfaceIndex < 0) ||
            (interfaceIndex > node->numberInterfaces))
        {
            ERROR_ReportError("Invalid Interface Index Specified");
        }
        return interfaceIndex;
    }
    else // if (it is a named interface)
    {
        // Extract interface index from name
        return EigrpGetInterfaceIndexFromName(
                   node,
                   interfaceStr);
    }
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpPrintRouterFilterRead();
//
// PURPOSE      : Printing the filter list for debugging purpose.
//
// PARAMETRES   : node - node which is printing the filter list
//                eigrpFilterListTable - pointer to eigrp filter list table.
//
// RETURN VALUE : Access List Interface Type
//--------------------------------------------------------------------------
static
void EigrpPrintRouterFilterRead(
    Node* node,
    EigrpFilterListTable eigrpFilterListTable)
{
    EigrpFilterList* eigrpFilterList = NULL;
    printf("--------------------------------------\n"
           "FILTER-TABLE OF NODE %u (%d)\n"
           "--------------------------------------\n",
           node->nodeId,
           eigrpFilterListTable.numList);

    eigrpFilterList = eigrpFilterListTable.first;

    while (eigrpFilterList)
    {
        const char* filterListType = NULL;
        const char* interfaceType = NULL;
        int interfaceIndex = ANY_INTERFACE;
        char aclName[MAX_STRING_LENGTH] = {0};

        if (eigrpFilterList->type == EIGRP_ACCESS_LIST_NUMBERED)
        {
            filterListType = (char *)"EIGRP_ACCESS_LIST_NUMBERED";
            sprintf(aclName, "%4d", eigrpFilterList->ACLnumber);
        }
        else if (eigrpFilterList->type == EIGRP_ACCESS_LIST_NAMED)
        {
            filterListType = (char *)"EIGRP_ACCESS_LIST_NAMED";
            strcpy(aclName, eigrpFilterList->ACLname);
        }
        else
        {
            ERROR_Assert(FALSE, "Wrong ACL type");
        }

        if (eigrpFilterList->aclInterface == ACCESS_LIST_IN)
        {
            interfaceType = (char *)"IN";
        }
        else if (eigrpFilterList->aclInterface == ACCESS_LIST_OUT)
        {
            interfaceType = (char *)"OUT";
        }
        else
        {
            ERROR_Assert(FALSE, "Wrong interface type");
        }
        interfaceIndex = eigrpFilterList->interfaceIndex;

        printf("Access List type = %s\n"
               "Access List interface type %s\n"
               "Access List name/number = %s\n"
               "The Interface Index %d\n"
               "--------------------------------------\n",
               filterListType,
               interfaceType,
               aclName,
               interfaceIndex);

        eigrpFilterList = eigrpFilterList->next;
    } //  end while (eigrpFilterList)
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpSetExternalFieldsInTopologyTable()
//
// PURPOSE      : Building up external protocol metric fields in the
//                topology table.
//
// PARAMETERS   : topologyTable - pointer to eigrp topology entry,
//                originatingRouter - Address of originating router.
//                originitingAs - The originating AS.
//                arbitraryTag - used in route tagging.
//                externalProtocolMetric - external protocol identifier.
//                externProtocolId - External protocol Id
//                flags - Flag indicating default route or external route
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpSetExternalFieldsInTopologyTable(
    EigrpTopologyTableEntry* topologyTable,
    NodeAddress originatingRouter,
    unsigned int originitingAs,
    NodeAddress arbitraryTag,
    unsigned int externalProtocolMetric,
    unsigned char externProtocolId,
    unsigned char flags)
{
    EigrpExtraTopologyMetric* temp = NULL;
    if (topologyTable->eigrpExtraTopologyMetric != NULL)
    {
        MEM_free(topologyTable->eigrpExtraTopologyMetric);
        topologyTable->eigrpExtraTopologyMetric = NULL;
    }

    topologyTable->eigrpExtraTopologyMetric = (EigrpExtraTopologyMetric*)
        MEM_malloc(sizeof(EigrpExtraTopologyMetric));
    memset(topologyTable->eigrpExtraTopologyMetric, 0,
        sizeof(EigrpExtraTopologyMetric));
    temp = topologyTable->eigrpExtraTopologyMetric;

    temp->originatingRouter = originatingRouter;
    temp->originitingAs = originitingAs;
    temp->arbitraryTag = arbitraryTag;
    temp->externalProtocolMetric = externalProtocolMetric;
    temp->externProtocolId = externProtocolId;
    temp->flags = flags;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpBuildExternalRoute()
//
// PURPOSE      : Building External route metric
//
// PARAMETERS   : topologyTable - pointer to topology table.
//                eigrpMetric - pointer to eigrp metric
//                externRoute - pointer to EigrpExternMetric structure.
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpBuildExternalRoute(
    EigrpTopologyTableEntry* topologyTable,
    EigrpMetric* eigrpMetric,
    EigrpExternMetric* externRoute)
{
    EigrpExtraTopologyMetric* externMetric  = NULL;
    ERROR_Assert(topologyTable, "Topology Table cannot be NULL");
    ERROR_Assert(externRoute, "pointer to externRoute cannot be NULL");

    externMetric = topologyTable->eigrpExtraTopologyMetric;

    // 1. Next hop Address.
    externRoute->nextHop = eigrpMetric->nextHop;
    // 2. Address of originating router.
    externRoute->originatingRouter = externMetric->originatingRouter;

    // 3. The originating AS.
    externRoute->originitingAs = externMetric->originitingAs;

    // 4. used in route tagging.
    externRoute->arbitraryTag = externMetric->arbitraryTag;

    // 5. external protocol identifier.
    externRoute->externalProtocolMetric = externMetric->
        externalProtocolMetric;

    // 6. Reserved (not used)
    externRoute->externReserved = externMetric->externReserved;

    // 7. External protocol Id
    externRoute->externProtocolId = externMetric->externProtocolId;

    // 8. Flag that distingushes default and external route.
    externRoute->flags = externMetric->flags;

    // 9. delay in tens of Micro Second.
    externRoute->delay = eigrpMetric->delay;

    // 10. Inverse Bandwidth
    externRoute->bandwidth = eigrpMetric->bandwidth;

    // 11. MTU
    memcpy(externRoute->mtu, eigrpMetric->mtu, 3);

    // 12. Hop Count
    externRoute->hopCount = eigrpMetric->hopCount;

    // 13. Reliability (not used)
    externRoute->reliability = eigrpMetric->reliability;

    // 14.Load
    externRoute->load = eigrpMetric->load;

    // 15.Reserved field (not used)
    externRoute->reserved = eigrpMetric->reserved;

    // 16. IP prefix
    externRoute->ipPrefix = eigrpMetric->ipPrefix;

    // 17. Three most significant bytes of IP address of destination
    memcpy(externRoute->destination, eigrpMetric->destination, 3);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpRouteThruDefaultRoute()
//
// PURPOSE      : to route the packet useing "Default Candidate Route" or
//                "Default Route".
// PARAMETERS   : node - node which is routing the packet
//                msg - the data packet
//                eigrp - pointer to eigrp data
//                destAddr - the destination node of this data packet
//                previouHopAddress - notUsed
//                PacketWasRouted - variable that indicates to IP that it
//                                  no longer needs to act on this
//                                  data packet
//
//-------------------------------------------------------------------------
static
void EigrpRouteThruDefaultRoute(
    Node* node,
    Message* msg,
    RoutingEigrp* eigrp,
    NodeAddress destAddr,
    NodeAddress previouHopAddress,
    BOOL* packetWasRouted)
{
    // If route does not matches any destination in the routing table
    // then it is the candidate for default/ default-candidate route.
    // First try to route via default-candidate route.
    // If not routed try to route thru default route.
    int i = 0;
    int interfaceIndex = ANY_INTERFACE;
    EigrpTopologyTableEntry* defaultRoutePtr = NULL;

    EigrpRoutingTable* eigrpRoutingTable = (EigrpRoutingTable*)
        BUFFER_GetData(&eigrp->eigrpRoutingTable);

    EigrpRoutingTableEntry* eigrpRoutingTableEntry =
        (EigrpRoutingTableEntry*) BUFFER_GetData(
             &(eigrpRoutingTable[0].eigrpRoutingTableEntry));

    int numEntry = BUFFER_GetCurrentSize(
        &(eigrpRoutingTable[0].eigrpRoutingTableEntry)) /
            sizeof(EigrpRoutingTableEntry);

    for (i = 0; i < numEntry; i++)
    {
        EigrpTopologyTableEntry* topologyTable = (EigrpTopologyTableEntry*)
            (eigrpRoutingTableEntry[i].feasibleRoute->data);

        unsigned int delay = topologyTable->metric.delay;
        NodeAddress nextHopAddress = topologyTable->metric.nextHop;
        BOOL isDefaultRoute = topologyTable->isDefaultRoute;

        if ((delay == EIGRP_DESTINATION_INACCESSABLE) ||
            (previouHopAddress == nextHopAddress) ||
            (isDefaultRoute == FALSE))
        {
            NodeAddress destAddress = EigrpConvertCharArrayToInt(
                topologyTable->metric.destination) << 8;
            int prefixLen = topologyTable->metric.ipPrefix;

            // Remember the default route. This will be needed
            // if routing thru default candidate route is failed,
            if ((destAddress | prefixLen) == 0)
            {
                defaultRoutePtr = topologyTable;
            }
            continue;
        }

        if (nextHopAddress == 0)
        {
            nextHopAddress = destAddr;
        }

        NetworkIpSendPacketToMacLayer(
            node,
            msg,
            interfaceIndex,
            nextHopAddress);

        *packetWasRouted = TRUE;
        return;
    } // end for (i = 0; i < numEntry; i++);

    if (defaultRoutePtr != NULL)
    {
        NetworkIpSendPacketToMacLayer(
            node,
            msg,
            defaultRoutePtr->outgoingInterface,
            ANY_DEST);

        *packetWasRouted = TRUE;
        return;
    }
    *packetWasRouted = FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION     : FindInterfaceIndexAndNextHop()
//
// PURPOSE      : Finding outgoing interface index and next hop from EIGRP
//                routing table.
//
// PARAMETERS   : eigrp - pointer to eigrp data structure
//                refAddress - address to be found in the routing table
//                previouHopAddress - the previous hop address
//                nextHopFound - the next hop address found in the
//                               routing table
//
// RETURN VALUE : outgoing interface index, nextHop address
//--------------------------------------------------------------------------
static
int FindInterfaceIndexAndNextHop(
    RoutingEigrp* eigrp,
    NodeAddress refAddress,
    NodeAddress previouHopAddress,
    NodeAddress* nextHopFound)
{
    int i = 0;
    int maxPrefixMatch = 0;
    int interfaceIndex = ANY_INTERFACE;
    EigrpRoutingTable* eigrpRoutingTable = (EigrpRoutingTable*)
        BUFFER_GetData(&eigrp->eigrpRoutingTable);

    EigrpRoutingTableEntry* eigrpRoutingTableEntry =
        (EigrpRoutingTableEntry*) BUFFER_GetData(
             &(eigrpRoutingTable[0].eigrpRoutingTableEntry));

    int numEntry = BUFFER_GetCurrentSize(
        &(eigrpRoutingTable[0].eigrpRoutingTableEntry)) /
            sizeof(EigrpRoutingTableEntry);

    *nextHopFound = ANY_DEST;

    for (i = 0; i < numEntry; i++)
    {
        EigrpTopologyTableEntry* topologyTable = (EigrpTopologyTableEntry*)
            (eigrpRoutingTableEntry[i].feasibleRoute->data);

        unsigned int delay = topologyTable->metric.delay;
        NodeAddress nextHopAddress = topologyTable->metric.nextHop;

        NodeAddress destAddress = EigrpConvertCharArrayToInt(
                                      topologyTable->metric.destination);

        int correspondingInterfaceIndex = topologyTable->outgoingInterface;
        int prefixLen = topologyTable->metric.ipPrefix;

        if ((delay == EIGRP_DESTINATION_INACCESSABLE) ||
          /*Commented during ARP Task*/
            /*(previouHopAddress == nextHopAddress)     ||*/
            (prefixLen == 0))
        {
            continue;
        }

        if (maxPrefixMatch <
            EigrpLPM(refAddress, (destAddress << 8), prefixLen))
        {
            maxPrefixMatch = prefixLen;
            interfaceIndex = correspondingInterfaceIndex;
            *nextHopFound = nextHopAddress;
        }
    } // end for (i = 0; i < numEntry; i++)
    return interfaceIndex;
}


//-------------------------------------------------------------------------
// FUNCTION     : EigrpRoutePacket()
//
// PURPOSE      : Snoop at the eigrp reply packet and updating its metric
//                if the packet is the reply packet.
//
// PARAMETERS   : node - node which is routing the packet
//                msg - the data packet
//                eigrp - pointer to eigrp data
//                destAddr - the destination node of this data packet
//                previouHopAddress - notUsed
//                PacketWasRouted - variable that indicates to IP that it
//                                  no longer needs to act on this
//                                  data packet
//                ipProtocolType - passed if needed for debugging
//
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void EigrpRoutePacket(
    Node* node,
    Message* msg,
    RoutingEigrp* eigrp,
    NodeAddress destAddr,
    NodeAddress previouHopAddress,
    BOOL* packetWasRouted,
    int ipProtocolType)
{
    int interfaceIndex = ANY_INTERFACE;
    NodeAddress nextHopAddress = ANY_DEST;

    if (NetworkIpIsMyIP(node, destAddr))
    {
        // Just sniff the packet
        // but do not route the packet
        *packetWasRouted = FALSE;
        return;
    }

    interfaceIndex = FindInterfaceIndexAndNextHop(
                         eigrp,
                         destAddr,
                         previouHopAddress,
                         &nextHopAddress);

    if (DEBUG_EIGRP_ROUTING)
    {
        int count = 0;
        #define NODE_ID 23
        char nextHop[MAX_STRING_LENGTH] = {0};
        char prevHop[MAX_STRING_LENGTH] = {0};
        char destAddress[MAX_STRING_LENGTH] = {0};
        char time[MAX_STRING_LENGTH] = {0};

        ctoa(node->getNodeTime() / SECOND, time);
        IO_ConvertIpAddressToString(previouHopAddress, prevHop);
        IO_ConvertIpAddressToString(nextHopAddress, nextHop);
        IO_ConvertIpAddressToString(destAddr, destAddress);

        // if (node->nodeId == NODE_ID)
        if ((node->nodeId == NODE_ID) && (ipProtocolType == IPPROTO_UDP))
        {
            printf("##node %2d next-hop == %16s index = %2d "
                   "previousHop = %16s pktNo = %d t = %s "
                   "dest = %16s %u\n",
                   node->nodeId,
                   nextHop,
                   interfaceIndex,
                   prevHop,
                   ++count,
                   time,
                   destAddress,
                   destAddr);
            printf("%d, ", node->nodeId);

            if (interfaceIndex == ANY_INTERFACE)
            {
                PrintRoutingTable(node);
                NetworkPrintForwardingTable(node);
                PrintTopologyTable(node);
            }
            count++;
        }
    }

    if (interfaceIndex != ANY_INTERFACE)
    {
        if (nextHopAddress == 0)
        {
            nextHopAddress = destAddr;
        }

        NetworkIpSendPacketToMacLayer(
            node,
            msg,
            interfaceIndex,
            nextHopAddress);

        *packetWasRouted = TRUE;
        return;
    }

    EigrpRouteThruDefaultRoute(
        node,
        msg,
        eigrp,
        destAddr,
        previouHopAddress,
        packetWasRouted);
}


//--------------------------------------------------------------------------
// FUNCTION     : IsInADifferentMajorNetwork()
//
// PURPOSE      : Check if an interface is attached with a different major
//                network; with respect to "addressToCheck"
//
// PARAMETERS   : node - node which is performing the above operation
//                addressToCheck - the address to check
//                interfaceIndex - interface index
//
// RETURN VALUE : Returns true if attached to a different major network
//--------------------------------------------------------------------------
static
BOOL IsInADifferentMajorNetwork(
    Node* node,
    NodeAddress addressToCheck,
    int ipPrefixIn,
    int interfaceIndex)
{
    int i = 0;
    unsigned int subnetMask[3] = {0xFF000000, 0xFFFF0000, 0xFFFFFF00};
    BOOL matched = FALSE;
    // Get interface network address
    NodeAddress interfaceNetworkAddress =
        NetworkIpGetInterfaceNetworkAddress(node, interfaceIndex);

    for (i = 0; i < 3; i++)
    {
        if ((subnetMask[2 - i] & addressToCheck) ==
            (subnetMask[2 - i] & interfaceNetworkAddress))
        {
            matched = TRUE;
        }
    }

    // Debug statement.
    if (EIGRP_DEBUG_SUBNETS)
    {
        if (!matched)
        {
            char addr[MAX_STRING_LENGTH] = {0};
            IO_ConvertIpAddressToString(addressToCheck, addr);
            printf("\t#node %d\n\t%s and", node->nodeId, addr);
            IO_ConvertIpAddressToString(interfaceNetworkAddress, addr);
            printf("%s is in DIFFERENT network\n", addr);
        }
        else
        {
            char addr[MAX_STRING_LENGTH] = {0};
            IO_ConvertIpAddressToString(addressToCheck, addr);
            printf("\t#node %d\n\t%s and", node->nodeId, addr);
            IO_ConvertIpAddressToString(interfaceNetworkAddress, addr);
            printf("%s is in SAME network\n", addr);
        }
    }
    return !matched;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpRouteSummarizationProssesResult()
//
// PURPOSE      : Checking route summery bit.
//
// PARAMETERS   : eigrp - pointer to eigrp data structre
//                topologyTable - pointer to topology table
//
// RETURN VALUE : status of route summarization flag.
//--------------------------------------------------------------------------
static
BOOL EigrpRouteSummarizationProssesResult(
    RoutingEigrp* eigrp,
    EigrpTopologyTableEntry* topologyTable)
{
    if (eigrp->isSummaryOn == TRUE)
    {
        if (topologyTable->isSummary)
        {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}


//-------------------------------------------------------------------------
// FUNCTION     : IsSuperNetOfABigNetwork()
//
// PURPOSE      : Determining whether an advertised network is a supernet of
//                a already existing network
//
// PARAMETERS   : eigrp - pointer to eigrp data structure
//                destination - the IP address of advertised network
//                incomingPrefix - prefix length of the advertised network
//
// RETURN VALUE : Returns TRUE if "destination" is a supernet.
//--------------------------------------------------------------------------
static
BOOL IsSuperNetOfABigNetwork(
    RoutingEigrp* eigrp,
    NodeAddress destination,
    int incomingPrefix)
{
    int i = 0;
    int j = 0;
    int maxPrefixMatch = 0;

    EigrpRoutingTable* eigrpRoutingTable = (EigrpRoutingTable*)
        BUFFER_GetData(&eigrp->eigrpRoutingTable);

    int numRouteTable = BUFFER_GetCurrentSize(
        &eigrp->eigrpRoutingTable) /
            sizeof(EigrpRoutingTable);

    for (j = 0; j < numRouteTable; j++)
    {
        EigrpRoutingTableEntry* routingTable =
            (EigrpRoutingTableEntry*) BUFFER_GetData(
                &(eigrpRoutingTable[j].eigrpRoutingTableEntry));

        int numEntry = BUFFER_GetCurrentSize(
               &(eigrpRoutingTable[j].eigrpRoutingTableEntry)) /
                    sizeof(EigrpRoutingTableEntry);

        for (i = 0; i < numEntry; i++)
        {
            EigrpTopologyTableEntry* topologyTable =
                (EigrpTopologyTableEntry*)
                    (routingTable[i].feasibleRoute->data);

            NodeAddress destAddress = EigrpConvertCharArrayToInt(
                                          topologyTable->metric.destination);

            int prefixLen = topologyTable->metric.ipPrefix;
            int networkPrefixLen = MIN(prefixLen, incomingPrefix);

            if (topologyTable->isDefinedInEigrpProcess == FALSE)
            {
                continue;
            }

            if (EigrpLPM(destination, destAddress << 8, networkPrefixLen) != -1)
            {
                maxPrefixMatch = prefixLen;
                if (maxPrefixMatch > incomingPrefix)
                {
                    topologyTable->isSummary = FALSE;
                }
            }
        } // end for (i = 0; i < numEntry; i++)
    } // end for (j = 0; j < numRouteTable; j++)

    if (maxPrefixMatch >= incomingPrefix)
    {
        // Destination address is supernet.
        return TRUE;
    }
    else if (maxPrefixMatch == 0)
    {
        // matching network not found. The incoming
        // address is a new network.
        return TRUE;
    }
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpGetNumEntriesInPacket()
//
// PURPOSE      : Returning/calculating appropriate numbers of route in a
//                routing update packet.
//
// PARAMETERS   : initialSize - total size of the routes.
//                tlv - tlv type of the route
//
// RETURN VALUE : total number of routes
//--------------------------------------------------------------------------
static
int EigrpGetNumEntriesInPacket(int initialSize, EigrpTLVType tlv)
{
    if ((tlv == EIGRP_INTERNAL_ROUTE_TLV_TYPE) ||
        (tlv == EIGRP_SYSTEM_ROUTE_TLV_TYPE))
    {
        return initialSize / sizeof(EigrpUpdateMessage);
    }
    else if (tlv == EIGRP_EXTERN_ROUTE_TLV_TYPE)
    {
        return initialSize / sizeof(EigrpExternMetric);
    }

    ERROR_Assert(FALSE, "Not a relevant TLV type");
    return 0;
}


//-------------------------------------------------------------------------
// FUNCTION     : EigrpAddNeighbor()
//
// PARAMETERS   : pointer to eigrp neighbor table
//                neighborAddress - address of the neighbor to add
//                holdTime - hold time of the neighbor
//
// RETURN VALUE : pointer to eigrp neighbor added
// -----------------------------------------------------------------------
static
EigrpNeighborInfo* EigrpAddNeighbor(
    EigrpNeighborTable* neighborTable,
    NodeAddress neighborAddress,
    unsigned int holdTime)
{
    EigrpNeighborInfo* newInfo = (EigrpNeighborInfo*) MEM_malloc(
        sizeof(EigrpNeighborTable));

    // neighborAddr = 0
    // holdTime = 0
    // isHelloOrAnyOtherPacketReceived = 0
    // state = EIGRP_NEIGHBOR_OFF
    // next = NULL
    memset(newInfo, 0, sizeof(EigrpNeighborTable));

    newInfo->neighborAddr = neighborAddress;
    newInfo->neighborHoldTime = (holdTime * SECOND);

    if (neighborTable->first == NULL)
    {
        neighborTable->first = newInfo;
        neighborTable->last = newInfo;
    }
    else
    {
        neighborTable->last->next = newInfo;
        neighborTable->last = newInfo;
    }
    return newInfo;
}


//-------------------------------------------------------------------------
// FUNCTION     : EigrpGetNeighbor()
//
// PARAMETERS   : pointer to eigrp neighbor table
//                neighborAddress - address of the neighbor to find
//                holdTime - hold time of the neighbor
//
// RETURN VALUE : pointer to eigrp neighbor (if found) or NULL otherwise
// -----------------------------------------------------------------------
static
EigrpNeighborInfo* EigrpGetNeighbor(
    EigrpNeighborTable* neighborTable,
    NodeAddress neighborAddress)
{
    EigrpNeighborInfo* temp = neighborTable->first;
    while (temp)
    {
        if (temp->neighborAddr == neighborAddress)
        {
           return temp;
        }
        temp = temp->next;
    }
    return NULL;
}


//-------------------------------------------------------------------------
// FUNCTION     : EigrpUpdatePacketReceivedStatus
//
// PURPOSE      : Updating the variable "isHelloOrAnyOtherPacketReceived"
//                that should be set to true if al least one packet is
//                received from the "sourceAddress".
//
// PARAMETERS   : neighborTable - eigrp neighbor table
//                sourceAddr - source address whoes status is to
//                             be updated
//-------------------------------------------------------------------------
static
void EigrpUpdatePacketReceivedStatus(
    EigrpNeighborTable* neighborTable,
    NodeAddress sourceAddr)
{
    EigrpNeighborInfo* neighbor =
        EigrpGetNeighbor(neighborTable, sourceAddr);

    if (neighbor != NULL)
    {
        neighbor->isHelloOrAnyOtherPacketReceived = TRUE;
    }
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpIncSeqNum()
//
// PURPOSE      : Incrementing the sequence number
//
// PARAMETERS   : neighbor - pointer to EigrpNeighborTable structure
//
// RETURN VALUE : new sequence number
//--------------------------------------------------------------------------
static
unsigned short int EigrpIncSeqNum(EigrpNeighborTable* neighbor)
{
    if (neighbor->seqNo == USHRT_MAX)
    {
        neighbor->seqNo = EIGRP_MIN_SEQ_NUM;
    }
    else
    {
        neighbor->seqNo++;
    }
    return (unsigned short) neighbor->seqNo;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpGetFlagField()
//
// PURPOSE      : Returning "flags" field that is used in EigrpHeader
//
// PARAMETERS   : neighbor - pointer to EigrpNeighborTable structure
//
// RETURN VALUE : "flags" value
//--------------------------------------------------------------------------
static
unsigned int EigrpGetFlagField(EigrpNeighborTable* neighbor)
{
    if (neighbor->seqNo == 0)
    {
        return EIGRP_FLAG_INITALLIZATION_BIT_ON;
    }
    else
    {
        return EIGRP_FLAG_INITALLIZATION_BIT_OFF;
    }
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpAddDataToPacketBuffer()
//
// PURPOSE      : Adding data to a temporary packet buffer (byte wise)
//                for building up a protocol packet.
//
// PARAMETERS   : packetBuffer - pointer to the packet buffer where
//                               data to be added.
//                size - size of the data-chunk in bytes
//                dataPacket - pointer to data packet
//--------------------------------------------------------------------------
static
void EigrpAddDataToPacketBuffer(
    PacketBuffer* packetBuffer,
    int size,
    char* dataPacket)
{
    int i = 0;
    char* bufferSpace = NULL;
    ERROR_Assert(packetBuffer, "Packet buffer not allocated");
    BUFFER_AddHeaderToPacketBuffer(packetBuffer, size, &bufferSpace);
    ERROR_Assert(bufferSpace, "Buffer space not allocated");
    for (i = 0; i < size; i++)
    {
        bufferSpace[i] = dataPacket[i];
    }
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpCopyBufferIntoMessage()
//
// PURPOSE      : Coping content of the packet buffer in the message
//
// PARAMETERS   : node - node which is coping buffer in the message
//                buffer - pointer to packet buffer
//                msg - pointer to message
//
// RETURN VALUE : none
//
// ASSUMPTION   : none
//--------------------------------------------------------------------------
static
void EigrpCopyBufferIntoMessage(
    Node* node,
    PacketBuffer* buffer,
    Message** msg)
{
    *msg = MESSAGE_Alloc(
               node,
               NETWORK_LAYER,
               ROUTING_PROTOCOL_EIGRP,
               MSG_DEFAULT);

    MESSAGE_PacketAlloc(
        node,
        *msg,
        BUFFER_GetCurrentSize(buffer),
        TRACE_EIGRP);

    memcpy(MESSAGE_ReturnPacket((*msg)),
           BUFFER_GetData(buffer),
           BUFFER_GetCurrentSize(buffer));
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpAddHeaderToPacketBuffer()
//
// PURPOSE      : Adding Eigrp Header
//
// PARAMETERS   : node - node which is adding the header
//                msg - eigrp protocol packet
//                version - eigrp version number
//                flags - eigrp flags value
//                seqNum - eigrp packet sequence number
//                as - AS ID
//                eigrpTlvType - the eigrp TLV type
//                eigrpTlvLength - the eigrp TLV length
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpAddHeaderToPacketBuffer(
    PacketBuffer* buffer,
    unsigned char opcode,
    unsigned char version,
    unsigned int flags,
    unsigned short seqNum,
    unsigned short as)
{
    EigrpHeader eigrpHeader = {0};

    eigrpHeader.version = version;
    eigrpHeader.opcode = opcode;
    eigrpHeader.checksum = 0;
    eigrpHeader.sequenceNumber = seqNum;
    eigrpHeader.asNum = as;
    eigrpHeader.flags = flags;

    EigrpAddDataToPacketBuffer(
        buffer,
        sizeof(EigrpHeader),
        (char*) &eigrpHeader);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpAddRouteToAppropriatePacketBuffer()
//
// PURPOSE      : Adding External route to external-route packet buffer
//                an internal/system-route to system-route packet buffer
//
// PARAMETERS   : packetBufferExternRoute - external route packet buffer
//                packetBufferSystemRoute - system route packet buffer
//                route - route to be added
//                routeType - type of the route external, internal or
//                            system route.
//                numSystemRoute - number of system route in the buffer
//                numExternRoute - number of external route in the buffer
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpAddRouteToAppropriatePacketBuffer(
    PacketBuffer* packetBufferExternRoute,
    PacketBuffer* packetBufferSystemRoute,
    EigrpMetric* route,
    unsigned char routeType,
    unsigned int* numSystemRoute,
    unsigned int* numExternRoute)
{
    switch (routeType)
    {
        case EIGRP_INTERNAL_ROUTES:
        case EIGRP_SYSTEM_ROUTE:
        {
            // Add to internal route packet buffer
            EigrpAddDataToPacketBuffer(
                packetBufferSystemRoute,
                sizeof(EigrpMetric),
                (char*) route);

            (*numSystemRoute)++;
            break;
        }

        case EIGRP_EXTERIOR_ROUTE:
        {
            // Add to external route packet buffer
            EigrpAddDataToPacketBuffer(
                packetBufferExternRoute,
                sizeof(EigrpExternMetric),
                (char*) route);

            (*numExternRoute)++;
            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Unknown Route type");
            break;
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpAddHeaderAndSendPacketToMacLayer()
//
// PURPOSE      : send the message to the network layer
//
// PARAMETERS   : node - node which is sending the message
//                eigrp - the routing eigrp structure
//                systemRoute - pointer to packet buffer containing system
//                              route
//                numSystemRoute - number of system route in the buffer
//                externRoute - pointer to packet buffer containing external
//                              route
//                numExternRoute - number of external route in the buffer
//                interfaceIndex - the interface through message is to
//                                 be sent
//                addrType - specifies if the packet is unicast or
//                           multicast packet.
//
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
static
void EigrpAddHeaderAndSendPacketToMacLayer(
    Node* node,
    RoutingEigrp* eigrp,
    PacketBuffer* systemRoute,
    int numSystemRoute,
    PacketBuffer* externRoute,
    int numExternRoute,
    int interfaceIndex,
    unsigned char opcode,
    NodeAddress target)
{
    Message* msg = NULL;
    PacketBuffer* concatinatedBuffer = NULL;
    NodeAddress nextHop = ANY_DEST;
    unsigned int ttlValue = ONE_HOP_TTL;

    if (opcode != EIGRP_QUERY_PACKET)
    {
        if (numSystemRoute > 0)
        {
            // Add TLV header (for system routes)
            EigrpTlv eigrpTlv = {EIGRP_SYSTEM_ROUTE_TLV_TYPE,
                (unsigned short) (numSystemRoute * sizeof(EigrpMetric))};

            EigrpAddDataToPacketBuffer(
                systemRoute,
                sizeof(EigrpTlv),
                (char*) &eigrpTlv);

            concatinatedBuffer = systemRoute;
        }

        if (numExternRoute > 0)
        {
            // Add TLV header (for external routes)
            EigrpTlv eigrpTlv = {EIGRP_EXTERN_ROUTE_TLV_TYPE,
                (unsigned short) (numExternRoute *
                    sizeof(EigrpExternMetric))};

            EigrpAddDataToPacketBuffer(
                externRoute,
                sizeof(EigrpTlv),
                (char*) &eigrpTlv);

            concatinatedBuffer = externRoute;
        }

        if ((numSystemRoute > 0) && (numExternRoute > 0))
        {
            BUFFER_ConcatenatePacketBuffer(systemRoute, externRoute);
            concatinatedBuffer = externRoute;
        }
    }
    else // if (opcode == EIGRP_QUERY_PACKET)
    {
        // ASSUMPTION: Query message will always use
        //             "systemRoute" Buffer
        EigrpTlv eigrpTlv = {EIGRP_EXTERN_ROUTE_TLV_TYPE,
            (unsigned short) BUFFER_GetCurrentSize(systemRoute)};

        concatinatedBuffer = systemRoute;

        EigrpAddDataToPacketBuffer(
            systemRoute,
            sizeof(EigrpTlv),
            (char*) &eigrpTlv);
    }

    EigrpAddHeaderToPacketBuffer(
        concatinatedBuffer,
        opcode,
        (unsigned char) EIGRP_VERSION,
        EigrpGetFlagField(&(eigrp->neighborTable[interfaceIndex])),
        EigrpIncSeqNum(&(eigrp->neighborTable[interfaceIndex])),
        eigrp->asId);

    EigrpCopyBufferIntoMessage(node, concatinatedBuffer, &msg);

    if (target != EIGRP_ROUTER_MULTICAST_ADDRESS)
    {
        nextHop = target;
    }

    if ((opcode == EIGRP_REPLY_PACKET) || (opcode == EIGRP_REQUEST_PACKET))
    {
        int notUsed = ANY_INTERFACE;
        ttlValue = IPDEFTTL;

        NetworkGetInterfaceAndNextHopFromForwardingTable(
            node,
            target,
            &notUsed,
            &nextHop);

        if (nextHop == 0)
        {
            nextHop = target;
        }
    }

    // Send packet to mac layer
    NetworkIpSendRawMessageToMacLayer(
        node,
        msg,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        target,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_EIGRP,
        ttlValue,
        interfaceIndex,
        nextHop);
} // end EigrpAddHeaderAndPacketToMacLayer()


//--------------------------------------------------------------------------
// FUNCTION     : EigrpCheckBufferAndSend()
//
// PURPOSE      : Checking size of packet buffer and if buffer is full
//                then send the packet
//
// PARAMETERS   : node - node which is checking buffer.
//                eigrp - pointer to eigrp structure.
//                packetBufferSystemRoute - packet buffer for system route
//                numSystemRoute - number of system route.
//                packetBufferExternRoute - packet buffer for external route
//                numExternRoute - number of external route
//                interfaceIndex - interface index through which message is
//                                 to be sent.
//                statUpdate - stat variable to be updated.
//                numRoutes - total number of routes.
//                packetType - packet type
//                addrType - wants address type unicast or multicast
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpCheckBufferAndSend(
    Node* node,
    RoutingEigrp* eigrp,
    PacketBuffer* packetBufferSystemRoute,
    unsigned int* numSystemRoute,
    PacketBuffer* packetBufferExternRoute,
    unsigned int* numExternRoute,
    int interfaceIndex,
    unsigned int* statUpdate,
    unsigned int* numRoutes,
    unsigned char packetType,
    NodeAddress target)
{
    if (*numRoutes == (EIGRP_MAX_ROUTE_ADVERTISED - 1))
    {
        EigrpAddHeaderAndSendPacketToMacLayer(
            node,
            eigrp,
            packetBufferSystemRoute,
            *numSystemRoute,
            packetBufferExternRoute,
            *numExternRoute,
            interfaceIndex,
            packetType,
            target);

        (*statUpdate)++;
        *numRoutes = 0;
        *numSystemRoute = 0;
        *numExternRoute = 0;

        BUFFER_ClearPacketBufferData(packetBufferExternRoute);
        BUFFER_ClearPacketBufferData(packetBufferSystemRoute);
    }
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpAdvancePacketPtr()
//
// PURPOSE      : Advancing a packet pointer by a definite number of bytes
//
// PARAMETERS   : packet - pointer to packet pointer.
//                length - num bytes to be advanced
//
// RETURN VALUE : tlv type
//--------------------------------------------------------------------------
static
void EigrpAdvancePacketPtr(char** packet, int length)
{
    (*packet) = (*packet) + length;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpRemoveHeader()
//
// PURPOSE      : Removing EIGRP header and extract content of header,
//
// PARAMETERS   : packet - the packet whose header to be removed.
//                size - estimated size of the packet
//                header - pointer to header
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpRemoveHeader(char** packet, int size, EigrpHeader* header)
{
    if (size >= (signed) (sizeof(EigrpHeader)))
    {
        memcpy(header, *packet, sizeof(EigrpHeader));
    }
    EigrpAdvancePacketPtr(packet, sizeof(EigrpHeader));
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpPreprocessTLVType()
//
// PURPOSE      : Removing TLV header and extract TLV value size,
//                TLV type etc
//
// PARAMETERS   : packet - the packet to be processed.
//                size - estimated size of the packet
//                length - "length" field of TLV
//
// RETURN VALUE : tlv type
//--------------------------------------------------------------------------
static
EigrpTLVType EigrpPreprocessTLVType(char** packet, int size, int* length)
{
    EigrpTLVType eigrpTlvType = EIGRP_NULL_TLV_TYPE;

    // This is to fix an optimization error on Ubuntu-9.10 64-bit.
    EigrpTlv* eigrpTlv = (EigrpTlv*)MEM_malloc(sizeof(EigrpTlv));
    memset(eigrpTlv, 0, sizeof(EigrpTlv));

    if (size >= (sizeof(EigrpTlv)))
    {
        memcpy(eigrpTlv, *packet, sizeof(EigrpTlv));
    }
    EigrpAdvancePacketPtr(packet, sizeof(EigrpTlv));
    eigrpTlvType = eigrpTlv->type;
    *length = eigrpTlv->length;
    MEM_free(eigrpTlv);
    return eigrpTlvType;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpReadNumBytes()
//
// PURPOSE      : Reading a given number of bytes fro packet pointer,
//                and advance the packet pointer
//
// PARAMETERS   : packet - the packet to be processed.
//                size - estimated size of the packet
//                length - length of the byte read
//                retValue - the byte read from the packet
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpReadNumBytes(char** packet, int size, int length, char* retValue)
{
    int i = 0;
    if (size >= length)
    {
        for (i = 0; i < length; i++)
        {
            retValue[i] = (*packet)[i];
        }
    }
    EigrpAdvancePacketPtr(packet, length);
}


//--------------------------------------------------------------------------
// FUNCTION     : IsItAckPacket
//
// PURPOSE      : Determining if a packet is acknowledgement packet or not
//
// PARAMETERS   : msg - incoming message.
//
// RETURN VALUE : TRUE if message is Ack packet
//
// ASSUMPTION   : none (this function will be removed latter)
//--------------------------------------------------------------------------
static
BOOL IsItAckPacket(Message* msg)
{
    return MESSAGE_ReturnPacketSize(msg) == EIGRP_HEADER_SIZE;
}


//--------------------------------------------------------------------------
//
// FUNCTION     : EigrpGetInvBandwidth()
//
// PURPOSE      : getting the inverted bandwidth information
//
// PARAMETERS   : node - the node who's bandwidth is needed.
//                interfaceIndex - interface Index
//
// RETURN VALUE : inverted bandwidth
//
// ASSUMPTION   : Bandwidth read from interface is in from of bps unit.
//                To invert the bandwidth we use the equation
//                10000000 / bandwidth. Where bandwidth is in Kbps unit.
//
//--------------------------------------------------------------------------
unsigned int EigrpGetInvBandwidth(Node* node, int interfaceIndex)
{
    unsigned int invBandwidth = 0;
    Int64 bandwidth = NetworkIpGetBandwidth(node, interfaceIndex);

    if ((bandwidth > 0) && (bandwidth <= INT_MAX))
    {
        bandwidth = bandwidth / 1000;
        invBandwidth = (unsigned int)(10000000 / bandwidth);
    }
    else
    {
        char errBuf[MAX_STRING_LENGTH] = {0};
        sprintf(errBuf, "#node %u Bandwidth must be in the range"
            " GREATER THAN 0 and LESS THAN EQUAL TO 10000000 Bps\n"
            " Bandwith given is %" TYPES_64BITFMT "d.\n",
              node->nodeId, bandwidth);
        ERROR_ReportError(errBuf);
    }
    return invBandwidth;
}


//--------------------------------------------------------------------------
//
// FUNCTION     : EigrpGetPropDelay()
//
// PURPOSE      : getting the propagation delay information
//
// PARAMETERS   : node - the node who's bandwidth is needed.
//                interfaceIndex - interface Index
//
// RETURN VALUE : propagation delay
//
// ASSUMPTION   : Array is exactly 3-byte long.
//
//--------------------------------------------------------------------------
static
clocktype EigrpGetPropDelay(Node* node, int interfaceIndex)
{
    ERROR_Assert((interfaceIndex >= 0) &&
                 (interfaceIndex <= node->numberInterfaces),
                 "Invalid Interface Index !!!");

    clocktype propdelay = NetworkIpGetPropDelay(node, interfaceIndex);
    return propdelay / 10;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpCalculateMetric()
//
// PURPOSE      : Calculating the IGRP metric value.
//
// PARAMETERS   : routingInfo - IGRP vector metric structure
//                k1, k2, k3, k4, k5 - weight values of type of service
//
// RETURN VALUE : caluclated metric value (see the calculation in the
//                function defination)
//--------------------------------------------------------------------------
static
float EigrpCalculateMetric(EigrpMetric* routingInfo,
    int k1, int k2, int k3, int k4, int k5)
{
    unsigned char reliability = 0;
    unsigned int inverseBandwidth = routingInfo->bandwidth;

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

    metric = (float) ((k1 * inverseBandwidth
                      + k2 * inverseBandwidth / (256 - routingInfo->load)
                      + k3 * routingInfo->delay)
                      * reliability);

    return metric * 256;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpSearchTopologyTableForFeasibleEntry()
//
// PURPOSE      : finding a feasible successor for a given destination
//                address
//                in the topology table.
//
// PARAMETERS   : metric - metric carrying the destination address for
//                         which feasible successor has to be found.
//                eigrp - routing eigrp structure.
//
// RETURN VALUE : pointer to the feasible successor if any, or NULL
//                otherwise
//--------------------------------------------------------------------------
static
ListItem* EigrpSearchTopologyTableForFeasibleEntry(
    EigrpMetric metric,
    RoutingEigrp* eigrp)
{
    ListItem* temp = NULL;
    ListItem* feasibleSucc = NULL;
    BOOL assumedFeasibleDistanceFound = FALSE;

    // Search the topology table for feasible  successor with
    // least feasible distance (route re computation)
    //
    // ASSUMPTION: Topology table contains only feasible successors.
    //             We are trying to find best of feasible successors.

    temp = eigrp->topologyTable->first;
    while (temp)
    {
        NodeAddress destAddressInTopologyTable =
            EigrpConvertCharArrayToInt(((EigrpTopologyTableEntry*)
                temp->data)->metric.destination);

        NodeAddress destAddress = EigrpConvertCharArrayToInt(
            metric.destination);

        unsigned int delay = ((EigrpTopologyTableEntry*)
            temp->data)->metric.delay;

        if ((destAddressInTopologyTable == destAddress) &&
            (delay != EIGRP_DESTINATION_INACCESSABLE))
        {
            if (assumedFeasibleDistanceFound == FALSE)
            {
                feasibleSucc = temp;
                assumedFeasibleDistanceFound = TRUE;
            }
            else
            {
                float feasibleDistance = ((EigrpTopologyTableEntry*)
                    feasibleSucc->data)->feasibleDistance;

                float tempFeasibleDistance = ((EigrpTopologyTableEntry*)
                    temp->data)->feasibleDistance;

                if (tempFeasibleDistance < feasibleDistance)
                {
                    feasibleSucc = temp;
                }
            }
        }
        temp = temp->next;
    } // end while (temp)
    return feasibleSucc;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpUpdateRoutingTableEntryWithFeasibleSuccessor()
//
// PURPOSE      : Update routing table with a new feasible route entry.
//                The new feasible route entry is taken from topology
//                table entry.
//
// PARAMETERS   : node - Node which is updating the table with feasible
//                       route entry.
//                eigrp - Pointer to eigrp structure.
//                feasibleSucc - pointer to feasible successor
//                unreachableEntry - pointer to ex-feasible-successor that
//                                   has become unreachable now.
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpUpdateRoutingTableEntryWithFeasibleSuccessor(
    Node* node,
    RoutingEigrp* eigrp,
    ListItem* feasibleSucc,
    ListItem* unreachableEntry)
{
    // if control reaches here then either some route has been
    // modified or a new route has been added.
    int i = 0;
    int numTos = BUFFER_GetCurrentSize(&eigrp->eigrpRoutingTable) /
        sizeof(EigrpRoutingTable);

    EigrpRoutingTable* eigrpRoutingTable = (EigrpRoutingTable*)
        BUFFER_GetData(&eigrp->eigrpRoutingTable);

    for (i = 0; i < numTos; i++)
    {
        int j = 0;
        int numEntries = BUFFER_GetCurrentSize(&eigrpRoutingTable[i].
            eigrpRoutingTableEntry) / sizeof(EigrpRoutingTableEntry);

        EigrpRoutingTableEntry* eigrpRoutingTableEntry =
            (EigrpRoutingTableEntry*) BUFFER_GetData(
                 &eigrpRoutingTable[i].eigrpRoutingTableEntry);

        for (j = 0; j < numEntries; j++)
        {
            if (eigrpRoutingTableEntry[j].feasibleRoute == unreachableEntry)
            {
                // store the old entry in a temporary variable
                EigrpTopologyTableEntry* temp = (EigrpTopologyTableEntry*)
                    eigrpRoutingTableEntry[j].feasibleRoute->data;

                eigrpRoutingTableEntry[j].feasibleRoute = feasibleSucc;

                ((EigrpTopologyTableEntry*) eigrpRoutingTableEntry[j].
                    feasibleRoute->data)->isOptimum = TRUE;

                eigrpRoutingTableEntry[j].outgoingInterface =
                    ((EigrpTopologyTableEntry*) eigrpRoutingTableEntry[j].
                        feasibleRoute->data)->outgoingInterface;

                ERROR_Assert(eigrpRoutingTableEntry[j].outgoingInterface !=
                    -1, "Unknown interface index");

                eigrpRoutingTableEntry[j].numOfPacketSend = 0;
                eigrpRoutingTableEntry[j].lastUpdateTime = node->getNodeTime();

                // previous optimum route is unreachable now
                // and hence isOptimum = FALSE
                temp->isOptimum = FALSE;
            }
        } // end for (j = 0; j < numEntries; j++)
    } // end for (i = 0; i < numTos; i++)
} // end EigrpUpdateRoutingTableEntryWithFeasibleSuccessor()


//--------------------------------------------------------------------------
// FUNCTION     : EigrpAddRouteToRoutingTable()
//
// PURPOSE      : Adding a new route to Eigrp routing table.
//
// PARAMETERS   : node - Node which is adding the route to the
//                       routing table
//                eigrp - Pointer to eigrp structure
//                destAddress - Destination address to be added
//                interfaceIndex - interface index
//                newTopologyEntry - Pointer to the route info in the
//                                   topology table
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpAddRouteToRoutingTable(
    Node* node,
    RoutingEigrp* eigrp,
    NodeAddress destAddress,
    int interfaceIndex,
    ListItem* newTopologyEntry)
{
    int i = 0;
    int numTos = BUFFER_GetCurrentSize(&eigrp->eigrpRoutingTable) /
                     sizeof(EigrpRoutingTable);

    EigrpRoutingTable* eigrpRoutingTable = (EigrpRoutingTable*)
        BUFFER_GetData(&eigrp->eigrpRoutingTable);

    EigrpTopologyTableEntry* topologyPtr =
        (EigrpTopologyTableEntry*) newTopologyEntry->data;

    if (interfaceIndex != EIGRP_NULL0)
    {
        NodeAddress interfaceAddr = NetworkIpGetInterfaceNetworkAddress(
                                        node,
                                        interfaceIndex);

        if ((topologyPtr->metric.nextHop == 0) &&
            (topologyPtr->isSummary == TRUE)   &&
            (interfaceAddr != (destAddress << 8)))
        {
            // Summary address must point to NULL0 interface.
            interfaceIndex = EIGRP_NULL0;
        }
    }

    for (i = 0; i < numTos; i++)
    {
        int j = 0;
        int numEntries = BUFFER_GetCurrentSize(&eigrpRoutingTable[i].
            eigrpRoutingTableEntry) / sizeof(EigrpRoutingTableEntry);

        EigrpRoutingTableEntry* eigrpRoutingTableEntry =
            (EigrpRoutingTableEntry*) BUFFER_GetData(
                &eigrpRoutingTable[i].eigrpRoutingTableEntry);

        for (j = 0; j < numEntries; j++)
        {
            EigrpTopologyTableEntry* temp = (EigrpTopologyTableEntry*)
                eigrpRoutingTableEntry[j].feasibleRoute->data;

            NodeAddress tempDestAddress =
               EigrpConvertCharArrayToInt(temp->metric.destination);

            if (tempDestAddress == destAddress)
            {
                // Modify the existing routing table entry ...
                // This block of code is suppose to execute,
                // if previous feasible entry is obsoleted by
                // the current incoming entry ....
                EigrpTopologyTableEntry* prevTopologyEntry = temp;

                EigrpTopologyTableEntry* newEnrty = ((EigrpTopologyTableEntry*)
                    (newTopologyEntry->data));

                ((EigrpTopologyTableEntry*)(newTopologyEntry->data))->
                    isOptimum = TRUE;

                eigrpRoutingTableEntry[j].feasibleRoute = newTopologyEntry;

                eigrpRoutingTableEntry[j].outgoingInterface =
                    interfaceIndex;

                eigrpRoutingTableEntry[j].numOfPacketSend = 0;
                eigrpRoutingTableEntry[j].lastUpdateTime = node->getNodeTime();

                // The previous feasible route is not the
                // feasible entry any more...
                if (prevTopologyEntry != newEnrty)
                {
                    temp->isOptimum = FALSE;
                }
                break;
            } // end if (tempDestAddress == destAddress)
        } // end for (j = 0; j < numEntries; j++)

        // If entry do not exist in the routing table then
        // it must be a new route (not seen before), add it
        if (j == numEntries)
        {
            EigrpRoutingTableEntry newRoute = {0};
            newRoute.feasibleRoute = newTopologyEntry;
            newRoute.outgoingInterface = interfaceIndex;
            newRoute.numOfPacketSend = 0;
            newRoute.lastUpdateTime = node->getNodeTime();

            BUFFER_AddDataToDataBuffer(
                &eigrpRoutingTable[i].eigrpRoutingTableEntry,
                (char*) &newRoute,
                sizeof(EigrpRoutingTableEntry));
        }
    } // end for (i = 0; i < numTos; i++)
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpEnterRouteIntoTopologyTable()
//
// PURPOSE      : Entering a route into the topology table
//
// PARAMETERS   : eigrp - routing eigrp structure
//                sourceAddress - source address of the node from which
//                                the routing update has been received.
//                metric - vector metric in routing information
//                reportedDistance - reported distance of the destination
//                calculatedDistance - calculated of the destination
//                routingTableMayChange - will this entry change EIGRP
//                                        routing table ? return value
//                                        should be either TRUE or FALSE
//
// RETURN VALUE : pointer to the entry that just has been added
//                or modified in the topology table
//--------------------------------------------------------------------------
static
ListItem* EigrpEnterRouteIntoTopologyTable(
    Node* node,
    RoutingEigrp* eigrp,
    NodeAddress sourceAddress,
    int interfaceIndex,
    EigrpMetric metric,
    float reportedDistance,
    float calculatedDistance,
    BOOL* routingTableMayChange,
    unsigned short asId)
{
    *routingTableMayChange = FALSE;
    if (metric.delay == EIGRP_DESTINATION_INACCESSABLE)
    {
        // If delay of incoming metric is EIGRP_DESTINATION_INACCESSABLE
        // and if that entry exist in topology table then make it's delay
        // equal to EIGRP_EIGRP_DESTINATION_INACCESSABLE

        ListItem* temp = eigrp->topologyTable->first;
        EigrpTopologyTableEntry* topologyEntry = NULL;
        *routingTableMayChange = FALSE;

        while (temp)
        {
            NodeAddress destinationInTopologyTable = 0, incomingDest = 0;
            topologyEntry = (EigrpTopologyTableEntry*) temp->data;

            destinationInTopologyTable = EigrpConvertCharArrayToInt(
               topologyEntry->metric.destination);

            incomingDest = EigrpConvertCharArrayToInt(
                metric.destination);

            if ((destinationInTopologyTable == incomingDest) &&
                ((topologyEntry->metric.nextHop == sourceAddress)
                ||(topologyEntry->outgoingInterface == interfaceIndex)))
            {
                if (topologyEntry->isOptimum == TRUE)
                {
                    topologyEntry->isOptimum = FALSE;
                    *routingTableMayChange = TRUE;
                }

                // Make the entry unreachable in the topology table.
                topologyEntry->metric.delay = EIGRP_DESTINATION_INACCESSABLE;
                topologyEntry->isOptimum = FALSE;
                break;
            }
            temp = temp->next;
        }
        // No new entry has been added in the topology table, but delay
        // field of some existing entry has been change to
        // EIGRP_EIGRP_DESTINATION_INACCESSABLE. And return pointer to the
        // entry that has been modified to EIGRP_DESTINATION_INACCESSABLE.
        return temp;
    }
    else
    {
        ListItem* temp = eigrp->topologyTable->first;
        ListItem* unreachableDestPtr = NULL;
        EigrpTopologyTableEntry* topologyEntry = NULL;
        EigrpTopologyTableEntry* previouslyUnreachableDest = NULL;

        #define FIX_WIRELESS_TOPOLOGY
        #ifdef  FIX_WIRELESS_TOPOLOGY
        BOOL isDuplicate = FALSE;
        ListItem* dupPtr = NULL;
        EigrpTopologyTableEntry* topologyEntryDup = NULL;
        while (temp)
        {
            NodeAddress destinationInTopologyTable = ANY_DEST;
            NodeAddress incomingDest = ANY_DEST;

            topologyEntry = (EigrpTopologyTableEntry*) temp->data;

            destinationInTopologyTable = EigrpConvertCharArrayToInt(
               topologyEntry->metric.destination);

            incomingDest = EigrpConvertCharArrayToInt(metric.destination);

            if ((destinationInTopologyTable == incomingDest) &&
                (topologyEntry->metric.nextHop == metric.nextHop))
            {
                isDuplicate = TRUE;
                topologyEntryDup = (EigrpTopologyTableEntry*) temp->data;
                dupPtr = temp;
                break;
            }
            temp = temp->next;
        }
        #endif

        // Search in the topology table if a feasible distance for
        // the incoming destination already exists or not.
        temp = eigrp->topologyTable->first;
        while (temp)
        {
            NodeAddress destinationInTopologyTable = ANY_DEST;
            NodeAddress incomingDest = ANY_DEST;

            topologyEntry = (EigrpTopologyTableEntry*) temp->data;

            destinationInTopologyTable = EigrpConvertCharArrayToInt(
               topologyEntry->metric.destination);

            incomingDest = EigrpConvertCharArrayToInt(metric.destination);

            // find most optimum entry.
            if ((destinationInTopologyTable == incomingDest) &&
                (topologyEntry->isOptimum == TRUE) &&
                (topologyEntry->metric.delay !=
                     EIGRP_DESTINATION_INACCESSABLE))
            {
                break;
            }

            if ((topologyEntry->metric.delay ==
                     EIGRP_DESTINATION_INACCESSABLE) &&
                (destinationInTopologyTable == incomingDest) &&
                (topologyEntry->metric.nextHop == metric.nextHop))
            {
                previouslyUnreachableDest = topologyEntry;
                unreachableDestPtr = temp;
            }
            temp = temp->next;
        }

        if (temp != NULL)
        {
            EigrpTopologyTableEntry* newEntry = NULL;

            // if (temp != NULL); then a feasible entry for that incoming
            // destination address already exists in the topology table.
            //
            // NOTE : We identify a path uniquely by the combination
            //        <destination address, nexthop Address>
            if ((calculatedDistance == topologyEntry->feasibleDistance) &&
                (topologyEntry->metric.nextHop == sourceAddress))
            {
                // probably duplicate entry ignore it.
                // Nothing is added to topology table
                if (sourceAddress == DIRECTLY_CONNECTED_NETWORK)
                {
                     topologyEntry->isOptimum = TRUE;
                     *routingTableMayChange = TRUE;
                     return temp;
                }
                *routingTableMayChange = FALSE;
                return NULL;
            }
            else if ((calculatedDistance != topologyEntry->feasibleDistance)
                      && (topologyEntry->metric.nextHop == sourceAddress))
            {
                // Control comes here if metric of an already existing
                // entry changes. If metric of an alteady existing changes
                // Then that entry may not be feasible any more. So look
                // for a feasible entry....
                ListItem* feasibleSucc =
                EigrpSearchTopologyTableForFeasibleEntry(metric, eigrp);

                if (feasibleSucc == NULL)
                {
                    *routingTableMayChange = FALSE;
                }
                else
                {
                    *routingTableMayChange = TRUE;
                }
                return feasibleSucc;
            }

            if (reportedDistance < topologyEntry->feasibleDistance)
            {
                newEntry = (EigrpTopologyTableEntry*)
                    MEM_malloc(sizeof(EigrpTopologyTableEntry));

                // 1) isDefaultRoute = FALSE;
                // 2) eigrpExtraTopologyMetric = NULL;
                // 3) isSummary = FALSE;
                // 4) isDefinedInEigrpProcess = FALSE;
                memset(newEntry, 0, sizeof(EigrpTopologyTableEntry));

                newEntry->metric = metric;
                newEntry->asId = asId;

                if (asId == eigrp->asId)
                {
                    newEntry->entryType = EIGRP_SYSTEM_ROUTE;
                }
                else
                {
                    newEntry->entryType = EIGRP_EXTERIOR_ROUTE;
                }
                // External route disabled
                newEntry->entryType = EIGRP_SYSTEM_ROUTE;

                newEntry->outgoingInterface = interfaceIndex;
                newEntry->reportedDistance = reportedDistance;
                newEntry->feasibleDistance = calculatedDistance;
                newEntry->isOptimum = FALSE;

                if (sourceAddress == DIRECTLY_CONNECTED_NETWORK)
                {
                    newEntry->isOptimum = TRUE;
                }

                // if reportedDistance is less than feasibleDistance
                // then enter the route in the topology table

                // Add data to the topology table
                if ((topologyEntryDup) &&
                    (topologyEntryDup->isOptimum == FALSE))
                {
                    ListGet(node,
                            eigrp->topologyTable,
                            dupPtr,
                            TRUE,
                            FALSE);
                }

                ListInsert(node, eigrp->topologyTable,
                    node->getNodeTime(), newEntry);

                // feasible distance of new path is less
                // than feasible distance of existing feasible path
                if (calculatedDistance < topologyEntry->feasibleDistance)
                {
                    *routingTableMayChange = TRUE;
                }
                // return (pointer to the entry added)
                return eigrp->topologyTable->last;
            }

            if ((sourceAddress == DIRECTLY_CONNECTED_NETWORK)
                && topologyEntryDup)
            {
                topologyEntryDup->metric = metric;
                *routingTableMayChange = TRUE;
                return dupPtr;
            }

            *routingTableMayChange = FALSE;
            return NULL;
        }
        else // end if (temp == NULL)
        {
            // Then a feasible entry for the incoming destination address
            // does not already exist in the topology table.
            EigrpTopologyTableEntry* newEntry = NULL;

            if (previouslyUnreachableDest != NULL)
            {
                newEntry = previouslyUnreachableDest;
            }
            else
            {
                newEntry = (EigrpTopologyTableEntry*)
                    MEM_malloc(sizeof(EigrpTopologyTableEntry));

                // 1) isDefaultRoute = FALSE;
                // 2) eigrpExtraTopologyMetric = NULL;
                // 3) isSummary = FALSE;
                // 4) isDefinedInEigrpProcess = FALSE;
                memset(newEntry, 0, sizeof(EigrpTopologyTableEntry));
            }

            if (interfaceIndex == EIGRP_NULL0)
            {
                metric.delay = EIGRP_DESTINATION_INACCESSABLE;
            }

            newEntry->metric = metric;
            newEntry->asId = asId;

            if (asId == eigrp->asId)
            {
                newEntry->entryType = EIGRP_SYSTEM_ROUTE;
            }
            else
            {
                newEntry->entryType = EIGRP_EXTERIOR_ROUTE;
            }
            // External route disabled
            newEntry->entryType = EIGRP_SYSTEM_ROUTE;

            newEntry->outgoingInterface = interfaceIndex;
            newEntry->reportedDistance = reportedDistance;
            newEntry->feasibleDistance = calculatedDistance;
            newEntry->isOptimum = TRUE;

            // Add data to the topology table
            *routingTableMayChange = TRUE;

            if (previouslyUnreachableDest == NULL)
            {
                // Add data to the topology table
                if ((topologyEntryDup) &&
                    (topologyEntryDup->isOptimum == FALSE))
                {
                    ListGet(node,
                            eigrp->topologyTable,
                            dupPtr,
                            TRUE,
                            FALSE);
                }

                ListInsert(node,
                    eigrp->topologyTable,
                    node->getNodeTime(),
                    newEntry);

                // return (pointer to the entry added)
                return eigrp->topologyTable->last;
            }
            return unreachableDestPtr;
        } // end else if (temp != NULL)
    } // end else of "(EigrpConvertCharArrayToInt(metric.delay)
      // == EIGRP_DESTINATION_INACCESSABLE)"
} // end EigrpEnterRouteIntoTopologyTable()


//-------------------------------------------------------------------------
// FUNCTION     : EigrpSetStuckInActiveRouteTimer()
//
// PURPOSE      : Setting SIA timer
//
// PARAMETERS   : node - node which is setting Stack In Active (SIA)
//                       Route Timer
//                eigrp - pointer to eigrp route
//                interfaceIndex - interface index to find neighbor
//
//  RETURN VALUE : none
//-------------------------------------------------------------------------
static
void EigrpSetStuckInActiveRouteTimer(
    Node* node,
    RoutingEigrp* eigrp,
    int intrefaceIndex)
{
    Message* msg = NULL;

    if (eigrp->isStuckTimerSet[intrefaceIndex] == EIGRP_STUCK_OFF)
    {
        eigrp->isStuckTimerSet[intrefaceIndex] = EIGRP_STUCK_SET;
        msg = MESSAGE_Alloc(
                  node,
                  NETWORK_LAYER,
                  ROUTING_PROTOCOL_EIGRP,
                  MSG_ROUTING_EigrpStuckInActiveRouteTimerExpired);

        MESSAGE_InfoAlloc(node, msg, sizeof(NeighborSearchInfo));
        memcpy(MESSAGE_ReturnInfo(msg), &intrefaceIndex, sizeof(int));
        MESSAGE_Send(node, msg, eigrp->waitTime);
   }
}


//----------------------------------------------------------------------
// FUNCTION     : EigrpSendQueryPacket()
//
// PURPOSE      : Sending query message to neighbors.
//
// PARAMETERS   : node - node sending the message
//                eigrp - routing eigrp structure
//                interfaceIndex - interface in dex thru which query
//                                 message will be send.
//                routeToFind - address of the neighbor
//                pointer to query message structure
//                queryMsgSize - sizeOf the query meassage
//                queryPacketType - query Packet Type
//
// RETURN VALUE : void
//----------------------------------------------------------------------
static
void EigrpSendQueryPacket(
    Node* node,
    RoutingEigrp* eigrp,
    int interfaceIndex,
    EigrpQueryMessage *routeToFind,
    int queryMsgSize,
    EigrpQueryPacketType queryPacketType)
{
    int numSystemRoute = 0;
    int numExternRoute = 0;

    // Allocate packet buffer to build up query packet
    PacketBuffer* packetBuffer =
        BUFFER_AllocatePacketBuffer(
        0,         // Initial size
        (EIGRP_MAX_ROUTE_ADVERTISED *
                 sizeof(EigrpUpdateMessage)),
        FALSE,
        NULL);

    if (queryPacketType == EIGRP_QUERY_ORIGIN)
    {
        NodeAddress  sourceAddress =
            NetworkIpGetInterfaceAddress(node, interfaceIndex);

        // Add the route to be quired. The query message will contain
        // only the destination and ip prefix of the destination
        // that is to be queried.
        EigrpAddDataToPacketBuffer(
            packetBuffer,
            sizeof(EigrpQueryMessage),
            (char*) routeToFind);

        if (DEBUG_EIGRP_QUERY)
        {
            char destAddress[MAX_STRING_LENGTH] = {0};

            IO_ConvertIpAddressToString(
                (EigrpConvertCharArrayToInt(routeToFind->destination) << 8),
                destAddress);

            printf("Node %d is sending query for the destination %s"
                   " thru interface index %d\n",
                   node->nodeId,
                   destAddress,
                   interfaceIndex);

            PrintRoutingTable(node);
            PrintTopologyTable(node);
        }

        // Add source node address to query packet. Set query source for
        // each route. This query source will be used by the reply-sender to
        // unicast the  reply message directly to the query-source.
        EigrpAddDataToPacketBuffer(
            packetBuffer,
            sizeof(NodeAddress),
            (char*) &sourceAddress);
    }
    else if (queryPacketType == EIGRP_QUERY_FORWORD)
    {
        if (DEBUG_EIGRP_QUERY)
        {
            char destAddress[MAX_STRING_LENGTH] = {0};
            char querySource[MAX_STRING_LENGTH] = {0};
            NodeAddress destAddr = ANY_DEST;
            unsigned char queriedRoute[3] = {0};

            char* destAddressPtr = ((char*) routeToFind)
                                   + sizeof(EigrpTlv);

            char* queriedRoutePtr = ((char*) routeToFind
                + sizeof(EigrpTlv) + sizeof(NodeAddress));

            // Get Ip address of the originator of the query
            memcpy(&destAddr, destAddressPtr , sizeof(NodeAddress));
            IO_ConvertIpAddressToString(destAddr, destAddress);

            // Get the destination that is being queried.
            memcpy(queriedRoute, queriedRoutePtr, 3);
            IO_ConvertIpAddressToString(
                (EigrpConvertCharArrayToInt(queriedRoute) << 8),
                querySource);

            printf("Node %d is Forwarding query for the destination %s"
                   " thru interface index %d\n QUERIED SOURCE = %s\n",
                   node->nodeId,
                   destAddress,
                   interfaceIndex,
                   querySource);
        }

        // Copy the packet only
        EigrpAddDataToPacketBuffer(
            packetBuffer,
            (queryMsgSize - sizeof(EigrpTlv)),
            (((char*) routeToFind) + sizeof(EigrpTlv)));
    }

    EigrpAddHeaderAndSendPacketToMacLayer(
        node,
        eigrp,
        packetBuffer,
        numSystemRoute, // This argument is not used.
        NULL,           // This argument is not used.
        numExternRoute, // This argument is not used.
        interfaceIndex,
        EIGRP_QUERY_PACKET,
        EIGRP_ROUTER_MULTICAST_ADDRESS);

    BUFFER_FreePacketBuffer(packetBuffer);
    eigrp->eigrpStat->numQuerySend++;

    // Set Stuck InActive Timer.
    EigrpSetStuckInActiveRouteTimer(node, eigrp, interfaceIndex);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpCallDualDecisionProcess()
//
// PURPOSE      : Building up eigrp routing table
//
// PARAMETER    : node - the node which is running the dual routing process
//                eigrp - eigrp routing structure
//                destAddress - destAddress to be placed in routing table
//                interfaceIndex - outgoing interface index for "destAddress"
//                newTopologyEntry - pointer to the topology table entry last
//                                   entered
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
static
void EigrpCallDualDecisionProcess(
    Node* node,
    RoutingEigrp* eigrp,
    EigrpMetric metric,
    unsigned short ipPrefix,
    int interfaceIndex,
    ListItem* newTopologyEntry,
    BOOL isSnooping)
{
    NodeAddress destAddress = EigrpConvertCharArrayToInt(
        metric.destination);

    if (metric.delay == EIGRP_DESTINATION_INACCESSABLE)
    {
        // When you reach the code inside this block, it means :
        // The Delay of a route in the topology table has been set as
        // EIGRP_DESTINATION_INACCESSABLE. And change may effect the
        // routing table of EIGRP.

        // Search the topology table for feasible  successor with
        // least feasible distance (route re computation)
        ListItem* feasibleSucc =
        EigrpSearchTopologyTableForFeasibleEntry(metric, eigrp);

        if (isSnooping == TRUE)
        {
            return;
        }

        if (feasibleSucc != NULL)
        {
            NodeAddress  destinationAddress = ANY_DEST;

            // If at least one alternative entry found in topology table,
            // then update the routing table entry with that entry.
            // in this block i.e. when
            // "EigrpConvertCharArrayToInt(metric.delay) ==
            // EIGRP_DESTINATION_INACCESSABLE", newTopologyEntry
            // indicates pointer to an infeasible entry that has to be
            // replaced.

            EigrpUpdateRoutingTableEntryWithFeasibleSuccessor(
                node,
                eigrp,
                feasibleSucc,
                newTopologyEntry);

            destinationAddress = EigrpConvertCharArrayToInt(
                ((EigrpTopologyTableEntry*) feasibleSucc->data)->
                    metric.destination);

            if (((EigrpTopologyTableEntry*) feasibleSucc->data)->
                    outgoingInterface != ANY_INTERFACE)
            {
                // inform network layer about routing table change
                NetworkUpdateForwardingTable(
                    node,
                    (destinationAddress << 8),
                    ConvertNumHostBitsToSubnetMask(32 - ipPrefix),
                    ((EigrpTopologyTableEntry*) feasibleSucc->data)->
                        metric.nextHop,
                    ((EigrpTopologyTableEntry*) feasibleSucc->data)->
                        outgoingInterface,
                    ((EigrpTopologyTableEntry*) feasibleSucc->data)->
                        metric.hopCount,
                    ROUTING_PROTOCOL_EIGRP);
            } // end if () [ADDED FOR ROUTE SUMMARIZATION]

            //
            // Send query to the feasible successor
            // This is needed for loop detection
            //
            EigrpQueryMessage routeToFind = {{0}};
            int queryMsgSize = sizeof(EigrpQueryMessage);

            // Build query entry...
            memcpy(routeToFind.destination, metric.destination, 3);
            routeToFind.ipPrefix = metric.ipPrefix;

            // set destination unreachable and isOptimumm = False.
            ((EigrpTopologyTableEntry*) newTopologyEntry->data)->
                isOptimum = FALSE;

            // send the query message
            EigrpSendQueryPacket(
                node,
                eigrp,
                ((EigrpTopologyTableEntry*) feasibleSucc->data)->
                    outgoingInterface,
                &routeToFind,
                queryMsgSize,
                EIGRP_QUERY_ORIGIN);
        }
        else // if (feasibleSucc == NULL)
        {
            // If no alternative feasible route entry found then send
            // query message through all interface.
            // schedule query message here.
            int i = ANY_INTERFACE;
            EigrpQueryMessage routeToFind = {{0}};
            int queryMsgSize = sizeof(EigrpQueryMessage);

            // Build query entry...
            memcpy(routeToFind.destination, metric.destination, 3);
            routeToFind.ipPrefix = metric.ipPrefix;

            // set destination unreachable and isOptimumm = False.
            ((EigrpTopologyTableEntry*) newTopologyEntry->data)->
                isOptimum = FALSE;

            // Send query message thru all interfaces
            for (i = 0; i < node->numberInterfaces; i++)
            {
                // Send the query thru all other interfaces except the
                // interface thru which the query has been received

                  if ((! NetworkIpInterfaceIsEnabled(node, i))
                       || (NetworkIpGetUnicastRoutingProtocolType(node, i)
                          != ROUTING_PROTOCOL_EIGRP))
                  {
                        continue;
                  }

                  EigrpSendQueryPacket(
                        node,
                        eigrp,
                        i,
                        &routeToFind,
                        queryMsgSize,
                        EIGRP_QUERY_ORIGIN);

            }

            // Make network unreachable in the network-layer
            // forwarding table.
            NetworkUpdateForwardingTable(
                node,
                (destAddress << 8),
                ConvertNumHostBitsToSubnetMask(32 - ipPrefix),
                (NodeAddress) NETWORK_UNREACHABLE,
                ANY_INTERFACE,
                -1, // may need change
                ROUTING_PROTOCOL_EIGRP);
        } // end if (assumedFeasibleDistanceFound)
    }
    else // if destination is NOT UNREACHABLE
    {
        NodeAddress  destinationAddress = ANY_DEST;

        // If control reaches here then either some route has been
        // modified or a new route has been added.
        EigrpAddRouteToRoutingTable(
            node,
            eigrp,
            destAddress,
            interfaceIndex,
            newTopologyEntry);

        destinationAddress = EigrpConvertCharArrayToInt(
            ((EigrpTopologyTableEntry*) newTopologyEntry->data)->
                metric.destination);

        if (interfaceIndex != ANY_INTERFACE)
        {
            // Update network layer forwarding table
            NetworkUpdateForwardingTable(
                node,
                (destinationAddress << 8),
                ConvertNumHostBitsToSubnetMask(32 - ipPrefix),
                ((EigrpTopologyTableEntry*) newTopologyEntry->data)->
                    metric.nextHop,
                interfaceIndex,
                ((EigrpTopologyTableEntry*) newTopologyEntry->data)->
                    metric.hopCount,
                ROUTING_PROTOCOL_EIGRP);
        } // end if () [ADDED FOR ROUTE SUMMARIZATION]
    } // end else part of "if (newTopologyEntry == NULL)"
} // EigrpCallDualDecisionProcess()


//--------------------------------------------------------------------------
// FUNCTION     : AllocateEigrpRoutingSpace()
//
// PURPOSE      : allocating space for EIGRP routing structure
//                and initialize structure element with default values
//
// PARAMETERS   : node - node which big getting initialized
//              : eigrp - routing EIGRP structure
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
static
void AllocateEigrpRoutingSpace(Node* node, RoutingEigrp** eigrp)
{
    int i = 0;
    EigrpRoutingTable* eigrpRoutingTable = NULL;

    if (EIGRP_DEBUG)
    {
        printf("Node %u is entering eigrpInit\n", node->nodeId);
    }

    (*eigrp) =(RoutingEigrp*) MEM_malloc(sizeof(RoutingEigrp));

    // create Initialize neighbor structure
    (*eigrp)->neighborTable = (EigrpNeighborTable*)
        MEM_malloc(sizeof(EigrpNeighborTable) * node->numberInterfaces);

    //  1. first = NULL
    //  2. last  = NULL
    //  3. seqNo = 0
    memset((*eigrp)->neighborTable, 0,
        sizeof(EigrpNeighborTable) * (node->numberInterfaces));

    // set default hold time and  default hello interval
    // for each interface in the neighbor table
    for (i = 0; i < node->numberInterfaces; i++)
    {
        // initialize my own hello interval
        (*eigrp)->neighborTable[i].helloInterval =
             HELLO_INTERVAL_ON_LOW_BANDWIDTH_LINK;

        // initialize my own hold timer.
        (*eigrp)->neighborTable[i].holdTime = EIGRP_DEFAULT_HOLD_TIMER;
    }

    // Initialize topology table
    ListInit(node, &((*eigrp)->topologyTable));

    // Initialization of routing table structures
    BUFFER_InitializeDataBuffer(&((*eigrp)->eigrpRoutingTable),
        MAX_NUM_OF_TOS * sizeof(EigrpRoutingTable));

    eigrpRoutingTable = (EigrpRoutingTable*)
        BUFFER_GetData(&((*eigrp)->eigrpRoutingTable));

    eigrpRoutingTable->tos = DEFAULT_TOS;
    eigrpRoutingTable->k1 = 1;
    eigrpRoutingTable->k2 = 0;
    eigrpRoutingTable->k3 = 1;
    eigrpRoutingTable->k4 = 0;
    eigrpRoutingTable->k5 = 0;

    BUFFER_InitializeDataBuffer(&(eigrpRoutingTable->eigrpRoutingTableEntry),
        MAX_ANTICIPATED_SIZE * sizeof(EigrpRoutingTableEntry));

    BUFFER_SetCurrentSize(&((*eigrp)->eigrpRoutingTable),
        sizeof(EigrpRoutingTable));

    // End initialization of routing table structure.
    (*eigrp)->edition = 0;
    (*eigrp)->variance = 1.0;
    (*eigrp)->asId = 0;
    (*eigrp)->maxHop = EIGRP_MAX_HOP_COUNT;

    // Initialize timer values
    (*eigrp)->ackTime = HELLO_INTERVAL_ON_LOW_BANDWIDTH_LINK * 2;
    (*eigrp)->waitTime = HELLO_INTERVAL_ON_LOW_BANDWIDTH_LINK * 1;
    (*eigrp)->sleepTime = EIGRP_SLEEP_TIME;
    (*eigrp)->lastUpdateSent = 0;

    (*eigrp)->splitHorizonEnabled = TRUE;
    (*eigrp)->poisonReverseEnabled = FALSE;
    (*eigrp)->isNextUpdateScheduled = FALSE;
    (*eigrp)->isQueryScheduled = FALSE;

    (*eigrp)->collectStat = FALSE;
    (*eigrp)->eigrpStat = NULL;

    (*eigrp)->isRouteFilterEnabled = FALSE;
    EigrpInitFilterListTable(*eigrp);

    // By default route summarization is on
    (*eigrp)->isSummaryOn = TRUE;

    (*eigrp)->isStuckTimerSet = (int*) MEM_malloc(
        node->numberInterfaces * sizeof(int));
    memset((*eigrp)->isStuckTimerSet, 0,
        node->numberInterfaces * sizeof(int));

    // Add yourself to a multicast group.
    NetworkIpAddToMulticastGroupList(
        node,
        EIGRP_ROUTER_MULTICAST_ADDRESS);
}


//-------------------------------------------------------------------------
// FUNCTION     : EigrpParseIpAddress()
//
// PARAMETERS   : ipAddrString - ip address in dotted decimal format
//
// RETURN VALUE : Ip address in unsigned int format
//
// ASSUMPTION   : Ip address must be null terminated string.
//-------------------------------------------------------------------------
static
unsigned int EigrpParseIpAddress(char ipAddrString[])
{
    unsigned int i = 0, j = 0, pos = 0, ipAddress = 0;
    char digit[4] = {0};

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 3; j++)
        {
            if (isdigit((int)ipAddrString[pos]))
            {
                digit[j] = ipAddrString[pos];
            }
            else if ((ipAddrString[pos] == '.') ||
                     (ipAddrString[pos] == '\0'))
            {
                break;
            }
            else
            {
                ERROR_Assert(FALSE, "Bad IP string");
                abort();
            }
            pos++;
        } // end for (j = 0; j < 3; j++)

        if (atoi(digit) > UCHAR_MAX)
        {
            ERROR_Assert(FALSE, "Bad digit in IP string");
            abort();
        }

        if ((ipAddrString[pos] != '.') &&
            (ipAddrString[pos] != '\0'))
        {
            ERROR_Assert(FALSE, "Bad IP string");
            abort();
        }
        else
        {
            pos++;
        }

        ipAddress = ipAddress + (atoi(digit) << (8 * (3 - i)));
        memset(digit, 0, 4);
    } // end for (i = 0; i < 4; i++)
    return ipAddress;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpReadFromConfigurationFile()
//
// PURPOSE      : 1.Reading configuration parameters from
//                  eigrp configuration file
//                2.Determine if the node is an EIGRP router or not
//
// PARAMETERS   : node - The node being initialized.
//                eigrp - The eigrp structure
//                eigrpConfigFile - The eigrp configuration file to be read.
//                found - is there any entry found in eigrp file for the
//                        "node" given? found == TRUE when an entry is
//                        found or FALES otherwise.
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
static
void EigrpReadFromConfigurationFile(
    Node* node,
    RoutingEigrp** eigrp,
    NodeInput* eigrpConfigFile,
    BOOL* found)
{
    int i = 0;
    *found = FALSE;
    while (i < eigrpConfigFile->numLines)
    {
        char token[MAX_STRING_LENGTH] = {0};
        unsigned int nodeId = 0;
        unsigned int asId = 0;
        char* currentLine = eigrpConfigFile->inputStrings[i];

        sscanf(currentLine, "%s", token);

        // Skip if not a router statement
        if (strcmp(token, "ROUTER"))
        {
            i++;
            continue;
        }

        // Program invariant: A "ROUTER" statement was read
        sscanf(currentLine, "%*s%u%u", &nodeId, &asId);

        if ((asId == 0) || (asId > USHRT_MAX))
        {
            char buff[MAX_STRING_LENGTH] = {0};
            unsigned int unsignedShortMax = USHRT_MAX;

            sprintf(buff, "For node %u Value of asId is specied is"
                " wrong (%u)\n. That must be in the range 0 to %u\n",
                node->nodeId, asId, unsignedShortMax);

            ERROR_ReportError(buff);
        }
        // Program invariant: nodeId and as id was read
        // Skip if router statement doesn't belong to us.
        if (node->nodeId != nodeId)
        {
            i++;
            continue;
        }

        // Program invariant: node reading the configuration file
        //                    and nodeId read from the config
        //                    file matched
        // Each eigrp router should reach this point of code
        // once and once only at the time of initialization

        // Initialize the node as EIGRP router
        AllocateEigrpRoutingSpace(node, eigrp);
        (*eigrp)->asId = (unsigned short) asId;

        // Program invariant: The node was initialized as an
        //                    eigrp router
        *found = TRUE;

        i++;
        currentLine = eigrpConfigFile->inputStrings[i];
        sscanf(currentLine, "%s\n", token);

        while ((strcmp(token, "ROUTER")) &&
               (i < eigrpConfigFile->numLines))
        {
            // Program invariant: next line is not the "ROUTER"
            //                    statement and not end of file.
            if (strcmp(token, "NETWORK") == 0)
            {
                char network[MAX_STRING_LENGTH] = {0};
                char userSubnetMask[MAX_STRING_LENGTH] = {0};
                NodeAddress outputNodeAddress = ANY_DEST;
                int numHostBits = 0;
                int interfaceIndex = ANY_INTERFACE;
                NodeAddress interfaceAddress = ANY_DEST;
                unsigned int propDelay = 0;
                unsigned int bandwidth = 0;
                EigrpMetric metric = {0};
                float reportedDistance = 0;
                float feasibleDistance = 0;
                BOOL routingTableMayChange = FALSE;
                ListItem* newTopologyEntry = NULL;

                sscanf(currentLine, "%*s%s%s", network,
                    userSubnetMask);

                outputNodeAddress = EigrpParseIpAddress(network);

                // Converting wildcard subnet mask to num host bits.
                numHostBits = ConvertSubnetMaskToNumHostBits(
                    (EigrpParseIpAddress(userSubnetMask)));

                // Program Invariant: A valid network address was read
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
                    interfaceIndex = EIGRP_NULL0;

                    bandwidth = EigrpGetInvBandwidth(
                                    node,
                                    DEFAULT_INTERFACE);
                    propDelay = (unsigned int)
                                (EigrpGetPropDelay(node, DEFAULT_INTERFACE)
                                 / MICRO_SECOND);
                }
                else
                {
                    // Program Invariant: It is ensured that network is valid
                    //                    and connected with the node

                    // Get bandwidth of the interface index
                    bandwidth = EigrpGetInvBandwidth(node, interfaceIndex);

                    // Get propagation delay of the interface index
                    propDelay = (unsigned int)
                        (EigrpGetPropDelay(node, interfaceIndex)
                            / MICRO_SECOND);
                }

                // Build up an routing metric entry.
                metric.nextHop = 0;
                metric.delay = propDelay;
                metric.bandwidth = bandwidth;
                memset(&metric.mtu, 0, (sizeof(char) * 3));
                metric.hopCount = 0;
                metric.reliability = 1;
                metric.load = 0;
                memset(&metric.reserved, 0, sizeof(unsigned char));
                metric.ipPrefix = (unsigned char) (32 - numHostBits);

                metric.destination[2] = ((unsigned char)
                    (0xFF & (outputNodeAddress >> 24)));
                metric.destination[1] = ((unsigned char)
                    (0xFF & (outputNodeAddress >> 16)));
                metric.destination[0] = ((unsigned char)
                    (0xFF & (outputNodeAddress >> 8)));

                reportedDistance = EigrpCalculateMetric(
                    &metric, 1, 0, 1, 0, 0);

                feasibleDistance = reportedDistance;

                newTopologyEntry = EigrpEnterRouteIntoTopologyTable(
                                       node,
                                       *eigrp,
                                       DIRECTLY_CONNECTED_NETWORK,
                                       interfaceIndex,
                                       metric,
                                       reportedDistance,
                                       feasibleDistance,
                                       &routingTableMayChange,
                                       (*eigrp)->asId);

                if (routingTableMayChange == TRUE)
                {
                    SET_SUMMARY_FLAG(newTopologyEntry, TRUE);

                    SET_DEFINED_IN_EIGRP_PROCESS_FLAG(
                        newTopologyEntry,
                        TRUE);

                    EigrpCallDualDecisionProcess(
                        node,
                        *eigrp,
                        metric,
                        metric.ipPrefix,
                        interfaceIndex,
                        newTopologyEntry,
                        FALSE);
                }

                if (EIGRP_DEBUG)
                {
                    printf("node: %4u neighbor Network: %s"
                           " interfaceIndex : %u\n"
                           " node: %4u bandWidth %u propagation-delay %u\n",
                           node->nodeId, network, interfaceIndex,
                           node->nodeId, bandwidth, propDelay);
                }
            }
            else if (strcmp(token, "VARIANCE") == 0)
            {
                float variance = 1.0;
                sscanf(currentLine, "%*s%f", &variance);
                (*eigrp)->variance = variance;

                if (EIGRP_DEBUG)
                {
                    printf("node: %4u variance: %f\n",
                        node->nodeId, variance);
                }
            }
            else if (strcmp(token, "IP-SPLIT-HORIZON") == 0)
            {
                (*eigrp)->splitHorizonEnabled = TRUE;

                if (EIGRP_DEBUG)
                {
                    printf("node: %u IP-SPLIT-HORIZON : split horizon"
                           " enabled\n",node->nodeId);
                }
            }
            else if (strcmp(token, "NO-IP-SPLIT-HORIZON") == 0)
            {
                (*eigrp)->splitHorizonEnabled = FALSE;

                if (EIGRP_DEBUG)
                {
                    printf("node: %u NO-IP-SPLIT-HORIZON : split horizon is "
                        "not enabled \n", node->nodeId);
                }
            }
            else if (strcmp(token, "EIGRP-HELLO-INTERVAL") == 0)
            {
                char buf[MAX_STRING_LENGTH];
                int interfaceIndex = ANY_INTERFACE;
                clocktype helloTime = 0;
                int numValueRead = sscanf(currentLine,
                                          "%*s%s%d",
                                          buf,
                                          &interfaceIndex);
                if (numValueRead != 2)
                {
                   ERROR_ReportError("erroneous configuration "
                       "EIGRP-HELLO-INTERVAL");
                }

                if ((interfaceIndex < 0) ||
                    (interfaceIndex > node->numberInterfaces))
                {
                   ERROR_ReportError("interface index exceeded"
                       " either min or max limit");
                }

                helloTime = TIME_ConvertToClock(buf);

                if (helloTime > 0)
                {
                    (*eigrp)->neighborTable[interfaceIndex].helloInterval =
                        helloTime;
                }
                else
                {
                    ERROR_ReportError("Hello Time is given less than 0\n");
                }
            }
            else if (strcmp(token, "EIGRP-HOLD-TIME") == 0)
            {
                char buf[MAX_STRING_LENGTH] = {0};
                int interfaceIndex = ANY_INTERFACE;
                clocktype holdTime = 0;
                int numValueRead = sscanf(currentLine, "%*s%s%d",
                                          buf, &interfaceIndex);

                if (numValueRead != 2)
                {
                    ERROR_ReportError("erroneous configuration"
                        "probably interface index is missing");
                }

                if ((interfaceIndex < 0) ||
                    (interfaceIndex > node->numberInterfaces))
                {
                    ERROR_ReportError("interface index exceeded"
                        " either min or max limit");
                }

                holdTime = TIME_ConvertToClock(buf);

                if (holdTime > 0)
                {
                    (*eigrp)->neighborTable[interfaceIndex].holdTime =
                        holdTime;
                }
                else
                {
                    ERROR_ReportError("Hold Time is given less than 0\n");
                }
            }
            else if (strcmp(token, "EIGRP-SLEEP-TIME") == 0)
            {
                char buf[MAX_STRING_LENGTH] = {0};
                sscanf(currentLine, "%*s%s", buf);
                (*eigrp)->sleepTime = TIME_ConvertToClock(buf);

                if ((*eigrp)->sleepTime < 0)
                {
                    ERROR_ReportError("Sleep Time is less than Zero");
                }
            }
            else if (strcmp(token, "EIGRP-POISON-REVERSE") == 0)
            {
                char buf[MAX_STRING_LENGTH] = {0};
                sscanf(currentLine, "%*s%s", buf);
                if (strcmp(buf, "YES") == 0)
                {
                    (*eigrp)->poisonReverseEnabled = TRUE;
                }
                else if (strcmp(buf, "NO") == 0)
                {
                    (*eigrp)->poisonReverseEnabled = FALSE;
                }
                else
                {
                    ERROR_ReportError("Parametre \"EIGRP-POISON-REVERSE\""
                        "must be \"YES\" or \"NO \"");
                    abort();
                }
            }
            else if (strcmp(token, "DISTRIBUTE-LIST") == 0)
            {
                unsigned int numValueRead = 0;

                char tkn[MAX_STRING_LENGTH] = {0}; // Read and throw.
                char acl[MAX_STRING_LENGTH] = {0}; // ACL type (name/number)
                char aclInterfaceType[MAX_STRING_LENGTH] = {0};
                char aclInterfaceNdx[MAX_STRING_LENGTH] = {0};

                // Read the configuration command from configuration file.
                numValueRead = sscanf(currentLine,
                                      "%s%s%s%s",
                                      tkn,
                                      acl,
                                      aclInterfaceType,
                                      aclInterfaceNdx);

                if (numValueRead == 3)
                {
                    (*eigrp)->isRouteFilterEnabled = TRUE;
                }
                else if (numValueRead == 4)
                {
                    if ((*eigrp)->isRouteFilterEnabled == TRUE)
                    {
                        // Add the parameter read into the
                        // Eigrp route filter table.
                        EigrpAddToFilterList(
                             &((*eigrp)->eigrpFilterListTable),
                             EigrpReturnAclType(acl),
                             acl,
                             EigrpReturnAclInterfaceType(
                                 aclInterfaceType),
                             EigrpCheckAndReturnInterfaceType(
                                 node,
                                 aclInterfaceNdx));
                    }
                    else
                    {
                        const char* errStr = (char *)"Distribute List is Not Activated";
                        ERROR_ReportError(errStr);
                        abort();
                    }
                }
                else
                {
                    const char* errStr = (char *)"Too few parameters in Distribute List";
                    ERROR_ReportError(errStr);
                    abort();
                }
            }
            else if (strcmp(token, "IP-DEFAULT-NETWORK") == 0)
            {
                char network[MAX_STRING_LENGTH] = {0};
                NodeAddress outputNodeAddress = ANY_DEST;
                int numHostBits = 32;
                int interfaceIndex = ANY_INTERFACE;
                unsigned int propDelay = 0;
                unsigned int bandwidth = 0;
                EigrpMetric metric = {0};
                float reportedDistance = 0;
                float feasibleDistance = 0;
                BOOL routingTableMayChange = FALSE;
                ListItem* newTopologyEntry = NULL;

                sscanf(currentLine, "%*s%s", network);
                outputNodeAddress = EigrpParseIpAddress(network);

                for (interfaceIndex = 0;
                     interfaceIndex < node->numberInterfaces;
                     interfaceIndex++)
                {
                    NodeAddress interfaceSubnetAddress =
                        MAPPING_GetSubnetAddressForInterface(
                            node,
                            node->nodeId,
                            interfaceIndex);

                    if (interfaceSubnetAddress == outputNodeAddress)
                    {
                        break;
                    }
                }

                if (interfaceIndex == node->numberInterfaces)
                {
                    interfaceIndex = DEFAULT_INTERFACE;
                }

                // Program Invariant: It is ensured that network is valid
                //                    and connected with the node
                bandwidth = EigrpGetInvBandwidth(node, interfaceIndex);

                propDelay = (unsigned int)
                    (EigrpGetPropDelay(node, interfaceIndex)
                        / MICRO_SECOND);

                // Build up an routing metric entry.
                metric.nextHop = 0;
                metric.delay = propDelay;
                metric.bandwidth = bandwidth;
                memset(&metric.mtu, 0, (sizeof(char) * 3));
                metric.hopCount = 0;
                metric.reliability = 1;
                metric.load = 0;
                memset(&metric.reserved, 0, sizeof(unsigned char));
                metric.ipPrefix = (unsigned char) (32 - numHostBits);

                metric.destination[2] = ((unsigned char)
                    (0xFF & (outputNodeAddress >> 24)));
                metric.destination[1] = ((unsigned char)
                    (0xFF & (outputNodeAddress >> 16)));
                metric.destination[0] = ((unsigned char)
                    (0xFF & (outputNodeAddress >> 8)));

                reportedDistance = EigrpCalculateMetric(
                    &metric, 1, 0, 1, 0, 0);

                feasibleDistance = reportedDistance;

                newTopologyEntry = EigrpEnterRouteIntoTopologyTable(
                                       node,
                                       *eigrp,
                                       DIRECTLY_CONNECTED_NETWORK,
                                       interfaceIndex,
                                       metric,
                                       reportedDistance,
                                       feasibleDistance,
                                       &routingTableMayChange,
                                       (*eigrp)->asId);

                if (routingTableMayChange == TRUE)
                {
                    // Set the default candidate flag to true.
                    SET_DEFAULT_ROUTE_FLAG(newTopologyEntry, TRUE);

                    EigrpSetExternalFieldsInTopologyTable(
                        (EigrpTopologyTableEntry*)(newTopologyEntry->data),
                        NetworkIpGetInterfaceAddress(
                            node,
                            DEFAULT_INTERFACE),
                        (*eigrp)->asId,
                        0, // Arbitrary TAG value (currently not used)
                        (unsigned int) feasibleDistance,
                        EIGRP_EXTERN_EIGRP,
                        EIGRP_DEFAULT_ROUTE_FLAG);

                    EigrpCallDualDecisionProcess(
                        node,
                        *eigrp,
                        metric,
                        metric.ipPrefix,
                        interfaceIndex,
                        newTopologyEntry,
                        FALSE);
                }
            }
            else if (strcmp(token, "IP-ROUTE") == 0)
            {
                char network[MAX_STRING_LENGTH] = {0};
                char prefix[MAX_STRING_LENGTH] = {0};
                char mask[MAX_STRING_LENGTH] = {0};

                NodeAddress interfaceAddress = ANY_DEST;
                NodeAddress subnetMask = ANY_DEST;
                NodeAddress prefixAddr = ANY_DEST;

                int interfaceIndex = ANY_INTERFACE;
                unsigned int propDelay = 0;
                unsigned int bandwidth = 0;

                EigrpMetric metric = {0};
                float reportedDistance = 0;
                float feasibleDistance = 0;
                BOOL routingTableMayChange = FALSE;
                ListItem* newTopologyEntry = NULL;

                // Read IP-ROUTE statement:
                // syntex is
                // IP-ROUTE <prefix> <mask> <address>
                // where prefix is the address of the network you are
                // creating a route for, mask is the mask for this network,
                // and address is the IP address of the interface you are
                // routing the packets to. For all of the eigrp routers
                // except the exit router (the one connected to the core)
                // use the following command to create the default route.
                // ip route 0.0.0.0 0.0.0.0 156.156.15.1
                sscanf(currentLine, "%*s%s%s%s", prefix, mask, network);

                prefixAddr = EigrpParseIpAddress(prefix);
                subnetMask = EigrpParseIpAddress(mask);
                interfaceAddress = EigrpParseIpAddress(network);

                interfaceIndex = NetworkIpGetInterfaceIndexFromAddress(
                                     node,
                                     interfaceAddress);

                if (interfaceIndex < 0)
                {
                    char errStr[MAX_STRING_LENGTH] = {0};

                    sprintf(errStr, "The network address %s "
                            "given is not connected with the node %u \n",
                            network, node->nodeId);
                    ERROR_ReportError(errStr);
                }

                bandwidth = EigrpGetInvBandwidth(node, interfaceIndex);

                propDelay = (unsigned int)
                    (EigrpGetPropDelay(node, interfaceIndex)
                        / MICRO_SECOND);

                // Build up an routing metric entry.
                metric.nextHop = 0;
                metric.delay = propDelay;
                metric.bandwidth = bandwidth;
                memset(&metric.mtu, 0, (sizeof(char) * 3));
                metric.hopCount = 0;
                metric.reliability = 1;
                metric.load = 0;
                memset(&metric.reserved, 0, sizeof(unsigned char));
                metric.ipPrefix = (unsigned char) (32 -
                    ConvertSubnetMaskToNumHostBits(subnetMask));

                metric.destination[2] = ((unsigned char)
                    (0xFF & (prefixAddr >> 24)));
                metric.destination[1] = ((unsigned char)
                    (0xFF & (prefixAddr >> 16)));
                metric.destination[0] = ((unsigned char)
                    (0xFF & (prefixAddr >> 8)));

                reportedDistance = EigrpCalculateMetric(
                    &metric, 1, 0, 1, 0, 0);

                feasibleDistance = reportedDistance;

                newTopologyEntry = EigrpEnterRouteIntoTopologyTable(
                                       node,
                                       *eigrp,
                                       DIRECTLY_CONNECTED_NETWORK,
                                       interfaceIndex,
                                       metric,
                                       reportedDistance,
                                       feasibleDistance,
                                       &routingTableMayChange,
                                       (*eigrp)->asId);

                if (routingTableMayChange == TRUE)
                {
                    EigrpCallDualDecisionProcess(
                        node,
                        *eigrp,
                        metric,
                        metric.ipPrefix,
                        interfaceIndex,
                        newTopologyEntry,
                        FALSE);
                }
            }
            else if (strcmp(token, "NO-AUTO-SUMMARY") == 0)
            {
                // Set EIGRP auto summary flag OFF which
                // was by default ON.
                (*eigrp)->isSummaryOn = FALSE;
            }
            else if (strcmp(token, "AUTO-SUMMARY") == 0)
            {
                // Set EIGRP auto summary flag ON
                (*eigrp)->isSummaryOn = TRUE;
            }
            else
            {
                char buf[MAX_STRING_LENGTH] = {0};
                sprintf(buf, "Unknown string in EIGRP configuration File"
                     ": %s\nThe input you have given is:\n\" %s \"\n",
                     token,
                     currentLine);
                ERROR_ReportWarning(buf);
            }
            i++;
            currentLine = eigrpConfigFile->inputStrings[i];

            if (i < eigrpConfigFile->numLines)
            {
                sscanf(currentLine, "%s", token);
            }
        } // end of reading of one node.
    } // end while (i < eigrpConfigFile->numLines)
} // end EigrpReadFromConfigurationFile()


//-------------------------------------------------------------------------
// FUNCTION     : EigrpSendHelloPacket()
//
// PURPOSE      : Sending unicast hello packets
//
// PARAMETERS   : node - node which is sending the hello message
//                eigrp - the eigrp routing structure
//                interfaceindex - interface index through message has
//                                 to be sent
// RETURN VALUE : void
//
// ASSUMPTION   : Eigrp hello packets contains EIGRP header, EIGRP TLV
//                and a Hold Time. Hold time is 4-byte long
//-------------------------------------------------------------------------
static
void EigrpSendHelloPacket(
     Node* node,
     RoutingEigrp* eigrp,
     int interfaceIndex)
{
    Message* helloMsg = NULL;

    PacketBuffer* helloPacket =
        BUFFER_AllocatePacketBuffer(
            0,     // Initial size
            (EIGRP_HOLD_TIME_SIZE + sizeof(EigrpTlv) + EIGRP_HEADER_SIZE),
            FALSE,
            NULL);

    // Build TLV header for hello packet
    EigrpTlv helloTlv = {EIGRP_HELLO_HOLD_TIME_TLV_TYPE,
                         EIGRP_HOLD_TIME_SIZE};

    unsigned int holdTime = (unsigned int)
        (eigrp->neighborTable[interfaceIndex].holdTime / SECOND);

    // Add TLV header to hello packet
    EigrpAddDataToPacketBuffer(
        helloPacket,
        EIGRP_HOLD_TIME_SIZE,
        (char*) &holdTime);

    // Eigrp add data to hello packet. Data part only contains hold
    // time. We assume hold time is in second and a 4-byte integer
    EigrpAddDataToPacketBuffer(
        helloPacket,
        sizeof(EigrpTlv),
        (char*) &helloTlv);

    if (DEBUG_UNICAST_UPDATE)
    {
        if (EigrpGetFlagField(&(eigrp->neighborTable[interfaceIndex])) ==
            EIGRP_FLAG_INITALLIZATION_BIT_ON)
        {
           printf("#node %u sending hello with initialization bit on\n",
                  node->nodeId);
        }
    }

    // Add EIGRP Header to hello packet.
    EigrpAddHeaderToPacketBuffer(
        helloPacket,
        EIGRP_HELLO_OR_ACK_PACKET,
        EIGRP_VERSION,
        EigrpGetFlagField(&(eigrp->neighborTable[interfaceIndex])),
        EigrpIncSeqNum(&(eigrp->neighborTable[interfaceIndex])),
        (unsigned short) eigrp->asId);

    // Encapsulate the hello packet in the message.
    EigrpCopyBufferIntoMessage(node, helloPacket, &helloMsg);

    // send packet to mac layer.
    NetworkIpSendRawMessageToMacLayer(
        node,
        helloMsg,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        EIGRP_ROUTER_MULTICAST_ADDRESS,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_EIGRP,
        ONE_HOP_TTL,
        interfaceIndex,
        ANY_DEST);

    eigrp->eigrpStat->numHelloSend++;
    BUFFER_FreePacketBuffer(helloPacket);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpSendRoutingUpdate()
//
// PURPOSE      : sending routing update message or query message
//
// PARAMETERS   : node - node sending the update.
//                eigrp - the eigrp routing structure
//                addrType - wants address type unicast or multicast
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
static
void EigrpSendRoutingUpdate(
    Node* node,
    RoutingEigrp* eigrp,
    NodeAddress destinationAddress)
{
    int i = 0, j = 0, k = 0;
    // Allocate buffer space for external route.
    PacketBuffer* packetBufferExternRoute =
        BUFFER_AllocatePacketBuffer(
            0,         // Initial size
            (EIGRP_MAX_ROUTE_ADVERTISED *
                 sizeof(EigrpUpdateMessage)),
            FALSE,
            NULL);

    // Allocate buffer space for internal and system route.
    PacketBuffer* packetBufferSystemRoute =
        BUFFER_AllocatePacketBuffer(
            0,         // Initial size
            (EIGRP_MAX_ROUTE_ADVERTISED *
                 sizeof(EigrpUpdateMessage)),
            FALSE,
            NULL);

    AccessListFilterType filterType = ACCESS_LIST_PERMIT;
    EigrpFilterListTable* filterTable = &(eigrp->eigrpFilterListTable);
    EigrpExternMetric externRoute = {0};

    for (i = 0; i < node->numberInterfaces; i++)
    {
        EigrpNeighborInfo* neighbor = NULL;
        NeighborState currentNeighborState = EIGRP_NEIGHBOR_OFF;

        EigrpRoutingTable* eigrpRoutingTable = (EigrpRoutingTable*)
            BUFFER_GetData(&eigrp->eigrpRoutingTable);

        int numTos = BUFFER_GetCurrentSize(&eigrp->eigrpRoutingTable) /
            sizeof(EigrpRoutingTable);

        unsigned int numRoutesInUpdatePacket = 0;
        unsigned int numExternRoute = 0;
        unsigned int numSystemRoute = 0;

        if (destinationAddress != EIGRP_ROUTER_MULTICAST_ADDRESS)
        {
            // Here destination Address must be a valid neighbor address.
            neighbor = EigrpGetNeighbor(&(eigrp->neighborTable[i]),
                           destinationAddress);
            if (!neighbor)
            {
                continue;
            }
            else
            {
                currentNeighborState = neighbor->state;
            }
        }
        else
        {
            currentNeighborState = EIGRP_NEIGHBOR_ACTIVE;
        }

        for (j = 0; j < numTos; j++)
        {
            EigrpRoutingTableEntry* eigrpRoutingEntry =
                (EigrpRoutingTableEntry*)
                    BUFFER_GetData(&eigrpRoutingTable[j].
                        eigrpRoutingTableEntry);

            int numEntries = BUFFER_GetCurrentSize(&eigrpRoutingTable[j].
                eigrpRoutingTableEntry) / sizeof(EigrpRoutingTableEntry);

            if (currentNeighborState == EIGRP_NEIGHBOR_CONNECTION_OPENED)
            {
                // It means this neighbor newly joined your neighbor list
                // so advertise entire routing table to it
                for (k = 0; k < numEntries; k++)
                {
                    unsigned int delay = 0;
                    EigrpMetric* route = &(((EigrpTopologyTableEntry*)
                        eigrpRoutingEntry[k].feasibleRoute->data)->metric);

                    unsigned char routeType = (((EigrpTopologyTableEntry*)
                        eigrpRoutingEntry[k].feasibleRoute->data)->
                            entryType);

                    // THIS SEGMENT OF CODE HAS BEEN ADDED FOR DEFAULT ROUTE
                    BOOL isDefaultRoute = (((EigrpTopologyTableEntry*)
                        eigrpRoutingEntry[k].feasibleRoute->data)->
                           isDefaultRoute);
                    // END PATCH-CODE DEFAULT ROUTE

                    delay = route->delay;

                    if (delay == EIGRP_DESTINATION_INACCESSABLE)
                    {
                        continue;
                    }

                    if (eigrpRoutingEntry[k].outgoingInterface == i)
                    {
                        if (eigrp->splitHorizonEnabled)
                        {
                            continue;
                        }

                        if (eigrp->poisonReverseEnabled)
                        {
                            if (NetworkIpIsWiredBroadcastNetwork(node, i))
                            {
                                continue;
                            }
                            // do route poisoning
                            route->delay = EIGRP_DESTINATION_INACCESSABLE;
                            route->hopCount = UCHAR_MAX;
                        }
                    }

                    if (!EigrpRouteSummarizationProssesResult(
                            eigrp,
                            (EigrpTopologyTableEntry*)
                            (eigrpRoutingEntry[k].feasibleRoute->data)))
                    {

                        NodeAddress addressToCheck =
                            EigrpConvertCharArrayToInt(route->destination);

                        if (IsInADifferentMajorNetwork(
                               node,
                               (addressToCheck << 8),
                               route->ipPrefix,
                               i))
                        {
                           continue;
                        }
                    }

                    // ROUTE FILTERING:
                    //
                    // Filter the route if ACCESS-LIST imposes restriction
                    // to accept this route.
                    //
                    // Route filters can be inserted to influence which routes
                    // a router is willing to accept from its neighbors or
                    // which routes the router is willing to propagate
                    // to its neighbors.
                    //
                    // To configure EIGRP route filters, the IP access list
                    // (ACL) used in all the commands can be numbered
                    // or named simple IP access list.
                    filterType = EigrpReturnACLPermission(
                                     node,
                                     filterTable,
                                     (EigrpConvertCharArrayToInt(
                                         route->destination) << 8),
                                     ACCESS_LIST_OUT,
                                     i);

                    if (EIGRP_FILTER_DEBUG)
                    {
                        char addrStr[MAX_STRING_LENGTH] = {0};
                        const char* permission = NULL;

                        IO_ConvertIpAddressToString(
                            (EigrpConvertCharArrayToInt(
                                 route->destination) << 8),
                            addrStr);

                        if (filterType == ACCESS_LIST_PERMIT)
                        {
                            permission = (char *)"ACCESS_LIST_PERMIT";
                        }
                        else if (filterType == ACCESS_LIST_DENY)
                        {
                            permission = (char *)"ACCESS_LIST_DENY";
                        }
                        else
                        {
                            ERROR_Assert(FALSE, "Unknown permission type");
                        }

                        printf("node = %d Addr = %s permission = %s"
                               "Interface index = %d\n",
                               node->nodeId,
                               addrStr,
                               permission,
                               i);
                    }

                    if (filterType == ACCESS_LIST_DENY)
                    {
                        continue;
                    }

                    // EIGRP tracks default candidate routes in
                    // the external section of its routing updates
                    if (isDefaultRoute == TRUE)
                    {
                        routeType = EIGRP_EXTERIOR_ROUTE;
                    }

                    // If route is exterior route build
                    // Exterior route metric
                    if (routeType == EIGRP_EXTERIOR_ROUTE)
                    {
                        EigrpBuildExternalRoute(
                            ((EigrpTopologyTableEntry*) eigrpRoutingEntry[k].
                                 feasibleRoute->data),
                            route,
                            &externRoute);
                        route = (EigrpMetric*) &externRoute;
                    }

                    EigrpAddRouteToAppropriatePacketBuffer(
                        packetBufferExternRoute,
                        packetBufferSystemRoute,
                        route,
                        routeType,
                        &numSystemRoute,
                        &numExternRoute);

                    numRoutesInUpdatePacket++;

                    EigrpCheckBufferAndSend(
                        node,
                        eigrp,
                        packetBufferSystemRoute,
                        &numSystemRoute,
                        packetBufferExternRoute,
                        &numExternRoute,
                        i,
                        &(eigrp->eigrpStat->numTriggeredUpdates),
                        &numRoutesInUpdatePacket,
                        EIGRP_UPDATE_PACKET,
                        destinationAddress);
                } // end for (k = 0; k < numEntries; k++)
            } // end if (neighbor->state == EIGRP_NEIGHBOR_OPENED)
            else if (currentNeighborState == EIGRP_NEIGHBOR_ACTIVE)
            {
                // If your neighbor is on advertise only those route
                // that just currently has been updated and not
                // previously advertised
                for (k = 0; k < numEntries; k++)
                {
                    if (eigrpRoutingEntry[k].lastUpdateTime >=
                        eigrp->lastUpdateSent)
                    {
                        EigrpMetric* route =
                            &(((EigrpTopologyTableEntry*)
                                eigrpRoutingEntry[k].feasibleRoute->
                                    data)->metric);

                        unsigned char routeType =
                            ((EigrpTopologyTableEntry*)
                                eigrpRoutingEntry[k].feasibleRoute->
                                    data)->entryType;

                        BOOL isDefaultRoute = (((EigrpTopologyTableEntry*)
                           eigrpRoutingEntry[k].feasibleRoute->data)->
                               isDefaultRoute);

                        if (eigrpRoutingEntry[k].outgoingInterface == i)
                        {
                            if (eigrp->splitHorizonEnabled)
                            {
                                continue;
                            }

                            if (eigrp->poisonReverseEnabled)
                            {
                               if (NetworkIpIsWiredBroadcastNetwork(node, i))
                                {
                                    continue;
                                }
                                // Do route poisoning
                                route->delay =
                                   EIGRP_DESTINATION_INACCESSABLE;
                                route->hopCount = UCHAR_MAX;
                            }
                        }

                        if (!EigrpRouteSummarizationProssesResult(
                                eigrp,
                                (EigrpTopologyTableEntry*)
                                (eigrpRoutingEntry[k].feasibleRoute->data)))
                        {

                            NodeAddress addressToCheck =
                                EigrpConvertCharArrayToInt(
                                    route->destination);

                            if (IsInADifferentMajorNetwork(
                                   node,
                                   (addressToCheck << 8),
                                   route->ipPrefix,
                                   i))
                            {
                               continue;
                            }
                        }

                        // ROUTE FILTERING:
                        //
                        // Filter the route if ACCESS-LIST imposes restriction
                        // to accept this route.
                        //
                        // Route filters can be inserted to influence which routes
                        // a router is willing to accept from its neighbors or
                        // which routes the router is willing to propagate
                        // to its neighbors.
                        //
                        // To configure EIGRP route filters, the IP access list
                        // (ACL) used in all the commands can be numbered
                        // or named simple IP access list.
                        filterType = EigrpReturnACLPermission(
                                         node,
                                         filterTable,
                                         (EigrpConvertCharArrayToInt(
                                             route->destination) << 8),
                                         ACCESS_LIST_OUT,
                                         i);

                        if (EIGRP_FILTER_DEBUG)
                        {
                            char addrStr[MAX_STRING_LENGTH] = {0};
                            const char* permission = NULL;

                            IO_ConvertIpAddressToString(
                                (EigrpConvertCharArrayToInt(
                                     route->destination) << 8),
                                addrStr);

                            if (filterType == ACCESS_LIST_PERMIT)
                            {
                                permission = (char *)"ACCESS_LIST_PERMIT";
                            }
                            else if (filterType == ACCESS_LIST_DENY)
                            {
                                permission = (char *)"ACCESS_LIST_DENY";
                            }
                            else
                            {
                                ERROR_Assert(FALSE, "Unknown permission type");
                            }

                            printf("**node = %d Addr %s permission = %s "
                                   "Interface index = %d\n",
                                   node->nodeId,
                                   addrStr,
                                   permission,
                                   i);
                        }

                        if (filterType == ACCESS_LIST_DENY)
                        {
                            continue;
                        }

                        // EIGRP tracks default routes in the external
                        // section of its routing updates
                        if (isDefaultRoute == TRUE)
                        {
                            routeType = EIGRP_EXTERIOR_ROUTE;
                        }

                        // If route is exterior route build
                        // Exterior route metric
                        if (routeType == EIGRP_EXTERIOR_ROUTE)
                        {
                            PrintTopologyTable(node);
                            EigrpBuildExternalRoute(
                                ((EigrpTopologyTableEntry*) eigrpRoutingEntry[k].
                                     feasibleRoute->data),
                                route,
                                &externRoute);

                            route = (EigrpMetric*) &externRoute;

                        }

                        EigrpAddRouteToAppropriatePacketBuffer(
                            packetBufferExternRoute,
                            packetBufferSystemRoute,
                            route,
                            routeType,
                            &numSystemRoute,
                            &numExternRoute);

                        numRoutesInUpdatePacket++;

                        EigrpCheckBufferAndSend(
                            node,
                            eigrp,
                            packetBufferSystemRoute,
                            &numSystemRoute,
                            packetBufferExternRoute,
                            &numExternRoute,
                            i,
                            &(eigrp->eigrpStat->numTriggeredUpdates),
                            &numRoutesInUpdatePacket,
                            EIGRP_UPDATE_PACKET,
                            destinationAddress);
                    } // end if (...)
                } // end for (k = 0; k < numEntries; k++)
            } // else if (neighbor->state == EIGRP_NEIGHBOR_ACTIVE)
        } // for (j = 0; j < numTos; j++)

        if (numRoutesInUpdatePacket > 0)
        {
            EigrpAddHeaderAndSendPacketToMacLayer(
                node,
                eigrp,
                packetBufferSystemRoute,
                numSystemRoute,
                packetBufferExternRoute,
                numExternRoute,
                i,
                EIGRP_UPDATE_PACKET,
                destinationAddress);

            eigrp->eigrpStat->numTriggeredUpdates++;

            BUFFER_ClearPacketBufferData(packetBufferExternRoute);
            BUFFER_ClearPacketBufferData(packetBufferSystemRoute);
        }
    } // end for (i = 0; i < node->numberInterfaces; i++)
    BUFFER_FreePacketBuffer(packetBufferExternRoute);
    BUFFER_FreePacketBuffer(packetBufferSystemRoute);
} // end EigrpSendRoutingUpdate()


//--------------------------------------------------------------------------
// FUNCTION     : EigrpCalculateNewPathMetricViaSource()
//
// PURPOSE      : calculating the vector metric of a new incoming path
//
// PAPAMETER    : node - node which is calculating the path metric
//                vectorMetric - vector metric of incoming path
//                interfaceIndex - interface index through which the new
//                                 path information has been received
//
// RETURN VALUE : pointer to the newly calculated vector metric
//--------------------------------------------------------------------------
static
EigrpMetric* EigrpCalculateNewPathMetricViaSource(
    Node* node,
    EigrpMetric* vectorMetric,
    int interfaceIndex)
{
    EigrpMetric* newVector = (EigrpMetric*)
        MEM_malloc(sizeof(EigrpMetric));
    unsigned int bandwidth = 0;
    clocktype propDelay = 0;

    memcpy(newVector->destination, vectorMetric->destination, 3);

    // get minimum bandwidth.
    bandwidth = MAX(((unsigned) EigrpGetInvBandwidth(node,interfaceIndex)),
                    (vectorMetric->bandwidth));

    newVector->bandwidth = bandwidth;

    if (vectorMetric->delay != EIGRP_DESTINATION_INACCESSABLE)
    {
        propDelay = ((EigrpGetPropDelay(node, interfaceIndex) / MICRO_SECOND)
                + vectorMetric->delay);

        newVector->delay = (unsigned int) propDelay;
    }
    else
    {
        newVector->delay = EIGRP_DESTINATION_INACCESSABLE;
    }

    newVector->reliability = (unsigned char)
        MIN(1, vectorMetric->reliability);

    // mtu is currently an unused field.
    memcpy(newVector->mtu, vectorMetric->mtu, 3);

    // load is currently an unused field.
    newVector->load = (unsigned char) MAX(1, vectorMetric->load);

    newVector->hopCount = (unsigned char) (vectorMetric->hopCount + 1);
    return newVector;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpScheduleUpdateMessage()
//
// PURPOSE      : scheduling/delaying the next update message.
//
// PARAMETERS   : node - node which is sending the update message
//                eigrp - the eigrp routing structure
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
static
void EigrpScheduleUpdateMessage(Node* node, RoutingEigrp* eigrp)
{
    if (eigrp->isNextUpdateScheduled == FALSE)
    {
        Message* msg = MESSAGE_Alloc(
                           node,
                           NETWORK_LAYER,
                           ROUTING_PROTOCOL_EIGRP,
                           MSG_ROUTING_EigrpRiseUpdateAlarm);

        eigrp->isNextUpdateScheduled = TRUE;
        MESSAGE_Send(node, msg, eigrp->sleepTime + RANDOM_nrand(eigrp->seed));
    }
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpRestartHello()
//
// PURPOSE      : Restart hello messaging after stuck in active timer
//                has expired
//
// PARAMETERS   : node - node which is sending hello message
//                eigrp - eigrp routing structure
//                msg - self timer message
//                interfaceIndex - the interface index suffring stuck in
//                                  active timeout
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpRestartHello(
    Node* node,
    RoutingEigrp* eigrp,
    Message* msg,
    int interfaceIndex)
{
    NeighborSearchInfo* neighborSearchInfo =
        (NeighborSearchInfo*) MESSAGE_ReturnInfo(msg);

    EigrpNeighborTable* neighborTable =
        &eigrp->neighborTable[neighborSearchInfo->tableIndex];


    neighborTable->seqNo = (EIGRP_MIN_SEQ_NUM - 1);

    EigrpSendHelloPacket(node, eigrp, interfaceIndex);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpHandleRestartHello()
//
// PURPOSE      : Restart hello messaging by calling EigrpRestartHello()
//
// PARAMETERS   : node - node which is sending hello message
//                interfaceIndex - the interface index suffring stuck in
//                                 active timeout
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpHandleRestartHello(
    Node* node,
    int interfaceIndex)
{
    RoutingEigrp* eigrp = (RoutingEigrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_EIGRP);

    if (eigrp != NULL)
    {
        EigrpNeighborTable* neighborTable = &eigrp->
            neighborTable[interfaceIndex];

        NeighborSearchInfo neighborSearchInfo = {ANY_INTERFACE, ANY_DEST};
        EigrpNeighborInfo* temp = neighborTable->first;

        while (temp)
        {
            Message* msg = MESSAGE_Alloc(
                               node,
                               NETWORK_LAYER,
                               ROUTING_PROTOCOL_EIGRP,
                               MSG_ROUTING_EigrpHoldTimerExpired);
            MESSAGE_InfoAlloc(node, msg, sizeof(NeighborSearchInfo));

            // Set neighborSearch info
            neighborSearchInfo.tableIndex = interfaceIndex;
            neighborSearchInfo.neighborAddress = temp->neighborAddr;
            memcpy(MESSAGE_ReturnInfo(msg), &neighborSearchInfo,
                sizeof(NeighborSearchInfo));

            // Do the same processing as if Hold timer  expired
            EigrpRestartHello(node, eigrp, msg, interfaceIndex);
            temp = temp->next;
        } // end while (temp)
    } // end if (eigrp != NULL)
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpProcessHoldTimeExpired()
//
// PURPOSE      : Handling Hold Timer Expired event
//
// PARAMETERS   : node - node which is handling the event
//                msg - the event it is handling
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
static
void EigrpProcessHoldTimeExpired(
    Node* node,
    RoutingEigrp* eigrp,
    Message* msg)
{
    NeighborSearchInfo* neighborSearchInfo =
        (NeighborSearchInfo*) MESSAGE_ReturnInfo(msg);

    EigrpNeighborTable* neighborTable =
        &eigrp->neighborTable[neighborSearchInfo->tableIndex];

    EigrpNeighborInfo* neighbor = EigrpGetNeighbor(neighborTable,
        neighborSearchInfo->neighborAddress);

    if ((neighbor->isHelloOrAnyOtherPacketReceived == TRUE) &&
         NetworkIpInterfaceIsEnabled(node, neighborSearchInfo->tableIndex))
    {
        // neighbor is alive. So again re-start the
        // hold timer for that neighbor
        if (neighbor->neighborHoldTime)
        {
            MESSAGE_Send(node, msg, neighbor->neighborHoldTime);
            neighbor->isHelloOrAnyOtherPacketReceived = FALSE;
        }
    }
    else
    {
        // If you reach this block of code it means : Currently your
        // neighboring eigrp router is dead.
        ListItem* temp = NULL;

        // set the state of the neighbor as OFF
        EigrpSetNeighborState(neighbor, EIGRP_NEIGHBOR_OFF);

        temp = eigrp->topologyTable->first;
        while (temp)
        {
            NodeAddress destAddress = EigrpConvertCharArrayToInt(
                ((EigrpTopologyTableEntry*) (temp->data))->metric.
                    destination) << 8;

            NodeAddress nextHop = ((EigrpTopologyTableEntry*)
                (temp->data))->metric.nextHop;

            EigrpTopologyTableEntry* topologyEntry =
                (EigrpTopologyTableEntry*) temp->data;

            int outgoingInterface = ((EigrpTopologyTableEntry*)
                (temp->data))->outgoingInterface;

            if ((neighbor->neighborAddr == destAddress) ||
                (neighbor->neighborAddr == nextHop))
            {
                BOOL routingTableMayChange = FALSE;
                unsigned int  notUsed = 0;
                routingTableMayChange = (BOOL) (outgoingInterface ==
                    topologyEntry->outgoingInterface);

                // Make the entry unreachable in the topology table.
                topologyEntry->metric.delay =
                    EIGRP_DESTINATION_INACCESSABLE;

                if (routingTableMayChange)
                {
                    EigrpCallDualDecisionProcess(
                        node,
                        eigrp,
                        topologyEntry->metric,
                        topologyEntry->metric.ipPrefix,
                        (unsigned short) notUsed,
                        temp,
                        FALSE);
                }
                topologyEntry->isOptimum = FALSE;
            } // end if (...)
            temp = temp->next;
        } // end while (temp)
        neighbor->neighborHoldTime = 0;

        if (!NetworkIpInterfaceIsEnabled(node, neighborSearchInfo->tableIndex))

        {
            // Set sequence number to zero
            neighborTable->seqNo = (EIGRP_MIN_SEQ_NUM - 1);
            if (DEBUG_UNICAST_UPDATE)
            {
                printf("#node %u is setting seq number to %d for"
                       "failed interface index %d\n",
                       node->nodeId,
                       neighborTable->seqNo,
                       neighborSearchInfo->tableIndex);
            }
        }
        // and free the hold timer.
        MESSAGE_Free(node, msg);
    }
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpSendDestUnrechable()
//
// PURPOSE      : Sending a destination unreachable reply
//
// PARAMETERS   : node - node which is sending destination unreachable reply
//                eigrp - pointer to eigrp data.
//                querySource - the source of the query message [this
//                              function reply back to the query source
//                              "destination unreachable"]
//                interfaceIndex - the interface index thru which query
//                                 message is appeared.
//                unreachableDest[] - the unreachable destination address
//                ipPrefix - ip prefix length of the unreachable destination
//                           address
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpSendReplyDestUnrechable(
    Node* node,
    RoutingEigrp* eigrp,
    NodeAddress querySource,
    int interfaceIndex,
    unsigned char unreachableDest[],
    char ipPrefix)
{
    unsigned int numSystemRoute = 0;
    unsigned int numExternRoute = 0;
    int outgoingInterface = ANY_INTERFACE;
    NodeAddress nextHopAddress = ANY_DEST;

    EigrpReplyMessage newReplyPacketEntry = {{0}};

    // Allocate packet buffer for reply packet (External route)
    PacketBuffer* replyPacketBufferExter =
        BUFFER_AllocatePacketBuffer(
            0,         // Initial size
            (EIGRP_MAX_ROUTE_ADVERTISED *
                 sizeof(EigrpReplyMessage)),
            FALSE,
            NULL); // This argument is not used here

    // Allocate packet buffer for reply packet (Internal route)
    PacketBuffer* replyPacketBufferInter =
        BUFFER_AllocatePacketBuffer(
            0,         // Initial size
            (EIGRP_MAX_ROUTE_ADVERTISED *
                 sizeof(EigrpReplyMessage)),
            FALSE,
        NULL); // This argument is not used here

    NodeAddress replySource = NetworkIpGetInterfaceAddress(
                                  node,
                                  interfaceIndex);

    // 1.set next hop address field.
    newReplyPacketEntry.metric.nextHop = replySource;

    // 2.set delay field
    newReplyPacketEntry.metric.delay = EIGRP_DESTINATION_INACCESSABLE;

    // 3.set bandwidth field (an infeasible value)
    newReplyPacketEntry.metric.bandwidth = 0;

    // 4. mtu (initialized to zero)

    // 5. set hop count field (to infinity)
    newReplyPacketEntry.metric.hopCount = UCHAR_MAX;

    // 6. reliability (initialized to zero)
    // 7. load (initialized to zero)
    // 8. reserved (initialized to zero)

    // 9. Ip prefix
    newReplyPacketEntry.metric.ipPrefix = ipPrefix;

    // 10. set unreachabl destination
    memcpy(newReplyPacketEntry.metric.destination,
           unreachableDest,
           3);

    NetworkGetInterfaceAndNextHopFromForwardingTable(
        node,
        querySource,
        &outgoingInterface,
        &nextHopAddress);

    EigrpAddRouteToAppropriatePacketBuffer(
        replyPacketBufferExter,
        replyPacketBufferInter,
        &newReplyPacketEntry.metric,
        EIGRP_SYSTEM_ROUTE,
        &numSystemRoute,
        &numExternRoute);

    if (nextHopAddress != (unsigned) NETWORK_UNREACHABLE)
    {
        EigrpAddHeaderAndSendPacketToMacLayer(
            node,
            eigrp,
            replyPacketBufferInter,
            numSystemRoute,
            replyPacketBufferExter,
            numExternRoute,
            outgoingInterface,
            EIGRP_REPLY_PACKET,
            querySource);

        eigrp->eigrpStat->numResponsesSend++;
    }

    BUFFER_FreePacketBuffer(replyPacketBufferExter);
    BUFFER_FreePacketBuffer(replyPacketBufferInter);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpProcessQueryPacketAndBuildReplyPacket()
//
// PURPOSE      : Processing Query messages.
//
// PARAMETERS   : node - node which is handling the packet
//                eigrp- routing eigrp structure.
//                msg - the packet it is handling
//                sourceAddr - immediate next hop from which is querying
//                interfaceIndex - the interface index through which
//                                 the message is being received.
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpProcessQueryPacketAndBuildReplyPacket(
    Node* node,
    RoutingEigrp* eigrp,
    Message* msg,
    NodeAddress sourceAddr,
    int interfaceIndex)
{
    int i = 0;
    BOOL isFurtherQueryNeeded = FALSE;
    char* packetPtr = MESSAGE_ReturnPacket(msg);
    int initialPacketSize = MESSAGE_ReturnPacketSize(msg);
    int numEntriesInQueryPacket = 0;
    unsigned int numEntriesInReplyPacket = 0;
    unsigned int numSystemRoute = 0;
    unsigned int numExternRoute = 0;
    unsigned short tlvType = EIGRP_NULL_TLV_TYPE;
    EigrpHeader eigrpHeader = {0};
    EigrpReplyMessage newReplyPacketEntry = {{0}};
    NodeAddress querySource = ANY_DEST;
    unsigned char* stratingPtrOfQueryMessageData = NULL;
    unsigned char* endPtrOfQueryMessageData = NULL;

    // Allocate packet buffer for reply packet (External route)
    PacketBuffer* replyPacketBufferExter =
        BUFFER_AllocatePacketBuffer(
            0,         // Initial size
            (EIGRP_MAX_ROUTE_ADVERTISED *
                 sizeof(EigrpReplyMessage)),
            FALSE,
            NULL); // This argument is not used here

    // Allocate packet buffer for reply packet (Internal route)
    PacketBuffer* replyPacketBufferInter =
        BUFFER_AllocatePacketBuffer(
            0,         // Initial size
            (EIGRP_MAX_ROUTE_ADVERTISED *
                 sizeof(EigrpReplyMessage)),
            FALSE,
            NULL); // This argument is not used here


    // Remove EIGRP commom header.
    EigrpRemoveHeader(&packetPtr, initialPacketSize, &eigrpHeader);
    initialPacketSize = (initialPacketSize - sizeof(EigrpHeader));

    // Precalculate query packet size (Total packet - header).
    stratingPtrOfQueryMessageData = (unsigned char*) packetPtr;
    endPtrOfQueryMessageData = (unsigned char*)
        (packetPtr + initialPacketSize);

    // Remove eigrp query TLV.
    tlvType = (unsigned short)
        EigrpPreprocessTLVType(
            &packetPtr,
            initialPacketSize,
            &numEntriesInQueryPacket);
    initialPacketSize = (initialPacketSize - sizeof(EigrpTlv));

    // Extract the query source from the query packet.
    // This will be used to generate unicast reply message.
    EigrpReadNumBytes(&packetPtr, initialPacketSize, sizeof(NodeAddress),
        (char*) &querySource);

    // If query source is your own IP do not process
    if (NetworkIpIsMyIP(node, querySource))
    {
        return;
    }

    // This is the number of entries in the query packet. Exclude
    // The first entry because it contains the query source address.
    numEntriesInQueryPacket = (numEntriesInQueryPacket
        / sizeof(EigrpQueryMessage) - 1);

    if (DEBUG_EIGRP_REPLY)
    {
        char querySourceAddr[MAX_STRING_LENGTH] = {0};
        char forwardedBy[MAX_STRING_LENGTH] = {0};

        IO_ConvertIpAddressToString(querySource, querySourceAddr);
        IO_ConvertIpAddressToString(sourceAddr, forwardedBy);

        printf("Node %d has received a query message from %s\n"
               "forworded by %s num entries in query packet = %u\n",
               node->nodeId,
               querySourceAddr,
               forwardedBy,
               numEntriesInQueryPacket);
        PrintRoutingTable(node);
        PrintTopologyTable(node);
    }

    for (i = 0; i < numEntriesInQueryPacket; i++)
    {
        ListItem* feasibleSucc = NULL;
        ListItem* prevFeasibleEntryThruQuerySource = NULL;
        ListItem* temp = eigrp->topologyTable->first;
        EigrpQueryMessage queryPacketEntry = {{0}};
        BOOL alternativeFeasibleFound = FALSE;
        BOOL isDestAddrFound = FALSE;

        feasibleSucc = NULL;

        EigrpReadNumBytes(
            &packetPtr,
            initialPacketSize,
            sizeof(EigrpQueryMessage),
            (char*)&queryPacketEntry);
        initialPacketSize = (initialPacketSize - sizeof(EigrpQueryMessage));

        ERROR_Assert(initialPacketSize >= 0, "packet size less"
            " than zero is not possible !!!");

        // Searching start in topology table
        while (temp)
        {
            NodeAddress destAddressInQueryPacket = ANY_DEST;

            NodeAddress destAddress = EigrpConvertCharArrayToInt(
                ((EigrpTopologyTableEntry*) (temp->data))->metric.
                    destination);

            NodeAddress nextHop = ((EigrpTopologyTableEntry*)
                (temp->data))->metric.nextHop;

            unsigned int delay = ((EigrpTopologyTableEntry*)
                (temp->data))->metric.delay;

            destAddressInQueryPacket = EigrpConvertCharArrayToInt(
                queryPacketEntry.destination);

            if ((nextHop != sourceAddr) &&
                (destAddress == destAddressInQueryPacket) &&
                (delay != EIGRP_DESTINATION_INACCESSABLE))
            {
                // At this point a feasible successor has
                // been found that is NOT the next-hop / successor
                // from which the query message has been received.

                if (alternativeFeasibleFound == FALSE)
                {
                    alternativeFeasibleFound = TRUE;
                    feasibleSucc = temp;
                }
                else
                {
                    float prevFeasibleDistance =
                        ((EigrpTopologyTableEntry*)
                              feasibleSucc->data)->feasibleDistance;

                    float nextFeasibleDistance =
                        ((EigrpTopologyTableEntry*)
                              temp->data)->feasibleDistance;

                    if (nextFeasibleDistance < prevFeasibleDistance)
                    {
                        feasibleSucc = temp;
                    }
                }
                isDestAddrFound = TRUE;
            }
            else if ((nextHop == sourceAddr) &&
                     (destAddress == destAddressInQueryPacket) &&
                     (delay != EIGRP_DESTINATION_INACCESSABLE))
            {
                // At this point you have find a next hop in the
                // topology table from which you have received a
                // query message :- querying to you for a path to
                // a destination "D". Where "D" was previously
                // reachable via that next hop. So mark that entry
                // as "EIGRP_DESTINATION_INACCESSABLE"
                NodeAddress destAddress = ANY_DEST;

                ((EigrpTopologyTableEntry*) temp->data)->metric.delay =
                    EIGRP_DESTINATION_INACCESSABLE;

                destAddress = EigrpConvertCharArrayToInt(
                    ((EigrpTopologyTableEntry*) temp->data)->metric.
                        destination);

                NetworkUpdateForwardingTable(
                    node,
                    (destAddress << 8),
                    ConvertNumHostBitsToSubnetMask(32
                        - ((EigrpTopologyTableEntry*) temp->data)->
                              metric.ipPrefix),
                    (NodeAddress) NETWORK_UNREACHABLE,
                    ANY_INTERFACE,
                    -1, // may need change
                    ROUTING_PROTOCOL_EIGRP);

                prevFeasibleEntryThruQuerySource = temp;
                isDestAddrFound = TRUE;
            }
            temp = temp->next;
        } // while (temp)
        // Searching end in topology table

        // ROUTE FILTERING:
        //
        // Filter the route if ACCESS-LIST imposes restriction
        // to accept this route.
        //
        // Route filters can be inserted to influence which routes
        // a router is willing to accept from its neighbors or
        // which routes the router is willing to propagate
        // to its neighbors.
        //
        // To configure EIGRP route filters, the IP access list
        // (ACL) used in all the commands can be numbered
        // or named simple IP access list.

        if (isDestAddrFound == FALSE)
        {
            // The destination address searched is not in the
            // topology table. So fire back with "destination
            // unreachable" reply.
            EigrpSendReplyDestUnrechable(
                node,
                eigrp,
                querySource,
                interfaceIndex,
                queryPacketEntry.destination,
                queryPacketEntry.ipPrefix);
            continue;
        }

        if (feasibleSucc != NULL)
        {
            // Build up a reply packet entry, with a feasible route
            // to unicast it to the query source.
            EigrpMetric* replyPath = NULL;

            EigrpExternMetric externRoute = {0};
            BOOL isDefaultRoute = ((EigrpTopologyTableEntry*)
                (feasibleSucc->data))->isDefaultRoute;

            NodeAddress replySource = NetworkIpGetInterfaceAddress(
                                          node,
                                          interfaceIndex);

            unsigned char routeType = ((EigrpTopologyTableEntry*)
                (feasibleSucc->data))->entryType;

            newReplyPacketEntry.metric = ((EigrpTopologyTableEntry*)
                (feasibleSucc->data))->metric;

            // Update network forwarding table
            NetworkUpdateForwardingTable(
                node,
                (EigrpConvertCharArrayToInt(
                     newReplyPacketEntry.metric.destination) << 8),
                ConvertNumHostBitsToSubnetMask(
                    32 - ((EigrpTopologyTableEntry*) feasibleSucc->
                         data)->metric.ipPrefix),
                ((EigrpTopologyTableEntry*) feasibleSucc->
                    data)->metric.nextHop,
                ((EigrpTopologyTableEntry*) feasibleSucc->
                    data)->outgoingInterface,
                ((EigrpTopologyTableEntry*) feasibleSucc->
                    data)->metric.hopCount,
                ROUTING_PROTOCOL_EIGRP);

            // Set the next hop field of the metric as reply source
            // such that originator of the query packet identifies
            // the reply-maker node.
            newReplyPacketEntry.metric.nextHop = replySource;
            replyPath = &(newReplyPacketEntry.metric);

            // EIGRP tracks default routes in the external
            // section of its routing updates
            if (isDefaultRoute == TRUE)
            {
                routeType = EIGRP_EXTERIOR_ROUTE;
            }

            // If route is exterior route build
            // Exterior route metric
            if (routeType == EIGRP_EXTERIOR_ROUTE)
            {
                EigrpBuildExternalRoute(
                    ((EigrpTopologyTableEntry*) (feasibleSucc->data)),
                    &(newReplyPacketEntry.metric),
                    &externRoute);

                replyPath = (EigrpMetric*) &externRoute;;
            }

            EigrpAddRouteToAppropriatePacketBuffer(
                replyPacketBufferExter,
                replyPacketBufferInter,
                replyPath,
                routeType,
                &numSystemRoute,
                &numExternRoute);

            numEntriesInReplyPacket++;
        }
        else if (prevFeasibleEntryThruQuerySource != NULL)
        {
            // No feasible successor found you have marked this
            // destination as  EIGRP_DESTINATION_INACCESSABLE in your
            // routing table. So you need to send a query message.
            isFurtherQueryNeeded = TRUE;
        }

        if ((feasibleSucc != NULL) &&
            (prevFeasibleEntryThruQuerySource != NULL))
        {
            int j = 0;
            EigrpRoutingTable* eigrpRoutingTable = (EigrpRoutingTable*)
                BUFFER_GetData(&eigrp->eigrpRoutingTable);

            int numTos = BUFFER_GetCurrentSize(&eigrp->eigrpRoutingTable)
                             / sizeof(EigrpRoutingTable);

            // Change the routing table entries...
            for (j = 0; j < numTos; j++)
            {
                int k = 0;
                int numEntries = BUFFER_GetCurrentSize(
                    &eigrpRoutingTable[j].eigrpRoutingTableEntry)
                        / sizeof(EigrpRoutingTableEntry);

                EigrpRoutingTableEntry* eigrpRoutingTableEntry =
                    (EigrpRoutingTableEntry*) BUFFER_GetData(
                        &eigrpRoutingTable[j].eigrpRoutingTableEntry);

                for (k = 0; k < numEntries; k++)
                {
                    if (eigrpRoutingTableEntry[k].feasibleRoute ==
                        prevFeasibleEntryThruQuerySource)
                    {
                        ((EigrpTopologyTableEntry*) feasibleSucc->data)->
                            isOptimum = TRUE;

                        eigrpRoutingTableEntry[k].feasibleRoute =
                            feasibleSucc;

                        // Routing table updated so break out
                        // of the routing update loop.
                        break;
                    }
                } // for (k = 0; k < numEntries; k++)
            } // for (j = 0; j < numTos; j++)
        } // if ((feasibleSucc != NULL) &&
          // (prevFeasibleEntryThruQuerySource != NULL))
    } // end for (i = 0; i < numEntriesInQueryPacket; i++)

    if (isFurtherQueryNeeded == TRUE)
    {
        int i = 0;
        char incomingQueryPacket[MAX_QUERY_PACKET_SIZE] = {0};

        memcpy(incomingQueryPacket,
            stratingPtrOfQueryMessageData,
            (endPtrOfQueryMessageData - stratingPtrOfQueryMessageData));

        // FeasibleSucc NULL means you have no feasible entry
        // for the query you have received. Send query message
        // to all other neighbor except from whom you have received it.
        for (i = 0; i < node->numberInterfaces; i++)
        {
             if (
                ! NetworkIpInterfaceIsEnabled(node, i) || (i == interfaceIndex)
                 || (NetworkIpGetUnicastRoutingProtocolType(node, i)
                          != ROUTING_PROTOCOL_EIGRP)
               )
            {
                continue;
            }

            //Type casted message size to int to support 64 bit compilation

            EigrpSendQueryPacket(
                    node,
                    eigrp,
                    i,
                    (EigrpQueryMessage*) incomingQueryPacket,
                    (int)(endPtrOfQueryMessageData
                      - stratingPtrOfQueryMessageData),
                    EIGRP_QUERY_FORWORD);

        }
    }

    if (numEntriesInReplyPacket > 0)
    {
        int outgoingInterface = ANY_INTERFACE;
        NodeAddress nextHopAddress = ANY_DEST;

        NetworkGetInterfaceAndNextHopFromForwardingTable(
            node,
            querySource,
            &outgoingInterface,
            &nextHopAddress);

        if (nextHopAddress != (unsigned) NETWORK_UNREACHABLE)
        {
            EigrpAddHeaderAndSendPacketToMacLayer(
                node,
                eigrp,
                replyPacketBufferInter,
                numSystemRoute,
                replyPacketBufferExter,
                numExternRoute,
                interfaceIndex,
                EIGRP_REPLY_PACKET,
                querySource);
            eigrp->eigrpStat->numResponsesSend++;

            if (DEBUG_EIGRP_REPLY)
            {
                char querySourceAddr[MAX_STRING_LENGTH] = {0};
                char forwardedBy[MAX_STRING_LENGTH] = {0};

                IO_ConvertIpAddressToString(querySource, querySourceAddr);
                IO_ConvertIpAddressToString(sourceAddr, forwardedBy);

                printf("Node %d has received a query message from %s\n"
                       "forworded by %s And sending a reply back to %s\n",
                       node->nodeId,
                       querySourceAddr,
                       forwardedBy,
                       querySourceAddr);
            }
        }
    }
    BUFFER_FreePacketBuffer(replyPacketBufferExter);
    BUFFER_FreePacketBuffer(replyPacketBufferInter);

    // Increment stat var.
    eigrp->eigrpStat->numQueryReceived++;
    EigrpUpdatePacketReceivedStatus(eigrp->neighborTable, sourceAddr);

    // Schedule routing update
    EigrpScheduleUpdateMessage(node, eigrp);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpProcessUpdatePacket()
//
// PURPOSE      : Processing hello message or acknowledgement messages.
//
// PARAMETERS   : node - node which is handling the packet
//                eigrp- routing eigrp structure.
//                msg - the packet it is handling
//                sourceAddr - immediate next hop from which
//                             message being received
//                interfaceIndex - the interface index through which
//                                 the message is being received.
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpProcessUpdatePacket(
    Node* node,
    RoutingEigrp* eigrp,
    Message* msg,
    NodeAddress sourceAddr,
    int interfaceIndex,
    BOOL isSnooping)
{
    int i = 0;
    EigrpUpdateMessage updateMsg = {{0}};
    EigrpHeader eigrpHeader = {0};
    char* packetPtr = MESSAGE_ReturnPacket(msg);
    int initialPacketSize = MESSAGE_ReturnPacketSize(msg);
    int numEntriesinUpdateMsg = 0;
    EigrpTLVType tlvType = EIGRP_NULL_TLV_TYPE;
    AccessListFilterType filterType = ACCESS_LIST_PERMIT;
    EigrpFilterListTable* filterTable = &(eigrp->eigrpFilterListTable);
    EigrpExternMetric externRoute = {0};


    EigrpRemoveHeader(&packetPtr,initialPacketSize, &eigrpHeader);
    initialPacketSize = initialPacketSize - sizeof(EigrpHeader);

    tlvType = EigrpPreprocessTLVType(
                  &packetPtr,
                  initialPacketSize,
                  &numEntriesinUpdateMsg);

    if (DEBUG_SHOW_UPDATE)
    {
        char sourceAddrStr[MAX_STRING_LENGTH] = {0};
        IO_ConvertIpAddressToString(sourceAddr, sourceAddrStr);

        printf("UPDATE from node %s\n", sourceAddrStr);
        PrintUpdateMessage(node, packetPtr,
            numEntriesinUpdateMsg / sizeof(EigrpUpdateMessage));
    }

    while (tlvType)
    {
        initialPacketSize = initialPacketSize - sizeof(EigrpTlv);

        numEntriesinUpdateMsg = EigrpGetNumEntriesInPacket(
                                    numEntriesinUpdateMsg,
                                    tlvType);

        for (i = 0; i < numEntriesinUpdateMsg; i++)
        {
            ListItem* newTopologyEntry = NULL;
            BOOL routingTableMayChange = FALSE;
            EigrpMetric* newMetric = NULL;
            float feasibleDistance = 0.0;
            float reportedDistance = 0.0;
            BOOL summaryFlag = FALSE;

            if ((tlvType == EIGRP_SYSTEM_ROUTE_TLV_TYPE) ||
                (tlvType == EIGRP_INTERNAL_ROUTE_TLV_TYPE))
            {
                EigrpReadNumBytes(
                    &packetPtr,
                    initialPacketSize,
                    sizeof(EigrpUpdateMessage),
                    (char*) &(updateMsg.metric));

                initialPacketSize = initialPacketSize -
                    sizeof(EigrpUpdateMessage);
            }
            else if (tlvType == EIGRP_EXTERN_ROUTE_TLV_TYPE)
            {
                EigrpReadNumBytes(
                    &packetPtr,
                    initialPacketSize,
                    sizeof(EigrpExternMetric),
                    (char*) &(externRoute));

                initialPacketSize = initialPacketSize -
                    sizeof(EigrpExternMetric);

                updateMsg.metric.nextHop = externRoute.nextHop;
                updateMsg.metric.delay = externRoute.delay;
                updateMsg.metric.bandwidth = externRoute.bandwidth;
                memcpy(updateMsg.metric.mtu, externRoute.mtu, 3);
                updateMsg.metric.hopCount = externRoute.hopCount;
                updateMsg.metric.reliability = externRoute.reliability;
                updateMsg.metric.load = externRoute.load;
                updateMsg.metric.reserved = externRoute.reserved;
                updateMsg.metric.ipPrefix = externRoute.ipPrefix;

                memcpy(updateMsg.metric.destination,
                       externRoute.destination,
                       3);
            }

            if (updateMsg.metric.hopCount == UCHAR_MAX)
            {
                 // HOP COUNT == infinity
                 // This entry is due to poison-reverse
                 continue;
            }

            // ROUTE FILTERING:
            //
            // Filter the route if ACCESS-LIST imposes restriction
            // to accept this route.
            //
            // Route filters can be inserted to influence which routes
            // a router is willing to accept from its neighbors or
            // which routes the router is willing to propagate
            // to its neighbors.
            //
            // To configure EIGRP route filters, the IP access list
            // (ACL) used in all the commands can be numbered
            // or named simple IP access list.
            filterType = EigrpReturnACLPermission(
                             node,
                             filterTable,
                             (EigrpConvertCharArrayToInt(
                                  updateMsg.metric.destination) << 8),
                             ACCESS_LIST_IN,
                             interfaceIndex);

            if (EIGRP_FILTER_DEBUG)
            {
                char addrStr[MAX_STRING_LENGTH] = {0};
                const char* permission = NULL;

                IO_ConvertIpAddressToString(
                    (EigrpConvertCharArrayToInt(
                         updateMsg.metric.destination) << 8),
                    addrStr);

                if (filterType == ACCESS_LIST_PERMIT)
                {
                    permission = (char *)"ACCESS_LIST_PERMIT";
                }
                else if (filterType == ACCESS_LIST_DENY)
                {
                    permission = (char *)"ACCESS_LIST_DENY";
                }
                else
                {
                    ERROR_Assert(FALSE, "Unknown permission type");
                }

                printf("EigrpProcessUpdatePacket() "
                       "node = %d Addr = %s permission = %s"
                       "Interface index = %d\n",
                       node->nodeId,
                       addrStr,
                       permission,
                       i);
            }

            if (filterType == ACCESS_LIST_DENY)
            {
                continue;
            }

            // Calculate the metric of new incoming path
            newMetric = EigrpCalculateNewPathMetricViaSource(
                             node,
                             &(updateMsg.metric),
                             interfaceIndex);

            // Set nextHop and ipPrefix field of new metric
            newMetric->nextHop = sourceAddr;
            newMetric->ipPrefix = updateMsg.metric.ipPrefix;

            // calculate the feasible distance of new incoming route.
            feasibleDistance = EigrpCalculateMetric(newMetric,
                                    1, 0, 1, 0, 0);

            // calculate the reported distance
            reportedDistance = EigrpCalculateMetric(&(updateMsg.metric),
                                    1, 0, 1, 0, 0);

            // PrintRoutingTable(node);
            if (IsSuperNetOfABigNetwork(
                    eigrp,
                    EigrpConvertCharArrayToInt(
                        newMetric->destination) << 8,
                    newMetric->ipPrefix))
            {
                // This flag should be set to TRUE if
                // the incoming destination is a supernet of
                // a big network or altogether a new network.
                summaryFlag = TRUE;
            }

            // enter the entry into the topology table
            newTopologyEntry = EigrpEnterRouteIntoTopologyTable(
                                   node,
                                   eigrp,
                                   sourceAddr,
                                   interfaceIndex,
                                   *newMetric,
                                   reportedDistance,
                                   feasibleDistance,
                                   &routingTableMayChange,
                                   eigrpHeader.asNum);

            if (routingTableMayChange)
            {
                if (tlvType == EIGRP_EXTERN_ROUTE_TLV_TYPE)
                {
                    if (externRoute.flags == EIGRP_DEFAULT_ROUTE_FLAG)
                    {
                        SET_DEFAULT_ROUTE_FLAG(newTopologyEntry, TRUE);
                    }

                    EigrpSetExternalFieldsInTopologyTable(
                        (EigrpTopologyTableEntry*) newTopologyEntry->data,
                        externRoute.originatingRouter,
                        externRoute.originitingAs,
                        externRoute.arbitraryTag,
                        externRoute.externalProtocolMetric,
                        externRoute.externProtocolId,
                        externRoute.flags);
                }

                EigrpCallDualDecisionProcess(
                    node,
                    eigrp,
                    *newMetric,
                    updateMsg.metric.ipPrefix,
                    interfaceIndex,
                    newTopologyEntry,
                    isSnooping);

                if (summaryFlag == TRUE)
                {
                    SET_SUMMARY_FLAG(newTopologyEntry, TRUE);
                }
            }
            MEM_free(newMetric);
        } // end for (i = 0; i < numEntriesinUpdateMsg; i++)

        // Get next TLV type if exists...
        tlvType = EigrpPreprocessTLVType(
                      &packetPtr,
                      initialPacketSize,
                      &numEntriesinUpdateMsg);
    } // end while (tlvType)
    // Send an update message
    EigrpScheduleUpdateMessage(node, eigrp);
    // Increment stat var
    eigrp->eigrpStat->numUpdateReceived++;
    EigrpUpdatePacketReceivedStatus(eigrp->neighborTable, sourceAddr);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpProcessHelloOrAckPacket()
//
// PURPOSE      : Processing hello message or acknowledgement messages.
//
// PARAMETERS   : node - node which is handling the packet
//                eigrp- routing eigrp structure.
//                msg - the packet it is handling
//                sourceAddr - immediate next hop from which
//                             message being received
//                interfaceIndex - the interface index through which
//                                 the message is being received.
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpProcessHelloOrAckPacket(
    Node* node,
    RoutingEigrp* eigrp,
    Message* msg,
    NodeAddress sourceAddr,
    int interfaceIndex)
{
    char* packetPtr = MESSAGE_ReturnPacket(msg);
    int initialPacketSize = MESSAGE_ReturnPacketSize(msg);
    EigrpHeader eigrpHeader = {0};
    int helloTlvSize = 0;
    unsigned int delay = 0;

    // Find the neighbor table to be searched
    EigrpNeighborTable* neighborTable =
        &eigrp->neighborTable[interfaceIndex];

    EigrpNeighborInfo* neighborInfo = NULL;

    if (IsItAckPacket(msg))
    {
        return;
    }

    // Remove EIGRP header
    EigrpRemoveHeader(&packetPtr, initialPacketSize, &eigrpHeader);
    initialPacketSize = initialPacketSize - sizeof(EigrpHeader);

    // Remove TLV header
    EigrpPreprocessTLVType(&packetPtr, initialPacketSize, &helloTlvSize);
    initialPacketSize = (initialPacketSize - sizeof(EigrpTlv));

    // Extract delay value from Hello packet;
    EigrpReadNumBytes(
        &packetPtr,
        initialPacketSize,
        helloTlvSize,
        (char*) &delay);

    // Get the neighbor information
    neighborInfo = EigrpGetNeighbor(neighborTable, sourceAddr);

    if ((neighborInfo == NULL) ||
        (eigrpHeader.flags == EIGRP_FLAG_INITALLIZATION_BIT_ON)||
        ((neighborInfo) && (neighborInfo->state == EIGRP_NEIGHBOR_OFF)))
    {
        EigrpNeighborInfo* neighbor = NULL;
        NeighborSearchInfo neighborSearchInfo = {ANY_INTERFACE,
                                                 ANY_DEST};
        Message* holdTimer = MESSAGE_Alloc(
                                 node,
                                 NETWORK_LAYER,
                                 ROUTING_PROTOCOL_EIGRP,
                                 MSG_ROUTING_EigrpHoldTimerExpired);

        // Add the neighbor info in the neighbor table.
        if (neighborInfo == NULL)
        {
            neighbor = EigrpAddNeighbor(neighborTable, sourceAddr, delay);
        }
        else
        {
            neighbor = neighborInfo;
            neighbor->neighborHoldTime = (delay * SECOND);
        }
        neighbor->isHelloOrAnyOtherPacketReceived = TRUE;

        // Set neighborSearch info
        neighborSearchInfo.tableIndex = interfaceIndex;
        neighborSearchInfo.neighborAddress = sourceAddr;

        // Set and fire the timer.
        MESSAGE_InfoAlloc(node, holdTimer, sizeof(NeighborSearchInfo));
        memcpy(MESSAGE_ReturnInfo(holdTimer), &neighborSearchInfo,
            sizeof(NeighborSearchInfo));
        MESSAGE_Send(node, holdTimer, delay);

        EigrpSetNeighborState(neighbor, EIGRP_NEIGHBOR_CONNECTION_OPENED);

        if (DEBUG_UNICAST_UPDATE)
        {
            char dbgStr[MAX_STRING_LENGTH] = {0};
            IO_ConvertIpAddressToString(sourceAddr, dbgStr);

            printf("#node %u Sending an unicast update to %s\n",
                   node->nodeId,
                   dbgStr);
        }

        EigrpSendRoutingUpdate(node, eigrp, sourceAddr);

        // do not update "lastUpdateSent" while sending unicast
        // eigrp->lastUpdateSent = node->getNodeTime();
        EigrpSetNeighborState(neighbor, EIGRP_NEIGHBOR_ACTIVE);

    } // if (neighbor->state == EIGRP_NEIGHBOR_OFF)
    else
    {
        neighborInfo->isHelloOrAnyOtherPacketReceived = TRUE;
    }

    // Increment stat var.
    eigrp->eigrpStat->numHelloReceived++;
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpProcessReplyMessage()
//
// PURPOSE      : Processing Eigrp Reply message
//
// PARAMETERS   : node - node which is processing the reply message
//                eigrp - Pointer to eigrp structure
//                msg - the reply message received
//                sourceAdddress - sender of the reply message
//                interfaceIndex - interface index through which message is
//                                 received.
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpProcessReplyMessage(
    Node* node,
    RoutingEigrp* eigrp,
    Message* msg,
    NodeAddress sourceAddress,
    int interfaceIndex)
{
    int i = 0;
    EigrpHeader eigrpHeader = {0};
    char* packetPtr = MESSAGE_ReturnPacket(msg);
    int initialPacketSize = MESSAGE_ReturnPacketSize(msg);
    int numEntriesInReplyMsg = 0;
    EigrpTLVType tlvType = EIGRP_NULL_TLV_TYPE;
    NodeAddress replySource = ANY_DEST;
    NodeAddress nextHopAddress = ANY_DEST;
    int outgoingInterfaceIndex = ANY_INTERFACE;

    AccessListFilterType filterType = ACCESS_LIST_PERMIT;
    EigrpFilterListTable* filterTable = &(eigrp->eigrpFilterListTable);
    EigrpExternMetric externRoute = {0};

    EigrpRemoveHeader(&packetPtr, initialPacketSize, &eigrpHeader);
    initialPacketSize = (initialPacketSize - sizeof(EigrpHeader));

    // clear stack in active timer.
    eigrp->isStuckTimerSet[interfaceIndex] = EIGRP_STUCK_CLS;

    tlvType = EigrpPreprocessTLVType(
                  &packetPtr,
                  initialPacketSize,
                  &numEntriesInReplyMsg);

    while (tlvType)
    {
        initialPacketSize = (initialPacketSize - sizeof(EigrpTlv));

        numEntriesInReplyMsg = EigrpGetNumEntriesInPacket(
                                    numEntriesInReplyMsg,
                                    tlvType);

        for (i = 0; i < numEntriesInReplyMsg; i++)
        {
            EigrpReplyMessage replyMessage = {{0}};

            if ((tlvType == EIGRP_SYSTEM_ROUTE_TLV_TYPE) ||
                (tlvType == EIGRP_INTERNAL_ROUTE_TLV_TYPE))
            {
                EigrpReadNumBytes(
                    &packetPtr,
                    initialPacketSize,
                    sizeof(EigrpReplyMessage),
                    (char*)&replyMessage);

                initialPacketSize = (initialPacketSize -
                    sizeof(EigrpReplyMessage));
            }
            else if (tlvType == EIGRP_EXTERN_ROUTE_TLV_TYPE)
            {
                EigrpReadNumBytes(
                    &packetPtr,
                    initialPacketSize,
                    sizeof(EigrpExternMetric),
                    (char*) &(externRoute));

                initialPacketSize = initialPacketSize -
                    sizeof(EigrpExternMetric);

                replyMessage.metric.nextHop = externRoute.nextHop;
                replyMessage.metric.delay = externRoute.delay;
                replyMessage.metric.bandwidth = externRoute.bandwidth;
                memcpy(replyMessage.metric.mtu, externRoute.mtu, 3);
                replyMessage.metric.hopCount = externRoute.hopCount;
                replyMessage.metric.reliability = externRoute.reliability;
                replyMessage.metric.load = externRoute.load;
                replyMessage.metric.reserved = externRoute.reserved;
                replyMessage.metric.ipPrefix = externRoute.ipPrefix;

                memcpy(replyMessage.metric.destination,
                       externRoute.destination,
                       3);
            }

            // ROUTE FILTERING:
            //
            // Filter the route if ACCESS-LIST imposes restriction
            // to accept this route.
            //
            // Route filters can be inserted to influence which routes
            // a router is willing to accept from its neighbors or
            // which routes the router is willing to propagate
            // to its neighbors.
            //
            // To configure EIGRP route filters, the IP access list
            // (ACL) used in all the commands can be numbered
            // or named simple IP access list.

            filterType = EigrpReturnACLPermission(
                             node,
                             filterTable,
                             (EigrpConvertCharArrayToInt(
                                  replyMessage.metric.destination) << 8),
                             ACCESS_LIST_IN,
                             interfaceIndex);

            if (filterType == ACCESS_LIST_DENY)
            {
                continue;
            }

            if (replyMessage.metric.delay !=
                EIGRP_DESTINATION_INACCESSABLE)
            {
                ListItem* newTopologyEntry = NULL;
                BOOL routingTableMayChange = FALSE;

                // Calculate the metric of new incoming path
                EigrpMetric* newMetric =
                    EigrpCalculateNewPathMetricViaSource(
                        node,
                        &(replyMessage.metric),
                        interfaceIndex);

                // calculate the feasible distance of new incoming route.
                float feasibleDistance = EigrpCalculateMetric(newMetric,
                                             1, 0, 1, 0, 0);
                // calculate the reported distance
                float reportedDistance = EigrpCalculateMetric(
                    &(replyMessage.metric), 1, 0, 1, 0, 0);

                if (DEBUG_EIGRP_REPLY)
                {
                    char replySource[MAX_STRING_LENGTH] = {0};

                    IO_ConvertIpAddressToString(replyMessage.metric.nextHop,
                        replySource);

                    printf("node %d received a reply message from %s\n",
                            node->nodeId,
                            replySource);

                    PrintRoutingTable(node);
                    NetworkPrintForwardingTable(node);
                }

                // Set reply source same as next hop field of metric
                replySource = replyMessage.metric.nextHop;

                NetworkGetInterfaceAndNextHopFromForwardingTable(
                    node,
                    replySource,
                    &outgoingInterfaceIndex,
                    &nextHopAddress);

                // Set next hop and ip prefix field of the newMetric
                if (nextHopAddress == 0)
                {
                    newMetric->nextHop = sourceAddress;
                }
                else
                {
                    newMetric->nextHop = nextHopAddress;
                }

                newMetric->ipPrefix = replyMessage.metric.ipPrefix;

                if (nextHopAddress == (unsigned) NETWORK_UNREACHABLE)
                {
                    // char replySourceAddr[MAX_STRING_LENGTH] = {0};
                    // char time[100];
                    // printf("Node %u: got NextHop Unreachable. Intf = %d\n",
                    // node->nodeId, interfaceIndex);
                    // NetworkPrintForwardingTable(node);
                    // IO_ConvertIpAddressToString(replySource,
                    //     replySourceAddr);
                    // printf("reply source address %s not found",
                    //     replySourceAddr);

                    // NetworkPrintForwardingTable(node);
                    // TIME_PrintClockInSecond(node->getNodeTime(), time);
                    // printf("%s\n", time);

                    // ERROR_Assert(FALSE, "Reply source address not"
                    //     "found !!!!");
                    return;
                }

                // enter the entry into the topology table
                newTopologyEntry = EigrpEnterRouteIntoTopologyTable(
                                       node,
                                       eigrp,
                                       sourceAddress,
                                       outgoingInterfaceIndex,
                                       *newMetric,
                                       reportedDistance,
                                       feasibleDistance,
                                       &routingTableMayChange,
                                       eigrpHeader.asNum);

                if (routingTableMayChange)
                {
                    // THIS SEGMENT OF CODE HAS BEEN ADDED FOR DEFAULT ROUTE
                    if (tlvType == EIGRP_EXTERN_ROUTE_TLV_TYPE)
                    {
                        if (externRoute.flags == EIGRP_DEFAULT_ROUTE_FLAG)
                        {
                            SET_DEFAULT_ROUTE_FLAG(newTopologyEntry, TRUE);
                        }

                        EigrpSetExternalFieldsInTopologyTable(
                            (EigrpTopologyTableEntry*) (newTopologyEntry->data),
                            externRoute.originatingRouter,
                            externRoute.originitingAs,
                            externRoute.arbitraryTag,
                            externRoute.externalProtocolMetric,
                            externRoute.externProtocolId,
                            externRoute.flags);
                    }

                    EigrpCallDualDecisionProcess(
                        node,
                        eigrp,
                        *newMetric,
                        replyMessage.metric.ipPrefix,
                        interfaceIndex,
                        newTopologyEntry,
                        FALSE);

                    // Network update forwarding table.
                    NetworkUpdateForwardingTable(
                        node,
                        (EigrpConvertCharArrayToInt(
                             replyMessage.metric.destination) << 8),
                        ConvertNumHostBitsToSubnetMask(
                            32 - replyMessage.metric.ipPrefix),
                        ((EigrpTopologyTableEntry*) newTopologyEntry->
                            data)->metric.nextHop,
                        ((EigrpTopologyTableEntry*) newTopologyEntry->
                            data)->outgoingInterface,
                        ((EigrpTopologyTableEntry*) newTopologyEntry->
                            data)->metric.hopCount,
                        ROUTING_PROTOCOL_EIGRP);

                    if (DEBUG_EIGRP_REPLY)
                    {
                        char replySource[MAX_STRING_LENGTH] = {0};

                        IO_ConvertIpAddressToString(
                            replyMessage.metric.nextHop, replySource);

                        printf("node %d AFTER RECEIVING a reply message"
                               " from %s Updated route table\n",
                               node->nodeId,
                               replySource);
                    }
                }

                if (DEBUG_EIGRP_REPLY)
                {
                    char replySource[MAX_STRING_LENGTH] = {0};

                    IO_ConvertIpAddressToString(replyMessage.metric.nextHop,
                        replySource);

                    printf("node %d AFTER RECEIVING a reply message"
                           " from %s\n",
                           node->nodeId,
                           replySource);

                    PrintRoutingTable(node);
                    NetworkPrintForwardingTable(node);
                }
                MEM_free(newMetric);
            } // end if (!EIGRP_DESTINATION_INACCESSABLE)
        } // end for (i = 0; i < numEntriesInReplyMsg; i++)
        // Get next TLV type if exists...
        tlvType = EigrpPreprocessTLVType(
                      &packetPtr,
                      initialPacketSize,
                      &numEntriesInReplyMsg);
    } // end while (tlvType)
    // Increment stat var.
    eigrp->eigrpStat->numResponsesReceived++;
    EigrpUpdatePacketReceivedStatus(eigrp->neighborTable, sourceAddress);

    // Schedule routing update
    EigrpScheduleUpdateMessage(node, eigrp);
}


//--------------------------------------------------------------------------
// FUNCTION     : CheckHoldTimeAndHelloTime()
//
// PURPOSE      : Comparing hello time and hold time for each interfaces.
//                If hello time is greater than hold time it generates
//                a warning.
//
// PARAMETERS   : node - node which is comparing hello and hold time.
//                eigrp - pointer to eigrp protocol structure
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
static
void CheckHoldTimeAndHelloTime(Node* node,RoutingEigrp *eigrp)
{
    int j = 0;
    char buff[MAX_STRING_LENGTH] = {0};
    char holdTime[MAX_STRING_LENGTH] = {0};
    char helloTime[MAX_STRING_LENGTH] = {0};

    for (j = 0; j < node->numberInterfaces; j++)
    {
        if (eigrp->neighborTable[j].helloInterval >
            eigrp->neighborTable[j].holdTime)
        {
            ctoa(eigrp->neighborTable[j].helloInterval, helloTime);
            ctoa(eigrp->neighborTable[j].holdTime, holdTime);

            sprintf(buff, "Hello interval (%s) is greater than hold time"
                "(%s)\n. For node %u, interface index = %d\n",
                helloTime,
                holdTime,
                node->nodeId,
                j);

            ERROR_ReportWarning(buff);
        } // end if (...)
    } // for (j = 0; j < node->numberInterfaces; j++)
}


//-------------------------------------------------------------------------
// FUNCTION     : EigrpSnoopAndUpdateReplyPacketMetric()
//
// PURPOSE      : updating a reply packet metric.
//
// PARAMETERS   : node - node which is updating reply packet metric
//                message - encapsulated reply packet
//                packetSize - reply packet size after removing ip header
//                             and also eigrp header.
//-------------------------------------------------------------------------
static
void EigrpSnoopAndUpdateReplyPacketMetric(
    Node* node,
    EigrpReplyMessage* replyMsg,
    int packetSize,
    int interfaceIndex)
{
    EigrpTLVType tlvType = EIGRP_NULL_TLV_TYPE;
    int numEntriesInReplyMsg = 0;
    EigrpMetric* replyMessageMetric = NULL;
    EigrpExternMetric* externRoute = NULL;
    EigrpMetric externReplyMetric = {0};

    tlvType = EigrpPreprocessTLVType(
                  (char**) &replyMsg,
                  packetSize,
                  &numEntriesInReplyMsg);

    packetSize = (packetSize - sizeof(EigrpTlv));

    while (tlvType)
    {
        int i = 0;
        numEntriesInReplyMsg = EigrpGetNumEntriesInPacket(
                                   numEntriesInReplyMsg,
                                   tlvType);

        if ((tlvType == EIGRP_INTERNAL_ROUTE_TLV_TYPE) ||
            (tlvType == EIGRP_SYSTEM_ROUTE_TLV_TYPE))
        {
            replyMessageMetric = (EigrpMetric*) replyMsg;
        }
        else if (tlvType == EIGRP_EXTERN_ROUTE_TLV_TYPE)
        {
            externRoute = (EigrpExternMetric*) replyMsg;
            externReplyMetric.delay = externRoute->delay;
            externReplyMetric.bandwidth = externRoute->bandwidth;
            memcpy(externReplyMetric.mtu, externRoute->mtu, 3);
            externReplyMetric.hopCount = externRoute->hopCount;
            externReplyMetric.reliability = externRoute->reliability;
            externReplyMetric.load = externRoute->load;
            replyMessageMetric = &externReplyMetric;
        }

        for (i = 0; i < numEntriesInReplyMsg; i++)
        {
            if (replyMessageMetric->delay !=
                EIGRP_DESTINATION_INACCESSABLE)
            {
                // Calculate the metric of new incoming path
                EigrpMetric* newMetric =
                    EigrpCalculateNewPathMetricViaSource(
                        node,
                        replyMessageMetric,
                        interfaceIndex);

                if ((tlvType == EIGRP_INTERNAL_ROUTE_TLV_TYPE) ||
                    (tlvType == EIGRP_SYSTEM_ROUTE_TLV_TYPE))
                {
                    // Update reply message metric
                    replyMessageMetric->delay = newMetric->delay;
                    replyMessageMetric->bandwidth = newMetric->bandwidth;
                    memcpy(replyMessageMetric->mtu, newMetric->mtu, 3);
                    replyMessageMetric->hopCount = newMetric->hopCount;
                    replyMessageMetric->reliability = newMetric->reliability;
                    replyMessageMetric->load = newMetric->load;
                }
                else if (tlvType == EIGRP_EXTERN_ROUTE_TLV_TYPE)
                {
                    externRoute->delay = newMetric->delay;
                    externRoute->bandwidth = newMetric->bandwidth;
                    memcpy(externRoute->mtu, newMetric->mtu, 3);
                    externRoute->hopCount = newMetric->hopCount;
                    externRoute->reliability = newMetric->reliability;
                    externRoute->load = newMetric->load;
                }
                MEM_free(newMetric);
            }
            else
            {
                if ((tlvType == EIGRP_INTERNAL_ROUTE_TLV_TYPE) ||
                    (tlvType == EIGRP_SYSTEM_ROUTE_TLV_TYPE))
                {
                    replyMessageMetric->hopCount = (unsigned char)
                        (replyMessageMetric->hopCount + 1);
                }
                else if (tlvType == EIGRP_EXTERN_ROUTE_TLV_TYPE)
                {
                    externRoute->hopCount = (unsigned char)
                        (externRoute->hopCount + 1);
                }
            } // end if

            if ((tlvType == EIGRP_INTERNAL_ROUTE_TLV_TYPE) ||
                (tlvType == EIGRP_SYSTEM_ROUTE_TLV_TYPE))
            {
                EigrpAdvancePacketPtr((char**)&replyMsg, sizeof(EigrpMetric));
                packetSize = (packetSize - sizeof(EigrpMetric));
            }
            else if (tlvType == EIGRP_EXTERN_ROUTE_TLV_TYPE)
            {
                EigrpAdvancePacketPtr(
                    (char**)&replyMsg,
                    sizeof(EigrpExternMetric));

                packetSize = (packetSize - sizeof(EigrpExternMetric));
            }
        } // end for (i = 0; i < numEntriesInReplyMsg; i++)

        tlvType = EigrpPreprocessTLVType(
                      (char**) &replyMsg,
                      packetSize,
                      &numEntriesInReplyMsg);
        packetSize = (packetSize - sizeof(EigrpTlv));
    } // end while (tlvType)
}


//-------------------------------------------------------------------------
// FUNCTION     : EigrpRouterFunction()
//
// PURPOSE      : Snoop at the eigrp reply packet and updating its metric
//                if the packet is the reply packet.
//
// PARAMETERS   : node            - node which is routing the packet
//                msg             - the data packet
//                destAddr        - the destination node of this data packet
//                previouHopAddress - notUsed
//                PacketWasRouted   - variable that indicates to IP that it
//                                    no longer needs to act on this
//                                    data packet
//
//-------------------------------------------------------------------------
void EigrpRouterFunction(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previouHopAddress,
    BOOL* packetWasRouted)
{
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    unsigned int initialPacketSize = MESSAGE_ReturnPacketSize(msg);
    unsigned char ipProtocolType = ipHeader->ip_p;
    unsigned int ipHeaderLength =
        IpHeaderGetHLen(ipHeader->ip_v_hl_tos_len) * 4;
    NodeAddress sourceAddress = ipHeader->ip_src;
    Message* msgDup = MESSAGE_Duplicate(node, msg);

    RoutingEigrp* eigrp = (RoutingEigrp*) NetworkIpGetRoutingProtocol(
                               node, ROUTING_PROTOCOL_EIGRP);

    if ((ipProtocolType == IPPROTO_EIGRP) &&
        (!NetworkIpIsMyIP(node, destAddr)))
    {
        EigrpOpcodeType eigrpOpcode = EIGRP_ANY_OPCODE;
        EigrpHeader* eigrpHeader = NULL;

        //EigrpAdvancePacketPtr((char**)&ipHeader, ipHeaderLength);
        char *payload = (char*) ipHeader;
        payload += ipHeaderLength;
        initialPacketSize = (initialPacketSize - ipHeaderLength);

        eigrpHeader = (EigrpHeader*) payload ;
        eigrpOpcode = (EigrpOpcodeType)(eigrpHeader->opcode);

        if (eigrpOpcode == EIGRP_REPLY_PACKET)
        {
            int interfaceIndex = ANY_INTERFACE;
            EigrpReplyMessage* replyMessage = NULL;

            //EigrpAdvancePacketPtr((char**)&eigrpHeader,
                //sizeof(EigrpHeader));
            char *eigrpHeaderPacket = (char*) eigrpHeader;
            eigrpHeaderPacket += sizeof(EigrpHeader);
            initialPacketSize = (initialPacketSize - sizeof(EigrpHeader));

            replyMessage = (EigrpReplyMessage*) eigrpHeaderPacket;
            if (previouHopAddress != 0)
            {
                interfaceIndex = NetworkIpGetInterfaceIndexForNextHop(
                                     node,
                                     previouHopAddress);

                if ((interfaceIndex >= 0) &&
                    (interfaceIndex < node->numberInterfaces))
                {
                    if (DEBUG_EIGRP_REPLY)
                    {
                        printf("node %d snooping on the reply packet"
                               "send to the dest %u from the source %u ***\n"
                               "Routing Tavble BEFORE SNOOPING\n",
                               node->nodeId,
                               sourceAddress,
                               destAddr);
                        PrintRoutingTable(node);
                    }

                    MESSAGE_RemoveHeader(
                        node,
                        msgDup,
                        ipHeaderLength,
                        TRACE_IP);

                    // Snooping Update
                    EigrpProcessUpdatePacket(
                        node,
                        eigrp,
                        msgDup,
                        previouHopAddress,
                        interfaceIndex,
                        TRUE);

                    // Snoop and update eigrp metric.
                    EigrpSnoopAndUpdateReplyPacketMetric(
                        node,
                        replyMessage,
                        initialPacketSize,
                        interfaceIndex);

                    if (DEBUG_EIGRP_REPLY)
                    {
                        printf("Route tabpe of node %d ATTER SNOOPING"
                               " on the reply packet"
                               "send to the dest %u from the source %u **\n",
                               node->nodeId,
                               sourceAddress,
                               destAddr);

                        PrintRoutingTable(node);
                    }
                } // end if (interfaceIndex >= 0) && (...)
            }
        }// end if (eigrpOpcode == EIGRP_REPLY_PACKET)
    }

    EigrpRoutePacket(
        node,
        msg,
        eigrp,
        destAddr,
        previouHopAddress,
        packetWasRouted,
        ipProtocolType);

    MESSAGE_Free(node, msgDup);
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpHandleInterfaceFailure()
//
// PURPOSE      : Handling Mac layer interface status information by
//                calling the function "EigrpProcessHoldTimeExpired()"
//
// PARAMETERS   : node - node which is handling information
//                interfaceIndex - interface Index who's status is being
//                                 handled.
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpHandleInterfaceFailure(
    Node* node,
    int interfaceIndex)
{
    RoutingEigrp* eigrp = (RoutingEigrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_EIGRP);

    if (eigrp != NULL)
    {
        EigrpNeighborTable* neighborTable = &eigrp->
            neighborTable[interfaceIndex];

        NeighborSearchInfo neighborSearchInfo = {ANY_INTERFACE, ANY_DEST};
        EigrpNeighborInfo* temp = neighborTable->first;

        while (temp)
        {
            Message* msg = MESSAGE_Alloc(
                               node,
                               NETWORK_LAYER,
                               ROUTING_PROTOCOL_EIGRP,
                               MSG_ROUTING_EigrpHoldTimerExpired);
            MESSAGE_InfoAlloc(node, msg, sizeof(NeighborSearchInfo));

            // Set neighborSearch info
            neighborSearchInfo.tableIndex = interfaceIndex;
            neighborSearchInfo.neighborAddress = temp->neighborAddr;
            memcpy(MESSAGE_ReturnInfo(msg), &neighborSearchInfo,
                sizeof(NeighborSearchInfo));

            // Do the same processing as if Hold timer  expired
            EigrpProcessHoldTimeExpired(node, eigrp, msg);
            temp = temp->next;
        } // end while (temp)
    } // end if (eigrp != NULL)
}


//---------------------------------------------------------------------------
// FUNCTION     : EigrpAddInterfaceAddresses()
//
// PURPOSE      : Reading and configuring interface network addresses
//                statically
//
// PARAMETERS   : node - node which is performing the above operation
//                eigrp - pointer to eigrp data structure.
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
static
void EigrpAddInterfaceAddresses(
    Node* node,
    RoutingEigrp* eigrp,
    int interfaceIndex = ANY_INTERFACE)
{
    #define IS_MATCH(i, j) ((j == ANY_INTERFACE) || (j == i) ? TRUE : FALSE)
    if (eigrp != NULL)
    {
        int i = 0;
        for (i = 0; i < node->numberInterfaces; i++)
        {
            if (!IS_MATCH(i, interfaceIndex))
            {
                continue;
            }

            ListItem* newTopologyEntry = NULL;
            NodeAddress outputNodeAddress = ANY_DEST;
            int numHostBits = 0;
            EigrpMetric metric = {0};
            unsigned int propDelay = 0;
            unsigned int bandwidth = 0;
            float reportedDistance = 0;
            float feasibleDistance = 0;
            BOOL routingTableMayChange = FALSE;

            // Get Interface network address.
            outputNodeAddress =
            NetworkIpGetInterfaceNetworkAddress(node, i);

            // Get interface number of host bits.
            numHostBits = NetworkIpGetInterfaceNumHostBits(node, i);

            // Program Invariant: It is ensured that network is valid
            //                    and connected with the node
            bandwidth = EigrpGetInvBandwidth(node, i);

            propDelay = (unsigned int)
                (EigrpGetPropDelay(node, i) / MICRO_SECOND);

            // Build up an routing metric entry.
            metric.nextHop = 0;
            metric.delay = propDelay;
            metric.bandwidth = bandwidth;
            memset(&metric.mtu, 0, (sizeof(char) * 3));
            metric.hopCount = 0;
            metric.reliability = 1;
            metric.load = 0;
            memset(&metric.reserved, 0, sizeof(unsigned char));
            metric.ipPrefix = (unsigned char) (32 - numHostBits);

            metric.destination[2] = ((unsigned char)
                (0xFF & (outputNodeAddress >> 24)));
            metric.destination[1] = ((unsigned char)
                (0xFF & (outputNodeAddress >> 16)));
            metric.destination[0] = ((unsigned char)
                (0xFF & (outputNodeAddress >> 8)));


            reportedDistance = EigrpCalculateMetric(
                &metric, 1, 0, 1, 0, 0);

            feasibleDistance = reportedDistance;

            newTopologyEntry = EigrpEnterRouteIntoTopologyTable(
                                   node,
                                   eigrp,
                                   DIRECTLY_CONNECTED_NETWORK,
                                   i,
                                   metric,
                                   reportedDistance,
                                   feasibleDistance,
                                   &routingTableMayChange,
                                   eigrp->asId);

            ERROR_Assert(routingTableMayChange, "routingTableMayChange"
                "Must be true here !!!");

            if (routingTableMayChange == TRUE)
            {
                EigrpCallDualDecisionProcess(
                    node,
                    eigrp,
                    metric,
                    metric.ipPrefix,
                    i,
                    newTopologyEntry,
                    FALSE);
            }
        } // end for (i = 0; i < node->numberInterfaces; i++)
    } // end if (eigrp != NULL)
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpInterfaceStatusHandler()
//
// PURPOSE      : Handling Mac layer interface status information
//
// PARAMETERS   : node - node which is handling information
//                interfaceIndex - interface Index who's status is being
//                                 handled.
//                state - state ACTIVE of FAILED
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
void EigrpInterfaceStatusHandler(
    Node* node,
    int interfaceIndex,
    MacInterfaceState state)
{
    switch (state)
    {
        case MAC_INTERFACE_DISABLE:
        {
           EigrpHandleInterfaceFailure(
                node,
                interfaceIndex);
           break;
        }

        case MAC_INTERFACE_ENABLE:
        {
            RoutingEigrp* eigrp = (RoutingEigrp*)
                NetworkIpGetRoutingProtocol(
                    node,
                    ROUTING_PROTOCOL_EIGRP);

            EigrpAddInterfaceAddresses(node, eigrp, interfaceIndex);
            break;
        }

        default:
        {
            break;
        }
    } // end switch (state)
}


//--------------------------------------------------------------------------
// FUNCTION     : EigrpInit
//
// PURPOSE      : initializing EIGRP protocol.
//
// PARAMETERS   : node - the node which is initializing
//                eigrp - the routing eigrp structure
//                nodeInput - the input configuration file
//                interfaceIndex - interface index.
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
void EigrpInit(Node* node,
    RoutingEigrp** eigrp,
    const NodeInput* nodeInput,
    int interfaceIndex)
{
    int i = 0;
    char buf[MAX_STRING_LENGTH] = {0};
    Message* helloTimer = NULL;
    BOOL wasFound = FALSE;
    NodeInput eigrpConfigFile;

    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    IO_ReadCachedFile(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        nodeInput,
        "EIGRP-CONFIG-FILE",
        &wasFound,
        &eigrpConfigFile);

    if (!wasFound)
    {
        ERROR_ReportError("EIGRP Configuration file not Found");
    }

    EigrpReadFromConfigurationFile(
        node,
        eigrp,
        &eigrpConfigFile,
        &wasFound);

    EigrpAddInterfaceAddresses(node, *eigrp);

    if (!wasFound)
    {
        *eigrp = NULL;
        return;
    }

    RANDOM_SetSeed((*eigrp)->seed,
                   node->globalSeed,
                   node->nodeId,
                   ROUTING_PROTOCOL_EIGRP,
                   interfaceIndex);

    // Compare hold time & hello time that read from the
    // configuration file; and generate a warning if
    // hold-time less than hello time is given.
    CheckHoldTimeAndHelloTime(node, *eigrp);

    if (EIGRP_FILTER_DEBUG)
    {
        EigrpPrintRouterFilterRead(
            node,
            (*eigrp)->eigrpFilterListTable);
    }

    if (((*eigrp)->sleepTime) < 0)
    {
        ERROR_ReportError("Sleep time is less than zero");
    }

    IO_ReadString(
        node->nodeId,
        ANY_IP,
        nodeInput,
        "ROUTING-STATISTICS",
        &wasFound,
        buf);

    // Initialize statistical variables
    //     numTriggeredUpdates = 0
    //     numQuerySend = 0
    //     numResponsesReceived = 0
    //     numUpdateReceived = 0
    //     numQueryReceived = 0
    //     numResponsesSend = 0
    //     numHelloSend = 0
    //     numHelloReceived = 0
    //     isStatPrinted = FALSE;
    (*eigrp)->eigrpStat = (EigrpStat*) MEM_malloc(sizeof(EigrpStat));
    memset((*eigrp)->eigrpStat, 0, sizeof(EigrpStat));

    if (wasFound)
    {
        if (!strcmp(buf, "YES"))
        {
            (*eigrp)->collectStat = TRUE;
        }
    }

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ip->interfaceInfo[i]->routingProtocolType !=
                            ROUTING_PROTOCOL_EIGRP)
        {
            continue;
        }
        // set hello timer
        helloTimer = MESSAGE_Alloc(
                         node,
                         NETWORK_LAYER,
                         ROUTING_PROTOCOL_EIGRP,
                         MSG_ROUTING_EigrpHelloTimerExpired);

        MESSAGE_InfoAlloc(node, helloTimer, sizeof(int));
        *((unsigned int*) MESSAGE_ReturnInfo(helloTimer)) = i;
        MESSAGE_Send(node, helloTimer, 10 * MILLI_SECOND);
    } // end for (i = 0; i < node->numberInterfaces; i++)

    NetworkIpAddToMulticastGroupList(node, EIGRP_ROUTER_MULTICAST_ADDRESS);
    NetworkIpSetRouterFunction(node, EigrpRouterFunction, interfaceIndex);


//     Set routing table update function for route redistribution

    RouteRedistributeSetRoutingTableUpdateFunction(
        node,
        &EigrpHookToRedistribute,
        interfaceIndex);
}
//-----------------------------------------------------------------------------
// FUNCTION     EigrpHookToRedistribute()
// PURPOSE      Update or add the redistributed
//              routes into EIGRP routing table.
// PARAMETERS   Node *node
//                  Pointer to node.
//              NodeAddress destAddress
//                  IP address of destination network or host.
//              NodeAddress destAddressMask
//                  Netmask.
//              NodeAddress nextHopAddress
//                  Next hop IP address.
//              int interfaceIndex
//                    The specify the routing protocol
//              void* eigrpMetric
//                  Metric associate with the route
// RETURN       NONE
//-----------------------------------------------------------------------------

void
EigrpHookToRedistribute(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    void* eigrpMetric)
{

    int numHostBits = 0;
    unsigned int propDelay = 0;
    unsigned int bandwidth = 0;
    float reportedDistance = 0;
    float feasibleDistance = 0;

    BOOL routingTableMayChange = FALSE;
    ListItem* newTopologyEntry = NULL;
    EigrpMetric metric = {0};


    RoutingEigrp* eigrp = (RoutingEigrp*)NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_EIGRP);

    EigrpMetric* eigrpMetricInfo = (EigrpMetric*) eigrpMetric;




    metric.nextHop = nextHopAddress;
    metric.delay = propDelay;
    metric.bandwidth = bandwidth;
    memset(&metric.mtu, 0, (sizeof(char) * 3));
    metric.hopCount = 0;
    metric.reliability = 1;
    metric.load = 0;
    memset(&metric.reserved, 0, sizeof(unsigned char));

    numHostBits = NetworkIpGetInterfaceNumHostBits(node, interfaceIndex);


    metric.ipPrefix = (unsigned char) (32 - numHostBits);

    metric.destination[2] = ((unsigned char)(0xFF & (destAddress >> 24)));
    metric.destination[1] = ((unsigned char)(0xFF & (destAddress >> 16)));
    metric.destination[0] = ((unsigned char)(0xFF & (destAddress >> 8)));

    // Explicitly METRIC Argument has been specified
    if (eigrpMetricInfo != NULL)
    {
        metric.hopCount = eigrpMetricInfo->hopCount;
        metric.bandwidth = eigrpMetricInfo->bandwidth;
        metric.reliability = eigrpMetricInfo->reliability;
        metric.load = eigrpMetricInfo->load;

        memcpy(
            metric.mtu,
            &eigrpMetricInfo->mtu,
            (sizeof(char) * 3));

        //If nexthop is unreachable then make the route inaccessible
        //by setting delay as inaccessible
        if (nextHopAddress == (unsigned) NETWORK_UNREACHABLE)
        {
            metric.delay = EIGRP_DESTINATION_INACCESSABLE;
        }
        else
        {
            metric.delay = eigrpMetricInfo->delay;
        }
    }

    newTopologyEntry = EigrpEnterRouteIntoTopologyTable(
                           node,
                           eigrp,
                           DIRECTLY_CONNECTED_NETWORK,
                           interfaceIndex,
                           metric,
                           reportedDistance,
                           feasibleDistance,
                           &routingTableMayChange,
                           eigrp->asId);

    if (routingTableMayChange)
    {
    EigrpAddRouteToRoutingTable(node,
        eigrp,
        destAddress,
        interfaceIndex,
        newTopologyEntry);

    EigrpScheduleUpdateMessage(node, eigrp);

    }

}

//--------------------------------------------------------------------------
// FUNCTION     : EigrpHandleProtocolEvent()
//
// PURPOSE      : Handling protocol related timer events
//
// PARAMETERS   : node - node which is handling the packet
//                msg - the event it is handling
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
void EigrpHandleProtocolEvent(Node* node, Message* msg)
{
    RoutingEigrp* eigrp = (RoutingEigrp*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_EIGRP);

    switch (msg->eventType)
    {
        case MSG_ROUTING_EigrpHelloTimerExpired :
        {
            int interfaceIndex = *((int*) MESSAGE_ReturnInfo(msg));

            if (NetworkIpGetUnicastRoutingProtocolType(
                     node,
                     interfaceIndex) == ROUTING_PROTOCOL_EIGRP)
            {
                // Send hello packet thru this in terface
                if (NetworkIpInterfaceIsEnabled(node, interfaceIndex))
                {
                    EigrpSendHelloPacket(node, eigrp, interfaceIndex);
                }
                // Eigrp start timer for sending next hello message
                MESSAGE_Send(node, msg,
                    eigrp->neighborTable[interfaceIndex].helloInterval);
            }
            else
            {
                MESSAGE_Free(node, msg);
            }
            break;
        }

        case MSG_ROUTING_EigrpHoldTimerExpired :
        {
            NeighborSearchInfo* neighborSearchInfo = (NeighborSearchInfo*)
                MESSAGE_ReturnInfo(msg);
            int interfaceindex = neighborSearchInfo->tableIndex;

            if (!NetworkIpInterfaceIsEnabled(node, interfaceindex))
            {
                MESSAGE_Free(node, msg);
                return;
            }

            EigrpProcessHoldTimeExpired(node, eigrp, msg);
            break;
        }

        case MSG_ROUTING_EigrpRiseUpdateAlarm :
        {
            EigrpSendRoutingUpdate(node, eigrp,
                 EIGRP_ROUTER_MULTICAST_ADDRESS);
            eigrp->lastUpdateSent = node->getNodeTime();
            eigrp->isNextUpdateScheduled = FALSE;
            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_ROUTING_EigrpStuckInActiveRouteTimerExpired :
        {
            // printf ("node %d speaking node connected via "
            //        "interface index %d not responded within time\n",
            //         node->nodeId, *(MESSAGE_ReturnInfo(msg)));
            int interfaceIndex = *((int*)MESSAGE_ReturnInfo(msg));

            if (eigrp->isStuckTimerSet[interfaceIndex] == EIGRP_STUCK_CLS)
            {
               // eigrp->isStuckTimerSet[interfaceIndex] = EIGRP_STUCK_OFF;
               // printf("node->nodeId  = %d got reply (%d)\n",
               //     node->nodeId, interfaceIndex);
               MESSAGE_Free(node, msg);
               return;
            }
            // printf("node->nodeId  = %d did not get reply (%d)\n",
            //    node->nodeId, interfaceIndex);
            // eigrp->isStuckTimerSet[interfaceIndex] = EIGRP_STUCK_OFF;
            // EigrpRestartHello(node, eigrp, msg, interfaceIndex);
            MESSAGE_Free(node, msg);
            break;
        }

        default :
        {
            ERROR_Assert(FALSE, "Error Unknown event fired");
            break;
        }
    } // end switch (msg->eventType)
} // EigrpHandleProtocolEvent()


//--------------------------------------------------------------------------
// FUNCTION     : EigrpHandleProtocolPacket()
//
// PURPOSE      : Handling protocol related control packets
//
// PARAMETERS   : node - node which is handling the packet
//                msg - the packet it is handling
//                sourceAddr - immediate next hop from which
//                             message being received
//                interfaceIndex - the interface index through which
//                                 the message is being received.
//                previousHopAddress - the previous hop address
//
// RETURN VALUE : void
//--------------------------------------------------------------------------
void EigrpHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddr,
    int interfaceIndex,
    NodeAddress previousHopAddress)
{
    RoutingEigrp* eigrp = (RoutingEigrp*) NetworkIpGetRoutingProtocol(
                               node, ROUTING_PROTOCOL_EIGRP);
    if (!eigrp)
    {
        // if this is not eigrp router
        // then discard the message
        MESSAGE_Free(node, msg);
        return;
    }

    // Eigrp is developed to work in wired scenario only. This fixers
    // are needed to run EIGRP in wireless scenario without crashing.
    // Well It means EIGRP will run just without crashing..
    // and hope packets will be routed nevertheless.
    /*Commented During ARP task, to remove previous hop dependency*/
   /* #define FIX_EIGRP_WIRELESS
    #ifdef  FIX_EIGRP_WIRELESS
    BOOL process = FALSE;

    NodeAddress netAddr = NetworkIpGetInterfaceNetworkAddress(
                              node,
                              interfaceIndex);

    NodeAddress netMask = NetworkIpGetInterfaceSubnetMask(
                              node,
                              interfaceIndex);

    if (netAddr == (previousHopAddress & netMask))
    {
        process = TRUE;
    }

    if (!process)
    {
        MESSAGE_Free(node, msg);
        return;
    }
    #endif*/

    switch (EigrpGetOpcode(msg))
    {
        case EIGRP_HELLO_OR_ACK_PACKET:
        {
            EigrpProcessHelloOrAckPacket(
                node,
                eigrp,
                msg,
                sourceAddr,
                interfaceIndex);
            break;
        }

        case EIGRP_UPDATE_PACKET :
        {
            EigrpProcessUpdatePacket(
                node,
                eigrp,
                msg,
                sourceAddr,
                interfaceIndex,
                FALSE);
            break;
        }

        case EIGRP_QUERY_PACKET :
        {
            EigrpProcessQueryPacketAndBuildReplyPacket(
                node,
                eigrp,
                msg,
                sourceAddr,
                interfaceIndex);
            break;
        }

        case EIGRP_REPLY_PACKET :
        {
            EigrpProcessReplyMessage(
                node,
                eigrp,
                msg,
                sourceAddr,
                interfaceIndex);
            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Unknown Packet type");
            break;
        }
    } // end switch (EigrpGetOpcode(msg->packet))
    MESSAGE_Free(node, msg);
} // end EigrpHandleProtocolPacket()


//--------------------------------------------------------------------------
// FUNCTION     : EigrpFinalize()
//
// PURPOSE      : Printing eigrp statistics
//
// PARAMETERS   : node - node which is printing statistics
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
void EigrpFinalize(Node* node)
{
    char buffer[MAX_STRING_LENGTH] = {0};

    RoutingEigrp* eigrp = (RoutingEigrp*) NetworkIpGetRoutingProtocol(
                               node, ROUTING_PROTOCOL_EIGRP);

    if ((eigrp) && (eigrp->collectStat))
    {
        if (eigrp->eigrpStat->isStatPrinted == TRUE)
        {
            return;
        }

        sprintf(buffer, "Number of triggered Updates = %u",
            eigrp->eigrpStat->numTriggeredUpdates);
        IO_PrintStat(node, "Network", "EIGRP", ANY_DEST, -1, buffer);

        sprintf(buffer, "Number of query send = %u",
            eigrp->eigrpStat->numQuerySend);
        IO_PrintStat(node, "Network", "EIGRP", ANY_DEST, -1, buffer);

        sprintf(buffer, "Number of reply received = %u",
            eigrp->eigrpStat->numResponsesReceived);
        IO_PrintStat(node, "Network", "EIGRP", ANY_DEST, -1, buffer);

        sprintf(buffer, "Number of Updates Received = %u",
            eigrp->eigrpStat->numUpdateReceived);
        IO_PrintStat(node, "Network", "EIGRP", ANY_DEST, -1, buffer);

        sprintf(buffer, "Number of query Received = %u",
            eigrp->eigrpStat->numQueryReceived);
        IO_PrintStat(node, "Network", "EIGRP", ANY_DEST, -1, buffer);

        sprintf(buffer, "Number of Reply send = %u",
            eigrp->eigrpStat->numResponsesSend);
        IO_PrintStat(node, "Network", "EIGRP", ANY_DEST, -1, buffer);

        sprintf(buffer, "Number of hello send = %u",
            eigrp->eigrpStat->numHelloSend);
        IO_PrintStat(node, "Network", "EIGRP", ANY_DEST, -1, buffer);

        sprintf(buffer, "Number of hello received = %u",
            eigrp->eigrpStat->numHelloReceived);
        IO_PrintStat(node, "Network", "EIGRP", ANY_DEST, -1, buffer);

        eigrp->eigrpStat->isStatPrinted = TRUE;
    }

    if (eigrp)
    {
        if (EIGRP_DEBUG_TABLE)
        {
            // PrintNeighborTable(node);
            // PrintTopologyTable(node);
            PrintTopologyTable(node);
            PrintRoutingTable(node);
            NetworkPrintForwardingTable(node);
        }
    }
}
