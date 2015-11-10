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
// Objectives: Support Diffserv Traffic Classification and Conditioning by
//             remarking/dropping based on bandwidth/delay policy
//
// References: RFC 2475

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"
#include "network_ip.h"
#include "ipv6.h"
#include "mf_traffic_conditioner.h"
#include "transport_udp.h"
#include "transport_tcp.h"

//#define   DIFFSERV_DEBUG_METER
//#define   DIFFSERV_DEBUG_MARKER
//#define   DIFFSERV_DEBUG_INIT

//-----------------------------------------------------------------------------
// Diffserv Multi-Field Traffic Conditioner
//-----------------------------------------------------------------------------


//--------------------------------------------------------------------------
// FUNCTION:   DIFFSERV_ReturnConditionClass.
// PURPOSE:    Returns condition class to which a particular packet belongs.
// PARAMETERS: ip:IP Layer information,
//             conditionClass: Condition Class value.
// RETURNS:    the index of the condition class.
//--------------------------------------------------------------------------

IpMultiFieldTrafficConditionerInfo* DIFFSERV_ReturnConditionClass(
    NetworkDataIp* ip,
    int conditionClass)
{
    int i = 0;

    while (i < ip->numMftcInfo)
    {
        if (ip->mftcInfo[i].conditionClass == conditionClass)
        {
             return &(ip->mftcInfo[i]);
        }
        i++;
    }
    return NULL;
}


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_ReturnServiceClass.
// PURPOSE :    Check the packet belong in which service class.
// PARAMETERS:  ds - value of the ds field.
// RETURN:      return Service class or ZERO.
//--------------------------------------------------------------------------

static
unsigned char DIFFSERV_ReturnServiceClass(int ds)
{
    if (ds == DIFFSERV_DS_CLASS_AF11 ||
        ds == DIFFSERV_DS_CLASS_AF12 ||
        ds == DIFFSERV_DS_CLASS_AF13)
    {
        return DIFFSERV_DS_CLASS_AF11;
    }
    else if (ds == DIFFSERV_DS_CLASS_AF21 ||
        ds == DIFFSERV_DS_CLASS_AF22 ||
        ds == DIFFSERV_DS_CLASS_AF23)
    {
        return DIFFSERV_DS_CLASS_AF21;
    }
    else if (ds == DIFFSERV_DS_CLASS_AF31 ||
        ds == DIFFSERV_DS_CLASS_AF32 ||
        ds == DIFFSERV_DS_CLASS_AF33)
    {
        return DIFFSERV_DS_CLASS_AF31;
    }
    else if (ds == DIFFSERV_DS_CLASS_AF41 ||
        ds == DIFFSERV_DS_CLASS_AF42 ||
        ds == DIFFSERV_DS_CLASS_AF43)
    {
        return DIFFSERV_DS_CLASS_AF41;
    }
    else if (ds == DIFFSERV_DS_CLASS_EF)
    {
        return DIFFSERV_DS_CLASS_EF;
    }
    return 0;
}
unsigned char DIFFSERV_ReturnServiceClass_NoNMS(int ds)
{
    if (ds == DIFFSERV_DS_CLASS_AF11 ||
        ds == DIFFSERV_DS_CLASS_AF12 ||
        ds == DIFFSERV_DS_CLASS_AF13)
    {
        return DIFFSERV_DS_CLASS_AF11;
    }
    else if (ds == DIFFSERV_DS_CLASS_AF21 ||
        ds == DIFFSERV_DS_CLASS_AF22 ||
        ds == DIFFSERV_DS_CLASS_AF23)
    {
        return DIFFSERV_DS_CLASS_AF21;
    }
    else if (ds == DIFFSERV_DS_CLASS_AF31 ||
        ds == DIFFSERV_DS_CLASS_AF32 ||
        ds == DIFFSERV_DS_CLASS_AF33)
    {
        return DIFFSERV_DS_CLASS_AF31;
    }
    else if (ds == DIFFSERV_DS_CLASS_AF41 ||
        ds == DIFFSERV_DS_CLASS_AF42 ||
        ds == DIFFSERV_DS_CLASS_AF43)
    {
        return DIFFSERV_DS_CLASS_AF41;
    }
    else if (ds == DIFFSERV_DS_CLASS_EF)
    {
        return DIFFSERV_DS_CLASS_EF;
    }
    return 0;
}

//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_ReturnServiceClass.
// PURPOSE :    Check the packet belong in which service class.
// PARAMETERS:  ds - value of the ds field.
// RETURN:      return Service class or ZERO.
//--------------------------------------------------------------------------

unsigned char DIFFSERV_ReturnServiceClass(Node* node, int ds)
{
    return DIFFSERV_ReturnServiceClass_NoNMS(ds);
}



//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_ReturnIPv4SourcePort
// PURPOSE:     Returns Source port of the message.
// PARAMETERS:  msg - application packet or fragments.
// RETURN:      return source port number.
//--------------------------------------------------------------------------

static
unsigned short DIFFSERV_ReturnIPv4SourcePort(Message* msg)
{
    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    char* transportHdr = NULL;
    unsigned short srcPort = 0;

    if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
    {
        if (IpHeaderGetIpLength(ipHdr->ip_v_hl_tos_len) <=
            (unsigned)MESSAGE_ReturnPacketSize(msg))
        {
            transportHdr  = ((char*) ipHdr +
                (IpHeaderGetHLen(ipHdr->ip_v_hl_tos_len) * 4));
            if (ipHdr->ip_p == IPPROTO_TCP)
            {
                struct tcphdr* tcpHeader = (struct tcphdr*)transportHdr;
                srcPort = tcpHeader->th_sport;
            }
            else if (ipHdr->ip_p == IPPROTO_UDP)
            {
                TransportUdpHeader* udpHeader  = 
                        (TransportUdpHeader*)transportHdr;
                srcPort = udpHeader->sourcePort;
            }
        }
    }else
    {
        ERROR_Assert(FALSE, "ERROR in finding IPv6 packet.");
    }
    return srcPort;
}


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_ReturnIPv4DestPort
// PURPOSE:     Returns Destination port of the message.
// PARAMETERS:  msg -  application packet or fragments.
//
// RETURN:      return protocol Id.
//--------------------------------------------------------------------------

static
unsigned short DIFFSERV_ReturnIPv4DestPort(Message* msg)
{
    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    char* transportHdr = NULL;
    unsigned short dstPort = 0;

    if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
    {
        if (IpHeaderGetIpLength(ipHdr->ip_v_hl_tos_len) <=
            (unsigned)MESSAGE_ReturnPacketSize(msg))
        {
            transportHdr = ((char*) ipHdr +
                (IpHeaderGetHLen(ipHdr->ip_v_hl_tos_len) * 4));
            if (ipHdr->ip_p == IPPROTO_TCP)
            {
                struct tcphdr* tcpHeader = (struct tcphdr*)transportHdr;
                dstPort = tcpHeader->th_dport;
            }
            else if (ipHdr->ip_p == IPPROTO_UDP)
            {
                TransportUdpHeader* udpHeader  = 
                        (TransportUdpHeader*)transportHdr;
                dstPort = udpHeader->destPort;
            }
        }
    }
    else
    {
        ERROR_Assert(FALSE, "ERROR in finding IPv6 packet.");
    }
    return dstPort;
}

//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_ReturnIPv6ProtocolId
// PURPOSE:     Returns Destination port of the message.
// PARAMETERS:  msg -  application packet or fragments.
//              srcPort - Pointer to source transportation port
//              dstPort - Pointer to destination transportation port
// RETURN:      return destination port number.
//--------------------------------------------------------------------------
static
unsigned char DIFFSERV_ReturnIPv6ProtocolId(Message *msg,
                                            unsigned short * srcPort,
                                            unsigned short * dstPort)
{
    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    unsigned char ipProtocol = IPPROTO_IPV6;
    *srcPort = 0;
    *dstPort = 0;

    if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPV6_VERSION)
    {
        ip6_hdr* ipv6Header = (ip6_hdr*) MESSAGE_ReturnPacket(msg);
        int nextHdr = ipv6Header->ip6_nxt;
        int hdrLength = ipv6Header->ip6_plen;
        char* payload = MESSAGE_ReturnPacket(msg);
        payload += sizeof(ip6_hdr);

        // look at payload
        while (nextHdr)
        {
            if (nextHdr == IPPROTO_UDP) {
                // this is a UDP packet
                TransportUdpHeader* udpHdr;
                udpHdr = (TransportUdpHeader*) payload;

                *srcPort = udpHdr->sourcePort;
                *dstPort = udpHdr->destPort;
                ipProtocol = IPPROTO_UDP;
                break;
            }

            else if (nextHdr == IPPROTO_TCP) {
                // this is a TCP packet
                struct tcphdr* tcpHdr;
                tcpHdr = (struct tcphdr *) payload;

                *srcPort = tcpHdr->th_sport;
                *dstPort = tcpHdr->th_dport;
                ipProtocol = IPPROTO_TCP;
                break;
            }
            else if (nextHdr == IPPROTO_ICMPV6 )
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                *srcPort = 0;
                *dstPort = 0;
                ipProtocol = IPPROTO_ICMPV6;
                break;
            }
#ifdef ENTERPRISE_LIB
            else if (nextHdr == IPPROTO_OSPF)
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                *srcPort = 0;
                *dstPort = 0;
                ipProtocol = (unsigned char) nextHdr;
                break;
            }
#endif
#ifdef WIRELESS_LIB
            else if ((nextHdr == IPPROTO_AODV) || (nextHdr == IPPROTO_DYMO))
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                *srcPort = 0;
                *dstPort = 0;
                ipProtocol = (unsigned char) nextHdr;
                break;
            }
#endif
#ifdef ADVANCED_WIRELESS_LIB
            else if (nextHdr == IPPROTO_DOT16)
            {
                *srcPort = 0;
                *dstPort = 0;
                ipProtocol = (unsigned char) nextHdr;
                break;
            }
