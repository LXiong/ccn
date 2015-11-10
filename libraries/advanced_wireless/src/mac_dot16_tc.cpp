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
// Objectives: Dot16 Traffic Conditioning by remarking/dropping based on
//             bandwidth/delay policy
//


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"
#include "network_ip.h"

// The following code has been added
// for integrating 802.16 with MPLS.
#include "mac.h"

#include "mac_dot16.h"
#include "mac_dot16_bs.h"
#include "mac_dot16_ss.h"
#include "mac_dot16_cs.h"
#include "mac_dot16_tc.h"


#define   DOT16_TC_DEBUG_METER              0
#define   DOT16_TC_DEBUG_MARKER             0
#define   DOT16_TC_DEBUG_INIT               0

// default qos parameters
const char* dot16QosDefaults[] = {
// Classifiers for remapping tos
//
//        Src   Dest   Tos  Protocol  Src Port  Dest Port  Direction  Class
//        ---   ----   ---  --------  --------  ---------  ---------  -----
//
"QOS-CLASSIFIER   ANY   ANY   56   ANY   ANY   ANY   ANY   56",
"QOS-CLASSIFIER   ANY   ANY   48   ANY   ANY   ANY   ANY   48",
"QOS-CLASSIFIER   ANY   ANY   46   ANY   ANY   ANY   ANY   46",
"QOS-CLASSIFIER   ANY   ANY   40   ANY   ANY   ANY   ANY   40",

"QOS-CLASSIFIER   ANY   ANY   32   ANY   ANY   ANY   ANY   32",
"QOS-CLASSIFIER   ANY   ANY   34   ANY   ANY   ANY   ANY   34",
"QOS-CLASSIFIER   ANY   ANY   36   ANY   ANY   ANY   ANY   36",
"QOS-CLASSIFIER   ANY   ANY   38   ANY   ANY   ANY   ANY   38",

"QOS-CLASSIFIER   ANY   ANY   24   ANY   ANY   ANY   ANY   24",
"QOS-CLASSIFIER   ANY   ANY   26   ANY   ANY   ANY   ANY   26",
"QOS-CLASSIFIER   ANY   ANY   28   ANY   ANY   ANY   ANY   28",
"QOS-CLASSIFIER   ANY   ANY   30   ANY   ANY   ANY   ANY   30",

"QOS-CLASSIFIER   ANY   ANY   16   ANY   ANY   ANY   ANY   16",
"QOS-CLASSIFIER   ANY   ANY   18   ANY   ANY   ANY   ANY   18",
"QOS-CLASSIFIER   ANY   ANY   20   ANY   ANY   ANY   ANY   20",
"QOS-CLASSIFIER   ANY   ANY   22   ANY   ANY   ANY   ANY   22",

"QOS-CLASSIFIER   ANY   ANY    8   ANY   ANY   ANY   ANY    8",
"QOS-CLASSIFIER   ANY   ANY   10   ANY   ANY   ANY   ANY   10",
"QOS-CLASSIFIER   ANY   ANY   12   ANY   ANY   ANY   ANY   12",
"QOS-CLASSIFIER   ANY   ANY   14   ANY   ANY   ANY   ANY   14",
//
// Metering classes for stations
//
// rate and Burst are in KBits per second
//
//         Class  Meter type  DL Rate  DL Burst UL Rate  UL Burst
//         ------ ----------- -------- -------- -------- --------
"QOS-METER GOLD   TokenBucket 3000000  3000000  1000000  10000000",
"QOS-METER SILVER TokenBucket 1500000  1500000   512000   5120000",
"QOS-METER BRONZE TokenBucket  512000   512000   128000   1280000",
//
// Action taken with a packet if it does not conform to the above metering
//
//               Class     Action
//               -----     ------
"QOS-CONDITIONER  GOLD      HOLD",
"QOS-CONDITIONER  SILVER    HOLD",
"QOS-CONDITIONER  BRONZE    HOLD",
//
// Service mapping TOS/DSCP -> Service Class
//
//           TOS
//           DSCP  Service class
//           ----  -------------
"QOS-SERVICE  0    BE",     // precedence 0

"QOS-SERVICE  56   UGS",    // precedence 7
"QOS-SERVICE  48   nrtPS",  // precedence 6

"QOS-SERVICE  40   UGS",    // precedence 5

"QOS-SERVICE  32   ErtPS",  // precedence 4
"QOS-SERVICE  34   ErtPS",
"QOS-SERVICE  36   ErtPS",
"QOS-SERVICE  38   ErtPS",

"QOS-SERVICE  24   rtPS",  // precedence 3
"QOS-SERVICE  26   rtPS",
"QOS-SERVICE  28   rtPS",
"QOS-SERVICE  30   rtPS",

"QOS-SERVICE  16   nrtPS",  // precedence 2
"QOS-SERVICE  18   nrtPS",
"QOS-SERVICE  20   nrtPS",
"QOS-SERVICE  22   nrtPS",

"QOS-SERVICE   8   nrtPS",  // precedence 1
"QOS-SERVICE  10   nrtPS",
"QOS-SERVICE  12   nrtPS",
"QOS-SERVICE  14   nrtPS",

 NULL // teminate the array
};




//--------------------------------------------------------------------------
// Dot16 Multi-Field Traffic Conditioner
//--------------------------------------------------------------------------

// /**
// FUNCTION     :: MacDot16ReturnServiceInfo.
// PURPOSE:     :: returns condition class to which a particular tos
//                 belongs.
// PARAMETERS   ::
// + tc :   MacDot16TcData* : pointer to the traffic conditioner state
//                            variables
// + tos :  TosType : type of service value.
// RETURNS     ::    MacDot16TcInfo* : the index of the condition class.
//--------------------------------------------------------------------------
static
MacDot16TcInfo* MacDot16ReturnServiceInfo(
    MacDot16TcData* tc,
    TosType tos)
{
    int i = 0;

    while (i < tc->numTcInfo)
    {
        if (tc->tcInfo[i].tos == tos)
        {
             return &(tc->tcInfo[i]);
        }
        i++;
    }
    return NULL;
}


// /**
// FUNCTION     :: MacDot16ReturnStationInfoByName.
// PURPOSE      :: Returns station class info by class name.
// PARAMETERS   ::
// + tc :   MacDot16TcData* : pointer to the traffic conditioner state
//                            variables
// + stationClassName  : char* : station Class name.
// RETURNS:    :: MacDot16TcStationInfo* : the struct of the condition
//                                         class.
// **/
static
MacDot16TcStationInfo* MacDot16ReturnStationInfoByName(
    MacDot16TcData* tc,
    char* stationClassName)
{
    int i = 0;
    MacDot16TcStationInfo* stationInfo;

    while (i < tc->numTcStationInfo)
    {
        stationInfo = &tc->tcStationInfo[i];
        if (strcmp ((char*) stationInfo->stationClassName, stationClassName)
            == 0)
        {
             return stationInfo;
        }
        i++;
    }
    return NULL;
}
#if 0
// /**
// FUNCTION     :: MacDot16ReturnStationInfo.
// PURPOSE      :: Returns station class info.
// PARAMETERS   ::
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + stationClassName : int : station class name.
// RETURNS:     :: MacDot16TcStationInfo* : the index of the condition
//                                          class.
// **/
static
MacDot16TcStationInfo* MacDot16ReturnStationInfo(
    MacDot16TcData* tc,
    int stationClass)
{
    int i = 0;

    while (i < tc->numTcStationInfo)
    {
        if (tc->tcStationInfo[i].stationClass == stationClass)
        {
             return &(tc->tcStationInfo[i]);
        }
        i++;
    }
    return NULL;
}


