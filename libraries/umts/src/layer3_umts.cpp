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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"
#include "ipv6.h"
#include "transport_udp.h"
#include "transport_tcp_hdr.h"
#include "app_cbr.h"
#include "app_vbr.h"
#include "app_util.h"
#include "stats_net.h"

#include "cellular.h"
#include "cellular_layer3.h"
#include "cellular_umts.h"
#include "layer3_umts.h"
#include "layer3_umts_ue.h"
#include "layer3_umts_nodeb.h"
#include "layer3_umts_rnc.h"
#include "layer3_umts_gsn.h"
#include "layer3_umts_hlr.h"
#include "layer2_umts_rlc.h"
#include "layer2_umts_mac.h"
#include "layer2_umts_mac_phy.h"

#define DEBUG_RR 0
#define DEBUG_RRC_RB 0
#define DEBUG_RRC_TRCH 0
#define DEBUG_RRC_RL  0

#define DEBUG_PARAMETER    0

// /**
// FUNCTION   :: UmtsLayer3PrintRunTimeInfo
// LAYER      :: UMTS LAYER2
// PURPOSE    :: Print run time info UMTS layer2 at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + sublayerType     : UmtsLayer2Subla9yerType : Pointer to node input.
// RETURN     :: void : NULL
// **/
void UmtsLayer3PrintRunTimeInfo(Node *node,
                                UmtsLayer3SublayerType sublayerType)
{
    char clockStr[MAX_STRING_LENGTH];
    char sublayerStr[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

    if (sublayerType == UMTS_LAYER3_SUBLAYER_RRC)
    {
        sprintf(sublayerStr,"%s","RRC");
    }
    else if (sublayerType == UMTS_LAYER3_SUBLAYER_MM)
    {
        sprintf(sublayerStr,"%s","MM");
    }
    else if (sublayerType == UMTS_LAYER3_SUBLAYER_GMM)
    {
        sprintf(sublayerStr,"%s","GMM");
    }
    else if (sublayerType == UMTS_LAYER3_SUBLAYER_CC)
    {
        sprintf(sublayerStr,"%s","CC");
    }
    else if (sublayerType == UMTS_LAYER3_SUBLAYER_SM)
    {
        sprintf(sublayerStr,"%s","SM");
    }
    else
    {
        sprintf(sublayerStr,"%s","GENERAL");
    }

    if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        printf("%s Node %d (UE %s): ",
            clockStr, node->nodeId, sublayerStr);
    }
    else if (UmtsGetNodeType(node) == CELLULAR_NODEB)
    {
        printf("%s Node %d (Node_B %s): ",
            clockStr, node->nodeId, sublayerStr);
    }
    else if (UmtsGetNodeType(node) == CELLULAR_RNC)
    {
        printf("%s Node %d (RNC %s): ",
            clockStr, node->nodeId, sublayerStr);
    }
    else if (UmtsGetNodeType(node) == CELLULAR_SGSN)
    {
        printf("%s Node %d (SGSN %s): ",
            clockStr, node->nodeId, sublayerStr);
    }
    else if (UmtsGetNodeType(node) == CELLULAR_GGSN)
    {
        printf("%s Node %d (GGSN %s): ",
            clockStr, node->nodeId, sublayerStr);
    }
    else if (UmtsGetNodeType(node) == CELLULAR_HLR)
    {
        printf("%s Node %d (HLR %s): ",
            clockStr, node->nodeId, sublayerStr);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GetData
// LAYER      :: Layer 3
// PURPOSE    :: Get the pointer which points to the UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node *           : Pointer to node.
// RETURN     :: UmtsLayer3Data * : Pointer to UMTS layer3 data or NULL
// **/
UmtsLayer3Data* UmtsLayer3GetData(Node *node)
{
    CellularLayer3Data *cellularData;

    cellularData = (CellularLayer3Data *)
                   node->networkData.cellularLayer3Var;

    ERROR_Assert(cellularData != NULL, "UMTS: Memory error!");
    ERROR_Assert(cellularData->umtsL3 != NULL, "UMTS: Memory error!");

    return cellularData->umtsL3;
}

// /**
// FUNCTION   :: UmtsLayer3GetL2InterfaceIndex(Node *node)
// LAYER      :: LAYER 3
// PURPOSE    :: Get the cellular layer2 interface index
// PARAMETERS ::
// + node      : Node *           : Pointer to node
// RETURN     :: BOOL             : success or not
// **/
static
BOOL UmtsLayer3GetL2InterfaceIndex(Node *node)
{
    CellularLayer3Data *cellularData;

    cellularData = (CellularLayer3Data *)
                   node->networkData.cellularLayer3Var;

    int interfaceCount;
    for (interfaceCount = 0;
         interfaceCount < node->numberInterfaces;
         interfaceCount ++)
    {
        if (node->macData[interfaceCount]->macProtocol ==
            MAC_PROTOCOL_CELLULAR)
        {
            cellularData->interfaceIndex =
                node->macData[interfaceCount]->interfaceIndex;

            cellularData->umtsL3->interfaceIndex =
                node->macData[interfaceCount]->interfaceIndex;

            if (DEBUG_PARAMETER)
            {
                printf("node %d  (L3): UMTS Init find "
                    "CELLULAR MAC interface %d\n",
                    node->nodeId,
                    cellularData->interfaceIndex);
            }

            return TRUE;
        }
    }
    if (interfaceCount == node->numberInterfaces)
    {
        char errorString[MAX_STRING_LENGTH];

        sprintf(errorString,
            "%d UMTS L3: CELLULAR MAC interface not found\n",
             node->nodeId);
        ERROR_ReportError(errorString);
    }

    return FALSE;
}

// /**
// FUNCTION   :: UmtsLayer3PrintParameter
// LAYER      :: Layer 3
// PURPOSE    :: Print out UMTS layer3 parameters
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3PrintParameter(Node *node, UmtsLayer3Data *umtsL3)
{
    printf("UMTS parameters of Node%d\n", node->nodeId);

    printf("    nodeType = ");
    if (umtsL3->nodeType == CELLULAR_UE)
    {
        printf("UE\n");
    }
    else if (umtsL3->nodeType == CELLULAR_NODEB)
    {
        printf("NodeB\n");
    }
    else if (umtsL3->nodeType == CELLULAR_RNC)
    {
        printf("RNC\n");
    }
    else if (umtsL3->nodeType == CELLULAR_SGSN)
    {
        printf("SGSN\n");
    }
    else if (umtsL3->nodeType == CELLULAR_GGSN)
    {
        printf("GGSN\n");
    }
    else if (umtsL3->nodeType == CELLULAR_HLR)
    {
        printf("HLR\n");
    }
    else
    {
        printf("Unknown\n");
    }

    return;
}

// /**
// FUNCTION   :: UmtsLayer3InitParameter
// LAYER      :: Layer 3
// PURPOSE    :: Read necessary parameters during initalization.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3InitParameter(Node *node, const NodeInput *nodeInput)
{
    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);
    BOOL wasFound;
    char strVal[MAX_STRING_LENGTH];
    char errStr[MAX_STRING_LENGTH];

    // read the node type
    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "UMTS-NODE-TYPE",
                  &wasFound,
                  strVal);

    if (wasFound)
    {
        if (strcmp(strVal, "UE") == 0)
        {
            // User Equipment
            umtsL3->nodeType = CELLULAR_UE;
        }
        else if (strcmp(strVal, "NodeB") == 0)
        {
            // Node B
            umtsL3->nodeType = CELLULAR_NODEB;
        }
        else if (strcmp(strVal, "RNC") == 0)
        {
            // Radio Network Controller
            umtsL3->nodeType = CELLULAR_RNC;
        }
        else if (strcmp(strVal, "SGSN") == 0)
        {
            // Serving GPRS Support Node
            umtsL3->nodeType = CELLULAR_SGSN;
        }
        else if (strcmp(strVal, "GGSN") == 0)
        {
            // Gateway GPRS Support Node
            umtsL3->nodeType = CELLULAR_GGSN;
        }
        else if (strcmp(strVal, "HLR") == 0)
        {
            umtsL3->nodeType = CELLULAR_HLR;
        }
        else
        {
            sprintf(errStr,
                    "UMTS: Wrong type of node %d as %s! "
                    "Use default node type as UE.",
                    node->nodeId, strVal);
            ERROR_ReportWarning(errStr);
            umtsL3->nodeType = CELLULAR_UE;
        }
    }
    else
    {
        // not configured. Assume default node type is UE
        umtsL3->nodeType = CELLULAR_UE;
    }

    if (umtsL3->nodeType == CELLULAR_SGSN ||
        umtsL3->nodeType == CELLULAR_GGSN)
    {
        // VLR is co-located at GSNs currently.
        umtsL3->isVLR = TRUE;
    }

    // read the node type
    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "UMTS-PRINT-DETAILED-STATISTICS",
                  &wasFound,
                  strVal);

    if (wasFound  && strcmp(strVal, "YES") == 0)
    {
        // need to print detailed statistics
        umtsL3->printDetailedStat = TRUE;
    }
    else if (!wasFound  || strcmp(strVal, "NO") == 0)
    {
        // default value is FALSE
        umtsL3->printDetailedStat = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Value of UMTS-PRINT-DETAILED-STATISTICS "
            "should be YES or NO! Default value NO is used.");
        umtsL3->printDetailedStat = FALSE;
    }

    return;
}

// /**
// FUNCTION   :: UmtsLayer3PrintStats
// LAYER      :: Layer 3
// PURPOSE    :: Print out UMTS layer3 statistics
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3PrintStats(Node *node, UmtsLayer3Data *umtsL3)
{
    char buf[MAX_STRING_LENGTH];

    // print out the node type
    if (umtsL3->nodeType == CELLULAR_UE)
    {
        sprintf(buf, "Node type = UE");
    }
    else if (umtsL3->nodeType == CELLULAR_NODEB)
    {
        sprintf(buf, "Node type = NodeB");
    }
    else if (umtsL3->nodeType == CELLULAR_RNC)
    {
        sprintf(buf, "Node type = RNC");
    }
    else if (umtsL3->nodeType == CELLULAR_SGSN)
    {
        sprintf(buf, "Node type = SGSN");
    }
    else if (umtsL3->nodeType == CELLULAR_GGSN)
    {
        sprintf(buf, "Node type = GGSN");
    }
    else if (umtsL3->nodeType == CELLULAR_HLR)
    {
        sprintf(buf, "Node type = HLR");
    }
    else
    {
        ERROR_ReportError("UMTS: Wrong node type!");
    }

    IO_PrintStat(node, "Layer3", "UMTS", ANY_DEST, -1, buf);

    return;
}