#endif
            else if (nextHdr == IP6_NHDR_FRAG)
            {
                *srcPort = 0;
                *dstPort = 0;
                ipProtocol = IPPROTO_IPV6;
                break;
            }
            else if (nextHdr == IPPROTO_IPV6 )
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                *srcPort = 0;
                *dstPort = 0;
                ipProtocol = (unsigned char)nextHdr;
                break;
            }

            // get next header -
            // we do not support other IPv6 next header type
            ERROR_Assert(FALSE, "DiffServ packet format incorrect.");

            payload += ipv6Header->ip6_plen;
            ipv6Header = (ip6_hdr*) payload;
            nextHdr = ipv6Header->ip6_nxt;
            hdrLength = ipv6Header->ip6_plen;

            if (hdrLength == 0)
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                *srcPort = 0;
                *dstPort = 0;
                ipProtocol = IPPROTO_IPV6;
                break;
            }
        }
    }else {
        ERROR_Assert(FALSE, "DiffServ packet format incorrect.");
    }
    return ipProtocol;
}

//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_TrafficConditionerCheckForValidAddress.
// PURPOSE :    Check whether the node specified in the input file for
//              traffic conditioning is valid or not.
// PARAMETERS:  node - pointer to the node,
//              inputString - the input file.
//              addressString - the source address string.
//              nodeId - nodeId of the source Node.
//              address - address of the source node.
// RETURN:      None.
//--------------------------------------------------------------------------

static
void DIFFSERV_TrafficConditionerCheckForValidAddress(
    Node* node,
    char* inputString,
    char* addressString,
    NodeAddress* nodeId,
    Address* address)
{
    BOOL  isNodeId = FALSE;

    if (strchr(addressString, ':'))
    {
        IO_ParseNodeIdOrHostAddress(
            addressString, &((*address).interfaceAddr.ipv6), nodeId);

        if (!*nodeId)
        {
            isNodeId = FALSE;
            *nodeId = MAPPING_GetNodeIdFromGlobalAddr(
                                node, (*address).interfaceAddr.ipv6);

            if (*nodeId == INVALID_MAPPING)
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr, "%s is not a valid nodeId or Address\n"
                        "\ton line:%s\n",
                        addressString, inputString);

                ERROR_ReportError(errorStr);
            }
            (*address).networkType = NETWORK_IPV6;
        }
    }else
    {
        IO_ParseNodeIdOrHostAddress(addressString, nodeId, &isNodeId);

        if (!isNodeId)
        {
            (*address).interfaceAddr.ipv4 = *nodeId;
            *nodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
            (*address).interfaceAddr.ipv4);

            if (*nodeId == INVALID_MAPPING)
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr, "%s is not a valid nodeId or Address\n"
                        "\ton line:%s\n",
                        addressString, inputString);

                ERROR_ReportError(errorStr);
            }
            (*address).networkType = NETWORK_IPV4;
        }

    }

    if (*nodeId && isNodeId)
    {
        (*address).networkType = NETWORK_INVALID;

        // Got a node Id , Find Default interface address, network type
        (*address) = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
                                                        node, *nodeId);

        if ((*address).networkType == NETWORK_INVALID)
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "%s is not a valid nodeId or Address\n"
                    "\ton line:%s\n",
                    addressString, inputString);

            ERROR_ReportError(errorStr);
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_Marker
// PURPOSE :    It will be marked the packet.
// PARAMETERS:  preference - packet will be marked according this field.
//              msg - pointer to the message,
// RETURN:      void.
//--------------------------------------------------------------------------

static
void DIFFSERV_Marker(
    int preference,
    unsigned char serviceClass,
    Message* msg)
{
    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
    {
        switch (preference)
        {
            case DIFFSERV_CONFORMING:
            {
                IpHeaderSetTOS(&(ipHdr->ip_v_hl_tos_len),
                    DIFFSERV_SET_TOS_FIELD((serviceClass),
                    IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len)));
                break;
            }
            case DIFFSERV_PART_CONFORMING:
            {
                ERROR_Assert(serviceClass != DIFFSERV_DS_CLASS_EF,
                    "Not possible to mark PART-CONFORM packet for EF Service");

                IpHeaderSetTOS(&(ipHdr->ip_v_hl_tos_len),
                    DIFFSERV_SET_TOS_FIELD((serviceClass + 2),
                    IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len)));
                break;
            }
            case DIFFSERV_NON_CONFORMING:
            {
                ERROR_Assert(serviceClass != DIFFSERV_DS_CLASS_EF,
                    "Not possible to mark OUT-PROFILE packet for EF Service");

                IpHeaderSetTOS(&(ipHdr->ip_v_hl_tos_len),
                    DIFFSERV_SET_TOS_FIELD((serviceClass + 4),
                    IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len)));
                break;
            }
            default:
            {
                ERROR_ReportError("Invalid switch value");
            }
        } //End switch
    }
     else if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPV6_VERSION)
    {
        ip6_hdr* ipv6Header = (ip6_hdr*) MESSAGE_ReturnPacket(msg);

        switch (preference)
        {
            case DIFFSERV_CONFORMING:
            {
                ip6_hdrSetClass(&(ipv6Header->ipv6HdrVcf),
                    DIFFSERV_SET_TOS_FIELD(serviceClass,
                    ip6_hdrGetClass(ipv6Header->ipv6HdrVcf)));
                break;
            }
            case DIFFSERV_PART_CONFORMING:
            {
                ERROR_Assert(serviceClass != DIFFSERV_DS_CLASS_EF,
                    "Not possible to mark PART-CONFORM packet for EF Service");

                ip6_hdrSetClass(&(ipv6Header->ipv6HdrVcf),
                    DIFFSERV_SET_TOS_FIELD((serviceClass + 2),
                    ip6_hdrGetClass(ipv6Header->ipv6HdrVcf)));
                break;
            }
            case DIFFSERV_NON_CONFORMING:
            {
                ERROR_Assert(serviceClass != DIFFSERV_DS_CLASS_EF,
                    "Not possible to mark OUT-PROFILE packet for EF Service");

                 ip6_hdrSetClass(&(ipv6Header->ipv6HdrVcf),
                     DIFFSERV_SET_TOS_FIELD((serviceClass + 4),
                    ip6_hdrGetClass(ipv6Header->ipv6HdrVcf))) ;
                break;
            }
        }
    }
    else
    {
        ERROR_ReportError("Invalid packet Ip version");
    }
}


//--------------All Meter Functions Start-------------------//

//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_PacketFitsTokenBucketProfile.
// PURPOSE :    Determin in which color the coming packet is marked by
//              the Token Bucket policer.
// PARAMETERS:  node - pointer to the node,
//              msg - pointer to the message,
//              conditionClass -  the condition class to which this
//                                packet belongs.
// RETURN:      TRUE or FALSE.
//--------------------------------------------------------------------------

static
BOOL DIFFSERV_PacketFitsTokenBucketProfile(
    Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int conditionClass,
    int* preference)
{
    Int32 tokensGeneratedSinceLastUpdate = 0;
    int packetSize = MESSAGE_ReturnPacketSize(msg);
    IpMultiFieldTrafficConditionerInfo* conditionClassInfo =
        DIFFSERV_ReturnConditionClass(ip, conditionClass);
    BOOL retVal = TRUE;

    ERROR_Assert(conditionClassInfo, "Unable to locate conditionClass");

    tokensGeneratedSinceLastUpdate =
        (Int32)((conditionClassInfo->rate * (node->getNodeTime()
            - conditionClassInfo->lastTokenUpdateTime)) / SECOND);

    conditionClassInfo->tokens =
        MIN(conditionClassInfo->tokens + tokensGeneratedSinceLastUpdate,
            conditionClassInfo->committedBurstSize);

     // Note the last update time.
     conditionClassInfo->lastTokenUpdateTime = node->getNodeTime();

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tBefore Metering(TB) for ConditionClass:%d\n"
            "\tPacketSize = %u, numOfTokens = %d\n",
            conditionClass, packetSize, conditionClassInfo->tokens);
    }
#endif
     // Are there enough tokens to admit this packet?
     if (conditionClassInfo->tokens >= packetSize)
     {
         // This packet is In-Profile: subtract the tokens from the bucket
         // and return TRUE for In-Profile
         *preference = DIFFSERV_CONFORMING;
         conditionClassInfo->numPacketsConforming++;
         conditionClassInfo->tokens -= packetSize;
    }
    else
    {
        // This packet is Out-Profile: return FALSE for Out-Profile
        *preference = DIFFSERV_PART_CONFORMING;
        conditionClassInfo->numPacketsNonConforming++;
        retVal = FALSE;
    }

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tAfter Metering(TB) for ConditionClass:%d\n"
            "\tPacketSize=%u, numOfTokens = %d and preference=%u\n",
            conditionClass, packetSize, conditionClassInfo->tokens,
           *preference);
    }
#endif

    return retVal;
}
//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_PacketFitsSingleRateThreeColorProfile
// PURPOSE :    Determin in which color the coming packet is marked by
//              the Single Rate Three Color policer.
// PARAMETERS:  node - pointer to the node,
//              msg - pointer to the message,
//              conditionClass - the condition class to which this
//                                 packet belongs.
//              preference - Meter assign the different type of preference
//                           into this variable by which Marker mark
//                           the packet.
// RETURN:  TRUE or FALSE.
//--------------------------------------------------------------------------

