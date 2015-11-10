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
#include <iostream>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"

#include "cellular.h"
#include "cellular_layer3.h"
#include "cellular_umts.h"
#include "layer3_umts.h"
#include "layer3_umts_nodeb.h"
#include "layer2_umts.h"
#include "layer2_umts_rlc.h"
#include "layer2_umts_mac.h"
#include "layer2_umts_mac_phy.h"
#include "umts_constants.h"

#define DEBUG_SPECIFIED_UE_ID 0 // DEFAULT shoud be 0
#define DEBUG_SINGLE_UE  (ueId == DEBUG_SPECIFIED_UE_ID)
#define DEBUG_SPECIFIED_NODE_ID 0 // DEFAULT shoud be 0
#define DEBUG_SINGLE_NODE   (node->nodeId == DEBUG_SPECIFIED_NODE_ID)
#define DEBUG_PARAMETER    (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_BACKBONE     (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RRC_RB       (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RRC_TRCH     (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RRC_RL       (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_MEASUREMENT  (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RR           (0 || (0 &&  DEBUG_SINGLE_UE))
#define DEBUG_TIMER        (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_NBAP         (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_HO           (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RRC_RL_HSDPA 0

// /**
// FUNCTION   :: UmtsLayer3NodeBConfigCommonTrCh
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the common Transport Channels
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodeBConfigCommonTrCh(Node *node,
                                     UmtsLayer3Data *umtsL3)
{
    // add BCH  TrCh
    UmtsCphyTrChConfigReqInfo trCfgInfo;
    trCfgInfo.trChDir = UMTS_CH_DOWNLINK;
    trCfgInfo.trChId = 1;
    trCfgInfo.trChType = UMTS_TRANSPORT_BCH;
    trCfgInfo.ueId = node->nodeId; // common one use nodeB Id
    trCfgInfo.logChPrio = 0;

    memcpy((void*)&(trCfgInfo.transFormat),
       (void*)&umtsDefaultBchTransFormat,
       sizeof(UmtsTransportFormat));
    UmtsMacPhyHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_TrCH_CONFIG_REQ,
        (void*)&trCfgInfo);
    if (DEBUG_RRC_TRCH)
    {
        printf("node %d (L3) configure a BCH TrCh\n", node->nodeId);
    }

    // add PCH  TrCh
    trCfgInfo.trChDir = UMTS_CH_DOWNLINK;
    trCfgInfo.trChId = 1;
    trCfgInfo.trChType = UMTS_TRANSPORT_PCH;
    trCfgInfo.ueId = node->nodeId; // common one use nodeB Id
    memcpy((void*)&(trCfgInfo.transFormat),
       (void*)&umtsDefaultPchTransFormat,
       sizeof(UmtsTransportFormat));
    UmtsMacPhyHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_TrCH_CONFIG_REQ,
        (void*)&trCfgInfo);
    if (DEBUG_RRC_TRCH)
    {
        printf("node %d (L3) configure a PCH TrCh\n", node->nodeId);
    }

    // add FACH  TrCh
    trCfgInfo.trChDir = UMTS_CH_DOWNLINK;
    trCfgInfo.trChId = 1;
    trCfgInfo.trChType = UMTS_TRANSPORT_FACH;
    trCfgInfo.ueId = node->nodeId; // common one use nodeB Id
    memcpy((void*)&(trCfgInfo.transFormat),
       (void*)&umtsDefaultFachTransFormat,
       sizeof(UmtsTransportFormat));

    UmtsMacPhyHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_TrCH_CONFIG_REQ,
        (void*)&trCfgInfo);
    if (DEBUG_RRC_TRCH)
    {
        printf("node %d (L3) configure a PCH TrCh\n", node->nodeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodeBConfigCommonRadioBearer
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the common radio bearer
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodeBConfigCommonRadioBearer(Node *node,
                                            UmtsLayer3Data *umtsL3)
{

    UmtsCmacConfigReqRbInfo rbInfo;
    UmtsTrCh2PhChMappingInfo mapInfo;
    UmtsRlcTmConfig tmConfig;
    //UmtsRlcUmConfig umConfig;

    // BCCH Logical Channel
    // set the default BCH transport format
    memcpy((void*)&(mapInfo.transFormat),
       (void*)&umtsDefaultBchTransFormat,
       sizeof(UmtsTransportFormat));

    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = 1;
    rbInfo.logChType = UMTS_LOGICAL_BCCH;
    rbInfo.logChPrio = 0;
    rbInfo.priSc = node->nodeId;
    rbInfo.rbId = UMTS_BCCH_RB_ID;
    rbInfo.trChType = UMTS_TRANSPORT_BCH;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = 1;
    rbInfo.releaseRb = FALSE;
    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CMAC_CONFIG_REQ_RB,
        (void*)&rbInfo);

    mapInfo.rbId = rbInfo.rbId;
    mapInfo.chDir = UMTS_CH_DOWNLINK;
    mapInfo.priSc = node->nodeId;
    mapInfo.ueId = node->nodeId;
    mapInfo.trChType = UMTS_TRANSPORT_BCH;
    mapInfo.trChId = 1;
    mapInfo.phChType = UMTS_PHYSICAL_PCCPCH;
    mapInfo.phChId = 1;

    UmtsMacPhyMappingTrChPhCh(node, umtsL3->interfaceIndex, &mapInfo);

    tmConfig.segIndi = FALSE;
    UmtsLayer3ConfigRlcEntity(
        node,
        umtsL3,
        rbInfo.rbId,
        UMTS_RLC_ENTITY_TX,
        UMTS_RLC_ENTITY_TM,
        &tmConfig,
        rbInfo.ueId);

    if (DEBUG_RRC_RB)
    {
        printf("node %d (L3): configure a BCCH rb\n", node->nodeId);
        printf("node %d (L3): mapping a BCH to PCCPCH\n", node->nodeId);
    }

    // PCCH
    //use default PCH format
    memcpy((void*)&(mapInfo.transFormat),
       (void*)&umtsDefaultPchTransFormat,
       sizeof(UmtsTransportFormat));

    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = 1;
    rbInfo.logChType = UMTS_LOGICAL_PCCH;
    rbInfo.logChPrio = 0;
    rbInfo.priSc = node->nodeId;
    rbInfo.rbId = UMTS_PCCH_RB_ID;
    rbInfo.trChType = UMTS_TRANSPORT_PCH;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = 1;
    rbInfo.releaseRb = FALSE;
    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CMAC_CONFIG_REQ_RB,
        (void*)&rbInfo);

    mapInfo.rbId = rbInfo.rbId;
    mapInfo.chDir = UMTS_CH_DOWNLINK;
    mapInfo.priSc = node->nodeId;
    mapInfo.ueId = node->nodeId;
    mapInfo.trChType = UMTS_TRANSPORT_PCH;
    mapInfo.trChId = 1;
    mapInfo.phChType = UMTS_PHYSICAL_SCCPCH;
    mapInfo.phChId = 1;

    UmtsMacPhyMappingTrChPhCh(node, umtsL3->interfaceIndex, &mapInfo);

    tmConfig.segIndi = FALSE;
    UmtsLayer3ConfigRlcEntity(
        node,
        umtsL3,
        rbInfo.rbId,
        UMTS_RLC_ENTITY_TX,
        UMTS_RLC_ENTITY_TM,
        &tmConfig,
        rbInfo.ueId);

    if (DEBUG_RRC_RB)
    {
        printf("node %d (L3): configure a PCCH rb\n", node->nodeId);
        printf("node %d (L3): mapping a PCH to SCCPCH\n", node->nodeId);
    }

    // downlink CCCH (FACH)
    //set the defualt FACH format
    memcpy((void*)&(mapInfo.transFormat),
       (void*)&umtsDefaultFachTransFormat,
       sizeof(UmtsTransportFormat));

    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = 1;
    rbInfo.logChType = UMTS_LOGICAL_CCCH;
    rbInfo.logChPrio = 0;
    rbInfo.priSc = node->nodeId; //TODO
    rbInfo.rbId = UMTS_CCCH_RB_ID;
    rbInfo.trChType = UMTS_TRANSPORT_FACH;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = 1;
    rbInfo.releaseRb = FALSE;
    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CMAC_CONFIG_REQ_RB,
        (void*)&rbInfo);

    mapInfo.rbId = rbInfo.rbId;
    mapInfo.chDir = UMTS_CH_DOWNLINK;
    mapInfo.priSc = node->nodeId; // TODO
    mapInfo.ueId = node->nodeId;
    mapInfo.trChType = UMTS_TRANSPORT_FACH;
    mapInfo.trChId = 1;
    mapInfo.phChType = UMTS_PHYSICAL_SCCPCH;
    mapInfo.phChId = 1;

    UmtsMacPhyMappingTrChPhCh(node, umtsL3->interfaceIndex, &mapInfo);

    //umConfig.alterEbit = FALSE;
    //umConfig.dlLiLen = UMTS_RLC_DEF_UMDLLILEN;
    //umConfig.maxUlPduSize = UMTS_RLC_DEF_UMMAXULPDUSIZE;
    UmtsLayer3ConfigRlcEntity(
        node,
        umtsL3,
        rbInfo.rbId,
        UMTS_RLC_ENTITY_TX,
        UMTS_RLC_ENTITY_TM,
        &tmConfig,
        rbInfo.ueId);

    // uplink CCCH
    rbInfo.chDir = UMTS_CH_UPLINK;
    rbInfo.logChId = 1;
    rbInfo.logChType = UMTS_LOGICAL_CCCH;
    rbInfo.logChPrio = 1;
    rbInfo.priSc = node->nodeId; // TODO
    rbInfo.rbId = UMTS_CCCH_RB_ID;
    rbInfo.trChType = UMTS_TRANSPORT_RACH;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = 1;
    rbInfo.releaseRb = FALSE;
    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CMAC_CONFIG_REQ_RB,
        (void*)&rbInfo);

    UmtsLayer3ConfigRlcEntity(
        node,
        umtsL3,
        rbInfo.rbId,
        UMTS_RLC_ENTITY_RX,
        UMTS_RLC_ENTITY_TM,
        &tmConfig,
        rbInfo.ueId);

    if (DEBUG_RRC_RB)
    {
        printf("node %d (L3): configure a UL/DL CCCH rb\n", node->nodeId);
        printf("node %d (L3): mapping a FACH to SCCPCH\n", node->nodeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodeBConfigCommonRadioLink
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the common Radio Link
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodeBConfigCommonRadioLink(Node *node,
                                          UmtsLayer3Data *umtsL3)
{
    UmtsCphyRadioLinkSetupReqInfo rlReqInfo;
    rlReqInfo.phChDir = UMTS_CH_DOWNLINK;
    rlReqInfo.ueId = node->nodeId;

    // PSCH
    rlReqInfo.phChType = UMTS_PHYSICAL_PSCH;
    rlReqInfo.phChId = 1;

    UmtsCphyChDescPSCH pschDesc;
    pschDesc.txDevisityMode = FALSE;
    rlReqInfo.phChDesc = (void*) &pschDesc;

    UmtsMacPhyHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)&rlReqInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): configure a PSCH\n", node->nodeId);
    }

    // SSCH
    rlReqInfo.phChType = UMTS_PHYSICAL_SSCH;
    rlReqInfo.phChId = 1;

    UmtsCphyChDescSSCH sschDesc;
    sschDesc.txDevisityMode = FALSE;
    rlReqInfo.phChDesc = (void*) &sschDesc;

    UmtsMacPhyHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)&rlReqInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): configure a SSCH\n", node->nodeId);
    }

    // CPICH, cch, 255, 0
    rlReqInfo.phChType = UMTS_PHYSICAL_CPICH;
    rlReqInfo.phChId = 1;

    rlReqInfo.phChDesc = NULL;

    UmtsMacPhyHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)&rlReqInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): configure a CPICH\n", node->nodeId);
    }

    // P-CCPCH, cch,255,1
    rlReqInfo.phChType = UMTS_PHYSICAL_PCCPCH;
    rlReqInfo.phChId = 1;

    UmtsCphyChDescPCCPCH pccpchDesc;
    pccpchDesc.dlScCode = node->nodeId; // TODO
    pccpchDesc.freq = 0; // TODO
    pccpchDesc.txDevisityMode = FALSE;
    rlReqInfo.phChDesc = (void*) &pccpchDesc;

    UmtsMacPhyHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)&rlReqInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): configure a PCCPCH\n", node->nodeId);
    }

    // S-CCPCH 1st,cch 255, 2
    rlReqInfo.phChType = UMTS_PHYSICAL_SCCPCH;
    rlReqInfo.phChId = 1;

    UmtsCphyChDescSCCPCH sccpchDesc;
    sccpchDesc.dlScCode = node->nodeId; // TODO
    sccpchDesc.freq = 0; // TODO
    sccpchDesc.txDevisityMode = FALSE;
    sccpchDesc.chCode = 2; // cch, 255,2
    sccpchDesc.sccpchIndex = 1;
    int tk=0; // refer to p41, TS 25.211
    sccpchDesc.offsetInChips = tk*256;
    sccpchDesc.sfFactor = UMTS_SF_256;
    rlReqInfo.phChDesc = (void*) &sccpchDesc;

    UmtsMacPhyHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)&rlReqInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): configure a SCCPCH\n", node->nodeId);
    }

    // 1 st PICH
    rlReqInfo.phChType = UMTS_PHYSICAL_PICH;
    rlReqInfo.phChId = 1;

    UmtsCphyChDescPICH pichDesc;
    pichDesc.scCode = node->nodeId; // TODO
    pichDesc.chCode = 3; // cch, 255, 3
    pichDesc.offsetInChips = UMTS_PICH_TIMING_OFFSET;
    pichDesc.pichIndex = 1;
    pichDesc.sfFactor = UMTS_SF_256;
    rlReqInfo.phChDesc = (void*) &pichDesc;

    UmtsMacPhyHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)&rlReqInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): configure a PICH\n", node->nodeId);
    }

    // AICH
    rlReqInfo.phChType = UMTS_PHYSICAL_AICH;
    rlReqInfo.phChId = 1;

    UmtsCphyChDescAICH aichDesc;
    aichDesc.scCode = node->nodeId; // TODO
    aichDesc.chCode = 4; // cch, 255, 4
    aichDesc.sfFactor = UMTS_SF_256;
    aichDesc.txDivesityMode = FALSE;
    rlReqInfo.phChDesc = (void*) &aichDesc;

    UmtsMacPhyHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)&rlReqInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): configure a AICH\n", node->nodeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebReleaseSignalRb1To3