//--------------------------------------------------------------------------
//  Utility API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: UmtsLayer3ConfigRadioLink
// LAYER      :: Layer 3
// PURPOSE    :: Configurate a specific Radio Link
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// + rlInfo    : UmtsCphyRadioLinkSetupReqInfo * : pointer to the RL info
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConfigRadioLink(
    Node *node,
    UmtsLayer3Data *umtsL3,
    UmtsCphyRadioLinkSetupReqInfo *rlInfo)
{
    UmtsMacPhyHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)rlInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): configure a dedicated phCh type %d id %d\n",
            node->nodeId, rlInfo->phChType, rlInfo->phChId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3ReleasedRadioLink
// LAYER      :: Layer 3
// PURPOSE    :: Configurate a specific Radio Link
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// + rlInfo    : UmtsCphyRadioLinkSetupReqInfo * : pointer to the RL info
// RETURN     :: void : NULL
// **/
void UmtsLayer3ReleaseRadioLink(
    Node *node,
    UmtsLayer3Data *umtsL3,
    UmtsCphyRadioLinkRelReqInfo *rlInfo)
{
    UmtsMacPhyHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_RELEASE_REQ,
        (void*)rlInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): release a dedicated phCh type %d id %d\n",
            node->nodeId, rlInfo->phChType, rlInfo->phChId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3ReleaseTrCh
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the common Transport Channels
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// + trRelInfo : UmtsCphyTrChReleaseReqInfo * : TrCh release request info
// RETURN     :: void : NULL
// **/
void UmtsLayer3ReleaseTrCh(
    Node *node,
    UmtsLayer3Data *umtsL3,
    UmtsCphyTrChReleaseReqInfo *trRelInfo)
{
    UmtsMacPhyHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_TrCH_RELEASE_REQ,
        (void*)trRelInfo);
    if (DEBUG_RRC_TRCH)
    {
        printf("node %d (L3) Release a TrCh type %d id %d\n",
            node->nodeId, trRelInfo->trChType, trRelInfo->trChId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3ConfigTrCh
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the common Transport Channels
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// + trRelInfo : UmtsCphyTrChReleaseReqInfo * : TrCh release request info
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConfigTrCh(
    Node *node,
    UmtsLayer3Data *umtsL3,
    UmtsCphyTrChConfigReqInfo *trCfgInfo)
{
    UmtsMacPhyHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_TrCH_CONFIG_REQ,
        (void*)trCfgInfo);
    if (DEBUG_RRC_TRCH)
    {
        printf("node %d (L3) configure a TrCh type %d id %d\n",
            node->nodeId, trCfgInfo->trChType, trCfgInfo->trChId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3ReleaseRadioBearer
// LAYER      :: Layer 3
// PURPOSE    :: Release the common radio bearer
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// + rbInfo    : UmtsCmacConfigReqRbInfo * : released rb info
// RETURN     :: void : NULL
// **/
void UmtsLayer3ReleaseRadioBearer(
         Node* node,
         UmtsLayer3Data *umtsL3,
         UmtsCmacConfigReqRbInfo *rbInfo)
{
    // config radio bearer
    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CMAC_CONFIG_REQ_RB,
        (void*)rbInfo);

    // no need to take care the logTrPhMap
    // when rb is deleted, map will be removed
}

// /**
// FUNCTION   :: UmtsLayer3ConfigRadioBearer
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the common radio bearer
// PARAMETERS ::
// + node      : Node*                      : Pointer to node.
// + umtsL3    : UmtsLayer3Data *           : Point to the umts layer3 data
// + rbInfo    : UmtsCmacConfigReqRbInfo    : Pointer to the RB info
// + phType    : UmtsPhysicalChannelType*   : physical channel type
// + phId      : int*                       : phId
// + numPhCh   : int                        : number of physical channels
// + transFormat : UmtsTransportFormat*     : transport format used in the RB
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConfigRadioBearer(
         Node* node,
         UmtsLayer3Data *umtsL3,
         const UmtsCmacConfigReqRbInfo *rbInfo,
         UmtsPhysicalChannelType* phType,
         int* phId,
         int numPhCh,
         UmtsTransportFormat* transFormat)
{
    UmtsTrCh2PhChMappingInfo mapInfo;
    memset(&mapInfo, 0, sizeof(UmtsTrCh2PhChMappingInfo));

    // config radio bearer
    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CMAC_CONFIG_REQ_RB,
        (void*)rbInfo);

    // config the mapping if uplink
    if ((rbInfo->chDir == UMTS_CH_UPLINK && UmtsIsUe(node))
         || (rbInfo->chDir == UMTS_CH_DOWNLINK && UmtsIsNodeB(node)))
    {
        mapInfo.rbId = rbInfo->rbId;
        mapInfo.chDir = rbInfo->chDir;
        mapInfo.priSc = rbInfo->priSc;
        mapInfo.ueId = rbInfo->ueId;
        mapInfo.trChType = rbInfo->trChType;
        mapInfo.trChId = rbInfo->trChId;
        mapInfo.phChType = phType[0];
        mapInfo.phChId = phId[0];

        // copy the transport format
        if (transFormat)
        {
            memcpy(&mapInfo.transFormat,
                   transFormat,
                   sizeof(UmtsTransportFormat));

        }
        UmtsMacPhyMappingTrChPhCh(node, umtsL3->interfaceIndex, &mapInfo);
        if (DEBUG_RRC_RB)
        {
            printf("node %d (L3): mapping a TrCh type %d to PhType %d \n",
                node->nodeId, rbInfo->trChType, phType[0]);
        }
    }

    if (DEBUG_RRC_RB)
    {
        printf("node %d (L3): configure a rb\n", node->nodeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3ReleaseRlcEntity
// LAYER      :: Layer 3
// PURPOSE    :: Rlease a RLC entity
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// + rbId      : unsigned char     : RB ID
// + direction : UmtsRlcEntityDirect : Entity direction
// + ueId      : NodeId            : UE ID, used at NodeB
// RETURN     :: void : NULL
// **/
void UmtsLayer3ReleaseRlcEntity(
    Node *node,
    UmtsLayer3Data *umtsL3,
    unsigned char   rbId,
    UmtsRlcEntityDirect direction,
    NodeId ueId)
{
    UmtsRlcRrcReleaseArgs rlsArgs;
    rlsArgs.ueId = ueId;
    rlsArgs.rbId = rbId;
    rlsArgs.direction = direction;

    UmtsRlcHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CRLC_CONFIG_RELEASE,
        &rlsArgs);
}

// /**
// FUNCTION   :: UmtsLayer3ConfigRlcEntity
// LAYER      :: Layer 3
// PURPOSE    :: Create a RLC entity
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// + rbId      : unsigned char     : RB ID
// + direction : UmtsRlcEntityDirect : Entity direction
// + entityType: UmtsRlcEntityType : entity type
// + entityConfig: entityConfig    : entity specific configuration parameters
// + ueId      : NodeId            : UE ID, used at NodeB
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConfigRlcEntity(
    Node *node,
    UmtsLayer3Data *umtsL3,
    unsigned char   rbId,
    UmtsRlcEntityDirect direction,
    UmtsRlcEntityType entityType,
    void* entityConfig,
    NodeId ueId)
{
    UmtsRlcRrcEstablishArgs estArgs;
    estArgs.ueId = ueId;
    estArgs.rbId = rbId;
    estArgs.direction = direction;
    estArgs.entityType = entityType;
    estArgs.entityConfig = entityConfig;

    UmtsRlcHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CRLC_CONFIG_ESTABLISH,
        &estArgs);
}

// /**
// FUNCTION   :: UmtsLayer3SetTimer
// LAYER      :: MAC
// PURPOSE    :: Set a timer message. If a non-NULL message pointer is
//               passed in, this function will just send out that msg.
//               If NULL message is passed in, it will create a new
//               message and send it out. In either case, a pointer to
//               the message is returned, in case the caller wants to
//               cancel the message in the future.
// PARAMETERS ::
// + node      : Node*               : Pointer to node
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + timerType : UmtsLayer3TimerType : Type of the timer
// + delay     : clocktype           : Delay of this timer
// + msg       : Message*            : If non-NULL, use this message
// + infoPtr   : void*               : Additional info fields if needed
// + infoSize  : int                 : Size of the additional info fields
// RETURN     :: Message*            : Pointer to the timer message
// **/
Message* UmtsLayer3SetTimer(Node *node,
                            UmtsLayer3Data *umtsL3,
                            UmtsLayer3TimerType timerType,
                            clocktype delay,
                            Message* msg,
                            void* infoPtr,
                            int infoSize)
{
    Message* timerMsg = NULL;
    UmtsLayer3Timer* timerInfo;

    if (msg != NULL)
    {
        timerMsg = msg;
    }
    else
    {
        // allocate the timer message and send out
        timerMsg = MESSAGE_Alloc(node,
                                 NETWORK_LAYER,
                                 NETWORK_PROTOCOL_CELLULAR,
                                 MSG_NETWORK_CELLULAR_TimerExpired);

        MESSAGE_SetInstanceId(timerMsg, 0);

        MESSAGE_InfoAlloc(node,
                          timerMsg,
                          sizeof(UmtsLayer3Timer) + (unsigned int)infoSize);
        timerInfo = (UmtsLayer3Timer*) MESSAGE_ReturnInfo(timerMsg);

        timerInfo->timerType = timerType;
        if (infoSize > 0 && infoPtr != NULL)
        {
            memcpy(((char *) timerInfo) + sizeof(UmtsLayer3Timer),
                   infoPtr,
                   infoSize);
        }
    }

    MESSAGE_Send(node, timerMsg, delay);

    return timerMsg;
}

// /**
// FUNCTION   :: UmtsAddBackboneHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to add the Backbone message header
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + msgType   : UmtsBackboneMessageType: The backbone message type
// + info      : const char*: The additional header info
// + infoSize  : int        : The additional info size
// RETURN     :: void       : NULL
// **/
void UmtsAddBackboneHeader(Node *node,
                           Message *msg,
                           UmtsBackboneMessageType msgType,
                           const char* info,
                           int infoSize)
{
    ERROR_Assert((unsigned int)infoSize <= UMTS_BACKBONE_HDR_MAX_INFO_SIZE,
        "UmtsAddBackboneHeader: infoSize is too large.");
    UmtsBackboneHeader backboneHeader;
    backboneHeader.msgType = (char)msgType;
    backboneHeader.infoSize = (char)infoSize;

    int hdrSize = ((sizeof(backboneHeader)+infoSize)/4 + 1) * 4;
    MESSAGE_AddHeader(node,
                      msg,
                      hdrSize,
                      TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           &backboneHeader,
           sizeof(backboneHeader));
    if (infoSize > 0)
    {
        memcpy(MESSAGE_ReturnPacket(msg) + sizeof(UmtsBackboneHeader),
               info,
               infoSize);
    }
}

// /**
// FUNCTION   :: UmtsRemoveBackboneHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to remove the backbone header
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + msgType   : UmtsBackboneMessageType: For returning of the backbone message type
// + info      : char*      : For returnning of the additional header info
// + infoSize  : int&       : The additional info size
// RETURN     :: void       : NULL
// **/
void UmtsRemoveBackboneHeader(Node *node,
                              Message *msg,
                              UmtsBackboneMessageType* msgType,
                              char* info,
                              int&  infoSize)
{
    ERROR_Assert((unsigned int)infoSize <= UMTS_BACKBONE_HDR_MAX_INFO_SIZE,
        "UmtsRemoveBackboneHeader: infoSize is too large.");
    UmtsBackboneHeader backboneHeader;

    memcpy((char *) &backboneHeader,
           (char *) MESSAGE_ReturnPacket(msg),
           sizeof(UmtsBackboneHeader));

    *msgType = (UmtsBackboneMessageType)(backboneHeader.msgType);
    infoSize = infoSize < backboneHeader.infoSize ?
                    infoSize : backboneHeader.infoSize;

    if (info && infoSize > 0)
    {
        memcpy(info,
               MESSAGE_ReturnPacket(msg) + sizeof(UmtsBackboneHeader),
               infoSize);
    }

    int hdrSize = ((sizeof(backboneHeader) + 
                   backboneHeader.infoSize)/4 + 1) * 4;
    MESSAGE_RemoveHeader(node,
                         msg,
                         hdrSize,
                         TRACE_UMTS_LAYER3);
}

// /**
// FUNCTION   :: UmtsAddGtpHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to add theGTP message header
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + firstByte : unsigned char : the first byte of the GTP header
// + msgType   : UmtsGtpMsgType : The GTP message type
// + teId      : UInt32  : TeId of the GTP
// + info      : const char*: The additional header info
// + infoSize  : int        : The additional info size
// RETURN     :: void       : NULL
// **/
void UmtsAddGtpHeader(Node *node,
                      Message *msg,
                      unsigned char firstByte,
                      UmtsGtpMsgType msgType,
                      UInt32 teId,
                      const char* info,
                      int infoSize)
{
    UmtsGtpHeader gtpHeader;
    gtpHeader.length = (UInt16)infoSize;
    gtpHeader.msgType = (unsigned char)msgType;
    gtpHeader.teId = teId;
    // write the first byte
    memset((unsigned char*)&gtpHeader, firstByte, 1);

    MESSAGE_AddHeader(node,
                      msg,
                      sizeof(UmtsGtpHeader) + infoSize,
                      TRACE_UMTS_LAYER3);

    char* header = (char*)MESSAGE_ReturnPacket(msg);
    memcpy(header, (char*)&gtpHeader, sizeof(UmtsGtpHeader));

    if (infoSize > 0)
    {
        memcpy(header + sizeof(UmtsGtpHeader), info, infoSize);
    }
}

// /**
// FUNCTION   :: UmtsRemoveGtpHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to remove the GTP header
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + gtpHeader : UmtsGtpHeader : Mandatory header
// + info      : char*      : For returnning of the additional header info
// + infoSize  : int*       : The additional info size
// RETURN     :: void       : NULL
// **/
void UmtsRemoveGtpHeader(Node *node,
                         Message *msg,
                         UmtsGtpHeader* gtpHeader,
                         char* info,
                         int* infoSize)
{
    char* header = MESSAGE_ReturnPacket(msg);

    memcpy((char *) gtpHeader,
           (char *) header,
           sizeof(UmtsGtpHeader));

    *infoSize = gtpHeader->length;
    if (*infoSize > 0)
    {
        memcpy(info,
               header + sizeof(UmtsGtpHeader),
               *infoSize);
    }

    if (gtpHeader->msgType == UMTS_GTP_G_PDU &&
        UmtsGetNodeType(node) == CELLULAR_SGSN)
    {
        // for SGSN, tunnling all the data to/from
    }
    else
    {
        MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(UmtsGtpHeader) + *infoSize,
                             TRACE_UMTS_LAYER3);
    }
}

// /**
// FUNCTION   :: UmtsAddGtpBackboneHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to add the backbone message header
//               at the Iu
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + uplink    : BOOL       : Whether this message is a uplink message
// + ueId      : NodeId     : Value of the ueId
// RETURN     :: void       : NULL
// **/
void UmtsAddGtpBackboneHeader(Node *node,
                              Message *msg,
                              BOOL upLink,
                              NodeId ueId)
{
    char headerInfo[5];
    memcpy(headerInfo, &ueId, sizeof(ueId));
    headerInfo[sizeof(ueId)] = (char)upLink;
    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_GTP,
                          headerInfo,
                          5);
}

// /**
// FUNCTION   :: UmtsLayer3SgsnSendRanapMsgToRnc
// LAYER      :: Layer 3
// PURPOSE    :: Send a RANAP message to one RNC
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + msg       : Message*     : Message containing the NBAP message
// + destAddt  : Address      : destination address
// + upLink    : BOOL         : Is it uplink 
// + ueId      : NodeId       : UE Id of the GTP msg
// **/
void UmtsLayer3GsnSendGtpMsgOverBackbone(
                                   Node *node,
                                   Message *msg,
                                   Address destAddr,
                                   BOOL upLink,
                                   NodeId ueId)
{
    UmtsAddGtpBackboneHeader(node,
                             msg,
                             upLink,
                             ueId);

    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        destAddr,
        ANY_INTERFACE,
        IPTOS_PREC_INTERNETCONTROL,
        IPDEFTTL);
}

