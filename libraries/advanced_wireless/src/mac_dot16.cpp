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


// /**
// PROTOCOL   :: MAC802.16
// LAYER      :: MAC
// REFERENCES ::
// + IEEE Std 802.16-2004 : "Part 16: Air Interface for Fixed Broadband
//                           Wireless Access Systems"
// + IEEE Std 802.16-2004 : "Part 16: Air Interface for Fixed Broadband
//                           Wireless Access Systems"
// COMMENTS   :: This is the implementation of the IEEE 802.16 MAC
// **/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"

#include "mac_dot16.h"
#include "mac_dot16_bs.h"
#include "mac_dot16_ss.h"
#include "mac_dot16_cs.h"
#include "mac_dot16_phy.h"
#include "mac_dot16_sch.h"
#include "mac_dot16_qos.h"
#include "mac_dot16_tc.h"
#include "phy_dot16.h"

#define DEBUG              0
#define DEBUG_TIMER        0
#define DEBUG_PARAMETER    0
#define DEBUG_IDLE         0
#define DEBUG_CHANNEL      0
#define DEBUG_PACKET       0
#define DEBUG_FLOW         0
#define DEBUG_PACKING_FRAGMENTATION    0
#define DEBUG_SetTimer 0
#define DEBUG_ARQ                      0// && (node->nodeId == 0)
#define DEBUG_ARQ_INIT                 0
#define DEBUG_ARQ_WINDOW               0
#define DEBUG_ARQ_TIMER                0// && (node->nodeId == 0)

// /**
// FUNCTION   :: MacDot16PrintModulationCodingInfo
// LAYER      :: MAC
// PURPOSE    :: printf out the modulation and coding information
// PARAMETERS ::
// + node  : Node*         : Pointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// RETURN     :: void : NULL
// **/
void MacDot16PrintModulationCodingInfo(Node* node,
                                       MacDataDot16* dot16)
{
    MacDot16DlBurstProfile* dlBurstProfile;
    int diuc;
    char intfStr[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(
        MAPPING_GetInterfaceAddressForInterface(
            node,
            node->nodeId,
            dot16->myMacData->interfaceIndex), intfStr);

    if (dot16->stationType == DOT16_SS)
    {
        MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

        // downlink
        diuc = dot16Ss->servingBs->leastRobustDiuc;
        dlBurstProfile = (MacDot16DlBurstProfile*)
                             &(dot16Ss->servingBs->dlBurstProfile[diuc]);
        printf(" node %d if %s: diuc %d:", node->nodeId, intfStr, diuc);
        if (dlBurstProfile->ofdma.fecCodeModuType == 0)
        {
            printf(" modulation and coding scheme is QPSK_R_1_2\n");
        }
        else if (dlBurstProfile->ofdma.fecCodeModuType == 1)
        {
            printf("modulation and coding scheme is QPSK_R_3_4\n");
        }
        else if (dlBurstProfile->ofdma.fecCodeModuType == 2)
        {
            printf("modulation and coding scheme is 16QAM_R_1_2\n");
        }
        else if (dlBurstProfile->ofdma.fecCodeModuType == 3)
        {
            printf("modulation and coding scheme is 16QAM_R_3_4\n");
        }
        else if (dlBurstProfile->ofdma.fecCodeModuType == 4)
        {
            printf("modulation and coding scheme is 64QAM_R_1_2\n");
        }
        else if (dlBurstProfile->ofdma.fecCodeModuType == 5)
        {
            printf("modeulation and coding scheme is 64QAM_R_2_3\n");
        }
        else if (dlBurstProfile->ofdma.fecCodeModuType == 6)
        {
            printf("modulation and coding scheme is 64QAM_R_3_4\n");
        }
    }

}
// /**
// FUNCTION   :: MacDot16PrintRunTimeInfo
// LAYER      :: MAC
// PURPOSE    :: printf out the node id and  simulation time
// PARAMETERS ::
// + node  : Node*         : Pointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// RETURN     :: void
// **/
void MacDot16PrintRunTimeInfo(Node* node, MacDataDot16* dot16)
{
    char clockStr[MAX_STRING_LENGTH];
    char intfStr[MAX_STRING_LENGTH];

    ctoa(node->getNodeTime(), clockStr);
    IO_ConvertIpAddressToString(
        MAPPING_GetInterfaceAddressForInterface(
            node,
            node->nodeId,
            dot16->myMacData->interfaceIndex), intfStr);
    if (dot16->stationType == DOT16_SS)
    {
        printf("At %s Node %d (SS) %s: ", clockStr, node->nodeId, intfStr);
    }
    else if (dot16->stationType == DOT16_BS)
    {
        printf("At %s Node %d (BS) %s: ", clockStr, node->nodeId, intfStr);
    }

}

// /**
// FUNCTION   :: MacDot16PrintParameter
// LAYER      :: MAC
// PURPOSE    :: Print system parameters
// PARAMETERS ::
// + node  : Node*         : Poointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16PrintParameter(Node* node, MacDataDot16* dot16)
{

    printf("Node%u interface %d MAC 802.16 parameters:\n",
           node->nodeId, dot16->myMacData->interfaceIndex);

    // print basic parameters
    if (dot16->dot16eEnabled)
    {
        printf("    802.16e (mobility) support = TRUE\n");
    }
    else
    {
        printf("    802.16e (mobility) support = FALSE\n");
    }

    // print station type
    if (dot16->stationType == DOT16_SS)
    {
        printf("    station type = SS\n");
    }
    else
    {
        printf("    station type = BS\n");
    }

    // print operation mode
    if (dot16->mode == DOT16_PMP)
    {
        printf("    operation mode = PMP\n");
    }
    else
    {
        printf("    operation mode = MESH\n");
    }

    // print duplex mode
    if (dot16->duplexMode == DOT16_TDD)
    {
        printf("    duplex mode = TDD\n");
    }
    else if (dot16->duplexMode == DOT16_FDD_HALF)
    {
        printf("    duplex mode = FDD-HALF\n");
    }
    else
    {
        printf("    duplex mode = FDD-FULL\n");
    }

    // print PHY type
    if (dot16->phyType == DOT16_PHY_SC)
    {
        printf("    phy type = SC\n");
    }
    else if (dot16->phyType == DOT16_PHY_SCA)
    {
        printf("    phy type = SCA\n");
    }
    else if (dot16->phyType == DOT16_PHY_OFDM)
    {
        printf("    phy type = OFDM\n");
    }
    else
    {
        printf("    phy type = OFDMA\n");
    }

    // print Mobility Support
    if (dot16->dot16eEnabled)
    {
        printf("    mobile WiMAX is supported\n");
    }
    else
    {
        printf("    Only Fixed WiMAX is supported\n");
    }

}

// /**
// FUNCTION   :: MacDot16InitParameter
// LAYER      :: MAC
// PURPOSE    :: Initialize system parameters
// PARAMETERS ::
// + node : Node* : Pointer to node
// + nodeInput : const NodeInput* : Pointer to node input
// + interfaceIndex : int : Interface Index
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16InitParameter(
         Node* node,
         const NodeInput* nodeInput,
         int interfaceIndex,
         MacDataDot16* dot16)
{
  //  Address interfaceAddress;

    BOOL wasFound;
    char stringVal[MAX_STRING_LENGTH];
    int intVal;

    // Read basic parameters

    // read the station type
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-STATION-TYPE",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "SS") == 0)
        {
            // subscriber station
            dot16->stationType = DOT16_SS;
        }
        else if (strcmp(stringVal, "BS") == 0)
        {
            // base station
            dot16->stationType = DOT16_BS;
        }
        else
        {
            ERROR_ReportWarning(
                "MAC802.16: Wrong value of MAC-802.16-STATION-TYPE,"
                "shoudle be MS or BS. Use default value as MS.");

            // by default, a node is subscriber station
            dot16->stationType = DOT16_SS;
        }
    }
    else
    {
        // not configured. Assume a subscriber station by default
        dot16->stationType = DOT16_SS;
    }

    // read the operation mode
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-OPERATION-MODE",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "PMP") == 0)
        {
            // point to multi-point
            dot16->mode = DOT16_PMP;
        }
        else if (strcmp(stringVal, "MESH") == 0)
        {
            //  mesh mode
            dot16->mode = DOT16_MESH;
        }
        else
        {
            ERROR_ReportWarning(
                "MAC802.16: Wrong value of MAC-802.16-OPERATION-MODE,"
                "shoudle be PMP or MESH. Use default value as PMP.");

            // by default, a node is operating in PMP mode
            dot16->mode = DOT16_PMP;
        }
    }
    else
    {
        // not configured. Assume PMP mode by default
        dot16->mode = DOT16_PMP;
    }

    // read the duplex mode
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-DUPLEX-MODE",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "TDD") == 0)
        {
            // time division duplex
            dot16->duplexMode = DOT16_TDD;
        }
        else if (strcmp(stringVal, "FDD-HALF") == 0)
        {
            // frequency division duplex - half duplex
            dot16->duplexMode = DOT16_FDD_HALF;
        }
        else if (strcmp(stringVal, "FDD-FULL") == 0)
        {
            // frequency division duplex - full duplex
            dot16->duplexMode = DOT16_FDD_FULL;
        }
        else
        {
            ERROR_ReportWarning(
                "MAC802.16: Wrong value of MAC-802.16-DUPLEX-MODE, shoudle"
                "be TDD, FDD-HALF or FDD-FULL. Use default value as TDD.");

            // by default, a node uses TDD duplex
            dot16->duplexMode = DOT16_TDD;
        }
    }
    else
    {
        // not configured. Assume TDD by default
        dot16->duplexMode = DOT16_TDD;
    }

    // Assuming OFDMA PHY in current implementation.
    dot16->phyType = DOT16_PHY_OFDMA;

    IO_ReadInt(node,
               node->nodeId,
               interfaceIndex,
               nodeInput,
               "MAC-802.16e-PAGING-HASHSKIP-THRESHOLD",
               &wasFound,
               &intVal);

    if (wasFound)
    {
        if (intVal < 0)
        {
            ERROR_ReportWarning(
                "MAC802.16: MAC-802.16e-PAGING-HASHSKIP-THRESHOLD must be "
                "larger than 0!");
            dot16->macHashSkipThshld = 0;
        }
        else if (intVal > 255)
        {
            ERROR_ReportWarning(
                "MAC802.16: MAC-802.16e-PAGING-HASHSKIP-THRESHOLD must be "
                "less than 255!");
            dot16->macHashSkipThshld = 0;
        }
        else
        {
            dot16->macHashSkipThshld =(UInt16) intVal;
        }
    }
    else
    {
        dot16->macHashSkipThshld = 0;
    }
}

// /**
// FUNCTION   :: MacDot16InitChannelList
// LAYER      :: MAC
// PURPOSE    :: Init the operational channel list
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + dot16     : MacDot16* : Pointer to DOT16 data struct
// RETURN     :: void      : NULL
// **/
static
void MacDot16InitChannelList(Node* node,
                             MacDataDot16* dot16)
{
    unsigned int phyNumber;
    int totalChannels = 0;
    int index;
    int i;

    phyNumber = dot16->myMacData->phyNumber;

    // count the number of channels that I can listen to
    dot16->numChannels = 0;
    totalChannels = PROP_NumberChannels(node);
    for (i = 0; i < totalChannels; i ++)
    {
        if (PHY_CanListenToChannel(node, phyNumber, i))
        {
            // this is a channel allocated to me
            dot16->numChannels ++;
        }
    }

    // must have a least one channel allocated if TDD
    // and two channels allocated if FDD.
    if ((dot16->numChannels < 1 && dot16->duplexMode == DOT16_TDD) ||
        (dot16->numChannels < 2 && dot16->duplexMode != DOT16_TDD))
    {
        char errString[MAX_STRING_LENGTH];

        sprintf(errString,
                "MAC 802.16: Too few channels (%d) are allocated to "
                "interface %d of node %d!",
                dot16->numChannels,
                dot16->myMacData->interfaceIndex,
                node->nodeId);

        ERROR_ReportError(errString);
    }

    // allocated space for storing my channels
    dot16->channelList =
        (int*) MEM_malloc(dot16->numChannels * sizeof(int));
    memset((char*)dot16->channelList, 0, dot16->numChannels * sizeof(int));
    // Now get the list of channels that I am capable to listen to
    index = 0;
    for (i = 0; i < totalChannels; i ++)
    {
        if (PHY_CanListenToChannel(node, phyNumber, i))
        {
            dot16->channelList[index] = i;
            index ++;
        }
    }

    if (DEBUG_CHANNEL)
    {
        // print out my channel list
        printf("Node%u's 802.16 interface%d has %d listenable channels\n",
               node->nodeId,
               dot16->myMacData->interfaceIndex,
               dot16->numChannels);

        printf("    channel list:");
        for (i = 0; i < dot16->numChannels; i ++)
        {
            printf(" %d", dot16->channelList[i]);
        }
        printf("\n");
    }
}

// /**
// FUNCTION   :: MacDot16PrintStats
// LAYER      :: MAC
// PURPOSE    :: Print out statistics
// PARAMETERS ::
// + node : Node* : Pointer to node
// + interfaceIndex : int : Interface Index
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16PrintStats(Node* node,
                        int interfaceIndex,
                        MacDataDot16* dot16)
{
    char buf[MAX_STRING_LENGTH];

    // print out the station type
    if (dot16->stationType == DOT16_BS)
    {
        sprintf(buf, "Station type = BS");
    }
    else
    {
        sprintf(buf, "Station type = SS");
    }
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
}


//--------------------------------------------------------------------------
//  Utility functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16SetTimer
// LAYER      :: MAC
// PURPOSE    :: Set a timer message. If a non-NULL message pointer is
//               passed in, this function will just send out that msg.
//               If NULL message is passed in, it will create a new
//               message and send it out. In either case, a pointer to
//               the message is returned, in case the caller wants to
//               cancel the message in the future.
// PARAMETERS ::
// + node      : Node*             : Pointer to node
// + dot16     : MacDataDot16*     : Pointer to DOT16 structure
// + timerType : MacDot16TimerType : Type of the timer
// + delay     : clocktype         : Delay of this timer
// + msg       : Message*          : If non-NULL, use this message
// + infoVal   : unsigned int      : Additional info if needed
// RETURN     :: Message*          : Pointer to the timer message
// **/
Message* MacDot16SetTimer(Node* node,
                          MacDataDot16* dot16,
                          MacDot16TimerType timerType,
                          clocktype delay,
                          Message* msg,
                          unsigned int infoVal)
{
    Message* timerMsg = NULL;
    MacDot16Timer* timerInfo;

    if (DEBUG_SetTimer)
    {   MacDot16PrintRunTimeInfo(node, dot16);
        printf("Dot16SetTimer: timer type: %d, delay: %" TYPES_64BITFMT "d"
               " timer expiry: %" TYPES_64BITFMT "d\n",
               timerType, delay,node->getNodeTime()+ delay);

    }

    if (msg != NULL)
    {
        timerMsg = msg;
    }
    else
    {
        // allocate the timer message and send out
        timerMsg = MESSAGE_Alloc(node,
                                 MAC_LAYER,
                                 MAC_PROTOCOL_DOT16,
                                 MSG_MAC_TimerExpired);

        MESSAGE_SetInstanceId(timerMsg,
                              (short) dot16->myMacData->interfaceIndex);

        MESSAGE_InfoAlloc(node, timerMsg, sizeof(MacDot16Timer));
        timerInfo = (MacDot16Timer*) MESSAGE_ReturnInfo(timerMsg);

        timerInfo->timerType = timerType;
        timerInfo->info = infoVal;
    }

    MESSAGE_Send(node, timerMsg, delay);

    return (timerMsg);
}

// /**
// FUNCTION   :: MacDot16SetTimer
// LAYER      :: MAC
// PURPOSE    :: Set a timer message. If a non-NULL message pointer is
//               passed in, this function will just send out that msg.
//               If NULL message is passed in, it will create a new
//               message and send it out. In either case, a pointer to
//               the message is returned, in case the caller wants to
//               cancel the message in the future.
// PARAMETERS ::
// + node      : Node*             : Pointer to node
// + dot16     : MacDataDot16*     : Pointer to DOT16 structure
// + timerType : MacDot16TimerType : Type of the timer
// + delay     : clocktype         : Delay of this timer
// + msg       : Message*          : If non-NULL, use this message
// + infoVal   : unsigned int      : Additional info if needed
// + infoVal2  : unsigned int      : Another additional info if needed
// RETURN     :: Message*          : Pointer to the timer message
// **/
Message* MacDot16SetTimer(Node* node,
                          MacDataDot16* dot16,
                          MacDot16TimerType timerType,
                          clocktype delay,
                          Message* msg,
                          unsigned int infoVal,
                          unsigned int infoVal2)
{
    Message* timerMsg = NULL;
    MacDot16Timer* timerInfo;

    if (msg != NULL)
    {
        timerMsg = msg;
    }
    else
    {
        // allocate the timer message and send out
        timerMsg = MESSAGE_Alloc(node,
                                 MAC_LAYER,
                                 MAC_PROTOCOL_DOT16,
                                 MSG_MAC_TimerExpired);
        ERROR_Assert(timerMsg != NULL, "MAC 802.16: Out of memory!");

        MESSAGE_SetInstanceId(timerMsg,
                              (short) dot16->myMacData->interfaceIndex);

        MESSAGE_InfoAlloc(node, timerMsg, sizeof(MacDot16Timer));
        timerInfo = (MacDot16Timer*) MESSAGE_ReturnInfo(timerMsg);

        timerInfo->timerType = timerType;
        timerInfo->info = infoVal;
        timerInfo->Info2 = infoVal2;
    }

    MESSAGE_Send(node, timerMsg, delay);
    return (timerMsg);
}