// LAYER      :: Layer 3
// PURPOSE    :: Release signalling RB 1-3
// + node      : Node*  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to the umts layer3 data
// + ueId      : NodeId  : The UE ID associated with this signalling RB
// + srbConfig : const UmtsNodebSrcConfigPara*  : 
//               Const pointer to the NodeB configuration parameters
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebReleaseSignalRb1To3(
    Node* node,
    UmtsLayer3Data *umtsL3,
    NodeId ueId,
    const UmtsNodebSrbConfigPara* srbConfig)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);

    // add this ue into local ueInfo at phy layer
    unsigned int uePriSc = srbConfig->uePriSc;
    UmtsMacPhyNodeBRemoveUeInfo(
        node,
        umtsL3->interfaceIndex,
        ueId,
        uePriSc);

    // update the number of UE in connected
    nodebL3->stat.numUeInConnected --;

    // GUI
    if (node->guiOption)
    {
        GUI_SendIntegerData(node->nodeId,
                            nodebL3->stat.numUeInConnectedGuiId,
                            nodebL3->stat.numUeInConnected,
                            node->getNodeTime());
    }

    if (DEBUG_RRC_RB)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("remove UE %d from UeInfoList\n",ueId);
    }

    // release Physical channel(downlink)
    UmtsCphyRadioLinkRelReqInfo relReqInfo;
    relReqInfo.phChDir = UMTS_CH_DOWNLINK;
    relReqInfo.phChType = UMTS_PHYSICAL_DPDCH;
    relReqInfo.phChId = srbConfig->dlDpdchId;
    UmtsLayer3ReleaseRadioLink(node, umtsL3, &relReqInfo);

    // release transport channel(downlink)
    UmtsCphyTrChReleaseReqInfo trRelInfo;
    trRelInfo.trChDir = UMTS_CH_DOWNLINK;
    trRelInfo.trChId = srbConfig->dlDchId;
    trRelInfo.trChType = UMTS_TRANSPORT_DCH;
    UmtsLayer3ReleaseTrCh(node, umtsL3, &trRelInfo);

    for (int i = 0; i < 3; i++)
    {
        // Rlease downLink bearer
        UmtsCmacConfigReqRbInfo rbInfo;
        rbInfo.chDir = UMTS_CH_DOWNLINK;
        rbInfo.logChId = srbConfig->dlDcchId[i];
        rbInfo.logChType = UMTS_LOGICAL_DCCH;
        rbInfo.logChPrio = (unsigned char)(i+1);
        rbInfo.priSc = uePriSc;
        rbInfo.rbId = (unsigned char)(i+1);
        rbInfo.trChType = trRelInfo.trChType;
        rbInfo.ueId = ueId;
        rbInfo.trChId = trRelInfo.trChId;
        rbInfo.releaseRb = TRUE;
        UmtsLayer3ReleaseRadioBearer(node, umtsL3, &rbInfo);

        UmtsLayer3ReleaseRlcEntity(node,
                                   umtsL3,
                                   (unsigned char)(i+1),
                                   UMTS_RLC_ENTITY_BI,
                                   ueId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebConfigSignalRb1To3
// LAYER      :: Layer 3
// PURPOSE    :: Setup signalling RB 1-3 for a UE at the NodeB
// + node      : Node* : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to the umts layer3 data
// + ueId      : NodeId : The UE ID associated with this signalling RB
// + srbConfig : const UmtsNodebSrcConfigPara*  : 
//               Const pointer to the NodeB configuration parameters
// + rrcConnSetup: BOOL : Whether this procedure is for RRC CONN SETUP
//                        (it may be active set update otherwise)
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebConfigSignalRb1To3(
    Node* node,
    UmtsLayer3Data *umtsL3,
    NodeId ueId,
    const UmtsNodebSrbConfigPara* srbConfig,
    BOOL  rrcConnSetup)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);

    // add this ue into local ueInfo at phy layer
    unsigned int uePriSc = srbConfig->uePriSc;
    UmtsMacPhyNodeBAddUeInfo(node,
                             umtsL3->interfaceIndex,
                             ueId,
                             uePriSc,
                             rrcConnSetup);

    // update the number of UE in connected
    nodebL3->stat.numUeInConnected ++;

    // GUI
    if (node->guiOption)
    {
        GUI_SendIntegerData(node->nodeId,
                            nodebL3->stat.numUeInConnectedGuiId,
                            nodebL3->stat.numUeInConnected,
                            node->getNodeTime());
    }

    if (DEBUG_RRC_RB)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("add UE %d into UEINFo List\n", ueId);
    }

    UmtsCphyRadioLinkSetupReqInfo rlInfo;
    UmtsCphyTrChConfigReqInfo trCfgInfo;
    UmtsTransportFormat transFormat;

    // assume 3.4k Singalling
    memcpy((void*)&transFormat,
           (void*)&umtsDefaultSigDchTransFormat_3400,
           sizeof(UmtsTransportFormat));

    UmtsCphyChDescDlDPCH phChDesc;
    phChDesc.dlScCode = nodebL3->cellId;
    phChDesc.sfFactor = srbConfig->sfFactor;
    phChDesc.dlChCode = srbConfig->chCode;
    phChDesc.slotFormatIndex = (unsigned char)-1; 
                                      // init as -1 to get the right one

    // use default one unless specified here
    UmtsLayer3GetDlPhChDataBitRate(
        phChDesc.sfFactor,
        (signed char*)&(phChDesc.slotFormatIndex));

    //Config physical channel(Downlink)
    rlInfo.phChDir = UMTS_CH_DOWNLINK;
    rlInfo.ueId = ueId;
    rlInfo.phChType = UMTS_PHYSICAL_DPDCH;
    rlInfo.phChId = srbConfig->dlDpdchId;
    rlInfo.priSc = nodebL3->cellId;
    rlInfo.phChDesc = &phChDesc;
    UmtsLayer3ConfigRadioLink(node, umtsL3, &rlInfo);

    // Config the transport channel(Downlink)
    trCfgInfo.trChDir = UMTS_CH_DOWNLINK;
    trCfgInfo.trChId = srbConfig->dlDchId;
    trCfgInfo.trChType = UMTS_TRANSPORT_DCH;
    trCfgInfo.ueId = ueId;
    trCfgInfo.logChPrio = 0;
    memcpy((void*)&trCfgInfo.transFormat,
           (void*)&transFormat,
           sizeof(UmtsTransportFormat));
    UmtsLayer3ConfigTrCh(node, umtsL3, &trCfgInfo);

    // Config radio bearer and make mapping
    for (int i = 0; i < 3; i++) // there are 3 common Rbs 
    {
        UmtsCmacConfigReqRbInfo rbInfo;

        // Configure Downlink
        rbInfo.chDir = UMTS_CH_DOWNLINK;
        rbInfo.logChId = srbConfig->dlDcchId[i];
        rbInfo.logChType = UMTS_LOGICAL_DCCH;
        rbInfo.logChPrio = (unsigned char)(i+1);
        rbInfo.priSc = ueId;
        rbInfo.rbId = (unsigned char)(i+1);
        rbInfo.trChType = trCfgInfo.trChType;
        rbInfo.ueId = ueId;
        rbInfo.trChId = trCfgInfo.trChId;
        rbInfo.releaseRb = FALSE;
        int phChId = rlInfo.phChId;
        UmtsLayer3ConfigRadioBearer(
             node,
             umtsL3,
             &rbInfo,
             &(rlInfo.phChType),
             &phChId,
             1,
             &transFormat);

        //Configure upLink
        rbInfo.chDir = UMTS_CH_UPLINK;
        rbInfo.logChId = srbConfig->dlDcchId[i];
        rbInfo.logChType = UMTS_LOGICAL_DCCH;
        rbInfo.logChPrio = (unsigned char)(i+1);
        rbInfo.priSc = ueId;
        rbInfo.rbId = (unsigned char)(i+1);
        rbInfo.trChType = trCfgInfo.trChType;
        rbInfo.ueId = ueId;
        rbInfo.trChId = srbConfig->dlDchId;
        rbInfo.releaseRb = FALSE;
        UmtsLayer3ConfigRadioBearer(
             node,
             umtsL3,
             &rbInfo,
             NULL,
             NULL,
             0);

        // Config RLC entity
        if (i == 0)
        {
            //Signalling RB1 use UM
            //Configure DownLink Entity
            UmtsRlcUmConfig entityConfig;
            entityConfig.alterEbit = FALSE;
            entityConfig.dlLiLen = UMTS_RLC_DEF_UMDLLILEN;
            entityConfig.maxUlPduSize = UMTS_RLC_DEF_UMMAXULPDUSIZE;
            UmtsLayer3ConfigRlcEntity(
                node,
                umtsL3,
                rbInfo.rbId,
                UMTS_RLC_ENTITY_TX,
                UMTS_RLC_ENTITY_UM,
                &entityConfig,
                ueId);

            //Config UpLink Entity
            UmtsLayer3ConfigRlcEntity(
                node,
                umtsL3,
                rbInfo.rbId,
                UMTS_RLC_ENTITY_RX,
                UMTS_RLC_ENTITY_UM,
                &entityConfig,
                ueId);
        }
        else
        {
            //Signalling RB2-3 use AM
            UmtsRlcAmConfig entityConfig;
            entityConfig.sndPduSize = transFormat.GetTransBlkSize()/8;
            entityConfig.rcvPduSize = transFormat.GetTransBlkSize()/8;
            ERROR_Assert(entityConfig.sndPduSize > 0
                         && entityConfig.rcvPduSize > 0,
                         "incorrect PDU size configured for RLC AM entity");
            entityConfig.sndWnd = UMTS_RLC_DEF_WINSIZE;
            entityConfig.rcvWnd = UMTS_RLC_DEF_WINSIZE;
            entityConfig.maxDat = UMTS_RLC_DEF_MAXDAT;
            entityConfig.maxRst = UMTS_RLC_DEF_MAXRST;
            entityConfig.rstTimerVal = UMTS_RLC_DEF_RSTTIMER_VAL;

            //Configure both upLink and downlink Entity
            UmtsLayer3ConfigRlcEntity(
                node,
                umtsL3,
                rbInfo.rbId,
                UMTS_RLC_ENTITY_TX,
                UMTS_RLC_ENTITY_AM,
                &entityConfig,
                ueId);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebReleaseHsdpaRb
// LAYER      :: Layer 3
// PURPOSE    :: Setup data HSDPA RB for a UE at the NodeB
// + node      : Node*  : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to the umts layer3 data
// + rbSetupPara : UmtsNodebRbSetupPara& : Info of the releasing RB
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebReleaseHsdpaRb(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsNodebRbSetupPara& rbSetupPara)
{
    NodeId ueId = rbSetupPara.ueId;

    int numPhCh = rbSetupPara.numDpdch;
    if (DEBUG_NBAP)
    {
        std::cout << "Number of downlink HSDPA channels: "
                  << numPhCh << std::endl;
    }

    // release HS-PDSCH Radio link if needed
    if (rbSetupPara.rlSetupNeeded)
    {
        for (int i = 0; i < numPhCh; i++)
        {
            int dlDpdchId = rbSetupPara.dlDpdchId[i];
            UmtsCphyRadioLinkRelReqInfo relReqInfo;
            relReqInfo.phChDir = UMTS_CH_DOWNLINK;
            if (rbSetupPara.sfFactor[i] == UMTS_SF_16)
            {
                relReqInfo.phChType = UMTS_PHYSICAL_HSPDSCH;
            }
            else if (rbSetupPara.sfFactor[i] == UMTS_SF_128)
            {
                relReqInfo.phChType = UMTS_PHYSICAL_HSSCCH;
            }
            else
            {
                ERROR_ReportWarning("unknown channel type for HSDPA rb");
                continue;
            }
            relReqInfo.phChId = (unsigned char)dlDpdchId;
            UmtsLayer3ReleaseRadioLink(node, umtsL3, &relReqInfo);
        }
    }

    char dlDchId = rbSetupPara.dlDchId;
    UmtsCphyTrChReleaseReqInfo trRelInfo;
    trRelInfo.trChDir = UMTS_CH_DOWNLINK;
    trRelInfo.trChId = dlDchId;
    trRelInfo.trChType = UMTS_TRANSPORT_HSDSCH;
    UmtsLayer3ReleaseTrCh(node, umtsL3, &trRelInfo);

    char dlDtchId = rbSetupPara.dlDtchId;
    char rbId = rbSetupPara.rbId;
    UmtsCmacConfigReqRbInfo rbInfo;
    rbInfo.priSc = rbSetupPara.uePriSc;
    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = dlDtchId;
    rbInfo.logChType = UMTS_LOGICAL_DTCH;
    rbInfo.ueId = ueId;
    rbInfo.rbId = rbId;
    rbInfo.trChType = trRelInfo.trChType;
    rbInfo.trChId = trRelInfo.trChId;
    rbInfo.releaseRb = TRUE;
    UmtsLayer3ReleaseRadioBearer(node, umtsL3, &rbInfo);

    //Release downlink entity
    UmtsLayer3ReleaseRlcEntity(node,
                               umtsL3,
                               rbId,
                               UMTS_RLC_ENTITY_BI,
                               ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebReleaseRb
// LAYER      :: Layer 3
// PURPOSE    :: Setup data RB for a UE at the NodeB
// + node      : Node*  : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to the umts layer3 data
// + rbSetupPara : UmtsNodebRbSetupPara& : Releasing Rb Info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebReleaseRb(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsNodebRbSetupPara& rbSetupPara)
{
    NodeId ueId = rbSetupPara.ueId;

    int numPhCh = rbSetupPara.numDpdch;
    if (DEBUG_NBAP)
    {
        std::cout << "Number of downlink channels: "
                  << numPhCh << std::endl;
    }

    if (rbSetupPara.rlSetupNeeded)
    {
        for (int i = 0; i < numPhCh; i++)
        {
            int dlDpdchId = rbSetupPara.dlDpdchId[i];
            UmtsCphyRadioLinkRelReqInfo relReqInfo;
            relReqInfo.phChDir = UMTS_CH_DOWNLINK;
            relReqInfo.phChType = UMTS_PHYSICAL_DPDCH;
            relReqInfo.phChId = (unsigned char)dlDpdchId;
            UmtsLayer3ReleaseRadioLink(node, umtsL3, &relReqInfo);
        }
    }

    char dlDchId = rbSetupPara.dlDchId;
    UmtsCphyTrChReleaseReqInfo trRelInfo;
    trRelInfo.trChDir = UMTS_CH_DOWNLINK;
    trRelInfo.trChId = dlDchId;
    trRelInfo.trChType = UMTS_TRANSPORT_DCH;
    UmtsLayer3ReleaseTrCh(node, umtsL3, &trRelInfo);

    char dlDtchId = rbSetupPara.dlDtchId;
    char rbId = rbSetupPara.rbId;
    UmtsCmacConfigReqRbInfo rbInfo;
    rbInfo.priSc = rbSetupPara.uePriSc;
    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = dlDtchId;
    rbInfo.logChType = UMTS_LOGICAL_DTCH;
    rbInfo.ueId = ueId;
    rbInfo.rbId = rbId;
    rbInfo.trChType = trRelInfo.trChType;
    rbInfo.trChId = trRelInfo.trChId;
    rbInfo.releaseRb = TRUE;
    UmtsLayer3ReleaseRadioBearer(node, umtsL3, &rbInfo);

    //Release downlink entity
    UmtsLayer3ReleaseRlcEntity(node,
                               umtsL3,
                               rbId,
                               UMTS_RLC_ENTITY_BI,
                               ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebConfigHsdpaRb
// LAYER      :: Layer 3
// PURPOSE    :: Setup HSDPA data RB for a UE at the NodeB
// + node      : Node* : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to the umts layer3 data
// + rbSetupPara : UmtsNodebRbSetupPara& : Info of RB
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebConfigHsdpaRb(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsNodebRbSetupPara& rbSetupPara)
{
    UmtsTransportFormat transFormat;
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);
    //NodeId ueId = rbSetupPara.ueId;

    int numPhCh = rbSetupPara.numDpdch;
    unsigned char numDataCh = 0;
    unsigned int hspdschId[15];
    unsigned char numCtrlCh = 0;
    unsigned int hsscchId[15];
    UmtsPhysicalChannelType dlPhChType[UMTS_MAX_DLDPDCH_PER_RAB];
    int dlPhChId[UMTS_MAX_DLDPDCH_PER_RAB];
    if (DEBUG_NBAP || DEBUG_RRC_RL_HSDPA)
    {
        std::cout << "Handling IE DL_DPCH_INFO: "
                  << "Number of channels: "
                  << numPhCh << std::endl;
    }

    // Get the right transport format
    // get the QoS requirement of this Rb

    memcpy((void*)&transFormat,
           &(rbSetupPara.transFormat),
           sizeof(UmtsTransportFormat));

    // for HSDPA
    // DPSCH is a shared downlink
    // add radio link if needed
    for (int i = 0; i < numPhCh; i++)
    {
        if (DEBUG_NBAP || DEBUG_RRC_RL_HSDPA)
        {
            std::cout << "Channel Id: " << (int)rbSetupPara.dlDpdchId[i]
                      << "     Spreading Factor: "
                      << rbSetupPara.sfFactor[i]
                      << "     Channelization code: "
                      << rbSetupPara.chCode[i]
                      << std::endl;
        }

        if (rbSetupPara.sfFactor[i] == UMTS_SF_16)
        {

            //keep the channel info for Rb setup use
            dlPhChType[numDataCh] = UMTS_PHYSICAL_HSPDSCH;
            dlPhChId[numDataCh] = rbSetupPara.dlDpdchId[i];

            hspdschId[numDataCh] = rbSetupPara.dlDpdchId[i];
            numDataCh ++;

            if (rbSetupPara.rlSetupNeeded)
            {
                UmtsCphyRadioLinkSetupReqInfo rlInfo;
                UmtsCphyChDescHspdsch pdschDesc;

                pdschDesc.scCode = nodebL3->cellId;;
                pdschDesc.sfFactor = rbSetupPara.sfFactor[i];
                pdschDesc.chCode = rbSetupPara.chCode[i];

                //Config physical channel(Downlink)
                rlInfo.phChDir = UMTS_CH_DOWNLINK;
                rlInfo.ueId = rbSetupPara.ueId;
                rlInfo.phChType = UMTS_PHYSICAL_HSPDSCH;
                rlInfo.phChId = rbSetupPara.dlDpdchId[i];
                rlInfo.priSc = nodebL3->cellId;
                rlInfo.phChDesc = (void*)&pdschDesc;
                UmtsLayer3ConfigRadioLink(node, umtsL3, &rlInfo);

                if (DEBUG_RRC_RL || DEBUG_RRC_RL_HSDPA)
                {
                    printf("node %d (L3): configure a HS-PDSCH %d\n",
                        node->nodeId, rlInfo.phChId);
                }
            }
        }
        else if (rbSetupPara.sfFactor[i] == UMTS_SF_128)
        {

            // keep the channelId to config phy
            hsscchId[numCtrlCh] = rbSetupPara.dlDpdchId[i];
            numCtrlCh ++;

            if (rbSetupPara.rlSetupNeeded)
            {
                UmtsCphyRadioLinkSetupReqInfo rlInfo;
                UmtsCphyChDescHsscch hscchDesc;

                hscchDesc.scCode = nodebL3->cellId;
                hscchDesc.sfFactor = rbSetupPara.sfFactor[i];
                hscchDesc.chCode = rbSetupPara.chCode[i];

                //Config physical channel(Downlink)
                rlInfo.phChDir = UMTS_CH_DOWNLINK;
                rlInfo.ueId = rbSetupPara.ueId;
                rlInfo.phChType = UMTS_PHYSICAL_HSSCCH;
                rlInfo.phChId = rbSetupPara.dlDpdchId[i];
                rlInfo.priSc = nodebL3->cellId;
                rlInfo.phChDesc = (void*)&hscchDesc;
                UmtsLayer3ConfigRadioLink(node, umtsL3, &rlInfo);

                if (DEBUG_RRC_RL || DEBUG_RRC_RL_HSDPA)
                {
                    printf("node %d (L3): configure a HS-SCCH\n",
                        node->nodeId);
                }
            }
        }
        else
        {
            if (DEBUG_RRC_RL)
            {
                printf("node %d (L3): configure Unknown Phy Ch\n", 
                    node->nodeId);
            }
        }
    }

    if (rbSetupPara.rlSetupNeeded)
    {
        // config the PHY HSDPA configuration
        UmtsMacPhyNodeBSetHsdpaConfig(
            node, umtsL3->interfaceIndex,
            numDataCh, hspdschId, numCtrlCh, hsscchId);
    }

    int dlTrChId = rbSetupPara.dlDchId;
    if (DEBUG_NBAP)
    {
        std::cout << "IE DL_TrCh added: " << dlTrChId
                  << std::endl;
    }
    UmtsCphyTrChConfigReqInfo trCfgInfo;
    trCfgInfo.trChDir = UMTS_CH_DOWNLINK;
    trCfgInfo.trChId = (unsigned char)dlTrChId;
    trCfgInfo.trChType = UMTS_TRANSPORT_HSDSCH;
    trCfgInfo.ueId = rbSetupPara.ueId;
    trCfgInfo.logChPrio = rbSetupPara.logPriority;
    memcpy(&(trCfgInfo.transFormat),
        &(rbSetupPara.transFormat),
        sizeof(UmtsTransportFormat));
    UmtsLayer3ConfigTrCh(node, umtsL3, &trCfgInfo);

    char rbId = rbSetupPara.rbId;
    if (DEBUG_NBAP)
    {
        std::cout << "IE RAB Info to Setup: "
                  << "  RB ID: " << (int)rbId
                  << std::endl;
    }

    UmtsCmacConfigReqRbInfo rbInfo;

    // Configure Downlink RB
    rbInfo.priSc = rbSetupPara.uePriSc;
    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = rbSetupPara.dlDtchId;
    rbInfo.logChType = UMTS_LOGICAL_DTCH;
    rbInfo.logChPrio = rbSetupPara.logPriority;
    rbInfo.rbId = rbId;
    rbInfo.trChType = UMTS_TRANSPORT_HSDSCH;
    rbInfo.ueId = rbSetupPara.ueId;
    rbInfo.trChId = rbSetupPara.dlDchId;
    rbInfo.releaseRb = FALSE;

    UmtsLayer3ConfigRadioBearer(
         node,
         umtsL3,
         &rbInfo,
         dlPhChType,
         dlPhChId,
         numPhCh,
         &transFormat);

    //Configure upLink
    rbInfo.chDir = UMTS_CH_UPLINK;
    UmtsLayer3ConfigRadioBearer(
         node,
         umtsL3,
         &rbInfo,
         NULL,
         NULL,
         0);

    UmtsRlcEntityType rlcEntityType = rbSetupPara.rlcType;
    if (rlcEntityType == UMTS_RLC_ENTITY_TM)
    {
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_TX,
            UMTS_RLC_ENTITY_TM,
            &rbSetupPara.rlcConfig.tmConfig,
            rbSetupPara.ueId);
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_RX,
            UMTS_RLC_ENTITY_TM,
            &rbSetupPara.rlcConfig.tmConfig,
            rbSetupPara.ueId);
    }
    else if (rlcEntityType == UMTS_RLC_ENTITY_UM)
    {
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_TX,
            UMTS_RLC_ENTITY_UM,
            &rbSetupPara.rlcConfig.umConfig,
            rbSetupPara.ueId);
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_RX,
            UMTS_RLC_ENTITY_UM,
            &rbSetupPara.rlcConfig.umConfig,
            rbSetupPara.ueId);
    }
    else if (rlcEntityType == UMTS_RLC_ENTITY_AM)
    {
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_TX,
            UMTS_RLC_ENTITY_AM,
            &rbSetupPara.rlcConfig.amConfig,
            rbSetupPara.ueId);
    }
    else
    {
        ERROR_ReportError("UmtsLayer3NodebConfigHsdpaRb: "
            "receives wrong RLC entity type to setup ");
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebConfigRb
// LAYER      :: Layer 3
// PURPOSE    :: Setup data RB for a UE at the NodeB
// + node      : Node* : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to the umts layer3 data
// + rbSetupPara : UmtsNodebRbSetupPara& : Rb Info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebConfigRb(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsNodebRbSetupPara& rbSetupPara)
{
    UmtsTransportFormat transFormat;
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);
    // NodeId ueId = rbSetupPara.ueId;

    int numPhCh = rbSetupPara.numDpdch;
    UmtsPhysicalChannelType dlPhChType[UMTS_MAX_DLDPDCH_PER_RAB];
    int dlPhChId[UMTS_MAX_DLDPDCH_PER_RAB];
    if (DEBUG_NBAP)
    {
        std::cout << "Handling IE DL_DPCH_INFO: "
                  << "Number of channels: "
                  << numPhCh << std::endl;
    }

    // Get the right transport format
    // get the QoS requirement of this Rb

    memcpy((void*)&transFormat,
           &(rbSetupPara.transFormat),
           sizeof(UmtsTransportFormat));

    for (int i = 0; i < numPhCh; i++)
    {
        dlPhChType[i] = UMTS_PHYSICAL_DPDCH;
        dlPhChId[i] = rbSetupPara.dlDpdchId[i];
        if (DEBUG_NBAP)
        {
            std::cout << "Channel Id: " << (int)rbSetupPara.dlDpdchId[i]
                      << "     Spreading Factor: "
                      << rbSetupPara.sfFactor[i]
                      << "     Channelization code: "
                      << rbSetupPara.chCode[i]
                      << std::endl;
        }

        if (rbSetupPara.rlSetupNeeded)
        {
            UmtsCphyRadioLinkSetupReqInfo rlInfo;
            UmtsCphyChDescDlDPCH phChDesc;
            phChDesc.dlScCode = nodebL3->cellId;
            phChDesc.sfFactor = rbSetupPara.sfFactor[i];
            phChDesc.dlChCode = rbSetupPara.chCode[i];
            phChDesc.slotFormatIndex = (unsigned char)-1; 
                                   // init as -1 to get the right one

            UmtsLayer3GetDlPhChDataBitRate(
                phChDesc.sfFactor,
                (signed char*)&(phChDesc.slotFormatIndex));

            //Config physical channel(Downlink)
            rlInfo.phChDir = UMTS_CH_DOWNLINK;
            rlInfo.ueId = rbSetupPara.ueId;
            rlInfo.phChType = UMTS_PHYSICAL_DPDCH;
            rlInfo.phChId = rbSetupPara.dlDpdchId[i];
            rlInfo.priSc = nodebL3->cellId;
            rlInfo.phChDesc = &phChDesc;
            UmtsLayer3ConfigRadioLink(node, umtsL3, &rlInfo);
        }
    }

    int dlTrChId = rbSetupPara.dlDchId;
    if (DEBUG_NBAP)
    {
        std::cout << "IE DL_TrCh added: " << dlTrChId
                  << std::endl;
    }
    UmtsCphyTrChConfigReqInfo trCfgInfo;
    trCfgInfo.trChDir = UMTS_CH_DOWNLINK;
    trCfgInfo.trChId = (unsigned char)dlTrChId;
    trCfgInfo.trChType = UMTS_TRANSPORT_DCH;
    trCfgInfo.ueId = rbSetupPara.ueId;
    trCfgInfo.logChPrio = rbSetupPara.logPriority;
    memcpy(&(trCfgInfo.transFormat),
        &(rbSetupPara.transFormat),
        sizeof(UmtsTransportFormat));
    UmtsLayer3ConfigTrCh(node, umtsL3, &trCfgInfo);

    char rbId = rbSetupPara.rbId;
    if (DEBUG_NBAP)
    {
        std::cout << "IE RAB Info to Setup: "
                  << "  RB ID: " << (int)rbId
                  << std::endl;
    }

    UmtsCmacConfigReqRbInfo rbInfo;
    // Configure Downlink
    rbInfo.priSc = rbSetupPara.uePriSc;
    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = rbSetupPara.dlDtchId;
    rbInfo.logChType = UMTS_LOGICAL_DTCH;
    rbInfo.logChPrio = rbSetupPara.logPriority;
    rbInfo.rbId = rbId;
    rbInfo.trChType = UMTS_TRANSPORT_DCH;
    rbInfo.ueId = rbSetupPara.ueId;
    rbInfo.trChId = rbSetupPara.dlDchId;
    rbInfo.releaseRb = FALSE;

    UmtsLayer3ConfigRadioBearer(
         node,
         umtsL3,
         &rbInfo,
         dlPhChType,
         dlPhChId,
         numPhCh,
         &transFormat);

    //Configure upLink
    rbInfo.chDir = UMTS_CH_UPLINK;
    UmtsLayer3ConfigRadioBearer(
         node,
         umtsL3,
         &rbInfo,
         NULL,
         NULL,
         0);

    UmtsRlcEntityType rlcEntityType = rbSetupPara.rlcType;
    if (rlcEntityType == UMTS_RLC_ENTITY_TM)
    {
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_TX,
            UMTS_RLC_ENTITY_TM,
            &rbSetupPara.rlcConfig.tmConfig,
            rbSetupPara.ueId);
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_RX,
            UMTS_RLC_ENTITY_TM,
            &rbSetupPara.rlcConfig.tmConfig,
            rbSetupPara.ueId);
    }
    else if (rlcEntityType == UMTS_RLC_ENTITY_UM)
    {
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_TX,
            UMTS_RLC_ENTITY_UM,
            &rbSetupPara.rlcConfig.umConfig,
            rbSetupPara.ueId);
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_RX,
            UMTS_RLC_ENTITY_UM,
            &rbSetupPara.rlcConfig.umConfig,
            rbSetupPara.ueId);
    }
    else if (rlcEntityType == UMTS_RLC_ENTITY_AM)
    {
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_TX,
            UMTS_RLC_ENTITY_AM,
            &rbSetupPara.rlcConfig.amConfig,
            rbSetupPara.ueId);
    }
    else
    {
        ERROR_ReportError("UmtsLayer3NodebConfigRb: "
            "receives wrong RLC entity type to setup ");
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebPrintParameter
// LAYER      :: Layer 3
// PURPOSE    :: Print out NodeB specific parameters
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + nodebL3   : UmtsLayer3Nodeb * : Pointer to UMTS NodeB Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebPrintParameter(Node *node, UmtsLayer3Nodeb *nodebL3)
{
    printf("Node%d(NodeB)'s specific parameters:\n", node->nodeId);
    printf("    Qrxlevmin = %f\n", nodebL3->para.Qrxlevmin);
    printf("    Ssearch = %f\n", nodebL3->para.Ssearch);
    printf("    Qhyst = %f\n", nodebL3->para.Qhyst);

    return;
}

// /**
// FUNCTION   :: UmtsLayer3NodebInitParameter
// LAYER      :: Layer 3
// PURPOSE    :: Initialize NodeB specific parameters
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + nodeInput : const NodeInput*  : Pointer to node input.
// + nodebL3   : UmtsLayer3Nodeb * : Pointer to UMTS NodeB Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebInitParameter(Node *node,
                                  const NodeInput *nodeInput,
                                  UmtsLayer3Nodeb *nodebL3)
{
    BOOL wasFound;
    double doubleVal;

    // read the minimum RX level for cell selection (Qrxlevmin)
    IO_ReadDouble(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "UMTS-CELL-SELECTION-MIN-RX-LEVEL",
                  &wasFound,
                  &doubleVal);

    if (wasFound)
    {
        // do validation check first
        if (nodebL3->duplexMode == UMTS_DUPLEX_MODE_FDD &&
            (doubleVal < UMTS_MIN_FDD_CELL_SELECTION_MIN_RX_LEVEL ||
             doubleVal > UMTS_MAX_FDD_CELL_SELECTION_MIN_RX_LEVEL))
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "UMTS Node%d(NodeB): The value of parameter "
                    "UMTS-CELL-SELECTION-MIN-RX-LEVEL must be "
                    "between [%.2f, %.2f]. Default value %.2f is used.\n",
                    node->nodeId,
                    UMTS_MIN_FDD_CELL_SELECTION_MIN_RX_LEVEL,
                    UMTS_MAX_FDD_CELL_SELECTION_MIN_RX_LEVEL,
                    UMTS_DEFAULT_FDD_CELL_SELECTION_MIN_RX_LEVEL);
            ERROR_ReportWarning(errString);

            nodebL3->para.Qrxlevmin =
                UMTS_DEFAULT_FDD_CELL_SELECTION_MIN_RX_LEVEL;
        }
        else
        {
            // stored the value
            nodebL3->para.Qrxlevmin = doubleVal;
        }
    }
    else
    {
        // use the default value
        if (nodebL3->duplexMode == UMTS_DUPLEX_MODE_FDD)
        {
            nodebL3->para.Qrxlevmin =
                UMTS_DEFAULT_FDD_CELL_SELECTION_MIN_RX_LEVEL;
        }
        else
        {
            nodebL3->para.Qrxlevmin =
                UMTS_DEFAULT_TDD_CELL_SELECTION_MIN_RX_LEVEL;
        }
    }

    // read the threshold triggering cell search (Sintrasearch)
    IO_ReadDouble(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "UMTS-CELL-SEARCH-THRESHOLD",
                  &wasFound,
                  &doubleVal);

    if (wasFound && doubleVal >= UMTS_MIN_CELL_SEARCH_THRESHOLD &&
        doubleVal <= UMTS_MAX_CELL_SEARCH_THRESHOLD)
    {
        // stored the value
        nodebL3->para.Ssearch = doubleVal;
    }
    else
    {
        if (wasFound)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "UMTS Node%d(NodeB): The value of parameter "
                    "UMTS-CELL-SEARCH-THRESHOLD must be "
                    "between [%.2f, %.2f]. Default value %.2f is used.\n",
                    node->nodeId,
                    UMTS_MIN_CELL_SEARCH_THRESHOLD,
                    UMTS_MAX_CELL_SEARCH_THRESHOLD,
                    UMTS_DEFAULT_CELL_SEARCH_THRESHOLD);
            ERROR_ReportWarning(errString);
        }

        // use the default value
        nodebL3->para.Ssearch = UMTS_DEFAULT_CELL_SEARCH_THRESHOLD;
    }

    // read the hysteresis for cell reselection (Qhyst)
    IO_ReadDouble(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "UMTS-CELL-RESELECTION-HYSTERESIS",
                  &wasFound,
                  &doubleVal);

    if (wasFound && doubleVal > UMTS_MIN_CELL_RESELECTION_HYSTERESIS &&
        doubleVal < UMTS_MAX_CELL_RESELECTION_HYSTERESIS)
    {
        // do validation check first
        // stored the value
        nodebL3->para.Qhyst = doubleVal;
    }
    else
    {
        if (wasFound)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "UMTS Node%d(UE): Value of parameter "
                    "UMTS-CELL-RESELECTION-HYSTERESIS is invalid. "
                    "It must be a number larger than %.2f "
                    "and less than %.2f. Default value as %.2f is used.\n",
                    node->nodeId,
                    UMTS_MIN_CELL_RESELECTION_HYSTERESIS,
                    UMTS_MAX_CELL_RESELECTION_HYSTERESIS,
                    UMTS_DEFAULT_CELL_RESELECTION_HYSTERESIS);
            ERROR_ReportWarning(errString);
        }

        // use the default value
        nodebL3->para.Qhyst = UMTS_DEFAULT_CELL_RESELECTION_HYSTERESIS;
    }

    nodebL3->para.sysInfoBcstTime = UMTS_DEFAULT_SIB_INTERVAL;

    //Get Link bandwidth and IP fragmentation size

    //Assuming all wired links in UMTS is configured as 10Mbps bandwidth
    nodebL3->linkBandWidth = 10000000;
    int ipFragUnit;
    IO_ReadInt(ANY_NODEID,
               ANY_ADDRESS,
               nodeInput,
               "IP-FRAGMENTATION-UNIT",
               &wasFound,
               &ipFragUnit);

    if (!wasFound)
    {
        ipFragUnit = MAX_NW_PKT_SIZE;
    }
    else if (ipFragUnit < MIN_NW_PKT_SIZE || ipFragUnit > MAX_NW_PKT_SIZE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "IP fragmentation unit (%d) should not be less"
                " than MIN_NW_PKT_SIZE (%d) nor greater than"
                " MAX_NW_PKT_SIZE (%d)", ipFragUnit, MIN_NW_PKT_SIZE,
                MAX_NW_PKT_SIZE);
        
        ERROR_ReportError(errString);
    }
    nodebL3->cellSwitchFragSize = ipFragUnit - 128;
    
    return;
}