// /**
// FUNCTION   :: UmtsLayer3GsnSendCsSigMsgOverBackbone
// LAYER      :: Layer 3
// PURPOSE    :: Send a CS signalling message over backbone
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + msg       : Message*     : Message containing the NBAP message
// + destAddt  : Address      : destination address
// + ti        : char         : transction Id  
// + ueId      : NodeId       : UE Id of the GTP msg
// + msgType   : UmtsCsSigMsgType : CS signalling msg type
// **/
void UmtsLayer3GsnSendCsSigMsgOverBackbone(
        Node *node,
        Message *msg,
        Address destAddr,
        char ti,
        NodeId ueId,
        UmtsCsSigMsgType msgType)
{
    char info[10];
    int index = 0;
    memcpy(info, &ueId, sizeof(ueId));
    index += sizeof(ueId);
    info[index++] = ti;
    info[index++] = (char)msgType;

    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_CSSIG,
                          info,
                          index);

    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        destAddr,
        ANY_INTERFACE,
        IPTOS_PREC_INTERNETCONTROL,
        IPDEFTTL);
}

// /**
// FUNCTION   :: UmtsLayer3GtpSendCsPktOverBackbone
// LAYER      :: Layer 3
// PURPOSE    :: Send a CS data packet over backbone
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + msg       : Message*     : Message containing the NBAP message
// + destAddt  : Address      : destination address
// + ti        : char         : transction Id  
// + ueId      : NodeId       : UE Id of the GTP msg
// **/
void UmtsLayer3GsnSendCsPktOverBackbone(
        Node *node,
        Message *msg,
        Address destAddr,
        char ti,
        NodeId ueId)
{
    char info[10];
    int index = 0;
    memcpy(info, &ueId, sizeof(ueId));
    index += sizeof(ueId);
    info[index++] = ti;

    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_CSDATA,
                          info,
                          index);

    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        destAddr,
        ANY_INTERFACE,
        IPTOS_PREC_INTERNETCONTROL,
        IPDEFTTL);
}

// /**
// FUNCTION   :: UmtsLayer3AddIuCsDataHeader
// LAYER      :: Layer 3
// PURPOSE    :: Add a CS data IU interface header
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + msg       : Message*          : Message containing the NBAP message
// + ueId      : NodeId            : Node Id 
// + rabId     : char              : RAB ID to  be added
// **/
void UmtsLayer3AddIuCsDataHeader(
        Node *node,
        Message *msg,
        NodeId ueId,
        char rabId)
{
    char info[10];
    int index = 0;
    memcpy(info, &ueId, sizeof(ueId));
    index += sizeof(ueId);
    info[index++] = rabId;

    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_IU_CSDATA,
                          info,
                          index);
}

// /**
// FUNCTION   :: UmtsAddIubBackboneHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to add the backbone message header
//               at the Iub Interface
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + ueId      : NodeId     : Value of the ueId
// + rbIdOrMsgType:     : UInt8       : Value of rbId or message type
// RETURN     :: void       : NULL
// **/
void UmtsAddIubBackboneHeader(Node *node,
                              Message *msg,
                              NodeId ueId,
                              UInt8 rbIdOrMsgType)
{
    char headerInfo[5];
    memcpy(headerInfo, &ueId, sizeof(ueId));
    memcpy(headerInfo+sizeof(ueId), &rbIdOrMsgType, 1);
    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_IUB,
                          headerInfo,
                          5);
}

// /**
// FUNCTION   :: UmtsAddIuBackboneHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to add the backbone message header
//               at the Iu Interface
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + msg       : Message *              : Message for adding the header
// + ueId      : NodeId                 : Value of the ueId
// + msgType   : UmtsRanapMessageType   : RANAP message type
// RETURN     :: void       : NULL
// **/
void UmtsAddIuBackboneHeader(Node *node,
                             Message *msg,
                             NodeId ueId,
                             UmtsRanapMessageType msgType)
{
    char headerInfo[5];
    headerInfo[0] = (char)msgType;
    memcpy(headerInfo + 1, &ueId, sizeof(ueId));
    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_IU,
                          headerInfo,
                          5);
}

// /**
// FUNCTION   :: UmtsAddNbapHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to add the NBAP message header
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + transctId : unsigned char       : Transaction ID
// + ueId      : NodeId     : UE ID
// RETURN     :: void       : NULL
// **/
void UmtsAddNbapHeader(Node *node,
                       Message *msg,
                       unsigned char transctId,
                       NodeId ueId)
{
    UmtsNbapHeader header;
    MESSAGE_AddHeader(node,
                      msg,
                      sizeof(UmtsNbapHeader),
                      TRACE_UMTS_LAYER3);

    header.transctId = transctId;
    header.ueId = ueId;

    memcpy((char *) MESSAGE_ReturnPacket(msg),
           (char *) &header,
           sizeof(UmtsNbapHeader));
}