static
BOOL DIFFSERV_PacketFitsSingleRateThreeColorProfile(
    Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int conditionClass,
    int* preference)
{
    Int32 tokensGeneratedSinceLastUpdate = 0;
    int packetSize = MESSAGE_ReturnPacketSize(msg);
    IpMultiFieldTrafficConditionerInfo* conditionClassInfo =
        DIFFSERV_ReturnConditionClass(ip, conditionClass);
    BOOL retVal = TRUE;
    unsigned char serviceClass = 0;
    unsigned char packetDSCode;

    ERROR_Assert(conditionClassInfo, "Unable to locate conditionClass");

    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
    {
        // handling Ipv4
        packetDSCode = (unsigned char)
            (IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len) >> 2);
    }else {
        ip6_hdr * ipv6_hdr = (ip6_hdr*) MESSAGE_ReturnPacket(msg);
        packetDSCode =
            ip6_hdrGetClass(ipv6_hdr->ipv6HdrVcf) >> 2;
    }

    if (conditionClassInfo->colorAware)
    {
        serviceClass = DIFFSERV_ReturnServiceClass(packetDSCode);
                                                   
        ERROR_Assert(serviceClass, "Unable to locate serviceClass");
    }

    //Calculate how many tokens are generated within the interval between
    //last update time and current time.
    tokensGeneratedSinceLastUpdate =
        (Int32)((conditionClassInfo->rate * (node->getNodeTime()
            - conditionClassInfo->lastTokenUpdateTime)) / SECOND);

    //Calculate the amount of tokens in the two Buckets.
    if (conditionClassInfo->tokens + tokensGeneratedSinceLastUpdate <=
        conditionClassInfo->committedBurstSize)
    {
        conditionClassInfo->tokens += tokensGeneratedSinceLastUpdate;
    }
    else
    {
        tokensGeneratedSinceLastUpdate = tokensGeneratedSinceLastUpdate -
            (conditionClassInfo->committedBurstSize -
            conditionClassInfo->tokens);
        conditionClassInfo->tokens = conditionClassInfo->committedBurstSize;

        if (conditionClassInfo->largeBucketTokens +
            tokensGeneratedSinceLastUpdate <=
            conditionClassInfo->excessBurstSize)
        {
            conditionClassInfo->largeBucketTokens +=
                tokensGeneratedSinceLastUpdate;
        }
        else
        {
            conditionClassInfo->largeBucketTokens =
                conditionClassInfo->excessBurstSize;
        }
    }
    //Note the last update time.
    conditionClassInfo->lastTokenUpdateTime = node->getNodeTime();

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tBefore Metering(srTCM) for ConditionClass:%d\n"
            "\tPacketSize = %u, TokensInSmallBucket = %d, "
            "TokensInLargeBucket = %d\n",
            conditionClass, packetSize, conditionClassInfo->tokens,
            conditionClassInfo->largeBucketTokens);
    }
#endif
    // If Meter is color aware, then handle the packet in that way:
    // Conforming, if small bucket has enough token to admit this packet
    // which is previously marked as Green.
    // Partial-conforming, if large bucket has enough token to admit this
    // packet which is previously marked as Green or yellow.
    // Otherwise packet is Non-conforming.

    if (conditionClassInfo->colorAware)
    {
        if (conditionClassInfo->tokens > packetSize &&
            packetDSCode == serviceClass)
        {
            *preference = DIFFSERV_CONFORMING;
            conditionClassInfo->numPacketsConforming++;
            conditionClassInfo->tokens -= packetSize;
        }
        else if (conditionClassInfo->largeBucketTokens > packetSize &&
            (packetDSCode == serviceClass ||
                packetDSCode == (serviceClass + 2)))
        {
            *preference = DIFFSERV_PART_CONFORMING;
            conditionClassInfo->largeBucketTokens -= packetSize;
            conditionClassInfo->numPacketsPartConforming++;
        }
        else
        {
            *preference = DIFFSERV_NON_CONFORMING;
            conditionClassInfo->numPacketsNonConforming++;
            retVal = FALSE;
        }
    }

    // If Meter is color blind, then handle the packet in that way:
    // Conforming, if small bucket has enough token to admit this packet.
    // Partial-conforming,if large bucket has enough token to admit packet.
    // Otherwise packet is Non-conforming.

    else
    {
        if (conditionClassInfo->tokens > packetSize)
        {
            *preference = DIFFSERV_CONFORMING;
            conditionClassInfo->tokens -= packetSize;
            conditionClassInfo->numPacketsConforming++;
        }
        else if (conditionClassInfo->largeBucketTokens > packetSize)
        {
            *preference = DIFFSERV_PART_CONFORMING;
            conditionClassInfo->largeBucketTokens -= packetSize;
            conditionClassInfo->numPacketsPartConforming++;
        }
        else
        {
            *preference = DIFFSERV_NON_CONFORMING;
            conditionClassInfo->numPacketsNonConforming++;
            retVal = FALSE;
        }
    }

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tAfter Metering(srTCM) for ConditionClass:%d\n"
            "\tPacketSize = %u, TokensInSmallBucket = %d, "
            "TokensInLargeBucket = %d, preference=%u\n",
            conditionClass, packetSize, conditionClassInfo->tokens,
            conditionClassInfo->largeBucketTokens, *preference);
    }
#endif
    return retVal;
}


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_PacketFitsTwoRateThreeColorProfile
// PURPOSE :    Determin in which color the coming packet is marked by
//              the Two Rate Three Color policer.
// PARAMETERS:  node - pointer to the node,
//              msg - pointer to the message,
//              conditionClass - the condition class to which this
//                                packet belongs.
//              preference - Meter assign the different type of preference
//                           into this variable by which Marker mark
//                           the packet.
// RETURN:      TRUE or FALSE.
//--------------------------------------------------------------------------

static
BOOL DIFFSERV_PacketFitsTwoRateThreeColorProfile(
    Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int conditionClass,
    int* preference)
{
    Int32 tokensGeneratedSinceLastUpdate = 0;
    int packetSize = MESSAGE_ReturnPacketSize(msg);
    IpMultiFieldTrafficConditionerInfo* conditionClassInfo =
        DIFFSERV_ReturnConditionClass(ip, conditionClass);
    BOOL retVal = TRUE;
    unsigned char serviceClass = 0;
    unsigned char packetDSCode;

    ERROR_Assert(conditionClassInfo, "Unable to locate conditionClass");

    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
    {
        // handling Ipv4
        packetDSCode = (unsigned char)
            (IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len) >> 2);
    }else {
        ip6_hdr * ipv6_hdr = (ip6_hdr*) MESSAGE_ReturnPacket(msg);
        packetDSCode = ip6_hdrGetClass(ipv6_hdr->ipv6HdrVcf) >> 2;
    }

    if (conditionClassInfo->colorAware)
    {
        serviceClass = DIFFSERV_ReturnServiceClass(packetDSCode);
                                                   
        ERROR_Assert(serviceClass, "Unable to locate serviceClass");
    }

    // Calculate how many tokens are generated within the interval between
    // last update time and current time accroding to CIR  and PIR rate.
    // Increments trTCM state variables tockens (cBucket) and
    // largeBucketTokens (pBucket) according to the elapsed time since
    // the last update time of the buckets.  tokens is filled at a rate
    // equal to CIR, capped at an upper bound of CBS. largeBucketTokens
    // is filled at a rate equal to PIR, capped at an upper bound of PBS.

    tokensGeneratedSinceLastUpdate =
        (Int32)((conditionClassInfo->rate * (node->getNodeTime()
            - conditionClassInfo->lastTokenUpdateTime)) / SECOND);

    if (conditionClassInfo->tokens + tokensGeneratedSinceLastUpdate <=
        conditionClassInfo->committedBurstSize)
    {
        conditionClassInfo->tokens += tokensGeneratedSinceLastUpdate;
    }
    else
    {
        conditionClassInfo->tokens = conditionClassInfo->committedBurstSize;
    }

    tokensGeneratedSinceLastUpdate =
        (Int32)((conditionClassInfo->peakInformationRate * (node->getNodeTime()
            - conditionClassInfo->lastTokenUpdateTime)) / SECOND);

    if (conditionClassInfo->largeBucketTokens +
        tokensGeneratedSinceLastUpdate <=
        conditionClassInfo->excessBurstSize)
    {
        conditionClassInfo->largeBucketTokens +=
            tokensGeneratedSinceLastUpdate;
    }
    else
    {
        conditionClassInfo->largeBucketTokens =
            conditionClassInfo->excessBurstSize;
    }
    //Note the last update time.
    conditionClassInfo->lastTokenUpdateTime = node->getNodeTime();

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tBefore Metering(trTCM) for ConditionClass:%d\n"
            "\tPacketSize = %u, TokensInSmallBucket = %d, "
            "TokensInLargeBucket = %d\n",
            conditionClass, packetSize, conditionClassInfo->tokens,
            conditionClassInfo->largeBucketTokens);
    }
#endif

    if (!conditionClassInfo->colorAware)
    {
        // If meter is not ColorAware and if Tp(t)-B < 0, the packet
        // is red, else
        // if Tc(t)-B < 0, the packet is yellow and Tp is decremented by B,
        // else
        // the packet is green and both Tp and Tc are decremented by B.

        if (conditionClassInfo->largeBucketTokens < packetSize)
        {
            *preference = DIFFSERV_NON_CONFORMING;
            conditionClassInfo->numPacketsNonConforming++;
            retVal = FALSE;
        }
        else if (conditionClassInfo->tokens < packetSize)
        {
            *preference = DIFFSERV_PART_CONFORMING;
            conditionClassInfo->largeBucketTokens -= packetSize;
            conditionClassInfo->numPacketsPartConforming++;
        }
        else
        {
            *preference = DIFFSERV_CONFORMING;
            conditionClassInfo->tokens -= packetSize;
            conditionClassInfo->largeBucketTokens -= packetSize;
            conditionClassInfo->numPacketsConforming++;
        }
    }
    else
    {
        // If meter is ColorAware and if the packet has been precolored
        // as red or if Tp(t)-B < 0, the packet is red, else
        // if the packet has been precolored as yellow or if Tc(t)-B < 0,
        // the packet is yellow and Tp is decremented by B, else
        // the packet is green and both Tp and Tc are decremented by B.

        if (packetDSCode == (serviceClass + 4) ||
            conditionClassInfo->largeBucketTokens < packetSize)
        {
            *preference = DIFFSERV_NON_CONFORMING;
            conditionClassInfo->numPacketsNonConforming++;
            retVal = FALSE;
        }
        else if (packetDSCode == (serviceClass + 2) ||
            conditionClassInfo->tokens < packetSize)
        {
            *preference = DIFFSERV_PART_CONFORMING;
            conditionClassInfo->largeBucketTokens -= packetSize;
            conditionClassInfo->numPacketsPartConforming++;
        }
        else
        {
            *preference = DIFFSERV_CONFORMING;
            conditionClassInfo->tokens -= packetSize;
            conditionClassInfo->largeBucketTokens -= packetSize;
            conditionClassInfo->numPacketsConforming++;
        }
    }

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tAfter Metering(trTCM) for ConditionClass:%d\n"
            "\tPacketSize = %u, TokensInSmallBucket = %d, "
            "TokensInLargeBucket = %d, preference=%u\n",
            conditionClass, packetSize, conditionClassInfo->tokens,
            conditionClassInfo->largeBucketTokens, *preference);
    }
#endif
    return retVal;
}


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_PacketFitsTSW2CMProfile
// PURPOSE :    Determin in which color the coming packet is marked by
//              the Time Sliding Window Two Color policer.
// PARAMETERS:  node - pointer to the node,
//              msg - pointer to the message,
//              conditionClass - the condition class to which this
//                               packet belongs.
//              preference - Meter assign the different type of preference
//                           into this variable by which Marker mark
//                           the packet.
// RETURN:      TRUE or FALSE.
//--------------------------------------------------------------------------

