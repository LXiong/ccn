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





//--------------------------------------------------------------------------
// File     : policy_routing.cpp
// Objective: Source file for policy_routing (CISCO implementation
//              of policy based routing)
// Reference: CISCO document Set
//--------------------------------------------------------------------------

// References:-
//  http://www.cisco.com/univercd/cc/td/doc/product/software/ios120/12cgcr/
//      qos_c/qcpart1/qcpolicy.htm#xtocid214451

// Notes and assumptions:-
//
//  PBR and local PBR are implemented. Fast-Switched PBR and CEF-Switched
//      PBR are not implemented.
//  The interface command should be put before the PBR command. The
//      interface command puts the router into interface configuration mode.
//  Assuming that there exists only one set command for a given set type.
// No particular check is available to ascertain whether its a data
//  or a control packet(we can do a precedence check, but thats not
// a foolproof test). So all the packets are policy routed. (Modified on next line)
// This is modified and TOS is checked to IPTOS_PREC_INTERNETCONTROL to
//  segregate the control and data packet. If a data packet is sent with this
//  TOS, PBR wont work.
// If just the interface is set, its passed to the connected network

//
// Syntax:-
//  route-map map-tag [permit | deny] [sequence-number]
//
//  match length min max
//  and/or
//  match ip address {access-list-number | name}
//  [...access-list-number | name]
//
//  set ip precedence [number | name]
//  set ip next-hop ip-address [... ip-address]
//  set interface interface-type interface-number
//      [... type number]
//  set ip default next-hop ip-address [... ip-address]
//  set default interface interface-type
//      interface-number [... type ...number]
//
//  interface interface-type interface-number
//  note that we are following the syntax: interface interface-number
//  ip policy route-map map-tag
//
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "qualnet_error.h"
#include "network_ip.h"
#include "route_map.h"
#include "network_access_list.h"
#include "routing_policy_routing.h"
#include "route_parse_util.h"

// Organization of the code:
//      Init
#define DEBUG 0

#define PBR_PACKET_DEBUG 0

// For the precedence
#define PBR_NULL_PRECED 0x1F


//--------------------------------------------------------------------------
// FUNCTION  PbrValidateSetCommand
// PURPOSE   Check the type of set commands and alter their orders as per PBR
// ARGUMENTS node, node concerned
//           entry, The route map entry
//           originalString, The original string as defined in router config
// RETURN    None
// NOTE      Assuming that there exists only one set command for a given
//              set type.
//           A better algo for arranging can be thought of later.
//--------------------------------------------------------------------------
static
void PbrValidateSetCommand(Node* node,
                           RouteMapEntry* entry,
                           const char* originalString)
{
    RouteMapSetCmd* set = entry->setHead;
    //RouteMapSetCmd* temp = set;
    //RouteMapSetCmd* modSet = NULL;

    while (set)
    {
        if ((set->setType == AUTO_TAG) ||
            (set->setType == IP_DEF_NEXT_HOP_VERFY_AVAIL) ||
            (set->setType == IP_NEXT_HOP_VERFY_AVAIL) ||
            (set->setType == LEVEL) ||
            (set->setType == LOCAL_PREF) ||
            (set->setType == SET_METRIC) ||
            (set->setType == NEXT_HOP) ||
            (set->setType == SET_TAG) ||
            (set->setType == METRIC_TYPE))
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node %u: Wrong set of SET commands defined"
                " for PBR in router config file\n%s\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        set = set->next;
    }

    if (DEBUG)
    {
        // printing the values
        printf("Set validation over\n");
    }

    // we need not this piece of code
    /* set = temp;
    while (set)
    {
        if (set->setType == IP_PRECEDENCE)
        {
            modSet = set;
            break;
        }
        set = set->next;
    }

    set = temp;
    while (set)
    {
        if (set->setType == SET_IP_NEXT_HOP)
        {
            while (modSet->next != NULL)
            {
                modSet = modSet->next;
            }
            modSet->next = set;
            break;
        }
        set = set->next;
    }

    set = temp;
    while (set)
    {
        if (set->setType == SET_INTERFACE)
        {
            while (modSet->next != NULL)
            {
                modSet = modSet->next;
            }
            modSet->next = set;
            break;
        }
        set = set->next;
    }

    set = temp;
    while (set)
    {
        if (set->setType == IP_DEF_NEXT_HOP)
        {
            while (modSet->next != NULL)
            {
                modSet = modSet->next;
            }
            modSet->next = set;
            break;
        }
        set = set->next;
    }

    set = temp;
    while (set)
    {
        if (set->setType == DEF_INTF)
        {
            while (modSet->next != NULL)
            {
                modSet = modSet->next;
            }
            modSet->next = set;
            break;
        }
        set = set->next;
    }

    entry->setHead = modSet;
    */
}



