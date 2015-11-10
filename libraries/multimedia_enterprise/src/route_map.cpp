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
// File     : route_map.cpp
// Objective: Source file for route_map (CISCO implementation
//              of route map)
// Reference: CISCO document Set
//--------------------------------------------------------------------------

// REFERENCES:-
// Match command: http://www.cisco.com/univercd/cc/td/doc/product/
//                  software/ios122/122cgcr/fiprrp_r/ind_r/1rfindp1.htm
// Set command and route map: http://www.cisco.com/univercd/cc/td/doc/
//                  product/software/ios122/122cgcr/fiprrp_r/ind_r/
//                  1rfindp2.htm#xtocid0
//
//

// Please note that:-
// 1) No check is made for declaration of Access lists when called from
//      route map. As said in Cisco doc, only the validity for number ACL
//      is made
// 2) No criteria for checking interface numbers was found, so a
//      string compare is dome for matching.
// 3) For Set levels the default value depends on the routing protcol
//      but this is initialized here. The caller function has to
//      take care of that. Same for default metric.
// 4) No commas are to be entered in between any numerical quantity.
// 5) The Cisco docs state that the following set statements are evaluated
//      in the following order:
//      set ip next-hop
//      set interface
//      set ip default next-hop
//      set default interface
//  If all of them are defined. This has to be arranged in the correct order
//      by the caller functionality depending on the above available
//      commands.
// 6) No for the match and set commands, leaving route map, remove the
//      corresponding command, if available, only from the last entry after
//      ensuring that the command matches in parameters as well.
// 7) If a match occurs, we break off. If its permit, then Set is executed.
//      And for no match, the route map proceeds to the next entry or
//      route map, whichever is applicable.



// Organization of this file <route_map.c> :-
//  utility functions for route map
//  match functions
//  set functions
//  parse set statements
//  parse match statements
//  free functions (match and set commands)
//  parse and execution of no commands
//  parse route map statements
//  route map interface functions
//  route map IOS commands <show commands>



// include files
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "api.h"
#include "qualnet_error.h"
#include "network_ip.h"
#include "network_access_list.h"
#include "route_map.h"
#include "route_parse_util.h"



// set this as 1 if you want to debug
#define ROUTE_MAP_DEBUG 0