static
BOOL DIFFSERV_PacketFitsTSW2CMProfile(
    Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int conditionClass,
    int* preference)
{
    Int32 bytesInTimeSlidingWindow = 0;
    int packetSize = MESSAGE_ReturnPacketSize(msg);
    IpMultiFieldTrafficConditionerInfo* conditionClassInfo =
        DIFFSERV_ReturnConditionClass(ip, conditionClass);
    float randomNumber = (float)RANDOM_erand(ip->trafficConditionerSeed);
    BOOL retVal = TRUE;

    ERROR_Assert(conditionClassInfo,
        "Unable to locate conditionClass");

    bytesInTimeSlidingWindow = (Int32)(conditionClassInfo->avgRate *
        conditionClassInfo->winLen + packetSize);

    conditionClassInfo->avgRate = bytesInTimeSlidingWindow /
        ((node->getNodeTime() - conditionClassInfo->lastTokenUpdateTime) /
        (SECOND * 1.0) + conditionClassInfo->winLen);

    ERROR_Assert(conditionClassInfo->avgRate,
        "avgRate become ZERO, It should not be occur");

    conditionClassInfo->lastTokenUpdateTime = node->getNodeTime();

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tBefore Metering(TSW2CM) for ConditionClass:%d\n"
            "\tPacketSize = %u, AvgRate = %f\n",
            conditionClass, packetSize, conditionClassInfo->avgRate);
    }
#endif
    if (conditionClassInfo->avgRate > conditionClassInfo->rate &&
        randomNumber <= (1 - (conditionClassInfo->rate /
        conditionClassInfo->avgRate)))
    {
        // This packet is Out-Profile: return FALSE for Out-Profile
        *preference = DIFFSERV_PART_CONFORMING;
        conditionClassInfo->numPacketsNonConforming++;
        retVal = FALSE;
    }
    else
    {
         // This packet is In-Profile: return TRUE for In-Profile
         *preference = DIFFSERV_CONFORMING;
         conditionClassInfo->numPacketsConforming++;
    }

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tAfter Metering(TSW2CM) for ConditionClass:%d\n"
            "\tPacketSize = %u, RandomNumber = %f, preference=%u\n",
            conditionClass, packetSize, randomNumber, *preference);
    }
#endif
    return retVal;
}


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_PacketFitsTSW3CMProfile
// PURPOSE :    Determin in which color the coming packet is marked by
//              the Time Sliding Window Three Color policer.
// PARAMETERS:  node - pointer to the node,
//              msg - pointer to the message,
//              conditionClass - the condition class to which this
//                               packet belongs.
//              preference - Meter assign the different type of preference
//                           into this variable by which Marker mark
//                           the packet.
// RETURN:      TRUE or FALSE.
//--------------------------------------------------------------------------

static
BOOL DIFFSERV_PacketFitsTSW3CMProfile(
    Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int conditionClass,
    int* preference)
{
    Int32 bytesInTimeSlidingWindow = 0;
    int packetSize = MESSAGE_ReturnPacketSize(msg);
    IpMultiFieldTrafficConditionerInfo* conditionClassInfo =
        DIFFSERV_ReturnConditionClass(ip, conditionClass);
    BOOL retVal = TRUE;
    float randomNumber = (float)RANDOM_erand(ip->trafficConditionerSeed);
    float probability1 = 0.0;
    float probability2 = 0.0;

    ERROR_Assert(conditionClassInfo, "Unable to locate conditionClass");

    bytesInTimeSlidingWindow = (Int32)(conditionClassInfo->avgRate *
        conditionClassInfo->winLen + packetSize);

    conditionClassInfo->avgRate = bytesInTimeSlidingWindow /
        ((node->getNodeTime() - conditionClassInfo->lastTokenUpdateTime) /
        (SECOND * 1.0) + conditionClassInfo->winLen);

    ERROR_Assert(conditionClassInfo->avgRate,
        "avgRate become ZERO, It should not be occur");

    conditionClassInfo->lastTokenUpdateTime = node->getNodeTime();

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tBefore Metering(TSW3CM) for ConditionClass:%d\n"
            "\tPacketSize = %u, AvgRate = %f\n",
            conditionClass, packetSize, conditionClassInfo->avgRate);
    }
#endif
    if (conditionClassInfo->avgRate <= conditionClassInfo->rate)
    {
        *preference = DIFFSERV_CONFORMING;
        conditionClassInfo->numPacketsConforming++;
    }
    else if ((conditionClassInfo->avgRate <=
        conditionClassInfo->peakInformationRate) &&
        (conditionClassInfo->avgRate > conditionClassInfo->rate))
    {
        probability1 = (float)(1 - (conditionClassInfo->rate /
                                conditionClassInfo->avgRate));
        if (randomNumber < probability1)
        {
            *preference = DIFFSERV_PART_CONFORMING;
            conditionClassInfo->numPacketsPartConforming++;
        }
        else
        {
            *preference = DIFFSERV_CONFORMING;
            conditionClassInfo->numPacketsConforming++;
        }
    }
    else
    {
        probability1 = (float)(1 - (conditionClassInfo->peakInformationRate /
                            conditionClassInfo->avgRate));
        probability2 = (float)((conditionClassInfo->peakInformationRate -
                            conditionClassInfo->rate) /
                            conditionClassInfo->avgRate);
        if (randomNumber < probability1)
        {
             *preference = DIFFSERV_NON_CONFORMING;
             conditionClassInfo->numPacketsNonConforming++;
             retVal = FALSE;
        }
        else if (randomNumber < (probability1 + probability2))
        {
            *preference = DIFFSERV_PART_CONFORMING;
            conditionClassInfo->numPacketsPartConforming++;
        }
        else
        {
            *preference = DIFFSERV_CONFORMING;
            conditionClassInfo->numPacketsConforming++;
        }
    }

#ifdef DIFFSERV_DEBUG_METER
    {
        printf("\tAfter Metering(TSW3CM) for ConditionClass:%d\n"
            "\tPacketSize = %u, preference=%u\n",
            conditionClass, packetSize, *preference);
    }
#endif
    return retVal;
}
//--------------All Meter Functions End-------------------//


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_AddConditionClassToMeterMapping.
// PURPOSE:     Initializing the parameters for profiling traffic.
// PARAMETERS:  node - Pointer to the node,
//              conditionClass - condition class value,
// RETURN:      none.
//--------------------------------------------------------------------------

static
void DIFFSERV_AddConditionClassToMeterMapping(
    Node* node,
    int conditionClass)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    IpMultiFieldTrafficConditionerInfo* conditionClassInfo =
        DIFFSERV_ReturnConditionClass(ip, conditionClass);

    if (conditionClassInfo)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "Already there is an entry for the "
            "condition class %d\n",conditionClass);
        ERROR_ReportError(errorStr);
    }

    if (ip->numMftcInfo == 0 || ip->numMftcInfo == ip->maxMftcInfo)
    {
        // Allocate the memory space for the structure IpMftcParameter.
        IpMultiFieldTrafficConditionerInfo* newMftcInfo = NULL;

        ip->maxMftcInfo += DIFFSERV_NUM_INITIAL_MFTC_INFO_ENTRIES;
        newMftcInfo = (IpMultiFieldTrafficConditionerInfo *) MEM_malloc(
                          sizeof(IpMultiFieldTrafficConditionerInfo) *
                          ip->maxMftcInfo);
        // Set the memory space to ZERO.
        memset(newMftcInfo, 0, sizeof(IpMultiFieldTrafficConditionerInfo) *
            ip->maxMftcInfo);

        if (ip->numMftcInfo != 0)
        {
            // Copy the old data of IpMftcParameter to newMftcInfo
            memcpy(newMftcInfo, ip->mftcInfo,
                sizeof(IpMultiFieldTrafficConditionerInfo) *
                ip->numMftcInfo);
            MEM_free(ip->mftcInfo);
        } //End if
        ip->mftcInfo = newMftcInfo;
    } //End if
    ip->numMftcInfo++;
}


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_AddConditionClassToOutProfileMapping.
// PURPOSE:     Initializing the parameters for profiling traffic.
// PARAMETERS:  node - Pointer to the node,
//              conditionClass - condition class value,
//              outProfile - To profile with Marking or Dropping.
// RETURN:      none.
//--------------------------------------------------------------------------

static
void DIFFSERV_AddConditionClassToOutProfileMapping(
    Node* node,
    int conditionClass,
    BOOL dropOutProfilePackets,
    int dscpToMark)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    IpMultiFieldTrafficConditionerInfo* conditionClassInfo = NULL;
    conditionClassInfo = DIFFSERV_ReturnConditionClass(ip, conditionClass);

    if (!conditionClassInfo)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "There is no entry for the condition class %d\n"
            "Cannot specify an out-of-profile action for nonexistent "
            "class",
            conditionClass);
        ERROR_ReportError(errorStr);
    }
    conditionClassInfo->dropOutProfilePackets = dropOutProfilePackets;
    conditionClassInfo->dsToMarkOutProfilePackets =
                                                (unsigned char) dscpToMark;

#ifdef DIFFSERV_DEBUG_INIT
    {
        printf("Into node %u Condition Class:%d, markOrDrop:%s\n",
            node->nodeId, conditionClass,
            (conditionClassInfo->dropOutProfilePackets == TRUE ?
            "DROP" : "MARK"));
    }
#endif
    return;
}

static
BOOL DIFFSERV_AddressIsUnused(Address comparedAddress)
{
    ERROR_Assert((comparedAddress.networkType == NETWORK_IPV4) ||
        (comparedAddress.networkType == NETWORK_IPV6)||
        (comparedAddress.networkType == NETWORK_DUAL), "Invalid address type. ");

    BOOL ret = FALSE;
    if (comparedAddress.networkType == NETWORK_IPV4)
    {
        if (comparedAddress.interfaceAddr.ipv4
            == DIFFSERV_UNUSED_FIELD_ADDRESS)
        {
            ret = TRUE;
        }
    }else if ((comparedAddress.networkType == NETWORK_IPV6) ||
        (comparedAddress.networkType == NETWORK_DUAL))
    {
        if ((comparedAddress.interfaceAddr.ipv6.s6_addr32[0]
            == DIFFSERV_UNUSED_FIELD_ADDRESS)&&
            (comparedAddress.interfaceAddr.ipv6.s6_addr32[1]
            == DIFFSERV_UNUSED_FIELD_ADDRESS) &&
            (comparedAddress.interfaceAddr.ipv6.s6_addr32[2]
            == DIFFSERV_UNUSED_FIELD_ADDRESS) &&
            (comparedAddress.interfaceAddr.ipv6.s6_addr32[3]
            == DIFFSERV_UNUSED_FIELD_ADDRESS))
        {
            ret = TRUE;
        }
    }
    return ret;
}

