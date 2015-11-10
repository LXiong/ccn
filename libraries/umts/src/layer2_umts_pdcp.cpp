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

#include "api.h"
#include "mac.h"
#include "cellular.h"
#include "mac_cellular.h"
#include "layer2_umts.h"
#include "layer2_umts_pdcp.h"

// /**
// FUNCTION::       UmtsPdcpPrintStat
// LAYER::          UMTS Layer2 Pdcp 
// PURPOSE::        Print Pdcp statistics
// PARAMETERS::     
//    + node:       pointer to the network node
//    + iface:      interface index
//    + PdcpData:    Pdcp sublayer data structure
// RETURN::         NULL
// **/
static
void UmtsPdcpPrintStat(
        Node* node, 
        int iface,
        UmtsPdcpData* PdcpData)
{
    //char buf[MAX_STRING_LENGTH];

// disabled, PDCP is not implemented in this release
#if 0
    sprintf(buf, "Number of data packets received from upper layers = %d",
        PdcpData->stats.numPktsFromUpperLayer);      
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS PDCP",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of data packets sent to upper layers = %d",
        PdcpData->stats.numPktsToUpperLayer);      
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS PDCP",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of data packets sent to RLC sublayer = %d",
        PdcpData->stats.numPktsToRlc);      
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS PDCP",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of data packets received from sublayer = %d",
        PdcpData->stats.numPktsFromRlc);      
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS PDCP",
                 ANY_DEST,
                 iface,
                 buf);
#endif
}

// /**
// FUNCTION::       UmtsPdcpInit
// LAYER::          UMTS Layer2 Pdcp 
// PURPOSE::        Pdcp Initalization 
// 
// PARAMETERS::     
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + nodeInput:  Input from configuration file             
// RETURN::         NULL
// **/
void UmtsPdcpInit(Node* node, 
                  unsigned int interfaceIndex, 
                  const NodeInput* nodeInput)
{
    MacCellularData* macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    CellularUmtsLayer2Data* layer2Data = 
        macCellularData->cellularUmtsL2Data;
    UmtsPdcpData* pdcpData; 
    
    pdcpData = (UmtsPdcpData*) MEM_malloc(sizeof(UmtsPdcpData));
    memset(pdcpData, 0, sizeof(UmtsPdcpData));
    layer2Data->umtsPdcpData  = pdcpData;
}

// /**
// FUNCTION::       UmtsPdcpFinalize
// LAYER::          UMTS Layer2 Pdcp 
// PURPOSE::        Pdcp finalization function
// PARAMETERS::     
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
// RETURN::         NULL
// **/
void UmtsPdcpFinalize(Node* node, unsigned int interfaceIndex)
{
    MacCellularData* macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    CellularUmtsLayer2Data* layer2Data = 
        macCellularData->cellularUmtsL2Data;
    UmtsPdcpData* pdcpData = layer2Data->umtsPdcpData; 

    if (macCellularData->collectStatistics ||
        macCellularData->myMacData->macStats)
    {
        UmtsPdcpPrintStat(node, (int)interfaceIndex, pdcpData);
    }

    MEM_free(layer2Data->umtsPdcpData);
    layer2Data->umtsPdcpData = NULL;
}

// /**
// FUNCTION::       UmtsPdcpLayer
// LAYER::          UMTS Layer2 Pdcp 
// PURPOSE::        Pdcp event handling function
// PARAMETERS::     
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + message     Message to be handled              
// RETURN::         NULL
// **/
void UmtsPdcpLayer(Node* node, 
                   unsigned int interfaceIndex, 
                   Message* msg)
{
#if 0
    MacCellularData* macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    CellularUmtsLayer2Data* layer2Data = 
        macCellularData->cellularUmtsL2Data;
    UmtsPdcpData* pdcpData = layer2Data->umtsPdcpData; 

#endif

    switch (msg->eventType)
    {
        case MSG_MAC_TimerExpired:
        {
            break;
        }
        default:
        {
            // default
        }
    }
}


