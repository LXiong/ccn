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
#include "partition.h"
#include "network_ip.h"

#include "cellular.h"
#include "mac_cellular.h"
#include "cellular_umts.h"
#include "layer2_umts.h"
#include "layer2_umts_pdcp.h"
#include "layer2_umts_rlc.h"
#include "layer2_umts_mac.h"

#define DEBUG_L2_GENERAL 0
#define DEBUG_L2_MAC 0
#define DEBUG_L2_RLC 0
#define DEBUG_L2_PDCP 0
#define DEBUG_L2_BMC 0
#define DEBUG_L2_TIMER 0
#define DEBUG_L2_SPECIFIC_NODE 0

// /**
// FUNCTION::   UmtsLayer2InitTimer
// LAYER::      UMTS LAYER2
// PURPOSE::    Initiate a timer message for RLC or MAC
// PARAMETERS::
// + node             :  Node*    : node pointer
// + interfaceIndex   :  UInt32   : the interface index
// + sublayerType     :  UmtsLayer2SublayerType : sublayer type MAC/RLC
// + timerType        :  unsigned int :   Timer type
// + infoLen          :  unsigned int : Length of additional info
// + info             :  char*    : Additional info data
// RETURN:: MESSAGE*  : Timer message
// **/
Message* UmtsLayer2InitTimer(Node* node,
                             UInt32 interfaceIndex,
                             UmtsLayer2SublayerType sublayerType,
                             UmtsLayer2TimerType timerType,
                             unsigned int infoLen,
                             char*   info)
{
    Message* timerMsg;
    UmtsLayer2TimerInfoHdr* timerInfo;

    // allocate the timer message and send out
    timerMsg = MESSAGE_Alloc(node,
                             MAC_LAYER,
                             MAC_PROTOCOL_CELLULAR,
                             MSG_MAC_TimerExpired);

    MESSAGE_SetInstanceId(timerMsg, (short)interfaceIndex);

    MESSAGE_InfoAlloc(
        node,
        timerMsg,
        sizeof(UmtsLayer2TimerInfoHdr) + infoLen);
    char* infoHead = MESSAGE_ReturnInfo(timerMsg);
    timerInfo = (UmtsLayer2TimerInfoHdr*) infoHead;

    timerInfo->subLayerType = sublayerType;
    timerInfo->timeType = timerType;
    timerInfo->infoLen = infoLen;
    if (infoLen > 0 && info)
    {
        memcpy(infoHead + sizeof(UmtsLayer2TimerInfoHdr),
               info,
               infoLen);
    }

    return timerMsg;
}

// /**
// FUNCTION   :: UmtsLayer2PrintRunTimeInfo
// LAYER      :: UMTS LAYER2
// PURPOSE    :: Print run time info UMTS layer2 at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32               : Interface index
// + sublayerType     : UmtsLayer2SublayerType : Pointer to node input.
// RETURN     :: void : NULL
// **/
void UmtsLayer2PrintRunTimeInfo(Node *node,
                                UInt32 interfaceIndex,
                                UmtsLayer2SublayerType sublayerType)
{
    char clockStr[MAX_STRING_LENGTH];
    char intfStr[MAX_STRING_LENGTH];
    char sublayerStr[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
    IO_ConvertIpAddressToString(
        MAPPING_GetInterfaceAddressForInterface(
            node,
            node->nodeId,
            interfaceIndex), intfStr);

    if (sublayerType == UMTS_LAYER2_SUBLAYER_MAC)
    {
        sprintf(sublayerStr,"%s","MAC");
    }
    else if (sublayerType == UMTS_LAYER2_SUBLAYER_RLC)
    {
        sprintf(sublayerStr,"%s","RLC");
    }
    else if (sublayerType == UMTS_LAYER2_SUBLAYER_BMC)
    {
        sprintf(sublayerStr,"%s","BMC");
    }
    else if (sublayerType == UMTS_LAYER2_SUBLAYER_PDCP)
    {
        sprintf(sublayerStr,"%s","PDCP");
    }
    else
    {
        sprintf(sublayerStr,"%s","GENERAL");
    }

    if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        printf("%s Node %d (UE) intf %s (%s): \n",
            clockStr, node->nodeId, intfStr, sublayerStr);
    }
    else if (UmtsGetNodeType(node) == CELLULAR_NODEB)
    {
        printf("%s Node %d (Node_B) intf %s (%s): \n",
            clockStr, node->nodeId, intfStr, sublayerStr);
    }
}

// /**
// FUNCTION   :: isNodeTypeUe
// LAYER      :: UMTS LAYER2
// PURPOSE    :: determine if it is a UE at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + iface            : UInt32            : Interface index
// RETURN     :: BOOL : TRUE if UE, FALSE if not
// **/
BOOL isNodeTypeUe(Node* node, UInt32 iface)
{
    return (UmtsGetNodeType(node) == CELLULAR_UE);
}

// /**
// FUNCTION   :: isNodeTypeNodeB
// LAYER      :: UMTS LAYER2
// PURPOSE    :: determine if it is a NodeB at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + iface            : UInt32            : Interface index
// RETURN     :: BOOL : TRUE if NodeB, FALSE if not
// **/
BOOL isNodeTypeNodeB(Node* node, UInt32 iface)
{
    return (UmtsGetNodeType(node) == CELLULAR_NODEB);
}