//--------------------------------------------------------------------------
// FUNCTION  RouteMapNewEntry
// PURPOSE   Creating a blank new entry structure
// ARGUMENTS void
// RETURN    Pointer to the newly created entry
//--------------------------------------------------------------------------
static
RouteMapEntry* RouteMapNewEntry()
{
    RouteMapEntry* templ
        = (RouteMapEntry*) RtParseMallocAndSet(sizeof(RouteMapEntry));

    // this default is kept for no route maps
    templ->seq = -1;
    templ->type = ROUTE_MAP_PERMIT;

    return templ;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapCreateTemplate
// PURPOSE   Creating a blank route map structure
// ARGUMENTS newEntry, the new entry to be plugged
//           tag, the name of the route map
// RETURN    Pointer to the newly created route map
//--------------------------------------------------------------------------
static
RouteMap* RouteMapCreateTemplate(RouteMapEntry* newEntry, char* tag)
{
    RouteMap* templ = (RouteMap *) RtParseMallocAndSet(sizeof(RouteMap));

    templ->entryHead = newEntry;
    templ->entryTail = newEntry;
    templ->mapTag = RtParseStringAllocAndCopy(tag);

    return templ;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapPlugSeq
// PURPOSE   Check the format of seq number
// ARGUMENTS node, the current node
//           seq, the token as received
//           originalString, the string thats parsed
// RETURN    int seq number
//--------------------------------------------------------------------------
static
int RouteMapPlugSeq(Node* node, char* seq, const char* originalString)
{
    int val = -1;

    if ((val = RtParseValidateAndConvertToInt(seq)) == -1)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nInappropriate sequence number. "
            "\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // it gotto be an integer
    return val;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapPlugEntry
// PURPOSE   Plug the new entry at the appropriate position in the route map
// ARGUMENTS node, node concerned
//           tempRMap, the route map where the entry would be plugged
//           newEntry, the entry to be plugged
//           originalString, the original string as in the config file
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapPlugEntry(Node* node,
                       RouteMap* rMap,
                       RouteMapEntry* newEntry,
                       const char* originalString)
{
    RouteMapEntry* tempEntry = rMap->entryHead;
    RouteMapEntry* prev = tempEntry;
    BOOL flag = TRUE;

    while (tempEntry && (tempEntry->seq < newEntry->seq))
    {
        flag = FALSE;
        prev = tempEntry;
        tempEntry = tempEntry->next;
    }

    // cant have same seq number for 2 route maps
    if ((tempEntry) && (tempEntry->seq == newEntry->seq))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nSame seq number entered for two"
            " route maps.\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    if (flag)
    {
        newEntry->next = tempEntry;
        rMap->entryHead = newEntry;
        return;
    }

    newEntry->next = tempEntry;
    prev->next = newEntry;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapCheckForMaxValue
// PURPOSE   Get the corresponding route map pointer from the list or NULL
//              if not founc.
// ARGUMENTS token, the token holding the value
//           valReturn, pointer to the value that is passed back
//              to the caller
// RETURN    TRUE and the value if its a proper value, else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapCheckForMaxValue(char* token, unsigned int* valReturn)
{
    // check for all numerics
    char s[MAX_STRING_LENGTH];
    char s1[MAX_STRING_LENGTH] = "4294967295";
    char *ptr = NULL;

    strcpy(s, token);

    ptr = s;

    while (*ptr != '\0')
    {
        if (!isdigit((int)*ptr))
        {
            return FALSE;
        }

        ptr++;
    }

    ptr = s;

    while (*ptr == '0')
    {
        ptr++;
    }

    if (strlen(ptr) < 10)
    {
        *valReturn = (unsigned int) atoi(token);
        return TRUE;
    }

    if (strcmp(ptr, s1) > 0)
    {
        return FALSE;
    }
    else
    {
        *valReturn = (unsigned int) atoi(token);
        return TRUE;
    }

}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapEntryFindBySeq
// PURPOSE   Get the corresponding route map with the mentioned sequence
//              number
// ARGUMENTS node, corresponding node
//           routeMap, head of the route map of same tag from where we will
//              search for the route map with the specifed tag
// RETURN    The corresponding route map entry or NULL if not found
//--------------------------------------------------------------------------
RouteMapEntry* RouteMapEntryFindBySeq(Node* node, RouteMap* rMap, int seq)
{
    RouteMapEntry* temp = rMap->entryHead;

    while (temp)
    {
        if (temp->seq == seq)
        {
            return temp;
        }
        temp = temp->next;
    }

    return NULL;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchTokenForMatchCommand
// PURPOSE   Find the type of Match command with the input token
// ARGUMENTS node, the current node
//           token1, first input token
//           token2, second input token
//           [Please note that there can be atmost two keywords after the
//              match command so we take two tokens for matching.]
// RETURN    Type of Match command or return -1 as fault value.
//--------------------------------------------------------------------------
static
RouteMapMatchType RouteMapMatchTokenForMatchCommand(Node* node,
                                                    char* token1,
                                                    char* token2)
{
    if (!RtParseStringNoCaseCompare(token1, "as-path"))
    {
        // Developer Comments: this is not implemented
        return AS_PATH;
    }
    else if (!RtParseStringNoCaseCompare(token1, "community"))
    {
        // Developer Comments: this is not implemented
        return COMMUNITY;
    }
    else if (!RtParseStringNoCaseCompare(token1, "extcommunity"))
    {
        // Developer Comments: this is not implemented
        return EXTCOMMUNITY;
    }
    else if (!RtParseStringNoCaseCompare(token1, "interface"))
    {
        return INTERFACE;
    }
    else if (!RtParseStringNoCaseCompare(token1, "ip"))
    {
        if (!RtParseStringNoCaseCompare(token2, "address"))
        {
            return IP_ADDRESS;
        }
        else if (!RtParseStringNoCaseCompare(token2, "next-hop"))
        {
            return IP_NEXT_HOP;
        }
        else if (!RtParseStringNoCaseCompare(token2, "route-source"))
        {
            return IP_ROUTE_SRC;
        }
        else
        {
            return (RouteMapMatchType) -1;
        }
    }
    else if (!RtParseStringNoCaseCompare(token1, "length"))
    {
        return LENGTH;
    }
    else if (!RtParseStringNoCaseCompare(token1, "metric"))
    {
        return METRIC;
    }
    else if (!RtParseStringNoCaseCompare(token1, "route-type"))
    {
        return ROUTE_TYPE;
    }
    else if (!RtParseStringNoCaseCompare(token1, "tag"))
    {
        return TAG;
    }
    else
    {
        return (RouteMapMatchType) -1;
    }
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapFindByName
// PURPOSE   Get the corresponding route map pointer from the list or NULL
//              if not found.
// ARGUMENTS node, corresponding node
//           routeMapName, name of the route map to be looked
// RETURN    The corresponding route map or NULL if not found
//--------------------------------------------------------------------------
RouteMap* RouteMapFindByName(Node* node, char* routeMapName)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    RouteMap* routeMap = NULL;

    if (!ip->routeMapList)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\n"
            "No route map, '%s', defined. Route map search"
            " aborted...\n", node->nodeId, routeMapName);
        ERROR_ReportWarning(errString);
        return NULL;
    }

    routeMap = ip->routeMapList->head;

    if (routeMap == NULL)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\n"
            "No route map, '%s', defined. Route map search"
            " aborted...\n", node->nodeId, routeMapName);
        ERROR_ReportWarning(errString);
        return NULL;
    }

    while (routeMap)
    {
        if (!RtParseStringNoCaseCompare(routeMapName, routeMap->mapTag))
        {
            // matches so return
            return routeMap;
        }
        routeMap = routeMap->next;
    }

    // no match
    return NULL;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapAddMatch
// PURPOSE   Plug the match command in the entry list
// ARGUMENTS entry, the particular route map where the match command is to
//              be plugged
//           newMatch, match command to be plugged
// RETURN    Nothing to return
//--------------------------------------------------------------------------
static
void RouteMapAddMatch(RouteMapEntry* entry, RouteMapMatchCmd* newMatch)
{
    if (entry->matchHead == NULL)
    {
        entry->matchHead = newMatch;
        entry->matchTail = newMatch;
    }
    else
    {
        entry->matchTail->next = newMatch;
        entry->matchTail = newMatch;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapAddSet
// PURPOSE   Plug the set command in the entry list
// ARGUMENTS entry, the particular route map where the set command is to
//              be plugged
//           newSet, set command to be plugged
// RETURN    Nothing to return
//--------------------------------------------------------------------------
static
void RouteMapAddSet(RouteMapEntry* entry, RouteMapSetCmd* newSet)
{
    if (entry->setHead == NULL)
    {
        entry->setHead = newSet;
        entry->setTail = newSet;
    }
    else
    {
        entry->setTail->next = newSet;
        entry->setTail = newSet;
    }
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapInitValueList
// PURPOSE   Allocate and initialize a new value set
// ARGUMENTS void
// RETURN    Pointer to the newly created value set.
// COMMENTS  Use this function when you want a new value set for passing
//              match values and retrieving set values.
//
//           For Set level the default value depends on the routing protcol
//              but this is initialized here.
//           For Set metric the default value is to be learned dynamically.
//           For Set next-hop the default value would be the next hop add.
//           For Set tag the default would be to forward the tag of the
//              source routing protocol.
//           The caller function has to take care of above mentioned facts.
//--------------------------------------------------------------------------
RouteMapValueSet* RouteMapInitValueList(void)
{
    RouteMapValueSet* valueSet =
        (RouteMapValueSet*) RtParseMallocAndSet(sizeof(RouteMapValueSet));

    valueSet->setLocalPref = ROUTE_MAP_DEF_LOCAL_PREF;
    valueSet->matchRtType = (RouteMapRouteType) -1;
    valueSet->matchDefIntfType = INVALID_TYPE;
    valueSet->setIpPrecedence = NO_PREC_DEFINED;

    return valueSet;
}



//--------------    MATCH FUNCTIONS    -------------------------------------



//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchFuncInterface
// PURPOSE   Matching the Interface params
// ARGUMENTS node, node concerned
//           param, the match command params
//           valueSet, the values as in the route
// RETURN    TRUE if it matches or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapMatchFuncInterface(Node* node,
                                void* param,
                                RouteMapValueSet* valueSet)
{
    RouteMapIntf* intfParam = (RouteMapIntf*) param;

    ERROR_Assert(valueSet->matchDefIntfType != -1,
        "RouteMapMatchFuncInterface: Expected interface"
        " type as param to match with route map criteria.\n");

    ERROR_Assert(valueSet->matchInterfaceIndex,
        "RouteMapMatchFuncInterface: Expected interface index as param "
        "to match with route map criteria.\n");

    // check for all the interface param set
    while (intfParam)
    {
        if ((intfParam->type == valueSet->matchDefIntfType) &&
            (!RtParseStringNoCaseCompare(
                intfParam->intfNumber, valueSet->matchInterfaceIndex)))
        {
            return TRUE;
        }

        // go to the next set
        intfParam = intfParam->next;
    }

    return FALSE;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchFuncIPAddress
// PURPOSE   Match the IP address with the specified access lists
// ARGUMENTS node, node concerned
//           param, the match command params
//           valueSet, the values as in the route
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapMatchFuncIPAddress(Node* node,
                                void* param,
                                RouteMapValueSet* valueSet)
{
    RouteMapAclList* aclList = (RouteMapAclList*) param;

    BOOL flag = FALSE;


    ERROR_Assert(valueSet->matchDestAddress,
        "RouteMapMatchFuncIPAddress: Expected destination address as "
        "param to match with route map criteria.\n");

    while (aclList && !flag)
    {
        // To distribute any routes that have a destination network number
        //  address that is permitted by a standard access list,
        //  an extended access list
        flag = AccessListVerifyRouteMap(
                     node,
                     aclList->aclId,
                     valueSet->matchSrcAddress,
                     valueSet->matchDestAddress);
        aclList = aclList->next;
    }

    return flag;
}


//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchFuncIPNextHop
// PURPOSE   Match the next hop with the specified access lists
// ARGUMENTS node, node concerned
//           param, the match command params
//           valueSet, the values as in the route
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapMatchFuncIPNextHop(Node* node,
                                void* param,
                                RouteMapValueSet* valueSet)
{
    RouteMapAclList* aclList = (RouteMapAclList*) param;

    NodeAddress destNWAddr = valueSet->matchDestAddNextHop;
    NodeAddress srcNWAddr = valueSet->matchDestAddNextHop;
    BOOL flag = FALSE;

    ERROR_Assert(valueSet->matchDestAddNextHop,
        "RouteMapMatchFuncIPNextHop: Expected destination address as "
        "param to match with route map criteria.\n");

    while (aclList && !flag)
    {
        // To redistribute any routes that have a next hop router address
        //  passed by one of the access lists specified.
        flag = AccessListVerifyRouteMap(node,
            aclList->aclId, srcNWAddr, destNWAddr);
        aclList = aclList->next;
    }

    return flag;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchFuncIPRtSrc
// PURPOSE   Match the route source with the specified access lists
// ARGUMENTS node, node concerned
//           param, the match command params
//           valueSet, the values as in the route
// COMMENTS  its upto the user to decide whether to send the specific
//            dest add or the dest number address
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapMatchFuncIPRtSrc(Node* node,
                              void* param,
                              RouteMapValueSet* valueSet)
{
    RouteMapAclList* aclList = (RouteMapAclList*) param;
    BOOL flag = FALSE;
    NodeAddress destNWAddr = valueSet->matchDestAddrRtSrc;
    NodeAddress srcAddr = 0;

    ERROR_Assert(valueSet->matchDestAddNextHop,
        "RouteMapMatchFuncIPRtSrc: Expected address for route source as "
        "param to match with route map criteria.\n");

    while (aclList && !flag)
    {
        flag = AccessListVerifyRouteMap(node,
            aclList->aclId, srcAddr, destNWAddr);
        aclList = aclList->next;
    }

    return flag;
}


//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchFuncLength
// PURPOSE   Match on the level 3 length of a packet
// ARGUMENTS node, node concerned
//           param, the match command params
//           valueSet, the values as in the route
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapMatchFuncLength(Node* node,
                             void* param,
                             RouteMapValueSet* valueSet)
{
    RouteMapMatchLength* length = (RouteMapMatchLength*) param;

    ERROR_Assert(valueSet->matchLength,
        "RouteMapMatchFuncLength: Expected packet length as "
        "param to match with route map criteria.\n");

    if ((valueSet->matchLength >= length->min) &&
        (valueSet->matchLength <= length->max))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    // its not a  default case, its for having a proper return

}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchFuncMetric
// PURPOSE   Match with the metric specified
// ARGUMENTS node, node concerned
//           param, the match command params
//           valueSet, the values as in the route
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapMatchFuncMetric(Node* node,
                             void* param,
                             RouteMapValueSet* valueSet)
{
    RouteMapMetric* metric = (RouteMapMetric*) param;
    BOOL retVal = FALSE;

    ERROR_Assert(valueSet->matchCost,
        "RouteMapMatchFuncMetric: Expected metric as "
        "param to match with route map criteria.\n");

    if (valueSet->matchCost == metric->metric)
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return retVal;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchFuncRtType
// PURPOSE   Match with the route type
// ARGUMENTS node, node concerned
//           param, the match command params
//           valueSet, the values as in the route
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapMatchFuncRtType(Node* node,
                             void* param,
                             RouteMapValueSet* valueSet)

{
    RouteMapMatchRouteType* rtType = (RouteMapMatchRouteType*) param;
    BOOL retVal = FALSE;

    ERROR_Assert(valueSet->matchRtType != -1,
        "RouteMapMatchFuncRtType: Expected route type as "
        "param to match with route map criteria.\n");

    if (valueSet->matchRtType == rtType->rtType)
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return retVal;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchFuncTag
// PURPOSE   Match with the specified tag
// ARGUMENTS node, node concerned
//           param, the match command params
//           valueSet, the values as in the route
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapMatchFuncTag(Node* node,
                          void* param,
                          RouteMapValueSet* valueSet)
{
    RouteMapMatchTag* tagVal = (RouteMapMatchTag*) param;

    while (tagVal)
    {
        if (tagVal->tag == valueSet->matchTag)
        {
            return TRUE;
        }

        tagVal = tagVal->next;
    }

    return FALSE;
}



// SET FUNCTIIONS FOLLOW

//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncAutoTag
// PURPOSE   Set the specified tag
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, the values to be set in the route
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncAutoTag(Node* node,
                            void* param,
                            RouteMapValueSet* valueSet)
{
    // enable automatic tag calculation
    valueSet->setAutoTag = TRUE;
}


//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncDefIntf
// PURPOSE   Set the specified default interface
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncDefIntf(Node* node,
                            void* param,
                            RouteMapValueSet* valueSet)
{
    // point to the param set
    valueSet->setDefIntf = (RouteMapIntf*)param;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncIntf
// PURPOSE   Set the specified interface
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncIntf(Node* node,
                         void* param,
                         RouteMapValueSet* valueSet)
{
    // point to the param set
    valueSet->setIntf = (RouteMapIntf*)param;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncDefIPNextHop
// PURPOSE   Set the specified default next hop
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncDefIPNextHop(Node* node,
                                 void* param,
                                 RouteMapValueSet* valueSet)
{
    // jus point to the param set
    valueSet->setDefNextHop = (RouteMapSetIPNextHop*) param;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncDefIPNextHopVerfAvail
// PURPOSE   Set IP default next hop verify availability
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncDefIPNextHopVerfAvail(Node* node,
                                          void* param,
                                          RouteMapValueSet* valueSet)
{
    // set ip default next-hop verify-availability
    valueSet->setIPDefNextHopVerAvail = TRUE;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncIPNextHop
// PURPOSE   Set the specified ip next hop
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncIPNextHop(Node* node,
                              void* param,
                              RouteMapValueSet* valueSet)
{
    // jus point to the param set
    valueSet->setIpNextHop = (RouteMapSetIPNextHop*) param;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncIPNextHopVerfAvail
// PURPOSE   Set IP next hop verify availability
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncIPNextHopVerfAvail(Node* node,
                                       void* param,
                                       RouteMapValueSet* valueSet)
{
    // set ip next-hop verify-availability
    valueSet->setIPNextHopVerAvail = TRUE;
}





//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncIPPrecedence
// PURPOSE   Set the specified ip precedence
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncIPPrecedence(Node* node,
                                 void* param,
                                 RouteMapValueSet* valueSet)
{
    //RouteMapPrecedence* precedence =  param;

    valueSet->setIpPrecedence = *((RouteMapPrecedence*)param);
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncLevel
// PURPOSE   Set the specified level
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncLevel(Node* node,
                          void* param,
                          RouteMapValueSet* valueSet)
{
    valueSet->setLevel = *((RouteMapLevel*) param);
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncLocalPref
// PURPOSE   Set the specified local preference
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncLocalPref(Node* node,
                              void* param,
                              RouteMapValueSet* valueSet)
{
    RouteMapSetLocalPref* localPref = (RouteMapSetLocalPref*) param;

    valueSet->setLocalPref = localPref->prefValue;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncMetric
// PURPOSE   Set the specified metric
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncMetric(Node* node,
                           void* param,
                           RouteMapValueSet* valueSet)
{
    RouteMapMetric* metricParam = (RouteMapMetric*) param;

    valueSet->setMetric = (int) metricParam->metric;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncMetricType
// PURPOSE   Set the specified metric
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncMetricType(Node* node,
                               void* param,
                               RouteMapValueSet* valueSet)
{
    valueSet->setMetricType = *((RouteMapMetricType*) param);
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncNextHop
// PURPOSE   Set the specified next hop
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncNextHop(Node* node,
                            void* param,
                            RouteMapValueSet* valueSet)
{
    valueSet->setNextHop = *((NodeAddress*) param);
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetFuncTag
// PURPOSE   Set the specified tag
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapSetFuncTag(Node* node,
                        void* param,
                        RouteMapValueSet* valueSet)
{
    RouteMapSetTag* tag = (RouteMapSetTag*) param;

    valueSet->setTag = tag->tagVal;
}



/////// PARSE FUNCTIONS FOLLOW



//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseSetTag
// PURPOSE   Parsing the next hop param
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           refCriteriaString, the string portion which remains unparsed
// RETURN    Return the next hop
//--------------------------------------------------------------------------
static
RouteMapSetTag* RouteMapParseSetTag(Node* node,
                                    char** criteriaString,
                                    const char* originalString)
{
    RouteMapSetTag* param
        = (RouteMapSetTag*) RtParseMallocAndSet(sizeof(RouteMapSetTag));
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    int numParam = 0;
    unsigned int tag = 0;

    numParam = sscanf(*criteriaString,"%s %s", token1, token2);

    if ((numParam > 1) || (numParam == EOF))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax for 'set tag' "
            "in router config file\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    if (RouteMapCheckForMaxValue(token1, &tag))
    {
        param->tagVal = tag;
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nImpermissible value for 'set tag' "
            "in router config file\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    return param;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseSetNextHop
// PURPOSE   Parsing the next hop param
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           refCriteriaString, the string portion which remains unparsed
// RETURN    Return the next hop address
//--------------------------------------------------------------------------
static
NodeAddress RouteMapParseSetNextHop(Node* node,
                                    char** criteriaString,
                                    const char* originalString)
{
    NodeAddress param = 0;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    int numParam = 0;
    BOOL isNodeId = FALSE;

    numParam = sscanf(*criteriaString,"%s %s", token1, token2);

    if ((numParam > 1) || (numParam == EOF))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax for 'set next-hop' "
            "in router config file\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    IO_ParseNodeIdOrHostAddress(token1, &param, &isNodeId);

    if (isNodeId)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nNode address expected for "
            "'set next-hop' in router config file\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    return param;
}


//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseSetMetricType
// PURPOSE   Parsing the metric type param
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           refCriteriaString, the string portion which remains unparsed
// RETURN    Return the metric type list
//--------------------------------------------------------------------------
static
RouteMapMetricType RouteMapParseSetMetricType(Node* node,
                                              char** criteriaString,
                                              const char* originalString)
{
    RouteMapMetricType param = (RouteMapMetricType) -1;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    int numParam = 0;

    numParam = sscanf(*criteriaString,"%s %s", token1, token2);

    if ((numParam > 1) || (numParam == EOF))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax for 'set metric-type' "
            "in router config file\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    if (!RtParseStringNoCaseCompare(token1, "internal"))
    {
        param = INTERNAL;
    }
    else if (!RtParseStringNoCaseCompare(token1, "external"))
    {
        param = EXTERNAL;
    }
    else if (!RtParseStringNoCaseCompare(token1, "type-1"))
    {
        param = TYPE_1;
    }
    else if (!RtParseStringNoCaseCompare(token1, "type-2"))
    {
        param = TYPE_2;
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nImpermissible metric type for "
            "'set metric-type' in router config file\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    return param;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseSetLocalPref
// PURPOSE   Parsing the local preference parameters
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           refCriteriaString, the string portion which remains unparsed
// RETURN    Return the local pref list
// COMMENT   If this command is missed out by the user, a default value of
//              100 is initialized at the init of value set.
//--------------------------------------------------------------------------
static
RouteMapSetLocalPref* RouteMapParseSetLocalPref(Node* node,
                                                char** criteriaString,
                                                const char* originalString)
{
    RouteMapSetLocalPref* param
        = (RouteMapSetLocalPref*)
        RtParseMallocAndSet(sizeof(RouteMapSetLocalPref));
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    int numParam = 0;
    unsigned int localPref = 0;

    numParam = sscanf(*criteriaString,"%s %s", token1, token2);

    if ((numParam > 1) || (numParam == EOF))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax for 'set "
            "local-preference' in router config file\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    if (RouteMapCheckForMaxValue(token1, &localPref))
    {
        param->prefValue = localPref;
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nImpermissible value for 'set "
            "local-preference' in router config file\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    return param;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseSetLevel
// PURPOSE   Parsing the level param
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           refCriteriaString, the string portion which remains unparsed
// RETURN    Return the level params
//--------------------------------------------------------------------------
static
RouteMapLevel RouteMapParseSetLevel(Node* node,
                                    char** criteriaString,
                                    const char* originalString)
{
    RouteMapLevel param = (RouteMapLevel) -1;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    int numParam = 0;

    numParam = sscanf(*criteriaString,"%s %s", token1, token2);

    if ((numParam > 1) || (numParam == EOF))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax for 'set level' "
            "in router config file\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    if (!RtParseStringNoCaseCompare(token1, "level-1"))
    {
        param = LEVEL_1;
    }
    else if (!RtParseStringNoCaseCompare(token1, "level-2"))
    {
        param = LEVEL_2;
    }
    else if (!RtParseStringNoCaseCompare(token1, "level-1-2"))
    {
        param = LEVEL_1_2;
    }
    else if (!RtParseStringNoCaseCompare(token1, "stub-area"))
    {
        param = STUB_AREA;
    }
    else if (!RtParseStringNoCaseCompare(token1, "backbone"))
    {
        param = BACKBONE;
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax for 'set level' "
            "in router config file\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    return param;
}

//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseSetPrecedence
// PURPOSE   Parsing the precedence param
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           refCriteriaString, the string portion which remains unparsed
// RETURN    Return the precedence list
//--------------------------------------------------------------------------
static
RouteMapPrecedence RouteMapParseSetPrecedence(
    Node* node,
    char** criteriaString,
    const char* originalString)
{
    RouteMapPrecedence param = (RouteMapPrecedence) -1;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    int numParam = 0;
    int precedenceNum = -1;

    numParam = sscanf(*criteriaString,"%s %s", token1, token2);

    if ((numParam > 1) || (numParam == EOF))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax for 'set ip "
            "precedence' in router config file\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // precedence number
    precedenceNum = RtParseValidateAndConvertToInt(token1);

    if ((precedenceNum >= 0) && (precedenceNum < 8))
    {
        param = (RouteMapPrecedence) precedenceNum;
    }
    else
    {
        // precedence name
        if (!RtParseStringNoCaseCompare(token1, "routine"))
        {
            param = ROUTINE;
        }
        else if (!RtParseStringNoCaseCompare(token1, "priority"))
        {
            param = PRIORITY;
        }
        else if (!RtParseStringNoCaseCompare(token1, "immediate"))
        {
            param = MM_IMMEDIATE;
        }
        else if (!RtParseStringNoCaseCompare(token1, "flash"))
        {
            param = FLASH;
        }
        else if (!RtParseStringNoCaseCompare(token1, "flash-override"))
        {
            param = FLASH_OVERRIDE;
        }
        else if (!RtParseStringNoCaseCompare(token1, "critical"))
        {
            param = CRITICAL;
        }
        else if (!RtParseStringNoCaseCompare(token1, "internet"))
        {
            param = INTERNET;
        }
        else if (!RtParseStringNoCaseCompare(token1, "network"))
        {
            param = NETWORK;
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for precedence "
                "in router config file\n'%s'\n",node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
    }

    return param;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseAddress
// PURPOSE   Parsing the IP address list
// ARGUMENTS node, node concerned
//           criteriaString, the string portion which remains unparsed
//           originalString, the original string as in the config file
// RETURN    Return the param with the address list
//--------------------------------------------------------------------------
static
RouteMapSetIPNextHop* RouteMapParseAddress(Node* node,
                                           char** criteriaString,
                                           const char* originalString)
{
    // parse the address params
    // syntax: ip-address [...ip-address]
    char token1[MAX_STRING_LENGTH] = {0};
    BOOL flag = FALSE;
    int numParam = 0;
    RouteMapSetIPNextHop* param = NULL;

    numParam = sscanf(*criteriaString,"%s", token1);

    while (numParam != EOF)
    {
        int position = 0;
        BOOL isNodeId = FALSE;
        RouteMapAddressList* addressList
            = (RouteMapAddressList*)
              RtParseMallocAndSet(sizeof(RouteMapAddressList));
        flag = TRUE;

        IO_ParseNodeIdOrHostAddress(token1,
            &(addressList->address), &isNodeId);

        if (isNodeId)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for node address "
                "in router config file\n'%s'\n",node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        if (param == NULL)
        {
            param = (RouteMapSetIPNextHop *)
                    RtParseMallocAndSet(sizeof(RouteMapSetIPNextHop));
            param->addressHead = param->addressTail = addressList;
        }
        else
        {
            param->addressTail->next = addressList;
            param->addressTail = addressList;
        }

        // get the remaining string
        position = RtParseGetPositionInString(*criteriaString, token1);

        // parse the remaining tokens...address...
        *criteriaString += position;

        numParam = sscanf(*criteriaString,"%s", token1);
    }

    if (!flag)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nMissing next hop address for "
            "'set ip default next-hop' command in Route map.\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    return param;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseInterfaceParams
// PURPOSE   Parsing and initializing interface parameters
// ARGUMENTS node, node concerned
//           criteriaString, the string portion which remains unparsed
//           originalString, the original string as in the config file
// RETURN    Return the parameter list
//--------------------------------------------------------------------------
static
RouteMapIntf* RouteMapParseInterfaceParams(Node* node,
                                           char** criteriaString,
                                           const char* originalString)
{
    // interface syntax:
    //  interface-type interface-number [interface-type interface-number..]
    // getting a new interface block
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    char token3[MAX_STRING_LENGTH] = {0};
    RouteMapIntf* paramHead = NULL;
    RouteMapIntf* paramTail = NULL;

    // check the type
    do
    {
        // checking the first block... interface-type interface-number
        int position = 0;
        RouteMapIntf* param
            = (RouteMapIntf*) RtParseMallocAndSet(sizeof(RouteMapIntf));
        int numParam = 0;
        RtInterfaceType intfType = INVALID_TYPE;

        numParam = sscanf(*criteriaString,"%s %s %s", token1, token2,
            token3);

        // check for the first time for existing tokens
        if (numParam != 2)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nWrong syntax for interface "
                "params.\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        // get the interface type
        intfType = RtParseFindInterfaceType(token1);

        if (intfType == INVALID_TYPE)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nInvalid 'interface type'."
                "\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        param->type = intfType;
        // Developer comments: No specific criteria is found to check the
        //  validity of interface number, so a string compare is done
        param->intfNumber = RtParseStringAllocAndCopy(token2);
        param->intf= -1;

        position = RtParseGetPositionInString(*criteriaString, token2);
        *criteriaString = *criteriaString + position;

        // make the list of params if any
        if (paramHead == NULL)
        {
            paramHead = param;
            paramTail = param;
        }
        else
        {
            paramTail->next = param;
            paramTail = param;
        }

    } while (**criteriaString != '\0');

    return paramHead;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapTestSetMetricParams
// PURPOSE   Verifying the set metric params
// ARGUMENTS token,the token holding the value
//           val, the value to be returned if verified
// RETURN    TRUE if verified else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTestSetMetricParams(char* token, int* val)
{
    // -294967295 to 294967295
    char* temp = token;

    if (*temp == '-')
    {
        temp = temp + 1;
    }

    while (*temp != '\0')
    {
        if (!isdigit((int)*temp))
        {
            return FALSE;
        }
        temp++;
    }

    *val = atoi(token);

    if ((*val < ROUTE_MAP_SET_MIN_METRIC) ||
        (*val > ROUTE_MAP_SET_MAX_METRIC))
    {
        return FALSE;
    }

    return TRUE;
}


//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseMetricParams
// PURPOSE   Parsing and initializing metric
// ARGUMENTS node, node concerned
//           criteriaString, the string portion which remains unparsed
//           originalString, the original string as in the config file
// RETURN    Return the parameter list
// COMMENTS  Match and cost are assumed to be same in this implementation.
//--------------------------------------------------------------------------
static
RouteMapMetric* RouteMapParseMetricParams(Node* node,
                                          BOOL isMatch,
                                          char** criteriaString,
                                          const char* originalString)
{
    // remaining tokens...minimum-length maximum-length
    int numParam = 0;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    RouteMapMetric* param
        = (RouteMapMetric*) RtParseMallocAndSet(sizeof(RouteMapMetric));

    numParam = sscanf(*criteriaString,"%s %s", token1, token2);

    if ((numParam == EOF) || (numParam > 1))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nWrong syntax for 'metric'"
            " command.\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // different metric limits for match and set
    if (isMatch)
    {
        // 0 to 4294967295
        unsigned int metric = 0;
        if (RouteMapCheckForMaxValue(token1, &metric))
        {
            param->metric = metric;
        }
        else
        {
            // impermissible value
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nImpermissible value for "
                "'match metric'.\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        int metric = 0;
        if (RouteMapTestSetMetricParams(token1, &metric))
        {
            param->metric = (unsigned int) metric;
        }
        else
        {
            // impermissible value
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nImpermissible value for "
                "'set metric'.\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
    }

    return param;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseSetStmt
// PURPOSE   Parsing and initializing the route map set statement
// ARGUMENTS node, node concerned
//           hasDefault, whether the set contains default
//           originalString, the original string as in the config file
//           ip, pointer to network data
//           refCriteriaString, the string portion which remains unparsed
// RETURN    The newly created set command structure
//--------------------------------------------------------------------------
static
RouteMapSetCmd* RouteMapParseSetStmt(Node* node,
                                     BOOL* hasDefault,
                                     const char* originalString,
                                     char** criteriaString)
{
    // classifying all the set statements
    //  set syntax:-
    //  SET ... <will depend on the corresponding set statement>
    RouteMapSetCmd* newSet
        = (RouteMapSetCmd*) RtParseMallocAndSet(sizeof(RouteMapSetCmd));
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    char token3[MAX_STRING_LENGTH] = {0};
    char token4[MAX_STRING_LENGTH] = {0};
    char token5[MAX_STRING_LENGTH] = {0};
    int position = 0;

    sscanf(*criteriaString,"%s %s %s %s %s", token1, token2,
        token3, token4, token5);

    // match the token for the type of set command
    if (!RtParseStringNoCaseCompare(token1, "automatic-tag"))
    {
        newSet->setType = AUTO_TAG;
        newSet->setFunc = RouteMapSetFuncAutoTag;
    }
    else if ((!RtParseStringNoCaseCompare(token1, "default")) &&
        (!RtParseStringNoCaseCompare(token2, "interface")))
    {
        RouteMapIntf* param = NULL;

        newSet->setType = DEF_INTF;
        newSet->setFunc = RouteMapSetFuncDefIntf;
        *hasDefault = TRUE;

        // get the remaining string
        position = RtParseGetPositionInString(*criteriaString, token2);
        *criteriaString += position;
        param = RouteMapParseInterfaceParams(node, criteriaString,
            originalString);

        // for 'set default interface'
        param->defaultSetting = TRUE;
        newSet->params = param;
    }
    else if (!RtParseStringNoCaseCompare(token1, "interface"))
    {
        RouteMapIntf* param = NULL;

        newSet->setType = SET_INTERFACE;
        newSet->setFunc = RouteMapSetFuncIntf;

        // get the remaining string
        position = RtParseGetPositionInString(*criteriaString, token1);
        *criteriaString += position;
        param = RouteMapParseInterfaceParams(node, criteriaString,
            originalString);

        // for 'set default interface'
        param->defaultSetting = FALSE;
        newSet->params = param;
    }
    else if (!RtParseStringNoCaseCompare(token1, "ip"))
    {
        if ((!RtParseStringNoCaseCompare(token2, "default")) &&
            (!RtParseStringNoCaseCompare(token3, "next-hop")))
        {
            if (!RtParseStringNoCaseCompare(token4, "verify-availability"))
            {
                // type of set command
                newSet->setType = IP_DEF_NEXT_HOP_VERFY_AVAIL;
                newSet->setFunc = RouteMapSetFuncDefIPNextHopVerfAvail;
            }
            else
            {
                RouteMapSetIPNextHop* param = NULL;

                // type of set command
                newSet->setType = IP_DEF_NEXT_HOP;
                newSet->setFunc = RouteMapSetFuncDefIPNextHop;

                // get the remaining string
                position =
                    RtParseGetPositionInString(*criteriaString, token3);

                // parse the remaining tokens...address
                *criteriaString += position;
                param = RouteMapParseAddress(node, criteriaString,
                    originalString);
                param->defaultSetting = TRUE;
                newSet->params = param;
            }

            *hasDefault = TRUE;
        }
        else if (!RtParseStringNoCaseCompare(token2, "next-hop"))
        {
            if (!RtParseStringNoCaseCompare(token3, "verify-availability"))
            {
                // type of set command
                newSet->setType = IP_NEXT_HOP_VERFY_AVAIL;
                newSet->setFunc = RouteMapSetFuncIPNextHopVerfAvail;
            }
            else
            {
                RouteMapSetIPNextHop* param = NULL;

                // type of set command
                newSet->setType = SET_IP_NEXT_HOP;
                newSet->setFunc = RouteMapSetFuncIPNextHop;

                // get the remaining string
                position =
                    RtParseGetPositionInString(*criteriaString, token2);

                // parse the remaining tokens...address...
                *criteriaString += position;
                param = RouteMapParseAddress(node, criteriaString,
                    originalString);
                param->defaultSetting = FALSE;
                newSet->params = param;
            }
        }
        else if (!RtParseStringNoCaseCompare(token2, "precedence"))
        {
            RouteMapPrecedence param = (RouteMapPrecedence) -1;

            // type of set command
            newSet->setType = IP_PRECEDENCE;
            newSet->setFunc = RouteMapSetFuncIPPrecedence;

            // get the remaining string
            position = RtParseGetPositionInString(*criteriaString, token2);

            // parse the remaining tokens...
            *criteriaString += position;
            param = RouteMapParseSetPrecedence(node, criteriaString,
                originalString);
            newSet->params =
                RtParseMallocAndSet(sizeof(RouteMapPrecedence));
            *((RouteMapPrecedence*) newSet->params) = param;
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nWrong syntax for 'set ip...' "
                "command in Route map.\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
    }
    else if (!RtParseStringNoCaseCompare(token1, "level"))
    {
        RouteMapLevel param = (RouteMapLevel) -1;

        // type of set command
        newSet->setType = LEVEL;
        newSet->setFunc = RouteMapSetFuncLevel;

        // get the remaining string
        position = RtParseGetPositionInString(*criteriaString, token1);

        // parse the remaining tokens
        *criteriaString += position;
        param = RouteMapParseSetLevel(node, criteriaString, originalString);
        newSet->params = RtParseMallocAndSet(sizeof(RouteMapLevel));
        *((RouteMapLevel*) newSet->params) = param;
    }
    else if (!RtParseStringNoCaseCompare(token1, "local-preference"))
    {
        RouteMapSetLocalPref* param = NULL;

        // type of set command
        newSet->setType = LOCAL_PREF;
        newSet->setFunc = RouteMapSetFuncLocalPref;

        // get the remaining string
        position = RtParseGetPositionInString(*criteriaString, token1);

        // parse the remaining tokens...
        *criteriaString += position;
        param = RouteMapParseSetLocalPref(node, criteriaString,
            originalString);
        newSet->params = param;
    }
    else if (!RtParseStringNoCaseCompare(token1, "metric"))
    {
        RouteMapMetric* param = NULL;

        // type of set command
        newSet->setType = SET_METRIC;
        newSet->setFunc = RouteMapSetFuncMetric;

        // get the remaining string
        position = RtParseGetPositionInString(*criteriaString, token1);

        // parse the remaining tokens...
        *criteriaString += position;
        param = RouteMapParseMetricParams(node, FALSE, criteriaString,
            originalString);
        newSet->params = param;
    }
    else if (!RtParseStringNoCaseCompare(token1, "metric-type"))
    {
        RouteMapMetricType param = (RouteMapMetricType) -1;

        // type of set command
        newSet->setType = METRIC_TYPE;
        newSet->setFunc = RouteMapSetFuncMetricType;

        // get the remaining string
        position = RtParseGetPositionInString(*criteriaString, token1);

        // parse the remaining tokens...
        *criteriaString += position;
        param = RouteMapParseSetMetricType(node, criteriaString,
            originalString);
        newSet->params = RtParseMallocAndSet(sizeof(RouteMapMetricType));
        *((RouteMapMetricType*) newSet->params) = param;
    }
    else if (!RtParseStringNoCaseCompare(token1, "next-hop"))
    {
        NodeAddress param = 0;

        // type of set command
        newSet->setType = NEXT_HOP;
        newSet->setFunc = RouteMapSetFuncNextHop;

        // get the remaining string
        position = RtParseGetPositionInString(*criteriaString, token1);

        // parse the remaining tokens...
        *criteriaString += position;
        param = RouteMapParseSetNextHop(node, criteriaString,
            originalString);
        newSet->params = RtParseMallocAndSet(sizeof(NodeAddress));
        *((NodeAddress*) newSet->params) = param;
    }
    else if (!RtParseStringNoCaseCompare(token1, "tag"))
    {
        RouteMapSetTag* param = NULL;

        // type of set command
        newSet->setType = SET_TAG;
        newSet->setFunc = RouteMapSetFuncTag;

        // get the remaining string
        position = RtParseGetPositionInString(*criteriaString, token1);

        // parse the remaining tokens...
        *criteriaString += position;
        param = RouteMapParseSetTag(node, criteriaString, originalString);
        newSet->params = param;
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nWrong syntax for 'set' command "
            "in Route map.\n'%s'\n", node->nodeId,
            originalString);
        ERROR_ReportError(errString);
    }

    return newSet;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseMatchTagParams
// PURPOSE   Parsing and initializing tag values
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           refCriteriaString, the string portion which remains unparsed
// RETURN    Return the parameter list
//--------------------------------------------------------------------------
static
RouteMapMatchTag* RouteMapParseMatchTagParams(Node* node,
                                              char** criteriaString,
                                              const char* originalString)
{
    // syntax:-
    //  tag-value [...tag-value]
    char token1[MAX_STRING_LENGTH] = {0};
    RouteMapMatchTag* tagList = NULL;
    RouteMapMatchTag* prevTag = NULL;

    if (**criteriaString == '\0')
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nSyntax error for 'match tag'"
            " in router config file. Must have atleast one tag value"
            "\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    do
    {
        int position = 0;
        unsigned int tag = 0;
        RouteMapMatchTag* param
            = (RouteMapMatchTag*)
              RtParseMallocAndSet(sizeof(RouteMapMatchTag));

        sscanf(*criteriaString,"%s", token1);

        if (RouteMapCheckForMaxValue(token1, &tag))
        {
            param->tag = tag;
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nImpermissible value for 'set "
                "tag' in router config file\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        if (tagList == NULL)
        {
            tagList = param;
        }
        else
        {
            prevTag->next = param;
        }

        prevTag = param;

        position = RtParseGetPositionInString(*criteriaString, token1);
        *criteriaString = *criteriaString + position;
    } while (**criteriaString != '\0');

    return tagList;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseRouteTypeParams
// PURPOSE   Parsing and initializing route types
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           refCriteriaString, the string portion which remains unparsed
//           param, the enumeration pointer
// RETURN    Nothing
//--------------------------------------------------------------------------
static
RouteMapMatchRouteType* RouteMapParseRouteTypeParams(
    Node* node,
    char** criteriaString,
    const char* originalString)
{
    // remaining tokens...{local | internal | external [type-1 | type-2]
    //      | level-1 | level-2}
    int numParam = 0;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    char token3[MAX_STRING_LENGTH] = {0};

    RouteMapMatchRouteType* param
        = (RouteMapMatchRouteType*)
        RtParseMallocAndSet(sizeof(RouteMapMatchRouteType));

    numParam = sscanf(*criteriaString,"%s %s %s", token1, token2, token3);

    if (numParam == EOF)
    {
        // wrong syntax
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nWrong syntax for "
            "'match route-type'.\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // match the tokens
    if (!RtParseStringNoCaseCompare(token1, "local"))
    {
        param->rtType = LOCAL;

        if (numParam > 1)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nlocal,Wrong syntax for "
                "'match route-type'.\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
    }
    else if (!RtParseStringNoCaseCompare(token1, "internal"))
    {
        param->rtType = RT_TYPE_INTERNAL;

        if (numParam > 1)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\ninternal, Wrong syntax for "
                "'match route-type'.\n'%s'n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
    }
    else if (!RtParseStringNoCaseCompare(token1, "external"))
    {
        if (!RtParseStringNoCaseCompare(token1, "type-1"))
        {
            param->rtType = EXTERNAL_TYPE_1;
        }
        else if (!RtParseStringNoCaseCompare(token1, "type-2"))
        {
            param->rtType = EXTERNAL_TYPE_2;
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nexternal, Wrong syntax for "
                "'match route-type'.\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        if (numParam > 2)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nexternal excess params, Wrong"
                " syntax for 'match route-type'.\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
    }
    else if (!RtParseStringNoCaseCompare(token1, "level-1"))
    {
        if (numParam > 1)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nlevel-1, Wrong syntax for "
                "'match route-type'.\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        param->rtType = RT_TYPE_LEVEL_1;
    }
    else if (!RtParseStringNoCaseCompare(token1, "level-2"))
    {
        param->rtType = RT_TYPE_LEVEL_2;
        if (numParam > 1)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nlevel-2, Wrong syntax for "
                "'match route-type'.\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nWrong syntax for "
            "'match route-type'.\n'%s'\n", node->nodeId,
            originalString);
        ERROR_ReportError(errString);
    }

    return param;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseMatchLengthParams
// PURPOSE   Parsing and initializing ip address parameters
// ARGUMENTS node, node concerned
//           criteriaString, the string portion which remains unparsed
//           originalString, the original string as in the config file
// RETURN    Return the parameter list
//--------------------------------------------------------------------------
static
RouteMapMatchLength* RouteMapParseMatchLengthParams(
    Node* node,
    char** criteriaString,
    const char* originalString)
{
    // remaining tokens...minimum-length maximum-length
    int numParam = 0;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    char token3[MAX_STRING_LENGTH] = {0};
    int min = 0;
    int max = 0;
    RouteMapMatchLength* param
        = (RouteMapMatchLength*)
        RtParseMallocAndSet(sizeof(RouteMapMatchLength));

    numParam = sscanf(*criteriaString,"%s %s %s", token1, token2, token3);

    if (numParam == EOF || numParam != 2)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nWrong syntax for 'match length'"
            ".\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    min = RtParseValidateAndConvertToInt(token1);
    max = RtParseValidateAndConvertToInt(token2);

    if ((min < 0) || (min > ROUTE_MAP_MATCH_MAX_LENGTH))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nImpermissible min value of length in"
            " 'match length' command.\n'%s'\n", node->nodeId,
            originalString);
        ERROR_ReportError(errString);
    }

    if ((max < 0) || (max > ROUTE_MAP_MATCH_MAX_LENGTH))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nImpermissible max value of length in"
            " 'match length' command.\n'%s'\n", node->nodeId,
            originalString);
        ERROR_ReportError(errString);
    }

    param->min = min;
    param->max = max;

    return param;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseAccessListParams
// PURPOSE   Parsing and initializing ip address parameters
// ARGUMENTS node, node concerned
//           criteriaString, the string portion which remains unparsed
//           originalString, the original string as in the config file
// RETURN    Return the parameter list
//--------------------------------------------------------------------------
static
RouteMapAclList* RouteMapParseAccessListParams(
    Node* node,
    char** criteriaString,
    const char* originalString)
{
    // access list syntax:
    //  {access-list-number | access-list-name} [... access-list-number |
    //  ... access-list-name]
    // getting a new access list block
    char token1[MAX_STRING_LENGTH] = {0};
    RouteMapAclList* paramHead = NULL;
    RouteMapAclList* paramTail = NULL;

    // extracting the access list id
    do
    {
        int aclVal = 0;
        int position = 0;
        RouteMapAclList* param
            = (RouteMapAclList*)
              RtParseMallocAndSet(sizeof(RouteMapAclList));
        int numParam = 0;

        numParam = sscanf(*criteriaString,"%s", token1);
        if (paramHead == NULL)
        {
            paramHead = param;
            paramTail = param;

            if (numParam == EOF)
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "node :%u\nMissing access list ID in "
                    "'match command'.\n'%s'\n", node->nodeId,
                    originalString);
                ERROR_ReportError(errString);
            }
        }
        else
        {
            paramTail->next = param;
            paramTail = param;
        }

        // Cisco recommends a check on ACL number that shud be between
        // 1 to 199
        aclVal = RtParseValidateAndConvertToInt(token1);
        if (aclVal != -1)
        {
            if ((aclVal < ACCESS_LIST_MIN_STANDARD) &&
                (aclVal > ACCESS_LIST_MAX_EXTENDED))
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "node :%u\nAccess list number not in "
                    "range for 'match command'.\nAllowed range is from 0"
                    " to 199.\n'%s'\n", node->nodeId, originalString);
                ERROR_ReportError(errString);
            }
        }

        param->aclId = RtParseStringAllocAndCopy(token1);

        position = RtParseGetPositionInString(*criteriaString, token1);
        *criteriaString = *criteriaString + position;

    } while (**criteriaString != '\0');

    return paramHead;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseMatchStmt
// PURPOSE   Parsing and initializing the route map match statement
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           ip, pointer to network data
//           criteriaString, the string portion which remains unparsed
// RETURN    The new match command structure pointer
//--------------------------------------------------------------------------
static
RouteMapMatchCmd* RouteMapParseMatchStmt(Node* node,
                                         const char* originalString,
                                         char** criteriaString)
{
    // classifying all the match statements
    // match syntax:-
    //  MATCH ... <will depend on the corresponding match statement>
    RouteMapMatchCmd* newMatch =
        (RouteMapMatchCmd*) RtParseMallocAndSet(sizeof(RouteMapMatchCmd));
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    char token3[MAX_STRING_LENGTH] = {0};
    int position = 0;
    int numParam = 0;

    numParam = sscanf(*criteriaString,"%s %s %s", token1, token2, token3);
    if (numParam == EOF)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nWrong syntax for 'match command'"
            " in Route map.\n'%s'\n", node->nodeId,
            originalString);
        ERROR_ReportError(errString);
    }

    newMatch->matchType = RouteMapMatchTokenForMatchCommand(node,
        token1, token2);

    // parse the remaining params
    switch (newMatch->matchType)
    {
        case AS_PATH:
        {
            // Developer comments: this is not implemented
            break;
        }
        case COMMUNITY:
        {
            // Developer Comments: this is not implemented
            break;
        }
        case EXTCOMMUNITY:
        {
            // Developer Comments: this is not implemented
            break;
        }
        case INTERFACE:
        {
            RouteMapIntf* param = NULL;

            newMatch->matchFunc = RouteMapMatchFuncInterface;

            position = RtParseGetPositionInString(*criteriaString, token1);
            *criteriaString += position;
            param = RouteMapParseInterfaceParams(node, criteriaString,
                originalString);
            param->defaultSetting = FALSE;
            newMatch->params = param;
            break;
        }
        case IP_ADDRESS:
        {
            RouteMapAclList* param = NULL;

            newMatch->matchFunc = RouteMapMatchFuncIPAddress;

            // parse the remaining tokens...access list id
            position = RtParseGetPositionInString(*criteriaString, token2);
            *criteriaString += position;
            param = RouteMapParseAccessListParams(node,
                criteriaString, originalString);

            newMatch->params = param;
            break;
        }
        case IP_NEXT_HOP:
        {
            RouteMapAclList* param = NULL;

            newMatch->matchFunc = RouteMapMatchFuncIPNextHop;

            // parse the remaining tokens...access list id
            position = RtParseGetPositionInString(*criteriaString, token2);
            *criteriaString += position;
            param = RouteMapParseAccessListParams(node,
                criteriaString, originalString);

            newMatch->params = param;
            break;
        }
        case IP_ROUTE_SRC:
        {
            RouteMapAclList* param = NULL;

            newMatch->matchFunc = RouteMapMatchFuncIPRtSrc;

            // parse the remaining tokens...access list id
            position = RtParseGetPositionInString(*criteriaString, token2);
            *criteriaString += position;
            param = RouteMapParseAccessListParams(node,
                criteriaString, originalString);

            newMatch->params = param;
            break;
        }
        case LENGTH:
        {
            RouteMapMatchLength* param = NULL;

            newMatch->matchFunc = RouteMapMatchFuncLength;

            // parse the remaining tokens..'minimum-length maximum-length'
            position = RtParseGetPositionInString(*criteriaString, token1);
            *criteriaString += position;
            param = RouteMapParseMatchLengthParams(node, criteriaString,
                originalString);

            newMatch->params = param;
            break;
        }
        case METRIC:
        {
            RouteMapMetric* param = NULL;

            newMatch->matchFunc = RouteMapMatchFuncMetric;

            // parse the remaining tokens..'minimum-length maximum-length'
            position = RtParseGetPositionInString(*criteriaString, token1);
            *criteriaString += position;
            param = RouteMapParseMetricParams(node, TRUE, criteriaString,
                originalString);

            newMatch->params = param;
            break;
        }
        case ROUTE_TYPE:
        {
            RouteMapMatchRouteType* param = NULL;

            newMatch->matchFunc = RouteMapMatchFuncRtType;

            // parse the remaining tokens..'minimum-length maximum-length'
            position = RtParseGetPositionInString(*criteriaString, token1);
            *criteriaString += position;
            param = RouteMapParseRouteTypeParams(node, criteriaString,
                originalString);

            newMatch->params = param;
            break;
        }
        case TAG:
        {
            RouteMapMatchTag* param = NULL;
            newMatch->matchFunc = RouteMapMatchFuncTag;

            // parse the remaining tokens..'minimum-length maximum-length'
            position = RtParseGetPositionInString(*criteriaString, token1);
            *criteriaString += position;
            param = RouteMapParseMatchTagParams(node, criteriaString,
                originalString);

            newMatch->params = param;
            break;
        }
        default:
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nWrong syntax for 'match command'"
                " in Route map.\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
    }

    return newMatch;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapValidateAndAddEntry
// PURPOSE   Validating the route map and adding in the correct order
// ARGUMENTS node, node concerned
//           ip, pointer to network data
//           originalString, the original string
//           newEntry, the new entry which needs to be plugged
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapValidateAndAddEntry(Node* node,
                                 NetworkDataIp* ip,
                                 const char* originalString,
                                 RouteMapEntry* newEntry,
                                 BOOL hasDefault)
{
    // add the default of sequence number in new route map
    if (newEntry->seq == -1)
    {
        newEntry->seq = ROUTE_MAP_DEF_SEQ;
    }

    // plug the new route map in the list
    if (ip->routeMapList == NULL)
    {
        RouteMap* newRouteMap = RouteMapCreateTemplate(newEntry,
            newEntry->mapTag);
        RouteMapList* list
            = (RouteMapList*) RtParseMallocAndSet(sizeof(RouteMapList));
        ip->routeMapList = list;

        list->head = newRouteMap;
        list->tail = newRouteMap;
        newRouteMap->hasDefault = hasDefault;
    }
    else
    {
        RouteMap* tempRouteMap = RouteMapFindByName(node, newEntry->mapTag);

        if (tempRouteMap == NULL)
        {
            RouteMap* newRouteMap = RouteMapCreateTemplate(newEntry,
                newEntry->mapTag);

            // a new tagged route map so plug it in the end
            ip->routeMapList->tail->next = newRouteMap;
            ip->routeMapList->tail = ip->routeMapList->tail->next;
            newRouteMap->hasDefault = hasDefault;
        }
        else
        {
            // route map with the same tag already plugged in...so add
            // it as a new entity.
            RouteMapPlugEntry(node, tempRouteMap, newEntry, originalString);
            tempRouteMap->hasDefault = hasDefault;
        }
    }

    // we store the last entry in the buffer list for
    //  plugging match/set command
    ip->bufferLastEntryRMap = newEntry;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseRouteMapStmt
// PURPOSE   Parsing and initializing the route map statement
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           refCriteriaString, the string portion which remains unparsed
// RETURN    The new entry collected after parsing
//--------------------------------------------------------------------------
static
RouteMapEntry* RouteMapParseRouteMapStmt(Node* node,
                                         const char* originalString,
                                         char** criteriaString)
{
    // route map syntax:-
    //  route-map map-tag [permit | deny] [sequence-number]

    // creating a blank route map structure
    RouteMapEntry* newEntry = RouteMapNewEntry();
    char tag[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    char token3[MAX_STRING_LENGTH] = {0};
    char token4[MAX_STRING_LENGTH] = {0};
    int numArgs = 0;

    // remaining options:-
    // map-tag [permit | deny] [sequence-number]
    numArgs = sscanf(*criteriaString,"%s %s %s %s",
        tag, token2, token3, token4);

    // we will break as per the number of args
    switch (numArgs)
    {
        case 1:
        {
            // only the tag is given
            // the token has to be a tag
            newEntry->type = ROUTE_MAP_PERMIT;
            break;
        }
        case 2:
        {
            // one tag is mising
            // check whether type is given
            if (!RtParseStringNoCaseCompare("DENY", token2))
            {
                // seq is missing, use default later
                newEntry->type = ROUTE_MAP_DENY;
            }
            else if (!RtParseStringNoCaseCompare("PERMIT", token2))
            {
                // seq is missing, use default later
                newEntry->type = ROUTE_MAP_PERMIT;
            }
            else
            {
                // type is missing
                newEntry->type = ROUTE_MAP_PERMIT;
                newEntry->seq = RouteMapPlugSeq(node, token2,
                    originalString);
            }
            break;
        }
        case 3:
        {
            // all options are given
            if (!RtParseStringNoCaseCompare("DENY", token2))
            {
                // seq is missing, use default later
                newEntry->type = ROUTE_MAP_DENY;
            }
            else if (!RtParseStringNoCaseCompare("PERMIT", token2))
            {
                // seq is missing, use default later
                newEntry->type = ROUTE_MAP_PERMIT;
            }

            newEntry->seq = RouteMapPlugSeq(node, token3, originalString);
            break;
        }
        case EOF: // fall through for default
        default:
        {
            // wrong syntax
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nProper 'Route map' syntax not "
                "followed.\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
            break;
        }
    }

    newEntry->mapTag = RtParseStringAllocAndCopy(tag);

    return newEntry;
}




///////// No commands for route map and freeing functions //////////////////



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeIntf
// PURPOSE   Remove this given match command
// ARGUMENTS node, current node
//           param, the match parameter which needs to be removed
// RETURN    void
//--------------------------------------------------------------------------
static
void RouteMapFreeIntf(void* param)
{
    RouteMapIntf* intf = (RouteMapIntf*) param;
    RouteMapIntf* prev = NULL;

    while (intf)
    {
        prev = intf;
        intf = intf->next;
        MEM_free(prev->intfNumber);
        MEM_free(prev);
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeSpecificSet
// PURPOSE   Remove this given set command
// ARGUMENTS match, the match which needs to be removed
// RETURN    void
//--------------------------------------------------------------------------
static
void RouteMapFreeSpecificSet(RouteMapSetCmd* set)
{
    // the only struct's in set are these, so just check these ones
    if ((set->setType == SET_IP_NEXT_HOP) ||
        (set->setType == IP_DEF_NEXT_HOP))
    {
        RouteMapSetIPNextHop* nextHop = (RouteMapSetIPNextHop*) set->params;
        RouteMapAddressList* add = nextHop->addressHead;
        RouteMapAddressList* prev = NULL;

        while (add)
        {
            prev = add;
            add = add->next;
            MEM_free(prev);
        }

        MEM_free(set->params);
    }
    else if ((set->setType == DEF_INTF) ||
        (set->setType == SET_INTERFACE))
    {
        RouteMapFreeIntf((RouteMapIntf*) set->params);
    }
    else if ((set->setType == LOCAL_PREF) || (set->setType == SET_TAG)
        || (set->setType == IP_PRECEDENCE) || (set->setType == METRIC_TYPE)
        || (set->setType == SET_METRIC) || (set->setType == LEVEL)
        || (set->setType == NEXT_HOP))
    {
        MEM_free(set->params);
    }

    MEM_free(set);
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeSet
// PURPOSE   Remove the set commands for the particular entry
// ARGUMENTS entry, the entry whose match needs to be removed
// RETURN    void
//--------------------------------------------------------------------------
static
void RouteMapFreeSet(RouteMapEntry* entry)
{
    RouteMapSetCmd* current = entry->setHead;
    RouteMapSetCmd* prev = NULL;

    while (current)
    {
        prev = current;
        current = current->next;
        RouteMapFreeSpecificSet(prev);
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeAclList
// PURPOSE   For freeing match ip address, match ip next hop, match
//              ip route source
// ARGUMENTS param, the match param which needs to be removed
// RETURN    void
//--------------------------------------------------------------------------
static
void RouteMapFreeAclList(void* param)
{
    RouteMapAclList* acl = (RouteMapAclList*) param;
    RouteMapAclList* prev = NULL;

    while (acl)
    {
        prev = acl;
        acl = acl->next;
        MEM_free(prev->aclId);
        MEM_free(prev);
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeMatchTag
// PURPOSE   Remove the match tag command
// ARGUMENTS param, the match param which needs to be removed
// RETURN    void
//--------------------------------------------------------------------------
static
void RouteMapFreeMatchTag(void* param)
{
    RouteMapMatchTag* tag = (RouteMapMatchTag*) param;
    RouteMapMatchTag* prev = NULL;

    while (tag)
    {
        prev = tag;
        tag = tag->next;
        MEM_free(prev);
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeSpecificMatch
// PURPOSE   Remove the given match command
// ARGUMENTS match, the match which needs to be removed
// RETURN    void
//--------------------------------------------------------------------------
static
void RouteMapFreeSpecificMatch(Node* node, RouteMapMatchCmd* match)
{
    switch (match->matchType)
    {
        case AS_PATH:
        {
            break;
        }
        case COMMUNITY:
        {
            break;
        }
        case EXTCOMMUNITY:
        {
            break;
        }
        case INTERFACE:
        {
            RouteMapFreeIntf(match->params);
            break;
        }
        case IP_ADDRESS:
        {
            RouteMapFreeAclList(match->params);
            break;
        }
        case IP_NEXT_HOP:
        {
            RouteMapFreeAclList(match->params);
            break;
        }
        case IP_ROUTE_SRC:
        {
            RouteMapFreeAclList(match->params);
            break;
        }
        case LENGTH:
        {
            MEM_free(match->params);
            break;
        }
        case METRIC:
        {
            MEM_free(match->params);
            break;
        }
        case ROUTE_TYPE:
        {
            MEM_free(match->params);
            break;
        }
        case TAG:
        {
            RouteMapFreeMatchTag(match->params);
            break;
        }
        default:
        {
            // invalid match type
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nInvalid match type encountered"
                " while removing match commands.\n", node->nodeId);
            ERROR_ReportError(errString);
            break;
        }
    }

    MEM_free(match);
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeMatch
// PURPOSE   Remove the match commands for the particular entry
// ARGUMENTS entry, the entry whose match needs to be removed
// RETURN    void
//--------------------------------------------------------------------------
static
void RouteMapFreeMatch(Node* node, RouteMapEntry* entry)
{
    RouteMapMatchCmd* current = entry->matchHead;
    RouteMapMatchCmd* prev = NULL;

    while (current)
    {
        prev = current;
        current = current->next;
        RouteMapFreeSpecificMatch(node, prev);
    }
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeRMapEntry
// PURPOSE   Remove the particular entry. The function assumes the user has
//              made sure that the entry exists  in the list
// ARGUMENTS entry, the entry to be removed
// RETURN    void
//--------------------------------------------------------------------------
static
void RouteMapFreeRMapEntry(Node* node, RouteMapEntry* entry)
{
    RouteMapFreeMatch(node, entry);
    RouteMapFreeSet(entry);
    MEM_free(entry->mapTag);
    MEM_free(entry);
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapNoCmd
// PURPOSE   Simulating the 'no route map' command. Both parsing and match
//              of route map is done here
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           ip, pointer to network data
//           criteriaString, the string portion which remains unparsed
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapNoCmd(Node* node,
                   const char* originalString,
                   char** criteriaString)
{
    RouteMapEntry* noEntry = RouteMapParseRouteMapStmt(
        node,
        originalString,
        criteriaString);

    RouteMap* rMap = RouteMapFindByName(node, noEntry->mapTag);

    if (!rMap)
    {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nNo route map of same tag "
                " found. Route map deletion aborted...\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);

            MEM_free(noEntry->mapTag);
            MEM_free(noEntry);
            return;
    }

    if (noEntry->seq != -1)
    {
        // remove the specific entry
        BOOL flag = FALSE;
        RouteMapEntry* temp = rMap->entryHead;
        RouteMapEntry* prev = NULL;

        while (temp)
        {
            if (temp->seq == noEntry->seq)
            {
                flag = TRUE;

                if (!prev)
                {
                    rMap->entryHead = temp->next;
                }
                else
                {
                    prev->next = temp->next;
                }

                RouteMapFreeRMapEntry(node, temp);

                if (ROUTE_MAP_DEBUG)
                {
                    printf("Route map '%s', seq = %d removed successfully"
                        "...\n", rMap->mapTag, noEntry->seq);
                }

                break;
            }
            prev = temp;
            temp = temp->next;
        }

        if (!flag)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nNo route map entry"
                " with corresponding sequence number found. "
                "\n", node->nodeId);
            ERROR_ReportWarning(errString);

            MEM_free(noEntry->mapTag);
            MEM_free(noEntry);
            return;
        }
    }
    else
    {
        // remove all entries

        // adjust the link of route maps
        NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
        RouteMapEntry* prev = NULL;
        RouteMapEntry* entry = NULL;
        RouteMap* routeMap = ip->routeMapList->head;
        RouteMap* prevRMap = NULL;

        while (routeMap != rMap)
        {
            prevRMap = routeMap;
            routeMap = routeMap->next;
        }

        if (!prevRMap)
        {
            ip->routeMapList->head = routeMap->next;
        }
        else
        {
            prevRMap->next = routeMap->next;
        }

        // get the entry
        entry = routeMap->entryHead;
        while (entry)
        {
            if (prev)
            {
                if (ROUTE_MAP_DEBUG)
                {
                    printf("Removing route map entry %s, seq: %d...\n",
                        prev->mapTag, prev->seq);
                }

                RouteMapFreeRMapEntry(node, prev);
                if (ROUTE_MAP_DEBUG)
                {
                    printf("Route map removed successfully...\n");
                }
            }

            prev = entry;
            entry = entry->next;
        }

        if (prev)
        {
            if (ROUTE_MAP_DEBUG)
            {
                printf("Removing route map entry %s, seq: %d...\n",
                    prev->mapTag, prev->seq);
            }

            RouteMapFreeRMapEntry(node, prev);
            if (ROUTE_MAP_DEBUG)
            {
                printf("Route map removed successfully...\n");
            }
        }

        MEM_free(rMap->mapTag);
        MEM_free(rMap);
    }

    // free the no command
    MEM_free(noEntry->mapTag);
    MEM_free(noEntry);
}




///////////////////// No command for match commands ////////////////////////

//  Tallying match commands for 'no' commands

//--------------------------------------------------------------------------
// FUNCTION  RouteMapTallyMatchInterface
// PURPOSE   Tally the Interface params of two match commands
// ARGUMENTS node, node concerned
//           intfParam, the match command params
//           noIntfParam, the values as in the no command
// RETURN    TRUE if it matches or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTallyMatchInterface(Node* node,
                                 RouteMapIntf* intfParam,
                                 RouteMapIntf* noIntfParam)
{
    BOOL result = FALSE;
    // check for all the interface param set
    while (intfParam || noIntfParam)
    {
        if ((intfParam->type) && (noIntfParam->type) &&
            (intfParam->intfNumber) && (noIntfParam->intfNumber) &&
            (intfParam->type == noIntfParam->type) &&
            (!RtParseStringNoCaseCompare(intfParam->intfNumber,
                intfParam->intfNumber)))
        {
            result = TRUE;
            intfParam = intfParam->next;
            noIntfParam = noIntfParam->next;
        }
        else
        {
            return FALSE;
        }
    }

    return result;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapTallyMatchACL
// PURPOSE   Match the IP address with the specified access lists
// ARGUMENTS node, node concerned
//           param, the match command params
//           noParam, the values as in the no command
// RETURN    TRUE if it matches or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTallyMatchACL(Node* node,
                           RouteMapAclList* param,
                           RouteMapAclList* noParam)
{
    RouteMapAclList* acl = param;
    RouteMapAclList*noAcl = noParam;
    BOOL flag = FALSE;

    while (acl || noAcl)
    {
        if ((acl->aclId) && (noAcl->aclId) &&
            (!RtParseStringNoCaseCompare(acl->aclId, noAcl->aclId)))
        {
            acl = acl->next;
            noAcl = noAcl->next;
            flag = TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    return flag;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapTallyMatchLength
// PURPOSE   Tally the contents of the no command
// ARGUMENTS node, node concerned
//           param, the match command params
//           noParam, the values as in the no command
// RETURN    TRUE if it matches or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTallyMatchLength(Node* node,
                              RouteMapMatchLength* param,
                              RouteMapMatchLength* noParam)
{
    if ((param->min == noParam->min) && (param->max == noParam->max))
    {
        return TRUE;
    }

    // its not a  default case, its for having a proper return
    return FALSE;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapTallyMatchMetric
// PURPOSE   Match with the metric specified
// ARGUMENTS node, node concerned
//           param, the match command params
//           noParam, the values as in the no command
// RETURN    TRUE if it matches or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTallyMatchMetric(Node* node,
                              RouteMapMetric* param,
                              RouteMapMetric* noParam)
{
    BOOL retVal = FALSE;

    if (param && noParam && (param->metric == noParam->metric))
    {
        retVal = TRUE;
    }

    return retVal;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapTallyMatchRtType
// PURPOSE   Match with the route type
// ARGUMENTS node, node concerned
//           param, the match command params
//           noParam, the values as in the no command
// RETURN    TRUE if it matches or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTallyMatchRtType(Node* node,
                              RouteMapMatchRouteType* param,
                              RouteMapMatchRouteType* noParam)
{
    if (param->rtType == noParam->rtType)
    {
        return TRUE;
    }

    return FALSE;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapTallyMatchTag
// PURPOSE   Match with the specified tag
// ARGUMENTS node, node concerned
//           param, the match command params
//           noParam, the values as in the no command
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTallyMatchTag(Node* node,
                           RouteMapMatchTag* tagVal,
                           RouteMapMatchTag* noTagVal)
{
    BOOL flag = FALSE;

    while (tagVal || noTagVal)
    {
        if ((tagVal->tag) && (noTagVal->tag) &&
            (tagVal->tag == noTagVal->tag))
        {
            flag = TRUE;
            tagVal = tagVal->next;
            noTagVal = noTagVal->next;
        }
        else
        {
            return FALSE;
        }
    }

    return flag;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapTallyAndFreeSpecificMatch
// PURPOSE   Test whether the no command matches and if it does then free
//              the corresponding command
// ARGUMENTS node, node concerned
//           param, the match command params
//           noParam, the values as in the no command
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTallyAndFreeSpecificMatch(Node* node,
                                       RouteMapMatchCmd* match,
                                       RouteMapMatchCmd* noMatch)
{
    BOOL flag = FALSE;
    switch (match->matchType)
    {
        case AS_PATH:
        {
            break;
        }
        case COMMUNITY:
        {
            break;
        }
        case EXTCOMMUNITY:
        {
            break;
        }
        case INTERFACE:
        {
            if (RouteMapTallyMatchInterface(node,
                (RouteMapIntf*) match->params,
                (RouteMapIntf*) noMatch->params))
            {
                RouteMapFreeIntf(match->params);
                flag = TRUE;
            }
            break;
        }
        case IP_ADDRESS: // fall down IP_ROUTE_SRC
        case IP_NEXT_HOP: // fall down IP_ROUTE_SRC
        case IP_ROUTE_SRC:
        {
            if (RouteMapTallyMatchACL(node,
                (RouteMapAclList*) match->params,
                (RouteMapAclList*) noMatch->params))
            {
                RouteMapFreeAclList(match->params);
                flag = TRUE;
            }
            break;
        }
        case LENGTH:
        {
            if (RouteMapTallyMatchLength(node,
                (RouteMapMatchLength*) match->params,
                (RouteMapMatchLength*) noMatch->params))
            {
                MEM_free(match->params);
                flag = TRUE;
            }
            break;
        }
        case METRIC:
        {
            if (RouteMapTallyMatchMetric(node,
                (RouteMapMetric*) match->params,
                (RouteMapMetric*) noMatch->params))
            {
                MEM_free(match->params);
                flag = TRUE;
            }
            break;
        }
        case ROUTE_TYPE:
        {
            if (RouteMapTallyMatchRtType(node,
                (RouteMapMatchRouteType*) match->params,
                (RouteMapMatchRouteType*) noMatch->params))
            {
                MEM_free(match->params);
                flag = TRUE;
            }
            break;
        }
        case TAG:
        {
            if (RouteMapTallyMatchTag(node,
                (RouteMapMatchTag*) match->params,
                (RouteMapMatchTag*) noMatch->params))
            {
                RouteMapFreeMatchTag(match->params);
                flag = TRUE;
            }
            break;
        }
        default:
        {
            // invalid match type
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nInvalid match type encountered"
                " while removing match commands.\n", node->nodeId);
            ERROR_ReportError(errString);
            break;
        }
    }

    if (flag)
    {
        MEM_free(match);
    }

    return flag;
}


//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseAndFreeMatchCmd
// PURPOSE   Remove the match commands.
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           ip, pointer to network data
//           criteriaString, the string portion which remains unparsed
// RETURN    None
// COMMENTS  The no command will remove commands of same route map entry.
//              If by any chance two similar match commands are encountered
//              in the same entry, then the first command will be removed.
//--------------------------------------------------------------------------
static
void RouteMapParseAndFreeMatchCmd(Node* node,
                                  const char* originalString,
                                  NetworkDataIp* ip,
                                  char** criteriaString)
{
    // get the match command
    RouteMapEntry* entry = ip->bufferLastEntryRMap;
    RouteMapMatchCmd* match = entry->matchHead;
    RouteMapMatchCmd* noMatch = NULL;
    RouteMapMatchCmd* prevMatch = NULL;
    RouteMapMatchCmd* nextMatch = NULL;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    char token3[MAX_STRING_LENGTH] = {0};
    char errString[MAX_STRING_LENGTH];
    int numParam = 0;

    numParam = sscanf(*criteriaString,"%s %s %s", token1, token2, token3);
    if (numParam == EOF)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nWrong syntax for 'no match command'"
            " in Route map.\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // parse the no match command
    noMatch = RouteMapParseMatchStmt(node,
        originalString,
        criteriaString);

    // traverse till we get the correct match
    while (match)
    {
        if (match->matchType == noMatch->matchType)
        {
            nextMatch = match->next;
            // check whether the entered match params match with
            //  the no command set
            if (RouteMapTallyAndFreeSpecificMatch(node, match, noMatch))
            {
                // removing the link for that match command
                if (!prevMatch)
                {
                    entry->matchHead = nextMatch;
                }
                else
                {
                    prevMatch->next = nextMatch;
                }

                // free the no command
                RouteMapFreeSpecificMatch(node, noMatch);
                return;
            }
        }

        prevMatch = match;
        match = match->next;
    }

    sprintf(errString, "Node: %u\n"
        "No corresponding match command defined. Match command deletion"
        " aborted...\n%s\n", node->nodeId, originalString);
    ERROR_ReportWarning(errString);
}



//////////////////////// No command for set commands /////////////////////

// Tally for SET commands follow

//--------------------------------------------------------------------------
// FUNCTION  RouteMapTallySetIPNextHop
// PURPOSE   Tally the next hop params
// ARGUMENTS node, node concerned
//           param, the set command params
//           valueSet, structure for setting the values
// RETURN    TRUE, if it matches, else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTallySetIPNextHop(Node* node,
                               RouteMapSetIPNextHop* param,
                               RouteMapSetIPNextHop* noParam)
{
    RouteMapAddressList* addParam = param->addressHead;
    RouteMapAddressList* addNoParam = noParam->addressHead;
    BOOL flag = FALSE;

    while (addParam || addNoParam)
    {
        if ((addParam->address) && (addNoParam->address) &&
            (addParam->address == addNoParam->address))
        {
            flag = TRUE;
            addParam = addParam->next;
            addNoParam = addNoParam->next;
        }
        else
        {
            return FALSE;
        }
    }

    return flag;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreePrecedence
// PURPOSE   Remove the precedence params
// ARGUMENTS node, node concerned
//           entry, the route map entry
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapFreePrecedence(Node* node, RouteMapEntry* entry)
{
    RouteMapSetCmd* set = entry->setHead;
    RouteMapSetCmd* prev = NULL;
    RouteMapSetCmd* next = NULL;

    // go on removing all the precedence commands
    while (set)
    {
        next = set->next;

        if (set->setType == IP_PRECEDENCE)
        {
            if (!prev)
            {
                entry->setHead = next;
            }
            else
            {
                prev->next = next;
            }

            MEM_free(set->params);
            MEM_free(set);
        }
        else
        {
            prev = set;
        }

        set = next;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeNextHop
// PURPOSE   Free the next hop address
// ARGUMENTS node, node concerned
//           nextHop, the next hop params
//           noParam, the values as in the no command
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
void RouteMapFreeNextHop(Node* node,
                         RouteMapSetIPNextHop* nextHop)
{
    RouteMapAddressList* add = nextHop->addressHead;
    RouteMapAddressList* prev = NULL;

    while (add)
    {
        prev = add;
        add = add->next;
        MEM_free(prev);
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapTallyAndFreeSpecificSet
// PURPOSE   Test whether the no command matches and if it does then free
//              the corresponding command
// ARGUMENTS node, node concerned
//           param, the match command params
//           noParam, the values as in the no command
// RETURN    TRUE if its a match or else FALSE
//--------------------------------------------------------------------------
static
BOOL RouteMapTallyAndFreeSpecificSet(Node* node,
                                     RouteMapSetCmd* set,
                                     RouteMapSetCmd* noSet)
{
    BOOL flag = FALSE;

    // precedence is skipped...we jus dont ned it here. Its already handled.
    switch (set->setType)
    {
        case AUTO_TAG:
        {
            // jus break
            break;
        }
        case DEF_INTF: // drop down to next
        case SET_INTERFACE:
        {
            if (RouteMapTallyMatchInterface(node,
                (RouteMapIntf*) set->params,
                (RouteMapIntf*) noSet->params))
            {
                RouteMapFreeIntf((RouteMapIntf*) set->params);
                flag = TRUE;
            }
            break;
        }
        case SET_IP_NEXT_HOP: // fall down to next
        case IP_DEF_NEXT_HOP:
        {
            if (RouteMapTallySetIPNextHop(node,
                (RouteMapSetIPNextHop*) set->params,
                (RouteMapSetIPNextHop*) noSet->params))
            {
                RouteMapFreeNextHop(node,
                    (RouteMapSetIPNextHop*) set->params);
                flag = TRUE;
            }
            break;
        }
        case IP_DEF_NEXT_HOP_VERFY_AVAIL:
        {
            // we dont have a param list, so just break
            break;
        }
        case IP_NEXT_HOP_VERFY_AVAIL:
        {
            // we dont have a param list, so just break
            break;
        }
        case LEVEL:
        {
            if ((RouteMapLevel*) set ==
                (RouteMapLevel*) noSet)
            {
                flag = TRUE;
            }
            break;
        }
        case LOCAL_PREF:
        {
            if ((RouteMapSetLocalPref*) set ==
                (RouteMapSetLocalPref*) noSet)
            {
                flag = TRUE;
            }
            break;
        }
        case SET_METRIC:
        {
            if ((RouteMapMetric*) set ==
                (RouteMapMetric*) noSet)
            {
                flag = TRUE;
            }
            break;
        }
        case METRIC_TYPE:
        {
            if (*((RouteMapMetricType*) set) ==
                *((RouteMapMetricType*) noSet))
            {
                flag = TRUE;
            }
            break;
        }
        case NEXT_HOP:
        {
            if (*((NodeAddress*) set) ==
                *((NodeAddress*) noSet))
            {
                flag = TRUE;
            }
            break;
        }
        case SET_TAG:
        {
            if (((RouteMapSetTag*) set) ==
                ((RouteMapSetTag*) noSet))
            {
                flag = TRUE;
            }
            break;
        }
        default:
        {
            // invalid match type
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nInvalid set type encountered"
                " while removing set commands.\n", node->nodeId);
            ERROR_ReportError(errString);
            break;
        }
    }

    if (flag)
    {
        MEM_free(set);
    }

    return flag;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapParseAndFreeSetCmd
// PURPOSE   Remove the set commands.
// ARGUMENTS node, node concerned
//           originalString, the original string as in the config file
//           ip, pointer to network data
//           criteriaString, the string portion which remains unparsed
// RETURN    None
// COMMENTS  The no command will remove commands of same route map entry.
//              If by any chance two similar set commands are encountered
//              in the same entry, then the first command will be removed.
//--------------------------------------------------------------------------
static
void RouteMapParseAndFreeSetCmd(Node* node,
                                const char* originalString,
                                NetworkDataIp* ip,
                                char** criteriaString)
{
    // get the set command
    RouteMapEntry* entry = ip->bufferLastEntryRMap;
    RouteMapSetCmd* set = entry->setHead;
    RouteMapSetCmd* noSet = NULL;
    RouteMapSetCmd* prevSet = NULL;
    RouteMapSetCmd* nextSet = NULL;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    char token3[MAX_STRING_LENGTH] = {0};
    char errString[MAX_STRING_LENGTH];
    int numParam = 0;

    numParam = sscanf(*criteriaString,"%s %s %s", token1, token2, token3);
    if (numParam == EOF)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nWrong syntax for 'no set command'"
            " in Route map.\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // exception for set precedence
    if ((!RtParseStringNoCaseCompare(token1, "ip") &&
        (!RtParseStringNoCaseCompare(token2, "precedence"))))
    {
        if (numParam == 3)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nWrong syntax for 'no set ip"
                " precedence' command in Route map configuratin file."
                "\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        RouteMapFreePrecedence(node, entry);
        return;
    }
    else
    {
        BOOL hasDefault = FALSE;
        // parse the no set command
        noSet = RouteMapParseSetStmt(node,
            &hasDefault,
            originalString,
            criteriaString);
    }

    // traverse till we get the correct match
    while (set)
    {
        if (set->setType == noSet->setType)
        {
            nextSet = set->next;
            // check whether the entered match params match with
            //  the no command set
            if (RouteMapTallyAndFreeSpecificSet(node, set, noSet))
            {
                // removing the link for that match command
                if (!prevSet)
                {
                    entry->setHead = nextSet;
                }
                else
                {
                    prevSet->next = nextSet;
                }

                // free the no command
                RouteMapFreeSpecificSet(noSet);
                return;
            }
        }

        prevSet = set;
        set = set->next;
    }

    sprintf(errString, "Node: %u\n"
        "No corresponding set command defined. Set command deletion"
        " aborted...\n%s\n", node->nodeId, originalString);
    ERROR_ReportWarning(errString);
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapInit
// PURPOSE   Initialize the route map
// ARGUMENTS node, node concerned
//           nodeInput, Main configuration file
// RETURN    None
// COMMENTS  Statements which are not used will be not parsed. No feedback
//              is given to the user.
//--------------------------------------------------------------------------
void RouteMapInit(Node* node, const NodeInput* nodeInput)
{
    NodeInput rtConfig;
    BOOL isFound = FALSE;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    // Read configuration for router configuration file
    IO_ReadCachedFile(node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTER-CONFIG-FILE",
        &isFound,
        &rtConfig);

    // file found
    if (isFound == TRUE)
    {
        // This node may have some entry for router configuration.
        // So we need to go through all the entries in the file
        int count = 0;
        int currentPosition = 0;
        BOOL foundRouterConfiguration = TRUE;
        BOOL gotForThisNode = FALSE;

        //Route map format followed:-
        // router <node-id>
        // route-map 5 permit 20
        // match route-type external
        // set metric 10

        for (count = 0; count < rtConfig.numLines; count++)
        {
            int numParam = 0;
            char* originalString = NULL;
            char* refCriteriaString = NULL;
            char token1[MAX_STRING_LENGTH] = {0};
            char token2[MAX_STRING_LENGTH] = {0};
            char token3[MAX_STRING_LENGTH] = {0};
            char token4[MAX_STRING_LENGTH] = {0};
            BOOL hasDefault = FALSE;

            // find the first and second token
            numParam = sscanf(rtConfig.inputStrings[count],"%s %s %s %s",
                token1, token2, token3, token4);

            // we make a copy of the original string for error reporting
            originalString = RtParseStringAllocAndCopy(
                rtConfig.inputStrings[count]);

            if (!RtParseStringNoCaseCompare("NODE-IDENTIFIER", token1))
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
                (!RtParseStringNoCaseCompare("ROUTE-MAP", token1)))
            {
                RouteMapEntry* newEntry = NULL;
                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token1);

                refCriteriaString = originalString + currentPosition;

                newEntry = RouteMapParseRouteMapStmt(
                    node,
                    originalString,
                    &refCriteriaString);

                RouteMapValidateAndAddEntry(node,
                    ip,
                    originalString,
                    newEntry,
                    hasDefault);

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (!RtParseStringNoCaseCompare("NO", token1)) &&
                (!RtParseStringNoCaseCompare("ROUTE-MAP", token2)))
            {
                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token2);

                refCriteriaString = originalString + currentPosition;

                RouteMapNoCmd(node, originalString, &refCriteriaString);

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (!RtParseStringNoCaseCompare("MATCH", token1)))
            {
                RouteMapEntry* entry = ip->bufferLastEntryRMap;
                RouteMapMatchCmd* newMatch = NULL;

                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token1);

                refCriteriaString = originalString + currentPosition;

                newMatch = RouteMapParseMatchStmt(
                    node,
                    originalString,
                    &refCriteriaString);

                RouteMapAddMatch(entry, newMatch);

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (!RtParseStringNoCaseCompare("NO", token1)) &&
                (!RtParseStringNoCaseCompare("MATCH", token2)))
            {
                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token2);

                refCriteriaString = originalString + currentPosition;

                RouteMapParseAndFreeMatchCmd(
                    node,
                    originalString,
                    ip,
                    &refCriteriaString);

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration
                && (!RtParseStringNoCaseCompare("SET", token1)))
            {
                RouteMapEntry* entry = ip->bufferLastEntryRMap;
                RouteMapSetCmd* newSet = NULL;
                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token1);

                refCriteriaString = originalString + currentPosition;

                newSet = RouteMapParseSetStmt(
                    node,
                    &hasDefault,
                    originalString,
                    &refCriteriaString);

                RouteMapAddSet(entry, newSet);

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (!RtParseStringNoCaseCompare("NO", token1)) &&
                (!RtParseStringNoCaseCompare("SET", token2)))
            {
                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token2);

                refCriteriaString = originalString + currentPosition;

                RouteMapParseAndFreeSetCmd(
                    node,
                    originalString,
                    ip,
                    &refCriteriaString);

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (!RtParseStringNoCaseCompare("SHOW", token1)) &&
                (!RtParseStringNoCaseCompare("ROUTE-MAP", token2)))
            {
                // check if name is given
                if (numParam == 3)
                {
                    // name is given
                    RouteMapShowCmd(node, token3);
                }
                else if (numParam == 2)
                {
                    // no name given
                    RouteMapShowCmd(node, NULL);
                }
                else
                {
                    // this is an error
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "node :%u\nInvalid syntax "
                        "'show' command.\n%s\n", node->nodeId,
                        originalString);
                    ERROR_ReportError(errString);
                }

                MEM_free(originalString);
                continue;
            }
            else
            {
                // got some invalid statement
                MEM_free(originalString);
            }
        }
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFreeRtMap
// PURPOSE   Free the given route map
// ARGUMENTS node, the cuirrent node
//           rMap, concerned route map
// RETURN    None
//--------------------------------------------------------------------------
static
void RouteMapFreeRtMap(Node* node, RouteMap* rMap)
{
    RouteMapEntry* entry = NULL;
    RouteMapEntry* prev = NULL;

    if (!rMap)
    {
        // nothing defined, so return
        return;
    }

    entry = rMap->entryHead;

    while (entry)
    {
        if (ROUTE_MAP_DEBUG)
        {
            printf("Removing route map entry %s...\n", entry->mapTag);
        }
        if (prev)
        {
            RouteMapFreeRMapEntry(node, prev);
            if (ROUTE_MAP_DEBUG)
            {
                printf("Route map removed successfully...\n");
            }
        }

        prev = entry;
        entry = entry->next;
    }

    RouteMapFreeRMapEntry(node, prev);
    if (ROUTE_MAP_DEBUG)
    {
        printf("Route map removed successfully...\n");
    }
    MEM_free(rMap->mapTag);
    MEM_free(rMap);
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapFinalize
// PURPOSE   Free all the route maps defined for this node
// ARGUMENTS node, concerned node
// RETURN    None
//--------------------------------------------------------------------------
void RouteMapFinalize(Node* node)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    RouteMap* rMap = NULL;
    RouteMap* prevRMap = NULL;

    if (!ip->routeMapList)
    {
        // no route map defined
        return;
    }

    rMap = ip->routeMapList->head;

    if (ROUTE_MAP_DEBUG)
    {
        printf("In finalize...\n");
        printf("\nNode: %u, Freeing all route maps...\n\n", node->nodeId);
    }

    while (rMap)
    {
        if (prevRMap)
        {
            RouteMapFreeRtMap(node, prevRMap);
        }

        prevRMap = rMap;
        rMap = rMap->next;
    }

    if (prevRMap)
    {
        RouteMapFreeRtMap(node, prevRMap);
    }

    MEM_free(ip->routeMapList);
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapAddHook
// PURPOSE   Plug the route map
// ARGUMENTS node, concerned node
//           hookList, pointer of pointer to the list of all route maps
//              required by different functionalities
//           routeMapName, name of the route map to be hooked
// RETURN    None
//--------------------------------------------------------------------------
RouteMap* RouteMapAddHook(Node* node,
                     char* routeMapName)
{
    RouteMap* currentRMap = RouteMapFindByName(node, routeMapName);

    if (!currentRMap)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node :%u\nRoute map, '%s', not declared in "
            "router configuration file.\n", node->nodeId, routeMapName);
        ERROR_ReportError(errString);
    }

    return currentRMap;
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetEntry
// PURPOSE   Perform the set action
// ARGUMENTS node, the concerned node
//           entry, the node where route map is set
//           valueSet, the set of values to match with the criteria
// RETURN    void
// COMMENTS:
// They are evaluated in the following order:
// set ip next-hop
// set interface
// set ip default next-hop
// set default interface
//  if all of them are defined. This has to be arranged in the correct order
//      by the caller functionality depending on the above available
//      commands.
//--------------------------------------------------------------------------
void RouteMapSetEntry(Node* node,
                      RouteMapEntry* entry,
                      RouteMapValueSet* valueSet)
{
    RouteMapSetCmd* setCriteria = entry->setHead;

    while (setCriteria)
    {
        (setCriteria->setFunc)(node, setCriteria->params,
            valueSet);
        setCriteria = setCriteria->next;
    }
}




//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchEntry
// PURPOSE   Perform the match action
// ARGUMENTS node, the concerned node
//           entry, the node where route map is set
//           valueSet, the set of values to match with the criteria
// RETURN    TRUE if its a match, else
//--------------------------------------------------------------------------
BOOL RouteMapMatchEntry(Node* node,
                        RouteMapEntry* entry,
                        RouteMapValueSet* valueSet)
{
    RouteMapMatchCmd* matchCriteria = entry->matchHead;

    while (matchCriteria)
    {
        if ((matchCriteria->matchFunc)(node, matchCriteria->params,
            valueSet))
        {
            // if it has matched we break off. Like matches in the same
            //  route map subblock are filtered with "or" semantics
            return TRUE;
        }
        else
        {
            // if it hasnt matched we move to the next criteria.
            //  Dissimilar match clauses are filtered with "and" semantics
            matchCriteria = matchCriteria->next;
        }
    }

    return FALSE;
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapAction
// PURPOSE   Perform the match and set action
// ARGUMENTS node, the node where route map is set
//           valueSet, the set of values to match with the criteria
//           entry, the route map entry that is to be analyzed
// RETURN    BOOL. Returns whether the route map matches or not. TRUE means
//              the route map has matched and FALSE means the route map ha
//              not matched or no valid route map has come.
//--------------------------------------------------------------------------
BOOL RouteMapAction(Node* node,
                    RouteMapValueSet* valueSet,
                    RouteMapEntry* entry)
{
    if (!entry)
    {
        // no route map defined..warn the user
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\nNo route map defined. All route "
            "map actions aborted...\n", node->nodeId);
        ERROR_ReportWarning(errString);
        return FALSE;
    }

    // test the entry with the value set
    if (RouteMapMatchEntry(node, entry, valueSet))
    {
        if (entry->type == ROUTE_MAP_PERMIT)
        {
            // set the entries and leave off
            RouteMapSetEntry(node, entry, valueSet);
        }

        // either way we leave
        return TRUE;
    }

    return FALSE;
}




//////////////////////////// IOS Commands //////////////////////////////////


//--------------------------------------------------------------------------
// FUNCTION  RouteMapPrintRMapEntry
// PURPOSE   Print the given route map entry in the console
// ARGUMENTS node, the current node
//           entry, name of route map entry to print
// RETURN    void
// ASSUMPTION Only the type of match and set command is printed.
//              The detailed param list is omitted.
//--------------------------------------------------------------------------
void RouteMapPrintRMapEntry(Node* node, RouteMapEntry* entry)
{
    RouteMapMatchCmd* match = entry->matchHead;
    RouteMapSetCmd* set = entry->setHead;

        printf("\nSequence number : %d\n\n", entry->seq);
        printf("Match clauses:\n");

        while (match)
        {
            switch (match->matchType)
            {
                case AS_PATH:
                {
                    // Developer comments: this is not implemented
                    printf("\tAS_PATH\n");
                    break;
                }
                case COMMUNITY:
                {
                    // Developer Comments: this is not implemented
                    printf("\tCommunity\n");
                    break;
                }
                case EXTCOMMUNITY:
                {
                    // Developer Comments: this is not implemented
                    printf("\tExternal Community\n");
                    break;
                }
                case INTERFACE:
                {
                    RouteMapIntf* intfParam = (RouteMapIntf*) match->params;
                    printf("\tinterface");
                    // check for all the interface param set
                    while (intfParam)
                    {
                        printf(" %d %s",
                            intfParam->type, intfParam->intfNumber);
                        intfParam = intfParam->next;
                    }
                    printf("\n");
                    break;
                }
                case IP_ADDRESS:
                {
                    RouteMapAclList* param
                        = (RouteMapAclList*) match->params;
                    printf("\tip address");
                    while (param)
                    {
                        printf(" %s", param->aclId);
                        param = param->next;
                    }
                    printf("\n");
                    break;
                }
                case IP_NEXT_HOP:
                {
                    RouteMapAclList* param
                        = (RouteMapAclList*) match->params;
                    printf("\tip next-hop");
                    while (param)
                    {
                        printf(" %s", param->aclId);
                        param = param->next;
                    }
                    printf("\n");

                    break;
                }
                case IP_ROUTE_SRC:
                {
                    RouteMapAclList* param
                        = (RouteMapAclList*) match->params;
                    printf("\tip route-source");
                    while (param)
                    {
                        printf(" %s", param->aclId);
                        param = param->next;
                    }
                    printf("\n");

                    break;
                }
                case LENGTH:
                {
                    RouteMapMatchLength* param
                        = (RouteMapMatchLength*) match->params;
                    printf("\tlength %u %u\n", param->min, param->max);
                    break;
                }
                case METRIC:
                {
                    RouteMapMetric* param
                        = (RouteMapMetric*) match->params;
                    printf("\tmetric %u\n", param->metric);
                    break;
                }
                case ROUTE_TYPE:
                {
                    // prints the enum value
                    RouteMapMatchRouteType* param
                        = (RouteMapMatchRouteType*) match->params;
                    printf("\troute-type %d\n", param->rtType);
                    break;
                }
                case TAG:
                {
                    RouteMapMatchTag* param
                        = (RouteMapMatchTag*) match->params;
                    printf("\ttag");
                    while (param)
                    {
                        printf(" %u", param->tag);
                        param = param->next;
                    }
                    printf("\n");
                    break;
                }
                default:
                {
                    break;
                }
            }

            match = match->next;
        }

        printf("\nSet clauses:\n");
        while (set)
        {
            switch (set->setType)
            {
                case AUTO_TAG:
                {
                    printf("\tautomatic-tag\n");
                    break;
                }
                case DEF_INTF:
                {
                    RouteMapIntf* intfParam = (RouteMapIntf*) set->params;
                    printf("\tdefault interface");
                    // check for all the interface param set
                    while (intfParam)
                    {
                        printf(" %d %s",
                            intfParam->type, intfParam->intfNumber);
                        intfParam = intfParam->next;
                    }
                    printf("\n");
                    break;
                }
                case SET_INTERFACE:
                {
                    RouteMapIntf* intfParam = (RouteMapIntf*) set->params;
                    printf("\tinterface");
                    // check for all the interface param set
                    while (intfParam)
                    {
                        printf(" %d %s",
                            intfParam->type, intfParam->intfNumber);
                        intfParam = intfParam->next;
                    }
                    printf("\n");
                    break;
                }
                case SET_IP_NEXT_HOP:
                {
                    RouteMapSetIPNextHop* nextHop =
                        (RouteMapSetIPNextHop*) set->params;
                    RouteMapAddressList* addParam = nextHop->addressHead;
                    printf("\tip next-hop");
                    while (addParam)
                    {
                        char address[MAX_STRING_LENGTH] = {0};
                        IO_ConvertIpAddressToString(addParam->address,
                            address);
                        printf(" %s", address);
                        addParam = addParam->next;
                    }
                    printf("\n");
                    break;
                }
                case IP_DEF_NEXT_HOP:
                {
                    RouteMapSetIPNextHop* nextHop =
                        (RouteMapSetIPNextHop*) set->params;
                    RouteMapAddressList* addParam = nextHop->addressHead;
                    printf("\tip default next-hop");
                    while (addParam)
                    {
                        char address[MAX_STRING_LENGTH] = {0};
                        IO_ConvertIpAddressToString(addParam->address,
                            address);
                        printf(" %s", address);
                        addParam = addParam->next;
                    }
                    printf("\n");
                    break;
                }
                case IP_DEF_NEXT_HOP_VERFY_AVAIL:
                {
                    printf("\tip default next-hop verify-availability\n");
                    break;
                }
                case IP_NEXT_HOP_VERFY_AVAIL:
                {
                    printf("\tip next-hop verify-availability\n");
                    break;
                }
                case LEVEL:
                {
                    RouteMapLevel level = *(RouteMapLevel*) set->params;
                    printf("\tlevel %d\n", level);
                    break;
                }
                case LOCAL_PREF:
                {
                    RouteMapSetLocalPref lpref =
                        *(RouteMapSetLocalPref*) set->params;
                    printf("\tlocal-preference %u\n", lpref.prefValue);
                    break;
                }
                case SET_METRIC:
                {
                    RouteMapMetric metric = *(RouteMapMetric*) set->params;
                    printf("\tmetric %d\n", metric.metric);
                    break;
                }
                case METRIC_TYPE:
                {
                    RouteMapMetricType metricType =
                        *(RouteMapMetricType*) set->params;
                    printf("\tmetric type %d\n", metricType);
                    break;
                }
                case NEXT_HOP:
                {
                    char address[MAX_STRING_LENGTH] = {0};
                    NodeAddress addParam = *(NodeAddress*) set->params;
                    IO_ConvertIpAddressToString(addParam, address);
                    printf("\tip next-hop %s\n", address);
                    break;
                }
                case SET_TAG:
                {
                    RouteMapSetTag *tag = (RouteMapSetTag*) set->params;
                    printf("\ttag %d\n", tag->tagVal);
                    break;
                }
                case IP_PRECEDENCE:
                {
                    RouteMapPrecedence prec =
                        *(RouteMapPrecedence*) set->params;
                    printf("\tprecedence %d\n", prec);
                    break;
                }
                default:
                {
                    break;
                }
            }
            set = set->next;
        }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapPrintRMap
// PURPOSE   Print the given route map in the console
// ARGUMENTS node, the current node
//           rMap, name of route map to print
// RETURN    void
// ASSUMPTION Only the type of match and set command is printed.
//              The detailed param list is omitted.
//--------------------------------------------------------------------------
void RouteMapPrintRMap(Node* node, RouteMap* rMap)
{
    RouteMapEntry* entry = rMap->entryHead;

    if (entry)
    {
        printf("\nPrinting '%s' route map details...\n", rMap->mapTag);
    }

    while (entry)
    {
        RouteMapPrintRMapEntry(node, entry);
        entry = entry->next;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RouteMapShowCmd
// PURPOSE   Simulating the 'show route map' command.
// ARGUMENTS node, the current node
//           rMapName, name of route map for showing specific route map
//           or NULL' keyword to show all.
// RETURN    void
//--------------------------------------------------------------------------
void RouteMapShowCmd(Node* node, char* rMapName)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    // check if any route map enabled for this node
    if (!ip->routeMapList)
    {
        printf("No Route-map declared for node %u...\n", node->nodeId);
        return;
    }

    if (rMapName == NULL)
    {
        RouteMap* rMap = ip->routeMapList->head;

        // showing all
        printf("\nNode: %u, Showing All route maps...\n\n", node->nodeId);

        while (rMap)
        {
            RouteMapPrintRMap(node, rMap);
            rMap = rMap->next;
        }
    }
    else
    {
        RouteMap* rMap = RouteMapFindByName(node, rMapName);

        if (rMap == NULL)
        {
            // not proper syntax
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\t%s\nInvalid syntax "
                "'RouteMapShowCmd(...)'.\nEnter corresponding "
                "node and 'NULL' for showing all or specific "
                "route map name.\n", node->nodeId, rMapName);
            ERROR_ReportError(errString);
        }
        else
        {
            printf("Node: %u, Printing '%s' route map details...\n",
                node->nodeId, rMap->mapTag);
            RouteMapPrintRMap(node, rMap);
        }
    }
}