//--------------------------------------------------------------------------
// FUNCTION  PbrValidateMatchCommand
// PURPOSE   Check the type of match commands
// ARGUMENTS node, node concerned
//           entry, The route map entry
//           originalString, The original string as defined in router config
// RETURN    None
//--------------------------------------------------------------------------
static
void PbrValidateMatchCommand(Node* node,
                             RouteMapEntry* entry,
                             const char* originalString)
{
    RouteMapMatchCmd* match = entry->matchHead;
    while (match)
    {
        if ((match->matchType != LENGTH) && (match->matchType != IP_ADDRESS))
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node %u: Wrong set of MATCH commands defined"
                " for PBR in router config file\n%s\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        match = match->next;
    }

    if (DEBUG)
    {
        // printing the values
        printf("Match validation over\n");
    }
}



//--------------------------------------------------------------------------
// FUNCTION  PbrValidateRouteMap
// PURPOSE   Check the match and set commands and rearrange them as per the
//              PBR demands.
// ARGUMENTS node, node concerned
//           rMap, The route map plugged with the PBR
//           originalString, The original string as defined in router config
// RETURN    None
//--------------------------------------------------------------------------
static
void PbrValidateRouteMap(Node* node,
                         RouteMap* rMap,
                         const char* originalString)
{
    RouteMapEntry* entry = rMap->entryHead;

    while (entry)
    {
        PbrValidateMatchCommand(node, entry, originalString);
        PbrValidateSetCommand(node, entry, originalString);

        entry = entry->next;
    }

    if (DEBUG)
    {
        // printing the values
        printf("Route map validation over\n");
    }

}



