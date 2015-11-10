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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "network_ip.h"
#include "transport_udp.h"
#include "transport_tcp_hdr.h"
#include "app_cbr.h"
#include "mac.h"
#include "mac_arp.h"
#include "app_util.h"
#include "ipv6.h"

#include "mac_dot16.h"
#include "mac_dot16_bs.h"
#include "mac_dot16_ss.h"
#include "mac_dot16_cs.h"
#include "mac_dot16_qos.h"
#include "mac_dot16_tc.h"

#define DEBUG              0
#define DEBUG_TIMER        0
#define DEBUG_PACKET       0
#define DEBUG_FLOW         0
#define DEBUG_WARNING      0

// /**
// FUNCTION   :: MacDot16CsPrintStats
// LAYER      :: MAC
// PURPOSE    :: Print out statistics
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16CsPrintStats(
        Node* node,
        MacDataDot16* dot16,
        int interfaceIndex)
{
    MacDot16Cs* dot16Cs = (MacDot16Cs*) dot16->csData;
    char buf[MAX_STRING_LENGTH];


    // print out # of data packets from upper layer
    sprintf(buf, "Number of data packets to classify = %d",
            dot16Cs->stats.numPktsFromUpper);

    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of data packets txed in MAC frames
    sprintf(buf, "Number of data packets classified = %d",
            dot16Cs->stats.numPktsClassified);

    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
/* Disabled as it is always 0 right now.
    // print out # of data packets txed in MAC frames
    sprintf(buf, "Number of data packets classifier dropped = %d",
            dot16Cs->stats.numPktsDropped);

    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
*/

    // print out # of data packets txed in MAC frames
    sprintf(buf, "Number of classifiers = %d",
            dot16Cs->csfId);

    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

}


//////////////////////////////////////////////////////////////////////
//
//  Start of Common Clasifier functions
//
//////////////////////////////////////////////////////////////////////

// /**
// FUNCTION   :: MacDot16CsInvalidClassifier
// LAYER      :: MAC
// PURPOSE    :: Invalid a classifier by the classifier ID
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + csfId     : unsigned int  : ID of the classifier
// RETURN     :: void : NULL
// **/
void MacDot16CsInvalidClassifier(
        Node* node,
        MacDataDot16* dot16,
        int csfId)
{
    MacDot16Cs* dot16Cs = (MacDot16Cs*) dot16->csData;
    clocktype currentTime;
    int i;

    currentTime = node->getNodeTime();

    // classifiers
    for (i = 0; i < dot16Cs->numClassifierRecord; i++)
    {
        if (dot16Cs->classifierRecord[i]->csfId == csfId)
        {
            // make sure classifier is not in use
            if (-- dot16Cs->classifierRecord[i]->refCount > 0)
                return;

            // found it, using initial ranging cid to indicate an
            // invalid classifier
            dot16Cs->classifierRecord[i]->basicCid =
                                        DOT16_INITIAL_RANGING_CID;
            dot16Cs->classifierRecord[i]->invalidTime = currentTime;

            // set a timer to remove the invalid classifier after a delay
            MacDot16SetTimer(node,
                             dot16,
                             DOT16_TIMER_CS_ClassifierTimeout,
                             DOT16_CS_CLASSIFIER_REMOVAL_DELAY,
                             NULL,
                             0);

            return;
        }
    }
}

// /**
// FUNCTION   :: MacDot16CsClassifierAddRecord
// LAYER      :: MAC
// PURPOSE    :: Add a new classifier record
// PARAMETERS ::
// + node           : Node*         : Pointer to node.
// + dot16          : MacDataDot16* : Pointer to DOT16 structure
// + serviceType    : MacDot16ServiceType   : Data service type of this flow
// + classifierInfo : MacDot16CsClassifier* : classifer info of this flow
// RETURN    :: MacDot16CsClassifierRecord* : Pointer to the added record
// **/
static
MacDot16CsClassifierRecord* MacDot16CsClassifierAddRecord(
        Node* node,
        MacDataDot16* dot16,
        MacDot16ServiceType serviceType,
        MacDot16CsClassifier* classifierInfo)
{
    MacDot16Cs* dot16Cs = (MacDot16Cs*) dot16->csData;
    MacDot16CsClassifierRecord* recordPtr = NULL;
    MacDot16CsClassifier* classifier = NULL;

    if (DEBUG)
    {
        char addrStr[MAX_STRING_LENGTH];
        printf("Node%d CS is adding a new classifier record.\n",
               node->nodeId);
        printf("    service type = %d\n", serviceType);
        IO_ConvertIpAddressToString(&(classifierInfo->srcAddr), addrStr);
        printf("    source address = %s\n", addrStr);
        IO_ConvertIpAddressToString(&(classifierInfo->dstAddr), addrStr);
        printf("    destination address = %s\n", addrStr);
        printf("    source port = %d\n", classifierInfo->srcPort);
        printf("    destination port = %d\n", classifierInfo->dstPort);
    }

    // construct the classifier record
    recordPtr = (MacDot16CsClassifierRecord*)
                MEM_malloc(sizeof(MacDot16CsClassifierRecord));
    memset(recordPtr, 0, sizeof(MacDot16CsClassifierRecord));

    ERROR_Assert(recordPtr != NULL, "MAC 802.16: Out of memory!");

    if (classifierInfo->srcAddr.networkType == NETWORK_IPV4)
    {
        recordPtr->type = DOT16_CS_CLASSIFIER_IPv4;
    }
    else if (classifierInfo->srcAddr.networkType == NETWORK_IPV6)
    {
        recordPtr->type = DOT16_CS_CLASSIFIER_IPv6;
    }
    else // for future ATM
    {
        ERROR_ReportError("MAC802.16: ATM is not supported!");
    }

    recordPtr->csfId = dot16Cs->csfId ++;
    recordPtr->serviceType = serviceType;
    recordPtr->refCount++;

    classifier = (MacDot16CsClassifier*)
                 MEM_malloc(sizeof(MacDot16CsClassifier));
    ERROR_Assert(classifier != NULL, "MAC 802.16: Out of memory!");
    memset(classifier, 0, sizeof(MacDot16CsClassifier));

    memcpy((char*) classifier,
           (char*) classifierInfo,
           sizeof(MacDot16CsClassifier));

    recordPtr->classifier = (void*) classifier;

    // add into the record array
    if (dot16Cs->numClassifierRecord == dot16Cs->allocClassifierRecord)
    { // no more space to hold new record, expand it
        MacDot16CsClassifierRecord** newPtr;

        dot16Cs->allocClassifierRecord += DOT16_CS_NUM_RECORD_INC;

        newPtr = (MacDot16CsClassifierRecord**)
                 MEM_malloc(sizeof(MacDot16CsClassifierRecord*) *
                            dot16Cs->allocClassifierRecord);
        ERROR_Assert(newPtr != NULL, "MAC 802.16: Out of memory!");

        // copy over existing records
        memcpy((char*) newPtr,
               (char*) dot16Cs->classifierRecord,
               dot16Cs->numClassifierRecord *
               sizeof(MacDot16CsClassifierRecord*));

        // free old memory
        MEM_free(dot16Cs->classifierRecord);

        dot16Cs->classifierRecord = newPtr;
    }

    dot16Cs->classifierRecord[dot16Cs->numClassifierRecord] = recordPtr;
    dot16Cs->numClassifierRecord ++;

    return recordPtr;
}