// /**
// FUNCTION   :: MacDot16GetIndexInChannelList
// LAYER      :: MAC
// PURPOSE    :: Get the index of a channel in the channel list
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + dot16     : MacDot16* : Pointer to DOT16 data struct
// + channelIndex : int    : Channel index of the channel
// RETURN     :: int       : The index of the channel in my channel list
// **/
int MacDot16GetIndexInChannelList(Node* node,
                                  MacDataDot16* dot16,
                                  int channelIndex)
{
    int i;

    for (i = 0; i < dot16->numChannels; i ++)
    {
        if (dot16->channelList[i] == channelIndex)
        {
            // found it in the list
            return i;
        }
    }

    // not found
    return DOT16_INVALID_CHANNEL;
}

// /**
// FUNCTION   :: MacDot16StartListening
// LAYER      :: MAC
// PURPOSE    :: Start listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// + channelIndex : int : index of the channel
// RETURN     :: void : NULL
// **/
void MacDot16StartListening(
         Node* node,
         MacDataDot16* dot16,
         int channelIndex)
{
    if (dot16->lastListeningChannel != DOT16_INVALID_CHANNEL &&
        dot16->lastListeningChannel != channelIndex &&
        PHY_IsListeningToChannel(
            node,
            dot16->myMacData->phyNumber,
            dot16->lastListeningChannel))
    {
        PhyData *thisPhy = node->phyData[dot16->myMacData->phyNumber];
        PhyDataDot16* phyDot16 = (PhyDataDot16*) thisPhy->phyVar;
        int i;

        PHY_StopListeningToChannel(
            node,
            dot16->myMacData->phyNumber,
            dot16->lastListeningChannel);

        dot16->lastListeningChannel = DOT16_INVALID_CHANNEL;

        int numSubchannels =
            PhyDot16GetNumberSubchannelsReceiving(
                node,
                dot16->myMacData->phyNumber);

        for (i = 0; i < numSubchannels; i ++)
        {
            phyDot16->interferencePower_mW[i] = 0.0;
        }
    }

    if (!PHY_IsListeningToChannel(
             node,
             dot16->myMacData->phyNumber,
             channelIndex))
    {
        if (DEBUG_CHANNEL)
        {   MacDot16PrintRunTimeInfo(node, dot16);
            MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*)dot16->phyData;
            printf("DOT16: node%u's intf %d starts listening to channel"
                   " %d.Current FrameNumber = %d\n",
                   node->nodeId,
                   dot16->myMacData->interfaceIndex,
                   channelIndex,
                   dot16Ofdma->frameNumber);
        }

        PHY_StartListeningToChannel(
            node,
            dot16->myMacData->phyNumber,
            channelIndex);
        dot16->lastListeningChannel = channelIndex;

        if (DEBUG_CHANNEL)
        {   MacDot16PrintRunTimeInfo(node, dot16);
            MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*)dot16->phyData;
            printf("DOT16: node%u's intf %d starts listening to channel"
                   " %d.Current FrameNumber = %d\n",
                   node->nodeId,
                   dot16->myMacData->interfaceIndex,
                   channelIndex,
                   dot16Ofdma->frameNumber);
        }
    }
}

// /**
// FUNCTION   :: MacDot16StopListening
// LAYER      :: MAC
// PURPOSE    :: Stop listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to Dot16 structure
// + channelIndex : int : index of the channel
// RETURN     :: void : NULL
// **/
void MacDot16StopListening(
         Node* node,
         MacDataDot16* dot16,
         int channelIndex)
{
    if (channelIndex != DOT16_INVALID_CHANNEL &&
        PHY_IsListeningToChannel(
            node,
            dot16->myMacData->phyNumber,
            channelIndex))
    {
        if (DEBUG_CHANNEL)
        {   MacDot16PrintRunTimeInfo(node, dot16);
            MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*)dot16->phyData;
            printf("DOT16: node%u's intf %d stops listening to channel"
                   " %d.Current FrameNumber = %d\n",
                   node->nodeId,
                   dot16->myMacData->interfaceIndex,
                   channelIndex,
                   dot16Ofdma->frameNumber);
        }

        PHY_StopListeningToChannel(
            node,
            dot16->myMacData->phyNumber,
            channelIndex);
    }

    dot16->lastListeningChannel = DOT16_INVALID_CHANNEL;
}

// /**
// FUNCTION   :: MacDot16IsMyCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID belongs or applicable to me
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if my CID, FALSE, else
// **/
BOOL MacDot16IsMyCid(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType cid)
{
    // check broadcast CID
    if (cid == DOT16_BROADCAST_CID ||
        cid == DOT16_INITIAL_RANGING_CID)
    {
        return TRUE;
    }

    // check multicast CIDs
    if (cid == DOT16_MULTICAST_ALL_SS_CID)
    {
        return TRUE;
    }

    if (dot16->stationType == DOT16_BS)
    {
        // this is a base station
        // ---> receive all packets from SSs under PMP
        return TRUE;
    }
    else
    {
        // this is a subscriber station
        MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
        MacDot16ServiceFlow* serviceFlow;
        int i;

        if (dot16->dot16eEnabled == FALSE ||
            dot16Ss->nbrScanStatus != DOT16e_SS_NBR_SCAN_InScan)
        {
            // not in neighbor scan mode
            if (cid == dot16Ss->servingBs->basicCid ||
                cid == dot16Ss->servingBs->primaryCid ||
                cid == dot16Ss->servingBs->secondaryCid)
            {
                return TRUE;
            }

            // check all unicast CIDs
            for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
            {
                serviceFlow = dot16Ss->dlFlowList[i].flowHead;
                while (serviceFlow != NULL)
                {
                    if (serviceFlow->cid == cid)
                    {
                        return TRUE;
                    }

                    serviceFlow = serviceFlow->next;
                }
            }
        }
        else
        {
            // in neighbor scan mode

            if ((dot16Ss->targetBs != NULL) &&

                (cid == dot16Ss->targetBs->basicCid ||
                 cid == dot16Ss->targetBs->primaryCid ||
                 cid == dot16Ss->targetBs->secondaryCid))
            {
                return TRUE;
            }
        }

    }

    return FALSE;
}