//--------------------------------------------------------------------------
// FUNCTION    PbrTraceInit
// PURPOSE     Initialize trace for the policy routing
// ARGUMENTS   node, node concerned
//             nodeInput
// RETURN      None
//--------------------------------------------------------------------------
static
void PbrTraceInit(Node* node,
                  const NodeInput* nodeInput)
{
    char yesOrNo[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    IO_ReadString(node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "PBR-TRACE",
        &retVal,
        yesOrNo);

    if (retVal == TRUE)
    {
        if (!strcmp(yesOrNo, "NO"))
        {
            ip->pbrTrace = FALSE;
        }
        else if (!strcmp(yesOrNo, "YES"))
        {
            ip->pbrTrace = TRUE;
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node %u: PbrTraceInit: "
                "Unknown value of PBR-TRACE in configuration "
                "file.\n", node->nodeId);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // PBR-TRACE not enabled.
        ip->pbrTrace = FALSE;
    }
}


//--------------------------------------------------------------------------
// FUNCTION  PbrInit
// PURPOSE   Plug the route maps that are included in the PBR and do the
//  required checks.
// ARGUMENTS node, node concerned
//           nodeInput, Main configuration file
// RETURN    None
// COMMENTS  Statements which are not used will be not parsed. No feedback
//              is given to the user.
//--------------------------------------------------------------------------
void PbrInit(Node* node, const NodeInput* nodeInput)
{
    NodeInput rtConfig;
    char buf[MAX_STRING_LENGTH];
    BOOL isFound = FALSE;
    BOOL retVal = FALSE;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    // Check for the file
    IO_ReadCachedFile(node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTER-CONFIG-FILE",
        &isFound,
        &rtConfig);

    // file found
    if (isFound == TRUE)
    {
        int count = 0;
        int interfaceIndex = -1;
        BOOL foundRouterConfiguration = TRUE;
        BOOL gotForThisNode = FALSE;

        for (count = 0; count < rtConfig.numLines; count++)
        {
            // The parsing logic doesnt check for any other unwanted lines.
            char* originalString = NULL;
            char* criteriaString = NULL;
            char token1[MAX_STRING_LENGTH] = {0};
            char token2[MAX_STRING_LENGTH] = {0};
            char token3[MAX_STRING_LENGTH] = {0};
            char token4[MAX_STRING_LENGTH] = {0};
            char token5[MAX_STRING_LENGTH] = {0};
            char token6[MAX_STRING_LENGTH] = {0};
            int countStr = 0;

            // find the first and second token
            countStr = sscanf(rtConfig.inputStrings[count],"%s %s %s %s %s"
                 "%s",token1, token2, token3, token4, token5, token6);

           // we make a copy of the original string for error reporting
            originalString = RtParseStringAllocAndCopy(
                rtConfig.inputStrings[count]);

            if (RtParseStringNoCaseCompare("NODE-IDENTIFIER", token1) == 0)
            {
                BOOL isAnother = FALSE;
                foundRouterConfiguration = FALSE;

                RtParseIdStmt(
                    node,
                    rtConfig,
                    count,
                    &foundRouterConfiguration,
                    &gotForThisNode,
                    &isAnother);

                MEM_free(originalString);
                if (isAnother)
                {
                    // Got another router statement so this router's
                    // configuration is complete
                    break;
                }
                else
                {
                    continue;
                }
            }
            else if (foundRouterConfiguration &&
                (RtParseStringNoCaseCompare("INTERFACE", token1) == 0))
            {
                if (countStr != 2)
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "node %u: Wrong syntax"
                        " in router config file\n%s\n", node->nodeId,
                        originalString);
                    ERROR_ReportError(errString);
                }

                sscanf(originalString, "%*s %d", &interfaceIndex);

                if ((interfaceIndex < 0) ||
                    (interfaceIndex >= node->numberInterfaces))
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "node %u: Wrong Interface index"
                        " in router config file\n%s\n", node->nodeId,
                        originalString);
                    ERROR_ReportError(errString);
                }

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (RtParseStringNoCaseCompare("IP", token1) == 0))
            {
                if (RtParseStringNoCaseCompare("LOCAL", token2) == 0)
                {
                    if (countStr != 5)
                    {
                        char errString[MAX_STRING_LENGTH];
                        sprintf(errString, "node %u: Wrong syntax"
                            " in router config file\n%s\n", node->nodeId,
                            originalString);
                        ERROR_ReportError(errString);
                    }
                    // Local PBR
                    ip->local = TRUE;

                    strcpy(token2, token3);
                    strcpy(token3, token4);
                    strcpy(token4, token5);
                    strcpy(token5, token6);
                }

                if ((RtParseStringNoCaseCompare("POLICY", token2) == 0)  &&
                (RtParseStringNoCaseCompare("ROUTE-MAP", token3) == 0))
                {
                    // ip policy route-map map-tag
                    // ip local policy route-map map-tag

                    int currentPosition = 0;
                    RouteMap* rMap = NULL;
                    criteriaString = originalString;

                    currentPosition = RtParseGetPositionInString(
                        originalString, token3);

                    criteriaString = currentPosition + criteriaString;

                    sscanf(criteriaString, "%s", token4);

                    // Is the route map already defined
                    rMap = RouteMapFindByName(node, token4);
                    if (rMap == NULL)
                    {
                        char errString[MAX_STRING_LENGTH];
                        sprintf(errString, "node %u: No route map, '%s',"
                            " defined for this node. PBR action cancelled."
                            "\n%s\n", node->nodeId, token4, originalString);
                        ERROR_ReportWarning(errString);
                        return;
                    }

                    // checking whether the route map has got the
                    //   permissible criterias and the set stmts
                    PbrValidateRouteMap(node, rMap, originalString);

                    // if its local we store at the ip
                    if (ip->local)
                    {
                        // Only one route map can be defined
                        if (ip->rMapForPbr != NULL)
                        {
                            char errString[MAX_STRING_LENGTH];
                            sprintf(errString, "node %u: Not more than one "
                                "route map can be defined for a given "
                                "interface.\n%s\n",
                                node->nodeId, originalString);
                            ERROR_ReportError(errString);
                        }

                        ip->rMapForPbr = rMap;
                    }
                    else
                    {
                        // Check the interface index
                        if (interfaceIndex == -1)
                        {
                            char errString[MAX_STRING_LENGTH];
                            sprintf(errString, "node %u: Interface index"
                                " not defined before setting PBR in router "
                                "config file\n%s\n", node->nodeId,
                                originalString);
                            ERROR_ReportError(errString);
                        }

                        // Only one route map can be defined
                        if (ip->interfaceInfo[interfaceIndex]->rMapForPbr !=
                            NULL)
                        {
                            char errString[MAX_STRING_LENGTH];
                            sprintf(errString, "node %u: Not more than one "
                                "route map can be defined for a given "
                                "interface.\n%s\n",
                                node->nodeId, originalString);
                            ERROR_ReportError(errString);
                        }

                        ip->interfaceInfo[interfaceIndex]->rMapForPbr = rMap;
                    }
                } // if

                MEM_free(originalString);

                continue;
            } // if

            MEM_free(originalString);

        } // for

        // Do we have the stats on
        IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "POLICY-ROUTING-STATISTICS",
        &retVal,
        buf);

        if (retVal == FALSE)
        {
            ip->isPBRStatOn = FALSE;
        }
        else
        {
            if (strcmp(buf, "YES") == 0)
            {
                ip->isPBRStatOn = TRUE;
            }
            else if (strcmp(buf, "NO") == 0)
            {
                ip->isPBRStatOn = FALSE;
            }
            else
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "node %u: Expecting YES or NO for"
                    "POLICY-ROUTING-STATISTICS parameter in configuration "
                    "file.\n", node->nodeId);
                ERROR_ReportError(errString);
            }
        }

        PbrTraceInit(node, nodeInput);

        if (DEBUG)
        {
            // printing the values
            printf("Node: %u PBR init over...\n", node->nodeId);
        }
    } // endif
}



