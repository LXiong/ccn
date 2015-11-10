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
// File      :  parse.h
// Objectives:  Common repository for generic functions.
// Note      :  These functions are used in access list source file. In
//              order to use these functions in route map we dump these
//              functions in this common file.

////////////////////////////////////////////////////////////////////////////

#ifndef RT_PARSE_UTIL_H
#define RT_PARSE_UTIL_H


// interface types
typedef enum
{
    ASYNC,
    ATM,
    BRI,
    BVI,
    CBR,
    CABLE,
    DIALER,
    ETHERNET,
    FDDI,
    GROUP_ASYNC,
    HSSI,
    LEX,
    LOOPBACK,
    NULL_INTERFACE,
    PORT_CHANNEL,
    SERIAL,
    TOKENRING,
    TUNNEL,
    VIRTUAL_TEMPLATE,
    VIRTUAL_TOKENRING,
    VLAN,
    INVALID_TYPE
} RtInterfaceType;



//--------------------------------------------------------------------------
// FUNCTION  RtParseMallocAndSet
// Old FUNCTION  AccessListMalloc
// PURPOSE   Allocate memeory and set all the values to 0
// ARGUMENTS size, size of chunk to be allocated
// RETURN    pointer to allocated memory
//--------------------------------------------------------------------------
void* RtParseMallocAndSet(size_t size);


//--------------------------------------------------------------------------
// FUNCTION  RtParseGetPositionInString
// Old FUNCTION  AccessListGetPosition
// PURPOSE   Find the terminating position of the token in a string
// ARGUMENTS criteria, the string holding the token
//           lastToken, the token whose terminating position to be found
// RETURN    position where token terminates in a string
//--------------------------------------------------------------------------
int RtParseGetPositionInString(char* criteria, char* lastToken);


//--------------------------------------------------------------------------
// FUNCTION  RtParseCountDotSeparator
// Old FUNCTION  AccessListCountDotSeparator
// PURPOSE   Find the number of dots in a given string
// ARGUMENTS string, the string
// RETURN    int
//--------------------------------------------------------------------------
int RtParseCountDotSeparator(char* string);


//--------------------------------------------------------------------------
// FUNCTION  RtParseStringAllocAndCopy
// Old FUNCTION  AccessListStringAllocAndCopy
// PURPOSE   Copy a string after allocating required memory
// ARGUMENTS src, the string to be copied
// RETURN    return copied string
//--------------------------------------------------------------------------
char* RtParseStringAllocAndCopy(const char* src);


//--------------------------------------------------------------------------
// FUNCTION  RtParseStringConvertToUpperCase
// Old FUNCTION  AccessListConvertToUpper
// PURPOSE   Convert a string to upper case
// ARGUMENTS source, the string to be converted
// RETURN    void
//--------------------------------------------------------------------------
void RtParseStringConvertToUpperCase(char* source);


//--------------------------------------------------------------------------
// FUNCTION  RtParseStringNoCaseCompare
// Old FUNCTION  AccessListStringNoCaseCompare
// PURPOSE   Compare case insensitive string
// ARGUMENTS source, the string to be converted
// RETURN    value of compare, follows the normal strcmp value
//--------------------------------------------------------------------------
int RtParseStringNoCaseCompare(const char* stringOne, const char* stringTwo);


//--------------------------------------------------------------------------
// FUNCTION  RtParseValidateAndConvertToInt
// PURPOSE   Convert a string to integer after validation
// ARGUMENTS token, the string to be converted
// RETURN    The integer value
//--------------------------------------------------------------------------
int RtParseValidateAndConvertToInt(char* token);



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
    BOOL* isAnother);


//--------------------------------------------------------------------------
// FUNCTION  RtParseIsPktFrmMe
// PURPOSE   This function is used to check whether the corresponding packet
//              has originated from me??
// ARGUMENTS node, The node initializing
//           srcAddress, the source address
// RETURN    None
//--------------------------------------------------------------------------
BOOL RtParseIsPktFrmMe(Node* node, NodeAddress srcAddress);



//--------------------------------------------------------------------------
// FUNCTION  RouteMapFindInterfaceType
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
RtInterfaceType RtParseFindInterfaceType(char* token1);



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
// [node-id] INTERFACE-TYPE interface-index interface-type interface-number
//--------------------------------------------------------------------------
void RtParseInterfaceTypeAndNumber(Node* node,
                                   int interfaceIndex,
                                   char* orgnlString);



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
int RtParseGetInterfaceIndex(Node *node, RtInterfaceType type, char* number);

#endif // RT_PARSE_UTIL_H