// /**
// FUNCTION   :: UmtsRemoveNbapHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to add the NBAP message header
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + transctId : unsigned char* : For returning transaction ID
// + ueId      : NodeId*    : For returnning UE ID
// RETURN     :: void       : NULL
// **/
void UmtsRemoveNbapHeader(Node *node,
                          Message *msg,
                          unsigned char* transctId,
                          NodeId* ueId)
{
    UmtsNbapHeader header;

    memcpy((char *) &header,
           (char *) MESSAGE_ReturnPacket(msg),
           sizeof(UmtsNbapHeader));

    *transctId = header.transctId;
    *ueId = header.ueId;

    MESSAGE_RemoveHeader(node,
                         msg,
                         sizeof(UmtsNbapHeader),
                         TRACE_UMTS_LAYER3);
}

// /**
// FUNCTION   :: UmtsLayer3AddHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to add the standard L3 message header
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + pd        : char       : Value of the PD field of the header
// + tiSpd     : char       : Value of the TI or SPD field of the header
// + msgType   : char       : Value of the Message Type field of the header
// RETURN     :: void       : NULL
// **/
void UmtsLayer3AddHeader(Node *node,
                         Message *msg,
                         char pd,
                         char tiSpd,
                         char msgType)
{
    UmtsLayer3Header l3Header;

    MESSAGE_AddHeader(node,
                      msg,
                      sizeof(UmtsLayer3Header),
                      TRACE_UMTS_LAYER3);

    l3Header.pd = pd;
    l3Header.tiSpd = tiSpd;
    l3Header.msgType = msgType;

    // memcopy is used for compatability with Solaris which only
    // allow pointers pointing to 4-byte aligned addresses.
    memcpy((char *) MESSAGE_ReturnPacket(msg),
           (char *) &l3Header,
           sizeof(UmtsLayer3Header));

}

// /**
// FUNCTION   :: UmtsLayer3RemoveHeader
// LAYER      :: Layer 3
// PURPOSE    :: A utility function to remove the standard L3 message header
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message for adding the header
// + pd        : char*      : For returning PD of the header
// + tiSpd     : char*      : For returning TI or SPD field of the header
// + msgType   : char*      : For returning Message Type field of the header
// RETURN     :: void       : NULL
// **/
void UmtsLayer3RemoveHeader(Node *node,
                            Message *msg,
                            char *pd,
                            char *tiSpd,
                            char *msgType)
{
    UmtsLayer3Header l3Header;

    // memcopy is used for compatability with Solaris which only
    // allow pointers pointing to 4-byte aligned addresses.
    memcpy((char *) &l3Header,
           (char *) MESSAGE_ReturnPacket(msg),
           sizeof(UmtsLayer3Header));

    *pd = l3Header.pd;
    *tiSpd = l3Header.tiSpd;
    *msgType = l3Header.msgType;

    MESSAGE_RemoveHeader(node,
                         msg,
                         sizeof(UmtsLayer3Header),
                         TRACE_UMTS_LAYER3);
}

// /**
// FUNCTION   :: UmtsLayer3SendMsgOverBackbone
// LAYER      :: Layer 3
// PURPOSE    :: A utility function for sending packets on the links
//               connecting NodeB, RNC, SGSN, GGSN, HLR etc.
// PARAMETERS ::
// + node      : Node*      : Pointer to node.
// + msg       : Message *  : Message to be transmitted
// + destAddr  : Address    : Destination address
// + interfaceIndex : int   : Outgoing interface index
// + priority  : TosType    : Priority of the message
// + ttl       : unsigned   : TTL value
// RETURN     :: void       : NULL
// **/
void UmtsLayer3SendMsgOverBackbone(Node *node,
                                   Message *msg,
                                   Address destAddr,
                                   int interfaceIndex,
                                   TosType priority,
                                   unsigned ttl)
{
    if (destAddr.networkType == NETWORK_IPV4)
    {
        NetworkIpSendRawMessage(
            node,
            msg,
            NetworkIpGetInterfaceAddress(node, interfaceIndex), // srcAddr
            GetIPv4Address(destAddr),  //destinationAddress,
            interfaceIndex,            //outgoingInterface,
            priority,                  // priority
            IPPROTO_CELLULAR,          // protocol
            ttl);                      // TTL
    }
    else if (destAddr.networkType == NETWORK_IPV6)
    {
        in6_addr srcAddr;

        Ipv6GetGlobalAggrAddress(node, interfaceIndex, &srcAddr);

        Ipv6SendRawMessage(
            node,
            msg,
            srcAddr,
            GetIPv6Address(destAddr),
            interfaceIndex,
            priority,
            IPPROTO_CELLULAR,
            ttl);
    }
    else
    {
        ERROR_ReportError("UMTS: Unknow destnation address type!");
    }
}

// /**
// FUNCTION   :: UmtsLayer3SendHelloPacket
// LAYER      :: Layer 3
// PURPOSE    :: The fixed nodes such as NodeB, RNC, SGSN will broadcast
//               hello packets to discover the connectivity. This is used
//               to simplify user configurations.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + int       : interfaceIndex   : Interface to send the hello. If value is
//                                  -1, will be send on all possible infs
// + additionalInfoSize :  int    : size of additional info
// + additionalInfo   : chonst char* : additional info
// RETURN     :: void : NULL
// **/
void UmtsLayer3SendHelloPacket(Node *node,
                               UmtsLayer3Data *umtsL3,
                               int interfaceIndex,
                               int additionInfoSize,
                               const char* additionInfo)
{
    UmtsLayer3HelloPacket *helloPkt;
    Message* msg;
    Address destAddr;

    msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_SetLayer(msg, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR);
    MESSAGE_PacketAlloc(node,
                        msg,
                        additionInfoSize + sizeof(UmtsLayer3HelloPacket),
                        TRACE_UMTS_LAYER3);

    helloPkt = (UmtsLayer3HelloPacket *) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(helloPkt != NULL, "UMTS: Memory error!");

    helloPkt->nodeType = umtsL3->nodeType;
    helloPkt->nodeId = node->nodeId;

    memcpy(MESSAGE_ReturnPacket(msg) + sizeof(UmtsLayer3HelloPacket),
           additionInfo,
           additionInfoSize);

    UmtsLayer3AddHeader(node,
                        msg,
                        UMTS_LAYER3_PD_SIM,
                        0,
                        0);

    //Add a bockbone header
    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_HELLO,
                          NULL,
                          0);

    if (interfaceIndex >= 0)
    {
        // a valid interface, so only send on this interface
        if (MAC_IsWiredNetwork(node, interfaceIndex))
        {
            SetIPv4AddressInfo(&destAddr,
                               NetworkIpGetInterfaceBroadcastAddress(
                                   node,
                                   interfaceIndex));
        }
        else
        {
            SetIPv4AddressInfo(&destAddr, ANY_DEST);
        }

        UmtsLayer3SendMsgOverBackbone(
            node,
            msg,
            destAddr,
            interfaceIndex,
            IPTOS_PREC_INTERNETCONTROL,
            1);
    }
    else
    {
        // not a valid interface, send on all possible interfaces
        // except the UMTS radio interface at NodeB.
        int i;

        for (i = 0; i < node->numberInterfaces; i ++)
        {
            if (node->macData[i]->macProtocol != MAC_PROTOCOL_CELLULAR)
            {
                if (MAC_IsWiredNetwork(node, i))
                {
                    SetIPv4AddressInfo(
                        &destAddr,
                        NetworkIpGetInterfaceBroadcastAddress(
                        node,
                        i));
                }
                else
                {
                    SetIPv4AddressInfo(&destAddr, ANY_DEST);
                }

                UmtsLayer3SendMsgOverBackbone(
                   node,
                   MESSAGE_Duplicate(node, msg),
                   destAddr,
                   i,
                   IPTOS_PREC_INTERNETCONTROL,
                   1);
            }
        }

        MESSAGE_Free(node, msg);
    }
}

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: UmtsLayer3ReportAmRlcError
// LAYER      :: Layer3
// PURPOSE    :: Called by RLC to report 
//               unrecoverable error for AM RLC entity.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + ueId             : NodeId            : UE identifier
// + rbId             : char              : Radio bearer ID
// RETURN     :: void : NULL
// **/
void UmtsLayer3ReportAmRlcError(Node *node,
                                NodeId ueId,
                                char rbId)
{
    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);
    if (DEBUG_RR)
    {
        printf ("Node: %u report an AM RLC error for UE: %u RB: %d\n",
            node->nodeId, ueId, rbId);
    }
    if (UmtsIsUe(node))
    {
#if 0
        UmtsLayer3UeReportAmRlcError(node,
                                     umtsL3,
                                     ueId,
                                     rbId);
#endif
        Message* msg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            NETWORK_PROTOCOL_CELLULAR,
                            MSG_NETWORK_FromMac);
        MESSAGE_SetInstanceId(msg, 0);

        char info[MAX_STRING_LENGTH];
        unsigned int index = 0;
        UmtsInterlayerMsgType type = UMTS_REPORT_AMRLC_ERROR;
        memcpy(&info[index], &type, sizeof(type));
        index += sizeof(type);
        memcpy(&info[index], &ueId, sizeof(ueId));
        index += sizeof(ueId);
        info[index++] = rbId;

        MESSAGE_InfoAlloc(
            node,
            msg,
            index);
        memcpy(MESSAGE_ReturnInfo(msg),
               info,
               index);

        MESSAGE_Send(node,
                     msg,
                     1 * MILLI_SECOND);

    }
    else
    {
        UmtsLayer3NodebReportAmRlcError(node,
                                        umtsL3,
                                        ueId,
                                        rbId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3ReceivePacketFromMacLayer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// + lastHopAddress   : NodeAddress       : Address of the last hop
// + interfaceIndex   : int               : Interface from which
//                                          the packet is received
// RETURN     :: void : NULL
// **/
void UmtsLayer3ReceivePacketFromMacLayer(Node *node,
                                         Message *msg,
                                         NodeAddress lastHopAddress,
                                         int interfaceIndex)
{
    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);
    if (umtsL3->nodeType == CELLULAR_UE)
    {
        // UE is in OFF state. So cancel the message.
        UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
        if (ueL3->mmData->mainState == UMTS_MM_MS_NULL)
        {
            MESSAGE_Free(node, msg);
            return;
        }
        UmtsLayer3UeReceivePacketFromMacLayer(
            node,
            interfaceIndex,
            umtsL3,
            msg,
            lastHopAddress);

    }
    else if (umtsL3->nodeType == CELLULAR_NODEB)
    {
        UmtsLayer3NodebReceivePacketFromMacLayer(
            node,
            interfaceIndex,
            umtsL3,
            msg,
            lastHopAddress);
    }
    else
    {
        ERROR_ReportError("UmtsLayer3ReceivePacketFromMacLayer: "
            " Unknow node type!");
    }
}

// /**
// FUNCTION   :: UmtsLayer3PreInit
// LAYER      :: Layer 3
// PURPOSE    :: Pre-Initialize UMTS layer 3 data. It may trigger node type
//               specific initialization too.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void UmtsLayer3PreInit(Node* node, const NodeInput* nodeInput)
{
    CellularLayer3Data *cellularData;
    UmtsLayer3Data *umtsL3;

    // initialize the basic UMTS layer3 data
    umtsL3 = (UmtsLayer3Data *) MEM_malloc(sizeof(UmtsLayer3Data));
    ERROR_Assert(umtsL3 != NULL, "UMTS: Out of memory!");
    memset(umtsL3, 0, sizeof(UmtsLayer3Data));

    cellularData = (CellularLayer3Data *)
                   node->networkData.cellularLayer3Var;
    ERROR_Assert(cellularData != NULL, "UMTS: Memory error!");

    cellularData->umtsL3 = umtsL3;

    // initialize the seed
    RANDOM_SetSeed(umtsL3->seed,
                   node->globalSeed,
                   node->nodeId,
                   NETWORK_PROTOCOL_CELLULAR,
                   Cellular_UMTS_Layer3);

    // read configuration parameters
    UmtsLayer3InitParameter(node, nodeInput);

    if (DEBUG_PARAMETER)
    {
        UmtsLayer3PrintParameter(node, umtsL3);
    }
    return;
}

// /**
// FUNCTION   :: UmtsLayer3Init
// LAYER      :: Layer 3
// PURPOSE    :: Initialize UMTS layer 3 data. It may trigger node type
//               specific initialization too.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void UmtsLayer3Init(Node* node, const NodeInput* nodeInput)
{
    BOOL wasFound;
    char strVal[MAX_STRING_LENGTH];
    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);

    // read paramete to see if need to collect and print stats
    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "CELLULAR-STATISTICS",
                  &wasFound,
                  strVal);

    if (!wasFound || strcmp(strVal, "YES") == 0)
    {
        umtsL3->collectStatistics = TRUE;
    }
    else if (wasFound && strcmp(strVal, "NO") == 0)
    {
        umtsL3->collectStatistics = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Value of CELLULAR-STATISTICS should be YES or NO!"
            "Default value YES is used.");
        umtsL3->collectStatistics = TRUE;
    }

    if (umtsL3->collectStatistics)
    {
        umtsL3->newStats = new STAT_NetStatistics(node);
    }
    else
    {
        umtsL3->newStats = NULL;
    }

    // initialize node type specific items
    if (umtsL3->nodeType == CELLULAR_UE)
    {
        // initialize UE specific data structures
        UmtsLayer3GetL2InterfaceIndex(node);
        UmtsLayer3UeInit(node, nodeInput, umtsL3);
    }
    else if (umtsL3->nodeType == CELLULAR_NODEB)
    {
        // initialize NodeB specific data structures
        UmtsLayer3GetL2InterfaceIndex(node);
        UmtsLayer3NodebInit(node, nodeInput, umtsL3);

    }
    else if (umtsL3->nodeType == CELLULAR_RNC)
    {
        // initialize RNC specific data structures
        UmtsLayer3RncInit(node, nodeInput, umtsL3);
    }
    else if (umtsL3->nodeType == CELLULAR_SGSN ||
             umtsL3->nodeType == CELLULAR_GGSN)
    {
        // initialize SGSN and GGSN specific data structures
        UmtsLayer3GsnInit(node, nodeInput, umtsL3);
    }
    else if (umtsL3->nodeType == CELLULAR_HLR)
    {
        // initialize HLR specific data structures
        UmtsLayer3HlrInit(node, nodeInput, umtsL3);
    }
    else
    {
        ERROR_ReportError("UMTS: Unknow node type!");
    }

    return;
}