// /**
// FUNCTION     :: MacDot16ReturnServiceClass.
// PURPOSE      :: check the packet tos and return which service class it is
//                 assigned to.
// PARAMETERS   ::
// + service : int : tos to remap
// RETURN       :: unsigned char : return service class or zero (BE).
// **/
static
unsigned char MacDot16ReturnServiceClass(int tos)
{

    if (tos == DOT16_DS_CLASS_AF10 ||
        tos == DOT16_DS_CLASS_AF11 ||
        tos == DOT16_DS_CLASS_AF12 ||
        tos == DOT16_DS_CLASS_AF13)
    {
        return DOT16_DS_CLASS_AF10;
    }
    else if (tos == DOT16_DS_CLASS_AF20 ||
        tos == DOT16_DS_CLASS_AF21 ||
        tos == DOT16_DS_CLASS_AF22 ||
        tos == DOT16_DS_CLASS_AF23)
    {
        return DOT16_DS_CLASS_AF20;
    }
    else if (tos == DOT16_DS_CLASS_AF30 ||
        tos == DOT16_DS_CLASS_AF31 ||
        tos == DOT16_DS_CLASS_AF32 ||
        tos == DOT16_DS_CLASS_AF33)
    {
        return DOT16_DS_CLASS_AF30;
    }
    else if (tos == DOT16_DS_CLASS_AF40 ||
        tos == DOT16_DS_CLASS_AF41 ||
        tos == DOT16_DS_CLASS_AF42 ||
        tos == DOT16_DS_CLASS_AF43)
    {
        return DOT16_DS_CLASS_AF40;
    }
    else if (tos == DOT16_DS_CLASS_EF ||
        tos == DOT16_DS_CLASS_EXF ||
        tos == Dot16_DS_CLASS_CTRL1 ||
        tos == Dot16_DS_CLASS_CTRL2)
    {
        return DOT16_DS_CLASS_EF;
    }

    return DOT16_DS_CLASS_BE;

}


// /**
// FUNCTION     :: MacDot16ReturnSourcePort
// PURPOSE:     :: returns source port of the message.
// PARAMETERS   ::
// + node : Node* : pointer to the node,
// + msg    : Message* : application packet.
// RETURN       ::  unsigned short : return source port number.
// **/

static
unsigned short MacDot16ReturnSourcePort(Node *node, Message* msg)
{
// Note: In future if used this function, update the code for ARP also


    // The following code has been added for integrating
    // MPLS with 802.16.
    char *payload = MESSAGE_ReturnPacket(msg) ;
    // Skip LLC and MPLS headers.
    payload = MacSkipLLCandMPLSHeader(node, payload) ;

    IpHeaderType* ipHdr = (IpHeaderType*) payload;
    char* transportHdr = ((char*) ipHdr + (ipHdr->ip_hl * 4));

    return((unsigned short)(*transportHdr));
}


// /**
// FUNCTION     ::MacDot16ReturnDestPort
// PURPOSE      :: returns destination port of the message.
// PARAMETERS   ::
// + node : Node* : pointer to the node,
// + msg    : Message* : application packet.
// RETURN       ::  unsigned short  return destination port number.
// **/

static
unsigned short MacDot16ReturnDestPort(Node *node, Message* msg)
{
// Note: In future if used this function, update the code for ARP also


    // The following code has been added for integrating
    // MPLS with 802.16.
    char *payload = MESSAGE_ReturnPacket(msg) ;

    // Skip LLC and MPLS headers.
    payload = MacSkipLLCandMPLSHeader(node, payload) ;

    IpHeaderType* ipHdr = (IpHeaderType*) payload;
    char* transportHdr = ((char*) ipHdr + (ipHdr->ip_hl * 4));

    return((unsigned short)(*(transportHdr + 2)));
}
#endif

// /**
// FUNCTION     :: MacDot16CheckForValidAddress.
// PURPOSE      :: check whether the node specified in the input file for
//                 traffic conditioning is valid or not.
// PARAMETERS   ::
// + node : Node* : pointer to the node,
// + inputString : char* : the input file.
// + addressString : char* : the source address string.
// + nodeId : NodeAddress* : nodeId of the source Node.
// + address : NodeAddress* : address of the source node.
// RETURN     :: void : NULL
// **/
static
void MacDot16CheckForValidAddress(
    Node* node,
    const char* inputString,
    const char* addressString,
    NodeAddress* nodeId,
    NodeAddress* address)
{
    BOOL  isNodeId = FALSE;

    IO_ParseNodeIdOrHostAddress(addressString, nodeId, &isNodeId);

    if (!isNodeId)
    {
        *address = *nodeId;

        *nodeId = MAPPING_GetNodeIdFromInterfaceAddress(node, *address);

        if (*nodeId == INVALID_MAPPING)
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "%s is not a valid nodeId or Address\n"
                    "\ton line:%s\n",
                    addressString, inputString);

            ERROR_ReportError(errorStr);
        }
    }
    else
    {
        *address = MAPPING_GetDefaultInterfaceAddressFromNodeId(
                                                        node, *nodeId);

        if (*address == INVALID_MAPPING)
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "%s is not a valid nodeId or Address\n"
                    "\ton line:%s\n",
                    addressString, inputString);

            ERROR_ReportError(errorStr);
        }
    }
}

#if 0
// /**
// FUNCTION     :: MacDot16Marker
// PURPOSE      :: It will mark the packet.
// PARAMETERS:
// + preference : int : packet will be marked according this field.
// + serviceClass : MacDot16TcStationInfo*  : service class to use
// + msg : Message*  : pointer to the message,
// RETURN     :: void : NULL
// **/
static
void MacDot16Marker(
    Node* node,
    int preference,
    MacDot16TcStationInfo* stationClassInfo,
    Message* msg)
{
    // retrive information from the packet headers
    char* payload = MESSAGE_ReturnPacket(msg);
    // Skip LLC and MPLS headers
    payload = MacSkipLLCandMPLSHeader(node, payload);
    IpHeaderType* ipHdr = (IpHeaderType*) payload;

    switch (preference)
    {
        case DOT16_TC_CONFORMING:
        {
            IpHeaderSetTOS(&(ipHdr->ip_v_hl_tos_len),
                DOT16_TC_SET_TOS_FIELD(
                stationClassInfo->dsToMarkOutProfilePackets,
                IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len)));
            break;
        }
        case DOT16_TC_PART_CONFORMING:
        {
            ERROR_Assert(stationClassInfo->stationClass!= DOT16_SERVICE_UGS,
                "Not possible to mark PART-CONFORM packet for UGS Service");

            IpHeaderSetTOS(&(ipHdr->ip_v_hl_tos_len),
                DOT16_TC_SET_TOS_FIELD(
                stationClassInfo->dsToMarkOutProfilePackets + 2,
                IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len)));
            break;
        }
        case DOT16_TC_NON_CONFORMING:
        {
            ERROR_Assert(stationClassInfo->stationClass !=
                DOT16_SERVICE_UGS,
                "Not possible to mark OUT-PROFILE packet for UGS Service");

            IpHeaderSetTOS(&(ipHdr->ip_v_hl_tos_len),
                DOT16_TC_SET_TOS_FIELD(
                stationClassInfo->dsToMarkOutProfilePackets + 4,
                IpHeaderGetTOS(ipHdr->ip_v_hl_tos_len)));
            break;
        }
        default:
        {
            ERROR_ReportError("Invalid switch value");
        }
    } //End switch
}


