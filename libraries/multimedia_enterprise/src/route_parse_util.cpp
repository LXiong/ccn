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

////////////////////////////////////////////////////////////////////////////
//
// File      :  rt_parse_util.c
// Objectives:  Repository for generic functions used while parsing router
//               configuration files.
// Note      :  These functions are used in access list source file. In
//              order to use these functions in route map we dump these
//              functions in this common file.

////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "main.h"
#include "api.h"
#include "qualnet_error.h"
#include "network_ip.h"
#include "route_parse_util.h"



//--------------------------------------------------------------------------
// FUNCTION  RtParseMallocAndSet
// Old FUNCTION  AccessListMalloc
// PURPOSE   Allocate memeory and set all the values to 0
// ARGUMENTS size, size of chunk to be allocated
// RETURN    pointer to allocated memory
//--------------------------------------------------------------------------
void* RtParseMallocAndSet(size_t size)
{
    void* ptr = MEM_malloc(size);
    memset(ptr, 0, size);
    return ptr;
}


//--------------------------------------------------------------------------
// FUNCTION  RtParseGetPositionInString
// Old FUNCTION  AccessListGetPosition
// PURPOSE   Find the terminating position of the token in a string
// ARGUMENTS criteria, the string holding the token
//           lastToken, the token whose terminating position to be found
// RETURN    position where token terminates in a string
//--------------------------------------------------------------------------
int RtParseGetPositionInString(char* criteria, char* lastToken)
{
   // count in the 'criteria' string where lastToken ends
   return((int)(strstr(criteria, lastToken) - criteria + strlen(lastToken)));
}


//--------------------------------------------------------------------------
// FUNCTION  RtParseCountDotSeparator
// Old FUNCTION  AccessListCountDotSeparator
// PURPOSE   Find the number of dots in a given string
// ARGUMENTS string, the string
// RETURN    int
//--------------------------------------------------------------------------
int RtParseCountDotSeparator(char* string)
{
    int count = 0;
    while (*string != '\0')
    {
        if (*string == '.')
        {
            count++;
        }
        string++;
    }
    return count;
}


//--------------------------------------------------------------------------
// FUNCTION  RtParseStringAllocAndCopy
// Old FUNCTION  AccessListStringAllocAndCopy
// PURPOSE   Copy a string after allocating required memory
// ARGUMENTS src, the string to be copied
// RETURN    return copied string
//--------------------------------------------------------------------------
char* RtParseStringAllocAndCopy(const char* src)
{
    char* dest = (char*) RtParseMallocAndSet(strlen(src) + 1);
    strcpy(dest, src);
    return dest;
}


//--------------------------------------------------------------------------
// FUNCTION  RtParseStringConvertToUpperCase
// Old FUNCTION  AccessListConvertToUpper
// PURPOSE   Convert a string to upper case
// ARGUMENTS source, the string to be converted
// RETURN    void
//--------------------------------------------------------------------------
void RtParseStringConvertToUpperCase(char* source)
{
    while (*source != '\0')
    {
        *source = (char) toupper(*source);
        source++;
    }
}


//--------------------------------------------------------------------------
// FUNCTION  RtParseStringNoCaseCompare
// Old FUNCTION  AccessListStringNoCaseCompare
// PURPOSE   Compare case insensitive string
// ARGUMENTS source, the string to be converted
// RETURN    value of compare, follows the normal strcmp value
//--------------------------------------------------------------------------
int RtParseStringNoCaseCompare(const char* stringOne, const char* stringTwo)
{
    char* string1;
    char* string2;
    int value = -1;

    string1 = (char*) RtParseMallocAndSet(strlen(stringOne) + 1);
    strcpy(string1, stringOne);
    RtParseStringConvertToUpperCase(string1);

    string2 = (char*) RtParseMallocAndSet(strlen(stringTwo) + 1);
    strcpy(string2, stringTwo);
    RtParseStringConvertToUpperCase(string2);

    value = strcmp(string1, string2);

    MEM_free(string1);
    MEM_free(string2);

    return value;
}



//--------------------------------------------------------------------------
// FUNCTION  RtParseValidateAndConvertToInt
// PURPOSE   Convert a string to integer after validation
// ARGUMENTS token , the string to be converted
// RETURN    The integer value
// COMMENT   Dont use this function for negtaive numbers
//--------------------------------------------------------------------------
int RtParseValidateAndConvertToInt(char* token)
{
    char* temp = token;

    ERROR_Assert(*temp != '\0',"NULL token recieved.\n");

    while (*temp != '\0')
    {
        if (!isdigit((int)*temp))
        {
            return -1;
        }

        temp++;
    }

    return atoi(token);
}