// /**
// FUNCTION   :: MacDot16IsManagementCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID is a management CID
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if mgmt CID, FALSE, else
// **/
BOOL MacDot16IsManagementCid(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType cid)
{
    if (cid == DOT16_BROADCAST_CID)
    {
        return TRUE;
    }

    if (cid <= DOT16_PRIMARY_CID_END)
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: MacDot16IsTransportCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID is a transport CID
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if mgmt CID, FALSE, else
// **/
BOOL MacDot16IsTransportCid(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType cid)
{
    return cid >= DOT16_TRANSPORT_CID_START &&
           cid <= DOT16_TRANSPORT_CID_END;
}

// /**
// FUNCTION   :: MacDot16IsMulticastCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID is a multicast CID
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if mgmt CID, FALSE, else
// **/
BOOL MacDot16IsMulticastCid(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType cid)
{
    if (cid == DOT16_MULTICAST_ALL_SS_CID)
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: MacDot16IsMulticastMacAddress
// LAYER      :: MAC
// PURPOSE    :: Check if a given MAC address is multicast address
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + macAddress : unsigned char* : MAC address
// RETURN     :: BOOL : TRUE, if mgmt CID, FALSE, else
// **/
BOOL MacDot16IsMulticastMacAddress(
         Node* node,
         MacDataDot16* dot16,
         unsigned char* macAddress)
{
    unsigned char bcastMacAddress[DOT16_MAC_ADDRESS_LENGTH] =
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        //{0x0, 0x0, 0xFF, 0xFF, 0xFF, 0xFF};

    if (memcmp(macAddress,
               bcastMacAddress,
               DOT16_MAC_ADDRESS_LENGTH) == 0)
    {
        return TRUE;
    }
    return FALSE;
}

// /**
// FUNCTION   :: MacDot16GetProfIndexFromCodeModType
// LAYER      :: MAC
// PURPOSE    :: get profile index from the modulation & coding information
// PARAMETERS ::
// + node  : Node*         : Pointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// + frameType : MacDot16SubframeType : UL or DL frame
// + codeModType : coding and modulation type
// + profIndex : int* : Pointer to the profile index
// RETURN     :: BOOL : NULL
// **/
BOOL MacDot16GetProfIndexFromCodeModType(Node* node,
                                         MacDataDot16* dot16,
                                         MacDot16SubframeType frameType,
                                         unsigned char codeModType,
                                         int* profIndex)
{
    MacDot16Ss* dot16Ss;
    MacDot16Bs* dot16Bs;
    unsigned char diuc;
    unsigned char uiuc;
    MacDot16DlBurstProfile* dlBurstProfile;
    MacDot16UlBurstProfile* ulBurstProfile;

    if (frameType == DOT16_DL && dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;

        for (diuc = 0; diuc < dot16Ss->numOfDLBurstProfileInUsed; diuc ++)
        {
            dlBurstProfile = (MacDot16DlBurstProfile*)
                             &(dot16Ss->servingBs->dlBurstProfile[diuc]);

            if (dlBurstProfile->ofdma.fecCodeModuType == codeModType)
            {
                *profIndex = diuc;

                return TRUE;
            }
        }
    }
    else if (frameType == DOT16_UL && dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;

        for (uiuc = 0; uiuc < dot16Ss->numOfULBurstProfileInUsed; uiuc ++)
        {
            ulBurstProfile = (MacDot16UlBurstProfile*)
                             &(dot16Ss->servingBs->ulBurstProfile[uiuc]);

            if (ulBurstProfile->ofdma.fecCodeModuType == codeModType)
            {
                *profIndex = uiuc;

                return TRUE;
            }
        }
    }

    if (frameType == DOT16_DL && dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;

        for (diuc = 0; diuc < dot16Bs->numOfDLBurstProfileInUsed; diuc ++)
        {
            dlBurstProfile = (MacDot16DlBurstProfile*)
                              &(dot16Bs->dlBurstProfile[diuc]);

            if (dlBurstProfile->ofdma.fecCodeModuType == codeModType)
            {
                *profIndex = diuc;

                return TRUE;
            }
        }
    }
    else if (frameType == DOT16_UL && dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;

        for (uiuc = 0; uiuc < dot16Bs->numOfULBurstProfileInUsed; uiuc ++)
        {
            ulBurstProfile = (MacDot16UlBurstProfile*)
                             &(dot16Bs->ulBurstProfile[uiuc]);

            if (ulBurstProfile->ofdma.fecCodeModuType == codeModType)
            {
                *profIndex = uiuc;

                return TRUE;
            }
        }
    }

    return FALSE;
}

// /**
// FUNCTION   :: MacDot16GetLeastRobustBurst
// LAYER      :: MAC
// PURPOSE    :: Get the least robust downlink burst profile
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + frameType : MacDot16SubframeType  : Frame type, either U: or DL
// + signalMea : double : CINR in dB
// + unsigned char* : leastRobustBurst : Pointer for the least robust DIUC
// RETURN     :: BOOL : TRUE, if find one; FALSE, if not find one
// **/
BOOL MacDot16GetLeastRobustBurst(Node* node,
                                 MacDataDot16* dot16,
                                 MacDot16SubframeType frameType,
                                 double signalMea,
                                 unsigned char* leastRobustBurst)
{
    MacDot16Ss* dot16Ss;
    MacDot16Bs* dot16Bs;
    unsigned char diuc;
    unsigned char uiuc;
    MacDot16DlBurstProfile* dlBurstProfile;
    MacDot16UlBurstProfile* ulBurstProfile;

    // The burst profile is in unit of 0.25dB
    signalMea = signalMea * 4;

    // for DL burst profile, DIUC is equal to array index
    if (frameType == DOT16_DL && dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;

        for (diuc = 1; diuc < dot16Ss->numOfDLBurstProfileInUsed; diuc ++)
        {
            dlBurstProfile = (MacDot16DlBurstProfile*)
                             &(dot16Ss->servingBs->dlBurstProfile[diuc]);

            if (dlBurstProfile->ofdma.entryThreshold > signalMea)
            {
                break;
            }
        }

        *leastRobustBurst = diuc - 1;
    }
    if (frameType == DOT16_DL && dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;

        for (diuc = 1; diuc < dot16Bs->numOfDLBurstProfileInUsed; diuc ++)
        {
            dlBurstProfile = (MacDot16DlBurstProfile*)
                              &(dot16Bs->dlBurstProfile[diuc]);

            if (dlBurstProfile->ofdma.entryThreshold > signalMea)
            {
                break;
            }
        }

        *leastRobustBurst = diuc - 1;
        dlBurstProfile = (MacDot16DlBurstProfile*)
                          &(dot16Bs->dlBurstProfile[*leastRobustBurst]);
        //Assumes last burst profile is BPSK. If index points to BPSK based burst profile
        //least robust profile is one index lower.
        if (dlBurstProfile->ofdma.fecCodeModuType == PHY802_16_BPSK)
        {
            *leastRobustBurst = *leastRobustBurst -1;
        }
    }
    else if (frameType == DOT16_UL && dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;

        for (uiuc = 1; uiuc < dot16Bs->numOfULBurstProfileInUsed; uiuc ++)
        {
            ulBurstProfile = (MacDot16UlBurstProfile*)
                             &(dot16Bs->ulBurstProfile[uiuc]);

            if (ulBurstProfile->ofdma.entryThreshold > signalMea)
            {
                break;
            }
        }
        *leastRobustBurst = uiuc;
        ulBurstProfile = (MacDot16UlBurstProfile*)
                          &(dot16Bs->ulBurstProfile[*leastRobustBurst -1]);
        //Assumes last burst profile is BPSK. If index points to BPSK based burst profile
        //least robust profile is one index lower.
        if (ulBurstProfile->ofdma.fecCodeModuType == PHY802_16_BPSK)
        {
            *leastRobustBurst = *leastRobustBurst -1;
        }
    }

    return TRUE;
}

// /**
// FUNCTION   :: MacDot16UpdateMeanMeasurement
// LAYER      :: MAC
// PURPOSE    :: Update the measurement histroy and recalcualte the
//               mean value
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + measInfo  : MacDot16SignalMeasurementInfo* : Pointer to the meas. hist.
// + signalMea : PhySignalMeasurement* : Pointer to the current measurement
// + meanMeas  : MacDot16SignalMeasurementInfo* : Pinter to the mean of meas
// RETURN     :: void : NULL
void MacDot16UpdateMeanMeasurement(Node* node,
                                   MacDataDot16* dot16,
                                   MacDot16SignalMeasurementInfo* measInfo,
                                   PhySignalMeasurement* signalMea,
                                   MacDot16SignalMeasurementInfo* meanMeas)
{

    int i;
    int oldestMeas = 0;
    clocktype oldestTimeStamp;
    int numActiveMeas = 0;

    // set those measurement out age to be inactive
    for (i = 0; i < DOT16_MAX_NUM_MEASUREMNT; i ++)
    {
        if (measInfo[i].measTime > 0 &&
            measInfo[i].isActive &&
            (measInfo[i].measTime + DOT16_MEASUREMENT_VALID_TIME) <
            node->getNodeTime())
        {
            measInfo[i].isActive = FALSE;
        }
    }

    // find one inactive or with the oldest time stamp
    oldestTimeStamp = node->getNodeTime();
    for (i = 0; i < DOT16_MAX_NUM_MEASUREMNT; i ++)
    {
        if (!measInfo[i].isActive)
        {
            // if find one inactive, it can be replaced with the new one
            oldestMeas = i;
            break;
        }
        else if (measInfo[i].measTime < oldestTimeStamp)
        {
            // if all are active find the one with oldest stamp
            oldestMeas = i;
            oldestTimeStamp = measInfo[i].measTime;
        }
    }

    // store the current meas info
    measInfo[oldestMeas].isActive = TRUE;
    measInfo[oldestMeas].cinr = signalMea->cinr;
    measInfo[oldestMeas].rssi = signalMea->rss;
    measInfo[oldestMeas].measTime = node->getNodeTime();

    // calculate the mean
    // both time and number of measurement are considered in this case
    meanMeas->cinr = 0;
    meanMeas->rssi = 0;
    for (i = 0; i < DOT16_MAX_NUM_MEASUREMNT; i ++)
    {
        if (measInfo[i].isActive)
        {
            meanMeas->cinr += measInfo[i].cinr;
            meanMeas->rssi += measInfo[i].rssi;
            numActiveMeas ++;
        }
    }

    // average it
    meanMeas->cinr /= numActiveMeas;
    meanMeas->rssi /= numActiveMeas;
    meanMeas->measTime = node->getNodeTime();
}

//--------------------------------------------------------------------------
// Functions for DSX
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16BuildDsaReqMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSA-REQ mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + pktInfo   : MacDot16CsIpv4Classifier* : Pointer to classifier
// + sfDirection : MacDot16ServiceFlowDirection  : Direction of service flow
// + cid       : Dot16CIDType : CID message shall use
// RETURN     :: Message* : Point to the message containing the DSA-REQ PDU
// **/
Message* MacDot16BuildDsaReqMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             MacDot16CsClassifier* pktInfo,
             MacDot16ServiceFlowDirection sfDirection,
             Dot16CIDType cid)
{
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16DsaReqMsg* dsaReq;
    BOOL isSleepModeEnabled;
    UInt16 delay = 0 ;

    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    int sfParamLenIndex;
    int csSpecLenIndex;
    int csRuleLenIndex;
    int i;
    int delayJitter;

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        isSleepModeEnabled = dot16Ss->isSleepModeEnabled;
        dot16Ss->lastCSPacketRcvd = node->getNodeTime();
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
        isSleepModeEnabled = dot16Bs->isSleepEnabled; // Mandatory at BS end
    }

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) (&macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, cid);

    dsaReq = (MacDot16DsaReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsaReqMsg);

    dsaReq->type = DOT16_DSA_REQ;
    if (dot16->stationType == DOT16_SS)
    {
        // SS
        MacDot16ShortToTwoByte(dsaReq->transId[0], dsaReq->transId[1],
            dot16Ss->transId);
    }
    else
    {
        // BS
        MacDot16ShortToTwoByte(dsaReq->transId[0], dsaReq->transId[1],
            dot16Bs->transId);
    }

    // add service flow parameter TLVs
    // TLV_COMMON_UlServiceFlow or TLV_COMMON_DlServiceFlow
    if (sfDirection == DOT16_UPLINK_SERVICE_FLOW)
    {
        macFrame[index ++] = TLV_COMMON_UlServiceFlow;
    }
    else
    {
        macFrame[index ++] = TLV_COMMON_DlServiceFlow;
    }
    sfParamLenIndex = index;
    macFrame[index ++] = 3; // temporary 3, will be adjust to right value
                            // later
    macFrame[index ++] = 0; // No meaning, for implementation purpose only

    // for DSA_REQ from BS cfId and transportCid should included
    if (dot16->stationType == DOT16_BS)
    {
        // TLV_DSX_ServiceFlowId
        macFrame[index ++] = TLV_DSX_ServiceFlowId;
        macFrame[index ++] = 4;
        memcpy((char*) &(macFrame[index]), &(serviceFlow->sfid), 4);
        index += 4;

        // TLV_DSX_Cid
        macFrame[index ++] = TLV_DSX_Cid;
        macFrame[index ++] = 2;
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               serviceFlow->cid);
        index += 2;
    }

    // TLV_DSX_QoSParamSetType
    macFrame[index ++] = TLV_DSX_QoSParamSetType;
    macFrame[index ++] = 1;
    macFrame[index ++] = 0x6; // 0x6 means perform admission ocntrol
                              // and activate the service flow,
                              // apply parameters to both
                              // admitted and active sets

    // TLV_DSX_MaxSustainedRate
    macFrame[index ++] = TLV_DSX_MaxSustainedRate;
    macFrame[index ++] = 4;
    memcpy((char*) &macFrame[index],
           &(serviceFlow->qosInfo.maxSustainedRate),
           4);
    index += 4;

    // TLV_DSX_MaxTrafficBurst
    macFrame[index ++] = TLV_DSX_MaxTrafficBurst;
    macFrame[index ++] = 4;
    memcpy((char*) &macFrame[index],
           &(serviceFlow->qosInfo.maxPktSize),
           4);
    index += 4;

    // TLV_DSX_MinReservedRate
    macFrame[index ++] = TLV_DSX_MinReservedRate;
    macFrame[index ++] = 4;
    memcpy((char*) &macFrame[index],
           &(serviceFlow->qosInfo.minReservedRate),
           4);
    index += 4;

    // TLV_DSX_MinTolerableRate
    macFrame[index ++] = TLV_DSX_MinTolerableRate;
    macFrame[index ++] = 4;
    memcpy((char*) &macFrame[index],
           &(serviceFlow->qosInfo.minPktSize),
           4); // Be sure to give the right value
    index += 4;

    // service type
    macFrame[index ++] = TLV_DSX_ServiceType;
    macFrame[index ++] = 1;
    macFrame[index ++] = (UInt8)serviceFlow->serviceType;

    // TLV_DSX_ToleratedJitter
    macFrame[index ++] = TLV_DSX_ToleratedJitter;
    macFrame[index ++] = 4;
    // convert to MILLI_SECOND
    delayJitter = (int) (serviceFlow->qosInfo.toleratedJitter /
                         MILLI_SECOND);
    if (delayJitter == 0)
    {
        delayJitter  = 1;
    }
    MacDot16IntToFourByte((char*) &macFrame[index],
                           delayJitter);
    index += 4;

    // TLV_DSX_MaxLatency
    macFrame[index ++] = TLV_DSX_MaxLatency;
    macFrame[index ++] = 4;
    // convert to MILLI_SECOND
    delayJitter = (int) (serviceFlow->qosInfo.maxLatency /
                         MILLI_SECOND);
    if (delayJitter == 0)
    {
        delayJitter  = 1;
    }
    MacDot16IntToFourByte((char*) &macFrame[index],
                           delayJitter);
    index += 4;
    if (serviceFlow->isPackingEnabled)
    {
        // TLV_DSX_FixLenVarLanSDU
        macFrame[index ++] = TLV_DSX_FixLenVarLanSDU;
        macFrame[index ++] = 1;
        if ((pktInfo->ipProtocol == IPPROTO_UDP)
            && (serviceFlow->qosInfo.minPktSize
            == serviceFlow->qosInfo.maxPktSize)
            && (serviceFlow->qosInfo.maxPktSize <=
            DOT16_FRAGMENTED_MAX_FIXED_LENGTH_PACKET_SIZE))
        {
            macFrame[index ++] = DOT16_FIXED_LENGTH_SDU;
            serviceFlow->isFixedLengthSDU = TRUE;

            // TLV_DSX_SduSize
            macFrame[index ++] = TLV_DSX_SduSize;
            macFrame[index ++] = 1;
            macFrame[index ++] = (UInt8)serviceFlow->qosInfo.maxPktSize;

            serviceFlow->fixedLengthSDUSize
                = serviceFlow->qosInfo.maxPktSize;
        }
        else
        {
            macFrame[index ++] = DOT16_VARIABLE_LENGTH_SDU;
        }
    }

    //ARQ related TLV parameters


    if (serviceFlow->isARQEnabled)
    {
        macFrame[index ++] = TLV_DSX_ARQ_Enable;
        macFrame[index ++] = 1;
        macFrame[index ++] = (UInt8)serviceFlow->isARQEnabled;

        macFrame[index ++] = TLV_DSX_ARQ_WindowSize;
        macFrame[index ++] = 2;
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               serviceFlow->arqParam->arqWindowSize);

        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_RetryTimeoutTxDelay ;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(serviceFlow->arqParam->arqRetryTimeoutTxDelay
            / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_RetryTimeoutRxDelay;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(serviceFlow->arqParam->arqRetryTimeoutRxDelay
            / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_BlockLifetime;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(serviceFlow->arqParam->arqBlockLifeTime
            / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_SyncLossTimeout;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(serviceFlow->arqParam->arqSyncLossTimeout
            / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;


        macFrame[index ++] = TLV_DSX_ARQ_DeliverInOrder ;
        macFrame[index ++] = 1;
        macFrame[index ++] = serviceFlow->arqParam->arqDeliverInOrder;

        macFrame[index ++] = TLV_DSX_ARQ_RxPurgeTimeout;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(serviceFlow->arqParam->arqRxPurgeTimeout
            / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_Blocksize ;
        macFrame[index ++] = 2;

        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               serviceFlow->arqParam->arqBlockSize);
        index += 2;
    }

    if (dot16->dot16eEnabled && isSleepModeEnabled)
    {
            macFrame[index ++] = TLV_DSX_TrafficIndicationPreference;
            macFrame[index ++] = 1;
            macFrame[index ++] = (UInt8)
                serviceFlow->TrafficIndicationPreference;
    }

    // More
    macFrame[index ++] = TLV_DSX_CSSpec;
    macFrame[index ++] = 1;
    macFrame[index ++] = 0; // Be sure to give the right value

    if (pktInfo->dstAddr.networkType == NETWORK_IPV4)
    {

        // CS specification
        // for IPv4 classfier
        macFrame[index ++] = TLV_DSX_CS_TYPE_IPv4;
        csSpecLenIndex = index;
        macFrame[index ++] = 1; // temporary length, will be adjust
                                // after adding all the CS TLVs
        macFrame[index ++] = 0; // Be sure to give the right value,
                                // now It is IPv4

        // TLV_CS_PACKET_ClassifierDscAction

        // TLV_CS_PACKET_ClassifierRule
        macFrame[index ++] = TLV_CS_PACKET_ClassifierRule;
        csRuleLenIndex = index;
        macFrame[index ++] = 1; // temporary length, will be adjust
                                // after adding all the CS TLVs
        macFrame[index ++] = 0; // Be sure to give the right value, now
                                // It is IPv4

        // TLV_CS_PACKET_Protocol
        macFrame[index ++] = TLV_CS_PACKET_Protocol;
        macFrame[index ++] = 1;
        macFrame[index ++] = pktInfo->ipProtocol;

        // assume only 1 set of src/dest address encoded, mask is omitted,
        // so only 4 bytes rather than 8 byte
        // TLV_CS_PACKET_IpSrcAddr
        macFrame[index ++] = TLV_CS_PACKET_IpSrcAddr;
        macFrame[index ++] = 4;
        NodeAddress srcAddress = GetIPv4Address(pktInfo->srcAddr);
        memcpy((char*) &macFrame[index], &srcAddress, 4);
        index += 4;

        // TLV_CS_PACKET_IpDestAddr
        macFrame[index ++] = TLV_CS_PACKET_IpDestAddr;
        macFrame[index ++] = 4;
        NodeAddress dstAddress = GetIPv4Address(pktInfo->dstAddr);
        memcpy((char*) &macFrame[index], &dstAddress, 4);
        index += 4;

        // TLV_CS_PACKET_SrcPortRange
        macFrame[index ++] = TLV_CS_PACKET_SrcPortRange;
        macFrame[index ++] = 2; // standard 4, we use 2 no
                                // harm to simulation;
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               pktInfo->srcPort);
        index += 2;

        // TLV_CS_PACKET_DestPortRange
        macFrame[index ++] = TLV_CS_PACKET_DestPortRange;
        macFrame[index ++] = 2; // standard 4, we use 2 no
                                // harm to simulation;
        MacDot16ShortToTwoByte(macFrame[index],
                           macFrame[index + 1],
                           pktInfo->dstPort);
        index += 2;
    }
    else  // IPv6
    {
        // CS specification
        // for IPv6 classfier
        macFrame[index ++] = TLV_DSX_CS_TYPE_IPv6;
        csSpecLenIndex = index;
        macFrame[index ++] = 1; // temporary length, will be
                                // adjusted after adding all the CS TLVs
        macFrame[index ++] = 0; // Be sure to give the right value,
                                // now It is IPv6

        // TLV_CS_PACKET_ClassifierDscAction

        // TLV_CS_PACKET_ClassifierRule
        macFrame[index ++] = TLV_CS_PACKET_ClassifierRule;
        csRuleLenIndex = index;
        macFrame[index ++] = 1; // temporary length, will be adjusted
                                // after adding all the CS TLVs
        macFrame[index ++] = 0; // Be sure to give the right value,
                                // now It is IPv6

        // TLV_CS_PACKET_Protocol
        macFrame[index ++] = TLV_CS_PACKET_Protocol;
        macFrame[index ++] = 1;
        macFrame[index ++] = pktInfo->ipProtocol;

        // TLV_CS_PACKET_IpSrcAddr
        // assume only 1 set of src/dest address encoded, mask is omitted,
        // so only 32 bytes rather than 16 byte
        macFrame[index ++] = TLV_CS_PACKET_IpSrcAddr;
        macFrame[index ++] = 16;
        in6_addr srcAddress = GetIPv6Address(pktInfo->srcAddr);
        memcpy((char*) &macFrame[index], &srcAddress, 16);
        index += 16;

        // TLV_CS_PACKET_IpDestAddr
        macFrame[index ++] = TLV_CS_PACKET_IpDestAddr;
        macFrame[index ++] = 16;
        in6_addr dstAddress = GetIPv6Address(pktInfo->dstAddr);
        memcpy((char*) &macFrame[index], &dstAddress, 16);
        index += 16;

        // TLV_CS_PACKET_SrcPortRange
        macFrame[index ++] = TLV_CS_PACKET_SrcPortRange;
        macFrame[index ++] = 2; // standard 4, we use 2 no harm to
                                // simulation;
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               pktInfo->srcPort);
        index += 2;

        // TLV_CS_PACKET_DestPortRange
        macFrame[index ++] = TLV_CS_PACKET_DestPortRange;
        macFrame[index ++] = 2; // standard 4, we use 2 no harm to
                                // simulation;
        MacDot16ShortToTwoByte(macFrame[index],
                           macFrame[index + 1],
                           pktInfo->dstPort);
        index += 2;
    }

    // reset CS  RULE length
    macFrame[csRuleLenIndex] = (UInt8)(index -1 - csRuleLenIndex);

    // reset CS param length
    macFrame[csSpecLenIndex] = (UInt8)(index -1 - csSpecLenIndex);
    // Be sure the index is of right value

    // other TLVs are not added right now

    // reset service flow param length
    macFrame[sfParamLenIndex] = (UInt8)(index -1 - sfParamLenIndex);
    // Be sure the index is of right value

    // Be sure to put HMAC tuple as the last TLV
    macFrame[index ++] = TLV_COMMON_HMacTuple;  // type
    macFrame[index ++] = 21; // length

    if (dot16->stationType == DOT16_SS)
    {
        macFrame[index ++] = dot16Ss->servingBs->authKeyInfo.hmacKeySeq;
    }
    else
    {
         macFrame[index ++] = 0; // Be sure to give the right value,
    }

    for (i = 0; i < 20; i ++)
    {
        // message digest

        macFrame[index ++] = 0; // Be sure to give the right value,
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");
    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a DSA-REQ message with transId %d\n",
                dsaReq->transId[0] * 256 + dsaReq->transId[1]);
    }

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16BuildDsaAckMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSA-ACK mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + cid       : Dot16CIDType : CID message shall use
// + transId   : unsigned int : transaction Id
// RETURN     :: Message* : Point to the message containing DSA-ACK PDU
// **/
Message* MacDot16BuildDsaAckMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             Dot16CIDType cid,
             unsigned int transId)
{
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16DsaAckMsg* dsaAck;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    int i;

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
    }
    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, cid);

    dsaAck = (MacDot16DsaAckMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsaAckMsg);

    dsaAck->type = DOT16_DSA_ACK;
    dsaAck->confirmCode = (UInt8)serviceFlow->dsaInfo.dsxCC;
    MacDot16ShortToTwoByte(dsaAck->transId[0], dsaAck->transId[1],
        transId);

    // Be sure to put HMAC tuple as the last TLV
    macFrame[index ++] = TLV_COMMON_HMacTuple;  // type
    macFrame[index ++] = 21; // length
    if (dot16->stationType == DOT16_SS)
    {
        macFrame[index ++] = dot16Ss->servingBs->authKeyInfo.hmacKeySeq;
    }
    else
    {
         macFrame[index ++] = 0; // Be sure to give the right value,
    }
    for (i = 0; i < 20; i ++)
    {
        // message digest

        macFrame[index ++] = 0; // Be sure to give the right value,
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a DSA-ACK message with transId %d\n", transId);
    }

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16BuildDsaRspMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSA-RSP mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + transId   : unsigned int : transId of this DSA-RSP
// + qosChanged : BOOL        : QoS parameters changed or not
// + cid       : Dot16CIDType : CID message shall use
// RETURN     :: Message* : Point to the message containing DSA-RSP PDU
// **/
Message* MacDot16BuildDsaRspMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* sFlow,
             unsigned int transId,
             BOOL qosChanged,
             Dot16CIDType cid)
{
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16DsaRspMsg* dsaRsp;

    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    int sfParamLenIndex;
    int csSpecLenIndex;
    int i;
    int delayJitter;
    UInt16 delay = 0 ;
    BOOL isSleepModeEnabled;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a DSA-RSP message with transId %d primary cid %d\n",
                transId, cid);
    }
    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        isSleepModeEnabled = dot16Ss->isSleepModeEnabled;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
         isSleepModeEnabled = dot16Bs->isSleepEnabled;
    }
    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, cid);
    // ss's primary cid not transport cid

    dsaRsp = (MacDot16DsaRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsaRspMsg);

    dsaRsp->type = DOT16_DSA_RSP;

    MacDot16ShortToTwoByte(dsaRsp->transId[0], dsaRsp->transId[1], transId);
    dsaRsp->confirmCode = (UInt8)sFlow->dsaInfo.dsxCC;

    // add service flow parameter TLVs
    // TLV_COMMON_UlServiceFlow or TLV_COMMON_DlServiceFlow
    if (sFlow->sfDirection == DOT16_UPLINK_SERVICE_FLOW)
    {
        macFrame[index ++] = TLV_COMMON_UlServiceFlow;
    }
    else
    {
        macFrame[index ++] = TLV_COMMON_DlServiceFlow;
    }
    sfParamLenIndex = index;
    macFrame[index ++] = 3; // temporary 3, will be adjust
                            // to right value later
    macFrame[index ++] = 0; // No meaning, for implementation purpose only

    // TLV_DSX_ServiceFlowId
    macFrame[index ++] = TLV_DSX_ServiceFlowId;
    macFrame[index ++] = 4;
    memcpy((char*) &(macFrame[index]), &(sFlow->sfid), 4);
    index += 4;

    // TLV_DSX_Cid
    macFrame[index ++] = TLV_DSX_Cid;
    macFrame[index ++] = 2;
    MacDot16ShortToTwoByte(macFrame[index],
                           macFrame[index + 1],
                           sFlow->cid);
    index += 2;
    if (qosChanged)
    {
        // TLV_DSX_QoSParamSetType
        macFrame[index ++] = TLV_DSX_QoSParamSetType;
        macFrame[index ++] = 1;
        macFrame[index ++] = 0x6;

        // TLV_DSX_MaxSustainedRate
        macFrame[index ++] = TLV_DSX_MaxSustainedRate;
        macFrame[index ++] = 4;
        memcpy((char*) &macFrame[index],
               &(sFlow->qosInfo.maxSustainedRate),
               4);
        index += 4;

        // TLV_DSX_MaxTrafficBurst
        macFrame[index ++] = TLV_DSX_MaxTrafficBurst;
        macFrame[index ++] = 4;
        memcpy((char*) &macFrame[index],
               &(sFlow->qosInfo.maxPktSize),
               4);
        index += 4;

        // TLV_DSX_MinReservedRate
        macFrame[index ++] = TLV_DSX_MinReservedRate;
        macFrame[index ++] = 4;
        memcpy((char*) &macFrame[index],
               &(sFlow->qosInfo.minReservedRate),
               4);
        index += 4;

        // TLV_DSX_MinTolerableRate
        macFrame[index ++] = TLV_DSX_MinTolerableRate;
        macFrame[index ++] = 4;
        memcpy((char*) &macFrame[index],
               &(sFlow->qosInfo.minPktSize), 4); // Be sure to
                                                 // give the right value
        index += 4;

        // TLV_DSX_ToleratedJitter
        macFrame[index ++] = TLV_DSX_ToleratedJitter;
        macFrame[index ++] = 4;
        // convert to MILLI_SECOND
        delayJitter = (int) (sFlow->qosInfo.toleratedJitter /
                             MILLI_SECOND);
        if (delayJitter == 0)
        {
            delayJitter  = 1;
        }
        MacDot16IntToFourByte((char*) &macFrame[index],
                               delayJitter);
        index += 4;

        // TLV_DSX_MaxLatency
        macFrame[index ++] = TLV_DSX_MaxLatency;
        macFrame[index ++] = 4;
        // convert to MILLI_SECOND
        delayJitter = (int) (sFlow->qosInfo.maxLatency /
                             MILLI_SECOND);
        if (delayJitter == 0)
        {
            delayJitter  = 1;
        }
        MacDot16IntToFourByte((char*) &macFrame[index],
                               delayJitter);
        index += 4;
    }