// /**
// FUNCTION     :: MacDot16PacketFitsTokenBucketProfile.
// PURPOSE      :: determin in which color the incoming packet is marked by
//                 the Token Bucket policer.
// PARAMETERS   ::
// + node : Node* : pointer to the node,
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + msg : Message* : pointer to the message,
// + stationClassName -  the class to which this packet belongs.
// + sfDirection : MacDot16ServiceFlowDirection : direction of the
//                                                service flow
// + preference : int* : preference
// RETURN       ::  BOOL :  true of false.
// **/
static
BOOL MacDot16PacketFitsTokenBucketProfile(
    Node* node,
    MacDot16TcData* tc,
    Message* msg,
    char* stationClassName,
    MacDot16ServiceFlowDirection sfDirection,
    int* preference)
{
    Int32 tokensGeneratedSinceLastUpdate = 0;
    int packetSize = MESSAGE_ReturnPacketSize(msg);
    MacDot16TcStationInfo* stationInfo =
        MacDot16ReturnStationInfoByName(tc, stationClassName);
    BOOL retVal = TRUE;

    ERROR_Assert(stationInfo, "Unable to locate conditionClass");

    if(sfDirection == DOT16_DOWNLINK_SERVICE_FLOW ) {
        tokensGeneratedSinceLastUpdate =
            (Int32)((stationInfo->dlRate * (node->getNodeTime()
                - stationInfo->dlLastTokenUpdateTime)) / SECOND);

        stationInfo->dlTokens =
            MIN(stationInfo->dlTokens + tokensGeneratedSinceLastUpdate,
                stationInfo->dlCommittedBurstSize);

         // Note the last update time.
         stationInfo->dlLastTokenUpdateTime = node->getNodeTime();

        if (DOT16_TC_DEBUG_METER)
        {
            printf("\tBefore DL Metering(TB) for ConditionClass:%s\n"
                "\tPacketSize = %u, numOfTokens = %d\n",
                stationClassName, packetSize, stationInfo->dlTokens);
        }

         // Are there enough tokens to admit this packet?
         if (stationInfo->ulTokens >= packetSize)
         {
             // This packet is In-Profile: subtract the tokens from the
             // bucket and return TRUE for In-Profile
             *preference = DOT16_TC_CONFORMING;
             stationInfo->numPacketsConforming++;
             stationInfo->dlTokens -= packetSize;
        }
        else
        {
            // This packet is Out-Profile: return FALSE for Out-Profile
            *preference = DOT16_TC_NON_CONFORMING;
            stationInfo->numPacketsNonConforming++;
            retVal = FALSE;
        }

        if (DOT16_TC_DEBUG_METER)
        {
            printf("\tAfter DL Metering(TB) for ConditionClass:%s\n"
                "\tPacketSize=%u, numOfTokens = %d and preference=%u\n",
                stationClassName, packetSize, stationInfo->dlTokens,
               *preference);
        }


    }
    else {
        tokensGeneratedSinceLastUpdate =
            (Int32)((stationInfo->ulRate * (node->getNodeTime()
                - stationInfo->ulLastTokenUpdateTime)) / SECOND);

        stationInfo->ulTokens =
            MIN(stationInfo->ulTokens + tokensGeneratedSinceLastUpdate,
                stationInfo->ulCommittedBurstSize);

         // Note the last update time.
         stationInfo->ulLastTokenUpdateTime = node->getNodeTime();

        if (DOT16_TC_DEBUG_METER)
        {
            printf("\tBefore UL Metering(TB) for ConditionClass:%s\n"
                "\tPacketSize = %u, numOfTokens = %d\n",
                stationClassName, packetSize, stationInfo->ulTokens);
        }

         // Are there enough tokens to admit this packet?
         if (stationInfo->ulTokens >= packetSize)
         {
             // This packet is In-Profile: subtract the tokens from the
             // bucket and return TRUE for In-Profile
             *preference = DOT16_TC_CONFORMING;
             stationInfo->numPacketsConforming++;
             stationInfo->ulTokens -= packetSize;
        }
        else
        {
            // This packet is Out-Profile: return FALSE for Out-Profile
            *preference = DOT16_TC_NON_CONFORMING;
            stationInfo->numPacketsNonConforming++;
            retVal = FALSE;
        }

        if (DOT16_TC_DEBUG_METER)
        {
            printf("\tAfter Metering(TB) for ConditionClass:%s\n"
                "\tPacketSize=%u, numOfTokens = %d and preference=%u\n",
                stationClassName, packetSize, stationInfo->ulTokens,
               *preference);
        }

    }

    return retVal;
}
#endif

// /**
// FUNCTION     :: MacDot16GetNextServiceRecordClass.
// PURPOSE      :: initializing and return next service class record.
// PARAMETERS   ::
// + node : Node* : pointer to the node,
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + tos : TosType ; type of service value,
// RETURN       :: MacDot16TcInfo* : next empty station class record.
// **/
static
MacDot16TcInfo* MacDot16GetNextServiceClassRecord(
    Node* node,
    MacDot16TcData* tc,
    TosType tos)
{

    MacDot16TcInfo* tcInfo =
        MacDot16ReturnServiceInfo(tc, tos);

    if (tcInfo)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "Already there is an entry for the "
            "tos '%d'\n",tos);
        ERROR_ReportError(errorStr);
        return NULL;
    }

    if (tc->numTcInfo == 0 || tc->numTcInfo == tc->maxTcInfo)
    {
        // Allocate the memory space for the structure
        MacDot16TcInfo* newTcInfo = NULL;

        tc->maxTcInfo += DOT16_NUM_INITIAL_TC_INFO_ENTRIES;
        newTcInfo = (MacDot16TcInfo *) MEM_malloc(
                          sizeof(MacDot16TcInfo) *
                          tc->maxTcInfo);

        // Set the memory space to ZERO.
        memset(newTcInfo, 0, sizeof(MacDot16TcInfo) *
            tc->maxTcInfo);

        if (tc->numTcInfo != 0)
        {
            memcpy(newTcInfo, tc->tcInfo,
                sizeof(MacDot16TcInfo) *
                tc->numTcInfo);
            MEM_free(tc->tcInfo);
        } //End if

        tc->tcInfo = newTcInfo;
    } //End if

   tc->numTcInfo++;

   return &(tc->tcInfo[tc->numTcInfo-1]);

}