// /**
// FUNCTION   :: UmtsLayer3NodebPrintStats
// LAYER      :: Layer 3
// PURPOSE    :: Print out NodeB specific statistics
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodebL3      : UmtsLayer3Nodeb *   : Pointer to UMTS NodeB Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebPrintStats(Node *node, UmtsLayer3Nodeb *nodebL3)
{
    return;
}

// /**
// FUNCTION   :: UmtsLayer3NodebPrintStats
// LAYER      :: Layer 3
// PURPOSE    :: Print out NodeB specific statistics
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3NodebSendHelloPacket(Node* node, UmtsLayer3Data* umtsL3)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);
    NodeAddress subnetAddress;
    int numHostBits;
    std::string buffer;
    int i;

    buffer.append((char*)&nodebL3->cellId, sizeof(nodebL3->cellId));
    for (i = 0; i < node->numberInterfaces; i ++)
    {
        if (node->macData[i]->macProtocol == MAC_PROTOCOL_CELLULAR)
        {
            subnetAddress = NetworkIpGetInterfaceNetworkAddress(node, i);
            numHostBits = NetworkIpGetInterfaceNumHostBits(node, i);
            buffer.append((char*)&subnetAddress, sizeof(subnetAddress));
            buffer.append((char*)&numHostBits, sizeof(numHostBits));
        }
    }
    UmtsLayer3SendHelloPacket(node,
                              umtsL3,
                              ANY_INTERFACE,
                              (int)buffer.size(),
                              buffer.data());
}