//ARQ related TLVs

    macFrame[index ++] = TLV_DSX_ARQ_Enable ;
    macFrame[index ++] = 1;
    macFrame[index ++] = (UInt8)sFlow->isARQEnabled;
    if (sFlow->isARQEnabled)
    {
        macFrame[index ++] = TLV_DSX_ARQ_WindowSize;
        macFrame[index ++] = 2;
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               sFlow->arqParam->arqWindowSize);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_RetryTimeoutTxDelay ;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(sFlow->arqParam->arqRetryTimeoutTxDelay
            / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_RetryTimeoutRxDelay;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(sFlow->arqParam->arqRetryTimeoutRxDelay / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_BlockLifetime;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(DOT16_ARQ_BLOCK_LIFETIME / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_SyncLossTimeout;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(sFlow->arqParam->arqSyncLossTimeout
            / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_DeliverInOrder ;
        macFrame[index ++] = 1;
        macFrame[index ++] = sFlow->arqParam->arqDeliverInOrder;

        macFrame[index ++] = TLV_DSX_ARQ_RxPurgeTimeout;
        macFrame[index ++] = 2;
        //Considering granularity = 10 micro seconds
        delay = (UInt16)(sFlow->arqParam->arqRxPurgeTimeout
            / MICRO_SECOND / 10);
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               delay);
        index += 2;

        macFrame[index ++] = TLV_DSX_ARQ_Blocksize ;
        macFrame[index ++] = 2;
        MacDot16ShortToTwoByte(macFrame[index],
                               macFrame[index + 1],
                               (UInt16)sFlow->arqParam->arqBlockSize);
        index += 2;
    }//end of if (sFlow->isARQEnabled )



    if (dot16->dot16eEnabled && isSleepModeEnabled)
    {
        macFrame[index ++] = TLV_DSX_TrafficIndicationPreference;
        macFrame[index ++] = 1;
        macFrame[index ++] = (UInt8)sFlow->TrafficIndicationPreference;
    }

    if (sFlow->dsaInfo.dsxCC > DOT16_FLOW_DSX_CC_OKSUCC) // unsuccessful
    {
        // CS specification
        // for IPv4 claafier
        macFrame[index ++] = TLV_DSX_CS_TYPE_IPv4;
        csSpecLenIndex = index;
        macFrame[index ++] = 1; // temporary length, will be adjust after
                                // adding all the CS TLVs
        macFrame[index ++] = 0; // Be sure to give the right value,
                                // now It is IPv4

        // TLV_CS_PACKET_ClassifierDscAction

        // TLV_CS_PACKET_ClassifierRule

        // reset CS param length
        macFrame[csSpecLenIndex] = (UInt8)(index - 1 - csSpecLenIndex);
        // Be sure the index is of right value
    }

    // reset service flow param length
    macFrame[sfParamLenIndex] = (UInt8)(index - 1 - sfParamLenIndex);
    // Be sure the index is of right value

    // Be sure to put HMAC tuple as the last TLV
    macFrame[index ++] = TLV_COMMON_HMacTuple;  // type
    macFrame[index ++] = 21; // length
    if (dot16->stationType == DOT16_SS)
    {
        macFrame[index ++] = dot16Ss->servingBs->authKeyInfo.hmacKeySeq;
    }
    else
    {
         macFrame[index ++] = 0; // Be sure to give the right value,
    }

    for (i = 0; i < 20; i ++)
    {
        // message digest

        macFrame[index ++] = 0;
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a DSA-RSP message with transId %d cid %d sfId %d\n",
                transId, sFlow->cid, sFlow->sfid);
    }

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16BuildDscReqMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSC-REQ mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow* : Pointer to the service flow
// + newQoSInfo : MacDot16QoSParameter* : Poniter to the new QoS info
// + cid       : Dot16CIDType : CID message shall use
// RETURN     :: Message* : Point to the message containing DSC-REQ PDU
// **/
Message* MacDot16BuildDscReqMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* sFlow,
             MacDot16QoSParameter* newQoSInfo,
             Dot16CIDType cid)
{
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16DscReqMsg* dscReq;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    int sfParamLenIndex;
    int i;
    int delayJitter;

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
    }
    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) (&macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, cid);

    dscReq = (MacDot16DscReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DscReqMsg);

    dscReq->type = DOT16_DSC_REQ;
    if (dot16->stationType == DOT16_SS)
    {
        // SS
        MacDot16ShortToTwoByte(dscReq->transId[0], dscReq->transId[1],
            dot16Ss->transId)
    }
    else
    {
        // BS
        MacDot16ShortToTwoByte(dscReq->transId[0], dscReq->transId[1],
            dot16Bs->transId)
    }

    // add service flow parameter TLVs
    // TLV_COMMON_UlServiceFlow or TLV_COMMON_DlServiceFlow
    if (sFlow->sfDirection == DOT16_UPLINK_SERVICE_FLOW)
    {
        macFrame[index ++] = TLV_COMMON_UlServiceFlow;
    }
    else
    {
        macFrame[index ++] = TLV_COMMON_DlServiceFlow;
    }
    sfParamLenIndex = index;
    macFrame[index ++] = 3; // temporary 3, will be adjust to
                            // right value later
    macFrame[index ++] = 0; // No meaning, for implementation purpose only

    // TLV_DSX_ServiceFlowId
    macFrame[index ++] = TLV_DSX_ServiceFlowId;
    macFrame[index ++] = 4;
    memcpy((char*) &(macFrame[index]), &(sFlow->sfid), 4);
    index += 4;


    // TLV_DSX_QoSParamSetType
    macFrame[index ++] = TLV_DSX_QoSParamSetType;
    macFrame[index ++] = 1;
    macFrame[index ++] = 0x6;

    // TLV_DSX_MaxSustainedRate
    macFrame[index ++] = TLV_DSX_MaxSustainedRate;
    macFrame[index ++] = 4;
    memcpy((char*) &macFrame[index],
           &(newQoSInfo->maxSustainedRate),
           4);
    index += 4;

    // TLV_DSX_MaxTrafficBurst
    macFrame[index ++] = TLV_DSX_MaxTrafficBurst;
    macFrame[index ++] = 4;
    memcpy((char*) &macFrame[index],
           &(newQoSInfo->maxPktSize),
           4);
    index += 4;

    // TLV_DSX_MinReservedRate
    macFrame[index ++] = TLV_DSX_MinReservedRate;
    macFrame[index ++] = 4;
    memcpy((char*) &macFrame[index],
           &(newQoSInfo->minReservedRate),
           4);
    index += 4;

    // TLV_DSX_MinTolerableRate
    macFrame[index ++] = TLV_DSX_MinTolerableRate;
    macFrame[index ++] = 4;
    memcpy((char*) &macFrame[index],
           &(newQoSInfo->minPktSize),
           4); // Be sure to give the right value
    index += 4;

    // TLV_DSX_ToleratedJitter
    macFrame[index ++] = TLV_DSX_ToleratedJitter;
    macFrame[index ++] = 4;
    // convert to MILLI_SECOND
    delayJitter = (int) (newQoSInfo->toleratedJitter /
                         MILLI_SECOND);
    if (delayJitter == 0)
    {
        delayJitter  = 1;
    }
    MacDot16IntToFourByte((char*) &macFrame[index],
                           delayJitter);
    index += 4;

    // TLV_DSX_MaxLatency
    macFrame[index ++] = TLV_DSX_MaxLatency;
    macFrame[index ++] = 4;
    // convert to MILLI_SECOND
    delayJitter = (int) (newQoSInfo->maxLatency /
                         MILLI_SECOND);
    if (delayJitter == 0)
    {
        delayJitter  = 1;
    }
    MacDot16IntToFourByte((char*) &macFrame[index],
                           delayJitter);
    index += 4;

    // reset service flow param length
    macFrame[sfParamLenIndex] = (UInt8)(index - 1 - sfParamLenIndex);
    // Be sure the index is of right value

    // Be sure to put HMAC tuple as the last TLV
    macFrame[index ++] = TLV_COMMON_HMacTuple;  // type
    macFrame[index ++] = 21; // length
    if (dot16->stationType == DOT16_SS)
    {
        macFrame[index ++] = dot16Ss->servingBs->authKeyInfo.hmacKeySeq;
    }
    else
    {
         macFrame[index ++] = 0; // Be sure to give the right value,
    }
    for (i = 0; i < 20; i ++)
    {
        // message digest

        macFrame[index ++] = 0; // Be sure to give the right value
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a DSC-REQ message with transId %d\n",
                dscReq->transId[0] * 256 + dscReq->transId[1]);
    }

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16BuildDscRspMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSC-Rsp mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + transId   : unsigned int : transaction Id
// + cid       : Dot16CIDType : CID message shall use
// RETURN     :: Message* : Point to the message containing DSC-RSP PDU
// **/
Message* MacDot16BuildDscRspMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             unsigned int transId,
             Dot16CIDType cid)
{
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs =NULL;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16DscRspMsg* dscRsp;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    int i;

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
    }

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, cid);

    dscRsp = (MacDot16DscRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DscRspMsg);

    dscRsp->type = DOT16_DSC_RSP;
    MacDot16ShortToTwoByte(dscRsp->transId[0], dscRsp->transId[1], transId);
    dscRsp->confirmCode = (UInt8)serviceFlow->dscInfo.dsxCC;


    // Be sure to put HMAC tuple as the last TLV
    macFrame[index ++] = TLV_COMMON_HMacTuple;  // type
    macFrame[index ++] = 21; // length
    if (dot16->stationType == DOT16_SS)
    {
        macFrame[index ++] = dot16Ss->servingBs->authKeyInfo.hmacKeySeq;
    }
    else
    {
         macFrame[index ++] = 0;
    }
    for (i = 0; i < 20; i ++)
    {
        // message digest

        macFrame[index ++] = 0; // Be sure to give the right value
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a DSC-RSP message with transId %d\n", transId);
    }

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16BuildDscAckMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSC-ACK mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + cid       : Dot16CIDType : CID message shall use
// + transId   : unsigned int : transaction Id
// RETURN     :: Message* : Point to the message containing DSA-ACK PDU
// **/
Message* MacDot16BuildDscAckMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             Dot16CIDType cid,
             unsigned int transId)
{
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16DscAckMsg* dscAck;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    int i;

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
    }

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, cid);

    dscAck = (MacDot16DscAckMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DscAckMsg);

    dscAck->type = DOT16_DSC_ACK;
    MacDot16ShortToTwoByte(dscAck->transId[0], dscAck->transId[1], transId);
    dscAck->confirmCode = (UInt8)serviceFlow->dscInfo.dsxCC;


    // Be sure to put HMAC tuple as the last TLV
    macFrame[index ++] = TLV_COMMON_HMacTuple;  // type
    macFrame[index ++] = 21; // length
    if (dot16->stationType == DOT16_SS)
    {
        macFrame[index ++] = dot16Ss->servingBs->authKeyInfo.hmacKeySeq;
    }
    else
    {
         macFrame[index ++] = 0; // Be sure to give the right value,
    }
    for (i = 0; i < 20; i ++)
    {
        // message digest

        macFrame[index ++] = 0;
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a DSC-ACK message with transId %d\n", transId);
    }

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16BuildDsdReqMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSD-REQ mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + cid       : Dot16CIDType : CID message shall use
// RETURN     :: Message* : Point to the message containing DSD-REQ PDU
// **/
Message* MacDot16BuildDsdReqMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             Dot16CIDType cid)
{
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16DsdReqMsg* dsdReq;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    int i;

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
    }

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, cid);

    dsdReq = (MacDot16DsdReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsdReqMsg);

    dsdReq->type = DOT16_DSD_REQ;
    MacDot16IntToFourByte(dsdReq->sfid, serviceFlow->sfid);
    if (dot16->stationType == DOT16_SS)
    {
        // SS
        MacDot16ShortToTwoByte(dsdReq->transId[0], dsdReq->transId[1],
            dot16Ss->transId);
    }
    else
    {
        // BS
        MacDot16ShortToTwoByte(dsdReq->transId[0], dsdReq->transId[1],
            dot16Bs->transId);
    }

    // Be sure to put HMAC tuple as the last TLV
    macFrame[index ++] = TLV_COMMON_HMacTuple;  // type
    macFrame[index ++] = 21; // length
    if (dot16->stationType == DOT16_SS)
    {
        macFrame[index ++] = dot16Ss->servingBs->authKeyInfo.hmacKeySeq;
    }
    else
    {
         macFrame[index ++] = 0; // Be sure to give the right value
    }
    for (i = 0; i < 20; i ++)
    {
        // message digest

        macFrame[index ++] = 0; // Be sure to give the right value,
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a DSD-REQ message with transId %d\n",
                dsdReq->transId[0] * 256 + dsdReq->transId[1]);
    }

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16BuildDsdRspMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSD_RSP mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + cid       : Dot16CIDType : CID message shall use
// + transId   : unsigned int : transaction Id
// RETURN     :: Message* : Point to the message containing DSD-RSP PDU
// **/
Message* MacDot16BuildDsdRspMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             Dot16CIDType cid,
             unsigned int transId)
{
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16DsdRspMsg* dsdRsp;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    int i;

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
    }

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, cid);

    dsdRsp = (MacDot16DsdRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsdRspMsg);

    dsdRsp->type = DOT16_DSD_RSP;
    dsdRsp->confirmCode = (UInt8)serviceFlow->dsdInfo.dsxCC;
    MacDot16ShortToTwoByte(dsdRsp->transId[0], dsdRsp->transId[1], transId);
    MacDot16IntToFourByte(dsdRsp->sfid, serviceFlow->sfid);

    // Be sure to put HMAC tuple as the last TLV
    macFrame[index ++] = TLV_COMMON_HMacTuple;  // type
    macFrame[index ++] = 21; // length
    if (dot16->stationType == DOT16_SS)
    {
        macFrame[index ++] = dot16Ss->servingBs->authKeyInfo.hmacKeySeq;
    }
    else
    {
         macFrame[index ++] = 0; // Be sure to give the right value,
    }
    for (i = 0; i < 20; i ++)
    {
        // message digest

        macFrame[index ++] = 0; // Be sure to give the right value,
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a DSD-RSP message with transId %d\n",
                transId);
    }

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16ResetDsxInfo
// LAYER      :: MAC
// PURPOSE    :: Build the DSA-RSP mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + dsxInfo   : MacDot16DsxInfo* : Pointer to the dsx information,
//                                  could be dsInfo for dsa/dsc/dsd
// RETURN     :: void : NULL
// **/
void MacDot16ResetDsxInfo(Node* node,
                          MacDataDot16* dot16,
                          MacDot16ServiceFlow* sFlow,
                          MacDot16DsxInfo* dsxInfo)
{
    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("reset dsxInfo for tranCid %d transId %d\n",
               sFlow->cid, dsxInfo->dsxTransId);
    }

    // free payload dsxReqCopy
    if (dsxInfo->dsxReqCopy != NULL)
    {
        MESSAGE_Free(node, dsxInfo->dsxReqCopy);
        dsxInfo->dsxReqCopy = NULL;
    }

    if (dsxInfo->dsxRspCopy != NULL)
    {
        MESSAGE_Free(node, dsxInfo->dsxRspCopy);
        dsxInfo->dsxRspCopy = NULL;
    }

    if (dsxInfo->dsxAckCopy != NULL)
    {
        MESSAGE_Free(node, dsxInfo->dsxAckCopy);
        dsxInfo->dsxAckCopy = NULL;
    }

    // free timers
    if (dsxInfo->timerT7 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dsxInfo->timerT7);
        dsxInfo->timerT7 = NULL;
    }

    if (dsxInfo->timerT8 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dsxInfo->timerT8);
        dsxInfo->timerT8 = NULL;
    }

    if (dsxInfo->timerT10 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dsxInfo->timerT10);
        dsxInfo->timerT10 = NULL;
    }

    if (dsxInfo->timerT14 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dsxInfo->timerT14);
        dsxInfo->timerT14 = NULL;
    }

    // free QosInfo
    if (dsxInfo->dscOldQosInfo != NULL)
    {
        MEM_free(dsxInfo->dscOldQosInfo);
        dsxInfo->dscOldQosInfo = NULL;
    }

    if (dsxInfo->dscNewQosInfo != NULL)
    {
        MEM_free(dsxInfo->dscNewQosInfo);
        dsxInfo->dscNewQosInfo = NULL;
    }

    dsxInfo->dsxRetry = DOT16_DEFAULT_DSX_REQ_RETRIES;
    dsxInfo->dsxTransId = DOT16_INVALID_TRANSACTION_ID;
    dsxInfo->dsxTransStatus = DOT16_FLOW_DSX_NULL;
}

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16Init
// LAYER      :: MAC
// PURPOSE    :: Initialize DOT16 MAC protocol at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + interfaceAddress : NodeAddress       : Interface address
// + nodeInput        : const NodeInput*  : Pointer to node input.
// + macProtocolName  : char*             : Name of different 802.16
//                                        : variants
// + numNodesInSubnet : int               : Number of nodes in subnet.
// + nodeIndexInSubnet: int               : index of the node in subnet.
// RETURN     :: void : NULL
// **/
void MacDot16Init(Node* node,
                  int interfaceIndex,
//                  NodeAddress interfaceAddress,
                  const NodeInput* nodeInput,
                  char* macProtocolName,
                  int numNodesInSubnet,
                  int nodeIndexInSubnet)
{
    MacDataDot16* dot16 = NULL;
    clocktype delay;

    BOOL wasFound;
    char stringVal[MAX_STRING_LENGTH];
    //Address ifAddress;
    if (DEBUG)
    {

        printf("Node%u is initializing 802.16 MAC on interface %d\n",
               node->nodeId, interfaceIndex);
        printf("    Total nodes in subnet is %d, my index is %d\n",
               numNodesInSubnet, nodeIndexInSubnet);
    }

    dot16 = (MacDataDot16*) MEM_malloc(sizeof(MacDataDot16));
    ERROR_Assert(dot16 != NULL,
                 "802.16: Unable to allocate memory for dot16 data struc.");

    // using memset to initialize the whole DOT16 data strucutre
    memset((char*)dot16, 0, sizeof(MacDataDot16));

    dot16->myMacData = node->macData[interfaceIndex];
    dot16->myMacData->macVar = (void*) dot16;

    RANDOM_SetSeed(dot16->seed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_DOT16,
                   interfaceIndex);

    // generate my MAC address
    //MacDot16Ipv4ToMacAddress(dot16->macAddress, interfaceAddress);
    Mac802Address mac802Addr;
    MacHWAddress tempHWAddr = GetMacHWAddress(node, interfaceIndex);
    ConvertVariableHWAddressTo802Address(
        node,
        &tempHWAddr,
        &mac802Addr);
    memcpy(&dot16->macAddress, &mac802Addr.byte,
        MAC_ADDRESS_LENGTH_IN_BYTE);
    // init system parameters
    MacDot16InitParameter(
        node, nodeInput, interfaceIndex, dot16);

    // configuration the  mobility support to enable DOT16e functionality
    // read the operation mode
    //NetworkGetInterfaceInfo(node, interfaceIndex, &ifAddress);
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-SUPPORT-MOBILITY",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "YES") == 0)
        {
            // support mobility (dot16e)
            dot16->dot16eEnabled = TRUE;
        }
        else if (strcmp(stringVal, "NO") == 0)
        {
            dot16->dot16eEnabled = FALSE;
        }
        else
        {
            ERROR_ReportWarning(
                "MAC802.16: Wrong value of MAC-802.16-SUPPORT-MOBILITY,"
                "shoudle be YES or NO. Use default value as NO.");

            // by default, a node supports mobility
            dot16->dot16eEnabled = FALSE;
        }
    }
    else
    {
        // not configured. Assume mobility support by default
        dot16->dot16eEnabled = FALSE;
    }

    if (DEBUG_PARAMETER)
    {
        // print out parameters
        MacDot16PrintParameter(node, dot16);
    }

    // initiate channel list
    MacDot16InitChannelList(node, dot16);
    dot16->lastListeningChannel = DOT16_INVALID_CHANNEL;

    // Init the PHY specific data first, so we can then initialize some
    // general variables
    MacDot16PhyInit(node, interfaceIndex, nodeInput);

    // get the duration of the physical slot after initializing PHY
    dot16->dlPsDuration = MacDot16PhyGetPsDuration(node, dot16, DOT16_DL);
    dot16->ulPsDuration = MacDot16PhyGetPsDuration(node, dot16, DOT16_UL);

    if (dot16->stationType == DOT16_BS)
    {
        // this is a BS node
        MacDot16BsInit(node, interfaceIndex, nodeInput);
    }
    else
    {
        // this is a SSnode
        MacDot16SsInit(node, interfaceIndex, nodeInput);
    }

    // init the Convergence Sublayer
    MacDot16CsInit(node, interfaceIndex, nodeInput);

    // init the schedulers
    MacDot16SchInit(node, interfaceIndex, nodeInput);

    // init the QOS
    MacDot16QosInit(node, interfaceIndex, nodeInput);

    // init the traffic conditoner
    MacDot16TcInit(node, interfaceIndex, nodeInput);

    // Set the operation start timer
    if (dot16->stationType == DOT16_BS)
    {
        // assume BSs starts from the beginning
        delay = 0;
    }
    else
    {
        // SS starts with a random delay
        delay = RANDOM_nrand(dot16->seed) % (1 * SECOND);
    }

    MacDot16SetTimer(node,
                     dot16,
                     DOT16_TIMER_OperationStart,
                     delay,
                     NULL,
                     0);
}


