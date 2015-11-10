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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"

#include "cellular.h"
#include "cellular_layer3.h"

#ifdef CELLULAR_LIB
// Abstract Cellular
#include "cellular_abstract.h"
#include "cellular_abstract_layer3.h"
#endif

#ifdef UMTS_LIB
// UMTS
#include "cellular_umts.h"
#include "layer3_umts.h"
#include "layer3_umts_gsn.h"
#endif

#ifdef MUOS_LIB
// MUOS
#include "cellular_muos.h"
#include "layer3_muos.h"
#include "layer3_muos_gsn.h"
#endif


#define  DEBUG 0

// /**
// FUNCTION   :: CellularLayer3PreInit
// LAYER      :: Layer3
// PURPOSE    :: Preinitialize Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularLayer3PreInit(Node *node, const NodeInput *nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    CellularLayer3Data *nwCellularData;

    if (DEBUG)
    {
        printf("node %d: Cellular PreInit\n", node->nodeId);
    }

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "CELLULAR-LAYER3-PROTOCOL",
        &retVal,
        buf);
    if (retVal == FALSE)
    {
        ERROR_ReportError("CELLULAR-LAYER3-PROTOCOL parameter"
            " was not found for node");
    }

    nwCellularData =
        (CellularLayer3Data *)node->networkData.cellularLayer3Var;
    if (strcmp(buf, "ABSTRACT-LAYER3") == 0)
    {
#ifdef CELLULAR_LIB
        if (DEBUG)
        {
            printf("node %d: Calling Abstract Cellular Preinit\n",
                node->nodeId);
        }
        nwCellularData->cellularLayer3ProtocolType =
            Cellular_ABSTRACT_Layer3;

        CellularAbstractLayer3PreInit(node,nodeInput);
#else // CELLULAR_LIB
        ERROR_ReportMissingLibrary("ABSTRACT-LAYER3", "CELLULAR");
#endif // CELLULAR_LIB

    }
    else if (strcmp(buf, "GSM-LAYER3") == 0)
    {
        if (DEBUG)
        {
            printf("node %d: GSM Cellular Preinit\n",node->nodeId);
        }
    }
    else if (strcmp(buf, "GPRS") == 0)
    {
        if (DEBUG)
        {
            printf("node %d: GPRS Cellular Preinit\n",node->nodeId);
        }
    }
    else if (strcmp(buf, "UMTS-LAYER3") == 0)
    {
#ifdef UMTS_LIB
        if (DEBUG)
        {
            printf("node %d: UMTS Cellular Preinit\n",node->nodeId);
        }
        nwCellularData->cellularLayer3ProtocolType =
                         Cellular_UMTS_Layer3;

        UmtsLayer3PreInit(node, nodeInput);
#else // UMTS_LIB
        ERROR_ReportMissingLibrary("UMTS-LAYER3", "UMTS");
#endif // UMTS_LIB
    }
    else if (strcmp(buf, "MUOS-LAYER3") == 0)
    {
#ifdef MUOS_LIB
        if (DEBUG)
        {
            printf("node %d: MUOS Cellular Preinit\n",node->nodeId);
        }
        nwCellularData->cellularLayer3ProtocolType =
                         Cellular_MUOS_Layer3;

        MuosLayer3PreInit(node, nodeInput);
#else // MUOS_LIB
        ERROR_ReportMissingLibrary("MUOS-LAYER3", "MUOS");
#endif // MUOS_LIB
    }
    else if (strcmp(buf, "CDMA2000-LAYER3") == 0)
    {
        if (DEBUG)
        {
            printf("node %d: CDMA2000 Cellular Preinit\n",node->nodeId);
        }
    }
    else
    {
        ERROR_ReportError(
            "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
            " GSM-LAYER3, GPRS-LAYER3, UMTS-LAYER3, CDMA2000-LAYER3");
    }
}