// /**
// FUNCTION   :: UmtsLayer3NodebSendNbapMessageToRnc
// LAYER      :: Layer 3
// PURPOSE    :: Send a NBAP message to the RNC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the NBAP message
// + msgType   : UmtsNbapMessageType: NBAP message type
// + transctId : unsigned char    : The message transaction ID
// + ueId      : NodeId           : The UE ID the message associated with
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebSendNbapMessageToRnc(Node *node,
                                         UmtsLayer3Data *umtsL3,
                                         Message *msg,
                                         UmtsNbapMessageType msgType,
                                         unsigned char transctId,
                                         NodeId ueId)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);

    UmtsAddNbapHeader(node,
                      msg,
                      transctId,
                      ueId);
    UmtsAddIubBackboneHeader(node,
                             msg,
                             ANY_NODEID,
                             (UInt8)msgType);
    UmtsLayer3SendMsgOverBackbone(node,
                                  msg,
                                  nodebL3->rncInfo.nodeAddr,
                                  nodebL3->rncInfo.interfaceIndex,
                                  IPTOS_PREC_INTERNETCONTROL,
                                  1);
}

// /**
// FUNCTION   :: UmtsLayer3NodebSendRrcSetupResp
// LAYER      :: Layer 3
// PURPOSE    :: Send a NBAP message to the RNC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : unsigned char    : The message transaction ID
// + ueId      : NodeId           : The UE ID the message associated with
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebSendRrcSetupResp(
    Node *node,
    UmtsLayer3Data *umtsL3,
    unsigned char transctId,
    NodeId ueId)
{
    Message* msg = MESSAGE_Alloc(
                       node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        0,
                        TRACE_UMTS_LAYER3);
    UmtsLayer3NodebSendNbapMessageToRnc(
        node,
        umtsL3,
        msg,
        UMTS_NBAP_MSG_TYPE_RRC_SETUP_RESPONSE,
        transctId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleHelloPacket
// LAYER      :: Layer 3
// PURPOSE    :: Handle a hello packet from neighbor RNCs. This is used to
//               find out the RNC node that this NodeB connects to
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message containing the packet
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + interfaceIndex : int         : Interface from which packet was received
// + srcAddr   : Address          : Address of the source node
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebHandleHelloPacket(Node *node,
                                      Message *msg,
                                      UmtsLayer3Data *umtsL3,
                                      int interfaceIndex,
                                      Address srcAddr)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);
    UmtsLayer3HelloPacket helloPkt;

    memcpy(&helloPkt, MESSAGE_ReturnPacket(msg), sizeof(helloPkt));

    if (helloPkt.nodeType == CELLULAR_RNC &&
        (nodebL3->rncInfo.nodeId == helloPkt.nodeId ||
         nodebL3->rncInfo.nodeId == ANY_DEST))
    {
        if (DEBUG_BACKBONE)
        {
            printf("Node%d(NodeB) discover RNC %d from inf %d\n",
                    node->nodeId, helloPkt.nodeId, interfaceIndex);
        }

        nodebL3->rncInfo.nodeId = helloPkt.nodeId;
        nodebL3->rncInfo.nodeAddr = srcAddr;
        nodebL3->rncInfo.interfaceIndex = interfaceIndex;
        // assuem each RNC has its own Reg Area Code
        nodebL3->regAreaId = (UInt16)(nodebL3->rncInfo.nodeId);
        nodebL3->configured = TRUE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleNbapRrcSetupRelease
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RRC_SETUP_RELEASE from RNC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + transctId : unsigned char    : The Message transaction ID
// + ueId      : NodeId           : UE ID which RRC connection will 
//                                  be setup with
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebHandleNbapRrcSetupRelease(
        Node* node,
        UmtsLayer3Data* umtsL3,
        Message* msg,
        unsigned char transctId,
        NodeId ueId)
{
    UmtsNodebSrbConfigPara srbConfig;

    memcpy(&srbConfig,
           MESSAGE_ReturnPacket(msg),
           sizeof(UmtsNodebSrbConfigPara));

    if (DEBUG_NBAP && 1)
    {
        std::cout << "NodeB: " << node->nodeId
                  << " received an RRC_SETUP_RELEASE message from RNC"
                  << " with transaction ID: " << (unsigned int)transctId
                  << " and UE ID: " << ueId << std::endl
                  << "Allocated channels for SRB 1-3: " << std::endl
                  << "Downlink DCCH ID:  "
                  << "SRB1: " << (int)srbConfig.dlDcchId[0] << "\t"
                  << "SRB2: " << (int)srbConfig.dlDcchId[1] <<  "\t"
                  << "SRB3: " << (int)srbConfig.dlDcchId[2] <<  std::endl
                  << "Downlink DCH ID:   "
                  << "SRB1-3: " << (int)srbConfig.dlDchId << "\t"
                  << "Downlink DPDCH: "
                  << "ID: " << (int)srbConfig.dlDpdchId 
                  << "\t" << std::endl;
    }

    UmtsLayer3NodebReleaseSignalRb1To3(
        node,
        umtsL3,
        ueId,
        &srbConfig);

    Message* resMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    NETWORK_PROTOCOL_CELLULAR,
                                    0);
    MESSAGE_PacketAlloc(node,
                        resMsg,
                        0,
                        TRACE_UMTS_LAYER3);
    UmtsLayer3NodebSendNbapMessageToRnc(
        node,
        umtsL3,
        resMsg,
        UMTS_NBAP_MSG_TYPE_RRC_RELEASE_RESPONSE,
        transctId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleNbapRrcSetupRequest
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RRC_SETUP_REQUEST from RNC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + transctId : unsigned char    : The Message transaction ID
// + ueId      : NodeId           : UE ID which RRC connection 
//                                  will be setup with
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebHandleNbapRrcSetupRequest(
        Node* node,
        UmtsLayer3Data* umtsL3,
        Message* msg,
        unsigned char transctId,
        NodeId ueId)
{
    UmtsNodebSrbConfigPara srbConfig;

    memcpy(&srbConfig,
           MESSAGE_ReturnPacket(msg),
           sizeof(UmtsNodebSrbConfigPara));

    if (DEBUG_NBAP && 0)
    {
        std::cout << "NodeB: " << node->nodeId
                  << " received an RRC_SETUP_REQUEST message from RNC"
                  << " with transaction ID: " << (unsigned int)transctId
                  << " and UE ID: " << ueId << std::endl
                  << "Allocated channels for SRB 1-3: " << std::endl
                  << "Downlink DCCH ID:  "
                  << "SRB1: " << (int)srbConfig.dlDcchId[0] << "\t"
                  << "SRB2: " << (int)srbConfig.dlDcchId[1] <<  "\t"
                  << "SRB3: " << (int)srbConfig.dlDcchId[2] <<  std::endl
                  << "Downlink DCH ID:   "
                  << "SRB1-3: " << (int)srbConfig.dlDchId << "\t"
                  << "Downlink DPDCH: "
                  << "ID: " << (int)srbConfig.dlDpdchId << "\t"
                  << "SFactor: " << srbConfig.sfFactor <<  "\t"
                  << "OVSF code: " << srbConfig.chCode <<  std::endl;
    }

    UmtsLayer3NodebConfigSignalRb1To3(
        node,
        umtsL3,
        ueId,
        &srbConfig,
        TRUE);

    UmtsLayer3NodebSendRrcSetupResp(
        node,
        umtsL3,
        transctId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleNbapRbRelease
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Radio Bearer Release from RNC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + transctId : unsigned char    : The Message transaction ID
// + ueId      : NodeId           : UE ID which RRC connection 
//                                  will be setup with
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebHandleNbapRbRelease(
        Node* node,
        UmtsLayer3Data* umtsL3,
        Message* msg,
        unsigned char transctId,
        NodeId ueId)
{
    char* packet = MESSAGE_ReturnPacket(msg);

    if (DEBUG_NBAP)
    {
        std::cout << "NodeB: " << node->nodeId
                  << " receives an RADIO_BEARER_SETUP_RELEASE "
                  << " message from RNC" << std::endl;
    }

    UmtsNodebRbSetupPara rbSetupPara;
    memcpy(&rbSetupPara,  packet, sizeof(rbSetupPara));

    if (!rbSetupPara.hsdpaRb)
    {
        UmtsLayer3NodebReleaseRb(
                             node,
                             umtsL3,
                             rbSetupPara);
    }
    else
    {
        UmtsLayer3NodebReleaseHsdpaRb(
                             node,
                             umtsL3,
                             rbSetupPara);
    }

    Message* resMsg = MESSAGE_Alloc(
                          node,
                          NETWORK_LAYER,
                          NETWORK_PROTOCOL_CELLULAR,
                          0);
    MESSAGE_PacketAlloc(node,
                        resMsg,
                        0,
                        TRACE_UMTS_LAYER3);
    UmtsLayer3NodebSendNbapMessageToRnc(
        node,
        umtsL3,
        resMsg,
        UMTS_NBAP_MSG_TYPE_RADIO_BEARER_RELEASE_RESPONSE,
        transctId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleNbapRbSetup
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Radio Bearer Setup from RNC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + transctId : unsigned char    : The Message transaction ID
// + ueId      : NodeId           : UE ID which RRC connection 
//                                  will be setup with
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebHandleNbapRbSetup(
        Node* node,
        UmtsLayer3Data* umtsL3,
        Message* msg,
        unsigned char transctId,
        NodeId ueId)
{
    char* packet = MESSAGE_ReturnPacket(msg);

    if (DEBUG_NBAP)
    {
        std::cout << "NodeB: " << node->nodeId
                  << " received an RADIO_BEARER_SETUP_REQUEST "
                  << " message from RNC" << std::endl;
    }

    UmtsNodebRbSetupPara rbSetupPara;
    memcpy(&rbSetupPara,  packet, sizeof(rbSetupPara));

    if (!rbSetupPara.hsdpaRb)
    {
        UmtsLayer3NodebConfigRb(node,
                            umtsL3,
                            rbSetupPara);
    }
    else
    {
        UmtsLayer3NodebConfigHsdpaRb(node,
                                     umtsL3,
                                     rbSetupPara);
    }
    Message* replyMsg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            NETWORK_PROTOCOL_CELLULAR,
                            0);
    MESSAGE_PacketAlloc(node,
                        replyMsg,
                        sizeof(rbSetupPara),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(replyMsg),
           &rbSetupPara,
           sizeof(rbSetupPara));
    UmtsLayer3NodebSendNbapMessageToRnc(
        node,
        umtsL3,
        replyMsg,
        UMTS_NBAP_MSG_TYPE_RADIO_BEARER_SETUP_RESPONSE,
        transctId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleNbapJoinActvSet
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Join active set message
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + transctId : unsigned char    : The Message transaction ID
// + ueId      : NodeId           : UE ID which RRC connection 
//                                  will be setup with
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebHandleNbapJoinActvSet(
        Node* node,
        UmtsLayer3Data* umtsL3,
        Message* msg,
        unsigned char transctId,
        NodeId ueId)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);
    if (DEBUG_HO)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\trcvd  JOIN ACTIVE SET for UE %d\n", ueId);
    }

    std::string msgBuf;
    msgBuf.reserve(100);
    msgBuf.append((char*)&(nodebL3->cellId), sizeof(UInt32));

    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);
    UmtsNodebSrbConfigPara nodebSrbConfig;
    memcpy(&nodebSrbConfig,
           &packet[index],
           sizeof(nodebSrbConfig));
    index += sizeof(nodebSrbConfig);
    UmtsLayer3NodebConfigSignalRb1To3(
            node,
            umtsL3,
            ueId,
            &nodebSrbConfig,
            FALSE);
    msgBuf.append((char*)&nodebSrbConfig.sfFactor,
                   sizeof(UmtsSpreadFactor));
    msgBuf.append((char*)&nodebSrbConfig.chCode,
                   sizeof(UInt32));

    int numRbs = (int) packet[index++];

    msgBuf += (char)numRbs;
    for (int i = 0; i < numRbs; i++)
    {
        UmtsNodebRbSetupPara nodebRbPara;
        memcpy(&nodebRbPara,
               &packet[index],
               sizeof(nodebRbPara));
        index += sizeof(nodebRbPara);
        if (!nodebRbPara.hsdpaRb)
        {

            UmtsLayer3NodebConfigRb(
                node,
                umtsL3,
                nodebRbPara);
        }
        else
        {
            UmtsLayer3NodebConfigHsdpaRb(
                node,
                umtsL3,
                nodebRbPara);
        }
        msgBuf += (char)nodebRbPara.rbId;
        msgBuf += (char)nodebRbPara.numDpdch;
        for (int j = 0; j < nodebRbPara.numDpdch; j++)
        {
            msgBuf.append((char*)&nodebRbPara.sfFactor[j],
                          sizeof(UmtsSpreadFactor));
            msgBuf.append((char*)&nodebRbPara.chCode[j],
                          sizeof(UInt32));
        }
    }

    Message* replyMsg = MESSAGE_Alloc(node,
                                      NETWORK_LAYER, 
                                      NETWORK_PROTOCOL_CELLULAR,
                                      0);
    MESSAGE_PacketAlloc(node,
                        replyMsg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(replyMsg),
           msgBuf.data(),
           msgBuf.size());
    UmtsLayer3NodebSendNbapMessageToRnc(
        node,
        umtsL3,
        replyMsg,
        UMTS_NBAP_MSG_TYPE_JOIN_ACTIVE_SET_RESPONSE,
        transctId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleNbapLeaveActvSet
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Leave active set message
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + transctId : unsigned char    : The Message transaction ID
// + ueId      : NodeId           : UE ID which RRC connection 
//                                  will be setup with
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebHandleNbapLeaveActvSet(
        Node* node,
        UmtsLayer3Data* umtsL3,
        Message* msg,
        unsigned char transctId,
        NodeId ueId)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);
    if (DEBUG_HO)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\trcvd  LEAVE ACTIVE SET for UE %d\n", ueId);
    }

    int index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);
    UmtsNodebSrbConfigPara nodebSrbConfig;
    memcpy(&nodebSrbConfig,
           &packet[index],
           sizeof(nodebSrbConfig));
    index += sizeof(nodebSrbConfig);
    UmtsLayer3NodebReleaseSignalRb1To3(
            node,
            umtsL3,
            ueId,
            &nodebSrbConfig);


    int numRbs = (int) packet[index++];
    for (int i = 0; i < numRbs; i++)
    {
        UmtsNodebRbSetupPara nodebRbPara;
        memcpy(&nodebRbPara,
               &packet[index],
               sizeof(nodebRbPara));
        index += sizeof(nodebRbPara);

        if (!nodebRbPara.hsdpaRb)
        {
            UmtsLayer3NodebReleaseRb(
                    node,
                    umtsL3,
                    nodebRbPara);
        }
        else
        {
            UmtsLayer3NodebReleaseHsdpaRb(
                    node,
                    umtsL3,
                    nodebRbPara);
        }
    }

    Message* replyMsg = MESSAGE_Alloc(
                            node, 
                            NETWORK_LAYER, 
                            NETWORK_PROTOCOL_CELLULAR,
                            0);
    MESSAGE_PacketAlloc(node,
                        replyMsg,
                        sizeof(UInt32),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(replyMsg),
           &nodebL3->cellId,
           sizeof(UInt32));
    UmtsLayer3NodebSendNbapMessageToRnc(
        node,
        umtsL3,
        replyMsg,
        UMTS_NBAP_MSG_TYPE_LEAVE_ACTIVE_SET_RESPONSE,
        transctId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleNbapSwitchCell
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Join active set message
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the packet
// + ueId      : NodeId           : UE ID which RRC connection 
//                                  will be setup with
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebHandleNbapSwitchCell(
        Node* node,
        UmtsLayer3Data* umtsL3,
        Message* msg,
        NodeId ueId)
{
    size_t index = 0;
    char* packet = MESSAGE_ReturnPacket(msg);
    UInt32 uePrimScCode;
    memcpy(&uePrimScCode, &packet[index], sizeof(uePrimScCode));
    index += sizeof(uePrimScCode);
    BOOL enableCell = (BOOL)packet[index ++];

    if (DEBUG_HO)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\trcvd Switch Primary Cell Message for UE %d: "
            "primScCode: %u\n", ueId, uePrimScCode);
    }

    if (enableCell == FALSE)
    {
        //Empty RLC queues and store RLC status for this UE
        std::string buffer;
        UmtsRlcInitPrimCellChange(
            node,
            umtsL3->interfaceIndex,
            buffer,
            ueId);

        // Stop listening to the UE.
        UmtsMacPhyNodeBDisableSelfPrimCell(
                node,
                umtsL3->interfaceIndex,
                uePrimScCode);

        Message* replyMsg = MESSAGE_Alloc(node,
                                          NETWORK_LAYER,
                                          NETWORK_PROTOCOL_CELLULAR,
                                          0);
        MESSAGE_PacketAlloc(node,
                            replyMsg,
                            0,
                            TRACE_UMTS_LAYER3);
        MESSAGE_AddInfo(node,
                        replyMsg,
                        (int)(sizeof(UmtsCellSwitchInfo) + buffer.size()),
                        INFO_TYPE_UmtsCellSwitch);
        UmtsCellSwitchInfo* switchInfo =
            (UmtsCellSwitchInfo*)MESSAGE_ReturnInfo(
                                     replyMsg,
                                     INFO_TYPE_UmtsCellSwitch);
        switchInfo->contextBufSize = buffer.size();

        memcpy((char*)switchInfo + sizeof(UmtsCellSwitchInfo),
               buffer.data(),
               buffer.size());

        UmtsLayer3NodebSendNbapMessageToRnc(
            node,
            umtsL3,
            replyMsg,
            UMTS_NBAP_MSG_TYPE_SWITCH_CELL_RESPONSE,
            0,
            ueId);
    }
    else
    {
        UmtsCellSwitchInfo* switchInfo =
            (UmtsCellSwitchInfo*)MESSAGE_ReturnInfo(
                                     msg, 
                                     INFO_TYPE_UmtsCellSwitch);
        ERROR_Assert(switchInfo,
            "UmtsLayer3NodebHandleNbapSwitchCell: "
            "cannot get cell switch context info from "
            "switching response message.");

        BOOL restoreDone = FALSE;
        // restore RLC status
        UmtsRlcCompletePrimCellChange(
            node,
            umtsL3->interfaceIndex,
            (char*)switchInfo + sizeof(UmtsCellSwitchInfo),
            switchInfo->contextBufSize,
            ueId,
            &restoreDone);

        // start listening to the UE.
        if (restoreDone)
        {
            UmtsMacPhyNodeBEnableSelfPrimCell(
                node,
                umtsL3->interfaceIndex,
                uePrimScCode);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleIubMessage
// LAYER      :: Layer 3
// PURPOSE    :: Handle a protocol message from RNC, it may be a NBAP
//               message or a RRC message to be forwarded to an UE
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message containing the packet
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + destNodeId: NodeId           : The destination of the message
// + rbIdOrMsgType: UInt8         : RB ID or NBAP message type
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebHandleIubMessage(Node *node,
                                     Message *msg,
                                     UmtsLayer3Data *umtsL3,
                                     NodeId destNodeId,
                                     UInt8 rbIdOrMsgType,
                                     Address srcAddr)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);
    
    //If the destination node is ANY_NODEID, then it is a NBAP message
    if (destNodeId == ANY_NODEID)
    {
        UmtsNbapMessageType nbapMsgType =
            (UmtsNbapMessageType)rbIdOrMsgType;
        unsigned char transctId;
        NodeId ueId;
        UmtsRemoveNbapHeader(node,
                             msg,
                             &transctId,
                             &ueId);

        switch (nbapMsgType)
        {
            case UMTS_NBAP_MSG_TYPE_RRC_SETUP_REQUEST:
            {
                UmtsLayer3NodebHandleNbapRrcSetupRequest(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_RRC_SETUP_RELEASE:
            {
                UmtsLayer3NodebHandleNbapRrcSetupRelease(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_RADIO_BEARER_SETUP_REQUEST:
            {
                UmtsLayer3NodebHandleNbapRbSetup(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_RADIO_BEARER_RELEASE_REQUEST:
            {
                UmtsLayer3NodebHandleNbapRbRelease(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_JOIN_ACTIVE_SET_REQUEST:
            {
                UmtsLayer3NodebHandleNbapJoinActvSet(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_LEAVE_ACTIVE_SET_REQUEST:
            {
                UmtsLayer3NodebHandleNbapLeaveActvSet(
                    node,
                    umtsL3,
                    msg,
                    transctId,
                    ueId);
                break;
            }
            case UMTS_NBAP_MSG_TYPE_SWITCH_CELL_REQUEST:
            {
                UmtsLayer3NodebHandleNbapSwitchCell(
                    node,
                    umtsL3,
                    msg,
                    ueId);
                break;
            }
            default:
                ERROR_ReportError("UmtsLayer3NodebHandleIubMessage: "
                    "Received unknown type of message from RNC.");
        }
        MESSAGE_Free(node, msg);
    }
    //Otherwise, forwarding the message to the destination UE
    else
    {
        UInt8 rbId = rbIdOrMsgType;
        NodeId ueId = destNodeId;

//GuiStart
        if (node->guiOption == TRUE)
        {
            NodeAddress previousHopNodeId;

            previousHopNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                    node,
                                    srcAddr);
            GUI_Receive(previousHopNodeId, 
                        node->nodeId, 
                        GUI_NETWORK_LAYER,
                        GUI_DEFAULT_DATA_TYPE,
                        DEFAULT_INTERFACE,
                        DEFAULT_INTERFACE,
                        node->getNodeTime());
        }
//GuiEnd

        if (DEBUG_RR)
        {
            char timeStr[20];
            TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
            printf("%s, NodeB: %u is forwarding a RRC message "
                "from RNC: %u to UE: %d \n",
                timeStr, node->nodeId, nodebL3->rncInfo.nodeId, ueId);
#if 0
            printf("packet: \t\t");
            for (int i = 0; i < msg->packetSize; i++)
                printf("%d", (char) msg->packet[i]);
            printf("\n");
#endif
        }
        UmtsLayer2UpperLayerHasPacketToSend(node,
                                            umtsL3->interfaceIndex,
                                            rbId,
                                            msg,
                                            ueId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleSibTimer
// LAYER      :: Layer 3
// PURPOSE    :: Handle a SIB timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + nodebL3   : UmtsLayer3Nodeb *: NodeB data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodebHandleSibTimer(Node *node,
                                   UmtsLayer3Data *umtsL3,
                                   UmtsLayer3Nodeb *nodebL3,
                                   Message *msg)
{
    if (DEBUG_TIMER)
    {
        printf("Node %d (L3): handle SIB  timer\n", node->nodeId);
    }
    Message *sibMsg;
    char *sibPkt;
    UmtsMasterInfoBlock  mibPkt;
    char sysInfo[MAX_STRING_LENGTH];
    int  curPos = 0;

    // update SFN at L3
    // since the TTI for BCCH is 20ms.
    // step is 2
    nodebL3->pccpchSfn += 2;
    nodebL3->pccpchSfn = (nodebL3->pccpchSfn) % UMTS_MAX_SFN;

    if (!nodebL3->configured)
    {
        // not configured yet
        // scheudle next one
        MESSAGE_Send(node, msg, nodebL3->para.sysInfoBcstTime);

        return;
    }

    // build the SIB msg
    sibMsg = MESSAGE_Alloc(node,
                           NETWORK_LAYER,
                           NETWORK_PROTOCOL_CELLULAR,
                           0);


    // now only complete format are supported
    sysInfo[curPos ++] = (char)nodebL3->pccpchSfn;
    sysInfo[curPos ++] = (char)UMTS_SIB_COMPLETE;

    sysInfo[curPos ++]  = (char)UMTS_SIB_MASTER;
    mibPkt.plmnId = nodebL3->plmnId;
    mibPkt.plmnType = nodebL3->plmnType;
    mibPkt.valueTag = nodebL3->mibValue;
    mibPkt.cellId = nodebL3->cellId;

    // copy the master information block
    memcpy(&sysInfo[curPos],
           (char*)&mibPkt,
           sizeof(UmtsMasterInfoBlock));
    curPos += sizeof(UmtsMasterInfoBlock);

    // swtich sibType
    // TODO
    //mibPkt->plmnId = nodebL3->rncInfo.nodeId;
    // currently only 4 type sib are broadcasted
    nodebL3->sibSchCount = (++nodebL3->sibSchCount) % 4;

    // suppose one blokc one transmission
    if (nodebL3->sibSchCount == 0)
    {
        // build type 1
        //sibType
        // int blkLen;
        UmtsSibType1 sibType1;
        memset((char*)&sibType1, 0, sizeof(UmtsSibType1));

        sysInfo[curPos ++] = (char) UMTS_SIB_TYPE1;

        // TODO
        // blkLen = UmtsNodebBuildSib1(node, nodebL3, &sibType1);
        // copy the type1 sib, use blkLen when fully implementedd
        memcpy(&sysInfo[curPos],
               (char*)&sibType1,
               sizeof(UmtsSibType1));
        curPos += sizeof(UmtsSibType1);
        MESSAGE_PacketAlloc(node,
                            sibMsg,
                            curPos,
                            TRACE_UMTS_LAYER3);
    }
    else if (nodebL3->sibSchCount == 1)
    {
        // build type 2
        // int blkLen;
        UmtsSibType2 sibType2;
        memset((char*)&sibType2, 0, sizeof(UmtsSibType2));

        sibType2.regAreaId = nodebL3->regAreaId;

        sysInfo[curPos ++] = (char) UMTS_SIB_TYPE2;

        // TODO
        // blkLen = UmtsNodebBuildSib2(node, nodebL3, &sibType2);
        // copy the type1 sib, use blkLen when fully implementedd
        memcpy(&sysInfo[curPos],
               (char*)&sibType2,
               sizeof(UmtsSibType2));
        curPos += sizeof(UmtsSibType2);

        MESSAGE_PacketAlloc(node,
                        sibMsg,
                        curPos,
                        TRACE_UMTS_LAYER3);
    }
    else if (nodebL3->sibSchCount == 2)
    {
        // build type 3
        // int blkLen;
        UmtsSibType3 sibType3;
        memset((char*)&sibType3, 0, sizeof(UmtsSibType3));

        sysInfo[curPos ++] = (char) UMTS_SIB_TYPE3;

        // TODO
        // blkLen = UmtsNodebBuildSib3(node, nodebL3, &sibType3);
        // copy the type1 sib, use blkLen when fully implementedd
        // get ul ch index UmtsGetUlChIndex(node, umtsL3->interfaceIndex);
        sibType3.ulChIndex = UmtsGetUlChIndex(node, umtsL3->interfaceIndex);
        memcpy(&sysInfo[curPos],
               (char*)&sibType3,
               sizeof(UmtsSibType3));
        curPos += sizeof(UmtsSibType3);
        MESSAGE_PacketAlloc(node,
                        sibMsg,
                        curPos,
                        TRACE_UMTS_LAYER3);

    }
    else if (nodebL3->sibSchCount == 3)
    {
        // build type 5
        // int blkLen;
        UmtsSibType5 sibType5;
        memset((char*)&sibType5, 0, sizeof(UmtsSibType5));

        sysInfo[curPos ++] = (char) UMTS_SIB_TYPE5;

        // TODO
        // blkLen = UmtsNodebBuildSib5(node, nodebL3, &sibType5);
        // copy the type1 sib, use blkLen when fully implementedd
        memcpy(&sysInfo[curPos],
               (char*)&sibType5,
               sizeof(UmtsSibType5));
        curPos += sizeof(UmtsSibType5);
        MESSAGE_PacketAlloc(node,
                        sibMsg,
                        curPos,
                        TRACE_UMTS_LAYER3);
    }

    sibPkt = (char *) MESSAGE_ReturnPacket(sibMsg);
    ERROR_Assert(sibPkt != NULL, "UMTS: Memory error!");

    // copy the msg
    memcpy(sibPkt, sysInfo, curPos);
    UmtsLayer3AddHeader(
        node,
        sibMsg,
        UMTS_LAYER3_PD_RR,
        0,
        (unsigned char)UMTS_RR_MSG_TYPE_SYSTEM_INFORMATION_BCH);

    // send to RLC
    UmtsLayer2UpperLayerHasPacketToSend(node,
        umtsL3->interfaceIndex,
        UMTS_BCCH_RB_ID,
        sibMsg,
        node->nodeId); // use the nodeB itsef as ueId

    // scheudle next one
    MESSAGE_Send(node, msg, nodebL3->para.sysInfoBcstTime);
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandleTimerMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + nodebL3   : UmtsLayer3Nodeb *: NodeB data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg reused or not
// **/
static
BOOL UmtsLayer3NodebHandleTimerMsg(Node *node,
                                   UmtsLayer3Data *umtsL3,
                                   UmtsLayer3Nodeb *nodebL3,
                                   Message *msg)
{
    UmtsLayer3Timer* timerInfo;
    BOOL msgReuse = FALSE;

    timerInfo = (UmtsLayer3Timer*) MESSAGE_ReturnInfo(msg);

    switch (timerInfo->timerType)
    {
        case UMTS_LAYER3_TIMER_HELLO:
        {
            UmtsLayer3NodebSendHelloPacket(node, umtsL3);

            break;
        }
        case UMTS_LAYER3_TIMER_SIB:
        {
            UmtsLayer3NodebHandleSibTimer(node, umtsL3, nodebL3, msg);
            msgReuse = TRUE;

            break;
        }
        default:
        {
            char infoStr[MAX_STRING_LENGTH];
            sprintf(infoStr,
                    "UMTS Layer3: Node%d(NodeB) receives a Layer3 timer "
                    "with an unknown timer type as %d",
                    node->nodeId,
                    timerInfo->timerType);

            ERROR_ReportWarning(infoStr);
        }
    }
    return msgReuse;
}

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: UmtsLayer3NodebReportAmRlcError
// LAYER      :: Layer3
// PURPOSE    :: Called by RLC to report unrecoverable 
//               error for AM RLC entity.
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
// + ueId      : NodeId            : UE identifier
// + rbId      : char              : Radio bearer ID
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebReportAmRlcError(Node *node,
                                     UmtsLayer3Data* umtsL3,
                                     NodeId ueId,
                                     char rbId)
{
    if (DEBUG_RR)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, NodeB: %u is notified of an AM RLC error in "
            "RB: %d for UE: %u\n",
            timeStr, node->nodeId, rbId, ueId);
    }

    //Send a NBAP message to RNC to further report error
    Message* msg = MESSAGE_Alloc(node, 
                                 NETWORK_LAYER, 
                                 NETWORK_PROTOCOL_CELLULAR,
                                 0);
    MESSAGE_PacketAlloc(node, msg, sizeof(char), TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg), &rbId, sizeof(char));

    UmtsLayer3NodebSendNbapMessageToRnc(
        node,
        umtsL3,
        msg,
        UMTS_NBAP_MSG_TYPE_REPORT_AMRLC_ERROR,
        0,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3NodebReceivePacketFromMacLayer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface from which the packet
//                                          is received
// + umtsL3           : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg              : Message*          : Message for node to interpret.
// + lastHopAddress   : NodeAddress       : Address of the last hop
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebReceivePacketFromMacLayer(
    Node *node,
    int interfaceIndex,
    UmtsLayer3Data* umtsL3,
    Message *msg,
    NodeAddress lastHopAddress)
{
    UmtsLayer2PktToUpperlayerInfo* info =
        (UmtsLayer2PktToUpperlayerInfo*) MESSAGE_ReturnInfo(msg);

    unsigned int rbId = info->rbId;
    NodeId ueId = info->ueId;

    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);

//GuiStart
    if (node->guiOption == TRUE && rbId >= 
        UMTS_RBINRAB_START_ID && 
        rbId < UMTS_BCCH_RB_ID)
    {
        GUI_Receive(ueId, node->nodeId, GUI_NETWORK_LAYER,
                    GUI_DEFAULT_DATA_TYPE,
                    DEFAULT_INTERFACE,
                    interfaceIndex,
                    node->getNodeTime());
    }
//GuiEnd

    //Add an Iub header and forward the message to the RNC
    UmtsAddIubBackboneHeader(node,
                             msg,
                             ueId,
                             (UInt8)rbId);

    UmtsLayer3SendMsgOverBackbone(node,
                                  msg,
                                  nodebL3->rncInfo.nodeAddr,
                                  nodebL3->rncInfo.interfaceIndex,
                                  IPTOS_PREC_INTERNETCONTROL,
                                  1);
    if (DEBUG_RR )
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, NodeB: %u is forwarding a RRC message "
            "from UE: %d in RB: %d to RNC: %u\n",
            timeStr, node->nodeId, ueId, rbId,
            nodebL3->rncInfo.nodeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebHandlePacket
// LAYER      :: Layer 3
// PURPOSE    :: Handle packets received from lower layer
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message containing the packet
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + interfaceIndex : int         : Interface from which packet was received
// + srcAddr   : Address          : Address of the source node
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebHandlePacket(Node *node,
                                 Message *msg,
                                 UmtsLayer3Data *umtsL3,
                                 int interfaceIndex,
                                 Address srcAddr)
{
    UmtsBackboneMessageType backboneMsgType;
    char info[UMTS_BACKBONE_HDR_MAX_INFO_SIZE];
    int infoSize = UMTS_BACKBONE_HDR_MAX_INFO_SIZE;
    UmtsRemoveBackboneHeader(node,
                             msg,
                             &backboneMsgType,
                             info,
                             infoSize);

    switch (backboneMsgType)
    {
        case UMTS_BACKBONE_MSG_TYPE_IUB:
        {
            //Receive a message from NodeB
            NodeId srcNodeId;
            memcpy(&srcNodeId, info, sizeof(NodeId));
            UInt32 rbIdOrMsgType = info[sizeof(NodeId)];
            UmtsLayer3NodebHandleIubMessage(node,
                                            msg,
                                            umtsL3,
                                            srcNodeId,
                                            (UInt8)rbIdOrMsgType,
                                            srcAddr);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_HELLO:
        {
            // this is the hello packet for discovering connectivity
            char pd;
            char tiSpd;
            char msgType;
            UmtsLayer3RemoveHeader(node, msg, &pd, &tiSpd, &msgType);
            UmtsLayer3NodebHandleHelloPacket(node,
                                             msg,
                                             umtsL3,
                                             interfaceIndex,
                                             srcAddr);
            MESSAGE_Free(node, msg);
            break;
        }
        default:
        {
            char infoStr[MAX_STRING_LENGTH];
            sprintf(infoStr,
                    "UMTS Layer3: Node%d(RNC) receives a backbone message "
                    "with an unknown Backbone message type as %d",
                    node->nodeId,
                    backboneMsgType);
            ERROR_ReportWarning(infoStr);
        }
    }
}

//  /**
// FUNCITON   :: UmtsLayer3NodebHandleInterLayerCommand
// LAYER      :: UMTS L3 at NodeB
// PURPOSE    :: Handle Interlayer command
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + cmdType          : UmtsInterlayerCommandType : command type
// + interfaceIdnex   : UInt32            : interface index of UMTS
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
void UmtsLayer3NodebHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd)
{
}

// /**
// FUNCTION   :: UmtsLayer3NodeBInitDynamicStats
// LAYER      :: Layer3
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + interfaceIdnex   : UInt32 : interface index of UMTS
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + nodebL3   : UmtsLayer3Nodeb*   : Pointer to L3 UMTS NodeB data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3NodeBInitDynamicStats(Node *node,
                                     UInt32 interfaceIndex,
                                     UmtsLayer3Data *umtsL3,
                                     UmtsLayer3Nodeb *nodebL3)
{
    if (!node->guiOption)
    {
        return;
    }

    // UE In connected
    nodebL3->stat.numUeInConnectedGuiId =
        GUI_DefineMetric(
            "UMTS NodeB: Number of UEs in Connected in the Cell",
            node->nodeId,
            GUI_NETWORK_LAYER,
            interfaceIndex,
            GUI_INTEGER_TYPE,
            GUI_CUMULATIVE_METRIC);
}

// /**
// FUNCTION   :: UmtsLayer3NodebInit
// LAYER      :: Layer 3
// PURPOSE    :: Initialize NodeB data at UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebInit(Node *node,
                         const NodeInput *nodeInput,
                         UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Nodeb *nodebL3;

    // initialize the basic UMTS layer3 data
    nodebL3 = (UmtsLayer3Nodeb *) MEM_malloc(sizeof(UmtsLayer3Nodeb));
    ERROR_Assert(nodebL3 != NULL, "UMTS: Out of memory!");
    memset(nodebL3, 0, sizeof(UmtsLayer3Nodeb));

    // system infomation
    nodebL3->plmnType = PLMN_GSM_MAP;
    nodebL3->plmnId.mcc[0] = 1; // TODO give the right value
    nodebL3->plmnId.mnc[0] = 1; // TODO give the right value
    nodebL3->mibValue = 1; // start from 1
    nodebL3->duplexMode = UmtsGetDuplexMode(node, umtsL3->interfaceIndex);
    nodebL3->cellId = UmtsNodeBGetCellId(node, umtsL3->interfaceIndex);
    nodebL3->pccpchSfn = 0;

    // HSDPA capability
    nodebL3->hsdpaEnabled = UmtsLayer3GetHsdpaCapability(
                                node, umtsL3->interfaceIndex);

    umtsL3->dataPtr = (void *) nodebL3;

    // read configuration parameters
    UmtsLayer3NodebInitParameter(node, nodeInput, nodebL3);

    if (DEBUG_PARAMETER)
    {
        UmtsLayer3NodebPrintParameter(node, nodebL3);
    }

    // init dynamic stat
    UmtsLayer3NodeBInitDynamicStats(
        node, umtsL3->interfaceIndex, umtsL3, nodebL3);

    nodebL3->rncInfo.nodeId = ANY_DEST;

    // config common physical channels (radio links)
    UmtsLayer3NodeBConfigCommonRadioLink(node, umtsL3);

    // config common transport channels
    UmtsLayer3NodeBConfigCommonTrCh(node, umtsL3);

    // config commonradio bearers
    UmtsLayer3NodeBConfigCommonRadioBearer(node, umtsL3);

    // send hello packet to discover RNC
    UmtsLayer3SetTimer(node, umtsL3,
                       UMTS_LAYER3_TIMER_HELLO,
                       UMTS_LAYER3_TIMER_HELLO_START,
                       NULL, NULL, 0);

    // schedule System information broadcast timer
    // a diff is inroduced to make sure L3 SFN update and
    // SIB msg creation happened before L1 updating its own SFN
    // and this will synchronize the SFN at L1 and L3
    clocktype diffL3L1 = 100 * NANO_SECOND;
    nodebL3->sysInfoTimer = UmtsLayer3SetTimer(
                                node, umtsL3, UMTS_LAYER3_TIMER_SIB,
                                nodebL3->para.sysInfoBcstTime - diffL3L1,
                                NULL, NULL, 0);
}

// /**
// FUNCTION   :: UmtsLayer3NodebLayer
// LAYER      :: Layer 3
// PURPOSE    :: Handle NodeB specific timers and layer messages.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message for node to interpret
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);
    BOOL msgReused = FALSE;

    switch((msg->eventType))
    {
        case MSG_NETWORK_CELLULAR_TimerExpired:
        {
            msgReused =
                UmtsLayer3NodebHandleTimerMsg(node, umtsL3, nodebL3, msg);

            break;
        }
        default:
        {
            char infoStr[MAX_STRING_LENGTH];
            sprintf(infoStr,
                    "UMTS Layer3: Node%d(NodeB) receives a Layer3 msg "
                    "with an unknown protocol %d event type as %d",
                    node->nodeId,
                    MESSAGE_GetProtocol(msg),
                    msg->eventType);

            ERROR_ReportWarning(infoStr);
        }
    }

    if (!msgReused)
    {
        MESSAGE_Free(node, msg);
    }
}

// /**
// FUNCTION   :: UmtsLayer3NodebFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print NodeB specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebFinalize(Node *node, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Nodeb *nodebL3 = (UmtsLayer3Nodeb *) (umtsL3->dataPtr);

    if (umtsL3->collectStatistics)
    {
        // print general statistics
        UmtsLayer3NodebPrintStats(node, nodebL3);
    }

    if (nodebL3->sysInfoTimer)
    {
        MESSAGE_CancelSelfMsg(node, nodebL3->sysInfoTimer);
        nodebL3->sysInfoTimer = NULL;
    }
    MEM_free(nodebL3);
    umtsL3->dataPtr = NULL;
}