// /**
// FUNCTION   :: MacDot16CsClassifierRemoveInvalidRecord
// LAYER      :: MAC
// PURPOSE    :: Remove invalidated classifier record.
//               Removed record will be freed
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16CsClassifierRemoveInvalidRecord(
        Node* node,
        MacDataDot16* dot16)
{
    MacDot16Cs* dot16Cs = (MacDot16Cs*) dot16->csData;
    MacDot16CsClassifierRecord* recordPtr = NULL;
    clocktype currentTime;
    int i;

    if (DEBUG)
    {
        printf("Node%d CS is removing invalid classifier record.\n",
               node->nodeId);
    }

    currentTime = node->getNodeTime();
    i = 0;
    while (i < dot16Cs->numClassifierRecord)
    {
        recordPtr = dot16Cs->classifierRecord[i];

        if (recordPtr->basicCid < DOT16_BASIC_CID_START &&
            recordPtr->invalidTime + DOT16_CS_CLASSIFIER_REMOVAL_DELAY <=
            currentTime)
        {
            // need to remove this one
            if (i < (dot16Cs->numClassifierRecord - 1))
            {
                // not the last one
                memmove((char*) &(dot16Cs->classifierRecord[i]),
                        (char*) &(dot16Cs->classifierRecord[i + 1]),
                        (dot16Cs->numClassifierRecord - i - 1) *
                        sizeof(MacDot16CsClassifierRecord*));
            }

            if (recordPtr->classifier != NULL)
            {
                MEM_free(recordPtr->classifier);
            }
            MEM_free(recordPtr);
            recordPtr = NULL;

            dot16Cs->numClassifierRecord --;
        }
        else
        {
            i ++;
        }
    }

    return;
}

// /**
// FUNCTION   :: MacDot16CsBuildIPv4FragmentClassifierInfo
// LAYER      :: MAC
// PURPOSE    :: Builds the fragment classifier for IPv4 fragmented packets
//               which in turn will be used to fill the classifier 
//               info structure
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + ipHeader  : IpHeaderType* : Pointer to IPv4 header
// + payload   : char**        : Packet payload
// + ipFragmentClassifierValue : MacDot16CsIPFragmentClassifierValue*    :
//                      Pointer to Classifier value for this fragment
// RETURN     :: void : NULL
// **/
static
void MacDot16CsBuildIPv4FragmentClassifierInfo(
          Node* node,
          MacDataDot16* dot16,
          IpHeaderType* ipHeader,
          char** payload,
          MacDot16CsIPFragmentClassifierValue* ipFragmentClassifierValue)
{
    MacDot16Cs* dot16Cs = (MacDot16Cs*) dot16->csData;
    MacDot16CsIPFragmentClassifierKey mapKey;
    MacDot16CsIPFragmentClassifierValue mapValue;
    // if first fragment build a fragment classifier
    UInt16 msgFragOff;
    msgFragOff = IpHeaderGetIpFragOffset(ipHeader->ipFragment);
    if (msgFragOff == 0)
    {
        // Check the size of Map, if it is equal to max map size then
        // delete the stale clssifier entries (if exists)
        if (dot16Cs->ipFragmentClassifierMap->size() == 
                DOT16_FRAGMENT_CLASSIFIER_MAP_MAX_SIZE)
        {
            // Traverse the map and remove stale fragment clssifiers
            MacDot16CsIpFragmentClassifierMap::iterator it;
            for (it = dot16Cs->ipFragmentClassifierMap->begin();
                 it != dot16Cs->ipFragmentClassifierMap->end();
                 it++)
            {
                memcpy(&mapValue, &it->second,
                   sizeof(MacDot16CsIPFragmentClassifierValue));
                if (node->getNodeTime() >= mapValue.fragmentCalssifierHoldTime)
                {
                    dot16Cs->ipFragmentClassifierMap->erase(it);
                }
            }

            // Check if some space is created for new fragment clssifier 
            if (dot16Cs->ipFragmentClassifierMap->size() ==
                DOT16_FRAGMENT_CLASSIFIER_MAP_MAX_SIZE)
            {
                // No more element can be inserted in map treat the fragment
                // with srcPort and destination port as 0 and put it in
                // different queue
                ipFragmentClassifierValue->srcPort = 0;
                ipFragmentClassifierValue->dstPort = 0;
                return;
            }
        }
        // Create the key and value pair to insert new element in Map
        mapKey.ip_id = ipHeader->ip_id;
        mapKey.ip_p = ipHeader->ip_p;
        SetIPv4AddressInfo(&mapKey.dstAddr, ipHeader->ip_dst);
        SetIPv4AddressInfo(&mapKey.srcAddr, ipHeader->ip_src);
        if (ipHeader->ip_p == IPPROTO_UDP)
        {
            // this is a UDP packet
            TransportUdpHeader* udpHdr;

            udpHdr = (TransportUdpHeader*) *payload;
            *payload += sizeof(TransportUdpHeader);
            mapValue.srcPort = udpHdr->sourcePort;
            mapValue.dstPort = udpHdr->destPort;
        }
        else if (ipHeader->ip_p == IPPROTO_TCP)
        {
            // this is a TCP packet
            struct tcphdr* tcpHdr;

            tcpHdr = (struct tcphdr*) *payload;
            *payload += sizeof(struct tcphdr);

            mapValue.srcPort = tcpHdr->th_sport;
            mapValue.dstPort = tcpHdr->th_dport;
        }

        // Populate the fragment classifier hold time
        NetworkDataIp* ip = 
            (NetworkDataIp*) node->networkData.networkVar;
        clocktype delay = ip->ipFragHoldTime +
                          DOT16_FRAGMENT_HOLD_TIME_INTERVAL;
        mapValue.fragmentCalssifierHoldTime = node->getNodeTime() + delay;

        dot16Cs->ipFragmentClassifierMap->insert(
                pair<MacDot16CsIPFragmentClassifierKey,
                    MacDot16CsIPFragmentClassifierValue>
                   (mapKey, mapValue));

        ipFragmentClassifierValue->srcPort = mapValue.srcPort;
        ipFragmentClassifierValue->dstPort = mapValue.dstPort;
    }
    else
    {
        // Not the first fragment.
        // Form the key to find
        mapKey.ip_id = ipHeader->ip_id;
        mapKey.ip_p = ipHeader->ip_p;
        SetIPv4AddressInfo(&mapKey.dstAddr, ipHeader->ip_dst);
        SetIPv4AddressInfo(&mapKey.srcAddr, ipHeader->ip_src);

        MacDot16CsIpFragmentClassifierMap::iterator it;
        it = dot16Cs->ipFragmentClassifierMap->find(mapKey);
        if (it == dot16Cs->ipFragmentClassifierMap->end())
        {
            // Fragment classifier not found, means first fragment not
            // received then treat the fragment with
            // srcPort and destination port as 0 and put it in different
            // queue
            ipFragmentClassifierValue->srcPort = 0;
            ipFragmentClassifierValue->dstPort = 0;
        }
        else
        {
            memcpy(&mapValue, &it->second,
                   sizeof(MacDot16CsIPFragmentClassifierValue));

            // Update the fragmentCalssifierHoldTime as new fragment
            // received
            NetworkDataIp* ip = 
                (NetworkDataIp*) node->networkData.networkVar;
            clocktype delay = ip->ipFragHoldTime +
                          DOT16_FRAGMENT_HOLD_TIME_INTERVAL;
            (*it).second.fragmentCalssifierHoldTime =
                node->getNodeTime() + delay;

            Int32 moreFrag = IpHeaderGetIpMoreFrag(ipHeader->ipFragment);
            if (moreFrag == 0)
            {
                // last fragment  delete the ipFragmentClassifier
                dot16Cs->ipFragmentClassifierMap->erase(it);
            }
            ipFragmentClassifierValue->srcPort = mapValue.srcPort;
            ipFragmentClassifierValue->dstPort = mapValue.dstPort;
       }
    }
}