// /**
// FUNCTION   :: CellularLayer3Init
// LAYER      :: Layer3
// PURPOSE    :: Initialize Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularLayer3Init(Node *node, const NodeInput *nodeInput)
{
    if (DEBUG)
    {
        printf("node %d: Cellular Init\n",node->nodeId);
    }
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    switch (node->networkData.cellularLayer3Var->cellularLayer3ProtocolType)
    {
        case Cellular_ABSTRACT_Layer3:
        {
#ifdef CELLULAR_LIB
            CellularAbstractLayer3Init(node, nodeInput);
            break;
#else // CELLULAR_LIB
        ERROR_ReportMissingLibrary("ABSTRACT-LAYER3", "CELLULAR");
#endif // CELLULAR_LIB
        }
        case Cellular_GSM_Layer3:
        {
            break;
        }
        case Cellular_GPRS_Layer3:
        {
            break;
        }
        case Cellular_UMTS_Layer3:
        {
#ifdef UMTS_LIB
            UmtsLayer3Init(node, nodeInput);
            break;
#else // UMTS_LIB
            ERROR_ReportMissingLibrary("UMTS-LAYER3", "UMTS");
#endif // UMTS_LIB
        }
        case Cellular_MUOS_Layer3:
        {
#ifdef MUOS_LIB
            MuosLayer3Init(node, nodeInput);
            break;
#else // MUOS_LIB
            ERROR_ReportMissingLibrary("MUOS-LAYER3", "MUOS");
#endif // MUOS_LIB
        }
        case Cellular_CDMA2000_Layer3:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
                " GSM-LAYER3, GPRS-LAYER3, UMTS-LAYER3, CDMA2000-LAYER3");
        }
    }
    IO_ReadString(
        node->nodeId,
        ANY_DEST,
        nodeInput,
        "CELLULAR-STATISTICS",
        &retVal,
        buf);

    if ((retVal == FALSE) || (strcmp(buf, "YES") == 0))
    {
        node->networkData.cellularLayer3Var->collectStatistics = TRUE;
    }
    else if (retVal && strcmp(buf, "NO") == 0)
    {
        node->networkData.cellularLayer3Var->collectStatistics = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Value of CELLULAR-STATISTICS should be YES or NO! Default value"
            "YES is used.");
        node->networkData.cellularLayer3Var->collectStatistics = TRUE;
    }
}
// /**
// FUNCTION   :: CellularLayer3Finalize
// LAYER      :: Layer3
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularLayer3Finalize(Node *node)
{
    if (DEBUG)
    {
        printf("node %d: Cellular Layer3 Finalize\n",node->nodeId);
    }
    switch(node->networkData.cellularLayer3Var->cellularLayer3ProtocolType)
    {
        case Cellular_ABSTRACT_Layer3:
        {
#ifdef CELLULAR_LIB
            CellularAbstractLayer3Finalize(node);
            break;
#else
            ERROR_ReportMissingLibrary("ABSTRACT-LAYER3", "CELLULAR");
#endif
        }
        case Cellular_GSM_Layer3:
        {
            break;
        }
        case Cellular_GPRS_Layer3:
        {
            break;
        }
        case Cellular_UMTS_Layer3:
        {
#ifdef UMTS_LIB
            UmtsLayer3Finalize(node);
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER3", "UMTS");
#endif
        }
        case Cellular_MUOS_Layer3:
        {
#ifdef MUOS_LIB
            MuosLayer3Finalize(node);
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER3", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Layer3:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
                " GSM-LAYER3, GPRS-LAYER3, UMTS-LAYER3, CDMA2000-LAYER3");
        }
    }
}

// /**
// FUNCTION   :: CellularLayer3Layer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void CellularLayer3Layer(Node *node, Message *msg)
{
    if (DEBUG)
    {
        printf("node %d NW: Cellualr Event Process\n",node->nodeId);
    }
    switch(node->networkData.cellularLayer3Var->cellularLayer3ProtocolType)
    {
        case Cellular_ABSTRACT_Layer3:
        {
#ifdef CELLULAR_LIB
            CellularAbstractLayer3Layer(node, msg);
            break;
#else
            ERROR_ReportMissingLibrary("ABSTRACT-LAYER3", "CELLULAR");
#endif
        }
        case Cellular_GSM_Layer3:
        {
            break;
        }
        case Cellular_GPRS_Layer3:
        {
            break;
        }
        case Cellular_UMTS_Layer3:
        {
#ifdef UMTS_LIB
            UmtsLayer3Layer(node, msg);
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER3", "UMTS");
#endif // UMTS_LIB
        }
        case Cellular_MUOS_Layer3:
        {
#ifdef MUOS_LIB
            MuosLayer3Layer(node, msg);
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER3", "MUOS");
#endif // MUOS_LIB
        }
        case Cellular_CDMA2000_Layer3:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
                " GSM-LAYER3, GPRS-LAYER3, UMTS-LAYER3, CDMA2000-LAYER3");
        }
    }
}
// /**
// FUNCTION   :: CellularLayer3ReceivePacketOverIp
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + msg              : Message* : Message for node to interpret
// + interfaceIndex   : int      : Interface from which packet was received
// + sourceAddress    : Address  : Source node address
// RETURN     :: void : NULL
// **/
void CellularLayer3ReceivePacketOverIp(Node *node,
                                       Message *msg,
                                       int interfaceIndex,
                                       Address sourceAddress)
{
    if (DEBUG)
    {
        printf(
            "node %d NW: Cellualr receive packet over IP \n",
            node->nodeId);
    }
    switch(node->networkData.cellularLayer3Var->cellularLayer3ProtocolType)
    {
        case Cellular_ABSTRACT_Layer3:
        {
#ifdef CELLULAR_LIB
            CellularAbstractLayer3ReceivePacketOverIp(
                node,
                msg,
                sourceAddress);
            break;
#else
            ERROR_ReportMissingLibrary("ABSTRACT-LAYER3", "CELLULAR");
#endif
        }
        case Cellular_GSM_Layer3:
        {
            break;
        }
        case Cellular_GPRS_Layer3:
        {
            break;
        }
        case Cellular_UMTS_Layer3:
        {
#ifdef UMTS_LIB
            UmtsLayer3ReceivePacketOverIp(
                node,
                msg,
                interfaceIndex,
                sourceAddress);
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER3", "UMTS");
#endif

        }
        case Cellular_MUOS_Layer3:
        {
#ifdef MUOS_LIB
            MuosLayer3ReceivePacketOverIp(
                node,
                msg,
                interfaceIndex,
                sourceAddress);
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER3", "MUOS");
#endif

        }
        case Cellular_CDMA2000_Layer3:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
                " GSM-LAYER3, GPRS-LAYER3, UMTS-LAYER3, CDMA2000-LAYER3");
        }
    }
}