// /**
// FUNCTION     :: MacDot16GetNextStationClassRecord.
// PURPOSE:     :: initializing and return next station class record.
// PARAMETERS:  ::
// + node : Node* : pointer to the node,
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + conditionClass : char* : condition class value,
// RETURN      :: MacDot16TcStationInfo* : next empty station class record.
// **/
static
MacDot16TcStationInfo* MacDot16GetNextStationClass(
    Node* node,
    MacDot16TcData* tc,
    char* stationClassString)
{
    MacDot16TcStationInfo* stationInfo =
        MacDot16ReturnStationInfoByName(tc, stationClassString);

    // Allocate the memory space for the structure
    MacDot16TcStationInfo* newTcStationInfo = NULL;

    if (stationInfo)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "Already there is an entry for the "
            "condition class '%s'\n",stationClassString);
        ERROR_ReportError(errorStr);
    }

    if (tc->numTcStationInfo == 0 || tc->numTcStationInfo ==
        tc->maxTcStationInfo)
    {

        tc->maxTcStationInfo += DOT16_NUM_INITIAL_TC_INFO_ENTRIES;
        newTcStationInfo = (MacDot16TcStationInfo *) MEM_malloc(
                          sizeof(MacDot16TcStationInfo) *
                          tc->maxTcStationInfo);

        // Set the memory space to ZERO.
        memset(newTcStationInfo, 0, sizeof(MacDot16TcStationInfo) *
            tc->maxTcStationInfo);

        if (tc->numTcStationInfo != 0)
        {
            memcpy(newTcStationInfo, tc->tcStationInfo,
                sizeof(MacDot16TcStationInfo) *
                tc->numTcInfo);
            MEM_free(tc->tcStationInfo);

        } //End if
        tc->tcStationInfo = newTcStationInfo;

    }

   tc->numTcStationInfo++;

   return &(tc->tcStationInfo[tc->numTcStationInfo-1]);

}


// /**
// FUNCTION     :: MacDot16AddStationClassToProfileMapping.
// PURPOSE      :: initializing the parameters for profiling traffic.
// PARAMETERS   ::
// + node : Node* : pointer to the node,
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + conditionClass : char* :  condition class value,
// + packetProfile : MacDot16TcConditionType : packet profile type
// + tosToMark : tos : tos to remark
// RETURN     :: void : NULL
// **/
static
void MacDot16AddStationClassToProfileMapping(
    Node* node,
    MacDot16TcData* tc,
    char* stationClassName,
    MacDot16TcConditionType packetProfile,
    int tosToMark)
{
    MacDot16TcStationInfo* stationInfo = NULL;
    stationInfo = MacDot16ReturnStationInfoByName(tc, stationClassName);

    if (!stationInfo)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "There is no entry for the condition class %s\n"
            "Cannot specify an out-of-profile action for nonexistent "
            "class",
            stationClassName);
        ERROR_ReportError(errorStr);
    }
    stationInfo->packetProfile = packetProfile;
    stationInfo->dsToMarkOutProfilePackets = (unsigned char) tosToMark;

    if (DOT16_TC_DEBUG_INIT)
    {
        printf("Into node %u Station Class:%s, profile:%d\n",
            node->nodeId, stationClassName,
            stationInfo->packetProfile);
    }

}


// /**
// FUNCTION     :: MacDot16AddTcParameterInfo
// PURPOSE      :: adding the tc parameter structure information
//
// PARAMETERS   ::
// + node :  Node* : pointer to the node
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + srcNode : NodeAddress : address of the source node
// + destNode NodeAddress : address of destination node
// + tos : TosType : the tos field
// + protocolId : unsigned char : protocol for which the packet is meant
// + srcPort : short : source port of the packet
// + destPort : short :  destination port of the application
// + sfDirection : MacDot16ServiceFlowDirection : service flow direction
// + conditionClass : int : condition class
// RETURN     :: void : NULL
// **/
static
void MacDot16AddTcParameterInfo(
    Node* node,
    MacDot16TcData* tc,
    NodeAddress srcNodeId,
    NodeAddress destNodeAddress,
    TosType tos,
    unsigned char protocolId,
    short srcPort,
    short destPort,
    MacDot16ServiceFlowDirection sfDirection,
    int conditionClass)
{

    int i = 0;

    // Check for this combination is previously exsit or not.
    for (i = 0; i < tc->numTcParameters; i++)
    {

        if (((tc->tcParameter[i].sourceNodeId ==
            DOT16_TC_UNUSED_FIELD_ADDRESS) ||
            (srcNodeId == DOT16_TC_UNUSED_FIELD_ADDRESS) ||
            (tc->tcParameter[i].sourceNodeId == srcNodeId)) &&

            ((tc->tcParameter[i].destAddress ==
            DOT16_TC_UNUSED_FIELD_ADDRESS) ||
            (destNodeAddress == DOT16_TC_UNUSED_FIELD_ADDRESS) ||
            (tc->tcParameter[i].destAddress == destNodeAddress)) &&

            ((tc->tcParameter[i].tos == DOT16_TC_UNUSED_FIELD) ||
            (tos == DOT16_TC_UNUSED_FIELD) ||
            (tc->tcParameter[i].tos == tos)) &&

            ((tc->tcParameter[i].protocolId == DOT16_TC_UNUSED_FIELD) ||
            (protocolId == DOT16_TC_UNUSED_FIELD) ||
            (tc->tcParameter[i].protocolId == protocolId)) &&

            ((tc->tcParameter[i].sourcePort == DOT16_TC_UNUSED_FIELD) ||
            (srcPort == DOT16_TC_UNUSED_FIELD) ||
            (tc->tcParameter[i].sourcePort == srcPort)) &&

            ((tc->tcParameter[i].destPort == DOT16_TC_UNUSED_FIELD) ||
            (destPort == DOT16_TC_UNUSED_FIELD) ||
            (tc->tcParameter[i].destPort == destPort)) &&

            ((tc->tcParameter[i].sfDirection ==
            DOT16_TC_UNUSED_FIELD) ||
            (sfDirection == DOT16_TC_UNUSED_FIELD) ||
            (tc->tcParameter[i].sfDirection ==
            sfDirection)))
        {
                ERROR_ReportError("You have two traffic classes with "
                    "identical identifying characteristics");
        } //End if
    } //End for

    if (tc->numTcParameters == 0 ||
        tc->numTcParameters == tc->maxTcParameters)
    {
        // Allocate the memory space for the structure TcParameter.
        MacDot16TcParameter* newTcParameter =
            (MacDot16TcParameter*)
            MEM_malloc(sizeof(MacDot16TcParameter) * (tc->maxTcParameters
                + DOT16_TC_NUM_INITIAL_MFTC_PARA_ENTRIES));
        // Set the memory space to ZERO.
        memset(newTcParameter,
               0,
               sizeof(MacDot16TcParameter) * tc->maxTcParameters);
        tc->maxTcParameters += DOT16_TC_NUM_INITIAL_MFTC_PARA_ENTRIES;

        if (tc->numTcParameters != 0)
        {
            // Copy the old data of TcParameter to newTcParameter
            memcpy(newTcParameter,
                   tc->tcParameter,
                   sizeof(MacDot16TcParameter) * tc->numTcParameters);
            MEM_free(tc->tcParameter);
        } //End if
        tc->tcParameter = newTcParameter;
    } //End if

    // Initializing the class definition parameters.
    i = tc->numTcParameters;
    tc->tcParameter[i].sourceNodeId = srcNodeId;
    tc->tcParameter[i].destAddress = destNodeAddress;
    tc->tcParameter[i].tos = tos;
    tc->tcParameter[i].protocolId = protocolId;
    tc->tcParameter[i].sourcePort = srcPort;
    tc->tcParameter[i].destPort = destPort;
    tc->tcParameter[i].sfDirection = sfDirection;
    tc->tcParameter[i].conditionClass = conditionClass;
    tc->numTcParameters++;
}