static
BOOL DIFFSERV_EqualAddress(Address comparedAddress1,
                           Address comparedAddress2)
{
    BOOL ret = FALSE;

    if (comparedAddress1.networkType == comparedAddress2.networkType)
    {
        if (comparedAddress1.networkType == NETWORK_IPV4)
        {
            if (comparedAddress1.interfaceAddr.ipv4
                == comparedAddress2.interfaceAddr.ipv4)
            {
                ret = TRUE;
            }
        }else if (comparedAddress1.networkType == NETWORK_IPV6)
        {
            if (SAME_ADDR6(comparedAddress1.interfaceAddr.ipv6,
                comparedAddress2.interfaceAddr.ipv6))
            {
                ret = TRUE;
            }
        }
    }
    return ret;
}
//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_AddIpMftcParameterInfo
// PURPOSE:     Adding the IpMftcParameter structure information to Ip Layer.
//
// PARAMETERS:  node - Pointer to the node,
//              srcNode - Address of the source node,
//              destNode - Address of destination node,
//              ds - the ds field,
//              protocolId - protocol for which the packet is meant,
//              srcPort - source port of the packet,
//              destPort - destination port of the application,
//              incomingInterface - interface id of the interface on which
//                                  the packet was received.
// RETURN:      none.
//--------------------------------------------------------------------------

static
void DIFFSERV_AddIpMftcParameterInfo(
    Node* node,
    Address srcNodeAddress,
    Address destNodeAddress,
    unsigned char ds,
    unsigned char protocolId,
    int srcPort,
    int destPort,
    int incomingInterface,
    int conditionClass)
{

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    int i = 0;

    // Check for this combination is previously exsit or not.
    for (i = 0; i < ip->numIpMftcParameters; i++)
    {
        if (((DIFFSERV_AddressIsUnused(ip->ipMftcParameter[i].srcAddress)) ||
            (DIFFSERV_AddressIsUnused(srcNodeAddress)) ||
            DIFFSERV_EqualAddress(
                ip->ipMftcParameter[i].srcAddress, srcNodeAddress)) &&

            ((DIFFSERV_AddressIsUnused(ip->ipMftcParameter[i].destAddress)) ||
            (DIFFSERV_AddressIsUnused(destNodeAddress)) ||
            DIFFSERV_EqualAddress(
                ip->ipMftcParameter[i].destAddress, destNodeAddress)) &&

            ((ip->ipMftcParameter[i].ds == DIFFSERV_UNUSED_FIELD) ||
            (ds == DIFFSERV_UNUSED_FIELD) ||
            (ip->ipMftcParameter[i].ds == ds)) &&

            ((ip->ipMftcParameter[i].protocolId == DIFFSERV_UNUSED_FIELD) ||
            (protocolId == DIFFSERV_UNUSED_FIELD) ||
            (ip->ipMftcParameter[i].protocolId == protocolId)) &&

            ((ip->ipMftcParameter[i].sourcePort == DIFFSERV_UNUSED_FIELD) ||
            (srcPort == DIFFSERV_UNUSED_FIELD) ||
            (ip->ipMftcParameter[i].sourcePort == srcPort)) &&

            ((ip->ipMftcParameter[i].destPort == DIFFSERV_UNUSED_FIELD) ||
            (destPort == DIFFSERV_UNUSED_FIELD) ||
            (ip->ipMftcParameter[i].destPort == destPort)) &&

            ((ip->ipMftcParameter[i].incomingInterface ==
            DIFFSERV_UNUSED_FIELD) ||
            (incomingInterface == DIFFSERV_UNUSED_FIELD) ||
            (ip->ipMftcParameter[i].incomingInterface ==
            incomingInterface)))
        {
                ERROR_ReportError("You have two traffic classes with "
                    "identical identifying characteristics");
        } //End if
    } //End for

    if (ip->numIpMftcParameters == 0 ||
        ip->numIpMftcParameters == ip->maxIpMftcParameters)
    {
        // Allocate the memory space for the structure IpMftcParameter.
        IpMftcParameter* newIpMftcParameter =
            (IpMftcParameter*)
            MEM_malloc(sizeof(IpMftcParameter) * (ip->maxIpMftcParameters
                + DIFFSERV_NUM_INITIAL_MFTC_PARA_ENTRIES));
        // Set the memory space to ZERO.
        memset(newIpMftcParameter,
               0,
               sizeof(IpMftcParameter) * ip->maxIpMftcParameters);
        ip->maxIpMftcParameters += DIFFSERV_NUM_INITIAL_MFTC_PARA_ENTRIES;

        if (ip->numIpMftcParameters != 0)
        {
            // Copy the old data of IpMftcParameter to newIpMftcParameter
            memcpy(newIpMftcParameter,
                   ip->ipMftcParameter,
                   sizeof(IpMftcParameter) * ip->numIpMftcParameters);
            MEM_free(ip->ipMftcParameter);
        } //End if
        ip->ipMftcParameter = newIpMftcParameter;
    } //End if

    // Initializing the class definition parameters.
    i = ip->numIpMftcParameters;
    ip->ipMftcParameter[i].srcAddress = srcNodeAddress;
    ip->ipMftcParameter[i].destAddress = destNodeAddress;
    ip->ipMftcParameter[i].ds = ds;
    ip->ipMftcParameter[i].protocolId = protocolId;
    ip->ipMftcParameter[i].sourcePort = srcPort;
    ip->ipMftcParameter[i].destPort = destPort;
    ip->ipMftcParameter[i].incomingInterface = incomingInterface;
    ip->ipMftcParameter[i].conditionClass = conditionClass;
    ip->numIpMftcParameters++;
}

static
void DIFFSERV_AddressSetUnused(Node * node, Address* address)
{

    if (node->networkData.networkProtocol == IPV4_ONLY)
    {
        address->networkType = NETWORK_IPV4;
    }
    else if (node->networkData.networkProtocol == IPV6_ONLY)
    {
        address->networkType = NETWORK_IPV6;
    }else if (node->networkData.networkProtocol == DUAL_IP)
    {
        address->networkType = NETWORK_DUAL;
    }
    address->interfaceAddr.ipv4 = DIFFSERV_UNUSED_FIELD_ADDRESS;
    address->interfaceAddr.ipv6.s6_addr32[0] = DIFFSERV_UNUSED_FIELD_ADDRESS;
    address->interfaceAddr.ipv6.s6_addr32[1] = DIFFSERV_UNUSED_FIELD_ADDRESS;
    address->interfaceAddr.ipv6.s6_addr32[2] = DIFFSERV_UNUSED_FIELD_ADDRESS;
    address->interfaceAddr.ipv6.s6_addr32[3] = DIFFSERV_UNUSED_FIELD_ADDRESS;
}
//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_MFTrafficConditionerParameterInit
// PURPOSE:     Parse the inputString for MultiFieldTrafficConditioner
//              Parameter Initialization.
// PARAMETERS:  node - Pointer to the node,
//              string - Information for the parameter "TRAFFIC-CLASSIFIER"
// RETURN:      None.
//--------------------------------------------------------------------------

static
void DIFFSERV_MFTrafficConditionerParameterInit(
    Node* node,
    char* inputString)
{
    char srcNodeString[MAX_ADDRESS_STRING_LENGTH];
    char destNodeString[MAX_ADDRESS_STRING_LENGTH];
    char dsString[MAX_ADDRESS_STRING_LENGTH];
    char protocolIdString[MAX_ADDRESS_STRING_LENGTH];
    char srcPortString[MAX_ADDRESS_STRING_LENGTH];
    char destPortString[MAX_ADDRESS_STRING_LENGTH];
    char incomingInterfaceString[MAX_ADDRESS_STRING_LENGTH];
    char conditionClassString[MAX_ADDRESS_STRING_LENGTH];

    NodeAddress   sourceNodeId = 0;
    NodeAddress   destNodeId = 0;
    Address sourceAddr; // = 0;
    Address destAddr; // = 0;
    unsigned char ds = 0;
    unsigned char protocolId = 0;
    int         srcPort = 0;
    int         destPort = 0;
    int           incomingInterface = 0;
    int           conditionClass = 0;
    int           items = 0;

    items = sscanf(inputString, "%*s %s %s %s %s %s %s %s %s",
                srcNodeString, destNodeString, dsString,
                protocolIdString, srcPortString, destPortString,
                incomingInterfaceString, conditionClassString);

    if (items != 8)
    {
        ERROR_ReportError("The Multi-Field Traffic Conditioner "
            "Class definition should be specified:\n"
            "\t<nodeId> <dest Node IP> <ds field> <protocolId> "
            "<srcPort> <destPort>\n"
            "\t<incoming interface> <conditionClass>\n"
            "\"ANY\" keyword can be substituted in any field");
    }

    //If any field is specified by the "ANY" keyword in the input file,
    //then the field is not used for checking the condition class.
    if (strcmp(srcNodeString, "ANY") == 0)
    {

        DIFFSERV_AddressSetUnused(node, &sourceAddr);
    }
    else
    {
        DIFFSERV_TrafficConditionerCheckForValidAddress(
            node,
            inputString,
            srcNodeString,
            &sourceNodeId,
            &sourceAddr);
    }

    if (strcmp(destNodeString, "ANY") == 0)
    {
        DIFFSERV_AddressSetUnused(node, &destAddr);
    }
    else
    {
        DIFFSERV_TrafficConditionerCheckForValidAddress(
            node,
            inputString,
            destNodeString,
            &destNodeId,
            &destAddr);
    }

    if (strcmp(dsString, "ANY") == 0)
    {
        ds = DIFFSERV_UNUSED_FIELD;
    }
    else
    {
        ds = (unsigned char)atoi(dsString);
        // checking for ds field.
        if ((atoi(dsString) > IPTOS_DSCP_MAX) ||
            (atoi(dsString) < IPTOS_DSCP_MIN + 1))
        {
            ERROR_ReportError(
                "The DS field must be a positive integer <= 63");
        }
    }

    if (strcmp(protocolIdString, "ANY") == 0)
    {
        protocolId = DIFFSERV_UNUSED_FIELD;
    }
    else
    {
        protocolId = (unsigned char)atoi(protocolIdString);
    }

    if (strcmp(srcPortString, "ANY") == 0)
    {
        srcPort = DIFFSERV_UNUSED_FIELD;
    }
    else
    {
        srcPort = atoi(srcPortString);
        ERROR_Assert((srcPort > 0) && (srcPort <= 65535),
            "srcPort must be a positive integer less than 65535.");
    }

    if (strcmp(destPortString, "ANY") == 0)
    {
        destPort = DIFFSERV_UNUSED_FIELD;
    }
    else
    {
        destPort = atoi(destPortString);
        ERROR_Assert((destPort > 0) && (destPort <= 65535),
            "destPort must be a positive integer less than 65535.");
    }

    if (sourceNodeId != node->nodeId)
    {
        if (strcmp(incomingInterfaceString, "ANY") == 0)
        {
            incomingInterface = DIFFSERV_UNUSED_FIELD;
        }
        else
        {
            incomingInterface = atoi(incomingInterfaceString);

            if (!(node->numberInterfaces >= incomingInterface))
            {
                ERROR_ReportError(
                    "The incoming interface ID is invalid");
            }
        }
    }
    else
    {
        incomingInterface = DIFFSERV_UNUSED_FIELD;
    }
    conditionClass = atoi(conditionClassString);
    ERROR_Assert(conditionClass >= 0,
        "conditionClass is a unique positive integer. Please change the "
        "conditionClass value in TRAFFIC-CLASSIFIER.");

#ifdef DIFFSERV_DEBUG_INIT
    {
        char srcAddressString[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&sourceAddr, srcAddressString);
        char addressString[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&destAddr, addressString);

        printf("Into node %u conditionClass:%u\n"
                "\tsourceNodeAddress:%s destNodeAddress:%s ds:%u protocolId:%u\n"
                "\t\tsrcPort:%u destPort:%u incomingInterface:%u\n",
                node->nodeId, conditionClass,
                srcAddressString, addressString, ds, protocolId,
                srcPort, destPort, incomingInterface);
    }
#endif

    DIFFSERV_AddIpMftcParameterInfo(
        node,
        sourceAddr,
        destAddr,
        ds,
        protocolId,
        srcPort,
        destPort,
        incomingInterface,
        conditionClass);
}

