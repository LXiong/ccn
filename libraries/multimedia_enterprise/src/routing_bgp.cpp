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

// Assumptions: The Bgp Speakers are themselves border routers. So all
//              traffic will pass through the bgp speakers.
//              Route map is not implemented.
//--------------------------------------------------------------------------
// PURPOSE: Simulate the BGP-4 protocol as specified in RFC 1771
// and the BGP extension for IPv6 as specified in RFC 2545and RFC 2858 .
// ASSUMPTIONS: 1. No authentication required.
//              2. Only one version of BGP is used.
//              3. All internal BGP peers maintain a full IBGP mesh within
//                 the AS.
//--------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "external_socket.h"
#include "network_ip.h"
#include "app_util.h"
#include "routing_bgp.h"
#include "transport_tcp.h"
#include "routing_ospfv2.h"
#include "routing_ospfv3.h"
#include "routing_ripng.h"
#include "ipv6.h"
#include "external_util.h"
#ifdef ADDON_DB
#include "db.h"
#endif

// Comment this debug not to see different type of message bgp is
// exchanging and the nodes are taking part in the bgp protocol

#define DEBUG_UPDATE        0
#define DEBUG_FORWARD_TABLE 0
#define DEBUG_TABLE         0
#define DEBUG_COLLISION     0
#define DEBUG_FINALIZE      0
#define DEBUG_CONNECTION    0
#define DEBUG_FAULT         0
#define DEBUG               0
#define DEBUG_OPEN_MSG      0
#define DEBUG_FSM           0

//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeSetOptional()
//
// PURPOSE:       Set the value of optional for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//                   optional, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static void BgpPathAttributeSetOptional(unsigned char *bgpPathAttr,
                                        BOOL optional)
{
    unsigned char x = (unsigned char) optional;

    //masks optional within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *bgpPathAttr = *bgpPathAttr & (~(maskChar(1, 1)));

    //setting the value of optional in bgpPathAttr
    *bgpPathAttr = *bgpPathAttr | LshiftChar(x, 1);

}


//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeSetTransitive()
//
// PURPOSE:       Set the value of transitive for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//                transitive, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static void BgpPathAttributeSetTransitive(unsigned char *bgpPathAttr,
                                          BOOL transitive)
{
    unsigned char x = (unsigned char) transitive;

    //masks transitive within boundry range
    x = x & maskChar(8, 8);

    //clears the second bit
    *bgpPathAttr = *bgpPathAttr & (~(maskChar(2, 2)));

    //setting the value of transitive in bgpPathAttr
    *bgpPathAttr = *bgpPathAttr | LshiftChar(x, 2);
}


//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeSetPartial()
//
// PURPOSE:       Set the value of partial for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//                   partial, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static void BgpPathAttributeSetPartial(unsigned char *bgpPathAttr,
                                       BOOL partial)
{
    unsigned char x = (unsigned char) partial;

    //masks partial within boundry range
    x = x & maskChar(8, 8);

    //clears the third bit
    *bgpPathAttr = *bgpPathAttr & (~(maskChar(3, 3)));

    //setting the value of partial in bgpPathAttr
    *bgpPathAttr = *bgpPathAttr | LshiftChar(x, 3);
}


//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeSetExtndLen()
//
// PURPOSE:       Set the value of extendedLength for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//             extendedLength, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static void BgpPathAttributeSetExtndLen(unsigned char *bgpPathAttr,
                                        BOOL extendedLength)
{
    unsigned char x = (unsigned char) extendedLength;

    //masks extendedLength within boundry range
    x = x & maskChar(8, 8);

    //clears the fourth bit
    *bgpPathAttr = *bgpPathAttr & (~(maskChar(4, 4)));

    //setting the value of extendedLength in bgpPathAttr
    *bgpPathAttr = *bgpPathAttr | LshiftChar(x, 4);
}


//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeSetReserved()
//
// PURPOSE:       Set the value of reserved for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//                   reserved, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static void BgpPathAttributeSetReserved(unsigned char *bgpPathAttr,
                                        unsigned char reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskChar(5, 8);

    //clears the last 4 bits
    *bgpPathAttr = *bgpPathAttr & maskChar(1, 4);

    //setting the value of reserved in bgpPathAttr
    *bgpPathAttr = *bgpPathAttr | reserved;
}


//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeGetOptional()
//
// PURPOSE:       Returns the value of optional for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//
// RETURN:        BOOL
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static BOOL BgpPathAttributeGetOptional(unsigned char bgpPathAttr)
{
    //masks reserved within boundry range
    unsigned char optional = bgpPathAttr;

    //clears all the bits except first
    optional = optional & maskChar(1, 1);

    //right shifts so that last bit represents optional
    optional = RshiftChar(optional, 1);

    return (BOOL)optional;
}


//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeGetTransitive()
//
// PURPOSE:       Returns the value of transitive for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//
// RETURN:        BOOL
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static BOOL BgpPathAttributeGetTransitive(unsigned char bgpPathAttr)
{
    unsigned char transitive = bgpPathAttr;

    //clears all the bits except second
    transitive = transitive & maskChar(2, 2);

    //right shifts so that last bit represents transitive
    transitive = RshiftChar(transitive, 2);

    return (BOOL)transitive;
}


//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeGetPartial()
//
// PURPOSE:       Returns the value of partial for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//
// RETURN:        BOOL
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static BOOL BgpPathAttributeGetPartial(unsigned char bgpPathAttr)
{
    unsigned char partial = bgpPathAttr;

    //clears all the bits except third
    partial = partial & maskChar(3, 3);

    //right shifts so that last bit represents partial
    partial = RshiftChar(partial, 3);

    return (BOOL)partial;
}


//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeGetExtndLen()
//
// PURPOSE:       Returns the value of extendedLength for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//
// RETURN:        BOOL
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static BOOL BgpPathAttributeGetExtndLen(unsigned char bgpPathAttr)
{
    unsigned char extendedLength = bgpPathAttr;

    //clears all the bits except fourth
    extendedLength = extendedLength & maskChar(4, 4);

    //right shifts so that last bit represents extendedLength
    extendedLength = RshiftChar(extendedLength, 4);

    return (BOOL)extendedLength;
}


//-------------------------------------------------------------------------
// NAME:          BgpPathAttributeGetReserved()
//
// PURPOSE:       Returns the value of reserved for BgpPathAttribute
//
// PARAMETERS:    bgpPathAttr, The variable containing the value of
//                             optional,transitive,partial,extendedLength
//                             and reserved
//
// RETURN:        BOOL
//
// ASSUMPTION:    None
//-------------------------------------------------------------------------
static unsigned char BgpPathAttributeGetReserved(unsigned char bgpPathAttr)
{
    unsigned char resv = bgpPathAttr;

    //clears the first 4 bits
    resv = resv & maskChar(5, 8);

    return resv;
}

extern void BgpSwapBytes(unsigned char* bgpPacket,
                  BOOL in);
static
void BgpPrintStruct(
    Node* node,
    BgpUpdateMessage* bgpUpdateMessage,
    BgpData* bgp,
    BOOL outgoing = TRUE);
//--------------------------------------------------------------------------
// FUNCTION      BgpIsSamePrefix
//
// PURPOSE:  To compare two BGP route info
//
// PARAMETERS:rtf1,rtf2
//
//
//
// RETURN:      TRUE if two route info are same
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL BgpIsSamePrefix(BgpRouteInfo rtf1,BgpRouteInfo rtf2)
{
    if ((rtf1.prefixLen == rtf2.prefixLen) &&
         (Address_IsSameAddress(&rtf1.prefix,&rtf2.prefix)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
/**
// API       :: BgpFindAttr
// PURPOSE   :: find the header in the UPDATE message
// RETURN    :: int
// **/
int BgpFindAttr(BgpUpdateMessage* updateMessage,
                AttributeTypeCode attr,
                int numAttr,
                BOOL& found)
{
    int i;
    found = FALSE;
    for (i = 0; i < numAttr; i++)
    {
        if (updateMessage->pathAttribute[i].attributeType == attr)
        {
            found = TRUE;
            break;
        }
    }
    return i;
}

/**
// API       :: BgpSwapOpen
// PURPOSE   :: Swap header bytes for the hello message
// PARAMETERS::
// + m : olsrmsg * : The start of the  hello message
// + in : BOOL: TRUE if the packet is coming in to QualNet, FALSE if
//               outgoing
// RETURN    :: void
// **/
void BgpSwapOpen(unsigned char* bgpPacket)
{
    unsigned char* next = bgpPacket;

    EXTERNAL_ntoh(
            next,
            BGP_VERSION_FIELD_LENGTH);
    next += BGP_VERSION_FIELD_LENGTH;

    EXTERNAL_ntoh(
            next,
            BGP_ASID_FIELD_LENGTH);
    next += BGP_ASID_FIELD_LENGTH;

    EXTERNAL_ntoh(
            next,
            BGP_HOLDTIMER_FIELD_LENGTH);
    next += BGP_HOLDTIMER_FIELD_LENGTH;

    EXTERNAL_ntoh(
        next,
        sizeof(NodeAddress));
    next += sizeof(NodeAddress);

    EXTERNAL_ntoh(
        next,
        BGP_MESSAGE_TYPE_SIZE);
    next += BGP_MESSAGE_TYPE_SIZE;

    return;
}
 /**
// API       :: BgpSwapNotification
// PURPOSE   :: Swap header bytes for the Notification message
// PARAMETERS::
// + m : olsrmsg * : The start of the  TC message
// + in : BOOL: TRUE if the packet is coming in to QualNet, FALSE if
//               outgoing
// RETURN    :: void
// **/
void BgpSwapNotification(unsigned char* bgpPacket)
{

    unsigned char* next = bgpPacket;

    EXTERNAL_ntoh(
            next,
            1);
    next += 1;

    EXTERNAL_ntoh(
        next,
        1);
    next += 1;

    return;


}


void BgpSwapBytes(unsigned char *bgpPacket, BOOL in)
{
     // Byte-swap common header.  This is input from external network so put
    // it in host order first.
    if (in)
    {
        unsigned char* next = bgpPacket;
        // Swap commonheader
        EXTERNAL_ntoh(
                next,
                BGP_SIZE_OF_MARKER);
        next += BGP_SIZE_OF_MARKER;

        EXTERNAL_ntoh(
                next,
                BGP_MESSAGE_LENGTH_SIZE);
        next += BGP_MESSAGE_LENGTH_SIZE;

        EXTERNAL_ntoh(
                next,
                1);
        next += 1;
    }
    unsigned char* temp = bgpPacket;
    unsigned char* temp1 = bgpPacket;
    temp = bgpPacket + BGP_SIZE_OF_MARKER + BGP_MESSAGE_LENGTH_SIZE;
    temp1 = bgpPacket + BGP_SIZE_OF_MARKER + BGP_MESSAGE_LENGTH_SIZE + 1;

    switch (*temp)
    {
        case BGP_OPEN:
            BgpSwapOpen(temp1);
            break;

        case BGP_NOTIFICATION:
            BgpSwapNotification(temp1);
            break;

        case BGP_KEEP_ALIVE:
            break;

        case BGP_UPDATE:
            break;

        default:
            ERROR_ReportError("Unknown BGP packet type");
            break;
    }

    // Byte-swap common header.  This is output to network, so put it in
    // network order last.
    if (!in)
    {
        unsigned char* next = bgpPacket;
        // Swap commonheader
        EXTERNAL_ntoh(
                next,
                BGP_SIZE_OF_MARKER);
        next += BGP_SIZE_OF_MARKER;

        EXTERNAL_ntoh(
                next,
                BGP_MESSAGE_LENGTH_SIZE);
        next += BGP_MESSAGE_LENGTH_SIZE;

        EXTERNAL_ntoh(
                next,
                1);
        next += 1;
    }
    return;
}

//--------------------------------------------------------------------------
// FUNCTION   GetNumAttrInUpdate
//
// PURPOSE:  To get number of attributes in an Update packet structure
//
// PARAMETERS: BgpUpdateMessage* bgpUpdateMessage
//
//
//
// RETURN:     Number of attributes
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------
static
int GetNumAttrInUpdate(BgpUpdateMessage* bgpUpdateMessage)
{

    int numEntry = 0;
    int attrLength = bgpUpdateMessage->totalPathAttributeLength;
    while (attrLength)
    {
        int type = 0;
        int total_attr_len=0;
        BOOL is_extend_len;
        is_extend_len =
            BgpPathAttributeGetExtndLen(
            bgpUpdateMessage->pathAttribute[numEntry].bgpPathAttr);
        if (is_extend_len)
        {
            total_attr_len = 4;
        }
        else
        {
            total_attr_len = 3;
        }

        // skip attribute flag, pointing to type
        type = bgpUpdateMessage->pathAttribute[numEntry].attributeType;
        // Type Origin
        if (type == BGP_PATH_ATTR_TYPE_ORIGIN)
        {
            // the type value is origin so increment the no of entry and
            // increment the temp pointer to the next attribute
            numEntry++;

            // length of origin attribute is type + length + 1 byte = 4

            // PathAttributeValue size + 1
            attrLength -= (total_attr_len
                    + sizeof(OriginValue));
        }
        // Type next hop
        else if (type == BGP_PATH_ATTR_TYPE_NEXT_HOP)
        {
            // the type value is next hop so increment the no of entry and
            // increment the temp pointer to the next attribute
            numEntry++;

            // length of next attribute is type + length + 4 = 7

            // PathAttributeValue size + next hop size
            attrLength -= (total_attr_len
                    + sizeof(NextHopValue));
        }

        // Type AS path
        else if (type == BGP_PATH_ATTR_TYPE_AS_PATH)
        {
            unsigned short lengthOfAttributeValue =
                  bgpUpdateMessage->pathAttribute[numEntry].attributeLength;

            numEntry++;


            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
                    + lengthOfAttributeValue);
        }

        // Type Multi Exit Disc
        else if (type == BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC)
        {
            unsigned short lengthOfAttributeValue =
                  bgpUpdateMessage->pathAttribute[numEntry].attributeLength;

            numEntry++;


            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
                    + lengthOfAttributeValue);
        }

        // Type Local Preference
        else if (type == BGP_PATH_ATTR_TYPE_LOCAL_PREF)
        {
            unsigned short lengthOfAttributeValue =
                bgpUpdateMessage->pathAttribute[numEntry].attributeLength;

            numEntry++;


            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
                    + lengthOfAttributeValue);
        }
        else if (type == BGP_MP_REACH_NLRI)
        {
            unsigned short lengthOfAttributeValue =
                bgpUpdateMessage->pathAttribute[numEntry].attributeLength;
            numEntry++;

          // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
               + lengthOfAttributeValue);

        }
        else if (type == BGP_MP_UNREACH_NLRI)
        {
            unsigned short lengthOfAttributeValue =
                bgpUpdateMessage->pathAttribute[numEntry].attributeLength;
            numEntry++;

                // the next byte contains the length of attribute

                // PathAttributeValue size + attribute value length
                attrLength -= (total_attr_len
                   + lengthOfAttributeValue);
        }
        else if (type == BGP_PATH_ATTR_TYPE_ORIGINATOR_ID)
        {
            unsigned short lengthOfAttributeValue =
            bgpUpdateMessage->pathAttribute[numEntry].attributeLength;
            numEntry++;
            // the next byte contains the length of attribute

            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
               + lengthOfAttributeValue);
        }
        else if (type == BGP_PATH_ATTR_TYPE_CLUSTER_ID)
        {
             numEntry++;
             attrLength -= (BGP_ATTRIBUTE_FLAG_SIZE + BGP_ATTRIBUTE_TYPE_SIZE);
        }
        else
        {
            ERROR_Assert(FALSE, "Unrecongnized path attribute type\n");
        }
        // Continue until the end of all the attributes
    }
    return numEntry;
}

//--------------------------------------------------------------------------
// FUNCTION   GetNumNlriInUpdate
//
// PURPOSE:  To get number of NLRIs in an Update packet structure
//
// PARAMETERS: BgpUpdateMessage* bgpUpdateMessage
//
//
//
// RETURN:     Number of NLRIs
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------
int GetNumNlriInUpdate(BgpRouteInfo* nlriRoutes,
                       int totalSizeOfNlri,
                       BOOL outgoing)
{
    int numBytesInPrefix = 0, i = 0;
    // Output the Nlri field of the update message
    while (totalSizeOfNlri > 0)
    {
        if (outgoing)
        {
            numBytesInPrefix = 4;
        }
        else
        {
            if ((nlriRoutes[i].prefixLen <= 8) &&
                (nlriRoutes[i].prefixLen >= 1))
            {
                numBytesInPrefix = 1;
            }
            else if ((nlriRoutes[i].prefixLen <= 16) &&
                    (nlriRoutes[i].prefixLen >= 9))
            {
                numBytesInPrefix = 2;
            }
            else if ((nlriRoutes[i].prefixLen <= 24) &&
                (nlriRoutes[i].prefixLen >= 17))
            {
                numBytesInPrefix = 3;
            }
            else if (nlriRoutes[i].prefixLen >= 25)
            {
                numBytesInPrefix = 4;
            }
        }
        totalSizeOfNlri = totalSizeOfNlri -
                          (sizeof(char) + numBytesInPrefix);
        i++;
    }
    return i;
}
//--------------------------------------------------------------------------
// FUNCTION  BgpPrintStruct
//
// PURPOSE:  To print the different fields of the update message structure
//
// PARAMETERS: bgpUpdateMessage, the update message structure
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpPrintStruct(
    Node* node,
    BgpUpdateMessage* bgpUpdateMessage,
    BgpData* bgp,
    BOOL outgoing)
{
    unsigned short i = 0;
    int totalSizeOfNlri = 0;
    BgpRouteInfo* infeasibleRoutes = NULL;
    BgpRouteInfo* nlriRoutes = NULL;
    int nodeId = 0;
    int numOfNlri = 0,sizeOfNlri =0;
    ERROR_Assert(bgpUpdateMessage, "Invalid update message\n");
    int index = 0;

    if (node)
    {
        nodeId = node->nodeId;
    }

    printf("\n\n-------------------------------------------------------\n"
        "node id == %u\n-------------------------------------------------"
        "---------\n", nodeId);

    printf("Length of packet = %5d\n"
       "Length of Infeasible routes field: %d\n",
       bgpUpdateMessage->commonHeader.length,
       bgpUpdateMessage->infeasibleRouteLength);

    if (bgpUpdateMessage->infeasibleRouteLength)
    {
        infeasibleRoutes = bgpUpdateMessage->withdrawnRoutes;

        for (i = 0; i < (bgpUpdateMessage->infeasibleRouteLength
                        / BGP_IPV4_ROUTE_INFO_STRUCT_SIZE); i++)
        {
            int prefix;
            char ipAddress[MAX_IP_STR_LEN];

            memcpy(&prefix,
                   &infeasibleRoutes[i].prefix.interfaceAddr.ipv4,
                   sizeof(unsigned int));

            IO_ConvertIpAddressToString(prefix, ipAddress);

            printf("%6d\t%15s\n", infeasibleRoutes[i].prefixLen,
                   ipAddress);
        }
    }

    printf("length of Attribute Field : %d\n",
       bgpUpdateMessage->totalPathAttributeLength);

    if (bgpUpdateMessage->totalPathAttributeLength)
    {
        int numAttr = GetNumAttrInUpdate(bgpUpdateMessage);

        char nextHopIp[MAX_IP_STR_LEN];
        unsigned int medOrLocalpref = 0;

        for (int i = 0; i < numAttr; i++)
        {
            if (bgpUpdateMessage->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_ORIGIN)
            {
                // Output the origin value field
                printf("-------------------origin-------------------------\n"
                "optional        :%1d\n"
                "transitive      :%1d\n"
                "partial         :%1d\n"
                "extendedLength  :%1d\n"
                "reserved        :%1d\n"
                "attributeType   :%d\n"
                "attributeLength :%d\n"
                "attributeValue  :%d\n",
                BgpPathAttributeGetOptional
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetTransitive
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetPartial
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetExtndLen
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetReserved
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                bgpUpdateMessage->pathAttribute[i].attributeType,
                bgpUpdateMessage->pathAttribute[i].attributeLength,
                *(bgpUpdateMessage->pathAttribute[i].attributeValue));
            }
            else if (bgpUpdateMessage->pathAttribute[i].attributeType ==
                            BGP_PATH_ATTR_TYPE_AS_PATH)
            {
                BgpPathAttributeValue* path_attr = NULL;
                int path_type = -1, path_seg_length = 0;
                if (bgpUpdateMessage->pathAttribute[i].attributeLength)
                {
                    path_attr = ((BgpPathAttributeValue*)
                        (bgpUpdateMessage->pathAttribute[i].attributeValue));
                    path_type = path_attr->pathSegmentType;
                    path_seg_length = path_attr->pathSegmentLength;
                }
                // Output the As path field
                printf("-------------------As Paths-----------------------\n"
                "optional        :%1d\n"
                "transitive      :%1d\n"
                "partial         :%1d\n"
                "extendedLength  :%1d\n"
                "reserved        :%1d\n"
                "attributeType   :%d\n"
                "attributeLength :%d\n"
                "path segment type    :%d\n"
                "path segment length :%d [",
                BgpPathAttributeGetOptional
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetTransitive
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetPartial
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetExtndLen
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetReserved
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                bgpUpdateMessage->pathAttribute[i].attributeType,
                bgpUpdateMessage->pathAttribute[i].attributeLength,

                path_type,
                path_seg_length);

                if (bgpUpdateMessage->pathAttribute[i].attributeLength)
                {
                    for (int j = 0;
                        j < ((BgpPathAttributeValue*)(bgpUpdateMessage->
                             pathAttribute[i].attributeValue))->
                             pathSegmentLength;
                        j++)
                    {
                        printf(" %d",((BgpPathAttributeValue*)
                        (bgpUpdateMessage->pathAttribute[i].attributeValue))
                            ->pathSegmentValue[j]);
                    }

                    printf(" ]");
                }
                printf(" \n");
            }
            else if (bgpUpdateMessage->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_LOCAL_PREF)
            {
                index = 4;
                memcpy(&medOrLocalpref,
                  bgpUpdateMessage->pathAttribute[i].attributeValue,
                  sizeof(unsigned int));

                printf("------------ Local Prefarence -----------\n"
                "optional        :%1d\n"
                "transitive      :%1d\n"
                "partial         :%1d\n"
                "extendedLength  :%1d\n"
                "reserved        :%1d\n"
                "attributeType   :%u\n"
                "attributeLength :%u\n"
                "local Pref      :%u\n",
                 BgpPathAttributeGetOptional
                 (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                 BgpPathAttributeGetTransitive
                 (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                 BgpPathAttributeGetPartial
                 (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                 BgpPathAttributeGetExtndLen
                 (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                 BgpPathAttributeGetReserved
                 (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                 bgpUpdateMessage->pathAttribute[i].attributeType,
                 bgpUpdateMessage->pathAttribute[i].attributeLength,
                 medOrLocalpref);
            }
            else if (bgpUpdateMessage->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC)
            {
                index = 3;
                medOrLocalpref = 0;
                memcpy(&medOrLocalpref,
                bgpUpdateMessage->pathAttribute[i].attributeValue,
                    sizeof(unsigned int));

                printf("------------ MED -----------\n"
                "optional        :%1d\n"
                "transitive      :%1d\n"
                "partial         :%1d\n"
                "extendedLength  :%1d\n"
                "reserved        :%1d\n"
                "attributeType   :%u\n"
                "attributeLength :%u\n"
                "Med             :%u\n",
                BgpPathAttributeGetOptional
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetTransitive
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetPartial
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetExtndLen
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetReserved
                (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                bgpUpdateMessage->pathAttribute[i].attributeType,
                bgpUpdateMessage->pathAttribute[i].attributeLength,
                medOrLocalpref);
            }

            else if (bgpUpdateMessage->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_NEXT_HOP)
            {
                IO_ConvertIpAddressToString(
                    *(unsigned int*)(bgpUpdateMessage->
                    pathAttribute[i].attributeValue),
                    nextHopIp);

                // Output the next hop field
                printf("----------------------Next hop-------------------\n"
                "optional        :%1d\n"
                "transitive      :%1d\n"
                "partial         :%1d\n"
                "extendedLength  :%1d\n"
                "reserved        :%1d\n"
                "attributeType   :%d\n"
                "attributeLength :%d\n"
                "next hop        :%s\n",
                BgpPathAttributeGetOptional
                 (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetTransitive
                 (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetPartial
                  (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetExtndLen
                 (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                BgpPathAttributeGetReserved
                 (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                bgpUpdateMessage->pathAttribute[i].attributeType,
                bgpUpdateMessage->pathAttribute[i].attributeLength,
                nextHopIp);
                index++;
            }
            else if (bgpUpdateMessage->pathAttribute[i].attributeType ==
                BGP_MP_REACH_NLRI)
            {
                MpReachNlri* mpReach =
                    (MpReachNlri* ) bgpUpdateMessage->pathAttribute[i].
                                                      attributeValue;

                sizeOfNlri =
                    bgpUpdateMessage->pathAttribute[i].attributeLength
                    - MBGP_AFI_SIZE - MBGP_SUB_AFI_SIZE
                    - MBGP_NEXT_HOP_LEN_SIZE - mpReach->nextHopLen
                    - MBGP_NUM_SNPA_SIZE;

                numOfNlri = 0;

                if (mpReach->afIden == MBGP_IPV4_AFI)
                    numOfNlri = sizeOfNlri / BGP_IPV4_ROUTE_INFO_STRUCT_SIZE;

                else if (mpReach->afIden == MBGP_IPV6_AFI)
                    numOfNlri = sizeOfNlri / BGP_IPV6_ROUTE_INFO_STRUCT_SIZE;

                Address nextHopAddress;
                        memcpy(&nextHopAddress.interfaceAddr,
                               mpReach->nextHop,
                               mpReach->nextHopLen);
                if (mpReach->nextHopLen == 4)
                {
                    nextHopAddress.networkType = NETWORK_IPV4;
                }
                else if (mpReach->nextHopLen == 16)
                {
                    nextHopAddress.networkType = NETWORK_IPV6;
                }
                IO_ConvertIpAddressToString(&nextHopAddress, nextHopIp);
                printf("------------ Reachability Info-------------------\n"
                   "optional        :%1d\n"
                   "transitive      :%1d\n"
                   "partial         :%1d\n"
                   "extendedLength  :%1d\n"
                   "reserved        :%1d\n"
                   "attributeType   :%u\n"
                   "attributeLength :%u\n"
                   "address types(AFI,SUB_AFI)    :(%1d,%1d)\n"
                   "size of advert routes   :%d\n"
                   "num of advert routes        :%d\n"
                   "next hop         :%s\n",
                   BgpPathAttributeGetOptional
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   BgpPathAttributeGetTransitive
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   BgpPathAttributeGetPartial
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   BgpPathAttributeGetExtndLen
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   BgpPathAttributeGetReserved
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   bgpUpdateMessage->pathAttribute[i].attributeType,
                   bgpUpdateMessage->pathAttribute[i].attributeLength,
                   mpReach->afIden,
                   mpReach->subAfIden,
                   sizeOfNlri,
                   numOfNlri,
                   nextHopIp);

                nlriRoutes = mpReach->Nlri;
                // Output the Nlri field of the update message
                for (int j = 0; j < numOfNlri; j++)
                {
                    char nlri[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(&nlriRoutes[j].prefix, nlri);
                    printf("%2u\t%s\n", nlriRoutes[j].prefixLen, nlri);
                }
                printf("------------------------------------------------\n");
                index++;
            }
            else if (bgpUpdateMessage->pathAttribute[i].attributeType ==
                BGP_MP_UNREACH_NLRI)
            {
                MpUnReachNlri* mpUnreach = (MpUnReachNlri* )
                bgpUpdateMessage->pathAttribute[i].attributeValue;

                sizeOfNlri =
                    bgpUpdateMessage->pathAttribute[i].attributeLength
                - MBGP_AFI_SIZE
                - MBGP_SUB_AFI_SIZE ;

                numOfNlri = 0;
                if (mpUnreach->afIden == MBGP_IPV4_AFI)
                    numOfNlri = sizeOfNlri / BGP_IPV4_ROUTE_INFO_STRUCT_SIZE;

                else if (mpUnreach->afIden == MBGP_IPV6_AFI )
                    numOfNlri = sizeOfNlri / BGP_IPV6_ROUTE_INFO_STRUCT_SIZE;

                printf("------------ UnReachability Info------------------\n"
                   "optional        :%1d\n"
                   "transitive      :%1d\n"
                   "partial         :%1d\n"
                   "extendedLength  :%1d\n"
                   "reserved        :%1d\n"
                   "attributeType   :%u\n"
                   "attributeLength :%u\n"
                   "address type    :%1d\n"
                   "size of withdrawn routes    :%d\n"
                   "num of withdrawn routes :%d\n",
                   BgpPathAttributeGetOptional
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   BgpPathAttributeGetTransitive
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   BgpPathAttributeGetPartial
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   BgpPathAttributeGetExtndLen
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   BgpPathAttributeGetReserved
                   (bgpUpdateMessage->pathAttribute[i].bgpPathAttr),
                   bgpUpdateMessage->pathAttribute[i].attributeType,
                   bgpUpdateMessage->pathAttribute[i].attributeLength,
                   mpUnreach->afIden,
                   sizeOfNlri,
                   numOfNlri);



                nlriRoutes = mpUnreach->Nlri;
                // Output the Nlri field of the update message
                for (int j = 0; j < numOfNlri; j++)
                {
                    char nlri[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(&nlriRoutes[j].prefix, nlri);

                    printf("%2u\t%s\n", nlriRoutes[j].prefixLen, nlri);
                }
                printf("------------------------------------------------\n");
            }
        }//All Attrs printed
    }
    printf("---------------------N.L.R.I---------------------------\n");

    totalSizeOfNlri = bgpUpdateMessage->commonHeader.length
            - BGP_MIN_HDR_LEN
            - BGP_INFEASIBLE_ROUTE_LENGTH_SIZE // for infeasible rt len
            - BGP_PATH_ATTRIBUTE_LENGTH_SIZE   // for total path attrib len
            - bgpUpdateMessage->infeasibleRouteLength
            - bgpUpdateMessage->totalPathAttributeLength;

    nlriRoutes = bgpUpdateMessage->networkLayerReachabilityInfo;
    int numNlri = 0;
    if (totalSizeOfNlri > 0)
    {
        numNlri = GetNumNlriInUpdate(
               bgpUpdateMessage->networkLayerReachabilityInfo,
                                     totalSizeOfNlri,
                                     outgoing);
    }
    int size = 0;
    if (bgp->ip_version == NETWORK_IPV4)
    {
        size = numNlri * BGP_IPV4_ROUTE_INFO_STRUCT_SIZE;
    }
    else
    {
        size = numNlri * BGP_IPV6_ROUTE_INFO_STRUCT_SIZE;
    }

    printf("size = %d  num = %d\n",
           size,
           numNlri);

    printf("-------------------------------------------------------\n");
    i = 0;
    int numBytesInPrefix = 0;
    // Output the Nlri field of the update message
    while (totalSizeOfNlri > 0)
    {
        char nlri[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&nlriRoutes[i].prefix, nlri);

        printf("%2u\t%s\n", nlriRoutes[i].prefixLen, nlri);
        if (outgoing)
        {
            numBytesInPrefix = 4;
        }
        else
        {
            if ((nlriRoutes[i].prefixLen <= 8) &&
                (nlriRoutes[i].prefixLen >= 1))
            {
                numBytesInPrefix = 1;
            }
            else if ((nlriRoutes[i].prefixLen <= 16) &&
                    (nlriRoutes[i].prefixLen >= 9))
            {
                numBytesInPrefix = 2;
            }
            else if ((nlriRoutes[i].prefixLen <= 24) &&
                (nlriRoutes[i].prefixLen >= 17))
            {
                numBytesInPrefix = 3;
            }
            else if (nlriRoutes[i].prefixLen >= 25)
            {
                numBytesInPrefix = 4;
            }
        }
        totalSizeOfNlri = totalSizeOfNlri -
                          (sizeof(char) + numBytesInPrefix);
        i++;
    }
    printf("-------------------------------------------------------\n");

}


//--------------------------------------------------------------------------
// FUNCTION  BgpPrintAdjRibLoc
//
// PURPOSE:  To show the entries of the Rib Local
//
// PARAMETERS:  node, node pointer
//              bgp,  bgp internal structure
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpPrintAdjRibLoc(Node* node, BgpData* bgp)
{
    int i = 0;
    int numEntriesAdjRibLoc = BUFFER_GetCurrentSize(&(bgp->ribLocal))
        / sizeof(BgpAdjRibLocStruct);

    BgpAdjRibLocStruct* adjRibLoc = (BgpAdjRibLocStruct*)
    BUFFER_GetData(&(bgp->ribLocal));

    printf("                               RibLoc of node %u\n"
        "--------------------------------------------------------------"
        "------------------------------\n"
        "      dest             nextHop          "
        "learnedFrom   validity origin adjRibOut\n"
        "--------------------------------------------------------------"
        "------------------------------\n", node->nodeId);

    for (i = 0; i < numEntriesAdjRibLoc; i++)
    {
        char destAddress[MAX_STRING_LENGTH];
        //char maskAddress[MAX_STRING_LENGTH];
        char nextHopAddress[MAX_STRING_LENGTH];
        char peerAddress[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            &adjRibLoc[i].ptrAdjRibIn->destAddress.prefix, destAddress);

        IO_ConvertIpAddressToString(
            &adjRibLoc[i].ptrAdjRibIn->nextHop, nextHopAddress);
        in6_addr bgpInvalidAddr;
        memset(&bgpInvalidAddr,0,sizeof(in6_addr));
        if (CMP_ADDR6(GetIPv6Address(adjRibLoc[i].ptrAdjRibIn->peerAddress),
             bgpInvalidAddr)!= 0)
        {
            IO_ConvertIpAddressToString(
                &adjRibLoc[i].ptrAdjRibIn->peerAddress, peerAddress);
        }
        else
        {
            strcpy(peerAddress, "LOCAL  ");
        }

        printf("%15s  %15s  %15s %5u %5u %5u\n",
            destAddress,
            nextHopAddress,
            peerAddress,
            adjRibLoc[i].isValid,
            adjRibLoc[i].ptrAdjRibIn->origin,
            adjRibLoc[i].movedToAdjRibOut);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  BgpPrintAdjRibOut
//
// PURPOSE:  To show the current entries in ribOut to advertise to the peer
//
// PARAMETERS:  node, the node pointer
//              connectionPtr, the connection pointer whose rib out is to
//                             be printed
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpPrintAdjRibOut(Node* node, BgpConnectionInformationBase* connPtr)
{
    int i = 0;

    BgpAdjRibLocStruct** adjRibOut = (BgpAdjRibLocStruct**)
                                 BUFFER_GetData(&(connPtr->adjRibOut));

    int numEntries = BUFFER_GetCurrentSize(&(connPtr->adjRibOut))
        / sizeof(BgpAdjRibLocStruct*);

    printf("              AdjRibOut of node %u remote Node %u numEntry %u\n"
        "--------------------------------------------------------------"
        "----------------\n"
        "      dest            nextHop          "
        "learnedFrom    peerAs\n"
        "--------------------------------------------------------------"
        "----------------\n",
        node->nodeId,
        connPtr->remoteId,
        numEntries);

    if (numEntries == 0)
    {
        printf("                         The table is empty           \n");
    }

    for (i = 0; i < numEntries; i++)
    {
        char destAddress[MAX_STRING_LENGTH];
        //char maskAddress[MAX_STRING_LENGTH];
        char nextHopAddress[MAX_STRING_LENGTH];
        char peerAddress[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            &adjRibOut[i]->ptrAdjRibIn->destAddress.prefix,
            destAddress);

        IO_ConvertIpAddressToString(
                &adjRibOut[i]->ptrAdjRibIn->nextHop, nextHopAddress);

        printf("%15s  %15s  ",
            destAddress, nextHopAddress);

        in6_addr bgpInvalidAddr;
        memset(&bgpInvalidAddr,0,sizeof(in6_addr));
        if (CMP_ADDR6(GetIPv6Address(adjRibOut[i]->ptrAdjRibIn->peerAddress),
               bgpInvalidAddr)!= 0  )
        {
            IO_ConvertIpAddressToString(
                &adjRibOut[i]->ptrAdjRibIn->peerAddress, peerAddress);
            printf("%15s  %5u\n", peerAddress,
           adjRibOut[i]->ptrAdjRibIn->asIdOfPeer);
        }
        else
        {
            printf("          LOCAL  LOCAL\n");
        }
    }
    printf("\n");
}


//--------------------------------------------------------------------------
// FUNCTION  BgpPrintWithdrawnRoutes
//
// PURPOSE:
//
// PARAMETERS:  node, the node pointer
//              bgp,  the bgp internal structure
//              connectionPtr, the connection pointer whose rib out is to
//                             be printed
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpPrintWithdrawnRoutes(BgpConnectionInformationBase* connPtr)
{
    BgpRouteInfo* rtInfo = (BgpRouteInfo*)BUFFER_GetData(&connPtr->
        withdrawnRoutes);

    int numWithdrawnRt = BUFFER_GetCurrentSize(&connPtr->withdrawnRoutes)
        / sizeof(BgpRouteInfo);

    int i = 0;
    printf("The withdrawn route field of node %u with %u is as follows"
        "(%d):\n", connPtr->localId, connPtr->remoteId,numWithdrawnRt);

    for (i = 0; i < numWithdrawnRt; i++)
    {
        char address[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&rtInfo[i].prefix, address);
        printf("%15s    %5u\n", address, rtInfo[i].prefixLen);
    }
}


//--------------------------------------------------------------------------
// FUNCTION   : PrintStructBgpRoutinginfo
//
// PURPOSE    : printing content of routing information base for debugging
//
// PARAMETERS : node, the node which is printing the structure
//              bgp,  bgp internal structure
//
// RETURN     : void
//
// ASSUMPTION : none
//--------------------------------------------------------------------------

static
void BgpPrintRoutingInformationBase(Node* node, BgpData* bgp)
{
    BgpRoutingInformationBase* bgpRoutingInfo = NULL;
    int numEntries = 0;
    int i = 0;
    int j = 0;
    int numAs = 0;

    ERROR_Assert(bgp, "Invalid BGP structure\n");

    bgpRoutingInfo = (BgpRoutingInformationBase*)
        BUFFER_GetData(&(bgp->adjRibIn));

    numEntries = BUFFER_GetCurrentSize(&(bgp->adjRibIn))
        / sizeof(BgpRoutingInformationBase);

    printf("                             AdjRibIn of node %u\n"
       "--------------------------------------------------------------"
       "------------------\n"
       "      dest     numHostBits     nextHop        learnedFrom    valid  "
       "isBest   origin asPath LocalPref MED\n"
       "--------------------------------------------------------------"
       "------------------\n", node->nodeId);

    for (i = 0; i < numEntries; i++)
    {
        char destAddress[MAX_STRING_LENGTH];
        char nextHopAddress[MAX_STRING_LENGTH];
        char peerAddress[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(
            &bgpRoutingInfo->destAddress.prefix, destAddress);

        IO_ConvertIpAddressToString(
            &bgpRoutingInfo->nextHop, nextHopAddress);

        in6_addr bgpInvalidAddr;
        memset(&bgpInvalidAddr,0,sizeof(in6_addr));
        if (CMP_ADDR6(GetIPv6Address(bgpRoutingInfo->peerAddress),
               bgpInvalidAddr)!= 0  )
        {
            IO_ConvertIpAddressToString(
                &bgpRoutingInfo->peerAddress, peerAddress);
        }
        else
        {
            strcpy(peerAddress, "LOCAL  ");
        }

        printf("%15s  %u %15s  %15s    %5s   %5s  %5u     ",
           destAddress, bgpRoutingInfo->destAddress.prefixLen,
           nextHopAddress, peerAddress,
           bgpRoutingInfo->isValid ? "TRUE" : "FALSE",
           bgpRoutingInfo->pathAttrBest == BGP_PATH_ATTR_BEST_TRUE
           ? "TRUE" : "FALSE",
           bgpRoutingInfo->origin);

        if (bgpRoutingInfo->asPathList)
        {
            numAs = bgpRoutingInfo->asPathList->pathSegmentLength /
                                                sizeof(unsigned short);
            for (j = 0; j < numAs; j++)
            {
                printf("%u,",
                (short) bgpRoutingInfo->asPathList->pathSegmentValue[j]);
            }
        }
        printf("       %d    %d",bgpRoutingInfo->localPref,
                                 bgpRoutingInfo->multiExitDisc);
        printf("\n");

        bgpRoutingInfo++;
    }
    printf("\n\n");
}


//--------------------------------------------------------------------------
// FUNCTION   : BgpPrintConnInformationBase
//
// PURPOSE    : printing content of connection information base for debugging
//
// PARAMETERS : node, the node which is to print its connection information
//                    base
//              bgp,  bgp internal structure
//
// RETURN     : void
//
// ASSUMPTION : none.
//
//--------------------------------------------------------------------------

static
void BgpPrintConnInformationBase(Node* node, BgpData* bgp)
{
    BgpConnectionInformationBase* bgpConnInfo = NULL;
    int numEntries = 0;
    int i = 0;

    ERROR_Assert(bgp, "Invalid BGP structure\n");

    bgpConnInfo = (BgpConnectionInformationBase*)
        BUFFER_GetData(&(bgp->connInfoBase));

    numEntries = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
        / sizeof(BgpConnectionInformationBase);

    printf("\n\n----------------Connection info. base-----------------\n"
        "                          node->Id = %d\n"
        "------------------------------------------------------\n",
        node->nodeId);

    for (i = 0; i < numEntries; i++)
    {
        char ipAddress[MAX_STRING_LENGTH];
        char bgpTimers[MAX_STRING_LENGTH];

        memset(ipAddress, 0, MAX_STRING_LENGTH);
        memset(bgpTimers, 0, MAX_STRING_LENGTH);
        if ((bgpConnInfo->remoteAddr.networkType == NETWORK_IPV6)||
        (bgpConnInfo->remoteAddr.networkType == NETWORK_IPV4))
        {
            IO_ConvertIpAddressToString(&bgpConnInfo->remoteAddr,
                    &ipAddress[MAX_IP_STR_LEN * 0]);
        }
        if ((bgpConnInfo->localAddr.networkType == NETWORK_IPV6)||
        (bgpConnInfo->localAddr.networkType == NETWORK_IPV4))
        {
            IO_ConvertIpAddressToString(&bgpConnInfo->localAddr,
                    &ipAddress[MAX_IP_STR_LEN * 1]);
        }
        // convert local Bgp Identifier Address to string
        IO_ConvertIpAddressToString(bgpConnInfo->localBgpIdentifier,
                    &ipAddress[MAX_IP_STR_LEN * 2]);

        // convert remote Bgp Identifier Address to string
        IO_ConvertIpAddressToString(bgpConnInfo->remoteBgpIdentifier,
                    &ipAddress[MAX_IP_STR_LEN * 3]);

        // covert bgpConnInfo->startEventTime to string
        ctoa(bgpConnInfo->startEventTime,
         &bgpTimers[MAX_CLOCK_TYPE_STR_LEN * 0]);

        // covert bgpConnInfo->holdTime to string
        ctoa(bgpConnInfo->holdTime,
            &bgpTimers[MAX_CLOCK_TYPE_STR_LEN * 1]);

        // covert bgpConnInfo->nextUpdate to string
        ctoa(bgpConnInfo->nextUpdate,
            &bgpTimers[MAX_CLOCK_TYPE_STR_LEN * 2]);


        printf("Entry number %d is:\n"
            "remoteAddr = %s\n"
            "remotePort = %d\n"
            "localAddr = %s\n"
            "localPort = %d\n"
            "localBgpIdentifier = %s\n"
            "remoteBgpIdentifier = %s\n"
            "asIdOfPeer = %d\n"
            "connectionId = %d\n"
            "uniqueId = %d\n"
            "remoteId = %d\n"
            "localId = %d\n"
            "state = %d\n"
            "connectRetrySeq = %u\n"
            "keepAliveTimerSeq = %u\n"
            "holdTimerSeq = %u\n"
            "startEventTime = %s ns\n"
            "holdTime = %s ns\n"
            "nextUpdate = %s ns\n"
            "internalConnection = %d\n"
            "isInitiatingNode = %d\n"
            "isClient = %d\n"
            "weight = %d\n\n"
            "------------------------------------------------------\n\n",
            i,
            &ipAddress[MAX_IP_STR_LEN * 0], // string remote address
            bgpConnInfo->remotePort,
            &ipAddress[MAX_IP_STR_LEN * 1], // string local address
            bgpConnInfo->localPort,
            &ipAddress[MAX_IP_STR_LEN * 2], // string local bgp identifier
            &ipAddress[MAX_IP_STR_LEN * 3], // string remote bgp identifier
            bgpConnInfo->asIdOfPeer,
            bgpConnInfo->connectionId,
            bgpConnInfo->uniqueId,
            bgpConnInfo->remoteId,
            bgpConnInfo->localId,
            bgpConnInfo->state,
            bgpConnInfo->connectRetrySeq,
            bgpConnInfo->keepAliveTimerSeq,
            bgpConnInfo->holdTimerSeq,
            &bgpTimers[MAX_CLOCK_TYPE_STR_LEN * 0], // startEventTime
            &bgpTimers[MAX_CLOCK_TYPE_STR_LEN * 1], // holdTime
            &bgpTimers[MAX_CLOCK_TYPE_STR_LEN * 2], // nextUpdate Time
            bgpConnInfo->isInternalConnection,
            bgpConnInfo->isInitiatingNode,
            bgpConnInfo->isClient,
            bgpConnInfo->weight);

        bgpConnInfo++;
    }
}

static
void BgpPrintOpenMessage(Node* node, char* openPacket)
{
    unsigned short totalLen = 0;
    unsigned char packetType = 0;
    unsigned char version = 0;
    unsigned short asId = 0;
    unsigned short holdTime = 0;
    char bgpIdentifier[MAX_IP_STR_LEN];
    unsigned char optParameterLen = 0;
    NodeAddress bgpId = 0;

    // pre-calculate minimum size of the open packet
    // (i.e excluding optional fields)
    unsigned short MinOpenMsgSize = BGP_MIN_HDR_LEN
                                    + BGP_MIN_OPEN_MESSAGE_SIZE;

    // get total length
    memcpy(&totalLen,
        (openPacket + BGP_SIZE_OF_MARKER),
        sizeof(unsigned short));

    // get packet type is should be BGP_OPEN
    memcpy(&packetType,
        (openPacket + BGP_SIZE_OF_MARKER + sizeof(unsigned short)),
        sizeof(unsigned char));

    // get BGP version number
    version = *(openPacket + BGP_MIN_HDR_LEN);

    // get AS id from the open packet;
    memcpy(&asId,
        (openPacket + BGP_MIN_HDR_LEN + sizeof(unsigned char)),
        sizeof(unsigned short));

    // get advertised hold time from the open packet
    memcpy(&holdTime,
        (openPacket + BGP_MIN_HDR_LEN + sizeof(char) + sizeof(short)),
        sizeof(unsigned short));

    // get BGP identifier value from the open packet
    memcpy(&bgpId,
        (openPacket + BGP_MIN_HDR_LEN + sizeof(char) + 2 * sizeof(short)),
        sizeof(NodeAddress));

    // get optional paremeter length from open packet.
    optParameterLen = *(openPacket
                        + BGP_MIN_HDR_LEN
                        + sizeof(char)
                        + sizeof(short)
                        + sizeof(short)
                        + sizeof(NodeAddress));

    IO_ConvertIpAddressToString(bgpId, bgpIdentifier);

    printf("---------------------------------------\n"
           "node = %d   length = %d  (Open Message)\n"
           "---------------------------------------\n"
           "BGP version                = %d\n"
           "As Id                      = %d\n"
           "Hold Time                  = %d second\n"
           "Bgp Identifier             = %s\n"
           "Optional Parameter Lenngth = %d\n"
           "---------------------------------------\n",
           node->nodeId,
           totalLen,
           version,
           asId,
           holdTime,
           bgpIdentifier,
           optParameterLen);

    if (optParameterLen == 0)
    {
       ERROR_Assert(MinOpenMsgSize == totalLen, "Malformed open packet");
    }

    ERROR_Assert(packetType == BGP_OPEN, "Not a open packet");
}
//////////////////////////////// END PRINTS /////////////////////////////////

//--------------------------------------------------------------------------
// FUNCTION BgpPrintStats
//
// PURPOSE  to print the statics collected at the end of the simulation
//
// PARAMETERS
//      node, the node whose statistics is to be printed
//
// RETURN
//      void
//--------------------------------------------------------------------------
static
void BgpPrintStats(Node* node)
{
    char buf[MAX_STRING_LENGTH];
    BgpData* bgp = (BgpData*) node->appData.exteriorGatewayVar;

    sprintf (buf, "Open Messages Sent = %u",
        bgp->stats.openMsgSent);
    IO_PrintStat(
        node,
        "Application",
        "BGPv4",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf (buf, "Keep Alive Messages Sent = %u",
        bgp->stats.keepAliveSent);
    IO_PrintStat(
        node,
        "Application",
        "BGPv4",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf (buf, "Update Messages Sent = %u",
        bgp->stats.updateSent);
    IO_PrintStat(
        node,
        "Application",
        "BGPv4",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf (buf, "Reflected Update Messages Sent = %u",
            bgp->stats.reflectedupdateSent);
    IO_PrintStat(
            node,
            "Application",
            "BGPv4",
            ANY_DEST,
            -1, // instance Id
            buf);

    sprintf (buf, "Notification Messages Sent = %u",
        bgp->stats.notificationSent);
    IO_PrintStat(
        node,
        "Application",
        "BGPv4",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf (buf, "Open Messages Received = %u",
        bgp->stats.openMsgReceived);
    IO_PrintStat(
        node,
        "Application",
        "BGPv4",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf (buf, "Keep Alive Messages Received = %u",
        bgp->stats.keepAliveReceived);
    IO_PrintStat(
        node,
        "Application",
        "BGPv4",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf (buf, "Update Messages Received = %u",
        bgp->stats.updateReceived);
    IO_PrintStat(
        node,
        "Application",
        "BGPv4",
        ANY_DEST,
        -1, // instance Id
        buf);

    sprintf (buf, "Notification Messages Received = %u",
        bgp->stats.notificationReceived);
    IO_PrintStat(
        node,
        "Application",
        "BGPv4",
        ANY_DEST,
        -1, // instance Id
        buf);
}


//--------------------------------------------------------------------------
// NAME:        BgpGetConnInfoFromUniqueId
//
// PURPOSE:     Finding the connection information structure for a given
//              uniqueId
//
// PARAMETERS:  bgp, the bgp internal structure
//              uniqueId, the unique id to be searched
//
// RETURN:      void.
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
BgpConnectionInformationBase* BgpGetConnInfoFromUniqueId(
    BgpData* bgp,
    int uniqueId)
{
    BgpConnectionInformationBase* bgpConnInfo = NULL;
    int numEntries = 0;
    int i = 0;

    ERROR_Assert(bgp, "Invalid BGP structure\n");

    bgpConnInfo = (BgpConnectionInformationBase*)
        BUFFER_GetData(&(bgp->connInfoBase));

    // Calculate the current number of entries in the connection information
    // base
    numEntries = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
        / sizeof(BgpConnectionInformationBase);

    for (i = 0; i < numEntries; i++)
    {
        // Found the matching
        if (bgpConnInfo[i].uniqueId == uniqueId)
        {
            return &bgpConnInfo[i];
        }
    }
    return NULL;
}


//--------------------------------------------------------------------------
// NAME:        BgpGetConnInfoFromConnectionId
//
// PURPOSE:     Finding the connection information structure for a given
//              connectionId
//
// PARAMETERS:  bgp, the bgp internal structure
//              connectionId, the unique id to be searched
//
// RETURN:      void.
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
BgpConnectionInformationBase* BgpGetConnInfoFromConnectionId(
    BgpData* bgp,
    int connectionId)
{
   BgpConnectionInformationBase* bgpConnInfo = NULL;
   int numEntries = 0;
   int i;

    ERROR_Assert(bgp, "Invalid BGP structure\n");

    bgpConnInfo = (BgpConnectionInformationBase*)
       BUFFER_GetData(&(bgp->connInfoBase));

   // Calculate the current number of entries in the connection information
   // base
   numEntries = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
       / sizeof(BgpConnectionInformationBase);

   for (i = 0; i < numEntries; i++)
   {
      // Found the matching
      if (bgpConnInfo[i].connectionId == connectionId)
      {
          return &bgpConnInfo[i];
      }
   }
   return NULL;
}


//--------------------------------------------------------------------------
// FUNCTION: BgpIsLoopInUpdate
//
// PURPOSE:  Check whether there is a loop in the update message came. A loop
//           is detected if the as path contains its own asid.
//
// ARGUMENTS: bgp, bgp main structure
//            bgpUpdate, the update message came
//
// RETURN:    TRUE if a loop is detected, otherwise FALSE
//--------------------------------------------------------------------------

static
BOOL BgpIsLoopInUpdate(BgpData* bgp, BgpUpdateMessage* bgpUpdate)
{
    ERROR_Assert(bgp, "Invalid BGP structure\n");
    ERROR_Assert(bgpUpdate, "Invalid update message\n");

    if (bgpUpdate->totalPathAttributeLength)
    {
        unsigned int counter = 0;
        BgpPathAttribute* pathAttribute = (BgpPathAttribute*)
        bgpUpdate->pathAttribute;

        BgpPathAttributeValue* asPathAttrVal = (BgpPathAttributeValue*)
        pathAttribute[1].attributeValue;

        // Check the whole path attribute field to see if there is any asId
        // same as mine
        if (asPathAttrVal)
        {
            for (counter = 0;
                 counter < asPathAttrVal->pathSegmentLength; counter++)
            {
                 unsigned short asIdInPathAttr;
                 memcpy(&asIdInPathAttr, &asPathAttrVal->
                        pathSegmentValue[counter],
                        sizeof(unsigned short));

                 if (bgp->asId == asIdInPathAttr)
                 {
                    // A loop is detected in the update message so free the
                    // update message and return from the function
                    if (DEBUG)
                        printf("loop Detected\n");
                    return TRUE;
                }
            }
        }
        return FALSE;
    }
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION  BgpSetUpdateTimer
//
// PURPOSE:  To schedule next update message
//
// PARAMETERS: node, The node which is scheduling update
//             connectionStatus, Peer connection which will send the update
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpSetUpdateTimer(
    Node* node,
    BgpConnectionInformationBase* connectionStatus)
{
    // There is some more route to be advertise so schedule update
    Message* newMsg = NULL;
    clocktype delay = 0;
    clocktype now = 0;

    ERROR_Assert(!connectionStatus->updateTimerAlreadySet,
                 "BGPv4: Update timer error\n");

    newMsg = MESSAGE_Alloc(node,
                 APP_LAYER,
                 APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                 MSG_APP_BGP_RouteAdvertisementTimer);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(int));

    memcpy(MESSAGE_ReturnInfo(newMsg),
           &connectionStatus->uniqueId,
           sizeof(int));

    now = node->getNodeTime();

    if (now < connectionStatus->nextUpdate)
    {
        // Need to wait since next update interval not passed yet.
        delay = connectionStatus->nextUpdate - now;
    }
    else
    {
        delay = 0;
    }

    connectionStatus->updateTimerAlreadySet = TRUE;

    MESSAGE_Send(node, newMsg, delay);
}


//--------------------------------------------------------------------------
// FUNCTION    BgpGetPacketSize
//
// PURPOSE:    To get the size of the packet
//
// PARAMETERS: packet, the packet pointer
//
// RETURN:     None
//
// ASSUMPTION: None
//--------------------------------------------------------------------------

static
int BgpGetPacketSize(char* packet)
{
   unsigned short packetSize = 0;
   memcpy(&packetSize, packet + BGP_SIZE_OF_MARKER, BGP_MESSAGE_LENGTH_SIZE);
   return packetSize;
}


//--------------------------------------------------------------------------
// FUNCTION  BgpInitStat
//
// PURPOSE:  Initialize the statistical variables
//
// PARAMETERS:  bgp, the bgp internal structure
//
// RETURN:      void
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpInitStat(BgpData* bgp)
{
    (bgp->stats).openMsgSent = 0;

    (bgp->stats).openMsgReceived = 0;

    (bgp->stats).keepAliveSent = 0;

    (bgp->stats).keepAliveReceived = 0;

    (bgp->stats).updateSent = 0;

    (bgp->stats).reflectedupdateSent = 0;

    (bgp->stats).updateReceived = 0;

    (bgp->stats).notificationSent = 0;

    (bgp->stats).notificationReceived = 0;
}


//--------------------------------------------------------------------------
// FUNCTION BgpIsSpeaker
//
// PURPOSE  checks from config file whether the current node is a speaker
//          if it is then return the asid and routing information
//
// PARAMETERS
//      node,    the node to check
//      index,  the line number from where the information for this node
//               is starting in the input file
//      bgpConf, the inputfile stucture for bgp
//
// RETURN
//      TRUE If the node is a bgp speaker or FALSE
//
// ASSUMPTION
//      The entry in the file will be in the following format.
//        ROUTER <node id of the speaker> BGP <as id of the speaker>
//--------------------------------------------------------------------------

static
BOOL BgpIsSpeaker(
    NodeAddress nodeId,
    int* index,
    NodeInput* bgpConf)
{
    // Go through the configuration file to find an entry for
    // ROUTER <nodeId> BGP <asId> and return TRUE and assign the asId
    // if there is one entry, else return FALSE.
    // return the index through assigning in *index, of the line where
    // found the above line to reduce searching all the every time.
    // as the file need to be

    BOOL isBgpSpeaker = FALSE;
    int i = 0;

    for (i = 0; i < bgpConf->numLines; i++)
    {
        const char* currentLine = bgpConf->inputStrings[i];
        char token[MAX_INPUT_FILE_LINE_LENGTH];

        sscanf(currentLine, "%s", token);

        if (!strcmp(token, "ROUTER"))
        {
            char router[MAX_STRING_LENGTH];
            char bgpStr[MAX_STRING_LENGTH];
            NodeAddress nodeIdRead = 0;

            sscanf(currentLine, "%s %d %s", router, &nodeIdRead, bgpStr);

            if (strcmp(bgpStr,"BGP"))
            {
                ERROR_ReportError("The format ROUTER <node id> "
                    "BGP <AS id>\n");
            }

            if (nodeId == nodeIdRead)
            {
                // Line number from where the specification for this speaker
                // starts
                *index = i;
                isBgpSpeaker = TRUE;
                break;
            }
        }
    }
    return isBgpSpeaker;
}


//--------------------------------------------------------------------------
// FUNCTION   : BgpReadTime
//
// PURPOSE    : Read the different timer values for bgp if nothing is
//              specified that will be initialized with default value
//
// PARAMETERS : node, the node which is reading the parameters
//              timer, the string to be searched
//              timeStr, the read value
//              defaultVal, the default value for the timer
//              nodeInput, the input file
//
// RETURN       void
//
// ASSUMPTION : none.
//
//--------------------------------------------------------------------------

static
void BgpReadTime(Node* node,
                 clocktype* timer,
                 const char* timeStr,
                 clocktype defaultVal,
                 const NodeInput* nodeInput)
{
    BOOL retVal = 0;
    clocktype timeVal = 0;

    IO_ReadTime(node->nodeId, ANY_DEST, nodeInput, timeStr,
        &retVal, &timeVal);

    if (retVal)
    {
        *timer = timeVal;
        if (timeVal <= 0)
        {
            ERROR_ReportWarning("Invalid BGP TIMER configuration:Timer "
            "should set to a positive value.It was set to default value\n");

            *timer = defaultVal;
        }
    }
    else
    {
        *timer = defaultVal;
    }


}


//--------------------------------------------------------------------------
// FUNCTION   : BgpFillRibEntry
//
// PURPOSE    : Adding one route entry in the Routing Information Base
//
// PARAMETERS : bgpRIB, bgp routing information base
//              destAddr, destination address of the route to be inserted
//              networkPrefix, number of network bits in the destAddress
//              peerAddress, address of the peer from which the route has
//                           been learnt
//              connectionId, the connection id of the connection which has
//                            learnt the route information
//              origin,       the origin of the route (internal or external)
//              originType    origin Attribute received in Update message
//              asPathList,   list of AS the node is likely to traverse to
//                            reach the destination
//              sizeOfAsPath, number of octates in the asPath field
//              calcLocalPref, internal weight for the route for internal
//                             routes it will be automatically assigned to
//                             a weight of 32768
//              isBest,        whether this route is the best route to the
//                             destination
//              asId,          ??
//
// RETURN       void
//
// ASSUPTION    none
//--------------------------------------------------------------------------
static
void BgpFillRibEntry(
    BgpRoutingInformationBase* bgpRIB,
    Address     destAddr,
    unsigned char networkPrefix,
    Address   peerAddress,
    unsigned short asIdOfPeer,
    BOOL          isClient, //Morteza:modified
    NodeAddress   bgpIdentifier,
    unsigned int  origin,
    unsigned int  originType,
    char*         asPathList,
    unsigned char sizeOfAsPath,
    unsigned int  localPref,
    unsigned int  calcLocalPref,
    unsigned int  multiExitDisc,
    unsigned char isBest,
    Address   nextHop,
    unsigned short weight,
    NodeAddress   originatorId)
{
    ERROR_Assert(bgpRIB, "Invalid RIB structure\n");


    bgpRIB->destAddress.prefixLen = networkPrefix;

    //bgpRIB->destAddress.prefix = destAddr;
    memcpy((char*) &bgpRIB->destAddress.prefix,
       (char*) &destAddr,
       sizeof(Address));


   // bgpRIB->peerAddress = peerAddress;
    memcpy((char*) &bgpRIB->peerAddress,
       (char*) &peerAddress,
       sizeof(Address));

    bgpRIB->asIdOfPeer  = asIdOfPeer;

    bgpRIB->isClient = isClient;

    bgpRIB->bgpIdentifier = bgpIdentifier;

    //bgpRIB->nextHop = nextHop;
    memcpy((char*) &bgpRIB->nextHop,
       (char*) &nextHop,
       sizeof(Address));

    bgpRIB->origin = (unsigned char) origin;

    bgpRIB->originType = (unsigned char) originType;

    if (sizeOfAsPath)
    {
        bgpRIB->asPathList = (BgpPathAttributeValue*)
            MEM_malloc(sizeof(BgpPathAttributeValue));
        bgpRIB->asPathList->pathSegmentType = (unsigned char)
            BGP_PATH_SEGMENT_AS_SEQUENCE;

        bgpRIB->asPathList->pathSegmentLength = sizeOfAsPath;

        bgpRIB->asPathList->pathSegmentValue =
            (unsigned short*) MEM_malloc(sizeOfAsPath);

        memcpy(bgpRIB->asPathList->pathSegmentValue, asPathList,
           sizeOfAsPath);
    }
    else
    {
        bgpRIB->asPathList = NULL;
    }
    bgpRIB->multiExitDisc = multiExitDisc;

    bgpRIB->pathAttrBest = isBest;

    bgpRIB->localPref = localPref;

    bgpRIB->calcLocalPref = calcLocalPref;

    bgpRIB->pathAttrAtomicAggregate = BGP_PATH_ATTR_ATOMIC_AGGRIGATE_INVALID;

    bgpRIB->pathAttrAggregatorAS = BGP_PATH_ATTR_AGGRIGATOR_AS_INVALID;

    bgpRIB->pathAttrAggregatorAddr = BGP_PATH_AGGRIGATOR_ADDR_INVALID;

    bgpRIB->weight = weight;

    bgpRIB->isValid = TRUE;

    bgpRIB->originatorId = originatorId;
}


//--------------------------------------------------------------------------
// FUNCTION   : BgpFillPeerEntry
//
// PURPOSE    : Adding one Peer entry in the Conneciton Information Base
//
// PARAMETERS : node, the node which is adding the peer information
//              bgpCIB, bgp connection information base
//              remoteAddr, address of the peer
//              localAddr, the interface address of this node through which
//                         it will communicate with the peer
//              localPort, local port number through which the node is
//                         communicating with the peer
//              localBgpIdentifier, this is the default address of this node
//              remoteBgpIdentifier, the bgp identifier of the peer
//              asIdOfPeer, the as id the Autonomous System the peer
//              resides in
//              weight,     the local weight the node assigning to the peer
//              isInternalConnection, TRUE if the peer is internal peer else
//                          FALSE
//              isClient, TRUE if the peer is CLIENT node.
//              holdTime,   the hold time the connection will be using
//              uniqueueId, the process id the connection is using
//
// RETURN       void
//
// ASSUPTION    none
//--------------------------------------------------------------------------
static
void BgpFillPeerEntry(
    Node* node,
    BgpConnectionInformationBase* bgpCIB,
    Address remoteAddr,
    Address localAddr,
    short     localPort,
    short     remotePort,
    NodeAddress localBgpIdentifier,
    NodeAddress remoteBgpIdentifier,
    unsigned short asIdOfPeer,
    unsigned short weight,
    unsigned int multiExitDisc,
    BOOL      isInternalConnection,
    BOOL isClient,
    clocktype holdTime,
    unsigned int uniqueId,
    BgpStateType state,
    int connId,
    BOOL isInitiatingNode)
{
    ERROR_Assert(bgpCIB, "Invalid RIB structure\n");


    //bgpCIB->remoteAddr = remoteAddr;
    memcpy((char*) &bgpCIB->remoteAddr,
       (char*) &remoteAddr,
       sizeof(Address));


    bgpCIB->remotePort = remotePort;

    bgpCIB->asIdOfPeer = asIdOfPeer;


    //bgpCIB->localAddr = localAddr;
    memcpy((char*) &bgpCIB->localAddr,
       (char*) &localAddr,
       sizeof(Address));

    bgpCIB->localPort = localPort;

    bgpCIB->localBgpIdentifier = localBgpIdentifier;

    bgpCIB->remoteBgpIdentifier = remoteBgpIdentifier;

    bgpCIB->connectionId = connId;

    bgpCIB->uniqueId = uniqueId;

    bgpCIB->remoteId = MAPPING_GetNodeIdFromInterfaceAddress( node,
                                  remoteAddr);

    bgpCIB->localId = node->nodeId;

    bgpCIB->state = state;

    bgpCIB->connectRetrySeq = 0;

    bgpCIB->startEventTime = BGP_DEFAULT_START_EVENT_TIME;

    bgpCIB->keepAliveTimerSeq = 0;

    bgpCIB->holdTimerSeq = 0;

    bgpCIB->holdTime = holdTime;

    bgpCIB->nextUpdate = 0;

    bgpCIB->isInternalConnection = isInternalConnection;

    bgpCIB->isInitiatingNode = isInitiatingNode;

    bgpCIB->isClient = isClient;

    bgpCIB->weight = weight;

    bgpCIB->multiExitDisc = multiExitDisc;

    bgpCIB->updateTimerAlreadySet = FALSE;

    // Initialize Message Buffer
    bgpCIB->buffer.data = NULL;
    bgpCIB->buffer.currentSize = 0;
    bgpCIB->buffer.expectedSize = 0;
    // Allocate space for adjRibOut
    BUFFER_InitializeDataBuffer(&(bgpCIB->adjRibOut),
            (sizeof(BgpAdjRibLocStruct*) * BGP_MIN_RT_BLOCK_SIZE));
    // Allocate space for withdrawn routes
    BUFFER_InitializeDataBuffer(&(bgpCIB->withdrawnRoutes),
               sizeof(BgpRouteInfo) * BGP_MIN_RT_BLOCK_SIZE);

}

//--------------------------------------------------------------------------
// FUNCTION   : BgpUpdatePeerEntry
//
// PURPOSE    : Adding one Peer entry in the Conneciton Information Base
//
// PARAMETERS : node, the node which is adding the peer information
//              bgpCIB, bgp connection information base
//              remoteAddr, address of the peer
//              localAddr, the interface address of this node through which
//                         it will communicate with the peer
//              localPort, local port number through which the node is
//                         communicating with the peer
//              localBgpIdentifier, this is the default address of this node
//              remoteBgpIdentifier, the bgp identifier of the peer
//              asIdOfPeer, the as id the Autonomous System the peer
//              resides in
//              weight,     the local weight the node assigning to the peer
//              isInternalConnection, TRUE if the peer is internal peer else
//                          FALSE
//              isClient, TRUE if the peer is CLIENT node.
//              holdTime,   the hold time the connection will be using
//              uniqueueId, the process id the connection is using
//
// RETURN       void
//
// ASSUPTION    none
//--------------------------------------------------------------------------
static
void BgpUpdatePeerEntry(
    Node* node,
    BgpConnectionInformationBase* bgpCIB,
    Address remoteAddr,
    Address localAddr,
    short     localPort,
    short     remotePort,
    NodeAddress localBgpIdentifier,
    NodeAddress remoteBgpIdentifier,
    unsigned short asIdOfPeer,
    unsigned short weight,
    BOOL      isInternalConnection,
    BOOL isClient,
    clocktype holdTime,
    unsigned int uniqueId,
    BgpStateType state,
    int connId,
    BOOL isInitiatingNode)
{
    ERROR_Assert(bgpCIB, "Invalid RIB structure\n");


    memcpy((char*) &bgpCIB->remoteAddr,
       (char*) &remoteAddr,
       sizeof(Address));


    bgpCIB->remotePort = remotePort;

    bgpCIB->asIdOfPeer = asIdOfPeer;


    memcpy((char*) &bgpCIB->localAddr,
       (char*) &localAddr,
       sizeof(Address));

    bgpCIB->localPort = localPort;

    bgpCIB->localBgpIdentifier = localBgpIdentifier;

    bgpCIB->remoteBgpIdentifier = remoteBgpIdentifier;

    bgpCIB->connectionId = connId;

    bgpCIB->uniqueId = uniqueId;

    bgpCIB->remoteId = MAPPING_GetNodeIdFromInterfaceAddress( node,
                                  remoteAddr);

    bgpCIB->localId = node->nodeId;

    bgpCIB->state = state;

    bgpCIB->connectRetrySeq = 0;

    bgpCIB->startEventTime = BGP_DEFAULT_START_EVENT_TIME;

    bgpCIB->keepAliveTimerSeq = 0;

    bgpCIB->holdTimerSeq = 0;

    bgpCIB->holdTime = holdTime;

    bgpCIB->nextUpdate = 0;

    bgpCIB->isInternalConnection = isInternalConnection;

    bgpCIB->isInitiatingNode = isInitiatingNode;

    bgpCIB->isClient = isClient;

    bgpCIB->weight = weight;

    bgpCIB->updateTimerAlreadySet = FALSE;

    // Initialize Message Buffer
    bgpCIB->buffer.data = NULL;
    bgpCIB->buffer.currentSize = 0;
    bgpCIB->buffer.expectedSize = 0;
}
//--------------------------------------------------------------------------
// FUNCTION : BgpUpdateRibFromIpv4ForwardingTable()
//
// PURPOSE  : Update Routing Information Base from IPv4 Network's forwarding
//            Table.
//
// PARAMETERS : node, The node which is updating RIB
//              bgp,  Bgp internal structure
//              isStatic, to know whether we are interested in static route
//                    entries of the forwarding table or interested in the
//                    dynamic routes ie. entered by any routing protocol
// RETURN :     None
//
// ASSUMPTION : None
//--------------------------------------------------------------------------

static
void BgpUpdateRibFromIpv4ForwardingTable(
    Node* node,
    BgpData* bgp,
    BOOL isStatic)
{
    int i = 0;
    int j = 0;
    int row_iterator = 0;

    if (DEBUG )
        printf("node %u updating RIB from IPv4 forwarding table\n",
                node->nodeId);

    NetworkDataIp* ip = node->networkData.networkVar;
    NetworkForwardingTable* rt = &(ip->forwardTable);
    NetworkForwardingTable* rt2 = &(ip->forwardTable);


    int numEntriesAdjRibIn = BUFFER_GetCurrentSize(&(bgp->adjRibIn))
        / sizeof(BgpRoutingInformationBase);

    BgpRoutingInformationBase* adjRibIn = (BgpRoutingInformationBase*)
                                 BUFFER_GetData(&(bgp->adjRibIn));

    char* prevStartLocation = (char*) adjRibIn;
    char* nextStartLocation = NULL;


    for (i = 0; i < rt->size; i++)
    {
        // If the boolean isStatic is true find the valid entries of the
        // forwarding table with routing protocol static and if the boolean
        // is false then take the entries from the forwarding table whose
        // routing protocol is not static or not entered by bgp and are valid
        // meaning the destinations are reachable.
        if (((isStatic) ? (rt->row[i].protocolType == ROUTING_PROTOCOL_STATIC
            || rt->row[i].protocolType == ROUTING_PROTOCOL_DEFAULT)
            : ((rt->row[i].protocolType != EXTERIOR_GATEWAY_PROTOCOL_EBGPv4)
             && (rt->row[i].protocolType != EXTERIOR_GATEWAY_PROTOCOL_IBGPv4)
             && (rt->row[i].protocolType !=
                EXTERIOR_GATEWAY_PROTOCOL_BGPv4_LOCAL)))
             && ((rt->row[i].nextHopAddress != (unsigned) NETWORK_UNREACHABLE)
             && (rt->row[i].destAddress != 0)))
        {
            BOOL isFound = FALSE;
            BOOL bgp_entry_found = FALSE;

            if (!bgp->bgpRedistributeStaticAndDefaultRts)
            {
                if ((rt->row[i].protocolType == ROUTING_PROTOCOL_STATIC) ||
                   (rt->row[i].protocolType == ROUTING_PROTOCOL_DEFAULT))
                {
                    continue;
                }
            }
            // If on any BGP/OSPF ASBR, the redistributeFromOspf flag is not
            // set, then do not advertise the external routes.
            if (!bgp->advertiseOspfExternalType1 &&
                (rt->row[i].protocolType == ROUTING_PROTOCOL_OSPFv2_TYPE1_EXTERNAL))
            {
                continue;
            }
            if (!bgp->advertiseOspfExternalType2 &&
                (rt->row[i].protocolType == ROUTING_PROTOCOL_OSPFv2_TYPE2_EXTERNAL))
            {
                continue;
            }
            /* If this node is a RR, and this route is known via BGP too,
            then dont advertise this as it would anyway be advertised through
            BGP. */
            if (bgp->isRtReflector)
            {
                for (row_iterator = 0; row_iterator < rt->size;
                     row_iterator++)
                {
                    if ((rt2->row[row_iterator].destAddress ==
                            rt->row[i].destAddress) &&
                        (rt2->row[row_iterator].destAddressMask ==
                            rt->row[i].destAddressMask) &&
                        (rt2->row[row_iterator].nextHopAddress ==
                            rt->row[i].nextHopAddress) &&
                        ((rt2->row[row_iterator].protocolType ==
                          EXTERIOR_GATEWAY_PROTOCOL_EBGPv4) ||
                         (rt2->row[row_iterator].protocolType ==
                          EXTERIOR_GATEWAY_PROTOCOL_IBGPv4) ||
                         (rt2->row[row_iterator].protocolType ==
                          EXTERIOR_GATEWAY_PROTOCOL_BGPv4_LOCAL)))
                    {
                        bgp_entry_found = TRUE;
                        break;
                    }
                }
                if (bgp_entry_found)
                {
                    continue;
                }
            }
            for (j = 0; j < numEntriesAdjRibIn; j++ ) // search AdjRibin
            {

                BOOL cond2 = ( (rt->row[i].destAddress ==
                     GetIPv4Address(adjRibIn[j].destAddress.prefix))
                     && (ConvertSubnetMaskToNumHostBits(rt->row[i].
                                  destAddressMask) ==
                                  (32 - adjRibIn[j].destAddress.prefixLen)));

                if (cond2)
                {
                    if (adjRibIn[j].origin == BGP_ORIGIN_IGP)
                    {
                        // Already a route exists from Igp with this
                        // destination so we need to compare the routes and
                        // don't need to add entry for this route
                        isFound = TRUE;
                        SetIPv4AddressInfo(&adjRibIn[j].nextHop,
                                           rt->row[i].nextHopAddress);

                        adjRibIn[j].isValid = TRUE;
                        adjRibIn[j].pathAttrBest = BGP_PATH_ATTR_BEST_TRUE;
                    }
                    else
                    {
                        adjRibIn[j].pathAttrBest = BGP_PATH_ATTR_BEST_FALSE;
                    }
                }
            } // end of search in adjRibIn

            if (!isFound)
            {
                // The forwarding table route entry is not there in the
                // Routing Information Base of Bgp so add the new route
                // in the RIB as these routes are learned by IGP and the
                // as is the as of its own
                BgpRoutingInformationBase bgpRoutingInformationBase;
                Address nextHop;
                int numHostBits = ConvertSubnetMaskToNumHostBits(rt->row[i].
                                 destAddressMask);

                if (DEBUG)
                {
                     char ipAddress[MAX_STRING_LENGTH];

                     IO_ConvertIpAddressToString(rt->row[i].destAddress,
                         ipAddress);

                     printf("Adding destination = %s\n", ipAddress);
                }

                if (rt->row[i].nextHopAddress != 0)
                {
                    SetIPv4AddressInfo(&nextHop, rt->row[i].nextHopAddress);
                }
                else
                {
                    SetIPv4AddressInfo(&nextHop,
                                       NetworkIpGetInterfaceAddress(node,
                                       rt->row[i].interfaceIndex));
                }
                Address destAddr;
                SetIPv4AddressInfo(&destAddr, rt->row[i].destAddress);

                Address bgpInvalidAddr;
                memset(&bgpInvalidAddr,0,sizeof(Address));

                BgpFillRibEntry(
                    &bgpRoutingInformationBase,
                    destAddr,
                    (unsigned char)(32 - numHostBits),
                    bgpInvalidAddr,     //BGP_INVALID_ADDRESS,
                    bgp->asId,
                    TRUE, //Morteza;isClient
                    node->nodeId,
                    BGP_ORIGIN_IGP,
                    BGP_ORIGIN_IGP,
                    NULL,
                    (unsigned char) 0,
                    bgp->localPref,
                    bgp->localPref,     // not used now
                    bgp->multiExitDisc,
                    BGP_PATH_ATTR_BEST_TRUE,
                    nextHop,
                    BGP_DEFAULT_INTERNAL_WEIGHT,
                    0);

                BUFFER_AddDataToDataBuffer(
                    &(bgp->adjRibIn),
                    (char*) &bgpRoutingInformationBase,
                    sizeof(BgpRoutingInformationBase));

                adjRibIn = (BgpRoutingInformationBase*)
                    BUFFER_GetData(&(bgp->adjRibIn));
                numEntriesAdjRibIn++;
            }
        }
    } // end for (i = 0; i < rt->size; i++)

    nextStartLocation = BUFFER_GetData(&(bgp->adjRibIn));

    if (prevStartLocation != nextStartLocation)
    {
        int i;
        IntPtr numByteShift = nextStartLocation - prevStartLocation;

        // Shift all the addresses of AdjRibLocal
        int numEntriesInRibLoc = BUFFER_GetCurrentSize(&(bgp->ribLocal))
            / sizeof(BgpAdjRibLocStruct);

        BgpAdjRibLocStruct* ribLocPtr = (BgpAdjRibLocStruct*)
        BUFFER_GetData(&bgp->ribLocal);

        for (i = 0; i < numEntriesInRibLoc; i++)
        {
            char* prevPtr = (char*) ribLocPtr[i].ptrAdjRibIn;
            ribLocPtr[i].ptrAdjRibIn = (BgpRoutingInformationBase*)
        (prevPtr + numByteShift);
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION :  BgpUpdateRibWithIpv6RouteEntry()
//
// PURPOSE  : Updates RIB with the fileds of visited entry in IPv6 Network's
//            forwarding Table.
//
// PARAMETERS :rrleaf, points to the visted entry in IPv6 forwarding table
//             node, Node structure
//             isStatic, to know whether we are interested in static route
//                    entries of the forwarding table or interested in the
//                    dynamic routes ie. entered by any routing protocol
// RETURN :     None
//
// ASSUMPTION : None
//--------------------------------------------------------------------------
static
void  BgpUpdateRibWithIpv6RouteEntry(rn_leaf* rleaf,
                                     Node* node,
                                     BOOL isStatic)
{
    int j = 0;
    BgpData* bgp = (BgpData*) node->appData.exteriorGatewayVar;
    int numEntriesAdjRibIn = BUFFER_GetCurrentSize(&(bgp->adjRibIn))
        / sizeof(BgpRoutingInformationBase);

    BgpRoutingInformationBase* adjRibIn = (BgpRoutingInformationBase*)
    BUFFER_GetData(&(bgp->adjRibIn));

    in6_addr k;
    unsigned char rn_flags;
    int rn_option;
    int flags;
    int type;
    int index;
    int metric;
    in6_addr nextHopAddress;
    in6_addr destAddress;
    in6_addr gtway;
    unsigned int destPrefixLen;

    rn_leaf* tempLeaf = (rn_leaf*)rleaf;

    k = tempLeaf->key;
    flags = tempLeaf->flags;
    index = tempLeaf->interfaceIndex;
    type = tempLeaf->type;
    gtway =tempLeaf->gateway;
    metric = tempLeaf->metric;
    destPrefixLen =  tempLeaf->keyPrefixLen;
    destPrefixLen =  tempLeaf->keyPrefixLen;
    destPrefixLen =  tempLeaf->keyPrefixLen;
    destPrefixLen =  tempLeaf->keyPrefixLen;
    rn_flags = tempLeaf->rn_flags;
    rn_option = tempLeaf->rn_option;

    if (flags == RT_LOCAL)
      memset(&nextHopAddress,0,sizeof(in6_addr));
    else
      memcpy(&nextHopAddress,(char*)&gtway, sizeof(in6_addr));

    memcpy(&destAddress,(char*)&k, sizeof(in6_addr));



    in6_addr InvalidAddr6;
    memset(&InvalidAddr6,0,sizeof(in6_addr));
    if ((isStatic) ? ((rn_flags != RNF_IGNORE) && ((flags == RT_LOCAL)
               || ((flags == RT_REMOTE) && (rn_option == RTF_STATIC))))
     : ((type != EXTERIOR_GATEWAY_PROTOCOL_EBGPv4) &&
        (type != EXTERIOR_GATEWAY_PROTOCOL_IBGPv4) &&
        (type != EXTERIOR_GATEWAY_PROTOCOL_BGPv4_LOCAL))
    && ((metric != (int) NETWORK_UNREACHABLE) &&
        (CMP_ADDR6(destAddress,InvalidAddr6) != 0)))
    {
        BOOL isFound = FALSE;

        for (j = 0; j < numEntriesAdjRibIn; j++ ) // search AdjRibin
        {
            BOOL cond1 = (( CMP_ADDR6(destAddress,
                            GetIPv6Address(adjRibIn[j].destAddress.prefix))
                            == 0 ) &&
                       (destPrefixLen == adjRibIn[j].destAddress.prefixLen));
            in6_addr netPrefix =
                GetIPv6Address(adjRibIn[j].destAddress.prefix);
            unsigned int prefixLen =adjRibIn[j].destAddress.prefixLen ;
            BOOL cond2 =  Ipv6IsAddressInNetwork(&destAddress,
                                                 &netPrefix,
                                                 prefixLen);
            if (cond1 || cond2)
            {
                if (adjRibIn[j].origin == BGP_ORIGIN_IGP)
                {
                    // Already a route exists from Igp with this
                    // destination so we need to compare the routes and
                    // don't need to add entry for this route
                    isFound = TRUE;
                    SetIPv6AddressInfo(&adjRibIn[j].nextHop, nextHopAddress);
                    adjRibIn[j].isValid = TRUE;
                    adjRibIn[j].pathAttrBest = BGP_PATH_ATTR_BEST_TRUE;
                }
                else
                {
                    adjRibIn[j].pathAttrBest = BGP_PATH_ATTR_BEST_FALSE;
                }
            }
        } // end of search in adjRibIn

        if (!isFound)
        {
            // The forwarding table route entry is not there in the
            // Routing Information Base of Bgp so add the new route
            // in the RIB as these routes are learned by IGP and the
            // as is the as of its own
            BgpRoutingInformationBase bgpRoutingInformationBase;
            Address nextHop;
            if (DEBUG)
            {
                char ipAddress[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(&destAddress,ipAddress);

                printf("Adding destination = %s\n", ipAddress);
            }

            if (CMP_ADDR6(nextHopAddress,InvalidAddr6) != 0 )
            {
                SetIPv6AddressInfo(&nextHop, nextHopAddress);
            }
            else
            {
                in6_addr ifxAddr =Ipv6GetPrefixFromInterfaceIndex(node,
                                                                  index);
                SetIPv6AddressInfo(&nextHop,ifxAddr);
            }
            Address destAddr;
            SetIPv6AddressInfo(&destAddr, destAddress);

            Address bgpInvalidAddr;
            memset(&bgpInvalidAddr,0,sizeof(Address));

            BgpFillRibEntry(
                &bgpRoutingInformationBase,
                destAddr,
                (unsigned char) destPrefixLen,
                bgpInvalidAddr,     //BGP_INVALID_ADDRESS,
                bgp->asId,
                TRUE, //Morteza:isClient
                node->nodeId,
                BGP_ORIGIN_IGP,
                BGP_ORIGIN_IGP,
                NULL,
                (unsigned char) 0,
                bgp->localPref,
                bgp->localPref,     // not used now
                bgp->multiExitDisc,
                BGP_PATH_ATTR_BEST_TRUE,
                nextHop,//next hop only includes prefix address,
                BGP_DEFAULT_INTERNAL_WEIGHT,
                0);

            BUFFER_AddDataToDataBuffer(
                &(bgp->adjRibIn),
                (char*) &bgpRoutingInformationBase,
                sizeof(BgpRoutingInformationBase));

            adjRibIn = (BgpRoutingInformationBase*)
            BUFFER_GetData(&(bgp->adjRibIn));
            numEntriesAdjRibIn++;
        }//if (!isFound)
    }

}

//--------------------------------------------------------------------------
// FUNCTION :  BgpScanIpv6Table()
//
// PURPOSE  : Visits all the enteries in IPv6 Network's forwarding
//            Table.
//
// PARAMETERS :radixNode, The node in the tree structure of IPv6 forwarding table
//                        from which visting(tour) is started
//              node, Node structure
//              isStatic, to know whether we are interested in static route
//                    entries of the forwarding table or interested in the
//                    dynamic routes ie. entered by any routing protocol
// RETURN :     None
//
// ASSUMPTION : None
//--------------------------------------------------------------------------
static
void  BgpScanIpv6Table( radix_node* radixNode,Node* node, BOOL isStatic)
{

    if (radixNode != NULL)
    {
        //Visit Left.
        if (radixNode->childlink & 0x01)
        {
            BgpScanIpv6Table(
                (radix_node*)radixNode->nextNode[0],
                node,
                isStatic);
        }

        // Print the Information.
        if (!(radixNode->childlink & 0x01) && radixNode->nextNode[0])
        {
            if (((rn_leaf *)radixNode->nextNode[0])->rn_flags != RNF_IGNORE)
            {
                BgpUpdateRibWithIpv6RouteEntry(
                           (rn_leaf *)radixNode->nextNode[0],
                           node,
                           isStatic);


            }
        }

        if (!(radixNode->childlink & 0x02) && radixNode->nextNode[1])
        {
            if (((rn_leaf *)radixNode->nextNode[1])->rn_flags != RNF_IGNORE)
            {
                BgpUpdateRibWithIpv6RouteEntry(
                          (rn_leaf *)radixNode->nextNode[1],
                           node,
                           isStatic);
            }
        }
        //Visit Right.
        if (radixNode->childlink & 0x02)
        {
            BgpScanIpv6Table((radix_node*)radixNode->nextNode[1],
                             node,
                             isStatic);
        }
    }//if (radixNode != NULL)

}

//--------------------------------------------------------------------------
// FUNCTION : BgpUpdateRibFromIpv6ForwardingTable()
//
// PURPOSE  : Update Routing Information Base from IPv6 Network's forwarding
//            Table.
//
// PARAMETERS : node, The node which is updating RIB
//              bgp,  Bgp internal structure
//              isStatic, to know whether we are interested in static route
//                    entries of the forwarding table or interested in the
//                    dynamic routes ie. entered by any routing protocol
// RETURN :     None
//
// ASSUMPTION : None
//--------------------------------------------------------------------------

static
void BgpUpdateRibFromIpv6ForwardingTable(
    Node* node,
    BgpData* bgp,
    BOOL isStatic)
{

    if (DEBUG )
        printf("node %u updating RIB from IPv6 forwarding table\n",
                node->nodeId);

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    IPv6Data* ipv6 = ip->ipv6;

    BgpRoutingInformationBase* adjRibIn = (BgpRoutingInformationBase*)
    BUFFER_GetData(&(bgp->adjRibIn));

    char* prevStartLocation = (char*) adjRibIn;
    char* nextStartLocation = NULL;
    if (DEBUG_FORWARD_TABLE)
    {
        Ipv6PrintForwardingTable(node);
    }
    //I need the fields of IP forwarding table
    BgpScanIpv6Table(ipv6->rt_tables->root,node,isStatic);

    nextStartLocation = BUFFER_GetData(&(bgp->adjRibIn));

    if (prevStartLocation != nextStartLocation)
    {
        int i;
        IntPtr numByteShift = nextStartLocation - prevStartLocation;

        // Shift all the addresses of AdjRibLocal
        int numEntriesInRibLoc = BUFFER_GetCurrentSize(&(bgp->ribLocal))
            / sizeof(BgpAdjRibLocStruct);

        BgpAdjRibLocStruct* ribLocPtr = (BgpAdjRibLocStruct*)
        BUFFER_GetData(&bgp->ribLocal);

        for (i = 0; i < numEntriesInRibLoc; i++)
        {
            char* prevPtr = (char*) ribLocPtr[i].ptrAdjRibIn;
            ribLocPtr[i].ptrAdjRibIn = (BgpRoutingInformationBase*)
            (prevPtr + numByteShift);
        }
    }
}
//--------------------------------------------------------------------------
// FUNCTION : BgpDeleteNonExistingIpv4IgpRoutes()
//
// PURPOSE  : Delete non-existing IGP routing information from AdjRibIn.
//
// PARAMETERS : node, The node which id Updating Routing Information Base
//              bgp,  The bgp internal structure
//
// RETURN :     None
//
// ASSUMPTION : None
//--------------------------------------------------------------------------

static
void BgpDeleteNonExistingIpv4IgpRoutes(Node* node, BgpData* bgp)
{
    int i = 0;
    int j = 0;
    NetworkDataIp* ip = node->networkData.networkVar;
    NetworkForwardingTable* rt = &(ip->forwardTable);

    int numEntriesAdjRibIn = BUFFER_GetCurrentSize(&bgp->adjRibIn)
        / sizeof(BgpRoutingInformationBase);

    BgpRoutingInformationBase* adjRibIn = (BgpRoutingInformationBase*)
    BUFFER_GetData(&bgp->adjRibIn);

    for (j = 0; j < numEntriesAdjRibIn; j++)
    {
        BOOL isFound = FALSE;
        if ((!adjRibIn[j].isValid) || (adjRibIn[j].origin != BGP_ORIGIN_IGP))
        {
            continue;
        }
        for (i = 0; i < rt->size; i++) // search in network forwarding table
        {
            if ((rt->row[i].protocolType == EXTERIOR_GATEWAY_PROTOCOL_EBGPv4)
                || (rt->row[i].protocolType ==
                    EXTERIOR_GATEWAY_PROTOCOL_IBGPv4)
                || (rt->row[i].protocolType ==
                    EXTERIOR_GATEWAY_PROTOCOL_BGPv4_LOCAL))
            {
                continue;
            }

            if ((rt->row[i].destAddress ==
                GetIPv4Address(adjRibIn[j].destAddress.prefix) )
               && (ConvertSubnetMaskToNumHostBits(rt->row[i].destAddressMask)
                    == (32 - adjRibIn[j].destAddress.prefixLen))
               && (rt->row[i].nextHopAddress !=
                    (unsigned) NETWORK_UNREACHABLE))
            {
                isFound = TRUE;
                break;
            }
        } // end search in network forwarding table

        // route is not forwarding table means, that IGP route
        // has been invalidated. delete it from AdjRibIn

        if (!isFound)
        {
            adjRibIn[j].isValid = FALSE;
            adjRibIn[j].pathAttrBest = BGP_PATH_ATTR_BEST_FALSE;
        }
    }
}

//--------------------------------------------------------------------------
// FUNCTION :  BgpFindIpv6RouteEntry()
//
// PURPOSE  : Finds the entry in IPv6 Network's  forwarding Table
//            which has a match with the input routing info.
//
// PARAMETERS :rrleaf, points to the visted entry in IPv6 forwarding table
//             node, Node structure
//             isfound, is set to true if match is found
// RETURN :     None
//
// ASSUMPTION : None
//--------------------------------------------------------------------------
static
void  BgpFindIpv6RouteEntry(rn_leaf * rleaf,BgpRouteInfo* adjrt,BOOL* isFound )
{

    in6_addr k;
    int flags;
    int type;
    int index;
    int metric;
    in6_addr nextHopAddress;
    in6_addr destAddress;
    in6_addr gtway;
    rn_leaf* tempLeaf = (rn_leaf*)rleaf;

    k = tempLeaf->key;
    flags = tempLeaf->flags;
    index = tempLeaf->interfaceIndex;
    type = tempLeaf->type;
    gtway =tempLeaf->gateway;
    metric = tempLeaf->metric;

    if (flags == RT_LOCAL)
      memset(&nextHopAddress,0,sizeof(in6_addr));
    else
      memcpy(&nextHopAddress,(char*)&gtway, sizeof(in6_addr));

    memcpy(&destAddress,(char*)&k, sizeof(in6_addr));


    in6_addr netUnreachAddr;
    memset(&netUnreachAddr,0,sizeof(in6_addr));//bgpInvalidAddr = 0;

    if ((type == EXTERIOR_GATEWAY_PROTOCOL_EBGPv4)
        || (type ==
            EXTERIOR_GATEWAY_PROTOCOL_IBGPv4)
        || (type ==
            EXTERIOR_GATEWAY_PROTOCOL_BGPv4_LOCAL))
    {
        return;
    }

    if ((CMP_ADDR6(destAddress, GetIPv6Address( adjrt->prefix)) == 0 )
    && (CMP_ADDR6(nextHopAddress, netUnreachAddr) != 0 ))
    {
        *isFound = TRUE;
        return;
    }
}

//--------------------------------------------------------------------------
// FUNCTION : BgpTraverseIpv6RouteEntries()
//
// PURPOSE  : Traverses the enteries tree structure of IPv6 Network's forwarding
//            Table.
//
// PARAMETERS :radixNode, The node in the tree structure of IPv6 forwarding table
//                        from which visting(tour) is started
//              node, Node structure
//              isStatic, to know whether we are interested in static route
//                    entries of the forwarding table or interested in the
//                    dynamic routes ie. entered by any routing protocol
// RETURN :     None
//
// ASSUMPTION : None
//--------------------------------------------------------------------------
static
void BgpTraverseIpv6RouteEntries(radix_node* radixNode,
                                 BgpRouteInfo* adjrt,
                                 BOOL* isFound)
{
    if (*isFound == TRUE)
        return;

    if (radixNode != NULL)
    {
        //Visit Left.
        if (radixNode->childlink & 0x01)
        {

            BgpTraverseIpv6RouteEntries((radix_node*)radixNode->nextNode[0],
                        adjrt,
                        isFound);
        }


        if (*isFound == TRUE)
            return;

        // Print the Information.
        if (!(radixNode->childlink & 0x01) && radixNode->nextNode[0])
        {
            if (((rn_leaf *)radixNode->nextNode[0])->rn_flags != RNF_IGNORE)
            {
                BgpFindIpv6RouteEntry((rn_leaf *)radixNode->nextNode[0],
                      adjrt,
                      isFound);
            }
        }

        if (*isFound == TRUE)
            return;

        if (!(radixNode->childlink & 0x02) && radixNode->nextNode[1])
        {
            if (((rn_leaf *)radixNode->nextNode[1])->rn_flags != RNF_IGNORE)
            {
                BgpFindIpv6RouteEntry((rn_leaf *)radixNode->nextNode[1],
                      adjrt,
                      isFound);
            }
        }

        if (*isFound == TRUE)
            return;

        //Visit Right.
        if (radixNode->childlink & 0x02)
        {
            BgpTraverseIpv6RouteEntries((radix_node*)radixNode->nextNode[1],
                    adjrt,
                    isFound);

        }
    }//if (radixNode != NULL)
}

//--------------------------------------------------------------------------
// FUNCTION : BgpDeleteNonExistingIpv6IgpRoutes()
//
// PURPOSE  : Delete non-existing IGP routing information from AdjRibIn.
//
// PARAMETERS : node, The node which id Updating Routing Information Base
//              bgp,  The bgp internal structure
//
// RETURN :     None
//
// ASSUMPTION : None
//--------------------------------------------------------------------------

static
void BgpDeleteNonExistingIpv6IgpRoutes(Node* node, BgpData* bgp)
{
    int j = 0;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    IPv6Data* ipv6 = ip->ipv6;
    radix_node* radixNode = ipv6->rt_tables->root;

    int numEntriesAdjRibIn = BUFFER_GetCurrentSize(&bgp->adjRibIn)
      / sizeof(BgpRoutingInformationBase);

    BgpRoutingInformationBase* adjRibIn = (BgpRoutingInformationBase*)
      BUFFER_GetData(&bgp->adjRibIn);

    for (j = 0; j < numEntriesAdjRibIn; j++)
    {
        BOOL isFound = FALSE;
        if ((!adjRibIn[j].isValid) || (adjRibIn[j].origin != BGP_ORIGIN_IGP))
        {
            continue;
        }
        BgpTraverseIpv6RouteEntries(radixNode,
                    &adjRibIn[j].destAddress,
                    &isFound);//end search in network forwarding table
        // route is not forwarding table means, that IGP route
        // has been invalidated. delete it from AdjRibIn
        if (!isFound)
        {
            adjRibIn[j].isValid = FALSE;
            adjRibIn[j].pathAttrBest = BGP_PATH_ATTR_BEST_FALSE;
        }
    }

}

//-------------------------------------------------------------------------
// FUNCTION   : BgpReadConfigurationFile
//
// PURPOSE    : Read the configuration file for bgp for connections and
//              initial internal routes to be advertised to the peer
//
// PARAMETERS : node,          the node which is reading the parameters
//              bgpConfigFile, the input file for bgp
//              bgp,           bgpInternal structure to store the
//                             informations
//              index,         the input file line no from which the entry
//                             for this node is starting
//
// RETURN       void
//
// ASSUMPTION   The configuration file will be in the following format
//              ROUTER <node id of the speaker> BGP <as id of the speaker>
//              NETWORK <network number of the route
//              to advertise> eg. NETWORK N2-1.0
//                   (multiple entries for more than one route)
//              NEIGHBOR< address of peer> REMOTE-AS <as id of the peer>
//                 eg. NEIGHBOR 3.2 REMOTE-AS 3
//                   (multiple entries for more than one peer)
//              NEIGHBOR <peer address> WEIGHT <internal weight to be
//                 assigned with the peer>
//                 eg. NEIGHBOR 3.2 WEIGHT 65000
//                   (multiple entries if user want to assign weights to
//                   more than one peer. by default the weight will be
//                   assigned to 32768, weight can have a maximum value of
//                   65535, higher weight means more preferrable)
//
//-------------------------------------------------------------------------

static
void BgpReadConfigurationFile(
    Node* node,
    const NodeInput* bgpConfigFile,
    BgpData* bgp,
    int index)
{
    int i = index;
    unsigned int nodeId = 0;
    unsigned int localAsId = 0;
    char router[MAX_INPUT_FILE_LINE_LENGTH];
    char bgpStr[MAX_INPUT_FILE_LINE_LENGTH];

    NetworkType bgpType;

    // Read the asId for this node
    sscanf(bgpConfigFile->inputStrings[i], "%s %d %s %d", router, &nodeId,
        bgpStr, &localAsId);

    ERROR_Assert(bgp, "Invalid BGP structure\n");
    ERROR_Assert(nodeId == node->nodeId, "There is something unusual!\n");

    if (localAsId > (unsigned) 65535 || localAsId <= 0)
    {
        ERROR_ReportError("Autonomous System id must be less than "
            "equal to 65535 and greater than 0");
    }

    bgp->asId = (unsigned short) localAsId;

    // go to the next line of the input file to read other paraters
    i++;
    BOOL isBgpRouterId = false;
    // index is the starting line for the inputs of this node. so read the
    // inputs until input for another node comes up
    for (; i < bgpConfigFile->numLines; i++)
    {
        const char* currentLine = bgpConfigFile->inputStrings[i];
        char token[MAX_INPUT_FILE_LINE_LENGTH];



        // Found configuration of another node so break as reading
        // parameters for this node is complete
        sscanf(currentLine, "%s", token);

        if (!strcmp(token, "ROUTER"))
        {
            break;
        }
        // Read BGP ROUTER-ID <bgp id>
        if (!strcmp(token, "BGP"))
        {
            char routeIdString[MAX_INPUT_FILE_LINE_LENGTH];
            char keyWord[MAX_INPUT_FILE_LINE_LENGTH];
            NodeAddress configRouterId;

            NetworkType bgpIdType;

            sscanf(currentLine, "%*s %s ", keyWord);
            ERROR_Assert(!strcmp(keyWord, "ROUTER-ID"),
                "Bad keyword after BGP");
            if (!strcmp(keyWord, "ROUTER-ID"))
            {
                int PrefixLenth;
                sscanf(currentLine, "%*s %*s %s", routeIdString);
                bgpIdType = MAPPING_GetNetworkType(routeIdString);
                ERROR_Assert((bgpIdType == NETWORK_IPV4),
                      "Bad Type of Address for ROUTE-ID Advertised");
                BOOL isNodeId;
                IO_ParseNodeIdHostOrNetworkAddress(routeIdString,
                           &configRouterId,
                           &PrefixLenth,
                           &isNodeId);
                ERROR_Assert((!NetworkIpIsMulticastAddress(node, configRouterId)
                    && configRouterId != ANY_DEST),
                    "Invalid BGP ROUTER-ID");

                bgp->routerId = configRouterId;
                isBgpRouterId = true;
            }
        }

        if (!isBgpRouterId)
        {

            // BGP uses router ID to identify BGP-speaking peers.
            // The BGP router ID is 32-bit value.
            // If user has not specified the router-id, the default ipv4
            // address shall be treated as the router-id.
            // For a pure ipv6 node,the user must manually configure
            // BGP router ID for the router.
            // However, in existing BGP scenarios, where we have pure
            // IPV6 nodes, ROUTER-ID is not configured. Hence, for
            // maintaining backward compatibility, if ROUTER-ID is not
            // present, the nodeId shall be treated as routerId.
            bgp->routerId = GetDefaultIPv4InterfaceAddress(node);
            if (bgp->routerId == ANY_ADDRESS)
            {
                bgp->routerId = node->nodeId;
            }
            isBgpRouterId = true;
        }

        // Read NETWORK <network number> if there is any modify the RIB
        // with this information of the route.
        if (!strcmp(token, "NETWORK") )
        {

            BgpRoutingInformationBase bgpRoutingInformationBase;
                char networkAddressString[MAX_INPUT_FILE_LINE_LENGTH];
               //Morteza:must be changed to address
            Address nextHopAddress;
            nextHopAddress.networkType = NETWORK_IPV4;

            Address outputNodeAddress;
            memset(&outputNodeAddress, 0, sizeof(Address));

            int numHostBits = 0;
            int ip_size = 0;
            BOOL valid = TRUE;

            int interfaceIndex; //not used

            // read the network address string of the route to be a
            // advertised
            sscanf(currentLine, "%*s %s", networkAddressString);

            bgpType = MAPPING_GetNetworkType( networkAddressString );
            ERROR_Assert((bgpType == NETWORK_IPV4)||(bgpType == NETWORK_IPV6),
                  "Bad Type of Address for Network Advertised");

            ERROR_Assert( (bgpType == bgp->ip_version ),
              "Currently NETWORKs with one address type can be advertised by"
              " each BGP speacker");

            //Morteza:must be modified
            if (bgp->ip_version == NETWORK_IPV4)
            {
                NodeAddress nextHopAddressV4;
                NodeAddress outputNodeAddressV4 = 0;
                IO_ParseNetworkAddress(networkAddressString,
                               &outputNodeAddressV4,
                               &numHostBits);

                //Morteza:must be modified,advertised network may be a subset
                // of AS therefore we get next hop to that network
                NetworkGetInterfaceAndNextHopFromForwardingTable(
                    node,
                    outputNodeAddressV4,
                    &interfaceIndex,
                    &nextHopAddressV4);

                SetIPv4AddressInfo(&outputNodeAddress, outputNodeAddressV4);
                SetIPv4AddressInfo(&nextHopAddress, nextHopAddressV4);
                ip_size = 32;
                if (nextHopAddressV4 == (unsigned) NETWORK_UNREACHABLE)
                    valid = FALSE;

            }
            else if (bgp->ip_version == NETWORK_IPV6)
            {
                in6_addr nextHopAddressV6;
                in6_addr outputNodeAddressV6;
                memset(&outputNodeAddressV6,0,sizeof(in6_addr));

                if (IO_FindCaseInsensitiveStringPos(networkAddressString,
                                                    "SLA") != -1)
                {
                    //finds the equivalanet
                    unsigned int tla = 0,nla = 0,sla  = 0;

                    IO_ParseNetworkAddress(
                             networkAddressString,
                             &tla,
                             &nla,
                             &sla);

                    MAPPING_CreateIpv6GlobalUnicastAddr(
                                      tla,
                                      nla,
                                      sla,
                                      0, // To Create Network Address
                                      &outputNodeAddressV6);

                    numHostBits = MAX_PREFIX_LEN;
                }
                else
                {
                    unsigned int subnetPrefixLen;
                    IO_ParseNetworkAddress(
                             networkAddressString,
                             &outputNodeAddressV6,
                             &subnetPrefixLen);
                    numHostBits = 128 - subnetPrefixLen;
                }
                //check point,the Ipv6x function is not tested??
                Ipv6GetInterfaceAndNextHopFromForwardingTable(
                                        node,
                                        outputNodeAddressV6,
                                        &interfaceIndex,
                                        &nextHopAddressV6);
                SetIPv6AddressInfo(&outputNodeAddress, outputNodeAddressV6);
                if (interfaceIndex ==  NETWORK_UNREACHABLE)
                {
                    valid = FALSE;
                    memset(&nextHopAddressV6,0,sizeof(in6_addr));
                }
                SetIPv6AddressInfo(&nextHopAddress, nextHopAddressV6);
                ip_size = 128;
            }

            Address bgpInvalidAddr;
            memset(&bgpInvalidAddr,0,sizeof(Address));//bgpInvalidAddr = 0;

            // add the entry to the internal routing information base
            BgpFillRibEntry(
                &bgpRoutingInformationBase,
                outputNodeAddress,
                (unsigned char)(ip_size- numHostBits),
                bgpInvalidAddr,     //BGP_INVALID_ADDRESS,
                bgp->asId,
                TRUE, //Morteza:isClient
                bgp->routerId,
                BGP_ORIGIN_IGP,
                BGP_ORIGIN_IGP,
                NULL,
                (unsigned char) 0,
                bgp->localPref,
                bgp->localPref,
                bgp->multiExitDisc,
                BGP_PATH_ATTR_BEST_TRUE,
                nextHopAddress,
                BGP_DEFAULT_INTERNAL_WEIGHT,
                0);

            if (bgp->isSynchronized)
            {
                if (!valid )
                {
                    bgpRoutingInformationBase.pathAttrBest =
                                BGP_PATH_ATTR_BEST_FALSE;
                    bgpRoutingInformationBase.isValid      = FALSE;
                }
            }

            BUFFER_AddDataToDataBuffer(&(bgp->adjRibIn),
                           (char*) &bgpRoutingInformationBase,
                           sizeof(BgpRoutingInformationBase));

        }
        // read NEIGHBOR <ip address> REMOTE-AS <as id of peer> .. OR
        // read NEIGHBOR <ip address> WIEGHT <weight>

        BgpConnectionInformationBase* bgpConnInfo = NULL;
        int numEntries = 0, i = 0;

        if (!strcmp(token, "NEIGHBOR"))
        {
            BgpConnectionInformationBase bgpConnectionInformationBase;
            char ipAddressString[MAX_INPUT_FILE_LINE_LENGTH];
            char keyWord[MAX_INPUT_FILE_LINE_LENGTH];
            BOOL isNodeId = FALSE;
            BOOL client_peer =  FALSE;
            unsigned int readVal = 0;
            NetworkType  neigh_type;

            Address outputNodeAddress ; //morteza:peer IP address
            memset(&outputNodeAddress,0,sizeof(Address));
            GenericAddress tempAddr;

            sscanf(currentLine, "%*s %s %s %u",ipAddressString,
               keyWord, &readVal);

            neigh_type = MAPPING_GetNetworkType( ipAddressString );
            ERROR_Assert( (neigh_type == NETWORK_IPV4)||
                           (neigh_type == NETWORK_IPV6),
              "Bad Network Type Address");

            if (neigh_type == NETWORK_IPV4 )
            {
                IO_ParseNodeIdOrHostAddress(ipAddressString,
                        &tempAddr.ipv4, &isNodeId);
                SetIPv4AddressInfo(&outputNodeAddress,tempAddr.ipv4 );
                outputNodeAddress.networkType = NETWORK_IPV4;
            }
            else if (neigh_type == NETWORK_IPV6 )
            {
                NodeId tempId;
                IO_ParseNodeIdOrHostAddress(ipAddressString,
                                &tempAddr.ipv6, &tempId);
                SetIPv6AddressInfo(&outputNodeAddress,tempAddr.ipv6 );
                outputNodeAddress.networkType = NETWORK_IPV6;

            }

            if (isNodeId)
            {
                ERROR_ReportError("The format is NEIGHBOR "
                    "<interface address> <..> ...");
            }

            if (!strcmp(keyWord, "REMOTE-AS") ||
                !strcmp(keyWord, "ROUTE-REFLECTOR-CLIENT") )
            {
                if (!strcmp(keyWord, "REMOTE-AS"))
                {
                    if ((readVal > (unsigned) 65535) || (readVal == 0))
                    {
                        char errString[MAX_STRING_LENGTH];
                        sprintf(errString, "Autonomous System id must be "
                            "less than equal to 65535 and greater than 0");
                        ERROR_ReportError(errString);
                    }
                }
                else if (!strcmp(keyWord, "ROUTE-REFLECTOR-CLIENT"))
                {
                    readVal = localAsId;
                    client_peer = TRUE;
                    bgp->isRtReflector = TRUE;

                    bgpConnInfo = (BgpConnectionInformationBase*)
                    BUFFER_GetData(&(bgp->connInfoBase));

                    numEntries = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
                            / sizeof(BgpConnectionInformationBase);

                    for (i = 0; i < numEntries; i++)
                    {
                        if (Address_IsSameAddress(&bgpConnInfo[i].remoteAddr,
                                  &outputNodeAddress))
                        {
                            if (bgpConnInfo[i].asIdOfPeer != bgp->asId)
                            {
                                ERROR_ReportError("The RR Client is not"
                                    "found in the same AS\n");
                            }
                            if (DEBUG)
                            {
                                printf("Setting %dth connection as peer"
                                " client for RR %d\n",i,node->nodeId);
                            }
                            bgpConnInfo[i].isClient= TRUE;
                            break;
                        }
                    }
                    if (i == numEntries) //CIB not found
                    {
                        ERROR_ReportError("The CIB is not found for"
                        "this route neighbor\n");
                    }
                    continue;
                }

                // Add the current peer in formation in the connection
                // information base
                Address bgpInvalidAddr;
                BOOL isInitiating;

                memset(&bgpInvalidAddr,0,sizeof(Address));

                NodeAddress remoteId =
                MAPPING_GetNodeIdFromInterfaceAddress(node,
                    outputNodeAddress);

                if (node->nodeId >= remoteId )
                    isInitiating = TRUE;
                else
                    isInitiating = FALSE;

                BgpFillPeerEntry( node,
                 &bgpConnectionInformationBase,
                 outputNodeAddress,//Address remoteAddr
                     bgpInvalidAddr, //BGP_INVALID_ADDRESS,
                 (short) BGP_INVALID_ID,
                 APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                 bgp->routerId,
                 BGP_INVALID_ADDRESS,
                 (unsigned short) readVal,
                 (unsigned short) BGP_DEFAULT_EXTERNAL_WEIGHT,
                 (unsigned int) BGP_DEFAULT_MULTI_EXIT_DISC_VALUE,
                 ((localAsId == readVal) ? TRUE : FALSE),
                client_peer,
                 bgp->holdInterval,
                 (unsigned int) (node->appData.uniqueId)++,
                 BGP_IDLE,
                 BGP_INVALID_ID,
                 isInitiating);
                bgpConnectionInformationBase.ismedPresent = FALSE;
                BUFFER_AddDataToDataBuffer(&(bgp->connInfoBase),
                       (char*) &bgpConnectionInformationBase,
                       sizeof(BgpConnectionInformationBase));

            }
            else if (!strcmp(keyWord, "WEIGHT"))
            {
                // Found one weight assignment to one of the peer so find
                // the neighbor out and assign the weight
                BgpConnectionInformationBase* bgpConnInfo = NULL;
                int numEntries = 0;
                int i = 0;
                // check the weight field is not big to fit into 2 byte
                if (readVal > (unsigned) 65535)
                {
                    ERROR_ReportWarning("weight must be less than "
                        "equal to 65535. So considering the default"
                        " value of weight\n");
                }
                else
                {
                    bgpConnInfo = (BgpConnectionInformationBase*)
                     BUFFER_GetData(&(bgp->connInfoBase));

                    numEntries = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
                        / sizeof(BgpConnectionInformationBase);

                    for (i = 0; i < numEntries; i++)
                    {
                        if (Address_IsSameAddress(&bgpConnInfo[i].remoteAddr,
                              &outputNodeAddress))
                        {
                            bgpConnInfo[i].weight = (unsigned short) readVal;
                            break;
                        }
                    }
                }
            }
            else if (!strcmp(keyWord, "DEFAULT-METRIC"))
            {
                BgpConnectionInformationBase* bgpConnInfo = NULL;
                int numEntries = 0;
                int i = 0;
                if (readVal > 65535)
                {
                    ERROR_ReportWarning("Invalid BGP MED:Metric should set"
                        " to a 4-octet positive or zero value."
                        " It was set to default value\n");
                }
                else
                {
                    bgpConnInfo = (BgpConnectionInformationBase*)
                        BUFFER_GetData(&(bgp->connInfoBase));

                    numEntries = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
                        / sizeof(BgpConnectionInformationBase);

                    for (i = 0; i < numEntries; i++)
                    {
                        if (Address_IsSameAddress(&bgpConnInfo[i].remoteAddr,
                           &outputNodeAddress))
                        {
                            bgpConnInfo[i].multiExitDisc =
                                          (unsigned int) readVal;
                            bgpConnInfo[i].ismedPresent = TRUE;
                            break;
                        }
                    }
                }
            }
        }

        if (!strcmp(token,"BGP-CLUSTER-ID") )
        {
            char nextToken[MAX_INPUT_FILE_LINE_LENGTH];
            unsigned int readVal = 0;
            sscanf(currentLine, "%*s %s", nextToken);
            readVal = atoi(nextToken);
            bgp->clusterId = readVal;
        }

        if (!strcmp(token, "NO"))
        {
            char nextToken[MAX_INPUT_FILE_LINE_LENGTH];
            sscanf(currentLine, "%*s %s", nextToken);
            if (!strcmp(nextToken, "SYNCHRONIZATION"))
            {
                bgp->isSynchronized = FALSE;
            }
        }

        if (!strcmp(token, "DEFAULT-LOCAL-PREFERENCE"))
        {
            double lpf;
            char str[MAX_STRING_LENGTH];
            sscanf(currentLine, "%*s %s",str);
            lpf = atof(str);

            if ((lpf >= 0.0)&&(lpf <= 4294967295.0) )
            {
                bgp->localPref = (unsigned int) lpf;
            }
            else
            {
                ERROR_ReportWarning("Invalid BGP Local Pref:Local Pref. "
                    "should set to a 4-octet positive or zero value."
                    " It was set to default value\n");
            }

        }

        if (!strcmp(token, "DEFAULT-METRIC"))
        {
            double  med;
            char str[MAX_STRING_LENGTH];
            sscanf(currentLine, "%*s %s", str);
            med = atof(str);

            if ((med >= 0.0)&&(med <= 4294967295.0) )
            {
                bgp->multiExitDisc = (unsigned int) med;
            }
            else
            {
                ERROR_ReportWarning("Invalid BGP MED:Metric should set to a"
                    " 4-octet positive or zero value."
                    " It was set to default value\n");
            }
        }

        if (!strcmp(token, "NO-ADVERTISEMENT-FROM-IGP"))
        {
            bgp->advertiseIgp = FALSE;
        }
        if (!strcmp(token, "ADVERTISEMENT-FROM-OSPFv2-TYPE1-EXTERNAL"))
        {
            bgp->advertiseOspfExternalType1 = TRUE;
        }
        if (!strcmp(token, "ADVERTISEMENT-FROM-OSPFv2-TYPE2-EXTERNAL"))
        {
            bgp->advertiseOspfExternalType2 = TRUE;
        }

        if (!strcmp(token, "NO-REDISTRIBUTION-TO-OSPF"))
        {
            bgp->redistribute2Ospf = FALSE;
        }
        if (!strcmp(token, "BGP-PROBE-IGP-INTERVAL"))
        {
            char str1[MAX_INPUT_FILE_LINE_LENGTH];
            sscanf(currentLine, "%*s %s", str1);
            bgp->igpProbeInterval = TIME_ConvertToClock(str1);
            if (bgp->igpProbeInterval <= 0)
            {
                ERROR_ReportWarning("Invalid IGP Probe TIMER configuration:Timer "
                    "should set to a positive value.It was"
                    " set to default value\n");
                bgp->igpProbeInterval = BGP_DEFAULT_IGP_PROBE_TIME;
            }
        }

        if (!strcmp(token, "BGP-NO-REDISTRIBUTE-STATIC-CONNECTED-ROUTE"))
        {
            bgp->bgpRedistributeStaticAndDefaultRts = FALSE;
        }
    } // end for for (i = 0; i < bgpConfigFile->numLines; i++ )

}


//--------------------------------------------------------------------------
// NAME:        BgpSendKeepAlivePackets
//
// PURPOSE:     Function to send keep alive message to the bgp peer
//
//
// PARAMETERS:  node, the node which is sending message
//              bgp,  the internal structure of bgp
//              connectionStatus, the connection for which the it is
//                      sending the notification
//
// RETURN:      None
//
// ASSUMPTION:  None
//
//--------------------------------------------------------------------------

static
void BgpSendKeepAlivePackets(
    Node* node,
    BgpData* bgp,
    BgpConnectionInformationBase* connectionStatus)
{
    char* payload = NULL;
    char* tempPktPtr = NULL;

    ERROR_Assert(connectionStatus, "Invalid RIB structure\n");

    // Allocating space for the keepalive packet
    payload = (char*) MEM_malloc(BGP_MIN_HDR_LEN);
    tempPktPtr = payload;

    // Initializing members for commonheader
    memset(payload, 255, BGP_SIZE_OF_MARKER);
    tempPktPtr += BGP_SIZE_OF_MARKER;

    *(unsigned short*) tempPktPtr = BGP_MIN_HDR_LEN;
    tempPktPtr += BGP_MESSAGE_LENGTH_SIZE;

    *tempPktPtr = (unsigned char) BGP_KEEP_ALIVE;
    BgpSwapBytes((unsigned char*)payload,
                 FALSE);
    // Increment the statistical variable
    bgp->stats.keepAliveSent++;

    // Sending the keep alive packet to the bgp peer
    Message *msg = APP_TcpCreateMessage(
        node,
        connectionStatus->connectionId,
        TRACE_BGP,
        IPTOS_PREC_INTERNETCONTROL);

    APP_AddPayload(
        node,
        msg,
        payload,
        BGP_MIN_HDR_LEN);

#ifdef ADDON_BOEINGFCS
    TransportToAppDataReceived appData;
    appData.connectionId = connectionStatus->connectionId;
    appData.priority = IPTOS_PREC_INTERNETCONTROL;
    APP_AddInfo(node, msg, 
        &appData,
        sizeof(TransportToAppDataReceived),
        INFO_TYPE_TransportToAppData);
#endif

    APP_TcpSend(node, msg);

    MEM_free(payload);
}


//--------------------------------------------------------------------------
// FUNCTION BgpSendNotification
//
// PURPOSE  This function will send notification to the peer mentioning the
//          error code and the error subcode (if there is anything) is case
//          there is any error in the systems (bgp internal system)
//
// PARAMETERS
//      node, the node sending the Notification
//      bgp,  the bgp internal structure
//      connection, the connection for which the it is sending the
//                  notification
//      errCode, the error code of the Notification
//      errSubCode, the error sub code of the Notification (if there is any)
//                   or a value of 0 is passed
//
// RETURN
//      void
//--------------------------------------------------------------------------

static
void BgpSendNotification(
    Node* node,
    BgpData* bgp,
    BgpConnectionInformationBase* connection,
    int errCode,
    int errSubCode,
    char* packetErrorData = NULL,
    int lengthErrorData = 0)
{
    char* payload = NULL;
    char* tempPayloadPtr = NULL;

    ERROR_Assert(connection, "Invalid RIB structure\n");

    // Allocating space for the open packet
    payload = (char*) MEM_malloc(BGP_SIZE_OF_NOTIFICATION_MSG_EXCL_DATA +
                                lengthErrorData);

    tempPayloadPtr = payload;

    // Initialising members for commonheader
    memset(payload, 255, BGP_SIZE_OF_MARKER);
    tempPayloadPtr += BGP_SIZE_OF_MARKER;

    *(unsigned short*) tempPayloadPtr =
        BGP_SIZE_OF_NOTIFICATION_MSG_EXCL_DATA +
        (unsigned short)lengthErrorData;

    tempPayloadPtr += BGP_MESSAGE_LENGTH_SIZE;

    *tempPayloadPtr = BGP_NOTIFICATION;
    tempPayloadPtr++;

    *tempPayloadPtr = (unsigned char) errCode;
    tempPayloadPtr++;

    *tempPayloadPtr = (unsigned char) errSubCode;

    if (lengthErrorData != 0)
    {
        tempPayloadPtr++;
        if (packetErrorData != NULL)
            memcpy(tempPayloadPtr, packetErrorData, lengthErrorData);
        else
            memcpy(tempPayloadPtr, 0, lengthErrorData);
    }
    BgpSwapBytes((unsigned char*)payload,
                 FALSE);
    bgp->stats.notificationSent++;

    // Send the open packet to tcp
    if (DEBUG)
    {
        printf("node %u sent notification\n",node->nodeId);
    }
    Message *msg = APP_TcpCreateMessage(
        node,
        connection->connectionId,
        TRACE_BGP,
        IPTOS_PREC_INTERNETCONTROL);

    APP_AddPayload(
        node,
        msg,
        payload,
        BGP_SIZE_OF_NOTIFICATION_MSG_EXCL_DATA);

#ifdef ADDON_BOEINGFCS
    TransportToAppDataReceived appData;
    appData.connectionId = connection->connectionId;
    appData.priority = IPTOS_PREC_INTERNETCONTROL;
    APP_AddInfo(node, msg, 
        &appData,
        sizeof(TransportToAppDataReceived),
        INFO_TYPE_TransportToAppData);
#endif

    APP_TcpSend(node, msg);

    MEM_free(payload);
}


//--------------------------------------------------------------------------
// FUNCTION  BgpDeleteConnection
//
// PURPOSE:  Function to free memory allocated for neighbor information
//
// PARAMETERS:  bgp, bgp internal structure
//              connectionId, connection id of the connection to be deleted
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

void BgpDeleteConnection(
    BgpData* bgp,
    BgpConnectionInformationBase* connection)
{
    if (BUFFER_GetCurrentSize(&(connection->withdrawnRoutes)) > 0)
    {
        BUFFER_DestroyDataBuffer(&connection->withdrawnRoutes);
    }
    if (BUFFER_GetCurrentSize(&(connection->adjRibOut)) > 0)
    {
        BUFFER_DestroyDataBuffer(&connection->adjRibOut);
    }
    MEM_free(connection->buffer.data);

    connection->buffer.currentSize = 0;
    connection->buffer.expectedSize = 0;

    BUFFER_ClearDataFromDataBuffer(
        &bgp->connInfoBase,
        (char*) connection,
        sizeof(BgpConnectionInformationBase),
        FALSE);
}


//--------------------------------------------------------------------------
// FUNCTION  BgpFreeUpdateMessgeStructure
//
// PURPOSE:  Function to free the memory allocated for the update message
//           structure
//
//
// PARAMETERS:  updateMsg, the update message pointer to free
//              lengthOfNlri, length of the Nlri field of the update
//                            message
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void BgpFreeUpdateMessageStructure( BgpUpdateMessage* updateMessage ,
                                   int lengthOfNlri)

{
    BgpRouteInfo* rtInfo;
    unsigned short pathLength = 0;

    ERROR_Assert(updateMessage, "Invalid update message\n");


    //in MBGP,we set updateMessage->withdrawnRoutes = NULL
    rtInfo = updateMessage->withdrawnRoutes;

    // free withdrawn part
    if (rtInfo != NULL)
    {
        MEM_free(updateMessage->withdrawnRoutes);
    }

    memcpy(&pathLength, &updateMessage->totalPathAttributeLength,
       BGP_PATH_ATTRIBUTE_LENGTH_SIZE);

    // free as attribute if the length is not 0
    if (pathLength)
    {
        int numAttr = GetNumAttrInUpdate(updateMessage);
        for (int i = 0; i < numAttr; i++)
        {
            if (updateMessage->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_ORIGIN)
            {
                // free origin value (attribute type 1)
                    MEM_free(updateMessage->pathAttribute[i].attributeValue);

                pathLength = (unsigned short)
                    (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                    - sizeof(OriginValue));
            }
            else if (updateMessage->pathAttribute[i].attributeType ==
                    BGP_PATH_ATTR_TYPE_NEXT_HOP)
            {
                // free nexthop value (attribute type 3)
                MEM_free(updateMessage->pathAttribute[i].attributeValue);

                pathLength = (unsigned short)
                    (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                 - sizeof(NextHopValue));
            }
            else if (updateMessage->pathAttribute[i].attributeType ==
                 BGP_PATH_ATTR_TYPE_AS_PATH)
            {
                // Free as path (attribute type 2)
                unsigned short asPathLength;
                BgpPathAttributeValue* asPath;

                memcpy(&asPathLength, &updateMessage->pathAttribute[i].
                    attributeLength, BGP_ATTRIBUTE_LENGTH_SIZE);

                asPath = (BgpPathAttributeValue*)
                    updateMessage->pathAttribute[i].attributeValue;
                if (asPathLength)
                {
                    if (asPath->pathSegmentValue)
                    {
                        MEM_free(asPath->pathSegmentValue);
                    }
                    MEM_free(asPath);
                }

                pathLength = (unsigned short)
                        (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE -
                        asPathLength);
            }
            else if (updateMessage->pathAttribute[i].attributeType ==
             BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC)
            {
                // Free next hop (attribute type 3)
                MEM_free(updateMessage->pathAttribute[i].attributeValue);

                pathLength = (unsigned short)
                        (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE -
                         sizeof(MedValue));

            }
            else if (updateMessage->pathAttribute[i].attributeType ==
             BGP_PATH_ATTR_TYPE_LOCAL_PREF)
            {
                // Free next hop (attribute type 3)
                MEM_free(updateMessage->pathAttribute[i].attributeValue);

                pathLength = (unsigned short)
                        (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE -
                        sizeof(LocalPrefValue));

            }
            else if (updateMessage->pathAttribute[i].attributeType ==
                    BGP_PATH_ATTR_TYPE_ORIGINATOR_ID)
            {
                // free originator ID value (attribute type 9)
                    MEM_free(updateMessage->pathAttribute[i].attributeValue);

                pathLength = (unsigned short)
                        (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE -
                        sizeof(NodeAddress));
            }

            else if (updateMessage->pathAttribute[i].attributeType ==
             BGP_MP_REACH_NLRI)
            {
                MpReachNlri* mpReach = (MpReachNlri* )
                updateMessage->pathAttribute[i].attributeValue;
                MEM_free(mpReach->nextHop);
                MEM_free(mpReach->Nlri);


                pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                 - updateMessage->pathAttribute[i].attributeLength);

                MEM_free(updateMessage->pathAttribute[i].attributeValue);

            }
            else if (updateMessage->pathAttribute[i].attributeType ==
                 BGP_MP_UNREACH_NLRI)
            {
                MpUnReachNlri* mpUnReach = (MpUnReachNlri* )
                updateMessage->pathAttribute[i].attributeValue;
                MEM_free(mpUnReach->Nlri);


                pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                 - updateMessage->pathAttribute[i].attributeLength);

                MEM_free(updateMessage->pathAttribute[i].attributeValue);

            }
        }
    }
    if (updateMessage->pathAttribute)
    {
        // free the path attribute field
        MEM_free(updateMessage->pathAttribute);
    }
    //in MBGP,we set updateMessage->networkLayerReachabilityInfo = NULL
    // free Nlri field

    if (lengthOfNlri)
    {
        MEM_free(updateMessage->networkLayerReachabilityInfo);
    }

    // Free the update message structure
    MEM_free(updateMessage);
}

// ------------------------------------------------------------------------
// FUNCTION        : BgpPreCalculatePathAttributeLength()
//
// PURPOSE         : calculating possible length of path-attribute field
//                   before packging it into update message structure.
// PARAMETER       : data - pointer to BgpRoutingInformationBase entry
//
// RETURN          : possible length of path attribute field
//-------------------------------------------------------------------------
static
int BgpPreCalculatePathAttributeLength(BgpRoutingInformationBase* data,
                                       BgpData* bgp)
{
    // as origin Attribute Length
    int originAttributeLen = sizeof(OriginValue);
    int asPathLength = 0;

    // as path length = existing path length + own AS length +
    // length of as path list.
    if (data->asPathList)
    {
        asPathLength = data->asPathList->pathSegmentLength
        + sizeof(unsigned short)
        + 2 * sizeof(char);
    }
    int nextHopAttributeLength = 0;
    int mpReachAttrHeaderLength = 0;

    //MBGP ext includes no next hop in attribute
    // next hop attribute length
    if (bgp->ip_version == NETWORK_IPV4 )
    {
        nextHopAttributeLength = sizeof(NextHopValue);
    }
    else
    {
        mpReachAttrHeaderLength =
            MBGP_REACH_ATTR_MAX_HEADER_SIZE; //check this one later?
    }

    int sizeOfLocalPrefOrMed = sizeof(UInt32);

    int totalAttrLen = nextHopAttributeLength
                       + mpReachAttrHeaderLength
                       + asPathLength
                       + originAttributeLen
                       + sizeOfLocalPrefOrMed
                       + (4 * BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE);

    return totalAttrLen;
}

//--------------------------------------------------------------------------
// FUNCTION : BgpAllocateUpdateMessageStructure
//
// PURPOSE  : Builds up a allocate message structure and initializes it
//            with desired values.
//
// PARAMETERS:  node, the node which is allocating update message
//              connectionId,  connection Id of the connection
//
// RETURN:      BgpUpdateMessage*, The update message structure
//              sizeNetworkReachability, The size of the NLRI field
//
// ASSUMPTION:  In update message only mandatory attributes has been send
//              in "Path Attributes" fields. They are :
//              a) ORIGIN attribute (Type code 1)
//              b) AS_PATH attributes (Type code 2)
//              c) NEXT_HOP attributes (Type code 3)
//              For more details and usaes of this attributes please refer to
//              RFC 1771 (Section 4.3 "Update message format").
//
//              Network Layer Reachability Inforfation information contains
//              only fixed length (4 byte long) IP addresses. Where as RFC
//              1771 suggest :
//              "Reachability information is encoded as one or more 2-tuples
//              of the form <length,perfix>,whose fields are described below
//              Length :
//              The length field indicates length in bits of the IP
//              address prefix.A length of zero indicates a prifix that
//              matches all IP addresses (with prefix, itself of zero
//              octets).
//              Prefix:
//              The prefix field contains IP address prefixes follwed
//              by enough trailing bits to make the end of field fall on a
//              octet boundary.Note that value of trailing bit is irrlevant"
//
//              our length field always contains a value of 4 byte
//              and prifix is 4 byte long always and contains an IP address.
//
//              In the route enabled flag of the route entry is false then we
//              will add them in the withdrawn route field of the update
//              message. And the withdrawn routes will go with each update
//              even if it has been sent once.
//
//              The AS paths are there as Sequence
//
//              The Bgp speakers are the border router of the corrensponding
//              AS so it will advertise itself as next hop for the routes it
//              is advertising.
//--------------------------------------------------------------------------

static
BgpUpdateMessage* BgpAllocateUpdateMessageStructure(
    Node* node,
    unsigned short* sizeNetworkReachability,
    BgpConnectionInformationBase* connection)
{
    BgpUpdateMessage* bgpUpdateMessage = NULL;

    BgpData* bgp = (BgpData*) node->appData.exteriorGatewayVar;

    unsigned short* ptrToAsPath = NULL;
    unsigned int asPathLength = 0;

    unsigned int totSizeOfWithdrawnRoutes = 0;
    unsigned int totNumOfWithdrawnRoutes = 0;

    int i = 0;
    int numRoutes = 0;
    int numAttr = 0;

    BgpRoutingInformationBase* data = NULL;
    BgpRoutingInformationBase* data1 = NULL;

    int currentPacketSize = BGP_MIN_HDR_LEN;

    unsigned short prefix,afi;
    if (bgp->ip_version == NETWORK_IPV4 )
    {
        prefix = 4;
        afi = MBGP_IPV4_AFI;
    }
    else
    {
        prefix = 16;
        afi = MBGP_IPV6_AFI;
    }

    BgpAdjRibLocStruct** adjRibOut = (BgpAdjRibLocStruct**)
        BUFFER_GetData(&(connection->adjRibOut));

    int numAdjRibOut = BUFFER_GetCurrentSize(&(connection->adjRibOut))
        / sizeof(BgpAdjRibLocStruct*);
    int numAdjRibOut_temp = numAdjRibOut;
    bgpUpdateMessage = (BgpUpdateMessage*)
    MEM_malloc(sizeof(BgpUpdateMessage));
    //Route info embedded in attributes part
    //Initialize header, except length which will be filled later
    bgpUpdateMessage->commonHeader.type = BGP_UPDATE;
    memset(bgpUpdateMessage->commonHeader.marker, 255, BGP_SIZE_OF_MARKER);

    bgpUpdateMessage->infeasibleRouteLength = 0;
    bgpUpdateMessage->networkLayerReachabilityInfo = NULL;
    bgpUpdateMessage->withdrawnRoutes = NULL;
    bgpUpdateMessage->totalPathAttributeLength = 0;
    *sizeNetworkReachability = 0;//in MBGP is set to zero
    totNumOfWithdrawnRoutes =
        (BUFFER_GetCurrentSize(&(connection->withdrawnRoutes))
        / sizeof(BgpRouteInfo));

    if (bgp->ip_version == NETWORK_IPV6 )
    {
      totSizeOfWithdrawnRoutes =
      (totNumOfWithdrawnRoutes *BGP_IPV6_ROUTE_INFO_STRUCT_SIZE);
    }
    else
    {
        totSizeOfWithdrawnRoutes =
            (totNumOfWithdrawnRoutes *BGP_IPV4_ROUTE_INFO_STRUCT_SIZE);

        bgpUpdateMessage->infeasibleRouteLength = (unsigned short)
          totSizeOfWithdrawnRoutes;

        if (bgpUpdateMessage->infeasibleRouteLength)
        {
            bgpUpdateMessage->withdrawnRoutes = (BgpRouteInfo*)
            MEM_malloc(totNumOfWithdrawnRoutes * sizeof(BgpRouteInfo));

            memcpy(bgpUpdateMessage->withdrawnRoutes,
               BUFFER_GetData(&(connection->withdrawnRoutes)),
               totNumOfWithdrawnRoutes * sizeof(BgpRouteInfo));

            BUFFER_ClearDataFromDataBuffer(
               &(connection->withdrawnRoutes),
               (char*) BUFFER_GetData(&(connection->withdrawnRoutes)),
               totNumOfWithdrawnRoutes * sizeof(BgpRouteInfo),
               FALSE);
         }
         else
         {
             bgpUpdateMessage->withdrawnRoutes = NULL;
         }
        // withdrawn route is considered to be added so incriment
        // currentPacketSize. by total withdrawn route size +
        // sizeof(Unfeasible Routes Length) field.
        currentPacketSize += (totSizeOfWithdrawnRoutes +
                             sizeof(unsigned short));
    }

    if (numAdjRibOut > 0)
    {
        unsigned short sizeOfNlri = 0;
        BgpPathAttribute* pathAttribute = NULL;
        BgpPathAttributeValue* pathAttrVal = NULL;
        int preCalculatedAttrLen = 0;
        int preCalcMaxNumNlri = 0;

        data = (*adjRibOut)->ptrAdjRibIn;
        preCalculatedAttrLen = BgpPreCalculatePathAttributeLength(data, bgp);
        if (connection->isInternalConnection)
        {
            preCalculatedAttrLen = preCalculatedAttrLen + sizeof(UInt32);
        }
        if ((BGP_MAX_HDR_LEN - currentPacketSize - preCalculatedAttrLen) <
            (signed) ((sizeof(unsigned short) +
                      sizeof(NodeAddress) + sizeof(char))))
        {
            // Modify the update message structure that it is not containing
            bgpUpdateMessage->totalPathAttributeLength = 0;
            bgpUpdateMessage->pathAttribute = NULL;
            *sizeNetworkReachability = 0;
            bgpUpdateMessage->networkLayerReachabilityInfo = NULL;

            // Here at last you fill up the length field of the header.
            bgpUpdateMessage->commonHeader.length = (unsigned short)
                (BGP_MIN_HDR_LEN
                 + BGP_INFEASIBLE_ROUTE_LENGTH_SIZE
                 + bgpUpdateMessage->infeasibleRouteLength
                 + BGP_PATH_ATTRIBUTE_LENGTH_SIZE);

            return bgpUpdateMessage;
        }
        int remainingPacketSize = BGP_MAX_HDR_LEN
                                      - currentPacketSize
                                      - preCalculatedAttrLen
                                      - sizeof(unsigned short);

        // pre-calculate number of the NLRI entries.
        preCalcMaxNumNlri =  remainingPacketSize
        / (prefix + sizeof(char));

        bgpUpdateMessage->pathAttribute = (BgpPathAttribute*)
            MEM_malloc(sizeof(BgpPathAttribute)
               * MBGP_ALLOWED_ATTRIBUTE_TYPE);
        memset(bgpUpdateMessage->pathAttribute,
               0,
               sizeof(BgpPathAttribute)* MBGP_ALLOWED_ATTRIBUTE_TYPE);

        pathAttribute = bgpUpdateMessage->pathAttribute;

        // Set origin attribute
        BgpPathAttributeSetOptional
            (&(pathAttribute[0].bgpPathAttr), BGP_WELL_KNOWN_ATTRIBUTE);
        BgpPathAttributeSetTransitive
            (&(pathAttribute[0].bgpPathAttr), BGP_TRANSITIVE_ATTRIBUTE);
        BgpPathAttributeSetPartial
            (&(pathAttribute[0].bgpPathAttr), BGP_PATH_ATTR_COMPLETE);
        BgpPathAttributeSetExtndLen
            (&(pathAttribute[0].bgpPathAttr), TWO_OCTATE);
        BgpPathAttributeSetReserved(&(pathAttribute[0].bgpPathAttr), 0);

        // type 1 is for origin
        pathAttribute[0].attributeType = BGP_PATH_ATTR_TYPE_ORIGIN;
        pathAttribute[0].attributeLength = sizeof(OriginValue);
        pathAttribute[0].attributeValue = (char*)
            MEM_malloc(sizeof(OriginValue));

        *(pathAttribute[0].attributeValue) = data->originType;
        numAttr++;
        // Set AS_Path attribute
        BgpPathAttributeSetOptional
            (&(pathAttribute[1].bgpPathAttr), BGP_WELL_KNOWN_ATTRIBUTE);
        BgpPathAttributeSetTransitive
            (&(pathAttribute[1].bgpPathAttr), BGP_TRANSITIVE_ATTRIBUTE);
        BgpPathAttributeSetPartial
            (&(pathAttribute[1].bgpPathAttr), BGP_PATH_ATTR_COMPLETE);
        BgpPathAttributeSetExtndLen
            (&(pathAttribute[1].bgpPathAttr), TWO_OCTATE);
        BgpPathAttributeSetReserved(&(pathAttribute[1].bgpPathAttr), 0);
        // type 2 is for as path
        pathAttribute[1].attributeType = BGP_PATH_ATTR_TYPE_AS_PATH;

        numAttr++;

        // In case of internal routes my as number is there in the routing
        // table so I don't need to prepend it
        if (data->asPathList)
        {
            asPathLength = data->asPathList->pathSegmentLength;
        }

        if (!connection->isInternalConnection)
        {
            pathAttribute[1].attributeValue = (char*)
                MEM_malloc(sizeof(BgpPathAttributeValue));

            pathAttrVal = (BgpPathAttributeValue*) pathAttribute[1].
                attributeValue;

            // path segment type is as As sequence
            pathAttrVal->pathSegmentType = BGP_PATH_SEGMENT_AS_SEQUENCE;
            pathAttrVal->pathSegmentLength =
                (unsigned char) ((asPathLength + sizeof(unsigned short)) /
                 sizeof(unsigned short));
            if ((asPathLength + sizeof(unsigned short)) /
                 sizeof(unsigned short) > 255)
            {
                BgpFreeUpdateMessageStructure(bgpUpdateMessage,
                                              *sizeNetworkReachability);
                return NULL;
            }
           // Get as path length of original path length + own AS ID length.
            pathAttrVal->pathSegmentValue = (unsigned short*)
                        MEM_malloc(asPathLength + sizeof(unsigned short));
            memset(pathAttrVal->pathSegmentValue,
                   0,
                   asPathLength + sizeof(unsigned short));

            // prepend my own id
            memcpy(pathAttrVal->pathSegmentValue,
                   &bgp->asId,
                   sizeof(unsigned short));
            if (data->asPathList)
            {
                memcpy(&pathAttrVal->pathSegmentValue[1],
                   data->asPathList->pathSegmentValue,
                   asPathLength);
            }
            pathAttribute[1].attributeLength = (unsigned short)
                (2 * sizeof(char) + (pathAttrVal->pathSegmentLength *
                                     sizeof(unsigned short)));
        }
        else if (asPathLength == 0)
        {
            pathAttribute[1].attributeLength = 0;
            pathAttribute[1].attributeValue = NULL;
        }
        else
        {

            pathAttribute[1].attributeValue = (char*)
                MEM_malloc(sizeof(BgpPathAttributeValue));

            pathAttrVal = (BgpPathAttributeValue*) pathAttribute[1].
                attributeValue;

            // path segment type is as As sequence
            pathAttrVal->pathSegmentType = BGP_PATH_SEGMENT_AS_SEQUENCE;
            pathAttrVal->pathSegmentLength = (unsigned char) asPathLength;

            pathAttrVal->pathSegmentValue = (unsigned short*)
                MEM_malloc((asPathLength));

            pathAttrVal->pathSegmentLength = pathAttrVal->pathSegmentLength /
                                                    sizeof(unsigned short);

            if (data->asPathList)
            {
                memcpy(pathAttrVal->pathSegmentValue, data->asPathList->
                pathSegmentValue, asPathLength);
            }

            pathAttribute[1].attributeLength = (unsigned short)
                 (2 * sizeof(char) + asPathLength);
        }

        // For internal links both Local Preference or MED shall be
        // passed in UPDATE message. For external links only MEDS shall
        // be passed. MED will be set if the nlri was recvd from another AS
        // and this MED will be passed to internal links.

        // This connection is an internal connection so send Local
        // Pref to the peer
        int index = 0;
        if (connection->isInternalConnection)
        {
            index = 4;
            BgpPathAttributeSetOptional
                (&(pathAttribute[2].bgpPathAttr), BGP_WELL_KNOWN_ATTRIBUTE);
            BgpPathAttributeSetTransitive
                (&(pathAttribute[2].bgpPathAttr), BGP_TRANSITIVE_ATTRIBUTE);
            BgpPathAttributeSetPartial
                (&(pathAttribute[2].bgpPathAttr), BGP_PATH_ATTR_COMPLETE);
            BgpPathAttributeSetExtndLen
                (&(pathAttribute[2].bgpPathAttr), TWO_OCTATE);
            BgpPathAttributeSetReserved(&(pathAttribute[2].bgpPathAttr), 0);

            // type 5 is for LOCAL-PREF
            pathAttribute[2].attributeType = BGP_PATH_ATTR_TYPE_LOCAL_PREF;
            pathAttribute[2].attributeLength = sizeof(LocalPrefValue);

            pathAttribute[2].attributeValue = (char*)
                MEM_malloc(sizeof(LocalPrefValue));
            numAttr++;

            *((LocalPrefValue*) pathAttribute[2].attributeValue) =
                bgp->localPref;

            BgpPathAttributeSetOptional
                (&(pathAttribute[3].bgpPathAttr), BGP_OPTIONAL_ATTRIBUTE);
            BgpPathAttributeSetTransitive(&(pathAttribute[3].bgpPathAttr),
                BGP_NON_TRANSITIVE_ATTRIBUTE);
            BgpPathAttributeSetPartial
                (&(pathAttribute[3].bgpPathAttr), BGP_PATH_ATTR_COMPLETE);
            BgpPathAttributeSetExtndLen
                (&(pathAttribute[3].bgpPathAttr), TWO_OCTATE);
            BgpPathAttributeSetReserved(&(pathAttribute[3].bgpPathAttr), 0);

            // type 4 is for MED
            pathAttribute[3].attributeType =
                BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC;

            pathAttribute[3].attributeLength = sizeof(MedValue);

            pathAttribute[3].attributeValue = (char*)
                MEM_malloc(sizeof(MedValue));
            numAttr++;
            if (data->origin == BGP_ORIGIN_EGP)
            {
                *((MedValue*) pathAttribute[3].attributeValue) =
                            data->multiExitDisc;
            }
            else
            {
                if (connection->ismedPresent)
                {
                    *((MedValue*) pathAttribute[3].attributeValue) =
                            connection->multiExitDisc;
                }
                else
                {
                    *((MedValue*) pathAttribute[3].attributeValue) =
                            bgp->multiExitDisc;
                }
            }
        }
        else
        {
            // This connection is an external connection so send MED value
            // to the peer
            index = 3;
            BgpPathAttributeSetOptional
                (&(pathAttribute[2].bgpPathAttr), BGP_OPTIONAL_ATTRIBUTE);
                BgpPathAttributeSetTransitive(&(pathAttribute[2].bgpPathAttr),
                    BGP_NON_TRANSITIVE_ATTRIBUTE);
            BgpPathAttributeSetPartial
                (&(pathAttribute[2].bgpPathAttr), BGP_PATH_ATTR_COMPLETE);
            BgpPathAttributeSetExtndLen
                (&(pathAttribute[2].bgpPathAttr), TWO_OCTATE);
            BgpPathAttributeSetReserved(&(pathAttribute[2].bgpPathAttr), 0);


            pathAttribute[2].attributeType =
                BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC;

            pathAttribute[2].attributeLength = sizeof(MedValue);

            pathAttribute[2].attributeValue = (char*)
                MEM_malloc(sizeof(MedValue));
            numAttr++;
            if (connection->ismedPresent)
            {
                *((MedValue*) pathAttribute[2].attributeValue) =
                    connection->multiExitDisc;
            }
            else
            {
                *((MedValue*) pathAttribute[2].attributeValue) =
                bgp->multiExitDisc;
            }
        }

        if (connection->isInternalConnection
            && bgp->isRtReflector
            && data->originatorId)
        {
            BgpPathAttributeSetOptional(&(pathAttribute[index].bgpPathAttr),
                                        BGP_OPTIONAL_ATTRIBUTE);
            BgpPathAttributeSetTransitive(&(pathAttribute[index].bgpPathAttr),
                                          BGP_NON_TRANSITIVE_ATTRIBUTE);
            BgpPathAttributeSetPartial(&(pathAttribute[index].bgpPathAttr),
                                       BGP_PATH_ATTR_COMPLETE);
            BgpPathAttributeSetExtndLen(&(pathAttribute[index].bgpPathAttr),
                                        TWO_OCTATE);
            BgpPathAttributeSetReserved(&(pathAttribute[index].bgpPathAttr),
                                        0);

            pathAttribute[index].attributeType =
                BGP_PATH_ATTR_TYPE_ORIGINATOR_ID;
            pathAttribute[index].attributeLength = sizeof (NodeAddress);
            pathAttribute[index].attributeValue = (char*)
            MEM_malloc(pathAttribute[index].attributeLength);
            memcpy(pathAttribute[index].attributeValue,
                (char*)&data->originatorId, sizeof(NodeAddress));
            numAttr++;
            index++;
        }
        if (bgp->ip_version == NETWORK_IPV4 )
        {
            // Set next hop attribute
            BgpPathAttributeSetOptional(&(pathAttribute[index].bgpPathAttr),
                BGP_WELL_KNOWN_ATTRIBUTE);
            BgpPathAttributeSetTransitive(&(pathAttribute[index].bgpPathAttr),
                BGP_TRANSITIVE_ATTRIBUTE);
            BgpPathAttributeSetPartial(&(pathAttribute[index].bgpPathAttr),
                BGP_PATH_ATTR_COMPLETE);
            BgpPathAttributeSetExtndLen(&(pathAttribute[index].bgpPathAttr),
                TWO_OCTATE);
            BgpPathAttributeSetReserved(&(pathAttribute[index].bgpPathAttr),
                0);

            // type 3 is for next hop
            pathAttribute[index].attributeType = BGP_PATH_ATTR_TYPE_NEXT_HOP;
            pathAttribute[index].attributeLength = 4;
            pathAttribute[index].attributeValue = (char*)
                MEM_malloc(pathAttribute[index].attributeLength);
            NodeAddress nextHopV4 = GetIPv4Address(connection->localAddr);
            memcpy(pathAttribute[index].attributeValue,
               (char*)&nextHopV4, sizeof(NodeAddress));
            BOOL isnexthopset = FALSE;
            Address next_hop = data->nextHop;
            asPathLength= 0;
            ptrToAsPath = NULL;
            if (data->asPathList)
            {
                asPathLength = data->asPathList->pathSegmentLength;
                ptrToAsPath = data->asPathList->pathSegmentValue;
            }

            BgpRoutingInformationBase* temp_data = (*adjRibOut)->ptrAdjRibIn;
            for (i = 0; i < numAdjRibOut; i++)
            {
                BgpRoutingInformationBase* data1 = adjRibOut[i]->ptrAdjRibIn;

                if (((data1->asPathList) &&
                    (data1->asPathList->pathSegmentLength == asPathLength))
                || ((data1->asPathList == NULL) && (asPathLength == 0)))
                {
                    if (((data1->asPathList) &&
                        !memcmp(data1->asPathList->pathSegmentValue,
                        ptrToAsPath, asPathLength)) ||
                        (data1->asPathList == NULL))
                    {
                        if (!bgp->bgpNextHopLegacySupport)
                        {
                            if ((connection->isInternalConnection) &&
                                (temp_data->origin == BGP_ORIGIN_EGP))
                            {
                                if ((!MAPPING_CompareAddress(next_hop,
                                                            data1->nextHop))
                                   && (!isnexthopset))
                                {
                                    NodeAddress nextHopV4 =
                                        GetIPv4Address(data1->nextHop);
                                    memcpy(pathAttribute[index].attributeValue,
                                   (char*)&nextHopV4, sizeof(NodeAddress));
                                    isnexthopset = TRUE;
                                }
                            }
                        }
                        i--;
                        numAdjRibOut--;
                    }
                }//if
            }//for
            numAttr++;
            index++;
        }
        else
        {
            //Morteza:MP_REACH_NLRI
            BgpPathAttributeSetOptional
            (&(pathAttribute[index].bgpPathAttr), BGP_OPTIONAL_ATTRIBUTE);
            BgpPathAttributeSetTransitive(&(pathAttribute[index].bgpPathAttr),
                BGP_NON_TRANSITIVE_ATTRIBUTE);
            BgpPathAttributeSetPartial
            (&(pathAttribute[index].bgpPathAttr), BGP_PATH_ATTR_COMPLETE);
            BgpPathAttributeSetExtndLen
            (&(pathAttribute[index].bgpPathAttr), TWO_OCTATE);
            BgpPathAttributeSetReserved(&(pathAttribute[index].bgpPathAttr),
                0);

            // type 14 is for MP_REACH_NLRI
            pathAttribute[index].attributeType = BGP_MP_REACH_NLRI;
            numAttr++;


            // Assignment of this node's network layers reachability
            // information in MP_REACH_NLRI.
            asPathLength = 0;
            ptrToAsPath = NULL;
            if (data->asPathList)
            {
                asPathLength = data->asPathList->pathSegmentLength;
                ptrToAsPath = data->asPathList->pathSegmentValue;
            }

            Address next_hop = data->nextHop;
            MpReachNlri*  mpReach =
                (MpReachNlri* ) MEM_malloc( sizeof(MpReachNlri) );
            mpReach->afIden = (unsigned short) afi;
            mpReach->subAfIden = 1; //curerrently only unicast route advert
            //No SNPA is included in UPDATE message
            mpReach->numSnpa = 0;
            mpReach->Snpa = NULL;

            in6_addr  nextHopV6 = GetIPv6Address(connection->localAddr);
            mpReach->nextHopLen = 16;//16 or 32????
            mpReach->nextHop = (char*)
            MEM_malloc( mpReach->nextHopLen );
            memcpy(mpReach->nextHop,(char*)&nextHopV6, sizeof(in6_addr));


            numRoutes = 0;
            sizeOfNlri = 0;

            for (i = 0; i < numAdjRibOut; i++)
            {

                data1 = adjRibOut[i]->ptrAdjRibIn;

                // The Nlri field must have entries with same as length
                if (((data1->asPathList) &&
                    (data1->asPathList->pathSegmentLength == asPathLength))
                || ((data1->asPathList == NULL) && (asPathLength == 0)))
                {
                    // NLRI informations are selected on the basis of
                    // content of the AS path field. All NLRI addresses
                    // should have same AS path list field.
                    if (((data1->asPathList) &&
                        !memcmp(data1->asPathList->pathSegmentValue,
                         ptrToAsPath, asPathLength)) ||
                         (data1->asPathList == NULL))
                    {
                        if (numRoutes == preCalcMaxNumNlri)
                        {
                            // maximum number of nlri entries has been
                            // packeged in Update message. So have a break
                            break;
                        }
                        numRoutes++;
                        sizeOfNlri += (sizeof(char) + prefix);
                    }
                }
            }
            // Allocate space to place network layer reachability
            // information+nextHopLen + nextHop+SnpaLen
            pathAttribute[index].attributeLength = MBGP_AFI_SIZE
                + MBGP_SUB_AFI_SIZE
                + MBGP_NEXT_HOP_LEN_SIZE
                + mpReach->nextHopLen
                + MBGP_NUM_SNPA_SIZE
                + (unsigned short) sizeOfNlri;
            mpReach->Nlri = (BgpRouteInfo* )
                MEM_malloc( sizeof(BgpRouteInfo)* numRoutes );

            numRoutes = 0;
            sizeOfNlri = 0;

            BOOL isnexthopset = FALSE;
            BgpRoutingInformationBase* temp_data = (*adjRibOut)->ptrAdjRibIn;


            for (i = 0; i < numAdjRibOut; i++)
            {
                data1 = adjRibOut[i]->ptrAdjRibIn;

                if (((data1->asPathList) &&
                    (data1->asPathList->pathSegmentLength == asPathLength))
                || ((data1->asPathList == NULL) && (asPathLength == 0)))
                {
                    if (((data1->asPathList) &&
                        !memcmp(data1->asPathList->pathSegmentValue,
                         ptrToAsPath, asPathLength)) ||
                         (data1->asPathList == NULL))
                    {
                        if (!bgp->bgpNextHopLegacySupport)
                        {
                            if ((connection->isInternalConnection) &&
                                (temp_data->origin == BGP_ORIGIN_EGP))
                            {
                                if ((!MAPPING_CompareAddress(next_hop,
                                                            data1->nextHop))
                                   && (!isnexthopset))
                                {
                                    in6_addr  nextHopV6 =
                                        GetIPv6Address(data1->nextHop);
                                    mpReach->nextHopLen = 16;//16 or 32????
                                    mpReach->nextHop = (char*)
                                    MEM_malloc( mpReach->nextHopLen );
                                    memcpy(mpReach->nextHop,
                                        (char*)&nextHopV6, sizeof(in6_addr));
                                    isnexthopset = TRUE;
                                }
                            }
                        }
                        if (numRoutes == preCalcMaxNumNlri)
                        {
                            // maximum number of nlri entries has been
                            // packeged in Update message. So have a break
                            break;
                        }

                        //check point??
                        memcpy((char* )& mpReach->Nlri[numRoutes]
                            ,(char* )&data1->destAddress,
                            sizeof(BgpRouteInfo) );


                        BUFFER_ClearDataFromDataBuffer(
                           &(connection->adjRibOut),
                           (char*) &adjRibOut[i],
                           sizeof(BgpAdjRibLocStruct*),
                           FALSE);
                        i--;
                        numAdjRibOut--;
                        numRoutes++;
                    }
            }//if
        }//for

        pathAttribute[index].attributeValue = (char* ) mpReach;
        // End assigement of network reachability info.


        if (totSizeOfWithdrawnRoutes)
        {
            ++index;
            //Morteza:MP_UNREACH_NLRI
            BgpPathAttributeSetOptional
                (&(pathAttribute[index].bgpPathAttr),
                BGP_OPTIONAL_ATTRIBUTE);
            BgpPathAttributeSetTransitive
                (&(pathAttribute[index].bgpPathAttr),
                BGP_NON_TRANSITIVE_ATTRIBUTE);
            BgpPathAttributeSetPartial
                (&(pathAttribute[index].bgpPathAttr),
                BGP_PATH_ATTR_COMPLETE);
            BgpPathAttributeSetExtndLen
                (&(pathAttribute[index].bgpPathAttr),
                TWO_OCTATE);
            BgpPathAttributeSetReserved(
                &(pathAttribute[index].bgpPathAttr), 0);

            // type 15 is for MP_UNREACH_NLRI
                pathAttribute[index].attributeType = BGP_MP_UNREACH_NLRI;
            // Get total size of with drawn routes
            numAttr++;

            pathAttribute[index].attributeLength = (unsigned short)
                (totSizeOfWithdrawnRoutes
                + MBGP_AFI_SIZE
                + MBGP_SUB_AFI_SIZE);


            MpUnReachNlri*  mpUnreach = (MpUnReachNlri*)
            MEM_malloc(sizeof(MpUnReachNlri));

            mpUnreach->afIden = (unsigned short) afi;
            mpUnreach->subAfIden = 1;//currently we only advertise unicast routes
            mpUnreach->Nlri = (BgpRouteInfo*)
                    MEM_malloc(totNumOfWithdrawnRoutes *
                                sizeof(BgpRouteInfo));
            //check point??
            memcpy(mpUnreach->Nlri,
               BUFFER_GetData(&(connection->withdrawnRoutes)),
               totNumOfWithdrawnRoutes * sizeof(BgpRouteInfo));

            BUFFER_ClearDataFromDataBuffer(
                           &(connection->withdrawnRoutes),
                           (char*) BUFFER_GetData(&(connection->withdrawnRoutes)),
                           totNumOfWithdrawnRoutes * sizeof(BgpRouteInfo),
                           FALSE);

            pathAttribute[index].attributeValue = (char* ) mpUnreach;
        }
    }
    if ((bgp->isRtReflector) &&
            (connection->isInternalConnection))
    {
        // if reflecting routes received from other nodes, add cluster id
        in6_addr bgpInvalidAddr;
        memset(&bgpInvalidAddr,0,sizeof(in6_addr));//bgpInvalidAddr = 0;
        if (CMP_ADDR6(GetIPv6Address(data->peerAddress),
                   bgpInvalidAddr) != 0  )
        {
            if (DEBUG)
                printf("Incrementing reflected route stats\n");
            bgp->stats.reflectedupdateSent++;
        }
    }
    bgpUpdateMessage->totalPathAttributeLength = (unsigned short)
                            (numAttr* BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE);
    for (i=0; i<numAttr; i++)
    {
        bgpUpdateMessage->totalPathAttributeLength =
                            bgpUpdateMessage->totalPathAttributeLength +
                                    (pathAttribute[i].attributeLength);
    }

    numAdjRibOut = numAdjRibOut_temp;
    if (bgp->ip_version == NETWORK_IPV4 )
    {
        asPathLength = 0;
        ptrToAsPath = NULL;
        // Assignment of this node's network layers reachability
        // information.
        if (data->asPathList)
        {
            asPathLength = data->asPathList->pathSegmentLength;
            ptrToAsPath = data->asPathList->pathSegmentValue;
        }
        numRoutes = 0;
        sizeOfNlri = 0;
        for (i = 0; i < numAdjRibOut; i++)
        {
            data1 = adjRibOut[i]->ptrAdjRibIn;
            // The Nlri field must have entries with same as length
                if (((data1->asPathList) &&
                   (data1->asPathList->pathSegmentLength == asPathLength)) ||
                   ((data1->asPathList == NULL) && (asPathLength == 0)))
            {
                // NLRI informations are selected on the basis of
                // content of the AS path field. All NLRI addresses
                // should have same AS path list field.
                    if (((data1->asPathList) &&
                        !memcmp(data1->asPathList->pathSegmentValue,
                        ptrToAsPath, asPathLength)) ||
                        (data1->asPathList == NULL))
                {
                    if (numRoutes == preCalcMaxNumNlri)
                    {
                        // maximum number of nlri entries has been
                        // packeged in Update message. So have a break
                        break;
                    }
                    numRoutes++;
                    sizeOfNlri += (sizeof(char) + prefix);
                }
            }
        }
       //Fill NLRI
        bgpUpdateMessage->networkLayerReachabilityInfo = (BgpRouteInfo* )
        MEM_malloc( sizeof(BgpRouteInfo)* numRoutes);
        numRoutes = 0;
        for (i = 0; i < numAdjRibOut; i++)
        {
            data1 = adjRibOut[i]->ptrAdjRibIn;
                if (((data1->asPathList) &&
                    (data1->asPathList->pathSegmentLength == asPathLength))
                || ((data1->asPathList == NULL) && (asPathLength == 0)))
                {
                    if (((data1->asPathList) &&
                        !memcmp(data1->asPathList->pathSegmentValue,
                        ptrToAsPath, asPathLength)) ||
                        (data1->asPathList == NULL))
                    {
                        if (numRoutes == preCalcMaxNumNlri)
                        {
                            // maximum number of nlri entries has been
                            // packeged in Update message. So have a break
                            break;
                        }
                        memcpy((char* )& bgpUpdateMessage->
                            networkLayerReachabilityInfo[numRoutes],
                            (char* )&data1->destAddress,
                        sizeof(BgpRouteInfo) );

                        BUFFER_ClearDataFromDataBuffer(
                           &(connection->adjRibOut),
                           (char*) &adjRibOut[i],
                           sizeof(BgpAdjRibLocStruct*),
                           FALSE);
                        i--;
                        numAdjRibOut--;
                        numRoutes++;
                    }
                }//if
            }//for
        }
        *sizeNetworkReachability = sizeOfNlri;

        // End of assignment of attribute fields.
        // Here at last you fill up the length field of the header.
        bgpUpdateMessage->commonHeader.length = (unsigned short)
        (BGP_MIN_HDR_LEN
         + BGP_INFEASIBLE_ROUTE_LENGTH_SIZE  // infeasiable route length
         + bgpUpdateMessage->infeasibleRouteLength //which is 0 for IPv6
         + BGP_PATH_ATTRIBUTE_LENGTH_SIZE    // path attiribute length
         + bgpUpdateMessage->totalPathAttributeLength
         + sizeOfNlri);//which is 0 for IPv6
    }
    else if (numAdjRibOut == 0)
    {
        // Control is entering into this block means that there is no valid
        // route in the update message but it is containing only withdrawn
        // routes so the as path length and the nlri field will be NULL and
        // the respective size will be 0

        // Modify the update message structure that it is not containing any
        // NLRI information


        if (bgp->ip_version == NETWORK_IPV6 )
        {
            *sizeNetworkReachability = 0;

            BgpPathAttribute* pathAttribute = NULL;

            bgpUpdateMessage->pathAttribute = (BgpPathAttribute*)
                    MEM_malloc(sizeof(BgpPathAttribute));

            pathAttribute = bgpUpdateMessage->pathAttribute;

            totNumOfWithdrawnRoutes =
                (BUFFER_GetCurrentSize(&(connection->withdrawnRoutes))
                 / sizeof(BgpRouteInfo));
                totSizeOfWithdrawnRoutes = (totNumOfWithdrawnRoutes *
                    BGP_IPV6_ROUTE_INFO_STRUCT_SIZE);

            if (totSizeOfWithdrawnRoutes)
            {
                BgpPathAttributeSetOptional
                (&(pathAttribute[0].bgpPathAttr), BGP_OPTIONAL_ATTRIBUTE);
                BgpPathAttributeSetTransitive(&(pathAttribute[0].bgpPathAttr),
                        BGP_NON_TRANSITIVE_ATTRIBUTE);
                BgpPathAttributeSetPartial
                (&(pathAttribute[0].bgpPathAttr), BGP_PATH_ATTR_COMPLETE);
                BgpPathAttributeSetExtndLen
                (&(pathAttribute[0].bgpPathAttr), TWO_OCTATE);
                BgpPathAttributeSetReserved(&(pathAttribute[0].bgpPathAttr),
                        0);

                // type 15 is for MP_UNREACH_NLRI
                pathAttribute[0].attributeType = BGP_MP_UNREACH_NLRI;
                // Get total size of with drawn routes
                numAttr++;


                pathAttribute[0].attributeLength = (unsigned short)
                (totSizeOfWithdrawnRoutes
                 + MBGP_AFI_SIZE
                 + MBGP_SUB_AFI_SIZE);

                MpUnReachNlri* mpUnreach =  (MpUnReachNlri*)
                MEM_malloc(sizeof(MpUnReachNlri));

                mpUnreach->afIden = (unsigned short) afi;
                        mpUnreach->subAfIden = 1;//only advertising unicast routes
                mpUnreach->Nlri = (BgpRouteInfo*)
                MEM_malloc(totNumOfWithdrawnRoutes * sizeof(BgpRouteInfo));


                memcpy(mpUnreach->Nlri,
                   BUFFER_GetData(&(connection->withdrawnRoutes)),
                   totNumOfWithdrawnRoutes * sizeof(BgpRouteInfo));

                BUFFER_ClearDataFromDataBuffer(&(connection->withdrawnRoutes),
                    (char*) BUFFER_GetData(&(connection->withdrawnRoutes)),
                    totNumOfWithdrawnRoutes * sizeof(BgpRouteInfo), FALSE);
                pathAttribute[0].attributeValue = (char* ) mpUnreach;

                // Here at last you fill up the length field of the header.
                bgpUpdateMessage->totalPathAttributeLength = (unsigned short)
                    ((pathAttribute[0].attributeLength)
                    +(BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE));

                bgpUpdateMessage->commonHeader.length = (unsigned short)
                    (BGP_MIN_HDR_LEN
                    + BGP_INFEASIBLE_ROUTE_LENGTH_SIZE
                    + bgpUpdateMessage->infeasibleRouteLength
                    + BGP_PATH_ATTRIBUTE_LENGTH_SIZE
                    + bgpUpdateMessage->totalPathAttributeLength);
            }
            else
            {
                //no use of sending an update message!
                //should we print out error message!
                ERROR_Assert(FALSE, "Size of AdjRibOut is zero! and Withdrawn"
                    " routes is also 0\n");
                bgpUpdateMessage->totalPathAttributeLength = 0;
                bgpUpdateMessage->pathAttribute = NULL;
                // Here at last you fill up the length field of the header.
                bgpUpdateMessage->commonHeader.length = (unsigned short)
                    (BGP_MIN_HDR_LEN
                 + BGP_INFEASIBLE_ROUTE_LENGTH_SIZE
                 + bgpUpdateMessage->infeasibleRouteLength
                 + BGP_PATH_ATTRIBUTE_LENGTH_SIZE);
            }
        }
        else
        {
            bgpUpdateMessage->totalPathAttributeLength = 0;
            bgpUpdateMessage->pathAttribute = NULL;
            *sizeNetworkReachability = 0;
            bgpUpdateMessage->networkLayerReachabilityInfo = NULL;

            // Here at last you fill up the length field of the header.
            bgpUpdateMessage->commonHeader.length = (unsigned short)
               (BGP_MIN_HDR_LEN
                + BGP_INFEASIBLE_ROUTE_LENGTH_SIZE
                + bgpUpdateMessage->infeasibleRouteLength
                + BGP_PATH_ATTRIBUTE_LENGTH_SIZE);

        }
    }
    else
    {
        ERROR_Assert(FALSE, "Size of AdjRibOut is negative!\n");
    }
    // Allocation of update message structure is complete.
    return bgpUpdateMessage;
}

//--------------------------------------------------------------------------
// FUNCTION BOOL BgpCalculateSizeOfPacket
//
// PURPOSE : To calculate the length of the packet to be advertise
//
// PARAMETERS:  int numOctateInNLRI, the length of Nlri field
//              int* size, to return the length of the packet
//
//
// RETURN:      size of the packet
//
// ASSUMPTION:  None
//
//--------------------------------------------------------------------------

static
int BgpCalculateSizeOfNlri(
    BgpData* bgp,
    int numOctateInNLRI,
    BgpRouteInfo* routeInfo)
{
    int numOfNlri = 0, length = 0, numBytesInPrefix = 0;
    int i= 0, remainder = 0;
    BgpRouteInfo rtInfo;

    if (bgp->ip_version == NETWORK_IPV4)
    {
        numOfNlri = numOctateInNLRI / BGP_IPV4_ROUTE_INFO_STRUCT_SIZE;
        remainder = numOctateInNLRI % BGP_IPV4_ROUTE_INFO_STRUCT_SIZE;
    }
    else
    {
        numOfNlri = numOctateInNLRI / BGP_IPV6_ROUTE_INFO_STRUCT_SIZE;
        remainder = numOctateInNLRI % BGP_IPV6_ROUTE_INFO_STRUCT_SIZE;
    }
    /* The following if block is to take care of the case when
     * numOctateInNLRI is less than BGP_IPV4_ROUTE_INFO_STRUCT_SIZE or
     * BGP_IPV6_ROUTE_INFO_STRUCT_SIZE */
    if (numOfNlri == 0)
    {
        if (remainder)
        {
            numOfNlri++;
        }
    }
    for (i = 0; i < numOfNlri; i++)
    {
        rtInfo = routeInfo[i];
        length = (rtInfo.prefixLen) % 8;
        if (length == 0)
        {
            numBytesInPrefix += ((rtInfo.prefixLen / 8) + 1);
        }
        else
        {
            numBytesInPrefix += ((rtInfo.prefixLen / 8) + 1 + 1);
        }
    }
    return numBytesInPrefix;

    //End of calculation of size of NLRI field
}
//--------------------------------------------------------------------------
// FUNCTION BOOL BgpCalculateSizeOfPacket
//
// PURPOSE : To calculate the length of the packet to be advertise
//
// PARAMETERS:  BgpUpdateMessage* updateMessage, the update message structure
//              int numOctateInNLRI, the length of Nlri field
//              int* size, to return the length of the packet
//
//
// RETURN:      size of the packet
//
// ASSUMPTION:  None
//
//--------------------------------------------------------------------------

static
void BgpCalculateSizeOfPacket(
    BgpData* bgp,
    BgpUpdateMessage* updateMessage,
    int numOctateInNLRI,
    int* size)
{
    int numBytesInPrefix = 0;
    int numBytesInMpReach = 0, sizeOfmpNlri = 0, numBytesInMpUnReach = 0;

    if (bgp->ip_version == NETWORK_IPV6)
    {
        // Size of the common header
        *size += BGP_MIN_HDR_LEN;

        // Place to store the no of bytes in the infeasible routes
        *size += BGP_INFEASIBLE_ROUTE_LENGTH_SIZE ;

        // No of octates in the infeasible route length
        *size += updateMessage->infeasibleRouteLength;//in MBGP this must be 0

        // Total path attribute length
        *size += BGP_PATH_ATTRIBUTE_LENGTH_SIZE;

        // Size of path attribute
        *size += updateMessage->totalPathAttributeLength;

        *size += numOctateInNLRI;//in MBGP this must be 0
        return;
    }
    // Size of the common header
    *size += BGP_MIN_HDR_LEN;

    // Place to store the no of bytes in the infeasible routes
    *size += BGP_INFEASIBLE_ROUTE_LENGTH_SIZE ;

    // No of octates in the infeasible route length
    numBytesInPrefix = BgpCalculateSizeOfNlri(bgp,
                            updateMessage->infeasibleRouteLength,
                            updateMessage->withdrawnRoutes);

    *size += numBytesInPrefix;//in MBGP this must be 0

    // Total path attribute length
    *size += BGP_PATH_ATTRIBUTE_LENGTH_SIZE;

    // Size of path attribute
    int attr_length = updateMessage->totalPathAttributeLength;
    int numAttr = GetNumAttrInUpdate(updateMessage);
    BOOL found = FALSE;
    int index = BgpFindAttr(updateMessage, BGP_MP_REACH_NLRI, numAttr, found);
    if (found)
    {
        sizeOfmpNlri = updateMessage->pathAttribute[index].attributeLength;
        numBytesInMpReach = BgpCalculateSizeOfNlri(bgp,
                            sizeOfmpNlri,
                            (BgpRouteInfo*)updateMessage->
                            pathAttribute[index].attributeValue);
        attr_length = updateMessage->totalPathAttributeLength -
                      updateMessage->pathAttribute[index].attributeLength;
        attr_length += numBytesInMpReach;
    }

    found = FALSE;
    index = BgpFindAttr(updateMessage, BGP_MP_UNREACH_NLRI, numAttr, found);
    if (found)
    {
        sizeOfmpNlri = updateMessage->pathAttribute[index].attributeLength;
        numBytesInMpUnReach = BgpCalculateSizeOfNlri(bgp,
                            sizeOfmpNlri,
                            (BgpRouteInfo*)updateMessage->
                            pathAttribute[index].attributeValue);
        attr_length = updateMessage->totalPathAttributeLength -
                      updateMessage->pathAttribute[index].attributeLength;
        attr_length += numBytesInMpUnReach;
    }
    *size += attr_length;//in MBGP this must be 0

   // No of octates in the NLRI
    numBytesInPrefix = 0;
    numBytesInPrefix = BgpCalculateSizeOfNlri(bgp,
                            numOctateInNLRI,
                            updateMessage->networkLayerReachabilityInfo);


    *size += numBytesInPrefix;//in MBGP this must be 0

}


//--------------------------------------------------------------------------
// FUNCTION  BgpAllocateBgpRouteInfo
//
// PURPOSE : To allocate route info in the packet. route info comes in two
//           fields of update message. One in withdrawn routes and other in
//           Nlri
//
// PARAMETERS:  BgpRouteInfo* routeInfo, the routeInfo field of update
//              message,ie. either withdrawn routes or Nlri
//              char* pktPtr, the position of the packet whether the route
//                            info should be allocated
//              int numBytesToAllocate, size of the route info field
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpAllocateBgpRouteInfo(
    BgpData* bgp,
    BgpRouteInfo* routeInfo,
    char* pktPtr,
    unsigned short numBytesToAllocate,
    int numBytesInPrefix,
    unsigned short &numBytesUsed)
{
    int i = 0,numOfNlri = 0, length = 0;

    if (bgp->ip_version == NETWORK_IPV4)
        numOfNlri = numBytesToAllocate / BGP_IPV4_ROUTE_INFO_STRUCT_SIZE;
    else
    {
        numOfNlri = numBytesToAllocate / BGP_IPV6_ROUTE_INFO_STRUCT_SIZE;
        while (numBytesToAllocate)
        {

            // assign the length field which is subnet bits now
            memcpy(pktPtr, &routeInfo[i].prefixLen, sizeof(unsigned char));
            pktPtr++;

            // point to the prefix field and assign prefix
            memcpy(pktPtr, &routeInfo[i].prefix.interfaceAddr,
                   numBytesInPrefix);
            EXTERNAL_ntoh(
                (void*)pktPtr,
                numBytesInPrefix);
            pktPtr += numBytesInPrefix;
            numBytesToAllocate = numBytesToAllocate -
                                ((unsigned short)(numBytesInPrefix +
                                 sizeof(char)));
            i++;
        }
        return;
     }
    numBytesUsed = 0;
     // If num of bytes to allocate is 0 just return
    for (i = 0; i < numOfNlri; i++)
    {

        // assign the length field which is subnet bits now
        memcpy(pktPtr, &routeInfo[i].prefixLen, sizeof(unsigned char));
        pktPtr++;
        numBytesUsed++;
        length = (routeInfo[i].prefixLen) % 8;
        if (length == 0)
        {
            numBytesInPrefix = (routeInfo[i].prefixLen / 8);
        }
        else
        {
            numBytesInPrefix = (routeInfo[i].prefixLen / 8) + 1;
        }

        // point to the prefix field and assign prefix
        NodeAddress ipAddress = 0;
        GenericAddress temp_addr = routeInfo[i].prefix.interfaceAddr;
        if (numBytesInPrefix == 1)
        {
            ipAddress =  temp_addr.ipv4 >> 24;
        }
        else if (numBytesInPrefix == 2)
        {
            ipAddress = temp_addr.ipv4 >> 16;
        }
        else if (numBytesInPrefix == 3)
        {
            ipAddress = temp_addr.ipv4 >> 8;
        }
        else if (numBytesInPrefix == 4)
        {
            ipAddress = temp_addr.ipv4;
        }
        memcpy(pktPtr, &ipAddress, numBytesInPrefix);
        EXTERNAL_ntoh(
                (void*)pktPtr,
                numBytesInPrefix);
        pktPtr += numBytesInPrefix;
        numBytesUsed = numBytesUsed + (unsigned short)numBytesInPrefix;
    }
}

//--------------------------------------------------------------------------
// FUNCTION  BgpAllocateAsPath
//
// PURPOSE : To allocate as path in the packet.
//
// PARAMETERS:  AsPathValue* pathInfo, the pathInfo field of update message
//              char* pktPtr, the position of the packet whether the as info
//                            should be allocated
//              int numBytesToAllocate, size of the as info field
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpAllocateAsPath(
    AsPathValue* pathInfo,
    char* pktPtr,
    int numBytesToAllocate)
{
    int i = 0;
    // If no of bytes to allocate is 0 then return there will not be any
    // path information with the message
    while (numBytesToAllocate)
    {
        int pathSegLength = 0;
        int j = 0;

        // assign path segment type
        memcpy(pktPtr,
           &pathInfo[i].pathSegmentType,
           BGP_PATH_SEGMENT_TYPE_SIZE);

        pktPtr += BGP_PATH_SEGMENT_TYPE_SIZE;

        // assign path segment length
        memcpy(pktPtr,
           &pathInfo[i].pathSegmentLength,
           BGP_PATH_SEGMENT_LENGTH_SIZE);

        pathSegLength = pathInfo[i].pathSegmentLength;
        numBytesToAllocate -= ((pathSegLength * sizeof(unsigned short))+
                                (2 * sizeof(char)));
        pktPtr++;

        for (j = 0; j < pathSegLength; j++)
        {
            // assign each path segment value
            memcpy(pktPtr, &pathInfo[i].pathSegmentValue[j],
           sizeof(unsigned short));
           EXTERNAL_ntoh(
                (void*)pktPtr,
                sizeof(unsigned short));
            pktPtr += sizeof(unsigned short);
        }
        i++;
    }
}


//--------------------------------------------------------------------------
// FUNCTION  BgpAllocateRoutesInPacket
//
// PURPOSE : To allocate update message in flat packet.
//
// PARAMETERS:  BgpUpdateMessage* updateMessage, The update message structure
//              char** pktPtr,                   The flat packet where I am
//                                               to allocate update message
//              int noOctateInNLRI               no of bytes in the Nlri
//                                               field of the packet
//
// RETURN:      The packet
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------
static
int BgpAllocateRoutesInPacket(
    BgpData* bgp,
    BgpUpdateMessage* updateMessage,
    char** pktPtr,
    unsigned short noOctateInNLRI)
{
    int sizeOfPacket = 0;
    char* tempPktPtr = NULL;
    unsigned short pathLength = 0, totalLength = 0;
    int i = 0;
    int numBytesInPrefix = 0 ;

    ERROR_Assert(updateMessage, "Invalid update message\n");
    // Calculate the size of the packet and allocate the space

    BgpCalculateSizeOfPacket(bgp,
                             updateMessage,
                             noOctateInNLRI,
                             &sizeOfPacket);

    (*pktPtr) = (char*) MEM_malloc(sizeOfPacket);

    // Copy the different fields of the header
    tempPktPtr = (*pktPtr);

    // copy marker
    memcpy(tempPktPtr,
       updateMessage->commonHeader.marker,
       BGP_SIZE_OF_MARKER);

    //Swap before sending to n/w
    EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_SIZE_OF_MARKER);
    tempPktPtr += BGP_SIZE_OF_MARKER;

    // copy packet length
    memcpy(tempPktPtr, &sizeOfPacket,
       BGP_MESSAGE_LENGTH_SIZE);
    EXTERNAL_ntoh(
                (void*)tempPktPtr,
       BGP_MESSAGE_LENGTH_SIZE);
    tempPktPtr += BGP_MESSAGE_LENGTH_SIZE;

    // copy packet type (update)
    memcpy(tempPktPtr, &updateMessage->commonHeader.type,
       BGP_MESSAGE_TYPE_SIZE);
    EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_MESSAGE_TYPE_SIZE);
    tempPktPtr += BGP_MESSAGE_TYPE_SIZE;

    //in MBGP updateMessage->withdrawnRoutes is NULL
    // copy infeasible route length field
    numBytesInPrefix = BgpCalculateSizeOfNlri(bgp,
                            updateMessage->infeasibleRouteLength,
                            updateMessage->withdrawnRoutes);
    memcpy(tempPktPtr, &numBytesInPrefix,
       BGP_INFEASIBLE_ROUTE_LENGTH_SIZE);
    EXTERNAL_ntoh(
                (void*)tempPktPtr,
       BGP_INFEASIBLE_ROUTE_LENGTH_SIZE);
    tempPktPtr += BGP_INFEASIBLE_ROUTE_LENGTH_SIZE;

    // assign the infeasible routes
    //in MBGP updateMessage->withdrawnRoutes is NULL

    BgpAllocateBgpRouteInfo(bgp, updateMessage->withdrawnRoutes,
        tempPktPtr, updateMessage->infeasibleRouteLength, 4, totalLength);

    tempPktPtr += totalLength;//this must be 0 in MBGP
    int attr_length = updateMessage->totalPathAttributeLength;
    int numBytesInMpReach = 0, sizeOfmpNlri = 0, numBytesInMpUnReach = 0;

    // assign attribute length
    int numAttr = GetNumAttrInUpdate(updateMessage);
    BOOL found = FALSE;
    int index = BgpFindAttr(updateMessage, BGP_MP_REACH_NLRI, numAttr, found);
    if (found)
    {
        if (bgp->ip_version == NETWORK_IPV4)
        {
            sizeOfmpNlri = updateMessage->pathAttribute[index].attributeLength;
            numBytesInMpReach = BgpCalculateSizeOfNlri(bgp,
                                sizeOfmpNlri,
                                (BgpRouteInfo*)updateMessage->
                                    pathAttribute[index].attributeValue);
            attr_length = updateMessage->totalPathAttributeLength -
                updateMessage->pathAttribute[index].attributeLength;
            attr_length += numBytesInMpReach;
        }
        else
        {
            numBytesInMpReach =
                updateMessage->pathAttribute[index].attributeLength;
        }
    }

    found = FALSE;
    index = BgpFindAttr(updateMessage, BGP_MP_UNREACH_NLRI, numAttr, found);
    if (found)
    {
        if (bgp->ip_version == NETWORK_IPV4)
        {
            sizeOfmpNlri = updateMessage->pathAttribute[index].attributeLength;
            numBytesInMpUnReach = BgpCalculateSizeOfNlri(bgp,
                                sizeOfmpNlri,
                                (BgpRouteInfo*)updateMessage->
                                    pathAttribute[index].attributeValue);
            attr_length = updateMessage->totalPathAttributeLength -
                    updateMessage->pathAttribute[index].attributeLength;
            attr_length += numBytesInMpUnReach;
        }
        else
        {
            numBytesInMpUnReach =
                updateMessage->pathAttribute[index].attributeLength;
        }
    }


    memcpy(tempPktPtr, &attr_length,
            BGP_PATH_ATTRIBUTE_LENGTH_SIZE);
    EXTERNAL_ntoh(
                (void*)tempPktPtr,
            BGP_PATH_ATTRIBUTE_LENGTH_SIZE);

    memcpy(&pathLength, &updateMessage->totalPathAttributeLength,
       BGP_PATH_ATTRIBUTE_LENGTH_SIZE);

    tempPktPtr += BGP_PATH_ATTRIBUTE_LENGTH_SIZE;

    // assign as attribute if the length is not 0

    while (pathLength)
    {
        // assign path attribute field of attribute
        memcpy(tempPktPtr, &updateMessage->pathAttribute[i],
           sizeof(unsigned char));

        tempPktPtr++;

        // assign attribute field of as attribute
        memcpy(tempPktPtr, &updateMessage->pathAttribute[i].attributeType,
           BGP_ATTRIBUTE_TYPE_SIZE);

        if (updateMessage->pathAttribute[i].attributeType ==
            BGP_PATH_ATTR_TYPE_ORIGIN)
        {

            // Allocate origin (type 1)
            tempPktPtr++;
            memcpy(tempPktPtr,
                &updateMessage->pathAttribute[i].attributeLength,
                BGP_ATTRIBUTE_LENGTH_SIZE);

            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_ATTRIBUTE_LENGTH_SIZE);
            tempPktPtr += BGP_ATTRIBUTE_LENGTH_SIZE ;

            // assign origin value
            memcpy(tempPktPtr, updateMessage->pathAttribute[i].
                attributeValue, updateMessage->pathAttribute[i].
                attributeLength);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                updateMessage->pathAttribute[i].attributeLength);
            i++;

            pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                 - sizeof(OriginValue));

            tempPktPtr++;

            // continue allocation as attribute if path length is not 0
            continue;
        }
        else if (updateMessage->pathAttribute[i].attributeType ==
         BGP_PATH_ATTR_TYPE_AS_PATH)
        {
            int lengthOfPath;

            // Allocate as path (type 2)
            tempPktPtr++;

            // assign path attribute length
            memcpy(tempPktPtr, &updateMessage->pathAttribute[i].
                attributeLength, BGP_ATTRIBUTE_LENGTH_SIZE);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_ATTRIBUTE_LENGTH_SIZE);

            lengthOfPath = updateMessage->pathAttribute[i].attributeLength;
            tempPktPtr += BGP_ATTRIBUTE_LENGTH_SIZE;

            // allocate as path
            BgpAllocateAsPath((AsPathValue*)
                 (updateMessage-> pathAttribute[i]).attributeValue,
                  tempPktPtr,
                  lengthOfPath);

            tempPktPtr += lengthOfPath;

            pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                 - lengthOfPath);
            i++;

            // continue allocation as attribute if path length is not 0
            continue;
        }
        else if (updateMessage->pathAttribute[i].attributeType ==
         BGP_PATH_ATTR_TYPE_NEXT_HOP)
        {
            // Allocate nextHop (type 3)
            tempPktPtr++;

            memcpy(tempPktPtr, &updateMessage->pathAttribute[i].
                attributeLength, BGP_ATTRIBUTE_LENGTH_SIZE);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_ATTRIBUTE_LENGTH_SIZE);
            tempPktPtr += BGP_ATTRIBUTE_LENGTH_SIZE;

            memcpy(tempPktPtr, updateMessage->pathAttribute[i].
                attributeValue, updateMessage->pathAttribute[i].
                attributeLength);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                updateMessage->pathAttribute[i].attributeLength);
            i++;

            pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                 - sizeof(NextHopValue));

            tempPktPtr += sizeof(NextHopValue);

            // continue allocation as attribute if path length is not 0
            continue;
        }
        else if (updateMessage->pathAttribute[i].attributeType ==
         BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC)
        {
            // Allocate multi exit disciminator (type 4)
            tempPktPtr++;
            memcpy(tempPktPtr, &updateMessage->pathAttribute[i].
                attributeLength, BGP_ATTRIBUTE_LENGTH_SIZE);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_ATTRIBUTE_LENGTH_SIZE);

            tempPktPtr += BGP_ATTRIBUTE_LENGTH_SIZE;

            memcpy(tempPktPtr, updateMessage->pathAttribute[i].
                attributeValue, updateMessage->pathAttribute[i].
                attributeLength);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                sizeof(MedValue));
            i++;

            pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                 - sizeof(MedValue));

            tempPktPtr += sizeof(MedValue);

            // continue allocation as attribute if path length is not 0
            continue;
        }
        else if (updateMessage->pathAttribute[i].attributeType ==
                 BGP_PATH_ATTR_TYPE_LOCAL_PREF)
        {

            // Allocate multi exit disciminator (type 4)
            tempPktPtr++;
            memcpy(tempPktPtr, &updateMessage->pathAttribute[i].
                attributeLength, BGP_ATTRIBUTE_LENGTH_SIZE);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_ATTRIBUTE_LENGTH_SIZE);
            tempPktPtr += BGP_ATTRIBUTE_LENGTH_SIZE;

            memcpy(tempPktPtr,
                updateMessage->pathAttribute[i].attributeValue,
                updateMessage->pathAttribute[i].attributeLength);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                sizeof(LocalPrefValue));
            i++;

            pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                - sizeof(LocalPrefValue));

            tempPktPtr += sizeof(LocalPrefValue);

            // continue allocation as attribute if path length is not 0
            continue;

        }
        else if (updateMessage->pathAttribute[i].attributeType ==
         BGP_PATH_ATTR_TYPE_ORIGINATOR_ID)
        {
            // Allocate originator ID (type 9)
            tempPktPtr++;

            memcpy(tempPktPtr, &updateMessage->pathAttribute[i].
                attributeLength, BGP_ATTRIBUTE_LENGTH_SIZE);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_ATTRIBUTE_LENGTH_SIZE);
            tempPktPtr += BGP_ATTRIBUTE_LENGTH_SIZE;

            memcpy(tempPktPtr, updateMessage->pathAttribute[i].
                attributeValue, updateMessage->pathAttribute[i].
                attributeLength);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                updateMessage->pathAttribute[i].attributeLength);
            i++;

            pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                 - sizeof(NodeAddress));

            tempPktPtr += sizeof(NodeAddress);

            // continue allocation as attribute if path length is not 0
            continue;
        }

        else if (updateMessage->pathAttribute[i].attributeType ==
            BGP_MP_REACH_NLRI)
        {
            tempPktPtr++;
            memcpy(tempPktPtr, &numBytesInMpReach, BGP_ATTRIBUTE_LENGTH_SIZE);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_ATTRIBUTE_LENGTH_SIZE);
            tempPktPtr += BGP_ATTRIBUTE_LENGTH_SIZE;
            MpReachNlri* mpReach = (MpReachNlri* )
                updateMessage->pathAttribute[i].attributeValue;


            //allocate (AFI,SubAFI,nextHopLen)
            memcpy(tempPktPtr, &mpReach->afIden, MBGP_AFI_SIZE);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                MBGP_AFI_SIZE);
            tempPktPtr += MBGP_AFI_SIZE ;

            memcpy(tempPktPtr,&mpReach->subAfIden,MBGP_SUB_AFI_SIZE);
            tempPktPtr += MBGP_SUB_AFI_SIZE;

            memcpy(tempPktPtr,&mpReach->nextHopLen,MBGP_NEXT_HOP_LEN_SIZE);
            tempPktPtr += MBGP_NEXT_HOP_LEN_SIZE;

            //allocate next hop
            memcpy(tempPktPtr, mpReach->nextHop, mpReach->nextHopLen);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                mpReach->nextHopLen);
            tempPktPtr += mpReach->nextHopLen;

            memcpy(tempPktPtr, &mpReach->numSnpa, MBGP_NUM_SNPA_SIZE);
            tempPktPtr += MBGP_NUM_SNPA_SIZE;
            //NO SNPA IS COPIED,mpReach->numSnpa has been set to zero.

            unsigned short sizeOfNlri =
                updateMessage->pathAttribute[i].attributeLength
                - MBGP_AFI_SIZE
                - MBGP_SUB_AFI_SIZE
                - MBGP_NEXT_HOP_LEN_SIZE
                - mpReach->nextHopLen
                - MBGP_NUM_SNPA_SIZE ;


            if (mpReach->afIden == MBGP_IPV4_AFI )
                numBytesInPrefix = 4;
            else if (mpReach->afIden == MBGP_IPV6_AFI )
                numBytesInPrefix = 16;

            BgpAllocateBgpRouteInfo(bgp, mpReach->Nlri,
                    tempPktPtr,
                    sizeOfNlri,
                    numBytesInPrefix,
                    totalLength);

            pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                - updateMessage->pathAttribute[i].attributeLength);

            tempPktPtr += sizeOfNlri;

            // continue allocation as attribute if path length is not 0
            i++;
            continue;
        }
        else if (updateMessage->pathAttribute[i].attributeType ==
            BGP_MP_UNREACH_NLRI)
        {
            tempPktPtr++;
            memcpy(tempPktPtr, &numBytesInMpUnReach,
                   BGP_ATTRIBUTE_LENGTH_SIZE);
            EXTERNAL_ntoh(
                (void*)tempPktPtr,
                BGP_ATTRIBUTE_LENGTH_SIZE);
            tempPktPtr += BGP_ATTRIBUTE_LENGTH_SIZE;
            MpUnReachNlri* mpUnreach = (MpUnReachNlri* )
                updateMessage->pathAttribute[i].attributeValue;
            //allocate (AFI,SubAFI)
            memcpy(tempPktPtr, &mpUnreach->afIden, MBGP_AFI_SIZE);
            tempPktPtr += MBGP_AFI_SIZE ;

            memcpy(tempPktPtr, &mpUnreach->subAfIden, MBGP_SUB_AFI_SIZE);
            tempPktPtr += MBGP_SUB_AFI_SIZE ;

            unsigned short unreachNlriSize =
                updateMessage->pathAttribute[i].attributeLength
                - MBGP_AFI_SIZE
                - MBGP_SUB_AFI_SIZE;

            if (mpUnreach->afIden == MBGP_IPV4_AFI )
                numBytesInPrefix = 4;
            else if (mpUnreach->afIden == MBGP_IPV6_AFI )
                numBytesInPrefix = 16;

            BgpAllocateBgpRouteInfo(bgp, mpUnreach->Nlri,
                    tempPktPtr,
                    unreachNlriSize,
                    numBytesInPrefix,
                    totalLength);
            tempPktPtr += unreachNlriSize;

            pathLength = (unsigned short)
                (pathLength - BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE
                - updateMessage->pathAttribute[i].attributeLength);
            i++;
        }

        else
        {
            ERROR_Assert(FALSE, "Unrecongnized as path attribute type\n");
        }
    }

    BgpAllocateBgpRouteInfo(bgp, updateMessage->networkLayerReachabilityInfo,
                            tempPktPtr,
                            noOctateInNLRI,
                            4,
                            totalLength);

    return sizeOfPacket;
}


//--------------------------------------------------------------------------
// FUNCTION  BgpSendUpdateMessage
//
// PURPOSE:  Function to send update message
//
//
// PARAMETERS:  node, the node which is sending update
//              bgp, the bgp internal structure
//              connectionStatus, The connection for which it is sending
//                                update
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpSendUpdateMessage(
    Node* node,
    BgpData* bgp,
    BgpConnectionInformationBase* connectionStatus)
{
    char* load = NULL;
    int sizeOfPacket = 0;
    unsigned short sizeOfNlri = 0;
    clocktype now = node->getNodeTime();


    BgpUpdateMessage* updateMessage = NULL;

    int numRouteEntries = BUFFER_GetCurrentSize(&connectionStatus->adjRibOut)
        / sizeof(BgpAdjRibLocStruct*);

    int numWithdrawnRoutes =
        BUFFER_GetCurrentSize(&connectionStatus->withdrawnRoutes)
            / sizeof(BgpRouteInfo);

    connectionStatus->updateTimerAlreadySet = FALSE;

    if (!numRouteEntries && !numWithdrawnRoutes)
    {
        return;
    }

    // Update the next time we can send UPDATE messages.
    if (!connectionStatus->isInternalConnection)
    {
        clocktype delay = (clocktype)
            ((0.75 * bgp->minRouteAdvertisementInterval_EBGP)
                    + RANDOM_erand (bgp->timerSeed)
                    * (0.25 * bgp->minRouteAdvertisementInterval_EBGP));

        // For EBGP
        connectionStatus->nextUpdate = now + delay;
    }
    else
    {
        clocktype delay = (clocktype)
            ((0.75 * bgp->minRouteAdvertisementInterval_IBGP)
                    + RANDOM_erand (bgp->timerSeed)
                    * (0.25 * bgp->minRouteAdvertisementInterval_IBGP));

        // For IBGP
        connectionStatus->nextUpdate = now + delay;
    }

    // Update will be send if there is any route to advertise or
    // there is a change in route in the previously advertised route
    if (connectionStatus->holdTime)
    {
        // If the hold time is not 0 then send one self timer to
        // send keep alive message. If the negotiated hold timer value is
        // 0 then no keep alive timer will be set
        Message* keepAliveTimer = NULL;
        clocktype delay = (clocktype)((0.75 * bgp->keepAliveInterval)
            + RANDOM_erand (bgp->timerSeed) * (0.25 * bgp->keepAliveInterval));

        connectionStatus->keepAliveTimerSeq++;

        keepAliveTimer = MESSAGE_Alloc(node, APP_LAYER,
                       APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                       MSG_APP_BGP_KeepAliveTimer);

        MESSAGE_InfoAlloc(node, keepAliveTimer, sizeof(ConnectionInfo));

        ((ConnectionInfo*) MESSAGE_ReturnInfo(keepAliveTimer))->uniqueId =
            connectionStatus->uniqueId;

        ((ConnectionInfo*) MESSAGE_ReturnInfo(keepAliveTimer))->sequence =
            connectionStatus->keepAliveTimerSeq;

        MESSAGE_Send(node, keepAliveTimer, delay);
    }

    // Allocate update message structure. This structure will be send to
    // BgpAllocateRoutes in packet to allocate the structure in flat
    // packet.

    if (DEBUG )
      printf("Node %u is Allocating packet to send UPDATE message\n",
             node->nodeId);


    if (DEBUG_TABLE)
    {
        char simTime[MAX_CLOCK_TYPE_STR_LEN];
        ctoa(node->getNodeTime(), simTime);
        printf("\nRib Local before update:\n");
        BgpPrintAdjRibLoc(node, bgp);
        printf("The entries in the ribOut before sending update: %s\n",
               simTime);

        BgpPrintAdjRibOut(node, connectionStatus);
        BgpPrintWithdrawnRoutes(connectionStatus);

    }



    updateMessage = BgpAllocateUpdateMessageStructure(node,
                              &sizeOfNlri,
                              connectionStatus);
    if (updateMessage)
    {
    //in MBGP must returen 0 for sizeOfNlri
    if (DEBUG_UPDATE)
    {
        char updateTime[MAX_CLOCK_TYPE_STR_LEN];
        ctoa(node->getNodeTime(), updateTime);

        printf("Node %u is sending update to %u sim-time %s ns\n"
               "The update message structure is:\n",
               node->nodeId,
               connectionStatus->remoteId,
               updateTime);

        BgpPrintStruct(node, updateMessage, bgp);
    }

    sizeOfPacket = BgpAllocateRoutesInPacket(bgp, updateMessage,
                         &load, sizeOfNlri);

    // Increment the statistical variable
    bgp->stats.updateSent++;

    if (DEBUG )
          printf("Node %u sent an UPDATE message with size %u \n",
                 node->nodeId,sizeOfPacket);

    // Sending update message to the peer
    Message *msg = APP_TcpCreateMessage(
        node,
        connectionStatus->connectionId,
        TRACE_BGP,
        IPTOS_PREC_INTERNETCONTROL);

    APP_AddPayload(
        node,
        msg,
        load,
        sizeOfPacket);

#ifdef ADDON_BOEINGFCS
    TransportToAppDataReceived appData;
    appData.connectionId = connectionStatus->connectionId;
    appData.priority = IPTOS_PREC_INTERNETCONTROL;
    APP_AddInfo(node, msg, 
        &appData,
        sizeof(TransportToAppDataReceived),
        INFO_TYPE_TransportToAppData);
#endif

    APP_TcpSend(node, msg);

    MEM_free(load);
        BgpFreeUpdateMessageStructure(updateMessage, sizeOfNlri);

    }
    // Free the update message structure as it is no longer necessary now




    // Check again if there is any unsent message if there then again
    // schedule an update
    numRouteEntries = BUFFER_GetCurrentSize(&(connectionStatus->adjRibOut))
        / sizeof(BgpRoutingInformationBase*);

    // Check again if there is any withdrawn route left, if there then
    // again schedule an update
    numWithdrawnRoutes =
        BUFFER_GetCurrentSize(&connectionStatus->withdrawnRoutes)
    / sizeof(BgpRouteInfo);

    if ((numRouteEntries) || (numWithdrawnRoutes))
    {
        // if any withdrawn route or unsent route is left schedule
        // an update message
        BgpSetUpdateTimer(node, connectionStatus);
        if (DEBUG)
        {
            printf("\tnode %d Setting timer to send update to %u\n",
               node->nodeId,connectionStatus->remoteId);
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION  BgpCheckForCollision
//
// PURPOSE:  Function to check if there is a connection collision
//
//
// PARAMETERS:  updateMsg, the update message pointer to free
//              lengthOfNlri, length of the Nlri field of the update
//                            message
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpCheckForCollision(
    Node* node,
    BgpData* bgp,
    NodeAddress remoteBgpIdentifier,
    BgpConnectionInformationBase* connectionOpenCame,
    BOOL* isCollision,
    unsigned short holdTime,
    unsigned short asId)
{
    int i = 0;

    int numEntries = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
        / sizeof(BgpConnectionInformationBase);

    BgpConnectionInformationBase* connectionStatus =
         (BgpConnectionInformationBase*)
            BUFFER_GetData(&(bgp->connInfoBase));

    *isCollision = FALSE;

    for (i = 0; i < numEntries; i++)
    {
        if (connectionStatus[i].state == BGP_OPEN_CONFIRM ||
            connectionStatus[i].state == BGP_ESTABLISHED)
        {
            if (connectionStatus[i].remoteBgpIdentifier ==
                remoteBgpIdentifier)
            {
                int connectionIdToDel;

                if (DEBUG_COLLISION)
                {
                    char time[MAX_STRING_LENGTH];

                    TIME_PrintClockInSecond(node->getNodeTime(), time);

                    printf("\tCollision between connectionid %d"
                           " and %d with node %u\n"
                           "Connection Information Base Before deletion\n",
                           connectionStatus[i].connectionId,
                           connectionOpenCame->connectionId,
                           connectionStatus[i].remoteId);

                    BgpPrintConnInformationBase(node, bgp);
                }

                if (connectionStatus[i].state != BGP_ESTABLISHED &&
                    connectionStatus[i].localBgpIdentifier <
                        remoteBgpIdentifier)
                {
                    // accept the new connection as the bgp identifier of the
                    // new open message is greater than the bgp identifier of
                    // the connection that is already there in my established
                    // list of connection.

                    connectionIdToDel = connectionStatus[i].connectionId;

                    if (DEBUG_COLLISION)
                    {
                        printf("node %d deleting existing connection\n",
                               node->nodeId);

                        if (connectionStatus[i].isInitiatingNode)
                        {
                            printf("\tDeleting active connection\n");
                        }
                        else
                        {
                            printf("\tDeleting passive connection\n");
                        }
                        printf("\tConnection id deleted %d\n",
                            connectionIdToDel);
                    }

                    BgpSendNotification(
                        node,
                        bgp,
                        &connectionStatus[i],
                        BGP_ERR_CODE_CEASE,
                        BGP_ERR_SUB_CODE_DEFAULT);

                    if (connectionOpenCame->weight ==
                        BGP_DEFAULT_EXTERNAL_WEIGHT)
                    {
                        connectionOpenCame->weight = connectionStatus[i].
                            weight;
                    }
                    connectionStatus[i].state = BGP_IDLE;
                    APP_TcpCloseConnection(node, connectionIdToDel);
                    BgpDeleteConnection(bgp, &connectionStatus[i]);

                    if (&connectionStatus[i] < connectionOpenCame)
                    {
                        connectionOpenCame--;
                    }
                    connectionOpenCame->remoteBgpIdentifier =
                        remoteBgpIdentifier;

                    connectionOpenCame->asIdOfPeer = asId;

                    if (asId == bgp->asId)
                    {
                        connectionOpenCame->isInternalConnection = TRUE;
                    }
                    else
                    {
                        connectionOpenCame->isInternalConnection = FALSE;
                    }

                    if (connectionOpenCame->holdTime / SECOND > holdTime)
                    {
                        connectionOpenCame->holdTime =
                            (clocktype)holdTime * SECOND;
                    }

                    BgpSendKeepAlivePackets(node,
                                            bgp,
                                            connectionOpenCame);

                    // We don't
                    // need to set keep alive and hold timer if the
                    // negotiated value for the hold time is zero

                    if (connectionOpenCame->holdTime)
                    {
                        Message* newMsg = NULL;
                        clocktype delay = (clocktype)
                            ((0.75 * bgp->keepAliveInterval)
                            + RANDOM_erand (bgp->timerSeed)
                            * (0.25 * bgp->keepAliveInterval));
                        // Set KeepAlive timer.
                        // After this keep alive timer expires we
                        // should again send another keep alive message
                        newMsg = MESSAGE_Alloc(
                                     node,
                                     APP_LAYER,
                                     APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                                     MSG_APP_BGP_KeepAliveTimer);

                        MESSAGE_InfoAlloc(node, newMsg,
                            sizeof(ConnectionInfo));

                        ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->
                            uniqueId = connectionOpenCame->uniqueId;

                        ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->
                            sequence = connectionOpenCame->keepAliveTimerSeq;

                        MESSAGE_Send(node, newMsg, delay);
                    }
                    else
                    {
                        // Cancel if any keepalive timer or hold timer if
                        // previously raised
                        connectionOpenCame->holdTimerSeq++;
                        connectionOpenCame->keepAliveTimerSeq++;
                    }

                    // Set state for this connection to open confirm.
                    connectionOpenCame->state = BGP_OPEN_CONFIRM;
                    if (DEBUG_COLLISION)
                    {
                        BgpPrintConnInformationBase(node, bgp);
                    }
                }
                else
                {
                    // Delete all the information and delete the connection
                    // for which the open message came and continue to use
                    // the previous connection
                    connectionIdToDel = connectionOpenCame->connectionId;

                    if (DEBUG_COLLISION)
                    {

                        printf("node %d deleting new incoming connection\n",
                               node->nodeId);

                        if (connectionOpenCame->isInitiatingNode)
                        {
                            printf("\tDeleting active connection\n");
                        }
                        else
                        {
                            printf("\tDeleting passive connection\n");
                        }
                        printf("\tConnection id Deleted %d\n",
                            connectionIdToDel);
                    }

                    BgpSendNotification(
                       node,
                       bgp,
                       connectionOpenCame,
                       BGP_ERR_CODE_CEASE,
                       BGP_ERR_SUB_CODE_DEFAULT);
                    connectionOpenCame->state = BGP_IDLE;

                    APP_TcpCloseConnection(node, connectionIdToDel);

                    if (connectionStatus[i].weight ==
                        BGP_DEFAULT_EXTERNAL_WEIGHT)
                    {
                        connectionStatus[i].weight =
                            connectionOpenCame->weight;
                    }

                    BgpDeleteConnection(bgp, connectionOpenCame);

                }

                *isCollision = TRUE;

                if (DEBUG_COLLISION)
                {
                    printf("Connection Information Base After deletion\n");
                    BgpPrintConnInformationBase(node, bgp);
                }
                break;
            }
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION BgpCheckIfOpenMessageError
//
// PURPOSE  Check error in the open Message
//
// PARAMETERS
//     node,
//     bgp, the bgp internal structure
//     data, The message to check for errors
//     connection,connection for which the event type is meant for
//
// RETURN   TRUE, If the open message is errornious
//          FALSE, Otherwise
//--------------------------------------------------------------------------

static
BOOL BgpCheckIfOpenMessageError(
     Node* node,
     BgpData* bgp,char* data,
     BgpConnectionInformationBase* connectionStatus)
{
    unsigned char version = 0;
    unsigned int asId = 0;
    unsigned short holdTime = 0;
    NodeAddress remoteBgpIdentifier = 0;
    char* tempPktPtr = data;
    tempPktPtr += BGP_MIN_HDR_LEN;

    memcpy(&version, tempPktPtr, sizeof(unsigned char));
    tempPktPtr += sizeof(unsigned char);

    // get BGP version from the open packet;
    if (version != BGPVERSION)
    {
        char packetErrorData = BGPVERSION;
        BgpSendNotification(node,
           bgp,
           connectionStatus,
           BGP_ERR_CODE_OPEN_MSG_ERR,
           BGP_ERR_SUB_CODE_OPEN_MSG_UNSUPPORTED_VERSION,
           &packetErrorData,
           sizeof(packetErrorData));
        APP_TcpCloseConnection(node, connectionStatus->connectionId);
        BgpCloseConnection(node, bgp, connectionStatus);

        return TRUE;
    }
    // get AS id from the open packet;
    memcpy(&asId, tempPktPtr, sizeof(unsigned short));
    tempPktPtr += sizeof(unsigned short);
    if ((asId > (unsigned) 65535) || (asId <= 0) ||
        ((connectionStatus->asIdOfPeer != 0 ) &&
        (asId != connectionStatus->asIdOfPeer)))
    {
        BgpSendNotification(node,
           bgp,
           connectionStatus,
           BGP_ERR_CODE_OPEN_MSG_ERR,
           BGP_ERR_SUB_CODE_OPEN_MSG_BAD_PEER);
        APP_TcpCloseConnection(node, connectionStatus->connectionId);
        BgpCloseConnection(node, bgp, connectionStatus);

        return TRUE;
    }
    // get the hole time from OPEN packet
    memcpy(&holdTime, tempPktPtr, BGP_HOLDTIMER_FIELD_LENGTH);
    tempPktPtr += BGP_HOLDTIMER_FIELD_LENGTH;
    if ((holdTime < 3) && (holdTime > 0))
    {
        BgpSendNotification(node,
           bgp,
           connectionStatus,
           BGP_ERR_CODE_OPEN_MSG_ERR,
           BGP_ERR_SUB_CODE_OPEN_MSG_UNACCEPTABLE_HOLD_TIME);
        APP_TcpCloseConnection(node, connectionStatus->connectionId);
        BgpCloseConnection(node, bgp, connectionStatus);

        return TRUE;
    }

    // get BGP Identifier from OPEN packet
    memcpy(&remoteBgpIdentifier, tempPktPtr, sizeof(NodeAddress));
    if (NetworkIpIsMulticastAddress(node, remoteBgpIdentifier) ||
        remoteBgpIdentifier == ANY_DEST)
    {
        BgpSendNotification(node,
            bgp,
            connectionStatus,
            BGP_ERR_CODE_OPEN_MSG_ERR,
            BGP_ERR_SUB_CODE_OPEN_MSG_BAD_BGP_IDENTIFIER);
        APP_TcpCloseConnection(node, connectionStatus->connectionId);
        BgpCloseConnection(node, bgp, connectionStatus);

        return TRUE;
    }

    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION BgpCheckIfMessageHeaderError
//
// PURPOSE  Check error in the Update Message
//
// PARAMETERS
//     node,
//     bgp, the bgp internal structure
//     data, The message to check for errors
//     connection,connection for which the event type is meant for
//
// RETURN   TRUE, If the open message is errornious
//          FALSE, Otherwise
//--------------------------------------------------------------------------

static
BOOL BgpCheckIfMessageHeaderError(
     Node* node,
     BgpData* bgp,char* data,
     BgpConnectionInformationBase* connectionStatus)
{
    unsigned short  expectedLengthOfPacket = 0;
    char markerValue[BGP_SIZE_OF_MARKER];
    char *mask;

    mask = (char*) MEM_malloc(BGP_SIZE_OF_MARKER);
    memset(mask,255,BGP_SIZE_OF_MARKER);

    memcpy(markerValue,
         data,
         BGP_SIZE_OF_MARKER);

    if (memcmp(markerValue,mask,BGP_SIZE_OF_MARKER) != 0)
    {
        BgpSendNotification(node,
           bgp,
           connectionStatus,
           BGP_ERR_CODE_MSG_HDR_ERR,
           BGP_ERR_SUB_CODE_MSG_HDR_CONNECTION_NOT_SYNCHRONIZED);
        APP_TcpCloseConnection(node,connectionStatus->connectionId);
        BgpCloseConnection(node, bgp, connectionStatus);
        return TRUE;
    }

    MEM_free(mask);

    memcpy(&expectedLengthOfPacket,(data + BGP_SIZE_OF_MARKER),
                                    BGP_MESSAGE_LENGTH_SIZE);
    if (( expectedLengthOfPacket >= BGP_MIN_HDR_LEN)
        &&(expectedLengthOfPacket <= BGP_MAX_HDR_LEN))
    {
        char type = *(data + BGP_SIZE_OF_MARKER + BGP_MESSAGE_LENGTH_SIZE);
        switch (type)
        {
            case BGP_OPEN:
            {
                if (expectedLengthOfPacket < BGP_SIZE_OF_OPEN_MSG_EXCL_OPT)
                {
                    BgpSendNotification(node,
                     bgp,
                     connectionStatus,
                     BGP_ERR_CODE_MSG_HDR_ERR,
                     BGP_ERR_SUB_CODE_MSG_HDR_BAD_MSG_LEN,
                     data + BGP_SIZE_OF_MARKER,
                     BGP_MESSAGE_LENGTH_SIZE);
                    APP_TcpCloseConnection(node,
                                           connectionStatus->connectionId);
                    BgpCloseConnection(node, bgp, connectionStatus);
                    return TRUE;
                }
                break;
            }
            case BGP_UPDATE:
            {
                if (expectedLengthOfPacket < BGP_SIZE_OF_UPDATE_MSG_EXCL_OPT)
                {
                    BgpSendNotification(node,
                      bgp,
                      connectionStatus,
                      BGP_ERR_CODE_MSG_HDR_ERR,
                      BGP_ERR_SUB_CODE_MSG_HDR_BAD_MSG_LEN,
                      data + BGP_SIZE_OF_MARKER,
                      BGP_MESSAGE_LENGTH_SIZE);
                    APP_TcpCloseConnection(node,
                                           connectionStatus->connectionId);
                    BgpCloseConnection(node, bgp, connectionStatus);
                    return TRUE;
                }
                break;
            }
            case BGP_KEEP_ALIVE:
            {
                if (expectedLengthOfPacket != BGP_MIN_HDR_LEN)
                {
                    BgpSendNotification(node,
                     bgp,
                     connectionStatus,
                     BGP_ERR_CODE_MSG_HDR_ERR,
                     BGP_ERR_SUB_CODE_MSG_HDR_BAD_MSG_LEN,
                     data + BGP_SIZE_OF_MARKER,
                     BGP_MESSAGE_LENGTH_SIZE);
                    APP_TcpCloseConnection(node,
                                           connectionStatus->connectionId);
                    BgpCloseConnection(node, bgp, connectionStatus);
                    return TRUE;
                }
                break;
            }
            case BGP_NOTIFICATION:
            {
                if ((expectedLengthOfPacket <
                   BGP_SIZE_OF_NOTIFICATION_MSG_EXCL_DATA))
                {
                    BgpSendNotification(node,
                    bgp,
                    connectionStatus,
                    BGP_ERR_CODE_MSG_HDR_ERR,
                    BGP_ERR_SUB_CODE_MSG_HDR_BAD_MSG_LEN,
                    data + BGP_SIZE_OF_MARKER,
                    BGP_MESSAGE_LENGTH_SIZE);
                    APP_TcpCloseConnection(node,
                                           connectionStatus->connectionId);
                    BgpCloseConnection(node, bgp, connectionStatus);
                    return TRUE;
                }
                break;
            }
            default:
            {
                BgpSendNotification(node,
                   bgp,
                   connectionStatus,
                   BGP_ERR_CODE_MSG_HDR_ERR,
                   BGP_ERR_SUB_CODE_MSG_HDR_BAD_MSG_TYPE,
                   data + BGP_SIZE_OF_MARKER + BGP_MESSAGE_LENGTH_SIZE,
                   BGP_MESSAGE_TYPE_SIZE);
                   APP_TcpCloseConnection(node,
                                          connectionStatus->connectionId);
                BgpCloseConnection(node, bgp, connectionStatus);
                return TRUE;
            }
        }
        return FALSE;
    }
    else
    {
        BgpSendNotification(node,
         bgp,
         connectionStatus,
         BGP_ERR_CODE_MSG_HDR_ERR,
         BGP_ERR_SUB_CODE_MSG_HDR_BAD_MSG_LEN,
         data + BGP_SIZE_OF_MARKER,
         BGP_MESSAGE_LENGTH_SIZE);
         APP_TcpCloseConnection(node, connectionStatus->connectionId);
        BgpCloseConnection(node, bgp, connectionStatus);
        return TRUE;
    }
}
//--------------------------------------------------------------------------
// NAME:        BgpSendOpenPacket
//
// PURPOSE:     Sending open packet to the bgp peer
//
//
// PARAMETERS:  bgp, the bgp internal structure
//              node, the node which is printing the information
//              uniqueId, The unique id of the connection for which the open
//                        msg will go.
//
// RETURN:      None
//
// ASSUMPTION:  None
//
//--------------------------------------------------------------------------

static
void BgpSendOpenPacket(
    Node* node,
    BgpData* bgp,
    int uniqueId)
{
    BgpConnectionInformationBase* connectionStatus =
        BgpGetConnInfoFromUniqueId(bgp, uniqueId);

    unsigned short holdTime = (unsigned short) (bgp->holdInterval / SECOND);

    char* payload = NULL;
    char* tempPayloadPtr = NULL;

    // Allocating space for the open packet
    payload = (char*) MEM_malloc(BGP_SIZE_OF_OPEN_MSG_EXCL_OPT);
    tempPayloadPtr = payload;

    // Initialising members for commonheader
    memset(payload, 255, BGP_SIZE_OF_MARKER);
    tempPayloadPtr += BGP_SIZE_OF_MARKER;

    *(unsigned short*) tempPayloadPtr = BGP_SIZE_OF_OPEN_MSG_EXCL_OPT;
    tempPayloadPtr += BGP_MESSAGE_LENGTH_SIZE;

    *tempPayloadPtr = BGP_OPEN;
    tempPayloadPtr++;

    *tempPayloadPtr = BGPVERSION;
    tempPayloadPtr++;

    memcpy(tempPayloadPtr, &bgp->asId, BGP_ASID_FIELD_LENGTH);
    tempPayloadPtr += BGP_ASID_FIELD_LENGTH;

    memcpy(tempPayloadPtr, &holdTime, BGP_HOLDTIMER_FIELD_LENGTH);
    tempPayloadPtr += BGP_HOLDTIMER_FIELD_LENGTH;

    memcpy(tempPayloadPtr, &connectionStatus->localBgpIdentifier,
           sizeof(NodeAddress));

    tempPayloadPtr += sizeof(NodeAddress);

    // Assuming that there is no optional field for now
    *tempPayloadPtr = 0;
    BgpSwapBytes((unsigned char*)payload,
                 FALSE);
    // Increment the statistical variable
    bgp->stats.openMsgSent++;

    // Send the open packet to tcp
    Message* msg = APP_TcpCreateMessage(
        node,
        connectionStatus->connectionId,
        TRACE_BGP,
        IPTOS_PREC_INTERNETCONTROL);

    APP_AddPayload(
        node,
        msg,
        payload,
        BGP_SIZE_OF_OPEN_MSG_EXCL_OPT);

#ifdef ADDON_BOEINGFCS
    TransportToAppDataReceived appData;
    appData.connectionId = connectionStatus->connectionId;
    appData.priority = IPTOS_PREC_INTERNETCONTROL;
    APP_AddInfo(node, msg, 
        &appData,
        sizeof(TransportToAppDataReceived),
        INFO_TYPE_TransportToAppData);
#endif

    APP_TcpSend(node, msg);

    MEM_free(payload);
}


//--------------------------------------------------------------------------
// FUNCTION  BgpCopyRtToRibOut
//
// PURPOSE:  Function to copy the routing table to each connection
//
//
// PARAMETERS:  bgp, the bgp internal structure
//              adjRibOut, pointer to ribOut
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpCopyRtToRibOut(
    BgpData* bgp,
    DataBuffer* adjRibOut,
    BgpConnectionInformationBase* connPtr)
{
    int i = 0;

    int numEntries = BUFFER_GetCurrentSize(&(bgp->ribLocal))
        / sizeof(BgpAdjRibLocStruct);

    BgpAdjRibLocStruct* ribLocal = (BgpAdjRibLocStruct*)
        BUFFER_GetData(&(bgp->ribLocal));

    for (i = 0; i < numEntries; i++)
    {
        //do not advertise a route to an internal peer, which
        //has been learned from another internal peer


        if (!(bgp->isRtReflector) &&
            (connPtr->isInternalConnection &&
            (connPtr->asIdOfPeer ==
             ribLocal[i].ptrAdjRibIn->asIdOfPeer)))
        {
                continue;
        }

        //do not advertise a route to a RR non client peer, which
        //has been learned from another RR non client peer
        else if (bgp->isRtReflector &&
            (connPtr->isInternalConnection &&
            !connPtr->isClient) &&
            (connPtr->asIdOfPeer ==
             ribLocal[i].ptrAdjRibIn->asIdOfPeer)&&
            !ribLocal[i].ptrAdjRibIn->isClient )
        {
            if (DEBUG)
            {
                printf("Do not advertise a route to a RR peer\n");
            }
            continue;
        }
        //do not advertise route to the peer from whom the route
        //has been learned.
        if (Address_IsSameAddress(&connPtr->remoteAddr,
              &ribLocal[i].ptrAdjRibIn->peerAddress))
        {
            if (DEBUG)
            {
                printf("Do not advertise route to the peer from whom"
                    "the route has been learnt\n");
                char addrStr1[20],addrStr2[20];
                IO_ConvertIpAddressToString(&connPtr->remoteAddr,
                                            addrStr1);
                IO_ConvertIpAddressToString(
                                   &ribLocal[i].ptrAdjRibIn->peerAddress,
                                   addrStr2);
                printf("Peer in conn info %s,In RibLocal %s\n",addrStr1,
                                                               addrStr2);
            }
            continue;
        }
        //peer belongs to the same network? then keep yourself
        //stop from route advertising.
        NetworkType remoteType = connPtr->remoteAddr.networkType;
        NetworkType netType =
            ribLocal[i].ptrAdjRibIn->destAddress.prefix.networkType;

        if (remoteType != netType )
        {
            continue;
        }

        if (remoteType == NETWORK_IPV4 )
        {
            NodeAddress remoteAddress =
            GetIPv4Address(connPtr->remoteAddr);
            NodeAddress ipAddress =
            GetIPv4Address(ribLocal[i].ptrAdjRibIn->destAddress.prefix);

            if (IsIpAddressInSubnet( remoteAddress,
                  ipAddress,
                  32 - ribLocal[i].ptrAdjRibIn->destAddress.prefixLen ) )
            {
                continue;
            }
        }

        if (remoteType == NETWORK_IPV6 )
        {
            in6_addr remoteAddress =
              GetIPv6Address(connPtr->remoteAddr);
            in6_addr ipAddress =
              GetIPv6Address(ribLocal[i].ptrAdjRibIn->destAddress.prefix);

            //unsigned int tla,nla,sla;
            unsigned int prefixLen =
                ribLocal[i].ptrAdjRibIn->destAddress.prefixLen;

            if (Ipv6IsAddressInNetwork(&remoteAddress,&ipAddress,prefixLen) )
            {
                continue;
            }
        }//if (remoteType == NETWORK_IPV6 )

        if (ribLocal[i].isValid)
        {
            BgpAdjRibLocStruct* ribLocalPtr;
            ribLocalPtr = &(ribLocal[i]);

            BUFFER_AddDataToDataBuffer(adjRibOut,
                             (char*) &ribLocalPtr,
                             sizeof(BgpAdjRibLocStruct*));

            adjRibOut = &(connPtr->adjRibOut);

            ribLocal[i].movedToAdjRibOut = TRUE;
        }
    }
}

//--------------------------------------------------------------------------
// FUNCTION  GetNumAttr
//
// PURPOSE : To calculate the total no of attributes in the path attribute
//           field of update message
//
// PARAMETERS:  attrLength, Total length of the path attribute field of the
//                          update message.
//              packetPtr,  the raw data byte now pointing to the path
//                          attribute field
//
// RETURN:      return the no of attribute
//
// ASSUMPTION:  though it has been calculated, for further extension
//              compatibility, now it will return asways 3. as there is
//                             existance of one of each type of attribute
//--------------------------------------------------------------------------
static
int GetNumAttr(int attrLength, char* packetPtr)
{
    int numEntry = 0;
    BOOL is_extend_len;
    int total_attr_len = 0;
    int attr_length = 0;
    while (attrLength)
    {
        is_extend_len = BgpPathAttributeGetExtndLen((char)packetPtr[0]);
        if (is_extend_len)
        {
            total_attr_len = 4;
            attr_length = 2;
        }
        else
        {
            total_attr_len = 3;
            attr_length = 1;
        }
        // skip attribute flag, pointing to type
        packetPtr++;

        // Type Origin
        if (*packetPtr == BGP_PATH_ATTR_TYPE_ORIGIN)
        {
            // the type value is origin so increment the no of entry and
            // increment the temp pointer to the next attribute
            numEntry++;

            // length of origin attribute is type + length + 1 byte = 4
            packetPtr += (BGP_ATTRIBUTE_TYPE_SIZE + attr_length
                    + sizeof(OriginValue));

            // PathAttributeValue size + 1
            attrLength -= (total_attr_len
                    + sizeof(OriginValue));
        }
        // Type next hop
        else if (*packetPtr == BGP_PATH_ATTR_TYPE_NEXT_HOP)
        {
            // the type value is next hop so increment the no of entry and
            // increment the temp pointer to the next attribute
            numEntry++;

            // length of next attribute is type + length + 4 = 7
            packetPtr += (BGP_ATTRIBUTE_TYPE_SIZE + attr_length
                    + sizeof(NextHopValue));

            // PathAttributeValue size + next hop size
            attrLength -= (total_attr_len
                    + sizeof(NextHopValue));
        }

        // Type AS path
        else if (*packetPtr == BGP_PATH_ATTR_TYPE_AS_PATH)
        {
            unsigned short lengthOfAttributeValue = 0;
            numEntry++;

            // the next byte contains the length of as path attribute
            memcpy(&lengthOfAttributeValue, (packetPtr + 1),
                    attr_length);
            EXTERNAL_ntoh(
                (void*)&lengthOfAttributeValue,
                attr_length);

            // length of AS attribute is type + length + attribute value length
            packetPtr += (BGP_ATTRIBUTE_TYPE_SIZE + attr_length
                    + lengthOfAttributeValue);

            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
                    + lengthOfAttributeValue);
        }

        // Type Multi Exit Disc
        else if (*packetPtr == BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC)
        {
            unsigned short lengthOfAttributeValue = 0;

            numEntry++;

            // the next byte contains the length of as path attribute
            memcpy(&lengthOfAttributeValue, (packetPtr + 1),
                    attr_length);
            EXTERNAL_ntoh(
                (void*)&lengthOfAttributeValue,
                attr_length);
            // length of MED attribute is type + length + attr value length
            packetPtr += (BGP_ATTRIBUTE_TYPE_SIZE + attr_length
                    + lengthOfAttributeValue);

            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
                    + lengthOfAttributeValue);
        }

        // Type Local Preference
        else if (*packetPtr == BGP_PATH_ATTR_TYPE_LOCAL_PREF)
        {
            unsigned short lengthOfAttributeValue = 0;

            numEntry++;

            // the next byte contains the length of as path attribute
            memcpy(&lengthOfAttributeValue, (packetPtr + 1),
                    attr_length);
            EXTERNAL_ntoh(
                (void*)&lengthOfAttributeValue,
                attr_length);
            // length of loc pref attr is type + length + attr value length
            packetPtr += (BGP_ATTRIBUTE_TYPE_SIZE + attr_length
                    + lengthOfAttributeValue);

            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
                    + lengthOfAttributeValue);
        }
        else if (*packetPtr == BGP_PATH_ATTR_TYPE_ORIGINATOR_ID)
        {
            // the type value is originator ID so increment the no of entry and
            // increment the temp pointer to the next attribute
            numEntry++;

            // length of originator ID attribute is type + length + 4 = 7
            packetPtr += (BGP_ATTRIBUTE_TYPE_SIZE + attr_length
                    + sizeof(NodeAddress));

            // PathAttributeValue size + next hop size
            attrLength -= (total_attr_len
                    + sizeof(NodeAddress));
        }

    else if (*packetPtr == BGP_MP_REACH_NLRI)
    {
        unsigned short lengthOfAttributeValue = 0;
        numEntry++;

        // the next byte contains the length of attribute
            memcpy(&lengthOfAttributeValue, (packetPtr + 1),
           attr_length);
           EXTERNAL_ntoh(
                (void*)&lengthOfAttributeValue,
                attr_length);
        // type + length + attr value length
            packetPtr += (BGP_ATTRIBUTE_TYPE_SIZE + attr_length
              + lengthOfAttributeValue);

            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
               + lengthOfAttributeValue);

    }
    else if (*packetPtr == BGP_MP_UNREACH_NLRI)
    {
        unsigned short lengthOfAttributeValue = 0;
        numEntry++;

            // the next byte contains the length of attribute
            memcpy(&lengthOfAttributeValue, (packetPtr + 1),
           attr_length);
           EXTERNAL_ntoh(
                (void*)&lengthOfAttributeValue,
                attr_length);
        // type + length + attr value length
            packetPtr += (BGP_ATTRIBUTE_TYPE_SIZE + attr_length
              + lengthOfAttributeValue);

            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
               + lengthOfAttributeValue);
    }
        // If there are any other type of entries are added later this
        // portion should be appended accordingly
        else
        {
            unsigned short lengthOfAttributeValue = 0;
            numEntry++;
            //Pass through the optional non-transitive attrs
            // the next byte contains the length of as path attribute
            memcpy(&lengthOfAttributeValue, (packetPtr + 1),
                    attr_length);
            EXTERNAL_ntoh(
                (void*)&lengthOfAttributeValue,
                attr_length);

            // length of AS attribute is type + length + attribute value length
            packetPtr += (BGP_ATTRIBUTE_TYPE_SIZE + attr_length
                    + lengthOfAttributeValue);

            // PathAttributeValue size + attribute value length
            attrLength -= (total_attr_len
                    + lengthOfAttributeValue);
        }
        // Continue until the end of all the attributes
    }
    return numEntry;
}


//--------------------------------------------------------------------------
// FUNCTION  BgpCalculateNoRoute
//
// PURPOSE : To calculate the no of routes in withdrawn route field or
//           in Nlri field of packet
//
// PARAMETERS:  char* packet, the raw byte of data, pointing to the withdrawn
//                            route field or in the Nlri field
//              routeLength,  the total length of the routeInfo field
//
// RETURN:      no of routes
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------
static
int BgpCalculateNoRoute(int noBytesInOneRoute, int routeLength)
{
    int noRoute = 0;

    while (routeLength >0)
    {

        noRoute++;
        routeLength -= noBytesInOneRoute + 1;
    }
    return noRoute;
}


//--------------------------------------------------------------------------
// FUNCTION  BgpCalculateNoRoute
//
// PURPOSE : To calculate the no of routes in withdrawn route field or
//           in Nlri field of packet
//
// PARAMETERS:  char* packet, the raw byte of data, pointing to the withdrawn
//                            route field or in the Nlri field
//              routeLength,  the total length of the routeInfo field
//
// RETURN:      no of routes
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------
static
int BgpCalculateNoRoutes(char* packet, int routeLength)
{
    int noRoute = 0, numBytesInPrefix = 0, length = 0;
    char prefixLen;
    while (routeLength)
    {
        prefixLen = *packet;
        packet = packet + sizeof(char);
        length = prefixLen % 8;
        if (length == 0)
        {
            numBytesInPrefix = (prefixLen / 8);
        }
        else
        {
            numBytesInPrefix = (prefixLen / 8) + 1;
        }
        packet += numBytesInPrefix;
        routeLength = routeLength - (numBytesInPrefix + 1);
        noRoute++;
    }

    return noRoute;
}

static
void BgpAllocateRoutes(
    BgpData* bgp,
    BgpRouteInfo** routeInfo,
    char* packet,
    int routeLength,
    int* numNlri)
{
    int numRt = 0;
    int i = 0,numBytesInPrefix = 0;
    int length = 0;
    BgpRouteInfo* rtInfo = NULL;
    GenericAddress temp;
    NodeAddress ipAddress = 0;
    // Calculate the no of routes in the packets withdrawn route field or
    // in the Nlri field

    if (bgp->ip_version == NETWORK_IPV6)
    {
        numBytesInPrefix = 16;
        numRt = BgpCalculateNoRoute(numBytesInPrefix, routeLength);
        (*routeInfo) = NULL;


        if (numRt)
        {
            // If there is any route information then allocate the space to
            // store them in the update message structure
            rtInfo = (BgpRouteInfo*) MEM_malloc(sizeof(BgpRouteInfo) * numRt);

            // Now allocate the routes in the structure
            for (i = 0; i < numRt; i++)
            {
                rtInfo[i].prefixLen = *packet;
                packet = packet + sizeof(char);

                EXTERNAL_ntoh(
                    (void*)packet,
                    numBytesInPrefix);
                memcpy( (char* )&temp, packet, numBytesInPrefix );
                SetIPv6AddressInfo(&rtInfo[i].prefix,temp.ipv6);
                packet += numBytesInPrefix;
            }
            *routeInfo = rtInfo;
        }
        return;
    }
    numRt = BgpCalculateNoRoutes(packet, routeLength);
    *numNlri = numRt;
    (*routeInfo) = NULL;
    memset((void*)&temp , 0 , sizeof(GenericAddress));

    if (numRt)
    {
        // If there is any route information then allocate the space to
        // store them in the update message structure
        rtInfo = (BgpRouteInfo*) MEM_malloc(sizeof(BgpRouteInfo) * numRt);

        // Now allocate the routes in the structure
        while (routeLength)
        {
            ipAddress = 0;
            rtInfo[i].prefixLen = *packet;
            packet = packet + sizeof(char);
            length = (rtInfo[i].prefixLen) % 8;
            if (length == 0)
            {
                numBytesInPrefix = (rtInfo[i].prefixLen / 8);
            }
            else
            {
                numBytesInPrefix = (rtInfo[i].prefixLen / 8) + 1;
            }
            EXTERNAL_ntoh(
                    (void*)packet,
                    numBytesInPrefix);
            memcpy( (char* )&temp, packet, numBytesInPrefix );

            if (numBytesInPrefix == 1)
            {
                ipAddress = ipAddress + ((temp.ipv4 | 0xffffff00) << 24);
            }
            else if (numBytesInPrefix == 2)
            {
                ipAddress = ipAddress + ((temp.ipv4 | 0xffff0000) << 16);
            }
            else if (numBytesInPrefix == 3)
            {
                ipAddress = ipAddress + ((temp.ipv4 | 0xff000000) << 8);
            }
            else if (numBytesInPrefix == 4)
            {
                ipAddress = ipAddress + temp.ipv4;
            }
            if (numBytesInPrefix <= 4)
                SetIPv4AddressInfo(&rtInfo[i].prefix,ipAddress);
            else
                SetIPv6AddressInfo(&rtInfo[i].prefix,temp.ipv6);


            packet += numBytesInPrefix;
            routeLength = routeLength - (numBytesInPrefix + 1);
            i++;
        }
        *routeInfo = rtInfo;
    }
    return;
}

//--------------------------------------------------------------------------
// FUNCTION : BgpCheckBestRoute
//
// PURPOSE  : Accepts an incoming route and examines the adjRibIn
//            to determine whether the incoming route is best or not.
//
// RETURN TYPE : BOOL, returns TRUE if incoming route is best, FALES
//               otherwise.
//--------------------------------------------------------------------------

static
void BgpCheckBestRoute(
    Node* node,
    BgpData* bgp,
    BgpUpdateMessage* updateMessage,
    BgpRoutingInformationBase* routeToCheck,
    BgpRoutingInformationBase* prevBestRt,
    BgpConnectionInformationBase* connectionStatus,
    BgpRoutingInformationBase** bestRtPtr,
    BOOL isWithdrawnRt)
{
    // According to CISCO:
    // ------------------
    // 1. If the next hop is inaccessible, the route is ignored.
    // 2. Prefer the path with the largest weight.
    // 3. If weights are the same, prefer the route with the largest local
    //    preference value.
    // 4. If no locally originated routes and local preference is the same
    //    prefer the route with the shortest AS_PATH.
    // 5. If AS_PATH length the same, prefer route with lowest origin type.
    // 6. If origin type is the same, prefer route with lowest MED value
    //    if routes were received from the same AS.
    // 7. If same MED value, prefer EBGP paths to IGP paths.
    // 8. If all the preceding scenarios are identical, prefer the route that
    //    can be reached via the close IGP neighbor (follow the shortest
    //    path to the BGP NEXT_HOP).
    // 9. If internal path is the same, prefer router with the lowest
    //    ID (in CISCO, ID is the highest IP address on the router).
    BgpPathAttribute *pathAttribute = NULL;
    LocalPrefValue   localPref = 0;
    MedValue         multiExitDisc = 0;
    BOOL found = FALSE;

    if (DEBUG)
    {
        printf("In BgpCheckBestRoute\n");
    }
    unsigned int pathSegmentLength = 0;
    unsigned short* pathSegmentValue = NULL;
    char originCode;
    //MBGP vars
    int peerCost = 0,index;
    int remoteCost = 0;
    char peerAddr[20];

    ERROR_Assert(routeToCheck, "Invalid RIB structure\n");

    if (isWithdrawnRt)
    {
        ERROR_Assert(prevBestRt, "Invalid RIB entry\n");
        if (prevBestRt->asPathList)
        {
            pathSegmentLength = prevBestRt->asPathList->pathSegmentLength;
        }
        originCode = prevBestRt->originType;
        localPref  = prevBestRt->localPref;
        multiExitDisc = prevBestRt->multiExitDisc;
    }
    else
    {
        ERROR_Assert(updateMessage, "Invalid update message\n");
        ERROR_Assert(connectionStatus, "Invalid RIB structure\n");

        pathAttribute = (BgpPathAttribute*) updateMessage->pathAttribute;
        if (pathAttribute[1].attributeLength)
        {
            pathSegmentLength = ((BgpPathAttributeValue*)
                 ( pathAttribute[1].attributeValue ) )->pathSegmentLength;

            pathSegmentValue = ((BgpPathAttributeValue*)
                 ( pathAttribute[1].attributeValue ) )->pathSegmentValue;
        }
        int numAttr = GetNumAttrInUpdate(updateMessage);
        BOOL found;
        //In MBGP update message,local pref is the second attr.
        if (connectionStatus->isInternalConnection)
        {
            index = BgpFindAttr(updateMessage,
                                BGP_PATH_ATTR_TYPE_LOCAL_PREF,
                                numAttr,
                                found);
            if (found)
            {
                localPref =
                    *((LocalPrefValue*) pathAttribute[index].attributeValue);
            }
            index = BgpFindAttr(updateMessage,
                                BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC,
                                numAttr,
                                found);
            if (found)
            {
                multiExitDisc =
                    *((MedValue*) pathAttribute[index].attributeValue);
            }
        }
        else
        {
            index = BgpFindAttr(updateMessage,
                                BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC,
                                numAttr,
                                found);
            if (found)
            {
                multiExitDisc =
                    *((MedValue*) pathAttribute[index].attributeValue);
            }
            localPref = bgp->localPref;
        }

        originCode = (char) *pathAttribute[0].attributeValue;

    }
    // The entry with bigger weight is better.
    if (routeToCheck->origin == BGP_ORIGIN_IGP)
    {
        // For internal routes the local as has been included in the
        // routing information base. So assume the length 0
        pathSegmentLength = 0;
    }

    if (routeToCheck->pathAttrBest == BGP_PATH_ATTR_BEST_TRUE)
    {
        routeToCheck->pathAttrBest = BGP_PATH_ATTR_BEST_FALSE;
        if (routeToCheck->weight > connectionStatus->weight)
        {
            if (DEBUG_UPDATE)
            {
                printf("Prev Route is best.\n"
                       "\tWeight of Prev route = %u\n"
                       "\tWeight of New route = %u\n",
                       (unsigned) routeToCheck->weight,
                       (unsigned) connectionStatus->weight);
            }
            *bestRtPtr = routeToCheck;
            return;
        }

        if (routeToCheck->weight < connectionStatus->weight)
        {
            if (DEBUG_UPDATE)
            {
                printf("New Route is best.\n"
                       "\tWeight of Prev route = %u\n"
                       "\tWeight of New route = %u\n",
                       routeToCheck->weight,
                       connectionStatus->weight);
            }
            return;
        }

        // Only the local pref will be checked in case of new update and
        // if the connection is internal connection
        if ((updateMessage == NULL) || ((updateMessage != NULL) &&
            (connectionStatus->isInternalConnection)))
        {
            if (routeToCheck->localPref > localPref)
            {
                if (DEBUG_UPDATE)
                {
                    printf("Prev Route is best.\n"
                           "\tLocalPref: Prev route = %u\n"
                           "\tLocalPref: New route = %u\n",
                           routeToCheck->localPref,
                           localPref);
                }
                *bestRtPtr = routeToCheck;
                return;
            }
            if (routeToCheck->localPref < localPref)
            {
                if (DEBUG_UPDATE)
                {
                    printf("New Route is best.\n"
                           "\tLocalPref: Prev route = %u\n"
                           "\tLocalPref: New route = %u\n",
                           routeToCheck->localPref,
                           localPref);
                }
                return;
            }
        }
        if ((routeToCheck->asPathList)
            && (routeToCheck->asPathList->pathSegmentLength <
            pathSegmentLength * sizeof(unsigned short)))
        {
            if (DEBUG_UPDATE)
            {
                printf("Prev Route is best.\n"
                       "\tPath Length of Prev route = %u\n"
                       "\tPath Length of New route = %" TYPES_SIZEOFMFT "u\n",
                       routeToCheck->asPathList->pathSegmentLength,
                       pathSegmentLength * sizeof(unsigned short));
            }

            *bestRtPtr = routeToCheck;
            return;
        }
        if ((routeToCheck->asPathList)
            && (routeToCheck->asPathList->pathSegmentLength >
            pathSegmentLength * sizeof(unsigned short)))
        {
            if (DEBUG_UPDATE)
            {
                printf("New Route is best.\n"
                       "\tPath Length of Prev route = %u\n"
                       "\tPath Length of New route = %" TYPES_SIZEOFMFT "u\n",
                       routeToCheck->asPathList->pathSegmentLength,
                       pathSegmentLength * sizeof(unsigned short));
            }
            return;
        }
        if (routeToCheck->originType < originCode)
        {
            if (DEBUG_UPDATE)
            {
                printf("Prev Route is best.\n"
                       "\tOrigin Code of Prev route = %u\n"
                       "\tOrigin Code of New route = %u\n",
                       routeToCheck->originType,
                       originCode);
            }
            *bestRtPtr = routeToCheck;
            return;
        }
        if (routeToCheck->originType > originCode)
        {
            if (DEBUG_UPDATE)
            {
                printf("New Route is best.\n"
                       "\tOrigin Code of Prev route = %u\n"
                       "\tOrigin Code of New route = %u\n",
                       routeToCheck->originType,
                       originCode);
            }
            return;
        }
        // Check the MEDS if update received from same AS as the previous route

        if ((updateMessage == NULL) || ((updateMessage != NULL) &&
            (pathSegmentValue != NULL) && (routeToCheck->asPathList) &&
            (!memcmp(routeToCheck->asPathList->pathSegmentValue,
                     pathSegmentValue,
                     routeToCheck->asPathList->pathSegmentLength))))
        {
            if (routeToCheck->multiExitDisc < multiExitDisc)
            {
                if (DEBUG_UPDATE)
                {
                    printf("Prev Route is best.\n"
                           "\tMED: Prev route = %u\n"
                           "\tMED: New route = %u\n",
                           routeToCheck->multiExitDisc,
                           (unsigned) multiExitDisc);
                }
                *bestRtPtr = routeToCheck;
                return;
            }
            if (routeToCheck->multiExitDisc > multiExitDisc)
            {
                if (DEBUG_UPDATE)
                {
                    printf("New Route is best.\n"
                           "\tMED: Prev route = %u\n"
                           "\tMED: New route = %u\n",
                           routeToCheck->multiExitDisc,
                           (unsigned) multiExitDisc);
                }
                return;
            }
        }

        // Prefer EBGP path over IGP path
        if ((routeToCheck->asIdOfPeer != bgp->asId) &&
            (connectionStatus->isInternalConnection))
        {
            *bestRtPtr = routeToCheck;
            return;
        }
        else if ((routeToCheck->asIdOfPeer == bgp->asId) &&
                 (!connectionStatus->isInternalConnection))
        {
            return;
        }

        // Prefer closest IGP neighbor
        //this pice is modified for MBGP

        //check these parts later
        if (routeToCheck->peerAddress.networkType == NETWORK_IPV4 )
            peerCost = NetworkGetMetricForDestAddress(node,
                                  GetIPv4Address(routeToCheck->peerAddress));
        else if (routeToCheck->peerAddress.networkType == NETWORK_IPV6 )
            //check point,Ipv6X function is not tested yet?
            peerCost = Ipv6GetMetricForDestAddress(node,
                               GetIPv6Address(routeToCheck->peerAddress) );

        if (connectionStatus->remoteAddr.networkType == NETWORK_IPV4)
            remoteCost = NetworkGetMetricForDestAddress(node,
                                GetIPv4Address(connectionStatus->remoteAddr));
        else if (connectionStatus->remoteAddr.networkType == NETWORK_IPV6)
            //check point,Ipv6X function is not tested yet?
            remoteCost = Ipv6GetMetricForDestAddress(node,
                                 GetIPv6Address(connectionStatus->remoteAddr));

        if (peerCost < remoteCost)
        {
            *bestRtPtr = routeToCheck;
            return;
        }
        else if (peerCost > remoteCost)
        {
            return;
        }

        int numAttr = GetNumAttrInUpdate(updateMessage);
        index = BgpFindAttr(updateMessage,
                            BGP_PATH_ATTR_TYPE_ORIGINATOR_ID,
                            numAttr,
                            found);

        NodeAddress rtTocheckBgpId;
        NodeAddress connectionBgpId;

        if (!found)
        {
            connectionBgpId = connectionStatus->remoteBgpIdentifier;
        }
        else
        {
            pathAttribute = (BgpPathAttribute*) updateMessage->pathAttribute;
            memcpy (&connectionBgpId,
                    pathAttribute[index].attributeValue,
                    sizeof (NodeAddress));
        }
        if (routeToCheck->originatorId)
        {
            rtTocheckBgpId = routeToCheck->originatorId;
        }
        else
        {
            rtTocheckBgpId = routeToCheck->bgpIdentifier;
        }


        if (rtTocheckBgpId < connectionBgpId)
        {
            IO_ConvertIpAddressToString(&(routeToCheck->peerAddress),
                peerAddr);
            if (DEBUG_UPDATE)
            {
                printf("Prev Route is best.\n"
                       "\tPeer Address of Prev route = %s\n"
                       "\tPeer Address of New route = %u\n",
                       peerAddr,
                       connectionStatus->remoteBgpIdentifier);
            }
            *bestRtPtr = routeToCheck;
            return;
        }
        if (rtTocheckBgpId > connectionBgpId)
        {
            if (DEBUG_UPDATE)
            {
                IO_ConvertIpAddressToString(&(routeToCheck->peerAddress),
                peerAddr);
                printf("New Route is best.\n"
                       "\tPeer Address of Prev route = %s\n"
                       "\tPeer Address of New route = %u\n",
                       peerAddr,
                       connectionStatus->remoteBgpIdentifier);
            }
            return;
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION : BgpModifyAdjRibInForNewUpdate
//
// PURPOSE  : Modifying the input routing table for a new route in the
//            update message
//
// PARAMETERS : node, the node which has received the update message
//              bgp,  bgp internal structure
//              nlri, Network Layer Reachability information for one route
//              updateMessage, pointer to the update message just came
//              connectionPtr, connection which has received the update
//
// RETURN TYPE : void
//--------------------------------------------------------------------------
static
void BgpModifyAdjRibInForNewUpdate(
    Node* node,
    BgpData * bgp,
    BgpRouteInfo nlri,
    BgpUpdateMessage* updateMessage,
    BgpConnectionInformationBase* connectionPtr,
    int numAttr)
{
    int i = 0,index;
    int interfaceIndex = -1, nextHopLen;
    BOOL isRtChange = FALSE, found;
    NetworkType pType = NETWORK_IPV4;

    int numEntriesInAdjRibIn = BUFFER_GetCurrentSize(&(bgp->adjRibIn))
    / sizeof(BgpRoutingInformationBase);

    BgpRoutingInformationBase* adjRibIn = (BgpRoutingInformationBase*)
    BUFFER_GetData(&(bgp->adjRibIn));

    BgpRoutingInformationBase* bestRtPtr = NULL;

    BgpPathAttribute* pathAttribute = (BgpPathAttribute*)
    updateMessage->pathAttribute;

    index = BgpFindAttr(updateMessage,
                        BGP_PATH_ATTR_TYPE_AS_PATH,
                        numAttr,
                        found);
    if (!found)
    {
        ERROR_Assert(updateMessage, "Invalid update message\n");
    }
    unsigned short* asPathList = NULL;
    unsigned char asPathLength = 0;
    if (pathAttribute[index].attributeLength)
    {
        asPathList = (unsigned short*)
            (((BgpPathAttributeValue*)
            (pathAttribute[index].attributeValue))->pathSegmentValue);

        asPathLength = ((BgpPathAttributeValue*)
                  (pathAttribute[index].attributeValue))->pathSegmentLength;
    }
    GenericAddress nextHopAddress;
    memset(&nextHopAddress, 0, sizeof(GenericAddress));
    NodeAddress nextHopToPeerV4 = 0;
    in6_addr nextHopToPeerV6;
    memset(&nextHopToPeerV6, 0, sizeof(in6_addr));
    Address temp;
     temp.networkType = NETWORK_IPV4;

    LocalPrefValue* localPref = NULL;
    index = BgpFindAttr(updateMessage,
                        BGP_PATH_ATTR_TYPE_ORIGIN,
                        numAttr,
                        found);
    if (!found)
    {
        ERROR_Assert(updateMessage, "Invalid update message\n");
    }

    unsigned int originType =
        updateMessage->pathAttribute[index].attributeValue[0];

    MedValue* medVal = NULL;
    MpReachNlri* mpReach = NULL;
    NodeAddress originatorId = 0;

    //Next we should consider the case that update messge carries
    // Next Hop attribute in MBGP
    if (connectionPtr->isInternalConnection)
    {
        index = BgpFindAttr(updateMessage,
                            BGP_PATH_ATTR_TYPE_LOCAL_PREF,
                            numAttr,
                            found);
        if (found)
        {
            localPref = (LocalPrefValue*) pathAttribute[index].attributeValue;
        }
        else
        {
            localPref = &(bgp->localPref);
        }

        index = BgpFindAttr(updateMessage,
                            BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC,
                            numAttr,
                            found);
        if (found)
        {
            medVal = (MedValue*) pathAttribute[index].attributeValue;
        }
        index = BgpFindAttr(updateMessage, BGP_MP_REACH_NLRI, numAttr, found);
        if (found)
        {
            mpReach = (MpReachNlri*) pathAttribute[index].attributeValue;
        }
        index = BgpFindAttr(updateMessage,
                            BGP_PATH_ATTR_TYPE_ORIGINATOR_ID,
                            numAttr,
                            found);
        if (found)
        {
            originatorId =
                *((NodeAddress*) pathAttribute[index].attributeValue);
        }
        else if (bgp->isRtReflector)
        {
            originatorId = (node->nodeId);
        }
    }
    else
    {
        localPref = &(bgp->localPref);
        index = BgpFindAttr(updateMessage,
                            BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC,
                            numAttr,
                            found);
        if (found)
        {
            medVal = (MedValue*) pathAttribute[index].attributeValue;
        }
        index =
            BgpFindAttr(updateMessage, BGP_MP_REACH_NLRI, numAttr, found);
        if (found)
        {
            mpReach = (MpReachNlri*) pathAttribute[index].attributeValue;
        }
    }

    //Get the next hop
    if (mpReach)
    {
        memcpy(&nextHopAddress, mpReach->nextHop,mpReach->nextHopLen);
        nextHopLen = mpReach->nextHopLen;
    }
    else
    {
        index = BgpFindAttr(updateMessage,
                            BGP_PATH_ATTR_TYPE_NEXT_HOP,
                            numAttr,
                            found);
        if (!found)
        {
            ERROR_Assert(updateMessage, "Invalid update message\n");
        }
        memcpy(&nextHopAddress,
                  updateMessage->pathAttribute[index].attributeValue,
                  updateMessage->pathAttribute[index].attributeLength);
        nextHopLen = updateMessage->pathAttribute[index].attributeLength;
    }

    if (nextHopLen == 4 )
    {
        NetworkGetInterfaceAndNextHopFromForwardingTable( node,
                            nextHopAddress.ipv4,
                            &interfaceIndex,
                            &nextHopToPeerV4);
        pType = NETWORK_IPV4;


    }
    else if (nextHopLen == 16 )
    {
        //check point,the Ipv6x function is not tested??
        Ipv6GetInterfaceAndNextHopFromForwardingTable( node,
                               nextHopAddress.ipv6,
                               &interfaceIndex,
                               &nextHopToPeerV6);
        pType = NETWORK_IPV6;


    }


    in6_addr bgpInvalidAddr;
    memset(&bgpInvalidAddr,0,sizeof(in6_addr));//bgpInvalidAddr = 0;

    if ((pType == NETWORK_IPV4) && (nextHopToPeerV4 == 0) )
    {
        if (mpReach)
            memcpy(&nextHopToPeerV4, mpReach->nextHop,mpReach->nextHopLen);
        else
        {
            index = BgpFindAttr(updateMessage,
                                BGP_PATH_ATTR_TYPE_NEXT_HOP,
                                numAttr,
                                found);
            memcpy(&nextHopToPeerV4,
                  updateMessage->pathAttribute[index].attributeValue,
                  updateMessage->pathAttribute[index].attributeLength);
        }
    }
    else if ((pType == NETWORK_IPV6)&&
            (CMP_ADDR6(nextHopToPeerV6,bgpInvalidAddr) == 0))
    {
        if (mpReach)
            memcpy(&nextHopToPeerV6, mpReach->nextHop,mpReach->nextHopLen);
        else
        {
            index = BgpFindAttr(updateMessage,
                                BGP_PATH_ATTR_TYPE_NEXT_HOP,
                                numAttr,
                                found);
            memcpy(&nextHopToPeerV6,
                  updateMessage->pathAttribute[index].attributeValue,
                  updateMessage->pathAttribute[index].attributeLength);
        }
    }

    if (pType == NETWORK_IPV6 )
        SetIPv6AddressInfo(&temp,nextHopToPeerV6);
    else if (pType == NETWORK_IPV4 )
        SetIPv4AddressInfo(&temp,nextHopToPeerV4);


    if (!bgp->bgpNextHopLegacySupport )
    {
        //If the update message has routes of external AS,
        //then next hop is learnt from the update message (mpReach->nextHop)
        if ((asPathLength) && (connectionPtr->isInternalConnection))
        {
            if (pType == NETWORK_IPV6 )
            {
                SetIPv6AddressInfo(&temp,nextHopAddress.ipv6);
            }
            else if (pType == NETWORK_IPV4)
            {
                SetIPv4AddressInfo(&temp,nextHopAddress.ipv4);
            }
        }
    }
    for (i = 0; i < numEntriesInAdjRibIn; i++)
    {
        if (BgpIsSamePrefix(adjRibIn[i].destAddress,nlri) )
        {
            if (Address_IsSameAddress(&adjRibIn[i].peerAddress,
                          &connectionPtr->remoteAddr))
            {
                // the route is from the same peer so modify the
                // previous route
                isRtChange = TRUE;
                //Handle Empty AS-PATH
                if (asPathLength)
                {
                    if (adjRibIn[i].asPathList)
                    {
                        MEM_free(adjRibIn[i].asPathList->pathSegmentValue);
                    }
                    else
                    {
                        adjRibIn[i].asPathList = (BgpPathAttributeValue*)
                            MEM_malloc(sizeof(BgpPathAttributeValue));
                    }
                    adjRibIn[i].asPathList->pathSegmentLength = asPathLength
                           * sizeof(unsigned short);
                    adjRibIn[i].asPathList->pathSegmentType =
                            BGP_PATH_SEGMENT_AS_SEQUENCE;

                    adjRibIn[i].asPathList->pathSegmentValue =
                        (unsigned short*) MEM_malloc(adjRibIn[i].asPathList->
                                                          pathSegmentLength);

                    memcpy(adjRibIn[i].asPathList->pathSegmentValue,
                           asPathList,
                           adjRibIn[i].asPathList->pathSegmentLength);
                }
                else
                {
                    if (adjRibIn[i].asPathList)
                    {
                        MEM_free(adjRibIn[i].asPathList->pathSegmentValue);
                        MEM_free(adjRibIn[i].asPathList);
                        adjRibIn[i].asPathList = NULL;
                    }
                }
                //The following piece may be modified for MBGP ext.
                memcpy((char*) &adjRibIn[i].nextHop,
                       (char*) &temp,//nextHopToPeer
                       sizeof(Address));

                adjRibIn[i].isValid = TRUE;

                if (bestRtPtr == NULL)
                {
                    bestRtPtr = &adjRibIn[i];
                    bestRtPtr->pathAttrBest = BGP_PATH_ATTR_BEST_FALSE;
                }
            }
            else
            {
                if (adjRibIn[i].isValid)
                {
                    //The following function was modified only in a line
                    //for MBGP ext.
                    BgpCheckBestRoute(node, bgp, updateMessage,
                              &adjRibIn[i], NULL, connectionPtr, &bestRtPtr,
                              FALSE);
                }
            }
        }
    }

    if (bestRtPtr != NULL)
    {
        bestRtPtr->pathAttrBest = BGP_PATH_ATTR_BEST_TRUE;
    }

    if (!isRtChange)
    {
        if (DEBUG)
        {
            printf("New routes Added to RibIn\n");
        }
        BgpRoutingInformationBase routingTableRow;
        // there is no privious route for this destination from the same peer
        // so add this route
        // and mark it as the best route

        if (medVal == NULL)
        {
            if (connectionPtr->ismedPresent)
            {
                medVal = &(connectionPtr->multiExitDisc);
            }
            else
            {
                medVal = &(bgp->multiExitDisc);
            }
        }
        BgpFillRibEntry(
            &routingTableRow,
            nlri.prefix,
            nlri.prefixLen,
            connectionPtr->remoteAddr,
            connectionPtr->asIdOfPeer,
            connectionPtr->isClient,
            connectionPtr->remoteBgpIdentifier,
            BGP_ORIGIN_EGP,
            originType,
            (char*) asPathList,
            asPathLength * sizeof(unsigned short),
            *localPref,
            *localPref,
            *medVal,
            (unsigned char) ((bestRtPtr == NULL) ? BGP_PATH_ATTR_BEST_TRUE :
                     BGP_PATH_ATTR_BEST_FALSE),
            temp,//nextHopToPeer
            connectionPtr->weight,
            originatorId);

        BUFFER_AddDataToDataBuffer(
            &(bgp->adjRibIn),
            (char*) &routingTableRow,
            sizeof(BgpRoutingInformationBase));
    }

}


//--------------------------------------------------------------------------
// FUNCTION  BgpFindAlternativeRoute
//
// PURPOSE : To find an alternative route for a withdrawn route.
//
// PARAMETERS:  bgp,  bgp internal structure
//              rtDeleted, the route which will be deleted
//              bestRoutePtr, if any alternative route is found this pointer
//                            will point to that route
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpFindAlternativeRoute(
    Node* node,
    BgpData* bgp,
    BgpRoutingInformationBase* rtDeleted,
    BgpRoutingInformationBase** bestRoutePtr)
{
    int  i = 0;
    int numEntriesAdjRibIn = BUFFER_GetCurrentSize(&bgp->adjRibIn)
    / sizeof(BgpRoutingInformationBase);

    BgpRoutingInformationBase* adjRibIn = (BgpRoutingInformationBase*)
    BUFFER_GetData(&bgp->adjRibIn);

    BgpRoutingInformationBase* prevBestRoute = NULL;

    BOOL wantAnAssumedBest = TRUE;

    for (i = 0; i < numEntriesAdjRibIn; i++)
    {
        if (BgpIsSamePrefix(adjRibIn[i].destAddress,
            rtDeleted->destAddress)&& (&adjRibIn[i] != rtDeleted) &&
             adjRibIn[i].isValid )
        {
            if (wantAnAssumedBest == TRUE)
            {
                //Make path attribute best TRUE to a path..
                //Most likely one, that is available in adjRiBIn
                //on the first place other than "rtDeleted".You
                //make it (or let call it)"prevBestRoute"
                prevBestRoute = &adjRibIn[i];
                prevBestRoute->pathAttrBest = BGP_PATH_ATTR_BEST_TRUE;
                //prevBestRoute is an "assumed best" route.
                //We do not need any more, so make the flag false
                wantAnAssumedBest = FALSE;
            }
            else
            {
                //search among the rest of the routes whether they
                //are better than than "prevBestRoute". And if better then
                //return it.
                int j = 0;
                BgpConnectionInformationBase* connInfoBase =
                    (BgpConnectionInformationBase*) BUFFER_GetData(&bgp->
                                   connInfoBase);

                int numConn = BUFFER_GetCurrentSize(&bgp->connInfoBase)
                    / sizeof(BgpConnectionInformationBase);

                *bestRoutePtr = NULL;

                for (j = 0; j < numConn; j++)
                {
                    if (Address_IsSameAddress(&connInfoBase[j].remoteAddr,
                       &adjRibIn[i].peerAddress))
                    {
                        break;
                    }
                }
                BgpCheckBestRoute(node, bgp, NULL, prevBestRoute,
                    &adjRibIn[i], &connInfoBase[j], bestRoutePtr, TRUE);

                if (*bestRoutePtr != NULL)
                {
                    // if "bestRoutePtr" != NULL then prevbestRoute
                    // is still best till now.
                    prevBestRoute->pathAttrBest = BGP_PATH_ATTR_BEST_TRUE;
                }
                else
                {
                    // some other route is best. It is  adjRibIn[i].
                    // let make it "prevBestRoute"
                    adjRibIn[i].pathAttrBest = BGP_PATH_ATTR_BEST_TRUE;
                    prevBestRoute = &adjRibIn[i];
                }
            }
        }
    }
    *bestRoutePtr = prevBestRoute;
}


//--------------------------------------------------------------------------
//  FUNCTION : BgpModifiyAdjRibInForWithdrawnRoutes()
//
//  PURPOSE : This is the part of BgpDecisionProcessPhase1. In addition
//            to selecting the routes for AdjRibLoc, decision process phase 1
//            also decides which routes are to be advertised as
//            withdrawn routes in the next update message.
//
// PARAMETER : bgp,  bgp internal structure
//             withdrawnRt, route to be deleted
//             connectionPtr, the connection which has received the withdrawn
//                            route
//  RETURN TYPE : none
//--------------------------------------------------------------------------

static
void BgpModifyAdjRibInForWithdrawnRoutes(
    Node* node,
    BgpData* bgp,
    BgpRouteInfo withdrawnRt,
    BgpConnectionInformationBase* connectionPtr)
{
    int numEntriesInAdjRibIn = BUFFER_GetCurrentSize(&(bgp->adjRibIn))
    / sizeof(BgpRoutingInformationBase);

    BgpRoutingInformationBase* adjRibIn = (BgpRoutingInformationBase*)
        BUFFER_GetData(&(bgp->adjRibIn));

    int i = 0;

    BgpRoutingInformationBase* rtToDelete = NULL;
    BgpRoutingInformationBase* bestRtPtr = NULL;
    BOOL bestRtDeleted = FALSE;

    for (i = 0; i < numEntriesInAdjRibIn; i++)
    {
        if (BgpIsSamePrefix(adjRibIn[i].destAddress,withdrawnRt) &&
         Address_IsSameAddress(&adjRibIn[i].peerAddress,
                   &connectionPtr->remoteAddr))

        {
            if (adjRibIn[i].pathAttrBest == BGP_PATH_ATTR_BEST_FALSE)
            {
                // The route to be deleted is not best route so
                // just delete the route.
                rtToDelete = &adjRibIn[i];
            }
            else
            {
                bestRtDeleted = TRUE;
                rtToDelete = &adjRibIn[i];
            }
            break;
        }
    }

    if (rtToDelete == NULL)
    {
        if (DEBUG)
        {
            printf("No routes to be delted so return\n");
        }
        return;
    }

    if (bestRtDeleted)
    {
        // The route which is to be deleted was chosen as best
        // route earlier. So with deleting the entry we need to find
        // if there is any other route avalable for the same
        // destination. And we need to modify the local routing table
        // and the rib out accordingly
        BgpFindAlternativeRoute(node, bgp, rtToDelete, &bestRtPtr);

        // Remove from forwarding table since we do not want to use
        // the withdrawn route for forwarding data packets.
        if (bestRtPtr != NULL)
        {
            int j = 0;

            int numConnections = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
                / sizeof(BgpConnectionInformationBase);

            BgpConnectionInformationBase* connPtr =
                (BgpConnectionInformationBase*)
                BUFFER_GetData(&(bgp->connInfoBase));


            bestRtPtr->pathAttrBest = BGP_PATH_ATTR_BEST_TRUE;

            // find the connection of best route (bestRoute->peerAddress
            // == connection->remoteAddr)
            // add rtToDelete->destAddr, in withdrawnRt of connection

            for (j = 0; j < numConnections; j++)
            {
                if (connPtr[j].state != BGP_ESTABLISHED)
                {
                    // The state of the current connection is not Established
                    // so continue with the next connection
                    continue;
                }

                if (connPtr[j].isInternalConnection &&
                        (rtToDelete->asIdOfPeer == bgp->asId))
                {
                    // do not advertise a route to an internal peer, which
                    // has been learned from another internal peer
                    continue;
                }
                if (Address_IsSameAddress(&rtToDelete->peerAddress,
                      &connPtr[j].remoteAddr))
                {
                    // don't advertise to a peer a route which is learnt
                    // from that speaker
                    continue;
                }

                BgpRouteInfo routeInfo;

                memcpy((char*) &routeInfo.prefix,
                       (char*)&rtToDelete->destAddress.prefix,
                       sizeof(Address));
                routeInfo.prefixLen = rtToDelete->destAddress.prefixLen;

                BUFFER_AddDataToDataBuffer(
                        &connPtr[j].withdrawnRoutes,
                        (char*) &routeInfo,
                        sizeof(BgpRouteInfo));
            }
        }
    }

    // Remove from AdjRibIn since we no longer use
    // the withdrawn route.
    rtToDelete->pathAttrBest = BGP_PATH_ATTR_BEST_FALSE;
    rtToDelete->isValid = FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION : BgpDecisionProcessPhase3()
//
// PURPOSE  : To move the best routes and not advertised still now to rib out
//            from Rib Local and to move the routes which has been already
//            advertised and now not valid.
//
// PARAMETERS : node, the node running the decision process
//              bgp, bgp internal structure
//
// RETURN :     void
//--------------------------------------------------------------------------

static
void BgpDecisionProcessPhase3(Node* node, BgpData* bgp)
{
    int i = 0;
    int j = 0;

    BgpConnectionInformationBase* bgpConnInfo =
        (BgpConnectionInformationBase*) BUFFER_GetData(&(bgp->connInfoBase));

    int numEntriesConn = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
    / sizeof(BgpConnectionInformationBase);

    BgpAdjRibLocStruct* adjRibLoc = (BgpAdjRibLocStruct*)
         BUFFER_GetData(&(bgp->ribLocal));

    int numEntriesRibLoc = BUFFER_GetCurrentSize(&(bgp->ribLocal))
    / sizeof(BgpAdjRibLocStruct);

    if (DEBUG_CONNECTION)
    {
        printf("The connection information is:\n");
        BgpPrintConnInformationBase(node, bgp);
    }

    if (DEBUG_TABLE)
    {
        char simTime[MAX_CLOCK_TYPE_STR_LEN];
        ctoa(node->getNodeTime(), simTime);

        printf("The entries in the ribOut before update: %s\n", simTime);

        for (i = 0; i < numEntriesConn; i++)
        {
            BgpPrintAdjRibOut(node, &bgpConnInfo[i]);
        }
        for (i = 0; i < numEntriesConn; i++)
        {
            BgpPrintWithdrawnRoutes(&bgpConnInfo[i]);
        }
    }

    // Loop through all the entries of Rib Local and update the Adjuscent
    // Rib Out of all the connections if the connections are in Established
    // state
    for (i = 0; i < numEntriesRibLoc; i++)
    {
        for (j = 0; j < numEntriesConn; j++)
        {
            BgpAdjRibLocStruct* adjRibLocPtr = NULL;
            BgpAdjRibLocStruct** adjRibOutPtr = (BgpAdjRibLocStruct **)
                BUFFER_GetData(&bgpConnInfo[j].adjRibOut);

            int numEntriesInRibOut = BUFFER_GetCurrentSize(&bgpConnInfo[j].
                               adjRibOut) / sizeof(BgpAdjRibLocStruct*);
            int k;
            BOOL isExistsInRibOut = FALSE;

            if (bgpConnInfo[j].state != BGP_ESTABLISHED)
            {
                // The state of the current connection is not Established
                // so continue with the next connection
                continue;
            }

            if (bgpConnInfo[j].isInternalConnection &&
                (adjRibLoc[i].ptrAdjRibIn->asIdOfPeer == bgp->asId))
            {
                // do not advertise a route to an internal peer, which
                // has been learned from another internal peer, only
                // if this node is not RR
                if (bgp->isRtReflector == FALSE)
                {
                    if (DEBUG)
                    {
                        printf("do not advertise %dth route to an internal"
                               " peer\n",i);
                    }
                    continue;
                }
                else if ((!(adjRibLoc[i].ptrAdjRibIn->isClient)) &&
                         (!(bgpConnInfo[j].isClient)))
                {
                    // Do not reflect routes to non-client peers if
                    // recvd from non client peers
                    if (DEBUG)
                    {
                        printf("Do not reflect routes to non-client peers"
                               " if this route recvd from non client"
                               " peers\n");
                    }
                    continue;
                }
            }

            NetworkType remoteType = bgpConnInfo[j].remoteAddr.networkType;
            NetworkType netType =
                adjRibLoc[i].ptrAdjRibIn->destAddress.prefix.networkType;

            if (remoteType == netType )
            {
                if ((remoteType == NETWORK_IPV4)&&
                    IsIpAddressInSubnet(
                      GetIPv4Address(bgpConnInfo[j].remoteAddr),
                     GetIPv4Address(adjRibLoc[i].ptrAdjRibIn->destAddress.prefix),
                     32 - adjRibLoc[i].ptrAdjRibIn->destAddress.prefixLen ) )
                {
                    // don't advertise to a peer a route through which both
                    // the peer are connected
                    if (DEBUG)
                    {
                        printf("don't advertise to a peer %d route through"
                        " which both the peer are connected \n",i);
                    }
                    continue;
                }


                if (remoteType == NETWORK_IPV6 )
                {
                    in6_addr remoteAddress =
                        GetIPv6Address(bgpConnInfo[j].remoteAddr);
                    in6_addr ipAddress =
                        GetIPv6Address(adjRibLoc[i].ptrAdjRibIn->
                            destAddress.prefix);
                    unsigned int prefixLen =
                        adjRibLoc[i].ptrAdjRibIn->destAddress.prefixLen;

                    if (Ipv6IsAddressInNetwork(&remoteAddress,
                                               &ipAddress,
                                               prefixLen))
                    {

                        continue;
                    }

                }//if (remoteType == NETWORK_IPV6 )
            }//if (remoteType == netType )




            if (Address_IsSameAddress(&adjRibLoc[i].ptrAdjRibIn->peerAddress,
                       &bgpConnInfo[j].remoteAddr))
            {
                // don't advertise to a peer a route which is learnt
                // from that speaker
                if (DEBUG)
                {
                    printf("don't advertise to a peer %dth route which"
                    " is learnt from that speaker\n",i);
                }
                continue;
            }

            // Go through all the entries of Adj Rib Out to check whether
            // the route is already there or not
            for (k = 0; k < numEntriesInRibOut; k ++)
            {
                if (&adjRibLoc[i] == adjRibOutPtr[k])
                {
                    isExistsInRibOut = TRUE;
                    // The route exits in Rib Out. If the validity of the
                    // if FALSE then delete the route from adj Rib Out not to
                    // advertise the route
                    if (adjRibLoc[i].isValid == FALSE)
                    {
                        if (DEBUG)
                        {
                            printf("Removed %d entry of RIb Loc from"
                                    " Ribout\n",i);
                        }
                        BUFFER_ClearDataFromDataBuffer(
                            &bgpConnInfo[j].adjRibOut,
                            (char*) &adjRibOutPtr[k],
                            sizeof(BgpAdjRibLocStruct*),
                            FALSE);
                        break;
                    }
                }
            }
            if (!isExistsInRibOut)
            {
                // The route is not there in Rib Out. So either it is a new
                // route or the route has already been advertised. If it
                // has already been advertised then the movedToRibOut field
                // of Rib Local will be TRUE
                if (adjRibLoc[i].isValid == FALSE  && adjRibLoc[i].
                    movedToAdjRibOut == TRUE)
                {
                    // The route aleardy been advertised in NLRI field. And
                    // now the route has become invalid so add the route in
                    // withdrawn route field
                    BgpRouteInfo routeInfo;
                    memcpy((char*) &(routeInfo.prefix),
                        (char*)&(adjRibLoc[i].ptrAdjRibIn->destAddress.prefix),
                        sizeof(Address));
                    //MBGP routeInfo.prefix = adjRibLoc[i].ptrAdjRibIn->
                    // destAddress prefix;
                    routeInfo.prefixLen = adjRibLoc[i].ptrAdjRibIn->
                        destAddress.prefixLen;
                    if (DEBUG)
                    {
                        printf("Added %d entry of RIb Loc to"
                            " WithdrawnRoutes\n",i);
                    }
                    BUFFER_AddDataToDataBuffer(
                        &bgpConnInfo[j].withdrawnRoutes,
                        (char*) &routeInfo,
                        sizeof(BgpRouteInfo));
                    adjRibLoc[i].movedToWithdrawnRt = TRUE;

                }
                else if (adjRibLoc[i].isValid == TRUE && adjRibLoc[i].
                    movedToAdjRibOut == FALSE)
                {
                    // The new route has not been advertised earlier so add
                    // it to rib Out to advertise
                    adjRibLocPtr = &adjRibLoc[i];
                   if (DEBUG)
                   {
                       printf("Added %d entry of RIb Loc to RibOut\n",i);
                   }
                   BUFFER_AddDataToDataBuffer(&bgpConnInfo[j].adjRibOut,
                           (char*) &adjRibLocPtr,
                           sizeof(BgpAdjRibLocStruct*));
                   if (adjRibLoc[i].movedToWithdrawnRt == TRUE)
                   {
                       /*It was earlier moved to withdrawn rts so remove
                        * from that list */
                        adjRibLoc[i].movedToWithdrawnRt = FALSE;
                        BgpRouteInfo* rtInfo = (BgpRouteInfo*)
                        BUFFER_GetData(&bgpConnInfo[j].withdrawnRoutes);

                        int numWithdrawnRt =
                        BUFFER_GetCurrentSize(&bgpConnInfo[j].withdrawnRoutes)
                        / sizeof(BgpRouteInfo);

                         int local = 0;
                        for (local = 0; local < numWithdrawnRt; local++)
                        {
                            if (BgpIsSamePrefix(adjRibLoc[i].
                               ptrAdjRibIn->destAddress,rtInfo[local]))
                            {
                                  if (DEBUG)
                                {
                                    printf("In DP3, removing route from"
                                       " withdrawn routes\n");
                                }
                                //Remove from withdrawn list
                                BUFFER_ClearDataFromDataBuffer(
                                 &(bgpConnInfo[j].withdrawnRoutes),
                                 (char*) (rtInfo+local),
                                 sizeof(BgpRouteInfo),
                                 TRUE);
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                if (DEBUG)
                {
                    printf("%d entry of RibLoc Already exists in"
                        " this conn\n",i);
                }
            }
        }

        adjRibLoc[i].movedToAdjRibOut = TRUE;

        if (adjRibLoc[i].isValid == FALSE  && adjRibLoc[i].
            movedToAdjRibOut == TRUE)
        {
            // if the route is invalid and previously advertised mark it as
            // not advertised not to add this entry in withdrawn route field
            // more than once.
            adjRibLoc[i].movedToAdjRibOut = FALSE;
        }
    }

    if (DEBUG_TABLE)
    {
        char simTime[MAX_CLOCK_TYPE_STR_LEN];
        ctoa(node->getNodeTime(), simTime);

        printf("The entries in the ribOut After update: %s\n", simTime);

        for (i = 0; i < numEntriesConn; i++)
        {
            BgpPrintAdjRibOut(node, &bgpConnInfo[i]);
        }
        for (i = 0; i < numEntriesConn; i++)
        {
            BgpPrintWithdrawnRoutes(&bgpConnInfo[i]);
        }
    }

    for (j = 0; j < numEntriesConn; j++)
    {
        if ((BUFFER_GetCurrentSize(&bgpConnInfo[j].adjRibOut) ||
         BUFFER_GetCurrentSize(&bgpConnInfo[j].withdrawnRoutes)) &&
            !bgpConnInfo[j].updateTimerAlreadySet)
        {
            // If there is any entry in withdrawn route field or in rib out
            // set timer to advertise the routes.
            BgpSetUpdateTimer(node, &bgpConnInfo[j]);

            if (DEBUG)
            {
                printf("\tnode %d Setting timer to send update to %u\n",
               node->nodeId,bgpConnInfo[j].remoteId);
            }
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION : BgpDecisionProcessPhase2
//
// PURPOSE : Move the best of the routes to AdjRIbLoc
//
// PARAMETERS : node, the node running decision process
//              bgp,  bgp internal structure
//
// RETURN : none
//--------------------------------------------------------------------------

static
void BgpDecisionProcessPhase2(Node* node, BgpData* bgp)
{
    int i = 0;
    int numEntriesInAdjRibIn = BUFFER_GetCurrentSize(&(bgp->adjRibIn))
    / sizeof(BgpRoutingInformationBase);

    BgpRoutingInformationBase* adjRibInPtr = (BgpRoutingInformationBase*)
    BUFFER_GetData(&(bgp->adjRibIn));

    char* prevStartPtr = BUFFER_GetData(&bgp->ribLocal);
    char* nextStartPtr = NULL;
    IntPtr numByteShift = 0;
    int pathSegmentLength = 0;
    if (DEBUG_TABLE)
    {
        printf("\nRib Local before update:\n");
        BgpPrintAdjRibLoc(node, bgp);
    }

    for (i = 0; i < numEntriesInAdjRibIn; i++)
    {
        // Go through all the entries of the and modify the Rib Local
        // accordingly
        int j;

        int numEntriesInRibLoc = BUFFER_GetCurrentSize(&(bgp->ribLocal))
            / sizeof(BgpAdjRibLocStruct);

        BgpAdjRibLocStruct* ribLocPtr = (BgpAdjRibLocStruct*)
        BUFFER_GetData(&bgp->ribLocal);

        BOOL isExistsInLoc = FALSE;

        if (adjRibInPtr[i].isValid == FALSE)
        {
            // The current entry in Routing Information Base is invalid
            // If any entry in Rib Local is pointing to this entry then
            // make that entry as invalid and update the Network Forwarding
            // table that the corresponding destination is invalid
            for (j = 0; j < numEntriesInRibLoc; j++)
            {
                if (ribLocPtr[j].ptrAdjRibIn == &adjRibInPtr[i] &&
                    ribLocPtr[j].isValid)
                {
                    NetworkRoutingProtocolType protocol,eprotocol =
                        EXTERIOR_GATEWAY_PROTOCOL_EBGPv4;

                    if (adjRibInPtr[i].asIdOfPeer == bgp->asId)
                    {
                        protocol = EXTERIOR_GATEWAY_PROTOCOL_IBGPv4;
                    }
                    else
                    {
                        protocol = EXTERIOR_GATEWAY_PROTOCOL_EBGPv4;
                    }

                    ribLocPtr[j].isValid = FALSE;

                    in6_addr bgpInvalidAddr;
                    memset(&bgpInvalidAddr,
                           0,
                           sizeof(in6_addr));//bgpInvalidAddr = 0;
                    if (CMP_ADDR6(GetIPv6Address(adjRibInPtr[i].peerAddress),
                                bgpInvalidAddr) == 0  )
                    {
                        // Don't need to add this entry in the forwarding
                        //table that is the task of interior gateway protocol
                        break;
                    }
                    if (bgp->ip_version == NETWORK_IPV4 )//IPv4
                    {
                        if (DEBUG_FORWARD_TABLE)
                        {
                            printf("Content of Forwarding Table Before "
                                    "Update\n:");
                            NetworkPrintForwardingTable(node);
                        }

                        //Extracting and passing correct interface index to
                        //NetworkUpdateForwardingTable() for further
                        //processing

                        int interfaceIndex = 0;
                        NodeAddress nextHopAddress = 0;

                        NetworkGetInterfaceAndNextHopFromForwardingTable(
                            node,
                            (((adjRibInPtr[i]).nextHop).interfaceAddr).ipv4,
                            &interfaceIndex,
                            &nextHopAddress);

                        if (interfaceIndex == NETWORK_UNREACHABLE)
                        {
                            interfaceIndex =
                                NetworkIpGetInterfaceIndexForNextHop(
                                    node,
                                    (((adjRibInPtr[i]).nextHop).
                                        interfaceAddr).ipv4);
                        }

                        if (adjRibInPtr[i].asPathList)
                        {
                            pathSegmentLength =
                            adjRibInPtr[i].asPathList->pathSegmentLength / 2;
                        }
                        NetworkUpdateForwardingTable(
                                node,
                                GetIPv4Address(
                                adjRibInPtr[i].destAddress.prefix),
                                ConvertNumHostBitsToSubnetMask(32
                                        - adjRibInPtr[i].destAddress.prefixLen),
                                (NodeAddress) NETWORK_UNREACHABLE,//next hop
                                interfaceIndex,
                                pathSegmentLength,
                                eprotocol);
                        pathSegmentLength = 0; //Init back to 0 for further use
                        if (DEBUG_FORWARD_TABLE)
                        {
                            printf("Content of Forwarding Table After "
                                    "Update\n:");
                            NetworkPrintForwardingTable(node);
                        }

                        break;
                    }
                    else
                    {
                        if (DEBUG_FORWARD_TABLE)
                        {
                            printf("Content of Forwarding Table Before"
                                    " Update\n:");
                            Ipv6PrintForwardingTable(node);
                        }

                        Ipv6UpdateForwardingTable(
                                      node,
                                      GetIPv6Address(adjRibInPtr[i].
                                      destAddress.prefix),
                                      adjRibInPtr[i].destAddress.prefixLen,
                                      bgpInvalidAddr,//NETWORK_UNREACHABLE_ADDRESS
                                      DEFAULT_INTERFACE,
                                      NETWORK_UNREACHABLE,
                                      eprotocol);
                        if (DEBUG_FORWARD_TABLE)
                        {
                            printf("Content of Forwarding Table After"
                                    " Update\n:");
                            Ipv6PrintForwardingTable(node);
                        }

                        break;
                    }//else IPv6
                }//if
            }//for
        }//if
        else if (adjRibInPtr[i].pathAttrBest == BGP_PATH_ATTR_BEST_TRUE)
        {
            // If the entry in the Routing information base has been chosen
            // as best. Check if there is entry in the Rib Local for that
            // destination. If the entry for the destination is already
            // there check if it is pointing to the same entry of Routing
            // Information Base. If not make it point to the entry. If
            // there is no entry for the best route in Rib Local then add
            // the entry for the destination in Rib Local

            for (j = 0; j < numEntriesInRibLoc; j++)
            {
                if (BgpIsSamePrefix(ribLocPtr[j].ptrAdjRibIn->destAddress,
                     adjRibInPtr[i].destAddress))
                {
                    //There is one entry in Rib Local for the same destnation
                    isExistsInLoc = TRUE;

                    if (ribLocPtr[j].ptrAdjRibIn != &adjRibInPtr[i])
                    {
                        // Rib Local is pointing to different entry of
                        // Routing information base for the same destination.
                        // Make it point to the new best route entry.

                        NetworkRoutingProtocolType protocol,eprotocol =
                            EXTERIOR_GATEWAY_PROTOCOL_EBGPv4 ;

                        in6_addr bgpInvalidAddr;
                        memset(&bgpInvalidAddr,
                               0,
                               sizeof(in6_addr));//bgpInvalidAddr = 0;

                        if (CMP_ADDR6(GetIPv6Address(
                                adjRibInPtr[i].peerAddress),
                              bgpInvalidAddr) == 0)

                        {
                            protocol =
                                EXTERIOR_GATEWAY_PROTOCOL_BGPv4_LOCAL;
                        }

                        if (ribLocPtr[j].ptrAdjRibIn->isValid == TRUE )
                        {
                            if ((ribLocPtr[j].ptrAdjRibIn)->asPathList)
                            {
                                pathSegmentLength =
                                ((ribLocPtr[j].ptrAdjRibIn)->asPathList->
                                 pathSegmentLength / 2);
                            }
                            if (ribLocPtr[j].ptrAdjRibIn->asIdOfPeer ==
                                bgp->asId)
                            {
                                protocol = EXTERIOR_GATEWAY_PROTOCOL_IBGPv4;
                            }
                            else
                            {
                                protocol = EXTERIOR_GATEWAY_PROTOCOL_EBGPv4;
                            }
                            if ((bgp->ip_version == NETWORK_IPV4)
                            && (adjRibInPtr[i].nextHop.networkType ==
                                NETWORK_IPV4) )//IPv4
                            {
                                if (DEBUG_FORWARD_TABLE)
                                {
                                    printf("Content of Forwarding Table"
                                        " Before Update\n:");
                                    NetworkPrintForwardingTable(node);
                                }

                                NetworkUpdateForwardingTable(
                                    node,
                                    GetIPv4Address(ribLocPtr[j].
                                        ptrAdjRibIn->destAddress.prefix),
                                    ConvertNumHostBitsToSubnetMask(32
                                            - ribLocPtr[j].ptrAdjRibIn->
                                            destAddress.prefixLen),
                                    (NodeAddress) NETWORK_UNREACHABLE,
                                    ANY_INTERFACE,
                                    pathSegmentLength,
                                    eprotocol);


                                if (DEBUG_FORWARD_TABLE)
                                {
                                    printf("Content of Forwarding Table"
                                            " After Update\n:");
                                    NetworkPrintForwardingTable(node);
                                }
                            }
                            else //IPv6
                            {
                                in6_addr nextHopAddr;
                                int interfaceIndex;
                                if (DEBUG_FORWARD_TABLE)
                                {
                                    printf("Content of Forwarding Table"
                                            " Before Update\n:");
                                    Ipv6PrintForwardingTable(node);
                                }

                                Ipv6GetInterfaceAndNextHopFromForwardingTable(
                                    node,
                                    GetIPv6Address(
                                    (ribLocPtr[j].ptrAdjRibIn)->nextHop),
                                    &interfaceIndex,
                                    &nextHopAddr);

                                if (interfaceIndex == NETWORK_UNREACHABLE )
                                    interfaceIndex = DEFAULT_INTERFACE;

                                Ipv6UpdateForwardingTable(
                                    node,
                                    GetIPv6Address((ribLocPtr[j].ptrAdjRibIn)
                                    ->destAddress.prefix),
                                    (ribLocPtr[j].ptrAdjRibIn)->
                                        destAddress.prefixLen,
                                    bgpInvalidAddr,//NETWORK_UNREACHABLE_ADDRESS
                                    DEFAULT_INTERFACE,
                                    pathSegmentLength,
                                    eprotocol);

                                if (DEBUG_FORWARD_TABLE)
                                {
                                    printf("Content of Forwarding Table"
                                            " After Update\n:");
                                    Ipv6PrintForwardingTable(node);
                                }
                            }
                            ribLocPtr[j].ptrAdjRibIn = &adjRibInPtr[i];
                            ribLocPtr[j].isValid = TRUE;
                            ribLocPtr[j].movedToAdjRibOut = FALSE;
                        }
                        if (adjRibInPtr[i].asIdOfPeer == bgp->asId)
                        {
                            protocol = EXTERIOR_GATEWAY_PROTOCOL_IBGPv4;
                        }
                        else
                        {
                            protocol = EXTERIOR_GATEWAY_PROTOCOL_EBGPv4;
                        }
                        if (adjRibInPtr[i].origin != BGP_ORIGIN_IGP )
                        {
                            // If the new route is not learned from IGP
                            // update the Network's forwarding table with the
                            // new entry for the destination (For IGP learned
                            // routes forwarding table should be modified by
                            // IGP itself)
                            pathSegmentLength = 0;
                            if (adjRibInPtr[i].asPathList)
                            {
                                pathSegmentLength =
                                (adjRibInPtr[i].asPathList->pathSegmentLength
                                    / 2);
                            }
                            if ((bgp->ip_version == NETWORK_IPV4)
                                    && (adjRibInPtr[i].nextHop.networkType
                                            == NETWORK_IPV4) )//IPv4
                            {
                                Address nextHopAddr;
                                int interfaceIndex;

                                if (DEBUG_FORWARD_TABLE)
                                {
                                    printf("Content of Forwarding Table"
                                           " Before Update\n:");
                                    NetworkPrintForwardingTable(node);
                                }
                                if (!bgp->bgpNextHopLegacySupport)
                                {
                                    NetworkGetInterfaceAndNextHopFromForwardingTable(
                                        node,
                                        ((((adjRibInPtr[i]).nextHop).
                                            interfaceAddr).ipv4),
                                        &interfaceIndex,
                                        &((nextHopAddr).
                                            interfaceAddr).ipv4);

                                    if (((nextHopAddr).interfaceAddr).ipv4 ==
                                        0)
                                    {
                                        SetIPv4AddressInfo(
                                            &nextHopAddr,
                                            ((adjRibInPtr[i].nextHop).
                                            interfaceAddr).ipv4);
                                    }
                                }
                                else
                                {
                                    SetIPv4AddressInfo(&nextHopAddr,
                                     ((adjRibInPtr[i].nextHop).interfaceAddr).ipv4);
                                }
                                NetworkUpdateForwardingTable(
                                    node,
                                    GetIPv4Address(
                                    adjRibInPtr[i].destAddress.prefix),
                                    ConvertNumHostBitsToSubnetMask(32
                                    - adjRibInPtr[i].destAddress.prefixLen),
                                    GetIPv4Address(nextHopAddr),
                                    ANY_INTERFACE,
                                    pathSegmentLength,
                                    eprotocol);

                                if (bgp->redistribute2Ospf)
                                {
                                    if (protocol ==
                                        EXTERIOR_GATEWAY_PROTOCOL_EBGPv4 )
                                    {
                                        Ospfv2IsEnabled(
                                        node,
                                        GetIPv4Address(adjRibInPtr[i].
                                            destAddress.prefix),
                                        ConvertNumHostBitsToSubnetMask(32 -
                                           adjRibInPtr[i].destAddress.prefixLen),
                                        GetIPv4Address(adjRibInPtr[i].nextHop),
                                        pathSegmentLength,
                                        FALSE);
                                    }
                                }
                                pathSegmentLength = 0;
                                if (DEBUG_FORWARD_TABLE)
                                {
                                    printf("Content of Forwarding Table"
                                           " After Update\n:");
                                    NetworkPrintForwardingTable(node);
                                }
                            }
                            else //IPv6
                            {
                                Address nextHopAddr;
                                int interfaceIndex;
                                if (DEBUG_FORWARD_TABLE)
                                {
                                    printf("Content of Forwarding "
                                            "Table Before Update\n:");
                                    Ipv6PrintForwardingTable(node);
                                }

                                Ipv6GetInterfaceAndNextHopFromForwardingTable(
                                      node,
                                      GetIPv6Address(adjRibInPtr[i].nextHop),
                                      &interfaceIndex,
                                      &(nextHopAddr.interfaceAddr).ipv6);

                                if (!bgp->bgpNextHopLegacySupport)
                                {
                                     if (CMP_ADDR6(
                                         GetIPv6Address(nextHopAddr),
                                           bgpInvalidAddr)== 0)
                                     {
                                         SetIPv6AddressInfo(&nextHopAddr,
                                             ((adjRibInPtr[i].nextHop).
                                                interfaceAddr).ipv6);
                                     }
                                }
                                else
                                {
                                    SetIPv6AddressInfo(&nextHopAddr,
                                        ((adjRibInPtr[i].nextHop).
                                            interfaceAddr).ipv6);
                                }
                                if (interfaceIndex == NETWORK_UNREACHABLE )
                                    interfaceIndex = DEFAULT_INTERFACE;

                                Ipv6UpdateForwardingTable(
                                    node,
                                    GetIPv6Address(
                                    adjRibInPtr[i].destAddress.prefix),
                                    adjRibInPtr[i].destAddress.prefixLen,
                                    (nextHopAddr.interfaceAddr).ipv6,
                                    interfaceIndex,
                                    pathSegmentLength,
                                    eprotocol);

                                if (bgp->redistribute2Ospf)
                                {
                                    if (protocol ==
                                        EXTERIOR_GATEWAY_PROTOCOL_EBGPv4)
                                    {
                                      Ospfv3IsEnabled(
                                          node,
                                          GetIPv6Address(
                                          adjRibInPtr[i].destAddress.prefix),
                                          adjRibInPtr[i].destAddress.prefixLen,
                                          GetIPv6Address(adjRibInPtr[i].nextHop),
                                          pathSegmentLength,
                                          FALSE);
                                    }
                                }
                                pathSegmentLength = 0;
                                if (DEBUG_FORWARD_TABLE)
                                {
                                    printf("Content of Forwarding Table"
                                            " After Update\n:");
                                    Ipv6PrintForwardingTable(node);
                                }
                            }
                        }
                    }
                    else
                    {
                        // If the Rib Out entry is invalid then make it valid
                        // again and modify the Networks Forwarding table
                        // that the route to the destination has backed up
                        if (ribLocPtr[j].isValid == FALSE)
                        {
                            ribLocPtr[j].movedToAdjRibOut = FALSE;
                            ribLocPtr[j].isValid = TRUE;
                            if (adjRibInPtr[i].origin != BGP_ORIGIN_IGP)
                            {
                                NetworkRoutingProtocolType protocol,eprotocol
                                            = EXTERIOR_GATEWAY_PROTOCOL_EBGPv4;

                                if (adjRibInPtr[i].asIdOfPeer == bgp->asId)
                                {
                                    protocol =
                                        EXTERIOR_GATEWAY_PROTOCOL_IBGPv4;
                                }
                                else
                                {
                                    protocol =
                                        EXTERIOR_GATEWAY_PROTOCOL_EBGPv4;
                                }
                                in6_addr bgpInvalidAddr;
                                memset(&bgpInvalidAddr,0,sizeof(in6_addr));
                                if (CMP_ADDR6(GetIPv6Address(
                                    adjRibInPtr[i].peerAddress),
                                    bgpInvalidAddr) == 0)
                                {
                                    protocol =
                                    EXTERIOR_GATEWAY_PROTOCOL_BGPv4_LOCAL;
                                }
                                pathSegmentLength = 0;
                                if (adjRibInPtr[i].asPathList)
                                {
                                    pathSegmentLength = adjRibInPtr[i].
                                        asPathList->pathSegmentLength / 2;
                                }
                                if ((bgp->ip_version == NETWORK_IPV4)//IPv4
                                   && (adjRibInPtr[i].nextHop.networkType
                                                    == NETWORK_IPV4) )//IPv4
                                {
                                     Address nextHopAddr;
                                     int interfaceIndex;

                                    if (DEBUG_FORWARD_TABLE)
                                    {
                                        printf("Content of Forwarding Table "
                                            "Before Update\n:");
                                        NetworkPrintForwardingTable(node);
                                    }

                                    if (!bgp->bgpNextHopLegacySupport)
                                    {
                                        NetworkGetInterfaceAndNextHopFromForwardingTable(
                                            node,
                                            ((((adjRibInPtr[i]).nextHop).
                                                interfaceAddr).ipv4),
                                            &interfaceIndex,
                                            &((nextHopAddr).
                                                interfaceAddr).ipv4);

                                        if (((nextHopAddr).
                                                interfaceAddr).ipv4 == 0)
                                        {
                                            SetIPv4AddressInfo(
                                                &nextHopAddr,
                                                ((adjRibInPtr[i].nextHop).
                                                interfaceAddr).ipv4);
                                        }
                                    }
                                    else
                                    {
                                        SetIPv4AddressInfo(&nextHopAddr,
                                         ((adjRibInPtr[i].nextHop).
                                            interfaceAddr).ipv4);
                                    }

                                    NetworkUpdateForwardingTable(
                                        node,
                                        GetIPv4Address(adjRibInPtr[i].
                                            destAddress.prefix),
                                        ConvertNumHostBitsToSubnetMask(32
                                           - adjRibInPtr[i].destAddress.
                                           prefixLen),
                                        GetIPv4Address(nextHopAddr),
                                        ANY_INTERFACE,
                                        pathSegmentLength,
                                        eprotocol);
                                    if (bgp->redistribute2Ospf)
                                    {
                                        if (protocol ==
                                            EXTERIOR_GATEWAY_PROTOCOL_EBGPv4)
                                            Ospfv2IsEnabled(
                                                node,
                                                GetIPv4Address(adjRibInPtr[i].
                                                    destAddress.prefix),
                                                ConvertNumHostBitsToSubnetMask(32 -
                                               adjRibInPtr[i].destAddress.prefixLen),
                                               GetIPv4Address(adjRibInPtr[i].nextHop),
                                               pathSegmentLength,
                                               FALSE);
                                    }
                                    pathSegmentLength = 0;
                                    if (DEBUG_FORWARD_TABLE)
                                    {
                                       printf("Content of Forwarding Table "
                                                "AFTER Update\n:");
                                       NetworkPrintForwardingTable(node);
                                    }
                                }
                                else //IPv6
                                {
                                    Address nextHopAddr;
                                    int interfaceIndex;

                                    if (DEBUG_FORWARD_TABLE)
                                    {
                                        printf("Content of Forwarding Table "
                                                "Before Update\n:");
                                        Ipv6PrintForwardingTable(node);
                                    }

                                    Ipv6GetInterfaceAndNextHopFromForwardingTable(
                                        node,
                                        GetIPv6Address(adjRibInPtr[i].nextHop),
                                        &interfaceIndex,
                                        &(nextHopAddr.interfaceAddr).ipv6);

                                    if (!bgp->bgpNextHopLegacySupport)
                                    {
                                         if (CMP_ADDR6(
                                             GetIPv6Address(nextHopAddr),
                                               bgpInvalidAddr)== 0)
                                         {
                                             SetIPv6AddressInfo(&nextHopAddr,
                                                 ((adjRibInPtr[i].nextHop).
                                                 interfaceAddr).ipv6);
                                         }
                                    }
                                    else
                                    {
                                        SetIPv6AddressInfo(&nextHopAddr,
                                            ((adjRibInPtr[i].nextHop).
                                                interfaceAddr).ipv6);
                                    }
                                    if (interfaceIndex == NETWORK_UNREACHABLE )
                                        interfaceIndex = DEFAULT_INTERFACE;

                                    Ipv6UpdateForwardingTable(
                                        node,
                                        GetIPv6Address(adjRibInPtr[i].destAddress.prefix),
                                        adjRibInPtr[i].destAddress.prefixLen,
                                        (nextHopAddr.interfaceAddr).ipv6,
                                        interfaceIndex,
                                        pathSegmentLength,
                                        eprotocol);

                                     if (bgp->redistribute2Ospf)
                                     {
                                        if (protocol ==
                                            EXTERIOR_GATEWAY_PROTOCOL_EBGPv4 )
                                        Ospfv3IsEnabled(
                                            node,
                                            GetIPv6Address(adjRibInPtr[i].
                                            destAddress.prefix),
                                            adjRibInPtr[i].destAddress.prefixLen,
                                            GetIPv6Address(adjRibInPtr[i].nextHop),
                                            pathSegmentLength,
                                            FALSE);
                                     }
                                     pathSegmentLength = 0;
                                    if (DEBUG_FORWARD_TABLE)
                                    {
                                        printf("Content of Forwarding Table "
                                            "After Update\n:");
                                        Ipv6PrintForwardingTable(node);
                                    }
                                 }
                            }
                        }
                    }
                    break;
                }
            }
            // There is no entry in Rib Local for the best route so add the
            // new entry and update Networks forwarding table if the route
            // is not learnt from IGP
            if (!isExistsInLoc)
            {
                BgpAdjRibLocStruct ribLocStruct;
                ribLocStruct.isValid = TRUE;
                ribLocStruct.movedToAdjRibOut = FALSE;
                ribLocStruct.movedToWithdrawnRt = FALSE;
                ribLocStruct.ptrAdjRibIn = &adjRibInPtr[i];

                BUFFER_AddDataToDataBuffer(&bgp->ribLocal,
                                           (char*) &ribLocStruct,
                                           sizeof(BgpAdjRibLocStruct));

                ribLocPtr = (BgpAdjRibLocStruct*)
                                          BUFFER_GetData(&bgp->ribLocal);
                if (DEBUG)
                {
                    printf("Entry does not exist in RibLoc\n");
                }
                if (adjRibInPtr[i].origin != BGP_ORIGIN_IGP)
                {
                    NetworkRoutingProtocolType protocol,
                        eprotocol = EXTERIOR_GATEWAY_PROTOCOL_EBGPv4;

                    if (adjRibInPtr[i].asIdOfPeer == bgp->asId)
                    {
                        protocol = EXTERIOR_GATEWAY_PROTOCOL_IBGPv4;
                    }
                    else
                    {
                        protocol = EXTERIOR_GATEWAY_PROTOCOL_EBGPv4;
                    }
                    in6_addr bgpInvalidAddr;
                    memset(&bgpInvalidAddr,0,sizeof(in6_addr));
                    if (CMP_ADDR6(GetIPv6Address(adjRibInPtr[i].peerAddress),
                           bgpInvalidAddr)== 0)
                    {
                        protocol = EXTERIOR_GATEWAY_PROTOCOL_BGPv4_LOCAL;
                    }
                    if ((bgp->ip_version == NETWORK_IPV4 )//IPv4
                    && (adjRibInPtr[i].nextHop.networkType == NETWORK_IPV4))
                    {
                        Address nextHopAddr;
                        int interfaceIndex;
                        if (DEBUG_FORWARD_TABLE)
                        {
                            printf("Content of Forwarding Table Before "
                                   "Update\n:");
                            NetworkPrintForwardingTable(node);
                        }
                        if (DEBUG)
                        {
                            if (adjRibInPtr[i].asPathList)
                            printf("Cost of path is %d\n",
                                   adjRibInPtr[i].asPathList->pathSegmentLength / 2);
                        }

                        if (!bgp->bgpNextHopLegacySupport)
                        {
                            NetworkGetInterfaceAndNextHopFromForwardingTable(
                                node,
                                ((((adjRibInPtr[i]).nextHop).interfaceAddr).ipv4),
                                &interfaceIndex,
                                &((nextHopAddr).interfaceAddr).ipv4);

                            if (((nextHopAddr).interfaceAddr).ipv4 == 0)
                            {
                                SetIPv4AddressInfo(
                                    &nextHopAddr,
                                    ((adjRibInPtr[i].nextHop).interfaceAddr).
                                    ipv4);
                            }
                        }
                        else
                        {
                            SetIPv4AddressInfo(&nextHopAddr,
                             ((adjRibInPtr[i].nextHop).interfaceAddr).ipv4);
                        }

                        if (adjRibInPtr[i].asPathList)
                        {
                            pathSegmentLength =
                                adjRibInPtr[i].asPathList->
                                 pathSegmentLength / 2;
                        }
                        NetworkUpdateForwardingTable(
                            node,
                            GetIPv4Address(adjRibInPtr[i].destAddress.prefix),
                            ConvertNumHostBitsToSubnetMask(32
                                - adjRibInPtr[i].destAddress.prefixLen),
                            GetIPv4Address(nextHopAddr),
                            ANY_INTERFACE,
                            pathSegmentLength,
                            eprotocol);
                        if (bgp->redistribute2Ospf)
                        {
                            if (protocol == EXTERIOR_GATEWAY_PROTOCOL_EBGPv4 )
                                Ospfv2IsEnabled(
                                node,
                                GetIPv4Address(
                                adjRibInPtr[i].destAddress.prefix),
                                ConvertNumHostBitsToSubnetMask(32 -
                                adjRibInPtr[i].destAddress.prefixLen),
                                GetIPv4Address(adjRibInPtr[i].nextHop),
                                pathSegmentLength,
                                FALSE);
                        }
                        pathSegmentLength = 0;
                        if (DEBUG_FORWARD_TABLE)
                        {
                            printf("Content of Forwarding Table After Update\n:");
                            NetworkPrintForwardingTable(node);
                        }
                    }
                    else if (bgp->ip_version == NETWORK_IPV6 )//IPv6
                    {

                        int interfaceIndex;
                        Address nextHopAddr;

                        if (DEBUG_FORWARD_TABLE)
                        {
                            printf("Content of Forwarding Table Before "
                                    "Update\n:");
                            Ipv6PrintForwardingTable(node);
                        }
                        Ipv6GetInterfaceAndNextHopFromForwardingTable(
                            node,
                            GetIPv6Address(adjRibInPtr[i].nextHop),
                            &interfaceIndex,
                            &((nextHopAddr).interfaceAddr).ipv6);

                        if (!bgp->bgpNextHopLegacySupport)
                        {
                             if (CMP_ADDR6(GetIPv6Address(nextHopAddr),
                                   bgpInvalidAddr)== 0)
                             {
                                 SetIPv6AddressInfo(&nextHopAddr,
                                     ((adjRibInPtr[i].nextHop).interfaceAddr).ipv6);
                             }
                        }
                        else
                        {
                            SetIPv6AddressInfo(&nextHopAddr,
                                ((adjRibInPtr[i].nextHop).interfaceAddr).ipv6);
                        }
                        interfaceIndex=interfaceIndex ;
                        if (interfaceIndex == NETWORK_UNREACHABLE )
                            interfaceIndex = DEFAULT_INTERFACE;
                        if (adjRibInPtr[i].asPathList)
                        {
                            pathSegmentLength =
                                adjRibInPtr[i].asPathList->pathSegmentLength / 2;
                        }
                        Ipv6UpdateForwardingTable(
                            node,
                            GetIPv6Address(adjRibInPtr[i].destAddress.prefix),
                            adjRibInPtr[i].destAddress.prefixLen,
                            GetIPv6Address(nextHopAddr),
                            interfaceIndex,//ANY_INTERFACE,
                            pathSegmentLength,
                            eprotocol);
            //legacy-false-v6-ends
                        if (bgp->redistribute2Ospf)
                        {
                            if (protocol == EXTERIOR_GATEWAY_PROTOCOL_EBGPv4 )
                              Ospfv3IsEnabled(
                              node,
                              GetIPv6Address(adjRibInPtr[i].destAddress.prefix),
                              adjRibInPtr[i].destAddress.prefixLen,
                              GetIPv6Address(adjRibInPtr[i].nextHop),
                              pathSegmentLength,
                              FALSE);
                       }

                        if (DEBUG_FORWARD_TABLE)
                        {
                            printf("Content of Forwarding Table After Update\n:");
                            Ipv6PrintForwardingTable(node);
                        }

                    }//IPv6


                }
            }
        }// else if (adjRibInPtr[i].pathAttrBest == BGP_PATH_ATTR_BEST_TRUE)
    }

    nextStartPtr = BUFFER_GetData(&bgp->ribLocal);
    numByteShift = nextStartPtr - prevStartPtr;

    if (nextStartPtr != prevStartPtr)
    {
        // If the rib local shifts to new memory location due to shortage
        // of allocation space modify the rib out pointers to reflect the
        // change.
        BgpConnectionInformationBase* bgpConnInfo =
            (BgpConnectionInformationBase*)
            BUFFER_GetData(&(bgp->connInfoBase));

        int numEntriesConn = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
        / sizeof(BgpConnectionInformationBase);

        int i = 0, j = 0;

        for (i = 0; i < numEntriesConn; i++)
        {

            BgpAdjRibLocStruct** adjRibOutPtr = (BgpAdjRibLocStruct **)
                BUFFER_GetData(&bgpConnInfo[i].adjRibOut);

            int numEntriesInRibOut = BUFFER_GetCurrentSize(&bgpConnInfo[i].
                    adjRibOut) / sizeof(BgpAdjRibLocStruct*);

            for (j = 0; j < numEntriesInRibOut; j++)
            {
                char* prevPtr = (char*) adjRibOutPtr[j];
                char* nextPtr = prevPtr + numByteShift;
                adjRibOutPtr[j] = (BgpAdjRibLocStruct*) nextPtr;
            }
        }
    }

    if (DEBUG_TABLE)
    {
        printf("\nRib Local after update was:\n");
        BgpPrintAdjRibLoc(node, bgp);
    }
}


//--------------------------------------------------------------------------
//  FUNCTION : BgpDecisionProcessPhase1
//
//  PURPOSE  : 1) Update AdjRibIn.
//             2) Select best routes
//             3) Mark the withdrawn routes as invalid that will go with next
//                update message.
//
// PARAMETERS : node, the node which is running the decision process
//              bgp,  bgp internal structure
//              connectionStatus, connection pointer which has received the
//                                update
//              update, the update message pointer
//              sizeOfNlri, the network layer reachability information
//
// RETURN :     None
//--------------------------------------------------------------------------
static
void BgpDecisionProcessPhase1(
    Node* node,
    BgpData* bgp,
    BgpConnectionInformationBase* connectionStatus,
    BgpUpdateMessage* update,
    int sizeOfNlri,
    int numNlri,
    int infeasibleRouteLength,
    int numWithdrawnRt,
    int numAttr)
{
    int i = 0, nextHopLen = 0;

    BgpRouteInfo* withdrawnRt = (BgpRouteInfo*) update->withdrawnRoutes;

    GenericAddress nextHop ;
    memset(&nextHop, 0,sizeof(GenericAddress) );

    int interfaceIndex = -1;

    GenericAddress destAddr;
    memset(&destAddr, 0,sizeof(GenericAddress) );

    char* prevStartLocation = BUFFER_GetData(&(bgp->adjRibIn));
    char* nextStartLocation = NULL;
    BOOL isLoopExists = FALSE, mp_found = FALSE;
    MpReachNlri* mpReach = NULL;
    BgpRouteInfo* nlri = NULL;

    // If the update does not contain any Network Layer reachability
    // information then there will not be any path attribute field
    if (sizeOfNlri != 0)
    {
        // The update contains MP_REACH_NLRI  information.
        for (i = 0; i < numAttr; i++)
        {
            if (update->pathAttribute[i].attributeType ==
                BGP_MP_REACH_NLRI)
            {
                mpReach = (MpReachNlri* )
               (update->pathAttribute[i].attributeValue);
                nlri = mpReach->Nlri;
                memcpy(&destAddr, mpReach->nextHop,mpReach->nextHopLen);
                nextHopLen = mpReach->nextHopLen;
                mp_found = TRUE;
                if (mpReach->afIden == MBGP_IPV6_AFI )
                {
                    numNlri = sizeOfNlri / BGP_IPV6_ROUTE_INFO_STRUCT_SIZE;
                }
                break;
            }
        }
        if (!mp_found)
        {
            nlri = update->networkLayerReachabilityInfo;
            for (i = 0; i < numAttr; i++)
            {
                if (update->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_NEXT_HOP)
                {
                    memcpy(&destAddr,
                          update->pathAttribute[i].attributeValue,
                          update->pathAttribute[i].attributeLength);
                    nextHopLen = update->pathAttribute[i].attributeLength;
                    break;
                }
            }
        }


        if (numNlri > 0)
        {
            int asPathLength = 0;
            for (i = 0; i < numAttr; i++)
            {
                if (update->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_AS_PATH)
                {
                    asPathLength = update->pathAttribute[i].attributeLength;
                    break;
                }
            }
            if (asPathLength)
            {
                isLoopExists = BgpIsLoopInUpdate(bgp, update);
            }
            else if (connectionStatus->asIdOfPeer == bgp->asId)
            {
                isLoopExists = TRUE;
                if (DEBUG)
                    printf("loop Detected\n");
            }
        }

        if (nextHopLen == 4 ) //next hop is IPv4
        {
            //the following function should be modified in MBGP
            NetworkGetInterfaceAndNextHopFromForwardingTable(node,
                                 destAddr.ipv4,
                                 &interfaceIndex,
                                 &nextHop.ipv4);
        }
        else if (nextHopLen == 16 ) //IPv6: only includes Global address
        {
            //check point,the Ipv6x function is not tested??
            Ipv6GetInterfaceAndNextHopFromForwardingTable(node,
                              destAddr.ipv6,
                              &interfaceIndex,
                              &nextHop.ipv6);
        }
        //what if next hop includes link local address as well?
        // If the next hop of UPDATE message is unreachable, discard message.
        in6_addr netUnreachAddr;
        memset(&netUnreachAddr,0,sizeof(in6_addr));//bgpInvalidAddr = 0;

        if (interfaceIndex == NETWORK_UNREACHABLE )
        {
            // Need to free update message
            //NetworkPrintForwardingTable(node);
            BgpFreeUpdateMessageStructure(update, numNlri);
            if (DEBUG)
            {
                printf("Next Hop in the received update is unreachable\n");

            }
            return;
        }

        if (DEBUG_TABLE)
        {
            printf("Content of adjRibIn before update in DP1\n");
            BgpPrintRoutingInformationBase(node, bgp);
        }
        if (isLoopExists)
        {
            if (DEBUG)
            {
                printf("\n Ignore the Update\n");
                BgpFreeUpdateMessageStructure(update, numNlri);
                return;
            }
        }
        for (i = 0; i < numNlri; i++)
        {
            // If the update message containg Network Layer Reachability
            // information then modify the routing information base
            // accordingly that is if routes are learned from same peer
            // modify previous entry and check for best route to the
            // destination. If there is already one entry in the routing
            // information base from different peer and if that entry is
            // best, compare with the new route if this is best
            // or not. If there is no previous entry for the destination
            // then add this entry in the Routing information base and mark
            //it as best route to the destination.
            if (!isLoopExists)
            {

                BgpModifyAdjRibInForNewUpdate(node,
                          bgp,
                          nlri[i],
                          update,
                          connectionStatus,
                          numAttr);
            }
            else
            {
                //we do not need to modify the following function
                BgpModifyAdjRibInForWithdrawnRoutes(node,
                            bgp,
                            nlri[i],
                            connectionStatus);
            }
        } // end for (i = 0; i < sizeNlri; i++)
    }


    if ((infeasibleRouteLength != 0) &&
        ((update->withdrawnRoutes) || (update->pathAttribute)))
    {

        MpUnReachNlri* mpUnreach = NULL;


        // The update contains MP_REACH_NLRI  information.
        if (update->withdrawnRoutes)
        {
            withdrawnRt = update->withdrawnRoutes;

        }
        else
        {
            if ((update->pathAttribute[0].attributeType ==
                  BGP_MP_UNREACH_NLRI) &&
              update->pathAttribute[0].attributeLength)
            {
                mpUnreach =
                   (MpUnReachNlri* ) update->pathAttribute[0].attributeValue;
            }
            else
            {
                if (connectionStatus->isInternalConnection)
                {
                    mpUnreach =
                   (MpUnReachNlri* ) update->pathAttribute[5].attributeValue;
                }
                else
                {
                    mpUnreach =
                   (MpUnReachNlri* ) update->pathAttribute[4].attributeValue;
                }
            }

            withdrawnRt = mpUnreach->Nlri;
        }
    }

    int j;
    for (i = 0; i < numWithdrawnRt; i++)
    {
        // If the Update message contains any withdrawn routes then modify
        // the routing table accordingly. ie. If there is any alternative
        // to the withdrawn destination find that or mark the destination as
        // invalid
        BOOL withdrawnRtFound = FALSE;
        for (j = 0; j < numNlri; j++)
        {
            if (Address_IsSameAddress(&nlri[j].prefix,
                &withdrawnRt[i].prefix))
            {
                withdrawnRtFound = TRUE;
                break;
            }
        }

        if (!withdrawnRtFound)
        {
            if (DEBUG)
            {
                printf("Invoking BgpModifyAdjRibInForWithdrawnRoutes\n");
            }
            BgpModifyAdjRibInForWithdrawnRoutes(node, bgp, withdrawnRt[i],
                        connectionStatus);
        }
    }

    // Everything has been done with the update message so free it now.
    BgpFreeUpdateMessageStructure( update, numNlri);

    if (DEBUG_TABLE)
    {
        printf("Content of adjRibIn after update in DP1\n");
        BgpPrintRoutingInformationBase(node, bgp);
    }

    // The following block of code is necessary as if the the size of the
    // routing information base exceeds previously allocated size then the
    // routing information base will be reallocated. Thus we need to change
    // the pointers of the Rib Local to point the entries of the newly
    // allocated Routing Information Base
    nextStartLocation = BUFFER_GetData(&(bgp->adjRibIn));

    if (prevStartLocation != nextStartLocation)
    {
        int i = 0;
        IntPtr numByteShift = nextStartLocation - prevStartLocation;

        // Shift all the addresses of AdjRibLocal
        int numEntriesInRibLoc = BUFFER_GetCurrentSize(&(bgp->ribLocal))
            / sizeof(BgpAdjRibLocStruct);
        BgpAdjRibLocStruct* ribLocPtr = (BgpAdjRibLocStruct*)
            BUFFER_GetData(&bgp->ribLocal);

        for (i = 0; i < numEntriesInRibLoc; i++)
        {
            char* prevPtr = (char*) ribLocPtr[i].ptrAdjRibIn;

            ribLocPtr[i].ptrAdjRibIn = (BgpRoutingInformationBase*)
                (prevPtr + numByteShift);
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION  BgpReconstructStructureFromPacket
//
// PURPOSE:  To construct update message structure back from raw data bytes
//           and running the decision process on it and scheduling update if
//           necessary to schedule update
//
// PARAMETERS:  node, the node which has received the update
//              bgp,  bgp internal structure
//              packet, the raw data byte that came to this node
//              size,   size of the packet came
//              connection, the connection pointer which has received the
//                          update
//
// RETURN:      None
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void BgpReconstructStructureFromPacket(
    Node* node,
    BgpData* bgp,
    char* packet,
    int size,
    BgpConnectionInformationBase* connection)
{
    int attrLength = 0;
    int numBytesInPrefix = 4;
    BgpUpdateMessage* updateMsg = NULL;

    int i = 0;
    int numAttr = 0;
    int mpNlriLen = 0,numNlri = 0;
    int numInfeasibleRts =0, nummpNlri = 0;
    int mpUnreachNlriLen = 0, nummpUnreachNlri = 0;
    int totalLength = 0;
    int hdrLength = 0;

    // allocate the update message structure
    updateMsg = (BgpUpdateMessage*) MEM_malloc(sizeof(BgpUpdateMessage));

    // Assign Marker
    memcpy(updateMsg->commonHeader.marker, packet, BGP_SIZE_OF_MARKER);
    packet += BGP_SIZE_OF_MARKER;

    memcpy(&updateMsg->commonHeader.length, packet, BGP_MESSAGE_LENGTH_SIZE);
    packet += BGP_MESSAGE_LENGTH_SIZE;

    memcpy(&updateMsg->commonHeader.type, packet, BGP_MESSAGE_TYPE_SIZE);
    packet += BGP_MESSAGE_TYPE_SIZE;
    EXTERNAL_ntoh(
                (void*)packet,
                BGP_INFEASIBLE_ROUTE_LENGTH_SIZE);

    // Assign infeasible Route Length
    memcpy(&updateMsg->infeasibleRouteLength, packet,
            BGP_INFEASIBLE_ROUTE_LENGTH_SIZE);
    packet += BGP_INFEASIBLE_ROUTE_LENGTH_SIZE;

    // Assign infeasible Routes

    //in MBGP it's assumed that updateMsg->infeasibleRouteLength is set to 0
    //withdrawn route info is carried in attribute parts
    //so => updateMsg->withdrawnRoutes = NULL;
    //check point??should be always zero
    if (updateMsg->infeasibleRouteLength)
    {
        BgpAllocateRoutes(bgp,
             &updateMsg->withdrawnRoutes,
             packet,
             updateMsg->infeasibleRouteLength,
             &numInfeasibleRts);
    }
    else
    {
        updateMsg->withdrawnRoutes = NULL;
    }

    // skip the update message's infeasible route length part
    packet = packet + updateMsg->infeasibleRouteLength;
    EXTERNAL_ntoh(
                (void*)packet,
                BGP_PATH_ATTRIBUTE_LENGTH_SIZE);

    memcpy(&updateMsg->totalPathAttributeLength,
        packet,
        BGP_PATH_ATTRIBUTE_LENGTH_SIZE);

    packet += BGP_PATH_ATTRIBUTE_LENGTH_SIZE;

    // Allocate the path attributes and assign the values
    attrLength = updateMsg->totalPathAttributeLength;

    totalLength = updateMsg->infeasibleRouteLength + attrLength
                  + BGP_SIZE_OF_UPDATE_MSG_EXCL_OPT;
    hdrLength = updateMsg->commonHeader.length;
    if (totalLength > hdrLength)
    {
        if (DEBUG_FSM)
        {
            char time[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), time);

            printf("node: %8u"
                    " state: ESTUBLISHED"
                    " event: BGP_RECEIVE_UPDATE_MESSAGE"
                    " activity: CLOSE_TRANSPORT_CONNECTION"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connection->remoteId);
        }

        BgpSendNotification(node,
                bgp,
                connection,
                BGP_ERR_CODE_UPDATE_MSG_ERR,
                BGP_ERR_SUB_CODE_UPDATE_MSG_MALFORMED_ATTR_LIST);

        APP_TcpCloseConnection(node,
                   connection->connectionId);

        BgpCloseConnection(node, bgp, connection);
    }


    // reduce the size of the packet by header length + infeasible route
    // length + total path attribute lenth + their length fields
    size = size - BGP_MIN_HDR_LEN - updateMsg->infeasibleRouteLength
           - updateMsg->totalPathAttributeLength
           - BGP_INFEASIBLE_ROUTE_LENGTH_SIZE
           - BGP_PATH_ATTRIBUTE_LENGTH_SIZE;

    //In current implementaion of MBGP,this field carries no info.
    updateMsg->networkLayerReachabilityInfo = NULL;

    if (attrLength == 0)
    {
        updateMsg->pathAttribute = NULL;
    }
    else
    {
        BOOL optionalbit = 0;
        BOOL transitiveBit = 0;
        BOOL partialBit = 0;
        BOOL isOriginAttrPresent = FALSE;
        BOOL isAsPathAttrPresent = FALSE;
        BOOL isNxtHopAttrPresent = FALSE;
        numAttr = GetNumAttr(attrLength, packet);

        // Allocate the space to assign the path attributes
        updateMsg->pathAttribute = (BgpPathAttribute*)
            MEM_malloc(numAttr * sizeof(BgpPathAttribute));
        memset(updateMsg->pathAttribute,0,sizeof(BgpPathAttribute)* numAttr);
        BOOL origin_seen = FALSE;
        BOOL originid_seen = FALSE;
        BOOL mpunreach_seen = FALSE;
        BOOL mpreach_seen = FALSE;
        BOOL localpref_seen = FALSE;
        BOOL med_seen = FALSE;
        BOOL nexthop_seen = FALSE;
        BOOL aspath_seen = FALSE;

        for (i = 0; i < numAttr; i++)
        {
            char* packetErrorData = NULL;
            int lengthErrorData = 0;
            // Assign the attr. flag part
            memcpy(&updateMsg->pathAttribute[i],
                packet,
                BGP_ATTRIBUTE_FLAG_SIZE);

            packet += BGP_ATTRIBUTE_FLAG_SIZE;

            optionalbit = BgpPathAttributeGetOptional
                    (updateMsg->pathAttribute[i].bgpPathAttr);
            transitiveBit = BgpPathAttributeGetTransitive
                    (updateMsg->pathAttribute[i].bgpPathAttr);
            partialBit = BgpPathAttributeGetPartial
                    (updateMsg->pathAttribute[i].bgpPathAttr);
            // Assign the attribute type
            memcpy(&updateMsg->pathAttribute[i].attributeType,
                   packet,
                   BGP_ATTRIBUTE_TYPE_SIZE);

            packet += BGP_ATTRIBUTE_TYPE_SIZE;
            BOOL xtendedlengthbit = BgpPathAttributeGetExtndLen(
                                   updateMsg->pathAttribute[i].bgpPathAttr);
            unsigned short lengthbytes;
            if (xtendedlengthbit)
            {
                lengthbytes = BGP_ATTRIBUTE_LENGTH_SIZE;
                EXTERNAL_ntoh(
                    (void*)packet,
                    BGP_ATTRIBUTE_LENGTH_SIZE);
            }
            else
            {
                lengthbytes = 1;
            }
            BOOL closeReturn = FALSE;
            if (optionalbit == BGP_WELL_KNOWN_ATTRIBUTE)
            {
                if (transitiveBit != BGP_TRANSITIVE_ATTRIBUTE)
                {
                    closeReturn = TRUE;
                }
                else if (partialBit != BGP_PATH_ATTR_COMPLETE)
                {
                    closeReturn = TRUE;
                }
            }
            else if (transitiveBit == BGP_NON_TRANSITIVE_ATTRIBUTE
                    && partialBit != BGP_PATH_ATTR_COMPLETE)
            {
                closeReturn = TRUE;
            }

            if (closeReturn == TRUE)
            {
                BgpSendNotification(node,
                    bgp,
                    connection,
                    BGP_ERR_CODE_UPDATE_MSG_ERR,
                    BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_FALG_ERR);

                APP_TcpCloseConnection(node,
                           connection->connectionId);

                BgpCloseConnection(node, bgp, connection);
                return;
            }
            // Assign the attribute values
            if (updateMsg->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_ORIGIN)
            {
                if (origin_seen)
                {
                    return;
                }
                origin_seen = TRUE;
                memcpy(&updateMsg->pathAttribute[i].attributeLength,
                    packet,
                    lengthbytes);
                if (optionalbit!= BGP_WELL_KNOWN_ATTRIBUTE)
                {
                    packetErrorData = packet - BGP_ATTRIBUTE_TYPE_SIZE;
                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;
                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_FALG_ERR,
                        packetErrorData,
                        lengthErrorData);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;

                }
                isOriginAttrPresent = TRUE;
                // Allocate origin

                packet = packet + lengthbytes;

                updateMsg->pathAttribute[i].attributeValue =
                    (char*) MEM_malloc(sizeof(OriginValue));

                memcpy(updateMsg->pathAttribute[i].attributeValue,
                       packet,
                       sizeof(OriginValue));

                packet = packet + sizeof(OriginValue);

                if (updateMsg->pathAttribute[i].attributeLength !=
                                                        sizeof(OriginValue))
                {
                    packetErrorData = packet -
                                      BGP_ATTRIBUTE_TYPE_SIZE -
                                      lengthbytes -
                                      sizeof(OriginValue);

                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;

                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_LEN_ERR,
                        packetErrorData,
                        lengthErrorData);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;
                }

                unsigned int attValue =
                    updateMsg->pathAttribute[i].attributeValue[0];
                if (attValue > BGP_ORIGIN_INCOMPLETE ||
                   attValue < BGP_ORIGIN_IGP )
                {
                    if (DEBUG_UPDATE)
                    {
                        char time[MAX_STRING_LENGTH];
                        TIME_PrintClockInSecond(node->getNodeTime(), time);
                        printf("node: %8u"
                          " state: ESTUBLISHED"
                          " event: BGP_RECEIVE_UPDATE_MESSAGE"
                          " activity:CLOSE_TRANSPORT_CONNECTION"
                          " next state: BGP_IDLE"
                          " time: %s remote-node: %u\n",
                          node->nodeId, time, connection->remoteId);
                    }
                    packetErrorData = packet -
                                      BGP_ATTRIBUTE_TYPE_SIZE -
                                      lengthbytes -
                                      sizeof(OriginValue);

                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;

                    BgpSendNotification(node,
                     bgp,
                     connection,
                     BGP_ERR_CODE_UPDATE_MSG_ERR,
                     BGP_ERR_SUB_CODE_UPDATE_MSG_INVALID_ORIGIN_ATTR,
                     packetErrorData,
                     lengthErrorData);

                    APP_TcpCloseConnection(node,
                           connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                }
            }
            else if (updateMsg->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_AS_PATH)
            {
                if (aspath_seen)
                {
                    return;
                }
                aspath_seen = TRUE;
                // Allocate as path
                AsPathValue* pathValue;
                memcpy(&updateMsg->pathAttribute[i].attributeLength,
                    packet,
                    lengthbytes);
                if (optionalbit!= BGP_WELL_KNOWN_ATTRIBUTE)
                {
                    packetErrorData = packet - BGP_ATTRIBUTE_TYPE_SIZE;
                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;

                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_FALG_ERR,
                        packetErrorData,
                        lengthErrorData);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;

                }

                isAsPathAttrPresent = TRUE;

                packet += lengthbytes;


                if (updateMsg->pathAttribute[i].attributeLength)
                {
                    packetErrorData = packet -
                                      BGP_ATTRIBUTE_TYPE_SIZE -
                                      lengthbytes;

                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;
                    updateMsg->pathAttribute[i].attributeValue = (char*)
                        MEM_malloc(sizeof(AsPathValue));

                    pathValue = (AsPathValue*) updateMsg->pathAttribute[i].
                                                attributeValue;

                    memcpy(&pathValue->pathSegmentType,
                           packet, BGP_PATH_SEGMENT_TYPE_SIZE);
                    packet += BGP_PATH_SEGMENT_TYPE_SIZE;

                    memcpy(&pathValue->pathSegmentLength,
                           packet, BGP_PATH_SEGMENT_LENGTH_SIZE);
                    packet += BGP_PATH_SEGMENT_LENGTH_SIZE;

                    pathValue->pathSegmentValue = (unsigned short*)
                        MEM_malloc(pathValue->pathSegmentLength *
                                   sizeof(unsigned short));
                    for (int j = 0; j < pathValue->pathSegmentLength; j++)
                    {
                        EXTERNAL_ntoh(
                                (void*)packet,
                                sizeof(unsigned short));
                        memcpy(pathValue->pathSegmentValue + j,
                           packet,
                           sizeof(unsigned short));
                        packet = packet + sizeof(unsigned short);
                    }
                    if (updateMsg->pathAttribute[i].attributeLength
                        != (pathValue->pathSegmentLength *
                            sizeof (unsigned short)
                            + 2 * (sizeof (unsigned char))))
                    {
                        BgpSendNotification(node,
                            bgp,
                            connection,
                            BGP_ERR_CODE_UPDATE_MSG_ERR,
                            BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_LEN_ERR,
                            packetErrorData,
                            lengthErrorData);

                        APP_TcpCloseConnection(node,
                                   connection->connectionId);

                        BgpCloseConnection(node, bgp, connection);

                        return;
                    }
                    if (!connection->isInternalConnection &&
                        *(pathValue->pathSegmentValue)!= connection->asIdOfPeer)
                    {
                        BgpSendNotification(node,
                            bgp,
                            connection,
                            BGP_ERR_CODE_UPDATE_MSG_ERR,
                            BGP_ERR_SUB_CODE_UPDATE_MSG_MALFORMED_AS_PATH);

                        APP_TcpCloseConnection(node,
                                   connection->connectionId);

                        BgpCloseConnection(node, bgp, connection);
                        return;
                    }
                }
            }
            else if (updateMsg->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_NEXT_HOP)
            {
                if (nexthop_seen)
                {
                    return;
                }
                nexthop_seen = TRUE;
                memcpy(&updateMsg->pathAttribute[i].attributeLength,
                       packet, lengthbytes);
                if (optionalbit!= BGP_WELL_KNOWN_ATTRIBUTE)
                {
                    packetErrorData = packet - BGP_ATTRIBUTE_TYPE_SIZE;
                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;
                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_FALG_ERR,
                        packetErrorData,
                        lengthErrorData);
                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;

                }
                isNxtHopAttrPresent = TRUE;
                // Allocate nextHop
                packet += lengthbytes;

                if (updateMsg->pathAttribute[i].attributeLength
                                                != sizeof(NextHopValue))
                {
                    packetErrorData = packet -
                                      BGP_ATTRIBUTE_TYPE_SIZE -
                                      lengthbytes;

                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;

                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_LEN_ERR,
                        packetErrorData,
                        lengthErrorData);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;
                }


                (updateMsg->pathAttribute[i].attributeValue) = (char*)
                    MEM_malloc(sizeof(NextHopValue));
                EXTERNAL_ntoh(
                        (void*)packet,
                        sizeof(NextHopValue));
                memcpy(updateMsg->pathAttribute[i].attributeValue,
                       packet,
                       sizeof(NextHopValue));
                NodeAddress myAddress = GetIPv4Address(connection->localAddr);
                NodeAddress next_hop;
                memcpy(&next_hop, updateMsg->pathAttribute[i].
                                    attributeValue, sizeof(NodeAddress));
                if (NetworkIpIsMulticastAddress(node, next_hop) || next_hop == ANY_DEST)
                {
                    packetErrorData = packet -
                                      BGP_ATTRIBUTE_TYPE_SIZE -
                                      lengthbytes;

                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;
                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_INVALID_NEXT_HOP_ATTR,
                        packetErrorData,
                        lengthErrorData);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;
                }
                packet = packet
                    + updateMsg->pathAttribute[i].attributeLength;
                if (myAddress == *((NodeAddress*) updateMsg->pathAttribute[i].
                    attributeValue))
                {
                    if (DEBUG)
                    {
                        printf("Ignoring this route as NETXT_HOP is same "
                            "as local BGP speaker Ip Address %d.%d.%d.%d\n",
                            (unsigned char)
                            updateMsg->pathAttribute[i].attributeValue[3],
                            (unsigned char)
                            updateMsg->pathAttribute[i].attributeValue[2],
                            (unsigned char)
                            updateMsg->pathAttribute[i].attributeValue[1],
                            (unsigned char)
                            updateMsg->pathAttribute[i].attributeValue[0]);
                    }
                    return;
                }
                /* If the UPDATE is received from an external AS,
                 * check if the NEXT-HOP is one of its directly connected
                 * destinations or not,If it is, but is different from the
                 * directly connected neighbor's AS address from which
                 * UPDATE is received, then check if the NEXT-HOP lies
                 * in the same subnet or not. If not, then we must ignore
                 * the UPDATE msg
                 * */
                if (connection->asIdOfPeer != bgp->asId)
                {
                    NetworkDataIp* ip = node->networkData.networkVar;
                    NetworkForwardingTable* rt = &(ip->forwardTable);
                    BOOL neglectRoot = 0;
                    for (int j = 0; j < rt->size; j++)
                    {
                        memcpy(&next_hop, updateMsg->pathAttribute[i].
                                    attributeValue, sizeof(NodeAddress));
                        next_hop = MaskIpAddress(
                            next_hop, rt->row[j].destAddressMask);
                        if (next_hop == rt->row[j].destAddress &&
                            rt->row[j].nextHopAddress == 0)
                        {
                            memcpy(&next_hop, updateMsg->pathAttribute[i].
                                    attributeValue, sizeof(NodeAddress));
                            if (connection->remoteAddr.interfaceAddr.ipv4 !=
                                next_hop)
                            {
                                neglectRoot = 1;
                                for (int k = 0; k < node->numberInterfaces; k++)
                                {
                                    if (NetworkIpCheckIpAddressIsInSameSubnet(
                                        node, k, next_hop))
                                    {
                                        neglectRoot = 0;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }
                    if (neglectRoot == 1)
                        return;
                }
            }
            else if (updateMsg->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC)
            {
                if (med_seen)
                {
                    return;
                }
                med_seen = TRUE;
                memcpy(&updateMsg->pathAttribute[i].attributeLength,
                       packet, lengthbytes);

                if (optionalbit == BGP_WELL_KNOWN_ATTRIBUTE
                    || transitiveBit == BGP_TRANSITIVE_ATTRIBUTE)
                {
                    packetErrorData = packet - BGP_ATTRIBUTE_TYPE_SIZE;
                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;

                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_FALG_ERR,
                        packetErrorData,
                        lengthErrorData);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;

                }

                // Allocate nextHop
                packet += lengthbytes;

                if (updateMsg->pathAttribute[i].attributeLength
                                                    != sizeof(MedValue))
                {
                    packetErrorData = packet -
                                      BGP_ATTRIBUTE_TYPE_SIZE -
                                      lengthbytes;

                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;

                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_LEN_ERR,
                        packetErrorData,
                        lengthErrorData);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;
                }
                (updateMsg->pathAttribute[i].attributeValue) = (char*)
                    MEM_malloc(sizeof(MedValue));
                EXTERNAL_ntoh(
                        (void*)packet,
                        sizeof(MedValue));
                memcpy(updateMsg->pathAttribute[i].attributeValue,
                       packet,
                       sizeof(MedValue));
                packet = packet
                    + updateMsg->pathAttribute[i].attributeLength;

            }
            else if (updateMsg->pathAttribute[i].attributeType ==
             BGP_PATH_ATTR_TYPE_LOCAL_PREF)
            {
                if (localpref_seen)
                {
                    return;
                }
                localpref_seen = TRUE;
                memcpy(&updateMsg->pathAttribute[i].attributeLength,
                       packet, lengthbytes);
                int errSubCode = 0;
                if (optionalbit != BGP_WELL_KNOWN_ATTRIBUTE)
                {
                    packetErrorData = packet - BGP_ATTRIBUTE_TYPE_SIZE;
                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;
                    errSubCode = BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_FALG_ERR;
                }
                // Allocate nextHop

                packet += lengthbytes;
                if (updateMsg->pathAttribute[i].attributeLength
                                                != sizeof(LocalPrefValue)
                   && errSubCode == 0)
                {
                    packetErrorData = packet -
                                      BGP_ATTRIBUTE_TYPE_SIZE -
                                      lengthbytes;

                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;
                    errSubCode = BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_LEN_ERR;
                }
                if (errSubCode != 0)
                {
                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        errSubCode);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;
                }
                (updateMsg->pathAttribute[i].attributeValue) = (char*)
                    MEM_malloc(sizeof(LocalPrefValue));
                EXTERNAL_ntoh(
                        (void*)packet,
                        sizeof(LocalPrefValue));
                memcpy(updateMsg->pathAttribute[i].attributeValue,
                       packet,
                       sizeof(LocalPrefValue));

                packet = packet
                    + updateMsg->pathAttribute[i].attributeLength;

            }
            else if (updateMsg->pathAttribute[i].attributeType ==
             BGP_MP_REACH_NLRI)
            {
                if (mpreach_seen)
                {
                    return;
                }
                mpreach_seen = TRUE;
                memcpy(&updateMsg->pathAttribute[i].attributeLength,
                       packet, lengthbytes);
                packet += lengthbytes;

                mpNlriLen = updateMsg->pathAttribute[i].attributeLength;
                MpReachNlri* mpReach =
                    (MpReachNlri*)MEM_malloc(sizeof(MpReachNlri));
                EXTERNAL_ntoh(
                                (void*)packet,
                                MBGP_AFI_SIZE);
                memcpy(&mpReach->afIden,packet, MBGP_AFI_SIZE);
                packet += MBGP_AFI_SIZE ;

                memcpy(&mpReach->subAfIden,packet,MBGP_SUB_AFI_SIZE);
                packet += MBGP_SUB_AFI_SIZE;

                memcpy(&mpReach->nextHopLen,packet,MBGP_NEXT_HOP_LEN_SIZE);
                packet += MBGP_NEXT_HOP_LEN_SIZE;

                //allocate next hop
                mpReach->nextHop = (char*) MEM_malloc( mpReach->nextHopLen );
                EXTERNAL_ntoh(
                                (void*)packet,
                                mpReach->nextHopLen);
                memcpy(mpReach->nextHop,packet, mpReach->nextHopLen);
                packet += mpReach->nextHopLen;

                memcpy( &mpReach->numSnpa,packet, MBGP_NUM_SNPA_SIZE);
                packet += MBGP_NUM_SNPA_SIZE;
                //NO SNPA IS COPIED,mpReach->numSnpa has been set to zero.

                mpNlriLen -= (MBGP_AFI_SIZE
                          + MBGP_SUB_AFI_SIZE
                          + MBGP_NEXT_HOP_LEN_SIZE
                          + MBGP_NUM_SNPA_SIZE
                          + mpReach->nextHopLen);

                //set a check point here to assert if mpReach->numSnpa !=0
                //in current implementaion of MBGP num of SNPA is set to zero
                mpReach->Snpa = NULL;
                if (mpReach->afIden == MBGP_IPV4_AFI )
                  numBytesInPrefix = 4;
                else if (mpReach->afIden == MBGP_IPV6_AFI )
                  numBytesInPrefix = 16;
                else
                    ERROR_ReportError("Unknown address family ientifier\n");

                //Allocate memory and copy the route info to mpReach->Nlri
                BgpAllocateRoutes(bgp,
                         &mpReach->Nlri,
                         packet,
                         mpNlriLen,
                         &nummpNlri);

                updateMsg->pathAttribute[i].attributeValue = (char* ) mpReach;
                        packet = packet + mpNlriLen;

            }
            else if (updateMsg->pathAttribute[i].attributeType ==
             BGP_MP_UNREACH_NLRI)
            {
                if (mpunreach_seen)
                {
                    return;
                }
                mpunreach_seen = TRUE;
                memcpy(&updateMsg->pathAttribute[i].attributeLength,
                       packet, lengthbytes);

                packet += lengthbytes;
                mpUnreachNlriLen =
                    updateMsg->pathAttribute[i].attributeLength;

                MpUnReachNlri* mpUnreach = (MpUnReachNlri*)
                    MEM_malloc(sizeof(MpUnReachNlri));

                //sizeof(AFI+ Sub_AFI )
                EXTERNAL_ntoh(
                                (void*)packet,
                                MBGP_AFI_SIZE);
                memcpy(&mpUnreach->afIden, packet, MBGP_AFI_SIZE);
                packet += MBGP_AFI_SIZE ;

                memcpy(&mpUnreach->subAfIden, packet, MBGP_SUB_AFI_SIZE);
                packet += MBGP_SUB_AFI_SIZE ;

                mpUnreachNlriLen -= (MBGP_AFI_SIZE
                             + MBGP_SUB_AFI_SIZE);

                if (mpUnreach->afIden == MBGP_IPV4_AFI )
                  numBytesInPrefix = 4;
                else if (mpUnreach->afIden == MBGP_IPV6_AFI )
                  numBytesInPrefix = 16;
                //copy the withdrawn route info to mpReach->Nlri

                BgpAllocateRoutes(bgp,
                         &mpUnreach->Nlri,
                         packet,
                         mpUnreachNlriLen,
                         &nummpUnreachNlri);

                updateMsg->pathAttribute[i].attributeValue =
                    (char* ) mpUnreach;
                packet = packet + mpUnreachNlriLen;
            }
            else if (updateMsg->pathAttribute[i].attributeType ==
                BGP_PATH_ATTR_TYPE_ORIGINATOR_ID)
            {
                if (originid_seen)
                {
                    return;
                }
                originid_seen = TRUE;
                memcpy(&updateMsg->pathAttribute[i].attributeLength,
                       packet, lengthbytes);

                if (optionalbit == BGP_WELL_KNOWN_ATTRIBUTE
                    || transitiveBit == BGP_TRANSITIVE_ATTRIBUTE)
                {
                    packetErrorData = packet - BGP_ATTRIBUTE_TYPE_SIZE;
                    lengthErrorData = BGP_ATTRIBUTE_TYPE_SIZE + lengthbytes +
                                updateMsg->pathAttribute[i].attributeLength;
                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_FALG_ERR);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;

                }

                // Allocate nextHop
                packet += lengthbytes;

                if (updateMsg->pathAttribute[i].attributeLength
                                                != sizeof(NodeAddress))
                {
                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_LEN_ERR);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;
                }
                (updateMsg->pathAttribute[i].attributeValue) = (char*)
                    MEM_malloc(sizeof(NextHopValue));
                EXTERNAL_ntoh(
                        (void*)packet,
                        sizeof(NextHopValue));
                memcpy(updateMsg->pathAttribute[i].attributeValue,
                       packet,
                       sizeof(NextHopValue));

                packet = packet
                    + updateMsg->pathAttribute[i].attributeLength;
                //if ORIGINATOR_ID attribute same as ROUTER_ID,
                // then ignore the route
                if ((bgp->isRtReflector)&&
                    (connection->localBgpIdentifier == (*((NodeAddress*)
                            updateMsg->pathAttribute[i].attributeValue))))
                {
                    if (DEBUG)
                    {
                        printf("Ignoring reflected route as originator id"
                            " same as BGP Id at RR %d\n",node->nodeId);
                    }
                    return;
                }
            }
            else
            {
                unsigned short attributeLength = 0;
                //ERROR_Assert(FALSE, "Unrecognized path attribute value\n");
                // Walk through the optional non transiitve attrs
                if (optionalbit == BGP_WELL_KNOWN_ATTRIBUTE)
                {
                    BgpSendNotification(node,
                        bgp,
                        connection,
                        BGP_ERR_CODE_UPDATE_MSG_ERR,
                        BGP_ERR_SUB_CODE_UPDATE_MSG_UNRECOGNIZED_WELL_KNOWN_ATTR);

                    APP_TcpCloseConnection(node,
                               connection->connectionId);

                    BgpCloseConnection(node, bgp, connection);
                    return;

                }

                memcpy(&attributeLength,
                       packet, lengthbytes);
                packet += lengthbytes;

                packet = packet
                    + attributeLength;
                updateMsg->totalPathAttributeLength =
                        updateMsg->totalPathAttributeLength -
                    (lengthbytes + attributeLength);
                updateMsg->commonHeader.length =
                        updateMsg->commonHeader.length -
                    (lengthbytes + attributeLength);
            }
        }
        // numAttr == 1 iff MP_UNREACH_NLRI comes
        if ((numAttr > 1) && (isOriginAttrPresent == FALSE
            || isAsPathAttrPresent == FALSE
            || (size != 0 && isNxtHopAttrPresent == FALSE)))
        {
            BgpSendNotification(node,
                    bgp,
                    connection,
                    BGP_ERR_CODE_UPDATE_MSG_ERR,
                    BGP_ERR_SUB_CODE_UPDATE_MSG_MISSING_WELL_KNOWN_ATTR);

            APP_TcpCloseConnection(node, connection->connectionId);

            BgpCloseConnection(node, bgp, connection);
            return;

        }

        // Allocate NLRI
        //This part is not required for MBGP
        if (size)
        {
            BgpAllocateRoutes(bgp,
            &updateMsg->networkLayerReachabilityInfo,
            packet,
            size,
            &numNlri);
        }

    }
    if (bgp->ip_version == NETWORK_IPV6 )
    {
        updateMsg->infeasibleRouteLength =
            (unsigned short)numInfeasibleRts *
            BGP_IPV6_ROUTE_INFO_STRUCT_SIZE;
    }
    else
    {
        updateMsg->infeasibleRouteLength =
            (unsigned short)numInfeasibleRts *
            BGP_IPV4_ROUTE_INFO_STRUCT_SIZE;
    }

    if (DEBUG_UPDATE)
    {

        char updateTime[MAX_CLOCK_TYPE_STR_LEN];
        ctoa(node->getNodeTime(), updateTime);

        printf("Node %u received an update from %u sim-time %s ns\n",
               node->nodeId,
               connection->remoteId,
               updateTime);

        BgpPrintStruct(node, updateMsg, bgp, FALSE);
    }

    if (bgp->ip_version == NETWORK_IPV6)
    {
        //process the received Update message;
        //next hop,entries of route info,entries of withdrwan routes
        BgpDecisionProcessPhase1(
            node,
            bgp,
            connection,
            updateMsg,
            mpNlriLen,      // size == sizeof nlri in updateMsg.
            nummpNlri,
            mpUnreachNlriLen, //size of withdrwan route
            nummpUnreachNlri,
            numAttr);
    }
    else
    {
        // process the received Update message;next hop,entries of route
        // info,entries of withdrwan routes
        BgpDecisionProcessPhase1(
            node,
            bgp,
            connection,
            updateMsg,
            size,      // size == sizeof nlri in updateMsg.
            numNlri,
            updateMsg->infeasibleRouteLength, //size of withdrwan route
            numInfeasibleRts,
            numAttr);

    }

    //process Adj-Rib-in for newly added/updated entries
    BgpDecisionProcessPhase2(node,bgp);

    // Apply decision process phase 3
    BgpDecisionProcessPhase3(node,bgp);

}

//--------------------------------------------------------------------------
// FUNCTION BgpCloseConnection
//
// PURPOSE  This will be used if the connection with one peer is closed
//          anyway. The structure which stores all the informations will
//          be cleared and the routing table entries will be freed and
//          a start event will be generated exponentially increasing the
//          previous start event time
//
// PARAMETERS : node, the node for which one connection will be closed
//              bgp,  the bgp internal structure
//              connection, connection pointer to be closed
//
// RETURN   None
//--------------------------------------------------------------------------

void BgpCloseConnection (
    Node* node,
    BgpData* bgp,
    BgpConnectionInformationBase* connection)
{
    clocktype delay = 0;
    Message* newMsg = NULL;
    int i = 0;

    BgpRoutingInformationBase* ribInPtr = (BgpRoutingInformationBase*)
    BUFFER_GetData(&bgp->adjRibIn);

    int numEntriesInRibIn = BUFFER_GetCurrentSize(&bgp->adjRibIn)
    / sizeof(BgpRoutingInformationBase);

    if (DEBUG_COLLISION)
    {
        printf("Inside closeConnection\n");
        BgpPrintConnInformationBase(node, bgp);
    }
    if (connection->state == BGP_ESTABLISHED)
    {
        for (i = 0; i < numEntriesInRibIn; i++)
        {
            //MBGP if (ribInPtr[i].peerAddress == connection->remoteAddr)
            if (Address_IsSameAddress(&ribInPtr[i].peerAddress,
                  &connection->remoteAddr))
            {
                BgpRouteInfo withdrawnRt;
                //MBGP withdrawnRt.prefix = ribInPtr[i].destAddress.prefix;
                memcpy((char*) &withdrawnRt.prefix,
                    (char*)&ribInPtr[i].destAddress.prefix,
                    sizeof(Address));

                withdrawnRt.prefixLen = ribInPtr[i].destAddress.prefixLen;

                BgpModifyAdjRibInForWithdrawnRoutes(
                    node,
                    bgp,
                    withdrawnRt,
                    connection);
            }
        }

        BgpDecisionProcessPhase2(node, bgp);
        BgpDecisionProcessPhase3(node, bgp);
    }
    connection->state = BGP_IDLE;

    // Delete all the previous informations of the connection
    connection->localPort = (unsigned short) BGP_INVALID_ID;

    // Cancell all the timers
    connection->connectRetrySeq++;
    connection->keepAliveTimerSeq++;
    connection->holdTimerSeq++;
    connection->nextUpdate = 0;

    // remove the bgp identifier
    connection->remoteBgpIdentifier = 0;

    if (connection->buffer.currentSize)
    {
        MEM_free(connection->buffer.data);
        connection->buffer.data = NULL;
        connection->buffer.currentSize = 0;
        connection->buffer.expectedSize = 0;
    }

    // to generate a random number to avoid synchronization
    delay = RANDOM_nrand(bgp->timerSeed) % BGP_RANDOM_TIMER;

    // Send start event after exponentially increasing the previous
    // start event time.
    delay += connection->startEventTime;

    // Exponentially increase the start event time
    connection->startEventTime *= 2;

    // After expiration of start timer bgp will
    // try to open a tcp connection
    newMsg = MESSAGE_Alloc(
    node,
    APP_LAYER,
    APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
    MSG_APP_BGP_StartTimer);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(int));

    // Include the unique id of the connection in the info
    // part so that when the start timer expires the node
    // may know to which not it is supposed to open connection
    memcpy(MESSAGE_ReturnInfo(newMsg), &connection->uniqueId, sizeof(int));

    // Send the start timer expired message with
    // a small random delay
    MESSAGE_Send(node, newMsg, delay);

    connection->connectionId = BGP_INVALID_ID;

    if (BUFFER_GetCurrentSize(&(connection->withdrawnRoutes)) > 0)
    {
        BUFFER_ClearDataFromDataBuffer(
            &(connection->withdrawnRoutes),
            (char*) BUFFER_GetData(&(connection->withdrawnRoutes)),
            BUFFER_GetCurrentSize(&(connection->withdrawnRoutes)),
            FALSE);
    }
    if (BUFFER_GetCurrentSize(&(connection->adjRibOut)) > 0)
    {
        BUFFER_ClearDataFromDataBuffer(
            &(connection->adjRibOut),
            (char*) BUFFER_GetData(&(connection->adjRibOut)),
            BUFFER_GetCurrentSize(&(connection->adjRibOut)),
            FALSE);
    }
}


//--------------------------------------------------------------------------
// FUNCTION BgpHandleNotification
//
// PURPOSE  This function will handle the notification message came in this
//          connection and close the tcp connection and release resources and
//          generate start event timer with exponentially increasing the
//          delay whereever necessary
//
// PARAMETERS
//      node, the node received the Notification
//      bgp,  the bgp internal structure
//      connection, the connection for which the notification came
//      packet, the notification packet
//
// RETURN
//      void
//
// NOTE This function will delete the connection and will not generate any
//      start event if the notification code is cease
//--------------------------------------------------------------------------

static
void BgpHandleNotification(
    Node* node,
    BgpData* bgp,
    BgpConnectionInformationBase* connection,
    char* packet)
{
    char* errCode = packet + BGP_MIN_HDR_LEN;
    char* subErrCode = packet + BGP_MIN_HDR_LEN + sizeof(char);
    int errorType = *errCode;
    int errorSubType = *subErrCode;

    switch (errorType)
    {
        case BGP_ERR_CODE_CEASE:
        {
            if (DEBUG)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);
                printf("\tAt %s Received notification error cease\n", time);
            }
            APP_TcpCloseConnection(node, connection->connectionId);
            BgpCloseConnection(node, bgp, connection);
            break;
        }

        case BGP_ERR_CODE_MSG_HDR_ERR:
        {
            if (DEBUG)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);
                printf("\tAt %s Received notification mesheader error"
                    " with sub error type %d\n", time, errorSubType);
            }
            APP_TcpCloseConnection(node, connection->connectionId);
            BgpCloseConnection(node, bgp, connection);
            break;
        }
        case BGP_ERR_CODE_OPEN_MSG_ERR:
        {
            if (DEBUG)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);
                printf("\tAt %s Received notification open message error"
                    " with sub error type %d\n", time, errorSubType);
            }

            APP_TcpCloseConnection(node, connection->connectionId);
            BgpCloseConnection(node, bgp, connection);
            break;
        }


        case BGP_ERR_CODE_UPDATE_MSG_ERR:
        {
            if (DEBUG)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);
                 printf("\tAt %s  Received notification update message error"
                   " with sub error type %d\n", time, errorSubType);
            }

            APP_TcpCloseConnection(node, connection->connectionId);
            BgpCloseConnection(node, bgp, connection);
            break;
        }
        case BGP_ERR_CODE_HOLD_TIMER_EXPIRED_ERR:
        case BGP_ERR_CODE_FINITE_STATE_MACHINE_ERR:
        {
            if (DEBUG)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);
                printf("\tAt %s Received notification other than cease"
                     " with sub error type %d\n", time, errorSubType);
            }

            APP_TcpCloseConnection(node, connection->connectionId);
            BgpCloseConnection(node, bgp, connection);
            break;
        }

        default:
        {
            ERROR_ReportWarning("Unrecognized Error type received in "
                    "Notification message\n");
            APP_TcpCloseConnection(node, connection->connectionId);
            BgpCloseConnection(node, bgp, connection);
            break;
        }
    } // end switch
} // end handle notification


//--------------------------------------------------------------------------
// NAME:        BgpProcessEstablishedState.
//
// PURPOSE:     Handling all event types at established state
//
// PARAMETERS:  node, node which is receiving any message
//              bgp, the bgp internal structure
//              packet,  bgp Packet pointer
//              event, the event type of the message
//              uniqueId, bgp maintains one id for each neighbor this is for
//                        which connection the event type is meant for
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//--------------------------------------------------------------------------

static
void BgpProcessEstablishedState(
    Node* node,
    BgpData* bgp,
    char* packet,
    BgpEvent event,
    BgpConnectionInformationBase* connectionStatus)
{

    switch (event)
    {
        case BGP_START:
        {
            // Do nothing. Should not come into this state with this event
            // type
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_ESTUBLISHED"
                    " event: BGP_START"
                    " activity: DO_NOTHING"
                    " next state : BGP_ESTUBLISHED"
                    " time: %s ns remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            break;
        }
        case BGP_KEEPALIVE_TIMER_EXPIRED:
        {
            // Restart KeepAlive timer if negotiated Hold Timer
            // value is non-zero. basically we don't need to check for the
            // hold timer value as if only its value is non zero the timer
            // will be scheduled otherwise not

            // Send KEEPALIVE message.

            if (connectionStatus->holdTime)
            {
                // If hold timer not equal to 0 schedule self time for
                // keep alive
                Message* newMsg = NULL;
                clocktype delay = (clocktype)((0.75 * bgp->keepAliveInterval)
                    + RANDOM_erand (bgp->timerSeed)
                    * (0.25 * bgp->keepAliveInterval));

                if (DEBUG_FSM)
                {
                    char time[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), time);

                    printf("node: %8u"
                        " state: BGP_ESTUBLISHED"
                        " event: BGP_KEEPALIVE_TIMER_EXPIRED"
                        " activity: SEND_KEEP_ALIVE"
                        " next state: BGP_ESTUBLISHED"
                        " time: %s ns remote-node: %u\n",
                        node->nodeId, time, connectionStatus->remoteId);
                }

                newMsg = MESSAGE_Alloc(
                             node,
                             APP_LAYER,
                             APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                             MSG_APP_BGP_KeepAliveTimer);

                MESSAGE_InfoAlloc(node, newMsg, sizeof(ConnectionInfo));

                ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                    connectionStatus->uniqueId;
                ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                    connectionStatus->keepAliveTimerSeq;

                MESSAGE_Send(node, newMsg, delay);

                BgpSendKeepAlivePackets(
                    node,
                    bgp,
                    connectionStatus);
            }
            break;
        }
        case BGP_RECEIVE_KEEPALIVE_MESSAGE:
        {
            Message* holdTimer = NULL;

            // Received keep alive message so cancel the hold timer
            connectionStatus->holdTimerSeq++;

            if (connectionStatus->holdTime)
            {
                if (DEBUG_FSM)
                {
                    char time[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), time);

                    printf("node: %8u"
                        " state: BGP_ESTUBLISHED"
                        " event: BGP_RECEIVE_KEEPALIVE_MESSAGE"
                        " activity: RESTART HOLD TIMER "
                        " next state : BGP_ESTUBLISHED"
                        " time: %s remote-node: %u\n",
                        node->nodeId, time, connectionStatus->remoteId);
                }

                // Restart Hold Timer if negotiated Hold Timer value is
                // non-zero.Cancel the previous timer
                holdTimer = MESSAGE_Alloc(
                                node,
                                APP_LAYER,
                                APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                                MSG_APP_BGP_HoldTimer);

                MESSAGE_InfoAlloc(node, holdTimer, sizeof(ConnectionInfo));

                ((ConnectionInfo*) MESSAGE_ReturnInfo(holdTimer))->uniqueId =
                    connectionStatus->uniqueId;

                ((ConnectionInfo*) MESSAGE_ReturnInfo(holdTimer))->sequence =
                    connectionStatus->holdTimerSeq;

                MESSAGE_Send(node, holdTimer, connectionStatus->holdTime);
            }
            break;
        }
        case BGP_RECEIVE_UPDATE_MESSAGE:
        {
            int packetSize = 0;

            // Cancel the previous hold timer
            connectionStatus->holdTimerSeq++;

            if (connectionStatus->holdTime)
            {
                // the hold time is not zero so re-schedule hold timer
                Message* holdTimer = NULL;

                if (DEBUG_FSM)
                {
                    char time[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), time);

                    printf("node: %8u"
                        " state: BGP_ESTUBLISHED"
                        " event: BGP_RECEIVE_UPDATE_MESSAGE"
                        " activity: PROCESS UPDATE (decision process)"
                        " next state: BGP_ESTUBLISHED"
                        " time: %s remote-node %u\n",
                        node->nodeId, time, connectionStatus->remoteId);
                }

                holdTimer = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                            MSG_APP_BGP_HoldTimer);

                MESSAGE_InfoAlloc(node, holdTimer, sizeof(ConnectionInfo));

                ((ConnectionInfo*) MESSAGE_ReturnInfo(holdTimer))->uniqueId =
                    connectionStatus->uniqueId;

                ((ConnectionInfo*) MESSAGE_ReturnInfo(holdTimer))->sequence =
                    connectionStatus->holdTimerSeq;

                MESSAGE_Send(node, holdTimer, connectionStatus->holdTime);
            }

            // This function will update the routing table as well

            packetSize = BgpGetPacketSize(packet);

            BgpReconstructStructureFromPacket(node,
                                              bgp,
                                              packet,
                                              packetSize,
                                              connectionStatus);
            break;
        }
        case SEND_UPDATE:
        {
            // Send update message to the peer and restart the
            // keepalive timer.
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_ESTUBLISHED"
                    " event: SEND_UPDATE"
                    " activity: SENDING_UPDATE_MESSAGE"
                    " next state: BGP_ESTUBLISHED"
                    " time: %s remote-node %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            ERROR_Assert(connectionStatus->updateTimerAlreadySet,
                         "BGPv4: Update timer error!\n");

            BgpSendUpdateMessage(node, bgp, connectionStatus);
            break;
        }
        case BGP_RECEIVE_NOTIFICATION_MESSAGE:
        {
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_ESTUBLISHED"
                    " event: BGP_RECEIVE_NOTIFICATION_MESSAGE"
                    " activity: PROCESSING_NOTIFICATION"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            BgpHandleNotification(node, bgp, connectionStatus, packet);
            break;
        }
        case BGP_TRANSPORT_CONNECTION_CLOSED:
        {
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: ESTUBLISHED"
                    " event: BGP_TRANSPORT_CONNECTION_CLOSED"
                    " activity: CLOSE_TRANSPORT_CONNECTION"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            BgpCloseConnection(node, bgp, connectionStatus);
            break;
        }
        case BGP_HOLD_TIMER_EXPIRED:
        {
            // Closed transport connection.
            // Release resources.
            // Send NOTIFICATION message with error code Hold Timer Expired.
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: ESTUBLISHED"
                    " event: BGP_HOLD_TIMER_EXPIRED"
                    " activity: CLOSE_TRANSPORT_CONNECTION"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            BgpSendNotification(node,
                bgp,
                connectionStatus,
                BGP_ERR_CODE_HOLD_TIMER_EXPIRED_ERR,
                BGP_ERR_SUB_CODE_DEFAULT);

            APP_TcpCloseConnection(node,
                   connectionStatus->connectionId);

            BgpCloseConnection(node, bgp, connectionStatus);

            break;
        }
        case BGP_RECEIVE_OPEN_MESSAGE:
        {
            if (!BgpCheckIfOpenMessageError(node, bgp, packet, connectionStatus))
            {
                // Got one error free open message
                unsigned short holdTime = 0;
                char* data;
                char* tempPktPtr;
                NodeAddress remoteBgpIdentifier = 0;
                BOOL isCollision = FALSE;
                unsigned short asId;

                if (DEBUG_FSM)
                {
                    char time[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), time);

                    printf("node: %8u"
                        " state: BGP_ESTABLISHED"
                        " event: BGP_RECEIVE_OPEN_MESSAGE"
                        " activity: DETECT_COLLISION"
                        " next state: BGP_ESTABLISHED"
                        " time: %s remote-node: %u\n",
                        node->nodeId, time, connectionStatus->remoteId);
                }

                if (DEBUG_OPEN_MSG)
                {
                    printf("Receiving an open message\n");
                    BgpPrintOpenMessage(node, packet);
                }

                data = packet;
                tempPktPtr = data;

                tempPktPtr += BGP_MIN_HDR_LEN + BGP_VERSION_FIELD_LENGTH;

                memcpy(&asId, tempPktPtr, BGP_ASID_FIELD_LENGTH);

                tempPktPtr += BGP_ASID_FIELD_LENGTH;
                memcpy(&holdTime, tempPktPtr, BGP_HOLDTIMER_FIELD_LENGTH);

                tempPktPtr += BGP_HOLDTIMER_FIELD_LENGTH;
                memcpy(&remoteBgpIdentifier,
                    tempPktPtr, sizeof(NodeAddress));

                BgpCheckForCollision(
                    node,
                    bgp,
                    remoteBgpIdentifier,
                    connectionStatus,
                    &isCollision,
                    holdTime,
                    asId);
            }
            break;
        }
        case BGP_TRANSPORT_CONNECTION_OPEN:
        case BGP_TRANSPORT_CONNECTION_OPEN_FAILED:
        case BGP_CONNECTRETRY_TIMER_EXPIRED:
        {
            // Sending Notification and closing the transport connection
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_ESTUBLISHED"
                    " event: AN_UNDESIRABLE_EVENT"
                    " activity: DISCONNECT_AND_SEND_FSM_ERR NOTIFICATION"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            BgpSendNotification(node,
                bgp,
                connectionStatus,
                BGP_ERR_CODE_FINITE_STATE_MACHINE_ERR,
                BGP_ERR_SUB_CODE_DEFAULT);

            APP_TcpCloseConnection(node,
                connectionStatus->connectionId);
            BgpCloseConnection(node, bgp, connectionStatus);

            break;
        }
        default:
        {
            // Shouldn't get to here.
            ERROR_Assert(FALSE,"Unknown event\n");
        }
    }
}


//--------------------------------------------------------------------------
// NAME:        BgpProcessOpenConfirmState.
//
// PURPOSE:     Handling all event types at open confirm state
//
// PARAMETERS:  node, node which is receiving any message
//              bgp, the bgp internal structure
//              packet,  bgp Packet pointer
//              event, the event type of the message
//              uniqueId, bgp maintains one id for each neighbor this is for
//                        which connection the event type is meant for
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//--------------------------------------------------------------------------

static
void BgpProcessOpenConfirmState(
    Node* node,
    BgpData* bgp,
    char* data,
    BgpEvent event,
    BgpConnectionInformationBase* connectionStatus)
{
    switch (event)
    {
        case BGP_START:
        {
            // Do nothing. This event type should not come here
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_CONFIRM"
                    " event: BGP_START"
                    " activity: DO NOTHING"
                    " next state:BGP_OPEN_CONFIRM"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            break;
        }
        case BGP_KEEPALIVE_TIMER_EXPIRED:
        {
            // Send KEEPALIVE message.
            Message* newMsg = NULL;
            clocktype delay = (clocktype)((0.75 * bgp->keepAliveInterval)
                + RANDOM_erand (bgp->timerSeed)
                * (0.25 * bgp->keepAliveInterval));


            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_CONFIRM"
                    " event: BGP_KEEPALIVE_TIMER_EXPIRED"
                    " activity: SEND KEEP ALIVE MESSAGE"
                    " next state: BGP_OPEN_CONFIRM"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            newMsg = MESSAGE_Alloc(node, APP_LAYER,
                   APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                   MSG_APP_BGP_KeepAliveTimer);

            MESSAGE_InfoAlloc(node, newMsg, sizeof(ConnectionInfo));

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                connectionStatus->uniqueId;

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                connectionStatus->keepAliveTimerSeq;

            MESSAGE_Send(node, newMsg, delay);

            BgpSendKeepAlivePackets(node, bgp, connectionStatus);

            break;
        }
        case BGP_RECEIVE_KEEPALIVE_MESSAGE:
        {
            //  Complete initialization.
            int routeToAdvertise = 0;

            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_CONFIRM"
                    " event: BGP_RECEIVE_KEEPALIVE_MESSAGE"
                    " activity: RESET HOLD TIMER OR SEND UPDATE IF ANY"
                    " next state: BGP_ESTABLISHED"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            connectionStatus->state = BGP_ESTABLISHED;
            if (DEBUG_COLLISION)
            {
                BgpPrintConnInformationBase(node, bgp);
            }
            // restart the hold timer
            // cancel the previous timer and raise a new one if the
            // negotiated hold time is not zero
            connectionStatus->holdTimerSeq++;

            if (connectionStatus->holdTime)
            {
                // hold timer value is not 0 so reschedule the hold timer
                Message* newMsg = NULL;
                newMsg = MESSAGE_Alloc(
                    node,
                    APP_LAYER,
                    APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                    MSG_APP_BGP_HoldTimer);

                MESSAGE_InfoAlloc(node, newMsg,
                    sizeof(ConnectionInfo));

                ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                    connectionStatus->uniqueId;

                ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                    connectionStatus->holdTimerSeq;

                MESSAGE_Send(node, newMsg, connectionStatus->holdTime);
            }

            // Copy the content of ribLoc to adjRibOut
            BgpCopyRtToRibOut(bgp, &(connectionStatus->adjRibOut),
                connectionStatus);

            // Find if there is any route to advertise
            routeToAdvertise =
                BUFFER_GetCurrentSize(&(connectionStatus->adjRibOut))
                    / sizeof(BgpAdjRibLocStruct*);

            if (routeToAdvertise)
            {
                BgpSendUpdateMessage(node, bgp, connectionStatus);
            }
            if (DEBUG)
            {
                BgpPrintConnInformationBase(node, bgp);
            }
            break;
        }
        case BGP_RECEIVE_NOTIFICATION_MESSAGE:
        {

            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_CONFIRM"
                    " event: BGP_RECEIVE_NOTIFICATION_MESSAGE"
                    " activity: PROCESSING_NOTIFICATION"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            BgpHandleNotification(node, bgp, connectionStatus, data);
            break;
        }
        case BGP_HOLD_TIMER_EXPIRED:
        {
            // Close transport connection.
            // Release resources.
            // Send NOTIFICATION message with error code Hold Timer Expired.
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_CONFIRM"
                    " event: BGP_HOLD_TIMER_EXPIRED"
                    " activity: SEND_NOTIFICATION DISCONNECT"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            BgpSendNotification(
                node,
                bgp,
                connectionStatus,
                BGP_ERR_CODE_HOLD_TIMER_EXPIRED_ERR,
                BGP_ERR_SUB_CODE_DEFAULT);

            APP_TcpCloseConnection(node, connectionStatus->connectionId);
            BgpCloseConnection(node, bgp, connectionStatus);

            break;
        }
        case BGP_TRANSPORT_CONNECTION_CLOSED:
        {
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_CONFIRM"
                    " event: BGP_TRANSPORT_CONNECTION_CLOSED"
                    " activity: DISCONNECT"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            BgpCloseConnection(node, bgp, connectionStatus);

            break;
        }
        case BGP_RECEIVE_OPEN_MESSAGE:
        {
            if (!BgpCheckIfOpenMessageError(node, bgp, data, connectionStatus))
            {
                // Got one error free open message
                unsigned short holdTime = 0;
                char* packet;
                char* tempPktPtr;
                NodeAddress remoteBgpIdentifier = 0;
                BOOL isCollision = FALSE;
                unsigned short asId;

                if (DEBUG_FSM)
                {
                    char time[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), time);

                    printf("node: %8u"
                        " state: BGP_OPEN_CONFIRM"
                        " event: BGP_RECEIVE_OPEN_MESSAGE"
                        " activity: DETECT_COLLISION"
                        " next state: BGP_OPEN_CONFIRM"
                        " time: %s remote-node: %u\n",
                        node->nodeId, time, connectionStatus->remoteId);
                }

                if (DEBUG_OPEN_MSG)
                {
                    printf("Receiving an open message\n");
                    BgpPrintOpenMessage(node, data);
                }

                packet = data;
                tempPktPtr = packet;

                tempPktPtr += BGP_MIN_HDR_LEN + BGP_VERSION_FIELD_LENGTH;

                memcpy(&asId, tempPktPtr, BGP_ASID_FIELD_LENGTH);

                tempPktPtr += BGP_ASID_FIELD_LENGTH;
                memcpy(&holdTime, tempPktPtr, BGP_HOLDTIMER_FIELD_LENGTH);

                tempPktPtr += BGP_HOLDTIMER_FIELD_LENGTH;
                memcpy(&remoteBgpIdentifier,
                    tempPktPtr, sizeof(NodeAddress));

                BgpCheckForCollision(
                    node,
                    bgp,
                    remoteBgpIdentifier,
                    connectionStatus,
                    &isCollision,
                    holdTime,
                    asId);
            }
            break;
        }
        case BGP_CONNECTRETRY_TIMER_EXPIRED:
        case BGP_TRANSPORT_CONNECTION_OPEN:
        case BGP_TRANSPORT_CONNECTION_OPEN_FAILED:
        case BGP_RECEIVE_UPDATE_MESSAGE:
        {
            // Sending Notification and closing the transport connection
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_CONFIRM"
                    " event: AN_UNDESIRABLE_EVENT"
                    " activity: DISCONNECT_AND_SEND_FSM_ERR NOTIFICATION"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            BgpSendNotification(
               node,
               bgp,
               connectionStatus,
               BGP_ERR_CODE_FINITE_STATE_MACHINE_ERR,
               BGP_ERR_SUB_CODE_DEFAULT);

            APP_TcpCloseConnection(node, connectionStatus->connectionId);
            BgpCloseConnection(node, bgp, connectionStatus);

            break;
        }
        default :
        {
            ERROR_Assert(FALSE, "Unknown event\n");
        }
    }
}


//--------------------------------------------------------------------------
// NAME:        BgpProcessOpenSentState.
//
// PURPOSE:     Handling all event types at open sent state
//
// PARAMETERS:  node, node which is receiving any message
//              bgp, the bgp internal structure
//              data,  bgp Packet pointer
//              event, the event type of the message
//              uniqueId, bgp maintains one id for each neighbor this is for
//                        which connection the event type is meant for
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//--------------------------------------------------------------------------

static
void BgpProcessOpenSentState(
    Node* node,
    BgpData* bgp,
    char* data,
    BgpEvent event,
    BgpConnectionInformationBase* connPtr)
{
    switch (event)
    {
        case BGP_START:
        {
            // Do nothing. But this event type should not come here
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_SENT"
                    " event: BGP_START"
                    " activity: DO NOTHING"
                    " next state: BGP_OPEN_SENT"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connPtr->remoteId);
            }
            break;
        }
        case BGP_RECEIVE_OPEN_MESSAGE:
        {
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_SENT"
                    " event: BGP_RECEIVE_OPEN_MESSAGE"
                    " activity: DETECT_COLLISION SEND KEEP ALIVE"
                    " next state: BGP_OPEN_CONFIRM"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connPtr->remoteId);
            }
            if (DEBUG_OPEN_MSG)
            {
                printf("Receiving an open message\n");
                BgpPrintOpenMessage(node, data);
            }
            if (!BgpCheckIfOpenMessageError(node, bgp, data, connPtr))
            {
                // Got one error free open message
                Message* newMsg = NULL;
                unsigned short holdTime = 0;
                char* packet;
                char* tempPktPtr;
                NodeAddress remoteBgpIdentifier = 0;
                BOOL isCollision = FALSE;

                // Send KEEPALIVE message.
                unsigned short asId;

                packet = data;
                tempPktPtr = packet;

                tempPktPtr += BGP_MIN_HDR_LEN + BGP_VERSION_FIELD_LENGTH;

                memcpy(&asId, tempPktPtr, BGP_ASID_FIELD_LENGTH);

                tempPktPtr += BGP_ASID_FIELD_LENGTH;
                memcpy(&holdTime, tempPktPtr, BGP_HOLDTIMER_FIELD_LENGTH);

                tempPktPtr += BGP_HOLDTIMER_FIELD_LENGTH;
                memcpy(&remoteBgpIdentifier,
                    tempPktPtr, sizeof(NodeAddress));

                BgpCheckForCollision(
                    node,
                    bgp,
                    remoteBgpIdentifier,
                    connPtr,
                    &isCollision,
                    holdTime,
                    asId);

                if (!isCollision)
                {
                    if (asId == bgp->asId)
                    {
                        connPtr->isInternalConnection = TRUE;
                    }
                    else
                    {
                        connPtr->isInternalConnection = FALSE;
                    }

                    connPtr->remoteBgpIdentifier = remoteBgpIdentifier;

                    connPtr->asIdOfPeer = asId;

                    if (connPtr->holdTime / SECOND > holdTime)
                    {
                        connPtr->holdTime = (clocktype)
                             holdTime * SECOND;
                    }

                    BgpSendKeepAlivePackets(node, bgp, connPtr);

                    // If the negotiated value for the hold time is zero then
                    // we don't need to schedule keepalive timer and we will
                    // not set hold timer for this connection and we will
                    // take the connection will be there until simulation.

                    if (connPtr->holdTime)
                    {
                        // Set KeepAlive timer.
                        // After this keep alive timer expires we
                        // should again send another keep alive message
                        clocktype delay = (clocktype)
                            ((0.75 * bgp->keepAliveInterval)
                            + RANDOM_erand (bgp->timerSeed)
                            * (0.25 * bgp->keepAliveInterval));

                        newMsg = MESSAGE_Alloc(
                                     node,
                                     APP_LAYER,
                                     APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                                     MSG_APP_BGP_KeepAliveTimer);

                        MESSAGE_InfoAlloc(node, newMsg,
                            sizeof(ConnectionInfo));

                        ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->
                            uniqueId = connPtr->uniqueId;

                        ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->
                            sequence = connPtr->keepAliveTimerSeq;

                        MESSAGE_Send(node, newMsg, delay);
                    }
                    else
                    {
                        // Cancel if any keepalive timer or hold timer if
                        // previously raised
                        connPtr->holdTimerSeq++;
                        connPtr->keepAliveTimerSeq++;
                    }

                    // Set the state to open confirm.
                    connPtr->state = BGP_OPEN_CONFIRM;
                    if (DEBUG)
                    {
                        BgpPrintConnInformationBase(node, bgp);
                    }
                }
            }
            break;
        }
        case BGP_HOLD_TIMER_EXPIRED:
        {
            // Sending Notification and closing the transport connection
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_SENT"
                    " event: BGP_HOLD_TIMER_EXPIRED"
                    " activity: CLOSE TCP CONNECTION"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connPtr->remoteId);
            }

            BgpSendNotification(
                node,
                bgp,
                connPtr,
                BGP_ERR_CODE_HOLD_TIMER_EXPIRED_ERR,
                BGP_ERR_SUB_CODE_DEFAULT);

            APP_TcpCloseConnection(node, connPtr->connectionId);
            BgpCloseConnection(node, bgp, connPtr);
            break;
        }
        case BGP_TRANSPORT_CONNECTION_CLOSED:
        {
            Message* newMsg = NULL;
            clocktype delay = 0;

            // Close the bgp connection, Restart Connect Retry Timer and
            // go back to active state.
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_SENT"
                    " event: BGP_TRANSPORT_CONNECTION_CLOSED"
                    " activity: RESET CONNECTION RETRY TIMER"
                    " next state: BGP_ACTIVE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connPtr->remoteId);
            }

            connPtr->localPort = (unsigned short) BGP_INVALID_ID;

            // Cancel all the timers
            connPtr->connectRetrySeq++;
            connPtr->keepAliveTimerSeq++;
            connPtr->holdTimerSeq++;

            connPtr->remoteBgpIdentifier = 0;

            newMsg = MESSAGE_Alloc(
                node,
                APP_LAYER,
                APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                MSG_APP_BGP_ConnectRetryTimer);

            MESSAGE_InfoAlloc(node, newMsg,
                sizeof(ConnectionInfo));

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                connPtr->uniqueId;

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                connPtr->connectRetrySeq;

            delay = (clocktype)((0.75 * bgp->connectRetryInterval)
                        + RANDOM_erand (bgp->timerSeed)
                        * (0.25 * bgp->connectRetryInterval));

            MESSAGE_Send(node, newMsg, delay);

            connPtr->state = BGP_ACTIVE;
            if (DEBUG_COLLISION)
            {
                BgpPrintConnInformationBase(node, bgp);
            }
            break;
        }
        case BGP_RECEIVE_NOTIFICATION_MESSAGE:
        case BGP_TRANSPORT_CONNECTION_OPEN:
        case BGP_TRANSPORT_CONNECTION_OPEN_FAILED:
        case BGP_CONNECTRETRY_TIMER_EXPIRED:
        case BGP_KEEPALIVE_TIMER_EXPIRED:
        case BGP_RECEIVE_KEEPALIVE_MESSAGE:
        case BGP_RECEIVE_UPDATE_MESSAGE:
        {
            // Sending Notification and closing the transport connection
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_OPEN_SENT"
                    " event: AN_UNDESIRABLE_EVENT"
                    " activity: DISCONNECT_AND_SEND_FSM_ERR NOTIFICATION"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connPtr->remoteId);
            }

            BgpSendNotification(node,
                bgp,
                connPtr,
                BGP_ERR_CODE_FINITE_STATE_MACHINE_ERR,
                BGP_ERR_SUB_CODE_DEFAULT);

            APP_TcpCloseConnection(node, connPtr->connectionId);
            BgpCloseConnection(node, bgp, connPtr);

            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Unknown event\n");
        }
    }
}


//--------------------------------------------------------------------------
// NAME:        BgpProcessActiveState.
//
// PURPOSE:     Handling all event types at active state
//
// PARAMETERS:  node, node which is receiving any message
//              bgp, the bgp internal structure
//              data,  bgp Packet pointer
//              event, the event type of the message
//              uniqueId, bgp maintains one id for each neighbor this is for
//                        which connection the event type is meant for
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//--------------------------------------------------------------------------

static
void BgpProcessActiveState(
    Node* node,
    BgpData* bgp,
    char* data,
    BgpEvent event,
    BgpConnectionInformationBase* connectionStatus)
{
    switch (event)
    {
        case BGP_START:
        {
            // Don't do anything. but this eventtype should not occure here!!
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_ACTIVE"
                    " event: BGP_START"
                    " activity: DO NOTHING"
                    " next state: BGP_ACTIVE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }
            break;
        }
        case BGP_TRANSPORT_CONNECTION_OPEN:
        {
            // Transport connection opened so go into the open sent state
            // after sending open message
            TransportToAppOpenResult* openResult =
                (TransportToAppOpenResult*) data;

            Message* newMsg = NULL;

            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_ACTIVE"
                    " event: BGP_TRANSPORT_CONNECTION_OPEN"
                    " activity: RESET HOLD TIMER"
                    " next state: BGP_OPEN_SENT"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            connectionStatus->localPort = openResult->localPort;
            connectionStatus->connectionId = openResult->connectionId;

            // Clear the connect retry timer
            connectionStatus->connectRetrySeq++;

            // Setting the hold timer a large value
            connectionStatus->holdTime = (clocktype) bgp->largeHoldInterval;

            newMsg = MESSAGE_Alloc(
                         node,
                         APP_LAYER,
                         APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                         MSG_APP_BGP_HoldTimer);

            MESSAGE_InfoAlloc(node, newMsg,
                sizeof(ConnectionInfo));

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                connectionStatus->uniqueId;

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                connectionStatus->holdTimerSeq;

            MESSAGE_Send(node, newMsg, connectionStatus->holdTime);

            // As the connection has been established send
            // open packet and goto opensent state to receive keepalive
            BgpSendOpenPacket(node, bgp, connectionStatus->uniqueId);

            connectionStatus->state = BGP_OPEN_SENT;
            if (DEBUG_COLLISION)
            {
                BgpPrintConnInformationBase(node, bgp);
            }
            break;
        }
        case BGP_TRANSPORT_CONNECTION_OPEN_FAILED:
        {
            // Check closing transport connection is not possible at this
            // stage as we don't know the connection id as the connection
            // is not made

            // Restart connect retry timer
            Message* newMsg = NULL;
            clocktype delay = 0;


            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_ACTIVE"
                    " event: BGP_TRANSPORT_CONNECTION_OPEN_FAILED"
                    " activity: RESET CONNECTION RETRY TIMER"
                    " next state: BGP_ACTIVE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            // Cancel the previous timer and restart a new one
            connectionStatus->connectRetrySeq++;

            newMsg = MESSAGE_Alloc(
                         node,
                         APP_LAYER,
                         APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                         MSG_APP_BGP_ConnectRetryTimer);

            MESSAGE_InfoAlloc(node, newMsg,
                sizeof(ConnectionInfo));

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
               connectionStatus->uniqueId;

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
               connectionStatus->connectRetrySeq;

            delay = (clocktype)((0.75 * bgp->connectRetryInterval)
                        + RANDOM_erand (bgp->timerSeed)
                        * (0.25 * bgp->connectRetryInterval));

            MESSAGE_Send(node, newMsg, delay);

            break;
        }
        case BGP_CONNECTRETRY_TIMER_EXPIRED:
        {
            // Restart ConnectRetry timer.
            // Initiate a transport connection.
            Message* newMsg = NULL;
            clocktype delay = 0;

            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_ACTIVE"
                    " event: BGP_CONNECTRETRY_TIMER_EXPIRED"
                    " activity: RESET CONNECTION RETRY TIMER"
                    " next state: BGP_CONNECT"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            newMsg = MESSAGE_Alloc(
                         node,
                         APP_LAYER,
                         APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                         MSG_APP_BGP_ConnectRetryTimer);

            MESSAGE_InfoAlloc(node, newMsg,
                 sizeof(ConnectionInfo));

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                connectionStatus->uniqueId;

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                connectionStatus->connectRetrySeq;

            delay = (clocktype)((0.75 * bgp->connectRetryInterval)
                        + RANDOM_erand (bgp->timerSeed)
                        * (0.25 * bgp->connectRetryInterval));

            MESSAGE_Send(node, newMsg, delay);

            connectionStatus->state = BGP_CONNECT;
            if (DEBUG_COLLISION)
            {
                BgpPrintConnInformationBase(node, bgp);
            }
            break;
        }
        case BGP_TRANSPORT_CONNECTION_CLOSED:
        case BGP_RECEIVE_NOTIFICATION_MESSAGE:
        case BGP_HOLD_TIMER_EXPIRED:
        case BGP_KEEPALIVE_TIMER_EXPIRED:
        case BGP_RECEIVE_OPEN_MESSAGE:
        case BGP_RECEIVE_KEEPALIVE_MESSAGE:
        case BGP_RECEIVE_UPDATE_MESSAGE:
        {
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_ACTIVE"
                    " event: AN_UNDESIRABLE_EVENT"
                    " activity: DISCONNECT"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            APP_TcpCloseConnection(node, connectionStatus->connectionId);
            BgpCloseConnection(node, bgp, connectionStatus);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Unknown Event\n");
        }
    }
}


//--------------------------------------------------------------------------
// NAME:        BgpProcessConnectState.
//
// PURPOSE:     Handling all event types at connect state
//
// PARAMETERS:  node, node which is receiving any message
//              bgp, the bgp internal structure
//              data,  bgp Packet pointer
//              event, the event type of the message
//              uniqueId, bgp maintains one id for each neighbor this is for
//              which connection the event type is meant for
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//--------------------------------------------------------------------------

static
void BgpProcessConnectState(
    Node* node,
    BgpData* bgp,
    char* data,
    BgpEvent event,
    BgpConnectionInformationBase* connectionStatus)
{
    switch (event)
    {
        case BGP_START:
        {
            // Do nothing but this event type should not come here!!
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_CONNECT"
                    " event: BGP_START"
                    " activity: DO NOTHING"
                    " next state: BGP_CONNECT"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }
            break;
        }
        case BGP_TRANSPORT_CONNECTION_OPEN:
        {
            Message* newMsg = NULL;

            // Transport connection opened so send open message and change
            // state to open sent to receive others open message

            TransportToAppOpenResult* openResult =
                (TransportToAppOpenResult*) data;

            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_CONNECT"
                    " event: BGP_TRANSPORT_CONNECTION_OPEN"
                    " activity: SEND OPEN PACKET"
                    " next state: BGP_OPEN_SENT"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            // Set connection id for this connection
            connectionStatus->connectionId = openResult->connectionId;
            connectionStatus->localPort    = openResult->localPort;

            // Clear connect retry timer. Incrementing the value of the
            // sequence ultimately do that as it will be checked with the
            // value of the sequence number that has been assigned in the
            // timer info
            connectionStatus->connectRetrySeq++;

            newMsg = MESSAGE_Alloc(
                         node,
                         APP_LAYER,
                         APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                         MSG_APP_BGP_HoldTimer);

            MESSAGE_InfoAlloc(node, newMsg, sizeof(ConnectionInfo));

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                connectionStatus->uniqueId;

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                connectionStatus->holdTimerSeq;


            MESSAGE_Send(node, newMsg, (clocktype) bgp->largeHoldInterval);

            // The transport connection has been opened successfully
            // so send the open packet and goto open sent state where
            // it will receive open result
            BgpSendOpenPacket(node, bgp, connectionStatus->uniqueId);

            connectionStatus->state = BGP_OPEN_SENT;
            if (DEBUG)
            {
                BgpPrintConnInformationBase(node, bgp);
            }

            break;
        }
        case BGP_TRANSPORT_CONNECTION_OPEN_FAILED:
        {
            // Restart Connect retry timer and go to active state.
            Message* newMsg = NULL;
            clocktype delay = 0;

            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_CONNECT"
                    " event: BGP_TRANSPORT_CONNECTION_OPEN_FAILED"
                    " activity: RESET CONNECTION RETRY TIMER"
                    " next state: BGP_ACTIVE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            // clear the previous timer
            connectionStatus->connectRetrySeq++;

            newMsg = MESSAGE_Alloc(node, APP_LAYER,
                   APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                   MSG_APP_BGP_ConnectRetryTimer);

            MESSAGE_InfoAlloc(node, newMsg, sizeof(ConnectionInfo));

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                connectionStatus->uniqueId;

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                connectionStatus->connectRetrySeq;

            delay = (clocktype)((0.75 * bgp->connectRetryInterval)
                        + RANDOM_erand (bgp->timerSeed)
                        * (0.25 * bgp->connectRetryInterval));

            MESSAGE_Send(node, newMsg, delay);

            connectionStatus->state = BGP_ACTIVE;
            if (DEBUG_COLLISION)
            {
                BgpPrintConnInformationBase(node, bgp);
            }

            break;
        }
        case BGP_CONNECTRETRY_TIMER_EXPIRED:
        {
            // Connection is not opened within time so again try to open
            // the connection and send connection retry timer and change
            // state to active
            Message* newMsg = NULL;
            clocktype delay = 0;

            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_CONNECT"
                    " event: BGP_CONNECTRETRY_TIMER_EXPIRED"
                    " activity: OPEN TRANSPORT CONNECTION"
                    " next state: BGP_CONNECT"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            newMsg = MESSAGE_Alloc(node,
                         APP_LAYER,
                         APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                         MSG_APP_BGP_ConnectRetryTimer);

            MESSAGE_InfoAlloc(node, newMsg, sizeof(ConnectionInfo));

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                connectionStatus->uniqueId;

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                connectionStatus->connectRetrySeq;

            delay = (clocktype)((0.75 * bgp->connectRetryInterval)
                        + RANDOM_erand (bgp->timerSeed)
                        * (0.25 * bgp->connectRetryInterval));

            MESSAGE_Send(node, newMsg, delay);

            // Ask tcp to open connection for the peer

            APP_TcpOpenConnection(
                node,
                APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                connectionStatus->localAddr,
                node->appData.nextPortNum++,
                connectionStatus->remoteAddr,
                (short)APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                connectionStatus->uniqueId,
                PROCESS_IMMEDIATELY,
                IPTOS_PREC_INTERNETCONTROL);

            break;
        }
        case BGP_TRANSPORT_CONNECTION_CLOSED:
        case BGP_RECEIVE_NOTIFICATION_MESSAGE:
        case BGP_HOLD_TIMER_EXPIRED:
        case BGP_KEEPALIVE_TIMER_EXPIRED:
        case BGP_RECEIVE_OPEN_MESSAGE:
        case BGP_RECEIVE_KEEPALIVE_MESSAGE:
        case BGP_RECEIVE_UPDATE_MESSAGE:
        {
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_CONNECT"
                    " event: AN_UNDESIRABLE_EVENT"
                    " activity: DISCONNECT"
                    " next state: BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionStatus->remoteId);
            }

            APP_TcpCloseConnection(node, connectionStatus->connectionId);
            BgpCloseConnection(node, bgp, connectionStatus);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Unknown event\n");
        }
    }
}


//--------------------------------------------------------------------------
// NAME:        BgpProcessIdleState.
//
// PURPOSE:     Handling all event types at idle state
//
// PARAMETERS:  node, node which is receiving any message
//              bgp, the bgp internal structure
//              data,  bgp Packet pointer
//              event, the event type of the message
//              uniqueId, bgp maintains one id for each neighbor this is for
//              which connection the event type is meant for
//
// RETURN:      void.
//
// ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void BgpProcessIdleState(
    Node* node,
    BgpData* bgp,
    BgpEvent event,
    BgpConnectionInformationBase* connectionPtr)
{
    GenericAddress  nextHop;
    int interfaceId = 0;

    switch (event)
    {
        // Here the bgp speaker will try to open a tcp connection with the
        // bgp peer
        case BGP_START:
        {
            if (!connectionPtr->isInitiatingNode)
            {
                if (DEBUG_FSM)
                {
                    char time[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), time);

                    printf("node: %8u"
                        " state: BGP_IDLE"
                        " event: BGP_START"
                        " activity: OPEN_TRANS_PORT_CONN"
                        " next state: BGP_CONNECT"
                        " time: %s"
                        " remote-node: %u\n",
                        node->nodeId, time, connectionPtr->remoteId);
                    printf("Breaking as Not an initiating node\n");
                }

                break;
            }

            Message* newMsg = NULL;

            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_IDLE"
                    " event: BGP_START"
                    " activity: OPEN_TRANS_PORT_CONN"
                    " next state: BGP_CONNECT"
                    " time: %s"
                    " remote-node: %u\n",
                    node->nodeId, time, connectionPtr->remoteId);
            }

            // Check whether the neighbor is reachable or not. If not wait
            // for some time so that it becomes reachable...
            // NOTE: Not specified in RFC
            if (connectionPtr->remoteAddr.networkType == NETWORK_IPV4)
            NetworkGetInterfaceAndNextHopFromForwardingTable(node,
                                     GetIPv4Address(connectionPtr->remoteAddr),
                                     &interfaceId,
                                     &nextHop.ipv4);

            else if (connectionPtr->remoteAddr.networkType == NETWORK_IPV6)
              Ipv6GetInterfaceAndNextHopFromForwardingTable(node,
                                    GetIPv6Address(connectionPtr->remoteAddr),
                                    &interfaceId,
                                    &nextHop.ipv6);


            in6_addr netUnreachAddr;
            memset(&netUnreachAddr,0,sizeof(in6_addr));//bgpInvalidAddr = 0;
            if (interfaceId == NETWORK_UNREACHABLE  )
            {
                // try to connect after some time as there is no route to the
                // peer available now

                Message* newMessage = NULL;

                int uniqueId = connectionPtr->uniqueId;

                if (DEBUG)
                {
                    printf("\tPeer unreachable so waiting\n");
                }

                newMessage = MESSAGE_Alloc(node, APP_LAYER,
                                 APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                                 MSG_APP_BGP_StartTimer);

                MESSAGE_InfoAlloc(node, newMessage, sizeof(int));

                memcpy(MESSAGE_ReturnInfo(newMessage), &uniqueId,
                    sizeof(int));

                MESSAGE_Send(node, newMessage, bgp->waitingTimeForRoute);
                return;
            }

            if (connectionPtr->remoteAddr.networkType == NETWORK_IPV4)
            {
                SetIPv4AddressInfo(&connectionPtr->localAddr,
                       NetworkIpGetInterfaceAddress(node, interfaceId));
            }
            else if (connectionPtr->remoteAddr.networkType == NETWORK_IPV6)
            {

                in6_addr local6 = GetIPv6Address(connectionPtr->localAddr);
                Ipv6GetGlobalAggrAddress(node,
                             interfaceId,
                             &local6);
                SetIPv6AddressInfo(&connectionPtr->localAddr,local6);
            }


            // Send one self event to set maximum time it will wait for tcp
            // to open the connection. Within this time if tcp fails the
            // speaker will again try to open the transport connection
            newMsg = MESSAGE_Alloc(node,
                                   APP_LAYER,
                                   APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                                   MSG_APP_BGP_ConnectRetryTimer);

            MESSAGE_InfoAlloc(node, newMsg, sizeof(ConnectionInfo));

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
                connectionPtr->uniqueId;

            ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
                connectionPtr->connectRetrySeq;

            clocktype delay = (clocktype)((0.75 * bgp->connectRetryInterval)
                        + RANDOM_erand (bgp->timerSeed)
                        * (0.25 * bgp->connectRetryInterval));

            MESSAGE_Send(node, newMsg, delay);

            if (DEBUG)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);
                printf("\tAt %s opening transport connection\n",
                       time);
            }

            connectionPtr->isInitiatingNode = TRUE;
           // Ask tcp to open connection for the peer

            APP_TcpOpenConnection(
                node,
                APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                connectionPtr->localAddr,//IPv4 or IPv6(only prefix?)
                node->appData.nextPortNum++,
                connectionPtr->remoteAddr, //IPv4 or IPv6
                (short) APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                connectionPtr->uniqueId,
                PROCESS_IMMEDIATELY,
                IPTOS_PREC_INTERNETCONTROL);

            // Switch to the connect state. bgp is supposed to receive the
            // connection open result in the connect state

            connectionPtr->state = BGP_CONNECT;
            if (DEBUG_COLLISION)
            {
                BgpPrintConnInformationBase(node, bgp);
            }
            break;
        }
        default:
        {
            // Ignore all the other event types except start event
            if (DEBUG_FSM)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time);

                printf("node: %8u"
                    " state: BGP_IDLE"
                    " event: AN UNDESIRABLE EVENT"
                    " activity: DO_NOTHING"
                    " next state : BGP_IDLE"
                    " time: %s remote-node: %u\n",
                    node->nodeId, time, connectionPtr->remoteId);
            }

            return;
        }
    }
}


//--------------------------------------------------------------------------
// NAME:        BgpProcessEventOrPacket.
//
// PURPOSE:     Handling all event types in Bgp and calls the corresponding
//                     state handling fuction checking the current state
//
// PARAMETERS:  node, node which is receiving any message
//              data,  the data it has received
//              event, the event type of the message
//              connectionPtr, connection pointer which has received data
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//--------------------------------------------------------------------------

static
void BgpProcessEventOrPacket(
   Node* node,
   char* data,
   BgpEvent event,
   BgpConnectionInformationBase* connectionPtr)
{
    BgpData* bgp = (BgpData*) node->appData.exteriorGatewayVar;
    if ((connectionPtr == NULL) || (bgp == NULL))
    {
        return;
    }
    BgpStateType state = connectionPtr->state;


    switch (state)
    {
        // The current state is idle
        case BGP_IDLE:
        {
            if (DEBUG)
            {
                printf("Node %u Remote Node %u :state is IDLE\n",
               node->nodeId, connectionPtr->remoteId);
            }
            BgpProcessIdleState(node, bgp, event, connectionPtr);
            break;
        }
        // The current state is connect
        case BGP_CONNECT:
        {
            if (DEBUG)
            {
                printf("Node %u Remote Node %u :state is CONNECT\n",
               node->nodeId, connectionPtr->remoteId);
            }

            BgpProcessConnectState(node, bgp, data, event, connectionPtr);
            break;
        }
        // The current state is active
        case BGP_ACTIVE:
        {
            if (DEBUG)
            {
                printf("Node %u Remote Node %u :state is ACTIVE\n",
               node->nodeId, connectionPtr->remoteId);
            }

            BgpProcessActiveState(node, bgp, data, event, connectionPtr);
            break;
        }
        // The current state is open sent
        case BGP_OPEN_SENT:
        {
            if (DEBUG)
            {
                printf("Node %u Remote Node %u :state is OPEN SENT\n",
               node->nodeId, connectionPtr->remoteId);
            }

            BgpProcessOpenSentState(node,
                                    bgp,
                                    data,
                                    event,
                                    connectionPtr);
            break;
        }
        // The current state is open confirm
        case BGP_OPEN_CONFIRM:
        {
            if (DEBUG)
            {
                printf("Node %u Remote Node %u :state is OPEN CONFIRM\n",
               node->nodeId, connectionPtr->remoteId);
            }

            BgpProcessOpenConfirmState(node, bgp, data, event,
                       connectionPtr);

            break;
        }
        // The current state is establish
        case BGP_ESTABLISHED:
        {
            if (DEBUG)
            {
                printf("Node %u Remote Node %u :state is ESTABLISHED\n",
               node->nodeId, connectionPtr->remoteId);
            }

            BgpProcessEstablishedState(node,
                       bgp,
                       data,
                       event,
                       connectionPtr);

            break;
        }
        // The state type is not a valid one, so output error
        default:
        {
            ERROR_Assert(FALSE, "Unknown state of the connection!\n");
        }
    }
}


//--------------------------------------------------------------------------
// NAME:        BgpProcessPassiveOpen
//
// PURPOSE:     When a passive open for a connection from a bgp speaker comes
//              this node adds the connection id with the bgp speaker
//
//
// PARAMETERS:  node, the node which is printing the information
//              data,  the open message from transport which came in
//
// RETURN:      None
//
// ASSUMPTION:  None
//
//--------------------------------------------------------------------------

static
void BgpProcessPassiveOpen(Node* node, char* data)
{
    TransportToAppOpenResult* openResult = (TransportToAppOpenResult*) data;
    BgpData* bgp = (BgpData*) node->appData.exteriorGatewayVar;
    unsigned short peerAsId = (unsigned short)BGP_INVALID_ID;

    int i,connectionIdToDel ;
    BOOL isInternalConnection = FALSE;
    unsigned short weight = (unsigned short) BGP_DEFAULT_EXTERNAL_WEIGHT;
    int numEntries = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
    / sizeof(BgpConnectionInformationBase);

    BgpConnectionInformationBase* bgpConnInfo = (BgpConnectionInformationBase*)
    BUFFER_GetData(&(bgp->connInfoBase));

    for (i = 0; i < numEntries; i++)
    {
        if (Address_IsSameAddress(&bgpConnInfo[i].remoteAddr,
                      &openResult->remoteAddr))
        {
            weight =  bgpConnInfo[i].weight;
            connectionIdToDel = bgpConnInfo[i].connectionId;
            isInternalConnection = bgpConnInfo[i].isInternalConnection;
            peerAsId = bgpConnInfo[i].asIdOfPeer;
            break;
        }
    }

    // All the connection will be assigned an unique id which is the
    // process id of the connection
    int uniqueId = node->appData.uniqueId++;

    // extract node Id of the peer

    //this function is overloaded for IPv6
    NodeAddress peerId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                   openResult->remoteAddr);

    Message* newMsg = NULL;

    BgpConnectionInformationBase connect, *connInfoPtr;
    //Morteza:this function must be changed,remote address and
    //local address must be IPv6

    BOOL isClient = FALSE, found = FALSE;

    for (int i = 0; i < numEntries; i++)
    {
        if ((Address_IsSameAddress(&bgpConnInfo[i].remoteAddr,
                  &openResult->remoteAddr)) &&
                  ((bgpConnInfo[i].state == BGP_IDLE) ||
                  (bgpConnInfo[i].state == BGP_CONNECT) ||
                  (bgpConnInfo[i].state == BGP_ACTIVE)))
        {
            if (DEBUG)
            {
               printf("Already has a CIB with this peer in idle state\n");
            }
            isClient = bgpConnInfo[i].isClient; //Indicates that this is a client peer
            found = TRUE;
            break;
        }
    }
    if (found)
    {
        if (DEBUG)
        {
            printf("Node %d updating conn entry\n",node->nodeId);
        }
        BgpUpdatePeerEntry( node,
            &bgpConnInfo[i],
            openResult->remoteAddr, //GetIPv4Address(openResult->remoteAddr),
            openResult->localAddr,  //GetIPv4Address(openResult->localAddr),
            openResult->localPort,
            openResult->remotePort,
            bgp->routerId,
            (NodeAddress) BGP_INVALID_ADDRESS,
            peerAsId,
            weight,
            isInternalConnection,
            isClient,
            bgp->holdInterval,
            uniqueId,
            BGP_OPEN_SENT,
            openResult->connectionId,
            FALSE);
        memcpy(&connect,&bgpConnInfo[i],sizeof(BgpConnectionInformationBase));
    }
    else
    {
        if (DEBUG)
        {
            printf("Node %d creating conn entry\n",node->nodeId);
        }
        BgpFillPeerEntry( node,
            &connect,
            openResult->remoteAddr,
            openResult->localAddr,
            openResult->localPort,
            openResult->remotePort,
            bgp->routerId,
            (NodeAddress) BGP_INVALID_ADDRESS,
            (unsigned short) BGP_INVALID_ADDRESS,
            weight,
            (unsigned int) BGP_DEFAULT_MULTI_EXIT_DISC_VALUE,
            isInternalConnection,
            isClient,
            bgp->holdInterval,
            uniqueId,
            BGP_OPEN_SENT,
            openResult->connectionId,
            FALSE);

        BUFFER_AddDataToDataBuffer(&(bgp->connInfoBase),
                               (char*) &connect,
                               sizeof(BgpConnectionInformationBase));
    }
    connInfoPtr = BgpGetConnInfoFromUniqueId(bgp, uniqueId);

    if (DEBUG)
    {
        char time[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), time);

        printf("Node %u Remote Node %u :PassiveOpen\n"
               "\tAt %s got passive open\n",
               node->nodeId,
               peerId,
               time);
    }

    newMsg = MESSAGE_Alloc(
                  node,
                  APP_LAYER,
                  APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                  MSG_APP_BGP_HoldTimer);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(ConnectionInfo));

    ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->uniqueId =
            connect.uniqueId;

    ((ConnectionInfo*) MESSAGE_ReturnInfo(newMsg))->sequence =
            connect.holdTimerSeq;


    MESSAGE_Send(node, newMsg, (clocktype) bgp->largeHoldInterval);

    // The transport connection has been opened successfully
    // so send the open packet and goto opensent state where
    // it will receive open result
    if (DEBUG_COLLISION)
    {
        printf("Passive connection Open\n");
        BgpPrintConnInformationBase(node, bgp);
    }
    BgpSendOpenPacket(node, bgp, connect.uniqueId);

}


//--------------------------------------------------------------------------
// FUNCTION BgpProcessPacket
//
// PURPOSE  at this point bgp has received one complete bgp packet from
//          transport layer so now process the packet, depending upon the
//          type of bgp packet and call the relevent function.
//
// PARAMETERS
//      node, the node which is receiving data
//      connection, the connection which has received the data
//      packet, the msg containing the complete bgp packet.
//
// RETURN
//      void
//--------------------------------------------------------------------------

static
void BgpProcessPacket(
    Node* node,
    BgpConnectionInformationBase* connection,
    char* packet)
{

    BgpSwapBytes((unsigned char*)packet, TRUE);

    char type = *(packet + BGP_SIZE_OF_MARKER + BGP_MESSAGE_LENGTH_SIZE);
    BgpData* bgp = (BgpData*) node->appData.exteriorGatewayVar;

    if (BgpCheckIfMessageHeaderError(node,bgp,packet,connection))
     {
         return;
     }

    switch (type)
    {
        case BGP_OPEN:
        {
            // Open msg came Increment Statistical variable
            bgp->stats.openMsgReceived++;

            BgpProcessEventOrPacket(
                node,
                packet,
                BGP_RECEIVE_OPEN_MESSAGE,
                connection);
            break;
        }
        case BGP_UPDATE:
        {
            // Update msg came Increment the statistical variable
            bgp->stats.updateReceived++;

            BgpProcessEventOrPacket(
                node,
                packet,
                BGP_RECEIVE_UPDATE_MESSAGE,
                connection);
            break;
        }
        case BGP_NOTIFICATION:
        {
            // Update msg Increment the statistical variable
            bgp->stats.notificationReceived++;

            BgpProcessEventOrPacket(
                node,
                packet,
                BGP_RECEIVE_NOTIFICATION_MESSAGE,
                connection);
            break;
        }
        case BGP_KEEP_ALIVE:
        {
            // Keep alive message came Increment the statistical variable
            bgp->stats.keepAliveReceived++;

            BgpProcessEventOrPacket(
                node,
                packet,
                BGP_RECEIVE_KEEPALIVE_MESSAGE,
                connection);
            break;
        }
        default:
        {
            BgpSendNotification(node,
               bgp,
               connection,
               BGP_ERR_CODE_MSG_HDR_ERR ,
               BGP_ERR_SUB_CODE_MSG_HDR_BAD_MSG_TYPE);
               APP_TcpCloseConnection(node,
                                      connection->connectionId);
            BgpCloseConnection(node, bgp, connection);
            break;
        }
    }
    MEM_free(packet);
}


//--------------------------------------------------------------------------
// FUNCTION BgpProcessRawData
//
// PURPOSE  handling raw data bytes received from the transport layer and
//          reassembling them or dividing them to make one complete bgp
//          packet and then sending that to the relevent function to
//          process that
//
// PARAMETERS
//      node, the node which is receiving data
//      bgp, the internal bgp structure
//      connection, the connection which has received the data
//      data, the raw trasport data bytes
//      dataSize, the size of data received
//
// RETURN
//      void
//--------------------------------------------------------------------------
static
void BgpProcessRawData(
   Node* node,
   BgpData* bgp,
   BgpConnectionInformationBase* connection,
   char* data,
   int dataSize)
{
    unsigned short  expectedLengthOfPacket;
    BgpMessageBuffer* buffer = &(connection->buffer);


    if (buffer->currentSize == 0)
    {
        // buffer size 0 means its a fresh data stream, and it contains
        // multiple packets
        if (dataSize >= BGP_MIN_HDR_LEN)
        {
            // data size is greater than the header size so I can
            // look at the bgp packet length
            memcpy(&expectedLengthOfPacket, data + BGP_SIZE_OF_MARKER,
           BGP_MESSAGE_LENGTH_SIZE);
           EXTERNAL_ntoh(
                    &expectedLengthOfPacket,
                    BGP_MESSAGE_LENGTH_SIZE);
            if (expectedLengthOfPacket <= dataSize)
            {
                // Got one complete packet. send it to the corresponding
                // bgp function
                char* newData = NULL;
                char* packet = NULL;
                int   remainingDataSize = 0;

                packet = (char*) MEM_malloc(expectedLengthOfPacket);
                memcpy(packet, data, expectedLengthOfPacket);

                // Call the process packet function to process the complete
                // packet received
                BgpProcessPacket(node, connection, packet);
                remainingDataSize = dataSize - expectedLengthOfPacket;

                if (remainingDataSize)
                {
                    newData = (char *) MEM_malloc(remainingDataSize);
                    memcpy(newData,
                           data + expectedLengthOfPacket,
                           remainingDataSize);
                    MEM_free(data);

                    // Process the remainin size of data.
                    BgpProcessRawData(node,
                                      bgp,
                                      connection,
                                      newData,
                                      remainingDataSize);
                }
                else
                {
                    MEM_free(data);
                }
            }
            else
            {
                // This is not one complete bgp packet so store it in the
                // buffer
                buffer->data
                    = (unsigned char*) MEM_malloc(expectedLengthOfPacket);
                memcpy(buffer->data, data, dataSize);
                MEM_free(data);
                buffer->expectedSize = expectedLengthOfPacket;
                buffer->currentSize  = dataSize;
            }
        }
        else
        {
            // Data is too small to learn the expected size so just buffer it
            buffer->data = (unsigned char*) MEM_malloc(dataSize);
            memcpy(buffer->data, data, dataSize);
            MEM_free(data);
            buffer->expectedSize = 0;
            buffer->currentSize  = dataSize;
        }
    }
    else
    {
        // There is data in the buffer
        if (buffer->currentSize && buffer->expectedSize)
        {
            // There is data in the buffer and it is with the header
            if (buffer->expectedSize - buffer->currentSize <= dataSize)
            {
                // The new data is big enough for reassembly.
                char* newData = NULL;
                int newDataSize = 0;

                newData = (char *)MEM_malloc(buffer->currentSize + dataSize);
                memcpy(newData, buffer->data, buffer->currentSize);
                memcpy(newData + buffer->currentSize, data, dataSize);
                MEM_free(buffer->data);
                buffer->data = NULL;
                MEM_free(data);
                newDataSize = buffer->currentSize + dataSize;
                buffer->currentSize = 0;
                buffer->expectedSize = 0;

                BgpProcessRawData(node,
                                  bgp,
                                  connection,
                                  newData,
                                  newDataSize);
            }
            else
            {
                // New data is not big enough for reassemly, so keep looking.
                memcpy(buffer->data + buffer->currentSize, data, dataSize);
                buffer->currentSize += dataSize;
                MEM_free(data);
            }
        }
        else
        {
            // The data in the buffer is not with header
            char* newData = NULL;
            int newDataSize = 0;

            newData = (char *) MEM_malloc(buffer->currentSize + dataSize);
            memcpy(newData, buffer->data, buffer->currentSize);
            memcpy(newData + buffer->currentSize, data, dataSize);
            MEM_free(buffer->data);
            buffer->data = NULL;
            MEM_free(data);
            newDataSize = buffer->currentSize + dataSize;
            buffer->currentSize = 0;
            buffer->expectedSize = 0;

            BgpProcessRawData(node,
                              bgp,
                              connection,
                              newData,
                              newDataSize);
        }
    }
}
//--------------------------------------------------------------------------
// NAME:        BgpHandleTransportData.
//
// PURPOSE:     Reassembling transport raw data to form one complete bgp
//              packet
//
// PARAMETERS:  node, node which is receiving any message
//              bgp, bgp internal structure
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//--------------------------------------------------------------------------

static
void BgpHandleTransportData(
    Node* node,
    BgpData* bgp,
    Message* msg)
{
    TransportToAppDataReceived* dataRecvd = NULL;
    char* packetRecvd = NULL;
    char* data = NULL;
    int  connectionId = -1;
    int packetSize = 0;
    BgpConnectionInformationBase* connectionStatus;



    dataRecvd = (TransportToAppDataReceived*) MESSAGE_ReturnInfo(msg);

    packetRecvd = MESSAGE_ReturnPacket(msg);
    packetSize = MESSAGE_ReturnPacketSize(msg);
    connectionId = dataRecvd->connectionId;

    if (connectionId >= 0)
    {
        BgpMessageBuffer* buffer = 0;
        unsigned short  expectedLengthOfPacket = 0;

        // Check for which connection the message came and
        // update that connections information only

        connectionStatus = BgpGetConnInfoFromConnectionId(bgp,
            connectionId);

        if (connectionStatus == NULL)
        {
            MESSAGE_Free(node, msg);
            return;
        }
        else
        {
            // Swap bytes from network byte order to host byte order
            // This is a data for a valid connection so process it
            buffer = &connectionStatus->buffer;
            if (buffer->currentSize == 0)
            {
                // buffer size 0 means its a fresh data stream, so parse it
                // if it is a single packet or multiple packet
                if (packetSize >= BGP_MIN_HDR_LEN)
                {
                    // data size is greater than the header size so I can
                    // look at the bgp packet length
                    memcpy(&expectedLengthOfPacket,
                        (packetRecvd + BGP_SIZE_OF_MARKER),
                        BGP_MESSAGE_LENGTH_SIZE);
                    EXTERNAL_ntoh(
                            (void*)&expectedLengthOfPacket,
                            BGP_MESSAGE_LENGTH_SIZE);
                    if (expectedLengthOfPacket == packetSize)
                    {
                        // the data contains only one complete bgp packet
                        data = (char*) MEM_malloc(packetSize);
                        memcpy(data, packetRecvd, packetSize);
                        BgpProcessPacket(node, connectionStatus, data);
                        MESSAGE_Free(node, msg);
                        return;
                    }
                }
            }
            // At this point the data either contains multiple bgp packets
            // or it needs re-assembling
            data = (char*) MEM_malloc(packetSize);
            memcpy(data, packetRecvd, packetSize);
            BgpProcessRawData(node, bgp, connectionStatus, data,
                packetSize);

            MESSAGE_Free(node, msg);
        }
    }
    else
    {
        ERROR_Assert(FALSE, "Packet came for a connection"
            " id which does not exist\n");
    }
}


//--------------------------------------------------------------------------
// NAME:        BgpLayer
//
// PURPOSE:     Handling all event types in Bgp and do the necessary action
//              receiving a packet from the lower layer ie tcp
//              when,
//
// PARAMETERS:  node, node which is receiving any message
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//--------------------------------------------------------------------------

void
BgpLayer(Node* node, Message* msg)
{
    int eventType = MESSAGE_GetEvent(msg);

    BgpConnectionInformationBase* connectionPtr = NULL;

    BgpData* bgp = (BgpData*) node->appData.exteriorGatewayVar;

    switch (eventType)
    {
        case MSG_APP_BGP_KeepAliveTimer:
        {
            int* uniqueId = &((ConnectionInfo*) MESSAGE_ReturnInfo(msg))->
                     uniqueId;

            connectionPtr = BgpGetConnInfoFromUniqueId(bgp, *uniqueId);

            // Check if the connection still exists, the connection may be
            // deleted for connection collision or for any other reason
            if (connectionPtr != NULL)
            {
                // check if this is a valid timer or not.
                if (connectionPtr->keepAliveTimerSeq == ((ConnectionInfo*)
                    MESSAGE_ReturnInfo(msg))->sequence)
                {
                    BgpProcessEventOrPacket(node, MESSAGE_ReturnInfo(msg),
                        BGP_KEEPALIVE_TIMER_EXPIRED, connectionPtr);
                }
            }
            MESSAGE_Free(node, msg);
            break;
        }

        // Hold timer expired So if no updation came within this time
        // interval close connection
        case MSG_APP_BGP_HoldTimer:
        {
            int* uniqueId = &((ConnectionInfo*) MESSAGE_ReturnInfo(msg))->
                uniqueId;

            connectionPtr = BgpGetConnInfoFromUniqueId(bgp, *uniqueId);

            if (connectionPtr)
            {
                // check if this is a valid timer or not.
                if (connectionPtr->holdTimerSeq == ((ConnectionInfo*)
                    MESSAGE_ReturnInfo(msg))->sequence)
                {
                    BgpProcessEventOrPacket(node, MESSAGE_ReturnInfo(msg),
                        BGP_HOLD_TIMER_EXPIRED, connectionPtr);
                }
            }
            MESSAGE_Free(node, msg);
            break;
        }

        // Connect retry timer expired, If tcp connection is made within
        // this time again try to open connection
        case MSG_APP_BGP_ConnectRetryTimer:
        {
            int* uniqueId = &((ConnectionInfo*) MESSAGE_ReturnInfo(msg))->
                uniqueId;

            connectionPtr = BgpGetConnInfoFromUniqueId(bgp, *uniqueId);

            // For checking if the connection still exists (the connection
            // may get deleted for connection collision or for any other
            // reason)
            if (connectionPtr)
            {
                // If the state is not connect or active then the connection
                // has already been opened
                if (connectionPtr->connectRetrySeq == ((ConnectionInfo*)
                    MESSAGE_ReturnInfo(msg))->sequence)
                {
                    BgpProcessEventOrPacket(node, MESSAGE_ReturnInfo(msg),
                        BGP_CONNECTRETRY_TIMER_EXPIRED, connectionPtr);
                }
            }
            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_BGP_StartTimer:
        {
            int* uniqueId = (int*) MESSAGE_ReturnInfo(msg);

            connectionPtr = BgpGetConnInfoFromUniqueId(bgp, *uniqueId);
            if (connectionPtr)
            {
                BgpProcessEventOrPacket( node,
                     (char*) MESSAGE_ReturnInfo(msg),
                     BGP_START,
                     connectionPtr);
            }
            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_FromTransOpenResult:
        {
            TransportToAppOpenResult* openResult =
                (TransportToAppOpenResult*) MESSAGE_ReturnInfo(msg);

            int uniqueId = openResult->uniqueId;

            if (openResult->type == TCP_CONN_ACTIVE_OPEN)
            {
                connectionPtr = BgpGetConnInfoFromUniqueId(bgp, uniqueId);

                if ((connectionPtr) &&
                    (openResult->connectionId == BGP_INVALID_ID))
                {
                    // Transport connection open failed so send the message
                    // to the connection so that it can reinitiate the
                    // transport connection
                    BgpProcessEventOrPacket(node, MESSAGE_ReturnInfo(msg),
                        BGP_TRANSPORT_CONNECTION_OPEN_FAILED,
                        connectionPtr);

                    node->appData.numAppTcpFailure++;
                }
                else if (connectionPtr)
                {
                    // Connection established with other BGP speaker.
                    BgpProcessEventOrPacket(node,
                        MESSAGE_ReturnInfo(msg),
                        BGP_TRANSPORT_CONNECTION_OPEN,
                        connectionPtr);
                }
            }
            else if (openResult->type == TCP_CONN_PASSIVE_OPEN)
            {

                if (openResult->connectionId >= 0)
                {
                    BgpProcessPassiveOpen(node, MESSAGE_ReturnInfo(msg));
                }
                else
                {
                    ERROR_Assert(FALSE, "Passive open with invalid id\n");
                }
            }
            MESSAGE_Free(node, msg);
            break;
        }
        // Listen result to the bgp speaker which was listening
        // for a connection opened by other nodes
        case MSG_APP_FromTransListenResult:
        {
            TransportToAppListenResult* transportToAppListenResult;
            transportToAppListenResult = (TransportToAppListenResult*)
                MESSAGE_ReturnInfo(msg);

            if (transportToAppListenResult->connectionId == BGP_INVALID_ID)
            {
                // Listen fails do nothing except listening again
                node->appData.numAppTcpFailure++;

                APP_TcpServerListen(node,
                    APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                    transportToAppListenResult->localAddr,
                    APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4);
            }

            MESSAGE_Free(node, msg);
            break;
        }
        // Data just sent to TCP.
        case MSG_APP_FromTransDataSent:
        {
            // Sending left data has been taken care of at the sending data
            // part so here we have nothing to do
            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_FromTransDataReceived:
        {
            BgpHandleTransportData(node, bgp, msg);
            break;
        }
        case MSG_APP_FromTransCloseResult:
        {
            TransportToAppCloseResult* closeResult =
                (TransportToAppCloseResult*) MESSAGE_ReturnInfo(msg);

            int  connectionId = closeResult->connectionId;

            BgpConnectionInformationBase* connectionStatus = NULL;

            if (connectionId >= 0)
            {
                // Check for which connection the message came and
                // update that connections information only
                connectionStatus =
                    BgpGetConnInfoFromConnectionId(bgp, connectionId);

                if (connectionStatus == NULL)
                {
                    // The connection has already been closed so we don't
                    // need to close it again.
                    MESSAGE_Free(node, msg);
                    return;
                }
                else
                {
                    // The connection still exists in my information base.
                    // where tcp has closed the connection. So close it from
                    // my structure.
                    BgpProcessEventOrPacket(
                        node,
                        MESSAGE_ReturnInfo(msg),
                        BGP_TRANSPORT_CONNECTION_CLOSED,
                        connectionStatus);
                }
            }
            MESSAGE_Free(node, msg);
            break;
        }
        case MSG_APP_BGP_RouteAdvertisementTimer:
        {
            // Need to send another update packet to the peer after this
            // time interval
            int uniqueId = -1;
            BgpConnectionInformationBase* connPtr = NULL;

            memcpy(&uniqueId, MESSAGE_ReturnInfo(msg), sizeof(int));
            connPtr = BgpGetConnInfoFromUniqueId(bgp, uniqueId);

            if (connPtr && (connPtr->connectionId > 0))
            {
                BgpProcessEventOrPacket(node, MESSAGE_ReturnPacket(msg),
                    SEND_UPDATE, connPtr);
            }
            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_APP_PrintRoutingTable:
        {
            if (DEBUG_FAULT)
            {
                BgpPrintRoutingInformationBase(node, bgp);
                NetworkPrintForwardingTable(node);
            }
            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_APP_BGP_OriginationTimer:
        {
            MESSAGE_Send(node, msg, bgp->minASOriginationInterval);

            if (DEBUG)
              printf("Node %u MSG_APP_BGP_IgpProbeTime\n",node->nodeId);

            if (bgp->ip_version == NETWORK_IPV4 )
                BgpDeleteNonExistingIpv4IgpRoutes(node, bgp);
            else
                BgpDeleteNonExistingIpv6IgpRoutes(node, bgp);

            if (DEBUG_TABLE)
            {
                printf("Content of adjRibIn before update from "
                    "Forwarding Table\n");
                BgpPrintRoutingInformationBase(node, bgp);

                printf("Content of Forwarding Table\n:");
                if (bgp->ip_version == NETWORK_IPV4)
                    NetworkPrintForwardingTable(node);
                else
                    Ipv6PrintForwardingTable(node);

            }

            if (bgp->ip_version == NETWORK_IPV4)
                BgpUpdateRibFromIpv4ForwardingTable(node, bgp, FALSE);
            else
                BgpUpdateRibFromIpv6ForwardingTable(node, bgp, FALSE);

            if (DEBUG_TABLE)
            {
                printf("Content of adjRibIn after update from "
                    "Forwarding Table\n");
                BgpPrintRoutingInformationBase(node, bgp);
            }

            BgpDecisionProcessPhase2(node, bgp);
            BgpDecisionProcessPhase3(node, bgp);
            break;
        }

        default:
        {
#ifndef EXATA
            ERROR_Assert(FALSE, "No other options now\n");
#endif
        }
    }
}


//--------------------------------------------------------------------------
// NAME:        BgpInit.
//
// PURPOSE:     Handling all initializations needed for Bgp.
//              initializing the internal structure
//              initializing neighboring info
//              initializing the internal routing table
//              initialize statistics
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:  The bgp speakers are the border routers in the corresponding
//              As. Igp routes are statically configured through file. This
//              should be through Igp Bgp interaction.
//--------------------------------------------------------------------------

void BgpInit(Node* node, const NodeInput* nodeInput)
{
    BOOL retVal = FALSE;
    NodeInput bgpInput;

    BgpData* bgp = NULL;
    char buf[MAX_STRING_LENGTH];

    int index = 0;

    // Check whether this node is a Bgp speaker. The other initialization
    // for bgp will be done only if this is a bgp speaker. The node which
    // are not bgp speakers will not take part in the functionality of bgp
    IO_ReadCachedFile(ANY_NODEID, ANY_ADDRESS, nodeInput,
        "BGP-CONFIG-FILE", &retVal, &bgpInput);

    if (!retVal)
    {
        ERROR_ReportError("The speaker config file is not specified");
    }

    if (BgpIsSpeaker(node->nodeId, &index, &bgpInput))
    {
        int i = 0;
        int numEntries = 0;
        BgpRoutingInformationBase* adjRibIn = NULL;
        BgpConnectionInformationBase* bgpConnInfo = NULL;
        Message* rtUpdateTimer = NULL;

        if (DEBUG)
        {
            printf("Node %u is initializing as a bgp speaker\n",
                node->nodeId);
        }

        // Initialize all necessary states, variables, and so forth
        bgp = (BgpData*) MEM_malloc(sizeof(BgpData));

        node->appData.exteriorGatewayVar = (void*) bgp;

        RANDOM_SetSeed(bgp->timerSeed,
                       node->globalSeed,
                       node->nodeId,
                       APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4);


        // Check if statistics to be collected at the end of simulation
        IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput,
            "EXTERIOR-GATEWAY-PROTOCOL-STATISTICS", &retVal, buf);

        if (retVal)
        {
            if (!strcmp(buf, "YES"))
            {
                bgp->statsAreOn = TRUE;
            }
            else
            {
                bgp->statsAreOn = FALSE;
            }
        }
        else
        {
            bgp->statsAreOn = FALSE;
        }

        retVal = FALSE ;
        IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput,
            "BGP-NEXT-HOP-LEGACY-SUPPORT", &retVal, buf);

        if (retVal)
        {
            if (!strcmp(buf, "YES"))
            {
                bgp->bgpNextHopLegacySupport = TRUE;
            }
            else
            {
                bgp->bgpNextHopLegacySupport = FALSE;
            }
        }
        else
        {
            bgp->bgpNextHopLegacySupport = TRUE;
        }


        bgp->isRtReflector = FALSE;
        // Initialize the statistical variables
        BgpInitStat(bgp);

        // Initialize the Buffers
        BUFFER_InitializeDataBuffer(&bgp->adjRibIn,
                    sizeof(BgpRoutingInformationBase) *
                    BGP_MIN_RT_BLOCK_SIZE);

        BUFFER_InitializeDataBuffer(&bgp->ribLocal,
                    sizeof(BgpAdjRibLocStruct) * BGP_MIN_RT_BLOCK_SIZE);

        BUFFER_InitializeDataBuffer(&bgp->connInfoBase,
                    sizeof(BgpConnectionInformationBase) *
                    BGP_MIN_CONN_BLOCK_SIZE);

        // not used.
        BUFFER_InitializeDataBuffer(&bgp->forwardingInfoBase, 0);

        retVal = 0;
        // Initializing Bgp Timers
        bgp->startTime = BGP_DEFAULT_START_TIME;
        IO_ReadTime(node->nodeId, ANY_DEST, nodeInput,
             "BGP-START-TIME",&retVal, &bgp->startTime);
        if (retVal)
        {
            if (bgp->startTime < 0)
            {
                ERROR_ReportWarning("Invalid BGP START TIMER configuration:"
                    " Timer should set to a positive value.It was set to"
                    " default value\n");
                bgp->startTime = BGP_DEFAULT_START_TIME;
            }
        }

        // Set the hold time for this node
        retVal = 0;
        clocktype timeVal = 0;
        IO_ReadTime(node->nodeId, ANY_DEST, nodeInput,
            "BGP-HOLD-TIME-INTERVAL", &retVal, &timeVal);

        if (retVal)
        {
            bgp->holdInterval = timeVal;
            if (timeVal < 0)
            {
                ERROR_ReportWarning("Invalid BGP TIMER configuration:Timer "
                "should set to a positive value.It was set to"
                " default value\n");

                bgp->holdInterval = BGP_DEFAULT_HOLD_TIME;
            }
        }
        else
        {
            bgp->holdInterval = BGP_DEFAULT_HOLD_TIME;
        }
        if (bgp->holdInterval > 0 && bgp->holdInterval < BGP_HOLD_INTERVAL_MIN_RANGE)
        {
            ERROR_ReportWarning("Hold time MUST be 0 or atleast 3 seconds. "
                "Continuiung with the default value\n");
            bgp->holdInterval = BGP_DEFAULT_HOLD_TIME;

        }
        // check the weight field is not big to fit into 2 byte
        if (bgp->holdInterval > BGP_TIME_INTERVAL_MAX_RANGE)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "HOLD time must be less than "
                    "equal to 65,535  seconds.Continuiung with"
                    " the default value\n");
            ERROR_ReportWarning(errString);
            bgp->holdInterval = BGP_DEFAULT_HOLD_TIME;
        }
        // Set the large hold time for this node
        BgpReadTime(node,
            &bgp->largeHoldInterval,
            "BGP-LARGE-HOLD-TIME-INTERVAL",
            BGP_DEFAULT_LARGE_HOLD_TIME,
            nodeInput);
        if (bgp->largeHoldInterval > BGP_TIME_INTERVAL_MAX_RANGE)
        {
            ERROR_ReportError("Large Hold time MUST be less than 65,535 seconds"
                "Continuiung with the default value\n");
            bgp->holdInterval = BGP_DEFAULT_HOLD_TIME;
        }

        // Min route advertisement timer for this node
        BgpReadTime(node,
            &bgp->minRouteAdvertisementInterval_EBGP,
            "BGP-MIN-RT-ADVERTISEMENT-INTERVAL-EBGP",
            BGP_DEFAULT_MIN_RT_ADVERTISEMENT_TIMER_EBGP,
            nodeInput);
        if (bgp->minRouteAdvertisementInterval_EBGP > BGP_TIME_INTERVAL_MAX_RANGE)
        {
            ERROR_ReportWarning("Min Rt Adv EBGP Interval time MUST be "
             "less than 65,535 seconds.Continuing with the default value\n");
            bgp->minRouteAdvertisementInterval_EBGP =
                BGP_DEFAULT_MIN_RT_ADVERTISEMENT_TIMER_EBGP;
        }
        // Min route advertisement timer for this node
        BgpReadTime(node,
            &bgp->minRouteAdvertisementInterval_IBGP,
            "BGP-MIN-RT-ADVERTISEMENT-INTERVAL-IBGP",
            BGP_DEFAULT_MIN_RT_ADVERTISEMENT_TIMER_IBGP,
            nodeInput);
        if (bgp->minRouteAdvertisementInterval_IBGP > BGP_TIME_INTERVAL_MAX_RANGE)
        {
            ERROR_ReportWarning("Min Rt Adv IBGP Interval time MUST be "
             "less than 65,535 seconds.Continuing with the default value\n");
            bgp->minRouteAdvertisementInterval_IBGP =
                BGP_DEFAULT_MIN_RT_ADVERTISEMENT_TIMER_IBGP;
        }
        // Min as origination timer for this node
        BgpReadTime(node,
            &bgp->minASOriginationInterval,
            "BGP-MIN-AS-ORIGINATION-INTERVAL",
            BGP_DEFAULT_MIN_AS_ORIGINATION_TIMER,
            nodeInput);
        if (bgp->minASOriginationInterval > BGP_TIME_INTERVAL_MAX_RANGE)
        {
            ERROR_ReportWarning("Min AS Orig Interval time MUST be "
             "less than 65,535 seconds.Continuing with the default value\n");
            bgp->minASOriginationInterval =
                BGP_DEFAULT_MIN_AS_ORIGINATION_TIMER;
        }
        // read keep alive interval
        BgpReadTime(node,
            &bgp->keepAliveInterval,
            "BGP-KEEPALIVE-INTERVAL",
            BGP_DEFAULT_KEEP_ALIVE_TIMER,
            nodeInput);
        if (bgp->keepAliveInterval > BGP_TIME_INTERVAL_MAX_RANGE)
        {
            ERROR_ReportWarning("Keep Alive Interval time MUST be "
                "less than 65,535 seconds.Continuing with the default value\n");
            bgp->keepAliveInterval = BGP_DEFAULT_KEEP_ALIVE_TIMER;
        }

        // read Connect Retry Timer
        BgpReadTime(node,
            &bgp->connectRetryInterval,
            "BGP-CONNECT-RETRY-INTERVAL",
            BGP_DEFAULT_CONNECT_RETRY_TIMER,
            nodeInput);

        // read Route Waiting Interval Timer
        BgpReadTime(node,
            &bgp->waitingTimeForRoute,
            "BGP-ROUTE-WAITING-INTERVAL",
            BGP_DEFAULT_ROUTE_WAITING_TIME,
            nodeInput);

        bgp->igpProbeInterval = BGP_DEFAULT_IGP_PROBE_TIME;
        bgp->advertiseIgp     = TRUE;
        bgp->redistribute2Ospf  = TRUE;
        // ospf external routes will not be distributed to BGP by default.
        bgp->advertiseOspfExternalType1 = FALSE;
        bgp->advertiseOspfExternalType2 = FALSE;

        bgp->bgpRedistributeStaticAndDefaultRts = TRUE;
        // By default make the synchronization of routes with Igp TRUE;
        bgp->isSynchronized = TRUE;
        bgp->localPref      = BGP_DEFAULT_LOCAL_PREF_VALUE;
        bgp->multiExitDisc  = BGP_DEFAULT_MULTI_EXIT_DISC_VALUE;

        NetworkProtocolType networkProtocol =
                        NetworkIpGetNetworkProtocolType(node, node->nodeId);

        if (networkProtocol == DUAL_IP)
        {
            ERROR_ReportError(
             "\nCurrently DUAL-IP cannot be enabled for a BGP speaker\n ");
        }
        else if (networkProtocol == IPV4_ONLY)
        {
            bgp->ip_version = NETWORK_IPV4;
        }
        else if (networkProtocol == IPV6_ONLY)
        {
            bgp->ip_version = NETWORK_IPV6;
        }
        else
        {
            bgp->ip_version = NETWORK_IPV4;
        }

        // Read the bgp configuration file and initialize internal routes
        // and the neighbor informations

        BgpReadConfigurationFile(node, &bgpInput, bgp, index);
        // For IGP
        if (bgp->advertiseIgp)
        {
            if (bgp->ip_version == NETWORK_IPV4)
            {
                BgpUpdateRibFromIpv4ForwardingTable(node, bgp, 
                    bgp->bgpRedistributeStaticAndDefaultRts);
            }
            else
            {
                BgpUpdateRibFromIpv6ForwardingTable(node, bgp, 
                    bgp->bgpRedistributeStaticAndDefaultRts);
            }
        }

        // Bgp speaker needs synchronization for the routes it will
        // advertise with IGP, so not choose the routes to advertise which
        // are still not learnt by IGP
        adjRibIn = (BgpRoutingInformationBase*) BUFFER_GetData(
                    &(bgp->adjRibIn));

        numEntries = BUFFER_GetCurrentSize(&(bgp->adjRibIn))
                         / sizeof(BgpRoutingInformationBase);
        in6_addr netUnreachAddr;
        memset(&netUnreachAddr,(unsigned)NETWORK_UNREACHABLE,
              sizeof(in6_addr));

        for (i = 0; i < numEntries; i++)
        {
            if (bgp->isSynchronized)
            {
                if (DEBUG)
                {
                   // debugging synchronization.
                    NetworkPrintForwardingTable(node);
                }
            }

            // Set multi exit disc and local pref attribute to the routes
            adjRibIn[i].localPref = bgp->localPref;
            adjRibIn[i].multiExitDisc = bgp->multiExitDisc;
        }

        if (bgp->advertiseIgp)
        {
            rtUpdateTimer = MESSAGE_Alloc(node,
                      APP_LAYER,
                      APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                      MSG_APP_BGP_OriginationTimer);

            MESSAGE_Send(node, rtUpdateTimer, bgp->igpProbeInterval);
        }

        // Initialize the ribLocal. ribLocal contains
        // pointers to all the entries in adjRibIn.
        adjRibIn = (BgpRoutingInformationBase*) BUFFER_GetData(
            &(bgp->adjRibIn));

        numEntries = BUFFER_GetCurrentSize(&(bgp->adjRibIn))
        / sizeof(BgpRoutingInformationBase);

        // at the time of initialization all routes are best. All routes
        // in adjRibin is also in adjRibloc.
        for (i = 0; i < numEntries; i++)
        {
            if (adjRibIn[i].isValid == TRUE)
            {
                BgpAdjRibLocStruct ribLoc;

                ribLoc.ptrAdjRibIn      = &adjRibIn[i];
                ribLoc.movedToAdjRibOut = FALSE;
                ribLoc.movedToWithdrawnRt = FALSE;
                ribLoc.isValid          = TRUE;

                BUFFER_AddDataToDataBuffer(&(bgp->ribLocal),
                       (char*) &ribLoc,
                       sizeof(BgpAdjRibLocStruct));
            }
        }

        // Initialization of bgp has been completed so send start event for
        // each connection information
        bgpConnInfo = (BgpConnectionInformationBase*)
        BUFFER_GetData(&bgp->connInfoBase);

        numEntries = BUFFER_GetCurrentSize(&(bgp->connInfoBase))
        / sizeof(BgpConnectionInformationBase);

        for (i = 0; i < numEntries; i++)
        {
            clocktype delay = 0;
            Message* newMessage = NULL;

            delay = RANDOM_nrand(bgp->timerSeed) % BGP_RANDOM_TIMER;

            newMessage = MESSAGE_Alloc(node, APP_LAYER,
                       APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                       MSG_APP_BGP_StartTimer);

            MESSAGE_InfoAlloc(node, newMessage, sizeof(int));

            memcpy(MESSAGE_ReturnInfo(newMessage),
                   &bgpConnInfo[i].uniqueId,
                   sizeof(int));

            if (DEBUG)
            {
                char string[MAX_STRING_LENGTH];
                ctoa(delay + BGP_MIN_DELAY, string);

                printf("Node #%u  fires START TIMER with delay = %s ns\n"
                       "\tfor the connection with node %d\n",
                       node->nodeId,
                       string,
                       bgpConnInfo[i].remoteId);
            }
            MESSAGE_Send(node, newMessage,
                        delay + BGP_MIN_DELAY + bgp->startTime);
        }

        if (DEBUG_TABLE)
        {
            printf("After initialization of Node %u AdjRibIn:\n",
                node->nodeId);
            BgpPrintRoutingInformationBase(node, bgp);
        }

        if (DEBUG_CONNECTION)
        {
            printf("After initialization of Node %u CIB:\n",
           node->nodeId);
            BgpPrintConnInformationBase(node, bgp);
        }

        // Listen for connections opened by the other peers
        for (i = 0; i < node->numberInterfaces; i++)
        {
            NetworkType networkType =
            NetworkIpGetInterfaceType(node, i);
            Address temp;
            temp.networkType = NETWORK_IPV4;

            if (networkType == NETWORK_IPV4)
            {
                SetIPv4AddressInfo(&temp,
                       NetworkIpGetInterfaceAddress(node, i));
            }
            else if (networkType == NETWORK_IPV6)
            {
                in6_addr tempv6;

                Ipv6GetGlobalAggrAddress(node,i,&tempv6);
                SetIPv6AddressInfo(&temp,tempv6);
            }


            APP_TcpServerListen(node,
                APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                temp,//NetworkIpGetInterfaceAddress(node, i),
                (short) APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4);
        }

        if (DEBUG_FAULT)
        {
            Message* startFault = NULL;
            Message* inFault = NULL;

            startFault = MESSAGE_Alloc(node,
                       APP_LAYER,
                       APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                       MSG_APP_PrintRoutingTable);

            inFault = MESSAGE_Alloc(node,
                    APP_LAYER,
                    APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4,
                    MSG_APP_PrintRoutingTable);

            MESSAGE_Send(node, startFault, 3 * MINUTE);
            MESSAGE_Send(node, inFault,  6 * MINUTE);
        }
    } // End of is Speaker
}


//--------------------------------------------------------------------------
// NAME:        BgpFinalize.
//
// PURPOSE:     Printing statistical information for bgp
//
// PARAMETERS:  node, node which is printing the statistical information
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//--------------------------------------------------------------------------

void
BgpFinalize(Node* node)
{
    BgpData* bgp = (BgpData *)node->appData.exteriorGatewayVar;

    if (bgp != NULL)
    {
        if (DEBUG_FINALIZE)
        {
            printf("Routing informations at the end of the "
                "simulation\n");

            BgpPrintRoutingInformationBase(node, bgp);
            NetworkPrintForwardingTable(node);
        }

        if (bgp->statsAreOn)
        {
            BgpPrintStats(node);
        }
    }
}