// /**
// FUNCTION   :: MacDot16ReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacDot16ReceivePacketFromPhy(Node* node,
                                  MacDataDot16* dot16,
                                  Message* msg)
{
    MacDot16PhyReceivePacketFromPhy(node, dot16, msg);
}

// /**
// FUNCTION   :: MacDot16ReceivePhyStatusChangeNotification
// LAYER      :: MAC
// PURPOSE    :: React to change in physical status.
// PARAMETERS ::
// + node             : Node*         : Pointer to node.
// + dot16            : MacDataDot16* : Pointer to Dot16 structure
// + oldPhyStatus     : PhyStatusType : The previous physical state
// + newPhyStatus     : PhyStatusType : New physical state
// + receiveDuration  : clocktype     : The receiving duration
// RETURN     :: void : NULL
// **/
void MacDot16ReceivePhyStatusChangeNotification(
         Node* node,
         MacDataDot16* dot16,
         PhyStatusType oldPhyStatus,
         PhyStatusType newPhyStatus,
         clocktype receiveDuration)
{
    if (DEBUG)
    {
        printf("MAC 802.16: Node%d phy status changed from %d to %d\n",
               node->nodeId, oldPhyStatus, newPhyStatus);
    }

    // handle PHY status change

    // pass to MAC Common Part Sublayer (CPS)
    if (dot16->stationType == DOT16_BS)
    {
        MacDot16BsReceivePhyStatusChangeNotification(
            node,
            dot16,
            oldPhyStatus,
            newPhyStatus,
            receiveDuration);
    }
    else
    {
        MacDot16SsReceivePhyStatusChangeNotification(
            node,
            dot16,
            oldPhyStatus,
            newPhyStatus,
            receiveDuration);
    }
}

// /**
// FUNCTION   :: MacDot16NetworkLayerHasPacketToSend
// LAYER      :: MAC
// PURPOSE    :: Called when network layer buffers transition from empty.
// PARAMETERS ::
// + node     : Node*        : Pointer to node.
// + dot16    : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: void        : NULL
// **/
void MacDot16NetworkLayerHasPacketToSend(Node *node, MacDataDot16 *dot16)
{
    Message* msg = NULL;
    Message* dupMsg = NULL;
    Mac802Address nextHopAddress;
    unsigned char macAddress[DOT16_MAC_ADDRESS_LENGTH];
    TosType priority;
    int networkType;
    MacDot16BsSsInfo* ssInfo = NULL;
    BOOL handled = FALSE;
    BOOL ssNextHopError = FALSE;
    while (1)
    {
        // peek top packet of the network queue
        MAC_OutputQueueTopPacket(node,
                                 dot16->myMacData->interfaceIndex,
                                 &msg,
                                 &nextHopAddress,
                                 &networkType,
                                 &priority);

        if (msg == NULL)
        {
            // no more packet
            return;
        }

        // for BS needs check the SS is in its range or not
        // this is useful when SS moves out of the old BS while BS has
        // a packet to it
        // for SS needs to check if the next hop of the packet
        // is the same as asociated BS (eg. after hanover 
        // or cell reselection), if different, report routing protocol 
        // for route refresh
        if (dot16->stationType == DOT16_BS)
        {
            memcpy(&macAddress, &nextHopAddress.byte,
                MAC_ADDRESS_LENGTH_IN_BYTE);

            if (!MacDot16IsMulticastMacAddress(node, dot16, macAddress))
            {
                // unicast packet
                ssInfo = MacDot16BsGetSsByMacAddress(node,
                                                     dot16,
                                                     macAddress);
                if (ssInfo == NULL)
                {
                    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

                    MAC_OutputQueueDequeuePacket(
                        node,
                        dot16->myMacData->interfaceIndex,
                        &msg,
                        &nextHopAddress,
                        &networkType,
                        &priority);

                    // free it as the packet cannot be handled
                    MAC_NotificationOfPacketDrop(
                        node,
                        nextHopAddress,
                        dot16->myMacData->interfaceIndex,
                        msg);

                    // update stats
                    dot16Bs->stats.numPktsDroppedUnknownSs ++;

                    return;
                }
            }
        }
        else
        {
            unsigned char bsMacAddress[DOT16_MAC_ADDRESS_LENGTH];

            MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
            memcpy(&macAddress, 
                   &nextHopAddress.byte,
                   MAC_ADDRESS_LENGTH_IN_BYTE);
            if (!(dot16Ss && dot16Ss->servingBs))
            {
                // if SS does n otfinish initialization or
                // if no BS is associated
                return;
            }
            memcpy(&bsMacAddress, 
                   &dot16Ss->servingBs->bsId,
                   DOT16_MAC_ADDRESS_LENGTH);
            
            // if unicast packet, 
            // see if match the associated BS's address
            if (!MacDot16IsMulticastMacAddress(node, dot16, macAddress) && 
                !MacDot16SameMacAddress(macAddress, bsMacAddress))
            {
                // next hop BS address is 
                // different from associated BS
                ssNextHopError = TRUE;
            }
        }

        // For safety, we duplicate the packet and pass to CS as the packet
        // is not dequeued yet.
        dupMsg = MESSAGE_Duplicate(node, msg);
        BOOL msgDropped = FALSE;
        handled = MacDot16CsPacketFromUpper(node,
                                            dot16,
                                            dupMsg,
                                            nextHopAddress,
                                            networkType,
                                            priority,
                                            &msgDropped);

        if (handled == TRUE)
        {
            MAC_OutputQueueDequeuePacket(
                node,
                dot16->myMacData->interfaceIndex,
                // actually dequeue this packet, which is already handled
                &msg,
                &nextHopAddress,
                &networkType,
                &priority);

            if (!msgDropped)
            {
                MESSAGE_CopyInfo(node, dupMsg, msg);
            }

            if (ssNextHopError)
            {
                Message* repMsg = NULL;

                // duplicate a msg for reporting purpose
                repMsg = MESSAGE_Duplicate(node, msg);
                
                // packet has been redirected to a new BS
                // notify routing protocol to refresh routing table
                // the packet could be routed by the BS to the desitnation
                // correctly, depending on the routing protocol
                MAC_NotificationOfPacketDrop(
                    node,
                    nextHopAddress,
                    dot16->myMacData->interfaceIndex,
                    repMsg);
            }
        
            // free it as the duplicated packet is already handled
            MESSAGE_Free(node, msg);
        }
        else
        {
            // unable to handle more packets, just return and let IP keep
            // rest packets in network queues until we have space in buff.

            // free the duplicated copy of the packet.
            MESSAGE_Free(node, dupMsg);

            if (ssNextHopError)
            {
                Message* repMsg = NULL;

                // duplicate a msg for reporting purpose
                repMsg = MESSAGE_Duplicate(node, msg);
                
                // packet has been redirected to a new BS
                // notify routing protocol to refresh routing table
                // msg will be freed there
                MAC_NotificationOfPacketDrop(
                    node,
                    nextHopAddress,
                    dot16->myMacData->interfaceIndex,
                    repMsg);
            }

            return;
        }
    }
}

// /**
// FUNCTION   :: MacDot16Layer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : int      : Interface index
// + msg              : Message* : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacDot16Layer(Node* node, int interfaceIndex, Message* msg)
{
    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;
    BOOL passOn = TRUE;

    if (DEBUG)
    {
        MacDot16PrintRunTimeInfo(node, dot16);

        printf("DOT16: interface%d get a message\n", interfaceIndex);
    }

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
                printf("DOT16: node%u interface%d get a timer message %d\n",
                       node->nodeId, interfaceIndex, timerType);
            }

            if (timerType >= DOT16_TIMER_CS_Begin &&
                timerType <= DOT16_TIMER_CS_End)
            {
                // timer message for CS sublayer, pass to it
                MacDot16CsLayer(node, interfaceIndex, msg);
                passOn = FALSE;
            }

            break;
        }

        default:
        {
            break;
        }
    }

    if (passOn)
    {
        if (dot16->stationType == DOT16_BS)
        {
            MacDot16BsLayer(node, interfaceIndex, msg);
        }
        else
        {
            MacDot16SsLayer(node, interfaceIndex, msg);
        }
    }
}