// /**
// FUNCTION   :: MacDot16CsBuildIPv6FragmentClassifierInfo
// LAYER      :: MAC
// PURPOSE    :: Builds the fragment classifier for IPv6 fragmented packets
//               which in turn will be used to fill the classifier 
//               info structure
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + ipv6Header: ip6_hdr*      : Pointer to IPv6 header
// + payload   : char**        : Packet payload
// + ipFragmentClassifierValue : MacDot16CsIPFragmentClassifierValue*    :
//                      Pointer to Classifier value for this fragment
// RETURN     :: void : NULL
// **/
static
void MacDot16CsBuildIPv6FragmentClassifierInfo(
          Node* node,
          MacDataDot16* dot16,
          ip6_hdr* ipv6Header,
          char** payload,
          MacDot16CsIPFragmentClassifierValue* ipFragmentClassifierValue)
{
    MacDot16Cs* dot16Cs = (MacDot16Cs*) dot16->csData;
    MacDot16CsIPFragmentClassifierKey mapKey;
    MacDot16CsIPFragmentClassifierValue mapValue;
    ipv6_fraghdr* frgp = NULL;
    // if first fragment build a fragment classifier
    frgp = (ipv6_fraghdr*) *payload;
    UInt16 msgFragOff;
    msgFragOff = frgp->if6_off;
    if (msgFragOff == 1)
    {
        // Check the size of Map, if it is equal to max map size then
        // delete the stale clssifier entries (if exists)
        if (dot16Cs->ipFragmentClassifierMap->size() == 
            DOT16_FRAGMENT_CLASSIFIER_MAP_MAX_SIZE)
        {
           // Traverse the map and remove stale fragment clssifiers
            MacDot16CsIpFragmentClassifierMap::iterator it;
            for (it = dot16Cs->ipFragmentClassifierMap->begin();
                 it != dot16Cs->ipFragmentClassifierMap->end();
                 it++)
            {
                memcpy(&mapValue, &it->second,
                   sizeof(MacDot16CsIPFragmentClassifierValue));
                if (node->getNodeTime() >= mapValue.fragmentCalssifierHoldTime)
                {
                    dot16Cs->ipFragmentClassifierMap->erase(it);
                }
            }

            // Check if some space is created for new fragment clssifier 
            if (dot16Cs->ipFragmentClassifierMap->size() ==
                DOT16_FRAGMENT_CLASSIFIER_MAP_MAX_SIZE)
            {
                // No more element can be inserted in map treat the fragment
                // with srcPort and destination port as 0 and put it in
                // different queue
                ipFragmentClassifierValue->srcPort = 0;
                ipFragmentClassifierValue->dstPort = 0;
                return;
            }
        }

        // Create the key and value pair to insert new element in Map
        mapKey.ip_id = frgp->if6_id;
        mapKey.ip_p = frgp->if6_nh;
        SetIPv6AddressInfo(&mapKey.dstAddr, ipv6Header->ip6_dst);
        SetIPv6AddressInfo(&mapKey.srcAddr, ipv6Header->ip6_src);

        *payload += sizeof(ipv6_fraghdr);
        if (frgp->if6_nh == IPPROTO_UDP)
        {
            // this is a UDP packet
            TransportUdpHeader* udpHdr;

            udpHdr = (TransportUdpHeader*) *payload;
            *payload += sizeof(TransportUdpHeader);
            mapValue.srcPort = udpHdr->sourcePort;
            mapValue.dstPort = udpHdr->destPort;
        }
        else if (frgp->if6_nh == IPPROTO_TCP)
        {
            // this is a TCP packet
            struct tcphdr* tcpHdr;

            tcpHdr = (struct tcphdr*) *payload;
            *payload += sizeof(struct tcphdr);

            mapValue.srcPort = tcpHdr->th_sport;
            mapValue.dstPort = tcpHdr->th_dport;
        }

        // Populate the fragment classifier hold time
        NetworkDataIp* ip = 
            (NetworkDataIp*) node->networkData.networkVar;
        IPv6Data* ipv6 = ip->ipv6;
        clocktype delay = ipv6->ipFragHoldTime +
                    DOT16_FRAGMENT_HOLD_TIME_INTERVAL;
        mapValue.fragmentCalssifierHoldTime = node->getNodeTime() + delay;

        dot16Cs->ipFragmentClassifierMap->insert(
                pair<MacDot16CsIPFragmentClassifierKey,
                     MacDot16CsIPFragmentClassifierValue>
                    (mapKey, mapValue));

        ipFragmentClassifierValue->srcPort = mapValue.srcPort;
        ipFragmentClassifierValue->dstPort = mapValue.dstPort;
    }
    else
    {
        // Not the first fragment.
        // Form the key to find
        mapKey.ip_id = frgp->if6_id;
        mapKey.ip_p = frgp->if6_nh;
        SetIPv6AddressInfo(&mapKey.dstAddr, ipv6Header->ip6_dst);
        SetIPv6AddressInfo(&mapKey.srcAddr, ipv6Header->ip6_src);

        MacDot16CsIpFragmentClassifierMap::iterator it;
        it = dot16Cs->ipFragmentClassifierMap->find(mapKey);
        if (it == dot16Cs->ipFragmentClassifierMap->end())
        {
            // Fragment classifier not found, means first fragment not
            // received then treat the fragment with
            // srcPort and destination port as 0 and put it in different
            // queue
            ipFragmentClassifierValue->srcPort = 0;
            ipFragmentClassifierValue->dstPort = 0;
        }
        else
        {
            memcpy(&mapValue, &it->second,
                   sizeof(MacDot16CsIPFragmentClassifierValue));

            // Update the fragmentCalssifierHoldTime as new fragment
            // received
            NetworkDataIp* ip = 
                (NetworkDataIp*) node->networkData.networkVar;
            IPv6Data* ipv6 = ip->ipv6;
            clocktype delay = ipv6->ipFragHoldTime +
                           DOT16_FRAGMENT_HOLD_TIME_INTERVAL;
            (*it).second.fragmentCalssifierHoldTime =
                node->getNodeTime() + delay;

            Int32 moreFrag = frgp->if6_off & IP6F_MORE_FRAG;
            if (moreFrag == 0)
            {
                // last fragment  delete the ipFragmentClassifier
                dot16Cs->ipFragmentClassifierMap->erase(it);
            }

            ipFragmentClassifierValue->srcPort = mapValue.srcPort;
            ipFragmentClassifierValue->dstPort = mapValue.dstPort;
        }
    }
}