// /**
// FUNCTION     :: MacDot16TcClassifierParameterInit
// PURPOSE:     :: parse the inputString for classifier parameter
//                 initialization.
// PARAMETERS   ::
// + node : Node* : pointer to the node
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + inputString : char* : information string to parse
// RETURN     :: void : NULL
// **/
static
void MacDot16TcClassifierParameterInit(
    Node* node,
    MacDot16TcData* tc,
    const char* inputString)
{
    char srcNodeString[MAX_ADDRESS_STRING_LENGTH];
    char destNodeString[MAX_ADDRESS_STRING_LENGTH];
    char dsString[MAX_ADDRESS_STRING_LENGTH];
    char protocolIdString[MAX_ADDRESS_STRING_LENGTH];
    char srcPortString[MAX_ADDRESS_STRING_LENGTH];
    char destPortString[MAX_ADDRESS_STRING_LENGTH];
    char sfDirectionString[MAX_ADDRESS_STRING_LENGTH];
    char conditionClassString[MAX_ADDRESS_STRING_LENGTH];

    NodeAddress   sourceNodeId = 0;
    NodeAddress   destNodeId = 0;
    NodeAddress   sourceAddr = 0;
    NodeAddress   destAddr = 0;
    unsigned char ds = 0;
    unsigned char protocolId = 0;
    short         srcPort = 0;
    short         destPort = 0;
    int           sfDirection = 0;
    int           conditionClass = 0;
    int           items = 0;

    items = sscanf(inputString, "%*s %s %s %s %s %s %s %s %s",
                srcNodeString, destNodeString, dsString,
                protocolIdString, srcPortString, destPortString,
                sfDirectionString, conditionClassString);

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
        sourceNodeId = DOT16_TC_UNUSED_FIELD_ADDRESS;
    }
    else
    {
        MacDot16CheckForValidAddress(
            node,
            inputString,
            srcNodeString,
            &sourceNodeId,
            &sourceAddr);
    }

    if (strcmp(destNodeString, "ANY") == 0)
    {
        destAddr = DOT16_TC_UNUSED_FIELD_ADDRESS;
    }
    else
    {
        MacDot16CheckForValidAddress(
            node,
            inputString,
            destNodeString,
            &destNodeId,
            &destAddr);
    }

    if (strcmp(dsString, "ANY") == 0)
    {
        ds = DOT16_TC_UNUSED_FIELD;
    }
    else
    {
        ds = (unsigned char)atoi(dsString);
        // checking for ds field.
        if ((ds > IPTOS_DSCP_MAX) || (ds < IPTOS_DSCP_MIN + 1 ))
        {
            ERROR_ReportError(
                "The DS field must be a positive integer <= 63");
        }
    }

    if (strcmp(protocolIdString, "ANY") == 0)
    {
        protocolId = DOT16_TC_UNUSED_FIELD;
    }
    else
    {
        protocolId = (unsigned char)atoi(protocolIdString);
    }

    if (strcmp(srcPortString, "ANY") == 0)
    {
        srcPort = DOT16_TC_UNUSED_FIELD;
    }
    else
    {
        srcPort = (short)atoi(srcPortString);
    }

    if (strcmp(destPortString, "ANY") == 0)
    {
        destPort = DOT16_TC_UNUSED_FIELD;
    }
    else
    {
        destPort = (short)atoi(destPortString);
    }

    if (strcmp(sfDirectionString, "ANY") == 0)
    {
        sfDirection = DOT16_TC_UNUSED_FIELD;
    }
    else
    {

        if (strcmp(destPortString, "UPLINK") == 0)
        {
            sfDirection = DOT16_UPLINK_SERVICE_FLOW;
        }
        else if (strcmp(destPortString, "DOWNLINK") == 0)
        {
            sfDirection = DOT16_DOWNLINK_SERVICE_FLOW;
        }
        else
        {
            ERROR_ReportError(
                "The service flow direction is invalid");
        }
    }

    conditionClass = atoi(conditionClassString);

    if (DOT16_TC_DEBUG_INIT)
    {
        printf("Into node %u conditionClass:%u\n"
            "\tsourceNodeId:%u destNodeAddress:%u ds:%u protocolId:%u\n"
            "\t\tsrcPort:%u destPort:%u incomingInterface:%u\n",
            node->nodeId, conditionClass,
            sourceNodeId, destAddr, ds, protocolId,
            srcPort, destPort, sfDirection);
    }

    MacDot16AddTcParameterInfo(
        node,
        tc,
        sourceNodeId,
        destAddr,
        ds,
        protocolId,
        srcPort,
        destPort,
        (MacDot16ServiceFlowDirection) sfDirection,
        conditionClass);
}

// /**
// FUNCTION     :: MacDot16TcMeterParameterInit
// PURPOSE:     :: parse the inputString for meter parameter initialization.
// PARAMETERS   ::
// + node : Node* : pointer to the node
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + inputString : char* : information string to parse
// RETURN     :: void : NULL
// **/
static
void MacDot16TcMeterParameterInit(
    Node* node,
    MacDot16TcData* tc,
    const char* inputString)
{
    int  items = 0;
    char stationClassString[20];
    char meterTypeString[20];
    char dlCommittedRateString[20];
    char dlCommittedBurstSizeString[20];
    char ulCommittedRateString[20];
    char ulCommittedBurstSizeString[20];
    MacDot16TcStationInfo* stationInfo = NULL;

    items = sscanf(inputString, "%*s %s %s %s %s %s %s",
                stationClassString, meterTypeString,
                dlCommittedRateString, dlCommittedBurstSizeString,
                ulCommittedRateString, ulCommittedBurstSizeString);

    stationInfo = MacDot16GetNextStationClass(
        node, tc, stationClassString );

    stationInfo->dlLastTokenUpdateTime = node->getNodeTime();
    stationInfo->ulLastTokenUpdateTime = node->getNodeTime();

    if (strcmp(meterTypeString, "TokenBucket") == 0 && items == 6)
    {
        // Initialize the token bucket meter parameters.
        strcpy((char*) stationInfo->stationClassName, stationClassString);
        stationInfo->stationClass = tc->numTcStationInfo;
        stationInfo->dlRate = atol(dlCommittedRateString);
        stationInfo->dlCommittedBurstSize =
                    atol(dlCommittedBurstSizeString);
        stationInfo->ulRate = atol(ulCommittedRateString);
        stationInfo->ulCommittedBurstSize =
                    atol(ulCommittedBurstSizeString);

        //Initially bucket should be full of token.
        stationInfo->dlTokens = stationInfo->dlCommittedBurstSize;
        stationInfo->ulTokens = stationInfo->ulCommittedBurstSize;

        if (DOT16_TC_DEBUG_INIT)
        {
            int i = tc->numTcStationInfo - 1;
            printf("Into node %u station class:%s Meter,TB\n"
                "\tDL-Rate:%u DL-CBS:%u"
                "\tUL-Rate:%u UL-CBS:%u\n",
                node->nodeId, tc->tcStationInfo[i].stationClassName,
                tc->tcStationInfo[i].dlRate,
                tc->tcStationInfo[i].dlCommittedBurstSize,
                tc->tcStationInfo[i].ulRate,
                tc->tcStationInfo[i].ulCommittedBurstSize);
        }
    }
    else
    {
        ERROR_Assert(FALSE, "QOS-METER: "
          "Specification of Meter is invalid\n");
    }
}


