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

#ifndef CELLUAR_UMTS_LAYER2_PDCP_H
#define CELLUAR_UMTS_LAYER2_PDCP_H

// /**
// STRUCT:: UmtsPdcpStats
// DESCRIPTION:: PDCP statistics
// **/ 
typedef struct 
{
    unsigned int numPktsFromUpperLayer;
    unsigned int numPktsToUpperLayer;
    unsigned int numPktsToRlc;
    unsigned int numPktsFromRlc;    
} UmtsPdcpStats;

// /**
// STRUCT:: UmtsPdcpData
// DESCRIPTION:: PDCP sublayer structure
// **/ 
typedef struct
{
    // more goes here
    UmtsPdcpStats stats;
} UmtsPdcpData;

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
                  const NodeInput* nodeInput);

// /**
// FUNCTION::       UmtsPdcpFinalize
// LAYER::          UMTS Layer2 Pdcp 
// PURPOSE::        Pdcp finalization function
// PARAMETERS::     
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
// RETURN::         NULL
// **/
void UmtsPdcpFinalize(Node* node, unsigned int interfaceIndex);

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
                   Message* message);


#endif //CELLUAR_UMTS_LAYER2_PDCP_H