// /**
// FUNCTION   :: MacDot16QosBuildClassifierInfo
// LAYER      :: MAC
// PURPOSE    :: Fill in classifier info structure.
// PARAMETERS ::
// + node        : Node*            : Pointer to node.
// + dot16       : MacDataDot16*    : Pointer to DOT16 structure
// + networkType : int              : Type of network
// + macAddress  : unsigned char*   : Nodes MAC address
// + msg         : Message*         : Packet from upper layer
// + priority    : TosType          : Priority of packet
// + classifierInfo   : MacDot16CsClassifier* : classifer info of this flow
// + payload     : char* : Packet payload
// + fragment    : BOOL* : packet is a fragment
// RETURN     :: void : NULL
// **/
static
void MacDot16CsBuildClassifierInfo(
        Node* node,
        MacDataDot16* dot16,
        int networkType,
        unsigned char* macAddress,
        Mac802Address nextHop,
        Message* msg,
        TosType* tos,
        MacDot16CsClassifier* classifierInfo,
        char** payload,
        BOOL* fragment)
{

    IpHeaderType* ipHeader = NULL;
    ip6_hdr* ipv6Header = NULL;
    BOOL isARPPacket = FALSE;
    memset(classifierInfo, 0, sizeof(MacDot16CsClassifier));

    if (networkType == NETWORK_PROTOCOL_IP
        && LlcIsEnabled(node,(int)DEFAULT_INTERFACE))
    {
        LlcHeader* llc;
        llc = (LlcHeader*)*payload;
        if (llc->etherType == PROTOCOL_TYPE_ARP)
        {
            isARPPacket = TRUE;
        }
    }

    // The following code has been added for integrating
    // MPLS with 802.16
    // Skip LLC and MPLS headers
    *payload = MacSkipLLCandMPLSHeader(node, *payload);

    // Build classifier info structure for IPv4
    if (networkType == NETWORK_PROTOCOL_IP)
    {
        if (isARPPacket == TRUE)
        {
            ArpPacket* arp = (ArpPacket*) *payload;
            // non-app layer packet, might be routing packet etc
            // Classify all such packet into one queue
            SetIPv4AddressInfo( &classifierInfo->srcAddr, arp->s_IpAddr);
            SetIPv4AddressInfo( &classifierInfo->dstAddr, arp->d_IpAddr);
            classifierInfo->srcPort = 0;
            classifierInfo->dstPort = 0;
            classifierInfo->ipProtocol = IPPROTO_IP;
            classifierInfo->type = CLASSIFIER_TYPE_MCAST_nrtPS;
            memcpy(macAddress, &nextHop.byte, MAC_ADDRESS_LENGTH_IN_BYTE);
            classifierInfo->nextHopAddr = nextHop;
            return;
        }
        // IPv4 packet
        // get IP header
        ipHeader = (IpHeaderType*) *payload;
        *payload += IpHeaderSize(ipHeader);
        unsigned char nextProtocol = ipHeader->ip_p;
        if (((IpHeaderGetIpMoreFrag(ipHeader->ipFragment)) == 1) ||
            (IpHeaderGetIpFragOffset(ipHeader->ipFragment))){
            *fragment = TRUE;
        }
        if (nextProtocol == IPPROTO_DSR && ((**payload == IPPROTO_UDP)
            || (**payload == IPPROTO_TCP)))
        {
            // Skip Routing protocol DSR header
            UInt16 payloadLen = 0;
            nextProtocol = **payload;
            memcpy(&payloadLen, (*payload + 2), sizeof(UInt16));
            //*payload += DSR_SIZE_OF_HEADER + payloadLen;
            *payload += 4 + payloadLen;
        }
        if (nextProtocol != IPPROTO_UDP && nextProtocol != IPPROTO_TCP)
        {
            // non-app layer packet, might be routing packet etc
            // Classify all such packet into one queue
            SetIPv4AddressInfo(&classifierInfo->srcAddr, ipHeader->ip_src);
            SetIPv4AddressInfo(&classifierInfo->dstAddr, ipHeader->ip_dst);
            classifierInfo->srcPort = 0;
            classifierInfo->dstPort = 0;
            classifierInfo->ipProtocol = IPPROTO_IP;
        }
        else if ((*fragment))
        {
            // Fragmented packet
            MacDot16CsIPFragmentClassifierValue ipFragmentClassifierValue;
            MacDot16CsBuildIPv4FragmentClassifierInfo(
                                              node,
                                              dot16,
                                              ipHeader,
                                              payload,
                                              &ipFragmentClassifierValue);
            SetIPv4AddressInfo(&classifierInfo->srcAddr, 
                               ipHeader->ip_src);
            SetIPv4AddressInfo(&classifierInfo->dstAddr, 
                               ipHeader->ip_dst);
            classifierInfo->ipProtocol = ipHeader->ip_p;
            classifierInfo->srcPort = ipFragmentClassifierValue.srcPort;
            classifierInfo->dstPort = ipFragmentClassifierValue.dstPort;
        }
        else
        {
            if (nextProtocol == IPPROTO_UDP)
            {   // this is a UDP packet
                TransportUdpHeader* udpHdr;

                udpHdr = (TransportUdpHeader*) *payload;
                *payload += sizeof(TransportUdpHeader);

                SetIPv4AddressInfo(&classifierInfo->srcAddr,
                                   ipHeader->ip_src);
                SetIPv4AddressInfo(&classifierInfo->dstAddr,
                                   ipHeader->ip_dst);
                classifierInfo->srcPort = udpHdr->sourcePort;
                classifierInfo->dstPort = udpHdr->destPort;
                classifierInfo->ipProtocol = IPPROTO_UDP;
            }
            else if (nextProtocol == IPPROTO_TCP)
            {   // this is a TCP packet
                struct tcphdr* tcpHdr;

                tcpHdr = (struct tcphdr *) *payload;
                *payload += sizeof(struct tcphdr);

                SetIPv4AddressInfo(&classifierInfo->srcAddr,
                                   ipHeader->ip_src);
                SetIPv4AddressInfo(&classifierInfo->dstAddr,
                                   ipHeader->ip_dst);
                classifierInfo->srcPort = tcpHdr->th_sport;
                classifierInfo->dstPort = tcpHdr->th_dport;
                classifierInfo->ipProtocol = IPPROTO_TCP;
            }
        }

        // get the packet TOS and use it as the priority
        *tos = IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len) >> 2;

        // set broadcasts as non-real time multicast
        if (ipHeader->ip_dst == ANY_ADDRESS)
        {
            classifierInfo->type = CLASSIFIER_TYPE_MCAST_nrtPS;
        }
        else if (dot16->stationType == DOT16_BS &&
                 NetworkIpIsMulticastAddress(node, ipHeader->ip_dst))
        {
            // only BS has such classifier
            MacDot16ServiceType serviceType;
            serviceType = MacDot16TcGetServiceClass(node,
                                                    dot16,
                                                    *tos);

            if (serviceType == DOT16_SERVICE_UGS)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_UGS;
            }
            else if (serviceType == DOT16_SERVICE_ertPS)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_ertPS;
            }
            else if (serviceType == DOT16_SERVICE_rtPS)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_rtPS;
            }
            else if (serviceType == DOT16_SERVICE_nrtPS)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_nrtPS;
            }
            else if (serviceType == DOT16_SERVICE_BE)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_BE;
            }
            else
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_BE;
                ERROR_ReportWarning("MAC 802.16: unknown service type!");
            }

        }
        else
        {
            classifierInfo->type = CLASSIFIER_TYPE_1;
        }
    }
    // Build classifier info structure for IPv6
    else if (networkType == NETWORK_PROTOCOL_IPV6)
    {
        // IPv6 packet
        // get IPv6 header
        ipv6Header = (ip6_hdr*) *payload;
        *payload += sizeof(ip6_hdr);

        // get the packet priority and use it as the tos
        *tos = ip6_hdrGetClass(ipv6Header->ipv6HdrVcf) >> 2;

        int nextHdr = ipv6Header->ip6_nxt;
        int hdrLength = ipv6Header->ip6_plen;

        // look at payload
        while (nextHdr)
        {
            if (nextHdr == IPPROTO_DSR && ((**payload == IPPROTO_UDP)
                || (**payload == IPPROTO_TCP)))
            {
                // Skip Routing protocol DSR header
                UInt16 payloadLen = 0;
                nextHdr = **payload;
                memcpy(&payloadLen, (*payload + 2), sizeof(UInt16));
                //*payload += DSR_SIZE_OF_HEADER + payloadLen;
                *payload += 4 + payloadLen;
            }
            else if (nextHdr == IPPROTO_ICMPV6 || nextHdr == IPPROTO_OSPF)
            {
                // non-applayer packet, ICMP or OSPF packets
                // Classify all such packet into one queue
                SetIPv6AddressInfo(&classifierInfo->srcAddr,
                                    ipv6Header->ip6_src);
                SetIPv6AddressInfo(&classifierInfo->dstAddr,
                                    ipv6Header->ip6_dst);
                classifierInfo->srcPort = 0;
                classifierInfo->dstPort = 0;
                classifierInfo->ipProtocol = nextHdr;
                break;
            }
            else if (nextHdr == IP6_NHDR_FRAG)
            {
                ipv6_fraghdr* frgp = (ipv6_fraghdr*) *payload;
                nextHdr = frgp->if6_nh;
                MacDot16CsIPFragmentClassifierValue 
                                                ipFragmentClassifierValue;
                MacDot16CsBuildIPv6FragmentClassifierInfo(
                                              node,
                                              dot16,
                                              ipv6Header,
                                              payload,
                                              &ipFragmentClassifierValue);
                SetIPv6AddressInfo(&classifierInfo->srcAddr, 
                                   ipv6Header->ip6_src);
                SetIPv6AddressInfo(&classifierInfo->dstAddr, 
                                   ipv6Header->ip6_dst);
                classifierInfo->ipProtocol = frgp->if6_nh;
                classifierInfo->srcPort = ipFragmentClassifierValue.srcPort;
                classifierInfo->dstPort = ipFragmentClassifierValue.dstPort;
                *fragment = TRUE;
                break;
            }
            else if (nextHdr == IPPROTO_UDP)
            {
                // this is a UDP packet
                TransportUdpHeader* udpHdr;

                udpHdr = (TransportUdpHeader*) *payload;
                *payload += sizeof(TransportUdpHeader);

                SetIPv6AddressInfo( &classifierInfo->srcAddr,
                                    ipv6Header->ip6_src);
                SetIPv6AddressInfo( &classifierInfo->dstAddr,
                                    ipv6Header->ip6_dst);
                classifierInfo->srcPort = udpHdr->sourcePort;
                classifierInfo->dstPort = udpHdr->destPort;
                classifierInfo->ipProtocol = IPPROTO_UDP;
                break;
            }
            else if (nextHdr == IPPROTO_TCP)
            {
                // this is a TCP packet
                struct tcphdr* tcpHdr;

                tcpHdr = (struct tcphdr *) *payload;
                *payload += sizeof(struct tcphdr);

                SetIPv6AddressInfo( &classifierInfo->srcAddr,
                                    ipv6Header->ip6_src);
                SetIPv6AddressInfo( &classifierInfo->dstAddr,
                                    ipv6Header->ip6_dst);
                classifierInfo->srcPort = tcpHdr->th_sport;
                classifierInfo->dstPort = tcpHdr->th_dport;
                classifierInfo->ipProtocol = IPPROTO_TCP;
                break;
            }
            else
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                SetIPv6AddressInfo( &classifierInfo->srcAddr,
                                    ipv6Header->ip6_src);
                SetIPv6AddressInfo( &classifierInfo->dstAddr,
                                    ipv6Header->ip6_dst);
                classifierInfo->srcPort = 0;
                classifierInfo->dstPort = 0;
                classifierInfo->ipProtocol = IPPROTO_IPV6;
                break;
            }
            // get next header
            *payload += ipv6Header->ip6_plen;
            ipv6Header = (ip6_hdr*) *payload;

            nextHdr = ipv6Header->ip6_nxt;
            hdrLength = ipv6Header->ip6_plen;

            if (hdrLength == 0)
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                SetIPv6AddressInfo( &classifierInfo->srcAddr,
                                    ipv6Header->ip6_src);
                SetIPv6AddressInfo( &classifierInfo->dstAddr,
                                    ipv6Header->ip6_dst);
                classifierInfo->srcPort = 0;
                classifierInfo->dstPort = 0;
                classifierInfo->ipProtocol = IPPROTO_IPV6;
                break;
            }

        }

        // set broadcasts as non real time multicast
        if (MAC_IsBroadcastMac802Address(&nextHop))
        {
            classifierInfo->type = CLASSIFIER_TYPE_MCAST_nrtPS;
        }
        else if (dot16->stationType == DOT16_BS &&
                 IS_MULTIADDR6(ipv6Header->ip6_dst))
        {
            // only BS has such classifier
            MacDot16ServiceType serviceType;
            serviceType = MacDot16TcGetServiceClass(node,
                                                    dot16,
                                                    *tos);

            if (serviceType == DOT16_SERVICE_UGS)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_UGS;
            }
            else if (serviceType == DOT16_SERVICE_ertPS)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_ertPS;
            }
            else if (serviceType == DOT16_SERVICE_rtPS)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_rtPS;
            }
            else if (serviceType == DOT16_SERVICE_nrtPS)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_nrtPS;
            }
            else if (serviceType == DOT16_SERVICE_BE)
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_BE;
            }
            else
            {
                classifierInfo->type = CLASSIFIER_TYPE_MCAST_BE;
                ERROR_ReportWarning("MAC 802.16: unknown service type!");
            }
        }
        else
        {
            classifierInfo->type = CLASSIFIER_TYPE_1;
        }

        // Need to revisit
        //MacDot16Ipv4ToMacAddress(macAddress, nextHop);
    }
    else if (networkType == NETWORK_ATM)
    {
        // ATM packet, use ATM classifier
        ERROR_Assert(FALSE,"Invalid network type");
    }
    memcpy(macAddress, &nextHop.byte, MAC_ADDRESS_LENGTH_IN_BYTE);
    classifierInfo->nextHopAddr = nextHop;
}