//--------------------------------------------------------------------------
// FUNCTION  PbrFillValues
// PURPOSE   Squeeze the values from the packet and fill up the value list
//              for route map.
// ARGUMENTS node, node concerned
//           valueSet, Structure for passing values to the route map
//           msg, the incoming message
// RETURN    None
//--------------------------------------------------------------------------
void PbrFillValues(Node* node,
                   RouteMapValueSet* valueSet,
                   const Message *msg)
{
    IpHeaderType* ipHeader = (IpHeaderType*) msg->packet;

    // match level 3 length of the packet
    valueSet->matchLength = IpHeaderGetIpLength(ipHeader->ip_v_hl_tos_len);

    // match destination and source address
    // Note: Match the source and destination IP address that is permitted
    //  by one or more standard or extended access lists.
    // So no other check is done by access lists.

    valueSet->matchDestAddress = ipHeader->ip_dst;
    valueSet->matchSrcAddress = ipHeader->ip_src;

    if (DEBUG)
    {
        // printing the values entered for matching
        char srcAddr[MAX_STRING_LENGTH];
        char destAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(valueSet->matchDestAddress, destAddr);
        IO_ConvertIpAddressToString(valueSet->matchSrcAddress, srcAddr);

        printf("\nValues entered by PBR for matching at node %u\n",
            node->nodeId);
        printf("Level 3 length of the packet %d\n", valueSet->matchLength);
        printf("Source address of the packet %s\n", srcAddr);
        printf("Dest   address of the packet %s\n", destAddr);
    }
}