// /**
// FUNCTION     :: MacDot16TcConditionerParameterInit
// PURPOSE:     :: parse the inputString for conditioner parameter
//                 initialization.
// PARAMETERS   ::
// + node : Node* : pointer to the node
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + inputString : char* : information string to parse
// RETURN     :: void : NULL
// **/
static
void MacDot16TcConditionerParameterInit(
    Node* node,
    MacDot16TcData* tc,
    const char* inputString)
{

    // MARKING/DROPPING/HOLDING Routines.
    char  conditionClassName[20];
    char  outProfileAction[20];
    int   dscpToMark = 0;
    int   items = 0;
    MacDot16TcConditionType  profile = DOT16_TC_CONDITION_HOLD;

    items = sscanf(inputString,
                "%*s %s %s %d",
                conditionClassName,
                outProfileAction,
                &dscpToMark);

    if (strcmp(outProfileAction, "MARK") == 0)
    {
        profile = DOT16_TC_CLASSIFIER_MARK;

        if (items != 3)
        {
            ERROR_ReportError("\"QOS-CONDITIONER\" "
                "\"MARK\" requires dscp to be specified");
        }
        else if (dscpToMark < IPTOS_DSCP_MIN + 1 ||
            dscpToMark > IPTOS_DSCP_MAX)
        {
             ERROR_ReportError("\"QOS-CONDITIONER\" "
                "\"MARK\" requires positive dscp <= 63");
        }
    }
    else if (strcmp(outProfileAction, "DROP") == 0)
    {
        profile = DOT16_TC_CLASSIFIER_DROP;

        if (items != 2)
        {
            ERROR_ReportError("\"QOS-CONDITIONER\" "
                "\"DROP\" takes no parameters");
        }
    }
    else if (strcmp(outProfileAction, "HOLD") == 0)
    {
        profile = DOT16_TC_CONDITION_HOLD;

        if (items != 2)
        {
            ERROR_ReportError("\"QOS-CONDITIONER\" "
                "\"HOLD\" takes no parameters");
        }
    }
    else
    {
        // default is to hold
        profile = DOT16_TC_CONDITION_HOLD;
    }

    MacDot16AddStationClassToProfileMapping(
        node,
        tc,
        conditionClassName,
        profile,
        dscpToMark);

}

// /**
// FUNCTION     :: MacDot16TcServiceParameterInit
// PURPOSE:     :: parse the inputString for service class parameter
//                 initialization.
// PARAMETERS   ::
// + node : Node* : pointer to the node
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + inputString : char* : information string to parse
// RETURN     :: void : NULL
// **/
static
void MacDot16TcServiceParameterInit(
    Node* node,
    MacDot16TcData* tc,
    const char* inputString)
{

    TosType tos = 0;
    char  serviceClassString[DOT16_TC_MAX_STATION_CLASS_NAME];
    int   items = 0;
    MacDot16TcInfo* serviceClassInfo = NULL;

    items = sscanf(inputString,
                "%*s %d %s",
                &tos,
                serviceClassString);

    // not currently remapping
    //            tos =
    //                MacDot16ReturnServiceClass(tos);

    serviceClassInfo = MacDot16GetNextServiceClassRecord( node, tc, tos );

    if (!serviceClassInfo)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr,
            "There is no entry for the tos %d\n"
            "Cannot specify an Service Class for nonexistent "
            "class",
            tos);
        ERROR_ReportError(errorStr);
    }

    if (strcmp(serviceClassString, "UGS") == 0)
    {
        serviceClassInfo->serviceClass = DOT16_SERVICE_UGS;
        strcpy(serviceClassInfo->serviceClassName, "UGS");
    }
    else if (strcmp(serviceClassString, "rtPS") == 0 )
    {
        serviceClassInfo->serviceClass = DOT16_SERVICE_rtPS;
        strcpy(serviceClassInfo->serviceClassName, "rtPS");
    }
    else if (strcmp(serviceClassString, "ErtPS") == 0 )
    {
        serviceClassInfo->serviceClass = DOT16_SERVICE_ertPS;
        strcpy(serviceClassInfo->serviceClassName, "ErtPS");
    }
    else if (strcmp(serviceClassString, "nrtPS") == 0)
    {
        serviceClassInfo->serviceClass = DOT16_SERVICE_nrtPS;
        strcpy(serviceClassInfo->serviceClassName, "nrtPS");
    }
    else if (strcmp(serviceClassString, "BE") == 0)
    {
        serviceClassInfo->serviceClass = DOT16_SERVICE_BE;
        strcpy(serviceClassInfo->serviceClassName, "BE");
    }
    else
    {
        ERROR_ReportError("The Service Class is invalid"
            "it should be one of these: UGS, rtPS, nrtPS, or BE");
    }

    serviceClassInfo->tos = tos;

}