//--------------------------------------------------------------------------
// FUNCTION  RtParseIdStmt
// PURPOSE   This function is used to identify the corresponding node for
//              whose the router configurations are defined.
// NOTE      The syntax is NODE-IDENTIFIER node-id. This keyword is to be
//              used if a global/common router configuration file is used
//              for defining router commands. If specific/single file is
//              used for defining the router commands this identifier should
//              not be used. ACL, Route map, Route distribution will either
//              accept a single router configuration file where router
//              defintions for all the nodes are defined or individual file's
//              for all the nodes. A combination of these two are not
//              supported.
// ARGUMENTS node, The node initializing
//           rtConfig, Router configuration file
//           index, line number of Router config file
//           foundRouterConfiguration, whether specific Router found or not
//           isAnother, should we continue
// RETURN    None
//--------------------------------------------------------------------------
void RtParseIdStmt(Node* node,
    const NodeInput rtConfig,
    int index,
    BOOL* foundRouterConfiguration,
    BOOL* gotForThisNode,
    BOOL* isAnother)
{
    NodeAddress nodeIdInFile;
    // Check if the following lines are meant for this node
    sscanf(rtConfig.inputStrings[index], "%*s %u", &nodeIdInFile);

    // match not found for this node
    if (node->nodeId != nodeIdInFile)
    {
        if (*gotForThisNode)
        {
            // Got another router statement so this router's
            // configuration is complete
            *isAnother = TRUE;
        }
        else
        {
            // Yet to find configuration for this router's entry
            // so continue searching
            *isAnother = FALSE;
        }
    }
    else
    {
        // Node ID's occuring twice
        if (*gotForThisNode)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "Duplicate configuration for %u\n",
                node->nodeId);
            ERROR_ReportError(errString);
        }

        // Found this router's configuration, continue reading the
        // configuration
        *foundRouterConfiguration = TRUE;
        *gotForThisNode = TRUE;
        *isAnother = FALSE;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RtParseIsPktFrmMe
// PURPOSE   This function is used to check whether the corresponding packet
//              has originated from me??
// ARGUMENTS node, The node initializing
// RETURN    None
//--------------------------------------------------------------------------
BOOL RtParseIsPktFrmMe(Node* node, NodeAddress srcAddress)
{
    int i = 0;
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (srcAddress == NetworkIpGetInterfaceAddress(node, i)
            || srcAddress == NetworkIpGetInterfaceBroadcastAddress(node, i))
        {
            return TRUE;
        }
    }

    return FALSE;
}