//--------------------------------------------------------------------------
// FUNCTION  PbrRouteMapTest
// PURPOSE   Match with the route map
// ARGUMENTS node, node concerned
//           valueSet, Structure for passing values to the route map
//           entry, the route map entry
// RETURN    BOOL, returns the match and route map type result
//--------------------------------------------------------------------------
BOOL PbrRouteMapTest(Node* node,
                     RouteMapValueSet* valueSet,
                     RouteMapEntry* entry)
{
    BOOL result = RouteMapMatchEntry(node, entry, valueSet);

    if (result && (entry->type == ROUTE_MAP_PERMIT))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  PbrSetPrecedence
// PURPOSE   Set the values of precedence for pbr
// ARGUMENTS node, node concerned
//           valueSet, Structure for passing values to the route map
//           ipHeader, the IP header
//           setValues, the structure for passing the set values
// RETURN    BOOL, whether the values are set or not
//--------------------------------------------------------------------------
static
BOOL PbrSetPrecedence(Node* node,
                      RouteMapValueSet* valueSet,
                      IpHeaderType* ipHeader,
                      PbrSetValues* setValues,
                      BOOL isLocal,
                      int incomingInterface)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    // Set the precedence values
    if (valueSet->setIpPrecedence != NO_PREC_DEFINED)
    {
        int precVal = IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len);
        int newPrec = 0;
        precVal = precVal & PBR_NULL_PRECED;

        switch (valueSet->setIpPrecedence)
        {
            case ROUTINE:
            {
                newPrec = IPTOS_PREC_ROUTINE;
                break;
            }
            case PRIORITY:
            {
                newPrec = IPTOS_PREC_PRIORITY;
                break;
            }
            case MM_IMMEDIATE:
            {
                newPrec = IPTOS_PREC_IMMEDIATE;
                break;
            }
            case FLASH:
            {
                newPrec = IPTOS_PREC_FLASH;
                break;
            }
            case FLASH_OVERRIDE:
            {
                newPrec = IPTOS_PREC_FLASHOVERRIDE;
                break;
            }
            case CRITICAL:
            {
                newPrec = IPTOS_PREC_CRITIC_ECP;
                break;
            }
            case INTERNET:
            {
                newPrec = IPTOS_PREC_INTERNETCONTROL;
                break;
            }
            case NETWORK:
            {
                newPrec = IPTOS_PREC_NETCONTROL;
                break;
            }
            default:
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "node %u: Improper precedence value "
                    "for precedence\n", node->nodeId);
                ERROR_ReportError(errString);
                break;
            }
        } // switch

        precVal = precVal | newPrec;
        IpHeaderSetTOS(&(ipHeader->ip_v_hl_tos_len), precVal);
        setValues->precedence = IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len);

        // Updating the precedence stat
        if (ip->isPBRStatOn)
        {
            if (isLocal)
            {
                ip->pbrStat.packetPrecSet++;
            }
            else
            {
                ip->interfaceInfo[incomingInterface]->
                    pbrStat.packetPrecSet++;
            }
        }


        if (DEBUG)
        {
            // printing the values
            printf("Setting precedence: %d\n", valueSet->setIpPrecedence);
        }
        return TRUE;

    } // if
    else
    {
        return FALSE;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  PbrSetIntfNxtHop
// PURPOSE   Use the values of route map for PBR
// ARGUMENTS node, node concerned
//           valueSet, Structure for passing values to the route map
//           interfaceIndex, the incoming interface
//           setValues, the structure for passing the set values
// RETURN    BOOL, whether the values are set or not
//--------------------------------------------------------------------------
static
BOOL PbrSetIntfNxtHop(Node* node,
                      RouteMapValueSet* valueSet,
                      PbrSetValues* setValues,
                      BOOL isDefault)
{
    BOOL noIp = FALSE;
    BOOL noInterface = FALSE;
    NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;
    NetworkForwardingTable *rt = &ip->forwardTable;
    RouteMapSetIPNextHop* valueSetNextHop = NULL;
    RouteMapIntf* valueSetIntf = NULL;
    int intf = -1;

    if (isDefault)
    {
        // If its default then we set the default values.
        valueSetNextHop = valueSet->setDefNextHop;
        valueSetIntf = valueSet->setDefIntf;
    }
    else
    {
        valueSetNextHop = valueSet->setIpNextHop;
        valueSetIntf = valueSet->setIntf;
    }

    if ((!valueSetNextHop) && (!valueSetIntf))
    {
        // If both the next-hop and the interface are not set then we
        //  return with FALSE value.
        return FALSE;
    }

    // set ip next-hop, if enabled
    if (valueSetNextHop)
    {
        RouteMapAddressList* nextHop = valueSetNextHop->addressHead;
        BOOL up = FALSE;
        noIp = TRUE;

        // go for the next hop who is up
        while (nextHop)
        {
            int i = 0;
            intf = NetworkIpGetInterfaceIndexForNextHop(
                node,nextHop->address);

            if (intf == -1)
            {
                // we go for the next nexthop
                nextHop = nextHop->next;
                continue;
            }

            for (i = 0; i < rt->size; i++)
            {
                if (rt->row[i].interfaceIndex == intf)
                {
                    if (rt->row[i].interfaceIsEnabled)
                    {
                        // if the interface is up
                        setValues->nextHop = nextHop->address;
                        up = TRUE;
                        break;
                    }
                }
            }

            if (up)
            {
                if (DEBUG)
                {
                    // printing the values
                    char address[PBR_ADDRESS_STRING];
                    IO_ConvertIpAddressToString(setValues->nextHop,
                        address);
                    printf("Setting next hop: %s\n", address);
                }

                break;
            }

            nextHop = nextHop->next;
        } // while

        if (up == FALSE)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node %u: All the next hop's set by PBR"
                " are down.\n", node->nodeId);
            ERROR_ReportError(errString);
        } // if
    } // if (valueSetNextHop)

    // set interface if enabled
    if (valueSetIntf)
    {
        noInterface = TRUE;
        BOOL up = FALSE;

        while (valueSetIntf)
        {
            int i = 0;

            if (valueSetIntf->intf == -1)
            {
                int intf = RtParseGetInterfaceIndex(node,
                    valueSetIntf->type,
                    valueSetIntf->intfNumber);

                if (intf == -1)
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "node %u: Invalid interface set by "
                        "PBR.\n", node->nodeId);
                    ERROR_ReportError(errString);
                }

                valueSetIntf->intf = intf;
            }

            // its considered that all the next hops who are up have
            //  their entries in the forwarding table.
            for (i = 0; i < rt->size; i++)
            {
                if ((rt->row[i].interfaceIndex == valueSetIntf->intf) &&
                    rt->row[i].interfaceIsEnabled)
                {
                    setValues->intfIndex = rt->row[i].interfaceIndex;
                    up = TRUE;
                    break;
                }
            }

            if (up)
            {
                if (DEBUG)
                {
                    // printing the values
                    printf("Setting interface: %d\n",
                        setValues->intfIndex);
                }
                break;
            }

            valueSetIntf = valueSetIntf->next;
        } // while

        if (!up)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node %u: All the interface's set by "
                "PBR are down. PBR action aborted.\n", node->nodeId);
            ERROR_ReportWarning(errString);
            return FALSE;
        } // if
    } // if (valueSetIntf)
    else
    {
        // if no interface is set, then we get the interface from the
        //  next hop and set that
        setValues->intfIndex = intf;
    }

    return TRUE;
}