// /**
// FUNCTION     :: MacDot16ReturnConditionClassForPacket
// PURPOSE:     :: returns condition class to which a particular packet
//                 belongs.
// PARAMETERS:
// + node : Node* : pointer to the node,
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + msg : Message* : application packet or fragments.
// + sfDirection : MacDot16ServiceFlowDirection : service flow direction
// + conditionClass : int : condition class
// + foundMatchingConditionClass : BOOL* : found a match
// RETURN  : int : condition class to which the packet belongs or NULL.
// **/
/*
static
int MacDot16ReturnConditionClassForPacket(
    Node* node,
    MacDot16TcData* tc,
    Message* msg,
    MacDot16ServiceFlowDirection sfDirection,
    BOOL* foundMatchingConditionClass)
{
// Note: In future if used this function, update the code for ARP also



    int i = 0;
    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    NodeAddress pktSourceAddress = ipHdr->ip_src;
    NodeAddress pktDestAddress = ipHdr->ip_dst;
    unsigned char pktDSCode = (unsigned char)(ipHdr->ip_tos >> 2);
    unsigned char pktProtocolId = ipHdr->ip_p;
    // The following code has been changed for supporting
    // MPLS + 802.16
    unsigned short pktSourcePort = MacDot16ReturnSourcePort(node, msg);
    unsigned short pktDestPort = MacDot16ReturnDestPort(node, msg);

    if (DOT16_TC_DEBUG_MARKER)
    {
        printf("Into node %u Packet's \n"
            "\tSrcAdd:%u, DestAdd:%u, DSCode:%u, Protocol:%u, "
            "SrcPort:%u, DestPort:%u, Direction:%s\n",
            node->nodeId, pktSourceAddress, pktDestAddress, pktDSCode,
            pktProtocolId, pktSourcePort, pktDestPort,
            sfDirection == DOT16_DOWNLINK_SERVICE_FLOW ? "Down" : "Up");
    }

    for (i = 0; i < tc->numTcParameters; i++)
    {
        if (((tc->tcParameter[i].sourceNodeId ==
            DOT16_TC_UNUSED_FIELD_ADDRESS) ||
            (tc->tcParameter[i].sourceNodeId ==
            MAPPING_GetNodeIdFromInterfaceAddress(
                node, pktSourceAddress))) &&

            ((tc->tcParameter[i].destAddress ==
            DOT16_TC_UNUSED_FIELD_ADDRESS) ||
            (tc->tcParameter[i].destAddress == pktDestAddress)) &&

            ((tc->tcParameter[i].tos == DOT16_TC_UNUSED_FIELD)
            || (tc->tcParameter[i].tos == pktDSCode)) &&

            ((tc->tcParameter[i].protocolId == DOT16_TC_UNUSED_FIELD)
            || (tc->tcParameter[i].protocolId == pktProtocolId)) &&

            ((tc->tcParameter[i].sourcePort == DOT16_TC_UNUSED_FIELD)
            || (tc->tcParameter[i].sourcePort == pktSourcePort)) &&

            ((tc->tcParameter[i].destPort == DOT16_TC_UNUSED_FIELD)
            || (tc->tcParameter[i].destPort == pktDestPort)) &&

            ((node->nodeId == MAPPING_GetNodeIdFromInterfaceAddress(
                node, pktSourceAddress))
            || ((tc->tcParameter[i].sfDirection ==
                DOT16_TC_UNUSED_FIELD)
            || (tc->tcParameter[i].sfDirection ==
                sfDirection))))
        {
            *foundMatchingConditionClass = TRUE;
            return tc->tcParameter[i].conditionClass;
        }
    }

    return 0;
}
*/

// /**
// FUNCTION     :: MacDot16TcFileInit
// PURPOSE      :: initialize the classifiers/services from the qos file
// PARAMETERS   ::
// + node : Node* : pointer to the node,
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + nodeInput : const NodeInput* : application packet or fragments.
//              nodeInput - Configuration Information.
// RETURN     :: void : NULL
// **/
void MacDot16TcFileInit(
    Node* node,
    MacDot16TcData* tc,
    const NodeInput* tcInput)
{

    for (int tmpCounter = 0; tmpCounter < tcInput->numLines; tmpCounter++)
    {
        char identifier[MAX_STRING_LENGTH];

        sscanf(tcInput->inputStrings[tmpCounter], "%s", identifier);

        if (strcmp(identifier, "QOS-CLASSIFIER") == 0)
        {
            MacDot16TcClassifierParameterInit(
                node, tc, tcInput->inputStrings[tmpCounter]);
        }
        else if (strcmp(identifier, "QOS-METER") == 0)
        {
            MacDot16TcMeterParameterInit(
                node, tc, tcInput->inputStrings[tmpCounter]);
        }
        else if (strcmp(identifier, "QOS-CONDITIONER") == 0)
        {
            MacDot16TcConditionerParameterInit(
                node, tc, tcInput->inputStrings[tmpCounter]);
        }
        else if (strcmp(identifier, "QOS-SERVICE") == 0)
        {
            MacDot16TcServiceParameterInit(
                node, tc, tcInput->inputStrings[tmpCounter]);
        }

    } // for

}


// /**
// FUNCTION     :: MacDot16TcDefaultInit
// PURPOSE      :: initialize the classifiers/services
// PARAMETERS   ::
// + node : Node* : pointer to the node,
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + nodeInput : const NodeInput* : application packet or fragments.
//              nodeInput - Configuration Information.
// RETURN     :: void : NULL
// **/
void MacDot16TcDefaultInit(
    Node* node,
    MacDot16TcData* tc)
{

    int tmpCounter = 0;
    char identifier[MAX_STRING_LENGTH];

    while (TRUE)
    {
        sscanf(dot16QosDefaults[tmpCounter], "%s", identifier);

        if (strcmp(identifier, "QOS-CLASSIFIER") == 0)
        {
            MacDot16TcClassifierParameterInit(
                node, tc, dot16QosDefaults[tmpCounter]);
        }
        else if (strcmp(identifier, "QOS-METER") == 0)
        {
            MacDot16TcMeterParameterInit(
                node, tc, dot16QosDefaults[tmpCounter]);
        }
        else if (strcmp(identifier, "QOS-CONDITIONER") == 0)
        {
            MacDot16TcConditionerParameterInit(
                node, tc, dot16QosDefaults[tmpCounter]);
        }
        else if (strcmp(identifier, "QOS-SERVICE") == 0)
        {
            MacDot16TcServiceParameterInit(
                node, tc, dot16QosDefaults[tmpCounter]);
        }

        if (dot16QosDefaults[++tmpCounter] == NULL)
            break;
    }

}


// /**
// FUNCTION   :: MacDot16TcInit
// LAYER      :: MAC
// PURPOSE    :: Initialize DOT16 traffic conditioner at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16TcInit(
        Node* node,
        int interfaceIndex,
        const NodeInput* nodeInput)
{

    int i = 0;

    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;

    // init traffic conditioner struct
    MacDot16TcData* dot16Tc =
            (MacDot16TcData*) MEM_malloc(sizeof(MacDot16TcData));
    ERROR_Assert(dot16Tc != NULL,
                 "MAC 802.16: Out of memory!");

    // using memset to initialize the whole DOT16 TC data strucutre
    memset((char*)dot16Tc, 0, sizeof(MacDot16TcData));

    dot16->tcData = (void*) dot16Tc;

/* Disabled as it is not competed yet.
    IO_ReadCachedFile(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "DOT16-QOS-FILE",
        &retVal,
        &tcInput);

    if (retVal == TRUE)
    {
        // read the traffic conditoner file
        MacDot16TcFileInit(node, dot16Tc, &tcInput);
    }
    else
    {
*/
        // read the traffic conditoner defaults
        MacDot16TcDefaultInit(node, dot16Tc);
//    }

    // check for station classes
    while (i < dot16Tc->numTcStationInfo)
    {
        if (!strlen(dot16Tc->tcStationInfo[i].stationClassName))
        {
            char errorMsg[MAX_STRING_LENGTH];
            sprintf(errorMsg, "Agreement is incomplete, Station Class %s "
                "has no Station Class Name!\n",
                dot16Tc->tcStationInfo[i].stationClassName);
            ERROR_ReportError(errorMsg);
        }
        i++;
    }

    // enable stats
    dot16Tc->tcStatisticsEnabled = TRUE;

}

// /**
// FUNCTION     :: MacDot16TcGetServiceClass.
// PURPOSE      :: return the service class for the given tos.
// PARAMETERS:
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN : MacDot16ServiceType : return service class or zero (BE).
// **/