// /**
// FUNCTION   :: CellularLayer3ReceivePacketFromMacLayer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// + lastHopAddress   : NodeAddress       : Address of the last hop
// + interfaceIndex   : int               : Interface from which the packet is received
// RETURN     :: void : NULL
// **/
void CellularLayer3ReceivePacketFromMacLayer(Node *node,
                                             Message *msg,
                                             NodeAddress lastHopAddress,
                                             int interfaceIndex)
{
    switch(node->networkData.cellularLayer3Var->cellularLayer3ProtocolType)
    {
        case Cellular_ABSTRACT_Layer3:
        {
#ifdef CELLULAR_LIB
            if (node->macData[interfaceIndex]->macProtocol
                == MAC_PROTOCOL_CELLULAR)
            {
                CellularAbstractReceivePacketFromMacLayer(
                    node,
                    msg,
                    lastHopAddress,
                    interfaceIndex);
            }
            else
            {

            }
            break;
#else
            ERROR_ReportMissingLibrary("ABSTRACT-LAYER3", "CELLULAR");
#endif
        }
        case Cellular_GSM_Layer3:
        {
            break;
        }
        case Cellular_GPRS_Layer3:
        {
            break;
        }
        case Cellular_UMTS_Layer3:
        {
#ifdef UMTS_LIB
            UmtsLayer3ReceivePacketFromMacLayer(
                node,
                msg,
                lastHopAddress,
                interfaceIndex);
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER3", "UMTS");
#endif
        }
        case Cellular_MUOS_Layer3:
        {
#ifdef MUOS_LIB
            MuosLayer3ReceivePacketFromMacLayer(
                node,
                msg,
                lastHopAddress,
                interfaceIndex);
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER3", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Layer3:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
                " GSM-LAYER3, GPRS-LAYER3, UMTS-LAYER3, CDMA2000-LAYER3");
        }
    }
}

// /**
// FUNCTION   :: CellularLayer3IsUserDevices
// LAYER      :: Layer3
// PURPOSE    :: TO see if it is a user devices ranther than fixed nodes
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface from which the packet is received
// RETURN     :: void : NULL
// **/
BOOL CellularLayer3IsUserDevices(Node *node, int interfaceIndex)
{
    BOOL isUserDevices = FALSE;
    switch(node->networkData.cellularLayer3Var->cellularLayer3ProtocolType)
    {
        case Cellular_ABSTRACT_Layer3:
        {
            break;
        }
        case Cellular_GSM_Layer3:
        {
            break;
        }
        case Cellular_GPRS_Layer3:
        {
            break;
        }
        case Cellular_UMTS_Layer3:
        {
#ifdef UMTS_LIB
            if (UmtsGetNodeType(node) == CELLULAR_UE)
            {
                isUserDevices = TRUE;
            }
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER3", "UMTS");
#endif
        }
        case Cellular_MUOS_Layer3:
        {
#ifdef MUOS_LIB
            if (MuosGetNodeType(node) == CELLULAR_UE)
            {
                isUserDevices = TRUE;
            }
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER3", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Layer3:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
                " GSM-LAYER3, GPRS-LAYER3, UMTS-LAYER3, CDMA2000-LAYER3");
        }
    }
    return isUserDevices;
}