// /**
// FUNCTION   :: UmtsLayer3Layer
// LAYER      :: Layer 3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + msg              : Message* : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void UmtsLayer3Layer(Node* node, Message* msg)
{
    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);
    BOOL handled = FALSE;

    if (!handled)
    {
        if (umtsL3->nodeType == CELLULAR_UE)
        {
            UmtsLayer3UeLayer(node, msg, umtsL3);
        }
        else if (umtsL3->nodeType == CELLULAR_NODEB)
        {
            UmtsLayer3NodebLayer(node, msg, umtsL3);
        }
        else if (umtsL3->nodeType == CELLULAR_RNC)
        {
            UmtsLayer3RncLayer(node, msg, umtsL3);
        }
        else if (umtsL3->nodeType == CELLULAR_SGSN ||
                 umtsL3->nodeType == CELLULAR_GGSN)
        {
            UmtsLayer3GsnLayer(node, msg, umtsL3);
        }
        else if (umtsL3->nodeType == CELLULAR_HLR)
        {
            UmtsLayer3HlrLayer(node, msg, umtsL3);
        }
        else
        {
            ERROR_ReportWarning("UMTS: Unknown node type!");
            MESSAGE_Free(node, msg);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3ReceivePacketOverIp
// LAYER      :: Layer 3
// PURPOSE    :: Handle received packet from IP
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Message for node to interpret.
// + interfaceIndex : int : Interface from which packet was received
// + srcAddr   : Address  : Address of the source of the packet
// RETURN     :: void : NULL
// **/
void UmtsLayer3ReceivePacketOverIp(Node* node,
                                   Message* msg,
                                   int interfaceIndex,
                                   Address srcAddr)
{
    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);
    BOOL handled = FALSE;

    if (!handled)
    {
        if (umtsL3->nodeType == CELLULAR_NODEB)
        {
            UmtsLayer3NodebHandlePacket(node,
                                        msg,
                                        umtsL3,
                                        interfaceIndex,
                                        srcAddr);
        }
        else if (umtsL3->nodeType == CELLULAR_RNC)
        {
            UmtsLayer3RncHandlePacket(node,
                                      msg,
                                      umtsL3,
                                      interfaceIndex,
                                      srcAddr);
        }
        else if (umtsL3->nodeType == CELLULAR_SGSN ||
                 umtsL3->nodeType == CELLULAR_GGSN)
        {
            UmtsLayer3GsnHandlePacket(node,
                                      msg,
                                      umtsL3,
                                      interfaceIndex,
                                      srcAddr);
        }
        else if (umtsL3->nodeType == CELLULAR_HLR)
        {
            UmtsLayer3HlrHandlePacket(node,
                                      msg,
                                      umtsL3,
                                      interfaceIndex,
                                      srcAddr);
        }
        else
        {
            ERROR_ReportWarning("UMTS Layer3: Unknown packet!");
            MESSAGE_Free(node, msg);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3Finalize
// LAYER      :: Layer 3
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void UmtsLayer3Finalize(Node *node)
{
    CellularLayer3Data* cellularData;
    cellularData = (CellularLayer3Data *)
                   node->networkData.cellularLayer3Var;
    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);

    if (umtsL3->collectStatistics)
    {
        // print general statistics
        UmtsLayer3PrintStats(node, umtsL3);
    }

    // print node specific statistics
    if (umtsL3->nodeType == CELLULAR_UE)
    {
        UmtsLayer3UeFinalize(node, umtsL3);
    }
    else if (umtsL3->nodeType == CELLULAR_NODEB)
    {
        UmtsLayer3NodebFinalize(node, umtsL3);
    }
    else if (umtsL3->nodeType == CELLULAR_RNC)
    {
        UmtsLayer3RncFinalize(node, umtsL3);
    }
    else if (umtsL3->nodeType == CELLULAR_SGSN ||
             umtsL3->nodeType == CELLULAR_GGSN)
    {
        UmtsLayer3GsnFinalize(node, umtsL3);
    }
    else if (umtsL3->nodeType == CELLULAR_HLR)
    {
        UmtsLayer3HlrFinalize(node, umtsL3);
    }
    else
    {
        ERROR_ReportWarning("UMTS: Unknown node type!");
    }

    // free the memory
    MEM_free(cellularData->umtsL3);
    cellularData->umtsL3 = NULL;
}

//  /**
// FUNCITON   :: UmtsLayer3HandleInterLayerCommand
// LAYER      :: UMTS L3 at UE
// PURPOSE    :: Handle Interlayer command
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + cmdType          : UmtsInterlayerCommandType : command type
// + interfaceIdnex   : UInt32            : interface index of UMTS
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
void UmtsLayer3HandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd)
{
    if (UmtsIsUe(node))
    {
        UmtsLayer3UeHandleInterLayerCommand(
            node,
            interfaceIndex,
            cmdType,
            cmd);
    }
    else if (UmtsIsNodeB(node))
    {
        UmtsLayer3NodebHandleInterLayerCommand(
            node,
            interfaceIndex,
            cmdType,
            cmd);
    }
    else
    {
        // more goes to here 
    }
}

// /**
// FUNCTION   :: UmtsLayer3HandlePacketFromUpperOrOutside
// LAYER      :: Layer3
// PURPOSE    :: Handle pakcets from upper
// PARAMETERS ::
// + node           : Node*    : Pointer to node.
// + msg            : Message* : Message to be sent onver the air interface
// + interfaceIndex : int      : Interface from which the packet is received
// + networkType    : int      : network type, IPv4 or IPv6
// RETURN     :: void : NULL
// **/
void UmtsLayer3HandlePacketFromUpperOrOutside(
         Node* node,
         Message* msg,
         int interfaceIndex,
         int networkType)
{
    if (UmtsIsUe(node))
    {
        UmtsLayer3UeHandlePacketFromUpper(
            node,
            msg,
            interfaceIndex,
            networkType);
    }
    else if (UmtsIsGgsn(node))
    {
        UmtsLayer3GgsnHandlePacketFromUpperOrOutside(
            node,
            msg,
            interfaceIndex,
            networkType);
    }
    else
    {
        ERROR_ReportWarning("Only UE or GGSN supports IP data packets");
        MESSAGE_Free(node, msg);
    }
}

//////////////////////////
// Handle app classifier
//////////////////////////
// /**
// FUNCTION   :: UmtsLayer3BuildFlowClassifierInfo
// LAYER      :: MAC
// PURPOSE    :: Fill in classifier info structure.
// PARAMETERS ::
// + node        : Node*            : Pointer to node.
// + interfaceIndex : int           : interfaceIndex
// + networkType : int              : Type of network
// + msg         : Message*         : Packet from upper layer
// + tos         : TosType*         : Priority of packet
// + classifierInfo   : MacDot16CsClassifier* : classifier info of this flow
// + payload     : char* : Packet payload
// RETURN     :: void : NULL
// **/
void UmtsLayer3BuildFlowClassifierInfo(
        Node* node,
        int interfaceIndex,
        int networkType,
        Message* msg,
        TosType* tos,
        UmtsLayer3FlowClassifier* classifierInfo,
        const char** payload)
{
    IpHeaderType ipHeader;
    ip6_hdr ipv6Header;

    memset(classifierInfo, 0, sizeof(UmtsLayer3FlowClassifier));

    TraceProtocolType appType =
            (TraceProtocolType) msg->originatingProtocol;

    if (appType == TRACE_CELLULAR_PHONE)
    {
        classifierInfo->domainInfo = UMTS_LAYER3_CN_DOMAIN_CS;
    }
    else
    {
        classifierInfo->domainInfo = UMTS_LAYER3_CN_DOMAIN_PS;
    }

    // Build classifier info structure for IPv4
    if (networkType == NETWORK_IPV4)
    { // IPv4 packet
        // get IP header
        memcpy(&ipHeader, *payload, sizeof(ipHeader));
        *payload += IpHeaderSize(&ipHeader);

        if (ipHeader.ip_p == IPPROTO_UDP)
        { // this is a UDP packet
            TransportUdpHeader udpHdr;

            memcpy(&udpHdr, *payload, sizeof(udpHdr));
            *payload += sizeof(TransportUdpHeader);

            SetIPv4AddressInfo( &classifierInfo->srcAddr, ipHeader.ip_src);
            SetIPv4AddressInfo( &classifierInfo->dstAddr, ipHeader.ip_dst);

            // for UMTS model, IP packets are not fragmented at this time
            // it will be fragmented at RLC layer
            classifierInfo->srcPort = udpHdr.sourcePort;
            classifierInfo->dstPort = udpHdr.destPort;

            classifierInfo->ipProtocol = IPPROTO_UDP;
        }
        else if (ipHeader.ip_p == IPPROTO_TCP)
        { // this is a TCP packet
            struct tcphdr tcpHdr;

            memcpy(&tcpHdr, *payload, sizeof(tcpHdr));
            *payload += sizeof(struct tcphdr);

            SetIPv4AddressInfo( &classifierInfo->srcAddr, ipHeader.ip_src);
            SetIPv4AddressInfo( &classifierInfo->dstAddr, ipHeader.ip_dst);

            classifierInfo->srcPort = tcpHdr.th_sport;
            classifierInfo->dstPort = tcpHdr.th_dport;

            classifierInfo->ipProtocol = IPPROTO_TCP;
        }
        else
        {
            // non-app layer packet, might be routing packet etc
            // Classify all such packet into one queue
            SetIPv4AddressInfo( &classifierInfo->srcAddr, ipHeader.ip_src);
            SetIPv4AddressInfo( &classifierInfo->dstAddr, ipHeader.ip_dst);
            classifierInfo->srcPort = 0;
            classifierInfo->dstPort = 0;
            classifierInfo->ipProtocol = IPPROTO_IP;
        }

        // get the packet TOS and use it as the priority
        *tos = IpHeaderGetTOS(ipHeader.ip_v_hl_tos_len) >> 2;

        if (ipHeader.ip_dst == ANY_ADDRESS)
        {
            // Code goes to here
        }
        else if (NetworkIpIsMulticastAddress(node, ipHeader.ip_dst))
        {
            // Code goes to here
        }
    }

    // Build classifier info structure for IPv6
    else if (networkType == NETWORK_PROTOCOL_IPV6)
    { // IPv6 packet
        // get IPv6 header
        memcpy(&ipv6Header, *payload, sizeof(ipv6Header));
        *payload += sizeof(ip6_hdr);

        // get the packet priority and use it as the tos
        *tos = ip6_hdrGetClass(ipv6Header.ipv6HdrVcf) >> 2;

        int nextHdr = ipv6Header.ip6_nxt;
        int hdrLength = ipv6Header.ip6_plen;

        // look at payload
        while (nextHdr)
        {

            if (nextHdr == IPPROTO_UDP) {
                // this is a UDP packet
                TransportUdpHeader udpHdr;

                memcpy(&udpHdr, *payload, sizeof(udpHdr));
                *payload += sizeof(TransportUdpHeader);

                SetIPv6AddressInfo( &classifierInfo->srcAddr,
                                    ipv6Header.ip6_src);
                SetIPv6AddressInfo( &classifierInfo->dstAddr,
                                    ipv6Header.ip6_dst);
                classifierInfo->srcPort = udpHdr.sourcePort;
                classifierInfo->dstPort = udpHdr.destPort;
                classifierInfo->ipProtocol = IPPROTO_UDP;
                break;
            }

            else if (nextHdr == IPPROTO_TCP) {
                // this is a TCP packet
                struct tcphdr tcpHdr;

                memcpy(&tcpHdr, *payload, sizeof(tcpHdr));
                *payload += sizeof(struct tcphdr);

                SetIPv6AddressInfo( &classifierInfo->srcAddr,
                                    ipv6Header.ip6_src);
                SetIPv6AddressInfo( &classifierInfo->dstAddr,
                                    ipv6Header.ip6_dst);
                classifierInfo->srcPort = tcpHdr.th_sport;
                classifierInfo->dstPort = tcpHdr.th_dport;
                classifierInfo->ipProtocol = IPPROTO_TCP;
                break;
            }
            else if (nextHdr == IPPROTO_ICMPV6 )
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                SetIPv6AddressInfo( &classifierInfo->srcAddr,
                                    ipv6Header.ip6_src);
                SetIPv6AddressInfo( &classifierInfo->dstAddr,
                                    ipv6Header.ip6_dst);
                classifierInfo->srcPort = 0;
                classifierInfo->dstPort = 0;
                classifierInfo->ipProtocol = IPPROTO_ICMPV6;
                break;
            }

            else if (nextHdr == IPPROTO_OSPF )
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                SetIPv6AddressInfo(
                    &classifierInfo->srcAddr, ipv6Header.ip6_src);
                SetIPv6AddressInfo(
                    &classifierInfo->dstAddr, ipv6Header.ip6_dst);
                classifierInfo->srcPort = 0;
                classifierInfo->dstPort = 0;
                classifierInfo->ipProtocol = IPPROTO_OSPF;
                break;
            }

            // get next header
            *payload += ipv6Header.ip6_plen;
            memcpy(&ipv6Header, *payload, sizeof(ipv6Header));

            nextHdr = ipv6Header.ip6_nxt;
            hdrLength = ipv6Header.ip6_plen;

            if (hdrLength == 0)
            {
                // non-applayer packet, might be routing packet etc
                // Classify all such packet into one queue
                SetIPv6AddressInfo( &classifierInfo->srcAddr,
                                    ipv6Header.ip6_src);
                SetIPv6AddressInfo( &classifierInfo->dstAddr,
                                    ipv6Header.ip6_dst);
                classifierInfo->srcPort = 0;
                classifierInfo->dstPort = 0;
                classifierInfo->ipProtocol = IPPROTO_IPV6;
                break;
            }

        }

        // broadcasts/ multicast
        if (IS_MULTIADDR6(ipv6Header.ip6_dst))
        {
            // TODO: muticast
        }
    }

    else if (networkType == NETWORK_ATM)
    { 
        // ATM packet, use ATM classifier
        ERROR_ReportWarning("MAC 802.16: ATM is not supported yet!");
        MESSAGE_Free(node, msg);
        return;
    }
}