// /**
// FUNCTION   :: MacDot16Finalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16Finalize(Node *node, int interfaceIndex)
{
    MacDataDot16* dot16 = (MacDataDot16 *)
                          node->macData[interfaceIndex]->macVar;

    if (node->macData[interfaceIndex]->macStats == TRUE)
    {
        MacDot16PrintStats(node, interfaceIndex, dot16);

        if (dot16->stationType == DOT16_BS)
        {
            MacDot16BsFinalize(node, interfaceIndex);
        }
        else
        {
            MacDot16SsFinalize(node, interfaceIndex);
        }

        MacDot16CsFinalize(node, interfaceIndex);
        MacDot16PhyFinalize(node, interfaceIndex);
    }
}

// /**
// FUNCTION       :: MacDot16Ipv6ToMacAddress(macAddr, ipv6Addr)
// DESCRIPTION :: Convert an IPv6 address to MAC address
// **/
void MacDot16Ipv6ToMacAddress(unsigned char* macAddr, in6_addr addr)
{

            (macAddr)[0] = (unsigned char)(addr.s6_addr8[15]);
            (macAddr)[1] = (unsigned char)(addr.s6_addr8[14]);
            (macAddr)[2] = (unsigned char)(addr.s6_addr8[13]);
            (macAddr)[3] = (unsigned char)(addr.s6_addr8[12]);
            (macAddr)[4] = (unsigned char)(addr.s6_addr8[1]);
            (macAddr)[5] = (unsigned char)(addr.s6_addr8[0]);
}

// /**
// FUNCTION       :: MacDot16MacAddressToIpv6(macAddr)
// DESCRIPTION :: Convert an MAC address to IPv6 address
// **/
void MacDot16MacAddressToIpv6(in6_addr addr, unsigned char* macAddr)
{
            addr.s6_addr8[15] = (unsigned char)(macAddr)[0];
            addr.s6_addr8[14] = (unsigned char)(macAddr)[1];
            addr.s6_addr8[13] = (unsigned char)(macAddr)[2];
            addr.s6_addr8[12] = (unsigned char)(macAddr)[3];
            addr.s6_addr8[1] = (unsigned char)(macAddr)[4];
            addr.s6_addr8[0] = (unsigned char)(macAddr)[5];
}
// /**
// FUNCTION       :: MacDot16HandleFirstFragDataPdu
// DESCRIPTION :: Handle first fargment PDu
// **/
int MacDot16HandleFirstFragDataPdu(
    Node* node,
    MacDataDot16* dot16,
    Message* msg,
    MacDot16ServiceFlow* sFlow,
    UInt16 firstMsgFSN)
{
    sFlow->isFragStart = TRUE;
    sFlow->fragFSNno = firstMsgFSN;
    sFlow->noOfFragPacketReceived = 0;
    sFlow->fragFSNNoReceived[sFlow->noOfFragPacketReceived] =
        firstMsgFSN;
    sFlow->noOfFragPacketReceived++;
    if (sFlow->fragMsg != NULL)
    {
        // Drop existing incomplete fragmented msg
        if (DEBUG_PACKING_FRAGMENTATION)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Delete incomplete fragmented packet\n");
        }
        MESSAGE_FreeList(node, sFlow->fragMsg);
        sFlow->fragMsg = NULL;
    }
    sFlow->fragMsg = msg;
    return TRUE;
}// end of MacDot16HandleFirstFragDataPdu

// /**
// FUNCTION       :: MacDot16HandleLastFragDataPdu
// DESCRIPTION :: Handle last fargment PDu
// **/
int MacDot16HandleLastFragDataPdu(
    Node* node,
    MacDataDot16* dot16,
    Message* msg,
    MacDot16ServiceFlow* sFlow,
    UInt16 lastMsgFSN,
    unsigned char* lastHopMacAddr)
{
    Message* tempMsg = sFlow->fragMsg;
    MacDot16Bs* dot16Bs;
    MacDot16Ss* dot16Ss;
    BOOL isNeedToDrop = FALSE;
    MacDot16BsSsInfo* ssInfo = NULL;
    if (sFlow->isFragStart == FALSE ||
        tempMsg == NULL)
    {
        // If fragStart is TRUE then make it FALSE
        sFlow->isFragStart = FALSE;
        if (DEBUG_PACKING_FRAGMENTATION)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Drop the incomplete fragmented packet\n");
        }
        if (sFlow->fragMsg != NULL)
        {
            // Drop existing incomplete fragmented msg
            if (DEBUG_PACKING_FRAGMENTATION)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Delete incomplete fragmented packet\n");
            }
            MESSAGE_FreeList(node, sFlow->fragMsg);
            sFlow->fragMsg = NULL;
        }
        // Drop the packet
        MESSAGE_Free(node, msg);
        return FALSE;
    }

    sFlow->isFragStart = FALSE;
    sFlow->bytesSent = 0;
    while (tempMsg->next != NULL)
    {
        tempMsg = tempMsg->next;
    }
    tempMsg->next = msg;
    msg->next = NULL;

    sFlow->fragFSNNoReceived[sFlow->noOfFragPacketReceived] =
        lastMsgFSN;
    sFlow->noOfFragPacketReceived++;

// NOTE: QualNet drop the complete burst instead of PDU
// due to that reason this check is required.
// In future we can remove this check from code
// and can use modulus procedure for fsn no.
    // Check all fragmented packet belongs to single message
    tempMsg = sFlow->fragMsg;
    while (tempMsg != NULL)
    {
        if ((tempMsg->sequenceNumber != sFlow->fragMsg->sequenceNumber) ||
            (tempMsg->originatingNodeId != sFlow->fragMsg->originatingNodeId))
        {
            isNeedToDrop = TRUE;
            break;
        }
        tempMsg = tempMsg->next;
    }// end of while

    if (isNeedToDrop == FALSE)
    {
        tempMsg = sFlow->fragMsg;
        int nextExpectedFSNno = sFlow->fragFSNno;
        int i = 0;
        while (tempMsg != NULL)
        {
            if (nextExpectedFSNno != sFlow->fragFSNNoReceived[i])
            {
                isNeedToDrop = TRUE;
                break;
            }
            i++;
            nextExpectedFSNno++;
            nextExpectedFSNno = nextExpectedFSNno %
                DOT16_MAX_NO_FRAGMENTED_PACKET;
            tempMsg = tempMsg->next;
        }
    }

    if (isNeedToDrop == FALSE)
    {
    // Receiver has received all fragmented packet so fwd it to upper layer
    // Reassemble Packet
        Message* reassembleMsg = sFlow->fragMsg;
        int realPayloadSize = 0;
        int virtualPayloadSize = 0;
        char* payload;
        for (int i = 0; (i < sFlow->noOfFragPacketReceived)
            && (reassembleMsg != NULL); i ++)
        {
            realPayloadSize += MESSAGE_ReturnActualPacketSize(reassembleMsg);
            virtualPayloadSize +=
                MESSAGE_ReturnVirtualPacketSize(reassembleMsg);
            reassembleMsg = reassembleMsg->next;
        }

        reassembleMsg = MESSAGE_Alloc(node, 0, 0, 0);
        MESSAGE_PacketAlloc(
            node,
            reassembleMsg,
            realPayloadSize,
            (TraceProtocolType)sFlow->fragMsg->originatingProtocol);

    // copy info fields from the first fragment
        MESSAGE_CopyInfo(node, reassembleMsg, sFlow->fragMsg);

        // copy other simulation specific information from the first fragment
        reassembleMsg->sequenceNumber = sFlow->fragMsg->sequenceNumber;
        reassembleMsg->originatingNodeId = sFlow->fragMsg->originatingNodeId;
        reassembleMsg->originatingProtocol =
            sFlow->fragMsg->originatingProtocol;
        reassembleMsg->numberOfHeaders = sFlow->fragMsg->numberOfHeaders;
        reassembleMsg->next = NULL;
        for (int i = 0; i < reassembleMsg->numberOfHeaders; i ++)
        {
            reassembleMsg->headerProtocols[i] =
                sFlow->fragMsg->headerProtocols[i];
            reassembleMsg->headerSizes[i] = sFlow->fragMsg->headerSizes[i];
        }

            // reassemble the payload
        payload = MESSAGE_ReturnPacket(reassembleMsg);
        tempMsg = sFlow->fragMsg;
        for (int i = 0; i < sFlow->noOfFragPacketReceived; i ++)
        {
            realPayloadSize = MESSAGE_ReturnActualPacketSize(tempMsg);

            // copy the real payload
            memcpy(payload,
                   MESSAGE_ReturnPacket(tempMsg),
                   realPayloadSize);
            payload += realPayloadSize;
            tempMsg = tempMsg->next;
        }

        // add the virtual payload
        MESSAGE_AddVirtualPayload(node, reassembleMsg, virtualPayloadSize);

        if (dot16->stationType == DOT16_BS)
        {
            dot16Bs = (MacDot16Bs*) dot16->bsData;
            // increase statistics
            dot16Bs->stats.numPktsFromLower ++;
            // update dynamic stats
            if (node->guiOption)
            {
                GUI_SendIntegerData(node->nodeId,
                                    dot16Bs->stats.numPktFromPhyGuiId,
                                    dot16Bs->stats.numPktsFromLower,
                                    node->getNodeTime());
            }
            ssInfo = MacDot16BsGetSsByCid(
                         node,
                         dot16,
                         sFlow->cid);
        }
        else
        {
            dot16Ss = (MacDot16Ss*) dot16->ssData;
            // increase statistics
            dot16Ss->stats.numPktsFromLower ++;
            // update dynamic stats
            if (node->guiOption)
            {
                GUI_SendIntegerData(node->nodeId,
                                    dot16Ss->stats.numPktFromPhyGuiId,
                                    dot16Ss->stats.numPktsFromLower,
                                    node->getNodeTime());
            }
        }
        // pass to CS layer
        MacDot16CsPacketFromLower(node,
                                  dot16,
                                  reassembleMsg,
                                  lastHopMacAddr,
                                  ssInfo);

        if (DEBUG_PACKING_FRAGMENTATION)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Delete fragmented message list after"
                " successful transmission to upper layer\n");
        }
    }
    else
    {
        if (DEBUG_PACKING_FRAGMENTATION)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Delete fragmented message list after"
                " unsuccessful transmission to upper layer\n");
        }
    }
    // some packet is still remaining or dropped
    // Drop existing incomplete fragmented msg
    MESSAGE_FreeList(node, sFlow->fragMsg);
    sFlow->fragMsg = NULL;
    return TRUE;
}// end of MacDot16HandleLastFragDataPdu

// /**
// FUNCTION       :: MacDot16ARQSetRetryAndBlockLifetimeTimer
// DESCRIPTION :: ARQ set Retry and Block life time timer
// **/
void  MacDot16ARQSetRetryAndBlockLifetimeTimer(
    Node* node,
    MacDataDot16* dot16,
    MacDot16ServiceFlow* sFlow,
    char* infoPtr,
    UInt16 numArqBlocks ,
    UInt16 bsnId)
{
    //ARQ_RETRY_TIMER needs to be set for all the blocks being sent
    clocktype tempDelay = DOT16_ARQ_RETRY_TIMEOUT;
    int infoVal1 = 0;
    int infoVal2 = 0;

    if (tempDelay > 0)
    {
        infoVal1= sFlow->cid;
        infoVal2= (unsigned)(bsnId )|((unsigned)numArqBlocks<<16 );
        //infoVal2 = (Num arq blocks)(starting BSN id )
        //              16 bits         16 bits
        MacDot16SetTimer(node,
                     dot16,
                     DOT16_ARQ_RETRY_TIMER,
                     tempDelay,
                     NULL,
                     infoVal1,
                     infoVal2);
        if (DEBUG_ARQ_TIMER)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            printf("\nStarting ARQ Retry timer at %s seconds for node%d"
                    " Sflow %d StartingBSNId %d NumberOfARQBlocks %d \n"
                    ,clockStr,node->nodeId,sFlow->cid,bsnId,numArqBlocks);
            TIME_PrintClockInSecond(node->getNodeTime()+tempDelay, clockStr);
            printf("Timer to expire at %s seconds\n",clockStr);
        }
    }
    //now check if we need to set any arq block lifetime timer.

    if (!(infoPtr == NULL))
    {
        tempDelay = DOT16_ARQ_BLOCK_LIFETIME;

        if (tempDelay > 0)
        {
            // Start ARQ_BLOCK_LIFETIME_TIMER for the blocks mentioned
            UInt8 numARQNotSentBlocks = (UInt8)infoPtr[0];
            UInt16 notSentStartBsn = 0;
            infoVal2 = 0;

            MacDot16TwoByteToShort(notSentStartBsn,
                (unsigned char)infoPtr[1],
                (unsigned char)infoPtr[2]);

            infoVal1 = sFlow->cid;
            infoVal2 = (unsigned)(notSentStartBsn)|
                ((unsigned)numARQNotSentBlocks << 16);
            MacDot16SetTimer(node,
                 dot16,
                 DOT16_ARQ_BLOCK_LIFETIME_TIMER,
                 tempDelay,
                 NULL,
                 infoVal1,
                 infoVal2);
            if (DEBUG_ARQ_TIMER)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("\nStarting ARQ Block Lifetime timer at %s seconds for "
                    "node%d Sflow %d StartingBSNId %d NumberOfARQBlocks %d \n"
                    ,clockStr,node->nodeId,sFlow->cid,notSentStartBsn,
                                                        numARQNotSentBlocks);
                TIME_PrintClockInSecond(node->getNodeTime()+tempDelay, clockStr);
                printf("Timer to expire at %s seconds\n",clockStr);
            }
        }
    }
}//end of function. MacDot16ARQSetRetryAndBlockLifetimeTimer

// /**
// FUNCTION       :: MacDot16ARQFindBSNInWindow
// DESCRIPTION :: ARQ find BSN in window
// **/
int MacDot16ARQFindBSNInWindow(
    MacDot16ServiceFlow* sFlow,
    Int16 incomingBsnId,
    UInt16 numArqBlocks,
    BOOL &isPresent,
    UInt16 &numUnACKArqBloacks)
{
    Int16 i;
    Int16 bsnId = 0;
    numUnACKArqBloacks = 0;
    for (i = 0; i < numArqBlocks; i++)
    {
        bsnId = (incomingBsnId + i) % DOT16_ARQ_BSN_MODULUS;
        isPresent = MacDot16ARQCheckIfBSNInWindow(sFlow, bsnId);
        if (isPresent)
        {
            numUnACKArqBloacks = numArqBlocks - i;
            break;
        }
    }

    if (!isPresent)
    {
        return 0;
    }
    else
    {
        // we need to return the index in the ARQ array where the ARQ block is
        //present.
        int index;
        MacDot16ARQBlock* arqBlockPtr = NULL;
        // Bug fix # 5434 start
        int tempIndex;
        // Bug fix # 5434 end
        UInt16 frontBsnId;
        // As per section 6.3.4.4.1.1 and 6.3.4.4.11.2 ARQ_RX_WINDOW_START
        // and ARQ_TX_WINDOW_START are defined as below:
        // ARQ_TX_WINDOW_START : All BSN upto (ARQ_TX_WINDOW_START - 1) has
        // been acknowledged
        // ARQ_RX_WINDOW_START : All BSN upto ARQ_RX_WINDOW_START - 1 has
        // been correctly received.
        // This implies that BSN corresponding to above is the expected start
        // It might happen that some blocks are lost/not recived but above
        // variable will always point to expected BSN.and thus is the
        // correct start of window. Any new BSN should be greater or equal to
        // this.The variable front on the other handd represents the index 
        // from where the block is available.
        if (sFlow->arqControlBlock->direction == DOT16_ARQ_TX)
        {
            frontBsnId = DOT16_ARQ_TX_WINDOW_START;
        }
        else
        {
            frontBsnId = DOT16_ARQ_RX_WINDOW_START;
        }
        // Calculate the index of frontBsnId
        index = frontBsnId % DOT16_ARQ_WINDOW_SIZE;

        MacDot16ARQSetARQBlockPointer(index);
        if (arqBlockPtr->blockPtr != NULL)
        {
            frontBsnId = MacDot16Get11bit(DOT16_ARQ_FRAG_SUBHEADER);
        }
        tempIndex = index;
        if (tempIndex == 0)
        {
            tempIndex = DOT16_ARQ_WINDOW_SIZE;
        }
        else
        {
            MacDot16ARQDecIndex(tempIndex);
        }

        // Bug fix # 5434 start
        for (; index != tempIndex;
            MacDot16ARQIncIndex(index),MacDot16ARQIncBsnId(frontBsnId))
        {
            if (frontBsnId == bsnId)
            {
                return index;
            }
        }

        // return in case of last index too
        if (frontBsnId == bsnId)
        {
            return index;
        }
         // Bug fix # 5434 end
        isPresent = FALSE;
        return 0;
    }

}//end of MacDot16ARQFindBSNInWindow