// /**
// FUNCTION   :: CellularLayer3IsPacketGateway
// LAYER      :: Layer3
// PURPOSE    :: To check if the node is a packet gateway which connects
//               cellular network with packet data network (PDN).
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// RETURN     :: void  : NULL
// **/
BOOL CellularLayer3IsPacketGateway(Node *node)
{
    BOOL isPktGateway = FALSE;

    switch(node->networkData.cellularLayer3Var->cellularLayer3ProtocolType)
    {
        case Cellular_ABSTRACT_Layer3:
        {
            break;
        }
        case Cellular_GSM_Layer3:
        {
            break;
        }
        case Cellular_UMTS_Layer3:
        {
#ifdef UMTS_LIB
            if (UmtsGetNodeType(node) == CELLULAR_GGSN)
            {
                isPktGateway = TRUE;
            }
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER3", "UMTS");
#endif
        }
        case Cellular_MUOS_Layer3:
        {
#ifdef MUOS_LIB
            if (MuosGetNodeType(node) == CELLULAR_GGSN)
            {
                isPktGateway = TRUE;
            }
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER3", "MUOS");
#endif
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
                " GSM-LAYER3, UMTS-LAYER3");
        }
    }
    return isPktGateway;
}

// /**
// FUNCTION   :: CellularLayer3IsPacketForMyPlmn
// LAYER      :: Layer3
// PURPOSE    :: Check if destination of the packet is in my PLMN
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Packet to be checked
// + inIfIndex : int      : Interface from which the packet is received
// + network   : int      : network type. IPv4 or IPv6
// RETURN     :: BOOL     : TRUE if pkt for my PLMN, FALSE otherwise
// **/
BOOL CellularLayer3IsPacketForMyPlmn(
         Node* node,
         Message* msg,
         int inIfIndex,
         int networkType)
{
    BOOL isMyPkt = FALSE;

    switch (node->networkData.cellularLayer3Var->cellularLayer3ProtocolType)
    {
        case Cellular_ABSTRACT_Layer3:
        {
            break;
        }
        case Cellular_GSM_Layer3:
        {
            break;
        }
        case Cellular_UMTS_Layer3:
        {
#ifdef UMTS_LIB
            if (UmtsGetNodeType(node) == CELLULAR_GGSN)
            {
                isMyPkt = UmtsLayer3GgsnIsPacketForMyPlmn(node,
                                                          msg,
                                                          inIfIndex,
                                                          networkType);
            }
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER3", "UMTS");
#endif
        }
        case Cellular_MUOS_Layer3:
        {
#ifdef MUOS_LIB
            if (MuosGetNodeType(node) == CELLULAR_GGSN)
            {
                isMyPkt = MuosLayer3GgsnIsPacketForMyPlmn(node,
                                                          msg,
                                                          inIfIndex,
                                                          networkType);
            }
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER3", "MUOS");
#endif
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
                "GSM-LAYER3, UMTS-LAYER3");
        }
    }
    return isMyPkt;
}

// /**
// FUNCTION   :: CellularLayer3HandlePacketFromUpperOrOutside
// LAYER      :: Layer3
// PURPOSE    :: handle packet from upper
// PARAMETERS ::
// + node           : Node*    : Pointer to node.
// + msg            : Message* : Message to be sent onver the air interface
// + interfaceIndex : int      : Interface from which the packet is received
// + network        : int      : network type. IPv4 or IPv6
// RETURN     :: void          : NULL
// **/
void CellularLayer3HandlePacketFromUpperOrOutside(
         Node* node,
         Message* msg,
         int interfaceIndex,
         int networkType)
{

    switch(node->networkData.cellularLayer3Var->cellularLayer3ProtocolType)
    {
        case Cellular_ABSTRACT_Layer3:
        {
            break;
        }
        case Cellular_GSM_Layer3:
        {
            break;
        }
        case Cellular_GPRS_Layer3:
        {
            break;
        }
        case Cellular_UMTS_Layer3:
        {
#ifdef UMTS_LIB
            UmtsLayer3HandlePacketFromUpperOrOutside(
                node,
                msg,
                interfaceIndex,
                networkType);

            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER3", "CELLULAR");
#endif
        }
        case Cellular_MUOS_Layer3:
        {
#ifdef MUOS_LIB
            MuosLayer3HandlePacketFromUpperOrOutside(
                node,
                msg,
                interfaceIndex,
                networkType);

            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER3", "CELLULAR");
#endif
        }
        case Cellular_CDMA2000_Layer3:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-LAYER3-PROTOCOL parameter must be ABSTRACT-LAYER3,"
                " GSM-LAYER3, GPRS-LAYER3, UMTS-LAYER3 or MUOS-LAYER3");
        }
    }
}