// /**
// FUNCTION   :: UmtsGetLayer3GetTrafficClassFromTos
// LAYER      :: MAC
// PURPOSE    :: Fill in classifier info structure.
// PARAMETERS ::
// + tos            : TosType          : Tos type
// RETURN     :: UmtsQoSTrafficClass : QoS traffic class
// **/
static
UmtsQoSTrafficClass UmtsGetLayer3GetTrafficClassFromTos(TosType tos)
{
    UmtsQoSTrafficClass trafficCls;

    switch (tos)
    {
        case 0: // precedence 0
        {
            trafficCls = UMTS_QOS_CLASS_BACKGROUND;

            break;
        }
        case 8:
        {
            trafficCls = UMTS_QOS_CLASS_INTERACTIVE_3;

            break;
        }
        case 16:
        {
            trafficCls = UMTS_QOS_CLASS_INTERACTIVE_2;

            break;
        }
        case 24:
        {
            trafficCls = UMTS_QOS_CLASS_INTERACTIVE_1;

            break;
        }
        case 32:
        case 40: // precedence 5
        {
            trafficCls = UMTS_QOS_CLASS_STREAMING;

            break;
        }
        case 48: // precedence 6
        case 56: // precedence 7
        {
             trafficCls = UMTS_QOS_CLASS_CONVERSATIONAL;

            break;
        }
        default:
        {
            trafficCls = UMTS_QOS_CLASS_BACKGROUND;
        }
    }

    return trafficCls;
}