//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_ReturnConditionClassForPacket
// PURPOSE:     Returns condition class to which a particular packet belongs.
// PARAMETERS:  node - Pointer to the node,
//              ip - Pointer to the Ip Structure,
//              msg -  application packet or fragments.
//              incomingInterface - incoming interface for intermediate hops.
// RETURN:      condition class to which the packet belongs or FALSE.
//--------------------------------------------------------------------------

static
int DIFFSERV_ReturnConditionClassForPacket(
    Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int incomingInterface,
    BOOL* foundMatchingConditionClass)
{
    int i = 0;
    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    ip6_hdr* ipv6Header = NULL;
    unsigned char pktProtocolId;
    Address pktSourceAddress;
    Address pktDestAddress;
    unsigned char pktDSCode;
    unsigned short pktSourcePort;
    unsigned short pktDestPort;

    if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
    {
        pktProtocolId = ipHdr->ip_p;
        pktSourceAddress.networkType = NETWORK_IPV4;
        pktSourceAddress.interfaceAddr.ipv4 = ipHdr->ip_src;
        pktDestAddress.networkType = NETWORK_IPV4;
        pktDestAddress.interfaceAddr.ipv4 = ipHdr->ip_dst;
        pktDSCode = (unsigned char)
            (IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len) >> 2);
        pktSourcePort = DIFFSERV_ReturnIPv4SourcePort(msg);
        pktDestPort = DIFFSERV_ReturnIPv4DestPort(msg);
    }
    else
    {
        ipv6Header = (ip6_hdr* ) MESSAGE_ReturnPacket(msg);
        pktSourceAddress.networkType = NETWORK_IPV6;
        COPY_ADDR6(ipv6Header->ip6_src, pktSourceAddress.interfaceAddr.ipv6);
        pktDestAddress.networkType = NETWORK_IPV6;
        COPY_ADDR6(ipv6Header->ip6_dst, pktDestAddress.interfaceAddr.ipv6);
        pktDSCode =
            ip6_hdrGetClass(ipv6Header->ipv6HdrVcf) >> 2;
        pktProtocolId = DIFFSERV_ReturnIPv6ProtocolId(msg,
            &pktSourcePort, &pktDestPort);
    }

#ifdef DIFFSERV_DEBUG_MARKER
    {
        char srcAddressString[MAX_STRING_LENGTH];
        char dstAddressString[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&pktSourceAddress, srcAddressString);
        IO_ConvertIpAddressToString(&pktDestAddress, dstAddressString);

        printf("Into node %u Packet's \n"
            "\tSrcAdd:%s, DestAdd:%s, DSCode:%u, Protocol:%u, "
            "SrcPort:%u, DestPort:%u, Interface:%d\n",
            node->nodeId, srcAddressString, dstAddressString, pktDSCode,
            pktProtocolId, pktSourcePort, pktDestPort, incomingInterface);

    }
#endif


    for (i = 0; i < ip->numIpMftcParameters; i++)
    {
         if (((DIFFSERV_AddressIsUnused(ip->ipMftcParameter[i].srcAddress)) ||
            DIFFSERV_EqualAddress(
                ip->ipMftcParameter[i].srcAddress, pktSourceAddress)) &&

            (DIFFSERV_AddressIsUnused(ip->ipMftcParameter[i].destAddress) ||
            DIFFSERV_EqualAddress(
            ip->ipMftcParameter[i].destAddress, pktDestAddress)) &&

            ((ip->ipMftcParameter[i].ds == DIFFSERV_UNUSED_FIELD)
            || (ip->ipMftcParameter[i].ds == pktDSCode)) &&

            ((ip->ipMftcParameter[i].protocolId == DIFFSERV_UNUSED_FIELD)
            || (ip->ipMftcParameter[i].protocolId == pktProtocolId)) &&

            ((ip->ipMftcParameter[i].sourcePort == DIFFSERV_UNUSED_FIELD)
            || (ip->ipMftcParameter[i].sourcePort == pktSourcePort)) &&

            ((ip->ipMftcParameter[i].destPort == DIFFSERV_UNUSED_FIELD)
            || (ip->ipMftcParameter[i].destPort == pktDestPort)) &&

            ((node->nodeId == MAPPING_GetNodeIdFromInterfaceAddress(
                node, pktSourceAddress))
            || ((ip->ipMftcParameter[i].incomingInterface ==
                DIFFSERV_UNUSED_FIELD)
                || (ip->ipMftcParameter[i].incomingInterface ==
                incomingInterface))))
        {
            *foundMatchingConditionClass = TRUE;
            return ip->ipMftcParameter[i].conditionClass;
        }
    }
    return 0;
}


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_MFTrafficConditionerInit
// PURPOSE:     Initialize the Traffic Classifier/Conditioner
// PARAMETERS:  node - Pointer to the node,
//              nodeInput - configuration Information.
// RETURN:      None.
//--------------------------------------------------------------------------