// /**
// FUNCTION   :: MacDot16CsGetClassifierInfo
// LAYER      :: MAC
// PURPOSE    :: Fill in classifier info structure.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + classifierInfo   : MacDot16CsClassifier* : classifer info of this flow
// RETURN     :: MacDot16CsClassifierRecord
// **/
MacDot16CsClassifierRecord* MacDot16CsGetClassifierInfo(
        Node* node,
        MacDataDot16* dot16,
        MacDot16CsClassifier* classifierInfo)
{
    MacDot16Cs* dot16Cs = (MacDot16Cs*) dot16->csData;
    MacDot16CsClassifierRecord* recordPtr = NULL;
    int i = 0;

    if ((classifierInfo->type == CLASSIFIER_TYPE_MCAST_UGS) ||
        (classifierInfo->type == CLASSIFIER_TYPE_MCAST_ertPS) ||
        (classifierInfo->type == CLASSIFIER_TYPE_MCAST_rtPS) ||
        (classifierInfo->type == CLASSIFIER_TYPE_MCAST_nrtPS) ||
        (classifierInfo->type == CLASSIFIER_TYPE_MCAST_BE))
    {
        for (i = 0; i < dot16Cs->numClassifierRecord; i ++)
        {
            recordPtr = dot16Cs->classifierRecord[i];
            if (recordPtr->basicCid < DOT16_BASIC_CID_START)
            {
                // for those invalid recorde skip
                continue;
            }

            MacDot16CsClassifier* classifier =
                (MacDot16CsClassifier*) recordPtr->classifier;

            if (classifier->type == classifierInfo->type)
            {
                // found an existing multicast Type classifier
                return recordPtr;
            }
        }

    }
    else
    {
         // Search the classifier list to see if a classifier
         // has been created
        for (i = 0; i < dot16Cs->numClassifierRecord; i ++)
        {
            recordPtr = dot16Cs->classifierRecord[i];

            if (recordPtr->basicCid < DOT16_BASIC_CID_START)
            {
                // for those invalid recorde skip
                continue;
            }
            if (memcmp((char*) classifierInfo,
                       (char*) recordPtr->classifier,
                       sizeof(MacDot16CsClassifier)) == 0)
            {
                // found an existing classifier
                return recordPtr;
            }

        }
    }

    return NULL;
}