// /**
// FUNCTION   :: UmtsLayer3CheckFlowQoSParam
// LAYER      :: MAC
// PURPOSE    :: Check basic QoS parameters.
// PARAMETERS ::
// + node           : Node*            : Pointer to node.
// + interfaceIndex : int              : interfaceIndex
// + appType        : TraceProtocolType : trace protocol type
// + tos            : TosType          : Tos type
// + payload        : char**           : Packet payload
// RETURN     :: void : NULL
// **/
BOOL UmtsLayer3CheckFlowQoSParam(
        Node* node,
        int interfaceIndex,
        TraceProtocolType appType,
        TosType tos,
        const char** payload)
{
    UInt64 maxPacketSize = 0;
    UmtsQoSTrafficClass trafficClass;
    clocktype interval = 0;
    unsigned int maxBitRate = 0;
    BOOL tooHighRate = FALSE;
    UInt64 sysMaxPacketSize = 0;
    unsigned int sysMaxRate = 0;
    char errBuf[255];
    char warnBuf[2550];
    int ipOverheader = UMTS_DATA_IP_OVERHEAD_SIZE;

    trafficClass = UmtsGetLayer3GetTrafficClassFromTos(tos);

    switch(appType)
    {

        case TRACE_CBR:
        {
            // this is a CBR packet
            CbrData cbrData;
            memcpy(&cbrData, *payload, sizeof(cbrData));
            
            maxPacketSize = (cbrData.pktSize + ipOverheader) * 8;
            maxBitRate = (int) ((cbrData.pktSize + ipOverheader) * 8 *
                                 SECOND / cbrData.interval);
            interval = cbrData.interval;

            break;
        }

        case TRACE_VBR:
        {
             // this is a VBR packet
            VbrData vbrData;
            memcpy(&vbrData, *payload, sizeof(vbrData));
            
            maxPacketSize = (vbrData.itemSize + ipOverheader) * 8;
            maxBitRate = (int) ((vbrData.itemSize + ipOverheader) * 8 *
                                SECOND / vbrData.meanInterval);
            interval = vbrData.meanInterval;

            break;
        }

        case TRACE_TCP:
        {

#if 0
            //should not use cast, instead use memcpy
            SuperApplicationTCPDataPacket* tcpDataPtr =
                    (SuperApplicationTCPDataPacket*) *payload;

            maxPacketSize = dataPtr->pktSize * 8;
            maxBitRate = (int) (dataPtr->pktSize * 8 *
                          SECOND / dataPtr->interval);
            interval = tcpDataPtr->interval;
#endif
            //break;
        }

    default:
        {
            trafficClass = UMTS_QOS_CLASS_BACKGROUND;
        }
    }

    if (trafficClass == UMTS_QOS_CLASS_CONVERSATIONAL)
    {
        UInt64 pktSize28800 = 
            umtsDefaultDataDchTransFormatConv_28800.transBlkSetSize;
        UInt64 pktSize32000 = 
            umtsDefaultDataDchTransFormatConv_32000.transBlkSetSize;
        UInt64 pktSize64000 = 
            umtsDefaultDataDchTransFormatConv_64000.transBlkSetSize;
        UInt64 pktSize128000 = 
            umtsDefaultDataDchTransFormatConv_128000.transBlkSetSize;

        sprintf(
               warnBuf,
               " QualNet UMTS Model: Rate & Packet Format Supported (Conversational)\n"
               "----------------------------------------------------------------------\n"
               "Rate Level----Max Data Rate (bps)----Max Data Packet Size (bytes)\n"
               "  1 (TTI 40ms)     28800                          %15" TYPES_64BITFMT"d\n"
               "  2 (TTI 20ms)     32000                          %15" TYPES_64BITFMT"d\n"
               "  3 (TTI 20ms)     64000                          %15" TYPES_64BITFMT"d\n"
               "  4 (TTI 20ms)     128000                         %15" TYPES_64BITFMT"d\n"
               "---------------------------------------------------------------------\n"
               "Refer to QualNet UMTS Model User's Guide on how to change these values\n",
               pktSize28800 / 8 - ipOverheader,
               pktSize32000 / 8 - ipOverheader,
               pktSize64000 / 8 - ipOverheader,
               pktSize128000 / 8 - ipOverheader);


        if (maxBitRate <= 28800)
        {
            sysMaxRate = 28800;
            sysMaxPacketSize =
                umtsDefaultDataDchTransFormatConv_28800.transBlkSetSize;
        }
        else if (maxBitRate <= 32000)
        {
            sysMaxRate = 32000;
            sysMaxPacketSize =
                umtsDefaultDataDchTransFormatConv_32000.transBlkSetSize;
        }
        else if (maxBitRate <= 64000)
        {
            sysMaxRate = 64000;
            sysMaxPacketSize =
                umtsDefaultDataDchTransFormatConv_64000.transBlkSetSize;
        }
        else if (maxBitRate <= 128000)
        {
            sysMaxRate = 128000;
            sysMaxPacketSize =
                umtsDefaultDataDchTransFormatConv_128000.transBlkSetSize;
        }
        else
        {
            tooHighRate = TRUE;
            sysMaxRate = 128000;
        }
        if (tooHighRate)
        {
           sprintf(
                errBuf,
                "node %d: Conversational application with rate %d bps "
                "beyong the system definition %d bps",
                node->nodeId, maxBitRate, sysMaxRate);

           ERROR_ReportWarning(errBuf);
           ERROR_ReportError(warnBuf);

           return FALSE;
        }
        if (maxPacketSize > sysMaxPacketSize)
        {
            sprintf(
                errBuf,
                "node %d: Conversational application (rate %d bps) "
                "with packet size %15" TYPES_64BITFMT"d "
                "bits, larger than the maximum value %15" TYPES_64BITFMT"d "
                "bits for rate %d bps.\n"
                "Pakcets will be dropped at the sources!\n",
                node->nodeId, maxBitRate, maxPacketSize / 8 - ipOverheader,
                sysMaxPacketSize / 8 - ipOverheader, sysMaxRate);


            ERROR_ReportWarning(errBuf);
            ERROR_ReportError(warnBuf);


            return FALSE;
        }
    }
    else if (trafficClass == UMTS_QOS_CLASS_STREAMING)
    {
        UInt64 pktSize14400 = 
            umtsDefaultDataDchTransFormatStream_14400.transBlkSetSize;
        UInt64 pktSize28800 = 
            umtsDefaultDataDchTransFormatStream_28800.transBlkSetSize;
        UInt64 pktSize57600 = 
            umtsDefaultDataDchTransFormatStream_57600.transBlkSetSize;
        UInt64 pktSize115200 = 
            umtsDefaultDataDchTransFormatStream_115200.transBlkSetSize;

        sprintf(
               warnBuf,
               " QualNet UMTS Model: Rate & Packet Format Supported (Streaming)\n"
               "----------------------------------------------------------------------\n"
               "Rate Level----Max Data Rate (bps)----Max Data Packet Size (bytes)\n"
               "  1 (TTI 40ms)     14400                          %15" TYPES_64BITFMT"d\n"
               "  2 (TTI 40ms)     28800                          %15" TYPES_64BITFMT"d\n"
               "  3 (TTI 40ms)     57600                          %15" TYPES_64BITFMT"d\n"
               "  4 (TTI 40ms)     115200                         %15" TYPES_64BITFMT"d\n"
               "---------------------------------------------------------------------\n"
               "Refer to QualNet UMTS Model User's Guide on how to change these values\n",
               pktSize14400 / 8 - ipOverheader,
               pktSize28800 / 8 - ipOverheader,
               pktSize57600 / 8 - ipOverheader,
               pktSize115200 / 8 - ipOverheader);

        if (maxBitRate <= 14400)
        {
            sysMaxRate = 14400;
            sysMaxPacketSize =
                umtsDefaultDataDchTransFormatStream_14400.transBlkSetSize;
        }
        else if (maxBitRate <= 28800)
        {
            sysMaxRate = 28800;
            sysMaxPacketSize =
                umtsDefaultDataDchTransFormatStream_28800.transBlkSetSize;
        }
        else if (maxBitRate <= 57600)
        {
            sysMaxRate = 57600;
            sysMaxPacketSize =
                umtsDefaultDataDchTransFormatStream_57600.transBlkSetSize;
        }
        else if (maxBitRate <= 115200)
        {
            sysMaxRate = 115200;
            sysMaxPacketSize =
                umtsDefaultDataDchTransFormatStream_115200.transBlkSetSize;
        }
        else
        {
            sysMaxRate = 115200;
            tooHighRate = TRUE;
        }
        if (tooHighRate)
        {
           sprintf(
                errBuf,
                "node %d: Streaming application with rate %d bps "
                "beyong the system definition %d bps",
                node->nodeId, maxBitRate, sysMaxRate);

           ERROR_ReportWarning(errBuf);
           ERROR_ReportError(warnBuf);

           return FALSE;
        }
        if (maxPacketSize > sysMaxPacketSize)
        {
            sprintf(
                errBuf,
                "node %d: Streaming application (rate %d bps) "
                "with packet size %15" TYPES_64BITFMT"d "
                "bytes, larger than the maximum value %15" TYPES_64BITFMT"d " 
                "bytes for rate %d bps.\n"
                "Pakcets will be dropped at the sources!\n",
                node->nodeId, maxBitRate, maxPacketSize / 8 - ipOverheader,
                sysMaxPacketSize / 8 - ipOverheader, sysMaxRate);

            ERROR_ReportWarning(errBuf);
            ERROR_ReportError(warnBuf);

            return FALSE;
        }
    }

    return TRUE;
}