//--------------------------------------------------------------------------
// FUNCTION  PbrSetEntry
// PURPOSE   Use the values of route map for PBR
// ARGUMENTS node, node concerned
//           valueSet, Structure for passing values to the route map
//           msg, The message that has reached
//           interfaceIndex, the incoming interface
//           setValues, the structure for passing the set values
//           isDefault, is it a default call
// RETURN    BOOL, if values are set
//--------------------------------------------------------------------------
static
BOOL PbrSetEntry(Node* node,
                 RouteMapValueSet* valueSet,
                 Message* msg,
                 int interfaceIndex,
                 PbrSetValues* setValues,
                 BOOL isDefault,
                 BOOL isLocal,
                 BOOL* isPrecSet)
{
    IpHeaderType* ipHeader = (IpHeaderType *) msg->packet;

    // We need to follow the order as mentioned in the above ref doc.

    if (DEBUG)
    {
        // printing the values
        printf("Node: %u PBR setting values...\n", node->nodeId);
    }

    // Set the precedence values.
    *isPrecSet = PbrSetPrecedence(node, valueSet, ipHeader, setValues,
        isLocal, interfaceIndex);

    // Set the next hop and the interface
    if (PbrSetIntfNxtHop
        (node, valueSet, setValues, isDefault))
    {
        return TRUE;
    }
    else
    {
        // If the interface and the next hop is not set we need to follow
        //  the normal flow, even if the precedence is set so we set FALSE.
        return FALSE;
    }
}



//--------------------------------------------------------------------------
// FUNCTION    PbrTrace
// PURPOSE     Print trace for the access list
// ARGUMENTS   node, node concerned
//             interfaceIndex, interface index
//             setValues, the values set by PBR
// RETURN      None
//--------------------------------------------------------------------------
static
void PbrTrace(Node* node,
              int interfaceIndex,
              PbrSetValues* setValues,
              const Message* msg)
{
    static int count = 0;
    FILE *fp = NULL;
    char address[PBR_ADDRESS_STRING];
    float simTime = 0.0;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    if (count == 0)
    {
        fp = fopen("PBRTrace.txt", "w");
        ERROR_Assert(fp != NULL,
            "PBRTrace.txt: file initial open error.\n");

        fprintf(fp, "\n\nPBR TRACE FILE\n");
        fprintf(fp, "(Packets policy routed by PBR)\n\n");
        fprintf(fp, "---------------------------------------------------\n");
        fprintf(fp, "Abbreviations and format used in this file:\n");
        fprintf(fp, "1)   Node ID\n");
        fprintf(fp, "2)   Interface index\n");
        fprintf(fp, "3)   Time(sec)\n");
        fprintf(fp, "4)   Packet source address\n");
        fprintf(fp, "5)   Packet destination address\n");
        fprintf(fp, "6)   Local PBR (Y..its local / N its not local)\n");
        fprintf(fp, "7)   Precedence value set\n");
        fprintf(fp, "8)   Next hop address value set\n");
        fprintf(fp, "9)   Interface value set\n");
        fprintf(fp, "10)  Default next hop address value set\n");
        fprintf(fp, "11)  Default interface value set\n");
        fprintf(fp, "---------------------------------------------------\n");
        fprintf(fp, "\n");
        fprintf(fp, "\n");
        fprintf(fp, "\n");
    }
    else
    {
        fp = fopen("PBRTrace.txt", "a");
        ERROR_Assert(fp != NULL,
            "PBRTrace.txt: file open error.\n");
    }

    count++;

    fprintf(fp, "\n%u ", node->nodeId);

    if (interfaceIndex == PBR_NO_INTERFACE)
    {
        fprintf(fp, "NA");
    }
    else
    {
        fprintf(fp, "%d ", interfaceIndex);
    }

    simTime = node->getNodeTime() / ((float) SECOND);
    fprintf(fp, "%15f ", simTime);

    IO_ConvertIpAddressToString(ipHeader->ip_src, address);
    fprintf(fp, "%15s ", address);

    IO_ConvertIpAddressToString(ipHeader->ip_dst, address);
    fprintf(fp, "%15s ", address);

    if (ip->local)
    {
        fprintf(fp, "Y ");
    }
    else
    {
        fprintf(fp, "N ");
    }

    fprintf(fp, "%d ", setValues->precedence);

    IO_ConvertIpAddressToString(setValues->nextHop, address);
    fprintf(fp, "%15s ", address);

    fprintf(fp, "%d ", setValues->intfIndex);

    IO_ConvertIpAddressToString(setValues->defNextHop, address);
    fprintf(fp, "%15s ", address);

    fprintf(fp, "%d ", setValues->defIntfIndex);


    fclose(fp);
}