// /**
// FUNCTION   :: MacDot16CsNewClassifier
// LAYER      :: MAC
// PURPOSE    :: create new classifier info structure.
// PARAMETERS ::
// + node           : Node*                 : Pointer to node.
// + dot16          : MacDataDot16*         : Pointer to DOT16 structure
// + serviceType    : MacDot16ServiceType   : Data service type of this flow
// + msg            : Message*              : Packet from upper layer
// + macAddress     : unsigned char*        : MAC address of node
// + nextHop        : NodeAddress nextHop   : Next hop address
// + qosInfo        : MacDot16QoSParameter* : QOS info of this flow
// + classifierInfo : MacDot16CsClassifier* : Classifer info of this flow
// RETURN     :: MacDot16CsClassifierRecord
// **/
MacDot16CsClassifierRecord* MacDot16CsNewClassifier(
        Node* node,
        MacDataDot16* dot16,
        MacDot16ServiceType serviceType,
        Message* msg,
        unsigned char* macAddress,
//        Mac802Address nextHop,
        MacDot16QoSParameter* qosInfo,
        MacDot16CsClassifier* classifierInfo)
{
    MacDot16CsClassifierRecord* recordPtr = NULL;
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;

    // need to create a new classifier and classifier record
    recordPtr = MacDot16CsClassifierAddRecord(node,
                                              dot16,
                                              serviceType,
                                              classifierInfo);

    // notify the MAC CPS about the new classifier (add a service flow)
    if (dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
        // add new base station flow
        Dot16CIDType transportCid;
        recordPtr->basicCid = MacDot16BsAddServiceFlow(
            node,
            dot16,
            macAddress,
            serviceType,
            recordPtr->csfId,
            qosInfo,
            classifierInfo,
            (unsigned int) (-1),
            // transId, only for remotely init ul flow
            DOT16_DOWNLINK_SERVICE_FLOW,
            DOT16_SERVICE_FLOW_LOCALLY_INITIATED,
            (Dot16CIDType) (-1),
            // ss primary cid, only for remtely
            // init ul flow
            &transportCid,
            dot16Bs->isPackingEnabled, //isPackingEnabled,
            FALSE, //isFixedLengthSDU,
            DOT16_DEFAULT_FIXED_LENGTH_SDU_SIZE, //fixedLengthSDUSize
            (TraceProtocolType) msg->originatingProtocol,
            dot16Bs->isARQEnabled,
            dot16Bs->arqParam);


        if (DEBUG_FLOW)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("CS layer, adding BS service flow %d.\n",
                   recordPtr->basicCid);
        }
    }
    else
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        MacDot16ARQParam dot16SsARQParam ;
        memset(&dot16SsARQParam,0,sizeof(MacDot16ARQParam));

        if (DEBUG_FLOW)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("CS layer, adding SS service flow %d.\n",
                    recordPtr->basicCid);
        }

        if (dot16Ss->isARQEnabled)
        {
            memcpy(&dot16SsARQParam,
                            dot16Ss->arqParam,sizeof (MacDot16ARQParam));
            MacDot16ARQConvertParam(&dot16SsARQParam,
                                    dot16Ss->servingBs->frameDuration);
        }

        recordPtr->basicCid = MacDot16SsAddServiceFlow(
            node,
            dot16,
            macAddress,
            serviceType,
            recordPtr->csfId,
            qosInfo,
            classifierInfo,
            (unsigned int) (-1),
            // transId, no meaning for ul flow
            DOT16_UPLINK_SERVICE_FLOW,
            DOT16_SERVICE_FLOW_LOCALLY_INITIATED,
            dot16Ss->isPackingEnabled, //isPackingEnabled,
            FALSE, //isFixedLengthSDU,
            DOT16_DEFAULT_FIXED_LENGTH_SDU_SIZE, //fixedLengthSDUSize
            (TraceProtocolType) msg->originatingProtocol,
            dot16Ss->isARQEnabled,
            &dot16SsARQParam);
        if (DEBUG_FLOW)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("CS layer, adding SS service flow %d.\n",
                    recordPtr->basicCid);
        }
    }

    if (recordPtr->basicCid < DOT16_BASIC_CID_START)
    {
        // not a valid service flow, drop the packet
        if (DEBUG_FLOW)
        {
            printf("Node%d is unable to add a service flow\n",
                   node->nodeId);
        }

        // the classifier record is invalid now, keep it for a delay
        // in order to aovid keeping adding it as routing may
        // continue send packet before the out dated route being
        // timed out.
        //
        // We set the invalid time and set a timer to remove this
        // classifier record after a delay.
        recordPtr->invalidTime = node->getNodeTime();

        // set a timer to remove the invalid classifier after a delay
        MacDot16SetTimer(node,
                         dot16,
                         DOT16_TIMER_CS_ClassifierTimeout,
                         DOT16_CS_CLASSIFIER_REMOVAL_DELAY,
                         NULL,
                         0);

        return NULL;
    }

    return recordPtr;
}