// /**
// FUNCTION   :: isNodeTypeRnc
// LAYER      :: UMTS LAYER2
// PURPOSE    :: determine if it is a RNC at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + iface            : UInt32            : Interface index
// RETURN     :: BOOL : TRUE if RNC, FALSE if not
// **/
BOOL isNodeTypeRnc(Node* node, UInt32 iface)
{
    return (UmtsGetNodeType(node) == CELLULAR_RNC);
}

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION::       UmtsLayer2UpperLayerHasPacketToSend
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by upper layers to request
//                  Layer2 to send SDU
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
//    + rbId:       radio bearer Id asscociated with this RLC entity
//    + msg:        The SDU message from upper layer
//    + ueId:       UE identifier, used to look up RLC entity at UTRAN side
// RETURN::         NULL
// **/
void  UmtsLayer2UpperLayerHasPacketToSend(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    Message* msg,
    NodeId ueId)
{
    UmtsRlcUpperLayerHasSduToSend(
        node,
        interfaceIndex,
        rbId,
        msg,
        ueId);
}



// /**
// FUNCTION   :: UmtsLayer2Init
// LAYER      :: UMTS LAYER2
// PURPOSE    :: Initialize UMTS layer2 at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void UmtsLayer2Init(Node *node,
                    UInt32 interfaceIndex,
                    const NodeInput* nodeInput)
{
    MacCellularData* macCellularData;
    CellularUmtsLayer2Data *layer2Data;

    if (DEBUG_L2_GENERAL)
    {
        UmtsLayer2PrintRunTimeInfo(node,
                                   interfaceIndex,
                                   UMTS_LAYER2_SUBLAYER_NONE);
        printf("UMTS L2 Init\n");
    }

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    layer2Data = (CellularUmtsLayer2Data *)
                  MEM_malloc(sizeof(CellularUmtsLayer2Data));
    ERROR_Assert(layer2Data != NULL, "Memory error!");
    memset(layer2Data, 0, sizeof(CellularUmtsLayer2Data));

    macCellularData->cellularUmtsL2Data = layer2Data;

    macCellularData->nodeType = UmtsGetNodeType(node);

    if (macCellularData->nodeType == CELLULAR_UE ||
        macCellularData->nodeType == CELLULAR_NODEB)
    {
        UmtsPdcpInit(node, interfaceIndex, nodeInput);

        // initialize RLC sublayer
        UmtsRlcInit(node, interfaceIndex, nodeInput);

        // initialize MAC sublayer
        UmtsMacInit(node, interfaceIndex, nodeInput);
    }
    else
    {
        ERROR_ReportWarning("Only UE & NODE_B have Uu interface\n");
    }
}

// /**
// FUNCTION   :: UmtsLayer2Finalize
// LAYER      :: UMTS LAYER2
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// RETURN     :: void : NULL
// **/
void UmtsLayer2Finalize(Node *node, UInt32 interfaceIndex)
{
    MacCellularData* macCellularData;

    if (DEBUG_L2_GENERAL)
    {
        UmtsLayer2PrintRunTimeInfo(node,
                                   interfaceIndex,
                                   UMTS_LAYER2_SUBLAYER_NONE);
        printf("UMTS L2 Finalize...\n");
    }

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;

    if (macCellularData->nodeType == CELLULAR_UE ||
        macCellularData->nodeType == CELLULAR_NODEB)
    {
        UmtsPdcpFinalize(node, interfaceIndex);

        // Finalize RLC sublayer
        UmtsRlcFinalize(node, interfaceIndex);

        // Finalize MAC sublayer
        UmtsMacFinalize(node, interfaceIndex);

    }
    else
    {
        ERROR_ReportWarning("Only UE & NODE_B have Uu interface\n");
    }

    MEM_free(macCellularData->cellularUmtsL2Data);
    macCellularData->cellularUmtsL2Data = NULL;
}

// /**
// FUNCTION   :: UmtsLayer2Layer
// LAYER      :: UMTS LAYER2
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void UmtsLayer2Layer(Node *node, UInt32 interfaceIndex, Message *msg)
{
    MacCellularData* macCellularData;

    if (DEBUG_L2_GENERAL)
    {
        UmtsLayer2PrintRunTimeInfo(node,
                                   interfaceIndex,
                                   UMTS_LAYER2_SUBLAYER_NONE);
        printf("UMTS L2 Layer Function...\n");
    }

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;

    switch (msg->eventType)
    {
        case MSG_MAC_TimerExpired:
        {
            UmtsLayer2TimerInfoHdr* timerInfo;
            timerInfo = (UmtsLayer2TimerInfoHdr*) MESSAGE_ReturnInfo(msg);
            if (DEBUG_L2_TIMER)
            {
                printf("UMTS L2: node%u intf%d get a timer message\n",
                       node->nodeId, interfaceIndex);
            }

            if (macCellularData->nodeType == CELLULAR_UE ||
                macCellularData->nodeType == CELLULAR_NODEB)
            {
                if (timerInfo->subLayerType == UMTS_LAYER2_SUBLAYER_PDCP)
                {
                    UmtsPdcpLayer(node, interfaceIndex, msg);
                }
                else if (timerInfo->subLayerType == UMTS_LAYER2_SUBLAYER_RLC)
                {
                    UmtsRlcLayer(node, interfaceIndex, msg);
                }
                else if(timerInfo->subLayerType == UMTS_LAYER2_SUBLAYER_MAC)
                {
                    UmtsMacLayer(node, interfaceIndex, msg);
                }
            }

            break;
        }
        case MSG_MAC_UMTS_LAYER2_HandoffPacket:
        {
            MAC_HandOffSuccessfullyReceivedPacket(
                                    node,
                                    interfaceIndex,
                                    msg,
                                    ANY_ADDRESS);
            break;
        }

        default:
        {
            char tmpString[MAX_STRING_LENGTH];

            sprintf(tmpString,
                    "UMTS L2 node%d: Unknow message event type %d\n",
                    node->nodeId, msg->eventType);
            ERROR_ReportError(tmpString);
            MESSAGE_Free(node, msg);

            break;
        }
    }
}