// /**
// FUNCTION   :: UmtsLayer3GetFlowQoSInfo
// LAYER      :: MAC
// PURPOSE    :: Fill in classifier info structure.
// PARAMETERS ::
// + node           : Node*            : Pointer to node.
// + interfaceIndex : int              : interfaceIndex
// + appType        : TraceProtocolType : trace protocol type
// + tos            : TosType          : Tos type
// + payload        : const char**           : Packet payload
// + qosInfo        : UmtsFlowQoSInfo** : QoS info
// RETURN     :: void : NULL
// **/
BOOL UmtsLayer3GetFlowQoSInfo(
        Node* node,
        int interfaceIndex,
        TraceProtocolType appType,
        TosType tos,
        const char** payload,
        UmtsFlowQoSInfo** qosInfo)
{
    int ipOverHeader = UMTS_DATA_IP_OVERHEAD_SIZE;
    switch(appType)
    {

        case TRACE_CBR:
        {
            // this is a CBR packet
            CbrData dataPtr; 
            memcpy(&dataPtr, *payload, sizeof(dataPtr));
            
            if (UmtsGetNodeType(node) == CELLULAR_UE)
            {
                (*qosInfo)->ulGuaranteedBitRate = 
                    (int) ((dataPtr.pktSize + ipOverHeader) * 8 *
                            SECOND / dataPtr.interval);
                (*qosInfo)->ulMaxBitRate = (*qosInfo)->ulGuaranteedBitRate;
                (*qosInfo)->dlGuaranteedBitRate = 0;
                (*qosInfo)->dlMaxBitRate = 0;
            }
            else if (UmtsGetNodeType(node) == CELLULAR_SGSN ||
                     UmtsGetNodeType(node) == CELLULAR_GGSN)
            {
                (*qosInfo)->dlGuaranteedBitRate = 
                    (int) ((dataPtr.pktSize + ipOverHeader) * 8 *
                            SECOND / dataPtr.interval);
                (*qosInfo)->dlMaxBitRate = (*qosInfo)->dlGuaranteedBitRate;
                (*qosInfo)->ulGuaranteedBitRate =  0;
                (*qosInfo)->ulMaxBitRate =  0;
            }
            (*qosInfo)->residualBER = 0; //
            (*qosInfo)->sduErrorRatio = 0; // TODO
            (*qosInfo)->transferDelay = dataPtr.interval;
            (*qosInfo)->trafficClass = 
                UmtsGetLayer3GetTrafficClassFromTos(tos);

            break;
        }

        case TRACE_VBR:
        {
             // this is a VBR packet
            VbrData dataPtr;
            memcpy(&dataPtr, *payload, sizeof(dataPtr));

            if (UmtsGetNodeType(node) == CELLULAR_UE)
            {
                (*qosInfo)->ulGuaranteedBitRate = 
                    (int) ((dataPtr.itemSize + ipOverHeader) * 8 *
                    SECOND / dataPtr.meanInterval);
                (*qosInfo)->ulMaxBitRate = (*qosInfo)->ulGuaranteedBitRate;
                (*qosInfo)->dlGuaranteedBitRate = 0;
                (*qosInfo)->dlMaxBitRate = 0;
            }
            else if (UmtsGetNodeType(node) == CELLULAR_SGSN ||
                     UmtsGetNodeType(node) == CELLULAR_GGSN)
            {
                (*qosInfo)->dlGuaranteedBitRate = 
                    (int) ((dataPtr.itemSize + ipOverHeader) * 8 *
                    SECOND / dataPtr.meanInterval);
                (*qosInfo)->dlMaxBitRate = (*qosInfo)->dlGuaranteedBitRate;
                (*qosInfo)->ulGuaranteedBitRate = 0;
                (*qosInfo)->ulMaxBitRate = 0;
            }

            (*qosInfo)->residualBER = 0; // TODO
            (*qosInfo)->sduErrorRatio = 0; // TODO
            (*qosInfo)->transferDelay = dataPtr.meanInterval;
            (*qosInfo)->trafficClass = 
                UmtsGetLayer3GetTrafficClassFromTos(tos);

            break;
        }

        case TRACE_TCP:
        {

#if 0

            SuperApplicationTCPDataPacket* tcpDataPtr =
                    (SuperApplicationTCPDataPacket*) *payload;

            if (tcpDataPtr->traceType == TRACE_SUPERAPPLICATION)
            {
                (*qosInfo)->guaranteedBitRate = 
                    (int) (dataPtr->pktSize * 8 *
                     SECOND / dataPtr->interval);
                (*qosInfo)->maxBitRate = tcpDataPtr->guaranteedBitRate;
                (*qosInfo)->residualBER = 0; // TODO
                (*qosInfo)->sduErrorRatio = 0; // TODO
                (*qosInfo)->transferDelay = tcpDataPtr->interval;
                (*qosInfo)->trafficClass = 
                    UmtsGetLayer3GetTrafficClassFromTos(tos);
            }
            else
            {
                return FALSE;
            }
#endif
        }

    default:
        {
            // app info not avaliable
            if (UmtsGetNodeType(node) == CELLULAR_UE)
            {
                (*qosInfo)->ulGuaranteedBitRate = 64000;
                (*qosInfo)->ulMaxBitRate = 64000;
                (*qosInfo)->dlGuaranteedBitRate = 0;
                (*qosInfo)->dlMaxBitRate = 0;
            }
            else if (UmtsGetNodeType(node) == CELLULAR_SGSN ||
                     UmtsGetNodeType(node) == CELLULAR_GGSN)
            {
                (*qosInfo)->dlGuaranteedBitRate = 64000;
                (*qosInfo)->dlMaxBitRate = 64000;
                (*qosInfo)->ulGuaranteedBitRate = 0;
                (*qosInfo)->ulMaxBitRate = 0;
            }
            (*qosInfo)->transferDelay = 1 * MILLI_SECOND;
            (*qosInfo)->trafficClass = UMTS_QOS_CLASS_BACKGROUND;

            return FALSE;
        }
    }

    // app info found
    return TRUE;
}

// /**
// FUNCTION   :: UmtsLayer3ConvertTrafficClassToOctet
// LAYER      :: Layer 3
// PURPOSE    :: convert traffic class into octet
// PARAMETERS ::
// + trafficClass     : UmtsQoSTrafficClass  : class of this flow
// + tfCls            : unsigned Char*       : output Octet
// + handlingPrio     : unsigned char*       : handling priority
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConvertTrafficClassToOctet(
                  UmtsQoSTrafficClass trafficClass,
                  unsigned char* tfCls,
                  unsigned char* handlingPrio)
{
    if (trafficClass < UMTS_QOS_CLASS_BACKGROUND &&
        trafficClass > UMTS_QOS_CLASS_STREAMING)
    {
        *tfCls = 3;
        if (trafficClass == UMTS_QOS_CLASS_INTERACTIVE_3)
        {
            *handlingPrio = 3;
        }
        else if (trafficClass == UMTS_QOS_CLASS_INTERACTIVE_2)
        {
            *handlingPrio = 2;
        }
        else
        {
            *handlingPrio = 1;
        }
    }
    else if (trafficClass == UMTS_QOS_CLASS_BACKGROUND)
    {
        *tfCls = 4;
        *handlingPrio = 0;
    }
    else if (trafficClass == UMTS_QOS_CLASS_STREAMING)
    {
        *tfCls = 2;
        *handlingPrio = 0;
    }
    else if (trafficClass ==
             UMTS_QOS_CLASS_CONVERSATIONAL)
    {
        *tfCls = 1;
        *handlingPrio = 0;
    }
}

// /**
// FUNCTION   :: UmtsLayer3ConvertOctetToTrafficClass
// LAYER      :: Layer 3
// PURPOSE    :: convert octet into traffic class
// PARAMETERS ::
// + tfCls            : unsigned Char         : output Octet
// + handlingPrio     : unsigned char         : handling priority
// + trafficClass     : UmtsQoSTrafficClass*  : class of this flow
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConvertOctetToTrafficClass(
                  unsigned char tfCls,
                  unsigned char handlingPrio,
                  UmtsQoSTrafficClass* trafficClass)
{
    if (tfCls == 1)
    {
        *trafficClass = UMTS_QOS_CLASS_CONVERSATIONAL;
    }
    else if (tfCls == 2)
    {
        *trafficClass = UMTS_QOS_CLASS_STREAMING;
    }
    else if (tfCls == 4)
    {
        *trafficClass = UMTS_QOS_CLASS_BACKGROUND;
    }
    else if (tfCls == 3)
    {
        if (handlingPrio == 1)
        {
            *trafficClass = UMTS_QOS_CLASS_INTERACTIVE_1;
        }
        else if (handlingPrio == 2)
        {
            *trafficClass = UMTS_QOS_CLASS_INTERACTIVE_2;
        }
        else if (handlingPrio == 3)
        {
            *trafficClass = UMTS_QOS_CLASS_INTERACTIVE_3;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3ConvertMaxRateToOctet
// LAYER      :: Layer 3
// PURPOSE    :: convert max rate into octet
// PARAMETERS ::
// + maxRate       : unsigned int        : max rate
// + rate          : unsigned char*      : Octet
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConvertMaxRateToOctet(unsigned int maxRate,
                                     unsigned char* rate)
{
    int rateInKbps;
    int rateIn8Kbps;
    int rateIn64Kbps;
    if (maxRate <= 63000) // 63K
    {
        rateInKbps = (int)ceil((double)(maxRate) / 1000);
        *rate = (unsigned char)rateInKbps;
    }
    else if (maxRate > 63000 && maxRate <= 576000)
    {
        rateIn8Kbps = (int)(ceil(((double)(maxRate) - 64000) / 8000)) + 64;
        *rate = (unsigned char)rateIn8Kbps;
    }
    else if (maxRate > 576000 && maxRate < 8640000)
    {
        rateIn64Kbps = 
            (int)(ceil(((double)(maxRate) - 576000) / 64000)) + 128;
        *rate = (unsigned char)rateIn64Kbps;
    }
    else
    {
        // need to use extend filed
        *rate = 0xFE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3ConvertOctetToMaxRate
// LAYER      :: Layer 3
// PURPOSE    :: convert octet into max rate
// PARAMETERS ::
// + rate          : unsigned char      : Octet
// + maxRate       : unsigned int*       : max rate
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConvertOctetToMaxRate(unsigned char rate,
                                     unsigned int* maxRate)
{
    if (rate <= 63)
    {
        *maxRate = rate * 1000;
    }
    else if (rate > 63 && rate <=127)
    {
        *maxRate = 64000 + (rate - 64) *  8000;
    }
    else if (rate > 127 && rate < 255)
    {
        *maxRate = 576000 + (rate - 128) * 64000;
    }
}

// /**
// FUNCTION   :: UmtsLayer3ConvertDelayToOctet
// LAYER      :: Layer 3
// PURPOSE    :: convert delay into octet
// PARAMETERS ::
// + appLatency   : clocktype      : delay
// + dealy        : unsigned char* : octet
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConvertDelayToOctet(clocktype appLaterncy,
                                     unsigned char* delay)
{
    int delayIn10ms;
    int delayIn50ms;
    int delayIn100ms;
    if (appLaterncy <= 150 * MILLI_SECOND) // 63K
    {
        delayIn10ms = 
            (int)ceil((double)(appLaterncy) / (10 * MILLI_SECOND));
        *delay = (unsigned char)delayIn10ms;
    }
    else if (appLaterncy > 150 * MILLI_SECOND &&
             appLaterncy < 1000 * MILLI_SECOND)
    {
        delayIn50ms = 
            (int)ceil((double)(appLaterncy - 200 * MILLI_SECOND) /
            (50 * MILLI_SECOND)) + 16;
        *delay = (unsigned char)delayIn50ms;
    }
    else if (appLaterncy >= 1000 * MILLI_SECOND &&
             appLaterncy <= 4000 * MILLI_SECOND)
    {
        delayIn100ms = 
            (int)ceil((double)(appLaterncy - 1000 * MILLI_SECOND) /
             (100 * MILLI_SECOND)) + 32;
        *delay = (unsigned char)delayIn100ms;
    }
    else
    {
        // need to use extend filed
        *delay = 0xFE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3ConvertOctetToDelay
// LAYER      :: Layer 3
// PURPOSE    :: convert octet into delay
// PARAMETERS ::
// + dealy        : unsigned char   : octet
// + appLatency   : clocktype*      : delay
// RETURN     :: void : NULL
// **/
void UmtsLayer3ConvertOctetToDelay(unsigned char delay,
                                   clocktype* appLaterncy)
{
    if (delay <= 15)
    {
        *appLaterncy = delay * 15 * MILLI_SECOND;
    }
    else if (delay > 15 && delay < 32)
    {
        *appLaterncy = (200 + (delay - 16) * 50 )* MILLI_SECOND;
    }
    else if (delay >= 32 && delay < 255)
    {
        *appLaterncy = (1000 + (delay - 32) * 100) * MILLI_SECOND;
    }
}