// /**
// FUNCTION   :: MacDot16CsClassifyPacket
// LAYER      :: MAC
// PURPOSE    :: Classify a packet
// PARAMETERS ::
// + node       : Node*         : Pointer to node.
// + dot16      : MacDataDot16* : Pointer to DOT16 structure
// + msg        : Message*      : Packet from upper layer
// + nextHop    : NodeAddress   : Address of the next hop
// + networkType  : int         : Type of network originated from
// + msgDropped  : BOOL*    : Parameter to determine whether msg was dropped
// RETURN     :: void : NULL
// **/
static
BOOL MacDot16CsClassifyPacket(
         Node* node,
         MacDataDot16* dot16,
         Message* msg,
         TosType priority,
         Mac802Address nextHop,
         int networkType,
         BOOL* msgDropped)
{
    *msgDropped = FALSE;
    MacDot16Cs* dot16Cs = (MacDot16Cs*) dot16->csData;
    MacDot16CsClassifierRecord* recordPtr = NULL;
    unsigned char macAddress[DOT16_MAC_ADDRESS_LENGTH];
    MacDot16ServiceType serviceType = DOT16_SERVICE_BE;
    TosType tos = 0;

    MacDot16QoSParameter qosInfo;
    memset(&qosInfo, 0, sizeof(MacDot16QoSParameter));

    MacDot16CsClassifier classifierInfo;
    memset(&classifierInfo, 0, sizeof(MacDot16CsClassifier));

    BOOL enqueued = FALSE;
    BOOL fragment = FALSE;

    char* payload = NULL;
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16StationClass stationClass = DOT16_STATION_CLASS_DEFAULT;

    // number of packets entering classifier
    dot16Cs->stats.numPktsFromUpper ++;

    // retrive information from the packet headers
    payload = MESSAGE_ReturnPacket(msg);
    // build the classifier info struct
    MacDot16CsBuildClassifierInfo(node,
            dot16,
            networkType,
            macAddress,
            nextHop,
            msg,
            &tos,
            &classifierInfo,
            &payload,
            &fragment);

    // get the classifier info for this packet
    recordPtr = MacDot16CsGetClassifierInfo(
                    node,
                    dot16,
                    &classifierInfo);

    TraceProtocolType appType =
            (TraceProtocolType) msg->originatingProtocol;

    // update QOS info
    MacDot16QosUpdateParameters(
            node,
            dot16,
            tos,
            appType,
            &classifierInfo,
            &payload,
            &stationClass,
            &qosInfo,
            MESSAGE_ReturnPacketSize(msg));

    qosInfo.priority = (UInt8)priority;

    // get the service class for the packet
    serviceType = MacDot16TcGetServiceClass(
                    node,
                    dot16,
                    tos);

    // need to add new record
    if (recordPtr == NULL)
    {

        // create new classifier
        recordPtr = MacDot16CsNewClassifier(
                        node,
                        dot16,
                        serviceType,
                        msg,
                        macAddress,
                        &qosInfo,
                        &classifierInfo);
    }

    if (recordPtr == NULL)
    {
        if (DEBUG_WARNING)
        {
            ERROR_ReportWarning(
                "MAC 802.16: Unable to create classifier! Packet dropped! \n");
        }
        MESSAGE_Free(node, msg);
        *msgDropped = TRUE;
        return TRUE;
    }

    // condition packet
/*
    BOOL packetWasDropped = FALSE;
    char* className =
            (char*)((MacDot16Qos*) dot16->qosData)->stationClassName;
*/
    MacDot16ServiceFlowDirection sfDirection;

    if (dot16->stationType == DOT16_BS)
        sfDirection = DOT16_DOWNLINK_SERVICE_FLOW;
    else
        sfDirection = DOT16_UPLINK_SERVICE_FLOW;
/*
    // police the packed
    MacDot16TcProfilePacketAndPassOrDrop(
            node,
            (MacDot16TcData*) dot16->tcData,
            msg,
            className,
            sfDirection,
            &packetWasDropped);

    if (packetWasDropped)
    {
        MAC_NotificationOfPacketDrop(
                node,
                nextHop,
                dot16->myMacData->interfaceIndex,
                msg);

        dot16Cs->stats.numPktsDropped ++;

        // tell the upper layer it was handled
        return TRUE;
    }
*/
    dot16Cs->stats.numPktsClassified ++;

    // pass the classfied packet to MAC CPS
    if (dot16->stationType == DOT16_BS)
    {
        //MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
        enqueued = MacDot16BsEnqueuePacket(
                        node,
                        dot16,
                        recordPtr->basicCid,
                        recordPtr->csfId,
                        recordPtr->serviceType,
                        msg,
                        &sFlow,
                        msgDropped);

        // not a valid service flow or full queue, drop the packet
        if (enqueued == FALSE)
        {
            return FALSE;
        }

        ssInfo = MacDot16BsGetSsByCid(node, dot16, recordPtr->basicCid);

        // tell BS about service flow change
        if (ssInfo && sFlow && !fragment &&
            MacDot16ServiceFlowQoSChanged(sFlow->qosInfo, qosInfo) == TRUE)
        {
            MacDot16BsChangeServiceFlow(node,
                                        dot16,
                                        ssInfo,
                                        sFlow,
                                        &qosInfo);

            if (DEBUG_FLOW)
            {

                MacDot16PrintRunTimeInfo(node, dot16);

                printf("data PDU has different QoS from the service flow. "
                       "DSC transaction is needed for service flow %d "
                       "(Cid: %d)\n",
                        sFlow->sfid, recordPtr->basicCid);

            }
        }

    }
    else
    {
        enqueued = MacDot16SsEnqueuePacket(
                       node,
                       dot16,
                       macAddress,
                       recordPtr->csfId,
                       recordPtr->serviceType,
                       msg,
                       &sFlow,
                       msgDropped);

        // not a valid service flow or queue is full, drop the packet
        if (enqueued == FALSE)
        {
            return FALSE;
        }

        // tell BS about service flow change
        if (sFlow && !fragment &&
            MacDot16ServiceFlowQoSChanged(sFlow->qosInfo, qosInfo) == TRUE)
        {
            MacDot16SsChangeServiceFlow(node, dot16, sFlow, &qosInfo);

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);

                printf("data PDU has different QoS from the service flow. "
                       "DSC transaction is needed for service flow %d "
                       "(Cid: %d)\n",
                        sFlow->sfid, sFlow->cid);

            }
        }

    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////
//
//  End of Common Clasifier functions
//
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Common Convergance Sublayer functions
//
//////////////////////////////////////////////////////////////////////