//--------------------------------------------------------------------------
// FUNCTION  PbrAction
// PURPOSE   All the PBR related work is done here
// ARGUMENTS node, node concerned
//           msg, The message that has reached
//           interfaceIndex, the incoming interface
//           rMap, the route map associated
//           setValues, the structure for passing the set values
//           isDefault, is it a default call
// RETURN    BOOL, true if matched
//--------------------------------------------------------------------------
static
BOOL PbrAction(Node* node,
               Message* msg,
               int interfaceIndex,
               RouteMap* rMap,
               PbrSetValues* setValues,
               BOOL isDefault,
               BOOL isLocal,
               BOOL* isPrecSet)
{
    BOOL flag = FALSE;
    NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;
    RouteMapEntry* entry = rMap->entryHead;

    // pass it to route map for analysis
    while (entry)
    {
        RouteMapValueSet* valueSet = RouteMapInitValueList();

        // extract the fields from the incoming packets and
        //  fill in the route map value list
        PbrFillValues(node, valueSet, msg);

        if (DEBUG)
        {
            // printing the route map entry
            RouteMapPrintRMapEntry(node, entry);
        }

        // If the route map matches, we take the set values else we move
        //  to the next route map entry
        if (PbrRouteMapTest(node, valueSet, entry))
        {
            // set the values for the route map
            RouteMapSetEntry(node, entry, valueSet);

            // use the values of route map for PBR
            if (!PbrSetEntry(node, valueSet, msg, interfaceIndex,
                setValues, isDefault, isLocal, isPrecSet))
            {
                // Free the value list
                MEM_free(valueSet);
                return FALSE;
            }

            if (ip->pbrTrace)
            {
                PbrTrace(node, interfaceIndex, setValues, msg);
            }

            // The route map entry has matched so we return
            // Free the value list
            MEM_free(valueSet);
            return TRUE;
        }

        // Free the value list
        MEM_free(valueSet);

        // Move to the next route map entry
        entry = entry->next;
    }

    return flag;
}


//--------------------------------------------------------------------------
// FUNCTION  PbrInvoke
// PURPOSE   Call PBR
// ARGUMENTS node, node concerned
//           msg, The message that has reached
//           incomingInterface, the incoming interface
//           rMap, the route map associated
//           outgoingInterface, pointer to the outgoing interface
//           isDefault, is it a default call
// RETURN    BOOL, true if action taken
//--------------------------------------------------------------------------
BOOL PbrInvoke(Node* node,
               Message* msg,
               NodeAddress previousHopAddress,
               int incomingInterface,
               RouteMap* rMap,
               BOOL isDefault,
               BOOL isLocal)
{
    BOOL flag = FALSE;
    PbrSetValues* setValues =
        (PbrSetValues*) RtParseMallocAndSet(sizeof(PbrSetValues));
    BOOL isPrecSet = FALSE;

    if (DEBUG)
    {
        if (isLocal)
        {
            printf("\nNode: %u, Invoking local PBR...\n",
                node->nodeId);
        }
        else
        {
            printf("\nNode: %u, Invoking PBR...\n", node->nodeId);
        }
    }

    flag =
        PbrAction(node, msg, incomingInterface, rMap, setValues, isDefault,
        isLocal, &isPrecSet);

    // Here the stats are updated only when PBR is absolutely responsible for
    //  routing the packets.
    if (!((flag == FALSE) && (isPrecSet == TRUE)))
    {
        PbrUpdateStat(node, flag, incomingInterface, isLocal);
    }

    if (flag)
    {
        // The packet is passed to the mac layer
        // Note: If just the interface is SET, then its passed with next hop
        //  equal to 0.0.0.0
        if (setValues->nextHop == 0)
        {
            IpHeaderType* ipHeader = (IpHeaderType*) msg->packet;
            // We set the next hop to the destination address.
            // 25 Aug, 2004
            // Note: This code will be updated after the 3.8 release. ARP
            //  will be used for figuring out the next hop.
            setValues->nextHop = ipHeader->ip_dst;

            if (DEBUG)
            {
                printf("Node: %u, No next hop set in PBR.\n",
                        node->nodeId);
            }
        }

        NetworkIpSendPacketToMacLayer(node,
            msg,
            setValues->intfIndex,
            setValues->nextHop);

        if (DEBUG)
        {
            // printing the values
            char nhop[20];
            IO_ConvertIpAddressToString(setValues->nextHop, nhop);

            printf("Node: %u, Packet passed to NetworkIpSendPacketToMacLayer"
                " with next hop %s,\n    interface index %d at time %f "
                "sec.\n",node->nodeId, nhop, setValues->intfIndex,
                node->getNodeTime()/((float)SECOND));
        }

        MEM_free(setValues);
        return TRUE;
    }

    if (DEBUG)
    {
        printf("Node: %u, PBR did not match at time %f "
            "sec.\n",node->nodeId, node->getNodeTime()/((float)SECOND));
    }

    MEM_free(setValues);
    return FALSE;
}



