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
// PACKAGE     :: CELLULAR_MAC
// DESCRIPTION :: Defines the data structures used in the Cellular MAC
//                Most of the time the data structures and functions start
//                with MacCellular**
// **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"

#include "cellular.h"
#include "mac_cellular.h"

#ifdef CELLULAR_LIB
//Abstract
#include "cellular_abstract.h"
#include "mac_cellular_abstract.h"
#endif

#ifdef UMTS_LIB
// UMTS
#include "cellular_umts.h"
#include "layer2_umts.h"
#include "layer2_umts_mac_phy.h"
#endif

#ifdef MUOS_LIB
// MUOS
#include "cellular_muos.h"
#include "layer2_muos.h"
#include "layer2_muos_mac_phy.h"
#endif

#define DEBUG 0

// /**
// FUNCTION   :: MacCellularInit
// LAYER      :: MAC
// PURPOSE    :: Initialize Cellular MAC protocol at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacCellularInit(Node *node,
                     int interfaceIndex,
                     const NodeInput* nodeInput)
{
    MacCellularData *macCellularData;
    char errorString[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    macCellularData = (MacCellularData *)
                      MEM_malloc(sizeof(MacCellularData));
    memset((char*) macCellularData, 0, sizeof(MacCellularData));

    macCellularData->myMacData = node->macData[interfaceIndex];
    macCellularData->myMacData->macVar = (void *) macCellularData;

    // generate initial seed
    RANDOM_SetSeed(macCellularData->randSeed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_CELLULAR,
                   interfaceIndex);

    // read some configuration parameters

    // read the type of cellular MAC
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "CELLULAR-MAC-PROTOCOL",
                  &retVal,
                  buf);

    if (retVal)
    {
        if (strcmp(buf, "ABSTRACT-MAC") == 0)
        {
#ifdef CELLULAR_LIB
            if (DEBUG)
            {
                printf("Node %d: running Abstract Cellular MAC\n",
                       node->nodeId);
            }

            macCellularData->cellularMACProtocolType = Cellular_ABSTRACT_Layer2;
            MacCellularAbstractInit(node, interfaceIndex, nodeInput);
#else
            ERROR_ReportMissingLibrary("ABSTRACT-LAYER2", "CELLULAR");
#endif
        }
        else if (strcmp(buf, "GSM-LAYER2") == 0)
        {
            if (DEBUG)
            {
                printf("Node %d: running GSM Cellular Layer2\n",
                       node->nodeId);
            }

            macCellularData->cellularMACProtocolType = Cellular_GSM_Layer2;
        }
        else if (strcmp(buf, "GPRS-LAYER2") == 0)
        {
            if (DEBUG)
            {
                printf("Node %d: running GPRS Cellular Layer2\n",
                       node->nodeId);
            }

            macCellularData->cellularMACProtocolType = Cellular_GPRS_Layer2;
        }
        else if (strcmp(buf, "UMTS-LAYER2") == 0)
        {
#ifdef UMTS_LIB
            if (DEBUG)
            {
                printf("Node %d: running UMTS Cellular Layer2\n",
                       node->nodeId);
            }

            macCellularData->cellularMACProtocolType = Cellular_UMTS_Layer2;
            UmtsLayer2Init(node,
                           (unsigned int)interfaceIndex,
                           nodeInput);
#else
           ERROR_ReportMissingLibrary("UMTS-LAYER2", "UMTS");
#endif
        }
        else if (strcmp(buf, "MUOS-LAYER2") == 0)
        {
#ifdef MUOS_LIB
            if (DEBUG)
            {
                printf("Node %d: running MUOS Cellular Layer2\n",
                       node->nodeId);
            }

            macCellularData->cellularMACProtocolType = Cellular_MUOS_Layer2;
            MuosLayer2Init(node,
                           (unsigned int)interfaceIndex,
                           nodeInput);
#else
           ERROR_ReportMissingLibrary("MUOS-LAYER2", "MUOS");
#endif
        }
        else if (strcmp(buf, "CDMA2000-LAYER2") == 0)
        {
            if (DEBUG)
            {
                printf("Node %d: running CDMA2000 Cellular Layer2\n",
                       node->nodeId);
            }

            macCellularData->cellularMACProtocolType = Cellular_CDMA2000_Layer2;
        }
        else
        {
            sprintf(errorString,
                    "Node%d unknown cellular MAC protocol\n",
                    node->nodeId);
            ERROR_ReportError(errorString);
        }
    }
    else
    {
        sprintf(errorString,
                "Node%d parameter CELLULAR-MAC-PROTOCOL is not specified!",
                node->nodeId);
        ERROR_ReportError(errorString);
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
        macCellularData->collectStatistics = TRUE;
    }
    else if (retVal && strcmp(buf, "NO") == 0)
    {
        macCellularData->collectStatistics = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Value of CELLULAR-STATISTICS should be YES or NO! Default value"
            "YES is used.");
        macCellularData->collectStatistics = TRUE;
    }
}