// /**
// FUNCTION   :: MacDot16CsPacketFromUpper
// LAYER      :: MAC
// PURPOSE    :: Upper layer pass a data PDU to SAP of Dot16 CS
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + msg       : Message*      : Packet from upper layer
// + nextHop   : NodeAddress   : Address of the next hop
// + networkType : int         : Type of network
// + priority  : TosType       : Priority of the packet
// + msgDropped  : BOOL*    : Parameter to determine whether msg was dropped
// RETURN     :: BOOL : TRUE, packet is handled, ie enqueued, dropped, etc
//                      FALSE, unable to enqueue this packet, stop passing
// **/
BOOL MacDot16CsPacketFromUpper(
        Node* node,
        MacDataDot16* dot16,
        Message* msg,
        Mac802Address nextHop,
        int networkType,
        TosType priority,
        BOOL* msgDropped)
{
    BOOL handled = TRUE;
    *msgDropped = FALSE;
    if (dot16->stationType == DOT16_SS)
    {
        MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

        if (dot16Ss->operational != TRUE)
        {
            return FALSE;
        }
        else if (dot16Ss->idleSubStatus != DOT16e_SS_IDLE_None)
        {
            // re-enter network because the node is in idle mode
            dot16Ss->performNetworkReentry = TRUE;
            return FALSE;
        }
        else
        {
            memcpy(&nextHop.byte, &dot16Ss->servingBs->bsId,
                MAC_ADDRESS_LENGTH_IN_BYTE);
        }

        if (dot16Ss->isIdleEnabled == TRUE)
        {
            dot16Ss->lastCSPacketRcvd = node->getNodeTime();
        }
    }
    if ((networkType == NETWORK_PROTOCOL_IP) ||
        (networkType == NETWORK_PROTOCOL_IPV6) )
    { // classify

        handled = MacDot16CsClassifyPacket(
                      node,
                      dot16,
                      msg,
                      priority,
                      nextHop,
                      networkType,
                      msgDropped);
    }
    else if (networkType == NETWORK_ATM )
    {
        // ATM packet, use ATM classifier
        ERROR_ReportWarning("MAC 802.16: ATM is not supported yet!");
        MESSAGE_Free(node, msg);
        *msgDropped = TRUE;
    }
    else
    { // Unknown upper layer protocol
        ERROR_ReportError(
            "MAC 802.16: Upper layer protocol is not supported!");
    }
    return handled;
}

// /**
// FUNCTION   :: MacDot16CsPacketFromLower
// LAYER      :: MAC
// PURPOSE    :: Lower Sublayer(CPS) pass a data PDU to CS
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + msg       : Message*      : Packet from lower layer
// + lastHopMacAddr : unsigned char* : MAC address of previous hop
// RETURN     :: void : NULL
// **/
void MacDot16CsPacketFromLower(
        Node* node,
        MacDataDot16* dot16,
        Message* msg,
        unsigned char* lastHopMacAddr,
        MacDot16BsSsInfo* ssInfo)
{
    Mac802Address lastHopAddr;

    memcpy(&lastHopAddr.byte, lastHopMacAddr,
        MAC_ADDRESS_LENGTH_IN_BYTE);

    msg->next = NULL;
    if (dot16->stationType == DOT16_SS)
    {
        MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
        if (dot16Ss->isIdleEnabled == TRUE)
        {
            dot16Ss->lastCSPacketRcvd = node->getNodeTime();
        }
    }
    else
    {
        MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
        if (dot16->dot16eEnabled && dot16Bs->isIdleEnabled)
        {
            ERROR_Assert(ssInfo != NULL, "ssInfo can not be NULL!");
        }
    }
    // assume no encryption
    // assume no header compression
    // pass packet to upper layer
    MAC_HandOffSuccessfullyReceivedPacket(
        node,
        dot16->myMacData->interfaceIndex,
        msg,
        &lastHopAddr);
}

// /**
// FUNCTION   :: MacDot16CsInit
// LAYER      :: MAC
// PURPOSE    :: Initialize DOT16 CS at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16CsInit(
        Node* node,
        int interfaceIndex,
        const NodeInput* nodeInput)
{
    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;

    MacDot16Cs* dot16Cs = (MacDot16Cs*) MEM_malloc(sizeof(MacDot16Cs));
    ERROR_Assert(dot16Cs != NULL,
                 "MAC 802.16: Out of memory!");

    // using memset to initialize the whole DOT16 CS data strucutre
    memset((char*)dot16Cs, 0, sizeof(MacDot16Cs));
    dot16->csData = (void*) dot16Cs;
    // alloc initial classifier array
    dot16Cs->numClassifierRecord = 0;
    dot16Cs->classifierRecord = (MacDot16CsClassifierRecord**)
                          MEM_malloc(sizeof(MacDot16CsClassifierRecord*) *
                                     DOT16_CS_NUM_RECORD);
    dot16Cs->allocClassifierRecord = DOT16_CS_NUM_RECORD;

    if (dot16->stationType == DOT16_SS)
    {
        MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
        if (dot16Ss->isIdleEnabled == TRUE)
        {
            dot16Ss->lastCSPacketRcvd = node->getNodeTime();
        }
    }

    // initialize the fragment clssifier map
    dot16Cs->ipFragmentClassifierMap =
                new MacDot16CsIpFragmentClassifierMap;
}

// /**
// FUNCTION   :: MacDot16CsLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : int      : Interface index
// + msg              : Message* : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacDot16CsLayer(
        Node* node,
        int interfaceIndex,
        Message* msg)
{
    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;
    BOOL msgReused = FALSE;

    switch (msg->eventType)
    {
        case MSG_MAC_TimerExpired:
        {
            MacDot16TimerType timerType;
            MacDot16Timer* timerInfo = NULL;

            // get info from message
            timerInfo = (MacDot16Timer*) MESSAGE_ReturnInfo(msg);
            timerType = timerInfo->timerType;

            if (DEBUG_TIMER)
            {
                printf("MAC 802.16: node%u intf%d get a timer msg %d\n",
                       node->nodeId, interfaceIndex, timerType);
            }

            switch (timerType)
            {
                case DOT16_TIMER_CS_ClassifierTimeout:
                {
                    // Remove invalid classifier
                    MacDot16CsClassifierRemoveInvalidRecord(node, dot16);
                    break;
                }
                default:
                {
                    char tmpString[MAX_STRING_LENGTH];

                    sprintf(tmpString,
                            "MAC 802.16: Unknow timer type %d\n",
                            timerType);
                    ERROR_ReportError(tmpString);

                    break;
                }
            }

            break;
        }

        default:
        {
            char tmpString[MAX_STRING_LENGTH];

            sprintf(tmpString,
                    "MAC 802.16: Unknow message event type %d\n",
                    msg->eventType);
            ERROR_ReportError(tmpString);

            break;
        }
    }

    if (!msgReused)
    {
        MESSAGE_Free(node, msg);
    }
}

// /**
// FUNCTION   :: MacDot16CsFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16CsFinalize(
        Node *node,
        int interfaceIndex)
{
    MacDataDot16* dot16 = (MacDataDot16 *)
                          node->macData[interfaceIndex]->macVar;

    // delete the memory for ipFragmentClassifierMap
    MacDot16Cs* dot16Cs = (MacDot16Cs*) dot16->csData;
    delete(dot16Cs->ipFragmentClassifierMap);

    MacDot16CsPrintStats(node, dot16, interfaceIndex);
}


//////////////////////////////////////////////////////////////////////
//
//  End of common Convergance Sublayer functions
//
//////////////////////////////////////////////////////////////////////