// /**
// FUNCTION       :: MacDot16ARQHandleResetRetryTimeOut
// DESCRIPTION :: ARQ handle Reset retry time out timer
// **/
void MacDot16ARQHandleResetRetryTimeOut(Node* node,
                                   MacDataDot16* dot16,
                                   UInt16 cid,
                                   UInt8 resetMsgType)
{
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16BsSsInfo* ssInfo  = NULL;

    if (DEBUG_ARQ_TIMER)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Expire MacDot16ARQHandleResetRetryTimeOut ");
        printf("cid = %d, Msg Type = %d\n", cid, resetMsgType);
    }
    //Get the service Flow.
    if (dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
        MacDot16BsGetServiceFlowByCid(node,
            dot16,
            cid,
            &ssInfo,
            &sFlow);
        if (sFlow == NULL || ssInfo == NULL)
        {
            //bug fix start #5372
            if (DEBUG)
            {
                ERROR_ReportWarning("No service flow / SS info associated "
                                    " with the timer expired \n");
            }
            //bug fix end #5372
            return ;
        }
    }
    else
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        sFlow =MacDot16SsGetServiceFlowByCid(node,
            dot16,
            cid);
        if (sFlow == NULL)
        {
            //bug fix start #5372
            if (DEBUG)
            {
                ERROR_ReportWarning("No service flow associated with the "
                                    "timer expired \n");
            }
            //bug fix end #5372
            return ;
        }
    }
    sFlow->timerT22 = NULL;
    sFlow->arqControlBlock->numOfARQResetRetry++;
    if (sFlow->arqControlBlock->numOfARQResetRetry >=
        DOT16_ARQ_MAX_RESET_RETRY)
    {
        sFlow->arqControlBlock->numOfARQResetRetry = 0;
        // re initialize MAC
        //MacDot16SsRestartOperation(node, dot16);
        return;
    }
    MacDot16ARQBuildandSendResetMsg(node,
        dot16,
        cid,
        resetMsgType);
    return;
}// end of MacDot16ARQHandleRetryTimeOut

// /**
// FUNCTION       :: MacDot16ARQBuildandSendResetMsg
// DESCRIPTION :: ARQ build and send reset message
// **/
void MacDot16ARQBuildandSendResetMsg(Node* node,
                                     MacDataDot16* dot16,
                                     UInt16 cid,
                                     UInt8 resetMsgFlag)
{
    unsigned char payload[DOT16_MAX_MGMT_MSG_SIZE] = {0, 0};
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16BsSsInfo* ssInfo  = NULL;
    Message* resetMsg = NULL;
    MacDot16MacHeader* macHeader;
    int index = 0;

    //now we need to send this as a management packet.
    macHeader = (MacDot16MacHeader*) &(payload[index]);
    index += sizeof(MacDot16MacHeader);
    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));

    //Get the service Flow.
    if (dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
        MacDot16BsGetServiceFlowByCid(node,
            dot16,
            cid,
            &ssInfo,
            &sFlow);
        if (sFlow == NULL || ssInfo == NULL)
        {
            //bug fix start #5372
            if (DEBUG)
            {
                ERROR_ReportWarning("No service flow / SS info associated"
                                    " with the timer expired \n");
            }
            //bug fix end #5372
            return ;
        }
        MacDot16MacHeaderSetCID(macHeader, ssInfo->basicCid);
    }
    else
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        sFlow =MacDot16SsGetServiceFlowByCid(node,
            dot16,
            cid);
        if (sFlow == NULL)
        {
            //bug fix start #5372
            if (DEBUG)
            {
                ERROR_ReportWarning("No service flow associated with the "
                                    "timer expired \n");
            }
            //bug fix end #5372

            return ;
        }
        MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->basicCid);
    }

    // Send reset message
    payload[index++] = DOT16_ARQ_RESET;
    // set service flow CID
    MacDot16ShortToTwoByte(payload[index], payload[index + 1], sFlow->cid);
    index += 2;
    payload[index++] = resetMsgFlag;
    MacDot16MacHeaderSetLEN(macHeader, index);

    resetMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(resetMsg != NULL, "MAC802.16: Out of memory!");
    MESSAGE_PacketAlloc(node, resetMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(resetMsg), payload, index);

    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("ARQ Send Reset Message.");
        printf("sflow cid = %d\n", sFlow->cid);
    }

    // disable transmission
    if (resetMsgFlag == DOT16_ARQ_RESET_Initiator)
    {
        sFlow->arqControlBlock->isARQBlockTransmisionEnable = FALSE;
        sFlow->arqControlBlock->waitForARQResetType = DOT16_ARQ_RESET_ACK_RSP;
    }

    if ((resetMsgFlag == DOT16_ARQ_RESET_Initiator)
        || (sFlow->arqControlBlock->direction == DOT16_ARQ_TX
            && resetMsgFlag == DOT16_ARQ_RESET_ACK_RSP))
    {
        if (sFlow->timerT22 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->timerT22);
            sFlow->timerT22 = NULL;
        }
        if (DOT16_ARQ_DEFAULT_T22_INTERVAL > 0)
        {
            // Start t22 timer
            sFlow->timerT22 = MacDot16SetTimer(node,
                dot16,
                DOT16_TIMER_T22,
                DOT16_ARQ_DEFAULT_T22_INTERVAL,
                NULL,
                sFlow->cid,
                resetMsgFlag);
        }
    }

    if (dot16->stationType == DOT16_BS)
    {
        MacDot16BsScheduleMgmtMsgToSs(node,
                                    dot16,
                                    ssInfo,
                                    ssInfo->basicCid,
                                    resetMsg);
    }
    else
    {
          MacDot16SsEnqueueMgmtMsg(node,
                                   dot16,
                                   DOT16_CID_BASIC,
                                   resetMsg);
    }
}// end of MacDot16ARQBuildandSendResetMsg

// /**
// FUNCTION       :: MacDot16ARQHandleResetMsg
// DESCRIPTION :: ARQ handle reset message
// **/
int MacDot16ARQHandleResetMsg(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    int index = startIndex;
    UInt16 cid;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16Ss* dot16Ss = NULL;
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16BsSsInfo* ssInfo = NULL;
    UInt8 arqResetType;

    // handle reset message
    index += sizeof(MacDot16MacHeader);
    index++;
    MacDot16TwoByteToShort(cid, macFrame[index], macFrame[index + 1]);
    index += 2;
    arqResetType = macFrame[index++];
    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("MacDot16ARQHandleResetMsg: cid = %d, msg Type = %d\n",
            cid, arqResetType);
    }

    //Get the service Flow.
    if (dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
        MacDot16BsGetServiceFlowByCid(node,
            dot16,
            cid,
            &ssInfo,
            &sFlow);
        if (sFlow == NULL || ssInfo == NULL)
        {
            //bug fix start #5372
            if (DEBUG)
            {
                ERROR_ReportWarning("No service flow / SsInfo associated with"
                " the timer expired \n");
            }
            //bug fix end #5372
                return(index - startIndex);
        }
    }
    else
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        sFlow = MacDot16SsGetServiceFlowByCid(node,
            dot16,
            cid);
        if (sFlow == NULL)
        {
            //bug fix start #5372
            if (DEBUG)
            {
                ERROR_ReportWarning("No service flow associated with the"
                                    "timer expired \n");
            }
            //bug fix end #5372
                return(index - startIndex);
        }
    }
    // BUG_FIX: Found during when sFlow is not activated and admitted
    if (sFlow->arqControlBlock == NULL)
    {
        return(index - startIndex);
    }
    if (sFlow->arqControlBlock->direction == DOT16_ARQ_RX
        && arqResetType == DOT16_ARQ_RESET_Initiator)
    {

        // discard all incomplete sdus
        // deliver all complete sdus
        MacDot16ARQBuildSDU(node,
                            dot16,
                            sFlow,
                            ssInfo);
        // set ARQ window rx start to 0
        MacDot16ARQResetWindow(node, dot16, sFlow);
        // send ARQ reset with DOT16_ARQ_RESET_ACK_RSP
        MacDot16ARQBuildandSendResetMsg(node, dot16, cid,
            DOT16_ARQ_RESET_ACK_RSP);
    }
    else if (sFlow->arqControlBlock->direction == DOT16_ARQ_TX
        && arqResetType == DOT16_ARQ_RESET_ACK_RSP)
    {

        // set ARQ window Tx start to 0
        // Discard SDus with blocks in discarded state
        MacDot16ARQResetWindow(node, dot16, sFlow);
        // enable transmission
        sFlow->arqControlBlock->isARQBlockTransmisionEnable = TRUE;
        sFlow->arqControlBlock->numOfARQResetRetry = 0;
    }
    else if (sFlow->arqControlBlock->direction == DOT16_ARQ_TX
        && arqResetType == DOT16_ARQ_RESET_Initiator)
    {
        if (sFlow->arqControlBlock->waitForARQResetType !=
            DOT16_ARQ_RESET_ACK_RSP)
        {
            sFlow->arqControlBlock->isARQBlockTransmisionEnable = FALSE;
            // Send reset message with 0x01
            MacDot16ARQBuildandSendResetMsg(node,
                dot16,
                cid,
                (UInt8)DOT16_ARQ_RESET_ACK_RSP);
        }
    }
    else if (sFlow->arqControlBlock->direction == DOT16_ARQ_RX
        && arqResetType == DOT16_ARQ_RESET_ACK_RSP)
    {

        // discard all incomplete sdus
        // deliver all complete sdus
        MacDot16ARQBuildSDU(node,
                            dot16,
                            sFlow,
                            ssInfo);
        // set ARQ window rx start to 0
        MacDot16ARQResetWindow(node, dot16, sFlow);
        // endable ARQ connection
        sFlow->arqControlBlock->isARQBlockTransmisionEnable = TRUE;
        // send ARQ reset with DOT16_ARQ_RESET_Confirm_Initiator
        MacDot16ARQBuildandSendResetMsg(node,
            dot16,
            cid,
            (UInt8)DOT16_ARQ_RESET_Confirm_Initiator);
    }
    else if (sFlow->arqControlBlock->direction == DOT16_ARQ_TX
        && arqResetType == DOT16_ARQ_RESET_Confirm_Initiator)
    {

        // discard all incomplete sdus
        // deliver all complete sdus
        // set ARQ window tx start to 0
        MacDot16ARQResetWindow(node, dot16, sFlow);

        // endable ARQ connection
        sFlow->arqControlBlock->isARQBlockTransmisionEnable = TRUE;
    }
    return(index - startIndex);
}// end of MacDot16ARQHandleResetMsg

// /**
// FUNCTION       :: MacDot16ARQHandleTimeout
// DESCRIPTION :: ARQ handle time out timer message
// **/
// Handles Retry,BlockLifeTime,Purge Timeout.
void MacDot16ARQHandleTimeout(Node* node,
                              MacDataDot16* dot16,
                              MacDot16Timer* timerInfo)
{
    MacDot16Bs* dot16Bs = NULL;
    MacDot16Ss* dot16Ss = NULL;
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16BsSsInfo* ssInfo = NULL;
    Int16 bsnId = 0;
    Dot16CIDType cid = 0;
    int bsnIndex = 0;
    int tempIndex = 0 ;
    UInt16 numArqBlocks = 0;
    BOOL isPresent = FALSE;
    UInt16 numUnACKArqBloacks = 0;
    MacDot16ARQBlock* arqBlockPtr = NULL;

    cid = (Dot16CIDType)timerInfo->info;
    bsnId = (UInt16) (timerInfo->Info2 & 0x0000ffff);
    numArqBlocks = (UInt16)(timerInfo->Info2 >> 16);


    //Get the service Flow.
    if (dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
        MacDot16BsGetServiceFlowByCid(node,
            dot16,
            cid,
            &ssInfo,
            &sFlow);
        if (sFlow == NULL || ssInfo == NULL)
        {
            //bug fix start #5372
            if (DEBUG)
            {
                ERROR_ReportWarning("No service flow / SsInfo associated with"
                " the timer expired \n");
            }
            //bug fix end #5372

            return ;
        }
    }
    else
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        sFlow = MacDot16SsGetServiceFlowByCid(node,
            dot16,
            cid);
        if (sFlow == NULL)
        {
            //bug fix start #5372
            if (DEBUG)
            {
                ERROR_ReportWarning("No service flow associated with the timer"
                                "expired \n");
            }
            //bug fix end #5372

            return ;
        }
    }

    if (DEBUG_ARQ_TIMER)
    {
            MacDot16PrintRunTimeInfo(node, dot16);
            switch(timerInfo->timerType)
            {
                case DOT16_ARQ_RETRY_TIMER:
                    printf("DOT16_ARQ_RETRY_TIMER");
                    break;
                case DOT16_ARQ_BLOCK_LIFETIME_TIMER:
                    printf("DOT16_ARQ_BLOCK_LIFETIME_TIMER");
                    break;
                case DOT16_ARQ_DISCARD_RETRY_TIMER:
                    printf("DOT16_ARQ_DISCARD_RETRY_TIMER");
                    break;
                case DOT16_ARQ_RX_PURGE_TIMER:
                    printf("DOT16_ARQ_RX_PURGE_TIMER");
                    break;
                default:
                    break;
            }
            printf(" expired. cid = %d, bsnId = %d, NumARQblocks = %d\n",
                cid, bsnId, numArqBlocks);
          MacDot16ARQPrintControlBlock(node, sFlow);
        }

    if (timerInfo->timerType == DOT16_ARQ_DISCARD_RETRY_TIMER )
    {
        // we need to check if there are any more discarded messages
        // and send a collective discard message for all of them.
        // tempindex now represents a cumulative discard BsnId
        // sending discard message
        // Send discard message if BSN in window
        bsnIndex = (UInt16)MacDot16ARQFindBSNInWindow(
                                         sFlow,
                                         (UInt16)sFlow->arqDiscardBlockBSNId,
                                         numArqBlocks,
                                         isPresent,
                                         numUnACKArqBloacks);
        if (isPresent)
        {
            MacDot16ARQBuildAndSendDiscardMsg(
               node,
               dot16,
               sFlow,
               ssInfo,
               cid,
               sFlow->arqDiscardBlockBSNId);
            sFlow->arqDiscardMsgSent = TRUE;
        }
        MacDot16SetTimer(node,
                         dot16,
                         DOT16_ARQ_DISCARD_RETRY_TIMER,
                         DOT16_ARQ_RETRY_TIMEOUT,
                         NULL,
                         (UInt16)sFlow->cid,
                         (UInt16)sFlow->arqDiscardBlockBSNId);
    }
    if (timerInfo->timerType == DOT16_ARQ_RX_PURGE_TIMER )
    {
        numArqBlocks = 1;
    }
    bsnIndex = (UInt16)MacDot16ARQFindBSNInWindow(sFlow,
                                           bsnId,
                                           numArqBlocks,
                                           isPresent,
                                           numUnACKArqBloacks);

    numArqBlocks = numUnACKArqBloacks;

    if (!isPresent)
    {
        if (DEBUG_ARQ)
        {
            printf("ARQ Block has been acknowledged successfully.\n");
        }
        //ARQ block is not in window.ie it has been acknowledged.
       // so nothing needs to be done
        return;
    }
    else if (timerInfo->timerType == DOT16_ARQ_RETRY_TIMER)
    {
        //ARQ Block present in the window
        // we need to change the state of these blocks from
        //"outstanding" to "waiting for retransmission"

        if (DEBUG_ARQ)
        {
            printf("Changed the state of the blocks to"
                    " DOT16_ARQ_BLOCK_WAIT_FOR_RETRANSMISSION \n");
        }
        for (tempIndex = bsnIndex; numArqBlocks > 0;
             MacDot16ARQIncIndex(tempIndex))
        {
            numArqBlocks--;
            MacDot16ARQSetARQBlockPointer(tempIndex);
            if (arqBlockPtr->arqBlockState == DOT16_ARQ_BLOCK_OUTSTANDING)
            {
                arqBlockPtr->arqBlockState =
                    DOT16_ARQ_BLOCK_WAIT_FOR_RETRANSMISSION;
            }
        }
    }
    else if (timerInfo->timerType == DOT16_ARQ_BLOCK_LIFETIME_TIMER)
    {
        int tempNumARQBlocks = numArqBlocks;
        UInt16 tempBSNId;
        //We need to mark the blocks as "Discarded"
        for (tempIndex = bsnIndex; tempNumARQBlocks > 0;
                                        MacDot16ARQIncIndex(tempIndex))
         {
            tempNumARQBlocks--;
            MacDot16ARQSetARQBlockPointer(tempIndex);
            arqBlockPtr->arqBlockState = DOT16_ARQ_BLOCK_DISCARDED ;
         }

        if (DEBUG_ARQ)
        {
            printf("Marked the Blocks as discarded\n");
        }
        MacDot16ARQSetARQBlockPointer(bsnIndex);
        tempBSNId = (MacDot16Get11bit(DOT16_ARQ_FRAG_SUBHEADER) +
            (numArqBlocks - 1)) % DOT16_ARQ_BSN_MODULUS;

        sFlow->arqDiscardBlockBSNId = tempBSNId;
        //If discard Message has not been sent then we will send it.
        if (!sFlow->arqDiscardMsgSent)
        {
            //Build discard Message
            MacDot16ARQBuildAndSendDiscardMsg(node,
                                              dot16,
                                              sFlow,
                                              ssInfo,
                                              cid,
                                              tempBSNId);
             sFlow->arqDiscardMsgSent = TRUE;
            //now we also need to make sure that the discard msg is sent
            //after every ARQ_RETRY_TIMEOUT time.
            MacDot16SetTimer(node,
                             dot16,
                             DOT16_ARQ_DISCARD_RETRY_TIMER,
                             DOT16_ARQ_RETRY_TIMEOUT,
                             NULL,
                            (UInt16)sFlow->cid,
                            (UInt16)tempBSNId);
        }
    }
    else if (timerInfo->timerType == DOT16_ARQ_RX_PURGE_TIMER)
    {
        UInt16 windowIncrement= 0;
        UInt16 tempBSNId;
        //UInt16 index = bsnIndex;
        MacDot16ARQSetARQBlockPointer(bsnIndex);
        arqBlockPtr->arqRxPurgeTimer = NULL;
        // Bug fix # 5434 start
        // As per section 6.3.4.6.3 we need to mark blocks from
        // DOT16_ARQ_RX_WINDOW_START to bsnIndex
        // as received.

        for (tempIndex = DOT16_ARQ_RX_WINDOW_START; tempIndex != bsnIndex;
            MacDot16ARQIncIndex(tempIndex))
        // Bug fix # 5434 end
        {
            MacDot16ARQSetARQBlockPointer(tempIndex);
            arqBlockPtr->arqBlockState = DOT16_ARQ_BLOCK_RECEIVED;
        }
        MacDot16ARQSetARQBlockPointer(bsnIndex);
        arqBlockPtr->arqBlockState = DOT16_ARQ_BLOCK_RECEIVED;
        tempBSNId = MacDot16Get11bit(DOT16_ARQ_FRAG_SUBHEADER);
        DOT16_ARQ_RX_WINDOW_START = MacDot16ARQIncBsnId(tempBSNId);
        DOT16_ARQ_WINDOW_REAR = (Int16)MacDot16ARQIncIndex(bsnIndex);

        if (DOT16_ARQ_RX_WINDOW_START != DOT16_ARQ_RX_HIGHEST_BSN)
        {   tempIndex = DOT16_ARQ_WINDOW_REAR;
            tempBSNId = DOT16_ARQ_RX_WINDOW_START;
            windowIncrement = 0;
            MacDot16ARQSetARQBlockPointer(tempIndex);
            while (tempBSNId!= DOT16_ARQ_RX_HIGHEST_BSN)
            {
                if (arqBlockPtr->arqBlockState == DOT16_ARQ_BLOCK_RECEIVED)
                {
                    windowIncrement++;
                    if (arqBlockPtr->arqRxPurgeTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(node,
                            arqBlockPtr->arqRxPurgeTimer);
                        arqBlockPtr->arqRxPurgeTimer = NULL;
                    }
                }
                else
                {
                    break;
                }
                MacDot16ARQIncBsnId(tempBSNId);
                MacDot16ARQIncIndex(tempIndex);
                MacDot16ARQSetARQBlockPointer(tempIndex);
            }

            DOT16_ARQ_RX_WINDOW_START =
                    (DOT16_ARQ_RX_WINDOW_START + windowIncrement) %
                                                DOT16_ARQ_BSN_MODULUS;
            DOT16_ARQ_WINDOW_REAR = (DOT16_ARQ_WINDOW_REAR +
                windowIncrement) % (UInt16)DOT16_ARQ_WINDOW_SIZE;
            //We will send a collective Feedback
            MacDot16BuildAndSendARQFeedback(node,
                dot16,
                sFlow,
                ssInfo);
            MacDot16ARQBuildSDU(node,
                dot16,
                sFlow,
                ssInfo);
        }
    }
    
    if (DEBUG_ARQ)
    {
        MacDot16ARQPrintControlBlock(node, sFlow);
    }
}//End of function MacDot16ARQHandleTimeout