//--------------------------------------------------------------------------
// FUNCTION  RtParseFindInterfaceType
// PURPOSE   Finding the interface type and assigning the value. If not a
//              valid type, an error is triggered.
// ARGUMENTS token1, the token containing the interface type
// RETURN    Return the interface type
// COMMENTS  The interface types have been collected from two different
//              Cisco sources for maximum accomodation:
// 1) http://www.cisco.com/univercd/cc/td/doc/product/atm/c8540/
//            12_0/13_19/cmd_ref/i.htm#69500
// 2) http://www.cisco.com/univercd/cc/td/doc/product/software/
//            ssr921/rpcr/79012.htm#xtocid262842
//--------------------------------------------------------------------------
RtInterfaceType RtParseFindInterfaceType(char* token1)
{
    if (!RtParseStringNoCaseCompare(token1, "async"))
    {
        return ASYNC;
    }
    else if (!RtParseStringNoCaseCompare(token1, "atm"))
    {
        return ATM;
    }
    else if (!RtParseStringNoCaseCompare(token1, "bri"))
    {
        return BRI;
    }
    else if (!RtParseStringNoCaseCompare(token1, "bvi"))
    {
        return BVI;
    }
    else if (!RtParseStringNoCaseCompare(token1, "cable"))
    {
        return CABLE;
    }
    else if (!RtParseStringNoCaseCompare(token1, "cbr"))
    {
        return CBR;
    }
    else if (!RtParseStringNoCaseCompare(token1, "dialer"))
    {
        return DIALER;
    }
    else if (!RtParseStringNoCaseCompare(token1, "ethernet"))
    {
        return ETHERNET;
    }
    else if (!RtParseStringNoCaseCompare(token1, "fddi"))
    {
        return FDDI;
    }
    else if (!RtParseStringNoCaseCompare(token1, "group-sync"))
    {
        return GROUP_ASYNC;
    }
    else if (!RtParseStringNoCaseCompare(token1, "hssi"))
    {
        return HSSI;
    }
    else if (!RtParseStringNoCaseCompare(token1, "lex"))
    {
        return LEX;
    }
    else if (!RtParseStringNoCaseCompare(token1, "loopback"))
    {
        return LOOPBACK;
    }
    else if (!RtParseStringNoCaseCompare(token1, "null"))
    {
        return NULL_INTERFACE;
    }
    else if (!RtParseStringNoCaseCompare(token1, "port-channel"))
    {
        return PORT_CHANNEL;
    }
    else if (!RtParseStringNoCaseCompare(token1, "serial"))
    {
        return SERIAL;
    }
    else if (!RtParseStringNoCaseCompare(token1, "tokenring"))
    {
        return TOKENRING;
    }
    else if (!RtParseStringNoCaseCompare(token1, "tunnel"))
    {
        return TUNNEL;
    }
    else if (!RtParseStringNoCaseCompare(token1, "virtual-template"))
    {
        return VIRTUAL_TEMPLATE;
    }
    else if (!RtParseStringNoCaseCompare(token1, "virtual-tokenring"))
    {
        return VIRTUAL_TOKENRING;
    }
    else if (!RtParseStringNoCaseCompare(token1, "vlan"))
    {
        return VLAN;
    }
    else
    {
        return INVALID_TYPE;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  RtParseInterfaceTypeAndNumber
// PURPOSE   Finding the interface type and assigning the value. If not a
//              valid type, an error is triggered.
// ARGUMENTS token1, the token containing the interface type
// RETURN    Return the interface type
// COMMENTS  The interface types have been collected from two different
//              Cisco sources for maximum accomodation:
// 1) http://www.cisco.com/univercd/cc/td/doc/product/atm/c8540/
//            12_0/13_19/cmd_ref/i.htm#69500
// 2) http://www.cisco.com/univercd/cc/td/doc/product/software/
//            ssr921/rpcr/79012.htm#xtocid262842
//
// For Interface type and interface number follow the exact syntax:
// [node-id] INTERFACE-TYPE [interface-index] interface-type interface-number
//--------------------------------------------------------------------------
void RtParseInterfaceTypeAndNumber(Node* node,
                                   int interfaceIndex,
                                   char* orgnlString)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    char token1[MAX_STRING_LENGTH] = {0};
    char token2[MAX_STRING_LENGTH] = {0};
    char token3[MAX_STRING_LENGTH] = {0};
    int countStr = 0;

    // tokenize
    countStr = sscanf(orgnlString,"%s %s %s",
        token1, token2, token3);

    if (countStr != 2)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u: Wrong syntax for Interface type"
            " in router config file\n%s\nSyntax: [node-id] INTERFACE-TYPE "
            "[interface-index] interface-type interface-number\n",
            node->nodeId, orgnlString);
        ERROR_ReportError(errString);
    }

    RtInterfaceType type = RtParseFindInterfaceType(token1);
    if (type == INVALID_TYPE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nInappropriate interface type."
            "\n'%s'\n", node->nodeId, orgnlString);
        ERROR_ReportError(errString);
    }

    // Right now we dont have a exact check for interface number
    ip->interfaceInfo[interfaceIndex]->intfType = type;
    ip->interfaceInfo[interfaceIndex]->intfNumber =
        RtParseStringAllocAndCopy(token2);
}



//-----------------------------------------------------------------------------
// FUNCTION     RtParseGetInterfaceIndex()
// PURPOSE      Get interface index.
// PARAMETERS   Node *node
//                  Pointer to node.
//              int interfaceIndex
//                  interface index
//              char* number
//                  interface number
// RETURN       Interface associated with interface type.
//-----------------------------------------------------------------------------
int RtParseGetInterfaceIndex(Node *node, RtInterfaceType type, char* number)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    int i = 0;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if ((ip->interfaceInfo[i]->intfType == type) &&
            (!strcmp(ip->interfaceInfo[i]->intfNumber, number)))
        {
            return i;
        }
    }

    return -1;
}