MacDot16ServiceType MacDot16TcGetServiceClass(
        Node* node,
        MacDataDot16* dot16,
        TosType tos)
{

    MacDot16TcData* tcData = (MacDot16TcData*) dot16->tcData;
    MacDot16ServiceType sc = DOT16_SERVICE_BE;
    MacDot16TcInfo* tc = NULL;

    tc = MacDot16ReturnServiceInfo(
            tcData,
            tos);

    if ( tc != NULL )
    {
        sc = tc->serviceClass;
    }

    return sc;

}

// /**
// FUNCTION     :: MacDot16TcProfilePacketAndPassOrDrop
// PURPOSE      :: analyze incomming packet to determine whether or not it
//                 is in/out-profile, and either pass or drop the packet
//                 as indicated by the class of station and policy defined
//                 in the configuration file.
// PARAMETERS   ::
// + node : Node* : pointer to the node,
// + tc : MacDot16TcData* : pointer to the traffic conditioner state
//                          variables
// + msg : Message* : application packet or fragments.
// + stationClassName : char* : condition class
// + sfDirection : MacDot16ServiceFlowDirection : service flow direction
// + packetWasDropped : BOOL* : packet was droped
//                   TRUE if packet was droped,
//                   FALSE otherwise (even if the packet was remarked)
// RETURN     :: void : NULL
// **/
/*
void MacDot16TcProfilePacketAndPassOrDrop(
    Node* node,
    MacDot16TcData* tc,
    Message* msg,
    char* stationClassName,
    MacDot16ServiceFlowDirection sfDirection,
    BOOL* packetWasDropped)
{
    MacDot16TcStationInfo* stationClassInfo = NULL;
    BOOL foundMatchingConditionClass = FALSE;
    IpHeaderType* ipHdr = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    BOOL isConforming = FALSE;
    int preference = 0;
    int conditionClass = MacDot16ReturnConditionClassForPacket(
                            node,
                            tc,
                            msg,
                            sfDirection,
                            &foundMatchingConditionClass);

    stationClassInfo =
        MacDot16ReturnStationInfoByName(tc, stationClassName);

    if (DOT16_TC_DEBUG_MARKER)
    {
        printf("\tReturn ConditionClass:%u\n", conditionClass);
    }

    // Check the emptiness of the interface's output queue(s) before
    // attempting to queue packet.
    *packetWasDropped = FALSE;

    if ((ipHdr->ip_tos == IPTOS_PREC_INTERNETCONTROL) ||
        (ipHdr->ip_tos == IPTOS_PREC_NETCONTROL))
    {
        // Packet with precedence 6 | 7
        return;
    }

    if (foundMatchingConditionClass && stationClassInfo)
    {
        stationClassInfo->numPacketsIncoming++;

        isConforming = MacDot16PacketFitsTokenBucketProfile(
                            node,
                            tc,
                            msg,
                            (char*) stationClassInfo->stationClassName,
                            sfDirection,
                            &preference);

        // Conforming means conforming or partial conforming packet.
        if (isConforming)
        {
            // Call the Marker for marking the packet
            MacDot16Marker(node, preference, stationClassInfo, msg);

            if (DOT16_TC_DEBUG_MARKER)
            {
                printf("\t\tPacket is In-Profile and can be transmitted\n");
            }
        }
        else
        {
            if (DOT16_TC_DEBUG_MARKER)
            {
                printf("\t\tPacket is Out-Of-Profile Action :%s\n",
                       stationClassInfo->packetProfile);
            }

            // Action for non-conforming packets
            // Drop, if specified
            // Default, as indicated by meter condition
            // Remark, if user specified
            if (stationClassInfo->packetProfile == DOT16_TC_CLASSIFIER_DROP)
            {
                *packetWasDropped = TRUE;
            }
            else if (stationClassInfo->dsToMarkOutProfilePackets == 0)
            {
                MacDot16Marker(node, preference, stationClassInfo, msg);
            }
            else
            {
                // Changed to use 6 bit DS field
                ipHdr->ip_tos = DOT16_TC_SET_TOS_FIELD(
                    stationClassInfo->dsToMarkOutProfilePackets,
                    ipHdr->ip_tos);
            } //End if
        } //End if
    } //End if
    else
    {
        ipHdr->ip_tos =
            DOT16_TC_SET_TOS_FIELD(DOT16_DS_CLASS_BE, ipHdr->ip_tos);
    }

    if (DOT16_TC_DEBUG_MARKER)
    {
        printf("\tAfter Marking, in node %u the value of DS Field=%u\n",
            node->nodeId, (ipHdr->ip_tos >> 2));
    }
}
*/

// /**
// FUNCTION     :: MacDot16TcFinalize(Node* node)
// PURPOSE      :: print out statistics
// PARAMETERS   ::
// + node : Node* : this node
// + int  : interfaceIndex : index to the mac interface
// RETURN     :: void : NULL
// **/

void MacDot16TcFinalize(
    Node* node,
    int interfaceIndex)
{

    MacDataDot16* dot16 = (MacDataDot16 *)
                          node->macData[interfaceIndex]->macVar;

    MacDot16TcData* tc = (MacDot16TcData *) dot16->tcData;

    char buf[MAX_STRING_LENGTH];

    if (tc->tcStatisticsEnabled == TRUE)
    {
        int i = 0;
        MacDot16TcStationInfo* stationClassInfo = NULL;

        while (i < tc->numTcStationInfo)
        {
            stationClassInfo = &(tc->tcStationInfo[i]);

            sprintf(buf, "Number of checked packets = %u",
                stationClassInfo->numPacketsIncoming);
            IO_PrintStat(
                node,
                "Mac",
                "802.16",
                ANY_DEST,
                stationClassInfo->stationClass,
                buf);

            sprintf(buf, "Number of conforming packets = %u",
                stationClassInfo->numPacketsConforming);
            IO_PrintStat(
                node,
                "Mac",
                "802.16",
                ANY_DEST,
                stationClassInfo->stationClass,
                buf);

            if (stationClassInfo->packetProfile == DOT16_TC_CLASSIFIER_DROP)
            {
                sprintf(buf, "Number of dropped Packets = %u",
                    stationClassInfo->numPacketsNonConforming);
                IO_PrintStat(
                    node,
                    "Mac",
                    "802.16",
                    ANY_DEST,
                    stationClassInfo->stationClass,
                    buf);
            }
            else
            if ((stationClassInfo->packetProfile ==
                DOT16_TC_CLASSIFIER_MARK) &&
                (stationClassInfo->dsToMarkOutProfilePackets > 0))
            {
                sprintf(buf, "Number of remarked Packets = %u",
                    stationClassInfo->numPacketsNonConforming);
                IO_PrintStat(
                    node,
                    "Mac",
                    "802.16",
                    ANY_DEST,
                    stationClassInfo->stationClass,
                    buf);
            }
            else
            {
                sprintf(buf, "Number of non-conforming packets = %u",
                    stationClassInfo->numPacketsNonConforming);
                IO_PrintStat(
                    node,
                    "Mac",
                    "802.16",
                    ANY_DEST,
                    stationClassInfo->stationClass,
                    buf);
            } //End if

            i++;

        } //End while
    } //End if
}