//--------------------------------------------------------------------------
// FUNCTION  PbrFinalize
// PURPOSE   Policy routing functionalities handled during termination.
//              Here we print the various PBR statistics.
// ARGUMENTS node, current node
// RETURN    None
//--------------------------------------------------------------------------
void PbrFinalize(Node* node)
{
    int i = 0;
    char buf[MAX_STRING_LENGTH];
    PbrStat* stat = NULL;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    stat = &(ip->interfaceInfo[i]->pbrStat);

    ERROR_Assert(stat != NULL, "PBR interface stats not initialized.\n");

    // print interface-wise
    for (i = 0; i < node->numberInterfaces; i++)
    {
        stat = &(ip->interfaceInfo[i]->pbrStat);

        ERROR_Assert(stat != NULL, "PBR stats not initialized.\n");

        sprintf(buf, "Packet Policy routed  = %u",
            stat->packetsPolicyRouted);
        IO_PrintStat(node, "Network", "PBR",
            ip->interfaceInfo[i]->ipAddress, -1, buf);

        sprintf(buf, "Packet precedence set = %u",
            stat->packetPrecSet);
        IO_PrintStat(node, "Network", "PBR",
            ip->interfaceInfo[i]->ipAddress, -1, buf);
    }

    stat = NULL;
    stat = &(ip->pbrStat);

    ERROR_Assert(stat != NULL, "PBR IP stats not initialized.\n");

    sprintf(buf, "Packet precedence set by LOCAL = %u",
        stat->packetPrecSet);
    IO_PrintStat(node, "Network", "PBR", ANY_DEST, -1, buf);

    sprintf(buf, "Packet Policy routed by LOCAL = %u",
        stat->packetsPolicyRoutedLocal);
    IO_PrintStat(node, "Network", "PBR", ANY_DEST, -1, buf);

    sprintf(buf, "Packet not Policy routed = %u",
        stat->packetsNotPolicyRouted);
    IO_PrintStat(node, "Network", "PBR", ANY_DEST, -1, buf);
}



//--------------------------------------------------------------------------
// FUNCTION  PbrUpdateStat
// PURPOSE   Update the policy routing stats
// ARGUMENTS node, current node
//           flag, the result of PBR action
//           incomingInterface, the interface index
// RETURN    None
//--------------------------------------------------------------------------
void PbrUpdateStat(Node* node,
                   BOOL flag,
                   int incomingInterface,
                   BOOL isLocal)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    if (ip->isPBRStatOn)
    {
        if (isLocal && flag)
        {
            ip->pbrStat.packetsPolicyRoutedLocal++;
        }
        else if (flag)
        {
            ip->interfaceInfo[incomingInterface]->
                pbrStat.packetsPolicyRouted++;
        }
        else
        {
            ip->pbrStat.packetsNotPolicyRouted++;
        }
    }
}




//--------------------------------------------------------------------------
// FUNCTION  PbrDebug
// PURPOSE   A trace file generator
// ARGUMENTS
// RETURN    None
//--------------------------------------------------------------------------
void PbrDebug(Node* node,
              IpHeaderType* ipHeader,
              int incomingInterface,
              NodeAddress nextHop,
              int outgoingInterface)
{
    static int count = 0;
    FILE *fp = NULL;
    char address[PBR_ADDRESS_STRING];
    float simTime = 0.0;

    if (!PBR_PACKET_DEBUG)
    {
        return;
    }

    if (count == 0)
    {
        fp = fopen("PacketFlow.txt", "w");
        ERROR_Assert(fp != NULL,
            "PacketFlow.txt: file initial open error.\n");

        fprintf(fp, "\n\nPACKET TRACE FILE\n");
        fprintf(fp, "--------------------------------------------------\n");
        fprintf(fp, "Abbreviations and format used in this file:\n");
        fprintf(fp, "1)   Node ID\n");
        fprintf(fp, "2)   In Interface index\n");
        fprintf(fp, "3)   Time(sec)\n");
        fprintf(fp, "4)   Packet source address\n");
        fprintf(fp, "5)   Packet destination address\n");
        fprintf(fp, "6)   Precedence value set\n");
        fprintf(fp, "7)   Next hop address value set\n");
        fprintf(fp, "8)   Out Interface value set\n");
        fprintf(fp, "---------------------------------------------------\n");
        fprintf(fp, "\n");
        fprintf(fp, "\n");
        fprintf(fp, "\n");
    }
    else
    {
        fp = fopen("PacketFlow.txt", "a");
        ERROR_Assert(fp != NULL,
            "PBRTrace.txt: file open error.\n");
    }

    count++;

    fprintf(fp, "\n%u ", node->nodeId);

    fprintf(fp, "%d ", incomingInterface);

    simTime = node->getNodeTime() / ((float) SECOND);
    fprintf(fp, "%15f ", simTime);

    IO_ConvertIpAddressToString(ipHeader->ip_src, address);
    fprintf(fp, "%15s ", address);

    IO_ConvertIpAddressToString(ipHeader->ip_dst, address);
    fprintf(fp, "%15s ", address);

    fprintf(fp, "%d ", IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len));

    IO_ConvertIpAddressToString(nextHop, address);
    fprintf(fp, "%15s ", address);

    fprintf(fp, "%d ", outgoingInterface);

    fclose(fp);
}