// /**
// FUNCTION   :: MacCellularFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacCellularFinalize(Node *node,int interfaceIndex)
{
    MacCellularData* macCellularData;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;

    switch(macCellularData->cellularMACProtocolType)
    {
        case Cellular_ABSTRACT_Layer2:
        {
#ifdef CELLULAR_LIB
            MacCellularAbstractFinalize(node, interfaceIndex);
            break;
#else
            ERROR_ReportMissingLibrary("ABSTRACT-LAYER2", "CELLULAR");
#endif
        }

        case Cellular_GSM_Layer2:
        {
            break;
        }

        case Cellular_GPRS_Layer2:
        {
            break;
        }

        case Cellular_UMTS_Layer2:
        {
#ifdef UMTS_LIB
            UmtsLayer2Finalize(node, (unsigned int)interfaceIndex);
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER2", "UMTS");
#endif
        }
        case Cellular_MUOS_Layer2:
        {
#ifdef MUOS_LIB
            MuosLayer2Finalize(node, (unsigned int)interfaceIndex);
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER2", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Layer2:
        {
            break;
        }

        default:
        {
            break;
        }
    }

    MEM_free(macCellularData);
    node->macData[interfaceIndex]->macVar = NULL;
}

// /**
// FUNCTION   :: MacCellularLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacCellularLayer(Node *node, int interfaceIndex, Message *msg)
{
    MacCellularData* macCellularData;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;

    switch(macCellularData->cellularMACProtocolType)
    {
        case Cellular_ABSTRACT_Layer2:
        {
#ifdef CELLULAR_LIB
            MacCellularAbstractLayer(node, interfaceIndex, msg);
            break;
#else
            ERROR_ReportMissingLibrary("ABSTRACT-LAYER2", "CELLULAR");
#endif
        }

        case Cellular_GSM_Layer2:
        {
            break;
        }

        case Cellular_GPRS_Layer2:
        {
            break;
        }

        case Cellular_UMTS_Layer2:
        {
#ifdef UMTS_LIB
            UmtsLayer2Layer(node,
                            (unsigned int)interfaceIndex,
                            msg);
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER2", "UMTS");
#endif
        }
       case Cellular_MUOS_Layer2:
        {
#ifdef MUOS_LIB
            MuosLayer2Layer(node,
                            (unsigned int)interfaceIndex,
                            msg);
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER2", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Layer2:
        {
            break;
        }

        default:
        {
            break;
        }
    }
}

// /**
// FUNCTION   :: MacCellularNetworkLayerHasPacketToSend
// LAYER      :: MAC
// PURPOSE    :: Called when network layer buffers transition from empty.
//               This function is not used in the cellular model
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface running this MAC
// + cellularMac      : MacCellularData*  : Pointer to cellular MAC structure
// RETURN     :: void : NULL
// **/
void MacCellularNetworkLayerHasPacketToSend(Node *node,
                                            int interfaceIndex,
                                            MacCellularData *cellularMac)
{
    MacCellularData* macCellularData;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;

    switch(macCellularData->cellularMACProtocolType)
    {
        case Cellular_ABSTRACT_Layer2:
        {
#ifdef CELLULAR_LIB
            MacCellularAbstractNetworkLayerHasPacketToSend(
                node,
                interfaceIndex,
                cellularMac->macCellularAbstractData);
            break;
#else
            ERROR_ReportMissingLibrary("ABSTRACT-LAYER2", "CELLULAR");
#endif
        }

        case Cellular_GSM_Layer2:
        {
            break;
        }

        case Cellular_GPRS_Layer2:
        {
            break;
        }

        case Cellular_UMTS_Layer2:
        {
#ifdef UMTS_LIB
            /*CellularUmtsLayer2NetworkLayerHasPacketToSend(
                            node,
                            interfaceIndex,
                cellularMac->macCellularAbstractData);*/
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER2", "UMTS");
#endif
        }
        case Cellular_MUOS_Layer2:
        {
#ifdef MUOS_LIB

            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER2", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Layer2:
        {
            break;
        }

        default:
        {
            break;
        }
    }
}

// /**
// FUNCTION   :: MacCellularReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface running this MAC
// + msg              : Message*          : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacCellularReceivePacketFromPhy(Node *node,
                                     int interfaceIndex,
                                     Message *msg)
{
    MacCellularData* macCellularData;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;

    switch (macCellularData->cellularMACProtocolType)
    {
        case Cellular_ABSTRACT_Layer2:
        {
#ifdef CELLULAR_LIB
            MacCellularAbstractReceivePacketFromPhy(
                node,
                interfaceIndex,
                msg);
            break;
#else
            ERROR_ReportMissingLibrary("ABSTRACT-LAYER2", "CELLULAR");
#endif
        }

        case Cellular_GSM_Layer2:
        {
            break;
        }

        case Cellular_GPRS_Layer2:
        {
            break;
        }

        case Cellular_UMTS_Layer2:
        {
#ifdef UMTS_LIB
            UmtsMacReceivePacketFromPhy(
                            node,
                            interfaceIndex,
                            msg);
            break;
#else
            ERROR_ReportMissingLibrary("UMTS-LAYER2", "UMTS");
#endif
        }
        case Cellular_MUOS_Layer2:
        {
#ifdef MUOS_LIB
            MuosMacReceivePacketFromPhy(
                            node,
                            interfaceIndex,
                            msg);
            break;
#else
            ERROR_ReportMissingLibrary("MUOS-LAYER2", "MUOS");
#endif
        }

        case Cellular_CDMA2000_Layer2:
        {
            break;
        }

        default:
        {
            break;
        }
    }
}