// /**
// FUNCTION       :: MacDot16ARQConvertParam
// DESCRIPTION :: ARQ convert parameter in to frame duration
// **/
void MacDot16ARQConvertParam(MacDot16ARQParam *arqParam,
                             clocktype frameDuration)
{
       clocktype timeVal = 0 ;
       int inRangeValue = 0 ;
       char clockStr[MAX_STRING_LENGTH];
       char buff[MAX_STRING_LENGTH];

       TIME_PrintClockInSecond(DOT16_ARQ_MAX_BLOCK_LIFE_TIME,
                                clockStr);

        ERROR_Assert(frameDuration < DOT16_ARQ_MAX_BLOCK_LIFE_TIME,
                     "802.16:MAC-802.16-BS-FRAME-DURATION lies out of"
                     " the permissible range of the ARQ timeout related"
                     " parameters.");

        arqParam->arqRetryTimeoutTxDelay =
            arqParam->arqRetryTimeoutTxDelay * frameDuration;

        arqParam->arqRetryTimeoutRxDelay =
            arqParam->arqRetryTimeoutRxDelay * frameDuration ;

        timeVal = arqParam->arqRetryTimeoutRxDelay;

         if (timeVal > DOT16_ARQ_MAX_RETRY_TIMEOUT_RECEIVER_DELAY )
            {
                inRangeValue = (int)
                 (DOT16_ARQ_MAX_RETRY_TIMEOUT_DELAY / frameDuration );

                TIME_PrintClockInSecond(
                                DOT16_ARQ_MAX_RETRY_TIMEOUT_RECEIVER_DELAY,
                                clockStr);
                sprintf(buff,
                        "MAC802.16:"
                        " MAC-802.16-ARQ-RETRY-TIMEOUT"
                        "must be shorter than %s seconds \n."
                        "Using the nearest value that falls in range."
                        " MAC-802.16-ARQ-RETRY-TIMEOUT = %d "
                        "frames",clockStr,inRangeValue);
                ERROR_ReportWarning(buff);

                arqParam->arqRetryTimeoutRxDelay =
                                (inRangeValue/2) * frameDuration ;

                arqParam->arqRetryTimeoutTxDelay =
                                (inRangeValue/2) * frameDuration ;
            }

        arqParam->arqBlockLifeTime =
                        arqParam->arqBlockLifeTime *
                        (arqParam->arqRetryTimeoutRxDelay +
                                        arqParam->arqRetryTimeoutTxDelay);
        timeVal = arqParam->arqBlockLifeTime ;

        if (timeVal > DOT16_ARQ_MAX_BLOCK_LIFE_TIME )
            {
                inRangeValue = (int)
                    (DOT16_ARQ_MAX_BLOCK_LIFE_TIME / frameDuration) ;

                TIME_PrintClockInSecond(DOT16_ARQ_MAX_BLOCK_LIFE_TIME,
                                        clockStr);
                sprintf(buff,
                        "MAC802.16: MAC-802.16-ARQ-BLOCK-LIFE-TIME "
                        "must be shorter than %s seconds \n."
                        "Using the nearest value that falls in range."
                        " MAC-802.16-ARQ-BLOCK-LIFE-TIME = %d frames",
                        clockStr,inRangeValue);
                ERROR_ReportWarning(buff);
                arqParam->arqBlockLifeTime = inRangeValue * frameDuration ;
            }

         arqParam->arqSyncLossTimeout =
            arqParam->arqSyncLossTimeout * frameDuration ;
         timeVal = arqParam->arqSyncLossTimeout;

        if (timeVal > DOT16_ARQ_MAX_SYNC_LOSS_TIMEOUT )
            {
               inRangeValue = (int)
                    (DOT16_ARQ_MAX_SYNC_LOSS_TIMEOUT / frameDuration);

                TIME_PrintClockInSecond(DOT16_ARQ_MAX_SYNC_LOSS_TIMEOUT,
                                        clockStr);
                sprintf(buff,
                        "MAC802.16: MAC-802.16-ARQ-SYNC-LOSS-INTERVAL "
                        "must be shorter than %s seconds \n."
                        "Using the nearest value that falls in range."
                        " MAC-802.16-ARQ-SYNC-LOSS-INTERVAL = %d frames",
                        clockStr,inRangeValue);
                ERROR_ReportWarning(buff);
                arqParam->arqSyncLossTimeout = inRangeValue * frameDuration ;
            }

        arqParam->arqRxPurgeTimeout =
            arqParam->arqRxPurgeTimeout * frameDuration ;

        timeVal = arqParam->arqRxPurgeTimeout ;

        if (timeVal > DOT16_ARQ_MAX_RX_PURGE_TIMEOUT )
            {
                inRangeValue = (int)
                    (DOT16_ARQ_MAX_RX_PURGE_TIMEOUT / frameDuration);

                TIME_PrintClockInSecond(DOT16_ARQ_MAX_RX_PURGE_TIMEOUT,
                                        clockStr);
                sprintf(buff,
                        "MAC802.16: MAC-802.16-ARQ-RX-PURGE-TIMEOUT "
                        "must be shorter than %s seconds \n."
                        "Using the nearest value that falls in range."
                        " MAC-802.16-ARQ-RX-PURGE-TIMEOUT = %d frames",
                        clockStr,inRangeValue);
                ERROR_ReportWarning(buff);
                arqParam->arqRxPurgeTimeout = inRangeValue * frameDuration ;
            }

            if (DEBUG_ARQ_INIT)
            {
                printf("ARQ parameters after converted.from frame to time\n");
            }
}//end of Function MacDot16ARQConvertParam

// /**
// FUNCTION       :: MacDot16ARQResetWindow
// DESCRIPTION :: ARQ reset window
// **/
void MacDot16ARQResetWindow(
    Node* node,
    MacDataDot16* dot16,
    MacDot16ServiceFlow* sFlow)
{
    UInt16 front = 0;
    UInt16 index = 0;
    MacDot16ARQBlock* arqBlockPtr = NULL;
    // We need to empty the window.
    front = DOT16_ARQ_WINDOW_FRONT ;
    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Deleting ARQ parameters!\n");
        MacDot16ARQPrintControlBlock(node, sFlow);
    }

    if (sFlow->timerT22 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, sFlow->timerT22);
        sFlow->timerT22 = NULL;
    }

    for (index = 0; index < DOT16_ARQ_ARRAY_SIZE; index++)
    {
        MacDot16ARQSetARQBlockPointer(index);
        MacDot16ResetARQBlockPtr(node, arqBlockPtr, TRUE);
    }
    sFlow->arqControlBlock->arqRxHighestBsn = 0;
    sFlow->arqControlBlock->arqRxWindowStart = 0;
    sFlow->arqControlBlock->arqTxNextBsn = 0;
    sFlow->arqControlBlock->arqTxWindowStart = 0;
    sFlow->arqControlBlock->front = 0;
    sFlow->arqControlBlock->rear = 0;
    sFlow->arqControlBlock->numOfARQResetRetry = 0;
    sFlow->arqControlBlock->isARQBlockTransmisionEnable = TRUE;
    sFlow->arqControlBlock->waitForARQResetType = 0;
    if (sFlow->arqSyncLossTimer != NULL)
    {
        MESSAGE_CancelSelfMsg(node, sFlow->arqSyncLossTimer);
        sFlow->arqSyncLossTimer = NULL;
    }
}//end of function

// /**
// FUNCTION       :: MacDot16ARQCalculateBwReq
// DESCRIPTION :: ARQ calculate bandwidth request
// **/
int MacDot16ARQCalculateBwReq(MacDot16ServiceFlow* sFlow, UInt16 crcSize)
{
    int tempARQIndex;
    UInt32 sFlowBwReq = 0;
    MacDot16ARQBlock* arqBlockPtr = NULL;
    int numARQPdu = 0;
    int subHeaderSize = 0;


    if (sFlow->isPackingEnabled == TRUE)
    {
        subHeaderSize = sizeof(MacDot16ExtendedorARQEnablePackSubHeader);
    }
    else
    {
        subHeaderSize = sizeof(MacDot16ExtendedorARQEnableFragSubHeader);
    }

    if (sFlow->arqControlBlock == NULL || (sFlow->activated == FALSE)
        || (sFlow->admitted == FALSE))
    {
        return sFlowBwReq;
    }
    if (sFlow->arqControlBlock->isARQBlockTransmisionEnable == FALSE)
    {
        return sFlowBwReq;
    }

    for (tempARQIndex = DOT16_ARQ_WINDOW_FRONT;
                                    tempARQIndex!= DOT16_ARQ_WINDOW_REAR;
                                        MacDot16ARQIncIndex(tempARQIndex))
    {
        MacDot16ARQSetARQBlockPointer(tempARQIndex);
        if ((arqBlockPtr->arqBlockState == DOT16_ARQ_BLOCK_NOT_SENT)||
            (arqBlockPtr->arqBlockState ==
                            DOT16_ARQ_BLOCK_WAIT_FOR_RETRANSMISSION))
        {
            sFlowBwReq += MESSAGE_ReturnPacketSize(arqBlockPtr->blockPtr);
        }
        if ((MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER) ==
            DOT16_NO_FRAGMENTATION) ||
            (MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER) ==
            DOT16_LAST_FRAGMENT)
            || (MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER) ==
            DOT16_FIRST_FRAGMENT))
        {
            numARQPdu++;
        }
    }
    if (sFlowBwReq > 0)
    {
        if (sFlow->isPackingEnabled && (numARQPdu > 1))
        {
            sFlowBwReq += crcSize + sizeof(MacDot16MacHeader)
                + (numARQPdu * subHeaderSize);
        }
        else
        {
            sFlowBwReq += ((subHeaderSize + crcSize +
                sizeof(MacDot16MacHeader)) * numARQPdu);
        }
    }
    return sFlowBwReq;
}

// /**
// FUNCTION       :: MacDot16ResetARQBlockPtr
// DESCRIPTION :: ARQ reset ARQ block pointer
// **/
void MacDot16ResetARQBlockPtr(Node* node,
                              MacDot16ARQBlock* arqBlockPtr,
                              BOOL flag)
{
    arqBlockPtr->arqBlockState = DOT16_ARQ_INVALID_STATE;
    arqBlockPtr->arqFragSubHeader.byte1 = 0;
    arqBlockPtr->arqFragSubHeader.byte2 = 0;
    if (flag == TRUE)
    {
        if (arqBlockPtr->arqRxPurgeTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(node, arqBlockPtr->arqRxPurgeTimer);
            // Bug fix # 5395 start
            arqBlockPtr->arqRxPurgeTimer = NULL;
            // Bug fix # 5395 end
        }
        if (arqBlockPtr->blockPtr != NULL)
        {
            MESSAGE_Free(node, arqBlockPtr->blockPtr);
            // Bug fix # 5395 start
            arqBlockPtr->blockPtr = NULL;
            // Bug fix # 5395 end
        }
    }
    arqBlockPtr->arqRxPurgeTimer = NULL;
    arqBlockPtr->blockPtr = NULL;
}// end of

// /**
// FUNCTION       :: MacDot16SkipPdu
// DESCRIPTION :: Skip PDu from the message list
// **/
Message* MacDot16SkipPdu(Message* pdu)
{
    // get the MAC header
    unsigned char* payload = (unsigned char*) MESSAGE_ReturnPacket(pdu);
    MacDot16MacHeader* macHeader = (MacDot16MacHeader*) payload;
    unsigned int pduLength = MacDot16MacHeaderGetLEN(macHeader);

    if (pduLength == 0)
    {
        pdu = pdu->next;
    }

    while ((pduLength > 0) && (pdu != NULL))
    {
        pduLength -= MESSAGE_ReturnPacketSize(pdu);
        pdu = pdu->next;
    }
    return pdu;
}// end of MacDot16SkipPdu

// /**
// FUNCTION       :: MacDot16eGetPSClassTypebyServiceType
// DESCRIPTION :: Return PS class type as per the service type
// **/
MacDot16ePSClassType MacDot16eGetPSClassTypebyServiceType(int serviceType)
{
    MacDot16ePSClassType retVal = POWER_SAVING_CLASS_1;
        switch(serviceType)
        {
        case DOT16_SERVICE_UGS:
            retVal=
                DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_UGS;
            break;
        case DOT16_SERVICE_ertPS:
            retVal =
                DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_ERTPS;
            break;
        case DOT16_SERVICE_rtPS:
            retVal =
                DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_RTPS;
            break;
        case DOT16_SERVICE_nrtPS:
            retVal =
                DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_NRTPS;
            break;
        case DOT16_SERVICE_BE:
            retVal =
                DOT16e_SS_DEFAULT_PS_CLASS_MAPPING_SERVICE_TYPE_BE;
            break;
        default:
            break;
        }// end of switch
        return retVal;
}// end of MacDot16eGetPSClassType