void DIFFSERV_MFTrafficConditionerInit(
    Node* node,
    const NodeInput* nodeInput)
{
    NodeInput  mftcInput;
    BOOL retVal;
    int tmpCounter;
    NetworkDataIp* ip = (NetworkDataIp*)node->networkData.networkVar;
    int i;

    RANDOM_SetSeed(ip->trafficConditionerSeed,
                   node->globalSeed,
                   node->nodeId);

    IO_ReadCachedFile(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRAFFIC-CONDITIONER-FILE",
        &retVal,
        &mftcInput);

    if (retVal != TRUE)
    {
        ERROR_Assert(FALSE,
            "TRAFFIC-CONDITIONER-FILE: File is not present, but node"
            " is DS-capable edge router\n");
    }

    for (tmpCounter = 0; tmpCounter < mftcInput.numLines; tmpCounter++)
    {
        char identifier[MAX_STRING_LENGTH];

        sscanf(mftcInput.inputStrings[tmpCounter], "%s", identifier);

        if (strcmp(identifier, "TRAFFIC-CLASSIFIER") == 0)
        {
            DIFFSERV_MFTrafficConditionerParameterInit(
                node, mftcInput.inputStrings[tmpCounter]);
        }
        else if (strcmp(identifier, "TRAFFIC-METER") == 0)
        {
            int  items = 0;
            char conditionClassString[20];
            char meterTypeString[20];
            char committedRateString[20];
            char argument4[20];
            char argument5[20];
            char argument6[20];
            char argument7[20];

            items = sscanf(mftcInput.inputStrings[tmpCounter],
                        "%*s %s %s %s %s %s %s %s",
                        conditionClassString, meterTypeString,
                        committedRateString, argument4,
                        argument5, argument6, argument7);
            ERROR_Assert(atoi(conditionClassString) >= 0,
                "conditionClass is a unique positive integer. Please change "
                "the conditionClass in TRAFFIC-METER.");

            DIFFSERV_AddConditionClassToMeterMapping(
                node, atoi(conditionClassString));


            ip->mftcInfo[ip->numMftcInfo - 1].lastTokenUpdateTime =
                    node->getNodeTime();
            ip->mftcInfo[ip->numMftcInfo - 1].winLen =
                    DIFFSERV_TIME_SLIDING_WINDOW_LENGTH;

            if (strcmp(meterTypeString, "TokenBucket") == 0 && items == 4)
            {
                // Initialize the token bucket meter parameters.
                ip->mftcInfo[ip->numMftcInfo - 1].conditionClass =
                    atoi(conditionClassString);
                ip->mftcInfo[ip->numMftcInfo - 1].rate =
                    (atol(committedRateString) / 8);
                ERROR_Assert(ip->mftcInfo[ip->numMftcInfo - 1].rate > 0,
                    "In profile data rate should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].committedBurstSize =
                    atol(argument4);
                ERROR_Assert(
                    ip->mftcInfo[ip->numMftcInfo - 1].committedBurstSize > 0,
                    "committedBurstSize should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].meterFunction =
                    &DIFFSERV_PacketFitsTokenBucketProfile;
                //Initially bucket should be full of token.
                ip->mftcInfo[ip->numMftcInfo - 1].tokens =
                    ip->mftcInfo[ip->numMftcInfo - 1].committedBurstSize;

#ifdef DIFFSERV_DEBUG_INIT
                {
                    int i = ip->numMftcInfo - 1;
                    printf("Into node %u conditionClass:%u Meter,TB\n"
                        "\tCIR:%u CBS:%u\n",
                        node->nodeId, ip->mftcInfo[i].conditionClass,
                        ip->mftcInfo[i].rate,
                        ip->mftcInfo[i].committedBurstSize);
                }
#endif
            }
            else if (strcmp(meterTypeString, "srTCM") == 0 &&
                (items == 5 || items == 6))
            {
                // Initialize the srTCM parameters.
                ip->mftcInfo[ip->numMftcInfo - 1].conditionClass =
                    atoi(conditionClassString);
                ip->mftcInfo[ip->numMftcInfo - 1].rate =
                    (atol(committedRateString) / 8);
                ERROR_Assert(ip->mftcInfo[ip->numMftcInfo - 1].rate > 0,
                    "In profile data rate should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].committedBurstSize =
                    atol(argument4);
                ERROR_Assert(
                    ip->mftcInfo[ip->numMftcInfo - 1].committedBurstSize > 0,
                    "committedBurstSize should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].excessBurstSize =
                    atol(argument5);
                ERROR_Assert(
                    ip->mftcInfo[ip->numMftcInfo - 1].excessBurstSize > 0,
                    "excessBurstSize should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].meterFunction =
                    &DIFFSERV_PacketFitsSingleRateThreeColorProfile;
                //Initially two buckets should be full of token.
                ip->mftcInfo[ip->numMftcInfo - 1].tokens =
                    ip->mftcInfo[ip->numMftcInfo - 1].committedBurstSize;
                ip->mftcInfo[ip->numMftcInfo - 1].largeBucketTokens =
                    ip->mftcInfo[ip->numMftcInfo - 1].excessBurstSize;

                if (items == 6 && strcmp(argument6, "YES") == 0)
                {
                    ip->mftcInfo[ip->numMftcInfo - 1].colorAware = TRUE;
                }

#ifdef DIFFSERV_DEBUG_INIT
                {
                    int i = ip->numMftcInfo - 1;
                    printf("Into node %u conditionClass:%u Meter,srTCM\n"
                        "\tCIR:%u CBS:%u EBS:%u colorAware:%u\n",
                        node->nodeId, ip->mftcInfo[i].conditionClass,
                        ip->mftcInfo[i].rate,
                        ip->mftcInfo[i].committedBurstSize,
                        ip->mftcInfo[i].excessBurstSize,
                        ip->mftcInfo[i].colorAware);
                }
#endif
            }
            else if (strcmp(meterTypeString, "trTCM") == 0 &&
                (items == 6 || items == 7))
            {
                // Initialize the trTCM parameters.
                ip->mftcInfo[ip->numMftcInfo - 1].conditionClass =
                    atoi(conditionClassString);
                ip->mftcInfo[ip->numMftcInfo - 1].rate =
                    (atol(committedRateString) / 8);
                ERROR_Assert(ip->mftcInfo[ip->numMftcInfo - 1].rate > 0,
                    "In profile data rate should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].committedBurstSize =
                    atol(argument4);
                ERROR_Assert(
                    ip->mftcInfo[ip->numMftcInfo - 1].committedBurstSize > 0,
                    "committedBurstSize should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].peakInformationRate =
                    (atol(argument5) / 8);
                ERROR_Assert(
                    ip->mftcInfo[ip->numMftcInfo - 1].peakInformationRate > 0,
                    "peakInformationRate should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].excessBurstSize =
                    atol(argument6);
                ERROR_Assert(
                    ip->mftcInfo[ip->numMftcInfo - 1].excessBurstSize > 0,
                    "excessBurstSize should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].meterFunction =
                    &DIFFSERV_PacketFitsTwoRateThreeColorProfile;
                //Initially two buckets should be full of token.
                ip->mftcInfo[ip->numMftcInfo - 1].tokens =
                    ip->mftcInfo[ip->numMftcInfo - 1].committedBurstSize;
                ip->mftcInfo[ip->numMftcInfo - 1].largeBucketTokens =
                    ip->mftcInfo[ip->numMftcInfo - 1].excessBurstSize;

                if (items == 7 && strcmp(argument7, "YES") == 0)
                {
                    ip->mftcInfo[ip->numMftcInfo - 1].colorAware = TRUE;
                }
                else
                {
                    ip->mftcInfo[ip->numMftcInfo - 1].colorAware = FALSE;
                }

#ifdef DIFFSERV_DEBUG_INIT
                {
                    int i = ip->numMftcInfo - 1;
                    printf("Into node %u conditionClass:%u Meter,srTCM\n"
                        "\tCIR:%u PIR:%u CBS:%u EBS:%u\n",
                        node->nodeId, ip->mftcInfo[i].conditionClass,
                        ip->mftcInfo[i].rate,
                        ip->mftcInfo[i].peakInformationRate,
                        ip->mftcInfo[i].committedBurstSize,
                        ip->mftcInfo[i].excessBurstSize);
                }
#endif
            }
            else if (strcmp(meterTypeString, "TSW2CM") == 0 &&
                (items == 3 || items == 4))
            {
                // Initialize the TSW2CM parameters.
                ip->mftcInfo[ip->numMftcInfo - 1].conditionClass =
                    atoi(conditionClassString);
                ip->mftcInfo[ip->numMftcInfo - 1].rate =
                    (atol(committedRateString) / 8);
                ip->mftcInfo[ip->numMftcInfo - 1].avgRate =
                    ip->mftcInfo[ip->numMftcInfo - 1].rate;
                ip->mftcInfo[ip->numMftcInfo - 1].meterFunction =
                    &DIFFSERV_PacketFitsTSW2CMProfile;

                if (items == 4)
                {
                   ip->mftcInfo[ip->numMftcInfo - 1].winLen =
                       atof(argument4);
                }

#ifdef DIFFSERV_DEBUG_INIT
                {
                    int i = ip->numMftcInfo - 1;
                    printf("Into node %u conditionClass:%u Meter,TSW2CM\n"
                        "\tCIR:%u WinLen:%f\n",
                        node->nodeId, ip->mftcInfo[i].conditionClass,
                        ip->mftcInfo[i].rate,
                        ip->mftcInfo[i].winLen);
                }
#endif
            }
            else if (strcmp(meterTypeString, "TSW3CM") == 0 &&
                (items == 4 || items == 5))
            {
                // Initialize the TSW3CM parameters.
                ip->mftcInfo[ip->numMftcInfo - 1].conditionClass =
                    atoi(conditionClassString);
                ip->mftcInfo[ip->numMftcInfo - 1].rate =
                    (atol(committedRateString) / 8);
                ERROR_Assert(ip->mftcInfo[ip->numMftcInfo - 1].rate > 0,
                    "In profile data rate should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].avgRate =
                    ip->mftcInfo[ip->numMftcInfo - 1].rate;
                ip->mftcInfo[ip->numMftcInfo - 1].peakInformationRate =
                    (atol(argument4) / 8);
                ERROR_Assert(
                    ip->mftcInfo[ip->numMftcInfo - 1].peakInformationRate > 0,
                    "peakInformationRate should be greater than 0.");
                ip->mftcInfo[ip->numMftcInfo - 1].meterFunction =
                    &DIFFSERV_PacketFitsTSW3CMProfile;

                if (items == 5)
                {
                   ip->mftcInfo[ip->numMftcInfo - 1].winLen =
                       atof(argument5);
                   ERROR_Assert(
                        ip->mftcInfo[ip->numMftcInfo - 1].winLen > 0,
                        "winLen should be greater than 0.");
                }

#ifdef DIFFSERV_DEBUG_INIT
                {
                    int i = ip->numMftcInfo - 1;
                    printf("Into node %u conditionClass:%u Meter,TSW3CM\n"
                        "\tCIR:%u PIR:%u WinLen:%f\n",
                        node->nodeId, ip->mftcInfo[i].conditionClass,
                        ip->mftcInfo[i].rate,
                        ip->mftcInfo[i].peakInformationRate,
                        ip->mftcInfo[i].winLen);
                }
#endif
            }
            else
            {
                ERROR_Assert(FALSE, "TRAFFIC-METER: "
                  "Specification of Meter is not right way\n");
            }
        }
        else if (strcmp(identifier, "TRAFFIC-CONDITIONER") == 0)
        {
            // MARKING/DROPPING Routines.
            int   conditionClass = 0;
            char  outProfileAction[20];
            BOOL  dropOutProfilePackets = FALSE;
            int   dscpToMark = 0;
            int   items = 0;

            items = sscanf(mftcInput.inputStrings[tmpCounter],
                        "%*s %d %s %d",
                        &conditionClass,
                        outProfileAction,
                        &dscpToMark);

            if (strcmp(outProfileAction, "MARK") == 0)
            {
                dropOutProfilePackets = FALSE;

                if (items != 3)
                {
                    ERROR_ReportError("\"TRAFFIC-CONDITIONER\" "
                        "\"MARK\" requires dscp to be specified");
                }
                else if (dscpToMark < IPTOS_DSCP_MIN + 1 ||
                    dscpToMark > IPTOS_DSCP_MAX)
                {
                     ERROR_ReportError("\"TRAFFIC-CONDITIONER\" "
                        "\"MARK\" requires positive dscp <= 63");
                }
            }
            else if (strcmp(outProfileAction, "DROP") == 0)
            {
                dropOutProfilePackets = TRUE;

                if (items != 2)
                {
                    ERROR_ReportError("\"TRAFFIC-CONDITIONER\" "
                        "\"DROP\" takes no parameters");
                }
            }
            else
            {
                ERROR_ReportError("\"TRAFFIC-CONDITIONER\" "
                    "must be either \"MARK\" or \"DROP\"");
            }

            DIFFSERV_AddConditionClassToOutProfileMapping(
                node,
                conditionClass,
                dropOutProfilePackets,
                dscpToMark);
        }
        else if (strcmp(identifier, "TRAFFIC-SERVICE") == 0)
        {
            int   conditionClass = 0;
            char  serviceClassString[20];
            int   items = 0;
            IpMultiFieldTrafficConditionerInfo* conditionClassInfo = NULL;

            items = sscanf(mftcInput.inputStrings[tmpCounter],
                        "%*s %d %s",
                        &conditionClass,
                        serviceClassString);

            conditionClassInfo =
                DIFFSERV_ReturnConditionClass(ip, conditionClass);

            if (!conditionClassInfo)
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr,
                    "There is no entry for the condition class %d\n"
                    "Cannot specify an Service Class for nonexistent "
                    "class",
                    conditionClass);
                ERROR_ReportError(errorStr);
            }

            if (strcmp(serviceClassString, "EF") == 0)
            {
                conditionClassInfo->serviceClass = DIFFSERV_DS_CLASS_EF;
            }
            else if (strcmp(serviceClassString, "AF1") == 0)
            {
                conditionClassInfo->serviceClass = DIFFSERV_DS_CLASS_AF11;
            }
            else if (strcmp(serviceClassString, "AF2") == 0)
            {
                conditionClassInfo->serviceClass = DIFFSERV_DS_CLASS_AF21;
            }
            else if (strcmp(serviceClassString, "AF3") == 0)
            {
                conditionClassInfo->serviceClass = DIFFSERV_DS_CLASS_AF31;
            }
            else if (strcmp(serviceClassString, "AF4") == 0)
            {
                conditionClassInfo->serviceClass = DIFFSERV_DS_CLASS_AF41;
            }
            else
            {
                ERROR_ReportError("The Service Class is invalid. "
                    "It should be one of these: EF, AF1, AF2, AF3 or AF4");
            }
        }
    } // for

    i = 0;

    while (i < ip->numMftcInfo)
    {
        if (!ip->mftcInfo[i].serviceClass)
        {
            char errorMsg[MAX_STRING_LENGTH];
            sprintf(errorMsg, "Agreement is incomplete, Condition Class %u "
                "has no Service Class!\n", ip->mftcInfo[i].conditionClass);
            ERROR_ReportError(errorMsg);
        }
        i++;
    }
}


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_TrafficConditionerProfilePacketAndMarkOrDrop
// PURPOSE :    Analyze outgoing packet to determine whether or not it
//              is in/out-profile, and either mark or drop the packet
//              as indicated by the class of traffic and policy defined
//              in the configuration file.
// PARAMETERS:  node - pointer to the node,
//              ip - pointer to the IP state variables
//              msg - the outgoing packet
//              interfaceIndex - the outgoing interface
//              packetWasDropped - pointer to a BOOL which is set to
//                   TRUE if the Traffic Conditioner drops the packet,
//                   FALSE otherwise (even if the packet was marked)
// RETURN:      None.
//--------------------------------------------------------------------------

void DIFFSERV_TrafficConditionerProfilePacketAndMarkOrDrop(
    Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int interfaceIndex,
    BOOL* packetWasDropped)
{
    IpMultiFieldTrafficConditionerInfo* conditionClassInfo = NULL;
    BOOL foundMatchingConditionClass = FALSE;
    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    ip6_hdr *ipv6_hdr = NULL;
    TosType tos;
    BOOL isConforming = FALSE;
    int preference = 0;
    int conditionClass = DIFFSERV_ReturnConditionClassForPacket(
                        node,
                        ip,
                        msg,
                        interfaceIndex,
                        &foundMatchingConditionClass);

    conditionClassInfo = DIFFSERV_ReturnConditionClass(ip, conditionClass);

#ifdef DIFFSERV_DEBUG_MARKER
    {
        printf("\tReturn ConditionClass:%u\n", conditionClass);
    }
#endif
    // Check the emptiness of the interface's output queue(s) before
    // attempting to queue packet.
    *packetWasDropped = FALSE;
    if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
    {
        tos = IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len);
    }else {
        ipv6_hdr = (ip6_hdr*) MESSAGE_ReturnPacket(msg);
        tos = ip6_hdrGetClass(ipv6_hdr->ipv6HdrVcf) >> 2;
    }

    if ((tos == IPTOS_PREC_INTERNETCONTROL) || (tos == IPTOS_PREC_NETCONTROL))
    {
        // Packet with precedence 6 | 7
        return;
    }

    if (foundMatchingConditionClass && conditionClassInfo)
    {
        conditionClassInfo->numPacketsIncoming++;

        isConforming = conditionClassInfo->meterFunction(
                            node,
                            ip,
                            msg,
                            conditionClassInfo->conditionClass,
                            &preference);

        // Conforming means conforming or partial conforming packet.
        if (isConforming)
        {
            // Call the Marker for marking the packet
            DIFFSERV_Marker(preference, conditionClassInfo->serviceClass, msg);

#ifdef DIFFSERV_DEBUG_MARKER
            {
                printf("\t\tPacket is In-Profile and can be transmitted\n");
            }
#endif
        }
        else
        {

#ifdef DIFFSERV_DEBUG_MARKER
            {
                printf("\t\tPacket is Out-Of-Profile Action :%s\n",
                    (conditionClassInfo->dropOutProfilePackets == TRUE ?
                    "DROP" : "MARK"));
            }
#endif
            // Action for non-conforming packets
            // Drop, if specified
            // Default, as indicated by meter condition
            // Remark, if user specified

            if (conditionClassInfo->dropOutProfilePackets == TRUE)
            {
                *packetWasDropped = TRUE;
            }
            else if (conditionClassInfo->dsToMarkOutProfilePackets == 0)
            {
                DIFFSERV_Marker(preference, conditionClassInfo->serviceClass, msg);
            }
            else
            {
                // Changed to use 6 bit DS field
                if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
                {
                    IpHeaderSetTOS(&(ipHdr->ip_v_hl_tos_len),
                        DIFFSERV_SET_TOS_FIELD(
                        (conditionClassInfo->dsToMarkOutProfilePackets),
                        IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len)));
                }else {
                    ip6_hdrSetClass(&(ipv6_hdr->ipv6HdrVcf),
                        DIFFSERV_SET_TOS_FIELD(
                            conditionClassInfo->dsToMarkOutProfilePackets,
                        ip6_hdrGetClass(ipv6_hdr->ipv6HdrVcf)));

                }
            } //End if
        } //End if
    } //End if
    else
    {
        if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
        {
            IpHeaderSetTOS(&(ipHdr->ip_v_hl_tos_len),
                DIFFSERV_SET_TOS_FIELD((DIFFSERV_DS_CLASS_BE),
                IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len)));
        }else {
            ip6_hdrSetClass(&(ipv6_hdr->ipv6HdrVcf), DIFFSERV_SET_TOS_FIELD(
                                      DIFFSERV_DS_CLASS_BE,
                ip6_hdrGetClass(ipv6_hdr->ipv6HdrVcf)));
        }
    }

#ifdef DIFFSERV_DEBUG_MARKER
    {
        if (IpHeaderGetVersion(ipHdr->ip_v_hl_tos_len) == IPVERSION4)
        {
            tos = IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len);
        }else {
            ip6_hdr *ipv6_hdr = (ip6_hdr*) MESSAGE_ReturnPacket(msg);
            tos = ip6_hdrGetClass(ipv6_hdr->ipv6HdrVcf) ;
        }
        printf("\tAfter Marking, in node %u the value of DS Field=%u\n",
            node->nodeId, (tos >> 2));
    }
#endif
}


//--------------------------------------------------------------------------
// FUNCTION    DIFFSERV_MFTrafficConditionerFinalize(Node* node)
// PURPOSE     Print out Diffserv statistics
// PARAMETERS: node,    Diffserv statistics of this node
//--------------------------------------------------------------------------

void DIFFSERV_MFTrafficConditionerFinalize(Node* node)
{
    char buf[MAX_STRING_LENGTH];
    NetworkDataIp* ipLayer = (NetworkDataIp*)node->networkData.networkVar;

    const char* app;
    app =(char *) "Diffserv";
    if (ipLayer->mftcStatisticsEnabled == TRUE)
    {
        int i = 0;
        IpMultiFieldTrafficConditionerInfo* conditionClassInfo = NULL;

        while (i < ipLayer->numMftcInfo)
        {
            conditionClassInfo = &(ipLayer->mftcInfo[i]);

            sprintf(buf, "Incoming Packets = %u",
                conditionClassInfo->numPacketsIncoming);
            IO_PrintStat(
                node,
                "Network",
                app,
                ANY_DEST,
                conditionClassInfo->conditionClass,
                buf);

            sprintf(buf, "Conforming Packets = %u",
                conditionClassInfo->numPacketsConforming);
            IO_PrintStat(
                node,
                "Network",
                app,
                ANY_DEST,
                conditionClassInfo->conditionClass,
                buf);

            if (conditionClassInfo->excessBurstSize > 0 ||
                conditionClassInfo->peakInformationRate > 0)
            {
                sprintf(buf, "Partially Conforming Packets = %u",
                    conditionClassInfo->numPacketsPartConforming);
                IO_PrintStat(
                    node,
                    "Network",
                    app,
                    ANY_DEST,
                    conditionClassInfo->conditionClass,
                    buf);
            }

            if (conditionClassInfo->dropOutProfilePackets == TRUE)
            {
                sprintf(buf, "Dropped Packets = %u",
                    conditionClassInfo->numPacketsNonConforming);
                IO_PrintStat(
                    node,
                    "Network",
                    app,
                    ANY_DEST,
                    conditionClassInfo->conditionClass,
                    buf);
            }
            else
            if ((conditionClassInfo->dropOutProfilePackets == FALSE) &&
                (conditionClassInfo->dsToMarkOutProfilePackets > 0))
            {
                sprintf(buf, "Remarked Packets = %u",
                    conditionClassInfo->numPacketsNonConforming);
                IO_PrintStat(
                    node,
                    "Network",
                    app,
                    ANY_DEST,
                    conditionClassInfo->conditionClass,
                    buf);
            }
            else
            {
                sprintf(buf, "Non-Conforming Packets = %u",
                    conditionClassInfo->numPacketsNonConforming);
                IO_PrintStat(
                    node,
                    "Network",
                    app,
                    ANY_DEST,
                    conditionClassInfo->conditionClass,
                    buf);
            } //End if
            i++;
        } //End while
    } //End if
}
